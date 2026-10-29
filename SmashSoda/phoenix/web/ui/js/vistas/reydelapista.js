// =============================================================================
//  Modo Autónomo: Rey de la Pista (Ganador se queda / King of the Hill)
//  Al terminar un partido, el equipo ganador se queda en sus puestos y el equipo
//  perdedor rota automáticamente a la cola de espectadores en Mirando, sentando
//  de inmediato a los siguientes retadores en turno.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Boton, Icono } from "../ui.js";

const CLAVE_STORAGE = "phoenix.reydelapista";
const EVENTO_CAMBIO = "phoenix:reydelapista";

const CONFIG_DEFECTO = {
  activo: false,
  rachaMax: 0,          // 0 = Sin límite de victorias consecutivas; 2, 3, 5 = límite
  rachaActual: 0,
  reyActual: null,      // "a" | "b" | null
  historial: [],
};

export function obtenerConfigRey() {
  try {
    const raw = localStorage.getItem(CLAVE_STORAGE);
    if (raw) return { ...CONFIG_DEFECTO, ...JSON.parse(raw) };
  } catch { /* ignorar */ }
  return { ...CONFIG_DEFECTO };
}

export function guardarConfigRey(cfg) {
  try {
    localStorage.setItem(CLAVE_STORAGE, JSON.stringify(cfg));
    window.dispatchEvent(new CustomEvent(EVENTO_CAMBIO, { detail: cfg }));
  } catch { /* ignorar */ }
}

export function useReyDeLaPista() {
  const [config, setConfigState] = useState(obtenerConfigRey);

  useEffect(() => {
    const alCambiar = (e) => {
      if (e?.detail) setConfigState(e.detail);
      else setConfigState(obtenerConfigRey());
    };
    window.addEventListener(EVENTO_CAMBIO, alCambiar);
    return () => window.removeEventListener(EVENTO_CAMBIO, alCambiar);
  }, []);

  const setConfig = (mod) => {
    const nuevo = typeof mod === "function" ? mod(config) : mod;
    guardarConfigRey(nuevo);
  };

  return [config, setConfig];
}

/** Ejecuta la rotación de Rey de la Pista según el ganador ("a" | "b" | "empate") */
export async function ejecutarRotacionRey({ ganador, m, forzar = false }) {
  const cfg = obtenerConfigRey();
  if (!cfg.activo && !forzar) return false;

  const listaMandos = m?.mandos?.lista || [];
  const invitados = m?.sala?.invitados || [];

  if (!ganador || ganador === "empate") {
    avisar(t("👑 Rey de la Pista: Empate. Ambos equipos se mantienen para desempate.", "👑 King of the Hill: Draw. Both teams stay for tiebreaker."), "info", 5000);
    return false;
  }

  const perdedor = ganador === "a" ? "b" : "a";
  const mismoRey = cfg.reyActual === ganador;
  const nuevaRacha = mismoRey ? (cfg.rachaActual || 0) + 1 : 1;

  // Verificar si el rey llegó a la racha máxima y se retira invicto
  const reySeRetira = cfg.rachaMax > 0 && nuevaRacha >= cfg.rachaMax;

  // Puestos del equipo que debe salir
  const puestosASalir = listaMandos.filter(
    (p) => p.ocupado && p.parsecId && (reySeRetira ? true : p.equipo === perdedor)
  );

  // Espectadores en espera que no están sentados en ningún puesto
  const sentadosIds = new Set(listaMandos.filter((p) => p.ocupado && p.parsecId).map((p) => p.parsecId));
  const enEspera = invitados.filter((inv) => !sentadosIds.has(inv.id));

  // 1. Quitar a los jugadores perdedores de sus puestos
  for (const p of puestosASalir) {
    await accion("mandos.asignar", { puesto: p.puesto, parsecId: 0 }, { silencioso: true });
  }

  // 2. Sentar a los siguientes retadores de la cola
  const puestosLibres = listaMandos.filter((p) => reySeRetira ? true : p.equipo === perdedor);
  let sentadosNuevos = 0;
  for (let i = 0; i < Math.min(enEspera.length, puestosLibres.length); i++) {
    const spec = enEspera[i];
    const puestoDestino = puestosLibres[i];
    if (spec && puestoDestino) {
      await accion("mandos.asignar", { puesto: puestoDestino.puesto, parsecId: spec.id }, { silencioso: true });
      sentadosNuevos++;
    }
  }

  if (reySeRetira) {
    avisar(
      t(
        `👑 ¡El Rey (${ganador.toUpperCase()}) se retira invicto con ${nuevaRacha}V! Entran retadores a la pista.`,
        `👑 King (${ganador.toUpperCase()}) retires undefeated with ${nuevaRacha}W! New challengers take the pitch.`
      ),
      "ok",
      7000
    );
    guardarConfigRey({
      ...cfg,
      reyActual: null,
      rachaActual: 0,
    });
  } else {
    avisar(
      t(
        `👑 ¡Equipo ${ganador.toUpperCase()} se mantiene como Rey (${nuevaRacha}V consecutiva${nuevaRacha > 1 ? "s" : ""})! Entran ${sentadosNuevos} retadores.`,
        `👑 Team ${ganador.toUpperCase()} stays King (${nuevaRacha}W streak)! ${sentadosNuevos} challengers seated.`
      ),
      "ok",
      7000
    );
    guardarConfigRey({
      ...cfg,
      reyActual: ganador,
      rachaActual: nuevaRacha,
    });
  }

  return true;
}

/** Barra de control visual del Modo Rey de la Pista */
export function BarraReyDeLaPista({ m, esclavo }) {
  const [cfg, setCfg] = useReyDeLaPista();
  const [rotando, setRotando] = useState(false);

  const toggleActivo = () => {
    setCfg((prev) => ({
      ...prev,
      activo: !prev.activo,
      rachaActual: !prev.activo ? 0 : prev.rachaActual,
      reyActual: !prev.activo ? null : prev.reyActual,
    }));
    avisar(
      !cfg.activo
        ? t("👑 Modo Rey de la Pista ACTIVADO: El ganador de cada partido se quedará en sus puestos.", "👑 King of the Hill ON: Match winner keeps their seats.")
        : t("👑 Modo Rey de la Pista desactivado.", "👑 King of the Hill turned off."),
      "info"
    );
  };

  const cambiarRachaMax = (e) => {
    const val = Number(e.currentTarget.value) || 0;
    setCfg((prev) => ({ ...prev, rachaMax: val }));
  };

  const forzarRotacion = async () => {
    if (rotando) return;
    const ok = await confirmar({
      titulo: t("👑 ¿Rotar Rey de la Pista ahora?", "👑 Rotate King of the Hill now?"),
      texto: t(
        "Elige qué equipo se queda como Rey. El equipo rival pasará a la cola y entrarán los siguientes espectadores.",
        "Choose which team stays as King. The rival team moves to queue and spectators will enter."
      ),
      botones: [
        { texto: t("EQUIPO A SE QUEDA", "TEAM A STAYS"), tipo: "ok", valor: "a" },
        { texto: t("EQUIPO B SE QUEDA", "TEAM B STAYS"), tipo: "ok", valor: "b" },
        { texto: t("Cancelar", "Cancel"), tipo: "suave", valor: null },
      ],
    });
    if (!ok) return;
    setRotando(true);
    try {
      await ejecutarRotacionRey({ ganador: ok, m, forzar: true });
    } finally {
      setRotando(false);
    }
  };

  const reiniciarRacha = () => {
    setCfg((prev) => ({ ...prev, rachaActual: 0, reyActual: null }));
    avisar(t("Racha de Rey de la Pista reiniciada.", "King of the Hill streak reset."), "info");
  };

  const reyNom = cfg.reyActual ? (cfg.reyActual === "a" ? t("Equipo A", "Team A") : t("Equipo B", "Team B")) : null;

  return html`
    <div style="display:flex;align-items:center;justify-content:space-between;gap:14px;padding:10px 16px;background:linear-gradient(90deg,rgba(255,184,0,0.08),rgba(255,184,0,0.02));border:1px solid ${cfg.activo ? 'rgba(255,184,0,0.35)' : 'var(--linea)'};border-radius:10px;margin-bottom:12px;flex-wrap:wrap">
      <div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
        <button
          onClick=${toggleActivo}
          disabled=${esclavo}
          style="display:flex;align-items:center;gap:8px;padding:6px 12px;background:${cfg.activo ? 'linear-gradient(135deg,#ffb800,#ff8800)' : 'var(--fondo-tarjeta)'};border:1px solid ${cfg.activo ? '#ffb800' : 'var(--linea)'};color:${cfg.activo ? '#000' : 'var(--mut)'};font:700 12px var(--f-mono);border-radius:6px;cursor:pointer;letter-spacing:0.05em;transition:all .2s ease"
          title=${t("Al terminar cada partido, el ganador se mantiene y el perdedor rota a espectadores", "At match end, winner stays and loser rotates to spectators")}
        >
          <span>👑</span>
          <span>${cfg.activo ? t("REY DE LA PISTA: ACTIVO", "KING OF THE HILL: ON") : t("REY DE LA PISTA: OFF", "KING OF THE HILL: OFF")}</span>
        </button>

        ${cfg.activo ? html`
          <div style="display:flex;align-items:center;gap:8px;font:500 12px var(--f-body);color:var(--txt)">
            ${reyNom ? html`
              <span class="chip ok" style="background:rgba(255,184,0,0.15);border:1px solid #ffb800;color:#ffb800;font-weight:700">
                👑 ${reyNom} (${cfg.rachaActual}V${cfg.rachaMax > 0 ? ` / ${cfg.rachaMax}` : ""})
              </span>
            ` : html`
              <span class="chip mut" style="font-size:11px">${t("Primer partido · Sin rey aún", "1st match · No king yet")}</span>
            `}

            <label style="display:flex;align-items:center;gap:6px;margin-left:8px;color:var(--mut);font-size:11px">
              <span>${t("Racha máx:", "Max streak:")}</span>
              <select
                value=${String(cfg.rachaMax)}
                onChange=${cambiarRachaMax}
                style="background:var(--fondo-tarjeta);color:var(--txt);border:1px solid var(--linea);border-radius:4px;padding:2px 6px;font:500 11px var(--f-mono);cursor:pointer"
              >
                <option value="0">${t("Sin límite", "No limit")}</option>
                <option value="2">2 ${t("victorias", "wins")}</option>
                <option value="3">3 ${t("victorias", "wins")}</option>
                <option value="5">5 ${t("victorias", "wins")}</option>
              </select>
            </label>
          </div>
        ` : null}
      </div>

      ${cfg.activo ? html`
        <div style="display:flex;align-items:center;gap:8px">
          <${Boton} tipo="mini suave" al=${forzarRotacion} deshabilitado=${esclavo || rotando}
            titulo=${t("Fuerza la rotación de Rey de la Pista sin esperar al final del partido", "Forces King of the Hill rotation right now")}>
            ⚡ ${t("ROTAR AHORA", "ROTATE NOW")}
          </${Boton}>
          ${cfg.rachaActual > 0 ? html`
            <${Boton} tipo="mini suave" al=${reiniciarRacha} deshabilitado=${esclavo}
              titulo=${t("Reinicia el contador de victorias del rey", "Resets the king streak counter")}>
              ↻ ${t("REINICIAR", "RESET")}
            </${Boton}>
          ` : null}
        </div>
      ` : null}
    </div>
  `;
}
