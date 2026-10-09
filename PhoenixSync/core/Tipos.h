#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Sync · Tipos básicos del Core
// -----------------------------------------------------------------------------
//  El Core NO conoce Windows, ImGui ni Parsec. Todo lo que depende del sistema
//  (HTTP, cifrado del token) entra por las interfaces de este archivo.
//  Así se prueba solo y se integra en Phoenix Link sin cambios.
// =============================================================================

namespace mercado {

	/// Rutas en UTF-8 (acentos, ñ) sin depender de las funciones u8 de C++20.
	inline std::filesystem::path aRuta(const std::string& utf8) {
		return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
	}
	inline std::string deRuta(const std::filesystem::path& p) {
		const std::u8string u = p.u8string();
		return std::string(u.begin(), u.end());
	}

	/// Error estable: `codigo` viene del contrato de la web (p. ej. TOKEN_INVALIDO)
	/// o es local (RED, RESPUESTA_INVALIDA, ARCHIVO_…).
	struct Error {
		std::string codigo;
		std::string detalle;
		int http = 0;
	};

	/// O trae un valor, o trae un error. Nunca las dos cosas.
	template <typename T>
	struct Resultado {
		std::optional<T> valor;
		Error error;
		bool ok() const { return valor.has_value(); }
		static Resultado bien(T v) { Resultado r; r.valor = std::move(v); return r; }
		static Resultado mal(std::string codigo, std::string detalle = "", int http = 0) {
			Resultado r; r.error = { std::move(codigo), std::move(detalle), http }; return r;
		}
	};

	// --- HTTP (lo implementa windows/HttpWinHttp) ---------------------------
	struct RespuestaHttp {
		int estado = 0;          ///< 0 = no hubo respuesta (red, timeout)
		std::string cuerpo;
		std::string errorRed;
	};

	class ClienteHttp {
	public:
		virtual ~ClienteHttp() = default;
		virtual RespuestaHttp peticion(const std::string& metodo, const std::string& url,
			const std::string& cuerpo, const std::vector<std::string>& cabeceras) = 0;
		/// Descarga a disco (sin límite de 1 MB). Devuelve false y rellena `error`.
		virtual bool descargar(const std::string& url, const std::string& rutaDestino,
			long long maxBytes, std::string& error) = 0;
	};

	// --- Token ---------------------------------------------------------------
	/// De dónde sale el token. Dos modos (ver README):
	///   compartido     → el mismo token de Phoenix Link (una sola vinculación)
	///   codigoManager  → token propio de Mercado (segundo código, más seguridad)
	class FuenteToken {
	public:
		virtual ~FuenteToken() = default;
		virtual std::string token() = 0;          ///< vacío = no vinculado
		virtual bool guardar(const std::string& token) = 0;
		virtual void borrar() = 0;
		virtual std::string nombreModo() const = 0;
	};

	/// Registro (log). El Core NUNCA le pasa el token; además `Registro::limpiar`
	/// borra cualquier "Bearer …" o "pml_…" por si acaso.
	using Registro = std::function<void(const std::string& nivel, const std::string& texto)>;

}
