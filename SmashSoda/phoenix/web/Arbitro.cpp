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
		constexpr double kLatidoVivoSeg = 120.0;   // el marcador en vivo se reenvía al menos cada 2 min (la web oculta los de >10 min sin noticias)
		double vivoEnviadoSeg = -1e9;              // último envío del marcador en vivo (ahoraSeg)
		double autoArmarEsperaHasta = 0.0;         // armar lados solo al pitazo: no reintentar antes de esto
		double anfitrionPulsoSeg = -1e9;           // última vez que el anfitrión tocó su mando físico (ahoraSeg)

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

		/// « (MESSI)», « (MESSI, en propia)» o «» si el juego no dijo quién (Sider 7.3.3).
		std::string quienMarco(const juego::GolJuego& g) {
			if (g.jugador.empty()) return std::string();
			return " (" + g.jugador + (g.propia ? ", en propia)" : ")");
		}

		/// Goles para la web: [{m, l, j?, pp?}] con «l» desde el lado A (local del calendario).
		json golesParaWeb(const juego::EstadoPartidoJuego& j, bool invertido) {
			json gs = json::array();
			for (const juego::GolJuego& g : j.goles) {
				json x = { {"m", g.minuto}, {"l", (g.local != invertido) ? "l" : "v"} };
				if (!g.jugador.empty()) x["j"] = g.jugador;
				if (g.propia) x["pp"] = true;
				gs.push_back(std::move(x));
			}
			return gs;
		}

		/// Cuántos goles traen ya el nombre del goleador (para reenviar el «en vivo» cuando llega uno).
		int golesConNombre(const juego::EstadoPartidoJuego& j) {
			int n = 0;
			for (const juego::GolJuego& g : j.goles) if (!g.jugador.empty()) n++;
			return n;
		}

		/// «12' Local (MESSI), 34' Visita» (máx. 12 goles; el resto se resume).
		std::string lineaGoles(Interno& in, const juego::EstadoPartidoJuego& j) {
			std::string r;
			for (size_t i = 0; i < j.goles.size() && i < 12; i++) {
				if (!r.empty()) r += ", ";
				r += std::to_string(j.goles[i].minuto) + "' " + nombreEquipo(in, j, j.goles[i].local) + quienMarco(j.goles[i]);
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

			// El anfitrión usa su mando físico (TU MANDO)? Se anota cuándo, para saber si juega aunque no esté en un puesto
			if (sala.hayMandoPropio() && sala.botonesMandoPropio() != 0) anfitrionPulsoSeg = ahora;

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

			// 2a0) Sin lados elegidos: al pitazo inicial (reloj en marcha, minuto 0–1) Link los arma solo con quien tiene
			//      mando, según la formación (los primeros «equipoLocal» mandos son el local; un lado vacío = la CPU).
			//      Si en el juego se eligió el otro lado, CORREGIR LADOS lo arregla sin mover mandos.
			if (pr.marcadorAuto && fp == FasePartido::Libre && sala.abierta() && j.valido && j.fase == "en_juego" && j.relojCorre
				&& j.minuto <= 1 && ahora >= autoArmarEsperaHasta) {
				autoArmarEsperaHasta = ahora + 10.0;   // si no se puede (nadie con mando), se vuelve a mirar en 10 s
				LadoPartido la, lb;
				const std::vector<AsientoVista> asientos = sala.asientos(8);
				const int activos = (std::min)(static_cast<int>(asientos.size()), pr.mandosActivos);
				for (int i = 0; i < activos; i++) {
					const AsientoVista& s = asientos[static_cast<size_t>(i)];
					if (!s.ocupado || s.parsecId == 0) continue;
					LadoPartido& l = i < pr.equipoLocal ? la : lb;
					bool repetido = false;
					for (const LadoPartido* x : { &la, &lb }) for (const JugadorPartido& jp : x->jugadores) if (jp.parsecId == s.parsecId) repetido = true;
					if (!repetido && l.jugadores.size() < 4) l.jugadores.push_back({ s.parsecId, s.jugador });
				}
				// El anfitrión: si juega con su mando de Link (MandoHost no deja «dueño» en el motor y por eso no sale arriba)
				// cuenta en el lado de su puesto. Si lo pidió («Contarme como jugador»), también sin puesto, en su lado habitual
				// (el de «Mi puesto al jugar»): juega directo en PES. Así un anfitrión contra un invitado es un 1v1, no «contra la CPU».
				try {
					Guest& anf = h.getHost();
					const int tomado = MandoHost::activo() - 1;
					int local = -1;   // 1 = lado local, 0 = visita, -1 = no cuenta
					if (anf.isValid() && anf.userID != 0) {
						if (tomado >= 0 && tomado < activos) local = tomado < pr.equipoLocal ? 1 : 0;
						else if (pr.anfitrionCuenta && ahora - anfitrionPulsoSeg < 120.0) {   // automático: tocó su mando en los últimos 2 min
							if (pr.miPuestoModo == "local") local = 1;
							else if (pr.miPuestoModo == "visitante") local = 0;
							else local = pr.miPuestoNumero <= pr.equipoLocal ? 1 : 0;
						}
					}
					if (local >= 0) {
						bool repetido = false;
						for (const LadoPartido* x : { &la, &lb }) for (const JugadorPartido& jp : x->jugadores) if (jp.parsecId == anf.userID) repetido = true;
						LadoPartido& l = local == 1 ? la : lb;
						if (!repetido && l.jugadores.size() < 4) l.jugadores.insert(l.jugadores.begin(), { anf.userID, anf.name });
					}
				}
				catch (...) {}
				if (!la.jugadores.empty() || !lb.jugadores.empty()) {
					if (la.jugadores.empty()) { la.nombre = "CPU"; la.jugadores.push_back({ 0, "CPU" }); }
					if (lb.jugadores.empty()) { lb.nombre = "CPU"; lb.jugadores.push_back({ 0, "CPU" }); }
					if (in.partido.preparar(la, lb)) in.partido.origenLados("mandos");
				}
			}

			// 2a) Arranque automático: lados ya elegidos (Listo) y el reloj del juego echa a andar
			bool vivo = marcadorVivo;
			if (pr.marcadorAuto && in.partido.fase() == FasePartido::Listo && sala.abierta() && j.valido && j.fase == "en_juego" && j.relojCorre
				&& in.partido.iniciar(ahoraEpochMs(), ahora)) {
				PhoenixLink::instancia().marcarPartido(true);
				PhoenixLink::instancia().evento("partida_inicio", "", json{ {"marcador", in.partido.marcador()}, {"auto", true} }.dump());
				mensajeDelBot(in, "¡Arrancó! " + in.partido.ladoA().nombre + " vs " + in.partido.ladoB().nombre + ". El marcador va solo con el juego."
					+ (in.partido.origenLados() == "mandos" ? std::string(" Lados según los mandos: si en el juego es al revés, CORREGIR LADOS en Partido.") : std::string()));
				a.golesAnunciados = static_cast<int>(j.goles.size());
				a.golEsperaDesde = -1.0;
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
			if (!vivo) { a.golesAnunciados = -1; a.golEsperaDesde = -1.0; }
			if (vivo && j.valido && pr.marcadorAuto) {
				const int total = static_cast<int>(j.goles.size());
				if (a.golesAnunciados < 0 || total < a.golesAnunciados) {   // primera vez o gol anulado: sin anunciar
					a.golesAnunciados = total;
					a.golEsperaDesde = -1.0;
				}
				// Con Sider 7.3.4+ el nombre del goleador llega un poco después del gol: se espera como mucho
				// kEsperaGoleadorSeg (con Sider 7.3.3 nunca llega y el gol se anuncia igual, sin nombre).
				constexpr double kEsperaGoleadorSeg = 3.0;
				if (a.golesAnunciados < total) {
					bool faltaNombre = false;
					for (int i = a.golesAnunciados; i < total; i++)
						if (j.goles[static_cast<size_t>(i)].jugador.empty()) faltaNombre = true;
					if (a.golEsperaDesde < 0.0) a.golEsperaDesde = ahora;
					const bool anunciar = !faltaNombre || ahora - a.golEsperaDesde >= kEsperaGoleadorSeg || j.fase == "final";
					if (anunciar) {
						for (int i = a.golesAnunciados; i < total; i++) {
							const juego::GolJuego& g = j.goles[static_cast<size_t>(i)];
							mensajeDelBot(in, "GOL " + std::to_string(g.minuto) + "' de " + nombreEquipo(in, j, g.local) + quienMarco(g) + ". "
								+ in.partido.ladoA().nombre + " " + std::to_string(in.partido.ladoA().goles) + "-"
								+ std::to_string(in.partido.ladoB().goles) + " " + in.partido.ladoB().nombre);
						}
						a.golesAnunciados = total;
						a.golEsperaDesde = -1.0;
					}
				}
				if (j.fase == "descanso" && a.faseJuego == "en_juego") mensajeDelBot(in, "Descanso: " + resumenMarcador(in) + ".");
				if (j.fase == "final" && j.completo) {
					const std::string goles = lineaGoles(in, j);
					// Partido oficial: el resultado del juego sube a la web («propuesto»; lo valida el staff o el árbitro designado)
					bool contraIa = false;   // contra la máquina nunca es un resultado oficial
					for (const LadoPartido* l : { &in.partido.ladoA(), &in.partido.ladoB() })
						for (const JugadorPartido& jp : l->jugadores) if (jp.parsecId == 0) contraIa = true;
					if (contraIa && a.partidoOficial > 0) {
						mensajeDelBot(in, "Partido contra la máquina: no se envía como resultado oficial.");
						a.partidoOficial = 0;
						a.oficialTexto.clear();
					}
					if (a.partidoOficial > 0) {
						const bool inv = in.partido.mandosInvertidos();   // lado A = DT local del calendario
						const json gs = golesParaWeb(j, inv);
						// Ficha del Doctor (solo informativa: el staff la ve; no decide nada)
						const json doc = in.doctorJson.empty() ? json() : json::parse(in.doctorJson, nullptr, false);
						PhoenixLink::instancia().ligaResultado(json{
							{"partido_id", a.partidoOficial}, {"completo", true},
							{"doctor", doc.is_discarded() || doc.is_null() ? json() : json{ {"modo", doc.value("modo", std::string())},
								{"huella", doc.value("huella", std::string())}, {"parche", doc.value("parche", std::string())},
								{"generado", doc.value("generado", std::string())}, {"avisos", doc.value("avisos", json::array())} }},
							{"goles_local", inv ? j.golesVisita : j.golesLocal}, {"goles_visita", inv ? j.golesLocal : j.golesVisita},
							{"juego", { {"goles", gs}, {"pk_local", inv ? j.pkVisita : j.pkLocal}, {"pk_visita", inv ? j.pkLocal : j.pkVisita},
								{"nombre_local", inv ? j.nombreVisita : j.nombreLocal}, {"nombre_visita", inv ? j.nombreLocal : j.nombreVisita} }},
						}.dump());
						mensajeDelBot(in, "Resultado enviado a la web (" + a.oficialTexto + "). Lo valida el staff.");
						a.partidoOficial = 0;
						a.oficialTexto.clear();
					}
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

			// 2d) Marcador EN VIVO en la web (función «liga» → tabla liga_vivo → Realtime). Partidos oficiales siempre;
			//     amistosos solo si la sala no es privada. Se manda al cambiar goles o fase y cada 5 minutos de juego.
			//     Con phoenix_referee v1.2+ va también la PANTALLA del juego («pantalla»), y ANTES del pitazo (lados ya
			//     elegidos en Link = «Listo») se manda como «previa»: la web dice «Eligiendo equipos», «Saliendo al campo»…
			const bool publicable = a.partidoOficial > 0 || PhoenixPrefs::get().visibilidad != "privada";
			const juego::PantallaJuego pj = PhoenixLink::instancia().pantallaJuego();
			const std::string pantalla = pj.valida ? pj.pantalla : std::string();
			const bool previa = !vivo && fp == FasePartido::Listo && pj.valida && sala.abierta() && publicable;
			if (vivo && j.valido && sala.abierta() && publicable) {
				const bool inv = in.partido.mandosInvertidos();
				const int gl = inv ? j.golesVisita : j.golesLocal, gv = inv ? j.golesLocal : j.golesVisita;
				const std::string fase = j.fase == "menu" ? "previa" : j.fase;
				const std::string firma = std::to_string(gl) + "-" + std::to_string(gv) + "|" + fase + "|" + std::to_string(golesConNombre(j)) + "|" + pantalla;
				if (firma != a.vivoFirma || j.minuto >= a.vivoMinuto + 5 || ahora - vivoEnviadoSeg >= kLatidoVivoSeg) {
					const json gs = golesParaWeb(j, inv);
					json c = { {"local", in.partido.ladoA().nombre}, {"visitante", in.partido.ladoB().nombre},
						{"goles_local", (std::max)(0, gl)}, {"goles_visita", (std::max)(0, gv)},
						{"minuto", (std::max)(0, (std::min)(200, j.minuto))}, {"periodo", j.periodo}, {"fase", fase}, {"goles", gs} };
					if (!pantalla.empty()) c["pantalla"] = pantalla;
					if (a.partidoOficial > 0) c["partido_id"] = a.partidoOficial;
					PhoenixLink::instancia().ligaVivo(c.dump());
					vivoEnviadoSeg = ahora;
					a.vivoFirma = firma;
					a.vivoMinuto = j.minuto;
					a.vivoMandado = true;
				}
			}
			else if (previa) {
				const std::string firma = "previa|" + pantalla;
				if (firma != a.vivoFirma || ahora - vivoEnviadoSeg >= kLatidoVivoSeg) {
					json c = { {"local", in.partido.ladoA().nombre}, {"visitante", in.partido.ladoB().nombre},
						{"goles_local", 0}, {"goles_visita", 0}, {"minuto", 0}, {"periodo", 0}, {"fase", "previa"},
						{"goles", json::array()}, {"pantalla", pantalla} };
					if (a.partidoOficial > 0) c["partido_id"] = a.partidoOficial;
					PhoenixLink::instancia().ligaVivo(c.dump());
					vivoEnviadoSeg = ahora;
					a.vivoFirma = firma;
					a.vivoMinuto = 0;
					a.vivoMandado = true;
				}
			}
			else if (!vivo && a.vivoMandado) {
				PhoenixLink::instancia().ligaVivoTerminado();   // el partido terminó o se canceló: se quita de la web
				a.vivoMandado = false;
				a.vivoFirma.clear();
				a.vivoMinuto = -100;
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
