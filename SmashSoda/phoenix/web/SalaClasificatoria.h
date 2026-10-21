#pragma once

#include <nlohmann/json.hpp>

#include "Partido.h"

// =============================================================================
//  Phoenix Link · Sala clasificatoria
// -----------------------------------------------------------------------------
//  Interruptor del anfitrión (Partido › Armar). Encendido, solo juega quien
//  tiene su Parsec vinculado a una cuenta de la web (lo dice /v1/perfiles):
//   · quien no lo tiene y toma un mando vuelve a «Mirando» (liberarMando) con
//     un aviso en el chat;
//   · no se puede preparar un partido con alguien sin cuenta;
//   · un partido ya empezado no se toca: solo se avisa de que no puntuará.
//  Quien aún no se sabe (la web todavía no respondió) puede jugar.
//  Se guarda en phoenix-clasificatoria.json. Ver docs/PEDIDOS-A-LA-WEB.md («SISTEMA-JUGADORES») §3.
// =============================================================================

namespace phoenix::web {

	struct Interno;

	bool clasificatoriaActiva();
	/// Lanza ErrorAccion("PC_SIN_VINCULAR") si se enciende sin token de la web.
	void fijarClasificatoria(bool si);
	/// ≈ cada frame; trabaja una vez por segundo.
	void tickClasificatoria(Interno& in);
	/// { activa, pcVinculada, sinCuenta:[{parsecId,nombre}], sinComprobar:[…] } de la gente en la sala.
	nlohmann::json estadoClasificatoria(Interno& in);
	/// Con la sala clasificatoria, lanza ErrorAccion("SIN_CUENTA") si alguien de los lados no tiene cuenta.
	void comprobarLadosClasificatoria(Interno& in, const LadoPartido& a, const LadoPartido& b);

}
