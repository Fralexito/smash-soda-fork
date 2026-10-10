// =============================================================================
//  PERSONAS EN LA PORTADA: quién está en la sala con su ping y su puesto, en una
//  sola lista (Sala › Resumen). Al tocar a alguien se abre su ficha en un panel
//  lateral sin cambiar de pantalla: puesto, conexión de 2 min, roles, permisos,
//  carta de la liga, expulsar y banear. Mismas acciones que Gente y Mandos.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, confirmar, pedirVista } from "../tienda.js";
import { Tarjeta, Titulo, Avatar, Chip, Boton, AjusteSw, Minigrafica, Ping, PanelLateral, nivelPing, cx, dos } from "../ui.js";
import { Carta } from "./gente.js";

/** El anfitrión primero y luego los invitados reales. */
export function personasDeSala(m) {
  const r = [];
  const lista = m.mandos?.lista || [];
  if (m.sala?.hostId) {
    const asiento = m.mandos?.host || (lista.find((p) => p.ocupado && p.parsecId === m.sala.hostId)?.n ?? 0);
    r.push({ parsecId: m.sala.hostId, nombre: m.sala.hostNombre || t("Anfitrión", "Host"), host: true, ping: -1, mando: asiento });
  }
  for (const g of m.invitados || []) if (!g.falso) r.push(g);
  return r;
}
const redDe = (m, id) => (m.red || []).find((r) => r.parsecId === id) || null;
const equipo = (eq) => (eq === "local" ? "Local" : eq === "visitante" ? t("Visita", "Away") : t("Fuera", "Off"));

/** Selector de puesto: el mismo en la lista y en la ficha. */
export function SelectorPuesto({ g, m, compacto = false }) {
  const lista = m.mandos?.lista || [];
  const actual = g.mando || 0;
  const esclavo = !!m.mandos?.esclavo;
  const soloMira = g.rolWeb === "espectador";
  const motivo = esclavo ? t("Los turnos controlan los mandos ahora.", "Hotseat controls the pads now.")
    : soloMira ? t("La lista de la web lo marca como espectador.", "The web list marks them as a spectator.")
    : lista.length === 0 ? t("No hay mandos virtuales.", "No virtual pads.") : "";
  const eq = actual ? lista[actual - 1]?.equipo : "";
  const elegir = (v) => {
    const n = Number(v);
    if (n === actual) return;
    if (g.host) return accion("mandos.tomar", { numero: n });
    if (n === 0) return accion("mandos.liberar", { indice: actual - 1 });
    if (actual > 0 && lista[n - 1]?.ocupado) return accion("mandos.intercambiar", { a: actual - 1, b: n - 1 });
    return accion("mandos.asignar", { indice: n - 1, parsecId: g.parsecId });
  };
  return html`<select class=${cx("campo selector-puesto", compacto && "compacto", eq && "eq-" + eq)} value=${String(actual)}
      disabled=${!!motivo && !(actual && !esclavo)} title=${motivo} aria-label=${t("Mando de ", "Pad for ") + g.nombre}
      onClick=${(e) => e.stopPropagation()} onChange=${(e) => elegir(e.currentTarget.value)}>
    <option value="0">${t("MIRANDO", "WATCHING")}</option>
    ${lista.map((p) => html`<option value=${String(p.n)}>${t("MANDO", "PAD")} ${dos(p.n)} · ${equipo(p.equipo)}${p.ocupado && p.n !== actual ? " · " + p.jugador : ""}</option>`)}
  </select>`;
}

/** Tarjeta «EN LA SALA» de la portada. */
export function PersonasSala({ m }) {
  const [ficha, setFicha] = useState(null);
  const gente = personasDeSala(m);
  const invitados = gente.filter((g) => !g.host).length;
  return html`<${Tarjeta} estilo="flex:none" interior="padding:18px 22px;display:flex;flex-direction:column;gap:10px">
    <${Titulo} texto=${t("EN LA SALA", "IN THE ROOM")} derecha=${html`<span class="mono mut" style="font-size:13px">${invitados} ${t(invitados === 1 ? "INVITADO" : "INVITADOS", "GUESTS")} · ${t("toca para ver su ficha", "tap for their card")}</span>`}/>
    ${invitados === 0 ? html`<div class="ayuda">${t("Aún no entra nadie. Copia el enlace y compártelo: cada persona aparecerá aquí con su ping y su mando.", "Nobody yet. Share the link: everyone shows up here with ping and pad.")}</div>` : null}
    <div class="lista-sala">${gente.map((g) => {
      const perfil = m.perfiles?.[g.parsecId];
      const r = redDe(m, g.parsecId);
      return html`<div key=${g.parsecId} class="persona entra-persona" role="button" tabindex="0"
          onClick=${() => setFicha(g.parsecId)} onKeyDown=${(e) => (e.key === "Enter" || e.key === " ") && e.target === e.currentTarget && (e.preventDefault(), setFicha(g.parsecId))}>
        <${Avatar} nombre=${perfil?.nombre || g.nombre} url=${perfil?.avatar_url} id=${g.parsecId} brillo=${!!g.mando} t=${38}/>
        <div style="flex:1;min-width:0">
          <div class="nom">${g.nombre}</div>
          <div class="sub">${g.host ? html`<${Chip} tipo="acc">${t("TÚ", "YOU")}</${Chip}>` : null}
            ${g.mod ? html`<${Chip} tipo="mod">MOD</${Chip}>` : null}${g.vip ? html`<${Chip} tipo="vip">VIP</${Chip}>` : null}
            ${g.rolWeb === "espectador" ? html`<${Chip}>${t("SOLO MIRA", "WATCH ONLY")}</${Chip}>` : null}</div>
        </div>
        ${g.host ? null : html`<${Ping} ms=${g.ping >= 0 ? g.ping : r?.ultimo ?? -1} semaforo=${r?.semaforo}/>`}
        <${SelectorPuesto} g=${g} m=${m} compacto/>
      </div>`;
    })}</div>
    ${ficha != null ? html`<${PanelPersona} id=${ficha} m=${m} alCerrar=${() => setFicha(null)}/>` : null}
  </${Tarjeta}>`;
}

const ROL_WEB = {
  jugador: ["acc", "EN LISTA · JUGADOR", "LISTED · PLAYER"],
  espectador: ["", "EN LISTA · ESPECTADOR", "LISTED · SPECTATOR"],
  no_listado: ["vip", "NO ESTÁ EN LA LISTA", "NOT LISTED"],
};

/** Ficha lateral de una persona. */
export function PanelPersona({ id, m, alCerrar }) {
  useEffect(() => pedirVista("sala", "red"), []);   // la gráfica de 2 min solo llega con «sala/red»
  const g = personasDeSala(m).find((x) => x.parsecId === id);
  if (!g) return html`<${PanelLateral} titulo=${t("Ya no está", "Gone")} alCerrar=${alCerrar}><div class="ayuda">${t("Esta persona salió de la sala.", "This person left the room.")}</div></${PanelLateral}>`;
  const perfil = m.perfiles?.[g.parsecId];
  const r = redDe(m, g.parsecId);
  const rol = ROL_WEB[g.rolWeb];
  const expulsar = async () => {
    if (await confirmar({ titulo: t(`¿Expulsar a ${g.nombre}?`, `Kick ${g.nombre}?`), texto: t("Sale de la sala, pero puede volver con el enlace.", "They can rejoin with the link."), si: t("Expulsar", "Kick"), peligro: true })) {
      if (await accion("gente.expulsar", { parsecId: g.parsecId, nombre: g.nombre }, { ok: t("Expulsado.", "Kicked.") })) alCerrar();
    }
  };
  const banear = async () => {
    if (await confirmar({ titulo: t(`¿Banear a ${g.nombre}?`, `Ban ${g.nombre}?`), texto: t("No podrá volver a tus salas. Se deshace en Gente › Moderación.", "Undo in People › Moderation."), si: t("Banear", "Ban"), peligro: true })) {
      if (await accion("gente.banear", { parsecId: g.parsecId, nombre: g.nombre }, { ok: t("Baneado.", "Banned.") })) alCerrar();
    }
  };
  return html`<${PanelLateral} titulo=${g.nombre} sub=${g.host ? t("ANFITRIÓN", "HOST") : "#" + g.parsecId} alCerrar=${alCerrar}>
    <div style="display:flex;gap:14px;align-items:center;flex-wrap:wrap">
      <${Avatar} nombre=${perfil?.nombre || g.nombre} url=${perfil?.avatar_url} id=${g.parsecId} brillo t=${60}/>
      <div style="display:flex;gap:6px;flex-wrap:wrap">
        ${g.mod ? html`<${Chip} tipo="mod">MOD</${Chip}>` : null}${g.vip ? html`<${Chip} tipo="vip">VIP</${Chip}>` : null}
        ${rol ? html`<${Chip} tipo=${rol[0]}>${t(rol[1], rol[2])}</${Chip}>` : null}
        ${perfil ? html`<${Chip} tipo="acc2">${t("LIGA", "LEAGUE")}: ${perfil.nombre}</${Chip}>` : null}
        ${g.cop ? html`<${Chip} tipo="bad">${t("MOD OFICIAL", "OFFICIAL MOD")}</${Chip}>` : null}</div>
    </div>
    <div class="ficha-bloque"><div class="lab acc">${t("MANDO", "PAD")}</div><${SelectorPuesto} g=${g} m=${m}/>
      ${g.host ? html`<div class="ayuda">${t("Tu mando físico pasa a ese puesto. Atajo: Ctrl+Alt+número.", "Your physical pad takes that seat. Shortcut: Ctrl+Alt+number.")}</div>` : null}</div>
    ${g.host ? null : html`
    <div class="ficha-bloque"><div class="lab acc">${t("CONEXIÓN · 2 MIN", "CONNECTION · 2 MIN")}</div>
      <div style="display:flex;align-items:center;gap:10px"><${Ping} ms=${r ? r.ultimo : g.ping} semaforo=${r?.semaforo} grande/>
        ${r?.alerta ? html`<${Chip} tipo="bad">${t("LAG AHORA", "LAGGING")}</${Chip}>` : null}</div>
      ${r ? html`<${Minigrafica} serie=${r.serie} alto=${56} color=${`var(--${nivelPing(r.media, r.semaforo) || "acc"})`}/>
        <div class="rej4 mono" style="gap:8px;font-size:12px">
          <div><div class="mut">${t("MEDIA", "AVG")}</div><b>${r.media >= 0 ? r.media : "—"}</b></div>
          <div><div class="mut">${t("MÁXIMO", "MAX")}</div><b>${r.maximo >= 0 ? r.maximo : "—"}</b></div>
          <div><div class="mut">JITTER</div><b>${r.jitter >= 0 ? r.jitter : "—"}</b></div>
          <div><div class="mut">${t("PICOS", "SPIKES")}</div><b>${r.picos}</b></div></div>` : null}
    </div>
    <div class="ficha-bloque"><div class="lab acc">${t("ROLES Y PERMISOS", "ROLES AND PERMISSIONS")}</div>
      <${AjusteSw} titulo="VIP" valor=${g.vip} al=${() => accion("gente.vip", { parsecId: g.parsecId, nombre: g.nombre })}/>
      <${AjusteSw} titulo=${t("Moderador", "Moderator")} valor=${g.mod} al=${() => accion("gente.mod", { parsecId: g.parsecId, nombre: g.nombre })}/>
      <${AjusteSw} titulo=${t("Permitir teclado", "Allow keyboard")} valor=${g.teclado} al=${(v) => accion("gente.teclado", { parsecId: g.parsecId, si: v })}/>
      <${AjusteSw} titulo=${t("Permitir ratón", "Allow mouse")} valor=${g.raton} al=${(v) => accion("gente.raton", { parsecId: g.parsecId, si: v })}/>
    </div>
    ${perfil?.carta ? html`<div class="ficha-bloque"><div class="lab acc">${t("CARTA EN LA LIGA", "LEAGUE CARD")}</div><${Carta} perfil=${perfil}/></div>` : null}
    <div class="ficha-bloque" style="flex-direction:row;gap:10px">
      <${Boton} tipo="warn" al=${expulsar}>${t("EXPULSAR", "KICK")}</${Boton}>
      <${Boton} tipo="peligro" deshabilitado=${g.cop} al=${banear}>${t("BANEAR", "BAN")}</${Boton}>
    </div>`}
  </${PanelLateral}>`;
}
