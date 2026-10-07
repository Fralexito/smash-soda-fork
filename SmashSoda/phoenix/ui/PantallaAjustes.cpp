#include "PantallaAjustes.h"

#include <cmath>
#include <string>

#include "UiComun.h"
#include "../I18n.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../../core/Config.h"
#include "../../globals/AppFonts.h"
#include "../../globals/AppIcons.h"
#include "../../services/ThemeController.h"

namespace phoenix {

	namespace ajustes {

		struct Preset {
			const char* clave;
			const char* claveDesc;
			int mbps;
			int fps;
		};

		const Preset kPresets[] = {
			{ "aj.ligero",      "aj.ligero_d",      8,  60 },
			{ "aj.equilibrado", "aj.equilibrado_d", 15, 60 },
			{ "aj.maxima",      "aj.maxima_d",      30, 60 },
		};

		double aplicadoEn = -10.0;
		int aplicadoIdx = -1;

		float escala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}

		void titulo(const char* clave, Theme* tema, float s) {
			ImGui::PushFont(AppFonts::label);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(T(clave));
			ImGui::PopStyleColor();
			ImGui::PopFont();
		}

		/// Tarjeta de preset de calidad. Devuelve true al clic.
		bool tarjetaPreset(int i, float ancho, bool activa, Theme* tema, float s) {
			const Preset& p = kPresets[i];
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float alto = 112.0f * s;
			const ImVec2 a = ImGui::GetCursorScreenPos();
			const ImVec2 b(a.x + ancho, a.y + alto);
			ImGui::PushID(i);
			const ImGuiID k = ImGui::GetID("##preset");
			const bool clic = ImGui::InvisibleButton("##preset", ImVec2(ancho, alto));
			const bool encima = ImGui::IsItemHovered();
			if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGuiStorage* st = ImGui::GetStateStorage();
			const float h = vis::acercar(st->GetFloat(k, 0.0f), encima ? 1.0f : 0.0f, 16.0f);
			st->SetFloat(k, h);
			ImGui::PopID();

			// Destello al aplicar
			const double desde = ImGui::GetTime() - aplicadoEn;
			const float flash = (aplicadoIdx == i && desde < 0.6) ? static_cast<float>(1.0 - desde / 0.6) : 0.0f;

			const float lev = 3.0f * s * h;
			const ImVec2 a2(a.x, a.y - lev), b2(b.x, b.y - lev);
			vis::tarjeta(dl, a2, b2, tema->listItemBackground, tema->primary, h, s, activa);
			if (flash > 0.0f) dl->AddRectFilled(a2, b2, vis::col(tema->primary, 0.35f * flash), 14.0f * s);

			// Barras de señal (1..3) como icono del preset
			for (int n = 0; n < 3; n++) {
				const float bx = a2.x + 18.0f * s + n * 9.0f * s;
				const float bh = (8.0f + n * 7.0f) * s;
				const bool on = n <= i;
				dl->AddRectFilled(ImVec2(bx, a2.y + 40.0f * s - bh), ImVec2(bx + 6.0f * s, a2.y + 40.0f * s),
					vis::col(on ? tema->primary : tema->textMuted, on ? 1.0f : 0.3f), 2.0f * s);
			}

			ImGui::PushFont(AppFonts::label);
			dl->AddText(ImVec2(a2.x + 56.0f * s, a2.y + 18.0f * s), vis::col(activa ? tema->primary : tema->text), T(p.clave));
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::input);
			const std::string datos = std::to_string(p.mbps) + " Mbps  ·  " + std::to_string(p.fps) + " FPS";
			dl->AddText(ImVec2(a2.x + 18.0f * s, a2.y + 54.0f * s), vis::col(tema->text, 0.85f), datos.c_str());
			dl->PushClipRect(a2, b2, true);
			dl->AddText(ImVec2(a2.x + 18.0f * s, a2.y + 78.0f * s), vis::col(tema->textMuted), T(p.claveDesc));
			dl->PopClipRect();
			if (activa) {
				const char* marca = T("aj.activo");
				const ImVec2 tm = ImGui::CalcTextSize(marca);
				dl->AddText(ImVec2(b2.x - tm.x - 16.0f * s, a2.y + 20.0f * s), vis::col(tema->primary), marca);
			}
			ImGui::PopFont();
			return clic;
		}
	}

	void PantallaAjustes::rapido(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa) {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = ajustes::escala();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * s, 20 * s));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * s, 10 * s));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
		ImGui::Begin("##phx_ajustes_rapido", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		try {
			// --- Calidad de transmisión -------------------------------------
			ajustes::titulo("aj.calidad", tema, s);
			const float disp = ImGui::GetContentRegionAvail().x;
			const float ancho = (disp - 24.0f * s) / 3.0f;
			ImGui::Dummy(ImVec2(0, 2.0f * s));
			for (int i = 0; i < 3; i++) {
				if (i > 0) ImGui::SameLine(0, 12.0f * s);
				const auto& p = ajustes::kPresets[i];
				const bool activa = static_cast<int>(Config::cfg.video.bandwidth) == p.mbps && static_cast<int>(Config::cfg.video.fps) == p.fps;
				if (ajustes::tarjetaPreset(i, ancho, activa, tema, s) && !activa) {
					if (sala != nullptr) sala->aplicarVideo(p.fps, p.mbps);
					else { Config::cfg.video.fps = p.fps; Config::cfg.video.bandwidth = p.mbps; Config::cfg.Save(); }
					ajustes::aplicadoEn = ImGui::GetTime();
					ajustes::aplicadoIdx = i;
				}
			}
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::Text("%s  %u Mbps · %u FPS", T("aj.actual"), Config::cfg.video.bandwidth, Config::cfg.video.fps);
			ImGui::PopStyleColor();
			ImGui::PopFont();

			ImGui::Dummy(ImVec2(0, 14.0f * s));

			// --- Idioma -----------------------------------------------------
			ajustes::titulo("barra.idioma", tema, s);
			ImGui::PushFont(AppFonts::label);
			const std::string actual = I18n::actual();
			int n = 0;
			for (const auto& par : I18n::disponibles()) {
				if (n++ > 0) ImGui::SameLine(0, 8.0f * s);
				const bool elegido = par.first == actual;
				const std::string id = par.second + "##idioma";
				if (vis::chip(id.c_str(), par.second.c_str(), tema->primary, elegido, s) && !elegido) {
					I18n::establecer(par.first);
					PhoenixPrefs::get().idioma = par.first;
					PhoenixPrefs::get().guardar();
				}
			}
			ImGui::PopFont();

			ImGui::Dummy(ImVec2(0, 14.0f * s));

			// --- Interfaz ---------------------------------------------------
			ajustes::titulo("aj.interfaz", tema, s);
			ImGui::PushFont(AppFonts::label);
			if (vis::chip("##clasica", T("barra.clasica"), tema->secondary, false, s)) {
				PhoenixPrefs::get().interfazPhoenix = false;
				PhoenixPrefs::get().guardar();
			}
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(T("aj.avanzado_hint"));
			ImGui::PopStyleColor();
			ImGui::PopFont();
		}
		catch (...) {
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
	}

}
