// =============================================================================
//  Phoenix Link · Sistema de Predicciones y Votaciones de Sala en Vivo (Esports)
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Permite a los espectadores en «Mirando» votar por el ganador antes del partido.
//  Incluye barra dinámica de porcentajes estilo Twitch/Kick, puntos de sala
//  simbólicos (sin apuestas reales) y resolución automática al terminar el partido.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton, Icono, Tarjeta } from "../ui.js";

const CLAVE_STORAGE = "phoenix.prediccion";
const CLAVE_PUNTOS = "phoenix.puntos_sala";

/** Obtiene el estado actual de la predicción activa */
export function obtenerPrediccion() {
  try {
    const raw = localStorage.getItem(CLAVE_STORAGE);
    if (!raw) return { activa: false, pregunta: "", opciones: [], cerrada: false, resuelta: false, ganador: null };
    return JSON.parse(raw);
  } catch {
    return { activa: false, pregunta: "", opciones: [], cerrada: false, resuelta: false, ganador: null };
  }
}

/** Guarda y emite cambios a toda la UI */
export function guardarPrediccion(datos) {
  try {
    localStorage.setItem(CLAVE_STORAGE, JSON.stringify(datos));
    window.dispatchEvent(new CustomEvent("phoenix:prediccion", { detail: datos }));
  } catch (e) {
    console.error("[predicciones] Error guardando estado:", e);
  }
}

/** Obtiene la tabla acumulada de puntos de reputación de la sala */
export function obtenerPuntosSala() {
  try {
    const raw = localStorage.getItem(CLAVE_PUNTOS);
    return raw ? JSON.parse(raw) : {};
  } catch {
    return {};
  }
}

/** Guarda la tabla de puntos */
export function guardarPuntosSala(puntos) {
  try {
    localStorage.setItem(CLAVE_PUNTOS, JSON.stringify(puntos));
    window.dispatchEvent(new CustomEvent("phoenix:puntos_sala", { detail: puntos }));
  } catch (e) {
    console.error("[predicciones] Error guardando puntos:", e);
  }
}

/** Abre una nueva predicción para el partido actual */
export async function iniciarPrediccion({ equipoA = "Equipo A", equipoB = "Equipo B", m }) {
  const nueva = {
    activa: true,
    creadaEn: Date.now(),
    pregunta: `${t("¿Quién gana?", "Who wins?")} ${equipoA} vs ${equipoB}`,
    cerrada: false,
    resuelta: false,
    ganador: null,
    opciones: [
      { id: "a", texto: equipoA, votos: 0, votantes: [] },
      { id: "x", texto: t("Empate", "Draw"), votos: 0, votantes: [] },
      { id: "b", texto: equipoB, votos: 0, votantes: [] },
    ],
  };
  guardarPrediccion(nueva);

  if (m?.sala?.abierta) {
    await accion("chat.enviar", {
      texto: `🎯 ¡PREDICCIÓN ABIERTA! ¿Quién ganará? [1] ${equipoA} | [X] Empate | [2] ${equipoB}. ¡Vota en Phoenix Link!`,
    }, { silencioso: true });
  }
  avisar(t("🎯 Predicción de partido abierta para los espectadores", "🎯 Match prediction open for spectators"), "ok");
}

/** Cierra las votaciones cuando el partido arranca */
export async function cerrarVotacion({ m } = {}) {
  const p = obtenerPrediccion();
  if (!p.activa || p.cerrada) return;
  p.cerrada = true;
  guardarPrediccion(p);

  const total = p.opciones.reduce((acc, o) => acc + o.votos, 0);
  if (m?.sala?.abierta) {
    await accion("chat.enviar", {
      texto: `🔒 Votaciones cerradas (${total} votos). ¡Que ruede el balón!`,
    }, { silencioso: true });
  }
}

/** Emite un voto por parte del usuario local o espectador */
export function emitirVoto({ opcionId, usuario = "Tú", idUsuario = "local" }) {
  const p = obtenerPrediccion();
  if (!p.activa || p.cerrada || p.resuelta) {
    avisar(t("La predicción está cerrada", "Prediction is closed"), "warn");
    return false;
  }

  // Quitar voto previo si ya había votado
  for (const op of p.opciones) {
    const idx = op.votantes.findIndex((v) => v.idUsuario === idUsuario);
    if (idx !== -1) {
      op.votantes.splice(idx, 1);
      op.votos = Math.max(0, op.votos - 1);
    }
  }

  // Asignar nuevo voto
  const destino = p.opciones.find((o) => o.id === opcionId);
  if (destino) {
    destino.votantes.push({ idUsuario, usuario, hora: Date.now() });
    destino.votos += 1;
    guardarPrediccion(p);
    avisar(t(`Voto registrado por: ${destino.texto}`, `Vote cast for: ${destino.texto}`), "ok");
    return true;
  }
  return false;
}

/** Resuelve la predicción con el resultado real y premia a los acertantes */
export async function resolverPrediccion({ resultado, m }) {
  const p = obtenerPrediccion();
  if (!p.activa || p.resuelta) return;

  p.resuelta = true;
  p.ganador = resultado; // "a", "x" o "b"
  guardarPrediccion(p);

  const opcionGanadora = p.opciones.find((o) => o.id === resultado);
  const acertantes = opcionGanadora ? opcionGanadora.votantes : [];

  // Premiar con +100 puntos de sala
  const tablaPuntos = obtenerPuntosSala();
  for (const v of acertantes) {
    tablaPuntos[v.usuario] = (tablaPuntos[v.usuario] || 0) + 100;
  }
  guardarPuntosSala(tablaPuntos);

  const textoGanador = opcionGanadora ? opcionGanadora.texto : resultado;
  const listaNombres = acertantes.map((v) => v.usuario).slice(0, 5).join(", ");
  const extra = acertantes.length > 5 ? ` (+${acertantes.length - 5})` : "";

  if (m?.sala?.abierta) {
    const mensaje = acertantes.length > 0
      ? `🏆 ¡PREDICCIÓN RESUELTA! Ganador: ${textoGanador}. Acertaron (+100 pts): ${listaNombres}${extra}!`
      : `🏁 Predicción resuelta: Ganó ${textoGanador}. Nadie acertó esta vez.`;
    await accion("chat.enviar", { texto: mensaje }, { silencioso: true });
  }

  avisar(t(`🎯 Predicción finalizada: Ganó ${textoGanador}`, `🎯 Prediction ended: Won ${textoGanador}`), "ok");
}

/** Resetea o apaga la predicción */
export function reiniciarPrediccion() {
  guardarPrediccion({ activa: false, pregunta: "", opciones: [], cerrada: false, resuelta: false, ganador: null });
  avisar(t("Predicción reiniciada", "Prediction reset"), "info");
}

// =============================================================================
//  Componentes Visuales de Predicciones
// =============================================================================

/** Widget de barra de predicción interactiva y animada estilo Esports */
export function WidgetPredicciones({ m, esHost }) {
  const [pred, setPred] = useState(obtenerPrediccion);
  const [modalAbierto, setModalAbierto] = useState(false);

  useEffect(() => {
    const alCambiar = (e) => setPred(e.detail);
    window.addEventListener("phoenix:prediccion", alCambiar);
    return () => window.removeEventListener("phoenix:prediccion", alCambiar);
  }, []);

  if (!pred.activa) {
    if (!esHost) return null;
    return html`
      <div style="display:flex;align-items:center;gap:8px;padding:6px 12px;background:rgba(255,255,255,0.03);border-radius:10px;border:1px dashed rgba(255,255,255,0.15)">
        <span style="font-size:12px;color:var(--mut)">🎯 ${t("¿Lanzar predicción de sala?", "Launch room prediction?")}</span>
        <${Boton} tipo="mini acc" al=${() => setModalAbierto(true)}>+ ${t("CREAR PREDICCIÓN", "NEW PREDICTION")}</${Boton}>
        ${modalAbierto ? html`<${ModalCrearPrediccion} m=${m} alCerrar=${() => setModalAbierto(false)}/>` : null}
      </div>`;
  }

  const totalVotos = pred.opciones.reduce((a, o) => a + o.votos, 0);
  const porcA = totalVotos > 0 ? Math.round((pred.opciones[0].votos / totalVotos) * 100) : 33;
  const porcX = totalVotos > 0 ? Math.round((pred.opciones[1].votos / totalVotos) * 100) : 34;
  const porcB = totalVotos > 0 ? 100 - porcA - porcX : 33;

  return html`
    <div class="tarjeta-prediccion" style="display:flex;flex-direction:column;gap:8px;padding:12px 14px;background:linear-gradient(135deg,rgba(0,240,255,0.08),rgba(179,0,255,0.08));border:1px solid rgba(0,240,255,0.25);border-radius:12px;position:relative">
      <div style="display:flex;justify-content:space-between;align-items:center">
        <div style="display:flex;align-items:center;gap:6px">
          <span style="font-size:14px">🎯</span>
          <span class="disp" style="font-weight:700;font-size:13px;letter-spacing:.05em">${pred.pregunta}</span>
          ${pred.cerrada
            ? html`<span class="badge" style="font-size:10px;background:rgba(255,70,70,0.2);color:#ff6b6b;padding:2px 6px;border-radius:4px">🔒 ${t("CERRADA", "CLOSED")}</span>`
            : html`<span class="badge" style="font-size:10px;background:rgba(0,255,136,0.2);color:#00ff88;padding:2px 6px;border-radius:4px">⚡ ${t("VOTANDO", "VOTING")}</span>`}
        </div>
        <div style="display:flex;align-items:center;gap:6px">
          <span class="mono mut" style="font-size:11px">${totalVotos} ${t(totalVotos === 1 ? "voto" : "votos", totalVotos === 1 ? "vote" : "votes")}</span>
          ${esHost ? html`<${Boton} tipo="mini peligro" al=${reiniciarPrediccion} title=${t("Cerrar y descartar", "Close and discard")}>✕</${Boton}>` : null}
        </div>
      </div>

      <!-- Barra dinámica de porcentajes estilo Twitch Predictions -->
      <div style="height:10px;width:100%;background:rgba(0,0,0,0.4);border-radius:6px;overflow:hidden;display:flex;border:1px solid rgba(255,255,255,0.1)">
        <div style=${`width:${porcA}%;background:var(--acc);transition:width .4s ease`} title=${`${pred.opciones[0].texto}: ${porcA}%`}></div>
        <div style=${`width:${porcX}%;background:#888;transition:width .4s ease`} title=${`${pred.opciones[1].texto}: ${porcX}%`}></div>
        <div style=${`width:${porcB}%;background:#b300ff;transition:width .4s ease`} title=${`${pred.opciones[2].texto}: ${porcB}%`}></div>
      </div>

      <!-- Botones de voto o etiquetas con porcentajes -->
      <div style="display:grid;grid-template-columns:1fr auto 1fr;gap:6px;align-items:center">
        <button
          class="btn-voto"
          disabled=${pred.cerrada || pred.resuelta}
          onClick=${() => emitirVoto({ opcionId: "a", usuario: m?.sala?.anfitrion || "Tú" })}
          style="padding:6px 10px;border-radius:8px;background:rgba(0,240,255,0.15);border:1px solid rgba(0,240,255,0.4);color:#fff;cursor:pointer;display:flex;justify-content:space-between;align-items:center;font-size:12px">
          <span style="font-weight:600">${pred.opciones[0].texto}</span>
          <span class="mono" style="color:var(--acc);font-weight:700">${porcA}%</span>
        </button>

        <button
          class="btn-voto"
          disabled=${pred.cerrada || pred.resuelta}
          onClick=${() => emitirVoto({ opcionId: "x", usuario: m?.sala?.anfitrion || "Tú" })}
          style="padding:6px 8px;border-radius:8px;background:rgba(255,255,255,0.08);border:1px solid rgba(255,255,255,0.2);color:#ccc;cursor:pointer;display:flex;gap:6px;align-items:center;font-size:12px">
          <span>X</span>
          <span class="mono" style="font-weight:700">${porcX}%</span>
        </button>

        <button
          class="btn-voto"
          disabled=${pred.cerrada || pred.resuelta}
          onClick=${() => emitirVoto({ opcionId: "b", usuario: m?.sala?.anfitrion || "Tú" })}
          style="padding:6px 10px;border-radius:8px;background:rgba(179,0,255,0.15);border:1px solid rgba(179,0,255,0.4);color:#fff;cursor:pointer;display:flex;justify-content:space-between;align-items:center;font-size:12px">
          <span class="mono" style="color:#d880ff;font-weight:700">${porcB}%</span>
          <span style="font-weight:600">${pred.opciones[2].texto}</span>
        </button>
      </div>

      ${esHost && !pred.cerrada ? html`
        <div style="display:flex;justify-content:flex-end;margin-top:2px">
          <${Boton} tipo="mini suave" al=${() => cerrarVotacion({ m })}>🔒 ${t("CERRAR VOTACIONES", "CLOSE VOTES")}</${Boton}>
        </div>` : null}
    </div>`;
}

/** Modal para configurar y lanzar una nueva predicción */
function ModalCrearPrediccion({ m, alCerrar }) {
  const [eqA, setEqA] = useState(m?.partido?.equipoA || "Equipo A");
  const [eqB, setEqB] = useState(m?.partido?.equipoB || "Equipo B");

  const lanzar = async () => {
    await iniciarPrediccion({ equipoA: eqA, equipoB: eqB, m });
    alCerrar();
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:440px">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:14px">
          <div style="display:flex;align-items:center;gap:10px">
            <span style="font-size:22px">🎯</span>
            <h2 style="margin:0;font-size:18px">${t("Nueva Predicción de Sala", "New Room Prediction")}</h2>
          </div>
          <div class="ayuda" style="font-size:13px">
            ${t("Los espectadores en «Mirando» podrán votar quién se lleva la victoria. Al terminar el partido se resuelven los aciertos automáticamente.", "Spectators can vote who wins. When the match ends, correct predictions are resolved automatically.")}
          </div>

          <div style="display:flex;flex-direction:column;gap:8px">
            <label class="lab">${t("Nombre del Equipo A (Local)", "Team A Name (Home)")}</label>
            <input class="campo" value=${eqA} onInput=${(e) => setEqA(e.target.value)} placeholder="Equipo A"/>
          </div>

          <div style="display:flex;flex-direction:column;gap:8px">
            <label class="lab">${t("Nombre del Equipo B (Visitante)", "Team B Name (Away)")}</label>
            <input class="campo" value=${eqB} onInput=${(e) => setEqB(e.target.value)} placeholder="Equipo B"/>
          </div>

          <div class="botones" style="margin-top:10px;display:flex;justify-content:flex-end;gap:8px">
            <button class="btn suave" onClick=${alCerrar}>${t("Cancelar", "Cancel")}</button>
            <button class="btn acc" onClick=${lanzar}>🚀 ${t("Lanzar Predicción", "Launch Prediction")}</button>
          </div>
        </div></div>
      </div>
    </div>`;
}
