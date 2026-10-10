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
			interfazWeb = j.value("interfazWeb", interfazWeb);
			tema = j.value("tema", tema);
			if (tema != "galaxy" && tema != "sudario") tema = "galaxy";
			idioma = j.value("idioma", idioma);
			seccion = j.value("seccion", seccion);
			visibilidad = j.value("visibilidad", visibilidad);
			if (visibilidad != "publica" && visibilidad != "amigos" && visibilidad != "privada") visibilidad = "amigos";
			espectadores = j.value("espectadores", espectadores);
			entradaParsec = j.value("entradaParsec", entradaParsec);
			avisosEnJuego = j.value("avisosEnJuego", avisosEnJuego);
			carpetaJuego = j.value("carpetaJuego", carpetaJuego);
			modoPes = j.value("modoPes", modoPes);
			if (modoPes != "publicar" && modoPes != "estricto") modoPes = "off";
			limiteEspectadores = j.value("limiteEspectadores", limiteEspectadores);
			if (limiteEspectadores < 0) limiteEspectadores = 0;
			if (limiteEspectadores > 16) limiteEspectadores = 16;
			juego = j.value("juego", juego).substr(0, 60);
			parche = j.value("parche", parche).substr(0, 60);
			region = j.value("region", region).substr(0, 40);
			mandosActivos = (std::max)(2, (std::min)(8, j.value("mandosActivos", mandosActivos)));
			equipoLocal = (std::max)(1, (std::min)(mandosActivos - 1, j.value("equipoLocal", equipoLocal)));
			anchoAuto = j.value("anchoAuto", anchoAuto);
			anchoPorPersona = (std::max)(1, (std::min)(50, j.value("anchoPorPersonaV2", anchoPorPersona)));
			subidaMbps = (std::max)(0, (std::min)(10000, j.value("subidaMbps", subidaMbps)));
			competitivo = j.value("competitivo", competitivo);
			pausaAuto = j.value("pausaAuto", pausaAuto);
			marcadorAuto = j.value("marcadorAuto", marcadorAuto);
			soloAutorizados = j.value("soloAutorizados", soloAutorizados);
			modoRecarga = j.value("modoRecarga", modoRecarga);
			if (modoRecarga != "AUTO-FICHAJES" && modoRecarga != "AUTO-SIEMPRE") modoRecarga = "ACTIVAR";
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
				{"interfazWeb", interfazWeb},
				{"tema", tema},
				{"idioma", idioma},
				{"seccion", seccion},
				{"visibilidad", visibilidad},
				{"espectadores", espectadores},
				{"entradaParsec", entradaParsec},
				{"avisosEnJuego", avisosEnJuego},
				{"carpetaJuego", carpetaJuego},
				{"modoPes", modoPes},
				{"limiteEspectadores", limiteEspectadores},
				{"juego", juego},
				{"parche", parche},
				{"region", region},
				{"mandosActivos", mandosActivos},
				{"equipoLocal", equipoLocal},
				{"anchoAuto", anchoAuto},
				{"anchoPorPersonaV2", anchoPorPersona},
				{"subidaMbps", subidaMbps},
				{"competitivo", competitivo},
				{"pausaAuto", pausaAuto},
				{"marcadorAuto", marcadorAuto},
				{"soloAutorizados", soloAutorizados},
				{"modoRecarga", modoRecarga},
			};
			const std::string texto = j.dump(2);
			MTY_WriteFile(r.c_str(), texto.c_str(), texto.size());
		}
		catch (...) {
			// Si no se puede guardar, la app sigue funcionando.
		}
	}

}
