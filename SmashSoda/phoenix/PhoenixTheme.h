#pragma once

#include "../models/Theme.h"

// =============================================================================
//  Phoenix Link · Tema «Phoenix» (identidad Galaxy)
// -----------------------------------------------------------------------------
//  Cian #00E5FF + púrpura sobre fondo espacial, igual que la web.
//  Primer paso visual: solo colores y redondeos. La reorganización de paneles
//  llega en la fase de rediseño (capa B).
// =============================================================================

class PhoenixTheme : public Theme {
public:

    PhoenixTheme() : Theme("Phoenix") {

        const ImVec4 cian        = hexToImVec4("#00E5FFFF");
        const ImVec4 cianSuave   = hexToImVec4("#5CF0FFFF");
        const ImVec4 purpura     = hexToImVec4("#8B5CF6FF");
        const ImVec4 purpuraOsc  = hexToImVec4("#6D3FD9FF");
        const ImVec4 tinta       = hexToImVec4("#06121AFF"); // texto sobre cian
        const ImVec4 fondo       = hexToImVec4("#0A0D1AFF");
        const ImVec4 panel       = hexToImVec4("#111629FF");
        const ImVec4 panelAlto   = hexToImVec4("#171D36FF");
        const ImVec4 borde       = hexToImVec4("#2A3358FF");
        const ImVec4 texto       = hexToImVec4("#E8F1FFFF");
        const ImVec4 apagado     = hexToImVec4("#8A93B5FF");

        panelBackground = panel;
        panelBorder = borde;
        panelBorderActive = cian;
        panelText = texto;
        panelTitleBar = panelAlto;
        panelTitleBarText = texto;
        panelTitleBarActive = purpura;
        panelTitleBarActiveText = hexToImVec4("#FFFFFFFF");
        panelFooter = fondo;
        panelFooterText = apagado;

        text = texto;
        textMuted = apagado;
        textHighlighted = cian;
        positive = hexToImVec4("#34D399FF");
        negative = hexToImVec4("#F87171FF");

        listItemBackground = panelAlto;

        tabsBackground = fondo;
        tabsBorder = purpura;
        tab = panelAlto;
        tabText = texto;
        tabHovered = purpuraOsc;
        tabActive = purpura;
        tabActiveText = hexToImVec4("#FFFFFFFF");

        navbarBackground = hexToImVec4("#070912FF");
        navbarBorder = borde;
        navbarIcon = hexToImVec4("#FFFFFF40");
        navbarIconActive = cian;

        formLabel = cianSuave;
        formInputBackground = hexToImVec4("#0D1122FF");
        formInputBorder = borde;
        formInputBorderActive = cian;
        formInputText = apagado;
        formInputTextActive = texto;
        formHelpText = apagado;

        buttonPrimary = cian;
        buttonPrimaryBorder = cian;
        buttonPrimaryText = tinta;
        buttonPrimaryHovered = cianSuave;
        buttonPrimaryHoveredText = tinta;
        buttonPrimaryActive = hexToImVec4("#00B8CCFF");
        buttonPrimaryActiveText = tinta;
        buttonPrimaryDisabled = panelAlto;

        buttonSecondary = panelAlto;
        buttonSecondaryBorder = borde;
        buttonSecondaryText = texto;
        buttonSecondaryHovered = purpuraOsc;
        buttonSecondaryHoveredText = hexToImVec4("#FFFFFFFF");
        buttonSecondaryActive = purpura;
        buttonSecondaryActiveText = hexToImVec4("#FFFFFFFF");
        buttonSecondaryDisabled = fondo;

        buttonEnable = cian;
        buttonDisable = panelAlto;

        scrollbarBackground = fondo;
        scrollbarHandle = borde;
        scrollbarHandleHovered = purpura;
        scrollbarHandleActive = cian;
        sliderGrab = cian;
        sliderGrabActive = cianSuave;

        background = fondo;
        primary = cian;
        secondary = purpura;
        disabled = hexToImVec4("#05070EFF");
        invisible = hexToImVec4("#FFFFFF00");
    }

    virtual void apply() {
        ImGuiStyle* style = &ImGui::GetStyle();

        style->WindowBorderSize = 1;
        style->PopupBorderSize = 1;
        style->WindowPadding = ImVec2(20, 20);
        style->ItemSpacing = ImVec2(6, 6);
        style->ScrollbarSize = 12;
        style->GrabMinSize = 18;

        style->WindowRounding = 10;
        style->FrameRounding = 8;
        style->PopupRounding = 10;
        style->ScrollbarRounding = 10;
        style->GrabRounding = 8;
        style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style->DisplaySafeAreaPadding = ImVec2(20, 20);

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = panelText;
        colors[ImGuiCol_TextDisabled] = textMuted;
        colors[ImGuiCol_TitleBg] = panelTitleBar;
        colors[ImGuiCol_TitleBgCollapsed] = panelTitleBar;
        colors[ImGuiCol_TitleBgActive] = panelTitleBarActive;
        colors[ImGuiCol_WindowBg] = panelBackground;
        colors[ImGuiCol_PopupBg] = panelBackground;
        colors[ImGuiCol_Border] = panelBorder;
        colors[ImGuiCol_FrameBg] = formInputBackground;
        colors[ImGuiCol_FrameBgHovered] = listItemBackground;
        colors[ImGuiCol_FrameBgActive] = listItemBackground;
        colors[ImGuiCol_Button] = buttonSecondary;
        colors[ImGuiCol_ButtonHovered] = buttonSecondaryHovered;
        colors[ImGuiCol_ButtonActive] = buttonSecondaryActive;
        colors[ImGuiCol_Header] = listItemBackground;
        colors[ImGuiCol_HeaderHovered] = buttonSecondaryHovered;
        colors[ImGuiCol_HeaderActive] = buttonSecondaryActive;
        colors[ImGuiCol_ScrollbarBg] = scrollbarBackground;
        colors[ImGuiCol_ScrollbarGrab] = scrollbarHandle;
        colors[ImGuiCol_ScrollbarGrabHovered] = scrollbarHandleHovered;
        colors[ImGuiCol_ScrollbarGrabActive] = scrollbarHandleActive;
        colors[ImGuiCol_SliderGrab] = sliderGrab;
        colors[ImGuiCol_SliderGrabActive] = sliderGrabActive;
        colors[ImGuiCol_ResizeGrip] = buttonSecondary;
        colors[ImGuiCol_ResizeGripHovered] = secondary;
        colors[ImGuiCol_ResizeGripActive] = primary;
        colors[ImGuiCol_CheckMark] = primary;
        colors[ImGuiCol_Separator] = panelBorder;

        colors[ImGuiCol_Tab] = tab;
        colors[ImGuiCol_TabHovered] = tabHovered;
        colors[ImGuiCol_TabActive] = tabActive;
        colors[ImGuiCol_TabUnfocused] = tab;
        colors[ImGuiCol_TabUnfocusedActive] = tabActive;

        colors[ImGuiCol_DragDropTarget] = primary;
    }
};
