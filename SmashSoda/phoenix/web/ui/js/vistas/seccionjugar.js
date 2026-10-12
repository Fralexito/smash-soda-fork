// =============================================================================
//  JUGAR: lo que quien va a jugar quiere encontrar rápido, sin pasar por la administración de la sala.
//    Salas abiertas (entrar con un clic) · Amigos (quién está y dónde) · Perfil PES (mi nombre y mis opciones
//    en PES) · Mi puesto (dónde me siento al pulsar JUGAR).
//  Solo compone vistas que ya existen: no tiene datos propios.
// =============================================================================
import { html } from "../lib.js";
import { t } from "../i18n.js";
import { SalasAbiertas, Amigos } from "./gente.js";
import { PerfilesPes } from "./perfiles.js";
import { PreferenciaPuesto } from "./jugar.js";
import { Vacio } from "../ui.js";

export const PESTANAS_JUGAR = [
  { id: "salas", es: "SALAS ABIERTAS", en: "OPEN ROOMS", d: ["Las mismas salas que ves en la web de la liga. Entra con un clic.", "The same rooms you see on the league website. Join with one click."] },
  { id: "amigos", es: "AMIGOS", en: "FRIENDS", d: ["Tus amigos de la web de la liga: quién está conectado y dónde. Invítalos a tu sala con un clic.", "Your league friends: who is online and where. Invite them to your room in one click."] },
  { id: "perfiles", es: "PERFIL PES", en: "PES PROFILE", d: ["El nombre y la configuración de cada persona en PES (comandos, cursor, pases…). Cuando se sienta, Link se lo pone solo.", "Each person's name and settings in PES (buttons, cursor, passing…). When they sit, Link applies them automatically."] },
  { id: "puesto", es: "MI PUESTO", en: "MY SEAT", d: ["Dónde te sientas tú cuando pulsas JUGAR: un puesto fijo o el primero de un equipo.", "Where you sit when you press PLAY: a fixed seat or the first one of a team."] },
];

/** Contador de la cabecera: salas donde se puede entrar ahora. */
export function metaJugar(s) {
  const n = ((s.motor?.salasAbiertas?.lista) || []).filter((x) => !x.soyHost && x.enlace && x.plazasLibres > 0).length;
  return html`<div class="mono mut" style="font-size:14px">${n} ${t(n === 1 ? "SALA ABIERTA" : "SALAS ABIERTAS", "OPEN ROOMS")}</div>`;
}

export function VistaJugar({ s, pestana }) {
  const m = s.motor;
  if (!m) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  if (pestana === "amigos") return html`<${Amigos} m=${m}/>`;
  if (pestana === "perfiles") return html`<${PerfilesPes}/>`;
  if (pestana === "puesto") {
    return html`<${PreferenciaPuesto} m=${m}/>`;
  }
  return html`<${SalasAbiertas} m=${m}/>`;
}
