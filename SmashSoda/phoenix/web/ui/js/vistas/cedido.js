// =============================================================================
//  Modo cedido: el anfitrión cede su PC a un invitado (teclado y ratón solo en
//  Link y PES). Botón en la ficha de la persona, diálogo para ceder, banda con
//  ABRIR PES y diálogo de PIN para recuperar la PC. Guía: docs/MODO-CEDIDO.md.
//  El motor bloquea por su cuenta lo que aquí se oculta (error PC_CEDIDA).
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar, avisar } from "../tienda.js";
import { Icono, Boton, cx } from "../ui.js";

// Lo que se ve mientras la PC está cedida (lo demás se oculta y el motor lo rechaza)
export const SECCIONES_CEDIDO = ["sala", "partido", "mandos"];
export const PESTANAS_CEDIDO = { sala: ["resumen", "red", "actividad"], partido: ["vivo", "historial"], mandos: ["puestos", "bloqueo"] };
export const estaCedida = (s) => !!s?.motor?.cedido?.activo;

// ---- Botón en la ficha de una persona ------------------------------------------------
export function BotonCeder({ g, m }) {
  if (!g || g.host || !m?.sala?.abierta || m?.cedido?.activo) return null;
  return html`<${Boton} tipo="acc" titulo=${t("Dale tu PC: solo podrá usar Phoenix Link y PES con teclado y ratón.", "Hand over your PC: only Phoenix Link and PES with keyboard and mouse.")}
    al=${() => cambiar({ ceder: { parsecId: g.parsecId, nombre: g.nombre } })}><${Icono} n="candado" t=${15}/>${t("CEDER MI PC", "HAND OVER PC")}</${Boton}>`;
}

function CampoPin({ valor, al, alEnter, autoFocus = false, etiqueta }) {
  const ref = useRef(null);
  useEffect(() => { if (autoFocus) setTimeout(() => ref.current?.focus(), 30); }, []);
  return html`<label style="display:flex;flex-direction:column;gap:6px;flex:1;min-width:140px">
    <span class="lab">${etiqueta}</span>
    <input ref=${ref} class="campo mono" type="password" inputmode="numeric" autocomplete="off" maxlength="8" value=${valor}
      style="letter-spacing:.4em;font-size:20px;text-align:center"
      onInput=${(e) => al(e.currentTarget.value.replace(/\D/g, "").slice(0, 8))}
      onKeyDown=${(e) => { if (e.key === "Enter" && alEnter) alEnter(); }}/>
  </label>`;
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
  const [pin, setPin] = useState("");
  const [pin2, setPin2] = useState("");
  const [enviando, setEnviando] = useState(false);
  useEffect(() => { setPin(""); setPin2(""); }, [d?.parsecId]);
  if (!d) return null;
  const cerrar = () => cambiar({ ceder: null });
  const necesitaPin = !c.pinGuardado;
  const pinListo = !necesitaPin || (/^\d{4,8}$/.test(pin) && pin === pin2);
  const ceder = async () => {
    if (!pinListo || enviando) return;
    setEnviando(true);
    const r = await accion("cedido.empezar", necesitaPin ? { parsecId: d.parsecId, pinNuevo: pin } : { parsecId: d.parsecId });
    setEnviando(false);
    if (r) { cerrar(); avisar(t(`PC cedida a ${d.nombre}.`, `PC handed over to ${d.nombre}.`), "ok"); }
  };
  return html`<${Velo} alCerrar=${cerrar}>
    <h2>${t(`¿Ceder tu PC a ${d.nombre}?`, `Hand over your PC to ${d.nombre}?`)}</h2>
    <ul class="ayuda lista-cedido">
      <li>${t(`Solo ${d.nombre} tendrá teclado y ratón. Los demás siguen con su mando.`, `Only ${d.nombre} gets keyboard and mouse. Everyone else keeps their pad.`)}</li>
      <li>${t("Solo podrá usar Phoenix Link (Sala, Partido y Mandos) y PES. Windows, Ajustes, Sync y tu cuenta quedan cerrados.", "Only Phoenix Link (Room, Match, Pads) and PES. Windows, Settings, Sync and your account stay locked.")}</li>
      <li>${t("Para recuperarla: tu PIN, o Ctrl+Alt+F12 en tu teclado. Si sale de la sala, la PC vuelve sola.", "To take it back: your PIN, or Ctrl+Alt+F12 on your keyboard. If they leave, it comes back on its own.")}</li>
      <li>${c.juego
        ? t(`El botón ABRIR PES abre «${c.juego}» (Sala › Juegos).`, `OPEN PES launches “${c.juego}” (Room › Games).`)
        : html`<span style="color:var(--warn)">${t("No hay juego elegido en Sala › Juegos: ABRIR PES no podrá abrirlo (solo traerlo al frente si ya está abierto).", "No game picked in Room › Games: OPEN PES can only bring it to front.")}</span>`}</li>
    </ul>
    ${necesitaPin ? html`
      <div class="lab acc" style="margin-top:16px">${t("ELIGE TU PIN (4 A 8 NÚMEROS)", "PICK YOUR PIN (4 TO 8 DIGITS)")}</div>
      <div class="ayuda" style="margin-top:4px">${t("Se guarda cifrado en esta PC. No se lo digas a quien recibe la PC.", "Stored hashed on this PC. Don't tell the person you hand it to.")}</div>
      <div style="display:flex;gap:12px;margin-top:10px;flex-wrap:wrap">
        <${CampoPin} valor=${pin} al=${setPin} autoFocus etiqueta=${t("PIN", "PIN")}/>
        <${CampoPin} valor=${pin2} al=${setPin2} alEnter=${ceder} etiqueta=${t("REPÍTELO", "REPEAT")}/>
      </div>
      ${pin2.length >= 4 && pin !== pin2 ? html`<div class="ayuda" style="color:var(--bad);margin-top:6px">${t("Los dos PIN no coinciden.", "PINs don't match.")}</div>` : null}` : null}
    <div class="botones">
      <button class="btn suave" onClick=${cerrar}>${t("Cancelar", "Cancel")}</button>
      <button class="btn lleno" disabled=${!pinListo || enviando} onClick=${ceder}>${t("CEDER MI PC", "HAND OVER")}</button>
    </div>
  </${Velo}>`;
}

// ---- Diálogo para recuperar (PIN) ---------------------------------------------------------
export function DialogoRecuperar({ s }) {
  const c = s.motor?.cedido || {};
  const [pin, setPin] = useState("");
  if (!s.recuperar || !c.activo) return null;
  const cerrar = () => { setPin(""); cambiar({ recuperar: false }); };
  const salir = async () => {
    if (pin.length < 4) return;
    const r = await accion("cedido.salir", { pin });
    setPin("");
    if (r) { cambiar({ recuperar: false }); avisar(t("PC recuperada.", "PC back."), "ok"); }
  };
  return html`<${Velo} alCerrar=${cerrar}>
    <h2>${t("Recuperar la PC", "Take the PC back")}</h2>
    <div class="ayuda" style="font-size:14px">${t(`Solo el anfitrión. Escribe tu PIN, o pulsa ${c.atajo || "Ctrl+Alt+F12"} en el teclado de esta PC.`, `Host only. Type your PIN, or press ${c.atajo || "Ctrl+Alt+F12"} on this PC's keyboard.`)}</div>
    ${c.espera > 0 ? html`<div class="ayuda" style="color:var(--warn);margin-top:8px">${t(`Demasiados intentos: espera ${c.espera} s.`, `Too many tries: wait ${c.espera} s.`)}</div>` : null}
    <div style="display:flex;margin-top:14px"><${CampoPin} valor=${pin} al=${setPin} alEnter=${salir} autoFocus etiqueta="PIN"/></div>
    <div class="botones">
      <button class="btn suave" onClick=${cerrar}>${t("Cancelar", "Cancel")}</button>
      <button class="btn lleno" disabled=${pin.length < 4 || c.espera > 0} onClick=${salir}>${t("RECUPERAR", "TAKE BACK")}</button>
    </div>
  </${Velo}>`;
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
      <div class="ayuda">${t("Solo Phoenix Link y PES, con teclado y ratón.", "Only Phoenix Link and PES, keyboard and mouse.")} ${minutos(c.desde) ? "· " + minutos(c.desde) : ""}</div>
    </div>
    <button class="btn lleno enorme" onClick=${abrir}>${c.pesAbierto ? t("IR A PES", "GO TO PES") : t("ABRIR PES", "OPEN PES")}</button>
    <button class="btn suave" onClick=${() => cambiar({ recuperar: true })}>${t("RECUPERAR PC", "TAKE BACK")}</button>
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

