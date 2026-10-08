#include "InterfazWebInterno.h"

#include <algorithm>
#include <fstream>

#include <windows.h>

#include "../../Hosting.h"
#include "../../core/Cache.h"
#include "../../core/Config.h"
#include "../../helpers/PathHelper.h"
#include "../../services/WebSocket.h"
#include "../../services/OverlayService.h"
#include "../PhoenixBuild.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

// =============================================================================
//  Acciones de la interfaz web: Ajustes (General, Permisos, Video, Audio,
//  Overlay) y Diagnóstico del PC. Cada clave reproduce el control original.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_ajustes {

		const char* kPosiciones[] = { "top Left", "top center", "top right", "bottom left", "bottom center", "bottom right" };

		bool posicionValida(const std::string& p) {
			for (const char* x : kPosiciones) if (p == x) return true;
			return false;
		}

		/// Versión real de Windows (GetVersionEx miente desde Windows 8.1).
		std::string versionWindows() {
			typedef LONG(WINAPI* FnRtl)(PRTL_OSVERSIONINFOW);
			HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
			if (ntdll == nullptr) return std::string();
			auto fn = reinterpret_cast<FnRtl>(GetProcAddress(ntdll, "RtlGetVersion"));
			if (fn == nullptr) return std::string();
			RTL_OSVERSIONINFOW v{};
			v.dwOSVersionInfoSize = sizeof(v);
			if (fn(&v) != 0) return std::string();
			const char* nombre = v.dwBuildNumber >= 22000 ? "Windows 11" : (v.dwMajorVersion >= 10 ? "Windows 10" : "Windows");
			return std::string(nombre) + " (compilación " + std::to_string(v.dwBuildNumber) + ")";
		}

		bool clavePresente(HKEY raiz, const wchar_t* ruta) {
			HKEY k = nullptr;
			const LONG r = RegOpenKeyExW(raiz, ruta, 0, KEY_READ, &k);
			if (r == ERROR_SUCCESS) RegCloseKey(k);
			return r == ERROR_SUCCESS;
		}

		bool carpetaEscribible() {
			try {
				const std::string ruta = rutaConfig("phoenix-prueba-escritura.tmp");
				{
					std::ofstream f(ruta, std::ios::trunc);
					if (!f) return false;
					f << "ok";
					if (!f.good()) return false;
				}
				DeleteFileA(ruta.c_str());
				return true;
			}
			catch (...) {
				return false;
			}
		}

		/// Sonidos de !sfx: mismo archivo y formato que la pestaña SFX original (sfx.json).
		std::string rutaSfx() {
			const std::string base = PathHelper::GetConfigPath();
			if (base.empty()) throw ErrorAccion("SIN_CARPETA", "No se encontró la carpeta de configuración.");
			return base + "sfx.json";
		}

		json leerSfx() {
			std::ifstream f(rutaSfx(), std::ios::binary);
			if (!f) return json{ {"sfx", json::array()} };
			const json j = json::parse(f, nullptr, false);
			if (j.is_discarded() || !j.is_object() || !j.contains("sfx") || !j["sfx"].is_array()) return json{ {"sfx", json::array()} };
			return j;
		}

		json listaSfx() {
			json lista = json::array();
			for (const json& x : leerSfx()["sfx"]) {
				if (!x.is_object()) continue;
				const std::string ruta = x.value("path", "");
				if (ruta.empty()) continue;
				lista.push_back({ {"ruta", ruta}, {"etiqueta", x.value("tag", ruta)}, {"espera", (std::max)(0, x.value("cooldown", 5))} });
			}
			std::sort(lista.begin(), lista.end(), [](const json& a, const json& b) { return a["etiqueta"].get<std::string>() < b["etiqueta"].get<std::string>(); });
			return lista;
		}

		json chequeo(const char* id, const char* estado, json datos) {
			return { {"id", id}, {"estado", estado}, {"datos", std::move(datos)} };
		}
	}

	using namespace detalle_ajustes;

	void registrarAccionesAjustes(Interno& in) {
		Puente& p = *in.puente;

		// ---- General (como SettingsWidget → General) ---------------------------
		p.registrar("ajustes.general", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string clave = texto(d, "clave", 32);
			Config& c = Config::cfg;
			if (clave == "flashWindow") c.general.flashWindow = booleano(d, "valor");
			else if (clave == "ttsEnabled") c.chat.ttsEnabled = booleano(d, "valor");
			else if (clave == "bonkEnabled") c.chat.bonkEnabled = booleano(d, "valor");
			else if (clave == "messageNotification") c.chat.messageNotification = booleano(d, "valor");
			else if (clave == "disableGuideButton") c.input.disableGuideButton = booleano(d, "valor");
			else if (clave == "disableKeyboard") c.input.disableKeyboard = booleano(d, "valor");
			else if (clave == "autoIndex") c.input.autoIndex = booleano(d, "valor");
			else if (clave == "parsecLogs") c.general.parsecLogs = booleano(d, "valor");
			else if (clave == "ipBan") c.general.ipBan = booleano(d, "valor");
			else if (clave == "blockVPN") c.general.blockVPN = booleano(d, "valor");
			else if (clave == "devMode") c.general.devMode = booleano(d, "valor");
			else if (clave == "chatbot") {
				c.chat.chatbot = texto(d, "valor", 64);
				c.chatbotName = "[" + c.chat.chatbot + "] ";
			}
			else if (clave == "discord") c.chat.discord = texto(d, "valor", 255);
			else if (clave == "welcomeMessage") c.chat.welcomeMessage = texto(d, "valor", 500);
			else if (clave == "socketEnabled") {
				c.socket.enabled = booleano(d, "valor");
				// Se aplica al momento (el panel original pedía reiniciar la app)
				if (c.socket.enabled && !WebSocket::instance.isRunning()) WebSocket::instance.createServer(static_cast<uint16_t>(c.socket.port));
				if (!c.socket.enabled && WebSocket::instance.isRunning()) WebSocket::instance.stopServer();
			}
			else if (clave == "socketPort") {
				c.socket.port = entero(d, "valor", 1024, 65535);
				if (WebSocket::instance.isRunning()) {
					WebSocket::instance.stopServer();
					if (c.socket.enabled) WebSocket::instance.createServer(static_cast<uint16_t>(c.socket.port));
				}
			}
			else throw ErrorAccion("DATOS_INVALIDOS", "Ajuste desconocido.");
			c.Save();
			return json::object();
		});

		// ---- Permisos por rol (SettingsWidget → Permissions) --------------------
		p.registrar("ajustes.permisos", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string grupo = texto(d, "grupo", 16);
			const std::string clave = texto(d, "clave", 32);
			const bool valor = booleano(d, "valor");
			Config::Permissions::PermissionGroup* g = nullptr;
			if (grupo == "guest") g = &Config::cfg.permissions.guest;
			else if (grupo == "vip") g = &Config::cfg.permissions.vip;
			else if (grupo == "moderator") g = &Config::cfg.permissions.moderator;
			if (g == nullptr) throw ErrorAccion("DATOS_INVALIDOS", "Grupo desconocido.");
			if (clave == "useBB") g->useBB = valor;
			else if (clave == "useSFX") g->useSFX = valor;
			else if (clave == "changeControls") g->changeControls = valor;
			else throw ErrorAccion("DATOS_INVALIDOS", "Permiso desconocido.");
			Config::cfg.Save();
			return json::object();
		});

		// ---- Video (VideoWidget) ---------------------------------------------------
		p.registrar("ajustes.video", [&in](const json& d, uint64_t) -> std::optional<json> {
			Hosting& h = hostingObligatorio(in);
			DX11& dx = h.getDX11();
			const std::string clave = texto(d, "clave", 32);
			if (clave == "monitor") {
				const int n = static_cast<int>((std::max)(size_t(1), in.listaPantallas.size()));
				dx.setScreen(static_cast<UINT>(entero(d, "valor", 0, n - 1)));
			}
			else if (clave == "gpu") {
				const int n = static_cast<int>((std::max)(size_t(1), in.listaGpus.size()));
				const UINT pantalla = dx.getScreen();
				dx.setGPU(static_cast<size_t>(entero(d, "valor", 0, n - 1)));
				in.listaPantallas = dx.listScreens(); // los monitores dependen de la GPU
				dx.setScreen(pantalla);
			}
			else if (clave == "captura") {
				const int metodo = entero(d, "valor", 0, 2);
				if (metodo == 2 && !DX11::isWGCSupported()) throw ErrorAccion("NO_SOPORTADO", "Graphics Capture necesita Windows 10 2004 o superior.");
				dx.setCaptureMethod(static_cast<DX11::CaptureMethod>(metodo));
			}
			else if (clave == "resolucion") {
				const int i = entero(d, "valor", 0, static_cast<int>(Config::cfg.resolutions.size()) - 1);
				Config::cfg.video.resolutionIndex = static_cast<unsigned int>(i);
				dx.setTargetResolution(Config::cfg.resolutions[i].width, Config::cfg.resolutions[i].height);
				Config::cfg.Save();
			}
			else if (clave == "lanczos") dx.setLanczosEnabled(booleano(d, "valor"));
			else if (clave == "ritmo") {
				Config::cfg.video.framePacing = booleano(d, "valor");
				Config::cfg.Save();
			}
			else throw ErrorAccion("DATOS_INVALIDOS", "Ajuste de video desconocido.");
			return json::object();
		});

		p.registrar("ajustes.videoListas", [&in](const json&, uint64_t) -> std::optional<json> {
			Hosting& h = hostingObligatorio(in);
			in.listaPantallas = h.getDX11().listScreens();
			in.listaGpus = h.getDX11().listGPUs();
			return json{ {"pantallas", in.listaPantallas}, {"gpus", in.listaGpus}, {"wgc", DX11::isWGCSupported()} };
		});

		// ---- Audio (AudioSettingsWidget) ------------------------------------------
		p.registrar("ajustes.audio", [&in](const json& d, uint64_t) -> std::optional<json> {
			Hosting& h = hostingObligatorio(in);
			const std::string canal = texto(d, "canal", 16);
			const std::string clave = texto(d, "clave", 16);
			const bool mic = canal == "mic";
			if (!mic && canal != "parlantes") throw ErrorAccion("DATOS_INVALIDOS", "Canal desconocido.");
			AudioSource& fuente = mic ? static_cast<AudioSource&>(h.audioIn) : static_cast<AudioSource&>(h.audioOut);
			if (clave == "activo") {
				fuente.isEnabled = booleano(d, "valor");
				if (mic) Config::cfg.audio.micEnabled = fuente.isEnabled;
				else Config::cfg.audio.speakersEnabled = fuente.isEnabled;
			}
			else if (clave == "volumen") {
				const int v = entero(d, "valor", 0, 100);
				fuente.volume = static_cast<float>(v) / 100.0f;
				if (mic) Config::cfg.audio.micVolume = static_cast<float>(v);
				else Config::cfg.audio.speakersVolume = static_cast<float>(v);
			}
			else if (clave == "dispositivo") {
				const int total = static_cast<int>(fuente.getDevices().size());
				if (total == 0) throw ErrorAccion("SIN_DISPOSITIVOS", "No hay dispositivos de audio.");
				const int i = entero(d, "valor", 0, total - 1);
				fuente.setDevice(i);
				if (mic) Config::cfg.audio.inputDevice = static_cast<unsigned int>(i);
				else Config::cfg.audio.outputDevice = static_cast<unsigned int>(i);
			}
			else throw ErrorAccion("DATOS_INVALIDOS", "Ajuste de audio desconocido.");
			Config::cfg.Save();
			return json::object();
		});

		// ---- Overlay (OverlayWidget) -----------------------------------------------
		p.registrar("ajustes.overlay", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string clave = texto(d, "clave", 32);
			Config::Overlay& o = Config::cfg.overlay;
			if (clave == "monitor") {
				const int n = static_cast<int>((std::max)(size_t(1), in.listaPantallas.size()));
				o.monitor = entero(d, "valor", 0, n - 1);
			}
			else if (clave == "tema") {
				const std::string t = texto(d, "valor", 120);
				const bool existe = t.empty() || std::find(Config::cfg.overlayThemes.begin(), Config::cfg.overlayThemes.end(), t) != Config::cfg.overlayThemes.end();
				if (!existe) throw ErrorAccion("DATOS_INVALIDOS", "Ese tema no está en la carpeta overlay/themes.");
				o.theme = t;
			}
			else if (clave == "chat.activo") o.chat.active = booleano(d, "valor");
			else if (clave == "chat.historial") o.chat.showHistory = booleano(d, "valor");
			else if (clave == "mandos.activo") o.gamepads.active = booleano(d, "valor");
			else if (clave == "invitados.activo") o.guests.active = booleano(d, "valor");
			else if (clave == "invitados.latencia") o.guests.showLatency = booleano(d, "valor");
			else if (clave == "chat.posicion" || clave == "mandos.posicion" || clave == "invitados.posicion") {
				const std::string pos = texto(d, "valor", 32);
				if (!posicionValida(pos)) throw ErrorAccion("DATOS_INVALIDOS", "Posición desconocida.");
				if (clave == "chat.posicion") o.chat.position = pos;
				else if (clave == "mandos.posicion") o.gamepads.position = pos;
				else o.guests.position = pos;
			}
			else throw ErrorAccion("DATOS_INVALIDOS", "Ajuste del overlay desconocido.");
			Config::cfg.Save();
			return json::object();
		});

		// ---- Sonidos (!sfx) ------------------------------------------------------------
		p.registrar("sfx.lista", [](const json&, uint64_t) -> std::optional<json> {
			return json{ {"sonidos", listaSfx()} };
		});
		p.registrar("sfx.recargar", [](const json&, uint64_t) -> std::optional<json> {
			Cache::cache.reloadSfxList(); // como «Rescan SFX Folder»
			return json{ {"sonidos", listaSfx()} };
		});
		p.registrar("sfx.espera", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string ruta = texto(d, "ruta", 400);
			const int segundos = entero(d, "segundos", 0, 3600);
			json j = leerSfx();
			bool hallado = false;
			for (json& x : j["sfx"]) {
				if (x.is_object() && x.value("path", "") == ruta) { x["cooldown"] = segundos; hallado = true; }
			}
			if (!hallado) throw ErrorAccion("NO_EXISTE", "Ese sonido ya no está en la lista. Pulsa «Buscar sonidos».");
			{
				std::ofstream f(rutaSfx(), std::ios::binary | std::ios::trunc);
				if (!f) throw ErrorAccion("NO_SE_PUDO_GUARDAR", "No se pudo guardar sfx.json.");
				f << j.dump(4);
				if (!f.good()) throw ErrorAccion("NO_SE_PUDO_GUARDAR", "No se pudo guardar sfx.json.");
			}
			Cache::cache.reloadSfxList();
			return json::object();
		});

		// ---- Diagnóstico del PC ------------------------------------------------------
		p.registrar("diag.ejecutar", [&in](const json&, uint64_t) -> std::optional<json> {
			json lista = json::array();
			const std::string runtime = in.anfitrion ? in.anfitrion->versionRuntime() : AnfitrionWeb::runtimeInstalado();
			lista.push_back(chequeo("webview2", runtime.empty() ? "error" : "ok", { {"version", runtime} }));

			const bool vigem = clavePresente(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\ViGEmBus");
			int mandos = 0, conectados = 0;
			if (in.ctx.sala != nullptr) {
				for (const AsientoVista& a : in.ctx.sala->asientos(8)) { mandos++; if (a.conectado) conectados++; }
			}
			lista.push_back(chequeo("vigem", vigem && mandos > 0 ? "ok" : (vigem ? "aviso" : "error"),
				{ {"instalado", vigem}, {"mandos", mandos}, {"conectados", conectados} }));

			if (in.ctx.hosting != nullptr) {
				Hosting& h = *in.ctx.hosting;
				const bool listo = h.isReady();
				const bool cuenta = h.getHost().isValid();
				lista.push_back(chequeo("parsec", listo && cuenta ? "ok" : (cuenta ? "aviso" : "error"),
					{ {"listo", listo}, {"cuenta", cuenta}, {"nombre", cuenta ? h.getHost().name : std::string()} }));
				lista.push_back(chequeo("video", in.listaGpus.empty() || in.listaPantallas.empty() ? "aviso" : "ok",
					{ {"gpus", in.listaGpus.size()}, {"pantallas", in.listaPantallas.size()}, {"wgc", DX11::isWGCSupported()} }));
				const size_t entradas = h.audioIn.getDevices().size(), salidas = h.audioOut.getDevices().size();
				lista.push_back(chequeo("audio", salidas == 0 ? "aviso" : "ok", { {"entradas", entradas}, {"salidas", salidas} }));
			}
			lista.push_back(chequeo("config", carpetaEscribible() ? "ok" : "error", { {"ruta", rutaConfig("")} }));

			PhoenixLink& link = PhoenixLink::instancia();
			const EstadoLink el = link.estado();
			lista.push_back(chequeo("web", el == EstadoLink::Conectado ? "ok" : (el == EstadoLink::SinVincular ? "aviso" : "error"),
				{ {"usuario", link.usuario()}, {"mensaje", link.mensaje()}, {"eventosEnCola", link.eventosEnCola()}, {"versionLiga", link.versionLiga()} }));

			const bool overlay = Config::cfg.overlay.enabled;
			const bool socket = WebSocket::instance.isRunning();
			lista.push_back(chequeo("overlay", !overlay ? "ok" : (socket ? "ok" : "aviso"),
				{ {"activo", overlay}, {"websocket", socket}, {"puerto", Config::cfg.socket.port} }));

			lista.push_back(chequeo("sistema", "ok", { {"windows", versionWindows()}, {"app", kVersion}, {"soda", Cache::cache.version} }));
			return json{ {"chequeos", lista} };
		});
	}

}
