#include "TableroMandos.h"

#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "imgui.h"
#include "UiComun.h"
#include "../I18n.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../core/Solicitudes.h"
#include "../core/AccionesSala.h"
#include "../PhoenixRoles.h"
#include <cmath>
#include "../../globals/AppFonts.h"
#include "../../globals/AppIcons.h"
#include "../../services/ThemeController.h"

namespace phoenix {

	namespace {

		const char* kPayload = "PHX_PARSEC";

		float tabEscala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}

		/// Botón pequeño redondo con texto. Devuelve true al hacer clic.
		bool tabMini(const char* id, ImVec2 p, float lado, const char* texto, const ImVec4& color, float s) {
			ImGui::SetCursorScreenPos(p);
			const bool clic = ImGui::InvisibleButton(id, ImVec2(lado, lado));
			const bool encima = ImGui::IsItemHovered();
			if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(p, ImVec2(p.x + lado, p.y + lado), vis::col(color, encima ? 0.35f : 0.18f), lado * 0.5f);
			const ImVec2 t = ImGui::CalcTextSize(texto);
			dl->AddText(ImVec2(p.x + (lado - t.x) * 0.5f, p.y + (lado - t.y) * 0.5f), vis::col(color), texto);
			return clic;
		}

		/// Selector «etiqueta  −  n  +» dibujado en `p`. Devuelve el nuevo valor.
		int tabStepper(const char* id, ImVec2 p, int valor, int minimo, int maximo, const char* etiqueta, Theme* tema, float s) {
			ImGui::PushID(id);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float lado = 30 * s;
			ImFont* f = AppFonts::label;
			const ImVec2 te = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, etiqueta);
			dl->AddText(f, f->FontSize, ImVec2(p.x, p.y + (lado - te.y) * 0.5f), vis::col(tema->textMuted), etiqueta);
			float x = p.x + te.x + 10 * s;
			if (tabMini("##menos", ImVec2(x, p.y), lado, "-", valor > minimo ? tema->primary : tema->textMuted, s) && valor > minimo) valor--;
			char n[8];
			snprintf(n, sizeof(n), "%d", valor);
			const ImVec2 tn = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, n);
			x += lado + 10 * s;
			dl->AddText(f, f->FontSize, ImVec2(x, p.y + (lado - tn.y) * 0.5f), vis::col(tema->text), n);
			x += tn.x + 10 * s;
			if (tabMini("##mas", ImVec2(x, p.y), lado, "+", valor < maximo ? tema->primary : tema->textMuted, s) && valor < maximo) valor++;
			ImGui::PopID();
			return valor;
		}

		/// Una tarjeta de mando. Devuelve la altura usada.
		void tabTarjeta(ProveedorSala& sala, const AsientoVista& a, int indice, ImVec2 p0, ImVec2 tam,
			const ImVec4& colorEquipo, Theme* tema, float s, bool compacto) {

			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 p1(p0.x + tam.x, p0.y + tam.y);
			ImGui::PushID(indice);

			// Zona interactiva de toda la tarjeta (clic, arrastrar y soltar)
			ImGui::SetCursorScreenPos(p0);
			const bool clic = ImGui::InvisibleButton("##tarjeta", tam);
			const bool encima = ImGui::IsItemHovered();

			ImGuiStorage* st = ImGui::GetStateStorage();
			const ImGuiID claveHover = ImGui::GetID("##h");
			float h = st->GetFloat(claveHover, 0.0f);
			h = vis::acercar(h, encima ? 1.0f : 0.0f, 16.0f);
			st->SetFloat(claveHover, h);

			if (a.ocupado && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload(kPayload, &a.parsecId, sizeof(a.parsecId));
				ImGui::Text("%s  ->  %s", a.jugador.c_str(), T("mandos.soltar"));
				ImGui::EndDragDropSource();
			}
			bool resaltarDrop = false;
			if (ImGui::BeginDragDropTarget()) {
				resaltarDrop = true;
				if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload(kPayload)) {
					uint32_t id = 0;
					memcpy(&id, pl->Data, sizeof(id));
					if (!a.conectado) sala.conectarMando(indice);
					sala.asignarMando(indice, id);
				}
				ImGui::EndDragDropTarget();
			}
			if (clic && !a.conectado) sala.conectarMando(indice);
			if (encima && !a.conectado) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			// Fondo y borde
			const float lift = 2.0f * s * h;
			const ImVec2 q0(p0.x, p0.y - lift), q1(p1.x, p1.y - lift);
			const ImVec4 fondo = a.ocupado ? tema->listItemBackground : tema->formInputBackground;
			dl->AddRectFilled(q0, q1, vis::col(fondo), 12 * s);
			const float bordeA = resaltarDrop ? 1.0f : (a.ocupado ? 0.55f : 0.18f) + 0.25f * h;
			dl->AddRect(q0, q1, vis::col(resaltarDrop ? tema->primary : colorEquipo, bordeA), 12 * s, 0, (resaltarDrop ? 2.0f : 1.0f) * s);
			dl->AddRectFilled(ImVec2(q0.x, q0.y + 12 * s), ImVec2(q0.x + 4 * s, q1.y - 12 * s), vis::col(colorEquipo, a.ocupado ? 1.0f : 0.35f), 2 * s);

			// Número del mando
			char num[8];
			snprintf(num, sizeof(num), "%d", a.numero);
			ImFont* fn = AppFonts::title;
			const float tamNum = fn->FontSize * (compacto ? 1.0f : 1.3f);
			dl->AddText(fn, tamNum, ImVec2(q0.x + 16 * s, q0.y + (tam.y - tamNum) * 0.5f), vis::col(colorEquipo, a.ocupado ? 1.0f : 0.5f), num);

			// Nombre / estado
			const float xTexto = q0.x + 16 * s + 34 * s;
			const char* principal = a.ocupado ? a.jugador.c_str()
				: (a.conectado ? T("sala.libre") : T("mandos.conectar"));
			dl->PushClipRect(ImVec2(xTexto, q0.y), ImVec2(q1.x - 70 * s, q1.y), true);
			dl->AddText(AppFonts::label, AppFonts::label->FontSize, ImVec2(xTexto, q0.y + tam.y * 0.5f - AppFonts::label->FontSize + 2 * s),
				vis::col(a.ocupado ? tema->text : (a.conectado ? tema->textMuted : tema->primary)), principal);
			const char* sub = a.bloqueado ? T("mandos.bloqueado")
				: a.ocupado ? T("mandos.arrastra") : (a.conectado ? T("mandos.arrastra_aqui") : T("mandos.clic_conectar"));
			dl->AddText(AppFonts::input, AppFonts::input->FontSize * 0.85f, ImVec2(xTexto, q0.y + tam.y * 0.5f + 4 * s),
				vis::col(a.bloqueado ? tema->negative : tema->textMuted, 0.8f), sub);
			dl->PopClipRect();

			// Ping
			if (a.ocupado) {
				const ImVec4 cp = vis::colorPing(a.pingMs, tema->positive, tema->negative, tema->textMuted);
				char ping[16];
				if (a.pingMs >= 0) snprintf(ping, sizeof(ping), "%d ms", a.pingMs); else snprintf(ping, sizeof(ping), "— ms");
				const ImVec2 tp = ImGui::CalcTextSize(ping);
				const ImVec2 c0(q1.x - tp.x - 26 * s, q0.y + 10 * s);
				dl->AddCircleFilled(ImVec2(c0.x + 6 * s, c0.y + tp.y * 0.5f), 4 * s, vis::col(cp));
				dl->AddText(ImVec2(c0.x + 14 * s, c0.y), vis::col(cp), ping);
			}

			// Acciones al pasar el mouse (solo mandos conectados)
			if (a.conectado && h > 0.3f) {
				const float lado = 24 * s;
				const float y = q1.y - lado - 8 * s;
				ImVec2 pb(q1.x - lado - 10 * s, y);
				if (a.ocupado && tabMini("##liberar", pb, lado, "x", tema->negative, s)) sala.liberarMando(indice);
				pb.x -= lado + 6 * s;
				if (tabMini("##bloq", pb, lado, a.bloqueado ? "U" : "L", a.bloqueado ? tema->positive : tema->secondary, s)) sala.alternarBloqueo(indice);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", a.bloqueado ? T("mandos.desbloquear") : T("mandos.bloquear"));
			}

			ImGui::PopID();
		}

		void tabColumna(ProveedorSala& sala, const std::vector<AsientoVista>& asientos, int desde, int hasta,
			const char* titulo, const ImVec4& color, ImVec2 origen, float ancho, Theme* tema, float s, bool compacto, float& altoUsado) {

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddText(AppFonts::label, AppFonts::label->FontSize, origen, vis::col(color), titulo);
			char cuenta[16];
			snprintf(cuenta, sizeof(cuenta), "%d", hasta - desde);
			const ImVec2 tt = AppFonts::label->CalcTextSizeA(AppFonts::label->FontSize, FLT_MAX, 0, titulo);
			dl->AddText(AppFonts::label, AppFonts::label->FontSize, ImVec2(origen.x + tt.x + 8 * s, origen.y), vis::col(tema->textMuted), cuenta);

			const float sep = 10 * s;
			const float altoTarjeta = (compacto ? 64.0f : 78.0f) * s;
			const int columnas = ancho >= 2 * 220 * s + sep ? 2 : 1;
			const float anchoT = (ancho - sep * (columnas - 1)) / columnas;
			const float y0 = origen.y + 30 * s;
			int n = 0;
			for (int i = desde; i < hasta && i < static_cast<int>(asientos.size()); i++, n++) {
				const int f = n / columnas, c = n % columnas;
				tabTarjeta(sala, asientos[i], i, ImVec2(origen.x + c * (anchoT + sep), y0 + f * (altoTarjeta + sep)),
					ImVec2(anchoT, altoTarjeta), color, tema, s, compacto);
			}
			const int filas = (n + columnas - 1) / columnas;
			altoUsado = 30 * s + filas * (altoTarjeta + sep);
		}
		/// Solicitudes de cambio de los jugadores: tarjeta con Aceptar / Rechazar.
		/// Quien entró directo por Parsec espera: el host elige Jugador, Espectador o Expulsar.
		void tabEnEspera(ProveedorSala& sala, const std::vector<AsientoVista>& asientos, float ancho, Theme* tema, float s) {
			const std::vector<uint32_t> ids = PhoenixRoles::instancia().enEspera();
			if (ids.empty()) return;
			const std::vector<EspectadorVista> presentes = sala.espectadores();
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float pulso = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.0f);

			for (uint32_t id : ids) {
				const EspectadorVista* quien = nullptr;
				for (const EspectadorVista& ev : presentes) if (ev.parsecId == id) quien = &ev;
				if (quien == nullptr) continue;   // ya se fue (o ya tiene mando)

				const ImVec2 p0 = ImGui::GetCursorScreenPos();
				const float alto = 48 * s;
				dl->AddRectFilled(p0, ImVec2(p0.x + ancho, p0.y + alto), vis::col(tema->primary, 0.12f), 12 * s);
				dl->AddRect(p0, ImVec2(p0.x + ancho, p0.y + alto), vis::col(tema->primary, 0.5f * pulso + 0.3f), 12 * s, 0, 1.5f * s);
				char texto[160];
				snprintf(texto, sizeof(texto), T("mandos.entro_parsec"), quien->nombre.c_str());
				dl->AddText(AppFonts::label, AppFonts::label->FontSize, ImVec2(p0.x + 16 * s, p0.y + (alto - AppFonts::label->FontSize) * 0.5f),
					vis::col(tema->text), texto);

				ImGui::PushID(static_cast<int>(id) ^ 0x5A5A);
				ImGui::PushFont(AppFonts::label);
				const float w1 = ImGui::CalcTextSize(T("mandos.como_jugador")).x + 26 * s;
				const float w2 = ImGui::CalcTextSize(T("mandos.como_espectador")).x + 26 * s;
				const float w3 = ImGui::CalcTextSize(T("gente.expulsar")).x + 26 * s;
				ImGui::SetCursorScreenPos(ImVec2(p0.x + ancho - w1 - w2 - w3 - 32 * s, p0.y + (alto - 32 * s) * 0.5f));
				const bool jugador = vis::chip("##jug", T("mandos.como_jugador"), tema->primary, true, s);
				ImGui::SameLine(0, 6 * s);
				const bool espectador = vis::chip("##esp", T("mandos.como_espectador"), tema->secondary, false, s);
				ImGui::SameLine(0, 6 * s);
				const bool fuera = vis::chip("##fuera", T("gente.expulsar"), tema->negative, false, s);
				ImGui::PopFont();
				ImGui::PopID();

				// Misma lógica que la interfaz web (core/AccionesSala)
				if (jugador) AccionesSala::decidirEspera(sala, id, DecisionEspera::Jugador);
				else if (espectador) AccionesSala::decidirEspera(sala, id, DecisionEspera::Espectador);
				else if (fuera) AccionesSala::decidirEspera(sala, id, DecisionEspera::Expulsar);

				ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + alto + 8 * s));
				ImGui::Dummy(ImVec2(ancho, 1));
			}
		}

		void tabSolicitudes(ProveedorSala& sala, const std::vector<AsientoVista>& asientos, float ancho, Theme* tema, float s) {
			const std::vector<Solicitud> lista = Solicitudes::instancia().pendientes();
			if (lista.empty()) return;
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float pulso = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.0f);

			for (size_t k = 0; k < lista.size(); k++) {
				const Solicitud& sol = lista[k];
				const ImVec2 p0 = ImGui::GetCursorScreenPos();
				const float alto = 48 * s;
				dl->AddRectFilled(p0, ImVec2(p0.x + ancho, p0.y + alto), vis::col(tema->secondary, 0.16f), 12 * s);
				dl->AddRect(p0, ImVec2(p0.x + ancho, p0.y + alto), vis::col(tema->secondary, 0.5f * pulso + 0.3f), 12 * s, 0, 1.5f * s);

				char texto[160];
				if (sol.mandoDestino > 0) snprintf(texto, sizeof(texto), T("mandos.pide_mando"), sol.nombre.c_str(), sol.mandoDestino);
				else snprintf(texto, sizeof(texto), T("mandos.pide_equipo"), sol.nombre.c_str());
				dl->AddText(AppFonts::label, AppFonts::label->FontSize, ImVec2(p0.x + 16 * s, p0.y + (alto - AppFonts::label->FontSize) * 0.5f),
					vis::col(tema->text), texto);

				ImGui::PushID(static_cast<int>(sol.parsecId));
				const float lado = 34 * s;
				const float wBoton = 110 * s;
				ImGui::SetCursorScreenPos(ImVec2(p0.x + ancho - 2 * wBoton - 20 * s, p0.y + (alto - lado) * 0.5f));
				ImGui::PushStyleColor(ImGuiCol_Button, tema->primary);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tema->buttonPrimaryHovered);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->buttonPrimaryText);
				const bool aceptar = ImGui::Button(T("mandos.aceptar"), ImVec2(wBoton, lado));
				ImGui::PopStyleColor(3);
				ImGui::SameLine(0, 8 * s);
				const bool rechazar = ImGui::Button(T("mandos.rechazar"), ImVec2(wBoton, lado));
				ImGui::PopID();

				// Misma lógica que la interfaz web (core/AccionesSala)
				if (aceptar) AccionesSala::aceptarSolicitud(sala, sol.parsecId);
				else if (rechazar) AccionesSala::rechazarSolicitud(sol.parsecId);
				ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + alto + 8 * s));
				ImGui::Dummy(ImVec2(ancho, 1));
			}
		}
	}

	void TableroMandos::render(ProveedorSala& sala, float ancho, bool compacto) {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = tabEscala();
		PhoenixPrefs& pr = PhoenixPrefs::get();
		bool cambio = false;

		std::vector<AsientoVista> asientos = sala.asientos(8);
		if (asientos.empty()) {
			ImGui::TextColored(tema->textMuted, "%s", T("sala.sin_mando"));
			return;
		}
		const int maximo = (std::min)(8, static_cast<int>(asientos.size()));

		tabEnEspera(sala, asientos, ancho, tema, s);
		tabSolicitudes(sala, asientos, ancho, tema, s);

		// Formación: cuántos mandos y cómo se reparten
		{
			const int antesMandos = pr.mandosActivos, antesLocal = pr.equipoLocal;
			// Cada equipo con su propio contador: «Local [-] 3 [+]  VS  Visitante [-] 3 [+]»
			int local = (std::max)(1, (std::min)(pr.equipoLocal, maximo - 1));
			int visita = (std::max)(1, (std::min)(pr.mandosActivos - pr.equipoLocal, maximo - local));
			const ImVec2 o = ImGui::GetCursorScreenPos();
			local = tabStepper("local", o, local, 1, maximo - visita, T("mandos.equipo_local"), tema, s);
			ImFont* ft = AppFonts::title;
			ImGui::GetWindowDrawList()->AddText(ft, ft->FontSize, ImVec2(o.x + 230 * s, o.y + 2 * s), vis::col(tema->text), "VS");
			visita = tabStepper("visita", ImVec2(o.x + 285 * s, o.y), visita, 1, maximo - local, T("mandos.equipo_visitante"), tema, s);

			// Atajos: un toque y listo
			ImGui::PushFont(AppFonts::label);
			const bool abajo = 560 * s + (maximo / 2) * 84 * s > ancho;   // sin espacio: atajos en otra fila
			float x = abajo ? o.x : o.x + 560 * s;
			const float yChips = abajo ? o.y + 40 * s : o.y - 2 * s;
			for (int n = 1; n * 2 <= maximo; n++) {
				char t[16];
				snprintf(t, sizeof(t), "%dvs%d", n, n);
				const bool activa = local == n && visita == n;
				ImGui::SetCursorScreenPos(ImVec2(x, yChips));
				ImGui::PushID(n);
				if (vis::chip("##form", t, tema->primary, activa, s)) { local = n; visita = n; }
				ImGui::PopID();
				x += ImGui::CalcTextSize(t).x + 34 * s;
			}
			ImGui::PopFont();

			pr.equipoLocal = local;
			pr.mandosActivos = local + visita;
			ImGui::SetCursorScreenPos(ImVec2(o.x, o.y + (abajo ? 76 : 34) * s));
			ImGui::Dummy(ImVec2(ancho, 1));
			cambio = antesMandos != pr.mandosActivos || antesLocal != pr.equipoLocal;
		}
		ImGui::Dummy(ImVec2(0, 10 * s));

		// Dos columnas: LOCAL (cian) y VISITANTE (púrpura)
		const float sep = 16 * s;
		const float anchoCol = (ancho - sep) * 0.5f;
		const ImVec2 origen = ImGui::GetCursorScreenPos();
		float altoA = 0, altoB = 0;
		tabColumna(sala, asientos, 0, pr.equipoLocal, T("mandos.equipo_local"), tema->primary, origen, anchoCol, tema, s, compacto, altoA);
		tabColumna(sala, asientos, pr.equipoLocal, pr.mandosActivos, T("mandos.equipo_visitante"), tema->secondary,
			ImVec2(origen.x + anchoCol + sep, origen.y), anchoCol, tema, s, compacto, altoB);
		ImGui::SetCursorScreenPos(ImVec2(origen.x, origen.y + (std::max)(altoA, altoB)));
		ImGui::Dummy(ImVec2(ancho, 2 * s));

		// Espectadores arrastrables
		const std::vector<EspectadorVista> lista = sala.espectadores();
		ImGui::PushFont(AppFonts::label);
		ImGui::TextColored(tema->textMuted, "%s  ·  %d", T("sala.espectadores"), static_cast<int>(lista.size()));
		ImGui::PopFont();
		if (lista.empty()) {
			ImGui::PushFont(AppFonts::input);
			ImGui::TextColored(tema->textMuted, "%s", T("sala.sin_espectadores"));
			ImGui::PopFont();
		}
		else {
			ImGui::PushFont(AppFonts::input);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 o = ImGui::GetCursorScreenPos();
			float x = o.x, y = o.y;
			const float alto = 34 * s;
			for (size_t i = 0; i < lista.size(); i++) {
				const EspectadorVista& e = lista[i];
				const ImVec2 tt = ImGui::CalcTextSize(e.nombre.c_str());
				const float w = tt.x + 44 * s;
				if (x + w > o.x + ancho && x > o.x) { x = o.x; y += alto + 8 * s; }
				ImGui::SetCursorScreenPos(ImVec2(x, y));
				ImGui::PushID(static_cast<int>(i) + 1000);
				ImGui::InvisibleButton("##esp", ImVec2(w, alto));
				const bool encima = ImGui::IsItemHovered();
				if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload(kPayload, &e.parsecId, sizeof(e.parsecId));
					ImGui::Text("%s  ->  %s", e.nombre.c_str(), T("mandos.soltar"));
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();
				const ImVec4 cp = vis::colorPing(e.pingMs, tema->positive, tema->negative, tema->textMuted);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + alto), vis::col(tema->listItemBackground, encima ? 1.0f : 0.8f), alto * 0.5f);
				if (encima) dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + alto), vis::col(tema->primary, 0.6f), alto * 0.5f);
				dl->AddText(ImVec2(x + 12 * s, y + (alto - tt.y) * 0.5f), vis::col(tema->textMuted, 0.7f), "::");
				dl->AddCircleFilled(ImVec2(x + 28 * s, y + alto * 0.5f), 4 * s, vis::col(cp));
				dl->AddText(ImVec2(x + 36 * s, y + (alto - tt.y) * 0.5f), vis::col(tema->text), e.nombre.c_str());
				x += w + 8 * s;
			}
			ImGui::SetCursorScreenPos(ImVec2(o.x, y + alto + 6 * s));
			ImGui::TextColored(ImVec4(tema->textMuted.x, tema->textMuted.y, tema->textMuted.z, 0.7f), "%s", T("mandos.ayuda_arrastrar"));
			ImGui::PopFont();
		}

		if (cambio) pr.guardar();
	}

}
