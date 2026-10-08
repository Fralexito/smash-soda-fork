#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <string>

// =============================================================================
//  Phoenix Link · Roles de sala y puerta de mandos
// -----------------------------------------------------------------------------
//  Regla: con una lista de roles activa, SOLO los invitados con rol «jugador»
//  pueden recibir un mando (autoíndice, !swap o !pick). Los espectadores nunca.
//
//  Fuente de la lista (fase A): archivo local `phoenix-sala.json` en la carpeta
//  de configuración de Smash Soda (%APPDATA%\Trybuchet\Smash Soda\).
//  Fase E: la misma lista llegará firmada desde la web (/v1/sala/abrir), sin
//  cambiar esta interfaz.
//
//  Formato:
//  {
//    "activo": true,
//    "expulsarNoListados": false,
//    "jugadores":   [ { "parsecId": 123456, "nombre": "Kaiser", "asiento": 1 } ],
//    "espectadores":[ { "parsecId": 654321, "nombre": "Mirko" } ]
//  }
//  "asiento" es el número de mando (1 = primero). 0 u omitido = cualquiera libre.
//
//  Si el archivo no existe, está vacío, es inválido o "activo" es false, la
//  app se comporta EXACTAMENTE como el Smash Soda original (fallo seguro).
// =============================================================================

namespace phoenix {

	enum class Rol { SinLista, Jugador, Espectador, NoListado };

	class PhoenixRoles {
	public:
		static PhoenixRoles& instancia();

		/// ¿Hay una lista de roles vigente? (si no, todo funciona como el original)
		bool activa();

		Rol rolDe(uint32_t parsecId);

		/// Puerta de mandos: ¿este invitado puede tomar el mando de índice
		/// `indiceMando` (0 = primer mando)? Sin lista activa siempre devuelve true.
		bool puedeTomarMando(uint32_t parsecId, int indiceMando);

		/// ¿Debe expulsarse a este invitado al entrar? (sala de reto con
		/// "expulsarNoListados": true y el invitado no figura en la lista)
		bool debeExpulsar(uint32_t parsecId);

		/// Fuerza releer el archivo (también se relee solo cada 2 s si cambió).
		void recargar();

		std::string rutaArchivo();

		/// Lista que llega de la web (PhoenixLink). Mientras esté puesta, manda
		/// sobre el archivo local. asiento = número de mando (1 = primero).
		void establecerDesdeWeb(bool activa, const std::map<uint32_t, int>& jugadores,
			const std::set<uint32_t>& espectadores, bool expulsarNoListados);
		/// Vuelve al archivo local (p. ej. al cerrar la sala).
		void limpiarWeb();

		/// Asiento reservado (1 = mando 1) de un jugador; 0 = sin asiento fijo / no es jugador.
		int asientoDe(uint32_t parsecId);
		/// Solo el host: mueve a un jugador a otro asiento (se mantiene sobre la lista de la web).
		bool moverAsiento(uint32_t parsecId, int asiento);

		// --- Entrada directa por Parsec (fuera de la lista) ---------------
		/// Si está activo, quien no figura en la lista no se expulsa: queda «en espera».
		void permitirParsec(bool si);
		/// IDs que entraron por Parsec y esperan la decisión del host.
		std::vector<uint32_t> enEspera();
		/// El host decide: jugador (puede tomar mando) o espectador (nunca juega).
		void admitir(uint32_t parsecId, bool comoJugador);

	private:
		PhoenixRoles() = default;
		void recargarSiCambio();
		void cargarDesdeTexto(const std::string& texto);

		std::mutex _mutex;
		bool _activa = false;
		bool _fuenteWeb = false;
		bool _expulsarNoListados = false;
		std::map<uint32_t, int> _jugadores;   // parsecId -> asiento (1..n, 0 = cualquiera)
		std::set<uint32_t> _espectadores;
		std::map<uint32_t, int> _movidosPorHost;   // parsecId -> asiento elegido por el host
		bool _permitirParsec = false;
		std::set<uint32_t> _enEspera;            // entraron por Parsec, sin decidir
		std::set<uint32_t> _admitidosJugador;    // el host los aceptó como jugadores
		std::set<uint32_t> _admitidosEspectador; // el host los aceptó como espectadores
		std::string _ultimoTexto;
		uint64_t _ultimaRevisionMs = 0;
	};

}
