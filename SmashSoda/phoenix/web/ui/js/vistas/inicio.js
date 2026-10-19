// =============================================================================
//  INICIO (plan UX, fase 2): lo primero que ves. No quita ningún apartado: va
//  delante del menú de siempre y junta lo que hace falta para jugar en 2 clics.
//    · Semáforo Phoenix en una línea (lo que falta, con su botón).
//    · JUGAR CON UN AMIGO: abre tu sala (de amigos), copia el enlace y te deja
//      invitar a tus amigos de la web (/v1/invitar, acción amigos.invitar).
//    · BUSCAR RIVAL: abre tu sala pública (sale en el radar de la web) y te
//      enseña las salas abiertas para entrar a la mejor.
//    · Amigos conectados con INVITAR y la sala abierta con su enlace.
//  Todo usa acciones que ya existían (sala.abrir, sala.phoenix, sala.copiarEnlace,
//  amigos.invitar, ui.abrir). Si la web cae, JUGAR CON UN AMIGO funciona igual.
// =============================================================================
import { html, useState } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, irA, leer } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Avatar, Chip, Icono, PanelLateral, cx } from "../ui.js";
import { useSemaforo, CabeceraSemaforo, comprobaciones, revisarSemaforo } from "./semaforo.js";
import { mejorSala, unirmeASala } from "./gente.js";
import { useUltimoPartido, TarjetaResultado } from "./partido.js";

export const PESTANAS_INICIO = [
  { id: "inicio", es: "INICIO", en: "HOME", d: ["Jugar en dos clics: con un amigo o contra quien esté libre. Todo lo demás sigue en el menú.", "Play in two clicks: with a friend or against whoever is free. Everything else is still in the menu."] },
];

const espera = (ms) => new Promise((r) => setTimeout(r, ms));

/** Espera (como mucho `ms`) a que el estado del motor cumpla algo. */
async function esperarA(cond, ms = 20000) {
  for (let i = 0; i < ms / 400; i++) {
    if (cond(leer().motor || {})) return true;
    await espera(400);
  }
  return cond(leer().motor || {});
}

/** Antes de abrir: si algo está en rojo lo dice en una frase con su botón (o espera a Parsec). true = se puede seguir. */
async function listoParaAbrir() {
  await revisarSemaforo();   // revisa en el momento: nada de abrir con datos viejos
  const lista = comprobaciones(leer().motor || {});
  const rojo = lista.find((x) => x.nivel === "falta");
  if (rojo) {
    avisar(`${rojo.titulo}: ${rojo.frase}`, "warn", 12000, rojo.arreglo ? { texto: rojo.arreglo[0], al: rojo.arreglo[1] } : null);
    return false;
  }
  if (!leer().motor?.sala?.lista) {
    avisar(t("Parsec se está preparando: en cuanto esté, abro tu sala.", "Parsec is getting ready: as soon as it is, I'll open your room."), "info", 6000);
    if (!(await esperarA((m) => !!m.sala?.lista, 30000))) {
      avisar(t("Parsec no terminó de prepararse. Mira el Semáforo Phoenix.", "Parsec didn't finish getting ready. See the Phoenix check."), "warn", 10000, { texto: t("VER", "SEE"), al: () => irA("sala", "red") });
      return false;
    }
  }
  return true;
}

/** Abre la sala con esa visibilidad (si ya está abierta solo cambia la visibilidad) y espera el enlace. */
async function abrirCon(visibilidad) {
  const m = leer().motor || {};
  if (m.sala?.phoenix?.visibilidad !== visibilidad) await accion("sala.phoenix", { visibilidad }, { silencioso: true });
  if (!m.sala?.abierta) {
    if (!(await accion("sala.abrir", {}))) return false;
  }
  return esperarA((x) => !!(x.sala?.abierta && x.sala?.enlace), 25000);
}

export function VistaInicio({ s }) {
  const m = s.motor;
  const { resumen } = useSemaforo(m);
  const [invitar, setInvitar] = useState(false);
  const [ocupado, setOcupado] = useState("");
  const ultimo = useUltimoPartido(m);
  if (!m || !m.sala) return null;
  const sala = m.sala;

  const conAmigo = async () => {
    setOcupado("amigo");
    try {
      if (!(await listoParaAbrir())) return;
      if (!(await abrirCon(sala.phoenix?.visibilidad === "privada" ? "privada" : "amigos"))) {
        avisar(t("La sala no terminó de abrirse. Inténtalo otra vez.", "The room didn't finish opening. Try again."), "warn", 8000);
        return;
      }
      await accion("sala.copiarEnlace", {}, { ok: t("Sala abierta y enlace copiado: pégalo en WhatsApp o Discord.", "Room open and link copied: paste it in WhatsApp or Discord.") });
      setInvitar(true);
    } finally { setOcupado(""); }
  };
  const buscarRival = async () => {
    setOcupado("rival");
    try {
      if (!(await listoParaAbrir())) return;
      if (!(await abrirCon("publica"))) {
        avisar(t("La sala no terminó de abrirse. Inténtalo otra vez.", "The room didn't finish opening. Try again."), "warn", 8000);
        return;
      }
      avisar(m.web?.estado === "conectado"
        ? t("Tu sala pública ya sale en el radar de la web: cualquiera de la liga puede entrar.", "Your public room is on the website radar: anyone in the league can join.")
        : t("Tu sala está abierta, pero sin cuenta conectada no sale en el radar. Comparte el enlace o conecta tu cuenta.", "Your room is open, but without a connected account it isn't on the radar. Share the link or connect your account."),
        m.web?.estado === "conectado" ? "ok" : "warn", 9000);
    } finally { setOcupado(""); }
  };

  const amigos = (m.amigos?.lista || []);
  const conectados = amigos.filter((x) => x.estado && x.estado !== "desconectado");
  const salas = (m.salasAbiertas?.lista || []).filter((x) => !x.soyHost && x.enlace && x.plazasLibres > 0);
  const mejor = mejorSala(m);
  const buscando = sala.abierta && sala.phoenix?.visibilidad === "publica";

  return html`<div class="col inicio">
    <${Tarjeta} clase="heroe" interior="padding:26px 30px;display:flex;flex-direction:column;gap:18px">
      <div style="display:flex;justify-content:space-between;align-items:flex-start;gap:16px;flex-wrap:wrap">
        <div style="min-width:0;flex:1 1 320px">
          <div class="lab acc">${t("PHOENIX", "PHOENIX")}</div>
          <div class="grande degradado" style="margin-top:10px;font-size:46px">${sala.abierta ? t("Tu sala está abierta", "Your room is open") : t("¿A qué jugamos hoy?", "What are we playing today?")}</div>
        </div>
        <div class="caja" style="flex:0 1 380px;min-width:260px;padding:12px 16px">
          <${CabeceraSemaforo} resumen=${resumen} derecha=${html`<${Boton} tipo="mini suave" al=${() => irA("sala", "red")}>${t("VER DETALLES", "SEE DETAILS")}</${Boton}>`}/>
          ${resumen.faltan[0] || resumen.avisos[0] ? html`<div class="ayuda" style="margin-top:6px">${(resumen.faltan[0] || resumen.avisos[0]).titulo}: ${(resumen.faltan[0] || resumen.avisos[0]).frase}</div>` : null}
        </div>
      </div>
      <div class="inicio-botones">
        <button class=${cx("inicio-boton", "principal", ocupado === "amigo" && "ocupado")} disabled=${!!ocupado} onClick=${conAmigo}>
          <span class="ib-icono"><${Icono} n="gente" t=${34}/></span>
          <span class="ib-texto"><span class="ib-t">${t("JUGAR CON UN AMIGO", "PLAY WITH A FRIEND")}</span>
            <span class="ib-d">${ocupado === "amigo" ? t("Abriendo tu sala…", "Opening your room…") : sala.abierta ? t("Copia el enlace e invita a tus amigos.", "Copy the link and invite your friends.") : t("Abre tu sala, copia el enlace e invita.", "Opens your room, copies the link and invites.")}</span></span>
        </button>
        <button class=${cx("inicio-boton", ocupado === "rival" && "ocupado")} disabled=${!!ocupado} onClick=${buscarRival}>
          <span class="ib-icono"><${Icono} n="trofeo" t=${34}/></span>
          <span class="ib-texto"><span class="ib-t">${buscando ? t("BUSCANDO RIVAL…", "LOOKING FOR A RIVAL…") : t("BUSCAR RIVAL", "FIND A RIVAL")}</span>
            <span class="ib-d">${ocupado === "rival" ? t("Abriendo tu sala pública…", "Opening your public room…") : buscando ? t("Tu sala pública está en el radar de la web.", "Your public room is on the website radar.") : t("Abre una sala pública en el radar de la web.", "Opens a public room on the website radar.")}</span></span>
        </button>
      </div>
      ${sala.abierta ? html`<${FranjaSala} m=${m} abrirInvitar=${() => setInvitar(true)}/>` : null}
      ${ultimo ? html`<${TarjetaResultado} m=${m} r=${ultimo} compacto/>` : null}
    </${Tarjeta}>
    <div class="rej2" style="align-items:start">
      <${Tarjeta} interior="padding:20px 24px;display:flex;flex-direction:column;gap:10px">
        <${Titulo} texto=${t("AMIGOS CONECTADOS", "FRIENDS ONLINE")} derecha=${html`<span class="mono mut">${conectados.length}</span>`}/>
        ${m.web?.estado !== "conectado" ? html`<div class="ayuda">${t("Conecta tu cuenta de la liga para ver a tus amigos e invitarlos con un toque.", "Connect your league account to see your friends and invite them in one tap.")}</div>
            <div><${Boton} tipo="mini lleno" al=${() => irA("sync", "mipc")}>${t("CONECTAR MI CUENTA", "CONNECT MY ACCOUNT")}</${Boton}></div>`
          : conectados.length === 0 ? html`<div class="ayuda">${t("Ninguno conectado ahora. Puedes invitarlos igual: la invitación dura 2 horas.", "None online now. You can still invite them: the invite lasts 2 hours.")}</div>
            ${amigos.length ? html`<div><${Boton} tipo="mini suave" al=${() => setInvitar(true)}>${t("INVITAR A…", "INVITE…")}</${Boton}></div>` : null}`
          : conectados.slice(0, 6).map((x) => html`<${FilaAmigo} key=${x.usuarioId} x=${x} m=${m} alInvitar=${conAmigoYa(x, m, conAmigo)}/>`)}
      </${Tarjeta}>
      <${Tarjeta} interior="padding:20px 24px;display:flex;flex-direction:column;gap:10px">
        <${Titulo} texto=${t("SALAS ABIERTAS AHORA", "OPEN ROOMS NOW")} derecha=${html`<span class="mono mut">${salas.length}</span>`}/>
        ${mejor ? html`<div class="caja" style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
            <div style="flex:1;min-width:180px"><b>${mejor.hostNombre}</b> ${mejor.hostAmigo ? html`<${Chip} tipo="ok">${t("AMIGO", "FRIEND")}</${Chip}>` : null}
              <div class="mono mut" style="font-size:12px">${[mejor.region, mejor.pingMs >= 0 ? mejor.pingMs + " ms" : "", t(`${mejor.plazasLibres} libres`, `${mejor.plazasLibres} free`)].filter(Boolean).join(" · ")}</div></div>
            <${Boton} tipo="mini lleno" al=${() => unirmeASala(mejor)}><${Icono} n="enviar" t=${14}/>${t("ENTRAR A LA MEJOR", "JOIN THE BEST")}</${Boton}></div>`
          : html`<div class="ayuda">${m.web?.estado === "conectado" ? t("No hay salas libres ahora. Con BUSCAR RIVAL la tuya sale en el radar para que entren a ti.", "No free rooms now. With FIND A RIVAL yours shows on the radar so others join you.") : t("Conecta tu cuenta para ver las salas de la liga.", "Connect your account to see the league's rooms.")}</div>`}
        ${salas.length > 1 ? html`<div><${Boton} tipo="mini suave" al=${() => irA("jugar", "salas")}>${t(`VER LAS ${salas.length}`, `SEE ALL ${salas.length}`)}</${Boton}></div>` : null}
      </${Tarjeta}>
    </div>
    ${invitar ? html`<${PanelInvitar} m=${m} alCerrar=${() => setInvitar(false)}/>` : null}
  </div>`;
}

/** Invitar a un amigo desde INICIO: si la sala está cerrada, primero la abre (todo en un toque). */
function conAmigoYa(x, m, abrir) {
  return async () => {
    if (!m.sala?.abierta) { await abrir(); if (!leer().motor?.sala?.abierta) return; }
    await accion("amigos.invitar", { usuarioId: x.usuarioId }, { ok: t(`Invitación enviada a ${x.nombre}.`, `Invite sent to ${x.nombre}.`) });
  };
}

const ESTADO_AMIGO = { en_sala: ["acc", "EN SALA", "IN ROOM"], en_partida: ["mor vivo", "JUGANDO", "PLAYING"], disponible: ["ok", "ACTIVO", "ACTIVE"], ausente: ["warn", "AUSENTE", "AWAY"], desconectado: ["", "DESCONECTADO", "OFFLINE"] };
function FilaAmigo({ x, alInvitar }) {
  const e = ESTADO_AMIGO[x.estado] || ["", String(x.estado || "").toUpperCase(), String(x.estado || "").toUpperCase()];
  return html`<div class="persona" style="cursor:default">
    <${Avatar} nombre=${x.nombre} url=${String(x.avatar || "").startsWith("https://") ? x.avatar : ""} t=${36}/>
    <div style="flex:1;min-width:0"><div class="nom">${x.nombre}</div><span class="semaforo"><span class=${cx("punto", e[0])}></span>${t(e[1], e[2])}</span></div>
    <${Boton} tipo="mini" al=${alInvitar}><${Icono} n="enviar" t=${14}/>${t("INVITAR", "INVITE")}</${Boton}>
  </div>`;
}

/** Con la sala abierta: enlace, cuántos hay y los atajos a la sala. */
function FranjaSala({ m, abrirInvitar }) {
  const sala = m.sala;
  return html`<div class="caja" style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
    <span class="punto ok vivo"></span>
    <b>${t(`EN VIVO · ${sala.invitados} EN SALA`, `LIVE · ${sala.invitados} IN ROOM`)}</b>
    <div class="campo mono" style="flex:1;min-width:200px;display:flex;align-items:center;height:38px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;user-select:text">${sala.enlace || t("Generando enlace…", "Generating link…")}</div>
    <${Boton} tipo="mini" deshabilitado=${!sala.enlace} al=${() => accion("sala.copiarEnlace", {}, { ok: t("Enlace copiado.", "Link copied.") })}><${Icono} n="copiar" t=${14}/>${t("COPIAR", "COPY")}</${Boton}>
    <${Boton} tipo="mini" al=${abrirInvitar}><${Icono} n="enviar" t=${14}/>${t("INVITAR A…", "INVITE…")}</${Boton}>
    <${Boton} tipo="mini suave" al=${() => irA("mandos", "puestos")}>${t("PUESTOS", "SEATS")}</${Boton}>
    <${Boton} tipo="mini suave" al=${() => irA("sala", "resumen")}>${t("MI SALA", "MY ROOM")}</${Boton}>
  </div>`;
}

/** «Invitar a…»: tus amigos de la web, cada uno con su botón (y el enlace por si la web no responde). */
function PanelInvitar({ m, alCerrar }) {
  const amigos = [...(m.amigos?.lista || [])];
  const peso = { en_partida: 3, en_sala: 1, disponible: 0, ausente: 2, desconectado: 4 };
  amigos.sort((a, b) => (peso[a.estado] ?? 5) - (peso[b.estado] ?? 5) || a.nombre.localeCompare(b.nombre));
  const [hecho, setHecho] = useState({});
  const invitar = async (x) => { if (await accion("amigos.invitar", { usuarioId: x.usuarioId }, { ok: t(`Invitación enviada a ${x.nombre}.`, `Invite sent to ${x.nombre}.`) })) setHecho({ ...hecho, [x.usuarioId]: true }); };
  return html`<${PanelLateral} titulo=${t("Invitar a…", "Invite…")} sub=${t("TU SALA ESTÁ ABIERTA", "YOUR ROOM IS OPEN")} alCerrar=${alCerrar}>
    <div class="ayuda" style="font-size:14px">${t("El enlace ya está copiado: pégalo en WhatsApp o Discord. Si tu amigo usa la web de la liga, invítalo aquí y le llega el aviso.", "The link is already copied: paste it in WhatsApp or Discord. If your friend uses the league website, invite them here and they get a notice.")}</div>
    <div><${Boton} tipo="mini suave" al=${() => accion("sala.copiarEnlace", {}, { ok: t("Enlace copiado.", "Link copied.") })}><${Icono} n="copiar" t=${14}/>${t("COPIAR EL ENLACE OTRA VEZ", "COPY THE LINK AGAIN")}</${Boton}></div>
    ${m.web?.estado !== "conectado" ? html`<div class="caja">${t("Para invitar desde aquí conecta tu cuenta de la liga. Mientras tanto, el enlace sirve igual.", "To invite from here connect your league account. Meanwhile, the link works the same.")}
        <div style="margin-top:8px"><${Boton} tipo="mini lleno" al=${() => { alCerrar(); irA("sync", "mipc"); }}>${t("CONECTAR MI CUENTA", "CONNECT MY ACCOUNT")}</${Boton}></div></div>`
      : amigos.length === 0 ? html`<div class="ayuda">${t("Aún no tienes amigos en la web. Agrégalos desde tu perfil.", "No friends on the website yet. Add them from your profile.")}</div>`
      : html`<div style="display:flex;flex-direction:column;gap:8px">${amigos.map((x) => {
          const e = ESTADO_AMIGO[x.estado] || ["", "", ""];
          return html`<div class="persona" style="cursor:default" key=${x.usuarioId}>
            <${Avatar} nombre=${x.nombre} url=${String(x.avatar || "").startsWith("https://") ? x.avatar : ""} t=${34}/>
            <div style="flex:1;min-width:0"><div class="nom">${x.nombre}</div><span class="semaforo"><span class=${cx("punto", e[0])}></span>${t(e[1], e[2])}</span></div>
            ${hecho[x.usuarioId] ? html`<${Chip} tipo="ok">✓ ${t("INVITADO", "INVITED")}</${Chip}>`
              : html`<${Boton} tipo="mini lleno" al=${() => invitar(x)}>${t("INVITAR", "INVITE")}</${Boton}>`}
          </div>`; })}</div>`}
  </${PanelLateral}>`;
}
