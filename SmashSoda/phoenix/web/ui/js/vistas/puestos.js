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
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar, avisar } from "../tienda.js";
import { Boton, Icono, Vacio, Avatar, Chip, Stepper, Interruptor, colorPing, cx, dos } from "../ui.js";
import { Pendientes, ElegirJugador } from "./mandos.js";
import { MandosEnVivo } from "./mandovivo.js";
import { BotonJugar } from "./jugar.js";
import { IndicadorRetraso, retrasoDe } from "./retraso.js";

function Agarre() {
  return html`<svg width="12" height="20" viewBox="0 0 14 22" fill="currentColor" style="opacity:.55;flex:none" aria-hidden="true">
    <circle cx="3" cy="4" r="1.6"/><circle cx="11" cy="4" r="1.6"/><circle cx="3" cy="11" r="1.6"/>
    <circle cx="11" cy="11" r="1.6"/><circle cx="3" cy="18" r="1.6"/><circle cx="11" cy="18" r="1.6"/></svg>`;
}

// ---- Un puesto (fila) -------------------------------------------------------------------
function Fila({ p, m, color, arr, setArr, alAsignar }) {
  const [sobre, setSobre] = useState(false);
  const esHost = m.mandos.host === p.n || (p.ocupado && p.parsecId && p.parsecId === m.sala?.hostId);
  const arrastrando = arr != null;
  const recibe = arrastrando && ((arr.tipo === "puesto" && arr.valor !== p.n - 1) || (arr.tipo === "persona" && !p.ocupado));
  const resaltado = recibe && sobre;
  const borde = resaltado ? "var(--acc)" : recibe ? color : p.ocupado ? color : "rgba(255,255,255,.22)";
  const fondo = resaltado ? "var(--acc-suave)" : p.ocupado ? "var(--panel-2)" : "rgba(0,0,0,.25)";
  const sombra = resaltado ? "0 0 0 4px var(--acc-suave),0 0 28px var(--acc-suave)" : "none";
  const opacidad = arrastrando && !recibe && arr.valor !== p.n - 1 ? ".55" : "1";
  const perfil = p.ocupado ? m.perfiles?.[p.parsecId] : null;

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
    if (esHost) return avisar(t("Ese es tu puesto: muévete tú a otro, o arrástrate a «Mirando».", "That's your seat: move yourself or drag yourself to «Watching»."), "warn");
    if (a.tipo === "puesto") accion("mandos.intercambiar", { a: a.valor, b: p.n - 1 });
    else accion("mandos.asignar", { indice: p.n - 1, parsecId: a.valor });
  };

  return html`<div
      draggable=${p.ocupado}
      onDragStart=${(e) => { e.dataTransfer.setData("text/plain", "puesto:" + (p.n - 1)); e.dataTransfer.effectAllowed = "move"; setArr({ tipo: "puesto", valor: p.n - 1 }); }}
      onDragEnd=${() => { setArr(null); setSobre(false); }}
      onDragOver=${(e) => { if (recibe) { e.preventDefault(); setSobre(true); } }}
      onDragLeave=${() => setSobre(false)}
      onDrop=${soltar}
      style=${`display:flex;flex-wrap:wrap;align-items:center;gap:10px 14px;padding:12px 14px;border-radius:10px;border:${p.ocupado ? "1px solid" : "1.5px dashed"} ${borde};background:${fondo};box-shadow:${sombra};opacity:${opacidad};cursor:${p.ocupado ? "grab" : "default"};transition:border-color .15s,background .15s,box-shadow .15s,opacity .15s`}>
    ${p.ocupado ? html`<span style="color:var(--mut)"><${Agarre}/></span>` : html`<span style="width:12px;flex:none"></span>`}
    <div class="disp" style=${`font-weight:700;font-size:40px;line-height:1;width:56px;flex:none;color:${p.ocupado ? color : "rgba(255,255,255,.22)"}`}>${dos(p.n)}</div>
    ${p.ocupado
      ? html`<${Avatar} nombre=${p.jugador} id=${p.parsecId} url=${perfil?.avatar_url} t=${44}/>
        <div style="flex:1 1 110px;min-width:110px">
          <div style="font:600 17px/1.2 var(--f-body);color:#fff;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${p.jugador}
            ${esHost ? html` <${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}
            <span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:20px;margin-left:6px;font-size:11px" title=${p.tipo === "ds4" ? t("Este puesto es un mando de PlayStation (DualShock 4)", "This seat is a PlayStation pad (DualShock 4)") : t("Este puesto es un mando de Xbox", "This seat is an Xbox pad")}>${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span></div>
          <div class="mono mut" style="font-size:12px">
            ${esHost ? (m.mandos.miMando ? t("con tu mando · ", "with your pad · ") + m.mandos.miMando : t("con el teclado", "with the keyboard"))
              : html`${p.conectado ? t("conectado", "connected") : t("desconectado", "unplugged")}${p.bloqueado ? t(" · bloqueado", " · locked") : ""}`}
            ${p.ping >= 0 ? html` · <span style=${`color:${colorPing(p.ping)}`}>${p.ping} ms</span>` : null}${!esHost ? html`<${IndicadorRetraso} r=${retrasoDe(m, p.parsecId)}/>` : null}</div>
        </div>`
      : html`<div style="flex:1 1 110px;min-width:110px">
          <div style="font:600 17px/1.2 var(--f-body);color:var(--mut)">${t("Puesto libre", "Free seat")}
            ${p.tipo ? html`<span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:18px;margin-left:6px;font-size:10px;opacity:.8">${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span>` : null}</div>
          <div style="font:400 13px/1.3 var(--f-body);color:var(--mut)">${arrastrando && recibe ? t("Suelta aquí", "Drop here") : t("Arrastra a alguien aquí", "Drag someone here")}</div>
        </div>`}
    ${p.ocupado && !esHost ? html`<button class="icono-btn" style="height:38px;width:38px;flex:none" data-tip=${p.bloqueado ? t("Desbloquear: el jugador vuelve a mover su mando", "Unlock: the player can move again") : t("Bloquear: el jugador no puede mover este mando (sigue en su puesto)", "Lock: the player can't move this pad (keeps the seat)")}
        onClick=${() => accion("mandos.bloquear", { indice: p.n - 1 })}><span style=${p.bloqueado ? "color:var(--warn)" : ""}><${Icono} n="candado" t=${17}/></span></button>` : null}
    ${p.equipo === "a" || p.equipo === "b" ? html`<${Boton} tipo="mini suave" estilo="flex:none"
        deshabilitado=${!!m.mandos.esclavo || (m.mandos.formacion?.[p.equipo] ?? 2) <= 1}
        titulo=${(m.mandos.formacion?.[p.equipo] ?? 2) <= 1 ? t("Es el único puesto de su equipo: cada equipo necesita al menos uno", "It's the only seat on its team: each team needs at least one")
          : (p.equipo === "a" ? t("PASAR A EQUIPO B", "MOVE TO TEAM B") : t("PASAR A EQUIPO A", "MOVE TO TEAM A")) + " · " + t("el puesto mantiene su número; solo cambia de equipo", "the seat keeps its number; only its team changes")}
        al=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: p.equipo === "a" ? "b" : "a" })}>${p.equipo === "a" ? "→ B" : "→ A"}</${Boton}>`
      : html`<${Boton} tipo="mini suave" estilo="flex:none" deshabilitado=${!!m.mandos.esclavo} titulo=${t("Meter este puesto en el Equipo A", "Put this seat on Team A")} al=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: "a" })}>A</${Boton}>
        <${Boton} tipo="mini suave" estilo="flex:none" deshabilitado=${!!m.mandos.esclavo} titulo=${t("Meter este puesto en el Equipo B", "Put this seat on Team B")} al=${() => accion("mandos.equipo", { indice: p.n - 1, equipo: "b" })}>B</${Boton}>`}
    ${p.ocupado
      ? html`<${Boton} tipo="peligro mini" estilo="flex:none" titulo=${t("Sale del puesto y pasa a mirar", "Leaves the seat and watches")} al=${() => esHost ? accion("mandos.tomar", { numero: 0 }) : accion("mandos.liberar", { indice: p.n - 1 })}>${t("QUITAR", "REMOVE")}</${Boton}>`
      : html`<${Boton} tipo="mini" estilo=${`flex:none;border-color:${color};color:${color}`} al=${() => alAsignar(p.n - 1)}>${t("ASIGNAR", "ASSIGN")}</${Boton}>`}
  </div>`;
}

// ---- Un equipo (Local o Visita) -----------------------------------------------------------
function Equipo({ nombre, desc, color, filas, m, arr, setArr, alAsignar, puedeMas, puedeMenos, mas, menos, esclavo }) {
  const n = filas.length;
  return html`<div class="cut" style=${`background:linear-gradient(135deg,${color},rgba(255,255,255,.04) 70%)`}>
    <div class="in" style="padding:20px;display:flex;flex-direction:column;gap:12px;height:100%;box-sizing:border-box">
      <div style="display:flex;align-items:center;justify-content:space-between;gap:10px">
        <div style="display:flex;flex-direction:column;gap:2px;min-width:0">
          <span class="disp" style=${`font-weight:700;font-size:22px;letter-spacing:.08em;color:${color}`}>${nombre}</span>
          <span class="ayuda">${desc}</span>
        </div>
        <span class="mono" style=${`font-size:12px;color:${color};background:rgba(255,255,255,.07);padding:6px 10px;border-radius:999px;white-space:nowrap`}>${n} ${n === 1 ? t("PUESTO", "SEAT") : t("PUESTOS", "SEATS")}</span>
      </div>
      ${filas.map((p) => html`<${Fila} key=${p.n} p=${p} m=${m} color=${color} arr=${arr} setArr=${setArr} alAsignar=${alAsignar}/>`)}
      <div style="margin-top:auto;display:flex;gap:10px;padding-top:6px">
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
  const sinSala = g.host && !m.sala?.abierta;
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

// ---- Pantalla ---------------------------------------------------------------------------------
export function PuestosNuevo({ m }) {
  const [eligiendo, setEligiendo] = useState(null);
  const [arr, setArr] = useState(null);
  const [sobreMirando, setSobreMirando] = useState(false);
  const [verVivo, setVerVivoEstado] = useState(() => { try { return localStorage.getItem("phoenix.verVivo") !== "0"; } catch { return true; } });
  const setVerVivo = (v) => { setVerVivoEstado(v); try { localStorage.setItem("phoenix.verVivo", v ? "1" : "0"); } catch { /* sin almacenamiento */ } };
  const lista = m.mandos.lista || [];
  const total = lista.length;
  const esclavo = !!m.mandos.esclavo;

  if (total === 0) {
    return html`<${Vacio} titulo=${t("No hay mandos virtuales", "No virtual pads")} texto=${t("Instala ViGEmBus o revisa Sala › Conexión.", "Install ViGEmBus or check Room › Connection.")}/>`;
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

    <div style="display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap">
      <div class="ayuda" style="font-size:14px">${t("Arrastra a cada persona al puesto que quieras. Quien esté en «Mirando» solo ve la partida.", "Drag each person to the seat you want. Anyone in «Watching» only watches.")}</div>
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <${Boton} tipo=${cx("mini", m.arbitro?.soloAutorizados ? "acc" : "suave")}
          titulo=${t("Encendido: pulsar el mando solo da puesto a quien tú ya sentaste. Los demás aparecen como «quiere jugar» y decides tú.", "On: pressing a pad only seats people you already seated. Others show up as «wants to play» and you decide.")}
          al=${() => accion("mandos.soloAutorizados", { si: !m.arbitro?.soloAutorizados })}>
          ${m.arbitro?.soloAutorizados ? t("PUESTOS: SOLO AUTORIZADOS", "SEATS: AUTHORIZED ONLY") : t("PUESTOS: LIBRES", "SEATS: OPEN")}</${Boton}>
        <${Boton} tipo=${cx("mini", verVivo ? "acc" : "suave")} al=${() => setVerVivo(!verVivo)}
          titulo=${t("Muestra qué botón pulsa cada uno, sus sticks y gatillos, en vivo. Pídeles que prueben su mando antes del partido.", "Shows each player's buttons, sticks and triggers live. Ask them to test their pad before the match.")}>
          ${verVivo ? t("▾ PLEGAR MANDOS EN VIVO", "▾ COLLAPSE LIVE PADS") : t("▸ MANDOS EN VIVO", "▸ LIVE PADS")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando} al=${() => herramienta("ordenar")}>${t("ORDENAR", "SORT")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando}
          al=${() => herramienta("reiniciar", [t("¿Reiniciar los mandos?", "Reset pads?"), t("Se desconectan y vuelven a conectar todos los mandos virtuales. Úsalo si el juego dejó de detectarlos.", "All virtual pads are re-plugged. Use it if the game stopped detecting them.")])}>
          ${m.mandos.reiniciando ? t("REINICIANDO…", "RESETTING…") : t("REINICIAR", "RESET")}</${Boton}>
        <${TodosAMirar} m=${m} esclavo=${esclavo}/>
      </div>
    </div>

    ${verVivo ? html`<div class="caja" style="padding:14px 16px;display:flex;flex-direction:column;gap:10px">
      <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
        <span class="disp" style="font-weight:700;font-size:14px;letter-spacing:.16em">${t("MANDOS EN VIVO", "LIVE PADS")}</span>
        <span class="ayuda" style="font-size:13px">${t("Lo que el juego recibe ahora mismo. Se actualiza unas 5 veces por segundo.", "What the game receives right now. Updates about 5 times per second.")}</span>
      </div>
      <${MandosEnVivo} m=${m}/>
    </div>` : null}

    <div style="display:grid;grid-template-columns:minmax(0,1fr) 320px;gap:20px;flex:1;align-items:stretch">
      <div style="display:flex;flex-direction:column;gap:14px;min-width:0">
        <div style="display:flex;align-items:center;gap:12px">
          <span class="disp" style="font-weight:700;font-size:15px;letter-spacing:.2em">${t("JUGANDO", "PLAYING")}</span>
          <span style="flex:1;height:1px;background:linear-gradient(90deg,var(--linea-fuerte),transparent)"></span>
        </div>
        <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(380px,1fr));gap:16px;align-items:stretch">
          <${Equipo} nombre=${t("EQUIPO A", "TEAM A")} desc=${t("Un equipo de Link: el que estaba a la izquierda en el saque", "A Link team: the one on the left at kick-off")} color="var(--acc)" filas=${equipoA}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} esclavo=${esclavo}
            puedeMas=${sinUsar.length > 0} puedeMenos=${equipoA.length > 1}
            mas=${() => masDe("a")} menos=${() => menosDe(equipoA)}/>
          <${Equipo} nombre=${t("EQUIPO B", "TEAM B")} desc=${t("El equipo rival", "The rival team")} color="var(--eq-b)" filas=${equipoB}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} esclavo=${esclavo}
            puedeMas=${sinUsar.length > 0} puedeMenos=${equipoB.length > 1}
            mas=${() => masDe("b")} menos=${() => menosDe(equipoB)}/>
        </div>
        <div class="ayuda" style="font-size:13px">${t(
          "Equipo A y Equipo B son los equipos de Link: no tienen por qué coincidir con local y visita del juego (A = el que estaba a la izquierda en el saque, según el Campo 2D). El número del puesto es el mando de Link y no cambia: con «Orden de mandos en PES» el puesto N es el jugador N. Pasa un puesto al otro equipo con su botón «→ A» / «→ B» (PASAR A EQUIPO A / B); por ejemplo, tú en el 01 siendo del Equipo B.",
          "Team A and Team B are Link's teams: they don't have to match the game's home and away (A = the one on the left at kick-off, per the 2D Pitch). The seat number is Link's pad and never changes: with «Pad order in PES», seat N is player N. Move a seat to the other team with its «→ A» / «→ B» button (MOVE TO TEAM A / B); for example, you on 01 as Team B.")}</div>
        ${sinUsar.length ? html`<div class="caja" style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;padding:12px 16px">
          <span class="lab" style="margin:0">${t("PUESTOS SIN USAR", "UNUSED SEATS")}</span>
          ${sinUsar.map((p) => html`<span key=${p.n} style="display:inline-flex;align-items:center;gap:6px">
            <span class="disp" style="font-weight:700;font-size:18px;color:var(--mut)">${dos(p.n)}</span>
            <${Boton} tipo="mini suave" deshabilitado=${esclavo} titulo=${t(`Meter el puesto ${dos(p.n)} en el Equipo A`, `Put seat ${dos(p.n)} on Team A`)} al=${() => ponerEquipo(p, "a")}>A</${Boton}>
            <${Boton} tipo="mini suave" deshabilitado=${esclavo} titulo=${t(`Meter el puesto ${dos(p.n)} en el Equipo B`, `Put seat ${dos(p.n)} on Team B`)} al=${() => ponerEquipo(p, "b")}>B</${Boton}>
          </span>`)}
        </div>` : null}
        ${sinEquipo.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
          <div class="lab">${t("MANDOS SIN EQUIPO", "PADS WITHOUT TEAM")}</div>
          <div class="ayuda">${t("Estos puestos quedaron fuera del Equipo A y del B. Pásalos a un equipo (botones A / B) o quítales el mando.", "These seats are outside both teams. Move them to a team (A / B buttons) or remove their pad.")}</div>
          ${sinEquipo.map((p) => html`<${Fila} key=${p.n} p=${p} m=${m} color="var(--mut)" arr=${arr} setArr=${setArr} alAsignar=${setEligiendo}/>`)}
        </div>` : null}
      </div>

      <div style="display:flex;flex-direction:column;gap:14px;min-width:0">
        <div style="display:flex;align-items:center;gap:12px">
          <span class="disp" style="font-weight:700;font-size:15px;letter-spacing:.2em">${t("MIRANDO", "WATCHING")} <span class="mut" style="letter-spacing:.1em;font-size:12px">· ${t("ESPECTADORES", "SPECTATORS")}</span></span>
          <span style="flex:1;height:1px;background:linear-gradient(90deg,rgba(255,255,255,.3),transparent)"></span>
          <span class="mono mut" style="font-size:12px">${mirando.length} ${mirando.length === 1 ? t("PERSONA", "PERSON") : t("PERSONAS", "PEOPLE")}</span>
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
  const [punto, texto] = ESTADO_ORDEN[o.estado] || ["", ""];
  const [es, en] = (o.estado === "error" ? (o.error || "Error") + "|" + (o.error || "Error") : texto).split("|");
  return html`<div class="caja" style="padding:16px 18px;display:flex;flex-direction:column;gap:12px">
    <div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
      <span class="disp" style="font-weight:700;font-size:14px;letter-spacing:.16em">${t("ORDEN DE MANDOS EN PES", "PAD ORDER IN PES")}</span>
      <${Chip} tipo="vip">${t("EXPERIMENTAL", "EXPERIMENTAL")}</${Chip}>
      <span style="flex:1"></span>
      ${o.activo ? html`<${Boton} tipo="mini suave" al=${() => accion("mandos.ordenarPes", {})}>${t("REVISAR AHORA", "CHECK NOW")}</${Boton}>` : null}
      <${Interruptor} valor=${o.activo} al=${(v) => accion("mandos.ordenPes", { si: v })} etiqueta=${t("Orden de mandos en PES", "Pad order in PES")}/>
    </div>
    <div class="ayuda" style="font-size:13px">${t(
      "Link escribe el settings.dat de PES para que el puesto 1 sea el jugador 1, el 2 el jugador 2… Lo hace solo con PES cerrado y guarda una copia antes. Mientras esté encendido, los mandos de Link no se desconectan al cerrar la sala (así Windows no los vuelve a barajar). Tu mando físico no va en Settings: úsalo con Marionetas.",
      "Link writes PES's settings.dat so seat 1 is player 1, seat 2 is player 2… Only with PES closed, with a backup first. While on, Link's pads stay plugged when the room closes (so Windows doesn't reshuffle them). Your physical pad doesn't go in Settings: use it with Puppets.")}</div>
    ${o.activo && es ? html`<div style="display:flex;align-items:center;gap:10px;font-size:14px">
      <span class=${cx("punto", punto)}></span><span>${t(es, en)}</span>
      ${o.ultimo ? html`<span class="mono mut" style="font-size:12px">· ${t("último orden", "last sort")} ${o.ultimo}</span>` : null}
    </div>` : null}
    ${o.activo && o.ds4windows ? html`<div class="caja" style="border-color:var(--warn);font-size:14px"><b class="warn">${t("DS4Windows está abierto.", "DS4Windows is open.")}</b> ${t("Mete su propio mando virtual y desordena los de Link. Ciérralo (y quítalo del inicio de Windows): tu DS4 entra por Mandos › TU MANDO.", "It adds its own virtual pad and shuffles Link's. Close it (and remove it from Windows startup): your DS4 goes through Pads › YOUR PAD.")}</div>` : null}
    ${o.activo && (o.casillas || []).length ? html`<div style="display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:6px">
      ${o.casillas.map((c) => html`<div key=${c.n} class="caja" title=${c.ok ? t("Coincide", "Matches") : t("Pendiente", "Pending")}
          style=${`padding:8px 4px;text-align:center;border-color:${c.ok ? "var(--ok)" : "var(--warn)"}`}>
        <div class="mono mut" style="font-size:11px">PES ${c.n}</div>
        <div class="disp" style="font-weight:700;font-size:16px">${c.puesto ? "P" + c.puesto : "—"}</div>
        <div style="font-size:12px">${c.ok ? "✓" : "…"}</div>
      </div>`)}
    </div>` : null}
    ${o.activo && o.copia ? html`<div class="ayuda mono" style="font-size:12px">${t("Copia de seguridad:", "Backup:")} ${o.copia}</div>` : null}
  </div>`;
}

/** Cuántos mandos virtuales de cada tipo emula tu PC (a mano). Los primeros puestos son Xbox y los últimos PlayStation. */
function TipoMando({ m, esclavo }) {
  const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0;
  const cantidad = (x, d) => accion("mandos.cantidad", { xbox: x, ds4: d });
  return html`<div style=${`display:flex;align-items:center;gap:10px;margin-right:8px;${esclavo ? "opacity:.4;pointer-events:none" : ""}`}
    title=${t("Cuántos mandos de cada tipo emula tu PC (máximo 8 en total). Cámbialo antes de abrir la sala.", "How many pads of each type your PC emulates (max 8 in total). Change it before opening the room.")}>
    <span class="chip mod" style="height:22px">XBOX</span>
    <${Stepper} valor=${xbox} min=${0} max=${8 - ds4} al=${(v) => cantidad(v, ds4)}/>
    <span class="chip acc2" style="height:22px">PLAYSTATION</span>
    <${Stepper} valor=${ds4} min=${0} max=${8 - xbox} al=${(v) => cantidad(xbox, v)}/>
  </div>`;
}
