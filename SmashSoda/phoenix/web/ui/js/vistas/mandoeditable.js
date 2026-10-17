// =============================================================================
//  MANDO EDITABLE (SVG): la silueta de un mando con piezas que se tocan.
//  Lo usan Partido › Más › Bloqueo (tocar = bloquear) y Partido › Más › Teclado (tocar =
//  cambiar su tecla). Mismo estilo que el mando en vivo (mandovivo.js).
//  Coordenadas en un lienzo de 640×400; POS da el centro de cada pieza.
// =============================================================================
import { html } from "../lib.js";
import { cx } from "../ui.js";

export const COLOR_CARA = { a: "#3ddc97", b: "#ff6b6b", x: "#4da3ff", y: "#ffd23f" };

// Cuerpo con dos asas; el hueco de abajo queda bajo la cruceta y el stick derecho.
const CUERPO = "M150 95 Q320 75 490 95 Q585 105 610 230 Q632 370 548 382 Q500 388 468 345 Q440 312 320 312 Q200 312 172 345 Q140 388 92 382 Q8 370 30 230 Q55 105 150 95 Z";

export const POS = {
  lt: [130, 38], rt: [510, 38], lb: [135, 92], rb: [505, 92],
  guia: [320, 112], back: [282, 152], start: [358, 152],
  stickL: [160, 180], stickR: [400, 262],
  arriba: [255, 226], abajo: [255, 298], izq: [207, 262], der: [303, 262],
  y: [480, 140], a: [480, 220], x: [432, 180], b: [528, 180],
};
export const PASO_X = 48, PASO_Y = 36;   // separación de las piezas de una cruz (stick o cruceta)

/** Lienzo con la silueta; las piezas van dentro. */
export function Lienzo({ children, etiqueta }) {
  return html`<svg class="mando-svg" viewBox="0 0 640 400" role="group" aria-label=${etiqueta}>
    <path d=${CUERPO} class="mando-cuerpo"/>
    ${children}
  </svg>`;
}

/** Aro de un stick (decorativo, o tocable si trae `al`). */
export function Aro({ x, y, r = 58, estado = "", titulo = "", al = null }) {
  if (!al) return html`<circle cx=${x} cy=${y} r=${r} class="mando-aro"/>`;
  return html`<${Pieza} x=${x} y=${y} w=${r * 2} redonda estado=${cx("aro", estado)} titulo=${titulo} al=${al} texto="" />`;
}

function Candado({ x, y }) {
  return html`<g class="pz-cand" transform=${`translate(${x} ${y})`}>
    <circle r="10"/>
    <path d="M-4 -1 h8 v6 h-8 z M-2.6 -1 v-2.2 a2.6 2.6 0 0 1 5.2 0 v2.2"/>
  </g>`;
}

/**
 * Una pieza tocable. x, y = centro. Con `arriba` y `abajo` lleva dos líneas
 * (p. ej. «LT» y la tecla); si no, solo `texto`. estado: "" | "bloq" | "esperando" | "vacio".
 */
export function Pieza({ x, y, w = 46, h = 32, redonda = false, estado = "", acento = "", texto = "", arriba = "", abajo = "", dy = 0, titulo = "", candado = false, al }) {
  const tecla = (e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); al && al(); } };
  const r = w / 2;
  return html`<g class=${cx("pz", estado, acento && "cara")} style=${acento ? `--pz-acento:${acento}` : ""}
      role="button" tabindex="0" aria-label=${titulo} aria-pressed=${estado.includes("bloq")} onClick=${() => al && al()} onKeyDown=${tecla}>
    <title>${titulo}</title>
    ${redonda ? html`<circle class="fondo" cx=${x} cy=${y} r=${r}/>`
      : html`<rect class="fondo" x=${x - w / 2} y=${y - h / 2} width=${w} height=${h} rx=${Math.min(10, h / 2)}/>`}
    ${arriba || abajo
      ? html`<text class="t1" x=${x} y=${y - 4} text-anchor="middle">${arriba}</text>
             <text class="t2" x=${x} y=${y + 11} text-anchor="middle">${abajo}</text>`
      : html`<text class="t" x=${x} y=${y + 5 + dy} text-anchor="middle">${texto}</text>`}
    ${candado ? html`<${Candado} x=${redonda ? x + r * 0.72 : x + w / 2 - 2} y=${redonda ? y - r * 0.72 : y - h / 2 + 2}/>` : null}
  </g>`;
}
