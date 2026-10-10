#pragma once

#include <cstdint>
#include <vector>
#include "Tipos.h"
#include "LigaMaster.h"

// =============================================================================
//  Phoenix Sync · Temporada de la Liga Máster (SOLO LECTURA)
// -----------------------------------------------------------------------------
//  Lee del guardado (DATOS descifrados) el calendario de partidos, las tablas de
//  posiciones y las listas de goleadores / asistencias. ESTRUCTURA-ML.md §22–§24.
//  No escribe nada. Todo se busca por su FORMA (multiparche), nunca por dirección:
//   · club = «ID interno» = (ID option << 14) | índice de bloque (§17);
//   · partido de liga (32 B): [local][visitante][nº u16][ffff][competición u16][código u16];
//   · fila de tabla (20 B): [club][puesto][C: pts·G·P·E·G fuera][A: GF·GC·PJ][goles fuera];
//   · fila de ranking (20 B): [reg][pid][club][puesto][valor].
// =============================================================================

namespace mercado::lm {

	struct PartidoLM {
		uint32_t numero = 0;          ///< número global del partido en la carrera
		uint32_t local = 0, visitante = 0;   ///< ID interno de club
		uint16_t competicion = 0;     ///< número de competición en ESTE guardado (depende del parche)
		int jornada = 0;              ///< 0 = primera jornada
		int orden = 0;                ///< orden del partido dentro de la jornada
		size_t ofs = 0;               ///< dónde está (para diagnóstico)
	};

	struct FilaTablaLM {
		uint32_t club = 0;
		int puesto = 0;               ///< 1 = primero (los empates comparten puesto)
		int puntos = 0, ganados = 0, empatados = 0, perdidos = 0;
		int golesFavor = 0, golesContra = 0, jugados = 0;
		int golesFuera = 0, ganadosFuera = 0;
	};

	struct TablaLM {
		size_t ofs = 0;
		bool vacia = false;           ///< liga sin empezar (puesto ffff ffff en todas las filas)
		std::vector<FilaTablaLM> filas;
	};

	struct FilaRankingLM { uint32_t reg = 0, pid = 0, club = 0; int puesto = 0, valor = 0; };
	struct RankingLM { size_t ofs = 0; std::vector<FilaRankingLM> filas; int total() const; };

	struct TemporadaLM {
		std::vector<PartidoLM> partidos;   ///< ordenados por número
		std::vector<TablaLM> tablas;       ///< en el orden del archivo (cada liga: la actual y luego la de la jornada anterior)
		std::vector<RankingLM> rankings;   ///< goleadores, asistencias y otras listas, en el orden del archivo

		/// Lee todo. `limite` = hasta dónde buscar (0 = todo; lo normal es la posición del blob).
		static Resultado<TemporadaLM> leer(const GuardadoLM& g, size_t limite = 0);

		/// ID interno de un club a partir de su bloque (0–699) — el del usuario también.
		static Resultado<uint32_t> idInterno(const GuardadoLM& g, int indice);
	};

}
