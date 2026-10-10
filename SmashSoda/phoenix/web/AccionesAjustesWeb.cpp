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
#include "../PhoenixPrefs.h"
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
			// Nativos de Smash Soda que solo estaban en el panel clásico (SettingsWidget)
			else if (clave == "autoMute") c.chat.autoMute = booleano(d, "valor");
			else if (clave == "autoMuteTime") c.chat.autoMuteTime = static_cast<unsigned int>(entero(d, "valor", 100, 10000));
			else if (clave == "muteTime") c.chat.muteTime = static_cast<unsigned int>(entero(d, "valor", 1, 1440));
			else if (clave == "hostBonkProof") c.chat.hostBonkProof = booleano(d, "valor");
			else if (clave == "sfxEnabled") c.audio.sfxEnabled = booleano(d, "valor");
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
		p.registrar("ajustes.avisosJuego", [](const json& d, uint64_t) -> std::optional<json> {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.avisosEnJuego = booleano(d, "valor");
			pr.guardar();
			return json::object();
		});

		// Prueba «Solo PES 2021»: off | publicar | estricto
		p.registrar("ajustes.modoPes", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string v = texto(d, "valor", 16);
			if (v != "off" && v != "publicar" && v != "estricto") throw ErrorAccion("VALOR_INVALIDO", "Modo no válido.");
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.modoPes = v;
			pr.guardar();
			return json::object();
		});

		// Phoenix Sync: deshacer la ultima entrega de datos (lo hace el hilo de Link, nunca el de la interfaz)
		p.registrar("sync.deshacerEntrega", [](const json&, uint64_t) -> std::optional<json> {
			PhoenixLink::instancia().deshacerEntrega();
			return json::object();
		});

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

		// ---- Abrir la configuración del overlay (la misma ventana de Ctrl+Alt+F1) ------
		p.registrar("ajustes.overlayMenu", [](const json&, uint64_t) -> std::optional<json> {
			if (!Config::cfg.overlay.enabled) throw ErrorAccion("DATOS_INVALIDOS", "El overlay está apagado. Enciéndelo primero.");
			if (!WebSocket::instance.isRunning()) throw ErrorAccion("DATOS_INVALIDOS", "Abre la sala primero: el overlay solo se conecta cuando la sala está abierta.");
			OverlayService::instance().openMenu();
			return json::object();
		});

		// ---- Sonidos (!sfx) ------------------------------------------------------------
		// ---- Biblioteca de juegos (LibraryWidget): lista, guardar y borrar ------------
		p.registrar("biblioteca.lista", [](const json&, uint64_t) -> std::optional<json> {
			json juegos = json::array();
			for (const GameData& g : Cache::cache.gameList.getGames()) {
				juegos.push_back({ {"id", g.itemID}, {"nombre", g.name}, {"ruta", g.path}, {"parametros", g.parameters} });
			}
			return json{ {"juegos", juegos} };
		});
		p.registrar("biblioteca.guardar", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string nombre = texto(d, "nombre", 128);
			const std::string ruta = texto(d, "ruta", 1024);
			const std::string parametros = d.contains("parametros") ? texto(d, "parametros", 512) : std::string();
			if (nombre.empty() || ruta.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Pon el nombre y la ruta del juego.");
			std::vector<GameData>& juegos = Cache::cache.gameList.getGames();
			const uint32_t id = d.contains("id") ? static_cast<uint32_t>(entero(d, "id", 0, 1000000)) : 0;
			bool editado = false;
			for (GameData& g : juegos) {
				if (id != 0 && g.itemID == id) { g.name = nombre; g.path = ruta; g.parameters = parametros; editado = true; break; }
			}
			if (!editado) {
				uint32_t siguiente = 1;
				for (const GameData& g : juegos) siguiente = (std::max)(siguiente, g.itemID + 1);
				juegos.push_back(GameData(siguiente, nombre, ruta, parametros));
			}
			if (!Cache::cache.gameList.SaveToFile()) throw ErrorAccion("ERROR_INTERNO", "No se pudo guardar la biblioteca.");
			return json::object();
		});
		p.registrar("biblioteca.borrar", [](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = static_cast<uint32_t>(entero(d, "id", 1, 1000000));
			std::vector<GameData>& juegos = Cache::cache.gameList.getGames();
			const auto antes = juegos.size();
			juegos.erase(std::remove_if(juegos.begin(), juegos.end(), [id](const GameData& g) { return g.itemID == id; }), juegos.end());
			if (juegos.size() == antes) throw ErrorAccion("DATOS_INVALIDOS", "Ese juego ya no está.");
			Cache::cache.gameList.SaveToFile();
			return json::object();
		});

		// ---- Atajos de teclado (Ctrl + tecla → comando del chat, como el panel original) ----
		p.registrar("atajos.lista", [](const json&, uint64_t) -> std::optional<json> {
			json lista = json::array();
			for (const Config::Hotkey& h : Config::cfg.hotkeys.keys) lista.push_back({ {"comando", h.command}, {"tecla", h.key}, {"nombre", h.keyName} });
			return json{ {"atajos", lista}, {"activos", Config::cfg.hotkeys.enabled} };
		});
		p.registrar("atajos.agregar", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string comando = texto(d, "comando", 200);
			const int tecla = entero(d, "tecla", 1, 254);
			if (comando.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Escribe el comando (por ejemplo !lockall).");
			const size_t antes = Config::cfg.hotkeys.keys.size();
			Config::cfg.AddHotkey(comando, tecla);
			bool existe = false;
			for (const Config::Hotkey& h : Config::cfg.hotkeys.keys) if (h.key == tecla) existe = true;
			if (!existe && Config::cfg.hotkeys.keys.size() == antes) throw ErrorAccion("NO_SE_PUDO", "Windows no dejó usar Ctrl + esa tecla (otra app ya la usa). Prueba con otra.");
			return json::object();
		});
		p.registrar("atajos.borrar", [](const json& d, uint64_t) -> std::optional<json> {
			const int indice = entero(d, "indice", 0, 200);
			if (indice >= static_cast<int>(Config::cfg.hotkeys.keys.size())) throw ErrorAccion("DATOS_INVALIDOS", "Ese atajo ya no está.");
			Config::cfg.RemoveHotkey(indice);
			return json::object();
		});

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
