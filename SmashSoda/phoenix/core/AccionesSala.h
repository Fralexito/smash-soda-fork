#pragma once

#include <cstdint>

// =============================================================================
//  Phoenix Link · Acciones de sala compartidas
// -----------------------------------------------------------------------------
//  Lo que hacen los botones del tablero de mandos (ImGui) y de la interfaz web
//  vive aquí UNA sola vez, para que las dos interfaces se comporten igual.
// =============================================================================

namespace phoenix {

	class ProveedorSala;

	enum class DecisionEspera { Jugador, Espectador, Expulsar };

	namespace AccionesSala {

		/// Acepta la solicitud de cambio pendiente de ese jugador (!cambio N o !equipo).
		/// Devuelve false si no había solicitud.
		bool aceptarSolicitud(ProveedorSala& sala, uint32_t parsecId);
		void rechazarSolicitud(uint32_t parsecId);

		/// Quien entró por Parsec sin estar en la lista: jugador (toma el primer mando
		/// libre de la formación), espectador (nunca juega) o fuera de la sala.
		void decidirEspera(ProveedorSala& sala, uint32_t parsecId, DecisionEspera decision);

		/// Intercambia los dueños de dos mandos (índices desde 0). Si uno está libre,
		/// el jugador simplemente se mueve. Devuelve false si los dos estaban libres.
		bool intercambiar(ProveedorSala& sala, int indiceA, int indiceB);

		/// Asigna el mando conectándolo antes si hace falta (como soltar a alguien encima).
		bool asignarConectando(ProveedorSala& sala, int indice, uint32_t parsecId);

	}

}
