#pragma once

#include "imgui.h"
#include "../../widgets/Widget.h"

namespace phoenix {
	class TecladoVisual {
	public:
		static bool render(void* keyboard, ImVec2 pos, ImVec2 tam, Widget* widget);
	};
}
