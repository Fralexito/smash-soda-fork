#include "ModulosJuego.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>

#include "BuzonJuego.h"
#include "Entrega.h"

namespace phoenix::modulos {

namespace {
	constexpr uintmax_t kMaxLua = 1024 * 1024;        // ningún módulo pesa más de 1 MB
	constexpr uintmax_t kMaxIni = 512 * 1024;
	constexpr uintmax_t kMaxLog = 8 * 1024 * 1024;

	// Módulos que Link conoce: descripción para la interfaz. Los que no estén aquí se muestran con su nombre.
	const std::map<std::string, std::string> kDescripciones = {
		{ "phoenix_estadio.lua", "Partido ↔ Phoenix Link: marcador, goles con minuto, HUD del partido y árbitro (solo lectura)." },
		{ "phoenix.lua", "Phoenix Sync: avisos de la web en el overlay, botón nativo «Datos Actual. en vivo» y modos de recarga de fichajes. Instala también la raíz Phoenix-DB y la carpeta de avisos." },
	};

	std::string minus(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); }); return s; }

	bool leer(const fs::path& ruta, std::string& out, uintmax_t maximo) {
		std::error_code ec;
		const uintmax_t n = fs::file_size(ruta, ec);
		if (ec || n > maximo) return false;
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		out.assign(static_cast<size_t>(n), '\0');
		if (n > 0) f.read(&out[0], static_cast<std::streamsize>(n));
		return static_cast<uintmax_t>(f.gcount()) == n;
	}

	std::string sha(const std::string& s) { return phoenix::entrega::sha256Hex(s.data(), s.size()); }

	std::string sello() {
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		char b[32];
		std::strftime(b, sizeof(b), "%Y%m%d-%H%M%S", &lt);
		return b;
	}

	/// ¿Es una línea lua.module (activa o comentada con ; o #)? Devuelve el archivo entre comillas.
	bool lineaModulo(const std::string& l, std::string& archivo, bool& comentada) {
		size_t i = 0;
		auto blancos = [&]() { while (i < l.size() && (l[i] == ' ' || l[i] == '\t')) i++; };
		blancos();
		comentada = false;
		while (i < l.size() && (l[i] == ';' || l[i] == '#')) { comentada = true; i++; blancos(); }
		if (l.compare(i, 10, "lua.module") != 0) return false;
		i += 10; blancos();
		if (i >= l.size() || l[i] != '=') return false;
		i++; blancos();
		if (i >= l.size() || l[i] != '"') return false;
		const size_t fin = l.find('"', i + 1);
		if (fin == std::string::npos) return false;
		archivo = l.substr(i + 1, fin - i - 1);
		return true;
	}

	struct Lineas {
		std::vector<std::string> v;   // sin fin de línea
		std::string eol = "\r\n";
		bool eolFinal = true;
	};
	Lineas partir(const std::string& s) {
		Lineas r;
		r.eol = s.find("\r\n") != std::string::npos ? "\r\n" : (s.find('\n') != std::string::npos ? "\n" : "\r\n");
		r.eolFinal = !s.empty() && s.back() == '\n';
		size_t i = 0;
		while (i <= s.size()) {
			const size_t j = s.find('\n', i);
			if (j == std::string::npos) { if (i < s.size()) r.v.push_back(s.substr(i)); break; }
			std::string l = s.substr(i, j - i);
			if (!l.empty() && l.back() == '\r') l.pop_back();
			r.v.push_back(l);
			i = j + 1;
		}
		return r;
	}
	std::string unir(const Lineas& r) {
		std::string s;
		for (size_t i = 0; i < r.v.size(); i++) {
			s += r.v[i];
			if (i + 1 < r.v.size() || r.eolFinal) s += r.eol;
		}
		return s;
	}

	// Modos de un cambiador de parches: <juego>\<carpeta>\<modo>\SiderAddons (limitado, solo lectura).
	std::vector<fs::path> modos(const fs::path& juego) {
		std::vector<fs::path> v;
		std::error_code ec;
		int a = 0;
		for (fs::directory_iterator it(juego, ec), fin; !ec && it != fin && a < 80; it.increment(ec), ++a) {
			std::error_code e2;
			if (!it->is_directory(e2) || it->path().filename() == "SiderAddons") continue;
			int b = 0;
			for (fs::directory_iterator it2(it->path(), e2), fin2; !e2 && it2 != fin2 && b < 80; it2.increment(e2), ++b) {
				std::error_code e3;
				if (it2->is_directory(e3) && fs::is_regular_file(it2->path() / "SiderAddons" / "sider.ini", e3)) v.push_back(it2->path());
			}
		}
		std::sort(v.begin(), v.end());
		return v;
	}

	/// ¿Es una línea cpk.root (activa o comentada)? Devuelve la ruta entre comillas.
	bool lineaRaiz(const std::string& l, std::string& ruta, bool& comentada) {
		size_t i = 0;
		auto blancos = [&]() { while (i < l.size() && (l[i] == ' ' || l[i] == '\t')) i++; };
		blancos();
		comentada = false;
		while (i < l.size() && (l[i] == ';' || l[i] == '#')) { comentada = true; i++; blancos(); }
		if (l.compare(i, 8, "cpk.root") != 0) return false;
		i += 8; blancos();
		if (i >= l.size() || l[i] != '=') return false;
		i++; blancos();
		if (i >= l.size() || l[i] != '"') return false;
		const size_t fin = l.find('"', i + 1);
		if (fin == std::string::npos) return false;
		ruta = l.substr(i + 1, fin - i - 1);
		return true;
	}
	bool esPhoenixDB(const std::string& ruta) {
		std::string m = minus(ruta);
		std::replace(m.begin(), m.end(), '/', '\\');
		return m.size() >= 10 && m.compare(m.size() - 10, 10, "phoenix-db") == 0;
	}

	bool nombreSeguro(const std::string& a) {
		if (a.size() < 5 || a.size() > 64 || minus(a.substr(a.size() - 4)) != ".lua") return false;
		for (unsigned char c : a) if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
		return a.find("..") == std::string::npos;
	}
}

// ─── lógica pura de sider.ini ──────────────────────────────────────────────────────────────────────

void buscarLinea(const std::string& ini, const std::string& archivo, bool& activa, bool& comentada) {
	activa = comentada = false;
	const std::string buscado = minus(archivo);
	for (const std::string& l : partir(ini).v) {
		std::string a; bool c = false;
		if (lineaModulo(l, a, c) && minus(a) == buscado) { if (c) comentada = true; else activa = true; }
	}
}

std::string activarLinea(const std::string& ini, const std::string& archivo, bool& cambio, std::string& error) {
	cambio = false;
	Lineas r = partir(ini);
	const std::string buscado = minus(archivo);
	int ultimaActiva = -1, ultimaCualquiera = -1, comentada = -1;
	for (size_t i = 0; i < r.v.size(); i++) {
		std::string a; bool c = false;
		if (!lineaModulo(r.v[i], a, c)) continue;
		if (minus(a) == buscado) {
			if (!c) return ini;                       // ya está activa: nada que hacer
			if (comentada < 0) comentada = static_cast<int>(i);
		}
		ultimaCualquiera = static_cast<int>(i);
		if (!c) ultimaActiva = static_cast<int>(i);
	}
	const std::string nueva = "lua.module = \"" + archivo + "\"";
	if (comentada >= 0) r.v[static_cast<size_t>(comentada)] = nueva;
	else {
		const int tras = ultimaActiva >= 0 ? ultimaActiva : ultimaCualquiera;
		if (tras < 0) { error = "sider.ini no tiene lista de módulos (lua.module): no se toca"; return std::string(); }
		r.v.insert(r.v.begin() + tras + 1, nueva);
	}
	cambio = true;
	return unir(r);
}

std::string comentarLinea(const std::string& ini, const std::string& archivo, bool& cambio) {
	cambio = false;
	Lineas r = partir(ini);
	const std::string buscado = minus(archivo);
	for (std::string& l : r.v) {
		std::string a; bool c = false;
		if (lineaModulo(l, a, c) && !c && minus(a) == buscado) { l = ";" + l; cambio = true; }
	}
	return cambio ? unir(r) : ini;
}

bool tieneRaizPhoenixDB(const std::string& ini) {
	for (const std::string& l : partir(ini).v) {
		std::string ruta; bool c = false;
		if (lineaRaiz(l, ruta, c) && !c && esPhoenixDB(ruta)) return true;
	}
	return false;
}

std::string activarRaizPhoenixDB(const std::string& ini, const std::function<bool(const std::string&)>& tieneBase, bool& cambio, std::string& error) {
	cambio = false;
	Lineas r = partir(ini);
	int primera = -1, primeraBase = -1, comentadaPhx = -1;
	for (size_t i = 0; i < r.v.size(); i++) {
		std::string ruta; bool c = false;
		if (!lineaRaiz(r.v[i], ruta, c)) continue;
		if (esPhoenixDB(ruta)) {
			if (!c) return ini;                            // ya está activa
			if (comentadaPhx < 0) comentadaPhx = static_cast<int>(i);
			continue;
		}
		if (c) continue;
		if (primera < 0) primera = static_cast<int>(i);
		if (primeraBase < 0 && tieneBase && tieneBase(ruta)) primeraBase = static_cast<int>(i);
	}
	int donde = primeraBase >= 0 ? primeraBase : primera;
	if (donde < 0) { error = "sider.ini no tiene raíces cpk.root: no se toca"; return std::string(); }
	if (comentadaPhx >= 0) {                                // se quita la comentada para no dejar dos
		r.v.erase(r.v.begin() + comentadaPhx);
		if (comentadaPhx < donde) donde--;
	}
	r.v.insert(r.v.begin() + donde, "cpk.root = \".\\livecpk\\Phoenix-DB\"");
	cambio = true;
	return unir(r);
}

std::string versionDe(const std::string& lua) {
	const size_t i = lua.find("version");
	if (i == std::string::npos || i > 4096) return "";
	size_t j = lua.find('"', i);
	if (j == std::string::npos || j - i > 20) return "";
	const size_t k = lua.find('"', j + 1);
	if (k == std::string::npos || k - j > 24) return "";
	return lua.substr(j + 1, k - j - 1);
}

// ─── carpetas ────────────────────────────────────────────────────────────────────────────────────

fs::path carpetaPaquete() {
#ifdef _WIN32
	wchar_t ruta[MAX_PATH * 2];
	const DWORD n = GetModuleFileNameW(nullptr, ruta, static_cast<DWORD>(sizeof(ruta) / sizeof(ruta[0])));
	if (n == 0 || n >= sizeof(ruta) / sizeof(ruta[0])) return {};
	return fs::path(std::wstring(ruta, n)).parent_path() / L"sider";
#else
	return fs::path("sider");
#endif
}

std::vector<std::pair<std::string, fs::path>> destinos(const fs::path& juego) {
	std::vector<std::pair<std::string, fs::path>> v;
	if (juego.empty()) return v;
	std::error_code ec;
	if (fs::is_regular_file(juego / "SiderAddons" / "sider.ini", ec)) v.push_back({ "Raíz del juego", juego / "SiderAddons" });
	for (const fs::path& m : modos(juego)) v.push_back({ phoenix::entrega::aU8(m.filename()), m / "SiderAddons" });
	return v;
}

// ─── estado ──────────────────────────────────────────────────────────────────────────────────────

std::vector<Modulo> estado(const fs::path& paquete, const fs::path& juego) {
	std::vector<Modulo> lista;
	std::error_code ec;
	// Los que lleva Link
	std::map<std::string, std::string> fuente;   // archivo → contenido
	for (fs::directory_iterator it(paquete, ec), fin; !ec && it != fin; it.increment(ec)) {
		const std::string a = phoenix::entrega::aU8(it->path().filename());
		if (!nombreSeguro(a)) continue;
		std::string s;
		if (leer(it->path(), s, kMaxLua)) fuente[a] = s;
	}
	std::vector<std::string> archivos;
	for (const auto& f : fuente) archivos.push_back(f.first);
	if (!fuente.count("phoenix.lua")) archivos.push_back("phoenix.lua");   // sin paquete: solo se muestra

	const auto ds = destinos(juego);
	std::map<fs::path, std::string> inis;
	for (const auto& d : ds) { std::string s; if (leer(d.second / "sider.ini", s, kMaxIni)) inis[d.second] = s; }
	std::string log;
	if (!ds.empty()) leer(ds.front().second / "sider.log", log, kMaxLog);

	for (const std::string& a : archivos) {
		Modulo m;
		m.archivo = a;
		auto it = kDescripciones.find(a);
		m.descripcion = it != kDescripciones.end() ? it->second : a;
		m.gestionable = fuente.count(a) > 0;
		const std::string shaFuente = m.gestionable ? sha(fuente[a]) : std::string();
		if (m.gestionable) m.version = versionDe(fuente[a]);
		int conArchivo = 0, iguales = 0, conLinea = 0, comentadas = 0;
		for (const auto& d : ds) {
			Destino x;
			x.nombre = d.first;
			x.carpeta = phoenix::entrega::aU8(d.second);
			std::string s;
			if (leer(d.second / "modules" / a, s, kMaxLua)) {
				x.archivo = true;
				x.version = versionDe(s);
				x.igual = m.gestionable && sha(s) == shaFuente;
			}
			if (inis.count(d.second)) buscarLinea(inis[d.second], a, x.linea, x.comentada);
			conArchivo += x.archivo; iguales += x.igual; conLinea += x.linea; comentadas += (!x.linea && x.comentada);
			m.destinos.push_back(x);
		}
		const int n = static_cast<int>(ds.size());
		if (!m.gestionable) m.estado = (conArchivo || conLinea) ? "solo_lectura" : "no_instalado";
		else if (n == 0 || (conArchivo == 0 && conLinea == 0)) m.estado = "no_instalado";
		else if (iguales == n && conLinea == n) m.estado = "instalado";
		else if (conArchivo == n && conLinea == 0 && comentadas > 0) m.estado = "apagado";
		else if (conLinea == n && conArchivo == n) m.estado = "desactualizado";
		else m.estado = "a_medias";
		if (a == "phoenix.lua" && m.gestionable && m.estado == "instalado")   // sin su raíz Phoenix-DB no está completo
			for (const auto& d : ds) if (!inis.count(d.second) || !tieneRaizPhoenixDB(inis[d.second])) { m.estado = "a_medias"; break; }
		if (!log.empty()) {
			if (log.find("Module (" + a + ") is NOT activated") != std::string::npos) m.carga = "error";
			else if (log.find("[" + a + "]") != std::string::npos) m.carga = "cargado";
		}
		if (m.gestionable || m.estado != "no_instalado") lista.push_back(m);
	}
	return lista;
}

// ─── instalar / quitar ───────────────────────────────────────────────────────────────────────────

namespace {
	/// Escribe los sider.ini nuevos con copia previa. Si uno falla, devuelve los ya escritos a su copia.
	bool escribirInis(const std::vector<std::pair<fs::path, std::string>>& cambios, Resultado& r) {
		const std::string s = sello();
		std::vector<std::pair<fs::path, fs::path>> hechos;   // carpeta, respaldo
		for (const auto& c : cambios) {
			std::error_code ec;
			const fs::path ini = c.first / "sider.ini";
			const fs::path resp = c.first / ("sider.ini.phoenix-" + s);
			if (!fs::copy_file(ini, resp, fs::copy_options::overwrite_existing, ec)) {
				r.mensaje = "No se pudo guardar la copia de sider.ini en " + phoenix::entrega::aU8(c.first);
			}
			else if (!phoenix::buzon::escribirAtomicoComo(c.first, "sider", ".ini", c.second)) {
				r.mensaje = "No se pudo escribir sider.ini en " + phoenix::entrega::aU8(c.first) + " (¿PES abierto?)";
			}
			else {
				hechos.push_back({ c.first, resp });
				r.respaldos.push_back(phoenix::entrega::aU8(resp));
				continue;
			}
			// Vuelta atrás de lo ya escrito
			for (const auto& h : hechos) {
				std::string viejo;
				if (leer(h.second, viejo, kMaxIni)) phoenix::buzon::escribirAtomicoComo(h.first, "sider", ".ini", viejo);
			}
			r.mensaje += ". Se dejó todo como estaba.";
			return false;
		}
		return true;
	}
}

Resultado instalar(const fs::path& paquete, const fs::path& juego, const std::string& archivo) {
	Resultado r;
	if (!nombreSeguro(archivo)) { r.mensaje = "Nombre de módulo no válido."; return r; }
	std::string lua;
	if (!leer(paquete / archivo, lua, kMaxLua)) { r.mensaje = "Phoenix Link no lleva ese módulo (" + archivo + ")."; return r; }
	const auto ds = destinos(juego);
	if (ds.empty()) { r.mensaje = "No encuentro SiderAddons\\sider.ini en la carpeta del juego. Abre PES una vez para que Link la conozca."; return r; }

	// 1) Comprobar TODO antes de escribir nada
	std::vector<std::pair<fs::path, std::string>> inisNuevos;
	for (const auto& d : ds) {
		std::error_code ec;
		if (!fs::is_directory(d.second / "modules", ec)) { r.mensaje = "Falta la carpeta modules en " + d.first + ": no se tocó nada."; return r; }
		std::string ini, error;
		if (!leer(d.second / "sider.ini", ini, kMaxIni)) { r.mensaje = "No se pudo leer sider.ini en " + d.first + ": no se tocó nada."; return r; }
		bool cambio = false;
		std::string nuevo = activarLinea(ini, archivo, cambio, error);
		if (!error.empty()) { r.mensaje = error + " (" + d.first + ")."; return r; }
		if (!cambio) nuevo = ini;
		if (archivo == "phoenix.lua") {
			bool cambioRaiz = false;
			const fs::path sider = d.second;
			const auto tieneBase = [&](const std::string& ruta) {
				std::error_code e2;
				return fs::is_regular_file(sider / phoenix::entrega::deU8(ruta) / "common" / "etc" / "pesdb" / "Player.bin", e2);
			};
			const std::string conRaiz = activarRaizPhoenixDB(nuevo, tieneBase, cambioRaiz, error);
			if (!error.empty()) { r.mensaje = error + " (" + d.first + ")."; return r; }
			if (cambioRaiz) { nuevo = conRaiz; cambio = true; }
		}
		if (cambio) inisNuevos.push_back({ d.second, nuevo });
	}

	// 1b) phoenix.lua: carpetas del buzón y de Phoenix-DB (vacías; Link escribe los avisos y Sync deja los datos)
	if (archivo == "phoenix.lua") {
		for (const auto& d : ds) {
			std::error_code ec;
			fs::create_directories(d.second / "content" / "phoenix", ec);
			if (ec) { r.mensaje = "No se pudo crear content\\phoenix en " + d.first + "."; return r; }
			fs::create_directories(d.second / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb", ec);
			if (ec) { r.mensaje = "No se pudo crear livecpk\\Phoenix-DB en " + d.first + "."; return r; }
		}
	}

	// 2) Copiar el módulo a cada destino y comprobar el sha256 (sin línea en sider.ini todavía no hace nada)
	const fs::path nombre(archivo);
	const std::string stem = phoenix::entrega::aU8(nombre.stem());
	const std::string shaLua = sha(lua);
	for (const auto& d : ds) {
		const fs::path mod = d.second / "modules";
		std::string actual;
		if (leer(mod / archivo, actual, kMaxLua) && sha(actual) == shaLua) continue;
		if (!actual.empty()) {   // había otra versión: se guarda al lado antes de reemplazarla
			const std::string ver = versionDe(actual);
			phoenix::buzon::escribirAtomicoComo(mod, stem + ".lua.antes-" + (ver.empty() ? sello() : "v" + ver), "", actual);
		}
		if (!phoenix::buzon::escribirAtomicoComo(mod, stem, ".lua", lua)) { r.mensaje = "No se pudo copiar " + archivo + " en " + d.first + "."; return r; }
		if (!leer(mod / archivo, actual, kMaxLua) || sha(actual) != shaLua) { r.mensaje = "La copia de " + archivo + " en " + d.first + " no quedó idéntica."; return r; }
	}

	// 3) Una línea en cada sider.ini (con copia previa y vuelta atrás si algo falla)
	if (!escribirInis(inisNuevos, r)) return r;
	r.ok = true;
	r.mensaje = archivo + " instalado en " + std::to_string(ds.size()) + (ds.size() == 1 ? " carpeta" : " carpetas")
		+ ". Abre PES: en sider.log debe aparecer que cargó.";
	return r;
}

Resultado quitar(const fs::path& paquete, const fs::path& juego, const std::string& archivo) {
	Resultado r;
	std::error_code ec;
	if (!nombreSeguro(archivo) || !fs::is_regular_file(paquete / archivo, ec)) { r.mensaje = "Phoenix Link solo quita los módulos que él instala."; return r; }
	const auto ds = destinos(juego);
	std::vector<std::pair<fs::path, std::string>> inisNuevos;
	for (const auto& d : ds) {
		std::string ini;
		if (!leer(d.second / "sider.ini", ini, kMaxIni)) { r.mensaje = "No se pudo leer sider.ini en " + d.first + ": no se tocó nada."; return r; }
		bool cambio = false;
		const std::string nuevo = comentarLinea(ini, archivo, cambio);
		if (cambio) inisNuevos.push_back({ d.second, nuevo });
	}
	if (inisNuevos.empty()) { r.ok = true; r.mensaje = archivo + " ya estaba apagado."; return r; }
	if (!escribirInis(inisNuevos, r)) return r;
	r.ok = true;
	r.mensaje = archivo + " apagado (línea comentada en sider.ini; el archivo se queda por si lo vuelves a activar).";
	return r;
}

} // namespace phoenix::modulos
