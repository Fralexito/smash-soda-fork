// =============================================================================
//  Phoenix Link · Locutor / Comentarista Deportivo con Voz IA & Soundboard
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Sintetiza voz en español usando Web Speech API nativo de Windows (sin coste)
//  y efectos de sonido de estadio generados en tiempo real con Web Audio API.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { Boton, Icono, Tarjeta, Interruptor } from "../ui.js";

const CLAVE_CONFIG = "phoenix.locutor_cfg";

/** Configuración del locutor */
export function obtenerConfigLocutor() {
  try {
    const raw = localStorage.getItem(CLAVE_CONFIG);
    return raw ? JSON.parse(raw) : { activo: true, volumen: 1.0, velocidad: 1.05, sfx: true };
  } catch {
    return { activo: true, volumen: 1.0, velocidad: 1.05, sfx: true };
  }
}

export function guardarConfigLocutor(cfg) {
  try {
    localStorage.setItem(CLAVE_CONFIG, JSON.stringify(cfg));
    window.dispatchEvent(new CustomEvent("phoenix:locutor_cfg", { detail: cfg }));
  } catch (e) {
    console.error("[locutor] Error guardando config:", e);
  }
}

/** Habla un texto con entonación de comentarista deportivo */
export function narrar(texto, { prioridad = false } = {}) {
  const cfg = obtenerConfigLocutor();
  if (!cfg.activo) return;
  if (!("speechSynthesis" in window)) return;

  if (prioridad) {
    window.speechSynthesis.cancel();
  }

  const u = new SpeechSynthesisUtterance(texto);
  u.lang = "es-ES";
  u.rate = cfg.velocidad || 1.05;
  u.volume = cfg.volumen || 1.0;
  u.pitch = 1.1; // tono ligeramente enérgico

  // Buscar una voz natural en español
  const voces = window.speechSynthesis.getVoices();
  const vozEs = voces.find((v) => v.lang.startsWith("es") && (v.name.includes("Natural") || v.name.includes("Sabina") || v.name.includes("Raul") || v.name.includes("Helena") || v.name.includes("Pablo")))
    || voces.find((v) => v.lang.startsWith("es"));
  if (vozEs) u.voice = vozEs;

  window.speechSynthesis.speak(u);
}

// =============================================================================
//  Soundboard Sintetizado con Web Audio API (Sin archivos externos)
// =============================================================================

let _ctxAudio = null;
function getAudioContext() {
  if (!_ctxAudio) {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    if (AudioContext) _ctxAudio = new AudioContext();
  }
  if (_ctxAudio && _ctxAudio.state === "suspended") {
    _ctxAudio.resume();
  }
  return _ctxAudio;
}

/** Reproduce un silbato de árbitro realista */
export function sonarSilbato({ largo = false } = {}) {
  const cfg = obtenerConfigLocutor();
  if (!cfg.sfx) return;
  const ctx = getAudioContext();
  if (!ctx) return;

  const osc = ctx.createOscillator();
  const osc2 = ctx.createOscillator();
  const gain = ctx.createGain();

  const dur = largo ? 0.7 : 0.25;
  const now = ctx.currentTime;

  osc.type = "sine";
  osc.frequency.setValueAtTime(2800, now);
  osc.frequency.exponentialRampToValueAtTime(3200, now + dur);

  osc2.type = "triangle";
  osc2.frequency.setValueAtTime(2850, now);
  osc2.frequency.exponentialRampToValueAtTime(3250, now + dur);

  gain.gain.setValueAtTime(0.3, now);
  gain.gain.exponentialRampToValueAtTime(0.001, now + dur);

  osc.connect(gain);
  osc2.connect(gain);
  gain.connect(ctx.destination);

  osc.start(now);
  osc2.start(now);
  osc.stop(now + dur);
  osc2.stop(now + dur);
}

/** Reproduce una ovación de gol / explosión de estadio */
export function sonarGol() {
  const cfg = obtenerConfigLocutor();
  if (!cfg.sfx) return;
  const ctx = getAudioContext();
  if (!ctx) return;

  // Ruido blanco filtrado para simular rugido de grada
  const bufferSize = ctx.sampleRate * 1.5;
  const buffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
  const data = buffer.getChannelData(0);
  for (let i = 0; i < bufferSize; i++) {
    data[i] = Math.random() * 2 - 1;
  }

  const noise = ctx.createBufferSource();
  noise.buffer = buffer;

  const filter = ctx.createBiquadFilter();
  filter.type = "bandpass";
  filter.frequency.setValueAtTime(800, ctx.currentTime);
  filter.frequency.linearRampToValueAtTime(1200, ctx.currentTime + 1.2);

  const gain = ctx.createGain();
  gain.gain.setValueAtTime(0.01, ctx.currentTime);
  gain.gain.linearRampToValueAtTime(0.4, ctx.currentTime + 0.3);
  gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 1.5);

  noise.connect(filter);
  filter.connect(gain);
  gain.connect(ctx.destination);

  noise.start();
}

/** Reproduce sonido de rebote seco en reja o chapa (Modo Street) */
export function sonarReboteMuro() {
  const cfg = obtenerConfigLocutor();
  if (!cfg.sfx) return;
  const ctx = getAudioContext();
  if (!ctx) return;

  const now = ctx.currentTime;
  const osc = ctx.createOscillator();
  const gain = ctx.createGain();

  osc.type = "sawtooth";
  osc.frequency.setValueAtTime(340, now);
  osc.frequency.exponentialRampToValueAtTime(60, now + 0.15);

  gain.gain.setValueAtTime(0.4, now);
  gain.gain.exponentialRampToValueAtTime(0.001, now + 0.15);

  osc.connect(gain);
  gain.connect(ctx.destination);

  osc.start(now);
  osc.stop(now + 0.15);
}

/** Sirena de alarma dramática de VAR */
export function sonarSirenaVAR() {
  const cfg = obtenerConfigLocutor();
  if (!cfg.sfx) return;
  const ctx = getAudioContext();
  if (!ctx) return;

  const now = ctx.currentTime;
  const osc = ctx.createOscillator();
  const gain = ctx.createGain();

  osc.type = "sine";
  osc.frequency.setValueAtTime(650, now);
  osc.frequency.linearRampToValueAtTime(950, now + 0.3);
  osc.frequency.linearRampToValueAtTime(650, now + 0.6);
  osc.frequency.linearRampToValueAtTime(950, now + 0.9);

  gain.gain.setValueAtTime(0.25, now);
  gain.gain.exponentialRampToValueAtTime(0.001, now + 1.2);

  osc.connect(gain);
  gain.connect(ctx.destination);

  osc.start(now);
  osc.stop(now + 1.2);
}

// =============================================================================
//  Componente Visual: Panel de Control del Locutor
// =============================================================================

export function PanelLocutorModal({ alCerrar }) {
  const [cfg, setCfg] = useState(obtenerConfigLocutor);

  const toggle = (campo) => {
    const nuevo = { ...cfg, [campo]: !cfg[campo] };
    setCfg(nuevo);
    guardarConfigLocutor(nuevo);
  };

  const probar = () => {
    sonarGol();
    narrar("¡Goooolazo impresionante! ¡Apareció la magia en Phoenix Link!", { prioridad: true });
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:440px">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:14px">
          <div style="display:flex;justify-content:space-between;align-items:center">
            <div style="display:flex;align-items:center;gap:10px">
              <span style="font-size:24px">🎙️</span>
              <div>
                <h2 style="margin:0;font-size:17px">${t("Locutor & Efectos de Estadio", "Announcer & Stadium SFX")}</h2>
                <div class="ayuda" style="font-size:12px">${t("Narración con voz IA en vivo y efectos sintetizados.", "Live AI voice narration & synthesized SFX.")}</div>
              </div>
            </div>
            <button class="btn suave mini" onClick=${alCerrar}>✕</button>
          </div>

          <div style="display:flex;flex-direction:column;gap:10px;margin-top:6px">
            <div style="display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:rgba(255,255,255,0.03);border-radius:8px">
              <div>
                <div style="font-weight:600;font-size:13px">${t("Narrador con Voz en Español", "Spanish Voice Announcer")}</div>
                <div class="ayuda" style="font-size:11px">${t("Relata goles, advertencias de lag y nuevos reyes de la pista.", "Narrates goals, lag alerts and new kings.")}</div>
              </div>
              <${Interruptor} valor=${cfg.activo} al=${() => toggle("activo")}/>
            </div>

            <div style="display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:rgba(255,255,255,0.03);border-radius:8px">
              <div>
                <div style="font-weight:600;font-size:13px">${t("Efectos de Sonido (SFX)", "Sound Effects (SFX)")}</div>
                <div class="ayuda" style="font-size:11px">${t("Silbato de árbitro, ovación de grada y rebote en chapa.", "Whistle, stadium cheer and street wall rebound.")}</div>
              </div>
              <${Interruptor} valor=${cfg.sfx} al=${() => toggle("sfx")}/>
            </div>
          </div>

          <!-- Soundboard de prueba rápida -->
          <div style="display:flex;flex-direction:column;gap:6px;margin-top:4px">
            <div class="lab" style="font-size:11px">${t("PROBAR SONIDOS RÁPIDOS", "TEST QUICK SOUNDS")}</div>
            <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">
              <${Boton} tipo="mini suave" al=${() => sonarSilbato({ largo: false })}>📢 ${t("Silbato", "Whistle")}</${Boton}>
              <${Boton} tipo="mini suave" al=${sonarGol}>🏟️ ${t("Ovación Gol", "Goal Cheer")}</${Boton}>
              <${Boton} tipo="mini suave" al=${sonarReboteMuro}>🧱 ${t("Rebote Reja", "Wall Rebound")}</${Boton}>
              <${Boton} tipo="mini suave" al=${sonarSirenaVAR}>🚨 ${t("Sirena VAR", "VAR Siren")}</${Boton}>
            </div>
          </div>

          <div class="botones" style="margin-top:10px;display:flex;justify-content:space-between;gap:8px">
            <${Boton} tipo="acc mini" al=${probar}>▶ ${t("Probar Narración", "Test Voice")}</${Boton}>
            <button class="btn suave" onClick=${alCerrar}>${t("Cerrar", "Close")}</button>
          </div>
        </div></div>
      </div>
    </div>`;
}
