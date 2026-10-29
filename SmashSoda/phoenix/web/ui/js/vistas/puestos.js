// =============================================================================
//  PUESTOS (diseño aprobado 2026-10-08): «Jugando» (Equipo A / Equipo B) y «Mirando».
//  Cada puesto (mando virtual de Link, número fijo) pertenece al Equipo A, al B o está fuera
//  (acción mandos.equipo): el número del puesto nunca cambia, solo su equipo.
//  Se arrastra a cada persona al puesto que se quiera; lo que se suelta en
//  «Mirando» solo ve la partida. Usa las MISMAS acciones que la pantalla anterior
//  (mandos.asignar, mandos.intercambiar, mandos.liberar, mandos.tomar,
//  mandos.formacion, mandos.bloquear, mandos.conectar/desconectar,
//  mandos.herramienta): no hay nada nuevo en C++ ni en el simulador.
//  La versión anterior (Puestos/Puesto en mandos.js) se conserva sin usar.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar, avisar, oirVivo, irA } from "../tienda.js";
import { Boton, BotonIr, Icono, Vacio, Avatar, Chip, Stepper, Interruptor, colorPing, cx, dos, useDestello } from "../ui.js";
import { instalarVigem } from "./arreglos.js";
import { Pendientes, ElegirJugador } from "./mandos.js";
import { BotonJugar } from "./jugar.js";
import { IndicadorRetraso, retrasoDe } from "./retraso.js";
import { MandosEnVivo } from "./mandovivo.js";
import { leerSentado, ponerSentado } from "./sentado.js";

// ---- Mini-mando en vivo integrado en cada fila (Minimalismo Táctico 0.3.57) ----
const BITS_MINI = [
  [0x1000, "A", "✕"], [0x2000, "B", "◯"], [0x4000, "X", "□"], [0x8000, "Y", "△"],
  [0x100, "LB", "L1"], [0x200, "RB", "R1"], [0x10, "START", "OPTIONS"], [0x20, "BACK", "SHARE"],
  [0x40, "L3", "L3"], [0x80, "R3", "R3"], [0x1, "↑", "↑"], [0x2, "↓", "↓"], [0x4, "←", "←"], [0x8, "→", "→"],
];
function flechaStick(x, y) {
  const nx = (x || 0) / 32767, ny = (y || 0) / 32767;
  if (Math.hypot(nx, ny) < 0.28) return "";
  const ang = Math.atan2(ny, nx) * (180 / Math.PI);
  if (ang >= -22.5 && ang < 22.5) return "→";
  if (ang >= 22.5 && ang < 67.5) return "↗";
  if (ang >= 67.5 && ang < 112.5) return "↑";
  if (ang >= 112.5 && ang < 157.5) return "↖";
  if (ang >= 157.5 || ang < -157.5) return "←";
  if (ang >= -157.5 && ang < -112.5) return "↙";
  if (ang >= -112.5 && ang < -67.5) return "↓";
  return "↘";
}
function resumenEntrada(e, esPs) {
  if (!e) return { activo: false, sticks: [], botones: [] };
  const b = e.b || 0;
  const sticks = [];
  const fl = flechaStick(e.lx, e.ly), fr = flechaStick(e.rx, e.ry);
  if (fl) sticks.push("LX " + fl);
  if (fr) sticks.push("RX " + fr);
  if ((e.lt || 0) > 45) sticks.push(esPs ? "L2" : "LT");
  if ((e.rt || 0) > 45) sticks.push(esPs ? "R2" : "RT");
  const botones = [];
  for (const [bit, xb, ps] of BITS_MINI) {
    if (b & bit) botones.push(esPs ? ps : xb);
  }
  return { activo: sticks.length > 0 || botones.length > 0, sticks, botones: botones.slice(0, 3) };
}
function MiniMandoFila({ p, e, esHost }) {
  const esPs = p.tipo === "ds4";
  const r = resumenEntrada(e, esPs);
  const ir = (ev) => {
    ev.stopPropagation();
    irA("mandos", esHost ? "marionetas" : "mapeo");
  };
  return html`<button type="button" class=${cx("mini-pad", p.equipo === "b" && "b", !r.activo && "reposo")}
    title=${t("En vivo: pulsa para abrir el mapeo de este mando", "Live: click to open mapping for this pad")}
    onClick=${ir}>
    <span>🎮 ${r.activo ? (r.sticks.join(" ") || t("Activo", "Active")) : t("En reposo", "Idle")}</span>
    ${r.botones.map((k) => html`<span key=${k} class="k">${k}</span>`)}
  </button>`;
}

function Agarre() {
  return html`<svg width="12" height="20" viewBox="0 0 14 22" fill="currentColor" style="opacity:.55;flex:none" aria-hidden="true">
    <circle cx="3" cy="4" r="1.6"/><circle cx="11" cy="4" r="1.6"/><circle cx="3" cy="11" r="1.6"/>
    <circle cx="11" cy="11" r="1.6"/><circle cx="3" cy="18" r="1.6"/><circle cx="11" cy="18" r="1.6"/></svg>`;
}

// ---- Un puesto (fila) -------------------------------------------------------------------
function Fila({ p, m, color, arr, setArr, alAsignar, mirando, vivo }) {
  const [sobre, setSobre] = useState(false);
  const entradaViva = vivo?.[p.n] || p.entrada;
  const pulsando = !!(entradaViva && ((entradaViva.b || 0) !== 0 || (entradaViva.lt || 0) > 45 || (entradaViva.rt || 0) > 45 || Math.hypot(entradaViva.lx || 0, entradaViva.ly || 0) > 9000 || Math.hypot(entradaViva.rx || 0, entradaViva.ry || 0) > 9000));
  const esHost = m.mandos.host === p.n || (p.ocupado && p.parsecId && p.parsecId === m.sala?.hostId);
  const arrastrando = arr != null;
  const recibe = arrastrando && ((arr.tipo === "puesto" && arr.valor !== p.n - 1) || (arr.tipo === "persona" && !p.ocupado));
  const resaltado = recibe && sobre;
  const enPartido = m.partido?.fase === "en_juego" || m.partido?.fase === "pausado";
  const sentadosIds = enPartido && !p.ocupado ? new Set((m.mandos?.lista || []).filter((x) => x.ocupado && x.parsecId).map((x) => x.parsecId)) : null;
  const caidosEquipo = sentadosIds && (p.equipo === "a" || p.equipo === "b")
    ? (m.partido?.[p.equipo]?.jugadores || []).filter((j) => j.parsecId && !sentadosIds.has(j.parsecId))
    : [];
  const libresEquipo = caidosEquipo.length
    ? (m.mandos?.lista || []).filter((x) => !x.ocupado && x.equipo === p.equipo)
    : [];
  const idxLibre = caidosEquipo.length ? libresEquipo.findIndex((x) => x.n === p.n) : -1;
  const caido = idxLibre >= 0 && idxLibre < caidosEquipo.length ? caidosEquipo[idxLibre] : null;
  const candidatosRelevo = caido
    ? [...(mirando || [])].filter((g) => g.rolWeb !== "espectador").sort((x, y) => (y.parsecId === caido.parsecId ? 1 : 0) - (x.parsecId === caido.parsecId ? 1 : 0))
    : [];
  const borde = resaltado ? "var(--acc)" : recibe ? color : p.ocupado ? color : caido ? "var(--warn)" : "rgba(255,255,255,.22)";
  const fondo = resaltado ? "var(--acc-suave)" : p.ocupado ? "var(--panel-2)" : caido ? "rgba(245,158,11,.08)" : "rgba(0,0,0,.25)";
  const sombra = resaltado ? "0 0 0 4px var(--acc-suave),0 0 28px var(--acc-suave)" : "none";
  const opacidad = arrastrando && !recibe && arr.tipo !== "asiento" && arr.valor !== p.n - 1 ? ".55" : "1";
  const perfil = p.ocupado ? m.perfiles?.[p.parsecId] : null;
  const fijado = p.ocupado && p.parsecId ? m.mandos.fijados?.[p.parsecId] || 0 : 0;   // candado: puesto fijado de este jugador (0 = ninguno)
  const destello = useDestello(p.ocupado ? p.parsecId || p.jugador : 0);   // alguien se sentó o se fue: el puesto destella

  // Me siento en un puesto que ocupa un amigo: él pasa a MI puesto (se cruzan), nadie se queda sin mando
  const cruzarConAmigo = async (mio) => {
    const amigo = p.jugador || t("Tu amigo", "Your friend");
    if (!(await accion("mandos.tomar", { numero: 0 }, { silencioso: true }))) return;   // suelto mi puesto
    if (!(await accion("mandos.intercambiar", { a: p.n - 1, b: mio - 1 }, { silencioso: true }))) {
      await accion("mandos.tomar", { numero: mio }, { silencioso: true });              // no se pudo: vuelvo a mi puesto
      return;
    }
    if (await accion("mandos.tomar", { numero: p.n }, { silencioso: true })) {
      avisar(t(`Te sentaste en el ${dos(p.n)} y ${amigo} pasó al ${dos(mio)}.`, `You took seat ${dos(p.n)} and ${amigo} moved to ${dos(mio)}.`), "ok");
    }
  };

  const soltar = (e) => {
    e.preventDefault();
    e.stopPropagation();   // soltar sobre una fila intercambia personas; no debe contar como soltar sobre la columna del equipo
    setSobre(false);
    const a = arr;
    setArr(null);
    if (!a || !recibe) return;
    const hostId = m.sala?.hostId;
    const origen = a.tipo === "puesto" ? m.mandos.lista[a.valor] : null;
    const origenHost = origen && (m.mandos.host === origen.n || (origen.parsecId && origen.parsecId === hostId));
    if ((a.tipo === "persona" && a.valor === hostId) || origenHost) {
      // El anfitrión se mueve «tomando» el puesto (con su mando físico o el teclado)
      if (p.ocupado && !esHost) {
        const mio = m.mandos.host;   // el puesto donde estoy sentado (0 = ninguno)
        if (mio > 0 && mio !== p.n) return cruzarConAmigo(mio);
        return avisar(t("Ese puesto está ocupado: primero quita a quien está ahí.", "That seat is taken: remove that player first."), "warn");
      }
      return accion("mandos.tomar", { numero: p.n });
    }
    // Dos puestos de equipos distintos (ninguno es el tuyo): se cruzan los equipos; cada puesto conserva su número y su mando
    if (a.tipo === "puesto" && origen && !origenHost && !esHost && (origen.equipo === "a" || origen.equipo === "b") && (p.equipo === "a" || p.equipo === "b") && origen.equipo !== p.equipo) {
      return accion("mandos.equipoIntercambiar", { a: a.valor, b: p.n - 1 });
    }
    if (esHost) return avisar(t("Ese es tu puesto: muévete tú a otro, o arrástrate a «Mirando».", "That's your seat: move yourself or drag yourself to «Watching»."), "warn");
    if (a.tipo === "puesto") accion("mandos.intercambiar", { a: a.valor, b: p.n - 1 });
    else accion("mandos.asignar", { indice: p.n - 1, parsecId: a.valor });
  };

  return html`<div class=${destello ? "destello" : ""}
      draggable=${p.ocupado || (p.equipo === "a" || p.equipo === "b")}
      onDragStart=${(e) => { e.dataTransfer.setData("text/plain", "puesto:" + (p.n - 1)); e.dataTransfer.effectAllowed = "move"; setArr({ tipo: "puesto", valor: p.n - 1 }); }}
      onDragEnd=${() => { setArr(null); setSobre(false); }}
      onDragOver=${(e) => { if (recibe) { e.preventDefault(); setSobre(true); } }}
      onDragLeave=${() => setSobre(false)}
      onDrop=${soltar}
      style=${`display:flex;flex-wrap:wrap;align-items:center;gap:10px 14px;padding:12px 14px;min-height:76px;box-sizing:border-box;border-radius:10px;border:${p.ocupado ? "1px solid" : "1.5px dashed"} ${borde};background:${fondo};box-shadow:${sombra};opacity:${opacidad};cursor:${p.ocupado ? "grab" : "default"};transition:border-color .15s,background .15s,box-shadow .15s,opacity .15s`}>
    ${p.ocupado ? html`<span style="color:var(--mut)"><${Agarre}/></span>` : html`<span style="width:12px;flex:none"></span>`}
    <div style="width:56px;flex:none;display:flex;flex-direction:column;align-items:center;gap:3px"><div class="disp" style=${`font-weight:700;font-size:40px;line-height:1;color:${p.ocupado ? color : caido ? "var(--warn)" : "rgba(255,255,255,.22)"}`}>${dos(p.n)}</div>${p.extra ? html`<span class="mono" style="font-size:9px;line-height:1;letter-spacing:.12em;color:var(--warn);opacity:.9" title=${t("Puesto 9–16: necesita el módulo de mandos extra de Sider", "Seat 9–16: needs Sider's extra pads module")}>EXTRA</span>` : null}</div>
    ${p.ocupado
      ? html`<${Avatar} nombre=${p.jugador} id=${p.parsecId} url=${perfil?.avatar_url} t=${42} brillo=${pulsando}/>
        <div style="flex:1 1 110px;min-width:110px">
          <div style="font:600 16px/1.2 var(--f-body);color:#fff;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${p.jugador}
            ${esHost ? html` <${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}
            <span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:20px;margin-left:6px;font-size:11px" title=${p.tipo === "ds4" ? t("Este puesto es un mando de PlayStation (DualShock 4)", "This seat is a PlayStation pad (DualShock 4)") : t("Este puesto es un mando de Xbox", "This seat is an Xbox pad")}>${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span></div>
          <div class="mono mut" style="font-size:12px">
            ${esHost ? (m.mandos.miMando ? t("con tu mando · ", "with your pad · ") + m.mandos.miMando : t("con el teclado", "with the keyboard"))
              : html`${p.conectado ? t("conectado", "connected") : t("desconectado", "unplugged")}${p.bloqueado ? t(" · bloqueado", " · locked") : ""}`}
            ${p.ping >= 0 ? html` · <span style=${`color:${colorPing(p.ping)}`}>${p.ping} ms</span>` : null}${!esHost ? html`<${IndicadorRetraso} r=${retrasoDe(m, p.parsecId)}/>` : null}</div>
        </div>`
      : html`<div style="flex:1 1 110px;min-width:110px">
          <div style=${`font:600 17px/1.2 var(--f-body);color:${caido ? "var(--warn)" : "var(--mut)"}`}>${caido ? t(`Puesto libre · Se cayó ${caido.nombre}`, `Free seat · ${caido.nombre} dropped`) : t("Puesto libre", "Free seat")}
            ${p.tipo ? html`<span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:18px;margin-left:6px;font-size:10px;opacity:.8">${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span>` : null}</div>
          <div style="font:400 13px/1.3 var(--f-body);color:var(--mut)">${arrastrando && recibe ? t("Suelta aquí", "Drop here") : caido ? t("Pásale este mando a quien reconectó o a un suplente sin salir del partido", "Give this pad to whoever reconnected or a sub without leaving the match") : t("Arrastra a alguien aquí", "Drag someone here")}</div>
        </div>`}
    ${caido && candidatosRelevo.length ? html`<div style="flex-basis:100%;order:98;display:flex;align-items:center;gap:8px;flex-wrap:wrap;padding:8px 10px;border-radius:8px;background:rgba(245,158,11,.12);border:1px solid rgba(245,158,11,.35)">
      <span style="font:700 12px/1.2 var(--f-body);color:var(--warn)">${t(`⚡ Relevo en 1 clic (#${dos(p.n)}):`, `⚡ 1-click sub (#${dos(p.n)}):`)}</span>
      ${candidatosRelevo.slice(0, 4).map((g) => {
        const reconecto = g.parsecId === caido.parsecId;
        return html`<${Boton} key=${g.parsecId} tipo=${cx("mini", reconecto ? "lleno" : "suave")}
          estilo=${reconecto ? "" : "border-color:var(--warn);color:#fff"}
          al=${() => (g.host ? accion("mandos.tomar", { numero: p.n }) : accion("mandos.asignar", { indice: p.n - 1, parsecId: g.parsecId }))}>
          ⚡ ${g.nombre} ${reconecto ? t("(reconectó)", "(reconnected)") : t("(mirando)", "(watching)")}</${Boton}>`;
      })}
      <span class="mono mut" style="margin-left:auto;font-size:11px" title=${t("Atajo desde tu mando físico sin salir de PES", "Shortcut from your physical pad without leaving PES")}>${t("Mando 1: L3+R3+Select", "Pad 1: L3+R3+Select")}</span>
    </div>` : null}
    ${p.ocupado && !esHost ? html`<button class="icono-btn" style="height:38px;width:38px;flex:none" data-tip=${p.bloqueado ? t("Desbloquear: el jugador vuelve a mover su mando", "Unlock: the player can move again") : t("Bloquear: el jugador no puede mover este mando (sigue en su puesto)", "Lock: the player can't move this pad (keeps the seat)")}
        onClick=${() => accion("mandos.bloquear", { indice: p.n - 1 })}><span style=${p.bloqueado ? "color:var(--warn)" : ""}><${Icono} n="candado" t=${17}/></span></button>` : null}
    ${p.ocupado && !esHost && p.parsecId ? html`<button class="icono-btn" style=${`height:38px;width:38px;flex:none;${fijado === p.n ? "border-color:var(--acc)" : ""}`}
        data-tip=${fijado === p.n ? t(`Puesto fijado: ${dos(p.n)}. Al volver, ${p.jugador} cae aquí (o en el siguiente libre). Clic para soltarlo.`, `Seat pinned: ${dos(p.n)}. When ${p.jugador} returns they land here (or on the next free one). Click to unpin.`)
          : fijado ? t(`Tiene fijado el ${dos(fijado)}. Clic para fijarlo aquí (${dos(p.n)}).`, `Pinned to ${dos(fijado)}. Click to pin here (${dos(p.n)}).`)
          : t(`Fijar este puesto: al volver, ${p.jugador} cae en el ${dos(p.n)} (o en el siguiente libre).`, `Pin this seat: when ${p.jugador} returns they land on ${dos(p.n)} (or the next free one).`)}
        onClick=${() => accion("mandos.fijarPuesto", { parsecId: p.parsecId, puesto: fijado === p.n ? 0 : p.n })}><span style=${fijado ? "color:var(--acc)" : ""}><${Icono} n="pin" t=${17}/></span></button>` : null}
    ${p.equipo === "a" || p.equipo === "b"
      ? html`<button class="icono-btn btn-sutil" style="height:34px;width:34px;flex:none;background:none;border-color:transparent" disabled=${!!m.mandos.esclavo || (m.mandos.formacion?.[p.equipo] ?? 2) <= 1}
          data-tip=${(m.mandos.formacion?.[p.equipo] ?? 2) <= 1 ? t("Es el único puesto de su equipo", "It's the only seat on its team")
            : (p.equipo === "a" ? t("Pasar este puesto al Equipo B", "Move this seat to Team B") : t("Pasar este puesto al Equipo A", "Move this seat to Team A")) + t(" (con su mando y su número). También puedes arrastrar la fila.", " (with its pad and number). You can also drag the row.")}
          onClick=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: p.equipo === "a" ? "b" : "a" })}><${Icono} n="intercambio" t=${16}/></button>`
      : html`<${Boton} tipo="mini suave" estilo="flex:none" deshabilitado=${!!m.mandos.esclavo} titulo=${t("Meter este puesto en el Equipo A", "Put this seat on Team A")} al=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: "a" })}>A</${Boton}>
        <${Boton} tipo="mini suave" estilo="flex:none" deshabilitado=${!!m.mandos.esclavo} titulo=${t("Meter este puesto en el Equipo B", "Put this seat on Team B")} al=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: "b" })}>B</${Boton}>`}
    ${p.ocupado
      ? html`<${Boton} tipo="peligro mini" estilo="flex:none" titulo=${t("Sale del puesto y pasa a mirar", "Leaves the seat and watches")} al=${() => esHost ? accion("mandos.tomar", { numero: 0 }) : accion("mandos.liberar", { indice: p.n - 1 })}>${t("QUITAR", "REMOVE")}</${Boton}>`
      : html`<${Boton} tipo="mini" estilo=${`flex:none;border-color:${color};color:${color}`} al=${() => alAsignar(p.n - 1)}>${t("ASIGNAR", "ASSIGN")}</${Boton}>`}
  </div>`;
}

// ---- Un equipo (Local o Visita) -----------------------------------------------------------
function Equipo({ equipo, nombre, desc, tip, color, filas, m, arr, setArr, alAsignar, mirando, vivo, puedeMas, puedeMenos, mas, menos, esclavo }) {
  const n = filas.length;
  const [sobre, setSobre] = useState(false);
  // Soltar en el hueco de un equipo (no sobre una fila) un puesto arrastrado del otro equipo: el puesto pasa a este equipo
  // con su mando y su número; quien esté sentado ahí se va con él.
  const origenAsiento = arr && arr.tipo === "puesto" ? m.mandos.lista?.[arr.valor] : null;
  const recibe = !!origenAsiento && origenAsiento.equipo !== equipo && !esclavo;
  const soltar = (e) => {
    e.preventDefault();
    setSobre(false);
    const a = arr;
    setArr(null);
    if (a && a.tipo === "puesto" && recibe) accion("mandos.equipo", { indice: a.valor, equipo });
  };
  return html`<div class="cut" style=${`background:linear-gradient(135deg,${color},rgba(255,255,255,.04) 70%)`}>
    <div class="in" style=${`padding:18px 20px;display:flex;flex-direction:column;gap:10px;height:100%;box-sizing:border-box;outline:${recibe ? `2px dashed ${sobre ? "var(--acc)" : color}` : "2px solid transparent"};outline-offset:-6px;transition:outline-color .15s`}
      onDragOver=${(e) => { if (recibe) { e.preventDefault(); setSobre(true); } }}
      onDragLeave=${() => setSobre(false)}
      onDrop=${soltar}>
      ${recibe ? html`<div style=${`text-align:center;font:700 14px/1.3 var(--f-body);color:${sobre ? "var(--acc)" : color};padding:10px;border:1.5px dashed ${color};border-radius:10px`}>
        ${t(`Suelta aquí: el puesto ${dos(origenAsiento.n)} pasa a ${nombre}`, `Drop here: seat ${dos(origenAsiento.n)} moves to ${nombre}`)}</div>` : null}
      <div style="display:flex;align-items:center;justify-content:space-between;gap:10px">
        <div style="display:flex;align-items:baseline;gap:8px;min-width:0;flex-wrap:wrap">
          <span class="disp" style=${`font-weight:700;font-size:21px;letter-spacing:.08em;color:${color}`}>${nombre}</span>
          <span class="ayuda" style="font-size:12.5px">${desc}${tip ? html`<span class="help-dot" title=${tip}>?</span>` : null}</span>
        </div>
        <span class="mono" style=${`font-size:12px;color:${color};background:rgba(255,255,255,.07);padding:5px 10px;border-radius:999px;white-space:nowrap`}>${n} ${n === 1 ? t("PUESTO", "SEAT") : t("PUESTOS", "SEATS")}</span>
      </div>
      ${filas.map((p) => html`<${Fila} key=${p.n} p=${p} m=${m} color=${color} arr=${arr} setArr=${setArr} alAsignar=${alAsignar} mirando=${mirando} vivo=${vivo}/>`)}
      <div style="margin-top:auto;display:flex;gap:10px;padding-top:4px">
        <${Boton} estilo=${`flex:1;border-color:${color};color:${color}`} deshabilitado=${!puedeMas || esclavo} al=${mas}>${t("+ AÑADIR PUESTO", "+ ADD SEAT")}</${Boton}>
        <${Boton} tipo="suave" deshabilitado=${!puedeMenos || esclavo} al=${menos}>${t("− QUITAR UNO", "− REMOVE ONE")}</${Boton}>
      </div>
    </div>
  </div>`;
}

// ---- Persona que mira ---------------------------------------------------------------------
// Una sola línea por persona (caben muchas): agarre, avatar pequeño, nombre con su dato al lado
// (ping, «web» o tu mando) y un selector corto de puesto.
function Espectador({ g, m, arr, setArr }) {
  const libres = (m.mandos?.lista || []).filter((p) => !p.ocupado);
  const soloWeb = g.rolWeb === "espectador";
  const perfil = m.perfiles?.[g.parsecId];
  const moviendo = arr && arr.tipo === "persona" && arr.valor === g.parsecId;
  if (moviendo) {
    return html`<div style="height:40px;box-sizing:border-box;border:1.5px dashed rgba(255,255,255,.22);border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:12px;color:var(--mut)">
      ${g.nombre} ${t("se está moviendo…", "is moving…")}</div>`;
  }
  const sinSala = false;   // ya se puede tomar un puesto con la sala cerrada
  const dato = g.host
    ? (m.mandos.miMando ? t("tu mando", "your pad") : t("teclado", "keyboard"))
    : soloWeb ? t("web", "web")
    : g.ping >= 0 ? html`<span style=${`color:${colorPing(g.ping)}`}>${g.ping} ms</span>` : "—";
  const titulo = g.host
    ? (m.mandos.miMando ? t("Tú · tu mando: ", "You · your pad: ") + m.mandos.miMando : t("Tú · jugarás con el teclado", "You · keyboard"))
    : soloWeb ? t("Solo espectador en la web", "Web spectator only") : t("Espectador por Parsec", "Parsec spectator");
  return html`<div draggable=${!soloWeb} title=${titulo}
      onDragStart=${(e) => { e.dataTransfer.setData("text/plain", "persona:" + g.parsecId); e.dataTransfer.effectAllowed = "move"; setArr({ tipo: "persona", valor: g.parsecId }); }}
      onDragEnd=${() => setArr(null)}
      style=${`display:flex;align-items:center;gap:8px;min-height:40px;padding:4px 8px;border-radius:8px;border:1px solid ${g.host ? "var(--linea)" : "rgba(255,255,255,.08)"};background:${g.host ? "var(--acc-suave)" : "rgba(255,255,255,.03)"};cursor:${soloWeb ? "default" : "grab"};opacity:${soloWeb ? ".7" : "1"}`}>
    ${soloWeb ? html`<span style="width:12px;flex:none"></span>` : html`<span style="color:var(--mut);flex:none"><${Agarre}/></span>`}
    <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${perfil?.avatar_url} t=${26}/>
    <div style="flex:1;min-width:0;display:flex;align-items:baseline;gap:6px;white-space:nowrap;overflow:hidden">
      <span style="font:600 14px/1.2 var(--f-body);color:#fff;overflow:hidden;text-overflow:ellipsis">${g.nombre}</span>
      ${g.host ? html`<span class="chip acc" style="height:16px;font-size:10px;flex:none">${t("TÚ", "YOU")}</span>` : null}
      ${m.mandos?.fijados?.[g.parsecId] ? html`<span class="chip mod" style="height:16px;font-size:10px;flex:none" title=${t("Tiene el puesto fijado: al volver cae ahí (o en el siguiente libre)", "Seat pinned: when they return they land there (or on the next free one)")}>${t("PUESTO", "SEAT")} ${dos(m.mandos.fijados[g.parsecId])}</span>` : null}
      <span class="mono mut" style="font-size:11px;flex:none">${dato}</span>
    </div>
    ${g.host ? html`<${BotonJugar} m=${m}/>` : soloWeb ? null : html`<select class="campo" style="flex:none;width:auto;max-width:112px;height:28px;padding:0 24px 0 8px;font-size:11px"
      disabled=${libres.length === 0 || m.mandos?.esclavo || sinSala}
      title=${sinSala ? t("Abre la sala para tomar un puesto", "Open the room to take a seat") : libres.length ? "" : t("No hay puestos libres", "No free seats")}
      aria-label=${(g.host ? t("Ponerme en un puesto", "Take a seat") : t("Dar puesto a ", "Give a seat to ") + g.nombre)}
      onChange=${(e) => { const v = Number(e.currentTarget.value); e.currentTarget.value = ""; if (!v) return; if (g.host) accion("mandos.tomar", { numero: v }); else accion("mandos.asignar", { indice: v - 1, parsecId: g.parsecId }); }}>
      <option value="">${sinSala ? t("SALA CERRADA", "ROOM CLOSED") : libres.length ? t("PUESTO…", "SEAT…") : t("LLENO", "FULL")}</option>
      ${libres.map((p) => html`<option value=${p.n}>${dos(p.n)} · ${p.equipo === "a" ? t("Equipo A", "Team A") : p.equipo === "b" ? t("Equipo B", "Team B") : t("Fuera", "Off")}</option>`)}
    </select>`}
  </div>`;
}

// ---- Barra rápida de partido en Mandos › Puestos (Propuesta D) -----------------------------
function BarraPartidoEnSala({ m }) {
  const tieneLiga = (m.liga?.elegido || 0) > 0;
  const tieneTorneo = !!(m.torneos?.elegidoId && m.torneos?.elegidoClave);
  const [modo, setModo] = useState(() => (tieneLiga ? "liga" : tieneTorneo ? "torneo" : "amistoso"));

  useEffect(() => {
    if (!m.sala?.abierta || !m.web?.vinculada) return;
    if (!m.liga?.cargado) accion("partido.oficiales", {}, { silencioso: true });
    if (!m.torneos?.cargado) accion("partido.torneos", {}, { silencioso: true });
  }, [m.sala?.abierta, m.web?.vinculada]);

  if (!m.sala?.abierta) return null;
  const modoActual = tieneLiga ? "liga" : tieneTorneo ? "torneo" : modo;
  const partidosLiga = m.liga?.partidos || [];
  const partidosTorneo = m.torneos?.partidos || [];
  const duelo = m.duelo;

  const elegirModo = (nuevo) => {
    setModo(nuevo);
    if (nuevo === "amistoso") {
      if (tieneLiga) accion("partido.oficial", { id: 0 });
      if (tieneTorneo) accion("partido.torneo", { torneoId: 0, clave: "" });
    } else if (nuevo === "liga") {
      if (tieneTorneo) accion("partido.torneo", { torneoId: 0, clave: "" });
      if (!m.liga?.cargado) accion("partido.oficiales", {}, { silencioso: true });
    } else if (nuevo === "torneo") {
      if (tieneLiga) accion("partido.oficial", { id: 0 });
      if (!m.torneos?.cargado) accion("partido.torneos", {}, { silencioso: true });
    }
  };

  return html`<div class="caja" style="padding:10px 14px;display:flex;align-items:center;gap:10px;flex-wrap:wrap">
    <span class="disp" style="font-weight:700;font-size:12px;letter-spacing:.14em;color:var(--mut)">${t("PARTIDO EN SALA", "ROOM MATCH")}</span>
    <div style="display:flex;gap:6px;flex-wrap:wrap">
      <${Boton} tipo=${cx("mini", modoActual === "amistoso" ? "acc" : "suave")} al=${() => elegirModo("amistoso")}>${t("AMISTOSO", "FRIENDLY")}</${Boton}>
      <${Boton} tipo=${cx("mini", modoActual === "liga" ? "acc" : "suave")} deshabilitado=${!m.web?.vinculada}
        titulo=${!m.web?.vinculada ? t("Vincula tu cuenta en Inicio para elegir partidos oficiales", "Link your account in Home to pick official matches") : ""}
        al=${() => elegirModo("liga")}>${t("LIGA OFICIAL", "OFFICIAL LEAGUE")}</${Boton}>
      <${Boton} tipo=${cx("mini", modoActual === "torneo" ? "acc" : "suave")} deshabilitado=${!m.web?.vinculada}
        titulo=${!m.web?.vinculada ? t("Vincula tu cuenta en Inicio para elegir cruces de torneo", "Link your account in Home to pick tournament matches") : ""}
        al=${() => elegirModo("torneo")}>${t("TORNEO", "TOURNAMENT")}</${Boton}>
    </div>
    ${modoActual === "liga" ? html`
      <select class="campo" style="flex:1;min-width:210px;max-width:360px;height:32px;padding:0 28px 0 10px;font-size:12px"
        value=${String(m.liga?.elegido || 0)}
        onChange=${(e) => accion("partido.oficial", { id: Number(e.currentTarget.value) || 0 })}>
        <option value="0">${partidosLiga.length ? t("Elegir partido de Liga Oficial…", "Pick Official League match…") : t("Sin partidos pendientes en Liga", "No pending League matches")}</option>
        ${partidosLiga.map((x) => html`<option value=${String(x.id)}>${x.mio ? "★ " : ""}${t("Fecha", "Round")} ${x.fecha} · ${x.local} vs ${x.visitante}</option>`)}
      </select>
      ${tieneLiga ? html`<span class="chip ok" style="height:22px;font-size:11px">✓ ${t("VINCULADO AL ACTA", "LINKED TO REPORT")}</span>` : null}
    ` : modoActual === "torneo" ? html`
      <select class="campo" style="flex:1;min-width:210px;max-width:360px;height:32px;padding:0 28px 0 10px;font-size:12px"
        value=${tieneTorneo ? `${m.torneos.elegidoId}:${m.torneos.elegidoClave}` : ""}
        onChange=${(e) => {
          const v = e.currentTarget.value;
          if (!v) return accion("partido.torneo", { torneoId: 0, clave: "" });
          const [tid, ... resto] = v.split(":");
          return accion("partido.torneo", { torneoId: Number(tid) || 0, clave: resto.join(":") });
        }}>
        <option value="">${partidosTorneo.length ? t("Elegir cruce de Torneo…", "Pick Tournament match…") : t("Sin cruces pendientes de Torneo", "No pending Tournament matches")}</option>
        ${partidosTorneo.map((x) => html`<option value=${`${x.torneoId}:${x.clave}`}>${x.mio ? "★ " : ""}${x.torneo} · ${x.ronda} · ${x.a} vs ${x.b}</option>`)}
      </select>
      ${tieneTorneo ? html`<span class="chip ok" style="height:22px;font-size:11px">✓ ${t("VINCULADO AL TORNEO", "LINKED TO TOURNAMENT")}</span>` : null}
    ` : duelo?.retoId > 0 ? html`
      <span class="chip acc" style="height:22px;font-size:11px">⚔ ${t("DUELO WEB", "WEB DUEL")}: ${duelo.titulo || "#" + duelo.retoId}</span>
    ` : html`<span class="ayuda" style="font-size:12px">${t("El marcador arranca solo al pitazo inicial según los puestos de A y B.", "Scoreboard starts automatically at kick-off from Team A and B seats.")}</span>`}
    <div style="margin-left:auto;display:flex;align-items:center;gap:8px">
      <${Boton} tipo="mini suave" al=${() => accion("sala.copiarEnlace")}>📋 ${t("COPIAR ENLACE", "COPY LINK")}</${Boton}>
    </div>
  </div>`;
}

// ---- Pantalla ---------------------------------------------------------------------------------
export function PuestosNuevo({ m }) {
  const [eligiendo, setEligiendo] = useState(null);
  const [arr, setArr] = useState(null);
  const [sobreMirando, setSobreMirando] = useState(false);
  const [ayudaAB, setAyudaAB] = useState(false);
  const [prefSentado, setPrefSentado] = useState(() => leerSentado());
  // Mandos en vivo: cada fila ocupada ya muestra su pastilla en vivo (~30 Hz); la rejilla grande 2D se abre si el anfitrión la despliega
  const [verVivo, setVerVivoEstado] = useState(() => { try { return localStorage.getItem("phoenix.verVivo2D") === "1"; } catch { return false; } });
  const setVerVivo = (v) => { setVerVivoEstado(v); try { localStorage.setItem("phoenix.verVivo2D", v ? "1" : "0"); } catch { /* sin almacenamiento */ } };
  const [vivo, setVivo] = useState({});
  useEffect(() => {
    accion("mandos.vivo", { si: true }, { silencioso: true });
    const quitar = oirVivo((d) => {
      const n = {};
      for (const [num, b, lt, rt, lx, ly, rx, ry] of d || []) n[num] = { b, lt, rt, lx, ly, rx, ry };
      setVivo(n);
    });
    return () => { quitar(); accion("mandos.vivo", { si: false }, { silencioso: true }); };
  }, []);
  const rotarSentado = () => {
    const p = leerSentado();
    const sig = !p.auto ? { auto: true, soloAmigos: false } : !p.soloAmigos ? { auto: true, soloAmigos: true } : { auto: false, soloAmigos: false };
    ponerSentado(sig);
    setPrefSentado(leerSentado());
  };
  const aplicarFormacion = async (k) => {
    const necesarios = k * 2;
    if ((m.mandos.puestosMax ?? 8) < necesarios) {
      await accion("mandos.puestosMax", { n: Math.max(8, necesarios) }, { silencioso: true });
    }
    const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0;
    if (xbox + ds4 < necesarios) {
      await accion("mandos.cantidad", { xbox: necesarios - ds4, ds4 }, { silencioso: true });
    }
    if (k <= 7) {
      return accion("mandos.formacion", { local: k, visitante: k });
    }
    // Para 8v8 (16 puestos): primero ponemos 7v7 de base y luego asignamos el 8.º de A y el 8.º de B con mandos.equipo,
    // así funciona tanto en binarios con tope 7 como en el nuevo con tope 15 sin dar error «visitante debe estar entre 1 y 7».
    await accion("mandos.formacion", { local: 7, visitante: 7 }, { silencioso: true });
    await accion("mandos.equipo", { indice: 7, equipo: "a" }, { silencioso: true });
    await accion("mandos.equipo", { indice: 15, equipo: "b" }, { silencioso: true });
    for (let i = 0; i < 8; i++) await accion("mandos.equipo", { indice: i, equipo: "a" }, { silencioso: true });
    for (let i = 8; i < 16; i++) await accion("mandos.equipo", { indice: i, equipo: "b" }, { silencioso: true });
  };
  const lista = m.mandos.lista || [];
  const total = lista.length;
  const esclavo = !!m.mandos.esclavo;

  if (total === 0) {
    return html`<${Vacio} icono="mandos" titulo=${t("No hay mandos virtuales", "No virtual pads")} texto=${t("Falta ViGEmBus, el controlador que crea los mandos de tus amigos. Sin él nadie puede jugar.", "ViGEmBus is missing, the driver that creates your friends' pads. Nobody can play without it.")}>
      <div style="display:flex;gap:10px;justify-content:center;flex-wrap:wrap">
        <${Boton} tipo="lleno" al=${instalarVigem}>${t("INSTALAR VIGEMBUS", "INSTALL VIGEMBUS")}</${Boton}>
        <${BotonIr} tipo="suave" ir=${["sala", "red"]}>${t("VER CONEXIÓN", "SEE CONNECTION")}</${BotonIr}>
      </div></${Vacio}>`;
  }

  const equipoA = lista.filter((p) => p.equipo === "a");
  const equipoB = lista.filter((p) => p.equipo === "b");
  // Puestos que quedaron sin equipo pero tienen a alguien: se muestran para que nadie quede oculto.
  const sinEquipo = lista.filter((p) => p.equipo !== "a" && p.equipo !== "b" && p.ocupado);
  // Puestos sin usar (fuera de los dos equipos y libres): se pueden meter en el equipo que se quiera, con su número de siempre
  const sinUsar = lista.filter((p) => p.equipo !== "a" && p.equipo !== "b" && !p.ocupado);
  const sentados = new Set(lista.filter((p) => p.ocupado && p.parsecId).map((p) => p.parsecId));
  const hostSentado = !!m.mandos.host;
  const anfitrion = m.sala?.hostId && !hostSentado ? [{ parsecId: m.sala.hostId, nombre: m.sala.hostNombre || t("Anfitrión", "Host"), host: true, ping: -1 }] : [];
  const mirando = [...anfitrion, ...(m.invitados || []).filter((g) => !g.falso && !sentados.has(g.parsecId))];

  // El equipo es del PUESTO: se cambia el equipo de un puesto, nunca su número ni su mando
  const ponerEquipo = (p, equipo) => accion("mandos.equipo", { indice: p.n - 1, equipo });
  const masDe = (equipo) => { const libre = sinUsar[0]; if (libre) ponerEquipo(libre, equipo); };
  const menosDe = async (filas) => {
    // Quita el último puesto libre de ese equipo; si todos están ocupados, pregunta (la persona sigue sentada, sin equipo)
    const p = [...filas].reverse().find((x) => !x.ocupado) || filas[filas.length - 1];
    if (!p) return;
    if (p.ocupado && !(await confirmar({ titulo: t(`¿Quitar el puesto ${dos(p.n)} del equipo?`, `Remove seat ${dos(p.n)} from the team?`), texto: t(`${p.jugador} sigue sentado, pero sin equipo: no cuenta en el partido hasta que lo pases a un equipo.`, `${p.jugador} stays seated without a team: not counted in the match until moved to a team.`), si: t("Quitar", "Remove"), peligro: true }))) return;
    return ponerEquipo(p, "fuera");
  };
  const herramienta = async (nombre, pregunta) => {
    if (pregunta && !(await confirmar({ titulo: pregunta[0], texto: pregunta[1], si: t("Sí", "Yes"), peligro: true }))) return;
    return accion("mandos.herramienta", { nombre });
  };

  const sentarEspectadores = async () => {
    const libresEquipos = lista.filter((p) => (p.equipo === "a" || p.equipo === "b") && !p.ocupado);
    const libresFuera = lista.filter((p) => p.equipo !== "a" && p.equipo !== "b" && !p.ocupado);
    const libres = [...libresEquipos, ...libresFuera];

    if (!mirando.length) {
      return avisar(t("No hay nadie en Mirando.", "Nobody is watching."), "warn");
    }
    if (!libres.length) {
      return avisar(t("No hay puestos libres disponibles.", "No free seats available."), "warn");
    }

    const aSentar = Math.min(mirando.length, libres.length);
    for (let i = 0; i < aSentar; i++) {
      const g = mirando[i];
      const p = libres[i];
      if (g.host) {
        await accion("mandos.tomar", { numero: p.n }, { silencioso: true });
      } else {
        await accion("mandos.asignar", { indice: p.n - 1, parsecId: g.parsecId }, { silencioso: true });
      }
    }
    avisar(t(`Se sentaron ${aSentar} espectadores en puestos libres.`, `Seated ${aSentar} spectators into free seats.`), "ok");
  };

  const variarEquipos = async () => {
    const asignados = lista.filter((p) => p.equipo === "a" || p.equipo === "b");
    const objetivos = asignados.length > 0
      ? asignados
      : lista.slice(0, Math.min(lista.length, (m.mandos?.puestosMax ?? 8)));

    if (objetivos.length === 0) return;

    const yaParesEnA = objetivos.every((p) => {
      const esPar = p.n % 2 === 0;
      return esPar ? p.equipo === "a" : p.equipo === "b";
    });

    const destinoPar = yaParesEnA ? "b" : "a";
    const destinoImpar = yaParesEnA ? "a" : "b";

    for (const p of objetivos) {
      const esPar = p.n % 2 === 0;
      const nuevo = esPar ? destinoPar : destinoImpar;
      if (p.equipo !== nuevo) {
        await accion("mandos.equipo", { indice: p.n - 1, equipo: nuevo }, { silencioso: true });
      }
    }

    const mensaje = yaParesEnA
      ? t("Variado: impares a Equipo A y pares a Equipo B.", "Varied: odds to Team A and evens to Team B.")
      : t("Variado: pares a Equipo A e impares a Equipo B.", "Varied: evens to Team A and odds to Team B.");
    avisar(mensaje, "ok");
  };

  const sueltaEnMirando = (e) => {
    e.preventDefault();
    setSobreMirando(false);
    const a = arr;
    setArr(null);
    if (!a || a.tipo !== "puesto") return;
    const p = lista[a.valor];
    if (!p) return;
    const esHost = m.mandos.host === p.n || (p.parsecId && p.parsecId === m.sala?.hostId);
    if (esHost) accion("mandos.tomar", { numero: 0 });
    else accion("mandos.liberar", { indice: a.valor });
  };
  const recibeMirando = arr && arr.tipo === "puesto";

  return html`<div class="col" style="min-height:100%">
    <${Pendientes} m=${m}/>
    ${esclavo ? html`<div class="caja" style="border-color:var(--warn)"><b class="warn">${t("Turnos o torneo controlan los mandos.", "Hotseat or tournament controls the pads.")}</b>
      <span class="ayuda"> ${t("Las herramientas generales están en pausa mientras tanto.", "General tools are paused meanwhile.")}</span></div>` : null}

    <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap">
      <div style="display:flex;align-items:center;gap:6px;flex-wrap:wrap">
        <span class="disp" style="font-weight:700;font-size:12px;letter-spacing:.14em;color:var(--mut);margin-right:4px">${t("FORMACIÓN:", "FORMATION:")}</span>
        ${[1, 2, 3, 4, 8].map((k) => {
          const es = equipoA.length === k && equipoB.length === k;
          return html`<${Boton} key=${k} tipo=${cx("mini", es ? "acc" : "suave")} deshabilitado=${esclavo}
            titulo=${t(`Poner ${k} puestos en Equipo A y ${k} en Equipo B`, `Set ${k} seats on Team A and ${k} on Team B`)}
            al=${() => aplicarFormacion(k)}>${k}V${k}</${Boton}>`;
        })}
      </div>
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <${Boton} tipo=${cx("mini", prefSentado.auto ? "acc" : "suave")} al=${rotarSentado}
          titulo=${t("Clic para alternar quién se sienta solo al pulsar un botón de su mando estando en Mirando (Todos / Solo amigos / Apagado)", "Click to cycle who takes a free seat automatically when pressing their pad while Watching (Everyone / Friends only / Off)")}>
          ⚡ ${prefSentado.auto ? (prefSentado.soloAmigos ? t("SENTADO AUTO: AMIGOS", "AUTO-SEAT: FRIENDS") : t("SENTADO AUTO: TODOS", "AUTO-SEAT: ALL")) : t("SENTADO AUTO: NO", "AUTO-SEAT: OFF")}</${Boton}>
        <${Boton} tipo=${cx("mini", m.arbitro?.soloAutorizados ? "acc" : "suave")}
          titulo=${t("Encendido: pulsar el mando solo da puesto a quien tú ya sentaste. Los demás aparecen como «quiere jugar» y decides tú.", "On: pressing a pad only seats people you already seated. Others show up as «wants to play» and you decide.")}
          al=${() => accion("mandos.soloAutorizados", { si: !m.arbitro?.soloAutorizados })}>
          ${m.arbitro?.soloAutorizados ? t("PUESTOS: SOLO AUTORIZADOS", "SEATS: AUTHORIZED ONLY") : t("PUESTOS: LIBRES", "SEATS: OPEN")}</${Boton}>
        <${Boton} tipo=${cx("mini", verVivo ? "acc" : "suave")} al=${() => setVerVivo(!verVivo)}
          titulo=${t("Abre o cierra el panel grande con el dibujo 2D completo de cada mando en vivo.", "Opens or closes the full 2D live pad drawings panel.")}>
          ${verVivo ? t("▾ PLEGAR MANDOS 2D", "▾ HIDE 2D PADS") : t("▸ MANDOS 2D", "▸ 2D PADS")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando} al=${() => herramienta("ordenar")}>${t("ORDENAR", "SORT")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando}
          al=${() => herramienta("reiniciar", [t("¿Reiniciar los mandos?", "Reset pads?"), t("Se desconectan y vuelven a conectar todos los mandos virtuales. Úsalo si el juego dejó de detectarlos.", "All virtual pads are re-plugged. Use it if the game stopped detecting them.")])}>
          ${m.mandos.reiniciando ? t("REINICIANDO…", "RESETTING…") : t("REINICIAR", "RESET")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo} al=${variarEquipos}
          titulo=${t("Pone en el Equipo A los mandos pares y en el Equipo B los mandos impares (o los alterna si ya están variados)", "Puts even pads on Team A and odd pads on Team B (or alternates them if already varied)")}>
          ⚡ ${t("VARIAR", "VARY")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || mirando.length === 0} al=${sentarEspectadores}
          titulo=${t("Sienta a todos los espectadores de la sala en los puestos libres disponibles", "Seats all room spectators into available free seats")}>
          ⚡ ${t("SENTAR ESPECTADORES", "SEAT SPECTATORS")}</${Boton}>
        <${TodosAMirar} m=${m} esclavo=${esclavo}/>
      </div>
    </div>

    <${BarraPartidoEnSala} m=${m}/>

    <div style="display:grid;grid-template-columns:minmax(0,1fr) 320px;gap:20px;flex:1;align-items:stretch">
      <div style="display:flex;flex-direction:column;gap:14px;min-width:0">
        <div style="display:flex;align-items:center;gap:12px">
          <span class="disp" style="font-weight:700;font-size:15px;letter-spacing:.2em">${t("JUGANDO", "PLAYING")}</span>
          ${lista.some((p) => p.extra && (p.equipo === "a" || p.equipo === "b"))
            ? html`<span class="chip vip" style="height:20px;font-size:10px" title=${t("Los puestos 09 al 16 usan phoenix_mandos_extra.dat y requieren el módulo de mandos extra de Sider", "Seats 09–16 use phoenix_mandos_extra.dat and require Sider's extra pads module")}>${t("MANDOS 9–16 · MÓDULO SIDER", "PADS 9–16 · SIDER MODULE")}</span>`
            : null}
          <span style="flex:1;height:1px;background:linear-gradient(90deg,var(--linea-fuerte),transparent)"></span>
          <button class="mono mut" style="background:none;border:0;cursor:pointer;font-size:12px" onClick=${() => setAyudaAB(!ayudaAB)}>${ayudaAB ? "▾" : "▸"} ${t("¿QUÉ SON A Y B?", "WHAT ARE A AND B?")}</button>
        </div>
        ${ayudaAB ? html`<div class="ayuda" style="font-size:13px">${t(
          "Equipo A y Equipo B son los equipos de Link: no tienen por qué coincidir con local y visita del juego (A = el que estaba a la izquierda en el saque, según el Campo 2D). El número del puesto es el mando de Link y no cambia: con «Orden de mandos en PES» el puesto N es el jugador N. Para pasar un puesto de equipo arrastra su fila al hueco del otro equipo (o usa el botoncito ⇄ de la fila): el puesto se va con su mando y su número. Si la sueltas sobre una fila del otro equipo, los dos puestos se cruzan. La chincheta fija el puesto de un jugador: al volver cae ahí (o en el siguiente libre). Por ejemplo, tú en el 01 siendo del Equipo B.",
          "Team A and Team B are Link's teams: they don't have to match the game's home and away (A = the one on the left at kick-off, per the 2D Pitch). The seat number is Link's pad and never changes: with «Pad order in PES», seat N is player N. To move a seat to the other team drag its row onto the other team's empty area (or use the small ⇄ button on the row): the seat goes with its pad and number. Dropped on a row of the other team, the two seats swap. The pin fixes a player's seat: when they return they land there (or on the next free one). For example, you on 01 as Team B.")}</div>` : null}
        <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(380px,1fr));gap:16px;align-items:stretch">
          <${Equipo} equipo="a" nombre=${t("EQUIPO A", "TEAM A")} desc=${t("Izquierda en el saque", "Left side at kick-off")}
            tip=${t("Equipo A es el que empieza a la izquierda en el saque según el Campo 2D. El número del puesto es el mando de Link.", "Team A starts on the left at kick-off per the 2D Pitch. The seat number is Link's pad.")}
            color="var(--acc)" filas=${equipoA}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} mirando=${mirando} vivo=${vivo} esclavo=${esclavo}
            puedeMas=${sinUsar.length > 0} puedeMenos=${equipoA.length > 1}
            mas=${() => masDe("a")} menos=${() => menosDe(equipoA)}/>
          <${Equipo} equipo="b" nombre=${t("EQUIPO B", "TEAM B")} desc=${t("Derecha en el saque", "Right side at kick-off")}
            tip=${t("Equipo B es el equipo rival (derecha en el saque inicial). Puedes arrastrar cualquier puesto de un equipo al otro.", "Team B is the rival team (right side at kick-off). You can drag any seat between teams.")}
            color="var(--eq-b)" filas=${equipoB}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} mirando=${mirando} vivo=${vivo} esclavo=${esclavo}
            puedeMas=${sinUsar.length > 0} puedeMenos=${equipoB.length > 1}
            mas=${() => masDe("b")} menos=${() => menosDe(equipoB)}/>
        </div>
        ${sinEquipo.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
          <div class="lab">${t("MANDOS SIN EQUIPO", "PADS WITHOUT TEAM")}</div>
          <div class="ayuda">${t("Estos puestos quedaron fuera del Equipo A y del B. Pásalos a un equipo (botones A / B) o quítales el mando.", "These seats are outside both teams. Move them to a team (A / B buttons) or remove their pad.")}</div>
          ${sinEquipo.map((p) => html`<${Fila} key=${p.n} p=${p} m=${m} color="var(--mut)" arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} mirando=${mirando} vivo=${vivo}/>`)}
        </div>` : null}
      </div>

      <div style="display:flex;flex-direction:column;gap:14px;min-width:0">
        <div style="display:flex;align-items:center;gap:12px">
          <span class="disp" style="font-weight:700;font-size:15px;letter-spacing:.2em">${t("MIRANDO", "WATCHING")} <span class="mut" style="letter-spacing:.1em;font-size:12px">· ${t("ESPECTADORES", "SPECTATORS")}</span></span>
          <span style="flex:1;height:1px;background:linear-gradient(90deg,rgba(255,255,255,.3),transparent)"></span>
          <span class="mono mut" style="font-size:12px">${mirando.length} ${mirando.length === 1 ? t("PERSONA", "PERSON") : t("PERSONAS", "PEOPLE")}</span>
          ${mirando.length > 0 ? html`<${Boton} tipo="mini suave" deshabilitado=${esclavo} al=${sentarEspectadores}
            titulo=${t("Sienta a todos los espectadores en los puestos libres", "Seats all spectators into free seats")}>
            ⚡ ${t("SENTAR A TODOS", "SEAT ALL")}</${Boton}>` : null}
        </div>
        <div
          onDragOver=${(e) => { if (recibeMirando) { e.preventDefault(); setSobreMirando(true); } }}
          onDragLeave=${() => setSobreMirando(false)}
          onDrop=${sueltaEnMirando}
          style=${`flex:1;border-radius:14px;padding:12px;display:flex;flex-direction:column;gap:6px;background:${sobreMirando ? "var(--acc-suave)" : "rgba(0,0,0,.3)"};border:1px solid ${sobreMirando ? "var(--acc)" : "rgba(255,255,255,.12)"};transition:background .15s,border-color .15s`}>
          <div class="ayuda">${t("Están en la sala pero no tienen mando. Arrástralos a un puesto para que jueguen.", "They are in the room without a pad. Drag them to a seat to let them play.")}</div>
          ${mirando.map((g) => html`<${Espectador} key=${g.parsecId} g=${g} m=${m} arr=${arr} setArr=${setArr}/>`)}
          ${mirando.length === 0 ? html`<div class="ayuda" style="padding:6px 2px">${t("Nadie está mirando ahora.", "Nobody is watching right now.")}</div>` : null}
          <div style=${`margin-top:auto;border:1.5px dashed ${recibeMirando ? "var(--acc)" : "rgba(255,255,255,.22)"};border-radius:10px;padding:18px 12px;text-align:center;font-size:13px;line-height:1.4;color:${recibeMirando ? "var(--acc)" : "var(--mut)"}`}>
            ${recibeMirando ? t("Suelta aquí: solo va a mirar", "Drop here: they will only watch") : t("Suelta aquí a quien solo va a mirar", "Drop here anyone who only watches")}</div>
        </div>
      </div>
    </div>
    ${verVivo ? html`<div class="caja" style="padding:14px 16px;display:flex;flex-direction:column;gap:10px">
      <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
        <span class="disp" style="font-weight:700;font-size:14px;letter-spacing:.16em">${t("MANDOS EN VIVO", "LIVE PADS")}</span>
        <span class="ayuda" style="font-size:13px">${t("Lo que el juego recibe ahora mismo, en tiempo real.", "What the game receives right now, in real time.")}</span>
      </div>
      <${MandosEnVivo} m=${m}/>
    </div>` : null}
    <${OrdenPes} m=${m}/>
    ${eligiendo != null ? html`<${ElegirJugador} m=${m} indice=${eligiendo} alCerrar=${() => setEligiendo(null)}/>` : null}
  </div>`;
}

/** Saca a todos de sus puestos: pasan a «Mirando» (el anfitrión suelta el suyo). */
function TodosAMirar({ m, esclavo }) {
  const ocupados = (m.mandos.lista || []).filter((p) => p.ocupado);
  if (!ocupados.length) return null;
  const todos = async () => {
    if (!(await confirmar({ titulo: t("¿Todos a espectador?", "Everyone to spectator?"), texto: t(`Los ${ocupados.length} jugadores salen de sus puestos y pasan a mirar. Nadie sale de la sala.`, `The ${ocupados.length} players leave their seats and watch. Nobody leaves the room.`), si: t("Todos a mirar", "Everyone watches") }))) return;
    for (const p of ocupados) {
      const esHost = m.mandos.host === p.n || (p.parsecId && p.parsecId === m.sala?.hostId);
      if (esHost) await accion("mandos.tomar", { numero: 0 }, { silencioso: true });
      else await accion("mandos.liberar", { indice: p.n - 1 }, { silencioso: true });
    }
  };
  return html`<${Boton} tipo="mini peligro" deshabilitado=${esclavo} titulo=${t("Todos salen de sus puestos y pasan a mirar", "Everyone leaves their seat and watches")} al=${todos}>${t("TODOS A ESPECTADOR", "EVERYONE TO SPECTATOR")}</${Boton}>`;
}

// ---- Orden de mandos en PES (EXPERIMENTAL) ---------------------------------------------
// Link escribe settings.dat de PES 2021 para que el puesto N sea el jugador N (ver phoenix/core/OrdenPes.h).
const ESTADO_ORDEN = {
  apagado: ["", ""],
  esperando: ["", "Esperando a que Windows registre los mandos de Link…|Waiting for Windows to register Link's pads…"],
  al_dia: ["ok", "PES y Link coinciden: el puesto N es el jugador N.|PES and Link match: seat N is player N."],
  ordenado: ["ok", "Ordenado. Abre PES: ya coincide con los puestos.|Sorted. Open PES: it matches the seats now."],
  pes_abierto: ["warn", "El orden cambió y PES está abierto. Ciérralo (y Settings) y Link lo arregla solo.|The order changed and PES is open. Close it (and Settings) and Link fixes it."],
  sin_settings: ["warn", "No encuentro settings.dat de PES 2021. Abre Settings.exe del juego una vez y guarda.|Can't find PES 2021 settings.dat. Open the game's Settings.exe once and save."],
  error: ["bad", ""],
};
function OrdenPes({ m }) {
  const o = m.mandos.ordenPes || { activo: false, estado: "apagado", casillas: [] };
  const [abierto, setAbierto] = useState(false);
  const [punto, texto] = ESTADO_ORDEN[o.estado] || ["", ""];
  const [es, en] = (o.estado === "error" ? (o.error || "Error") + "|" + (o.error || "Error") : texto).split("|");
  const max = m.mandos.puestosMax ?? 8, tope = m.mandos.puestosTope ?? 16;
  const hayAviso = o.estado === "pes_abierto" || o.estado === "sin_settings" || o.estado === "error" || !!o.ds4windows;
  const mostrarDetalle = abierto || hayAviso;
  return html`<div class="caja" style="padding:10px 16px;display:flex;flex-direction:column;gap:10px">
    <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
      <span class=${cx("punto", o.activo ? (punto || "ok") : "")}></span>
      <span class="disp" style="font-weight:700;font-size:13px;letter-spacing:.14em">${t("ORDEN EN PES", "PAD ORDER IN PES")}</span>
      <span class="chip ${o.activo ? "ok" : ""}" style="height:20px;font-size:10.5px">${o.activo ? t("✓ PUESTO N = JUGADOR N", "✓ SEAT N = PLAYER N") : t("APAGADO", "OFF")}</span>
      <span class="mono mut" style="font-size:12px">${t("Máx:", "Max:")} <b>${max}/${tope}</b> ${t("puestos", "seats")}</span>
      <span class="help-dot" title=${t("Link escribe settings.dat de PES 2021 (con PES cerrado y copia de seguridad) para que el puesto 1 sea el jugador 1, el 2 el jugador 2…", "Link writes PES 2021 settings.dat (with PES closed and backup) so seat 1 is player 1, seat 2 is player 2…")}>?</span>
      <span style="flex:1"></span>
      ${o.activo ? html`<${Boton} tipo="mini suave" al=${() => accion("mandos.ordenarPes", {})}>${t("REVISAR AHORA", "CHECK NOW")}</${Boton}>` : null}
      <${Boton} tipo="mini suave" al=${() => setAbierto(!abierto)}>${abierto ? t("▴ OCULTAR AJUSTES", "▴ HIDE SETTINGS") : t(`⚙ CONFIGURAR (8–${tope}) ▾`, `⚙ CONFIGURE (8–${tope}) ▾`)}</${Boton}>
      <${Interruptor} valor=${o.activo} al=${(v) => accion("mandos.ordenPes", { si: v })} etiqueta=${t("Orden de mandos en PES", "Pad order in PES")}/>
    </div>
    ${mostrarDetalle ? html`
      ${o.activo && es ? html`<div style="display:flex;align-items:center;gap:10px;font-size:13px">
        <span class=${cx("punto", punto)}></span><span>${t(es, en)}</span>
        ${o.ultimo ? html`<span class="mono mut" style="font-size:12px">· ${t("último orden", "last sort")} ${o.ultimo}</span>` : null}
      </div>` : null}
      ${o.activo && o.ds4windows ? html`<div class="caja" style="border-color:var(--warn);font-size:13px"><b class="warn">${t("DS4Windows está abierto.", "DS4Windows is open.")}</b> ${t("Mete su propio mando virtual y desordena los de Link. Ciérralo: tu DS4 entra por Mandos › Tu mando.", "It adds its own virtual pad and shuffles Link's. Close it: your DS4 goes through Pads › Your pad.")}
        <div style="margin-top:8px"><${BotonIr} ir=${["mandos", "marionetas"]}>${t("IR A TU MANDO", "GO TO YOUR PAD")}</${BotonIr}></div></div>` : null}
      ${abierto ? html`
        ${o.activo && (o.casillas || []).length ? html`<div style="display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:6px">
          ${o.casillas.map((c) => html`<div key=${c.n} class="caja" title=${c.ok ? t("Coincide", "Matches") : t("Pendiente", "Pending")}
              style=${`padding:6px 4px;text-align:center;border-color:${c.ok ? "var(--ok)" : "var(--warn)"}`}>
            <div class="mono mut" style="font-size:11px">${c.extra ? "EXTRA" : "PES"} ${c.n}</div>
            <div class="disp" style="font-weight:700;font-size:15px">${c.puesto ? "P" + c.puesto : "—"}</div>
            <div style="font-size:11px">${c.ok ? "✓" : "…"}</div>
          </div>`)}
        </div>` : null}
        ${o.activo && o.copia ? html`<div class="ayuda mono" style="font-size:12px">${t("Copia de seguridad:", "Backup:")} ${o.copia}</div>` : null}
        ${o.activo && o.copiaExtra ? html`<div class="ayuda mono" style="font-size:12px">${t("Copia de mandos extra:", "Extra pads backup:")} ${o.copiaExtra}</div>` : null}
        <${PuestosMax} m=${m}/>
      ` : null}
    ` : null}
  </div>`;
}

/** «Puestos máximos»: 8 de serie (las casillas de settings.dat); del 9 en adelante EXPERIMENTAL. */
function PuestosMax({ m }) {
  const max = m.mandos.puestosMax ?? 8, tope = m.mandos.puestosTope ?? 16, pes = m.mandos.puestosPes ?? 8;
  return html`<div style="display:flex;flex-direction:column;gap:8px;border-top:1px solid rgba(255,255,255,.1);padding-top:12px">
    <div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
      <span class="disp" style="font-weight:700;font-size:13px;letter-spacing:.16em">${t("PUESTOS MÁXIMOS", "MAX SEATS")}</span>
      ${max > pes ? html`<${Chip} tipo="vip">${t("EXPERIMENTAL", "EXPERIMENTAL")}</${Chip}>` : null}
      <span style="flex:1"></span>
      <${Stepper} valor=${max} min=${pes} max=${tope} deshabilitado=${!!m.mandos.esclavo} al=${(v) => accion("mandos.puestosMax", { n: v })}/>
    </div>
    <div class="ayuda" style="font-size:13px">${t(
      `Cuántos mandos virtuales puede tener la sala (también sube solo si pones más mandos Xbox en Tu mando › Mandos virtuales). ${pes} es lo normal. Hasta ${tope} es experimental: los puestos ${pes + 1}+ no caben en settings.dat; con «Orden de mandos en PES» encendido, Link los escribe (con PES cerrado y copia) en phoenix_mandos_extra.dat y PES solo los usa con el módulo de mandos extra de Sider. Después sube los mandos Xbox en Mandos › Mandos virtuales.`,
      `How many virtual pads the room can have. ${pes} is normal. Up to ${tope} is experimental: seats ${pes + 1}+ don't fit in settings.dat; with «Pad order in PES» on, Link writes them (PES closed, with backup) to phoenix_mandos_extra.dat and PES only uses them with Sider's extra pads module. Then raise the Xbox pads in Pads › Virtual pads.`)}</div>
  </div>`;
}

/** Cuántos mandos virtuales de cada tipo emula tu PC (a mano). Los primeros puestos son Xbox y los últimos PlayStation. */
function TipoMando({ m, esclavo }) {
  const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0, tope = m.mandos.puestosTope ?? 16;
  const cantidad = (x, d) => accion("mandos.cantidad", { xbox: x, ds4: d });
  return html`<div style=${`display:flex;align-items:center;gap:10px;margin-right:8px;${esclavo ? "opacity:.4;pointer-events:none" : ""}`}
    title=${t(`Cuántos mandos de cada tipo emula tu PC (máximo ${tope} en total). Cámbialo antes de abrir la sala.`, `How many pads of each type your PC emulates (max ${tope} in total). Change it before opening the room.`)}>
    <span class="chip mod" style="height:22px">XBOX</span>
    <${Stepper} valor=${xbox} min=${0} max=${tope - ds4} al=${(v) => cantidad(v, ds4)}/>
    <span class="chip acc2" style="height:22px">PLAYSTATION</span>
    <${Stepper} valor=${ds4} min=${0} max=${tope - xbox} al=${(v) => cantidad(xbox, v)}/>
  </div>`;
}
