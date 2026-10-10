// =============================================================================
//  SALA: Resumen (diseño v4), Opciones de sala, Juegos, Red y Actividad.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t, duracion, haceCuanto } from "../i18n.js";
import { accion, confirmar, irA, avisar } from "../tienda.js";
import {
  Cifra, Tarjeta, Titulo, Boton, Interruptor, Ajuste, AjusteSw, Stepper, Segmentos, Selector, Campo,
  Minigrafica, Chip, Icono, Vacio, colorPing, claseSemaforo, cx, dos,
} from "../ui.js";

export const PESTANAS_SALA = [
  { id: "resumen", es: "RESUMEN", en: "OVERVIEW", d: ["Tu sala de un vistazo: enlace, tiempo en vivo, quién juega y cómo va la conexión.", "Your room at a glance."] },
  { id: "opciones", es: "OPCIONES DE SALA", en: "ROOM OPTIONS", d: ["Nombre, plazas, quién puede entrar y la calidad de la imagen. Se guarda al instante.", "Name, slots, who can join and image quality."] },
  { id: "juegos", es: "JUEGOS", en: "GAMES", d: ["Qué se juega (se muestra en la web de la liga) y el juego que abre el modo quiosco.", "What is being played and the kiosk game."] },
  { id: "red", es: "RED", en: "NETWORK", d: ["El ping de cada invitado segundo a segundo. Verde va bien, ámbar aguanta, rojo es lag.", "Each guest's ping, live."] },
  { id: "actividad", es: "ACTIVIDAD", en: "ACTIVITY", d: ["Todo lo que pasó en la sala, del más nuevo al más viejo.", "Everything that happened in the room."] },
];

// Parches conocidos. Para agregar uno nuevo basta con añadir su nombre aquí.
export const PARCHES = ["Conmegol Patch 26", "Original (sin parche)"];

export const PRESETS_CALIDAD = [
  { id: "ligero", es: "LIGERO", en: "LIGHT", fps: 60, mbps: 8, d: ["Internet modesto o muchos invitados.", "Modest internet or many guests."] },
  { id: "equilibrado", es: "EQUILIBRADO", en: "BALANCED", fps: 60, mbps: 15, d: ["Lo recomendado para la liga.", "Recommended for league play."] },
  { id: "maxima", es: "MÁXIMA", en: "MAX", fps: 60, mbps: 30, d: ["Fibra y pocos invitados.", "Fiber and few guests."] },
];

export function VistaSala({ s, pestana }) {
  const m = s.motor;
  if (!m || !m.sala) return html`<${Vacio} titulo=${t("Conectando con el motor…", "Connecting to the engine…")} texto=""/>`;
  switch (pestana) {
    case "opciones": return html`<${OpcionesSala} m=${m}/>`;
    case "juegos": return html`<${JuegosSala} m=${m}/>`;
    case "red": return html`<${RedSala} m=${m}/>`;
    case "actividad": return html`<${ActividadSala} s=${s}/>`;
    default: return html`<${ResumenSala} s=${s} m=${m}/>`;
  }
}

// ---- Resumen -------------------------------------------------------------------------
function useSegundos(base) {
  // El motor manda los segundos 5 veces por segundo; se interpola para el reloj
  const [, setX] = useState(0);
  useEffect(() => { const i = setInterval(() => setX((x) => x + 1), 1000); return () => clearInterval(i); }, []);
  return base;
}

export async function abrirSala() {
  return accion("sala.abrir", {}, { ok: null });
}
export async function cerrarSala(m) {
  const enJuego = m?.partido?.fase === "en_juego" || m?.partido?.fase === "pausado";
  const ok = await confirmar({
    titulo: t("¿Detener la sala?", "Stop the room?"),
    texto: enJuego
      ? t("Hay un partido en juego: se cancelará sin guardarse. Todos los invitados saldrán.", "A match is in progress: it will be cancelled. Everyone will be disconnected.")
      : t("Todos los invitados saldrán y el enlace dejará de funcionar.", "Everyone will be disconnected and the link will stop working."),
    si: t("Detener", "Stop"), peligro: true,
  });
  if (ok) await accion("sala.cerrar");
}

function Heroe({ m }) {
  const sala = m.sala;
  useSegundos(sala.segundos);
  const titulo = sala.opciones?.nombre || sala.phoenix?.juego || "eFootball PES 2021";
  const detalle = [sala.phoenix?.juego, sala.phoenix?.parche].filter((x) => x && x !== titulo).join(" · ");
  if (!sala.abierta) {
    return html`<${Tarjeta} clase="heroe" estilo="flex:1;min-height:236px" interior="padding:26px 30px;display:flex;flex-direction:column;gap:18px">
      <div style="display:flex;justify-content:space-between;align-items:flex-start;gap:24px;flex-wrap:wrap">
        <div style="display:flex;flex-direction:column;min-width:0;flex:1 1 320px">
          <div class="lab">${t("SALA CERRADA", "ROOM CLOSED")}</div>
          <div class="grande degradado" style="margin-top:12px;font-size:56px">${titulo}</div>
          ${detalle ? html`<div class="mono mut" style="margin-top:6px;font-size:13px;letter-spacing:.08em">${detalle}</div>` : null}
          <div class="ayuda" style="margin-top:8px;font-size:14px">
            ${sala.lista
              ? t("Abre la sala y comparte el enlace. Tú decides quién juega y quién mira.", "Open the room and share the link. You decide who plays and who watches.")
              : t("Parsec se está preparando. Si tarda, revisa Ajustes › Diagnóstico.", "Parsec is getting ready. If it takes long, check Settings › Diagnostics.")}
          </div>
        </div>
        <div class="derecha" style="display:flex;gap:12px;flex:none">
          <div class="caja" style="min-width:120px;display:flex;flex-direction:column;gap:4px">
            <div class="lab">${t("PLAZAS", "SLOTS")}</div>
            <div class="disp" style="font-size:40px;font-weight:700;line-height:1;margin-top:6px">${sala.plazas}</div>
            <div class="mono mut" style="font-size:12px">${t("invitados a la vez", "guests at once")}</div>
          </div>
          <div class="caja" style="min-width:120px;display:flex;flex-direction:column;gap:4px">
            <div class="lab">${t("VISIBILIDAD", "VISIBILITY")}</div>
            <div class="disp" style="font-size:28px;font-weight:700;line-height:1.2;margin-top:10px">${textoVisibilidad(sala.phoenix?.visibilidad)}</div>
            <div class="mono mut" style="font-size:12px">${t("quién ve tu sala", "who sees your room")}</div>
          </div>
        </div>
      </div>
      <div style="display:flex;gap:10px;flex-wrap:wrap">
        <${Boton} tipo="lleno enorme" deshabilitado=${!sala.lista} al=${abrirSala}><${Icono} n="rayo" t=${18}/>${t("ABRIR SALA", "OPEN ROOM")}</${Boton}>
        <${Boton} tipo="suave enorme" al=${() => irA("sala", "opciones")}>${t("OPCIONES", "OPTIONS")}</${Boton}>
      </div>
      <div style="margin-top:auto">
        <div class="lab acc" style="margin-bottom:10px">${t("ANTES DE ABRIR", "BEFORE OPENING")}</div>
        <${ListaParaAbrir} m=${m}/>
      </div>
    </${Tarjeta}>`;
  }
  const copiar = () => accion("sala.copiarEnlace", {}, { ok: t("Enlace copiado. Pégalo en Discord o WhatsApp.", "Link copied.") });
  return html`<${Tarjeta} vivo clase="heroe" estilo="min-height:236px" interior="padding:26px 30px;display:flex;justify-content:space-between;gap:24px">
    <div style="display:flex;flex-direction:column;min-width:0;flex:1">
      <div class="lab acc">${t("SALA EN VIVO", "LIVE ROOM")}</div>
      <div class="grande degradado" style="margin-top:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${titulo}</div>
      ${detalle ? html`<div class="mono mut" style="margin-top:6px;font-size:13px;letter-spacing:.08em">${detalle}</div>` : null}
      <div class="ayuda" style="margin-top:8px;font-size:14px">${t("Los invitados entran con el enlace. Tú decides quién juega y quién mira.", "Guests join with the link. You decide who plays and who watches.")}</div>
      <${PingSala} m=${m}/>
      <div style="margin-top:auto;padding-top:16px;display:flex;gap:10px;align-items:center">
        <div class="campo mono" style="flex:1;display:flex;align-items:center;height:44px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;user-select:text">${sala.enlace || t("Generando enlace…", "Generating link…")}</div>
        <${Boton} tipo="" estilo="height:44px" deshabilitado=${!sala.enlace} al=${copiar}>${t("COPIAR", "COPY")}</${Boton}>
      </div>
    </div>
    <div class="derecha" style="width:190px;display:flex;flex-direction:column;justify-content:space-between;gap:16px">
      <div style="text-align:right"><div class="lab">${t("TIEMPO EN VIVO", "LIVE TIME")}</div>
        <div class="reloj" style="margin-top:8px;font-size:34px">${duracion(sala.segundos)}</div></div>
      ${sala.opciones?.pendiente ? html`<${Boton} tipo="warn" al=${() => accion("sala.aplicar", {}, { ok: t("Cambios aplicados.", "Changes applied.") })}>${t("APLICAR CAMBIOS", "APPLY CHANGES")}</${Boton}>` : null}
      <${Boton} tipo="peligro lleno enorme" al=${() => cerrarSala(m)}>${t("DETENER SALA", "STOP ROOM")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

// Ping de cada invitado a la vista: verde hasta 50 ms, naranja de 50 a 120, rojo de 120 para arriba.
function PingSala({ m }) {
  const lista = (m.red || []).filter((r) => r.presente);
  if (lista.length === 0) return html`<div class="mono mut" style="margin-top:14px;font-size:12px;letter-spacing:.1em">${t("PING DE LA SALA · esperando invitados", "ROOM PING · waiting for guests")}</div>`;
  return html`<div style="margin-top:14px;display:flex;flex-direction:column;gap:8px">
    <div class="mono mut" style="font-size:12px;letter-spacing:.1em">${t("PING DE LA SALA", "ROOM PING")}</div>
    <div style="display:flex;gap:8px;flex-wrap:wrap">${lista.map((r) => {
      const ms = r.ultimo >= 0 ? r.ultimo : r.media;
      const c = colorPing(ms);
      return html`<div key=${r.parsecId} title=${t("Media ", "Avg ") + (r.media >= 0 ? r.media : "—") + " ms"}
        style=${`display:flex;align-items:center;gap:8px;padding:6px 12px;border-radius:999px;border:1px solid ${c};background:rgba(0,0,0,.25)`}>
        <span style=${`width:8px;height:8px;border-radius:50%;background:${c}`}></span>
        <span style="font-size:13px;max-width:130px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${r.nombre || "—"}</span>
        <span class="mono" style=${`font-size:13px;font-weight:600;color:${c}`}>${ms >= 0 ? ms + " ms" : "—"}</span>
      </div>`;
    })}</div>
  </div>`;
}

function ListaParaAbrir({ m }) {
  const mandos = m.mandos?.lista || [];
  const conectados = mandos.filter((p) => p.conectado).length;
  const web = m.web?.estado;
  const pasos = [
    { ok: m.sala.lista, mal: !m.sala.lista, t: t("Parsec listo", "Parsec ready"), d: m.sala.lista ? (m.sala.cuentaHost || t("Sesión iniciada", "Signed in")) : t("Preparando…", "Starting…"), ir: ["ajustes", "diagnostico"] },
    { ok: conectados > 0, mal: mandos.length === 0, t: t("Mandos virtuales", "Virtual pads"), d: mandos.length ? `${conectados}/${mandos.length} ${t("conectados", "connected")}` : t("Falta ViGEmBus", "ViGEmBus missing"), ir: ["mandos", "puestos"] },
    { ok: web === "conectado", mal: web === "sin_conexion", t: t("Web de la liga", "League website"), d: web === "conectado" ? (m.web.usuario || t("Vinculada", "Linked")) : web === "sin_vincular" ? t("Toca para vincular", "Tap to link") : t("Sin conexión", "Offline"), ir: ["ajustes", "web"] },
    { ok: (m.sala.calidad?.fps || 0) > 0, mal: false, t: t("Conexión", "Connection"), d: `${m.sala.calidad?.fps ?? "—"} FPS · ${m.sala.calidad?.mbps ?? "—"} MBPS`, ir: ["sala", "red"] },
  ];
  return html`<div class="pasos" style="margin-top:0">${pasos.map((p) => html`
    <button class=${cx("paso", p.ok ? "ok" : p.mal ? "mal" : "falta")} onClick=${() => irA(p.ir[0], p.ir[1])}>
      <span class="marca-paso">${p.ok ? "✓" : p.mal ? "×" : "!"}</span>
      <span style="min-width:0"><div class="t">${p.t}</div><div class="d">${p.d}</div></span>
    </button>`)}</div>`;
}

function textoVisibilidad(v) {
  return v === "publica" ? t("Pública", "Public") : v === "privada" ? t("Privada", "Private") : t("Amigos", "Friends");
}

function PuestosResumen({ m }) {
  const lista = (m.mandos?.lista || []);
  const activos = lista.filter((x) => x.equipo !== "fuera");
  const visibles = (activos.length ? activos : lista).slice(0, 8);
  const ocupados = visibles.filter((x) => x.ocupado).length;
  return html`<${Tarjeta} estilo="flex:none" interior="padding:18px 22px;display:flex;flex-direction:column;gap:12px">
    <div style="display:flex;justify-content:space-between;align-items:center">
      <div class="lab acc">${t("PUESTOS", "SEATS")}</div>
      <button class="mono mut" style="background:none;border:0;cursor:pointer;font-size:13px" onClick=${() => irA("mandos", "puestos")}>
        ${ocupados} ${t("DE", "OF")} ${visibles.length} ${t("OCUPADOS", "TAKEN")} › ${t("ver y arrastrar", "view and drag")}</button>
    </div>
    ${visibles.length === 0
      ? html`<div class="ayuda">${t("No hay mandos virtuales. Revisa Ajustes › Diagnóstico (ViGEmBus).", "No virtual pads. Check Settings › Diagnostics (ViGEmBus).")}</div>`
      : html`<div class="rej4">${visibles.map((p) => html`<${MiniPuesto} p=${p} key=${p.n}/>`)}</div>`}
  </${Tarjeta}>`;
}

function MiniPuesto({ p }) {
  const ping = p.ocupado && p.ping >= 0 ? p.ping : null;
  return html`<div class=${cx("puesto", p.equipo, p.ocupado ? "ocupado" : "libre")}
      style=${`min-height:0;flex-direction:row;align-items:center;gap:14px;padding:12px 14px;${p.ocupado ? "" : "border-style:dashed;border-width:1.5px;border-color:rgba(255,255,255,.2);background:rgba(0,0,0,.2)"}`}>
    <div class="numeral" style="font-size:38px;flex:none">${dos(p.n)}</div>
    <div style="min-width:0;flex:1">
      <div class="nombre" style=${`font-size:15px;${p.ocupado ? "" : "color:var(--mut)"}`}>${p.ocupado ? p.jugador : t("Puesto libre", "Free seat")}</div>
      <div class="mono mut" style="font-size:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${p.ocupado
        ? html`${p.equipo === "local" ? "LOCAL" : p.equipo === "visitante" ? t("VISITA", "AWAY") : "—"}${ping != null ? html` · <span style=${`color:${colorPing(ping)}`}>${ping} ms</span>` : null}${p.bloqueado ? " · 🔒" : ""}`
        : t("esperando jugador", "waiting for player")}</div>
    </div>
  </div>`;
}

function Indicadores({ m }) {
  const presentes = (m.red || []).filter((r) => r.presente && r.media >= 0);
  const pingMedio = presentes.length ? Math.round(presentes.reduce((a, r) => a + r.media, 0) / presentes.length) : null;
  const caja = (lab, valor, unidad, color) => html`
    <div class="caja" style="border-color:var(--linea);background:rgba(13,18,42,.6)">
      <div class="lab" style="letter-spacing:.12em">${lab}</div>
      <div class="disp" style=${`margin-top:10px;font-size:30px;font-weight:700;line-height:1;color:${color}`}>${typeof valor === "number" ? html`<${Cifra} valor=${valor}/>` : valor}</div>
      <div class="mono mut" style="margin-top:6px;font-size:12px">${unidad}</div>
    </div>`;
  return html`<div class="rej3" style="gap:12px">
    ${caja(t("INVITADOS", "GUESTS"), `${m.sala.invitados}/${m.sala.plazas}`, m.sala.abierta ? t("EN SALA", "IN ROOM") : t("SALA CERRADA", "CLOSED"), "#fff")}
    ${caja(t("PING MEDIO", "AVG PING"), pingMedio == null ? "—" : pingMedio, t("MILISEGUNDOS", "MILLISECONDS"), colorPing(pingMedio))}
    ${caja(t("CALIDAD", "QUALITY"), m.sala.calidad?.fps ?? "—", `FPS · ${m.sala.calidad?.mbps ?? "—"} MBPS`, "var(--acc)")}
  </div>`;
}

export function colorLinea(texto) {
  const x = texto.toLowerCase();
  if (/(ban|kick|expuls|desconect|left|salió|error|fail)/.test(x)) return "var(--warn)";
  if (/(joined|entró|asign|connected|conect|ok)/.test(x)) return "var(--ok)";
  return "var(--acc)";
}

function ActividadResumen({ s }) {
  const ultimas = s.actividad.slice(-6).reverse();
  return html`<${Tarjeta} estilo="flex:1;min-height:0" interior="padding:22px;display:flex;flex-direction:column;gap:14px;overflow:hidden">
    <div style="display:flex;justify-content:space-between"><div class="lab acc">${t("ACTIVIDAD", "ACTIVITY")}</div>
      <button class="mono mut" style="background:none;border:0;cursor:pointer;font-size:12px" onClick=${() => irA("sala", "actividad")}>${t("VER TODO", "SEE ALL")} ›</button></div>
    ${ultimas.length === 0 ? html`<div style="flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;text-align:center;padding:0 24px">
      <span style="color:var(--mut);opacity:.7"><${Icono} n="rayo" t=${40} w=${1.3}/></span>
      <div style="font:600 16px/1.2 var(--f-body)">${t("Todo tranquilo por ahora", "All quiet for now")}</div>
      <div class="ayuda" style="font-size:14px">${t("Aquí verás quién entra, quién sale y los avisos de la sala en cuanto la abras.", "You will see who joins, who leaves and room notices here once the room is open.")}</div>
    </div>` : null}
    ${ultimas.map((e) => html`<div key=${e.id} style="display:flex;gap:12px;align-items:flex-start;padding-bottom:12px;border-bottom:1px solid var(--tenue)">
      <span class="punto" style=${`margin-top:6px;background:${colorLinea(e.texto)}`}></span>
      <div style="min-width:0"><div style="font-size:14px;word-break:break-word">${e.texto}</div>
        ${e.en ? html`<div class="mono mut" style="margin-top:4px;font-size:12px">${haceCuanto(e.en)}</div>` : null}</div>
    </div>`)}
  </${Tarjeta}>`;
}

function ResumenSala({ s, m }) {
  return html`<div class="fila resumen" style="min-height:100%">
    <div class="col" style="flex:1 1 0">
      <${Heroe} m=${m}/>
      <${PuestosResumen} m=${m}/>
    </div>
    <div class="col" style="flex:0 0 clamp(360px,25vw,440px)">
      <${Indicadores} m=${m}/>
      <${ActividadResumen} s=${s}/>
    </div>
  </div>`;
}

// ---- Opciones de sala ---------------------------------------------------------------
export function TarjetaCalidad({ m }) {
  const fps = m.sala?.calidad?.fps ?? m.ajustes?.video?.fps ?? 60;
  const mbps = m.sala?.calidad?.mbps ?? m.ajustes?.video?.mbps ?? 15;
  const actual = PRESETS_CALIDAD.find((p) => p.fps === fps && p.mbps === mbps);
  const aplicar = (f, b) => accion("sala.calidad", { fps: f, mbps: b });
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("CALIDAD DE TRANSMISIÓN", "STREAM QUALITY")} derecha=${actual
      ? html`<span class="mono mut" style="font-size:12px">${fps} FPS · ${mbps} MBPS</span>`
      : html`<span class="chip acc2">${t("PERSONALIZADA", "CUSTOM")} · ${mbps} MBPS</span>`}/>
    <div class="rej3" style="gap:10px">${PRESETS_CALIDAD.map((p) => html`
      <button class="caja" style=${`cursor:pointer;text-align:left;border-color:${actual?.id === p.id ? "var(--acc)" : "var(--tenue)"};background:${actual?.id === p.id ? "var(--acc-suave)" : ""}`}
        onClick=${() => aplicar(p.fps, p.mbps)}>
        <div class="lab" style=${actual?.id === p.id ? "color:var(--acc)" : ""}>${t(p.es, p.en)}</div>
        <div class="disp" style="font-size:22px;font-weight:700;margin-top:8px">${p.mbps} <span class="mut" style="font-size:12px">MBPS</span></div>
        <div class="ayuda" style="margin-top:4px">${t(...p.d)}</div>
      </button>`)}</div>
    <${Ajuste} titulo=${t("Fotogramas por segundo", "Frames per second")} desc=${t("60 es lo ideal para PES.", "60 is ideal for PES.")}>
      <${Stepper} valor=${fps} min=${10} max=${250} paso=${5} al=${(v) => aplicar(v, mbps)}/></${Ajuste}>
    <${Ajuste} titulo=${t("Ancho de banda (Mbps)", "Bandwidth (Mbps)")} desc=${t("Escribe el valor que quieras (1–1000) o usa − / +. Más alto = mejor imagen, pero exige más a tu internet.", "Type any value (1–1000) or use − / +.")}>
      <${Stepper} valor=${mbps} min=${1} max=${1000} ancho=${4} al=${(v) => aplicar(fps, v)}/></${Ajuste}>
  </${Tarjeta}>`;
}

function OpcionesSala({ m }) {
  const o = m.sala.opciones || {};
  const ph = m.sala.phoenix || {};
  const op = (datos) => accion("sala.opciones", datos);
  const phx = (datos) => accion("sala.phoenix", datos);
  return html`<div class="col">
    ${o.pendiente ? html`<div class="caja" style="border-color:var(--warn);display:flex;align-items:center;gap:14px;justify-content:space-between">
      <div><b class="warn">${t("Hay cambios sin aplicar.", "There are pending changes.")}</b>
        <div class="ayuda">${t("La sala está abierta: aplícalos para que los invitados los vean.", "The room is open: apply them so guests see them.")}</div></div>
      <${Boton} tipo="warn" al=${() => accion("sala.aplicar", {}, { ok: t("Cambios aplicados.", "Changes applied.") })}>${t("APLICAR", "APPLY")}</${Boton}>
    </div>` : null}
    <div class="rej2">
      <div class="col">
        <${Tarjeta} interior="padding:22px 24px">
          <${Titulo} texto=${t("SALA DE PARSEC", "PARSEC ROOM")}/>
          <${Ajuste} bloque titulo=${t("Nombre de la sala", "Room name")} desc=${t("Lo que ven en Parsec (máx. 50).", "What guests see in Parsec (max 50).")}>
            <${Campo} valor=${o.nombre} max=${50} al=${(v) => op({ nombre: v })}/></${Ajuste}>
          <${Ajuste} titulo=${t("Plazas", "Slots")} desc=${t("Invitados a la vez (0–20).", "Guests at once (0–20).")}>
            <${Stepper} valor=${o.plazas ?? 0} min=${0} max=${20} al=${(v) => op({ plazas: v })}/></${Ajuste}>
          <${AjusteSw} titulo=${t("Limitar latencia", "Latency limiter")} desc=${t("Frena a quien supera el límite de ms.", "Throttles guests over the ms limit.")}
            valor=${o.limitador} al=${(v) => op({ limitador: v })}/>
          ${o.limitador ? html`<${Ajuste} titulo=${t("Límite (ms)", "Limit (ms)")}>
            <${Stepper} valor=${o.limite ?? 0} min=${0} max=${64} al=${(v) => op({ limite: v })}/></${Ajuste}>` : null}
          <${AjusteSw} titulo=${t("Turnos (hotseat)", "Hotseat")} desc=${t("Rotación automática del mando por tiempo.", "Automatic pad rotation by time.")}
            valor=${o.turnos} al=${(v) => op({ turnos: v })}/>
          <${AjusteSw} titulo=${t("Overlay en pantalla", "On-screen overlay")} desc=${t("Chat y mandos sobre el juego.", "Chat and pads over the game.")}
            valor=${o.overlay} al=${(v) => op({ overlay: v })}/>
          <${AjusteSw} titulo=${t("Modo quiosco", "Kiosk mode")} desc=${t("Abre y vigila el juego de la biblioteca.", "Launches and watches the library game.")}
            valor=${o.quiosco} al=${(v) => op({ quiosco: v })}/>
        </${Tarjeta}>
        <${TarjetaCalidad} m=${m}/>
      </div>
      <div class="col">
        <${Tarjeta} interior="padding:22px 24px">
          <${Titulo} texto=${t("PHOENIX · WEB DE LA LIGA", "PHOENIX · LEAGUE WEBSITE")}/>
          <${Ajuste} titulo=${t("Visibilidad en el radar", "Radar visibility")} desc=${t("Quién ve tu sala en la web.", "Who sees your room on the website.")}>
            <${Segmentos} valor=${ph.visibilidad} al=${(v) => phx({ visibilidad: v })} opciones=${[
              { valor: "publica", texto: t("PÚBLICA", "PUBLIC") }, { valor: "amigos", texto: t("AMIGOS", "FRIENDS") }, { valor: "privada", texto: t("PRIVADA", "PRIVATE") }]}/></${Ajuste}>
          <${AjusteSw} titulo=${t("Aceptar espectadores", "Allow spectators")} desc=${t("Pueden entrar a mirar sin mando.", "They can join to watch without a pad.")}
            valor=${ph.espectadores} al=${(v) => phx({ espectadores: v })}/>
          ${ph.espectadores ? html`<${Ajuste} titulo=${t("Máximo de espectadores", "Max spectators")}>
            <${Stepper} valor=${ph.limiteEspectadores ?? 4} min=${1} max=${16} al=${(v) => phx({ limiteEspectadores: v })}/></${Ajuste}>` : null}
          <${AjusteSw} titulo=${t("Entrada directa por Parsec", "Direct Parsec entry")} desc=${t("Quien no está en la lista de la web entra y tú decides si juega o mira.", "People not on the web list can join; you decide.")}
            valor=${ph.entradaParsec} al=${(v) => phx({ entradaParsec: v })}/>
        </${Tarjeta}>
        <${PerfilesSala}/>
      </div>
    </div>
  </div>`;
}

function PerfilesSala() {
  const [datos, setDatos] = useState(null);
  const [nombre, setNombre] = useState("");
  const cargar = async () => { const r = await accion("perfilesSala.lista", {}, { silencioso: true }); if (r) setDatos(r); };
  useEffect(() => { cargar(); }, []);
  const guardar = async () => {
    if (!nombre.trim()) { avisar(t("Ponle un nombre al perfil.", "Name the profile."), "warn"); return; }
    if (await accion("perfilesSala.guardar", { nombre }, { ok: t("Perfil guardado.", "Profile saved.") })) { setNombre(""); cargar(); }
  };
  const borrar = async (n) => {
    if (await confirmar({ titulo: t("¿Borrar perfil?", "Delete profile?"), texto: `«${n}»`, si: t("Borrar", "Delete"), peligro: true })) {
      if (await accion("perfilesSala.borrar", { nombre: n })) cargar();
    }
  };
  return html`<${Tarjeta} interior="padding:22px 24px">
    <${Titulo} texto=${t("PERFILES DE SALA", "ROOM PROFILES")}/>
    <div class="ayuda" style="margin-bottom:12px">${t("Guarda la configuración actual (sala, web, calidad y formación) y vuelve a ella con un clic: «Liga», «Amistoso», «Torneo»…",
      "Save the current setup (room, web, quality, formation) and restore it in one click.")}</div>
    <div style="display:flex;gap:8px">
      <input class="campo" placeholder=${t("Nombre del perfil", "Profile name")} maxLength="40" value=${nombre}
        onInput=${(e) => setNombre(e.currentTarget.value)} onKeyDown=${(e) => e.key === "Enter" && guardar()}/>
      <${Boton} tipo="lleno" al=${guardar}>${t("GUARDAR", "SAVE")}</${Boton}>
    </div>
    <div style="margin-top:12px;display:flex;flex-direction:column;gap:8px">
      ${(datos?.perfiles || []).map((p) => html`<div class="persona" style="cursor:default" key=${p.nombre}>
        <div style="flex:1;min-width:0"><div class="nom">${p.nombre}</div>
          <div class="sub mono mut" style="font-size:12px">${p.valores?.plazas ?? "?"} ${t("plazas", "slots")} · ${p.valores?.mbps ?? "?"} Mbps · ${p.valores?.local ?? "?"}v${p.valores?.visitante ?? "?"} · ${textoVisibilidad(p.valores?.visibilidad)}</div></div>
        <${Boton} tipo="mini" al=${() => accion("perfilesSala.aplicar", { nombre: p.nombre }, { ok: t("Perfil aplicado.", "Profile applied.") })}>${t("APLICAR", "APPLY")}</${Boton}>
        <${Boton} tipo="mini peligro" al=${() => borrar(p.nombre)}>×</${Boton}>
      </div>`)}
      ${datos && datos.perfiles.length === 0 ? html`<div class="mut" style="font-size:13px">${t("Aún no hay perfiles.", "No profiles yet.")}</div>` : null}
    </div>
  </${Tarjeta}>`;
}

// ---- Juegos --------------------------------------------------------------------------
function JuegosSala({ m }) {
  const o = m.sala.opciones || {};
  const ph = m.sala.phoenix || {};
  const juegos = o.juegos || [];
  const phx = (d) => accion("sala.phoenix", d);
  return html`<div class="rej2">
    <${Tarjeta} interior="padding:22px 24px">
      <${Titulo} texto=${t("LO QUE SE JUEGA", "WHAT'S BEING PLAYED")}/>
      <div class="ayuda" style="margin-bottom:6px">${t("Se muestra en la web de la liga y en el resumen de la sala.", "Shown on the league website and the room overview.")}</div>
      <${Ajuste} bloque titulo=${t("Juego", "Game")}><${Campo} valor=${ph.juego} max=${60} al=${(v) => phx({ juego: v })}/></${Ajuste}>
      <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:10px">
        ${["eFootball PES 2021", "SP Football Life 2026"].map((j) => html`<button class=${cx("tab", ph.juego === j && "on")} onClick=${() => phx({ juego: j })}>${j}</button>`)}
      </div>
      <${Ajuste} bloque titulo=${t("Parche", "Patch")} desc=${t("Elige uno de la lista. Si el tuyo no está, escríbelo abajo.", "Pick one from the list. If yours is missing, type it below.")}>
        <div style="display:flex;gap:8px;flex-wrap:wrap">
          ${[...PARCHES, ...(ph.parche && !PARCHES.includes(ph.parche) ? [ph.parche] : [])].map((p) => html`<button key=${p} class=${cx("tab", ph.parche === p && "on")} onClick=${() => phx({ parche: p })}>${p}</button>`)}
        </div>
        <div class="ayuda" style="margin:12px 0 6px">${t("Otro parche (extra, escríbelo)", "Other patch (extra, type it)")}</div>
        <${Campo} valor=${PARCHES.includes(ph.parche) ? "" : ph.parche} max=${60} al=${(v) => v.trim() && phx({ parche: v.trim() })}/>
      </${Ajuste}>
      <${Ajuste} bloque titulo=${t("Región", "Region")} desc=${t("Ayuda a que te encuentren rivales cercanos.", "Helps nearby rivals find you.")}>
        <${Campo} valor=${ph.region} max=${40} al=${(v) => phx({ region: v })}/></${Ajuste}>
    </${Tarjeta}>
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column">
      <${Titulo} texto=${t("BIBLIOTECA", "LIBRARY")} derecha=${html`<${Boton} tipo="mini" al=${() => accion("ui.panelClasico", { seccion: 3, pestana: 5 })}>${t("EDITAR", "EDIT")}</${Boton}>`}/>
      <div class="ayuda" style="margin-bottom:12px">${t("Juego que se abre con el modo quiosco. Agregar o quitar juegos se hace en el panel clásico.", "Game launched by kiosk mode. Add/remove games in the classic panel.")}</div>
      <div style="display:flex;flex-direction:column;gap:8px;overflow:auto">
        ${["Default", ...juegos].map((j) => html`<button key=${j} class=${cx("persona", o.biblioteca === j && "on")} onClick=${() => accion("sala.opciones", { biblioteca: j })}>
          <span class="avatar" style="width:34px;height:34px;font-size:14px;background:${o.biblioteca === j ? "var(--acc)" : "rgba(255,255,255,.15)"}">${j === "Default" ? "—" : j[0]}</span>
          <div class="nom" style="flex:1;text-align:left">${j === "Default" ? t("Ninguno (escritorio)", "None (desktop)") : j}</div>
          ${o.biblioteca === j ? html`<${Chip} tipo="acc">${t("ACTUAL", "CURRENT")}</${Chip}>` : null}
        </button>`)}
      </div>
    </${Tarjeta}>
  </div>`;
}

// ---- Red ----------------------------------------------------------------------------
function RedSala({ m }) {
  const red = m.red || [];
  if (!m.sala.abierta && red.length === 0) {
    return html`<${Vacio} titulo=${t("Sin datos de red", "No network data")} texto=${t("Abre la sala: aquí verás el ping de cada invitado segundo a segundo.", "Open the room to see each guest's ping live.")}/>`;
  }
  return html`<div class="col">
    <div class="caja" style="display:flex;gap:18px;flex-wrap:wrap;align-items:center">
      <span class="lab">${t("SEMÁFORO", "TRAFFIC LIGHT")}</span>
      <span class="semaforo"><span class="punto ok"></span>${t("≤ 60 ms y jitter ≤ 10", "≤ 60 ms, jitter ≤ 10")}</span>
      <span class="semaforo"><span class="punto warn"></span>${t("≤ 100 ms y jitter ≤ 25", "≤ 100 ms, jitter ≤ 25")}</span>
      <span class="semaforo"><span class="punto bad"></span>${t("más que eso: lag probable", "worse: likely lag")}</span>
      <span class="mut" style="font-size:12px;margin-left:auto">${t("Medias de los últimos 30 s · gráfica de 2 min", "30 s averages · 2 min chart")}</span>
    </div>
    ${red.length === 0 ? html`<${Vacio} titulo=${t("Nadie en la sala", "Nobody here")} texto=${t("Cuando entren invitados aparecerán aquí.", "Guests will show up here.")}/>` : null}
    <div class="rej2">${red.map((r) => html`<${Tarjeta} key=${r.parsecId} suave interior="padding:18px 20px">
      <div style="display:flex;justify-content:space-between;align-items:center;gap:10px">
        <div style="min-width:0"><div style="font-weight:600;font-size:16px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${r.nombre || "#" + r.parsecId}</div>
          <div class="mono mut" style="font-size:12px">#${r.parsecId} ${r.presente ? "" : t("· se fue", "· left")}</div></div>
        <div style="text-align:right"><div class="disp" style=${`font-size:30px;font-weight:700;color:${colorPing(r.ultimo)}`}>${r.ultimo >= 0 ? r.ultimo : "—"}<span class="mut" style="font-size:12px"> ms</span></div>
          <span class="semaforo"><span class=${cx("punto", claseSemaforo(r.semaforo))}></span>${r.alerta ? t("LAG AHORA", "LAGGING") : (r.semaforo || "").replace("_", " ").toUpperCase()}</span></div>
      </div>
      <div style="margin:12px 0"><${Minigrafica} serie=${r.serie} alto=${56} color=${colorPing(r.media)}/></div>
      <div class="rej4 mono" style="gap:8px;font-size:12px">
        <div><div class="mut">${t("MEDIA", "AVG")}</div><b>${r.media >= 0 ? r.media : "—"}</b></div>
        <div><div class="mut">${t("MÁXIMO", "MAX")}</div><b>${r.maximo >= 0 ? r.maximo : "—"}</b></div>
        <div><div class="mut">JITTER</div><b>${r.jitter >= 0 ? r.jitter : "—"}</b></div>
        <div><div class="mut">${t("PICOS", "SPIKES")}</div><b>${r.picos}</b></div>
      </div>
    </${Tarjeta}>`)}</div>
  </div>`;
}

// ---- Actividad completa ---------------------------------------------------------------
function ActividadSala({ s }) {
  const [filtro, setFiltro] = useState("");
  const lista = s.actividad.filter((e) => !filtro || e.texto.toLowerCase().includes(filtro.toLowerCase())).slice().reverse();
  return html`<${Tarjeta} estilo="min-height:100%" interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
    <div style="display:flex;gap:12px;align-items:center">
      <div class="lab acc" style="flex:none">${t("REGISTRO DE LA SALA", "ROOM LOG")}</div>
      <input class="campo" style="max-width:320px;margin-left:auto" placeholder=${t("Filtrar…", "Filter…")} value=${filtro} onInput=${(e) => setFiltro(e.currentTarget.value)}/>
      <${Boton} tipo="mini suave" al=${() => accion("ui.copiar", { texto: s.actividad.map((e) => e.texto).join("\n").slice(-3900) }, { ok: t("Copiado.", "Copied.") })}><${Icono} n="copiar" t=${14}/></${Boton}>
    </div>
    <div style="display:flex;flex-direction:column;gap:2px;user-select:text">
      ${lista.length === 0 ? html`<div class="ayuda">${t("Sin registros.", "No entries.")}</div>` : null}
      ${lista.map((e) => html`<div key=${e.id} style="display:flex;gap:10px;padding:7px 0;border-bottom:1px solid var(--tenue);font-size:13px">
        <span class="punto" style=${`margin-top:6px;background:${colorLinea(e.texto)}`}></span>
        <span style="flex:1;word-break:break-word">${e.texto}</span>
        ${e.en ? html`<span class="mono mut" style="font-size:12px;flex:none">${haceCuanto(e.en)}</span>` : null}
      </div>`)}
    </div>
  </${Tarjeta}>`;
}
