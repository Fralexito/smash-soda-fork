#include "InterfazWebInterno.h"

#include <Windows.h>
#include <ShlObj.h>
#include <TlHelp32.h>

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "../core/PerfilesAmigos.h"
#include "../core/PerfilesPes.h"

// =============================================================================
//  Acciones de la interfaz web: «Perfiles de amigos» (GENTE › PERFILES PES).
//  PES guarda 32 «Datos personales»; Link guarda los de TODOS tus amigos y pone en PES solo los de hoy.
//    perfiles.estado    → ruta del guardado, huecos 1…32, perfiles, selección, ¿PES abierto?
//    perfiles.traer     → PES → Link (copia de seguridad de todos los Usuarios; solo lee PES)
//    perfiles.preparar  → Link → PES (con PES cerrado; copia con sello del guardado antes de escribir)
//    perfiles.seleccion / proteger / borrar / renombrar / ruta
//  Detalle y reglas: docs/PERFILES-DE-AMIGOS.md. Todo con PES CERRADO para escribir.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;
	namespace fs = std::filesystem;
	namespace pp = phoenix::perfilespes;

	namespace {

		std::mutex mtxPerfiles;

		std::string selloPerfiles() {
			std::time_t t = std::time(nullptr);
			std::tm tm{};
			localtime_s(&tm, &t);
			char b[32];
			std::strftime(b, sizeof b, "%Y%m%d-%H%M%S", &tm);
			return b;
		}

		std::string aUtf8Perfiles(const fs::path& p) { const auto s = p.u8string(); return std::string(s.begin(), s.end()); }

		bool pesAbiertoPerfiles() {
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (s == INVALID_HANDLE_VALUE) return false;
			PROCESSENTRY32W p{}; p.dwSize = sizeof p;
			bool hay = false;
			for (BOOL ok = Process32FirstW(s, &p); ok; ok = Process32NextW(s, &p))
				if (_wcsicmp(p.szExeFile, L"PES2021.exe") == 0) { hay = true; break; }
			CloseHandle(s);
			return hay;
		}

		// Carpeta KONAMI de los Documentos de Windows (puede estar en OneDrive o en otro disco: se pregunta a Windows, no se fija).
		fs::path carpetaKonamiPerfiles() {
			fs::path r;
			PWSTR docs = nullptr;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) r = docs;
			if (docs) CoTaskMemFree(docs);
			return r / L"KONAMI";
		}

		struct CandidatoPerfiles { std::string ruta; int64_t modificado = 0; };

		// Busca KONAMI/eFootball PES 2021*/.../save/SYSTEM00000000 (hasta 3 niveles: SteamID/save, parche/año/save...).
		// Cualquier PC, cualquier SteamID y cualquier parche: solo cuenta lo que se abre como guardado SYSTEM válido de PES 2021.
		void buscarEn(const fs::path& dir, int nivel, std::vector<CandidatoPerfiles>& v) {
			std::error_code ec;
			for (auto& e : fs::directory_iterator(dir, ec)) {
				if (!e.is_directory(ec)) continue;
				const fs::path f = e.path() / L"save" / L"SYSTEM00000000";
				if (fs::is_regular_file(f, ec)) {
					pp::Guardado g; std::string err;
					if (pp::Guardado::abrir(aUtf8Perfiles(f), g, err)) {
						CandidatoPerfiles c; c.ruta = aUtf8Perfiles(f);
						c.modificado = static_cast<int64_t>(fs::last_write_time(f, ec).time_since_epoch().count());
						v.push_back(std::move(c));
					}
				}
				if (nivel < 3 && e.path().filename() != L"save") buscarEn(e.path(), nivel + 1, v);
			}
		}

		std::vector<CandidatoPerfiles> buscarGuardadosPerfiles() {
			std::vector<CandidatoPerfiles> v;
			std::error_code ec;
			const fs::path konami = carpetaKonamiPerfiles();
			if (!fs::is_directory(konami, ec)) return v;
			for (auto& e : fs::directory_iterator(konami, ec)) {
				const std::wstring n = e.path().filename().wstring();
				if (e.is_directory(ec) && n.rfind(L"eFootball PES 2021", 0) == 0) buscarEn(e.path(), 1, v);
			}
			std::sort(v.begin(), v.end(), [](const CandidatoPerfiles& a, const CandidatoPerfiles& b) { return a.modificado > b.modificado; });
			return v;
		}

		std::string carpetaBasePerfiles() { return rutaConfig("phoenix-perfiles-amigos"); }
		std::string archivoAjustesPerfiles() { return carpetaBasePerfiles() + "\\ajustes.json"; }

		struct AjustesPerfiles { std::string ruta; bool autoRecoger = true; };

		AjustesPerfiles leerAjustesPerfiles() {
			AjustesPerfiles a;
			try {
				std::ifstream in(fs::u8path(archivoAjustesPerfiles()));
				const auto j = json::parse(in, nullptr, false);
				if (j.is_object()) { a.ruta = j.value("ruta", std::string()); a.autoRecoger = j.value("autoRecoger", true); }
			}
			catch (...) {}
			return a;
		}

		void guardarAjustesPerfiles(const AjustesPerfiles& a) {
			std::error_code ec;
			fs::create_directories(fs::u8path(carpetaBasePerfiles()), ec);
			std::ofstream o(fs::u8path(archivoAjustesPerfiles()), std::ios::trunc);
			o << json{ {"ruta", a.ruta}, {"autoRecoger", a.autoRecoger} }.dump(1);
		}

		// El guardado que se usa: el elegido a mano si existe y es válido; si no, el más reciente.
		std::string rutaElegidaPerfiles(const std::vector<CandidatoPerfiles>& c, const AjustesPerfiles& a) {
			if (!a.ruta.empty()) for (auto& x : c) if (x.ruta == a.ruta) return a.ruta;
			return c.empty() ? std::string() : c.front().ruta;
		}

		bool pesFueAbiertoPerfiles = false;     // para «recoger solo al cerrar PES»
		std::string avisoAutoPerfiles;          // lo último que hizo Link solo

		json informeJsonPerfiles(const pp::Informe& i) {
			json j = { {"nuevos", i.nuevos}, {"actualizados", i.actualizados}, {"iguales", i.iguales}, {"escritos", i.escritos}, {"copia", i.copia}, {"avisos", i.avisos} };
			j["puestos"] = json::array();
			for (auto& p : i.puestos) j["puestos"].push_back({ {"hueco", p.hueco}, {"perfil", p.perfilId} });
			return j;
		}

		json estadoPerfiles(const json& informe = json()) {
			const AjustesPerfiles aj = leerAjustesPerfiles();
			const auto cands = buscarGuardadosPerfiles();
			const std::string ruta = rutaElegidaPerfiles(cands, aj);
			const bool abierto = pesAbiertoPerfiles();

			pp::Base base(carpetaBasePerfiles());
			std::string err;
			json r = { {"ruta", ruta}, {"pesAbierto", abierto}, {"autoRecoger", aj.autoRecoger}, {"candidatos", json::array()} };
			for (auto& c : cands) r["candidatos"].push_back(c.ruta);
			if (!base.cargar(err)) { r["error"] = err; return r; }

			// «recoger solo al cerrar PES»: cuando PES pasa de abierto a cerrado y hay guardado, se traen los Usuarios
			if (pesFueAbiertoPerfiles && !abierto && aj.autoRecoger && !ruta.empty()) {
				pp::Informe i; std::string e2;
				const std::string st = selloPerfiles();
				if (pp::traerDePes(ruta, base, st, i, e2) && base.guardar(st, e2))
					avisoAutoPerfiles = (i.nuevos + i.actualizados) ? ("PES se cerró: guardé " + std::to_string(i.nuevos) + " perfil(es) nuevo(s) y " + std::to_string(i.actualizados) + " actualizado(s)") : std::string("PES se cerró: no hubo cambios en los Usuarios");
				else avisoAutoPerfiles = "PES se cerró, pero no pude recoger los Usuarios: " + e2;
			}
			pesFueAbiertoPerfiles = abierto;
			if (!avisoAutoPerfiles.empty()) r["avisoAuto"] = avisoAutoPerfiles;
			if (!informe.is_null()) r["informe"] = informe;

			r["huecos"] = json::array();
			std::vector<pp::Usuario> us;
			if (!ruta.empty()) {
				pp::Guardado g;
				if (pp::Guardado::abrir(ruta, g, err)) us = g.usuarios(); else r["error"] = "no pude leer el guardado: " + err;
			}
			for (int i = 0; i < pp::kUsuarios; i++) {
				json h = { {"n", i + 1}, {"protegido", base.protegido(i + 1)}, {"nombre", i < static_cast<int>(us.size()) ? us[i].nombre : std::string()}, {"perfil", nullptr} };
				if (i < static_cast<int>(us.size())) {
					const std::string hex = pp::aHex(us[i].bytes);
					for (auto& p : base.datos.perfiles) if (p.registro == hex) { h["perfil"] = p.id; break; }
				}
				r["huecos"].push_back(std::move(h));
			}
			r["perfiles"] = json::array();
			for (auto& p : base.datos.perfiles) {
				int donde = 0;
				for (auto& h : r["huecos"]) if (h["perfil"] == p.id) { donde = h["n"].get<int>(); break; }
				r["perfiles"].push_back({ {"id", p.id}, {"nombre", p.nombre}, {"sinNombre", p.sinNombre}, {"actualizado", p.actualizado}, {"enHueco", donde}, {"tieneAnterior", !p.anterior.empty()} });
			}
			r["seleccion"] = base.datos.seleccion;
			r["ultimaSala"] = base.datos.ultimaSala;
			return r;
		}

		void requerirRutaPerfiles(const std::string& ruta) {
			if (ruta.empty()) throw ErrorAccion("SIN_GUARDADO", "No encuentro el guardado de PES (SYSTEM00000000). Abre PES una vez y termina un partido.");
		}
	}

	void registrarAccionesPerfiles(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("perfiles.estado", [](const json&, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			return estadoPerfiles();
		});

		// PES → Link: guarda todos los Usuarios de PES como perfiles (solo lee PES).
		p.registrar("perfiles.traer", [](const json&, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			const std::string ruta = rutaElegidaPerfiles(buscarGuardadosPerfiles(), leerAjustesPerfiles());
			requerirRutaPerfiles(ruta);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			pp::Informe inf;
			const std::string st = selloPerfiles();
			if (!pp::traerDePes(ruta, base, st, inf, err)) throw ErrorAccion("NO_SE_PUDO", err);
			if (!base.guardar(st, err)) throw ErrorAccion("NO_SE_PUDO", err);
			avisoAutoPerfiles.clear();
			return estadoPerfiles(informeJsonPerfiles(inf));
		});

		// Link → PES: con PES cerrado.
		p.registrar("perfiles.preparar", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			if (pesAbiertoPerfiles()) throw ErrorAccion("PES_ABIERTO", "Cierra PES antes de preparar la sala (PES lee los Usuarios al arrancar).");
			const std::string ruta = rutaElegidaPerfiles(buscarGuardadosPerfiles(), leerAjustesPerfiles());
			requerirRutaPerfiles(ruta);
			std::vector<std::string> ids;
			if (d.contains("ids") && d["ids"].is_array()) for (auto& x : d["ids"]) if (x.is_string()) ids.push_back(x.get<std::string>());
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			pp::Informe inf;
			const std::string st = selloPerfiles();
			const bool ok = pp::prepararSala(ruta, base, ids, st, inf, err);
			std::string e2;
			base.guardar(st, e2);   // lo traído de PES se guarda aunque la preparación falle (nada se pierde)
			if (!ok) throw ErrorAccion("NO_SE_PUDO", err);
			avisoAutoPerfiles.clear();
			return estadoPerfiles(informeJsonPerfiles(inf));
		});

		p.registrar("perfiles.seleccion", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			std::vector<std::string> ids;
			if (d.contains("ids") && d["ids"].is_array()) for (auto& x : d["ids"]) if (x.is_string() && base.porId(x.get<std::string>())) ids.push_back(x.get<std::string>());
			base.datos.seleccion = ids;
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles();
		});

		p.registrar("perfiles.proteger", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			const int hueco = entero(d, "hueco", 1, pp::kUsuarios);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			base.proteger(hueco, booleano(d, "si"));
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles();
		});

		p.registrar("perfiles.borrar", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			if (!base.borrar(texto(d, "id", 20))) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);   // queda copia en respaldos
			return estadoPerfiles();
		});

		// Cambia el nombre del perfil (el que verá PES al ponerlo en un hueco). El nombre va dentro del registro.
		p.registrar("perfiles.renombrar", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			const std::string id = texto(d, "id", 20), nombre = texto(d, "nombre", 64);
			if (pp::claveNombre(nombre).empty()) throw ErrorAccion("DATOS_INVALIDOS", "Escribe un nombre.");
			if (nombre.size() > pp::kMaxNombre) throw ErrorAccion("DATOS_INVALIDOS", "El nombre es demasiado largo (máx. " + std::to_string(pp::kMaxNombre) + " letras).");
			for (unsigned char c : nombre) if (c < 0x20 || c == 0x7F) throw ErrorAccion("DATOS_INVALIDOS", "El nombre tiene caracteres no válidos.");
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			const pp::PerfilAmigo* ex = base.porNombre(nombre);
			if (ex && ex->id != id) throw ErrorAccion("DATOS_INVALIDOS", "Ya hay otro perfil con ese nombre.");
			auto it = std::find_if(base.datos.perfiles.begin(), base.datos.perfiles.end(), [&](const pp::PerfilAmigo& x) { return x.id == id; });
			if (it == base.datos.perfiles.end()) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			std::vector<uint8_t> reg;
			if (!pp::deHex(it->registro, reg) || reg.size() != pp::kTamUsuario) throw ErrorAccion("NO_SE_PUDO", "El perfil está dañado.");
			std::fill(reg.begin() + 4, reg.begin() + 4 + pp::kTamNombre, uint8_t(0));
			std::copy(nombre.begin(), nombre.end(), reg.begin() + 4);
			it->anterior = it->registro; it->registro = pp::aHex(reg); it->nombre = nombre; it->sinNombre = false; it->actualizado = selloPerfiles();
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles();
		});

		p.registrar("perfiles.ruta", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			AjustesPerfiles a = leerAjustesPerfiles();
			if (d.contains("ruta")) {
				const std::string r = texto(d, "ruta", 600);
				const auto c = buscarGuardadosPerfiles();
				if (!r.empty() && std::none_of(c.begin(), c.end(), [&](const CandidatoPerfiles& x) { return x.ruta == r; })) throw ErrorAccion("DATOS_INVALIDOS", "Ese no es un guardado de PES válido de los que encontré.");
				a.ruta = r;
			}
			if (d.contains("autoRecoger")) a.autoRecoger = booleano(d, "autoRecoger");
			guardarAjustesPerfiles(a);
			return estadoPerfiles();
		});
	}
}
