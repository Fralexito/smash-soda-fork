#!/usr/bin/env node
// =============================================================================
//  Phoenix Evolution · MCP de PES 2021 (servidor MCP por stdio, sin dependencias)
//  Deja que Claude (Claude Code / Claude Desktop en la PC de Fralex) mire y maneje PES 2021 en vivo.
//    LECTURA (siempre):   pes_estado · pes_partido · pes_eventos · pes_salud · pes_log · pes_captura ·
//                         pes_memoria_leer · pes_memoria_firma · pes_modulos · pes_escenas ·
//                         pes_hallazgos_buscar · pes_herramienta · pes_core (estado/versiones)
//    ACCIONES (apagadas): pes_tecla · pes_navegar · pes_recargar_modulos · pes_core (instalar/revertir/
//                         funcion/apagar/encender)                         → config.json "acciones": true
//    MEMORIA (apagada):   pes_memoria_escribir · pes_memoria_deshacer      → config.json "escribir_memoria": true
//  Nunca toca sider.ini ni abre o cierra PES.
//  Fuentes: content\phoenix\sonda.json (phoenix_sonda.lua), estado.json / pantalla.json / marcador_free.json
//  (phoenix_referee.lua y otros), sala.txt (Phoenix Link), sider.log, phoenix_core\* (Phoenix Core) y
//  ayudante.ps1 (Windows: proceso, memoria, foto, teclas, registro de fallos).
//  Uso: node servidor.mjs   (lo arranca Claude solo; ver LEEME.md)   · Prueba: node servidor.mjs --prueba
// =============================================================================
import { readFileSync, writeFileSync, appendFileSync, existsSync, statSync, openSync, readSync, closeSync, readdirSync, mkdtempSync, mkdirSync, rmSync, copyFileSync, unlinkSync } from 'node:fs';
import { join, dirname, basename, extname, isAbsolute } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFile } from 'node:child_process';
import { tmpdir, homedir } from 'node:os';
import { createInterface } from 'node:readline';

const VERSION = '0.2.0';
const AQUI = dirname(fileURLToPath(import.meta.url));

// ─── configuración ───────────────────────────────────────────────────────────
const POR_DEFECTO = {
  juego: 'D:\\Frank\\Games_\\Conmegol Patch',
  investigacion: 'D:\\Frank\\Games_\\PRUEBA_CANCHA',
  documentos: ['D:\\Frank\\Games_\\PRUEBA_CANCHA\\informes'],
  acciones: false,                  // teclas, navegar, Shift+R y cambiar archivos de Phoenix Core
  acciones_con_sala_abierta: false, // con invitados en la sala de Parsec, nunca se pulsa ni se escribe nada
  escribir_memoria: false,          // pes_memoria_escribir / pes_memoria_deshacer
  powershell: 'powershell.exe',
};
function cargarConfig() {
  const ruta = process.env.PES_MCP_CONFIG || join(AQUI, 'config.json');
  let propia = {};
  if (existsSync(ruta)) propia = JSON.parse(readFileSync(ruta, 'utf8').replace(/^\uFEFF/, ''));
  return { ...POR_DEFECTO, ...propia };
}
const cfg = cargarConfig();
const siderDir = () => join(cfg.juego, 'SiderAddons');
const phoenixDir = () => join(siderDir(), 'content', 'phoenix');
const coreDir = () => join(siderDir(), 'phoenix_core');

// ─── utilidades ──────────────────────────────────────────────────────────────
const edadSeg = (ruta) => Math.round((Date.now() - statSync(ruta).mtimeMs) / 1000);
function leerJson(ruta) {
  if (!existsSync(ruta)) return { existe: false };
  const edad = edadSeg(ruta);
  try { return { existe: true, edad_seg: edad, datos: JSON.parse(readFileSync(ruta, 'utf8').replace(/^\uFEFF/, '')) }; }
  catch { return { existe: true, edad_seg: edad, error: 'a medio escribir (vuelve a pedirlo en 1 s)' }; }
}
function leerClaves(ruta) {
  if (!existsSync(ruta)) return { existe: false };
  const datos = {};
  for (const l of readFileSync(ruta, 'utf8').split(/\r?\n/)) {
    const i = l.indexOf('=');
    if (i > 0) { const k = l.slice(0, i); datos[k] = k in datos ? [].concat(datos[k], l.slice(i + 1)) : l.slice(i + 1); }
  }
  return { existe: true, edad_seg: edadSeg(ruta), datos };
}
// Últimos `max` bytes de un archivo (los registros pesan decenas de MB). Se descarta la primera línea, que puede venir cortada.
function cola(ruta, max = 512 * 1024) {
  const tam = statSync(ruta).size;
  const n = Math.min(tam, max);
  const b = Buffer.alloc(n);
  const fd = openSync(ruta, 'r');
  try { readSync(fd, b, 0, n, tam - n); } finally { closeSync(fd); }
  const t = b.toString('utf8');
  return n < tam ? t.slice(t.indexOf('\n') + 1) : t;
}
const lineas = (ruta, max) => (existsSync(ruta) ? cola(ruta, max).split(/\r?\n/).filter(Boolean) : []);
function ejecutar(archivo, args, { timeout = 60000, cwd } = {}) {
  return new Promise((res) => {
    execFile(archivo, args, { timeout, cwd, windowsHide: true, maxBuffer: 16 * 1024 * 1024, encoding: 'utf8' },
      (err, stdout, stderr) => res({ ok: !err, codigo: err?.code ?? 0, stdout: stdout || '', stderr: stderr || '', error: err?.message }));
  });
}
async function ayudante(accion, extra = [], timeout = 30000) {
  if (process.platform !== 'win32') throw new Error('El ayudante solo funciona en Windows (en la PC de PES).');
  const r = await ejecutar(cfg.powershell, ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', join(AQUI, 'ayudante.ps1'), '-Accion', accion, ...extra], { timeout });
  const linea = r.stdout.trim().split(/\r?\n/).pop() || '';
  if (!r.ok || !linea.startsWith('{')) throw new Error((r.stderr || r.error || r.stdout || 'sin respuesta').trim().slice(0, 800));
  return JSON.parse(linea);
}
const texto = (o) => ({ content: [{ type: 'text', text: typeof o === 'string' ? o : JSON.stringify(o, null, 1) }] });
const espera = (ms) => new Promise((r) => setTimeout(r, ms));
function pick(o, claves) { const r = {}; for (const k of claves) if (o[k] !== undefined) r[k] = o[k]; return r; }
const comoLista = (x) => (Array.isArray(x) ? x : x && typeof x === 'object' ? Object.values(x) : []);
const sello = () => isoLocal(Date.now()).replace(/[-:]/g, '').replace(' ', '-').slice(0, 15);
// Hora local «AAAA-MM-DD hh:mm:ss.mmm» (registros de Core) → milisegundos.
const msLocal = (s) => new Date(s.replace(' ', 'T')).getTime();
// «hh:mm:ss» de hoy (peticiones.log no trae fecha); si queda en el futuro, era de ayer.
function msHoy(hms) {
  const [h, m, s] = hms.split(':').map(Number);
  const d = new Date(); d.setHours(h, m, s, 0);
  if (d.getTime() - Date.now() > 60000) d.setDate(d.getDate() - 1);
  return d.getTime();
}
const isoLocal = (ms) => { const d = new Date(ms - new Date(ms).getTimezoneOffset() * 60000); return d.toISOString().slice(0, 23).replace('T', ' '); };

// Lee el hex del ayudante en el formato pedido.
function interpretar(hex, formato) {
  const b = Buffer.from(hex, 'hex');
  const lista = (paso, f) => { const r = []; for (let i = 0; i + paso <= b.length; i += paso) r.push(f(i)); return r; };
  switch (formato) {
    case 'u8': return [...b];
    case 'u16': return lista(2, (i) => b.readUInt16LE(i));
    case 'u32': return lista(4, (i) => b.readUInt32LE(i));
    case 'i32': return lista(4, (i) => b.readInt32LE(i));
    case 'f32': return lista(4, (i) => +b.readFloatLE(i).toPrecision(7));
    case 'f64': return lista(8, (i) => b.readDoubleLE(i));
    case 'ptr': return lista(8, (i) => '0x' + b.readBigUInt64LE(i).toString(16).toUpperCase());
    case 'texto': return b.toString('latin1').replace(/[^\x20-\x7e\xa0-\xff]/g, '·');
    default: { // volcado hex de 16 en 16
      const l = [];
      for (let i = 0; i < b.length; i += 16) {
        const t = b.subarray(i, i + 16);
        l.push(`+${i.toString(16).padStart(4, '0')}  ${[...t].map((x) => x.toString(16).padStart(2, '0')).join(' ').padEnd(48)} ${t.toString('latin1').replace(/[^\x20-\x7e]/g, '.')}`);
      }
      return l.join('\n');
    }
  }
}
// Convierte un valor al hex que se escribe en memoria.
function aHex(valor, formato) {
  if (formato === 'hex') {
    const h = String(valor).replace(/\s+/g, '');
    if (!/^([0-9a-fA-F]{2})+$/.test(h)) throw new Error('hex no válido (pares de dígitos, ej. "90 90" o "EB")');
    return h.toUpperCase();
  }
  const anchos = { u8: 1, u16: 2, u32: 4, i32: 4, f32: 4, f64: 8 };
  const n = anchos[formato]; if (!n) throw new Error('formato no válido');
  const b = Buffer.alloc(n), v = Number(valor);
  if (!Number.isFinite(v)) throw new Error('valor no numérico');
  ({ u8: () => b.writeUInt8(v), u16: () => b.writeUInt16LE(v), u32: () => b.writeUInt32LE(v), i32: () => b.writeInt32LE(v), f32: () => b.writeFloatLE(v), f64: () => b.writeDoubleLE(v) })[formato]();
  return b.toString('hex').toUpperCase();
}
const EXPR_OK = /^[\s0-9a-zA-Z_.+\-\[\]]+$/;

// Herramientas de PRUEBA_CANCHA\scripts que SOLO LEEN (sin depurador ni congelar el juego).
// Las que usan depurador (observar*) o congelan el proceso (buscar_fisica) no se exponen.
const HERRAMIENTAS = {
  pelota: 'pelota.exe <segundos> <salida.txt> · encuentra el objeto de la pelota (se mueve)',
  estado_partido: 'estado_partido.exe <segundos> <salida.txt> · busca el objeto «estado del partido»',
  estado_log: 'estado_log.exe <dir_partido_hex> <segundos> <salida.txt> [dir_pelota_hex] · registra cada cambio de estado',
  instantanea: 'instantanea.exe <salida.bin> <hexdir>:<hexlen> ... · foto de memoria para comparar antes/después',
  buscar: 'buscar.exe <salida.txt> <v1,v2,...> · floats juntos en 64 bytes',
  buscar_valor: 'buscar_valor.exe scan <valor> <salida.txt> [ancho] | filtrar <entrada> <valor> <salida> [ancho]',
  volcar: 'volcar.exe info | leer <RVA_hex> [n]   (la orden «volcar» no se permite: escribe un archivo enorme)',
  grabar: 'grabar.exe <dir_hex_objeto> <segundos> <salida.bin> · graba la pelota a ~200 muestras/s',
  vigilar_arbitraje: 'vigilar_arbitraje.exe <carpeta_salida> [segundos] · vuelca la incidencia del árbitro',
};

// ─── comprobaciones de seguridad ─────────────────────────────────────────────
function salaAbierta() {
  if (cfg.acciones_con_sala_abierta) return null;
  const sala = leerClaves(join(phoenixDir(), 'sala.txt'));
  if (sala.existe && sala.datos.abierta === '1' && sala.edad_seg < 30) return 'Hay una sala de Phoenix Link abierta con gente: no se toca nada para no molestar a los invitados.';
  return null;
}
function puedeActuar() {
  if (!cfg.acciones) return 'Las acciones están APAGADAS. Para encenderlas: "acciones": true en config.json (y reiniciar Claude).';
  return salaAbierta();
}
function puedeEscribir() {
  if (!cfg.escribir_memoria) return 'Escribir memoria está APAGADO. Para encenderlo: "escribir_memoria": true en config.json (lo pone la persona a mano; luego reiniciar Claude).';
  return salaAbierta();
}
async function pesCerrado() {
  const p = await ayudante('proceso');
  return p.pes ? 'PES está abierto: cierra el juego antes (los archivos de Core los usa el juego mientras corre).' : null;
}

// ─── lectores de las fuentes de Phoenix ──────────────────────────────────────
function sondaJson() {
  const s = leerJson(join(phoenixDir(), 'sonda.json'));
  if (!s.existe) s.aviso = 'Falta phoenix_sonda.lua en sider.ini (ver LEEME.md).';
  else if (s.edad_seg > 10) s.aviso = `sonda.json lleva ${s.edad_seg} s sin cambiar: el juego está cerrado, minimizado o el módulo se apagó (pes_log fuente "sider" filtro "sonda").`;
  return s;
}
// Pantalla que el juego está mostrando: el último «flow» (.json de script\flow) que pidió, según peticiones.log de Core.
function flujoActual() {
  const ruta = join(coreDir(), 'peticiones.log');
  const ls = lineas(ruta, 256 * 1024);
  for (let i = ls.length - 1; i >= 0; i--) {
    const m = ls[i].match(/^(\d\d:\d\d:\d\d) \S+ (.*script\\flow\\(.+)\.json)\s*$/);
    if (m) return { pantalla: m[3].replace(/\\/g, '/'), hace_seg: Math.round((Date.now() - msHoy(m[1])) / 1000) };
  }
  return null;
}
// Último dato de cada etiqueta del registro de Core («AAAA-MM-DD hh:mm:ss.mmm  [ETIQUETA] texto»).
const RE_CORE = /^(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3})\s+\[([A-Za-z_0-9]+)\]\s?(.*)$/;
function coreUltimos(etiquetas, bytes = 1024 * 1024) {
  const res = {};
  const ls = lineas(join(coreDir(), 'phoenix_core.log'), bytes);
  for (let i = ls.length - 1; i >= 0 && Object.keys(res).length < etiquetas.length; i--) {
    const m = ls[i].match(RE_CORE);
    if (!m || !etiquetas.includes(m[2])) continue;
    if (m[2] === 'BALL' && !/pos=/.test(m[3])) continue;
    if (m[2] === 'JUG' && !res.JUG_V && /^\s*V:/.test(m[3])) { res.JUG_V = { t: msLocal(m[1]), txt: m[3] }; continue; }
    if (m[2] === 'JUG' && !/L:/.test(m[3])) continue;
    if (!res[m[2]]) res[m[2]] = { t: msLocal(m[1]), txt: m[3] };
  }
  return res;
}
const nums = (s) => s.split(',').map((x) => +x);
function jugadores(txt) {
  const r = [];
  for (const m of txt.matchAll(/(\d+)\((-?[\d.]+),(-?[\d.]+)\)(\*?)/g)) r.push({ n: +m[1], x: +m[2], z: +m[3], ...(m[4] ? { marca: '*' } : {}) });
  return r;
}
function coreEstado() {
  const e = leerJson(join(coreDir(), 'phoenix_core_estado.json'));
  if (!e.datos) return e;
  const d = e.datos;
  return { existe: true, edad_seg: e.edad_seg, version: d.version, modo: d.modo, apagado_por_archivo: existsSync(join(coreDir(), 'PHOENIX_CORE_OFF')), funciones: comoLista(d.funciones).map((f) => ({ id: f.id, activa: f.activa, ...(f.deseada !== f.activa ? { deseada: f.deseada } : {}), estado: String(f.estado || '').slice(0, 160) })) };
}

// ─── eventos en vivo (todas las fuentes juntas, ordenadas por hora) ──────────
const TIPOS_CORE = ['MARCADOR', 'CELEBRACION', 'SAQUE', 'DESCANSO', 'SHOOT', 'MURO', 'RED', 'ERROR'];
const TIPOS_CORE_DETALLE = ['COLA', 'PASO', 'SUPL', 'PORTERO', 'FORMACION', 'PT_PROPIO', 'CURSOR_PT', 'PORTERO_RAMA', 'MEDIDOR'];
function recogerEventos(desdeMs, tipos, detalle = false) {
  const ev = [];
  const quiere = (t) => !tipos?.length || tipos.includes(t);
  // 1) Sider (sonda): set_teams, set_conditions, goal_scored…
  if (quiere('sider')) {
    const s = leerJson(join(phoenixDir(), 'sonda.json'));
    for (const e of comoLista(s.datos?.eventos)) {
      const t = (e.t || 0) * 1000;
      if (t > desdeMs) ev.push({ t, fuente: 'sider', tipo: e.evento, ...(e.datos ? { datos: e.datos } : {}) });
    }
  }
  // 2) Phoenix Core: etiquetas de eventos y cualquier línea con error / congelado.
  if (quiere('core')) {
    for (const l of lineas(join(coreDir(), 'phoenix_core.log'), 1024 * 1024)) {
      const m = l.match(RE_CORE); if (!m) continue;
      const t = msLocal(m[1]); if (!(t > desdeMs)) continue;
      const alerta = /error|fall[oó]|excepci|crash|congel|bloque|MODO SEGURO/i.test(m[3]);
      if (TIPOS_CORE.includes(m[2]) || (detalle && TIPOS_CORE_DETALLE.includes(m[2])) || alerta || (m[2] === 'VIGILANTE' && !/^vivo/.test(m[3]))) ev.push({ t, fuente: 'core', tipo: m[2], texto: m[3].slice(0, 300) });
    }
    const st = join(coreDir(), 'street_estado.json');
    if (existsSync(st) && statSync(st).mtimeMs > desdeMs) { const j = leerJson(st); if (j.datos?.evento) ev.push({ t: statSync(st).mtimeMs, fuente: 'core', tipo: 'street', texto: j.datos.evento, datos: j.datos }); }
  }
  // 3) Pantallas del juego (flows pedidos, de peticiones.log).
  if (quiere('pantalla')) {
    let ultima = '';
    for (const l of lineas(join(coreDir(), 'peticiones.log'), 512 * 1024)) {
      const m = l.match(/^(\d\d:\d\d:\d\d) \S+ .*script\\flow\\(.+)\.json\s*$/); if (!m) continue;
      const t = msHoy(m[1]); const p = m[2].replace(/\\/g, '/');
      if (t > desdeMs && p !== ultima) ev.push({ t, fuente: 'juego', tipo: 'pantalla', texto: p });
      ultima = p;
    }
  }
  // 4) Escenas (opcional: son muchas animaciones).
  if (tipos?.includes('escenas')) {
    for (const l of lineas(join(phoenixDir(), 'escenas.log'), 128 * 1024)) {
      const [ts, ...resto] = l.split('|'); const t = +ts * 1000;
      if (t > desdeMs) ev.push({ t, fuente: 'referee', tipo: 'escena', texto: resto.join('|').split('\\').pop() });
    }
  }
  return ev.sort((a, b) => a.t - b.t);
}

// ─── rutas de menú guardadas ─────────────────────────────────────────────────
const RUTAS = join(AQUI, 'rutas.json');
const leerRutas = () => (existsSync(RUTAS) ? JSON.parse(readFileSync(RUTAS, 'utf8')) : {});
async function zonaYPantalla() {
  const s = leerJson(join(phoenixDir(), 'sonda.json'));
  let zona = s.datos?.zona && s.edad_seg <= 3 ? s.datos.zona : null;
  if (!zona) { try { const r = await ayudante('leer', ['-Expr', '[exe+0x3704E38]+0xF0', '-N', '4']); if (r.ok) zona = { id: Buffer.from(r.hex, 'hex').readUInt32LE(0) }; } catch { /* PES cerrado o sin gestor todavía */ } }
  return { zona, pantalla: flujoActual()?.pantalla };
}
const coincide = (actual, objetivo) => objetivo === undefined || objetivo === null || objetivo === '' ? false
  : typeof objetivo === 'number' ? actual.zona?.id === objetivo
  : String(actual.zona?.nombre || '').toUpperCase() === String(objetivo).toUpperCase() || String(actual.pantalla || '').toLowerCase().includes(String(objetivo).toLowerCase());

// ─── escrituras de memoria (registro para poder deshacer) ────────────────────
const ESCRITURAS = join(AQUI, 'escrituras.jsonl');
const leerEscrituras = () => (existsSync(ESCRITURAS) ? readFileSync(ESCRITURAS, 'utf8').split(/\r?\n/).filter(Boolean).map((l) => JSON.parse(l)) : []);

// ─── herramientas MCP ────────────────────────────────────────────────────────
const TOOLS = [
  {
    name: 'pes_estado',
    description: 'Foto rápida del juego AHORA: si PES y Sider están abiertos, zona (TOP_MENU, EXHIBITION, FREE_TRAINING…), pantalla del juego (último «flow» pedido), marcador, minuto, goleadores, equipos, estadio, ajustes, últimos eventos de Sider, versión y modo de Phoenix Core y la sala de Link. Solo lectura. Para el partido en detalle usa pes_partido.',
    inputSchema: { type: 'object', properties: { completo: { type: 'boolean', description: 'true = incluir ctx, eventos y funciones de Core completos' } } },
    async run({ completo = false } = {}) {
      const r = {};
      try { r.proceso = await ayudante('proceso'); } catch (e) { r.proceso = { error: e.message }; }
      const sonda = sondaJson();
      if (sonda.datos && !completo) {
        const d = sonda.datos;
        sonda.datos = { zona: d.zona, en_partido: d.en_partido, stats: d.stats, ctx: d.ctx && pick(d.ctx, ['home_team', 'away_team', 'tournament_id', 'match_id', 'stadium', 'stadium_choice', 'weather', 'timeofday', 'season', 'match_time', 'difficulty', 'extra_time', 'penalties', 'substitutions']), ultimos_eventos: comoLista(d.eventos).slice(-5) };
      }
      r.sonda = sonda;
      r.pantalla_juego = flujoActual();
      r.partido = leerJson(join(phoenixDir(), 'estado.json'));
      r.pantalla = leerJson(join(phoenixDir(), 'pantalla.json'));
      r.sala_link = leerClaves(join(phoenixDir(), 'sala.txt'));
      const core = coreEstado();
      r.core = completo || !core.funciones ? core : { version: core.version, modo: core.modo, apagado_por_archivo: core.apagado_por_archivo, edad_seg: core.edad_seg, funciones_activas: core.funciones.filter((f) => f.activa).length, funciones_con_problema: core.funciones.filter((f) => f.deseada !== undefined || /error|fall/i.test(f.estado)) };
      return texto(r);
    },
  },
  {
    name: 'pes_partido',
    description: 'El partido en detalle AHORA: marcador y reloj (de Sider y de Core), zona, equipos, posición y velocidad de la pelota, posición (x, z en metros) de los 22 jugadores, estado de street y los avisos de Core. Cada dato dice hace cuántos segundos se midió. Solo lectura.',
    inputSchema: { type: 'object', properties: {} },
    async run() {
      const ahora = Date.now();
      const hace = (t) => Math.round((ahora - t) / 100) / 10;
      const s = sondaJson();
      const r = { zona: s.datos?.zona, en_partido: s.datos?.en_partido, sonda_edad_seg: s.edad_seg };
      if (s.datos?.stats) { const st = s.datos.stats; r.marcador = { local: st.home_score, visita: st.away_score, penales: [st.pk_home_score, st.pk_away_score], periodo: st.period, minuto: `${st.clock_minutes}:${String(st.clock_seconds).padStart(2, '0')}`, anadido: st.added_minutes, ptr_partido: st.ptr_hex }; }
      if (s.datos?.ctx) r.equipos = { local: s.datos.ctx.home_team, visita: s.datos.ctx.away_team, torneo: s.datos.ctx.tournament_id, estadio: s.datos.ctx.stadium };
      r.goles = comoLista(s.datos?.eventos).filter((e) => e.evento === 'goal_scored').map((e) => ({ hora: isoLocal(e.t * 1000), ...e.datos }));
      const free = leerJson(join(phoenixDir(), 'marcador_free.json')); if (free.datos) r.marcador_core = { ...free.datos, edad_seg: free.edad_seg };
      const c = coreUltimos(['BALL', 'JUG', 'TIEMPOS', 'RELOJ', 'VIGILANTE', 'MARCADOR']);
      if (c.BALL) { const m = c.BALL.txt.match(/pos=\(([^)]+)\)\s*d=\(([^)]+)\)/); if (m) r.pelota = { pos_xyz: nums(m[1]), paso_xyz: nums(m[2]), hace_seg: hace(c.BALL.t) }; }
      if (c.JUG) {
        const lado = (txt) => { const js = jugadores(txt); return { en_campo: js.filter((j) => !j.marca).map((j) => `${j.n}(${j.x}, ${j.z})`), aparcados: js.filter((j) => j.marca).map((j) => j.n) }; };
        r.jugadores = { local: lado(c.JUG.txt), visita: c.JUG_V ? lado(c.JUG_V.txt) : null, fuera_de_la_cancha: +(c.JUG.txt.match(/fuera=(\d+)/)?.[1] ?? 0), hace_seg: hace(c.JUG.t), nota: 'n(x, z) en metros; «aparcados» = fuera del campo a propósito (z≈100: suplentes o jugadores que sobran en street)' };
      }
      for (const k of ['TIEMPOS', 'RELOJ', 'VIGILANTE', 'MARCADOR']) if (c[k]) r['core_' + k.toLowerCase()] = { texto: c[k].txt, hace_seg: hace(c[k].t) };
      const st = leerJson(join(coreDir(), 'street_estado.json')); if (st.datos) r.street = { ...st.datos, edad_seg: st.edad_seg };
      const res = leerJson(join(phoenixDir(), 'resultado.json')); if (res.datos) r.ultimo_resultado = { ...res.datos, edad_seg: res.edad_seg };
      r.pantalla_juego = flujoActual();
      if (!c.BALL && !c.JUG) r.aviso = 'Sin pelota ni jugadores: Phoenix Core no los está anotando (necesita las funciones pelota_sonda y jugadores_sonda) o no hay partido.';
      return texto(r);
    },
  },
  {
    name: 'pes_eventos',
    description: 'Eventos EN VIVO de todas las fuentes, ordenados por hora: Sider (equipos, condiciones, goles), Phoenix Core (marcador, celebración, saque, descanso, disparos, muros, red, avisos y errores, street) y las pantallas que el juego abre. Pasa el «cursor» de la respuesta anterior como «desde» para recibir solo lo nuevo, y «esperar» para quedarte escuchando hasta que pase algo.',
    inputSchema: { type: 'object', properties: {
      desde: { type: ['number', 'string'], description: 'cursor (ms) de la respuesta anterior, o hora «hh:mm:ss» de hoy. Por defecto: últimos 60 s' },
      esperar: { type: 'number', description: 'segundos a escuchar si aún no hay nada nuevo (0–90, por defecto 0)' },
      tipos: { type: 'array', items: { type: 'string', enum: ['sider', 'core', 'pantalla', 'escenas'] }, description: 'fuentes (por defecto sider, core y pantalla)' },
      max: { type: 'number', description: 'máximo de eventos (por defecto 80)' },
      detalle: { type: 'boolean', description: 'true = también los pasos finos de Core (cola de pasos del gol, portero, formación, cursor…)' },
    } },
    async run({ desde, esperar = 0, tipos, max = 80, detalle = false } = {}) {
      let d = Date.now() - 60000;
      if (typeof desde === 'number') d = desde;
      else if (typeof desde === 'string' && /^\d\d:\d\d:\d\d$/.test(desde)) d = msHoy(desde);
      else if (typeof desde === 'string' && /^\d+$/.test(desde)) d = +desde;
      const fin = Date.now() + Math.min(Math.max(esperar, 0), 90) * 1000;
      let ev = recogerEventos(d, tipos, detalle);
      while (!ev.length && Date.now() < fin) { await espera(700); ev = recogerEventos(d, tipos, detalle); }
      const lista = ev.slice(-Math.min(max, 400));
      const cursor = Math.round(lista.length ? lista[lista.length - 1].t : Math.max(d, Date.now() - 1000));
      return texto({ cursor, nuevos: ev.length, ...(ev.length > lista.length ? { recortados: ev.length - lista.length } : {}), eventos: lista.map((e) => ({ hora: isoLocal(e.t).slice(11), ...e, t: undefined })) });
    },
  },
  {
    name: 'pes_salud',
    description: '¿PES está bien? Dice si está abierto, si Windows lo ve «sin responder», cuánta CPU usa, si la sonda y Core siguen escribiendo, el veredicto (vivo / cargando / congelado / cerrado), los últimos cierres y cuelgues que anotó Windows (Visor de eventos: módulo culpable y dirección) y los volcados .dmp recientes. Solo lectura.',
    inputSchema: { type: 'object', properties: { dias: { type: 'number', description: 'días hacia atrás para cierres de Windows (por defecto 2, máx 30)' } } },
    async run({ dias = 2 } = {}) {
      const r = {};
      try { r.proceso = await ayudante('salud'); } catch (e) { r.proceso = { error: e.message }; }
      const s = sondaJson(); r.sonda_edad_seg = s.edad_seg;
      const log = join(coreDir(), 'phoenix_core.log'); r.core_log_edad_seg = existsSync(log) ? edadSeg(log) : null;
      const v = coreUltimos(['VIGILANTE']).VIGILANTE; if (v) r.core_vigilante = { texto: v.txt, hace_seg: Math.round((Date.now() - v.t) / 1000) };
      const ce = coreEstado(); if (ce.existe) r.core = { version: ce.version, modo: ce.modo, apagado_por_archivo: ce.apagado_por_archivo };
      const p = r.proceso || {};
      if (!p.pes) r.veredicto = 'cerrado';
      else if (p.colgada || p.responde === false) r.veredicto = (s.edad_seg ?? 99) > 5 ? 'CONGELADO: Windows lo ve sin responder y la sonda no avanza' : 'lento: Windows lo ve sin responder un momento, pero la sonda sigue';
      else if ((s.edad_seg ?? 99) > 10 && (p.cpu_seg_por_seg ?? 0) > 0.5) r.veredicto = 'cargando o en una pantalla sin Sider (CPU ocupada, sonda quieta)';
      else if ((s.edad_seg ?? 99) > 10) r.veredicto = 'sospechoso: la sonda no avanza y la CPU está casi quieta (¿minimizado o colgado?)';
      else r.veredicto = 'vivo';
      try { r.cierres_windows = (await ayudante('fallos', ['-Dias', String(Math.min(Math.max(dias, 1), 30))], 60000)).fallos; } catch (e) { r.cierres_windows = { error: e.message }; }
      const dmps = [];
      for (const dir of [cfg.juego, join(process.env.LOCALAPPDATA || join(homedir(), 'AppData', 'Local'), 'CrashDumps')]) {
        if (!existsSync(dir)) continue;
        for (const f of readdirSync(dir)) if (/\.dmp$/i.test(f) && /pes|sider|phoenix|dlss/i.test(f)) { const st = statSync(join(dir, f)); if (Date.now() - st.mtimeMs < dias * 86400000) dmps.push({ archivo: join(dir, f), fecha: isoLocal(st.mtimeMs), mb: Math.round(st.size / 1048576) }); }
      }
      r.volcados = dmps;
      return texto(r);
    },
  },
  {
    name: 'pes_log',
    description: 'Últimas líneas de un registro, con filtro. Fuentes: sider (sider.log), core (phoenix_core.log), peticiones (archivos que PES carga, de Core), sonda_partido (estados del partido, de Core), cargador (arranques de Core), escenas (escenas.log), avisos (avisos de la sala de Link).',
    inputSchema: { type: 'object', properties: {
      fuente: { type: 'string', enum: ['sider', 'core', 'peticiones', 'sonda_partido', 'cargador', 'escenas', 'avisos'], description: 'por defecto sider' },
      lineas: { type: 'number', description: 'cuántas (por defecto 60, máx 400)' },
      filtro: { type: 'string', description: 'solo líneas que contengan esto (sin distinguir mayúsculas); varias alternativas con «|»' },
    } },
    async run({ fuente = 'sider', lineas: n = 60, filtro = '' } = {}) {
      const rutas = { sider: join(siderDir(), 'sider.log'), core: join(coreDir(), 'phoenix_core.log'), peticiones: join(coreDir(), 'peticiones.log'), sonda_partido: join(coreDir(), 'sonda_partido.txt'), cargador: join(coreDir(), 'cargador.txt'), escenas: join(phoenixDir(), 'escenas.log'), avisos: join(phoenixDir(), 'avisos.txt') };
      const ruta = rutas[fuente]; if (!ruta) throw new Error('fuente no válida');
      if (!existsSync(ruta)) return texto('No existe ' + ruta);
      let l = cola(ruta, filtro ? 4 * 1024 * 1024 : 512 * 1024).split(/\r?\n/);
      if (filtro) { const alts = filtro.toLowerCase().split('|').filter(Boolean); l = l.filter((x) => { const y = x.toLowerCase(); return alts.some((a) => y.includes(a)); }); }
      return texto(`${basename(ruta)} · cambió hace ${edadSeg(ruta)} s\n` + (l.slice(-Math.min(Math.max(n, 1), 400)).join('\n') || '(sin líneas)'));
    },
  },
  {
    name: 'pes_captura',
    description: 'Foto SOLO de la ventana de PES (no del resto de la pantalla). Con PES al frente copia la pantalla; con PES detrás de otras ventanas usa la captura de Windows (Windows Graphics Capture, sin traerlo al frente); minimizado no hay foto (el juego no dibuja). Con «veces» saca una serie (para ver una animación o un cambio). Devuelve las imágenes.',
    inputSchema: { type: 'object', properties: { ancho: { type: 'number', description: 'ancho máximo en píxeles (por defecto 1280)' }, veces: { type: 'number', description: '1–6 fotos seguidas (por defecto 1)' }, intervalo_ms: { type: 'number', description: 'pausa entre fotos (por defecto 500)' } } },
    async run({ ancho = 1280, veces = 1, intervalo_ms = 500 } = {}) {
      const dir = mkdtempSync(join(tmpdir(), 'pesmcp-'));
      const png = join(dir, 'pes.png');
      const n = Math.min(Math.max(Math.round(veces), 1), 6);
      try {
        const r = await ayudante('captura', ['-Salida', png, '-Ancho', String(Math.round(ancho)), '-Veces', String(n), '-Intervalo', String(Math.max(100, Math.round(intervalo_ms)))], 30000 + n * 5000);
        const content = r.archivos.map((f) => ({ type: 'image', data: readFileSync(f).toString('base64'), mimeType: 'image/png' }));
        content.push({ type: 'text', text: `${n} foto(s) · ${r.ancho}x${r.alto} · ${r.metodo}${r.aviso ? ' · ' + r.aviso : ''}` });
        return { content };
      } finally { rmSync(dir, { recursive: true, force: true }); }
    },
  },
  {
    name: 'pes_memoria_leer',
    description: 'Lee memoria de PES2021.exe en vivo (SOLO LECTURA; el proceso se abre sin permiso de escribir). Dirección como expresión: "exe+0x3704E38", "[exe+0x3704E38]+0xF0" (corchetes = seguir puntero de 8 bytes), "0x17EFEF2A0", "phoenix_core.dll+0x1234", "sider.dll+0x10". Formatos: hex, u8, u16, u32, i32, f32, f64, ptr, texto.',
    inputSchema: { type: 'object', required: ['direccion'], properties: { direccion: { type: 'string' }, bytes: { type: 'number', description: '1–4096, por defecto 64' }, formato: { type: 'string', enum: ['hex', 'u8', 'u16', 'u32', 'i32', 'f32', 'f64', 'ptr', 'texto'] } } },
    async run({ direccion, bytes = 64, formato = 'hex' }) {
      if (!EXPR_OK.test(direccion)) throw new Error('Dirección con caracteres no válidos.');
      const r = await ayudante('leer', ['-Expr', direccion, '-N', String(Math.round(bytes))]);
      if (!r.ok) return texto(r);
      return texto(`dir ${r.dir} (${r.donde || (r.rva.startsWith('0x') ? 'exe+' + r.rva : r.rva)}) · ${r.n} bytes · ${formato}\n` + (formato === 'hex' ? interpretar(r.hex, 'hex') : JSON.stringify(interpretar(r.hex, formato))));
    },
  },
  {
    name: 'pes_memoria_firma',
    description: 'Busca una FIRMA de bytes (AOB) en el código del exe en vivo, como hace Phoenix Core: "48 8B 05 ?? ?? ?? ?? C3" («??» = cualquier byte). Devuelve cada coincidencia como exe+RVA. Con rip_desp y rip_fin calcula además el destino de una instrucción relativa (ej. para "48 8B 05 ?? ?? ?? ??": rip_desp 3, rip_fin 7). Sirve para encontrar funciones y variables aunque cambie el exe (otro parche o un PES limpio). Solo lectura.',
    inputSchema: { type: 'object', required: ['patron'], properties: {
      patron: { type: 'string' }, max: { type: 'number', description: 'coincidencias máximas (por defecto 20, máx 200)' },
      todo: { type: 'boolean', description: 'true = buscar también en las secciones de datos (textos, tablas), no solo en el código' },
      modulo: { type: 'string', description: 'otro módulo en vez del exe, ej. "phoenix_core.dll"' },
      rip_desp: { type: 'number' }, rip_fin: { type: 'number' },
    } },
    async run({ patron, max = 20, todo = false, modulo = '', rip_desp, rip_fin }) {
      if (!/^(\s*([0-9a-fA-F]{2}|\?\??)\s*)+$/.test(patron)) throw new Error('Patrón no válido: bytes hex separados por espacios y «??» como comodín.');
      if (modulo && !/^[\w.\-]+\.(dll|exe)$/i.test(modulo)) throw new Error('módulo no válido');
      const args = ['-Patron', patron.trim(), '-N', String(Math.min(Math.max(Math.round(max), 1), 200))];
      if (todo) args.push('-Todo');
      if (modulo) args.push('-Modulo', modulo);
      if (rip_desp !== undefined && rip_fin !== undefined) args.push('-Desp', String(rip_desp), '-Fin', String(rip_fin));
      return texto(await ayudante('firma', args, 120000));
    },
  },
  {
    name: 'pes_modulos',
    description: 'Lista los módulos (DLL) cargados en PES con su dirección base y tamaño: sider.dll, phoenix_core.dll, d3d11, ReShade… Sirve para leer memoria de un módulo («phoenix_core.dll+0x…») o saber qué está inyectado. Solo lectura.',
    inputSchema: { type: 'object', properties: { todos: { type: 'boolean', description: 'true = incluir también los de Windows' } } },
    async run({ todos = false } = {}) { return texto(await ayudante('modulos', todos ? ['-Todo'] : [])); },
  },
  {
    name: 'pes_memoria_escribir',
    description: 'ESCRIBE memoria de PES en vivo (apagado por defecto: config.json "escribir_memoria": true). Para probar un parche o un valor SIN recompilar Core. Obligatorio: «esperado» = los bytes que hay ahora (léelos antes con pes_memoria_leer); si no coinciden, no escribe. Máx 256 bytes. Cada escritura queda anotada y se deshace con pes_memoria_deshacer. Si es código, Windows lo cambia de protección un instante y lo devuelve. No actúa con sala de Link abierta.',
    inputSchema: { type: 'object', required: ['direccion', 'valor', 'esperado'], properties: {
      direccion: { type: 'string' }, valor: { type: ['string', 'number'], description: 'lo que se escribe' },
      formato: { type: 'string', enum: ['hex', 'u8', 'u16', 'u32', 'i32', 'f32', 'f64'], description: 'por defecto hex' },
      esperado: { type: 'string', description: 'bytes actuales en hex (mismo largo que lo que se escribe)' },
      nota: { type: 'string', description: 'para qué es (queda en el registro)' },
    } },
    async run({ direccion, valor, formato = 'hex', esperado, nota = '' }) {
      const no = puedeEscribir(); if (no) return texto(no);
      if (!EXPR_OK.test(direccion)) throw new Error('Dirección con caracteres no válidos.');
      const hex = aHex(valor, formato), esp = aHex(esperado, 'hex');
      if (hex.length > 512) throw new Error('Máximo 256 bytes.');
      if (esp.length !== hex.length) throw new Error(`«esperado» mide ${esp.length / 2} bytes y lo que se escribe ${hex.length / 2}: tienen que medir lo mismo.`);
      const r = await ayudante('escribir', ['-Expr', direccion, '-Hex', hex, '-Esperado', esp]);
      if (r.ok) appendFileSync(ESCRITURAS, JSON.stringify({ id: Date.now(), hora: isoLocal(Date.now()), pid: r.pid, dir: r.dir, rva: r.rva, expr: direccion, antes: r.antes, despues: hex, nota }) + '\n');
      return texto(r);
    },
  },
  {
    name: 'pes_memoria_deshacer',
    description: 'Deshace escrituras de pes_memoria_escribir en el PES que está abierto ahora (la última, o todas con «todas»). Solo si los bytes siguen siendo los que se escribieron. Sin argumentos y con «listar» muestra el registro.',
    inputSchema: { type: 'object', properties: { todas: { type: 'boolean' }, listar: { type: 'boolean' } } },
    async run({ todas = false, listar = false } = {}) {
      const reg = leerEscrituras();
      if (listar) return texto(reg.slice(-40));
      const no = puedeEscribir(); if (no) return texto(no);
      const p = await ayudante('proceso'); if (!p.pes) return texto('PES está cerrado: al cerrarse, todo lo escrito ya se perdió.');
      const deshechas = new Set(reg.filter((e) => e.deshace).map((e) => e.deshace));
      const pendientes = reg.filter((e) => !e.deshace && e.pid === p.pid && !deshechas.has(e.id)).reverse();
      if (!pendientes.length) return texto('No hay escrituras pendientes de deshacer en este PES.');
      const res = [];
      for (const e of todas ? pendientes : pendientes.slice(0, 1)) {
        const r = await ayudante('escribir', ['-Expr', e.dir, '-Hex', e.antes, '-Esperado', e.despues]);
        if (r.ok) appendFileSync(ESCRITURAS, JSON.stringify({ deshace: e.id, hora: isoLocal(Date.now()) }) + '\n');
        res.push({ dir: e.dir, nota: e.nota, ...r });
      }
      return texto(res);
    },
  },
  {
    name: 'pes_escenas',
    description: 'Escenas que el juego pidió durante el partido (tarjetas, faltas, cambios, córners, celebraciones…), de content\\phoenix\\escenas.log (phoenix_referee v1.3+). Sirve para saber QUÉ pasó.',
    inputSchema: { type: 'object', properties: { lineas: { type: 'number' } } },
    async run({ lineas: n = 80 } = {}) {
      const ruta = join(phoenixDir(), 'escenas.log');
      if (!existsSync(ruta)) return texto('No hay escenas.log');
      return texto(cola(ruta, 256 * 1024).split(/\r?\n/).filter(Boolean).slice(-Math.min(n, 500)).join('\n') || '(vacío: aún no hubo escenas en este partido)');
    },
  },
  {
    name: 'pes_hallazgos_buscar',
    description: 'Busca texto en los informes de investigación de PES (catálogo de hallazgos, bitácora, guías: direcciones, funciones, offsets ya descubiertos) antes de investigar algo de nuevo.',
    inputSchema: { type: 'object', required: ['texto'], properties: { texto: { type: 'string' }, max: { type: 'number', description: 'resultados (por defecto 40)' } } },
    async run({ texto: q, max = 40 }) {
      const res = [];
      const ql = q.toLowerCase();
      const recorrer = (dir, prof = 0) => {
        if (prof > 4 || res.length >= max || !existsSync(dir)) return;
        for (const e of readdirSync(dir, { withFileTypes: true })) {
          if (res.length >= max) return;
          const p = join(dir, e.name);
          if (e.isDirectory()) { if (!/^(datos|copias|respaldo)/i.test(e.name)) recorrer(p, prof + 1); }
          else if (['.md', '.txt'].includes(extname(e.name).toLowerCase()) && statSync(p).size < 4 * 1024 * 1024) {
            readFileSync(p, 'utf8').split(/\r?\n/).forEach((l, i) => { if (res.length < max && l.toLowerCase().includes(ql)) res.push(`${basename(p)}:${i + 1}: ${l.trim().slice(0, 300)}`); });
          }
        }
      };
      for (const d of [].concat(cfg.documentos)) recorrer(d);
      return texto(res.join('\n') || 'Nada encontrado.');
    },
  },
  {
    name: 'pes_herramienta',
    description: 'Ejecuta una herramienta de investigación de PRUEBA_CANCHA\\scripts que SOLO LEE la memoria del juego. Disponibles: ' + Object.entries(HERRAMIENTAS).map(([k, v]) => `${k} (${v})`).join(' · ') + '. Las salidas relativas quedan en PRUEBA_CANCHA\\informes\\datos.',
    inputSchema: { type: 'object', required: ['nombre'], properties: { nombre: { type: 'string', enum: Object.keys(HERRAMIENTAS) }, argumentos: { type: 'array', items: { type: 'string' } }, segundos_max: { type: 'number', description: 'tope de espera (por defecto 120, máx 600)' } } },
    async run({ nombre, argumentos = [], segundos_max = 120 }) {
      if (!HERRAMIENTAS[nombre]) throw new Error('Herramienta no permitida.');
      if (nombre === 'volcar' && argumentos[0] && !['info', 'leer'].includes(argumentos[0])) throw new Error('volcar: solo «info» o «leer».');
      if (argumentos.some((a) => /[&|<>;`]/.test(a))) throw new Error('Argumento con caracteres no permitidos.');
      const exe = join(cfg.investigacion, 'scripts', nombre + '.exe');
      if (!existsSync(exe)) throw new Error('No existe ' + exe);
      const r = await ejecutar(exe, argumentos, { cwd: join(cfg.investigacion, 'informes', 'datos'), timeout: Math.min(segundos_max, 600) * 1000 });
      return texto({ ok: r.ok, codigo: r.codigo, salida: (r.stdout + r.stderr).slice(-8000) });
    },
  },
  {
    name: 'pes_tecla',
    description: 'ACCIÓN (apagada por defecto): pulsa teclas en la ventana de PES. Teclas: Enter, Esc, Espacio, Tab, Retroceso, Arriba, Abajo, Izquierda, Derecha, Shift, Ctrl, Q W E R A S D Z C V F, 1, 2; combinación con «+» (ej. "Shift+R"). La X no está (abre «Guardar repetición»). No actúa si hay sala de Link abierta. Para ir a una pantalla concreta usa pes_navegar.',
    inputSchema: { type: 'object', required: ['teclas'], properties: { teclas: { type: 'string' }, veces: { type: 'number', description: '1–20' } } },
    async run({ teclas, veces = 1 }) {
      const no = puedeActuar(); if (no) return texto(no);
      return texto(await ayudante('tecla', ['-Teclas', teclas, '-Veces', String(Math.round(veces))]));
    },
  },
  {
    name: 'pes_navegar',
    description: 'ACCIÓN (apagada por defecto): recorre menús con una sola orden. Ejecuta «pasos» (teclas) uno a uno y, tras cada uno, mira la zona y la pantalla del juego; para en cuanto llega a «hasta» (número o nombre de zona, ej. "TOP_MENU", o un trozo del nombre de la pantalla, ej. "MatchPause"). Una ruta que funcionó se guarda con «guardar_como» y luego se repite con «ruta». Sin argumentos lista las rutas guardadas.',
    inputSchema: { type: 'object', properties: {
      ruta: { type: 'string', description: 'nombre de una ruta guardada' },
      pasos: { type: 'array', items: { type: 'object', required: ['teclas'], properties: { teclas: { type: 'string' }, veces: { type: 'number' }, esperar_ms: { type: 'number', description: 'pausa tras el paso (por defecto 800)' } } } },
      hasta: { type: ['string', 'number'], description: 'objetivo: zona (id o nombre) o trozo del nombre de pantalla' },
      repetir_hasta: { type: 'number', description: 'repetir los pasos hasta llegar al objetivo, como máximo N vueltas (por defecto 1)' },
      guardar_como: { type: 'string', description: 'guardar esta ruta si llega al objetivo' },
      captura: { type: 'boolean', description: 'foto al final (por defecto true)' },
    } },
    async run({ ruta, pasos, hasta, repetir_hasta = 1, guardar_como, captura = true } = {}) {
      const rutas = leerRutas();
      if (!ruta && !pasos) return texto({ rutas, consejo: 'Para crear una: pasos + hasta + guardar_como.' });
      const no = puedeActuar(); if (no) return texto(no);
      if (ruta) {
        const g = rutas[ruta];
        if (!g) throw new Error('No hay ninguna ruta «' + ruta + '». Guardadas: ' + Object.keys(rutas).join(', '));
        pasos = g.pasos; if (hasta === undefined) hasta = g.hasta; repetir_hasta = g.repetir_hasta ?? repetir_hasta;
      }
      if (!Array.isArray(pasos) || !pasos.length || pasos.length > 40) throw new Error('pasos: entre 1 y 40.');
      const traza = [];
      let actual = await zonaYPantalla();
      traza.push({ paso: 'inicio', ...actual });
      let llego = coincide(actual, hasta);
      for (let vuelta = 0; vuelta < Math.min(Math.max(repetir_hasta, 1), 20) && !llego; vuelta++) {
        for (const p of pasos) {
          const otra = puedeActuar(); if (otra) { traza.push({ paro: otra }); break; }
          try { await ayudante('tecla', ['-Teclas', p.teclas, '-Veces', String(Math.round(p.veces || 1))]); }
          catch (e) { traza.push({ paso: p.teclas, error: e.message }); return texto({ llego: false, traza }); }
          await espera(Math.min(Math.max(p.esperar_ms ?? 800, 100), 10000));
          actual = await zonaYPantalla();
          traza.push({ paso: p.teclas + (p.veces > 1 ? ' ×' + p.veces : ''), ...actual });
          if ((llego = coincide(actual, hasta))) break;
        }
      }
      if (llego && guardar_como) { rutas[guardar_como] = { pasos, hasta, repetir_hasta, guardada: isoLocal(Date.now()) }; writeFileSync(RUTAS, JSON.stringify(rutas, null, 1)); }
      const res = texto({ llego: hasta === undefined ? null : llego, ...(llego && guardar_como ? { guardada: guardar_como } : {}), traza });
      if (captura) { try { const f = await TOOLS.find((t) => t.name === 'pes_captura').run({ ancho: 960 }); res.content.unshift(...f.content); } catch (e) { res.content.push({ type: 'text', text: 'sin foto: ' + e.message }); } }
      return res;
    },
  },
  {
    name: 'pes_recargar_modulos',
    description: 'ACCIÓN (apagada por defecto): recarga los módulos Lua de Sider sin cerrar el juego (Shift+R). Úsalo tras copiar un .lua nuevo. Mejor en el menú, no en pleno partido.',
    inputSchema: { type: 'object', properties: {} },
    async run() {
      const no = puedeActuar(); if (no) return texto(no);
      const r = await ayudante('tecla', ['-Teclas', 'Shift+R']);
      return texto({ ...r, consejo: 'Mira pes_log con filtro "listo" para ver qué módulos volvieron a cargar.' });
    },
  },
  {
    name: 'pes_core',
    description: 'Phoenix Core (la DLL propia). Lectura: «estado» (versión, modo normal/seguro, cada función y su estado), «versiones» (copias guardadas: street_estable_*, street_prueba_*, .bak y respaldos del MCP). ACCIONES (apagadas por defecto y SOLO con PES cerrado; siempre guarda copia antes): «instalar» (copia phoenix_core.dll + .json y reemplaza los .lua de esa versión que ya estén en modules), «revertir» (vuelve a la copia anterior del MCP), «funcion» (enciende/apaga o cambia un valor en phoenix_core.json), «apagar»/«encender» (archivo PHOENIX_CORE_OFF: Core no arranca, sin desinstalar). El código de Core es del hilo de street: avisa antes de cambiar la versión.',
    inputSchema: { type: 'object', required: ['accion'], properties: {
      accion: { type: 'string', enum: ['estado', 'versiones', 'instalar', 'revertir', 'funcion', 'apagar', 'encender'] },
      version: { type: 'string', description: 'instalar: carpeta dentro de phoenix_core (ej. "street_estable_0486") o ruta completa; revertir: nombre del respaldo (por defecto el último)' },
      id: { type: 'string', description: 'funcion: clave dentro de "funciones" (o de la raíz) de phoenix_core.json' },
      valor: { type: ['boolean', 'number', 'string'], description: 'funcion: valor nuevo' },
    } },
    async run({ accion, version, id, valor }) {
      const dir = coreDir();
      const respaldos = join(dir, '_mcp_respaldos');
      if (accion === 'estado') return texto({ ...coreEstado(), dll: existsSync(join(dir, 'phoenix_core.dll')) ? { fecha: isoLocal(statSync(join(dir, 'phoenix_core.dll')).mtimeMs), kb: Math.round(statSync(join(dir, 'phoenix_core.dll')).size / 1024) } : null, ultimos_arranques: lineas(join(dir, 'cargador.txt'), 8192).slice(-5) });
      if (accion === 'versiones') {
        const v = [];
        for (const e of readdirSync(dir, { withFileTypes: true })) {
          const p = join(dir, e.name);
          if (e.isDirectory() && existsSync(join(p, 'phoenix_core.dll'))) v.push({ nombre: e.name, fecha: isoLocal(statSync(join(p, 'phoenix_core.dll')).mtimeMs), nota: existsSync(join(p, 'LEEME.txt')) ? readFileSync(join(p, 'LEEME.txt'), 'utf8').split(/\r?\n/)[0].slice(0, 240) : undefined, archivos: readdirSync(p) });
          else if (e.isFile() && /\.dll\..*bak$|\.bak$/i.test(e.name)) v.push({ nombre: e.name, fecha: isoLocal(statSync(p).mtimeMs), tipo: 'dll suelta' });
        }
        const r = existsSync(respaldos) ? readdirSync(respaldos).sort() : [];
        return texto({ instalada: coreEstado().version, versiones: v, respaldos_mcp: r });
      }
      const no = puedeActuar() || await pesCerrado(); if (no) return texto(no);
      const respaldar = (archivos) => {
        const d = join(respaldos, sello()); mkdirSync(d, { recursive: true });
        const hechos = [];
        for (const [origen, nombre] of archivos) if (existsSync(origen)) { copyFileSync(origen, join(d, nombre)); hechos.push(nombre); }
        writeFileSync(join(d, 'origen.json'), JSON.stringify({ hora: isoLocal(Date.now()), version_antes: coreEstado().version, accion, version, id, valor, archivos: hechos }, null, 1));
        return d;
      };
      const modules = join(siderDir(), 'modules');
      if (accion === 'instalar') {
        if (!version || /[<>|"*?]/.test(version)) throw new Error('Falta «version» (carpeta o ruta).');
        const src = isAbsolute(version) ? version : join(dir, version);
        if (!existsSync(join(src, 'phoenix_core.dll'))) throw new Error('No hay phoenix_core.dll en ' + src);
        const luas = readdirSync(src).filter((f) => /\.lua$/i.test(f));
        const luasReemplazo = luas.filter((f) => existsSync(join(modules, f)));
        const d = respaldar([[join(dir, 'phoenix_core.dll'), 'phoenix_core.dll'], [join(dir, 'phoenix_core.json'), 'phoenix_core.json'], ...luasReemplazo.map((f) => [join(modules, f), f])]);
        copyFileSync(join(src, 'phoenix_core.dll'), join(dir, 'phoenix_core.dll'));
        if (existsSync(join(src, 'phoenix_core.json'))) copyFileSync(join(src, 'phoenix_core.json'), join(dir, 'phoenix_core.json'));
        for (const f of luasReemplazo) copyFileSync(join(src, f), join(modules, f));
        return texto({ ok: true, instalada: version, respaldo: d, lua_reemplazados: luasReemplazo, lua_no_copiados: luas.filter((f) => !luasReemplazo.includes(f)).map((f) => f + ' (no está en modules: los módulos nuevos los añade Link)'), siguiente: 'Abrir PES y mirar pes_core estado (versión) y pes_salud.' });
      }
      if (accion === 'revertir') {
        const lista = existsSync(respaldos) ? readdirSync(respaldos).sort() : [];
        const nombre = version || lista[lista.length - 1];
        if (!nombre || !existsSync(join(respaldos, nombre))) throw new Error('No hay respaldo «' + (nombre || '') + '». Hay: ' + lista.join(', '));
        const d = join(respaldos, nombre);
        const archivos = readdirSync(d).filter((f) => f !== 'origen.json');
        const destino = (f) => (/\.lua$/i.test(f) ? join(modules, f) : join(dir, f));
        const nuevo = respaldar(archivos.map((f) => [destino(f), f]));
        for (const f of archivos) copyFileSync(join(d, f), destino(f));
        return texto({ ok: true, restaurado: nombre, archivos, respaldo_de_lo_que_habia: nuevo });
      }
      if (accion === 'funcion') {
        if (!id) throw new Error('Falta «id».');
        const ruta = join(dir, 'phoenix_core.json');
        const j = JSON.parse(readFileSync(ruta, 'utf8').replace(/^\uFEFF/, ''));
        const donde = j.funciones && id in j.funciones ? j.funciones : id in j ? j : null;
        if (!donde) throw new Error('«' + id + '» no está en phoenix_core.json (ni en funciones ni en la raíz). Mira pes_core estado.');
        if (typeof donde[id] !== typeof valor) throw new Error(`«${id}» es ${typeof donde[id]} y el valor nuevo es ${typeof valor}.`);
        const antes = donde[id];
        const d = respaldar([[ruta, 'phoenix_core.json']]);
        donde[id] = valor;
        writeFileSync(ruta, JSON.stringify(j, null, 2));
        return texto({ ok: true, id, antes, ahora: valor, respaldo: d });
      }
      if (accion === 'apagar') { writeFileSync(join(dir, 'PHOENIX_CORE_OFF'), ''); return texto({ ok: true, nota: 'Core no arrancará en el próximo inicio de PES (el juego queda como sin Core).' }); }
      if (accion === 'encender') { if (existsSync(join(dir, 'PHOENIX_CORE_OFF'))) unlinkSync(join(dir, 'PHOENIX_CORE_OFF')); return texto({ ok: true, nota: 'Core vuelve a arrancar en el próximo inicio de PES.' }); }
      throw new Error('acción no válida');
    },
  },
];

// ─── protocolo MCP (JSON-RPC 2.0, una línea por mensaje) ─────────────────────
function responder(id, result, error) {
  process.stdout.write(JSON.stringify(error ? { jsonrpc: '2.0', id, error } : { jsonrpc: '2.0', id, result }) + '\n');
}
async function atender(msg) {
  const { id, method, params } = msg;
  if (method === 'initialize') {
    return responder(id, {
      protocolVersion: params?.protocolVersion || '2025-06-18',
      capabilities: { tools: {} },
      serverInfo: { name: 'phoenix-pes', version: VERSION },
      instructions: 'MCP de PES 2021 de Phoenix Evolution. Empieza por pes_estado (o pes_salud si algo va mal). En partido: pes_partido y pes_eventos (con «esperar» para escuchar en vivo). Antes de investigar algo nuevo usa pes_hallazgos_buscar. Para encontrar código usa pes_memoria_firma. Todo lo de lectura es seguro; las acciones y la escritura de memoria están apagadas salvo que config.json diga lo contrario. Otros hilos también usan PES: no abras ni cierres el juego por tu cuenta y no pulses nada mientras otro prueba.',
    });
  }
  if (method === 'ping') return responder(id, {});
  if (method === 'tools/list') return responder(id, { tools: TOOLS.map(({ run, ...t }) => t) });
  if (method === 'tools/call') {
    const t = TOOLS.find((x) => x.name === params?.name);
    if (!t) return responder(id, null, { code: -32602, message: 'Herramienta desconocida: ' + params?.name });
    try { return responder(id, await t.run(params.arguments || {})); }
    catch (e) { return responder(id, { content: [{ type: 'text', text: 'Error: ' + e.message }], isError: true }); }
  }
  if (id !== undefined) responder(id, null, { code: -32601, message: 'Método no soportado: ' + method });
}

if (process.argv.includes('--prueba')) {
  // Prueba rápida sin Claude: lista las herramientas y ejecuta las de lectura que no necesitan argumentos.
  console.log(`phoenix-pes ${VERSION} · ${TOOLS.length} herramientas: ` + TOOLS.map((t) => t.name).join(', '));
  for (const [n, a] of [['pes_estado', {}], ['pes_partido', {}], ['pes_eventos', { desde: Date.now() - 300000, max: 15 }], ['pes_salud', { dias: 1 }], ['pes_core', { accion: 'versiones' }]]) {
    try { console.log(`\n── ${n} ──\n` + (await TOOLS.find((t) => t.name === n).run(a)).content.map((c) => c.text || '[imagen]').join('\n').slice(0, 3000)); }
    catch (e) { console.log(`\n── ${n} ── ERROR: ${e.message}`); }
  }
} else {
  const rl = createInterface({ input: process.stdin });
  rl.on('line', (l) => { if (!l.trim()) return; let m; try { m = JSON.parse(l); } catch { return responder(null, null, { code: -32700, message: 'JSON inválido' }); } atender(m); });
}
