#pragma once

#include <string>
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · Cliente de la API /mercado/v1
// -----------------------------------------------------------------------------
//  Respuestas de la web:  { ok:true, datos:{…} }  |  { ok:false, error:{codigo} }
//  Cabeceras: Authorization: Bearer <token> · X-Mercado-Version: <versión>
//  Bloqueante: en Phoenix Link se llama desde un hilo propio, nunca desde la UI.
// =============================================================================

namespace mercado {

	inline constexpr const char* kVersion = "0.1.0";
	inline constexpr const char* kBaseUrl = "https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/mercado/v1";

	struct Eco { std::string versionApi, hora; };
	struct Usuario { std::string id, nombre; };
	struct OptionOficial {
		std::string version, sha256, url, notas;
		long long tamano = 0;
		int expiraEnSeg = 0;
	};

	class ClienteMercado {
	public:
		ClienteMercado(ClienteHttp& http, FuenteToken& token, Registro registro = nullptr,
			std::string baseUrl = kBaseUrl);

		Resultado<Eco> eco();
		/// Solo modo codigoManager: canjea el código de 8 caracteres por un token.
		Resultado<Usuario> vincular(const std::string& codigo, const std::string& nombrePc);
		Resultado<Usuario> yo();
		Resultado<OptionOficial> optionActual();

		/// POST /catalogo (solo staff). `cuerpo` = un lote de lotesCatalogo(). Devuelve el JSON de «datos».
		Resultado<std::string> subirCatalogo(const std::string& cuerpo);
		/// GET /plantillas → texto firmado. Devuelve el «contenido» (JSON) tal cual llegó.
		/// Pendiente: verificar la firma Ed25519 antes de usarlo para escribir en el juego.
		Resultado<std::string> plantillas();
		/// POST /equivalencias (solo staff). Máx. 3000 filas por llamada.
		Resultado<std::string> subirEquivalencias(const std::string& cuerpo);

		/// Baja el option oficial a `rutaDestino` y comprueba tamaño y SHA-256.
		/// Si no coincide, borra lo descargado y devuelve OPTION_HUELLA_DISTINTA.
		Resultado<std::string> descargarOption(const OptionOficial& oficial, const std::string& rutaDestino);

	private:
		/// Hace la petición y desenvuelve el sobre { ok, datos | error }.
		/// En éxito devuelve el JSON de "datos" como texto.
		Resultado<std::string> llamar(const std::string& metodo, const std::string& ruta,
			const std::string& cuerpo, bool conToken);
		void log(const std::string& nivel, const std::string& texto);

		ClienteHttp& _http;
		FuenteToken& _token;
		Registro _registro;
		std::string _base;
	};

	/// Quita del texto cualquier "Bearer …" o "pml_…" (defensa extra para logs).
	std::string limpiarSecretos(std::string texto);

}
