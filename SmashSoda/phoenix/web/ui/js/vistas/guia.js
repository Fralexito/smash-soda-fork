// =============================================================================
//  GUÍA PARA NOVATOS: «Tu sala en 4 pasos» (portada con la sala cerrada) y el
//  «siguiente paso» (con la sala abierta). Todo sale del estado real del motor:
//  no guarda nada salvo si el usuario ya la ocultó (solo en este navegador).
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, irA } from "../tienda.js";
import { Tarjeta, Boton, Chip, Icono, cx } from "../ui.js";

const CLAVE = "phoenix.guia.oculta";

/** [visible, ocultar, mostrar]. Se oculta sola la primera vez que la sala se abre. */
export function useGuia(abierta) {
  const [oculta, setOculta] = useState(() => { try { return localStorage.getItem(CLAVE) === "1"; } catch { return false; } });
  const poner = (v) => { setOculta(v); try { localStorage.setItem(CLAVE, v ? "1" : "0"); } catch { /* sin almacenamiento: no pasa nada */ } };
  useEffect(() => { if (abierta && !oculta) poner(true); }, [abierta]);
  return [!oculta, () => poner(true), () => poner(false)];
}

const TERMINOS = [
  [["Sala", "Room"], ["Tu partida abierta en Parsec. La gente entra con un enlace que tú mandas.", "Your open Parsec session. People join with a link you send."]],
  [["Invitado", "Guest"], ["Cualquiera que entra a tu sala. Ve tu pantalla y, si le das mando, juega.", "Anyone who joins. They see your screen and play if you give them a pad."]],
  [["Puesto", "Seat"], ["Un mando del juego (1, 2, 3…). Cada puesto lo usa una persona.", "One game pad (1, 2, 3…). One person per seat."]],
  [["Espectador", "Spectator"], ["Alguien que solo mira, sin mando.", "Someone who only watches, no pad."]],
  [["Parche", "Patch"], ["La versión modificada del juego que usas (por ejemplo Conmegol Patch 26).", "The modded game version you use."]],
  [["Mbps", "Mbps"], ["Cuánto internet gasta la imagen. Más alto se ve mejor, pero exige más a tu subida.", "How much internet the video uses. Higher looks better but needs more upload."]],
  [["Ping", "Ping"], ["Lo que tarda un invitado en responder, en milisegundos. Menos es mejor.", "How long a guest takes to respond, in ms. Lower is better."]],
];

function Glosario() {
  return html`<details class="glosario">
    <summary>${t("¿QUÉ SIGNIFICA CADA COSA?", "WHAT DOES EVERYTHING MEAN?")}</summary>
    <dl>${TERMINOS.map(([n, d]) => html`<div key=${n[0]}><dt>${t(n[0], n[1])}</dt><dd>${t(d[0], d[1])}</dd></div>`)}</dl>
  </details>`;
}

export function GuiaInicio({ m, ocultar, abrir }) {
  const sala = m.sala;
  const ph = sala.phoenix || {};
  const c = sala.calidad || {};
  const mandos = m.mandos?.lista || [];
  const pcOk = !!sala.lista && mandos.length > 0;
  const juegoOk = !!(ph.juego && ph.parche);
  const subida = c.subida || 0;
  const web = m.web || {};
  const vinculada = web.estado === "conectado";

  const pasos = [
    {
      n: 1, ok: pcOk, titulo: t("Revisa tu PC", "Check your PC"),
      texto: pcOk ? t("Parsec y los mandos virtuales funcionan.", "Parsec and the virtual pads work.")
        : !sala.lista ? t("Parsec se está preparando. Si tarda, toca Revisar.", "Parsec is starting. If it takes long, tap Check.")
        : t("Falta ViGEmBus: sin él nadie puede jugar. Toca Revisar.", "ViGEmBus is missing: nobody can play without it. Tap Check."),
      boton: t("REVISAR", "CHECK"), al: () => irA("sala", "red"),
    },
    {
      n: 2, ok: juegoOk, titulo: t("Elige tu juego", "Pick your game"),
      texto: juegoOk ? `${ph.juego} · ${ph.parche}` : t("Dile a todos qué se juega y con qué parche.", "Tell everyone what is played and which patch."),
      boton: juegoOk ? t("CAMBIAR", "CHANGE") : t("ELEGIR", "PICK"), al: () => irA("sala", "juegos"),
    },
    {
      n: 3, ok: subida > 0, reco: true, titulo: t("Tu internet", "Your internet"),
      texto: subida > 0 ? t(`Subida ${subida} Mbps: Link cuida la calidad solo.`, `Upload ${subida} Mbps: Link manages quality for you.`)
        : t("Mide tu subida en fast.com y escríbela. Así Link cuida la calidad solo.", "Measure your upload at fast.com and enter it. Link then manages quality for you."),
      boton: subida > 0 ? t("CAMBIAR", "CHANGE") : t("ESCRIBIR", "ENTER"), al: () => irA("sala", "opciones"),
    },
    {
      n: 4, ok: false, titulo: t("Abre la sala", "Open the room"),
      texto: t("Se crea tu enlace. Mándalo por Discord o WhatsApp y listo.", "Your link is created. Send it on Discord or WhatsApp."),
      boton: t("ABRIR SALA", "OPEN ROOM"), al: abrir, desactivado: !sala.lista,
    },
  ];
  const listos = pasos.slice(0, 3).filter((p) => p.ok).length;
  let sig = pasos.findIndex((p) => !p.ok);
  if (sig < 0) sig = 3;

  return html`<${Tarjeta} clase="guia-inicio" interior="padding:24px 28px;display:flex;flex-direction:column;gap:18px">
    <div style="display:flex;justify-content:space-between;align-items:flex-start;gap:20px;flex-wrap:wrap">
      <div style="min-width:0;flex:1 1 320px">
        <div class="lab acc">${t("EMPIEZA AQUÍ", "START HERE")}</div>
        <div class="grande degradado" style="margin-top:10px;font-size:34px">${t("Tu sala en 4 pasos", "Your room in 4 steps")}</div>
        <div class="ayuda" style="margin-top:6px;font-size:14px">${t("Sigue los pasos de izquierda a derecha. El que brilla es el siguiente. Solo el 1 y el 4 son obligatorios.", "Follow the steps left to right. The glowing one is next. Only 1 and 4 are required.")}</div>
      </div>
      <div style="display:flex;flex-direction:column;align-items:flex-end;gap:10px">
        <div class="mono mut" style="font-size:13px;letter-spacing:.1em">${listos} ${t("DE 3 LISTOS", "OF 3 READY")}</div>
        <${Boton} tipo="mini suave" al=${ocultar} titulo=${t("Puedes volver a verla con «Cómo empezar»", "You can bring it back with «How to start»")}>${t("YA LO ENTIENDO", "GOT IT")}</${Boton}>
      </div>
    </div>
    <div class="progreso-guia" aria-hidden="true">${pasos.map((p, i) => html`<i key=${i} class=${p.ok ? "on" : ""}></i>`)}</div>
    <div class="camino">${pasos.map((p, i) => html`
      <div key=${p.n} class=${cx("peldano", p.ok && "ok", i === sig && "sig")}>
        <div class="cab">
          <span class="num">${p.ok ? "✓" : p.n}</span>
          ${p.ok ? html`<${Chip} tipo="mod">${t("LISTO", "READY")}</${Chip}>`
            : i === sig ? html`<${Chip} tipo="acc">${t("SIGUIENTE", "NEXT")}</${Chip}>`
            : p.reco ? html`<${Chip}>${t("RECOMENDADO", "RECOMMENDED")}</${Chip}>` : null}
        </div>
        <div class="t">${p.titulo}</div>
        <div class="d">${p.texto}</div>
        <${Boton} tipo=${i === sig ? "lleno" : "suave"} deshabilitado=${!!p.desactivado} al=${p.al}>${p.boton}</${Boton}>
      </div>`)}</div>
    <div class="guia-extra">
      <span class=${cx("punto", vinculada ? "ok" : "")}></span>
      <div style="min-width:0;flex:1"><b>${t("Opcional · Cuenta de la liga", "Optional · League account")}</b>
        <span class="mut"> — ${vinculada ? t("Vinculada: apareces en el radar y ves a tus amigos.", "Linked: you show on the radar and see your friends.")
          : t("Vincúlala para aparecer en el radar de la web y ver a tus amigos.", "Link it to show on the website radar and see your friends.")}</span></div>
      <${Boton} tipo="mini suave" al=${() => irA("sync", "web")}>${vinculada ? t("VER", "VIEW") : t("VINCULAR", "LINK")}</${Boton}>
    </div>
    <${Glosario}/>
  </${Tarjeta}>`;
}

/** Con la sala abierta: una sola cosa por hacer, según lo que pasa ahora mismo. */
export function Entrenador({ m }) {
  const sala = m.sala;
  const inv = sala.invitados || 0;
  const sentados = (m.mandos?.lista || []).filter((p) => p.ocupado).length;
  let texto, botones;
  if (inv === 0) {
    texto = t("Tu sala está abierta. Siguiente paso: copia el enlace y mándalo por Discord o WhatsApp. Cuando alguien entre, aparecerá aquí.",
      "Your room is open. Next: copy the link and send it on Discord or WhatsApp. When someone joins, they appear here.");
    botones = html`<${Boton} tipo="lleno" deshabilitado=${!sala.enlace} al=${() => accion("sala.copiarEnlace", {}, { ok: t("Enlace copiado. Pégalo en Discord o WhatsApp.", "Link copied.") })}>${t("COPIAR ENLACE", "COPY LINK")}</${Boton}>`;
  } else if (sentados < 2) {
    texto = t("Ya entró gente. Dale mando a quien va a jugar: en Mandos › Puestos arrastra a cada persona a un puesto. Quien dejes en «Mirando» solo ve la pantalla.",
      "People joined. Give a pad to whoever will play: in Pads › Seats drag each person to a seat. Anyone left in «Watching» only sees the screen.");
    botones = html`<${Boton} tipo="lleno" al=${() => irA("mandos", "puestos")}>${t("IR A PUESTOS", "GO TO SEATS")}</${Boton}>`;
  } else {
    texto = t("Todo en marcha. Si alguien va con lag, mira Sala › Conexión. Para llevar el marcador del partido, usa Partido.",
      "All running. If someone lags, check Room › Connection. To keep score, use Match.");
    botones = html`<${Boton} tipo="suave" al=${() => irA("sala", "red")}>${t("CONEXIÓN", "CONNECTION")}</${Boton}>
      <${Boton} tipo="suave" al=${() => irA("partido", "vivo")}>${t("PARTIDO", "MATCH")}</${Boton}>`;
  }
  return html`<div class="entrenador">
    <span class="acc" style="flex:none"><${Icono} n="rayo" t=${22}/></span>
    <div style="min-width:0;flex:1"><div class="lab acc" style="margin-bottom:4px">${t("SIGUIENTE PASO", "NEXT STEP")}</div><div style="font-size:14px;line-height:1.45">${texto}</div></div>
    <div style="display:flex;gap:8px;flex:none;flex-wrap:wrap">${botones}</div>
  </div>`;
}
