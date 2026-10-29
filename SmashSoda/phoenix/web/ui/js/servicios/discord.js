// ==============================================================================
// Phoenix Link — Notificador Discord Webhook en Vivo
// Autor: Antigravity (Gemini) · Google DeepMind
// Fecha: 29 de Octubre de 2026
//
// Permite transmitir resultados de partidos, veredictos de VAR, ruleta de
// penitencias y eventos clave directamente al servidor de Discord de la comunidad.
// ==============================================================================

const CLAVE_WEBHOOK = "phoenix.discord_webhook";

export function obtenerWebhookDiscord() {
  try {
    return localStorage.getItem(CLAVE_WEBHOOK) || "";
  } catch {
    return "";
  }
}

export function guardarWebhookDiscord(url) {
  try {
    localStorage.setItem(CLAVE_WEBHOOK, (url || "").trim());
    return true;
  } catch {
    return false;
  }
}

/**
 * Envía un Embed rico a Discord mediante Webhook.
 */
export async function enviarEmbedDiscord({ titulo, descripcion, color = 0x00f0ff, campos = [], pie = "Phoenix Link · Esports & Street Mode" }) {
  const url = obtenerWebhookDiscord();
  if (!url || !url.startsWith("https://discord.com/api/webhooks/")) {
    return { ok: false, motivo: "Webhook no configurado o inválido" };
  }

  const payload = {
    username: "Phoenix Bot Esports",
    avatar_url: "https://raw.githubusercontent.com/Fralexito/pes-liga-automatica/main/public/favicon.ico",
    embeds: [
      {
        title: titulo,
        description: descripcion,
        color: color,
        fields: campos,
        footer: { text: pie },
        timestamp: new Date().toISOString()
      }
    ]
  };

  try {
    const res = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    return { ok: res.ok, status: res.status };
  } catch (err) {
    console.error("[discord-webhook] Error al enviar embed:", err);
    return { ok: false, motivo: err.message };
  }
}

/**
 * Notifica el final de un partido a Discord
 */
export async function notificarFinalPartidoDiscord({ equipoA, equipoB, golesA, golesB, ganador, esStreet = false, mvp = null }) {
  const color = ganador === "empate" ? 0xaaaaaa : (ganador === "a" ? 0x00f0ff : 0xb300ff);
  const modoTexto = esStreet ? "🏟️ MODO STREET (Futsal Callejero 15x10)" : "⚽ PARTIDO OFICIAL";
  const titulo = `🏆 ¡FINAL DEL PARTIDO! · ${golesA} - ${golesB}`;
  const resTexto = ganador === "empate" 
    ? `¡Empate electrizante entre **${equipoA}** y **${equipoB}**!` 
    : `¡Victoria indiscutida de **${ganador === "a" ? equipoA : equipoB}**!`;

  const campos = [
    { name: "Equipo Local", value: `**${equipoA}** (${golesA})`, inline: true },
    { name: "Equipo Visitante", value: `**${equipoB}** (${golesB})`, inline: true },
    { name: "Modalidad", value: modoTexto, inline: false }
  ];

  if (mvp) {
    campos.push({ name: "⭐ Jugador MVP", value: `**${mvp}**`, inline: true });
  }

  return enviarEmbedDiscord({
    titulo,
    descripcion: resTexto,
    color,
    campos
  });
}

/**
 * Notifica una resolución de VAR a Discord
 */
export async function notificarVARDiscord({ veredicto, votosA, votosX, votosB, totalVotos }) {
  let color = 0xffaa00;
  let icono = "🚨";
  if (veredicto.includes("PENAL")) { color = 0x00ff88; icono = "⚽"; }
  else if (veredicto.includes("ROJA")) { color = 0xff0044; icono = "🟥"; }
  else { color = 0x888888; icono = "🌊"; }

  return enviarEmbedDiscord({
    titulo: `${icono} RESOLUCIÓN DEL VAR EN VIVO: ${veredicto}`,
    descripcion: `La sala y los espectadores han dictado sentencia unánime tras la revisión.`,
    color,
    campos: [
      { name: "⚽ Penal", value: `${votosA} votos`, inline: true },
      { name: "🌊 Piscinazo / Siga", value: `${votosX} votos`, inline: true },
      { name: "🟥 Roja Directa", value: `${votosB} votos`, inline: true },
      { name: "Total Votantes", value: `${totalVotos} espectadores`, inline: false }
    ]
  });
}

/**
 * Notifica una penitencia asignada a Discord
 */
export async function notificarPenitenciaDiscord({ perdedor, penitencia }) {
  return enviarEmbedDiscord({
    titulo: `🎡 ¡LA RULETA DEL CASTIGO HA HABLADO!`,
    descripcion: `El jugador derrotado **${perdedor}** ha recibido su penitencia en vivo ante toda la sala:`,
    color: 0xff0055,
    campos: [
      { name: "Jugador Sentenciado", value: `💀 **${perdedor}**`, inline: true },
      { name: "Castigo Obligatorio", value: `🎯 **${penitencia}**`, inline: true }
    ],
    pie: "Phoenix Link · Tribunal de la Sala"
  });
}
