// =============================================================================
//  Tienda de la interfaz: todo lo que llega del motor + estado local de la UI.
//  Un solo objeto inmutable; los componentes se suscriben con useTienda().
// =============================================================================
import { useState, useEffect } from "./lib.js";
import { puente } from "./puente.js";

const MAX_CHAT = 300;
const MAX_ACTIVIDAD = 400;

let estado = {
  conectado: false,        // llegó la bienvenida
  motor: null,             // último «estado» del motor
  info: { idiomas: [], resoluciones: [], pantallas: [], gpus: [], wgc: false, temasOverlay: [] },
  chat: [],                // { id, texto, en }
  actividad: [],           // { id, texto, en }
  noLeidos: 0,             // sin leer del chat de SALA
  chatGlobal: { mensajes: [] },   // chat general (el mismo de la web): lista que manda el motor
  chatPestana: "general",  // general | sala | registro
  leidoGlobal: null,       // id del último mensaje general ya visto (null = aún sin historial)
  chatAbierto: false,
  seccion: "sala",
  pestanas: { sala: "resumen", partido: "vivo", mandos: "puestos", gente: "sala", sync: "puente", ajustes: "general" },
  avisos: [],              // toasts
  dialogo: null,           // { titulo, texto, botones:[{texto, tipo, valor}], resolver }
  paleta: false,
};

const suscriptores = new Set();
let idLinea = 1;
let idAviso = 1;

export function leer() { return estado; }

export function cambiar(parcial) {
  estado = { ...estado, ...(typeof parcial === "function" ? parcial(estado) : parcial) };
  for (const fn of suscriptores) fn(estado);
}

export function useTienda(selector = (s) => s) {
  const [, forzar] = useState(0);
  useEffect(() => {
    let previo = selector(estado);
    const fn = (s) => {
      const nuevo = selector(s);
      if (nuevo !== previo) { previo = nuevo; forzar((x) => x + 1); }
    };
    suscriptores.add(fn);
    return () => suscriptores.delete(fn);
  }, []);
  return selector(estado);
}

// ---- Navegación -------------------------------------------------------------
export function irA(seccion, pestana) {
  cambiar((s) => ({
    seccion,
    pestanas: pestana ? { ...s.pestanas, [seccion]: pestana } : s.pestanas,
  }));
  avisarSeccion();
}

export function elegirPestana(pestana) {
  cambiar((s) => ({ pestanas: { ...s.pestanas, [s.seccion]: pestana } }));
  avisarSeccion();
}

// El motor solo manda datos pesados (serie de red, audio, amigos) de lo que se mira
function avisarSeccion() {
  const s = estado;
  puente.pedir("ui.seccion", { seccion: s.seccion, pestana: s.pestanas[s.seccion] || "" }).catch(() => {});
}

// ---- Avisos (toasts) ----------------------------------------------------------
export function avisar(texto, tipo = "info", ms = 4200) {
  const id = idAviso++;
  cambiar((s) => ({ avisos: [...s.avisos.slice(-4), { id, texto, tipo }] }));
  if (ms > 0) setTimeout(() => quitarAviso(id), ms);
  return id;
}
export function quitarAviso(id) {
  cambiar((s) => ({ avisos: s.avisos.filter((a) => a.id !== id) }));
}

// ---- Diálogo de confirmación (nunca confirm() nativo) --------------------------
export function confirmar({ titulo, texto, si = "Aceptar", no = "Cancelar", peligro = false }) {
  return new Promise((resolver) => {
    cambiar({
      dialogo: {
        titulo, texto, resolver,
        botones: [
          { texto: no, tipo: "suave", valor: false },
          { texto: si, tipo: peligro ? "peligro lleno" : "lleno", valor: true },
        ],
      },
    });
  });
}
export function cerrarDialogo(valor) {
  const d = estado.dialogo;
  cambiar({ dialogo: null });
  if (d && d.resolver) d.resolver(valor);
}

// ---- Acciones con aviso de error ---------------------------------------------------
/** Ejecuta una acción del motor; si falla muestra el motivo. Devuelve los datos o null. */
export async function accion(nombre, datos = {}, { ok = null, silencioso = false } = {}) {
  try {
    const r = await puente.pedir(nombre, datos);
    if (ok) avisar(ok, "ok");
    return r ?? {};
  } catch (e) {
    if (!silencioso) avisar(e.message || "No se pudo completar.", "bad", 6000);
    console.warn("[accion]", nombre, e);
    return null;
  }
}

// ---- Chat y actividad ---------------------------------------------------------------
function agregarLineas(clave, maximo, datos) {
  const ahora = Date.now();
  const nuevas = (datos.lineas || []).map((texto) => ({ id: idLinea++, texto: String(texto), en: ahora }));
  cambiar((s) => {
    const base = datos.reinicio ? [] : s[clave];
    const cambios = { [clave]: [...base, ...nuevas].slice(-maximo) };
    if (clave === "chat" && !datos.reinicio && !(s.chatAbierto && s.chatPestana === "sala")) cambios.noLeidos = s.noLeidos + nuevas.length;
    return cambios;
  });
}

// ---- Chat general (contrato §27) -----------------------------------------------------
/** Mensajes del chat general sin leer (no cuenta los propios). */
export function noLeidosGlobal(s) {
  if (s.leidoGlobal == null) return 0;
  let n = 0;
  for (const m of s.chatGlobal.mensajes) if (!m.propio && m.id > s.leidoGlobal) n++;
  return n;
}
function ultimoGlobal(g) { return g.mensajes.length ? g.mensajes[g.mensajes.length - 1].id : 0; }

/** Llega la lista del chat general. El primer historial no cuenta como «sin leer». */
function recibirGlobal(g) {
  cambiar((s) => {
    const nuevo = { ...g, mensajes: g.mensajes || [] };
    const leido = s.leidoGlobal == null ? ultimoGlobal(nuevo) : s.leidoGlobal;
    // viendo GENERAL con el panel abierto: todo queda leído
    const viendo = s.chatAbierto && s.chatPestana === "general";
    return { chatGlobal: nuevo, leidoGlobal: viendo ? Math.max(leido, ultimoGlobal(nuevo)) : leido };
  });
}

/** Cambia de pestaña del chat y marca lo visible como leído. */
export function elegirPestanaChat(pestana) {
  cambiar((s) => ({
    chatPestana: pestana,
    noLeidos: pestana === "sala" ? 0 : s.noLeidos,
    leidoGlobal: pestana === "general" && s.chatAbierto ? Math.max(s.leidoGlobal ?? 0, ultimoGlobal(s.chatGlobal)) : s.leidoGlobal,
  }));
}

/** Sala abierta → SALA por defecto; sala cerrada → vuelve a GENERAL. */
let _salaAbiertaPrevia = null;
function vigilarSala(m) {
  const abierta = !!m?.sala?.abierta;
  if (_salaAbiertaPrevia === abierta) return;
  const primera = _salaAbiertaPrevia === null;
  _salaAbiertaPrevia = abierta;
  const s = estado;
  if (abierta && s.chatPestana !== "registro") elegirPestanaChat("sala");
  else if (!abierta && s.chatPestana === "sala") elegirPestanaChat("general");
  else if (primera && !abierta) { /* ya está en GENERAL */ }
}

// ---- Arranque ----------------------------------------------------------------------
export function conectarTienda() {
  puente.on("bienvenida", (b) => {
    cambiar({
      conectado: true,
      info: {
        idiomas: b.idiomas || [], resoluciones: b.resoluciones || [], pantallas: b.pantallas || [],
        gpus: b.gpus || [], wgc: !!b.wgc, temasOverlay: b.temasOverlay || [],
      },
      chat: (b.chat || []).map((texto) => ({ id: idLinea++, texto, en: 0 })),
      actividad: (b.actividad || []).map((texto) => ({ id: idLinea++, texto, en: 0 })),
      motor: b.estado || null,
    });
    if (b.chatGlobal) recibirGlobal(b.chatGlobal);
    vigilarSala(b.estado);
    avisarSeccion();
  });
  puente.on("estado", (e) => { cambiar({ motor: e }); vigilarSala(e); });
  puente.on("evento:chatglobal", (d) => recibirGlobal(d));
  puente.on("evento:chat", (d) => agregarLineas("chat", MAX_CHAT, d));
  puente.on("evento:actividad", (d) => agregarLineas("actividad", MAX_ACTIVIDAD, d));
}

export function actualizarInfo(parcial) {
  cambiar((s) => ({ info: { ...s.info, ...parcial } }));
}
