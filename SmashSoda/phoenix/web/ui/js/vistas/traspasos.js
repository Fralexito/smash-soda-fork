// =============================================================================
//  DESPUÉS › TRASPASOS. «Pasar un fichaje a un amigo»: un asistente de 3 pasos, siempre en el mismo orden.
//    1. ¿Qué vas a hacer?  (hago los traspasos = anfitrión · los recibo = amigo; «Más opciones»: prueba, apagar, carpeta compartida)
//    2. Anfitrión: TU CÓDIGO (copiar / cambiar). Amigo: escribe el código y pulsa SEGUIR.
//    3. Estado en vivo en frases cortas.  «¿Algo no funciona?»: la lista de comprobaciones, abierta sola si hay un ✗.
//  La lógica (traspasos.estado / config / accion) no cambia: solo cómo se presenta. Guía: docs/TRASPASOS-EN-VIVO.md
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar, elegirPestana } from "../tienda.js";
import { Tarjeta, Titulo, AjusteSw, Chip, Boton, cx } from "../ui.js";

// Estado de la conexión (web o carpeta): lo mismo para el anfitrión («transporte:<clave>») y para el amigo (la clave sola)
const CONEXION = {
  ok: ["ok", "Conectado a la web.", "Connected to the web."],
  web_sin_endpoints: ["warn", "La web todavía no tiene esta función. Se vuelve a probar sola.", "The web doesn't have this feature yet. It retries by itself."],
  sin_cuenta: ["bad", "Esta PC no está unida a tu cuenta de la liga.", "This PC isn't linked to your league account."],
  sin_permiso: ["bad", "La web no te deja ser anfitrión. Pídele al staff que apruebe tu cuenta.", "The web doesn't let you host. Ask the staff to approve your account."],
  pendiente_aprobacion: ["warn", "Tu anfitrión todavía no te ha aceptado.", "Your host hasn't accepted you yet."],
  limite: ["warn", "Hay demasiadas peticiones. Espero un momento.", "Too many requests. Waiting a moment."],
  sin_conexion: ["warn", "No hay conexión con la web. Se reintenta sola.", "No connection to the web. It retries by itself."],
  respuesta_rara: ["warn", "La web contestó algo raro. Se reintenta sola.", "The web answered something odd. It retries by itself."],
  sin_anfitrion: ["warn", "Escribe el código de tu anfitrión para seguirlo.", "Type your host's code to follow them."],
  liga_pausada: ["warn", "La liga está en pausa. Espero.", "The league is paused. Waiting."],
};
const ESTADO_LOTE = { pendiente: ["warn", "PENDIENTE", "PENDING"], aplicado: ["ok", "PUESTO", "APPLIED"], parcial: ["vip", "A MEDIAS", "PARTIAL"], saltado: ["bad", "NO SE PUDO", "SKIPPED"], deshecho: ["", "DESHECHO", "UNDONE"], propio: ["", "TUYO", "YOURS"], error: ["bad", "ERROR", "ERROR"] };
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
    case "sin_option_file": return ["bad", t("No encuentro tu archivo de guardado. Indícalo en «Más opciones».", "I can't find your save file. Set it under «More options».")];
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
  if (!d) return html`<${Tarjeta} interior="padding:22px 24px"><div class="ayuda">${t("Revisando…", "Checking…")}</div></${Tarjeta}>`;

  const aj = d.ajustes || {};
  const a = d.anfitrion || {};
  const m = d.amigo || {};
  const w = d.web || {};
  const esWeb = (aj.transporte || "web") === "web";
  const pend = a.pendientes || [];
  const lista = m.historial || [];
  const nombre = (c) => c.jugadorNombre || ("#" + c.jugador);
  const modo = d.modoRecarga || "AUTO-SIEMPRE";
  const rol = aj.publicar && aj.recibir ? "prueba" : aj.publicar ? "anfitrion" : aj.recibir ? "amigo" : "apagado";
  const quiereCanal = rol === "anfitrion" || rol === "prueba";
  const quiereSeguir = rol === "amigo";
  const sig = w.siguiendo || [];
  const canal = w.canal;
  const [cp, ct, cen] = CONEXION[w.estado || "ok"] || ["", w.estado || "", w.estado || ""];
  const ponerRol = (r) => poner({ publicar: r === "anfitrion" || r === "prueba", recibir: r === "amigo" || r === "prueba" });
  const copiar = async (texto) => { try { await navigator.clipboard.writeText(texto); avisar(t("Copiado.", "Copied."), "ok", 2500); } catch { avisar(t("No pude copiar: selecciónalo y cópialo a mano.", "Couldn't copy: select it and copy by hand."), "bad", 4000); } };
  const irMiPc = () => elegirPestana("mipc");

  // ── Qué paso toca: el primero que no está hecho ──
  const hecho1 = rol !== "apagado";
  const hecho2 = hecho1 && (!esWeb ? !!aj.carpeta : (quiereCanal ? !!canal : sig.length > 0));
  const actual = !hecho1 ? 1 : !hecho2 ? 2 : 3;

  // ── «¿Algo no funciona?»: lista de comprobaciones según el papel ──
  const diag = d.diag || {};
  const sitiosSider = (diag.sitios || []).filter((x) => x.sider);
  const sinRaiz = sitiosSider.filter((x) => !x.raiz).map((x) => x.nombre);
  const arch = a.archivo || {};
  const F = (ok, titulo, detalle, pista, boton) => ({ ok, titulo, detalle, pista, boton });
  const aMiPc = { texto: t("IR A MI PC", "GO TO MY PC"), al: irMiPc };
  const filasHost = [
    F(!!w.vinculada, t("Cuenta de la liga unida", "League account linked"), "", t("Une esta PC a tu cuenta en «Mi PC».", "Link this PC to your account in «My PC»."), aMiPc),
    F(!!canal, t("Cuenta aprobada como anfitrión", "Account approved as host"), canal ? t(`Tu código: ${canal.codigo}`, `Your code: ${canal.codigo}`) : "",
      !w.vinculada ? t("Primero une la cuenta.", "Link the account first.") : w.estado === "sin_permiso" ? t("La web no aprueba esta cuenta como anfitrión: pídeselo al staff.", "The web doesn't approve this account as host: ask the staff.") : w.estado === "web_sin_endpoints" ? t("La web todavía no tiene esta función.", "The web doesn't have this feature yet.") : t("Pidiendo tu código… si tarda, pulsa «Volver a comprobar».", "Getting your code… if it takes long, press «Check again».")),
    F(!!arch.existe, t("Archivo de guardado encontrado", "Save file found"), arch.existe ? `${arch.ruta} · ${t("cambió el", "changed on")} ${arch.fecha}` : "", t("No encuentro tu archivo EDIT00000000: indícalo en «Más opciones» del paso 3.", "I can't find your EDIT00000000 file: set it under «More options» in step 3.")),
    F(!!a.hayReferencia, t("Punto de partida anotado", "Starting point noted"), a.hayReferencia ? t("Solo se enviará lo que cambies desde ahora.", "Only what you change from now on will be sent.") : "", t("Se anota sola en unos segundos. Si no, guarda una vez en PES.", "It's noted by itself in a few seconds. If not, save once in PES.")),
    F(esWeb ? (w.estado === "ok" && d.webDisponible === true) : !!aj.carpeta, esWeb ? t("Conexión con la web", "Connection to the web") : t("Carpeta compartida elegida", "Shared folder chosen"), "",
      esWeb ? (w.vinculada ? t(CONEXION[w.estado]?.[1] || "Comprobando…", CONEXION[w.estado]?.[2] || "Checking…") : t("Primero une la cuenta.", "Link the account first.")) : t("Elige la carpeta en «Más opciones» del paso 1.", "Choose the folder under «More options» in step 1.")),
  ];
  const filasAmigo = [
    F(!!w.vinculada, t("Cuenta de la liga unida", "League account linked"), "", t("Une esta PC a tu cuenta en «Mi PC».", "Link this PC to your account in «My PC»."), aMiPc),
    F(rol === "prueba" || sig.length > 0 || !esWeb, rol === "prueba" ? t("Modo prueba: te recibes a ti mismo", "Test mode: you receive yourself") : t("Sigues a un anfitrión", "You follow a host"),
      sig.length ? sig.map((x) => x.anfitrionNombre || "?").join(", ") : "", t("Escribe el código de tu anfitrión en el paso 2.", "Type your host's code in step 2.")),
    F(!!diag.ok, t("Juego encontrado", "Game found"), diag.ok ? diag.juego : "", t("Pulsa ARREGLAR TODO en «Mi PC»: busca el juego solo.", "Press FIX EVERYTHING in «My PC»: it finds the game by itself."), aMiPc),
    F(!!diag.ok && sitiosSider.length > 0 && sinRaiz.length === 0, t("El juego lee los traspasos", "The game reads the transfers"), diag.ok && sinRaiz.length === 0 && sitiosSider.length ? t(`Listo en ${sitiosSider.length} ${sitiosSider.length === 1 ? "sitio" : "sitios"}.`, `Ready in ${sitiosSider.length} ${sitiosSider.length === 1 ? "place" : "places"}.`) : "",
      !diag.ok ? t("Primero hace falta saber dónde está el juego.", "The game folder is needed first.") : !sitiosSider.length ? t("No veo la configuración de Sider en el juego.", "I can't see Sider's setup in the game.") : t(`Falta en: ${sinRaiz.join(", ")}. Con PES cerrado, pulsa ARREGLAR TODO en «Mi PC».`, `Missing in: ${sinRaiz.join(", ")}. With PES closed, press FIX EVERYTHING in «My PC».`), aMiPc),
    F(!!diag.lua && diag.lua.estado === "instalado", t("Archivo de Phoenix instalado en el juego", "Phoenix file installed in the game"), diag.lua && diag.lua.version ? `v${diag.lua.version}` : "",
      diag.lua && diag.lua.estado === "desactualizado" ? t("Hay una versión nueva. Con PES cerrado, pulsa ARREGLAR TODO en «Mi PC».", "A newer version exists. With PES closed, press FIX EVERYTHING in «My PC».") : t("Con PES cerrado, pulsa ARREGLAR TODO en «Mi PC».", "With PES closed, press FIX EVERYTHING in «My PC»."), aMiPc),
  ];
  const chequeo = rol === "apagado" ? [] : [...(quiereCanal ? filasHost.map((f) => ({ ...f, grupo: t("ANFITRIÓN", "HOST") })) : []), ...((rol === "amigo" || rol === "prueba") ? filasAmigo.map((f) => ({ ...f, grupo: t("AMIGO", "FRIEND") })) : [])];
  const faltan = chequeo.filter((f) => !f.ok).length;

  // ── Piezas de cada paso ──
  const marco = (n) => "padding:20px 22px;display:flex;flex-direction:column;gap:12px;" + (actual === n ? "outline:1.5px solid var(--acc);outline-offset:-1.5px" : "");
  const conexionMsg = () => !w.vinculada ? html`<div class="caja" style="display:flex;flex-direction:column;gap:8px">
      <div style="font-size:14px"><span class="punto bad"></span> ${t("Esta PC todavía no está unida a tu cuenta de la liga.", "This PC isn't linked to your league account yet.")}</div>
      <div><${Boton} tipo="lleno mini" al=${irMiPc}>${t("UNIR MI CUENTA", "LINK MY ACCOUNT")}</${Boton}></div></div>`
    : (w.estado !== "ok" || d.webDisponible === false) ? html`<div class="caja" style="display:flex;flex-direction:column;gap:8px">
      <div style="font-size:14px"><span class=${cx("punto", cp)}></span> ${t(ct, cen)}${w.esperaSeg > 0 && w.estado !== "ok" ? t(` Reintento en ${w.esperaSeg} s.`, ` Retry in ${w.esperaSeg} s.`) : ""}</div>
      ${w.texto ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-word">${w.texto}</div>` : null}
      <div><${Boton} tipo="suave mini" al=${() => hacer("reintentar")}>${t("VOLVER A COMPROBAR", "CHECK AGAIN")}</${Boton}></div></div>` : null;

  const [pA, fraseA] = estadoAnfitrion(a, w, pend);
  const amigoEstado = (() => {
    if (!aj.recibir) return null;
    if (m.ultimoAplicado && (m.estado === "al_dia" || m.estado === "aplicado")) return ["ok", m.ultimoAplicado.texto, m.ultimoAplicado.accion];
    if (m.estado === "al_dia" || m.estado === "aplicado") return ["ok", t("Esperando fichajes. Cuando tu anfitrión envíe uno, aparece aquí.", "Waiting for transfers. When your host sends one, it shows up here."), ""];
    if (m.estado === "esperando") return ["warn", m.mensaje || t("Hay un fichaje esperando.", "A transfer is waiting."), d.pesAbierto ? t("PES está abierto: cierra PES y se pone solo.", "PES is open: close PES and it's applied by itself.") : ""];
    if (m.estado === "sin_transporte") return ["warn", t("Primero hay que resolver la conexión (paso 2).", "The connection must be fixed first (step 2)."), ""];
    const c = CONEXION[m.estado];
    if (c) return [c[0], t(c[1], c[2]), ""];
    return ["", m.estado || "", ""];
  })();
  const infoPes = d.pesAbierto ? t("PES está abierto. Cierra y abre PES para ver el cambio, o espera la recarga automática.", "PES is open. Close and reopen PES to see the change, or wait for the automatic reload.") : t("PES está cerrado: se pone al momento y lo ves al abrir PES.", "PES is closed: it's applied right away and you'll see it when you open PES.");

  return html`<div class="col" style="gap:14px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
      <${Titulo} texto=${t("PASAR UN FICHAJE A UN AMIGO", "PASS A TRANSFER TO A FRIEND")} derecha=${html`<${Chip} tipo=${rol !== "apagado" ? "acc" : ""}>${rol !== "apagado" ? t("ENCENDIDO", "ON") : t("APAGADO", "OFF")}</${Chip}>`}/>
      <div style="font-size:15px">${t("Haces un traspaso en tu PES y aparece solo en el PES de tu amigo. Son 3 pasos, siempre en este orden.", "You make a transfer in your PES and it shows up by itself in your friend's PES. 3 steps, always in this order.")}</div>
    </${Tarjeta}>

    <${Tarjeta} interior=${marco(1)}>
      <${Cabecera} n="1" titulo=${t("¿Qué vas a hacer?", "What are you going to do?")} hecho=${hecho1} activo=${actual === 1}/>
      <div style="display:flex;gap:12px;flex-wrap:wrap">
        <${Opcion} sel=${rol === "anfitrion"} al=${() => ponerRol("anfitrion")} titulo=${t("Hago los traspasos", "I make the transfers")} texto=${t("Eres el anfitrión: tus fichajes le llegan a tus amigos.", "You're the host: your transfers reach your friends.")}/>
        <${Opcion} sel=${rol === "amigo"} al=${() => ponerRol("amigo")} titulo=${t("Los recibo", "I receive them")} texto=${t("Eres el amigo: los fichajes de tu anfitrión llegan a tu juego.", "You're the friend: your host's transfers reach your game.")}/>
      </div>
      ${rol === "prueba" ? html`<div class="caja" style="font-size:13px"><span class="punto warn"></span> ${t("Modo prueba: haces y recibes con tu propia cuenta. Sirve para ver que todo funciona.", "Test mode: you make and receive with your own account. It's to check that everything works.")}</div>` : null}
      <details class="sec-detalle"><summary>${t("Más opciones", "More options")}</summary>
        <div style="display:flex;gap:8px;flex-wrap:wrap;margin:8px 0">
          <${Boton} tipo=${rol === "prueba" ? "lleno mini" : "suave mini"} al=${() => ponerRol("prueba")}>${t("MODO PRUEBA (LOS DOS EN MI CUENTA)", "TEST MODE (BOTH ON MY ACCOUNT)")}</${Boton}>
          <${Boton} tipo=${rol === "apagado" ? "lleno mini" : "suave mini"} al=${() => ponerRol("apagado")}>${t("APAGAR TRASPASOS", "TURN OFF TRANSFERS")}</${Boton}>
        </div>
        <div class="ayuda" style="font-size:13px">${t("Por defecto viajan por la web de Phoenix. Si lo prefieres, puedes usar una carpeta compartida (Google Drive u OneDrive) que vean los dos.", "By default they travel through the Phoenix web. If you prefer, you can use a shared folder (Google Drive or OneDrive) both of you can see.")}</div>
        ${esWeb ? null : html`<div class="caja" style="margin:6px 0;display:flex;flex-direction:column;gap:6px"><span class="ayuda" style="font-size:13px">${t("Ahora usas una carpeta compartida en vez de la web.", "You're using a shared folder instead of the web.")}</span>
          <div><${Boton} tipo="lleno mini" al=${() => poner({ transporte: "web" }, t("Vuelves a usar la web de Phoenix.", "Back to the Phoenix web."))}>${t("VOLVER A USAR LA WEB", "BACK TO THE WEB")}</${Boton}></div></div>`}
        ${d.carpetaPrueba && !(aj.carpeta === d.carpetaPrueba && !esWeb) ? html`<div><${Boton} tipo="suave mini" al=${() => poner({ carpeta: d.carpetaPrueba, transporte: "carpeta" }, t("Listo: carpeta de prueba elegida.", "Done: test folder chosen."))}>${t("USAR UNA CARPETA DE PRUEBA (SOLO EN ESTA PC)", "USE A TEST FOLDER (THIS PC ONLY)")}</${Boton}></div>` : null}
        <div style="display:flex;gap:8px;align-items:center;margin-top:6px"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${carpeta} placeholder="D:\\Drive\\PhoenixTraspasos" onInput=${(e) => setCarpeta(e.currentTarget.value)}/></div>
          <${Boton} tipo="suave mini" deshabilitado=${!carpeta.trim()} al=${async () => { await poner({ carpeta: carpeta.trim(), transporte: "carpeta" }, t("Ahora usas esa carpeta en vez de la web.", "You now use that folder instead of the web.")); setCarpeta(""); }}>${t("USAR ESTA CARPETA", "USE THIS FOLDER")}</${Boton}></div>
        ${aj.carpeta ? html`<div class="ayuda mono" style="font-size:12px">${t("Carpeta guardada: ", "Saved folder: ")}${aj.carpeta}</div>` : null}
      </details>
    </${Tarjeta}>

    <${Tarjeta} interior=${marco(2)}>
      <${Cabecera} n="2" titulo=${quiereSeguir ? t("Escribe el código de tu anfitrión", "Type your host's code") : t("Tu código", "Your code")} hecho=${hecho2} activo=${actual === 2}/>
      ${!hecho1 ? html`<div class="ayuda" style="font-size:14px">${t("Primero elige qué vas a hacer (paso 1).", "First choose what you're going to do (step 1).")}</div>` : html`
        ${esWeb ? conexionMsg() : html`<div class="ayuda" style="font-size:14px">${t("Usas una carpeta compartida: no hace falta código.", "You use a shared folder: no code needed.")}</div>`}
        ${esWeb && quiereCanal && w.vinculada ? (canal ? html`
          <div class="ayuda" style="font-size:14px">${t("Pásale este código a tu amigo. Él lo escribe en su Link.", "Give this code to your friend. They type it in their Link.")}</div>
          <div class="mono acc" style="font-size:38px;letter-spacing:6px;font-weight:700;word-break:break-all">${canal.codigo}</div>
          <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
            <${Boton} tipo="lleno" al=${() => copiar(canal.codigo)}>${t("COPIAR CÓDIGO", "COPY CODE")}</${Boton}>
            <${Boton} tipo="suave mini" titulo=${t("El código anterior deja de servir para unirse. Quienes ya te siguen siguen.", "The old code stops working for joining. People already following stay.")} al=${() => hacer("rotarCodigo")}>${t("CAMBIAR CÓDIGO", "CHANGE CODE")}</${Boton}>
          </div>
          <div class="ayuda" style="font-size:13px">${(w.meSiguen || []).length ? t("Te siguen: ", "Following you: ") + w.meSiguen.join(", ") : t("Todavía nadie te sigue.", "Nobody follows you yet.")}</div>`
          : html`<div class="ayuda" style="font-size:14px">${w.estado === "sin_permiso" ? t("Tu cuenta todavía no está aprobada como anfitrión. Pídeselo al staff.", "Your account isn't approved as a host yet. Ask the staff.") : t("Pidiendo tu código a la web…", "Getting your code from the web…")}</div>`) : null}
        ${esWeb && quiereSeguir && w.vinculada ? html`
          ${sig.length ? sig.map((x) => html`<div key=${x.anfitrionId} style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
            <span style="font-size:15px;flex:1 1 140px"><b>${x.anfitrionNombre || "?"}</b></span><${Chip} tipo="acc">${t("SIGUIENDO", "FOLLOWING")}</${Chip}>
            <${Boton} tipo="suave mini" al=${() => hacer("dejarDeSeguir", { anfitrion_id: x.anfitrionId })}>${t("DEJAR DE SEGUIR", "UNFOLLOW")}</${Boton}></div>`)
            : html`<div class="ayuda" style="font-size:14px">${t("Pídele el código a tu anfitrión y escríbelo aquí.", "Ask your host for their code and type it here.")}</div>`}
          ${sig.length ? html`<details class="sec-detalle"><summary>${t("Seguir a otro anfitrión", "Follow another host")}</summary><div style="margin-top:6px">${CampoCodigo({ codigo, setCodigo, hacer, etiqueta: t("SEGUIR", "FOLLOW") })}</div></details>`
            : CampoCodigo({ codigo, setCodigo, hacer, etiqueta: t("SEGUIR", "FOLLOW"), principal: true })}` : null}
        ${d.mensaje ? html`<div class="caja" style="font-size:13px">${d.mensaje}</div>` : null}`}
    </${Tarjeta}>

    <${Tarjeta} interior=${marco(3)}>
      <${Cabecera} n="3" titulo=${t("Qué está pasando ahora", "What's happening now")} hecho=${false} activo=${actual === 3}/>
      ${actual < 3 ? html`<div class="ayuda" style="font-size:14px">${t("Aquí verás el estado en cuanto termines el paso " + actual + ".", "You'll see the status here once you finish step " + actual + ".")}</div>` : html`
        ${quiereCanal ? html`
          <div style="display:flex;align-items:flex-start;gap:10px"><span class=${cx("punto", pA)} style="margin-top:6px"></span><span style="font-size:16px">${fraseA}</span></div>
          ${pend.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px">
            ${pend.slice(0, 12).map((c) => html`<div class="mono" style="font-size:13px" key=${c.jugador_id + "-" + c.hasta_id}>· ${c.jugador || ("#" + c.jugador_id)}: ${c.desde || c.desde_id} → ${c.hasta || c.hasta_id}</div>`)}
            ${pend.length > 12 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${pend.length - 12} más`, `and ${pend.length - 12} more`)}</div>` : null}
            <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:4px">
              <${Boton} tipo="lleno" deshabilitado=${!d.transporteListo} titulo=${d.transporteListo ? "" : t("Antes resuelve la conexión (paso 2)", "Fix the connection first (step 2)")}
                al=${async () => { if (await confirmar({ titulo: t("¿Enviar estos fichajes?", "Send these transfers?"), texto: t("Se envían a quienes te siguen y su Link los pone en su juego.", "They're sent to whoever follows you and their Link puts them in their game."), si: t("Publicar", "Publish") })) hacer("publicar"); }}>${t("PUBLICAR", "PUBLISH")}</${Boton}>
              <${Boton} tipo="suave mini" al=${() => hacer("descartar")}>${t("DESCARTAR", "DISCARD")}</${Boton}>
            </div></div>` : null}
          ${a.publicados ? html`<div class="ayuda" style="font-size:13px">${t(`Enviados en esta sesión: ${a.publicados}`, `Sent this session: ${a.publicados}`)}</div>` : null}
          <div class="ayuda" style="font-size:13px">${t("Se envía cuando PES guarda: al guardar o al salir de Editar.", "It's sent when PES saves: on saving or leaving Edit.")}</div>` : null}
        ${rol === "prueba" ? html`<div style="height:1px;background:var(--line, rgba(255,255,255,.1))"></div>` : null}
        ${amigoEstado ? html`
          ${rol === "prueba" ? html`<div class="lab">${t("COMO AMIGO", "AS FRIEND")}</div>` : null}
          <div style="display:flex;align-items:flex-start;gap:10px"><span class=${cx("punto", amigoEstado[0])} style="margin-top:6px"></span><span style="font-size:16px">${amigoEstado[1]}</span></div>
          ${amigoEstado[2] ? html`<div class="ayuda" style="font-size:14px">${amigoEstado[2]}</div>` : null}
          ${m.ultimoAplicado ? html`<div class="ayuda" style="font-size:14px">${infoPes} ${t(MODO_TXT[modo]?.[0] || "", MODO_TXT[modo]?.[1] || "")}</div>` : null}` : null}
        ${quiereCanal || amigoEstado ? html`<details class="sec-detalle"><summary>${t("Más opciones", "More options")}</summary>
          ${quiereCanal ? html`
            <${AjusteSw} titulo=${t("Preguntarme antes de enviar", "Ask me before sending")} desc=${t("Verás los fichajes detectados y pulsas PUBLICAR. Recomendado al empezar.", "You'll see the detected transfers and press PUBLISH. Recommended at first.")}
              valor=${!!aj.confirmarAntes} al=${(v) => poner({ confirmarAntes: v })}/>
            <div class="ayuda" style="font-size:13px;margin-top:6px">${t("Si Link no encuentra tu archivo de guardado, pega su ruta (el archivo EDIT00000000 o su carpeta «save»):", "If Link can't find your save file, paste its path (the EDIT00000000 file or its «save» folder):")}</div>
            <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${opt} placeholder="C:\\Users\\…\\Documents\\KONAMI\\…\\save" onInput=${(e) => setOpt(e.currentTarget.value)}/></div>
              <${Boton} tipo="suave mini" deshabilitado=${!opt.trim()} al=${async () => { await poner({ optionFile: opt.trim() }, t("Archivo de guardado anotado.", "Save file noted.")); setOpt(""); }}>${t("GUARDAR", "SAVE")}</${Boton}></div>
            ${aj.optionFile ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-all">${aj.optionFile} <button style="background:none;border:0;color:inherit;text-decoration:underline;cursor:pointer;font:inherit" onClick=${() => poner({ optionFile: "" }, t("Vuelve a la búsqueda automática.", "Back to automatic search."))}>${t("quitar", "clear")}</button></div>` : null}
            ${a.optionFile ? html`<div class="ayuda mono" style="font-size:12px;word-break:break-all">${t("Archivo que vigilo: ", "File I watch: ")}${a.optionFile}</div>` : null}
            ${aj.publicar && a.ignorados && (a.ignorados.altas || a.ignorados.bajas || a.ignorados.otros) ? html`<div class="ayuda" style="font-size:12px">${t(`Cambios que no son traspasos y se ignoran: ${a.ignorados.altas} altas, ${a.ignorados.bajas} bajas, ${a.ignorados.otros} otros.`, `Changes that aren't transfers and are ignored: ${a.ignorados.altas} additions, ${a.ignorados.bajas} releases, ${a.ignorados.otros} other.`)}</div>` : null}
            <div style="margin-top:6px"><${Boton} tipo="suave mini" titulo=${t("La próxima vez que PES guarde, solo anoto cómo está todo", "The next time PES saves, I only note how everything is")} al=${() => hacer("olvidarReferencia")}>${t("EMPEZAR DE CERO EL PUNTO DE PARTIDA", "RESET THE STARTING POINT")}</${Boton}></div>` : null}
          ${amigoEstado ? html`
            <div class="lab" style="margin-top:10px">${t("FICHAJES RECIBIDOS", "RECEIVED TRANSFERS")}</div>
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
              al=${async () => { if (await confirmar({ titulo: t("¿Deshacer el último fichaje?", "Undo the last transfer?"), texto: t("Link quita ese fichaje de tu juego y avisa para que se recargue.", "Link removes that transfer from your game and notifies it to reload."), si: t("Deshacer", "Undo") })) hacer("deshacer"); }}>${t("DESHACER EL ÚLTIMO FICHAJE", "UNDO LAST TRANSFER")}</${Boton}></div>` : null}
        </details>` : null}`}
    </${Tarjeta}>

    ${rol !== "apagado" ? html`<${Tarjeta} interior="padding:16px 22px">
      <details class="sec-detalle" open=${faltan > 0}><summary style="font-weight:600">${t("¿Algo no funciona?", "Something not working?")} <${Chip} tipo=${faltan === 0 ? "acc" : "vip"}>${faltan === 0 ? t("TODO BIEN", "ALL GOOD") : t(`FALTA ${faltan}`, `${faltan} MISSING`)}</${Chip}></summary>
        <div class="ayuda" style="font-size:13px;margin:6px 0">${t("Se revisa sola cada pocos segundos. Si algo sale ✗, la frase de abajo dice qué hacer.", "It's checked by itself every few seconds. If something shows ✗, the line under it says what to do.")}</div>
        ${chequeo.map((f, i) => html`<div key=${i} style="display:flex;flex-direction:column;gap:2px;padding:6px 0;border-top:1px solid var(--line, rgba(255,255,255,.08))">
          <div style="display:flex;align-items:baseline;gap:10px;flex-wrap:wrap"><span class="mono" style=${"flex:none;width:18px;font-weight:700;color:" + (f.ok ? "var(--ok, #4ade80)" : "var(--bad, #ff6b6b)")}>${f.ok ? "✓" : "✗"}</span>
            <span class="mono mut" style="font-size:11px;flex:none">${f.grupo}</span><span style="font-size:14px;flex:1 1 200px">${f.titulo}</span>
            ${!f.ok && f.boton ? html`<${Boton} tipo="suave mini" al=${f.boton.al}>${f.boton.texto}</${Boton}>` : null}</div>
          ${f.ok && f.detalle ? html`<div class="ayuda mono" style="font-size:12px;margin-left:28px;word-break:break-all">${f.detalle}</div>` : null}
          ${!f.ok && f.pista ? html`<div class="ayuda" style="font-size:13px;margin-left:28px">${f.pista}</div>` : null}
        </div>`)}
      </details>
    </${Tarjeta}>` : null}
  </div>`;
}

/** Cabecera de un paso: círculo con el número (✓ si está hecho), título y «AHORA» si es el que toca. */
function Cabecera({ n, titulo, hecho, activo }) {
  return html`<div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
    <span class="mono" style=${"flex:none;width:30px;height:30px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:14px;font-weight:700;border:1.5px solid " + (hecho ? "var(--ok, #4ade80)" : activo ? "var(--acc)" : "var(--line, rgba(255,255,255,.2))") + ";color:" + (hecho ? "var(--ok, #4ade80)" : activo ? "var(--acc)" : "inherit")}>${hecho ? "✓" : n}</span>
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
