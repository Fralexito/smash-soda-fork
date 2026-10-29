// =============================================================================
//  PARTIDO (nuevo): armar lados, marcador en vivo con reloj, cambio de lados
//  (cruza los mandos), final con historial y tabla de la sala.
//  Integración: la web recibe partida_inicio / pausa / partida_fin y la sala
//  sale del radar mientras se juega.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t, duracion, fecha } from "../i18n.js";
import { accion, confirmar, avisar, irA } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Interruptor, Campo, Avatar, Chip, Icono, Vacio, colorPing, cx, dos, Esqueleto } from "../ui.js";
import { CampoEnVivo } from "./campo.js";

export const PESTANAS_PARTIDO = [
  { id: "vivo", es: "MARCADOR Y CAMPO", en: "SCORE & PITCH", d: ["Arma los dos lados, lleva el marcador y el reloj y mira el campo 2D en vivo, todo en una sola vista. Al terminar se guarda y se avisa en el chat y la web.", "Set up sides, keep score and time and watch the live 2D pitch, all in one view."] },
  { id: "historial", es: "HISTORIAL", en: "HISTORY", d: ["Partidos jugados en esta PC y la tabla de la sala (3 puntos por victoria).", "Matches played on this PC and the room table."] },
];

export function VistaPartido({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.sala) return html`<${Esqueleto} texto=${t("Conectando…", "Connecting…")}/>`;
  if (pestana === "historial") return html`<${Historial}/>`;
  const fase = m.partido?.fase || "libre";
  if (fase === "libre") return html`<${ResultadoYArmar} m=${m}/>`;
  return html`<${Marcador} m=${m}/>`;
}

/** Sin partido en marcha: si acaba de terminar uno, primero su resultado con REVANCHA; debajo, armar el siguiente. */
function ResultadoYArmar({ m }) {
  const r = useUltimoPartido(m);
  return html`<div class="col">${r?.reciente ? html`<${TarjetaResultado} m=${m} r=${r}/>` : null}<${Armar} m=${m}/><${CampoEnVivo} m=${m}/></div>`;
}

// ---- Armar el partido ---------------------------------------------------------------------
let borradorLados = { a: [], b: [], nombreA: "", nombreB: "", asignar: true, cpu: false };

function participantes(m) {
  const r = [];
  if (m.sala?.hostId) r.push({ parsecId: m.sala.hostId, nombre: m.sala.hostNombre || t("Anfitrión", "Host"), host: true, ping: -1, mando: m.mandos?.host || 0 });
  for (const g of m.invitados || []) if (!g.falso) r.push(g);
  return r;
}

let armarAMano = false;   // el anfitrión abrió el armado manual (se recuerda mientras la interfaz siga abierta)

// Lados automáticos: no hay que armar nada; se ve cómo quedarán y al pitazo se arman (y se corrigen) solos según el juego
function ArmarAuto({ m, aMano }) {
  const pv = m.partido?.previa;
  const fila = (j) => html`<div class="persona on" style="cursor:default" key=${j.parsecId || "cpu"}>
    ${j.parsecId ? html`<${Avatar} nombre=${j.nombre} id=${j.parsecId} url=${m.perfiles?.[j.parsecId]?.avatar_url} t=${34}/>` : html`<span style="font-size:22px">🤖</span>`}
    <div class="nom" style="flex:1">${j.parsecId ? j.nombre : t("La máquina (CPU)", "The CPU")}</div></div>`;
  const col = (id, color, l) => html`<${Tarjeta} estilo="flex:1;min-width:0" interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
    <div class="lab" style=${`color:${color}`}>${id === "a" ? t("EQUIPO A", "TEAM A") : t("EQUIPO B", "TEAM B")}</div>
    ${(l || []).length ? l.map(fila) : html`<div class="ayuda">${t("Nadie todavía.", "Nobody yet.")}</div>`}
  </${Tarjeta}>`;
  return html`<div class="col">
    <div class="caja" style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
      <${Icono} n="rayo" t=${22}/>
      <div style="flex:1;min-width:220px"><b>${t("Los lados se arman solos al empezar el partido", "Sides are set automatically at kick-off")}</b>
        <div class="ayuda">${t("No hace falta tocar nada: al pitazo Link pone a cada uno con su mando y, en el primer minuto, mira en el juego en qué lado juega cada uno y lo corrige solo si hace falta.", "Nothing to do: at kick-off Link sets everyone by their pad and, in the first minute, checks each side in the game and fixes it if needed.")}</div></div>
      <${Boton} tipo="mini suave" al=${aMano}>${t("ARMAR A MANO", "SET UP MANUALLY")}</${Boton}>
    </div>
    ${pv ? html`<div class="fila partible">${col("a", "var(--acc)", pv.a)}${col("b", "var(--eq-b)", pv.b)}</div>`
      : html`<${Vacio} icono="mandos" titulo=${t("Nadie tiene mando todavía", "Nobody has a pad yet")} texto=${t("Cuando cada uno tenga su mando (o tú toques el tuyo), aparece aquí en su equipo.", "Once everyone has a pad (or you touch yours), they show up here.")}><${Boton} tipo="lleno" al=${() => irA("mandos", "puestos")}>${t("IR A PUESTOS", "GO TO SEATS")}</${Boton}></${Vacio}>`}
    ${pv && (pv.mirando || []).length ? html`<${Tarjeta} interior="padding:18px 24px;display:flex;flex-direction:column;gap:10px">
      <div class="lab mut">${t("MIRANDO (mando en el centro del juego)", "WATCHING (pad in the middle)")}</div>
      ${pv.mirando.map(fila)}
      <div class="ayuda">${t("No juegan este partido pero conservan su puesto: si mueven su mando a un lado, vuelven a jugar.", "Not playing this match but they keep their seat: moving the pad to a side brings them back.")}</div>
    </${Tarjeta}>` : null}
    <div class="ayuda">${pv && pv.fuente === "juego"
      ? t("Así quedan ahora según la pantalla de mandos del juego (izquierda = Equipo A, derecha = Equipo B, centro = Mirando).", "As picked on the game's controller screen (left = Team A, right = Team B, middle = Watching).")
      : t("Así quedan ahora según el equipo de cada puesto (Mandos › Puestos). Si en el juego alguien se pone del otro lado, Link lo ve y lo corrige.", "This is how they stand now by seat team (Pads › Seats). If someone picks the other side in the game, Link fixes it.")}</div>
    ${m.arbitro && m.arbitro.pantallaLados ? html`<div class="caja" style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
      <${Interruptor} valor=${!!m.arbitro.mirandoAuto} al=${(v) => accion("partido.mirandoAuto", { si: v })} etiqueta=${t("Mirando automático", "Auto watching")}/>
      <div style="flex:1;min-width:220px"><b>${t("Mirando automático", "Auto watching")}</b>
        <div class="ayuda">${t("Quien deja su mando en el centro de la pantalla de mandos no juega (sigue en su puesto).", "Whoever leaves their pad in the middle doesn't play (keeps the seat).")}</div></div>
    </div>` : null}
  </div>`;
}

function Armar({ m }) {
  const [manual, setManual] = useState(armarAMano);
  const auto = !!(m.arbitro && m.arbitro.ladosAuto && m.arbitro.marcadorAuto);
  if (auto && !manual && m.sala.abierta) return html`<${ArmarAuto} m=${m} aMano=${() => { armarAMano = true; setManual(true); }}/>`;
  return html`<${ArmarManual} m=${m} volver=${auto ? () => { armarAMano = false; setManual(false); } : null}/>`;
}

function ArmarManual({ m, volver }) {
  const [b, setB] = useState(borradorLados);
  const fijar = (x) => { borradorLados = { ...b, ...x }; setB(borradorLados); };
  const gente = participantes(m);
  const [clas, ponerClas] = useClasificatoria(m.sala.abierta);
  const sinCuenta = new Set((clas?.activa ? clas.sinCuenta : []).map((x) => x.parsecId));
  const presentes = new Set(gente.map((g) => g.parsecId));
  const a = b.a.filter((id) => presentes.has(id));
  const cpu = !!b.cpu;   // lado B = la máquina (CPU del juego): no cuenta para el ranking ni como resultado oficial
  const bb = cpu ? [] : b.b.filter((id) => presentes.has(id));
  const lado = (id) => (a.includes(id) ? "a" : bb.includes(id) ? "b" : null);
  const mover = (id, destino) => {
    const na = a.filter((x) => x !== id), nb = bb.filter((x) => x !== id);
    if (destino === "a" && na.length < 8) na.push(id);
    if (destino === "b" && nb.length < 8 && !cpu) nb.push(id);
    fijar({ a: na, b: nb });
  };
  const desdeMandos = () => {
    const lista = m.mandos?.lista || [];
    fijar({
      a: lista.filter((p) => p.ocupado && p.equipo === "a" && p.parsecId).map((p) => p.parsecId).slice(0, 8),
      b: lista.filter((p) => p.ocupado && p.equipo === "b" && p.parsecId).map((p) => p.parsecId).slice(0, 8),
    });
  };
  const preparar = () => accion("partido.preparar", {
    a, b: bb, nombreA: b.nombreA.trim() || nombreDe(gente, a) || t("Equipo A", "Team A"),
    nombreB: b.nombreB.trim() || (cpu ? t("Máquina (CPU)", "CPU") : nombreDe(gente, bb) || t("Equipo B", "Team B")), asignar: b.asignar,
    ...(cpu ? { cpu: "b" } : {}),
  });
  if (!m.sala.abierta) {
    return html`<${Vacio} icono="sala" titulo=${t("Abre la sala para armar un partido", "Open the room to set up a match")}
      texto=${t("El marcador, el reloj y el historial funcionan con la gente que está en la sala.", "Score, clock and history work with people in the room.")}><${Boton} tipo="lleno" al=${() => irA("inicio", "inicio")}>${t("JUGAR CON UN AMIGO", "PLAY WITH A FRIEND")}</${Boton}></${Vacio}>`;
  }
  return html`<div class="col">
    ${volver ? html`<div class="caja" style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
      <div style="flex:1;min-width:220px" class="ayuda">${t("Armado a mano: los lados que elijas aquí mandan (al pitazo Link igual comprueba el juego).", "Manual setup: your sides here are used (Link still checks the game at kick-off).")}</div>
      <${Boton} tipo="mini suave" al=${volver}>${t("VOLVER A AUTOMÁTICO", "BACK TO AUTO")}</${Boton}>
    </div>` : null}
    <div class="fila partible">
      <${Columna} id="a" ids=${a} nombre=${b.nombreA} clave="nombreA" color="var(--acc)" gente=${gente} m=${m} fijar=${fijar} mover=${mover}/>
      <${Tarjeta} estilo="width:min(360px,34%);flex:none" interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
        <${Titulo} texto=${t("EN LA SALA", "IN THE ROOM")} derecha=${html`<${Boton} tipo="mini suave" al=${desdeMandos}>${t("SEGÚN MANDOS", "FROM PADS")}</${Boton}>`}/>
        ${gente.map((g) => html`<div class="persona" style="cursor:default" key=${g.parsecId}>
          <${Avatar} nombre=${g.nombre} id=${g.parsecId} url=${m.perfiles?.[g.parsecId]?.avatar_url} t=${34} brillo=${!!lado(g.parsecId)}/>
          <div style="flex:1;min-width:0"><div class="nom">${g.nombre}${g.host ? html` <${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}${sinCuenta.has(g.parsecId) ? html` <${Chip} tipo="bad">${t("SIN CUENTA", "NO ACCOUNT")}</${Chip}>` : null}</div>
            <div class="mono mut" style="font-size:12px">${g.mando ? `${t("MANDO", "PAD")} ${dos(g.mando)}` : t("sin mando", "no pad")}</div></div>
          <button class=${cx("btn mini", lado(g.parsecId) === "a" && "lleno")} onClick=${() => mover(g.parsecId, lado(g.parsecId) === "a" ? null : "a")}>A</button>
          <button class=${cx("btn mini", lado(g.parsecId) === "b" && "lleno")} onClick=${() => mover(g.parsecId, lado(g.parsecId) === "b" ? null : "b")}>B</button>
        </div>`)}
      </${Tarjeta}>
      <${Columna} id="b" ids=${bb} nombre=${b.nombreB} clave="nombreB" color="var(--eq-b)" gente=${gente} m=${m} fijar=${fijar} mover=${mover} cpu=${cpu}/>
    </div>
    <${Clasificatoria} clas=${clas} poner=${ponerClas}/>
    <div class="caja" style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
      <${Interruptor} valor=${b.asignar} al=${(v) => fijar({ asignar: v })} etiqueta="asignar"/>
      <div style="flex:1;min-width:220px"><b>${t("Sentar a cada uno en su mando", "Seat everyone on their pad")}</b>
        <div class="ayuda">${t("Cada uno va a un puesto de su equipo (A o B, los de Mandos › Puestos), en orden. A y B son equipos de Link: no tienen por qué coincidir con local y visita del juego.", "Everyone goes to a seat of their team (A or B, as set in Pads › Seats), in order. A and B are Link's teams: they don't have to match the game's home and away.")}</div></div>
      <${Boton} tipo="lleno enorme" deshabilitado=${a.length === 0 || (!cpu && bb.length === 0)} al=${preparar}><${Icono} n="partido" t=${18}/>${t("PREPARAR PARTIDO", "SET UP MATCH")}</${Boton}>
    </div>
  </div>`;
}

// ---- Sala clasificatoria: solo juega quien tiene su Parsec vinculado a la web ----------------
let ultimaClasificatoria = null;

function useClasificatoria(abierta) {
  const [c, setC] = useState(ultimaClasificatoria);
  useEffect(() => {
    if (!abierta) return undefined;
    let vivo = true;
    const leer = () => accion("partido.clasificatoria", {}, { silencioso: true }).then((r) => { if (vivo && r) { ultimaClasificatoria = r; setC(r); } });
    leer();
    const id = setInterval(leer, 5000);
    return () => { vivo = false; clearInterval(id); };
  }, [abierta]);
  return [c, (r) => { ultimaClasificatoria = r; setC(r); }];
}

function Clasificatoria({ clas, poner }) {
  if (!clas) return null;
  const fijar = (si) => accion("partido.clasificatoria", { si }).then((r) => { if (r) poner(r); });
  const nombres = (l) => l.map((x) => x.nombre).join(", ");
  return html`<div class="caja" style="display:flex;align-items:center;gap:16px;flex-wrap:wrap">
    <${Interruptor} valor=${!!clas.activa} al=${fijar} etiqueta="clasificatoria"/>
    <div style="flex:1;min-width:220px"><b>${t("Sala clasificatoria", "Ranked room")}</b>
      <div class="ayuda">${!clas.pcVinculada
        ? t("Conecta tu cuenta de la liga para usarla.", "Link this PC to the Phoenix website to use it.")
        : t("Solo juega quien tiene su Parsec vinculado a su cuenta de la web; los demás miran. Así cada partido cuenta para el ranking.", "Only people with their Parsec linked to the website can play.")}</div>
      ${clas.activa && clas.sinCuenta?.length ? html`<div class="ayuda" style="color:var(--bad)">${t("Sin cuenta (no pueden jugar): ", "No account: ")}${nombres(clas.sinCuenta)}</div>` : null}
      ${clas.activa && clas.sinComprobar?.length ? html`<div class="ayuda">${t("Comprobando: ", "Checking: ")}${nombres(clas.sinComprobar)}</div>` : null}
    </div>
  </div>`;
}

function Columna({ id, ids, nombre, clave, color, gente, m, fijar, mover, cpu }) {
  return html`<${Tarjeta} estilo="flex:1;min-width:0" interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <div class="lab" style=${`color:${color}`}>${id === "a" ? t("EQUIPO A", "TEAM A") : t("EQUIPO B", "TEAM B")}${!cpu && ids.length ? ` · ${ids.length}/8` : ""}</div>
    <${Campo} valor=${nombre} max=${40} placeholder=${cpu ? t("Máquina (CPU)", "CPU") : t("Nombre del equipo (opcional)", "Team name (optional)")} al=${(v) => fijar({ [clave]: v })}/>
    ${id === "b" ? html`<${Boton} tipo=${cx("mini", cpu ? "lleno" : "suave")} al=${() => fijar({ cpu: !cpu, b: [] })}
      titulo=${t("Juegas contra la CPU del juego. Se guarda en el historial y se ve en la web, pero no cuenta para el ranking ni como resultado oficial.", "You play against the game's CPU. Saved and shown on the web, but not ranked nor official.")}>
      🤖 ${cpu ? t("CONTRA LA MÁQUINA: SÍ", "VS CPU: ON") : t("CONTRA LA MÁQUINA (CPU)", "VS CPU")}</${Boton}>` : null}
    ${cpu ? html`<div class="persona on" style="cursor:default"><span style="font-size:22px">🤖</span><div class="nom" style="flex:1">${t("La máquina (CPU del juego)", "The game's CPU")}</div></div>
      <div class="ayuda">${t("No cuenta para el ranking ni como partido oficial. El marcador y el historial funcionan igual.", "Not ranked nor official. Score and history work the same.")}</div>` : null}
    <div style=${`display:${cpu ? "none" : "flex"};flex-direction:column;gap:8px;min-height:120px`}>
      ${ids.length === 0 ? html`<div class="ayuda">${t("Toca a alguien de la lista para sumarlo (máx. 8).", "Tap someone to add them (max 8).")}</div>` : null}
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
  const corregirLados = () => accion("partido.corregirLados", { anunciar: true }, { ok: t("Lados corregidos (los mandos no se movieron).", "Sides fixed.") });
  const ids = [...p.a.jugadores, ...p.b.jugadores].map((j) => j.parsecId);
  const red = (m.red || []).filter((r) => ids.includes(r.parsecId));
  return html`<div class="col">
    <${Tarjeta} clase="heroe" vivo=${p.fase === "en_juego"} interior="padding:26px 32px;position:relative;overflow:hidden">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:14px">
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
      <${LineaJuego} j=${m.juego} a=${m.arbitro} p=${p}/>
      ${fiesta ? html`<${Rafaga} key=${fiesta}/>` : null}
    </${Tarjeta}>
    <div class="caja" style="display:flex;gap:10px;flex-wrap:wrap;align-items:center">
      ${p.fase === "listo" ? html`<${Boton} tipo="lleno enorme" al=${() => accion("partido.iniciar", { anunciar: true })}><${Icono} n="rayo" t=${18}/>${t("¡EMPEZAR!", "KICK OFF!")}</${Boton}>` : null}
      ${p.fase === "en_juego" ? html`<${Boton} tipo="warn" al=${() => accion("partido.pausa", { si: true })}>${t("PAUSA", "PAUSE")}</${Boton}>` : null}
      ${p.fase === "pausado" ? html`<${Boton} tipo="lleno" al=${() => accion("partido.pausa", { si: false })}>${t("REANUDAR", "RESUME")}</${Boton}>` : null}
      <${Boton} al=${cambiarLados}><${Icono} n="intercambio" t=${16}/>${t("CAMBIAR LADOS", "SWAP SIDES")}</${Boton}>
      <${Boton} tipo="suave" al=${corregirLados}>${t("CORREGIR LADOS", "FIX SIDES")}</${Boton}>
      <span style="display:inline-flex;gap:8px;align-items:center" title=${t("Link mira en el juego en qué lado juega cada mando y corrige los lados solo si están al revés.", "Link checks each pad's side in game and fixes sides if reversed.")}>
        <span class="ayuda">${t("LADOS AUTO", "AUTO SIDES")}</span>
        <${Interruptor} valor=${!!(m.arbitro && m.arbitro.ladosAuto)} al=${(v) => accion("partido.ladosAuto", { si: v })} etiqueta=${t("Lados automáticos", "Auto sides")}/>
      </span>
      ${p.origenLados === "juego" ? html`<span class="ayuda">${t("Lados comprobados en el juego.", "Sides checked in game.")}</span>`
        : m.arbitro && m.arbitro.ladosEstado === "midiendo" ? html`<span class="ayuda">${t("Mirando en el juego en qué lado juega cada mando…", "Checking each pad's side in game…")}</span>`
        : p.origenLados === "mandos" ? html`<span class="ayuda">${t("Lados armados solos según los mandos.", "Sides set from pads.")}</span><span class="help-dot" title=${t("Si en el juego es al revés, pulsa CORREGIR LADOS (no mueve mandos).", "If reversed in game, press FIX SIDES (does not move pads).")}>?</span>` : null}
      <div style="margin-left:auto;display:flex;gap:10px">
        ${enJuego ? html`<${Boton} tipo="ok" al=${finalizar}><${Icono} n="trofeo" t=${16}/>${t("TERMINAR", "FINISH")}</${Boton}>` : null}
        <${Boton} tipo="peligro" al=${cancelar}>${t("CANCELAR", "CANCEL")}</${Boton}>
      </div>
    </div>
    <${CampoEnVivo} m=${m}/>
    <div class="rej2" style="align-items:start">
      <${PartidoOficial} m=${m}/>
      <${TorneoYDuelo} m=${m}/>
    </div>
    ${red.length ? html`<div class="rej4">${red.map((r) => html`<div class="caja" key=${r.parsecId}>
      <div style="display:flex;justify-content:space-between"><b style="white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${r.nombre}</b>
        <span class="mono" style=${`color:${colorPing(r.ultimo)}`}>${r.ultimo >= 0 ? r.ultimo + " ms" : "—"}</span></div>
      <${MiniSerie} serie=${r.serie}/>
    </div>`)}</div>` : null}
  </div>`;
}

// Partido oficial (liga automática): elegir qué fecha del calendario de la web se juega. El Equipo A (lado a) es el DT local
// del calendario. Al terminar (con el módulo del juego) el resultado sube a la web y lo valida el staff o el árbitro.
function PartidoOficial({ m }) {
  const lg = m.liga || { partidos: [] };
  useEffect(() => { accion("partido.oficiales", {}, { silencioso: true }); }, []);
  const lista = lg.partidos || [];
  const mios = lista.filter((x) => x.mio), resto = lista.filter((x) => !x.mio);
  const op = (x) => html`<option value=${x.id}>${`F${x.fecha} · ${x.local} vs ${x.visitante}${x.estado === "disputa" ? " (en disputa)" : x.estado === "propuesto" ? " (ya enviado)" : ""}`}</option>`;
  return html`<div class="caja" style="display:flex;gap:12px;flex-wrap:wrap;align-items:center">
    <b class="disp" style="font-size:13px;letter-spacing:.14em">${t("PARTIDO OFICIAL", "OFFICIAL MATCH")}</b>
    <select class="campo" style="flex:1;min-width:240px" value=${lg.elegido || 0}
      onChange=${(e) => accion("partido.oficial", { id: Number(e.target.value) })}>
      <option value="0">${t("Amistoso (no cuenta para la liga)", "Friendly (not for the league)")}</option>
      ${mios.length ? html`<optgroup label=${t("Tus partidos", "Your matches")}>${mios.map(op)}</optgroup>` : null}
      ${resto.length ? html`<optgroup label=${lista[0]?.edicion || t("Liga", "League")}>${resto.map(op)}</optgroup>` : null}
    </select>
    <${Boton} tipo="mini suave" al=${() => accion("partido.oficiales", {}, { ok: t("Lista actualizada.", "List updated.") })}>${t("ACTUALIZAR", "REFRESH")}</${Boton}>
    ${lg.enVivo ? html`<${Chip} tipo="acc"><span class="punto ok vivo"></span> ${t("EN LA WEB", "ON THE WEB")}</${Chip}>` : null}
    <div class="ayuda" style="flex-basis:100%;font-size:13px">
      ${lg.elegido
        ? t("Pon al DT local del calendario en el Equipo A. Al terminar, el resultado sube solo a la web y lo valida el staff o el árbitro designado.", "Put the calendar's HOME manager on Team A. At full time the result goes to the web and staff or the assigned referee validates it.")
        : t("Elige la fecha si es un partido de liga. El marcador se ve en vivo en la web (los amistosos solo si la sala no es privada).", "Pick the fixture if it's a league match. The score shows live on the web (friendlies only if the room isn't private).")}
      ${lg.ultimoEnvio ? html`<div style="margin-top:4px"><b>${lg.ultimoEnvio}</b>${lg.pendientes ? ` (${lg.pendientes} ${t("por enviar", "queued")})` : ""}</div>` : null}
      ${lg.error ? html`<div style="margin-top:4px;color:var(--warn)">${lg.error}</div>` : null}
    </div>
  </div>`;
}

// Torneo premium/oficial (función «torneos») y duelo de la web (función «goles»), 0.3.47. El resultado del torneo sube
// solo al final y lo confirma el organizador o el staff; los goles del duelo van a «Goleadores de los duelos».
function TorneoYDuelo({ m }) {
  const tn = m.torneos || { partidos: [] };
  const du = m.duelo || { id: 0 };
  const [num, setNum] = useState("");
  useEffect(() => { accion("partido.torneos", {}, { silencioso: true }); }, []);
  const lista = tn.partidos || [];
  const mios = lista.filter((x) => x.mio), resto = lista.filter((x) => !x.mio);
  const valor = tn.elegido ? `${tn.elegido}|${tn.elegidaClave}` : "0";
  const op = (x) => html`<option value=${`${x.torneoId}|${x.clave}`}>${`${x.torneo}${x.ronda ? " · " + x.ronda : ""} · ${x.a} vs ${x.b}${x.estado === "disputa" ? " (en disputa)" : x.estado === "propuesto" ? " (ya enviado)" : ""}`}</option>`;
  const elegir = (v) => { const [id, ...c] = v.split("|"); accion("partido.torneo", { torneo_id: Number(id), clave: c.join("|") }); };
  const ligar = () => { const n = Number(String(num).replace(/[^0-9]/g, "")); if (n > 0) { accion("partido.duelo", { id: n }, { ok: t("Sala ligada al duelo.", "Room linked to the duel.") }); setNum(""); } };
  return html`<div class="caja" style="display:flex;flex-direction:column;gap:10px">
    <div style="display:flex;gap:12px;flex-wrap:wrap;align-items:center">
      <b class="disp" style="font-size:13px;letter-spacing:.14em">${t("TORNEO", "TOURNAMENT")}</b>
      <select class="campo" style="flex:1;min-width:240px" value=${valor} onChange=${(e) => elegir(e.target.value)}>
        <option value="0">${t("Ninguno", "None")}</option>
        ${mios.length ? html`<optgroup label=${t("Tus partidos", "Your matches")}>${mios.map(op)}</optgroup>` : null}
        ${resto.length ? html`<optgroup label=${t("Otros torneos", "Other tournaments")}>${resto.map(op)}</optgroup>` : null}
      </select>
      <${Boton} tipo="mini suave" al=${() => accion("partido.torneos", {}, { ok: t("Lista actualizada.", "List updated.") })}>${t("ACTUALIZAR", "REFRESH")}</${Boton}>
    </div>
    <div class="ayuda" style="font-size:13px">
      ${tn.elegido ? t("Al terminar, el resultado sube solo a la web. Lo confirma el organizador (torneo premium) o el staff (oficial).", "At full time the result goes to the web. The organizer (premium) or staff (official) confirms it.")
        : lista.length ? t("Elige el cruce si este partido es de un torneo premium u oficial.", "Pick the match if it's from a premium or official tournament.")
        : t("No hay partidos de torneos premium u oficiales por jugar.", "No premium or official tournament matches to play.")}
      ${tn.ultimoEnvio ? html`<div style="margin-top:4px"><b>${tn.ultimoEnvio}</b>${tn.pendientes ? ` (${tn.pendientes} ${t("por enviar", "queued")})` : ""}</div>` : null}
      ${tn.error ? html`<div style="margin-top:4px;color:var(--warn)">${tn.error}</div>` : null}
    </div>
    <div style="display:flex;gap:12px;flex-wrap:wrap;align-items:center;border-top:1px solid rgba(255,255,255,.08);padding-top:10px">
      <b class="disp" style="font-size:13px;letter-spacing:.14em">${t("DUELO DE LA WEB", "WEB DUEL")}</b>
      ${du.id ? html`<${Chip} tipo="acc">#${du.id}</${Chip}>
          <${Boton} tipo="mini suave" al=${() => accion("partido.duelo", { id: 0 })}>${t("QUITAR", "REMOVE")}</${Boton}>`
        : html`<input class="campo mono" style="width:120px" inputmode="numeric" maxLength=${10} placeholder=${t("N.º del duelo", "Duel no.")} value=${num}
            onInput=${(e) => setNum(e.currentTarget.value)} onKeyDown=${(e) => { if (e.key === "Enter") ligar(); }}/>
          <${Boton} tipo="mini" deshabilitado=${!String(num).trim()} al=${ligar}>${t("LIGAR SALA", "LINK ROOM")}</${Boton}>`}
      <div class="ayuda" style="flex-basis:100%;font-size:13px">
        ${du.id ? t("Al terminar, los goles (con su goleador) suben a «Goleadores de los duelos» de la web. Cuentan cuando los dos confirmen el marcador.", "At full time the goals (with scorer) go to the web's duel scorers. They count once both confirm the score.")
          : t("Si juegas un duelo de la web, escribe su número (sale en la página del duelo). La sala se liga a ese duelo y sus goles suben solos.", "If you play a web duel, type its number (shown on the duel page). The room is linked and its goals go up by themselves.")}
        ${du.ultimoEnvio ? html`<div style="margin-top:4px"><b>${du.ultimoEnvio}</b>${du.pendientes ? ` (${du.pendientes} ${t("por enviar", "queued")})` : ""}</div>` : null}
      </div>
    </div>
  </div>`;
}

// «Juego: en juego · 34'» (estado.json de phoenix.lua) o «sin datos del juego» (todo en manual).
const FASE_JUEGO = { menu: ["en el menú", "in menu"], en_juego: ["en juego", "playing"], pausado: ["en pausa", "paused"], descanso: ["descanso", "half time"], final: ["final", "full time"] };
function LineaJuego({ j, a, p }) {
  const hay = j?.datos;
  const f = hay ? FASE_JUEGO[j.fase] || [j.fase, j.fase] : null;
  const conMinuto = hay && (j.fase === "en_juego" || j.fase === "pausado" || j.fase === "descanso");
  // Equipos del juego (kit-server) y goles con minuto; el local del juego es el lado A salvo tras «Cambiar lados»
  const equipos = hay && (j.nombreLocal || j.nombreVisita) ? `${j.nombreLocal || "Local"} vs ${j.nombreVisita || "Visita"}` : "";
  const ladoDe = (local) => (local !== !!p?.invertido ? p?.a?.nombre : p?.b?.nombre) || (local ? "Local" : "Visita");
  // Goleador (Sider 7.3.4+ con phoenix_referee v1.1+): «34' Fralex (MESSI)»; sin él, solo el minuto
  const quien = (g) => (g.jugador ? ` (${g.jugador}${g.propia ? t(", en propia", ", own goal") : ""})` : "");
  const goles = hay ? (j.goles || []).map((g) => `${g.m}' ${ladoDe(g.local)}${quien(g)}`) : [];
  return html`<div class="mono mut" style="margin-top:14px;display:flex;flex-direction:column;gap:6px;align-items:center;font-size:13px;text-align:center">
    <div style="display:flex;gap:14px;flex-wrap:wrap;justify-content:center">
      <span>${t("Juego:", "Game:")} ${hay ? html`<b style=${j.fase === "en_juego" ? "color:var(--ok)" : ""}>${t(f[0], f[1])}</b>${conMinuto ? ` · ${j.minuto}'` : ""}` : t("sin datos del juego", "no game data")}</span>
      ${equipos ? html`<span>${equipos}</span>` : null}
      ${a?.activo ? html`<span>${t("Modo competitivo: botones bloqueados a los invitados", "Competitive mode: buttons locked for guests")}</span>` : null}
    </div>
    ${goles.length ? html`<div>${t("Goles:", "Goals:")} ${goles.join(" · ")}</div>` : null}
  </div>`;
}

function Equipo({ l, clave, enJuego, gol }) {
  return html`<div class=${cx("equipo", clave)}>
    <div class="lab" style=${`color:${clave === "a" ? "var(--acc)" : "var(--eq-b)"}`}>${clave === "a" ? t("EQUIPO A", "TEAM A") : t("EQUIPO B", "TEAM B")}</div>
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

// ---- Tarjeta de resultado y Revancha (plan UX, fase 2) -----------------------------------------
// El último partido guardado (partido.historial max 1): se vuelve a pedir cada vez que un partido termina.
let ultimo = null, faseVista = null;
const oyentesUltimo = new Set();
async function cargarUltimo() {
  const r = await accion("partido.historial", { max: 1 }, { silencioso: true });
  ultimo = r?.partidos?.[0] || null;
  oyentesUltimo.forEach((f) => f(ultimo));
}
/** El último partido; null si no hay. `reciente` = terminó hace menos de 30 min. */
export function useUltimoPartido(m) {
  const [r, setR] = useState(ultimo);
  useEffect(() => { oyentesUltimo.add(setR); if (!ultimo) cargarUltimo(); return () => oyentesUltimo.delete(setR); }, []);
  const fase = m?.partido?.fase;
  useEffect(() => {
    if ((faseVista === "en_juego" || faseVista === "pausado" || faseVista === "listo") && fase === "libre") cargarUltimo();
    faseVista = fase;
  }, [fase]);
  if (!r) return null;
  const fin = (r.inicioMs || 0) + (r.duracionSeg || 0) * 1000;
  return { ...r, reciente: Date.now() - fin < 30 * 60 * 1000 };
}

/** REVANCHA (mismos lados) o CAMBIAR LADOS. Con los lados automáticos basta con jugar otro partido en PES. */
async function revancha(m, r, cruzar) {
  const auto = !!(m.arbitro && m.arbitro.ladosAuto && m.arbitro.marcadorAuto);
  if (m.sala?.abierta) await accion("chat.enviar", { texto: cruzar ? t("🔁 ¡Revancha cambiando de lado!", "🔁 Rematch, switching sides!") : t("🔁 ¡Revancha!", "🔁 Rematch!") }, { silencioso: true });
  if (auto) {
    avisar(t("¡Revancha! Empezad otro partido en PES: Link arma los lados solo al pitazo.", "Rematch! Start another match in PES: Link sets the sides at kick-off."), "ok", 8000);
    return;
  }
  const presentes = new Set(participantes(m).map((g) => g.parsecId));
  const ids = (l) => l.jugadores.map((j) => j.parsecId).filter((id) => presentes.has(id));
  const cpuA = r.a.jugadores.some((j) => !j.parsecId), cpuB = r.b.jugadores.some((j) => !j.parsecId);
  let a = { ids: ids(r.a), nombre: r.a.nombre, cpu: cpuA }, b = { ids: ids(r.b), nombre: r.b.nombre, cpu: cpuB };
  if (cruzar) [a, b] = [b, a];
  if (a.cpu) { avisar(t("Contra la máquina, la máquina va siempre en el Equipo B: arma el partido a mano.", "Against the CPU, the CPU is always Team B: set it up manually."), "warn"); irA("partido", "vivo"); return; }
  if (!a.ids.length || (!b.cpu && !b.ids.length)) { avisar(t("Alguien del partido ya no está en la sala: arma el partido a mano.", "Someone from the match left the room: set it up manually."), "warn"); irA("partido", "vivo"); return; }
  if (await accion("partido.preparar", { a: a.ids, b: b.cpu ? [] : b.ids, nombreA: a.nombre, nombreB: b.nombre, asignar: true, ...(b.cpu ? { cpu: "b" } : {}) }, { ok: t("Revancha lista: pulsa ¡EMPEZAR! o arranca en PES.", "Rematch ready: press KICK OFF or start in PES.") })) irA("partido", "vivo");
}

/** La tarjeta del último resultado con REVANCHA y CAMBIAR LADOS. compacto = una línea (INICIO). */
export function TarjetaResultado({ m, r, compacto = false }) {
  if (!r) return null;
  const gana = r.ganador === "empate" ? t("Empate", "Draw") : t(`Ganó ${r.ganador === "a" ? r.a.nombre : r.b.nombre}`, `${r.ganador === "a" ? r.a.nombre : r.b.nombre} won`);
  const puedeJugar = !!m.sala?.abierta;
  const botones = html`<div style="display:flex;gap:8px;flex-wrap:wrap">
    <${Boton} tipo=${compacto ? "mini lleno" : "lleno"} deshabilitado=${!puedeJugar} titulo=${puedeJugar ? "" : t("Abre la sala para jugar la revancha", "Open the room to play the rematch")} al=${() => revancha(m, r, false)}><${Icono} n="rayo" t=${15}/>${t("REVANCHA", "REMATCH")}</${Boton}>
    <${Boton} tipo=${compacto ? "mini suave" : "suave"} deshabilitado=${!puedeJugar} al=${() => revancha(m, r, true)}><${Icono} n="intercambio" t=${15}/>${t("CAMBIAR LADOS", "SWITCH SIDES")}</${Boton}>
    ${compacto ? null : html`<${Boton} tipo="suave" al=${() => irA("partido", "historial")}>${t("HISTORIAL", "HISTORY")}</${Boton}>`}
  </div>`;
  if (compacto) {
    return html`<div class="caja" style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
      <span class="lab acc" style="flex:none">${r.reciente ? t("TU ÚLTIMO PARTIDO", "YOUR LAST MATCH") : t("ÚLTIMO PARTIDO", "LAST MATCH")}</span>
      <b style="flex:1;min-width:200px">${r.a.nombre} <span class="disp acc">${r.a.goles} : ${r.b.goles}</span> ${r.b.nombre} <span class="mut" style="font-weight:400">· ${gana}</span></b>
      ${botones}
    </div>`;
  }
  return html`<${Tarjeta} vivo=${r.reciente} interior="padding:24px 28px;display:flex;flex-direction:column;gap:16px">
    <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
      <span class="lab acc">${t("RESULTADO FINAL", "FULL TIME")}</span>
      <span class="mono mut" style="font-size:12px">${fecha(r.inicioMs)} · ${duracion(r.duracionSeg)}</span>
    </div>
    <div class="marcador">
      <div class="equipo a"><div class="lab" style="color:var(--acc)">${t("EQUIPO A", "TEAM A")}</div><div class="nom">${r.a.nombre}</div></div>
      <div class="goles"><div class="num a">${r.a.goles}</div><span class="sep">:</span><div class="num b">${r.b.goles}</div></div>
      <div class="equipo b"><div class="lab" style="color:var(--eq-b)">${t("EQUIPO B", "TEAM B")}</div><div class="nom">${r.b.nombre}</div></div>
    </div>
    <div style="display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap">
      <b class="disp" style="font-size:20px">${gana}</b>
      ${botones}
    </div>
  </${Tarjeta}>`;
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
  if (!lista) return html`<${Esqueleto} texto=${t("Cargando…", "Loading…")}/>`;
  if (lista.length === 0) return html`<${Vacio} icono="trofeo" titulo=${t("Aún no hay partidos", "No matches yet")} texto=${t("Cuando termines un partido desde «Marcador» quedará aquí con su marcador y duración.", "Finished matches show up here.")}/>`;
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
