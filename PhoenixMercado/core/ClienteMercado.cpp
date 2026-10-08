#include "ClienteMercado.h"
#include "Sha256.h"

#include <filesystem>
#include <regex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mercado {

	std::string limpiarSecretos(std::string texto) {
		static const std::regex bearer(R"(Bearer\s+\S+)", std::regex::icase);
		static const std::regex pml(R"(pml_[A-Za-z0-9_\-]+)");
		texto = std::regex_replace(texto, bearer, "Bearer ***");
		return std::regex_replace(texto, pml, "pml_***");
	}

	ClienteMercado::ClienteMercado(ClienteHttp& http, FuenteToken& token, Registro registro, std::string baseUrl)
		: _http(http), _token(token), _registro(std::move(registro)), _base(std::move(baseUrl)) {}

	void ClienteMercado::log(const std::string& nivel, const std::string& texto) {
		if (_registro) _registro(nivel, limpiarSecretos(texto));
	}

	Resultado<std::string> ClienteMercado::llamar(const std::string& metodo, const std::string& ruta,
		const std::string& cuerpo, bool conToken) {
		using R = Resultado<std::string>;
		std::vector<std::string> cab = { std::string("X-Mercado-Version: ") + kVersion };
		if (conToken) {
			const std::string t = _token.token();
			if (t.empty()) return R::mal("NO_VINCULADO", "Esta PC no está vinculada (modo " + _token.nombreModo() + ")");
			cab.push_back("Authorization: Bearer " + t);
		}

		const RespuestaHttp r = _http.peticion(metodo, _base + ruta, cuerpo, cab);
		if (r.estado == 0) {
			log("warn", metodo + " " + ruta + " sin respuesta: " + r.errorRed);
			return R::mal("RED", r.errorRed);
		}

		const json j = json::parse(r.cuerpo, nullptr, false);
		if (j.is_discarded() || !j.is_object() || !j.contains("ok") || !j["ok"].is_boolean()) {
			log("error", metodo + " " + ruta + " respuesta no válida (HTTP " + std::to_string(r.estado) + ")");
			return R::mal("RESPUESTA_INVALIDA", "La web respondió algo que no es el formato esperado", r.estado);
		}
		if (!j["ok"].get<bool>()) {
			std::string codigo = "ERROR_DESCONOCIDO";
			if (j.contains("error") && j["error"].is_object()) codigo = j["error"].value("codigo", codigo);
			log("warn", metodo + " " + ruta + " → " + codigo);
			// Token rechazado por la web: se borra para obligar a re-vincular (igual que PhoenixLink).
			if (codigo == "TOKEN_INVALIDO" || codigo == "TOKEN_REVOCADO") _token.borrar();
			return R::mal(codigo, j.contains("error") ? j["error"].dump() : "", r.estado);
		}
		if (!j.contains("datos")) return R::mal("RESPUESTA_INVALIDA", "Falta «datos»", r.estado);
		log("info", metodo + " " + ruta + " ok");
		return R::bien(j["datos"].dump());
	}

	Resultado<Eco> ClienteMercado::eco() {
		auto r = llamar("GET", "/eco", "", false);
		if (!r.ok()) return Resultado<Eco>{ std::nullopt, r.error };
		const json d = json::parse(*r.valor);
		return Resultado<Eco>::bien({ d.value("version_api", ""), d.value("hora", "") });
	}

	Resultado<Usuario> ClienteMercado::vincular(const std::string& codigo, const std::string& nombrePc) {
		const json cuerpo = { {"codigo", codigo}, {"nombre_pc", nombrePc.substr(0, 40)} };
		auto r = llamar("POST", "/vincular", cuerpo.dump(), false);
		if (!r.ok()) return Resultado<Usuario>{ std::nullopt, r.error };
		const json d = json::parse(*r.valor);
		const std::string token = d.value("token", "");
		if (token.empty()) return Resultado<Usuario>::mal("RESPUESTA_INVALIDA", "La web no devolvió token");
		if (!_token.guardar(token)) return Resultado<Usuario>::mal("TOKEN_NO_GUARDADO", "No se pudo cifrar/guardar el token");
		const json u = d.value("usuario", json::object());
		return Resultado<Usuario>::bien({ u.value("id", ""), u.value("nombre", "") });
	}

	Resultado<Usuario> ClienteMercado::yo() {
		auto r = llamar("GET", "/yo", "", true);
		if (!r.ok()) return Resultado<Usuario>{ std::nullopt, r.error };
		const json u = json::parse(*r.valor).value("usuario", json::object());
		return Resultado<Usuario>::bien({ u.value("id", ""), u.value("nombre", "") });
	}

	Resultado<OptionOficial> ClienteMercado::optionActual() {
		auto r = llamar("GET", "/option/actual", "", true);
		if (!r.ok()) return Resultado<OptionOficial>{ std::nullopt, r.error };
		const json d = json::parse(*r.valor);
		OptionOficial o;
		o.version = d.value("version", "");
		o.sha256 = d.value("sha256", "");
		o.url = d.value("url", "");
		o.notas = d.contains("notas") && d["notas"].is_string() ? d["notas"].get<std::string>() : "";
		o.tamano = d.value("tamano", 0LL);
		o.expiraEnSeg = d.value("expira_en_seg", 0);
		if (o.url.empty() || o.sha256.size() != 64) return Resultado<OptionOficial>::mal("RESPUESTA_INVALIDA", "Falta url o sha256");
		return Resultado<OptionOficial>::bien(o);
	}

	Resultado<std::string> ClienteMercado::descargarOption(const OptionOficial& oficial, const std::string& rutaDestino) {
		using R = Resultado<std::string>;
		namespace fs = std::filesystem;
		const fs::path destino = aRuta(rutaDestino);
		if (fs::exists(destino)) return R::mal("DESTINO_OCUPADO", "No se sobrescribe: " + rutaDestino);

		std::string err;
		const long long maximo = oficial.tamano > 0 ? oficial.tamano + 1024 : 64LL * 1024 * 1024;
		if (!_http.descargar(oficial.url, rutaDestino, maximo, err)) {
			std::error_code ec; fs::remove(destino, ec);
			log("warn", "descarga option falló: " + err);
			return R::mal("DESCARGA_FALLIDA", err);
		}
		const std::string huella = sha256::deArchivo(rutaDestino, &err);
		if (huella != oficial.sha256) {
			std::error_code ec; fs::remove(destino, ec); // solo lo que acabamos de bajar
			log("error", "option descargado con huella distinta");
			return R::mal("OPTION_HUELLA_DISTINTA", "esperado " + oficial.sha256 + " · obtenido " + huella);
		}
		log("info", "option " + oficial.version + " descargado y verificado");
		return R::bien(huella);
	}


	Resultado<std::string> ClienteMercado::subirCatalogo(const std::string& cuerpo) { return llamar("POST", "/catalogo", cuerpo, true); }
	Resultado<std::string> ClienteMercado::subirEquivalencias(const std::string& cuerpo) { return llamar("POST", "/equivalencias", cuerpo, true); }
	Resultado<std::string> ClienteMercado::plantillas() {
		auto r = llamar("GET", "/plantillas", "", true);
		if (!r.ok()) return r;
		const json d = json::parse(*r.valor, nullptr, false);
		if (d.is_discarded() || !d.contains("contenido") || !d["contenido"].is_string())
			return Resultado<std::string>::mal("RESPUESTA_INVALIDA", "Falta «contenido»");
		return Resultado<std::string>::bien(d["contenido"].get<std::string>());
	}

}
