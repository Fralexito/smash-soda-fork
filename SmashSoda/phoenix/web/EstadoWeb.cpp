#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../AudioTools.h"
#include "../../core/Cache.h"
#include "../../core/Config.h"
#include "../../services/Hotseat.h"
#include "../../services/WebSocket.h"
#include "../../services/OverlayService.h"
#include "../../widgets/HostSettingsWidget.h"
#include "../PhoenixBuild.h"
#include "../PhoenixPrefs.h"
#include "../PhoenixRoles.h"
#include "../I18n.h"
#include "../core/ProveedorSala.h"
#include "../core/Solicitudes.h"
#include "../core/MandoHost.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_estado {

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
			{"version", kVersion}, {"idioma", I18n::actual()}, {"tema", pr.tema},
			{"dev", Config::cfg.general.devMode}, {"soda", Cache::cache.version},
		};
		PhoenixLink& link = PhoenixLink::instancia();
		e["web"] = {
			{"estado", textoEstadoLink(link.estado())}, {"usuario", link.usuario()}, {"mensaje", link.mensaje()},
			{"publicada", link.salaPublicada()}, {"jugadoresLista", link.jugadoresEnLista()},
			{"versionLiga", link.versionLiga()}, {"eventosEnCola", link.eventosEnCola()},
		};
		e["ajustes"] = estadoAjustes(in);

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
		s["calidad"] = { {"fps", Config::cfg.video.fps}, {"mbps", Config::cfg.video.bandwidth} };
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
			for (size_t i = 0; i < asientos.size(); i++) {
				const AsientoVista& a = asientos[i];
				const int n = a.numero;
				const char* equipo = n <= pr.equipoLocal ? "local" : (n <= pr.mandosActivos ? "visitante" : "fuera");
				if (a.ocupado && a.parsecId != 0) mandoDe[a.parsecId] = n;
				lista.push_back({
					{"n", n}, {"conectado", a.conectado}, {"ocupado", a.ocupado}, {"bloqueado", a.bloqueado},
					{"jugador", a.jugador}, {"parsecId", a.parsecId}, {"ping", a.pingMs}, {"equipo", equipo},
					{"tipo", i < tipos.size() ? tipos[i] : std::string("xbox")},
				});
			}
			e["mandos"] = {
				{"lista", lista},
				{"formacion", { {"local", pr.equipoLocal}, {"visitante", pr.mandosActivos - pr.equipoLocal} }},
				{"host", MandoHost::activo()},
				{"bloqueoGlobal", gc.lock}, {"bloqueoBotones", gc.lockButtons}, {"esclavo", gc.isSlave},
				{"xbox", Config::cfg.input.xboxPuppetCount}, {"ds4", Config::cfg.input.ds4PuppetCount},
				{"reiniciando", in.reiniciandoMandos},
			};
		}

		// ---- Solicitudes y espera -------------------------------------------
		json solicitudes = json::array();
		for (const Solicitud& sol : Solicitudes::instancia().pendientes()) {
			solicitudes.push_back({ {"parsecId", sol.parsecId}, {"nombre", sol.nombre}, {"mando", sol.mandoDestino} });
		}
		e["solicitudes"] = solicitudes;

		const std::vector<EspectadorVista> espectadores = sala.espectadores();
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

		e["partido"] = in.partido.comoJson(ahora);
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

		// Previsualización de audio (igual que el panel de audio original mientras está a la vista)
		if (web && in.seccion == "ajustes" && in.pestana == "audio" && !h.isRunning() && h.isReady()) {
			try {
				h.audioIn.captureAudio();
				h.audioOut.captureAudio();
			}
			catch (...) {}
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
