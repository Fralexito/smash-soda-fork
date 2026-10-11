// =============================================================================
//  PHOENIX SYNC: el puente entre la web y el juego. Muestra si el juego está
//  abierto, si el puente de Sider está instalado, los últimos avisos de la web
//  y deja apagar el envío. «Fichajes» muestra la última entrega de datos que
//  Phoenix Sync dejó en el juego (con botón para deshacerla).
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, elegirPestana, confirmar } from "../tienda.js";
import { CuentaWeb } from "./ajustes.js";
import { Tarjeta, Titulo, AjusteSw, Segmentos, Vacio, Chip, Icono, Boton, cx } from "../ui.js";

export const PESTANAS_SYNC = [
  { id: "puente", es: "PUENTE", en: "BRIDGE", d: ["Los avisos de la web salen dentro del juego mientras juegas.", "Web notices show up inside the game while you play."] },
  { id: "web", es: "CUENTA WEB", en: "WEB ACCOUNT", d: ["Vincula esta PC con tu cuenta de la liga para aparecer en el radar y ver a tus amigos.", "Link this PC to your league account to show on the radar and see your friends."] },
  { id: "fichajes", es: "FICHAJES", en: "TRANSFERS", d: ["Los datos y fichajes que Phoenix Sync dejó en tu juego.", "The data and transfers Phoenix Sync placed in your game."] },
];

const CUENTA = { conectado: ["ok", "CONECTADA", "CONNECTED"], vinculando: ["warn", "VINCULANDO", "LINKING"], sin_conexion: ["bad", "SIN CONEXIÓN", "OFFLINE"], pausado: ["warn", "EN PAUSA", "PAUSED"], sin_vincular: ["", "SIN VINCULAR", "NOT LINKED"] };
const DATOS = { ninguna: ["", "SIN ENTREGAS", "NO DELIVERIES"], esperando: ["warn", "ESPERANDO AL JUEGO", "WAITING FOR GAME"], colocada: ["ok", "EN EL JUEGO", "IN GAME"], rechazada: ["bad", "RECHAZADA", "REJECTED"], deshecha: ["warn", "DESHECHA", "UNDONE"] };

/** Tres pastillas: cuenta de la liga · puente con el juego · últimos datos entregados. */
export function metaSync(s) {
  const m = s.motor || {};
  const b = m.buzon;
  const c = CUENTA[m.web?.estado] || CUENTA.sin_vincular;
  const j = b?.estado === "conectado" ? "ok" : b?.estado === "noinstalado" ? "warn" : b?.estado === "sinconexion" ? "bad" : "";
  const d = DATOS[m.entrega?.estado] || DATOS.ninguna;
  const p = (estado, et, texto, pes) => html`<button class="pill" onClick=${() => elegirPestana(pes)}><span class=${cx("punto", estado, estado === "ok" && "vivo")}></span><span class="mut">${et}</span>${texto}</button>`;
  return html`<div class="grupo-estado">
    ${p(c[0], t("CUENTA ·", "ACCOUNT ·"), t(c[1], c[2]), "web")}
    ${p(j, t("JUEGO ·", "GAME ·"), ESTADOS(b)[0].toUpperCase(), "puente")}
    ${p(d[0], t("DATOS ·", "DATA ·"), t(d[1], d[2]), "fichajes")}
  </div>`;
}

function ESTADOS(b) {
  const e = b?.estado || "cerrado";
  const tabla = {
    apagado: [t("Apagado", "Off"), "", t("Tú apagaste el envío de avisos al juego.", "You turned off notices to the game.")],
    cerrado: [t("Juego cerrado", "Game closed"), "", t("Abre PES 2021 y Phoenix Link lo detecta solo.", "Open PES 2021 and Phoenix Link finds it by itself.")],
    noinstalado: [t("Puente Sider no instalado", "Sider bridge not installed"), "warn", t("El juego está abierto, pero su parche no trae el módulo «phoenix» de Sider. No se escribe nada.", "The game is open but its patch has no Sider «phoenix» module. Nothing is written.")],
    conectado: [b?.ultimo ? t(`Conectado · último aviso ${b.ultimo}`, `Connected · last notice ${b.ultimo}`) : t("Conectado", "Connected"), "ok vivo", t("Los avisos de la web llegan al juego en pocos segundos.", "Web notices reach the game within seconds.")],
    sinconexion: [t("Sin conexión", "No connection"), "bad", t("No se pudo hablar con la web. Se reintenta solo, con calma.", "Couldn't reach the web. Retrying by itself.")],
  };
  return tabla[e] || tabla.cerrado;
}

// Prueba «Solo PES 2021»: la sala solo se publica (y en estricto, solo se abre) con PES 2021 abierto.
function SoloPes({ m }) {
  const p = m.pes || { modo: "off", abierto: false };
  const oculta = p.modo !== "off" && !p.abierto;
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("SOLO PES 2021 (PRUEBA)", "PES 2021 ONLY (TEST)")}/>
    <${Segmentos} valor=${p.modo} al=${(v) => accion("ajustes.modoPes", { valor: v })}
      opciones=${[{ valor: "off", texto: t("APAGADO", "OFF") }, { valor: "publicar", texto: t("SOLO PUBLICAR", "PUBLISH ONLY") }, { valor: "estricto", texto: t("ESTRICTO", "STRICT") }]}/>
    <div class="ayuda" style="font-size:14px">${p.modo === "off"
      ? t("Apagado: Link funciona con cualquier juego, como siempre.", "Off: Link works with any game, as always.")
      : p.modo === "publicar"
        ? t("Tu sala solo aparece en la web mientras PES 2021 está abierto.", "Your room only shows on the web while PES 2021 is open.")
        : t("Además, Link no te deja abrir la sala si PES 2021 está cerrado.", "Also, Link won't let you open the room while PES 2021 is closed.")}</div>
    ${oculta ? html`<div class="ayuda" style="font-size:14px;color:var(--warn, #ffb347)">${t("PES 2021 está cerrado: tu sala no se publica.", "PES 2021 is closed: your room is not published.")}</div>` : null}
  </${Tarjeta}>`;
}

// Modo de recarga de phoenix.lua v0.18b (Link escribe content\phoenix\modo.txt). Textos del pedido de Sync.
// «Automático: siempre» va primero y por defecto (PROMPT-LINK-modo-por-defecto.md de Sync).
const MODOS_RECARGA = {
  "ACTIVAR": ["Los fichajes se cargan solo cuando pulsas Partido → Datos Actual. en vivo → Activar.", "Transfers load only when you press Match → Live update → Activate."],
  "AUTO-FICHAJES": ["Activar sigue funcionando. Además, cuando haya fichajes nuevos, el juego los carga solo al volver al menú principal y entrar a un modo.", "Activate still works. Also, when there are new transfers, the game loads them by itself when you go back to the main menu and enter a mode."],
  "AUTO-SIEMPRE": ["Los fichajes entran solos cada vez que vuelves al menú principal y entras a un modo. No hay que pulsar nada.", "Transfers come in by themselves every time you go back to the main menu and enter a mode. Nothing to press."],
};
function ModoRecarga({ m }) {
  const v = m.arbitro?.modoRecarga || "AUTO-SIEMPRE";
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("MODO DE RECARGA DE FICHAJES", "TRANSFER RELOAD MODE")}/>
    <${Segmentos} valor=${v} al=${(x) => accion("sync.modoRecarga", { modo: x })}
      opciones=${[{ valor: "AUTO-SIEMPRE", texto: t("🟣 AUTO: SIEMPRE ⭐", "🟣 AUTO: ALWAYS ⭐") }, { valor: "AUTO-FICHAJES", texto: t("🔵 AUTO: FICHAJES", "🔵 AUTO: TRANSFERS") }, { valor: "ACTIVAR", texto: t("🟢 ACTIVAR", "🟢 ACTIVATE") }]}/>
    <div class="ayuda" style="font-size:14px">${t(MODOS_RECARGA[v][0], MODOS_RECARGA[v][1])}</div>
    <div class="ayuda" style="font-size:13px">${t("⭐ Automático: siempre es el recomendado y viene por defecto. Necesita phoenix.lua v0.18b (instálala abajo, en Módulos del juego). Si lo automático falla 3 veces, el juego vuelve solo a Activar. El juego nota el cambio en unos 2 segundos.", "⭐ Auto: always is recommended and the default. Needs phoenix.lua v0.18b (install it below, in Game modules). If auto fails 3 times, the game falls back to Activate. The game notices the change in about 2 seconds.")}</div>
  </${Tarjeta}>`;
}

function Puente({ m }) {
  const b = m.buzon || { activo: true, estado: "cerrado", ultimo: "", juego: "", avisos: [] };
  const [texto, punto, ayuda] = ESTADOS(b);
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:1 1 0;min-width:320px">
      <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
        <${Titulo} texto=${t("ESTADO DEL PUENTE", "BRIDGE STATUS")}/>
        <div style="display:flex;align-items:center;gap:12px">
          <span class=${cx("punto", punto)} style="width:14px;height:14px"></span>
          <span class="disp" style="font-size:22px;font-weight:700">${texto}</span>
        </div>
        <div class="ayuda" style="font-size:14px">${ayuda}</div>
        <div>
          <div class="lab" style="margin-bottom:6px">${t("JUEGO DETECTADO", "DETECTED GAME")}</div>
          ${b.juego ? html`<div class="caja mono" style="font-size:13px;word-break:break-all">${b.juego}</div>`
                    : html`<div class="ayuda">${t("Ninguno por ahora.", "None right now.")}</div>`}
          ${b.parche ? html`<div class="ayuda" style="margin-top:6px">${t("Parche detectado:", "Detected patch:")} <b>${b.parche}</b></div>` : null}
        </div>
        <${AjusteSw} titulo=${t("Mostrar avisos de la web en el juego", "Show web notices in the game")}
          desc=${t("Con PES 2021 abierto, los avisos salen en el panel de Sider (barra espaciadora). Si lo apagas, no se consulta ni se escribe nada.", "With PES 2021 open, notices show in the Sider panel (space bar). If off, nothing is fetched or written.")}
          valor=${b.activo} al=${(v) => accion("ajustes.avisosJuego", { valor: v })}/>
      </${Tarjeta}>
      <${ModoRecarga} m=${m}/>
      <${SoloPes} m=${m}/>
    </div>
    <div class="col" style="flex:1 1 0;min-width:320px">
      <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
        <${Titulo} texto=${t("ÚLTIMOS AVISOS", "LATEST NOTICES")} derecha=${html`<span class="mono mut">${(b.avisos || []).length} / 5</span>`}/>
        ${(b.avisos || []).length === 0
          ? html`<div class="ayuda">${t("Aún no hay avisos. Los que mandes desde la web aparecerán aquí.", "No notices yet. Anything you send from the web shows up here.")}</div>`
          : (b.avisos || []).map((a) => html`<div class="fila-mar" key=${a.id} style="align-items:flex-start">
              <span class="mono mut" style="font-size:13px;flex:none;padding-top:2px">${a.hora}</span>
              <div style="flex:1;min-width:0;word-break:break-word">${a.texto}</div>
              ${a.escrito ? html`<${Chip} tipo="acc">✅ ${t("EN EL JUEGO", "IN GAME")}</${Chip}>` : html`<${Chip}>${t("PENDIENTE", "PENDING")}</${Chip}>`}
            </div>`)}
      </${Tarjeta}>
      <${Modulos}/>
      <${Doctor}/>
    </div>
  </div>`;
}

// ---- Phoenix Doctor (MODO INFORMATIVO): ficha del juego del anfitrión. Aún no hay parche oficial: no aprueba ni rechaza. ----
function Doctor() {
  const [d, setD] = useState(null);
  const [ver, setVer] = useState(false);
  const revisar = async () => { const r = await accion("sync.doctor", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { revisar(); }, []);
  const phoenixLuas = (d?.luaActivos || []).filter((x) => /phoenix/i.test(x));
  const conDatos = (d?.archivos || []).filter((a) => a.sha256);
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("PHOENIX DOCTOR", "PHOENIX DOCTOR")}
      derecha=${html`<div style="display:flex;gap:8px;align-items:center"><${Chip}>${t("INFORMATIVO", "INFO ONLY")}</${Chip}><${Boton} tipo="suave mini" al=${revisar}>${t("REVISAR", "CHECK")}</${Boton}></div>`}/>
    <div class="ayuda" style="font-size:14px">
      ${t("Ficha del juego de este anfitrión: parche, módulos y una huella de los archivos que deciden el partido. Aún no hay parche oficial, así que no aprueba ni rechaza nada: la huella viaja con cada resultado oficial para que el staff la vea.",
          "This host's game sheet: patch, modules and a fingerprint of the files that decide the match. There is no official patch yet, so it approves or rejects nothing: the fingerprint travels with each official result for staff.")}
    </div>
    ${!d ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : !d.juegoEncontrado
      ? html`<div class="ayuda">${(d.avisos || [])[0] || t("No encuentro el juego.", "Game not found.")}</div>`
      : html`<div class="col" style="gap:6px;font-size:14px">
          <div><span class="mut">${t("Parche:", "Patch:")}</span> <b>${d.parche}</b> · ${(d.modos || []).length} ${t("carpetas de Sider", "Sider folders")}</div>
          <div><span class="mut">${t("Módulos Phoenix activos:", "Active Phoenix modules:")}</span> <b>${phoenixLuas.length ? phoenixLuas.join(", ") : t("ninguno", "none")}</b> · ${(d.luaActivos || []).length} ${t("Luas en total", "Luas in total")}</div>
          <div><span class="mut">${t("Raíces de la base:", "Database roots:")}</span> ${(d.cpkRoots || []).slice(0, 3).join(" → ")}${(d.cpkRoots || []).length > 3 ? " …" : ""}</div>
          <div><span class="mut">${t("Huella del anfitrión:", "Host fingerprint:")}</span> <span class="mono" title=${d.huella}>${(d.huella || "").slice(0, 16)}…</span> <span class="mut">· ${d.generado}</span></div>
          ${(d.avisos || []).map((x) => html`<div style="color:var(--warn)">⚠️ ${x}</div>`)}
          <div><${Boton} tipo="mini suave" al=${() => setVer(!ver)}>${ver ? t("OCULTAR ARCHIVOS", "HIDE FILES") : t(`VER ARCHIVOS (${conDatos.length})`, `SHOW FILES (${conDatos.length})`)}</${Boton}></div>
          ${ver ? html`<div class="mono mut" style="font-size:12px;display:flex;flex-direction:column;gap:2px">
            ${(d.archivos || []).map((a) => html`<span key=${a.nombre}>${a.sha256 ? "●" : "·"} ${a.nombre}${a.sha256 ? " · " + a.sha256.slice(0, 12) + "…" : t(" · no está", " · missing")}</span>`)}
          </div>` : null}
        </div>`}
  </${Tarjeta}>`;
}

// ---- Módulos del juego: el paquete de Luas de Sider que instala Phoenix Link (solo con PES cerrado) ----
const ESTADO_MODULO = {
  instalado: [["INSTALADO", "INSTALLED"], "acc"],
  desactualizado: [["HAY VERSIÓN NUEVA", "UPDATE AVAILABLE"], "vip"],
  apagado: [["APAGADO", "OFF"], ""],
  a_medias: [["A MEDIAS", "PARTIAL"], "vip"],
  no_instalado: [["NO INSTALADO", "NOT INSTALLED"], ""],
  solo_lectura: [["LO GESTIONA SYNC", "MANAGED BY SYNC"], ""],
};
function Modulos() {
  const [d, setD] = useState(null);
  const [ocupado, setOcupado] = useState("");
  const cargar = async () => { const r = await accion("sync.modulos", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { cargar(); }, []);
  const hacer = async (archivo, que) => {
    setOcupado(archivo);
    const r = await accion("sync.modulo", { archivo, accion: que });
    setOcupado("");
    if (r) { setD(r); if (r.mensaje) avisar(r.mensaje, "ok", 7000); }
  };
  const lista = d?.modulos || [];
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("MÓDULOS DEL JUEGO", "GAME MODULES")}
      derecha=${html`<${Boton} tipo="suave mini" al=${cargar}>${t("REVISAR", "CHECK")}</${Boton}>`}/>
    <div class="ayuda" style="font-size:14px">
      ${t("El paquete de Luas de Phoenix para Sider. Se instala en la raíz del juego y en cada modo del parche, con copia de sider.ini antes de cada cambio.",
          "Phoenix's Lua pack for Sider. Installed in the game root and in every patch mode, backing up sider.ini before each change.")}
    </div>
    ${d?.pesAbierto ? html`<div class="caja" style="font-size:14px"><span class="punto warn"></span> ${t("Cierra PES 2021 para instalar o apagar: Sider lee los módulos al arrancar.", "Close PES 2021 to install or turn off: Sider loads modules at startup.")}</div>` : null}
    ${d && !d.juego ? html`<div class="ayuda">${t("Abre PES 2021 una vez para que Phoenix Link conozca la carpeta del juego.", "Open PES 2021 once so Phoenix Link learns the game folder.")}</div>` : null}
    ${!d ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : lista.map((x) => {
      const [et, tipo] = ESTADO_MODULO[x.estado] || [[x.estado, x.estado], ""];
      const puedeInstalar = x.gestionable && x.estado !== "instalado";
      const puedeQuitar = x.gestionable && (x.estado === "instalado" || x.estado === "desactualizado" || x.estado === "a_medias");
      const bloqueado = !!d.pesAbierto || !d.juego || !!ocupado;
      return html`<div class="caja" key=${x.archivo} style="display:flex;flex-direction:column;gap:8px">
        <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
          <b class="mono" style="font-size:14px">${x.archivo}${x.version ? html` <span class="mut">v${x.version}</span>` : null}</b>
          <${Chip} tipo=${tipo}>${t(et[0], et[1])}</${Chip}>
        </div>
        <div class="ayuda" style="font-size:13px">${x.descripcion}</div>
        ${x.carga ? html`<div class="ayuda" style="font-size:13px">${x.carga === "cargado" ? t("✅ Cargó en el último arranque del juego.", "✅ Loaded on the last game start.") : t("⚠️ Sider no lo pudo cargar en el último arranque (mira sider.log).", "⚠️ Sider could not load it on the last start (see sider.log).")}</div>` : null}
        ${(x.destinos || []).length ? html`<div class="mono mut" style="font-size:12px;display:flex;flex-direction:column;gap:2px">
          ${x.destinos.map((ds) => html`<span key=${ds.carpeta}>${ds.linea ? "●" : ds.comentada ? "○" : "·"} ${ds.nombre}${ds.version ? " · v" + ds.version : ""}${ds.archivo && x.gestionable && !ds.igual ? t(" · distinto", " · different") : ""}</span>`)}
        </div>` : null}
        ${x.gestionable ? html`<div style="display:flex;gap:8px;flex-wrap:wrap">
          ${puedeInstalar ? html`<${Boton} tipo="lleno mini" deshabilitado=${bloqueado} al=${() => hacer(x.archivo, "instalar")}>${x.estado === "desactualizado" ? t("ACTUALIZAR", "UPDATE") : x.estado === "apagado" ? t("ENCENDER", "TURN ON") : t("INSTALAR", "INSTALL")}</${Boton}>` : null}
          ${puedeQuitar ? html`<${Boton} tipo="suave mini" deshabilitado=${bloqueado} titulo=${t("Deja de cargarse en el juego; el archivo se queda para volver a encenderlo", "Stops loading in the game; the file stays")} al=${() => hacer(x.archivo, "quitar")}>${t("APAGAR", "TURN OFF")}</${Boton}>` : null}
          ${x.gestionable && x.estado !== "no_instalado" ? html`<${Boton} tipo="peligro mini" deshabilitado=${bloqueado} titulo=${t("Lo quita del juego por completo (con copia de seguridad)", "Removes it from the game completely (with backup)")}
            al=${async () => { if (await confirmar({ titulo: t(`¿Desinstalar ${x.archivo}?`, `Uninstall ${x.archivo}?`), texto: x.archivo === "phoenix.lua"
              ? t("Se quita del juego y de sider.ini (también la carpeta Phoenix-DB deja de usarse: vuelves a los datos del parche). Se guarda copia de todo y puedes volver a instalarlo cuando quieras.", "Removed from the game and sider.ini (Phoenix-DB stops being used). Everything is backed up; reinstall any time.")
              : t("Se quita del juego y de sider.ini. Se guarda copia de todo y puedes volver a instalarlo cuando quieras.", "Removed from the game and sider.ini. Everything is backed up; reinstall any time."), si: t("Desinstalar", "Uninstall"), peligro: true })) hacer(x.archivo, "desinstalar"); }}>${t("DESINSTALAR", "UNINSTALL")}</${Boton}>` : null}
        </div>` : null}
      </div>`;
    })}
  </${Tarjeta}>`;
}

function Entrega({ m }) {
  const en = m.entrega || { estado: "ninguna" };
  const tabla = {
    ninguna: [t("Aún no hay entregas", "No deliveries yet"), "", ""],
    esperando: [t("Entrega esperando al juego", "Delivery waiting for the game"), "warn", t("Abre PES 2021 una vez para que Phoenix Link encuentre su carpeta. No se escribe nada hasta entonces.", "Open PES 2021 once so Phoenix Link can find its folder. Nothing is written until then.")],
    colocada: [t("Colocada", "Placed"), "ok", t("En el juego pulsa: Partido → Datos Actual. en vivo → Activar.", "In the game press: Match → Live data update → Activate.")],
    rechazada: [t("Rechazada", "Rejected"), "bad", t("No se tocó nada de tu juego.", "Nothing in your game was touched.")],
    deshecha: [t("Deshecha", "Undone"), "warn", t("Se restauraron los datos anteriores. Pulsa Activar en el juego para recargarlos.", "Previous data restored. Press Activate in the game to reload it.")],
  };
  const [texto, punto, ayuda] = tabla[en.estado] || tabla.ninguna;
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
    <${Titulo} texto=${t("ÚLTIMA ENTREGA", "LAST DELIVERY")} derecha=${en.fecha ? html`<span class="mono mut">${en.fecha}</span>` : null}/>
    <div style="display:flex;align-items:center;gap:12px">
      <span class=${cx("punto", punto)} style="width:14px;height:14px"></span>
      <span class="disp" style="font-size:22px;font-weight:700">${texto}</span>
    </div>
    ${en.resumen ? html`<div class="caja" style="font-size:14px;word-break:break-word">${en.resumen}</div>` : null}
    ${en.motivo ? html`<div class="ayuda" style="font-size:14px;color:var(--bad, #ff6b6b);word-break:break-word">${en.motivo}</div>` : null}
    ${ayuda ? html`<div class="ayuda" style="font-size:14px">${ayuda}</div>` : null}
    <div>
      <${Boton} tipo="suave" deshabilitado=${!en.puedeDeshacer}
        titulo=${t("Restaura el option file y el Player.bin anteriores", "Restores the previous option file and Player.bin")}
        al=${() => accion("sync.deshacerEntrega", {}, { ok: t("Orden enviada: se deshace en unos segundos.", "Sent: it will be undone in a few seconds.") })}>${t("Deshacer última entrega", "Undo last delivery")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

export function VistaSync({ s, pestana }) {
  const m = s.motor;
  if (!m) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  if (pestana === "web") return html`<${CuentaWeb} m=${m}/>`;
  if (pestana === "fichajes") return html`<${Entrega} m=${m}/>`;
  return html`<${Puente} m=${m}/>`;
}
