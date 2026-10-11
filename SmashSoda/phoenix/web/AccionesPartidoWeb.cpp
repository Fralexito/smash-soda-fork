#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../core/AccionesSala.h"
#include "../link/PhoenixLink.h"
#include "EstadisticasPartido.h"
#include "SalaClasificatoria.h"

// =============================================================================
//  Acciones de la interfaz web: Partido.
//  Preparar (sienta a cada lado en sus mandos), iniciar, marcador, pausa,
//  cambiar lados (cruza los mandos), finalizar (historial + chat + web).
//  Integración: el latido pasa a «en_partida» (la sala sale del radar) y la
//  web recibe partida_inicio / pausa / partida_fin con el marcador.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_partido_web {

		/// Nombre de un participante: invitado presente o el propio host.
		bool nombreParticipante(Interno& in, uint32_t id, std::string& nombre) {
			Hosting& h = hostingObligatorio(in);
			Guest& host = h.getHost();
			if (host.isValid() && host.userID == id) { nombre = host.name; return true; }
			for (Guest& g : h.getGuests()) if (g.userID == id) { nombre = g.name; return true; }
			return false;
		}

		LadoPartido lado(Interno& in, const json& d, const char* clave, const char* claveNombre) {
			if (!d.contains(clave) || !d[clave].is_array() || d[clave].empty() || d[clave].size() > 4) {
				throw ErrorAccion("DATOS_INVALIDOS", "Cada lado necesita entre 1 y 4 jugadores.");
			}
			LadoPartido l;
			for (const json& x : d[clave]) {
				const uint32_t id = idParsec(json{ {"parsecId", x} });
				std::string nombre;
				if (!nombreParticipante(in, id, nombre)) throw ErrorAccion("NO_ESTA", "Un jugador elegido ya no está en la sala.");
				l.jugadores.push_back({ id, nombre });
			}
			if (d.contains(claveNombre) && d[claveNombre].is_string()) l.nombre = texto(d, claveNombre, 40);
			return l;
		}

		std::string resumen(const Partido& p) {
			return p.ladoA().nombre + " " + std::to_string(p.ladoA().goles) + "-" + std::to_string(p.ladoB().goles) + " " + p.ladoB().nombre;
		}

		/// Mandos (índices desde 0) que ocupa hoy cada jugador del lado.
		std::vector<int> mandosDe(ProveedorSala& sala, const LadoPartido& l) {
			std::vector<int> r;
			const std::vector<AsientoVista> asientos = sala.asientos(8);
			for (const JugadorPartido& j : l.jugadores) {
				for (size_t i = 0; i < asientos.size(); i++) {
					if (asientos[i].ocupado && asientos[i].parsecId == j.parsecId) { r.push_back(static_cast<int>(i)); break; }
				}
			}
			return r;
		}

		/// Un lado para la web (ranking): ids Parsec como texto de dígitos, si sigue en la sala y si es el anfitrión.
		json ladoWeb(Interno& in, const LadoPartido& l, uint32_t idHost, bool local, int64_t inicioMs) {
			json jugadores = json::array();
			for (const JugadorPartido& j : l.jugadores) {
				jugadores.push_back({ {"parsec_id", std::to_string(j.parsecId)}, {"nombre", j.nombre},
					{"presente", in.presentesAntes.count(j.parsecId) > 0 || (idHost != 0 && j.parsecId == idHost)},
					{"es_host", idHost != 0 && j.parsecId == idHost} });
				json ficha = fichaJugador(inicioMs, j.parsecId);   // vacía con el sistema apagado
				if (!ficha.empty()) jugadores.back()["ficha"] = std::move(ficha);
			}
			return { {"nombre", l.nombre}, {"goles", l.goles}, {"local", local}, {"jugadores", jugadores} };
		}

		/// `datos` de «partida_fin» v2 (docs/RANKING-1V1.md): todo lo que la web necesita para el ranking.
		/// `invertido` = mandos cruzados al terminar (Partido::finalizar lo pone a false).
		json datosResultado(Interno& in, const RegistroPartido& r, bool invertido, bool cancelado) {
			const juego::EstadoPartidoJuego j = PhoenixLink::instancia().estadoJuego();
			uint32_t idHost = 0;
			try { if (in.ctx.hosting != nullptr) idHost = hostingObligatorio(in).getHost().userID; } catch (...) {}
			const bool unoContraUno = r.a.jugadores.size() == 1 && r.b.jugadores.size() == 1;
			const bool completo = !cancelado && j.valido && j.fase == "final" && j.completo;
			const bool golesDelJuego = j.valido && j.golesLocal >= 0 && j.golesVisita >= 0;
			json d = {
				{"v", 2},
				{"marcador", cancelado ? std::string("cancelado") : std::to_string(r.a.goles) + "-" + std::to_string(r.b.goles)},
				{"partido_id", std::to_string(r.inicioMs)},
				{"inicio_ms", r.inicioMs},
				{"fin_ms", ahoraEpochMs()},
				{"duracion_seg", r.duracionSeg},
				{"cierre", cancelado ? "cancelado" : (completo ? "completo" : "manual")},
				{"modo", unoContraUno ? "1v1" : "equipos"},
				{"ganador", cancelado ? "" : r.ganador},
				{"a", ladoWeb(in, r.a, idHost, !invertido, r.inicioMs)},
				{"b", ladoWeb(in, r.b, idHost, invertido, r.inicioMs)},
				{"correcciones", r.correcciones},
				{"pausas", r.pausas},
				{"host_parsec", idHost != 0 ? std::to_string(idHost) : std::string()},
				{"abandono_juego", in.arbitro.avisoAbandono || (j.valido && j.fase == "menu")},
			};
			if (j.valido) {
				// El lado A juega con el equipo local, salvo con los mandos cruzados
				const int golesA = invertido ? j.golesVisita : j.golesLocal;
				const int golesB = invertido ? j.golesLocal : j.golesVisita;
				json info = { {"fase", j.fase}, {"minuto", j.minuto}, {"completo", j.completo},
					{"equipo_a", invertido ? j.visita : j.local}, {"equipo_b", invertido ? j.local : j.visita},
					{"nombre_equipo_a", invertido ? j.nombreVisita : j.nombreLocal},
					{"nombre_equipo_b", invertido ? j.nombreLocal : j.nombreVisita},
					{"coincide", golesDelJuego && golesA == r.a.goles && golesB == r.b.goles} };
				if (golesDelJuego) { info["goles_a"] = golesA; info["goles_b"] = golesB; }
				if (j.pkLocal >= 0 && j.pkVisita >= 0) {
					info["penales_a"] = invertido ? j.pkVisita : j.pkLocal;
					info["penales_b"] = invertido ? j.pkLocal : j.pkVisita;
				}
				json goles = json::array();
				for (const juego::GolJuego& g : j.goles) goles.push_back({ {"m", g.minuto}, {"lado", g.local != invertido ? "a" : "b"} });
				info["goles"] = goles;
				d["juego"] = info;
			}
			// El contrato limita `datos` a 4 KB: si no cabe, fuera la lista de goles (se queda el marcador)
			if (d.dump(-1, ' ', false, json::error_handler_t::replace).size() > 3800 && d.contains("juego")) d["juego"].erase("goles");
			// Y si aún no cabe, fuera las fichas de los jugadores (el resultado siempre llega)
			if (d.dump(-1, ' ', false, json::error_handler_t::replace).size() > 3800)
				for (const char* lado : { "a", "b" }) for (json& jj : d[lado]["jugadores"]) jj.erase("ficha");
			return d;
		}
	}

	using namespace detalle_partido_web;

	std::optional<json> finalizarPartido(Interno& in, bool anunciar, const std::string& extra) {
		const bool invertido = in.partido.mandosInvertidos();
		auto reg = in.partido.finalizar(ahoraSeg());
		if (!reg) return std::nullopt;
		const std::string marcador = std::to_string(reg->a.goles) + "-" + std::to_string(reg->b.goles);
		const json datos = datosResultado(in, *reg, invertido, false);
		reg->cierre = datos.value("cierre", "");
		const bool guardado = in.historial && in.historial->agregar(*reg);
		in.partidosSesion++;
		PhoenixLink::instancia().marcarPartido(false);
		PhoenixLink::instancia().evento("partida_fin", "", datos.dump(-1, ' ', false, json::error_handler_t::replace));
		if (anunciar) {
			const std::string res = reg->a.nombre + " " + marcador + " " + reg->b.nombre;
			mensajeDelBot(in, "Final: " + res + (reg->ganador == "empate" ? ". ¡Empate!" : ". ¡Bien jugado!") + extra);
		}
		return json{ {"registro", reg->comoJson()}, {"guardado", guardado} };
	}

	void registrarAccionesPartido(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("partido.preparar", [&in](const json& d, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			LadoPartido a = lado(in, d, "a", "nombreA");
			LadoPartido b = lado(in, d, "b", "nombreB");
			comprobarLadosClasificatoria(in, a, b);
			if (!in.partido.preparar(a, b)) throw ErrorAccion("NO_SE_PUDO", "Revisa los lados: nadie puede estar en los dos y hay un partido en juego.");

			const bool asignar = d.contains("asignar") && d["asignar"].is_boolean() && d["asignar"].get<bool>();
			if (asignar) {
				// La formación sigue a los lados (los mandos de A primero, luego los de B)
				const int nA = static_cast<int>(a.jugadores.size()), nB = static_cast<int>(b.jugadores.size());
				aplicarFormacion(in, nA, nB);
				const uint32_t idHost = hostingObligatorio(in).getHost().userID;
				int mando = 0;
				for (const JugadorPartido& j : in.partido.ladoA().jugadores) {
					if (j.parsecId != idHost) AccionesSala::asignarConectando(sala, mando, j.parsecId);
					mando++;
				}
				mando = nA;
				for (const JugadorPartido& j : in.partido.ladoB().jugadores) {
					if (j.parsecId != idHost) AccionesSala::asignarConectando(sala, mando, j.parsecId);
					mando++;
				}
			}
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.iniciar", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (!in.partido.iniciar(ahoraEpochMs(), ahoraSeg())) throw ErrorAccion("NO_LISTO", "Primero elige los dos lados.");
			PhoenixLink::instancia().marcarPartido(true);
			PhoenixLink::instancia().evento("partida_inicio", "", json{ {"marcador", in.partido.marcador()} }.dump());
			if (d.value("anunciar", true)) {
				mensajeDelBot(in, "Partido: " + in.partido.ladoA().nombre + " vs " + in.partido.ladoB().nombre + ". ¡Suerte!");
			}
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.gol", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string l = texto(d, "lado", 2);
			const int delta = entero(d, "delta", -1, 1);
			if ((l != "a" && l != "b") || !in.partido.gol(l == "a" ? 0 : 1, delta)) throw ErrorAccion("NO_EN_JUEGO", "No hay partido en juego.");
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.pausa", [&in](const json& d, uint64_t) -> std::optional<json> {
			const bool pausa = booleano(d, "si");
			if (!in.partido.pausar(pausa, ahoraSeg())) throw ErrorAccion("NO_EN_JUEGO", "No hay partido en juego.");
			if (pausa) PhoenixLink::instancia().evento("pausa", "", json{ {"marcador", in.partido.marcador()} }.dump());
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.cambiarLados", [&in](const json& d, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			const FasePartido f = in.partido.fase();
			if (f == FasePartido::Libre) throw ErrorAccion("NO_LISTO", "Primero elige los dos lados.");
			const std::vector<int> ma = mandosDe(sala, in.partido.ladoA());
			const std::vector<int> mb = mandosDe(sala, in.partido.ladoB());
			if (ma.empty() || mb.empty()) throw ErrorAccion("SIN_MANDOS", "Los dos lados necesitan mando para cruzarlos.");
			if (ma.size() != mb.size()) throw ErrorAccion("LADOS_DESIGUALES", "Para cruzar mandos los dos lados deben tener los mismos jugadores con mando.");
			for (size_t i = 0; i < ma.size(); i++) AccionesSala::intercambiar(sala, ma[i], mb[i]);
			in.partido.invertirMandos();
			if (d.value("anunciar", true)) mensajeDelBot(in, "Cambio de lados: cada uno toma el mando del otro.");
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.finalizar", [&in](const json& d, uint64_t) -> std::optional<json> {
			auto r = finalizarPartido(in, d.value("anunciar", true), "");
			if (!r) throw ErrorAccion("NO_EN_JUEGO", "No hay partido en juego.");
			return r;
		});

		p.registrar("partido.cancelar", [&in](const json&, uint64_t) -> std::optional<json> {
			const bool enJuego = in.partido.fase() == FasePartido::EnJuego || in.partido.fase() == FasePartido::Pausado;
			std::string datos;
			if (enJuego) {
				// Se guarda en la web (no cuenta para el ranking) para ver quién cancela partidos
				RegistroPartido r;
				r.id = r.inicioMs = in.partido.inicioMs();
				r.duracionSeg = in.partido.segundosJugados(ahoraSeg());
				r.a = in.partido.ladoA();
				r.b = in.partido.ladoB();
				r.correcciones = in.partido.correcciones();
				r.pausas = in.partido.pausas();
				datos = datosResultado(in, r, in.partido.mandosInvertidos(), true).dump(-1, ' ', false, json::error_handler_t::replace);
			}
			in.partido.cancelar();
			if (enJuego) {
				PhoenixLink::instancia().marcarPartido(false);
				PhoenixLink::instancia().evento("partida_fin", "", datos);
			}
			return in.partido.comoJson(ahoraSeg());
		});

		p.registrar("partido.clasificatoria", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (d.contains("si") && d["si"].is_boolean()) fijarClasificatoria(d["si"].get<bool>());
			return estadoClasificatoria(in);
		});

		p.registrar("partido.historial", [&in](const json& d, uint64_t) -> std::optional<json> {
			const int maximo = d.contains("max") ? entero(d, "max", 1, 200) : 50;
			json lista = json::array();
			if (in.historial) {
				int n = 0;
				for (const RegistroPartido& r : in.historial->lista()) {
					lista.push_back(r.comoJson());
					if (++n >= maximo) break;
				}
			}
			return json{ {"partidos", lista} };
		});
	}

}
