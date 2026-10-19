// =============================================================================
//  «MI PUESTO AL JUGAR»: dónde se sienta el anfitrión cuando pulsa JUGAR.
//  Dinámico: un puesto fijo (1, 2…) o «el primero del Equipo A / del Equipo B», que
//  se adapta a la formación del momento. Si el puesto está ocupado, ese amigo
//  pasa al primer puesto libre de su mismo equipo (o a cualquiera libre): nadie
//  se queda sin mando. La preferencia la guarda el motor (phoenix-ui.json), que
//  además sienta solo al anfitrión al abrir la sala y cuando pulsa A/B/X/Y en su
//  mando sin puesto (solo si hay uno libre: ahí nunca saca a nadie).
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, irA } from "../tienda.js";
import { Boton, Segmentos, Stepper, AjusteSw, dos } from "../ui.js";

const CLAVE = "phoenix.miPuesto";
const POR_DEFECTO = { modo: "fijo", numero: 1 };   // modo: fijo | a | b  (los viejos «local» y «visitante» se leen como a y b)

/** «local» → a, «visitante» → b (versiones anteriores a 0.2.31). */
function modoActual(modo) {
  return modo === "local" ? "a" : modo === "visitante" ? "b" : modo;
}
function leerLocal() {
  try { const p = JSON.parse(localStorage.getItem(CLAVE) || "null"); if (p && p.modo) return { ...p, modo: modoActual(p.modo) }; } catch { /* sin almacenamiento */ }
  return null;
}
/** La del motor (si la publica); si no, la que quedó en este PC (versiones ≤ 0.2.15). */
export function leerPreferencia(m) {
  const motor = m?.mandos?.miPuesto;
  const pref = { ...POR_DEFECTO, alAbrir: true, pulsando: true, ...(motor || leerLocal() || {}) };
  return { ...pref, modo: modoActual(pref.modo) };
}
function guardarPreferencia(p) {
  try { localStorage.setItem(CLAVE, JSON.stringify(p)); } catch { /* nada */ }
  accion("mandos.miPuesto", p, { silencioso: true });
}

/** Puesto (1..8) que toca según la preferencia y la formación actual; 0 si no hay. */
export function puestoDestino(m, pref = leerPreferencia(m)) {
  const lista = m.mandos?.lista || [];
  if (!lista.length) return 0;
  if (pref.modo === "fijo") return Math.min(Math.max(1, pref.numero || 1), lista.length);
  const delEquipo = lista.filter((p) => p.equipo === pref.modo);
  return delEquipo.length ? delEquipo[0].n : 0;
}

/** Sienta al anfitrión según su preferencia, moviendo a quien ocupe ese puesto a uno libre. */
export async function jugarYo(m) {
  const lista = m.mandos?.lista || [];
  const destino = puestoDestino(m);
  if (!destino) { avisar(t("No hay puestos en ese equipo: añade uno en Mandos › Puestos.", "No seats on that team: add one in Pads › Seats."), "warn", 8000, { texto: t("IR A PUESTOS", "GO TO SEATS"), al: () => irA("mandos", "puestos") }); return; }
  if (m.mandos.host === destino) { avisar(t(`Ya estás en el puesto ${dos(destino)}.`, `You're already on seat ${dos(destino)}.`), "ok"); return; }
  const p = lista[destino - 1];
  const esMio = (x) => m.mandos.host === x.n || (x.parsecId && x.parsecId === m.sala?.hostId);
  if (p && p.ocupado && !esMio(p)) {
    // Primer libre de su mismo equipo; si no hay, uno sin equipo; y solo en último caso el otro equipo (avisando)
    const libres = lista.filter((x) => !x.ocupado && x.n !== destino);
    const mismo = libres.find((x) => x.equipo === p.equipo);
    const fuera = libres.find((x) => x.equipo !== "a" && x.equipo !== "b");
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

/** Tarjeta de preferencia (va en Mandos › Tu mando). */
export function PreferenciaPuesto({ m }) {
  const [local, setLocal] = useState(null);   // lo que acabas de tocar, hasta que el motor lo publique
  const pref = { ...leerPreferencia(m), ...(local || {}) };
  const poner = (p) => { const n = { ...pref, ...p }; setLocal(n); guardarPreferencia(n); };
  // Antes se guardaba solo en este PC: se pasa al motor una vez
  useEffect(() => {
    const viejo = leerLocal();
    if (viejo && m?.mandos?.miPuesto && (viejo.modo !== m.mandos.miPuesto.modo || viejo.numero !== m.mandos.miPuesto.numero) && !viejo.migrado) {
      guardarPreferencia({ ...m.mandos.miPuesto, modo: viejo.modo, numero: viejo.numero, migrado: true });
    }
  }, []);
  const sinMando = !m?.mandos?.miMando;
  const destino = puestoDestino(m, pref);
  const total = (m.mandos?.lista || []).length || 8;
  return html`<div style="display:flex;flex-direction:column;gap:10px">
    <div class="lab acc">${t("MI PUESTO AL JUGAR", "MY SEAT WHEN PLAYING")}</div>
    <div class="ayuda" style="font-size:13px">${t("Dónde te sienta el botón JUGAR. Si está ocupado, ese amigo pasa a un puesto libre de su equipo.", "Where the PLAY button seats you. If taken, that friend moves to a free seat on their team.")}</div>
    <div class="ayuda" style="font-size:13px">${t("Equipo A y B son equipos de Link, no el local y la visita del juego. ¿Quieres ser siempre el 01 aunque juegues del otro lado? Elige PUESTO FIJO 1 y en Mandos › Puestos pasa el puesto 01 al equipo que toque: tu número de mando no cambia.", "Team A and B are Link's teams, not the game's home and away. Want to always be pad 01 even when you play the other side? Pick FIXED SEAT 1 and, in Pads › Seats, move seat 01 to the team you need: your pad number doesn't change.")}</div>
    <${Segmentos} valor=${pref.modo} al=${(v) => poner({ modo: v })} opciones=${[
      { valor: "fijo", texto: t("PUESTO FIJO", "FIXED SEAT") },
      { valor: "a", texto: t("1.º DE A", "1ST OF A") },
      { valor: "b", texto: t("1.º DE B", "1ST OF B") }]}/>
    ${pref.modo === "fijo" ? html`<div style="display:flex;align-items:center;gap:10px">
      <span class="ayuda">${t("Puesto", "Seat")}</span>
      <${Stepper} valor=${pref.numero} min=${1} max=${total} al=${(v) => poner({ numero: v })}/></div>` : null}
    <div class="mono mut" style="font-size:12px">${destino ? t(`Ahora mismo: puesto ${dos(destino)}`, `Right now: seat ${dos(destino)}`) : t("Ahora mismo: ese equipo no tiene puestos", "Right now: that team has no seats")}</div>
    <${AjusteSw} titulo=${t("Empezar sentado", "Start seated")} valor=${pref.alAbrir !== false} al=${(v) => poner({ alAbrir: v })}
      desc=${t("Al abrir la sala ya estás en tu puesto. Si no quieres jugar, sácate a «Mirando».", "When the room opens you are already seated. Not playing? Move yourself to «Watching».")}/>
    <${AjusteSw} titulo=${t("Entrar pulsando tu mando", "Join by pressing your pad")} valor=${pref.pulsando !== false} al=${(v) => poner({ pulsando: v })}
      desc=${t("Si no estás en un puesto, pulsa A, B, X o Y (✕ ○ □ △) y entras al libre que toque. Nunca saca a nadie: si está todo lleno, no pasa nada.", "Not seated? Press A, B, X or Y and you take the right free seat. Never kicks anyone out.")}/>
    <${AjusteSw} titulo=${t("Contarme como jugador en la web (automático)", "Count me as a player on the web (automatic)")} valor=${pref.cuenta !== false} al=${(v) => poner({ cuenta: v })}
      desc=${t("Sentado, cuentas siempre. Sin puesto, Link te apunta solo si ve que tocas tu mando justo antes del partido (últimos 2 min), en tu lado habitual (el de tu puesto de arriba). Si solo miras y no tocas el mando, no sales.", "Seated, you always count. Not seated, Link lists you only if it sees you touching your pad just before the match (last 2 min), on your usual side. If you only watch, you are not listed.")}/>
    ${sinMando ? html`<div class="ayuda" style="font-size:13px;color:var(--warn)">${t("Las dos funcionan con tu mando elegido arriba («USAR ESTE»). Con teclado, usa el botón JUGAR.", "Both need your pad chosen above. With keyboard, use the PLAY button.")}</div>` : null}
  </div>`;
}

/** Botón JUGAR (fila del anfitrión en «Mirando»). */
export function BotonJugar({ m }) {
  const destino = puestoDestino(m);
  return html`<${Boton} tipo="mini lleno" deshabilitado=${!!m.mandos?.esclavo}
    titulo=${t(`Te sienta en el puesto ${dos(destino)} (cámbialo en TU MANDO)`, `Seats you on ${dos(destino)} (change it in YOUR PAD)`)}
    al=${() => jugarYo(m)}>${t("JUGAR", "PLAY")}${destino ? " · " + dos(destino) : ""}</${Boton}>`;
}
