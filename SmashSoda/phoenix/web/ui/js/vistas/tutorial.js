// =============================================================================
//  TUTORIAL GUIADO: la primera vez que se abre Phoenix Link, un recorrido corto
//  que ilumina cada parte (sala, puestos, tu mando, orden de mandos, partido,
//  gente, sync, ajustes, Ctrl+K) con una explicación. Se puede omitir y volver
//  a ver cuando se quiera (Ajustes › Interfaz y Ctrl+K → abrirTutorial()).
//  Solo guarda si ya se vio (localStorage, en este PC).
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { irA } from "../tienda.js";
import { Boton } from "../ui.js";

const CLAVE = "phoenix.tutorial.visto";
const EVENTO = "phoenix:tutorial";

/** Abre el tutorial desde cualquier sitio (botón de Ajustes, paleta Ctrl+K…). */
export function abrirTutorial() { window.dispatchEvent(new CustomEvent(EVENTO)); }

const porTexto = (selector, re) => () => [...document.querySelectorAll(selector)].find((e) => re.test(e.textContent.trim()) && e.getClientRects().length);
const nav = (n) => () => document.querySelectorAll(".menu .nv")[n];

const PASOS = [
  { titulo: ["Bienvenido a Phoenix Link", "Welcome to Phoenix Link"],
    texto: ["En un minuto te enseño dónde está cada cosa. Puedes omitirlo y volver a verlo cuando quieras desde Ajustes › Interfaz.", "In one minute I'll show you where everything is. Skip it and replay it any time from Settings › Interface."] },
  { ir: ["sala", "resumen"], foco: nav(0), titulo: ["Sala", "Room"],
    texto: ["Aquí abres tu sala de Parsec, copias el enlace para tus amigos y ves de un vistazo quién entra y cómo va la conexión.", "Open your Parsec room, copy the link for your friends and see who joins and how the connection is doing."] },
  { ir: ["sala", "resumen"], foco: porTexto("main button", /^(ABRIR SALA|DETENER SALA|OPEN ROOM|STOP ROOM)$/), titulo: ["Abrir la sala", "Open the room"],
    texto: ["Este botón crea tu sala. Después, «Copiar enlace» y lo mandas por Discord o WhatsApp.", "This button creates your room. Then «Copy link» and send it on Discord or WhatsApp."] },
  { ir: ["mandos", "puestos"], foco: nav(2), titulo: ["Mandos › Puestos", "Pads › Seats"],
    texto: ["Cada puesto es un mando del juego. Arrastra a cada persona a un puesto; quien quede en «Mirando» solo ve la pantalla. Tú también apareces como TÚ.", "Each seat is a game pad. Drag each person to a seat; anyone in «Watching» only sees the screen. You show up as YOU."] },
  { ir: ["mandos", "puestos"], foco: porTexto(".caja", /^ORDEN DE MANDOS EN PES/), titulo: ["Orden de mandos en PES", "Pad order in PES"],
    texto: ["Enciéndelo y Link ordena el settings.dat de PES solo: el puesto 1 es el jugador 1, el 2 el jugador 2… Cierra DS4Windows mientras usas Link.", "Turn it on and Link sorts PES's settings.dat: seat 1 is player 1, seat 2 is player 2… Close DS4Windows while using Link."] },
  { ir: ["mandos", "marionetas"], foco: porTexto("button", /^(TU MANDO|YOUR PAD)$/), titulo: ["Tu mando", "Your pad"],
    texto: ["Elige una vez el mando con el que juegas tú. Luego te arrastras a cualquier puesto y tu mando mueve ese puesto.", "Pick the pad you play with once. Then drag yourself to any seat and your pad drives it."] },
  { ir: ["partido"], foco: nav(1), titulo: ["Partido", "Match"],
    texto: ["Marcador, lados Local y Visita, modo competitivo (Start bloqueado) y el historial de partidos.", "Score, Home and Away sides, competitive mode (Start locked) and match history."] },
  { ir: ["gente"], foco: nav(3), titulo: ["Gente", "People"],
    texto: ["Quién está en tu sala, su ficha, permisos, moderadores y baneos.", "Who's in your room, their card, permissions, moderators and bans."] },
  { ir: ["sync"], foco: nav(4), titulo: ["Sync", "Sync"],
    texto: ["Tu cuenta de la liga, el puente con el juego (avisos dentro de PES) y los fichajes de Phoenix Sync.", "Your league account, the game bridge (notices inside PES) and Phoenix Sync transfers."] },
  { ir: ["ajustes"], foco: nav(5), titulo: ["Ajustes", "Settings"],
    texto: ["Todo lo demás: bot del chat, video, audio, overlay e interfaz. En Ajustes › Interfaz puedes volver a ver este tutorial.", "Everything else: chat bot, video, audio, overlay and interface. Replay this tutorial from Settings › Interface."] },
  { foco: () => document.querySelector(".pill.atajo"), titulo: ["Buscar con Ctrl+K", "Search with Ctrl+K"],
    texto: ["¿No sabes dónde está algo? Pulsa Ctrl+K y escribe: te lleva directo a cualquier ajuste o acción.", "Can't find something? Press Ctrl+K and type: it takes you straight to any setting or action."] },
  { titulo: ["¡Listo!", "All set!"],
    texto: ["Ya conoces Phoenix Link. Empieza por Sala › Abrir sala. Si alguna vez te pierdes, Ctrl+K o este tutorial te ayudan.", "You know Phoenix Link now. Start with Room › Open room. If you ever get lost, Ctrl+K or this tutorial help."] },
];

export function Tutorial({ s }) {
  const [paso, setPaso] = useState(null);   // null = cerrado
  const [rect, setRect] = useState(null);

  // Primera vez: se abre solo cuando el motor ya respondió (y nunca con la PC cedida)
  useEffect(() => {
    const abrir = () => setPaso(0);
    window.addEventListener(EVENTO, abrir);
    let visto = true;
    try { visto = localStorage.getItem(CLAVE) === "1"; } catch { /* sin almacenamiento: no insistir */ }
    const id = !visto ? setTimeout(() => { if (!document.querySelector(".tour-caja")) setPaso(0); }, 1500) : 0;
    return () => { window.removeEventListener(EVENTO, abrir); clearTimeout(id); };
  }, []);

  const cerrar = () => { setPaso(null); setRect(null); try { localStorage.setItem(CLAVE, "1"); } catch { /* nada */ } };
  const p = paso != null ? PASOS[paso] : null;

  // Cada paso: ir a su sección y medir lo que se ilumina (cuando ya se pintó)
  useEffect(() => {
    if (!p) return undefined;
    if (p.ir) irA(p.ir[0], p.ir[1]);
    let vivo = true;
    const medir = () => {
      if (!vivo) return;
      const el = p.foco ? p.foco() : null;
      if (el) { el.scrollIntoView({ block: "nearest", inline: "nearest" }); const r = el.getBoundingClientRect(); setRect({ x: r.left, y: r.top, w: r.width, h: r.height }); }
      else setRect(null);
    };
    const id = setTimeout(medir, 350);
    window.addEventListener("resize", medir);
    return () => { vivo = false; clearTimeout(id); window.removeEventListener("resize", medir); };
  }, [paso]);

  // Teclado: ← → para moverse, Esc para salir
  useEffect(() => {
    if (paso == null) return undefined;
    const tecla = (e) => {
      if (e.key === "Escape") { e.preventDefault(); cerrar(); }
      else if (e.key === "ArrowRight") { e.preventDefault(); paso < PASOS.length - 1 ? setPaso(paso + 1) : cerrar(); }
      else if (e.key === "ArrowLeft" && paso > 0) { e.preventDefault(); setPaso(paso - 1); }
    };
    window.addEventListener("keydown", tecla, true);
    return () => window.removeEventListener("keydown", tecla, true);
  }, [paso]);

  if (!p || s?.motor?.cedido?.activo) return null;
  const ultimo = paso === PASOS.length - 1;
  const M = 8;
  // La caja va debajo de lo iluminado si cabe; si no, encima; sin foco, al centro
  let caja = "left:50%;top:50%;transform:translate(-50%,-50%)";
  if (rect) {
    const ancho = 380, bajo = rect.y + rect.h + M + 14;
    const x = Math.max(16, Math.min(window.innerWidth - ancho - 16, rect.x + rect.w / 2 - ancho / 2));
    caja = bajo + 230 < window.innerHeight ? `left:${x}px;top:${bajo}px` : `left:${x}px;top:${Math.max(16, rect.y - M - 14 - 230)}px`;
    if (rect.x + rect.w < 200) caja = `left:${rect.x + rect.w + M + 14}px;top:${Math.max(16, Math.min(window.innerHeight - 250, rect.y - 20))}px`;   // menú lateral: a su derecha
  }
  return html`<div class="tour" role="dialog" aria-modal="true" aria-labelledby="tour-titulo">
    ${rect ? html`<div class="tour-foco" style=${`left:${rect.x - M}px;top:${rect.y - M}px;width:${rect.w + 2 * M}px;height:${rect.h + 2 * M}px`}></div>`
      : html`<div class="tour-velo"></div>`}
    <div class="tour-caja" style=${caja}>
      <div class="tour-cab">
        <span class="mono acc">${t("PASO", "STEP")} ${paso + 1} / ${PASOS.length}</span>
        <button class="tour-omitir" onClick=${cerrar}>${ultimo ? t("CERRAR", "CLOSE") : t("OMITIR", "SKIP")}</button>
      </div>
      <div id="tour-titulo" class="tour-titulo">${t(p.titulo[0], p.titulo[1])}</div>
      <div class="tour-texto">${t(p.texto[0], p.texto[1])}</div>
      <div class="tour-puntos" aria-hidden="true">${PASOS.map((_, i) => html`<i key=${i} class=${i === paso ? "on" : i < paso ? "hecho" : ""}></i>`)}</div>
      <div class="tour-botones">
        ${paso > 0 ? html`<${Boton} tipo="suave" al=${() => setPaso(paso - 1)}>${t("ATRÁS", "BACK")}</${Boton}>` : html`<span></span>`}
        <${Boton} tipo="lleno" al=${() => (ultimo ? cerrar() : setPaso(paso + 1))}>${ultimo ? t("EMPEZAR", "START") : paso === 0 ? t("VAMOS", "LET'S GO") : t("SIGUIENTE", "NEXT")}</${Boton}>
      </div>
    </div>
  </div>`;
}
