// =============================================================================
//  Phoenix Link · Bracket / Cuadro de Torneo Relámpago (Esports)
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Genera llaves eliminatorias directas (4 u 8 jugadores) para organizar copas
//  express en la sala. Incluye avance automático y botón de 1 clic para sentar
//  a los rivales del cruce en los mandos sin salir de la vista.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Boton, Icono, Tarjeta, Avatar } from "../ui.js";

const CLAVE_STORAGE = "phoenix.bracket_relampago";

/** Obtiene el estado del torneo actual */
export function obtenerBracket() {
  try {
    const raw = localStorage.getItem(CLAVE_STORAGE);
    return raw ? JSON.parse(raw) : { activo: false, tamano: 4, partidos: [], campeon: null };
  } catch {
    return { activo: false, tamano: 4, partidos: [], campeon: null };
  }
}

/** Guarda y sincroniza el torneo */
export function guardarBracket(datos) {
  try {
    localStorage.setItem(CLAVE_STORAGE, JSON.stringify(datos));
    window.dispatchEvent(new CustomEvent("phoenix:bracket", { detail: datos }));
  } catch (e) {
    console.error("[bracket] Error guardando estado:", e);
  }
}

/** Genera las llaves para 4 jugadores (Semis -> Final) */
export async function iniciarBracket4({ jugadores = [], m } = {}) {
  if (jugadores.length < 2) {
    avisar(t("Se necesitan al menos 2 jugadores en la sala.", "Need at least 2 players in the room."), "warn");
    return;
  }

  // Barajar aleatoriamente
  const copia = [...jugadores].sort(() => Math.random() - 0.5);
  while (copia.length < 4) {
    copia.push({ nombre: `BYE (Pase)`, parsecId: null, bye: true });
  }

  const partidos = [
    { id: 1, ronda: "semis", etiqueta: "Semifinal 1", j1: copia[0], j2: copia[1], goles1: null, goles2: null, ganador: null, estado: "listo" },
    { id: 2, ronda: "semis", etiqueta: "Semifinal 2", j1: copia[2], j2: copia[3], goles1: null, goles2: null, ganador: null, estado: "listo" },
    { id: 3, ronda: "final", etiqueta: "GRAN FINAL 🏆", j1: null, j2: null, goles1: null, goles2: null, ganador: null, estado: "esperando" },
  ];

  // Resolver BYE si alguno existe
  for (const p of partidos.slice(0, 2)) {
    if (p.j1.bye && !p.j2.bye) p.ganador = p.j2;
    else if (!p.j1.bye && p.j2.bye) p.ganador = p.j1;
  }

  // Si hubo BYEs en semis, avanzar directo a la final
  if (partidos[0].ganador) partidos[2].j1 = partidos[0].ganador;
  if (partidos[1].ganador) partidos[2].j2 = partidos[1].ganador;
  if (partidos[2].j1 && partidos[2].j2) partidos[2].estado = "listo";

  const bracket = { activo: true, tamano: 4, creadaEn: Date.now(), partidos, campeon: null };
  guardarBracket(bracket);

  if (m?.sala?.abierta) {
    await accion("chat.enviar", {
      texto: `🏆 ¡COPA RELÁMPAGO INICIADA (4 Jugadores)! Semifinal 1: ${copia[0].nombre} vs ${copia[1].nombre} | Semifinal 2: ${copia[2].nombre} vs ${copia[3].nombre}.`,
    }, { silencioso: true });
  }
  avisar(t("🏆 Torneo relámpago iniciado con éxito", "🏆 Blitz tournament started successfully"), "ok");
}

/** Sienta automáticamente a los 2 jugadores del cruce en el Equipo A y Equipo B */
export async function cargarCruceEnPuestos({ partido, m }) {
  if (!partido?.j1 || !partido?.j2) return;
  const esclavo = !!m?.mandos?.esclavo;
  if (esclavo) {
    avisar(t("Los mandos están controlados externamente.", "Pads are controlled externally."), "warn");
    return;
  }

  // Asignar J1 al Puesto 1 (Equipo A)
  if (partido.j1.parsecId) {
    if (partido.j1.parsecId === m?.sala?.hostId) {
      await accion("mandos.tomar", { numero: 1 }, { silencioso: true });
    } else {
      await accion("mandos.asignar", { indice: 0, parsecId: partido.j1.parsecId }, { silencioso: true });
    }
  }

  // Asignar J2 al Puesto 2 (Equipo B)
  if (partido.j2.parsecId) {
    if (partido.j2.parsecId === m?.sala?.hostId) {
      await accion("mandos.tomar", { numero: 2 }, { silencioso: true });
    } else {
      await accion("mandos.asignar", { indice: 1, parsecId: partido.j2.parsecId }, { silencioso: true });
    }
  }

  avisar(t(`⚡ Cruce cargado: ${partido.j1.nombre} (Local) vs ${partido.j2.nombre} (Visitante)`, `⚡ Match loaded: ${partido.j1.nombre} vs ${partido.j2.nombre}`), "ok");
}

/** Registra el resultado del cruce y avanza a la siguiente ronda */
export async function registrarResultadoBracket({ partidoId, ganadorLado, m }) {
  const b = obtenerBracket();
  if (!b.activo) return;

  const p = b.partidos.find((x) => x.id === partidoId);
  if (!p || p.ganador) return;

  p.ganador = ganadorLado === "a" ? p.j1 : p.j2;
  p.estado = "terminado";

  // Si fue Semifinal 1 -> avanza a Final J1
  if (partidoId === 1) {
    b.partidos[2].j1 = p.ganador;
  }
  // Si fue Semifinal 2 -> avanza a Final J2
  else if (partidoId === 2) {
    b.partidos[2].j2 = p.ganador;
  }
  // Si fue la Final -> Coronar Campeón
  else if (partidoId === 3) {
    b.campeon = p.ganador;
    if (m?.sala?.abierta) {
      await accion("chat.enviar", {
        texto: `👑 ¡TENEMOS CAMPEÓN DE LA SALA! ¡Felicidades a ${p.ganador.nombre} por ganar la Copa Relámpago! 🏆🥇`,
      }, { silencioso: true });
    }
  }

  if (b.partidos[2].j1 && b.partidos[2].j2 && !b.partidos[2].ganador) {
    b.partidos[2].estado = "listo";
  }

  guardarBracket(b);
  avisar(t(`🏆 Avanza ${p.ganador.nombre} en el torneo`, `🏆 ${p.ganador.nombre} advances in tournament`), "ok");
}

/** Reinicia el torneo */
export function reiniciarBracket() {
  guardarBracket({ activo: false, tamano: 4, partidos: [], campeon: null });
  avisar(t("Torneo reiniciado", "Tournament reset"), "info");
}

// =============================================================================
//  Componentes Visuales de Bracket
// =============================================================================

export function ModalBracket({ m, alCerrar }) {
  const [b, setB] = useState(obtenerBracket);

  useEffect(() => {
    const alCambiar = (e) => setB(e.detail);
    window.addEventListener("phoenix:bracket", alCambiar);
    return () => window.removeEventListener("phoenix:bracket", alCambiar);
  }, []);

  const iniciarNuevo = async () => {
    const lista = [];
    if (m?.sala?.hostId) lista.push({ nombre: m.sala.hostNombre || "Host", parsecId: m.sala.hostId });
    for (const inv of m?.invitados || []) {
      if (!inv.falso) lista.push({ nombre: inv.nombre, parsecId: inv.parsecId });
    }
    await iniciarBracket4({ jugadores: lista, m });
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:680px;width:95%">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:16px">
          <div style="display:flex;justify-content:space-between;align-items:center">
            <div style="display:flex;align-items:center;gap:10px">
              <span style="font-size:24px">🏆</span>
              <div>
                <h2 style="margin:0;font-size:18px">${t("Copa Relámpago de la Sala", "Room Blitz Cup")}</h2>
                <span class="ayuda" style="font-size:12px">${t("Torneo eliminatorio directo para los miembros de la sala.", "Single-elimination cup for room members.")}</span>
              </div>
            </div>
            <div style="display:flex;gap:8px">
              ${b.activo ? html`<${Boton} tipo="mini peligro" al=${reiniciarBracket}>✕ ${t("REINICIAR", "RESET")}</${Boton}>` : null}
              <button class="btn suave mini" onClick=${alCerrar}>✕</button>
            </div>
          </div>

          ${!b.activo ? html`
            <div style="display:flex;flex-direction:column;align-items:center;gap:14px;padding:32px 16px;text-align:center">
              <div style="font-size:38px">⚡⚔️</div>
              <div class="disp" style="font-weight:700;font-size:18px">${t("¿Organizar un torneo rápido?", "Host a blitz cup?")}</div>
              <div class="ayuda" style="max-width:440px;font-size:13.5px">
                ${t("Toma a los jugadores y espectadores conectados en la sala, arma un cuadro de Semifinales y Final, y gestiona los mandos con un clic.", "Picks online room members, creates a Semis and Final bracket, and loads seats in one click.")}
              </div>
              <${Boton} tipo="acc lleno" al=${iniciarNuevo}>🚀 ${t("CREAR TORNEO AHORA (4 JUGADORES)", "START CUP (4 PLAYERS)")}</${Boton}>
            </div>`
            : html`
            <div style="display:flex;flex-direction:column;gap:16px">
              ${b.campeon ? html`
                <div style="display:flex;align-items:center;justify-content:center;gap:12px;padding:12px;border-radius:12px;background:linear-gradient(90deg,rgba(255,215,0,0.2),rgba(255,140,0,0.2));border:1px solid #ffd700">
                  <span style="font-size:28px">👑</span>
                  <div>
                    <div style="font-size:11px;font-weight:700;color:#ffd700;letter-spacing:.1em">${t("CAMPEÓN DEL TORNEO", "TOURNAMENT CHAMPION")}</div>
                    <div class="disp" style="font-size:20px;font-weight:700;color:#fff">${b.campeon.nombre}</div>
                  </div>
                </div>` : null}

              <!-- Árbol de llaves (Bracket UI) -->
              <div style="display:grid;grid-template-columns:1fr 1fr;gap:20px;align-items:center">
                <!-- Columna Semis -->
                <div style="display:flex;flex-direction:column;gap:16px">
                  <div class="lab" style="color:var(--acc)">${t("SEMIFINALES", "SEMIFINALS")}</div>
                  ${b.partidos.slice(0, 2).map((p) => html`<${TarjetaPartidoBracket} key=${p.id} p=${p} m=${m}/>`)}
                </div>

                <!-- Columna Final -->
                <div style="display:flex;flex-direction:column;gap:16px">
                  <div class="lab" style="color:#ffd700">🏆 ${t("GRAN FINAL", "GRAND FINAL")}</div>
                  <${TarjetaPartidoBracket} p=${b.partidos[2]} m=${m}/>
                </div>
              </div>
            </div>`}
        </div></div>
      </div>
    </div>`;
}

function TarjetaPartidoBracket({ p, m }) {
  if (!p) return null;
  const listo = p.j1 && p.j2 && !p.ganador;

  return html`
    <div style="display:flex;flex-direction:column;gap:6px;padding:10px 12px;border-radius:10px;background:rgba(0,0,0,0.3);border:1px solid ${p.ganador ? "rgba(0,255,136,0.3)" : "rgba(255,255,255,0.12)"}">
      <div style="display:flex;justify-content:space-between;align-items:center;font-size:11px;color:var(--mut)">
        <span>${p.etiqueta}</span>
        ${p.ganador ? html`<span style="color:#00ff88;font-weight:600">✓ ${t("FINALIZADO", "DONE")}</span>` : null}
      </div>

      <!-- Jugador 1 -->
      <div style=${`display:flex;justify-content:space-between;align-items:center;padding:5px 8px;border-radius:6px;background:${p.ganador === p.j1 ? "rgba(0,255,136,0.15)" : "rgba(255,255,255,0.03)"};border:1px solid ${p.ganador === p.j1 ? "#00ff88" : "transparent"}`}>
        <span style="font-weight:600;font-size:13px">${p.j1 ? p.j1.nombre : "—"}</span>
        ${listo ? html`<button class="btn mini suave" onClick=${() => registrarResultadoBracket({ partidoId: p.id, ganadorLado: "a", m })}>👑 Ganó</button>` : null}
      </div>

      <!-- Jugador 2 -->
      <div style=${`display:flex;justify-content:space-between;align-items:center;padding:5px 8px;border-radius:6px;background:${p.ganador === p.j2 ? "rgba(0,255,136,0.15)" : "rgba(255,255,255,0.03)"};border:1px solid ${p.ganador === p.j2 ? "#00ff88" : "transparent"}`}>
        <span style="font-weight:600;font-size:13px">${p.j2 ? p.j2.nombre : "—"}</span>
        ${listo ? html`<button class="btn mini suave" onClick=${() => registrarResultadoBracket({ partidoId: p.id, ganadorLado: "b", m })}>👑 Ganó</button>` : null}
      </div>

      ${listo ? html`
        <div style="margin-top:4px">
          <${Boton} tipo="mini acc" al=${() => cargarCruceEnPuestos({ partido: p, m })} estilo="width:100%">
            ⚡ ${t("CARGAR CRUCE EN MANDOS", "LOAD MATCH INTO PADS")}
          </${Boton}>
        </div>` : null}
    </div>`;
}
