#pragma once

#include "imgui.h"

namespace phoenix {

	class ProveedorSala;

	// =========================================================================
	//  Phoenix Link · Ajustes rápidos
	// -------------------------------------------------------------------------
	//  Lo que más se toca, en tarjetas de un clic: calidad (presets), idioma
	//  e interfaz. El detalle fino queda en las pestañas General/Video/Audio.
	// =========================================================================
	class PantallaAjustes {
	public:
		static void rapido(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa);
	};

}
