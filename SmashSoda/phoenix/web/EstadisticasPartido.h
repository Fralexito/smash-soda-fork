#pragma once

#include <cstdint>

#include <nlohmann/json.hpp>

// =============================================================================
//  Phoenix Link · Ficha de cada jugador durante el partido
// -----------------------------------------------------------------------------
//  Mientras el partido está en juego mira, por persona (parsecId): su mando,
//  pulsaciones e inactividad (lo que recibe su mando virtual), ping, caídas y
//  tiempo fuera de la sala. Al finalizar, AccionesPartidoWeb añade la ficha a
//  cada jugador de «partida_fin» (docs/SISTEMA-JUGADORES.md).
//  Solo lee: no toca mandos, sala ni juego.
// =============================================================================

namespace phoenix::web {

	struct Interno;

	/// Interruptor del sistema. APAGADO hasta que la web guarde las fichas (PENDIENTE-WEB):
	/// con false no se mira nada y «partida_fin» sale igual que antes.
	constexpr bool kFichaJugadorActiva = false;

	/// ≈ cada frame; trabaja 20 veces por segundo y solo con el partido en juego.
	/// También mueve la sala clasificatoria (SalaClasificatoria.h), que va siempre aunque la ficha esté apagada.
	void tickEstadisticas(Interno& in);

	/// Ficha de `parsecId` en el partido que empezó en `inicioMs` (objeto vacío si no hay).
	nlohmann::json fichaJugador(int64_t inicioMs, uint32_t parsecId);

}
