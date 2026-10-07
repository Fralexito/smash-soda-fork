#include "PantallaSala.h"

#include <cfloat>
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
			dl->PushClipRect(ImVec2(e0.x + 14 * s, e0.y), ImVec2(e1.x - 110 * s, e1.y), true);
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
	}

	void PantallaSala::render(ProveedorSala* sala,
		const std::function<void()>& panelActividad,
		const std::function<void()>& panelAvanzado,
		const Acoplador& acoplar,
		ImVec2 pos, ImVec2 tam, float alfa) {

		EstadoSala& e = salaEstado();
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = salaEscala();
		const float sep = 12.0f * s;

		// Reparto: principal a la izquierda, actividad a la derecha (o abajo si es angosto)
		const bool ancho = tam.x >= 900.0f * s;
		const ImVec2 tamPrincipal = ancho
			? ImVec2(std::floor((tam.x - sep) * 0.64f), tam.y)
			: ImVec2(tam.x, std::floor((tam.y - sep) * 0.68f));
		const ImVec2 posActividad = ancho
			? ImVec2(pos.x + tamPrincipal.x + sep, pos.y)
			: ImVec2(pos.x, pos.y + tamPrincipal.y + sep);
		const ImVec2 tamActividad = ancho
			? ImVec2(tam.x - tamPrincipal.x - sep, tam.y)
			: ImVec2(tam.x, tam.y - tamPrincipal.y - sep);

		if (sala == nullptr) return; // forma sin modo host: el shell muestra otra pantalla

		if (e.avanzado) {
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
				salaTarjetaPrincipal(*sala, tema, s, anchoUtil);
				salaOpciones(tema, s, anchoUtil);
				salaTitulo(T("sala.mandos"), tema, s);
				TableroMandos::render(*sala, anchoUtil, true);

				ImGui::Dummy(ImVec2(0, 10 * s));
				static float hoverAvanzado = 0.0f;
				if (salaBoton("##avanzado", ImVec2(260 * s, 38 * s), hoverAvanzado, false, tema->secondary,
					T("sala.avanzado"), tema, s)) {
					e.avanzado = true;
				}
			}
			catch (...) {
				// Si algo falla al leer la sala, se muestra lo que haya sin cerrar la app.
			}

			ImGui::End();
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar(4);
		}

		acoplar(panelActividad, posActividad, tamActividad);
	}

}
