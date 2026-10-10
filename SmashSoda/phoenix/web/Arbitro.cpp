#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../core/Config.h"
#include "../PhoenixPrefs.h"
#include "../PhoenixRoles.h"
#include "../core/ProveedorSala.h"
#include "../core/MandoHost.h"
#include "../link/PhoenixLink.h"

// =============================================================================
//  Phoenix Link · Árbitro del partido (desde FUERA del juego)
// -----------------------------------------------------------------------------
//  Lee la fase del partido de estado.json (phoenix.lua) o, si no hay datos del
//  juego, del marcador de Link. Con el partido en juego:
//   · Modo competitivo: Start, Back y Guía bloqueados a los invitados por el
//     mismo camino que «Bloquear botones» (GamepadClient::lockButtons +
//     Hosting::_lockedGamepad). La máscara del anfitrión NO se toca en disco:
//     al salir se vuelve a la suya (Config) y a su interruptor previo.
//   · Pausa automática (preferencia, apagada por defecto): si un jugador de un
//     lado se cae o pasa 150 ms de ping 5 s seguidos → aviso en el chat y pausa
//     del marcador. Solo con estado.json en «en_juego» pulsa Start una vez en
//     un mando virtual libre (o el del anfitrión) para pausar PES de verdad.
//   · Goles: si el juego da el marcador, el de Link lo sigue (cuando cambia en
//     el juego); la corrección manual sigue funcionando.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_arbitro {
		constexpr unsigned int kBotonesCompetitivo = 0x0010 | 0x0020 | 0x0400;   // Start | Back | Guía
		constexpr uint16_t kBotonStart = 0x0010;
		constexpr int kPingAlto = 150;      // ms
		constexpr size_t kSegundosPing = 5;  // muestras seguidas (1 por segundo)

		/// Copia la máscara del anfitrión (Config) y le suma lo del modo competitivo si está activo.
		void aplicarBloqueo(Interno& in, Hosting& h) {
			const auto& c = Config::cfg.input;
			LockedGamepadState& l = h._lockedGamepad;
			if (in.arbitro.competitivoActivo && !in.arbitro.lockPrevio) {
				// El anfitrión no bloqueaba nada: solo Start/Back/Guía (sus gatillos y sticks quedan libres)
				l.wButtons = kBotonesCompetitivo;
				l.bLeftTrigger = false; l.bRightTrigger = false;
				l.sThumbLX = false; l.sThumbLY = false; l.sThumbRX = false; l.sThumbRY = false;
				return;
			}
			l.wButtons = c.lockedGamepadButtons | (in.arbitro.competitivoActivo ? kBotonesCompetitivo : 0u);
			l.bLeftTrigger = c.lockedGamepadLeftTrigger;
			l.bRightTrigger = c.lockedGamepadRightTrigger;
			l.sThumbLX = c.lockedGamepadLX;
			l.sThumbLY = c.lockedGamepadLY;
			l.sThumbRX = c.lockedGamepadRX;
			l.sThumbRY = c.lockedGamepadRY;
		}

		/// Start una vez en un mando virtual libre (o el del anfitrión). Nunca en el que el host maneja con Ctrl+Alt (lo pisaría).
		bool pulsarStart(Interno& in, ProveedorSala& sala, Hosting& h, double ahora) {
			const std::vector<AsientoVista> asientos = sala.asientos(8);
			const int tomado = MandoHost::activo() - 1;
			uint32_t idHost = 0;
			try { idHost = h.getHost().userID; } catch (...) {}
			int elegido = -1;
			for (size_t i = 0; i < asientos.size() && elegido < 0; i++) {
				if (asientos[i].conectado && !asientos[i].ocupado && static_cast<int>(i) != tomado) elegido = static_cast<int>(i);
			}
			for (size_t i = 0; i < asientos.size() && elegido < 0; i++) {
				if (asientos[i].conectado && asientos[i].ocupado && idHost != 0 && asientos[i].parsecId == idHost && static_cast<int>(i) != tomado) elegido = static_cast<int>(i);
			}
			if (elegido < 0) return false;
			sala.inyectar(elegido, kBotonStart, 0, 0);
			in.arbitro.pulsando = elegido;
			in.arbitro.soltarEn = ahora + 0.15;
			return true;
		}

		/// Texto del problema de un jugador del partido ("" = está bien).
		std::string problemaDe(Interno& in, const JugadorPartido& j, const std::vector<ResumenRed>& red) {
			if (!in.presentesAntes.count(j.parsecId)) return j.nombre + " se desconectó";
			for (const ResumenRed& r : red) {
				if (r.parsecId != j.parsecId) continue;
				if (r.serie.size() < kSegundosPing) return "";
				for (size_t k = r.serie.size() - kSegundosPing; k < r.serie.size(); k++) {
					if (r.serie[k] <= kPingAlto) return "";   // -1 (sin dato) tampoco cuenta como ping alto
				}
				return j.nombre + " tiene el ping alto (" + std::to_string(r.ultimo) + " ms)";
			}
			return "";
		}
	}

	namespace detalle_arbitro {
		/// Nombre del lado de Link que juega con el equipo local (o visitante) del juego.
		std::string nombreEquipo(Interno& in, const juego::EstadoPartidoJuego& j, bool localDelJuego) {
			const bool ladoA = localDelJuego != in.partido.mandosInvertidos();
			const std::string& n = ladoA ? in.partido.ladoA().nombre : in.partido.ladoB().nombre;
			if (!n.empty()) return n;
			return localDelJuego ? (j.nombreLocal.empty() ? std::string("Local") : j.nombreLocal)
				: (j.nombreVisita.empty() ? std::string("Visita") : j.nombreVisita);
		}

		std::string resumenMarcador(Interno& in) {
			return in.partido.ladoA().nombre + " " + std::to_string(in.partido.ladoA().goles) + "-"
				+ std::to_string(in.partido.ladoB().goles) + " " + in.partido.ladoB().nombre;
		}

		/// «12' Local, 34' Visita» (máx. 12 goles; el resto se resume).
		std::string lineaGoles(Interno& in, const juego::EstadoPartidoJuego& j) {
			std::string r;
			for (size_t i = 0; i < j.goles.size() && i < 12; i++) {
				if (!r.empty()) r += ", ";
				r += std::to_string(j.goles[i].minuto) + "' " + nombreEquipo(in, j, j.goles[i].local);
			}
			if (j.goles.size() > 12) r += " y " + std::to_string(j.goles.size() - 12) + " más";
			return r;
		}

		/// Texto de una línea de sala.txt: sin saltos, tabuladores ni «|» y corto (UTF-8 entero).
		std::string limpioSala(const std::string& s, size_t maximo) {
			std::string r;
			for (char c : s) {
				const unsigned char u = static_cast<unsigned char>(c);
				if (u < 0x20 || c == '|' || u == 0x7F) { if (!r.empty() && r.back() != ' ') r += ' '; continue; }
				r += c;
			}
			return phoenix::buzon::cortarUtf8(r, maximo);
		}

		/// sala.txt (Link → juego): líneas clave=valor que lee phoenix_estadio.lua para su HUD.
		std::string textoSalaJuego(Interno& in, ProveedorSala& sala, Hosting& h) {
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			std::string t = "v=1\n";
			if (!sala.abierta()) return t + "abierta=0\n";
			t += "abierta=1\n";
			t += "sala=" + limpioSala(sala.nombreSala(), 40) + "\n";
			const FasePartido fp = in.partido.fase();
			const bool hayLados = fp != FasePartido::Libre;
			const bool inv = in.partido.mandosInvertidos();
			const LadoPartido& ll = inv ? in.partido.ladoB() : in.partido.ladoA();
			const LadoPartido& lv = inv ? in.partido.ladoA() : in.partido.ladoB();
			t += "local=" + (hayLados && !ll.nombre.empty() ? limpioSala(ll.nombre, 24) : std::string("Local")) + "\n";
			t += "visita=" + (hayLados && !lv.nombre.empty() ? limpioSala(lv.nombre, 24) : std::string("Visita")) + "\n";
			t += std::string("marcador=") + Partido::nombreFase(fp) + "\n";
			if (hayLados) t += "goles=" + std::to_string(ll.goles) + "-" + std::to_string(lv.goles) + "\n";
			// Jugadores por mando: los primeros «equipoLocal» mandos son del local (como la formación del juego)
			const std::vector<AsientoVista> asientos = sala.asientos(8);
			const int activos = (std::min)(static_cast<int>(asientos.size()), pr.mandosActivos);
			for (int i = 0; i < activos; i++) {
				const AsientoVista& s = asientos[static_cast<size_t>(i)];
				if (!s.ocupado) continue;
				const int ping = s.pingMs < 0 ? -1 : (s.pingMs + 5) / 10 * 10;   // a 10 ms: el archivo no cambia por nada
				t += (i < pr.equipoLocal ? "ja=" : "jb=") + limpioSala(s.jugador, 20) + "|" + std::to_string(ping) + "|" + std::to_string(s.numero) + "\n";
			}
			std::string arb;
			if (in.arbitro.competitivoActivo) arb = "Start/Back/Guía bloqueados a los invitados";
			if (pr.pausaAuto) arb += std::string(arb.empty() ? "" : " · ") + "pausa automática";
			if (!arb.empty()) t += "arbitro=" + arb + "\n";
			const std::vector<std::string>& chat = h.getMessageLog();
			for (size_t i = chat.size() > 4 ? chat.size() - 4 : 0; i < chat.size(); i++) t += "chat=" + limpioSala(chat[i], 90) + "\n";
			return t;
		}
	}

	using namespace detalle_arbitro;

	unsigned int mascaraBotonesPropia(Interno& in) {
		if (in.ctx.hosting == nullptr) return 0;
		return in.arbitro.competitivoActivo ? Config::cfg.input.lockedGamepadButtons
			: static_cast<unsigned int>(in.ctx.hosting->_lockedGamepad.wButtons);
	}

	bool bloqueoBotonesPropio(Interno& in) {
		if (in.ctx.hosting == nullptr) return false;
		return in.arbitro.competitivoActivo ? in.arbitro.lockPrevio : in.ctx.hosting->getGamepadClient().lockButtons;
	}

	void alternarBloqueoBotones(Interno& in) {
		Hosting& h = hostingObligatorio(in);
		if (!in.arbitro.competitivoActivo) { h.toggleGamepadLockButtons(); return; }
		// En modo competitivo el interruptor sigue encendido: se cambia el del anfitrión, que vuelve al terminar
		in.arbitro.lockPrevio = !in.arbitro.lockPrevio;
		aplicarBloqueo(in, h);
	}

	void reaplicarArbitro(Interno& in) {
		if (in.ctx.hosting != nullptr && in.arbitro.competitivoActivo) aplicarBloqueo(in, *in.ctx.hosting);
	}

	void tickArbitro(Interno& in) {
		if (in.ctx.sala == nullptr || in.ctx.hosting == nullptr) return;
		ProveedorSala& sala = *in.ctx.sala;
		Hosting& h = *in.ctx.hosting;
		Interno::Arbitro& a = in.arbitro;
		const double ahora = ahoraSeg();
		try {
			// Soltar el Start de la pausa automática (~150 ms después)
			if (a.pulsando >= 0 && ahora >= a.soltarEn) {
				sala.inyectar(a.pulsando, 0, 0, 0);
				a.pulsando = -1;
			}
			if (ahora - a.ultimoTick < 0.5) return;
			a.ultimoTick = ahora;

			const PhoenixPrefs& pr = PhoenixPrefs::get();
			const juego::EstadoPartidoJuego j = PhoenixLink::instancia().estadoJuego();
			const FasePartido fp = in.partido.fase();
			const bool marcadorVivo = fp == FasePartido::EnJuego || fp == FasePartido::Pausado;
			const bool enJuego = j.valido ? j.fase == "en_juego" : fp == FasePartido::EnJuego;
			a.origen = j.valido ? "juego" : (marcadorVivo ? "marcador" : "");

			// 0) Puestos: quien está sentado (por el host, turnos o torneo) queda autorizado para volver a su mando
			PhoenixRoles::instancia().soloAutorizados(pr.soloAutorizados);
			for (const AsientoVista& s : sala.asientos(8)) if (s.ocupado) PhoenixRoles::instancia().autorizar(s.parsecId);

			// 1) Modo competitivo
			GamepadClient& gc = h.getGamepadClient();
			const bool debe = pr.competitivo && sala.abierta() && enJuego;
			if (debe && !a.competitivoActivo) {
				a.lockPrevio = gc.lockButtons;
				a.competitivoActivo = true;
				aplicarBloqueo(in, h);
				gc.lockButtons = true;
			}
			else if (!debe && a.competitivoActivo) {
				a.competitivoActivo = false;
				aplicarBloqueo(in, h);
				gc.lockButtons = a.lockPrevio;
			}
			else if (a.competitivoActivo && !gc.lockButtons) gc.lockButtons = true;   // lo apagó otro panel: sigue mandando el árbitro

			// HUD del juego (sala.txt): quién juega en cada lado, su ping, el árbitro y el chat
			PhoenixLink::instancia().salaJuego(textoSalaJuego(in, sala, h));

			// 2a) Arranque automático: lados ya elegidos (Listo) y el reloj del juego echa a andar
			bool vivo = marcadorVivo;
			if (pr.marcadorAuto && fp == FasePartido::Listo && sala.abierta() && j.valido && j.fase == "en_juego" && j.relojCorre
				&& in.partido.iniciar(ahoraEpochMs(), ahora)) {
				PhoenixLink::instancia().marcarPartido(true);
				PhoenixLink::instancia().evento("partida_inicio", "", json{ {"marcador", in.partido.marcador()}, {"auto", true} }.dump());
				mensajeDelBot(in, "¡Arrancó! " + in.partido.ladoA().nombre + " vs " + in.partido.ladoB().nombre + ". El marcador va solo con el juego.");
				a.golesAnunciados = static_cast<int>(j.goles.size());
				a.golesJuegoL = a.golesJuegoV = -1;
				vivo = true;
			}

			// 2b) Goles que da el juego (solo cuando cambian allí; la corrección manual se respeta)
			if (vivo && j.valido && j.golesLocal >= 0 && j.golesVisita >= 0) {
				if (j.golesLocal != a.golesJuegoL || j.golesVisita != a.golesJuegoV) {
					a.golesJuegoL = j.golesLocal;
					a.golesJuegoV = j.golesVisita;
					// El lado A juega con los mandos del local, salvo tras «Cambiar lados»
					const bool inv = in.partido.mandosInvertidos();
					in.partido.fijarGoles(inv ? j.golesVisita : j.golesLocal, inv ? j.golesLocal : j.golesVisita);
				}
			}
			else if (!vivo) {
				a.golesJuegoL = a.golesJuegoV = -1;
			}

			// 2c) Relato en el chat: goles con minuto, descanso y final (marcador automático)
			if (!vivo) a.golesAnunciados = -1;
			if (vivo && j.valido && pr.marcadorAuto) {
				const int total = static_cast<int>(j.goles.size());
				if (a.golesAnunciados < 0 || total < a.golesAnunciados) a.golesAnunciados = total;   // primera vez o gol anulado: sin anunciar
				for (int i = a.golesAnunciados; i < total; i++) {
					const juego::GolJuego& g = j.goles[static_cast<size_t>(i)];
					mensajeDelBot(in, "GOL " + std::to_string(g.minuto) + "' de " + nombreEquipo(in, j, g.local) + ". "
						+ in.partido.ladoA().nombre + " " + std::to_string(in.partido.ladoA().goles) + "-"
						+ std::to_string(in.partido.ladoB().goles) + " " + in.partido.ladoB().nombre);
				}
				a.golesAnunciados = total;
				if (j.fase == "descanso" && a.faseJuego == "en_juego") mensajeDelBot(in, "Descanso: " + resumenMarcador(in) + ".");
				if (j.fase == "final" && j.completo) {
					const std::string goles = lineaGoles(in, j);
					finalizarPartido(in, true, goles.empty() ? std::string() : " Goles: " + goles + ".");
					vivo = false;
				}
				else if (j.fase == "menu" && a.faseJuego == "en_juego" && !a.avisoAbandono) {
					a.avisoAbandono = true;
					mensajeDelBot(in, "El partido se dejó a medias en el juego. El marcador de Link sigue abierto: termínalo o cancélalo.");
				}
			}
			if (j.valido) {
				if (j.fase == "en_juego") a.avisoAbandono = false;
				a.faseJuego = j.fase;
			}

			// 3) Pausa automática
			if (!vivo) { a.avisados.clear(); a.startPendienteHasta = 0.0; return; }
			// Start que esperaba a que el reloj corriera (no se pulsa en repeticiones ni celebraciones: las saltaría)
			if (a.startPendienteHasta > 0.0) {
				if (ahora > a.startPendienteHasta || in.partido.fase() != FasePartido::Pausado || !j.valido) a.startPendienteHasta = 0.0;
				else if (j.fase == "en_juego" && j.relojCorre && pulsarStart(in, sala, h, ahora)) {
					a.startPendienteHasta = 0.0;
					a.ultimaConStart = true;
					mensajeDelBot(in, "Juego en pausa.");
				}
			}
			if (!pr.pausaAuto || !sala.abierta()) return;
			uint32_t idHost = 0;
			try { idHost = h.getHost().userID; } catch (...) {}
			const std::vector<ResumenRed> red = in.red.resumen();
			std::vector<std::string> nuevos;
			for (const LadoPartido* lado : { &in.partido.ladoA(), &in.partido.ladoB() }) {
				for (const JugadorPartido& jp : lado->jugadores) {
					if (jp.parsecId == 0 || jp.parsecId == idHost) continue;
					const std::string p = problemaDe(in, jp, red);
					if (p.empty()) { a.avisados.erase(jp.parsecId); continue; }
					if (a.avisados.insert(jp.parsecId).second) nuevos.push_back(p);
				}
			}
			if (nuevos.empty()) return;
			std::string motivo = nuevos.front();
			for (size_t i = 1; i < nuevos.size(); i++) motivo += "; " + nuevos[i];
			if (in.partido.fase() == FasePartido::EnJuego) {
				in.partido.pausar(true, ahora);
				PhoenixLink::instancia().evento("pausa", "", json{ {"marcador", in.partido.marcador()}, {"auto", true} }.dump());
				const bool juegoEnJuego = j.valido && j.fase == "en_juego";
				const bool conStart = juegoEnJuego && j.relojCorre && pulsarStart(in, sala, h, ahora);
				if (juegoEnJuego && !conStart && !j.relojCorre) a.startPendienteHasta = ahora + 30.0;   // se pulsa cuando vuelva a correr el reloj
				a.ultimaPausa = motivo;
				a.ultimaConStart = conStart;
				mensajeDelBot(in, "Pausa automática: " + motivo + (conStart ? ". Juego en pausa."
					: a.startPendienteHasta > 0.0 ? ". Marcador en pausa; el juego se pausa en cuanto vuelva a correr el reloj." : ". Marcador en pausa."));
			}
			else {
				mensajeDelBot(in, "Aviso del árbitro: " + motivo + ".");
			}
		}
		catch (...) {
			// El árbitro nunca debe tumbar la interfaz.
		}
	}

}
