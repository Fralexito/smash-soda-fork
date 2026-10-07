#pragma once

#include "imgui.h"

namespace phoenix {
	class Widget;
	class Keyboard;

	class TecladoVisual {
	public:
		static bool render(Keyboard& keyboard, ImVec2 pos, ImVec2 tam, Widget* widget);
	};
}
