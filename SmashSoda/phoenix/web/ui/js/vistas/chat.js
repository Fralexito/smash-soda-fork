// Panel lateral de chat: GENERAL (el mismo de la web) y SALA (solo con sala abierta), más el registro.
import { html, useState, useEffect, useRef, useMemo } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar, elegirPestanaChat, noLeidosGlobal } from "../tienda.js";
import { Icono, cx } from "../ui.js";
import { COMANDOS } from "../comandos.js";
import { colorLinea } from "./sala.js";

const ROLES = { admin: "ADMIN", moderador: "MOD", ayudante: "AYUDA", arbitro: "ÁRBITRO" };
const MAX_GLOBAL = 300;

export function PanelChat({ s }) {
  const m = s.motor;
  const enSala = !!m?.sala?.abierta;
  // Sala cerrada: GENERAL siempre. Sala abierta: SALA por defecto (el cambio automático lo hace la tienda).
  const vista = s.chatPestana === "registro" ? "registro" : (enSala && s.chatPestana === "sala" ? "sala" : "general");
  const [texto, setTexto] = useState("");
  const [indice, setIndice] = useState(0);
  const [hasta, setHasta] = useState(0);        // hora (ms) hasta la que se espera para escribir otra vez
  const [, reloj] = useState(0);
  const lineasRef = useRef(null);
  const entradaRef = useRef(null);
  const g = s.chatGlobal;
  const meta = m?.chatGlobal || {};
  const vinculada = m?.web?.estado === "conectado" || m?.web?.estado === "sin_conexion" || m?.web?.estado === "pausado";
  const lineas = vista === "sala" ? s.chat : vista === "registro" ? s.actividad : g.mensajes;
  const noGlobal = noLeidosGlobal(s);

  useEffect(() => {
    const el = lineasRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [lineas.length, vista, s.chatAbierto]);
  useEffect(() => { if (s.chatAbierto) setTimeout(() => entradaRef.current?.focus(), 60); }, [s.chatAbierto, vista]);
  // El motor solo pregunta rápido por el chat general si se está mirando
  useEffect(() => {
    accion("chatGlobal.abierto", { abierto: !!(s.chatAbierto && vista === "general") }, { silencioso: true });
  }, [s.chatAbierto, vista]);
  // Cuenta atrás del anti-spam
  useEffect(() => {
    if (hasta <= Date.now()) return;
    const id = setInterval(() => { reloj((x) => x + 1); if (Date.now() >= hasta) clearInterval(id); }, 250);
    return () => clearInterval(id);
  }, [hasta]);
  const faltan = Math.max(0, Math.ceil((hasta - Date.now()) / 1000));

  const sugerencias = useMemo(() => {
    if (vista !== "sala" || !texto.startsWith("!") || texto.includes(" ")) return [];
    const q = texto.toLowerCase();
    return COMANDOS.filter((c) => c.c.startsWith(q)).slice(0, 8);
  }, [texto, vista]);

  const enviar = async () => {
    const x = texto.trim();
    if (!x) return;
    if (vista === "general") {
      if (faltan > 0 || meta.pausado || !vinculada) return;
      const r = await accion("chatGlobal.enviar", { texto: x });
      if (r) { setTexto(""); setHasta(Date.now() + (meta.esperaSeg || 0) * 1000); }
      return;
    }
    if (await accion("chat.enviar", { texto: x })) setTexto("");
  };
  const tecla = (e) => {
    if (sugerencias.length) {
      if (e.key === "ArrowDown") { e.preventDefault(); setIndice((i) => (i + 1) % sugerencias.length); return; }
      if (e.key === "ArrowUp") { e.preventDefault(); setIndice((i) => (i - 1 + sugerencias.length) % sugerencias.length); return; }
      if (e.key === "Tab") { e.preventDefault(); setTexto(sugerencias[indice % sugerencias.length].c + " "); setIndice(0); return; }
    }
    if (e.key === "Enter" && !e.shiftKey) { e.preventDefault(); enviar(); }
  };

  const bloqueado = vista === "general" && (meta.pausado || !vinculada);
  let aviso = null;
  if (vista === "general") {
    if (!vinculada) aviso = t("Vincula esta PC con la web para usar el chat general.", "Link this PC to the web to use the general chat.");
    else if (meta.pausado) aviso = t("El chat general está en pausa por el staff.", "The general chat is paused by staff.");
    else if (meta.error) aviso = meta.error;
  }

  return html`<aside class="chat" aria-label="Chat" aria-hidden=${!s.chatAbierto}>
    <div class="cab">
      <div class="seg">
        <button class=${vista === "general" ? "on" : ""} onClick=${() => elegirPestanaChat("general")}>${t("GENERAL", "GENERAL")}${noGlobal > 0 && vista !== "general" ? html`<span class="cuenta">${noGlobal > 99 ? "99+" : noGlobal}</span>` : null}</button>
        ${enSala ? html`<button class=${vista === "sala" ? "on" : ""} onClick=${() => elegirPestanaChat("sala")}>${t("SALA", "ROOM")}${s.noLeidos > 0 && vista !== "sala" ? html`<span class="cuenta">${s.noLeidos > 99 ? "99+" : s.noLeidos}</span>` : null}</button>` : null}
        <button class=${vista === "registro" ? "on" : ""} onClick=${() => elegirPestanaChat("registro")}>${t("REGISTRO", "LOG")}</button>
      </div>
      <button class="icono-btn" onClick=${() => cambiar({ chatAbierto: false })} aria-label=${t("Cerrar", "Close")}><${Icono} n="cerrar" t=${16}/></button>
    </div>
    ${vista === "general" ? html`<div class="sub-chat">${t("Es el mismo chat de la página web.", "Same chat as the website.")}</div>` : null}
    ${aviso ? html`<div class="aviso-chat">${aviso}</div>` : null}
    <div class="lineas" ref=${lineasRef}>
      ${lineas.length === 0 ? html`<div class="mut" style="font-size:13px">${
        vista === "registro" ? t("Sin registros.", "No entries.")
        : vista === "general" ? (vinculada && !meta.cargado ? t("Cargando el chat…", "Loading chat…") : t("Todavía no hay mensajes.", "No messages yet."))
        : t("Todavía no hay mensajes.", "No messages yet.")}</div>` : null}
      ${vista === "general" ? g.mensajes.map((x) => html`<${LineaGlobal} key=${x.id} m=${x}/>`)
        : vista === "sala" ? s.chat.map((l) => html`<${LineaChat} key=${l.id} texto=${l.texto}/>`)
        : s.actividad.map((l) => html`<div key=${l.id} class="linea sistema" style=${`border-left:2px solid ${colorLinea(l.texto)};padding-left:8px`}>${l.texto}</div>`)}
    </div>
    ${vista !== "registro" ? html`<div class="entrada">
      ${sugerencias.length ? html`<div class="sugerencias">${sugerencias.map((c, i) => html`
        <div class=${cx("sug", i === indice % sugerencias.length && "on")} onMouseDown=${(e) => { e.preventDefault(); setTexto(c.c + " "); entradaRef.current?.focus(); }}>
          <span class="c">${c.c}</span><span class="mut" style="text-align:right">${t(c.es, c.en)}</span></div>`)}</div>` : null}
      <div style="display:flex;gap:8px">
        <input ref=${entradaRef} class="campo" maxLength=${vista === "general" ? MAX_GLOBAL : 1000} disabled=${bloqueado}
          placeholder=${vista === "general" ? t("Escribe al chat general…", "Write to the general chat…") : t("Escribe… (! para comandos)", "Type… (! for commands)")}
          value=${texto} onInput=${(e) => { setTexto(e.currentTarget.value); setIndice(0); }} onKeyDown=${tecla}/>
        <button class="btn lleno redondo" style="height:42px;width:42px;border-radius:21px" disabled=${bloqueado || (vista === "general" && faltan > 0)}
          onClick=${enviar} aria-label=${t("Enviar", "Send")} title=${faltan > 0 ? t("Espera ", "Wait ") + faltan + " s" : ""}>
          ${vista === "general" && faltan > 0 ? html`<span style="font:600 13px/1 var(--f-mono)">${faltan}</span>` : html`<${Icono} n="enviar" t=${17}/>`}</button>
      </div>
      ${vista === "general" && texto.length > 240 ? html`<div class="mut" style="font-size:12px;margin-top:6px;text-align:right">${MAX_GLOBAL - texto.length} ${t("caracteres restantes", "characters left")}</div>` : null}
    </div>` : null}
  </aside>`;
}

function LineaGlobal({ m }) {
  const rol = ROLES[m.rol];
  return html`<div class=${cx("linea", "global", m.propio && "propia")}>
    <span class="hora">${m.hora}</span>
    <span class="quien">${m.nombre}</span>${rol ? html` <span class=${cx("chip", m.rol === "admin" ? "vip" : "mod")} style="height:18px;padding:0 6px;font-size:10px">${rol}</span>` : null}: ${m.texto}
  </div>`;
}

function LineaChat({ texto }) {
  const i = texto.indexOf(": ");
  if (i > 0 && i < 48) {
    return html`<div class="linea"><span class="quien">${texto.slice(0, i)}</span>: ${texto.slice(i + 2)}</div>`;
  }
  return html`<div class="linea sistema">${texto}</div>`;
}
