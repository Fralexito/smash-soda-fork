// Barra de «última hora»: una sola línea discreta en la cabecera. Rota las noticias vigentes;
// al pulsarla se abre el texto completo. Solo se «nota» (punto que respira) si es importante o urgente.
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { cx } from "../ui.js";

const CLAVE = "phx.noticias.cerradas";
function leerCerradas() {
  try { const v = JSON.parse(localStorage.getItem(CLAVE) || "[]"); return Array.isArray(v) ? v : []; } catch { return []; }
}
function guardarCerradas(ids) {
  try { localStorage.setItem(CLAVE, JSON.stringify(ids.slice(-60))); } catch { /* sin almacenamiento: se vuelve a ver, no pasa nada */ }
}
const ETIQUETA = { urgente: ["URGENTE", "URGENT"], importante: ["IMPORTANTE", "IMPORTANT"], info: ["ÚLTIMA HORA", "LATEST"] };

export function UltimaHora({ s }) {
  const reales = s.motor?.noticias?.lista || [];
  // Movimientos reales de la web: lo último que se escribió en el chat general (se ve aunque no haya noticias)
  const movs = (s.chatGlobal?.mensajes || []).filter((m) => !m.propio && m.texto).slice(-2).reverse()
    .map((m) => ({ id: "chat-" + m.id, nivel: "info", mov: true, texto: `${m.nombre || "—"}: ${m.texto}`, hora: "" }));
  const lista = [...reales, ...movs];
  const [cerradas, setCerradas] = useState(leerCerradas);
  const [indice, setIndice] = useState(0);
  const [abierta, setAbierta] = useState(false);
  const visibles = lista.filter((n) => !cerradas.includes(n.id));
  // las más graves primero
  const orden = { urgente: 0, importante: 1, info: 2 };
  visibles.sort((a, b) => (a.mov ? 1 : 0) - (b.mov ? 1 : 0) || (orden[a.nivel] ?? 2) - (orden[b.nivel] ?? 2) || (typeof a.id === "number" && typeof b.id === "number" ? b.id - a.id : 0));

  useEffect(() => {
    if (visibles.length < 2 || abierta) return;
    const id = setInterval(() => setIndice((i) => i + 1), 9000);
    return () => clearInterval(id);
  }, [visibles.length, abierta]);
  useEffect(() => {
    if (!abierta) return;
    const fuera = (e) => { if (!e.target.closest?.(".ultima, .ultima-detalle")) setAbierta(false); };
    const tecla = (e) => { if (e.key === "Escape") setAbierta(false); };
    window.addEventListener("mousedown", fuera);
    window.addEventListener("keydown", tecla);
    return () => { window.removeEventListener("mousedown", fuera); window.removeEventListener("keydown", tecla); };
  }, [abierta]);

  if (visibles.length === 0) return null;
  const n = visibles[indice % visibles.length];
  const nivel = ORDEN_OK(n.nivel);
  const et = n.mov ? ["CHAT GENERAL", "GENERAL CHAT"] : ETIQUETA[nivel];
  const cerrar = () => {
    const nuevas = [...cerradas, n.id];
    setCerradas(nuevas); guardarCerradas(nuevas); setAbierta(false); setIndice(0);
  };

  return html`<div class="ultima-caja">
    <button key=${n.id} class=${cx("ultima", "nv-" + nivel)} onClick=${() => setAbierta(!abierta)} title=${n.texto} aria-expanded=${abierta}>
      <span class="u-punto"></span>
      <span class="u-et">${t(et[0], et[1])}</span>
      <span class="u-texto">${n.texto}</span>
      ${visibles.length > 1 ? html`<span class="u-n">${(indice % visibles.length) + 1}/${visibles.length}</span>` : null}
    </button>
    ${abierta ? html`<div class=${cx("ultima-detalle", "nv-" + nivel)} role="dialog">
      <div class="u-cab"><span class="u-et">${t(et[0], et[1])}</span>${n.hora ? html`<span class="mut" style="font-size:12px">${n.hora}</span>` : null}</div>
      <div class="u-cuerpo">${n.texto}</div>
      <div style="display:flex;justify-content:flex-end;gap:8px;margin-top:12px">
        ${visibles.length > 1 ? html`<button class="btn suave" onClick=${() => setIndice((i) => i + 1)}>${t("Siguiente", "Next")}</button>` : null}
        <button class="btn" onClick=${cerrar}>${t("Entendido", "Got it")}</button>
      </div>
    </div>` : null}
  </div>`;
}
function ORDEN_OK(v) { return v === "urgente" || v === "importante" ? v : "info"; }
