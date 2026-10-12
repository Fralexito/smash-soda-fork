#include "PhoenixPrefs.h"

#include <algorithm>
#include <filesystem>

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
			if (r.empty()) return;

			// El archivo; si está dañado (corte de luz a mitad de guardar), la copia .bak del guardado anterior.
			json j;
			bool leido = false;
			for (const std::string& p : { r, r + ".bak" }) {
				if (!MTY_FileExists(p.c_str())) continue;
				size_t tam = 0;
				void* datos = MTY_ReadFile(p.c_str(), &tam);
				if (datos == nullptr) continue;
				const std::string texto(static_cast<const char*>(datos), tam);
				MTY_Free(datos);
				j = json::parse(texto, nullptr, false);
				if (!j.is_discarded() && j.is_object()) { leido = true; break; }
			}
			if (!leido) return;

			interfazPhoenix = j.value("interfaz", std::string("phoenix")) != "clasica";
			interfazWeb = true;   // siempre la interfaz web (2026-10-11); las viejas solo si WebView2 falla
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
			// Equipo de cada puesto; un archivo anterior (sin equipoPuesto) migra: los primeros equipoLocal = A, el resto hasta mandosActivos = B
			equipoPuesto = j.contains("equipoPuesto") && j["equipoPuesto"].is_string()
				? equipos::normalizar(j["equipoPuesto"].get<std::string>())
				: equipos::migrar(mandosActivos, equipoLocal);
			sincronizarEquipos();
			puestosFijados.clear();
			if (j.contains("puestosFijados") && j["puestosFijados"].is_object()) {
				for (const auto& par : j["puestosFijados"].items()) {
					try {
						const unsigned long id = std::stoul(par.key());
						const int puesto = par.value().is_number_integer() ? par.value().get<int>() : 0;
						if (id != 0 && puesto >= 1 && puesto <= 8) puestosFijados[static_cast<uint32_t>(id)] = puesto;
					}
					catch (...) {}
				}
			}
			anchoAuto = j.value("anchoAuto", anchoAuto);
			anchoPorPersona = (std::max)(1, (std::min)(50, j.value("anchoPorPersonaV2", anchoPorPersona)));
			subidaMbps = (std::max)(0, (std::min)(10000, j.value("subidaMbps", subidaMbps)));
			competitivo = j.value("competitivo", competitivo);
			pausaAuto = j.value("pausaAuto", pausaAuto);
			marcadorAuto = j.value("marcadorAuto", marcadorAuto);
			soloAutorizados = j.value("soloAutorizados", soloAutorizados);
			avisoEntrada = j.value("avisoEntrada", avisoEntrada);
			avisoEntradaSonido = j.value("avisoEntradaSonido", avisoEntradaSonido);
			avisoEntradaNotif = j.value("avisoEntradaNotif", avisoEntradaNotif);
			avisoEntradaJuego = j.value("avisoEntradaJuego", avisoEntradaJuego);
			miMando = j.value("miMando", miMando);
			miPuestoModo = j.value("miPuestoModo", miPuestoModo);
			if (miPuestoModo == "local") miPuestoModo = "a";   // archivos viejos: Local → Equipo A, Visitante → Equipo B
			else if (miPuestoModo == "visitante") miPuestoModo = "b";
			else if (miPuestoModo != "a" && miPuestoModo != "b") miPuestoModo = "fijo";
			miPuestoNumero = (std::max)(1, (std::min)(8, j.value("miPuestoNumero", miPuestoNumero)));
			sentarmeAlAbrir = j.value("sentarmeAlAbrir", sentarmeAlAbrir);
			entrarPulsando = j.value("entrarPulsando", entrarPulsando);
			anfitrionCuenta = j.value("anfitrionCuenta", anfitrionCuenta);
			ordenPes = j.value("ordenPes", ordenPes);
			campoWeb = j.value("campoWeb", campoWeb);
			ladosAuto = j.value("ladosAuto", ladosAuto);
			mirandoAuto = j.value("mirandoAuto", mirandoAuto);
			modoRecarga = j.value("modoRecarga", modoRecarga);
			cedidoExtras = j.value("cedidoExtras", cedidoExtras);
			if (modoRecarga != "ACTIVAR" && modoRecarga != "AUTO-FICHAJES") modoRecarga = "AUTO-SIEMPRE";   // v0.18b: por defecto
		}
		catch (...) {
			// Preferencias dañadas: se quedan los valores por defecto.
		}
	}

	void PhoenixPrefs::guardar() const {
		try {
			const std::string r = ruta();
			if (r.empty()) return;
			json fijados = json::object();
			for (const auto& par : puestosFijados) fijados[std::to_string(par.first)] = par.second;
			const json j = {
				{"puestosFijados", fijados},
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
				{"equipoPuesto", equipoPuesto},
				{"anchoAuto", anchoAuto},
				{"anchoPorPersonaV2", anchoPorPersona},
				{"subidaMbps", subidaMbps},
				{"competitivo", competitivo},
				{"pausaAuto", pausaAuto},
				{"marcadorAuto", marcadorAuto},
				{"soloAutorizados", soloAutorizados},
				{"avisoEntrada", avisoEntrada},
				{"avisoEntradaSonido", avisoEntradaSonido},
				{"avisoEntradaNotif", avisoEntradaNotif},
				{"avisoEntradaJuego", avisoEntradaJuego},
				{"miMando", miMando},
				{"miPuestoModo", miPuestoModo},
				{"miPuestoNumero", miPuestoNumero},
				{"sentarmeAlAbrir", sentarmeAlAbrir},
				{"entrarPulsando", entrarPulsando},
				{"anfitrionCuenta", anfitrionCuenta},
				{"ordenPes", ordenPes},
				{"campoWeb", campoWeb},
				{"ladosAuto", ladosAuto},
				{"mirandoAuto", mirandoAuto},
				{"modoRecarga", modoRecarga},
				{"cedidoExtras", cedidoExtras},
			};
			const std::string texto = j.dump(2);
			// Atómico: se escribe aparte, se guarda copia del anterior (.bak) y se cambia de nombre. Si algo falla, queda el archivo de antes.
			const std::string tmp = r + ".tmp";
			if (!MTY_WriteFile(tmp.c_str(), texto.c_str(), texto.size())) return;
			std::error_code ec;
			const std::filesystem::path pr = std::filesystem::u8path(r);
			if (std::filesystem::exists(pr, ec)) std::filesystem::copy_file(pr, std::filesystem::u8path(r + ".bak"), std::filesystem::copy_options::overwrite_existing, ec);
			std::filesystem::rename(std::filesystem::u8path(tmp), pr, ec);
			if (ec) std::filesystem::remove(std::filesystem::u8path(tmp), ec);
		}
		catch (...) {
			// Si no se puede guardar, la app sigue funcionando.
		}
	}

}
