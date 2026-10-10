#include "ClienteSync.h"
#include "Entrega.h"

#include <algorithm>
#include <regex>

using json = nlohmann::json;

namespace mercado::sync {

	namespace {
		/// Nunca deja pasar un token al registro: Bearer …, phx_…, pml_…
		std::string sinSecretos(std::string t) {
			static const std::regex bearer(R"(Bearer\s+\S+)", std::regex::icase);
			static const std::regex tok(R"((phx|pml)_[A-Za-z0-9_\-]+)");
			t = std::regex_replace(t, bearer, "Bearer ***");
			return std::regex_replace(t, tok, "$1_***");
		}
	}

	// --- Ritmo -----------------------------------------------------------------------
	bool Ritmo::puede(const std::string& ruta, int64_t ahora) const {
		auto it = _e.find(ruta);
		return it == _e.end() || ahora >= it->second.siguiente;
	}
	void Ritmo::exito(const std::string& ruta, int64_t ahora) { _e[ruta] = { ahora + 5, 0 }; }
	void Ritmo::fallo(const std::string& ruta, int64_t ahora, int esperaForzada) {
		auto& e = _e[ruta];
		e.fallos++;
		int64_t espera = 2;
		for (int i = 1; i < e.fallos && espera < 300; i++) espera *= 2;
		espera = std::min<int64_t>(espera, 300);
		if (esperaForzada > 0) espera = std::max<int64_t>(5, std::min<int64_t>(esperaForzada, 3600));
		e.siguiente = ahora + espera;
	}
	int64_t Ritmo::siguiente(const std::string& ruta) const { auto it = _e.find(ruta); return it == _e.end() ? 0 : it->second.siguiente; }
	int Ritmo::fallosSeguidos(const std::string& ruta) const { auto it = _e.find(ruta); return it == _e.end() ? 0 : it->second.fallos; }

	std::string codificarUrl(const std::string& s) {
		static const char* hex = "0123456789ABCDEF";
		std::string r;
		for (unsigned char c : s) {
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') r += static_cast<char>(c);
			else { r += '%'; r += hex[c >> 4]; r += hex[c & 15]; }
		}
		return r;
	}

	// --- Cliente ----------------------------------------------------------------------
	ClienteSync::ClienteSync(ClienteHttp& http, FuenteToken& token, Registro registro, std::string base)
		: _http(http), _token(token), _log(std::move(registro)), _base(std::move(base)) {}

	Resultado<json> ClienteSync::llamar(const std::string& metodo, const std::string& ruta, const std::string& cuerpo,
		const std::string& etag, bool* sinCambios) {
		using R = Resultado<json>;
		_reintentarEn = 0;
		if (sinCambios) *sinCambios = false;
		const std::string t = _token.token();
		if (t.empty()) return R::mal("NO_VINCULADO", "Esta PC no está vinculada con Phoenix Link");
		std::vector<std::string> cab = { "Authorization: Bearer " + t, std::string("X-Phoenix-Version: sync-") + kVersionSync };
		if (!cuerpo.empty()) cab.push_back("Content-Type: application/json");
		if (!etag.empty()) cab.push_back("If-None-Match: " + etag);

		const RespuestaHttp r = _http.peticion(metodo, _base + ruta, cuerpo, cab);
		auto log = [&](const std::string& n, const std::string& x) { if (_log) _log(n, sinSecretos(x)); };
		if (r.estado == 0) { log("warn", metodo + " " + ruta + " sin respuesta: " + r.errorRed); return R::mal("RED", r.errorRed); }
		if (r.estado == 304) { if (sinCambios) *sinCambios = true; return R::bien(json::object()); }

		const json j = json::parse(r.cuerpo, nullptr, false);
		if (r.estado == 404 && (j.is_discarded() || !j.is_object() || !j.contains("ok")))
			return R::mal("RUTA_NO_ENCONTRADA", "La web todavía no tiene esta función", 404);
		if (j.is_discarded() || !j.is_object() || !j.contains("ok") || !j["ok"].is_boolean())
			return R::mal("RESPUESTA_INVALIDA", "HTTP " + std::to_string(r.estado), r.estado);
		if (!j["ok"].get<bool>()) {
			std::string codigo = "ERROR_DESCONOCIDO", mensaje;
			if (j.contains("error") && j["error"].is_object()) {
				const auto& e = j["error"];
				codigo = e.value("codigo", codigo);
				mensaje = e.contains("mensaje") && e["mensaje"].is_string() ? e["mensaje"].get<std::string>() : "";
				if (e.contains("reintentar_en") && e["reintentar_en"].is_number()) _reintentarEn = e["reintentar_en"].get<int>();
			}
			if (r.estado == 429 && _reintentarEn <= 0) _reintentarEn = 60;
			log("warn", metodo + " " + ruta + " → " + codigo);
			return R::mal(codigo, mensaje, r.estado);
		}
		if (!j.contains("datos")) return R::mal("RESPUESTA_INVALIDA", "Falta «datos»", r.estado);
		return R::bien(j["datos"]);
	}

	Resultado<grupo::Config> ClienteSync::config(const std::string& grupoId, const std::string& etag, bool& sinCambios) {
		auto r = llamar("GET", "/v1/sync/config?grupo_id=" + codificarUrl(grupoId), "", etag, &sinCambios);
		if (!r.ok()) return Resultado<grupo::Config>{ std::nullopt, r.error };
		if (sinCambios) return Resultado<grupo::Config>::bien({});
		return Resultado<grupo::Config>::bien(grupo::parsearConfig(*r.valor));
	}

	Resultado<Lote> ClienteSync::operaciones(const std::string& grupoId, int64_t desde, const std::string& etag) {
		using R = Resultado<Lote>;
		bool sin = false;
		auto r = llamar("GET", "/v1/sync/operaciones?grupo_id=" + codificarUrl(grupoId) + "&desde=" + std::to_string(desde), "", etag, &sin);
		if (!r.ok()) return R{ std::nullopt, r.error };
		Lote l;
		if (sin) { l.sinCambios = true; l.hasta = desde; l.etag = etag; return R::bien(std::move(l)); }
		const json& d = *r.valor;
		if (!d.is_object() || !d.contains("operaciones") || !d["operaciones"].is_array()) return R::mal("RESPUESTA_INVALIDA", "Falta «operaciones»");
		for (const auto& o : d["operaciones"]) {
			auto op = grupo::deJson(o);
			if (op.ok() && op.valor->seq > desde) l.operaciones.push_back(*op.valor);
			else if (!op.ok()) l.invalidas.push_back(op.error.detalle);
		}
		std::sort(l.operaciones.begin(), l.operaciones.end(), [](const auto& a, const auto& b) { return a.seq < b.seq; });
		l.hasta = d.contains("hasta") && d["hasta"].is_number_integer() ? d["hasta"].get<int64_t>() : desde;
		for (const auto& o : l.operaciones) l.hasta = std::max(l.hasta, o.seq);
		l.etag = d.contains("etag") && d["etag"].is_string() ? d["etag"].get<std::string>() : "";
		return R::bien(std::move(l));
	}

	Resultado<int64_t> ClienteSync::publicar(const std::string& grupoId, const grupo::Operacion& op) {
		json cuerpo = grupo::aJson(op);
		cuerpo["grupo_id"] = grupoId;
		cuerpo.erase("seq");
		cuerpo.erase("autor");   // el autor lo pone la web a partir del token
		auto r = llamar("POST", "/v1/sync/operaciones", cuerpo.dump(), "", nullptr);
		if (!r.ok()) return Resultado<int64_t>{ std::nullopt, r.error };
		const json& d = *r.valor;
		return Resultado<int64_t>::bien(d.is_object() && d.contains("seq") && d["seq"].is_number_integer() ? d["seq"].get<int64_t>() : 0);
	}

	Resultado<bool> ClienteSync::confirmar(const std::string& grupoId, const std::string& opId, const std::string& estado, const std::string& motivo) {
		const json cuerpo = { { "grupo_id", grupoId }, { "op_id", opId }, { "estado", estado }, { "motivo", entrega::recortarResumen(motivo, 200) } };
		auto r = llamar("POST", "/v1/sync/operaciones/aplicada", cuerpo.dump(), "", nullptr);
		if (!r.ok()) return Resultado<bool>{ std::nullopt, r.error };
		return Resultado<bool>::bien(true);
	}

}
