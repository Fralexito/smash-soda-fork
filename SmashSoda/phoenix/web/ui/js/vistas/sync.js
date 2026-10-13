// =============================================================================
//  SYNC (3 pestañas, siempre en este orden):
//    MI PC       «¿Está listo mi PC?»: la lista ✓/✗ y ARREGLAR TODO (vistas/preparar.js).
//    TRASPASOS   «Pasar un fichaje a un amigo»: asistente de 3 pasos (vistas/traspasos.js).
//    AVANZADO    todo lo técnico, agrupado, cada bloque con su para qué sirve (este archivo).
//  Un solo sitio para cada cosa: lo que sale en MI PC o en TRASPASOS no se repite aquí.
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, elegirPestana, confirmar } from "../tienda.js";
import { CuentaWeb } from "./ajustes.js";
import { Traspasos } from "./traspasos.js";
import { Preparar } from "./preparar.js";
import { Tarjeta, Titulo, AjusteSw, Segmentos, Vacio, Chip, Icono, Boton, cx } from "../ui.js";

/** Sección plegable: una línea con el título y un resumen; el detalle solo se ve al abrirla (se recuerda por sección en este PC). */
function Seccion({ id, titulo, resumen = null, punto = null, acciones = null, abierta = false, children }) {
  const clave = "phoenix.sync." + id;
  const [abierto, setAbierto] = useState(() => { try { const v = localStorage.getItem(clave); return v === null ? abierta : v === "1"; } catch { return abierta; } });
  const poner = (v) => { setAbierto(v); try { localStorage.setItem(clave, v ? "1" : "0"); } catch { /* sin almacenamiento */ } };
  return html`<${Tarjeta} interior="padding:0">
    <div class="sec-cab" role="button" tabindex="0" aria-expanded=${abierto} onClick=${() => poner(!abierto)}
      onKeyDown=${(e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); poner(!abierto); } }}>
      <span class="sec-flecha">${abierto ? "▾" : "▸"}</span>
      <span class="lab acc sec-titulo">${titulo}</span>
      ${punto != null ? html`<span class=${cx("punto", punto)} style="flex:none"></span>` : null}
      <span class="sec-resumen ayuda">${resumen}</span>
      ${acciones ? html`<span class="sec-acciones" onClick=${(e) => e.stopPropagation()}>${acciones}</span>` : null}
    </div>
    ${abierto ? html`<div class="sec-cuerpo">${children}</div>` : null}
  </${Tarjeta}>`;
}

export const PESTANAS_SYNC = [
  { id: "mipc", es: "MI PC", en: "MY PC", d: ["¿Está listo mi PC? Cuenta, juego y archivos de Phoenix en una lista, con un botón que lo arregla.", "Is my PC ready? Account, game and Phoenix files in one list, with a button that fixes it."] },
  { id: "traspasos", es: "TRASPASOS", en: "TRANSFERS", d: ["Pasar un fichaje a un amigo: lo que haces en Editar aparece en el PES de tu amigo.", "Pass a transfer to a friend: what you do in Edit shows up in your friend's PES."] },
  { id: "avanzado", es: "AVANZADO", en: "ADVANCED", d: ["Opciones avanzadas: solo para cuando algo no va bien o quieres ajustar algo.", "Advanced options: only for when something goes wrong or you want to tweak something."] },
];

/** Una sola pastilla: «todo bien» o «atención». Lleva a MI PC. */
export function metaSync(s) {
  const m = s.motor || {};
  const b = m.buzon;
  const cuenta = m.web?.estado;
  const mal = cuenta === "sin_conexion" || b?.estado === "sinconexion";
  const ok = cuenta === "conectado" && !mal && b?.estado !== "noinstalado";
  const punto = ok ? "ok" : mal ? "bad" : "warn";
  return html`<div class="grupo-estado">
    <button class="pill" title=${ok ? t("Tu cuenta está unida y el juego recibe los avisos.", "Your account is linked and the game gets the notices.") : t("Falta algo o hay un aviso. Mira «Mi PC».", "Something is missing or there is a notice. See «My PC».")} onClick=${() => elegirPestana("mipc")}>
      <span class=${cx("punto", punto, ok && "vivo")}></span>${ok ? t("TODO BIEN", "ALL GOOD") : t("ATENCIÓN", "ATTENTION")}</button>
  </div>`;
}

function ESTADOS(b) {
  const e = b?.estado || "cerrado";
  const tabla = {
    apagado: [t("Apagado", "Off"), "", t("Apagaste los avisos de la web dentro del juego.", "You turned off web notices inside the game.")],
    cerrado: [t("Juego cerrado", "Game closed"), "", t("Abre PES 2021 y Phoenix Link lo detecta solo.", "Open PES 2021 and Phoenix Link finds it by itself.")],
    noinstalado: [t("Falta el archivo de Phoenix en el juego", "Phoenix file missing in the game"), "warn", t("El juego está abierto, pero no tiene el archivo de Phoenix. No se escribe nada. Míralo en «Mi PC».", "The game is open but has no Phoenix file. Nothing is written. See «My PC».")],
    conectado: [b?.ultimo ? t(`Conectado · último aviso ${b.ultimo}`, `Connected · last notice ${b.ultimo}`) : t("Conectado", "Connected"), "ok vivo", t("Los avisos de la web llegan al juego en pocos segundos.", "Web notices reach the game within seconds.")],
    sinconexion: [t("Sin conexión", "No connection"), "bad", t("No se pudo hablar con la web. Se reintenta solo, con calma.", "Couldn't reach the web. Retrying by itself.")],
  };
  return tabla[e] || tabla.cerrado;
}

// Prueba «Solo PES 2021»: la sala solo se publica (y en estricto, solo se abre) con PES 2021 abierto.
function SoloPes({ m }) {
  const p = m.pes || { modo: "off", abierto: false };
  const oculta = p.modo !== "off" && !p.abierto;
  return html`<${Seccion} id="solopes" titulo=${t("SOLO PES 2021 (PRUEBA)", "PES 2021 ONLY (TEST)")}
    resumen=${p.modo === "off" ? t("Apagado: Link funciona con cualquier juego", "Off: Link works with any game") : p.modo === "publicar" ? t("Solo publicar con PES abierto", "Publish only with PES open") : t("Estricto", "Strict")}>
    <${Para}>${t("Para qué sirve: que tu sala solo salga en la web mientras PES 2021 está abierto.", "What it's for: your room only shows on the web while PES 2021 is open.")}</${Para}>
    <${Segmentos} valor=${p.modo} al=${(v) => accion("ajustes.modoPes", { valor: v })}
      opciones=${[{ valor: "off", texto: t("APAGADO", "OFF") }, { valor: "publicar", texto: t("SOLO PUBLICAR", "PUBLISH ONLY") }, { valor: "estricto", texto: t("ESTRICTO", "STRICT") }]}/>
    <div class="ayuda" style="font-size:14px">${p.modo === "off"
      ? t("Apagado: Link funciona con cualquier juego, como siempre.", "Off: Link works with any game, as always.")
      : p.modo === "publicar"
        ? t("Tu sala solo aparece en la web mientras PES 2021 está abierto.", "Your room only shows on the web while PES 2021 is open.")
        : t("Además, Link no te deja abrir la sala si PES 2021 está cerrado.", "Also, Link won't let you open the room while PES 2021 is closed.")}</div>
    ${oculta ? html`<div class="ayuda" style="font-size:14px;color:var(--warn, #ffb347)">${t("PES 2021 está cerrado: tu sala no se publica.", "PES 2021 is closed: your room is not published.")}</div>` : null}
  </${Seccion}>`;
}

// Modo de recarga de phoenix.lua v0.18b (Link escribe content\phoenix\modo.txt). Textos del pedido de Sync.
// «Automático: siempre» va primero y por defecto (PROMPT-LINK-modo-por-defecto.md de Sync).
const MODOS_RECARGA = {
  "ACTIVAR": ["Los fichajes se cargan solo cuando pulsas Partido → Datos Actual. en vivo → Activar.", "Transfers load only when you press Match → Live update → Activate."],
  "AUTO-FICHAJES": ["Activar sigue funcionando. Además, cuando haya fichajes nuevos, el juego los carga solo al volver al menú principal y entrar a un modo.", "Activate still works. Also, when there are new transfers, the game loads them by itself when you go back to the main menu and enter a mode."],
  "AUTO-SIEMPRE": ["Los fichajes entran solos cada vez que vuelves al menú principal y entras a un modo. No hay que pulsar nada.", "Transfers come in by themselves every time you go back to the main menu and enter a mode. Nothing to press."],
};
function ModoRecarga({ m, mods }) {
  const v = m.arbitro?.modoRecarga || "AUTO-SIEMPRE";
  return html`<${Seccion} id="recarga" titulo=${t("MODO DE RECARGA DE FICHAJES", "TRANSFER RELOAD MODE")}
    resumen=${v === "AUTO-SIEMPRE" ? t("Automático: siempre ⭐", "Auto: always ⭐") : v === "AUTO-FICHAJES" ? t("Automático: solo con fichajes nuevos", "Auto: only with new transfers") : t("A mano: pulsas Activar en el juego", "Manual: press Activate in the game")}>
    <${Para}>${t("Para qué sirve: elegir cómo carga el juego los fichajes nuevos. Lo normal es «Automático: siempre».", "What it's for: choose how the game loads new transfers. Normally «Auto: always».")}</${Para}>
    <${Segmentos} valor=${v} al=${(x) => accion("sync.modoRecarga", { modo: x })}
      opciones=${[{ valor: "AUTO-SIEMPRE", texto: t("🟣 AUTO: SIEMPRE ⭐", "🟣 AUTO: ALWAYS ⭐") }, { valor: "AUTO-FICHAJES", texto: t("🔵 AUTO: FICHAJES", "🔵 AUTO: TRANSFERS") }, { valor: "ACTIVAR", texto: t("🟢 ACTIVAR", "🟢 ACTIVATE") }]}/>
    <div class="ayuda" style="font-size:14px">${t(MODOS_RECARGA[v][0], MODOS_RECARGA[v][1])}</div>
    <div class="ayuda" style="font-size:13px">${t("⭐ Automático: siempre es el recomendado y viene por defecto. Necesita phoenix.lua v0.18b (instálala abajo, en Módulos del juego). Si lo automático falla 3 veces, el juego vuelve solo a Activar. El juego nota el cambio en unos 2 segundos.", "⭐ Auto: always is recommended and the default. Needs phoenix.lua v0.18b (install it below, in Game modules). If auto fails 3 times, the game falls back to Activate. The game notices the change in about 2 seconds.")}</div>
  </${Seccion}>`;
}

const Para = ({ children }) => html`<div class="ayuda" style="font-size:14px">${children}</div>`;

function Avanzado({ m }) {
  const [mods, setMods] = useState(null);
  const cargarMods = async () => { const r = await accion("sync.modulos", {}, { silencioso: true }); if (r) setMods(r); };
  const [fich, setFich] = useState(null);
  const cargarFich = async () => { const r = await accion("fichajes.estado", {}, { silencioso: true }); if (r) setFich(r); };
  const [tr, setTr] = useState(null);
  const cargarTr = async () => { const r = await accion("traspasos.estado", {}, { silencioso: true }); if (r) setTr(r); };
  useEffect(() => { cargarMods(); cargarFich(); cargarTr(); const id = setInterval(cargarTr, 6000); return () => clearInterval(id); }, []);
  const b = m.buzon || { activo: true, estado: "cerrado", ultimo: "", juego: "", avisos: [] };
  const [texto, punto, ayuda] = ESTADOS(b);
  const web = tr?.web || {};
  return html`<div class="col" style="gap:14px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:8px">
      <${Titulo} texto=${t("OPCIONES AVANZADAS", "ADVANCED OPTIONS")}/>
      <div style="font-size:15px">${t("Solo para cuando algo no va bien o quieres ajustar algo. Lo normal está en «Mi PC» y «Traspasos».", "Only for when something goes wrong or you want to tweak something. The normal things are in «My PC» and «Transfers».")}</div>
    </${Tarjeta}>

    <${Seccion} id="puente" titulo=${t("CONEXIÓN CON EL JUEGO", "GAME CONNECTION")} punto=${punto} resumen=${texto}>
      <${Para}>${t("Para qué sirve: que los avisos de la web salgan dentro del juego (panel de Sider, barra espaciadora).", "What it's for: so web notices show inside the game (Sider panel, space bar).")}</${Para}>
      <div class="ayuda" style="font-size:14px">${ayuda}</div>
      <div>
        <div class="lab" style="margin-bottom:6px">${t("JUEGO DETECTADO", "DETECTED GAME")}</div>
        ${b.juego ? html`<div class="caja mono" style="font-size:13px;word-break:break-all">${b.juego}</div>`
                  : html`<div class="ayuda">${t("Ninguno por ahora.", "None right now.")}</div>`}
        ${b.parche ? html`<div class="ayuda" style="margin-top:6px">${t("Parche detectado:", "Detected patch:")} <b>${b.parche}</b></div>` : null}
      </div>
      <${AjusteSw} titulo=${t("Mostrar avisos de la web en el juego", "Show web notices in the game")}
        desc=${t("Con PES 2021 abierto, los avisos salen en el panel de Sider (barra espaciadora). Si lo apagas, no se consulta ni se escribe nada.", "With PES 2021 open, notices show in the Sider panel (space bar). If off, nothing is fetched or written.")}
        valor=${b.activo} al=${(v) => accion("ajustes.avisosJuego", { valor: v })}/>
    </${Seccion}>
    <${ModoRecarga} m=${m} mods=${mods}/>
    <${Fichajes} d=${fich} setD=${setFich} cargar=${cargarFich}/>
    <${Entrega} m=${m}/>
    <${Modulos} d=${mods} setD=${setMods} cargar=${cargarMods}/>
    <${Doctor}/>
    <${SoloPes} m=${m}/>
    <${Seccion} id="avisos" titulo=${t("ÚLTIMOS AVISOS", "LATEST NOTICES")}
      resumen=${(b.avisos || []).length === 0 ? t("Aún no hay avisos", "No notices yet") : t(`${(b.avisos || []).length} de 5`, `${(b.avisos || []).length} of 5`)}>
      <${Para}>${t("Para qué sirve: ver los últimos avisos que la web mandó al juego.", "What it's for: see the latest notices the web sent to the game.")}</${Para}>
      ${(b.avisos || []).length === 0
        ? html`<div class="ayuda">${t("Aún no hay avisos. Los que mandes desde la web aparecerán aquí.", "No notices yet. Anything you send from the web shows up here.")}</div>`
        : (b.avisos || []).map((a) => html`<div class="fila-mar" key=${a.id} style="align-items:flex-start">
            <span class="mono mut" style="font-size:13px;flex:none;padding-top:2px">${a.hora}</span>
            <div style="flex:1;min-width:0;word-break:break-word">${a.texto}</div>
            ${a.escrito ? html`<${Chip} tipo="acc">✅ ${t("EN EL JUEGO", "IN GAME")}</${Chip}>` : html`<${Chip}>${t("PENDIENTE", "PENDING")}</${Chip}>`}
          </div>`)}
    </${Seccion}>
    <${Seccion} id="cuentaweb" titulo=${t("CUENTA WEB DETALLADA", "WEB ACCOUNT IN DETAIL")} resumen=${m.web?.usuario || t("Sin unir", "Not linked")}>
      <${Para}>${t("Para qué sirve: ver el estado de la unión con tu cuenta, reintentar, soltar al rival del radar o separar esta PC. Unir la cuenta se hace en «Mi PC».", "What it's for: see the account link status, retry, release the rival from the radar or unlink this PC. Linking is done in «My PC».")}</${Para}>
      <${CuentaWeb} m=${m}/>
    </${Seccion}>
    <${Seccion} id="tecnicos" titulo=${t("DETALLES TÉCNICOS", "TECHNICAL DETAILS")} resumen=${m.app?.version ? "Phoenix Link " + m.app.version : ""}>
      <${Para}>${t("Para qué sirve: datos para quien te ayude a revisar un problema. No hace falta tocar nada aquí.", "What it's for: data for whoever helps you check a problem. Nothing to touch here.")}</${Para}>
      <div class="mono mut" style="display:flex;flex-direction:column;gap:4px;font-size:12px;word-break:break-all">
        ${m.app?.version ? html`<span>Phoenix Link ${m.app.version}${m.app.compilado ? " · " + m.app.compilado : ""}</span>` : null}
        ${mods?.juego ? html`<span>${t("Carpeta del juego: ", "Game folder: ")}${mods.juego}</span>` : null}
        ${tr ? html`<span>${t("Modo de recarga: ", "Reload mode: ")}${tr.modoRecarga || "AUTO-SIEMPRE"} · PES ${tr.pesAbierto ? t("abierto", "open") : t("cerrado", "closed")}</span>` : null}
        ${tr ? html`<span>${t("Conexión de traspasos: ", "Live transfers connection: ")}${web.estado || "—"}${web.texto ? " · " + web.texto : ""}</span>` : null}
        ${web.tecnico ? html`<span>${t("Web (técnico): ", "Web (technical): ")}${JSON.stringify(web.tecnico)}</span>` : null}
        ${tr?.anfitrion?.archivo?.ruta ? html`<span>${t("Archivo de guardado: ", "Save file: ")}${tr.anfitrion.archivo.ruta}</span>` : null}
      </div>
    </${Seccion}>
  </div>`;
}

// Carpeta del juego: la que vio Link, o la que pegues a mano si no la ha visto (solo se valida y se recuerda; no escribe en el juego).
function CarpetaJuego({ d, setD }) {
  const [editar, setEditar] = useState(false);
  const [ruta, setRuta] = useState("");
  const guardar = async () => {
    const r = await accion("sync.carpetaJuego", { ruta });
    if (r) { setD(r); setEditar(false); setRuta(""); if (r.mensaje) avisar(r.mensaje, "ok", 5000); }
  };
  const abierto = editar || !d.juego;
  return html`<div class="col" style="gap:6px">
    ${d.juego ? html`<div class="ayuda" style="font-size:13px">${t("Carpeta del juego:", "Game folder:")} <span class="mono">${d.juego}</span> <button style="background:none;border:0;color:inherit;text-decoration:underline;cursor:pointer;font:inherit" onClick=${() => setEditar(!editar)}>${editar ? t("cancelar", "cancel") : t("cambiar", "change")}</button></div>` : null}
    ${abierto ? html`<div class="ayuda" style="font-size:13px">${d.juego ? t("Pega la carpeta de PES 2021 (la que tiene PES2021.exe y SiderAddons):", "Paste the PES 2021 folder (the one with PES2021.exe and SiderAddons):")
        : t("Link aún no ha visto tu juego. Abre PES 2021 una vez con Link abierto, o pega aquí su carpeta (la que tiene PES2021.exe y SiderAddons). Truco: en el Explorador, clic en la barra de arriba, copia y pega aquí.", "Link hasn't seen your game yet. Open PES 2021 once with Link running, or paste its folder here (the one with PES2021.exe and SiderAddons). Tip: in Explorer, click the top bar, copy and paste here.")}</div>
      <div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo mono" maxLength=${500} value=${ruta} placeholder="D:\\Juegos\\PES 2021" onInput=${(e) => setRuta(e.currentTarget.value)} onKeyDown=${(e) => { if (e.key === "Enter") { if (ruta.trim()) guardar(); } }}/></div><${Boton} tipo="lleno mini" deshabilitado=${!ruta.trim()} al=${guardar}>${t("GUARDAR", "SAVE")}</${Boton}></div>` : null}
  </div>`;
}

// ---- Phoenix Doctor (MODO INFORMATIVO): ficha del juego del anfitrión. Aún no hay parche oficial: no aprueba ni rechaza. ----
function Doctor() {
  const [d, setD] = useState(null);
  const [ver, setVer] = useState(false);
  const revisar = async () => { const r = await accion("sync.doctor", {}, { silencioso: true }); if (r) setD(r); };
  useEffect(() => { revisar(); }, []);
  const phoenixLuas = (d?.luaActivos || []).filter((x) => /phoenix/i.test(x));
  const conDatos = (d?.archivos || []).filter((a) => a.sha256);
  return html`<${Seccion} id="doctor" titulo=${t("REVISIÓN DEL PARCHE", "PATCH CHECK")}
    resumen=${!d ? t("Revisando…", "Checking…") : !d.juegoEncontrado ? t("No encuentro el juego", "Game not found") : t(`Informativo · parche ${d.parche}`, `Info only · patch ${d.parche}`)}
    acciones=${html`<${Boton} tipo="suave mini" al=${revisar}>${t("REVISAR", "CHECK")}</${Boton}>`}>
    <div class="ayuda" style="font-size:14px">
      ${t("Para qué sirve: una ficha de tu juego (qué parche tienes y una huella de sus archivos clave). Viaja con los resultados oficiales para que el staff la vea. No aprueba ni rechaza nada.",
          "What it's for: a sheet of your game (which patch you have and a fingerprint of its key files). It travels with official results so the staff can see it. It approves or rejects nothing.")}
    </div>
    ${!d ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : !d.juegoEncontrado
      ? html`<div class="ayuda">${(d.avisos || [])[0] || t("No encuentro el juego.", "Game not found.")}</div>`
      : html`<div class="col" style="gap:6px;font-size:14px">
          <div><span class="mut">${t("Parche:", "Patch:")}</span> <b>${d.parche}</b> · ${(d.modos || []).length} ${t("carpetas de Sider", "Sider folders")}</div>
          <div><span class="mut">${t("Módulos Phoenix activos:", "Active Phoenix modules:")}</span> <b>${phoenixLuas.length ? phoenixLuas.join(", ") : t("ninguno", "none")}</b> · ${(d.luaActivos || []).length} ${t("Luas en total", "Luas in total")}</div>
          <div><span class="mut">${t("Raíces de la base:", "Database roots:")}</span> ${(d.cpkRoots || []).slice(0, 3).join(" → ")}${(d.cpkRoots || []).length > 3 ? " …" : ""}</div>
          <div><span class="mut">${t("Huella del anfitrión:", "Host fingerprint:")}</span> <span class="mono" title=${d.huella}>${(d.huella || "").slice(0, 16)}…</span> <span class="mut">· ${d.generado}</span></div>
          ${(d.avisos || []).map((x) => html`<div style="color:var(--warn)">⚠️ ${x}</div>`)}
          <div><${Boton} tipo="mini suave" al=${() => setVer(!ver)}>${ver ? t("OCULTAR ARCHIVOS", "HIDE FILES") : t(`VER ARCHIVOS (${conDatos.length})`, `SHOW FILES (${conDatos.length})`)}</${Boton}></div>
          ${ver ? html`<div class="mono mut" style="font-size:12px;display:flex;flex-direction:column;gap:2px">
            ${(d.archivos || []).map((a) => html`<span key=${a.nombre}>${a.sha256 ? "●" : "·"} ${a.nombre}${a.sha256 ? " · " + a.sha256.slice(0, 12) + "…" : t(" · no está", " · missing")}</span>`)}
          </div>` : null}
        </div>`}
  </${Seccion}>`;
}

// ---- Fichajes de Phoenix: Link genera Phoenix-DB sobre TU base (la del parche/modo activo) + una lista de cambios ----
const ESTADO_FICH = {
  sin_base: [["BASE NO RECONOCIDA", "BASE NOT RECOGNISED"], ""],
  nada: [["NADA QUE APLICAR", "NOTHING TO APPLY"], ""],
  sin_aplicar: [["LISTO PARA APLICAR", "READY TO APPLY"], "vip"],
  aplicado: [["APLICADO", "APPLIED"], "acc"],
  desfasado: [["HAY QUE VOLVER A APLICAR", "RE-APPLY NEEDED"], "vip"],
  cambiado: [["LO CAMBIÓ OTRA HERRAMIENTA", "CHANGED BY ANOTHER TOOL"], "vip"],
};
function Fichajes({ d, setD, cargar }) {
  const [ocupado, setOcupado] = useState(false);
  const hacer = async (que) => {
    setOcupado(true);
    const r = await accion("fichajes.accion", { accion: que });
    setOcupado(false);
    if (r) { setD(r); if (r.mensaje) avisar(r.mensaje, "ok", 9000); }
  };
  const obj = d?.objetivos || [];
  const n = d?.lista?.cambios ?? 0;
  const hayGenerado = obj.some((x) => x.estado === "aplicado" || x.estado === "desfasado" || x.estado === "cambiado");
  const bloqueado = ocupado || !d || !!d.pesAbierto || !d.juego || !!d.lista?.error;
  const aplicados = obj.filter((x) => x.estado === "aplicado").length;
  return html`<${Seccion} id="fichajes" titulo=${t("FICHAJES ANTIGUOS (LISTA DE PHOENIX SYNC)", "OLD TRANSFERS (PHOENIX SYNC LIST)")}
    resumen=${!d ? t("Revisando…", "Checking…") : d.lista?.error ? t("Hay un problema con la lista", "There is a problem with the list")
      : n === 0 ? t("Lista vacía: nada que aplicar", "Empty list: nothing to apply")
      : t(`${n} ${n === 1 ? "cambio" : "cambios"} · aplicados en ${aplicados} de ${obj.length} modos`, `${n} ${n === 1 ? "change" : "changes"} · applied in ${aplicados} of ${obj.length} modes`)}
    acciones=${html`<${Boton} tipo="suave mini" al=${cargar}>${t("REVISAR", "CHECK")}</${Boton}>`}>
    <div class="ayuda" style="font-size:14px">
      ${t("Para qué sirve: pone en tu juego la lista de fichajes que dejó Phoenix Sync (no son los «Traspasos en vivo»). Se hace sobre TU base, con copia, y se puede quitar con un clic.",
          "What it's for: puts in your game the transfer list Phoenix Sync left (not the «Live transfers»). It's done on YOUR database, with a backup, and can be removed with one click.")}
    </div>
    ${d?.pesAbierto ? html`<div class="caja" style="font-size:14px"><span class="punto warn"></span> ${t("Cierra PES 2021 (y Sider) para aplicar o quitar: lee los datos al arrancar.", "Close PES 2021 (and Sider) to apply or remove: it reads the data at startup.")}</div>` : null}
    ${!d ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : html`
      <div class="ayuda" style="font-size:13px">${t(`Lista: ${n} ${n === 1 ? "cambio" : "cambios"} · regla ${d.lista?.regla === "estricta" ? "estricta" : "relajada"} · `, `List: ${n} ${n === 1 ? "change" : "changes"} · ${d.lista?.regla === "estricta" ? "strict" : "relaxed"} rule · `)}<span class="mono">${d.lista?.fuente || ""}</span></div>
      ${d.lista?.error ? html`<div class="caja" style="font-size:14px"><span class="punto bad"></span> ${d.lista.error}</div>` : null}
      ${n === 0 && !d.lista?.error ? html`<div class="ayuda" style="font-size:14px">${t("La lista de fichajes está vacía: no hay nada que aplicar y el juego queda con la base original del parche.", "The transfer list is empty: nothing to apply and the game keeps the patch's original database.")}</div>` : null}
      ${obj.map((x) => {
        const [et, tipo] = ESTADO_FICH[x.estado] || [[x.estado, x.estado], ""];
        return html`<div class="caja" key=${x.nombre} style="display:flex;flex-direction:column;gap:6px">
          <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
            <b style="font-size:14px">${x.nombre}</b>
            <${Chip} tipo=${tipo}>${t(et[0], et[1])}</${Chip}>
          </div>
          <div class="ayuda" style="font-size:13px">${x.parche ? t(`Parche detectado: ${x.parche}${x.baseGenerica ? " (base genérica)" : ""}`, `Detected patch: ${x.parche}${x.baseGenerica ? " (generic base)" : ""}`) : t("Parche: no se pudo detectar", "Patch: could not be detected")}</div>
          <details class="sec-detalle"><summary>${t("Ver detalle", "Details")}</summary>
          <div class="ayuda mono" style="font-size:12px">${x.baseOk ? t(`Base: ${x.baseOrigen} · ${x.filasBase} filas`, `Base: ${x.baseOrigen} · ${x.filasBase} rows`) : x.error}</div>
          ${!x.baseOk && x.queHacer ? html`<div class="ayuda" style="font-size:13px">${t("Qué hacer: ", "What to do: ")}${x.queHacer}</div>` : null}
          ${(x.comoLoSupe || []).length ? html`<details class="ayuda" style="font-size:12px"><summary>${t("Cómo lo supe", "How I know")}</summary>${x.comoLoSupe.map((e, i) => html`<div key=${i} class="mono mut">· ${e}</div>`)}</details>` : null}
          ${x.baseOk ? html`<div class="ayuda" style="font-size:13px">${t("Fichajes: ", "Transfers: ")}${n === 0 ? t("la lista está vacía", "list is empty") : t(`${x.aplicados} aplicados · ${x.yaEstaban} ya estaban · ${x.saltados.length} saltados`, `${x.aplicados} applied · ${x.yaEstaban} already there · ${x.saltados.length} skipped`)}</div>` : html`<div class="ayuda" style="font-size:13px">${t("Fichajes: no disponible en este modo (no se toca nada)", "Transfers: not available in this mode (nothing is touched)")}</div>`}
          ${x.baseOk && n > 0 ? html`<div class="ayuda" style="font-size:13px">${x.playerBinAjeno ? t(" Phoenix-DB tiene un Player.bin que no es el de la base: se apartará (con copia) para no tapar a los jugadores del parche.", " Phoenix-DB has a Player.bin that is not the base's: it will be set aside (with a backup) so it does not cover the patch's players.") : ""}</div>` : null}
          ${(x.saltados || []).slice(0, 3).map((s) => html`<div class="mono mut" style="font-size:12px" key=${s.jugador + "-" + s.equipo}>· ${s.jugador} → ${s.equipo}: ${s.motivo}</div>`)}
          ${(x.saltados || []).length > 3 ? html`<div class="mono mut" style="font-size:12px">… ${t(`y ${x.saltados.length - 3} más (están en el informe de diagnóstico)`, `and ${x.saltados.length - 3} more (in the diagnostic report)`)}</div>` : null}
          </details>
        </div>`;
      })}
      <div style="display:flex;gap:8px;flex-wrap:wrap">
        <${Boton} tipo="lleno mini" deshabilitado=${bloqueado || n === 0} titulo=${t("Genera los fichajes sobre la base de cada modo (con copia de seguridad)", "Builds the transfers on each mode's base (with backup)")}
          al=${async () => { if (await confirmar({ titulo: t("¿Aplicar los fichajes de Phoenix?", "Apply Phoenix transfers?"), texto: t("Link generará PlayerAssignment.bin en Phoenix-DB de cada modo con base reconocida, sobre tu base, y añadirá la raíz Phoenix-DB a sider.ini si falta (con copia). Todo o nada; puedes quitarlo cuando quieras.", "Link will build PlayerAssignment.bin in each recognised mode's Phoenix-DB on top of your base and add the Phoenix-DB root to sider.ini if missing (with backup). All or nothing; remove any time."), si: t("Aplicar", "Apply") })) hacer("aplicar"); }}>${t("APLICAR FICHAJES DE PHOENIX", "APPLY PHOENIX TRANSFERS")}</${Boton}>
        <${Boton} tipo="suave mini" deshabilitado=${bloqueado || !hayGenerado} titulo=${t("Vuelve a la base original del parche (la copia de seguridad se queda)", "Back to the patch's original database (backup stays)")}
          al=${() => hacer("quitar")}>${t("QUITAR", "REMOVE")}</${Boton}>
      </div>`}
  </${Seccion}>`;
}

// ---- Módulos del juego: el paquete de Luas de Sider que instala Phoenix Link (solo con PES cerrado) ----
const ESTADO_MODULO = {
  instalado: [["INSTALADO", "INSTALLED"], "acc"],
  desactualizado: [["HAY VERSIÓN NUEVA", "UPDATE AVAILABLE"], "vip"],
  apagado: [["APAGADO", "OFF"], ""],
  a_medias: [["A MEDIAS", "PARTIAL"], "vip"],
  perdido: [["SE DESACTIVÓ TRAS UNA ACTUALIZACIÓN: REINSTALAR", "TURNED OFF AFTER A PATCH UPDATE: REINSTALL"], "vip"],
  no_instalado: [["NO INSTALADO", "NOT INSTALLED"], ""],
  solo_lectura: [["LO GESTIONA SYNC", "MANAGED BY SYNC"], ""],
};
function Modulos({ d, setD, cargar }) {
  const [ocupado, setOcupado] = useState("");
  const hacer = async (archivo, que) => {
    setOcupado(archivo);
    const r = await accion("sync.modulo", { archivo, accion: que });
    setOcupado("");
    if (r) { setD(r); if (r.mensaje) avisar(r.mensaje, "ok", 7000); }
  };
  const lista = d?.modulos || [];
  const gest = lista.filter((x) => x.gestionable);
  const hayPend = !!d && (!d.juego || gest.some((x) => x.estado !== "instalado"));
  return html`<${Seccion} id="modulos" titulo=${t("ARCHIVOS DE PHOENIX EN EL JUEGO", "PHOENIX FILES IN THE GAME")} abierta=${hayPend}
    punto=${!d ? null : hayPend ? "warn" : "ok"}
    resumen=${!d ? t("Revisando…", "Checking…") : !d.juego ? t("No sé dónde está tu juego", "I don't know where your game is") : hayPend ? t("Hay archivos por instalar o actualizar", "Some files need installing or updating") : t("Instalados y al día", "Installed and up to date")}
    acciones=${html`<${Boton} tipo="suave mini" al=${cargar}>${t("REVISAR", "CHECK")}</${Boton}>`}>
    <div class="ayuda" style="font-size:14px">
      ${t("Para qué sirve: instalar, actualizar, apagar o quitar uno por uno los archivos de Phoenix. Lo normal es hacerlo todo con ARREGLAR TODO en «Mi PC». Antes de cada cambio se guarda una copia.",
          "What it's for: install, update, turn off or remove Phoenix's files one by one. Normally you do it all with FIX EVERYTHING in «My PC». A backup is saved before every change.")}
    </div>
    ${d?.pesAbierto ? html`<div class="caja" style="font-size:14px"><span class="punto warn"></span> ${t("Cierra PES 2021 para instalar o apagar: el juego lee estas piezas al arrancar.", "Close PES 2021 to install or turn off: the game reads these pieces at startup.")}</div>` : null}
    ${d?.parche && (d.parche.modosRaiz?.length || d.parche.modosPropios?.length) ? html`<div class="ayuda" style="font-size:13px">
      ${d.parche.modoActual ? t(`Último modo lanzado: ${d.parche.modoActual}. `, `Last mode launched: ${d.parche.modoActual}. `) : null}
      ${d.parche.modosRaiz?.length ? t(`Usan el sider.ini de la raíz: ${d.parche.modosRaiz.join(", ")}. `, `Use the root sider.ini: ${d.parche.modosRaiz.join(", ")}. `) : null}
      ${d.parche.modosPropios?.length ? t(`Con sider.ini propio: ${d.parche.modosPropios.join(", ")}.`, `With their own sider.ini: ${d.parche.modosPropios.join(", ")}.`) : null}
    </div>` : null}
    ${(d?.parche?.advertencias || []).map((w) => html`<div class="caja" key=${w} style="font-size:14px"><span class="punto warn"></span> ${w}</div>`)}
    ${d ? html`<${CarpetaJuego} d=${d} setD=${setD}/>` : null}
    ${!d ? html`<div class="ayuda">${t("Revisando…", "Checking…")}</div>` : lista.map((x) => {
      const [et, tipo] = ESTADO_MODULO[x.estado] || [[x.estado, x.estado], ""];
      const puedeInstalar = x.gestionable && x.estado !== "instalado";
      const puedeQuitar = x.gestionable && (x.estado === "instalado" || x.estado === "desactualizado" || x.estado === "a_medias" || x.estado === "perdido");
      const bloqueado = !!d.pesAbierto || !d.juego || !!ocupado;
      return html`<div class="caja" key=${x.archivo} style="display:flex;flex-direction:column;gap:8px">
        <div style="display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap">
          <b class="mono" style="font-size:14px">${x.archivo}${x.version ? html` <span class="mut">v${x.version}</span>` : null}</b>
          <${Chip} tipo=${tipo}>${t(et[0], et[1])}</${Chip}>
        </div>
        <div class="ayuda" style="font-size:13px">${x.descripcion}</div>
        ${x.carga ? html`<div class="ayuda" style="font-size:13px">${x.carga === "cargado" ? t("✅ Cargó en el último arranque del juego.", "✅ Loaded on the last game start.") : t("⚠️ Sider no lo pudo cargar en el último arranque (mira sider.log).", "⚠️ Sider could not load it on the last start (see sider.log).")}</div>` : null}
        ${(x.destinos || []).length ? html`<div class="mono mut" style="font-size:12px;display:flex;flex-direction:column;gap:2px">
          ${x.destinos.map((ds) => html`<span key=${ds.carpeta}>${ds.linea ? "●" : ds.comentada ? "○" : "·"} ${ds.nombre}${ds.version ? " · v" + ds.version : ""}${ds.archivo && x.gestionable && !ds.igual ? t(" · distinto", " · different") : ""}${ds.perdida ? t(" · SIN CARGAR (falta su línea en sider.ini)", " · NOT LOADING (its sider.ini line is gone)") : ""}${x.archivo === "phoenix.lua" && ds.sinBase ? t(" · sin base reconocida: Phoenix-DB no se pone", " · no recognised base: Phoenix-DB is not added") : ""}</span>`)}
        </div>` : null}
        ${x.gestionable ? html`<div style="display:flex;gap:8px;flex-wrap:wrap">
          ${puedeInstalar ? html`<${Boton} tipo="lleno mini" deshabilitado=${bloqueado} al=${() => hacer(x.archivo, "instalar")}>${x.estado === "desactualizado" ? t("ACTUALIZAR", "UPDATE") : x.estado === "apagado" ? t("ENCENDER", "TURN ON") : x.estado === "perdido" ? t("REINSTALAR", "REINSTALL") : t("INSTALAR", "INSTALL")}</${Boton}>` : null}
          ${puedeQuitar ? html`<${Boton} tipo="suave mini" deshabilitado=${bloqueado} titulo=${t("Deja de cargarse en el juego; el archivo se queda para volver a encenderlo", "Stops loading in the game; the file stays")} al=${() => hacer(x.archivo, "quitar")}>${t("APAGAR", "TURN OFF")}</${Boton}>` : null}
          ${x.gestionable && x.estado !== "no_instalado" ? html`<${Boton} tipo="peligro mini" deshabilitado=${bloqueado} titulo=${t("Lo quita del juego por completo (con copia de seguridad)", "Removes it from the game completely (with backup)")}
            al=${async () => { if (await confirmar({ titulo: t(`¿Desinstalar ${x.archivo}?`, `Uninstall ${x.archivo}?`), texto: x.archivo === "phoenix.lua"
              ? t("Se quita del juego y de sider.ini (también la carpeta Phoenix-DB deja de usarse: vuelves a los datos del parche). Se guarda copia de todo y puedes volver a instalarlo cuando quieras.", "Removed from the game and sider.ini (Phoenix-DB stops being used). Everything is backed up; reinstall any time.")
              : t("Se quita del juego y de sider.ini. Se guarda copia de todo y puedes volver a instalarlo cuando quieras.", "Removed from the game and sider.ini. Everything is backed up; reinstall any time."), si: t("Desinstalar", "Uninstall"), peligro: true })) hacer(x.archivo, "desinstalar"); }}>${t("DESINSTALAR", "UNINSTALL")}</${Boton}>` : null}
        </div>` : null}
      </div>`;
    })}
  </${Seccion}>`;
}

function Entrega({ m }) {
  const en = m.entrega || { estado: "ninguna" };
  const tabla = {
    ninguna: [t("Aún no hay entregas", "No deliveries yet"), "", ""],
    esperando: [t("Entrega esperando al juego", "Delivery waiting for the game"), "warn", t("Abre PES 2021 una vez para que Phoenix Link encuentre su carpeta. No se escribe nada hasta entonces.", "Open PES 2021 once so Phoenix Link can find its folder. Nothing is written until then.")],
    colocada: [t("Colocada", "Placed"), "ok", t("En el juego pulsa: Partido → Datos Actual. en vivo → Activar.", "In the game press: Match → Live data update → Activate.")],
    rechazada: [t("Rechazada", "Rejected"), "bad", t("No se tocó nada de tu juego.", "Nothing in your game was touched.")],
    deshecha: [t("Deshecha", "Undone"), "warn", t("Se restauraron los datos anteriores. Pulsa Activar en el juego para recargarlos.", "Previous data restored. Press Activate in the game to reload it.")],
  };
  const [texto, punto, ayuda] = tabla[en.estado] || tabla.ninguna;
  return html`<${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:14px">
    <${Titulo} texto=${t("ÚLTIMA ENTREGA DE PHOENIX SYNC", "LAST PHOENIX SYNC DELIVERY")} derecha=${en.fecha ? html`<span class="mono mut">${en.fecha}</span>` : null}/>
    <div style="display:flex;align-items:center;gap:12px">
      <span class=${cx("punto", punto)} style="width:14px;height:14px"></span>
      <span class="disp" style="font-size:22px;font-weight:700">${texto}</span>
    </div>
    ${en.resumen ? html`<div class="caja" style="font-size:14px;word-break:break-word">${en.resumen}</div>` : null}
    ${en.motivo ? html`<div class="ayuda" style="font-size:14px;color:var(--bad, #ff6b6b);word-break:break-word">${en.motivo}</div>` : null}
    ${ayuda ? html`<div class="ayuda" style="font-size:14px">${ayuda}</div>` : null}
    <div>
      <${Boton} tipo="suave" deshabilitado=${!en.puedeDeshacer}
        titulo=${t("Restaura el option file y el Player.bin anteriores", "Restores the previous option file and Player.bin")}
        al=${() => accion("sync.deshacerEntrega", {}, { ok: t("Orden enviada: se deshace en unos segundos.", "Sent: it will be undone in a few seconds.") })}>${t("Deshacer última entrega", "Undo last delivery")}</${Boton}>
    </div>
  </${Tarjeta}>`;
}

export function VistaSync({ s, pestana }) {
  const m = s.motor;
  if (!m) return html`<${Vacio} titulo=${t("Conectando…", "Connecting…")} texto=""/>`;
  if (pestana === "traspasos") return html`<${Traspasos}/>`;
  if (pestana === "avanzado") return html`<${Avanzado} m=${m}/>`;
  return html`<${Preparar} s=${s}/>`;
}
