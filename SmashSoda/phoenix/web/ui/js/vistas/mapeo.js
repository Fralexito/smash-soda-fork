// =============================================================================
//  JUGANDO › MAPEO: corrige el mando de cada invitado (genérico o mal mapeado).
//  Lista de personas · su mando en vivo (lo que LLEGA y lo que SALE al juego,
//  con el dibujo de mandovivo.js) · asistente paso a paso · sticks (invertir,
//  cambiar, zona muerta con vista en vivo) · botones uno a uno · restablecer.
//  Motor: phoenix/core/MapeoMandos (acciones mapeo.ver/guardar/restablecer).
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, AjusteSw, Selector, Avatar, Chip, Vacio, cx, dos } from "../ui.js";
import { MandoVivo, tomarPendienteMapeo } from "./mandovivo.js";
import { FUENTES, ETIQUETA, ZONA_MAX, PASOS, normalizar, destinoDe, conDestino, esIdentidad, construirPerfil, encaja, zonaSugerida, limpiar } from "../mapeo.js";

const NOMBRE = {
  a: ["Botón A", "A button"], b: ["Botón B", "B button"], x: ["Botón X", "X button"], y: ["Botón Y", "Y button"],
  lb: ["LB (superior izq.)", "LB (left bumper)"], rb: ["RB (superior der.)", "RB (right bumper)"],
  lt: ["LT (gatillo izq.)", "LT (left trigger)"], rt: ["RT (gatillo der.)", "RT (right trigger)"],
  back: ["Back / Select", "Back / Select"], start: ["Start (pausa)", "Start (pause)"], guia: ["Guía", "Guide"],
  ls: ["L3 (pulsar stick izq.)", "L3 (left stick click)"], rs: ["R3 (pulsar stick der.)", "R3 (right stick click)"],
  arriba: ["Cruceta arriba", "D-pad up"], abajo: ["Cruceta abajo", "D-pad down"], izq: ["Cruceta izquierda", "D-pad left"], der: ["Cruceta derecha", "D-pad right"],
};
const nom = (f) => t(...(NOMBRE[f] || [f, f]));
const textoPulsacion = (f) => {
  if (!f) return "";
  const m = /^([lr])([xy])([+-])$/.exec(f);
  if (!m) return ETIQUETA[f] || f;
  const stick = m[1] === "l" ? t("stick izq.", "left stick") : t("stick der.", "right stick");
  const dir = m[2] === "x" ? (m[3] === "+" ? t("derecha", "right") : t("izquierda", "left")) : (m[3] === "+" ? t("arriba", "up") : t("abajo", "down"));
  return `${stick} ${dir}`;
};
const textoPaso = (p) => p.tipo === "boton"
  ? t(`pulsa el botón que quieres que sea ${ETIQUETA[p.f]}`, `press the button you want to be ${ETIQUETA[p.f]}`)
  : t(`empuja el stick ${p.stick === "l" ? "IZQUIERDO" : "DERECHO"} hacia ${p.dir === "x" ? "la DERECHA" : "ARRIBA"} y suéltalo`,
    `push the ${p.stick === "l" ? "LEFT" : "RIGHT"} stick ${p.dir === "x" ? "RIGHT" : "UP"} and let go`);
const clavePaso = (p) => (p.tipo === "boton" ? p.f : `${p.stick}.${p.dir}`);

const guardar = (persona, perfil) => accion("mapeo.guardar", { parsecId: persona.parsecId, nombre: persona.nombre, perfil: limpiar(perfil) });

// ---- Lista de personas -------------------------------------------------------------------
function personasDe(m) {
  const perfiles = m.mapeo?.perfiles || {};
  const mandoDe = new Map((m.mandos?.lista || []).filter((p) => p.ocupado && p.parsecId).map((p) => [p.parsecId, p.n]));
  const lista = (m.invitados || []).filter((g) => !g.falso).map((g) => ({ parsecId: g.parsecId, nombre: g.nombre, presente: true, mando: mandoDe.get(g.parsecId) || 0 }));
  for (const [id, x] of Object.entries(perfiles)) {
    if (!lista.some((g) => String(g.parsecId) === id)) lista.push({ parsecId: Number(id), nombre: x.nombre || `#${id}`, presente: false, mando: 0 });
  }
  return lista.map((g) => ({ ...g, perfil: perfiles[String(g.parsecId)]?.perfil || null }));
}

function Personas({ lista, elegido, elegir }) {
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("PERSONAS", "PEOPLE")} derecha=${html`<span class="mono mut">${lista.filter((g) => g.perfil).length} ${t("con mapeo", "mapped")}</span>`}/>
    <div class="ayuda" style="margin-bottom:10px">${t("Cada persona tiene su propio mapeo. Se aplica solo cuando entra y toma un mando.", "Each person has their own mapping. It applies by itself when they join and take a pad.")}</div>
    ${lista.length === 0 ? html`<div class="ayuda">${t("Aún no hay invitados en la sala.", "No guests in the room yet.")}</div>` : null}
    <div style="display:flex;flex-direction:column;gap:8px">${lista.map((g) => html`
      <button class=${cx("persona", g.parsecId === elegido && "on")} key=${g.parsecId} onClick=${() => elegir(g.parsecId)}>
        <${Avatar} nombre=${g.nombre} id=${g.parsecId} t=${34}/>
        <div style="flex:1;min-width:0;text-align:left"><div class="nom">${g.nombre}</div>
          <div class="sub">
            ${g.mando ? html`<${Chip} tipo="acc">${t("MANDO", "PAD")} ${dos(g.mando)}</${Chip}>` : g.presente ? html`<${Chip}>${t("SIN MANDO", "NO PAD")}</${Chip}>` : html`<${Chip}>${t("NO ESTÁ", "AWAY")}</${Chip}>`}
            ${g.perfil ? html`<${Chip} tipo="vip">${t("MAPEO", "MAPPED")}</${Chip}>` : null}
          </div></div>
      </button>`)}</div>
  </${Tarjeta}>`;
}

// ---- Mando en vivo: llega / sale ------------------------------------------------------------
function EnVivo({ persona, vista, alRestablecer }) {
  const hay = vista && vista.parsecId === persona.parsecId && vista.edadMs >= 0;
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <${Titulo} texto=${t("MANDO EN VIVO", "LIVE PAD")} derecha=${persona.perfil ? html`<${Boton} tipo="peligro mini" al=${alRestablecer}>${t("RESTABLECER", "RESET")}</${Boton}>` : null}/>
    ${persona.mando ? null : html`<div class="caja" style="border-color:var(--warn);font-size:13px">${persona.presente
      ? t("Dale un puesto para ver su mando: solo llega lo que manda con un mando tomado.", "Give them a seat to see their pad: input only arrives while they hold a pad.")
      : t("No está en la sala. Su mapeo queda guardado y se aplica solo cuando vuelva.", "Not in the room. Their mapping stays saved and applies when they return.")}</div>`}
    <div class="rej2">
      <div class="caja" style="display:flex;flex-direction:column;gap:6px;padding:12px">
        <div class="lab mut" style="font-size:12px">${t("LLEGA (SU MANDO)", "IN (THEIR PAD)")}</div>
        <${MandoVivo} e=${hay ? vista.crudo : null}/>
      </div>
      <div class="caja" style=${`display:flex;flex-direction:column;gap:6px;padding:12px;${persona.perfil ? "border-color:var(--acc)" : ""}`}>
        <div class="lab acc" style="font-size:12px">${t("SALE AL JUEGO", "OUT TO THE GAME")}</div>
        <${MandoVivo} e=${hay ? vista.salida : null}/>
      </div>
    </div>
    <div class="mono" style="font-size:12px;text-align:center;min-height:16px;color:${hay && vista.ultima ? "var(--acc)" : "var(--mut)"}">
      ${hay ? (vista.ultima ? t(`Última pulsación: ${textoPulsacion(vista.ultima)}`, `Last press: ${textoPulsacion(vista.ultima)}`) : t("Recibiendo su mando…", "Receiving their pad…"))
        : t("Aún no llega nada de su mando: que pulse cualquier botón.", "Nothing from their pad yet: ask them to press any button.")}
    </div>
  </${Tarjeta}>`;
}

// ---- Asistente paso a paso -----------------------------------------------------------------
function Asistente({ persona, vista, perfil, alTerminar }) {
  const [paso, setPaso] = useState(-1);               // -1 = sin empezar · PASOS.length = resumen
  const [capturas, setCapturas] = useState({});
  const [eco, setEco] = useState("");
  const base = useRef(-1);
  const n = vista && vista.parsecId === persona.parsecId ? vista.n : -1;

  // Cada pulsación nueva (contador del motor) que encaje con el paso actual se anota y avanza.
  useEffect(() => {
    if (paso < 0 || paso >= PASOS.length) { base.current = n; return; }
    if (base.current < 0) { base.current = n; return; }
    if (n === base.current) return;
    base.current = n;
    const p = PASOS[paso];
    if (!encaja(p, vista?.ultima)) return;
    setCapturas((c) => ({ ...c, [clavePaso(p)]: vista.ultima }));
    setEco(t(`Pulsó ${textoPulsacion(vista.ultima)} → será ${p.tipo === "boton" ? ETIQUETA[p.f] : "✓"}`, `Pressed ${textoPulsacion(vista.ultima)} → becomes ${p.tipo === "boton" ? ETIQUETA[p.f] : "✓"}`));
    const id = setTimeout(() => setPaso((x) => (x === paso ? x + 1 : x)), 650);
    return () => clearTimeout(id);
  }, [n, paso]);
  useEffect(() => { setEco(""); }, [paso]);
  useEffect(() => { setPaso(-1); setCapturas({}); }, [persona.parsecId]);

  const avisarChat = (p) => accion("chat.enviar", { texto: `@${persona.nombre} ${textoPaso(p)}` }, { ok: t("Enviado al chat.", "Sent to chat.") });
  const empezar = () => { base.current = n; setCapturas({}); setPaso(0); };

  if (paso < 0) {
    return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("ASISTENTE PASO A PASO", "STEP-BY-STEP WIZARD")}/>
      <div class="ayuda">${t(`Te dice qué pedirle a ${persona.nombre}: botón por botón y luego los sticks. Lo que pulse en cada paso pasa a ser ese botón. Puedes saltar los que ya estén bien.`,
        `Tells you what to ask ${persona.nombre}: button by button, then the sticks. Whatever they press becomes that button. Skip the ones already right.`)}</div>
      <div><${Boton} tipo="ok" deshabilitado=${!persona.mando} al=${empezar}>${t("EMPEZAR", "START")}</${Boton}></div>
      ${persona.mando ? null : html`<div class="ayuda" style="font-size:12px">${t("Necesita tener un mando tomado.", "They need to hold a pad.")}</div>`}
    </${Tarjeta}>`;
  }

  if (paso >= PASOS.length) {
    const { perfil: nuevo, avisos } = construirPerfil(perfil, capturas);
    const cambios = FUENTES.filter((f) => destinoDe(nuevo, f).join() !== destinoDe(normalizar(perfil), f).join());
    const ejes = [];
    if (nuevo.cambiarSticks !== normalizar(perfil).cambiarSticks) ejes.push(nuevo.cambiarSticks ? t("sticks cambiados", "sticks swapped") : t("sticks en su sitio", "sticks back"));
    for (const k of ["lx", "ly", "rx", "ry"]) if (nuevo.invertir.includes(k) !== normalizar(perfil).invertir.includes(k)) ejes.push(`${k.toUpperCase()} ${nuevo.invertir.includes(k) ? t("invertido", "inverted") : t("normal", "normal")}`);
    return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("ASISTENTE · RESUMEN", "WIZARD · SUMMARY")}/>
      ${cambios.length === 0 && ejes.length === 0 ? html`<div class="ayuda">${t("No hay nada que cambiar: su mando ya estaba bien.", "Nothing to change: their pad was already fine.")}</div>` : html`
        <div style="display:flex;flex-wrap:wrap;gap:6px">
          ${cambios.map((f) => html`<${Chip} tipo="acc" key=${f}>${ETIQUETA[f]} → ${destinoDe(nuevo, f).map((d) => ETIQUETA[d]).join("+") || t("nada", "none")}</${Chip}>`)}
          ${ejes.map((x) => html`<${Chip} tipo="vip" key=${x}>${x}</${Chip}>`)}
        </div>`}
      ${avisos.includes("ejes_cruzados") ? html`<div class="caja" style="border-color:var(--warn);font-size:13px">${t("Un stick movió el eje equivocado (horizontal por vertical). Eso no se corrige aquí: revisa su mando.", "A stick moved the wrong axis. That can't be fixed here: check their pad.")}</div>` : null}
      <div style="display:flex;gap:8px;flex-wrap:wrap">
        <${Boton} tipo="ok" al=${async () => { if (await guardar(persona, nuevo)) { setPaso(-1); alTerminar && alTerminar(); } }}>${t("GUARDAR MAPEO", "SAVE MAPPING")}</${Boton}>
        <${Boton} al=${() => setPaso(PASOS.length - 1)}>${t("ATRÁS", "BACK")}</${Boton}>
        <${Boton} tipo="suave" al=${() => setPaso(-1)}>${t("DESCARTAR", "DISCARD")}</${Boton}>
      </div>
    </${Tarjeta}>`;
  }

  const p = PASOS[paso];
  const ya = capturas[clavePaso(p)];
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
    <${Titulo} texto=${t("ASISTENTE PASO A PASO", "STEP-BY-STEP WIZARD")} derecha=${html`<span class="mono mut">${paso + 1} / ${PASOS.length}</span>`}/>
    <div style="height:4px;border-radius:2px;background:var(--tenue);overflow:hidden"><div style=${`height:100%;width:${(paso / PASOS.length) * 100}%;background:var(--acc);transition:width .2s`}></div></div>
    <div style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
      <span class="disp acc" style="font-size:40px;font-weight:700;line-height:1;min-width:64px;text-align:center">${p.tipo === "boton" ? ETIQUETA[p.f] : p.stick === "l" ? "L" : "R"}${p.tipo === "eje" ? (p.dir === "x" ? " ▶" : " ▲") : ""}</span>
      <div style="flex:1;min-width:220px;font-size:16px">${t("Pide a", "Ask")} <b>${persona.nombre}</b>: ${textoPaso(p)}</div>
    </div>
    <div class="mono" style="font-size:13px;min-height:18px;color:${eco ? "var(--ok)" : "var(--mut)"}">${eco || (ya ? t(`Antes pulsó ${textoPulsacion(ya)}`, `Earlier pressed ${textoPulsacion(ya)}`) : t("Esperando…", "Waiting…"))}</div>
    <div style="display:flex;gap:8px;flex-wrap:wrap">
      <${Boton} tipo="mini" al=${() => setPaso(paso + 1)}>${t("SALTAR (ESTÁ BIEN)", "SKIP (IT'S FINE)")}</${Boton}>
      <${Boton} tipo="mini" deshabilitado=${paso === 0} al=${() => setPaso(paso - 1)}>${t("ATRÁS", "BACK")}</${Boton}>
      <${Boton} tipo="mini" al=${() => avisarChat(p)}>${t("AVISAR POR CHAT", "TELL VIA CHAT")}</${Boton}>
      <${Boton} tipo="suave mini" al=${() => setPaso(-1)}>${t("CANCELAR", "CANCEL")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

// ---- Zona muerta con vista en vivo -------------------------------------------------------------
function Zona({ etiqueta, valor, x, y, al }) {
  const [v, setV] = useState(valor);
  useEffect(() => setV(valor), [valor]);
  const R = 46, px = 60 + ((x || 0) / 32767) * R, py = 60 - ((y || 0) / 32767) * R;
  const dentro = Math.hypot(x || 0, y || 0) / 32767 * 100 <= v;
  return html`<div class="caja" style="display:flex;gap:14px;align-items:center;padding:12px;flex-wrap:wrap">
    <svg viewBox="0 0 120 120" width="96" height="96" role="img" aria-label=${etiqueta}>
      <circle cx="60" cy="60" r=${R} fill="rgba(255,255,255,.04)" stroke="rgba(255,255,255,.28)"/>
      <circle cx="60" cy="60" r=${(R * v) / 100} fill="var(--acc-suave)" stroke="var(--acc)" stroke-dasharray="3 3"/>
      <circle cx=${px} cy=${py} r="5" fill=${dentro ? "var(--mut)" : "var(--acc)"}/>
    </svg>
    <div style="flex:1;min-width:160px;display:flex;flex-direction:column;gap:6px">
      <div style="display:flex;justify-content:space-between"><b>${etiqueta}</b><span class="mono acc">${v}%</span></div>
      <input type="range" min="0" max=${ZONA_MAX} value=${v} aria-label=${etiqueta} onInput=${(e) => setV(Number(e.currentTarget.value))} onChange=${(e) => al(Number(e.currentTarget.value))}/>
      <div class="ayuda" style="font-size:12px">${dentro && (x || y) ? t("Ahora se ignora (dentro de la zona).", "Ignored right now (inside the zone).") : t("El punto es lo que llega; dentro del círculo no se mueve nada. «Ajustar solo»: con el stick suelto.", "The dot is what arrives; inside the circle nothing moves. «Auto»: with the stick released.")}</div>
      <div><${Boton} tipo="mini" al=${() => al(zonaSugerida(x, y))}>${t("AJUSTAR SOLO", "AUTO")}</${Boton}></div>
    </div>
  </div>`;
}

function Sticks({ persona, perfil, vista }) {
  const p = normalizar(perfil);
  const hay = vista && vista.parsecId === persona.parsecId && vista.edadMs >= 0;
  const c = hay ? vista.crudo : {};
  // Para la zona se mira el stick que llega a cada lado (tras cambiar sticks)
  const [lx, ly, rx, ry] = p.cambiarSticks ? [c.rx, c.ry, c.lx, c.ly] : [c.lx, c.ly, c.rx, c.ry];
  const cambiar = (f) => guardar(persona, f(normalizar(perfil)));
  const inv = (k) => (v) => cambiar((x) => ({ ...x, invertir: v ? [...x.invertir, k] : x.invertir.filter((y) => y !== k) }));
  const zona = (i) => (v) => cambiar((x) => { const z = [...x.zona]; z[i] = v; return { ...x, zona: z }; });
  return html`<${Tarjeta} secundaria interior="padding:22px 24px;display:flex;flex-direction:column;gap:6px">
    <${Titulo} texto=${t("STICKS", "STICKS")}/>
    <${AjusteSw} titulo=${t("Cambiar stick izquierdo por derecho", "Swap left and right sticks")} desc=${t("Para mandos que mandan un stick por el otro.", "For pads that send one stick as the other.")} valor=${p.cambiarSticks} al=${(v) => cambiar((x) => ({ ...x, cambiarSticks: v }))}/>
    <${AjusteSw} titulo=${t("Invertir stick izq. horizontal (LX)", "Invert left stick X")} valor=${p.invertir.includes("lx")} al=${inv("lx")}/>
    <${AjusteSw} titulo=${t("Invertir stick izq. vertical (LY)", "Invert left stick Y")} desc=${t("Si arriba va abajo.", "If up goes down.")} valor=${p.invertir.includes("ly")} al=${inv("ly")}/>
    <${AjusteSw} titulo=${t("Invertir stick der. horizontal (RX)", "Invert right stick X")} valor=${p.invertir.includes("rx")} al=${inv("rx")}/>
    <${AjusteSw} titulo=${t("Invertir stick der. vertical (RY)", "Invert right stick Y")} valor=${p.invertir.includes("ry")} al=${inv("ry")}/>
    <div class="lab mut" style="font-size:12px;margin-top:10px">${t("ZONA MUERTA (STICK QUE CAMINA SOLO)", "DEADZONE (DRIFTING STICK)")}</div>
    <div class="rej2">
      <${Zona} etiqueta=${t("Stick izquierdo", "Left stick")} valor=${p.zona[0]} x=${lx} y=${ly} al=${zona(0)}/>
      <${Zona} etiqueta=${t("Stick derecho", "Right stick")} valor=${p.zona[1]} x=${rx} y=${ry} al=${zona(1)}/>
    </div>
  </${Tarjeta}>`;
}

// ---- Botones uno a uno ---------------------------------------------------------------------------
function Botones({ persona, perfil }) {
  const p = normalizar(perfil);
  const opciones = [...FUENTES.map((f) => ({ valor: f, texto: nom(f) })), { valor: "", texto: t("— nada (desactivado)", "— nothing (disabled)") }];
  return html`<${Tarjeta} secundaria interior="padding:22px 24px">
    <${Titulo} texto=${t("BOTONES UNO A UNO", "BUTTONS ONE BY ONE")}/>
    <div class="ayuda" style="margin-bottom:10px">${t("Izquierda: lo que pulsa en su mando. Derecha: lo que recibe el juego. Un gatillo puede ser un botón y al revés.", "Left: what they press. Right: what the game gets. A trigger can be a button and vice versa.")}</div>
    <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(min(250px,100%),1fr));gap:8px">
      ${FUENTES.map((f) => {
        const d = destinoDe(p, f);
        const cambiado = d.join() !== f;
        return html`<div class="caja" key=${f} style=${`display:flex;align-items:center;gap:10px;padding:8px 10px;${cambiado ? "border-color:var(--acc)" : ""}`}>
          <span class="mono" style="min-width:52px;font-weight:700">${ETIQUETA[f]}</span><span class="mut">→</span>
          <${Selector} estilo="flex:1;min-width:0" valor=${d.length > 1 ? d[0] : d[0] || ""} opciones=${opciones}
            al=${(v) => guardar(persona, conDestino(p, f, v ? [v] : []))}/>
        </div>`;
      })}
    </div>
  </${Tarjeta}>`;
}

// ---- Pestaña --------------------------------------------------------------------------------------
export function MapeoNuevo({ m }) {
  const lista = personasDe(m);
  const [elegido, setElegido] = useState(() => tomarPendienteMapeo() || (lista.find((g) => g.mando) || lista[0])?.parsecId || 0);
  const persona = lista.find((g) => g.parsecId === elegido) || null;
  useEffect(() => { accion("mapeo.ver", { parsecId: persona ? persona.parsecId : 0 }, { silencioso: true }); }, [persona?.parsecId]);
  useEffect(() => { if (!persona && lista.length) setElegido((lista.find((g) => g.mando) || lista[0]).parsecId); }, [lista.length, !!persona]);

  const vista = m.mapeo?.vista || null;
  const restablecer = async () => {
    if (!(await confirmar({ titulo: t(`¿Restablecer el mando de ${persona.nombre}?`, `Reset ${persona.nombre}'s pad?`),
      texto: t("Su mando vuelve a llegar tal cual, sin cambios.", "Their pad goes back to arriving as is."), si: t("Restablecer", "Reset"), peligro: true }))) return;
    await accion("mapeo.restablecer", { parsecId: persona.parsecId }, { ok: t("Mapeo restablecido.", "Mapping reset.") });
  };
  return html`<div class="fila partible" style="align-items:flex-start">
    <div class="col" style="flex:0 1 320px;min-width:min(260px,100%)"><${Personas} lista=${lista} elegido=${elegido} elegir=${setElegido}/></div>
    <div class="col" style="flex:1 1 0;min-width:min(340px,100%)">
      ${!persona ? html`<${Vacio} titulo=${t("Elige a una persona", "Pick a person")} texto=${t("Cuando haya invitados, aparecen a la izquierda.", "Guests show up on the left.")}/>` : html`
        <${EnVivo} persona=${persona} vista=${vista} alRestablecer=${restablecer}/>
        <${Asistente} persona=${persona} vista=${vista} perfil=${persona.perfil}/>
        <${Sticks} persona=${persona} perfil=${persona.perfil} vista=${vista}/>
        <${Botones} persona=${persona} perfil=${persona.perfil}/>
        ${persona.perfil && !esIdentidad(persona.perfil) ? null : html`<div class="ayuda" style="font-size:12px">${t("Sin mapeo: su mando llega al juego tal cual.", "No mapping: their pad reaches the game as is.")}</div>`}`}
    </div>
  </div>`;
}
