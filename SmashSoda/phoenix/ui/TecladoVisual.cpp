#include "TecladoVisual.h"

#include <string>
#include <vector>

#include "imgui.h"
#include "../I18n.h"
#include "UiComun.h"
#include "../../globals/AppFonts.h"
#include "../../services/ThemeController.h"
#include "../../widgets/Widget.h"
#include "../../AGamepad.h"

namespace phoenix {

	namespace tv {
		struct Boton {
			const char* label;
			ImVec2 pos;   // en unidades UI
			ImVec2 tam;
			int idx;      // índice en Keyboard
		};

		const std::vector<Boton> botones() {
			return {
				{"A", ImVec2(150, 80), ImVec2(28, 28), 0},
				{"B", ImVec2(110, 80), ImVec2(28, 28), 1},
				{"X", ImVec2(150, 50), ImVec2(28, 28), 2},
				{"Y", ImVec2(110, 50), ImVec2(28, 28), 3},
				{"LT", ImVec2(8, 8), ImVec2(36, 20), 4},
				{"RT", ImVec2(156, 8), ImVec2(36, 20), 5},
				{"LB", ImVec2(8, 30), ImVec2(36, 20), 6},
				{"RB", ImVec2(156, 30), ImVec2(36, 20), 7},
				{"Start", ImVec2(108, 65), ImVec2(44, 12), 8},
				{"Back", ImVec2(58, 65), ImVec2(44, 12), 9},
			};
		}
	}

	bool TecladoVisual::render(void* keyboard, ImVec2 pos, ImVec2 tam, Widget* widget) {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = ThemeController::getInstance().getUiScale();
		ImDrawList* dl = ImGui::GetWindowDrawList();

		ImGui::SetCursorScreenPos(pos);
		bool cambio = false;

		const float escala = (std::min)(tam.x / 200.0f, tam.y / 140.0f);
		const ImVec2 tamanio_escalado = ImVec2(200.0f * escala, 140.0f * escala);
		const ImVec2 offset = ImVec2((tam.x - tamanio_escalado.x) * 0.5f, (tam.y - tamanio_escalado.y) * 0.5f);
		const ImVec2 origen = ImVec2(pos.x + offset.x, pos.y + offset.y);

		// Marco del mando
		const ImVec2 m0 = origen;
		const ImVec2 m1 = ImVec2(origen.x + tamanio_escalado.x, origen.y + tamanio_escalado.y);
		dl->AddRectFilled(m0, m1, ImGui::ColorConvertFloat4ToU32(tema->listItemBackground), 16.0f * escala);
		dl->AddRect(m0, m1, ImGui::ColorConvertFloat4ToU32(tema->primary), 12.0f * escala, 0, 1.0f * escala);

		const auto bs = tv::botones();
		for (const auto& b : bs) {
			const ImVec2 p0 = ImVec2(origen.x + b.pos.x * escala, origen.y + b.pos.y * escala);
			const ImVec2 p1 = ImVec2(p0.x + b.tam.x * escala, p0.y + b.tam.y * escala);

			ImGui::SetCursorScreenPos(p0);
			ImGui::PushID(b.idx);
			const bool clic = ImGui::InvisibleButton("##btn", ImVec2(b.tam.x * escala, b.tam.y * escala));

			// Dibujar botón
			const bool h = ImGui::IsItemHovered();
			const ImVec4 col = h ? tema->primary : tema->textMuted;
			dl->AddRectFilled(p0, p1, vis::col(col, 0.3f), 4.0f * escala);
			dl->AddRect(p0, p1, vis::col(col), 4.0f * escala, 0, 1.0f * escala);

			// Texto del botón
			ImGui::PushFont(AppFonts::label);
			const ImVec2 tsize = ImGui::CalcTextSize(b.label);
			dl->AddText(ImVec2(p0.x + (b.tam.x * escala - tsize.x) * 0.5f, p0.y + (b.tam.y * escala - tsize.y) * 0.5f),
				vis::col(col), b.label);
			ImGui::PopFont();

			if (clic && widget != nullptr) {
				// Mostrar prompt para capturar tecla (futuro: implementar listener real)
				widget->elLabel("teclado.presiona_tecla");
			}

			ImGui::PopID();
		}

		ImGui::SetCursorScreenPos(ImVec2(m1.x, m1.y + 12.0f * escala));
		ImGui::Dummy(ImVec2(tam.x, 1));

		return cambio;
	}
}
