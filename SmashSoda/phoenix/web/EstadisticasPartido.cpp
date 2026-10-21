#include "EstadisticasPartido.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "InterfazWebInterno.h"
#include "SalaClasificatoria.h"
#include "../../Hosting.h"
#include "../core/ProveedorSala.h"
#include "../core/EquiposPuestos.h"

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_estadisticas {
		constexpr double kEstMuestreo = 0.05;     // 20 veces por segundo: alcanza para contar pulsaciones
		constexpr double kEstSaltoMax = 0.5;      // un parón de la ventana no cuenta como tiempo jugado
		constexpr int kEstZonaMuerta = 8000;      // stick quieto (de −32768…32767)
		constexpr int kEstGatilloPulsado = 30;    // 0–255
		constexpr int kEstPingAlto = 150;         // ms
		constexpr uint16_t kEstBotonStart = 0x0010;

		struct Ficha {
			int mando = 0;                     // último mando (1 = primero); 0 = nunca tuvo
			int cambiosMando = 0;
			double segMando = 0.0;             // en juego con mando
			double segFuera = 0.0;             // fuera de la sala
			int caidas = 0;
			bool presente = true;
			long long pingSuma = 0;
			int pingMuestras = 0, pingMax = -1, segLag = 0;
			int pulsaciones = 0, starts = 0;
			double segInactivo = 0.0, inactivoSeguido = 0.0, inactivoMax = 0.0;
			uint16_t botonesAntes = 0;
			bool hayAntes = false;
		};

		struct Estado {
			int64_t partido = 0;               // inicioMs del partido medido
			double ultimo = 0.0, ultimoSegundo = 0.0;
			std::map<uint32_t, Ficha> fichas;
		};

		Estado& estado() {
			static Estado e;
			return e;
		}

		int bitsEncendidos(uint16_t v) {
			int n = 0;
			for (; v != 0; v &= static_cast<uint16_t>(v - 1)) n++;
			return n;
		}

		bool fueraDeZona(int v) { return v > kEstZonaMuerta || v < -kEstZonaMuerta; }
	}

	using namespace detalle_estadisticas;

	void tickEstadisticas(Interno& in) {
		tickClasificatoria(in);   // va aquí para no tocar otra vez el bucle de InterfazWeb
		if (!kFichaJugadorActiva || in.ctx.sala == nullptr) return;
		try {
			const FasePartido fase = in.partido.fase();
			if (fase != FasePartido::EnJuego && fase != FasePartido::Pausado) return;   // la última ficha se queda hasta el siguiente partido
			Estado& e = estado();
			const double ahora = ahoraSeg();
			if (in.partido.inicioMs() != e.partido) {
				e = Estado();
				e.partido = in.partido.inicioMs();
				e.ultimo = e.ultimoSegundo = ahora;
				for (const LadoPartido* l : { &in.partido.ladoA(), &in.partido.ladoB() })
					for (const JugadorPartido& j : l->jugadores) if (j.parsecId != 0) e.fichas[j.parsecId] = Ficha();
			}
			if (fase != FasePartido::EnJuego) { e.ultimo = ahora; return; }   // la pausa no cuenta
			const double dt = (std::min)(kEstSaltoMax, ahora - e.ultimo);
			if (dt < kEstMuestreo) return;
			e.ultimo = ahora;
			const bool nuevoSegundo = ahora - e.ultimoSegundo >= 1.0;
			if (nuevoSegundo) e.ultimoSegundo = ahora;

			uint32_t idHost = 0;
			try { if (in.ctx.hosting != nullptr) idHost = in.ctx.hosting->getHost().userID; } catch (...) {}
			const std::vector<AsientoVista> asientos = in.ctx.sala->asientos(equipos::kPuestos);

			for (auto& par : e.fichas) {
				const uint32_t id = par.first;
				Ficha& f = par.second;
				const bool presente = id == idHost || in.presentesAntes.count(id) > 0;
				if (!presente) {
					f.segFuera += dt;
					if (f.presente) f.caidas++;
				}
				f.presente = presente;

				const AsientoVista* s = nullptr;
				for (const AsientoVista& a : asientos) if (a.ocupado && a.parsecId == id) { s = &a; break; }
				if (s == nullptr) { f.hayAntes = false; continue; }

				if (f.mando != s->numero) {
					if (f.mando != 0) f.cambiosMando++;
					f.mando = s->numero;
				}
				f.segMando += dt;

				// Pulsaciones = botones que pasan de suelto a pulsado
				const uint16_t nuevos = static_cast<uint16_t>(s->botones & ~f.botonesAntes);
				if (f.hayAntes) {
					f.pulsaciones += bitsEncendidos(nuevos);
					if (nuevos & kEstBotonStart) f.starts++;
				}
				f.botonesAntes = s->botones;
				f.hayAntes = true;

				// Inactivo = nada pulsado y sticks en reposo
				const bool activo = s->botones != 0 || s->gatilloI > kEstGatilloPulsado || s->gatilloD > kEstGatilloPulsado
					|| fueraDeZona(s->ejeLX) || fueraDeZona(s->ejeLY) || fueraDeZona(s->ejeRX) || fueraDeZona(s->ejeRY);
				if (activo) f.inactivoSeguido = 0.0;
				else {
					f.segInactivo += dt;
					f.inactivoSeguido += dt;
					f.inactivoMax = (std::max)(f.inactivoMax, f.inactivoSeguido);
				}

				// Ping: una muestra por segundo (el del anfitrión no significa nada)
				if (nuevoSegundo && id != idHost && s->pingMs >= 0) {
					f.pingSuma += s->pingMs;
					f.pingMuestras++;
					f.pingMax = (std::max)(f.pingMax, s->pingMs);
					if (s->pingMs > kEstPingAlto) f.segLag++;
				}
			}
		}
		catch (...) {
			// Las estadísticas nunca deben tumbar la interfaz.
		}
	}

	json fichaJugador(int64_t inicioMs, uint32_t parsecId) {
		if (!kFichaJugadorActiva) return json::object();
		const Estado& e = estado();
		if (e.partido != inicioMs) return json::object();
		const auto it = e.fichas.find(parsecId);
		if (it == e.fichas.end()) return json::object();
		const Ficha& f = it->second;
		const int segMando = static_cast<int>(std::lround(f.segMando));
		json r = {
			{"mando", f.mando},
			{"cambios_mando", f.cambiosMando},
			{"seg_mando", segMando},
			{"seg_fuera", static_cast<int>(std::lround(f.segFuera))},
			{"caidas", f.caidas},
			{"pulsaciones", f.pulsaciones},
			{"pulsaciones_min", f.segMando >= 30.0 ? static_cast<int>(std::lround(f.pulsaciones * 60.0 / f.segMando)) : 0},
			{"start", f.starts},
			{"seg_inactivo", static_cast<int>(std::lround(f.segInactivo))},
			{"inactivo_max_seg", static_cast<int>(std::lround(f.inactivoMax))},
		};
		if (f.pingMuestras > 0) {
			r["ping_medio"] = static_cast<int>(f.pingSuma / f.pingMuestras);
			r["ping_max"] = f.pingMax;
			r["seg_lag"] = f.segLag;
		}
		return r;
	}

}
