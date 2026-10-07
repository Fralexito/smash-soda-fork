#include "PhoenixPrefs.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "../helpers/PathHelper.h"

using json = nlohmann::json;

namespace phoenix {

	namespace {
		std::string ruta() {
			return PathHelper::GetConfigPath() + "phoenix-ui.json";
		}
	}

	PhoenixPrefs& PhoenixPrefs::get() {
		static PhoenixPrefs prefs;
		return prefs;
	}

	void PhoenixPrefs::cargar() {
		try {
			const std::string r = ruta();
			if (r.empty() || !MTY_FileExists(r.c_str())) return;

			size_t tam = 0;
			void* datos = MTY_ReadFile(r.c_str(), &tam);
			if (datos == nullptr) return;
			const std::string texto(static_cast<const char*>(datos), tam);
			MTY_Free(datos);

			const json j = json::parse(texto, nullptr, false);
			if (j.is_discarded() || !j.is_object()) return;

			interfazPhoenix = j.value("interfaz", std::string("phoenix")) != "clasica";
			idioma = j.value("idioma", idioma);
			seccion = j.value("seccion", seccion);
			visibilidad = j.value("visibilidad", visibilidad);
			if (visibilidad != "publica" && visibilidad != "amigos" && visibilidad != "privada") visibilidad = "amigos";
			espectadores = j.value("espectadores", espectadores);
			entradaParsec = j.value("entradaParsec", entradaParsec);
			limiteEspectadores = j.value("limiteEspectadores", limiteEspectadores);
			if (limiteEspectadores < 0) limiteEspectadores = 0;
			if (limiteEspectadores > 16) limiteEspectadores = 16;
			juego = j.value("juego", juego).substr(0, 60);
			parche = j.value("parche", parche).substr(0, 60);
			region = j.value("region", region).substr(0, 40);
			mandosActivos = (std::max)(2, (std::min)(8, j.value("mandosActivos", mandosActivos)));
			equipoLocal = (std::max)(1, (std::min)(mandosActivos - 1, j.value("equipoLocal", equipoLocal)));
		}
		catch (...) {
			// Preferencias dañadas: se quedan los valores por defecto.
		}
	}

	void PhoenixPrefs::guardar() const {
		try {
			const std::string r = ruta();
			if (r.empty()) return;
			const json j = {
				{"interfaz", interfazPhoenix ? "phoenix" : "clasica"},
				{"idioma", idioma},
				{"seccion", seccion},
				{"visibilidad", visibilidad},
				{"espectadores", espectadores},
				{"entradaParsec", entradaParsec},
				{"limiteEspectadores", limiteEspectadores},
				{"juego", juego},
				{"parche", parche},
				{"region", region},
				{"mandosActivos", mandosActivos},
				{"equipoLocal", equipoLocal},
			};
			const std::string texto = j.dump(2);
			MTY_WriteFile(r.c_str(), texto.c_str(), texto.size());
		}
		catch (...) {
			// Si no se puede guardar, la app sigue funcionando.
		}
	}

}
