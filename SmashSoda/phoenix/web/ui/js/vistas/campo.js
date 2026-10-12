// =============================================================================
//  CAMPO 2D EN VIVO: las 22 fichitas y el balón de PES 2021 como el radar del modo entrenador.
//  El motor (lector de solo lectura de Phoenix Link) manda el evento «campo» ~20 veces por segundo
//  mientras esta tarjeta está abierta; aquí se interpola para que las fichitas se muevan suaves.
//  Evento «campo»: { j: [[x, z, eq], …], b: [x, z, y] | null, t?: { l: [[eq, gol], …], e, listo, vuelo } }
//  (metros, centro del campo = 0,0; eq: 0 equipo A (izquierda en el primer saque), 1 equipo B, 2 árbitro/asistente,
//  -1 sin equipo aún; t = tanda de penales, solo cuando hay una).
//  Estado m.campo: { estado: apagado | esperando | buscando | activo | no_compatible, motivo, equipos, balon, tanda }.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, oirCampo } from "../tienda.js";
import { Tarjeta, Titulo, Chip, Icono } from "../ui.js";

const LARGO = 105, ANCHO = 68;
const MX = 7, MZ = 13; // margen alrededor del campo (en la tanda de penales los que esperan se ponen fuera de la banda)

const MENSAJES = {
  apagado: ["La lectura del campo está apagada.", "Pitch reading is off."],
  esperando: ["Esperando a que empiece un partido en PES…", "Waiting for a match to start in PES…"],
  buscando: ["Buscando a los jugadores en el juego…", "Looking for the players in the game…"],
  no_compatible: ["Esta versión del juego no es compatible: el campo se apagó solo para no molestar al partido.", "This game version isn't supported: the pitch turned itself off to keep the match safe."],
};

/** Líneas del campo (reglamentarias, en metros). */
function Lineas() {
  const hx = LARGO / 2, hz = ANCHO / 2;
  const area = (lado) => { // lado = ±1
    const x0 = lado * hx, g = (a) => x0 - lado * a;
    return html`<g>
      <rect x=${Math.min(x0, g(16.5))} y=${-20.16} width=${16.5} height=${40.32}/>
      <rect x=${Math.min(x0, g(5.5))} y=${-9.16} width=${5.5} height=${18.32}/>
      <circle cx=${g(11)} cy=${0} r=${0.35} fill="currentColor" stroke="none"/>
      <path d=${`M ${g(16.5)} ${-7.3} A 9.15 9.15 0 0 ${lado > 0 ? 0 : 1} ${g(16.5)} ${7.3}`}/>
      <rect x=${lado > 0 ? x0 : x0 - 2} y=${-3.66} width=${2} height=${7.32} stroke-width="0.3"/>
    </g>`;
  };
  return html`<g fill="none" stroke="currentColor" stroke-width="0.35" style="color:rgba(255,255,255,.55)">
    <rect x=${-hx} y=${-hz} width=${LARGO} height=${ANCHO}/>
    <line x1="0" y1=${-hz} x2="0" y2=${hz}/>
    <circle cx="0" cy="0" r="9.15"/><circle cx="0" cy="0" r="0.35" fill="currentColor" stroke="none"/>
    ${area(-1)}${area(1)}
  </g>`;
}

const COLOR = { 0: "var(--acc)", 1: "var(--eq-b, #b07bff)", 2: "#f5c451", "-1": "#cfd8ee" };
const NOMBRE_EQ = { 0: ["Equipo A", "Team A"], 1: ["Equipo B", "Team B"] };

/** Dibuja el campo y mueve las fichitas con suavidad (animación directa sobre el SVG, sin redibujar Preact). */
export function CampoSvg({ alTanda }) {
  const refs = useRef({ puntos: [], bola: null, obj: { j: [], b: null }, cur: { j: [], b: null }, t0: 0 });
  const [n, setN] = useState(0);
  const [clave, setClave] = useState("");
  useEffect(() => {
    const R = refs.current;
    const quitar = oirCampo((d) => {
      R.obj = { j: d.j || [], b: d.b || null };
      // Solo se redibuja con Preact si cambia el número de fichitas o algún equipo (colores); el movimiento va aparte
      const k = (d.j || []).map((p) => p[2]).join(",");
      if (k !== clave) { setClave(k); setN((d.j || []).length); }
      if (alTanda) alTanda(d.t || null);
    });
    let raf = 0, ultimo = performance.now();
    const paso = (ahora) => {
      raf = requestAnimationFrame(paso);
      const dt = Math.min(0.1, (ahora - ultimo) / 1000); ultimo = ahora;
      const k = 1 - Math.exp(-dt / 0.07); // ~70 ms de suavizado: sigue el ritmo de ~20 envíos/s sin saltos
      const { obj, cur, puntos } = R;
      for (let i = 0; i < obj.j.length; i++) {
        const o = obj.j[i];
        let c = cur.j[i];
        if (!c || Math.abs(o[0] - c[0]) > 15 || Math.abs(o[1] - c[1]) > 15) c = cur.j[i] = [o[0], o[1]]; // salto grande (reinicio, otro tiempo): sin deslizar
        else { c[0] += (o[0] - c[0]) * k; c[1] += (o[1] - c[1]) * k; }
        const el = puntos[i];
        if (el) { el.setAttribute("cx", c[0].toFixed(2)); el.setAttribute("cy", c[1].toFixed(2)); }
      }
      cur.j.length = obj.j.length;
      const b = obj.b, el = R.bola;
      if (el) {
        if (!b) el.setAttribute("opacity", "0");
        else {
          if (!cur.b || Math.abs(b[0] - cur.b[0]) > 20 || Math.abs(b[1] - cur.b[1]) > 20) cur.b = [b[0], b[1]];
          else { cur.b[0] += (b[0] - cur.b[0]) * Math.min(1, k * 1.6); cur.b[1] += (b[1] - cur.b[1]) * Math.min(1, k * 1.6); }
          el.setAttribute("opacity", "1");
          const alto = Math.min(1.6, 1 + (b[2] || 0) * 0.12); // en el aire se dibuja un poco más grande
          el.setAttribute("transform", `translate(${cur.b[0].toFixed(2)} ${cur.b[1].toFixed(2)}) scale(${alto.toFixed(2)})`);
        }
      }
    };
    raf = requestAnimationFrame(paso);
    return () => { quitar(); cancelAnimationFrame(raf); };
  }, [clave]);
  const R = refs.current;
  const pts = [];
  for (let i = 0; i < Math.max(n, 0); i++) {
    const eq = R.obj.j[i] ? R.obj.j[i][2] : -1;
    const ref = (el) => { R.puntos[i] = el; };
    pts.push(html`<circle key=${i} ref=${ref} r=${eq === 2 ? 0.9 : 1.35} fill=${COLOR[eq] || COLOR[-1]}
      stroke="rgba(0,0,0,.55)" stroke-width="0.25" cx="0" cy="0"/>`);
  }
  return html`<svg viewBox=${`${-LARGO / 2 - MX} ${-ANCHO / 2 - MZ} ${LARGO + MX * 2} ${ANCHO + MZ * 2}`} role="img"
      aria-label=${t("Campo con los jugadores en vivo", "Pitch with live players")} style="width:100%;height:auto;display:block;border-radius:10px;background:#0b3320">
    <defs><pattern id="cesped" width="8.75" height="1" patternUnits="userSpaceOnUse"><rect width="4.375" height="1" fill="rgba(255,255,255,.035)"/></pattern></defs>
    <rect x=${-LARGO / 2} y=${-ANCHO / 2} width=${LARGO} height=${ANCHO} fill="#0d3b24"/>
    <rect x=${-LARGO / 2} y=${-ANCHO / 2} width=${LARGO} height=${ANCHO} fill="url(#cesped)"/>
    <${Lineas}/>
    <g>${pts}</g>
    <g ref=${(el) => { R.bola = el; }} opacity="0"><circle r="1.5" fill="rgba(0,0,0,.35)" cx="0.3" cy="0.4"/><circle r="0.95" fill="#fff" stroke="#111" stroke-width="0.25"/></g>
  </svg>`;
}

/** Tanda de penales en vivo: quién va, quién anotó y quién falló. */
function Tanda({ tanda }) {
  if (!tanda) return null;
  const porEq = { 0: [], 1: [] };
  for (const [eq, gol] of tanda.l || []) if (porEq[eq]) porEq[eq].push(gol);
  const goles = (eq) => porEq[eq].filter(Boolean).length;
  const fila = (eq) => html`<div style="display:flex;align-items:center;gap:10px;padding:8px 0">
    <span style=${`width:12px;height:12px;border-radius:50%;background:${COLOR[eq]};flex:none`}></span>
    <b style="min-width:84px">${t(...NOMBRE_EQ[eq])}</b>
    <span class="mono" style="font-size:20px;min-width:28px">${goles(eq)}</span>
    <div style="display:flex;gap:6px;flex-wrap:wrap">${porEq[eq].map((g, i) => html`<span key=${i} title=${g ? t("Gol", "Goal") : t("Fallo o parada", "Missed or saved")}
      style=${`width:18px;height:18px;border-radius:50%;display:inline-flex;align-items:center;justify-content:center;font-size:11px;font-weight:700;background:${g ? "var(--ok)" : "var(--bad)"};color:#06101a`}>${g ? "✓" : "✗"}</span>`)}
      ${tanda.e === eq && tanda.listo ? html`<span class="punto ok vivo" style="margin-left:4px" title=${t("A punto de patear", "About to shoot")}></span>` : null}
    </div>
  </div>`;
  const quien = tanda.vuelo ? t("¡Balón en camino!", "Ball on its way!")
    : tanda.listo && NOMBRE_EQ[tanda.e] ? t(`A punto de patear: ${NOMBRE_EQ[tanda.e][0]}`, `About to shoot: ${NOMBRE_EQ[tanda.e][1]}`)
    : tanda.listo ? t("A punto de patear…", "About to shoot…") : t("Esperando al siguiente lanzamiento…", "Waiting for the next kick…");
  return html`<div class="caja" style="margin-top:12px">
    <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
      <b><${Icono} n="trofeo" t=${16}/> ${t("TANDA DE PENALES", "PENALTY SHOOTOUT")}</b>
      <span class=${tanda.vuelo || tanda.listo ? "chip acc" : "chip"}>${quien}</span>
    </div>
    ${fila(0)}${fila(1)}
    <div class="ayuda" style="font-size:12px">${t("Se lee del juego: gol si el balón cruza la línea entre los palos; si no, fallo o parada. Los nombres de los lanzadores aún no se leen.", "Read from the game: a goal when the ball crosses the line between the posts; otherwise missed or saved. Kickers' names aren't read yet.")}</div>
  </div>`;
}

export function CampoEnVivo({ m }) {
  const c = m.campo || { estado: "apagado", motivo: "" };
  const [tanda, setTanda] = useState(null);
  useEffect(() => {
    accion("campo.vivo", { si: true }, { silencioso: true });
    return () => { accion("campo.vivo", { si: false }, { silencioso: true }); };
  }, []);
  const activo = c.estado === "activo";
  const msg = MENSAJES[c.estado];
  const alTanda = (tt) => {
    // solo se redibuja cuando cambia algo de la tanda (no 20 veces por segundo)
    const k = tt ? JSON.stringify(tt) : "";
    setTanda((prev) => ((prev ? JSON.stringify(prev) : "") === k ? prev : tt));
  };
  return html`<div class="col">
    <${Tarjeta} vivo=${activo}>
      <${Titulo} texto=${t("CAMPO EN VIVO", "LIVE PITCH")}/>
      <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-bottom:12px">
        <${Chip} tipo=${activo ? "acc" : ""}>${activo ? t("EN VIVO", "LIVE") : t("SIN SEÑAL", "NO SIGNAL")}</${Chip}>
        ${c.jugadores ? html`<span class="mut mono" style="font-size:12px">${c.jugadores} ${t("fichitas", "pieces")}${c.balon ? " · " + t("balón", "ball") : ""}</span>` : null}
        ${activo ? html`<span style="display:flex;gap:12px;align-items:center;margin-left:auto;font-size:12px" class="mut">
          <span><span style=${`display:inline-block;width:10px;height:10px;border-radius:50%;background:${COLOR[0]};vertical-align:-1px;margin-right:5px`}></span>${t("Equipo A (izq. al empezar)", "Team A (left at kick-off)")}</span>
          <span><span style=${`display:inline-block;width:10px;height:10px;border-radius:50%;background:${COLOR[1]};vertical-align:-1px;margin-right:5px`}></span>${t("Equipo B", "Team B")}</span>
          <span><span style=${`display:inline-block;width:10px;height:10px;border-radius:50%;background:${COLOR[2]};vertical-align:-1px;margin-right:5px`}></span>${t("Árbitros", "Officials")}</span>
        </span>` : null}
      </div>
      <${CampoSvg} alTanda=${alTanda}/>
      ${activo && c.equipos === false ? html`<div class="ayuda" style="margin-top:10px">${t("Los equipos se colorean en el siguiente saque de centro (gol o descanso).", "Teams get their colours at the next kick-off (goal or half-time).")}</div>` : null}
      ${!activo && msg ? html`<div class="ayuda" style="margin-top:10px">${t(msg[0], msg[1])}${c.motivo ? html` <span class="mono mut">(${c.motivo})</span>` : null}</div>` : null}
      ${activo ? html`<${Tanda} tanda=${tanda}/>` : null}
      <div class="ayuda" style="margin-top:10px;font-size:12px">${t("Solo lee la memoria del juego (no escribe ni cambia nada). Si la versión del juego no coincide, se apaga sola.", "Read-only: it never writes to or changes the game. If the game version doesn't match it switches itself off.")}</div>
    </${Tarjeta}>
  </div>`;
}
