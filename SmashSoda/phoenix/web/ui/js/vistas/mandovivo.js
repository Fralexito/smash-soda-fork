// =============================================================================
//  MANDO EN VIVO (2D): lo que el juego recibe ahora mismo de cada mando virtual.
//  Botones que se iluminan, sticks que se mueven y gatillos que se llenan.
//  Sirve para comprobar antes del partido que cada invitado tiene bien su mando.
//  Datos: mandos.lista[i].entrada {b, lt, rt, lx, ly, rx, ry} (solo en Mandos › Puestos).
// =============================================================================
import { html } from "../lib.js";
import { t } from "../i18n.js";
import { dos } from "../ui.js";

// Máscara XInput (la misma de botones.js y del motor de mandos)
const B = { arriba: 0x1, abajo: 0x2, izq: 0x4, der: 0x8, start: 0x10, back: 0x20, ls: 0x40, rs: 0x80, lb: 0x100, rb: 0x200, guia: 0x400, a: 0x1000, b: 0x2000, x: 0x4000, y: 0x8000 };
const COLOR_CARA = { a: "#3ddc97", b: "#ff6b6b", x: "#4da3ff", y: "#ffd23f" };
const APAGADO = "rgba(255,255,255,.10)";
const BORDE = "rgba(255,255,255,.28)";

const on = (e, bit) => ((e?.b || 0) & bit) !== 0;
const eje = (v) => Math.max(-1, Math.min(1, (v || 0) / 32767));

function Stick({ cx, cy, x, y, pulsado }) {
  const px = cx + eje(x) * 9, py = cy - eje(y) * 9;   // Y de XInput va hacia arriba
  return html`<g>
    <circle cx=${cx} cy=${cy} r="15" fill=${APAGADO} stroke=${pulsado ? "var(--acc)" : BORDE} stroke-width=${pulsado ? 2.5 : 1.2}/>
    <circle cx=${px} cy=${py} r="8" fill=${pulsado || Math.abs(eje(x)) > .2 || Math.abs(eje(y)) > .2 ? "var(--acc)" : "rgba(255,255,255,.35)"}/>
  </g>`;
}

function Gatillo({ x, valor, letra }) {
  const alto = 22, lleno = Math.round(((valor || 0) / 255) * alto);
  return html`<g>
    <rect x=${x} y="4" width="26" height=${alto} rx="5" fill=${APAGADO} stroke=${BORDE}/>
    <rect x=${x} y=${4 + alto - lleno} width="26" height=${lleno} rx="5" fill="var(--acc)"/>
    <text x=${x + 13} y="19" text-anchor="middle" font-size="9" font-weight="700" fill="#fff">${letra}</text>
  </g>`;
}

/** Un mando dibujado con su estado en vivo. */
export function MandoVivo({ e }) {
  const cara = (k, cx, cy) => html`<g>
    <circle cx=${cx} cy=${cy} r="8.5" fill=${on(e, B[k]) ? COLOR_CARA[k] : APAGADO} stroke=${on(e, B[k]) ? COLOR_CARA[k] : BORDE}/>
    <text x=${cx} y=${cy + 3.2} text-anchor="middle" font-size="9" font-weight="700" fill=${on(e, B[k]) ? "#0b0b14" : "rgba(255,255,255,.7)"}>${k.toUpperCase()}</text>
  </g>`;
  const cruz = (k, x, y, w, h) => html`<rect x=${x} y=${y} width=${w} height=${h} rx="2" fill=${on(e, B[k]) ? "var(--acc)" : APAGADO} stroke=${BORDE}/>`;
  const chico = (k, cx, cy, r = 5) => html`<circle cx=${cx} cy=${cy} r=${r} fill=${on(e, B[k]) ? "var(--acc)" : APAGADO} stroke=${BORDE}/>`;
  const hombro = (k, x) => html`<rect x=${x} y="30" width="44" height="9" rx="4.5" fill=${on(e, B[k]) ? "var(--acc)" : APAGADO} stroke=${BORDE}/>`;
  return html`<svg viewBox="0 0 240 150" role="img" aria-label=${t("Mando en vivo", "Live pad")} style="width:100%;height:auto;display:block">
    ${html`<${Gatillo} x=${34} valor=${e?.lt} letra="LT"/>`}
    ${html`<${Gatillo} x=${180} valor=${e?.rt} letra="RT"/>`}
    ${hombro("lb", 26)}${hombro("rb", 170)}
    <path d="M60 42 Q120 34 180 42 Q214 46 224 92 Q232 140 200 144 Q182 146 168 122 Q156 106 120 106 Q84 106 72 122 Q58 146 40 144 Q8 140 16 92 Q26 46 60 42 Z"
      fill="rgba(255,255,255,.04)" stroke=${BORDE} stroke-width="1.4"/>
    ${html`<${Stick} cx=${62} cy=${70} x=${e?.lx} y=${e?.ly} pulsado=${on(e, B.ls)}/>`}
    ${html`<${Stick} cx=${150} cy=${104} x=${e?.rx} y=${e?.ry} pulsado=${on(e, B.rs)}/>`}
    ${cruz("arriba", 84, 88, 10, 11)}${cruz("abajo", 84, 110, 10, 11)}${cruz("izq", 72, 99, 12, 10)}${cruz("der", 94, 99, 12, 10)}
    ${cara("y", 180, 56)}${cara("a", 180, 84)}${cara("x", 166, 70)}${cara("b", 194, 70)}
    ${chico("back", 104, 66)}${chico("start", 136, 66)}${chico("guia", 120, 54, 6.5)}
  </svg>`;
}

/** Rejilla con un mando por puesto ocupado (o conectado). */
export function MandosEnVivo({ m }) {
  const lista = (m.mandos?.lista || []).filter((p) => p.conectado && (p.ocupado || p.entrada?.b));
  if (lista.length === 0) return html`<div class="ayuda">${t("No hay mandos en uso ahora mismo.", "No pads in use right now.")}</div>`;
  return html`<div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:12px">
    ${lista.map((p) => html`<div class="caja" key=${p.n} style="display:flex;flex-direction:column;gap:6px;padding:12px">
      <div style="display:flex;justify-content:space-between;align-items:center;gap:8px">
        <b style="white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${p.jugador || t("Libre", "Free")}</b>
        <span class="mono mut" style="font-size:12px">${t("MANDO", "PAD")} ${dos(p.n)}</span>
      </div>
      <${MandoVivo} e=${p.entrada}/>
      ${p.entrada ? null : html`<div class="ayuda" style="font-size:12px">${t("Sin datos de este mando.", "No data for this pad.")}</div>`}
    </div>`)}
  </div>`;
}
