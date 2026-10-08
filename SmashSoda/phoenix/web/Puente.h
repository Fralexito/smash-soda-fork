#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

// =============================================================================
//  Phoenix Link · Puente entre la interfaz web (WebView2) y el motor C++
// -----------------------------------------------------------------------------
//  Protocolo (todo en JSON, texto UTF-8):
//
//    interfaz → C++   {"t":"hola", "version":"…"}                  (al cargar)
//                     {"t":"pedir","id":7,"accion":"sala.abrir","datos":{…}}
//
//    C++ → interfaz   {"t":"bienvenida","datos":{…}}               (respuesta a hola)
//                     {"t":"resp","id":7,"ok":true,"datos":{…}}
//                     {"t":"resp","id":7,"ok":false,"error":{"codigo":"…","mensaje":"…"}}
//                     {"t":"estado","datos":{…}}                   (foto completa, solo si cambió)
//                     {"t":"evento","nombre":"chat","datos":{…}}
//
//  Esta clase no sabe nada de Windows ni de Smash Soda: solo enruta. Por eso
//  se prueba en Linux (pruebas/puente_test.cpp).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	/// Error controlado de una acción. `codigo` es estable (la interfaz lo traduce).
	class ErrorAccion : public std::runtime_error {
	public:
		explicit ErrorAccion(std::string codigo, const std::string& mensaje = "")
			: std::runtime_error(mensaje.empty() ? codigo : mensaje), _codigo(std::move(codigo)) {}
		const std::string& codigo() const { return _codigo; }
	private:
		std::string _codigo;
	};

	class Puente {
	public:
		/// Envía texto al WebView.
		using Enviar = std::function<void(const std::string&)>;
		/// Una acción devuelve sus datos, o std::nullopt si responderá más tarde
		/// con `responder(id, …)` (p. ej. cuando espera a la web).
		using Accion = std::function<std::optional<json>(const json& datos, uint64_t idPeticion)>;
		/// Al recibir «hola»: devuelve los datos de bienvenida.
		using AlSaludar = std::function<json(const json& hola)>;

		explicit Puente(Enviar enviar);

		void registrar(const std::string& nombre, Accion accion);
		bool existe(const std::string& nombre) const;
		size_t totalAcciones() const { return _acciones.size(); }

		void alSaludar(AlSaludar fn) { _alSaludar = std::move(fn); }

		/// Mensaje que llega del WebView (JSON en texto). Nunca lanza.
		void recibir(const std::string& texto);

		/// Responde una petición que quedó pendiente.
		void responder(uint64_t id, const json& datos);
		void responderError(uint64_t id, const std::string& codigo, const std::string& mensaje = "");

		/// Envía la foto del estado solo si cambió desde la última (o si `forzar`).
		/// Devuelve true si se envió.
		bool publicarEstado(const json& estado, bool forzar = false);

		void evento(const std::string& nombre, const json& datos);

		/// La interfaz ya saludó (está lista para recibir estado y eventos).
		bool listo() const { return _listo; }
		/// La interfaz se recargó o se cerró: hay que volver a saludar.
		void reiniciar();

		/// Contadores para diagnóstico.
		uint64_t recibidos() const { return _recibidos; }
		uint64_t errores() const { return _errores; }

	private:
		void enviarJson(const json& j);

		Enviar _enviar;
		AlSaludar _alSaludar;
		std::map<std::string, Accion> _acciones;
		std::string _ultimoEstado;
		bool _listo = false;
		uint64_t _recibidos = 0;
		uint64_t _errores = 0;
	};

}
