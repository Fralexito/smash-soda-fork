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
			{"app.nombre", "Phoenix Link"},
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
			{"gente.banear", "Banear"},
			{"gente.en_sala_n", "conectados"},
			{"gente.filtrar", "Filtrar por nombre o ID"},
			{"gente.sin_resultados", "Nadie coincide con tu búsqueda."},
			{"gente.elige", "Elige a alguien de la lista"},
			{"gente.jugando", "jugando"},
			{"gente.permitir_teclado", "Permitir teclado"},
			{"gente.permitir_raton", "Permitir ratón"},
			{"gente.hacer_vip", "Hacer VIP"},
			{"gente.quitar_vip", "Quitar VIP"},
			{"gente.hacer_mod", "Hacer moderador"},
			{"gente.quitar_mod", "Quitar moderador"},
			{"gente.no_baneable", "Es un moderador oficial: no se puede banear"},
			{"gente.b_mod", "MOD"},
			{"gente.b_vip", "VIP"},
			{"gente.b_teclado", "TECLADO"},
			{"gente.b_raton", "RATÓN"},
			{"gente.permisos", "PERMISOS"},
			{"gente.roles", "ROLES"},
			{"gente.acciones", "ACCIONES"},
			{"aj.g_programa", "PROGRAMA"},
			{"aj.g_chat", "CHAT Y BOT"},
			{"aj.g_entrada", "ENTRADA"},
			{"aj.g_seguridad", "SEGURIDAD"},
			{"aj.g_clasico", "MÁS OPCIONES"},
			{"aj.g_tema", "Tema"},
			{"aj.g_tema_d", "Cambia el esquema de colores."},
			{"aj.g_flash", "Parpadear al recibir mensajes"},
			{"aj.g_flash_d", "La ventana parpadea si no está en primer plano."},
			{"aj.g_dev", "Modo desarrollador"},
			{"aj.g_dev_d", "Opciones extra para probar la app. Solo si sabes lo que haces."},
			{"aj.g_bot", "Nombre del bot"},
			{"aj.g_bot_d", "Ponle un nombre divertido si quieres."},
			{"aj.g_discord", "Enlace de Discord"},
			{"aj.g_discord_d", "Se muestra en el chat con !discord."},
			{"aj.g_bienvenida", "Mensaje de bienvenida"},
			{"aj.g_bienvenida_d", "Los invitados lo verán al entrar. Escribe _PLAYER_ para insertar su nombre."},
			{"aj.g_tts", "Leer el chat en voz alta"},
			{"aj.g_tts_d", "Lee en voz alta los mensajes visibles con la voz de Windows."},
			{"aj.g_bonk", "Permitir !bonk"},
			{"aj.g_bonk_d", "Divertido al inicio, pero puede cansar rápido."},
			{"aj.g_guia", "Desactivar botón Guía"},
			{"aj.g_guia_d", "El botón Guía suele abrir overlays y puede dar problemas al hostear."},
			{"aj.g_teclado", "Desactivar teclado"},
			{"aj.g_teclado_d", "Impide que invitados sin mando jueguen con teclado."},
			{"aj.g_autoindex", "Indexar mandos automáticamente"},
			{"aj.g_autoindex_d", "Identifica los índices XInput solos. Puede causar pantallazos azules en algunos equipos."},
			{"aj.g_ip", "Bloquear también la IP"},
			{"aj.g_ip_d", "Al banear a alguien, se bloquea también su dirección IP."},
			{"aj.g_vpn", "Bloquear VPN"},
			{"aj.g_vpn_d", "Actívalo solo si tienes problemas con trolls. Algunos usan VPN legítimamente."},
			{"aj.g_logs", "Registros de Parsec"},
			{"aj.g_logs_d", "Muestra los registros de Parsec en el Registro."},
			{"aj.g_clasico_t", "Más opciones"},
			{"aj.g_clasico_d", "WebSocket, permisos por rol, sonidos y el resto de opciones."},
			{"aj.g_clasico_b", "Ver más opciones"},
			{"aj.g_volver", "Volver a General"},
			{"web.volver_nueva", "Volver a la interfaz nueva"},
			{"web.no_disponible", "La interfaz nueva no pudo abrirse. Sigues en la interfaz de siempre."},
			{"web.reintentar_nueva", "Reintentar"},
			{"web.instalar_webview2", "Instalar componente de Microsoft"},
			{"web.ocultar_aviso", "Ocultar"},
			{"aj.usar_nueva", "Usar la interfaz nueva (HTML)"},
			{"aj.usar_nueva_d", "Diseño nuevo con Partido, Red, Amigos y más. Si tu PC no puede mostrarla, la app vuelve sola a esta."},
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
			{"tab.puppets", "Maestro"},
			{"tab.hotseat", "Turnos"},
			{"tab.bloqueo", "Bloqueo"},
			{"tab.teclado", "Teclado"},
			{"tab.avanzado", "Avanzado"},
			{"teclado.presiona_tecla", "Presiona la tecla en el teclado..."},
			{"tab.general", "General"},
			{"tab.video", "Video"},
			{"tab.audio", "Audio"},
			{"tab.streaming", "Transmisión"},
			{"tab.biblioteca", "Biblioteca"},
			{"tab.overlay", "Overlay"},
			{"tab.resumen", "Resumen"},
			{"tab.actividad", "Actividad"},
			{"sala.latencia_vivo", "LATENCIA EN VIVO"},
			{"sala.transmision", "TRANSMISIÓN"},
			{"sala.plazas", "Plazas"},
			{"tab.opciones", "Opciones"},
			{"sala.host", "ANFITRIÓN"},
			{"sala.invitado", "INVITADO"},
			{"sala.host_detalle", "Control local"},
			{"sala.mando_n", "Mando %d · Parsec"},
			{"sala.esperando", "Esperando…"},
			{"sala.sin_rival", "Sin rival"},
			{"sala.comparte_enlace", "Comparte el enlace"},
			{"sala.abre_sala", "Abre la sala primero"},
			{"sala.midiendo", "Midiendo…"},
			{"sala.enlace_estable", "Enlace estable"},
			{"sala.enlace_regular", "Enlace regular"},
			{"sala.enlace_inestable", "Enlace inestable"},
			{"panel.config", "Configuración de la sala"},
			{"panel.invitados", "Invitados"},
			{"panel.actividad", "Actividad"},
			{"partido.titulo", "Próximamente"},
			{"partido.desc", "Pausas técnicas, detección de inicio y fin, captura del resultado y retraso al host llegan en las próximas fases."},
			{"bib.titulo", "Biblioteca"},
			{"bib.pes2021", "EFootball PES 2021"},
			{"bib.spfl2026", "SP Football Life 2026"},
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
			{"web.ayuda_codigo", "En la web: Mi perfil → vincular PC. Escribe aquí el código de 6 dígitos."},
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
			{"nav.gente", "People"}, {"estado.controlando", "YOU CONTROL PAD %d  ·  Ctrl+Alt+0 releases"}, {"sala.entrada_parsec", "Allow joining directly via Parsec"}, {"sala.entrada_parsec_on", "Anyone not on the list waits: you decide if they play or watch."}, {"sala.entrada_parsec_off", "Only people on the web list can join (everyone in friendly mode)."}, {"mandos.entro_parsec", "%s joined via Parsec"}, {"mandos.como_jugador", "Player"}, {"mandos.como_espectador", "Spectator"}, {"sub.gente", "Who is in your room"}, {"tab.en_sala", "In room"}, {"tab.moderacion", "Moderation"}, {"tab.rapido", "Quick"}, {"gente.vacio", "Your room is empty"}, {"gente.vacio_abierta", "Share the link and every player will show up here."}, {"gente.vacio_cerrada", "Open your room in Room to receive players."}, {"gente.copiar", "Copy link"}, {"gente.mando", "Pad"}, {"gente.espectador", "Spectator"}, {"gente.jugadores", "playing"}, {"gente.espectadores", "watching"}, {"gente.dar_mando", "Give pad"}, {"gente.quitar_mando", "Remove pad"}, {"gente.sin_mandos", "Pads full"}, {"gente.expulsar", "Kick"}, {"gente.seguro", "Sure?"}, {"gente.banear", "Ban"}, {"gente.en_sala_n", "connected"}, {"gente.filtrar", "Filter by name or ID"}, {"gente.sin_resultados", "Nobody matches your search."}, {"gente.elige", "Pick someone from the list"}, {"gente.jugando", "playing"}, {"gente.permitir_teclado", "Allow keyboard"}, {"gente.permitir_raton", "Allow mouse"}, {"gente.hacer_vip", "Make VIP"}, {"gente.quitar_vip", "Remove VIP"}, {"gente.hacer_mod", "Make moderator"}, {"gente.quitar_mod", "Remove moderator"}, {"gente.no_baneable", "Official moderator: cannot be banned"}, {"gente.b_mod", "MOD"}, {"gente.b_vip", "VIP"}, {"gente.b_teclado", "KEYBOARD"}, {"gente.b_raton", "MOUSE"}, {"gente.permisos", "PERMISSIONS"}, {"gente.roles", "ROLES"}, {"gente.acciones", "ACTIONS"}, {"aj.g_programa", "PROGRAM"}, {"aj.g_chat", "CHAT AND BOT"}, {"aj.g_entrada", "INPUT"}, {"aj.g_seguridad", "SECURITY"}, {"aj.g_clasico", "MORE OPTIONS"}, {"aj.g_tema", "Theme"}, {"aj.g_tema_d", "Changes the color scheme."}, {"aj.g_flash", "Flash window on message"}, {"aj.g_flash_d", "The window flashes when a message arrives and it is not focused."}, {"aj.g_dev", "Developer mode"}, {"aj.g_dev_d", "Extra options for testing the app. Only if you know what you are doing."}, {"aj.g_bot", "Bot name"}, {"aj.g_bot_d", "Give the chat bot a fun name if you like."}, {"aj.g_discord", "Discord invite link"}, {"aj.g_discord_d", "Shown in chat with !discord."}, {"aj.g_bienvenida", "Welcome message"}, {"aj.g_bienvenida_d", "Guests see it when they join. Type _PLAYER_ to insert their name."}, {"aj.g_tts", "Read chat aloud"}, {"aj.g_tts_d", "Reads visible chat messages aloud with the Windows voice."}, {"aj.g_bonk", "Allow !bonk"}, {"aj.g_bonk_d", "Fun at first, but it can get old fast."}, {"aj.g_guia", "Disable Guide button"}, {"aj.g_guia_d", "The Guide button often opens overlays and can cause problems when hosting."}, {"aj.g_teclado", "Disable keyboard"}, {"aj.g_teclado_d", "Stops guests without a gamepad from playing with the keyboard."}, {"aj.g_autoindex", "Auto-index gamepads"}, {"aj.g_autoindex_d", "Finds XInput indices on its own. May cause blue screens on some PCs."}, {"aj.g_ip", "Also ban the IP"}, {"aj.g_ip_d", "When you ban someone, their IP address is blocked too."}, {"aj.g_vpn", "Block VPNs"}, {"aj.g_vpn_d", "Enable only if you have trouble with trolls. Some people use VPNs legitimately."}, {"aj.g_logs", "Parsec logs"}, {"aj.g_logs_d", "Shows Parsec logs in the Log window."}, {"aj.g_clasico_t", "More options"}, {"aj.g_clasico_d", "WebSocket, role permissions, sounds and the rest of the options."}, {"aj.g_clasico_b", "Show more options"}, {"aj.g_volver", "Back to General"}, {"web.volver_nueva", "Back to the new interface"}, {"web.no_disponible", "The new interface could not open. You are still on the usual one."}, {"web.reintentar_nueva", "Retry"}, {"web.instalar_webview2", "Install Microsoft component"}, {"web.ocultar_aviso", "Hide"}, {"aj.usar_nueva", "Use the new interface (HTML)"}, {"aj.usar_nueva_d", "New design with Match, Network, Friends and more. If your PC cannot show it, the app falls back to this one."}, {"aj.calidad", "STREAM QUALITY"}, {"aj.ligero", "Light"}, {"aj.ligero_d", "Weak internet or distant rivals"}, {"aj.equilibrado", "Balanced"}, {"aj.equilibrado_d", "Best for most people"}, {"aj.maxima", "Maximum"}, {"aj.maxima_d", "Fiber and nearby rivals"}, {"aj.activo", "ACTIVE"}, {"aj.actual", "Now:"}, {"aj.interfaz", "INTERFACE"}, {"aj.avanzado_hint", "Looking for finer control? It is in General, Video and Audio."},
			{"nav.comunidad", "Community"}, {"nav.ajustes", "Settings"},
			{"sub.sala", "Your room at a glance"}, {"sub.partido", "Match control and fairness"},
			{"sub.mandos", "Who controls what"}, {"sub.comunidad", "Moderation and players"},
			{"sub.ajustes", "Make the app yours"},
			{"tab.mandos", "Gamepads"}, {"tab.puppets", "Puppets"}, {"tab.hotseat", "Hotseat"},
			{"tab.bloqueo", "Button lock"}, {"tab.teclado", "Keyboard"}, {"tab.avanzado", "Advanced"}, {"teclado.presiona_tecla", "Press the key on your keyboard..."},
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
			{"sala.mandos", "GAMEPADS"}, {"sala.mando", "PAD"}, {"sala.libre", "Free"}, {"tab.resumen", "Summary"}, {"tab.actividad", "Activity"}, {"sala.latencia_vivo", "LIVE LATENCY"}, {"sala.transmision", "STREAM"}, {"sala.plazas", "Slots"}, {"tab.opciones", "Options"}, {"sala.host", "HOST"}, {"sala.invitado", "GUEST"}, {"sala.host_detalle", "Local control"}, {"sala.mando_n", "Pad %d · Parsec"}, {"sala.esperando", "Waiting…"}, {"sala.sin_rival", "No opponent"}, {"sala.comparte_enlace", "Share the link"}, {"sala.abre_sala", "Open the room first"}, {"sala.midiendo", "Measuring…"}, {"sala.enlace_estable", "Stable link"}, {"sala.enlace_regular", "Fair link"}, {"sala.enlace_inestable", "Unstable link"},
			{"sala.sin_mando", "Gamepad disconnected"}, {"sala.espectadores", "SPECTATORS"},
			{"sala.sin_espectadores", "Nobody watching yet."}, {"sala.avanzado", "Advanced settings"},
			{"sala.volver", "‹ Back to room"},
			{"web.conectado", "Connected to the web as"}, {"web.vinculando", "Linking this PC…"},
			{"web.sin_conexion", "No connection to the web: retrying"}, {"web.pausado", "Web connection paused"},
			{"web.sin_vincular", "This PC is not linked to your account"},
			{"web.ayuda_codigo", "On the web: My profile → link PC. Type the 6-digit code here."},
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
