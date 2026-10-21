// =============================================================================
//  SYNC › TRASPASOS. «Pasar fichajes entre amigos», sin elegir papel: enviar y recibir van juntos.
//    ENVIAR MIS FICHAJES: tu código (copiar / cambiar) y lo detectado al guardar en Editar.
//    RECIBIR FICHAJES DE UN AMIGO: pega su código; estado y fichajes recibidos.
//    «¿Algo no funciona?» y «Ajustes de traspasos» (0.3.45: ahí van todos los interruptores — oficiales, aprobar al enviar/recibir,
//    mantener mis fichajes, modo prueba —, la carpeta compartida y apagar; arriba solo quedan el código y recibir).
//  La lógica (traspasos.estado / config / accion) no cambia: solo cómo se presenta. Guía: docs/FICHAJES-Y-SYNC.md («TRASPASOS-EN-VIVO»)
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar, elegirPestana } from "../tienda.js";
import { Tarjeta, Titulo, AjusteSw, Chip, Boton, cx } from "../ui.js";

// Estado de la conexión (web o carpeta): lo mismo para el anfitrión («transporte:<clave>») y para el amigo (la clave sola)
const CONEXION = {
  ok: ["ok", "Conectado a la web.", "Connected to the web."],
  web_sin_endpoints: ["warn", "La web todavía no tiene esta función. Se vuelve a probar sola.", "The web doesn't have this feature yet. It retries by itself."],
  sin_cuenta: ["bad", "Esta PC no tiene tu cuenta de la liga conectada.", "This PC isn't linked to your league account."],
  sin_permiso: ["bad", "La web no te deja ser anfitrión. Pídele al staff que apruebe tu cuenta.", "The web doesn't let you host. Ask the staff to approve your account."],
  pendiente_aprobacion: ["warn", "Tu anfitrión todavía no te ha aceptado.", "Your host hasn't accepted you yet."],
  limite: ["warn", "Hay demasiadas peticiones. Espero un momento.", "Too many requests. Waiting a moment."],
  sin_conexion: ["warn", "No hay conexión con la web. Se reintenta sola.", "No connection to the web. It retries by itself."],
  respuesta_rara: ["warn", "La web contestó algo raro. Se reintenta sola.", "The web answered something odd. It retries by itself."],
  sin_anfitrion: ["warn", "Escribe el código de tu anfitrión para seguirlo.", "Type your host's code to follow them."],
  liga_pausada: ["warn", "La liga está en pausa. Espero.", "The league is paused. Waiting."],
};
const ESTADO_LOTE = { por_aprobar: ["warn", "POR APROBAR", "TO APPROVE"], rechazado: ["", "RECHAZADO", "REJECTED"], pendiente: ["warn", "PENDIENTE", "PENDING"], aplicado: ["ok", "PUESTO", "APPLIED"], parcial: ["vip", "A MEDIAS", "PARTIAL"], saltado: ["bad", "NO SE PUDO", "SKIPPED"], deshecho: ["", "DESHECHO", "UNDONE"], propio: ["", "TUYO", "YOURS"], error: ["bad", "ERROR", "ERROR"] };
const ESTADO_CAMBIO = { aplicado: ["ok", "puesto", "applied"], ya_estaba: ["", "ya estaba", "already there"], saltado: ["bad", "no se pudo", "skipped"], reemplazado: ["", "lo cambió un fichaje posterior", "replaced by a later one"], pendiente: ["warn", "pendiente", "pending"], publicado: ["ok", "enviado", "sent"] };
const MODO_TXT = {
  "AUTO-SIEMPRE": ["El juego carga el fichaje solo al volver al menú principal y entrar a un modo.", "The game loads the transfer by itself when you go back to the main menu and enter a mode."],
  "AUTO-FICHAJES": ["El juego carga el fichaje solo al volver al menú principal y entrar a un modo.", "The game loads the transfer by itself when you go back to the main menu and enter a mode."],
  "ACTIVAR": ["Para verlo pulsa en el juego: Partido → Datos Actual. en vivo → Activar.", "To see it press in the game: Match → Live data update → Activate."],
};

/** Frase corta del anfitrión según lo que hace Link ahora: [color, frase]. */
function estadoAnfitrion(a, w, pend) {
  const est = a.estado || "";
  if (est.startsWith("transporte:")) { const c = CONEXION[est.slice(11)]; return c ? [c[0], t(c[1], c[2])] : ["warn", est]; }
  if (pend.length) return ["warn", a.masivo ? t("Son demasiados cambios de golpe. Si cargaste otro archivo de guardado, descártalos.", "That's too many changes at once. If you loaded another save file, discard them.") : t("Fichaje detectado: pulsa PUBLICAR.", "Transfer detected: press PUBLISH.")];
  const clave = est.split(":")[0];
  switch (clave) {
    case "publicado": return ["ok", t("Fichaje enviado. Tu amigo lo recibe en unos segundos.", "Transfer sent. Your friend gets it in a few seconds.")];
    case "descartado": return ["", t("Descartaste ese fichaje: no se envió.", "You discarded that transfer: it wasn't sent.")];
    case "esperando_que_termine_de_guardar": return ["warn", t("PES está guardando. Espero a que termine.", "PES is saving. Waiting for it to finish.")];
    case "referencia_borrada": return ["warn", t("Empiezo de cero: la próxima vez que PES guarde, solo anoto cómo está todo.", "Starting over: the next time PES saves, I only note how everything is.")];
    case "sin_option_file": return ["bad", t("No encuentro tu archivo de guardado. Indícalo en «Mi archivo de guardado» (ENVIAR).", "I can't find your save file. Set it under «My save file» (SEND).")];
    case "apagado": return ["", t("Apagado.", "Off.")];
    case "pendiente_masivo": return ["warn", t("Son demasiados cambios de golpe. Si cargaste otro archivo de guardado, descártalos.", "That's too many changes at once. If you loaded another save file, discard them.")];
    case "pendiente_de_confirmar": return ["warn", t("Fichaje detectado: pulsa PUBLICAR.", "Transfer detected: press PUBLISH.")];
    default:
      if (est.startsWith("no_se_pudo_leer")) return ["warn", t("No pude leer tu archivo de guardado (quizá se está guardando). Reintento solo.", "I couldn't read your save file (maybe it's saving). Retrying by itself.")];
      return ["ok", t("Esperando tu fichaje: haz el traspaso en Editar y guarda.", "Waiting for your transfer: make it in Edit and save.")];
  }
}

export function Traspasos() {
  const [d, setD] = useState(null);
  const [carpeta, setCarpeta] = useState("");
  const [opt, setOpt] = useState("");
  const [codigo, setCodigo] = useState("");
  const cargar = async () => { const r = await accion("traspasos.estado", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { cargar(); const id = setInterval(cargar, 3000); return () => clearInterval(id); }, []);
  const poner = async (campos, ok) => { const r = await accion("traspasos.config", campos); if (r) { setD(r); if (ok) avisar(ok, "ok", 5000); } };
  const hacer = async (que, extra = {}) => { const r = await accion("traspasos.accion", { accion: que, ...extra }); if (r) setD(r); };
  // Sin papel que elegir: enviar y recibir se encienden solos una vez (salvo que se hayan apagado a mano)
  const [autoHecho, setAutoHecho] = useState(false);
  useEffect(() => {
    if (!d || autoHecho) return;
    setAutoHecho(true);
    let apagadoAMano = false;
    try { apagadoAMano = localStorage.getItem("phoenix.traspasos.apagado") === "1"; } catch { /* nada */ }
    const aj0 = d.ajustes || {};
    if (!apagadoAMano && !(aj0.publicar && aj0.recibir)) poner({ publicar: true, recibir: true });
  }, [d]);
  if (!d) return html`<${Tarjeta} interior="padding:22px 24px"><div class="ayuda">${t("Revisando…", "Checking…")}</div></${Tarjeta}>`;

  const aj = d.ajustes || {};
  const a = d.anfitrion || {};
  const m = d.amigo || {};
  const w = d.web || {};
  const esWeb = (aj.transporte || "web") === "web";
  const pend = a.pendientes || [];
  const lista = m.historial || [];
  const porAprobar = lista.filter((l) => l.estado === "por_aprobar");
  const nombre = (c) => c.jugadorNombre || ("#" + c.jugador);
  const modo = d.modoRecarga || "AUTO-SIEMPRE";
  // Sin «papel»: enviar y recibir van siempre juntos (el motor lo permite). El modo prueba (seguirme a mí mismo) va aparte.
  const encendido = !!(aj.publicar || aj.recibir);
  const prueba = !!aj.prueba;
  const sig = w.siguiendo || [];
  const canal = w.canal;
  const noAprobado = w.estado === "sin_permiso";   // la cuenta no puede enviar (no es anfitrión): recibir sí
  const [cp, ct, cen] = CONEXION[w.estado || "ok"] || ["", w.estado || "", w.estado || ""];
  const copiar = async (texto) => { try { await navigator.clipboard.writeText(texto); avisar(t("Copiado.", "Copied."), "ok", 2500); } catch { avisar(t("No pude copiar: selecciónalo y cópialo a mano.", "Couldn't copy: select it and copy by hand."), "bad", 4000); } };
  const irMiPc = () => elegirPestana("mipc");
  const apagar = () => { try { localStorage.setItem("phoenix.traspasos.apagado", "1"); } catch { /* nada */ } poner({ publicar: false, recibir: false, prueba: false }, t("Traspasos apagados.", "Transfers turned off.")); };
  const encender = () => { try { localStorage.removeItem("phoenix.traspasos.apagado"); } catch { /* nada */ } poner({ publicar: true, recibir: true }, t("Traspasos encendidos.", "Transfers on.")); };

  // ── «¿Algo no funciona?»: comprobaciones de lo que se usa (enviar si la cuenta puede; recibir si sigues a alguien) ──
  const diag = d.diag || {};
  const sitiosSider = (diag.sitios || []).filter((x) => x.sider);
  const sinRaiz = sitiosSider.filter((x) => !x.raiz).map((x) => x.nombre);
  const arch = a.archivo || {};
  const F = (ok, titulo, detalle, pista, boton) => ({ ok, titulo, detalle, pista, boton });
  const aMiPc = { texto: t("IR A MI JUEGO", "GO TO MY GAME"), al: irMiPc };
  const filaCuenta = F(!!w.vinculada, t("Cuenta de la liga conectada", "League account linked"), "", t("Conecta tu cuenta en «Mi juego».", "Link this PC to your account in «My game»."), aMiPc);
  const filasEnviar = [
    F(!!arch.existe, t("Archivo de guardado encontrado", "Save file found"), arch.existe ? `${arch.ruta} · ${t("cambió el", "changed on")} ${arch.fecha}` : "", t("No encuentro tu archivo EDIT00000000: indícalo en «Mi archivo de guardado» de ENVIAR.", "I can't find your EDIT00000000 file: set it under «My save file» in SEND.")),
    F(!!a.hayReferencia, t("Punto de partida anotado", "Starting point noted"), a.hayReferencia ? t("Solo se enviará lo que cambies desde ahora.", "Only what you change from now on will be sent.") : "", t("Se anota sola en unos segundos. Si no, guarda una vez en PES.", "It's noted by itself in a few seconds. If not, save once in PES.")),
    F(esWeb ? (w.estado === "ok" && d.webDisponible === true) : !!aj.carpeta, esWeb ? t("Conexión con la web", "Connection to the web") : t("Carpeta compartida elegida", "Shared folder chosen"), "",
      esWeb ? (w.vinculada ? t(CONEXION[w.estado]?.[1] || "Comprobando…", CONEXION[w.estado]?.[2] || "Checking…") : t("Primero une la cuenta.", "Link the account first.")) : t("Elige la carpeta en «Ajustes de traspasos».", "Choose the folder under «Transfer settings».")),
  ];
  const filasRecibir = [
    F(!!diag.ok, t("Juego encontrado", "Game found"), diag.ok ? diag.juego : "", t("Pulsa ARREGLAR TODO en «Mi juego»: busca el juego solo.", "Press FIX EVERYTHING in «My game»: it finds the game by itself."), aMiPc),
    F(!!diag.ok && sitiosSider.length > 0 && sinRaiz.length === 0, t("El juego lee los traspasos", "The game reads the transfers"), diag.ok && sinRaiz.length === 0 && sitiosSider.length ? t(`Listo en ${sitiosSider.length} ${sitiosSider.length === 1 ? "sitio" : "sitios"}.`, `Ready in ${sitiosSider.length} ${sitiosSider.length === 1 ? "place" : "places"}.`) : "",
      !diag.ok ? t("Primero hace falta saber dónde está el juego.", "The game folder is needed first.") : !sitiosSider.length ? t("No veo la configuración de Sider en el juego.", "I can't see Sider's setup in the game.") : t(`Falta en: ${sinRaiz.join(", ")}. Con PES cerrado, pulsa ARREGLAR TODO en «Mi juego».`, `Missing in: ${sinRaiz.join(", ")}. With PES closed, press FIX EVERYTHING in «My game».`), aMiPc),
    F(!!diag.lua && diag.lua.estado === "instalado", t("Archivo de Phoenix instalado en el juego", "Phoenix file installed in the game"), diag.lua && diag.lua.version ? `v${diag.lua.version}` : "",
      diag.lua && diag.lua.estado === "desactualizado" ? t("Hay una versión nueva. Con PES cerrado, pulsa ARREGLAR TODO en «Mi juego».", "A newer version exists. With PES closed, press FIX EVERYTHING in «My game».") : t("Con PES cerrado, pulsa ARREGLAR TODO en «Mi juego».", "With PES closed, press FIX EVERYTHING in «My game»."), aMiPc),
  ];
  const chequeo = !encendido ? [] : [
    { ...filaCuenta, grupo: "" },
    ...(aj.publicar && !noAprobado ? filasEnviar.map((f) => ({ ...f, grupo: t("ENVIAR", "SEND") })) : []),
    ...(aj.recibir && (sig.length > 0 || prueba || !esWeb) ? filasRecibir.map((f) => ({ ...f, grupo: t("RECIBIR", "RECEIVE") })) : []),
  ];
  const faltan = chequeo.filter((f) => !f.ok).length;

  const conexionMsg = () => !w.vinculada ? html`<div class="caja" style="display:flex;flex-direction:column;gap:8px">
      <div style="font-size:14px"><span class="punto bad"></span> ${t("Esta PC todavía no tiene tu cuenta de la liga conectada.", "This PC isn't linked to your league account yet.")}</div>
      <div><${Boton} tipo="lleno mini" al=${irMiPc}>${t("CONECTAR MI CUENTA", "LINK MY ACCOUNT")}</${Boton}></div></div>`
    : (w.estado !== "ok" && !noAprobado) || d.webDisponible === false ? html`<div class="caja" style="display:flex;flex-direction:column;gap:8px">
      <div style="font-size:14px"><span class=${cx("punto", cp)}></span> ${t(ct, cen)}${w.esperaSeg > 0 && w.estado !== "ok" ? t(` Reintento en ${w.esperaSeg} s.`, ` Retry in ${w.esperaSeg} s.`) : ""}</div>
      ${w.texto ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-word">${w.texto}</div>` : null}
      <div><${Boton} tipo="suave mini" al=${() => hacer("reintentar")}>${t("VOLVER A COMPROBAR", "CHECK AGAIN")}</${Boton}></div></div>` : null;

  const [pA, fraseA] = estadoAnfitrion(a, w, pend);
  const amigoEstado = (() => {
    if (!aj.recibir) return null;
    if (m.ultimoAplicado && (m.estado === "al_dia" || m.estado === "aplicado")) return ["ok", m.ultimoAplicado.texto, m.ultimoAplicado.accion];
    if (m.estado === "al_dia" || m.estado === "aplicado") return ["ok", t("Esperando fichajes. Cuando tu amigo envíe uno, aparece aquí.", "Waiting for transfers. When your friend sends one, it shows up here."), ""];
    if (m.estado === "esperando") return ["warn", m.mensaje || t("Hay un fichaje esperando.", "A transfer is waiting."), d.pesAbierto ? t("PES está abierto: cierra PES y se pone solo.", "PES is open: close PES and it's applied by itself.") : ""];
    if (m.estado === "por_aprobar") return ["warn", t("Llegaron fichajes: acéptalos o recházalos.", "Transfers arrived: accept or reject them."), ""];
    if (m.estado === "sin_transporte" || m.estado === "sin_anfitrion") return null;
    const c = CONEXION[m.estado];
    if (c) return [c[0], t(c[1], c[2]), ""];
    return ["", m.estado || "", ""];
  })();
  const infoPes = d.pesAbierto ? t("PES está abierto. Cierra y abre PES para ver el cambio, o espera la recarga automática.", "PES is open. Close and reopen PES to see the change, or wait for the automatic reload.") : t("PES está cerrado: se pone al momento y lo ves al abrir PES.", "PES is closed: it's applied right away and you'll see it when you open PES.");
  const tarjeta = "padding:20px 22px;display:flex;flex-direction:column;gap:12px";

  // ── Cambios OFICIALES de Phoenix (la web, docs/PEDIDOS-A-LA-WEB.md («CONTRATO-WEB-PES»)): llegan solos a todos, sin código; aparte de amigos ──
  const oficiales = lista.filter((l) => l.oficial).slice(0, 5);
  const tarjetaOficial = html`<${Tarjeta} interior=${tarjeta}>
      <${Titulo} texto=${t("CAMBIOS OFICIALES DE PHOENIX", "OFFICIAL PHOENIX CHANGES")} derecha=${html`<${Chip} tipo=${aj.oficial !== false ? "acc" : ""}>${aj.oficial !== false ? t("ENCENDIDO", "ON") : t("APAGADO", "OFF")}</${Chip}>`}/>
      <${AjusteSw} titulo=${t("Recibir los cambios oficiales de Phoenix", "Receive official Phoenix changes")}
        desc=${aj.oficial !== false ? t("Encendido: los fichajes que el staff hace en la web llegan solos a tu PES (no hace falta código).", "On: the transfers the staff makes on the web reach your PES by themselves (no code needed).") : t("Apagado: no recibes los cambios oficiales y se quitan de tu juego.", "Off: you don't receive official changes and they're removed from your game.")}
        valor=${aj.oficial !== false} al=${(v) => poner({ oficial: v })}/>
      ${aj.oficial !== false && !w.vinculada ? html`<div class="ayuda" style="font-size:13px">${t("Para recibirlos, conecta tu cuenta en «Mi juego».", "To receive them, link this PC to your account in «My game».")}</div>` : null}
      ${oficiales.length ? html`<div class="col" style="gap:4px">${oficiales.map((l) => html`<div style="font-size:13px"><span class=${cx("punto", l.estado === "aplicado" ? "ok" : l.estado === "saltado" || l.estado === "error" ? "bad" : "warn")}></span> ${(l.cambios || []).map((c) => `${nombre(c)} → ${c.hastaNombre || "#" + c.hasta}`).join(", ")} <span class="ayuda">· ${l.estado}${l.motivo ? " · " + l.motivo : ""}</span></div>`)}</div>`
        : aj.oficial !== false ? html`<div class="ayuda" style="font-size:13px">${t("Todavía no llegó ningún cambio oficial.", "No official change has arrived yet.")}</div>` : null}
    </${Tarjeta}>`;

  if (!encendido) return html`<div class="col" style="gap:14px">
    ${tarjetaOficial}
    <${Tarjeta} interior=${tarjeta}>
      <${Titulo} texto=${t("PASAR FICHAJES ENTRE AMIGOS", "PASS TRANSFERS BETWEEN FRIENDS")} derecha=${html`<${Chip}>${t("APAGADO", "OFF")}</${Chip}>`}/>
      <div style="font-size:15px">${t("Lo que fichas en tu PES llega al PES de tus amigos, y lo que fichan ellos al tuyo, con un código.", "What you sign in your PES reaches your friends' PES, and theirs reaches yours, with a code.")}</div>
      <div><${Boton} tipo="lleno" al=${encender}>${t("ENCENDER", "TURN ON")}</${Boton}></div>
    </${Tarjeta}></div>`;

  return html`<div class="col" style="gap:14px">
    <${Tarjeta} interior="padding:18px 22px;display:flex;flex-direction:column;gap:6px">
      <${Titulo} texto=${t("PASAR FICHAJES ENTRE AMIGOS", "PASS TRANSFERS BETWEEN FRIENDS")} derecha=${html`<${Chip} tipo="acc">${t("ENCENDIDO", "ON")}</${Chip}>`}/>
      <div style="font-size:14px">${t("Para mandar tus fichajes, dale tu código a tu amigo. Para recibir los suyos, pega su código. Puedes hacer las dos cosas a la vez.", "To send your transfers, give your friend your code. To receive theirs, paste their code. You can do both at once.")}</div>
      ${esWeb ? conexionMsg() : html`<div class="ayuda" style="font-size:13px">${t("Usas una carpeta compartida en vez de la web: no hace falta código.", "You use a shared folder instead of the web: no code needed.")}</div>`}
      ${d.mensaje ? html`<div class="caja" style="font-size:13px">${d.mensaje}</div>` : null}
      <div class="ayuda" style="font-size:13px"><span class=${cx("punto", aj.oficial !== false ? "ok" : "")}></span> ${aj.oficial !== false ? t("Los cambios oficiales de Phoenix llegan solos, sin código.", "Official Phoenix changes arrive by themselves, no code.") : t("Cambios oficiales de Phoenix apagados (en «Ajustes de traspasos»).", "Official Phoenix changes off (in «Transfer settings»).")}</div>
    </${Tarjeta}>

    <div class="rej2" style="align-items:start">
      <${Tarjeta} interior=${tarjeta}>
        <${Titulo} texto=${t("ENVIAR MIS FICHAJES", "SEND MY TRANSFERS")}/>
        ${esWeb && w.vinculada ? (canal ? html`
          <div class="ayuda" style="font-size:14px">${t("Tu código. Pásaselo a tu amigo: él lo pega en RECIBIR de su Link.", "Your code. Give it to your friend: they paste it in RECEIVE in their Link.")}</div>
          <div class="mono acc" style="font-size:36px;letter-spacing:6px;font-weight:700;word-break:break-all">${canal.codigo}</div>
          <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
            <${Boton} tipo="lleno" al=${() => copiar(canal.codigo)}>${t("COPIAR CÓDIGO", "COPY CODE")}</${Boton}>
            <${Boton} tipo="suave mini" titulo=${t("El código anterior deja de servir para unirse. Quienes ya te siguen siguen.", "The old code stops working for joining. People already following stay.")} al=${() => hacer("rotarCodigo")}>${t("CAMBIAR CÓDIGO", "CHANGE CODE")}</${Boton}>
          </div>
          <div class="ayuda" style="font-size:13px">${(w.meSiguen || []).length ? t("Lo reciben: ", "Receiving: ") + w.meSiguen.join(", ") : t("Todavía nadie recibe tus fichajes.", "Nobody receives your transfers yet.")}</div>`
          : html`<div class="ayuda" style="font-size:14px">${noAprobado ? t("Tu cuenta todavía no puede enviar fichajes (no está aprobada como anfitrión). Recibir sí puedes. Si quieres enviar, pídeselo al staff.", "Your account can't send transfers yet (not approved as host). You can still receive. To send, ask the staff.") : t("Pidiendo tu código a la web…", "Getting your code from the web…")}</div>`) : null}
        ${!noAprobado ? html`
          <div style="display:flex;align-items:flex-start;gap:10px"><span class=${cx("punto", pA)} style="margin-top:6px"></span><span style="font-size:15px">${fraseA}</span></div>
          ${pend.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px">
            ${pend.slice(0, 12).map((c) => html`<div class="mono" style="font-size:13px" key=${c.jugador_id + "-" + c.hasta_id}>· ${c.jugador || ("#" + c.jugador_id)}: ${c.desde || c.desde_id} → ${c.hasta || c.hasta_id}</div>`)}
            ${pend.length > 12 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${pend.length - 12} más`, `and ${pend.length - 12} more`)}</div>` : null}
            <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:4px">
              <${Boton} tipo="lleno" deshabilitado=${!d.transporteListo} titulo=${d.transporteListo ? "" : t("Antes resuelve la conexión", "Fix the connection first")}
                al=${async () => { if (await confirmar({ titulo: t("¿Enviar estos fichajes?", "Send these transfers?"), texto: t("Se envían a quienes tienen tu código y su Link los pone en su juego.", "They're sent to whoever has your code and their Link puts them in their game."), si: t("Publicar", "Publish") })) hacer("publicar"); }}>${t("PUBLICAR", "PUBLISH")}</${Boton}>
              <${Boton} tipo="suave mini" al=${() => hacer("descartar")}>${t("DESCARTAR", "DISCARD")}</${Boton}>
            </div></div>` : null}
          ${a.publicados ? html`<div class="ayuda" style="font-size:13px">${t(`Enviados en esta sesión: ${a.publicados}`, `Sent this session: ${a.publicados}`)}</div>` : null}
          <div class="ayuda" style="font-size:13px">${t("Se envía cuando guardas y sales de Editar en PES.", "It's sent when you save and leave Edit in PES.")}</div>
          <details class="sec-detalle"><summary>${t("Mi archivo de guardado", "My save file")}</summary>
            <div class="ayuda" style="font-size:13px;margin-top:6px">${t("Si Link no encuentra tu archivo de guardado, pega su ruta (el archivo EDIT00000000 o su carpeta «save»):", "If Link can't find your save file, paste its path (the EDIT00000000 file or its «save» folder):")}</div>
            <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${opt} placeholder="C:\\Users\\…\\Documents\\KONAMI\\…\\save" onInput=${(e) => setOpt(e.currentTarget.value)}/></div>
              <${Boton} tipo="suave mini" deshabilitado=${!opt.trim()} al=${async () => { await poner({ optionFile: opt.trim() }, t("Archivo de guardado anotado.", "Save file noted.")); setOpt(""); }}>${t("GUARDAR", "SAVE")}</${Boton}></div>
            ${aj.optionFile ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-all">${aj.optionFile} <button style="background:none;border:0;color:inherit;text-decoration:underline;cursor:pointer;font:inherit" onClick=${() => poner({ optionFile: "" }, t("Vuelve a la búsqueda automática.", "Back to automatic search."))}>${t("quitar", "clear")}</button></div>` : null}
            ${a.optionFile ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-all">${t("Archivo que vigilo: ", "File I watch: ")}${a.optionFile}</div>` : null}
            ${a.ignorados && (a.ignorados.altas || a.ignorados.bajas || a.ignorados.otros) ? html`<div class="ayuda" style="font-size:12px">${t(`Cambios que no son traspasos y se ignoran: ${a.ignorados.altas} altas, ${a.ignorados.bajas} bajas, ${a.ignorados.otros} otros.`, `Changes that aren't transfers and are ignored: ${a.ignorados.altas} additions, ${a.ignorados.bajas} releases, ${a.ignorados.otros} other.`)}</div>` : null}
            <div style="margin-top:6px"><${Boton} tipo="suave mini" titulo=${t("La próxima vez que PES guarde, solo anoto cómo está todo", "The next time PES saves, I only note how everything is")} al=${() => hacer("olvidarReferencia")}>${t("EMPEZAR DE CERO EL PUNTO DE PARTIDA", "RESET THE STARTING POINT")}</${Boton}></div>
          </details>` : null}
      </${Tarjeta}>

      <${Tarjeta} interior=${tarjeta}>
        <${Titulo} texto=${t("RECIBIR FICHAJES DE UN AMIGO", "RECEIVE A FRIEND'S TRANSFERS")}/>
        ${porAprobar.length ? html`<div class="caja" style="border-color:var(--warn);display:flex;flex-direction:column;gap:8px">
          <b class="warn">${porAprobar.length === 1 ? t("1 envío esperando tu aprobación", "1 batch awaiting your approval") : t(`${porAprobar.length} envíos esperando tu aprobación`, `${porAprobar.length} batches awaiting your approval`)}</b>
          ${porAprobar.slice(0, 5).map((l) => html`<div key=${l.id} style="display:flex;flex-direction:column;gap:2px">
            <span class="mono mut" style="font-size:12px">${l.autor || "?"} · ${(l.creado || "").replace("T", " ").replace("Z", " UTC")}</span>
            ${(l.cambios || []).slice(0, 8).map((c, i) => html`<div class="mono" style="font-size:13px" key=${i}>· ${nombre(c)} → ${c.hastaNombre || c.hasta}</div>`)}
            ${(l.cambios || []).length > 8 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${l.cambios.length - 8} más`, `and ${l.cambios.length - 8} more`)}</div>` : null}</div>`)}
          <div style="display:flex;gap:8px;flex-wrap:wrap">
            <${Boton} tipo="lleno" al=${() => hacer("aceptarRecibidos")}>${t("ACEPTAR", "ACCEPT")}</${Boton}>
            <${Boton} tipo="suave mini" al=${async () => { if (await confirmar({ titulo: t("¿Rechazar estos fichajes?", "Reject these transfers?"), texto: t("No se pondrán en tu juego.", "They won't go into your game."), si: t("Rechazar", "Reject"), peligro: true })) hacer("rechazarRecibidos"); }}>${t("RECHAZAR", "REJECT")}</${Boton}>
          </div></div>` : null}
        ${esWeb && w.vinculada ? html`
          ${sig.length ? sig.map((x) => html`<div key=${x.anfitrionId} style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
            <span style="font-size:15px;flex:1 1 140px"><b>${x.anfitrionNombre || "?"}</b></span><${Chip} tipo="acc">${t("RECIBIENDO", "RECEIVING")}</${Chip}>
            <${Boton} tipo="suave mini" al=${() => hacer("dejarDeSeguir", { anfitrion_id: x.anfitrionId })}>${t("DEJAR DE RECIBIR", "STOP RECEIVING")}</${Boton}></div>`)
            : html`<div class="ayuda" style="font-size:14px">${t("Pídele su código a tu amigo y pégalo aquí.", "Ask your friend for their code and paste it here.")}</div>`}
          ${sig.length ? html`<details class="sec-detalle"><summary>${t("Recibir también de otro amigo", "Also receive from another friend")}</summary><div style="margin-top:6px">${CampoCodigo({ codigo, setCodigo, hacer, etiqueta: t("RECIBIR", "RECEIVE") })}</div></details>`
            : CampoCodigo({ codigo, setCodigo, hacer, etiqueta: t("RECIBIR", "RECEIVE"), principal: true })}` : null}
        ${amigoEstado ? html`
          <div style="display:flex;align-items:flex-start;gap:10px"><span class=${cx("punto", amigoEstado[0])} style="margin-top:6px"></span><span style="font-size:15px">${amigoEstado[1]}</span></div>
          ${amigoEstado[2] ? html`<div class="ayuda" style="font-size:14px">${amigoEstado[2]}</div>` : null}
          ${m.ultimoAplicado && !amigoEstado[2] ? html`<div class="ayuda" style="font-size:14px">${infoPes} ${t(MODO_TXT[modo]?.[0] || "", MODO_TXT[modo]?.[1] || "")}</div>` : null}` : null}
        <details class="sec-detalle"><summary>${t("Fichajes recibidos", "Received transfers")}</summary>
          ${lista.length ? lista.slice(0, 8).map((l) => {
            const [tp, es, en] = ESTADO_LOTE[l.estado] || ["", l.estado, l.estado];
            return html`<div class="caja" key=${l.id} style="display:flex;flex-direction:column;gap:4px;margin-top:6px">
              <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
                <span class="mono" style="font-size:13px">#${l.seq} · ${l.autor || "?"} · ${(l.creado || "").replace("T", " ").replace("Z", " UTC")}</span>
                <${Chip} tipo=${tp}>${t(es, en)}</${Chip}></div>
              ${l.motivo ? html`<div class="ayuda" style="font-size:12px">${l.motivo}</div>` : null}
              ${(l.cambios || []).slice(0, 6).map((c, i) => { const [ct2, ce, cen2] = ESTADO_CAMBIO[c.estado] || ["", c.estado, c.estado];
                return html`<div class="mono" style="font-size:12px" key=${i}><span class=${cx("punto", ct2)} style="display:inline-block;width:8px;height:8px;margin-right:6px"></span>${nombre(c)} → ${c.hastaNombre || c.hasta} · ${t(ce, cen2)}${c.motivo ? " — " + c.motivo : ""}</div>`; })}
              ${(l.cambios || []).length > 6 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${l.cambios.length - 6} más`, `and ${l.cambios.length - 6} more`)}</div>` : null}
            </div>`; }) : html`<div class="ayuda" style="font-size:13px">${t("Todavía no ha llegado ningún fichaje.", "No transfer has arrived yet.")}</div>`}
          <div style="margin-top:8px"><${Boton} tipo="suave mini" deshabilitado=${!m.puedeDeshacer} titulo=${t("Quita el último fichaje puesto y deja el juego como estaba antes", "Removes the last transfer and leaves the game as it was before")}
            al=${async () => { if (await confirmar({ titulo: t("¿Deshacer el último fichaje?", "Undo the last transfer?"), texto: t("Link quita ese fichaje de tu juego y avisa para que se recargue.", "Link removes that transfer from your game and notifies it to reload."), si: t("Deshacer", "Undo") })) hacer("deshacer"); }}>${t("DESHACER EL ÚLTIMO FICHAJE", "UNDO LAST TRANSFER")}</${Boton}></div>
        </details>
      </${Tarjeta}>
    </div>

    <${Tarjeta} interior="padding:16px 22px">
      <details class="sec-detalle" open=${faltan > 0}><summary style="font-weight:600">${t("¿Algo no funciona?", "Something not working?")} <${Chip} tipo=${faltan === 0 ? "acc" : "vip"}>${faltan === 0 ? t("TODO BIEN", "ALL GOOD") : t(`FALTA ${faltan}`, `${faltan} MISSING`)}</${Chip}></summary>
        <div class="ayuda" style="font-size:13px;margin:6px 0">${t("Se revisa sola cada pocos segundos. Si algo sale ✗, la frase de abajo dice qué hacer.", "It's checked by itself every few seconds. If something shows ✗, the line under it says what to do.")}</div>
        ${chequeo.map((f, i) => html`<div key=${i} style="display:flex;flex-direction:column;gap:2px;padding:6px 0;border-top:1px solid var(--line, rgba(255,255,255,.08))">
          <div style="display:flex;align-items:baseline;gap:10px;flex-wrap:wrap"><span class="mono" style=${"flex:none;width:18px;font-weight:700;color:" + (f.ok ? "var(--ok)" : "var(--bad)")}>${f.ok ? "✓" : "✗"}</span>
            ${f.grupo ? html`<span class="mono mut" style="font-size:11px;flex:none">${f.grupo}</span>` : null}<span style="font-size:14px;flex:1 1 200px">${f.titulo}</span>
            ${!f.ok && f.boton ? html`<${Boton} tipo="suave mini" al=${f.boton.al}>${f.boton.texto}</${Boton}>` : null}</div>
          ${f.ok && f.detalle ? html`<div class="ayuda mono" style="font-size:12px;margin-left:28px;word-break:break-all">${f.detalle}</div>` : null}
          ${!f.ok && f.pista ? html`<div class="ayuda" style="font-size:13px;margin-left:28px">${f.pista}</div>` : null}
        </div>`)}
      </details>
      <details class="sec-detalle" style="margin-top:8px"><summary style="font-weight:600">${t("Ajustes de traspasos", "Transfer settings")}</summary>
        <div class="ayuda" style="font-size:13px;margin:6px 0">${t("Lo normal es dejarlos como vienen. Solo cámbialos si sabes lo que buscas.", "Usually leave them as they are. Only change them if you know what you want.")}</div>
        <${AjusteSw} titulo=${t("Recibir los cambios oficiales de Phoenix", "Receive official Phoenix changes")}
          desc=${aj.oficial !== false ? t("Encendido: los fichajes que el staff hace en la web llegan solos a tu PES (no hace falta código).", "On: the transfers the staff makes on the web reach your PES by themselves (no code needed).") : t("Apagado: no recibes los cambios oficiales y se quitan de tu juego.", "Off: you don't receive official changes and they're removed from your game.")}
          valor=${aj.oficial !== false} al=${(v) => poner({ oficial: v })}/>
        ${oficiales.length ? html`<div class="col" style="gap:4px;margin:4px 0 8px">${oficiales.map((l) => html`<div style="font-size:13px"><span class=${cx("punto", l.estado === "aplicado" ? "ok" : l.estado === "saltado" || l.estado === "error" ? "bad" : "warn")}></span> ${(l.cambios || []).map((c) => `${nombre(c)} → ${c.hastaNombre || "#" + c.hasta}`).join(", ")} <span class="ayuda">· ${l.estado}${l.motivo ? " · " + l.motivo : ""}</span></div>`)}</div>` : null}
        ${!noAprobado ? html`
          <${AjusteSw} titulo=${t("Aprobar cada fichaje antes de enviarlo", "Approve each transfer before sending it")}
            desc=${aj.confirmarAntes ? t("Encendido: ves el fichaje y tú pulsas PUBLICAR.", "On: you see the transfer and press PUBLISH.") : t("Apagado: se envía solo al guardar en Editar.", "Off: it's sent by itself when you save in Edit.")}
            valor=${!!aj.confirmarAntes} al=${(v) => poner({ confirmarAntes: v })}/>
          <${AjusteSw} titulo=${t("Mantener mis fichajes en mi juego", "Keep my transfers in my game")}
            desc=${aj.aplicarPropios !== false ? t("Encendido: lo que fichas se queda en tu PES aunque lo cierres y lo abras (sin pulsar Activar).", "On: what you sign stays in your PES even after closing and reopening it (no Activate).") : t("Apagado: tus fichajes solo se envían; en tu PES no se guardan aparte.", "Off: your transfers are only sent; they aren't kept separately in your PES.")}
            valor=${aj.aplicarPropios !== false} al=${(v) => poner({ aplicarPropios: v })}/>` : null}
        <${AjusteSw} titulo=${t("Aprobar cada fichaje antes de ponerlo en mi juego", "Approve each transfer before it goes into my game")}
          desc=${aj.aprobarRecibidos ? t("Encendido: lo que llega espera a que pulses ACEPTAR o RECHAZAR.", "On: what arrives waits until you press ACCEPT or REJECT.") : t("Apagado: lo que llega se pone solo en tu juego.", "Off: what arrives goes into your game by itself.")}
          valor=${!!aj.aprobarRecibidos} al=${(v) => poner({ aprobarRecibidos: v })}/>
        <${AjusteSw} titulo=${t("Modo prueba", "Test mode")} desc=${t("Te recibes a ti mismo con tu propia cuenta. Solo para comprobar que todo funciona.", "You receive yourself with your own account. Only to check everything works.")}
          valor=${prueba} al=${(v) => poner({ prueba: v, publicar: true, recibir: true })}/>
        <div class="ayuda" style="font-size:13px;margin-top:6px">${t("Por defecto viajan por la web de Phoenix. Si lo prefieres, puedes usar una carpeta compartida (Google Drive u OneDrive) que vean los dos.", "By default they travel through the Phoenix web. If you prefer, you can use a shared folder (Google Drive or OneDrive) both of you can see.")}</div>
        ${esWeb ? null : html`<div style="margin:6px 0"><${Boton} tipo="lleno mini" al=${() => poner({ transporte: "web" }, t("Vuelves a usar la web de Phoenix.", "Back to the Phoenix web."))}>${t("VOLVER A USAR LA WEB", "BACK TO THE WEB")}</${Boton}></div>`}
        ${d.carpetaPrueba && !(aj.carpeta === d.carpetaPrueba && !esWeb) ? html`<div><${Boton} tipo="suave mini" al=${() => poner({ carpeta: d.carpetaPrueba, transporte: "carpeta" }, t("Listo: carpeta de prueba elegida.", "Done: test folder chosen."))}>${t("USAR UNA CARPETA DE PRUEBA (SOLO EN ESTA PC)", "USE A TEST FOLDER (THIS PC ONLY)")}</${Boton}></div>` : null}
        <div style="display:flex;gap:8px;align-items:center;margin-top:6px"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${carpeta} placeholder="D:\\Drive\\PhoenixTraspasos" onInput=${(e) => setCarpeta(e.currentTarget.value)}/></div>
          <${Boton} tipo="suave mini" deshabilitado=${!carpeta.trim()} al=${async () => { await poner({ carpeta: carpeta.trim(), transporte: "carpeta" }, t("Ahora usas esa carpeta en vez de la web.", "You now use that folder instead of the web.")); setCarpeta(""); }}>${t("USAR ESTA CARPETA", "USE THIS FOLDER")}</${Boton}></div>
        ${aj.carpeta ? html`<div class="ayuda mono" style="font-size:12px">${t("Carpeta guardada: ", "Saved folder: ")}${aj.carpeta}</div>` : null}
        <div style="margin-top:10px"><${Boton} tipo="suave mini" al=${apagar}>${t("APAGAR TRASPASOS", "TURN OFF TRANSFERS")}</${Boton}></div>
      </details>
    </${Tarjeta}>
  </div>`;
}

/** Cabecera de un paso: círculo con el número (✓ si está hecho), título y «AHORA» si es el que toca. */
function Cabecera({ n, titulo, hecho, activo }) {
  return html`<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
    <span class="mono" style=${"flex:none;width:30px;height:30px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:14px;font-weight:700;border:1.5px solid " + (hecho ? "var(--ok)" : activo ? "var(--acc)" : "var(--line, rgba(255,255,255,.2))") + ";color:" + (hecho ? "var(--ok)" : activo ? "var(--acc)" : "inherit")}>${hecho ? "✓" : n}</span>
    <b style="font-size:17px;flex:1 1 200px">${titulo}</b>
    ${activo ? html`<${Chip} tipo="acc">${t("AHORA", "NOW")}</${Chip}>` : null}
  </div>`;
}
/** Opción elegible del paso 1 (una tarjeta con título y frase). */
function Opcion({ sel, al, titulo, texto }) {
  return html`<button type="button" aria-pressed=${sel} onClick=${() => { if (!sel) al(); }}
    style=${"flex:1 1 230px;min-width:220px;text-align:left;cursor:pointer;font:inherit;color:inherit;border-radius:12px;padding:14px 16px;background:" + (sel ? "rgba(120,200,255,.10)" : "transparent") + ";border:1.5px solid " + (sel ? "var(--acc)" : "var(--line, rgba(255,255,255,.18))")}>
    <div style="display:flex;align-items:center;gap:8px"><span style=${"font-weight:700;color:" + (sel ? "var(--acc)" : "inherit")}>${sel ? "● " : "○ "}${titulo}</span></div>
    <div class="ayuda" style="font-size:13px;margin-top:4px">${texto}</div></button>`;
}

/** Caja para escribir el código del anfitrión + botón SEGUIR. */
function CampoCodigo({ codigo, setCodigo, hacer, etiqueta, principal = false }) {
  const seguir = async () => { if (!codigo.trim()) return; await hacer("seguirCodigo", { codigo: codigo.trim() }); setCodigo(""); };
  return html`<div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${16} value=${codigo} placeholder=${t("Código del anfitrión", "Host code")}
      style=${principal ? "font-size:20px;letter-spacing:3px;height:48px" : ""} onInput=${(e) => setCodigo(e.currentTarget.value.toUpperCase())} onKeyDown=${(e) => { if (e.key === "Enter") seguir(); }}/></div>
    <${Boton} tipo=${principal ? "lleno" : "suave mini"} deshabilitado=${!codigo.trim()} al=${seguir}>${etiqueta}</${Boton}></div>`;
}
