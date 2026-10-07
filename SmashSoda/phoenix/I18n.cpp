#include "I18n.h"
#include "I18nOriginal.h"

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

#include "../helpers/PathHelper.h"

using json = nlohmann::json;

namespace phoenix {

	namespace {

		using Tabla = std::map<std::string, std::string>;

		// Español = idioma base: toda clave nueva se escribe primero aquí.
		const Tabla kEs = {
			{"app.nombre", "Phoenix Soda"},
			{"estado.vivo", "EN VIVO"},
			{"estado.cerrada", "SALA CERRADA"},
			{"estado.invitados", "en sala"},
			{"nav.sala", "Sala"},
			{"nav.partido", "Partido"},
			{"nav.mandos", "Mandos"},
			{"nav.comunidad", "Comunidad"},
			{"nav.ajustes", "Ajustes"},
			{"nav.gente", "Gente"},
			{"estado.controlando", "CONTROLAS EL MANDO %d  ·  Ctrl+Alt+0 suelta"},
			{"sala.entrada_parsec", "Permitir entrar directo por Parsec"},
			{"sala.entrada_parsec_on", "Quien no esté en la lista entra en espera: tú decides si juega o mira."},
			{"sala.entrada_parsec_off", "Solo entran los de la lista de la web (o todos en amistoso)."},
			{"mandos.entro_parsec", "%s entró por Parsec"},
			{"mandos.como_jugador", "Jugador"},
			{"mandos.como_espectador", "Espectador"},
			{"sub.gente", "Quién está en tu sala"},
			{"tab.en_sala", "En la sala"},
			{"tab.moderacion", "Moderación"},
			{"tab.rapido", "Rápido"},
			{"gente.vacio", "Tu sala está vacía"},
			{"gente.vacio_abierta", "Comparte el enlace y aquí aparecerá cada jugador."},
			{"gente.vacio_cerrada", "Abre tu sala en «Sala» para recibir jugadores."},
			{"gente.copiar", "Copiar enlace"},
			{"gente.mando", "Mando"},
			{"gente.espectador", "Espectador"},
			{"gente.jugadores", "jugando"},
			{"gente.espectadores", "mirando"},
			{"gente.dar_mando", "Dar mando"},
			{"gente.quitar_mando", "Quitar mando"},
			{"gente.sin_mandos", "Mandos llenos"},
			{"gente.expulsar", "Expulsar"},
			{"gente.seguro", "¿Seguro?"},
			{"aj.calidad", "CALIDAD DE TRANSMISIÓN"},
			{"aj.ligero", "Ligero"},
			{"aj.ligero_d", "Internet justo o rivales lejos"},
			{"aj.equilibrado", "Equilibrado"},
			{"aj.equilibrado_d", "Lo ideal para la mayoría"},
			{"aj.maxima", "Máxima"},
			{"aj.maxima_d", "Fibra y rivales cerca"},
			{"aj.activo", "ACTIVO"},
			{"aj.actual", "Ahora:"},
			{"aj.interfaz", "INTERFAZ"},
			{"aj.avanzado_hint", "¿Buscas algo más fino? Está en General, Video y Audio."},
			{"sub.sala", "Tu sala de un vistazo"},
			{"sub.partido", "Control y justicia del partido"},
			{"sub.mandos", "Quién controla qué"},
			{"sub.comunidad", "Moderación y jugadores"},
			{"sub.ajustes", "Configura la app a tu gusto"},
			{"tab.mandos", "Mandos"},
			{"tab.puppets", "Puppets"},
			{"tab.hotseat", "Turnos"},
			{"tab.bloqueo", "Bloqueo de botones"},
			{"tab.teclado", "Teclado"},
			{"tab.general", "General"},
			{"tab.video", "Video"},
			{"tab.audio", "Audio"},
			{"tab.streaming", "Transmisión"},
			{"tab.biblioteca", "Biblioteca"},
			{"panel.config", "Configuración de la sala"},
			{"panel.invitados", "Invitados"},
			{"panel.actividad", "Actividad"},
			{"partido.titulo", "Próximamente"},
			{"partido.desc", "Pausas técnicas, detección de inicio y fin, captura del resultado y retraso al host llegan en las próximas fases."},
			{"barra.idioma", "Idioma"},
			{"barra.clasica", "Interfaz clásica"},
			{"barra.volver", "Volver a Phoenix"},
			{"barra.version", "Versión"},
			{"barra.sin_host", "Sin cuenta Parsec"},
			{"sala.abierta_titulo", "TU SALA ESTÁ ABIERTA"},
			{"sala.cerrada_titulo", "TU SALA ESTÁ CERRADA"},
			{"sala.abrir", "Abrir sala"},
			{"sala.cerrar", "Cerrar sala"},
			{"sala.confirmar_cerrar", "¿Seguro? Toca otra vez"},
			{"sala.invitados", "invitados"},
			{"sala.copiar", "Copiar"},
			{"sala.copiado", "¡Copiado!"},
			{"sala.sin_enlace", "Abre la sala para obtener el enlace de invitación"},
			{"sala.visibilidad", "VISIBILIDAD"},
			{"sala.publica", "Pública"},
			{"sala.amigos", "Amigos"},
			{"sala.privada", "Privada"},
			{"sala.vis_ayuda_publica", "Aparece en el radar de retos de la web: cualquiera puede retarte."},
			{"sala.vis_ayuda_amigos", "Solo tus amigos la ven en la web y en la app."},
			{"sala.vis_ayuda_privada", "No aparece en ningún lado: solo entra quien tenga el enlace."},
			{"sala.permitir_esp", "Permitir espectadores"},
			{"sala.limite", "Límite"},
			{"sala.web_pendiente", "La visibilidad se publicará en la web cuando vincules tu cuenta."},
			{"sala.mandos", "MANDOS"},
			{"sala.mando", "MANDO"},
			{"sala.libre", "Libre"},
			{"sala.sin_mando", "Mando desconectado"},
			{"sala.espectadores", "ESPECTADORES"},
			{"sala.sin_espectadores", "Nadie mirando todavía."},
			{"sala.avanzado", "Configuración avanzada"},
			{"sala.volver", "‹ Volver a la sala"},
			{"web.conectado", "Conectado a la web como"},
			{"web.vinculando", "Vinculando esta PC…"},
			{"web.sin_conexion", "Sin conexión con la web: reintentando solo"},
			{"web.pausado", "Conexión con la web en pausa"},
			{"web.sin_vincular", "Esta PC no está vinculada a tu cuenta"},
			{"web.ayuda_codigo", "En la web: Perfil → Vincular Smash Soda. Escribe aquí el código de 6 dígitos."},
			{"web.vincular", "Vincular"},
			{"mandos.cantidad", "Mandos"},
			{"mandos.pide_mando", "%s pide el mando %d"},
			{"mandos.pide_equipo", "%s pide cambiar de equipo"},
			{"mandos.aceptar", "Aceptar"},
			{"mandos.rechazar", "Rechazar"},
			{"mandos.local", "Local"},
			{"mandos.equipo_local", "LOCAL"},
			{"mandos.equipo_visitante", "VISITANTE"},
			{"mandos.conectar", "Conectar mando"},
			{"mandos.clic_conectar", "Clic para conectarlo"},
			{"mandos.arrastra", "Arrástralo para moverlo"},
			{"mandos.arrastra_aqui", "Arrastra aquí a un jugador"},
			{"mandos.bloqueado", "Bloqueado"},
			{"mandos.bloquear", "Bloquear este mando"},
			{"mandos.desbloquear", "Desbloquear este mando"},
			{"mandos.soltar", "suelta sobre un mando"},
			{"mandos.ayuda_arrastrar", "Arrastra a un espectador sobre un mando para darle control. Los jugadores pueden pedir cambios con !cambio 3 o !equipo en el chat."},
			{"web.reintentar", "Reintentar"},
			{"web.desvincular", "Desvincular esta PC"},
		};

		const Tabla kEn = {
			{"estado.vivo", "LIVE"}, {"estado.cerrada", "ROOM CLOSED"}, {"estado.invitados", "in room"},
			{"nav.sala", "Room"}, {"nav.partido", "Match"}, {"nav.mandos", "Gamepads"},
			{"nav.gente", "People"}, {"estado.controlando", "YOU CONTROL PAD %d  ·  Ctrl+Alt+0 releases"}, {"sala.entrada_parsec", "Allow joining directly via Parsec"}, {"sala.entrada_parsec_on", "Anyone not on the list waits: you decide if they play or watch."}, {"sala.entrada_parsec_off", "Only people on the web list can join (everyone in friendly mode)."}, {"mandos.entro_parsec", "%s joined via Parsec"}, {"mandos.como_jugador", "Player"}, {"mandos.como_espectador", "Spectator"}, {"sub.gente", "Who is in your room"}, {"tab.en_sala", "In room"}, {"tab.moderacion", "Moderation"}, {"tab.rapido", "Quick"}, {"gente.vacio", "Your room is empty"}, {"gente.vacio_abierta", "Share the link and every player will show up here."}, {"gente.vacio_cerrada", "Open your room in Room to receive players."}, {"gente.copiar", "Copy link"}, {"gente.mando", "Pad"}, {"gente.espectador", "Spectator"}, {"gente.jugadores", "playing"}, {"gente.espectadores", "watching"}, {"gente.dar_mando", "Give pad"}, {"gente.quitar_mando", "Remove pad"}, {"gente.sin_mandos", "Pads full"}, {"gente.expulsar", "Kick"}, {"gente.seguro", "Sure?"}, {"aj.calidad", "STREAM QUALITY"}, {"aj.ligero", "Light"}, {"aj.ligero_d", "Weak internet or distant rivals"}, {"aj.equilibrado", "Balanced"}, {"aj.equilibrado_d", "Best for most people"}, {"aj.maxima", "Maximum"}, {"aj.maxima_d", "Fiber and nearby rivals"}, {"aj.activo", "ACTIVE"}, {"aj.actual", "Now:"}, {"aj.interfaz", "INTERFACE"}, {"aj.avanzado_hint", "Looking for finer control? It is in General, Video and Audio."},
			{"nav.comunidad", "Community"}, {"nav.ajustes", "Settings"},
			{"sub.sala", "Your room at a glance"}, {"sub.partido", "Match control and fairness"},
			{"sub.mandos", "Who controls what"}, {"sub.comunidad", "Moderation and players"},
			{"sub.ajustes", "Make the app yours"},
			{"tab.mandos", "Gamepads"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Hotseat"},
			{"tab.bloqueo", "Button lock"}, {"tab.teclado", "Keyboard"},
			{"tab.general", "General"}, {"tab.video", "Video"}, {"tab.audio", "Audio"},
			{"tab.streaming", "Streaming"}, {"tab.biblioteca", "Library"},
			{"panel.config", "Room settings"}, {"panel.invitados", "Guests"}, {"panel.actividad", "Activity"},
			{"partido.titulo", "Coming soon"},
			{"partido.desc", "Technical pauses, match start/end detection, result capture and host delay arrive in the next phases."},
			{"barra.idioma", "Language"}, {"barra.clasica", "Classic interface"},
			{"barra.volver", "Back to Phoenix"}, {"barra.version", "Version"}, {"barra.sin_host", "No Parsec account"},
			{"sala.abierta_titulo", "YOUR ROOM IS OPEN"}, {"sala.cerrada_titulo", "YOUR ROOM IS CLOSED"},
			{"sala.abrir", "Open room"}, {"sala.cerrar", "Close room"}, {"sala.confirmar_cerrar", "Sure? Tap again"},
			{"sala.invitados", "guests"}, {"sala.copiar", "Copy"}, {"sala.copiado", "Copied!"},
			{"sala.sin_enlace", "Open the room to get the invite link"},
			{"sala.visibilidad", "VISIBILITY"}, {"sala.publica", "Public"}, {"sala.amigos", "Friends"}, {"sala.privada", "Private"},
			{"sala.vis_ayuda_publica", "Shows up on the web challenge radar: anyone can challenge you."},
			{"sala.vis_ayuda_amigos", "Only your friends see it on the web and in the app."},
			{"sala.vis_ayuda_privada", "Not listed anywhere: only people with the link can join."},
			{"sala.permitir_esp", "Allow spectators"}, {"sala.limite", "Limit"},
			{"sala.web_pendiente", "Visibility will be published on the web once you link your account."},
			{"sala.mandos", "GAMEPADS"}, {"sala.mando", "PAD"}, {"sala.libre", "Free"},
			{"sala.sin_mando", "Gamepad disconnected"}, {"sala.espectadores", "SPECTATORS"},
			{"sala.sin_espectadores", "Nobody watching yet."}, {"sala.avanzado", "Advanced settings"},
			{"sala.volver", "‹ Back to room"},
			{"web.conectado", "Connected to the web as"}, {"web.vinculando", "Linking this PC…"},
			{"web.sin_conexion", "No connection to the web: retrying"}, {"web.pausado", "Web connection paused"},
			{"web.sin_vincular", "This PC is not linked to your account"},
			{"web.ayuda_codigo", "On the web: Profile → Link Smash Soda. Type the 6-digit code here."},
			{"mandos.pide_mando", "%s asks for gamepad %d"}, {"mandos.pide_equipo", "%s asks to switch teams"}, {"mandos.aceptar", "Accept"}, {"mandos.rechazar", "Decline"}, {"mandos.cantidad", "Gamepads"}, {"mandos.local", "Home"}, {"mandos.equipo_local", "HOME"},
			{"mandos.equipo_visitante", "AWAY"}, {"mandos.conectar", "Connect gamepad"}, {"mandos.clic_conectar", "Click to connect"},
			{"mandos.arrastra", "Drag to move"}, {"mandos.arrastra_aqui", "Drop a player here"}, {"mandos.bloqueado", "Locked"},
			{"mandos.bloquear", "Lock this gamepad"}, {"mandos.desbloquear", "Unlock this gamepad"}, {"mandos.soltar", "drop on a gamepad"},
			{"mandos.ayuda_arrastrar", "Drag a spectator onto a gamepad to give control."}, {"sala.parche", "Patch"}, {"sala.region", "Region"}, {"web.vincular", "Link"}, {"web.reintentar", "Retry"}, {"web.desvincular", "Unlink this PC"},
		};

		const Tabla kPt = {
			{"estado.vivo", "AO VIVO"}, {"estado.cerrada", "SALA FECHADA"}, {"estado.invitados", "na sala"},
			{"nav.sala", "Sala"}, {"nav.partido", "Partida"}, {"nav.mandos", "Controles"},
			{"nav.comunidad", "Comunidade"}, {"nav.ajustes", "Ajustes"},
			{"sub.sala", "Sua sala num relance"}, {"sub.partido", "Controle e justiça da partida"},
			{"sub.mandos", "Quem controla o quê"}, {"sub.comunidad", "Moderação e jogadores"},
			{"sub.ajustes", "Configure o app do seu jeito"},
			{"tab.mandos", "Controles"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Turnos"},
			{"tab.bloqueo", "Bloqueio de botões"}, {"tab.teclado", "Teclado"},
			{"tab.general", "Geral"}, {"tab.video", "Vídeo"}, {"tab.audio", "Áudio"},
			{"tab.streaming", "Transmissão"}, {"tab.biblioteca", "Biblioteca"},
			{"panel.config", "Configuração da sala"}, {"panel.invitados", "Convidados"}, {"panel.actividad", "Atividade"},
			{"partido.titulo", "Em breve"},
			{"partido.desc", "Pausas técnicas, detecção de início e fim, captura do resultado e atraso do host chegam nas próximas fases."},
			{"barra.idioma", "Idioma"}, {"barra.clasica", "Interface clássica"},
			{"barra.volver", "Voltar ao Phoenix"}, {"barra.version", "Versão"}, {"barra.sin_host", "Sem conta Parsec"},
		};

		const Tabla kFr = {
			{"estado.vivo", "EN DIRECT"}, {"estado.cerrada", "SALLE FERMÉE"}, {"estado.invitados", "dans la salle"},
			{"nav.sala", "Salle"}, {"nav.partido", "Match"}, {"nav.mandos", "Manettes"},
			{"nav.comunidad", "Communauté"}, {"nav.ajustes", "Réglages"},
			{"sub.sala", "Votre salle en un coup d'oeil"}, {"sub.partido", "Contrôle et équité du match"},
			{"sub.mandos", "Qui contrôle quoi"}, {"sub.comunidad", "Modération et joueurs"},
			{"sub.ajustes", "Personnalisez l'application"},
			{"tab.mandos", "Manettes"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Tours"},
			{"tab.bloqueo", "Verrouillage"}, {"tab.teclado", "Clavier"},
			{"tab.general", "Général"}, {"tab.video", "Vidéo"}, {"tab.audio", "Audio"},
			{"tab.streaming", "Diffusion"}, {"tab.biblioteca", "Bibliothèque"},
			{"panel.config", "Réglages de la salle"}, {"panel.invitados", "Invités"}, {"panel.actividad", "Activité"},
			{"partido.titulo", "Bientôt"},
			{"partido.desc", "Pauses techniques, détection de début et de fin, capture du résultat et délai de l'hôte arrivent dans les prochaines phases."},
			{"barra.idioma", "Langue"}, {"barra.clasica", "Interface classique"},
			{"barra.volver", "Retour à Phoenix"}, {"barra.version", "Version"}, {"barra.sin_host", "Aucun compte Parsec"},
		};

		const Tabla kIt = {
			{"estado.vivo", "IN DIRETTA"}, {"estado.cerrada", "STANZA CHIUSA"}, {"estado.invitados", "in stanza"},
			{"nav.sala", "Stanza"}, {"nav.partido", "Partita"}, {"nav.mandos", "Controller"},
			{"nav.comunidad", "Comunità"}, {"nav.ajustes", "Impostazioni"},
			{"sub.sala", "La tua stanza a colpo d'occhio"}, {"sub.partido", "Controllo e correttezza della partita"},
			{"sub.mandos", "Chi controlla cosa"}, {"sub.comunidad", "Moderazione e giocatori"},
			{"sub.ajustes", "Configura l'app a modo tuo"},
			{"tab.mandos", "Controller"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Turni"},
			{"tab.bloqueo", "Blocco pulsanti"}, {"tab.teclado", "Tastiera"},
			{"tab.general", "Generale"}, {"tab.video", "Video"}, {"tab.audio", "Audio"},
			{"tab.streaming", "Trasmissione"}, {"tab.biblioteca", "Libreria"},
			{"panel.config", "Impostazioni stanza"}, {"panel.invitados", "Ospiti"}, {"panel.actividad", "Attività"},
			{"partido.titulo", "In arrivo"},
			{"partido.desc", "Pause tecniche, rilevamento di inizio e fine, cattura del risultato e ritardo dell'host arrivano nelle prossime fasi."},
			{"barra.idioma", "Lingua"}, {"barra.clasica", "Interfaccia classica"},
			{"barra.volver", "Torna a Phoenix"}, {"barra.version", "Versione"}, {"barra.sin_host", "Nessun account Parsec"},
		};

		const Tabla kDe = {
			{"estado.vivo", "LIVE"}, {"estado.cerrada", "RAUM GESCHLOSSEN"}, {"estado.invitados", "im Raum"},
			{"nav.sala", "Raum"}, {"nav.partido", "Spiel"}, {"nav.mandos", "Controller"},
			{"nav.comunidad", "Community"}, {"nav.ajustes", "Einstellungen"},
			{"sub.sala", "Dein Raum auf einen Blick"}, {"sub.partido", "Spielkontrolle und Fairness"},
			{"sub.mandos", "Wer steuert was"}, {"sub.comunidad", "Moderation und Spieler"},
			{"sub.ajustes", "Passe die App an"},
			{"tab.mandos", "Controller"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Hotseat"},
			{"tab.bloqueo", "Tastensperre"}, {"tab.teclado", "Tastatur"},
			{"tab.general", "Allgemein"}, {"tab.video", "Video"}, {"tab.audio", "Audio"},
			{"tab.streaming", "Streaming"}, {"tab.biblioteca", "Bibliothek"},
			{"panel.config", "Raumeinstellungen"}, {"panel.invitados", "Gäste"}, {"panel.actividad", "Aktivität"},
			{"partido.titulo", "Demnächst"},
			{"partido.desc", "Technische Pausen, Erkennung von Spielbeginn und -ende, Ergebniserfassung und Host-Verzögerung folgen in den nächsten Phasen."},
			{"barra.idioma", "Sprache"}, {"barra.clasica", "Klassische Oberfläche"},
			{"barra.volver", "Zurück zu Phoenix"}, {"barra.version", "Version"}, {"barra.sin_host", "Kein Parsec-Konto"},
		};

		struct Estado {
			std::mutex mutex;
			std::string codigo = "es";
			std::map<std::string, Tabla> tablas;
			std::map<std::string, std::string> nombres;
			bool cargado = false;
		};

		Estado& estado() {
			static Estado e;
			return e;
		}

		// Llamar con el mutex tomado.
		void cargarSiHaceFalta(Estado& e) {
			if (e.cargado) return;
			e.cargado = true;

			e.tablas["es"] = kEs;
			for (const auto& par : textosOriginalesEs()) e.tablas["es"].insert(par);
			e.tablas["en"] = kEn;
			e.tablas["pt"] = kPt;
			e.tablas["fr"] = kFr;
			e.tablas["it"] = kIt;
			e.tablas["de"] = kDe;
			e.nombres = {
				{"es", "Español"}, {"en", "English"}, {"pt", "Português"},
				{"fr", "Français"}, {"it", "Italiano"}, {"de", "Deutsch"},
			};

			// Idiomas extra / correcciones desde archivos (nunca rompe la app).
			try {
				const std::string carpeta = PathHelper::GetConfigPath() + "idiomas\\";
				for (const std::string& archivo : PathHelper::GetFilenames(carpeta, true)) {
					if (archivo.size() < 6 || archivo.substr(archivo.size() - 5) != ".json") continue;
					const std::string codigo = archivo.substr(0, archivo.size() - 5);
					size_t tam = 0;
					void* datos = MTY_ReadFile((carpeta + archivo).c_str(), &tam);
					if (datos == nullptr) continue;
					const std::string texto(static_cast<const char*>(datos), tam);
					MTY_Free(datos);

					const json j = json::parse(texto, nullptr, false);
					if (j.is_discarded() || !j.is_object()) continue;
					Tabla& destino = e.tablas[codigo];
					for (auto it = j.begin(); it != j.end(); ++it) {
						if (it.key() == "_nombre" && it->is_string()) e.nombres[codigo] = it->get<std::string>();
						else if (it->is_string()) destino[it.key()] = it->get<std::string>();
					}
					if (!e.nombres.count(codigo)) e.nombres[codigo] = codigo;
				}
			}
			catch (...) {
				// Archivos de idioma defectuosos se ignoran.
			}
		}
	}

	const char* T(const char* clave) {
		Estado& e = estado();
		std::lock_guard<std::mutex> lock(e.mutex);
		cargarSiHaceFalta(e);

		auto buscar = [&](const std::string& codigo) -> const char* {
			auto t = e.tablas.find(codigo);
			if (t == e.tablas.end()) return nullptr;
			auto it = t->second.find(clave);
			return it == t->second.end() ? nullptr : it->second.c_str();
		};

		if (const char* s = buscar(e.codigo)) return s;
		if (const char* s = buscar("es")) return s;
		return clave;
	}

	std::string Tr(const std::string& original) {
		if (original.empty()) return original;

		const size_t corte = original.find("##");
		const std::string base = corte == std::string::npos ? original : original.substr(0, corte);
		const std::string sufijo = corte == std::string::npos ? std::string() : original.substr(corte);
		if (base.empty()) return original;

		Estado& e = estado();
		std::lock_guard<std::mutex> lock(e.mutex);
		cargarSiHaceFalta(e);
		if (e.codigo == "en") return original;

		auto t = e.tablas.find(e.codigo);
		if (t == e.tablas.end()) return original;
		auto it = t->second.find("en:" + base);
		if (it == t->second.end()) return original;
		return it->second + sufijo;
	}

	void I18n::establecer(const std::string& codigo) {
		Estado& e = estado();
		std::lock_guard<std::mutex> lock(e.mutex);
		cargarSiHaceFalta(e);
		e.codigo = e.tablas.count(codigo) ? codigo : "es";
	}

	const std::string& I18n::actual() {
		Estado& e = estado();
		std::lock_guard<std::mutex> lock(e.mutex);
		return e.codigo;
	}

	std::vector<std::pair<std::string, std::string>> I18n::disponibles() {
		Estado& e = estado();
		std::lock_guard<std::mutex> lock(e.mutex);
		cargarSiHaceFalta(e);
		std::vector<std::pair<std::string, std::string>> lista;
		for (const auto& par : e.nombres) lista.push_back(par);
		return lista;
	}

}
