// =============================================================================
//  PARTIDO (nuevo): armar lados, marcador en vivo con reloj, cambio de lados
//  (cruza los mandos), final con historial y tabla de la sala.
//  Integración: la web recibe partida_inicio / pausa / partida_fin y la sala
//  sale del radar mientras se juega.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t, duracion, fecha } from "../i18n.js";
import { accion, confirmar, avisar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Interruptor, Campo, Avatar, Chip, Icono, Vacio, colorPing, cx, dos } from "../ui.js";

export const PESTANAS_PARTIDO = [
  { id: "vivo", es: "EN VIVO", en: "LIVE", d: ["Arma los dos lados, lleva el marcador y el reloj. Al terminar se guarda y se avisa en el chat y la web.", "Set up sides, keep score and time."] },
  { id: "historial", es: "HISTORIAL", en: "HISTORY", d: ["Partidos jugados en esta PC y la tabla de la sala (3 puntos por victoria).", "Matches played on this PC and the room table."] },
];

export function VistaPartido({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.sala) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  if (pestana === "historial") return html`<${Historial}/>`;
  const fase = m.partido?.fase || "libre";
  if (fase === "libre") return html`<${Armar} m=${m}/>`;
  return html`<${Marcador} m=${m}/>`;
}

// ---- Armar el partido ---------------------------------------------------------------------
let borradorLados = { a: [], b: [], nombreA: "", nombreB: "", asignar: true };

function participantes(m) {
  const r = [];
  if (m.sala?.hostId) r.push({ parsecId: m.sala.hostId, nombre: m.sala.hostNombre || t("Anfitrión", "Host"), host: true, ping: -1, mando: m.mandos?.host || 0 });
  for (const g of m.invitados || []) if (!g.falso) r.push(g);
  return r;
}

function Armar({ m }) {
  const [b, setB] = useState(borradorLados);
  const fijar = (x) => { borradorLados = { ...b, ...x }; setB(borradorLados); };
  const gente = participantes(m);
  const presentes = new Set(gente.map((g) => g.parsecId));
  const a = b.a.filter((id) => presentes.has(id));
  const bb = b.b.filter((id) => presentes.has(id));
  const lado = (id) => (a.includes(id) ? "a" : bb.includes(id) ? "b" : null);
  const mover = (id, destino) => {
    const na = a.filter((x) => x !== id), nb = bb.filter((x) => x !== id);
    if (destino === "a" && na.length < 4) na.push(id);
    if (destino === "b" && nb.length < 4) nb.push(id);
    fijar({ a: na, b: nb });
  };
  const desdeMandos = () => {
    const lista = m.mandos?.lista || [];
    fijar({
      a: lista.filter((p) => p.ocupado && p.equipo === "local" && p.parsecId).map((p) => p.parsecId).slice(0, 4),
      b: lista.filter((p) => p.ocupado && p.equipo === "visitante" && p.parsecId).map((p) => p.parsecId).slice(0, 4),
    });
  };
  const preparar = () => accion("partido.preparar", {
    a, b: bb, nombreA: b.nombreA.trim() || nombreDe(gente, a) || "Local", nombreB: b.nombreB.trim() || nombreDe(gente, bb) || t("Visitante", "Away"), asignar: b.asignar,
  });
  if (!m.sala.abierta) {
    return html`<${Vacio} titulo=${t("Abre la sala para armar un partido", "Open the room to set up a match")}
      texto=${t("El marcador, el reloj y el historial funcionan con la gente que está en la sala.", "Score, clock and history work with people in the room.")}/>`;
  }
  return html`<div class="col">
    <div class="fila partible">
      <${Columna} id="a" ids=${a} nombre=${b.nombreA} clave="nombreA" color="var(--acc)" gente=${gente} m=${m} fijar=${fijar} mover=${mover}/>
      <${Tarjeta} estilo="width:min(360px,34%);flex:none" interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
        <${Titulo} texto=${t("EN LA SALA", "IN THE ROOM")} derecha=${html`<${Boton} tipo="mini suave" al=${desdeMandos}>${t("SEGÚN MANDOS", "FROM PADS")}</${Boton}>`}/>
        ${gente.map((g) => html`<div class="persona" style="cursor:default" key=${g.parsecId}>
          <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${m.perfiles?.[g.parsecId]?.avatar_url} t=${34} brillo=${!!lado(g.parsecId)}/>
          <div style="flex:1;min-width:0"><div class="nom">${g.nombre}${g.host ? html` <${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}</div>
            <div class="mono mut" style="font-size:12px">${g.mando ? `${t("MANDO", "PAD")} ${dos(g.mando)}` : t("sin mando", "no pad")}</div></div>
          <button class=${cx("btn mini", lado(g.parsecId) === "a" && "lleno")} onClick=${() => mover(g.parsecId, lado(g.parsecId) === "a" ? null : "a")}>A</button>
          <button class=${cx("btn mini", lado(g.parsecId) === "b" && "lleno")} onClick=${() => mover(g.parsecId, lado(g.parsecId) === "b" ? null : "b")}>B</button>
        </div>`)}
      </${Tarjeta}>
      <${Columna} id="b" ids=${bb} nombre=${b.nombreB} clave="nombreB" color="var(--eq-b)" gente=${gente} m=${m} fijar=${fijar} mover=${mover}/>
    </div>
    <div class="caja" style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
      <${Interruptor} valor=${b.asignar} al=${(v) => fijar({ asignar: v })} etiqueta="asignar"/>
      <div style="flex:1;min-width:220px"><b>${t("Sentar a cada uno en su mando", "Seat everyone on their pad")}</b>
        <div class="ayuda">${t("Ajusta la formación (A local, B visita) y asigna los mandos en orden.", "Sets the formation and assigns pads in order.")}</div></div>
      <${Boton} tipo="lleno enorme" deshabilitado=${a.length === 0 || bb.length === 0} al=${preparar}><${Icono} n="partido" t=${18}/>${t("PREPARAR PARTIDO", "SET UP MATCH")}</${Boton}>
    </div>
  </div>`;
}

function Columna({ id, ids, nombre, clave, color, gente, m, fijar, mover }) {
  return html`<${Tarjeta} estilo="flex:1;min-width:0" interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <div class="lab" style=${`color:${color}`}>${id === "a" ? t("LADO A · LOCAL", "SIDE A · HOME") : t("LADO B · VISITA", "SIDE B · AWAY")}</div>
    <${Campo} valor=${nombre} max=${40} placeholder=${t("Nombre del equipo (opcional)", "Team name (optional)")} al=${(v) => fijar({ [clave]: v })}/>
    <div style="display:flex;flex-direction:column;gap:8px;min-height:120px">
      ${ids.length === 0 ? html`<div class="ayuda">${t("Toca a alguien de la lista para sumarlo (máx. 4).", "Tap someone to add them (max 4).")}</div>` : null}
      ${ids.map((pid) => { const g = gente.find((x) => x.parsecId === pid); return html`<button class="persona on" key=${pid} onClick=${() => mover(pid, null)}>
        <${Avatar} nombre=${g?.nombre} id=${pid} url=${m.perfiles?.[pid]?.avatar_url} t=${34}/><div class="nom" style="flex:1;text-align:left">${g?.nombre}</div><span class="mut">×</span></button>`; })}
    </div>
  </${Tarjeta}>`;
}

function nombreDe(gente, ids) {
  if (ids.length !== 1) return "";
  return gente.find((g) => g.parsecId === ids[0])?.nombre || "";
}

// ---- Marcador ----------------------------------------------------------------------------
function Numero({ valor, lado }) {
  const ref = useRef(null);
  const previo = useRef(valor);
  useEffect(() => {
    if (valor > previo.current && ref.current) {
      ref.current.classList.remove("pop");
      void ref.current.offsetWidth;
      ref.current.classList.add("pop");
    }
    previo.current = valor;
  }, [valor]);
  return html`<div ref=${ref} class=${cx("num", lado)}>${valor}</div>`;
}

function Rafaga() {
  // Celebración breve (solo transform/opacity; se respeta reduced-motion por CSS)
  const piezas = Array.from({ length: 28 }, (_, i) => i);
  return html`<div class="rafaga" aria-hidden="true">${piezas.map((i) => html`<i key=${i} style=${`--x:${Math.cos(i / 28 * 6.283) * (120 + (i % 5) * 30)}px;--y:${Math.sin(i / 28 * 6.283) * (90 + (i % 4) * 26)}px;--d:${(i % 7) * 18}ms`}></i>`)}</div>`;
}

function Marcador({ m }) {
  const p = m.partido;
  const [fiesta, setFiesta] = useState(0);
  const enJuego = p.fase === "en_juego" || p.fase === "pausado";
  const gol = (lado, delta) => {
    if (delta > 0) setFiesta((x) => x + 1);
    return accion("partido.gol", { lado, delta });
  };
  const finalizar = async () => {
    const res = `${p.a.nombre} ${p.a.goles} - ${p.b.goles} ${p.b.nombre}`;
    if (!(await confirmar({ titulo: t("¿Terminar el partido?", "End the match?"), texto: t(`Resultado final: ${res}. Se guarda en el historial y se avisa en el chat.`, `Final: ${res}. Saved and announced.`), si: t("Terminar", "End") }))) return;
    const r = await accion("partido.finalizar", { anunciar: true });
    if (r?.registro) {
      const g = r.registro.ganador;
      setFiesta((x) => x + 1);
      avisar(g === "empate" ? t("¡Empate!", "Draw!") : t(`¡Ganó ${g === "a" ? p.a.nombre : p.b.nombre}!`, `${g === "a" ? p.a.nombre : p.b.nombre} wins!`), "ok", 6000);
      if (!r.guardado) avisar(t("No se pudo guardar en el historial.", "Could not save to history."), "warn");
    }
  };
  const cancelar = async () => {
    if (await confirmar({ titulo: t("¿Cancelar el partido?", "Cancel the match?"), texto: t("No se guarda en el historial.", "It won't be saved."), si: t("Cancelar partido", "Cancel match"), no: t("Seguir", "Keep"), peligro: true })) {
      await accion("partido.cancelar");
    }
  };
  const cambiarLados = () => accion("partido.cambiarLados", { anunciar: true }, { ok: t("Mandos cruzados.", "Pads swapped.") });
  const ids = [...p.a.jugadores, ...p.b.jugadores].map((j) => j.parsecId);
  const red = (m.red || []).filter((r) => ids.includes(r.parsecId));
  return html`<div class="col">
    <${Tarjeta} vivo=${p.fase === "en_juego"} interior="padding:30px 34px;position:relative;overflow:hidden">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:18px">
        <span class=${cx("chip", p.fase === "en_juego" ? "acc" : p.fase === "pausado" ? "vip" : "")}>
          ${p.fase === "en_juego" ? html`<span class="punto ok vivo"></span> ${t("EN JUEGO", "LIVE")}` : p.fase === "pausado" ? t("PAUSADO", "PAUSED") : t("LISTO PARA EMPEZAR", "READY")}</span>
        <span class="reloj">${duracion(p.segundos)}</span>
        ${p.invertido ? html`<${Chip} tipo="acc2">${t("LADOS CAMBIADOS", "SIDES SWAPPED")}</${Chip}>` : html`<span></span>`}
      </div>
      <div class="marcador">
        <${Equipo} l=${p.a} clave="a" enJuego=${enJuego} gol=${gol}/>
        <div class="goles"><${Numero} valor=${p.a.goles} lado="a"/><span class="sep">:</span><${Numero} valor=${p.b.goles} lado="b"/></div>
        <${Equipo} l=${p.b} clave="b" enJuego=${enJuego} gol=${gol}/>
      </div>
      ${fiesta ? html`<${Rafaga} key=${fiesta}/>` : null}
    </${Tarjeta}>
    <div class="caja" style="display:flex;gap:10px;flex-wrap:wrap;align-items:center">
      ${p.fase === "listo" ? html`<${Boton} tipo="lleno enorme" al=${() => accion("partido.iniciar", { anunciar: true })}><${Icono} n="rayo" t=${18}/>${t("¡EMPEZAR!", "KICK OFF!")}</${Boton}>` : null}
      ${p.fase === "en_juego" ? html`<${Boton} tipo="warn" al=${() => accion("partido.pausa", { si: true })}>${t("PAUSA", "PAUSE")}</${Boton}>` : null}
      ${p.fase === "pausado" ? html`<${Boton} tipo="lleno" al=${() => accion("partido.pausa", { si: false })}>${t("REANUDAR", "RESUME")}</${Boton}>` : null}
      <${Boton} al=${cambiarLados}><${Icono} n="intercambio" t=${16}/>${t("CAMBIAR LADOS", "SWAP SIDES")}</${Boton}>
      <div style="margin-left:auto;display:flex;gap:10px">
        ${enJuego ? html`<${Boton} tipo="ok" al=${finalizar}><${Icono} n="trofeo" t=${16}/>${t("TERMINAR", "FINISH")}</${Boton}>` : null}
        <${Boton} tipo="peligro" al=${cancelar}>${t("CANCELAR", "CANCEL")}</${Boton}>
      </div>
    </div>
    ${red.length ? html`<div class="rej4">${red.map((r) => html`<div class="caja" key=${r.parsecId}>
      <div style="display:flex;justify-content:space-between"><b style="white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${r.nombre}</b>
        <span class="mono" style=${`color:${colorPing(r.ultimo)}`}>${r.ultimo >= 0 ? r.ultimo + " ms" : "—"}</span></div>
      <${MiniSerie} serie=${r.serie}/>
    </div>`)}</div>` : null}
  </div>`;
}

function Equipo({ l, clave, enJuego, gol }) {
  return html`<div class=${cx("equipo", clave)}>
    <div class="lab" style=${`color:${clave === "a" ? "var(--acc)" : "var(--eq-b)"}`}>${clave === "a" ? "LOCAL" : t("VISITA", "AWAY")}</div>
    <div class="nom">${l.nombre}</div>
    <div style=${`display:flex;gap:6px;flex-wrap:wrap;justify-content:${clave === "a" ? "flex-start" : "flex-end"}`}>
      ${l.jugadores.map((j) => html`<${Chip} key=${j.parsecId}>${j.nombre}</${Chip}>`)}</div>
    ${enJuego ? html`<div style="display:flex;gap:8px">
      <button class="btn redondo" aria-label="-1" disabled=${l.goles <= 0} onClick=${() => gol(clave, -1)}><${Icono} n="menos" t=${16}/></button>
      <button class="btn lleno" onClick=${() => gol(clave, 1)}><${Icono} n="mas" t=${16}/>${t("GOL", "GOAL")}</button>
    </div>` : null}
  </div>`;
}

function MiniSerie({ serie }) {
  const d = (serie || []).slice(-60).map((v) => (v < 0 ? 0 : v));
  if (d.length < 2) return null;
  const max = Math.max(120, ...d);
  return html`<svg class="spark" viewBox="0 0 100 30" preserveAspectRatio="none" style="height:30px;margin-top:8px">
    <polyline fill="none" stroke="var(--acc)" stroke-width="1.6" vector-effect="non-scaling-stroke"
      points=${d.map((v, i) => `${(i / (d.length - 1)) * 100},${29 - (v / max) * 28}`).join(" ")}/></svg>`;
}

// ---- Historial y tabla ------------------------------------------------------------------------
function tabla(partidos) {
  const t2 = new Map();
  const sumar = (j, gf, gc, res) => {
    const k = j.parsecId || j.nombre;
    const x = t2.get(k) || { nombre: j.nombre, pj: 0, g: 0, e: 0, p: 0, gf: 0, gc: 0 };
    x.nombre = j.nombre || x.nombre;
    x.pj++; x.gf += gf; x.gc += gc;
    if (res === 1) x.g++; else if (res === 0) x.e++; else x.p++;
    t2.set(k, x);
  };
  for (const r of partidos) {
    const ra = r.ganador === "a" ? 1 : r.ganador === "empate" ? 0 : -1;
    for (const j of r.a.jugadores) sumar(j, r.a.goles, r.b.goles, ra);
    for (const j of r.b.jugadores) sumar(j, r.b.goles, r.a.goles, -ra);
  }
  return [...t2.values()].map((x) => ({ ...x, pts: x.g * 3 + x.e })).sort((a, b) => b.pts - a.pts || (b.gf - b.gc) - (a.gf - a.gc) || b.gf - a.gf);
}

function Historial() {
  const [lista, setLista] = useState(null);
  useEffect(() => { accion("partido.historial", { max: 200 }, { silencioso: true }).then((r) => setLista(r?.partidos || [])); }, []);
  if (!lista) return html`<${Vacio} titulo=${t("Cargando…", "Loading…")} texto=""/>`;
  if (lista.length === 0) return html`<${Vacio} titulo=${t("Aún no hay partidos", "No matches yet")} texto=${t("Cuando termines un partido desde «En vivo» quedará aquí con su marcador y duración.", "Finished matches show up here.")}/>`;
  const tab = tabla(lista);
  return html`<div class="fila partible">
    <${Tarjeta} estilo="flex:1.2;min-width:0" interior="padding:22px 24px">
      <${Titulo} texto=${t("ÚLTIMOS PARTIDOS", "LATEST MATCHES")} derecha=${html`<span class="mono mut">${lista.length}</span>`}/>
      <div style="display:flex;flex-direction:column;gap:8px">${lista.slice(0, 80).map((r) => html`<div class="caja" key=${r.id + "-" + r.inicioMs} style="display:grid;grid-template-columns:1fr auto 1fr;gap:12px;align-items:center">
        <div style=${`font-weight:${r.ganador === "a" ? 700 : 500};white-space:nowrap;overflow:hidden;text-overflow:ellipsis`}>${r.a.nombre}</div>
        <div class="disp" style="font-size:22px;font-weight:700;text-align:center">${r.a.goles} <span class="mut">:</span> ${r.b.goles}</div>
        <div style=${`text-align:right;font-weight:${r.ganador === "b" ? 700 : 500};white-space:nowrap;overflow:hidden;text-overflow:ellipsis`}>${r.b.nombre}</div>
        <div class="mono mut" style="grid-column:1/-1;font-size:12px;display:flex;justify-content:space-between"><span>${fecha(r.inicioMs)}</span><span>${duracion(r.duracionSeg)}</span></div>
      </div>`)}</div>
    </${Tarjeta}>
    <${Tarjeta} estilo="flex:1;min-width:0" interior="padding:22px 24px">
      <${Titulo} texto=${t("TABLA DE LA SALA", "ROOM TABLE")}/>
      <table class="tabla"><thead><tr><th>#</th><th>${t("JUGADOR", "PLAYER")}</th><th>PJ</th><th>G</th><th>E</th><th>P</th><th>DG</th><th>PTS</th></tr></thead>
        <tbody>${tab.slice(0, 30).map((x, i) => html`<tr key=${x.nombre + i}>
          <td class="mono ${i < 3 ? "acc" : "mut"}">${i + 1}</td><td style="font-weight:600">${x.nombre}</td>
          <td class="mono">${x.pj}</td><td class="mono">${x.g}</td><td class="mono">${x.e}</td><td class="mono">${x.p}</td>
          <td class="mono">${x.gf - x.gc > 0 ? "+" : ""}${x.gf - x.gc}</td><td class="mono acc" style="font-weight:700">${x.pts}</td></tr>`)}</tbody></table>
      <div class="ayuda" style="margin-top:12px">${t("3 puntos por victoria y 1 por empate. Cuenta los partidos guardados en esta PC.", "3 pts per win, 1 per draw. Counts matches saved on this PC.")}</div>
    </${Tarjeta}>
  </div>`;
}
