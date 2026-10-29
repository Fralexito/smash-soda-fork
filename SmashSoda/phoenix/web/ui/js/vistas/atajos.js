// =============================================================================
//  Matriz de Atajos de Moderador / Anfitrión para Phoenix Link
//  Permite ejecutar acciones tácticas y de moderación directamente con teclado
//  sin interrumpir la partida en PES ni depender de menús contextuales.
// =============================================================================

import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton, Icono, Tarjeta } from "../ui.js";
import { obtenerConfigRey, ejecutarRotacionRey } from "./reydelapista.js";

export const LISTA_ATAJOS_MODERADOR = [
  { teclas: "Ctrl + Shift + M", accion: "Silenciar / Activar Chat", desc: "Silencia o reactiva el chat de invitados en toda la sala.", icono: "chat" },
  { teclas: "Ctrl + Shift + F", accion: "Congelar Mandos (Árbitro)", desc: "Bloquea o desbloquea al instante los mandos de los invitados.", icono: "candado" },
  { teclas: "Ctrl + Shift + E", accion: "Todos a Espectador", desc: "Mueve a todos los jugadores a la lista de Mirando.", icono: "usuario" },
  { teclas: "Ctrl + Shift + I", accion: "Invertir Lados (A ⇄ B)", desc: "Intercambia los lados de ambos equipos (para el partido de vuelta).", icono: "intercambio" },
  { teclas: "Ctrl + Shift + R", accion: "Rotar Rey de la Pista", desc: "Fuerza la rotación del rey y sienta a los siguientes retadores.", icono: "trofeo" },
  { teclas: "Ctrl + Shift + S", accion: "Escudo Streamer (Anti-Doxxing)", desc: "Censura o revela IPs, correos y enlaces en vivo.", icono: "candado" },
  { teclas: "Ctrl + Shift + K", accion: "Chuleta de Atajos", desc: "Abre esta ventana de referencia rápida.", icono: "buscar" },
];

/** Hook global que escucha la combinación de teclas en toda la aplicación */
export function useMatrizAtajosModerador(s) {
  const [modalAtajos, setModalAtajos] = useState(false);
  const m = s?.motor;

  useEffect(() => {
    const alPulsar = async (e) => {
      // Requiere Ctrl + Shift
      if (!e.ctrlKey || !e.shiftKey) return;
      const tecla = e.key.toUpperCase();

      // 1. Ctrl + Shift + M -> Silenciar / Activar Chat
      if (tecla === "M") {
        e.preventDefault();
        const silenciado = !m?.sala?.chatSilenciado;
        await accion("sala.silenciarChat", { silenciado }, { silencioso: true });
        avisar(
          silenciado
            ? t("🔇 Chat de la sala SILENCIADO (Ctrl+Shift+M)", "🔇 Room chat MUTED (Ctrl+Shift+M)")
            : t("🔊 Chat de la sala ACTIVADO (Ctrl+Shift+M)", "🔊 Room chat UNMUTED (Ctrl+Shift+M)"),
          "info"
        );
      }

      // 2. Ctrl + Shift + F -> Congelar / Descongelar Mandos (Árbitro)
      else if (tecla === "F") {
        e.preventDefault();
        const bloqueado = !m?.arbitro?.bloqueoActivo;
        await accion("mandos.bloqueo", { activo: bloqueado }, { silencioso: true });
        avisar(
          bloqueado
            ? t("🛑 Mandos de invitados CONGELADOS (Ctrl+Shift+F)", "🛑 Guest controllers FROZEN (Ctrl+Shift+F)")
            : t("▶ Mandos de invitados ACTIVOS (Ctrl+Shift+F)", "▶ Guest controllers ACTIVE (Ctrl+Shift+F)"),
          bloqueado ? "warn" : "ok"
        );
      }

      // 3. Ctrl + Shift + E -> Mandar a todos a espectador
      else if (tecla === "E") {
        e.preventDefault();
        await accion("mandos.todosAMirar", {}, { silencioso: true });
        avisar(t("👥 Todos los jugadores movidos a espectadores (Ctrl+Shift+E)", "👥 All players moved to spectators (Ctrl+Shift+E)"), "info");
      }

      // 4. Ctrl + Shift + I -> Invertir lados completos (A ⇄ B)
      else if (tecla === "I") {
        e.preventDefault();
        const lista = m?.mandos?.lista || [];
        for (const p of lista) {
          if (p.equipo === "a") await accion("mandos.equipo", { indice: p.indice, equipo: "b" }, { silencioso: true });
          else if (p.equipo === "b") await accion("mandos.equipo", { indice: p.indice, equipo: "a" }, { silencioso: true });
        }
        avisar(t("⚡ Lados intercambiados: Equipo A ⇄ Equipo B (Ctrl+Shift+I)", "⚡ Swapped sides: Team A ⇄ Team B (Ctrl+Shift+I)"), "ok");
      }

      // 5. Ctrl + Shift + R -> Rotar Rey de la Pista
      else if (tecla === "R") {
        e.preventDefault();
        const cfg = obtenerConfigRey();
        if (cfg.activo) {
          await ejecutarRotacionRey({ ganador: cfg.reyActual || "a", m, forzar: true });
        } else {
          avisar(t("👑 El Modo Rey de la Pista está apagado. Actívalo en Puestos.", "👑 King of the Hill is off. Turn it on in Seats."), "warn");
        }
      }

      // 6. Ctrl + Shift + K -> Chuleta de Atajos
      else if (tecla === "K") {
        e.preventDefault();
        setModalAtajos((v) => !v);
      }
    };

    window.addEventListener("keydown", alPulsar);
    return () => window.removeEventListener("keydown", alPulsar);
  }, [m]);

  return { modalAtajos, setModalAtajos };
}

/** Modal visual de referencia rápida de atajos de moderador */
export function ModalAtajosModerador({ alCerrar }) {
  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()} style="position:fixed;inset:0;background:rgba(2,10,18,0.85);backdrop-filter:blur(10px);z-index:9999;display:flex;align-items:center;justify-content:center;padding:20px">
      <div class="tarjeta cut" style="width:min(620px,100%);background:var(--fondo-tarjeta);border:1px solid rgba(0,229,255,0.35);box-shadow:0 0 35px rgba(0,229,255,0.15);border-radius:16px;padding:26px 28px;display:flex;flex-direction:column;gap:18px">
        
        <div style="display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid rgba(255,255,255,0.08);padding-bottom:14px">
          <div style="display:flex;align-items:center;gap:10px">
            <span style="font-size:24px">⌨️</span>
            <div>
              <div class="disp" style="font-size:16px;font-weight:700;color:var(--txt)">${t("ATAJOS DE MODERADOR Y ANFITRIÓN", "HOST & MODERATOR HOTKEYS")}</div>
              <div class="mono mut" style="font-size:11px">${t("Acciones tácticas rápidas sin cambiar de ventana", "Quick tactical actions without switching windows")}</div>
            </div>
          </div>
          <button onClick=${alCerrar} style="background:none;border:none;color:var(--mut);font-size:20px;cursor:pointer;padding:4px 8px">✕</button>
        </div>

        <div style="display:flex;flex-direction:column;gap:10px">
          ${LISTA_ATAJOS_MODERADOR.map((a) => html`
            <div class="caja" key=${a.teclas} style="display:flex;align-items:center;justify-content:space-between;gap:14px;padding:12px 16px;background:rgba(0,0,0,0.35);border:1px solid rgba(255,255,255,0.06);border-radius:10px">
              <div style="display:flex;flex-direction:column;gap:3px">
                <div style="font-weight:700;font-size:13px;color:#fff">${t(a.accion, a.accion)}</div>
                <div class="mut" style="font-size:11.5px">${t(a.desc, a.desc)}</div>
              </div>
              <div class="mono" style="padding:4px 10px;background:rgba(0,229,255,0.12);border:1px solid rgba(0,229,255,0.35);border-radius:6px;color:var(--acc);font-weight:700;font-size:12px;white-space:nowrap">
                ${a.teclas}
              </div>
            </div>
          `)}
        </div>

        <div style="display:flex;justify-content:flex-end;margin-top:6px">
          <${Boton} tipo="suave" al=${alCerrar}>${t("ENTENDIDO", "GOT IT")}</${Boton}>
        </div>

      </div>
    </div>
  `;
}
