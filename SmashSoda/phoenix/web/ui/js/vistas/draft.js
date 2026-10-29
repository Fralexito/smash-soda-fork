// =============================================================================
//  Phoenix Link · Modo «Pan y Queso» / Draft Callejero por Turnos
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Dos capitanes eligen jugadores por turnos como en el potrero y sienta a
//  ambos equipos en los puestos correspondientes de forma automática.
// =============================================================================

import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton, Avatar } from "../ui.js";
import { sonarSilbato, narrar } from "./locutor.js";

export function ModalDraftPanYQueso({ m, alCerrar }) {
  const listaMiembros = [];
  if (m?.sala?.hostId) listaMiembros.push({ nombre: m.sala.hostNombre || "Host", parsecId: m.sala.hostId, host: true });
  for (const inv of m?.invitados || []) {
    if (!inv.falso && !listaMiembros.some((x) => x.parsecId === inv.parsecId)) {
      listaMiembros.push({ nombre: inv.nombre, parsecId: inv.parsecId, host: false });
    }
  }

  const [capA, setCapA] = useState(listaMiembros[0] || null);
  const [capB, setCapB] = useState(listaMiembros[1] || null);
  const [equipoA, setEquipoA] = useState([]);
  const [equipoB, setEquipoB] = useState([]);
  const [turnoA, setTurnoA] = useState(true);

  // Pool de jugadores disponibles
  const elegidosIds = new Set([capA?.parsecId, capB?.parsecId, ...equipoA.map((x) => x.parsecId), ...equipoB.map((x) => x.parsecId)]);
  const disponibles = listaMiembros.filter((j) => !elegidosIds.has(j.parsecId));

  const elegirJugador = (j) => {
    sonarSilbato({ largo: false });
    if (turnoA) {
      setEquipoA([...equipoA, j]);
      setTurnoA(false);
      narrar(`${capA.nombre} elige a ${j.nombre}`);
    } else {
      setEquipoB([...equipoB, j]);
      setTurnoA(true);
      narrar(`${capB.nombre} elige a ${j.nombre}`);
    }
  };

  const aplicarAMandos = async () => {
    const listA = [capA, ...equipoA].filter(Boolean);
    const listB = [capB, ...equipoB].filter(Boolean);

    // Ajustar formación al tamaño máximo
    const maxK = Math.max(listA.length, listB.length);
    await accion("mandos.formacion", { local: maxK, visitante: maxK }, { silencioso: true });

    // Asignar Equipo A (índices 0 en adelante)
    for (let i = 0; i < listA.length; i++) {
      const j = listA[i];
      if (j.host) await accion("mandos.tomar", { numero: i + 1 }, { silencioso: true });
      else await accion("mandos.asignar", { indice: i, parsecId: j.parsecId }, { silencioso: true });
    }

    // Asignar Equipo B (índices maxK en adelante)
    for (let i = 0; i < listB.length; i++) {
      const j = listB[i];
      const indicePuesto = maxK + i;
      if (j.host) await accion("mandos.tomar", { numero: indicePuesto + 1 }, { silencioso: true });
      else await accion("mandos.asignar", { indice: indicePuesto, parsecId: j.parsecId }, { silencioso: true });
    }

    narrar("¡Equipos armados en el draft callejero! ¡A la cancha!", { prioridad: true });
    if (m?.sala?.abierta) {
      accion("chat.enviar", {
        texto: `⚽ ¡DRAFT DE BARRIO FINALIZADO! Equipo A (${listA.map((x) => x.nombre).join(", ")}) vs Equipo B (${listB.map((x) => x.nombre).join(", ")})`,
      }, { silencioso: true });
    }
    avisar(t("⚽ Equipos del draft asignados a los mandos", "⚽ Draft teams seated into pads"), "ok");
    alCerrar();
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:640px;width:95%">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:14px">
          
          <div style="display:flex;justify-content:space-between;align-items:center">
            <div style="display:flex;align-items:center;gap:10px">
              <span style="font-size:26px">🤝⚽</span>
              <div>
                <h2 style="margin:0;font-size:18px">${t("Draft Callejero (Pan y Queso)", "Street Draft (Pick Teams)")}</h2>
                <div class="ayuda" style="font-size:12px">${t("Los capitanes eligen jugadores por turnos y van directo a la cancha.", "Captains pick players in turns and sit them directly.")}</div>
              </div>
            </div>
            <button class="btn suave mini" onClick=${alCerrar}>✕</button>
          </div>

          <!-- Indicador de turno -->
          <div style="padding:8px 12px;border-radius:8px;background:${turnoA ? "rgba(0,240,255,0.15)" : "rgba(179,0,255,0.15)"};border:1px solid ${turnoA ? "#00f0ff" : "#b300ff"};text-align:center">
            <span style="font-weight:700;font-size:13px;color:${turnoA ? "#00f0ff" : "#d880ff"}">
              👉 ${t("TURNO DE ELECCIÓN:", "PICK TURN:")} ${turnoA ? capA?.nombre || "Capitán A" : capB?.nombre || "Capitán B"}
            </span>
          </div>

          <!-- Columnas de equipos A y B -->
          <div style="display:grid;grid-template-columns:1fr 1fr;gap:14px">
            <!-- Equipo A -->
            <div style="padding:10px 12px;border-radius:8px;background:rgba(0,240,255,0.05);border:1px solid rgba(0,240,255,0.3)">
              <div style="font-weight:700;font-size:12px;color:var(--acc);margin-bottom:6px">EQUIPO A (${[capA, ...equipoA].filter(Boolean).length})</div>
              <div style="font-size:13px;font-weight:600">👑 ${capA?.nombre || "—"} <span class="ayuda">(Capitán)</span></div>
              ${equipoA.map((x) => html`<div style="font-size:12.5px;color:#ccc;margin-top:3px">• ${x.nombre}</div>`)}
            </div>

            <!-- Equipo B -->
            <div style="padding:10px 12px;border-radius:8px;background:rgba(179,0,255,0.05);border:1px solid rgba(179,0,255,0.3)">
              <div style="font-weight:700;font-size:12px;color:#d880ff;margin-bottom:6px">EQUIPO B (${[capB, ...equipoB].filter(Boolean).length})</div>
              <div style="font-size:13px;font-weight:600">👑 ${capB?.nombre || "—"} <span class="ayuda">(Capitán)</span></div>
              ${equipoB.map((x) => html`<div style="font-size:12.5px;color:#ccc;margin-top:3px">• ${x.nombre}</div>`)}
            </div>
          </div>

          <!-- Jugadores disponibles para elegir -->
          <div style="display:flex;flex-direction:column;gap:6px;margin-top:6px">
            <div class="lab" style="font-size:11px">${t("JUGADORES DISPONIBLES EN LA SALA", "AVAILABLE PLAYERS IN ROOM")}</div>
            <div style="display:flex;gap:8px;flex-wrap:wrap">
              ${disponibles.map((j) => html`
                <button class="btn suave mini" style="display:flex;align-items:center;gap:6px;padding:6px 10px" onClick=${() => elegirJugador(j)}>
                  <span>👤 ${j.nombre}</span>
                  <b style="color:var(--acc)">+ Elegir</b>
                </button>
              `)}
              ${disponibles.length === 0 ? html`<div class="ayuda">${t("No quedan más jugadores en la lista.", "No more players in list.")}</div>` : null}
            </div>
          </div>

          <div class="botones" style="margin-top:10px;display:flex;justify-content:space-between;gap:8px">
            <button class="btn suave" onClick=${alCerrar}>${t("Cancelar", "Cancel")}</button>
            <${Boton} tipo="acc lleno" al=${aplicarAMandos}>⚡ ${t("ASIGNAR A MANDOS Y JUGAR", "SEAT INTO PADS & PLAY")}</${Boton}>
          </div>

        </div></div>
      </div>
    </div>`;
}
