#pragma once

#include <string>
#include <vector>
#include "../core/Tipos.h"

// =============================================================================
//  Phoenix Sync · Piezas que solo existen en Windows
// -----------------------------------------------------------------------------
//  HttpWinHttp  → HTTPS con WinHTTP (viene con Windows, cero dependencias).
//                 Arregla dos límites del Http.cpp de Phoenix Link: conserva el
//                 «?token=…» de las URLs firmadas y descarga a disco sin tope de 1 MB.
//  TokenCompartido → lee el token de Phoenix Link (phoenix-token.dat). SOLO LEE:
//                    nunca lo escribe ni lo borra, para no desvincular la sala.
//  TokenManager    → token propio de Mercado (segundo código), cifrado con DPAPI
//                    en %APPDATA%\Phoenix Mercado\mercado-token.dat (nombre antiguo a propósito: así no se pierde el token)
// =============================================================================

namespace mercado::windows {

	class HttpWinHttp : public ClienteHttp {
	public:
		explicit HttpWinHttp(int timeoutMs = 8000) : _timeoutMs(timeoutMs) {}
		RespuestaHttp peticion(const std::string& metodo, const std::string& url,
			const std::string& cuerpo, const std::vector<std::string>& cabeceras) override;
		bool descargar(const std::string& url, const std::string& rutaDestino,
			long long maxBytes, std::string& error) override;
	private:
		int _timeoutMs;
	};

	class TokenCompartido : public FuenteToken {
	public:
		std::string token() override;
		bool guardar(const std::string&) override { return false; } // lo gestiona Phoenix Link
		void borrar() override {}                                   // idem: jamás lo borra
		std::string nombreModo() const override { return "compartido"; }
	};

	class TokenManager : public FuenteToken {
	public:
		std::string token() override;
		bool guardar(const std::string& token) override;
		void borrar() override;
		std::string nombreModo() const override { return "codigoManager"; }
	};

	std::string nombrePc();
	/// %APPDATA%\Phoenix Mercado\  (la crea si falta; se mantiene el nombre antiguo por compatibilidad)
	std::string carpetaDatos();
	/// Carpetas «Documentos» candidatas: la de Windows (puede estar en OneDrive), %USERPROFILE%\Documents y las de OneDrive.
	std::vector<std::string> carpetasDocumentos();
	/// ¿Hay un PES2021.exe abierto?
	bool juegoAbierto();

}
