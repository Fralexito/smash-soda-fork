#pragma once

// =============================================================================
//  Phoenix Link · Medidor de retraso del mando — enganche con el hilo de entrada.
// -----------------------------------------------------------------------------
//  Hosting::pollInputs llama a alMensajeMando() con cada mensaje de mando de un
//  invitado y a alGirar() en cada vuelta del bucle. Ambos solo tocan datos del
//  hilo de entrada (sin bloqueos por mensaje); una vez por segundo se publica
//  una copia que la interfaz lee con instantanea(). Lógica en core/MedidorRetraso.
// =============================================================================

#include <cstdint>
#include <vector>

#include "../core/MedidorRetraso.h"

namespace phoenix::retraso {

	/// Hilo de entrada: llegó un mensaje de mando (estado, eje o botón) del invitado.
	void alMensajeMando(uint32_t userId);
	/// Hilo de entrada: una vuelta del bucle (haya o no mensaje). Publica 1 vez por segundo.
	void alGirar();
	/// Cualquier hilo: última medida publicada (≤ 1 s de antigüedad).
	std::vector<MedidaMando> instantanea();
	/// Cualquier hilo: se borra todo en la próxima vuelta (sala cerrada).
	void pedirLimpieza();

}
