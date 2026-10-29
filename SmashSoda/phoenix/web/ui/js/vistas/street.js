// =============================================================================
//  Phoenix Link · Panel Táctico Modo Street, Físicas Callejeras & Contador de Caños
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Controla las reglas de fútbol callejero/losa: Presets de físicas de muro,
//  regla rápida «A 2 Goles se Queda» / «Gol de Oro» y botón «¡QUÉ CAÑO!» (Huachas).
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton, Interruptor } from "../ui.js";
import { sonarReboteMuro, sonarSilbato, narrar } from "./locutor.js";
import { sumarProgresoRPG } from "./rpgsala.js";

const CLAVE_STREET = "phoenix.street_cfg";

export const PRESETS_STREET = {
  jaula: {
    nombre: "🧱 La Jaula de Hierro",
    desc: "Muros elásticos de acero (90% rebote), pases cohete y disparos bomba.",
    rebote: 0.90,
    friccion: 0.98,
    velPase: 1.4,
    tiroFuerte: true,
  },
  futsal: {
    nombre: "⚽ Futsal Técnico de Losa",
    desc: "Rebotes amortiguados (40%), pases milimétricos al pie y control fino.",
    rebote: 0.40,
    friccion: 0.94,
    velPase: 1.1,
    tiroFuerte: false,
  },
  frenesi: {
    nombre: "🌪️ Frenesí Arcade / Modo Loco",
    desc: "Rebotes al 100%, súper velocidad, rebotes infinitos y caos total.",
    rebote: 1.00,
    friccion: 0.99,
    velPase: 1.5,
    tiroFuerte: true,
  },
};

export function obtenerConfigStreet() {
  try {
    const raw = localStorage.getItem(CLAVE_STREET);
    return raw ? JSON.parse(raw) : {
      activo: false,
      preset: "jaula",
      reglaGoles: 2, // 0 = sin límite, 1 = gol de oro, 2 = a 2 goles, 3 = a 3 goles
      totalHuachasPartido: 0,
    };
  } catch {
    return { activo: false, preset: "jaula", reglaGoles: 2, totalHuachasPartido: 0 };
  }
}

export function guardarConfigStreet(cfg) {
  try {
    localStorage.setItem(CLAVE_STREET, JSON.stringify(cfg));
    window.dispatchEvent(new CustomEvent("phoenix:street_evento", { detail: cfg }));
  } catch (e) {}
}

/** Sincroniza las físicas en caliente con el archivo phoenix_core.json de Sider */
export async function sincronizarFisicasSider(cfg, { avisarExito = true } = {}) {
  const c = cfg || obtenerConfigStreet();
  const p = PRESETS_STREET[c.preset] || PRESETS_STREET.jaula;
  const payload = {
    preset: c.preset,
    muro_rebote: p.rebote,
    muro_friccion: p.friccion,
    pase_vel_k: p.velPase,
    cancha_semilargo: 15,
    cancha_semiancho: 10,
    muros_core: true,
    cancha_paredes: false
  };

  try {
    const res = await fetch("http://127.0.0.1:38888/sync-street", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    if (res.ok) {
      if (avisarExito) {
        avisar(t("⚡ ¡Físicas sincronizadas en vivo con PES 2021!", "⚡ Physics synced live with PES 2021!"), "ok");
        sonarReboteMuro();
      }
      return { ok: true };
    }
  } catch (e) {
    // Si no está corriendo el demonio local
  }
  if (avisarExito) {
    avisar(t("ℹ️ Físicas guardadas en Link. Para sync en vivo ejecuta: tools/INICIAR_SYNC_SIDER.bat", "Saved in Link. For live sync run: tools/INICIAR_SYNC_SIDER.bat"), "info", 5000);
  }
  return { ok: false, motivo: "Demonio de sync apagado" };
}

/** Dispara la celebración de un caño o huacha */
export async function registrarCannoHuacha({ jugador = "El atacante", m } = {}) {
  sonarReboteMuro();
  narrar("¡Upaaa! ¡Tremendo caño callejero! ¡Humillación total en la cancha!", { prioridad: true });

  const cfg = obtenerConfigStreet();
  cfg.totalHuachasPartido = (cfg.totalHuachasPartido || 0) + 1;
  guardarConfigStreet(cfg);

  sumarProgresoRPG({ nombre: jugador, huachas: 1 });

  if (m?.sala?.abierta) {
    await accion("chat.enviar", {
      texto: `🥜 ¡QUÉ CAÑO! ¡${jugador} acaba de tirar una huacha humillante en el Modo Street! (+1 Lujo)`,
    }, { silencioso: true });
  }
  avisar(t(`🥜 ¡QUÉ CAÑO de ${jugador}!`, `🥜 NUTMEG by ${jugador}!`), "ok");
}

// =============================================================================
//  Componentes Visuales de Modo Street
// =============================================================================

export function BarraModoStreet({ m, esclavo }) {
  const [cfg, setCfg] = useState(obtenerConfigStreet);
  const [modalTuning, setModalTuning] = useState(false);

  useEffect(() => {
    const alEv = (e) => setCfg(e.detail);
    window.addEventListener("phoenix:street_evento", alEv);
    return () => window.removeEventListener("phoenix:street_evento", alEv);
  }, []);

  const alternarModo = () => {
    const nuevo = { ...cfg, activo: !cfg.activo };
    setCfg(nuevo);
    guardarConfigStreet(nuevo);
    if (nuevo.activo) {
      narrar("Modo Street activado. ¡Fútbol callejero en la losa!");
      avisar(t("⚽ MODO STREET ACTIVADO (Cancha chica y muros)", "⚽ STREET MODE ACTIVE"), "ok");
    } else {
      avisar(t("Modo Street desactivado", "Street mode deactivated"), "info");
    }
  };

  const rotarReglaGoles = () => {
    const orden = [0, 1, 2, 3];
    const idx = orden.indexOf(cfg.reglaGoles);
    const sig = orden[(idx + 1) % orden.length];
    const nuevo = { ...cfg, reglaGoles: sig };
    setCfg(nuevo);
    guardarConfigStreet(nuevo);
  };

  const textoRegla = cfg.reglaGoles === 0 ? t("90 MINUTOS", "90 MINUTES")
    : cfg.reglaGoles === 1 ? t("GOL DE ORO", "GOLDEN GOAL")
    : t(`A ${cfg.reglaGoles} GOLES`, `FIRST TO ${cfg.reglaGoles}`);

  const presetActual = PRESETS_STREET[cfg.preset] || PRESETS_STREET.jaula;

  return html`
    <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;padding:8px 14px;border-radius:10px;background:linear-gradient(90deg,rgba(255,115,0,0.12),rgba(0,0,0,0.4));border:1px solid rgba(255,115,0,0.35);margin-bottom:8px;flex-wrap:wrap">
      
      <div style="display:flex;align-items:center;gap:8px">
        <span style="font-size:18px">🧱⚽</span>
        <span class="disp" style="font-weight:700;font-size:13px;letter-spacing:.08em;color:#ff8800">
          ${t("MODO STREET", "STREET MODE")}
        </span>
        <button class="btn mini" style=${`background:${cfg.activo ? "rgba(255,115,0,0.25)" : "rgba(255,255,255,0.06)"};border:1px solid ${cfg.activo ? "#ff8800" : "rgba(255,255,255,0.15)"};color:${cfg.activo ? "#ffaa33" : "var(--mut)"};font-size:11px`}
          onClick=${alternarModo}>
          ${cfg.activo ? t("● ACTIVO", "● ACTIVE") : t("○ APAGADO", "○ OFF")}
        </button>
      </div>

      ${cfg.activo ? html`
        <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap">
          <button class="btn mini suave" onClick=${() => setModalTuning(true)} style="border-color:#ff8800;color:#fff" title=${presetActual.desc}>
            🎛️ ${presetActual.nombre}
          </button>

          <button class="btn mini suave" onClick=${rotarReglaGoles} title=${t("Haz clic para cambiar la regla de finalización rápida", "Click to cycle quick finish rule")}>
            ⚡ ${t("REGLA:", "RULE:")} <b style="color:#ffd700">${textoRegla}</b>
          </button>

          <button class="btn mini" style="background:rgba(255,215,0,0.15);border:1px solid #ffd700;color:#ffd700"
            onClick=${() => registrarCannoHuacha({ jugador: m?.sala?.hostNombre || "Anfitrión", m })}>
            🥜 ${t("¡QUÉ CAÑO!", "NUTMEG!")} (${cfg.totalHuachasPartido || 0})
          </button>
        </div>` : null}

      ${modalTuning ? html`<${ModalStreetTuning} m=${m} alCerrar=${() => setModalTuning(false)}/>` : null}
    </div>`;
}

export function ModalStreetTuning({ m, alCerrar }) {
  const [cfg, setCfg] = useState(obtenerConfigStreet);
  const [sincronizando, setSincronizando] = useState(false);

  const elegirPreset = async (k) => {
    const nuevo = { ...cfg, preset: k };
    setCfg(nuevo);
    guardarConfigStreet(nuevo);
    narrar(`Físicas actualizadas a: ${PRESETS_STREET[k].nombre}`);
    avisar(t(`Preset cargado: ${PRESETS_STREET[k].nombre}`, `Preset loaded: ${PRESETS_STREET[k].nombre}`), "ok");
    await sincronizarFisicasSider(nuevo, { avisarExito: false });
  };

  const forzarSync = async () => {
    setSincronizando(true);
    await sincronizarFisicasSider(cfg, { avisarExito: true });
    setSincronizando(false);
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:520px;width:95%">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:14px">
          
          <div style="display:flex;justify-content:space-between;align-items:center">
            <div style="display:flex;align-items:center;gap:10px">
              <span style="font-size:26px">🎛️🧱</span>
              <div>
                <h2 style="margin:0;font-size:18px">${t("Físicas Callejeras (Street Tuning)", "Street Physics Tuning")}</h2>
                <div class="ayuda" style="font-size:12px">${t("Ajusta en tiempo real los muros y el ritmo de juego.", "Tune rebound walls and pace in real-time.")}</div>
              </div>
            </div>
            <button class="btn suave mini" onClick=${alCerrar}>✕</button>
          </div>

          <div class="lab" style="font-size:11px;color:#ff8800;margin-top:4px">${t("PRESETS DE FÍSICA PARA PHOENIX CORE", "PHYSICS PRESETS FOR PHOENIX CORE")}</div>

          <div style="display:flex;flex-direction:column;gap:10px">
            ${Object.entries(PRESETS_STREET).map(([k, p]) => {
              const sel = cfg.preset === k;
              return html`
                <div onClick=${() => elegirPreset(k)}
                  style=${`cursor:pointer;padding:12px 14px;border-radius:10px;background:${sel ? "rgba(255,115,0,0.15)" : "rgba(255,255,255,0.03)"};border:1.5px solid ${sel ? "#ff8800" : "rgba(255,255,255,0.12)"};transition:all .2s ease`}>
                  <div style="display:flex;justify-content:space-between;align-items:center">
                    <b style="font-size:14px;color:${sel ? "#ffaa33" : "#fff"}">${p.nombre}</b>
                    ${sel ? html`<span style="color:#00ff88;font-size:12px;font-weight:700">✓ ACTIVO</span>` : null}
                  </div>
                  <div class="ayuda" style="font-size:12px;margin-top:4px">${p.desc}</div>
                  <div class="mono mut" style="font-size:11px;margin-top:6px">
                    Rebote: ${Math.round(p.rebote * 100)}% · Fricción: ${p.friccion} · Vel. Pase: x${p.velPase} · Tiro Potente: ${p.tiroFuerte ? "SÍ" : "NO"}
                  </div>
                </div>`;
            })}
          </div>

          <div style="background:rgba(0,0,0,0.3);border:1px dashed rgba(255,115,0,0.4);border-radius:8px;padding:10px 14px;display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:4px">
            <div style="font-size:12px">
              <b style="color:#ffaa33">⚡ Sider Hot-Sync:</b>
              <div class="mut" style="font-size:11px">Aplica cambios en <code>phoenix_core.json</code> sin reiniciar PES.</div>
            </div>
            <button class="btn mini" style="background:#ff7300;color:#000;font-weight:700;white-space:nowrap"
              disabled=${sincronizando} onClick=${forzarSync}>
              ${sincronizando ? "..." : t("⚡ SINCRONIZAR YA", "⚡ SYNC NOW")}
            </button>
          </div>

          <div class="botones" style="margin-top:8px;display:flex;justify-content:flex-end">
            <button class="btn acc" onClick=${alCerrar}>${t("Listo", "Done")}</button>
          </div>

        </div></div>
      </div>
    </div>`;
}

