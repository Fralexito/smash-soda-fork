#pragma once

#include "imgui.h"

namespace phoenix {
	class ProveedorSala;

	class PantallaBiblioteca {
	public:
		static void render(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa);
	};
}
