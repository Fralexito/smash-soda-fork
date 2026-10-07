#include "PantallaBiblioteca.h"

#include <string>
#include <vector>

#include "UiComun.h"
#include "../I18n.h"
#include "../../globals/AppFonts.h"
#include "../../services/ThemeController.h"

namespace phoenix {

	namespace bib {
		struct Juego {
			const char* nombre;
			const char* claveDesc;
			ImVec4 color;
		};

		const Juego juegos[] = {
			{ "PES 2021", "bib.pes2021", ImVec4(0.1f, 0.3f, 0.6f, 1.0f) },
			{ "SP Football Life 2026", "bib.spfl2026", ImVec4(0.2f, 0.5f, 0.2f, 1.0f) },
		};

		float escala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}
	}

	void PantallaBiblioteca::render(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa) {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = bib::escala();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * s, 18 * s));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * s, 12 * s));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
		ImGui::Begin("##phx_biblioteca", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		ImGui::PushFont(AppFonts::label);
		ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
		ImGui::TextUnformatted(T("bib.titulo"));
		ImGui::PopStyleColor();
		ImGui::PopFont();
		ImGui::Dummy(ImVec2(0, 8 * s));

		// Tarjetas de juegos
		const float disp = ImGui::GetContentRegionAvail().x;
		const float anchoTarjeta = (std::min)(320.0f * s, disp);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		for (const auto& j : bib::juegos) {
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const ImVec2 p1(p0.x + anchoTarjeta, p0.y + 120.0f * s);
			
			// Fondo de tarjeta
			dl->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(j.color.x, j.color.y, j.color.z, 0.2f)), 12.0f * s);
			dl->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(j.color), 12.0f * s, 0, 1.0f * s);

			// Nombre del juego
			ImGui::SetCursorScreenPos(ImVec2(p0.x + 14.0f * s, p0.y + 18.0f * s));
			ImGui::PushFont(AppFonts::label);
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ImGui::ColorConvertFloat4ToU32(j.color)));
			ImGui::TextUnformatted(j.nombre);
			ImGui::PopStyleColor();
			ImGui::PopFont();

			// Descripción
			ImGui::SetCursorScreenPos(ImVec2(p0.x + 14.0f * s, p0.y + 48.0f * s));
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(T(j.claveDesc));
			ImGui::PopStyleColor();
			ImGui::PopFont();

			ImGui::SetCursorScreenPos(p1 + ImVec2(0, 8.0f * s));
			ImGui::Dummy(ImVec2(anchoTarjeta, 1));
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
	}
}
