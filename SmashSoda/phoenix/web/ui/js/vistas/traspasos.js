// =============================================================================
//  TRASPASOS EN VIVO (SYNC › TRASPASOS). Lo que haces en Editar de PES (un jugador cambia de equipo) se refleja, ya guardado,
//  en el PES de tu amigo. Anfitrión: Link vigila el option file y publica los traspasos. Amigo: Link los recibe, los aplica
//  sobre SU base (Phoenix-DB) y avisa a phoenix.lua, que recarga solo en un momento seguro según su modo de recarga.
//  Todo APAGADO por defecto. Guía: docs/TRASPASOS-EN-VIVO.md
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, AjusteSw, Chip, Boton, cx } from "../ui.js";

const ANFITRION = {
  apagado: ["", "Apagado", "Off"],
  referencia_borrada: ["warn", "Referencia olvidada: la próxima vez que PES guarde, solo se toma la foto de partida", "Reference forgotten: the next time PES saves, only the starting snapshot is taken"],
  esperando_que_termine_de_guardar: ["warn", "PES está guardando… espero a que termine", "PES is saving… waiting for it to finish"],
  referencia_guardada: ["ok", "Listo: guardé la foto de partida. Haz un traspaso en Editar y guarda", "Ready: starting snapshot saved. Make a transfer in Edit and save"],
  sin_cambios: ["ok", "Vigilando. No hay cambios desde la última vez", "Watching. No changes since last time"],
  sin_traspasos: ["ok", "Vigilando. El último guardado no tenía traspasos nuevos", "Watching. The last save had no new transfers"],
  publicado: ["ok", "Traspaso publicado", "Transfer published"],
  descartado: ["warn", "Descartado: no se envió", "Discarded: not sent"],
  pendiente_de_confirmar: ["warn", "Detecté traspasos: confírmalos para enviarlos", "Transfers detected: confirm to send them"],
  pendiente_masivo: ["warn", "Demasiados cambios a la vez (¿cargaste otro option file?): no se envía solo", "Too many changes at once (did you load another option file?): not sent automatically"],
  sin_option_file: ["bad", "No encuentro tu option file (EDIT00000000). Indícalo abajo", "I can't find your option file (EDIT00000000). Set it below"],
};
const ESTADO_LOTE = { pendiente: ["warn", "PENDIENTE", "PENDING"], aplicado: ["ok", "APLICADO", "APPLIED"], parcial: ["vip", "PARCIAL", "PARTIAL"], saltado: ["bad", "SALTADO", "SKIPPED"], deshecho: ["", "DESHECHO", "UNDONE"], propio: ["", "TUYO", "YOURS"], error: ["bad", "ERROR", "ERROR"] };
const ESTADO_CAMBIO = { aplicado: ["ok", "aplicado", "applied"], ya_estaba: ["", "ya estaba", "already there"], saltado: ["bad", "saltado", "skipped"], reemplazado: ["", "reemplazado por uno posterior", "replaced by a later one"], pendiente: ["warn", "pendiente", "pending"], publicado: ["ok", "publicado", "published"] };
const MODO_TXT = {
  "AUTO-SIEMPRE": ["Tu modo es AUTOMÁTICO: el juego recarga solo al volver al menú principal y entrar a un modo (nunca en un partido ni en Editar).", "Your mode is AUTOMATIC: the game reloads by itself when you return to the main menu and enter a mode (never in a match or in Edit)."],
  "AUTO-FICHAJES": ["Tu modo es AUTOMÁTICO CON AVISO: el juego recarga solo al volver al menú y entrar a un modo, porque Link le avisa de cada lote nuevo.", "Your mode is AUTO WITH NOTICE: the game reloads by itself at the menu because Link notifies it of each new batch."],
  "ACTIVAR": ["Tu modo es MANUAL: Link deja el traspaso listo, pero el juego lo carga cuando pulses Partido → Datos Actual. en vivo → Activar.", "Your mode is MANUAL: Link leaves the transfer ready, but the game loads it when you press Match → Live update → Activate."],
};

export function Traspasos() {
  const [d, setD] = useState(null);
  const [carpeta, setCarpeta] = useState("");
  const [opt, setOpt] = useState("");
  const cargar = async () => { const r = await accion("traspasos.estado", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { cargar(); const id = setInterval(cargar, 3000); return () => clearInterval(id); }, []);
  const poner = async (campos, ok) => { const r = await accion("traspasos.config", campos); if (r) { setD(r); if (ok) avisar(ok, "ok", 5000); } };
  const hacer = async (que) => { const r = await accion("traspasos.accion", { accion: que }); if (r) setD(r); };
  if (!d) return html`<${Tarjeta} interior="padding:22px 24px"><div class="ayuda">${t("Revisando…", "Checking…")}</div></${Tarjeta}>`;
  const aj = d.ajustes || {};
  const a = d.anfitrion || {};
  const m = d.amigo || {};
  const [punto, et] = (() => {
    const x = ANFITRION[(a.estado || "").split(":")[0]];
    if (x) return [x[0], t(x[1], x[2])];
    if ((a.estado || "").startsWith("no_se_pudo_leer")) return ["warn", t("No pude leer el option file (puede estar guardándose); reintento solo", "I couldn't read the option file (it may be saving); retrying by itself")];
    return ["", a.estado || ""];
  })();
  const pend = a.pendientes || [];
  const lista = m.historial || [];
  const nombre = (c) => c.jugadorNombre || ("#" + c.jugador);
  const modo = d.modoRecarga || "AUTO-SIEMPRE";
  return html`<div class="col" style="gap:14px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("TRASPASOS EN VIVO", "LIVE TRANSFERS")} derecha=${html`<${Chip} tipo=${aj.publicar || aj.recibir ? "acc" : ""}>${aj.publicar || aj.recibir ? t("ENCENDIDO", "ON") : t("APAGADO", "OFF")}</${Chip}>`}/>
      <div class="ayuda" style="font-size:14px">${t("Haces un traspaso en Editar de PES (base normal) y lo GUARDAS: Link lo detecta y lo envía. En el PC de tu amigo, Link lo aplica sobre su propia base y el juego lo recarga solo en un momento seguro. Solo viajan cambios «jugador → equipo», nunca bases. Liga Máster no está incluida.",
        "You make a transfer in PES Edit (normal database) and SAVE: Link detects and sends it. On your friend's PC, Link applies it to their own database and the game reloads it by itself at a safe moment. Only «player → team» changes travel, never databases. Master League is not included.")}</div>
      <div class="caja" style="font-size:13px"><span class="punto warn"></span> ${t("«Al toque» = cuando PES guarda el option file (al guardar / salir de Editar). Mientras no guardes, no hay nada que enviar. BETA: apagado por defecto.", "«Instantly» = when PES saves the option file (on save / leaving Edit). Until you save there is nothing to send. BETA: off by default.")}</div>
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("SOY EL ANFITRIÓN · PUBLICO MIS TRASPASOS", "I'M THE HOST · I PUBLISH MY TRANSFERS")}/>
      <${AjusteSw} titulo=${t("Publicar mis traspasos de Editar", "Publish my Edit transfers")} desc=${t("Link mira tu option file cada pocos segundos (solo lectura). Un jugador que sale de un equipo y entra en otro = traspaso; lo demás se ignora.", "Link checks your option file every few seconds (read-only). A player leaving one team and joining another = transfer; everything else is ignored.")}
        valor=${!!aj.publicar} al=${(v) => poner({ publicar: v }, v ? t("Publicación encendida: la primera lectura solo toma la foto de partida.", "Publishing on: the first read only takes the starting snapshot.") : null)}/>
      <${AjusteSw} titulo=${t("Pedirme confirmación antes de publicar", "Ask me before publishing")} desc=${t("Verás los traspasos detectados y pulsas PUBLICAR. Recomendado al empezar.", "You'll see the detected transfers and press PUBLISH. Recommended at first.")}
        valor=${!!aj.confirmarAntes} al=${(v) => poner({ confirmarAntes: v })}/>
      ${aj.publicar ? html`<div style="display:flex;align-items:center;gap:10px"><span class=${cx("punto", punto)}></span><span style="font-size:14px">${et}</span></div>` : null}
      ${a.optionFile ? html`<div class="ayuda mono" style="font-size:12px">${t("Option file:", "Option file:")} ${a.optionFile}</div>` : null}
      ${pend.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px">
        <b style="font-size:14px">${t(`${pend.length} ${pend.length === 1 ? "traspaso detectado" : "traspasos detectados"}`, `${pend.length} ${pend.length === 1 ? "transfer detected" : "transfers detected"}`)}</b>
        ${a.masivo ? html`<div class="ayuda" style="font-size:13px;color:var(--bad, #ff6b6b)">${t("Son demasiados de golpe. Si cargaste otro option file, descártalos.", "That's too many at once. If you loaded another option file, discard them.")}</div>` : null}
        ${pend.slice(0, 12).map((c) => html`<div class="mono" style="font-size:13px" key=${c.jugador_id + "-" + c.hasta_id}>· ${c.jugador || ("#" + c.jugador_id)}: ${c.desde || c.desde_id} → ${c.hasta || c.hasta_id}</div>`)}
        ${pend.length > 12 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${pend.length - 12} más`, `and ${pend.length - 12} more`)}</div>` : null}
        <div style="display:flex;gap:8px;flex-wrap:wrap">
          <${Boton} tipo="lleno mini" deshabilitado=${!d.transporteListo} titulo=${d.transporteListo ? "" : t("Antes elige la carpeta de envío (abajo)", "Choose the send folder first (below)")}
            al=${async () => { if (await confirmar({ titulo: t("¿Publicar estos traspasos?", "Publish these transfers?"), texto: t("Se enviarán a los Link de tus amigos, que los aplicarán sobre sus bases.", "They will be sent to your friends' Link, which will apply them to their databases."), si: t("Publicar", "Publish") })) hacer("publicar"); }}>${t("PUBLICAR", "PUBLISH")}</${Boton}>
          <${Boton} tipo="suave mini" al=${() => hacer("descartar")}>${t("DESCARTAR", "DISCARD")}</${Boton}>
        </div>
      </div>` : null}
      ${aj.publicar && a.ignorados && (a.ignorados.altas || a.ignorados.bajas || a.ignorados.otros) ? html`<div class="ayuda" style="font-size:12px">${t(`Ignorados en el último guardado (no son traspasos): ${a.ignorados.altas} altas/convocatorias, ${a.ignorados.bajas} bajas, ${a.ignorados.otros} otros.`, `Ignored in the last save (not transfers): ${a.ignorados.altas} additions/call-ups, ${a.ignorados.bajas} releases, ${a.ignorados.otros} other.`)}</div>` : null}
      ${a.publicados ? html`<div class="ayuda" style="font-size:13px">${t(`Lotes publicados en esta sesión: ${a.publicados}`, `Batches published this session: ${a.publicados}`)}</div>` : null}
      <details class="sec-detalle"><summary>${t("Ajustes del anfitrión", "Host settings")}</summary>
        <div class="ayuda" style="font-size:13px">${t("Si Link no encuentra tu option file solo, pega su ruta (el archivo EDIT00000000 o su carpeta «save»):", "If Link can't find your option file, paste its path (the EDIT00000000 file or its «save» folder):")}</div>
        <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${opt} placeholder="C:\\Users\\…\\Documents\\KONAMI\\…\\save" onInput=${(e) => setOpt(e.currentTarget.value)}/></div>
          <${Boton} tipo="lleno mini" deshabilitado=${!opt.trim()} al=${async () => { await poner({ optionFile: opt.trim() }, t("Option file guardado.", "Option file saved.")); setOpt(""); }}>${t("GUARDAR", "SAVE")}</${Boton}></div>
        ${aj.optionFile ? html`<div class="ayuda mono" style="font-size:12px">${aj.optionFile} <button style="background:none;border:0;color:inherit;text-decoration:underline;cursor:pointer;font:inherit" onClick=${() => poner({ optionFile: "" }, t("Vuelve a la búsqueda automática.", "Back to automatic search."))}>${t("quitar", "clear")}</button></div>` : null}
        <div style="margin-top:6px"><${Boton} tipo="suave mini" titulo=${t("La próxima lectura solo toma la foto de partida", "The next read only takes the starting snapshot")} al=${() => hacer("olvidarReferencia")}>${t("OLVIDAR LA FOTO DE PARTIDA", "FORGET STARTING SNAPSHOT")}</${Boton}></div>
      </details>
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("SOY EL AMIGO · RECIBO TRASPASOS", "I'M THE FRIEND · I RECEIVE TRANSFERS")}/>
      <${AjusteSw} titulo=${t("Recibir los traspasos de mi anfitrión", "Receive my host's transfers")} desc=${t("Link los aplica sobre TU base (Phoenix-DB) y avisa a phoenix.lua. Si PES está abierto solo cambia el archivo de Phoenix-DB (nunca el sider.ini); si falta algo, espera a que cierres PES. Se pone al día solo si Link estuvo apagado.", "Link applies them to YOUR database (Phoenix-DB) and notifies phoenix.lua. With PES open it only replaces the Phoenix-DB file (never sider.ini); if something's missing it waits until you close PES. It catches up by itself if Link was off.")}
        valor=${!!aj.recibir} al=${(v) => poner({ recibir: v }, v ? t("Recepción encendida.", "Receiving on.") : null)}/>
      <div class="ayuda" style="font-size:13px">${t(MODO_TXT[modo]?.[0] || "", MODO_TXT[modo]?.[1] || "")}</div>
      ${aj.recibir ? html`<div style="display:flex;align-items:center;gap:10px"><span class=${cx("punto", m.estado === "al_dia" || m.estado === "aplicado" ? "ok" : m.estado === "esperando" ? "warn" : "")}></span>
        <span style="font-size:14px">${m.estado === "al_dia" ? t("Al día", "Up to date") : m.estado === "aplicado" ? t("Último lote aplicado", "Last batch applied") : m.estado === "esperando" ? t("Esperando: ", "Waiting: ") + (m.mensaje || "") : m.estado === "sin_transporte" ? t("Falta elegir la carpeta de recepción (abajo)", "Choose the receive folder (below)") : m.estado}</span></div>` : null}
      ${d.pesAbierto ? html`<div class="ayuda" style="font-size:12px">${t("PES está abierto.", "PES is open.")}</div>` : null}
      ${lista.length ? html`<div style="display:flex;flex-direction:column;gap:8px">
        ${lista.slice(0, 8).map((l) => {
          const [tp, es, en] = ESTADO_LOTE[l.estado] || ["", l.estado, l.estado];
          return html`<div class="caja" key=${l.id} style="display:flex;flex-direction:column;gap:4px">
            <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
              <span class="mono" style="font-size:13px">#${l.seq} · ${l.autor || "?"} · ${(l.creado || "").replace("T", " ").replace("Z", " UTC")}</span>
              <${Chip} tipo=${tp}>${t(es, en)}</${Chip}>
            </div>
            ${l.motivo ? html`<div class="ayuda" style="font-size:12px">${l.motivo}</div>` : null}
            ${(l.cambios || []).slice(0, 6).map((c, i) => { const [ct, ce, cen] = ESTADO_CAMBIO[c.estado] || ["", c.estado, c.estado];
              return html`<div class="mono" style="font-size:12px" key=${i}><span class=${cx("punto", ct)} style="display:inline-block;width:8px;height:8px;margin-right:6px"></span>${nombre(c)} → ${c.hastaNombre || c.hasta} · ${t(ce, cen)}${c.motivo ? " — " + c.motivo : ""}</div>`; })}
            ${(l.cambios || []).length > 6 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${l.cambios.length - 6} más`, `and ${l.cambios.length - 6} more`)}</div>` : null}
          </div>`; })}
        ${lista.length > 8 ? html`<div class="ayuda" style="font-size:12px">${t(`Mostrando los 8 últimos de ${lista.length}.`, `Showing the latest 8 of ${lista.length}.`)}</div>` : null}
      </div>` : html`<div class="ayuda" style="font-size:13px">${t("Aún no ha llegado ningún lote.", "No batch has arrived yet.")}</div>`}
      <div style="display:flex;gap:8px;flex-wrap:wrap">
        <${Boton} tipo="suave mini" deshabilitado=${!m.puedeDeshacer} titulo=${t("Quita el último lote aplicado y deja Phoenix-DB como estaba antes de él", "Removes the last applied batch and leaves Phoenix-DB as it was before it")}
          al=${async () => { if (await confirmar({ titulo: t("¿Deshacer el último lote?", "Undo the last batch?"), texto: t("Link volverá a generar Phoenix-DB sin ese lote y avisará al juego para que recargue.", "Link will rebuild Phoenix-DB without that batch and notify the game to reload."), si: t("Deshacer", "Undo") })) hacer("deshacer"); }}>${t("DESHACER EL ÚLTIMO LOTE", "UNDO LAST BATCH")}</${Boton}>
      </div>
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
      <${Titulo} texto=${t("POR DÓNDE VIAJAN", "HOW THEY TRAVEL")}/>
      <div class="ayuda" style="font-size:14px">${t("Hoy: una carpeta compartida (Google Drive, OneDrive, red local) que ven los dos Link; cada lote es un archivito. Más adelante: la web de Phoenix como centro (falta pegar su prompt en el chat WEB).", "Today: a shared folder (Google Drive, OneDrive, local network) that both Link can see; each batch is a tiny file. Later: the Phoenix website as the hub (its prompt still has to be pasted into the WEB chat).")}</div>
      <div class="ayuda mono" style="font-size:12px">${aj.carpeta ? t("Carpeta actual: ", "Current folder: ") + aj.carpeta : t("Sin carpeta elegida", "No folder chosen")}</div>
      <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${carpeta} placeholder="D:\\Drive\\PhoenixTraspasos" onInput=${(e) => setCarpeta(e.currentTarget.value)}/></div>
        <${Boton} tipo="lleno mini" deshabilitado=${!carpeta.trim()} al=${async () => { await poner({ carpeta: carpeta.trim(), transporte: "carpeta" }, t("Carpeta guardada.", "Folder saved.")); setCarpeta(""); }}>${t("GUARDAR", "SAVE")}</${Boton}></div>
      <div class="ayuda" style="font-size:12px">${t("Ruta completa. Los dos (anfitrión y amigo) deben apuntar a la misma carpeta compartida.", "Full path. Both (host and friend) must point to the same shared folder.")}</div>
      ${d.mensaje ? html`<div class="caja" style="font-size:13px">${d.mensaje}</div>` : null}
    </${Tarjeta}>
  </div>`;
}
