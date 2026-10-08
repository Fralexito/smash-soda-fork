#pragma once

// =============================================================================
//  Phoenix Link · El host toma cualquier mando (atajo global, funciona en el juego)
// -----------------------------------------------------------------------------
//  Ctrl + Alt + 1…8  → el host maneja ese mando (el dueño queda bloqueado)
//  Ctrl + Alt + 0    → lo suelta
//  Mientras controla: flechas = stick/cruceta · Enter = A · Retroceso = B
//                     Espacio = Start
//  Sirve para mover el mando que alguien dejó en un equipo al irse.
// =============================================================================

namespace phoenix {

	class ProveedorSala;

	class MandoHost {
	public:
		/// Llamar en cada frame.
		static void tick(ProveedorSala* sala);
		/// Mando que controla el host (1..8) o 0.
		static int activo();
		static void soltar(ProveedorSala* sala);
		/// Igual que Ctrl+Alt+N desde un botón de la interfaz (1..8). n == activo() → lo suelta.
		static void tomar(ProveedorSala* sala, int n);
	};

}
