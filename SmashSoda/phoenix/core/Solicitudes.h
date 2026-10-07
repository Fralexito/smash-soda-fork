#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Soda · Solicitudes de cambio de los jugadores
// -----------------------------------------------------------------------------
//  Un jugador pide desde el chat:
//    !cambio 3   → quiere el mando 3
//    !equipo     → quiere pasar al otro equipo
//  El host ve la solicitud en el tablero y la acepta o la rechaza.
//  Solo pueden pedir quienes juegan (jugadores de la lista o con mando).
//  Cada solicitud vence a los 60 s. Una por jugador (la nueva reemplaza a la vieja).
// =============================================================================

namespace phoenix {

	struct Solicitud {
		uint32_t parsecId = 0;
		std::string nombre;
		int mandoDestino = 0;   ///< 1..8; 0 = «cambiar de equipo»
		double creada = 0.0;    ///< segundos (reloj interno)
	};

	class Solicitudes {
	public:
		static Solicitudes& instancia();

		/// Llamado con cada mensaje de chat. Devuelve true si era un comando de
		/// Phoenix (y entonces `respuesta` trae el texto para el chat).
		bool procesarChat(const std::string& mensaje, uint32_t parsecId, const std::string& nombre,
			bool puedePedir, std::string& respuesta);

		std::vector<Solicitud> pendientes();
		void quitar(uint32_t parsecId);

	private:
		std::mutex _mutex;
		std::vector<Solicitud> _lista;
	};

}
