// =============================================================================
//  MEDIDOR DE RETRASO del mando por jugador (0.2.16). El motor manda cada
//  segundo `m.retraso` (phoenix/web/RetrasoWeb.cpp): ping, codificar y
//  decodificar de Parsec, paquetes reenviados y cómo LLEGAN los mensajes del
//  mando (intervalo medio, p95, jitter, saltos, ritmo). Aquí se pinta:
//  - BloqueRetraso: dentro de cada tarjeta de Antes › Conexión.
//  - LeyendaRetraso: cómo se calcula y qué NO se puede medir.
//  - IndicadorRetraso: punto pequeño junto a cada puesto en Jugando › Puestos.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { claseSemaforo, cx } from "../ui.js";

export const UMBRALES_RETRASO = { verde: 80, ambar: 130, perdidaAmbar: 2, perdidaRojo: 5, saltosAmbar: 6, saltosRojo: 20 };

export function retrasoDe(m, parsecId) {
  return (m.retraso || []).find((r) => r.parsecId === parsecId) || null;
}

const MOTIVOS_EN = { "ping alto": "high ping", "mando irregular": "irregular pad", "vídeo lento (codificar/decodificar)": "slow video (encode/decode)", "pierde paquetes": "losing packets", "mando a saltos": "pad stutters" };
export const motivoRetraso = (mo) => (t(mo, MOTIVOS_EN[mo] || mo));

const TEXTO_SEMAFORO = { verde: ["VA BIEN", "GOOD"], ambar: ["SE NOTA", "NOTICEABLE"], rojo: ["CON RETRASO", "LAGGING"], sin_datos: ["SIN DATOS", "NO DATA"] };
const textoSemaforo = (s) => { const x = TEXTO_SEMAFORO[s] || TEXTO_SEMAFORO.sin_datos; return t(x[0], x[1]); };
const ms = (v) => (v == null || v < 0 ? "—" : `${v}`);

/** Texto corto para el globo del indicador. */
function resumenTexto(r) {
  if (!r) return t("Sin datos de retraso todavía.", "No delay data yet.");
  const partes = [`${t("Retraso estimado", "Estimated delay")}: ${r.total >= 0 ? "~" + r.total + " ms" : "—"} (${textoSemaforo(r.semaforo).toLowerCase()})`];
  if (r.motivo) partes.push(`${t("Causa", "Cause")}: ${motivoRetraso(r.motivo)}`);
  partes.push(`Ping ${ms(r.ping)} ms · ${t("vídeo", "video")} ${Math.round((r.codificar || 0) + (r.decodificar || 0))} ms · ${t("pérdida", "loss")} ${r.perdida ?? 0} %`);
  if (r.mando) partes.push(`${t("Mando", "Pad")}: jitter ${ms(r.mando.jitter)} ms · ${r.mando.saltos} ${t("saltos/min", "stutters/min")} · ${r.mando.ritmo}/s`);
  else partes.push(t("Mando: aún no lo ha movido.", "Pad: not moved yet."));
  partes.push(t("No incluye el retraso de su mando ni de su pantalla.", "Does not include their own pad or screen delay."));
  return partes.join("\n");
}

/** Jugando › Puestos: punto + «~84» junto al ping (los detalles, en el globo). */
export function IndicadorRetraso({ r }) {
  if (!r || r.semaforo === "sin_datos") return null;
  const color = r.semaforo === "verde" ? "var(--ok)" : r.semaforo === "ambar" ? "var(--warn)" : "var(--bad)";
  return html` · <span class="semaforo" style=${`font-size:12px;color:${color};cursor:help;white-space:nowrap`} title=${resumenTexto(r)}>
    <span class=${cx("punto", claseSemaforo(r.semaforo))} style="width:7px;height:7px"></span>${r.total >= 0 ? "~" + r.total : "!"}</span>`;
}

/** Antes › Conexión: bloque dentro de la tarjeta de cada invitado. */
export function BloqueRetraso({ r }) {
  if (!r) return null;
  const md = r.mando;
  const colorS = r.semaforo === "verde" ? "var(--ok)" : r.semaforo === "ambar" ? "var(--warn)" : r.semaforo === "rojo" ? "var(--bad)" : "var(--mut)";
  const celda = (tit, val, unidad, titulo) => html`<div title=${titulo}><div class="mut">${tit}</div><b>${val}</b>${unidad && val !== "—" ? html`<span class="mut"> ${unidad}</span>` : null}</div>`;
  return html`<div style="margin-top:14px;padding-top:12px;border-top:1px solid var(--tenue)">
    <div style="display:flex;justify-content:space-between;align-items:baseline;gap:10px;flex-wrap:wrap">
      <span class="lab">${t("RETRASO DEL MANDO", "PAD DELAY")}</span>
      <span class="semaforo" style=${`color:${colorS}`}><span class=${cx("punto", claseSemaforo(r.semaforo))}></span>
        <span class="disp" style="font-size:20px;font-weight:700">${r.total >= 0 ? "~" + r.total : "—"}</span><span class="mut"> ms</span>
        · ${textoSemaforo(r.semaforo)}${r.motivo && r.semaforo !== "verde" ? " · " + motivoRetraso(r.motivo) : ""}</span>
    </div>
    <div class="rej4 mono" style="gap:8px;font-size:12px;margin-top:10px">
      ${celda(t("VÍDEO", "VIDEO"), Math.round((r.codificar || 0) + (r.decodificar || 0)), "ms", t(`Codificar en tu PC ${r.codificar} ms + decodificar en la suya ${r.decodificar} ms`, `Encode on your PC ${r.codificar} ms + decode on theirs ${r.decodificar} ms`))}
      ${celda(t("PÉRDIDA", "LOSS"), r.perdida ?? 0, "%", t("Paquetes de vídeo reenviados (últimos ~10 s)", "Video packets resent (last ~10 s)"))}
      ${celda(t("JITTER MANDO", "PAD JITTER"), md && md.muestras >= 8 ? md.jitter : "—", "ms", t("Cuánto varía el tiempo entre mensajes seguidos de su mando mientras lo mueve", "How much the time between consecutive pad messages varies while moving"))}
      ${celda(t("SALTOS/MIN", "STUTTERS/MIN"), md ? md.saltos : "—", "", t("Huecos seguidos de varios mensajes de golpe: la red retuvo su mando y lo soltó junto", "Gaps followed by a burst of messages: the network held their pad input and released it at once"))}
    </div>
    <div class="mono mut" style="font-size:11px;margin-top:8px">
      ${md && md.muestras >= 8
        ? t(`Llegada del mando: cada ${md.media} ms (p95 ${md.p95}) · ${md.ritmo} mensajes/s`, `Pad arrival: every ${md.media} ms (p95 ${md.p95}) · ${md.ritmo} msgs/s`)
        : t("Mando: se medirá cuando lo mueva.", "Pad: measured once they move it.")}
      ${r.cola > 2 ? t(` · ${r.cola} fotogramas en cola en su PC`, ` · ${r.cola} frames queued on their PC`) : ""}
    </div>
  </div>`;
}

/** Antes › Conexión: cómo se calcula (plegable). */
export function LeyendaRetraso() {
  const [abierta, setAbierta] = useState(false);
  const u = UMBRALES_RETRASO;
  return html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
    <div style="display:flex;gap:18px;flex-wrap:wrap;align-items:center">
      <span class="lab">${t("RETRASO DEL MANDO", "PAD DELAY")}</span>
      <span class="semaforo"><span class="punto ok"></span>≤ ${u.verde} ms</span>
      <span class="semaforo"><span class="punto warn"></span>≤ ${u.ambar} ms · ${t(`pérdida ≥ ${u.perdidaAmbar} % · ≥ ${u.saltosAmbar} saltos/min`, `loss ≥ ${u.perdidaAmbar} % · ≥ ${u.saltosAmbar} stutters/min`)}</span>
      <span class="semaforo"><span class="punto bad"></span>${t(`más · pérdida ≥ ${u.perdidaRojo} % · ≥ ${u.saltosRojo} saltos/min`, `more · loss ≥ ${u.perdidaRojo} % · ≥ ${u.saltosRojo} stutters/min`)}</span>
      <button class="enlace-mas" style="margin-left:auto;font-size:12px" onClick=${() => setAbierta(!abierta)}>${abierta ? t("Ocultar", "Hide") : t("¿Cómo se calcula?", "How is it calculated?")}</button>
    </div>
    ${abierta ? html`<div class="ayuda" style="display:flex;flex-direction:column;gap:6px;font-size:13px">
      <div>${t("Retraso estimado = ping (ida y vuelta) + vídeo (tu PC lo codifica, la suya lo decodifica) + jitter de su mando. Manda la peor regla: el total, la pérdida de paquetes o los saltos.", "Estimated delay = ping (round trip) + video (your PC encodes, theirs decodes) + their pad jitter. The worst rule wins: total, packet loss or stutters.")}</div>
      <div>${t("Jitter y saltos salen de cómo LLEGAN a tu PC los mensajes de su mando mientras lo mueve. Un «salto» es un hueco seguido de varios mensajes de golpe: su conexión retuvo el mando y lo soltó junto (una pausa normal del jugador no cuenta).", "Jitter and stutters come from how their pad messages ARRIVE at your PC while moving. A «stutter» is a gap followed by a burst: their connection held the input and released it at once (a normal pause doesn't count).")}</div>
      <div>${t("Durante un partido, si alguien sigue en rojo 10 s seguidos, el bot avisa en el chat (como mucho 1 vez cada 3 min por jugador).", "During a match, if someone stays red for 10 s, the bot warns in chat (at most once every 3 min per player).")}</div>
      <div><b>${t("Lo que NO se puede medir desde aquí:", "What can NOT be measured from here:")}</b> ${t("el retraso dentro de su casa: su mando (sobre todo por Bluetooth), su PC y su pantalla o tele (modo juego apagado = +50 ms o más). Si todo sale verde y aún nota lag, el problema suele estar ahí.", "the delay inside their home: their pad (especially Bluetooth), their PC and their screen or TV (game mode off = +50 ms or more). If everything is green and they still feel lag, that's usually where it is.")}</div>
    </div>` : null}
  </div>`;
}
