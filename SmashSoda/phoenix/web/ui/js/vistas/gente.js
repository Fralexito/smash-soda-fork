// =============================================================================
//  GENTE: En sala (lista + ficha, diseño v4, con la carta de la web),
//  Moderación (baneados, moderadores, VIP, historial) y Amigos de la web.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t, fecha } from "../i18n.js";
import { accion, confirmar, irA } from "../tienda.js";
import { Permisos } from "./ajustes.js";
import { Tarjeta, Titulo, Boton, AjusteSw, Campo, Avatar, Chip, Icono, Vacio, colorPing, cx, dos } from "../ui.js";

export const PESTANAS_GENTE = [
  { id: "sala", es: "EN SALA", en: "IN ROOM", d: ["Elige a alguien de la lista para ver su ficha, su carta de la liga y sus permisos.", "Pick someone to see their card and permissions."] },
  { id: "moderacion", es: "MODERACIÓN", en: "MODERATION", d: ["Baneados, moderadores, VIP y quién pasó por tu sala.", "Bans, mods, VIPs and visit history."] },
  { id: "amigos", es: "AMIGOS", en: "FRIENDS", d: ["Tus amigos de la web de la liga. Invítalos a tu sala con un clic.", "Your league friends. Invite them in one click."] },
  { id: "permisos", es: "PERMISOS", en: "PERMISSIONS", d: ["Qué puede hacer cada rol (invitado, VIP, moderador) desde el chat.", "What each role (guest, VIP, moderator) can do from chat."] },
];

export function metaGente(s) {
  const n = (s.motor?.invitados || []).filter((g) => !g.falso).length;
  return html`<div class="mono mut" style="font-size:14px">${n} ${t(n === 1 ? "CONECTADO" : "CONECTADOS", "CONNECTED")}</div>`;
}

export function VistaGente({ s, pestana }) {
  const m = s.motor;
  if (!m) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  if (pestana === "moderacion") return html`<${Moderacion}/>`;
  if (pestana === "amigos") return html`<${Amigos} m=${m}/>`;
  if (pestana === "permisos") return html`<${Permisos} m=${m}/>`;
  return html`<${EnSala} m=${m}/>`;
}

// ---- Carta de jugador (de la web de la liga) ---------------------------------------------
export function Carta({ perfil }) {
  const c = perfil?.carta;
  if (!c) return null;
  const st = c.stats || {};
  return html`<div class=${cx("carta", c.rareza)}>
    <div class="media">${c.media ?? "—"}</div>
    <div class="pos">${c.posicion || ""}</div>
    ${c.foto_url || perfil.avatar_url ? html`<img class="foto" src=${c.foto_url || perfil.avatar_url} alt="" referrerpolicy="no-referrer" onError=${(e) => { e.currentTarget.style.display = "none"; }}/>` : null}
    <div class="apodo">${c.apodo || perfil.nombre}</div>
    <div class="club">${c.club || ""}${c.pais ? " · " + c.pais : ""}</div>
    <div class="stats">
      <span>RIT ${st.rit ?? "-"}</span><span>TIR ${st.tir ?? "-"}</span><span>PAS ${st.pas ?? "-"}</span>
      <span>REG ${st.reg ?? "-"}</span><span>DEF ${st.def ?? "-"}</span><span>FIS ${st.fis ?? "-"}</span>
    </div>
  </div>`;
}

const ROL_WEB = {
  jugador: ["acc", "EN LISTA · JUGADOR", "LISTED · PLAYER"],
  espectador: ["", "EN LISTA · ESPECTADOR", "LISTED · SPECTATOR"],
  no_listado: ["vip", "NO ESTÁ EN LA LISTA", "NOT LISTED"],
};

// ---- En sala ----------------------------------------------------------------------------
let elegidoGuardado = 0;

function EnSala({ m }) {
  const [filtro, setFiltro] = useState("");
  const [elegido, setElegidoLocal] = useState(elegidoGuardado);
  const setElegido = (id) => { elegidoGuardado = id; setElegidoLocal(id); };
  const gente = (m.invitados || []).filter((g) => !g.falso);
  const f = filtro.trim().toLowerCase();
  const lista = gente.filter((g) => !f || g.nombre.toLowerCase().includes(f) || String(g.parsecId).includes(f));
  const actual = gente.find((g) => g.parsecId === elegido) || lista[0] || null;

  if (!m.sala?.abierta && gente.length === 0) {
    return html`<${Vacio} titulo=${t("La sala está cerrada", "The room is closed")} texto=${t("Cuando abras la sala, aquí verás a cada invitado con su ping, su carta de la liga y sus permisos.", "Open the room to see every guest here.")}>
      <${Boton} tipo="lleno" al=${() => irA("sala", "resumen")}>${t("IR A SALA", "GO TO ROOM")}</${Boton}></${Vacio}>`;
  }
  return html`<div class="fila partible" style="min-height:100%">
    <div class="col" style="width:min(470px,42%);flex:none;gap:12px">
      <input class="campo" style="height:44px" placeholder=${t("Filtrar por nombre o ID", "Filter by name or ID")} value=${filtro} onInput=${(e) => setFiltro(e.currentTarget.value)}/>
      <div style="display:flex;flex-direction:column;gap:8px">
        ${lista.length === 0 ? html`<div class="ayuda" style="padding:10px">${gente.length ? t("Nadie coincide.", "No matches.") : t("Aún no hay invitados.", "No guests yet.")}</div>` : null}
        ${lista.map((g) => {
          const on = actual && actual.parsecId === g.parsecId;
          const perfil = m.perfiles?.[g.parsecId];
          return html`<button key=${g.parsecId} class=${cx("persona", on && "on")} style="height:68px" onClick=${() => setElegido(g.parsecId)}>
            <${Avatar} nombre=${perfil?.nombre || g.nombre} url=${perfil?.avatar_url} id=${g.parsecId} brillo=${on} t=${40}/>
            <div style="flex:1;min-width:0;text-align:left">
              <div class="nom">${g.nombre}</div>
              <div class="sub"><span class="mono mut" style="font-size:12px">#${g.parsecId}</span>
                ${g.mod ? html`<${Chip} tipo="mod">MOD</${Chip}>` : null}${g.vip ? html`<${Chip} tipo="vip">VIP</${Chip}>` : null}
                ${perfil?.carta ? html`<${Chip} tipo="acc2">${perfil.carta.media}</${Chip}>` : null}</div>
            </div>
            <div style="text-align:right"><div class="mono" style=${`font-size:16px;font-weight:600;color:${colorPing(g.ping)}`}>${g.ping >= 0 ? g.ping + " ms" : "—"}</div>
              <div class="mono mut" style="margin-top:4px;font-size:12px">${g.mando ? `${t("MANDO", "PAD")} ${dos(g.mando)}` : t("ESPECTADOR", "SPECTATOR")}</div></div>
          </button>`;
        })}
      </div>
    </div>
    ${actual ? html`<${Ficha} m=${m} g=${actual}/>` : html`<div style="flex:1"></div>`}
  </div>`;
}

function Ficha({ m, g }) {
  const perfil = m.perfiles?.[g.parsecId];
  const libres = (m.mandos?.lista || []).filter((p) => !p.ocupado);
  const rol = ROL_WEB[g.rolWeb];
  const expulsar = async () => {
    if (await confirmar({ titulo: t(`¿Expulsar a ${g.nombre}?`, `Kick ${g.nombre}?`), texto: t("Sale de la sala, pero puede volver a entrar con el enlace.", "They leave but can rejoin with the link."), si: t("Expulsar", "Kick"), peligro: true })) {
      await accion("gente.expulsar", { parsecId: g.parsecId, nombre: g.nombre }, { ok: t("Expulsado.", "Kicked.") });
    }
  };
  const banear = async () => {
    if (await confirmar({ titulo: t(`¿Banear a ${g.nombre}?`, `Ban ${g.nombre}?`), texto: t("No podrá volver a entrar a tus salas. Se puede deshacer en Moderación.", "They won't be able to join again. Undo in Moderation."), si: t("Banear", "Ban"), peligro: true })) {
      await accion("gente.banear", { parsecId: g.parsecId, nombre: g.nombre }, { ok: t("Baneado.", "Banned.") });
    }
  };
  return html`<${Tarjeta} estilo="flex:1;min-width:0" interior="padding:26px 30px;display:flex;flex-direction:column;gap:18px">
    <div style="display:flex;gap:18px;align-items:center">
      <${Avatar} nombre=${perfil?.nombre || g.nombre} url=${perfil?.avatar_url} brillo t=${72}/>
      <div style="flex:1;min-width:0">
        <div class="disp" style="font-size:30px;font-weight:700;line-height:1.1;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${g.nombre}</div>
        <div style="margin-top:8px;display:flex;gap:8px;align-items:center;flex-wrap:wrap">
          <span class="mono mut" style="font-size:13px">#${g.parsecId}</span>
          <${Chip} tipo="acc">${g.mando ? `${t("JUGANDO · MANDO", "PLAYING · PAD")} ${dos(g.mando)}` : t("MIRANDO", "WATCHING")}</${Chip}>
          ${rol ? html`<${Chip} tipo=${rol[0]}>${t(rol[1], rol[2])}</${Chip}>` : null}
          ${perfil ? html`<${Chip} tipo="acc2">${t("CUENTA DE LA LIGA", "LEAGUE ACCOUNT")}: ${perfil.nombre}</${Chip}>` : null}
          ${g.cop ? html`<${Chip} tipo="bad">${t("MOD OFICIAL", "OFFICIAL MOD")}</${Chip}>` : null}
        </div>
      </div>
      <div style="text-align:right"><div class="lab">PING</div><div class="mono" style=${`margin-top:8px;font-size:30px;font-weight:700;color:${colorPing(g.ping)}`}>${g.ping >= 0 ? g.ping + " ms" : "—"}</div></div>
    </div>

    <div style="display:flex;gap:24px;flex-wrap:wrap">
      <div style="flex:1;min-width:260px;display:flex;flex-direction:column;gap:18px">
        <div>
          <div class="lab acc">${t("ROLES", "ROLES")}</div>
          <div style="margin-top:12px;display:flex;gap:10px;flex-wrap:wrap">
            <${Boton} tipo="warn" al=${() => accion("gente.vip", { parsecId: g.parsecId, nombre: g.nombre })}>${g.vip ? t("QUITAR VIP", "REMOVE VIP") : t("HACER VIP", "MAKE VIP")}</${Boton}>
            <${Boton} tipo="ok" al=${() => accion("gente.mod", { parsecId: g.parsecId, nombre: g.nombre })}>${g.mod ? t("QUITAR MODERADOR", "REMOVE MOD") : t("HACER MODERADOR", "MAKE MOD")}</${Boton}>
          </div>
        </div>
        <div>
          <div class="lab acc">${t("PERMISOS", "PERMISSIONS")}</div>
          <${AjusteSw} titulo=${t("Permitir teclado", "Allow keyboard")} desc=${t("Puede jugar con el teclado aunque no tenga mando.", "Can play with keyboard without a pad.")}
            valor=${g.teclado} al=${(v) => accion("gente.teclado", { parsecId: g.parsecId, si: v })}/>
          <${AjusteSw} titulo=${t("Permitir ratón", "Allow mouse")} desc=${t("Puede mover el ratón en el equipo anfitrión.", "Can move the host's mouse.")}
            valor=${g.raton} al=${(v) => accion("gente.raton", { parsecId: g.parsecId, si: v })}/>
        </div>
      </div>
      ${perfil?.carta ? html`<div><div class="lab acc" style="margin-bottom:12px">${t("CARTA EN LA LIGA", "LEAGUE CARD")}</div><${Carta} perfil=${perfil}/></div>` : null}
    </div>

    <div style="margin-top:auto;display:flex;gap:10px;flex-wrap:wrap;align-items:center">
      ${g.mando
        ? html`<${Boton} al=${() => accion("mandos.liberar", { indice: g.mando - 1 })}>${t("QUITAR MANDO", "REMOVE PAD")}</${Boton}>`
        : html`<select class="campo" style="width:auto;height:38px" disabled=${libres.length === 0}
            onChange=${(e) => { const v = Number(e.currentTarget.value); e.currentTarget.value = ""; if (v) accion("mandos.asignar", { indice: v - 1, parsecId: g.parsecId }); }}>
            <option value="">${libres.length ? t("DAR MANDO…", "GIVE PAD…") : t("SIN MANDOS LIBRES", "NO FREE PADS")}</option>
            ${libres.map((p) => html`<option value=${p.n}>${t("Mando", "Pad")} ${dos(p.n)} · ${p.equipo === "local" ? "Local" : p.equipo === "visitante" ? t("Visita", "Away") : t("Fuera", "Off")}</option>`)}
          </select>`}
      <${Boton} tipo="warn" al=${expulsar}>${t("EXPULSAR", "KICK")}</${Boton}>
      <${Boton} tipo="peligro" deshabilitado=${g.cop} al=${banear}>${t("BANEAR", "BAN")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

// ---- Moderación ----------------------------------------------------------------------------
function Moderacion() {
  const [l, setL] = useState(null);
  const cargar = async () => { const r = await accion("moderacion.listas", {}, { silencioso: true }); if (r) setL(r); };
  useEffect(() => { cargar(); const i = setInterval(cargar, 5000); return () => clearInterval(i); }, []);
  if (!l) return html`<${Vacio} titulo=${t("Cargando listas…", "Loading lists…")} texto=""/>`;
  const quitar = async (nombreAccion, x, pregunta) => {
    if (pregunta && !(await confirmar({ titulo: pregunta, texto: `${x.nombre} · #${x.parsecId}`, si: t("Sí", "Yes") }))) return;
    if (await accion(nombreAccion, { parsecId: x.parsecId })) cargar();
  };
  const banearHist = async (x) => {
    if (await confirmar({ titulo: t(`¿Banear a ${x.nombre}?`, `Ban ${x.nombre}?`), texto: t("Ya no podrá entrar a tus salas.", "They won't be able to join."), si: t("Banear", "Ban"), peligro: true })) {
      if (await accion("moderacion.banear", { parsecId: x.parsecId, nombre: x.nombre })) cargar();
    }
  };
  return html`<div class="rej2">
    <${ListaMod} titulo=${t("BANEADOS", "BANNED")} items=${l.baneados || []} vacio=${t("Nadie baneado. ¡Buena comunidad!", "Nobody banned.")}
      render=${(x) => html`<div class="caja" key=${x.parsecId} style="display:flex;flex-direction:column;gap:8px">
        <div style="display:flex;align-items:center;gap:10px"><${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${32}/>
          <div style="flex:1;min-width:0"><div class="nom" style="font-weight:600">${x.nombre}</div><div class="mono mut" style="font-size:12px">#${x.parsecId}</div></div>
          <${Boton} tipo="mini ok" al=${() => quitar("moderacion.desbanear", x, t("¿Quitar el ban?", "Unban?"))}>${t("DESBANEAR", "UNBAN")}</${Boton}></div>
        <${Campo} valor=${x.motivo || ""} max=${200} placeholder=${t("Motivo (opcional)", "Reason (optional)")} al=${(v) => accion("moderacion.motivo", { parsecId: x.parsecId, motivo: v }, { ok: t("Motivo guardado.", "Saved.") })}/>
      </div>`}/>
    <div class="col">
      <${ListaMod} titulo=${t("MODERADORES", "MODERATORS")} items=${l.mods || []} vacio=${t("Sin moderadores.", "No moderators.")}
        render=${(x) => html`<div class="persona" style="cursor:default" key=${x.parsecId}><${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${32}/>
          <div style="flex:1;min-width:0"><div class="nom">${x.nombre}</div><div class="mono mut" style="font-size:12px">#${x.parsecId}</div></div>
          <${Boton} tipo="mini peligro" al=${() => quitar("moderacion.quitarMod", x)}>${t("QUITAR", "REMOVE")}</${Boton}></div>`}/>
      <${ListaMod} titulo="VIP" items=${l.vips || []} vacio=${t("Sin VIP.", "No VIPs.")}
        render=${(x) => html`<div class="persona" style="cursor:default" key=${x.parsecId}><${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${32}/>
          <div style="flex:1;min-width:0"><div class="nom">${x.nombre}</div><div class="mono mut" style="font-size:12px">#${x.parsecId}</div></div>
          <${Boton} tipo="mini peligro" al=${() => quitar("moderacion.quitarVip", x)}>${t("QUITAR", "REMOVE")}</${Boton}></div>`}/>
      <${ListaMod} titulo=${t("HISTORIAL DE VISITAS", "VISIT HISTORY")} items=${(l.historial || []).slice(0, 60)} vacio=${t("Aún no entró nadie.", "Nobody yet.")}
        render=${(x) => html`<div class="persona" style="cursor:default" key=${"h" + x.parsecId}><${Avatar} nombre=${x.nombre} id=${x.parsecId} t=${32}/>
          <div style="flex:1;min-width:0"><div class="nom">${x.nombre}</div><div class="mono mut" style="font-size:12px">#${x.parsecId}</div></div>
          <${Boton} tipo="mini peligro" al=${() => banearHist(x)}>${t("BANEAR", "BAN")}</${Boton}></div>`}/>
    </div>
  </div>`;
}

function ListaMod({ titulo, items, vacio, render }) {
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:8px">
    <${Titulo} texto=${titulo} derecha=${html`<span class="mono mut">${items.length}</span>`}/>
    ${items.length === 0 ? html`<div class="ayuda">${vacio}</div>` : null}
    <div style="display:flex;flex-direction:column;gap:8px;max-height:420px;overflow:auto">${items.map(render)}</div>
  </${Tarjeta}>`;
}

// ---- Amigos de la web ---------------------------------------------------------------------------
// Presencia de la web: disponible = activo en la plataforma; en_partida = jugando (partido o PES abierto); en_sala = dentro de una sala.
const ESTADO_AMIGO = {
  en_sala: ["acc", "EN SALA", "IN ROOM"], en_partida: ["mor vivo", "JUGANDO", "PLAYING"],
  disponible: ["ok", "ACTIVO", "ACTIVE"], ausente: ["warn", "AUSENTE", "AWAY"], desconectado: ["", "DESCONECTADO", "OFFLINE"],
};

function Amigos({ m }) {
  const web = m.web || {};
  if (web.estado === "sin_vincular") {
    return html`<${Vacio} titulo=${t("Vincula la app con la web", "Link the app to the website")} texto=${t("Con tu cuenta de la liga verás a tus amigos conectados y podrás invitarlos a tu sala con un clic.", "See your friends online and invite them in one click.")}>
      <${Boton} tipo="lleno" al=${() => irA("sync", "web")}><${Icono} n="web" t=${16}/>${t("VINCULAR", "LINK")}</${Boton}></${Vacio}>`;
  }
  const a = m.amigos || { cargados: false, lista: [] };
  if (!a.cargados) return html`<${Vacio} titulo=${t("Cargando amigos…", "Loading friends…")} texto=${t("Se actualizan solos cada pocos segundos.", "They refresh automatically.")}/>`;
  const peso = { en_partida: 0, en_sala: 1, disponible: 2, ausente: 3, desconectado: 4 };
  const orden = [...a.lista].sort((x, y) => (peso[x.estado] ?? 5) - (peso[y.estado] ?? 5) || x.nombre.localeCompare(y.nombre));
  const invitar = (x) => accion("amigos.invitar", { usuarioId: x.usuarioId }, { ok: t(`Invitación enviada a ${x.nombre}.`, `Invite sent to ${x.nombre}.`) });
  const cuenta = (k) => a.lista.filter((x) => x.estado === k).length;
  return html`<div class="col">
    <div class="resumen-presencia">
      ${["disponible", "en_partida", "en_sala"].map((k) => html`<span class="semaforo" key=${k}><span class=${cx("punto", ESTADO_AMIGO[k][0])}></span>${cuenta(k)} ${t(ESTADO_AMIGO[k][1], ESTADO_AMIGO[k][2])}</span>`)}
    </div>
    ${!m.sala?.abierta ? html`<div class="caja" style="border-color:var(--warn)">${t("Abre tu sala para poder invitar.", "Open your room to invite.")}</div>` : null}
    ${orden.length === 0 ? html`<${Vacio} titulo=${t("Sin amigos todavía", "No friends yet")} texto=${t("Agrégalos desde tu perfil en la web de la liga.", "Add them from your profile on the website.")}/>` : null}
    <div class="rej3">${orden.map((x) => {
      const e = ESTADO_AMIGO[x.estado] || ["", String(x.estado || "").toUpperCase(), String(x.estado || "").toUpperCase()];
      return html`<${Tarjeta} key=${x.usuarioId} suave interior="padding:18px 20px;display:flex;flex-direction:column;gap:12px">
        <div style="display:flex;gap:12px;align-items:center"><${Avatar} nombre=${x.nombre} url=${String(x.avatar || "").startsWith("https://") ? x.avatar : ""} t=${46}/>
          <div style="min-width:0"><div style="font-weight:600;font-size:16px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${x.nombre}</div>
            <span class="semaforo"><span class=${cx("punto", e[0])}></span>${t(e[1], e[2])}</span>
            ${x.desde ? html`<div class="mono mut" style="font-size:12px;margin-top:2px">${t("desde", "since")} ${fecha(Date.parse(x.desde))}</div>` : null}</div></div>
        <${Boton} deshabilitado=${!m.sala?.abierta} titulo=${x.estado === "desconectado" ? t("La invitación dura 2 h: la verá al conectarse.", "The invite lasts 2 h.") : ""} al=${() => invitar(x)}><${Icono} n="enviar" t=${15}/>${t("INVITAR A MI SALA", "INVITE")}</${Boton}>
      </${Tarjeta}>`;
    })}</div>
  </div>`;
}
