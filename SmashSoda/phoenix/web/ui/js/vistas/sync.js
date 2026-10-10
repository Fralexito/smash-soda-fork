// =============================================================================
//  PHOENIX SYNC: el puente entre la web y el juego. Muestra si el juego está
//  abierto, si el puente de Sider está instalado, los últimos avisos de la web
//  y deja apagar el envío. «Fichajes» muestra la última entrega de datos que
//  Phoenix Sync dejó en el juego (con botón para deshacerla).
// =============================================================================
import { html } from "../lib.js";
import { t } from "../i18n.js";
import { accion } from "../tienda.js";
import { CuentaWeb } from "./ajustes.js";
import { Tarjeta, Titulo, AjusteSw, Segmentos, Vacio, Chip, Icono, Boton, cx } from "../ui.js";

export const PESTANAS_SYNC = [
  { id: "puente", es: "PUENTE", en: "BRIDGE", d: ["Los avisos de la web salen dentro del juego mientras juegas.", "Web notices show up inside the game while you play."] },
  { id: "web", es: "CUENTA WEB", en: "WEB ACCOUNT", d: ["Vincula esta PC con tu cuenta de la liga para aparecer en el radar y ver a tus amigos.", "Link this PC to your league account to show on the radar and see your friends."] },
  { id: "fichajes", es: "FICHAJES", en: "TRANSFERS", d: ["Los datos y fichajes que Phoenix Sync dejó en tu juego.", "The data and transfers Phoenix Sync placed in your game."] },
];

export function metaSync(s) {
  const b = s.motor?.buzon;
  const ok = b && b.estado === "conectado";
  return html`<div style="display:flex;align-items:center;gap:10px">
    <span class=${cx("punto", ok ? "ok vivo" : b?.estado === "noinstalado" ? "warn" : b?.estado === "sinconexion" ? "bad" : "")}></span>
    <span class="mono mut" style="font-size:14px">${ESTADOS(b)[0]}</span>
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
    </div>
  </div>`;
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
