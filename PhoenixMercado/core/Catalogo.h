#pragma once

#include <string>
#include "BaseDatosParche.h"
#include "OptionFile.h"

// =============================================================================
//  Phoenix Mercado · Catálogo para la web (POST /catalogo)
// -----------------------------------------------------------------------------
//  Equipos, plantillas y dorsales  → del option file (es lo que usa el juego).
//  Datos de cada jugador           → del option file si el parche lo editó;
//                                    si no, de Player.bin del parche.
// =============================================================================

namespace mercado {

	struct ResumenCatalogo {
		std::string json;
		int equipos = 0, jugadores = 0, sinDatos = 0;
	};

	ResumenCatalogo construirCatalogo(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base,
		const std::string& nombreParche);

}
