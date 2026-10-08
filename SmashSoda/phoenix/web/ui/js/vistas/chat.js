// Panel lateral de chat y registro, con autocompletado de comandos (!).
import { html, useState, useEffect, useRef, useMemo } from "../lib.js";
import { t } from "../i18n.js";
import { accion, cambiar } from "../tienda.js";
import { Icono, cx } from "../ui.js";
import { COMANDOS } from "../comandos.js";
import { colorLinea } from "./sala.js";

export function PanelChat({ s }) {
  const [vista, setVista] = useState("chat");
  const [texto, setTexto] = useState("");
  const [indice, setIndice] = useState(0);
  const lineasRef = useRef(null);
  const entradaRef = useRef(null);
  const lineas = vista === "chat" ? s.chat : s.actividad;

  useEffect(() => {
    const el = lineasRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [lineas.length, vista, s.chatAbierto]);
  useEffect(() => { if (s.chatAbierto) setTimeout(() => entradaRef.current?.focus(), 60); }, [s.chatAbierto]);

  const sugerencias = useMemo(() => {
    if (!texto.startsWith("!") || texto.includes(" ")) return [];
    const q = texto.toLowerCase();
    return COMANDOS.filter((c) => c.c.startsWith(q)).slice(0, 8);
  }, [texto]);

  const enviar = async () => {
    const x = texto.trim();
    if (!x) return;
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

  return html`<aside class="chat" aria-label="Chat" aria-hidden=${!s.chatAbierto}>
    <div class="cab">
      <div class="seg">
        <button class=${vista === "chat" ? "on" : ""} onClick=${() => setVista("chat")}>CHAT</button>
        <button class=${vista === "registro" ? "on" : ""} onClick=${() => setVista("registro")}>${t("REGISTRO", "LOG")}</button>
      </div>
      <button class="icono-btn" onClick=${() => cambiar({ chatAbierto: false })} aria-label=${t("Cerrar", "Close")}><${Icono} n="cerrar" t=${16}/></button>
    </div>
    <div class="lineas" ref=${lineasRef}>
      ${lineas.length === 0 ? html`<div class="mut" style="font-size:13px">${vista === "chat" ? t("Todavía no hay mensajes.", "No messages yet.") : t("Sin registros.", "No entries.")}</div>` : null}
      ${lineas.map((l) => vista === "chat" ? html`<${LineaChat} key=${l.id} texto=${l.texto}/>`
        : html`<div key=${l.id} class="linea sistema" style=${`border-left:2px solid ${colorLinea(l.texto)};padding-left:8px`}>${l.texto}</div>`)}
    </div>
    ${vista === "chat" ? html`<div class="entrada">
      ${sugerencias.length ? html`<div class="sugerencias">${sugerencias.map((c, i) => html`
        <div class=${cx("sug", i === indice % sugerencias.length && "on")} onMouseDown=${(e) => { e.preventDefault(); setTexto(c.c + " "); entradaRef.current?.focus(); }}>
          <span class="c">${c.c}</span><span class="mut" style="text-align:right">${t(c.es, c.en)}</span></div>`)}</div>` : null}
      <div style="display:flex;gap:8px">
        <input ref=${entradaRef} class="campo" maxLength="1000" placeholder=${t("Escribe… (! para comandos)", "Type… (! for commands)")}
          value=${texto} onInput=${(e) => { setTexto(e.currentTarget.value); setIndice(0); }} onKeyDown=${tecla}/>
        <button class="btn lleno redondo" style="height:42px;width:42px;border-radius:21px" onClick=${enviar} aria-label=${t("Enviar", "Send")}><${Icono} n="enviar" t=${17}/></button>
      </div>
    </div>` : null}
  </aside>`;
}

function LineaChat({ texto }) {
  const i = texto.indexOf(": ");
  if (i > 0 && i < 48) {
    return html`<div class="linea"><span class="quien">${texto.slice(0, i)}</span>: ${texto.slice(i + 2)}</div>`;
  }
  return html`<div class="linea sistema">${texto}</div>`;
}
