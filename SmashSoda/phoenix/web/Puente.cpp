#include "Puente.h"

namespace phoenix::web {

	Puente::Puente(Enviar enviar) : _enviar(std::move(enviar)) {}

	void Puente::registrar(const std::string& nombre, Accion accion) {
		_acciones[nombre] = std::move(accion);
	}

	bool Puente::existe(const std::string& nombre) const {
		return _acciones.find(nombre) != _acciones.end();
	}

	void Puente::enviarJson(const json& j) {
		if (!_enviar) return;
		try {
			// replace: un texto con UTF-8 inválido (nombre raro de Parsec) no debe tumbar el envío
			_enviar(j.dump(-1, ' ', false, json::error_handler_t::replace));
		}
		catch (...) {
			_errores++;
		}
	}

	void Puente::recibir(const std::string& texto) {
		_recibidos++;
		json msg = json::parse(texto, nullptr, false);
		// WebView2 entrega lo publicado con postMessage(objeto) como JSON; si llegó un
		// texto JSON (postMessage("…")), viene doblemente codificado.
		if (!msg.is_discarded() && msg.is_string()) msg = json::parse(msg.get<std::string>(), nullptr, false);
		if (msg.is_discarded() || !msg.is_object()) {
			_errores++;
			return;
		}

		const std::string tipo = msg.value("t", "");
		if (tipo == "hola") {
			_listo = true;
			_ultimoEstado.clear(); // la interfaz es nueva: el próximo estado va completo
			json datos = json::object();
			try {
				if (_alSaludar) datos = _alSaludar(msg);
			}
			catch (...) {
				_errores++;
			}
			enviarJson({ {"t", "bienvenida"}, {"datos", datos} });
			return;
		}
		if (tipo != "pedir") {
			_errores++;
			return;
		}

		uint64_t id = 0;
		if (msg.contains("id") && msg["id"].is_number_unsigned()) id = msg["id"].get<uint64_t>();
		else if (msg.contains("id") && msg["id"].is_number_integer() && msg["id"].get<int64_t>() >= 0) id = static_cast<uint64_t>(msg["id"].get<int64_t>());
		const std::string nombre = msg.value("accion", "");
		const json datos = msg.contains("datos") && msg["datos"].is_object() ? msg["datos"] : json::object();

		auto it = _acciones.find(nombre);
		if (it == _acciones.end()) {
			responderError(id, "ACCION_DESCONOCIDA", "La acción «" + nombre + "» no existe.");
			return;
		}
		try {
			std::optional<json> r = it->second(datos, id);
			if (r.has_value()) responder(id, *r);
		}
		catch (const ErrorAccion& e) {
			responderError(id, e.codigo(), e.what());
		}
		catch (const json::exception& e) {
			// datos con el tipo equivocado (p. ej. texto donde iba número)
			responderError(id, "DATOS_INVALIDOS", e.what());
		}
		catch (const std::exception& e) {
			responderError(id, "ERROR_INTERNO", e.what());
		}
		catch (...) {
			responderError(id, "ERROR_INTERNO", "Error inesperado.");
		}
	}

	void Puente::responder(uint64_t id, const json& datos) {
		enviarJson({ {"t", "resp"}, {"id", id}, {"ok", true}, {"datos", datos} });
	}

	void Puente::responderError(uint64_t id, const std::string& codigo, const std::string& mensaje) {
		_errores++;
		enviarJson({ {"t", "resp"}, {"id", id}, {"ok", false},
			{"error", { {"codigo", codigo}, {"mensaje", mensaje.empty() ? codigo : mensaje} }} });
	}

	bool Puente::publicarEstado(const json& estado, bool forzar) {
		if (!_listo) return false;
		std::string texto;
		try {
			texto = estado.dump(-1, ' ', false, json::error_handler_t::replace);
		}
		catch (...) {
			_errores++;
			return false;
		}
		if (!forzar && texto == _ultimoEstado) return false;
		_ultimoEstado = texto;
		if (_enviar) {
			try { _enviar(std::string("{\"t\":\"estado\",\"datos\":") + texto + "}"); }
			catch (...) { _errores++; return false; }
		}
		return true;
	}

	void Puente::evento(const std::string& nombre, const json& datos) {
		if (!_listo) return;
		enviarJson({ {"t", "evento"}, {"nombre", nombre}, {"datos", datos} });
	}

	void Puente::reiniciar() {
		_listo = false;
		_ultimoEstado.clear();
	}

}
