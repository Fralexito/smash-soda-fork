#include "InterfazWebInterno.h"

#include "../PhoenixPrefs.h"
#include "../link/BuscarJuego.h"
#include "../link/Entrega.h"

// =============================================================================
//  SYNC › PREPARAR TODO: busca la carpeta del juego sin pedirle nada al usuario.
//  Solo lee nombres de carpetas; lo unico que escribe es la carpeta elegida en las preferencias de Link (como «sync.carpetaJuego»).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_modulos {
		std::filesystem::path carpetaJuego(bool& abierto);   // AccionesModulosWeb.cpp
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
	}

}
