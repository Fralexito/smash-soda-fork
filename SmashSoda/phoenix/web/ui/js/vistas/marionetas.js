// =============================================================================
//  JUGANDO › TU MANDO: el anfitrión elige su mando físico una vez y, al ponerse
//  en cualquier puesto (arrastrándose en Puestos), ese mando mueve ese puesto.
//  Por dentro es una marioneta de un solo puesto que pone MandoHost; sin mando,
//  el puesto del anfitrión se mueve con el teclado. (Antes: maestro y títeres.)
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Ajuste, Stepper, Segmentos, Chip, Vacio, cx, dos } from "../ui.js";
import { MandoVivo as Dibujo } from "./mandovivo.js";
import { PreferenciaPuesto } from "./jugar.js";

const TIPOS = { xbox: "XBOX", dualshock: "DUALSHOCK", ds4: "DS4", xinput: "XINPUT" };

function MandosVirtuales({ m }) {
  const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0;
  const cantidad = (x, d) => accion("mandos.cantidad", { xbox: x, ds4: d });
  return html`<${Tarjeta} secundaria interior="padding:22px 24px">
    <${Titulo} texto=${t("MANDOS VIRTUALES", "VIRTUAL PADS")} derecha=${html`<span class="mono mut">${xbox + ds4} / 8</span>`}/>
    <div class="ayuda" style="margin-bottom:6px">${t("Son los mandos que ve PES. Máximo 8 en total; con 4 por equipo alcanza para un 4 vs 4.", "Pads PES sees. Max 8 in total.")}</div>
    <${Ajuste} titulo="Xbox (XInput)" desc=${t("Los más compatibles con PES.", "Most compatible with PES.")}>
      <${Stepper} valor=${xbox} min=${0} max=${8 - ds4} deshabilitado=${m.mandos.esclavo} al=${(v) => cantidad(v, ds4)}/></${Ajuste}>
    <${Ajuste} titulo="DualShock 4" desc=${t("Para juegos que solo aceptan PlayStation.", "For games that only accept PlayStation pads.")}>
      <${Stepper} valor=${ds4} min=${0} max=${8 - xbox} deshabilitado=${m.mandos.esclavo} al=${(v) => cantidad(xbox, v)}/></${Ajuste}>
  </${Tarjeta}>`;
}

function TuMando({ m }) {
  const mar = m.marionetas;
  const bloqueado = !!m.mandos.esclavo;
  const actual = mar ? mar.maestro : -1;
  const usar = (i) => accion("mandos.miMando", { indice: i }, { ok: i >= 0 ? t("Listo: ese es tu mando.", "Done: that's your pad.") : t("Listo: usarás el teclado.", "Done: you'll use the keyboard.") });
  const nombre = actual >= 0 && mar?.maestros?.[actual] ? mar.maestros[actual].nombre : "";
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:16px">
    <${Titulo} texto=${t("TU MANDO (ANFITRIÓN)", "YOUR PAD (HOST)")}
      derecha=${nombre ? html`<${Chip} tipo="acc">${nombre}</${Chip}>` : html`<${Chip}>${t("TECLADO", "KEYBOARD")}</${Chip}>`}/>
    <ol class="ayuda" style="font-size:14px;margin:0;padding-left:20px;display:flex;flex-direction:column;gap:6px">
      <li>${t("Conecta tu mando y pulsa un botón: su luz se enciende en la lista.", "Plug in your pad and press a button: its light turns on in the list.")}</li>
      <li>${t("Pulsa «USAR ESTE».", "Press «USE THIS».")}</li>
      <li>${t("En Jugando › Puestos, arrástrate (TÚ) al puesto que quieras. Tu mando mueve ese puesto y puedes cambiarte cuando quieras.", "In Playing › Seats, drag yourself (YOU) to any seat. Your pad drives that seat; switch any time.")}</li>
    </ol>
    ${m.mandos?.ordenPes?.ds4windows ? html`<div class="caja" style="border-color:var(--warn);font-size:14px;display:flex;flex-direction:column;gap:6px">
      <b class="warn">${t("DS4Windows está abierto: esconde tu mando a Link.", "DS4Windows is open: it hides your pad from Link.")}</b>
      <span>${t("Por eso tu DS4 no sale aquí y aparece un «Xbox 360» de más. Ciérralo (clic derecho en su icono junto al reloj › Salir), quítalo del inicio de Windows (Administrador de tareas › Aplicaciones de inicio), reconecta tu mando y pulsa BUSCAR MANDOS.", "That's why your DS4 doesn't show and there's an extra «Xbox 360». Close it, remove it from Windows startup, reconnect your pad and press FIND PADS.")}</span>
    </div>` : null}
    ${bloqueado ? html`<div class="caja" style="border-color:var(--warn)">${t("Con turnos o torneo activos no se puede cambiar.", "Can't change during hotseat or tournament.")}</div>` : null}
    ${!mar ? html`<div class="ayuda">${t("Buscando mandos…", "Looking for pads…")}</div>` : html`
      <div class="col" style="gap:8px">
        ${mar.maestros.length === 0 ? html`<div class="caja mut" style="font-size:13px">${t("No veo ningún mando físico. Conéctalo y pulsa «BUSCAR MANDOS».", "No physical pad found. Plug it in and press «FIND PADS».")}</div>` : null}
        ${mar.maestros.map((x) => {
          const es = actual === x.n - 1;
          const ps = x.tipo === "ds4" || x.tipo === "dualshock";
          return html`<div class=${cx("fila-mar", es && "es")} key=${x.n}>
            <span class=${cx("luz", x.activo && "on")} title=${t("Se enciende al pulsar un botón", "Lights up when you press a button")}></span>
            <span class=${cx("chip", ps ? "acc2" : "mod")} style="height:22px;font-size:11px;flex:none">${ps ? "PLAYSTATION" : "XBOX"}</span>
            <div style="flex:1;min-width:0">
              <div class="nombre" style="font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${x.nombre}</div>
              ${x.activo ? html`<div class="mono" style="font-size:12px;color:var(--acc)">${t("¡te estoy viendo!", "I see you!")}</div>` : null}
            </div>
            ${es && mar.motor === "sdl" ? html`<${Boton} tipo="suave mini" deshabilitado=${bloqueado} titulo=${t("Si los botones salen cambiados, prueba otra lectura (Xbox / PlayStation)", "If buttons come out swapped, try another reading")}
              al=${() => accion("marionetas.tipo", { indice: x.n - 1 })}>${t("¿BOTONES CAMBIADOS?", "BUTTONS SWAPPED?")}</${Boton}>` : null}
            <${Boton} tipo=${es ? "lleno mini" : "mini"} deshabilitado=${bloqueado || es} al=${() => usar(x.n - 1)}>${es ? t("✓ EN USO", "✓ IN USE") : t("USAR ESTE", "USE THIS")}</${Boton}>
          </div>`;
        })}
        <div class=${cx("fila-mar", actual < 0 && "es")}>
          <span class="luz"></span>
          <div style="flex:1;min-width:0">
            <div class="nombre" style="font-weight:600">${t("Sin mando: teclado", "No pad: keyboard")}</div>
            <div class="mono mut" style="font-size:12px">${t("flechas · Enter = A · Retroceso = B · Espacio = Start", "arrows · Enter = A · Backspace = B · Space = Start")}</div>
          </div>
          <${Boton} tipo=${actual < 0 ? "lleno mini" : "mini"} deshabilitado=${bloqueado || actual < 0} al=${() => usar(-1)}>${actual < 0 ? t("✓ EN USO", "✓ IN USE") : t("USAR TECLADO", "USE KEYBOARD")}</${Boton}>
        </div>
      </div>
      <div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
        <${Boton} tipo="suave mini" deshabilitado=${bloqueado} al=${() => accion("marionetas.actualizar", {}, { ok: t("Lista actualizada.", "List refreshed.") })}>${t("BUSCAR MANDOS", "FIND PADS")}</${Boton}>
        ${mar.ocultos360 > 0 || mar.mostrar360 ? html`<span class="mono mut" style="font-size:12px">${mar.mostrar360
          ? t("Mostrando también los «Xbox 360».", "Also showing «Xbox 360» pads.")
          : t(`${mar.ocultos360} «Xbox 360» ocultos (son los de Link).`, `${mar.ocultos360} «Xbox 360» hidden (Link's own).`)}</span>
          <button class="enlace-mas" onClick=${() => accion("marionetas.mostrar360", { si: !mar.mostrar360 })}>${mar.mostrar360
          ? t("OCULTARLOS", "HIDE THEM") : t("¿JUEGAS CON UN XBOX 360? MOSTRARLOS", "PLAYING WITH A REAL XBOX 360? SHOW THEM")}</button>` : null}
      </div>`}
    <div style="height:1px;background:var(--tenue)"></div>
    <${PreferenciaPuesto} m=${m}/>
    <div class="ayuda" style="font-size:13px">${t("Aquí solo salen tus mandos físicos: los 8 mandos virtuales de Link no se muestran. Para que PES no lea tu mando por su cuenta (y no manejes dos jugadores), enciende «Orden de mandos en PES» en Jugando › Puestos.", "Only your physical pads show here; Link's 8 virtual pads are hidden. So PES doesn't also read your pad directly, turn on «Pad order in PES» in Playing › Seats.")}</div>
  </${Tarjeta}>`;
}

// ---- Prueba de mando: tu mando en vivo, con el mismo dibujo de «Mandos en vivo». ----
// Lee la API de mandos del navegador (mapeo estándar W3C) y la pasa al formato XInput de mandovivo.js.
const BIT = [0x1000, 0x2000, 0x4000, 0x8000, 0x100, 0x200, 0, 0, 0x20, 0x10, 0x40, 0x80, 0x1, 0x2, 0x4, 0x8, 0x400];
const NOMBRE_BOTON = ["A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "Start", "L3", "R3", "↑", "↓", "←", "→", "Guía"];
function aXInput(g) {
  let b = 0;
  g.botones.forEach((p, i) => { if (p && BIT[i]) b |= BIT[i]; });
  const ej = (v) => Math.round(Math.max(-1, Math.min(1, v || 0)) * 32767);
  return { b, lt: Math.round((g.valores[6] || 0) * 255), rt: Math.round((g.valores[7] || 0) * 255),
    lx: ej(g.ejes[0]), ly: -ej(g.ejes[1]), rx: ej(g.ejes[2]), ry: -ej(g.ejes[3]) };
}
function lectura(g) {
  const partes = g.botones.map((p, i) => (p ? NOMBRE_BOTON[i] || "B" + i : null)).filter(Boolean);
  const pal = (x, y, n) => (Math.abs(x) > 0.25 || Math.abs(y) > 0.25 ? `${n} ${Math.round(x * 100)}/${Math.round(-y * 100)}` : null);
  [pal(g.ejes[0] || 0, g.ejes[1] || 0, "L"), pal(g.ejes[2] || 0, g.ejes[3] || 0, "R")].forEach((x) => x && partes.push(x));
  return partes;
}

function PruebaMando({ m }) {
  const [todos, setPads] = useState([]);
  const virtuales = (m?.mandos?.lista || []).filter((p) => p.conectado && p.tipo !== "ds4").length;
  const x360 = todos.filter((g) => /xbox 360/i.test(g.nombre)).length;
  // Igual que en el motor: con mandos de Link enchufados, los «Xbox 360» son suyos (la cuenta no siempre cuadra)
  const pads = virtuales > 0 && x360 > 0 && !m?.marionetas?.mostrar360 ? todos.filter((g) => !/xbox 360/i.test(g.nombre)) : todos;
  useEffect(() => {
    let vivo = true, id = 0, ultimo = 0;
    const leer = (ahora) => {
      if (!vivo) return;
      if (!document.hidden && ahora - ultimo > 33) {   // ~30 lecturas por segundo, y nada con la ventana oculta
        ultimo = ahora;
        const lista = (navigator.getGamepads ? Array.from(navigator.getGamepads()) : []).filter(Boolean).map((g) => ({
          i: g.index, nombre: g.id.replace(/\(.*?\)/g, "").trim() || "Mando", estandar: g.mapping === "standard",
          botones: g.buttons.map((x) => x.pressed), valores: g.buttons.map((x) => x.value || 0), ejes: Array.from(g.axes),
        }));
        setPads((viejo) => (lista.length === 0 && viejo.length === 0 ? viejo : lista));
      }
      id = requestAnimationFrame(leer);
    };
    id = requestAnimationFrame(leer);
    return () => { vivo = false; cancelAnimationFrame(id); };
  }, []);
  return html`<${Tarjeta} secundaria interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
    <${Titulo} texto=${t("PRUEBA DE MANDO", "PAD TEST")}/>
    <div class="ayuda" style="font-size:14px">${t("Pulsa botones y mueve los sticks: se iluminan aquí. Sirve para reconocer tu mando y comprobar que todo responde.", "Press buttons and move the sticks: they light up here.")}</div>
    ${pads.length === 0 ? html`<div class="caja mut" style="font-size:13px">${t("Todavía no veo tu mando. Conéctalo y pulsa cualquier botón (los mandos virtuales de Link no salen aquí).", "No pad yet. Plug it in and press any button (Link's virtual pads don't show here).")}</div>`
      : pads.map((g) => html`<${MandoPrueba} key=${g.i} g=${g}/>`)}
  </${Tarjeta}>`;
}

function MandoPrueba({ g }) {
  const partes = lectura(g);
  const hay = partes.length > 0;
  return html`<div class="caja" style=${`display:flex;flex-direction:column;gap:10px;padding:14px 16px;transition:border-color .15s,box-shadow .15s;${hay ? "border-color:var(--acc);box-shadow:0 0 0 3px var(--acc-suave)" : ""}`}>
    <div style="display:flex;align-items:center;gap:10px;min-width:0">
      <span class=${cx("luz", hay && "on")}></span>
      <b style="flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${g.nombre}</b>
      <${Chip} tipo=${hay ? "acc" : ""}>${hay ? t("RESPONDE", "RESPONDING") : t("EN ESPERA", "IDLE")}</${Chip}>
    </div>
    <div style="max-width:340px;width:100%;margin:0 auto"><${Dibujo} e=${aXInput(g)}/></div>
    <div class="mono" style="font-size:12px;min-height:16px;text-align:center;color:${hay ? "var(--acc)" : "var(--mut)"}">${hay ? partes.join(" · ") : t("pulsa algo…", "press something…")}</div>
    ${g.estandar ? null : html`<div class="ayuda" style="font-size:12px;color:var(--warn)">${t("Este mando no usa el mapeo estándar: puede que algunos botones salgan cambiados.", "This pad doesn't use the standard mapping: some buttons may be swapped.")}</div>`}
  </div>`;
}

export function MarionetasNuevo({ m }) {
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:1.5 1 0;min-width:340px"><${TuMando} m=${m}/></div>
    <div class="col" style="flex:1 1 0;min-width:300px"><${PruebaMando} m=${m}/><${MandosVirtuales} m=${m}/></div>
  </div>`;
}
