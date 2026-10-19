// Paleta de acciones (Ctrl+K): ir a cualquier pantalla o ejecutar lo más usado.
import { abrirTutorial } from "./tutorial.js";
import { html, useState, useEffect, useRef, useMemo } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar, irA } from "../tienda.js";
import { Icono, cx } from "../ui.js";
import { INDICE_AJUSTES } from "../indice-ajustes.js";
import { GRUPOS, rutaDe } from "../grupos.js";

const limpio = (x) => String(x || "").replace(/\s+/g, " ").trim().toLowerCase();
/** Tras ir a la pestaña: despliega la tarjeta si estaba plegada y resalta el ajuste. */
function resaltar(texto, tarjeta) {
  const buscar = (intento) => {
    if (tarjeta) {
      const cab = [...document.querySelectorAll(".titulo-tarjeta.cerrado")].find((c) => limpio(c.textContent).includes(limpio(tarjeta)));
      if (cab) cab.querySelector(".titulo-btn")?.click();
    }
    const el = [...document.querySelectorAll(".vista .ajuste .t, .vista .lab")].find((e) => limpio(e.textContent) === limpio(texto))
      || [...document.querySelectorAll(".vista .ajuste .t, .vista .lab")].find((e) => limpio(e.textContent).startsWith(limpio(texto)));
    if (!el) { if (intento < 6) setTimeout(() => buscar(intento + 1), 150); return; }
    const caja = el.closest(".ajuste") || el.closest(".titulo-tarjeta") || el;
    caja.scrollIntoView({ block: "center", behavior: matchMedia("(prefers-reduced-motion: reduce)").matches ? "auto" : "smooth" });
    caja.classList.remove("resaltado"); void caja.offsetWidth; caja.classList.add("resaltado");
    setTimeout(() => caja.classList.remove("resaltado"), 1800);
  };
  setTimeout(() => buscar(0), 200);
}
import { abrirSala, cerrarSala, aplicarPresetCalidad } from "./sala.js";

function opciones(m) {
  const sala = m?.sala;
  const r = [];
  const ir = (sec, pes, es, en, icono) => r.push({ es, en, icono, k: t("IR", "GO"), hacer: () => irA(sec, pes) });
  r.push({ es: "Ver el tutorial de Phoenix Link", en: "Show the Phoenix Link tutorial", icono: "rayo", hacer: abrirTutorial });
  if (sala && !sala.abierta && sala.lista) r.push({ es: "Abrir sala", en: "Open room", icono: "rayo", hacer: abrirSala });
  if (sala?.abierta) {
    r.push({ es: "Copiar enlace de la sala", en: "Copy room link", icono: "copiar", hacer: () => accion("sala.copiarEnlace", {}, { ok: t("Enlace copiado.", "Link copied.") }) });
    r.push({ es: "Detener sala", en: "Stop room", icono: "cerrar", hacer: () => cerrarSala(m) });
  }
  r.push({ es: "Bloquear / desbloquear todos los mandos", en: "Lock / unlock all pads", icono: "candado", hacer: () => accion("mandos.herramienta", { nombre: "bloquearTodo" }) });
  r.push({ es: "Reiniciar mandos", en: "Reset pads", icono: "mandos", hacer: () => accion("mandos.herramienta", { nombre: "reiniciar" }) });
  r.push({ es: "Cambiar tema (Galaxy / Liga B)", en: "Switch theme", icono: "ajustes", hacer: () => accion("ui.tema", { tema: m?.app?.tema === "sudario" ? "galaxy" : "sudario" }) });
  r.push({ es: "Abrir chat", en: "Open chat", icono: "chat", k: "Ctrl+Espacio", hacer: () => cambiar({ chatAbierto: true, noLeidos: 0 }) });
  for (const [es, en, mb] of [["Calidad: ligera (8 Mbps)", "Quality: light (8 Mbps)", 8], ["Calidad: equilibrada (15 Mbps)", "Quality: balanced (15 Mbps)", 15], ["Calidad: máxima (30 Mbps)", "Quality: max (30 Mbps)", 30]]) {
    r.push({ es, en, icono: "rayo", hacer: () => aplicarPresetCalidad(m, mb) });
  }
  // Todas las pantallas, con su sitio en el menú nuevo y una pista de qué hay (también sirve para buscar)
  const PISTA = {
    "sala/resumen": ["abrir sala, enlace, ping", "open room, link, ping", "sala"], "sala/transmision": ["calidad, FPS, pantalla, audio", "quality, FPS, screen, audio", "sala"],
    "sala/opciones": ["nombre, plazas, web de la liga", "name, slots, league website", "sala"], "sync/mipc": ["Sync: ¿está listo mi PC?", "Sync: is my PC ready?", "web"],
    "sala/red": ["semáforo, diagnóstico y ping", "check, diagnostics & ping", "red"], "sala/juegos": ["qué se juega, biblioteca", "what is played, library", "sala"],
    "mandos/puestos": ["mandos, quién juega y quién mira", "pads, who plays and who watches", "mandos"], "partido/vivo": ["partido, equipos y goles", "match, teams and goals", "partido"],
    "gente/sala": ["invitados y su ficha", "guests and their card", "gente"], "mandos/marionetas": ["tu mando físico, cantidad de mandos", "your physical pad, pad count", "mandos"],
    "partido/campo": ["campo 2D en vivo", "live 2D pitch", "partido"], "mandos/mapeo": ["botones cambiados, stick que camina", "swapped buttons, drifting stick", "mandos"],
    "mandos/bloqueo": ["modo competitivo, pausa y marcador automáticos, congelar mandos, bloquear botones", "competitive mode, auto pause and scoreboard, freeze pads, lock buttons", "candado"], "mandos/vivo": ["qué pulsa cada jugador ahora", "what each player is pressing now", "mandos"], "mandos/turnos": ["hotseat", "hotseat", "mandos"],
    "mandos/teclado": ["mapa de teclas", "key map", "mandos"], "sala/puesto": ["mi puesto al pulsar JUGAR", "my seat when pressing PLAY", "mandos"],
    "partido/historial": ["historial y tabla", "history and table", "trofeo"], "sync/traspasos": ["pasar un fichaje", "pass a transfer", "web"],
    "sala/actividad": ["registro de la sala", "room log", "sala"], "jugar/salas": ["salas de otros", "other people's rooms", "sala"],
    "jugar/amigos": ["amigos de la web", "website friends", "web"], "jugar/perfiles": ["nombre y opciones de PES", "PES name and options", "mandos"],
    "ajustes/general": ["tema, chat y bot, avisos", "theme, chat and bot, notices", "ajustes"], "ajustes/overlay": ["overlay en PES", "overlay in PES", "ajustes"],
    "gente/moderacion": ["baneos, moderadores, VIP", "bans, moderators, VIP", "gente"], "sync/avanzado": ["opciones avanzadas de Sync", "advanced Sync options", "web"],
    "ajustes/sonidos": ["!sfx", "!sfx", "ajustes"], "ajustes/interfaz": ["idioma, atajos, tutorial", "language, shortcuts, tutorial", "ajustes"],
    "gente/permisos": ["permisos por rol", "permissions by role", "gente"], "ajustes/avanzado": ["websocket, registros", "websocket, logs", "ajustes"],
  };
  for (const g of GRUPOS) for (const x of g.pestanas) {
    const pista = PISTA[x.p.join("/")] || ["", "", g.icono];
    ir(x.p[0], x.p[1], `${rutaDe(x.p[0], x.p[1])}${pista[0] ? ` (${pista[0]})` : ""}`, `${rutaDe(x.p[0], x.p[1], true)}${pista[1] ? ` (${pista[1]})` : ""}`, pista[2]);
  }
  return r;
}

// Cada ajuste de la interfaz (índice generado de las vistas): solo aparece al escribir
const AJUSTES = INDICE_AJUSTES.map((x) => ({
  es: `${x.es} — ${rutaDe(x.seccion, x.pestana)}`, en: `${x.en} — ${rutaDe(x.seccion, x.pestana, true)}`, icono: "ajustes", k: t("AJUSTE", "SETTING"), ajuste: true,
  hacer: () => { irA(x.seccion, x.pestana); resaltar(t(x.es, x.en), x.tarjeta); },
}));

const normal = (x) => x.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "");

export function Paleta({ s }) {
  const [q, setQ] = useState("");
  const [i, setI] = useState(0);
  const ref = useRef(null);
  useEffect(() => ref.current?.focus(), []);
  const todas = useMemo(() => opciones(s.motor), [s.motor?.sala?.abierta, s.motor?.sala?.lista, s.motor?.app?.tema]);
  const lista = (q ? [...todas, ...AJUSTES] : todas).filter((o) => !q || normal(t(o.es, o.en)).includes(normal(q))).slice(0, 14);
  const ejecutar = (o) => { cambiar({ paleta: false }); o && o.hacer(); };
  const tecla = (e) => {
    if (e.key === "ArrowDown") { e.preventDefault(); setI((x) => Math.min(lista.length - 1, x + 1)); }
    else if (e.key === "ArrowUp") { e.preventDefault(); setI((x) => Math.max(0, x - 1)); }
    else if (e.key === "Enter") { e.preventDefault(); ejecutar(lista[i]); }
  };
  return html`<div class="velo" onClick=${(e) => e.target === e.currentTarget && cambiar({ paleta: false })}>
    <div class="paleta"><div class="cut"><div class="in" style="padding:18px">
      <div style="display:flex;gap:10px;align-items:center"><span class="acc"><${Icono} n="buscar" t=${20}/></span>
        <input ref=${ref} class="campo" style="border:0;background:transparent;box-shadow:none;font-size:17px" placeholder=${t("¿Qué quieres hacer?", "What do you want to do?")}
          value=${q} onInput=${(e) => { setQ(e.currentTarget.value); setI(0); }} onKeyDown=${tecla}/><kbd>Esc</kbd></div>
      <div class="lista">${lista.map((o, n) => html`<div class=${cx("op", n === i && "on")} onMouseEnter=${() => setI(n)} onClick=${() => ejecutar(o)}>
        <span class="acc"><${Icono} n=${o.icono || "rayo"} t=${18}/></span><span>${t(o.es, o.en)}</span>${o.k ? html`<span class="k">${o.k}</span>` : null}</div>`)}
        ${lista.length === 0 ? html`<div class="mut" style="padding:12px">${t("Nada coincide.", "No matches.")}</div>` : null}</div>
    </div></div></div>
  </div>`;
}
