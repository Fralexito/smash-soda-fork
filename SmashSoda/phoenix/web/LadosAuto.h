#pragma once

#include "../host/CampoVivo.h"
#include "../link/EstadoJuego.h"

#include <utility>
#include <vector>

// =============================================================================
//  Phoenix Link · Lados automáticos: Link mira en el juego en qué lado (local/visita) juega cada mando
// -----------------------------------------------------------------------------
//  Cruza el stick de cada mando (puestos y mando físico del anfitrión) con el Campo 2D (solo lectura de PES) en
//  los primeros minutos del partido. Si TODOS los detectados están al revés en Link, corrige los lados sin mover
//  mandos (la vía de CORREGIR LADOS) y avisa una vez en el chat. Ante la duda no toca nada.
//  Lógica en core/DeteccionLados (probada en pruebas-link/lados_test.cpp). Ver docs/DETECCION-LADOS.md.
// =============================================================================

namespace phoenix::web {

	struct Interno;

	/// ¿Hace falta el lector del Campo para esto ahora? (interruptor encendido, saliendo al campo o primeros minutos sin resolver)
	bool ladosQuiere(Interno& in);
	/// Con cada lectura nueva del Campo (~20/s): alimenta al detector con los sticks de ese instante.
	void ladosFoto(Interno& in, const phoenix::campo::FotoCampo& fc, double ahora);
	/// Desde el árbitro (2/s): anclas con el estado del juego, decisión y corrección.
	void ladosRevisar(Interno& in, const juego::EstadoPartidoJuego& j, double ahora);
	/// Con cada lectura del Campo: alimenta un detector aparte (no depende de «lados automáticos») para saber qué mando lleva a qué fichita.
	void controlFoto(Interno& in, const phoenix::campo::FotoCampo& fc, double ahora);
	/// Última lectura de controlFoto: pares (índice de ficha, asiento 1-8; 9 = mando físico del anfitrión).
	const std::vector<std::pair<int, int>>& controlActual();
	/// Para la interfaz: "" (apagado o nada que hacer) | midiendo | coinciden | corregidos | dudas
	const char* ladosEstado();
	/// Lado del puesto i (0-7) en la pantalla de mandos de PES, ya estable: 'A' izquierda, 'B' derecha, '-' centro
	/// (Mirando) o '?' (sin lector, sin lectura reciente o LADOS AUTO apagado). Ver core/LadoPantalla.h.
	char ladoPantallaPuesto(int i);
	/// ¿Llegan lecturas de la pantalla de mandos ahora?
	bool ladoPantallaVivo();

}
