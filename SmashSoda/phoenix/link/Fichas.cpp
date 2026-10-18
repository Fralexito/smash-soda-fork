#include "Fichas.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

#include "BuzonJuego.h"
#include "Entrega.h"
#include "ModulosJuego.h"

namespace phoenix::fichas {

namespace {
	constexpr size_t kTam = 312;
	constexpr size_t kMaxArchivo = 64u * 1024u * 1024u;
	constexpr size_t kMaxCambios = 20000;
	constexpr int kCopiasQueSeQuedan = 3;   // cada copia de Player.bin pesa ~9 MB

	std::string sha(const std::string& s) { return phoenix::entrega::sha256Hex(s.data(), s.size()); }

	bool leerArchivo(const fs::path& ruta, std::string& out, uintmax_t maximo = kMaxArchivo) {
		std::error_code ec;
		const uintmax_t n = fs::file_size(ruta, ec);
		if (ec || n > maximo) return false;
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		out.assign(static_cast<size_t>(n), '\0');
		if (n > 0) f.read(&out[0], static_cast<std::streamsize>(n));
		return static_cast<uintmax_t>(f.gcount()) == n;
	}

	std::string ahora(const char* formato) {
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		char b[32];
		std::strftime(b, sizeof(b), formato, &lt);
		return b;
	}

	uint32_t le32(const std::string& s, size_t p) {
		return static_cast<uint32_t>(static_cast<unsigned char>(s[p])) | (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 1])) << 8)
			| (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 2])) << 16) | (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 3])) << 24);
	}
	uint32_t bitsDe(const std::string& s, size_t r, int bit, int n) {
		uint32_t v = 0;
		for (int i = 0; i < n; i++) { const size_t b = static_cast<size_t>(bit + i); v |= static_cast<uint32_t>((static_cast<unsigned char>(s[r + (b >> 3)]) >> (b & 7)) & 1) << i; }
		return v;
	}
	void ponBits(std::string& s, size_t r, int bit, int n, uint32_t v) {
		for (int i = 0; i < n; i++) {
			const size_t b = static_cast<size_t>(bit + i);
			const unsigned char m = static_cast<unsigned char>(1u << (b & 7));
			unsigned char& c = reinterpret_cast<unsigned char&>(s[r + (b >> 3)]);
			if ((v >> i) & 1) c = static_cast<unsigned char>(c | m); else c = static_cast<unsigned char>(c & ~m);
		}
	}

	fs::path dePesdb(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"; }
	fs::path dePhoenixDB(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB"; }
	bool cambiarNombre(const fs::path& de, const fs::path& a) { std::error_code ec; fs::rename(de, a, ec); return !ec; }

	nlohmann::json leerEstado(const fs::path& sider) {
		std::string t;
		if (!leerArchivo(dePhoenixDB(sider) / "fichas_aplicadas.json", t, 4 * 1024 * 1024)) return nlohmann::json();
		nlohmann::json j = nlohmann::json::parse(t, nullptr, false);
		return (j.is_discarded() || !j.is_object()) ? nlohmann::json() : j;
	}

	/// sha de los bytes reales del Player.bin de Phoenix-DB («» si no hay o no se lee).
	std::string shaActual(const fs::path& sider, bool& hay) {
		std::string b, crudo, e;
		hay = leerArchivo(dePesdb(sider) / "Player.bin", b);
		if (!hay || !fichajes::leerBin(b, crudo, e)) return std::string();
		return sha(crudo);
	}

	/// Deja solo las kCopiasQueSeQuedan carpetas _antes-de-fichas-* más nuevas (el nombre lleva la fecha: orden alfabético = orden de fecha).
	void podarCopias(const fs::path& phx) {
		std::error_code ec;
		std::vector<fs::path> v;
		for (fs::directory_iterator it(phx, ec), fin; !ec && it != fin; it.increment(ec))
			if (it->is_directory(ec) && phoenix::entrega::aU8(it->path().filename()).rfind("_antes-de-fichas-", 0) == 0) v.push_back(it->path());
		std::sort(v.begin(), v.end());
		for (size_t i = 0; i + kCopiasQueSeQuedan < v.size(); i++) fs::remove_all(v[i], ec);
	}
}

// ─── campos ──────────────────────────────────────────────────────────────────────────────────────
// Bits del motor (PhoenixSync, guía 16 §2–§4 y 01-MOTOR §J), confirmados con las pantallas del juego.
const std::vector<Campo>& campos() {
	static const std::vector<Campo> c = {
		{ "ataque", "Actitud ofensiva", "campo", 370, 6, 40, 40, 99 },
		{ "control", "Control del balón", "campo", 281, 6, 40, 40, 99 },
		{ "regate", "Regate", "campo", 352, 6, 40, 40, 99 },
		{ "regate_ajustado", "Conservación del balón", "campo", 416, 6, 40, 40, 99 },
		{ "pase_raso", "Pase raso", "campo", 263, 6, 40, 40, 99 },
		{ "pase_bombeado", "Pase bombeado", "campo", 402, 6, 40, 40, 99 },
		{ "finalizacion", "Finalización", "campo", 396, 6, 40, 40, 99 },
		{ "cabezazo", "Cabeceo", "campo", 288, 6, 40, 40, 99 },
		{ "balon_parado", "Balón parado", "campo", 250, 6, 40, 40, 99 },
		{ "efecto", "Efecto", "campo", 332, 6, 40, 40, 99 },
		{ "velocidad", "Velocidad", "campo", 306, 6, 40, 40, 99 },
		{ "aceleracion", "Aceleración", "campo", 344, 6, 40, 40, 99 },
		{ "equilibrio", "Equilibrio", "campo", 376, 6, 40, 40, 99 },
		{ "contacto_fisico", "Contacto físico", "campo", 390, 6, 40, 40, 99 },
		{ "salto", "Salto", "campo", 294, 6, 40, 40, 99 },
		{ "resistencia", "Resistencia", "campo", 338, 6, 40, 40, 99 },
		{ "defensa", "Actitud defensiva", "campo", 275, 6, 40, 40, 99 },
		{ "recuperacion", "Recuperación de balón", "campo", 312, 6, 40, 40, 99 },
		{ "agresividad", "Agresividad", "campo", 384, 6, 40, 40, 99 },
		{ "potencia_tiro", "Potencia de tiro", "campo", 358, 6, 40, 40, 99 },
		{ "pt_despejar", "Despejar (portero)", "portero", 269, 6, 40, 40, 99 },
		{ "pt_cobertura", "Cobertura (portero)", "portero", 300, 6, 40, 40, 99 },
		{ "pt_reflejos", "Reflejos (portero)", "portero", 320, 6, 40, 40, 99 },
		{ "pt_actitud", "Actitud de portero", "portero", 326, 6, 40, 40, 99 },
		{ "pt_atajar", "Atajar (portero)", "portero", 364, 6, 40, 40, 99 },
		{ "pie_malo_uso", "Uso del pie malo", "pie_malo", 454, 2, 1, 1, 4 },
		{ "pie_malo_precision", "Precisión del pie malo", "pie_malo", 462, 2, 1, 1, 4 },
		{ "forma", "Forma (regularidad)", "estado", 438, 3, 1, 1, 8 },
		{ "resistencia_lesiones", "Resistencia a lesiones", "estado", 458, 2, 1, 1, 3 },
		// Posiciones: 0 = C, 1 = B, 2 = A.
		{ "pos_pt", "Posición PT", "posicion", 350, 2, 0, 0, 2 },
		{ "pos_dfc", "Posición DFC", "posicion", 468, 2, 0, 0, 2 },
		{ "pos_li", "Posición LI", "posicion", 318, 2, 0, 0, 2 },
		{ "pos_ld", "Posición LD", "posicion", 474, 2, 0, 0, 2 },
		{ "pos_mcd", "Posición MCD", "posicion", 414, 2, 0, 0, 2 },
		{ "pos_mc", "Posición MC", "posicion", 456, 2, 0, 0, 2 },
		{ "pos_mi", "Posición MI", "posicion", 466, 2, 0, 0, 2 },
		{ "pos_md", "Posición MD", "posicion", 460, 2, 0, 0, 2 },
		{ "pos_mo", "Posición MO", "posicion", 464, 2, 0, 0, 2 },
		{ "pos_ei", "Posición EI", "posicion", 472, 2, 0, 0, 2 },
		{ "pos_ed", "Posición ED", "posicion", 476, 2, 0, 0, 2 },
		{ "pos_sd", "Posición SD", "posicion", 478, 2, 0, 0, 2 },
		{ "pos_dc", "Posición DC", "posicion", 470, 2, 0, 0, 2 },
		// Datos básicos (01-MOTOR §J). Rangos prudentes, no los máximos de los bits.
		{ "altura", "Altura (cm)", "basico", 216, 8, 100, 150, 210 },
		{ "peso", "Peso (kg)", "basico", 256, 7, 30, 45, 110 },
		{ "edad", "Edad", "basico", 408, 6, 15, 15, 45 },
		// 0 PT · 1 DFC · 2 LI · 3 LD · 4 MCD · 5 MC · 6 MI · 7 MD · 8 MO · 9 EI · 10 ED · 11 SD · 12 DC (orden del motor).
		{ "posicion_registrada", "Posición registrada", "basico", 434, 4, 0, 0, 12 },
	};
	return c;
}

const Campo* campo(const std::string& clave) {
	for (const Campo& c : campos()) if (clave == c.clave) return &c;
	return nullptr;
}

// ─── Ficha ───────────────────────────────────────────────────────────────────────────────────────
bool Ficha::cargar(const std::string& crudo, std::string& error) {
	_crudo.clear(); _indice.clear(); _porNombre.clear();
	if (crudo.size() < 8 + kTam * 1000 || ((crudo.size() - 8) % kTam != 0 && (crudo.size() - 8) % kTam != 304)) { error = "Player.bin con un tamaño que no es de PES 2021"; return false; }
	_crudo = crudo;
	for (size_t r = 0; r + 8 + 64 <= _crudo.size(); r += kTam) {
		const uint32_t id = le32(_crudo, r + 8);
		if (!id || _indice.count(id)) continue;   // hueco, o id repetido (se usa el primero, como el juego)
		_indice[id] = r;
		if (r + 8 + 243 + 61 <= _crudo.size()) {
			const std::string n(_crudo.data() + r + 8 + 243, 61);
			const std::string p = fichajes::plegarNombre(n.substr(0, n.find('\0')));
			if (!p.empty()) _porNombre[p].push_back(id);
		}
	}
	if (_indice.size() < 1000) { error = "Player.bin con muy pocos jugadores"; _crudo.clear(); _indice.clear(); _porNombre.clear(); return false; }
	return true;
}

bool Ficha::leer(uint32_t jugador, const std::string& clave, int& valor) const {
	const Campo* c = campo(clave);
	const auto it = _indice.find(jugador);
	if (!c || it == _indice.end()) return false;
	valor = static_cast<int>(bitsDe(_crudo, it->second, c->bit, c->bits)) + c->sumar;
	return true;
}

std::string Ficha::poner(uint32_t jugador, const std::string& clave, int valor) {
	const Campo* c = campo(clave);
	if (!c) return "campo_desconocido";
	if (valor < c->minimo || valor > c->maximo) return "fuera_de_rango";
	const auto it = _indice.find(jugador);
	if (it == _indice.end()) return "jugador_no_existe";
	const uint32_t crudo = static_cast<uint32_t>(valor - c->sumar);
	if (crudo >= (1u << c->bits)) return "fuera_de_rango";
	ponBits(_crudo, it->second, c->bit, c->bits, crudo);
	return std::string();
}

std::string Ficha::nombrePlegado(uint32_t jugador) const {
	const auto it = _indice.find(jugador);
	if (it == _indice.end() || it->second + 8 + 243 + 61 > _crudo.size()) return std::string();
	const std::string n(_crudo.data() + it->second + 8 + 243, 61);
	return fichajes::plegarNombre(n.substr(0, n.find('\0')));
}

std::vector<uint32_t> Ficha::buscarNombre(const std::string& plegado) const {
	const auto it = _porNombre.find(plegado);
	return it == _porNombre.end() ? std::vector<uint32_t>() : it->second;
}

std::vector<uint32_t> Ficha::distintos(const Ficha& otra) const {
	std::vector<uint32_t> r;
	if (_crudo.size() != otra._crudo.size()) return r;
	for (size_t o = 0; o < _crudo.size(); o += kTam) {
		const size_t n = (std::min)(kTam, _crudo.size() - o);
		if (_crudo.compare(o, n, otra._crudo, o, n) != 0) r.push_back(o + 12 <= _crudo.size() ? le32(_crudo, o + 8) : 0);
	}
	return r;
}

// ─── lista ───────────────────────────────────────────────────────────────────────────────────────
bool leerLista(const std::string& texto, Lista& lista, std::string& error) {
	lista = Lista();
	const nlohmann::json j = nlohmann::json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) { error = "fichas.json no es un JSON válido"; return false; }
	lista.version = j.value("version", 1);
	if (lista.version != kVersionLista) { error = "fichas.json de una versión que este Link no entiende (" + std::to_string(lista.version) + ")"; return false; }
	if (!j.contains("cambios") || !j["cambios"].is_array()) { error = "fichas.json sin la lista «cambios»"; return false; }
	if (j["cambios"].size() > kMaxCambios) { error = "fichas.json con demasiados cambios"; return false; }
	for (const auto& c : j["cambios"]) {
		if (!c.is_object()) { error = "un cambio de fichas.json no es un objeto"; return false; }
		Cambio x;
		if (c.contains("id")) x.id = c["id"].is_string() ? c["id"].get<std::string>() : c["id"].dump();
		if (c.contains("player_id")) {
			const auto& p = c["player_id"];
			if (p.is_number_unsigned() || (p.is_number_integer() && p.get<long long>() > 0)) x.jugador = static_cast<uint32_t>(p.get<unsigned long long>() & 0xFFFFFFFFu);
			else if (!p.is_null()) { error = "player_id no válido en fichas.json"; return false; }
		}
		x.nombre = c.value("jugador", std::string());
		x.campo = c.value("campo", std::string());
		if (!c.contains("valor") || !c["valor"].is_number_integer()) { error = "un cambio de fichas.json no trae «valor» entero"; return false; }
		x.valor = c["valor"].get<int>();
		if (x.jugador == 0 && x.nombre.empty()) { error = "un cambio de fichas.json no dice qué jugador"; return false; }
		if (x.campo.empty()) { error = "un cambio de fichas.json no dice qué campo"; return false; }
		lista.cambios.push_back(x);
	}
	lista.sha256 = sha(texto);
	return true;
}

// ─── aplicar en memoria ──────────────────────────────────────────────────────────────────────────
Resultado aplicar(const Ficha& base, const Lista& lista) {
	Resultado r;
	Ficha nueva = base;
	std::map<std::pair<uint32_t, std::string>, size_t> donde;   // (jugador, campo) → índice en aplicados (gana el último)
	auto saltar = [&](const Cambio& c, const char* codigo, const std::string& motivo) { r.saltados.push_back({ c, codigo, motivo }); };
	for (const Cambio& c : lista.cambios) {
		uint32_t id = 0;
		const std::string pedido = c.nombre.empty() ? std::string() : fichajes::plegarNombre(c.nombre);
		auto porNombre = [&]() -> bool {
			const std::vector<uint32_t> v = base.buscarNombre(pedido);
			if (v.empty()) { saltar(c, "jugador_no_encontrado", "No hay ningún jugador llamado «" + c.nombre + "» en esta base."); return false; }
			if (v.size() > 1) { saltar(c, "jugador_ambiguo", "Hay " + std::to_string(v.size()) + " jugadores llamados «" + c.nombre + "» en esta base."); return false; }
			id = v[0];
			return true;
		};
		if (c.jugador) {
			if (!base.existe(c.jugador)) {
				if (pedido.empty()) { saltar(c, "jugador_no_existe", "El jugador " + std::to_string(c.jugador) + " no está en esta base."); continue; }
				if (!porNombre()) continue;
			}
			else {
				const std::string tiene = base.nombrePlegado(c.jugador);
				if (!pedido.empty() && !tiene.empty() && tiene != pedido) {
					// Otro parche reusa el id con otro jugador: se busca por nombre, y si no, se salta (nunca se toca al jugador equivocado).
					const std::vector<uint32_t> v = base.buscarNombre(pedido);
					if (v.size() != 1) { saltar(c, "jugador_distinto", "En esta base el id " + std::to_string(c.jugador) + " es otro jugador, no «" + c.nombre + "»."); continue; }
					id = v[0];
				}
				else id = c.jugador;
			}
		}
		else if (!porNombre()) continue;
		const Campo* cp = campo(c.campo);
		if (!cp) { saltar(c, "campo_desconocido", "Campo «" + c.campo + "» desconocido para este Link."); continue; }
		if (c.valor < cp->minimo || c.valor > cp->maximo) { saltar(c, "fuera_de_rango", std::string(cp->texto) + ": " + std::to_string(cp->minimo) + "–" + std::to_string(cp->maximo) + "."); continue; }
		int antes = 0;
		base.leer(id, c.campo, antes);
		const std::string err = nueva.poner(id, c.campo, c.valor);
		if (!err.empty()) { saltar(c, err.c_str(), "No se pudo poner el valor."); continue; }
		Hecho h{ c, id, antes, antes == c.valor };
		const auto clave = std::make_pair(id, std::string(cp->clave));
		const auto it = donde.find(clave);
		if (it != donde.end()) r.aplicados[it->second] = h;
		else { donde[clave] = r.aplicados.size(); r.aplicados.push_back(h); }
	}
	for (const Hecho& h : r.aplicados) if (!h.igual) r.cambiados++;
	// Comprobación: solo cambian los bits de los campos pedidos de los jugadores pedidos.
	std::map<uint32_t, std::string> mascara;   // id → máscara de 312 bytes con los bits permitidos
	for (const Hecho& h : r.aplicados) {
		std::string& m = mascara[h.jugador];
		if (m.empty()) m.assign(kTam, '\0');
		const Campo* cp = campo(h.cambio.campo);
		for (int i = 0; i < cp->bits; i++) { const int b = cp->bit + i; m[static_cast<size_t>(b >> 3)] = static_cast<char>(m[static_cast<size_t>(b >> 3)] | (1 << (b & 7))); }
	}
	const std::string& a = base.crudo();
	const std::string& b = nueva.crudo();
	for (size_t o = 0; o < a.size() && r.error.empty(); o += kTam) {
		const size_t n = (std::min)(kTam, a.size() - o);
		if (a.compare(o, n, b, o, n) == 0) continue;
		const uint32_t id = o + 12 <= a.size() ? le32(a, o + 8) : 0;
		const auto m = mascara.find(id);
		if (m == mascara.end()) { r.error = "cambió un jugador que no estaba en la lista (" + std::to_string(id) + ")"; break; }
		for (size_t k = 0; k < n; k++) {
			const unsigned char d = static_cast<unsigned char>(a[o + k] ^ b[o + k]);
			if (d & ~static_cast<unsigned char>(m->second[k])) { r.error = "cambiaron bits fuera de los campos pedidos (jugador " + std::to_string(id) + ")"; break; }
		}
	}
	if (r.error.empty()) r.crudo = b;
	return r;
}

// ─── juego real ──────────────────────────────────────────────────────────────────────────────────
std::vector<Analisis> analizar(const fs::path& juego, const Lista& lista) {
	std::vector<Analisis> res;
	for (const fichajes::Objetivo& o : fichajes::objetivos(juego)) {
		Analisis a;
		a.objetivo = o;
		const fichajes::BaseJugadores bj = fichajes::leerPlayerBinDeBase(juego, o);
		a.parche = bj.parche; a.baseOrigen = bj.origen; a.error = bj.error; a.queHacer = bj.queHacer;
		if (!bj.ok) { a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
		Ficha base;
		std::string e;
		if (!base.cargar(bj.crudo, e)) { a.error = "Player.bin de la base no tiene el formato esperado: " + e + "."; a.queHacer = "No se crea nada."; a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
		a.baseOk = true;
		a.baseSha = bj.sha;
		const Resultado r = aplicar(base, lista);
		if (!r.error.empty()) { a.baseOk = false; a.error = "El resultado no pasa la comprobación: " + r.error + "."; a.queHacer = "No se crea nada."; a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
		a.hechos = r.aplicados; a.saltados = r.saltados; a.cambiados = r.cambiados;
		if (r.cambiados > 0) { a.salida = fichajes::escribirBin(r.crudo); a.salidaSha = sha(r.crudo); }
		const nlohmann::json st = leerEstado(o.siderAddons);
		const std::string guardada = st.is_object() ? st.value("salidaSha", std::string()) : std::string();
		bool hay = false;
		const std::string actual = shaActual(o.siderAddons, hay);
		if (guardada.empty()) a.estado = a.cambiados > 0 ? "sin_aplicar" : "nada";
		else if (!hay || actual != guardada) a.estado = "cambiado";
		else if (a.cambiados == 0) a.estado = "desfasado";
		else if (a.salidaSha == guardada && st.value("baseSha", std::string()) == a.baseSha) a.estado = "aplicado";
		else a.estado = "desfasado";
		res.push_back(std::move(a));
	}
	return res;
}

namespace {
	struct Deshacer {
		fs::path destino, copia;           // Player.bin escrito / copia del que había (vacía = no había)
		fs::path estado, estadoCopia;      // fichas_aplicadas.json
	};
	void deshacerTodo(std::vector<Deshacer>& hechos) {
		std::error_code ec;
		for (size_t i = hechos.size(); i-- > 0;) {
			Deshacer& d = hechos[i];
			if (!d.copia.empty()) fs::copy_file(d.copia, d.destino, fs::copy_options::overwrite_existing, ec);
			else fs::remove(d.destino, ec);
			if (!d.estadoCopia.empty()) fs::copy_file(d.estadoCopia, d.estado, fs::copy_options::overwrite_existing, ec);
			else fs::remove(d.estado, ec);
		}
		hechos.clear();
	}

	nlohmann::json informeHechos(const Analisis& a) {
		nlohmann::json h = nlohmann::json::array(), s = nlohmann::json::array();
		for (const Hecho& x : a.hechos) h.push_back({ {"id", x.cambio.id}, {"player_id", x.jugador}, {"campo", x.cambio.campo}, {"antes", x.antes}, {"despues", x.cambio.valor}, {"igual", x.igual} });
		for (const Salto& x : a.saltados) s.push_back({ {"id", x.cambio.id}, {"player_id", x.cambio.jugador}, {"jugador", x.cambio.nombre}, {"campo", x.cambio.campo}, {"codigo", x.codigo}, {"motivo", x.motivo} });
		return { {"aplicados", h}, {"saltados", s} };
	}
}

Informe aplicarAlJuego(const fs::path& juego, const Lista& lista) {
	Informe inf;
	if (juego.empty()) { inf.mensaje = "No sé dónde está tu juego: no se tocó nada."; return inf; }
	inf.objetivos = analizar(juego, lista);
	std::vector<size_t> escribir, retirar;
	for (size_t i = 0; i < inf.objetivos.size(); i++) {
		const Analisis& a = inf.objetivos[i];
		if (!a.baseOk) continue;
		if (a.cambiados > 0 && a.estado != "aplicado") escribir.push_back(i);
		else if (a.cambiados == 0 && a.estado == "desfasado") retirar.push_back(i);
	}
	if (escribir.empty() && retirar.empty()) {
		inf.ok = true;
		bool algunoAplicado = false;
		for (const Analisis& a : inf.objetivos) if (a.estado == "aplicado") algunoAplicado = true;
		inf.mensaje = algunoAplicado ? "Las estadísticas ya estaban puestas en tu juego: no se tocó nada."
			: lista.cambios.empty() ? "No hay cambios de estadísticas: no se tocó el juego."
			: "Ningún cambio de estadísticas se puede aplicar en tu juego (mira los motivos de cada modo). No se tocó nada.";
		return inf;
	}
	const std::string marca = ahora("%Y%m%d-%H%M%S");
	std::vector<Deshacer> hechos;
	std::error_code ec;
	auto fallar = [&](const std::string& msg) { deshacerTodo(hechos); inf.ok = false; inf.escritos = inf.retirados = 0; inf.mensaje = msg + " Se dejó todo como estaba."; return inf; };

	for (size_t i : escribir) {
		const Analisis& a = inf.objetivos[i];
		const fs::path pesdb = dePesdb(a.objetivo.siderAddons), phx = dePhoenixDB(a.objetivo.siderAddons);
		const fs::path copiaDir = phx / ("_antes-de-fichas-" + marca);
		fs::create_directories(pesdb, ec);
		if (ec) return fallar("No se pudo crear Phoenix-DB en " + a.objetivo.nombre + " (¿sin permisos?).");
		Deshacer d;
		d.destino = pesdb / "Player.bin";
		d.estado = phx / "fichas_aplicadas.json";
		if (fs::exists(d.destino, ec)) {
			fs::create_directories(copiaDir, ec);
			d.copia = copiaDir / "Player.bin";
			if (!fs::copy_file(d.destino, d.copia, fs::copy_options::overwrite_existing, ec)) return fallar("No se pudo guardar la copia de Player.bin en " + a.objetivo.nombre + ".");
			inf.respaldos.push_back(phoenix::entrega::aU8(d.copia));
		}
		if (fs::exists(d.estado, ec)) {
			fs::create_directories(copiaDir, ec);
			d.estadoCopia = copiaDir / "fichas_aplicadas.json";
			fs::copy_file(d.estado, d.estadoCopia, fs::copy_options::overwrite_existing, ec);
		}
		hechos.push_back(d);
		std::string err;
		if (!phoenix::buzon::escribirAtomicoComo(pesdb, "Player", ".bin", a.salida, &err)) return fallar("No se pudo escribir Player.bin en " + a.objetivo.nombre + " (" + err + "; ¿PES abierto o el antivirus lo bloquea?).");
		bool hay = false;
		if (shaActual(a.objetivo.siderAddons, hay) != a.salidaSha) return fallar("Lo escrito en " + a.objetivo.nombre + " no quedó idéntico a lo previsto.");
		nlohmann::json st = {
			{"version", 1}, {"fecha", ahora("%Y-%m-%d %H:%M")}, {"modo", a.objetivo.nombre}, {"parche", a.parche}, {"baseOrigen", a.baseOrigen},
			{"baseSha", a.baseSha}, {"listaSha", lista.sha256}, {"listaFuente", lista.fuente}, {"salidaSha", a.salidaSha}, {"cambiados", a.cambiados}
		};
		st.update(informeHechos(a));
		if (!phoenix::buzon::escribirAtomicoComo(phx, "fichas_aplicadas", ".json", st.dump(1), &err)) return fallar("No se pudo guardar el estado de las estadísticas en " + a.objetivo.nombre + ".");
	}
	for (size_t i : retirar) {
		const Analisis& a = inf.objetivos[i];
		const fs::path pesdb = dePesdb(a.objetivo.siderAddons), phx = dePhoenixDB(a.objetivo.siderAddons);
		const fs::path copiaDir = phx / ("_antes-de-fichas-" + marca);
		fs::create_directories(copiaDir, ec);
		Deshacer d;
		d.destino = pesdb / "Player.bin"; d.estado = phx / "fichas_aplicadas.json";
		d.copia = copiaDir / "Player.bin"; d.estadoCopia = copiaDir / "fichas_aplicadas.json";
		if (!fs::copy_file(d.destino, d.copia, fs::copy_options::overwrite_existing, ec)) return fallar("No se pudo guardar la copia de Player.bin en " + a.objetivo.nombre + ".");
		fs::copy_file(d.estado, d.estadoCopia, fs::copy_options::overwrite_existing, ec);
		hechos.push_back(d);
		fs::remove(d.destino, ec);
		if (ec) return fallar("No se pudo quitar Player.bin en " + a.objetivo.nombre + " (¿PES abierto?).");
		fs::remove(d.estado, ec);
		inf.respaldos.push_back(phoenix::entrega::aU8(d.copia));
	}
	std::set<fs::path> inis;
	for (size_t i : escribir) inis.insert(inf.objetivos[i].objetivo.ini.parent_path());
	if (!inis.empty()) {
		const phoenix::modulos::Resultado r = phoenix::modulos::asegurarRaizPhoenixDB(std::vector<fs::path>(inis.begin(), inis.end()));
		if (!r.ok) return fallar(r.mensaje);
		for (const std::string& x : r.respaldos) inf.respaldos.push_back(x);
	}
	for (size_t i : escribir) podarCopias(dePhoenixDB(inf.objetivos[i].objetivo.siderAddons));
	for (size_t i : retirar) podarCopias(dePhoenixDB(inf.objetivos[i].objetivo.siderAddons));
	inf.ok = true;
	inf.escritos = static_cast<int>(escribir.size());
	inf.retirados = static_cast<int>(retirar.size());
	int cambiados = 0, saltados = 0;
	for (size_t i : escribir) { cambiados += inf.objetivos[i].cambiados; saltados += static_cast<int>(inf.objetivos[i].saltados.size()); }
	inf.mensaje = escribir.empty()
		? "Estadísticas devueltas a las de la base en " + std::to_string(retirar.size()) + (retirar.size() == 1 ? " sitio." : " sitios.")
		: "Estadísticas puestas en " + std::to_string(escribir.size()) + (escribir.size() == 1 ? " sitio: " : " sitios: ") + std::to_string(cambiados) + " cambios, "
			+ std::to_string(saltados) + " saltados. Se ven al cargar los datos del juego.";
	inf.objetivos = analizar(juego, lista);
	return inf;
}

Informe quitarDelJuego(const fs::path& juego) {
	Informe inf;
	if (juego.empty()) { inf.mensaje = "No sé dónde está tu juego: no se tocó nada."; return inf; }
	std::error_code ec;
	int quitados = 0;
	std::vector<std::string> notas;
	const std::string marca = ahora("%Y%m%d-%H%M%S");
	for (const fichajes::Objetivo& o : fichajes::objetivos(juego)) {
		const nlohmann::json st = leerEstado(o.siderAddons);
		const std::string guardada = st.is_object() ? st.value("salidaSha", std::string()) : std::string();
		if (guardada.empty()) continue;
		const fs::path pesdb = dePesdb(o.siderAddons), phx = dePhoenixDB(o.siderAddons);
		bool hay = false;
		const std::string actual = shaActual(o.siderAddons, hay);
		if (hay && actual != guardada) { notas.push_back(o.nombre + ": el Player.bin ya no es el que generó Link (lo cambió otra herramienta): no se borra"); continue; }
		const fs::path copiaDir = phx / ("_antes-de-fichas-" + marca);
		fs::create_directories(copiaDir, ec);
		if (hay) {
			if (!cambiarNombre(pesdb / "Player.bin", copiaDir / "Player.bin")) { inf.mensaje = "No se pudo quitar Player.bin en " + o.nombre + " (¿PES abierto?)."; return inf; }
			inf.respaldos.push_back(phoenix::entrega::aU8(copiaDir / "Player.bin"));
		}
		cambiarNombre(phx / "fichas_aplicadas.json", copiaDir / "fichas_aplicadas.json");
		podarCopias(phx);
		quitados++;
	}
	inf.ok = true;
	inf.retirados = quitados;
	inf.mensaje = quitados == 0 && notas.empty() ? "No había estadísticas generadas por Link: tu juego ya usa las de la base."
		: "Estadísticas quitadas en " + std::to_string(quitados) + (quitados == 1 ? " sitio" : " sitios") + ": el juego vuelve a las de la base (la copia queda en _antes-de-fichas-… dentro de Phoenix-DB).";
	for (const std::string& n : notas) inf.mensaje += " " + n + ".";
	return inf;
}

fs::path carpetaListaUsuario() {
	const char* ap = std::getenv("APPDATA");
	if (!ap || !*ap) return {};
	return fs::path(ap) / "Phoenix Mercado" / "fichas";
}

bool cargarLista(const fs::path& carpeta, Lista& lista, std::string& error) {
	lista = Lista();
	std::error_code ec;
	const fs::path f = carpeta / "fichas.json";
	if (carpeta.empty() || !fs::is_regular_file(f, ec)) { lista.fuente = "(no hay cambios de estadísticas)"; return true; }
	std::string t;
	if (!leerArchivo(f, t, 8 * 1024 * 1024)) { error = "No se pudo leer " + phoenix::entrega::aU8(f); return false; }
	if (t.compare(0, 3, "\xEF\xBB\xBF") == 0) t.erase(0, 3);
	if (!leerLista(t, lista, error)) { error += " (" + phoenix::entrega::aU8(f) + ")"; return false; }
	lista.fuente = phoenix::entrega::aU8(f);
	return true;
}

} // namespace phoenix::fichas
