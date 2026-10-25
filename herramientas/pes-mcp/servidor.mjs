#!/usr/bin/env node
// =============================================================================
//  Phoenix Evolution · MCP de PES 2021 (servidor MCP por stdio, sin dependencias)
//  Deja que Claude (Claude Code / Claude Desktop en la PC de Fralex) mire PES 2021 en vivo:
//    LECTURA (siempre):   pes_estado · pes_log · pes_captura · pes_memoria_leer · pes_escenas ·
//                         pes_hallazgos_buscar · pes_herramienta (herramientas de PRUEBA_CANCHA que solo leen)
//    ACCIONES (apagadas): pes_tecla · pes_recargar_modulos      → config.json "acciones": true
//  Nunca escribe en la memoria del juego ni toca sider.ini.
//  Fuentes: content\phoenix\sonda.json (phoenix_sonda.lua), estado.json / pantalla.json
//  (phoenix_referee.lua), sala.txt (Phoenix Link), sider.log y ayudante.ps1 (Windows).
//  Uso: node servidor.mjs   (lo arranca Claude solo; ver LEEME.md)   · Prueba: node servidor.mjs --prueba
// =============================================================================
import { readFileSync, existsSync, statSync, openSync, readSync, closeSync, readdirSync, mkdtempSync, rmSync } from 'node:fs';
import { join, dirname, basename, extname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFile } from 'node:child_process';
import { tmpdir } from 'node:os';
import { createInterface } from 'node:readline';

const VERSION = '0.1.0';
const AQUI = dirname(fileURLToPath(import.meta.url));

// ─── configuración ───────────────────────────────────────────────────────────
const POR_DEFECTO = {
  juego: 'D:\\Frank\\Games_\\Conmegol Patch',
  investigacion: 'D:\\Frank\\Games_\\PRUEBA_CANCHA',
  documentos: ['D:\\Frank\\Games_\\PRUEBA_CANCHA\\informes'],
  acciones: false,                 // pes_tecla y pes_recargar_modulos
  acciones_con_sala_abierta: false, // con invitados en la sala de Parsec, nunca se pulsa nada
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

// ─── utilidades ──────────────────────────────────────────────────────────────
function leerJson(ruta) {
  if (!existsSync(ruta)) return { existe: false };
  const edad = Math.round((Date.now() - statSync(ruta).mtimeMs) / 1000);
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
  return { existe: true, edad_seg: Math.round((Date.now() - statSync(ruta).mtimeMs) / 1000), datos };
}
// Últimos `max` bytes de un archivo (sider.log puede pesar mucho).
function cola(ruta, max = 512 * 1024) {
  const tam = statSync(ruta).size;
  const n = Math.min(tam, max);
  const b = Buffer.alloc(n);
  const fd = openSync(ruta, 'r');
  try { readSync(fd, b, 0, n, tam - n); } finally { closeSync(fd); }
  return b.toString('utf8');
}
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

// ─── comprobación de seguridad para acciones ─────────────────────────────────
function puedeActuar() {
  if (!cfg.acciones) return 'Las acciones están APAGADAS. Para encenderlas: "acciones": true en config.json (y reiniciar Claude).';
  if (!cfg.acciones_con_sala_abierta) {
    const sala = leerClaves(join(phoenixDir(), 'sala.txt'));
    if (sala.existe && sala.datos.abierta === '1' && sala.edad_seg < 30) return 'Hay una sala de Phoenix Link abierta con gente: no se pulsa nada para no molestar a los invitados.';
  }
  return null;
}

// ─── herramientas MCP ────────────────────────────────────────────────────────
const TOOLS = [
  {
    name: 'pes_estado',
    description: 'Foto del juego AHORA: si PES y Sider están abiertos, zona/menú (TOP_MENU, EXHIBITION, EDIT…), pantalla (menu/equipos/en_juego/descanso…), marcador, minuto, goles con goleador, equipos, estadio, ajustes del partido, últimos eventos de Sider y la sala de Phoenix Link. Solo lectura.',
    inputSchema: { type: 'object', properties: { completo: { type: 'boolean', description: 'true = incluir ctx y eventos completos de la sonda' } } },
    async run({ completo = false } = {}) {
      const r = {};
      try { r.proceso = await ayudante('proceso'); } catch (e) { r.proceso = { error: e.message } }
      const sonda = leerJson(join(phoenixDir(), 'sonda.json'));
      if (sonda.datos && !completo) {
        const d = sonda.datos;
        sonda.datos = { zona: d.zona, en_partido: d.en_partido, stats: d.stats, ctx: d.ctx && pick(d.ctx, ['home_team', 'away_team', 'tournament_id', 'match_id', 'stadium', 'stadium_choice', 'weather', 'timeofday', 'season', 'match_time', 'difficulty', 'extra_time', 'penalties', 'substitutions']), ultimos_eventos: (d.eventos || []).slice(-5) };
      }
      if (!sonda.existe) sonda.aviso = 'Falta phoenix_sonda.lua en sider.ini (ver LEEME.md). Sin ella solo hay estado.json/pantalla.json.';
      else if (sonda.edad_seg > 10) sonda.aviso = 'sonda.json lleva ' + sonda.edad_seg + ' s sin cambiar: el juego está cerrado, minimizado o el módulo se apagó (mira pes_log con filtro "sonda").';
      r.sonda = sonda;
      r.partido = leerJson(join(phoenixDir(), 'estado.json'));
      r.pantalla = leerJson(join(phoenixDir(), 'pantalla.json'));
      r.sala_link = leerClaves(join(phoenixDir(), 'sala.txt'));
      return texto(r);
    },
  },
  {
    name: 'pes_log',
    description: 'Últimas líneas de sider.log (errores de módulos, avisos de phoenix, referee, sonda…). Filtro opcional por texto.',
    inputSchema: { type: 'object', properties: { lineas: { type: 'number', description: 'cuántas (por defecto 60, máx 400)' }, filtro: { type: 'string', description: 'solo líneas que contengan esto (sin distinguir mayúsculas)' } } },
    async run({ lineas = 60, filtro = '' } = {}) {
      const ruta = join(siderDir(), 'sider.log');
      if (!existsSync(ruta)) return texto('No hay sider.log en ' + ruta);
      let l = cola(ruta).split(/\r?\n/);
      if (filtro) l = l.filter((x) => x.toLowerCase().includes(filtro.toLowerCase()));
      return texto(l.slice(-Math.min(Math.max(lineas, 1), 400)).join('\n') || '(sin líneas)');
    },
  },
  {
    name: 'pes_captura',
    description: 'Foto SOLO de la ventana de PES (no del resto de la pantalla). Devuelve la imagen.',
    inputSchema: { type: 'object', properties: { ancho: { type: 'number', description: 'ancho máximo en píxeles (por defecto 1280)' } } },
    async run({ ancho = 1280 } = {}) {
      const dir = mkdtempSync(join(tmpdir(), 'pesmcp-'));
      const png = join(dir, 'pes.png');
      try {
        const r = await ayudante('captura', ['-Salida', png, '-Ancho', String(Math.round(ancho))]);
        return { content: [{ type: 'image', data: readFileSync(png).toString('base64'), mimeType: 'image/png' }, { type: 'text', text: `${r.ancho}x${r.alto}` }] };
      } finally { rmSync(dir, { recursive: true, force: true }); }
    },
  },
  {
    name: 'pes_memoria_leer',
    description: 'Lee memoria de PES2021.exe en vivo (SOLO LECTURA; el proceso se abre sin permiso de escribir). Dirección como expresión: "exe+0x3704E38", "[exe+0x3704E38]+0xF0" (corchetes = seguir puntero de 8 bytes), "0x17EFEF2A0". Formatos: hex, u8, u16, u32, i32, f32, f64, ptr, texto.',
    inputSchema: { type: 'object', required: ['direccion'], properties: { direccion: { type: 'string' }, bytes: { type: 'number', description: '1–4096, por defecto 64' }, formato: { type: 'string', enum: ['hex', 'u8', 'u16', 'u32', 'i32', 'f32', 'f64', 'ptr', 'texto'] } } },
    async run({ direccion, bytes = 64, formato = 'hex' }) {
      if (!/^[\s0-9a-fA-FxX+\-\[\]exEX]+$/.test(direccion)) throw new Error('Dirección con caracteres no válidos.');
      const r = await ayudante('leer', ['-Expr', direccion, '-N', String(Math.round(bytes))]);
      if (!r.ok) return texto(r);
      return texto(`dir ${r.dir} (${r.rva.startsWith('0x') ? 'exe+' + r.rva : r.rva}) · ${r.n} bytes · ${formato}\n` + (formato === 'hex' ? interpretar(r.hex, 'hex') : JSON.stringify(interpretar(r.hex, formato))));
    },
  },
  {
    name: 'pes_escenas',
    description: 'Escenas que el juego pidió durante el partido (tarjetas, faltas, cambios, córners…), de content\\phoenix\\escenas.log (phoenix_referee v1.3+). Sirve para saber QUÉ pasó.',
    inputSchema: { type: 'object', properties: { lineas: { type: 'number' } } },
    async run({ lineas = 80 } = {}) {
      const ruta = join(phoenixDir(), 'escenas.log');
      if (!existsSync(ruta)) return texto('No hay escenas.log');
      return texto(cola(ruta, 256 * 1024).split(/\r?\n/).filter(Boolean).slice(-Math.min(lineas, 500)).join('\n') || '(vacío: aún no hubo escenas en este partido)');
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
    description: 'ACCIÓN (apagada por defecto): pulsa teclas en la ventana de PES. Teclas: Enter, Esc, Espacio, Tab, Retroceso, Arriba, Abajo, Izquierda, Derecha, Shift, Ctrl, Q W E R A S D Z C V F, 1, 2; combinación con «+» (ej. "Shift+R"). La X no está (abre «Guardar repetición»). No actúa si hay sala de Link abierta.',
    inputSchema: { type: 'object', required: ['teclas'], properties: { teclas: { type: 'string' }, veces: { type: 'number', description: '1–20' } } },
    async run({ teclas, veces = 1 }) {
      const no = puedeActuar(); if (no) return texto(no);
      return texto(await ayudante('tecla', ['-Teclas', teclas, '-Veces', String(Math.round(veces))]));
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
];
function pick(o, claves) { const r = {}; for (const k of claves) if (o[k] !== undefined) r[k] = o[k]; return r; }

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
      instructions: 'MCP de PES 2021 de Phoenix Evolution. Empieza por pes_estado. Antes de investigar algo nuevo usa pes_hallazgos_buscar. Todo lo de lectura es seguro; las acciones están apagadas salvo que config.json diga lo contrario. Otros hilos también usan PES: no abras ni cierres el juego por tu cuenta.',
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
  // Prueba rápida sin Claude: imprime la lista de herramientas y pes_estado.
  console.log(TOOLS.map((t) => t.name).join(', '));
  console.log((await TOOLS[0].run({})).content[0].text);
} else {
  const rl = createInterface({ input: process.stdin });
  rl.on('line', (l) => { if (!l.trim()) return; let m; try { m = JSON.parse(l); } catch { return responder(null, null, { code: -32700, message: 'JSON inválido' }); } atender(m); });
}
