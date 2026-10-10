#pragma once

#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Entrega para Phoenix Link  (%APPDATA%\Phoenix Mercado\entrega\)
// -----------------------------------------------------------------------------
//  Link (ya hecho, no se cambia) mira la carpeta cada ~3 s. Si aparece
//  entrega.json, comprueba todo, coloca los archivos en el juego y avisa en el
//  overlay. Reglas para nosotros:
//    · primero los archivos de datos (.tmp → verificar → renombrar),
//      AL FINAL entrega.json (.tmp → renombrar);
//    · solo «Player.bin» y «EDIT00000000»; sha256 en minúsculas;
//    · id único (letras, números, - y _, máx. 64);
//    · si ya hay un entrega.json pendiente, se espera (no se apila).
//  Formato:
//  { "version":1, "id":"…", "creado_en":"<ISO>", "resumen":"texto corto",
//    "archivos":[ { "nombre":"EDIT00000000", "sha256":"<64 hex>" } ] }
// =============================================================================

namespace mercado::entrega {

	struct Archivo {
		std::string nombre;       ///< "EDIT00000000" o "Player.bin"
		std::string rutaOrigen;   ///< de dónde se copia
	};

	bool nombrePermitido(const std::string& nombre);
	bool idValido(const std::string& id);
	/// «sync-<AAAAMMDDhhmmss>-<8 hex>» (único).
	std::string nuevoId();
	/// Recorta a `maxBytes` sin cortar una letra UTF-8 por la mitad.
	std::string recortarResumen(const std::string& texto, size_t maxBytes = 40);

	/// ¿Hay un entrega.json que Link todavía no procesó?
	bool hayPendiente(const std::string& carpetaEntrega);
	/// ¿Link ya procesó ese id? (existe entregados\<id> o rechazadas\<id>). 1 = entregado, -1 = rechazado, 0 = no.
	int estadoDe(const std::string& carpetaEntrega, const std::string& id);

	/// Escribe una entrega. Errores: ENTREGA_PENDIENTE, ENTREGA_INVALIDA, ARCHIVO_…  Devuelve el texto de entrega.json.
	Resultado<std::string> escribir(const std::string& carpetaEntrega, const std::string& id,
		const std::string& resumen, const std::vector<Archivo>& archivos, const std::string& creadoEnIso);

}
