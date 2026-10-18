// Aviso arriba «Hay una versión nueva X · ACTUALIZAR» (auto-actualización, phoenix/link/Actualizacion.h).
// El motor baja y comprueba la versión (firma y sha256) y, sin sala abierta ni partido, cierra Link para instalarla:
// PhoenixActualizador la instala, vuelve a abrir Link y, si no arranca, deja la anterior.
import { html, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Icono } from "../ui.js";

let resultadoMostrado = "";

async function actualizar(a) {
  const si = await confirmar({
    titulo: t(`Actualizar a la ${a.version}`, `Update to ${a.version}`),
    texto: t("Phoenix Link se baja la versión nueva, se cierra unos segundos y vuelve a abrirse solo. Tus ajustes, tu cuenta y tus fichajes no se tocan. Si Windows te pide permiso, pulsa «Sí». Si la versión nueva no arranca, vuelve sola la que tienes ahora.",
      "Phoenix Link downloads the new version, closes for a few seconds and reopens by itself. Your settings, account and transfers are not touched. If Windows asks for permission, click «Yes». If the new version does not start, your current one comes back by itself."),
    si: t("ACTUALIZAR", "UPDATE"), no: t("Ahora no", "Not now"),
  });
  if (!si) return;
  const r = await accion("app.actualizar");
  if (r?.esperaSala) avisar(t("Se está bajando. Cuando cierres la sala, Link se actualiza solo.", "Downloading. When you close the room, Link updates by itself."), "info", 7000);
}

export function BandaActualizacion({ s }) {
  const a = s.motor?.actualizacion;
  // Lo que pasó en la última actualización: un aviso, una sola vez
  useEffect(() => {
    if (!a?.resultado || a.resultado === resultadoMostrado) return;
    resultadoMostrado = a.resultado;
    avisar(a.resultado, a.resultadoTipo === "ok" ? "ok" : "bad", a.resultadoTipo === "ok" ? 6000 : 0);
    accion("app.actualizacionVisto", {}, { silencioso: true });
  }, [a?.resultado]);
  if (!a || a.estado === "nada" || !a.version) return null;

  const pct = a.total > 0 ? Math.min(100, Math.round((a.bajado / a.total) * 100)) : 0;
  let detalle = a.notas || "";
  let boton = null;
  if (a.estado === "disponible") boton = html`<button class="btn lleno" onClick=${() => actualizar(a)}>${t("ACTUALIZAR", "UPDATE")}</button>`;
  else if (a.estado === "error") { detalle = a.mensaje; boton = html`<button class="btn suave" onClick=${() => actualizar(a)}>${t("REINTENTAR", "RETRY")}</button>`; }
  else if (a.estado === "descargando") detalle = t(`Bajando y comprobando… ${pct} %`, `Downloading and checking… ${pct} %`);
  else if (a.estado === "lista") detalle = a.mensaje || t("Lista. Link se cierra y vuelve a abrirse solo.", "Ready. Link closes and reopens by itself.");
  else if (a.estado === "instalando") detalle = t("Instalando… Link se cierra y vuelve a abrirse solo. Si Windows pide permiso, pulsa «Sí».", "Installing… Link closes and reopens by itself. If Windows asks, click «Yes».");

  return html`<div class="banda-cedida" role="status" style=${a.obligatoria ? "border-color:var(--bad)" : ""}>
    <span class="acc"><${Icono} n=${a.estado === "error" ? "alerta" : "rayo"} t=${22}/></span>
    <div style="flex:1;min-width:0">
      <div class="disp titulo">${t(`HAY UNA VERSIÓN NUEVA · ${a.version}`, `NEW VERSION · ${a.version}`)}${a.obligatoria ? html` <span style="color:var(--bad)">${t("· OBLIGATORIA", "· REQUIRED")}</span>` : null}</div>
      ${a.obligatoria && a.estado === "disponible" ? html`<div class="ayuda" style="color:var(--bad)">${t("Es obligatoria: sin ella no vas a poder hacer fichajes ni jugar con quienes ya la tienen.", "It is required: without it you cannot trade or play with those who already have it.")}</div>` : null}
      ${detalle ? html`<div class="ayuda" style="white-space:pre-line">${detalle}</div>` : null}
      ${a.estado === "descargando" ? html`<div style="height:6px;border-radius:3px;background:var(--acc-suave);margin-top:6px"><div style=${`height:6px;border-radius:3px;background:var(--acc);width:${pct}%`}></div></div>` : null}
    </div>
    ${boton}
  </div>`;
}
