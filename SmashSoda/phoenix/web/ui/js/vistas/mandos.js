// =============================================================================
//  MANDOS: Puestos (diseño v4, arrastrar para intercambiar), Teclado, Turnos,
//  Marionetas (mandos virtuales) y Bloqueo.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar } from "../tienda.js";
import { PuestosNuevo } from "./puestos.js";
import { BloqueoNuevo } from "./botones.js";
import { MarionetasNuevo } from "./marionetas.js";
import { TecladoNuevo } from "./teclado.js";
import { MapeoNuevo } from "./mapeo.js";
import { Tarjeta, Titulo, Boton, Interruptor, Ajuste, AjusteSw, Stepper, Icono, Vacio, Avatar, Chip, colorPing, cx, dos } from "../ui.js";

export const PESTANAS_MANDOS = [
  { id: "puestos", es: "PUESTOS", en: "SEATS", d: ["Arrastra a cada persona a un puesto, o toca «Asignar». Lo que dejes en «Mirando» solo ve la partida.", "Drag each person to a seat, or tap Assign. Anyone in Watching only watches."] },
  { id: "marionetas", es: "TU MANDO", en: "YOUR PAD", d: ["Elige el mando con el que juegas tú y cuántos mandos ve el juego.", "Pick the pad you play with and how many pads the game sees."] },
  { id: "mapeo", es: "MAPEO", en: "MAPPING", d: ["Corrige el mando de cada invitado: botones cambiados, stick que camina solo, sticks al revés, gatillos raros.", "Fix each guest's pad: swapped buttons, drifting stick, inverted sticks, odd triggers."] },
  { id: "teclado", mas: true, es: "TECLADO", en: "KEYBOARD", d: ["Quién puede jugar con teclado y qué tecla es cada botón del mando.", "Who can play with keyboard and which key is each pad button."] },
  { id: "turnos", mas: true, es: "TURNOS", en: "HOTSEAT", d: ["Rotación automática: cada uno juega un rato y cede el mando.", "Automatic rotation of the pads."] },
  { id: "bloqueo", mas: true, es: "BLOQUEO", en: "LOCK", d: ["Congela los mandos (pausas, revisión) o bloquea solo los botones que elijas.", "Freeze pads or lock only the buttons you pick."] },
];

export function metaMandos(s) {
  const lista = s.motor?.mandos?.lista || [];
  const usados = lista.filter((p) => p.ocupado).length;
  return html`<div style="display:flex;align-items:baseline;gap:10px">
    <span class="disp acc" style="font-size:34px;font-weight:700;line-height:1">${usados}</span>
    <span class="mono mut" style="font-size:14px">${t("DE", "OF")} ${lista.length} ${t("MANDOS EN USO", "PADS IN USE")}</span>
  </div>`;
}

export function VistaMandos({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.mandos) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  switch (pestana) {
    case "mapeo": return html`<${MapeoNuevo} m=${m}/>`;
    case "teclado": return html`<${TecladoNuevo} m=${m}/>`;
    case "turnos": return html`<${Turnos} m=${m}/>`;
    case "marionetas": return html`<${MarionetasNuevo} m=${m}/>`;
    case "bloqueo": return html`<${BloqueoNuevo} m=${m}/>`;
    default: return html`<${PuestosNuevo} m=${m}/>`;
  }
}


// ---- Avisos pendientes (solicitudes y entrada por Parsec) -------------------------------
export function Pendientes({ m }) {
  const sol = m.solicitudes || [];
  const esp = m.espera || [];
  if (!sol.length && !esp.length) return null;
  return html`<div class="col" style="gap:10px">
    ${esp.map((x) => html`<div class="caja" key=${"e" + x.parsecId} style="border-color:var(--warn);display:flex;align-items:center;gap:14px;flex-wrap:wrap">
      <${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${36}/>
      <div style="flex:1;min-width:200px"><b>${x.nombre}</b> <span class="mono mut">#${x.parsecId}</span>
        <div class="ayuda">${t("Entró por Parsec y no está en la lista de la web. ¿Qué hace?", "Joined via Parsec, not on the web list. What now?")}
        ${x.ping >= 0 ? html` · <span style=${`color:${colorPing(x.ping)}`}>${x.ping} ms</span>` : null}</div></div>
      <${Boton} tipo="ok mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "jugador" })}>${t("JUEGA", "PLAYS")}</${Boton}>
      <${Boton} tipo="mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "espectador" })}>${t("MIRA", "WATCHES")}</${Boton}>
      <${Boton} tipo="peligro mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "expulsar" })}>${t("EXPULSAR", "KICK")}</${Boton}>
    </div>`)}
    ${sol.map((x) => html`<div class="caja" key=${"s" + x.parsecId} style="border-color:var(--acc);display:flex;align-items:center;gap:14px;flex-wrap:wrap">
      <${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${36}/>
      ${x.mando < 0
        ? html`<div style="flex:1;min-width:200px"><b>${x.nombre}</b> ${t("quiere jugar", "wants to play")}
            <div class="ayuda">${t("Pulsó su mando estando en «Mirando». Si aceptas, toma el primer puesto libre.", "Pressed their pad while watching. If you accept, they take the first free seat.")}</div></div>`
        : html`<div style="flex:1;min-width:200px"><b>${x.nombre}</b> ${x.mando === 0 ? t("quiere cambiar de equipo", "wants to switch teams") : html`${t("pide el mando", "asks for pad")} <b class="acc">${dos(x.mando)}</b>`}
            <div class="ayuda">${t("Lo pidió con !cambio o !equipo en el chat.", "Requested with !cambio or !equipo.")}</div></div>`}
      <${Boton} tipo="ok mini" al=${() => accion("solicitud.aceptar", { parsecId: x.parsecId })}>${t("ACEPTAR", "ACCEPT")}</${Boton}>
      <${Boton} tipo="peligro mini" al=${() => accion("solicitud.rechazar", { parsecId: x.parsecId })}>${t("RECHAZAR", "DECLINE")}</${Boton}>
    </div>`)}
  </div>`;
}

// ---- Elegir quién toma un mando ---------------------------------------------------------
export function ElegirJugador({ m, indice, alCerrar }) {
  const ocupadoPor = new Map((m.mandos.lista || []).filter((p) => p.ocupado).map((p) => [p.parsecId, p.n]));
  const invitados = (m.invitados || []).filter((g) => !g.falso);
  const elegir = async (g) => {
    if (await accion("mandos.asignar", { indice, parsecId: g.parsecId })) alCerrar();
  };
  const tomarYo = async () => { if (await accion("mandos.tomar", { numero: indice + 1 })) alCerrar(); };
  return html`<div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
    <div class="dialogo" style="width:min(520px,100%)"><div class="cut"><div class="in" style="padding:24px">
      <h2>${t("¿Quién toma el mando", "Who takes pad")} ${dos(indice + 1)}?</h2>
      <div style="display:flex;flex-direction:column;gap:8px;max-height:56vh;overflow:auto;margin-top:8px">
        <button class="persona" onClick=${tomarYo}>
          <${Avatar} nombre=${m.sala?.hostNombre || "Yo"} brillo t=${38}/>
          <div style="flex:1;text-align:left"><div class="nom">${t("Yo (anfitrión)", "Me (host)")}</div>
            <div class="sub mut" style="font-size:12px">${t("Lo manejas con el teclado: flechas, Enter = A, Retroceso = B, Espacio = Start (atajo: Ctrl+Alt+número). Para usar tu mando físico, ve a Jugando › Tu mando.", "You drive it with the keyboard: arrows, Enter = A, Backspace = B, Space = Start (shortcut: Ctrl+Alt+number). To use your physical pad, go to Playing › Your pad.")}</div></div>
        </button>
        ${invitados.map((g) => html`<button class="persona" key=${g.parsecId} onClick=${() => elegir(g)} disabled=${g.rolWeb === "espectador"}>
          <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${m.perfiles?.[g.parsecId]?.avatar_url} t=${38}/>
          <div style="flex:1;text-align:left;min-width:0"><div class="nom">${g.nombre}</div>
            <div class="sub"><span class="mono mut" style="font-size:12px">#${g.parsecId}</span>
              ${ocupadoPor.has(g.parsecId) ? html`<${Chip} tipo="acc">${t("MANDO", "PAD")} ${dos(ocupadoPor.get(g.parsecId))}</${Chip}>` : null}
              ${g.rolWeb === "espectador" ? html`<${Chip}>${t("ESPECTADOR EN LA WEB", "WEB SPECTATOR")}</${Chip}>` : null}</div></div>
          <span class="mono" style=${`color:${colorPing(g.ping)}`}>${g.ping >= 0 ? g.ping + " ms" : "—"}</span>
        </button>`)}
        ${invitados.length === 0 ? html`<div class="ayuda" style="padding:10px">${t("Aún no hay invitados en la sala.", "No guests in the room yet.")}</div>` : null}
      </div>
      <div class="botones"><button class="btn suave" onClick=${alCerrar}>${t("Cancelar", "Cancel")}</button></div>
    </div></div></div>
  </div>`;
}

// ---- Puestos -------------------------------------------------------------------------------
function Turnos({ m }) {
  const tu = m.turnos || {};
  const aj = (d) => accion("turnos.ajustes", d);
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("TURNOS (HOTSEAT)", "HOTSEAT")} derecha=${html`<${Interruptor} valor=${tu.activo} al=${(v) => accion("turnos.activar", { si: v })} etiqueta="Hotseat"/>`}/>
      <div class="ayuda" style="margin-bottom:6px">${t("Cada invitado juega un rato y luego cede el mando al siguiente. Ideal para salas abiertas con muchos esperando.",
        "Each guest plays for a while and then hands over the pad.")}</div>
      <${Ajuste} titulo=${t("Minutos de juego", "Play minutes")} desc=${t("Tiempo con el mando antes de rotar.", "Time with the pad before rotating.")}>
        <${Stepper} valor=${tu.juegoMin ?? 10} min=${1} max=${999} al=${(v) => aj({ juegoMin: v })}/></${Ajuste}>
      <${Ajuste} titulo=${t("Minutos de descanso", "Cooldown minutes")} desc=${t("Cuándo puede volver a jugar (≥ minutos de juego).", "When they can play again.")}>
        <${Stepper} valor=${tu.reinicioMin ?? 30} min=${tu.juegoMin ?? 1} max=${999} al=${(v) => aj({ reinicioMin: v })}/></${Ajuste}>
      <${Ajuste} titulo=${t("Aviso antes de terminar", "Reminder")} desc=${t("Minutos antes del final (0 = sin aviso).", "Minutes before the end (0 = off).")}>
        <${Stepper} valor=${tu.recordatorioMin ?? 0} min=${0} max=${Math.max(0, (tu.juegoMin ?? 1) - 1)} al=${(v) => aj({ recordatorioMin: v })}/></${Ajuste}>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("EN TURNO", "IN ROTATION")} derecha=${html`<span class=${cx("chip", tu.corriendo ? "acc" : "")}>${tu.corriendo ? t("CORRIENDO", "RUNNING") : t("EN PAUSA", "IDLE")}</span>`}/>
      ${(tu.usuarios || []).length === 0 ? html`<div class="ayuda">${tu.activo ? t("Aparecen al tomar un mando.", "They show up when they take a pad.") : t("Activa los turnos para empezar.", "Enable hotseat to start.")}</div>` : null}
      <div style="display:flex;flex-direction:column;gap:8px">${(tu.usuarios || []).map((u) => html`<div class="persona" style="cursor:default" key=${u.parsecId}>
        <${Avatar} nombre=${u.nombre} id=${u.parsecId} t=${34}/>
        <div style="flex:1;min-width:0"><div class="nom">${u.nombre}</div>
          <div class="sub">${u.enAsiento ? html`<${Chip} tipo="acc">${t("JUGANDO", "PLAYING")}</${Chip}>` : null}${u.enfriamiento ? html`<${Chip} tipo="vip">${t("DESCANSO", "COOLDOWN")}</${Chip}>` : null}</div></div>
        <span class="mono" style="font-size:15px">${u.enfriamiento ? u.restanteEnfriamiento : u.restante || "—"}</span>
      </div>`)}</div>
      <div class="ayuda" style="margin-top:12px">${t("Comandos para moderadores: !extend, !decrease, !warmup, !cooldown.", "Mod commands: !extend, !decrease, !warmup, !cooldown.")}</div>
    </${Tarjeta}>
  </div>`;
}

// ---- Marionetas (mandos virtuales) --------------------------------------------------------------