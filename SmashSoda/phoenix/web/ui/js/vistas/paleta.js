// Paleta de acciones (Ctrl+K): ir a cualquier pantalla o ejecutar lo más usado.
import { html, useState, useEffect, useRef, useMemo } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar, irA } from "../tienda.js";
import { Icono, cx } from "../ui.js";
import { INDICE_AJUSTES } from "../indice-ajustes.js";

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
import { abrirSala, cerrarSala } from "./sala.js";

function opciones(m) {
  const sala = m?.sala;
  const r = [];
  const ir = (sec, pes, es, en, icono) => r.push({ es, en, icono, k: t("IR", "GO"), hacer: () => irA(sec, pes) });
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
    r.push({ es, en, icono: "rayo", hacer: () => accion("sala.calidad", { fps: 60, mbps: mb }, { ok: t("Calidad aplicada.", "Quality applied.") }) });
  }
  ir("sala", "resumen", "Sala · Resumen", "Room · Overview", "sala");
  ir("sala", "opciones", "Sala · Opciones", "Room · Options", "sala");
  ir("sala", "juegos", "Sala · Juegos", "Room · Games", "sala");
  ir("sala", "red", "Sala · Conexión (diagnóstico y ping)", "Room · Connection (diagnostics & ping)", "red");
  ir("partido", "vivo", "Partido · Marcador", "Match · Scoreboard", "partido");
  ir("partido", "historial", "Partido · Historial y tabla", "Match · History", "trofeo");
  ir("mandos", "puestos", "Mandos · Puestos", "Pads · Seats", "mandos");
  ir("mandos", "teclado", "Mandos · Teclado (mapa de teclas)", "Pads · Keyboard (key map)", "mandos");
  ir("mandos", "turnos", "Mandos · Turnos", "Pads · Hotseat", "mandos");
  ir("mandos", "marionetas", "Mandos · Marionetas (cantidad de mandos)", "Pads · Puppets (pad count)", "mandos");
  ir("mandos", "bloqueo", "Mandos · Bloqueo", "Pads · Lock", "candado");
  ir("gente", "sala", "Gente · En sala", "People · In room", "gente");
  ir("gente", "moderacion", "Gente · Moderación y baneos", "People · Moderation", "gente");
  ir("gente", "amigos", "Gente · Amigos de la web", "People · Friends", "web");
  ir("gente", "permisos", "Gente · Permisos por rol", "People · Permissions", "gente");
  ir("sync", "puente", "Sync · Puente con el juego", "Sync · Game bridge", "web");
  ir("sync", "web", "Sync · Cuenta web (vincular)", "Sync · Web account (link)", "web");
  ir("sync", "fichajes", "Sync · Fichajes", "Sync · Transfers", "web");
  for (const [p, es, en] of [["general", "General", "General"], ["video", "Video", "Video"], ["audio", "Audio", "Audio"], ["sonidos", "Sonidos (!sfx)", "Sounds (!sfx)"], ["overlay", "Overlay", "Overlay"],
    ["interfaz", "Interfaz e idioma", "Interface"], ["avanzado", "Avanzado", "Advanced"]]) {
    ir("ajustes", p, `Ajustes · ${es}`, `Settings · ${en}`, "ajustes");
  }
  return r;
}

// Cada ajuste de la interfaz (índice generado de las vistas): solo aparece al escribir
const AJUSTES = INDICE_AJUSTES.map((x) => ({
  es: `${x.es} — ${x.donde[0]} › ${x.pestana}`, en: `${x.en} — ${x.donde[1]} › ${x.pestana}`, icono: "ajustes", k: t("AJUSTE", "SETTING"), ajuste: true,
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
