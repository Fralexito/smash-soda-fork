// =============================================================================
//  Phoenix Link · Radar de Conexión & Alertas de Lagger en Vivo (Telemetría)
//  Autor: Antigravity (Gemini) · Google DeepMind
//  Monitorea en tiempo real la estabilidad de red de cada jugador activo.
//  Detecta micro-cortes, jitter y pérdidas de paquetes para evitar arruinar
//  la fluidez de los partidos competitivos en PES 2021.
// =============================================================================

import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { colorPing, Boton, Icono } from "../ui.js";
import { retrasoDe } from "./retraso.js";

/** Evalúa el estado de conexión de un invitado según ping y pérdidas */
export function clasificarConexion(parsecId, m) {
  if (!parsecId || parsecId === m?.sala?.hostId) {
    return { estado: "host", ping: 0, perdida: 0, etiqueta: "HOST (0 ms)", color: "var(--acc)", clase: "ok" };
  }
  const r = retrasoDe(parsecId, m);
  const red = (m?.red || []).find((x) => x.parsecId === parsecId);

  const ping = r?.ms ?? red?.ultimo ?? -1;
  const perdida = r?.perdida ?? 0;

  if (ping < 0) {
    return { estado: "desconocido", ping: -1, perdida: 0, etiqueta: "—", color: "var(--mut)", clase: "" };
  }
  if (ping <= 45 && perdida < 1) {
    return { estado: "optimo", ping, perdida, etiqueta: `${ping} ms`, color: "#00ff88", clase: "ok" };
  }
  if (ping <= 85 && perdida <= 3) {
    return { estado: "medio", ping, perdida, etiqueta: `${ping} ms`, color: "#ffb700", clase: "warn" };
  }
  return { estado: "critico", ping, perdida, etiqueta: `⚠️ ${ping} ms`, color: "#ff4444", clase: "bad" };
}

/** Pastilla compacta con semáforo para incrustar en cada puesto o fila */
export function BadgeConexion({ parsecId, m }) {
  const c = clasificarConexion(parsecId, m);
  if (c.estado === "desconocido") return null;

  return html`
    <span
      class="badge-ping"
      style=${`display:inline-flex;align-items:center;gap:4px;padding:2px 6px;border-radius:6px;font-size:11px;font-family:var(--mono);font-weight:600;background:rgba(0,0,0,0.35);border:1px solid ${c.color}44;color:${c.color}`}
      title=${t(`Latencia: ${c.ping} ms · Pérdida estimada: ${c.perdida}%`, `Latency: ${c.ping} ms · Estimated loss: ${c.perdida}%`)}>
      <span style=${`width:6px;height:6px;border-radius:50%;background:${c.color};box-shadow:0 0 6px ${c.color};flex:none`}></span>
      <span>${c.etiqueta}</span>
    </span>`;
}

/** Alerta proactiva si algún jugador que tiene mando está lagueando */
export function AlertaLaggerSala({ m }) {
  const puestos = m?.mandos?.lista || [];
  const laggers = [];

  for (const p of puestos) {
    if (p.ocupado && p.parsecId && p.parsecId !== m?.sala?.hostId) {
      const c = clasificarConexion(p.parsecId, m);
      if (c.estado === "critico") {
        laggers.push({ nombre: p.nombre || `Puesto ${p.n}`, ping: c.ping, perdida: c.perdida, puesto: p.n });
      }
    }
  }

  if (laggers.length === 0) return null;

  return html`
    <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;padding:8px 14px;border-radius:10px;background:rgba(255,50,50,0.12);border:1px solid rgba(255,50,50,0.4);color:#ff7777;margin-bottom:8px">
      <div style="display:flex;align-items:center;gap:8px;font-size:12.5px">
        <span style="font-size:16px">📡⚠️</span>
        <span>
          <b>${t("ALERTA DE CONEXIÓN INESTABLE:", "UNSTABLE CONNECTION ALERT:")}</b>
          ${laggers.map((l) => ` ${l.nombre} (${l.ping} ms)`).join(", ")}.
          <span style="color:#ffb700;margin-left:4px">${t("Puede provocar tirones en PES.", "May cause stuttering in PES.")}</span>
        </span>
      </div>
    </div>`;
}
