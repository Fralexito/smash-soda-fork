#include "Shell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "imgui.h"
#include "UiDock.h"
#include "../I18n.h"
#include "../PhoenixBuild.h"
#include "../PhoenixPrefs.h"
#include "PantallaSala.h"
#include "TableroMandos.h"
#include "PantallaGente.h"
#include "PantallaAjustes.h"
#include "PantallaBiblioteca.h"
#include "../core/ProveedorSala.h"
#include "../core/MandoHost.h"
#include "../../globals/AppFonts.h"
#include "../../globals/AppIcons.h"
#include "../../services/ThemeController.h"


namespace phoenix {

	namespace {

		ProveedorSala* gProveedor = nullptr; // nullptr = app sin modo host

		// ---------------------------------------------------------------------
		//  Medidas base (se multiplican por la escala de la UI)
		// ---------------------------------------------------------------------
		constexpr float kAltoBarra = 58.0f;
		constexpr float kAnchoMenu = 92.0f;
		constexpr float kAnchoChat = 360.0f;
		constexpr float kAltoEstado = 30.0f;
		constexpr float kSeparacion = 12.0f;
		constexpr float kAltoCabecera = 70.0f;
		constexpr float kDuracionTransicion = 0.20f; // s, corto y firme

		enum Seccion { SALA = 0, MANDOS, GENTE, AJUSTES, TOTAL_SECCIONES };

		struct EstadoUi {
			int seccion = SALA;
			int pestana[TOTAL_SECCIONES] = {};
			double cambioEn = -10.0;      // momento del último cambio (para el fundido)
			float indicadorY = -1.0f;     // barra activa del menú (animada)
			float hover[TOTAL_SECCIONES] = {};
			float subrayadoX = -1.0f, subrayadoW = 0.0f; // pestaña activa (animada)
			bool iniciado = false;
		};

		EstadoUi& ui() {
			static EstadoUi e;
			return e;
		}

		float escala() {
			const float s = ThemeController::getInstance().getUiScale();
			return s > 0.0f ? s : 1.0f;
		}

		float acercar(float actual, float objetivo, float velocidad) {
			const float dt = ImGui::GetIO().DeltaTime;
			const float k = (std::min)(1.0f, dt * velocidad);
			return actual + (objetivo - actual) * k;
		}

		ImU32 col(const ImVec4& c, float alfa = 1.0f) {
			return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alfa));
		}

		const ImGuiWindowFlags kFlagsFijo =
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoScrollWithMouse;

		/// Abre una ventana fija del shell (barra, menú…). Siempre cerrar con finFija().
		void inicioFija(const char* id, ImVec2 pos, ImVec2 tam, const ImVec4& fondo) {
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, fondo);
			ImGui::Begin(id, nullptr, kFlagsFijo);
		}

		void finFija() {
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(3);
		}

		void cambiarSeccion(int nueva) {
			EstadoUi& e = ui();
			if (nueva == e.seccion) return;
			e.seccion = nueva;
			e.cambioEn = ImGui::GetTime();
			e.subrayadoX = -1.0f;
			PhoenixPrefs::get().seccion = nueva;
			PhoenixPrefs::get().guardar();
		}

		// ---------------------------------------------------------------------
		//  Barra superior
		// ---------------------------------------------------------------------
		void barraSuperior(ImVec2 pos, ImVec2 tam, Theme* tema, float s) {
			inicioFija("##phx_barra", pos, tam, tema->navbarBackground);
			ImDrawList* dl = ImGui::GetWindowDrawList();

			// Línea de acento cian → púrpura bajo la barra
			const float yLinea = pos.y + tam.y - 2.0f * s;
			dl->AddRectFilledMultiColor(ImVec2(pos.x, yLinea), ImVec2(pos.x + tam.x, pos.y + tam.y),
				col(tema->primary), col(tema->secondary), col(tema->secondary), col(tema->primary));

			// Marca
			ImGui::PushFont(AppFonts::title);
			const float yTexto = pos.y + (tam.y - ImGui::GetFontSize()) * 0.5f;
			dl->AddText(ImVec2(pos.x + 24.0f * s, yTexto), col(tema->primary), "PHOENIX");
			const float anchoMarca = ImGui::CalcTextSize("PHOENIX").x;
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::label);
			dl->AddText(ImVec2(pos.x + 30.0f * s + anchoMarca, yTexto + 2.0f * s), col(tema->textMuted), "SODA");
			ImGui::PopFont();

			// Píldora de estado de la sala (late suave cuando está en vivo)
			const bool enVivo = gProveedor != nullptr && gProveedor->abierta();
			const int invitados = gProveedor != nullptr ? gProveedor->totalInvitados() : 0;
			std::string textoEstado = enVivo ? T("estado.vivo") : T("estado.cerrada");
			if (enVivo) textoEstado += "  ·  " + std::to_string(invitados) + " " + T("estado.invitados");
			if (MandoHost::activo() > 0) {
				char b[96];
				snprintf(b, sizeof(b), T("estado.controlando"), MandoHost::activo());
				textoEstado = b;
			}

			ImGui::PushFont(AppFonts::label);
			const ImVec2 tamTexto = ImGui::CalcTextSize(textoEstado.c_str());
			const float xPildora = pos.x + 230.0f * s;
			const float altoPildora = 30.0f * s;
			const ImVec2 p0(xPildora, pos.y + (tam.y - altoPildora) * 0.5f);
			const ImVec2 p1(p0.x + tamTexto.x + 44.0f * s, p0.y + altoPildora);
			const ImVec4 colorEstado = enVivo ? tema->primary : tema->textMuted;
			const float pulso = enVivo ? 0.55f + 0.45f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.0f) : 1.0f;

			dl->AddRectFilled(p0, p1, col(colorEstado, 0.12f), altoPildora * 0.5f);
			dl->AddRect(p0, p1, col(colorEstado, 0.45f), altoPildora * 0.5f, 0, 1.0f * s);
			const ImVec2 centroPunto(p0.x + 16.0f * s, (p0.y + p1.y) * 0.5f);
			if (enVivo) dl->AddCircleFilled(centroPunto, 9.0f * s, col(colorEstado, 0.25f * pulso));
			dl->AddCircleFilled(centroPunto, 5.0f * s, col(colorEstado, enVivo ? pulso : 0.7f));
			dl->AddText(ImVec2(p0.x + 30.0f * s, (p0.y + p1.y - tamTexto.y) * 0.5f), col(colorEstado), textoEstado.c_str());
			ImGui::PopFont();

			// Derecha: idioma + interfaz clásica
			ImGui::PushFont(AppFonts::input);
			const float anchoBoton = ImGui::CalcTextSize(T("barra.clasica")).x + 28.0f * s;
			const float anchoCombo = 150.0f * s;
			const float altoControl = ImGui::GetFrameHeight();
			const float yControl = (tam.y - altoControl) * 0.5f;

			ImGui::SetCursorPos(ImVec2(tam.x - anchoBoton - anchoCombo - 36.0f * s, yControl));
			ImGui::SetNextItemWidth(anchoCombo);
			const std::string actual = I18n::actual();
			std::string nombreActual = actual;
			const auto idiomas = I18n::disponibles();
			for (const auto& par : idiomas) if (par.first == actual) nombreActual = par.second;
			if (ImGui::BeginCombo("##phx_idioma", nombreActual.c_str())) {
				for (const auto& par : idiomas) {
					const bool elegido = par.first == actual;
					if (ImGui::Selectable(par.second.c_str(), elegido)) {
						I18n::establecer(par.first);
						PhoenixPrefs::get().idioma = par.first;
						PhoenixPrefs::get().guardar();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", T("barra.idioma"));

			ImGui::SameLine(0, 12.0f * s);
			ImGui::PushStyleColor(ImGuiCol_Button, tema->buttonSecondary);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tema->buttonSecondaryHovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, tema->buttonSecondaryActive);
			if (ImGui::Button(T("barra.clasica"), ImVec2(anchoBoton, altoControl))) {
				PhoenixPrefs::get().interfazPhoenix = false;
				PhoenixPrefs::get().guardar();
			}
			ImGui::PopStyleColor(3);
			ImGui::PopFont();

			finFija();
		}

		// ---------------------------------------------------------------------
		//  Menú lateral
		// ---------------------------------------------------------------------
		struct ItemMenu {
			const char* clave;
			ID3D11ShaderResourceView* icono;
		};

		void menuLateral(ImVec2 pos, ImVec2 tam, Theme* tema, float s) {
			EstadoUi& e = ui();
			inicioFija("##phx_menu", pos, tam, tema->navbarBackground);
			ImDrawList* dl = ImGui::GetWindowDrawList();

			const ItemMenu items[TOTAL_SECCIONES] = {
				{ "nav.sala",      AppIcons::play },
				{ "nav.mandos",    AppIcons::padOn },
				{ "nav.gente",     AppIcons::users },
				{ "nav.ajustes",   AppIcons::settings },
			};

			const float altoItem = 76.0f * s;
			const float yInicio = pos.y + 14.0f * s;
			const float objetivoY = yInicio + e.seccion * altoItem;
			e.indicadorY = (e.indicadorY < 0.0f) ? objetivoY : acercar(e.indicadorY, objetivoY, 16.0f);

			// Fondo del ítem activo + barra de acento (se desliza entre ítems)
			dl->AddRectFilled(ImVec2(pos.x + 10.0f * s, e.indicadorY + 4.0f * s),
				ImVec2(pos.x + tam.x - 10.0f * s, e.indicadorY + altoItem - 4.0f * s),
				col(tema->secondary, 0.18f), 12.0f * s);
			dl->AddRectFilled(ImVec2(pos.x, e.indicadorY + 18.0f * s),
				ImVec2(pos.x + 4.0f * s, e.indicadorY + altoItem - 18.0f * s),
				col(tema->primary), 2.0f * s);

			ImGui::PushFont(AppFonts::input);
			for (int i = 0; i < TOTAL_SECCIONES; i++) {
				const ImVec2 p0(pos.x, yInicio + i * altoItem);
				ImGui::SetCursorScreenPos(p0);
				ImGui::PushID(i);
				const bool clic = ImGui::InvisibleButton("##item", ImVec2(tam.x, altoItem));
				const bool encima = ImGui::IsItemHovered();
				ImGui::PopID();
				if (clic) cambiarSeccion(i);
				if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				const bool activo = (i == e.seccion);
				e.hover[i] = acercar(e.hover[i], encima ? 1.0f : 0.0f, 18.0f);

				if (!activo && e.hover[i] > 0.01f) {
					dl->AddRectFilled(ImVec2(p0.x + 10.0f * s, p0.y + 4.0f * s),
						ImVec2(p0.x + tam.x - 10.0f * s, p0.y + altoItem - 4.0f * s),
						col(tema->textMuted, 0.08f * e.hover[i]), 12.0f * s);
				}

				const ImVec4 colorItem = activo ? tema->primary
					: ImVec4(tema->textMuted.x, tema->textMuted.y, tema->textMuted.z, 0.75f + 0.25f * e.hover[i]);
				const float lado = 26.0f * s;
				const float levantar = 2.0f * s * e.hover[i]; // micro-elevación al pasar el mouse
				const ImVec2 i0(p0.x + (tam.x - lado) * 0.5f, p0.y + 14.0f * s - levantar);
				if (items[i].icono != nullptr) {
					dl->AddImage(items[i].icono, i0, ImVec2(i0.x + lado, i0.y + lado),
						ImVec2(0, 0), ImVec2(1, 1), col(colorItem));
				}
				const char* texto = T(items[i].clave);
				const ImVec2 tamTexto = ImGui::CalcTextSize(texto);
				const float escalaTexto = (std::min)(1.0f, (tam.x - 8.0f * s) / (std::max)(1.0f, tamTexto.x));
				dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * escalaTexto,
					ImVec2(p0.x + (tam.x - tamTexto.x * escalaTexto) * 0.5f, p0.y + 46.0f * s),
					col(colorItem), texto);
			}
			ImGui::PopFont();
			finFija();
		}

		// ---------------------------------------------------------------------
		//  Barra inferior
		// ---------------------------------------------------------------------
		void barraEstado(ImVec2 pos, ImVec2 tam, Theme* tema, float s) {
			inicioFija("##phx_estado", pos, tam, tema->navbarBackground);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImGui::PushFont(AppFonts::input);

			std::string izquierda = std::string(kNombreBuild) + " " + kVersion;
			const std::string cuenta = gProveedor != nullptr ? gProveedor->cuentaHost() : std::string();
			std::string derecha = !cuenta.empty() ? "Parsec: " + cuenta : std::string(T("barra.sin_host"));

			const float y = pos.y + (tam.y - ImGui::GetFontSize()) * 0.5f;
			dl->AddText(ImVec2(pos.x + 18.0f * s, y), col(tema->textMuted), izquierda.c_str());
			const float anchoDer = ImGui::CalcTextSize(derecha.c_str()).x;
			dl->AddText(ImVec2(pos.x + tam.x - anchoDer - 18.0f * s, y), col(tema->textMuted), derecha.c_str());

			ImGui::PopFont();
			finFija();
		}

		// ---------------------------------------------------------------------
		//  Cabecera de sección + pestañas
		// ---------------------------------------------------------------------
		/// Dibuja título, subtítulo y (si hay) pestañas. Devuelve la pestaña activa.
		int cabecera(ImVec2 pos, ImVec2 tam, Theme* tema, float s,
			const char* claveTitulo, const char* claveSub, const std::vector<const char*>& pestanas) {

			EstadoUi& e = ui();
			inicioFija("##phx_cabecera", pos, tam, tema->background);
			ImDrawList* dl = ImGui::GetWindowDrawList();

			ImGui::PushFont(AppFonts::title);
			dl->AddText(ImVec2(pos.x + 4.0f * s, pos.y + 8.0f * s), col(tema->text), T(claveTitulo));
			const float anchoTitulo = ImGui::CalcTextSize(T(claveTitulo)).x;
			ImGui::PopFont();
			ImGui::PushFont(AppFonts::input);
			dl->AddText(ImVec2(pos.x + 4.0f * s, pos.y + 40.0f * s), col(tema->textMuted), T(claveSub));
			ImGui::PopFont();

			int& activa = e.pestana[e.seccion];
			if (pestanas.empty()) {
				finFija();
				return 0;
			}
			activa = std::clamp(activa, 0, static_cast<int>(pestanas.size()) - 1);

			// Pestañas tipo píldora a la derecha del título, con subrayado que se desliza
			ImGui::PushFont(AppFonts::label);
			float x = pos.x + (std::max)(anchoTitulo, 260.0f * s) + 40.0f * s;
			const float altoPestana = 36.0f * s;
			const float yPestana = pos.y + 12.0f * s;
			float objetivoX = x, objetivoW = 0.0f;

			for (int i = 0; i < static_cast<int>(pestanas.size()); i++) {
				const char* texto = T(pestanas[i]);
				const float ancho = ImGui::CalcTextSize(texto).x + 28.0f * s;
				ImGui::SetCursorScreenPos(ImVec2(x, yPestana));
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##pestana", ImVec2(ancho, altoPestana))) {
					if (activa != i) {
						activa = i;
						e.cambioEn = ImGui::GetTime();
					}
				}
				const bool encima = ImGui::IsItemHovered();
				ImGui::PopID();
				if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				const bool esActiva = (i == activa);
				if (esActiva) { objetivoX = x; objetivoW = ancho; }
				if (encima && !esActiva) {
					dl->AddRectFilled(ImVec2(x, yPestana), ImVec2(x + ancho, yPestana + altoPestana),
						col(tema->textMuted, 0.08f), altoPestana * 0.5f);
				}
				dl->AddText(ImVec2(x + 14.0f * s, yPestana + (altoPestana - ImGui::GetFontSize()) * 0.5f),
					col(esActiva ? tema->primary : tema->textMuted), texto);
				x += ancho + 6.0f * s;
			}

			if (e.subrayadoX < 0.0f) { e.subrayadoX = objetivoX; e.subrayadoW = objetivoW; }
			e.subrayadoX = acercar(e.subrayadoX, objetivoX, 18.0f);
			e.subrayadoW = acercar(e.subrayadoW, objetivoW, 18.0f);
			dl->AddRectFilled(ImVec2(e.subrayadoX + 12.0f * s, yPestana + altoPestana + 2.0f * s),
				ImVec2(e.subrayadoX + e.subrayadoW - 12.0f * s, yPestana + altoPestana + 5.0f * s),
				col(tema->primary), 2.0f * s);

			ImGui::PopFont();
			finFija();
			return activa;
		}

		// ---------------------------------------------------------------------
		//  Paneles acoplados (con fundido + desplazamiento corto al cambiar)
		// ---------------------------------------------------------------------
		float progresoTransicion() {
			const double t = (ImGui::GetTime() - ui().cambioEn) / kDuracionTransicion;
			const float x = static_cast<float>(std::clamp(t, 0.0, 1.0));
			return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x); // ease-out
		}

		void acoplar(const std::function<void()>& dibujar, ImVec2 pos, ImVec2 tam, float s) {
			if (!dibujar) return;
			const float p = progresoTransicion();
			const ImVec2 posAnimada(pos.x, pos.y + (1.0f - p) * 10.0f * s);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f + 0.75f * p);
			UiDock::colocar(posAnimada, tam);
			try {
				dibujar();
			}
			catch (...) {
				// Un panel defectuoso no debe tumbar la app entera.
			}
			UiDock::cancelar(); // por si el panel no llamó a startWidget
			ImGui::PopStyleVar();
		}
	}

	// =========================================================================
	//  API pública
	// =========================================================================
	void Shell::render(const Paneles& p) {
		EstadoUi& e = ui();
		if (!e.iniciado) {
			e.iniciado = true;
			e.seccion = std::clamp(PhoenixPrefs::get().seccion, 0, TOTAL_SECCIONES - 1);
			e.cambioEn = ImGui::GetTime();
		}

		PantallaSala::tick(gProveedor); // web y asientos, en cualquier sección
		MandoHost::tick(gProveedor);    // Ctrl+Alt+N: el host toma un mando

		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = escala();
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		const ImVec2 o = vp->WorkPos;
		const ImVec2 v = vp->WorkSize;

		const float altoBarra = kAltoBarra * s;
		const float anchoMenu = kAnchoMenu * s;
		const float altoEstado = kAltoEstado * s;
		const float sep = kSeparacion * s;
		const float anchoChat = (std::max)(260.0f * s, (std::min)(kAnchoChat * s, v.x * 0.26f));

		// Zonas fijas
		barraSuperior(o, ImVec2(v.x, altoBarra), tema, s);
		menuLateral(ImVec2(o.x, o.y + altoBarra), ImVec2(anchoMenu, v.y - altoBarra - altoEstado), tema, s);
		barraEstado(ImVec2(o.x, o.y + v.y - altoEstado), ImVec2(v.x, altoEstado), tema, s);

		// Chat fijo a la derecha
		const float yCuerpo = o.y + altoBarra + sep;
		const float altoCuerpo = v.y - altoBarra - altoEstado - sep * 2.0f;
		const float xChat = o.x + v.x - anchoChat - sep;
		{
			UiDock::colocar(ImVec2(xChat, yCuerpo), ImVec2(anchoChat, altoCuerpo));
			if (p.chat) p.chat();
			UiDock::cancelar();
		}

		// Área de contenido
		const float xCont = o.x + anchoMenu + sep;
		const float anchoCont = xChat - sep - xCont;
		const float altoCab = kAltoCabecera * s;
		const ImVec2 posPanel(xCont, yCuerpo + altoCab);
		const ImVec2 tamPanel(anchoCont, altoCuerpo - altoCab);

		switch (e.seccion) {
		case SALA: {
			cabecera(ImVec2(xCont, yCuerpo), ImVec2(anchoCont, altoCab), tema, s, "nav.sala", "sub.sala", {});
			const float alfa = 0.25f + 0.75f * progresoTransicion();
			PantallaSala::render(gProveedor, p.actividad, p.configSala,
				[s](const std::function<void()>& dibujar, ImVec2 pos, ImVec2 tam) { acoplar(dibujar, pos, tam, s); },
				posPanel, tamPanel, alfa);
			break;
		}
		case MANDOS: {
			const int t = cabecera(ImVec2(xCont, yCuerpo), ImVec2(anchoCont, altoCab), tema, s, "nav.mandos", "sub.mandos",
				{ "tab.mandos", "tab.turnos", "tab.teclado" });
			if (t == 0 && gProveedor != nullptr) {
				// Tablero propio de Phoenix (8 mandos, equipos, arrastrar y soltar)
				const float alfa = 0.25f + 0.75f * progresoTransicion();
				ImGui::SetNextWindowPos(posPanel, ImGuiCond_Always);
				ImGui::SetNextWindowSize(tamPanel, ImGuiCond_Always);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alfa);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * s, 20 * s));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
				ImGui::Begin("##phx_mandos", nullptr,
					ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
				try { TableroMandos::render(*gProveedor, ImGui::GetContentRegionAvail().x, false); } catch (...) {}
				ImGui::End();
				ImGui::PopStyleColor();
				ImGui::PopStyleVar(3);
			}
			else {
				const std::function<void()>* paneles[] = { &p.mandos, &p.hotseat, &p.teclado };
				acoplar(*paneles[t], posPanel, tamPanel, s);
			}
			break;
		}
		case GENTE: {
			const int t = cabecera(ImVec2(xCont, yCuerpo), ImVec2(anchoCont, altoCab), tema, s, "nav.gente", "sub.gente",
				{ "tab.en_sala", "tab.moderacion" });
			if (t == 0) PantallaGente::render(gProveedor, posPanel, tamPanel, 0.25f + 0.75f * progresoTransicion());
			else acoplar(p.invitados, posPanel, tamPanel, s);
			break;
		}
		case AJUSTES: {
			const int t = cabecera(ImVec2(xCont, yCuerpo), ImVec2(anchoCont, altoCab), tema, s, "nav.ajustes", "sub.ajustes",
				{ "tab.rapido", "tab.general", "tab.video", "tab.audio", "tab.biblioteca", "tab.avanzado" });
			if (t == 0) {
				PantallaAjustes::rapido(gProveedor, posPanel, tamPanel, 0.25f + 0.75f * progresoTransicion());
			}
			else if (t == 4 && PhoenixPrefs::get().interfazPhoenix) {
				// Biblioteca mejorada en Phoenix
				PantallaBiblioteca::render(gProveedor, posPanel, tamPanel, 0.25f + 0.75f * progresoTransicion());
			}
			else {
				const std::function<void()>* paneles[] = { &p.general, &p.video, &p.audio, &p.biblioteca, &p.avanzado };
				acoplar(*paneles[t - 1], posPanel, tamPanel, s);
			}
			break;
		}
		default:
			e.seccion = SALA;
			break;
		}
	}

	void Shell::establecerProveedor(ProveedorSala* proveedor) {
		gProveedor = proveedor;
	}

	void Shell::renderBotonVolver() {
		Theme* tema = ThemeController::getInstance().getActiveTheme();
		const float s = escala();
		const ImGuiViewport* vp = ImGui::GetMainViewport();

		ImGui::PushFont(AppFonts::label);
		const char* texto = T("barra.volver");
		const ImVec2 tam(ImGui::CalcTextSize(texto).x + 36.0f * s, 40.0f * s);
		const ImVec2 pos(vp->WorkPos.x + vp->WorkSize.x - tam.x - 20.0f * s, vp->WorkPos.y + 14.0f * s);

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, tam.y * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, tema->buttonPrimary);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tema->buttonPrimaryHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, tema->buttonPrimaryActive);
		ImGui::PushStyleColor(ImGuiCol_Text, tema->buttonPrimaryText);
		ImGui::Begin("##phx_volver", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		if (ImGui::Button(texto, tam)) {
			PhoenixPrefs::get().interfazPhoenix = true;
			PhoenixPrefs::get().guardar();
		}
		ImGui::End();
		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(3);
		ImGui::PopFont();
	}

}
