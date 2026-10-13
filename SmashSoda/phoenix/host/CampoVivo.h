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
		/// «vivo» = juego en directo; «repeticion» = el balón está aparcado (repetición/cinemática): las posiciones se congelan en
		/// la última buena; «otro» = las fichitas dejaron de leerse un rato (también congeladas); «pausa» = todo quieto ≥ 4 s
		/// (posiciones reales, solo informativo).
		std::string fase = "vivo";
		int poseedor = -1;          ///< índice en `j` de quien lleva el balón (-1 = balón libre o sin balón)
		/// Diagnóstico del lector (ventana de ~1 s): lecturas por segundo, duración de una pasada (media y máxima, ms) y
		/// hora (ms desde 1970) a la que se publicó esta lectura: la interfaz y la web restan para medir el retraso.
		double lecturasPorSeg = 0, pasadaMediaMs = 0, pasadaMaxMs = 0;
		long long tMs = 0;
		EstadisticasPartido est;   ///< estadísticas derivadas (posesión, pases, tiros, córners…; aproximadas)
		int portCopias = 0, portPuestos = 0;   ///< porteros: copias encontradas en el juego / puestos (uno por lado) que se están leyendo
	};

	void iniciar();                 ///< arranca el hilo (idempotente)
	void detener();                 ///< lo para (al cerrar Link)
	/// El reloj del partido está parado (repetición, celebración, pausa, descanso): lo dice Link con estado.json del juego. El lector
	/// congela las últimas posiciones buenas y publica fase «detenido» (o «descanso»).
	void fijarDetenido(bool detenido, bool descanso);
	void pedirVivo(bool si);        ///< la interfaz lo enciende al abrir la tarjeta y lo apaga al cerrarla
	FotoCampo instantanea();        ///< cualquier hilo: la última lectura

}
