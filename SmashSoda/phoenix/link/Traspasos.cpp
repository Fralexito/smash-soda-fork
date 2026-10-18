#include "Traspasos.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <sstream>
#include <thread>

#include "BuzonJuego.h"
#include "Entrega.h"
#include "ModulosJuego.h"

namespace phoenix::traspasos {

using json = nlohmann::json;

namespace {
	bool leerTexto(const fs::path& p, std::string& t, size_t maximo = 64u * 1024u * 1024u) {
		std::ifstream f(p, std::ios::binary);
		if (!f) return false;
		t.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		return t.size() <= maximo;
	}
	bool escribirAtomico(const fs::path& destino, const std::string& contenido, std::string* error = nullptr) {
		std::error_code ec;
		fs::create_directories(destino.parent_path(), ec);
		fs::path tmp = destino; tmp += ".tmp";
		{
			std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
			f.write(contenido.data(), static_cast<std::streamsize>(contenido.size()));
			f.flush();
			if (!f.good()) { f.close(); fs::remove(tmp, ec); if (error) *error = "no se pudo escribir " + destino.string(); return false; }
		}
		for (int i = 0; i < 10; i++) {
			fs::rename(tmp, destino, ec);   // en Windows reemplaza el destino
			if (!ec) return true;
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		fs::remove(tmp, ec);
		if (error) *error = "no se pudo reemplazar " + destino.string() + " (" + ec.message() + ")";
		return false;
	}
	std::string sha(const std::string& s) { return phoenix::entrega::sha256Hex(s.data(), s.size()); }
	long long ahoraMs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

	json cambioAJson(const CambioLote& c) {
		return { {"jugador_id", c.jugador}, {"jugador", c.jugadorNombre}, {"desde_id", c.desde}, {"desde", c.desdeNombre}, {"hasta_id", c.hasta}, {"hasta", c.hastaNombre} };
	}
	bool entero(const json& j, const char* k, uint32_t& v) {
		if (!j.contains(k) || !j[k].is_number_integer()) return false;
		const long long n = j[k].get<long long>();
		if (n <= 0 || n > 0x7FFFFFFFLL) return false;
		v = static_cast<uint32_t>(n);
		return true;
	}
	std::string textoCorto(const json& j, const char* k, size_t max) {
		if (!j.contains(k) || !j[k].is_string()) return std::string();
		std::string t = j[k].get<std::string>();
		if (t.size() > max) t.resize(max);
		return t;
	}
	bool esHex(const std::string& s) { return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c) != 0; }); }
}

std::string horaUtcIso() {
	const std::time_t t = std::time(nullptr);
	std::tm g{};
#ifdef _WIN32
	gmtime_s(&g, &t);
#else
	gmtime_r(&t, &g);
#endif
	char b[32];
	std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%SZ", &g);
	return b;
}

// ─── Lote ────────────────────────────────────────────────────────────────────────────────────────
std::string huellaDeCambios(const std::vector<CambioLote>& c) {
	std::string t;
	for (const CambioLote& x : c)
		t += std::to_string(x.jugador) + "|" + std::to_string(x.desde) + "|" + std::to_string(x.hasta) + "|" + x.jugadorNombre + "|" + x.desdeNombre + "|" + x.hastaNombre + "\n";
	return sha(t);
}

Lote crearLote(const std::string& autor, const std::string& anfitrion, const std::string& creado, const std::string& parche,
	const std::string& baseHuella, std::vector<CambioLote> cambios) {
	std::sort(cambios.begin(), cambios.end(), [](const CambioLote& a, const CambioLote& b) { return a.jugador != b.jugador ? a.jugador < b.jugador : a.hasta < b.hasta; });
	for (CambioLote& x : cambios) {   // limites de la web: nombres <= 48; se recorta antes de la huella para que cuadre en todos lados
		x.jugadorNombre = phoenix::buzon::cortarUtf8(x.jugadorNombre, 48); x.desdeNombre = phoenix::buzon::cortarUtf8(x.desdeNombre, 48); x.hastaNombre = phoenix::buzon::cortarUtf8(x.hastaNombre, 48);
	}
	Lote l;
	l.autor = phoenix::buzon::cortarUtf8(autor, 40); l.anfitrion = anfitrion; l.creado = creado; l.parche = phoenix::buzon::cortarUtf8(parche, 60); l.baseHuella = phoenix::buzon::cortarUtf8(baseHuella, 64);
	l.cambios = std::move(cambios);
	l.huella = huellaDeCambios(l.cambios);
	l.id = sha(anfitrion + "|" + l.huella + "|" + creado).substr(0, 24);
	return l;
}

std::string loteAJson(const Lote& l, bool conSeq) {
	json j = { {"formato", l.formato}, {"id", l.id}, {"huella", l.huella}, {"creado", l.creado}, {"autor", l.autor}, {"anfitrion", l.anfitrion},
		{"parche", l.parche}, {"base_huella", l.baseHuella}, {"cambios", json::array()} };
	if (conSeq) j["seq"] = l.seq;
	if (l.oficial) { j["oficial"] = true; j["cambio_id"] = l.cambioId; }
	for (const CambioLote& c : l.cambios) j["cambios"].push_back(cambioAJson(c));
	return j.dump(1);
}

std::string loteAJsonWeb(const Lote& l) {
	json j = { {"id", l.id}, {"huella", l.huella}, {"creado", l.creado}, {"autor", l.autor}, {"parche", l.parche}, {"base_huella", l.baseHuella}, {"cambios", json::array()} };
	for (const CambioLote& c : l.cambios) j["cambios"].push_back(cambioAJson(c));
	return j.dump();
}

bool loteDeJson(const std::string& texto, Lote& l, std::string& error) {
	l = Lote();
	json j = json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) { error = "no es JSON"; return false; }
	if (j.contains("formato") && (!j["formato"].is_number_integer() || j["formato"].get<int>() != kFormatoLote)) { error = "formato de lote desconocido"; return false; }   // la web no manda «formato»
	l.formato = kFormatoLote;
	l.id = textoCorto(j, "id", 64); l.huella = textoCorto(j, "huella", 80);
	l.creado = textoCorto(j, "creado", 40); l.autor = textoCorto(j, "autor", 40);
	if (j.contains("anfitrion_id") && j["anfitrion_id"].is_string() && !j["anfitrion_id"].get<std::string>().empty()) { l.anfitrion = textoCorto(j, "anfitrion_id", 64); l.anfitrionNombre = textoCorto(j, "anfitrion", 60); }   // lote de la web
	else l.anfitrion = textoCorto(j, "anfitrion", 64);
	l.parche = textoCorto(j, "parche", 60); l.baseHuella = textoCorto(j, "base_huella", 64);
	if (j.contains("seq") && j["seq"].is_number_integer()) l.seq = j["seq"].get<long long>();
	l.oficial = j.contains("oficial") && j["oficial"].is_boolean() && j["oficial"].get<bool>();   // canal oficial de Phoenix (CONTRATO-WEB-PES §3a)
	if (l.oficial) {
		l.cambioId = textoCorto(j, "cambio_id", 36);
		const bool uuidOk = l.cambioId.size() == 36 && std::all_of(l.cambioId.begin(), l.cambioId.end(), [](unsigned char c) { return std::isxdigit(c) != 0 || c == '-'; });
		if (!uuidOk) l.cambioId.clear();   // sin id se aplica igual; solo no se puede informar su resultado
	}
	if (!esHex(l.id) || l.id.size() != 24) { error = "id de lote no valido"; return false; }
	if (l.anfitrion.empty()) { error = "falta el anfitrion"; return false; }
	if (!l.baseHuella.empty() && !esHex(l.baseHuella)) { error = "huella de base no valida"; return false; }
	if (!j.contains("cambios") || !j["cambios"].is_array() || j["cambios"].empty()) { error = "el lote no trae cambios"; return false; }
	if (j["cambios"].size() > 5000) { error = "demasiados cambios"; return false; }
	for (const json& c : j["cambios"]) {
		CambioLote x;
		const bool desdeCero = c.is_object() && c.contains("desde_id") && c["desde_id"].is_number_integer() && c["desde_id"].get<long long>() == 0;   // la web admite desde_id = 0 (sin equipo)
		if (!c.is_object() || !entero(c, "jugador_id", x.jugador) || !(desdeCero || entero(c, "desde_id", x.desde)) || !entero(c, "hasta_id", x.hasta)) { error = "un cambio no tiene ids validos"; return false; }
		if (x.desde == x.hasta) { error = "un cambio tiene el mismo equipo de origen y destino"; return false; }
		x.jugadorNombre = textoCorto(c, "jugador", 48); x.desdeNombre = textoCorto(c, "desde", 48); x.hastaNombre = textoCorto(c, "hasta", 48);
		l.cambios.push_back(x);
	}
	if (huellaDeCambios(l.cambios) != l.huella) { error = "la huella no coincide con el contenido (lote alterado o de otro orden)"; return false; }
	return true;
}

void enriquecerConBaseDelJuego(const fs::path& juego, std::vector<CambioLote>& cambios, std::string& parche, std::string& baseHuella) {
	if (juego.empty()) return;
	const fichajes::BaseNombres bn = fichajes::leerNombresDeBase(juego, std::string());
	if (!bn.ok) return;
	parche = bn.parche;
	baseHuella = bn.baseSha.substr(0, 12);
	for (CambioLote& c : cambios) {
		auto j = bn.jugadorLegible.find(c.jugador);
		if (j != bn.jugadorLegible.end()) c.jugadorNombre = j->second;
		auto d = bn.equipoLegible.find(c.desde);
		if (d != bn.equipoLegible.end()) c.desdeNombre = d->second;
		auto h = bn.equipoLegible.find(c.hasta);
		if (h != bn.equipoLegible.end()) c.hastaNombre = h->second;
	}
}

// ─── Transporte en carpeta ───────────────────────────────────────────────────────────────────────
namespace {
	// "00000012-<id>.json" -> 12 (0 si no es un nombre de lote)
	long long seqDeNombre(const std::string& n) {
		if (n.size() < 9 + 24 + 5 || n[8] != '-') return 0;
		for (int i = 0; i < 8; i++) if (n[i] < '0' || n[i] > '9') return 0;
		return std::stoll(n.substr(0, 8));
	}
}

bool TransporteCarpeta::publicar(Lote& l, bool& ya, std::string& error) {
	std::lock_guard<std::mutex> g(_m);
	ya = false;
	std::error_code ec;
	fs::create_directories(_carpeta, ec);
	if (ec) { error = "no se pudo crear la carpeta del transporte"; return false; }
	long long maximo = 0;
	for (const auto& e : fs::directory_iterator(_carpeta, ec)) {
		const std::string n = e.path().filename().string();
		const long long s = seqDeNombre(n);
		if (s == 0) continue;
		maximo = (std::max)(maximo, s);
		if (n.find(l.id) != std::string::npos) { l.seq = s; ya = true; return true; }
	}
	l.seq = maximo + 1;
	char pref[16];
	std::snprintf(pref, sizeof(pref), "%08lld", l.seq);
	return escribirAtomico(_carpeta / (std::string(pref) + "-" + l.id + ".json"), loteAJson(l, true), &error);
}

bool TransporteCarpeta::traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) {
	std::lock_guard<std::mutex> g(_m);
	lotes.clear(); ilegibles = 0;
	std::error_code ec;
	if (!fs::is_directory(_carpeta, ec)) return true;   // aun no hay nada
	std::vector<std::pair<long long, fs::path>> v;
	for (const auto& e : fs::directory_iterator(_carpeta, ec)) {
		const long long s = seqDeNombre(e.path().filename().string());
		if (s > despuesDe) v.push_back({ s, e.path() });
	}
	std::sort(v.begin(), v.end());
	for (const auto& [s, p] : v) {
		std::string t, e;
		Lote l;
		if (!leerTexto(p, t, 1024 * 1024) || !loteDeJson(t, l, e)) { ilegibles++; continue; }
		l.seq = s;   // manda el nombre del archivo
		lotes.push_back(std::move(l));
	}
	(void)error;
	return true;
}

// ─── Transporte por la web ───────────────────────────────────────────────────────────────────────
namespace {
	bool idSeguro(const std::string& s) {   // ids de la web (uuid): nada que se pueda colar en una peticion
		if (s.empty() || s.size() > 64) return false;
		return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c) || c == '-'; });
	}
	long long relojReal() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
	std::string cadena(const json& j, const char* k) { return j.is_object() && j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); }
}

TransporteWeb::TransporteWeb(PeticionWeb peticion, std::function<long long()> reloj) : _p(std::move(peticion)), _reloj(std::move(reloj)) {}

long long TransporteWeb::ahora() const { return _reloj ? _reloj() : relojReal(); }
long long TransporteWeb::esperaMs() const { std::lock_guard<std::mutex> g(_m); const long long r = _hastaMs - ahora(); return r > 0 ? r : 0; }

void TransporteWeb::marcar(const std::string& clave, const std::string& texto, long long espera) {
	_clave = clave; _texto = texto; _error = texto;
	if (espera > 0) _hastaMs = ahora() + espera;
}

bool TransporteWeb::pedir(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& respuesta) {
	estado = 0; respuesta.clear();
	std::lock_guard<std::mutex> g(_m);
	if (!_p) { marcar("sin_conexion", "Sin conexión con la web.", 10000); return false; }
	bool hubo = false;
	const auto t0 = std::chrono::steady_clock::now();
	try { hubo = _p(metodo, ruta, cuerpo, estado, respuesta); }
	catch (...) { hubo = false; estado = 0; }
	if (!hubo && estado == 0) {   // un fallo de red suelto (DNS, proxy, WinHTTP al arrancar) se reintenta UNA vez enseguida antes de avisar
		if (!_reloj) std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		try { hubo = _p(metodo, ruta, cuerpo, estado, respuesta); }
		catch (...) { hubo = false; estado = 0; }
	}
	_ultimaMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
	_maxMs = (std::max)(_maxMs, _ultimaMs); _peticiones++;
	if (!hubo || estado == 0 || estado >= 500) {   // se recuerda el fallo con su motivo tecnico (sin token ni cuerpos)
		_fallosTotales++;
		std::string det = respuesta.rfind("ERR:", 0) == 0 ? respuesta.substr(4, 80) : (estado >= 500 ? "HTTP " + std::to_string(estado) : std::string("sin detalle"));
		_ultimosFallos.push_back(horaUtcIso().substr(11, 8) + " " + metodo + " " + ruta.substr(0, ruta.find('?')) + " → " + det + " (" + std::to_string(_ultimaMs) + " ms)");
		if (_ultimosFallos.size() > 8) _ultimosFallos.erase(_ultimosFallos.begin());
	}
	const json j = json::parse(respuesta, nullptr, false);
	const std::string codigo = cadena(j, "codigo");
	std::string msg = cadena(j, "mensaje");
	if (msg.size() > 160) msg.resize(160);
	auto atras = [&](const char* clave, const std::string& texto) {   // red caida o 5xx: 10 s, 20 s, 40 s, 80 s, 120 s
		_fallos++;
		const long long e = (std::min)(120000LL, 10000LL << (std::min)(_fallos - 1, 4));
		marcar(clave, texto + " (reintento en " + std::to_string(e / 1000) + " s)", e);
	};
	if (estado == -1) { marcar("sin_cuenta", "Esta PC no está vinculada a una cuenta de la web: vincúlala en Sync › Cuenta web.", 15000); return false; }
	if (!hubo || estado == 0) {
		std::string detalle = respuesta.rfind("ERR:", 0) == 0 ? respuesta.substr(4) : std::string();
		if (detalle.size() > 80) detalle.resize(80);
		atras("sin_conexion", "No hay conexión con la web" + (detalle.empty() ? std::string() : " [" + detalle + "]") + ".");
		return false;
	}
	if (estado == 503 && codigo == "LIGA_PAUSADA") { marcar("liga_pausada", msg.empty() ? "La web tiene la liga en pausa." : msg, 60000); return false; }
	if (estado >= 500) { atras("sin_conexion", "La web tiene un problema (" + std::to_string(estado) + ")."); return false; }
	if (estado == 401 || codigo.rfind("TOKEN_", 0) == 0 || codigo == "DISPOSITIVO_SUSPENDIDO") {
		marcar("sin_cuenta", "La web no reconoce este Link: vuelve a vincular tu cuenta (Sync › Cuenta web).", 60000); return false;
	}
	if (estado == 404 && (codigo.empty() || codigo == "RUTA_NO_EXISTE")) {
		marcar("web_sin_endpoints", "La web aún no tiene los traspasos en vivo (se está desplegando): Link vuelve a probar solo cada 5 minutos.", 300000); return false;
	}
	if (estado == 429 || codigo == "DEMASIADOS_INTENTOS") {
		long long s = j.is_object() && j.contains("reintentar_en") && j["reintentar_en"].is_number() ? j["reintentar_en"].get<long long>() : 30;
		s = (std::max)(5LL, (std::min)(600LL, s));
		marcar("limite", "Demasiadas peticiones: espero " + std::to_string(s) + " s.", s * 1000); return false;
	}
	if (codigo == "HOST_NO_AUTORIZADO" || codigo == "SIN_PERMISO_TRASPASOS" || estado == 403) {
		marcar("sin_permiso", msg.empty() ? "Esta cuenta no está aprobada como anfitrión." : msg, 60000); return false;
	}
	const bool bueno = (estado == 200 || estado == 201) && j.is_object() && !(j.contains("ok") && j["ok"].is_boolean() && !j["ok"].get<bool>());
	if (!bueno) {
		// error de la accion (codigo incorrecto, seguirse a uno mismo, datos invalidos…): se lo decimos al usuario sin tocar el estado del transporte
		if (j.is_object() && !codigo.empty()) { _error = msg.empty() ? codigo : msg; }
		else { marcar("respuesta_rara", "La respuesta de la web no se entiende (" + std::to_string(estado) + ").", 30000); }
		return false;
	}
	_fallos = 0; _clave = "ok"; _texto.clear(); _error.clear();
	return true;
}

json TransporteWeb::tecnico() const {
	std::lock_guard<std::mutex> g(_m);
	return { {"ultimaMs", _ultimaMs}, {"maxMs", _maxMs}, {"peticiones", _peticiones}, {"fallos", _fallosTotales}, {"ultimosFallos", _ultimosFallos} };
}

bool TransporteWeb::publicar(Lote& l, bool& ya, std::string& error) {
	ya = false;
	int estado = 0; std::string r;
	if (!pedir("POST", "/traspasos", loteAJsonWeb(l), estado, r)) { error = ultimoError(); return false; }
	const json j = json::parse(r, nullptr, false);
	if (!j.contains("seq") || !j["seq"].is_number_integer() || j["seq"].get<long long>() <= 0) {
		std::lock_guard<std::mutex> g(_m); marcar("respuesta_rara", "La respuesta de la web al publicar no se entiende.", 30000); error = _texto; return false;
	}
	l.seq = j["seq"].get<long long>();
	ya = j.contains("repetido") && j["repetido"].is_boolean() && j["repetido"].get<bool>();
	return true;
}

bool TransporteWeb::traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) {
	lotes.clear(); ilegibles = 0;
	{
		std::lock_guard<std::mutex> g(_m);
		_hayMas = false;
		if (!_puedeLeer) { marcar("sin_anfitrion", "Elige a qué anfitrión sigues (escribe su código).", 0); error = _texto; return false; }
	}
	constexpr int kPagina = 30;
	int estado = 0; std::string r;
	bool solo;
	{ std::lock_guard<std::mutex> g(_m); solo = _soloOficial; }
	if (!pedir("GET", "/traspasos?desde=" + std::to_string(despuesDe) + "&limite=" + std::to_string(kPagina) + (solo ? "&solo_oficial=1" : ""), "", estado, r)) { error = ultimoError(); return false; }
	const json j = json::parse(r, nullptr, false);
	if (!j.contains("lotes") || !j["lotes"].is_array()) {
		std::lock_guard<std::mutex> g(_m); marcar("respuesta_rara", "La respuesta de la web no se entiende.", 30000); error = _texto; return false;
	}
	size_t recibidos = 0;
	for (const json& x : j["lotes"]) {
		recibidos++;
		Lote l; std::string e;
		if (!loteDeJson(x.dump(), l, e) || l.seq <= despuesDe) { ilegibles++; continue; }   // alterado, corrupto o ya visto
		lotes.push_back(std::move(l));
	}
	std::sort(lotes.begin(), lotes.end(), [](const Lote& a, const Lote& b) { return a.seq < b.seq; });
	std::lock_guard<std::mutex> g(_m);
	_hayMas = !lotes.empty();   // si llegaron lotes se vuelve a preguntar enseguida: asi se trae todo aunque la web pagine mas corto de lo pedido (la ultima respuesta vacia corta el bucle)
	return true;
}

// ─── Mi codigo / a quien sigo ────────────────────────────────────────────────────────────────────
bool ClienteTraspasosWeb::llamar(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, json& j, std::string& error) {
	int estado = 0; std::string r;
	if (!_t.pedir(metodo, ruta, cuerpo, estado, r)) { error = _t.ultimoError(); return false; }
	j = json::parse(r, nullptr, false);
	if (!j.is_object()) { error = "La respuesta de la web no se entiende."; return false; }
	return true;
}
namespace {
	std::string nombreCorto(const json& o, const char* k) { std::string t = cadena(o, k); if (t.size() > 60) t.resize(60); return t; }
}
bool ClienteTraspasosWeb::codigo(bool rotar, CanalWeb& c, std::string& error) {
	c = CanalWeb();
	json j;
	if (!llamar("POST", "/traspasos/codigo", json{ {"rotar", rotar} }.dump(), j, error)) return false;
	const std::string cod = cadena(j, "codigo");
	if (cod.empty() || cod.size() > 16 || !std::all_of(cod.begin(), cod.end(), [](unsigned char x) { return std::isalnum(x) != 0; })) { error = "La respuesta de la web no trae un código válido."; return false; }
	c.codigo = cod;
	if (j.contains("seguidores") && j["seguidores"].is_array())
		for (const json& s : j["seguidores"]) { const std::string n = nombreCorto(s, "nombre"); if (!n.empty() && c.seguidores.size() < 100) c.seguidores.push_back(n); }
	c.hay = true;
	return true;
}
bool ClienteTraspasosWeb::seguir(const std::string& codigo, bool prueba, SiguiendoWeb& s, std::string& error) {
	s = SiguiendoWeb();
	if (codigo.empty() || codigo.size() > 16 || !std::all_of(codigo.begin(), codigo.end(), [](unsigned char c) { return std::isalnum(c) || c == '-'; })) { error = "El código no es válido."; return false; }
	json cuerpo = { {"codigo", codigo} };
	if (prueba) cuerpo["prueba"] = true;
	json j;
	if (!llamar("POST", "/traspasos/seguir", cuerpo.dump(), j, error)) return false;
	s.anfitrionId = cadena(j, "anfitrion_id"); s.anfitrionNombre = nombreCorto(j, "anfitrion");
	if (!idSeguro(s.anfitrionId)) { error = "La respuesta de la web no trae el anfitrión."; return false; }
	return true;
}
bool ClienteTraspasosWeb::siguiendo(std::vector<SiguiendoWeb>& sigo, std::vector<std::string>& meSiguen, std::string& error) {
	sigo.clear(); meSiguen.clear();
	json j;
	if (!llamar("GET", "/traspasos/siguiendo", "", j, error)) return false;
	if (!j.contains("sigo") || !j["sigo"].is_array()) { error = "La respuesta de la web no se entiende."; return false; }
	for (const json& s : j["sigo"]) { SiguiendoWeb x; x.anfitrionId = cadena(s, "anfitrion_id"); x.anfitrionNombre = nombreCorto(s, "nombre"); if (idSeguro(x.anfitrionId) && sigo.size() < 20) sigo.push_back(x); }
	if (j.contains("me_siguen") && j["me_siguen"].is_array())
		for (const json& s : j["me_siguen"]) { const std::string n = nombreCorto(s, "nombre"); if (!n.empty() && meSiguen.size() < 100) meSiguen.push_back(n); }
	return true;
}
bool ClienteTraspasosWeb::dejar(const std::string& anfitrionId, std::string& error) {
	if (!idSeguro(anfitrionId)) { error = "datos no validos"; return false; }
	json j;
	return llamar("POST", "/traspasos/dejar", json{ {"anfitrion_id", anfitrionId} }.dump(), j, error);
}

// ─── Ajustes guardados ───────────────────────────────────────────────────────────────────────────
AjustesGuardados leerAjustes(const std::string& texto) {
	AjustesGuardados a;
	const json j = json::parse(texto, nullptr, false);
	if (!j.is_object()) return a;
	auto b = [&](const char* k, bool def) { return j.contains(k) && j[k].is_boolean() ? j[k].get<bool>() : def; };
	auto s = [&](const char* k, size_t max) { std::string t = j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); if (t.size() > max) t.clear(); return t; };
	a.publicar = b("publicar", false); a.recibir = b("recibir", false); a.confirmarAntes = b("confirmarAntes", true);
	a.prueba = b("prueba", false); a.aprobarRecibidos = b("aprobarRecibidos", false); a.aplicarPropios = b("aplicarPropios", true);
	a.oficial = b("oficial", true);   // los cambios oficiales de Phoenix llegan solos salvo que el usuario los apague
	a.transporte = s("transporte", 12); a.carpeta = s("carpeta", 500); a.autor = s("autor", 40); a.optionFile = s("optionFile", 500);
	if (a.transporte != "web" && a.transporte != "carpeta") a.transporte = "web";
	if (a.transporte == "carpeta" && a.carpeta.empty()) { a.transporte = "web"; a.migrado = true; }   // prueba vieja con carpeta vacia: la web
	return a;
}

// ─── Autodiagnostico del juego ───────────────────────────────────────────────────────────────────
DiagJuego diagnosticarJuego(const fs::path& juego, const fs::path& paquete) {
	DiagJuego d;
	if (juego.empty()) return d;
	std::error_code ec;
	if (!fs::is_directory(juego, ec)) { d.juego = juego.string(); return d; }
	d.ok = true; d.juego = juego.string();
	for (const fichajes::Objetivo& o : fichajes::objetivos(juego)) {
		SitioDiag s;
		s.nombre = o.nombre;
		std::string ini;
		s.sider = leerTexto(o.ini, ini, 4u * 1024u * 1024u);
		s.raiz = s.sider && phoenix::modulos::tieneRaizPhoenixDB(ini);
		const fs::path phx = o.siderAddons / "livecpk" / "Phoenix-DB";
		s.carpeta = fs::is_directory(phx, ec);
		s.archivo = fs::is_regular_file(phx / "common" / "etc" / "pesdb" / "PlayerAssignment.bin", ec);
		d.sitios.push_back(s);
	}
	for (const phoenix::modulos::Modulo& m : phoenix::modulos::estado(paquete, juego)) {
		if (m.archivo != "phoenix.lua") continue;
		d.luaEstado = m.estado.empty() ? "no_instalado" : m.estado;
		d.luaVersionLink = m.version;
		for (const phoenix::modulos::Destino& x : m.destinos) if (x.archivo && d.luaVersion.empty()) d.luaVersion = x.version;
	}
	return d;
}

json diagAJson(const DiagJuego& d) {
	json s = json::array();
	for (const SitioDiag& x : d.sitios) s.push_back({ {"nombre", x.nombre}, {"sider", x.sider}, {"raiz", x.raiz}, {"carpeta", x.carpeta}, {"archivo", x.archivo} });
	return { {"ok", d.ok}, {"juego", d.juego}, {"sitios", s}, {"lua", { {"estado", d.luaEstado}, {"version", d.luaVersion}, {"versionLink", d.luaVersionLink} }} };
}

// ─── Lista efectiva ──────────────────────────────────────────────────────────────────────────────
namespace {
	/// Orden de aplicacion: por buzon (en orden de aparicion) y dentro de cada buzon por seq. Sin buzon anotado, por seq.
	long long claveOrden(const Registro& r) { return r.orden ? r.orden : r.lote.seq; }
}

/// Lotes que NO cuentan para el juego: deshechos, propios, con error, sin aprobar todavía o rechazados.
static bool loteInactivo(const std::string& e) {
	return e == "deshecho" || e == "propio" || e == "error" || e == "por_aprobar" || e == "rechazado" || e == "ignorado";
}

fichajes::Lista construirLista(const fichajes::Lista& manual, const std::vector<Registro>& registros) {
	fichajes::Lista l = manual;
	l.version = fichajes::kVersionLista;
	l.fuente = "Traspasos en vivo" + std::string(manual.cambios.empty() ? "" : " + lista manual");
	std::vector<const Registro*> activos;
	for (const Registro& r : registros) {
		if (loteInactivo(r.estado) || r.modo == "nada") continue;
		activos.push_back(&r);
	}
	std::stable_sort(activos.begin(), activos.end(), [](const Registro* a, const Registro* b) { return claveOrden(*a) < claveOrden(*b); });
	// jugador -> primer origen y ultimo destino (los nombres y el modo, los del ultimo lote)
	struct Acum { uint32_t desde = 0, hasta = 0; std::string desdeNombre; const Registro* ultimo = nullptr; const CambioLote* c = nullptr; };
	std::map<uint32_t, Acum> m;
	std::vector<uint32_t> orden;
	for (const Registro* r : activos) {
		for (const CambioLote& c : r->lote.cambios) {
			auto it = m.find(c.jugador);
			if (it == m.end()) { m[c.jugador] = { c.desde, c.hasta, c.desdeNombre, r, &c }; orden.push_back(c.jugador); }
			else { it->second.hasta = c.hasta; it->second.ultimo = r; it->second.c = &c; }
		}
	}
	for (uint32_t j : orden) {
		const Acum& a = m[j];
		if (a.desde == a.hasta) continue;   // fue y volvio: no hay nada que aplicar
		fichajes::Cambio c;
		c.nota = std::to_string(j);   // para reconocer el resultado en los informes de Fichajes
		if (a.ultimo->modo == "nombres") {
			c.jugadorNombre = a.c->jugadorNombre;
			c.equipoNombre = a.c->hastaNombre;
			c.origenNombre = a.desdeNombre;
		}
		else { c.jugador = j; c.equipo = a.hasta; c.origen = a.desde; }
		l.cambios.push_back(c);
	}
	std::string t;
	for (const fichajes::Cambio& c : l.cambios)
		t += std::to_string(c.jugador) + "|" + std::to_string(c.equipo) + "|" + std::to_string(c.origen) + "|" + c.jugadorNombre + "|" + c.equipoNombre + "|" + c.origenNombre + "\n";
	l.sha256 = sha(t);
	return l;
}

// ─── Motor ───────────────────────────────────────────────────────────────────────────────────────
Motor::Motor(Config c, std::shared_ptr<Transporte> t) : _c(std::move(c)), _t(std::move(t)) { cargarEstado(); fijarOrigen(); }

void Motor::fijarOrigen() {
	if (!_t) return;
	const std::string o = _t->identidad();
	if (o == _origenActual) return;
	if (!_origenActual.empty()) _seqPorOrigen[_origenActual] = _ultimoSeq;
	_origenActual = o;
	if (std::find(_origenes.begin(), _origenes.end(), o) == _origenes.end()) _origenes.push_back(o);
	const auto it = _seqPorOrigen.find(o);
	_ultimoSeq = it == _seqPorOrigen.end() ? 0 : it->second;
	_proximoTraerMs = 0;
}

long long Motor::ordenDe(const std::string& origen, long long seq) {
	auto it = std::find(_origenes.begin(), _origenes.end(), origen);
	if (it == _origenes.end()) { _origenes.push_back(origen); it = _origenes.end() - 1; }
	return (static_cast<long long>(it - _origenes.begin() + 1) << 40) + seq;
}

void Motor::reintentarYa() {
	std::lock_guard<std::mutex> g(_m);
	_proximoTraerMs = 0; _proximoPublicarMs = 0; _reintentoAplicar = 0;
	if (_t) _t->reintentarYa();
}

void Motor::cambiarTransporte(std::shared_ptr<Transporte> t) { std::lock_guard<std::mutex> g(_m); _t = std::move(t); fijarOrigen(); }
void Motor::ajustar(const fs::path& juego, const fs::path& optionFile, bool publicar, bool recibir, bool confirmarAntes, const std::string& autor) {
	std::lock_guard<std::mutex> g(_m);
	_c.juego = juego; _c.optionFile = optionFile; _c.publicar = publicar; _c.recibir = recibir; _c.confirmarAntes = confirmarAntes;
	if (!autor.empty()) _c.autor = autor;
}
std::string Motor::ahora() const { return _c.reloj ? _c.reloj() : horaUtcIso(); }
std::vector<Registro> Motor::registros() const { std::lock_guard<std::mutex> g(_m); return _reg; }

void Motor::guardarFoto(const Foto& f, const char* nombre) const {
	std::string t = f.sha256 + "\n";
	for (const auto& e : f.equipos) t += "N " + std::to_string(e.first) + " " + e.second + "\n";
	for (const auto& p : f.plantillas) {
		t += "T " + std::to_string(p.first);
		for (uint32_t j : p.second) t += " " + std::to_string(j);
		t += "\n";
	}
	escribirAtomico(_c.dirEstado / nombre, t);
}
bool Motor::cargarFoto(Foto& f, const char* nombre) const {
	std::string t;
	if (!leerTexto(_c.dirEstado / nombre, t, 64u * 1024u * 1024u)) return false;
	f = Foto();
	std::istringstream in(t);
	std::string linea;
	if (!std::getline(in, f.sha256)) return false;
	while (std::getline(in, linea)) {
		if (!linea.empty() && linea.back() == '\r') linea.pop_back();
		if (linea.size() < 3) continue;
		std::istringstream ls(linea.substr(2));
		uint32_t id = 0;
		ls >> id;
		if (linea[0] == 'N') { std::string n; std::getline(ls, n); if (!n.empty() && n[0] == ' ') n.erase(0, 1); f.equipos[id] = n; }
		else if (linea[0] == 'T') { uint32_t j; auto& v = f.plantillas[id]; while (ls >> j) v.push_back(j); }
	}
	return !f.plantillas.empty();
}

void Motor::guardarRegistros() const {
	std::map<std::string, long long> porOrigen = _seqPorOrigen;
	if (!_origenActual.empty()) porOrigen[_origenActual] = _ultimoSeq;
	json j = { {"version", 2}, {"seqPorOrigen", porOrigen}, {"origenes", _origenes}, {"registros", json::array()} };
	for (const Registro& r : _reg) {
		json d = json::array();
		for (const Detalle& x : r.detalle) d.push_back({ {"jugador", x.jugador}, {"desde", x.desde}, {"hasta", x.hasta}, {"jugadorNombre", x.jugadorNombre}, {"desdeNombre", x.desdeNombre}, {"hastaNombre", x.hastaNombre}, {"estado", x.estado}, {"motivo", x.motivo} });
		j["registros"].push_back({ {"lote", json::parse(loteAJson(r.lote, true))}, {"estado", r.estado}, {"propio", r.propio}, {"modo", r.modo}, {"recibido", r.recibido}, {"aplicadoEn", r.aplicadoEn}, {"motivo", r.motivo}, {"origen", r.origen}, {"orden", r.orden}, {"detalle", d} });
	}
	escribirAtomico(_c.dirEstado / "recibidos.json", j.dump(1));
}

void Motor::guardarPendientes() const {
	if (_pendientes.empty()) {
		std::error_code ec;
		fs::remove(_c.dirEstado / "pendientes.json", ec);
		fs::remove(_c.dirEstado / "foto_pendiente.txt", ec);
		return;
	}
	json c = json::array();
	for (const CambioLote& x : _pendientes) c.push_back(cambioAJson(x));
	escribirAtomico(_c.dirEstado / "pendientes.json", json{ {"masivo", _masivo}, {"parche", _parchePend}, {"base_huella", _huellaPend}, {"cambios", c} }.dump(1));
	guardarFoto(_pend, "foto_pendiente.txt");
}

void Motor::cargarEstado() {
	std::error_code ec;
	fs::create_directories(_c.dirEstado, ec);
	// identidad estable de este Link
	std::string id;
	if (!leerTexto(_c.dirEstado / "identidad.txt", id, 200) || id.size() < 8) {
		std::random_device rd;
		std::ostringstream o;
		for (int i = 0; i < 4; i++) { char b[9]; std::snprintf(b, sizeof(b), "%08x", rd()); o << b; }
		id = o.str();
		escribirAtomico(_c.dirEstado / "identidad.txt", id);
	}
	while (!id.empty() && (id.back() == '\n' || id.back() == '\r' || id.back() == ' ')) id.pop_back();
	_anfitrionId = id;
	_hayBase = cargarFoto(_base, "foto.txt");
	std::string t;
	if (leerTexto(_c.dirEstado / "recibidos.json", t, 32u * 1024u * 1024u)) {
		json j = json::parse(t, nullptr, false);
		if (j.is_object() && j.contains("registros") && j["registros"].is_array()) {
			if (j.contains("seqPorOrigen") && j["seqPorOrigen"].is_object())
				for (auto it = j["seqPorOrigen"].begin(); it != j["seqPorOrigen"].end(); ++it) if (it.value().is_number_integer()) _seqPorOrigen[it.key()] = it.value().get<long long>();
			if (j.contains("origenes") && j["origenes"].is_array())
				for (const json& o : j["origenes"]) if (o.is_string()) _origenes.push_back(o.get<std::string>());
			for (const json& r : j["registros"]) {
				Registro x;
				std::string e;
				if (!r.contains("lote") || !loteDeJson(r["lote"].dump(), x.lote, e)) continue;
				x.estado = r.value("estado", std::string("pendiente")); x.modo = r.value("modo", std::string("ids"));
				x.propio = r.contains("propio") && r["propio"].is_boolean() ? r["propio"].get<bool>() : x.estado == "propio";
				x.origen = r.value("origen", std::string()); x.orden = r.value("orden", 0LL);
				x.recibido = r.value("recibido", std::string()); x.aplicadoEn = r.value("aplicadoEn", std::string()); x.motivo = r.value("motivo", std::string());
				if (r.contains("detalle") && r["detalle"].is_array())
					for (const json& d : r["detalle"]) {
						Detalle y;
						y.jugador = d.value("jugador", 0u); y.desde = d.value("desde", 0u); y.hasta = d.value("hasta", 0u);
						y.jugadorNombre = d.value("jugadorNombre", std::string()); y.desdeNombre = d.value("desdeNombre", std::string()); y.hastaNombre = d.value("hastaNombre", std::string());
						y.estado = d.value("estado", std::string()); y.motivo = d.value("motivo", std::string());
						x.detalle.push_back(y);
					}
				_reg.push_back(std::move(x));
			}
		}
	}
	if (leerTexto(_c.dirEstado / "pendientes.json", t, 4u * 1024u * 1024u)) {
		json j = json::parse(t, nullptr, false);
		Foto f;
		if (j.is_object() && j.contains("cambios") && j["cambios"].is_array() && cargarFoto(f, "foto_pendiente.txt")) {
			_masivo = j.value("masivo", false); _parchePend = j.value("parche", std::string()); _huellaPend = j.value("base_huella", std::string());
			for (const json& c : j["cambios"]) {
				CambioLote x;
				x.jugador = c.value("jugador_id", 0u); x.desde = c.value("desde_id", 0u); x.hasta = c.value("hasta_id", 0u);
				x.jugadorNombre = c.value("jugador", std::string()); x.desdeNombre = c.value("desde", std::string()); x.hastaNombre = c.value("hasta", std::string());
				if (x.jugador && x.desde && x.hasta) _pendientes.push_back(x);
			}
			if (!_pendientes.empty()) _pend = f;
		}
	}
}

void Motor::reiniciarReferencia() {
	std::lock_guard<std::mutex> g(_m);
	_hayBase = false; _base = Foto(); _pendientes.clear(); _masivo = false; _firmaVista.clear(); _firmaProcesada.clear();
	std::error_code ec;
	fs::remove(_c.dirEstado / "foto.txt", ec);
	guardarPendientes();
	_estadoAnfitrion = "referencia_borrada";
}

std::string Motor::firmaOptionFile(fs::path& archivo) const {
	std::error_code ec;
	archivo = _c.optionFile;
	if (archivo.empty()) return std::string();
	if (fs::is_directory(archivo, ec)) archivo /= "EDIT00000000";
	if (!fs::is_regular_file(archivo, ec)) return std::string();
	const auto tam = fs::file_size(archivo, ec);
	if (ec) return std::string();
	const auto t = fs::last_write_time(archivo, ec);
	if (ec) return std::string();
	return std::to_string(tam) + ":" + std::to_string(t.time_since_epoch().count());
}

bool Motor::publicarLote(const std::vector<CambioLote>& cambios, const std::string& parche, const std::string& huella, std::string& mensaje) {
	if (!_t) { mensaje = "No hay transporte configurado."; return false; }
	Lote l = crearLote(_c.autor, _anfitrionId, ahora(), parche, huella, cambios);
	bool ya = false;
	std::string e;
	if (!_t->publicar(l, ya, e)) { mensaje = "No se pudo publicar: " + e; return false; }
	_publicados++;
	_ultimoPublicado = l.id;
	// el anfitrion tambien guarda lo que publico; con «mantener mis fichajes» tambien se pone en su propio juego (Phoenix-DB)
	bool conocido = false;
	for (const Registro& r : _reg) if (r.lote.id == l.id) conocido = true;
	if (!conocido) {
		Registro r;
		r.lote = l; r.propio = true; r.estado = _c.aplicarPropios ? "pendiente" : "propio"; r.modo = _c.aplicarPropios ? "ids" : "nada"; r.recibido = ahora(); r.origen = _origenActual; r.orden = ordenDe(_origenActual, l.seq);
		for (const CambioLote& c : l.cambios) r.detalle.push_back({ c.jugador, c.desde, c.hasta, c.jugadorNombre, c.hastaNombre, "publicado", "" });
		_reg.push_back(std::move(r));
		guardarRegistros();
	}
	mensaje = std::to_string(l.cambios.size()) + (l.cambios.size() == 1 ? " traspaso publicado" : " traspasos publicados") + " (lote " + l.id.substr(0, 8) + ").";
	return true;
}

void Motor::pasoPublicar() {
	std::lock_guard<std::mutex> g(_m);
	if (!_c.publicar) { _estadoAnfitrion = "apagado"; return; }
	if (ahoraMs() < _proximoPublicarMs) return;   // el transporte pidio esperar (web caida, 404, 429…)
	fs::path archivo;
	const std::string firma = firmaOptionFile(archivo);
	_ultimaRevision = ahora();
	if (firma.empty()) { _estadoAnfitrion = "sin_option_file"; _firmaVista.clear(); return; }
	if (firma != _firmaVista) { _firmaVista = firma; if (firma != _firmaProcesada) _estadoAnfitrion = "esperando_que_termine_de_guardar"; return; }   // aun cambiando: se espera al siguiente paso
	if (firma == _firmaProcesada) return;                                                                                                  // ya se miro este archivo
	Foto nueva;
	std::string error;
	if (!leerFotoArchivo(archivo, nueva, error)) { _estadoAnfitrion = "no_se_pudo_leer: " + error; return; }   // se reintenta en el siguiente paso
	auto cerrar = [&](const char* estado) { _estadoAnfitrion = estado; _firmaProcesada = firma; };
	if (!_hayBase) {
		_base = nueva; _hayBase = true;
		guardarFoto(_base, "foto.txt");
		cerrar("referencia_guardada");
		return;
	}
	if (nueva.sha256 == _base.sha256) { cerrar("sin_cambios"); return; }
	Diferencia d = comparar(_base, nueva);
	_ignAltas = d.altas; _ignBajas = d.bajas; _ignRaros = d.raros;
	// anti-bucle: lo que este PC ya recibio de otros lotes no es un traspaso nuevo del anfitrion
	std::vector<Movimiento> movs;
	for (const Movimiento& m : d.traspasos) {
		bool recibido = false;
		for (const Registro& r : _reg) {
			if (loteInactivo(r.estado)) continue;
			for (const CambioLote& c : r.lote.cambios) if (c.jugador == m.jugador && c.hasta == m.hasta) recibido = true;
		}
		if (!recibido) movs.push_back(m);
	}
	if (movs.empty()) {   // otros cambios de Editar (o solo lo recibido): se anota la foto y no se publica nada
		_base = nueva; guardarFoto(_base, "foto.txt");
		cerrar("sin_traspasos");
		return;
	}
	std::vector<CambioLote> cambios;
	for (const Movimiento& m : movs) { CambioLote c; c.jugador = m.jugador; c.desde = m.desde; c.hasta = m.hasta; c.desdeNombre = m.desdeNombre; c.hastaNombre = m.hastaNombre; cambios.push_back(c); }
	std::string parche, huella;
	if (_c.enriquecer) _c.enriquecer(cambios, parche, huella);
	const bool masivo = cambios.size() > kMaxCambiosLote;
	if (_c.confirmarAntes || masivo) {
		_pendientes = cambios; _pend = nueva; _masivo = masivo; _parchePend = parche; _huellaPend = huella;
		guardarPendientes();
		cerrar(masivo ? "pendiente_masivo" : "pendiente_de_confirmar");
		return;
	}
	std::string msg;
	if (!publicarLote(cambios, parche, huella, msg)) {   // no se marca como procesado: reintenta cuando el transporte lo permita
		_estadoAnfitrion = (_t && _t->estadoClave() != "ok") ? "transporte:" + _t->estadoClave() : "error_al_publicar: " + msg;
		_proximoPublicarMs = _c.sondeoMs == 0 ? 0 : ahoraMs() + (std::max)(_t ? _t->esperaMs() : 0LL, 3000LL);
		return;
	}
	_base = nueva; guardarFoto(_base, "foto.txt");
	cerrar("publicado");
}

bool Motor::publicarPendientes(std::string& mensaje) {
	std::lock_guard<std::mutex> g(_m);
	if (_pendientes.empty()) { mensaje = "No hay traspasos pendientes."; return false; }
	if (!publicarLote(_pendientes, _parchePend, _huellaPend, mensaje)) return false;
	_base = _pend; guardarFoto(_base, "foto.txt");
	_pendientes.clear(); _masivo = false;
	guardarPendientes();
	_estadoAnfitrion = "publicado";
	return true;
}

bool Motor::descartarPendientes(std::string& mensaje) {
	std::lock_guard<std::mutex> g(_m);
	if (_pendientes.empty()) { mensaje = "No hay traspasos pendientes."; return false; }
	_base = _pend; guardarFoto(_base, "foto.txt");   // lo descartado se da por visto: no vuelve a salir
	_pendientes.clear(); _masivo = false;
	guardarPendientes();
	_estadoAnfitrion = "descartado";
	mensaje = "Descartado: esos traspasos no se enviaron.";
	return true;
}

void Motor::avisarJuego(const std::string& texto) const {
	if (_c.juego.empty()) return;
	for (const auto& d : phoenix::modulos::destinos(_c.juego)) {
		std::error_code ec;
		const fs::path c = d.second / "content" / "phoenix";
		if (!fs::is_directory(c, ec)) continue;
		phoenix::buzon::escribirAtomicoComo(c, "recargar", ".txt", texto.substr(0, 200) + "\n");
	}
}

void Motor::pasoRecibir() {
	std::lock_guard<std::mutex> g(_m);
	if (!_c.recibir && !_c.oficial) { _estadoAmigo = "apagado"; return; }
	if (!_t) { _estadoAmigo = "sin_transporte"; return; }
	fijarOrigen();
	const long long ahoraM = ahoraMs();
	std::string estadoTransporte;   // vacio = bien; si no, la clave del transporte (web_sin_endpoints, sin_cuenta, limite…)
	bool cambio = false;
	if (ahoraM >= _proximoTraerMs && _t->esperaMs() == 0) {
		// «ponerse al dia»: si el transporte dice que hay mas paginas, se sigue enseguida (como mucho 10 por paso)
		for (int pagina = 0; pagina < 10; pagina++) {
			std::vector<Lote> nuevos;
			std::string e;
			int ilegibles = 0;
			if (!_t->traer(_ultimoSeq, nuevos, ilegibles, e)) {
				estadoTransporte = _t->estadoClave() == "ok" ? "sin_conexion" : _t->estadoClave();
				_mensajeAmigo = _t->estadoTexto().empty() ? e : _t->estadoTexto();
				break;
			}
			_ilegibles += ilegibles;
			for (Lote& l : nuevos) {
				_ultimoSeq = (std::max)(_ultimoSeq, l.seq);
				cambio = true;
				bool conocido = false;
				for (Registro& r : _reg) if (r.lote.id == l.id) { conocido = true; if (r.lote.seq == 0) r.lote.seq = l.seq; }
				if (conocido) continue;   // lote repetido: idempotente
				if (!l.oficial && !_c.recibir) continue;   // solo sigo el canal oficial: lo de amigos no me toca (la web ya no lo manda con solo_oficial)
				Registro r;
				r.lote = l; r.recibido = ahora(); r.origen = _origenActual; r.orden = ordenDe(_origenActual, l.seq);
				if (l.oficial) r.estado = _c.oficial ? "pendiente" : "ignorado";   // los oficiales de Phoenix no piden aprobacion
				else if (l.anfitrion == _anfitrionId) { r.propio = true; r.estado = _c.aplicarPropios ? "pendiente" : "propio"; r.modo = _c.aplicarPropios ? "ids" : "nada"; }
				else r.estado = _c.aprobarRecibidos ? "por_aprobar" : "pendiente";
				_reg.push_back(std::move(r));
			}
			if (!_t->hayMas()) break;
		}
		_proximoTraerMs = _c.sondeoMs == 0 ? 0 : ahoraM + (std::max)(_c.sondeoMs, _t->intervaloMs());
	}
	else estadoTransporte = _t->estadoClave() == "ok" ? std::string() : _t->estadoClave();   // entre sondeos tambien se ve el problema (no vuelve a «al dia»)
	std::stable_sort(_reg.begin(), _reg.end(), [](const Registro& a, const Registro& b) { return claveOrden(a) < claveOrden(b); });
	if (cambio) guardarRegistros();
	bool hayPend = false, hayPorAprobar = false;
	for (const Registro& r : _reg) { if (r.estado == "pendiente") hayPend = true; if (r.estado == "por_aprobar") hayPorAprobar = true; }
	if (!hayPend) {
		_estadoAmigo = !estadoTransporte.empty() ? estadoTransporte : hayPorAprobar ? "por_aprobar" : "al_dia";
		if (estadoTransporte.empty()) _mensajeAmigo.clear();
		return;
	}
	if (ahoraMs() < _reintentoAplicar) return;
	std::string msg;
	if (aplicar(false, msg)) { _estadoAmigo = "aplicado"; _mensajeAmigo = msg; _reintentoAplicar = 0; }
	else { _estadoAmigo = "esperando"; _mensajeAmigo = msg; _reintentoAplicar = ahoraMs() + _c.esperaReintentoMs; }
}

namespace {
	std::string nombreJugador(const Detalle& d) { return d.jugadorNombre.empty() ? "el jugador " + std::to_string(d.jugador) : d.jugadorNombre; }
	std::string nombreEquipo(const std::string& n, uint32_t id) { return n.empty() ? "el equipo " + std::to_string(id) : n; }
	/// «Fichaje aplicado: X pasa de A a B.» / «3 fichajes aplicados: X (A → B); …»
	std::string resumenAplicado(const std::vector<const Registro*>& rs) {
		std::vector<std::string> items, cortos;
		for (const Registro* r : rs)
			for (const Detalle& d : r->detalle)
				if (d.estado == "aplicado") {
					items.push_back(nombreJugador(d) + " pasa de " + nombreEquipo(d.desdeNombre, d.desde) + " a " + nombreEquipo(d.hastaNombre, d.hasta));
					cortos.push_back(nombreJugador(d) + " (" + nombreEquipo(d.desdeNombre, d.desde) + " → " + nombreEquipo(d.hastaNombre, d.hasta) + ")");
				}
		if (items.empty()) return std::string();
		if (items.size() == 1) return "Fichaje aplicado: " + items[0] + ".";
		std::string s = std::to_string(items.size()) + " fichajes aplicados: " + cortos[0] + "; " + cortos[1];
		if (items.size() > 2) s += "; y " + std::to_string(items.size() - 2) + " más";
		return s + ".";
	}
}

std::string textoAccionRecarga(bool pesAbierto, const std::string& modo) {
	if (!pesAbierto) return "Abre PES para verlo.";
	if (modo == "ACTIVAR") return "Cierra y vuelve a abrir PES para verlo, o sin cerrar: Partido → Datos Actual. en vivo → Activar.";
	return "Cierra y vuelve a abrir PES para verlo, o espera: el juego lo recarga solo al volver al menú principal y entrar a un modo.";
}

std::string Motor::tomarAviso() { std::lock_guard<std::mutex> g(_m); std::string a; a.swap(_avisoPendiente); return a; }

bool Motor::aplicar(bool deshaciendo, std::string& mensaje) {
	if (_c.juego.empty()) { mensaje = "Link aun no sabe donde esta tu juego."; return false; }
	const bool abierto = _c.pesAbierto ? _c.pesAbierto() : false;
	fichajes::Lista manual;
	if (_c.listaManual) manual = _c.listaManual();

	// 1) como se aplica cada lote en ESTE PC: mismos ids (misma base que el anfitrion), por nombres (otro parche) o no se puede
	const std::vector<fichajes::Analisis> an0 = fichajes::analizar(_c.juego, fichajes::Lista());
	bool hayBase = false;
	for (const fichajes::Analisis& a : an0) if (a.baseOk) hayBase = true;
	if (!hayBase) { mensaje = "Tu juego no tiene una base reconocida (mira SYNC › Fichajes): no se aplica nada."; return false; }
	for (Registro& r : _reg) {
		if (r.estado != "pendiente" && r.estado != "aplicado" && r.estado != "parcial" && r.estado != "saltado") continue;
		bool mismaBase = !r.lote.baseHuella.empty();
		for (const fichajes::Analisis& a : an0)
			if (a.baseOk && (r.lote.baseHuella.empty() || a.baseSha.compare(0, r.lote.baseHuella.size(), r.lote.baseHuella) != 0)) mismaBase = false;
		bool nombres = true;
		for (const CambioLote& c : r.lote.cambios) if (c.jugadorNombre.empty() || c.hastaNombre.empty()) nombres = false;
		r.modo = mismaBase ? "ids" : (nombres ? "nombres" : "nada");
	}

	// 2) la lista efectiva y lo que pasaria (solo lectura)
	const fichajes::Lista lista = construirLista(manual, _reg);
	const std::vector<fichajes::Analisis> an = fichajes::analizar(_c.juego, lista);
	if (abierto) {
		// Con PES abierto solo se reemplaza el PlayerAssignment.bin de Phoenix-DB (atomico). No se toca sider.ini ni se aparta ningun Player.bin.
		for (const fichajes::Analisis& a : an) {
			if (!a.baseOk || (a.aplicados == 0 && a.estado != "desfasado")) continue;
			std::string ini;
			if (!leerTexto(a.objetivo.ini, ini, 4u * 1024u * 1024u) || !phoenix::modulos::tieneRaizPhoenixDB(ini)) {
				mensaje = "PES esta abierto y Phoenix-DB aun no esta en el sider.ini de «" + a.objetivo.nombre + "»: se aplicara al cerrar PES (o instala los Modulos del juego).";
				return false;
			}
			if (a.conPlayerBinAjeno) { mensaje = "PES esta abierto y Phoenix-DB tiene un Player.bin ajeno en «" + a.objetivo.nombre + "»: se aplicara al cerrar PES."; return false; }
		}
	}

	// 3) escribir (todo o nada, con copia): lo hace Fichajes
	fichajes::Informe inf = lista.cambios.empty() ? fichajes::quitarDelJuego(_c.juego) : fichajes::aplicarAlJuego(_c.juego, lista);
	if (!inf.ok) { mensaje = inf.mensaje; return false; }

	// 4) resultado por lote y por cambio
	std::map<std::string, std::string> hecho, saltado, ya;   // nota(jugador) -> estado / motivo
	bool algoAplicado = false;
	for (const fichajes::Analisis& a : (inf.objetivos.empty() ? an : inf.objetivos)) {
		if (!a.baseOk) continue;
		for (const fichajes::Hecho& h : a.hechos) { hecho[h.cambio.nota] = "ok"; algoAplicado = true; }
		for (const fichajes::Salto& s : a.saltados) if (!saltado.count(s.cambio.nota)) saltado[s.cambio.nota] = s.motivo;
	}
	std::map<uint32_t, long long> ultimoSeqDe;   // jugador -> seq del ultimo lote activo que lo mueve
	for (const Registro& r : _reg) {
		if (loteInactivo(r.estado) || r.modo == "nada") continue;
		for (const CambioLote& c : r.lote.cambios) ultimoSeqDe[c.jugador] = (std::max)(ultimoSeqDe[c.jugador], claveOrden(r));
	}
	const std::string hora = ahora();
	std::set<std::string> eranNuevos;   // lotes que estaban «pendiente» al entrar: de ellos es el aviso «Fichaje aplicado…»
	for (const Registro& r : _reg) if (r.estado == "pendiente") eranNuevos.insert(r.lote.id);
	for (Registro& r : _reg) {
		if (loteInactivo(r.estado)) continue;
		r.detalle.clear();
		int ok = 0, no = 0;
		for (const CambioLote& c : r.lote.cambios) {
			Detalle d;
			d.jugador = c.jugador; d.desde = c.desde; d.hasta = c.hasta; d.jugadorNombre = c.jugadorNombre; d.desdeNombre = c.desdeNombre; d.hastaNombre = c.hastaNombre;
			const std::string nota = std::to_string(c.jugador);
			if (r.modo == "nada") { d.estado = "saltado"; d.motivo = "Tu parche es otro y el lote no trae nombres: no se adivina."; no++; }
			else if (ultimoSeqDe[c.jugador] > claveOrden(r)) { d.estado = "reemplazado"; d.motivo = "Un lote posterior lo movio de nuevo."; ok++; }
			else if (hecho.count(nota)) { d.estado = "aplicado"; ok++; }
			else if (saltado.count(nota)) { d.estado = "saltado"; d.motivo = saltado[nota]; no++; }
			else { d.estado = "ya_estaba"; ok++; }
			r.detalle.push_back(d);
		}
		if (r.modo == "nada") { r.estado = "saltado"; r.motivo = "Parche distinto y sin nombres en el lote."; }
		else r.estado = no == 0 ? "aplicado" : (ok == 0 ? "saltado" : "parcial");
		if (r.aplicadoEn.empty() || r.estado == "aplicado" || r.estado == "parcial") r.aplicadoEn = hora;
	}
	guardarRegistros();
	if (algoAplicado || deshaciendo) avisarJuego("traspasos " + hora);
	if (!deshaciendo) {
		std::vector<const Registro*> rs;
		for (const Registro& r : _reg) if (eranNuevos.count(r.lote.id) && (r.estado == "aplicado" || r.estado == "parcial")) rs.push_back(&r);
		const std::string res = resumenAplicado(rs);
		if (!res.empty()) _avisoPendiente = res + " " + textoAccionRecarga(_c.pesAbierto ? _c.pesAbierto() : false, _c.modoRecarga ? _c.modoRecarga() : std::string("AUTO-SIEMPRE"));
	}
	mensaje = inf.mensaje;
	return true;
}

void Motor::aplicarPropios(bool si) {
	std::lock_guard<std::mutex> g(_m);
	_c.aplicarPropios = si;
	int cambiados = 0;
	for (Registro& r : _reg) {
		if (!r.propio) continue;
		if (si && r.estado == "propio") { r.estado = "pendiente"; r.modo = "ids"; cambiados++; }
		else if (!si && (r.estado == "pendiente" || r.estado == "aplicado" || r.estado == "parcial" || r.estado == "saltado")) { r.estado = "propio"; r.modo = "nada"; cambiados++; }
	}
	if (cambiados == 0) return;
	guardarRegistros();
	_reintentoAplicar = 0;
	if (!si) { std::string msg; aplicar(false, msg); }   // vuelve a generar Phoenix-DB sin mis fichajes
}

void Motor::aprobarRecibidos(bool si) { std::lock_guard<std::mutex> g(_m); _c.aprobarRecibidos = si; }

void Motor::oficial(bool si) {
	std::lock_guard<std::mutex> g(_m);
	_c.oficial = si;
	int cambiados = 0;
	for (Registro& r : _reg) {
		if (!r.lote.oficial) continue;
		if (si && r.estado == "ignorado") { r.estado = "pendiente"; cambiados++; }
		else if (!si && (r.estado == "pendiente" || r.estado == "aplicado" || r.estado == "parcial" || r.estado == "saltado")) { r.estado = "ignorado"; cambiados++; }
	}
	if (cambiados == 0) return;
	guardarRegistros();
	_reintentoAplicar = 0;
	if (!si) { std::string msg; aplicar(false, msg); }   // vuelve a generar Phoenix-DB sin los cambios oficiales
}

std::vector<ResultadoOficial> Motor::resultadosOficiales() const {
	std::lock_guard<std::mutex> g(_m);
	std::vector<ResultadoOficial> v;
	for (const Registro& r : _reg) {
		if (!r.lote.oficial || r.lote.cambioId.empty()) continue;
		ResultadoOficial x;
		x.cambioId = r.lote.cambioId;
		x.jugadores = static_cast<int>(r.lote.cambios.size());
		x.cuando = r.aplicadoEn.empty() ? r.recibido : r.aplicadoEn;
		x.motivo = r.motivo;
		for (const Detalle& d : r.detalle) if (x.motivo.empty() && !d.motivo.empty() && d.estado == "saltado") x.motivo = d.motivo;
		if (r.estado == "aplicado") {
			bool todosYa = !r.detalle.empty(), todosReempl = !r.detalle.empty();
			for (const Detalle& d : r.detalle) { if (d.estado != "ya_estaba") todosYa = false; if (d.estado != "reemplazado") todosReempl = false; }
			x.resultado = todosYa ? "ya_estaba" : todosReempl ? "reemplazado" : "aplicado";
		}
		else if (r.estado == "ignorado") { x.resultado = "saltado"; x.motivo = "Este PC tiene apagado «Recibir los cambios oficiales de Phoenix»."; }
		else if (r.estado == "parcial" || r.estado == "saltado" || r.estado == "pendiente" || r.estado == "por_aprobar" || r.estado == "deshecho" || r.estado == "error") x.resultado = r.estado;
		else if (r.estado == "rechazado") x.resultado = "deshecho";
		else continue;
		if (x.resultado == "pendiente" && x.motivo.empty()) x.motivo = _mensajeAmigo;   // p. ej. «PES está abierto y Phoenix-DB aún no está en el sider.ini…»
		x.motivo = phoenix::buzon::cortarUtf8(x.motivo, 200);
		v.push_back(std::move(x));
	}
	return v;
}

// ─── Registro de este PC en la web ───────────────────────────────────────────────────────────────
std::string cuerpoInforme(const DatosPc& d, const std::vector<ResultadoOficial>& r) {
	int build = 0;
	{   // 0.3.17 → 317 (mayor*10000 + menor*100 + parche): solo para ordenar en la web
		int a = 0, b = 0, c = 0;
		if (std::sscanf(d.version.c_str(), "%d.%d.%d", &a, &b, &c) >= 1) build = a * 10000 + b * 100 + c;
	}
	json j = { {"link_version", phoenix::buzon::cortarUtf8(d.version, 20)}, {"link_build", build}, {"parche", phoenix::buzon::cortarUtf8(d.parche, 60)},
		{"base_huella", esHex(d.baseHuella) ? d.baseHuella.substr(0, 64) : std::string()}, {"juego", d.juego}, {"pes_abierto", d.pesAbierto},
		{"oficial", d.oficial}, {"recibir", d.recibir}, {"publicar", d.publicar}, {"resultados", json::array()} };
	for (size_t i = 0; i < r.size() && i < 50; i++)
		j["resultados"].push_back({ {"cambio_id", r[i].cambioId}, {"destino", "base"}, {"resultado", r[i].resultado}, {"motivo", r[i].motivo}, {"jugadores", r[i].jugadores}, {"cuando", r[i].cuando} });
	return j.dump();
}

namespace {
	std::string firmaDatos(const DatosPc& d) {
		return d.version + "|" + d.parche + "|" + d.baseHuella + "|" + (d.juego ? "1" : "0") + (d.oficial ? "1" : "0") + (d.recibir ? "1" : "0") + (d.publicar ? "1" : "0");
	}
}

bool Informador::toca(long long ahoraMs, const DatosPc& d, const std::vector<ResultadoOficial>& todos, std::vector<ResultadoOficial>& aMandar) {
	aMandar.clear();
	if (ahoraMs < _proximo) return false;
	for (const ResultadoOficial& r : todos) {
		auto it = _enviado.find(r.cambioId);
		if (it == _enviado.end() || it->second != r.resultado) aMandar.push_back(r);
		if (aMandar.size() >= 50) break;
	}
	const bool latido = _ultimo < 0 || ahoraMs - _ultimo >= _latido;
	return latido || !aMandar.empty() || firmaDatos(d) != _datosEnviados;
}

void Informador::enviado(long long ahoraMs, const DatosPc& d, const std::vector<ResultadoOficial>& mandados) {
	for (const ResultadoOficial& r : mandados) _enviado[r.cambioId] = r.resultado;
	_datosEnviados = firmaDatos(d);
	_ultimo = ahoraMs; _espera = 0;
	_proximo = ahoraMs + 10000;   // como mucho un informe cada 10 s (la web deja 10/min)
}

void Informador::fallo(long long ahoraMs) {
	_espera = _espera == 0 ? 60000 : (std::min)(_espera * 2, 10LL * 60 * 1000);
	_proximo = ahoraMs + _espera;
}

bool Motor::aceptarRecibidos(std::string& mensaje) {
	std::lock_guard<std::mutex> g(_m);
	int n = 0;
	for (Registro& r : _reg) if (r.estado == "por_aprobar") { r.estado = "pendiente"; n++; }
	if (n == 0) { mensaje = "No hay fichajes esperando tu aprobación."; return false; }
	guardarRegistros();
	_reintentoAplicar = 0;
	mensaje = n == 1 ? "Aceptado: el fichaje se pone en tu juego." : std::to_string(n) + " fichajes aceptados: se ponen en tu juego.";
	return true;
}

bool Motor::rechazarRecibidos(std::string& mensaje) {
	std::lock_guard<std::mutex> g(_m);
	int n = 0;
	for (Registro& r : _reg) if (r.estado == "por_aprobar") { r.estado = "rechazado"; n++; }
	if (n == 0) { mensaje = "No hay fichajes esperando tu aprobación."; return false; }
	guardarRegistros();
	mensaje = n == 1 ? "Rechazado: ese fichaje no se pone en tu juego." : std::to_string(n) + " fichajes rechazados: no se ponen en tu juego.";
	return true;
}

bool Motor::deshacerUltimo(std::string& mensaje) {
	std::lock_guard<std::mutex> g(_m);
	Registro* ultimo = nullptr;
	for (Registro& r : _reg) if ((r.estado == "aplicado" || r.estado == "parcial") && (!ultimo || r.lote.seq >= ultimo->lote.seq)) ultimo = &r;
	if (!ultimo) { mensaje = "No hay ningun lote aplicado que deshacer."; return false; }
	const std::string anterior = ultimo->estado;
	ultimo->estado = "deshecho";
	std::string msg;
	if (!aplicar(true, msg)) { ultimo->estado = anterior; mensaje = "No se pudo deshacer: " + msg; return false; }
	mensaje = "Lote " + ultimo->lote.id.substr(0, 8) + " deshecho. " + msg;
	return true;
}

json Motor::archivoOption() const {
	json r = { {"existe", false}, {"ruta", ""}, {"bytes", 0}, {"fecha", ""} };
	fs::path archivo = _c.optionFile;
	std::error_code ec;
	if (archivo.empty()) return r;
	if (fs::is_directory(archivo, ec)) archivo /= "EDIT00000000";
	r["ruta"] = archivo.string();
	if (!fs::is_regular_file(archivo, ec)) return r;
	const auto tam = fs::file_size(archivo, ec);
	const auto ft = fs::last_write_time(archivo, ec);
	if (ec) return r;
	r["existe"] = true; r["bytes"] = static_cast<long long>(tam);
	const auto sc = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ft - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
	const std::time_t t = std::chrono::system_clock::to_time_t(sc);
	std::tm lt{};
#ifdef _WIN32
	localtime_s(&lt, &t);
#else
	localtime_r(&t, &lt);
#endif
	char b[32];
	std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &lt);
	r["fecha"] = b;
	return r;
}

std::string Motor::estadoJson() const {
	std::lock_guard<std::mutex> g(_m);
	json j;
	j["publicar"] = _c.publicar; j["recibir"] = _c.recibir; j["confirmarAntes"] = _c.confirmarAntes;
	j["transporte"] = _t ? _t->descripcion() : std::string();
	j["transporteEstado"] = { {"clave", _t ? _t->estadoClave() : std::string("sin_transporte")}, {"texto", _t ? _t->estadoTexto() : std::string()}, {"esperaSeg", _t ? _t->esperaMs() / 1000 : 0LL}, {"tipo", _t ? (_t->identidad().rfind("web", 0) == 0 ? "web" : "carpeta") : ""} };
	j["anfitrionId"] = _anfitrionId;
	j["modoRecarga"] = _c.modoRecarga ? _c.modoRecarga() : std::string("AUTO-SIEMPRE");
	j["pesAbierto"] = _c.pesAbierto ? _c.pesAbierto() : false;
	json a = { {"estado", _estadoAnfitrion}, {"optionFile", _c.optionFile.string()}, {"ultimaRevision", _ultimaRevision}, {"hayReferencia", _hayBase}, {"archivo", archivoOption()},
		{"publicados", _publicados}, {"ultimoLote", _ultimoPublicado}, {"masivo", _masivo}, {"ignorados", { {"altas", _ignAltas}, {"bajas", _ignBajas}, {"otros", _ignRaros} }}, {"pendientes", json::array()} };
	for (const CambioLote& c : _pendientes) a["pendientes"].push_back(cambioAJson(c));
	j["anfitrion"] = a;
	json m = { {"estado", _estadoAmigo}, {"mensaje", _mensajeAmigo}, {"ultimoSeq", _ultimoSeq}, {"ilegibles", _ilegibles}, {"historial", json::array()} };
	bool puedeDeshacer = false;
	for (auto it = _reg.rbegin(); it != _reg.rend(); ++it) {
		const Registro& r = *it;
		if ((r.estado == "aplicado" || r.estado == "parcial")) puedeDeshacer = true;
		json cs = json::array();
		for (const Detalle& d : r.detalle) cs.push_back({ {"jugador", d.jugador}, {"jugadorNombre", d.jugadorNombre}, {"desde", d.desde}, {"hasta", d.hasta}, {"hastaNombre", d.hastaNombre}, {"estado", d.estado}, {"motivo", d.motivo} });
		if (cs.empty()) for (const CambioLote& c : r.lote.cambios) cs.push_back({ {"jugador", c.jugador}, {"jugadorNombre", c.jugadorNombre}, {"desde", c.desde}, {"hasta", c.hasta}, {"hastaNombre", c.hastaNombre}, {"estado", "pendiente"}, {"motivo", ""} });
		m["historial"].push_back({ {"id", r.lote.id}, {"seq", r.lote.seq}, {"autor", r.lote.autor}, {"creado", r.lote.creado}, {"recibido", r.recibido}, {"aplicadoEn", r.aplicadoEn},
			{"estado", r.estado}, {"modo", r.modo}, {"motivo", r.motivo}, {"resumen", resumenAplicado({ &r })}, {"oficial", r.lote.oficial}, {"cambios", cs} });
		if (m["historial"].size() >= 50) break;
	}
	m["puedeDeshacer"] = puedeDeshacer;
	int porAprobar = 0;
	for (const Registro& r : _reg) if (r.estado == "por_aprobar") porAprobar++;
	m["porAprobar"] = porAprobar;
	m["aprobarRecibidos"] = _c.aprobarRecibidos;
	for (auto it = _reg.rbegin(); it != _reg.rend(); ++it) {   // el ultimo lote que de verdad cambio algo, con lo que hay que hacer AHORA para verlo
		const std::string res = (it->estado == "aplicado" || it->estado == "parcial") ? resumenAplicado({ &*it }) : std::string();
		if (res.empty()) continue;
		const bool abierto = _c.pesAbierto ? _c.pesAbierto() : false;
		m["ultimoAplicado"] = { {"id", it->lote.id}, {"texto", res}, {"accion", textoAccionRecarga(abierto, j["modoRecarga"].get<std::string>())}, {"cuando", it->aplicadoEn} };
		break;
	}
	j["amigo"] = m;
	return j.dump();
}

} // namespace phoenix::traspasos
