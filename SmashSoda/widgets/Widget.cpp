#include <cfloat>
#include <algorithm>
#include "Widget.h"
#include "../phoenix/ui/UiDock.h"
#include "../phoenix/I18n.h"
#include <algorithm>

namespace {
    float WidgetScale()
    {
        const float scale = ThemeController::getInstance().getUiScale();
        return scale > 0.0f ? scale : 1.0f;
    }

    float S(float value)
    {
        return value * WidgetScale();
    }

    ImVec2 SV(float x, float y)
    {
        return ImVec2(S(x), S(y));
    }
}

int Widget::AutoWrapCallback(ImGuiInputTextCallbackData* data) {

    AutoWrapContext* ctx = static_cast<AutoWrapContext*>(data->UserData);

    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    int cursor = data->CursorPos;

    int lineStart = cursor;
    while (lineStart > 0 && data->Buf[lineStart - 1] != '\n') {
        lineStart--;
    }

    int lineEnd = lineStart;
    while (lineEnd < data->BufTextLen && data->Buf[lineEnd] != '\n') {
        lineEnd++;
    }

    float lineWidth = font->CalcTextSizeA(
        fontSize,
        FLT_MAX,
        0.0f,
        data->Buf + lineStart,
        data->Buf + lineEnd
    ).x;

    if (lineWidth <= ctx->wrapWidth) {
        return 0;
    }

    int wrapPos = cursor;
    while (wrapPos > lineStart && data->Buf[wrapPos - 1] != ' ') {
        wrapPos--;
    }

    if (wrapPos <= lineStart) {
        return 0;
    }

    data->InsertChars(wrapPos, "\n");

    if (cursor >= wrapPos) {
        data->CursorPos++;
    }

    return 0;
}


/**
 * Set the position and dimensions.
 *
 * \param x	The x coordinate.
 * \param y	The y coordinate.
 * \param w	The width.
 * \param h The height.
 */
void Widget::startWidget(const char* name, bool& visible, int x, int y, int w, int h, int minW, int minH) {

    // Set widget name
    widgetName = (char*)name;

    // Phoenix: si el shell reservó un hueco, el panel va fijo ahí
    ImVec2 dockPos, dockTam;
    const bool acoplado = phoenix::UiDock::tomar(dockPos, dockTam);
    estaAcoplado = acoplado;

    Theme* theme = ThemeController::getInstance().getActiveTheme();
    bool wasPopupOpen = isPopupOpen;
    isPopupOpen = false;

    // Check if the window is focused, and set text color accordingly
    if (!acoplado && (isTitleFocused || isHeaderFocused || isBodyFocused || isFooterFocused || isFocused || wasPopupOpen)) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme->panelTitleBarActiveText);  // Focused text color
        ImGui::PushStyleColor(ImGuiCol_Border, theme->panelBorderActive);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, theme->panelTitleBarActive);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, theme->panelTitleBarActive);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, theme->panelTitleBarText);
        ImGui::PushStyleColor(ImGuiCol_Border, theme->panelBorder);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, theme->panelTitleBar);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, theme->panelTitleBar);
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme->panelBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->panelBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->panelBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->panelBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->panelBackground);

    // Set the font
    ImGui::PushFont(AppFonts::title);

    // Set window position and size constraints
    if (acoplado) {
        ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(dockTam, ImGuiCond_Always);
    }
    else {
        ImGui::SetNextWindowPos(ImVec2(S(static_cast<float>(x)), S(static_cast<float>(y))), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(S(static_cast<float>(minW)), S(static_cast<float>(minH))),
            ImVec2(S(static_cast<float>(w)), S(static_cast<float>(h)))
        );
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, SV(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(8.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0.2f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (acoplado) {
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    }
    ImGui::Begin(name, acoplado ? nullptr : &visible, flags);

    size = ImGui::GetContentRegionAvail();
    pos = ImGui::GetWindowPos();

    // Is ImGui::Begin() focused? (include child windows)
    isTitleFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Reset style
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

void Widget::endTabs() {
    ImGui::Unindent(S(20.0f));
    ImGui::EndTabBar();
}

/**
 * Tabs header
 */
void Widget::startTabs(const std::vector<Tab>& tabs, bool footer) {
    hasFooter = footer;

    Theme* theme = ThemeController::getInstance().getActiveTheme();
    const float tabsHeaderHeight = S(estaAcoplado ? 50.0f : 64.0f);
    const float tabsBorderThickness = S(1.0f);
    const ImU32 tabsBorderColor = ImGui::ColorConvertFloat4ToU32(theme->panelBorder);

    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    pos = ImGui::GetCursorScreenPos();
    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x + size.x, pos.y + tabsHeaderHeight),
        ImGui::ColorConvertFloat4ToU32(estaAcoplado ? theme->panelBackground : theme->tabsBackground),
        S(10.0f),
        ImDrawFlags_RoundCornersBottom
    );
    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImGui::Dummy(SV(0.0f, estaAcoplado ? 14.0f : 30.0f));

    std::string pillsName = "##pills-" + std::string(widgetName);
    ImGui::BeginTabBar(pillsName.c_str());

    ImGui::Dummy(SV(20.0f, 0.0f));
    ImGui::Indent(S(20.0f));

    for (int i = 0; i < tabs.size(); i++) {

        // Phoenix: pestañas tipo texto con subrayado cuando el panel está acoplado
        if (estaAcoplado) {
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(theme->textMuted.x, theme->textMuted.y, theme->textMuted.z, 0.10f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(theme->secondary.x, theme->secondary.y, theme->secondary.z, 0.16f));
        }

        if (activeTab == i) {
            ImGui::PushStyleColor(ImGuiCol_Button, theme->buttonPrimary);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->buttonPrimaryHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->buttonPrimaryActive);
            ImGui::PushStyleColor(ImGuiCol_Text, estaAcoplado ? theme->primary : theme->buttonPrimaryText);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, theme->buttonSecondary);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->buttonSecondaryHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->buttonSecondaryActive);
            ImGui::PushStyleColor(ImGuiCol_Text, estaAcoplado ? theme->textMuted : theme->buttonSecondaryText);
        }

        const std::string nombrePestana = phoenix::Tr(tabs[i].name);
        if (ImGui::BeginTabItem(nombrePestana.c_str())) {
            activeTab = i;
            if (estaAcoplado) {
                const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
                drawList->AddRectFilled(ImVec2(a.x + S(8.0f), b.y - S(3.0f)), ImVec2(b.x - S(8.0f), b.y),
                    ImGui::ColorConvertFloat4ToU32(theme->primary), S(2.0f));
            }
            const float seamOffset = ImGui::GetStyle().ItemSpacing.y;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - seamOffset);
            const ImVec2 contentStart = ImGui::GetCursorScreenPos();
            drawList->AddLine(
                ImVec2(pos.x, contentStart.y),
                ImVec2(pos.x + size.x, contentStart.y),
                tabsBorderColor,
                tabsBorderThickness
            );
            tabs[i].render();
            ImGui::EndTabItem();
        }
        ImGui::PopStyleColor(estaAcoplado ? 7 : 4);

    }
}

/**
 * Start widget header.
 *
 */
void Widget::startHeader(string id, int height) {

	headerHeight = static_cast<int>(S(static_cast<float>(height)));
    size = ImGui::GetContentRegionAvail();
    pos = ImGui::GetCursorScreenPos();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, S(10.0f));
    ImGui::BeginChild(id.c_str(), ImVec2(size.x, S(50.0f)), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    pos = ImGui::GetCursorScreenPos();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + static_cast<float>(headerHeight)), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.1)), S(8.0f), ImDrawFlags_RoundCornersBottom);
    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImGui::Dummy(SV(0.0f, 2.0f));
    ImGui::Indent(S(10.0f));
	isHeaderFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    hasHeader = true;

}

/**
 * Stop widget header.
 *
 */
void Widget::endHeader() {

    ImGui::Unindent(S(10.0f));
    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImGui::SetCursorPos(ImVec2(0.0f, static_cast<float>(headerHeight) - S(1.0f)));
    ImGui::Separator();
    ImGui::EndChild();
    ImGui::PopStyleVar();

}

void Widget::startBody(bool footer) {
    hasFooter = footer;

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    if (hasFooter) {
    }
    ImGui::BeginChild("##body", ImVec2(size.x, (footer ? size.y - S(58.0f) : size.y)));
    ImGui::Dummy(SV(20.0f, 10.0f));  // Adds top-left padding
    ImGui::Indent(S(20.0f));
    isBodyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme->panelBackground);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->panelText);
}

void Widget::endBody() {
    ImGui::Unindent(S(20.0f));
	ImGui::EndChild();
    ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

/**
 * Start widget footer.
 *
 */
void Widget::startFooter() {
    Theme* theme = ThemeController::getInstance().getActiveTheme();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, SV(0.0f, 0.0f));
    size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##footer", ImVec2(size.x, S(58.0f)), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImDrawFlags_RoundCornersBottom);
    isFooterFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    ImGui::SetCursorPos(SV(0.0f, 0.0f));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    pos = ImGui::GetCursorScreenPos();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + S(52.0f)), ImGui::ColorConvertFloat4ToU32(theme->panelFooter), S(10.0f), ImDrawFlags_RoundCornersBottom);
	ImGui::PopStyleVar();
	ImGui::Separator();
	ImGui::SetCursorPos(SV(10.0f, 10.0f));
    ImGui::Indent(S(10.0f));
}

/**
 * Stop widget footer.
 *
 */
void Widget::endFooter() {
    ImGui::Unindent(S(10.0f));
    ImGui::EndChild();
}

/**
 * End the widget.
 *
 */
void Widget::endWidget() {
    isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || isPopupOpen;
	ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleColor(9);
}


// =============================================================================
//  Phoenix: filas compactas. Cada ajuste ocupa una tarjeta-fila:
//  [ Nombre en negrita                       ] [ control ]
//  [ ayuda corta en gris                     ]
//  Solo dentro del shell Phoenix (estaAcoplado); la interfaz clásica no cambia.
// =============================================================================
namespace {
    struct FilaPhoenix { ImVec2 p0; float ancho = 0, alto = 0; };
    FilaPhoenix gFila;

    float filaAcercar(float a, float b, float v) {
        const float k = (std::min)(1.0f, ImGui::GetIO().DeltaTime * v);
        return a + (b - a) * k;
    }
    ImU32 filaCol(const ImVec4& c, float a = 1.0f) {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * a));
    }
}

void Widget::filaInicio(const std::string& label, const std::string& help, float anchoCtrl, float altoCtrl) {
    Theme* t = ThemeController::getInstance().getActiveTheme();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float pad = S(14.0f);
    const float W = ImGui::GetContentRegionAvail().x - S(6.0f);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const std::string L = phoenix::Tr(label);
    const std::string H = help.empty() ? std::string() : phoenix::Tr(help);
    const float textoW = (std::max)(S(120.0f), W - anchoCtrl - pad * 3.0f);

    ImFont* fL = AppFonts::label;
    ImFont* fH = AppFonts::input;
    const ImVec2 tl = fL->CalcTextSizeA(fL->FontSize, FLT_MAX, textoW, L.c_str());
    const ImVec2 th = H.empty() ? ImVec2(0, 0) : fH->CalcTextSizeA(fH->FontSize, FLT_MAX, textoW, H.c_str());
    const float altoTexto = tl.y + (H.empty() ? 0.0f : th.y + S(3.0f));
    const float alto = (std::max)(altoTexto, altoCtrl) + pad * 1.3f;

    // Brillo al pasar el mouse (animado)
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID k = ImGui::GetID(("##fila" + label).c_str());
    const bool encima = ImGui::IsMouseHoveringRect(p0, ImVec2(p0.x + W, p0.y + alto));
    const float h = filaAcercar(st->GetFloat(k, 0.0f), encima ? 1.0f : 0.0f, 16.0f);
    st->SetFloat(k, h);

    const float r = S(12.0f);
    dl->AddRectFilled(p0, ImVec2(p0.x + W, p0.y + alto), filaCol(t->listItemBackground, 0.55f + 0.45f * h), r);
    dl->AddRect(p0, ImVec2(p0.x + W, p0.y + alto), filaCol(t->primary, 0.06f + 0.40f * h), r, 0, S(1.0f));
    dl->AddRectFilled(ImVec2(p0.x, p0.y + r), ImVec2(p0.x + S(3.0f), p0.y + alto - r), filaCol(t->primary, h), S(2.0f));

    const float yTexto = p0.y + (alto - altoTexto) * 0.5f;
    dl->AddText(fL, fL->FontSize, ImVec2(p0.x + pad, yTexto), filaCol(t->text), L.c_str(), nullptr, textoW);
    if (!H.empty()) {
        dl->AddText(fH, fH->FontSize, ImVec2(p0.x + pad, yTexto + tl.y + S(3.0f)), filaCol(t->textMuted), H.c_str(), nullptr, textoW);
    }

    ImGui::SetCursorScreenPos(ImVec2(p0.x + W - pad - anchoCtrl, p0.y + (alto - altoCtrl) * 0.5f));
    gFila.p0 = p0; gFila.ancho = W; gFila.alto = alto;
}

void Widget::filaFin(const std::string& error) {
    ImGui::SetCursorScreenPos(ImVec2(gFila.p0.x, gFila.p0.y + gFila.alto + S(4.0f)));
    elError(error);
    ImGui::Dummy(ImVec2(gFila.ancho, S(4.0f)));
}

bool Widget::interruptor(const std::string& id, bool& valor) {
    Theme* t = ThemeController::getInstance().getActiveTheme();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 tam(S(46.0f), S(26.0f));
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImGuiID k = ImGui::GetID(id.c_str());
    const bool clic = ImGui::InvisibleButton(id.c_str(), tam);
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (clic) valor = !valor;
    ImGuiStorage* st = ImGui::GetStateStorage();
    const float pos = filaAcercar(st->GetFloat(k, valor ? 1.0f : 0.0f), valor ? 1.0f : 0.0f, 18.0f);
    st->SetFloat(k, pos);
    const ImVec4 off = t->buttonDisable;
    const ImVec4 on = t->primary;
    const ImVec4 c(off.x + (on.x - off.x) * pos, off.y + (on.y - off.y) * pos, off.z + (on.z - off.z) * pos, 1.0f);
    if (pos > 0.01f) dl->AddRectFilled(ImVec2(p0.x - S(3), p0.y - S(3)), ImVec2(p0.x + tam.x + S(3), p0.y + tam.y + S(3)),
        filaCol(on, 0.18f * pos), tam.y);
    dl->AddRectFilled(p0, ImVec2(p0.x + tam.x, p0.y + tam.y), filaCol(c), tam.y * 0.5f);
    const float rad = tam.y * 0.5f - S(3.0f);
    dl->AddCircleFilled(ImVec2(p0.x + tam.y * 0.5f + (tam.x - tam.y) * pos, p0.y + tam.y * 0.5f), rad, filaCol(ImVec4(1, 1, 1, 1)));
    return clic;
}

void Widget::elLabel(std::string label) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImGui::PushFont(AppFonts::label);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formLabel);
    ImGui::SetNextItemWidth(size.x - S(40.0f));
	ImGui::TextUnformatted(phoenix::Tr(label).c_str());
	ImGui::PopStyleColor();
    ImGui::PopFont();
}

void Widget::elHelp(std::string help) {

    if (help.empty()) return;

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImGui::PushFont(AppFonts::input);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formHelpText);
    ImGui::SetNextItemWidth(size.x - S(60.0f));
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + size.x - S(60.0f));
	ImGui::TextWrapped("%s", phoenix::Tr(help).c_str());
    ImGui::PopStyleColor();
	ImGui::PopFont();
}

void Widget::elError(std::string error) {
    if (error.empty()) return;

    ImGui::PushFont(AppFonts::input);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::SetNextItemWidth(size.x - S(60.0f));
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + size.x - S(60.0f));
    ImGui::TextWrapped("%s", phoenix::Tr(error).c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
}



void Widget::elParagraph(std::string text) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImGui::PushFont(AppFonts::input);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->panelText);
    ImGui::SetNextItemWidth(size.x - S(40.0f));
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + size.x - S(60.0f));
    ImGui::TextWrapped("%s", phoenix::Tr(text).c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(SV(0.0f, 10.0f));
}

bool Widget::elText(
    std::string label,
    char* buffer,
    std::string help,
    std::string error,
    int bufferSize,
    float width,
    ImGuiInputTextFlags flags
) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    bool response = false;
    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    const bool fila = estaAcoplado && !label.empty() && width <= 0.0f;
    if (fila) {
        const float ancho = (std::min)(S(360.0f), size.x * 0.5f);
        filaInicio(label, help, ancho, ImGui::GetFrameHeight());
        ImGui::SetNextItemWidth(ancho);
    }
    else {
        if (!label.empty()) elLabel(label);
        ImGui::SetNextItemWidth(width > 0.0f ? width : size.x - S(20.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);

    if (!error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    if (ImGui::InputText(inputLabel.c_str(), buffer, bufferSize, flags)) {
        response = true;
    }
    else {
        response = false;
    }

    if (error.empty() && ImGui::IsItemActive()) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 rectMax = ImGui::GetItemRectMax();
        float rounding = ImGui::GetStyle().FrameRounding;
        drawList->AddRect(
            rectMin,
            rectMax,
            ImGui::ColorConvertFloat4ToU32(theme->formInputBorderActive),
            rounding,
            0,
            1.0f
        );
    }

    if (!error.empty()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor(2);

    if (fila) {
        filaFin(error);
        return response;
    }

    if (!help.empty() || !error.empty()) {
        ImGui::SetNextItemWidth(size.x - S(20.0f));
        elHelp(help);
        elError(error);
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return response;

}

void Widget::elReadOnly(std::string label, char* buffer, std::string help, std::string error) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    if (!label.empty()) {
        elLabel(label);
    }
    ImGui::SetNextItemWidth(size.x - S(20.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    ImGui::PushStyleColor(ImGuiCol_Border, theme->formInputBorderActive);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::InputText(inputLabel.c_str(), buffer, 256, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    elHelp(help);
    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }
}



bool Widget::elPassword(std::string label, char* buffer, std::string help, std::string error) {
    
    Theme* theme = ThemeController::getInstance().getActiveTheme();

    bool response = false;
    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    if (!label.empty()) {
        elLabel(label);
    }
    ImGui::SetNextItemWidth(size.x - S(20.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    if (!error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    if (ImGui::InputText(inputLabel.c_str(), buffer, 256, ImGuiInputTextFlags_Password)) {
        response = true;
    }
    else {
        response = false;
    }

    if (error.empty() && ImGui::IsItemActive()) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 rectMax = ImGui::GetItemRectMax();
        float rounding = ImGui::GetStyle().FrameRounding;
        drawList->AddRect(
            rectMin,
            rectMax,
            ImGui::ColorConvertFloat4ToU32(theme->formInputBorderActive),
            rounding,
            0,
            1.0f
        );
    }

    if (!error.empty()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor(2);

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    elHelp(help);

    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return response;

}

bool Widget::elNumber(std::string label, int& value, int from, int to, std::string help, std::string error) {
    
    Theme* theme = ThemeController::getInstance().getActiveTheme();

    bool response = false;
    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    if (estaAcoplado) {
        filaInicio(label, help, S(120.0f), ImGui::GetFrameHeight());
        ImGui::SetNextItemWidth(S(120.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
        response = IntRangeWidget::render(label.c_str(), value, from, to, 0.025f);
        ImGui::PopStyleColor(2);
        filaFin(error);
        return response;
    }

    elLabel(label);
    ImGui::SetNextItemWidth(size.x - S(20.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    if (IntRangeWidget::render(label.c_str(), value, from, to, 0.025f)) {
        response = true;
    }
    else {
        response = false;
    }
    ImGui::PopStyleColor(2);

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    elHelp(help);

    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return response;

}

bool Widget::elTextArea(std::string label, char* buffer, std::string help, std::string error) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    elLabel(label);

    ImVec2 boxSize(
        avail.x - S(20.0f),
        ImGui::GetTextLineHeight() * 6
    );

    AutoWrapContext ctx;
    ctx.wrapWidth = boxSize.x - ImGui::GetStyle().FramePadding.x * 2.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->formInputBackground);
    if (!error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    bool changed = ImGui::InputTextMultiline(
        inputLabel.c_str(),
        buffer,
        500,
        boxSize,
        ImGuiInputTextFlags_NoHorizontalScroll |
        ImGuiInputTextFlags_CallbackEdit,
        Widget::AutoWrapCallback,
        &ctx
    );

    if (error.empty() && ImGui::IsItemActive()) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 rectMax = ImGui::GetItemRectMax();
        float rounding = ImGui::GetStyle().FrameRounding;
        drawList->AddRect(
            rectMin,
            rectMax,
            ImGui::ColorConvertFloat4ToU32(theme->formInputBorderActive),
            rounding,
            0,
            1.0f
        );
    }

    if (!error.empty()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor(4);

    elHelp(help);
    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return changed;
}

bool Widget::elCheckbox(std::string label, bool& isOn, std::string help, std::string error) {
    
    Theme* theme = ThemeController::getInstance().getActiveTheme();

    bool response = false;
    ImVec2 size = ImGui::GetContentRegionAvail();

    if (estaAcoplado) {
        filaInicio(label, help, S(46.0f), S(26.0f));
        response = interruptor("##chk" + label, isOn);
        filaFin(error);
        return response;
    }

    ImGui::PushFont(AppFonts::label);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formLabel);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->formInputBackground);

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    if (ImGui::Checkbox(phoenix::Tr(label).c_str(), &isOn)) {
        response = true;
    }
    ImGui::PopStyleColor(4);
    ImGui::PopFont();

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    elHelp(help);

    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return response;

}

bool Widget::elSelect(std::string label,
    std::vector<std::pair<std::string, std::string>> options,
    string& callback, std::string help, std::string error) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    const bool fila = estaAcoplado;
    if (fila) {
        const float ancho = (std::min)(S(320.0f), size.x * 0.45f);
        filaInicio(label, help, ancho, ImGui::GetFrameHeight());
        ImGui::SetNextItemWidth(ancho);
    }
    else {
        elLabel(label);
        ImGui::SetNextItemWidth(size.x - S(20.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->formInputBackground);

    bool itemSelected = false;

    // Get selected option
    std::string selected = "";
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i].first == callback) {
			selected = options[i].second.c_str();
			break;
		}
	}

    ImGui::PushStyleColor(ImGuiCol_Button, theme->buttonPrimary);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->buttonPrimaryActive);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->buttonPrimaryHovered);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    if (ImGui::BeginCombo(inputLabel.c_str(), selected.c_str(), ImGuiComboFlags_HeightLarge)) {
        isPopupOpen = true;
        for (size_t i = 0; i < options.size(); ++i) {
            bool isSelected = (options[i].first == callback);
            if (ImGui::Selectable(options[i].second.c_str(), isSelected)) {
                callback = options[i].first;
                itemSelected = true; // Store selection state
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleColor(8);

    if (fila) {
        filaFin(error);
        return itemSelected;
    }

    ImGui::SetNextItemWidth(size.x - S(20.0f));
    elHelp(help);

    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return itemSelected; // Return after everything is drawn
}

bool Widget::elMultiSelect(
    const std::string& label,
    const std::vector<std::pair<std::string, std::string>>& options,
    std::vector<std::string>& selectedValues,
    size_t limit,
    const std::string& help,
    const std::string& error
) {
    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImVec2 size = ImGui::GetContentRegionAvail();
    std::string inputLabel = "##" + label;

    elLabel(label);
    ImGui::SetNextItemWidth(size.x - S(20.0f));

    std::string preview;
    for (size_t i = 0; i < selectedValues.size(); ++i) {
        auto it = std::find_if(
            options.begin(),
            options.end(),
            [&](const auto& o) { return o.first == selectedValues[i]; }
        );
        if (it != options.end()) {
            if (!preview.empty()) preview += ", ";
            preview += it->second;
        }
    }
    if (preview.empty()) {
        preview = "Select...";
    }

    bool changed = false;

    ImGui::PushStyleColor(ImGuiCol_Text, theme->formInputText);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->formInputBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme->formInputBackground);

    if (ImGui::BeginCombo(inputLabel.c_str(), preview.c_str(), ImGuiComboFlags_HeightLarge)) {
        isPopupOpen = true;

        for (const auto& opt : options) {

            bool isSelected = std::find(
                selectedValues.begin(),
                selectedValues.end(),
                opt.first
            ) != selectedValues.end();

            if (ImGui::Selectable(
                    opt.second.c_str(),
                    isSelected,
                    ImGuiSelectableFlags_DontClosePopups
                )) {

                if (isSelected) {
                    selectedValues.erase(
                        std::remove(selectedValues.begin(), selectedValues.end(), opt.first),
                        selectedValues.end()
                    );
                    changed = true;
                }
                else if (selectedValues.size() < limit) {
                    selectedValues.push_back(opt.first);
                    changed = true;
                }
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopStyleColor(4);

    elHelp(help);
    elError(error);

    if (!help.empty() || !error.empty()) {
        ImGui::Dummy(SV(0.0f, 10.0f));
    }

    return changed;
}



bool Widget::elBtn(std::string label) {

    Theme* theme = ThemeController::getInstance().getActiveTheme();

    ImGui::PushStyleColor(ImGuiCol_Button, theme->buttonPrimary);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->buttonPrimaryActive);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->buttonPrimaryHovered);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->buttonPrimaryText);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SV(10.0f, 5.0f));

    if (ImGui::Button(phoenix::Tr(label).c_str())) {

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        return true;
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return false;
}

bool Widget::elBtnSecondary(std::string label) {
    
    Theme* theme = ThemeController::getInstance().getActiveTheme();

	ImGui::PushStyleColor(ImGuiCol_Button, theme->buttonSecondary);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->buttonSecondaryActive);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->buttonSecondaryHovered);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->buttonSecondaryText);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SV(10.0f, 5.0f));
    if (ImGui::Button(phoenix::Tr(label).c_str())) {

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		return true;
	}

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	return false;
}
