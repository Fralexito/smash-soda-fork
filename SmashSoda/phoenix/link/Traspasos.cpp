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
	Lote l;
	l.autor = autor; l.anfitrion = anfitrion; l.creado = creado; l.parche = parche; l.baseHuella = baseHuella;
	l.cambios = std::move(cambios);
	l.huella = huellaDeCambios(l.cambios);
	l.id = sha(anfitrion + "|" + l.huella + "|" + creado).substr(0, 24);
	return l;
}

std::string loteAJson(const Lote& l, bool conSeq) {
	json j = { {"formato", l.formato}, {"id", l.id}, {"huella", l.huella}, {"creado", l.creado}, {"autor", l.autor}, {"anfitrion", l.anfitrion},
		{"parche", l.parche}, {"base_huella", l.baseHuella}, {"cambios", json::array()} };
	if (conSeq) j["seq"] = l.seq;
	for (const CambioLote& c : l.cambios) j["cambios"].push_back(cambioAJson(c));
	return j.dump(1);
}

bool loteDeJson(const std::string& texto, Lote& l, std::string& error) {
	l = Lote();
	json j = json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) { error = "no es JSON"; return false; }
	if (!j.contains("formato") || !j["formato"].is_number_integer() || j["formato"].get<int>() != kFormatoLote) { error = "formato de lote desconocido"; return false; }
	l.formato = kFormatoLote;
	l.id = textoCorto(j, "id", 64); l.huella = textoCorto(j, "huella", 80);
	l.creado = textoCorto(j, "creado", 40); l.autor = textoCorto(j, "autor", 60); l.anfitrion = textoCorto(j, "anfitrion", 64);
	l.parche = textoCorto(j, "parche", 80); l.baseHuella = textoCorto(j, "base_huella", 16);
	if (j.contains("seq") && j["seq"].is_number_integer()) l.seq = j["seq"].get<long long>();
	if (!esHex(l.id) || l.id.size() != 24) { error = "id de lote no valido"; return false; }
	if (l.anfitrion.empty()) { error = "falta el anfitrion"; return false; }
	if (!l.baseHuella.empty() && !esHex(l.baseHuella)) { error = "huella de base no valida"; return false; }
	if (!j.contains("cambios") || !j["cambios"].is_array() || j["cambios"].empty()) { error = "el lote no trae cambios"; return false; }
	if (j["cambios"].size() > 5000) { error = "demasiados cambios"; return false; }
	for (const json& c : j["cambios"]) {
		CambioLote x;
		if (!c.is_object() || !entero(c, "jugador_id", x.jugador) || !entero(c, "desde_id", x.desde) || !entero(c, "hasta_id", x.hasta)) { error = "un cambio no tiene ids validos"; return false; }
		if (x.desde == x.hasta) { error = "un cambio tiene el mismo equipo de origen y destino"; return false; }
		x.jugadorNombre = textoCorto(c, "jugador", 80); x.desdeNombre = textoCorto(c, "desde", 80); x.hastaNombre = textoCorto(c, "hasta", 80);
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
bool TransporteWeb::publicar(Lote& l, bool& ya, std::string& error) {
	ya = false;
	if (!_p) { error = "sin conexion con la web"; return false; }
	json cuerpo = { {"mundo", _mundo}, {"lote", json::parse(loteAJson(l, false))} };
	int estado = 0; std::string r;
	if (!_p("POST", "/v1/traspasos/lotes", cuerpo.dump(), estado, r)) { error = "la web no responde"; return false; }
	if (estado == 401 || estado == 403) { error = "la web no te deja publicar traspasos (permiso)"; return false; }
	if (estado != 200 && estado != 201 && estado != 409) { error = "la web respondio " + std::to_string(estado); return false; }
	json j = json::parse(r, nullptr, false);
	if (!j.is_object() || !j.contains("seq") || !j["seq"].is_number_integer()) { error = "respuesta de la web no entendida"; return false; }
	l.seq = j["seq"].get<long long>();
	ya = estado == 409 || j.value("ya", false);
	return true;
}

bool TransporteWeb::traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) {
	lotes.clear(); ilegibles = 0;
	if (!_p) { error = "sin conexion con la web"; return false; }
	int estado = 0; std::string r;
	if (!_p("GET", "/v1/traspasos/lotes?mundo=" + _mundo + "&desde=" + std::to_string(despuesDe) + "&limite=100", "", estado, r)) { error = "la web no responde"; return false; }
	if (estado != 200) { error = "la web respondio " + std::to_string(estado); return false; }
	json j = json::parse(r, nullptr, false);
	if (!j.is_object() || !j.contains("lotes") || !j["lotes"].is_array()) { error = "respuesta de la web no entendida"; return false; }
	for (const json& x : j["lotes"]) {
		Lote l; std::string e;
		if (!loteDeJson(x.dump(), l, e) || l.seq <= despuesDe) { ilegibles++; continue; }
		lotes.push_back(std::move(l));
	}
	std::sort(lotes.begin(), lotes.end(), [](const Lote& a, const Lote& b) { return a.seq < b.seq; });
	return true;
}

// ─── Lista efectiva ──────────────────────────────────────────────────────────────────────────────
fichajes::Lista construirLista(const fichajes::Lista& manual, const std::vector<Registro>& registros) {
	fichajes::Lista l = manual;
	l.version = fichajes::kVersionLista;
	l.fuente = "Traspasos en vivo" + std::string(manual.cambios.empty() ? "" : " + lista manual");
	std::vector<const Registro*> activos;
	for (const Registro& r : registros) {
		if (r.estado == "deshecho" || r.estado == "propio" || r.estado == "error" || r.modo == "nada") continue;
		activos.push_back(&r);
	}
	std::stable_sort(activos.begin(), activos.end(), [](const Registro* a, const Registro* b) { return a->lote.seq < b->lote.seq; });
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
Motor::Motor(Config c, std::shared_ptr<Transporte> t) : _c(std::move(c)), _t(std::move(t)) { cargarEstado(); }

void Motor::cambiarTransporte(std::shared_ptr<Transporte> t) { std::lock_guard<std::mutex> g(_m); _t = std::move(t); }
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
	json j = { {"version", 1}, {"ultimoSeq", _ultimoSeq}, {"registros", json::array()} };
	for (const Registro& r : _reg) {
		json d = json::array();
		for (const Detalle& x : r.detalle) d.push_back({ {"jugador", x.jugador}, {"desde", x.desde}, {"hasta", x.hasta}, {"jugadorNombre", x.jugadorNombre}, {"hastaNombre", x.hastaNombre}, {"estado", x.estado}, {"motivo", x.motivo} });
		j["registros"].push_back({ {"lote", json::parse(loteAJson(r.lote, true))}, {"estado", r.estado}, {"modo", r.modo}, {"recibido", r.recibido}, {"aplicadoEn", r.aplicadoEn}, {"motivo", r.motivo}, {"detalle", d} });
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
			if (j.contains("ultimoSeq") && j["ultimoSeq"].is_number_integer()) _ultimoSeq = j["ultimoSeq"].get<long long>();
			for (const json& r : j["registros"]) {
				Registro x;
				std::string e;
				if (!r.contains("lote") || !loteDeJson(r["lote"].dump(), x.lote, e)) continue;
				x.estado = r.value("estado", std::string("pendiente")); x.modo = r.value("modo", std::string("ids"));
				x.recibido = r.value("recibido", std::string()); x.aplicadoEn = r.value("aplicadoEn", std::string()); x.motivo = r.value("motivo", std::string());
				if (r.contains("detalle") && r["detalle"].is_array())
					for (const json& d : r["detalle"]) {
						Detalle y;
						y.jugador = d.value("jugador", 0u); y.desde = d.value("desde", 0u); y.hasta = d.value("hasta", 0u);
						y.jugadorNombre = d.value("jugadorNombre", std::string()); y.hastaNombre = d.value("hastaNombre", std::string());
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
	// el anfitrion tambien guarda lo que publico (historial; no se aplica a si mismo)
	bool conocido = false;
	for (const Registro& r : _reg) if (r.lote.id == l.id) conocido = true;
	if (!conocido) {
		Registro r;
		r.lote = l; r.estado = "propio"; r.modo = "nada"; r.recibido = ahora();
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
			if (r.estado == "deshecho" || r.estado == "propio" || r.estado == "error") continue;
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
	if (!publicarLote(cambios, parche, huella, msg)) { _estadoAnfitrion = "error_al_publicar: " + msg; return; }   // no se marca como procesado: reintenta
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
	if (!_c.recibir) { _estadoAmigo = "apagado"; return; }
	if (!_t) { _estadoAmigo = "sin_transporte"; return; }
	std::vector<Lote> nuevos;
	std::string e;
	int ilegibles = 0;
	if (!_t->traer(_ultimoSeq, nuevos, ilegibles, e)) { _estadoAmigo = "sin_conexion: " + e; return; }
	_ilegibles += ilegibles;
	bool cambio = false;
	for (Lote& l : nuevos) {
		_ultimoSeq = (std::max)(_ultimoSeq, l.seq);
		cambio = true;
		bool conocido = false;
		for (Registro& r : _reg) if (r.lote.id == l.id) { conocido = true; if (r.lote.seq == 0) r.lote.seq = l.seq; }
		if (conocido) continue;   // lote repetido: idempotente
		Registro r;
		r.lote = l; r.recibido = ahora();
		if (l.anfitrion == _anfitrionId) { r.estado = "propio"; r.modo = "nada"; }
		else r.estado = "pendiente";
		_reg.push_back(std::move(r));
	}
	std::stable_sort(_reg.begin(), _reg.end(), [](const Registro& a, const Registro& b) { return a.lote.seq < b.lote.seq; });
	if (cambio) guardarRegistros();
	bool hayPend = false;
	for (const Registro& r : _reg) if (r.estado == "pendiente") hayPend = true;
	if (!hayPend) { _estadoAmigo = "al_dia"; _mensajeAmigo.clear(); return; }
	if (ahoraMs() < _reintentoAplicar) return;
	std::string msg;
	if (aplicar(false, msg)) { _estadoAmigo = "aplicado"; _mensajeAmigo = msg; _reintentoAplicar = 0; }
	else { _estadoAmigo = "esperando"; _mensajeAmigo = msg; _reintentoAplicar = ahoraMs() + _c.esperaReintentoMs; }
}

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
		if (r.estado == "deshecho" || r.estado == "propio" || r.estado == "error" || r.modo == "nada") continue;
		for (const CambioLote& c : r.lote.cambios) ultimoSeqDe[c.jugador] = (std::max)(ultimoSeqDe[c.jugador], r.lote.seq);
	}
	const std::string hora = ahora();
	for (Registro& r : _reg) {
		if (r.estado == "deshecho" || r.estado == "propio" || r.estado == "error") continue;
		r.detalle.clear();
		int ok = 0, no = 0;
		for (const CambioLote& c : r.lote.cambios) {
			Detalle d;
			d.jugador = c.jugador; d.desde = c.desde; d.hasta = c.hasta; d.jugadorNombre = c.jugadorNombre; d.hastaNombre = c.hastaNombre;
			const std::string nota = std::to_string(c.jugador);
			if (r.modo == "nada") { d.estado = "saltado"; d.motivo = "Tu parche es otro y el lote no trae nombres: no se adivina."; no++; }
			else if (ultimoSeqDe[c.jugador] > r.lote.seq) { d.estado = "reemplazado"; d.motivo = "Un lote posterior lo movio de nuevo."; ok++; }
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
	mensaje = inf.mensaje;
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

std::string Motor::estadoJson() const {
	std::lock_guard<std::mutex> g(_m);
	json j;
	j["publicar"] = _c.publicar; j["recibir"] = _c.recibir; j["confirmarAntes"] = _c.confirmarAntes;
	j["transporte"] = _t ? _t->descripcion() : std::string();
	j["anfitrionId"] = _anfitrionId;
	j["modoRecarga"] = _c.modoRecarga ? _c.modoRecarga() : std::string("AUTO-SIEMPRE");
	j["pesAbierto"] = _c.pesAbierto ? _c.pesAbierto() : false;
	json a = { {"estado", _estadoAnfitrion}, {"optionFile", _c.optionFile.string()}, {"ultimaRevision", _ultimaRevision}, {"hayReferencia", _hayBase},
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
			{"estado", r.estado}, {"modo", r.modo}, {"motivo", r.motivo}, {"cambios", cs} });
		if (m["historial"].size() >= 50) break;
	}
	m["puedeDeshacer"] = puedeDeshacer;
	j["amigo"] = m;
	return j.dump();
}

} // namespace phoenix::traspasos
