#include "MandoHost.h"

#include <Windows.h>
#include <Xinput.h>

#include "ProveedorSala.h"

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
		if (sala == nullptr || n < 0 || n > 8) return;
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
			if (mhActivo == 0 || !sala->abierta()) { if (mhActivo) soltar(sala); return; }

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
