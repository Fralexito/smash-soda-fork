#pragma once

// =============================================================================
//  Phoenix Link · Aviso cuando alguien entra a la sala
// -----------------------------------------------------------------------------
//  Tres avisos, cada uno se apaga por separado en Ajustes › Sonidos:
//    · sonido: una campanita corta que genera Link (sin archivos). Si existe
//      <carpeta del exe>\SFX\entrada.wav, suena ese en su lugar (sonido propio).
//    · notificación de Windows (globo del área de notificaciones, ~6 s).
//    · aviso dentro del juego (overlay de Phoenix) si el puente está instalado.
//  Se llama desde el hilo de Hosting; todo lo pesado va en hilos aparte.
// =============================================================================

#include <string>

namespace phoenix::aviso {

	/// Alguien entró a la sala. No hace nada si el aviso está apagado.
	void entro(const std::string& nombre);
	/// Botón PROBAR de Ajustes: lanza los avisos activos con un nombre de ejemplo.
	void probar();

}
