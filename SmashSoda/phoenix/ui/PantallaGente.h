#pragma once

#include "imgui.h"

namespace phoenix {

	class ProveedorSala;

	// =========================================================================
	//  Phoenix Soda · Gente
	// -------------------------------------------------------------------------
	//  Fichas de todos los que están en la sala (jugadores y espectadores),
	//  con ping en color y acciones de un toque: dar/quitar mando, expulsar.
	//  Sala vacía → invitación a compartir el enlace.
	// =========================================================================
	class PantallaGente {
	public:
		static void render(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa);
	};

}
