#include "PantallaGente.h"

#include <cctype>
#include <cmath>
#include <string>
#include <vector>

#include "UiComun.h"
#include "../I18n.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../../globals/AppFonts.h"
#include "../../globals/AppIcons.h"
#include "../../services/ThemeController.h"

namespace phoenix {

	namespace gente {

		struct Ficha {
			std::string nombre;
			uint32_t parsecId = 0;
			int pingMs = -1;
			int mando = 0;   // 0 = espectador
		};

		struct Estado {
			uint32_t confirmarExpulsar = 0;
			double confirmarHasta = -1.0;
			double copiadoEn = -10.0;
		};

		Estado& estado() { static Estado e; return e; }

		float escala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}

		std::string iniciales(const std::string& n) {
			std::string r;
			bool nueva = true;
			for (char c : n) {
				if (c == ' ' || c == '_' || c == '-') { nueva = true; continue; }
				if (nueva && static_cast<unsigned char>(c) < 128) { r += static_cast<char>(std::toupper(static_cast<unsigned char>(c))); nueva = false; }
				if (r.size() == 2) break;
			}
			return r.empty() ? "?" : r;
		}

		/// Primer mando libre dentro de los activos (índice 0-based) o -1.
		int mandoLibre(const std::vector<AsientoVista>& asientos) {
			for (const AsientoVista& a : asientos) if (!a.ocupado) return a.numero - 1;
			return -1;
		}

		void vacio(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, Theme* tema, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 c(pos.x + tam.x * 0.5f, pos.y + tam.y * 0.38f);
			const float t = static_cast<float>(ImGui::GetTime());
			for (int i = 0; i < 3; i++) {
				const float f = std::fmod(t * 0.5f + i / 3.0f, 1.0f);
				dl->AddCircle(c, (34.0f + 60.0f * f) * s, vis::col(tema->primary, 0.30f * (1.0f - f)), 48, 2.0f * s);
			}
			if (AppIcons::users != nullptr) {
				const float l = 44.0f * s;
				dl->AddImage(AppIcons::users, ImVec2(c.x - l * 0.5f, c.y - l * 0.5f), ImVec2(c.x + l * 0.5f, c.y + l * 0.5f),
					ImVec2(0, 0), ImVec2(1, 1), vis::col(tema->primary));
			}
			ImGui::PushFont(AppFonts::title);
			const char* titulo = T("gente.vacio");
			const ImVec2 tt = ImGui::CalcTextSize(titulo);
			dl->AddText(ImVec2(c.x - tt.x * 0.5f, c.y + 80.0f * s), vis::col(tema->text), titulo);
			ImGui::PopFont();

			ImGui::PushFont(AppFonts::label);
			const bool abierta = sala != nullptr && sala->abierta();
			const char* sub = abierta ? T("gente.vacio_abierta") : T("gente.vacio_cerrada");
			const ImVec2 ts = ImGui::CalcTextSize(sub);
			dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y + 116.0f * s), vis::col(tema->textMuted), sub);
			if (abierta) {
				Estado& e = estado();
				const bool copiado = ImGui::GetTime() - e.copiadoEn < 1.5;
				const char* txt = copiado ? T("sala.copiado") : T("gente.copiar");
				const float ancho = 200.0f * s;
				ImGui::SetCursorScreenPos(ImVec2(c.x - ancho * 0.5f, c.y + 150.0f * s));
				if (vis::chip("##g_copiar", txt, tema->primary, true, s, ancho)) {
					const std::string enlace = sala->enlace();
					if (!enlace.empty()) { ImGui::SetClipboardText(enlace.c_str()); e.copiadoEn = ImGui::GetTime(); }
				}
			}
			ImGui::PopFont();
		}

		void ficha(ProveedorSala& sala, const Ficha& f, const std::vector<AsientoVista>& asientos,
			float ancho, Theme* tema, float s) {

			Estado& e = estado();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float alto = 132.0f * s;
			const ImVec2 a = ImGui::GetCursorScreenPos();
			const ImVec2 b(a.x + ancho, a.y + alto);

			// Hover de toda la ficha
			ImGui::PushID(static_cast<int>(f.parsecId));
			ImGuiStorage* st = ImGui::GetStateStorage();
			const ImGuiID k = ImGui::GetID("##h");
			const bool encima = ImGui::IsMouseHoveringRect(a, b);
			const float h = vis::acercar(st->GetFloat(k, 0.0f), encima ? 1.0f : 0.0f, 14.0f);
			st->SetFloat(k, h);

			const ImVec4 acento = f.mando > 0 ? tema->primary : tema->secondary;
			vis::tarjeta(dl, a, b, tema->listItemBackground, acento, h, s);

			// Avatar con iniciales
			const ImVec4 cAv = vis::colorDe(f.nombre.c_str());
			const ImVec2 cc(a.x + 36.0f * s, a.y + 38.0f * s);
			dl->AddCircleFilled(cc, (20.0f + 2.0f * h) * s, vis::col(cAv, 0.22f));
			dl->AddCircle(cc, (20.0f + 2.0f * h) * s, vis::col(cAv), 32, 2.0f * s);
			ImGui::PushFont(AppFonts::label);
			const std::string ini = iniciales(f.nombre);
			const ImVec2 ti = ImGui::CalcTextSize(ini.c_str());
			dl->AddText(ImVec2(cc.x - ti.x * 0.5f, cc.y - ti.y * 0.5f), vis::col(cAv), ini.c_str());
			ImGui::PopFont();

			// Nombre + rol
			ImGui::PushFont(AppFonts::label);
			dl->PushClipRect(a, ImVec2(b.x - 70.0f * s, b.y), true);
			dl->AddText(ImVec2(a.x + 66.0f * s, a.y + 18.0f * s), vis::col(tema->text), f.nombre.c_str());
			dl->PopClipRect();
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::input);
			const std::string rol = f.mando > 0 ? std::string(T("gente.mando")) + " " + std::to_string(f.mando) : T("gente.espectador");
			dl->AddText(ImVec2(a.x + 66.0f * s, a.y + 42.0f * s), vis::col(acento), rol.c_str());

			// Ping (punto que late)
			const ImVec4 cp = vis::colorPing(f.pingMs, tema->positive, tema->negative, tema->textMuted);
			const std::string ping = f.pingMs >= 0 ? std::to_string(f.pingMs) + " ms" : "-- ms";
			const ImVec2 tp = ImGui::CalcTextSize(ping.c_str());
			const float pulso = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.0f);
			dl->AddCircleFilled(ImVec2(b.x - tp.x - 28.0f * s, a.y + 26.0f * s), 4.5f * s, vis::col(cp, pulso));
			dl->AddText(ImVec2(b.x - tp.x - 18.0f * s, a.y + 18.0f * s), vis::col(cp), ping.c_str());

			// Acciones
			const float yAcc = a.y + alto - 44.0f * s;
			ImGui::SetCursorScreenPos(ImVec2(a.x + 14.0f * s, yAcc));
			if (f.mando > 0) {
				if (vis::chip("##quitar", T("gente.quitar_mando"), tema->primary, false, s)) sala.liberarMando(f.mando - 1);
			}
			else {
				const int libre = mandoLibre(asientos);
				if (libre >= 0) {
					if (vis::chip("##dar", T("gente.dar_mando"), tema->primary, false, s)) sala.asignarMando(libre, f.parsecId);
				}
				else {
					vis::chip("##lleno", T("gente.sin_mandos"), tema->textMuted, false, s);
				}
			}
			ImGui::SameLine(0, 8.0f * s);
			const double ahora = ImGui::GetTime();
			const bool confirmando = e.confirmarExpulsar == f.parsecId && ahora < e.confirmarHasta;
			if (vis::chip("##expulsar", confirmando ? T("gente.seguro") : T("gente.expulsar"), tema->negative, confirmando, s)) {
				if (confirmando) { sala.expulsar(f.parsecId); e.confirmarExpulsar = 0; }
				else { e.confirmarExpulsar = f.parsecId; e.confirmarHasta = ahora + 3.0; }
			}
			ImGui::PopFont();
			ImGui::PopID();

			ImGui::SetCursorScreenPos(a);
			ImGui::Dummy(ImVec2(ancho, alto));
		}
	}

	void PantallaGente::render(ProveedorSala* sala, ImVec2 pos, ImVec2 tam, float alfa) {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = gente::escala();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * s, 18 * s));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * s, 12 * s));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
		ImGui::Begin("##phx_gente", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		try {
			std::vector<gente::Ficha> fichas;
			std::vector<AsientoVista> asientos;
			if (sala != nullptr && sala->abierta()) {
				asientos = sala->asientos(PhoenixPrefs::get().mandosActivos);
				for (const AsientoVista& a : asientos)
					if (a.ocupado) fichas.push_back({ a.jugador, a.parsecId, a.pingMs, a.numero });
				for (const EspectadorVista& ev : sala->espectadores())
					fichas.push_back({ ev.nombre, ev.parsecId, ev.pingMs, 0 });
			}

			if (fichas.empty()) {
				gente::vacio(sala, ImGui::GetWindowPos(), ImGui::GetWindowSize(), tema, s);
			}
			else {
				// Contadores
				int jugadores = 0;
				for (const auto& f : fichas) if (f.mando > 0) jugadores++;
				ImGui::PushFont(AppFonts::label);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->primary);
				ImGui::Text("%d %s", jugadores, T("gente.jugadores"));
				ImGui::PopStyleColor();
				ImGui::SameLine(0, 18.0f * s);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->secondary);
				ImGui::Text("%d %s", static_cast<int>(fichas.size()) - jugadores, T("gente.espectadores"));
				ImGui::PopStyleColor();
				ImGui::PopFont();

				// Cuadrícula responsiva
				const float disp = ImGui::GetContentRegionAvail().x;
				const float minimo = 280.0f * s;
				const int columnas = (std::max)(1, static_cast<int>((disp + 12.0f * s) / (minimo + 12.0f * s)));
				const float ancho = (disp - 12.0f * s * (columnas - 1)) / columnas;
				for (size_t i = 0; i < fichas.size(); i++) {
					if (i % columnas != 0) ImGui::SameLine(0, 12.0f * s);
					gente::ficha(*sala, fichas[i], asientos, ancho, tema, s);
				}
			}
		}
		catch (...) {
			// Una ficha rota no debe tumbar la pantalla.
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
	}

}
