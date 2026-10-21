#include "InterfazWebInterno.h"

#include "../PhoenixPrefs.h"
#include "../link/BuzonJuego.h"
#include "../link/Entrega.h"
#include "../link/ModulosJuego.h"
#include "../link/Doctor.h"
#include "../PhoenixBuild.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

// =============================================================================
//  Acciones de la interfaz web: «Módulos del juego» (SYNC › Puente).
//  Instala, actualiza o apaga los módulos Lua de Sider que lleva Phoenix Link
//  (phoenix/sider/*.lua). Solo con PES cerrado; sider.ini siempre con copia previa.
//  Detalle: phoenix/link/ModulosJuego.h y docs/CAMPO-2D-Y-ARBITRO.md («PHOENIX-ESTADIO-ARBITRO»).
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

	namespace detalle_fichajes {
		std::string textoFichajes(const std::filesystem::path& juego, bool conDetalle);   // AccionesFichajesWeb.cpp
	}

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

		bool soloLectura(const std::filesystem::path& p) {
#ifdef _WIN32
			const DWORD a = GetFileAttributesW(p.c_str());
			return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_READONLY) != 0;
#else
			(void)p; return false;
#endif
		}

		int copiasDeLink() {
			int n = 0;
#ifdef _WIN32
			HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snap == INVALID_HANDLE_VALUE) return 0;
			PROCESSENTRY32W pe{};
			pe.dwSize = sizeof(pe);
			if (Process32FirstW(snap, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PhoenixLink.exe") == 0) n++; } while (Process32NextW(snap, &pe));
			CloseHandle(snap);
#endif
			return n;
		}

		std::string minusculas(std::string s) { for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }

		/// Versión de Sider según la primera línea «Sider DLL: version 7.3.4 (0)» de sider.log ("" si no se sabe).
		std::string versionSider(const std::string& log) {
			const size_t i = log.find("Sider DLL: version ");
			if (i == std::string::npos) return std::string();
			size_t j = i + 19, k = j;
			while (k < log.size() && (std::isdigit(static_cast<unsigned char>(log[k])) || log[k] == '.')) k++;
			return log.substr(j, k - j);
		}
		bool siderMenorQue734(const std::string& v) {
			int a = 0, b = 0, c = 0;
			if (std::sscanf(v.c_str(), "%d.%d.%d", &a, &b, &c) < 2) return false;
			return a < 7 || (a == 7 && (b < 3 || (b == 3 && c < 4)));
		}

		/// Qué pasa y qué hacer, en lenguaje claro, para cada caso conocido (solo lee). Cada línea: «[nivel] Qué pasa → Qué hacer».
		std::vector<std::string> diagnosticoClaro(const std::filesystem::path& juego, bool pesAbierto) {
			std::vector<std::string> d;
			auto di = [&](const char* nivel, const std::string& pasa, const std::string& hacer) { d.push_back(std::string("[") + nivel + "] " + pasa + " → " + hacer); };
			if (juego.empty()) { di("X", "Link no sabe dónde está tu juego.", "Abre PES 2021 con Link abierto, o pega la carpeta (la de PES2021.exe) en SYNC › Módulos del juego."); return d; }
			const auto ds = phoenix::modulos::destinos(juego);
			if (ds.empty()) { di("X", "No hay SiderAddons\\sider.ini en esa carpeta: ese juego no usa Sider.", "Phoenix necesita Sider (el ConmeGOL ya lo trae). Comprueba que la carpeta es la de PES2021.exe y que abres el juego con el lanzador del parche."); return d; }
			if (pesAbierto || !phoenix::modulos::procesosAbiertos().empty()) di("!", "PES 2021 (o Sider) está abierto.", "Ciérralo del todo antes de instalar o cambiar módulos: Sider los lee al arrancar.");
			const std::string rj = minusculas(phoenix::entrega::aU8(juego));
			if (rj.find("\\program files") != std::string::npos || rj.find("\\archivos de programa") != std::string::npos)
				di("!", "El juego está en Archivos de programa (carpeta protegida).", "Si Link no puede escribir, ábrelo como administrador o mueve el juego a otra carpeta, p. ej. C:\\Juegos.");
			if (rj.find("onedrive") != std::string::npos)
				di("!", "El juego está dentro de OneDrive.", "OneDrive puede bloquear archivos mientras sincroniza: si falla, pausa la sincronización o mueve el juego fuera de OneDrive.");
			for (const auto& x : ds) {
				if (soloLectura(x.second / "sider.ini")) di("X", "El sider.ini de «" + x.first + "» está marcado como solo lectura.", "Quítale la marca (clic derecho › Propiedades) y vuelve a instalar. Link no lo pisa.");
				if (soloLectura(x.second / "modules")) di("!", "La carpeta modules de «" + x.first + "» está en solo lectura.", "Quítale la marca de solo lectura.");
			}
			std::string log;
			{
				std::ifstream f(ds.front().second / "sider.log", std::ios::binary);
				if (f) log.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
			}
			const std::string vs = versionSider(log);
			if (log.empty()) di("i", "Todavía no hay sider.log: el juego aún no se abrió con Sider.", "Abre PES con el lanzador del parche una vez y vuelve a revisar.");
			else if (vs.empty()) di("i", "No se pudo leer la versión de Sider en sider.log.", "Si algo no carga, manda este informe.");
			else if (siderMenorQue734(vs)) di("!", "Tu Sider es la versión " + vs + " (anterior a 7.3.4).", "El marcador y el HUD funcionan, pero el nombre del goleador (goal_scored) no llega. Con el ConmeGOL actualizado se arregla.");
			else di("OK", "Sider " + vs + ".", "Nada que hacer.");
			for (const phoenix::modulos::Modulo& m : phoenix::modulos::estado(phoenix::modulos::carpetaPaquete(), juego)) {
				if (!m.gestionable) continue;
				if (m.estado == "perdido") di("X", m.archivo + " se perdió: el parche reescribió sider.ini.", "Cierra PES y pulsa REINSTALAR en SYNC › Módulos del juego.");
				else if (m.estado == "desactualizado") di("!", m.archivo + " tiene una versión vieja.", "Cierra PES y pulsa ACTUALIZAR.");
				else if (m.estado == "no_instalado") di("!", m.archivo + " no está instalado.", "Cierra PES y pulsa INSTALAR.");
				else if (m.estado == "a_medias") di("!", m.archivo + " está instalado solo en algunas carpetas.", "Cierra PES y pulsa INSTALAR otra vez: completa lo que falte.");
				else if (m.estado == "apagado") di("i", m.archivo + " está apagado (línea comentada).", "Pulsa ENCENDER si lo quieres.");
				if (m.carga == "error") di("X", "Sider no pudo cargar " + m.archivo + " en el último arranque.", "Mira sider.log (al final de este informe) y manda el informe completo.");
				if (m.archivo == "phoenix.lua") {
					std::string sin;
					for (const auto& x : m.destinos) if (x.sinBase) sin += (sin.empty() ? "" : ", ") + x.nombre;
					if (!sin.empty()) di("i", "Phoenix-DB (fichajes) NO se puso en: " + sin + ".", "Es normal, no es un error: esa carpeta no tiene una base de parche reconocida en su sider.ini, y así no se tapan tus jugadores. Los fichajes de Phoenix no usan esa raíz: SYNC › Puente › «Fichajes de Phoenix» los genera sobre la base de cada modo.");
				}
			}
			{
				const std::string f = detalle_fichajes::textoFichajes(juego, false);
				if (!f.empty()) di("i", f, "Mira SYNC › Puente › Fichajes de Phoenix para el detalle por modo.");
			}
			const phoenix::modulos::Parche pa = phoenix::modulos::infoParche(juego);
			for (const std::string& w : pa.advertencias) di("!", w, "Desinstala phoenix.lua desde Link y vuelve a instalarlo.");
			const int copias = copiasDeLink();
			if (copias > 1) di("!", "Hay " + std::to_string(copias) + " copias de Phoenix Link abiertas.", "Cierra las demás: dos copias se pelean por los mandos y la sala.");
			return d;
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
			t += "\n\n-- Qué pasa y qué hacer (en claro) --\n";
			for (const std::string& l : diagnosticoClaro(juego, pesAbierto)) t += l + "\n";
			t += "\n-- Módulos de Phoenix --\n";
			for (const phoenix::modulos::Modulo& m : phoenix::modulos::estado(phoenix::modulos::carpetaPaquete(), juego)) {
				t += m.archivo + " (Link lleva v" + m.version + ") estado=" + m.estado + " carga=" + (m.carga.empty() ? "?" : m.carga) + "\n";
				for (const phoenix::modulos::Destino& x : m.destinos)
					t += "   · " + x.nombre + ": archivo=" + (x.archivo ? "sí" : "no") + (x.version.empty() ? "" : " v" + x.version) + " igual=" + (x.igual ? "sí" : "no")
						+ " línea=" + (x.linea ? "sí" : "no") + (x.perdida ? " PERDIDA" : "") + (x.sinBase ? " sinBase" : "") + "\n";
			}
			t += "\n-- Fichajes de Phoenix --\n" + detalle_fichajes::textoFichajes(juego, true);
			t += "\n-- Archivos que deciden los datos (sha256 corto) --\n";
			for (const phoenix::doctor::Archivo& a : d.archivos) t += a.nombre + ": " + (a.sha256.empty() ? std::string("no está") : a.sha256.substr(0, 12) + " · " + std::to_string(a.bytes) + " bytes") + "\n";
			const auto destinos = phoenix::modulos::destinos(juego);
			if (!destinos.empty()) {
				t += "\n-- Final de sider.log (raíz) --\n" + ultimasLineas(destinos.front().second / "sider.log", 30);
			}
			// Sin datos personales: el nombre de usuario de Windows se tapa en las rutas.
			if (const char* usuario = std::getenv("USERNAME")) {
				const std::string quien = std::string("\\Users\\") + usuario, tapa = "\\Users\\<usuario>";
				if (std::strlen(usuario) > 0)
					for (size_t i = 0; (i = t.find(quien, i)) != std::string::npos; i += tapa.size()) t.replace(i, quien.size(), tapa);
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
