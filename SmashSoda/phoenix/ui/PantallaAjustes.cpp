#include "PantallaAjustes.h"

#include <cmath>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

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

		// ---- Pestaña General ------------------------------------------------

		/// Fila con título, descripción que se ajusta al ancho e interruptor a la derecha.
		bool filaInterruptor(const char* id, const char* clave, const char* claveDesc, bool& valor,
			float w, Theme* tema, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 a = ImGui::GetCursorScreenPos();
			const float anchoSw = 42.0f * s;
			const float anchoTxt = w - anchoSw - 16.0f * s;

			ImGui::PushFont(AppFonts::label);
			const char* titulo = T(clave);
			const float altoTitulo = ImGui::CalcTextSize(titulo).y;
			dl->AddText(ImVec2(a.x, a.y + 6.0f * s), vis::col(tema->text), titulo);
			ImGui::PopFont();

			float altoDesc = 0.0f;
			if (claveDesc != nullptr) {
				ImGui::PushFont(AppFonts::input);
				const char* desc = T(claveDesc);
				altoDesc = ImGui::CalcTextSize(desc, nullptr, false, anchoTxt).y;
				dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(a.x, a.y + 6.0f * s + altoTitulo + 3.0f * s),
					vis::col(tema->textMuted), desc, nullptr, anchoTxt);
				ImGui::PopFont();
			}

			const float alto = (std::max)(altoTitulo + (altoDesc > 0.0f ? altoDesc + 3.0f * s : 0.0f) + 12.0f * s, 34.0f * s);
			ImGui::SetCursorScreenPos(ImVec2(a.x + w - anchoSw, a.y + (alto - 22.0f * s) * 0.5f));
			const bool clic = vis::interruptor(id, valor, tema->primary, s);
			if (clic) valor = !valor;

			dl->AddLine(ImVec2(a.x, a.y + alto), ImVec2(a.x + w, a.y + alto), vis::col(tema->text, 0.06f), 1.0f * s);
			ImGui::SetCursorScreenPos(a);
			ImGui::Dummy(ImVec2(w, alto));
			return clic;
		}

		/// Título + descripción de un campo de texto (el campo lo dibuja quien llama).
		void rotuloCampo(const char* clave, const char* claveDesc, float w, Theme* tema, float s) {
			ImGui::PushFont(AppFonts::label);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->text);
			ImGui::TextUnformatted(T(clave));
			ImGui::PopStyleColor();
			ImGui::PopFont();
			if (claveDesc != nullptr) {
				ImGui::PushFont(AppFonts::input);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w);
				ImGui::TextUnformatted(T(claveDesc));
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
				ImGui::PopFont();
			}
		}

		/// Campo de texto ligado a un std::string de la configuración. Devuelve true si el usuario lo cambió.
		bool campoTexto(const char* id, std::string& destino, char* buf, size_t tam, bool multilinea,
			float w, float alto, Theme* tema, float s) {
			// Mientras no se esté escribiendo, el búfer sigue a la configuración.
			ImGuiStorage* est = ImGui::GetStateStorage();
			const ImGuiID kid = ImGui::GetID(id);
			if (!est->GetBool(kid, false)) {
				std::strncpy(buf, destino.c_str(), tam - 1);
				buf[tam - 1] = '\0';
			}
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, tema->listItemBackground);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->text);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f * s);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f * s, 9.0f * s));
			ImGui::SetNextItemWidth(w);
			const bool cambio = multilinea
				? ImGui::InputTextMultiline(id, buf, tam, ImVec2(w, alto))
				: ImGui::InputText(id, buf, tam);
			est->SetBool(kid, ImGui::IsItemActive());
ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
			ImGui::PopFont();
			if (cambio) destino = buf;
			return cambio;
		}

		/// Marco de sección: rótulo, fondo de tarjeta y contenido con margen interior.
		template <class F>
		void seccion(const char* clave, float ancho, Theme* tema, float s, F contenido) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float pad = 18.0f * s;
			const ImVec2 a = ImGui::GetCursorScreenPos();
			dl->ChannelsSplit(2);
			dl->ChannelsSetCurrent(1);
			ImGui::SetCursorScreenPos(ImVec2(a.x + pad, a.y + pad));
			ImGui::BeginGroup();
			ImGui::PushFont(AppFonts::label);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->primary);
			ImGui::TextUnformatted(T(clave));
			ImGui::PopStyleColor();
			ImGui::PopFont();
			ImGui::Dummy(ImVec2(0, 4.0f * s));
			contenido(ancho - 2.0f * pad);
			ImGui::EndGroup();
			const ImVec2 fin(a.x + ancho, ImGui::GetCursorScreenPos().y + pad - 4.0f * s);
			dl->ChannelsSetCurrent(0);
			vis::tarjeta(dl, a, fin, tema->listItemBackground, tema->primary, 0.0f, s);
			dl->ChannelsMerge();
			ImGui::SetCursorScreenPos(a);
			ImGui::Dummy(ImVec2(ancho, fin.y - a.y));
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

	bool PantallaAjustes::general(ImVec2 pos, ImVec2 tam, float alfa) {
		bool abrirClasico = false;
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = ajustes::escala();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * s, 20 * s));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * s, 10 * s));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
		ImGui::Begin("##phx_ajustes_general", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		try {
			static char bufBot[96] = { 0 };
			static char bufDiscord[160] = { 0 };
			static char bufBienvenida[512] = { 0 };

			const float disp = ImGui::GetContentRegionAvail().x;
			const float sep = 14.0f * s;
			const bool dos = disp >= 840.0f * s;
			const float col = dos ? (disp - sep) * 0.5f : disp;
			Config& c = Config::cfg;

			// ---------- Columna izquierda ----------
			ImGui::BeginGroup();
			ajustes::seccion("aj.g_programa", col, tema, s, [&](float w) {
				ajustes::rotuloCampo("aj.g_tema", "aj.g_tema_d", w, tema, s);
				ImGui::PushFont(AppFonts::label);
				int n = 0;
				for (const auto& par : ThemeController::getInstance().getThemeNames()) {
					if (n++ > 0) ImGui::SameLine(0, 8.0f * s);
					const bool elegido = par.first == c.general.theme;
					const std::string id = par.second + "##tema";
					if (vis::chip(id.c_str(), par.second.c_str(), tema->primary, elegido, s) && !elegido) {
						c.general.theme = par.first;
						ThemeController::getInstance().applyTheme(c.general.theme);
						c.Save();
					}
				}
				ImGui::PopFont();
				ImGui::Dummy(ImVec2(0, 6.0f * s));
				if (ajustes::filaInterruptor("##g_flash", "aj.g_flash", "aj.g_flash_d", c.general.flashWindow, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_dev", "aj.g_dev", "aj.g_dev_d", c.general.devMode, w, tema, s)) c.Save();
			});

			ImGui::Dummy(ImVec2(0, 2.0f * s));
			ajustes::seccion("aj.g_chat", col, tema, s, [&](float w) {
				ajustes::rotuloCampo("aj.g_bot", "aj.g_bot_d", w, tema, s);
				if (ajustes::campoTexto("##g_bot", c.chat.chatbot, bufBot, sizeof(bufBot), false, w, 0.0f, tema, s)) {
					c.Save();
					c.chatbotName = "[" + c.chat.chatbot + "] ";
				}
				ImGui::Dummy(ImVec2(0, 4.0f * s));
				ajustes::rotuloCampo("aj.g_discord", "aj.g_discord_d", w, tema, s);
				if (ajustes::campoTexto("##g_discord", c.chat.discord, bufDiscord, sizeof(bufDiscord), false, w, 0.0f, tema, s)) c.Save();
				ImGui::Dummy(ImVec2(0, 4.0f * s));
				ajustes::rotuloCampo("aj.g_bienvenida", "aj.g_bienvenida_d", w, tema, s);
				if (ajustes::campoTexto("##g_bienvenida", c.chat.welcomeMessage, bufBienvenida, sizeof(bufBienvenida), true, w, 84.0f * s, tema, s)) c.Save();
				ImGui::Dummy(ImVec2(0, 6.0f * s));
				if (ajustes::filaInterruptor("##g_tts", "aj.g_tts", "aj.g_tts_d", c.chat.ttsEnabled, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_bonk", "aj.g_bonk", "aj.g_bonk_d", c.chat.bonkEnabled, w, tema, s)) c.Save();
			});
			ImGui::EndGroup();

			if (dos) ImGui::SameLine(0, sep);

			// ---------- Columna derecha ----------
			ImGui::BeginGroup();
			ajustes::seccion("aj.g_entrada", col, tema, s, [&](float w) {
				if (ajustes::filaInterruptor("##g_guia", "aj.g_guia", "aj.g_guia_d", c.input.disableGuideButton, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_teclado", "aj.g_teclado", "aj.g_teclado_d", c.input.disableKeyboard, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_autoindex", "aj.g_autoindex", "aj.g_autoindex_d", c.input.autoIndex, w, tema, s)) c.Save();
			});

			ImGui::Dummy(ImVec2(0, 2.0f * s));
			ajustes::seccion("aj.g_seguridad", col, tema, s, [&](float w) {
				if (ajustes::filaInterruptor("##g_ip", "aj.g_ip", "aj.g_ip_d", c.general.ipBan, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_vpn", "aj.g_vpn", "aj.g_vpn_d", c.general.blockVPN, w, tema, s)) c.Save();
				if (ajustes::filaInterruptor("##g_logs", "aj.g_logs", "aj.g_logs_d", c.general.parsecLogs, w, tema, s)) c.Save();
			});

			ImGui::Dummy(ImVec2(0, 2.0f * s));
			ajustes::seccion("aj.g_clasico", col, tema, s, [&](float w) {
				ajustes::rotuloCampo("aj.g_clasico_t", "aj.g_clasico_d", w, tema, s);
				ImGui::Dummy(ImVec2(0, 4.0f * s));
				ImGui::PushFont(AppFonts::label);
				if (vis::chip("##g_clasico", T("aj.g_clasico_b"), tema->secondary, false, s)) abrirClasico = true;
				ImGui::PopFont();
			});
			ImGui::EndGroup();
		}
		catch (...) {
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
		return abrirClasico;
	}

	bool PantallaAjustes::barraVolver(ImVec2 pos, ImVec2 tam, float alfa) {
		bool volver = false;
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = ajustes::escala();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14 * s, 7 * s));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
		ImGui::Begin("##phx_ajustes_volver", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		try {
			ImGui::PushFont(AppFonts::label);
			if (vis::chip("##g_volver", T("aj.g_volver"), tema->primary, false, s)) volver = true;
			ImGui::PopFont();
		}
		catch (...) {
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
		return volver;
	}

}
