// =============================================================================
//  «¿QUÉ HAGO AQUÍ?» (segunda pasada visual): arriba de cada pantalla, una frase
//  que responde a esa pregunta según lo que pasa AHORA, y el botón principal
//  destacado (solo si la pantalla no lo tiene ya grande arriba). Sustituye a la línea fina de descripción (.guia). Si una pantalla
//  no tiene acción principal, solo va la frase (la de PESTANAS_*.d de siempre).
// =============================================================================
import { html } from "../lib.js";
import { t } from "../i18n.js";
import { accion, irA } from "../tienda.js";
import { Boton, Icono, cx } from "../ui.js";
import { mejorSala } from "./gente.js";

const ICONO = { inicio: "inicio", sala: "sala", gente: "gente", partido: "partido", mandos: "mandos", jugar: "jugar", sync: "sync", ajustes: "ajustes" };

// Frase de acción por pantalla (si no hay un caso según el estado). Responde a «¿qué hago aquí?», no a «¿qué hay aquí?».
const FRASES = {
  "inicio/inicio": ["Elige: JUGAR CON UN AMIGO o BUSCAR RIVAL. Si falta algo, el semáforo de la derecha te lo dice.", "Pick: PLAY WITH A FRIEND or FIND A RIVAL. If anything is missing, the check on the right tells you."],
  "gente/moderacion": ["Quita un ban, un moderador o un VIP con su botón. Nada cambia hasta que lo pulses.", "Remove a ban, a moderator or a VIP with its button. Nothing changes until you press it."],
  "gente/permisos": ["Enciende o apaga lo que cada rol puede hacer desde el chat. Se guarda solo.", "Turn on or off what each role can do from chat. Saved automatically."],
  "sala/juegos": ["Elige qué se juega: sale en la web para que te encuentren. La biblioteca solo hace falta para el modo quiosco.", "Pick what is played: it shows on the website so people find you. The library is only for kiosk mode."],
  "sala/actividad": ["Lee lo que pasó en la sala. Usa el filtro para buscar a alguien.", "Read what happened in the room. Use the filter to find someone."],
  "partido/campo": ["Solo mira: el campo se dibuja solo mientras se juega en PES.", "Just watch: the pitch draws itself while PES is being played."],
  "partido/historial": ["Mira cómo quedaron los partidos y quién va primero en la tabla de la sala.", "See how matches ended and who leads the room table."],
  "mandos/bloqueo": ["Deja el árbitro encendido en partidos serios (nadie pausa con Start). Bloquea un mando si alguien molesta.", "Keep the referee on for serious matches (nobody pauses with Start). Lock a pad if someone is bothering."],
  "mandos/marionetas": ["Pulsa USAR ESTE en tu mando físico: así juegas tú con él en cualquier puesto.", "Press USE THIS on your physical pad: then you play with it on any seat."],
  "mandos/mapeo": ["Elige al invitado cuyo mando va raro y sigue los pasos: Link lo corrige solo.", "Pick the guest whose pad acts up and follow the steps: Link fixes it."],
  "mandos/teclado": ["Da permiso de teclado a quien no tenga mando y elige qué tecla es cada botón.", "Allow keyboard for whoever has no pad and pick which key is each button."],
  "mandos/turnos": ["Enciéndelo si hay más gente que mandos: cada uno juega un rato y cede el mando.", "Turn it on when there are more people than pads: each plays a while and hands over."],
  "sala/puesto": ["Elige dónde te sientas tú cuando pulsas JUGAR.", "Pick where you sit when you press PLAY."],
  "jugar/salas": ["Elige una sala y pulsa UNIRME. Las de tus amigos y donde te invitaron salen primero.", "Pick a room and press JOIN. Rooms from friends and invites come first."],
  "jugar/amigos": ["Invita a un amigo con un toque: le llega el aviso en la web aunque no esté conectado.", "Invite a friend in one tap: they get the notice on the website even if offline."],
  "jugar/perfiles": ["Guarda el nombre y los controles de cada amigo en PES: Link se los pone solo cuando se sienta.", "Save each friend's PES name and controls: Link applies them when they sit."],
  "sync/traspasos": ["Pasa un fichaje a un amigo: haz el cambio en Editar de PES y aparece en su juego.", "Pass a transfer to a friend: make the change in PES Edit and it shows up in their game."],
  "ajustes/general": ["Cambia lo fundamental: idioma, tema y quién se sienta solo. Lo demás casi nunca hace falta.", "Change the basics: language, theme and who seats themselves. The rest is rarely needed."],
  "ajustes/overlay": ["Enciende el overlay para ver el chat y los mandos encima de PES (funciona con la sala abierta).", "Turn on the overlay to see chat and pads on top of PES (works while the room is open)."],
  "ajustes/sonidos": ["Elige qué suena cuando alguien entra y quién puede usar !sfx.", "Pick what sounds when someone joins and who can use !sfx."],
  "ajustes/interfaz": ["Cambia el idioma o vuelve a ver la bienvenida y el tutorial.", "Change the language or see the welcome and the tutorial again."],
  "ajustes/avanzado": ["Solo para herramientas externas o si te lo pide quien te ayuda.", "Only for external tools or if whoever helps you asks."],
};

/** { frase, boton: [texto, fn] | null, tono: "" | "ok" | "warn" } para esa pantalla ahora mismo. */
function queHago(m, sec, pes, d) {
  const sala = m?.sala || {};
  const k = sec + "/" + pes;
  const f = FRASES[k] || d;
  const base = { frase: f ? t(f[0], f[1]) : "", boton: null, tono: "" };
  switch (k) {
    case "sala/resumen":
      if (sala.abierta) return { frase: t("Tu sala está abierta: comparte el enlace y mira quién entra.", "Your room is open: share the link and see who joins."), boton: null, tono: "ok" };
      return { frase: t("Abre tu sala y manda el enlace a tus amigos. Tú decides quién juega.", "Open your room and send the link to your friends. You decide who plays."), boton: null };
    case "sala/transmision":
      return (sala.calidad?.auto ?? true)
        ? { ...base, frase: t("Elige LIGERO, EQUILIBRADO o MÁXIMA. Con «Automático» encendido, Link reparte tu internet solo.", "Pick LIGHT, BALANCED or MAX. With «Automatic» on, Link shares your internet by itself.") }
        : { frase: t("La calidad está en manual: con varios amigos a cada uno le llega poca imagen.", "Quality is manual: with several friends each gets a poor picture."), boton: [t("PONER AUTOMÁTICO", "MAKE AUTOMATIC"), () => accion("sala.anchoAuto", { auto: true })], tono: "warn" };
    case "sala/opciones":
      return sala.opciones?.pendiente ? { frase: t("Hay cambios sin aplicar: tus invitados aún no los ven.", "There are pending changes: your guests don't see them yet."), boton: null, tono: "warn" } : base;
    case "sala/red":
      return { frase: t("Todo lo que hace falta para jugar, en una lista. Si algo sale en rojo o ámbar, pulsa su botón.", "Everything needed to play, in one list. If something is red or amber, press its button."), boton: null };
    case "gente/sala":
      return sala.abierta ? { ...base, frase: t("Toca a alguien para darle mando, hacerlo VIP o expulsarlo.", "Tap someone to give them a pad, make them VIP or kick them.") }
        : { frase: t("La sala está cerrada: aquí verás a cada invitado cuando entre.", "The room is closed: each guest shows up here when they join."), boton: [t("JUGAR CON UN AMIGO", "PLAY WITH A FRIEND"), () => irA("inicio", "inicio")] };
    case "partido/vivo": {
      const f = m?.partido?.fase || "libre";
      if (f === "listo") return { frase: t("Los lados están listos. Empieza cuando estéis todos.", "Sides are ready. Kick off when everyone's in."), boton: null, tono: "ok" };
      if (f === "en_juego" || f === "pausado") return { ...base, frase: t("Partido en marcha: el marcador y los lados van solos. Tú solo miras.", "Match on: score and sides run by themselves. You just watch."), tono: "ok" };
      return m?.arbitro?.ladosAuto && m?.arbitro?.marcadorAuto
        ? { ...base, frase: t("No hay que hacer nada: el partido se arma solo al pitazo en PES.", "Nothing to do: the match sets itself up at kick-off in PES.") }
        : { ...base, frase: t("Elige quién va en cada lado y pulsa PREPARAR PARTIDO.", "Pick who goes on each side and press SET UP MATCH.") };
    }
    case "mandos/puestos":
      return (m?.solicitudes || []).length
        ? { frase: t("Alguien pide jugar: acéptalo abajo o deja el sentado automático encendido.", "Someone wants to play: accept below or keep auto-seat on."), boton: null, tono: "warn" }
        : { ...base, frase: t("Cada puesto es un mando del juego (puesto 1 = mando 1). Arrastra a cada persona a su puesto.", "Each seat is a game pad (seat 1 = pad 1). Drag each person to their seat.") };
    case "mandos/vivo":
      return { ...base, frase: t("Mira qué pulsa cada uno ahora mismo. Si un mando va raro, tócalo y se arregla en Mapeo.", "See what each player presses right now. If a pad acts up, tap it to fix it in Mapping.") };
    case "jugar/salas": {
      const x = mejorSala(m || {});
      return x ? { frase: t(`La mejor sala para ti ahora es la de ${x.hostNombre}.`, `The best room for you now is ${x.hostNombre}'s.`), boton: null, tono: "ok" } : base;
    }
    case "sync/mipc":
      return { frase: t("Deja tu juego listo para los fichajes. Un botón lo hace todo (con copia y nunca con PES abierto).", "Get your game ready for transfers. One button does it all (backed up, never with PES open)."), boton: null };
    case "sync/avanzado":
      return { ...base, frase: t("Solo para cuando algo falla o quieres tocar algo concreto. Lo normal se hace en «Mi juego».", "Only for when something fails or you want to tweak something. The usual is done in «My game».") };
    default:
      return base;
  }
}

/** La franja de arriba de cada pantalla. */
export function QueHago({ s, sec, pestana, d }) {
  const x = queHago(s.motor, sec, pestana, d);
  if (!x.frase && !x.boton) return null;
  return html`<div class=${cx("que-hago", x.tono)} key=${"q" + sec + pestana}>
    <span class="qh-icono" aria-hidden="true"><${Icono} n=${ICONO[sec] || "rayo"} t=${18}/></span>
    <span class="qh-frase">${x.frase}</span>
    ${x.boton ? html`<${Boton} tipo="lleno mini qh-boton" al=${x.boton[1]}>${x.boton[0]}</${Boton}>` : null}
  </div>`;
}
