#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "BaseDatosParche.h"
#include "OptionFile.h"

// =============================================================================
//  Phoenix Mercado · Integridad (antitrampa + modo seguro + actualizaciones)
// -----------------------------------------------------------------------------
//  huellaPlantillas   → SHA-256 de las plantillas traducidas a Phoenix IDs.
//                       Igual en todas las PCs aunque usen parches distintos.
//  revisarEstructura  → ¿los datos tienen sentido? Si no, MODO SEGURO: no se
//                       escribe nada y se informa.
//  compararBases      → qué cambió entre dos versiones de la base de un parche
//                       (tras una actualización), centrado en los jugadores de la liga.
// =============================================================================

namespace mercado {

	/// Plantillas en Phoenix IDs: club → jugadores (el orden no importa).
	using PlantillasPhoenix = std::map<int64_t, std::vector<int64_t>>;

	/// Traduce las plantillas del option file usando los emparejamientos.
	/// Solo incluye los clubes emparejados; los jugadores sin Phoenix ID se ignoran
	/// (no pertenecen a la liga). `sinTraducir` cuenta los ignorados.
	PlantillasPhoenix traducirPlantillas(const OptionFile& option,
		const std::map<uint32_t, int64_t>& clubLocalAPhoenix,
		const std::map<uint32_t, int64_t>& jugadorLocalAPhoenix, int* sinTraducir = nullptr);

	/// SHA-256 de una forma canónica: clubes ordenados, jugadores ordenados.
	/// «club:j1,j2,…\n» por club. La web calcula exactamente lo mismo.
	std::string huellaPlantillas(const PlantillasPhoenix& p);
	std::string textoCanonico(const PlantillasPhoenix& p);

	struct Diferencia {
		int64_t jugador = 0;
		int64_t clubEsperado = 0;   ///< 0 = no debería estar en ningún club de la liga
		int64_t clubActual = 0;     ///< 0 = no está en ningún club de la liga
	};
	/// Lista exacta de jugadores fuera de lugar (para corregir y reportar).
	std::vector<Diferencia> diferencias(const PlantillasPhoenix& esperado, const PlantillasPhoenix& actual);

	struct Anomalia { std::string gravedad, codigo, detalle; };   // gravedad: "bloquea" | "aviso"
	/// Chequeo de estructura antes de escribir. Si alguna anomalía «bloquea»,
	/// Mercado entra en MODO SEGURO.
	std::vector<Anomalia> revisarEstructura(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base);
	bool hayBloqueo(const std::vector<Anomalia>& a);

	struct InformeCambios {
		int nuevos = 0, eliminados = 0, modificados = 0;
		std::vector<uint32_t> ligaEliminados;          ///< jugadores de la liga que ya no existen
		std::vector<uint32_t> ligaCambiaronIdentidad;  ///< mismo ID pero parece otra persona
		std::string json() const;
	};
	/// Compara la base antes/después de una actualización del parche.
	/// `idsLiga` = IDs locales de los jugadores que importan (los emparejados).
	InformeCambios compararBases(const std::map<uint32_t, FichaJugador>& antes,
		const std::map<uint32_t, FichaJugador>& despues, const std::vector<uint32_t>& idsLiga);

}
