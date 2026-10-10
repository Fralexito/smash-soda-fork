// =============================================================================
//  PUESTOS (diseño aprobado 2026-10-08): «Jugando» (Local / Visita) y «Mirando».
//  Se arrastra a cada persona al puesto que se quiera; lo que se suelta en
//  «Mirando» solo ve la partida. Usa las MISMAS acciones que la pantalla anterior
//  (mandos.asignar, mandos.intercambiar, mandos.liberar, mandos.tomar,
//  mandos.formacion, mandos.bloquear, mandos.conectar/desconectar,
//  mandos.herramienta): no hay nada nuevo en C++ ni en el simulador.
//  La versión anterior (Puestos/Puesto en mandos.js) se conserva sin usar.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar } from "../tienda.js";
import { Boton, Icono, Vacio, Avatar, Chip, Stepper, colorPing, cx, dos } from "../ui.js";
import { Pendientes, ElegirJugador } from "./mandos.js";
import { MandosEnVivo } from "./mandovivo.js";

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

  const soltar = (e) => {
    e.preventDefault();
    setSobre(false);
    const a = arr;
    setArr(null);
    if (!a || !recibe) return;
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
      style=${`display:flex;align-items:center;gap:14px;padding:12px 14px;border-radius:10px;border:${p.ocupado ? "1px solid" : "1.5px dashed"} ${borde};background:${fondo};box-shadow:${sombra};opacity:${opacidad};cursor:${p.ocupado ? "grab" : "default"};transition:border-color .15s,background .15s,box-shadow .15s,opacity .15s`}>
    ${p.ocupado ? html`<span style="color:var(--mut)"><${Agarre}/></span>` : html`<span style="width:12px;flex:none"></span>`}
    <div class="disp" style=${`font-weight:700;font-size:40px;line-height:1;width:56px;flex:none;color:${p.ocupado ? color : "rgba(255,255,255,.22)"}`}>${dos(p.n)}</div>
    ${p.ocupado
      ? html`<${Avatar} nombre=${p.jugador} id=${p.parsecId} url=${perfil?.avatar_url} t=${44}/>
        <div style="flex:1;min-width:0">
          <div style="font:600 17px/1.2 var(--f-body);color:#fff;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${p.jugador}
            ${esHost ? html` <${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}
            <span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:20px;margin-left:6px;font-size:11px" title=${p.tipo === "ds4" ? t("Este puesto es un mando de PlayStation (DualShock 4)", "This seat is a PlayStation pad (DualShock 4)") : t("Este puesto es un mando de Xbox", "This seat is an Xbox pad")}>${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span></div>
          <div class="mono mut" style="font-size:12px">
            ${p.conectado ? t("conectado", "connected") : t("desconectado", "unplugged")}${p.bloqueado ? t(" · bloqueado", " · locked") : ""}
            ${p.ping >= 0 ? html` · <span style=${`color:${colorPing(p.ping)}`}>${p.ping} ms</span>` : null}</div>
        </div>`
      : html`<div style="flex:1;min-width:0">
          <div style="font:600 17px/1.2 var(--f-body);color:var(--mut)">${t("Puesto libre", "Free seat")}
            ${p.tipo ? html`<span class=${cx("chip", p.tipo === "ds4" ? "acc2" : "mod")} style="height:18px;margin-left:6px;font-size:10px;opacity:.8">${p.tipo === "ds4" ? "PLAYSTATION" : "XBOX"}</span>` : null}</div>
          <div style="font:400 13px/1.3 var(--f-body);color:var(--mut)">${arrastrando && recibe ? t("Suelta aquí", "Drop here") : t("Arrastra a alguien aquí", "Drag someone here")}</div>
        </div>`}
    ${p.ocupado ? html`<button class="icono-btn" style="height:38px;width:38px;flex:none" data-tip=${p.bloqueado ? t("Desbloquear este mando", "Unlock this pad") : t("Bloquear este mando", "Lock this pad")}
        onClick=${() => accion("mandos.bloquear", { indice: p.n - 1 })}><span style=${p.bloqueado ? "color:var(--warn)" : ""}><${Icono} n="candado" t=${17}/></span></button>
      <button class="icono-btn" style="height:38px;width:38px;flex:none" data-tip=${p.conectado ? t("Desenchufar el mando virtual", "Unplug virtual pad") : t("Enchufar el mando virtual", "Plug virtual pad")}
        onClick=${() => accion(p.conectado ? "mandos.desconectar" : "mandos.conectar", { indice: p.n - 1 })}><span style=${p.conectado ? "color:var(--ok)" : "color:var(--bad)"}><${Icono} n="enlace" t=${17}/></span></button>` : null}
    ${p.ocupado
      ? html`<${Boton} tipo="peligro mini" estilo="flex:none" al=${() => esHost ? accion("mandos.tomar", { numero: 0 }) : accion("mandos.liberar", { indice: p.n - 1 })}>${t("QUITAR", "REMOVE")}</${Boton}>`
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
function Espectador({ g, m, arr, setArr }) {
  const libres = (m.mandos?.lista || []).filter((p) => !p.ocupado);
  const soloWeb = g.rolWeb === "espectador";
  const perfil = m.perfiles?.[g.parsecId];
  const moviendo = arr && arr.tipo === "persona" && arr.valor === g.parsecId;
  if (moviendo) {
    return html`<div style="height:58px;box-sizing:border-box;border:1.5px dashed rgba(255,255,255,.22);border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:13px;color:var(--mut)">
      ${g.nombre} ${t("se está moviendo…", "is moving…")}</div>`;
  }
  return html`<div draggable=${!soloWeb}
      onDragStart=${(e) => { e.dataTransfer.setData("text/plain", "persona:" + g.parsecId); e.dataTransfer.effectAllowed = "move"; setArr({ tipo: "persona", valor: g.parsecId }); }}
      onDragEnd=${() => setArr(null)}
      style=${`display:flex;align-items:center;gap:12px;padding:10px 12px;border-radius:10px;border:1px solid rgba(255,255,255,.14);background:var(--panel-2);cursor:${soloWeb ? "default" : "grab"};opacity:${soloWeb ? ".75" : "1"}`}>
    ${soloWeb ? html`<span style="width:12px;flex:none"></span>` : html`<span style="color:var(--mut)"><${Agarre}/></span>`}
    <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${perfil?.avatar_url} t=${38}/>
    <div style="flex:1;min-width:0">
      <div style="font:600 15px/1.2 var(--f-body);color:#fff;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${g.nombre}
        <span class="chip" style="height:18px;margin-left:6px;font-size:10px">${t("ESPECTADOR", "SPECTATOR")}</span>
        <span class=${cx("chip", soloWeb ? "acc2" : "acc")} style="height:18px;margin-left:4px;font-size:10px">${soloWeb ? "WEB" : "PARSEC"}</span></div>
      <div class="mono mut" style="font-size:12px">${soloWeb
        ? t("solo espectador en la web", "web spectator only")
        : g.ping >= 0 ? html`<span style=${`color:${colorPing(g.ping)}`}>${g.ping} ms</span>` : "—"}</div>
    </div>
    ${soloWeb ? null : html`<select class="campo" style="width:auto;height:32px;font-size:12px;flex:none" disabled=${libres.length === 0 || m.mandos?.esclavo}
      aria-label=${t("Dar puesto a ", "Give a seat to ") + g.nombre}
      onChange=${(e) => { const v = Number(e.currentTarget.value); e.currentTarget.value = ""; if (v) accion("mandos.asignar", { indice: v - 1, parsecId: g.parsecId }); }}>
      <option value="">${libres.length ? t("DAR PUESTO…", "GIVE SEAT…") : t("SIN LIBRES", "NONE FREE")}</option>
      ${libres.map((p) => html`<option value=${p.n}>${dos(p.n)} · ${p.equipo === "local" ? "Local" : p.equipo === "visitante" ? t("Visita", "Away") : t("Fuera", "Off")}</option>`)}
    </select>`}
  </div>`;
}

// ---- Pantalla ---------------------------------------------------------------------------------
export function PuestosNuevo({ m }) {
  const [eligiendo, setEligiendo] = useState(null);
  const [arr, setArr] = useState(null);
  const [sobreMirando, setSobreMirando] = useState(false);
  const [verVivo, setVerVivo] = useState(false);
  const lista = m.mandos.lista || [];
  const f = m.mandos.formacion || { local: 1, visitante: 1 };
  const total = lista.length;
  const esclavo = !!m.mandos.esclavo;

  if (total === 0) {
    return html`<${Vacio} titulo=${t("No hay mandos virtuales", "No virtual pads")} texto=${t("Instala ViGEmBus o revisa Sala › Conexión.", "Install ViGEmBus or check Room › Connection.")}/>`;
  }

  const locales = lista.filter((p) => p.equipo === "local");
  const visitas = lista.filter((p) => p.equipo === "visitante");
  // Puestos que quedaron sin equipo pero tienen a alguien: se muestran para que nadie quede oculto.
  const sinEquipo = lista.filter((p) => p.equipo !== "local" && p.equipo !== "visitante" && p.ocupado);
  const sentados = new Set(lista.filter((p) => p.ocupado && p.parsecId).map((p) => p.parsecId));
  const mirando = (m.invitados || []).filter((g) => !g.falso && !sentados.has(g.parsecId));

  const formacion = (local, visitante) => accion("mandos.formacion", { local, visitante });
  const maxLocal = Math.max(1, Math.min(7, total - f.visitante));
  const maxVisita = Math.max(1, Math.min(7, total - f.local));
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
          ${verVivo ? t("OCULTAR MANDOS EN VIVO", "HIDE LIVE PADS") : t("VER MANDOS EN VIVO", "SHOW LIVE PADS")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando} al=${() => herramienta("ordenar")}>${t("ORDENAR", "SORT")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${esclavo || m.mandos.reiniciando}
          al=${() => herramienta("reiniciar", [t("¿Reiniciar los mandos?", "Reset pads?"), t("Se desconectan y vuelven a conectar todos los mandos virtuales. Úsalo si el juego dejó de detectarlos.", "All virtual pads are re-plugged. Use it if the game stopped detecting them.")])}>
          ${m.mandos.reiniciando ? t("REINICIANDO…", "RESETTING…") : t("REINICIAR", "RESET")}</${Boton}>
        <${Boton} tipo="mini peligro" deshabilitado=${esclavo}
          al=${() => herramienta("desconectarTodos", [t("¿Quitar todos los mandos?", "Remove all pads?"), t("Nadie podrá jugar hasta que vuelvas a asignar.", "Nobody can play until you assign again.")])}>${t("QUITAR TODOS", "REMOVE ALL")}</${Boton}>
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
          <${Equipo} nombre="LOCAL" desc=${t("El equipo de quien abre la sala", "The room owner's team")} color="var(--acc)" filas=${locales}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} esclavo=${esclavo}
            puedeMas=${f.local < maxLocal} puedeMenos=${f.local > 1}
            mas=${() => formacion(f.local + 1, f.visitante)} menos=${() => formacion(f.local - 1, f.visitante)}/>
          <${Equipo} nombre=${t("VISITA", "AWAY")} desc=${t("El equipo rival", "The rival team")} color="var(--eq-b)" filas=${visitas}
            m=${m} arr=${arr} setArr=${setArr} alAsignar=${setEligiendo} esclavo=${esclavo}
            puedeMas=${f.visitante < maxVisita} puedeMenos=${f.visitante > 1}
            mas=${() => formacion(f.local, f.visitante + 1)} menos=${() => formacion(f.local, f.visitante - 1)}/>
        </div>
        ${sinEquipo.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
          <div class="lab">${t("MANDOS SIN EQUIPO", "PADS WITHOUT TEAM")}</div>
          <div class="ayuda">${t("Estos puestos quedaron fuera de Local y Visita. Añade puestos a un equipo o quítales el mando.", "These seats are outside both teams. Add seats to a team or remove their pad.")}</div>
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
          style=${`flex:1;border-radius:14px;padding:16px;display:flex;flex-direction:column;gap:10px;background:${sobreMirando ? "var(--acc-suave)" : "rgba(0,0,0,.3)"};border:1px solid ${sobreMirando ? "var(--acc)" : "rgba(255,255,255,.12)"};transition:background .15s,border-color .15s`}>
          <div class="ayuda">${t("Están en la sala pero no tienen mando. Arrástralos a un puesto para que jueguen.", "They are in the room without a pad. Drag them to a seat to let them play.")}</div>
          ${mirando.map((g) => html`<${Espectador} key=${g.parsecId} g=${g} m=${m} arr=${arr} setArr=${setArr}/>`)}
          ${mirando.length === 0 ? html`<div class="ayuda" style="padding:6px 2px">${t("Nadie está mirando ahora.", "Nobody is watching right now.")}</div>` : null}
          <div style=${`margin-top:auto;border:1.5px dashed ${recibeMirando ? "var(--acc)" : "rgba(255,255,255,.22)"};border-radius:10px;padding:18px 12px;text-align:center;font-size:13px;line-height:1.4;color:${recibeMirando ? "var(--acc)" : "var(--mut)"}`}>
            ${recibeMirando ? t("Suelta aquí: solo va a mirar", "Drop here: they will only watch") : t("Suelta aquí a quien solo va a mirar", "Drop here anyone who only watches")}</div>
        </div>
      </div>
    </div>
    ${eligiendo != null ? html`<${ElegirJugador} m=${m} indice=${eligiendo} alCerrar=${() => setEligiendo(null)}/>` : null}
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
