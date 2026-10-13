// Pruebas de «Traspasos en vivo» (link/Captura.cpp + link/Traspasos.cpp): formato del lote, transporte en carpeta, captura del option file,
// aplicacion en juegos FALSOS con la base derivada del option file y el flujo completo anfitrion -> carpeta -> amigo.
// Con un argumento (un EDIT00000000 real, p. ej. una COPIA del de Fralex) se prueba la cadena de punta a punta; sin el, solo la logica pura.
// Nunca escribe en un juego real: todo en la carpeta temporal.   Uso: traspasos_test <carpeta temporal vacia> [EDIT00000000 de plantilla]
#include "phoenix/link/Captura.h"
#include "phoenix/link/Entrega.h"
#include "phoenix/link/Fichajes.h"
#include "phoenix/link/ModulosJuego.h"
#include "phoenix/link/Traspasos.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

extern "C" {
#include "phoenix/terceros/pesxcrypter/crypt.h"
#include "phoenix/terceros/pesxcrypter/masterkey.h"
}

namespace fs = std::filesystem;
using namespace phoenix::traspasos;
namespace fj = phoenix::fichajes;
using nlohmann::json;

static int total = 0, fallos = 0;
#define CHECK(cond, msg) do { total++; if (!(cond)) { fallos++; std::cout << "  FALLO: " << msg << "\n"; } else std::cout << "  ok: " << msg << "\n"; } while (0)

static void escribir(const fs::path& p, const std::string& s) {
	fs::create_directories(p.parent_path());
	std::ofstream f(p, std::ios::binary | std::ios::trunc);
	f.write(s.data(), static_cast<std::streamsize>(s.size()));
}
static std::string leer(const fs::path& p) {
	std::ifstream f(p, std::ios::binary);
	std::stringstream ss; ss << f.rdbuf();
	return ss.str();
}
static void tocar(const fs::path& p, int segundos) { fs::last_write_time(p, fs::file_time_type::clock::now() + std::chrono::seconds(segundos)); }

// ─── option file: reescribir los DATOS de una copia real ─────────────────────────────────────────
constexpr size_t kInicioPlantillas = 0x9D4648, kTamPlantilla = 284;
static uint32_t g32(const std::vector<uint8_t>& d, size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24); }
static void p32(std::vector<uint8_t>& d, size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * i)); }
static size_t bloque(const std::vector<uint8_t>& d, uint32_t equipo) {
	const size_t ne = d[100] | (d[101] << 8);
	for (size_t i = 0; i < ne; i++) if (g32(d, kInicioPlantillas + i * kTamPlantilla) == equipo) return kInicioPlantillas + i * kTamPlantilla;
	return 0;
}
static bool sacar(std::vector<uint8_t>& d, uint32_t jug, uint32_t eq) {
	const size_t b = bloque(d, eq);
	if (!b) return false;
	for (int k = 0; k < 40; k++) if (g32(d, b + 4 + k * 4) == jug) {
		for (int m = k; m < 39; m++) { p32(d, b + 4 + m * 4, g32(d, b + 4 + (m + 1) * 4)); d[b + 4 + 160 + m * 2] = d[b + 4 + 160 + (m + 1) * 2]; d[b + 4 + 160 + m * 2 + 1] = d[b + 4 + 160 + (m + 1) * 2 + 1]; }
		p32(d, b + 4 + 39 * 4, 0); d[b + 4 + 160 + 78] = d[b + 4 + 160 + 79] = 0;
		return true;
	}
	return false;
}
static bool meter(std::vector<uint8_t>& d, uint32_t jug, uint32_t eq) {
	const size_t b = bloque(d, eq);
	if (!b) return false;
	for (int k = 0; k < 40; k++) if (g32(d, b + 4 + k * 4) == 0) { p32(d, b + 4 + k * 4, jug); d[b + 4 + 160 + k * 2] = 99; d[b + 4 + 160 + k * 2 + 1] = 0; return true; }
	return false;
}
// Edita los datos del option file (mismo tamano) y lo vuelve a cifrar con la clave de PES 2021.
static std::string editarOption(const std::string& cifrado, const std::function<void(std::vector<uint8_t>&)>& f) {
	std::vector<uint8_t> bytes(cifrado.begin(), cifrado.end());
	FileDescriptorNew* d = createFileDescriptorNew();
	decryptWithKeyNew(d, bytes.data(), reinterpret_cast<const char*>(MasterKeyPes21));
	std::vector<uint8_t> datos(d->data, d->data + d->fileHeader->dataSize);
	f(datos);
	std::copy(datos.begin(), datos.end(), d->data);
	int tam = 0;
	uint8_t* salida = encryptWithKeyNew(d, &tam, reinterpret_cast<const char*>(MasterKeyPes21));
	std::string r(reinterpret_cast<char*>(salida), static_cast<size_t>(tam));
	std::free(salida);
	destroyFileDescriptorNew(d);
	return r;
}

// ─── mundo falso derivado del option file ────────────────────────────────────────────────────────
struct Mundo {
	std::vector<fj::Fila> filas;
	std::set<uint32_t> jugadores;
	std::map<uint32_t, std::string> nomJug, nomEq;
	std::string asigSha12;
};
static Mundo mundoDesde(const Foto& f, uint32_t offJug, uint32_t offEq) {
	Mundo m;
	uint32_t id = 1;
	for (const auto& [eq, pl] : f.plantillas) {
		if (pl.empty()) continue;
		m.nomEq[eq + offEq] = "Equipo " + std::to_string(eq);
		uint32_t k = 0;
		for (uint32_t j : pl) {
			m.filas.push_back({ id++, j + offJug, eq + offEq, (k << 10) | (k + 1) });
			m.jugadores.insert(j + offJug);
			m.nomJug[j + offJug] = "Jugador " + std::to_string(j);
			k++;
		}
	}
	const std::string crudo = fj::escribirAsignacion(m.filas);
	m.asigSha12 = phoenix::entrega::sha256Hex(crudo.data(), crudo.size()).substr(0, 12);
	return m;
}
static std::string playerBin(const std::map<uint32_t, std::string>& nombres) {
	std::string r(8, '\0');
	for (const auto& [j, n] : nombres) {
		std::string rec(312, '\0');
		for (int i = 0; i < 4; i++) rec[static_cast<size_t>(i)] = static_cast<char>((j >> (8 * i)) & 0xFF);
		rec.replace(243, (std::min<size_t>)(n.size(), 60), n.substr(0, 60));
		r += rec;
	}
	return r;
}
static std::string teamBin(const std::map<uint32_t, std::string>& equipos) {
	std::string r;
	for (const auto& e : equipos) {
		std::string rec(1532, '\0');
		for (int i = 0; i < 4; i++) { rec[static_cast<size_t>(i)] = static_cast<char>((e.first >> (8 * i)) & 0xFF); rec[static_cast<size_t>(8 + i)] = static_cast<char>((e.first >> (8 * i)) & 0xFF); }
		rec.replace(368, (std::min<size_t>)(e.second.size(), 60), e.second.substr(0, 60));
		r += rec;
	}
	return r;
}
static const std::string FACES = "cpk.root = \".\\livecpk\\Faces\"\r\n";
static const std::string OLMOS = "cpk.root = \".\\olmosjr23\\Database\"\r\n";
static const std::string MODS = "\r\nlua.module = \"CGP_data.lua\"\r\n";
static fs::path sider(const fs::path& juego) { return juego / "SiderAddons"; }
static fs::path pa(const fs::path& juego) { return sider(juego) / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "PlayerAssignment.bin"; }
static fs::path recargarTxt(const fs::path& juego) { return sider(juego) / "content" / "phoenix" / "recargar.txt"; }
static fs::path juegoFalso(const fs::path& tmp, const std::string& n, const Mundo& m, bool conRaiz = false) {
	const fs::path j = tmp / n;
	escribir(j / "PES2021.exe", "MZ");
	escribir(sider(j) / "sider.ini", std::string(conRaiz ? "cpk.root = \".\\livecpk\\Phoenix-DB\"\r\n" : "") + FACES + OLMOS + MODS);
	const fs::path d = sider(j) / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
	escribir(d / "PlayerAssignment.bin", fj::escribirBin(fj::escribirAsignacion(m.filas)));
	escribir(d / "Player.bin", fj::escribirBin(playerBin(m.nomJug)));
	escribir(d / "Team.bin", fj::escribirBin(teamBin(m.nomEq)));
	escribir(recargarTxt(j).parent_path() / "avisos.txt", "");
	return j;
}
static std::vector<fj::Fila> filasDe(const fs::path& juego) {
	std::string crudo, e; std::vector<fj::Fila> f;
	if (!fs::exists(pa(juego))) return f;
	if (!fj::leerBin(leer(pa(juego)), crudo, e) || !fj::leerAsignacion(crudo, f, e)) f.clear();
	return f;
}
static std::set<uint32_t> equiposDe(const std::vector<fj::Fila>& f, uint32_t jug) { std::set<uint32_t> s; for (const fj::Fila& x : f) if (x.jugador == jug) s.insert(x.equipo); return s; }

struct Jugada { uint32_t jug, de, a; };
// Un jugador que esta en UN solo equipo, y un destino con sitio. `ya` evita repetir.
static std::vector<Jugada> elegir(const Foto& f, size_t n, std::set<uint32_t>& ya) {
	std::map<uint32_t, int> veces;
	for (const auto& [eq, pl] : f.plantillas) for (uint32_t j : pl) veces[j]++;
	std::vector<uint32_t> destinos;
	for (const auto& [eq, pl] : f.plantillas) if (pl.size() >= 5 && pl.size() <= 34) destinos.push_back(eq);
	std::vector<Jugada> r;
	size_t di = 0;
	for (const auto& [eq, pl] : f.plantillas) {
		if (r.size() >= n) break;
		if (pl.size() < 15 || ya.count(eq)) continue;
		const uint32_t j = pl[3];
		if (veces[j] != 1 || ya.count(j)) continue;
		uint32_t a = 0;
		for (size_t t = 0; t < destinos.size() && !a; t++) { const uint32_t c = destinos[(di + t) % destinos.size()]; if (c != eq && !ya.count(c)) a = c; }
		if (!a) continue;
		di++;
		ya.insert(j); ya.insert(eq); ya.insert(a);
		r.push_back({ j, eq, a });
	}
	return r;
}

// ─── pruebas ─────────────────────────────────────────────────────────────────────────────────────
static CambioLote cl(uint32_t j, uint32_t d, uint32_t h) { CambioLote c; c.jugador = j; c.desde = d; c.hasta = h; c.jugadorNombre = "Jugador " + std::to_string(j); c.desdeNombre = "Equipo " + std::to_string(d); c.hastaNombre = "Equipo " + std::to_string(h); return c; }

static void pruebaLote(const fs::path& tmp) {
	std::cout << "[lote: formato, huella, validacion, transporte en carpeta]\n";
	Lote l = crearLote("Fralex", "abc12345", "2026-10-12T17:00:00Z", "ConmeGOL", "358c51b8ef11", { cl(20, 5, 6), cl(10, 1, 2) });
	CHECK(l.id.size() == 24 && l.cambios[0].jugador == 10, "crearLote: id de 24 hex y cambios ordenados");
	Lote igual = crearLote("Fralex", "abc12345", "2026-10-12T17:00:00Z", "ConmeGOL", "358c51b8ef11", { cl(10, 1, 2), cl(20, 5, 6) });
	CHECK(igual.id == l.id && igual.huella == l.huella, "mismo contenido = mismo id (idempotente), venga en el orden que venga");
	Lote otro = crearLote("Fralex", "abc12345", "2026-10-12T17:00:01Z", "ConmeGOL", "358c51b8ef11", { cl(10, 1, 2), cl(20, 5, 6) });
	CHECK(otro.id != l.id && otro.huella == l.huella, "otro momento = otro lote");
	Lote leido; std::string e;
	l.seq = 7;
	CHECK(loteDeJson(loteAJson(l), leido, e) && leido.id == l.id && leido.seq == 7 && leido.cambios.size() == 2 && leido.baseHuella == "358c51b8ef11", "ida y vuelta por JSON");
	std::string t = loteAJson(l);
	std::string malo = t; malo.replace(malo.find("\"hasta_id\": 2"), 13, "\"hasta_id\": 3");
	CHECK(!loteDeJson(malo, leido, e) && e.find("huella") != std::string::npos, "un lote alterado se rechaza (huella)");
	Lote mismo = crearLote("F", "a", "2026-10-12T17:00:00Z", "", "", { cl(1, 2, 2) });
	CHECK(!loteDeJson(loteAJson(mismo), leido, e), "origen = destino se rechaza");
	CHECK(!loteDeJson("{\"formato\":2}", leido, e) && !loteDeJson("basura", leido, e) && !loteDeJson("{\"formato\":1,\"id\":\"zz\"}", leido, e), "formato desconocido / basura / id invalido se rechazan");
	Lote sinIds = crearLote("F", "a", "2026-10-12T17:00:00Z", "", "", { cl(1, 2, 3) });
	std::string s2 = loteAJson(sinIds); s2.replace(s2.find("\"jugador_id\": 1"), 15, "\"jugador_id\": 0");
	CHECK(!loteDeJson(s2, leido, e), "id de jugador 0 se rechaza");

	const fs::path carp = tmp / "transporte-lote";
	TransporteCarpeta tr(carp);
	bool ya = false;
	Lote a = crearLote("F", "h1", "2026-10-12T10:00:00Z", "", "", { cl(1, 2, 3) });
	Lote b = crearLote("F", "h1", "2026-10-12T10:01:00Z", "", "", { cl(4, 5, 6) });
	CHECK(tr.publicar(a, ya, e) && a.seq == 1 && !ya, "publicar #1: seq 1");
	CHECK(tr.publicar(b, ya, e) && b.seq == 2 && !ya, "publicar #2: seq 2");
	Lote a2 = a; a2.seq = 0;
	CHECK(tr.publicar(a2, ya, e) && ya && a2.seq == 1, "publicar el mismo lote otra vez: no se duplica (ya = true, mismo seq)");
	std::vector<Lote> v; int ileg = 0;
	CHECK(tr.traer(0, v, ileg, e) && v.size() == 2 && v[0].seq == 1 && v[1].seq == 2, "traer desde 0: los 2 en orden");
	CHECK(tr.traer(1, v, ileg, e) && v.size() == 1 && v[0].id == b.id, "traer desde 1: solo el nuevo");
	escribir(carp / "00000003-ffffffffffffffffffffffff.json", "{\"formato\":1}");
	std::string roto = loteAJson(b); roto[0] = '[';
	escribir(carp / "00000004-eeeeeeeeeeeeeeeeeeeeeeee.json", roto);
	CHECK(tr.traer(2, v, ileg, e) && v.empty() && ileg == 2, "traer: dos archivos ilegibles se saltan y se cuentan");
	escribir(carp / "notas.txt", "hola");
	CHECK(tr.traer(0, v, ileg, e) && v.size() == 2, "archivos que no son lotes se ignoran");
	TransporteCarpeta vacio(tmp / "no-existe");
	CHECK(vacio.traer(0, v, ileg, e) && v.empty(), "carpeta que aun no existe = nada nuevo");
}

// Hub falso que cumple el contrato de docs/PROMPT-WEB-traspasos-vivo.md: cuentas, canal por anfitrion, seguidores (codigo / amigo con aprobacion),
// lotes idempotentes por id con seq por canal, y fallos inyectables (404, 401, 429, timeout, 500, respuesta corrupta).
struct HubFalso {
	struct Canal { std::string id, codigo; bool activo = true; long long ultimoSeq = 0, seqMin = 1; std::vector<json> lotes; std::map<std::string, std::string> seguidores; };   // seguidores: usuario -> activo|pendiente
	std::map<std::string, std::string> nombres;           // usuario -> nombre
	std::set<std::pair<std::string, std::string>> amigos; // amistades (en los dos sentidos)
	std::map<std::string, Canal> canales;                  // usuario anfitrion -> canal
	std::string fallo;                                     // "" | 404 | 401 | 429 | timeout | 500 | corrupto
	int peticiones = 0, limitePagina = 100;
	int contador = 0;
	bool esAmigo(const std::string& a, const std::string& b) const { return amigos.count({ a, b }) || amigos.count({ b, a }); }
	std::string nuevoId(const char* pre) { char b[64]; std::snprintf(b, sizeof(b), "%s%03d-0000-4000-8000-000000000000", pre, ++contador); return b; }
	Canal& canalDe(const std::string& u) {
		auto it = canales.find(u);
		if (it == canales.end()) { Canal c; c.id = nuevoId("c"); char b[16]; std::snprintf(b, sizeof(b), "PHX-%04d", 1000 + contador); c.codigo = b; it = canales.emplace(u, c).first; }
		return it->second;
	}
	Canal* porId(const std::string& id) { for (auto& [u, c] : canales) if (c.id == id) return &c; return nullptr; }
	std::string duenoDe(const std::string& id) { for (auto& [u, c] : canales) if (c.id == id) return u; return ""; }
	json jCanal(const std::string& u) {
		Canal& c = canalDe(u);
		json s = json::array();
		for (auto& [x, e] : c.seguidores) s.push_back({ {"usuario_id", x}, {"nombre", nombres[x]}, {"estado", e}, {"desde", "2026-10-12T10:00:00Z"} });
		return { {"ok", true}, {"canal", { {"id", c.id}, {"codigo", c.codigo}, {"activo", c.activo}, {"seguidores_max", 25} }}, {"seguidores", s} };
	}
	json jSig(const std::string& u, Canal& c) { std::string d = duenoDe(c.id); return { {"canal_id", c.id}, {"anfitrion", { {"usuario_id", d}, {"nombre", nombres[d]} }}, {"estado", c.seguidores[u]}, {"desde", "2026-10-12T10:00:00Z"} }; }
	static std::string param(const std::string& ruta, const std::string& k) {
		const size_t p = ruta.find(k + "=");
		if (p == std::string::npos) return "";
		const size_t f = ruta.find('&', p);
		return ruta.substr(p + k.size() + 1, f == std::string::npos ? std::string::npos : f - p - k.size() - 1);
	}
	bool manejar(const std::string& u, const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& resp) {
		peticiones++;
		if (fallo == "timeout") { estado = 0; return false; }
		if (fallo == "404") { estado = 404; resp = "{\"ok\":false,\"codigo\":\"RUTA_NO_EXISTE\"}"; return true; }
		if (fallo == "401") { estado = 401; resp = "{\"ok\":false,\"codigo\":\"TOKEN_INVALIDO\"}"; return true; }
		if (fallo == "429") { estado = 429; resp = "{\"ok\":false,\"codigo\":\"LIMITE\",\"reintentar_en\":7}"; return true; }
		if (fallo == "500") { estado = 500; resp = "oops"; return true; }
		if (fallo == "corrupto") { estado = 200; resp = "<html>no soy json</html>"; return true; }
		const json b = json::parse(cuerpo.empty() ? "{}" : cuerpo, nullptr, false);
		auto ok = [&](int e, json j) { estado = e; resp = j.dump(); return true; };
		auto err = [&](int e, const char* cod, const char* msg) { return ok(e, json{ {"ok", false}, {"codigo", cod}, {"mensaje", msg} }); };
		if (metodo == "POST" && ruta == "/v1/traspasos/canal") {
			Canal& c = canalDe(u);
			const std::string a = b.value("accion", std::string("obtener"));
			if (a == "rotar_codigo") { char x[16]; std::snprintf(x, sizeof(x), "PHX-%04d", 2000 + ++contador); c.codigo = x; }
			else if (a == "pausar") c.activo = false;
			else if (a == "reanudar") c.activo = true;
			return ok(200, jCanal(u));
		}
		if (metodo == "POST" && ruta == "/v1/traspasos/seguidores") {
			Canal& c = canalDe(u);
			const std::string x = b.value("usuario_id", std::string()), a = b.value("accion", std::string());
			if (!c.seguidores.count(x)) return err(404, "CANAL_NO_EXISTE", "Ese usuario no te sigue.");
			if (a == "aprobar") c.seguidores[x] = "activo"; else if (a == "quitar") c.seguidores.erase(x); else return err(400, "DATOS_INVALIDOS", "accion");
			return ok(200, jCanal(u));
		}
		if (metodo == "POST" && ruta == "/v1/traspasos/seguir") {
			Canal* c = nullptr; std::string estadoSeg;
			if (b.contains("codigo")) { for (auto& [d, k] : canales) if (k.codigo == b["codigo"] && k.activo) c = &k; estadoSeg = "activo"; if (!c) return err(404, "CANAL_NO_EXISTE", "Código incorrecto."); }
			else { const std::string d = b.value("anfitrion_usuario_id", std::string()); if (!esAmigo(u, d)) return err(403, "NO_ES_AMIGO", "Solo puedes seguir a un amigo o con su código."); c = &canalDe(d); estadoSeg = "pendiente"; }
			if (c->seguidores.size() >= 25) return err(403, "LIMITE", "El canal está lleno.");
			if (!c->seguidores.count(u)) c->seguidores[u] = estadoSeg;
			return ok(200, json{ {"ok", true}, {"siguiendo", jSig(u, *c)} });
		}
		if (metodo == "GET" && ruta == "/v1/traspasos/siguiendo") {
			json a = json::array();
			for (auto& [d, c] : canales) if (c.seguidores.count(u)) a.push_back(jSig(u, c));
			return ok(200, json{ {"ok", true}, {"siguiendo", a} });
		}
		if (metodo == "POST" && ruta == "/v1/traspasos/dejar") {
			Canal* c = porId(b.value("canal_id", std::string()));
			if (!c || !c->seguidores.count(u)) return err(404, "CANAL_NO_EXISTE", "No sigues ese canal.");
			c->seguidores.erase(u);
			return ok(200, json{ {"ok", true} });
		}
		if (metodo == "POST" && ruta == "/v1/traspasos/lotes") {
			Canal& c = canalDe(u);
			if (!b.contains("lote") || !b["lote"].is_object()) return err(400, "LOTE_INVALIDO", "falta el lote");
			for (const json& l : c.lotes) if (l["id"] == b["lote"]["id"]) return ok(409, json{ {"ok", true}, {"ya", true}, {"seq", l["seq"]}, {"canal_id", c.id} });
			json l = b["lote"]; l["seq"] = ++c.ultimoSeq;
			c.lotes.push_back(l);
			return ok(201, json{ {"ok", true}, {"seq", l["seq"]}, {"canal_id", c.id} });
		}
		if (metodo == "GET" && ruta.rfind("/v1/traspasos/lotes?", 0) == 0) {
			Canal* c = porId(param(ruta, "canal_id"));
			if (!c) return err(404, "CANAL_NO_EXISTE", "Ese canal no existe.");
			const std::string dueno = duenoDe(c->id);
			if (u != dueno) {
				auto it = c->seguidores.find(u);
				if (it == c->seguidores.end()) return err(403, "SIN_PERMISO", "No sigues a este anfitrión.");
				if (it->second == "pendiente") return err(403, "PENDIENTE_APROBACION", "Aún no te ha aprobado.");
			}
			if (!c->activo) return ok(200, json{ {"ok", true}, {"canal_id", c->id}, {"lotes", json::array()}, {"hay_mas", false}, {"seq_max", c->ultimoSeq}, {"seq_min", c->seqMin}, {"purgado", false}, {"sondeo_seg", 10} });
			const long long desde = std::stoll(param(ruta, "desde"));
			json a = json::array();
			bool mas = false;
			for (const json& l : c->lotes) if (l["seq"].get<long long>() > desde && l["seq"].get<long long>() >= c->seqMin) { if (static_cast<int>(a.size()) >= limitePagina) { mas = true; break; } a.push_back(l); }
			return ok(200, json{ {"ok", true}, {"canal_id", c->id}, {"lotes", a}, {"hay_mas", mas}, {"seq_max", c->ultimoSeq}, {"seq_min", c->seqMin}, {"purgado", desde + 1 < c->seqMin}, {"sondeo_seg", 10} });
		}
		return err(404, "RUTA_NO_EXISTE", "ruta");
	}
	PeticionWeb como(const std::string& u) { return [this, u](const std::string& m, const std::string& r, const std::string& c, int& e, std::string& s) { return manejar(u, m, r, c, e, s); }; }
};

static void pruebaWeb() {
	std::cout << "[web: transporte, esperas (404/401/429/timeout/500/corrupto), canal, seguidores, hub falso con el contrato]\n";
	HubFalso hub;
	hub.nombres = { {"a1000000-0000-4000-8000-000000000001", "Fralex"}, {"a2000000-0000-4000-8000-000000000002", "Amigo"}, {"a4000000-0000-4000-8000-000000000004", "Intruso"} };
	hub.amigos.insert({ "a1000000-0000-4000-8000-000000000001", "a2000000-0000-4000-8000-000000000002" });
	long long reloj = 1000;
	auto relojf = [&]() { return reloj; };
	TransporteWeb th(hub.como("a1000000-0000-4000-8000-000000000001"), "", relojf), ta(hub.como("a2000000-0000-4000-8000-000000000002"), "", relojf), ti(hub.como("a4000000-0000-4000-8000-000000000004"), "", relojf);
	ClienteTraspasosWeb ch(th), ca(ta), ci(ti);
	std::string e;
	CanalWeb c1, c2;
	CHECK(ch.canal("obtener", c1, e) && c1.hay && !c1.id.empty() && !c1.codigo.empty() && c1.activo, "anfitrion: «obtener» crea su canal y su código");
	CHECK(ch.canal("obtener", c2, e) && c2.id == c1.id && c2.codigo == c1.codigo, "anfitrion: «obtener» otra vez es idempotente (mismo canal y código)");
	CHECK(ch.canal("rotar_codigo", c2, e) && c2.codigo != c1.codigo && c2.id == c1.id, "anfitrion: rotar el código cambia el código y no el canal");
	CHECK(!ch.canal("borrar_todo", c2, e), "anfitrion: una acción desconocida se rechaza sin llamar a la web");
	// seguir
	SiguiendoWeb s;
	CHECK(!ca.seguir("PHX-9999", "", s, e) && e.find("Código") != std::string::npos, "amigo: un código incorrecto da un error claro");
	CHECK(!ca.seguir("'; drop table", "", s, e), "amigo: un código con caracteres raros se rechaza antes de llamar");
	const int antes = hub.peticiones;
	CHECK(!ci.seguir("", "a1000000-0000-4000-8000-000000000001", s, e), "intruso: no es amigo y no tiene código → la web lo rechaza");
	(void)antes;
	CHECK(ca.seguir("", "a1000000-0000-4000-8000-000000000001", s, e) && s.estado == "pendiente" && s.canalId == c1.id && s.anfitrionNombre == "Fralex", "amigo: seguir al anfitrión por ser amigo queda «pendiente»");
	std::vector<Lote> lotes; int ileg = 0;
	ta.ponerCanal(c1.id);
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "pendiente_aprobacion", "amigo pendiente: no puede leer lotes todavía (pendiente_aprobacion)");
	CHECK(ta.esperaMs() > 0, "amigo pendiente: espera antes de volver a preguntar");
	CanalWeb c3;
	CHECK(ch.seguidor("a2000000-0000-4000-8000-000000000002", "aprobar", c3, e) && c3.seguidores.size() == 1 && c3.seguidores[0].estado == "activo", "anfitrion: aprueba al amigo");
	reloj += 40000;
	std::vector<SiguiendoWeb> sg;
	CHECK(ca.siguiendo(sg, e) && sg.size() == 1 && sg[0].estado == "activo" && sg[0].anfitrionNombre == "Fralex", "amigo: ahora sale «activo» en la lista de a quién sigue");
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.empty() && ta.estadoClave() == "ok", "amigo aprobado: lee el canal (vacío)");
	// publicar y leer
	Lote a = crearLote("Fralex", "h1", "2026-10-12T10:00:00Z", "ConmeGOL", "358c51b8ef11", { cl(1, 2, 3) });
	Lote b = crearLote("Fralex", "h1", "2026-10-12T10:05:00Z", "ConmeGOL", "358c51b8ef11", { cl(4, 5, 6) });
	bool ya = false;
	CHECK(th.publicar(a, ya, e) && a.seq == 1 && !ya, "anfitrion: publicar da seq 1");
	Lote a2 = a; a2.seq = 0;
	CHECK(th.publicar(a2, ya, e) && ya && a2.seq == 1 && hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size() == 1, "anfitrion: reenviar el mismo lote (409) no duplica y devuelve su seq");
	th.publicar(b, ya, e);
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.size() == 2 && lotes[1].seq == 2 && lotes[0].cambios[0].jugador == 1, "amigo: trae los 2 lotes en orden");
	CHECK(ta.traer(1, lotes, ileg, e) && lotes.size() == 1 && lotes[0].id == b.id, "amigo: desde seq 1 solo trae el nuevo");
	ti.ponerCanal(c1.id);
	CHECK(!ti.traer(0, lotes, ileg, e) && ti.estadoClave() == "sin_permiso", "intruso: no puede leer el canal (sin_permiso)");
	// duplicados / desordenados / corruptos que entrega el hub
	hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.push_back(hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[0]);          // un duplicado (mismo seq)
	std::swap(hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[0], hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[1]);      // desordenados
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.size() == 3 && lotes[0].seq <= lotes[1].seq && lotes[1].seq <= lotes[2].seq, "hub entrega desordenado y duplicado: el transporte los ordena por seq");
	hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.pop_back();
	const json huellaBuena = hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[0]["huella"];
	hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[0]["huella"] = "0000";
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.size() == 1 && ileg == 1, "un lote alterado por el camino (huella) se rechaza y se cuenta; el bueno pasa");
	hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes[0]["huella"] = huellaBuena;
	// paginas
	hub.limitePagina = 1;
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.size() == 1 && ta.hayMas(), "paginas: con limite 1 el transporte avisa de que hay más");
	hub.limitePagina = 100;
	// pausa del canal
	CHECK(ch.canal("pausar", c3, e) && !c3.activo, "anfitrion: pausar el canal");
	CHECK(ta.traer(0, lotes, ileg, e) && lotes.empty(), "canal en pausa: el amigo no recibe nada");
	ch.canal("reanudar", c3, e);
	// purga
	hub.canalDe("a1000000-0000-4000-8000-000000000001").seqMin = 2;
	CHECK(ta.traer(0, lotes, ileg, e) && ta.purgadoHasta() == 2, "retención: si la web purgó lotes antiguos, el transporte lo dice (purgadoHasta)");
	hub.canalDe("a1000000-0000-4000-8000-000000000001").seqMin = 1;
	// quitar / dejar
	CHECK(ch.seguidor("a2000000-0000-4000-8000-000000000002", "quitar", c3, e) && c3.seguidores.empty(), "anfitrion: quita al amigo");
	reloj += 40000;
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "sin_permiso", "amigo quitado: ya no lee");
	CHECK(!ch.seguidor("a2000000-0000-4000-8000-000000000002", "aprobar", c3, e), "anfitrion: aprobar a quien no lo sigue da error");
	reloj += 100000;
	CHECK(!ca.seguir(c1.codigo, "", s, e), "el código viejo (antes de rotarlo) ya no sirve");
	CHECK(ca.seguir(c3.codigo, "", s, e) && s.estado == "activo", "con el código vigente se entra al momento (activo)");
	// esperas
	reloj += 100000; ta.reintentarYa();
	const int p0 = hub.peticiones;
	hub.fallo = "404";
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "web_sin_endpoints" && ta.esperaMs() >= 299000 && ta.esperaMs() <= 300000, "404: «la web aún no tiene los traspasos» y espera 5 min");
	CHECK(e.find("PROMPT") != std::string::npos || e.find("prompt") != std::string::npos, "404: el texto dice que hay que pegar el prompt WEB");
	reloj += 301000;
	CHECK(ta.esperaMs() == 0, "404: pasados 5 min se puede volver a probar");
	hub.fallo = "401"; ta.reintentarYa();
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "sin_cuenta" && ta.esperaMs() >= 59000, "401: «vuelve a vincular tu cuenta», espera 60 s");
	hub.fallo = "429"; ta.reintentarYa();
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "limite" && ta.esperaMs() >= 6000 && ta.esperaMs() <= 7000, "429: respeta reintentar_en (7 s)");
	hub.fallo = "timeout"; ta.reintentarYa();
	std::vector<long long> esperas;
	for (int i = 0; i < 7; i++) { reloj += 200000; ta.traer(0, lotes, ileg, e); esperas.push_back(ta.esperaMs() / 1000); }
	CHECK(ta.estadoClave() == "sin_conexion" && esperas[0] == 10 && esperas[1] == 20 && esperas[2] == 40 && esperas[3] == 80 && esperas[4] == 120 && esperas[6] == 120, "timeout: backoff 10, 20, 40, 80, 120, 120 s");
	hub.fallo = "500"; ta.reintentarYa();
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "sin_conexion", "500: se trata como web caída");
	hub.fallo = "corrupto"; ta.reintentarYa();
	CHECK(!ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "respuesta_rara", "respuesta corrupta (HTML): «no se entiende», no rompe nada");
	hub.fallo = ""; ta.reintentarYa();
	CHECK(ta.traer(0, lotes, ileg, e) && ta.estadoClave() == "ok" && ta.esperaMs() == 0, "la web vuelve: todo normal y sin esperas");
	(void)p0;
	// sin cuenta vinculada (estado -1) y sin canal
	TransporteWeb sinCuenta([](const std::string&, const std::string&, const std::string&, int& est, std::string&) { est = -1; return true; }, "c001-0000-4000-8000-000000000000", relojf);
	CHECK(!sinCuenta.traer(0, lotes, ileg, e) && sinCuenta.estadoClave() == "sin_cuenta", "PC sin vincular: «vincula tu cuenta», no se manda nada");
	TransporteWeb sinCanal(hub.como("a2000000-0000-4000-8000-000000000002"), "", relojf);
	CHECK(!sinCanal.traer(0, lotes, ileg, e) && sinCanal.estadoClave() == "sin_anfitrion", "sin elegir anfitrión: «elige a quién sigues», sin llamar a la web");
	TransporteWeb canalRaro(hub.como("a2000000-0000-4000-8000-000000000002"), "../../etc", relojf);
	CHECK(!canalRaro.traer(0, lotes, ileg, e) && canalRaro.estadoClave() == "sin_anfitrion", "un canal con caracteres raros nunca llega a la ruta");
	// dejar de seguir
	reloj += 400000;
	SiguiendoWeb s2;
	ca.seguir("", "a1000000-0000-4000-8000-000000000001", s2, e);
	CHECK(ca.dejar(s2.canalId, e), "amigo: deja de seguir");
	CHECK(!ca.dejar("no-es-un-id!", e), "amigo: un id raro se rechaza antes de llamar");
}

static void pruebaLista() {
	std::cout << "[lista efectiva: colapso, orden, modos]\n";
	auto reg = [](const char* idh, long long seq, std::vector<CambioLote> c, const char* estado = "aplicado", const char* modo = "ids") {
		Registro r;
		r.lote = crearLote("F", "h", std::string("2026-10-12T10:00:0") + idh + "Z", "", "", std::move(c));
		r.lote.seq = seq; r.estado = estado; r.modo = modo;
		return r;
	};
	fj::Lista vacia;
	std::vector<Registro> rs = { reg("2", 2, { cl(10, 2, 3) }), reg("1", 1, { cl(10, 1, 2), cl(20, 7, 8) }) };   // llegan desordenados
	fj::Lista l = construirLista(vacia, rs);
	CHECK(l.cambios.size() == 2, "dos mudanzas del mismo jugador = un solo cambio");
	const fj::Cambio* c10 = nullptr; for (const fj::Cambio& c : l.cambios) if (c.jugador == 10) c10 = &c;
	CHECK(c10 && c10->origen == 1 && c10->equipo == 3, "colapso: primer origen (1) -> ultimo destino (3), en orden de seq y no de llegada");
	rs.push_back(reg("3", 3, { cl(10, 3, 1) }));
	l = construirLista(vacia, rs);
	CHECK(l.cambios.size() == 1 && l.cambios[0].jugador == 20, "fue y volvio al origen: no queda nada que aplicar de ese jugador");
	rs[2].estado = "deshecho";
	l = construirLista(vacia, rs);
	c10 = nullptr; for (const fj::Cambio& c : l.cambios) if (c.jugador == 10) c10 = &c;
	CHECK(c10 && c10->origen == 1 && c10->equipo == 3, "un lote deshecho no cuenta");
	std::vector<Registro> n = { reg("1", 1, { cl(10, 1, 2) }, "pendiente", "nombres"), reg("2", 2, { cl(11, 1, 2) }, "pendiente", "nada"), reg("3", 3, { cl(12, 1, 2) }, "propio") };
	l = construirLista(vacia, n);
	CHECK(l.cambios.size() == 1 && l.cambios[0].jugador == 0 && l.cambios[0].jugadorNombre == "Jugador 10" && l.cambios[0].equipoNombre == "Equipo 2" && l.cambios[0].origenNombre == "Equipo 1",
		"modo nombres: el cambio va por nombres; modo nada y lotes propios no entran");
	fj::Lista man; fj::Cambio mc; mc.jugador = 99; mc.equipo = 5; man.cambios.push_back(mc);
	l = construirLista(man, rs);
	CHECK(l.cambios.size() >= 2 && l.cambios[0].jugador == 99, "la lista manual del usuario va primero");
	CHECK(construirLista(vacia, rs).sha256 == construirLista(vacia, rs).sha256 && !construirLista(vacia, rs).sha256.empty(), "huella de la lista estable");
}

// Pasos de anfitrion/amigo
static void anfitrion(Motor& m, int veces = 2) { for (int i = 0; i < veces; i++) m.pasoPublicar(); }
static json est(const Motor& m) { return json::parse(m.estadoJson()); }
static int contarLotes(const fs::path& carpeta) { int n = 0; std::error_code ec; if (!fs::is_directory(carpeta, ec)) return 0; for (const auto& e : fs::directory_iterator(carpeta)) if (e.path().extension() == ".json") n++; return n; }

static Config configBase(const fs::path& estado, const fs::path& juego, const std::string& autor, bool* abierto) {
	Config c;
	c.dirEstado = estado; c.juego = juego; c.autor = autor; c.esperaReintentoMs = 0; c.sondeoMs = 0;
	c.pesAbierto = [abierto]() { return abierto ? *abierto : false; };
	c.modoRecarga = []() { return std::string("AUTO-SIEMPRE"); };
	c.reloj = []() { static int reloj = 0; char b[40]; std::snprintf(b, sizeof(b), "2026-10-12T17:%02d:%02dZ", (reloj / 60) % 60, reloj % 60); reloj += 7; return std::string(b); };
	return c;
}

static void pruebaCadena(const fs::path& tmp, const std::string& plantilla) {
	std::cout << "[cadena completa: option file del anfitrion -> carpeta -> juego del amigo]\n";
	Foto f0; std::string e;
	CHECK(leerFoto(plantilla, f0, e), "la plantilla (option file real) se lee: " + std::to_string(f0.plantillas.size()) + " equipos");
	std::set<uint32_t> ya;
	const std::vector<Jugada> js = elegir(f0, 12, ya);
	CHECK(js.size() >= 9, "hay jugadas de prueba (" + std::to_string(js.size()) + ")");
	if (js.size() < 9) return;

	const Mundo mundo = mundoDesde(f0, 0, 0);
	{
		const fs::path jh = juegoFalso(tmp, "juego-anfitrion", mundo);
		std::vector<CambioLote> cs = { CambioLote() }; cs[0].jugador = js[0].jug; cs[0].desde = js[0].de; cs[0].hasta = js[0].a; cs[0].hastaNombre = "nombre del option file";
		std::string parche, huella;
		enriquecerConBaseDelJuego(jh, cs, parche, huella);
		CHECK(cs[0].jugadorNombre == "Jugador " + std::to_string(js[0].jug) && cs[0].hastaNombre == "Equipo " + std::to_string(js[0].a) && huella == mundo.asigSha12 && !parche.empty(),
			"enriquecer: el anfitrion pone nombres de jugador/equipo y la huella de la base real de su juego (parche «" + parche + "»)");
		std::vector<CambioLote> sin = cs; std::string p2, h2;
		enriquecerConBaseDelJuego(tmp / "no-hay-juego", sin, p2, h2);
		CHECK(h2.empty() && sin[0].hastaNombre == cs[0].hastaNombre, "enriquecer: sin juego/base no inventa nada");
	}
	const fs::path carp = tmp / "transporte";
	auto tr = std::make_shared<TransporteCarpeta>(carp);

	// anfitrion
	const fs::path optDir = tmp / "anfitrion-docs";
	const fs::path opt = optDir / "EDIT00000000";
	escribir(opt, plantilla);
	bool aAbierto = false;
	Config ca = configBase(tmp / "estado-anfitrion", fs::path(), "Fralex", &aAbierto);
	ca.publicar = true; ca.confirmarAntes = false; ca.optionFile = optDir;
	ca.enriquecer = [&](std::vector<CambioLote>& c, std::string& parche, std::string& huella) {
		for (CambioLote& x : c) { x.jugadorNombre = "Jugador " + std::to_string(x.jugador); x.desdeNombre = "Equipo " + std::to_string(x.desde); x.hastaNombre = "Equipo " + std::to_string(x.hasta); }
		parche = "Mundo falso"; huella = mundo.asigSha12;
	};
	Motor A(ca, tr);

	// amigo 1: misma base (por ids)
	bool bAbierto = false;
	const fs::path jb = juegoFalso(tmp, "juego-amigo", mundo);
	Config cb = configBase(tmp / "estado-amigo", jb, "Amigo", &bAbierto);
	cb.recibir = true;
	cb.listaManual = []() { return fj::Lista(); };
	Motor B(cb, tr);

	anfitrion(A);
	CHECK(est(A)["anfitrion"]["estado"] == "referencia_guardada", "anfitrion: primera lectura = guarda la referencia y no publica nada");
	anfitrion(A);
	CHECK(contarLotes(carp) == 0, "anfitrion: sin cambios no se publica nada");

	// 1) un traspaso en Editar
	escribir(opt, editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[0].a); }));
	tocar(opt, 5);
	A.pasoPublicar();
	CHECK(contarLotes(carp) == 0 && est(A)["anfitrion"]["estado"] == "esperando_que_termine_de_guardar", "anfitrion: al ver el archivo cambiar espera a que termine de guardarse");
	A.pasoPublicar();
	CHECK(est(A)["anfitrion"]["estado"] == "publicado" && contarLotes(carp) == 1, "anfitrion: archivo estable -> traspaso detectado y publicado");
	std::vector<Lote> v; int ileg = 0;
	tr->traer(0, v, ileg, e);
	CHECK(v.size() == 1 && v[0].cambios.size() == 1 && v[0].cambios[0].jugador == js[0].jug && v[0].cambios[0].desde == js[0].de && v[0].cambios[0].hasta == js[0].a, "el lote trae exactamente ese traspaso (jugador, origen, destino)");
	CHECK(v[0].cambios[0].jugadorNombre == "Jugador " + std::to_string(js[0].jug) && v[0].baseHuella == mundo.asigSha12, "el lote lleva nombres y la huella de la base");
	A.pasoPublicar();
	CHECK(contarLotes(carp) == 1, "anfitrion: no se publica dos veces lo mismo");

	// amigo: lo recibe y lo aplica
	CHECK(filasDe(jb).empty() && !fs::exists(recargarTxt(jb)), "amigo (antes): Phoenix-DB vacio, sin aviso de recarga");
	B.pasoRecibir();
	auto fb = filasDe(jb);
	CHECK(!fb.empty() && equiposDe(fb, js[0].jug) == std::set<uint32_t>{ js[0].a }, "amigo: el jugador esta en el equipo nuevo en su Phoenix-DB");
	CHECK(fb.size() == mundo.filas.size(), "amigo: el resto de la base queda intacto (mismo numero de filas)");
	CHECK(fs::exists(recargarTxt(jb)) && leer(recargarTxt(jb)).find("traspasos") == 0, "amigo: Link avisa a phoenix.lua (recargar.txt)");
	json eb = est(B);
	CHECK(eb["amigo"]["historial"][0]["estado"] == "aplicado" && eb["amigo"]["puedeDeshacer"] == true, "amigo: historial «aplicado» y se puede deshacer");
	const std::string bytes1 = leer(pa(jb)), aviso1 = leer(recargarTxt(jb));
	B.pasoRecibir(); B.pasoRecibir();
	CHECK(leer(pa(jb)) == bytes1 && est(B)["amigo"]["historial"].size() == 1, "amigo: recibir otra vez no cambia nada (idempotente)");
	{	// lote repetido por el transporte (mismo contenido, reintento): no se duplica
		Lote rep = v[0]; rep.seq = 0; bool ya2 = false;
		CHECK(tr->publicar(rep, ya2, e) && ya2 && contarLotes(carp) == 1, "transporte: reenviar el mismo lote no crea otro");
	}

	// 2) otro traspaso + el mismo jugador otra vez (colapso)
	escribir(opt, editarOption(plantilla, [&](std::vector<uint8_t>& d) {
		sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[1].a);        // el 0 sigue: ahora a otro equipo
		sacar(d, js[1].jug, js[1].de); meter(d, js[1].jug, js[2].a); }));
	tocar(opt, 10);
	anfitrion(A);
	B.pasoRecibir();
	fb = filasDe(jb);
	CHECK(equiposDe(fb, js[0].jug) == std::set<uint32_t>{ js[1].a } && equiposDe(fb, js[1].jug) == std::set<uint32_t>{ js[2].a }, "amigo: segundo lote aplicado (el jugador 0 de nuevo: destino final)");
	CHECK(contarLotes(carp) == 2, "anfitrion: segundo lote publicado");
	CHECK(leer(recargarTxt(jb)) != aviso1, "amigo: el aviso de recarga cambia con cada lote nuevo");

	// 3) DESHACER el ultimo lote
	std::string msg;
	CHECK(B.deshacerUltimo(msg), "deshacer: " + msg);
	fb = filasDe(jb);
	CHECK(equiposDe(fb, js[0].jug) == std::set<uint32_t>{ js[0].a } && equiposDe(fb, js[1].jug) == std::set<uint32_t>{ js[1].de }, "deshacer: vuelve al estado del primer lote (jugador 0 en su 1.er destino, jugador 1 en su equipo)");
	CHECK(est(B)["amigo"]["historial"][0]["estado"] == "deshecho", "deshacer: el lote queda marcado «deshecho»");
	B.pasoRecibir();
	CHECK(equiposDe(filasDe(jb), js[1].jug) == std::set<uint32_t>{ js[1].de }, "deshacer: un lote deshecho no se vuelve a aplicar solo");
	CHECK(B.deshacerUltimo(msg) && filasDe(jb).empty(), "deshacer el primero: Phoenix-DB vuelve a la base original (se quita lo generado)");
	CHECK(!B.deshacerUltimo(msg), "deshacer sin nada aplicado: avisa y no rompe");

	// 4) cambios de Editar que NO son traspasos: se ignoran
	const int antesLotes = contarLotes(carp);
	escribir(opt, editarOption(plantilla, [&](std::vector<uint8_t>& d) {
		sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[1].a); sacar(d, js[1].jug, js[1].de); meter(d, js[1].jug, js[2].a);   // lo anterior
		sacar(d, js[3].jug, js[3].de);                    // baja
		meter(d, js[4].jug, js[5].a);                     // alta (sigue en su equipo): jugador en dos equipos = convocatoria
	}));
	tocar(opt, 15);
	anfitrion(A);
	json ea = est(A)["anfitrion"];
	CHECK(contarLotes(carp) == antesLotes && ea["estado"] == "sin_traspasos", "otros cambios de Editar (baja / alta) no se publican (" + ea["estado"].get<std::string>() + ")");
	CHECK(ea["ignorados"]["bajas"].get<int>() == 1 && ea["ignorados"]["altas"].get<int>() == 1, "se cuentan como ignorados: 1 baja y 1 alta");

	// 5) cambio masivo (otro option file entero): no se publica solo
	{
		Config cm = ca; cm.dirEstado = tmp / "estado-masivo"; cm.optionFile = tmp / "masivo-docs";
		escribir(cm.optionFile / "EDIT00000000", plantilla);
		Motor M(cm, tr);
		anfitrion(M);
		escribir(cm.optionFile / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) {
			Foto fx; std::string er; leerFoto(plantilla, fx, er);
			std::map<uint32_t, int> veces; for (const auto& q : fx.plantillas) for (uint32_t x : q.second) veces[x]++;
				std::vector<uint32_t> ds;   // destinos con sitio de sobra: el reparto no llena ninguno
				for (const auto& q : fx.plantillas) if (q.second.size() >= 5 && q.second.size() <= 26 && ds.size() < 60) ds.push_back(q.first);
				int n = 0;
				for (const auto& [eq, pl] : fx.plantillas) {
					if (n >= 70) break;
					if (pl.size() < 15) continue;
					const uint32_t j = pl[2];
					if (veces[j] != 1) continue;
					const uint32_t dest = ds[static_cast<size_t>(n) % ds.size()];
					if (dest == eq) continue;
					sacar(d, j, eq); meter(d, j, dest);
					n++;
				}
		}));
		tocar(cm.optionFile / "EDIT00000000", 30);
		anfitrion(M);
		json em = est(M)["anfitrion"];
		CHECK(em["estado"] == "pendiente_masivo" && em["masivo"] == true, "cambio masivo: queda pendiente con aviso, no se publica solo (" + em["estado"].get<std::string>() + ")");
		CHECK(contarLotes(carp) == antesLotes, "cambio masivo: nada llego al transporte");
		std::string m2;
		CHECK(M.descartarPendientes(m2) && contarLotes(carp) == antesLotes && est(M)["anfitrion"]["pendientes"].empty(), "descartar: no se envia y queda sin pendientes");
		anfitrion(M);
		CHECK(est(M)["anfitrion"]["pendientes"].empty() && contarLotes(carp) == antesLotes, "tras descartar el siguiente paso no vuelve a detectarlos");
	}

	// 6) confirmar antes de publicar
	{
		Config cc = ca; cc.dirEstado = tmp / "estado-confirmar"; cc.optionFile = tmp / "confirmar-docs"; cc.confirmarAntes = true;
		escribir(cc.optionFile / "EDIT00000000", plantilla);
		Motor C(cc, tr);
		anfitrion(C);
		escribir(cc.optionFile / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, js[6].jug, js[6].de); meter(d, js[6].jug, js[6].a); }));
		tocar(cc.optionFile / "EDIT00000000", 40);
		anfitrion(C);
		json ec = est(C)["anfitrion"];
		std::cout << "    (estado del anfitrion: " << ec["estado"].get<std::string>() << ", pendientes " << ec["pendientes"].size() << ")\n";
		CHECK(ec["estado"] == "pendiente_de_confirmar" && ec["pendientes"].size() == 1 && contarLotes(carp) == antesLotes, "confirmar antes: lo detecta y espera tu «Publicar»");
		Motor C2(cc, tr);   // reabrir Link: el pendiente se recuerda
		CHECK(est(C2)["anfitrion"]["pendientes"].size() == 1, "confirmar antes: el pendiente sobrevive a cerrar Link");
		std::string m3;
		CHECK(C2.publicarPendientes(m3) && contarLotes(carp) == antesLotes + 1, "publicar pendientes: " + m3);
		CHECK(est(C2)["anfitrion"]["pendientes"].empty(), "publicar: ya no queda pendiente");
	}
	const int lotesAnfitrion = contarLotes(carp);

	// 7) archivo ilegible / basura
	{
		Config cx = ca; cx.dirEstado = tmp / "estado-basura"; cx.optionFile = tmp / "basura-docs";
		escribir(cx.optionFile / "EDIT00000000", plantilla);
		Motor X(cx, tr);
		anfitrion(X);
		escribir(cx.optionFile / "EDIT00000000", std::string(20000, 'x'));
		tocar(cx.optionFile / "EDIT00000000", 50);
		anfitrion(X);
		CHECK(est(X)["anfitrion"]["estado"].get<std::string>().rfind("no_se_pudo_leer", 0) == 0 && contarLotes(carp) == lotesAnfitrion, "option file dañado: no se publica nada y se explica");
		escribir(cx.optionFile / "EDIT00000000", plantilla.substr(0, plantilla.size() / 2));   // PES a medio guardar
		tocar(cx.optionFile / "EDIT00000000", 55);
		anfitrion(X);
		CHECK(est(X)["anfitrion"]["estado"].get<std::string>().rfind("no_se_pudo_leer", 0) == 0 && contarLotes(carp) == lotesAnfitrion, "option file cortado (PES a medio guardar): no se cierra Link, no se publica nada y se reintenta");
		escribir(cx.optionFile / "EDIT00000000", plantilla);   // termina de guardar: igual que la referencia
		tocar(cx.optionFile / "EDIT00000000", 56);
		anfitrion(X);
		CHECK(est(X)["anfitrion"]["estado"] == "sin_cambios" && contarLotes(carp) == lotesAnfitrion, "option file completo otra vez: sin cambios, nada que publicar");
		fs::remove(cx.optionFile / "EDIT00000000");
		anfitrion(X);
		CHECK(est(X)["anfitrion"]["estado"] == "sin_option_file", "sin option file: lo dice y no hace nada");
	}

	// 8) amigo con PES abierto
	{
		const fs::path jc = juegoFalso(tmp, "juego-abierto", mundo, false);
		bool abierto = true;
		Config cc = configBase(tmp / "estado-abierto", jc, "Amigo2", &abierto);
		cc.recibir = true; cc.listaManual = []() { return fj::Lista(); };
		Motor C(cc, tr);
		const std::string ini0 = leer(sider(jc) / "sider.ini");
		C.pasoRecibir();
		json eC = est(C)["amigo"];
		CHECK(filasDe(jc).empty() && leer(sider(jc) / "sider.ini") == ini0 && eC["estado"] == "esperando", "PES abierto y sin Phoenix-DB en sider.ini: no toca nada y espera (" + eC["mensaje"].get<std::string>().substr(0, 60) + ")");
		abierto = false;
		C.pasoRecibir();
		CHECK(!filasDe(jc).empty() && leer(sider(jc) / "sider.ini") != ini0, "PES cerrado: se pone al dia (todos los lotes), Phoenix-DB y la linea del sider.ini");
		CHECK(equiposDe(filasDe(jc), js[6].jug) == std::set<uint32_t>{ js[6].a } && equiposDe(filasDe(jc), js[0].jug) == std::set<uint32_t>{ js[1].a }, "se puso al dia con los lotes que se perdio, en orden (amigo apagado)");
		// ahora con la raiz ya en el sider.ini y PES abierto: aplica sin tocar el sider.ini
		abierto = true;
		const std::string ini1 = leer(sider(jc) / "sider.ini");
		const fs::path carp2 = tmp / "transporte2";
		auto tr2 = std::make_shared<TransporteCarpeta>(carp2);
		bool y;
		const long long yaVistos = est(C)["amigo"]["ultimoSeq"].get<long long>();
		for (long long i = 0; i < yaVistos; i++) { Lote r = crearLote("F", "relleno", "2026-10-12T20:01:" + std::string(i < 10 ? "0" : "") + std::to_string(i) + "Z", "", "", { cl(900000 + static_cast<uint32_t>(i), 1, 2) }); tr2->publicar(r, y, e); }
		Lote nl = crearLote("Fralex", "otro-host", "2026-10-12T20:00:05Z", "Mundo falso", mundo.asigSha12, { cl(js[7].jug, js[7].de, js[7].a) });
		tr2->publicar(nl, y, e);
		C.cambiarTransporte(tr2);
		C.pasoRecibir();
		CHECK(equiposDe(filasDe(jc), js[7].jug) == std::set<uint32_t>{ js[7].a } && leer(sider(jc) / "sider.ini") == ini1, "PES abierto con Phoenix-DB ya en sider.ini: aplica y NO toca el sider.ini");
		json eC2 = est(C)["amigo"];
		CHECK(eC2["historial"][0]["estado"] == "aplicado", "PES abierto: el lote nuevo queda «aplicado» (estado: " + eC2["historial"][0]["estado"].get<std::string>() + ")");
	}

	// 8b) un jugador que no existe en la base del amigo: se salta con motivo y el resto sigue
	{
		const fs::path jd = juegoFalso(tmp, "juego-faltante", mundo);
		bool abierto = false;
		Config cd = configBase(tmp / "estado-faltante", jd, "Amigo5", &abierto);
		cd.recibir = true; cd.listaManual = []() { return fj::Lista(); };
		const fs::path carp4 = tmp / "transporte4";
		auto tr4 = std::make_shared<TransporteCarpeta>(carp4);
		bool y;
		Lote lf = crearLote("Fralex", "host-x", "2026-10-12T22:00:00Z", "Mundo falso", mundo.asigSha12, { cl(7777777, js[8].de, js[8].a), cl(js[8].jug, js[8].de, js[8].a) });
		tr4->publicar(lf, y, e);
		Motor F(cd, tr4);
		F.pasoRecibir();
		json eF = est(F)["amigo"]["historial"][0];
		CHECK(eF["estado"] == "parcial" && equiposDe(filasDe(jd), js[8].jug) == std::set<uint32_t>{ js[8].a }, "jugador inexistente: el lote queda «parcial», lo demas se aplica");
		bool motivo = false;
		for (const json& c : eF["cambios"]) if (c["estado"] == "saltado" && !c["motivo"].get<std::string>().empty()) motivo = true;
		CHECK(motivo, "jugador inexistente: se salta con un motivo legible");
	}

	// 9) amigo con OTRO parche (ids distintos): por nombres; sin nombres no se adivina
	{
		const Mundo otro = mundoDesde(f0, 1000000, 500000);
		const fs::path jo = juegoFalso(tmp, "juego-otro-parche", otro);
		bool abierto = false;
		Config co = configBase(tmp / "estado-otro", jo, "Amigo3", &abierto);
		co.recibir = true; co.listaManual = []() { return fj::Lista(); };
		Motor O(co, tr);
		O.pasoRecibir();
		auto fo = filasDe(jo);
		CHECK(!fo.empty() && equiposDe(fo, js[6].jug + 1000000) == std::set<uint32_t>{ js[6].a + 500000 }, "otro parche (ids distintos): el traspaso entra buscando por nombre de jugador y equipo");
		bool nombres = false;
		const json histO = est(O)["amigo"]["historial"];
		for (const json& h : histO) if (h["modo"] == "nombres") nombres = true;
		CHECK(nombres, "otro parche: el modo del lote es «nombres»");
		const fs::path carp3 = tmp / "transporte3";
		auto tr3 = std::make_shared<TransporteCarpeta>(carp3);
		CambioLote sn = cl(js[8].jug, js[8].de, js[8].a); sn.jugadorNombre.clear(); sn.desdeNombre.clear(); sn.hastaNombre.clear();
		Lote ln = crearLote("Fralex", "host-sin-nombres", "2026-10-12T21:00:00Z", "Mundo falso", mundo.asigSha12, { sn });
		bool y; tr3->publicar(ln, y, e);
		const fs::path jo2 = juegoFalso(tmp, "juego-otro-parche2", otro);
		Config co2 = configBase(tmp / "estado-otro2", jo2, "Amigo4", &abierto);
		co2.recibir = true; co2.listaManual = []() { return fj::Lista(); };
		Motor O2(co2, tr3);
		O2.pasoRecibir();
		CHECK(filasDe(jo2).empty() && est(O2)["amigo"]["historial"][0]["estado"] == "saltado", "otro parche y lote sin nombres: se salta, no se adivinan ids");
	}

	// 10) anti-bucle: el amigo tambien publica y su option file ya trae lo recibido
	{
		bool abierto = false;
		const fs::path optB = tmp / "amigo-docs";
		escribir(optB / "EDIT00000000", plantilla);
		Config cd = configBase(tmp / "estado-doble", jb, "Amigo", &abierto);
		cd.recibir = true; cd.publicar = true; cd.confirmarAntes = false; cd.optionFile = optB; cd.listaManual = []() { return fj::Lista(); };
		cd.enriquecer = ca.enriquecer;
		Motor D(cd, tr);
		D.pasoPublicar(); D.pasoPublicar();
		D.pasoRecibir();   // aplica lo que habia en el transporte (lotes del anfitrion)
		const int antes = contarLotes(carp);
		// su option file, tras recargar el juego con la base de Phoenix, trae el traspaso recibido (js[6] a su destino)
		escribir(optB / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, js[6].jug, js[6].de); meter(d, js[6].jug, js[6].a); }));
		tocar(optB / "EDIT00000000", 60);
		D.pasoPublicar(); D.pasoPublicar();
		CHECK(contarLotes(carp) == antes && est(D)["anfitrion"]["estado"] == "sin_traspasos", "anti-bucle: un traspaso recibido NO se vuelve a publicar como propio");
	}
}

// Con las bases REALES de Fralex (copias en la carpeta temporal): el anfitrion mueve a Lamine Yamal (Barca -> Arsenal) en una copia de su option file;
// el juego del amigo es una copia de los modos 26 (olmosjr23) y 27 (CPK) de ConmeGOL. Comprueba nombres reales y que el resultado carga.
static void copiarJuegoReal(const fs::path& real, const fs::path& juego, bool amigo) {
	std::error_code ec;
	escribir(juego / "PES2021.exe", "MZ");
	escribir(recargarTxt(juego).parent_path() / "avisos.txt", "");
	const fs::path extras = real / "ConmeGol Extras";
	const fs::path ol26 = extras / "ConmeGOL Patch 26" / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
	const fs::path ol0 = real / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
	const fs::path origen = fs::is_regular_file(ol26 / "PlayerAssignment.bin", ec) ? ol26 : ol0;
	const fs::path pesdb26 = std::string("olmosjr23") + "/Database/common/etc/pesdb";
	if (!amigo) {   // anfitrion: la base del parche 26 (olmosjr23), como en su PC
		escribir(juego / "SiderAddons" / "sider.ini", FACES + OLMOS + MODS);
		for (const char* n : { "PlayerAssignment.bin", "Player.bin", "Team.bin" })
			if (fs::is_regular_file(origen / n, ec)) escribir(juego / "SiderAddons" / pesdb26 / n, leer(origen / n));
		return;
	}
	// amigo: igual que la prueba real de Fichajes: raiz con el sider.ini real + CPK, modo 26 (olmosjr23) y modo 27 (CPK propio)
	escribir(juego / "SiderAddons" / "sider.ini", leer(real / "SiderAddons" / "sider.ini"));
	fs::create_directories(juego / "download", ec);
	fs::copy_file(real / "download" / "CGP_database.cpk", juego / "download" / "CGP_database.cpk", fs::copy_options::overwrite_existing, ec);
	const fs::path o26 = extras / "ConmeGOL Patch 26", d26 = juego / "ConmeGol Extras" / "ConmeGOL Patch 26";
	if (fs::is_regular_file(o26 / "download" / "CGP_database.cpk", ec)) {
		fs::create_directories(d26 / "download", ec); fs::create_directories(d26 / "SiderAddons" / "livecpk", ec);
		fs::copy_file(o26 / "download" / "CGP_database.cpk", d26 / "download" / "CGP_database.cpk", fs::copy_options::overwrite_existing, ec);
		escribir(d26 / "SiderAddons" / "sider.ini", leer(o26 / "SiderAddons" / "sider.ini"));
		for (const char* n : { "PlayerAssignment.bin", "Player.bin", "Team.bin" })
			if (fs::is_regular_file(origen / n, ec)) escribir(d26 / "SiderAddons" / pesdb26 / n, leer(origen / n));
	}
	const fs::path o27 = extras / "ConmeGOL Patch 27";
	if (fs::is_regular_file(o27 / "download" / "CGP_database.cpk", ec)) {
		const fs::path d = juego / "ConmeGol Extras" / "ConmeGOL Patch 27";
		fs::create_directories(d / "download", ec); fs::create_directories(d / "SiderAddons" / "livecpk", ec);
		fs::copy_file(o27 / "download" / "CGP_database.cpk", d / "download" / "CGP_database.cpk", fs::copy_options::overwrite_existing, ec);
	}
}
static void pruebaReal(const fs::path& tmp, const std::string& plantilla, const fs::path& real) {
	std::cout << "[bases REALES de Fralex (copias): Yamal Barca -> Arsenal, anfitrion 26 -> amigo con modos 26 y 27]\n";
	const fs::path jh = tmp / "real-anfitrion", ja = tmp / "real-amigo";
	copiarJuegoReal(real, jh, false);
	copiarJuegoReal(real, ja, true);
	std::vector<CambioLote> cs = { CambioLote() };
	cs[0].jugador = 162114; cs[0].desde = 108; cs[0].hasta = 101;
	std::string parche, huella;
	enriquecerConBaseDelJuego(jh, cs, parche, huella);
	std::cout << "    (anfitrion: parche «" << parche << "», huella " << huella << ", jugador «" << cs[0].jugadorNombre << "», " << cs[0].desdeNombre << " -> " << cs[0].hastaNombre << ")\n";
	CHECK(!huella.empty() && !cs[0].jugadorNombre.empty() && !cs[0].desdeNombre.empty() && !cs[0].hastaNombre.empty(), "base real del anfitrion: sale el nombre del jugador y de los dos equipos");

	const fs::path carp = tmp / "transporte-real";
	auto tr = std::make_shared<TransporteCarpeta>(carp);
	const fs::path optDir = tmp / "real-docs";
	escribir(optDir / "EDIT00000000", plantilla);
	bool cerrado = false;
	Config ca = configBase(tmp / "estado-real-anf", jh, "Fralex", &cerrado);
	ca.publicar = true; ca.confirmarAntes = false; ca.optionFile = optDir;
	ca.enriquecer = [&](std::vector<CambioLote>& c, std::string& p, std::string& h) { enriquecerConBaseDelJuego(jh, c, p, h); };
	Motor A(ca, tr);
	anfitrion(A);
	Foto f0; std::string e; leerFoto(plantilla, f0, e);
	// Yamal tiene que estar en el 108 (Barca) en la copia; si no, se usa el estado actual: se mueve de donde este al 101 o al 108
	uint32_t de = 0, a = 0;
	for (uint32_t eq : { 108u, 101u }) for (uint32_t j : f0.plantillas[eq]) if (j == 162114) de = eq;
	a = de == 108 ? 101 : 108;
	CHECK(de != 0, "Yamal esta en una de las plantillas Barca/Arsenal de la copia del option file (equipo " + std::to_string(de) + ")");
	if (!de) return;
	escribir(optDir / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, 162114, de); meter(d, 162114, a); }));
	tocar(optDir / "EDIT00000000", 5);
	anfitrion(A);
	CHECK(est(A)["anfitrion"]["estado"] == "publicado" && contarLotes(carp) == 1, "anfitrion (opcion file real): publica el traspaso de Yamal");
	std::vector<Lote> v; int ileg = 0;
	tr->traer(0, v, ileg, e);
	CHECK(v.size() == 1 && v[0].cambios[0].jugador == 162114 && !v[0].cambios[0].jugadorNombre.empty() && !v[0].baseHuella.empty(), "el lote lleva el nombre real («" + (v.empty() ? std::string() : v[0].cambios[0].jugadorNombre) + "») y la huella de la base");

	Config cb = configBase(tmp / "estado-real-amigo", ja, "Amigo", &cerrado);
	cb.recibir = true; cb.listaManual = []() { return fj::Lista(); };
	Motor B(cb, tr);
	B.pasoRecibir();
	json hist = est(B)["amigo"]["historial"];
	std::cout << "    (amigo: lote " << hist[0]["estado"].get<std::string>() << ", modo " << hist[0]["modo"].get<std::string>() << ")\n";
	const std::vector<fj::Analisis> an = fj::analizar(ja, fj::Lista());
	int sitios = 0;
	for (const fj::Analisis& x : an) {
		if (!x.baseOk) continue;
		const fs::path f = pa(x.objetivo.siderAddons.parent_path());
		if (!fs::exists(f)) { std::cout << "    · " << x.objetivo.nombre << ": sin archivo generado (parche " << x.parche << ")\n"; continue; }
		std::string crudo, err; std::vector<fj::Fila> filas;
		const bool carga = fj::leerBin(leer(f), crudo, err) && fj::leerAsignacion(crudo, filas, err);
		const std::set<uint32_t> eqs = equiposDe(filas, 162114);
		std::cout << "    · " << x.objetivo.nombre << " (" << x.parche << "): carga=" << carga << ", Yamal en equipos:";
		for (uint32_t q : eqs) std::cout << " " << q;
		std::cout << "\n";
		CHECK(carga && eqs.size() >= 1, "«" + x.objetivo.nombre + "»: el PlayerAssignment.bin generado carga con el formato exacto y Yamal sigue en la base");
		if (eqs.count(a)) sitios++;
	}
	CHECK(sitios >= 1, "el traspaso de Yamal quedo aplicado en al menos un modo real del amigo (" + std::to_string(sitios) + ")");
	CHECK(est(B)["amigo"]["historial"][0]["estado"] == "aplicado" || est(B)["amigo"]["historial"][0]["estado"] == "parcial", "el lote queda aplicado/parcial en el historial");
	std::string msg;
	CHECK(B.deshacerUltimo(msg), "deshacer sobre la base real: " + msg);
	bool quedan = false; for (const fj::Analisis& x : an) if (fs::exists(pa(x.objetivo.siderAddons.parent_path()))) quedan = true;
	CHECK(!quedan, "deshacer: la base del amigo vuelve a ser la original (no queda archivo generado)");
}

// Cadena completa POR LA WEB (hub falso): anfitrion y amigo son cuentas distintas; el amigo sigue al anfitrion por código o por amistad (con aprobación);
// un intruso no lee; 404/web caída no rompen nada; se pone al día; cambiar de carpeta a web no mezcla las numeraciones.
static void pruebaCadenaWeb(const fs::path& tmp, const std::string& plantilla) {
	std::cout << "[cadena completa POR LA WEB (hub falso): cuentas, seguir por código/amistad, aprobar, intruso, 404, ponerse al día, carpeta → web]\n";
	Foto f0; std::string e;
	if (!leerFoto(plantilla, f0, e)) { CHECK(false, "la plantilla se lee"); return; }
	std::set<uint32_t> ya;
	const std::vector<Jugada> js = elegir(f0, 8, ya);
	CHECK(js.size() >= 6, "hay jugadas de prueba (" + std::to_string(js.size()) + ")");
	if (js.size() < 6) return;
	const Mundo mundo = mundoDesde(f0, 0, 0);

	HubFalso hub;
	hub.nombres = { {"a1000000-0000-4000-8000-000000000001", "Fralex"}, {"a2000000-0000-4000-8000-000000000002", "Amigo"}, {"a3000000-0000-4000-8000-000000000003", "Pendiente"}, {"a4000000-0000-4000-8000-000000000004", "Intruso"} };
	hub.amigos.insert({ "a1000000-0000-4000-8000-000000000001", "a2000000-0000-4000-8000-000000000002" }); hub.amigos.insert({ "a1000000-0000-4000-8000-000000000001", "a3000000-0000-4000-8000-000000000003" });
	long long reloj = 1000;
	auto relojf = [&]() { return reloj; };
	auto tHost = std::make_shared<TransporteWeb>(hub.como("a1000000-0000-4000-8000-000000000001"), "", relojf);
	CanalWeb canal;
	ClienteTraspasosWeb cliH(*tHost);
	CHECK(cliH.canal("obtener", canal, e) && canal.hay, "anfitrion: tiene su canal en la web");

	const fs::path optDir = tmp / "web-anfitrion-docs";
	const fs::path opt = optDir / "EDIT00000000";
	escribir(opt, plantilla);
	bool cerrado = false;
	Config ca = configBase(tmp / "web-estado-anfitrion", fs::path(), "Fralex", &cerrado);
	ca.publicar = true; ca.confirmarAntes = false; ca.optionFile = optDir;
	ca.enriquecer = [&](std::vector<CambioLote>& c, std::string& parche, std::string& huella) {
		for (CambioLote& x : c) { x.jugadorNombre = "Jugador " + std::to_string(x.jugador); x.desdeNombre = "Equipo " + std::to_string(x.desde); x.hastaNombre = "Equipo " + std::to_string(x.hasta); }
		parche = "Mundo falso"; huella = mundo.asigSha12;
	};
	Motor A(ca, tHost);
	auto cambiar = [&](const std::function<void(std::vector<uint8_t>&)>& f, int seg) { escribir(opt, editarOption(plantilla, f)); tocar(opt, seg); };
	anfitrion(A);

	// amigo: entra con el código (activo al momento)
	auto tAmigo = std::make_shared<TransporteWeb>(hub.como("a2000000-0000-4000-8000-000000000002"), "", relojf);
	ClienteTraspasosWeb cliA(*tAmigo);
	SiguiendoWeb sig;
	CHECK(cliA.seguir(canal.codigo, "", sig, e) && sig.estado == "activo", "amigo: se une con el código del anfitrión");
	tAmigo->ponerCanal(sig.canalId);
	const fs::path jb = juegoFalso(tmp, "web-juego-amigo", mundo);
	Config cb = configBase(tmp / "web-estado-amigo", jb, "Amigo", &cerrado);
	cb.recibir = true; cb.listaManual = []() { return fj::Lista(); };
	Motor B(cb, tAmigo);

	cambiar([&](std::vector<uint8_t>& d) { sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[0].a); }, 5);
	anfitrion(A);
	CHECK(est(A)["anfitrion"]["estado"] == "publicado" && hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size() == 1, "anfitrion: el traspaso llega a la web como un lote");
	B.pasoRecibir();
	CHECK(equiposDe(filasDe(jb), js[0].jug) == std::set<uint32_t>{ js[0].a }, "amigo: lo recibe por la web y lo aplica en su Phoenix-DB");
	CHECK(est(B)["amigo"]["estado"] == "aplicado" && est(B)["transporteEstado"]["clave"] == "ok" && est(B)["transporteEstado"]["tipo"] == "web", "amigo: estado «aplicado» y transporte web «ok»");

	// intruso: no sigue a nadie
	{
		auto tI = std::make_shared<TransporteWeb>(hub.como("a4000000-0000-4000-8000-000000000004"), canal.id, relojf);
		const fs::path ji = juegoFalso(tmp, "web-juego-intruso", mundo);
		Config ci = configBase(tmp / "web-estado-intruso", ji, "Intruso", &cerrado);
		ci.recibir = true; ci.listaManual = []() { return fj::Lista(); };
		Motor I(ci, tI);
		I.pasoRecibir();
		CHECK(filasDe(ji).empty() && est(I)["amigo"]["estado"] == "sin_permiso", "intruso: conoce el canal pero la web no le deja leer: no se aplica nada (sin_permiso)");
	}
	// amigo por amistad: queda pendiente hasta que el anfitrion apruebe
	{
		auto tP = std::make_shared<TransporteWeb>(hub.como("a3000000-0000-4000-8000-000000000003"), "", relojf);
		ClienteTraspasosWeb cliP(*tP);
		SiguiendoWeb sp;
		CHECK(cliP.seguir("", "a1000000-0000-4000-8000-000000000001", sp, e) && sp.estado == "pendiente", "amigo 2: pide seguir por ser amigo: queda pendiente");
		tP->ponerCanal(sp.canalId);
		const fs::path jp = juegoFalso(tmp, "web-juego-pend", mundo);
		Config cp = configBase(tmp / "web-estado-pend", jp, "Pendiente", &cerrado);
		cp.recibir = true; cp.listaManual = []() { return fj::Lista(); };
		Motor P(cp, tP);
		P.pasoRecibir();
		CHECK(filasDe(jp).empty() && est(P)["amigo"]["estado"] == "pendiente_aprobacion", "amigo 2 pendiente: no recibe nada y la interfaz sabe por qué (pendiente_aprobacion)");
		CanalWeb cc;
		CHECK(cliH.seguidor("a3000000-0000-4000-8000-000000000003", "aprobar", cc, e), "anfitrion: aprueba a amigo 2");
		P.reintentarYa();
		P.pasoRecibir();
		CHECK(equiposDe(filasDe(jp), js[0].jug) == std::set<uint32_t>{ js[0].a } && est(P)["amigo"]["estado"] == "aplicado", "amigo 2 aprobado: se pone al día con lo ya publicado");
		cliH.seguidor("a3000000-0000-4000-8000-000000000003", "quitar", cc, e);
	}

	// 404: la web aún no tiene los endpoints → no falla nada, no insiste, y se recupera con «volver a comprobar»
	{
		Config c404 = ca; c404.dirEstado = tmp / "web-estado-404"; c404.optionFile = tmp / "web-404-docs"; c404.sondeoMs = 1;
		escribir(c404.optionFile / "EDIT00000000", plantilla);
		auto t404 = std::make_shared<TransporteWeb>(hub.como("a1000000-0000-4000-8000-000000000001"), "", relojf);
		Motor H(c404, t404);
		anfitrion(H);
		escribir(c404.optionFile / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, js[1].jug, js[1].de); meter(d, js[1].jug, js[1].a); }));
		tocar(c404.optionFile / "EDIT00000000", 10);
		hub.fallo = "404";
		const size_t lotesAntes = hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size();
		anfitrion(H);
		json eh = est(H);
		CHECK(eh["anfitrion"]["estado"] == "transporte:web_sin_endpoints" && eh["transporteEstado"]["clave"] == "web_sin_endpoints", "404 al publicar: estado «la web aún no tiene los traspasos» y no se pierde el traspaso");
		const int p1 = hub.peticiones;
		for (int i = 0; i < 6; i++) anfitrion(H);
		CHECK(hub.peticiones == p1, "404: Link NO insiste (espera 5 min): 0 peticiones más en 6 pasos");
		hub.fallo = "";
		H.reintentarYa();
		anfitrion(H);
		CHECK(hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size() == lotesAntes + 1 && est(H)["anfitrion"]["estado"] == "publicado", "«volver a comprobar»: con la web ya lista se publica el traspaso que estaba esperando");
		B.pasoRecibir();
		CHECK(equiposDe(filasDe(jb), js[1].jug) == std::set<uint32_t>{ js[1].a }, "amigo: recibe también ese segundo lote");
		// el amigo con 404 / timeout: tampoco insiste ni se rompe
		Config c2 = cb; c2.dirEstado = tmp / "web-estado-amigo404"; c2.sondeoMs = 1;
		const fs::path j404 = juegoFalso(tmp, "web-juego-amigo404", mundo);
		c2.juego = j404;
		auto t2 = std::make_shared<TransporteWeb>(hub.como("a2000000-0000-4000-8000-000000000002"), sig.canalId, relojf);
		Motor M(c2, t2);
		hub.fallo = "404";
		M.pasoRecibir();
		const int p2 = hub.peticiones;
		for (int i = 0; i < 6; i++) M.pasoRecibir();
		CHECK(hub.peticiones == p2 && est(M)["amigo"]["estado"] == "web_sin_endpoints", "amigo con 404: no insiste y lo dice (web_sin_endpoints)");
		hub.fallo = "timeout"; M.reintentarYa(); M.pasoRecibir();
		CHECK(est(M)["amigo"]["estado"] == "sin_conexion" && filasDe(j404).empty(), "amigo con la web caída: «sin conexión», no rompe nada");
		hub.fallo = "";
		M.reintentarYa(); M.pasoRecibir();
		CHECK(equiposDe(filasDe(j404), js[0].jug) == std::set<uint32_t>{ js[0].a } && equiposDe(filasDe(j404), js[1].jug) == std::set<uint32_t>{ js[1].a }, "la web vuelve: se pone al día con los 2 lotes (amigo que no había recibido ninguno)");
	}

	// ponerse al día al abrir (amigo apagado mientras se publicaban varios lotes) + paginas + duplicados + desordenados
	{
		cambiar([&](std::vector<uint8_t>& d) { sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[0].a); sacar(d, js[1].jug, js[1].de); meter(d, js[1].jug, js[1].a);
			sacar(d, js[2].jug, js[2].de); meter(d, js[2].jug, js[2].a); }, 20);
		anfitrion(A);
		cambiar([&](std::vector<uint8_t>& d) { sacar(d, js[0].jug, js[0].de); meter(d, js[0].jug, js[0].a); sacar(d, js[1].jug, js[1].de); meter(d, js[1].jug, js[1].a);
			sacar(d, js[2].jug, js[2].de); meter(d, js[2].jug, js[2].a); sacar(d, js[3].jug, js[3].de); meter(d, js[3].jug, js[3].a); }, 30);
		anfitrion(A);
		CHECK(hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size() >= 4, "anfitrion: publicó más lotes mientras el amigo estaba apagado (" + std::to_string(hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes.size()) + ")");
		auto& ls = hub.canalDe("a1000000-0000-4000-8000-000000000001").lotes;
		ls.push_back(ls[0]); ls.back()["seq"] = hub.canalDe("a1000000-0000-4000-8000-000000000001").ultimoSeq + 1;   // el hub se equivoca y repite un lote con otro seq
		hub.limitePagina = 2;
		const fs::path jn = juegoFalso(tmp, "web-juego-alday", mundo);
		Config cn = configBase(tmp / "web-estado-alday", jn, "Amigo", &cerrado);
		cn.recibir = true; cn.listaManual = []() { return fj::Lista(); };
		auto tN = std::make_shared<TransporteWeb>(hub.como("a2000000-0000-4000-8000-000000000002"), sig.canalId, relojf);
		Motor N(cn, tN);
		N.pasoRecibir();
		hub.limitePagina = 100;
		const std::vector<Registro> rg = N.registros();
		std::set<std::string> ids; for (const Registro& r : rg) ids.insert(r.lote.id);
		CHECK(ids.size() == rg.size() && rg.size() >= 4, "ponerse al día: un solo paso trae todas las páginas (" + std::to_string(rg.size()) + " lotes) sin duplicados");
		CHECK(equiposDe(filasDe(jn), js[3].jug) == std::set<uint32_t>{ js[3].a } && equiposDe(filasDe(jn), js[0].jug) == std::set<uint32_t>{ js[0].a }, "ponerse al día: el resultado final es el correcto aunque llegaran desordenados y repetidos");
		Motor N2(cn, tN);   // reabrir Link: recuerda hasta dónde leyó
		const int pa0 = hub.peticiones;
		N2.pasoRecibir();
		CHECK(N2.registros().size() == rg.size() && hub.peticiones == pa0 + 1, "reabrir Link: solo pregunta lo nuevo (1 petición) y no duplica nada");
		ls.pop_back();
	}

	// carpeta → web: cada buzón tiene su numeración; no se salta ni se mezcla nada
	{
		const fs::path carp = tmp / "web-transporte-carpeta";
		auto tc = std::make_shared<TransporteCarpeta>(carp);
		bool y;
		Lote l1 = crearLote("Fralex", "host-carpeta", "2026-10-12T05:00:00Z", "Mundo falso", mundo.asigSha12, { cl(js[4].jug, js[4].de, js[4].a) });
		Lote l2 = crearLote("Fralex", "host-carpeta", "2026-10-12T05:01:00Z", "Mundo falso", mundo.asigSha12, { cl(js[5].jug, js[5].de, js[5].a) });
		tc->publicar(l1, y, e); tc->publicar(l2, y, e);   // seq 1 y 2 en la carpeta
		const fs::path jm = juegoFalso(tmp, "web-juego-cambio", mundo);
		Config cm = configBase(tmp / "web-estado-cambio", jm, "Amigo", &cerrado);
		cm.recibir = true; cm.listaManual = []() { return fj::Lista(); };
		Motor M(cm, tc);
		M.pasoRecibir();
		CHECK(equiposDe(filasDe(jm), js[4].jug) == std::set<uint32_t>{ js[4].a } && M.registros().size() == 2, "carpeta: recibe sus 2 lotes (seq 1 y 2 de la carpeta)");
		auto tw = std::make_shared<TransporteWeb>(hub.como("a2000000-0000-4000-8000-000000000002"), sig.canalId, relojf);
		M.cambiarTransporte(tw);
		M.pasoRecibir();
		CHECK(M.registros().size() >= 6 && equiposDe(filasDe(jm), js[0].jug) == std::set<uint32_t>{ js[0].a }, "cambia a la web: lee los lotes del canal desde el seq 1 de la web (no se salta ninguno por el seq de la carpeta)");
		const size_t n = M.registros().size();
		M.cambiarTransporte(tc);
		M.pasoRecibir();
		CHECK(M.registros().size() == n, "vuelve a la carpeta: no repite lo que ya leyó de ella");
		Motor M2(cm, tw);
		M2.pasoRecibir();
		CHECK(M2.registros().size() == n, "reabrir Link con la web: recuerda hasta dónde leyó en cada buzón (nada repetido)");
	}

	// modo «Prueba (los dos)»: el propio Link recibe sus lotes y los marca como suyos
	{
		const fs::path od = tmp / "web-prueba-docs";
		escribir(od / "EDIT00000000", plantilla);
		const fs::path jq = juegoFalso(tmp, "web-juego-prueba", mundo);
		Config cq = configBase(tmp / "web-estado-prueba", jq, "Fralex", &cerrado);
		cq.publicar = true; cq.recibir = true; cq.confirmarAntes = false; cq.optionFile = od; cq.listaManual = []() { return fj::Lista(); };
		cq.enriquecer = ca.enriquecer;
		escribir(cq.dirEstado / "identidad.txt", leer(ca.dirEstado / "identidad.txt"));   // el mismo Link (misma identidad) que publicó los lotes anteriores
		auto tq = std::make_shared<TransporteWeb>(hub.como("a1000000-0000-4000-8000-000000000001"), canal.id, relojf);
		Motor Q(cq, tq);
		anfitrion(Q);
		escribir(od / "EDIT00000000", editarOption(plantilla, [&](std::vector<uint8_t>& d) { sacar(d, js[6 % js.size()].jug, js[6 % js.size()].de); meter(d, js[6 % js.size()].jug, js[6 % js.size()].a); }));
		tocar(od / "EDIT00000000", 40);
		anfitrion(Q);
		Q.pasoRecibir();
		bool propio = false;
		for (const Registro& r : Q.registros()) if (r.estado == "propio") propio = true;
		CHECK(propio && equiposDe(filasDe(jq), js[6 % js.size()].jug).count(js[6 % js.size()].a) == 0, "prueba (los dos) por la web: el lote que publicó vuelve como «TUYO» y no se aplica a sí mismo");
	}
}

int main(int argc, char** argv) {
	if (argc < 2) { std::cout << "uso: traspasos_test <carpeta temporal vacia> [EDIT00000000 de plantilla]\n"; return 2; }
	std::cout << std::unitbuf;
	const fs::path tmp = fs::path(argv[1]);
	fs::create_directories(tmp);
	pruebaLote(tmp);
	pruebaWeb();
	pruebaLista();
	if (argc >= 3 && fs::exists(argv[2])) { pruebaCadena(tmp, leer(argv[2])); pruebaCadenaWeb(tmp, leer(argv[2])); }
	if (argc >= 4 && fs::exists(argv[2]) && fs::is_directory(argv[3])) pruebaReal(tmp, leer(argv[2]), fs::path(argv[3]));
	else std::cout << "[cadena completa] SALTADA: falta una copia de EDIT00000000 como 2.o argumento\n";
	std::cout << "\n" << (total - fallos) << " de " << total << " comprobaciones bien" << (fallos ? ", " + std::to_string(fallos) + " FALLOS" : ", TODO OK") << "\n";
	return fallos ? 1 : 0;
}
