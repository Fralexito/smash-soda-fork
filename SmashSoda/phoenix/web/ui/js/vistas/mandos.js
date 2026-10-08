// =============================================================================
//  MANDOS: Puestos (diseño v4, arrastrar para intercambiar), Teclado, Turnos,
//  Marionetas (mandos virtuales) y Bloqueo.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Interruptor, Ajuste, AjusteSw, Stepper, Icono, Vacio, Avatar, Chip, colorPing, cx, dos } from "../ui.js";

export const PESTANAS_MANDOS = [
  { id: "puestos", es: "PUESTOS", en: "SEATS" },
  { id: "teclado", es: "TECLADO", en: "KEYBOARD" },
  { id: "turnos", es: "TURNOS", en: "HOTSEAT" },
  { id: "marionetas", es: "MARIONETAS", en: "PUPPETS" },
  { id: "bloqueo", es: "BLOQUEO", en: "LOCK" },
];

export function metaMandos(s) {
  const lista = s.motor?.mandos?.lista || [];
  const usados = lista.filter((p) => p.ocupado).length;
  return html`<div style="display:flex;align-items:baseline;gap:10px">
    <span class="disp acc" style="font-size:34px;font-weight:700;line-height:1">${usados}</span>
    <span class="mono mut" style="font-size:14px">${t("DE", "OF")} ${lista.length} ${t("MANDOS EN USO", "PADS IN USE")}</span>
  </div>`;
}

export function VistaMandos({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.mandos) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  switch (pestana) {
    case "teclado": return html`<${Teclado} m=${m}/>`;
    case "turnos": return html`<${Turnos} m=${m}/>`;
    case "marionetas": return html`<${Marionetas} m=${m}/>`;
    case "bloqueo": return html`<${Bloqueo} m=${m}/>`;
    default: return html`<${Puestos} m=${m}/>`;
  }
}

const panelClasico = (pestana) => accion("ui.panelClasico", { seccion: 1, pestana });

// ---- Avisos pendientes (solicitudes y entrada por Parsec) -------------------------------
export function Pendientes({ m }) {
  const sol = m.solicitudes || [];
  const esp = m.espera || [];
  if (!sol.length && !esp.length) return null;
  return html`<div class="col" style="gap:10px">
    ${esp.map((x) => html`<div class="caja" key=${"e" + x.parsecId} style="border-color:var(--warn);display:flex;align-items:center;gap:14px;flex-wrap:wrap">
      <${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${36}/>
      <div style="flex:1;min-width:200px"><b>${x.nombre}</b> <span class="mono mut">#${x.parsecId}</span>
        <div class="ayuda">${t("Entró por Parsec y no está en la lista de la web. ¿Qué hace?", "Joined via Parsec, not on the web list. What now?")}
        ${x.ping >= 0 ? html` · <span style=${`color:${colorPing(x.ping)}`}>${x.ping} ms</span>` : null}</div></div>
      <${Boton} tipo="ok mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "jugador" })}>${t("JUEGA", "PLAYS")}</${Boton}>
      <${Boton} tipo="mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "espectador" })}>${t("MIRA", "WATCHES")}</${Boton}>
      <${Boton} tipo="peligro mini" al=${() => accion("espera.decidir", { parsecId: x.parsecId, como: "expulsar" })}>${t("EXPULSAR", "KICK")}</${Boton}>
    </div>`)}
    ${sol.map((x) => html`<div class="caja" key=${"s" + x.parsecId} style="border-color:var(--acc);display:flex;align-items:center;gap:14px;flex-wrap:wrap">
      <${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${36}/>
      <div style="flex:1;min-width:200px"><b>${x.nombre}</b> ${t("pide el mando", "asks for pad")} <b class="acc">${dos(x.mando)}</b>
        <div class="ayuda">${t("Lo pidió con !cambio o !equipo en el chat.", "Requested with !cambio or !equipo.")}</div></div>
      <${Boton} tipo="ok mini" al=${() => accion("solicitud.aceptar", { parsecId: x.parsecId })}>${t("ACEPTAR", "ACCEPT")}</${Boton}>
      <${Boton} tipo="peligro mini" al=${() => accion("solicitud.rechazar", { parsecId: x.parsecId })}>${t("RECHAZAR", "DECLINE")}</${Boton}>
    </div>`)}
  </div>`;
}

// ---- Elegir quién toma un mando ---------------------------------------------------------
export function ElegirJugador({ m, indice, alCerrar }) {
  const ocupadoPor = new Map((m.mandos.lista || []).filter((p) => p.ocupado).map((p) => [p.parsecId, p.n]));
  const invitados = (m.invitados || []).filter((g) => !g.falso);
  const elegir = async (g) => {
    if (await accion("mandos.asignar", { indice, parsecId: g.parsecId })) alCerrar();
  };
  const tomarYo = async () => { if (await accion("mandos.tomar", { numero: indice + 1 })) alCerrar(); };
  return html`<div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
    <div class="dialogo" style="width:min(520px,100%)"><div class="cut"><div class="in" style="padding:24px">
      <h2>${t("¿Quién toma el mando", "Who takes pad")} ${dos(indice + 1)}?</h2>
      <div style="display:flex;flex-direction:column;gap:8px;max-height:56vh;overflow:auto;margin-top:8px">
        <button class="persona" onClick=${tomarYo}>
          <${Avatar} nombre=${m.sala?.hostNombre || "Yo"} brillo t=${38}/>
          <div style="flex:1;text-align:left"><div class="nom">${t("Yo (anfitrión)", "Me (host)")}</div>
            <div class="sub mut" style="font-size:12px">${t("Tu mando físico pasa a este puesto. Atajo: Ctrl+Alt+número.", "Your physical pad takes this seat. Shortcut: Ctrl+Alt+number.")}</div></div>
        </button>
        ${invitados.map((g) => html`<button class="persona" key=${g.parsecId} onClick=${() => elegir(g)} disabled=${g.rolWeb === "espectador"}>
          <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${m.perfiles?.[g.parsecId]?.avatar_url} t=${38}/>
          <div style="flex:1;text-align:left;min-width:0"><div class="nom">${g.nombre}</div>
            <div class="sub"><span class="mono mut" style="font-size:12px">#${g.parsecId}</span>
              ${ocupadoPor.has(g.parsecId) ? html`<${Chip} tipo="acc">${t("MANDO", "PAD")} ${dos(ocupadoPor.get(g.parsecId))}</${Chip}>` : null}
              ${g.rolWeb === "espectador" ? html`<${Chip}>${t("ESPECTADOR EN LA WEB", "WEB SPECTATOR")}</${Chip}>` : null}</div></div>
          <span class="mono" style=${`color:${colorPing(g.ping)}`}>${g.ping >= 0 ? g.ping + " ms" : "—"}</span>
        </button>`)}
        ${invitados.length === 0 ? html`<div class="ayuda" style="padding:10px">${t("Aún no hay invitados en la sala.", "No guests in the room yet.")}</div>` : null}
      </div>
      <div class="botones"><button class="btn suave" onClick=${alCerrar}>${t("Cancelar", "Cancel")}</button></div>
    </div></div></div>
  </div>`;
}

// ---- Puestos -------------------------------------------------------------------------------
function Puesto({ p, m, alAsignar, arrastre }) {
  const esHost = m.mandos.host === p.n || (p.ocupado && p.parsecId && p.parsecId === m.sala?.hostId);
  const [sobre, setSobre] = useState(false);
  const fuera = p.equipo === "fuera";
  return html`<div class=${cx("cut", sobre && "vivo")} style=${fuera && !p.ocupado ? "opacity:.55" : ""} draggable=${p.ocupado}
      onDragStart=${(e) => { e.dataTransfer.setData("text/plain", String(p.n - 1)); e.dataTransfer.effectAllowed = "move"; arrastre.set(p.n - 1); }}
      onDragEnd=${() => arrastre.set(null)}
      onDragOver=${(e) => { if (arrastre.valor != null && arrastre.valor !== p.n - 1) { e.preventDefault(); setSobre(true); } }}
      onDragLeave=${() => setSobre(false)}
      onDrop=${(e) => { e.preventDefault(); setSobre(false); const a = Number(e.dataTransfer.getData("text/plain")); if (!Number.isNaN(a) && a !== p.n - 1) accion("mandos.intercambiar", { a, b: p.n - 1 }); }}>
    <div class=${cx("in", "puesto-in", p.equipo)} style=${`padding:18px;display:flex;flex-direction:column;gap:8px;position:relative;cursor:${p.ocupado ? "grab" : "default"};background:${p.ocupado ? "var(--panel)" : "#080b1f"}`}>
      <div style="display:flex;justify-content:space-between;align-items:flex-start">
        <div class="numeral" style=${`font-size:60px;color:${p.ocupado ? (p.equipo === "visitante" ? "var(--eq-b)" : "var(--acc)") : "rgba(255,255,255,.22)"}`}>${dos(p.n)}</div>
        <div style="display:flex;flex-direction:column;align-items:flex-end;gap:6px">
          <span style=${`color:${p.ocupado ? "var(--acc)" : "rgba(255,255,255,.22)"}`}><${Icono} n="mandos" t=${40} w=${1.5}/></span>
          <span class="mono mut" style="font-size:12px">${p.equipo === "local" ? "LOCAL" : p.equipo === "visitante" ? t("VISITA", "AWAY") : t("FUERA", "OFF")}</span>
        </div>
      </div>
      <div style=${`margin-top:4px;font:600 18px/1.2 var(--f-body);color:${p.ocupado ? "#fff" : "var(--mut)"};white-space:nowrap;overflow:hidden;text-overflow:ellipsis`}>
        ${p.ocupado ? p.jugador : t("Mando libre", "Free pad")} ${esHost ? html`<${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}</div>
      <div class="mono mut" style="font-size:12px">
        ${p.tipo === "ds4" ? "DS4" : "XInput"} · ${p.conectado ? t("conectado", "connected") : t("desconectado", "unplugged")}${p.bloqueado ? t(" · bloqueado", " · locked") : ""}
        ${p.ocupado && p.ping >= 0 ? html` · <span style=${`color:${colorPing(p.ping)}`}>${p.ping} ms</span>` : null}</div>
      <div style="margin-top:auto;display:flex;gap:6px">
        ${p.ocupado
          ? html`<${Boton} tipo="peligro" estilo="flex:1" al=${() => esHost ? accion("mandos.tomar", { numero: 0 }) : accion("mandos.liberar", { indice: p.n - 1 })}>${t("QUITAR MANDO", "REMOVE")}</${Boton}>`
          : html`<${Boton} estilo="flex:1" al=${() => alAsignar(p.n - 1)}>${t("ASIGNAR", "ASSIGN")}</${Boton}>`}
        <button class="icono-btn" style="height:38px;width:38px" title=${p.bloqueado ? t("Desbloquear", "Unlock") : t("Bloquear", "Lock")}
          onClick=${() => accion("mandos.bloquear", { indice: p.n - 1 })}><span style=${p.bloqueado ? "color:var(--warn)" : ""}><${Icono} n="candado" t=${17}/></span></button>
        <button class="icono-btn" style="height:38px;width:38px" title=${p.conectado ? t("Desconectar mando virtual", "Unplug virtual pad") : t("Conectar mando virtual", "Plug virtual pad")}
          onClick=${() => accion(p.conectado ? "mandos.desconectar" : "mandos.conectar", { indice: p.n - 1 })}><span style=${p.conectado ? "color:var(--ok)" : "color:var(--bad)"}><${Icono} n="enlace" t=${17}/></span></button>
      </div>
    </div>
  </div>`;
}

function Puestos({ m }) {
  const [eligiendo, setEligiendo] = useState(null);
  const [arr, setArr] = useState(null);
  const lista = m.mandos.lista || [];
  const f = m.mandos.formacion || { local: 1, visitante: 1 };
  const total = lista.length;
  const formacion = (local, visitante) => accion("mandos.formacion", { local, visitante });
  const herramienta = async (nombre, pregunta) => {
    if (pregunta && !(await confirmar({ titulo: pregunta[0], texto: pregunta[1], si: t("Sí", "Yes"), peligro: true }))) return;
    return accion("mandos.herramienta", { nombre });
  };
  if (total === 0) {
    return html`<${Vacio} titulo=${t("No hay mandos virtuales", "No virtual pads")} texto=${t("Instala ViGEmBus o revisa Ajustes › Diagnóstico.", "Install ViGEmBus or check Settings › Diagnostics.")}/>`;
  }
  const filas = Math.ceil(total / 4);
  return html`<div class="col" style="min-height:100%">
    <${Pendientes} m=${m}/>
    ${m.mandos.esclavo ? html`<div class="caja" style="border-color:var(--warn)"><b class="warn">${t("Turnos o torneo controlan los mandos.", "Hotseat or tournament controls the pads.")}</b>
      <span class="ayuda"> ${t("Las herramientas generales están en pausa mientras tanto.", "General tools are paused meanwhile.")}</span></div>` : null}
    <div class="caja" style="display:flex;gap:18px;align-items:center;flex-wrap:wrap;padding:10px 14px">
      <span class="lab">${t("FORMACIÓN", "FORMATION")}</span>
      <span class="mono" style="font-size:13px">LOCAL</span>
      <${Stepper} valor=${f.local} min=${1} max=${Math.max(1, Math.min(7, total - f.visitante))} al=${(v) => formacion(v, f.visitante)}/>
      <span class="mono" style="font-size:13px">${t("VISITA", "AWAY")}</span>
      <${Stepper} valor=${f.visitante} min=${1} max=${Math.max(1, Math.min(7, total - f.local))} al=${(v) => formacion(f.local, v)}/>
      <span class="ayuda">${t("Arrastra un puesto sobre otro para intercambiarlos.", "Drag a seat onto another to swap.")}</span>
      <div style="margin-left:auto;display:flex;gap:8px;flex-wrap:wrap">
        <${Boton} tipo="mini suave" deshabilitado=${m.mandos.esclavo || m.mandos.reiniciando} al=${() => herramienta("ordenar")}>${t("ORDENAR", "SORT")}</${Boton}>
        <${Boton} tipo="mini suave" deshabilitado=${m.mandos.esclavo || m.mandos.reiniciando}
          al=${() => herramienta("reiniciar", [t("¿Reiniciar los mandos?", "Reset pads?"), t("Se desconectan y vuelven a conectar todos los mandos virtuales. Úsalo si el juego dejó de detectarlos.", "All virtual pads are re-plugged. Use it if the game stopped detecting them.")])}>
          ${m.mandos.reiniciando ? t("REINICIANDO…", "RESETTING…") : t("REINICIAR", "RESET")}</${Boton}>
        <${Boton} tipo="mini peligro" deshabilitado=${m.mandos.esclavo}
          al=${() => herramienta("desconectarTodos", [t("¿Quitar todos los mandos?", "Remove all pads?"), t("Nadie podrá jugar hasta que vuelvas a asignar.", "Nobody can play until you assign again.")])}>${t("QUITAR TODOS", "REMOVE ALL")}</${Boton}>
      </div>
    </div>
    <div style=${`display:grid;grid-template-columns:repeat(4,minmax(0,1fr));grid-template-rows:repeat(${filas},minmax(220px,1fr));gap:16px;flex:1`}>
      ${lista.map((p) => html`<${Puesto} key=${p.n} p=${p} m=${m} alAsignar=${setEligiendo} arrastre=${{ valor: arr, set: setArr }}/>`)}
    </div>
    ${eligiendo != null ? html`<${ElegirJugador} m=${m} indice=${eligiendo} alCerrar=${() => setEligiendo(null)}/>` : null}
  </div>`;
}

// ---- Teclado ------------------------------------------------------------------------------
function Teclado({ m }) {
  const g = m.ajustes?.general || {};
  const invitados = (m.invitados || []).filter((x) => !x.falso);
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:8px">
      <${Titulo} texto=${t("TECLADO DE LOS INVITADOS", "GUEST KEYBOARD")}/>
      <${AjusteSw} titulo=${t("Desactivar teclado para todos", "Disable keyboard for everyone")} desc=${t("Nadie juega con teclado aunque tenga permiso.", "Nobody plays with keyboard even if allowed.")}
        valor=${g.disableKeyboard} al=${(v) => accion("ajustes.general", { clave: "disableKeyboard", valor: v })}/>
      <div class="lab" style="margin-top:14px">${t("PERMISO POR INVITADO", "PER-GUEST PERMISSION")}</div>
      ${invitados.length === 0 ? html`<div class="ayuda">${t("Sin invitados ahora.", "No guests now.")}</div>` : null}
      ${invitados.map((x) => html`<${AjusteSw} key=${x.parsecId} titulo=${x.nombre} desc=${"#" + x.parsecId}
        valor=${x.teclado} al=${(v) => accion("gente.teclado", { parsecId: x.parsecId, si: v })}/>`)}
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("MAPA DE TECLAS", "KEY MAP")}/>
      <div class="ayuda" style="font-size:14px">${t("El teclado visual (qué tecla es cada botón del mando) se edita en el panel de Phoenix: arrastras y sueltas sobre un teclado dibujado.",
        "The visual keyboard (which key is each pad button) is edited in the Phoenix panel with drag and drop.")}</div>
      <div style="margin-top:18px"><${Boton} tipo="lleno" al=${() => panelClasico(1)}><${Icono} n="panel" t=${16}/>${t("ABRIR TECLADO VISUAL", "OPEN VISUAL KEYBOARD")}</${Boton}></div>
      <div class="ayuda" style="margin-top:14px">${t("Vuelves aquí con el botón «Volver a la interfaz nueva».", "Come back with the «Back to the new interface» button.")}</div>
    </${Tarjeta}>
  </div>`;
}

// ---- Turnos ----------------------------------------------------------------------------------
function Turnos({ m }) {
  const tu = m.turnos || {};
  const aj = (d) => accion("turnos.ajustes", d);
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("TURNOS (HOTSEAT)", "HOTSEAT")} derecha=${html`<${Interruptor} valor=${tu.activo} al=${(v) => accion("turnos.activar", { si: v })} etiqueta="Hotseat"/>`}/>
      <div class="ayuda" style="margin-bottom:6px">${t("Cada invitado juega un rato y luego cede el mando al siguiente. Ideal para salas abiertas con muchos esperando.",
        "Each guest plays for a while and then hands over the pad.")}</div>
      <${Ajuste} titulo=${t("Minutos de juego", "Play minutes")} desc=${t("Tiempo con el mando antes de rotar.", "Time with the pad before rotating.")}>
        <${Stepper} valor=${tu.juegoMin ?? 10} min=${1} max=${999} al=${(v) => aj({ juegoMin: v })}/></${Ajuste}>
      <${Ajuste} titulo=${t("Minutos de descanso", "Cooldown minutes")} desc=${t("Cuándo puede volver a jugar (≥ minutos de juego).", "When they can play again.")}>
        <${Stepper} valor=${tu.reinicioMin ?? 30} min=${tu.juegoMin ?? 1} max=${999} al=${(v) => aj({ reinicioMin: v })}/></${Ajuste}>
      <${Ajuste} titulo=${t("Aviso antes de terminar", "Reminder")} desc=${t("Minutos antes del final (0 = sin aviso).", "Minutes before the end (0 = off).")}>
        <${Stepper} valor=${tu.recordatorioMin ?? 0} min=${0} max=${Math.max(0, (tu.juegoMin ?? 1) - 1)} al=${(v) => aj({ recordatorioMin: v })}/></${Ajuste}>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("EN TURNO", "IN ROTATION")} derecha=${html`<span class=${cx("chip", tu.corriendo ? "acc" : "")}>${tu.corriendo ? t("CORRIENDO", "RUNNING") : t("EN PAUSA", "IDLE")}</span>`}/>
      ${(tu.usuarios || []).length === 0 ? html`<div class="ayuda">${tu.activo ? t("Aparecen al tomar un mando.", "They show up when they take a pad.") : t("Activa los turnos para empezar.", "Enable hotseat to start.")}</div>` : null}
      <div style="display:flex;flex-direction:column;gap:8px">${(tu.usuarios || []).map((u) => html`<div class="persona" style="cursor:default" key=${u.parsecId}>
        <${Avatar} nombre=${u.nombre} id=${u.parsecId} t=${34}/>
        <div style="flex:1;min-width:0"><div class="nom">${u.nombre}</div>
          <div class="sub">${u.enAsiento ? html`<${Chip} tipo="acc">${t("JUGANDO", "PLAYING")}</${Chip}>` : null}${u.enfriamiento ? html`<${Chip} tipo="vip">${t("DESCANSO", "COOLDOWN")}</${Chip}>` : null}</div></div>
        <span class="mono" style="font-size:15px">${u.enfriamiento ? u.restanteEnfriamiento : u.restante || "—"}</span>
      </div>`)}</div>
      <div class="ayuda" style="margin-top:12px">${t("Comandos para moderadores: !extend, !decrease, !warmup, !cooldown.", "Mod commands: !extend, !decrease, !warmup, !cooldown.")}</div>
    </${Tarjeta}>
  </div>`;
}

// ---- Marionetas (mandos virtuales) --------------------------------------------------------------
function Marionetas({ m }) {
  const xbox = m.mandos.xbox ?? 0, ds4 = m.mandos.ds4 ?? 0;
  const cantidad = (x, d) => accion("mandos.cantidad", { xbox: x, ds4: d });
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("MANDOS VIRTUALES", "VIRTUAL PADS")} derecha=${html`<span class="mono mut">${xbox + ds4} / 8</span>`}/>
      <div class="ayuda" style="margin-bottom:6px">${t("Son los mandos que ve PES. Máximo 8 en total; con 4 por equipo alcanza para un 4 vs 4.", "Pads PES sees. Max 8 in total.")}</div>
      <${Ajuste} titulo="Xbox (XInput)" desc=${t("Los más compatibles con PES.", "Most compatible with PES.")}>
        <${Stepper} valor=${xbox} min=${0} max=${8 - ds4} deshabilitado=${m.mandos.esclavo} al=${(v) => cantidad(v, ds4)}/></${Ajuste}>
      <${Ajuste} titulo="DualShock 4" desc=${t("Para juegos que solo aceptan PlayStation.", "For games that only accept PlayStation pads.")}>
        <${Stepper} valor=${ds4} min=${0} max=${8 - xbox} deshabilitado=${m.mandos.esclavo} al=${(v) => cantidad(xbox, v)}/></${Ajuste}>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("MARIONETAS", "PUPPETS")}/>
      <div class="ayuda" style="font-size:14px">${t("Las marionetas conectan los mandos físicos de esta PC a los puestos virtuales (por ejemplo, para que juegue alguien sentado a tu lado).",
        "Puppets bind this PC's physical pads to virtual seats.")}</div>
      <div style="margin-top:18px"><${Boton} tipo="lleno" al=${() => panelClasico(3)}><${Icono} n="panel" t=${16}/>${t("ABRIR MARIONETAS", "OPEN PUPPETS")}</${Boton}></div>
    </${Tarjeta}>
  </div>`;
}

// ---- Bloqueo ---------------------------------------------------------------------------------------
function Bloqueo({ m }) {
  const lista = m.mandos.lista || [];
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("BLOQUEO GENERAL", "GLOBAL LOCK")}/>
      <${AjusteSw} titulo=${t("Bloquear todos los mandos", "Lock all pads")} desc=${t("Nadie puede mover nada (pausas, árbitro, revisión).", "Nobody can move (pauses, review).")}
        valor=${m.mandos.bloqueoGlobal} deshabilitado=${m.mandos.esclavo} al=${() => accion("mandos.herramienta", { nombre: "bloquearTodo" })}/>
      <${AjusteSw} titulo=${t("Bloquear botones elegidos", "Lock chosen buttons")} desc=${t("Bloquea solo los botones marcados en el panel de bloqueo (p. ej. Start/Guía).", "Locks only the buttons chosen in the lock panel.")}
        valor=${m.mandos.bloqueoBotones} deshabilitado=${m.mandos.esclavo} al=${() => accion("mandos.herramienta", { nombre: "bloquearBotones" })}/>
      <div style="margin-top:16px"><${Boton} al=${() => panelClasico(4)}><${Icono} n="panel" t=${16}/>${t("ELEGIR BOTONES", "CHOOSE BUTTONS")}</${Boton}></div>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("POR MANDO", "PER PAD")}/>
      ${lista.map((p) => html`<${AjusteSw} key=${p.n} titulo=${`${t("Mando", "Pad")} ${dos(p.n)}`} desc=${p.ocupado ? p.jugador : t("libre", "free")}
        valor=${p.bloqueado} al=${() => accion("mandos.bloquear", { indice: p.n - 1 })}/>`)}
    </${Tarjeta}>
  </div>`;
}
