// =============================================================================
//  SYNC › PREPARAR TODO. Una sola lista de lo que hace falta para jugar con fichajes y traspasos en vivo, cada punto con su botón,
//  y un botón grande «ARREGLAR TODO» que hace en orden lo que se puede sin riesgo y se detiene si necesita algo de la persona.
//  Nada nuevo en el motor: usa las mismas acciones que el resto de Sync (sync.modulos, sync.modulo, sync.detectarJuego,
//  sync.modoRecarga, traspasos.estado/config/accion). Con PES abierto no se escribe nada en el juego.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, elegirPestana } from "../tienda.js";
import { Tarjeta, Titulo, Boton, cx } from "../ui.js";

const espera = (ms) => new Promise((r) => setTimeout(r, ms));
/** Nombre corto de una ruta para la pantalla; la ruta entera queda en «detalles». */
const corto = (ruta) => { const p = String(ruta || "").replace(/[\\/]+$/, "").split(/[\\/]/); return p[p.length - 1] || ruta; };

export function Preparar({ s }) {
  const m = s.motor || {};
  const [mods, setMods] = useState(null);      // sync.modulos
  const [tr, setTr] = useState(null);          // traspasos.estado
  const [raiz, setRaiz] = useState(null);      // sync.raizPhoenixDB: modos cuyo sider.ini no lee Phoenix-DB
  const [busca, setBusca] = useState(null);    // candidatos del juego si hay varios
  const [haciendo, setHaciendo] = useState(false);
  const [informe, setInforme] = useState([]);  // lo que hizo «ARREGLAR TODO»: [{ok, texto}]
  const vivo = useRef(true);
  useEffect(() => () => { vivo.current = false; }, []);

  const leer = async () => {
    const a = await accion("sync.modulos", {}, { silencioso: true });
    const b = await accion("traspasos.estado", {}, { silencioso: true });
    if (!vivo.current) return [a, b];
    const c = await accion("sync.raizPhoenixDB", {}, { silencioso: true });
    if (a) setMods(a);
    if (b) setTr(b);
    if (c) setRaiz(c);
    return [a, b, c];
  };
  useEffect(() => { leer(); const id = setInterval(leer, 5000); return () => clearInterval(id); }, []);

  const aj = tr?.ajustes || {};
  const w = tr?.web || {};
  const modo = tr?.modoRecarga || "AUTO-SIEMPRE";
  const gest = (mods?.modulos || []).filter((x) => x.gestionable);
  const faltan = gest.filter((x) => x.estado !== "instalado");
  const rol = aj.publicar && aj.recibir ? "prueba" : aj.publicar ? "anfitrion" : aj.recibir ? "amigo" : "apagado";
  const esWeb = (aj.transporte || "web") === "web";
  const cuenta = m.web?.estado || "sin_vincular";
  const pesAbierto = !!mods?.pesAbierto;

  // ---- Acciones sueltas (las usan los botones de cada fila y «ARREGLAR TODO») ----
  const buscarJuego = async () => {
    await accion("sync.detectarJuego", { accion: "iniciar" }, { silencioso: true });
    for (let i = 0; i < 60 && vivo.current; i++) {
      const r = await accion("sync.detectarJuego", { accion: "estado" }, { silencioso: true });
      if (!r) return { error: true };
      if (r.conocido) { await leer(); return { ok: true, ruta: r.ruta }; }
      if (r.terminada) { setBusca(r.candidatos || []); return { candidatos: r.candidatos || [] }; }
      await espera(500);
    }
    return { error: true };
  };
  const elegirCarpeta = async (ruta) => {
    const r = await accion("sync.carpetaJuego", { ruta });
    if (r) { setBusca(null); await leer(); avisar(t("Listo: ya sé dónde está tu juego.", "Done: I know where your game is."), "ok", 4000); }
  };
  const instalar = async () => {
    for (const x of faltan) {
      const r = await accion("sync.modulo", { archivo: x.archivo, accion: "instalar" });
      if (!r) return false;
    }
    await leer();
    return true;
  };
  const arreglarRaiz = async () => {
    const r = await accion("sync.raizPhoenixDB", { accion: "arreglar" });
    await leer();
    return !!r;
  };
  const ponerRecarga = async () => { const r = await accion("sync.modoRecarga", { modo: "AUTO-SIEMPRE" }); await leer(); return !!r; };
  const ponerRol = async (r) => { await accion("traspasos.config", { publicar: r === "anfitrion", recibir: r === "amigo" }); await leer(); };
  const reintentarWeb = async () => { await accion("traspasos.accion", { accion: "reintentar" }); await leer(); };

  // ---- Las filas: ok = true | false | "espera" ----
  const filas = [];
  filas.push({
    id: "cuenta", ok: cuenta === "conectado" ? true : cuenta === "vinculando" || cuenta === "pausado" ? "espera" : false,
    titulo: t("Tu cuenta de la liga", "Your league account"),
    frase: cuenta === "conectado" ? t("Esta PC ya está unida a tu cuenta.", "This PC is already linked to your account.")
      : cuenta === "vinculando" ? t("Uniendo esta PC a tu cuenta… espera un momento.", "Linking this PC to your account… wait a moment.")
      : cuenta === "pausado" ? t("La unión está en pausa.", "The link is paused.")
      : cuenta === "sin_conexion" ? t("No hay conexión con la web ahora mismo. Se reintenta sola.", "No connection to the web right now. It retries by itself.")
      : t("Esta PC todavía no está unida a tu cuenta. Es lo único que debes hacer tú.", "This PC isn't linked to your account yet. It's the only thing you must do yourself."),
    boton: cuenta === "conectado" ? null : [t("UNIR MI CUENTA", "LINK MY ACCOUNT"), () => elegirPestana("web")],
  });
  filas.push({
    id: "juego", ok: !mods ? "espera" : !!mods.juego,
    titulo: t("Carpeta del juego", "Game folder"),
    frase: !mods ? t("Revisando…", "Checking…") : mods.juego ? t(`Encontrada: ${corto(mods.juego)}.`, `Found: ${corto(mods.juego)}.`)
      : t("Todavía no sé dónde tienes PES. Lo busco yo, no tienes que escribir nada.", "I don't know where your PES is yet. I'll look for it, you don't type anything."),
    boton: !mods || mods.juego ? null : [t("BUSCAR MI JUEGO", "FIND MY GAME"), buscarJuego],
  });
  filas.push({
    id: "modulos", ok: !mods ? "espera" : !mods.juego ? false : faltan.length === 0,
    titulo: t("Piezas de Phoenix en el juego", "Phoenix pieces in the game"),
    frase: !mods ? t("Revisando…", "Checking…") : !mods.juego ? t("Primero hace falta la carpeta del juego.", "The game folder is needed first.")
      : faltan.length === 0 ? t("Todo instalado y al día.", "Everything installed and up to date.")
      : pesAbierto ? t("Hay piezas por instalar o actualizar. Cierra PES y pulsa el botón.", "Some pieces need installing or updating. Close PES and press the button.")
      : t("Hay piezas por instalar o actualizar. Se hace una copia antes de tocar nada.", "Some pieces need installing or updating. A backup is made before touching anything."),
    boton: !mods || !mods.juego || faltan.length === 0 ? null : [t("INSTALAR AHORA", "INSTALL NOW"), instalar, pesAbierto],
  });
  const faltaRaiz = raiz?.faltan || [];
  filas.push({
    id: "raiz", ok: !raiz ? "espera" : faltaRaiz.length === 0,
    titulo: t("Que el juego lea los traspasos", "Game reads the transfers"),
    frase: !raiz ? t("Revisando…", "Checking…") : faltaRaiz.length === 0 ? t("El juego está listo para leer los fichajes de Phoenix.", "The game is ready to read Phoenix transfers.")
      : raiz.pesAbierto ? t("Hay fichajes puestos, pero el juego aún no los lee. Cierra PES y pulsa el botón.", "Transfers are in place but the game doesn't read them yet. Close PES and press the button.")
      : t("Hay fichajes puestos, pero el juego aún no los lee: falta una línea de configuración. Se arregla sola, con copia antes.", "Transfers are in place but the game doesn't read them yet: a setting line is missing. It fixes itself, with a backup first."),
    boton: !raiz || faltaRaiz.length === 0 ? null : [t("ARREGLAR", "FIX"), arreglarRaiz, raiz.pesAbierto],
    detalle: faltaRaiz.join(", "),
  });
  filas.push({
    id: "recarga", ok: !tr ? "espera" : modo !== "ACTIVAR",
    titulo: t("Que los fichajes se pongan solos", "Transfers apply by themselves"),
    frase: !tr ? t("Revisando…", "Checking…") : modo === "ACTIVAR" ? t("Ahora tienes que pulsar «Activar» en el juego cada vez. Lo mejor es que se haga solo.", "Right now you must press «Activate» in the game each time. It's best if it happens by itself.")
      : t("El juego recarga solo, en un momento seguro.", "The game reloads by itself, at a safe moment."),
    boton: !tr || modo !== "ACTIVAR" ? null : [t("PONER AUTOMÁTICO", "MAKE AUTOMATIC"), ponerRecarga],
  });
  filas.push({
    id: "rol", ok: !tr ? "espera" : rol !== "apagado",
    titulo: t("Traspasos en vivo: tu papel", "Live transfers: your role"),
    frase: !tr ? t("Revisando…", "Checking…") : rol === "anfitrion" ? t("Eres el anfitrión: tus traspasos le llegan a tus amigos.", "You're the host: your transfers reach your friends.")
      : rol === "amigo" ? t("Eres el amigo: recibes los traspasos del anfitrión.", "You're the friend: you receive the host's transfers.")
      : rol === "prueba" ? t("Modo de prueba: te mandas los traspasos a ti mismo.", "Test mode: you send transfers to yourself.")
      : t("Elige si haces los traspasos o los recibes. Si solo quieres fichajes normales, puedes dejarlo así.", "Choose whether you make the transfers or receive them. If you only want normal transfers, you can leave this."),
    extra: tr && rol === "apagado" ? html`<div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:6px">
      <${Boton} tipo="suave mini" al=${() => ponerRol("anfitrion")}>${t("SOY EL ANFITRIÓN", "I'M THE HOST")}</${Boton}>
      <${Boton} tipo="suave mini" al=${() => ponerRol("amigo")}>${t("SOY EL AMIGO", "I'M THE FRIEND")}</${Boton}></div>` : null,
    opcional: true,
  });
  if (tr && rol !== "apagado") {
    const lista = esWeb && cuenta === "conectado";
    filas.push({
      id: "web", ok: !esWeb ? true : !lista ? false : w.estado === "ok" && tr.webDisponible !== false ? true : w.estado === "ok" ? "espera" : false,
      titulo: t("Conexión de los traspasos", "Live transfers connection"),
      frase: !esWeb ? t("Usas la carpeta compartida.", "You use the shared folder.")
        : !lista ? t("Necesita tu cuenta unida (el primer punto).", "It needs your linked account (the first point).")
        : w.estado === "ok" ? t("Conectado: los traspasos viajan por la web.", "Connected: transfers travel through the web.")
        : w.estado === "sin_conexion" ? t("No consigo hablar con la web. Se reintenta sola; también puedes reintentar ahora.", "I can't reach the web. It retries by itself; you can also retry now.")
        : w.estado === "sin_anfitrion" ? t("Falta el código de tu anfitrión (se escribe en «Traspasos en vivo»).", "Your host's code is missing (type it in «Live transfers»).")
        : w.estado === "pendiente_aprobacion" ? t("Tu anfitrión todavía tiene que aceptarte.", "Your host still has to accept you.")
        : t("La web no está lista todavía. Se reintenta sola.", "The web isn't ready yet. It retries by itself."),
      boton: !esWeb || !lista || w.estado === "ok" ? null : w.estado === "sin_anfitrion" ? [t("IR A ESCRIBIRLO", "GO TYPE IT"), () => elegirPestana("traspasos")] : [t("REINTENTAR AHORA", "RETRY NOW"), reintentarWeb],
    });
  }

  const malas = filas.filter((f) => f.ok === false && !f.opcional);
  const cargando = !mods || !tr;

  // ---- ARREGLAR TODO: en orden, parando cuando haga falta la persona ----
  const arreglarTodo = async () => {
    if (haciendo) return;
    setHaciendo(true);
    const log = [];
    const nota = (ok, texto) => { log.push({ ok, texto }); if (vivo.current) setInforme([...log]); };
    try {
      let [a, b] = await leer();
      if (!a || !b) { nota(false, t("No pude leer el estado de Link. Inténtalo otra vez.", "I couldn't read Link's state. Try again.")); return; }
      // 1. carpeta del juego
      if (!a.juego) {
        const r = await buscarJuego();
        if (r.ok) nota(true, t(`Encontré tu juego: ${corto(r.ruta)}.`, `Found your game: ${corto(r.ruta)}.`));
        else if (r.candidatos && r.candidatos.length > 1) { nota(false, t("Encontré varios juegos: elige el tuyo en la lista de abajo y vuelve a pulsar ARREGLAR TODO.", "I found several games: pick yours in the list below and press FIX EVERYTHING again.")); return; }
        else if (r.candidatos && r.candidatos.length === 1) { nota(false, t("Encontré un juego pero todavía no tiene Sider. Ábrelo una vez con su lanzador y vuelve a pulsar.", "I found a game but it has no Sider yet. Open it once with its launcher and press again.")); return; }
        else { nota(false, t("No encontré PES 2021 en este PC. Si está en una carpeta rara, ábrelo una vez y vuelve a pulsar.", "I didn't find PES 2021 on this PC. If it's in an odd folder, open it once and press again.")); return; }
        [a, b] = await leer();
      } else nota(true, t("La carpeta del juego ya estaba bien.", "The game folder was already fine."));
      // 2. piezas de Phoenix (solo con PES cerrado)
      const pend = (a.modulos || []).filter((x) => x.gestionable && x.estado !== "instalado");
      if (pend.length) {
        if (a.pesAbierto) { nota(false, t("Cierra PES 2021 (y Sider) y vuelve a pulsar ARREGLAR TODO: con el juego abierto no toco nada.", "Close PES 2021 (and Sider) and press FIX EVERYTHING again: I touch nothing while the game is open.")); return; }
        let bien = true;
        for (const x of pend) {
          const r = await accion("sync.modulo", { archivo: x.archivo, accion: "instalar" });
          if (!r) { bien = false; break; }
        }
        if (!bien) { nota(false, t("No pude instalar una pieza. Tu juego no se ha roto: hay copia de lo que se tocó.", "I couldn't install a piece. Your game isn't broken: what was touched is backed up.")); return; }
        nota(true, t("Instalé las piezas de Phoenix en el juego (con copia antes).", "Installed Phoenix's pieces in the game (backed up first)."));
        [a, b] = await leer();
      } else nota(true, t("Las piezas de Phoenix ya estaban al día.", "Phoenix's pieces were already up to date."));
      // 2b. que el juego lea los fichajes ya puestos (línea de Phoenix-DB en sider.ini)
      const cr = await accion("sync.raizPhoenixDB", {}, { silencioso: true });
      if (cr && cr.faltan && cr.faltan.length) {
        if (cr.pesAbierto) { nota(false, t("Cierra PES 2021 y vuelve a pulsar: falta que el juego lea los fichajes y con el juego abierto no toco nada.", "Close PES 2021 and press again: the game doesn't read the transfers yet and I touch nothing while it's open.")); return; }
        if (await arreglarRaiz()) nota(true, t(`Dejé al juego leyendo los fichajes (${cr.faltan.join(", ")}).`, `The game now reads the transfers (${cr.faltan.join(", ")}).`));
        else { nota(false, t("No pude dejar al juego leyendo los fichajes. No se rompió nada.", "I couldn't make the game read the transfers. Nothing was broken.")); return; }
      }
      // 3. recarga automática
      if ((b.modoRecarga || "AUTO-SIEMPRE") === "ACTIVAR") {
        if (await ponerRecarga()) nota(true, t("Dejé la recarga automática.", "Set the automatic reload."));
        else nota(false, t("No pude cambiar la recarga automática.", "I couldn't change the automatic reload."));
      } else nota(true, t("La recarga ya era automática.", "The reload was already automatic."));
      // 4. cuenta: solo la persona puede
      if ((s.motor?.web?.estado || "sin_vincular") !== "conectado") { nota(false, t("Falta unir tu cuenta de la liga. Pulsa «UNIR MI CUENTA» en la lista: es lo único que debes hacer tú.", "Your league account isn't linked. Press «LINK MY ACCOUNT» in the list: it's the only thing you must do yourself.")); return; }
      // 5. conexión de traspasos
      if (b.ajustes && (b.ajustes.publicar || b.ajustes.recibir) && (b.ajustes.transporte || "web") === "web" && b.web?.estado !== "ok") {
        await accion("traspasos.accion", { accion: "reintentar" }, { silencioso: true });
        await espera(2500);
        [a, b] = await leer();
        if (b?.web?.estado === "ok") nota(true, t("La conexión de los traspasos funciona.", "The live transfers connection works."));
        else nota(false, t("Los traspasos todavía no conectan; se reintenta sola. Mira el punto «Conexión de los traspasos».", "Live transfers aren't connected yet; it retries by itself. See the «Live transfers connection» point."));
      }
      if (!(b?.ajustes && (b.ajustes.publicar || b.ajustes.recibir))) nota(true, t("Los traspasos en vivo siguen apagados. Si los quieres, elige tu papel en la lista.", "Live transfers are still off. If you want them, pick your role in the list."));
    } finally {
      if (vivo.current) setHaciendo(false);
    }
  };

  const simbolo = (ok) => ok === true ? "✓" : ok === "espera" ? "…" : "✗";
  const todoBien = !cargando && malas.length === 0;
  const detalle = [];
  if (mods?.juego) detalle.push(t("Carpeta del juego: ", "Game folder: ") + mods.juego);
  for (const x of gest) detalle.push(`${x.archivo}${x.version ? " v" + x.version : ""}: ${x.estado}`);
  detalle.push(t("Modo de recarga: ", "Reload mode: ") + modo);
  if (tr) detalle.push(t("Conexión de traspasos: ", "Live transfers connection: ") + (w.estado || "—"));
  const tec = w.tecnico;
  if (tec) detalle.push(t("Datos técnicos de la web: ", "Web technical data: ") + JSON.stringify(tec));

  return html`<div style="display:flex;flex-direction:column;gap:16px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:16px">
      <${Titulo} texto=${t("PREPARAR TODO", "GET EVERYTHING READY")}/>
      <div class="ayuda" style="font-size:14px">${t("Aquí ves, de un vistazo, si todo está listo para jugar con fichajes. Pulsa el botón grande y Phoenix hace lo que pueda por ti.", "See at a glance whether everything is ready to play with transfers. Press the big button and Phoenix does what it can for you.")}</div>
      <${Boton} tipo="lleno" deshabilitado=${haciendo || cargando} al=${arreglarTodo}>
        ${haciendo ? t("ARREGLANDO…", "FIXING…") : todoBien ? t("TODO LISTO · REVISAR OTRA VEZ", "ALL SET · CHECK AGAIN") : t("ARREGLAR TODO", "FIX EVERYTHING")}</${Boton}>
      ${informe.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px;font-size:14px">
        ${informe.map((i, k) => html`<div key=${k} style="display:flex;gap:8px"><span class=${cx("punto", i.ok ? "ok" : "warn")} style="flex:none;margin-top:7px"></span><span>${i.texto}</span></div>`)}
      </div>` : null}
    </${Tarjeta}>
    <${Tarjeta} interior="padding:8px 20px;display:flex;flex-direction:column">
      ${filas.map((f) => html`<div key=${f.id} style="display:flex;gap:14px;align-items:flex-start;padding:14px 0;border-bottom:1px solid var(--linea, rgba(255,255,255,.08))">
        <span class=${cx("disp", f.ok === true ? "acc" : "")} style=${"flex:none;width:30px;font-size:22px;font-weight:700;text-align:center;" + (f.ok === false && !f.opcional ? "color:var(--bad, #ff6b6b)" : "")}>${simbolo(f.ok)}</span>
        <div style="flex:1;min-width:0;display:flex;flex-direction:column;gap:3px">
          <b style="font-size:15px">${f.titulo}</b>
          <span class="ayuda" style="font-size:14px">${f.frase}</span>
          ${f.detalle ? html`<span class="mut mono" style="font-size:12px">${f.detalle}</span>` : null}
          ${f.extra || null}
        </div>
        ${f.boton ? html`<${Boton} tipo=${f.ok === false ? "lleno mini" : "suave mini"} deshabilitado=${haciendo || f.boton[2]} titulo=${f.boton[2] ? t("Cierra PES 2021 primero", "Close PES 2021 first") : ""} al=${f.boton[1]}>${f.boton[0]}</${Boton}>` : null}
      </div>`)}
    </${Tarjeta}>
    ${busca && busca.length ? html`<${Tarjeta} interior="padding:18px 22px;display:flex;flex-direction:column;gap:10px">
      <${Titulo} texto=${t("¿CUÁL ES TU JUEGO?", "WHICH IS YOUR GAME?")}/>
      <div class="ayuda" style="font-size:14px">${t("Encontré estos. Pulsa el tuyo (el que tiene Sider es el que usa Phoenix).", "I found these. Press yours (the one with Sider is what Phoenix uses).")}</div>
      ${busca.map((c) => html`<div key=${c.ruta} class="caja" style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
        <span style="min-width:0;word-break:break-all"><b>${corto(c.ruta)}</b> <span class="mut mono" style="font-size:12px">${c.ruta}</span> ${c.sider ? "" : html`<span class="mut">· ${t("sin Sider", "no Sider")}</span>`}</span>
        <${Boton} tipo="suave mini" deshabilitado=${!c.sider} al=${() => elegirCarpeta(c.ruta)}>${t("ES ESTE", "THIS ONE")}</${Boton}></div>`)}
    </${Tarjeta}>` : null}
    <details class="caja" style="font-size:13px">
      <summary style="cursor:pointer" class="ayuda">${t("Detalles técnicos", "Technical details")}</summary>
      <div class="mono mut" style="display:flex;flex-direction:column;gap:4px;margin-top:8px;word-break:break-all">${detalle.map((d, k) => html`<span key=${k}>${d}</span>`)}</div>
    </details>
  </div>`;
}
