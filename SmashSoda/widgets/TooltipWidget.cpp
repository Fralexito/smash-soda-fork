#include "TooltipWidget.h"
#include "../phoenix/I18n.h"

bool TooltipWidget::render(const char* text, bool forceShow)
{
	if (ImGui::IsItemHovered() || forceShow)
	{
		Theme* theme = ThemeController::getInstance().getActiveTheme();
		ImGui::PushStyleColor(ImGuiCol_Text, theme->text);

		ImGui::SetTooltip("%s", phoenix::Tr(text).c_str());
		ImGui::PopStyleColor();
		return true;
	}
	return false;
}



