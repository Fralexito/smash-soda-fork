// =============================================================================
//  MANDOS › BLOQUEO (rehecho): bloqueo general, bloqueo por mando y «Elegir
//  botones» con un mando dibujado donde se toca cada botón para bloquearlo.
//  Todo dentro de la interfaz nueva (ya no salta al panel clásico).
// =============================================================================
import { html } from "../lib.js";
import { t } from "../i18n.js";
import { accion } from "../tienda.js";
import { Tarjeta, Titulo, Boton, AjusteSw, Icono, cx, dos } from "../ui.js";

// Máscara de bits de los botones (los mismos que usa el motor de mandos).
const BIT = { arriba: 0x1, abajo: 0x2, izq: 0x4, der: 0x8, start: 0x10, back: 0x20, ls: 0x40, rs: 0x80, lb: 0x100, rb: 0x200, guia: 0x400, a: 0x1000, b: 0x2000, x: 0x4000, y: 0x8000 };
const TODOS_BITS = Object.values(BIT).reduce((a, b) => a | b, 0);
const VACIO = { mascara: 0, lt: false, rt: false, lx: false, ly: false, rx: false, ry: false };

// Cada control: etiqueta + a qué pertenece (un bit, un gatillo o los dos ejes de un stick).
const C = {
  lt: { et: "LT", gatillo: "lt", nom: ["Gatillo izquierdo", "Left trigger"] },
  rt: { et: "RT", gatillo: "rt", nom: ["Gatillo derecho", "Right trigger"] },
  lb: { et: "LB", bit: BIT.lb, nom: ["Botón superior izquierdo", "Left bumper"] },
  rb: { et: "RB", bit: BIT.rb, nom: ["Botón superior derecho", "Right bumper"] },
  guia: { et: "GUÍA", bit: BIT.guia, nom: ["Botón Guía (Xbox)", "Guide button"] },
  back: { et: "BACK", bit: BIT.back, nom: ["Back / Select", "Back / Select"] },
  start: { et: "START", bit: BIT.start, nom: ["Start (pausa)", "Start (pause)"] },
  arriba: { et: "▲", bit: BIT.arriba, nom: ["Cruceta arriba", "D-pad up"] },
  abajo: { et: "▼", bit: BIT.abajo, nom: ["Cruceta abajo", "D-pad down"] },
  izq: { et: "◀", bit: BIT.izq, nom: ["Cruceta izquierda", "D-pad left"] },
  der: { et: "▶", bit: BIT.der, nom: ["Cruceta derecha", "D-pad right"] },
  a: { et: "A", bit: BIT.a, nom: ["Botón A", "A button"] },
  b: { et: "B", bit: BIT.b, nom: ["Botón B", "B button"] },
  x: { et: "X", bit: BIT.x, nom: ["Botón X", "X button"] },
  y: { et: "Y", bit: BIT.y, nom: ["Botón Y", "Y button"] },
  stickL: { et: "STICK", ejes: ["lx", "ly"], nom: ["Palanca izquierda (mover)", "Left stick (move)"], redondo: true },
  stickR: { et: "STICK", ejes: ["rx", "ry"], nom: ["Palanca derecha (mover)", "Right stick (move)"], redondo: true },
  ls: { et: "L3", bit: BIT.ls, nom: ["Pulsar palanca izquierda", "Left stick click"] },
  rs: { et: "R3", bit: BIT.rs, nom: ["Pulsar palanca derecha", "Right stick click"] },
};
const TOTAL = Object.keys(C).length;

function leer(m) {
  const b = m.mandos?.botonesBloq;
  return b ? { ...VACIO, ...b } : { ...VACIO };
}
function bloqueado(c, b) {
  if (c.bit) return (b.mascara & c.bit) !== 0;
  if (c.gatillo) return !!b[c.gatillo];
  return c.ejes.every((k) => !!b[k]);
}
function conCambio(c, b, valor) {
  const n = { ...b };
  if (c.bit) n.mascara = valor ? (n.mascara | c.bit) : (n.mascara & ~c.bit);
  else if (c.gatillo) n[c.gatillo] = valor;
  else for (const k of c.ejes) n[k] = valor;
  return n;
}
const enviar = (n) => accion("mandos.botonesBloq", n);

function Tecla({ id, b, estilo = "" }) {
  const c = C[id];
  const bl = bloqueado(c, b);
  return html`<button class=${cx("tecla-m", bl && "bloq", c.redondo && "redondo")} style=${estilo}
    title=${(bl ? t("Bloqueado: ", "Locked: ") : t("Libre: ", "Free: ")) + t(c.nom[0], c.nom[1])}
    aria-pressed=${bl} onClick=${() => enviar(conCambio(c, b, !bl))}>
    <span class="et">${c.et}</span>
    ${bl ? html`<span class="cand"><${Icono} n="candado" t=${13} w=${2}/></span>` : null}
  </button>`;
}

function MandoDibujado({ b }) {
  const T = (id, estilo) => html`<${Tecla} id=${id} b=${b} estilo=${estilo || ""}/>`;
  return html`<div class="mando-m">
    <div class="mm-fila mm-sup">
      <div class="mm-par">${T("lt")}${T("lb")}</div>
      <div class="mm-centro">${T("guia")}<div class="mm-bs">${T("back")}${T("start")}</div></div>
      <div class="mm-par">${T("rt")}${T("rb")}</div>
    </div>
    <div class="mm-fila mm-inf">
      <div class="mm-grupo">
        ${T("stickL")}
        ${T("ls", "margin-top:6px")}
        <div class="mm-cruz">
          <span></span>${T("arriba")}<span></span>
          ${T("izq")}<span class="mm-hueco"></span>${T("der")}
          <span></span>${T("abajo")}<span></span>
        </div>
      </div>
      <div class="mm-grupo">
        ${T("stickR")}
        ${T("rs", "margin-top:6px")}
        <div class="mm-cruz">
          <span></span>${T("y")}<span></span>
          ${T("x")}<span class="mm-hueco"></span>${T("b")}
          <span></span>${T("a")}<span></span>
        </div>
      </div>
    </div>
  </div>`;
}

function BotonesElegidos({ m }) {
  const b = leer(m);
  const n = Object.values(C).filter((c) => bloqueado(c, b)).length;
  const aplicado = m.mandos.bloqueoBotones;
  const todo = { mascara: TODOS_BITS, lt: true, rt: true, lx: true, ly: true, rx: true, ry: true };
  const pausa = { ...VACIO, mascara: BIT.start | BIT.back | BIT.guia };
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:16px">
    <${Titulo} texto=${t("ELEGIR BOTONES", "CHOOSE BUTTONS")}
      derecha=${html`<span class="mono mut">${n} ${t("DE", "OF")} ${TOTAL} ${t("BLOQUEADOS", "LOCKED")}</span>`}/>
    <div class="ayuda" style="font-size:14px">
      ${t("Toca un botón del mando para bloquearlo (rojo con candado). Un botón bloqueado deja de funcionar para todos, pero solo cuando el bloqueo está encendido.",
          "Tap a button to lock it (red with a padlock). A locked button stops working for everyone, only while the lock is on.")}
    </div>
    <${AjusteSw} titulo=${t("Bloqueo de botones encendido", "Button lock on")}
      desc=${aplicado ? t("Ahora mismo los botones en rojo NO funcionan.", "Right now the red buttons do NOT work.")
                      : t("Apagado: todos los botones funcionan aunque estén marcados.", "Off: every button works even if marked.")}
      valor=${aplicado} deshabilitado=${m.mandos.esclavo} al=${() => accion("mandos.herramienta", { nombre: "bloquearBotones" })}/>
    <${MandoDibujado} b=${b}/>
    <div class="leyenda-m">
      <span><i class="pt"></i>${t("Libre", "Free")}</span>
      <span><i class="pt bloq"></i>${t("Bloqueado", "Locked")}</span>
    </div>
    <div style="display:flex;gap:10px;flex-wrap:wrap">
      <${Boton} tipo="suave mini" al=${() => enviar(pausa)}>${t("SOLO START · BACK · GUÍA", "ONLY START · BACK · GUIDE")}</${Boton}>
      <${Boton} tipo="suave mini" al=${() => enviar(todo)}>${t("BLOQUEAR TODO", "LOCK ALL")}</${Boton}>
      <${Boton} tipo="suave mini" al=${() => enviar(VACIO)}>${t("LIBERAR TODO", "UNLOCK ALL")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

// Árbitro del partido: lo decide Link mirando la fase del partido (la del juego o, si no hay datos, la del marcador).
function Arbitro({ m }) {
  const a = m.arbitro || {};
  const juego = m.juego?.datos;
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("ÁRBITRO DEL PARTIDO", "MATCH REFEREE")}
      derecha=${a.activo ? html`<span class="mono" style="color:var(--ok)">${t("START BLOQUEADO", "START LOCKED")}</span>` : null}/>
    <${AjusteSw} titulo=${t("Modo competitivo", "Competitive mode")}
      desc=${t("Con el partido en juego, los invitados no pueden usar Start, Back ni Guía. Al parar vuelve tu bloqueo de siempre.",
               "While the match is live, guests can't use Start, Back or Guide. Your usual lock comes back afterwards.")}
      valor=${!!a.competitivo} al=${() => accion("mandos.competitivo", { si: !a.competitivo })}/>
    <${AjusteSw} titulo=${t("Marcador automático", "Automatic scoreboard")}
      desc=${t("Con el módulo Phoenix Estadio en el juego: el marcador arranca solo, cada gol se anuncia en el chat con su minuto y al final se guarda en el historial.",
               "With the Phoenix Estadio game module: the scoreboard starts by itself, each goal is announced with its minute and the result is saved at full time.")}
      valor=${!!a.marcadorAuto} al=${() => accion("mandos.marcadorAuto", { si: !a.marcadorAuto })}/>
    <${AjusteSw} titulo=${t("Pausa automática (prueba)", "Auto pause (test)")}
      desc=${juego ? t("Si un jugador del partido se cae o pasa de 150 ms de ping 5 s: aviso en el chat, pausa del marcador y Start en el juego.",
                       "If a match player drops or stays above 150 ms for 5 s: chat notice, scoreboard pause and Start in the game.")
                   : t("Si un jugador del partido se cae o pasa de 150 ms de ping 5 s: aviso en el chat y pausa del marcador. Sin datos del juego no se pulsa nada.",
                       "If a match player drops or stays above 150 ms for 5 s: chat notice and scoreboard pause. Without game data nothing is pressed.")}
      valor=${!!a.pausaAuto} al=${() => accion("mandos.pausaAuto", { si: !a.pausaAuto })}/>
    ${a.ultimaPausa ? html`<div class="ayuda" style="margin-top:8px">${t("Última pausa automática:", "Last auto pause:")} ${a.ultimaPausa}${a.conStart ? t(" (Start pulsado)", " (Start pressed)") : ""}</div>` : null}
  </${Tarjeta}>`;
}

export function BloqueoNuevo({ m }) {
  const lista = m.mandos.lista || [];
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:1 1 0;min-width:300px">
      <${Tarjeta} interior="padding:22px 24px">
        <${Titulo} texto=${t("BLOQUEO GENERAL", "GLOBAL LOCK")}/>
        <${AjusteSw} titulo=${t("Bloquear todos los mandos", "Lock all pads")} desc=${t("Nadie puede mover nada (pausas, árbitro, revisión).", "Nobody can move (pauses, review).")}
          valor=${m.mandos.bloqueoGlobal} deshabilitado=${m.mandos.esclavo} al=${() => accion("mandos.herramienta", { nombre: "bloquearTodo" })}/>
      </${Tarjeta}>
      <div class="ayuda">${t("Para bloquear un solo mando usa el candado de su puesto en Mandos › Puestos.", "To lock a single pad use the padlock on its seat in Pads › Seats.")}</div>
      <${Arbitro} m=${m}/>
    </div>
    <div class="col" style="flex:1.5 1 0;min-width:340px">
      <${BotonesElegidos} m=${m}/>
    </div>
  </div>`;
}
