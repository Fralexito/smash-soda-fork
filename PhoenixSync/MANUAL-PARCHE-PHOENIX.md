# 📘 Manual del Parche Phoenix Evolution (PES 2021)

**Versión del manual:** 2026-10-09 (madrugada, Lima) · **Autor del proyecto:** FRALEX · Redactado con Claude.

**Para qué sirve:** reúne **todo lo aprendido y probado** para construir el **Phoenix Evolution Patch**: qué es cada pieza, cómo funciona el juego por dentro, las recetas paso a paso, los errores que no hay que repetir y la ruta para fabricarlo.

**Regla de lectura:** si algo de aquí choca con un documento más viejo, **manda este manual**. Lo más nuevo y detallado está en `base-conocimiento/05-PUERTA-EN-VIVO.md` y en el diario `PRUEBAS.md`.

**Leyenda:**
- ✅ **probado en el juego**;
- 🟢 **se puede**: mismo método ya probado, falta hacerlo;
- 🟡 **con investigación**: el camino es conocido;
- 🔴 **frontera**: nadie lo ha hecho;
- ❌ **no funciona**.

---

## 1. La visión

El **Phoenix Evolution Patch** quiere ser el parche de PES 2021 **más completo, estable y conectado** que exista: un solo sistema con todo el ecosistema de FRALEX.

| Pieza | Qué es | Dónde vive |
|---|---|---|
| **Web Phoenix Evolution** | Liga (Galaxy League), mercado de fichajes, perfiles, buzón de avisos | repo `Fralexito/phoenixevolution` (rama `borrador`) + Supabase |
| **Phoenix Link** | App de PC: salas para jugar a distancia por Parsec (fork de Smash Soda 7.0.3) y **cartero** web ⇄ juego | repo `Fralexito/smash-soda-fork`, rama `rediseño-phoenix-portal`, carpeta `SmashSoda/` |
| **Phoenix Sync** (antes «Phoenix Mercado») | Programa que **mete en el juego lo que dice la web**: option file, Liga Máster y **base de datos** | mismo repo, rama `mercado-fase0`, carpeta `PhoenixSync/` |
| **Módulo Sider `phoenix.lua`** | Código Phoenix **dentro del juego**: muestra avisos y espía lecturas | `PhoenixSync/sider/phoenix.lua` |
| **El parche** | Base de datos + contenido + `sider.ini` limpio + lanzador | por construir (ver §7) |

**El camino de los datos:**

```
WEB (Supabase) ⇄ PHOENIX LINK (cartero, internet) ⇄ archivos en la PC ⇄ SIDER (dentro del juego) ⇄ PES 2021
                                    ↑
                         PHOENIX SYNC (fabrica los archivos: option file, Liga Máster, base de datos)
```

**El juego nunca toca internet:** todo lo online pasa por Phoenix Link. Sider no tiene red, y eso lo hace más seguro.

---

## 2. Cómo funciona PES 2021 por dentro (lo esencial)

### 2.1 Capas del juego
1. **El ejecutable** `PES2021.exe` (1.07.02 + Data Pack 7.0). Las reglas «cableadas» (tamaños de liga, IA, físicas) solo se cambian parcheando código. 🔴
2. **Paquetes CPK** listados en `DpFileList.bin` (carpeta `download`). Es el modelo clásico; lo usa Sudamerican.
3. **Sider** (`SiderAddons\sider.exe`, `sider.dll`, `sider.ini`). Entrega archivos sueltos al juego (**livecpk**) y corre módulos Lua. Lo usa ConmeGOL para casi todo.
   - **Gana la primera raíz** `cpk.root` que tenga el archivo pedido.
4. **Carpeta `save`**: `EDIT00000000` (option file, cifrado), `ML0000000N` (Liga Máster, ranura N = archivo N−1 en hexadecimal) y `SYSTEM00000000`.

### 2.2 La base de datos (`common\etc\pesdb\*.bin`)
- Cada archivo va comprimido con cabecera **WESYS**: 16 bytes = `ff 10 81 57` + `"WESYS"` + tamaño comprimido u32 + tamaño real u32, seguidos del contenido en **zlib**.
  - **El juego acepta una recompresión distinta** (zlib nivel 9). ✅
- **Archivos principales:** `Player.bin` (fichas de 312 B, ID u32 en +8), `PlayerAssignment.bin` (16 B: jugador en +4, equipo en +8), `Team.bin` (1532 B, ID en +8), `Competition.bin` (36 B), `CompetitionEntry.bin` (12 B), `Coach.bin`, `Stadium.bin`, `Country.bin`, `Tactics*.bin`, `PlayerWeekly.bin` y `TeamWeekly.bin` (forma semanal), `Boots.bin`, `Glove.bin`, `Derby.bin`…
- **Límites del juego:** 750 equipos, 30.000 jugadores, 40 por plantilla. No cambiar el tamaño de una liga sin parchear el exe, porque cuelga la Liga Máster.

### 2.3 Cuándo lee el juego la base (✅ medido con el espía `livecpk_read`, 2026-10-09)
| Momento | ¿Relee `pesdb`? |
|---|---|
| Al arrancar | Sí, todo, dos veces (arranque y «Datos del sistema: Cargando») |
| **Al entrar a Editar** | Sí (Player, PlayerAssignment, Team, Coach, Competition…), pero **en un borrador** que se descarta al salir |
| **Editar → Cargar** | Sí, **y lo aplica al juego** ✅ |
| Al entrar a la Liga Máster | Solo `InstallVersionPlayer.bin` |
| Al abrir pantallas de jugadores, armar un amistoso o cambiar «Datos Actual. en vivo» | No |

### 2.4 Mapa de `Player.bin` (registro de 312 B; bits desde el inicio; 6 bits = valor − 40)
- **19 habilidades de campo** ✅:
  - ataque 370, control 281, regate 352, posesión estrecha 416;
  - pase raso 263, pase bombeado 402, finalización 396, cabezazo 288;
  - balón parado 250, efecto 332;
  - velocidad 306, aceleración 344, equilibrio 376, contacto físico 390, salto 294, resistencia 338;
  - defensa 275, recuperación 312, agresividad 384.
- **Potencia de tiro** 358 ✅ (calibrada con la pantalla del juego).
- **Físico y datos** ✅:
  - altura 216 (8 bits, +100);
  - peso 256 (7 bits, +30);
  - edad 408 (6 bits, +15);
  - posición principal 434 (4 bits);
  - nacionalidad 233 (9 bits → `Country.bin`).
- **Portero**: bits 269, 300, 320, 326, 364 🔎. Falta saber cuál es cuál: hace falta una foto de la ficha de Joan García.
- **Posiciones jugables** (2 bits: 0 no, 1 B, 2 A) 🔎:
  - PT 350, DFC 468, LI 318, LD 474;
  - MCD 414, MC 456, MI 466, MD 460, MO 464;
  - EI 472, ED 476, SD 478, DC 470.
- **Pie malo** uso 454 / precisión 462, **forma** 438, **lesiones** 458 ❓. El juego **varía** los bytes 54-57 al cargar.
- **Habilidades especiales y estilos COM**: bits 480–531 ❓.
- **Nombres** dentro de la ficha (en mayúsculas en el byte 129 y otra vez en el 190; en formato normal en el 251).

**Imágenes:**
- minicaras: `livecpk\Logos\common\render\symbol\player\<ID>.dds`;
- escudos: `…\symbol\flag\e_<6 dígitos>_r_l.png`;
- caras 3D: carpeta `face\real\<ID>\`.

### 2.5 La base viva de ConmeGOL
El juego **no** usa `CGP_database.cpk`. Usa la base que sirve Sider desde `SiderAddons\olmosjr23\Database` (5/9/2026). El catálogo de la web hay que regenerarlo desde ahí.

---

## 3. Las piezas que YA funcionan

| Capacidad | Estado | Cómo |
|---|---|---|
| Mover jugadores en el option file (alineación y dorsales correctos) | ✅ | `PhoenixSync.exe mover` / `sincronizar` |
| Liga Máster: vender, traspasos de la IA, **fichar para el usuario**, presupuesto y tope salarial | ✅ | Phoenix Sync, con el juego en el menú (el juego relee el guardado al cargar la ranura) |
| Cambios firmados desde la web (Ed25519), aplicados todo o nada | ✅ | `/liga/cambios` |
| **Avisos web → juego en vivo** (1-2 s) | ✅ | Web → Phoenix Link → `content\phoenix\avisos.txt` → `phoenix.lua` → overlay de Sider |
| **Base de datos propia servida por Sider** (stats cambiadas al arrancar) | ✅ | Raíz `livecpk\Phoenix-DB` antes de `olmosjr23\Database` |
| **Cambiar stats sin reiniciar** | ✅ | Archivo nuevo + **Editar → Cargar** |
| Leer la memoria del juego sin riesgo | ✅ | `ReadProcessMemory` sobre el propio proceso (copia segura) |
| Página «Vestuario» con la plantilla editable | ✅ prototipo | artifact con base de datos; los cambios quedan «pendientes» |
| 295/295 pruebas automáticas de Phoenix Sync | ✅ | `PhoenixSyncPruebas.exe` |

---

## 4. Recetas paso a paso

### R1 · Cambiar stats de un jugador en vivo ✅
1. **Phoenix Sync** toma la base que sirve el parche (`olmosjr23\...\Player.bin`) y la descomprime.
2. Busca la ficha por ID (u32 en +8) y cambia **solo** los bits del campo, conservando los demás bits del byte.
3. Recomprime con WESYS + zlib. Verifica de ida y vuelta (descomprimir y comparar).
4. Escribe en `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\Player.bin` de forma **atómica** (`.tmp` y luego renombrar), en la raíz del juego **y** en la carpeta del modo.
5. En el juego: **Editar → Cargar** (sin guardar).
6. El siguiente partido o pantalla nueva muestra el cambio.

**Comprobación:** el espía de `phoenix.lua` anota cada lectura de `player.bin` con una **huella** (suma cada 64 bytes del trozo leído). Compárala con la del archivo.

### R2 · Mandar un aviso al juego ✅
- **Desde la web:** tarjeta «Mandar aviso a mi juego». Phoenix Link revisa cada 15 s con el juego abierto y escribe `avisos.txt`.
- **A mano:** escribir `SiderAddons\content\phoenix\avisos.txt` (UTF-8 sin BOM, máx. 14 líneas, de forma atómica).
- **En el juego:** barra espaciadora → tecla 1 hasta «PHOENIX EVOLUTION».

### R3 · Instalar algo en un parche con «switcher» ✅
- El switcher de ConmeGOL (`PES2021 Start.exe`, Python con PyInstaller) hace `robocopy "ConmeGol Extras\<modo>" "<juego>" /E /IS /IT`, escribe `version_actual.txt`, abre `sider.exe` y, 2 s después, `PES2021.exe`.
- **Por eso todo va en `ConmeGol Extras\<modo>\SiderAddons`**; si no, se pierde. Lo que se cambia **en vivo** se escribe en la raíz del juego.

### R4 · Probar un módulo Sider sin romper nada ✅
1. **Sintaxis:** `luajit -bl modulo.lua`.
2. **Simulación con los globales exactos de Sider 7.3.3:** assert, pairs, ipairs, tostring, tonumber, type, error, unpack, collectgarbage, table, string, math, os, io, fs, ffi, jit, bit, zlib, memory, match, audio, input, log. **No hay `pcall`.**
3. Memoria y ffi falsos para probar búsquedas y escrituras, incluida una zona que «desaparece» a mitad de la prueba.
4. **Instalar con respaldo** de `sider.ini` y **una sola línea** nueva. Comprobar con `diff`.
5. **Ver `sider.log`:** «Module (x) is NOT activated» explica cualquier fallo de arranque.

### R5 · Día de actualización de un parche (ConmeGOL / Sudamerican)
1. **Antes:** copia de la carpeta `save`.
2. **Comparar con la foto del 8/10** (`_PhoenixMercado_prueba\_versiones\2026-10-08\`): base, DpFileList, Sider y option file.
3. **Regenerar `livecpk\Phoenix-DB`** desde la base nueva. **Si no, tapa los datos nuevos del parche.**
4. **Auditar** solo los módulos Sider nuevos o cambiados.
5. **Regenerar el catálogo** y subirlo a la web.
6. **Probar** y anotar en `PRUEBAS.md`, `parches/<parche>.md` y Drive.

### R6 · Operaciones de Liga Máster ✅
- Siempre con el juego **en el menú** (la ranura sin cargar) y con copia de seguridad.
- El juego relee el guardado al entrar a la ranura.
- Detalle técnico: `liga-master/ESTRUCTURA-ML.md` y `base-conocimiento/01-MOTOR-PES2021.md`.

---

## 5. Reglas de oro técnicas (aprendidas a golpes)

1. **Nunca leer ni escribir memoria del juego directo.** Otros hilos (ReShade, DLSS) la liberan en cualquier momento: crash 0xC0000005 comprobado. Hay que usar `ReadProcessMemory` / `WriteProcessMemory`, que fallan sin cerrar el juego.
2. **Mejor archivo que memoria.** El juego rehace sus copias desde la base, así que escribir en memoria no sirve para las stats. El camino es **archivo + Editar → Cargar**.
3. **El Lua de Sider no trae `pcall`.** Un error en `init` deja el módulo apagado. Para contar errores se usa una «bandera».
4. **`ffi.cdef` no deja redeclarar.** Con Shift+R el Lua es el mismo, así que se usan nombres con versión (`phx06_*` + `__asm__("FunciónReal")`).
5. **Shift+R solo recarga módulos que ya estaban activos.**
6. **Buscar fichas por ID y verificar sus cualidades.** El juego varía bits (forma, lesión), así que una ficha nunca coincide byte a byte.
7. **El switcher pisa la raíz.** Lo permanente va en la carpeta del modo.
8. **La actualización en vivo de Konami ya no existe:** sus servidores cerraron el 25/08/2022.
9. **Respaldos, siempre.** Un cambio = una línea en `REGISTRO.md`. Cada prueba en el juego se anota en `PRUEBAS.md`, salga bien o mal.
10. **0 errores:** si algo falla, no se avanza hasta entenderlo y arreglarlo. «Si algo se rompe, mejor no lo hagas.»
11. **Contenido de terceros** (caras, kits, estadios, módulos) solo con **permiso y crédito**. Lo propio (base, programas, conexión) es de FRALEX. **Nunca se distribuye el juego.**

---

## 6. El universo que abrió la «puerta en vivo»

| Qué | Archivo | Estado |
|---|---|---|
| Stats y habilidades | `Player.bin` | ✅ |
| **Fichajes y traspasos en vivo** | `PlayerAssignment.bin` | 🟢 (siguiente experimento) |
| Entrenadores | `Coach.bin` | 🟢 |
| Equipos | `Team.bin` | 🟢 |
| Ascensos, descensos, participantes de cada liga | `CompetitionEntry.bin` | 🟢 (ojo: los módulos CGP comprueban equipos por competición) |
| Estadios, botines y guantes | `Stadium.bin`, `Boots.bin`, `Glove.bin` | 🟢 |
| Forma semanal propia | `PlayerWeekly.bin` / `TeamWeekly.bin` | 🟡 |
| Habilidades en la Liga Máster | `Player.bin` (la LM no las guarda aparte) | 🟢 por confirmar en una carrera |

**Productos posibles:**
1. **Mercado real conectado:** un fichaje en la web o en la vida real aparece en el juego.
2. **Phoenix Weekly:** stats según el rendimiento real, con reglas fijas y aprobación de FRALEX. **Sin copiar notas de otros juegos.**
3. **Liga online justa:** todos con el mismo archivo, verificado por huella SHA-256.
4. **Eventos:** jugador de la semana, lesiones o suspensiones, homenajes.
5. **Liga Máster conectada** a la web.

---

## 7. Ruta para construir el parche (actualizada)

| Fase | Qué | Estado |
|---|---|---|
| **0. Terreno** | Instalación limpia de referencia (¿PES 2021 de Steam?), exe 1.07.02 + DP 7.0, Sider fijado, partido de referencia | ⏳ |
| **1. Núcleo** | Base de datos Phoenix (Liga 1 + Liga 2 + Copa; lo mejor de ConmeGOL y Sudamerican **con permiso**), option file oficial de la web, `sider.ini` limpio y auditado | ⏳ (la técnica de base propia ya está ✅) |
| **2. Herramientas** | Phoenix Sync fabrica Player/PlayerAssignment/… (WESYS), CPK y DpFileList; **Phoenix Doctor** (auditoría automática de Sider); **Auditor de parches**; lanzador sin `.bat` | 🟢 en parte |
| **3. Conexión** | Avisos ✅ → cambios de stats en vivo ✅ → **fichajes en vivo** 🟢 → resultado automático (`match.stats()` → `resultado.json` → Link → web) 🟢 → verificador del anfitrión 🟢 | En marcha |
| **4. Contenido** | Addons por liga (caras, kits, estadios) con permisos y créditos; menos archivos sueltos; texturas bien comprimidas | ⏳ |
| **5. Más allá** | Competiciones propias 🟡, estadísticas completas 🟡, **Liga Máster online asíncrona** 🔴 (la idea más original), crear jugadores 🟡, avisos con aspecto nativo 🟡 | Futuro |

**Inmediato (para la próxima sesión):**
1. Fichaje en vivo con `PlayerAssignment.bin` + Editar → Cargar.
2. Confirmar si los cambios de stats llegan a la Liga Máster.
3. Automatizar: Vestuario web → Phoenix Sync arma el archivo → Phoenix Link lo coloca → aviso «Editar → Cargar». Los prompts para LINK y WEB están por escribir.
4. Calibrar el portero, el pie malo, la forma y las lesiones con fotos de la ficha de Joan García.
5. Actualización de ConmeGOL: aplicar la receta R5 y regenerar `Phoenix-DB`.

---

## 8. Futuro (deseos de FRALEX, guardados para no olvidarlos)

- **Crear jugadores que no existen** (juveniles, jugadores reales que faltan, jugadores de la liga).
  - **Hipótesis:** fichas nuevas en `Player.bin` (ID libre) + su fila en `PlayerAssignment.bin`.
  - **Falta investigar:** rango de IDs seguros, `PlayerDeleteList.bin`, `InstallVersionPlayer.bin` y el efecto en la Liga Máster.
- **Hacer faces:** FRALEX quiere aprender. Claude le dará el **ID exacto** de cada jugador y **la carpeta** (`face\real\<ID>\`, minicara `symbol\player\<ID>.dds`), revisará los nombres de archivo y lo instalará en una raíz Phoenix sin pisar las del parche.
- **Avisos con aspecto nativo:** imágenes de los menús generadas por Phoenix Link y servidas por Sider, en lugar del overlay.
- **Botones nativos en vez de Sider:** el overlay es solo de laboratorio. En la versión final todo se aplica solo, y el jugador usa la web, Phoenix Link y un único botón del juego (Editar → Cargar).
- **PES online propio:** Phoenix Link (Parsec) + verificador + resultado automático. Un servidor propio estilo PESBUL es 🔴 (meses o años).
- **Liga Máster online asíncrona** 🔴: cada amigo con su club y su carrera, compartiendo mercado y tabla en la web.

---

## 9. Dónde está todo

| Qué | Dónde |
|---|---|
| Este manual | `PhoenixSync/MANUAL-PARCHE-PHOENIX.md` (y en Drive) |
| Base de conocimiento | `PhoenixSync/base-conocimiento/` (00-05, `parches/`, `investigacion/`, `datos/`) + PDF `base-conocimiento-PES21.pdf` |
| Diario de pruebas en el juego | `PhoenixSync/PRUEBAS.md` |
| Bitácora de cambios | `PhoenixSync/REGISTRO.md` (Sync) · `REGISTRO-LINK.md` (Link, en su rama) · `REGISTRO.md` del repo web |
| Sider | `PhoenixSync/sider/` (`phoenix.lua`, `VINCULO-TIEMPO-REAL.md`, `RIESGOS-SIDER.md`, `AUDITORIA-SIDER.md`) |
| Prompts para los otros chats | `PhoenixSync/prompts/` · traspaso completo: `PhoenixSync/PROMPT-PARCHE-PHOENIX.md` |
| Coordinación entre chats y cuentas | `COORDINACION.md`, `CLAUDE.md` |
| Juego ConmeGOL | `D:\Frank\Games_\Conmegol Patch\` (Sider en `SiderAddons\`, modos en `ConmeGol Extras\`) |
| Juego Sudamerican | `D:\SP2026\` |
| Pruebas en la PC | `…\Conmegol Patch\_PhoenixMercado_prueba\` (exe de Sync, `.bat`, `db\` con las variantes v90/v95/v99, `_versiones\2026-10-08\`) |
| Respaldos de Sider | `sider.ini.respaldo-phoenix-20261009` y `sider.ini.respaldo-phoenixdb-20261009` (en la raíz y en la carpeta del modo) |
| Google Drive | carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI` |
| Página Vestuario Barça | https://claude.ai/artifact/WskFnXgRmMhAs12Mv6cwTh |

---

## 10. Créditos y respeto

- **ConmeGOL Patch** y **Sudamerican Project:** referencias de estudio. Su contenido (caras, kits, estadios, módulos) es de sus autores (Olmos Jr 23, facemakers, kitmakers…). Para usarlo en Phoenix: **permiso + crédito**.
- **Herramientas de la comunidad** (Sider de juce, libpesXcrypter, 4ccEditor, pes-file-tools, módulos Lua como CommonLib de zlac): se respetan sus licencias y se citan.
- **Lo propio del Phoenix Evolution Patch** (base de datos, Phoenix Sync, Phoenix Link, el módulo Phoenix y la conexión con la web): **de FRALEX**.
