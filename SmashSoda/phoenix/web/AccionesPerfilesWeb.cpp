#include "InterfazWebInterno.h"

#include <Windows.h>
#include <ShlObj.h>
#include <TlHelp32.h>

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>

#include "../core/OpcionesUsuario.h"
#include "../core/PerfilesAmigos.h"
#include "../core/PerfilesPes.h"
#include "../core/UsuariosEnVivo.h"

// =============================================================================
//  Acciones de la interfaz web: «Perfiles de amigos» (GENTE › PERFILES PES).
//  PES guarda 32 «Datos personales»; Link guarda los de TODOS tus amigos y pone en PES solo los de hoy.
//    perfiles.estado    → ruta del guardado, huecos 1…32, perfiles, selección, ¿PES abierto?
//    perfiles.traer     → PES → Link (copia de seguridad de todos los Usuarios; solo lee PES)
//    perfiles.preparar  → Link → PES (con PES cerrado; copia con sello del guardado antes de escribir)
//    perfiles.seleccion / proteger / borrar / renombrar / ruta
//  Detalle y reglas: docs/MANDOS.md («PERFILES-DE-AMIGOS»). Todo con PES CERRADO para escribir.
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

		struct AjustesPerfiles { std::string ruta; bool autoRecoger = true; bool autoUsuarios = true; };

		AjustesPerfiles leerAjustesPerfiles() {
			AjustesPerfiles a;
			try {
				std::ifstream in(fs::u8path(archivoAjustesPerfiles()));
				const auto j = json::parse(in, nullptr, false);
				if (j.is_object()) { a.ruta = j.value("ruta", std::string()); a.autoRecoger = j.value("autoRecoger", true); a.autoUsuarios = j.value("autoUsuarios", true); }
			}
			catch (...) {}
			return a;
		}

		void guardarAjustesPerfiles(const AjustesPerfiles& a) {
			std::error_code ec;
			fs::create_directories(fs::u8path(carpetaBasePerfiles()), ec);
			std::ofstream o(fs::u8path(archivoAjustesPerfiles()), std::ios::trunc);
			o << json{ {"ruta", a.ruta}, {"autoRecoger", a.autoRecoger}, {"autoUsuarios", a.autoUsuarios} }.dump(1);
		}

		// El guardado que se usa: el elegido a mano si existe y es válido; si no, el más reciente.
		std::string rutaElegidaPerfiles(const std::vector<CandidatoPerfiles>& c, const AjustesPerfiles& a) {
			if (!a.ruta.empty()) for (auto& x : c) if (x.ruta == a.ruta) return a.ruta;
			return c.empty() ? std::string() : c.front().ruta;
		}

		// Perfiles puestos EN VIVO en la memoria de PES (prueba): puesto 1…8, Usuario 0…31 y el registro original para restaurarlo
		struct AplicadoVivo { int puesto = 0; int usuario = 0; std::string perfilId; std::vector<uint8_t> original; };
		std::vector<AplicadoVivo> aplicadosVivo;

		// ---- USUARIO AUTOMÁTICO: cuando alguien con perfil vinculado se sienta, Link le pone su perfil en el Usuario de su mando ----
		struct SentadoAuto { int puesto = 0; uint32_t parsecId = 0; std::string nombre; bool simulado = false; };
		struct AplicadoAuto { int puesto = 0; int usuario = 0; uint32_t parsecId = 0; std::string perfilId; std::vector<uint8_t> original; std::string escrito; };
		std::vector<SentadoAuto> sentadosAuto;            // lo último que vio el vigilante (para la interfaz)
		std::vector<AplicadoAuto> aplicadosAuto;          // lo que Link pisó y debe devolver
		std::map<int, uint32_t> simuladosAuto;            // puesto -> parsecId: sentados de PRUEBA (sin gente de verdad)
		std::string logAuto;                              // último aviso del vigilante
		std::unique_ptr<pp::vivo::Conexion> conexionAuto;
		double ultimoAuto = 0;

		bool nombreDeRegistroValido(const std::vector<uint8_t>& reg, std::string& nombre) {
			size_t n = 0; while (n < pp::kTamNombre && reg[4 + n]) n++;
			nombre.assign(reinterpret_cast<const char*>(&reg[4]), n);
			if (!pp::utf8Valido(nombre)) { nombre.clear(); return false; }
			return !nombre.empty();
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
				r["perfiles"].push_back({ {"id", p.id}, {"nombre", p.nombre}, {"sinNombre", p.sinNombre}, {"actualizado", p.actualizado}, {"enHueco", donde}, {"tieneAnterior", !p.anterior.empty()}, {"parsecId", p.parsecId} });
			}
			r["vivo"] = json::array();
			for (auto& a : aplicadosVivo) r["vivo"].push_back({ {"puesto", a.puesto}, {"usuario", a.usuario + 1}, {"perfil", a.perfilId} });
			r["auto"] = aj.autoUsuarios;
			r["autoLog"] = logAuto;
			r["sentados"] = json::array();
			for (auto& s : sentadosAuto) r["sentados"].push_back({ {"puesto", s.puesto}, {"parsecId", s.parsecId}, {"nombre", s.nombre}, {"simulado", s.simulado}, {"perfil", base.porParsec(s.parsecId) ? base.porParsec(s.parsecId)->id : std::string()} });
			r["autoAplicados"] = json::array();
			for (auto& a : aplicadosAuto) r["autoAplicados"].push_back({ {"puesto", a.puesto}, {"usuario", a.usuario + 1}, {"perfil", a.perfilId} });
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
			std::string motivoNombre;
			if (!pp::ponerNombre(reg, nombre, motivoNombre)) throw ErrorAccion("DATOS_INVALIDOS", motivoNombre);   // no pisa las opciones que comparten el campo
			it->anterior = it->registro; it->registro = pp::aHex(reg); it->nombre = nombre; it->sinNombre = false; it->actualizado = selloPerfiles();
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles();
		});

		// ---- EN VIVO (prueba, beta): poner el perfil de un amigo en el Usuario que tiene un mando, con PES ABIERTO ----
		// Usa la memoria de PES (core/UsuariosEnVivo.h). Solo escribe el registro de UN Usuario; guarda antes lo que había (en Link y aquí)
		// y lo restaura con perfiles.restaurar. Nunca toca archivos de guardado.
		auto nombreDeRegistro = [](const std::vector<uint8_t>& reg) { size_t n = 0; while (n < pp::kTamNombre && reg[4 + n]) n++; std::string s(reinterpret_cast<const char*>(&reg[4]), n); return pp::utf8Valido(s) ? s : std::string(); };

		p.registrar("perfiles.mandos", [nombreDeRegistro](const json&, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			pp::vivo::Conexion c; std::string err;
			if (!c.abrir(err)) throw ErrorAccion("PES_NO_LISTO", err);
			int m[pp::vivo::kMandos];
			if (!c.leerMandos(m, err)) throw ErrorAccion("NO_SE_PUDO", err);
			json r = { {"mandos", json::array()} };
			for (int i = 0; i < pp::vivo::kMandos; i++) {
				std::vector<uint8_t> reg; std::string e;
				// usuario 0 = ese mando todavía no tiene Usuario en PES
				r["mandos"].push_back({ {"puesto", i + 1}, {"usuario", m[i] + 1}, {"nombre", m[i] >= 0 && c.leerUsuario(m[i], reg, e) ? nombreDeRegistro(reg) : std::string()} });
			}
			return r;
		});

		p.registrar("perfiles.aplicar", [nombreDeRegistro](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			const int puesto = entero(d, "puesto", 1, pp::vivo::kMandos);
			const std::string id = texto(d, "id", 20);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			const pp::PerfilAmigo* perfil = base.porId(id);
			if (!perfil) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			std::vector<uint8_t> nuevo;
			if (!pp::deHex(perfil->registro, nuevo)) throw ErrorAccion("NO_SE_PUDO", "El perfil está dañado.");
			const std::string nombrePerfil = perfil->nombre;
			pp::vivo::Conexion c;
			if (!c.abrir(err)) throw ErrorAccion("PES_NO_LISTO", err);
			int m[pp::vivo::kMandos];
			if (!c.leerMandos(m, err)) throw ErrorAccion("NO_SE_PUDO", err);
			const int usuario = m[puesto - 1];
			if (usuario < 0) throw ErrorAccion("MANDO_SIN_USUARIO", "El mando " + std::to_string(puesto) + " todavía no tiene Usuario en PES. Entra con ese mando a la pantalla de elegir lados y vuelve a probar.");
			std::vector<uint8_t> actual;
			if (!c.leerUsuario(usuario, actual, err)) throw ErrorAccion("NO_SE_PUDO", err);
			// nada se pierde: lo que hay ahora en ese Usuario se guarda como perfil si no lo estaba (salvo que ya lo hubiéramos pisado antes)
			auto ya = std::find_if(aplicadosVivo.begin(), aplicadosVivo.end(), [&](const AplicadoVivo& a) { return a.usuario == usuario; });
			const std::string hexActual = pp::aHex(actual);
			const bool guardado = std::any_of(base.datos.perfiles.begin(), base.datos.perfiles.end(), [&](const pp::PerfilAmigo& x) { return x.registro == hexActual; });
			if (!guardado && ya == aplicadosVivo.end()) {
				int mx = 0;
				for (auto& x : base.datos.perfiles) if (x.id.size() > 1 && x.id[0] == 'p') mx = (std::max)(mx, atoi(x.id.c_str() + 1));
				pp::PerfilAmigo n; n.id = "p" + std::to_string(mx + 1); n.registro = hexActual; n.actualizado = selloPerfiles();
				std::string nom = nombreDeRegistro(actual);
				n.sinNombre = nom.empty();
				if (nom.empty() || base.porNombre(nom)) nom = "Usuario " + std::to_string(usuario + 1) + " (antes del vivo)";
				while (base.porNombre(nom)) nom += "+";
				n.nombre = nom;
				base.datos.perfiles.push_back(n);
				if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", "No pude guardar una copia de lo que había: " + err);
			}
			std::vector<uint8_t> anterior;
			if (!c.escribirUsuario(usuario, nuevo, anterior, err)) throw ErrorAccion("NO_SE_PUDO", err);
			if (ya != aplicadosVivo.end()) { ya->puesto = puesto; ya->perfilId = id; }
			else { AplicadoVivo a; a.puesto = puesto; a.usuario = usuario; a.perfilId = id; a.original = anterior; aplicadosVivo.push_back(std::move(a)); }
			return estadoPerfiles(json{ {"vivoAplicado", { {"puesto", puesto}, {"usuario", usuario + 1}, {"nombre", nombrePerfil} }} });
		});

		p.registrar("perfiles.restaurar", [](const json&, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			if (aplicadosVivo.empty()) return estadoPerfiles();
			pp::vivo::Conexion c; std::string err;
			if (!c.abrir(err)) {
				// PES está abierto pero la lista de mandos solo existe en la pantalla de elegir lados: NO se olvida lo pendiente
				if (pesAbiertoPerfiles()) throw ErrorAccion("PES_NO_LISTO", "Para restaurar, vuelve en PES a la pantalla de elegir lados y pulsa otra vez. (" + err + ")");
				// PES ya se cerró: los originales están guardados como perfiles en Link; no hay nada que restaurar en memoria
				aplicadosVivo.clear();
				return estadoPerfiles(json{ {"vivoAviso", "PES está cerrado: lo que había quedó guardado en Link (Preparar la sala lo vuelve a poner)."} });
			}
			std::vector<AplicadoVivo> fallidos; std::string errores;
			for (auto& a : aplicadosVivo) {
				std::vector<uint8_t> ant; std::string e;
				if (!c.escribirUsuario(a.usuario, a.original, ant, e)) { fallidos.push_back(a); errores += "Usuario " + std::to_string(a.usuario + 1) + ": " + e + " "; }
			}
			aplicadosVivo = fallidos;
			if (!errores.empty()) throw ErrorAccion("NO_SE_PUDO", errores);
			return estadoPerfiles();
		});

		// Liga un perfil a una persona (parsecId): cuando se siente en un puesto, Link le pone ese perfil solo. parsecId 0 = quitar.
		p.registrar("perfiles.vincular", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			const uint32_t pid = d.contains("parsecId") && d["parsecId"].is_number_unsigned() ? d["parsecId"].get<uint32_t>() : 0;
			if (!base.vincular(texto(d, "id", 20), pid)) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles();
		});

		p.registrar("perfiles.auto", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			AjustesPerfiles a = leerAjustesPerfiles();
			a.autoUsuarios = booleano(d, "si");
			guardarAjustesPerfiles(a);
			if (!a.autoUsuarios) logAuto = "Automático apagado: Link ya no pone perfiles solo.";
			return estadoPerfiles();
		});

		// Sentados de PRUEBA (sin gente de verdad): {puesto, parsecId}; parsecId 0 quita ese puesto; sin puesto quita todos.
		p.registrar("perfiles.simular", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			if (!d.contains("puesto")) { simuladosAuto.clear(); return estadoPerfiles(); }
			const int puesto = entero(d, "puesto", 1, pp::vivo::kMandos);
			const uint32_t pid = d.contains("parsecId") && d["parsecId"].is_number_unsigned() ? d["parsecId"].get<uint32_t>() : 0;
			if (pid) simuladosAuto[puesto] = pid; else simuladosAuto.erase(puesto);
			return estadoPerfiles();
		});

		// ---- MI PERFIL DE PES: el formulario de nombre + opciones (todo por BYTES; las etiquetas viven en la interfaz, por parche) ----
		// perfiles.opciones{id?} → las opciones de ese perfil; sin id, las de partida para uno nuevo (las de un registro real de la base).
		p.registrar("perfiles.opciones", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			const std::string id = d.contains("id") ? texto(d, "id", 20) : std::string();
			std::vector<uint8_t> reg;
			std::string nombre;
			if (!id.empty()) {
				const pp::PerfilAmigo* pf = base.porId(id);
				if (!pf) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
				if (!pp::deHex(pf->registro, reg)) throw ErrorAccion("NO_SE_PUDO", "El perfil está dañado.");
				nombre = pf->sinNombre ? std::string() : pf->nombre;
			}
			else {
				for (auto& pf : base.datos.perfiles) if (pp::deHex(pf.registro, reg) && reg.size() == pp::kTamUsuario) break; else reg.clear();
				if (reg.empty()) throw ErrorAccion("SIN_PERFILES", "Primero trae los Usuarios de PES (Avanzado › Traer de PES): sirven de base para un perfil nuevo.");
			}
			pp::OpcionesUsuario o;
			if (!pp::leerOpciones(reg, o)) throw ErrorAccion("NO_SE_PUDO", "El perfil está dañado.");
			if (id.empty()) o.tipoCursor = 4;
			json op = { {"desplazamiento", o.desplazamiento}, {"preajuste", o.preajuste}, {"botones", json::array()}, {"cambiarCursor", o.cambiarCursor}, {"cursorSiguiente", o.cursorSiguiente}, {"tipoCursor", o.tipoCursor},
				{"nivelPases", o.nivelPases}, {"paseProfundidad", o.paseProfundidad}, {"disparo", o.disparo}, {"guiaDireccion", o.guiaDireccion}, {"guiaObjetivo", o.guiaObjetivo}, {"guiaPosicion", o.guiaPosicion},
				{"amago", o.amago}, {"entrada", o.entrada}, {"companero", o.companero} };
			for (int b : o.botones) op["botones"].push_back(b);
			return json{ {"id", id}, {"nombre", nombre}, {"opciones", op} };
		});

		// perfiles.editar{id?, nombre, parsecId?, opciones{…}} → crea (sin id) o actualiza un perfil. Parte del registro real del perfil
		// (o de uno de la base si es nuevo) y cambia SOLO el nombre y los bytes de opciones conocidos. «Tipo de cursor» siempre «Datos personales».
		p.registrar("perfiles.editar", [](const json& d, uint64_t) -> std::optional<json> {
			std::lock_guard<std::mutex> g(mtxPerfiles);
			const std::string id = d.contains("id") ? texto(d, "id", 20) : std::string();
			const std::string nombre = texto(d, "nombre", 64);
			if (!d.contains("opciones") || !d["opciones"].is_object()) throw ErrorAccion("DATOS_INVALIDOS", "Faltan las opciones.");
			const json& op = d["opciones"];
			auto num = [&](const char* k, int def) { return op.contains(k) && op[k].is_number_integer() ? op[k].get<int>() : def; };
			pp::Base base(carpetaBasePerfiles());
			std::string err;
			if (!base.cargar(err)) throw ErrorAccion("BASE_DANADA", err);
			std::vector<uint8_t> reg;
			pp::PerfilAmigo* destino = nullptr;
			if (!id.empty()) {
				auto it = std::find_if(base.datos.perfiles.begin(), base.datos.perfiles.end(), [&](const pp::PerfilAmigo& x) { return x.id == id; });
				if (it == base.datos.perfiles.end()) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
				destino = &*it;
				if (!pp::deHex(it->registro, reg) || reg.size() != pp::kTamUsuario) throw ErrorAccion("NO_SE_PUDO", "El perfil está dañado.");
			}
			else {
				if (base.datos.perfiles.size() >= pp::Base::kMaxPerfiles) throw ErrorAccion("DEMASIADOS", "Ya hay demasiados perfiles en Link.");
				for (auto& pf : base.datos.perfiles) if (pp::deHex(pf.registro, reg) && reg.size() == pp::kTamUsuario) break; else reg.clear();
				if (reg.empty()) throw ErrorAccion("SIN_PERFILES", "Primero trae los Usuarios de PES (Avanzado › Traer de PES): sirven de base para un perfil nuevo.");
			}
			pp::OpcionesUsuario o;
			pp::leerOpciones(reg, o);   // lo que no venga en el formulario se queda como estaba
			o.desplazamiento = num("desplazamiento", o.desplazamiento); o.preajuste = num("preajuste", o.preajuste);
			if (op.contains("botones") && op["botones"].is_array() && op["botones"].size() == o.botones.size())
				for (size_t i = 0; i < o.botones.size(); i++) o.botones[i] = op["botones"][i].is_number_integer() ? op["botones"][i].get<int>() : o.botones[i];
			o.cambiarCursor = num("cambiarCursor", o.cambiarCursor); o.cursorSiguiente = num("cursorSiguiente", o.cursorSiguiente);
			o.tipoCursor = 4;   // pedido de Fralex: siempre «Datos personales»
			o.nivelPases = num("nivelPases", o.nivelPases); o.paseProfundidad = num("paseProfundidad", o.paseProfundidad); o.disparo = num("disparo", o.disparo);
			o.guiaDireccion = num("guiaDireccion", o.guiaDireccion); o.guiaObjetivo = num("guiaObjetivo", o.guiaObjetivo); o.guiaPosicion = num("guiaPosicion", o.guiaPosicion);
			o.amago = num("amago", o.amago); o.entrada = num("entrada", o.entrada); o.companero = num("companero", o.companero);
			std::string motivo;
			if (!pp::ponerOpciones(reg, o, motivo) || !pp::ponerNombre(reg, nombre, motivo)) throw ErrorAccion("DATOS_INVALIDOS", motivo);
			if (!pp::registroValido(reg, motivo)) throw ErrorAccion("NO_SE_PUDO", "El perfil que salió no es válido para PES (" + motivo + "). No se guardó.");
			const pp::PerfilAmigo* ex = base.porNombre(nombre);
			if (ex && ex->id != id) throw ErrorAccion("DATOS_INVALIDOS", "Ya hay otro perfil con ese nombre.");
			std::string idFinal = id;
			if (destino) {
				if (destino->registro != pp::aHex(reg)) destino->anterior = destino->registro;
				destino->registro = pp::aHex(reg); destino->nombre = nombre; destino->sinNombre = false; destino->actualizado = selloPerfiles();
			}
			else {
				int mx = 0;
				for (auto& x : base.datos.perfiles) if (x.id.size() > 1 && x.id[0] == 'p') mx = (std::max)(mx, atoi(x.id.c_str() + 1));
				pp::PerfilAmigo n; n.id = "p" + std::to_string(mx + 1); n.nombre = nombre; n.registro = pp::aHex(reg); n.actualizado = selloPerfiles();
				idFinal = n.id;
				base.datos.perfiles.push_back(std::move(n));
			}
			if (d.contains("parsecId") && d["parsecId"].is_number_unsigned() && !base.vincular(idFinal, d["parsecId"].get<uint32_t>())) throw ErrorAccion("NO_SE_PUDO", "No pude ligar el perfil.");
			if (!base.guardar(selloPerfiles(), err)) throw ErrorAccion("NO_SE_PUDO", err);
			return estadoPerfiles(json{ {"editado", idFinal} });
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

	// Vigilante del Usuario automático: una vez por segundo mira quién está sentado y, si tiene perfil vinculado,
	// se lo pone en el Usuario de su mando (solo en la pantalla de elegir lados, que es cuando existe la lista de mandos).
	// Al levantarse (o cambiar de persona) devuelve lo que había. Si la persona edita su Usuario en PES, el cambio se
	// guarda en SU perfil (no se pisa). Cada paso es un solo Usuario, con relectura (Conexion::escribirUsuario).
	void tickPerfilesAuto(Interno& in) {
		const double ahora = static_cast<double>(GetTickCount64()) / 1000.0;
		if (ahora - ultimoAuto < 1.0) return;
		ultimoAuto = ahora;
		std::unique_lock<std::mutex> cerrojo(mtxPerfiles, std::try_to_lock);
		if (!cerrojo.owns_lock()) return;
		try {
			std::vector<SentadoAuto> sent;
			if (in.ctx.sala != nullptr) {
				for (const AsientoVista& a : in.ctx.sala->asientos(pp::vivo::kMandos))
					if (a.ocupado && a.parsecId != 0) sent.push_back({ a.numero, a.parsecId, a.jugador, false });
			}
			for (const auto& kv : simuladosAuto) {
				sent.erase(std::remove_if(sent.begin(), sent.end(), [&](const SentadoAuto& s) { return s.puesto == kv.first; }), sent.end());
				sent.push_back({ kv.first, kv.second, "(prueba)", true });
			}
			sentadosAuto = sent;

			const bool activo = leerAjustesPerfiles().autoUsuarios;
			const bool pes = pesAbiertoPerfiles();
			if (!pes) {
				// PES se cerró: los originales ya están en Link como perfiles; no hay nada vivo que devolver
				conexionAuto.reset();
				aplicadosAuto.clear();
				return;
			}
			if (!activo && aplicadosAuto.empty()) { conexionAuto.reset(); return; }
			if (activo && sent.empty() && aplicadosAuto.empty()) return;

			if (!conexionAuto) conexionAuto = std::make_unique<pp::vivo::Conexion>();
			std::string err;
			if (!conexionAuto->abierta() && !conexionAuto->abrir(err)) { conexionAuto.reset(); return; }   // PES abierto pero sin pantalla de lados
			int mandos[pp::vivo::kMandos];
			if (!conexionAuto->leerMandos(mandos, err)) { conexionAuto.reset(); return; }

			pp::Base base(carpetaBasePerfiles());
			if (!base.cargar(err)) { logAuto = "Automático: la base de perfiles está dañada (" + err + ")"; return; }

			// 1) devolver lo que había a quien ya no está sentado, o cambió de persona o de Usuario
			for (size_t i = 0; i < aplicadosAuto.size();) {
				AplicadoAuto& a = aplicadosAuto[i];
				const bool sigue = activo && std::any_of(sent.begin(), sent.end(), [&](const SentadoAuto& s) { return s.puesto == a.puesto && s.parsecId == a.parsecId; })
					&& a.puesto >= 1 && a.puesto <= pp::vivo::kMandos && mandos[a.puesto - 1] == a.usuario;
				if (sigue) { i++; continue; }
				std::vector<uint8_t> actual;
				if (!conexionAuto->leerUsuario(a.usuario, actual, err)) { i++; continue; }
				// si cambió (la persona lo editó) y no se levantó por error, ese cambio es suyo: se guarda en su perfil antes de devolver
				if (pp::aHex(actual) != a.escrito) {
					for (auto& pf : base.datos.perfiles) if (pf.id == a.perfilId && pp::aHex(actual) != pf.registro) {
						pf.anterior = pf.registro; pf.registro = pp::aHex(actual); pf.actualizado = selloPerfiles();
						std::string nom; if (nombreDeRegistroValido(actual, nom)) { pf.nombre = nom; pf.sinNombre = false; }
						std::string e2; base.guardar(selloPerfiles(), e2);
						break;
					}
				}
				std::vector<uint8_t> ant;
				if (conexionAuto->escribirUsuario(a.usuario, a.original, ant, err)) {
					logAuto = "Devolví el Usuario " + std::to_string(a.usuario + 1) + " a como estaba (el puesto " + std::to_string(a.puesto) + " quedó libre).";
					aplicadosAuto.erase(aplicadosAuto.begin() + i);
				}
				else { logAuto = "No pude devolver el Usuario " + std::to_string(a.usuario + 1) + ": " + err; i++; }
			}
			if (!activo) return;

			// 2) poner el perfil a quien se acaba de sentar
			for (const SentadoAuto& s : sent) {
				if (s.puesto < 1 || s.puesto > pp::vivo::kMandos) continue;
				const pp::PerfilAmigo* perfil = base.porParsec(s.parsecId);
				const int usuario = mandos[s.puesto - 1];
				if (!perfil || usuario < 0) continue;
				std::vector<uint8_t> nuevo, actual;
				if (!pp::deHex(perfil->registro, nuevo) || nuevo.size() != static_cast<size_t>(pp::kTamUsuario)) continue;
				if (!conexionAuto->leerUsuario(usuario, actual, err)) continue;
				const std::string hexActual = pp::aHex(actual);
				auto ya = std::find_if(aplicadosAuto.begin(), aplicadosAuto.end(), [&](const AplicadoAuto& a) { return a.usuario == usuario; });
				if (hexActual == perfil->registro) { if (ya != aplicadosAuto.end()) ya->escrito = hexActual; continue; }
				if (ya != aplicadosAuto.end()) {
					// el perfil cambió en Link (se editó con la persona sentada) y PES aún tiene la versión que Link puso: se pone la nueva
					if (hexActual == ya->escrito) {
						std::vector<uint8_t> ant2;
						if (conexionAuto->escribirUsuario(usuario, nuevo, ant2, err)) { ya->escrito = perfil->registro; logAuto = "Actualicé el perfil «" + perfil->nombre + "» en el Usuario " + std::to_string(usuario + 1) + "."; }
						continue;
					}
					// ya se le puso y la persona lo está cambiando en PES: se guarda en su perfil, no se vuelve a pisar
					if (hexActual != ya->escrito) {
						for (auto& pf : base.datos.perfiles) if (pf.id == perfil->id) {
							pf.anterior = pf.registro; pf.registro = hexActual; pf.actualizado = selloPerfiles();
							std::string nom; if (nombreDeRegistroValido(actual, nom)) { pf.nombre = nom; pf.sinNombre = false; }
							break;
						}
						std::string e2; base.guardar(selloPerfiles(), e2);
						ya->escrito = hexActual;
						logAuto = "Guardé los cambios que hizo el puesto " + std::to_string(s.puesto) + " en su perfil.";
					}
					continue;
				}
				// primera vez: nada se pierde, lo que había se guarda como perfil si no lo estaba
				const bool guardado = std::any_of(base.datos.perfiles.begin(), base.datos.perfiles.end(), [&](const pp::PerfilAmigo& x) { return x.registro == hexActual; });
				if (!guardado) {
					int mx = 0;
					for (auto& x : base.datos.perfiles) if (x.id.size() > 1 && x.id[0] == 'p') mx = (std::max)(mx, atoi(x.id.c_str() + 1));
					pp::PerfilAmigo n; n.id = "p" + std::to_string(mx + 1); n.registro = hexActual; n.actualizado = selloPerfiles();
					std::string nom; nombreDeRegistroValido(actual, nom);
					n.sinNombre = nom.empty();
					if (nom.empty() || base.porNombre(nom)) nom = "Usuario " + std::to_string(usuario + 1) + " (antes del automático)";
					while (base.porNombre(nom)) nom += "+";
					n.nombre = nom;
					base.datos.perfiles.push_back(n);
					if (!base.guardar(selloPerfiles(), err)) { logAuto = "No pude guardar una copia de lo que había: " + err; continue; }
					perfil = base.porParsec(s.parsecId);   // el vector pudo moverse
					if (!perfil) continue;
				}
				std::vector<uint8_t> anterior;
				if (!conexionAuto->escribirUsuario(usuario, nuevo, anterior, err)) { logAuto = "No pude poner el perfil en el Usuario " + std::to_string(usuario + 1) + ": " + err; continue; }
				AplicadoAuto a; a.puesto = s.puesto; a.usuario = usuario; a.parsecId = s.parsecId; a.perfilId = perfil->id; a.original = anterior; a.escrito = perfil->registro;
				aplicadosAuto.push_back(std::move(a));
				logAuto = "Puse el perfil «" + perfil->nombre + "» en el Usuario " + std::to_string(usuario + 1) + " (puesto " + std::to_string(s.puesto) + ").";
			}
		}
		catch (...) {
			logAuto = "Automático: error inesperado (no se tocó nada más).";
		}
	}
}
