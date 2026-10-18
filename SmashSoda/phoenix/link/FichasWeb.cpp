#include "FichasWeb.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace phoenix::fichas::web {

using json = nlohmann::json;

namespace {
	bool esUuid(const std::string& s) {
		if (s.size() != 36) return false;
		for (size_t i = 0; i < s.size(); i++) {
			const char c = s[i];
			if (i == 8 || i == 13 || i == 18 || i == 23) { if (c != '-') return false; }
			else if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
		}
		return true;
	}
	/// Corta a `max` bytes sin partir una letra UTF-8.
	std::string cortar(std::string s, size_t max) {
		if (s.size() <= max) return s;
		size_t n = max;
		while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) n--;
		s.resize(n);
		return s;
	}
	CambioWeb deJson(const json& c) {
		CambioWeb x;
		x.seq = c.value("seq", 0LL);
		x.id = c.value("id", std::string());
		x.tipo = c.value("tipo", std::string());
		x.destino = c.value("destino", std::string());
		x.estado = c.value("estado", std::string("activo"));
		x.datos = c.contains("datos") ? c["datos"] : json::object();
		return x;
	}
	bool nosInteresa(const CambioWeb& x) {
		return (x.tipo == "stats" || x.tipo == "info") && (x.destino == "base" || x.destino == "ambos") && x.estado == "activo"
			&& x.seq > 0 && esUuid(x.id) && x.datos.is_object() && x.datos.contains("valores") && x.datos["valores"].is_object();
	}
	/// Clave del jugador: id real si lo hay, si no el nombre plegado.
	std::string claveJugador(const json& d) {
		const long long id = d.contains("jugador_id") && d["jugador_id"].is_number_integer() ? d["jugador_id"].get<long long>() : 0;
		if (id > 0 && id < kIdSintetico) return "id:" + std::to_string(id);
		return "n:" + fichajes::plegarNombre(d.value("jugador", std::string()));
	}
}

int leerPagina(const std::string& respuesta, Almacen& a, std::string& error) {
	const json j = json::parse(respuesta, nullptr, false);
	if (j.is_discarded() || !j.is_object() || !j.contains("cambios") || !j["cambios"].is_array()) { error = "respuesta de /v1/cambios no esperada"; return -1; }
	if (j.contains("ok") && j["ok"].is_boolean() && !j["ok"].get<bool>()) { error = j.value("error", std::string("la web dijo que no")); return -1; }
	long long ultimo = a.ultimoSeq;
	int n = 0;
	for (const json& c : j["cambios"]) {
		if (!c.is_object()) continue;
		n++;
		const CambioWeb x = deJson(c);
		ultimo = (std::max)(ultimo, x.seq);
		if (c.value("version", 1) != 1) continue;   // versión del contrato que este Link no entiende: se ignora
		if (nosInteresa(x)) a.cambios[x.seq] = x;
		else a.cambios.erase(x.seq);
	}
	if (j.contains("ultimo_seq") && j["ultimo_seq"].is_number_integer() && n == 0) ultimo = (std::max)(ultimo, j["ultimo_seq"].get<long long>());
	a.ultimoSeq = ultimo;
	return n;
}

std::string guardar(const Almacen& a) {
	json arr = json::array();
	for (const auto& kv : a.cambios) arr.push_back({ {"seq", kv.second.seq}, {"id", kv.second.id}, {"tipo", kv.second.tipo}, {"destino", kv.second.destino}, {"estado", kv.second.estado}, {"datos", kv.second.datos} });
	return json({ {"version", 1}, {"ultimo_seq", a.ultimoSeq}, {"cambios", arr} }).dump(1);
}

bool leer(const std::string& texto, Almacen& a) {
	a = Almacen();
	const json j = json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object() || !j.contains("cambios") || !j["cambios"].is_array()) return false;
	a.ultimoSeq = j.value("ultimo_seq", 0LL);
	for (const json& c : j["cambios"]) if (c.is_object()) { const CambioWeb x = deJson(c); if (nosInteresa(x)) a.cambios[x.seq] = x; }
	return true;
}

Lista listaDesde(const Almacen& a) {
	struct Hueco { long long seq; Cambio c; };
	std::map<std::string, Hueco> vigente;   // jugador|campo → último valor
	for (const auto& kv : a.cambios) {
		const CambioWeb& x = kv.second;
		const std::string jug = claveJugador(x.datos);
		for (auto it = x.datos["valores"].begin(); it != x.datos["valores"].end(); ++it) {
			const std::string clave = jug + "|" + it.key();
			if (it.value().is_null()) { vigente.erase(clave); continue; }
			if (!it.value().is_number_integer()) continue;
			Cambio c;
			c.id = x.id;
			const long long id = x.datos.contains("jugador_id") && x.datos["jugador_id"].is_number_integer() ? x.datos["jugador_id"].get<long long>() : 0;
			c.jugador = (id > 0 && id < kIdSintetico) ? static_cast<uint32_t>(id) : 0;
			c.nombre = x.datos.value("jugador", std::string());
			c.campo = it.key();
			c.valor = it.value().get<int>();
			vigente[clave] = { x.seq, c };
		}
	}
	std::vector<Hueco> v;
	for (auto& kv : vigente) v.push_back(kv.second);
	std::stable_sort(v.begin(), v.end(), [](const Hueco& p, const Hueco& q) { return p.seq < q.seq; });
	Lista l;
	for (const Hueco& h : v) l.cambios.push_back(h.c);
	l.fuente = "web (/v1/cambios)";
	l.sha256 = std::to_string(a.ultimoSeq) + ":" + std::to_string(l.cambios.size());
	return l;
}

std::vector<ResultadoCambio> resultados(const Almacen& a, const std::vector<Analisis>& analisis, bool pendiente, const std::string& motivoPendiente) {
	std::vector<ResultadoCambio> r;
	const Lista l = listaDesde(a);
	std::map<std::string, int> enLista;
	for (const Cambio& c : l.cambios) enLista[c.id]++;
	const Analisis* primero = nullptr;
	for (const Analisis& an : analisis) if (an.baseOk) { primero = &an; break; }
	for (const auto& kv : a.cambios) {
		const CambioWeb& x = kv.second;
		ResultadoCambio rc;
		rc.cambioId = x.id;
		bool todosNull = true;
		for (const auto& v : x.datos["valores"]) if (!v.is_null()) todosNull = false;
		if (!enLista.count(x.id)) {
			if (pendiente) { rc.resultado = "pendiente"; rc.motivo = motivoPendiente; }
			else if (todosNull) { rc.resultado = "aplicado"; rc.motivo = "vuelve a los valores de la base"; rc.jugadores = 1; }
			else { rc.resultado = "reemplazado"; rc.motivo = "un cambio posterior de la web lo pisó"; }
			r.push_back(rc); continue;
		}
		if (pendiente) { rc.resultado = "pendiente"; rc.motivo = cortar(motivoPendiente, 200); r.push_back(rc); continue; }
		if (!primero) { rc.resultado = "error"; rc.motivo = "ningún modo del juego tiene una base que se pueda leer"; r.push_back(rc); continue; }
		int cambiados = 0, iguales = 0, saltos = 0;
		std::string motivo;
		std::set<std::string> vistos;
		for (const Hecho& h : primero->hechos) {
			if (h.cambio.id != x.id) continue;
			if (h.igual) iguales++; else cambiados++;
			if (vistos.insert(h.cambio.campo).second)
				motivo += (motivo.empty() ? "" : ", ") + h.cambio.campo + " " + std::to_string(h.antes) + "→" + std::to_string(h.cambio.valor);
		}
		for (const Salto& s : primero->saltados) {
			if (s.cambio.id != x.id) continue;
			saltos++;
			motivo += (motivo.empty() ? "" : ", ") + s.cambio.campo + ": " + s.codigo;
		}
		if (saltos == 0 && cambiados > 0) rc.resultado = "aplicado";
		else if (saltos == 0 && iguales > 0) rc.resultado = "ya_estaba";
		else if (saltos > 0 && cambiados + iguales > 0) rc.resultado = "parcial";
		else rc.resultado = "saltado";
		rc.jugadores = cambiados + iguales > 0 ? 1 : 0;
		rc.motivo = cortar(motivo, 200);
		r.push_back(rc);
	}
	return r;
}

} // namespace phoenix::fichas::web
