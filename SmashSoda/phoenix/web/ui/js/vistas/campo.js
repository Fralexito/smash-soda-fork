// =============================================================================
//  CAMPO 2D EN VIVO: las 22 fichitas y el balón de PES 2021 como el radar del modo entrenador.
//  El motor (lector de solo lectura de Phoenix Link) manda el evento «campo» ~20 veces por segundo
//  mientras esta tarjeta está abierta; aquí se interpola para que las fichitas se muevan suaves.
//  Evento «campo»: { j: [[x, z, eq], …], b: [x, z, y] | null }  (metros, centro del campo = 0,0;
//  eq: 0 local, 1 visitante, 2 asistente/árbitro, -1 sin equipo conocido).
//  Estado m.campo: { estado: apagado | esperando | buscando | activo | no_compatible, motivo }.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, oirCampo } from "../tienda.js";
import { Tarjeta, Titulo, Chip, Vacio } from "../ui.js";

const LARGO = 105, ANCHO = 68;
const MX = 6, MZ = 5; // margen alrededor del campo

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

/** Dibuja el campo y mueve las fichitas con suavidad (animación directa sobre el SVG, sin redibujar Preact). */
export function CampoSvg({ vivo }) {
  const refs = useRef({ puntos: [], bola: null, obj: { j: [], b: null }, cur: { j: [], b: null }, t0: 0 });
  const [n, setN] = useState(0);
  useEffect(() => {
    const R = refs.current;
    const quitar = oirCampo((d) => {
      R.obj = { j: d.j || [], b: d.b || null };
      if ((d.j || []).length !== n) setN((d.j || []).length);
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
          if (!cur.b) cur.b = [b[0], b[1]];
          else { cur.b[0] += (b[0] - cur.b[0]) * Math.min(1, k * 1.6); cur.b[1] += (b[1] - cur.b[1]) * Math.min(1, k * 1.6); }
          el.setAttribute("opacity", "1");
          el.setAttribute("transform", `translate(${cur.b[0].toFixed(2)} ${cur.b[1].toFixed(2)})`);
        }
      }
    };
    raf = requestAnimationFrame(paso);
    return () => { quitar(); cancelAnimationFrame(raf); };
  }, []);
  const R = refs.current;
  const pts = [];
  for (let i = 0; i < Math.max(n, 0); i++) {
    const eq = R.obj.j[i] ? R.obj.j[i][2] : -1;
    const ref = (el) => { R.puntos[i] = el; };
    pts.push(html`<circle key=${i} ref=${ref} r=${eq === 2 ? 0.9 : 1.35} fill=${COLOR[eq] || COLOR[-1]}
      stroke="rgba(0,0,0,.55)" stroke-width="0.25" cx="0" cy="0"/>`);
  }
  return html`<svg viewBox=${`${-LARGO / 2 - MX} ${-ANCHO / 2 - MZ} ${LARGO + MX * 2} ${ANCHO + MZ * 2}`} role="img"
      aria-label=${t("Campo con los jugadores en vivo", "Pitch with live players")} style="width:100%;height:auto;display:block;border-radius:10px;background:#0d3b24">
    <defs><pattern id="cesped" width="8.75" height="1" patternUnits="userSpaceOnUse"><rect width="4.375" height="1" fill="rgba(255,255,255,.035)"/></pattern></defs>
    <rect x=${-LARGO / 2} y=${-ANCHO / 2} width=${LARGO} height=${ANCHO} fill="url(#cesped)"/>
    <${Lineas}/>
    <g>${pts}</g>
    <g ref=${(el) => { R.bola = el; }} opacity="0"><circle r="1.5" fill="rgba(0,0,0,.35)" cx="0.3" cy="0.4"/><circle r="0.95" fill="#fff" stroke="#111" stroke-width="0.25"/></g>
  </svg>`;
}

export function CampoEnVivo({ m }) {
  const c = m.campo || { estado: "apagado", motivo: "" };
  useEffect(() => {
    accion("campo.vivo", { si: true }, { silencioso: true });
    return () => { accion("campo.vivo", { si: false }, { silencioso: true }); };
  }, []);
  const activo = c.estado === "activo";
  const msg = MENSAJES[c.estado];
  return html`<div class="col">
    <${Tarjeta} vivo=${activo}>
      <${Titulo} texto=${t("CAMPO EN VIVO", "LIVE PITCH")}/>
      <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-bottom:12px">
        <${Chip} tipo=${activo ? "acc" : ""}>${activo ? t("EN VIVO", "LIVE") : t("SIN SEÑAL", "NO SIGNAL")}</${Chip}>
        ${c.jugadores ? html`<span class="mut mono" style="font-size:12px">${c.jugadores} ${t("jugadores", "players")}${c.balon ? " · " + t("balón", "ball") : ""}</span>` : null}
      </div>
      <${CampoSvg} vivo=${activo}/>
      ${!activo && msg ? html`<div class="ayuda" style="margin-top:10px">${t(msg[0], msg[1])}${c.motivo ? html` <span class="mono mut">(${c.motivo})</span>` : null}</div>` : null}
      <div class="ayuda" style="margin-top:10px;font-size:12px">${t("Solo lee la memoria del juego (no escribe ni cambia nada). Si la versión del juego no coincide, se apaga sola.", "Read-only: it never writes to or changes the game. If the game version doesn't match it switches itself off.")}</div>
    </${Tarjeta}>
  </div>`;
}
