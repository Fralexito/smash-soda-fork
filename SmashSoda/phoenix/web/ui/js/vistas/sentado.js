// =============================================================================
//  SENTADO AUTOMÁTICO (plan UX, fase 2): quien está en «Mirando» y pulsa un botón
//  de su mando ya no espera a que el anfitrión pulse ACEPTAR: ocupa solo el primer
//  puesto libre (la misma acción solicitud.aceptar de siempre, que en el motor
//  respeta el candado de puesto y nunca da un puesto fuera de los equipos).
//  Encendido por defecto, para cualquiera. Con «Solo mis amigos», solo se sienta
//  solo quien es tu amigo en la web; los demás siguen pidiendo ACEPTAR.
//  No toca: «!cambio N» / «!equipo» (siguen pidiendo permiso), ni a quien entra
//  por Parsec sin estar en la lista de la web (sigue JUEGA / MIRA / EXPULSAR),
//  ni a quien la web lista como espectador.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";
import { AjusteSw } from "../ui.js";

const CLAVE = "phoenix.sentadoAuto";
const POR_DEFECTO = { auto: true, soloAmigos: false };
let pref = (() => { try { return { ...POR_DEFECTO, ...JSON.parse(localStorage.getItem(CLAVE) || "{}") }; } catch { return { ...POR_DEFECTO }; } })();
const oyentes = new Set();
export function leerSentado() { return pref; }
export function ponerSentado(p) {
  pref = { ...pref, ...p };
  try { localStorage.setItem(CLAVE, JSON.stringify(pref)); } catch { /* sin almacenamiento */ }
  oyentes.forEach((f) => f(pref));
}

// Amigos de la web: la última lista que llegó (el motor la manda siempre desde 0.3.27)
let amigosConocidos = [];
const normal = (x) => String(x || "").trim().toLowerCase();
/** ¿Es amigo en la web? Por la cuenta ligada a su Parsec (/v1/perfiles): id si la web lo manda, si no por nombre. */
export function esAmigo(m, parsecId) {
  const perfil = m.perfiles?.[parsecId];
  if (!perfil) return false;
  const id = perfil.usuario_id || perfil.usuarioId || perfil.id;
  return amigosConocidos.some((a) => (id && a.usuarioId === id) || (normal(a.nombre) && normal(a.nombre) === normal(perfil.nombre)));
}

const intentados = new Map();   // parsecId → hora del último intento (no insistir más de una vez cada 20 s)
/** Se llama con cada estado del motor: sienta solo a quien pulsó su mando estando en «Mirando». */
export function vigilarSentado(m) {
  if (!m) return;
  if (m.amigos?.cargados) amigosConocidos = m.amigos.lista || [];
  if (!pref.auto || !m.sala?.abierta) return;
  const libres = (m.mandos?.lista || []).filter((p) => !p.ocupado && p.equipo !== "fuera").length;
  if (!libres) return;
  const ahora = Date.now();
  for (const s of m.solicitudes || []) {
    if (s.mando >= 0) continue;   // «!cambio» / «!equipo»: los decide el anfitrión
    const g = (m.invitados || []).find((x) => x.parsecId === s.parsecId);
    if (g?.rolWeb === "espectador") continue;
    if (pref.soloAmigos && !esAmigo(m, s.parsecId)) continue;
    if (ahora - (intentados.get(s.parsecId) || 0) < 20000) continue;
    intentados.set(s.parsecId, ahora);
    accion("solicitud.aceptar", { parsecId: s.parsecId }, { silencioso: true }).then((r) => {
      if (r) avisar(t(`${s.nombre} pulsó su mando y se sentó solo en un puesto libre.`, `${s.nombre} pressed their pad and took a free seat.`), "ok", 5000);
    });
    return;   // de uno en uno: el siguiente estado ya trae los puestos al día
  }
}

/** Los dos interruptores (en Mandos › Puestos y en Ajustes). */
export function AjustesSentado({ compacto = false }) {
  const [p, setP] = useState(pref);
  const poner = (x) => { ponerSentado(x); setP(leerSentado()); };
  return html`<div class=${compacto ? "caja" : ""} style=${compacto ? "display:flex;flex-direction:column;gap:2px;padding:6px 16px" : ""}>
    <${AjusteSw} titulo=${t("Sentar solo a quien pulsa su mando", "Seat whoever presses their pad")} valor=${p.auto} al=${(v) => poner({ auto: v })}
      desc=${t("Quien está mirando y pulsa un botón ocupa el primer puesto libre, sin que tengas que pulsar ACEPTAR.", "Whoever is watching and presses a button takes the first free seat, no ACCEPT needed.")}/>
    ${p.auto ? html`<${AjusteSw} titulo=${t("Solo mis amigos", "Only my friends")} valor=${p.soloAmigos} al=${(v) => poner({ soloAmigos: v })}
      desc=${t("Solo se sientan solos tus amigos de la web; los demás te piden ACEPTAR.", "Only your website friends seat themselves; others ask you to ACCEPT.")}/>` : null}
  </div>`;
}
