// =============================================================================
//  MANDOS › TECLADO (rehecho): permisos de teclado por invitado y el mapa de
//  teclas (qué tecla del teclado es cada botón del mando). Se toca un botón del
//  mando dibujado y se pulsa la tecla nueva. Todo dentro de la interfaz nueva.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, irA } from "../tienda.js";
import { Tarjeta, Titulo, Boton, AjusteSw, Chip, cx } from "../ui.js";

// KeyboardEvent.code → código de tecla del motor (los de Windows; 0x100 = tecla extendida).
const CODIGOS = {
  Digit1: 0x02, Digit2: 0x03, Digit3: 0x04, Digit4: 0x05, Digit5: 0x06, Digit6: 0x07, Digit7: 0x08, Digit8: 0x09, Digit9: 0x0A, Digit0: 0x0B,
  Minus: 0x0C, Equal: 0x0D, Backspace: 0x0E, Tab: 0x0F,
  KeyQ: 0x10, KeyW: 0x11, KeyE: 0x12, KeyR: 0x13, KeyT: 0x14, KeyY: 0x15, KeyU: 0x16, KeyI: 0x17, KeyO: 0x18, KeyP: 0x19,
  BracketLeft: 0x1A, BracketRight: 0x1B, Enter: 0x1C, ControlLeft: 0x1D,
  KeyA: 0x1E, KeyS: 0x1F, KeyD: 0x20, KeyF: 0x21, KeyG: 0x22, KeyH: 0x23, KeyJ: 0x24, KeyK: 0x25, KeyL: 0x26,
  Semicolon: 0x27, Quote: 0x28, Backquote: 0x29, ShiftLeft: 0x2A, Backslash: 0x2B,
  KeyZ: 0x2C, KeyX: 0x2D, KeyC: 0x2E, KeyV: 0x2F, KeyB: 0x30, KeyN: 0x31, KeyM: 0x32,
  Comma: 0x33, Period: 0x34, Slash: 0x35, ShiftRight: 0x36, NumpadMultiply: 0x37, AltLeft: 0x38, Space: 0x39, CapsLock: 0x3A,
  F1: 0x3B, F2: 0x3C, F3: 0x3D, F4: 0x3E, F5: 0x3F, F6: 0x40, F7: 0x41, F8: 0x42, F9: 0x43, F10: 0x44,
  NumLock: 0x45, ScrollLock: 0x46, Numpad7: 0x47, Numpad8: 0x48, Numpad9: 0x49, NumpadSubtract: 0x4A,
  Numpad4: 0x4B, Numpad5: 0x4C, Numpad6: 0x4D, NumpadAdd: 0x4E, Numpad1: 0x4F, Numpad2: 0x50, Numpad3: 0x51, Numpad0: 0x52, NumpadDecimal: 0x53,
  F11: 0x57, F12: 0x58, F13: 0x64, F14: 0x65, F15: 0x66, F16: 0x67, F17: 0x68, F18: 0x69, F19: 0x6A,
  NumpadEnter: 0x11C, ControlRight: 0x11D, NumpadDivide: 0x135, PrintScreen: 0x137, AltRight: 0x138, Pause: 0x146,
  Home: 0x147, ArrowUp: 0x148, PageUp: 0x149, ArrowLeft: 0x14B, ArrowRight: 0x14D, End: 0x14F, ArrowDown: 0x150,
  PageDown: 0x151, Insert: 0x152, Delete: 0x153, MetaLeft: 0x15B, MetaRight: 0x15C, ContextMenu: 0x15D,
};

// Botones del mando, con su nombre en el motor (en minúsculas) y cómo se ven.
const BOTON = {
  lt: ["LT", "lt"], lb: ["LB", "lb"], rt: ["RT", "rt"], rb: ["RB", "rb"], back: ["BACK", "back"], start: ["START", "start"],
  lup: ["▲", "lup"], ldown: ["▼", "ldown"], lleft: ["◀", "lleft"], lright: ["▶", "lright"], lthumb: ["L3", "lthumb"],
  rup: ["▲", "rup"], rdown: ["▼", "rdown"], rleft: ["◀", "rleft"], rright: ["▶", "rright"], rthumb: ["R3", "rthumb"],
  dup: ["▲", "dup"], ddown: ["▼", "ddown"], dleft: ["◀", "dleft"], dright: ["▶", "dright"],
  a: ["A", "a"], b: ["B", "b"], x: ["X", "x"], y: ["Y", "y"],
};

function Tk({ id, perfil, esperando, alElegir }) {
  const [pad, nombre] = BOTON[id];
  const tc = (perfil?.teclas || []).find((x) => x.b === nombre);
  const et = tc && tc.v ? (tc.e || "#" + tc.v) : "—";
  const esta = esperando === nombre;
  return html`<button class=${cx("tk", esta && "esperando", !(tc && tc.v) && "vacio")} onClick=${() => alElegir(esta ? null : nombre)}
    title=${esta ? t("Pulsa la tecla nueva (Esc cancela)", "Press the new key (Esc cancels)") : t("Toca para cambiar su tecla", "Tap to change its key")}>
    <span class="pad">${pad}</span><span class="kc">${esta ? "…" : et}</span>
  </button>`;
}

function MapaTeclas({ m, invitados }) {
  const perfiles = m.teclado?.perfiles || [];
  const [sel, setSel] = useState(0);
  const [esperando, setEsperando] = useState(null);
  const perfil = perfiles.find((p) => p.userId === sel) || perfiles.find((p) => p.userId === 0) || perfiles[0];
  const userId = perfil ? perfil.userId : 0;

  useEffect(() => {
    if (!esperando) return undefined;
    const alTeclear = (e) => {
      e.preventDefault(); e.stopPropagation();
      if (e.code === "Escape") { setEsperando(null); return; }
      const v = CODIGOS[e.code];
      if (!v) { avisar(t("Esa tecla no se puede usar. Prueba con otra.", "That key can't be used. Try another."), "bad"); return; }
      setEsperando(null);
      accion("teclado.asignar", { userId, boton: esperando, tecla: v, nombre: perfil?.nombre || "" });
    };
    window.addEventListener("keydown", alTeclear, true);
    return () => window.removeEventListener("keydown", alTeclear, true);
  }, [esperando, userId]);

  if (!m.teclado) return html`<${Tarjeta} interior="padding:22px 24px"><${Titulo} texto=${t("MAPA DE TECLAS", "KEY MAP")}/><div class="ayuda">${t("Cargando…", "Loading…")}</div></${Tarjeta}>`;

  const sinPerfil = invitados.filter((x) => !perfiles.some((p) => p.userId === x.parsecId));
  const T = (id) => html`<${Tk} id=${id} perfil=${perfil} esperando=${esperando} alElegir=${setEsperando}/>`;
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:16px">
    <${Titulo} texto=${t("MAPA DE TECLAS", "KEY MAP")}
      derecha=${esperando ? html`<${Chip} tipo="warn">${t("PULSA UNA TECLA…", "PRESS A KEY…")}</${Chip}>` : null}/>
    <div class="ayuda" style="font-size:14px">
      ${t("Toca un botón del mando y pulsa la tecla del teclado que quieres usar. Si esa tecla ya estaba en otro botón, ese queda sin tecla. «Esc» cancela.",
          "Tap a pad button, then press the keyboard key to use. If the key was on another button, that one is cleared. Esc cancels.")}
    </div>
    <div>
      <div class="lab" style="margin-bottom:8px">${t("PERFIL", "PROFILE")}</div>
      <div style="display:flex;gap:8px;flex-wrap:wrap">${perfiles.map((p) => html`<button key=${p.userId} class=${cx("chip-perfil", p.userId === userId && "on")}
        onClick=${() => { setSel(p.userId); setEsperando(null); }}>${p.userId === 0 ? t("Predeterminado", "Default") : p.nombre}</button>`)}</div>
      ${sinPerfil.length ? html`<div class="ayuda" style="margin:10px 0 6px">${t("Dale su propio mapa a un invitado:", "Give a guest their own map:")}</div>
        <div style="display:flex;gap:8px;flex-wrap:wrap">${sinPerfil.map((x) => html`<${Boton} key=${x.parsecId} tipo="suave mini"
          al=${async () => { await accion("teclado.crear", { userId: x.parsecId, nombre: x.nombre }); setSel(x.parsecId); }}>+ ${x.nombre}</${Boton}>`)}</div>` : null}
    </div>
    <div class="mando-m">
      <div class="mm-fila mm-sup">
        <div class="mm-par">${T("lt")}${T("lb")}</div>
        <div class="mm-bs" style="margin-top:6px">${T("back")}${T("start")}</div>
        <div class="mm-par">${T("rt")}${T("rb")}</div>
      </div>
      <div class="mm-fila mm-inf">
        <div class="mm-grupo">
          <div class="lab">${t("PALANCA IZQ.", "LEFT STICK")}</div>
          <div class="mm-cruz2"><span></span>${T("lup")}<span></span>${T("lleft")}${T("lthumb")}${T("lright")}<span></span>${T("ldown")}<span></span></div>
          <div class="lab" style="margin-top:6px">${t("CRUCETA", "D-PAD")}</div>
          <div class="mm-cruz2"><span></span>${T("dup")}<span></span>${T("dleft")}<span class="mm-hueco"></span>${T("dright")}<span></span>${T("ddown")}<span></span></div>
        </div>
        <div class="mm-grupo">
          <div class="lab">${t("PALANCA DER.", "RIGHT STICK")}</div>
          <div class="mm-cruz2"><span></span>${T("rup")}<span></span>${T("rleft")}${T("rthumb")}${T("rright")}<span></span>${T("rdown")}<span></span></div>
          <div class="lab" style="margin-top:6px">${t("BOTONES", "BUTTONS")}</div>
          <div class="mm-cruz2"><span></span>${T("y")}<span></span>${T("x")}<span class="mm-hueco"></span>${T("b")}<span></span>${T("a")}<span></span></div>
        </div>
      </div>
    </div>
    <div style="display:flex;gap:10px;flex-wrap:wrap">
      <${Boton} tipo="suave mini" al=${() => accion("teclado.reiniciar", { userId }, { ok: t("Teclas devueltas a las de fábrica.", "Keys reset to default.") })}>${t("VOLVER A LAS DE FÁBRICA", "RESET TO DEFAULT")}</${Boton}>
      ${userId !== 0 ? html`<${Boton} tipo="peligro mini" al=${async () => { await accion("teclado.borrar", { userId }); setSel(0); }}>${t("BORRAR ESTE PERFIL", "DELETE THIS PROFILE")}</${Boton}>` : null}
    </div>
  </${Tarjeta}>`;
}

export function TecladoNuevo({ m }) {
  const g = m.ajustes?.general || {};
  const invitados = (m.invitados || []).filter((x) => !x.falso);
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:1 1 0;min-width:300px">
      <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:8px">
        <${Titulo} texto=${t("TECLADO DE LOS INVITADOS", "GUEST KEYBOARD")}/>
        <${AjusteSw} titulo=${t("Desactivar teclado para todos", "Disable keyboard for everyone")} desc=${t("Nadie juega con teclado aunque tenga permiso.", "Nobody plays with keyboard even if allowed.")}
          valor=${g.disableKeyboard} al=${(v) => accion("ajustes.general", { clave: "disableKeyboard", valor: v })}/>
        <div class="ayuda" style="margin-top:10px">${t("El permiso de teclado de cada invitado está en su ficha (Gente › En sala).", "Each guest's keyboard permission is on their card (People › In room).")}</div>
        <div><${Boton} tipo="mini suave" al=${() => irA("gente", "sala")}>${t("IR A GENTE", "GO TO PEOPLE")}</${Boton}></div>
      </${Tarjeta}>
    </div>
    <div class="col" style="flex:1.6 1 0;min-width:360px"><${MapaTeclas} m=${m} invitados=${invitados}/></div>
  </div>`;
}
