#include "InterfazWebInterno.h"

#include "../PhoenixPrefs.h"
#include "../link/BuzonJuego.h"
#include "../link/Entrega.h"
#include "../link/ModulosJuego.h"
#include "../link/Doctor.h"
#include "../PhoenixBuild.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <vector>

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
						{"version", d.version}, {"linea", d.linea}, {"comentada", d.comentada}, {"perdida", d.perdida}, {"sinBase", d.sinBase} });
				}
				lista.push_back({ {"archivo", m.archivo}, {"descripcion", m.descripcion}, {"gestionable", m.gestionable},
					{"version", m.version}, {"estado", m.estado}, {"carga", m.carga}, {"destinos", ds} });
			}
			// Cambiador de parches: qué modos usan el sider.ini de la raíz (parche 27) y cuáles traen el suyo (parche 26)
			const phoenix::modulos::Parche pa = phoenix::modulos::infoParche(juego);
			const std::vector<std::string> otros = phoenix::modulos::procesosAbiertos();
			json parche = { {"modoActual", pa.modoActual}, {"modosRaiz", pa.modosRaiz}, {"modosPropios", pa.modosPropios}, {"advertencias", pa.advertencias} };
			return json{ {"juego", phoenix::entrega::aU8(juego)}, {"pesAbierto", abierto || !otros.empty()}, {"abiertos", otros},
				{"parche", parche}, {"modulos", lista} };
		}
	}

	using namespace detalle_modulos;

	namespace {
		/// Texto seguro para JSON: los bytes que no son UTF-8 válido se cambian por «?» (sider.log puede venir en Latin-1).
		std::string utf8Seguro(const std::string& s) {
			std::string r;
			r.reserve(s.size());
			for (size_t i = 0; i < s.size();) {
				const unsigned char c = static_cast<unsigned char>(s[i]);
				size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
				bool bien = n > 0 && i + n <= s.size();
				for (size_t k = 1; bien && k < n; k++) bien = (static_cast<unsigned char>(s[i + k]) & 0xC0) == 0x80;
				if (bien) { r.append(s, i, n); i += n; }
				else { r += '?'; i++; }
			}
			return r;
		}

		/// Últimas `n` líneas de un archivo de texto (lee como máximo los últimos 256 KB). "" si no existe.
		std::string ultimasLineas(const std::filesystem::path& ruta, int n) {
			std::error_code ec;
			const uintmax_t tam = std::filesystem::file_size(ruta, ec);
			if (ec) return std::string();
			std::ifstream f(ruta, std::ios::binary);
			if (!f) return std::string();
			const uintmax_t tope = 256 * 1024;
			if (tam > tope) f.seekg(static_cast<std::streamoff>(tam - tope));
			std::string todo((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			std::vector<std::string> lineas;
			size_t i = 0;
			while (i < todo.size()) {
				size_t j = todo.find('\n', i);
				if (j == std::string::npos) j = todo.size();
				std::string l = todo.substr(i, j - i);
				if (!l.empty() && l.back() == '\r') l.pop_back();
				lineas.push_back(l.size() > 200 ? l.substr(0, 200) : l);
				i = j + 1;
			}
			std::string r;
			for (size_t k = lineas.size() > static_cast<size_t>(n) ? lineas.size() - static_cast<size_t>(n) : 0; k < lineas.size(); k++) r += lineas[k] + "\n";
			return r;
		}

		/// Informe de texto para mandar a quien depura sin acceso a este PC: versiones, juego, parche, módulos y el final de sider.log.
		/// Solo lee archivos. Lleva rutas de este PC (pueden incluir el nombre de usuario de Windows).
		std::string informeTexto(const std::filesystem::path& juego, bool pesAbierto) {
			std::string t = "== PHOENIX LINK · INFORME PARA DEPURAR ==\n";
			t += std::string("Phoenix Link ") + kVersion + " (" + kEtapa + ")\n";
			const phoenix::doctor::Informe d = phoenix::doctor::revisar(juego);
			t += "Fecha: " + d.generado + " · PES abierto: " + (pesAbierto ? "sí" : "no") + "\n";
			t += "Carpeta del juego: " + (d.juego.empty() ? std::string("(no se conoce)") : d.juego) + "\n";
			t += "Parche: " + d.parche + " · exe: " + std::to_string(d.exeBytes) + " bytes · huella " + d.exeHuella.substr(0, 12) + "\n";
			t += "Carpetas de Sider: " + std::to_string(d.modos.size()) + "\n";
			const phoenix::modulos::Parche pa = phoenix::modulos::infoParche(juego);
			t += "Último modo lanzado: " + (pa.modoActual.empty() ? std::string("(ninguno)") : pa.modoActual) + "\n";
			for (const std::string& w : pa.advertencias) t += "  ! " + w + "\n";
			for (const std::string& a : d.avisos) t += "Aviso del Doctor: " + a + "\n";
			t += "Raíces de la base (cpk.root, primeras 6):";
			for (size_t i = 0; i < d.cpkRoots.size() && i < 6; i++) t += "\n  " + d.cpkRoots[i];
			t += "\n\n-- Módulos de Phoenix --\n";
			for (const phoenix::modulos::Modulo& m : phoenix::modulos::estado(phoenix::modulos::carpetaPaquete(), juego)) {
				t += m.archivo + " (Link lleva v" + m.version + ") estado=" + m.estado + " carga=" + (m.carga.empty() ? "?" : m.carga) + "\n";
				for (const phoenix::modulos::Destino& x : m.destinos)
					t += "   · " + x.nombre + ": archivo=" + (x.archivo ? "sí" : "no") + (x.version.empty() ? "" : " v" + x.version) + " igual=" + (x.igual ? "sí" : "no")
						+ " línea=" + (x.linea ? "sí" : "no") + (x.perdida ? " PERDIDA" : "") + (x.sinBase ? " sinBase" : "") + "\n";
			}
			t += "\n-- Archivos que deciden los datos (sha256 corto) --\n";
			for (const phoenix::doctor::Archivo& a : d.archivos) t += a.nombre + ": " + (a.sha256.empty() ? std::string("no está") : a.sha256.substr(0, 12) + " · " + std::to_string(a.bytes) + " bytes") + "\n";
			const auto destinos = phoenix::modulos::destinos(juego);
			if (!destinos.empty()) {
				t += "\n-- Final de sider.log (raíz) --\n" + ultimasLineas(destinos.front().second / "sider.log", 30);
			}
			if (t.size() > 15000) t = t.substr(0, 15000) + "\n...(recortado)\n";
			return utf8Seguro(t);
		}
	}

	void registrarAccionesModulos(Interno& in) {
		Puente& p = *in.puente;

		// Carpeta del juego a mano (si Link no la ha visto todavía): se valida y se recuerda. No escribe nada en el juego.
		p.registrar("sync.carpetaJuego", [](const json& d, uint64_t) -> std::optional<json> {
			std::string ruta = texto(d, "ruta", 520);
			while (!ruta.empty() && (ruta.front() == ' ' || ruta.front() == '"' || ruta.front() == '\'')) ruta.erase(ruta.begin());
			while (!ruta.empty() && (ruta.back() == ' ' || ruta.back() == '"' || ruta.back() == '\'' || ruta.back() == '\\' || ruta.back() == '/')) ruta.pop_back();
			if (ruta.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Pega la carpeta de PES 2021 (la que tiene PES2021.exe).");
			std::filesystem::path c = phoenix::entrega::deU8(ruta);
			auto minusc = [](std::string s) { for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); return s; };
			if (minusc(phoenix::entrega::aU8(c.filename())) == "pes2021.exe") c = c.parent_path();
			if (minusc(phoenix::entrega::aU8(c.filename())) == "sideraddons") c = c.parent_path();
			std::error_code ec;
			if (!std::filesystem::is_regular_file(c / "PES2021.exe", ec))
				throw ErrorAccion("CARPETA_INVALIDA", "Esa carpeta no tiene PES2021.exe. Pega la carpeta principal del juego (la de PES2021.exe).");
			if (!std::filesystem::is_regular_file(c / "SiderAddons" / "sider.ini", ec))
				throw ErrorAccion("SIN_SIDER", "Esa carpeta no tiene SiderAddons\\sider.ini: ¿tu parche usa Sider? Phoenix lo necesita.");
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.carpetaJuego = phoenix::entrega::aU8(c);
			pr.guardar();
			json res = listaModulos();
			res["mensaje"] = "Carpeta del juego guardada.";
			return res;
		});

		// Informe de texto para depurar a distancia (botón «Copiar informe» de Diagnóstico). Solo lee archivos.
		p.registrar("diag.informe", [](const json&, uint64_t) -> std::optional<json> {
			bool abierto = false;
			const std::filesystem::path juego = carpetaJuego(abierto);
			return json{ {"texto", informeTexto(juego, abierto)} };
		});

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

		// Phoenix Doctor (MODO INFORMATIVO: aún no hay parche oficial; no aprueba ni rechaza). Solo lee archivos.
		// El informe queda guardado y viaja con el resultado oficial (detalle.doctor) para que el staff lo vea.
		p.registrar("sync.doctor", [&in](const json&, uint64_t) -> std::optional<json> {
			bool abierto = false;
			const std::filesystem::path juego = carpetaJuego(abierto);
			in.doctorJson = phoenix::doctor::comoJson(phoenix::doctor::revisar(juego));
			json r = json::parse(in.doctorJson, nullptr, false);
			if (r.is_discarded()) throw ErrorAccion("NO_SE_PUDO", "No se pudo revisar el juego.");
			r["pesAbierto"] = abierto;
			return r;
		});

		p.registrar("sync.modulos", [](const json&, uint64_t) -> std::optional<json> {
			return listaModulos();
		});

		p.registrar("sync.modulo", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string archivo = texto(d, "archivo", 64);
			const std::string que = texto(d, "accion", 16);
			if (que != "instalar" && que != "quitar" && que != "desinstalar") throw ErrorAccion("DATOS_INVALIDOS", "Acción desconocida.");
			bool abierto = false;
			const std::filesystem::path juego = carpetaJuego(abierto);
			if (abierto) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 primero: Sider lee los módulos al arrancar.");
			if (!phoenix::modulos::procesosAbiertos().empty()) throw ErrorAccion("PES_ABIERTO", "Cierra PES 2021 y Sider (sider.exe sigue abierto) antes de tocar sider.ini.");
			if (juego.empty()) throw ErrorAccion("SIN_JUEGO", "Abre PES 2021 una vez para que Phoenix Link conozca su carpeta.");
			const phoenix::modulos::Resultado r = que == "instalar"
				? phoenix::modulos::instalar(phoenix::modulos::carpetaPaquete(), juego, archivo)
				: que == "desinstalar" ? phoenix::modulos::desinstalar(phoenix::modulos::carpetaPaquete(), juego, archivo)
				: phoenix::modulos::quitar(phoenix::modulos::carpetaPaquete(), juego, archivo);
			if (!r.ok) throw ErrorAccion("NO_SE_PUDO", r.mensaje);
			json res = listaModulos();
			res["mensaje"] = r.mensaje;
			res["respaldos"] = r.respaldos;
			return res;
		});
	}

}
