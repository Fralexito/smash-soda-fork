// =============================================================================
//  Motor simulado (solo vista previa en un navegador normal, nunca dentro de
//  Phoenix Link). Implementa las mismas acciones y forma de estado que el C++
//  para poder revisar el diseño y probar la interfaz sin Windows.
// =============================================================================

export function crearSimulador(enviar) {
  const ahora = () => Date.now() / 1000;
  const inicio = ahora();
  const s = {
    tema: "galaxy", idioma: "es", abierta: true, abiertaEn: inicio - 6128,
    opciones: { nombre: "Phoenix · Galaxy League", plazas: 4, limitador: false, limite: 30, biblioteca: "Default", juegos: ["PES 2021", "Football Life 2026"], pendiente: false, turnos: false, quiosco: false, overlay: true },
    phoenix: { visibilidad: "amigos", espectadores: true, limiteEspectadores: 4, entradaParsec: true, juego: "eFootball PES 2021", parche: "Conmegol", region: "Lima" },
    calidad: { fps: 60, mbps: 15 },
    formacion: { local: 2, visitante: 2 },
    xbox: 8, ds4: 0, bloqueoGlobal: false, bloqueoBotones: false, host: 0,
    invitados: [
      { parsecId: 1187, nombre: "Mirko", base: 18, mod: true, vip: false, teclado: false, raton: false, rolWeb: "jugador" },
      { parsecId: 2041, nombre: "Kaiser", base: 27, mod: false, vip: true, teclado: false, raton: false, rolWeb: "jugador" },
      { parsecId: 3310, nombre: "ElTigre", base: 58, mod: false, vip: false, teclado: true, raton: false, rolWeb: "jugador" },
      { parsecId: 5126, nombre: "Zurdo10", base: 41, mod: false, vip: false, teclado: false, raton: false, rolWeb: "espectador" },
    ],
    mandos: [1187, 2041, 3310, 0, 0, 0, 0, 0], bloqueados: new Set(), desconectados: new Set(),
    solicitudes: [], espera: [{ parsecId: 7777, nombre: "Visitante99", ping: 66 }],
    series: {}, partido: { fase: "libre", a: { nombre: "", jugadores: [], goles: 0 }, b: { nombre: "", jugadores: [], goles: 0 }, inicio: 0, acumulado: 0, invertido: false },
    historial: [
      { id: 1, inicioMs: Date.now() - 86400000, duracionSeg: 1210, a: { nombre: "Mirko", jugadores: [{ parsecId: 1187, nombre: "Mirko" }], goles: 3 }, b: { nombre: "Kaiser", jugadores: [{ parsecId: 2041, nombre: "Kaiser" }], goles: 1 }, ganador: "a" },
      { id: 2, inicioMs: Date.now() - 3600000, duracionSeg: 1180, a: { nombre: "ElTigre", jugadores: [{ parsecId: 3310, nombre: "ElTigre" }], goles: 2 }, b: { nombre: "Mirko", jugadores: [{ parsecId: 1187, nombre: "Mirko" }], goles: 2 }, ganador: "empate" },
    ],
    perfilesSala: [{ nombre: "Liga", valores: { plazas: 4, mbps: 15, local: 2, visitante: 2, visibilidad: "amigos" } }],
    ajustes: {
      general: { flashWindow: true, ttsEnabled: false, bonkEnabled: true, messageNotification: true, disableGuideButton: true, disableKeyboard: false, autoIndex: false, parsecLogs: false, ipBan: true, blockVPN: false, devMode: false, chatbot: "PhoenixBot", discord: "https://discord.gg/phoenix", welcomeMessage: "¡Bienvenido _PLAYER_! Respeta los turnos y diviértete.", socketEnabled: true, socketPort: 9002, socketActivo: true },
      permisos: { guest: { useBB: false, useSFX: true, changeControls: true }, vip: { useBB: true, useSFX: true, changeControls: true }, moderator: { useBB: true, useSFX: true, changeControls: true } },
      video: { monitor: 0, gpu: 0, captura: 0, resolucion: 0, lanczos: false, ritmo: true, fps: 60, mbps: 15 },
      overlay: { monitor: 0, tema: "", chat: { activo: true, historial: true, posicion: "top Left" }, mandos: { activo: true, posicion: "bottom center" }, invitados: { activo: true, latencia: true, posicion: "top right" }, corriendo: true },
    },
    audio: { mic: { activo: false, volumen: 80, dispositivo: 0, dispositivos: ["Micrófono (USB)"], mic: true }, parlantes: { activo: true, volumen: 100, dispositivo: 0, dispositivos: ["Altavoces (Realtek)", "Auriculares"], mic: false } },
    turnos: { activo: false, corriendo: false, usuarios: [], juegoMin: 10, reinicioMin: 30, recordatorioMin: 2 },
    baneados: [{ parsecId: 6666, nombre: "Troll", motivo: "Insultos" }],
    web: { estado: "conectado", usuario: "Fralex", mensaje: "Sala publicada en la web.", publicada: true },
    seccion: "sala", pestana: "resumen",
    chat: ["Mirko: buenas!", "Kaiser: listos para la revancha", "[PhoenixBot] ElTigre entró a la sala."],
    actividad: ["[PhoenixBot] Sala abierta con eFootball PES 2021", "[PhoenixBot] Mirko joined.", "[PhoenixBot] Kaiser joined.", "[PhoenixBot] Mando 03 asignado a ElTigre"],
  };
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

  const estado = () => {
    const lista = s.mandos.slice(0, s.xbox + s.ds4).map((id, i) => {
      const n = i + 1;
      const hostAqui = s.host === n;
      return {
        n, conectado: !s.desconectados.has(i), ocupado: !!id || hostAqui, bloqueado: s.bloqueados.has(i),
        jugador: hostAqui ? "Fralex" : id ? nombreDe(id) : "", parsecId: hostAqui ? 99 : id, ping: id ? pingDe(id) : -1,
        equipo: n <= s.formacion.local ? "local" : n <= s.formacion.local + s.formacion.visitante ? "visitante" : "fuera",
        tipo: i < s.xbox ? "xbox" : "ds4",
      };
    });
    const verSerie = (s.seccion === "sala" && s.pestana === "red") || s.seccion === "partido";
    const p = s.partido;
    const e = {
      v: 1,
      app: { version: "1.0.0", idioma: s.idioma, tema: s.tema, dev: false, soda: "7.0.4" },
      web: { ...s.web, jugadoresLista: 3, versionLiga: "2026.10.08", eventosEnCola: 0 },
      ajustes: s.ajustes,
      sala: {
        abierta: s.abierta, lista: true, enlace: s.abierta ? "https://parsec.gg/g/7K2Qx9abcdef/phoenix" : "", nombre: s.opciones.nombre, plazas: s.opciones.plazas,
        invitados: s.abierta ? s.invitados.length : 0, cuentaHost: "Fralex#1234", hostId: 99, hostNombre: "Fralex",
        segundos: s.abierta ? Math.round(ahora() - s.abiertaEn) : -1, opciones: s.opciones, phoenix: s.phoenix, calidad: s.calidad,
        sesion: { pico: 5, entradas: 7, partidos: s.historial.length },
      },
      mandos: { lista, formacion: s.formacion, host: s.host, bloqueoGlobal: s.bloqueoGlobal, bloqueoBotones: s.bloqueoBotones, esclavo: false, xbox: s.xbox, ds4: s.ds4, reiniciando: false },
      solicitudes: s.solicitudes, espera: s.espera,
      invitados: s.abierta ? s.invitados.map((g) => ({ parsecId: g.parsecId, nombre: g.nombre, ping: pingDe(g.parsecId), mando: s.mandos.indexOf(g.parsecId) + 1, mod: g.mod, vip: g.vip, teclado: g.teclado, raton: g.raton, rolWeb: g.rolWeb, cop: false, falso: false })) : [],
      perfiles,
      red: s.abierta ? resumenRed(verSerie) : [],
      partido: { fase: p.fase, a: p.a, b: p.b, segundos: segundosPartido(), inicioMs: 0, invertido: p.invertido },
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
      ] };
    }
    return e;
  };

  const error = (codigo, mensaje) => { const x = new Error(mensaje); x.codigo = codigo; throw x; };
  const chat = (linea) => { s.chat.push(linea); enviar({ t: "evento", nombre: "chat", datos: { reinicio: false, lineas: [linea] } }); };
  const act = (linea) => { s.actividad.push(linea); enviar({ t: "evento", nombre: "actividad", datos: { reinicio: false, lineas: [linea] } }); };
  const bot = (x) => chat("[PhoenixBot] " + x);

  const acciones = {
    "ui.seccion": (d) => { s.seccion = d.seccion; s.pestana = d.pestana || ""; },
    "ui.tema": (d) => { s.tema = d.tema; },
    "ui.idioma": (d) => { s.idioma = d.idioma; },
    "ui.interfaz": () => ({}), "ui.panelClasico": () => error("SOLO_APP", "El panel clásico existe solo dentro de Phoenix Link."),
    "ui.copiar": (d) => { navigator.clipboard?.writeText(d.texto).catch(() => {}); }, "ui.abrir": () => ({}), "ui.recargar": () => location.reload(),
    "sala.abrir": () => { s.abierta = true; s.abiertaEn = ahora(); act("[PhoenixBot] Sala abierta."); },
    "sala.cerrar": () => { s.abierta = false; s.partido.fase = "libre"; act("[PhoenixBot] Sala cerrada."); },
    "sala.copiarEnlace": () => ({ enlace: "https://parsec.gg/g/7K2Qx9abcdef/phoenix" }),
    "sala.opciones": (d) => { if (d.quiosco && s.opciones.biblioteca === "Default") error("QUIOSCO_SIN_JUEGO", "El modo quiosco necesita un juego de la biblioteca."); Object.assign(s.opciones, d); if (s.abierta && ("nombre" in d || "plazas" in d)) s.opciones.pendiente = true; return { pendiente: s.opciones.pendiente }; },
    "sala.aplicar": () => { s.opciones.pendiente = false; },
    "sala.phoenix": (d) => { Object.assign(s.phoenix, d); },
    "sala.calidad": (d) => { s.calidad = { fps: d.fps, mbps: d.mbps }; },
    "web.vincular": (d) => { s.web = { estado: "conectado", usuario: "Fralex", mensaje: "Vinculado con el código " + d.codigo, publicada: s.abierta }; },
    "web.reintentar": () => ({}), "web.desvincular": () => { s.web = { estado: "sin_vincular", usuario: "", mensaje: "Escribe el código de 6 dígitos de la web.", publicada: false }; },
    "web.soltarRival": () => ({ mensaje: "ok" }),
    "perfilesSala.lista": () => ({ perfiles: s.perfilesSala, actual: {} }),
    "perfilesSala.guardar": (d) => { s.perfilesSala = s.perfilesSala.filter((p) => p.nombre.toLowerCase() !== d.nombre.toLowerCase()); s.perfilesSala.push({ nombre: d.nombre, valores: { plazas: s.opciones.plazas, mbps: s.calidad.mbps, local: s.formacion.local, visitante: s.formacion.visitante, visibilidad: s.phoenix.visibilidad } }); return { nombre: d.nombre }; },
    "perfilesSala.aplicar": (d) => ({ nombre: d.nombre }),
    "perfilesSala.borrar": (d) => { s.perfilesSala = s.perfilesSala.filter((p) => p.nombre !== d.nombre); },
    "mandos.conectar": (d) => { s.desconectados.delete(d.indice); }, "mandos.desconectar": (d) => { s.desconectados.add(d.indice); },
    "mandos.bloquear": (d) => { s.bloqueados.has(d.indice) ? s.bloqueados.delete(d.indice) : s.bloqueados.add(d.indice); },
    "mandos.liberar": (d) => { s.mandos[d.indice] = 0; },
    "mandos.asignar": (d) => { const i = s.mandos.indexOf(d.parsecId); if (i >= 0) s.mandos[i] = 0; s.mandos[d.indice] = d.parsecId; act(`[PhoenixBot] Mando ${d.indice + 1} asignado a ${nombreDe(d.parsecId)}`); },
    "mandos.intercambiar": (d) => { const x = s.mandos[d.a]; s.mandos[d.a] = s.mandos[d.b]; s.mandos[d.b] = x; },
    "mandos.formacion": (d) => { s.formacion = { local: d.local, visitante: d.visitante }; },
    "mandos.tomar": (d) => { s.host = d.numero === s.host ? 0 : d.numero; return { activo: s.host }; },
    "mandos.herramienta": (d) => { if (d.nombre === "bloquearTodo") s.bloqueoGlobal = !s.bloqueoGlobal; else if (d.nombre === "bloquearBotones") s.bloqueoBotones = !s.bloqueoBotones; else if (d.nombre === "desconectarTodos") s.mandos = s.mandos.map(() => 0); else if (d.nombre === "ordenar") { const v = s.mandos.filter(Boolean); s.mandos = s.mandos.map((_, i) => v[i] || 0); } },
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
    "chat.enviar": (d) => { chat("Fralex: " + d.texto); },
    "amigos.invitar": () => ({ mensaje: "Invitación enviada." }),
    "ajustes.general": (d) => { s.ajustes.general[d.clave] = d.valor; },
    "ajustes.permisos": (d) => { s.ajustes.permisos[d.grupo][d.clave] = d.valor; },
    "ajustes.video": (d) => { s.ajustes.video[d.clave] = d.valor; },
    "ajustes.videoListas": () => ({ pantallas: ["Pantalla 1 (1920×1080)", "Pantalla 2 (2560×1440)"], gpus: ["NVIDIA GeForce RTX 3060"], wgc: true }),
    "ajustes.audio": (d) => { s.audio[d.canal][d.clave] = d.valor; },
    "ajustes.overlay": (d) => { const [a, b] = d.clave.split("."); if (b) s.ajustes.overlay[a][b] = d.valor; else s.ajustes.overlay[a] = d.valor; },
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
      s.partido = { ...s.partido, fase: "listo", a: lado(d.a, d.nombreA), b: lado(d.b, d.nombreB), acumulado: 0, invertido: false };
    },
    "partido.iniciar": () => { s.partido.fase = "en_juego"; s.partido.inicio = ahora(); bot(`Partido: ${s.partido.a.nombre} vs ${s.partido.b.nombre}. ¡Suerte!`); },
    "partido.gol": (d) => { const l = s.partido[d.lado]; l.goles = Math.max(0, Math.min(99, l.goles + d.delta)); },
    "partido.pausa": (d) => { const p = s.partido; if (d.si && p.fase === "en_juego") { p.acumulado += ahora() - p.inicio; p.fase = "pausado"; } else if (!d.si && p.fase === "pausado") { p.inicio = ahora(); p.fase = "en_juego"; } },
    "partido.cambiarLados": () => { s.partido.invertido = !s.partido.invertido; },
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
  };

  // Estado cada 200 ms (como el motor real) y una muestra de red por segundo
  setInterval(() => enviar({ t: "estado", datos: estado() }), 200);
  setInterval(tick, 1000);
  setTimeout(() => { s.solicitudes.push({ parsecId: 2041, nombre: "Kaiser", mando: 4 }); }, 9000);

  return {
    recibir(msg) {
      if (msg.t === "hola") {
        enviar({ t: "bienvenida", datos: {
          protocolo: 1, idiomas: [{ codigo: "es", nombre: "Español" }, { codigo: "en", nombre: "English" }],
          resoluciones: ["Escritorio", "1920×1080", "1600×900", "1280×720"], chat: s.chat, actividad: s.actividad,
          pantallas: ["Pantalla 1 (1920×1080)", "Pantalla 2 (2560×1440)"], gpus: ["NVIDIA GeForce RTX 3060"], wgc: true,
          temasOverlay: ["phoenix", "minimal"], estado: estado(),
        } });
        return;
      }
      if (msg.t !== "pedir") return;
      const fn = acciones[msg.accion];
      try {
        if (!fn) error("ACCION_DESCONOCIDA", `La acción «${msg.accion}» no existe.`);
        const r = fn(msg.datos || {});
        setTimeout(() => enviar({ t: "resp", id: msg.id, ok: true, datos: r || {} }), 60 + Math.random() * 120);
      } catch (e) {
        enviar({ t: "resp", id: msg.id, ok: false, error: { codigo: e.codigo || "ERROR_INTERNO", mensaje: e.message } });
      }
    },
  };
}
