// =============================================================================
//  MANDOS › TU MANDO: el anfitrión elige su mando físico una vez y, al ponerse
//  en cualquier puesto (arrastrándose en Puestos), ese mando mueve ese puesto.
//  Por dentro es una marioneta de un solo puesto que pone MandoHost; sin mando,
//  el puesto del anfitrión se mueve con el teclado. (Antes: maestro y títeres.)
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Ajuste, Stepper, Segmentos, Chip, Vacio, cx, dos } from "../ui.js";

const TIPOS = { xbox: "XBOX", dualshock: "DUALSHOCK", ds4: "DS4", xinput: "XINPUT" };

function MandosVirtuales({ m }) {
  const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0;
  const cantidad = (x, d) => accion("mandos.cantidad", { xbox: x, ds4: d });
  return html`<${Tarjeta} interior="padding:22px 24px">
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
      <li>${t("En Mandos › Puestos, arrástrate (TÚ) al puesto que quieras. Tu mando mueve ese puesto y puedes cambiarte cuando quieras.", "In Pads › Seats, drag yourself (YOU) to any seat. Your pad drives that seat; switch any time.")}</li>
    </ol>
    ${bloqueado ? html`<div class="caja" style="border-color:var(--warn)">${t("Con turnos o torneo activos no se puede cambiar.", "Can't change during hotseat or tournament.")}</div>` : null}
    ${!mar ? html`<div class="ayuda">${t("Buscando mandos…", "Looking for pads…")}</div>` : html`
      <div class="col" style="gap:8px">
        ${mar.maestros.length === 0 ? html`<div class="ayuda">${t("No veo ningún mando físico. Conéctalo y pulsa «BUSCAR MANDOS».", "No physical pad found. Plug it in and press «FIND PADS».")}</div>` : null}
        ${mar.maestros.map((x) => {
          const es = actual === x.n - 1;
          return html`<div class=${cx("fila-mar", es && "es")} key=${x.n}>
            <span class=${cx("luz", x.activo && "on")} title=${t("Se enciende al pulsar un botón", "Lights up when you press a button")}></span>
            <div style="flex:1;min-width:0">
              <div class="nombre" style="font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${x.nombre}</div>
              <div class="mono mut" style="font-size:12px">${es ? t("es tu mando", "your pad") : t("pulsa un botón para reconocerlo", "press a button to spot it")}</div>
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
      <div><${Boton} tipo="suave mini" deshabilitado=${bloqueado} al=${() => accion("marionetas.actualizar", {}, { ok: t("Lista actualizada.", "List refreshed.") })}>${t("BUSCAR MANDOS", "FIND PADS")}</${Boton}></div>`}
    <div class="ayuda" style="font-size:13px">${t("Para que PES no vea tu mando por su cuenta (y no controles dos jugadores), enciende «Orden de mandos en PES» en Mandos › Puestos.", "So PES doesn't also read your pad directly, turn on «Pad order in PES» in Pads › Seats.")}</div>
  </${Tarjeta}>`;
}

// ---- Prueba de mando: dibuja tu mando en vivo (API de mandos del navegador). Solo trabaja con esta pestaña abierta. ----
function PruebaMando() {
  const [pads, setPads] = useState([]);
  useEffect(() => {
    let vivo = true, id = 0, ultimo = 0;
    const leer = (ahora) => {
      if (!vivo) return;
      if (!document.hidden && ahora - ultimo > 33) {   // ~30 lecturas por segundo, y nada con la ventana oculta
        ultimo = ahora;
        const lista = (navigator.getGamepads ? Array.from(navigator.getGamepads()) : []).filter(Boolean).map((g) => ({
          i: g.index, nombre: g.id.replace(/\(.*?\)/g, "").trim() || "Mando",
          b: g.buttons.map((x) => (x.pressed ? 1 : 0)), v: g.buttons.map((x) => x.value || 0), ejes: Array.from(g.axes),
        }));
        setPads((viejo) => (lista.length === 0 && viejo.length === 0 ? viejo : lista));
      }
      id = requestAnimationFrame(leer);
    };
    id = requestAnimationFrame(leer);
    return () => { vivo = false; cancelAnimationFrame(id); };
  }, []);
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
    <${Titulo} texto=${t("PRUEBA DE MANDO", "PAD TEST")}/>
    <div class="ayuda" style="font-size:14px">${t("Pulsa un botón de tu mando: se ilumina aquí. Así reconoces cuál es el tuyo y compruebas que todo responde.", "Press a button on your pad: it lights up here. That shows which one is yours and that everything responds.")}</div>
    ${pads.length === 0 ? html`<div class="caja mut" style="font-size:13px">${t("Todavía no veo ningún mando. Conéctalo y pulsa cualquier botón.", "No pad yet. Plug it in and press any button.")}</div>`
      : pads.map((g) => html`<${MandoVivo} key=${g.i} g=${g}/>`)}
  </${Tarjeta}>`;
}

function MandoVivo({ g }) {
  const on = (n) => (g.b[n] ? "var(--acc)" : "rgba(255,255,255,.14)");
  const hay = g.b.some((x) => x) || g.ejes.some((e) => Math.abs(e) > 0.3);
  const palo = (cx0, cy0, ax, ay, pres) => html`<g>
    <circle cx=${cx0} cy=${cy0} r="17" fill="none" stroke=${pres ? "var(--acc)" : "rgba(255,255,255,.2)"} stroke-width="2"/>
    <circle cx=${cx0 + (g.ejes[ax] || 0) * 11} cy=${cy0 + (g.ejes[ay] || 0) * 11} r="8" fill=${pres ? "var(--acc)" : "rgba(255,255,255,.35)"}/></g>`;
  return html`<div class=${cx("caja", hay && "vivo-mando")} style=${hay ? "border-color:var(--acc)" : ""}>
    <div style="display:flex;align-items:center;gap:8px;margin-bottom:8px">
      <span class=${cx("luz", hay && "on")}></span>
      <div class="mono" style="font-size:12px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${dos(g.i + 1)} · ${g.nombre}</div></div>
    <svg viewBox="0 0 240 110" width="100%" style="max-width:340px;display:block;margin:0 auto" aria-hidden="true">
      <rect x="60" y="2" width="30" height="9" rx="4" fill=${on(4)}/><rect x="150" y="2" width="30" height="9" rx="4" fill=${on(5)}/>
      <rect x="62" y="14" width="26" height="4" rx="2" fill="rgba(255,255,255,.14)"/><rect x="62" y="14" width=${26 * (g.v[6] || 0)} height="4" rx="2" fill="var(--acc)"/>
      <rect x="152" y="14" width="26" height="4" rx="2" fill="rgba(255,255,255,.14)"/><rect x="152" y="14" width=${26 * (g.v[7] || 0)} height="4" rx="2" fill="var(--acc)"/>
      ${palo(60, 60, 0, 1, g.b[10])}${palo(150, 88, 2, 3, g.b[11])}
      <g fill=${on(12)}><rect x="93" y="42" width="8" height="10" rx="2"/></g><g fill=${on(13)}><rect x="93" y="62" width="8" height="10" rx="2"/></g>
      <g fill=${on(14)}><rect x="84" y="52" width="9" height="10" rx="2"/></g><g fill=${on(15)}><rect x="101" y="52" width="9" height="10" rx="2"/></g>
      <circle cx="112" cy="40" r="5" fill=${on(8)}/><circle cx="128" cy="40" r="5" fill=${on(9)}/>
      <circle cx="205" cy="58" r="8" fill=${on(0)}/><circle cx="220" cy="46" r="8" fill=${on(1)}/><circle cx="190" cy="46" r="8" fill=${on(2)}/><circle cx="205" cy="34" r="8" fill=${on(3)}/>
    </svg></div>`;
}

export function MarionetasNuevo({ m }) {
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:1.5 1 0;min-width:340px"><${TuMando} m=${m}/></div>
    <div class="col" style="flex:1 1 0;min-width:300px"><${PruebaMando}/><${MandosVirtuales} m=${m}/></div>
  </div>`;
}
