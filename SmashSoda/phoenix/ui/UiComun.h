#pragma once

#include <algorithm>
#include "imgui.h"

// =============================================================================
//  Phoenix Soda · utilidades visuales compartidas por las pantallas del shell
// =============================================================================

namespace phoenix::vis {

	inline ImU32 col(const ImVec4& c, float alfa = 1.0f) {
		return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alfa));
	}

	/// Acerca `actual` a `objetivo` de forma suave e independiente de los FPS.
	inline float acercar(float actual, float objetivo, float velocidad) {
		const float k = (std::min)(1.0f, ImGui::GetIO().DeltaTime * velocidad);
		return actual + (objetivo - actual) * k;
	}

	/// Mezcla dos colores (t = 0 → a, t = 1 → b).
	inline ImVec4 mezclar(const ImVec4& a, const ImVec4& b, float t) {
		return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}

	/// Color de semáforo para un ping en ms (-1 = sin dato).
	inline ImVec4 colorPing(int ms, const ImVec4& bueno, const ImVec4& malo, const ImVec4& apagado) {
		if (ms < 0) return apagado;
		if (ms < 60) return bueno;
		if (ms < 120) return ImVec4(0.96f, 0.71f, 0.27f, 1.0f); // ámbar
		return malo;
	}

}
