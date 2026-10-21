#include "MandoHost.h"

#include <Windows.h>
#include <Xinput.h>

#include "ProveedorSala.h"
#include "../PhoenixPrefs.h"

#include <algorithm>

namespace phoenix {

	namespace {
		int mhActivo = 0;              // 1..8, 0 = ninguno
		bool mhConMando = false;       // true: lo mueve el mando físico del host («Tu mando»); false: el teclado

		void aplicar(ProveedorSala* sala) {
			if (mhActivo <= 0) return;
			mhConMando = sala->hayMandoPropio();
			if (mhConMando) sala->titereUnico(mhActivo - 1);   // sin bloquear: nadie más lo usa
			else sala->tomarControl(mhActivo - 1, true);
		}
		bool mhTeclaAntes[10] = {};

		bool mhAbajo(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

		// Sentarse solo: al abrir la sala y al pulsar A/B/X/Y en su mando sin puesto (solo con sitio libre; nunca saca a nadie)
		bool mhAbiertaAntes = false;
		bool mhBotonAntes = true;      // hay que soltar y volver a pulsar (no entra con un botón que ya estaba pulsado)
		ULONGLONG mhUltimoIntento = 0;
		ULONGLONG mhUltimoReenganche = 0;   // con la sala cerrada Link puede desenchufar los mandos: se vuelve a enchufar el del anfitrión
		ULONGLONG mhSentarHasta = 0;   // tras abrir la sala: hasta cuándo se intenta sentarlo (su mando puede tardar en aparecer)

		/// Puesto libre según «Mi puesto al jugar»: el preferido; si está ocupado, otro libre de su equipo; si no, cualquiera.
		int puestoLibre(ProveedorSala* sala) {
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			const std::vector<AsientoVista> as = sala->asientos(equipos::kPuestos);
			const int total = (std::min)(pr.mandosActivos, static_cast<int>(as.size()));
			// Un puesto con equipo (A o B) y libre; los «fuera» no se usan. Los equipos son los del mapa por puesto, no «los primeros N».
			const auto libre = [&](int n) { return n >= 1 && n <= total && pr.equipoDe(n - 1) != '-' && !as[n - 1].ocupado && n != mhActivo; };
			int destino = 0;   // 0 = el primer puesto libre de su equipo
			char equipo = 'A';
			if (pr.miPuestoModo == "b") equipo = 'B';
			else if (pr.miPuestoModo == "fijo") {
				destino = (std::max)(1, (std::min)(pr.miPuestoNumero, total));
				equipo = pr.equipoDe(destino - 1) == 'B' ? 'B' : 'A';
			}
			if (destino > 0 && libre(destino)) return destino;
			for (int n = 1; n <= total; n++) if (pr.equipoDe(n - 1) == equipo && libre(n)) return n;
			for (int n = 1; n <= total; n++) if (libre(n)) return n;
			return 0;
		}
	}

	int MandoHost::activo() { return mhActivo; }

	void MandoHost::soltar(ProveedorSala* sala) {
		if (mhActivo > 0 && sala != nullptr) {
			if (mhConMando) sala->titereUnico(-1);
			else sala->tomarControl(mhActivo - 1, false);
		}
		mhActivo = 0;
		mhConMando = false;
	}

	void MandoHost::tomar(ProveedorSala* sala, int n) {
		if (sala == nullptr || n < 0 || n > equipos::kPuestos) return;
		try {
			if (n == 0 || n == mhActivo) { soltar(sala); return; }
			soltar(sala);
			mhActivo = n;
			aplicar(sala);
		}
		catch (...) {}
	}

	void MandoHost::tick(ProveedorSala* sala) {
		if (sala == nullptr) return;
		try {
			// Atajos: Ctrl + Alt + número (flanco de subida, sin repetir)
			const bool mod = mhAbajo(VK_CONTROL) && mhAbajo(VK_MENU);
			for (int n = 0; n <= 8; n++) {
				const bool ahora = mod && mhAbajo('0' + n);
				if (ahora && !mhTeclaAntes[n]) {
					if (n == 0 || n == mhActivo) soltar(sala);
					else tomar(sala, n);
				}
				mhTeclaAntes[n] = ahora;
			}
			// Su mando físico guardado vuelve a quedar elegido al abrir Link (cada 2 s mientras no esté)
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			const ULONGLONG t = GetTickCount64();
			if (!pr.miMando.empty() && !sala->hayMandoPropio() && t - mhUltimoIntento > 2000) {
				mhUltimoIntento = t;
				sala->elegirMandoPropio(pr.miMando);
			}
			const bool abierta = sala->abierta();
			const bool conMando = sala->hayMandoPropio();
			const uint16_t abxy = conMando ? (sala->botonesMandoPropio() & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y)) : 0;
			if (abierta && !mhAbiertaAntes) mhSentarHasta = pr.sentarmeAlAbrir ? t + 15000 : 0;
			if (!abierta || mhActivo > 0) mhSentarHasta = 0;
			if (abierta && mhActivo == 0 && conMando) {
				const bool alAbrir = t < mhSentarHasta;
				const bool pulsado = pr.entrarPulsando && abxy != 0 && !mhBotonAntes;
				if (alAbrir || pulsado) {
					const int n = puestoLibre(sala);
					if (n > 0) tomar(sala, n);
					mhSentarHasta = 0;
				}
			}
			mhAbiertaAntes = abierta;
			mhBotonAntes = abxy != 0;

			// Con la sala cerrada el anfitrión mantiene su puesto (jugar solo, probar el mando): no se suelta al cerrar.
			if (mhActivo == 0) return;
			if (t - mhUltimoReenganche > 1000) {
				mhUltimoReenganche = t;
				const std::vector<AsientoVista> as = sala->asientos(equipos::kPuestos);
				if (static_cast<size_t>(mhActivo - 1) < as.size() && !as[static_cast<size_t>(mhActivo - 1)].conectado) aplicar(sala);
			}

			// Si el host elige o quita «Tu mando» mientras está en un puesto, se cambia de modo sin moverlo de puesto.
			if (sala->hayMandoPropio() != mhConMando) {
				if (mhConMando) sala->titereUnico(-1);
				else sala->tomarControl(mhActivo - 1, false);
				aplicar(sala);
			}
			if (mhConMando) return;   // lo mueve su mando físico: el teclado no se inyecta

			// Teclado del host → mando tomado
			uint16_t b = 0;
			int16_t lx = 0, ly = 0;
			if (!mod) {
				if (mhAbajo(VK_LEFT))  { lx = -32767; b |= XINPUT_GAMEPAD_DPAD_LEFT; }
				if (mhAbajo(VK_RIGHT)) { lx = 32767;  b |= XINPUT_GAMEPAD_DPAD_RIGHT; }
				if (mhAbajo(VK_UP))    { ly = 32767;  b |= XINPUT_GAMEPAD_DPAD_UP; }
				if (mhAbajo(VK_DOWN))  { ly = -32767; b |= XINPUT_GAMEPAD_DPAD_DOWN; }
				if (mhAbajo(VK_RETURN)) b |= XINPUT_GAMEPAD_A;
				if (mhAbajo(VK_BACK))   b |= XINPUT_GAMEPAD_B;
				if (mhAbajo(VK_SPACE))  b |= XINPUT_GAMEPAD_START;
			}
			sala->inyectar(mhActivo - 1, b, lx, ly);
		}
		catch (...) {}
	}

}
