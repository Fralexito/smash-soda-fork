// =============================================================================
//  SEMÁFORO PHOENIX (plan UX, fase 1): UNA sola respuesta a «¿está listo para jugar?».
//  Reúne lo que antes estaba en cuatro sitios (paso 1 de la guía, ANTES DE ABRIR,
//  Diagnóstico del PC y Sync › Mi PC) y los avisos de ANTES DEL PARTIDO:
//    verde «Todo listo» · ámbar «Listo, con un aviso» · rojo «Falta algo».
//  Cada punto lleva su causa en una frase y un botón que lo arregla o lleva a arreglarlo.
//  Rojo = no se puede jugar (Parsec, mandos virtuales). Ámbar = se juega igual, pero
//  algo va peor (cuenta de la liga, juego para los fichajes, transmisión).
//  Datos: el estado del motor + diag.ejecutar y sync.modulos (se piden solos, con calma,
//  solo mientras algún semáforo está a la vista). No escribe nada salvo lo que pulses.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar, irA } from "../tienda.js";
import { Tarjeta, Titulo, Boton, cx, useDestello } from "../ui.js";
import { instalarVigem } from "./arreglos.js";

// ---- Datos que no vienen en el estado: se comparten entre todos los semáforos a la vista ----
let datos = { diag: null, mods: null };
const oyentes = new Set();
let usuarios = 0, reloj = 0, ultimoDiag = 0;
async function refrescar(forzar = false) {
  const ahora = Date.now();
  const pedirDiag = forzar || !datos.diag || ahora - ultimoDiag > 60000;   // diag es «lento»: como mucho una vez por minuto
  const [d, mo] = await Promise.all([
    pedirDiag ? accion("diag.ejecutar", {}, { silencioso: true }) : Promise.resolve(null),
    accion("sync.modulos", {}, { silencioso: true }),
  ]);
  if (d) { ultimoDiag = ahora; datos = { ...datos, diag: d.chequeos || [] }; }
  if (mo) datos = { ...datos, mods: mo };
  oyentes.forEach((f) => f(datos));
}
/** Vuelve a revisar todo ahora (botón «Revisar» y «Ya lo hice»). */
export const revisarSemaforo = () => refrescar(true);
/** Refresco sin forzar el diagnóstico (lo usa ARREGLAR TODO tras cada paso). */
export const refrescarSemaforo = () => refrescar(false);

function useDatos() {
  const [d, setD] = useState(datos);
  useEffect(() => {
    oyentes.add(setD);
    if (usuarios++ === 0) { refrescar(); reloj = setInterval(refrescar, 20000); }
    return () => { oyentes.delete(setD); if (--usuarios === 0) clearInterval(reloj); };
  }, []);
  return d;
}

// ---- Avisos de transmisión (los mismos de ANTES DEL PARTIDO) -------------------------------
// Una vez que el anfitrión cerró Parsec desde aquí, el aviso no vuelve mientras Link siga abierto
// (el servicio de Parsec suele volver a abrir la app solo).
let parsecYaCerrado = false;
// Avisos que el anfitrión cerró con ✕: se recuerdan en este PC por «id:valor», así vuelven solo si el problema cambia.
const CLAVE_AVISOS = "phoenix.avisosCerrados";
export const leerCerrados = () => { try { return new Set(JSON.parse(localStorage.getItem(CLAVE_AVISOS) || "[]")); } catch { return new Set(); } };
export const guardarCerrados = (set) => { try { localStorage.setItem(CLAVE_AVISOS, JSON.stringify([...set].slice(-40))); } catch { /* sin almacenamiento */ } };

export async function cerrarParsec() {
  if (await confirmar({ titulo: t("¿Cerrar la app de Parsec?", "Close the Parsec app?"), texto: t("Se cierra la app normal de Parsec (no Link). Tu sala sigue abierta.", "The normal Parsec app closes (not Link). Your room stays open."), si: t("Cerrar Parsec", "Close Parsec") })) {
    if (await accion("sala.cerrarParsec", {}, { ok: t("Parsec cerrado.", "Parsec closed.") })) { parsecYaCerrado = true; return true; }
  }
  return false;
}

/** Lo que puede dar lag en la transmisión: [{ id, firma, t, d, boton, al }]. Lo usan ANTES DEL PARTIDO y el semáforo. */
export function avisosTransmision(m) {
  const c = m.sala?.calidad || {};
  const auto = c.auto ?? true, por = c.porPersona ?? 10, mbps = c.mbps ?? 15;
  const items = [];
  if (c.parsecApp && !parsecYaCerrado) items.push({ id: "parsec", firma: "parsec", t: t("La app de Parsec está abierta", "The Parsec app is open"), d: t("Compite con Link por la tarjeta de video y el internet: puede dar tirones.", "It competes with Link for the GPU and internet: it can cause stutter."), boton: t("CERRAR PARSEC", "CLOSE PARSEC"), al: cerrarParsec });
  if (!auto) items.push({ id: "auto", firma: `auto:${mbps}`, t: t(`Ancho de banda automático apagado (${mbps} Mbps para todos)`, `Auto bandwidth off (${mbps} Mbps for everyone)`), d: t("Se reparte entre todos los que entran: con varios amigos, a cada uno le llega poca calidad.", "It's split among everyone: with several friends each gets low quality."), boton: t("ENCENDER · 15 POR PERSONA", "TURN ON · 15 PER PERSON"), al: () => accion("sala.anchoAuto", { auto: true, porPersona: Math.max(por, 15) }) });
  if (c.energia === "ahorro") items.push({ id: "energia", firma: "energia", t: t("Windows está en «Ahorro de energía»", "Windows is on Power Saver"), d: t("Cámbialo a «Equilibrado» o «Alto rendimiento» (Configuración › Sistema › Energía) para que no haya tirones.", "Switch to Balanced or High performance (Settings › System › Power).") });
  if ((c.perdida ?? 0) >= 50) items.push({ id: "perdida", firma: "perdida", t: t(`Un amigo pierde paquetes (${((c.perdida ?? 0) / 10).toFixed(1)} %)`, `A friend is losing packets (${((c.perdida ?? 0) / 10).toFixed(1)} %)`), d: t("Su internet no aguanta: que use cable o WiFi de 5 GHz.", "Their internet can't keep up: cable or 5 GHz WiFi.") });
  return items;
}

// ---- Las comprobaciones ---------------------------------------------------------------------
/** [{ id, nivel: "ok"|"aviso"|"falta"|"espera", titulo, frase, arreglo: [texto, fn, llevaAOtraPantalla?] | null }] */
export function comprobaciones(m, d = datos) {
  const sala = m.sala || {};
  const diag = (id) => (d.diag || []).find((c) => c.id === id);
  const lista = [];

  // 1. Parsec (rojo: sin Parsec no hay sala)
  const dp = diag("parsec");
  if (sala.lista) lista.push({ id: "parsec", nivel: "ok", titulo: t("Parsec", "Parsec"), frase: sala.cuentaHost || t("Sesión iniciada.", "Signed in.") });
  else if (dp && !dp.datos?.cuenta) lista.push({ id: "parsec", nivel: "falta", titulo: t("Parsec", "Parsec"), frase: t("Inicia sesión en la app de Parsec: Link abre tu sala con esa cuenta.", "Sign in to the Parsec app: Link opens your room with that account."), arreglo: [t("YA LO HICE", "DONE"), revisarSemaforo] });
  else lista.push({ id: "parsec", nivel: "espera", titulo: t("Parsec", "Parsec"), frase: t("Se está preparando. Tarda unos segundos al abrir Link.", "Getting ready. It takes a few seconds after opening Link.") });

  // 2. Mandos virtuales (rojo: sin ViGEmBus nadie juega)
  const dv = diag("vigem");
  const nMandos = (m.mandos?.lista || []).length;
  if (dv && !dv.datos?.instalado) lista.push({ id: "mandos", nivel: "falta", titulo: t("Mandos virtuales", "Virtual pads"), frase: t("Falta ViGEmBus, el controlador que crea los mandos de tus amigos.", "ViGEmBus is missing, the driver that creates your friends' pads."), arreglo: [t("INSTALAR VIGEMBUS", "INSTALL VIGEMBUS"), instalarVigem] });
  else if (nMandos === 0) lista.push({ id: "mandos", nivel: dv ? "falta" : "espera", titulo: t("Mandos virtuales", "Virtual pads"), frase: dv ? t("ViGEmBus está, pero no hay mandos: cierra y abre Link.", "ViGEmBus is there but there are no pads: close and reopen Link.") : t("Revisando…", "Checking…"), arreglo: dv ? [t("VER PUESTOS", "SEE SEATS"), () => irA("mandos", "puestos"), true] : null });
  else lista.push({ id: "mandos", nivel: "ok", titulo: t("Mandos virtuales", "Virtual pads"), frase: t(`${nMandos} mandos listos.`, `${nMandos} pads ready.`) });

  // 3. Cuenta de la liga (ámbar: con amigos se juega igual)
  const w = m.web || {};
  if (w.estado === "conectado") lista.push({ id: "cuenta", nivel: "ok", titulo: t("Tu cuenta de la liga", "Your league account"), frase: w.usuario ? t(`Conectada: ${w.usuario}.`, `Connected: ${w.usuario}.`) : t("Conectada.", "Connected.") });
  else if (w.estado === "vinculando") lista.push({ id: "cuenta", nivel: "espera", titulo: t("Tu cuenta de la liga", "Your league account"), frase: t("Conectando…", "Connecting…") });
  else if (w.estado === "pausado") lista.push({ id: "cuenta", nivel: "aviso", titulo: t("Tu cuenta de la liga", "Your league account"), frase: causaPausa(w), arreglo: arregloPausa(w, m) });
  else if (w.estado === "sin_conexion") lista.push({ id: "cuenta", nivel: "aviso", titulo: t("Tu cuenta de la liga", "Your league account"), frase: t("No hay conexión con la web ahora. Se reintenta sola; con amigos se juega igual.", "No connection to the website right now. It retries by itself; you can still play with friends."), arreglo: [t("REINTENTAR", "RETRY"), () => accion("web.reintentar", {}, { ok: t("Reintentando…", "Retrying…") })] });
  else lista.push({ id: "cuenta", nivel: "aviso", titulo: t("Tu cuenta de la liga", "Your league account"), frase: t("Sin conectar: con amigos juegas igual, pero no sales en el radar ni recibes fichajes.", "Not connected: you can play with friends, but you won't show on the radar or get transfers."), arreglo: [t("CONECTAR MI CUENTA", "CONNECT MY ACCOUNT"), () => irA("sync", "mipc"), true] });

  // 4. Mi juego: PES y los archivos de Phoenix (ámbar: hacen falta para los fichajes, no para jugar)
  const mods = d.mods;
  const faltan = (mods?.modulos || []).filter((x) => x.gestionable && x.estado !== "instalado");
  if (!mods) lista.push({ id: "juego", nivel: "espera", titulo: t("Mi juego", "My game"), frase: t("Revisando…", "Checking…") });
  else if (!mods.juego) lista.push({ id: "juego", nivel: "aviso", titulo: t("Mi juego", "My game"), frase: t("Link aún no sabe dónde está tu PES (hace falta para los fichajes).", "Link doesn't know where your PES is yet (needed for transfers)."), arreglo: [t("BUSCAR MI JUEGO", "FIND MY GAME"), () => irA("sync", "mipc"), true] });
  else if (faltan.length) lista.push({ id: "juego", nivel: "aviso", titulo: t("Mi juego", "My game"), frase: mods.pesAbierto ? t("Faltan archivos de Phoenix en el juego. Se ponen con PES cerrado: Link espera solo.", "Phoenix files are missing in the game. They go in with PES closed: Link waits by itself.") : t("Faltan archivos de Phoenix en el juego (para los fichajes).", "Phoenix files are missing in the game (for transfers)."), arreglo: [t("ARREGLAR", "FIX"), () => irA("sync", "mipc"), true] });
  else lista.push({ id: "juego", nivel: "ok", titulo: t("Mi juego", "My game"), frase: t("PES encontrado y archivos de Phoenix al día.", "PES found and Phoenix files up to date.") });

  // 5. Transmisión (ámbar): los avisos de ANTES DEL PARTIDO que no cerraste con ✕
  const cerrados = leerCerrados();
  const tx = avisosTransmision(m).filter((x) => !cerrados.has(x.firma));
  if (tx.length) for (const x of tx) lista.push({ id: "tx-" + x.id, nivel: "aviso", titulo: x.t, frase: x.d, arreglo: x.al ? [x.boton, x.al] : null });
  else {
    const c = sala.calidad || {};
    lista.push({ id: "tx", nivel: "ok", titulo: t("Transmisión", "Stream"), frase: (c.auto ?? true) ? t(`Calidad automática · ${c.porPersona ?? 10} Mbps por persona.`, `Auto quality · ${c.porPersona ?? 10} Mbps per person.`) : t(`${c.mbps ?? 15} Mbps para todos.`, `${c.mbps ?? 15} Mbps for everyone.`) });
  }
  return lista;
}

/** Causa de la pausa de la web en una frase (código estable del contrato si Link lo manda; si no, el mensaje). */
export function causaPausa(w) {
  switch (w.codigo) {
    case "HOST_NO_AUTORIZADO": return t("Tu cuenta aún no está aprobada como anfitrión. Mientras tanto puedes jugar con amigos y unirte a salas de otros.", "Your account isn't approved as host yet. Meanwhile you can play with friends and join other rooms.");
    case "APP_DESACTUALIZADA": case "VERSION_DESACTIVADA": return t("Esta versión de Link ya no vale para la web. Actualiza Link; con amigos sigues jugando igual.", "This Link version is no longer accepted by the website. Update Link; you can still play with friends.");
    case "DISPOSITIVO_SUSPENDIDO": return t("El staff suspendió esta PC en la web. Habla con el staff en Discord.", "The staff suspended this PC on the website. Talk to the staff on Discord.");
    default: return (w.mensaje ? w.mensaje + " " : "") + t("La web está en pausa para esta PC; con amigos se juega igual.", "The website is paused for this PC; you can still play with friends.");
  }
}
function arregloPausa(w, m) {
  const hayNueva = m.actualizacion?.estado === "disponible";
  if ((w.codigo === "APP_DESACTUALIZADA" || w.codigo === "VERSION_DESACTIVADA") && hayNueva) return [t("ACTUALIZAR", "UPDATE"), () => accion("app.actualizar", {})];
  return [t("REINTENTAR", "RETRY"), () => accion("web.reintentar", {}, { ok: t("Reintentando…", "Retrying…") })];
}

/** Resumen: { color: "ok"|"warn"|"bad", texto, faltan, avisos, cargando } */
export function resumenSemaforo(lista) {
  const faltan = lista.filter((x) => x.nivel === "falta");
  const avisos = lista.filter((x) => x.nivel === "aviso");
  const cargando = lista.some((x) => x.nivel === "espera");
  if (faltan.length) return { color: "bad", texto: t(`Falta algo: ${faltan.length} ${faltan.length === 1 ? "cosa" : "cosas"}`, `Something's missing: ${faltan.length}`), faltan, avisos, cargando };
  if (avisos.length) return { color: "warn", texto: avisos.length === 1 ? t("Listo, con un aviso", "Ready, with one notice") : t(`Listo, con ${avisos.length} avisos`, `Ready, with ${avisos.length} notices`), faltan, avisos, cargando };
  if (cargando) return { color: "", texto: t("Revisando…", "Checking…"), faltan, avisos, cargando };
  return { color: "ok", texto: t("Todo listo para jugar", "All set to play"), faltan, avisos, cargando };
}

/** Hook: las comprobaciones de ahora y su resumen. */
export function useSemaforo(m) {
  const d = useDatos();
  const lista = m ? comprobaciones(m, d) : [];
  return { lista, resumen: resumenSemaforo(lista) };
}

const SIMBOLO = { ok: "✓", aviso: "!", falta: "×", espera: "…" };
const CLASE = { ok: "ok", aviso: "aviso", falta: "error", espera: "" };

function Fila({ x, alIr }) {
  return html`<div class="chequeo" key=${x.id}>
    <span class=${cx("icono", CLASE[x.nivel])}>${SIMBOLO[x.nivel]}</span>
    <div style="min-width:0;flex:1"><div style="font-weight:600">${x.titulo}</div><div class="ayuda" style="word-break:break-word">${x.frase}</div></div>
    ${x.arreglo ? html`<${Boton} tipo=${x.nivel === "falta" ? "mini lleno" : "mini suave"} al=${async () => { const r = x.arreglo[1](); if (x.arreglo[2] && alIr) alIr(); return r; }}>${x.arreglo[0]}</${Boton}>` : null}
  </div>`;
}

/** La línea de arriba: punto de color + «Todo listo / Listo, con un aviso / Falta algo». */
export function CabeceraSemaforo({ resumen, derecha = null, grande = false }) {
  const destello = useDestello(resumen.color);   // cambia de color: destella
  return html`<div class=${cx("semaforo-cab", destello && "destello")} style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
    <span class=${cx("punto", resumen.color, resumen.color === "ok" && "vivo")} style=${grande ? "width:16px;height:16px;border-radius:8px" : "width:12px;height:12px;border-radius:6px"}></span>
    <span class="disp" style=${`font-size:${grande ? 26 : 19}px;font-weight:700;flex:1;min-width:0;color:${resumen.color === "ok" ? "var(--ok)" : resumen.color === "warn" ? "var(--warn)" : resumen.color === "bad" ? "var(--bad)" : "inherit"}`}>${resumen.texto}</span>
    ${derecha}
  </div>`;
}

/**
 * El Semáforo Phoenix. compacto: la cabecera y solo lo que falla (para portadas); sin compacto: todo (Sala › Conexión).
 * enTarjeta=false para meterlo dentro de otra tarjeta. alIr: se llama cuando un botón lleva a otra pantalla (el asistente se cierra).
 */
export function SemaforoPhoenix({ m, compacto = false, enTarjeta = true, alIr = null }) {
  const { lista, resumen } = useSemaforo(m);
  const [abierto, setAbierto] = useState(false);
  const visibles = compacto && !abierto ? lista.filter((x) => x.nivel === "falta" || x.nivel === "aviso") : lista;
  const cuerpo = html`
    <${CabeceraSemaforo} resumen=${resumen} grande=${!compacto} derecha=${html`<div style="display:flex;gap:8px">
      ${compacto ? html`<${Boton} tipo="mini suave" al=${() => setAbierto(!abierto)}>${abierto ? t("SOLO LO QUE FALTA", "ONLY ISSUES") : t("VER DETALLES", "SEE DETAILS")}</${Boton}>` : null}
      <${Boton} tipo="mini suave" al=${revisarSemaforo}>${t("REVISAR", "CHECK")}</${Boton}></div>`}/>
    ${visibles.length ? html`<div style="margin-top:6px">${visibles.map((x) => html`<${Fila} x=${x} alIr=${alIr} key=${x.id}/>`)}</div>` : null}`;
  if (!enTarjeta) return html`<div>${cuerpo}</div>`;
  return html`<${Tarjeta} interior="padding:20px 24px">
    <${Titulo} texto=${t("SEMÁFORO PHOENIX", "PHOENIX CHECK")}/>
    ${cuerpo}
  </${Tarjeta}>`;
}
