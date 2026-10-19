// =============================================================================
//  SYNC › MI PC («¿Está listo mi PC?»). Una sola lista de lo que hace falta para jugar con fichajes y traspasos en vivo, cada punto con su botón,
//  un mensaje arriba («Todo listo» / «Falta algo: N cosas») y un botón grande «ARREGLAR TODO» que hace en orden lo que se puede sin riesgo
//  y se detiene si necesita algo de la persona. Los traspasos (enviar y recibir, sin papel) se ven en la pestaña Traspasos: aquí solo hay un enlace.
//  Nada nuevo en el motor: usa las mismas acciones que el resto de Sync (sync.modulos, sync.modulo, sync.detectarJuego,
//  sync.modoRecarga, traspasos.estado/config/accion). Con PES abierto no se escribe nada en el juego.
// =============================================================================
import { html, useState, useEffect, useRef } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, elegirPestana, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, BotonIr, cx } from "../ui.js";
import { useSemaforo } from "./semaforo.js";

const espera = (ms) => new Promise((r) => setTimeout(r, ms));
/** Nombre corto de una ruta para la pantalla; la ruta entera queda en «detalles». */
const corto = (ruta) => { const p = String(ruta || "").replace(/[\\/]+$/, "").split(/[\\/]/); return p[p.length - 1] || ruta; };

/** Esta pantalla es el punto «Mi juego» del Semáforo Phoenix: arriba, cómo está todo lo demás y un botón al semáforo entero. */
function EnElSemaforo({ m }) {
  const { lista, resumen } = useSemaforo(m);
  const otros = lista.filter((x) => x.id !== "juego" && (x.nivel === "falta" || x.nivel === "aviso"));
  return html`<div class="caja" style="display:flex;align-items:center;gap:12px;flex-wrap:wrap">
    <span class=${cx("punto", resumen.color)} style="width:12px;height:12px;border-radius:6px"></span>
    <div style="flex:1;min-width:220px"><b>${t("Semáforo Phoenix", "Phoenix check")}: ${resumen.texto}</b>
      <div class="ayuda">${otros.length ? t(`Aparte de tu juego: ${otros.map((x) => x.titulo).join(" · ")}.`, `Besides your game: ${otros.map((x) => x.titulo).join(" · ")}.`)
        : t("Esta pantalla es la parte «Mi juego» del semáforo. Parsec, mandos y transmisión están en Sala › Conexión.", "This screen is the «My game» part of the check. Parsec, pads and stream are in Room › Connection.")}</div></div>
    <${BotonIr} ir=${["sala", "red"]}>${t("VER SEMÁFORO COMPLETO", "SEE FULL CHECK")}</${BotonIr}>
  </div>`;
}

export function Preparar({ s }) {
  const m = s.motor || {};
  const [mods, setMods] = useState(null);      // sync.modulos
  const [tr, setTr] = useState(null);          // traspasos.estado
  const [raiz, setRaiz] = useState(null);      // sync.raizPhoenixDB: modos cuyo sider.ini no lee Phoenix-DB
  const [busca, setBusca] = useState(null);    // candidatos del juego si hay varios
  const [haciendo, setHaciendo] = useState(false);
  const [codigoCta, setCodigoCta] = useState("");   // los 6 dígitos para unir la cuenta
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
  const rol = aj.prueba ? "prueba" : (aj.publicar || aj.recibir) ? "encendido" : "apagado";   // sin papel: enviar y recibir van juntos
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
  const unirCuenta = async () => {
    const c = codigoCta.replace(/\D/g, "");
    if (c.length !== 6) { avisar(t("El código tiene 6 dígitos.", "The code has 6 digits."), "warn"); return; }
    if (await accion("web.vincular", { codigo: c })) setCodigoCta("");
  };
  const soltarCuenta = async () => {
    if (await confirmar({ titulo: t("¿Separar esta PC de tu cuenta?", "Unlink this PC from your account?"), texto: t("Tu sala dejará de salir en la web hasta que la unas otra vez con un código nuevo.", "Your room won't show on the web until you link it again with a new code."), si: t("Separar", "Unlink"), peligro: true })) await accion("web.desvincular");
  };

  // ---- Las filas: ok = true | false | "espera" ----
  const filas = [];
  filas.push({
    id: "cuenta", ok: cuenta === "conectado" ? true : cuenta === "vinculando" || cuenta === "pausado" ? "espera" : false,
    titulo: t("Tu cuenta de la liga", "Your league account"),
    frase: cuenta === "conectado" ? t(`Esta PC está unida a tu cuenta${m.web?.usuario ? ": " + m.web.usuario : ""}.`, `This PC is linked to your account${m.web?.usuario ? ": " + m.web.usuario : ""}.`)
      : cuenta === "vinculando" ? t("Uniendo esta PC a tu cuenta… espera un momento.", "Linking this PC to your account… wait a moment.")
      : cuenta === "pausado" ? t("La unión está en pausa.", "The link is paused.")
      : cuenta === "sin_conexion" ? t("No hay conexión con la web ahora. Se reintenta sola.", "No connection to the web right now. It retries by itself.")
      : t("Une esta PC a tu cuenta con un código de la web. Es lo único que haces tú.", "Link this PC to your account with a code from the web. It's the only thing you do yourself."),
    extra: cuenta === "sin_vincular" ? html`<div style="display:flex;flex-direction:column;gap:6px;margin-top:6px">
      <div class="ayuda" style="font-size:13px">${t("En la web: Mi perfil › vincular PC. Copia los 6 dígitos y escríbelos aquí.", "On the web: My profile › link PC. Copy the 6 digits and type them here.")}</div>
      <div style="display:flex;gap:8px;align-items:center;max-width:360px"><input class="campo mono" inputmode="numeric" maxLength="6" placeholder="123456" value=${codigoCta} style="font-size:18px;letter-spacing:.25em;text-align:center;height:40px"
        onInput=${(e) => setCodigoCta(e.currentTarget.value.replace(/[^\d]/g, "").slice(0, 6))} onKeyDown=${(e) => e.key === "Enter" && unirCuenta()}/>
        <${Boton} tipo="lleno mini" deshabilitado=${codigoCta.length !== 6} al=${unirCuenta}>${t("UNIR", "LINK")}</${Boton}></div></div>` : null,
    boton: cuenta === "conectado" ? [t("SEPARAR", "UNLINK"), soltarCuenta] : cuenta === "sin_conexion" ? [t("REINTENTAR", "RETRY"), () => accion("web.reintentar", {}, { ok: t("Reintentando…", "Retrying…") })] : null,
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
    titulo: t("Archivos de Phoenix en el juego", "Phoenix files in the game"),
    frase: !mods ? t("Revisando…", "Checking…") : !mods.juego ? t("Primero hace falta la carpeta del juego.", "The game folder is needed first.")
      : faltan.length === 0 ? t("Todo instalado y al día.", "Everything installed and up to date.")
      : pesAbierto ? t("Faltan archivos o hay versión nueva. Cierra PES y pulsa el botón.", "Files are missing or outdated. Close PES and press the button.")
      : t("Faltan archivos o hay versión nueva. Antes se hace una copia.", "Files are missing or outdated. A backup is made first."),
    boton: !mods || !mods.juego || faltan.length === 0 ? null : [t("INSTALAR AHORA", "INSTALL NOW"), instalar, pesAbierto],
  });
  const faltaRaiz = raiz?.faltan || [];
  filas.push({
    id: "raiz", ok: !raiz ? "espera" : faltaRaiz.length === 0,
    titulo: t("Que el juego lea los traspasos", "Game reads the transfers"),
    frase: !raiz ? t("Revisando…", "Checking…") : faltaRaiz.length === 0 ? t("El juego está listo para leer los fichajes.", "The game is ready to read the transfers.")
      : raiz.pesAbierto ? t("Hay fichajes puestos pero el juego no los lee. Cierra PES y pulsa el botón.", "Transfers are in place but the game doesn't read them. Close PES and press the button.")
      : t("Hay fichajes puestos pero el juego no los lee. Se arregla con copia antes.", "Transfers are in place but the game doesn't read them. Fixed with a backup first."),
    boton: !raiz || faltaRaiz.length === 0 ? null : [t("ARREGLAR", "FIX"), arreglarRaiz, raiz.pesAbierto],
    detalle: faltaRaiz.join(", "),
  });
  filas.push({
    id: "recarga", ok: !tr ? "espera" : modo !== "ACTIVAR",
    titulo: t("Recarga automática de fichajes", "Automatic transfer reload"),
    frase: !tr ? t("Revisando…", "Checking…") : modo === "ACTIVAR" ? t("Ahora debes pulsar «Activar» en el juego cada vez. Mejor que sea solo.", "Right now you must press «Activate» in the game each time. Better automatic.")
      : t("El juego recarga solo, en un momento seguro.", "The game reloads by itself, at a safe moment."),
    boton: !tr || modo !== "ACTIVAR" ? null : [t("PONER AUTOMÁTICO", "MAKE AUTOMATIC"), ponerRecarga],
  });
  filas.push({
    id: "rol", ok: !tr ? "espera" : rol !== "apagado" ? true : "apagado", opcional: true,
    titulo: t("Traspasos en vivo", "Live transfers"),
    frase: !tr ? t("Revisando…", "Checking…") : rol === "encendido" ? t("Encendidos: envías tus fichajes con tu código y recibes los de quien te dé el suyo.", "On: you send your transfers with your code and receive those of whoever gives you theirs.")
      : rol === "prueba" ? t("Modo prueba: te mandas los fichajes a ti mismo.", "Test mode: you send transfers to yourself.")
      : t("Apagados. Actívalos solo si vas a pasar fichajes a un amigo.", "Off. Turn them on only to pass transfers to a friend."),
    boton: [t("IR A TRASPASOS →", "GO TO TRANSFERS →"), () => elegirPestana("traspasos")],
  });

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
        if (!bien) { nota(false, t("No pude instalar un archivo. Tu juego no se rompió: hay copia de lo que se tocó.", "I couldn't install a file. Your game isn't broken: what was touched is backed up.")); return; }
        nota(true, t("Instalé los archivos de Phoenix en el juego (con copia antes).", "Installed Phoenix's files in the game (backed up first)."));
        [a, b] = await leer();
      } else nota(true, t("Los archivos de Phoenix ya estaban al día.", "Phoenix's files were already up to date."));
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
      if ((s.motor?.web?.estado || "sin_vincular") !== "conectado") { nota(false, t("Falta unir tu cuenta de la liga. Escribe el código de la web en «Tu cuenta de la liga»: es lo único que haces tú.", "Your league account isn't linked. Type the web code in «Your league account»: it's the only thing you do yourself.")); return; }
      // 5. conexión de traspasos
      if (b.ajustes && (b.ajustes.publicar || b.ajustes.recibir) && (b.ajustes.transporte || "web") === "web" && b.web?.estado !== "ok") {
        await accion("traspasos.accion", { accion: "reintentar" }, { silencioso: true });
        await espera(2500);
        [a, b] = await leer();
        if (b?.web?.estado === "ok") nota(true, t("La conexión de los traspasos funciona.", "The live transfers connection works."));
        else nota(false, t("Los traspasos todavía no conectan. Se reintenta sola; mira «Traspasos» para ver el motivo.", "Live transfers aren't connected yet. It retries by itself; see «Transfers» for the reason."));
      }
      if (!(b?.ajustes && (b.ajustes.publicar || b.ajustes.recibir))) nota(true, t("Los traspasos en vivo siguen apagados. Si los quieres, enciéndelos en «Traspasos».", "Live transfers are still off. If you want them, turn them on in «Transfers»."));
    } finally {
      if (vivo.current) setHaciendo(false);
    }
  };

  const simbolo = (ok) => ok === true ? "✓" : ok === "espera" ? "…" : ok === "apagado" ? "–" : "✗";
  const todoBien = !cargando && malas.length === 0;
  const detalle = [];
  if (mods?.juego) detalle.push(t("Carpeta del juego: ", "Game folder: ") + mods.juego);
  for (const x of gest) detalle.push(`${x.archivo}${x.version ? " v" + x.version : ""}: ${x.estado}`);
  detalle.push(t("Modo de recarga: ", "Reload mode: ") + modo);
  if (tr) detalle.push(t("Conexión de traspasos: ", "Live transfers connection: ") + (w.estado || "—") + (w.texto ? " · " + w.texto : ""));
  const tec = w.tecnico;
  if (tec) detalle.push(t("Datos técnicos de la web: ", "Web technical data: ") + JSON.stringify(tec));

  return html`<div style="display:flex;flex-direction:column;gap:16px">
    <${EnElSemaforo} m=${m}/>
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
      <${Titulo} texto=${t("¿ESTÁ LISTO MI JUEGO PARA LOS FICHAJES?", "IS MY GAME READY FOR TRANSFERS?")}/>
      <div style="display:flex;align-items:center;gap:12px"><span class=${cx("punto", cargando ? "" : todoBien ? "ok" : "warn")} style="width:16px;height:16px;flex:none"></span>
        <span class="disp" style="font-size:26px;font-weight:700;color:${cargando ? "inherit" : todoBien ? "var(--ok, #4ade80)" : "var(--warn, #ffb347)"}">${cargando ? t("Revisando…", "Checking…") : todoBien ? t("Listo para los fichajes", "Ready for transfers") : t(`Para los fichajes falta: ${malas.length} ${malas.length === 1 ? "cosa" : "cosas"}`, `Missing for transfers: ${malas.length}`)}</span></div>
      <div class="ayuda" style="font-size:14px">${todoBien ? t("Tu PC está lista para jugar con fichajes.", "Your PC is ready to play with transfers.") : t("Pulsa el botón y Phoenix arregla lo que pueda. Antes cierra PES.", "Press the button and Phoenix fixes what it can. Close PES first.")}</div>
      <${Boton} tipo=${todoBien ? "suave" : "lleno"} deshabilitado=${haciendo || cargando} al=${arreglarTodo}>
        ${haciendo ? t("ARREGLANDO…", "FIXING…") : todoBien ? t("REVISAR OTRA VEZ", "CHECK AGAIN") : t("ARREGLAR TODO", "FIX EVERYTHING")}</${Boton}>
      ${informe.length ? html`<div class="caja" style="display:flex;flex-direction:column;gap:6px;font-size:14px">
        ${informe.map((i, k) => html`<div key=${k} style="display:flex;gap:8px"><span class=${cx("punto", i.ok ? "ok" : "warn")} style="flex:none;margin-top:7px"></span><span>${i.texto}</span></div>`)}
      </div>` : null}
    </${Tarjeta}>
    <${Tarjeta} interior="padding:8px 20px;display:flex;flex-direction:column">
      ${filas.map((f) => html`<div key=${f.id} style="display:flex;gap:10px 14px;align-items:flex-start;flex-wrap:wrap;padding:14px 0;border-bottom:1px solid var(--linea, rgba(255,255,255,.08))">
        <span class=${cx("disp", f.ok === true ? "acc" : "")} style=${"flex:none;width:30px;font-size:22px;font-weight:700;text-align:center;" + (f.ok === false && !f.opcional ? "color:var(--bad, #ff6b6b)" : "")}>${simbolo(f.ok)}</span>
        <div style="flex:1 1 140px;min-width:0;display:flex;flex-direction:column;gap:3px">
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
