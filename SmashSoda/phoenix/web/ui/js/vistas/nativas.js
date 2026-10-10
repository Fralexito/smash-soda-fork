// =============================================================================
//  FUNCIONES NATIVAS DE SMASH SODA que antes solo estaban en el panel clásico:
//  la biblioteca de juegos (añadir, editar, quitar) y los atajos de teclado
//  (Ctrl + tecla → comando del chat). Mismo camino que LibraryWidget y
//  Config::AddHotkey / RemoveHotkey del original.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Campo, Chip, Icono } from "../ui.js";

// ---- Biblioteca de juegos ---------------------------------------------------------
export function EditorBiblioteca({ alCambiar }) {
  const [juegos, setJuegos] = useState(null);
  const [form, setForm] = useState(null);   // null | { id?, nombre, ruta, parametros }
  const cargar = async () => { const r = await accion("biblioteca.lista", {}, { silencioso: true }); if (r) setJuegos(r.juegos || []); };
  useEffect(() => { cargar(); }, []);
  const guardar = async () => {
    if (await accion("biblioteca.guardar", form, { ok: t("Juego guardado.", "Game saved.") })) { setForm(null); cargar(); alCambiar && alCambiar(); }
  };
  const borrar = async (j) => {
    if (await confirmar({ titulo: t(`¿Quitar «${j.nombre}» de la biblioteca?`, `Remove «${j.nombre}»?`), texto: t("No se borra el juego de tu PC, solo deja de estar en la lista.", "The game stays on your PC."), si: t("Quitar", "Remove"), peligro: true })) {
      if (await accion("biblioteca.borrar", { id: j.id })) { cargar(); alCambiar && alCambiar(); }
    }
  };
  if (!juegos) return html`<div class="ayuda">${t("Cargando biblioteca…", "Loading library…")}</div>`;
  return html`<div style="display:flex;flex-direction:column;gap:8px">
    ${juegos.map((j) => html`<div class="persona" style="cursor:default" key=${j.id}>
      <div style="flex:1;min-width:0"><div class="nom">${j.nombre}</div>
        <div class="mono mut" style="font-size:12px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap" title=${j.ruta}>${j.ruta}${j.parametros ? " " + j.parametros : ""}</div></div>
      <${Boton} tipo="mini suave" al=${() => setForm({ id: j.id, nombre: j.nombre, ruta: j.ruta, parametros: j.parametros || "" })}>${t("EDITAR", "EDIT")}</${Boton}>
      <${Boton} tipo="mini peligro" al=${() => borrar(j)}>${t("QUITAR", "REMOVE")}</${Boton}>
    </div>`)}
    ${juegos.length === 0 && !form ? html`<div class="ayuda">${t("La biblioteca está vacía.", "The library is empty.")}</div>` : null}
    ${form ? html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
      <div class="lab acc">${form.id ? t("EDITAR JUEGO", "EDIT GAME") : t("AÑADIR JUEGO", "ADD GAME")}</div>
      <${Campo} valor=${form.nombre} max=${128} placeholder=${t("Nombre (p. ej. PES 2021)", "Name (e.g. PES 2021)")} al=${(v) => setForm({ ...form, nombre: v })}/>
      <${Campo} valor=${form.ruta} max=${1024} mono placeholder=${t("Ruta del .exe (p. ej. C:\\Juegos\\PES 2021\\PES2021.exe)", "Path to the .exe")} al=${(v) => setForm({ ...form, ruta: v })}/>
      <${Campo} valor=${form.parametros} max=${512} mono placeholder=${t("Parámetros (opcional)", "Parameters (optional)")} al=${(v) => setForm({ ...form, parametros: v })}/>
      <div style="display:flex;gap:8px;justify-content:flex-end">
        <${Boton} tipo="suave" al=${() => setForm(null)}>${t("CANCELAR", "CANCEL")}</${Boton}>
        <${Boton} tipo="lleno" deshabilitado=${!form.nombre.trim() || !form.ruta.trim()} al=${guardar}>${t("GUARDAR", "SAVE")}</${Boton}>
      </div>
      <div class="ayuda">${t("Tras guardar, elígelo arriba para que el modo quiosco lo abra y lo vigile.", "After saving, pick it above so kiosk mode launches it.")}</div>
    </div>` : html`<div><${Boton} al=${() => setForm({ nombre: "", ruta: "", parametros: "" })}><${Icono} n="mas" t=${15}/>${t("AÑADIR JUEGO", "ADD GAME")}</${Boton}></div>`}
  </div>`;
}

// ---- Atajos de teclado ---------------------------------------------------------------
// Teclas que Windows deja usar con Ctrl: letras, números y F1–F12 (código de tecla virtual).
function codigoTecla(e) {
  if (/^Key[A-Z]$/.test(e.code)) return e.code.charCodeAt(3);
  if (/^Digit[0-9]$/.test(e.code)) return e.code.charCodeAt(5);
  const f = /^F([1-9]|1[0-2])$/.exec(e.code);
  if (f) return 0x6f + Number(f[1]);
  return 0;
}
const SUGERIDOS = ["!lockall", "!unlockall", "!dcall", "!rc", "!videofix", "!restart"];

export function AtajosTeclado() {
  const [d, setD] = useState(null);
  const [comando, setComando] = useState("");
  const [esperando, setEsperando] = useState(false);
  const cargar = async () => { const r = await accion("atajos.lista", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { cargar(); }, []);
  useEffect(() => {
    if (!esperando) return undefined;
    const tecla = async (e) => {
      e.preventDefault(); e.stopPropagation();
      if (e.key === "Escape") { setEsperando(false); return; }
      const v = codigoTecla(e);
      if (!v) return;   // otra tecla: se sigue esperando
      setEsperando(false);
      if (await accion("atajos.agregar", { comando: comando.trim(), tecla: v }, { ok: t("Atajo guardado.", "Shortcut saved.") })) { setComando(""); cargar(); }
    };
    window.addEventListener("keydown", tecla, true);
    return () => window.removeEventListener("keydown", tecla, true);
  }, [esperando, comando]);
  const borrar = async (i) => { if (await accion("atajos.borrar", { indice: i })) cargar(); };
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("ATAJOS DE TECLADO", "KEYBOARD SHORTCUTS")} derecha=${esperando ? html`<${Chip} tipo="vip">${t("PULSA UNA TECLA…", "PRESS A KEY…")}</${Chip}>` : html`<span class="mono mut">${d ? d.atajos.length : "…"}</span>`}/>
    <div class="ayuda">${t("Ctrl + una tecla ejecuta un comando del chat aunque estés dentro del juego (por ejemplo Ctrl+L → !lockall).", "Ctrl + a key runs a chat command even in-game (e.g. Ctrl+L → !lockall).")}</div>
    ${(d?.atajos || []).map((a, i) => html`<div class="persona" style="cursor:default" key=${i}>
      <span class="chip acc" style="height:26px">Ctrl + ${a.nombre || a.tecla}</span>
      <span class="mono" style="flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis">${a.comando}</span>
      <${Boton} tipo="mini peligro" al=${() => borrar(i)}>${t("QUITAR", "REMOVE")}</${Boton}>
    </div>`)}
    <div style="display:flex;gap:8px;align-items:center">
      <input class="campo mono" placeholder=${t("Comando, p. ej. !lockall", "Command, e.g. !lockall")} maxLength="200" value=${comando} aria-label=${t("Comando del atajo", "Shortcut command")}
        onInput=${(e) => setComando(e.currentTarget.value)}/>
      <${Boton} tipo="lleno" deshabilitado=${!comando.trim() || esperando} al=${() => setEsperando(true)}>${t("ELEGIR TECLA", "PICK KEY")}</${Boton}>
    </div>
    <div style="display:flex;gap:6px;flex-wrap:wrap">${SUGERIDOS.map((c) => html`<button key=${c} class="chip-perfil" onClick=${() => setComando(c)}>${c}</button>`)}</div>
    <div class="ayuda">${t("Letras, números o F1–F12. Esc cancela. Si otra app ya usa ese Ctrl + tecla, Windows no lo deja.", "Letters, digits or F1–F12. Esc cancels.")}</div>
  </${Tarjeta}>`;
}
