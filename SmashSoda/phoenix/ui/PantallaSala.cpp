#include "PantallaSala.h"

#include <cfloat>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "UiComun.h"
#include "../I18n.h"
#include "../PhoenixPrefs.h"
#include "../PhoenixRoles.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"
#include "../../core/Config.h"
#include <functional>
#include "TableroMandos.h"
#include <cstring>
#include "../../globals/AppFonts.h"
#include "../../services/ThemeController.h"

namespace phoenix {

	namespace {

		struct EstadoSala {
			bool avanzado = false;            // mostrando la configuración original
			double copiadoEn = -10.0;         // feedback «¡Copiado!»
			double confirmarCerrarHasta = -1; // cerrar en dos toques
			std::string error;
			double errorEn = -10.0;
			float hoverBoton = 0.0f;
			float vivo = 0.0f;                // 0 = cerrada, 1 = abierta (animado)
			float relleno[16] = {};           // animación de cada asiento
			std::string enlaceCache;
			double enlaceEn = -10.0;
		};

		EstadoSala& salaEstado() {
			static EstadoSala e;
			return e;
		}

		float salaEscala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}

		/// Texto con una fuente a un tamaño concreto (sin cambiar la fuente activa).
		void salaTexto(ImDrawList* dl, ImFont* fuente, float tam, ImVec2 p, ImU32 c, const char* t) {
			dl->AddText(fuente, tam, p, c, t);
		}

		ImVec2 salaMedir(ImFont* fuente, float tam, const char* t) {
			return fuente->CalcTextSizeA(tam, FLT_MAX, 0.0f, t);
		}

		/// Botón dibujado a mano. Devuelve true al hacer clic.
		bool salaBoton(const char* id, ImVec2 tam, float& hover, bool primario, const ImVec4& color,
			const char* texto, Theme* tema, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const bool clic = ImGui::InvisibleButton(id, tam);
			const bool encima = ImGui::IsItemHovered();
			const bool presionado = ImGui::IsItemActive();
			if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			hover = vis::acercar(hover, encima ? 1.0f : 0.0f, 16.0f);

			// Se hunde un poco al presionar (escala 0.97 alrededor del centro)
			const float hundir = presionado ? 0.015f : 0.0f;
			const ImVec2 a(p0.x + tam.x * hundir, p0.y + tam.y * hundir);
			const ImVec2 b(p0.x + tam.x * (1.0f - hundir), p0.y + tam.y * (1.0f - hundir));
			const float radio = tam.y * 0.5f;

			if (primario) {
				dl->AddRectFilled(ImVec2(a.x - 4 * s, a.y - 4 * s), ImVec2(b.x + 4 * s, b.y + 4 * s),
					vis::col(color, 0.18f * hover), radio + 4 * s); // halo al pasar el mouse
				dl->AddRectFilled(a, b, vis::col(vis::mezclar(color, ImVec4(1, 1, 1, 1), 0.12f * hover)), radio);
			}
			else {
				dl->AddRectFilled(a, b, vis::col(color, 0.10f + 0.10f * hover), radio);
				dl->AddRect(a, b, vis::col(color, 0.8f), radio, 0, 1.5f * s);
			}

			ImFont* f = AppFonts::title;
			const float tf = f->FontSize;
			const ImVec2 tt = salaMedir(f, tf, texto);
			salaTexto(dl, f, tf, ImVec2((a.x + b.x - tt.x) * 0.5f, (a.y + b.y - tt.y) * 0.5f),
				primario ? vis::col(tema->buttonPrimaryText) : vis::col(color), texto);
			return clic;
		}

		/// Interruptor tipo píldora. Devuelve true si cambió.
		bool salaInterruptor(const char* id, bool& valor, Theme* tema, float s) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 tam(44.0f * s, 24.0f * s);
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const bool clic = ImGui::InvisibleButton(id, tam);
			if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			if (clic) valor = !valor;

			ImGuiStorage* st = ImGui::GetStateStorage();
			const ImGuiID clave = ImGui::GetID(id);
			float pos = st->GetFloat(clave, valor ? 1.0f : 0.0f);
			pos = vis::acercar(pos, valor ? 1.0f : 0.0f, 18.0f);
			st->SetFloat(clave, pos);

			const ImVec4 fondo = vis::mezclar(tema->buttonDisable, tema->primary, pos);
			dl->AddRectFilled(p0, ImVec2(p0.x + tam.x, p0.y + tam.y), vis::col(fondo, 0.9f), tam.y * 0.5f);
			const float r = tam.y * 0.5f - 3.0f * s;
			dl->AddCircleFilled(ImVec2(p0.x + tam.y * 0.5f + (tam.x - tam.y) * pos, p0.y + tam.y * 0.5f), r,
				vis::col(ImVec4(1, 1, 1, 1)));
			return clic;
		}

		void salaTitulo(const char* texto, Theme* tema, float s) {
			ImGui::Dummy(ImVec2(0, 6.0f * s));
			ImGui::PushFont(AppFonts::label);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(texto);
			ImGui::PopStyleColor();
			ImGui::PopFont();
			ImGui::Dummy(ImVec2(0, 2.0f * s));
		}

		// ---------------------------------------------------------------------
		//  Historial de latencia (se alimenta cada frame desde tick)
		// ---------------------------------------------------------------------
		struct HistLat {
			float v[64] = {};
			double ultima = -10.0;
			float ms = 0.0f;
			int ping = -1;
			bool hayRival = false;
			std::string rival;
			int numRival = 0;
		};

		HistLat& histLat() {
			static HistLat h;
			return h;
		}

		void histActualizar(ProveedorSala& sala) {
			HistLat& h = histLat();
			h.rival.clear();
			h.ping = -1;
			h.numRival = 0;
			for (const AsientoVista& a : sala.asientos(16)) {
				if (a.ocupado) { h.rival = a.jugador; h.ping = a.pingMs; h.numRival = a.numero; break; }
			}
			h.hayRival = !h.rival.empty();
			const double ahora = ImGui::GetTime();
			if (ahora - h.ultima >= 0.5) {
				h.ultima = ahora;
				for (int i = 0; i < 63; i++) h.v[i] = h.v[i + 1];
				h.v[63] = (h.hayRival && h.ping >= 0) ? static_cast<float>(h.ping) : 0.0f;
			}
			h.ms = vis::acercar(h.ms, (h.hayRival && h.ping >= 0) ? static_cast<float>(h.ping) : 0.0f, 8.0f);
		}

		/// Dos columnas (o una sola si no cabe). Cada lado recibe su ancho.
		void salaFila(float ancho, float s, const std::function<void(float)>& izq, const std::function<void(float)>& der) {
			const ImVec2 o = ImGui::GetCursorScreenPos();
			if (ancho < 760.0f * s) {
				izq(ancho);
				der(ancho);
				return;
			}
			const float sepCol = 14.0f * s;
			const float w = (ancho - sepCol) * 0.5f;
			ImGui::SetCursorScreenPos(o);
			izq(w);
			const float y1 = ImGui::GetCursorScreenPos().y;
			ImGui::SetCursorScreenPos(ImVec2(o.x + w + sepCol, o.y));
			ImGui::Indent(w + sepCol); // ImGui devuelve el cursor al margen izquierdo tras cada elemento
			der(w);
			ImGui::Unindent(w + sepCol);
			const float y2 = ImGui::GetCursorScreenPos().y;
			ImGui::SetCursorScreenPos(ImVec2(o.x, (std::max)(y1, y2)));
		}

		/// Panel «Latencia en vivo»: gráfica con rejilla (0–150 ms).
		void salaPanelLatencia(Theme* tema, float s, float ancho) {
			const HistLat& h = histLat();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float alto = 176.0f * s;
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const ImVec2 p1(p0.x + ancho, p0.y + alto);
			dl->AddRectFilled(p0, p1, vis::col(tema->listItemBackground), 14.0f * s);
			dl->AddRect(p0, p1, vis::col(tema->panelBorder), 14.0f * s, 0, 1.0f * s);

			salaTexto(dl, AppFonts::label, AppFonts::label->FontSize, ImVec2(p0.x + 20 * s, p0.y + 16 * s),
				vis::col(tema->textMuted), T("sala.latencia_vivo"));
			const ImVec4 cw = (h.hayRival && h.ping >= 0)
				? vis::colorPing(h.ping, tema->positive, tema->negative, tema->textMuted) : tema->textMuted;
			const std::string ms = h.hayRival ? std::to_string(static_cast<int>(h.ms + 0.5f)) + " ms" : std::string("— ms");
			const ImVec2 tm = salaMedir(AppFonts::title, AppFonts::title->FontSize, ms.c_str());
			salaTexto(dl, AppFonts::title, AppFonts::title->FontSize, ImVec2(p1.x - 20 * s - tm.x, p0.y + 14 * s),
				vis::col(cw), ms.c_str());

			const float xa = p0.x + 54.0f * s, xb = p1.x - 20.0f * s;
			const float ya = p0.y + 52.0f * s, yb = p1.y - 20.0f * s;
			const float marcas[4] = { 150.0f, 100.0f, 50.0f, 0.0f };
			for (int i = 0; i < 4; i++) {
				const float y = ya + (yb - ya) * (i / 3.0f);
				dl->AddLine(ImVec2(xa, y), ImVec2(xb, y), vis::col(tema->textMuted, 0.18f), 1.0f * s);
				char t[8];
				snprintf(t, sizeof(t), "%d", static_cast<int>(marcas[i]));
				const ImVec2 tt = salaMedir(AppFonts::input, AppFonts::input->FontSize, t);
				salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(xa - 8 * s - tt.x, y - tt.y * 0.5f),
					vis::col(tema->textMuted, 0.8f), t);
			}
			const float paso = (xb - xa) / 63.0f;
			auto yDe = [&](float v) { return yb - (yb - ya) * ((std::min)(v, 150.0f) / 150.0f); };
			for (int i = 0; i < 63; i++) {
				const float xA = xa + paso * i, xB = xa + paso * (i + 1);
				dl->AddLine(ImVec2(xB, yDe(h.v[i + 1])), ImVec2(xB, yb), vis::col(cw, 0.07f), (std::max)(1.0f, paso));
				dl->AddLine(ImVec2(xA, yDe(h.v[i])), ImVec2(xB, yDe(h.v[i + 1])), vis::col(cw, 0.9f), 2.0f * s);
			}

			ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y));
			ImGui::Dummy(ImVec2(ancho, 14.0f * s));
		}

		/// Panel «Transmisión»: cuatro datos grandes.
		void salaPanelTransmision(ProveedorSala& sala, Theme* tema, float s, float ancho) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			salaTitulo(T("sala.transmision"), tema, s);
			const ImVec2 o = ImGui::GetCursorScreenPos();
			const float sepT = 8.0f * s;
			const float w = (ancho - sepT * 3) / 4.0f;
			const float alto = 70.0f * s;
			const std::string valores[4] = {
				std::to_string(Config::cfg.video.bandwidth),
				std::to_string(Config::cfg.video.fps),
				std::to_string(sala.totalInvitados()),
				std::to_string(sala.plazas()),
			};
			const char* etiquetas[4] = { "Mbps", "FPS", T("sala.invitados"), T("sala.plazas") };
			for (int i = 0; i < 4; i++) {
				const ImVec2 a(o.x + i * (w + sepT), o.y), b(a.x + w, a.y + alto);
				dl->AddRectFilled(a, b, vis::col(tema->listItemBackground), 12.0f * s);
				dl->AddRect(a, b, vis::col(tema->panelBorder), 12.0f * s, 0, 1.0f * s);
				const ImVec2 tv = salaMedir(AppFonts::title, AppFonts::title->FontSize * 1.3f, valores[i].c_str());
				salaTexto(dl, AppFonts::title, AppFonts::title->FontSize * 1.3f, ImVec2(a.x + (w - tv.x) * 0.5f, a.y + 10 * s),
					vis::col(tema->primary), valores[i].c_str());
				const ImVec2 te = salaMedir(AppFonts::input, AppFonts::input->FontSize, etiquetas[i]);
				dl->PushClipRect(a, b, true);
				salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(a.x + (w - te.x) * 0.5f, a.y + alto - te.y - 8 * s),
					vis::col(tema->textMuted), etiquetas[i]);
				dl->PopClipRect();
			}
			ImGui::SetCursorScreenPos(ImVec2(o.x, o.y + alto));
			ImGui::Dummy(ImVec2(ancho, 8.0f * s));
		}

		// ---------------------------------------------------------------------
		//  Enlace visual: anfitrión ↔ invitado, con la latencia en vivo
		// ---------------------------------------------------------------------
		std::string salaIniciales(const std::string& nombre) {
			std::string r;
			for (char c : nombre) {
				const unsigned char u = static_cast<unsigned char>(c);
				if (u < 128 && std::isalnum(u)) {
					r += static_cast<char>(std::toupper(u));
					if (r.size() == 2) break;
				}
			}
			if (r.empty()) r = "+";
			return r;
		}

		/// Un lado del enlace: avatar + etiqueta + nombre + detalle.
		void salaAsiento(ImDrawList* dl, ImVec2 p0, float w, float yc, bool derecha, const char* etiqueta,
			const std::string& nombre, const std::string& detalle, bool vacio, const ImVec4& color,
			Theme* tema, float s) {
			const float av = 54.0f * s;
			const float xAv = derecha ? p0.x + w - av * 0.5f : p0.x + av * 0.5f;
			const ImVec2 c(xAv, yc);
			const ImVec4 cc = vacio ? tema->textMuted : color;

			dl->AddCircleFilled(c, av * 0.5f, vis::col(cc, vacio ? 0.08f : 0.18f), 48);
			dl->AddCircle(c, av * 0.5f, vis::col(cc, vacio ? 0.40f : 0.90f), 48, 2.0f * s);
			const std::string ini = vacio ? std::string("+") : salaIniciales(nombre);
			const ImVec2 ti = salaMedir(AppFonts::title, AppFonts::title->FontSize, ini.c_str());
			salaTexto(dl, AppFonts::title, AppFonts::title->FontSize, ImVec2(c.x - ti.x * 0.5f, c.y - ti.y * 0.5f),
				vis::col(cc), ini.c_str());

			const float xIzq = derecha ? p0.x : p0.x + av + 12.0f * s;
			const float xDer = derecha ? p0.x + w - av - 12.0f * s : p0.x + w;
			dl->PushClipRect(ImVec2(xIzq, yc - 40.0f * s), ImVec2(xDer, yc + 40.0f * s), true);
			struct Linea { ImFont* f; float tam; float dy; ImU32 color; const char* txt; };
			const Linea lineas[3] = {
				{ AppFonts::label, AppFonts::label->FontSize, -32.0f, vis::col(vacio ? tema->textMuted : tema->primary), etiqueta },
				{ AppFonts::title, AppFonts::title->FontSize, -14.0f, vis::col(vacio ? tema->textMuted : tema->text), nombre.c_str() },
				{ AppFonts::input, AppFonts::input->FontSize, 12.0f, vis::col(tema->textMuted), detalle.c_str() },
			};
			for (const Linea& l : lineas) {
				const ImVec2 tt = salaMedir(l.f, l.tam, l.txt);
				const float x = derecha ? (std::max)(xIzq, xDer - tt.x) : xIzq;
				salaTexto(dl, l.f, l.tam, ImVec2(x, yc + l.dy * s), l.color, l.txt);
			}
			dl->PopClipRect();
		}

		void salaEnlaceVisual(ProveedorSala& sala, Theme* tema, float s, float ancho) {
			const HistLat& H = histLat();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const double ahora = ImGui::GetTime();

			std::string anfitrion = sala.cuentaHost();
			if (anfitrion.empty()) anfitrion = "Host";
			std::string rival;
			int ping = -1, numRival = 0;
			for (const AsientoVista& a : sala.asientos(16)) {
				if (a.ocupado) { rival = a.jugador; ping = a.pingMs; numRival = a.numero; break; }
			}
			const bool hayRival = !rival.empty();
			const bool abierta = sala.abierta();

			const float alto = 132.0f * s;
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const ImVec2 p1(p0.x + ancho, p0.y + alto);
			const float yc = p0.y + alto * 0.5f;
			dl->AddRectFilled(p0, p1, vis::col(tema->listItemBackground), 14.0f * s);
			dl->AddRect(p0, p1, vis::col(hayRival ? vis::mezclar(tema->panelBorder, tema->primary, 0.45f) : tema->panelBorder),
				14.0f * s, 0, 1.0f * s);

			const float margen = 20.0f * s;
			const float wAsiento = (std::max)(150.0f * s, ancho * 0.26f);
			salaAsiento(dl, ImVec2(p0.x + margen, p0.y), wAsiento, yc, false, T("sala.host"), anfitrion,
				T("sala.host_detalle"), false, tema->primary, tema, s);

			std::string detalleRival = T("sala.mando_n");
			const size_t posN = detalleRival.find("%d");
			if (posN != std::string::npos) detalleRival.replace(posN, 2, std::to_string(numRival));
			salaAsiento(dl, ImVec2(p1.x - margen - wAsiento, p0.y), wAsiento, yc, true, T("sala.invitado"),
				hayRival ? rival : std::string(abierta ? T("sala.esperando") : T("sala.sin_rival")),
				hayRival ? detalleRival : std::string(T(abierta ? "sala.comparte_enlace" : "sala.abre_sala")),
				!hayRival, tema->secondary, tema, s);

			// El cable
			const float x0 = p0.x + margen + wAsiento + 16.0f * s;
			const float x1 = p1.x - margen - wAsiento - 16.0f * s;
			if (x1 - x0 > 90.0f * s) {
				const float cx = (x0 + x1) * 0.5f;
				auto centrado = [&](ImFont* f, float tam, float y, ImU32 c, const char* t) {
					const ImVec2 tt = salaMedir(f, tam, t);
					salaTexto(dl, f, tam, ImVec2(cx - tt.x * 0.5f, y), c, t);
				};
				const float yCable = p0.y + 92.0f * s;

				if (hayRival) {
					const ImVec4 cw = ping >= 0 ? vis::colorPing(ping, tema->positive, tema->negative, tema->textMuted) : tema->primary;
					const char* calidad = ping < 0 ? T("sala.midiendo")
						: (ping < 60 ? T("sala.enlace_estable") : (ping < 120 ? T("sala.enlace_regular") : T("sala.enlace_inestable")));
					centrado(AppFonts::label, AppFonts::label->FontSize, p0.y + 16.0f * s, vis::col(cw), calidad);

					// Gráfica de latencia (0–150 ms)
					const float amp = 38.0f * s;
					const float yTope = p0.y + 40.0f * s;
					const float paso = (x1 - x0) / 63.0f;
					for (int i = 0; i < 63; i++) {
						const float va = (std::min)(H.v[i], 150.0f) / 150.0f;
						const float vb = (std::min)(H.v[i + 1], 150.0f) / 150.0f;
						dl->AddLine(ImVec2(x0 + paso * i, yTope + amp * (1.0f - va)),
							ImVec2(x0 + paso * (i + 1), yTope + amp * (1.0f - vb)), vis::col(cw, 0.35f + 0.65f * (i / 63.0f)), 2.0f * s);
					}

					// Cable con paquetes que viajan
					dl->AddLine(ImVec2(x0, yCable), ImVec2(x1, yCable), vis::col(cw, 0.25f), 2.0f * s);
					for (int k = 0; k < 4; k++) {
						const float t = std::fmod(static_cast<float>(ahora) * 0.7f + k * 0.25f, 1.0f);
						const float sentido = (k % 2 == 0) ? t : 1.0f - t;
						dl->AddCircleFilled(ImVec2(x0 + (x1 - x0) * sentido, yCable), 3.2f * s, vis::col(cw, 0.9f), 16);
					}

					const std::string ms = std::to_string(static_cast<int>(H.ms + 0.5f)) + " ms";
					centrado(AppFonts::title, AppFonts::title->FontSize, yCable + 10.0f * s, vis::col(tema->text), ms.c_str());
				}
				else {
					centrado(AppFonts::label, AppFonts::label->FontSize, yc - 22.0f * s, vis::col(tema->textMuted),
						abierta ? T("sala.esperando") : T("sala.sin_rival"));
					for (float x = x0; x < x1 - 8.0f * s; x += 14.0f * s) {
						dl->AddLine(ImVec2(x, yc), ImVec2(x + 7.0f * s, yc), vis::col(tema->textMuted, 0.45f), 2.0f * s);
					}
				}
			}

			ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y));
			ImGui::Dummy(ImVec2(ancho, 14.0f * s));
		}

		// ---------------------------------------------------------------------
		//  Tarjeta principal: estado + botón + enlace
		// ---------------------------------------------------------------------
		void salaTarjetaPrincipal(ProveedorSala& sala, Theme* tema, float s, float ancho) {
			EstadoSala& e = salaEstado();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const bool abierta = sala.abierta();
			const double ahora = ImGui::GetTime();
			e.vivo = vis::acercar(e.vivo, abierta ? 1.0f : 0.0f, 6.0f);

			const float alto = 176.0f * s;
			const ImVec2 p0 = ImGui::GetCursorScreenPos();
			const ImVec2 p1(p0.x + ancho, p0.y + alto);

			// Fondo: se tiñe de cian cuando la sala está abierta
			const ImVec4 base = tema->listItemBackground;
			dl->AddRectFilled(p0, p1, vis::col(vis::mezclar(base, tema->primary, 0.07f * e.vivo)), 14.0f * s);
			dl->AddRect(p0, p1, vis::col(vis::mezclar(tema->panelBorder, tema->primary, 0.6f * e.vivo)),
				14.0f * s, 0, 1.0f * s);
			dl->AddRectFilled(ImVec2(p0.x, p0.y + 18 * s), ImVec2(p0.x + 4 * s, p1.y - 18 * s),
				vis::col(vis::mezclar(tema->textMuted, tema->primary, e.vivo)), 2 * s);

			// Halo que respira cuando está en vivo
			if (e.vivo > 0.02f) {
				const float resp = 0.5f + 0.5f * std::sin(static_cast<float>(ahora) * 2.2f);
				const ImVec2 c(p1.x - 90 * s, p0.y + alto * 0.5f);
				for (int i = 3; i >= 1; i--) {
					dl->AddCircleFilled(c, (26.0f + 18.0f * i + 6.0f * resp) * s,
						vis::col(tema->primary, 0.035f * e.vivo * (4 - i)), 48);
				}
			}

			// Estado + nombre
			const char* estado = abierta ? T("sala.abierta_titulo") : T("sala.cerrada_titulo");
			salaTexto(dl, AppFonts::label, AppFonts::label->FontSize, ImVec2(p0.x + 24 * s, p0.y + 20 * s),
				vis::col(abierta ? tema->primary : tema->textMuted), estado);

			std::string nombre = sala.nombreSala();
			if (nombre.empty()) nombre = "Phoenix";
			salaTexto(dl, AppFonts::title, AppFonts::title->FontSize * 1.35f, ImVec2(p0.x + 24 * s, p0.y + 46 * s),
				vis::col(tema->text), nombre.c_str());

			const std::string resumen = std::to_string(sala.totalInvitados()) + " / " + std::to_string(sala.plazas())
				+ "  " + T("sala.invitados");
			salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(p0.x + 24 * s, p0.y + 86 * s),
				vis::col(tema->textMuted), resumen.c_str());

			// Botón principal
			ImGui::SetCursorScreenPos(ImVec2(p0.x + 24 * s, p0.y + alto - 70 * s));
			if (!abierta) {
				if (salaBoton("##abrir", ImVec2(200 * s, 48 * s), e.hoverBoton, true, tema->primary, T("sala.abrir"), tema, s)) {
					std::string error;
					if (!sala.abrir(error)) {
						e.error = error;
						e.errorEn = ahora;
					}
				}
			}
			else {
				const bool confirmando = ahora < e.confirmarCerrarHasta;
				if (salaBoton("##cerrar", ImVec2(200 * s, 48 * s), e.hoverBoton, false, tema->negative,
					confirmando ? T("sala.confirmar_cerrar") : T("sala.cerrar"), tema, s)) {
					if (confirmando) {
						sala.cerrar();
						e.confirmarCerrarHasta = -1;
					}
					else {
						e.confirmarCerrarHasta = ahora + 3.0;
					}
				}
			}

			// Enlace con copiar (a la derecha del botón)
			if (ahora - e.enlaceEn > 1.0) { // se refresca una vez por segundo
				e.enlaceCache = abierta ? sala.enlace() : std::string();
				e.enlaceEn = ahora;
			}
			const float xEnlace = p0.x + 24 * s + 200 * s + 20 * s;
			const float anchoEnlace = (std::max)(120.0f * s, ancho - (xEnlace - p0.x) - 24 * s);
			const float yEnlace = p0.y + alto - 70 * s;
			const ImVec2 e0(xEnlace, yEnlace), e1(xEnlace + anchoEnlace, yEnlace + 48 * s);
			dl->AddRectFilled(e0, e1, vis::col(tema->formInputBackground), 12 * s);
			const char* textoEnlace = e.enlaceCache.empty() ? T("sala.sin_enlace") : e.enlaceCache.c_str();
			dl->PushClipRect(ImVec2(e0.x + 14 * s, e0.y), ImVec2(e1.x - (e.enlaceCache.empty() ? 14 : 110) * s, e1.y), true);
			salaTexto(dl, AppFonts::input, AppFonts::input->FontSize,
				ImVec2(e0.x + 14 * s, e0.y + (48 * s - AppFonts::input->FontSize) * 0.5f),
				vis::col(e.enlaceCache.empty() ? tema->textMuted : tema->text), textoEnlace);
			dl->PopClipRect();

			if (!e.enlaceCache.empty()) {
				const bool copiado = ahora - e.copiadoEn < 1.6;
				ImGui::SetCursorScreenPos(ImVec2(e1.x - 102 * s, e0.y + 8 * s));
				static float hoverCopiar = 0.0f;
				if (salaBoton("##copiar", ImVec2(94 * s, 32 * s), hoverCopiar, !copiado,
					copiado ? tema->positive : tema->secondary,
					copiado ? T("sala.copiado") : T("sala.copiar"), tema, s)) {
					ImGui::SetClipboardText(e.enlaceCache.c_str());
					e.copiadoEn = ahora;
				}
			}

			// Error de apertura (se desvanece a los 6 s)
			const double edadError = ahora - e.errorEn;
			if (!e.error.empty() && edadError < 6.0) {
				const float a = static_cast<float>((std::min)(1.0, (6.0 - edadError) / 1.0));
				salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(p0.x + 24 * s, p1.y - 18 * s),
					vis::col(tema->negative, a), phoenix::Tr(e.error).c_str());
			}

			ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y));
			ImGui::Dummy(ImVec2(ancho, 14 * s));
		}

		// ---------------------------------------------------------------------
		//  Conexión con la web (PhoenixLink)
		// ---------------------------------------------------------------------
		void salaConexionWeb(Theme* tema, float s, float ancho) {
			PhoenixLink& link = PhoenixLink::instancia();
			const EstadoLink est = link.estado();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImGui::Dummy(ImVec2(0, 10 * s));

			ImVec4 color = tema->textMuted;
			std::string titulo;
			switch (est) {
			case EstadoLink::Conectado:   color = tema->positive; titulo = std::string(T("web.conectado")) + " " + link.usuario(); break;
			case EstadoLink::Vinculando:  color = tema->primary;  titulo = T("web.vinculando"); break;
			case EstadoLink::SinConexion: color = ImVec4(0.96f, 0.71f, 0.27f, 1.0f); titulo = T("web.sin_conexion"); break;
			case EstadoLink::Pausado:     color = tema->negative; titulo = T("web.pausado"); break;
			default:                      titulo = T("web.sin_vincular"); break;
			}

			const ImVec2 p = ImGui::GetCursorScreenPos();
			const float pulso = 0.6f + 0.4f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.0f);
			dl->AddCircleFilled(ImVec2(p.x + 6 * s, p.y + 11 * s), 5 * s, vis::col(color, est == EstadoLink::Conectado ? 1.0f : pulso));
			ImGui::SetCursorScreenPos(ImVec2(p.x + 20 * s, p.y));
			ImGui::PushFont(AppFonts::label);
			ImGui::TextColored(color, "%s", titulo.c_str());
			ImGui::PopFont();

			const std::string msg = link.mensaje();
			if (!msg.empty()) {
				ImGui::PushFont(AppFonts::input);
				ImGui::TextColored(tema->textMuted, "%s", msg.c_str());
				ImGui::PopFont();
			}

			ImGui::PushFont(AppFonts::input);
			if (est == EstadoLink::SinVincular) {
				static char codigo[16] = {};
				ImGui::TextColored(tema->textMuted, "%s", T("web.ayuda_codigo"));
				ImGui::SetNextItemWidth(160 * s);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, tema->formInputBackground);
				ImGui::InputTextWithHint("##codigo", "123456", codigo, sizeof(codigo), ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopStyleColor();
				ImGui::SameLine(0, 10 * s);
				static float hoverVincular = 0.0f;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4 * s);
				if (salaBoton("##vincular", ImVec2(140 * s, 36 * s), hoverVincular, true, tema->primary, T("web.vincular"), tema, s)
					&& strlen(codigo) == 6) {
					link.emparejar(codigo);
					codigo[0] = 0;
				}
			}
			else if (est == EstadoLink::Pausado) {
				static float hoverReintentar = 0.0f;
				if (salaBoton("##reintentar", ImVec2(160 * s, 34 * s), hoverReintentar, false, tema->primary, T("web.reintentar"), tema, s)) {
					link.reintentar();
				}
			}
			else if (est == EstadoLink::Conectado) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(tema->textMuted.x, tema->textMuted.y, tema->textMuted.z, 0.7f));
				if (ImGui::SmallButton(T("web.desvincular"))) link.desvincular();
				ImGui::PopStyleColor();
			}
			ImGui::PopFont();
		}

		/// Envía a PhoenixLink una foto de la sala (≈ 1 vez por segundo).
		void salaAlimentarLink(ProveedorSala& sala) {
			static double ultima = -10.0;
			const double ahora = ImGui::GetTime();
			if (ahora - ultima < 1.0) return;
			ultima = ahora;

			InstantaneaSala foto;
			foto.abierta = sala.abierta();
			if (foto.abierta && salaEstado().enlaceCache.empty()) salaEstado().enlaceCache = sala.enlace();
			foto.enlace = foto.abierta ? salaEstado().enlaceCache : std::string();
			foto.plazasTotal = sala.plazas();
			foto.plazasLibres = (std::max)(0, foto.plazasTotal - sala.totalInvitados());
			for (const AsientoVista& a : sala.asientos(16)) {
				if (a.ocupado && a.parsecId != 0) foto.invitados.push_back({ std::to_string(a.parsecId), a.jugador, a.pingMs });
			}
			for (const EspectadorVista& e : sala.espectadores()) {
				foto.invitados.push_back({ std::to_string(e.parsecId), e.nombre, e.pingMs });
			}
			sala.aplicarAsientosReservados();
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			foto.visibilidad = pr.visibilidad;
			foto.juego = pr.juego;
			foto.parche = pr.parche;
			foto.region = pr.region;
			foto.aceptaEspectadores = pr.espectadores;
			foto.limiteEspectadores = pr.limiteEspectadores;
			PhoenixLink::instancia().actualizar(foto);
		}

		// ---------------------------------------------------------------------
		//  Visibilidad y espectadores
		// ---------------------------------------------------------------------
		void salaOpciones(Theme* tema, float s, float ancho) {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			bool cambio = false;

			salaTitulo(T("sala.visibilidad"), tema, s);

			struct Opcion { const char* valor; const char* clave; const char* ayuda; };
			const Opcion opciones[3] = {
				{ "publica", "sala.publica", "sala.vis_ayuda_publica" },
				{ "amigos",  "sala.amigos",  "sala.vis_ayuda_amigos" },
				{ "privada", "sala.privada", "sala.vis_ayuda_privada" },
			};

			const float anchoPildora = (std::min)(150.0f * s, (ancho - 16 * s) / 3.0f);
			const float altoPildora = 40.0f * s;
			const ImVec2 inicio = ImGui::GetCursorScreenPos();
			const ImVec2 finFondo(inicio.x + anchoPildora * 3 + 8 * s, inicio.y + altoPildora + 8 * s);
			dl->AddRectFilled(inicio, finFondo, vis::col(tema->formInputBackground), (altoPildora + 8 * s) * 0.5f);

			// Selector que se desliza bajo la opción elegida
			int elegida = 1;
			for (int i = 0; i < 3; i++) if (pr.visibilidad == opciones[i].valor) elegida = i;
			ImGuiStorage* st = ImGui::GetStateStorage();
			const ImGuiID clave = ImGui::GetID("##vis_selector");
			float x = st->GetFloat(clave, static_cast<float>(elegida));
			x = vis::acercar(x, static_cast<float>(elegida), 16.0f);
			st->SetFloat(clave, x);
			const ImVec2 s0(inicio.x + 4 * s + x * anchoPildora, inicio.y + 4 * s);
			dl->AddRectFilled(s0, ImVec2(s0.x + anchoPildora, s0.y + altoPildora), vis::col(tema->secondary), altoPildora * 0.5f);

			for (int i = 0; i < 3; i++) {
				ImGui::SetCursorScreenPos(ImVec2(inicio.x + 4 * s + i * anchoPildora, inicio.y + 4 * s));
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##vis", ImVec2(anchoPildora, altoPildora))) {
					if (pr.visibilidad != opciones[i].valor) {
						pr.visibilidad = opciones[i].valor;
						cambio = true;
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::PopID();
				const char* t = T(opciones[i].clave);
				const ImVec2 tt = salaMedir(AppFonts::label, AppFonts::label->FontSize, t);
				salaTexto(dl, AppFonts::label, AppFonts::label->FontSize,
					ImVec2(inicio.x + 4 * s + i * anchoPildora + (anchoPildora - tt.x) * 0.5f,
						inicio.y + 4 * s + (altoPildora - tt.y) * 0.5f),
					vis::col(i == elegida ? ImVec4(1, 1, 1, 1) : tema->textMuted), t);
			}
			ImGui::SetCursorScreenPos(ImVec2(inicio.x, finFondo.y + 6 * s));
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(T(opciones[elegida].ayuda));
			ImGui::PopStyleColor();
			ImGui::PopFont();

			// Espectadores: interruptor + límite
			ImGui::Dummy(ImVec2(0, 8 * s));
			if (salaInterruptor("##esp", pr.espectadores, tema, s)) cambio = true;
			ImGui::SameLine(0, 12 * s);
			ImGui::PushFont(AppFonts::label);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(T("sala.permitir_esp"));
			ImGui::PopFont();
			if (pr.espectadores) {
				ImGui::SameLine(0, 24 * s);
				ImGui::PushFont(AppFonts::input);
				ImGui::SetNextItemWidth((std::min)(220.0f * s, ancho * 0.4f));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, tema->formInputBackground);
				ImGui::PushStyleColor(ImGuiCol_SliderGrab, tema->primary);
				ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, tema->primary);
				std::string formato = std::string(T("sala.limite")) + ": %d";
				if (ImGui::SliderInt("##limite", &pr.limiteEspectadores, 1, 16, formato.c_str())) cambio = true;
				ImGui::PopStyleColor(3);
				ImGui::PopFont();
			}

			// Entrada directa por Parsec (quien no está en la lista: el host decide)
			ImGui::Dummy(ImVec2(0, 4 * s));
			if (salaInterruptor("##parsec", pr.entradaParsec, tema, s)) cambio = true;
			ImGui::SameLine(0, 12 * s);
			ImGui::PushFont(AppFonts::label);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(T("sala.entrada_parsec"));
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextUnformatted(T(pr.entradaParsec ? "sala.entrada_parsec_on" : "sala.entrada_parsec_off"));
			ImGui::PopStyleColor();
			ImGui::PopFont();

			// Juego, parche y región: se muestran en las tarjetas de la web
			ImGui::Dummy(ImVec2(0, 8 * s));
			ImGui::PushFont(AppFonts::input);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, tema->formInputBackground);
			const float col3 = (ancho - 20 * s) / 3.0f;
			static const char* juegos[] = { "eFootball PES 2021", "Football Life 2026", "Football Life 2027" };
			ImGui::SetNextItemWidth(col3);
			if (ImGui::BeginCombo("##juego", pr.juego.c_str())) {
				for (const char* jn : juegos) {
					if (ImGui::Selectable(jn, pr.juego == jn)) { pr.juego = jn; cambio = true; }
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine(0, 10 * s);
			static char parche[64] = {}, region[48] = {};
			static bool listos = false;
			if (!listos) {
				strncpy_s(parche, pr.parche.c_str(), _TRUNCATE);
				strncpy_s(region, pr.region.c_str(), _TRUNCATE);
				listos = true;
			}
			ImGui::SetNextItemWidth(col3);
			if (ImGui::InputTextWithHint("##parche", T("sala.parche"), parche, sizeof(parche))) { pr.parche = parche; cambio = true; }
			ImGui::SameLine(0, 10 * s);
			ImGui::SetNextItemWidth(col3);
			if (ImGui::InputTextWithHint("##region", T("sala.region"), region, sizeof(region))) { pr.region = region; cambio = true; }
			ImGui::PopStyleColor();
			ImGui::PopFont();

			salaConexionWeb(tema, s, ancho);

			if (cambio) pr.guardar();
		}

		// ---------------------------------------------------------------------
		//  Asientos (mandos)
		// ---------------------------------------------------------------------
		void salaAsientos(ProveedorSala& sala, Theme* tema, float s, float ancho) {
			EstadoSala& e = salaEstado();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			salaTitulo(T("sala.mandos"), tema, s);

			std::vector<AsientoVista> asientos = sala.asientos(16);
			int visibles = (std::min)(4, static_cast<int>(asientos.size()));
			for (int i = 0; i < static_cast<int>(asientos.size()); i++) {
				if (asientos[i].ocupado) visibles = (std::max)(visibles, i + 1);
			}
			if (visibles == 0) {
				ImGui::PushFont(AppFonts::input);
				ImGui::TextColored(tema->textMuted, "%s", T("sala.sin_mando"));
				ImGui::PopFont();
				return;
			}

			const float sep = 12.0f * s;
			const int columnas = (std::max)(1, static_cast<int>((ancho + sep) / (230.0f * s + sep)));
			const float anchoTarjeta = (ancho - sep * (columnas - 1)) / columnas;
			const float altoTarjeta = 92.0f * s;
			const ImVec2 origen = ImGui::GetCursorScreenPos();

			for (int i = 0; i < visibles; i++) {
				const AsientoVista& a = asientos[i];
				const int fila = i / columnas, colu = i % columnas;
				const ImVec2 p0(origen.x + colu * (anchoTarjeta + sep), origen.y + fila * (altoTarjeta + sep));
				const ImVec2 p1(p0.x + anchoTarjeta, p0.y + altoTarjeta);
				e.relleno[i] = vis::acercar(e.relleno[i], a.ocupado ? 1.0f : 0.0f, 10.0f);
				const float r = e.relleno[i];

				dl->AddRectFilled(p0, p1, vis::col(vis::mezclar(tema->formInputBackground, tema->listItemBackground, r)), 12 * s);
				if (r < 0.98f) {
					// Asiento libre: borde punteado
					const ImU32 c = vis::col(tema->textMuted, 0.35f * (1.0f - r));
					for (float x = p0.x + 12 * s; x < p1.x - 12 * s; x += 10 * s) {
						dl->AddLine(ImVec2(x, p0.y), ImVec2(x + 5 * s, p0.y), c, 1.0f * s);
						dl->AddLine(ImVec2(x, p1.y), ImVec2(x + 5 * s, p1.y), c, 1.0f * s);
					}
				}
				if (r > 0.02f) {
					dl->AddRect(p0, p1, vis::col(tema->secondary, 0.55f * r), 12 * s, 0, 1.0f * s);
					dl->AddRectFilled(ImVec2(p0.x, p0.y + 14 * s), ImVec2(p0.x + 4 * s, p1.y - 14 * s),
						vis::col(tema->primary, r), 2 * s);
				}

				char etiqueta[32];
				snprintf(etiqueta, sizeof(etiqueta), "%s %d", T("sala.mando"), a.numero);
				salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(p0.x + 18 * s, p0.y + 14 * s),
					vis::col(tema->textMuted), etiqueta);

				const char* nombre = a.ocupado ? a.jugador.c_str() : (a.conectado ? T("sala.libre") : T("sala.sin_mando"));
				dl->PushClipRect(p0, ImVec2(p1.x - 80 * s, p1.y), true);
				salaTexto(dl, AppFonts::title, AppFonts::title->FontSize, ImVec2(p0.x + 18 * s, p0.y + 44 * s),
					vis::col(a.ocupado ? tema->text : tema->textMuted, a.ocupado ? 1.0f : 0.7f), nombre);
				dl->PopClipRect();

				if (a.ocupado) {
					const ImVec4 cp = vis::colorPing(a.pingMs, tema->positive, tema->negative, tema->textMuted);
					const std::string ping = a.pingMs >= 0 ? std::to_string(a.pingMs) + " ms" : "— ms";
					const ImVec2 tt = salaMedir(AppFonts::input, AppFonts::input->FontSize, ping.c_str());
					const ImVec2 c0(p1.x - tt.x - 34 * s, p0.y + 14 * s);
					dl->AddRectFilled(c0, ImVec2(p1.x - 12 * s, c0.y + 26 * s), vis::col(cp, 0.15f), 13 * s);
					dl->AddCircleFilled(ImVec2(c0.x + 10 * s, c0.y + 13 * s), 4 * s, vis::col(cp));
					salaTexto(dl, AppFonts::input, AppFonts::input->FontSize,
						ImVec2(c0.x + 18 * s, c0.y + (26 * s - tt.y) * 0.5f), vis::col(cp), ping.c_str());
				}
			}

			const int filas = (visibles + columnas - 1) / columnas;
			ImGui::SetCursorScreenPos(ImVec2(origen.x, origen.y + filas * (altoTarjeta + sep)));
			ImGui::Dummy(ImVec2(ancho, 4 * s));
		}

		// ---------------------------------------------------------------------
		//  Espectadores (fichas)
		// ---------------------------------------------------------------------
		void salaEspectadores(ProveedorSala& sala, Theme* tema, float s, float ancho) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const std::vector<EspectadorVista> lista = sala.espectadores();
			const std::string titulo = std::string(T("sala.espectadores")) + "  ·  " + std::to_string(lista.size());
			salaTitulo(titulo.c_str(), tema, s);

			if (lista.empty()) {
				ImGui::PushFont(AppFonts::input);
				ImGui::TextColored(tema->textMuted, "%s", T("sala.sin_espectadores"));
				ImGui::PopFont();
				return;
			}

			const ImVec2 origen = ImGui::GetCursorScreenPos();
			float x = origen.x, y = origen.y;
			const float alto = 34.0f * s;
			for (const EspectadorVista& esp : lista) {
				const ImVec2 tt = salaMedir(AppFonts::input, AppFonts::input->FontSize, esp.nombre.c_str());
				const float w = tt.x + 40 * s;
				if (x + w > origen.x + ancho && x > origen.x) {
					x = origen.x;
					y += alto + 8 * s;
				}
				const ImVec4 cp = vis::colorPing(esp.pingMs, tema->positive, tema->negative, tema->textMuted);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + alto), vis::col(tema->listItemBackground), alto * 0.5f);
				dl->AddCircleFilled(ImVec2(x + 16 * s, y + alto * 0.5f), 4 * s, vis::col(cp));
				salaTexto(dl, AppFonts::input, AppFonts::input->FontSize, ImVec2(x + 28 * s, y + (alto - tt.y) * 0.5f),
					vis::col(tema->text), esp.nombre.c_str());
				x += w + 8 * s;
			}
			ImGui::SetCursorScreenPos(ImVec2(origen.x, y + alto + 8 * s));
			ImGui::Dummy(ImVec2(ancho, 1));
		}
	}

	// =========================================================================
	void PantallaSala::tick(ProveedorSala* sala) {
		if (sala == nullptr) return;
		PhoenixRoles::instancia().permitirParsec(PhoenixPrefs::get().entradaParsec);
		try { salaAlimentarLink(*sala); } catch (...) {}
		try { histActualizar(*sala); } catch (...) {}
	}

	void PantallaSala::render(ProveedorSala* sala,
		const std::function<void()>& panelActividad,
		const std::function<void()>& panelAvanzado,
		const Acoplador& acoplar,
		ImVec2 pos, ImVec2 tam, float alfa, int pestana) {

		EstadoSala& e = salaEstado();
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = salaEscala();
		
		// Pestañas: 0 = Resumen · 1 = Opciones · 2 = Actividad (registro)
		const ImVec2 tamPrincipal = tam;

		if (sala == nullptr) return; // forma sin modo host: el shell muestra otra pantalla

		if (pestana == 2) {
			acoplar(panelActividad, pos, tam);
			return;
		}

		if (e.avanzado && pestana == 1) {
			// Configuración original del Soda, con un botón para volver
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(tamPrincipal.x, 44 * s), ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
			ImGui::Begin("##phx_sala_volver", nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			static float hoverVolver = 0.0f;
			if (salaBoton("##volver", ImVec2(220 * s, 36 * s), hoverVolver, false, tema->primary, T("sala.volver"), tema, s)) {
				e.avanzado = false;
			}
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(2);

			acoplar(panelAvanzado, ImVec2(pos.x, pos.y + 52 * s), ImVec2(tamPrincipal.x, tamPrincipal.y - 52 * s));
		}
		else {
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(tamPrincipal, ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * s, 20 * s));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
			ImGui::PushStyleColor(ImGuiCol_Border, tema->panelBorder);
			ImGui::Begin("##phx_sala", nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

			const float anchoUtil = ImGui::GetContentRegionAvail().x;
			try {
				if (pestana == 0) {
					salaEnlaceVisual(*sala, tema, s, anchoUtil);
					salaFila(anchoUtil, s,
						[&](float w) { salaTarjetaPrincipal(*sala, tema, s, w); },
						[&](float w) { salaPanelLatencia(tema, s, w); });
					salaFila(anchoUtil, s,
						[&](float w) { salaPanelTransmision(*sala, tema, s, w); },
						[&](float w) { salaEspectadores(*sala, tema, s, w); });
					salaTitulo(T("sala.mandos"), tema, s);
					TableroMandos::render(*sala, anchoUtil, true);
				}
				else {
					salaOpciones(tema, s, (std::min)(anchoUtil, 820.0f * s));

					ImGui::Dummy(ImVec2(0, 10 * s));
					static float hoverAvanzado = 0.0f;
					if (salaBoton("##avanzado", ImVec2(260 * s, 38 * s), hoverAvanzado, false, tema->secondary,
						T("sala.avanzado"), tema, s)) {
						e.avanzado = true;
					}
				}
			}
			catch (...) {
				// Si algo falla al leer la sala, se muestra lo que haya sin cerrar la app.
			}

			ImGui::End();
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar(4);
		}

	}

}
