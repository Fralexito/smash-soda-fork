#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../core/AccionesSala.h"
#include "../link/PhoenixLink.h"

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
	}

	using namespace detalle_partido_web;

	void registrarAccionesPartido(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("partido.preparar", [&in](const json& d, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			LadoPartido a = lado(in, d, "a", "nombreA");
			LadoPartido b = lado(in, d, "b", "nombreB");
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
			auto reg = in.partido.finalizar(ahoraSeg());
			if (!reg) throw ErrorAccion("NO_EN_JUEGO", "No hay partido en juego.");
			const std::string marcador = std::to_string(reg->a.goles) + "-" + std::to_string(reg->b.goles);
			const bool guardado = in.historial && in.historial->agregar(*reg);
			in.partidosSesion++;
			PhoenixLink::instancia().marcarPartido(false);
			PhoenixLink::instancia().evento("partida_fin", "", json{ {"marcador", marcador} }.dump());
			if (d.value("anunciar", true)) {
				const std::string res = reg->a.nombre + " " + marcador + " " + reg->b.nombre;
				mensajeDelBot(in, "Final: " + res + (reg->ganador == "empate" ? ". ¡Empate!" : ". ¡Bien jugado!"));
			}
			return json{ {"registro", reg->comoJson()}, {"guardado", guardado} };
		});

		p.registrar("partido.cancelar", [&in](const json&, uint64_t) -> std::optional<json> {
			const bool enJuego = in.partido.fase() == FasePartido::EnJuego || in.partido.fase() == FasePartido::Pausado;
			in.partido.cancelar();
			if (enJuego) {
				PhoenixLink::instancia().marcarPartido(false);
				PhoenixLink::instancia().evento("partida_fin", "", json{ {"marcador", "cancelado"} }.dump());
			}
			return in.partido.comoJson(ahoraSeg());
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
