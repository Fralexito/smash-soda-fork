// =============================================================================
//  Piezas reutilizables del diseño v4 (tarjeta con esquinas cortadas,
//  interruptor, stepper, campos, iconos, minigráfica…).
// =============================================================================
import { html, useState, useEffect, useRef } from "./lib.js";

export const cx = (...c) => c.filter(Boolean).join(" ");

// ---- Iconos (trazos del diseño) ---------------------------------------------------
const TRAZOS = {
  sala: html`<circle cx="12" cy="12" r="3"/><circle cx="12" cy="12" r="8" stroke-dasharray="3 3"/>`,
  partido: html`<path d="M6 21V4h11l-2 4 2 4H6"/>`,
  mandos: html`<rect x="2.5" y="7" width="19" height="10" rx="5"/><path d="M8 10v4M6 12h4M16 11h.01M18 13h.01"/>`,
  gente: html`<circle cx="9" cy="8" r="3.5"/><path d="M2.5 20c0-4 3-6 6.5-6s6.5 2 6.5 6M16 5a3.5 3.5 0 010 6.5M18 14c2.5.7 3.5 2.7 3.5 6"/>`,
  sync: html`<path d="M4 12a8 8 0 0113.7-5.7L20 8M20 4v4h-4M20 12a8 8 0 01-13.7 5.7L4 16M4 20v-4h4"/>`,
  ajustes: html`<path d="M4 7h10M18 7h2M4 17h2M10 17h10"/><circle cx="16" cy="7" r="2"/><circle cx="8" cy="17" r="2"/>`,
  chat: html`<path d="M4 5h16v11H9l-5 4z"/>`,
  copiar: html`<rect x="8" y="8" width="12" height="12" rx="2"/><path d="M16 8V5a1 1 0 00-1-1H5a1 1 0 00-1 1v10a1 1 0 001 1h3"/>`,
  cerrar: html`<path d="M6 6l12 12M18 6L6 18"/>`,
  candado: html`<rect x="5" y="11" width="14" height="9" rx="2"/><path d="M8 11V8a4 4 0 018 0v3"/>`,
  enlace: html`<path d="M10 14a4 4 0 005.7 0l3-3a4 4 0 00-5.7-5.7l-1 1M14 10a4 4 0 00-5.7 0l-3 3a4 4 0 005.7 5.7l1-1"/>`,
  red: html`<path d="M3 17l5-6 4 3 5-7 4 4"/>`,
  buscar: html`<circle cx="11" cy="11" r="6"/><path d="M20 20l-4.5-4.5"/>`,
  mas: html`<path d="M12 5v14M5 12h14"/>`,
  menos: html`<path d="M5 12h14"/>`,
  intercambio: html`<path d="M7 7h12l-3-3M17 17H5l3 3"/>`,
  rayo: html`<path d="M13 3L5 14h6l-1 7 8-11h-6z"/>`,
  web: html`<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3c3 3.5 3 14.5 0 18M12 3c-3 3.5-3 14.5 0 18"/>`,
  trofeo: html`<path d="M8 4h8v5a4 4 0 01-8 0zM8 6H5a3 3 0 003 4M16 6h3a3 3 0 01-3 4M12 13v4M8 21h8M9 17h6"/>`,
  ojo: html`<path d="M2 12s4-7 10-7 10 7 10 7-4 7-10 7S2 12 2 12z"/><circle cx="12" cy="12" r="3"/>`,
  enviar: html`<path d="M4 12l16-8-6 16-2-7z"/>`,
  panel: html`<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M9 4v16"/>`,
  check: html`<path d="M5 12l5 5 9-10"/>`,
  alerta: html`<path d="M12 4l9 16H3zM12 10v4M12 17h.01"/>`,
};

export function Icono({ n, t = 20, w = 1.6, color = "currentColor" }) {
  return html`<svg width=${t} height=${t} viewBox="0 0 24 24" fill="none" stroke=${color} stroke-width=${w}
    stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${TRAZOS[n] || null}</svg>`;
}

// Logos de redes sociales (de relleno, no de trazo). Para sumar otra red: agregar su trazo aquí.
const REDES = {
  discord: { color: "#5865F2", d: "M20.317 4.3698a19.7913 19.7913 0 00-4.8851-1.5152.0741.0741 0 00-.0785.0371c-.211.3753-.4447.8648-.6083 1.2495-1.8447-.2762-3.68-.2762-5.4868 0-.1636-.3933-.4058-.8742-.6177-1.2495a.077.077 0 00-.0785-.037 19.7363 19.7363 0 00-4.8852 1.515.0699.0699 0 00-.0321.0277C.5334 9.0458-.319 13.5799.0992 18.0578a.0824.0824 0 00.0312.0561c2.0528 1.5076 4.0413 2.4228 5.9929 3.0294a.0777.0777 0 00.0842-.0276c.4616-.6304.8731-1.2952 1.226-1.9942a.076.076 0 00-.0416-.1057c-.6528-.2476-1.2743-.5495-1.8722-.8923a.077.077 0 01-.0076-.1277c.1258-.0943.2517-.1923.3718-.2914a.0743.0743 0 01.0776-.0105c3.9278 1.7933 8.18 1.7933 12.0614 0a.0739.0739 0 01.0785.0095c.1202.099.246.1981.3728.2924a.077.077 0 01-.0066.1276 12.2986 12.2986 0 01-1.873.8914.0766.0766 0 00-.0407.1067c.3604.698.7719 1.3628 1.225 1.9932a.076.076 0 00.0842.0286c1.961-.6067 3.9495-1.5219 6.0023-3.0294a.077.077 0 00.0313-.0552c.5004-5.177-.8382-9.6739-3.5485-13.6604a.061.061 0 00-.0312-.0286zM8.02 15.3312c-1.1825 0-2.1569-1.0857-2.1569-2.419 0-1.3332.9555-2.4189 2.157-2.4189 1.2108 0 2.1757 1.0952 2.1568 2.419 0 1.3332-.9555 2.4189-2.1569 2.4189zm7.9748 0c-1.1825 0-2.1569-1.0857-2.1569-2.419 0-1.3332.9554-2.4189 2.1569-2.4189 1.2108 0 2.1757 1.0952 2.1568 2.419 0 1.3332-.946 2.4189-2.1568 2.4189z" },
};
export function IconoRed({ red, t = 22, color }) {
  const r = REDES[red];
  if (!r) return null;
  return html`<svg width=${t} height=${t} viewBox="0 0 24 24" fill=${color || "currentColor"} aria-hidden="true"><path d=${r.d}/></svg>`;
}
export const COLOR_RED = (red) => (REDES[red] ? REDES[red].color : "#888");

export function Logo({ t = 36 }) {
  return html`<img src="logo.png" width=${t} height=${t} alt="Phoenix Link" draggable="false" style="display:block;flex:none"/>`;
}

// ---- Contenedores ----------------------------------------------------------------
export function Tarjeta({ children, clase = "", estilo = "", interior = "", vivo = false, suave = false }) {
  return html`<div class=${cx("cut", vivo && "vivo", suave && "suave", clase)} style=${estilo}>
    <div class="in" style=${interior}>${children}</div>
  </div>`;
}

export function Titulo({ texto, derecha = null, acc = true }) {
  return html`<div class="titulo-tarjeta"><div class=${cx("lab", acc && "acc")}>${texto}</div>${derecha}</div>`;
}

export function Vacio({ titulo, texto, children }) {
  return html`<div class="vacio"><div class="grande">${titulo}</div><div>${texto}</div>${children ? html`<div style="margin-top:16px">${children}</div>` : null}</div>`;
}

// ---- Controles ------------------------------------------------------------------
/** Botón que se bloquea mientras su acción async está en curso. */
export function Boton({ children, tipo = "", al, deshabilitado = false, titulo = "", estilo = "" }) {
  const [ocupado, setOcupado] = useState(false);
  const vivo = useRef(true);
  useEffect(() => () => { vivo.current = false; }, []);
  const clic = async (e) => {
    e.stopPropagation();
    if (!al || ocupado) return;
    const r = al(e);
    if (r && typeof r.then === "function") {
      setOcupado(true);
      try { await r; } finally { if (vivo.current) setOcupado(false); }
    }
  };
  return html`<button class=${cx("btn", tipo)} disabled=${deshabilitado || ocupado} title=${titulo} style=${estilo} onClick=${clic}>${children}</button>`;
}

export function Interruptor({ valor, al, deshabilitado = false, etiqueta = "" }) {
  return html`<button role="switch" aria-checked=${!!valor} aria-label=${etiqueta} class=${cx("sw", valor && "on")}
    disabled=${deshabilitado} onClick=${(e) => { e.stopPropagation(); al && al(!valor); }}></button>`;
}

/** Fila de ajuste: título, descripción y control a la derecha. */
export function Ajuste({ titulo, desc, children, bloque = false }) {
  return html`<div class=${cx("ajuste", bloque && "bloque")}>
    <div style="min-width:0"><div class="t">${titulo}</div>${desc ? html`<div class="d">${desc}</div>` : null}</div>
    ${children}
  </div>`;
}

export function AjusteSw({ titulo, desc, valor, al, deshabilitado }) {
  return html`<${Ajuste} titulo=${titulo} desc=${desc}><${Interruptor} valor=${valor} al=${al} deshabilitado=${deshabilitado} etiqueta=${titulo}/></${Ajuste}>`;
}

/**
 * Número editable: botones − / + y el valor se puede escribir a mano
 * (se guarda al salir del campo o con Enter; se ajusta al rango permitido).
 */
export function Stepper({ valor, min = 0, max = 99, paso = 1, al, sufijo = "", deshabilitado = false, ancho = 3 }) {
  const [borrador, setBorrador] = useState(String(valor ?? ""));
  const enfocado = useRef(false);
  useEffect(() => { if (!enfocado.current) setBorrador(String(valor ?? "")); }, [valor]);
  const fijar = (v) => { const n = Math.min(max, Math.max(min, Math.round(v))); setBorrador(String(n)); if (n !== valor) al(n); };
  const confirmar = () => {
    enfocado.current = false;
    const n = Number(String(borrador).replace(",", "."));
    if (borrador === "" || Number.isNaN(n)) setBorrador(String(valor ?? ""));
    else fijar(n);
  };
  return html`<div class=${cx("stepper", deshabilitado && "off")}>
    <button class="icono-btn" disabled=${deshabilitado || valor <= min} onClick=${() => fijar((valor ?? 0) - paso)} aria-label="Menos"><${Icono} n="menos" t=${16}/></button>
    <input class="v" inputmode="numeric" disabled=${deshabilitado} value=${borrador} title=${`${min}–${max}`}
      style=${`width:${Math.max(3, ancho) + 1.2}ch`}
      onFocus=${(e) => { enfocado.current = true; e.currentTarget.select(); }}
      onInput=${(e) => setBorrador(e.currentTarget.value.replace(/[^0-9.,-]/g, ""))}
      onBlur=${confirmar}
      onKeyDown=${(e) => {
        if (e.key === "Enter") e.currentTarget.blur();
        else if (e.key === "Escape") { setBorrador(String(valor ?? "")); enfocado.current = false; e.currentTarget.blur(); }
        else if (e.key === "ArrowUp") { e.preventDefault(); fijar((valor ?? 0) + paso); }
        else if (e.key === "ArrowDown") { e.preventDefault(); fijar((valor ?? 0) - paso); }
      }}/>
    ${sufijo ? html`<span class="mono mut" style="font-size:12px">${sufijo}</span>` : null}
    <button class="icono-btn" disabled=${deshabilitado || valor >= max} onClick=${() => fijar((valor ?? 0) + paso)} aria-label="Más"><${Icono} n="mas" t=${16}/></button>
  </div>`;
}

/** Número que sube o baja con una animación corta cuando cambia. */
export function Cifra({ valor, decimales = 0 }) {
  const [mostrado, setMostrado] = useState(typeof valor === "number" ? valor : 0);
  const desde = useRef(mostrado);
  useEffect(() => {
    if (typeof valor !== "number") return;
    const reducido = window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    if (reducido || Math.abs(valor - desde.current) < 1e-9) { desde.current = valor; setMostrado(valor); return; }
    const inicio = performance.now(), a = desde.current, d = 360;
    let id = 0;
    const paso = (t) => {
      const k = Math.min(1, (t - inicio) / d), e = 1 - Math.pow(1 - k, 3);
      const v = a + (valor - a) * e;
      desde.current = v; setMostrado(v);
      if (k < 1) id = requestAnimationFrame(paso);
    };
    id = requestAnimationFrame(paso);
    return () => cancelAnimationFrame(id);
  }, [valor]);
  if (typeof valor !== "number") return html`${valor ?? "—"}`;
  return html`${mostrado.toFixed(decimales)}`;
}

export function Segmentos({ opciones, valor, al }) {
  return html`<div class="seg">${opciones.map((o) => html`
    <button class=${o.valor === valor ? "on" : ""} onClick=${() => o.valor !== valor && al(o.valor)}>${o.texto}</button>`)}</div>`;
}

export function Selector({ valor, opciones, al, estilo = "", deshabilitado = false }) {
  return html`<select class="campo" style=${estilo} disabled=${deshabilitado} value=${String(valor)}
    onChange=${(e) => al(e.currentTarget.value)}>
    ${opciones.map((o) => html`<option value=${String(o.valor)}>${o.texto}</option>`)}
  </select>`;
}

/**
 * Campo de texto que no se pisa con el estado del motor mientras se escribe:
 * guarda al salir del campo o con Enter (en multilínea, Ctrl+Enter).
 */
export function Campo({ valor, al, max = 200, multilinea = false, placeholder = "", mono = false, estilo = "", soloNumeros = false }) {
  const [borrador, setBorrador] = useState(valor ?? "");
  const enfocado = useRef(false);
  useEffect(() => { if (!enfocado.current) setBorrador(valor ?? ""); }, [valor]);
  const guardar = () => {
    enfocado.current = false;
    const v = soloNumeros ? borrador.replace(/\D/g, "") : borrador;
    if (v !== (valor ?? "")) al(v);
  };
  const props = {
    class: cx("campo", mono && "mono"), style: estilo, value: borrador, maxLength: max, placeholder,
    onFocus: () => { enfocado.current = true; },
    onInput: (e) => setBorrador(e.currentTarget.value),
    onBlur: guardar,
    onKeyDown: (e) => {
      if (e.key === "Enter" && (!multilinea || e.ctrlKey)) { e.preventDefault(); e.currentTarget.blur(); }
      if (e.key === "Escape") { setBorrador(valor ?? ""); enfocado.current = false; e.currentTarget.blur(); }
    },
  };
  return multilinea ? html`<textarea ...${props}></textarea>` : html`<input type="text" ...${props}/>`;
}

// ---- Datos ---------------------------------------------------------------------
const COLORES_AVATAR = ["#00e5ff", "#8000ff", "#3ddc97", "#ffb547", "#ff6b80", "#3b82f6", "#f5c451", "#c084fc"];
export function Avatar({ nombre = "?", url = "", t = 40, brillo = false, id = 0 }) {
  const [roto, setRoto] = useState(false);
  const inicial = (String(nombre).trim()[0] || "?").toUpperCase();
  const color = COLORES_AVATAR[(Number(id) || nombre.length) % COLORES_AVATAR.length];
  const estilo = `width:${t}px;height:${t}px;font-size:${Math.round(t * 0.45)}px;` +
    (brillo ? "background:linear-gradient(135deg,var(--acc),var(--acc2));" : `background:${color};`);
  return html`<span class=${cx("avatar", brillo && "brillo")} style=${estilo}>
    ${url && /^https:\/\//.test(url) && !roto ? html`<img src=${url} alt="" referrerpolicy="no-referrer" onError=${() => setRoto(true)}/>` : inicial}
  </span>`;
}

export function colorPing(ms, umbral = [50, 120]) {
  if (ms == null || ms < 0) return "var(--mut)";
  if (ms <= umbral[0]) return "var(--ok)";
  if (ms <= umbral[1]) return "var(--warn)";
  return "var(--bad)";
}
export function claseSemaforo(s) {
  return s === "verde" ? "ok" : s === "ambar" ? "warn" : s === "rojo" ? "bad" : "";
}

/** Minigráfica de ping (SVG). */
export function Minigrafica({ serie = [], alto = 40, max = null, color = "var(--acc)" }) {
  const datos = serie.filter((x) => x != null && x >= 0);
  if (datos.length < 2) return html`<div style=${`height:${alto}px`} class="mut mono" >—</div>`;
  const tope = max || Math.max(120, ...datos) * 1.1;
  const ancho = 100;
  const pts = datos.map((v, i) => `${(i / (datos.length - 1)) * ancho},${alto - (Math.min(v, tope) / tope) * (alto - 2) - 1}`).join(" ");
  const y60 = alto - (60 / tope) * (alto - 2) - 1;
  const y100 = alto - (100 / tope) * (alto - 2) - 1;
  return html`<svg class="spark" viewBox=${`0 0 ${ancho} ${alto}`} preserveAspectRatio="none" style=${`height:${alto}px`}>
    ${y100 > 0 ? html`<line x1="0" x2=${ancho} y1=${y100} y2=${y100} stroke="rgba(255,107,128,.35)" stroke-dasharray="2 2" vector-effect="non-scaling-stroke"/>` : null}
    ${y60 > 0 ? html`<line x1="0" x2=${ancho} y1=${y60} y2=${y60} stroke="rgba(255,181,71,.3)" stroke-dasharray="2 2" vector-effect="non-scaling-stroke"/>` : null}
    <polyline points=${pts} fill="none" stroke=${color} stroke-width="1.8" vector-effect="non-scaling-stroke" stroke-linejoin="round"/>
  </svg>`;
}

export function Chip({ children, tipo = "" }) {
  return html`<span class=${cx("chip", tipo)}>${children}</span>`;
}

export function dos(n) { return String(n).padStart(2, "0"); }
