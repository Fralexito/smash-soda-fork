#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../AudioTools.h"
#include "../../core/Cache.h"
#include "../../core/Config.h"
#include "../../services/Hotseat.h"
#include "../../services/WebSocket.h"
#include "../host/CampoVivo.h"
#include "../../services/OverlayService.h"
#include "../../widgets/HostSettingsWidget.h"
#include "../PhoenixBuild.h"
#include "../PhoenixPrefs.h"
#include "../PhoenixRoles.h"
#include "../I18n.h"
#include "../core/ProveedorSala.h"
#include "../core/Solicitudes.h"
#include "../core/MandoHost.h"
#include "../core/OrdenPes.h"
#include "../core/PerfilesMapeo.h"
#include "../../core/MasterOfPuppets.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_estado {

		/// Hora en que se compiló el PhoenixLink.exe abierto (fecha de escritura del propio archivo).
		std::string compiladoEn() {
			static std::string cache;
			if (!cache.empty()) return cache;
			wchar_t ruta[MAX_PATH];
			if (GetModuleFileNameW(nullptr, ruta, MAX_PATH) == 0) return cache;
			WIN32_FILE_ATTRIBUTE_DATA a{};
			if (!GetFileAttributesExW(ruta, GetFileExInfoStandard, &a)) return cache;
			FILETIME local{}; SYSTEMTIME st{};
			if (!FileTimeToLocalFileTime(&a.ftLastWriteTime, &local) || !FileTimeToSystemTime(&local, &st)) return cache;
			char b[32];
			snprintf(b, sizeof(b), "%02d/%02d %02d:%02d", st.wDay, st.wMonth, st.wHour, st.wMinute);
			cache = b;
			return cache;
		}
		/// «Tu mando»: nombre del mando físico que eligió el anfitrión ("" = ninguno, usa el teclado).
		std::string nombreMiMando() {
			try {
				MasterOfPuppets& mp = MasterOfPuppets::instance;
				std::lock_guard<std::mutex> cierre(mp.inputMutex);
				const int i = mp.getMasterIndex();
				if (i < 0) return std::string();
				if (mp.isSDLEngine) {
					std::vector<SDLGamepad>& v = mp.getSDLGamepads();
					if (i < static_cast<int>(v.size()) && v[i].joystick != nullptr) {
						const char* n = SDL_JoystickName(v[i].joystick);
						return n != nullptr ? n : "Mando";
					}
					return std::string();
				}
				return "Mando " + std::to_string(i + 1);
			}
			catch (...) { return std::string(); }
		}


		const char* textoEstadoLink(EstadoLink e) {
			switch (e) {
			case EstadoLink::Conectado: return "conectado";
			case EstadoLink::Vinculando: return "vinculando";
			case EstadoLink::SinConexion: return "sin_conexion";
			case EstadoLink::Pausado: return "pausado";
			default: return "sin_vincular";
			}
		}

		const char* textoRol(Rol r) {
			switch (r) {
			case Rol::Jugador: return "jugador";
			case Rol::Espectador: return "espectador";
			case Rol::NoListado: return "no_listado";
			default: return "sin_lista";
			}
		}

		/// Enlace de la sala: se recalcula como mucho una vez por segundo.
		std::string enlaceCacheado(ProveedorSala& sala, bool abierta) {
			static std::string cache;
			static double en = -10.0;
			const double ahora = ahoraSeg();
			if (!abierta) { cache.clear(); return cache; }
			if (ahora - en > 1.0 || cache.empty()) {
				cache = sala.enlace();
				en = ahora;
			}
			return cache;
		}

		/// Ping de cada invitado presente (con mando o mirando).
		/// Plan de energia de Windows: "ahorro" | "ok" | "?" (se consulta como mucho una vez por minuto)
		std::string planEnergia() {
			static std::string cache = "?";
			static ULONGLONG hasta = 0;
			const ULONGLONG ahora = GetTickCount64();
			if (ahora < hasta) return cache;
			hasta = ahora + 60000;
			try {
				HMODULE h = LoadLibraryW(L"powrprof.dll");
				if (h) {
					typedef DWORD(WINAPI* Fn)(HKEY, GUID**);
					Fn f = (Fn)GetProcAddress(h, "PowerGetActiveScheme");
					GUID* g = nullptr;
					if (f && f(NULL, &g) == 0 && g) {
						static const GUID ahorro = { 0xa1841308, 0x3541, 0x4fab, { 0xbc, 0x81, 0xf7, 0x15, 0x56, 0xf2, 0x0b, 0x4a } };
						cache = memcmp(g, &ahorro, sizeof(GUID)) == 0 ? "ahorro" : "ok";
						LocalFree(g);
					}
					FreeLibrary(h);
				}
			}
			catch (...) {}
			return cache;
		}

		std::map<uint32_t, std::pair<std::string, int>> pingsPresentes(ProveedorSala& sala) {
			std::map<uint32_t, std::pair<std::string, int>> r;
			for (const AsientoVista& a : sala.asientos(8)) {
				if (a.ocupado && a.parsecId != 0) r[a.parsecId] = { a.jugador, a.pingMs };
			}
			for (const EspectadorVista& e : sala.espectadores()) {
				if (e.parsecId != 0 && !r.count(e.parsecId)) r[e.parsecId] = { e.nombre, e.pingMs };
			}
			return r;
		}

		json lineas(const std::vector<std::string>& todas, size_t desde, size_t maximo) {
			json arr = json::array();
			const size_t inicio = todas.size() > desde ? desde : todas.size();
			const size_t primero = todas.size() - inicio > maximo ? todas.size() - maximo : inicio;
			for (size_t i = primero; i < todas.size(); i++) arr.push_back(todas[i]);
			return arr;
		}

		json estadoAudio(Hosting& h) {
			json r = json::object();
			auto canal = [&](AudioSource& fuente, bool esMic) {
				json lista = json::array();
				for (const AudioSourceDevice& d : fuente.getDevices()) lista.push_back(d.name);
				const int db = h.isRunning() ? fuente.peekPreviewDecibel() : fuente.popPreviewDecibel();
				return json{
					{"activo", fuente.isEnabled},
					{"volumen", static_cast<int>(fuente.volume * 100.0f + 0.5f)},
					{"nivel", AudioTools::decibelToFloat(db)},
					{"dispositivo", static_cast<int>(fuente.currentDevice.index)},
					{"dispositivos", lista},
					{"mic", esMic},
				};
			};
			r["mic"] = canal(h.audioIn, true);
			r["parlantes"] = canal(h.audioOut, false);
			return r;
		}

		json estadoTurnos() {
			json usuarios = json::array();
			try {
				for (Hotseat::HotseatUser& u : Hotseat::instance.users) {
					std::string restante, enfriamiento;
					try {
						if (u.cooldown) enfriamiento = Hotseat::instance.getCooldownRemaining(u.userId);
						else if (u.stopwatch != nullptr) restante = u.stopwatch->getRemainingTime();
					}
					catch (...) {}
					usuarios.push_back({
						{"parsecId", u.userId}, {"nombre", u.userName}, {"enAsiento", u.inSeat},
						{"corriendo", u.stopwatch != nullptr && u.stopwatch->isRunning()},
						{"enfriamiento", u.cooldown}, {"restante", restante}, {"restanteEnfriamiento", enfriamiento},
					});
				}
			}
			catch (...) {}
			return {
				{"activo", Config::cfg.hotseat.enabled},
				{"corriendo", Hotseat::instance.running},
				{"usuarios", usuarios},
				{"juegoMin", Config::cfg.hotseat.playTime},
				{"reinicioMin", Config::cfg.hotseat.resetTime},
				{"recordatorioMin", Config::cfg.hotseat.reminderInterval},
			};
		}

		json estadoAjustes(Interno& in) {
			const Config& c = Config::cfg;
			json ajustes = {
				{"general", {
					{"flashWindow", c.general.flashWindow}, {"ttsEnabled", c.chat.ttsEnabled}, {"bonkEnabled", c.chat.bonkEnabled},
					{"messageNotification", c.chat.messageNotification},
					{"disableGuideButton", c.input.disableGuideButton}, {"disableKeyboard", c.input.disableKeyboard},
					{"autoIndex", c.input.autoIndex}, {"parsecLogs", c.general.parsecLogs}, {"ipBan", c.general.ipBan},
					{"blockVPN", c.general.blockVPN}, {"devMode", c.general.devMode},
					{"chatbot", c.chat.chatbot}, {"discord", c.chat.discord}, {"welcomeMessage", c.chat.welcomeMessage},
					{"socketEnabled", c.socket.enabled}, {"socketPort", c.socket.port},
					{"socketActivo", WebSocket::instance.isRunning()},
					{"autoMute", c.chat.autoMute}, {"autoMuteTime", c.chat.autoMuteTime}, {"muteTime", c.chat.muteTime},
					{"hostBonkProof", c.chat.hostBonkProof}, {"sfxEnabled", c.audio.sfxEnabled},
				}},
				{"permisos", {
					{"guest", { {"useBB", c.permissions.guest.useBB}, {"useSFX", c.permissions.guest.useSFX}, {"changeControls", c.permissions.guest.changeControls} }},
					{"vip", { {"useBB", c.permissions.vip.useBB}, {"useSFX", c.permissions.vip.useSFX}, {"changeControls", c.permissions.vip.changeControls} }},
					{"moderator", { {"useBB", c.permissions.moderator.useBB}, {"useSFX", c.permissions.moderator.useSFX}, {"changeControls", c.permissions.moderator.changeControls} }},
				}},
				{"video", {
					{"monitor", c.video.monitor}, {"gpu", c.video.adapter}, {"captura", c.video.captureMethod},
					{"resolucion", c.video.resolutionIndex}, {"lanczos", c.video.lanczos}, {"ritmo", c.video.framePacing},
					{"fps", c.video.fps}, {"mbps", c.video.bandwidth},
				}},
				{"overlay", {
					{"monitor", c.overlay.monitor}, {"tema", c.overlay.theme},
					{"chat", { {"activo", c.overlay.chat.active}, {"historial", c.overlay.chat.showHistory}, {"posicion", c.overlay.chat.position} }},
					{"mandos", { {"activo", c.overlay.gamepads.active}, {"posicion", c.overlay.gamepads.position} }},
					{"invitados", { {"activo", c.overlay.guests.active}, {"latencia", c.overlay.guests.showLatency}, {"posicion", c.overlay.guests.position} }},
					{"corriendo", OverlayService::instance().isRunning()},
				}},
			};
			(void)in;
			return ajustes;
		}
	}

	using namespace detalle_estado;

	// =========================================================================
	json construirEstado(Interno& in) {
		const PhoenixPrefs& pr = PhoenixPrefs::get();
		json e;
		e["v"] = 1;
		e["app"] = {
			{"version", kVersion}, {"compilado", detalle_estado::compiladoEn()}, {"idioma", I18n::actual()}, {"tema", pr.tema},
			{"dev", Config::cfg.general.devMode}, {"soda", Cache::cache.version},
		};
		PhoenixLink& link = PhoenixLink::instancia();
		e["web"] = {
			{"estado", textoEstadoLink(link.estado())}, {"usuario", link.usuario()}, {"mensaje", link.mensaje()},
			{"publicada", link.salaPublicada()}, {"jugadoresLista", link.jugadoresEnLista()},
			{"versionLiga", link.versionLiga()}, {"eventosEnCola", link.eventosEnCola()},
		};
		e["ajustes"] = estadoAjustes(in);
		e["cedido"] = estadoCedido(in);

		if (in.ctx.sala == nullptr || in.ctx.hosting == nullptr) {
			e["sala"] = nullptr;
			return e;
		}
		ProveedorSala& sala = *in.ctx.sala;
		Hosting& h = *in.ctx.hosting;
		const double ahora = ahoraSeg();

		// ---- Sala ------------------------------------------------------------
		const bool abierta = sala.abierta();
		json s;
		s["abierta"] = abierta;
		s["lista"] = h.isReady();
		s["enlace"] = enlaceCacheado(sala, abierta);
		s["nombre"] = sala.nombreSala();
		s["plazas"] = sala.plazas();
		s["invitados"] = sala.totalInvitados();
		s["cuentaHost"] = sala.cuentaHost();
		{
			Guest& host = h.getHost();
			s["hostId"] = host.isValid() ? host.userID : 0u;
			s["hostNombre"] = host.isValid() ? host.name : std::string();
		}
		s["segundos"] = in.salaAbiertaDesde >= 0.0 ? static_cast<int>(ahora - in.salaAbiertaDesde) : -1;
		if (in.ctx.ajustesSala != nullptr) {
			std::string nombre, biblioteca;
			int plazas = 0, limite = 0;
			bool limitador = false, pendiente = false;
			in.ctx.ajustesSala->phoenixLeerOpciones(nombre, plazas, limitador, limite, biblioteca, pendiente);
			json juegos = json::array();
			for (const GameData& g : Cache::cache.gameList.getGames()) juegos.push_back(g.name);
			s["opciones"] = {
				{"nombre", nombre}, {"plazas", plazas}, {"limitador", limitador}, {"limite", limite},
				{"biblioteca", biblioteca}, {"juegos", juegos}, {"pendiente", pendiente},
				{"turnos", Config::cfg.hotseat.enabled}, {"quiosco", Config::cfg.kioskMode.enabled},
				{"overlay", Config::cfg.overlay.enabled},
			};
		}
		s["phoenix"] = {
			{"visibilidad", pr.visibilidad}, {"espectadores", pr.espectadores}, {"limiteEspectadores", pr.limiteEspectadores},
			{"entradaParsec", pr.entradaParsec}, {"juego", pr.juego}, {"parche", pr.parche}, {"region", pr.region},
		};
		s["calidad"] = { {"fps", Config::cfg.video.fps}, {"mbps", Config::cfg.video.bandwidth},
			{"auto", PhoenixPrefs::get().anchoAuto}, {"porPersona", PhoenixPrefs::get().anchoPorPersona},
			{"subida", PhoenixPrefs::get().subidaMbps}, {"perdida", g_perdidaPorMil.load()}, {"energia", planEnergia()} };
		s["sesion"] = { {"pico", in.picoInvitados}, {"entradas", in.entradasSesion}, {"partidos", in.partidosSesion} };
		e["sala"] = s;

		// ---- Mandos ----------------------------------------------------------
		const std::vector<AsientoVista> asientos = sala.asientos(8);
		std::map<uint32_t, int> mandoDe;
		json lista = json::array();
		{
			GamepadClient& gc = h.getGamepadClient();
			std::vector<std::string> tipos;
			for (AGamepad* p : gc.gamepads) {
				if (p == nullptr) continue;
				tipos.push_back(p->type() == AGamepad::Type::DUALSHOCK ? "ds4" : "xbox");
			}
			// El mando que tomó el anfitrión (MandoHost) no tiene «dueño» en el motor: se publica como suyo,
			// igual que en el simulador, para que salga con su nombre en Puestos, Resumen y Partido.
			const int mandoHost = MandoHost::activo();
			Guest& anfitrion = h.getHost();
			for (size_t i = 0; i < asientos.size(); i++) {
				AsientoVista a = asientos[i];
				const int n = a.numero;
				if (n == mandoHost && !a.ocupado && anfitrion.isValid()) {
					a.ocupado = true;
					a.jugador = anfitrion.name;
					a.parsecId = anfitrion.userID;
					a.pingMs = -1;
				}
				if (n == mandoHost) a.bloqueado = false;   // el bloqueo interno de «tomar mando» no es para el host
				const char* equipo = n <= pr.equipoLocal ? "local" : (n <= pr.mandosActivos ? "visitante" : "fuera");
				if (a.ocupado && a.parsecId != 0) mandoDe[a.parsecId] = n;
				lista.push_back({
					{"n", n}, {"conectado", a.conectado}, {"ocupado", a.ocupado}, {"bloqueado", a.bloqueado},
					{"jugador", a.jugador}, {"parsecId", a.parsecId}, {"ping", a.pingMs}, {"equipo", equipo},
					{"tipo", i < tipos.size() ? tipos[i] : std::string("xbox")},
				});
				// Mando en vivo (solo mirando Mandos › Puestos: si no, el estado cambiaría con cada botón)
				if (in.seccion == "mandos" && (in.pestana == "puestos" || in.pestana.empty()) && a.conectado) {
					lista.back()["entrada"] = { {"b", a.botones}, {"lt", a.gatilloI}, {"rt", a.gatilloD},
						{"lx", a.ejeLX}, {"ly", a.ejeLY}, {"rx", a.ejeRX}, {"ry", a.ejeRY} };
				}
			}
			// Orden de mandos en PES (EXPERIMENTAL): mandos siempre enchufados en orden y su serie al módulo.
			{
				const bool quiere = pr.ordenPes;
				if (phoenix::ordenpes::estado().activo != quiere) phoenix::ordenpes::activar(quiere);
				if (quiere && !in.reiniciandoMandos) {
					static ULONGLONG ultimoEnchufe = 0;
					const ULONGLONG ahora = GetTickCount64();
					bool falta = false;
					for (size_t i = 0; i < gc.gamepads.size() && i < 8; i++) if (gc.gamepads[i] && !gc.gamepads[i]->isConnected()) falta = true;
					if (falta && ahora - ultimoEnchufe > 10000) {
						ultimoEnchufe = ahora;
						for (size_t i = 0; i < gc.gamepads.size() && i < 8; i++) if (gc.gamepads[i] && !gc.gamepads[i]->isConnected()) gc.gamepads[i]->connect();
					}
				}
				std::vector<unsigned long> series;
				for (size_t i = 0; i < gc.gamepads.size() && i < 8; i++) series.push_back(gc.gamepads[i] ? gc.gamepads[i]->serieBus() : 0);
				phoenix::ordenpes::publicarSeries(series);
			}
			json ordenPes;
			{
				const phoenix::ordenpes::Estado op = phoenix::ordenpes::estado();
				json casillas = json::array();
				for (const auto& c : op.casillas) casillas.push_back({ {"n", c.n}, {"puesto", c.puesto}, {"ok", c.ok} });
				ordenPes = { {"activo", op.activo}, {"estado", op.estado}, {"casillas", casillas}, {"ultimo", op.ultimo}, {"copia", op.copia}, {"error", op.error}, {"ds4windows", op.ds4windows} };
			}
			e["mandos"] = {
				{"lista", lista},
				{"ordenPes", ordenPes},
				{"formacion", { {"local", pr.equipoLocal}, {"visitante", pr.mandosActivos - pr.equipoLocal} }},
				{"host", MandoHost::activo()},
				{"miMando", detalle_estado::nombreMiMando()},
				{"miPuesto", { {"modo", pr.miPuestoModo}, {"numero", pr.miPuestoNumero}, {"alAbrir", pr.sentarmeAlAbrir}, {"pulsando", pr.entrarPulsando}, {"mandoRecordado", !pr.miMando.empty()} }},
				{"bloqueoGlobal", gc.lock}, {"bloqueoBotones", bloqueoBotonesPropio(in)}, {"esclavo", gc.isSlave},
				{"xbox", Config::cfg.input.xboxPuppetCount}, {"ds4", Config::cfg.input.ds4PuppetCount},
				{"reiniciando", in.reiniciandoMandos},
				// Lo del anfitrión, sin lo que añade el modo competitivo mientras dura el partido
				{"botonesBloq", in.arbitro.competitivoActivo ? json{
					{"mascara", mascaraBotonesPropia(in)},
					{"lt", Config::cfg.input.lockedGamepadLeftTrigger}, {"rt", Config::cfg.input.lockedGamepadRightTrigger},
					{"lx", Config::cfg.input.lockedGamepadLX}, {"ly", Config::cfg.input.lockedGamepadLY},
					{"rx", Config::cfg.input.lockedGamepadRX}, {"ry", Config::cfg.input.lockedGamepadRY},
				} : json{
					{"mascara", static_cast<unsigned int>(h._lockedGamepad.wButtons)},
					{"lt", h._lockedGamepad.bLeftTrigger}, {"rt", h._lockedGamepad.bRightTrigger},
					{"lx", h._lockedGamepad.sThumbLX}, {"ly", h._lockedGamepad.sThumbLY},
					{"rx", h._lockedGamepad.sThumbRX}, {"ry", h._lockedGamepad.sThumbRY},
				}},
			};

			// Mapeo por persona: perfiles siempre (pocos); lo que llega/sale solo mirando Mandos › Mapeo
			{
				const bool enMapeo = in.seccion == "mandos" && in.pestana == "mapeo";
				phoenix::mapeo::observar(enMapeo ? in.mapeoVer : 0);
				e["mapeo"] = { {"perfiles", phoenix::mapeo::listar()}, {"vista", enMapeo ? phoenix::mapeo::vista() : json(nullptr)} };
			}

			// Teclado: perfiles de teclas (qué tecla es cada botón del mando)
			if (in.seccion == "mandos" && in.pestana == "teclado") {
				KeyboardMap& km = gc.getKeyMap();
				json perfiles = json::array();
				for (const KeyboardProfile& pf : km.profiles) {
					json teclas = json::array();
					for (size_t i = 0; i < km.buttonNamesLower.size() && i < pf.keyMap.size(); i++) {
						const uint16_t v = pf.keyMap[i];
						teclas.push_back({ {"b", km.buttonNamesLower[i]}, {"v", static_cast<int>(v)}, {"e", v != 0 ? KeyboardMapsUtil::getKeyToString(v) : std::string()} });
					}
					perfiles.push_back({ {"userId", static_cast<int64_t>(pf.userID)}, {"nombre", pf.name}, {"teclas", teclas} });
				}
				e["teclado"] = { {"perfiles", perfiles} };
			}

			// Marionetas: un mando físico (maestro) maneja los mandos virtuales marcados (títeres)
			if (in.seccion == "mandos" && in.pestana == "marionetas") {
				MasterOfPuppets& mp = MasterOfPuppets::instance;
				std::lock_guard<std::mutex> cierre(mp.inputMutex);
				json maestros = json::array();
				int maestro = mp.getMasterIndex();
				if (mp.isSDLEngine) {
					std::vector<SDLGamepad>& v = mp.getSDLGamepads();
					for (size_t i = 0; i < v.size(); i++) {
						const char* nombre = v[i].joystick != nullptr ? SDL_JoystickName(v[i].joystick) : nullptr;
						const char* tipo = v[i].type == SDLGamepad::Type::DS4 ? "ds4" : (v[i].type == SDLGamepad::Type::DS ? "dualshock" : "xbox");
						const bool activo = v[i].getGamepadState().state.Gamepad.wButtons != 0;
						maestros.push_back({ {"n", static_cast<int>(i) + 1}, {"nombre", nombre != nullptr ? nombre : "Mando"}, {"tipo", tipo}, {"activo", activo} });
					}
					if (maestro >= static_cast<int>(v.size())) maestro = -1;
				}
				else {
					std::vector<GamepadState>& v = mp.getXInputGamepads();
					for (size_t i = 0; i < v.size(); i++) {
						maestros.push_back({ {"n", static_cast<int>(i) + 1}, {"nombre", "Mando " + std::to_string(i + 1)}, {"tipo", "xinput"}, {"activo", v[i].state.Gamepad.wButtons != 0} });
					}
					if (maestro >= static_cast<int>(v.size())) maestro = -1;
				}
				json titeres = json::array();
				for (size_t i = 0; i < gc.gamepads.size(); i++) {
					if (gc.gamepads[i] == nullptr) continue;
					titeres.push_back({ {"n", static_cast<int>(i) + 1}, {"activo", gc.gamepads[i]->isPuppet} });
				}
				e["marionetas"] = { {"motor", mp.isSDLEngine ? "sdl" : "xinput"}, {"maestro", maestro}, {"maestros", maestros}, {"titeres", titeres},
					{"ocultos360", mp.lectorSdl().ocultos360}, {"mostrar360", mp.lectorSdl().mostrar360} };
			}
		}

		// ---- Campo 2D en vivo (solo con la tarjeta Partido › Campo abierta; si no, apagado y sin coste) ----
		{
			json c = { {"estado", "apagado"}, {"motivo", ""}, {"jugadores", 0}, {"balon", false} };
			if (in.campoRapido) {
				using phoenix::campo::EstadoCampo;
				const phoenix::campo::FotoCampo fc = phoenix::campo::instantanea();
				const char* nombre = fc.estado == EstadoCampo::Activo ? "activo" : fc.estado == EstadoCampo::Buscando ? "buscando"
					: fc.estado == EstadoCampo::Esperando ? "esperando" : fc.estado == EstadoCampo::NoCompatible ? "no_compatible" : "apagado";
				c = { {"estado", nombre}, {"motivo", fc.motivo}, {"jugadores", static_cast<int>(fc.j.size())}, {"balon", fc.hayBola}, {"equipos", fc.equiposListos}, {"tanda", fc.tanda.activa} };
			}
			c["campoWeb"] = PhoenixPrefs::get().campoWeb;
			e["campo"] = c;
		}

		// ---- Solicitudes y espera -------------------------------------------
		json solicitudes = json::array();
		for (const Solicitud& sol : Solicitudes::instancia().pendientes()) {
			solicitudes.push_back({ {"parsecId", sol.parsecId}, {"nombre", sol.nombre}, {"mando", sol.mandoDestino} });
		}
		const std::vector<EspectadorVista> espectadores = sala.espectadores();
		// «Quiere jugar»: pulsó su mando sin permiso (está en «Mirando» o no lo autorizó el host). mando = -1
		for (uint32_t id : PhoenixRoles::instancia().quierenJugar()) {
			bool repetida = false;
			for (const json& s : solicitudes) if (s.value("parsecId", 0u) == id) repetida = true;
			if (repetida) continue;
			std::string nombre;
			for (const EspectadorVista& ev : espectadores) if (ev.parsecId == id) { nombre = ev.nombre; break; }
			if (nombre.empty()) continue;   // ya no está en la sala o ya tiene mando
			solicitudes.push_back({ {"parsecId", id}, {"nombre", nombre}, {"mando", -1} });
		}
		e["solicitudes"] = solicitudes;

		json espera = json::array();
		for (uint32_t id : PhoenixRoles::instancia().enEspera()) {
			for (const EspectadorVista& ev : espectadores) {
				if (ev.parsecId == id) { espera.push_back({ {"parsecId", id}, {"nombre", ev.nombre}, {"ping", ev.pingMs} }); break; }
			}
		}
		e["espera"] = espera;

		// ---- Invitados -------------------------------------------------------
		const std::map<uint32_t, std::pair<std::string, int>> pings = pingsPresentes(sala);
		json invitados = json::array();
		try {
			const uint32_t idHost = h.getHost().userID;
			for (Guest& g : h.getGuests()) {
				if (g.userID == idHost) continue;
				auto itPing = pings.find(g.userID);
				auto itMando = mandoDe.find(g.userID);
				invitados.push_back({
					{"parsecId", g.userID}, {"nombre", g.name},
					{"ping", itPing != pings.end() ? itPing->second.second : -1},
					{"mando", itMando != mandoDe.end() ? itMando->second : 0},
					{"mod", Cache::cache.modList.isModded(g.userID)}, {"vip", Cache::cache.vipList.isVIP(g.userID)},
					{"teclado", g.allowKeyboardInput}, {"raton", g.allowMouseInput},
					{"rolWeb", textoRol(PhoenixRoles::instancia().rolDe(g.userID))},
					{"cop", Cache::cache.isSodaCop(g.userID)}, {"falso", g.fake},
				});
			}
		}
		catch (...) {}
		e["invitados"] = invitados;

		// ---- Cartas de la web ------------------------------------------------
		json cartas = json::object();
		for (const auto& par : link.perfiles()) {
			const json c = json::parse(par.second, nullptr, false);
			if (!c.is_discarded()) cartas[std::to_string(par.first)] = c;
		}
		e["perfiles"] = cartas;

		// ---- Red (la serie completa solo si se está mirando) ----------------
		const bool verSerie = (in.seccion == "sala" && in.pestana == "red") || in.seccion == "partido";
		json red = json::array();
		for (const ResumenRed& r : in.red.resumen()) {
			json x = {
				{"parsecId", r.parsecId}, {"nombre", r.nombre}, {"presente", r.presente}, {"ultimo", r.ultimo},
				{"media", r.media}, {"maximo", r.maximo}, {"jitter", r.jitter}, {"picos", r.picos},
				{"alerta", r.alerta}, {"semaforo", r.semaforo},
			};
			if (verSerie) x["serie"] = r.serie;
			else {
				// Para las minigráficas: últimos 30 s
				const size_t desde = r.serie.size() > 30 ? r.serie.size() - 30 : 0;
				x["serie"] = std::vector<int>(r.serie.begin() + static_cast<std::ptrdiff_t>(desde), r.serie.end());
			}
			red.push_back(x);
		}
		e["red"] = red;
		e["retraso"] = estadoRetraso(in);   // medidor de retraso del mando (RetrasoWeb.cpp)

		{
			const InfoBuzon bz = link.buzon();
			const char* nombres[] = { "apagado", "cerrado", "noinstalado", "conectado", "sinconexion" };
			json avisosBz = json::array();
			for (const AvisoBuzon& a : bz.avisos) avisosBz.push_back({ {"id", a.id}, {"texto", a.texto}, {"hora", a.hora}, {"escrito", a.escrito} });
			e["buzon"] = { {"activo", PhoenixPrefs::get().avisosEnJuego}, {"estado", nombres[static_cast<int>(bz.estado)]}, {"ultimo", bz.ultimo},
				{"juego", bz.juego}, {"parche", bz.parche}, {"avisos", avisosBz} };
			{
				const InfoChatGlobal cg = link.chatGlobal();
				e["chatGlobal"] = { {"cargado", cg.cargado}, {"pausado", cg.pausado}, {"esperaSeg", cg.esperaSeg}, {"rev", cg.rev}, {"error", cg.error} };
			}
			{
				json ns = json::array();
				for (const NoticiaWeb& n : link.noticias()) ns.push_back({ {"id", n.id}, {"texto", n.texto}, {"nivel", n.nivel}, {"enlace", n.enlace}, {"hora", n.hora} });
				e["noticias"] = { {"cargado", link.noticiasCargadas()}, {"lista", ns} };
			}
			e["pes"] = { {"modo", PhoenixPrefs::get().modoPes}, {"abierto", link.pesAbierto()} };
			const InfoEntrega en = link.entrega();
			e["entrega"] = { {"estado", en.estado}, {"id", en.id}, {"resumen", en.resumen}, {"motivo", en.motivo},
				{"fecha", en.fecha}, {"puedeDeshacer", en.puedeDeshacer} };
		}

		e["partido"] = in.partido.comoJson(ahora);
		{
			// Partido según el juego (estado.json de phoenix.lua). Sin la edad del archivo: cambiaría en cada foto.
			const juego::EstadoPartidoJuego ej = link.estadoJuego();
			json golesJ = json::array();
			for (const juego::GolJuego& g : ej.goles) golesJ.push_back({ {"m", g.minuto}, {"local", g.local}, {"jugador", g.jugador}, {"propia", g.propia} });
			e["juego"] = ej.valido ? json{ {"datos", true}, {"fase", ej.fase}, {"minuto", ej.minuto}, {"periodo", ej.periodo},
				{"golesLocal", ej.golesLocal}, {"golesVisita", ej.golesVisita}, {"pkLocal", ej.pkLocal}, {"pkVisita", ej.pkVisita},
				{"nombreLocal", ej.nombreLocal}, {"nombreVisita", ej.nombreVisita}, {"relojCorre", ej.relojCorre}, {"goles", golesJ} }
				: json{ {"datos", false} };
			const PhoenixPrefs& prA = PhoenixPrefs::get();
			e["arbitro"] = { {"competitivo", prA.competitivo}, {"pausaAuto", prA.pausaAuto}, {"marcadorAuto", prA.marcadorAuto},
				{"soloAutorizados", prA.soloAutorizados}, {"modoRecarga", prA.modoRecarga},
				{"activo", in.arbitro.competitivoActivo}, {"origen", in.arbitro.origen},
				{"ultimaPausa", in.arbitro.ultimaPausa}, {"conStart", in.arbitro.ultimaConStart} };
			// Liga automática: partidos oficiales por jugar, el elegido y el último envío del resultado
			const InfoLiga lg = link.liga();
			json ofi = json::array();
			for (const PartidoOficial& po : lg.partidos) {
				ofi.push_back({ {"id", po.id}, {"fecha", po.fecha}, {"edicion", po.edicionNombre}, {"estado", po.estado},
					{"local", po.local}, {"visitante", po.visitante}, {"clubLocal", po.clubLocal}, {"clubVisita", po.clubVisita}, {"mio", po.mio} });
			}
			e["liga"] = { {"cargado", lg.cargado}, {"error", lg.error}, {"partidos", ofi}, {"ultimoEnvio", lg.ultimoEnvio},
				{"pendientes", lg.pendientes}, {"elegido", in.arbitro.partidoOficial}, {"elegidoTexto", in.arbitro.oficialTexto},
				{"enVivo", in.arbitro.vivoMandado} };
		}
		e["turnos"] = estadoTurnos();
		if (in.seccion == "ajustes" && in.pestana == "audio") e["audio"] = estadoAudio(h);
		if (in.seccion == "gente" || in.seccion == "partido") {
			json amigos = json::array();
			for (const AmigoWeb& a : link.amigos()) {
				amigos.push_back({ {"usuarioId", a.usuarioId}, {"nombre", a.nombre}, {"avatar", a.avatarUrl},
					{"estado", a.estado}, {"salaId", a.salaId}, {"desde", a.desde} });
			}
			e["amigos"] = { {"cargados", link.amigosCargados()}, {"lista", amigos} };
		}
		return e;
	}

	// =========================================================================
	// Chat general (contrato §27) en JSON para la interfaz.
	static json chatGlobalJson(const InfoChatGlobal& c) {
		json ms = json::array();
		for (const MensajeGlobal& m : c.mensajes) {
			ms.push_back({ {"id", m.id}, {"usuarioId", m.usuarioId}, {"nombre", m.nombre}, {"texto", m.texto}, {"hora", m.hora}, {"rol", m.rol}, {"propio", m.propio} });
		}
		return { {"cargado", c.cargado}, {"pausado", c.pausado}, {"esperaSeg", c.esperaSeg}, {"rev", c.rev}, {"error", c.error}, {"mensajes", ms} };
	}
	static long long chatGlobalRevVista = -1;

	json construirBienvenida(Interno& in) {
		json b;
		b["protocolo"] = 1;
		json idiomas = json::array();
		for (const auto& par : I18n::disponibles()) idiomas.push_back({ {"codigo", par.first}, {"nombre", par.second} });
		b["idiomas"] = idiomas;

		json resoluciones = json::array();
		for (const Config::Resolution& r : Config::cfg.resolutions) resoluciones.push_back(r.label);
		b["resoluciones"] = resoluciones;
		b["chat"] = json::array();
		b["actividad"] = json::array();
		{
			const InfoChatGlobal cg = PhoenixLink::instancia().chatGlobal();
			chatGlobalRevVista = cg.rev;
			b["chatGlobal"] = chatGlobalJson(cg);
		}
		if (in.ctx.hosting != nullptr) {
			Hosting& h = *in.ctx.hosting;
			if (in.listaPantallas.empty()) {
				try {
					in.listaPantallas = h.getDX11().listScreens();
					in.listaGpus = h.getDX11().listGPUs();
				}
				catch (...) {}
			}
			std::vector<std::string>& chat = h.getMessageLog();
			std::vector<std::string>& actividad = h.getCommandLog();
			b["chat"] = lineas(chat, 0, 100);
			b["actividad"] = lineas(actividad, 0, 150);
			in.chatVistos = chat.size();
			in.actividadVistos = actividad.size();
		}
		b["pantallas"] = in.listaPantallas;
		b["gpus"] = in.listaGpus;
		b["wgc"] = DX11::isWGCSupported();
		b["temasOverlay"] = Config::cfg.overlayThemes;
		b["estado"] = construirEstado(in);
		return b;
	}

	// =========================================================================
	void tickDatos(Interno& in) {
		if (in.ctx.sala == nullptr || in.ctx.hosting == nullptr) return;
		ProveedorSala& sala = *in.ctx.sala;
		Hosting& h = *in.ctx.hosting;
		const double ahora = ahoraSeg();
		const bool web = in.puente && in.puente->listo();

		// Muestras de red y sesión: 1 vez por segundo
		if (ahora - in.ultimaMuestraRed >= 1.0) {
			in.ultimaMuestraRed = ahora;
			const bool abierta = sala.abierta();
			if (abierta && in.salaAbiertaDesde < 0.0) {
				in.salaAbiertaDesde = ahora;
				in.picoInvitados = 0;
				in.entradasSesion = 0;
				in.partidosSesion = 0;
				in.presentesAntes.clear();
				in.red.limpiar();
			}
			if (!abierta && in.salaAbiertaDesde >= 0.0) {
				in.salaAbiertaDesde = -1.0;
				// Sala cerrada en pleno partido: se cancela (sin registro) y la web vuelve a «abierta».
				if (in.partido.fase() == FasePartido::EnJuego || in.partido.fase() == FasePartido::Pausado) {
					in.partido.cancelar();
					PhoenixLink::instancia().marcarPartido(false);
				}
			}
			if (abierta) {
				const auto pings = pingsPresentes(sala);
				in.red.registrar(pings, ahora);
				in.picoInvitados = (std::max)(in.picoInvitados, sala.totalInvitados());
				std::set<uint32_t> ahoraPresentes;
				for (const auto& par : pings) ahoraPresentes.insert(par.first);
				// Además de los que tienen ping, cualquiera en la lista de Parsec
				try {
					const uint32_t idHost = h.getHost().userID;
					for (Guest& g : h.getGuests()) if (g.userID != idHost && !g.fake) ahoraPresentes.insert(g.userID);
				}
				catch (...) {}
				for (uint32_t id : ahoraPresentes) {
					if (!in.presentesAntes.count(id)) {
						in.entradasSesion++;
						auto it = pings.find(id);
						const std::string nombre = it != pings.end() ? it->second.first : std::string();
						PhoenixLink::instancia().evento("entra", std::to_string(id), json{ {"nombre", nombre} }.dump());
					}
				}
				for (uint32_t id : in.presentesAntes) {
					if (!ahoraPresentes.count(id)) PhoenixLink::instancia().evento("sale", std::to_string(id), "{}");
				}
				in.presentesAntes = ahoraPresentes;
			}
		}

		// Árbitro del partido (modo competitivo, pausa automática, goles del juego)
		tickArbitro(in);

		// Medidor de retraso del mando por jugador (1 vez por segundo)
		tickRetraso(in);

		// Previsualización de audio (igual que el panel de audio original mientras está a la vista)
		if (web && in.seccion == "ajustes" && in.pestana == "audio" && !h.isRunning() && h.isReady()) {
			try {
				h.audioIn.captureAudio();
				h.audioOut.captureAudio();
			}
			catch (...) {}
		}

		// Chat general: solo se manda la lista cuando cambió (lo normal es nada)
		if (web) {
			const InfoChatGlobal cg = PhoenixLink::instancia().chatGlobal();
			if (cg.rev != chatGlobalRevVista) {
				chatGlobalRevVista = cg.rev;
				in.puente->evento("chatglobal", chatGlobalJson(cg));
			}
		}

		// Chat y actividad nuevos → eventos; aviso (parpadeo/sonido) si la web cubre la ventana
		if (web) {
			std::vector<std::string>& chat = h.getMessageLog();
			if (chat.size() != in.chatVistos) {
				const bool reinicio = chat.size() < in.chatVistos;
				const bool hayNuevos = chat.size() > in.chatVistos;
				in.puente->evento("chat", { {"reinicio", reinicio}, {"lineas", reinicio ? lineas(chat, 0, 100) : lineas(chat, in.chatVistos, 100)} });
				in.chatVistos = chat.size();
				if (hayNuevos && InterfazWeb::instancia().cubreVentana() && in.ctx.alMensajeChat) {
					try { in.ctx.alMensajeChat(); }
					catch (...) {}
				}
			}
			std::vector<std::string>& actividad = h.getCommandLog();
			if (actividad.size() != in.actividadVistos) {
				const bool reinicio = actividad.size() < in.actividadVistos;
				in.puente->evento("actividad", { {"reinicio", reinicio}, {"lineas", reinicio ? lineas(actividad, 0, 150) : lineas(actividad, in.actividadVistos, 150)} });
				in.actividadVistos = actividad.size();
			}
		}
	}

	void tickResultadosWeb(Interno& in) {
		for (const ResultadoWeb& r : PhoenixLink::instancia().tomarResultados()) {
			auto it = in.pendientesWeb.find(r.ticket);
			if (it == in.pendientesWeb.end()) continue;
			const uint64_t id = it->second;
			in.pendientesWeb.erase(it);
			if (r.ok) in.puente->responder(id, json{ {"mensaje", r.mensaje} });
			else in.puente->responderError(id, r.codigo.empty() ? "ERROR_WEB" : r.codigo, r.mensaje);
		}
	}

}
