// =============================================================================
//  ASISTENTE DE BIENVENIDA (plan UX, fase 1): la primera vez, en 4 pasos cortos,
//  deja la PC lista y enseña dónde se juega. Reutiliza lo que ya existe:
//    1. Hola · 2. Conectar mi cuenta (web.vincular) · 3. Preparar mi PC (Semáforo
//    Phoenix + ARREGLAR TODO de Sync › Mi PC) · 4. Listo (recorrido del tutorial).
//  Sale solo si a esta PC le falta algo (cuenta o mandos virtuales) y nunca se vio;
//  se vuelve a ver desde Ajustes › Interfaz. Nunca con la PC cedida.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, irA } from "../tienda.js";
import { Boton, Logo, cx } from "../ui.js";
import { SemaforoPhoenix, useSemaforo } from "./semaforo.js";
import { arreglarTodo, useTareaArreglar, cancelarArreglar } from "./preparar.js";
import { abrirTutorial, marcarTutorialVisto } from "./tutorial.js";

const CLAVE = "phoenix.bienvenida.vista";
const EVENTO = "phoenix:bienvenida";
const vista = () => { try { return localStorage.getItem(CLAVE) === "1"; } catch { return true; } };
const marcarVista = () => { try { localStorage.setItem(CLAVE, "1"); } catch { /* sin almacenamiento */ } };

/** Abre el asistente desde cualquier sitio (Ajustes › Interfaz). */
export function abrirBienvenida() { window.dispatchEvent(new CustomEvent(EVENTO)); }

/** ¿Le falta algo a esta PC como para enseñar la bienvenida sola? (lo usa el tutorial para no taparla). */
export function bienvenidaPendiente(m) {
  if (vista() || !m || m.cedido?.activo) return false;
  return m.web?.estado === "sin_vincular" || (m.mandos?.lista || []).length === 0;   // PC nueva: sin cuenta o sin mandos virtuales
}

const PASOS = 4;

export function Bienvenida({ s }) {
  const m = s.motor;
  const [paso, setPaso] = useState(null);   // null = cerrado
  useEffect(() => {
    const abrir = () => setPaso(0);
    window.addEventListener(EVENTO, abrir);
    return () => window.removeEventListener(EVENTO, abrir);
  }, []);
  // Primera vez: cuando el motor ya respondió y le falta algo a la PC
  const pendiente = s.conectado && bienvenidaPendiente(m);
  useEffect(() => { if (pendiente && paso == null) setPaso(0); }, [pendiente]);
  if (paso == null || !m || m.cedido?.activo) return null;

  const cerrar = () => { marcarVista(); setPaso(null); };
  const siguiente = () => (paso < PASOS - 1 ? setPaso(paso + 1) : cerrar());
  return html`<div class="velo" role="dialog" aria-modal="true" aria-labelledby="bv-titulo">
    <div class="dialogo bienvenida" style="width:min(640px,100%)"><div class="cut"><div class="in" style="padding:26px 30px;display:flex;flex-direction:column;gap:16px">
      <div style="display:flex;align-items:center;justify-content:space-between;gap:12px">
        <div class="tour-puntos" aria-hidden="true">${Array.from({ length: PASOS }, (_, i) => html`<i key=${i} class=${i === paso ? "on" : i < paso ? "hecho" : ""}></i>`)}</div>
        <button class="tour-omitir" onClick=${cerrar}>${paso === PASOS - 1 ? t("CERRAR", "CLOSE") : t("AHORA NO", "NOT NOW")}</button>
      </div>
      ${paso === 0 ? html`<${PasoHola} siguiente=${siguiente}/>`
        : paso === 1 ? html`<${PasoCuenta} m=${m} siguiente=${siguiente}/>`
        : paso === 2 ? html`<${PasoPc} m=${m} siguiente=${siguiente} cerrar=${cerrar}/>`
        : html`<${PasoListo} m=${m} cerrar=${cerrar}/>`}
      ${paso > 0 && paso < PASOS - 1 ? html`<div><button class="enlace-mas" onClick=${() => setPaso(paso - 1)}>‹ ${t("Atrás", "Back")}</button></div>` : null}
    </div></div></div>
  </div>`;
}

function Cabeza({ lab, titulo, texto }) {
  return html`<div>
    <div class="lab acc">${lab}</div>
    <h2 id="bv-titulo" class="grande degradado" style="margin:8px 0 6px;font-size:32px">${titulo}</h2>
    <div class="ayuda" style="font-size:14px;line-height:1.5">${texto}</div>
  </div>`;
}

function PasoHola({ siguiente }) {
  return html`
    <div style="display:flex;align-items:center;gap:16px"><${Logo} t=${56}/>
      <${Cabeza} lab=${t("BIENVENIDO", "WELCOME")} titulo=${t("Phoenix Link", "Phoenix Link")}
        texto=${t("Con Link juegas PES con tus amigos por internet: tú abres la sala, ellos entran con un enlace y cada uno se sienta en su mando. Dejemos tu PC lista en 3 pasos cortos (una sola vez).", "With Link you play PES with friends online: you open the room, they join with a link and each one takes a pad. Let's get your PC ready in 3 short steps (only once).")}/></div>
    <ol class="ayuda" style="font-size:14px;line-height:1.8;margin:0;padding-left:22px">
      <li>${t("Conectar tu cuenta de la liga (opcional).", "Connect your league account (optional).")}</li>
      <li>${t("Preparar tu PC: Parsec, mandos y tu juego.", "Prepare your PC: Parsec, pads and your game.")}</li>
      <li>${t("Listo: a jugar.", "Done: let's play.")}</li>
    </ol>
    <div class="botones" style="margin-top:4px"><${Boton} tipo="lleno enorme" al=${siguiente}>${t("EMPEZAR", "START")}</${Boton}></div>`;
}

function PasoCuenta({ m, siguiente }) {
  const [codigo, setCodigo] = useState("");
  const w = m.web || {};
  const conectada = w.estado === "conectado";
  useEffect(() => { if (conectada) { const id = setTimeout(siguiente, 900); return () => clearTimeout(id); } return undefined; }, [conectada]);
  const conectar = async () => {
    const c = codigo.replace(/\D/g, "");
    if (c.length !== 6) { avisar(t("El código tiene 6 dígitos.", "The code has 6 digits."), "warn"); return; }
    if (await accion("web.vincular", { codigo: c })) setCodigo("");
  };
  return html`
    <${Cabeza} lab=${t("PASO 1 DE 3 · OPCIONAL", "STEP 1 OF 3 · OPTIONAL")} titulo=${t("Conectar mi cuenta", "Connect my account")}
      texto=${t("Con tu cuenta de la liga sales en el radar, ves a tus amigos y recibes los fichajes. Para jugar con un amigo no hace falta.", "With your league account you show on the radar, see your friends and get transfers. Not needed to play with a friend.")}/>
    ${conectada ? html`<div class="caja" style="border-color:var(--ok)"><b class="ok">✓ ${t(`Conectada: ${w.usuario || ""}`, `Connected: ${w.usuario || ""}`)}</b></div>` : html`
      <ol class="ayuda" style="font-size:14px;line-height:1.7;margin:0;padding-left:20px">
        <li>${t("En la web de la liga, entra con tu cuenta.", "On the league website, sign in.")}</li>
        <li>${t("Ve a Mi perfil › vincular PC y copia los 6 números.", "Go to My profile › link PC and copy the 6 digits.")}</li>
        <li>${t("Escríbelos aquí.", "Type them here.")}</li>
      </ol>
      <div style="display:flex;gap:10px;max-width:420px">
        <input class="campo mono" inputmode="numeric" maxLength="6" placeholder="123456" value=${codigo} style="font-size:22px;letter-spacing:.3em;text-align:center;height:52px"
          onInput=${(e) => setCodigo(e.currentTarget.value.replace(/[^\d]/g, "").slice(0, 6))} onKeyDown=${(e) => e.key === "Enter" && conectar()}/>
        <${Boton} tipo="lleno enorme" deshabilitado=${codigo.length !== 6} al=${conectar}>${t("CONECTAR", "CONNECT")}</${Boton}>
      </div>
      ${w.estado === "pausado" || w.estado === "sin_conexion" ? html`<div class="ayuda" style="color:var(--warn)">${w.mensaje || ""}</div>` : null}`}
    <div class="botones" style="margin-top:0">
      ${conectada ? html`<${Boton} tipo="lleno" al=${siguiente}>${t("SIGUIENTE", "NEXT")}</${Boton}>`
        : html`<${Boton} tipo="suave" al=${siguiente}>${t("SALTAR: SOLO QUIERO JUGAR", "SKIP: I JUST WANT TO PLAY")}</${Boton}>`}
    </div>`;
}

function PasoPc({ m, siguiente, cerrar }) {
  const { resumen } = useSemaforo(m);
  const { haciendo, esperandoPes, informe } = useTareaArreglar();
  return html`
    <${Cabeza} lab=${t("PASO 2 DE 3", "STEP 2 OF 3")} titulo=${t("Preparar mi PC", "Prepare my PC")}
      texto=${t("Link revisa Parsec, los mandos virtuales y tu juego. Lo que pueda, lo arregla solo (con copia antes y nunca con PES abierto).", "Link checks Parsec, the virtual pads and your game. Whatever it can, it fixes by itself (backed up first, never with PES open).")}/>
    <div class="caja" style="padding:14px 16px"><${SemaforoPhoenix} m=${m} compacto enTarjeta=${false} alIr=${cerrar}/></div>
    ${informe.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px;font-size:13px;max-height:160px;overflow:auto">
      ${informe.map((i, k) => html`<div key=${k} style="display:flex;gap:8px"><span class=${cx("punto", i.ok ? "ok" : "warn")} style="flex:none;margin-top:6px"></span><span>${i.texto}</span></div>`)}</div>` : null}
    <div class="botones" style="margin-top:0">
      ${esperandoPes ? html`<${Boton} tipo="suave" al=${cancelarArreglar}>${t("CANCELAR", "CANCEL")}</${Boton}>` : null}
      <${Boton} tipo=${resumen.color === "ok" ? "suave" : "lleno"} deshabilitado=${haciendo} al=${arreglarTodo}>
        ${esperandoPes ? t("ESPERANDO A QUE CIERRES PES…", "WAITING FOR PES TO CLOSE…") : haciendo ? t("PREPARANDO…", "PREPARING…") : t("PREPARAR MI JUEGO", "PREPARE MY GAME")}</${Boton}>
      <${Boton} tipo=${resumen.color === "ok" ? "lleno" : "suave"} al=${siguiente}>${resumen.faltan.length ? t("SEGUIR IGUAL", "CONTINUE ANYWAY") : t("SIGUIENTE", "NEXT")}</${Boton}>
    </div>`;
}

function PasoListo({ m, cerrar }) {
  const { resumen } = useSemaforo(m);
  const listo = !resumen.faltan.length;
  return html`
    <${Cabeza} lab=${t("PASO 3 DE 3", "STEP 3 OF 3")} titulo=${listo ? t("Listo. Ya puedes jugar.", "Done. You can play now.") : t("Casi listo", "Almost ready")}
      texto=${listo ? t("Desde ahora, jugar es un toque: en INICIO pulsa JUGAR CON UN AMIGO y manda el enlace. Si alguna vez algo falla, el Semáforo Phoenix te dice qué es y lo arregla.", "From now on, playing is one tap: in HOME press PLAY WITH A FRIEND and send the link. If something ever fails, the Phoenix check tells you what and fixes it.")
        : t("Aún falta algo para poder jugar. El Semáforo Phoenix (Sala › Conexión) te dice qué y tiene el botón para arreglarlo.", "Something is still missing to play. The Phoenix check (Room › Connection) tells you what and has the fix button.")}/>
    <div class="caja" style="padding:14px 16px"><${SemaforoPhoenix} m=${m} compacto enTarjeta=${false} alIr=${cerrar}/></div>
    <div class="botones" style="margin-top:0">
      <${Boton} tipo="suave" al=${() => { cerrar(); abrirTutorial(); }}>${t("VER DÓNDE ESTÁ CADA COSA (1 MIN)", "SHOW ME AROUND (1 MIN)")}</${Boton}>
      <${Boton} tipo="lleno enorme" al=${() => { cerrar(); marcarTutorialVisto(); irA("inicio", "inicio"); }}>${t("EMPEZAR A JUGAR", "START PLAYING")}</${Boton}>
    </div>`;
}
