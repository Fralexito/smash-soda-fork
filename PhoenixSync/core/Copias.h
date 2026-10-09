#pragma once

#include <string>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Copias de seguridad
// -----------------------------------------------------------------------------
//  Reglas: nunca se toca el original; nunca se sobrescribe una copia existente;
//  cada copia lleva fecha y hora y se verifica por SHA-256 al terminar.
//  Ejemplo: EDIT00000000  →  copias/EDIT00000000.2026-10-07_21-40-05.bak
// =============================================================================

namespace mercado::copias {

	struct Copia {
		std::string ruta;      ///< dónde quedó la copia
		std::string sha256;    ///< huella (igual a la del original)
		long long bytes = 0;
	};

	/// Copia `origen` dentro de `carpetaCopias` (la crea si falta).
	Resultado<Copia> crear(const std::string& origen, const std::string& carpetaCopias);

	/// Nombre con sello de tiempo local, p. ej. "EDIT00000000.2026-10-07_21-40-05.bak".
	std::string nombreConFecha(const std::string& nombreArchivo);

}
