#include "InterfazWebInterno.h"

#include <algorithm>
#include <cmath>
#include <string>
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
		int gReintentosAuto = 0;
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
			if ((est == EstadoAnfitrion::Creando || est == EstadoAnfitrion::Cargando || est == EstadoAnfitrion::Listo) && a.segundosDesdeInicio() > 30.0
				&& !(in.puente && in.puente->listo())) {
				// Primer arranque lento (WebView2 prepara su perfil) o página sin «hola»: reintenta solo dos veces.
				if (gReintentosAuto < 2) {
					gReintentosAuto++;
					destruirVista(in);
					crearVista(in);
					return;
				}
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

	namespace { void* gLogoCarga = nullptr; }
	void InterfazWeb::fijarLogoCarga(void* textura) { gLogoCarga = textura; }

	bool InterfazWeb::cargando() const {
		const Interno& in = interno();
		if (!in.iniciada || in.panelClasico || in.anfitrion == nullptr) return false;
		if (!PhoenixPrefs::get().interfazWeb) return false;
		const EstadoAnfitrion e = in.anfitrion->estado();
		// Solo mientras arranca; si falla o se apaga, se vuelve a la interfaz de siempre
		return (e == EstadoAnfitrion::Creando || e == EstadoAnfitrion::Cargando || e == EstadoAnfitrion::Listo) && !in.anfitrion->visible();
	}

	namespace {
		// Número «al azar» fijo para cada n (sin estado): sirve para colocar estrellas y brasas siempre igual.
		float azarFijo(int n) {
			unsigned x = static_cast<unsigned>(n) * 747796405u + 2891336453u;
			x = ((x >> ((x >> 28) + 4)) ^ x) * 277803737u;
			x = (x >> 22) ^ x;
			return static_cast<float>(x & 0xFFFFFFu) / 16777216.0f;
		}
		ImU32 color4(float r, float g, float b, float a) {
			auto c = [](float v) { return static_cast<int>((std::max)(0.0f, (std::min)(1.0f, v)) * 255.0f + 0.5f); };
			return IM_COL32(c(r), c(g), c(b), c(a));
		}
	}

	void InterfazWeb::renderCarga() {
		if (ImGui::GetCurrentContext() == nullptr) return;
		try {
			const ImGuiViewport* vp = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(vp->WorkSize, ImGuiCond_Always);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.005f, 0.005f, 0.02f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::Begin("##phx_carga", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 o = vp->WorkPos;
			const float W = vp->WorkSize.x, H = vp->WorkSize.y;
			const float t = static_cast<float>(ImGui::GetTime());
			const float esc = (std::max)(1.0f, H / 800.0f);
			const float lado = (std::min)(W, H) * 0.26f;
			const ImVec2 c(o.x + W * 0.5f, o.y + H * 0.5f - H * 0.03f);
			const float R = (std::max)(W, H);
			const float PI2 = 6.2831853f;

			// Tiempo desde que empezó esta pantalla (para que todo «emerja» de la oscuridad)
			static float ultima = -100.0f, inicio = 0.0f;
			if (t - ultima > 0.5f) inicio = t;
			ultima = t;
			const float dentro = t - inicio;
			auto suave = [](float a, float b, float x) { float k = (std::max)(0.0f, (std::min)(1.0f, (x - a) / (b - a))); return k * k * (3.0f - 2.0f * k); };

			// Latido: dos golpes seguidos («tum-tum») cada 2,6 s, como algo vivo dormido
			const float periodo = 2.6f;
			const float p = fmodf(t, periodo) / periodo;
			const float g1 = p * 14.0f, g2 = (p - 0.16f) * 14.0f;
			const float latido = expf(-g1 * g1) + 0.6f * expf(-g2 * g2);

			// 1) Fondo casi negro que baja a un índigo muy oscuro
			dl->AddRectFilledMultiColor(o, ImVec2(o.x + W, o.y + H),
				color4(0.005f, 0.005f, 0.02f, 1), color4(0.005f, 0.005f, 0.02f, 1), color4(0.045f, 0.02f, 0.10f, 1), color4(0.045f, 0.02f, 0.10f, 1));

			// 2) Niebla lenta que se mueve entre las sombras
			for (int k = 0; k < 7; k++) {
				const float fk = static_cast<float>(k);
				const float cx = o.x + W * (0.5f + 0.42f * sinf(t * (0.035f + 0.012f * fk) + fk * 1.7f));
				const float cy = o.y + H * (0.58f + 0.28f * sinf(t * (0.028f + 0.010f * fk) + fk * 2.3f));
				const bool fria = (k % 2) == 0;
				for (int l = 0; l < 6; l++) {
					const float r = R * (0.10f + 0.07f * static_cast<float>(l)) * (0.8f + 0.1f * fk * 0.3f);
					dl->AddCircleFilled(ImVec2(cx, cy), r, fria ? color4(0.10f, 0.35f, 0.55f, 0.010f) : color4(0.40f, 0.12f, 0.70f, 0.012f), 48);
				}
			}

			// 3) Resplandor oscuro detrás del logo, que se enciende con cada latido
			const float brillo = 0.30f + 0.70f * (std::min)(1.0f, latido);
			for (int k = 0; k < 9; k++) {
				const float f = 1.0f - static_cast<float>(k) / 9.0f;
				const float r = lado * (0.45f + static_cast<float>(k) * 0.20f) * (1.0f + 0.025f * latido);
				dl->AddCircleFilled(c, r, k < 3 ? color4(0.30f, 0.75f, 1.0f, 0.05f * f * brillo) : color4(0.50f, 0.20f, 0.95f, 0.055f * f * brillo), 64);
			}

			// 4) Una sola onda tenue por latido
			{
				const float r = lado * 0.55f + p * R * 0.50f;
				const float a = (1.0f - p) * (1.0f - p) * (1.0f - p) * 0.22f * suave(0.8f, 2.5f, dentro);
				dl->AddCircle(c, r, color4(0.40f, 0.80f, 1.0f, a), 96, 1.5f * esc);
			}

			// 5) Arcos finos que giran en sentidos opuestos alrededor del logo (como un sello)
			{
				const float aparece = suave(1.0f, 3.0f, dentro);
				for (int k = 0; k < 3; k++) {
					const float a0 = t * 0.10f + static_cast<float>(k) * (PI2 / 3.0f);
					dl->PathArcTo(c, lado * 0.74f, a0, a0 + 0.9f, 40);
					dl->PathStroke(color4(0.45f, 0.85f, 1.0f, 0.22f * aparece), 0, 1.2f * esc);
					const float b0 = -t * 0.07f + static_cast<float>(k) * (PI2 / 3.0f) + 0.6f;
					dl->PathArcTo(c, lado * 0.86f, b0, b0 + 0.5f, 32);
					dl->PathStroke(color4(0.65f, 0.40f, 1.0f, 0.18f * aparece), 0, 1.0f * esc);
				}
			}

			// 6) Pocas brasas, muy lentas; casi todas frías y alguna cálida, brillan un poco con el latido
			{
				const float alto = H * 1.1f;
				for (int i = 0; i < 30; i++) {
					const float vel = (9.0f + azarFijo(i * 5 + 1) * 24.0f) * esc;
					const float dist = fmodf(t * vel + azarFijo(i * 5 + 2) * alto, alto);
					const float y = o.y + H - dist;
					const float x = o.x + azarFijo(i * 5 + 3) * W + sinf(t * (0.25f + azarFijo(i * 5 + 4) * 0.4f) + static_cast<float>(i)) * 22.0f * esc;
					const float vida = dist / alto;
					const float a = sinf(vida * 3.14159f) * 0.55f * (0.75f + 0.45f * (std::min)(1.0f, latido)) * suave(0.3f, 2.0f, dentro);
					const float r = (0.8f + azarFijo(i * 5 + 5) * 1.5f) * esc;
					const bool calida = (i % 7) == 0;
					const float cr = calida ? 1.0f : 0.45f, cg = calida ? 0.50f : 0.80f, cb = calida ? 0.25f : 1.0f;
					dl->AddCircleFilled(ImVec2(x, y), r * 3.0f, color4(cr, cg, cb, a * 0.12f), 12);
					dl->AddCircleFilled(ImVec2(x, y), r, color4(cr, cg, cb, a), 8);
				}
			}

			// 7) Viñeta: los bordes se hunden en la oscuridad
			{
				const ImU32 negro = color4(0.0f, 0.0f, 0.01f, 0.88f), nada = color4(0.0f, 0.0f, 0.01f, 0.0f);
				const float bh = H * 0.38f, bw = W * 0.28f;
				dl->AddRectFilledMultiColor(o, ImVec2(o.x + W, o.y + bh), negro, negro, nada, nada);
				dl->AddRectFilledMultiColor(ImVec2(o.x, o.y + H - bh), ImVec2(o.x + W, o.y + H), nada, nada, negro, negro);
				dl->AddRectFilledMultiColor(o, ImVec2(o.x + bw, o.y + H), negro, nada, nada, negro);
				dl->AddRectFilledMultiColor(ImVec2(o.x + W - bw, o.y), ImVec2(o.x + W, o.y + H), nada, negro, negro, nada);
			}

			// 8) El logo emerge despacio de la oscuridad y «late»
			ImGui::PushFont(AppFonts::label);
			const char* nombre = "PHOENIX LINK";
			const float altoTxt = ImGui::CalcTextSize("A").y;
			float yTexto = c.y;
			if (gLogoCarga != nullptr) {
				const float entra = suave(0.4f, 3.0f, dentro);
				const float l = lado * (0.94f + 0.06f * entra) * (1.0f + 0.012f * latido);
				const float alfa = entra * (0.90f + 0.10f * (std::min)(1.0f, latido));
				const ImVec2 a(c.x - l * 0.5f, c.y - l * 0.5f - altoTxt * 1.2f);
				dl->AddImage(reinterpret_cast<ImTextureID>(gLogoCarga), a, ImVec2(a.x + l, a.y + l), ImVec2(0, 0), ImVec2(1, 1),
					IM_COL32(255, 255, 255, static_cast<int>(alfa * 255.0f)));
				yTexto = a.y + l + altoTxt * 0.9f;
			}

			// 9) Nombre con letras muy separadas, tenue; y una línea fina con una luz que la recorre
			{
				const float sep = altoTxt * 0.45f;
				float ancho = 0.0f;
				for (const char* q = nombre; *q; ++q) ancho += ImGui::CalcTextSize(std::string(1, *q).c_str()).x + sep;
				ancho -= sep;
				const float aT = suave(1.6f, 3.4f, dentro);
				float x = c.x - ancho * 0.5f;
				for (const char* q = nombre; *q; ++q) {
					const std::string s(1, *q);
					dl->AddText(ImVec2(x, yTexto), color4(0.65f, 0.82f, 0.95f, 0.65f * aT), s.c_str());
					x += ImGui::CalcTextSize(s.c_str()).x + sep;
				}
				const float yl = yTexto + altoTxt * 1.7f;
				const float mitad = lado * 0.55f;
				dl->AddLine(ImVec2(c.x - mitad, yl), ImVec2(c.x + mitad, yl), color4(0.5f, 0.7f, 0.9f, 0.14f * aT), 1.0f);
				const float k = fmodf(t * 0.30f, 1.0f);
				const float cx = c.x - mitad + 2.0f * mitad * k;
				const float tr = mitad * 0.28f;
				dl->AddRectFilledMultiColor(ImVec2((std::max)(c.x - mitad, cx - tr), yl - 1.0f), ImVec2(cx, yl + 1.0f),
					color4(0.5f, 0.85f, 1.0f, 0.0f), color4(0.5f, 0.85f, 1.0f, 0.75f * aT), color4(0.5f, 0.85f, 1.0f, 0.75f * aT), color4(0.5f, 0.85f, 1.0f, 0.0f));
			}
			ImGui::PopFont();
			ImGui::End();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
		catch (...) {}
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
