#include "InterfazWebInterno.h"

#include "../PhoenixPrefs.h"
#include "../link/BuzonJuego.h"
#include "../link/Entrega.h"
#include "../link/ModulosJuego.h"

// =============================================================================
//  Acciones de la interfaz web: «Módulos del juego» (SYNC › Puente).
//  Instala, actualiza o apaga los módulos Lua de Sider que lleva Phoenix Link
//  (phoenix/sider/*.lua). Solo con PES cerrado; sider.ini siempre con copia previa.
//  Detalle: phoenix/link/ModulosJuego.h y docs/PHOENIX-ESTADIO-ARBITRO.md.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_modulos {
		/// Carpeta del juego: la del PES abierto o la última conocida (PhoenixPrefs::carpetaJuego).
		std::filesystem::path carpetaJuego(bool& abierto) {
			const phoenix::buzon::EstadoJuego g = phoenix::buzon::detectarJuego();
			abierto = g.puente != phoenix::buzon::Puente::JuegoCerrado;
			PhoenixPrefs& pr = PhoenixPrefs::get();
			if (abierto && !g.carpetaJuego.empty()) {
				const std::string ruta = phoenix::entrega::aU8(g.carpetaJuego);
				if (pr.carpetaJuego != ruta) { pr.carpetaJuego = ruta; pr.guardar(); }
				return g.carpetaJuego;
			}
			return pr.carpetaJuego.empty() ? std::filesystem::path() : phoenix::entrega::deU8(pr.carpetaJuego);
		}

		json listaModulos() {
			bool abierto = false;
			const std::filesystem::path juego = carpetaJuego(abierto);
			json lista = json::array();
			for (const phoenix::modulos::Modulo& m : phoenix::modulos::estado(phoenix::modulos::carpetaPaquete(), juego)) {
				json ds = json::array();
				for (const phoenix::modulos::Destino& d : m.destinos) {
					ds.push_back({ {"nombre", d.nombre}, {"carpeta", d.carpeta}, {"archivo", d.archivo}, {"igual", d.igual},
						{"version", d.version}, {"linea", d.linea}, {"comentada", d.comentada} });
				}
				lista.push_back({ {"archivo", m.archivo}, {"descripcion", m.descripcion}, {"gestionable", m.gestionable},
					{"version", m.version}, {"estado", m.estado}, {"carga", m.carga}, {"destinos", ds} });
			}
			return json{ {"juego", phoenix::entrega::aU8(juego)}, {"pesAbierto", abierto}, {"modulos", lista} };
		}
	}

	using namespace detalle_modulos;

	void registrarAccionesModulos(Interno& in) {
		Puente& p = *in.puente;

		// Modo de recarga de phoenix.lua v0.18 (contrato modo.txt, PROMPT-LINK-modo-recarga.md de Sync).
		// Lo escribe el hilo de Link en content\phoenix\modo.txt de la raíz y de cada modo del cambiador.
		p.registrar("sync.modoRecarga", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string modo = texto(d, "modo", 16);
			if (modo != "ACTIVAR" && modo != "AUTO-FICHAJES" && modo != "AUTO-SIEMPRE") throw ErrorAccion("VALOR_INVALIDO", "Modo no válido.");
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.modoRecarga = modo;
			pr.guardar();
			return json::object();
		});

		p.registrar("sync.modulos", [](const json&, uint64_t) -> std::optional<json> {
			return listaModulos();
		});

		p.registrar("sync.modulo", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string archivo = texto(d, "archivo", 64);
			const std::string que = texto(d, "accion", 16);
			if (que != "instalar" && que != "quitar") throw ErrorAccion("DATOS_INVALIDOS", "Acción desconocida.");
			bool abierto = false;
			const std::filesystem::path juego = carpetaJuego(abierto);
			if (abierto) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 primero: Sider lee los módulos al arrancar.");
			if (juego.empty()) throw ErrorAccion("SIN_JUEGO", "Abre PES 2021 una vez para que Phoenix Link conozca su carpeta.");
			const phoenix::modulos::Resultado r = que == "instalar"
				? phoenix::modulos::instalar(phoenix::modulos::carpetaPaquete(), juego, archivo)
				: phoenix::modulos::quitar(phoenix::modulos::carpetaPaquete(), juego, archivo);
			if (!r.ok) throw ErrorAccion("NO_SE_PUDO", r.mensaje);
			json res = listaModulos();
			res["mensaje"] = r.mensaje;
			res["respaldos"] = r.respaldos;
			return res;
		});
	}

}
