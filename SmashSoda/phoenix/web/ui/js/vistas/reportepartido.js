// =============================================================================
//  Ficha MVP y Acta Oficial de Partido para WhatsApp / Discord (Esports)
//  Calcula el Jugador del Partido (MVP), genera el reporte oficial con formato
//  para redes/mensajería y permite copiarlo con un solo clic.
// =============================================================================

import { html, useState } from "../lib.js";
import { t, duracion, fecha } from "../i18n.js";
import { avisar } from "../tienda.js";
import { Boton, Icono, Tarjeta, Avatar } from "../ui.js";

/** Calcula el jugador MVP del partido según goles, resultado y aportes */
export function calcularMvp(r) {
  if (!r) return null;
  const jugadoresA = r.a?.jugadores || [];
  const jugadoresB = r.b?.jugadores || [];
  const ganador = r.ganador; // "a" | "b" | "empate"
  const golesA = Number(r.a?.goles) || 0;
  const golesB = Number(r.b?.goles) || 0;

  let mejor = null;
  let maxScore = -1;

  const evaluar = (jugador, equipo, golesEquipo, esGanador) => {
    const nombre = jugador.nombre || jugador.jugador || t("Jugador", "Player");
    let score = 5.0;
    if (esGanador) score += 2.5;
    else if (ganador === "empate") score += 1.0;

    // Ponderación de goles de equipo divididos entre alineación
    const golesEstimados = Math.max(1, Math.round(golesEquipo / Math.max(1, (equipo === "a" ? jugadoresA : jugadoresB).length)));
    score += golesEstimados * 1.5;

    if (score > maxScore) {
      maxScore = score;
      mejor = {
        nombre,
        equipo: equipo.toUpperCase(),
        goles: golesEstimados,
        rating: Math.min(9.9, (score).toFixed(1)),
        esGanador,
      };
    }
  };

  jugadoresA.forEach((j) => evaluar(j, "a", golesA, ganador === "a"));
  jugadoresB.forEach((j) => evaluar(j, "b", golesB, ganador === "b"));

  if (!mejor && jugadoresA.length > 0) {
    mejor = { nombre: jugadoresA[0].nombre || "Jugador", equipo: "A", goles: golesA, rating: "8.5", esGanador: ganador === "a" };
  }

  return mejor;
}

/** Genera texto con formato oficial para copiar y pegar en WhatsApp o Discord */
export function generarTextoActa(r, mvp) {
  if (!r) return "";
  const eqA = r.a?.nombre || "Equipo A";
  const eqB = r.b?.nombre || "Equipo B";
  const golesA = r.a?.goles ?? 0;
  const golesB = r.b?.goles ?? 0;
  const fechaStr = r.inicioMs ? new Date(r.inicioMs).toLocaleDateString() : new Date().toLocaleDateString();
  const duracionStr = r.duracionSeg ? duracion(r.duracionSeg) : "90 min";

  const alineacionA = (r.a?.jugadores || []).map((j) => j.nombre || j.jugador).filter(Boolean).join(", ") || "Sin registrar";
  const alineacionB = (r.b?.jugadores || []).map((j) => j.nombre || j.jugador).filter(Boolean).join(", ") || "Sin registrar";

  let resultadoTexto = "";
  if (r.ganador === "a") resultadoTexto = `🏆 Ganador: ${eqA}`;
  else if (r.ganador === "b") resultadoTexto = `🏆 Ganador: ${eqB}`;
  else resultadoTexto = `🤝 Empate`;

  return [
    `🏆 *ACTA OFICIAL DE PARTIDO · PHOENIX LEAGUE*`,
    `━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`,
    `⚽ *${eqA}*  ${golesA} - ${golesB}  *${eqB}*`,
    resultadoTexto,
    mvp ? `🌟 *MVP DEL PARTIDO:* ${mvp.nombre} (Equipo ${mvp.equipo}) · Rating ${mvp.rating}` : "",
    `⏱ *Duración:* ${duracionStr} · 📅 *Fecha:* ${fechaStr}`,
    ``,
    `👥 *Alineaciones:*`,
    `🔵 *${eqA}:* ${alineacionA}`,
    `🟣 *${eqB}:* ${alineacionB}`,
    `━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`,
    `🎮 *Verificado en tiempo real con Phoenix Link v0.3.58*`,
  ].filter(Boolean).join("\n");
}

/** Modal visual esport con detalles del partido y botón de exportar reporte */
export function FichaPartidoModal({ r, alCerrar }) {
  if (!r) return null;
  const [copiado, setCopiado] = useState(false);
  const mvp = calcularMvp(r);
  const actaTexto = generarTextoActa(r, mvp);

  const copiarActa = async () => {
    try {
      await navigator.clipboard.writeText(actaTexto);
      setCopiado(true);
      avisar(t("✓ ¡Acta oficial copiada! Lista para pegar en WhatsApp o Discord.", "✓ Match report copied! Ready to paste into WhatsApp or Discord."), "ok", 4000);
      setTimeout(() => setCopiado(false), 3000);
    } catch {
      avisar(t("No se pudo copiar automáticamente.", "Could not copy automatically."), "warn");
    }
  };

  const gana = r.ganador === "empate" ? t("Empate", "Draw") : t(`Ganó ${r.ganador === "a" ? r.a.nombre : r.b.nombre}`, `${r.ganador === "a" ? r.a.nombre : r.b.nombre} won`);

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()} style="position:fixed;inset:0;background:rgba(2,10,18,0.85);backdrop-filter:blur(10px);z-index:9999;display:flex;align-items:center;justify-content:center;padding:20px">
      <div class="tarjeta cut" style="width:min(580px,100%);max-height:90vh;overflow-y:auto;background:var(--fondo-tarjeta);border:1px solid rgba(255,184,0,0.35);box-shadow:0 0 35px rgba(255,184,0,0.15);border-radius:16px;padding:26px 28px;display:flex;flex-direction:column;gap:18px">
        
        <!-- Cabecera del Acta -->
        <div style="display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid rgba(255,255,255,0.08);padding-bottom:14px">
          <div style="display:flex;align-items:center;gap:10px">
            <span style="font-size:24px">🏆</span>
            <div>
              <div class="disp" style="font-size:16px;font-weight:700;color:var(--txt)">${t("ACTA OFICIAL DE PARTIDO", "OFFICIAL MATCH REPORT")}</div>
              <div class="mono mut" style="font-size:11px">${fecha(r.inicioMs)} · ${duracion(r.duracionSeg)}</div>
            </div>
          </div>
          <button onClick=${alCerrar} style="background:none;border:none;color:var(--mut);font-size:20px;cursor:pointer;padding:4px 8px">✕</button>
        </div>

        <!-- Marcador Central -->
        <div style="display:flex;align-items:center;justify-content:space-around;padding:16px 20px;background:rgba(0,0,0,0.4);border-radius:12px;border:1px solid rgba(255,255,255,0.06)">
          <div style="text-align:right;flex:1">
            <div class="disp" style="font-weight:700;font-size:20px;color:var(--acc)">${r.a?.nombre || "Equipo A"}</div>
            <div class="mono mut" style="font-size:11px">${(r.a?.jugadores || []).map(j => j.nombre).join(", ") || "Equipo A"}</div>
          </div>
          <div style="display:flex;align-items:center;gap:14px;padding:0 20px">
            <span class="disp" style="font-size:42px;font-weight:900;color:#fff">${r.a?.goles ?? 0}</span>
            <span class="disp mut" style="font-size:20px;font-weight:700">:</span>
            <span class="disp" style="font-size:42px;font-weight:900;color:#fff">${r.b?.goles ?? 0}</span>
          </div>
          <div style="text-align:left;flex:1">
            <div class="disp" style="font-weight:700;font-size:20px;color:var(--eq-b)">${r.b?.nombre || "Equipo B"}</div>
            <div class="mono mut" style="font-size:11px">${(r.b?.jugadores || []).map(j => j.nombre).join(", ") || "Equipo B"}</div>
          </div>
        </div>

        <!-- Tarjeta Dorada MVP -->
        ${mvp ? html`
          <div style="display:flex;align-items:center;justify-content:space-between;gap:14px;padding:14px 18px;background:linear-gradient(135deg,rgba(255,184,0,0.18),rgba(255,136,0,0.06));border:1px solid #ffb800;border-radius:12px">
            <div style="display:flex;align-items:center;gap:12px">
              <span style="font-size:32px">🌟</span>
              <div>
                <div class="mono" style="font-size:10px;font-weight:700;letter-spacing:0.12em;color:#ffb800">${t("MVP DEL PARTIDO", "MAN OF THE MATCH")}</div>
                <div class="disp" style="font-size:18px;font-weight:800;color:#fff">${mvp.nombre}</div>
                <div class="mut" style="font-size:11px">${t("Equipo", "Team")} ${mvp.equipo} · ${t("Aporte decisivo en el resultado", "Decisive match contribution")}</div>
              </div>
            </div>
            <div style="text-align:center;padding:6px 14px;background:rgba(0,0,0,0.4);border-radius:8px;border:1px solid rgba(255,184,0,0.4)">
              <div class="mono mut" style="font-size:9px">${t("VALORACIÓN", "RATING")}</div>
              <div class="disp" style="font-size:22px;font-weight:900;color:#ffb800">${mvp.rating}</div>
            </div>
          </div>
        ` : null}

        <!-- Vista Previa del Acta para WhatsApp / Discord -->
        <div style="display:flex;flex-direction:column;gap:6px">
          <div class="mono mut" style="font-size:11px">${t("REPORTE LISTO PARA COMPARTIR EN GRUPOS:", "READY-TO-SHARE MATCH REPORT:")}</div>
          <div class="mono" style="padding:12px 14px;background:rgba(0,0,0,0.5);border-radius:8px;font-size:11.5px;color:#d0d7de;line-height:1.5;max-height:140px;overflow-y:auto;white-space:pre-wrap;border:1px solid rgba(255,255,255,0.06);user-select:all">
            ${actaTexto}
          </div>
        </div>

        <!-- Botones de Acción -->
        <div style="display:flex;justify-content:flex-end;gap:10px;margin-top:6px;flex-wrap:wrap">
          <${Boton} tipo="suave" al=${alCerrar}>${t("CERRAR", "CLOSE")}</${Boton}>
          <${Boton} tipo="ok" al=${copiarActa}>
            <${Icono} n="copiar" t=${15}/>
            ${copiado ? t("✓ ¡COPIADO!", "✓ COPIED!") : t("COPIAR PARA WHATSAPP / DISCORD", "COPY FOR WHATSAPP / DISCORD")}
          </${Boton}>
        </div>

      </div>
    </div>
  `;
}

