#include "TitleTooltipWidget.h"
#include "../phoenix/I18n.h"

bool TitleTooltipWidget::render(const char* title, const char* description, bool forceShow)
{
	if (ImGui::IsItemHovered() || forceShow) {

		Theme* theme = ThemeController::getInstance().getActiveTheme();
		ImGui::PushStyleColor(ImGuiCol_Border, theme->panelBorderActive);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->panelBackground);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

		ImGui::PushFont(AppFonts::title);
		ImGui::BeginTooltip();

		ImGui::PushStyleColor(ImGuiCol_Text, theme->formLabel);
		ImGui::TextUnformatted(phoenix::Tr(title).c_str());

		ImGui::PushStyleColor(ImGuiCol_Text, theme->panelText);
		ImGui::TextUnformatted(phoenix::Tr(description).c_str());

		ImGui::EndTooltip();

		ImGui::PopFont();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		return true;
	}
}



