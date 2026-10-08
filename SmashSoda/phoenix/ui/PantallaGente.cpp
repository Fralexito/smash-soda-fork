#include "PantallaGente.h"

#include <cctype>
#include <cfloat>
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
			uint32_t seleccion = 0;
			uint32_t confirmarExpulsar = 0;
			uint32_t confirmarBanear = 0;
			double confirmarHasta = -1.0;
			double avisoHasta = -1.0;
			double copiadoEn = -10.0;
			char filtro[64] = { 0 };
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

		std::string minusculas(std::string t) {
			for (char& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return t;
		}

		const ImVec4 kAmbar(0.96f, 0.71f, 0.27f, 1.0f);

		/// Píldora de texto pegada a `xDer`. Devuelve su ancho. (La fuente la elige quien llama.)
		float insignia(ImDrawList* dl, float xDer, float y, const char* t, const ImVec4& c, float s) {
			const ImVec2 ts = ImGui::CalcTextSize(t);
			const float w = ts.x + 14.0f * s, h = ts.y + 6.0f * s;
			const ImVec2 a(xDer - w, y), b(xDer, y + h);
			dl->AddRectFilled(a, b, vis::col(c, 0.14f), h * 0.5f);
			dl->AddRect(a, b, vis::col(c, 0.50f), h * 0.5f, 0, 1.0f * s);
			dl->AddText(ImVec2(a.x + 7.0f * s, a.y + 3.0f * s), vis::col(c), t);
			return w;
		}

		/// Interruptor animado (como el del diseño). Devuelve true al clic.
		bool interruptor(const char* id, bool activo, const ImVec4& color, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 tam(42.0f * s, 22.0f * s);
			const ImVec2 p = ImGui::GetCursorScreenPos();
			const ImGuiID k = ImGui::GetID(id);
			const bool clic = ImGui::InvisibleButton(id, tam);
			if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGuiStorage* st = ImGui::GetStateStorage();
			const float meta = activo ? 1.0f : 0.0f;
			const float v = vis::acercar(st->GetFloat(k, meta), meta, 16.0f);
			st->SetFloat(k, v);
			const ImVec4 pista = vis::mezclar(ImVec4(1, 1, 1, 0.12f), ImVec4(color.x, color.y, color.z, 0.85f), v);
			dl->AddRectFilled(p, ImVec2(p.x + tam.x, p.y + tam.y), vis::col(pista), tam.y * 0.5f);
			const float r = tam.y * 0.5f;
			const ImVec2 c(p.x + r + (tam.x - tam.y) * v, p.y + r);
			dl->AddCircleFilled(c, r - 3.0f * s, vis::col(ImVec4(1, 1, 1, 1)));
			return clic;
		}

		void etiqueta(ImDrawList* dl, float x, float& y, const char* t, Theme* tema, float s) {
			dl->AddText(ImVec2(x, y), vis::col(tema->textMuted), t);
			y += ImGui::CalcTextSize(t).y + 8.0f * s;
		}

		/// Fila de la lista. Devuelve true al clic.
		bool fila(ProveedorSala& sala, const Ficha& f, bool sel, float ancho, Theme* tema, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float alto = 58.0f * s;
			const ImVec2 a = ImGui::GetCursorScreenPos();
			const ImVec2 b(a.x + ancho, a.y + alto);

			ImGui::PushID(static_cast<int>(f.parsecId));
			const ImGuiID k = ImGui::GetID("##h");
			const bool clic = ImGui::InvisibleButton("##fila", ImVec2(ancho, alto));
			const bool encima = ImGui::IsItemHovered();
			if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGuiStorage* st = ImGui::GetStateStorage();
			const float h = vis::acercar(st->GetFloat(k, 0.0f), encima ? 1.0f : 0.0f, 14.0f);
			st->SetFloat(k, h);

			const ImVec4 acento = f.mando > 0 ? tema->primary : tema->secondary;
			vis::tarjeta(dl, a, b, tema->listItemBackground, tema->primary, h, s, sel);

			// Avatar
			const ImVec4 cAv = vis::colorDe(f.nombre.c_str());
			const ImVec2 cc(a.x + 30.0f * s, a.y + alto * 0.5f);
			dl->AddCircleFilled(cc, (17.0f + 1.5f * h) * s, vis::col(cAv, 0.22f));
			dl->AddCircle(cc, (17.0f + 1.5f * h) * s, vis::col(cAv), 32, 2.0f * s);
			ImGui::PushFont(AppFonts::label);
			const std::string ini = iniciales(f.nombre);
			const ImVec2 ti = ImGui::CalcTextSize(ini.c_str());
			dl->AddText(ImVec2(cc.x - ti.x * 0.5f, cc.y - ti.y * 0.5f), vis::col(cAv), ini.c_str());
			ImGui::PopFont();

			// Derecha: ping (punto que late) + insignias
			ImGui::PushFont(AppFonts::input);
			const ImVec4 cp = vis::colorPing(f.pingMs, tema->positive, tema->negative, tema->textMuted);
			const std::string ping = f.pingMs >= 0 ? std::to_string(f.pingMs) + " ms" : "-- ms";
			const ImVec2 tp = ImGui::CalcTextSize(ping.c_str());
			const float xPing = b.x - 14.0f * s - tp.x;
			dl->AddText(ImVec2(xPing, a.y + (alto - tp.y) * 0.5f), vis::col(cp), ping.c_str());
			const float pulso = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.0f);
			dl->AddCircleFilled(ImVec2(xPing - 10.0f * s, a.y + alto * 0.5f), 4.0f * s, vis::col(cp, pulso));

			float xDer = xPing - 24.0f * s;
			const float limite = a.x + 58.0f * s + 110.0f * s;   // el nombre siempre conserva sitio
			struct Ins { const char* t; ImVec4 c; bool on; };
			const Ins ins[] = {
				{ T("gente.b_mod"), tema->primary, sala.esMod(f.parsecId) },
				{ T("gente.b_vip"), kAmbar, sala.esVip(f.parsecId) },
				{ T("gente.b_teclado"), tema->secondary, sala.tecladoPermitido(f.parsecId) },
				{ T("gente.b_raton"), tema->secondary, sala.ratonPermitido(f.parsecId) } };
			const float hIns = ImGui::CalcTextSize("A").y + 6.0f * s;
			for (const Ins& i : ins) {
				if (!i.on) continue;
				const float w = ImGui::CalcTextSize(i.t).x + 14.0f * s;
				if (xDer - w < limite) break;
				insignia(dl, xDer, a.y + (alto - hIns) * 0.5f, i.t, i.c, s);
				xDer -= w + 6.0f * s;
			}

			// Segunda línea
			std::string sub = "#" + std::to_string(f.parsecId) + "  ·  ";
			sub += f.mando > 0 ? std::string(T("gente.jugando")) + " " + std::to_string(f.mando) : std::string(T("gente.espectador"));
			dl->PushClipRect(ImVec2(a.x + 58.0f * s, a.y), ImVec2(xDer, b.y), true);
			dl->AddText(ImVec2(a.x + 58.0f * s, a.y + 33.0f * s), vis::col(acento), sub.c_str());
			dl->PopClipRect();
			ImGui::PopFont();

			// Nombre
			ImGui::PushFont(AppFonts::label);
			dl->PushClipRect(ImVec2(a.x + 58.0f * s, a.y), ImVec2(xDer, b.y), true);
			dl->AddText(ImVec2(a.x + 58.0f * s, a.y + 9.0f * s), vis::col(tema->text), f.nombre.c_str());
			dl->PopClipRect();
			ImGui::PopFont();

			ImGui::PopID();
			return clic;
		}

		/// Ficha del invitado elegido (panel derecho).
		void detalle(ProveedorSala& sala, const Ficha* f, const std::vector<AsientoVista>& asientos,
			ImVec2 tam, Theme* tema, float s) {

			Estado& e = estado();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 org = ImGui::GetCursorScreenPos();
			const ImVec2 fin(org.x + tam.x, org.y + tam.y);
			const float pad = 18.0f * s;
			vis::tarjeta(dl, org, fin, tema->listItemBackground, tema->primary, 0.0f, s);

			if (f == nullptr) {
				ImGui::PushFont(AppFonts::label);
				const char* t = T("gente.elige");
				const ImVec2 ts = ImGui::CalcTextSize(t);
				dl->AddText(ImVec2(org.x + (tam.x - ts.x) * 0.5f, org.y + (tam.y - ts.y) * 0.5f), vis::col(tema->textMuted), t);
				ImGui::PopFont();
				ImGui::Dummy(tam);
				return;
			}

			const float x0 = org.x + pad;
			const float wc = tam.x - 2.0f * pad;
			float y = org.y + pad;
			const uint32_t id = f->parsecId;
			const ImVec4 acento = f->mando > 0 ? tema->primary : tema->secondary;

			// Cabecera: avatar grande + nombre + #id + ping
			const ImVec4 cAv = vis::colorDe(f->nombre.c_str());
			const ImVec2 cc(x0 + 26.0f * s, y + 26.0f * s);
			dl->AddCircleFilled(cc, 26.0f * s, vis::col(cAv, 0.22f));
			dl->AddCircle(cc, 26.0f * s, vis::col(cAv), 40, 2.0f * s);
			ImGui::PushFont(AppFonts::title);
			const std::string ini = iniciales(f->nombre);
			const ImVec2 ti = ImGui::CalcTextSize(ini.c_str());
			dl->AddText(ImVec2(cc.x - ti.x * 0.5f, cc.y - ti.y * 0.5f), vis::col(cAv), ini.c_str());
			ImGui::PopFont();

			ImGui::PushFont(AppFonts::title);
			dl->PushClipRect(ImVec2(x0 + 64.0f * s, y), ImVec2(x0 + wc, y + 60.0f * s), true);
			dl->AddText(ImVec2(x0 + 64.0f * s, y), vis::col(tema->text), f->nombre.c_str());
			dl->PopClipRect();
			ImGui::PopFont();

			ImGui::PushFont(AppFonts::input);
			const ImVec4 cp = vis::colorPing(f->pingMs, tema->positive, tema->negative, tema->textMuted);
			const std::string linea = "#" + std::to_string(id) + "  ·  " +
				(f->pingMs >= 0 ? std::to_string(f->pingMs) + " ms" : std::string("-- ms")) + "  ·  " +
				(f->mando > 0 ? std::string(T("gente.mando")) + " " + std::to_string(f->mando) : std::string(T("gente.espectador")));
			dl->AddText(ImVec2(x0 + 64.0f * s, y + 32.0f * s), vis::col(f->pingMs >= 0 ? cp : acento), linea.c_str());
			y += 70.0f * s;

			// --- Roles ---
			dl->AddLine(ImVec2(x0, y), ImVec2(x0 + wc, y), vis::col(tema->text, 0.08f), 1.0f * s);
			y += 14.0f * s;
			etiqueta(dl, x0, y, T("gente.roles"), tema, s);
			const bool vip = sala.esVip(id), mod = sala.esMod(id);
			{
				const char* tv = vip ? T("gente.quitar_vip") : T("gente.hacer_vip");
				const char* tm = mod ? T("gente.quitar_mod") : T("gente.hacer_mod");
				const float wv = ImGui::CalcTextSize(tv).x + 26.0f * s;
				const float wm = ImGui::CalcTextSize(tm).x + 26.0f * s;
				float x = x0;
				ImGui::SetCursorScreenPos(ImVec2(x, y));
				if (vis::chip("##vip", tv, kAmbar, vip, s)) sala.alternarVip(id, f->nombre);
				x += wv + 8.0f * s;
				if (x + wm > x0 + wc) { x = x0; y += 40.0f * s; }
				ImGui::SetCursorScreenPos(ImVec2(x, y));
				if (vis::chip("##mod", tm, tema->primary, mod, s)) sala.alternarMod(id, f->nombre);
				y += 32.0f * s + 18.0f * s;
			}

			// --- Permisos ---
			dl->AddLine(ImVec2(x0, y), ImVec2(x0 + wc, y), vis::col(tema->text, 0.08f), 1.0f * s);
			y += 14.0f * s;
			etiqueta(dl, x0, y, T("gente.permisos"), tema, s);
			{
				struct Perm { const char* id; const char* txt; bool on; bool teclado; };
				const Perm perms[] = {
					{ "##p_tec", T("gente.permitir_teclado"), sala.tecladoPermitido(id), true },
					{ "##p_rat", T("gente.permitir_raton"), sala.ratonPermitido(id), false } };
				for (const Perm& p : perms) {
					const float hTxt = ImGui::CalcTextSize(p.txt).y;
					dl->AddText(ImVec2(x0, y + (22.0f * s - hTxt) * 0.5f), vis::col(tema->text), p.txt);
					ImGui::SetCursorScreenPos(ImVec2(x0 + wc - 42.0f * s, y));
					if (interruptor(p.id, p.on, tema->primary, s)) {
						if (p.teclado) sala.permitirTeclado(id, !p.on);
						else sala.permitirRaton(id, !p.on);
					}
					y += 34.0f * s;
				}
				y += 4.0f * s;
			}

			// --- Acciones ---
			dl->AddLine(ImVec2(x0, y), ImVec2(x0 + wc, y), vis::col(tema->text, 0.08f), 1.0f * s);
			y += 14.0f * s;
			etiqueta(dl, x0, y, T("gente.acciones"), tema, s);
			{
				const double ahora = ImGui::GetTime();
				const bool vigente = ahora < e.confirmarHasta;
				const bool confExp = vigente && e.confirmarExpulsar == id;
				const bool confBan = vigente && e.confirmarBanear == id;

				std::string tMando;
				if (f->mando > 0) tMando = T("gente.quitar_mando");
				else tMando = mandoLibre(asientos) >= 0 ? T("gente.dar_mando") : T("gente.sin_mandos");
				const char* tExp = confExp ? T("gente.seguro") : T("gente.expulsar");
				const char* tBan = confBan ? T("gente.seguro") : T("gente.banear");

				const float anchos[] = { ImGui::CalcTextSize(tMando.c_str()).x + 26.0f * s,
					ImGui::CalcTextSize(tExp).x + 26.0f * s, ImGui::CalcTextSize(tBan).x + 26.0f * s };
				float x = x0;
				auto colocar = [&](int i) {
					if (x + anchos[i] > x0 + wc && x > x0) { x = x0; y += 40.0f * s; }
					ImGui::SetCursorScreenPos(ImVec2(x, y));
					x += anchos[i] + 8.0f * s;
				};

				colocar(0);
				if (f->mando > 0) {
					if (vis::chip("##a_quitar", tMando.c_str(), tema->primary, false, s)) sala.liberarMando(f->mando - 1);
				}
				else {
					const int libre = mandoLibre(asientos);
					if (libre >= 0) {
						if (vis::chip("##a_dar", tMando.c_str(), tema->primary, false, s)) sala.asignarMando(libre, id);
					}
					else vis::chip("##a_lleno", tMando.c_str(), tema->textMuted, false, s);
				}

				colocar(1);
				if (vis::chip("##a_exp", tExp, tema->negative, confExp, s)) {
					if (confExp) { sala.expulsar(id); e.confirmarExpulsar = 0; e.seleccion = 0; }
					else { e.confirmarExpulsar = id; e.confirmarBanear = 0; e.confirmarHasta = ahora + 3.0; }
				}

				colocar(2);
				if (vis::chip("##a_ban", tBan, tema->negative, confBan, s)) {
					if (confBan) {
						if (sala.banear(id, f->nombre)) e.seleccion = 0;
						else e.avisoHasta = ahora + 3.0;
						e.confirmarBanear = 0;
					}
					else { e.confirmarBanear = id; e.confirmarExpulsar = 0; e.confirmarHasta = ahora + 3.0; }
				}
				y += 32.0f * s + 8.0f * s;

				if (ahora < e.avisoHasta) {
					dl->AddText(ImVec2(x0, y), vis::col(tema->negative), T("gente.no_baneable"));
					y += ImGui::CalcTextSize("A").y + 6.0f * s;
				}
			}
			ImGui::PopFont();

			ImGui::SetCursorScreenPos(org);
			ImGui::Dummy(ImVec2(tam.x, (std::max)(tam.y, y - org.y + pad)));
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

		std::vector<gente::Ficha> fichas;
		std::vector<AsientoVista> asientos;
		try {
			if (sala != nullptr && sala->abierta()) {
				asientos = sala->asientos(PhoenixPrefs::get().mandosActivos);
				for (const AsientoVista& a : asientos)
					if (a.ocupado) fichas.push_back({ a.jugador, a.parsecId, a.pingMs, a.numero });
				for (const EspectadorVista& ev : sala->espectadores())
					fichas.push_back({ ev.nombre, ev.parsecId, ev.pingMs, 0 });
			}
		}
		catch (...) {
			fichas.clear();
		}

		if (fichas.empty()) {
			gente::vacio(sala, ImGui::GetWindowPos(), ImGui::GetWindowSize(), tema, s);
		}
		else {
			gente::Estado& e = gente::estado();

			// La selección debe seguir existiendo; si no, la primera persona.
			const gente::Ficha* elegida = nullptr;
			for (const auto& f : fichas) if (f.parsecId == e.seleccion) { elegida = &f; break; }
			if (elegida == nullptr) { elegida = &fichas.front(); e.seleccion = elegida->parsecId; }

			int jugadores = 0;
			for (const auto& f : fichas) if (f.mando > 0) jugadores++;
			ImGui::PushFont(AppFonts::label);
			ImGui::Text("%d %s", static_cast<int>(fichas.size()), T("gente.en_sala_n"));
			ImGui::SameLine(0, 18.0f * s);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->primary);
			ImGui::Text("%d %s", jugadores, T("gente.jugadores"));
			ImGui::PopStyleColor();
			ImGui::SameLine(0, 18.0f * s);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->secondary);
			ImGui::Text("%d %s", static_cast<int>(fichas.size()) - jugadores, T("gente.espectadores"));
			ImGui::PopStyleColor();
			ImGui::PopFont();

			const ImVec2 disp = ImGui::GetContentRegionAvail();
			const float sep = 12.0f * s;
			const bool ancha = disp.x >= 760.0f * s;
			const float anchoLista = ancha ? disp.x * 0.56f - sep * 0.5f : disp.x;
			const float anchoDet = ancha ? disp.x - anchoLista - sep : disp.x;
			const float altoLista = ancha ? disp.y : disp.y * 0.45f;
			const float altoDet = ancha ? disp.y : disp.y - altoLista - sep;

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

			// ---- Lista con buscador ----
			ImGui::BeginChild("##g_lista", ImVec2(anchoLista, altoLista), false, ImGuiWindowFlags_NoBackground);
			try {
				ImGui::PushFont(AppFonts::input);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, tema->listItemBackground);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->text);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f * s);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f * s, 9.0f * s));
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##g_filtro", T("gente.filtrar"), e.filtro, sizeof(e.filtro));
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);
				ImGui::PopFont();

				const std::string q = gente::minusculas(e.filtro);
				const float anchoFila = ImGui::GetContentRegionAvail().x;
				int mostradas = 0;
				for (const auto& f : fichas) {
					if (!q.empty() && gente::minusculas(f.nombre).find(q) == std::string::npos &&
						std::to_string(f.parsecId).find(q) == std::string::npos) continue;
					mostradas++;
					if (gente::fila(*sala, f, f.parsecId == e.seleccion, anchoFila, tema, s)) e.seleccion = f.parsecId;
				}
				if (mostradas == 0) {
					ImGui::PushFont(AppFonts::input);
					ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
					ImGui::TextUnformatted(T("gente.sin_resultados"));
					ImGui::PopStyleColor();
					ImGui::PopFont();
				}
			}
			catch (...) {
			}
			ImGui::EndChild();

			if (ancha) ImGui::SameLine(0, sep);

			// ---- Ficha del invitado ----
			ImGui::BeginChild("##g_det", ImVec2(anchoDet, altoDet), false, ImGuiWindowFlags_NoBackground);
			try {
				gente::detalle(*sala, elegida, asientos, ImVec2(anchoDet, altoDet), tema, s);
			}
			catch (...) {
			}
			ImGui::EndChild();

			ImGui::PopStyleColor();
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
	}

}
