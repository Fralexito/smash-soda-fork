// =============================================================================
//  Motor simulado (solo vista previa en un navegador normal, nunca dentro de
//  Phoenix Link). Implementa las mismas acciones y forma de estado que el C++
//  para poder revisar el diseño y probar la interfaz sin Windows.
//  Escenarios (en la URL): ?escenario=cerrada · ?escenario=vacia (sala abierta
//  sin nadie) · ?escenario=cruzado (el puesto 01 es del Equipo B y el 02 del A; tú en el 01) · ?escenario=llena (por defecto: 6 personas, 2 solicitudes,
//  1 en espera, pings variados y un partido en curso 2–1).
// =============================================================================

import { aplicar as aplicarMapeo, pulsacionNueva, limpiar as limpiarMapeo, esIdentidad as mapeoIdentidad } from "./mapeo.js";

// Mandos › Mapeo: el «mando» del invitado observado tiene el stick izquierdo que camina solo,
// A y B cambiados y el stick izquierdo al revés en vertical. El invitado «obedece» al chat:
// AVISAR POR CHAT («@Mirko pulsa el botón que quieres que sea A») → pulsa B en su mando.
// En la consola también: simPulsar("a") | simPulsar("lt") | simPulsar("lx+").
const ETQ_SIM = { A: "a", B: "b", X: "x", Y: "y", LB: "lb", RB: "rb", LT: "lt", RT: "rt", BACK: "back", START: "start", "GUÍA": "guia", L3: "ls", R3: "rs", "▲": "arriba", "▼": "abajo", "◀": "izq", "▶": "der" };
const FALLO_SIM = { a: "b", b: "a" };
function respuestaChatMapeo(texto) {
  const b = /(?:que sea|to be) (\S+)\s*$/.exec(texto);
  if (b && ETQ_SIM[b[1]]) { const f = ETQ_SIM[b[1]]; return FALLO_SIM[f] || f; }
  const izq = /IZQUIERDO|LEFT stick/.test(texto), horizontal = /DERECHA|stick RIGHT/.test(texto);
  if (/stick/i.test(texto)) return horizontal ? (izq ? "lx+" : "rx+") : (izq ? "ly-" : "ry+");
  return "";
}
const BITS_SIM = { arriba: 0x1, abajo: 0x2, izq: 0x4, der: 0x8, start: 0x10, back: 0x20, ls: 0x40, rs: 0x80, lb: 0x100, rb: 0x200, guia: 0x400, a: 0x1000, b: 0x2000, x: 0x4000, y: 0x8000 };
function crudoSimulado(seg, manual) {
  const e = { b: 0, lt: 0, rt: 0, lx: Math.round(2400 + Math.sin(seg * 3) * 500), ly: Math.round(-1700 + Math.cos(seg * 2) * 400), rx: 0, ry: 0 };
  const f = manual || "";
  if (!f) return e;
  const m = /^([lr][xy])([+-])$/.exec(f);
  if (m) e[m[1]] = m[2] === "+" ? 30000 : -30000;
  else if (f === "lt" || f === "rt") e[f] = 255;
  else e.b = BITS_SIM[f] || 0;
  return e;
}

export function crearSimulador(enviar) {
  const ahora = () => Date.now() / 1000;
  const inicio = ahora();
  let escenario = "llena";
  try { escenario = new URLSearchParams(location.search).get("escenario") || "llena"; } catch { /* sin URL */ }
  const llena = escenario === "llena";
  const s = {
    tema: "galaxy", idioma: "es", abierta: escenario !== "cerrada", abiertaEn: inicio - 6128,
    opciones: { nombre: "Phoenix · Galaxy League", plazas: 4, limitador: false, limite: 30, biblioteca: "Default", juegos: ["PES 2021", "Football Life 2026"], pendiente: false, turnos: false, quiosco: false, overlay: true },
    phoenix: { visibilidad: "amigos", espectadores: true, limiteEspectadores: 4, entradaParsec: true, juego: "eFootball PES 2021", parche: "Conmegol", region: "Lima" },
    calidad: { fps: 60, mbps: 15, auto: true, porPersona: 10, subida: 0, perdida: 0, energia: "ok" },
    // Equipo de cada puesto (como equipoPuesto del motor): A, B o - (fuera). «cruzado»: el 01 es del Equipo B y el 02 del A.
    mapa: escenario === "cruzado" ? "BABA----" : "AABB----",
    fijados: {},   // candado de puesto: id de Parsec → puesto
    botonesBloq: { mascara: 0x10 | 0x400, lt: false, rt: false, lx: false, ly: false, rx: false, ry: false },
    perfilesTeclado: [
      { userId: 0, nombre: "Default", teclas: [["lleft", 30, "A"], ["lright", 32, "D"], ["lup", 17, "W"], ["ldown", 31, "S"], ["rleft", 331, "←"], ["rright", 333, "→"], ["rup", 328, "↑"], ["rdown", 336, "↓"], ["dleft", 75, "Num 4"], ["dright", 77, "Num 6"], ["dup", 72, "Num 8"], ["ddown", 76, "Num 5"], ["a", 38, "L"], ["b", 24, "O"], ["x", 37, "K"], ["y", 23, "I"], ["lb", 16, "Q"], ["rb", 25, "P"], ["lt", 33, "F"], ["rt", 36, "J"], ["back", 14, "Retroceso"], ["start", 28, "Enter"], ["lthumb", 46, "C"], ["rthumb", 50, "M"]].map(([b, v, e]) => ({ b, v, e })) },
    ],
    marionetas: { motor: "sdl", maestro: -1, maestros: [{ n: 1, nombre: "Xbox Wireless Controller", tipo: "xbox", activo: true }, { n: 2, nombre: "Wireless Controller", tipo: "ds4", activo: false }], titeres: [1, 2, 3, 4, 5, 6, 7, 8].map((n) => ({ n, activo: false })), ocultos360: 8, mostrar360: false },
    xbox: 8, ds4: 0, bloqueoGlobal: false, bloqueoBotones: false, host: escenario === "cruzado" ? 1 : 0,
    invitados: escenario === "vacia" ? [] : [
      { parsecId: 1187, nombre: "Mirko", base: 18, mod: true, vip: false, teclado: false, raton: false, rolWeb: "jugador" },
      { parsecId: 2041, nombre: "Kaiser", base: 27, mod: false, vip: true, teclado: false, raton: false, rolWeb: "jugador" },
      { parsecId: 3310, nombre: "ElTigre", base: 58, mod: false, vip: false, teclado: true, raton: false, rolWeb: "jugador" },
      { parsecId: 4420, nombre: "Lucho", base: 84, mod: false, vip: false, teclado: false, raton: false, rolWeb: "jugador" },
      { parsecId: 6031, nombre: "Pibe", base: 132, mod: false, vip: false, teclado: false, raton: false, rolWeb: "no_listado" },
      { parsecId: 5126, nombre: "Zurdo10", base: 41, mod: false, vip: false, teclado: false, raton: false, rolWeb: "espectador" },
    ],
    mandos: escenario === "vacia" ? [0, 0, 0, 0, 0, 0, 0, 0] : escenario === "cruzado" ? [0, 3310, 2041, 4420, 0, 0, 0, 0] : [1187, 3310, 2041, 4420, 0, 0, 0, 0], bloqueados: new Set(), desconectados: new Set(),
    solicitudes: llena ? [{ parsecId: 6031, nombre: "Pibe", mando: 5 }, { parsecId: 4420, nombre: "Lucho", mando: 2 }, { parsecId: 9911, nombre: "Zurdo10", mando: -1 }] : [],
    espera: llena ? [{ parsecId: 7777, nombre: "Visitante99", ping: 66 }] : [],
    series: {},
    partido: llena
      ? { fase: "en_juego", a: { nombre: "Equipo A", jugadores: [{ parsecId: 1187, nombre: "Mirko" }, { parsecId: 3310, nombre: "ElTigre" }], goles: 2 }, b: { nombre: "Equipo B", jugadores: [{ parsecId: 2041, nombre: "Kaiser" }, { parsecId: 4420, nombre: "Lucho" }], goles: 1 }, inicio: inicio - 1114, acumulado: 0, invertido: false }
      : { fase: "libre", a: { nombre: "", jugadores: [], goles: 0 }, b: { nombre: "", jugadores: [], goles: 0 }, inicio: 0, acumulado: 0, invertido: false },
    historial: [
      { id: 1, inicioMs: Date.now() - 86400000, duracionSeg: 1210, a: { nombre: "Mirko", jugadores: [{ parsecId: 1187, nombre: "Mirko" }], goles: 3 }, b: { nombre: "Kaiser", jugadores: [{ parsecId: 2041, nombre: "Kaiser" }], goles: 1 }, ganador: "a" },
      { id: 2, inicioMs: Date.now() - 3600000, duracionSeg: 1180, a: { nombre: "ElTigre", jugadores: [{ parsecId: 3310, nombre: "ElTigre" }], goles: 2 }, b: { nombre: "Mirko", jugadores: [{ parsecId: 1187, nombre: "Mirko" }], goles: 2 }, ganador: "empate" },
    ],
    perfilesSala: [{ nombre: "Liga", valores: { plazas: 4, mbps: 15, local: 2, visitante: 2, visibilidad: "amigos" } }],
    buzon: { activo: true, estado: "conectado", ultimo: "00:30", juego: "C:\\Juegos\\PES 2021 ConmeGOL", parche: "ConmeGOL Patch 26", avisos: [{ id: 12, texto: "hola causa", hora: "00:30", escrito: true }, { id: 11, texto: "Tu fichaje de Kaiser ya está listo ⚡", hora: "00:12", escrito: true }, { id: 10, texto: "Recuerda: partido de liga hoy 9 pm", hora: "23:40", escrito: false }] },
    pes: { modo: "off", abierto: true },
    // Modo cedido (sin PIN: devolver la PC siempre le quita el control al invitado)
    cedido: { activo: false, parsecId: 0, nombre: "", desde: 0, ultimoFin: "" },
    biblioteca: [{ id: 1, nombre: "PES 2021", ruta: "C:\\Juegos\\PES 2021 ConmeGOL\\PES2021.exe", parametros: "" }, { id: 2, nombre: "Football Life 2026", ruta: "D:\\FL26\\FL26.exe", parametros: "" }],
    atajos: [{ comando: "!lockall", tecla: 76, nombre: "L" }],
    entrega: { estado: "colocada", id: "demo-1", resumen: "Lamine Yamal: velocidad 99", motivo: "", fecha: "2026-10-09 06:12", puedeDeshacer: true },
    ajustes: {
      general: { flashWindow: true, ttsEnabled: false, bonkEnabled: true, messageNotification: true, disableGuideButton: true, disableKeyboard: false, autoIndex: false, parsecLogs: false, ipBan: true, blockVPN: false, devMode: false, chatbot: "PhoenixBot", discord: "https://discord.gg/phoenix", welcomeMessage: "¡Bienvenido _PLAYER_! Respeta los turnos y diviértete.", socketEnabled: true, socketPort: 9002, socketActivo: true, autoMute: true, autoMuteTime: 500, muteTime: 5, hostBonkProof: false, sfxEnabled: true },
      permisos: { guest: { useBB: false, useSFX: true, changeControls: true }, vip: { useBB: true, useSFX: true, changeControls: true }, moderator: { useBB: true, useSFX: true, changeControls: true } },
      video: { monitor: 0, gpu: 0, captura: 0, resolucion: 0, lanczos: false, ritmo: true, fps: 60, mbps: 15 },
      overlay: { monitor: 0, tema: "", chat: { activo: true, historial: true, posicion: "top Left" }, mandos: { activo: true, posicion: "bottom center" }, invitados: { activo: true, latencia: true, posicion: "top right" }, corriendo: true },
    },
    audio: { mic: { activo: false, volumen: 80, dispositivo: 0, dispositivos: ["Micrófono (USB)"], mic: true }, parlantes: { activo: true, volumen: 100, dispositivo: 0, dispositivos: ["Altavoces (Realtek)", "Auriculares"], mic: false } },
    turnos: { activo: false, corriendo: false, usuarios: [], juegoMin: 10, reinicioMin: 30, recordatorioMin: 2 },
    baneados: [{ parsecId: 6666, nombre: "Troll", motivo: "Insultos" }],
    web: { estado: "conectado", usuario: "Fralex", mensaje: "Sala publicada en la web.", publicada: true },
    seccion: "sala", pestana: "resumen",
    mapeo: { perfiles: { 2041: { nombre: "Kaiser", actualizado: Date.now() - 86400000, perfil: { botones: {}, invertir: [], cambiarSticks: false, zona: [12, 0] } } }, ver: 0, n: 0, ultima: "", antes: null, manual: null, desde: 0 },
    sonidos: [{ ruta: "sfx/gol.wav", etiqueta: "gol", espera: 5 }, { ruta: "sfx/silbato.wav", etiqueta: "silbato", espera: 10 }, { ruta: "sfx/abucheo.wav", etiqueta: "abucheo", espera: 30 }],
    noticias: [
      { id: 7, nivel: "importante", texto: "Hoy 21:00 · Final de la Copa Galaxy en vivo. Entra a la sala del árbitro 10 minutos antes.", hora: "19:42", enlace: "" },
      { id: 8, nivel: "info", texto: "Mantenimiento de la web mañana a las 03:00 (unos 10 minutos).", hora: "18:05", enlace: "" },
    ],
    chatGlobal: { esperaSeg: 3, pausado: false, ultimoEnvio: 0, nextId: 104, mensajes: [
      { id: 101, usuarioId: "u1", nombre: "TU PAPI CHULO", texto: "hola", hora: "23:00", rol: "admin", propio: false },
      { id: 102, usuarioId: "u2", nombre: "Mirko", texto: "¿quién juega hoy la liga?", hora: "23:01", rol: "jugador", propio: false },
      { id: 103, usuarioId: "u3", nombre: "Lucía", texto: "yo, a las 9", hora: "23:02", rol: "moderador", propio: false },
    ] },
    chat: ["Mirko: buenas!", "Kaiser: listos para la revancha", "[PhoenixBot] ElTigre entró a la sala."],
    actividad: ["[PhoenixBot] Sala abierta con eFootball PES 2021", "[PhoenixBot] Mirko joined.", "[PhoenixBot] Kaiser joined.", "[PhoenixBot] Mando 03 asignado a ElTigre"],
    // Árbitro: estado.json del juego (?estado=archivo.json lo lee cada segundo) y preferencias
    juego: { datos: false }, golesJuego: null,
    arbitro: { competitivo: true, pausaAuto: false, marcadorAuto: true, soloAutorizados: true, modoRecarga: "AUTO-SIEMPRE", ultimaPausa: "", conStart: false, ladosAuto: true, ladosEstado: "" },
    golesAnunciados: -1,
    // Liga automática: partidos oficiales por jugar (GET /v1/liga/partidos)
    liga: { cargado: true, error: "", ultimoEnvio: "", pendientes: 0, elegido: 0, elegidoTexto: "", enVivo: false, partidos: [
      { id: 55, fecha: 9, edicion: "Galaxy League · 1° Edición 2026", estado: "pendiente", local: "Fralex", visitante: "Morgado", clubLocal: "FC Barcelona", clubVisita: "AC Milan", mio: true },
      { id: 51, fecha: 9, edicion: "Galaxy League · 1° Edición 2026", estado: "pendiente", local: "Roberto", visitante: "Degox", clubLocal: "Inter", clubVisita: "Atlético de Madrid", mio: false },
      { id: 60, fecha: 10, edicion: "Galaxy League · 1° Edición 2026", estado: "pendiente", local: "Fralex", visitante: "Jeremi", clubLocal: "FC Barcelona", clubVisita: "Chelsea", mio: true },
      { id: 56, fecha: 10, edicion: "Galaxy League · 1° Edición 2026", estado: "pendiente", local: "Jack", visitante: "Hugo", clubLocal: "Manchester United", clubVisita: "Arsenal", mio: false },
    ] },
  };
  // Prueba con un estado.json falso: ?estado=estado-prueba.json (junto a index.html). Sin archivo = «sin datos del juego».
  try {
    const archivo = new URLSearchParams(location.search).get("estado");
    if (archivo) setInterval(async () => {
      try {
        const r = await fetch(archivo, { cache: "no-store" });
        const j = r.ok ? await r.json() : null;
        const fases = ["menu", "en_juego", "pausado", "descanso", "final"];
        s.juego = j && fases.includes(j.fase)
          ? { datos: true, fase: j.fase, minuto: j.minuto | 0, periodo: j.periodo | 0, golesLocal: j.goles_local ?? -1, golesVisita: j.goles_visita ?? -1,
              pkLocal: j.pk_local ?? -1, pkVisita: j.pk_visita ?? -1, nombreLocal: j.nombre_local || "", nombreVisita: j.nombre_visita || "",
              relojCorre: j.reloj_corre !== false, completo: !!j.completo,
              goles: (Array.isArray(j.goles) ? j.goles : []).slice(0, 40).map((x) => ({ m: x.m | 0, local: x.l !== "v", jugador: typeof x.j === "string" ? x.j.slice(0, 32) : "", propia: x.pp === true })) }
          : { datos: false };
      } catch { s.juego = { datos: false }; }
      // Goles del juego → marcador (solo cuando cambian en el juego; la corrección manual se respeta)
      const p = s.partido, g = s.juego;
      const vivo = p.fase === "en_juego" || p.fase === "pausado";
      if (vivo && g.datos && g.golesLocal >= 0 && g.golesVisita >= 0) {
        const firma = g.golesLocal + ":" + g.golesVisita;
        if (firma !== s.golesJuego) {
          s.golesJuego = firma;
          p.a.goles = p.invertido ? g.golesVisita : g.golesLocal;
          p.b.goles = p.invertido ? g.golesLocal : g.golesVisita;
        }
      } else if (!vivo) s.golesJuego = null;
      // Marcador automático: relato de goles y final (como Arbitro.cpp)
      if (!vivo) s.golesAnunciados = -1;
      if (vivo && g.datos && s.arbitro.marcadorAuto) {
        const total = g.goles.length;
        if (s.golesAnunciados < 0 || total < s.golesAnunciados) s.golesAnunciados = total;
        for (let i = s.golesAnunciados; i < total; i++) {
          const x = g.goles[i], lado = (x.local !== p.invertido ? p.a : p.b).nombre;
          const quien = x.jugador ? ` (${x.jugador}${x.propia ? ", en propia" : ""})` : "";
          bot(`GOL ${x.m}' de ${lado}${quien}. ${p.a.nombre} ${p.a.goles}-${p.b.goles} ${p.b.nombre}`);
        }
        s.golesAnunciados = total;
        if (g.fase === "final" && g.completo) acciones["partido.finalizar"]({ anunciar: true });
      }
    }, 1000);
  } catch { /* sin URL */ }
  const perfiles = {
    1187: { parsec_id: 1187, nombre: "Mirko", avatar_url: null, carta: { media: 87, posicion: "DC", club: "Galaxy FC", rareza: "oro", pais: "PE", apodo: "El Mago", stats: { rit: 90, tir: 85, pas: 80, reg: 88, def: 40, fis: 75 } } },
    2041: { parsec_id: 2041, nombre: "Kaiser", avatar_url: null, carta: { media: 91, posicion: "MC", club: "Sudario United", rareza: "leyenda", pais: "AR", apodo: "Kaiser", stats: { rit: 78, tir: 82, pas: 93, reg: 90, def: 70, fis: 77 } } },
  };
  let siguienteId = 3;

  const nombreDe = (id) => (id === 99 ? "Fralex" : s.invitados.find((g) => g.parsecId === id)?.nombre || "#" + id);
  const pingDe = (id) => {
    const g = s.invitados.find((x) => x.parsecId === id);
    if (!g) return -1;
    return Math.max(5, Math.round(g.base + Math.sin(ahora() / 3 + id) * 8 + (Math.random() - .5) * 6));
  };
  const tick = () => {
    if (!s.abierta) return;
    for (const g of s.invitados) {
      const serie = (s.series[g.parsecId] ||= []);
      serie.push(pingDe(g.parsecId));
      if (serie.length > 120) serie.shift();
    }
  };
  for (let i = 0; i < 90; i++) tick();

  const segundosPartido = () => {
    const p = s.partido;
    if (p.fase === "en_juego") return Math.round(p.acumulado + (ahora() - p.inicio));
    return Math.round(p.acumulado);
  };
  const resumenRed = (todo) => s.invitados.map((g) => {
    const serie = s.series[g.parsecId] || [];
    const ult = serie.slice(-30);
    const media = ult.length ? Math.round(ult.reduce((a, b) => a + b, 0) / ult.length) : -1;
    let v = 0; for (let i = 1; i < ult.length; i++) v += Math.abs(ult[i] - ult[i - 1]);
    const jitter = ult.length > 1 ? Math.round(v / (ult.length - 1) * 10) / 10 : -1;
    const peor = Math.max(media <= 60 ? 0 : media <= 100 ? 1 : 2, jitter <= 10 ? 0 : jitter <= 25 ? 1 : 2);
    return { parsecId: g.parsecId, nombre: g.nombre, presente: true, ultimo: serie[serie.length - 1] ?? -1, media, maximo: Math.max(-1, ...ult), jitter, picos: ult.filter((x) => x > 100).length, alerta: false, semaforo: ["verde", "ambar", "rojo"][peor], serie: todo ? serie : serie.slice(-30) };
  });

  // Medidor de retraso del mando (como phoenix/core/MedidorRetraso: total = ping + vídeo + jitter; manda la peor regla)
  const PERFIL_RETRASO = { 3310: { saltos: 9, jitter: 6 }, 6031: { perdida: 3.2, jitter: 9 }, 4420: { jitter: 4 } };
  const resumenRetraso = () => s.invitados.map((g) => {
    const pr = PERFIL_RETRASO[g.parsecId] || {};
    const ping = pingDe(g.parsecId);
    const conMando = s.mandos.indexOf(g.parsecId) >= 0;
    const ruido = () => Math.round(Math.random() * 10) / 10;
    const mando = conMando ? { muestras: 128, media: 8.3, p95: Math.round((9 + (pr.saltos ? 50 : 2) * Math.random()) * 10) / 10, jitter: Math.round(((pr.jitter || 1.2) + ruido()) * 10) / 10, saltos: pr.saltos ? pr.saltos + Math.round(Math.random() * 3) : 0, ritmo: 118 + Math.round(Math.random() * 6), haceMs: 12 } : null;
    const codificar = 4.6, decodificar = Math.round((3 + (g.parsecId % 5)) * 10) / 10, perdida = pr.perdida || 0;
    const total = ping >= 0 ? Math.round(ping + codificar + decodificar + (mando ? mando.jitter : 0)) : -1;
    let nivel = -1, motivo = "";
    const subir = (n, mo) => { if (n > nivel) { nivel = n; motivo = n > 0 ? mo : ""; } };
    if (total >= 0) subir(total <= 80 ? 0 : total <= 130 ? 1 : 2, mando && mando.jitter >= ping ? "mando irregular" : "ping alto");
    if (perdida >= 5) subir(2, "pierde paquetes"); else if (perdida >= 2) subir(1, "pierde paquetes");
    if (mando) { if (mando.saltos >= 20) subir(2, "mando a saltos"); else if (mando.saltos >= 6) subir(1, "mando a saltos"); }
    const semaforo = nivel < 0 ? "sin_datos" : ["verde", "ambar", "rojo"][nivel];
    return { parsecId: g.parsecId, nombre: g.nombre, total, semaforo, motivo, ping, codificar, decodificar, perdida, cola: 0, ...(mando ? { mando } : {}) };
  });

  // Mando en vivo de Mandos › Mapeo (como el motor: solo mirando esa pestaña y con un mando tomado)
  const vistaMapeo = () => {
    const mp = s.mapeo;
    if (s.seccion !== "mandos" || s.pestana !== "mapeo" || !mp.ver) return null;
    if (!s.abierta || s.mandos.indexOf(mp.ver) < 0) return { parsecId: mp.ver, crudo: {}, salida: {}, ultima: "", n: mp.n, edadMs: -1 };
    const seg = ahora() - mp.desde;
    if (mp.manual && ahora() > mp.manual.hasta) mp.manual = null;
    const crudo = crudoSimulado(seg, mp.manual?.f);
    const p = pulsacionNueva(mp.antes || {}, crudo);
    if (p) { mp.ultima = p; mp.n++; }
    mp.antes = crudo;
    return { parsecId: mp.ver, crudo, salida: aplicarMapeo(mp.perfiles[mp.ver]?.perfil, crudo), ultima: mp.ultima, n: mp.n, edadMs: 20 };
  };
  try { window.simPulsar = (f) => { s.mapeo.manual = { f, hasta: ahora() + 0.6 }; }; } catch { /* sin ventana */ }

  const estado = () => {
    const lista = s.mandos.slice(0, s.xbox + s.ds4).map((id, i) => {
      const n = i + 1;
      const hostAqui = s.host === n;
      return {
        n, conectado: !s.desconectados.has(i), ocupado: !!id || hostAqui, bloqueado: s.bloqueados.has(i),
        jugador: hostAqui ? "Fralex" : id ? nombreDe(id) : "", parsecId: hostAqui ? 99 : id, ping: id ? pingDe(id) : -1,
        equipo: s.mapa[i] === "A" ? "a" : s.mapa[i] === "B" ? "b" : "fuera",
        tipo: i < s.xbox ? "xbox" : "ds4",
        // Mando en vivo (solo en Mandos › Puestos, como el motor): cada mando «pulsa» algo distinto con el tiempo
        ...(s.seccion === "mandos" && (s.pestana === "puestos" || !s.pestana) && (id || hostAqui) ? { entrada: (() => {
          const k = Math.floor(ahora() * 2 + i * 3), bits = [0x1000, 0x2000, 0x4000, 0x8000, 0x1, 0x8, 0x100, 0x10, 0];
          return { b: bits[k % bits.length], lt: (k % 5) * 60, rt: ((k + 2) % 5) * 60,
            lx: Math.round(Math.cos(ahora() + i) * 30000), ly: Math.round(Math.sin(ahora() + i) * 30000), rx: 0, ry: (k % 3 - 1) * 25000 };
        })() } : {}),
      };
    });
    const verSerie = (s.seccion === "sala" && s.pestana === "red") || s.seccion === "partido";
    const p = s.partido;
    const e = {
      v: 1,
      app: { version: "0.2.34", compilado: "11/10 04:00", idioma: s.idioma, tema: s.tema, dev: false, soda: "7.0.4" },
      noticias: { cargado: true, lista: s.noticias },
      chatGlobal: { cargado: true, pausado: s.chatGlobal.pausado, esperaSeg: s.chatGlobal.esperaSeg, rev: s.chatGlobal.mensajes.length, error: "" },
      web: { ...s.web, jugadoresLista: 3, versionLiga: "2026.10.08", eventosEnCola: 0 },
      ajustes: s.ajustes,
      buzon: s.buzon,
      entrega: s.entrega,
      pes: s.pes,
      cedido: { activo: s.cedido.activo, parsecId: s.cedido.parsecId, nombre: s.cedido.nombre, desde: s.cedido.desde, atajo: "Ctrl+Alt+F12",
        juego: s.opciones.biblioteca && s.opciones.biblioteca !== "Default" ? s.opciones.biblioteca : "", pesAbierto: !!s.pes.abierto,
        ultimoFin: s.cedido.ultimoFin },
      marionetas: s.marionetas,
      campo: { estado: s.campoVivo ? "activo" : "esperando", motivo: "", jugadores: s.campoVivo ? 24 : 0, balon: !!s.campoVivo, equipos: !!s.campoVivo, campoWeb: !!s.campoWeb, tanda: !!s.campoVivo && ahora() - (s.campoDesde || ahora()) >= 40 },
      teclado: { perfiles: s.perfilesTeclado },
      sala: {
        abierta: s.abierta, lista: true, enlace: s.abierta ? "https://parsec.gg/g/7K2Qx9abcdef/phoenix" : "", nombre: s.opciones.nombre, plazas: s.opciones.plazas,
        invitados: s.abierta ? s.invitados.length : 0, cuentaHost: "Fralex#1234", hostId: 99, hostNombre: "Fralex",
        segundos: s.abierta ? Math.round(ahora() - s.abiertaEn) : -1, opciones: s.opciones, phoenix: s.phoenix, calidad: s.calidad,
        sesion: { pico: 5, entradas: 7, partidos: s.historial.length },
      },
      mandos: { lista, miMando: s.marionetas.maestro >= 0 ? s.marionetas.maestros[s.marionetas.maestro]?.nombre || "" : "", ordenPes: s.ordenPes || { activo: false, estado: "apagado", casillas: [] }, miPuesto: s.miPuesto || (s.miPuesto = { modo: "fijo", numero: 1, alAbrir: true, pulsando: true, cuenta: true, mandoRecordado: false }), formacion: { a: eqCuenta("A"), b: eqCuenta("B"), mapa: s.mapa }, fijados: s.fijados, host: s.host, bloqueoGlobal: s.bloqueoGlobal, bloqueoBotones: s.bloqueoBotones, botonesBloq: s.botonesBloq, esclavo: false, xbox: s.xbox, ds4: s.ds4, reiniciando: false },
      mapeo: { perfiles: s.mapeo.perfiles, vista: vistaMapeo() },
      solicitudes: s.solicitudes, espera: s.espera,
      invitados: s.abierta ? s.invitados.map((g) => ({ parsecId: g.parsecId, nombre: g.nombre, ping: pingDe(g.parsecId), mando: s.mandos.indexOf(g.parsecId) + 1, mod: g.mod, vip: g.vip, teclado: g.teclado, raton: g.raton, rolWeb: g.rolWeb, cop: false, falso: false })) : [],
      perfiles,
      red: s.abierta ? resumenRed(verSerie) : [],
      retraso: s.abierta ? resumenRetraso() : [],
      partido: { fase: p.fase, a: p.a, b: p.b, segundos: segundosPartido(), inicioMs: 0, invertido: p.invertido },
      juego: s.juego,
      liga: { ...s.liga, enVivo: s.liga.elegido > 0 && p.fase === "en_juego" },
      arbitro: {
        ...s.arbitro,
        activo: s.arbitro.competitivo && s.abierta && (s.juego.datos ? s.juego.fase === "en_juego" : p.fase === "en_juego"),
        origen: s.juego.datos ? "juego" : p.fase === "en_juego" || p.fase === "pausado" ? "marcador" : "",
      },
      turnos: s.turnos,
    };
    if (s.seccion === "ajustes" && s.pestana === "audio") {
      e.audio = JSON.parse(JSON.stringify(s.audio));
      e.audio.parlantes.nivel = s.audio.parlantes.activo ? 0.35 + Math.random() * 0.4 : 0;
      e.audio.mic.nivel = s.audio.mic.activo ? Math.random() * 0.3 : 0;
    }
    if (s.seccion === "gente" || s.seccion === "partido") {
      e.amigos = { cargados: true, lista: [
        { usuarioId: "6f1c2a10-0000-4000-8000-000000000001", nombre: "Mirko", avatar: null, estado: "en_sala", salaId: null, desde: new Date(Date.now() - 600000).toISOString() },
        { usuarioId: "6f1c2a10-0000-4000-8000-000000000002", nombre: "Lucho", avatar: null, estado: "disponible", salaId: null, desde: new Date(Date.now() - 3600000).toISOString() },
        { usuarioId: "6f1c2a10-0000-4000-8000-000000000003", nombre: "Pibe", avatar: null, estado: "desconectado", salaId: null, desde: "" },
        { usuarioId: "6f1c2a10-0000-4000-8000-000000000004", nombre: "Zurdo", avatar: null, estado: "en_partida", salaId: null, desde: new Date(Date.now() - 900000).toISOString() },
      ] };
    }
    return e;
  };

  const error = (codigo, mensaje) => { const x = new Error(mensaje); x.codigo = codigo; throw x; };
  const eqCuenta = (c) => [...s.mapa].filter((x) => x === c).length;   // puestos del Equipo A ("A") o B ("B")
  const chat = (linea) => { s.chat.push(linea); enviar({ t: "evento", nombre: "chat", datos: { reinicio: false, lineas: [linea] } }); };
  const act = (linea) => { s.actividad.push(linea); enviar({ t: "evento", nombre: "actividad", datos: { reinicio: false, lineas: [linea] } }); };
  const bot = (x) => chat("[PhoenixBot] " + x);
  const globalJson = () => ({ cargado: true, pausado: s.chatGlobal.pausado, esperaSeg: s.chatGlobal.esperaSeg, rev: s.chatGlobal.mensajes.length, error: "", mensajes: s.chatGlobal.mensajes });
  const global = (nombre, texto, rol = "jugador", propio = false) => {
    const d = new Date();
    s.chatGlobal.mensajes.push({ id: s.chatGlobal.nextId++, usuarioId: propio ? "yo" : "x" + nombre, nombre, texto, hora: String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0"), rol, propio });
    enviar({ t: "evento", nombre: "chatglobal", datos: globalJson() });
  };

  // Lista de «Módulos del juego» (lo que devuelve sync.modulos en el motor real)
  const modulosSim = () => {
    const est = s.modEstadio || "no_instalado";
    const destinos = (activo, version) => [
      { nombre: "Raíz del juego", carpeta: "C:\\Juegos\\PES 2021 ConmeGOL\\SiderAddons", archivo: est !== "no_instalado", igual: true, version, linea: activo, comentada: !activo && est === "apagado" },
      { nombre: "ConmeGOL Patch 26", carpeta: "C:\\Juegos\\PES 2021 ConmeGOL\\ConmeGol Extras\\ConmeGOL Patch 26\\SiderAddons", archivo: est !== "no_instalado", igual: true, version, linea: activo, comentada: !activo && est === "apagado" },
    ];
    return {
      juego: s.buzon.juego, pesAbierto: s.pes.abierto, abiertos: [],
      parche: { modoActual: "ConmeGOL Patch 27", modosRaiz: ["ConmeGOL Clasicos", "ConmeGOL Patch 27", "UEFAGOL 27"], modosPropios: ["ConmeGOL Patch 26"], advertencias: [] },
      modulos: [
        { archivo: "phoenix_referee.lua", descripcion: "Phoenix Referee: marcador, goles con minuto y goleador, pantalla del juego para la web, HUD del partido y árbitro (solo lectura).", gestionable: true, version: "1.2", estado: est, carga: "", destinos: est === "no_instalado" ? [] : destinos(est === "instalado", "1.2") },
        { archivo: "phoenix.lua", descripcion: "Phoenix Sync: avisos de la web en el overlay, botón nativo «Datos Actual. en vivo» y modos de recarga de fichajes. Instala también la raíz Phoenix-DB y la carpeta de avisos.", gestionable: true, version: "0.18b", estado: s.modPhx || "desactualizado", carga: "cargado",
          destinos: [{ nombre: "Raíz del juego", carpeta: "a", archivo: true, igual: s.modPhx === "instalado", version: s.modPhx === "instalado" ? "0.18b" : "0.18", linea: true, comentada: false }, { nombre: "ConmeGOL Patch 26", carpeta: "b", archivo: true, igual: s.modPhx === "instalado", version: s.modPhx === "instalado" ? "0.18b" : "0.18", linea: true, comentada: false }] },
      ],
    };
  };

  const acciones = {
    "ui.seccion": (d) => { s.seccion = d.seccion; s.pestana = d.pestana || ""; },
    "ui.tema": (d) => { s.tema = d.tema; },
    "ui.idioma": (d) => { s.idioma = d.idioma; },

    "ui.copiar": (d) => { navigator.clipboard?.writeText(d.texto).catch(() => {}); }, "ui.abrir": () => ({}), "ui.recargar": () => location.reload(),
    "sala.abrir": () => { s.abierta = true; s.abiertaEn = ahora(); act("[PhoenixBot] Sala abierta."); },
    "sala.cerrar": () => { s.abierta = false; s.partido.fase = "libre"; act("[PhoenixBot] Sala cerrada."); },
    "sala.copiarEnlace": () => ({ enlace: "https://parsec.gg/g/7K2Qx9abcdef/phoenix" }),
    "sala.opciones": (d) => { if (d.quiosco && s.opciones.biblioteca === "Default") error("QUIOSCO_SIN_JUEGO", "El modo quiosco necesita un juego de la biblioteca."); Object.assign(s.opciones, d); if (s.abierta && ("nombre" in d || "plazas" in d)) s.opciones.pendiente = true; return { pendiente: s.opciones.pendiente }; },
    "sala.aplicar": () => { s.opciones.pendiente = false; },
    "sala.phoenix": (d) => { Object.assign(s.phoenix, d); },
    "sala.calidad": (d) => { s.calidad = { ...s.calidad, fps: d.fps, mbps: d.mbps }; },
    "sala.anchoAuto": (d) => { if (typeof d.auto === "boolean") s.calidad.auto = d.auto; if (d.porPersona) s.calidad.porPersona = d.porPersona; if (typeof d.subida === "number") s.calidad.subida = d.subida; if (s.calidad.auto) s.calidad.mbps = s.calidad.porPersona * Math.max(1, s.invitados?.length || 1); },
    "web.vincular": (d) => { s.web = { estado: "conectado", usuario: "Fralex", mensaje: "Vinculado con el código " + d.codigo, publicada: s.abierta }; },
    "web.reintentar": () => ({}), "web.desvincular": () => { s.web = { estado: "sin_vincular", usuario: "", mensaje: "Escribe el código de 6 dígitos de la web.", publicada: false }; },
    "web.soltarRival": () => ({ mensaje: "ok" }),
    "perfilesSala.lista": () => ({ perfiles: s.perfilesSala, actual: {} }),
    "perfilesSala.guardar": (d) => { s.perfilesSala = s.perfilesSala.filter((p) => p.nombre.toLowerCase() !== d.nombre.toLowerCase()); s.perfilesSala.push({ nombre: d.nombre, valores: { plazas: s.opciones.plazas, mbps: s.calidad.mbps, local: eqCuenta("A"), visitante: eqCuenta("B"), visibilidad: s.phoenix.visibilidad } }); return { nombre: d.nombre }; },
    "perfilesSala.aplicar": (d) => ({ nombre: d.nombre }),
    "perfilesSala.borrar": (d) => { s.perfilesSala = s.perfilesSala.filter((p) => p.nombre !== d.nombre); },
    "mandos.conectar": (d) => { s.desconectados.delete(d.indice); }, "mandos.desconectar": (d) => { s.desconectados.add(d.indice); },
    "mandos.bloquear": (d) => { s.bloqueados.has(d.indice) ? s.bloqueados.delete(d.indice) : s.bloqueados.add(d.indice); },
    "mandos.liberar": (d) => { s.mandos[d.indice] = 0; },
    "mandos.asignar": (d) => { const i = s.mandos.indexOf(d.parsecId); if (i >= 0) s.mandos[i] = 0; s.mandos[d.indice] = d.parsecId; act(`[PhoenixBot] Mando ${d.indice + 1} asignado a ${nombreDe(d.parsecId)}`); },
    "mandos.intercambiar": (d) => { const x = s.mandos[d.a]; s.mandos[d.a] = s.mandos[d.b]; s.mandos[d.b] = x; },
    // Formación por cuentas (local = puestos del Equipo A, visitante = del B): si el mapa ya las tiene se respeta; si no, A primero y luego B
    "mandos.formacion": (d) => { if (eqCuenta("A") !== d.local || eqCuenta("B") !== d.visitante) s.mapa = ("A".repeat(d.local) + "B".repeat(d.visitante)).padEnd(8, "-").slice(0, 8); },
    "mandos.fijarPuesto": (d) => { if (!d.puesto) delete s.fijados[d.parsecId]; else s.fijados[d.parsecId] = d.puesto; },
    // Equipo de un puesto (como mandos.equipo del motor): el número del puesto no cambia; cada equipo conserva al menos un puesto
    "mandos.equipo": (d) => {
      const i = d.indice, nuevo = d.equipo === "a" ? "A" : d.equipo === "b" ? "B" : d.equipo === "fuera" ? "-" : "";
      if (!nuevo) error("DATOS_INVALIDOS", "El equipo es a, b o fuera.");
      const antes = s.mapa[i];
      if (antes !== nuevo && antes !== "-" && eqCuenta(antes) <= 1) error("FUERA_DE_RANGO", "Cada equipo necesita al menos un puesto.");
      s.mapa = s.mapa.slice(0, i) + nuevo + s.mapa.slice(i + 1);
    },
    "mandos.ordenPes": (d) => { s.ordenPes = { activo: !!d.si, estado: d.si ? "ordenado" : "apagado", ultimo: d.si ? "00:15" : "", copia: d.si ? "settings.dat.phoenix-20261011-001500" : "", error: "",
      casillas: d.si ? Array.from({ length: 8 }, (_, i) => ({ n: i + 1, puesto: i + 1, ok: true })) : [] }; },
    "mandos.ordenarPes": () => { if (s.ordenPes?.activo) s.ordenPes.estado = "al_dia"; },
    "mandos.vivo": () => ({}),
    "campo.vivo": (d) => { s.campoVivo = !!d.si; s.campoDesde = ahora(); },
    "campo.web": (d) => { s.campoWeb = !!d.si; },
    "mandos.tomar": (d) => { s.host = d.numero === s.host ? 0 : d.numero; return { activo: s.host }; },
    "mandos.herramienta": (d) => { if (d.nombre === "bloquearTodo") s.bloqueoGlobal = !s.bloqueoGlobal; else if (d.nombre === "bloquearBotones") s.bloqueoBotones = !s.bloqueoBotones; else if (d.nombre === "desconectarTodos") s.mandos = s.mandos.map(() => 0); else if (d.nombre === "ordenar") { const v = s.mandos.filter(Boolean); s.mandos = s.mandos.map((_, i) => v[i] || 0); } },
    "teclado.asignar": (d) => { const p = s.perfilesTeclado.find((x) => x.userId === d.userId) || s.perfilesTeclado[0]; p.teclas.forEach((x) => { if (x.v === d.tecla) { x.v = 0; x.e = ""; } }); const k = p.teclas.find((x) => x.b === d.boton); k.v = d.tecla; k.e = "K" + d.tecla; },
    "teclado.crear": (d) => { const base = s.perfilesTeclado[0]; s.perfilesTeclado.push({ userId: d.userId, nombre: d.nombre, teclas: base.teclas.map((x) => ({ ...x })) }); },
    "teclado.reiniciar": () => {},
    "teclado.borrar": (d) => { s.perfilesTeclado = s.perfilesTeclado.filter((x) => x.userId !== d.userId); },
    "mandos.competitivo": (d) => { s.arbitro.competitivo = !!d.si; },
    "mandos.pausaAuto": (d) => { s.arbitro.pausaAuto = !!d.si; },
    "mandos.marcadorAuto": (d) => { s.arbitro.marcadorAuto = !!d.si; },
    "mandos.soloAutorizados": (d) => { s.arbitro.soloAutorizados = !!d.si; },
    "sync.modoRecarga": (d) => { s.arbitro.modoRecarga = d.modo; },
    "mandos.botonesBloq": (d) => { s.botonesBloq = { mascara: d.mascara, lt: d.lt, rt: d.rt, lx: d.lx, ly: d.ly, rx: d.rx, ry: d.ry }; },
    "marionetas.motor": (d) => { s.marionetas.motor = d.sdl ? "sdl" : "xinput"; s.marionetas.maestro = -1; },
    "marionetas.actualizar": () => {},
    "mandos.miPuesto": (d) => { const p = s.miPuesto; for (const k of ["modo", "numero", "alAbrir", "pulsando", "cuenta"]) if (k in d) p[k] = d[k]; },
    "marionetas.mostrar360": (d) => { s.marionetas.mostrar360 = !!d.si; s.marionetas.ocultos360 = d.si ? 0 : 8; },
    "marionetas.maestro": (d) => { s.marionetas.maestro = s.marionetas.maestro === d.indice ? -1 : d.indice; if (s.marionetas.maestro < 0) s.marionetas.titeres.forEach((x) => { x.activo = false; }); },
    "marionetas.titere": (d) => { s.marionetas.titeres[d.indice].activo = d.si; },
    "mandos.miMando": (d) => { s.marionetas.maestro = d.indice; s.marionetas.titeres.forEach((x) => { x.activo = false; }); },
    "marionetas.tipo": (d) => { const x = s.marionetas.maestros[d.indice]; x.tipo = x.tipo === "xbox" ? "dualshock" : x.tipo === "dualshock" ? "ds4" : "xbox"; },
    "mapeo.ver": (d) => { const id = Number(d.parsecId) || 0; if (id !== s.mapeo.ver) { s.mapeo.ver = id; s.mapeo.antes = null; s.mapeo.ultima = ""; s.mapeo.desde = ahora(); } },
    "mapeo.guardar": (d) => {
      if (!d.parsecId) error("DATOS_INVALIDOS", "Falta el ID de Parsec.");
      if (!d.perfil || typeof d.perfil !== "object") error("DATOS_INVALIDOS", "Falta el perfil de mapeo.");
      const perfil = limpiarMapeo(d.perfil), identidad = mapeoIdentidad(perfil);
      if (identidad) delete s.mapeo.perfiles[d.parsecId];
      else s.mapeo.perfiles[d.parsecId] = { nombre: d.nombre || "", actualizado: Date.now(), perfil };
      return { identidad };
    },
    "mapeo.restablecer": (d) => { delete s.mapeo.perfiles[d.parsecId]; },
    "mandos.cantidad": (d) => { if (d.xbox + d.ds4 > 8) error("FUERA_DE_RANGO", "Máximo 8 mandos en total."); s.xbox = d.xbox; s.ds4 = d.ds4; },
    "solicitud.aceptar": (d) => { s.solicitudes = s.solicitudes.filter((x) => x.parsecId !== d.parsecId); },
    "solicitud.rechazar": (d) => { s.solicitudes = s.solicitudes.filter((x) => x.parsecId !== d.parsecId); },
    "espera.decidir": (d) => { const x = s.espera.find((e) => e.parsecId === d.parsecId); s.espera = s.espera.filter((e) => e.parsecId !== d.parsecId); if (x && d.como !== "expulsar") s.invitados.push({ parsecId: x.parsecId, nombre: x.nombre, base: x.ping, mod: false, vip: false, teclado: false, raton: false, rolWeb: "no_listado" }); },
    "turnos.activar": (d) => { s.turnos.activo = d.si; s.opciones.turnos = d.si; },
    "turnos.ajustes": (d) => { Object.assign(s.turnos, d); },
    "gente.mod": (d) => { const g = s.invitados.find((x) => x.parsecId === d.parsecId); g.mod = !g.mod; return { mod: g.mod }; },
    "gente.vip": (d) => { const g = s.invitados.find((x) => x.parsecId === d.parsecId); g.vip = !g.vip; return { vip: g.vip }; },
    "gente.teclado": (d) => { s.invitados.find((x) => x.parsecId === d.parsecId).teclado = d.si; },
    "gente.raton": (d) => { s.invitados.find((x) => x.parsecId === d.parsecId).raton = d.si; },
    "gente.expulsar": (d) => { s.invitados = s.invitados.filter((x) => x.parsecId !== d.parsecId); s.mandos = s.mandos.map((x) => (x === d.parsecId ? 0 : x)); },
    "gente.banear": (d) => { s.baneados.push({ parsecId: d.parsecId, nombre: nombreDe(d.parsecId), motivo: "" }); acciones["gente.expulsar"](d); },
    "moderacion.listas": () => ({ baneados: s.baneados, mods: s.invitados.filter((g) => g.mod), vips: s.invitados.filter((g) => g.vip), historial: s.invitados.map((g) => ({ parsecId: g.parsecId, nombre: g.nombre })) }),
    "moderacion.desbanear": (d) => { s.baneados = s.baneados.filter((x) => x.parsecId !== d.parsecId); },
    "moderacion.quitarMod": (d) => { const g = s.invitados.find((x) => x.parsecId === d.parsecId); if (g) g.mod = false; },
    "moderacion.quitarVip": (d) => { const g = s.invitados.find((x) => x.parsecId === d.parsecId); if (g) g.vip = false; },
    "moderacion.banear": (d) => { s.baneados.push({ parsecId: d.parsecId, nombre: d.nombre || nombreDe(d.parsecId), motivo: "" }); },
    "moderacion.motivo": (d) => { const b = s.baneados.find((x) => x.parsecId === d.parsecId); if (b) b.motivo = d.motivo; },
    "chat.enviar": (d) => {
      chat("Fralex: " + d.texto);
      // Mandos › Mapeo: el invitado aludido «pulsa» lo que se le pidió (con su mando mal mapeado)
      const g = s.invitados.find((x) => d.texto.startsWith("@" + x.nombre + " "));
      const f = g && g.parsecId === s.mapeo.ver ? respuestaChatMapeo(d.texto) : "";
      if (f) setTimeout(() => { s.mapeo.manual = { f, hasta: ahora() + 0.6 }; }, 900);
    },
    "chatGlobal.abierto": () => ({}),
    "chatGlobal.enviar": (d) => {
      const texto = String(d.texto || "").trim();
      if (!texto || texto.length > 300) error("MENSAJE_INVALIDO", "El mensaje está vacío o pasa de 300 caracteres.");
      if (s.chatGlobal.pausado) error("CHAT_PAUSADO", "El chat general está en pausa por el staff.");
      const falta = s.chatGlobal.esperaSeg - (Date.now() - s.chatGlobal.ultimoEnvio) / 1000;
      if (falta > 0) error("ESPERA", "Espera " + Math.ceil(falta) + " s para escribir otra vez.");
      s.chatGlobal.ultimoEnvio = Date.now();
      global("Fralex", texto, "admin", true);
      setTimeout(() => global("Mirko", "jajaja bien ahí", "jugador"), 2500);
      return {};
    },
    "amigos.invitar": () => ({ mensaje: "Invitación enviada." }),
    "ajustes.modoPes": (d) => { s.pes.modo = d.valor; },
    "sync.deshacerEntrega": () => { s.entrega = { ...s.entrega, estado: "deshecha", puedeDeshacer: false, motivo: "" }; },
    // Módulos del juego (instalador del paquete de Luas): imita ModulosJuego.cpp
    "sync.modulos": () => modulosSim(),
    // Phoenix Doctor (modo informativo): como phoenix/link/Doctor.cpp
    "sync.doctor": () => ({
      modo: "informativo", generado: "2026-10-11 12:30", huella: "7c1e0b9a44d2f0a3b8e65d21c90a4e77f3b2a1c0d9e8f7a6b5c4d3e2f1a0b9c8",
      juegoEncontrado: true, juego: s.buzon.juego, parche: "ConmeGOL Patch 26", exe: { bytes: 458190848, huellaInicio: "ab12…" },
      modos: ["Raíz del juego", "Clasicos (No LM)", "ConmeGOL Patch 26", "ConmeGOL Patch 26 (Peru, Chile, Uruguay B)"],
      luaActivos: ["lib\\CommonLib.lua", "kserv.lua", "sifflet.lua", "phoenix.lua", "phoenix_referee.lua"],
      cpkRoots: [".\\livecpk\\Phoenix-DB", ".\\olmosjr23\\Database", ".\\livecpk\\Kits"],
      archivos: [
        { nombre: "sider.ini (Raíz del juego)", sha256: "f00d1e…", bytes: 9120 },
        { nombre: "modules/phoenix.lua (Raíz del juego)", sha256: "a39b54de9f6a…", bytes: 66120 },
        { nombre: "Phoenix-DB/Player.bin (Raíz del juego)", sha256: "c0ffee12ab34…", bytes: 2401120 },
        { nombre: "Phoenix-DB/PlayerAssignment.bin (Raíz del juego)", sha256: "d1cf73c6aa01…", bytes: 401220 },
        { nombre: "option file (EDIT00000000)", sha256: "cb6e176e0b11…", bytes: 1176520 },
      ],
      avisos: ["phoenix.lua no es igual en todos los modos del switcher."], pesAbierto: s.pes.abierto,
    }),
    "sync.modulo": (d) => {
      if (s.pes.abierto) throw Object.assign(new Error("Cierra PES 2021 primero: Sider lee los módulos al arrancar."), { codigo: "PES_ABIERTO" });
      const nuevo = d.accion === "instalar" ? "instalado" : d.accion === "desinstalar" ? "no_instalado" : "apagado";
      if (d.archivo === "phoenix.lua") s.modPhx = nuevo; else s.modEstadio = nuevo;
      return { ...modulosSim(), mensaje: d.accion === "instalar" ? d.archivo + " instalado en 2 carpetas. Abre PES: en sider.log debe aparecer que cargó." : d.archivo + " apagado (línea comentada en sider.ini)." };
    },
    "ajustes.avisosJuego": (d) => { s.buzon.activo = d.valor; s.buzon.estado = d.valor ? "conectado" : "apagado"; },
    "ajustes.general": (d) => { s.ajustes.general[d.clave] = d.valor; },
    "ajustes.permisos": (d) => { s.ajustes.permisos[d.grupo][d.clave] = d.valor; },
    "ajustes.video": (d) => { s.ajustes.video[d.clave] = d.valor; },
    "ajustes.videoListas": () => ({ pantallas: ["Pantalla 1 (1920×1080)", "Pantalla 2 (2560×1440)"], gpus: ["NVIDIA GeForce RTX 3060"], wgc: true }),
    "ajustes.audio": (d) => { s.audio[d.canal][d.clave] = d.valor; },
    "ajustes.overlayMenu": () => ({}),
    "biblioteca.lista": () => ({ juegos: s.biblioteca }),
    "biblioteca.guardar": (d) => { const j = s.biblioteca.find((x) => x.id === d.id); if (j) Object.assign(j, { nombre: d.nombre, ruta: d.ruta, parametros: d.parametros || "" }); else s.biblioteca.push({ id: Math.max(0, ...s.biblioteca.map((x) => x.id)) + 1, nombre: d.nombre, ruta: d.ruta, parametros: d.parametros || "" }); s.opciones.juegos = s.biblioteca.map((x) => x.nombre); },
    "biblioteca.borrar": (d) => { s.biblioteca = s.biblioteca.filter((x) => x.id !== d.id); s.opciones.juegos = s.biblioteca.map((x) => x.nombre); },
    "atajos.lista": () => ({ atajos: s.atajos, activos: true }),
    "atajos.agregar": (d) => { const n = d.tecla >= 0x70 ? "F" + (d.tecla - 0x6f) : String.fromCharCode(d.tecla); s.atajos = s.atajos.filter((x) => x.tecla !== d.tecla); s.atajos.push({ comando: d.comando, tecla: d.tecla, nombre: n }); },
    "atajos.borrar": (d) => { s.atajos.splice(d.indice, 1); },
    "ajustes.overlay": (d) => { const [a, b] = d.clave.split("."); if (b) s.ajustes.overlay[a][b] = d.valor; else s.ajustes.overlay[a] = d.valor; },
    "sfx.lista": () => ({ sonidos: s.sonidos }),
    "sfx.recargar": () => ({ sonidos: s.sonidos }),
    "sfx.espera": (d) => { const x = s.sonidos.find((y) => y.ruta === d.ruta); if (!x) error("NO_EXISTE", "Ese sonido ya no está."); x.espera = d.segundos; },
    "diag.ejecutar": () => ({ chequeos: [
      { id: "webview2", estado: "ok", datos: { version: "130.0.2849.80" } },
      { id: "vigem", estado: "ok", datos: { instalado: true, mandos: 8, conectados: 8 } },
      { id: "parsec", estado: "ok", datos: { listo: true, cuenta: true, nombre: "Fralex" } },
      { id: "video", estado: "ok", datos: { gpus: 1, pantallas: 2, wgc: true } },
      { id: "audio", estado: "ok", datos: { entradas: 1, salidas: 2 } },
      { id: "config", estado: "ok", datos: { ruta: "C:\\Users\\…\\AppData\\Roaming\\Trybuchet\\Smash Soda\\" } },
      { id: "web", estado: "ok", datos: { usuario: "Fralex", mensaje: "Conectado", eventosEnCola: 0 } },
      { id: "overlay", estado: "ok", datos: { activo: true, websocket: true, puerto: 9002 } },
      { id: "sistema", estado: "ok", datos: { windows: "Windows 11 (compilación 22631)", app: "1.0.0", soda: "7.0.4" } },
    ] }),
    "partido.preparar": (d) => {
      const lado = (ids, nombre) => ({ nombre, jugadores: ids.map((id) => ({ parsecId: id, nombre: nombreDe(id) })), goles: 0 });
      // Contra la máquina (CPU): ese lado lleva un solo «jugador» con parsecId 0 (como AccionesPartidoWeb.cpp)
      const cpu = (nombre) => ({ nombre: nombre || "Máquina (CPU)", jugadores: [{ parsecId: 0, nombre: "Máquina" }], goles: 0 });
      s.partido = { ...s.partido, fase: "listo", a: d.cpu === "a" ? cpu(d.nombreA) : lado(d.a, d.nombreA), b: d.cpu === "b" ? cpu(d.nombreB) : lado(d.b, d.nombreB), acumulado: 0, invertido: false };
    },
    "partido.iniciar": () => { s.partido.fase = "en_juego"; s.partido.inicio = ahora(); bot(`Partido: ${s.partido.a.nombre} vs ${s.partido.b.nombre}. ¡Suerte!`); },
    "partido.gol": (d) => { const l = s.partido[d.lado]; l.goles = Math.max(0, Math.min(99, l.goles + d.delta)); },
    "partido.pausa": (d) => { const p = s.partido; if (d.si && p.fase === "en_juego") { p.acumulado += ahora() - p.inicio; p.fase = "pausado"; } else if (!d.si && p.fase === "pausado") { p.inicio = ahora(); p.fase = "en_juego"; } },
    "partido.cambiarLados": () => { s.partido.invertido = !s.partido.invertido; },
    "partido.corregirLados": () => { s.partido.invertido = !s.partido.invertido; bot("Lados corregidos."); },
    "partido.ladosAuto": (d) => { s.arbitro.ladosAuto = !!d.si; },
    "partido.finalizar": () => {
      const p = s.partido;
      const reg = { id: siguienteId++, inicioMs: Date.now() - segundosPartido() * 1000, duracionSeg: segundosPartido(), a: p.a, b: p.b, ganador: p.a.goles > p.b.goles ? "a" : p.b.goles > p.a.goles ? "b" : "empate" };
      s.historial.unshift(reg);
      s.partido = { fase: "libre", a: { nombre: "", jugadores: [], goles: 0 }, b: { nombre: "", jugadores: [], goles: 0 }, inicio: 0, acumulado: 0, invertido: false };
      bot(`Final: ${reg.a.nombre} ${reg.a.goles}-${reg.b.goles} ${reg.b.nombre}`);
      return { registro: reg, guardado: true };
    },
    "partido.cancelar": () => { s.partido.fase = "libre"; },
    "partido.historial": () => ({ partidos: s.historial }),
    // Liga automática (como AccionesPartidoWeb.cpp + función «liga»)
    "partido.oficiales": () => { s.liga.cargado = true; },
    "partido.oficial": (d) => {
      const p = s.liga.partidos.find((x) => x.id === d.id);
      s.liga.elegido = p ? p.id : 0;
      s.liga.elegidoTexto = p ? `Fecha ${p.fecha} · ${p.local} vs ${p.visitante}` : "";
      return { texto: s.liga.elegidoTexto };
    },
    "partido.clasificatoria": (d) => {
      if (typeof d.si === "boolean") s.clasificatoria = d.si;
      const g = s.invitados.filter((x) => !x.falso);
      return { activa: !!s.clasificatoria, pcVinculada: true, sinCuenta: g.filter((x) => !perfiles[x.parsecId]).map((x) => ({ parsecId: x.parsecId, nombre: x.nombre })), sinComprobar: [] };
    },
    // ---- Modo cedido (mismas reglas que phoenix/web/AccionesCedidoWeb.cpp)
    "cedido.empezar": (d) => {
      if (s.cedido.activo) error("YA_CEDIDA", "La PC ya está cedida.");
      if (!s.abierta) error("SALA_CERRADA", "Abre la sala primero.");
      const g = s.invitados.find((x) => x.parsecId === d.parsecId);
      if (!g) error("NO_ESTA", "Esa persona no está en la sala.");
      Object.assign(s.cedido, { activo: true, parsecId: g.parsecId, nombre: g.nombre, desde: Date.now(), ultimoFin: "" });
      bot(`Fralex cedió su PC a ${g.nombre}: maneja Phoenix Link y PES con teclado y ratón. Nada más de Windows.`);
    },
    "cedido.salir": () => { if (s.cedido.activo) finCedido("se devolvió desde Link"); },
    "cedido.abrirPes": () => { s.pes.abierto = true; },
  };
  function finCedido(motivo) {
    const g = s.invitados.find((x) => x.parsecId === s.cedido.parsecId);
    if (g) { g.teclado = false; g.raton = false; }
    bot(`La PC vuelve al anfitrión (${motivo}). ${s.cedido.nombre} ya no tiene teclado ni ratón.`);
    Object.assign(s.cedido, { activo: false, parsecId: 0, nombre: "", desde: 0, ultimoFin: motivo });
  }
  // Portero: con la PC cedida, solo lo de jugar (copia de phoenix/core/ModoCedido.cpp)
  const CEDIDO_PERMITIDAS = new Set(["ui.seccion", "ui.copiar", "ui.recargar", "sala.copiarEnlace", "chat.enviar", "chatGlobal.abierto",
    "mandos.conectar", "mandos.desconectar", "mandos.bloquear", "mandos.liberar", "mandos.asignar", "mandos.intercambiar", "mandos.formacion", "mandos.equipo", "mandos.fijarPuesto",
    "mandos.tomar", "mandos.herramienta", "solicitud.aceptar", "solicitud.rechazar", "partido.preparar", "partido.iniciar", "partido.gol",
    "partido.pausa", "partido.cambiarLados", "partido.finalizar", "partido.cancelar", "partido.historial", "cedido.salir", "cedido.abrirPes"]);
  const cedidoPermite = (a, d) => a === "espera.decidir" ? (d.como === "jugador" || d.como === "espectador") : CEDIDO_PERMITIDAS.has(a);
  // Vista previa: Ctrl+Alt+F12 en el navegador simula el atajo físico del anfitrión
  window.addEventListener("keydown", (e) => { if (e.ctrlKey && e.altKey && e.key === "F12" && s.cedido.activo) finCedido("el anfitrión pulsó Ctrl+Alt+F12"); });

  // Estado cada 200 ms (como el motor real) y una muestra de red por segundo
  setInterval(() => enviar({ t: "estado", datos: estado() }), 200);
  setInterval(tick, 1000);

  // Campo 2D en vivo (como el motor: solo si la tarjeta lo pide con campo.vivo). 4-4-2 vs 4-3-3 que se mueven con el balón.
  const FORM_L = [[-48, 0], [-36, -22], [-38, -8], [-38, 8], [-36, 22], [-24, -24], [-26, -8], [-26, 8], [-24, 24], [-12, -10], [-12, 10]];
  const FORM_V = [[48, 0], [36, -22], [38, -8], [38, 8], [36, 22], [26, -16], [24, 0], [26, 16], [14, -22], [12, 0], [14, 22]];
  const campoSim = () => {
    const tt = ahora();
    const bx = Math.sin(tt * 0.35) * 34, bz = Math.sin(tt * 0.9 + 1) * 20;      // balón: va y viene por el campo
    const mover = (f, lado, i) => {
      const [x, z] = f[i], desp = i === 0 ? 0.15 : 0.45;                       // el portero apenas se mueve
      const px = x + (bx - x) * desp + Math.sin(tt * 0.8 + i * 1.7) * 2.2;
      const pz = z + (bz - z) * desp * 0.7 + Math.cos(tt * 0.7 + i * 2.3) * 2.2;
      return [Math.round(px * 10) / 10, Math.round(pz * 10) / 10, lado];
    };
    const j = [...FORM_L.map((_, i) => mover(FORM_L, 0, i)), ...FORM_V.map((_, i) => mover(FORM_V, 1, i))];
    j.push([Math.round((bx * 0.5) * 10) / 10, 35.2, 2], [Math.round((-bx * 0.3) * 10) / 10, -35.2, 2]); // los dos asistentes en la banda
    return { j, b: [Math.round(bx * 10) / 10, Math.round(bz * 10) / 10, 0.11] };
  };
  // Tanda de penales simulada: tras 40 s con el campo abierto, un lanzamiento cada 9 s (alternando equipos; 1 de cada 4 falla)
  const tandaSim = () => {
    const seg = ahora() - (s.campoDesde || ahora());
    if (seg < 40) return null;
    const n = Math.floor((seg - 40) / 9), fase = (seg - 40) % 9;            // n = lanzamientos ya hechos
    const l = []; for (let i = 0; i < Math.min(n, 10); i++) l.push([i % 2, i % 4 !== 3]);
    const eq = n % 2;
    return { l, e: eq, listo: fase < 6, vuelo: fase >= 6 && fase < 7.5 };
  };
  setInterval(() => { if (s.campoVivo) { const d = campoSim(); if (Math.floor(Date.now() / 4000) % 8 === 7) d.f = "repeticion"; d.c = ["#75aadb", "#f6d90f"]; const tt = tandaSim(); if (tt) d.t = tt; enviar({ t: "evento", nombre: "campo", datos: d }); } }, 50);
  setTimeout(() => global("Kaiser", "¿alguien para unas revanchas?", "jugador"), 11000);

  return {
    recibir(msg) {
      if (msg.t === "hola") {
        enviar({ t: "bienvenida", datos: {
          protocolo: 1, idiomas: [{ codigo: "es", nombre: "Español" }, { codigo: "en", nombre: "English" }],
          resoluciones: ["Escritorio", "1920×1080", "1600×900", "1280×720"], chat: s.chat, actividad: s.actividad,
          pantallas: ["Pantalla 1 (1920×1080)", "Pantalla 2 (2560×1440)"], gpus: ["NVIDIA GeForce RTX 3060"], wgc: true,
          temasOverlay: ["phoenix", "minimal"], estado: estado(), chatGlobal: globalJson(),
        } });
        return;
      }
      if (msg.t !== "pedir") return;
      const fn = acciones[msg.accion];
      try {
        if (!fn) error("ACCION_DESCONOCIDA", `La acción «${msg.accion}» no existe.`);
        if (s.cedido.activo && !cedidoPermite(msg.accion, msg.datos || {})) error("PC_CEDIDA", "La PC está cedida: eso solo lo puede hacer el anfitrión.");
        const r = fn(msg.datos || {});
        setTimeout(() => enviar({ t: "resp", id: msg.id, ok: true, datos: r || {} }), 60 + Math.random() * 120);
      } catch (e) {
        enviar({ t: "resp", id: msg.id, ok: false, error: { codigo: e.codigo || "ERROR_INTERNO", mensaje: e.message } });
      }
    },
  };
}
