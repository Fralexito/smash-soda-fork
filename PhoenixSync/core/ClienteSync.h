#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Grupo.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Cliente web de «Sync compartido» (contrato v1.8.0, PUBLICADO por WEB 2026-10-09)
// -----------------------------------------------------------------------------
//  Base: la API de Phoenix Link (/functions/v1/phoenix) con el token phx_ de
//  siempre (modo «compartido»). Rutas (los nombres finales los fija el chat WEB):
//    GET  /v1/sync/config?grupo_id=…                      → { modo, actualizado_por, actualizado_en, etag }
//    GET  /v1/sync/operaciones?grupo_id=…&desde=<seq>     → { operaciones:[…], hasta, etag }  (304 = sin cambios)
//    POST /v1/sync/operaciones                            → { seq }
//    POST /v1/sync/operaciones/aplicada                   → { ok }
//  Sobre: { ok:true, datos:{…} } | { ok:false, error:{ codigo, mensaje, reintentar_en } }.
//  Mientras la web no tenga estas rutas: 404 / RUTA_NO_ENCONTRADA → se espera 5 min, sin romperse.
//  Ritmo: ≥ 5 s entre llamadas a la misma ruta; reintentos 2, 4, 8… hasta 300 s; 429 respeta reintentar_en.
// =============================================================================

namespace mercado::sync {

	inline constexpr const char* kBaseSync = "https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/phoenix";
	inline constexpr const char* kVersionSync = "0.2.0";

	/// Cuándo se puede volver a llamar a cada ruta (segundos de reloj, inyectable para pruebas).
	class Ritmo {
	public:
		bool puede(const std::string& ruta, int64_t ahora) const;
		void exito(const std::string& ruta, int64_t ahora);                 ///< siguiente llamada ≥ 5 s
		void fallo(const std::string& ruta, int64_t ahora, int esperaForzada = 0);   ///< 2, 4, 8… 300 s (o lo forzado)
		int64_t siguiente(const std::string& ruta) const;
		int fallosSeguidos(const std::string& ruta) const;
	private:
		struct E { int64_t siguiente = 0; int fallos = 0; };
		std::map<std::string, E> _e;
	};

	struct Lote {
		std::vector<grupo::Operacion> operaciones;
		std::vector<std::string> invalidas;   ///< operaciones que no se entendieron (se saltan)
		int64_t hasta = 0;
		std::string etag;
		bool sinCambios = false;              ///< 304
	};

	class ClienteSync {
	public:
		ClienteSync(ClienteHttp& http, FuenteToken& token, Registro registro = nullptr, std::string base = kBaseSync);

		Resultado<grupo::Config> config(const std::string& grupoId, const std::string& etag, bool& sinCambios);
		Resultado<Lote> operaciones(const std::string& grupoId, int64_t desde, const std::string& etag);
		Resultado<int64_t> publicar(const std::string& grupoId, const grupo::Operacion& op);
		Resultado<bool> confirmar(const std::string& grupoId, const std::string& opId, const std::string& estado, const std::string& motivo);

		/// Segundos que pidió esperar la web en el último 429 (0 si no).
		int reintentarEn() const { return _reintentarEn; }

	private:
		ClienteHttp& _http;
		FuenteToken& _token;
		Registro _log;
		std::string _base;
		int _reintentarEn = 0;
		Resultado<grupo::json> llamar(const std::string& metodo, const std::string& ruta, const std::string& cuerpo,
			const std::string& etag, bool* sinCambios);
	};

	/// Codifica para query string (grupo_id).
	std::string codificarUrl(const std::string& s);

}
