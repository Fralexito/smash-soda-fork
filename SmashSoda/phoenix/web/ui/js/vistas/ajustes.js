// =============================================================================
//  AJUSTES: General (diseño v4), Video, Audio, Overlay, Permisos, Cuenta web,
//  Interfaz, Diagnóstico y Avanzado. Cada control usa el camino del original.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar, actualizarInfo, avisar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Interruptor, Ajuste, AjusteSw, Stepper, Selector, Segmentos, Campo, Icono, Vacio, Chip, cx } from "../ui.js";
import { TarjetaCalidad } from "./sala.js";

export const PESTANAS_AJUSTES = [
  { id: "general", es: "GENERAL", en: "GENERAL" },
  { id: "video", es: "VIDEO", en: "VIDEO" },
  { id: "audio", es: "AUDIO", en: "AUDIO" },
  { id: "overlay", es: "OVERLAY", en: "OVERLAY" },
  { id: "permisos", es: "PERMISOS", en: "PERMISSIONS" },
  { id: "web", es: "CUENTA WEB", en: "WEB ACCOUNT" },
  { id: "interfaz", es: "INTERFAZ", en: "INTERFACE" },
  { id: "diagnostico", es: "DIAGNÓSTICO", en: "DIAGNOSTICS" },
  { id: "avanzado", es: "AVANZADO", en: "ADVANCED" },
];

export function VistaAjustes({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.ajustes) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  switch (pestana) {
    case "video": return html`<${Video} m=${m} info=${s.info}/>`;
    case "audio": return html`<${Audio} m=${m}/>`;
    case "overlay": return html`<${Overlay} m=${m} info=${s.info}/>`;
    case "permisos": return html`<${Permisos} m=${m}/>`;
    case "web": return html`<${CuentaWeb} m=${m}/>`;
    case "interfaz": return html`<${Interfaz} m=${m} info=${s.info}/>`;
    case "diagnostico": return html`<${Diagnostico}/>`;
    case "avanzado": return html`<${Avanzado} m=${m}/>`;
    default: return html`<${General} m=${m}/>`;
  }
}

const gen = (clave, valor) => accion("ajustes.general", { clave, valor });
const clasico = (pestana) => accion("ui.panelClasico", { seccion: 3, pestana });

// ---- General -------------------------------------------------------------------------------
function TemaCarta({ id, actual, colores, nombre, sub, fondo }) {
  const on = actual === id;
  return html`<button class="caja" style=${`text-align:left;cursor:pointer;padding:16px;border-color:${on ? "var(--acc)" : "rgba(255,255,255,.14)"};background:${fondo}`}
    onClick=${() => !on && accion("ui.tema", { tema: id })} aria-pressed=${on}>
    <div style="display:flex;gap:6px">${colores.map((c) => html`<span style=${`width:26px;height:8px;border-radius:4px;background:${c}`}></span>`)}</div>
    <div class="disp" style="margin-top:14px;font-size:18px;font-weight:700;letter-spacing:.14em;color:#fff">${nombre}</div>
    <div class="mono mut" style="margin-top:6px;font-size:12px">${sub}</div>
  </button>`;
}

function General({ m }) {
  const g = m.ajustes.general;
  const tema = m.app?.tema || "galaxy";
  return html`<div class="fila partible" style="min-height:100%">
    <div class="col" style="flex:1">
      <${Tarjeta} interior="padding:22px 24px">
        <div class="lab acc">${t("TEMA", "THEME")}</div>
        <div class="ayuda" style="margin-top:6px">${t("Cambia el esquema de colores de toda la app.", "Changes the whole app's colors.")}</div>
        <div class="rej2" style="margin-top:16px;gap:14px">
          <${TemaCarta} id="galaxy" actual=${tema} colores=${["#00e5ff", "#8000ff"]} nombre="GALAXY" sub=${t("CIAN · PÚRPURA", "CYAN · PURPLE")} fondo="linear-gradient(135deg,#0b1030,#1a0a3a)"/>
          <${TemaCarta} id="sudario" actual=${tema} colores=${["#f5c451", "#3b82f6"]} nombre="SUDARIO" sub=${t("DORADO · AZUL", "GOLD · BLUE")} fondo="linear-gradient(135deg,#0c1634,#10244f)"/>
        </div>
      </${Tarjeta}>
      <${Tarjeta} estilo="flex:1" interior="padding:22px 24px;display:flex;flex-direction:column;gap:4px">
        <div class="lab acc">${t("CHAT Y BOT", "CHAT AND BOT")}</div>
        <${Ajuste} bloque titulo=${t("Nombre del bot", "Bot name")} desc=${t("Ponle un nombre divertido si quieres.", "Give it a fun name.")}>
          <${Campo} valor=${g.chatbot} max=${64} al=${(v) => gen("chatbot", v)}/></${Ajuste}>
        <${Ajuste} bloque titulo=${t("Mensaje de bienvenida", "Welcome message")} desc=${t("Los invitados lo ven al entrar. Escribe _PLAYER_ para insertar su nombre.", "Guests see it on join. Use _PLAYER_ for their name.")}>
          <${Campo} multilinea valor=${g.welcomeMessage} max=${500} al=${(v) => gen("welcomeMessage", v)}/></${Ajuste}>
        <${Ajuste} bloque titulo=${t("Enlace de Discord", "Discord link")} desc=${t("Lo responde el comando !discord.", "Answered by !discord.")}>
          <${Campo} valor=${g.discord} max=${255} mono al=${(v) => gen("discord", v)}/></${Ajuste}>
      </${Tarjeta}>
    </div>
    <div class="col" style="flex:1">
      <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column">
        <div class="lab acc">${t("ENTRADA Y SEGURIDAD", "INPUT AND SECURITY")}</div>
        <${AjusteSw} titulo=${t("Desactivar botón Guía", "Disable Guide button")} desc=${t("Suele abrir overlays y puede dar problemas al hostear.", "It opens overlays and can cause trouble.")} valor=${g.disableGuideButton} al=${(v) => gen("disableGuideButton", v)}/>
        <${AjusteSw} titulo=${t("Desactivar teclado", "Disable keyboard")} desc=${t("Impide que invitados sin mando jueguen con teclado.", "Guests without a pad can't use keyboard.")} valor=${g.disableKeyboard} al=${(v) => gen("disableKeyboard", v)}/>
        <${AjusteSw} titulo=${t("Indexar mandos automáticamente", "Auto-index pads")} desc=${t("Puede causar pantallazos azules en algunos equipos.", "May cause blue screens on some PCs.")} valor=${g.autoIndex} al=${(v) => gen("autoIndex", v)}/>
        <${AjusteSw} titulo=${t("Bloquear también la IP", "Also ban the IP")} desc=${t("Al banear a alguien se bloquea su dirección IP.", "Bans also block the IP address.")} valor=${g.ipBan} al=${(v) => gen("ipBan", v)}/>
        <${AjusteSw} titulo=${t("Bloquear VPN", "Block VPN")} desc=${t("Actívalo solo si tienes problemas con trolls.", "Only if you have trolls.")} valor=${g.blockVPN} al=${(v) => gen("blockVPN", v)}/>
      </${Tarjeta}>
      <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column">
        <div class="lab acc">${t("AVISOS", "NOTIFICATIONS")}</div>
        <${AjusteSw} titulo=${t("Parpadear la ventana", "Flash the window")} desc=${t("La barra de tareas avisa cuando alguien escribe.", "Taskbar flashes on new messages.")} valor=${g.flashWindow} al=${(v) => gen("flashWindow", v)}/>
        <${AjusteSw} titulo=${t("Sonido de mensaje", "Message sound")} valor=${g.messageNotification} al=${(v) => gen("messageNotification", v)}/>
        <${AjusteSw} titulo=${t("Leer el chat en voz alta", "Text to speech")} valor=${g.ttsEnabled} al=${(v) => gen("ttsEnabled", v)}/>
        <${AjusteSw} titulo=${t("Permitir !bonk", "Allow !bonk")} desc=${t("El juego de golpes del chat.", "The chat bonk game.")} valor=${g.bonkEnabled} al=${(v) => gen("bonkEnabled", v)}/>
      </${Tarjeta}>
      <${Tarjeta} interior="padding:20px 24px;display:flex;justify-content:space-between;align-items:center;gap:16px">
        <div><div style="font-weight:600;font-size:16px">${t("Todas las opciones originales", "All original options")}</div>
          <div class="ayuda">${t("País, WebSocket, permisos por rol y SFX.", "Country, WebSocket, role permissions and SFX.")}</div></div>
        <${Boton} al=${() => clasico(1)}>${t("ABRIR PANEL CLÁSICO", "OPEN CLASSIC PANEL")}</${Boton}>
      </${Tarjeta}>
    </div>
  </div>`;
}

// ---- Video ----------------------------------------------------------------------------------
function Video({ m, info }) {
  const v = m.ajustes.video;
  const vid = (clave, valor) => accion("ajustes.video", { clave, valor });
  const refrescar = async () => {
    const r = await accion("ajustes.videoListas", {}, { ok: t("Listas actualizadas.", "Lists refreshed.") });
    if (r) actualizarInfo({ pantallas: r.pantallas || [], gpus: r.gpus || [], wgc: !!r.wgc });
  };
  const op = (lista) => (lista.length ? lista : [t("(sin datos)", "(no data)")]).map((x, i) => ({ valor: i, texto: x }));
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("CAPTURA", "CAPTURE")} derecha=${html`<${Boton} tipo="mini suave" al=${refrescar}>${t("ACTUALIZAR", "REFRESH")}</${Boton}>`}/>
      <${Ajuste} bloque titulo=${t("Tarjeta de video", "Graphics card")}>
        <${Selector} valor=${v.gpu} opciones=${op(info.gpus)} al=${(x) => vid("gpu", Number(x))}/></${Ajuste}>
      <${Ajuste} bloque titulo=${t("Pantalla que se transmite", "Screen to stream")}>
        <${Selector} valor=${v.monitor} opciones=${op(info.pantallas)} al=${(x) => vid("monitor", Number(x))}/></${Ajuste}>
      <${Ajuste} bloque titulo=${t("Método de captura", "Capture method")} desc=${t("Si la imagen se congela o sale negra, prueba otro.", "If the image freezes or is black, try another.")}>
        <${Selector} valor=${v.captura} al=${(x) => vid("captura", Number(x))} opciones=${[
          { valor: 0, texto: t("Automático", "Automatic") }, { valor: 1, texto: "Desktop Duplication" },
          ...(info.wgc ? [{ valor: 2, texto: "Windows Graphics Capture" }] : [])]}/></${Ajuste}>
      <${Ajuste} bloque titulo=${t("Resolución de salida", "Output resolution")}>
        <${Selector} valor=${v.resolucion} opciones=${op(info.resoluciones)} al=${(x) => vid("resolucion", Number(x))}/></${Ajuste}>
      <${AjusteSw} titulo=${t("Escalado Lanczos", "Lanczos scaling")} desc=${t("Imagen más nítida al reducir resolución (usa más GPU).", "Sharper downscaling (more GPU).")} valor=${v.lanczos} al=${(x) => vid("lanczos", x)}/>
      <${AjusteSw} titulo=${t("Ritmo de fotogramas", "Frame pacing")} desc=${t("Movimiento más parejo; puede sumar un poco de retraso.", "Smoother motion; may add a bit of delay.")} valor=${v.ritmo} al=${(x) => vid("ritmo", x)}/>
    </${Tarjeta}>
    <${TarjetaCalidad} m=${m}/>
  </div>`;
}

// ---- Audio ----------------------------------------------------------------------------------
function Canal({ c, canal, titulo, desc }) {
  const au = (clave, valor) => accion("ajustes.audio", { canal, clave, valor });
  const [vol, setVol] = useState(c.volumen);
  useEffect(() => setVol(c.volumen), [c.volumen]);
  const nivel = Math.max(0, Math.min(1, c.nivel || 0));
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${titulo} derecha=${html`<${Interruptor} valor=${c.activo} al=${(x) => au("activo", x)} etiqueta=${titulo}/>`}/>
    <div class="ayuda" style="margin-bottom:8px">${desc}</div>
    <${Ajuste} bloque titulo=${t("Dispositivo", "Device")}>
      <${Selector} valor=${c.dispositivo} deshabilitado=${!c.dispositivos?.length}
        opciones=${(c.dispositivos?.length ? c.dispositivos : [t("(ninguno)", "(none)")]).map((x, i) => ({ valor: i, texto: x }))} al=${(x) => au("dispositivo", Number(x))}/></${Ajuste}>
    <${Ajuste} bloque titulo=${`${t("Volumen", "Volume")} · ${vol}%`}>
      <input type="range" min="0" max="100" value=${vol} style="margin-top:12px"
        onInput=${(e) => setVol(Number(e.currentTarget.value))} onChange=${(e) => au("volumen", Number(e.currentTarget.value))}/></${Ajuste}>
    <div class="lab" style="margin-top:12px">${t("NIVEL", "LEVEL")}</div>
    <div class="medidor" style="height:10px;margin-top:8px"><i style=${`width:${Math.round(nivel * 100)}%;background:linear-gradient(90deg,var(--ok),var(--warn) 75%,var(--bad))`}></i></div>
  </${Tarjeta}>`;
}

function Audio({ m }) {
  const a = m.audio;
  if (!a) return html`<${Vacio} titulo=${t("Cargando audio…", "Loading audio…")} texto=""/>`;
  return html`<div class="rej2">
    <${Canal} c=${a.parlantes} canal="parlantes" titulo=${t("SONIDO DEL JUEGO", "GAME SOUND")} desc=${t("Lo que escuchan los invitados (los parlantes de esta PC).", "What guests hear (this PC's speakers).")}/>
    <${Canal} c=${a.mic} canal="mic" titulo=${t("MICRÓFONO", "MICROPHONE")} desc=${t("Tu voz para los invitados. Apágalo si no lo usas.", "Your voice for guests. Turn it off if unused.")}/>
  </div>`;
}

// ---- Overlay --------------------------------------------------------------------------------
const POSICIONES = [
  ["top Left", "↖"], ["top center", "↑"], ["top right", "↗"],
  ["bottom left", "↙"], ["bottom center", "↓"], ["bottom right", "↘"],
];
function Posicion({ valor, al }) {
  return html`<div style="display:grid;grid-template-columns:repeat(3,38px);gap:4px">${POSICIONES.map(([p, f]) => html`
    <button class=${cx("icono-btn", valor === p && "on")} style=${valor === p ? "border-color:var(--acc);color:var(--acc);background:var(--acc-suave)" : ""}
      title=${p} onClick=${() => valor !== p && al(p)}>${f}</button>`)}</div>`;
}

function BloqueOverlay({ titulo, clave, extra, o, ov }) {
  return html`<div class="caja" style="display:flex;flex-direction:column;gap:6px">
    <div style="display:flex;justify-content:space-between;align-items:center"><b>${titulo}</b>
      <${Interruptor} valor=${o[clave].activo} al=${(x) => ov(clave + ".activo", x)} etiqueta=${titulo}/></div>
    <div style="display:flex;justify-content:space-between;align-items:center;gap:10px">
      <span class="ayuda">${t("Posición en pantalla", "Screen position")}</span>
      <${Posicion} valor=${o[clave].posicion} al=${(p) => ov(clave + ".posicion", p)}/></div>
    ${extra}
  </div>`;
}

function Overlay({ m, info }) {
  const o = m.ajustes.overlay;
  const ov = (clave, valor) => accion("ajustes.overlay", { clave, valor });
  const activo = m.sala?.opciones?.overlay;
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto="OVERLAY" derecha=${html`<${Interruptor} valor=${activo} al=${(x) => accion("sala.opciones", { overlay: x })} etiqueta="Overlay"/>`}/>
      <div class="ayuda">${t("Muestra chat, mandos e invitados encima del juego, para ti y para quien mira la transmisión.", "Shows chat, pads and guests over the game.")}
        ${activo ? html` <${Chip} tipo=${o.corriendo ? "mod" : "vip"}>${o.corriendo ? t("FUNCIONANDO", "RUNNING") : t("INICIANDO", "STARTING")}</${Chip}>` : null}</div>
      <${Ajuste} bloque titulo=${t("Pantalla", "Screen")}>
        <${Selector} valor=${o.monitor} opciones=${(info.pantallas.length ? info.pantallas : ["#1"]).map((x, i) => ({ valor: i, texto: x }))} al=${(x) => ov("monitor", Number(x))}/></${Ajuste}>
      <${Ajuste} bloque titulo=${t("Tema del overlay", "Overlay theme")} desc=${t("Carpeta overlay/themes.", "overlay/themes folder.")}>
        <${Selector} valor=${o.tema} opciones=${[{ valor: "", texto: t("(predeterminado)", "(default)") }, ...info.temasOverlay.map((x) => ({ valor: x, texto: x }))]} al=${(x) => ov("tema", x)}/></${Ajuste}>
    </${Tarjeta}>
    <div class="col" style="gap:12px">
      <${BloqueOverlay} o=${o} ov=${ov} titulo="Chat" clave="chat" extra=${html`<${AjusteSw} titulo=${t("Mostrar historial", "Show history")} valor=${o.chat.historial} al=${(x) => ov("chat.historial", x)}/>`}/>
      <${BloqueOverlay} o=${o} ov=${ov} titulo=${t("Mandos", "Pads")} clave="mandos"/>
      <${BloqueOverlay} o=${o} ov=${ov} titulo=${t("Invitados", "Guests")} clave="invitados" extra=${html`<${AjusteSw} titulo=${t("Mostrar latencia", "Show latency")} valor=${o.invitados.latencia} al=${(x) => ov("invitados.latencia", x)}/>`}/>
    </div>
  </div>`;
}

// ---- Permisos por rol ---------------------------------------------------------------------------
function Permisos({ m }) {
  const p = m.ajustes.permisos;
  const grupos = [["guest", t("INVITADOS", "GUESTS")], ["vip", "VIP"], ["moderator", t("MODERADORES", "MODERATORS")]];
  const claves = [
    ["useBB", t("Usar !bb (botones)", "Use !bb"), t("Pulsar botones de cualquier mando desde el chat.", "Press buttons from chat.")],
    ["useSFX", t("Usar !sfx (sonidos)", "Use !sfx"), t("Reproducir efectos de sonido en la sala.", "Play sound effects.")],
    ["changeControls", t("Cambiar de mando", "Change pads"), t("Usar !swap, !pick y similares.", "Use !swap, !pick…")],
  ];
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("PERMISOS POR ROL", "PERMISSIONS BY ROLE")}/>
    <table class="tabla"><thead><tr><th>${t("PERMISO", "PERMISSION")}</th>${grupos.map(([, n]) => html`<th style="text-align:center">${n}</th>`)}</tr></thead>
      <tbody>${claves.map(([k, n, d]) => html`<tr key=${k}><td><div style="font-weight:500">${n}</div><div class="ayuda">${d}</div></td>
        ${grupos.map(([g]) => html`<td style="text-align:center"><${Interruptor} valor=${p[g][k]} etiqueta=${n + " " + g}
          al=${(v) => accion("ajustes.permisos", { grupo: g, clave: k, valor: v })}/></td>`)}</tr>`)}</tbody></table>
  </${Tarjeta}>`;
}

// ---- Cuenta web ----------------------------------------------------------------------------------
function CuentaWeb({ m }) {
  const w = m.web || {};
  const [codigo, setCodigo] = useState("");
  const vincular = async () => {
    const c = codigo.replace(/\D/g, "");
    if (c.length !== 6) { avisar(t("El código tiene 6 dígitos.", "The code has 6 digits."), "warn"); return; }
    if (await accion("web.vincular", { codigo: c })) setCodigo("");
  };
  const desvincular = async () => {
    if (await confirmar({ titulo: t("¿Desvincular esta PC?", "Unlink this PC?"), texto: t("La sala dejará de aparecer en la web hasta que vuelvas a vincular con un código nuevo.", "The room won't show on the website until you link again."), si: t("Desvincular", "Unlink"), peligro: true })) {
      await accion("web.desvincular");
    }
  };
  const vinculada = w.estado && w.estado !== "sin_vincular";
  return html`<div class="rej2">
    <${Tarjeta} vivo=${w.estado === "conectado"} interior="padding:24px 26px;display:flex;flex-direction:column;gap:14px">
      <div class="lab acc">${t("WEB DE LA LIGA", "LEAGUE WEBSITE")}</div>
      <div class="grande degradado" style="font-size:34px">${vinculada ? (w.usuario || t("Vinculada", "Linked")) : t("Sin vincular", "Not linked")}</div>
      <div class="ayuda" style="font-size:14px">${w.mensaje || ""}</div>
      ${vinculada ? html`<div class="rej3" style="gap:10px">
        <div class="caja"><div class="lab">${t("ESTADO", "STATUS")}</div><div style="margin-top:8px;font-weight:600">${(w.estado || "").replace("_", " ").toUpperCase()}</div></div>
        <div class="caja"><div class="lab">${t("EN LA LISTA", "LISTED")}</div><div class="disp" style="margin-top:8px;font-size:22px;font-weight:700">${w.jugadoresLista ?? 0}</div></div>
        <div class="caja"><div class="lab">${t("EVENTOS EN COLA", "QUEUED EVENTS")}</div><div class="disp" style="margin-top:8px;font-size:22px;font-weight:700">${w.eventosEnCola ?? 0}</div></div>
      </div>
      <div style="display:flex;gap:10px;flex-wrap:wrap">
        <${Boton} al=${() => accion("web.reintentar", {}, { ok: t("Reintentando…", "Retrying…") })}>${t("REINTENTAR AHORA", "RETRY NOW")}</${Boton}>
        ${m.sala?.abierta && w.publicada ? html`<${Boton} tipo="warn" al=${() => accion("web.soltarRival", {}, { ok: t("Rival liberado: la sala vuelve al radar.", "Rival released.") })}>${t("SOLTAR RIVAL DEL RADAR", "RELEASE RADAR RIVAL")}</${Boton}>` : null}
        <${Boton} tipo="peligro" al=${desvincular}>${t("DESVINCULAR", "UNLINK")}</${Boton}>
      </div>` : null}
    </${Tarjeta}>
    <${Tarjeta} interior="padding:24px 26px;display:flex;flex-direction:column;gap:12px">
      <div class="lab acc">${vinculada ? t("VINCULAR DE NUEVO", "LINK AGAIN") : t("CÓMO VINCULAR", "HOW TO LINK")}</div>
      <ol class="ayuda" style="font-size:14px;line-height:1.7;margin:0;padding-left:20px">
        <li>${t("Entra a la web de la liga con tu cuenta.", "Sign in on the league website.")}</li>
        <li>${t("Ve a Mi perfil › pestaña «Smash Soda» (o a Mis salas) y genera el código.", "Go to My profile › «Smash Soda» tab (or My rooms) and generate the code.")}</li>
        <li>${t("Escribe aquí los 6 dígitos (vale 10 minutos).", "Type the 6 digits here (valid 10 minutes).")}</li>
      </ol>
      <div style="display:flex;gap:10px">
        <input class="campo mono" inputmode="numeric" maxLength="7" placeholder="123456" value=${codigo} style="font-size:22px;letter-spacing:.3em;text-align:center;height:52px"
          onInput=${(e) => setCodigo(e.currentTarget.value.replace(/[^\d]/g, "").slice(0, 6))} onKeyDown=${(e) => e.key === "Enter" && vincular()}/>
        <${Boton} tipo="lleno enorme" deshabilitado=${codigo.length !== 6} al=${vincular}>${t("VINCULAR", "LINK")}</${Boton}>
      </div>
      <div class="ayuda">${t("Nunca compartas el código: es como una contraseña de un solo uso.", "Never share the code: it's a one-time password.")}</div>
      ${w.versionLiga ? html`<div class="mono mut" style="font-size:12px">${t("Versión de la liga", "League version")}: ${w.versionLiga}</div>` : null}
    </${Tarjeta}>
  </div>`;
}

// ---- Interfaz --------------------------------------------------------------------------------------
function Interfaz({ m, info }) {
  const idiomas = info.idiomas?.length ? info.idiomas : [{ codigo: "es", nombre: "Español" }, { codigo: "en", nombre: "English" }];
  const cambiarInterfaz = async () => {
    if (await confirmar({ titulo: t("¿Volver a la interfaz anterior?", "Go back to the previous interface?"), texto: t("Se usará la interfaz Phoenix dibujada con ImGui. Puedes volver desde Ajustes › Rápido › «Usar la interfaz nueva».", "The ImGui Phoenix interface will be used. Come back from Settings › Quick."), si: t("Cambiar", "Switch") })) {
      await accion("ui.interfaz", { modo: "clasica" });
    }
  };
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("IDIOMA", "LANGUAGE")}/>
      <${Selector} valor=${m.app?.idioma || "es"} opciones=${idiomas.map((x) => ({ valor: x.codigo, texto: x.nombre }))} al=${(x) => accion("ui.idioma", { idioma: x })}/>
      <div class="ayuda" style="margin-top:10px">${t("Cambia también los textos del panel clásico.", "Also changes the classic panel.")}</div>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("INTERFAZ", "INTERFACE")}/>
      <div class="ayuda">${t("Atajos: Ctrl+K busca cualquier acción · Ctrl+Espacio abre el chat · Alt+1…5 cambia de sección · Esc cierra.", "Shortcuts: Ctrl+K search · Ctrl+Space chat · Alt+1…5 sections · Esc close.")}</div>
      <div style="display:flex;gap:10px;flex-wrap:wrap">
        <${Boton} al=${() => accion("ui.recargar")}>${t("RECARGAR INTERFAZ", "RELOAD INTERFACE")}</${Boton}>
        <${Boton} tipo="suave" al=${() => clasico(0)}>${t("ABRIR PANEL CLÁSICO", "OPEN CLASSIC PANEL")}</${Boton}>
        <${Boton} tipo="peligro" al=${cambiarInterfaz}>${t("USAR LA INTERFAZ ANTERIOR", "USE PREVIOUS INTERFACE")}</${Boton}>
      </div>
    </${Tarjeta}>
  </div>`;
}

// ---- Diagnóstico -------------------------------------------------------------------------------------
const diag = () => ({
  webview2: [t("Motor de la interfaz (WebView2)", "Interface engine (WebView2)"), (d) => d.version ? `${t("Versión", "Version")} ${d.version}` : t("No instalado: la app usa la interfaz anterior.", "Missing: the app uses the previous interface.")],
  vigem: [t("Mandos virtuales (ViGEmBus)", "Virtual pads (ViGEmBus)"), (d) => d.instalado ? `${d.conectados}/${d.mandos} ${t("conectados", "connected")}` : t("Falta instalar ViGEmBus: sin él nadie puede jugar.", "ViGEmBus missing: nobody can play.")],
  parsec: [t("Cuenta de Parsec", "Parsec account"), (d) => d.cuenta ? `${d.nombre}${d.listo ? "" : t(" · preparando", " · starting")}` : t("Inicia sesión en Parsec.", "Sign in to Parsec.")],
  video: [t("Video", "Video"), (d) => `${d.gpus} GPU · ${d.pantallas} ${t("pantallas", "screens")}${d.wgc ? " · WGC" : ""}`],
  audio: [t("Audio", "Audio"), (d) => `${d.salidas} ${t("salidas", "outputs")} · ${d.entradas} ${t("entradas", "inputs")}`],
  config: [t("Carpeta de configuración", "Config folder"), (d) => d.ruta || ""],
  web: [t("Web de la liga", "League website"), (d) => d.usuario ? `${d.usuario} · ${d.mensaje || ""}` : (d.mensaje || t("Sin vincular", "Not linked"))],
  overlay: [t("Overlay", "Overlay"), (d) => d.activo ? (d.websocket ? `WebSocket :${d.puerto}` : t("WebSocket apagado", "WebSocket off")) : t("Desactivado", "Off")],
  sistema: [t("Sistema", "System"), (d) => `${d.windows || "Windows"} · Phoenix ${d.app} · Soda ${d.soda}`],
});

function Diagnostico() {
  const [r, setR] = useState(null);
  const correr = async () => { setR(null); const x = await accion("diag.ejecutar"); setR(x?.chequeos || []); };
  useEffect(() => { correr(); }, []);
  const errores = (r || []).filter((c) => c.estado === "error").length;
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("DIAGNÓSTICO DEL PC", "PC DIAGNOSTICS")} derecha=${html`<${Boton} tipo="mini" al=${correr}>${t("REVISAR DE NUEVO", "RUN AGAIN")}</${Boton}>`}/>
    ${!r ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : html`
      <div class=${cx("caja")} style=${`margin-bottom:6px;border-color:${errores ? "var(--bad)" : "var(--ok)"}`}>
        <b class=${errores ? "bad" : "ok"}>${errores ? t(`${errores} problema(s) para resolver`, `${errores} problem(s)`) : t("Todo listo para hostear.", "All set to host.")}</b></div>
      ${r.map((c) => { const d = diag()[c.id] || [c.id, () => JSON.stringify(c.datos)]; return html`<div class="chequeo" key=${c.id}>
        <span class=${cx("icono", c.estado)}>${c.estado === "ok" ? "✓" : c.estado === "aviso" ? "!" : "×"}</span>
        <div style="min-width:0"><div style="font-weight:600">${d[0]}</div><div class="ayuda" style="word-break:break-word">${d[1](c.datos || {})}</div></div>
      </div>`; })}`}
  </${Tarjeta}>`;
}

// ---- Avanzado ----------------------------------------------------------------------------------------
function Avanzado({ m }) {
  const g = m.ajustes.general;
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto="WEBSOCKET"/>
      <div class="ayuda">${t("Lo usan el overlay y herramientas externas (bots, OBS).", "Used by the overlay and external tools.")}</div>
      <${AjusteSw} titulo=${t("Servidor WebSocket", "WebSocket server")} desc=${g.socketActivo ? t("Funcionando.", "Running.") : t("Apagado.", "Stopped.")} valor=${g.socketEnabled} al=${(v) => gen("socketEnabled", v)}/>
      <${Ajuste} titulo=${t("Puerto", "Port")} desc="1024–65535"><div style="width:130px"><${Campo} mono soloNumeros valor=${String(g.socketPort)} max=${5}
        al=${(v) => { const n = Number(v); if (n >= 1024 && n <= 65535) gen("socketPort", n); else avisar(t("Puerto entre 1024 y 65535.", "Port between 1024 and 65535."), "warn"); }}/></div></${Ajuste}>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("REGISTROS Y DESARROLLO", "LOGS AND DEVELOPMENT")}/>
      <${AjusteSw} titulo=${t("Registros de Parsec", "Parsec logs")} desc=${t("Muestra los registros de Parsec en el Registro.", "Shows Parsec logs in the log.")} valor=${g.parsecLogs} al=${(v) => gen("parsecLogs", v)}/>
      <${AjusteSw} titulo=${t("Modo desarrollador", "Developer mode")} desc=${t("Herramientas extra. Al reiniciar, permite inspeccionar esta interfaz con F12.", "Extra tools; after restart enables F12 here.")} valor=${g.devMode} al=${(v) => gen("devMode", v)}/>
      <div style="margin-top:14px"><${Boton} al=${() => clasico(6)}>${t("AVANZADO DEL PANEL CLÁSICO", "CLASSIC ADVANCED")}</${Boton}></div>
    </${Tarjeta}>
  </div>`;
}
