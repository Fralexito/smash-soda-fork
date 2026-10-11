// =============================================================================
//  «MI PUESTO AL JUGAR»: dónde se sienta el anfitrión cuando pulsa JUGAR.
//  Dinámico: un puesto fijo (1, 2…) o «el primero de Local / de Visita», que
//  se adapta a la formación del momento. Si el puesto está ocupado, ese amigo
//  pasa al primer puesto libre de su mismo equipo (o a cualquiera libre): nadie
//  se queda sin mando. La preferencia se guarda en este PC.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton, Segmentos, Stepper, dos } from "../ui.js";

const CLAVE = "phoenix.miPuesto";
const POR_DEFECTO = { modo: "fijo", numero: 1 };   // modo: fijo | local | visitante

export function leerPreferencia() {
  try { const p = JSON.parse(localStorage.getItem(CLAVE) || "null"); if (p && p.modo) return { ...POR_DEFECTO, ...p }; } catch { /* sin almacenamiento */ }
  return { ...POR_DEFECTO };
}
function guardarPreferencia(p) { try { localStorage.setItem(CLAVE, JSON.stringify(p)); } catch { /* nada */ } }

/** Puesto (1..8) que toca según la preferencia y la formación actual; 0 si no hay. */
export function puestoDestino(m, pref = leerPreferencia()) {
  const lista = m.mandos?.lista || [];
  if (!lista.length) return 0;
  if (pref.modo === "fijo") return Math.min(Math.max(1, pref.numero || 1), lista.length);
  const delEquipo = lista.filter((p) => p.equipo === pref.modo);
  return delEquipo.length ? delEquipo[0].n : 0;
}

/** Sienta al anfitrión según su preferencia, moviendo a quien ocupe ese puesto a uno libre. */
export async function jugarYo(m) {
  if (!m.sala?.abierta) { avisar(t("Abre la sala para tomar un puesto.", "Open the room to take a seat."), "warn"); return; }
  const lista = m.mandos?.lista || [];
  const destino = puestoDestino(m);
  if (!destino) { avisar(t("No hay puestos en ese equipo: añade uno en Mandos › Puestos.", "No seats on that team: add one in Pads › Seats."), "warn"); return; }
  if (m.mandos.host === destino) { avisar(t(`Ya estás en el puesto ${dos(destino)}.`, `You're already on seat ${dos(destino)}.`), "ok"); return; }
  const p = lista[destino - 1];
  const esMio = (x) => m.mandos.host === x.n || (x.parsecId && x.parsecId === m.sala?.hostId);
  if (p && p.ocupado && !esMio(p)) {
    // Primer libre de su mismo equipo; si no hay, uno sin equipo; y solo en último caso el otro equipo (avisando)
    const libres = lista.filter((x) => !x.ocupado && x.n !== destino);
    const mismo = libres.find((x) => x.equipo === p.equipo);
    const fuera = libres.find((x) => x.equipo !== "local" && x.equipo !== "visitante");
    const libre = mismo || fuera || libres[0];
    if (!libre) { avisar(t(`El puesto ${dos(destino)} está ocupado y no queda ninguno libre: quita a alguien primero.`, `Seat ${dos(destino)} is taken and none is free: remove someone first.`), "warn"); return; }
    if (!(await accion("mandos.intercambiar", { a: destino - 1, b: libre.n - 1 }, { silencioso: true }))) return;
    const cambioEquipo = !mismo && !fuera;
    avisar(cambioEquipo
      ? t(`${p.jugador || "Tu amigo"} pasó al puesto ${dos(libre.n)}, que es del OTRO equipo: revisa los lados.`, `${p.jugador || "Your friend"} moved to seat ${dos(libre.n)}, on the OTHER team: check sides.`)
      : t(`${p.jugador || "Tu amigo"} pasó al puesto ${dos(libre.n)}.`, `${p.jugador || "Your friend"} moved to seat ${dos(libre.n)}.`), cambioEquipo ? "warn" : "ok");
  }
  await accion("mandos.tomar", { numero: destino });
}

/** Tarjeta de preferencia (va en Mandos › TU MANDO). */
export function PreferenciaPuesto({ m }) {
  const [pref, setPref] = useState(leerPreferencia);
  const poner = (p) => { const n = { ...pref, ...p }; setPref(n); guardarPreferencia(n); };
  const destino = puestoDestino(m, pref);
  const total = (m.mandos?.lista || []).length || 8;
  return html`<div style="display:flex;flex-direction:column;gap:10px">
    <div class="lab acc">${t("MI PUESTO AL JUGAR", "MY SEAT WHEN PLAYING")}</div>
    <div class="ayuda" style="font-size:13px">${t("Dónde te sienta el botón JUGAR. Si está ocupado, ese amigo pasa a un puesto libre de su equipo.", "Where the PLAY button seats you. If taken, that friend moves to a free seat on their team.")}</div>
    <${Segmentos} valor=${pref.modo} al=${(v) => poner({ modo: v })} opciones=${[
      { valor: "fijo", texto: t("PUESTO FIJO", "FIXED SEAT") },
      { valor: "local", texto: t("1.º DE LOCAL", "1ST HOME") },
      { valor: "visitante", texto: t("1.º DE VISITA", "1ST AWAY") }]}/>
    ${pref.modo === "fijo" ? html`<div style="display:flex;align-items:center;gap:10px">
      <span class="ayuda">${t("Puesto", "Seat")}</span>
      <${Stepper} valor=${pref.numero} min=${1} max=${total} al=${(v) => poner({ numero: v })}/></div>` : null}
    <div class="mono mut" style="font-size:12px">${destino ? t(`Ahora mismo: puesto ${dos(destino)}`, `Right now: seat ${dos(destino)}`) : t("Ahora mismo: ese equipo no tiene puestos", "Right now: that team has no seats")}</div>
  </div>`;
}

/** Botón JUGAR (fila del anfitrión en «Mirando»). */
export function BotonJugar({ m }) {
  const destino = puestoDestino(m);
  return html`<${Boton} tipo="mini lleno" deshabilitado=${!m.sala?.abierta || !!m.mandos?.esclavo}
    titulo=${m.sala?.abierta ? t(`Te sienta en el puesto ${dos(destino)} (cámbialo en TU MANDO)`, `Seats you on ${dos(destino)} (change it in YOUR PAD)`) : t("Abre la sala para jugar", "Open the room to play")}
    al=${() => jugarYo(m)}>${t("JUGAR", "PLAY")}${destino ? " · " + dos(destino) : ""}</${Boton}>`;
}
