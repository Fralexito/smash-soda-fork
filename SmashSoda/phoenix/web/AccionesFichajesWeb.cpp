#include "InterfazWebInterno.h"

#include "../link/Entrega.h"
#include "../link/Fichajes.h"
#include "../link/ModulosJuego.h"

// =============================================================================
//  Acciones de la interfaz web: «Fichajes de Phoenix» (SYNC › Puente).
//  Link genera Phoenix-DB\…\PlayerAssignment.bin a partir de la base REAL de tu juego + la lista de cambios
//  (fichajes.json). No copia bases de nadie. Solo con PES cerrado. Detalle: docs/FICHAJES-Y-SYNC.md («FICHAJES-COMO-CAMBIOS»).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_modulos {
		std::filesystem::path carpetaJuego(bool& abierto);   // AccionesModulosWeb.cpp
	}

	namespace detalle_fichajes {
		json aJson(const phoenix::fichajes::Analisis& a) {
			json saltados = json::array();
			for (const phoenix::fichajes::Salto& s : a.saltados)
				saltados.push_back({ {"jugador", s.cambio.jugador}, {"equipo", s.cambio.equipo}, {"codigo", s.codigo}, {"motivo", s.motivo} });
			return { {"nombre", a.objetivo.nombre}, {"baseOk", a.baseOk}, {"baseOrigen", a.baseOrigen}, {"error", a.error}, {"filasBase", a.filasBase},
				{"parche", a.parche}, {"baseGenerica", a.baseGenerica}, {"comoLoSupe", a.comoLoSupe}, {"queHacer", a.queHacer}, {"nombresDisponibles", a.nombresDisponibles},
				{"aplicados", a.aplicados}, {"yaEstaban", a.yaEstaban}, {"saltados", saltados}, {"estado", a.estado}, {"playerBinAjeno", a.conPlayerBinAjeno} };
		}

		/// Texto corto «Fichajes: N aplicados, M saltados» para el Doctor y el resumen de Sync.
		std::string resumenTexto(const std::vector<phoenix::fichajes::Analisis>& v, size_t cambios) {
			if (cambios == 0) return "Fichajes: la lista está vacía (nada que aplicar).";
			int aplicados = 0, saltados = 0, sitios = 0;
			for (const phoenix::fichajes::Analisis& a : v) {
				if (!a.baseOk) continue;
				if (a.estado == "aplicado") { aplicados += a.aplicados; sitios++; }
				saltados += static_cast<int>(a.saltados.size());
			}
			if (sitios == 0) return "Fichajes: sin aplicar (" + std::to_string(cambios) + " en la lista).";
			return "Fichajes: " + std::to_string(aplicados) + " aplicados, " + std::to_string(saltados) + " saltados en " + std::to_string(sitios) + (sitios == 1 ? " sitio" : " sitios") + ".";
		}

		json estadoFichajes() {
			bool abierto = false;
			const std::filesystem::path juego = detalle_modulos::carpetaJuego(abierto);
			phoenix::fichajes::Lista lista;
			std::string error;
			const bool listaOk = phoenix::fichajes::cargarLista(phoenix::fichajes::carpetaListaUsuario(), phoenix::fichajes::carpetaListaPaquete(), lista, error);
			json r = { {"juego", phoenix::entrega::aU8(juego)}, {"pesAbierto", abierto || !phoenix::modulos::procesosAbiertos().empty()},
				{"lista", { {"fuente", lista.fuente}, {"nombre", lista.nombre}, {"regla", lista.estricta ? "estricta" : "relajada"}, {"cambios", lista.cambios.size()}, {"error", listaOk ? "" : error} }},
				{"objetivos", json::array()} };
			if (!listaOk || juego.empty()) { r["texto"] = juego.empty() ? "Fichajes: Link aún no sabe dónde está tu juego." : "Fichajes: la lista no se pudo leer."; return r; }
			const std::vector<phoenix::fichajes::Analisis> an = phoenix::fichajes::analizar(juego, lista);
			for (const phoenix::fichajes::Analisis& a : an) r["objetivos"].push_back(aJson(a));
			r["texto"] = resumenTexto(an, lista.cambios.size());
			return r;
		}
	}

	namespace detalle_fichajes {
		/// Texto para el diagnóstico y el informe (solo lee). Con detalle: una línea por modo y los motivos de lo saltado.
		std::string textoFichajes(const std::filesystem::path& juego, bool conDetalle) {
			if (juego.empty()) return conDetalle ? std::string("(no se conoce la carpeta del juego)\n") : std::string();
			phoenix::fichajes::Lista lista;
			std::string error;
			if (!phoenix::fichajes::cargarLista(phoenix::fichajes::carpetaListaUsuario(), phoenix::fichajes::carpetaListaPaquete(), lista, error)) return "Fichajes: la lista no se pudo leer (" + error + ")" + (conDetalle ? "\n" : "");
			const std::vector<phoenix::fichajes::Analisis> an = phoenix::fichajes::analizar(juego, lista);
			std::string t = resumenTexto(an, lista.cambios.size());
			if (!conDetalle) return t;
			t += "\nLista: " + lista.fuente + " (" + std::to_string(lista.cambios.size()) + " cambios, regla " + (lista.estricta ? "estricta" : "relajada") + ")\n";
			for (const phoenix::fichajes::Analisis& a : an) {
				t += "  · " + a.objetivo.nombre + ": " + a.estado + " · parche detectado: " + (a.parche.empty() ? std::string("(ninguno)") : a.parche) + (a.baseGenerica ? " (base genérica)" : "") + (a.baseOk ? " · base " + a.baseOrigen + " (" + std::to_string(a.filasBase) + " filas)" : " · " + a.error + (a.queHacer.empty() ? "" : " → " + a.queHacer))
					+ " · aplicables " + std::to_string(a.aplicados) + ", ya estaban " + std::to_string(a.yaEstaban) + ", saltados " + std::to_string(a.saltados.size()) + (a.conPlayerBinAjeno ? " · Phoenix-DB tiene un Player.bin ajeno" : "") + "\n";
				for (const std::string& e : a.comoLoSupe) t += "      cómo lo supe: " + e + "\n";
				for (const phoenix::fichajes::Salto& s : a.saltados) t += "      salta " + std::to_string(s.cambio.jugador) + " → " + std::to_string(s.cambio.equipo) + ": " + s.motivo + "\n";
			}
			return t;
		}
	}

	using namespace detalle_fichajes;

	void registrarAccionesFichajes(Interno& in) {
		Puente& p = *in.puente;

		// Solo lee archivos: qué haría Link en cada modo con tu lista de fichajes.
		p.registrar("fichajes.estado", [](const json&, uint64_t) -> std::optional<json> {
			return estadoFichajes();
		});

		p.registrar("fichajes.accion", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string que = texto(d, "accion", 12);
			if (que != "aplicar" && que != "quitar") throw ErrorAccion("DATOS_INVALIDOS", "Acción desconocida.");
			bool abierto = false;
			const std::filesystem::path juego = detalle_modulos::carpetaJuego(abierto);
			if (abierto) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 primero: Sider lee los datos al arrancar.");
			if (!phoenix::modulos::procesosAbiertos().empty()) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 y Sider (sider.exe sigue abierto) antes de tocar los fichajes.");
			if (juego.empty()) throw ErrorAccion("SIN_JUEGO", "Abre PES 2021 una vez para que Phoenix Link conozca su carpeta.");
			phoenix::fichajes::Informe inf;
			if (que == "aplicar") {
				phoenix::fichajes::Lista lista;
				std::string error;
				if (!phoenix::fichajes::cargarLista(phoenix::fichajes::carpetaListaUsuario(), phoenix::fichajes::carpetaListaPaquete(), lista, error))
					throw ErrorAccion("LISTA_INVALIDA", error);
				inf = phoenix::fichajes::aplicarAlJuego(juego, lista);
			}
			else inf = phoenix::fichajes::quitarDelJuego(juego);
			if (!inf.ok) throw ErrorAccion("NO_SE_PUDO", inf.mensaje);
			json res = estadoFichajes();
			res["mensaje"] = inf.mensaje;
			res["respaldos"] = inf.respaldos;
			return res;
		});
	}

}
