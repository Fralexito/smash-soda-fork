#pragma once

// =============================================================================
//  Phoenix Link · Campo 2D en vivo — lector del juego (Windows).
// -----------------------------------------------------------------------------
//  SOLO LECTURA del proceso PES2021.exe (OpenProcess con PROCESS_VM_READ y nada más): no escribe, no inyecta,
//  no hookea, no pausa el juego. Lógica y firma en core/CampoVivo (probada aparte).
//  Solo trabaja mientras la interfaz lo pide (tarjeta Partido › Campo abierta): con la tarjeta cerrada el coste es cero.
//  Seguridad (regla de Fralex: «si algo se rompe, mejor no lo hagas»): solo con el PES2021.exe conocido (tamaño + SHA-256)
//  y con una lista de jugadores creíble; si no, se apaga sola y el partido sigue igual.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

#include "../core/CampoVivo.h"

namespace phoenix::campo {

	enum class EstadoCampo { Apagado, Esperando, Buscando, Activo, NoCompatible };

	/// Una fichita: posición en metros (centro del campo = 0,0) y equipo (0 local, 1 visitante, 2 asistente, -1 sin equipo conocido).
	struct Pieza { float x = 0, z = 0; int eq = -1; };

	struct FotoCampo {
		EstadoCampo estado = EstadoCampo::Apagado;
		std::string motivo;
		std::vector<Pieza> j;
		bool hayBola = false;
		float bx = 0, bz = 0, by = 0;
		bool equiposListos = false; ///< ya se vio un saque de centro y cada fichita tiene equipo
		EstadoTanda tanda;          ///< tanda de penales (activa = en marcha)
		uint32_t seq = 0;           ///< sube con cada lectura nueva
	};

	void iniciar();                 ///< arranca el hilo (idempotente)
	void detener();                 ///< lo para (al cerrar Link)
	void pedirVivo(bool si);        ///< la interfaz lo enciende al abrir la tarjeta y lo apaga al cerrarla
	FotoCampo instantanea();        ///< cualquier hilo: la última lectura

}
