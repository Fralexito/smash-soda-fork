#pragma once

#include "imgui.h"

namespace phoenix {
	class Widget;

	class TecladoVisual {
	public:
		static bool render(void* keyboard, ImVec2 pos, ImVec2 tam, Widget* widget);
	};
}
