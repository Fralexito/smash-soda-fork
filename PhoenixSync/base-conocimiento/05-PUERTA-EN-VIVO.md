# 05 · La «puerta en vivo»: cambiar la base del juego sin reiniciar

**Descubierto el 2026-10-09 (madrugada, Lima).** Probado en el juego de Fralex (ConmeGOL Patch 26, Sider 7.3.3).
Diario completo de cada prueba: `../PRUEBAS.md` (Pruebas 1–4 de Sider). Este documento resume **qué se aprendió, qué abre y qué queda para el futuro**.

---

## 1. El método (✅ PROBADO)

1. **Nuestro libro.** Phoenix Sync genera un archivo de la base (por ejemplo, `Player.bin`):
   - parte de la base que sirve el parche (hoy `olmosjr23\Database`), descomprimida;
   - le aplica los cambios;
   - lo vuelve a comprimir como Konami: cabecera WESYS de 16 bytes (`ff 10 81 57 'WESYS'`, tamaño comprimido u32, tamaño real u32) + zlib.
   - La compresión no tiene que ser idéntica a la original: el juego acepta zlib nivel 9.
2. **Sider lo entrega primero.** Se pone en la raíz `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\` y en `sider.ini` va la línea `cpk.root = ".\livecpk\Phoenix-DB"` **antes** de `cpk.root = ".\olmosjr23\Database"`. Gana el primero que tenga el archivo.
   - Va en la raíz del juego **y** en la carpeta del modo (`ConmeGol Extras\<modo>\SiderAddons`), porque el switcher copia esa carpeta encima con robocopy.
3. **Con el juego abierto** se reemplaza el archivo de forma atómica (`.tmp` y luego renombrar).
4. **En el juego: Editar → Cargar** (cargar EDIT00000000, **sin guardar**).
5. **En el siguiente partido o pantalla nueva** se ven los cambios.

**Prueba:** Lamine Yamal (ID 162114), Velocidad 99 → 95 → 90. Cada lectura del juego se identificó con una «huella» (suma cada 64 bytes del trozo que entrega Sider por `livecpk_read`).

## 2. Lo que NO funciona (para no repetirlo)

| Intento | Resultado | Motivo |
|---|---|---|
| Leer memoria directo (`memory.read`, búsqueda por regiones a lo largo de varios cuadros) | ❌ crash 0xC0000005 en sider.dll | Otros hilos (ReShade, DLSS) liberan memoria en cualquier momento. **Regla:** copiar con `ReadProcessMemory`, que falla sin cerrar el juego. |
| Buscar la ficha exacta de 58 bytes | ❌ 0 coincidencias | El juego cambia bits al cargar (forma, lesión). Hay que buscar por ID y verificar cualidades. |
| Escribir la Velocidad en las copias de memoria (`WriteProcessMemory`) | ❌ la pantalla no cambia | Las copias de 312 bytes se rehacen desde el archivo; la pantalla lee otra representación. |
| «Datos Actual. en vivo» (actualización nativa de Konami) | ❌ | «Los servicios en línea finalizaron el 25/08/2022»: depende del servidor de Konami. |
| Solo entrar y salir de Editar | 🟡 lee la base y la descarta | Editar trabaja con un borrador propio. Hace falta **Cargar**. |
| Shift+R (recargar módulos de Sider) | 🟡 | Solo recarga módulos que ya estaban activos. Uno que falló al iniciar necesita reiniciar el juego. |

**Otras lecciones del Lua de Sider 7.3.3:**
- **No hay `pcall`.** Si `init` falla, el módulo no se activa. Para contar errores se usa una «bandera».
- **`ffi.cdef` no deja redeclarar.** Tras Shift+R el Lua es el mismo, así que se usan nombres con versión (`phx06_*` + `__asm__("NombreReal")`).
- **`ctx.sider_dir`** termina en `\`.

## 3. Cuándo lee el juego cada archivo de `pesdb` (espía `livecpk_read`)

- **Al arrancar:** todos, dos veces (arranque y «Datos del sistema: Cargando»).
- **Al entrar a Editar y al pulsar Cargar:**
  - **sí relee:** Player, PlayerAssignment, Team, Coach, Competition, CompetitionEntry, CompetitionRegulation, Country, Stadium, Boots, Glove;
  - **no relee siempre:** Tactics, PlayerWeekly y otros. Revisar en cada prueba.
- **Al entrar a la Liga Máster:** lee `InstallVersionPlayer.bin`, pero no Player.bin.
- **No relee:** al abrir la pantalla de habilidades, al armar un amistoso ni al cambiar «Datos Actual. en vivo».

## 4. El universo que abre

| Archivo que se relee | Qué permitiría cambiar en vivo | Estado |
|---|---|---|
| `Player.bin` | Stats, habilidades, posiciones, altura, edad, nacionalidad | ✅ Probado (Velocidad) |
| `PlayerAssignment.bin` | ❌ **No sirve para fichajes en vivo**: el juego lo lee, pero las plantillas las manda el **option file** | ❌ Probado 04:41 |
| **Option file `EDIT00000000`** (plantillas, 284 B por equipo) | **Fichajes y traspasos en vivo**: `OptionFile::mover` + Editar → Cargar | ✅ Probado (Lamine → Real Madrid, dorsal 26) |
| `Coach.bin` | Cambios de entrenador | 🟢 Por probar |
| `Team.bin` | Datos de los clubes | 🟢 Por probar |
| `CompetitionEntry.bin` | Ascensos, descensos, qué equipos juegan cada liga | 🟢 Por probar (ojo: los módulos CGP comprueban equipos en ciertas competiciones) |
| `Stadium.bin`, `Boots.bin`, `Glove.bin` | Estadios, botines y guantes | 🟢 Por probar |
| `PlayerWeekly.bin` / `TeamWeekly.bin` | **Forma semanal propia** (las flechas), al estilo de la actualización en vivo de Konami | 🟡 Por investigar (la opción nativa pide el servidor) |

**Regla de oro (probada el 2026-10-09, 04:47):** las **stats** van por archivo de la base (`Player.bin` en Phoenix-DB); las **plantillas** (fichajes) van por el **option file**. Las dos se aplican con el mismo botón nativo: **Editar → Cargar**. Las stats de un jugador funcionan por archivo solo si **no está entre los jugadores editados** del option file (los editados mandan); Phoenix Sync debe comprobarlo antes de generar.

**Formato `PlayerAssignment.bin` (16 B):** `[índice u32][jugador u32][equipo u32][dorsal−1 u8][orden<<2 u8][banderas u8 (0x20 = capitán?)][0]`.

**Productos que salen de ahí:**
1. **Mercado de fichajes real:** fichajes confirmados en la vida real o hechos en la web, aplicados con Editar → Cargar.
2. **Phoenix Weekly:** stats ajustadas cada semana según el rendimiento real, con reglas fijas y aprobación de Fralex. Sin copiar notas de otros juegos.
3. **Liga online justa:** todos con el mismo archivo, verificado por huella SHA-256.
4. **Eventos:** jugador de la semana, castigos por lesión o suspensión, homenajes.
5. **Liga Máster conectada:** las habilidades de la LM salen de Player.bin (no están en el guardado). Por confirmar en la carrera.

## 5. Para el futuro (deseos de Fralex, aún no son prioridad)

- **Crear jugadores que no existen** (juveniles, jugadores reales que faltan, jugadores inventados para la liga).
  - **Hipótesis:** agregar registros nuevos (312 bytes, con un ID libre) a `Player.bin` y su fila en `PlayerAssignment.bin`.
  - **Falta investigar:** rango de IDs seguros, límites de tamaño, `PlayerDeleteList.bin`, `InstallVersionPlayer.bin` y cómo reacciona la Liga Máster.
  - **Caras:** carpeta `face\real\<ID>\` y minicara `symbol\player\<ID>.dds`. Fralex quiere aprender a hacer faces; Claude le dará el ID exacto y la carpeta de cada jugador.
- **Avisos con aspecto nativo:** imágenes de los menús generadas por Phoenix Link y servidas por Sider (por ejemplo, una tarjeta «FICHAJE: …» con la cara del jugador), en lugar del overlay de Sider.

## 6. Cuidados obligatorios

- **Cuando el parche se actualiza** (ConmeGOL o SP), la raíz Phoenix-DB **tapa** su base: hay que **regenerar** nuestros archivos desde la base nueva (Auditor de parches) **antes** de jugar. Si no, se servirían datos viejos.
- **No pulsar «Guardar» en Editar** durante las pruebas.
- **Nunca escribir memoria** durante un partido. El método por archivo no la necesita.
- **Respaldos en el PC de Fralex:** `sider.ini.respaldo-phoenix-20261009` (módulo phoenix.lua), `sider.ini.respaldo-phoenixdb-20261009` (raíz Phoenix-DB) y `EDIT00000000.respaldo-fichaje-20261009` (option file antes del fichaje de Lamine, en `save\` y en `_PhoenixMercado_prueba\db\`). Variantes de prueba en `_PhoenixMercado_prueba\db\`.
- **La carpeta de guardado activa** es `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
- **Estado del option file al cerrar la prueba:** Lamine Yamal sigue en el Real Madrid (dorsal 26) hasta que Fralex pida volver al respaldo.
- **Estado al cerrar la prueba:** Phoenix-DB contiene v90, que son los datos originales (solo recomprimidos).
