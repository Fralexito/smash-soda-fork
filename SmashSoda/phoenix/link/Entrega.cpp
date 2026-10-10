#include "Entrega.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <thread>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using json = nlohmann::json;

namespace phoenix::entrega {

// ---------------------------------------------------------------- utilidades
std::string aU8(const fs::path& p) {
#if defined(__cpp_char8_t)
	const std::u8string s = p.u8string();
	return std::string(s.begin(), s.end());
#else
	return p.u8string();
#endif
}

fs::path deU8(const std::string& s) {
#if defined(__cpp_char8_t)
	return fs::path(std::u8string(s.begin(), s.end()));
#else
	return fs::u8path(s);
#endif
}

namespace {

const char* kNombrePlayer = "Player.bin";
const char* kNombreAsignacion = "PlayerAssignment.bin";   // plantillas de la base: las usa «Datos Actual. en vivo → Activar»
const char* kNombreEdit = "EDIT00000000";
const char* kRelPesdb = "livecpk/Phoenix-DB/common/etc/pesdb";
// Carpetas de Documentos donde PES 2021 guarda el option file (varian segun la edicion/parche).
const char* kBasesOptionFile[] = { "eFootball PES 2021 SEASON UPDATE", "eFootball PES 2021", "PRO EVOLUTION SOCCER 2021" };

// Modos de un cambiador de parches: <juego>\<carpeta>\<modo>\SiderAddons (limitado, solo lectura).
std::vector<fs::path> modosConSider(const fs::path& juego) {
	std::vector<fs::path> v;
	std::error_code ec;
	int a = 0;
	for (fs::directory_iterator it(juego, ec), fin; !ec && it != fin && a < 80; it.increment(ec), ++a) {
		std::error_code e2;
		if (!it->is_directory(e2) || it->path().filename() == "SiderAddons") continue;
		int b = 0;
		for (fs::directory_iterator it2(it->path(), e2), fin2; !e2 && it2 != fin2 && b < 80; it2.increment(e2), ++b) {
			std::error_code e3;
			if (it2->is_directory(e3) && fs::is_directory(it2->path() / "SiderAddons", e3)) v.push_back(it2->path());
		}
	}
	std::sort(v.begin(), v.end());
	return v;
}

std::string ahora(const char* fmt) {
	std::time_t t = std::time(nullptr);
	std::tm lt{};
#ifdef _WIN32
	localtime_s(&lt, &t);
#else
	localtime_r(&t, &lt);
#endif
	char b[40];
	std::strftime(b, sizeof b, fmt, &lt);
	return b;
}

void dormir(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// tmp -> dst reemplazando; si esta en uso reintenta 10 veces cada 200 ms.
bool moverReemplazando(const fs::path& tmp, const fs::path& dst, std::string& error) {
	for (int i = 0; i < 10; ++i) {
#ifdef _WIN32
		if (MoveFileExW(tmp.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#else
		std::error_code ec;
		fs::rename(tmp, dst, ec);
		if (!ec) return true;
#endif
		if (i < 9) dormir(200);
	}
	error = "el archivo esta en uso";
	return false;
}

// Copia src a dst: primero dst.phoenix-tmp en la misma carpeta, luego reemplazo atomico.
bool copiarAtomico(const fs::path& src, const fs::path& dst, std::string& error) {
	fs::path tmp = dst;
	tmp += ".phoenix-tmp";
	std::error_code ec;
	fs::copy_file(src, tmp, fs::copy_options::overwrite_existing, ec);
	if (ec) { error = "no se pudo copiar (" + ec.message() + ")"; fs::remove(tmp, ec); return false; }
	if (!moverReemplazando(tmp, dst, error)) { fs::remove(tmp, ec); return false; }
	return true;
}

bool leerTodo(const fs::path& ruta, std::string& out) {
	std::ifstream f(ruta, std::ios::binary);
	if (!f) return false;
	out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return true;
}

bool escribirTexto(const fs::path& ruta, const std::string& texto) {
	fs::path tmp = ruta;
	tmp += ".tmp";
	{
		std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
		if (!f) return false;
		f.write(texto.data(), static_cast<std::streamsize>(texto.size()));
		if (!f) return false;
	}
	std::string e;
	std::error_code ec;
	if (!moverReemplazando(tmp, ruta, e)) { fs::remove(tmp, ec); return false; }
	return true;
}

std::string minusculas(std::string s) {
	for (auto& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
	return s;
}

bool idValido(const std::string& id) {
	if (id.empty() || id.size() > 64) return false;
	for (unsigned char c : id) {
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_')) return false;
	}
	return true;
}

// ultima_link.json = lo que se muestra (cualquier estado). deshacer_link.json = solo la ultima entrega colocada
// (un rechazo posterior no borra la posibilidad de deshacer).
fs::path archivoEstado(const fs::path& carpeta) { return carpeta / "ultima_link.json"; }
fs::path archivoDeshacer(const fs::path& carpeta) { return carpeta / "deshacer_link.json"; }

void guardarUltima(const fs::path& carpeta, const Ultima& u, bool paraDeshacer = false) {
	json j = { {"estado", u.estado}, {"id", u.id}, {"resumen", u.resumen}, {"motivo", u.motivo}, {"fecha", u.fecha},
		{"puedeDeshacer", u.puedeDeshacer}, {"player", u.player}, {"edit", u.edit}, {"editRespaldo", u.editRespaldo} };
	escribirTexto(paraDeshacer ? archivoDeshacer(carpeta) : archivoEstado(carpeta), j.dump(2));
}

void historial(const fs::path& carpeta, const Ultima& u, const std::string& detalle) {
	std::ofstream f(carpeta / "historial.log", std::ios::app | std::ios::binary);
	if (!f) return;
	std::string linea = ahora("%Y-%m-%d %H:%M:%S") + " | Link | " + u.estado + " | " + u.id + " | " + u.resumen;
	if (!u.motivo.empty()) linea += " | " + u.motivo;
	if (!detalle.empty()) linea += " | " + detalle;
	for (auto& c : linea) if (c == '\n' || c == '\r') c = ' ';
	f << linea << "\n";
}

// Mueve entrega.json y los archivos de entrada a <carpeta>/<subcarpeta>/<id>/ (para no repetir la entrega).
void archivar(const fs::path& carpeta, const char* subcarpeta, const std::string& id, const Pedido& p) {
	std::error_code ec;
	std::string nombre = idValido(id) ? id : ("sin-id-" + ahora("%Y%m%d-%H%M%S"));
	fs::path dest = carpeta / subcarpeta / nombre;
	for (int n = 2; fs::exists(dest, ec); ++n) dest = carpeta / subcarpeta / (nombre + "-" + std::to_string(n));
	fs::create_directories(dest, ec);
	auto mover = [&](const fs::path& de) {
		if (!fs::exists(de, ec)) return;
		fs::rename(de, dest / de.filename(), ec);
		if (ec) { ec.clear(); fs::copy_file(de, dest / de.filename(), fs::copy_options::overwrite_existing, ec); fs::remove(de, ec); }
	};
	mover(carpeta / "entrega.json");
	mover(carpeta / kNombrePlayer);
	mover(carpeta / kNombreAsignacion);
	mover(carpeta / kNombreEdit);
	(void)p;
}

Ultima rechazar(const Rutas& r, Ultima u, const Pedido& p, const std::string& motivo) {
	u.estado = "rechazada";
	u.motivo = motivo;
	u.puedeDeshacer = false;
	u.fecha = ahora("%Y-%m-%d %H:%M");
	u.aviso = avisoRechazada(motivo);
	guardarUltima(r.carpetaEntrega, u);
	historial(r.carpetaEntrega, u, "");
	archivar(r.carpetaEntrega, "rechazadas", p.id, p);
	return u;
}

void podarRespaldos(const fs::path& carpeta, size_t quedan) {
	std::vector<fs::path> v;
	std::error_code ec;
	for (const auto& e : fs::directory_iterator(carpeta, ec)) {
		const std::string n = aU8(e.path().filename());
		if (n.rfind("EDIT00000000.phoenix-", 0) == 0) v.push_back(e.path());
	}
	std::sort(v.begin(), v.end(), [](const fs::path& a, const fs::path& b) { return a.filename() > b.filename(); });
	for (size_t i = quedan; i < v.size(); ++i) fs::remove(v[i], ec);
}

} // namespace

// -------------------------------------------------------------------- SHA-256
namespace {
struct Sha256 {
	uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	uint8_t buf[64];
	size_t nbuf = 0;
	uint64_t total = 0;
	static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
	void bloque(const uint8_t* p) {
		static const uint32_t K[64] = {
			0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
			0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
			0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
			0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
		uint32_t w[64];
		for (int i = 0; i < 16; ++i) w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) | (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
		for (int i = 16; i < 64; ++i) {
			const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
			const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}
		uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
		for (int i = 0; i < 64; ++i) {
			const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
			const uint32_t ch = (e & f) ^ (~e & g);
			const uint32_t t1 = hh + S1 + ch + K[i] + w[i];
			const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
			const uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
			const uint32_t t2 = S0 + mj;
			hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
	}
	void poner(const void* datos, size_t n) {
		const uint8_t* p = static_cast<const uint8_t*>(datos);
		total += n;
		while (n > 0) {
			const size_t k = (std::min)(n, 64 - nbuf);
			std::memcpy(buf + nbuf, p, k);
			nbuf += k; p += k; n -= k;
			if (nbuf == 64) { bloque(buf); nbuf = 0; }
		}
	}
	std::string fin() {
		const uint64_t bits = total * 8;
		const uint8_t uno = 0x80, cero = 0;
		poner(&uno, 1);
		while (nbuf != 56) poner(&cero, 1);
		uint8_t l[8];
		for (int i = 0; i < 8; ++i) l[i] = static_cast<uint8_t>(bits >> (56 - 8 * i));
		poner(l, 8);
		static const char* hx = "0123456789abcdef";
		std::string s;
		for (int i = 0; i < 8; ++i) for (int k = 28; k >= 0; k -= 4) s += hx[(h[i] >> k) & 15];
		return s;
	}
};
} // namespace

std::string sha256Hex(const void* datos, size_t n) {
	Sha256 s;
	s.poner(datos, n);
	return s.fin();
}

bool sha256Archivo(const fs::path& ruta, std::string& hex) {
	std::ifstream f(ruta, std::ios::binary);
	if (!f) return false;
	Sha256 s;
	char b[65536];
	while (f) {
		f.read(b, sizeof b);
		const std::streamsize n = f.gcount();
		if (n > 0) s.poner(b, static_cast<size_t>(n));
	}
	hex = s.fin();
	return true;
}

// -------------------------------------------------------------------- pedido
bool leerPedido(const fs::path& entregaJson, Pedido& p, std::string& error) {
	std::string texto;
	if (!leerTodo(entregaJson, texto)) { error = "no se pudo abrir"; return false; }
	if (texto.size() >= 3 && (unsigned char)texto[0] == 0xEF && (unsigned char)texto[1] == 0xBB && (unsigned char)texto[2] == 0xBF) texto.erase(0, 3);
	const json j = json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) { error = "no es un JSON valido"; return false; }
	p = Pedido{};
	p.version = j.value("version", 0);
	p.id = j.contains("id") && j["id"].is_string() ? j["id"].get<std::string>() : std::string();
	p.creadoEn = j.contains("creado_en") && j["creado_en"].is_string() ? j["creado_en"].get<std::string>() : std::string();
	p.resumen = j.contains("resumen") && j["resumen"].is_string() ? j["resumen"].get<std::string>() : std::string();
	if (j.contains("archivos") && j["archivos"].is_array()) {
		for (const json& a : j["archivos"]) {
			if (!a.is_object()) continue;
			Archivo x;
			x.nombre = a.contains("nombre") && a["nombre"].is_string() ? a["nombre"].get<std::string>() : std::string();
			x.sha256 = a.contains("sha256") && a["sha256"].is_string() ? a["sha256"].get<std::string>() : std::string();
			p.archivos.push_back(x);
		}
	}
	return true;
}

bool hayEntrega(const fs::path& carpetaEntrega) {
	std::error_code ec;
	return fs::is_regular_file(carpetaEntrega / "entrega.json", ec);
}

namespace {
Ultima leerArchivoUltima(const fs::path& archivo) {
	Ultima u;
	std::string texto;
	if (!leerTodo(archivo, texto)) return u;
	const json j = json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) return u;
	auto s = [&](const char* k) { return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); };
	u.estado = s("estado"); u.id = s("id"); u.resumen = s("resumen"); u.motivo = s("motivo"); u.fecha = s("fecha");
	u.edit = s("edit"); u.editRespaldo = s("editRespaldo");
	u.puedeDeshacer = j.value("puedeDeshacer", false);
	if (j.contains("player") && j["player"].is_array()) for (const json& x : j["player"]) if (x.is_string()) u.player.push_back(x.get<std::string>());
	return u;
}
}

Ultima leerUltima(const fs::path& carpetaEntrega) {
	Ultima u = leerArchivoUltima(archivoEstado(carpetaEntrega));
	const Ultima d = leerArchivoUltima(archivoDeshacer(carpetaEntrega));
	u.puedeDeshacer = d.estado == "colocada" && d.puedeDeshacer;
	return u;
}

// ------------------------------------------------------------------- avisos
namespace {
std::string recortar(const std::string& s, size_t max) {
	if (s.size() <= max) return s;
	size_t n = max;
	while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;
	return s.substr(0, n);
}
std::string plano(std::string s) {
	for (auto& c : s) if (c == '\n' || c == '\r' || c == '\t') c = ' ';
	return s;
}
}

std::string avisoColocada(const std::string& resumen) {
	// El buzon corta cada linea a 110 bytes ("[hh:mm] " ya ocupa 8): el resumen cede para que la instruccion quede entera.
	const std::string cola = ". Partido \xE2\x86\x92 Datos Actual. en vivo \xE2\x86\x92 Activar";
	const std::string cab = "\xE2\x9A\xA1 Datos nuevos de Phoenix: ";
	const size_t libre = 105 > cab.size() + cola.size() ? 105 - cab.size() - cola.size() : 0;
	return cab + recortar(plano(resumen), libre) + cola;
}
std::string avisoRechazada(const std::string& motivo) {
	return std::string("\xE2\x9A\xA0 entrega rechazada: ") + plano(motivo);
}
std::string avisoDeshecha() {
	return "\xE2\x86\xA9 Entrega deshecha. Partido \xE2\x86\x92 Datos Actual. en vivo \xE2\x86\x92 Activar";
}

// ---------------------------------------------------------------- option file
fs::path buscarCarpetaOptionFile(const fs::path& documentos) {
	std::error_code ec;
	fs::path mejor;
	fs::file_time_type tMejor{};
	for (const char* nombre : kBasesOptionFile) {
		const fs::path base = documentos / "KONAMI" / nombre;
		if (!fs::is_directory(base, ec)) continue;
		for (const auto& e : fs::directory_iterator(base, ec)) {
			if (!e.is_directory(ec)) continue;
			const fs::path f = e.path() / "save" / kNombreEdit;
			if (!fs::is_regular_file(f, ec)) continue;
			const auto t = fs::last_write_time(f, ec);
			if (mejor.empty() || t > tMejor) { mejor = e.path() / "save"; tMejor = t; }
		}
	}
	return mejor;
}

std::vector<fs::path> buscarCarpetasPesdb(const fs::path& juego, std::string* omitidos) {
	std::vector<fs::path> v;
	std::error_code ec;
	const fs::path principal = juego / "SiderAddons" / kRelPesdb;
	if (fs::is_directory(principal, ec)) v.push_back(principal);
	for (const fs::path& modo : modosConSider(juego)) {
		const fs::path d = modo / "SiderAddons" / kRelPesdb;
		if (fs::is_directory(d, ec)) v.push_back(d);
		else if (omitidos) *omitidos += "sin Phoenix-DB en " + aU8(modo.filename()) + "; ";
	}
	return v;
}

std::string nombreParche(const fs::path& juego) {
	const std::vector<fs::path> m = modosConSider(juego);
	if (!m.empty()) return aU8(m.front().filename());
	return aU8(juego.filename());
}

// -------------------------------------------------------------------- entregar
Ultima entregar(const Rutas& r) {
	Ultima u;
	Pedido p;
	try {
		std::error_code ec;
		std::string err;
		if (!leerPedido(r.carpetaEntrega / "entrega.json", p, err)) return rechazar(r, u, p, "entrega.json ilegible: " + err);
		u.id = p.id;
		u.resumen = p.resumen;
		if (p.version != 1) return rechazar(r, u, p, "version de entrega no soportada");
		if (!idValido(p.id)) return rechazar(r, u, p, "id de entrega invalido");
		if (p.archivos.empty()) return rechazar(r, u, p, "la entrega no trae archivos");
		if (r.carpetaJuego.empty() || !fs::is_directory(r.carpetaJuego, ec)) return rechazar(r, u, p, "no encuentro la carpeta del juego");

		bool conPlayer = false, conAsig = false, conEdit = false;
		fs::path srcPlayer, srcAsig, srcEdit;
		for (const Archivo& a : p.archivos) {
			const bool esPlayer = a.nombre == kNombrePlayer;
			const bool esAsig = a.nombre == kNombreAsignacion;
			const bool esEdit = a.nombre == kNombreEdit;
			if (!esPlayer && !esAsig && !esEdit) return rechazar(r, u, p, "archivo no permitido: " + recortar(a.nombre, 40));
			if ((esPlayer && conPlayer) || (esAsig && conAsig) || (esEdit && conEdit)) return rechazar(r, u, p, "archivo repetido: " + a.nombre);
			const fs::path src = r.carpetaEntrega / a.nombre;
			if (!fs::is_regular_file(src, ec)) return rechazar(r, u, p, "falta " + a.nombre);
			std::string hex;
			if (!sha256Archivo(src, hex)) return rechazar(r, u, p, "no se pudo leer " + a.nombre);
			if (hex != minusculas(a.sha256)) return rechazar(r, u, p, "sha256 no coincide en " + a.nombre);
			if (esPlayer || esAsig) {
				// Archivos de la base (pesdb): misma envoltura WESYS + zlib
				if (esPlayer) { conPlayer = true; srcPlayer = src; }
				else { conAsig = true; srcAsig = src; }
				std::ifstream f(src, std::ios::binary);
				char cab[16] = {};
				f.read(cab, 16);
				const std::string c(cab, static_cast<size_t>(f.gcount()));
				if (f.gcount() < 16 || c.find("WESYS") == std::string::npos) return rechazar(r, u, p, a.nombre + " no tiene la cabecera WESYS");
			}
			else {
				conEdit = true; srcEdit = src;
			}
		}

		// Destinos (nada se crea: si no estan, se rechaza)
		std::vector<fs::path> carpetasPlayer;
		std::string omitidos;
		if (conPlayer || conAsig) {
			carpetasPlayer = buscarCarpetasPesdb(r.carpetaJuego, &omitidos);
			if (carpetasPlayer.empty()) return rechazar(r, u, p, "Phoenix-DB no instalado");
		}
		fs::path destEdit;
		if (conEdit) {
			destEdit = r.carpetaOptionFile / kNombreEdit;
			if (r.carpetaOptionFile.empty() || !fs::is_regular_file(destEdit, ec)) return rechazar(r, u, p, "no encuentro el option file del juego");
			if (fs::file_size(srcEdit, ec) != fs::file_size(destEdit, ec)) return rechazar(r, u, p, "EDIT00000000 no mide lo mismo que el actual");
		}

		// Colocar (con vuelta atras si algo falla a medias)
		struct Puesto { fs::path dst, anterior; bool habia; };
		std::vector<Puesto> puestos;
		auto volverAtras = [&]() {
			std::string e;
			for (const Puesto& q : puestos) {
				if (q.habia) copiarAtomico(q.anterior, q.dst, e);
				else fs::remove(q.dst, ec);
			}
		};
		// Player.bin y/o PlayerAssignment.bin en cada carpeta Phoenix-DB (todo o nada)
		std::vector<std::pair<const char*, fs::path>> deLaBase;
		if (conPlayer) deLaBase.push_back({ kNombrePlayer, srcPlayer });
		if (conAsig) deLaBase.push_back({ kNombreAsignacion, srcAsig });
		for (const fs::path& dir : carpetasPlayer) {
			for (const auto& b : deLaBase) {
				const std::string nombre = b.first;
				Puesto q{ dir / nombre, dir / (nombre + ".anterior"), false };
				q.habia = fs::is_regular_file(q.dst, ec);
				std::string e;
				if (q.habia && !copiarAtomico(q.dst, q.anterior, e)) { volverAtras(); return rechazar(r, u, p, "no se pudo guardar " + nombre + ".anterior: " + e); }
				if (!q.habia && fs::is_regular_file(q.anterior, ec)) {
					// Un .anterior viejo de otra entrega no debe volver al deshacer: se aparta (no se borra)
					fs::rename(q.anterior, dir / (nombre + ".anterior.viejo"), ec);
					ec.clear();
				}
				if (!copiarAtomico(b.second, q.dst, e)) { volverAtras(); return rechazar(r, u, p, "no se pudo colocar " + nombre + ": " + e); }
				puestos.push_back(q);
				u.player.push_back(aU8(q.dst));   // «player» = archivos de la base colocados (Player.bin y PlayerAssignment.bin)
			}
		}
		if (conEdit) {
			std::string e;
			fs::path respaldo = r.carpetaOptionFile / (std::string(kNombreEdit) + ".phoenix-" + ahora("%Y%m%d-%H%M%S"));
			for (int n = 2; fs::exists(respaldo, ec); ++n)
				respaldo = r.carpetaOptionFile / (std::string(kNombreEdit) + ".phoenix-" + ahora("%Y%m%d-%H%M%S") + "-" + std::to_string(n));
			if (!copiarAtomico(destEdit, respaldo, e)) { volverAtras(); return rechazar(r, u, p, "no se pudo respaldar el option file: " + e); }
			if (!copiarAtomico(srcEdit, destEdit, e)) { volverAtras(); return rechazar(r, u, p, "no se pudo colocar el option file: " + e); }
			u.edit = aU8(destEdit);
			u.editRespaldo = aU8(respaldo);
			podarRespaldos(r.carpetaOptionFile, 5);
		}

		u.estado = "colocada";
		u.puedeDeshacer = true;
		u.motivo.clear();
		u.fecha = ahora("%Y-%m-%d %H:%M");
		u.aviso = avisoColocada(p.resumen);
		guardarUltima(r.carpetaEntrega, u);
		guardarUltima(r.carpetaEntrega, u, true);
		historial(r.carpetaEntrega, u, std::string(conPlayer ? "Player.bin x" + std::to_string(carpetasPlayer.size()) + " " : "")
			+ (conAsig ? "PlayerAssignment.bin x" + std::to_string(carpetasPlayer.size()) + " " : "") + (conEdit ? "EDIT00000000 " : "") + omitidos);
		archivar(r.carpetaEntrega, "entregados", p.id, p);
		return u;
	}
	catch (...) {
		return rechazar(r, u, p, "error inesperado");
	}
}

// --------------------------------------------------------------------- deshacer
Ultima deshacer(const Rutas& r) {
	Ultima u = leerUltima(r.carpetaEntrega);   // lo que se ve
	try {
		Ultima d = leerArchivoUltima(archivoDeshacer(r.carpetaEntrega));   // lo que hay que deshacer
		if (d.estado != "colocada" || !d.puedeDeshacer) { u.aviso.clear(); return u; }
		std::error_code ec;
		std::string e, fallos;
		for (const std::string& ruta : d.player) {
			const fs::path dst = deU8(ruta);
			const std::string nombre = aU8(dst.filename());
			const fs::path anterior = dst.parent_path() / (nombre + ".anterior");
			if (fs::is_regular_file(anterior, ec)) {
				if (!copiarAtomico(anterior, dst, e)) fallos += nombre + ": " + e + "; ";
			}
			else if (nombre == kNombreAsignacion) {
				// No había PlayerAssignment.bin propio antes: se aparta el nuestro y vuelve a mandar la base del parche
				fs::rename(dst, dst.parent_path() / (nombre + ".deshecho"), ec);
				if (ec) { fallos += nombre + ": no se pudo apartar; "; ec.clear(); }
			}
			else fallos += "falta " + nombre + ".anterior; ";
		}
		if (!d.edit.empty()) {
			const fs::path respaldo = deU8(d.editRespaldo);
			if (!fs::is_regular_file(respaldo, ec)) fallos += "falta el respaldo del option file; ";
			else if (!copiarAtomico(respaldo, deU8(d.edit), e)) fallos += "option file: " + e + "; ";
		}
		if (!fallos.empty()) {
			u.motivo = "No se pudo deshacer: " + fallos;
			u.aviso.clear();
			return u;
		}
		d.estado = "deshecha";
		d.puedeDeshacer = false;
		d.motivo.clear();
		d.fecha = ahora("%Y-%m-%d %H:%M");
		d.aviso = avisoDeshecha();
		guardarUltima(r.carpetaEntrega, d);
		guardarUltima(r.carpetaEntrega, d, true);
		historial(r.carpetaEntrega, d, "deshecha");
		return d;
	}
	catch (...) {
		u.motivo = "No se pudo deshacer: error inesperado";
		u.aviso.clear();
		return u;
	}
}

} // namespace phoenix::entrega
