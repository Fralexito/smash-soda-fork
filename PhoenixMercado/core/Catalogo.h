#pragma once

#include <string>
#include <vector>
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

	/// Divide el catálogo en lotes para POST /catalogo (máx. 5 MB y 10 llamadas/hora en la web).
	/// El primer lote lleva TODOS los equipos (la web crea los clubes antes que los jugadores);
	/// los demás llevan equipos:[]. Los nombres de club repetidos se hacen únicos con « (#id)»,
	/// porque la web identifica los clubes por (liga, nombre).
	std::vector<std::string> lotesCatalogo(const std::string& catalogoJson, size_t jugadoresPorLote = 3000);

	ResumenCatalogo construirCatalogo(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base,
		const std::string& nombreParche);

}
