// =============================================================================
//  Phoenix Link · interfaz nueva. Arranque, barra, menú, avisos y diálogos.
// =============================================================================
import { html, render, useEffect, useRef, useState } from "./lib.js";
import { puente } from "./puente.js";
import { t, fijarIdioma } from "./i18n.js";
import {
  useTienda, conectarTienda, irA, cambiar, avisar, quitarAviso, cerrarDialogo, accion, leer, elegirPestanaChat, noLeidosGlobal,
} from "./tienda.js";
import { Icono, Logo, cx, useDestello } from "./ui.js";
import { VistaSala, PESTANAS_SALA } from "./vistas/sala.js";
import { VistaPartido, PESTANAS_PARTIDO } from "./vistas/partido.js";
import { VistaMandos, PESTANAS_MANDOS, metaMandos } from "./vistas/mandos.js";
import { VistaGente, PESTANAS_GENTE, metaGente } from "./vistas/gente.js";
import { VistaJugar, PESTANAS_JUGAR, metaJugar } from "./vistas/seccionjugar.js";
import { VistaSync, PESTANAS_SYNC, metaSync } from "./vistas/sync.js";
import { VistaAjustes, PESTANAS_AJUSTES } from "./vistas/ajustes.js";
import { PanelChat } from "./vistas/chat.js";
import { UltimaHora } from "./vistas/ultimahora.js";
import { Paleta } from "./vistas/paleta.js";
import { Tutorial } from "./vistas/tutorial.js";
import { vigilarCambios } from "./vigia.js";
import { GRUPOS, grupoDe } from "./grupos.js";
import { BandaCedido, DialogoCeder, SECCIONES_CEDIDO, PESTANAS_CEDIDO, estaCedida, vigilarCedido } from "./vistas/cedido.js";
import { BandaActualizacion } from "./vistas/actualizacion.js";
import { causaPausa } from "./vistas/semaforo.js";
import { Bienvenida } from "./vistas/bienvenida.js";
import { VistaInicio, PESTANAS_INICIO } from "./vistas/inicio.js";
import { vigilarSentado } from "./vistas/sentado.js";
import { QueHago } from "./vistas/quehago.js";

const SECCIONES = [
  { id: "inicio", es: "INICIO", en: "HOME", vista: VistaInicio, pestanas: PESTANAS_INICIO },   // plan UX: delante de todo, sin quitar nada
  { id: "sala", es: "SALA", en: "ROOM", vista: VistaSala, pestanas: PESTANAS_SALA },
  { id: "partido", es: "PARTIDO", en: "MATCH", vista: VistaPartido, pestanas: PESTANAS_PARTIDO },
  { id: "mandos", es: "MANDOS", en: "PADS", vista: VistaMandos, pestanas: PESTANAS_MANDOS, meta: metaMandos },
  { id: "gente", es: "GENTE", en: "PEOPLE", vista: VistaGente, pestanas: PESTANAS_GENTE, meta: metaGente },
  { id: "jugar", es: "JUGAR", en: "PLAY", vista: VistaJugar, pestanas: PESTANAS_JUGAR, meta: metaJugar },
  { id: "sync", es: "SYNC", en: "SYNC", vista: VistaSync, pestanas: PESTANAS_SYNC, meta: metaSync },
  { id: "ajustes", es: "AJUSTES", en: "SETTINGS", vista: VistaAjustes, pestanas: PESTANAS_AJUSTES },
];

// PC cedida (vistas/cedido.js): solo Sala, Partido y Mandos, y en ellas solo las pestañas de jugar.
const seccionesVisibles = (s) => (estaCedida(s) ? SECCIONES.filter((x) => SECCIONES_CEDIDO.includes(x.id)) : SECCIONES);
const pestanasVisibles = (s, sec) => (estaCedida(s) ? sec.pestanas.filter((p) => PESTANAS_CEDIDO[sec.id]?.includes(p.id)) : sec.pestanas);

// ---- Menú por momento de uso (grupos.js) -------------------------------------------
// Cada pestaña del menú es una pantalla de siempre: [sección, pestaña].
const infoPantalla = (sec, pes) => SECCIONES.find((x) => x.id === sec)?.pestanas.find((p) => p.id === pes);
const pantallaVisible = (s, [sec, pes]) => !!infoPantalla(sec, pes) && (!estaCedida(s) || (SECCIONES_CEDIDO.includes(sec) && PESTANAS_CEDIDO[sec]?.includes(pes)));
const pestanasGrupo = (s, g) => g.pestanas.filter((x) => pantallaVisible(s, x.p));
const gruposVisibles = (s) => GRUPOS.filter((g) => pestanasGrupo(s, g).length);
// Última pantalla vista de cada grupo: al volver al grupo se abre esa
const ultimaDeGrupo = {};
function irAGrupo(s, g) {
  const lista = pestanasGrupo(s, g);
  const u = ultimaDeGrupo[g.id];
  const x = (u && lista.find((y) => y.p[0] === u[0] && y.p[1] === u[1])) || lista[0];
  if (x) irA(x.p[0], x.p[1]);
}
// Pantalla que se ve ahora (la pedida, o la primera visible de su sección)
function pantallaActual(s) {
  const visibles = seccionesVisibles(s);
  const sec = visibles.find((x) => x.id === s.seccion) || visibles[0];
  const pestanas = pestanasVisibles(s, sec);
  const pedida = s.pestanas[sec.id];
  return { sec, pestana: pestanas.some((p) => p.id === pedida) ? pedida : pestanas[0].id };
}

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
  conectado: ["ok", "WEB CONECTADA", "WEB CONNECTED"],
  vinculando: ["warn", "CONECTANDO", "CONNECTING"],
  sin_conexion: ["bad", "WEB SIN CONEXIÓN", "WEB OFFLINE"],
  pausado: ["warn", "WEB EN PAUSA", "WEB PAUSED"],
  sin_vincular: ["", "CONECTAR MI CUENTA", "CONNECT MY ACCOUNT"],
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
      <${GrupoEstado} m=${m} w=${w} enSync=${s.seccion === "sync" || estaCedida(s)}/>
      <button class=${cx("pill", s.chatAbierto && "acc")} onClick=${() => alternarChat()} title="Chat (Ctrl+Espacio)">
        <${Icono} n="chat" t=${15}/>CHAT${totalNoLeidos(s) > 0 ? html`<span class="chip bad" style="height:20px">${totalNoLeidos(s) > 99 ? "99+" : totalNoLeidos(s)}</span>` : null}
      </button>
      ${estaCedida(s) ? null : html`<button class="pill atajo" onClick=${() => cambiar({ paleta: true })} title=${t("Buscar acciones", "Search actions")}>
        <${Icono} n="buscar" t=${15}/><kbd>Ctrl K</kbd></button>
      <button class="pill acc" onClick=${cambiarTema} title=${t("Cambiar tema", "Switch theme")}>${tema === "sudario" ? "LIGA B" : "GALAXY"}</button>`}
    </div>
  </header>`;
}

// Grupo «conectado»: sala, web de la liga y juego (puente de Phoenix Sync), con su punto vivo.
const ESTADO_JUEGO = {
  conectado: ["ok", "JUEGO CONECTADO", "GAME CONNECTED"], cerrado: ["", "JUEGO CERRADO", "GAME CLOSED"],
  noinstalado: ["warn", "SIN MÓDULO SIDER", "NO SIDER MODULE"], sinconexion: ["bad", "JUEGO SIN WEB", "GAME OFFLINE"], apagado: ["", "AVISOS APAGADOS", "NOTICES OFF"],
};
// En Sync, la web y el juego ya salen en sus propias pastillas: aquí solo queda la sala (sin repetir).
function GrupoEstado({ m, w, enSync = false }) {
  const b = m?.buzon;
  const j = ESTADO_JUEGO[b?.estado] || ESTADO_JUEGO.cerrado;
  // Cada pastilla destella una vez cuando cambia su estado (sala abierta, web conectada, juego conectado…)
  const dSala = useDestello(m?.sala?.abierta ? "abierta" : m?.sala?.lista ? "cerrada" : "preparando");
  const dWeb = useDestello(w[1]);
  const dJuego = useDestello(j[1]);
  const ultimo = b?.estado === "conectado" && b?.ultimo ? " · " + b.ultimo : "";
  return html`<div class="grupo-estado" role="status" aria-label=${t("Estado de la conexión", "Connection status")}>
    <span class=${dSala ? "destello" : ""} style="display:inline-flex;border-radius:999px"><${PildoraSala} m=${m}/></span>
    ${enSync ? null : html`<button class=${cx("pill web", dWeb && "destello")} onClick=${() => irA("sync", "mipc")} title=${m?.web?.estado === "pausado" ? causaPausa(m.web) : m?.web?.mensaje || t("Cuenta de la liga", "League account")}>
      <span class=${cx("punto", w[0], w[0] === "ok" && "vivo")}></span>${t(w[1], w[2])}</button>
    <button class=${cx("pill juego", dJuego && "destello")} onClick=${() => irA("sync", "avanzado")} title=${t("Conexión de Phoenix con el juego", "Phoenix game connection")}>
      <span class=${cx("punto", j[0], j[0] === "ok" && "vivo")}></span>${t(j[1], j[2])}${ultimo}</button>`}
  </div>`;
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
  const { sec, pestana } = pantallaActual(s);
  const activo = grupoDe(sec.id, pestana).id;
  return html`<nav class="menu" aria-label=${t("Secciones", "Sections")}>
    ${gruposVisibles(s).map((g) => html`
      <button class=${cx("nv", activo === g.id && "on")} title=${t(g.d[0], g.d[1])} onClick=${() => irAGrupo(s, g)} aria-current=${activo === g.id ? "page" : null}>
        <${Icono} n=${g.icono} t=${26}/><span>${t(g.es, g.en)}</span>
        ${chapas[g.id] ? html`<span class="chapa">${chapas[g.id]}</span>` : null}
      </button>`)}
    <div class="pie" title=${m?.app?.compilado ? t("Compilado el ", "Built on ") + m.app.compilado : ""}>PHOENIX<br/>LINK ${m?.app?.version || ""}${m?.app?.etapa ? html`<br/><span style="opacity:.8;font-size:10.5px;letter-spacing:.08em">${String(m.app.etapa).toUpperCase()}</span>` : null}${m?.app?.compilado ? html`<br/><span style="opacity:.65;font-size:10.5px">${m.app.compilado}</span>` : null}</div>
  </nav>`;
}

// ---- Contenido -------------------------------------------------------------------
function Contenido({ s }) {
  const { sec, pestana } = pantallaActual(s);
  const Vista = sec.vista;
  const meta = sec.meta ? sec.meta(s) : null;
  const info = sec.pestanas.find((p) => p.id === pestana);
  // Pestañas del grupo (pueden ser de varias secciones). Las poco usadas (mas: true)
  // quedan detrás de «MÁS»; la que se ve, siempre a la vista.
  const g = grupoDe(sec.id, pestana);
  useEffect(() => { ultimaDeGrupo[g.id] = [sec.id, pestana]; }, [g.id, sec.id, pestana]);
  const pestanas = pestanasGrupo(s, g);
  const esActual = (x) => x.p[0] === sec.id && x.p[1] === pestana;
  const [verMas, setVerMas] = useState({});
  const abierto = !!verMas[g.id];
  const ocultas = pestanas.filter((x) => x.mas && !esActual(x));
  const mostradas = abierto ? pestanas : pestanas.filter((x) => !x.mas || esActual(x));
  const nombre = (x) => { const p = infoPantalla(x.p[0], x.p[1]); return t(x.es || p.es, x.en || p.en); };
  return html`<main class="contenido">
    <${BandaCedido} s=${s}/>
    ${estaCedida(s) ? null : html`<${BandaActualizacion} s=${s}/>`}
    <div class="cabecera">
      <div class="pestanas" role="tablist">
        ${mostradas.map((x) => { const p = infoPantalla(x.p[0], x.p[1]); return html`
          <button role="tab" aria-selected=${esActual(x)} class=${cx("tab", esActual(x) && "on")} onClick=${() => irA(x.p[0], x.p[1])}>
            ${nombre(x)}${p.cuenta ? html`<span class="n">${p.cuenta(s) || ""}</span>` : null}
          </button>`; })}
        ${ocultas.length ? html`<button class="tab mas" aria-expanded=${abierto} onClick=${() => setVerMas({ ...verMas, [g.id]: !abierto })}
          title=${abierto ? t("Ocultar las opciones menos usadas", "Hide the less used options") : ocultas.map(nombre).join(" · ")}>
          ${abierto ? t("MENOS ▴", "LESS ▴") : t(`MÁS (${ocultas.length}) ▾`, `MORE (${ocultas.length}) ▾`)}</button>` : null}
      </div>
      ${meta}
    </div>
    <${QueHago} s=${s} sec=${sec.id} pestana=${pestana} d=${info?.d}/>
    <div class="vista" key=${sec.id + "/" + pestana}>
      <${Vista} s=${s} pestana=${pestana}/>
    </div>
  </main>`;
}

// ---- Avisos y diálogo -------------------------------------------------------------
function Avisos({ avisos }) {
  return html`<div class="avisos" aria-live="polite">${avisos.map((a) => html`
    <div class=${cx("aviso", a.tipo, a.saliendo && "saliendo")} key=${a.id}>
      <span style=${`color:var(--${a.tipo === "ok" ? "ok" : a.tipo === "bad" ? "bad" : a.tipo === "warn" ? "warn" : "acc"})`}>
        <${Icono} n=${a.tipo === "ok" ? "check" : a.tipo === "info" ? "rayo" : "alerta"} t=${18}/></span>
      <div>${a.texto}</div>
      ${a.boton ? html`<button class="btn mini" onClick=${() => { quitarAviso(a.id); a.boton.al(); }}>${a.boton.texto}</button>` : null}
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
  const cedida = !!m?.cedido?.activo;
  useEffect(() => { vigilarSentado(m); }, [m]);   // sentado automático (plan UX)
  useEffect(() => {
    vigilarCedido(m);
    if (cedida) cambiar({ paleta: false, ceder: null });
  }, [cedida, m?.cedido?.ultimoFin]);

  useEffect(() => {
    const tecla = (e) => {
      if (e.ctrlKey && (e.key === "k" || e.key === "K")) { e.preventDefault(); if (!estaCedida(leer())) cambiar({ paleta: !leer().paleta }); }
      else if (e.ctrlKey && e.code === "Space") { e.preventDefault(); alternarChat(); }
      else if (e.key === "Escape") {
        const st = leer();
        if (st.ceder) cambiar({ ceder: null });
        else if (st.dialogo) cerrarDialogo(false);
        else if (st.paleta) cambiar({ paleta: false });
        else if (st.chatAbierto) alternarChat(false);
      } else if (e.altKey && !e.ctrlKey && /^[1-7]$/.test(e.key)) {   // Ctrl+Alt+número es «tomar mando» (MandoHost)
        e.preventDefault();
        const st = leer();
        const g = gruposVisibles(st)[Number(e.key) - 1];
        if (g) irAGrupo(st, g);
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
    <${DialogoCeder} s=${s}/>
    ${s.paleta && !cedida ? html`<${Paleta} s=${s}/>` : null}
    <${Bienvenida} s=${s}/>
    <${Tutorial} s=${s}/>
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

// Con la ventana oculta se congelan las animaciones (ver .oculta en app.css)
try {
  const marcar = () => document.documentElement.classList.toggle("oculta", document.hidden);
  document.addEventListener("visibilitychange", marcar);
  marcar();
  // Sin foco (otra ventana delante, p. ej. PES): se paran los adornos infinitos (ver .sin-foco en app.css)
  const foco = () => document.documentElement.classList.toggle("sin-foco", !document.hasFocus());
  window.addEventListener("focus", foco);
  window.addEventListener("blur", foco);
  foco();
} catch (e) { /* nada */ }
