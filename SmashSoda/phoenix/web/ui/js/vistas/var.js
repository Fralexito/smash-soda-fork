// =============================================================================
//  Phoenix Link · El VAR de la Sala (Votación Dramática de Jugadas Polémicas)
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Permite al anfitrión o moderadores pausar momentáneamente y abrir una votación
//  relámpago de 15 segundos para que la sala decida jugadas dudosas en PES.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton } from "../ui.js";
import { sonarSirenaVAR, narrar } from "./locutor.js";
import { notificarVARDiscord } from "../servicios/discord.js";

const CLAVE_VAR = "phoenix.var_estado";

export function obtenerEstadoVAR() {
  try {
    const raw = localStorage.getItem(CLAVE_VAR);
    return raw ? JSON.parse(raw) : { activo: false, segundos: 15, votos: { penal: 0, nada: 0, roja: 0 }, votantes: [] };
  } catch {
    return { activo: false, segundos: 15, votos: { penal: 0, nada: 0, roja: 0 }, votantes: [] };
  }
}

export function guardarEstadoVAR(st) {
  try {
    localStorage.setItem(CLAVE_VAR, JSON.stringify(st));
    window.dispatchEvent(new CustomEvent("phoenix:var_evento", { detail: st }));
  } catch (e) {
    console.error("[var] Error guardando estado:", e);
  }
}

/** Inicia una revisión de VAR en vivo */
export async function lanzarRevisionVAR({ m } = {}) {
  sonarSirenaVAR();
  narrar("¡Atención! ¡El árbitro detiene el juego y pide revisión en el monitor del VAR!", { prioridad: true });

  const nuevo = {
    activo: true,
    iniciadoEn: Date.now(),
    segundos: 15,
    cerrado: false,
    votos: { penal: 0, nada: 0, roja: 0 },
    votantes: [],
  };
  guardarEstadoVAR(nuevo);

  if (m?.sala?.abierta) {
    await accion("chat.enviar", {
      texto: "🚨 ¡REVISIÓN DE VAR EN PROGRESO! Tienes 15 segundos para votar en Phoenix Link: ¿Penal, Piscinazo o Tarjeta Roja?",
    }, { silencioso: true });
  }
}

/** Emite un voto en el VAR */
export function votarVAR({ opcion, usuario = "Tú" }) {
  const st = obtenerEstadoVAR();
  if (!st.activo || st.cerrado) return;

  const idx = st.votantes.findIndex((v) => v.usuario === usuario);
  if (idx !== -1) {
    st.votos[st.votantes[idx].opcion] = Math.max(0, st.votos[st.votantes[idx].opcion] - 1);
    st.votantes.splice(idx, 1);
  }

  st.votantes.push({ usuario, opcion });
  st.votos[opcion] = (st.votos[opcion] || 0) + 1;
  guardarEstadoVAR(st);
  avisar(t(`Voto VAR registrado: ${opcion.toUpperCase()}`, `VAR vote recorded: ${opcion.toUpperCase()}`), "ok");
}

export function cerrarVAR({ m } = {}) {
  const st = obtenerEstadoVAR();
  st.activo = false;
  guardarEstadoVAR(st);
}

// =============================================================================
//  Componente Visual del Modal VAR
// =============================================================================

export function ModalVAR({ m, alCerrar }) {
  const [st, setSt] = useState(obtenerEstadoVAR);
  const [tiempo, setTiempo] = useState(15);

  useEffect(() => {
    const alEvento = (e) => setSt(e.detail);
    window.addEventListener("phoenix:var_evento", alEvento);

    const timer = setInterval(() => {
      setTiempo((tRest) => {
        if (tRest <= 1) {
          clearInterval(timer);
          return 0;
        }
        return tRest - 1;
      });
    }, 1000);

    return () => {
      window.removeEventListener("phoenix:var_evento", alEvento);
      clearInterval(timer);
    };
  }, []);

  const total = (st.votos.penal || 0) + (st.votos.nada || 0) + (st.votos.roja || 0);
  const pPenal = total > 0 ? Math.round((st.votos.penal / total) * 100) : 33;
  const pNada = total > 0 ? Math.round((st.votos.nada / total) * 100) : 34;
  const pRoja = total > 0 ? 100 - pPenal - pNada : 33;

  const finalizado = tiempo === 0;
  let veredicto = "REVISANDO JUGADA...";
  if (finalizado) {
    if (pPenal >= pNada && pPenal >= pRoja) veredicto = "¡PENAL CONFIRMADO! ⚽";
    else if (pNada >= pPenal && pNada >= pRoja) veredicto = "¡NO HAY NADA / PISCINAZO! 🏊";
    else veredicto = "¡TARJETA ROJA DIRECTA! 🟥";
  }

  return html`
    <div class="velo" style="background:rgba(0,0,0,0.85);backdrop-filter:blur(10px)">
      <div class="dialogo" style="max-width:520px;border:2px solid #ff0055;box-shadow:0 0 35px rgba(255,0,85,0.4)">
        <div class="cut"><div class="in" style="padding:28px 32px;display:flex;flex-direction:column;gap:16px;text-align:center">
          
          <!-- Cabecera de Alerta VAR -->
          <div style="display:flex;flex-direction:column;align-items:center;gap:6px">
            <span style="font-size:36px;animation:pulse 1s infinite">🚨🖥️</span>
            <div class="disp" style="font-weight:900;font-size:22px;letter-spacing:.2em;color:#ff0055">
              ${t("SALA DE REVISIÓN VAR", "VAR REVIEW ROOM")}
            </div>
            <div style="font-family:var(--mono);font-size:14px;color:#ffd700;font-weight:700">
              ${finalizado ? t("VEREDICTO FINAL DE LA SALA", "ROOM FINAL VERDICT") : `${t("Tiempo para votar:", "Time to vote:")} ${tiempo}s`}
            </div>
          </div>

          <!-- Dictamen central -->
          <div style="padding:14px;border-radius:10px;background:rgba(255,0,85,0.1);border:1px dashed #ff0055">
            <div class="disp" style="font-size:20px;font-weight:700;color:#fff">${veredicto}</div>
            <div class="mono mut" style="font-size:12px;margin-top:4px">${total} ${t("votos de espectadores", "spectator votes")}</div>
          </div>

          <!-- Barra de porcentajes -->
          <div style="height:12px;width:100%;background:rgba(0,0,0,0.5);border-radius:6px;overflow:hidden;display:flex;border:1px solid rgba(255,255,255,0.15)">
            <div style=${`width:${pPenal}%;background:#00f0ff;transition:width .3s ease`}></div>
            <div style=${`width:${pNada}%;background:#888;transition:width .3s ease`}></div>
            <div style=${`width:${pRoja}%;background:#ff0055;transition:width .3s ease`}></div>
          </div>

          <!-- Opciones de voto para espectadores -->
          <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px">
            <button class="btn" style="background:rgba(0,240,255,0.15);border:1px solid #00f0ff;color:#fff;padding:8px 4px;font-size:12px"
              disabled=${finalizado} onClick=${() => votarVAR({ opcion: "penal", usuario: m?.sala?.anfitrion || "Tú" })}>
              <div>⚽ Penal</div>
              <b class="mono" style="color:#00f0ff">${pPenal}%</b>
            </button>
            <button class="btn" style="background:rgba(255,255,255,0.08);border:1px solid #aaa;color:#fff;padding:8px 4px;font-size:12px"
              disabled=${finalizado} onClick=${() => votarVAR({ opcion: "nada", usuario: m?.sala?.anfitrion || "Tú" })}>
              <div>🏊 Piscinazo</div>
              <b class="mono">${pNada}%</b>
            </button>
            <button class="btn" style="background:rgba(255,0,85,0.15);border:1px solid #ff0055;color:#fff;padding:8px 4px;font-size:12px"
              disabled=${finalizado} onClick=${() => votarVAR({ opcion: "roja", usuario: m?.sala?.anfitrion || "Tú" })}>
              <div>🟥 Roja</div>
              <b class="mono" style="color:#ff0055">${pRoja}%</b>
            </button>
          </div>

          <!-- Botón de cierre -->
          <div style="margin-top:8px">
            <button class="btn suave" onClick=${() => { cerrarVAR({ m }); alCerrar(); }} style="width:100%">
              ${finalizado ? t("ACEPTAR Y CONTINUAR JUEGO", "ACCEPT & CONTINUE") : t("CANCELAR VAR", "CANCEL VAR")}
            </button>
          </div>

        </div></div>
      </div>
    </div>`;
}

