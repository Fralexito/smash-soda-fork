#include "InterfazWebInterno.h"

#include "../PhoenixPrefs.h"
#include "../link/BuscarJuego.h"
#include "../link/Entrega.h"
#include "../link/Fichajes.h"
#include "../link/ModulosJuego.h"
#include <fstream>
#include <set>

// =============================================================================
//  SYNC › PREPARAR TODO: busca la carpeta del juego sin pedirle nada al usuario.
//  Solo lee nombres de carpetas; lo unico que escribe es la carpeta elegida en las preferencias de Link (como «sync.carpetaJuego»).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_modulos {
		std::filesystem::path carpetaJuego(bool& abierto);   // AccionesModulosWeb.cpp
	}


	namespace {
		/// sider.ini que cargan los traspasos/fichajes generados (hay PlayerAssignment.bin en su Phoenix-DB) pero que no tienen la línea de Phoenix-DB:
		/// sin ella Sider no lee el archivo y el juego no cambia. Ocurre, por ejemplo, si el cambiador de parches reescribe el sider.ini.
		std::vector<std::filesystem::path> carpetasSinRaiz(const std::filesystem::path& juego, std::vector<std::string>& nombres) {
			std::vector<std::filesystem::path> r;
			std::set<std::string> vistas;
			// Solo donde Phoenix-DB tiene un PlayerAssignment.bin que GENERÓ Link y sigue siendo ese (estado aplicado/desfasado): un archivo viejo o ajeno no se activa.
			for (const phoenix::fichajes::Analisis& a : phoenix::fichajes::analizar(juego, phoenix::fichajes::Lista())) {
				if (!a.baseOk || (a.estado != "aplicado" && a.estado != "desfasado")) continue;
				const std::filesystem::path carpeta = a.objetivo.ini.parent_path();
				if (!vistas.insert(carpeta.string()).second) continue;
				std::ifstream f(a.objetivo.ini, std::ios::binary);
				if (!f) continue;
				const std::string ini((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
				if (phoenix::modulos::tieneRaizPhoenixDB(ini)) continue;
				r.push_back(carpeta);
				nombres.push_back(a.objetivo.nombre);
			}
			return r;
		}
	}

	void registrarAccionesPreparar(Interno& in) {
		Puente& p = *in.puente;

		// accion: "iniciar" (empieza a buscar en segundo plano) | "estado" (por defecto: como va). Si Link ya conoce una carpeta valida, no busca.
		// Cuando termina con UN solo juego con Sider, lo deja guardado; si hay varios, devuelve la lista para que la persona elija (sync.carpetaJuego).
		p.registrar("sync.detectarJuego", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string que = d.contains("accion") && d["accion"].is_string() ? texto(d, "accion", 10) : std::string("estado");
			if (que != "iniciar" && que != "estado") throw ErrorAccion("DATOS_INVALIDOS", "Acción desconocida.");
			bool abierto = false;
			const std::filesystem::path conocido = detalle_modulos::carpetaJuego(abierto);
			std::error_code ec;
			if (!conocido.empty() && std::filesystem::is_regular_file(conocido / "PES2021.exe", ec) && std::filesystem::is_regular_file(conocido / "SiderAddons" / "sider.ini", ec))
				return json{ {"conocido", true}, {"ruta", phoenix::entrega::aU8(conocido)}, {"buscando", false}, {"terminada", true}, {"candidatos", json::array()} };
			if (que == "iniciar") phoenix::buscar::iniciarBusqueda();
			const phoenix::buscar::EstadoBusqueda e = phoenix::buscar::estadoBusqueda();
			json cs = json::array();
			for (const phoenix::buscar::Candidato& c : e.candidatos) cs.push_back({ {"ruta", phoenix::entrega::aU8(c.carpeta)}, {"sider", c.conSider} });
			json r = { {"conocido", false}, {"ruta", ""}, {"buscando", e.buscando}, {"terminada", e.terminada}, {"candidatos", cs} };
			if (e.terminada) {
				std::vector<const phoenix::buscar::Candidato*> buenos;
				for (const phoenix::buscar::Candidato& c : e.candidatos) if (c.conSider) buenos.push_back(&c);
				if (buenos.size() == 1) {   // uno solo con Sider: es ese
					PhoenixPrefs& pr = PhoenixPrefs::get();
					pr.carpetaJuego = phoenix::entrega::aU8(buenos[0]->carpeta);
					pr.guardar();
					r["conocido"] = true; r["ruta"] = pr.carpetaJuego;
				}
			}
			return r;
		});

		// accion: "estado" (por defecto) | "arreglar". Repone la línea cpk.root de Phoenix-DB en los sider.ini que la perdieron.
		// «arreglar» exige PES cerrado, hace copia de cada sider.ini (sider.ini.phoenix-<sello>) y solo toca sitios donde ya hay un PlayerAssignment.bin generado.
		p.registrar("sync.raizPhoenixDB", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string que = d.contains("accion") && d["accion"].is_string() ? texto(d, "accion", 10) : std::string("estado");
			if (que != "estado" && que != "arreglar") throw ErrorAccion("DATOS_INVALIDOS", "Acción desconocida.");
			bool abierto = false;
			const std::filesystem::path juego = detalle_modulos::carpetaJuego(abierto);
			if (juego.empty()) return json{ {"juego", false}, {"pesAbierto", abierto}, {"faltan", json::array()}, {"arreglado", false}, {"mensaje", "Todavía no sé dónde está tu juego."} };
			std::vector<std::string> nombres;
			std::vector<std::filesystem::path> carpetas = carpetasSinRaiz(juego, nombres);
			json r = { {"juego", true}, {"pesAbierto", abierto}, {"faltan", nombres}, {"arreglado", false}, {"mensaje", ""} };
			if (que == "arreglar" && !carpetas.empty()) {
				if (abierto) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 (y Sider) primero: Sider lee su configuración al arrancar.");
				const phoenix::modulos::Resultado m = phoenix::modulos::asegurarRaizPhoenixDB(carpetas);
				if (!m.ok) throw ErrorAccion("NO_SE_PUDO", m.mensaje);
				std::vector<std::string> quedan;
				r["faltan"] = carpetasSinRaiz(juego, quedan).empty() ? json::array() : json(quedan);
				r["arreglado"] = true;
				r["mensaje"] = m.mensaje;
			}
			return r;
		});
	}

}
