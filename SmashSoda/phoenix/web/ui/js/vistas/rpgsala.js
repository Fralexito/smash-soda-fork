// =============================================================================
//  Phoenix Link · Sistema RPG de Sala: Rangos, XP, Niveles y Títulos Legendarios
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Otorga progresión y prestigio a los miembros de la sala por cada partido.
// =============================================================================

import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { Boton } from "../ui.js";

const CLAVE_RPG = "phoenix.rpg_perfiles";

export function obtenerTablaRPG() {
  try {
    const raw = localStorage.getItem(CLAVE_RPG);
    return raw ? JSON.parse(raw) : {};
  } catch {
    return {};
  }
}

export function guardarTablaRPG(tabla) {
  try {
    localStorage.setItem(CLAVE_RPG, JSON.stringify(tabla));
    window.dispatchEvent(new CustomEvent("phoenix:rpg_evento", { detail: tabla }));
  } catch (e) {}
}

export function obtenerPerfilRPG(nombre) {
  const t = obtenerTablaRPG();
  const def = {
    xp: 0,
    nivel: 1,
    partidos: 0,
    victorias: 0,
    goles: 0,
    huachas: 0,
    insignias: [],
  };
  return t[nombre] || def;
}

export function calcularRango(nivel) {
  if (nivel >= 35) return { titulo: "👑 Leyenda Inmortal", color: "#ffd700", marco: "0 0 10px #ffd700" };
  if (nivel >= 20) return { titulo: "💎 Crack de la Sala", color: "#00f0ff", marco: "0 0 8px #00f0ff" };
  if (nivel >= 10) return { titulo: "🥇 Capitán y Caudillo", color: "#b300ff", marco: "0 0 6px #b300ff" };
  if (nivel >= 5) return { titulo: "🥈 Titular de Primera", color: "#00ff88", marco: "none" };
  return { titulo: "🥉 Novato de Cantera", color: "var(--mut)", marco: "none" };
}

/** Registra experiencia y logros tras un partido o evento */
export function sumarProgresoRPG({ nombre, victoria = false, goles = 0, arcoCero = false, huachas = 0, destronado = false }) {
  if (!nombre) return;
  const tabla = obtenerTablaRPG();
  const p = tabla[nombre] || { xp: 0, nivel: 1, partidos: 0, victorias: 0, goles: 0, huachas: 0, insignias: [] };

  p.partidos += 1;
  p.goles += (goles || 0);
  p.huachas += (huachas || 0);

  let gananciaXP = 50; // XP base por jugar
  if (victoria) {
    p.victorias += 1;
    gananciaXP += 100;
  }
  gananciaXP += (goles * 25);
  gananciaXP += (huachas * 30);

  // Insignias desbloqueables
  if (arcoCero && !p.insignias.includes("muro")) p.insignias.push("muro");
  if (goles >= 3 && !p.insignias.includes("matador")) p.insignias.push("matador");
  if (destronado && !p.insignias.includes("destronador")) p.insignias.push("destronador");
  if (p.huachas >= 5 && !p.insignias.includes("rey_canno")) p.insignias.push("rey_canno");

  p.xp += gananciaXP;
  p.nivel = Math.floor(1 + Math.sqrt(p.xp / 40));

  tabla[nombre] = p;
  guardarTablaRPG(tabla);
}

/** Insignia visual compacta para mostrar en la fila de mandos */
export function BadgeNivelRPG({ nombre }) {
  if (!nombre) return null;
  const p = obtenerPerfilRPG(nombre);
  const rango = calcularRango(p.nivel);

  return html`
    <span
      class="badge-rpg"
      style=${`display:inline-flex;align-items:center;gap:3px;padding:1.5px 6px;border-radius:4px;font-size:10.5px;font-family:var(--mono);font-weight:700;background:rgba(0,0,0,0.4);border:1px solid ${rango.color}55;color:${rango.color};box-shadow:${rango.marco}`}
      title=${`${rango.titulo} · Nivel ${p.nivel} (${p.xp} XP) · ${p.victorias}V / ${p.partidos}PJ · ${p.goles} Goles · ${p.huachas} Caños`}>
      <span>Nv.${p.nivel}</span>
    </span>`;
}
