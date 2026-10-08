#include "InterfazWebInterno.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

#include "imgui.h"
#include "../../Hosting.h"
#include "../../core/Config.h"
#include "../../helpers/PathHelper.h"
#include "../../globals/AppFonts.h"
#include "../../services/ThemeController.h"
#include "../PhoenixPrefs.h"
#include "../I18n.h"
#include "../ui/Shell.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	// =========================================================================
	//  Utilidades compartidas
	// =========================================================================
	Interno& interno() {
		static Interno in;
		return in;
	}

	double ahoraSeg() {
		using namespace std::chrono;
		return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
	}

	int64_t ahoraEpochMs() {
		using namespace std::chrono;
		return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}

	std::string rutaConfig(const std::string& archivo) {
		return PathHelper::GetConfigPath() + archivo;
	}

	ProveedorSala& salaObligatoria(Interno& in) {
		if (in.ctx.sala == nullptr) throw ErrorAccion("SIN_SALA", "Esta copia de la app no tiene el modo host activo.");
		return *in.ctx.sala;
	}

	Hosting& hostingObligatorio(Interno& in) {
		if (in.ctx.hosting == nullptr) throw ErrorAccion("SIN_SALA", "Esta copia de la app no tiene el modo host activo.");
		return *in.ctx.hosting;
	}

	int entero(const nlohmann::json& datos, const char* clave, int minimo, int maximo) {
		if (!datos.contains(clave) || !datos[clave].is_number()) throw ErrorAccion("DATOS_INVALIDOS", std::string("Falta el número «") + clave + "».");
		const double v = datos[clave].get<double>();
		if (v < minimo || v > maximo) throw ErrorAccion("FUERA_DE_RANGO", std::string("«") + clave + "» debe estar entre " + std::to_string(minimo) + " y " + std::to_string(maximo) + ".");
		return static_cast<int>(v);
	}

	uint32_t idParsec(const nlohmann::json& datos, const char* clave) {
		if (!datos.contains(clave)) throw ErrorAccion("DATOS_INVALIDOS", "Falta el ID de Parsec.");
		const nlohmann::json& v = datos[clave];
		try {
			if (v.is_number_unsigned()) return v.get<uint32_t>();
			if (v.is_number_integer() && v.get<int64_t>() > 0) return static_cast<uint32_t>(v.get<int64_t>());
			if (v.is_string()) return static_cast<uint32_t>(std::stoul(v.get<std::string>()));
		}
		catch (...) {}
		throw ErrorAccion("DATOS_INVALIDOS", "ID de Parsec inválido.");
	}

	std::string texto(const nlohmann::json& datos, const char* clave, size_t maximo) {
		if (!datos.contains(clave) || !datos[clave].is_string()) throw ErrorAccion("DATOS_INVALIDOS", std::string("Falta el texto «") + clave + "».");
		std::string s = datos[clave].get<std::string>();
		if (s.size() > maximo) {
			size_t corte = maximo;
			while (corte > 0 && (static_cast<unsigned char>(s[corte]) & 0xC0) == 0x80) corte--; // sin partir letras
			s = s.substr(0, corte);
		}
		return s;
	}

	bool booleano(const nlohmann::json& datos, const char* clave) {
		if (!datos.contains(clave) || !datos[clave].is_boolean()) throw ErrorAccion("DATOS_INVALIDOS", std::string("Falta el valor sí/no «") + clave + "».");
		return datos[clave].get<bool>();
	}

	bool copiarAlPortapapeles(const std::string& texto) {
		const int n = MultiByteToWideChar(CP_UTF8, 0, texto.c_str(), -1, nullptr, 0);
		if (n <= 0) return false;
		if (!OpenClipboard(static_cast<HWND>(interno().ctx.ventana))) return false;
		EmptyClipboard();
		HGLOBAL memoria = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(n) * sizeof(wchar_t));
		bool ok = false;
		if (memoria != nullptr) {
			wchar_t* destino = static_cast<wchar_t*>(GlobalLock(memoria));
			if (destino != nullptr) {
				MultiByteToWideChar(CP_UTF8, 0, texto.c_str(), -1, destino, n);
				GlobalUnlock(memoria);
				ok = SetClipboardData(CF_UNICODETEXT, memoria) != nullptr;
			}
			if (!ok) GlobalFree(memoria);
		}
		CloseClipboard();
		return ok;
	}

	void abrirEnlace(const std::string& url) {
		if (url.rfind("https://", 0) != 0 || url.size() > 500) throw ErrorAccion("ENLACE_INVALIDO", "Solo se abren enlaces https.");
		const int n = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
		std::wstring w(static_cast<size_t>((std::max)(n, 1)), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &w[0], n);
		ShellExecuteW(nullptr, L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}

	void mensajeDelBot(Interno& in, const std::string& texto) {
		if (in.ctx.hosting == nullptr || texto.empty()) return;
		try {
			// Como Hotseat y AutoMod: a todos los invitados y a la Actividad del host.
			in.ctx.hosting->broadcastChatMessageAndLogCommand(Config::cfg.chatbotName + texto);
		}
		catch (...) {}
	}

	// =========================================================================
	namespace detalle_interfaz {

		std::wstring carpetaExe() {
			wchar_t ruta[MAX_PATH] = {};
			const DWORD n = GetModuleFileNameW(nullptr, ruta, MAX_PATH);
			std::wstring r(ruta, n);
			const size_t barra = r.find_last_of(L"\\/");
			return barra == std::wstring::npos ? std::wstring(L".") : r.substr(0, barra);
		}

		std::wstring carpetaDatosWebView() {
			PWSTR local = nullptr;
			std::wstring r;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local)) && local != nullptr) {
				r = std::wstring(local) + L"\\PhoenixLink\\WebView2";
			}
			if (local != nullptr) CoTaskMemFree(local);
			if (r.empty()) r = carpetaExe() + L"\\datos-webview2";
			return r;
		}

		void crearVista(Interno& in) {
			in.anfitrion = std::make_unique<AnfitrionWeb>();
			in.anfitrion->alMensaje([&in](const std::string& texto) {
				if (in.puente) in.puente->recibir(texto);
			});
			if (in.puente) in.puente->reiniciar();
			in.creadaEn = ahoraSeg();
			in.anfitrion->iniciar(in.ctx.ventana, carpetaExe() + L"\\ui", carpetaDatosWebView(), Config::cfg.general.devMode);
		}

		void destruirVista(Interno& in) {
			if (in.anfitrion) {
				in.anfitrion->mostrar(false);
				in.anfitrion->cerrar();
				in.anfitrion.reset();
			}
			if (in.puente) in.puente->reiniciar();
		}

		EstadoAnfitrion gEstadoAnterior = EstadoAnfitrion::Apagado;
	}

	using namespace detalle_interfaz;

	// =========================================================================
	InterfazWeb& InterfazWeb::instancia() {
		static InterfazWeb unica;
		return unica;
	}

	void InterfazWeb::iniciar(const ContextoWeb& contexto) {
		Interno& in = interno();
		if (in.iniciada) return;
		try {
			in.ctx = contexto;
			in.historial = std::make_unique<HistorialPartidos>(rutaConfig("phoenix-partidos.json"));
			in.perfilesSala = std::make_unique<PerfilesSala>(rutaConfig("phoenix-perfiles-sala.json"));
			in.puente = std::make_unique<Puente>([&in](const std::string& texto) {
				if (in.anfitrion) in.anfitrion->enviar(texto);
			});
			in.puente->alSaludar([&in](const nlohmann::json&) { return construirBienvenida(in); });

			registrarAccionesApp(in);
			registrarAccionesSala(in);
			registrarAccionesMandos(in);
			registrarAccionesGente(in);
			registrarAccionesAjustes(in);
			registrarAccionesPartido(in);

			in.iniciada = true;
			if (PhoenixPrefs::get().interfazWeb) crearVista(in);
		}
		catch (...) {
			// Si algo falla aquí, la app sigue con la interfaz ImGui.
			in.iniciada = in.puente != nullptr;
		}
	}

	void InterfazWeb::cerrar() {
		Interno& in = interno();
		try {
			destruirVista(in);
			PhoenixLink::instancia().marcarPartido(false);
		}
		catch (...) {}
	}

	void InterfazWeb::tick(bool permitida) {
		Interno& in = interno();
		if (!in.iniciada) return;
		try {
			in.permitida = permitida;
			HWND h = static_cast<HWND>(in.ctx.ventana);
			const bool ventanaVisible = h != nullptr && IsWindowVisible(h) && !IsIconic(h);
			PhoenixLink::instancia().ventanaVisible(ventanaVisible);

			tickDatos(in);

			// Cambios pedidos por la propia página (cerrar el WebView dentro de su aviso no es seguro)
			if (!in.cambioInterfaz.empty()) {
				const std::string modo = in.cambioInterfaz;
				in.cambioInterfaz.clear();
				PhoenixPrefs& pr = PhoenixPrefs::get();
				pr.interfazWeb = false;
				pr.interfazPhoenix = modo != "clasica";
				pr.guardar();
				in.panelClasico = false;
				destruirVista(in);
				return;
			}
			if (in.recargarPendiente && in.anfitrion) {
				in.recargarPendiente = false;
				in.anfitrion->recargar();
			}

			if (!in.anfitrion) return;
			AnfitrionWeb& a = *in.anfitrion;
			EstadoAnfitrion est = a.estado();

			// Vigilante: si en 30 s no cargó, se da por fallida (la app sigue con ImGui).
			if ((est == EstadoAnfitrion::Creando || est == EstadoAnfitrion::Cargando) && a.segundosDesdeInicio() > 30.0
				&& !(in.puente && in.puente->listo())) {
				destruirVista(in);
				in.anfitrion = std::make_unique<AnfitrionWeb>(); // queda en Apagado con motivo
				return;
			}
			// La página se recargó (o se cayó): hay que esperar un «hola» nuevo.
			if (gEstadoAnterior == EstadoAnfitrion::Listo && est != EstadoAnfitrion::Listo && in.puente) in.puente->reiniciar();
			gEstadoAnterior = est;

			const bool lista = est == EstadoAnfitrion::Listo && in.puente && in.puente->listo();
			const bool mostrar = lista && permitida && !in.panelClasico && PhoenixPrefs::get().interfazWeb;
			a.mostrar(mostrar);

			if (lista) {
				tickResultadosWeb(in);
				const double ahora = ahoraSeg();
				if (ahora - in.ultimaFoto >= 0.2) { // ~5 veces por segundo, solo si cambió
					in.ultimaFoto = ahora;
					in.puente->publicarEstado(construirEstado(in));
				}
			}
		}
		catch (...) {
			// Un error aquí nunca debe tumbar el bucle principal.
		}
	}

	bool InterfazWeb::cubreVentana() const {
		const Interno& in = interno();
		return in.anfitrion != nullptr && in.anfitrion->visible() && in.permitida && !in.panelClasico;
	}

	bool InterfazWeb::disponible() const {
		const Interno& in = interno();
		return in.anfitrion != nullptr && in.anfitrion->estado() == EstadoAnfitrion::Listo;
	}

	std::string InterfazWeb::motivoNoDisponible() const {
		const Interno& in = interno();
		if (!in.anfitrion) return "La interfaz nueva está apagada.";
		const std::string e = in.anfitrion->error();
		if (!e.empty()) return e;
		if (in.anfitrion->estado() == EstadoAnfitrion::Apagado) return "La interfaz nueva tardó demasiado en abrir.";
		return std::string();
	}

	void InterfazWeb::usarWeb(bool si) {
		Interno& in = interno();
		PhoenixPrefs& pr = PhoenixPrefs::get();
		pr.interfazWeb = si;
		if (si) pr.interfazPhoenix = true; // el respaldo de la web es el shell Phoenix
		pr.guardar();
		in.panelClasico = false;
		in.avisoCerrado = false;
		if (si) {
			if (!in.anfitrion || in.anfitrion->estado() == EstadoAnfitrion::Fallo || in.anfitrion->estado() == EstadoAnfitrion::Apagado) {
				destruirVista(in);
				crearVista(in);
			}
		}
		else {
			destruirVista(in);
		}
	}

	void InterfazWeb::alRedimensionar(bool minimizada) {
		Interno& in = interno();
		if (!in.anfitrion) return;
		in.anfitrion->minimizada(minimizada);
		if (!minimizada) in.anfitrion->ajustar();
	}

	void InterfazWeb::alMoverVentana() {
		Interno& in = interno();
		if (in.anfitrion) in.anfitrion->ventanaMovida();
	}

	// =========================================================================
	//  ImGui: «Volver a la interfaz nueva» y aviso si no se pudo abrir
	// =========================================================================
	void InterfazWeb::renderImGui() {
		Interno& in = interno();
		if (!in.iniciada || ImGui::GetCurrentContext() == nullptr) return;
		try {
			Theme* tema = ThemeController::getInstance().getActiveTheme();
			const float s = (std::max)(1.0f, ThemeController::getInstance().getUiScale());
			const ImGuiViewport* vp = ImGui::GetMainViewport();

			if (in.panelClasico) {
				ImGui::PushFont(AppFonts::label);
				const char* texto = T("web.volver_nueva");
				const ImVec2 tam(ImGui::CalcTextSize(texto).x + 40.0f * s, 38.0f * s);
				ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - tam.x - 18.0f * s, vp->WorkPos.y + vp->WorkSize.y - tam.y - 44.0f * s), ImGuiCond_Always);
				ImGui::SetNextWindowSize(tam, ImGuiCond_Always);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, tam.y * 0.5f);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_Button, tema->primary);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tema->buttonPrimaryHovered);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, tema->buttonPrimaryActive);
				ImGui::PushStyleColor(ImGuiCol_Text, tema->buttonPrimaryText);
				ImGui::Begin("##phx_web_volver", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
				if (ImGui::Button(texto, tam)) in.panelClasico = false;
				ImGui::End();
				ImGui::PopStyleColor(5);
				ImGui::PopStyleVar(3);
				ImGui::PopFont();
				return;
			}

			if (!PhoenixPrefs::get().interfazWeb || in.avisoCerrado || disponible()) return;
			if (in.anfitrion && (in.anfitrion->estado() == EstadoAnfitrion::Creando || in.anfitrion->estado() == EstadoAnfitrion::Cargando)) return;

			// La interfaz nueva no pudo abrirse: aviso discreto abajo a la derecha.
			const std::string motivo = motivoNoDisponible();
			const bool faltaRuntime = motivo.find("WebView2") != std::string::npos;
			ImGui::PushFont(AppFonts::input);
			const float ancho = 380.0f * s;
			ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - ancho - 18.0f * s, vp->WorkPos.y + vp->WorkSize.y - 150.0f * s), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(ancho, 0), ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * s, 12 * s));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * s);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, tema->panelBackground);
			ImGui::PushStyleColor(ImGuiCol_Border, tema->primary);
			ImGui::Begin("##phx_web_aviso", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
			ImGui::PushStyleColor(ImGuiCol_Text, tema->text);
			ImGui::TextWrapped("%s", T("web.no_disponible"));
			ImGui::PopStyleColor();
			ImGui::PushStyleColor(ImGuiCol_Text, tema->textMuted);
			ImGui::TextWrapped("%s", motivo.c_str());
			ImGui::PopStyleColor();
			ImGui::Dummy(ImVec2(0, 4 * s));
			if (ImGui::Button(T("web.reintentar_nueva"))) {
				in.reintentos++;
				destruirVista(in);
				crearVista(in);
			}
			if (faltaRuntime) {
				ImGui::SameLine();
				if (ImGui::Button(T("web.instalar_webview2"))) {
					try { abrirEnlace("https://developer.microsoft.com/microsoft-edge/webview2/"); }
					catch (...) {}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(T("web.ocultar_aviso"))) in.avisoCerrado = true;
			ImGui::End();
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar(2);
			ImGui::PopFont();
		}
		catch (...) {}
	}

}
