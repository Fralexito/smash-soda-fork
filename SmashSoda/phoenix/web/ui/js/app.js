// =============================================================================
//  Phoenix Link · interfaz nueva. Arranque, barra, menú, avisos y diálogos.
// =============================================================================
import { html, render, useEffect, useRef } from "./lib.js";
import { puente } from "./puente.js";
import { t, fijarIdioma } from "./i18n.js";
import {
  useTienda, conectarTienda, irA, elegirPestana, cambiar, avisar, quitarAviso, cerrarDialogo, accion, leer, elegirPestanaChat, noLeidosGlobal,
} from "./tienda.js";
import { Icono, Logo, cx } from "./ui.js";
import { VistaSala, PESTANAS_SALA } from "./vistas/sala.js";
import { VistaPartido, PESTANAS_PARTIDO } from "./vistas/partido.js";
import { VistaMandos, PESTANAS_MANDOS, metaMandos } from "./vistas/mandos.js";
import { VistaGente, PESTANAS_GENTE, metaGente } from "./vistas/gente.js";
import { VistaSync, PESTANAS_SYNC, metaSync } from "./vistas/sync.js";
import { VistaAjustes, PESTANAS_AJUSTES } from "./vistas/ajustes.js";
import { PanelChat } from "./vistas/chat.js";
import { UltimaHora } from "./vistas/ultimahora.js";
import { Paleta } from "./vistas/paleta.js";
import { vigilarCambios } from "./vigia.js";

const SECCIONES = [
  { id: "sala", es: "SALA", en: "ROOM", vista: VistaSala, pestanas: PESTANAS_SALA },
  { id: "partido", es: "PARTIDO", en: "MATCH", vista: VistaPartido, pestanas: PESTANAS_PARTIDO },
  { id: "mandos", es: "MANDOS", en: "PADS", vista: VistaMandos, pestanas: PESTANAS_MANDOS, meta: metaMandos },
  { id: "gente", es: "GENTE", en: "PEOPLE", vista: VistaGente, pestanas: PESTANAS_GENTE, meta: metaGente },
  { id: "sync", es: "SYNC", en: "SYNC", vista: VistaSync, pestanas: PESTANAS_SYNC, meta: metaSync },
  { id: "ajustes", es: "AJUSTES", en: "SETTINGS", vista: VistaAjustes, pestanas: PESTANAS_AJUSTES },
];

// ---- Barra superior --------------------------------------------------------------
function PildoraSala({ m }) {
  const sala = m && m.sala;
  if (!sala) return html`<span class="pill"><span class="punto"></span>${t("SIN MOTOR", "NO ENGINE")}</span>`;
  if (sala.abierta) {
    return html`<button class="pill" onClick=${() => irA("sala", "resumen")}><span class="punto ok vivo"></span>
      ${t("EN VIVO", "LIVE")} · ${sala.invitados} ${t("EN SALA", "IN ROOM")}</button>`;
  }
  if (!sala.lista) return html`<span class="pill"><span class="punto warn"></span>${t("PREPARANDO", "STARTING")}</span>`;
  return html`<button class="pill" onClick=${() => irA("sala", "resumen")}><span class="punto"></span>${t("SALA CERRADA", "ROOM CLOSED")}</button>`;
}

const ESTADO_WEB = {
  conectado: ["ok", "WEB SYNC", "WEB SYNC"],
  vinculando: ["warn", "VINCULANDO", "LINKING"],
  sin_conexion: ["bad", "WEB SIN CONEXIÓN", "WEB OFFLINE"],
  pausado: ["warn", "WEB EN PAUSA", "WEB PAUSED"],
  sin_vincular: ["", "SIN VINCULAR", "NOT LINKED"],
};

function Barra({ s }) {
  const m = s.motor;
  const tema = m?.app?.tema === "sudario" ? "sudario" : "galaxy";
  const w = ESTADO_WEB[m?.web?.estado] || ESTADO_WEB.sin_vincular;
  const cambiarTema = () => accion("ui.tema", { tema: tema === "galaxy" ? "sudario" : "galaxy" });
  return html`<header class="barra">
    <div class="marca">
      <${Logo}/>
      <div style="min-width:0">
        <div class="nombre">PHOENIX LINK</div>
        <div class="liga">${tema === "sudario" ? "Liga B" : "Galaxy League"}${puente.simulado ? t(" · VISTA PREVIA", " · PREVIEW") : ""}</div>
      </div>
    </div>
    <div class="ultima-hueco"><${UltimaHora} s=${s}/></div>
    <div class="lado">
      <${PildoraSala} m=${m}/>
      <button class="pill web" onClick=${() => irA("ajustes", "web")} title=${m?.web?.mensaje || ""}>
        <span class=${cx("punto", w[0])}></span>${t(w[1], w[2])}</button>
      <button class=${cx("pill", s.chatAbierto && "acc")} onClick=${() => alternarChat()} title="Chat (Ctrl+Espacio)">
        <${Icono} n="chat" t=${15}/>CHAT${totalNoLeidos(s) > 0 ? html`<span class="chip bad" style="height:20px">${totalNoLeidos(s) > 99 ? "99+" : totalNoLeidos(s)}</span>` : null}
      </button>
      <button class="pill atajo" onClick=${() => cambiar({ paleta: true })} title=${t("Buscar acciones", "Search actions")}>
        <${Icono} n="buscar" t=${15}/><kbd>Ctrl K</kbd></button>
      <button class="pill acc" onClick=${cambiarTema} title=${t("Cambiar tema", "Switch theme")}>${tema.toUpperCase()}</button>
    </div>
  </header>`;
}

function totalNoLeidos(s) { return s.noLeidos + noLeidosGlobal(s); }

export function alternarChat(abrir) {
  const s = leer();
  const nuevo = abrir ?? !s.chatAbierto;
  // Al abrir, queda leído lo de la pestaña que se ve
  cambiar({ chatAbierto: nuevo });
  if (nuevo) elegirPestanaChat(s.chatPestana);
}

// ---- Menú lateral ----------------------------------------------------------------
function Menu({ s }) {
  const m = s.motor;
  const pendientesMandos = (m?.solicitudes?.length || 0) + (m?.espera?.length || 0);
  const enJuego = m?.partido?.fase === "en_juego" || m?.partido?.fase === "pausado";
  const chapas = { mandos: pendientesMandos, partido: enJuego ? "●" : 0 };
  return html`<nav class="menu" aria-label=${t("Secciones", "Sections")}>
    ${SECCIONES.map((x) => html`
      <button class=${cx("nv", s.seccion === x.id && "on")} onClick=${() => irA(x.id)} aria-current=${s.seccion === x.id ? "page" : null}>
        <${Icono} n=${x.id} t=${26}/><span>${t(x.es, x.en)}</span>
        ${chapas[x.id] ? html`<span class="chapa">${chapas[x.id]}</span>` : null}
      </button>`)}
    <div class="pie">PHOENIX<br/>LINK ${m?.app?.version || ""}</div>
  </nav>`;
}

// ---- Contenido -------------------------------------------------------------------
function Contenido({ s }) {
  const sec = SECCIONES.find((x) => x.id === s.seccion) || SECCIONES[0];
  const pestana = s.pestanas[sec.id] || sec.pestanas[0].id;
  const Vista = sec.vista;
  const meta = sec.meta ? sec.meta(s) : null;
  const info = sec.pestanas.find((p) => p.id === pestana);
  return html`<main class="contenido">
    <div class="cabecera">
      <div class="pestanas" role="tablist">
        ${sec.pestanas.map((p) => html`
          <button role="tab" aria-selected=${p.id === pestana} class=${cx("tab", p.id === pestana && "on")} onClick=${() => elegirPestana(p.id)}>
            ${t(p.es, p.en)}${p.cuenta ? html`<span class="n">${p.cuenta(s) || ""}</span>` : null}
          </button>`)}
      </div>
      ${meta}
    </div>
    ${info?.d ? html`<div class="guia" key=${"g" + pestana}><span class="acc"><${Icono} n="rayo" t=${14}/></span>${t(info.d[0], info.d[1])}</div>` : null}
    <div class="vista" key=${sec.id + "/" + pestana}>
      <${Vista} s=${s} pestana=${pestana}/>
    </div>
  </main>`;
}

// ---- Avisos y diálogo -------------------------------------------------------------
function Avisos({ avisos }) {
  return html`<div class="avisos" aria-live="polite">${avisos.map((a) => html`
    <div class=${cx("aviso", a.tipo)} key=${a.id}>
      <span style=${`color:var(--${a.tipo === "ok" ? "ok" : a.tipo === "bad" ? "bad" : a.tipo === "warn" ? "warn" : "acc"})`}>
        <${Icono} n=${a.tipo === "ok" ? "check" : a.tipo === "info" ? "rayo" : "alerta"} t=${18}/></span>
      <div>${a.texto}</div>
      <button class="x" onClick=${() => quitarAviso(a.id)} aria-label=${t("Cerrar", "Close")}>×</button>
    </div>`)}</div>`;
}

function Dialogo({ d }) {
  const ref = useRef(null);
  useEffect(() => { const b = ref.current?.querySelector(".botones .btn:last-child"); b && b.focus(); }, [d]);
  if (!d) return null;
  return html`<div class="velo" onClick=${(e) => e.target === e.currentTarget && cerrarDialogo(false)}>
    <div class="dialogo" ref=${ref} role="dialog" aria-modal="true">
      <div class="cut"><div class="in" style="padding:26px 28px">
        <h2>${d.titulo}</h2>
        <div class="ayuda" style="font-size:14px">${d.texto}</div>
        <div class="botones">${d.botones.map((b) => html`
          <button class=${cx("btn", b.tipo)} onClick=${() => cerrarDialogo(b.valor)}>${b.texto}</button>`)}</div>
      </div></div>
    </div>
  </div>`;
}

// ---- Raíz ------------------------------------------------------------------------
function App() {
  const s = useTienda();
  const m = s.motor;
  const tema = m?.app?.tema === "sudario" ? "sudario" : "galaxy";
  useEffect(() => { document.documentElement.dataset.tema = tema; }, [tema]);
  fijarIdioma(m?.app?.idioma || "es");

  useEffect(() => {
    const tecla = (e) => {
      if (e.ctrlKey && (e.key === "k" || e.key === "K")) { e.preventDefault(); cambiar({ paleta: !leer().paleta }); }
      else if (e.ctrlKey && e.code === "Space") { e.preventDefault(); alternarChat(); }
      else if (e.key === "Escape") {
        const st = leer();
        if (st.dialogo) cerrarDialogo(false);
        else if (st.paleta) cambiar({ paleta: false });
        else if (st.chatAbierto) alternarChat(false);
      } else if (e.altKey && /^[1-5]$/.test(e.key)) {
        e.preventDefault();
        irA(SECCIONES[Number(e.key) - 1].id);
      }
    };
    window.addEventListener("keydown", tecla);
    return () => window.removeEventListener("keydown", tecla);
  }, []);

  return html`
    <div class="fondo"></div>
    <div class=${cx("app", s.chatAbierto && "chat-abierto")}>
      <${Barra} s=${s}/>
      <${Menu} s=${s}/>
      <${Contenido} s=${s}/>
      <${PanelChat} s=${s}/>
    </div>
    <${Avisos} avisos=${s.avisos}/>
    <${Dialogo} d=${s.dialogo}/>
    ${s.paleta ? html`<${Paleta} s=${s}/>` : null}
    <div class=${cx("carga", s.conectado && "fuera")} aria-hidden=${s.conectado}>
      <${Logo} t=${64}/><div class="nombre">PHOENIX LINK</div><div class="giro"></div>
    </div>`;
}

// ---- Arranque --------------------------------------------------------------------
window.addEventListener("error", (e) => console.error("[ui]", e.message, e.filename, e.lineno));
window.addEventListener("unhandledrejection", (e) => console.error("[ui] promesa", e.reason));
// Sin menú contextual del navegador (la app no es una página web)
window.addEventListener("contextmenu", (e) => { if (!e.target.closest("input, textarea, .chat .lineas")) e.preventDefault(); });

conectarTienda();
vigilarCambios();
render(html`<${App}/>`, document.getElementById("raiz"));
puente.iniciar().catch((e) => {
  console.error(e);
  avisar(t("No se pudo conectar con el motor.", "Could not reach the engine."), "bad", 0);
});
