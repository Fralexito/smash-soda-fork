// =============================================================================
//  Phoenix Link · La Rueda de Penitencias del Perdedor (El Castigo de la Sala)
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Ruleta animada con Canvas y física de desaceleración para aplicar castigos
//  divertidos a quienes pierden en partidos o en el Rey de la Pista.
// =============================================================================

import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { Boton } from "../ui.js";
import { sonarSilbato, narrar } from "./locutor.js";
import { notificarPenitenciaDiscord } from "../servicios/discord.js";

const CLAVE_PENITENCIAS = "phoenix.penitencias_lista";

const CASTIGOS_DEFECTO = [
  "🐧 Foto de perfil de Magallanes en Discord por 24h",
  "🎤 Cantar el himno nacional con micro abierto",
  "🤚 Jugar el siguiente partido con una sola mano",
  "👶 Cambiar apodo a 'Hijo del Ganador' hoy",
  "☕ Invitar un café / refresco simbólico",
  "🎙️ Relatar el siguiente partido como comentarista",
  "🧎‍♂️ Declarar lealtad al Rey de la Pista en el chat",
  "🛡️ Jugar con defensa de delanteros en el próximo duelo",
];

export function obtenerCastigos() {
  try {
    const raw = localStorage.getItem(CLAVE_PENITENCIAS);
    return raw ? JSON.parse(raw) : CASTIGOS_DEFECTO;
  } catch {
    return CASTIGOS_DEFECTO;
  }
}

export function guardarCastigos(lista) {
  try {
    localStorage.setItem(CLAVE_PENITENCIAS, JSON.stringify(lista));
  } catch (e) {
    console.error("[penitencias] Error guardando:", e);
  }
}

export function ModalRuletaPenitencias({ perdedor = "El perdedor", m, alCerrar }) {
  const [castigos, setCastigos] = useState(obtenerCastigos);
  const [girando, setGirando] = useState(false);
  const [resultado, setResultado] = useState(null);
  const [nuevoCastigo, setNuevoCastigo] = useState("");
  const canvasRef = useRef(null);
  const anguloRef = useRef(0);

  // Dibujar ruleta en Canvas
  const dibujarRuleta = (angulo) => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    const centroX = canvas.width / 2;
    const centroY = canvas.height / 2;
    const radio = centroX - 16;
    const n = castigos.length;
    const paso = (Math.PI * 2) / n;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Colores tácticos alternados
    const colores = ["#00f0ff", "#ff0055", "#b300ff", "#ffd700", "#00ff88", "#ff7700", "#3388ff", "#e024c3"];

    for (let i = 0; i < n; i++) {
      const inicio = angulo + i * paso;
      const fin = inicio + paso;

      ctx.beginPath();
      ctx.moveTo(centroX, centroY);
      ctx.arc(centroX, centroY, radio, inicio, fin);
      ctx.fillStyle = colores[i % colores.length];
      ctx.fill();
      ctx.lineWidth = 2;
      ctx.strokeStyle = "rgba(0,0,0,0.4)";
      ctx.stroke();

      // Texto de cada gajo
      ctx.save();
      ctx.translate(centroX, centroY);
      ctx.rotate(inicio + paso / 2);
      ctx.textAlign = "right";
      ctx.fillStyle = "#ffffff";
      ctx.font = "bold 11.5px sans-serif";
      ctx.shadowColor = "rgba(0,0,0,0.8)";
      ctx.shadowBlur = 4;
      const textoCorto = castigos[i].length > 24 ? castigos[i].substring(0, 22) + "…" : castigos[i];
      ctx.fillText(textoCorto, radio - 18, 4);
      ctx.restore();
    }

    // Centro brillante
    ctx.beginPath();
    ctx.arc(centroX, centroY, 22, 0, Math.PI * 2);
    ctx.fillStyle = "#11141c";
    ctx.fill();
    ctx.lineWidth = 3;
    ctx.strokeStyle = "#ffd700";
    ctx.stroke();

    // Flecha indicadora superior
    ctx.beginPath();
    ctx.moveTo(centroX - 12, 10);
    ctx.lineTo(centroX + 12, 10);
    ctx.lineTo(centroX, 30);
    ctx.fillStyle = "#ffd700";
    ctx.fill();
    ctx.stroke();
  };

  useEffect(() => {
    dibujarRuleta(anguloRef.current);
  }, [castigos]);

  const girar = () => {
    if (girando) return;
    setGirando(true);
    setResultado(null);
    sonarSilbato({ largo: false });

    const girosCompletos = 5 + Math.random() * 5;
    const duracion = 4200; // 4.2 segundos de giro
    const inicio = performance.now();
    const anguloInicial = anguloRef.current;
    const anguloTotal = anguloInicial + girosCompletos * Math.PI * 2;

    const animar = (ahora) => {
      const tiempoTranscurrido = ahora - inicio;
      const progreso = Math.min(1, tiempoTranscurrido / duracion);
      // Desaceleración cúbica ease-out
      const factor = 1 - Math.pow(1 - progreso, 3);
      const anguloActual = anguloInicial + (anguloTotal - anguloInicial) * factor;
      anguloRef.current = anguloActual;
      dibujarRuleta(anguloActual);

      if (progreso < 1) {
        requestAnimationFrame(animar);
      } else {
        setGirando(false);
        // Calcular en qué gajo cayó la flecha superior (ángulo -PI/2)
        const n = castigos.length;
        const paso = (Math.PI * 2) / n;
        const normalizado = (Math.PI * 1.5 - (anguloActual % (Math.PI * 2)) + Math.PI * 2) % (Math.PI * 2);
        const indexGajo = Math.floor(normalizado / paso) % n;
        const elegido = castigos[indexGajo];
        setResultado(elegido);

        narrar(`¡La ruleta ha hablado! La penitencia para ${perdedor} es: ${elegido}`);
        if (m?.sala?.abierta) {
          accion("chat.enviar", {
            texto: `🎡 ¡LA RUEDA DEL CASTIGO! El perdedor (${perdedor}) debe cumplir: ${elegido}`,
          }, { silencioso: true });
        }
        try {
          notificarPenitenciaDiscord({ perdedor, penitencia: elegido });
        } catch (e) {}
      }
    };
    requestAnimationFrame(animar);
  };

  const agregarCastigo = () => {
    if (!nuevoCastigo.trim()) return;
    const nueva = [...castigos, nuevoCastigo.trim()];
    setCastigos(nueva);
    guardarCastigos(nueva);
    setNuevoCastigo("");
  };

  return html`
    <div class="velo" onClick=${(e) => e.target === e.currentTarget && alCerrar()}>
      <div class="dialogo" style="max-width:560px;width:95%">
        <div class="cut"><div class="in" style="padding:24px 28px;display:flex;flex-direction:column;gap:14px;align-items:center">
          
          <div style="display:flex;justify-content:space-between;align-items:center;width:100%">
            <div style="display:flex;align-items:center;gap:10px">
              <span style="font-size:26px">🎡</span>
              <div>
                <h2 style="margin:0;font-size:18px">${t("La Ruleta del Castigo", "Punishment Wheel")}</h2>
                <div class="ayuda" style="font-size:12px">${t(`Penitencia para el perdedor (${perdedor})`, `Punishment for the loser (${perdedor})`)}</div>
              </div>
            </div>
            <button class="btn suave mini" onClick=${alCerrar}>✕</button>
          </div>

          <!-- Canvas interactivo de la ruleta -->
          <div style="position:relative;margin:10px 0">
            <canvas ref=${canvasRef} width="360" height="360" style="border-radius:50%;box-shadow:0 0 30px rgba(0,240,255,0.2)"></canvas>
          </div>

          ${resultado ? html`
            <div style="padding:12px 16px;border-radius:10px;background:rgba(255,0,85,0.15);border:1.5px dashed #ff0055;text-align:center;width:100%">
              <div style="font-size:12px;font-weight:700;color:#ff0055;letter-spacing:.1em">${t("CASTIGO CONFIRMADO:", "CONFIRMED PUNISHMENT:")}</div>
              <div class="disp" style="font-size:16px;font-weight:700;color:#fff;margin-top:4px">${resultado}</div>
            </div>` : null}

          <div style="display:flex;gap:10px;width:100%;margin-top:4px">
            <${Boton} tipo="acc lleno" al=${girar} deshabilitado=${girando} estilo="flex:1;height:44px;font-size:14px">
              ${girando ? t("GIRANDO...", "SPINNING...") : `🎡 ${t("¡GIRAR RULETA!", "SPIN WHEEL!")}`}
            </${Boton}>
          </div>

          <!-- Editor rápido de castigos -->
          <div style="display:flex;gap:6px;width:100%;margin-top:6px">
            <input class="campo" value=${nuevoCastigo} onInput=${(e) => setNuevoCastigo(e.target.value)}
              placeholder=${t("Añadir penitencia personalizada...", "Add custom punishment...")} style="flex:1"/>
            <${Boton} tipo="mini suave" al=${agregarCastigo}>+ ${t("Añadir", "Add")}</${Boton}>
          </div>

        </div></div>
      </div>
    </div>`;
}

