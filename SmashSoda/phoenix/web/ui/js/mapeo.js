// =============================================================================
//  MAPEO DE MANDOS — lógica pura de la interfaz (espejo de phoenix/core/MapeoMandos.cpp).
//  Perfil (mismo JSON que el motor):
//    { botones: { fuente: [salidas…] } (solo lo que cambia; [] = desactivado),
//      invertir: ["lx"|"ly"|"rx"|"ry"], cambiarSticks: bool, zona: [izq%, der%] }
//  Fuentes/salidas: los 16 bits XInput por nombre + "lt" y "rt".
//  La usan la pestaña Mandos › Mapeo (asistente y vista previa) y el simulador.
// =============================================================================

export const BIT = { arriba: 0x1, abajo: 0x2, izq: 0x4, der: 0x8, start: 0x10, back: 0x20, ls: 0x40, rs: 0x80, lb: 0x100, rb: 0x200, guia: 0x400, a: 0x1000, b: 0x2000, x: 0x4000, y: 0x8000 };
export const FUENTES = ["a", "b", "x", "y", "lb", "rb", "lt", "rt", "back", "start", "guia", "ls", "rs", "arriba", "abajo", "izq", "der"];
export const EJES = ["lx", "ly", "rx", "ry"];
export const UMBRAL_GATILLO = 100;   // gatillo → botón (igual que el motor)
export const ZONA_MAX = 60;

export const ETIQUETA = { a: "A", b: "B", x: "X", y: "Y", lb: "LB", rb: "RB", lt: "LT", rt: "RT", back: "BACK", start: "START", guia: "GUÍA", ls: "L3", rs: "R3", arriba: "▲", abajo: "▼", izq: "◀", der: "▶" };

export function perfilVacio() { return { botones: {}, invertir: [], cambiarSticks: false, zona: [0, 0] }; }

/** Normaliza lo que llega del motor (o nada) a un perfil completo. */
export function normalizar(p) {
  const v = perfilVacio();
  if (!p || typeof p !== "object") return v;
  for (const [k, d] of Object.entries(p.botones || {})) if (FUENTES.includes(k) && Array.isArray(d)) v.botones[k] = d.filter((x) => FUENTES.includes(x));
  v.invertir = (p.invertir || []).filter((x) => EJES.includes(x));
  v.cambiarSticks = !!p.cambiarSticks;
  v.zona = [0, 1].map((i) => Math.max(0, Math.min(ZONA_MAX, Math.round(Number(p.zona?.[i]) || 0))));
  return v;
}

/** Salidas de una fuente (por defecto, ella misma). */
export function destinoDe(p, f) { return p.botones?.[f] ?? [f]; }

/** Quita las entradas que equivalen a la identidad (el motor guarda solo lo que cambia). */
export function limpiar(p) {
  const v = normalizar(p);
  for (const [k, d] of Object.entries(v.botones)) if (d.length === 1 && d[0] === k) delete v.botones[k];
  return v;
}

export function esIdentidad(p) {
  const v = limpiar(p);
  return Object.keys(v.botones).length === 0 && v.invertir.length === 0 && !v.cambiarSticks && !v.zona[0] && !v.zona[1];
}

export function conDestino(p, fuente, salidas) {
  const v = normalizar(p);
  v.botones[fuente] = salidas;
  return limpiar(v);
}

const limitar = (x) => Math.max(-32768, Math.min(32767, x));
function zonaMuerta(x, y, pc) {
  if (!pc) return [x, y];
  const zona = 32767 * Math.min(pc, ZONA_MAX) / 100, mag = Math.hypot(x, y);
  if (mag <= zona) return [0, 0];
  const f = (mag - zona) / (32767 - zona) * 32767 / mag;
  return [limitar(Math.round(x * f)), limitar(Math.round(y * f))];
}

/** Aplica el perfil a un estado {b, lt, rt, lx, ly, rx, ry} (igual que el motor). */
export function aplicar(perfil, e) {
  const p = normalizar(perfil), s = { b: 0, lt: 0, rt: 0, lx: 0, ly: 0, rx: 0, ry: 0 };
  const salida = (lista, analogico) => {
    for (const d of lista) {
      if (d === "lt") s.lt = Math.max(s.lt, analogico);
      else if (d === "rt") s.rt = Math.max(s.rt, analogico);
      else if (analogico >= UMBRAL_GATILLO) s.b |= BIT[d];
    }
  };
  for (const f of FUENTES) {
    if (f === "lt" || f === "rt") { if (e[f]) salida(destinoDe(p, f), e[f]); }
    else if ((e.b || 0) & BIT[f]) salida(destinoDe(p, f), 255);
  }
  let [lx, ly, rx, ry] = p.cambiarSticks ? [e.rx, e.ry, e.lx, e.ly] : [e.lx, e.ly, e.rx, e.ry];
  const inv = (k, v) => (p.invertir.includes(k) ? limitar(-(v || 0)) : (v || 0));
  lx = inv("lx", lx); ly = inv("ly", ly); rx = inv("rx", rx); ry = inv("ry", ry);
  [s.lx, s.ly] = zonaMuerta(lx, ly, p.zona[0]);
  [s.rx, s.ry] = zonaMuerta(rx, ry, p.zona[1]);
  return s;
}

/** Primera pulsación nueva entre dos estados (igual que el motor): "a", "lt", "lx+", "ry-"… */
export function pulsacionNueva(antes, ahora) {
  const nuevos = (ahora.b || 0) & ~(antes.b || 0);
  for (const f of FUENTES) if (BIT[f] && (nuevos & BIT[f])) return f;
  if (ahora.lt >= 128 && !(antes.lt >= 128)) return "lt";
  if (ahora.rt >= 128 && !(antes.rt >= 128)) return "rt";
  for (const k of EJES) {
    if (ahora[k] >= 20000 && !(antes[k] >= 20000)) return k + "+";
    if (ahora[k] <= -20000 && !(antes[k] <= -20000)) return k + "-";
  }
  return "";
}

// ---- Asistente paso a paso ----------------------------------------------------------
// Botones: «pulsa el que quieres que sea X». Ejes: «empuja el stick izq. a la derecha / arriba».
export const PASOS = [
  ...["a", "b", "x", "y", "lb", "rb", "lt", "rt", "back", "start", "ls", "rs", "arriba", "abajo", "izq", "der"].map((f) => ({ tipo: "boton", f })),
  { tipo: "eje", stick: "l", dir: "x" }, { tipo: "eje", stick: "l", dir: "y" },
  { tipo: "eje", stick: "r", dir: "x" }, { tipo: "eje", stick: "r", dir: "y" },
];

/** ¿Vale esta pulsación para el paso? */
export function encaja(paso, fuente) {
  if (!fuente) return false;
  const esEje = /^[lr][xy][+-]$/.test(fuente);
  return paso.tipo === "eje" ? esEje : !esEje;
}

/**
 * Construye el perfil a partir de lo que se pulsó en cada paso.
 * capturas: { a: "b", b: "a", …, "l.x": "lx+", "l.y": "ly-", "r.x": …, "r.y": … }
 * Lo que no se capturó queda como estaba (sin chocar con lo nuevo). Devuelve { perfil, avisos }.
 */
export function construirPerfil(base, capturas) {
  const p = normalizar(base), avisos = [];
  // Botones: cada destino capturado sale de la fuente pulsada
  const nuevos = {};
  for (const paso of PASOS) {
    if (paso.tipo !== "boton") continue;
    const fuente = capturas[paso.f];
    if (fuente) (nuevos[fuente] = nuevos[fuente] || []).push(paso.f);
  }
  if (Object.keys(nuevos).length) {
    const reclamados = new Set(Object.values(nuevos).flat());
    for (const f of FUENTES) {
      if (nuevos[f]) p.botones[f] = nuevos[f];
      else p.botones[f] = destinoDe(p, f).filter((d) => !reclamados.has(d));   // no duplicar lo reasignado
    }
  }
  // Ejes: de qué stick sale cada dirección y con qué signo
  const lee = (k) => { const c = capturas[k]; return c ? { stick: c[0], eje: c[1], signo: c[2] } : null; };
  const lx = lee("l.x"), ly = lee("l.y"), rx = lee("r.x"), ry = lee("r.y");
  for (const [c, dir] of [[lx, "x"], [ly, "y"], [rx, "x"], [ry, "y"]]) {
    if (c && c.eje !== dir) avisos.push("ejes_cruzados");
  }
  const ref = lx || ly;
  if (ref) p.cambiarSticks = ref.stick === "r";
  else if (rx || ry) p.cambiarSticks = (rx || ry).stick === "l";
  const inv = new Set(p.invertir);
  const marca = (eje, c) => { if (!c || c.eje !== eje[1]) return; if (c.signo === "-") inv.add(eje); else inv.delete(eje); };
  marca("lx", lx); marca("ly", ly); marca("rx", rx); marca("ry", ry);
  p.invertir = EJES.filter((k) => inv.has(k));
  return { perfil: limpiar(p), avisos: [...new Set(avisos)] };
}

/** Zona muerta sugerida (%) con el stick suelto: un poco más de lo que «camina solo». */
export function zonaSugerida(x, y) {
  const pc = Math.hypot(x || 0, y || 0) / 32767 * 100;
  return pc < 1 ? 0 : Math.min(ZONA_MAX, Math.ceil(pc + 4));
}
