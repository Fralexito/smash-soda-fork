// =============================================================================
//  Modo cedido: el anfitrión cede su PC a un invitado (teclado y ratón solo en
//  Link y PES). Botón en la ficha de la persona (jugador o espectador), diálogo
//  para ceder y banda con ABRIR PES y DEVOLVER LA PC. Sin PIN: devolver la PC
//  siempre le quita el control al invitado. Guía: docs/MODO-CEDIDO.md.
//  El motor bloquea por su cuenta lo que aquí se oculta (error PC_CEDIDA).
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar, avisar, confirmar, irA } from "../tienda.js";
import { Icono, Boton, cx } from "../ui.js";

// Lo que se ve mientras la PC está cedida (lo demás se oculta y el motor lo rechaza)
export const SECCIONES_CEDIDO = ["sala", "partido", "mandos"];
export const PESTANAS_CEDIDO = { sala: ["resumen", "red", "actividad"], partido: ["vivo", "historial"], mandos: ["puestos", "bloqueo", "vivo"] };
export const estaCedida = (s) => !!s?.motor?.cedido?.activo;

// ---- Botón en la ficha de una persona ------------------------------------------------
export function BotonCeder({ g, m }) {
  if (!g || g.host || !m?.sala?.abierta || m?.cedido?.activo) return null;
  return html`<${Boton} tipo="acc" titulo=${t("Dale tu PC: solo podrá usar Phoenix Link y PES con teclado y ratón.", "Hand over your PC: only Phoenix Link and PES with keyboard and mouse.")}
    al=${() => cambiar({ ceder: { parsecId: g.parsecId, nombre: g.nombre } })}><${Icono} n="candado" t=${15}/>${t("CEDER MI PC", "HAND OVER PC")}</${Boton}>`;
}

function Velo({ children, alCerrar }) {
  return html`<div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
    <div class="dialogo" role="dialog" aria-modal="true" style="width:min(520px,100%)">
      <div class="cut"><div class="in" style="padding:26px 28px">${children}</div></div>
    </div></div>`;
}

// ---- Diálogo para ceder -----------------------------------------------------------------
export function DialogoCeder({ s }) {
  const d = s.ceder;
  const c = s.motor?.cedido || {};
  const [enviando, setEnviando] = useState(false);
  if (!d) return null;
  const cerrar = () => cambiar({ ceder: null });
  const ceder = async () => {
    if (enviando) return;
    setEnviando(true);
    const r = await accion("cedido.empezar", { parsecId: d.parsecId });
    setEnviando(false);
    if (r) { cerrar(); avisar(t(`PC cedida a ${d.nombre}.`, `PC handed over to ${d.nombre}.`), "ok"); }
  };
  return html`<${Velo} alCerrar=${cerrar}>
    <h2>${t(`¿Ceder tu PC a ${d.nombre}?`, `Hand over your PC to ${d.nombre}?`)}</h2>
    <ul class="ayuda lista-cedido">
      <li>${t(`${d.nombre} tendrá teclado y ratón, juegue o mire. Los demás siguen con su mando.`, `${d.nombre} gets keyboard and mouse, playing or watching. Everyone else keeps their pad.`)}</li>
      <li>${t("Solo podrá usar Phoenix Link (Sala, Partido y Mandos) y PES. Windows, Ajustes, Sync y tu cuenta quedan cerrados.", "Only Phoenix Link (Room, Match, Pads) and PES. Windows, Settings, Sync and your account stay locked.")}</li>
      <li>${t("Tú sigues usando tu teclado y tu ratón cuando quieras.", "You can still use your own keyboard and mouse anytime.")}</li>
      <li>${t("Para recuperarla: DEVOLVER LA PC, o Ctrl+Alt+F12 en tu teclado. Si sale de la sala, vuelve sola.", "To take it back: GIVE BACK THE PC, or Ctrl+Alt+F12 on your keyboard. If they leave, it comes back on its own.")}</li>
      <li>${c.juego
        ? t(`El botón ABRIR PES abre «${c.juego}» (Sala › Juegos).`, `OPEN PES launches “${c.juego}” (Room › Games).`)
        : html`<span style="color:var(--warn)">${t("No hay juego elegido en Sala › Juegos: ABRIR PES no podrá abrirlo (solo traerlo al frente si ya está abierto).", "No game picked in Room › Games: OPEN PES can only bring it to front.")}</span>
          <button class="enlace-mas" style="margin-left:6px" onClick=${() => { cerrar(); irA("sala", "juegos"); }}>${t("ELEGIR EL JUEGO ›", "PICK THE GAME ›")}</button>`}</li>
    </ul>
    <div class="botones">
      <button class="btn suave" onClick=${cerrar}>${t("Cancelar", "Cancel")}</button>
      <button class="btn lleno" disabled=${enviando} onClick=${ceder}>${t("CEDER MI PC", "HAND OVER")}</button>
    </div>
  </${Velo}>`;
}

/** Devolver la PC al anfitrión (con confirmación, para no pulsarlo sin querer). */
export async function devolverPc(m) {
  const anfitrion = m?.sala?.hostNombre || t("el anfitrión", "the host");
  const nombre = m?.cedido?.nombre || "";
  if (!(await confirmar({
    titulo: t(`¿Devolver la PC a ${anfitrion}?`, `Give the PC back to ${anfitrion}?`),
    texto: t(`${nombre} perderá el teclado y el ratón, y Link vuelve a verse completo.`, `${nombre} loses keyboard and mouse, and Link is complete again.`),
    si: t("Devolver", "Give back"),
  }))) return;
  if (await accion("cedido.salir", {})) avisar(t("PC devuelta.", "PC given back."), "ok");
}

// ---- Banda del modo cedido (arriba del contenido) ------------------------------------------------
function minutos(desde) {
  if (!desde) return "";
  const m = Math.max(0, Math.floor((Date.now() - desde) / 60000));
  return m < 60 ? `${m} min` : `${Math.floor(m / 60)} h ${m % 60} min`;
}

export function BandaCedido({ s }) {
  const c = s.motor?.cedido;
  if (!c?.activo) return null;
  const abrir = () => accion("cedido.abrirPes", {}, { ok: c.pesAbierto ? t("PES al frente.", "PES to front.") : t("Abriendo PES…", "Opening PES…") });
  return html`<div class="banda-cedida" role="status">
    <span class="acc"><${Icono} n="candado" t=${22}/></span>
    <div style="flex:1;min-width:0">
      <div class="disp titulo">${t(`PC CEDIDA A ${c.nombre}`, `PC HANDED TO ${c.nombre}`)}</div>
      <div class="ayuda">${t("Solo Phoenix Link y PES, con teclado y ratón. El anfitrión puede usar su teclado y ratón a la vez.", "Only Phoenix Link and PES, keyboard and mouse. The host can still use theirs.")} ${minutos(c.desde) ? "· " + minutos(c.desde) : ""}</div>
    </div>
    <button class="btn lleno enorme" onClick=${abrir}>${c.pesAbierto ? t("IR A PES", "GO TO PES") : t("ABRIR PES", "OPEN PES")}</button>
    <button class="btn suave" onClick=${() => devolverPc(s.motor)}>${t("DEVOLVER LA PC", "GIVE BACK PC")}</button>
  </div>`;
}

/** Aviso cuando el modo termina (lo dice el motor en `ultimoFin`). */
let finVisto = null;
export function vigilarCedido(m) {
  const fin = m?.cedido?.ultimoFin || "";
  if (finVisto === null) { finVisto = fin; return; }
  if (fin && fin !== finVisto && !m?.cedido?.activo) avisar(t(`La PC volvió al anfitrión: ${fin}.`, `PC is back with the host: ${fin}.`), "info", 8000);
  finVisto = fin;
}

