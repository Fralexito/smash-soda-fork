# Base de conocimiento · Datos en vivo, Player.bin, «Datos Actual. en vivo» y modos de recarga

Cómo se abrió la puerta de los datos en vivo:
- el guardado;
- la ficha de Player.bin;
- dónde está la opción «Datos Actual. en vivo» y su paso a paso;
- en qué zona del juego es seguro recargar;
- el experimento B;
- los tres modos de recarga de `phoenix.lua` v0.18.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 8 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · 05 · La «puerta en vivo»: cambiar la base del juego sin reiniciar](#parte-1-05-la-puerta-en-vivo-cambiar-la-base-del-juego-sin-reiniciar) — origen `PhoenixSync/base-conocimiento/05-PUERTA-EN-VIVO.md` (9444d91 2026-10-09)
- [Parte 2 · 10 · Cómo se abrió el guardado de la Liga Máster, paso a paso](#parte-2-10-como-se-abrio-el-guardado-de-la-liga-master-paso-a-paso) — origen `PhoenixSync/base-conocimiento/10-COMO-SE-ABRIO-EL-GUARDADO.md` (50f5f95 2026-10-09)
- [Parte 3 · 16 · Player.bin — ficha completa de un jugador (calibrada con capturas del juego)](#parte-3-16-playerbin-ficha-completa-de-un-jugador-calibrada-con-capturas-del-juego) — origen `PhoenixSync/base-conocimiento/16-PLAYER-BIN-FICHA-COMPLETA.md` (8f50132 2026-10-10)
- [Parte 4 · 17 · «Datos Actual. en vivo»: dónde guarda el juego si la opción está activada](#parte-4-17-datos-actual-en-vivo-donde-guarda-el-juego-si-la-opcion-esta-activada) — origen `PhoenixSync/base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` (bea741d 2026-10-09)
- [Parte 5 · 19 · «Datos Actual. en vivo»: informe paso a paso de todo lo que se hizo](#parte-5-19-datos-actual-en-vivo-informe-paso-a-paso-de-todo-lo-que-se-hizo) — origen `PhoenixSync/base-conocimiento/19-OPCION-EN-VIVO-PASO-A-PASO.md` (15a9ed6 2026-10-09)
- [Parte 6 · 22 · Fase D: en qué parte del juego está el jugador y cuándo es seguro recargar](#parte-6-22-fase-d-en-que-parte-del-juego-esta-el-jugador-y-cuando-es-seguro-recargar) — origen `PhoenixSync/base-conocimiento/22-ZONA-DEL-JUEGO-MOMENTO-SEGURO.md` (cea786b 2026-10-10)
- [Parte 7 · 30 · Experimento B (phoenix.lua v0.17e-B) — cómo replicarlo desde cero](#parte-7-30-experimento-b-phoenixlua-v017e-b-como-replicarlo-desde-cero) — origen `PhoenixSync/base-conocimiento/30-EXPERIMENTO-B-COMO-REPLICAR.md` (97febd2 2026-10-10)
- [Parte 8 · 31 · phoenix.lua v0.18 — los tres modos de recarga: dónde, cómo y cuándo se aplica cada uno](#parte-8-31-phoenixlua-v018-los-tres-modos-de-recarga-donde-como-y-cuando-se-aplica-cada-uno) — origen `PhoenixSync/base-conocimiento/31-MODOS-DE-RECARGA.md` (0387464 2026-10-10)

---

## Parte 1 · 05 · La «puerta en vivo»: cambiar la base del juego sin reiniciar

> Origen: `PhoenixSync/base-conocimiento/05-PUERTA-EN-VIVO.md` · último cambio 9444d91 2026-10-09 · 8057 bytes. Copia íntegra.

**Descubierto el 2026-10-09 (madrugada, Lima).** Probado en el juego de Fralex (ConmeGOL Patch 26, Sider 7.3.3).
Diario completo de cada prueba: `../PRUEBAS.md` (Pruebas 1–4 de Sider). Este documento resume **qué se aprendió, qué abre y qué queda para el futuro**.

---

### 1. El método (✅ PROBADO)

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

### 2. Lo que NO funciona (para no repetirlo)

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

### 3. Cuándo lee el juego cada archivo de `pesdb` (espía `livecpk_read`)

- **Al arrancar:** todos, dos veces (arranque y «Datos del sistema: Cargando»).
- **Al entrar a Editar y al pulsar Cargar:**
  - **sí relee:** Player, PlayerAssignment, Team, Coach, Competition, CompetitionEntry, CompetitionRegulation, Country, Stadium, Boots, Glove;
  - **no relee siempre:** Tactics, PlayerWeekly y otros. Revisar en cada prueba.
- **Al entrar a la Liga Máster:** lee `InstallVersionPlayer.bin`, pero no Player.bin.
- **No relee:** al abrir la pantalla de habilidades, al armar un amistoso ni al cambiar «Datos Actual. en vivo».

### 4. El universo que abre

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

### 5. Para el futuro (deseos de Fralex, aún no son prioridad)

- **Crear jugadores que no existen** (juveniles, jugadores reales que faltan, jugadores inventados para la liga).
  - **Hipótesis:** agregar registros nuevos (312 bytes, con un ID libre) a `Player.bin` y su fila en `PlayerAssignment.bin`.
  - **Falta investigar:** rango de IDs seguros, límites de tamaño, `PlayerDeleteList.bin`, `InstallVersionPlayer.bin` y cómo reacciona la Liga Máster.
  - **Caras:** carpeta `face\real\<ID>\` y minicara `symbol\player\<ID>.dds`. Fralex quiere aprender a hacer faces; Claude le dará el ID exacto y la carpeta de cada jugador.
- **Avisos con aspecto nativo:** imágenes de los menús generadas por Phoenix Link y servidas por Sider (por ejemplo, una tarjeta «FICHAJE: …» con la cara del jugador), en lugar del overlay de Sider.

### 6. Cuidados obligatorios

- **Cuando el parche se actualiza** (ConmeGOL o SP), la raíz Phoenix-DB **tapa** su base: hay que **regenerar** nuestros archivos desde la base nueva (Auditor de parches) **antes** de jugar. Si no, se servirían datos viejos.
- **No pulsar «Guardar» en Editar** durante las pruebas.
- **Nunca escribir memoria** durante un partido. El método por archivo no la necesita.
- **Respaldos en el PC de Fralex:** `sider.ini.respaldo-phoenix-20261009` (módulo phoenix.lua), `sider.ini.respaldo-phoenixdb-20261009` (raíz Phoenix-DB) y `EDIT00000000.respaldo-fichaje-20261009` (option file antes del fichaje de Lamine, en `save\` y en `_PhoenixMercado_prueba\db\`). Variantes de prueba en `_PhoenixMercado_prueba\db\`.
- **La carpeta de guardado activa** es `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
- **Estado del option file al cerrar la prueba:** Lamine Yamal sigue en el Real Madrid (dorsal 26) hasta que Fralex pida volver al respaldo.
- **Estado al cerrar la prueba:** Phoenix-DB contiene v90, que son los datos originales (solo recomprimidos).

---

## Parte 2 · 10 · Cómo se abrió el guardado de la Liga Máster, paso a paso

> Origen: `PhoenixSync/base-conocimiento/10-COMO-SE-ABRIO-EL-GUARDADO.md` · último cambio 50f5f95 2026-10-09 · 11012 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026.** Este informe cuenta solo **la apertura** del archivo de la Liga Máster (`ML0000000N`): cómo se pasó de un archivo «revuelto» a uno que se puede leer y editar, y cómo se volvió a cerrar para que el juego lo acepte.
Lo que se hizo **después** de abrirlo (fichajes, dinero, blob) está en `09-LIGA-MASTER-COMO-LO-HICIMOS.md`.

---

### 0. La idea con un ejemplo sencillo

Imagina que el guardado es **una carta dentro de un sobre cerrado con candado**.

- **El sobre** = el cifrado. Desde fuera solo se ven letras sin sentido.
- **La llave** = la clave de PES 2021. Es **la misma para todos los guardados** del juego (option file, Liga Máster, etc.).
- **La carta** = los datos de tu carrera (plantillas, dinero, contratos…).
- **La etiqueta del sobre** = el texto que ves en el menú *Cargar* (equipo, fecha, competición).

Abrir el guardado = **abrir el sobre con la llave, leer la carta, cambiarla y volver a meterla en un sobre igual**, con la misma etiqueta (o una nueva), para que el juego no note la diferencia.

---

### 1. Palabras que se usan aquí

| Palabra | Qué significa |
|---|---|
| Cifrar | Revolver los datos con una llave para que no se puedan leer |
| Descifrar | Usar la llave para dejarlos legibles otra vez |
| Byte | El trocito más pequeño de un archivo (un número del 0 al 255) |
| Cabecera | La primera parte del archivo, que dice qué es y cuánto mide cada pieza |
| Ida y vuelta | Abrir y volver a cerrar sin cambiar nada, y comprobar que sale idéntico |
| Huella (md5 / sha256) | Un código que identifica un archivo exacto. Si cambia un solo byte, cambia la huella |
| Compilar | Convertir un programa escrito en texto (código) en un programa que se puede ejecutar |

---

### 2. Las herramientas que se usaron

| Herramienta | Qué es | Dónde está |
|---|---|---|
| libpesXcrypter | Biblioteca libre y de dominio público (licencia *Unlicense*) hecha por la comunidad de PES. Sabe abrir y cerrar los guardados de PES. Trae la llave de PES 2021 | `PhoenixSync/terceros/pesxcrypter/` |
| gcc | Compilador de C (en la nube, Linux) | — |
| MinGW | El mismo compilador, pero produce programas para Windows (tu PC) | — |
| `dec` | Programa propio: abre el guardado, lo vuelve a cerrar y comprueba que salga idéntico | `liga-master/herramientas/dec.c` |
| `info` | Programa propio: lee la etiqueta y la cabecera, sin cambiar nada | `liga-master/herramientas/info.c` |
| `enc2` | Programa propio: cierra los datos editados en un sobre nuevo y puede cambiar la etiqueta | `liga-master/herramientas/enc2.c` |
| `SobrePes` | La versión definitiva dentro de Phoenix Sync (C++): abre, cierra con cualquier tamaño y nunca sobrescribe | `core/SobrePes.h` y `.cpp` |
| El enlace con tu PC | Copia archivos entre tu PC y la nube | App de escritorio de Claude |
| md5 / sha256 | Para comprobar que cada archivo es exactamente el que se cree | — |

> Importante: **no se tocó el juego** para abrir el guardado. Se usó una biblioteca pública que la comunidad de editores de PES usa desde hace años.

---

### 3. Paso a paso

#### Paso 1 · Darse cuenta de que ya teníamos la llave
- **Qué pasaba:** antes de la Liga Máster, Phoenix Sync ya abría el **option file** (`EDIT00000000`) para hacer fichajes.
- **Qué se pensó:** PES 2021 usa **el mismo tipo de sobre** para todos sus guardados. Si la llave abre el option file, debería abrir también la Liga Máster.
- **Herramienta:** la misma biblioteca libpesXcrypter que ya estaba en `terceros/pesxcrypter`.

#### Paso 2 · Traer una copia del guardado a la nube
- **Qué:** se copió `ML00000000` (tu carrera con el City) desde
  `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` a la nube.
- **Cómo:** con el enlace con tu PC (copia, no mueve). **El original nunca se tocó.**
- **Además:** se guardó una copia de respaldo en tu PC (`save\_referencias_mercado\`).

#### Paso 3 · Escribir y compilar `dec` (el abridor)
- **Qué hace, en orden:**
  1. Lee el archivo cifrado completo.
  2. Prepara un «sobre vacío» (en el código, `createFileDescriptorNew`).
  3. Lo abre con la llave de PES 2021 (`decryptWithKeyNew`).
  4. Muestra qué tipo de archivo es, la versión del juego y cuánto miden los datos.
  5. Guarda **los datos descifrados** en un archivo nuevo (`salida.bin`).
  6. **Vuelve a cerrar** esos datos con la misma llave (`encryptWithKeyNew`) en otro archivo nuevo.
  7. Compara el cerrado nuevo con el original **byte a byte** y dice si son idénticos.
  8. Lo abre una segunda vez y comprueba que los datos sean iguales a los primeros.
- **Cómo se compiló** (desde la carpeta de herramientas):
  ```
  gcc -O2 -o dec dec.c (+ los 3 archivos de la biblioteca: crypt.c, masterkey.c, mt19937ar.c)
  ```
- Lo mismo para `info` y `enc2`.

#### Paso 4 · Abrirlo por primera vez
- **Resultado:** se abrió sin problemas. Salieron **19.759.355 bytes** de datos legibles (unos 19,7 MB).
- **Ida y vuelta:** el archivo volvió a cerrarse **idéntico**, y los datos de la segunda apertura eran iguales a los de la primera. ✅
- **Qué significa:** la llave es correcta y el sobre se puede **abrir y cerrar sin dañar nada**.

#### Paso 5 · Entender las piezas del sobre (con `info`)
`info` mostró que el guardado tiene **5 piezas**, en este orden:

| Pieza | Tamaño | Qué es |
|---|---|---|
| Cabecera | fija | Tipo de archivo, versión del juego, tamaño de cada pieza y una huella |
| Descripción | 384 bytes | La «etiqueta» del guardado |
| Logo | 17.315 bytes | La imagen que acompaña al guardado |
| Datos | ~19,7 MB | Tu carrera (lo que nos interesa) |
| Serial | 44 bytes | Un código de identificación |

#### Paso 6 · Descubrir qué muestra el menú *Cargar*
- **Primera idea:** la etiqueta son los primeros 128 bytes de la descripción (un «nombre»). Se cambió para reconocer las ranuras de prueba.
- **Prueba en el juego:** ❌ el menú *Cargar* **no mostraba** ese nombre.
- **Segunda mirada con `info`:** a partir del byte 128 de la descripción hay un **texto de 3 líneas**:
  ```
  Equipo / Liga
  Fecha
  Competición
  ```
- **Prueba en el juego:** ✅ **eso es lo que muestra** el menú *Cargar*.
- **Arreglo:** `enc2` recibió un 5.º dato para cambiar la **primera línea** de ese texto. Así tus ranuras de prueba salían como «PRUEBA 4», «PRUEBA 5»…

#### Paso 7 · Escribir `enc2` (el que vuelve a cerrar)
- **Qué hace, en orden:**
  1. Vuelve a abrir el **original** (para reutilizar su cabecera, logo y serial).
  2. Cambia los datos por los **datos editados**.
  3. Si se pide, cambia el nombre de 128 bytes y el texto info.
  4. Lo cierra con la llave y lo guarda como **archivo nuevo** (nunca encima del original).
- **Uso típico:**
  ```
  enc2 <original> <datos_editados.bin> <ML_nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'
  ```

#### Paso 8 · La gran duda: ¿el juego acepta un sobre «rehecho»?
- **El riesgo:** la cabecera trae una **huella** del contenido. Al cerrar, la biblioteca **no la recalcula**. Si el juego la comprobara, rechazaría el archivo editado.
- **Prueba en el juego:** se cerró un guardado **con cambios** y lo cargaste.
- **Resultado:** ✅ **el juego lo aceptó**. No comprueba esa huella.
- **Qué significa:** se puede editar la carrera y el juego la carga normal.

#### Paso 9 · Mirar por dentro los datos ya abiertos
- Con los datos legibles se vio el comienzo:
  - una **cabecera interna de 80 bytes** (contiene el número 700, la cantidad de equipos);
  - después, los **700 bloques de equipo**, empezando en el byte 80 (`0x50`).
- **Detalle:** 32 bytes de esa cabecera interna cambian en cada guardado y **el juego no los valida**.
- Desde aquí empezó el trabajo de fichajes (ver el informe 09).

#### Paso 10 · El problema del tamaño
- **Qué se vio:** al fichar a Neymar, el guardado del juego **creció 3.662 bytes**. Y el «blob» (la zona comprimida) crece con la temporada.
- **El límite:** `enc2` solo sabía cerrar datos **del mismo tamaño** que el original.
- **Solución:** en el programa definitivo (`core/SobrePes.cpp`) se cambia el bloque de datos por uno nuevo **del tamaño que haga falta** y se **actualiza el tamaño en la cabecera** antes de cerrar.
- **Prueba:** fichajes y blob recomprimido (pruebas 17, 18 y 19) cargaron bien en el juego. ✅

#### Paso 11 · Hacerlo seguro para siempre (`SobrePes`)
El cerrador definitivo de Phoenix Sync sigue estas reglas:
1. **Comprueba el tipo** de archivo antes de abrir (que sea un `ML` y no otra cosa).
2. **Rechaza** tamaños imposibles.
3. **Nunca sobrescribe**: si el archivo de destino ya existe, se niega.
4. Escribe primero un **archivo temporal**, lo **vuelve a abrir para comprobarlo** y solo entonces le pone el nombre final.
5. Conserva la cabecera, el logo y el serial **del original**.

#### Paso 12 · Llevarlo a tu PC
- Las ranuras de prueba se devolvieron a tu carpeta `save` con el enlace con tu PC.
- **Siempre se anotó la huella** de cada archivo entregado. Así se supo después si el archivo era el nuestro o si el juego ya había guardado encima.
- Las pruebas de Windows se compilaron con **MinGW** y se corrieron en tu PC con `PROBAR2.bat`.

---

### 4. Resumen de resultados

| Qué | Resultado |
|---|---|
| Abrir el guardado con la llave de PES 2021 | ✅ |
| Abrir y cerrar sin cambios → archivo idéntico | ✅ |
| El juego acepta un guardado editado (sin recalcular la huella) | ✅ probado en el juego |
| Cambiar la etiqueta del menú *Cargar* | ✅ (texto info, no el nombre de 128 bytes) |
| Cerrar con un tamaño distinto | ✅ (`SobrePes`) |
| Nunca sobrescribir el original | ✅ |

---

### 5. Errores y cómo se arreglaron

| Error | Causa | Arreglo |
|---|---|---|
| El menú *Cargar* no mostraba la etiqueta de prueba | Se cambió el nombre de 128 bytes, que el menú no usa | Cambiar el texto info (desde el byte 128) |
| `enc2` no podía cerrar datos más grandes | Exigía el mismo tamaño que el original | `SobrePes` cierra con cualquier tamaño y corrige la cabecera |
| A veces `dec` se cerraba con error al final | Ocurre después de escribir los archivos (causa exacta sin investigar) | Se comprobó que los archivos salían completos (ida y vuelta idéntica). No afecta al resultado |
| El juego guardaba encima de la ranura de prueba | Autoguardado al terminar un partido | Anotar la huella de cada entrega |

---

### 6. Reglas que quedaron

1. **Nunca** trabajar sobre el original: siempre una copia.
2. **Siempre** hacer la ida y vuelta antes de editar: si no sale idéntico, no se sigue.
3. **Siempre** cerrar en un archivo **nuevo**.
4. **Siempre** anotar la huella de lo que se entrega.
5. Los guardados y los datos abiertos (`.bin`) **no se suben a GitHub**: son tus datos.

---

## Parte 3 · 16 · Player.bin — ficha completa de un jugador (calibrada con capturas del juego)

> Origen: `PhoenixSync/base-conocimiento/16-PLAYER-BIN-FICHA-COMPLETA.md` · último cambio 8f50132 2026-10-10 · 7185 bytes. Copia íntegra.

Fecha: 9 oct 2026 · Autor: Claude (cuenta del chat de Player.bin) para FRALEX · Estado: 🔎 OBSERVADO / ✅ CONFIRMADO con pantallas del juego
Complementa y amplía §J de `01-MOTOR-PES2021.md`. No sustituye nada.

### 0. Cómo leer este documento (clasificación por capa)

Cada dato lleva una etiqueta de **a qué parche aplica**. No hay que encasillarse en un solo parche:

| Etiqueta | Significa |
|---|---|
| 🧩 MOTOR | Es del motor de PES 2021 → vale para **TODOS** los parches (ConmeGOL, Sudamerican, Football Life, Evoweb, Gogosz, Phoenix). |
| 🅲 CONMEGOL 26 | Dato propio de ConmeGOL 26 (plantillas, tamaños, rutas, versión). |
| 🅢 SUDAMERICAN | Dato propio de Sudamerican Project 2026. |
| ⏳ PENDIENTE | Aún no comprobado en ese parche. |

Regla: el **formato** del registro (bits, tamaños) es 🧩 MOTOR (lo define el .exe). Los **valores** (qué jugador, qué stats) son del parche.

### 1. Archivo y registro 🧩 MOTOR (formato) · 🅲 (tamaños de ConmeGOL 26)

- Ruta viva en ConmeGOL 26: `D:\Frank\Games_\Conmegol Patch\SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin` (base real de olmosjr23; 5.894 jugadores difieren de `CGP_database.cpk`). 🅲
- Formato: cabecera WESYS 16 B + zlib. Descomprimido: **9.359.064 B = 29.997 registros × 312 B**. 🅲 (el tamaño de registro 312 B es 🧩)
- `pid` = u32 en el byte +8 del registro. 🧩
- Convención de bits: se lee el registro entero como número little-endian; campo = `(reg >> bit) & máscara`.
- Habilidades (stats): 6 bits; **valor mostrado = crudo + 40**. En pantalla puede diferir ±1 (ver §6).

### 2. Estadísticas (6 bits, valor = crudo + 40) — ✅ CONFIRMADO 🧩

| Stat en pantalla | Bit | Stat | Bit |
|---|---|---|---|
| Actitud ofensiva | 370 | Velocidad | 306 |
| Control del balón | 281 | Aceleración | 344 |
| Regate | 352 | Equilibrio | 376 |
| Conservación del balón (regate ajustado) | 416 | Contacto físico | 390 |
| Pase raso | 263 | Salto | 294 |
| Pase bombeado | 402 | Resistencia | 338 |
| Finalización | 396 | Actitud defensiva | 275 |
| Cabeceo | 288 | Recuperación de balón | 312 |
| Balón parado | 250 | Agresividad | 384 |
| Efecto | 332 | Potencia de tiro | 358 |

Portero (6 bits, +40): Despejar **269** · Cobertura **300** · Reflejos **320** · Actitud de portero **326** · Atajar **364**
(confirmado con Joan García, Livakovic, Szczęsny: 83/87/87/87/82).

### 3. Otros campos pequeños 🧩 — ✅ CONFIRMADO

| Campo | Bit | Bits | Nota |
|---|---|---|---|
| Uso del pie malo | 454 | 2 | el juego muestra guardado + 1 |
| Precisión del pie malo | 462 | 2 | idem |
| Forma | 438 | 3 | el juego muestra guardado + 1 |
| Resistencia a lesiones | 458 | 2 | idem |

### 4. Posiciones (2 bits: 0 = C, 1 = B, 2 = A) — ✅ CONFIRMADO 🧩

PT 350 · DFC 468 · LI 318 · LD 474 · MCD 414 · MC 456 · MI 466 · MD 460 · MO 464 · EI 472 · ED 476 · SD 478 · DC 470.
Confirmado con Isak, Mbappé, Dembélé, João Pedro (SD/DC en A, EI en B).

### 5. Habilidades especiales y COM (1 bit cada una, rango 480–531) 🧩

✅ Resueltas (bit → nombre que muestra el juego):

| Bit | Habilidad | Bit | Habilidad |
|---|---|---|---|
| 482 | Sombrero | 506 | Vaselina |
| 483 | Pase cruzado | 507 | Pase al primer toque |
| 484 y 487 | Pase en profundidad (dos bits, mismo texto) | 510 | COM Llegador |
| 485 | Elástica | 511 | Remate al primer toque |
| 486 | Espíritu de lucha | 518 | Tiro de larga distancia |
| 488 | Pase bombeado bajo | 519 | Bicicleta |
| 489 | COM Mago del balón | 520 | COM Cañonero |
| 490 | Patadón por bajo (PT) | 522 | Pase largo de portero |
| 491 | Picardía | 523 | Dos toques |
| 492 | Capitanía (por descarte) | 524 | Finalización acrobática |
| 493 | Centro con rosca | 525 | Rebote interior |
| 494 | Disparo descendente | 526 | COM Misil con el balón |
| 495 | Cabeceador | 527 | Cañonero (habilidad) |
| 496 | Patadón en largo (PT) | 528 | Amago por detrás y giro |
| 497 | Marsellesa | 529 | COM Experto en pases largos |
| 499 | Disparo ascendente | 530 | Despeje acrobático |
| 500 | Pisar el balón | 531 | COM As del eslalon |
| 501 | Especialista en penaltis | 502 | Parapenaltis |
| 503 | Interceptador | 504 | Marcaje |
| 505 | Espuela | | |

⏳ NO resueltas: **Cortada** (sale con los bits 514, 498/515 y a veces 480 según el jugador → probablemente el juego muestra un solo nombre para varias habilidades internas) · bit **514** encendido en Bernal sin mostrar nada · **COM Centrador** (sin bit) · grupo 479/516 «As en la manga» · 498.

### 6. Curiosidades y advertencias

1. **Diferencias de ±1** entre Player.bin y la pantalla: NO se explican por edad (hipótesis retirada: Gordon, Balde, Bernal y Eric García casi idénticos). Explicación más probable (🔎 por confirmar, doc 11 §D): la **Liga Máster no lee Player.bin al entrar a la carrera** y guarda su propia copia de los jugadores. Se comprueba con una prueba de reinicio.
2. «Pase en profundidad» ocupa **dos bits** con el mismo texto; «Cortada» parece lo contrario (un texto, varios bits).
3. Los porteros usan **otros 5 stats** en el mismo registro (269/300/320/326/364) además de los 20 de campo.
4. Pie malo, forma y lesiones se guardan **restando 1** respecto a lo que muestra el juego.
5. Para encontrar a un jugador por sus stats basta cruzar los 20 valores con tolerancia ±1: así se localizaron Isak 115079, Mbappé 110718, Dembélé 110626, Pedro (Flamengo) 111408, João Pedro Chelsea 127787, João Pedro Botafogo 135249, Livakovic 113596, Szczęsny 40937, Cancelo 47787, Bernal 171610, Balde 147233, Gordon 119835, Eric García 126337, Koundé 110784, Yamal 162114, Cubarsí 165696 (pid de ConmeGOL 26 🅲).
6. Livakovic, Hamza Abdelkarim y Gabriel Jesús ya no están en el Barça de la plantilla actual (experimento «Vestuario Barça» desactualizado en 3 jugadores).

### 7. Aún sin mapear (para próximas sesiones)

Rol en el equipo (Leyenda, Estrella Nv.2…), Personalidad (Jugador individualista, Pasión/Aplomo, Técnica/Fuerza, Clarividencia/Instinto), Influencia, tablas de tácticas ofensivas/defensivas (pares tipo 29/89), «Hoja de progreso» (curva de desarrollo), cortada/Centrador.

### 8. Qué parche aprovecha qué (para sacar el parche Phoenix)

| Tema | Todos los parches (🧩) | ConmeGOL 26 (🅲) | Sudamerican (🅢) |
|---|---|---|---|
| Formato de registro y bits | ✅ | — | ⏳ comprobar con 2 jugadores |
| Cantidad de jugadores | — | 29.997 (olmosjr23) | 29.323 |
| Equipos / competiciones | — | 749 / 90 | 726 / 80 |
| Carpeta de guardados | — | `239200` | `292733975847239680` |
| Sider | — | 7.3.3 (~57 módulos) | 7.3.4 (`netblock.lua` bloquea online) |
| Estilo | — | casi todo por Sider/livecpk | CPK clásicos gigantes (`SP_Subs.cpk`) |

Football Life 27, Evoweb, Gogosz y ConmeGOL 27: ⏳ pendiente (ver `12-GUIA-MULTIPARCHE.md`).

### 9. Siguiente paso seguro

Escribir **un** valor de **un** jugador en una **copia** de Player.bin (con respaldo y con aprobación de FRALEX) y verificarlo en el juego. Todavía no se ha escrito nada en Player.bin.

---

## Parte 4 · 17 · «Datos Actual. en vivo»: dónde guarda el juego si la opción está activada

> Origen: `PhoenixSync/base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` · último cambio bea741d 2026-10-09 · 11752 bytes. Copia íntegra.

**Investigación del 2026-10-09, de 21:04 a 23:10 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. Amplía el [07 · El motor por dentro](07-MOTOR-POR-DENTRO.md).
El camino completo, paso a paso, está en el [19](19-OPCION-EN-VIVO-PASO-A-PASO.md).

---

### 0. Resumen en 5 líneas

1. La opción **no se guarda en disco**. Ningún archivo de la carpeta de guardado cambia al abrir el juego, al pulsar Activar o Desactivar, ni al cerrar.
2. La opción vive **en la memoria**: es un número dentro del «gestor de la base» del juego. **1 = activada, 0 = desactivada.**
3. `phoenix.lua` ya lo puede leer de forma segura (solo lectura, con comprobación previa del código). Probado 3 veces.
4. Con la v0.17, el número vale 1 **solo unos 3 segundos** al pulsar Activar. Luego nuestro parche C lo devuelve a 0. Por eso la pantalla dice siempre «Selección actual: Desactivar».
5. Al reiniciar el juego, la opción vuelve sola a «Desactivar».

---

### 1. Palabras nuevas

- **Huella (sha256):** código que se calcula a partir de un archivo. Si el archivo cambia aunque sea en 1 byte, la huella cambia.
- **Foto:** calcular la huella de todos los archivos de una carpeta en un momento dado, para comparar después.
- **RVA:** posición dentro del `PES2021.exe`. Se escribe `exe+0x…`.
- **Puntero:** un dato que guarda la dirección de otro dato.
- **Gestor de la base:** el objeto del juego que carga los archivos de datos (`Player.bin`, `PlayerAssignment.bin`…).
- **Modo de carga:** el número que el gestor guarda para saber si la actualización en vivo está aplicada.
- **Copia segura:** leer la memoria con `ReadProcessMemory`. Si la zona ya no existe, la lectura falla sin cerrar el juego.

---

### 2. El resultado: dónde está el dato

| Qué | Dónde | Cómo se sabe |
|---|---|---|
| Puntero al gestor de la base | `exe+0x3705E10` (8 bytes) | Lo devuelve la función `0x14B6A60`: `mov rax, [exe+0x3705E10] ; ret` |
| **Modo de carga (la opción)** | **número de 4 bytes en `[gestor + 0x38]`** | Lo escribe `0x14B7560`. El juego lo lee en 12 sitios; en 11 de ellos lo compara con 1 |
| Estado del gestor | 4 bytes en `[gestor + 0x30]` | 4 = quieto, 1 = cargando (visto en el juego) |
| Bandera «carga terminada» | byte `[gestor + 0x91]` | 0 mientras carga, 1 al terminar (visto en el overlay; 🔎 observado) |

**Valores del modo de carga:**

| Valor | Significa | Estado |
|---|---|---|
| 0 | Desactivada | ✅ visto en el juego, coincide con «Selección actual: Desactivar» |
| 1 | Activada | ✅ visto en el juego durante Activar (3 de 3). En pantalla no se llega a ver, porque dura 3 segundos |
| 2 | ¿Valoraciones generales uniformes? | ❓ hipótesis. El código usa el valor 2 en otro camino (`0x20AE705`). Sin probar |

#### Cómo leerlo con seguridad (así lo hace la v0.17m)

1. Comprobar estos bytes del código. Si alguno no coincide, **no leer nada**:

| RVA | Bytes (hex) | Qué es |
|---|---|---|
| `0x14B6A60` | `48 8B 05 A9 F3 24 02 C3` | `mov rax, [exe+0x3705E10] ; ret` |
| `0x13040C9` | `E8 92 29 1B 00 83 78 38 01 74 34` | `call 0x14B6A60 ; cmp dword [rax+0x38], 1 ; je` |
| `0x14B7587` | `44 89 41 38 45 85 C0 75 07` | `mov [rcx+0x38], r8d ; test r8d, r8d ; jne` |

2. Leer 8 bytes en `exe+0x3705E10` con copia segura. Si valen 0, el gestor todavía no existe: no leer más.
3. Comprobar que el puntero es razonable (ni muy bajo ni fuera del espacio de usuario).
4. Leer con copia segura el trozo `[gestor+0x30 … +0x93]` a un búfer propio.
5. El modo es el número de 4 bytes en la posición `+0x38`.

Nunca se escribe en la memoria.

---

### 3. Por qué la pantalla dice «Desactivar» después de pulsar Activar

Con la v0.17 (parches A, B y C), pulsar Activar hace **dos recargas seguidas**:

| Paso | Qué hace el juego | Modo que queda |
|---|---|---|
| Estado 22 de `LiveDataSetFlow` (`0x20AE523`) | Llama a `0x14B7560(gestor, 0, 1)`: recarga toda la base en **modo 1**. Es el paso original de Konami | **1** |
| Estado 26 (`editLoadDataInLiveDataSet`) con el parche C | La tarea de carga del EDIT llama a `0x14B7560(gestor, 0, 0)` (`0x1EFAFB0`) cuando su bandera `[tarea+0x71]` no vale 0: recarga la base otra vez en **modo 0**. 🔎 Se deduce que esa bandera es el primer parámetro, el que el parche C pone a 1 | **0** |

Por eso el espía ve la base leída **dos veces**, con unos 3 segundos de diferencia, y el modo termina en 0.

**Consecuencia:** con la v0.17, el juego nunca se queda en «Activar». Para él, la opción está siempre desactivada.

#### Lo que hace cada función (leído en el código, sin tocar el exe)

- `0x14B6A60` → devuelve el gestor de la base. La llaman 5.413 sitios del juego.
- `0x14B6A70(gestor)` → responde «¿está quieto?» (`[gestor+0x30] == 4`).
- `0x14B7560(gestor, bandera, modo)` → «recargar la base con este modo». Guarda el modo en `[gestor+0x38]` y pone el estado en 1.
  - La llaman 5 sitios: `0x20AE529` (modo 1), `0x20AE705` (modo 2), `0x1EFAFB0`, `0x13A4A5D` y `0xE9F708` (modo 0).
- `0x14B6F20(gestor, n)` → «quitar datos en vivo»: pone a 0 el byte `[gestor+0x90+n]`. La usa Desactivar (`LiveDataRemoveFlow`, `0x20AF320`) y otros 4 sitios, siempre solo si ese byte no vale 0.
- `ProcessLiveDataCheck` (`0x13040C0`): al entrar a Partido, si el modo ya es 1 sigue de largo. Si no, mira otro dato, el byte `[[exe+0x37F9648]+0xA]` (❓ parece la respuesta del jugador a «¿aplicar la actualización en vivo?»), y si vale 1 lanza el flujo de Activar.

#### Por qué con Activar los equipos salen de la base (❓ hipótesis)

La tarea `editLoadDataInLiveDataSet` recibe los parámetros `0 (1 con el parche C), 1, 1, 0, 0`. Editar → Cargar usa `1, 1, 0, 0, 0`.
La diferencia es el **tercer parámetro**. Lo más probable es que signifique «no apliques las plantillas del option file».
No está comprobado.

---

### 4. Fase A · ¿Se guarda en disco? → No

Carpeta activa: `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
Cada foto incluye todos los archivos de esa carpeta, las otras carpetas de guardado (`292733975847239680`, `48`, `SP`) y `settings.dat`.

| Hora | Momento | ¿Cambió algo? |
|---|---|---|
| 21:09 | Foto inicial | — |
| 22:02 | Juego cerrado (confirmado por FRALEX) | No |
| 22:10 | Juego abierto (arrancó a las 22:07) | No |
| 22:23 | Un minuto después de Activar (22:22) | No |
| 22:31 | Nueve minutos después de Activar | No |
| 22:32 | Juego cerrado (Sider anotó el cierre a las 22:31:48) | No |
| 22:39 | Juego abierto de nuevo y Desactivar pulsado | No |
| 22:41 | Después de otro Activar (22:41) | No |
| 22:44 | Después de otro Desactivar | No |

- En la carpeta del juego, en ese tiempo, solo cambiaron los diarios de Sider.
- **Al reiniciar, la opción sale en «Desactivar»** aunque se hubiera pulsado Activar antes de cerrar (visto una vez, 22:39).
- Pulsar Desactivar estando ya en «Desactivar» **no recarga nada** (22:44: ninguna lectura nueva de la base).
- La pantalla muestra el estado en la línea **«Selección actual: …»**. La opción tiene tres posiciones: Activar, Desactivar y Valoraciones generales uniformes.

---

### 5. Fase B y C · En la memoria → encontrado y confirmado

Método: primero se leyó el código del exe (copia de solo lectura, Python + capstone) hasta dar con un candidato. Después se
instaló `phoenix.lua` **v0.17m**, que es la v0.17 más una parte que **solo lee**:

- tecla **M**: toma una muestra y la anota en `sider.log`;
- línea **[MODO]** en el overlay;
- una muestra automática cada vez que el juego empieza a leer `player.bin`.

Antes de instalar se simuló en LuaJIT con memoria falsa (6 casos: normal, exe distinto, gestor inexistente, zona ilegible,
durante Activar y módulo recargado con Shift+R): 6 de 6 bien, 0 escrituras.

**Resultado en el juego (sesión que arrancó a las 23:00:09):**

| Hora | Lectura de `player.bin` | Modo leído |
|---|---|---|
| 23:00:09 | n.º 1, arranque | 0 |
| 23:01:54 | n.º 2, segunda pasada del arranque | 0 |
| 23:02:06 | n.º 3, Activar (primera recarga) | **1** |
| 23:02:09 | n.º 4, Activar (segunda recarga) | 0 |
| 23:04:06 | n.º 5, Activar | **1** |
| 23:04:09 | n.º 6 | 0 |
| 23:04:18 | n.º 7, Activar | **1** |
| 23:04:21 | n.º 8 | 0 |

- Tres repeticiones, las tres iguales. Ningún error de Lua. El juego siguió normal.
- A las 23:04:30 el overlay mostraba «[MODO] opción en vivo: DESACTIVADA (0) · gestor estado 4» y el juego «Selección actual: Desactivar». Coinciden.

---

### 6. Riesgos y otros parches

- **Otra versión del exe:** las direcciones cambian. La lectura comprueba antes los bytes del código; si no coinciden, no lee y muestra «exe distinto». Degrada sin romper nada.
- **Otro parche con el mismo exe** (por ejemplo Sudamerican, si usa el mismo `PES2021.exe`): debería funcionar igual, porque el dato es del juego y no del parche. Sin probar.
- **El gestor puede no existir** muy al principio del arranque: la lectura lo detecta (puntero 0) y no lee.
- **Exe estudiado:** 458.806.784 bytes, sha256 `5e27a78246dbe4f1e66cb60ec56e86a739303708b02d4048f6fec6b570bc224a`.
- **Secciones:** `.trace` (código) en RVA `0x1000`; `.rdata` en `0x252F000`; `.data` en `0x339B000` (tamaño en memoria `0x72A000`); `.bss` en `0x3C98000`; `.data1` en `0x3CA1000`.

---

### 7. Propuesta para phoenix.lua (sin instalar; pendiente de aprobación)

**Opción A · Recordar la última recarga (no toca los parches).**
Cada vez que el juego empieza a leer `player.bin`, phoenix.lua lee el modo:

- si vale **1** → apunta «última recarga: ACTIVAR» y la hora;
- si vale **0** y no hubo un 1 en los últimos 15 segundos → apunta «última recarga: normal» (arranque, Editar → Cargar o menú principal).

Con eso sabe en qué situación está el jugador, sin escribir nada.
Límites: si se recarga el módulo con Shift+R, pierde la memoria hasta la siguiente recarga del juego.

**Opción B · Experimento sin el parche C.**
Aplicar solo A y B. La recarga del estado 22 ya relee toda la base en modo 1, así que el modo se quedaría en 1 y la pantalla
debería decir «Selección actual: Activar». Dudas a comprobar: si las stats y los fichajes siguen llegando, y cómo se portan
otros menús con la opción activada de verdad. El cambio vive solo en la memoria y desaparece al cerrar el juego.

**Pendiente (Fase D):** cómo saber desde Lua que se está en el menú principal y no en un partido ni en Editar.

---

### 8. Dónde está todo

| Qué | Dónde |
|---|---|
| Versión de prueba instalada el 2026-10-09 a las 22:57 | `sider/pruebas/phoenix-v0.17m-prueba.lua` (sha256 `57ff5ec0…52805`) |
| Simulación de la versión de prueba | `sider/pruebas/simular_v017m.py` (necesita `pip install lupa`) |
| Versión estable | `sider/phoenix.lua` = v0.17 (sha256 `d85e1071…bfc69c`), sin cambios |
| Respaldos de la v0.17 en el PC | `phoenix.lua.v017` en `SiderAddons\modules\` (raíz) y en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\` |
| Ayudantes para leer el exe | `herramientas/exe/lib_exe.py` (necesita `pip install capstone numpy`; se ejecuta junto a una copia de `PES2021.exe`) |
| Guion de las fotos del guardado | `herramientas/exe/foto_guardado.sh` |
| Diario de las pruebas | `PRUEBAS.md`, entradas del 2026-10-09 de 21:09 a 23:04 |

**Para volver a la v0.17:** con el juego cerrado, copiar `phoenix.lua.v017` encima de `phoenix.lua` en las dos carpetas `modules`.

---

## Parte 5 · 19 · «Datos Actual. en vivo»: informe paso a paso de todo lo que se hizo

> Origen: `PhoenixSync/base-conocimiento/19-OPCION-EN-VIVO-PASO-A-PASO.md` · último cambio 15a9ed6 2026-10-09 · 23818 bytes. Copia íntegra.

**Sesión del 2026-10-09, de 21:04 a 23:50 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. El resultado resumido está en [17 · dónde está la opción](17-OPCION-EN-VIVO-DONDE-ESTA.md).
Este documento cuenta **el camino completo**, en orden, con las horas, los errores y lo que se decidió en cada momento.

---

### 0. La pregunta y la respuesta

**Pregunta de FRALEX:** ¿cómo puede saber `phoenix.lua`, de forma segura y solo leyendo, si «Datos Actual. en vivo» está activado?

**Respuesta en 4 líneas:**

1. La opción no está en ningún archivo. Está en la memoria del juego.
2. Es un número dentro del «gestor de la base»: 1 = activada, 0 = desactivada.
3. Con la v0.17, ese número vale 1 solo unos 3 segundos cada vez que se pulsa Activar.
4. Por eso `phoenix.lua` ahora **recuerda él mismo** cuál fue la última recarga (versión de prueba v0.17a).

---

### 1. Palabras que se usan

- **Huella (sha256):** código que sale de un archivo. Si el archivo cambia en 1 byte, la huella cambia.
- **Foto:** apuntar la huella de todos los archivos de una carpeta en un momento, para comparar después.
- **Registro de Sider (`sider.log`):** el diario donde Sider anota lo que pasa, con la hora.
- **Espía:** la parte de `phoenix.lua` que anota cada vez que el juego lee un archivo de la base.
- **Exe:** el archivo `PES2021.exe`.
- **Desensamblar:** traducir los bytes del exe a instrucciones legibles. No cambia el archivo.
- **RVA:** posición dentro del exe. Se escribe `exe+0x…`.
- **Puntero:** un dato que guarda la dirección de otro dato.
- **Gestor de la base:** el objeto del juego que carga los archivos de datos.
- **Copia segura:** leer la memoria con `ReadProcessMemory`. Si la zona no existe, falla sin cerrar el juego.
- **Simular:** ejecutar el Lua fuera del juego, con una memoria falsa, para probarlo sin riesgo.

---

### 2. Reglas que se respetaron

- Nada se cambió sin el «sí» de FRALEX.
- Nunca se escribió en la memoria del juego.
- El exe nunca se modificó: se estudió una copia.
- Respaldo antes de cada instalación, y huella comprobada en el PC después de copiar.
- No se pidieron contraseñas ni tokens.
- No se hizo `git pull` ni `git checkout` en `C:\dev\smash-soda-fork`.

**Lo único que cambió en el PC de FRALEX en toda la sesión:**

| Archivo | Carpetas | Qué pasó |
|---|---|---|
| `phoenix.lua` | las dos `modules` | v0.17 → v0.17m (22:57) → v0.17a (23:31) |
| `phoenix.lua.v017` | las dos `modules` | Nuevo. Respaldo de la v0.17 (22:54) |
| `phoenix.lua.v017m` | las dos `modules` | Nuevo. Respaldo de la v0.17m (23:31) |

Las dos carpetas `modules` son:

- `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\`
- `D:\Frank\Games_\Conmegol Patch\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\`

No se tocó `sider.ini`, ni la carpeta de guardado, ni el option file, ni Phoenix-DB.

---

### 3. Paso 1 · Leer la documentación (21:04–21:08)

1. Se pidió acceso a `C:\dev\smash-soda-fork`. FRALEX lo aprobó.
2. Se leyeron `CLAUDE.md`, `COORDINACION.md`, el final de `PhoenixSync/REGISTRO.md` y de `PRUEBAS.md`, `07-MOTOR-POR-DENTRO.md` y `sider/phoenix.lua`.
3. **Problema:** `INFORME-COMPLETO-2026-10-09.md` no estaba en la carpeta.
4. **Causa:** la carpeta del PC estaba atrasada. Estaba en el commit `540fa1a` y GitHub iba varios commits por delante.
   Se comprobó con `git ls-remote origin`, que solo pregunta y no cambia nada.
5. **Solución sin tocar el PC:** se bajó una copia de la rama `mercado-fase0` a una carpeta temporal de trabajo
   (fuera de las carpetas de FRALEX) y se leyó desde ahí el informe, `SYNC-COMPARTIDO.md` y las entradas nuevas de `PRUEBAS.md`.
6. Se comprobó que el `phoenix.lua` instalado en el juego era el mismo que el del repo (misma huella, `d85e1071…`).

---

### 4. Paso 2 · Fase A: ¿se guarda en disco? (21:08–22:44)

#### 4.1 Encontrar la carpeta de guardado

1. Los documentos decían que la carpeta activa es `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
2. En la carpeta de usuario no hay «Documents»: está en `D:\Users\Alexander\Documents`.
3. Se pidió acceso solo a `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE`. FRALEX lo aprobó.
4. Dentro hay varias carpetas de guardado: `239200` (la activa: option file de las 19:59), `292733975847239680`, `48` y `SP`.

#### 4.2 El guion de las fotos

Se escribió `herramientas/exe/foto_guardado.sh`. En cada foto:

- calcula la huella de cada archivo de `239200\save`, de las otras carpetas de guardado y de `settings.dat`;
- guarda un listado con tamaño y hora de todos los archivos, también los de las subcarpetas;
- copia a la carpeta temporal `SYSTEM00000000`, `GRAPHICS000000`, `EDIT00000000` y `settings.dat`, para poder comparar byte a byte.

Solo lee. No escribe nada en el PC.

#### 4.3 Las fotos, una por una

| N.º | Hora | Qué había pasado | Resultado |
|---|---|---|---|
| 0 | 21:09 | Foto inicial | — |
| 1 | 22:02 | FRALEX confirma que el juego está cerrado | Idéntica a la 0 |
| 2 | 22:10 | El juego ya estaba abierto (arrancó a las 22:07:47) | Idéntica |
| 3 | 22:23 | Un minuto después de pulsar Activar (22:22) | Idéntica |
| 4 | 22:31 | Nueve minutos después de ese Activar | Idéntica |
| 5 | 22:32 | Juego cerrado (Sider anotó «All done» a las 22:31:48) | Idéntica |
| 6 | 22:39 | Juego abierto de nuevo (22:33:59) y Desactivar pulsado | Idéntica |
| 7 | 22:41 | Después de otro Activar (22:41:03) | Idéntica |
| 8 | 22:44 | Después de otro Desactivar | Idéntica |

En las fotos 3 y 5 se compararon además los cuatro archivos copiados byte a byte: 0 bytes distintos.
En la carpeta del juego, en ese tiempo, solo cambiaron los diarios de Sider.

#### 4.4 Lo que se aprendió por el camino

**a) El registro de Sider sirve de reloj.** Al principio dependíamos de que FRALEX contara qué había hecho y cuándo.
A las 22:26 FRALEX conectó la carpeta del juego. Ahí se vio que el espía de `phoenix.lua` anota en `sider.log`
cada lectura de la base con su hora. Desde entonces se pudo saber sin preguntar:

- cuándo arrancó el juego (primera lectura de la base);
- cuándo se pulsó Activar (dos lecturas completas de la base, con 3 a 5 segundos de diferencia);
- cuándo se cerró («All done»).

**b) El juego dice el estado en pantalla.** La captura de las 22:39 mostró el cuadro
«Datos Actual. en vivo/Valoración uniformes», con la línea **«Selección actual: Desactivar Actualización en vivo»**
y tres posiciones: Activar, Desactivar y Valoraciones generales uniformes.

**c) Al reiniciar, la opción vuelve a «Desactivar».** FRALEX había pulsado Activar a las 22:22 y cerrado a las 22:31.
Al abrir de nuevo, la selección salía en «Desactivar» por defecto.

**d) Después de Activar, la pantalla sigue diciendo «Desactivar».** Las tres capturas de las 22:41 lo muestran:
«Actualización en vivo en curso», luego el mensaje de la forma física, y al volver a entrar, «Selección actual: Desactivar».
Este fue el dato que cambió la investigación.

**e) Desactivar, estando ya en «Desactivar», no hace nada.** A las 22:44 no hubo ninguna lectura nueva de la base.

#### 4.5 Errores y dudas de esta fase

- **Sospecha equivocada:** se pensó que la opción estaría en `SYSTEM00000000` (el archivo de ajustes). No cambió nunca.
- **Duda que hubo que resolver:** que Activar no escribiera nada podía deberse a que la opción «ya estaba activada de antes».
  La captura de las 22:39 lo descartó: estaba en «Desactivar».
- **Sin determinar:** el juego lee la base una segunda vez un rato después de arrancar (16 segundos, 4 minutos
  y 1 minuto 45 segundos en las tres sesiones). No se sabe qué pantalla la dispara. En esa lectura el modo vale 0.

**Conclusión de la Fase A:** la opción no se guarda en disco.

---

### 5. Paso 3 · Decisión de FRALEX (22:44)

Se le explicaron dos caminos:

- **Camino 1:** que `phoenix.lua` lleve él mismo la cuenta de la última recarga.
- **Camino 2:** buscar en la memoria el dato que dibuja «Selección actual».

FRALEX eligió: «intenta el camino 2 y luego, si no funciona, vemos el 1».

---

### 6. Paso 4 · Estudiar el exe (22:44–22:53)

En vez de fotografiar a ciegas 7,5 MB de memoria, primero se leyó el código para llegar a un candidato concreto.

#### 6.1 Preparación

1. En la carpeta temporal se instalaron `capstone` (desensamblador) y `pefile` (lector de exe) con `pip`.
2. Se copió `PES2021.exe` a la carpeta temporal. Tamaño: 458.806.784 bytes. Huella: `5e27a782…bc224a`.
3. Se leyó la tabla de secciones: el código está en `.trace` (RVA `0x1000`), los textos en `.rdata` (`0x252F000`)
   y los datos en `.data` (`0x339B000`).
4. Se escribió `herramientas/exe/lib_exe.py`, con estas funciones:
   - `show` y `brief`: desensamblar una zona, completa o resumida;
   - `xrefs`: encontrar qué instrucciones apuntan a una dirección;
   - `callers`: encontrar quién llama a una función;
   - `func` y `funcs_in`: límites de cada función, sacados de la tabla `.pdata` (158.648 funciones);
   - `cstr`: leer un texto de `.rdata`.

#### 6.2 El rastro, eslabón por eslabón

1. **Buscar textos con «LiveData».** Salieron 58. Entre ellos `Exhibition/LiveData/LiveDataCheck`, `…/LiveDataSet`,
   `…/LiveDataRemove`, `LiveDataRemoveFlow` y `ProcessCmnLiveDataSetFlow`.
2. **Tabla de procesos** en `.data 0x34D7E70`: cada entrada tiene la fábrica del proceso y su nombre.
   - LiveDataCheck → `0x1303F70`
   - LiveDataSet → `0x1308080`
   - LiveDataRemove → `0x1308DD0`
3. **Seguir «Desactivar»**, porque es lo más corto. `0x1308DE0` crea `LiveDataRemoveFlow` con `0x20AF2D0`.
   Su función de pasos es `0x20AF320`.
4. **Dentro de ese flujo** aparece `0x14B6A60`, que es solo `mov rax, [exe+0x3705E10] ; ret`.
   Devuelve un objeto global: el **gestor de la base**. Luego llama a `0x14B6F20(gestor, 0)`.
5. **`0x14B6F20`** pone a 0 el byte `[gestor+0x90+n]`. Tiene 5 llamadores; los otros 4 la llaman solo si ese byte no vale 0.
6. **`ProcessLiveDataCheck`** (`0x13040C0`) contiene la pista clave:
   `call 0x14B6A60 ; cmp dword [rax+0x38], 1`. El juego pregunta «¿el número en `+0x38` vale 1?».
7. **¿Quién escribe ese número?** Se buscó en las funciones del gestor: `0x14B7587` hace `mov [rcx+0x38], r8d`,
   dentro de la función `0x14B7560(gestor, bandera, modo)`. Es «recargar la base con este modo».
8. **¿Quién llama a `0x14B7560` y con qué modo?** Cinco sitios:
   - `0x20AE529` → modo 1 (estado 22 de `LiveDataSetFlow`);
   - `0x20AE705` → modo 2 (estado 24 del mismo flujo);
   - `0x1EFAFB0`, `0x13A4A5D` y `0xE9F708` → modo 0 (carga del EDIT y Editar → Cargar).
9. **Leer los estados 22 a 26** de `LiveDataSetFlow` (tabla de saltos en `0x20AECC0`, 38 estados):
   - estado 22: crea el diálogo de éxito y luego llama a `0x14B7560(gestor, 0, 1)` → **modo 1**;
   - estado 23: espera a que el gestor termine y al menos 2 segundos;
   - estado 26: crea la tarea `editLoadDataInLiveDataSet`.
10. **La tarea de carga del EDIT** (`0x1EFAF60`), si su bandera `[tarea+0x71]` no vale 0, llama a
    `0x14B7560(gestor, 0, 0)` → **modo 0**. Con el parche C esa bandera vale 1.
11. **Comprobación final:** de los 5.413 sitios que llaman a `0x14B6A60`, 12 leen `[gestor+0x38]`,
    y 11 de ellos lo comparan con 1.

#### 6.3 La predicción

Con eso se pudo predecir, antes de probar nada:

- pulsar Activar con la v0.17 pone el modo en **1** (estado 22) y unos segundos después en **0** (parche C);
- por eso el espía ve la base leída dos veces;
- por eso la pantalla dice «Selección actual: Desactivar».

---

### 7. Paso 5 · La versión de prueba v0.17m (22:53–23:04)

#### 7.1 Cómo se construyó

1. Se partió del `phoenix.lua` instalado (v0.17, huella `d85e1071…`).
2. Se añadió un bloque nuevo antes de `m.key_down`, y cuatro cambios pequeños:
   - versión `0.17` → `0.17m`;
   - tecla **M** en `key_down`;
   - una muestra al empezar cada lectura de `player.bin`, en `livecpk_read`;
   - la línea **[MODO]** al final del overlay.
3. Los cambios se aplicaron con un guion que exige que cada texto buscado aparezca **exactamente una vez**.
   Si no, se detiene. Así no se cambia nada por error.

#### 7.2 Qué hace el bloque nuevo, en orden

1. Comprueba tres trozos del código (`0x14B6A60`, `0x13040C9`, `0x14B7587`). Si alguno no coincide, no lee nunca más.
2. Lee 8 bytes en `exe+0x3705E10` con copia segura.
3. Si el puntero vale 0, o es raro, no sigue.
4. Lee con copia segura el trozo `[gestor+0x30 … +0x93]` a un búfer **propio** de 256 bytes.
5. Saca el estado (`+0x30`), el modo (`+0x38`) y las banderas (`+0x90` a `+0x93`).
6. Lo muestra en el overlay y lo anota en `sider.log`.

Nunca escribe en la memoria.

#### 7.3 La simulación

Se instaló `lupa` (trae LuaJIT) y se montó un entorno falso de Sider: `log`, `memory`, y un `ffi` con
`ReadProcessMemory`, `WriteProcessMemory` y `VirtualProtect` falsos que **cuentan** cada llamada.

| Caso | Resultado |
|---|---|
| 1. Normal: modo 0, luego 1, luego 2 | Bien |
| 2. Exe distinto | No lee nada más |
| 3. El gestor no existe (puntero 0) | Avisa y no lee |
| 4. Zona ilegible y puntero raro | Avisa y no lee |
| 5. Lecturas de `player.bin` durante Activar | Anota 0, 1, 0 |
| 6. Módulo cargado dos veces (Shift+R) | Bien |

Escrituras contadas en los 6 casos: 0. Guion: `sider/pruebas/simular_v017m.py`.

#### 7.4 Respaldo e instalación

1. **22:54** · FRALEX pidió guardar bien la v0.17. Se creó `phoenix.lua.v017` en las dos carpetas `modules`
   y se le envió una copia por el chat. La v0.17 quedó en 5 sitios, todos con la misma huella.
2. **22:57** · FRALEX dio el «sí». Antes de instalar:
   - se comprobó la v0.17 en 6 sitios: 6 de 6 con la huella correcta;
   - se comprobó que el juego estaba cerrado (Sider: «All done» a las 22:57:04).
3. Se copió la v0.17m a las dos carpetas.
4. Se comprobó la huella en el PC: `57ff5ec0…52805` en las dos. Respaldos intactos.

#### 7.5 El resultado en el juego

Sesión que arrancó a las 23:00:09. FRALEX pulsó Activar tres veces.

| Hora | Lectura de `player.bin` | Modo |
|---|---|---|
| 23:00:09 | n.º 1, arranque | 0 |
| 23:01:54 | n.º 2, segunda pasada | 0 |
| 23:02:06 | n.º 3, Activar | **1** |
| 23:02:09 | n.º 4 | 0 |
| 23:04:06 | n.º 5, Activar | **1** |
| 23:04:09 | n.º 6 | 0 |
| 23:04:18 | n.º 7, Activar | **1** |
| 23:04:21 | n.º 8 | 0 |

- Tres de tres, igual que la predicción. Sin errores de Lua.
- Captura de FRALEX a las 23:04:30: el overlay dice «[MODO] opción en vivo: DESACTIVADA (0)» y el juego
  «Selección actual: Desactivar». Coinciden.

---

### 8. Paso 6 · Decisión de FRALEX (23:07)

Leer el dato funciona, pero con la v0.17 casi siempre dirá 0. Se le explicaron dos opciones:

- **A:** `phoenix.lua` recuerda la última recarga. No toca los parches.
- **B:** experimento sin el parche C, para que el juego se quede en «Activar» de verdad.

FRALEX eligió: «guarda primero, luego empezamos con la A y luego hacemos la B para experimentar».

---

### 9. Paso 7 · Guardar (23:07–23:15)

1. **Problema:** desde el PC no había credenciales para subir a GitHub.
2. **Solución:** se conectó el repo `Fralexito/smash-soda-fork` a la sesión de Claude y se clonó la rama `mercado-fase0`
   en el espacio de trabajo de la nube. El PC no se tocó.
3. Los guiones se pasaron del PC a la nube empaquetados, y se comprobó la huella de cada uno al llegar.
4. Se escribieron o ampliaron:
   - `base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` (nuevo);
   - `PRUEBAS.md` (4 entradas añadidas al final);
   - `REGISTRO.md` (1 línea añadida);
   - `base-conocimiento/00-INDICE.md` (1 línea añadida);
   - `sider/pruebas/` y `herramientas/exe/` (carpetas nuevas).
5. Se volvió a pasar la simulación con los archivos ya guardados: bien.
6. `git fetch`, `git rebase`, commit `ff88f3d` y `git push`. Se comprobó con `git ls-remote` que GitHub tenía ese commit.
7. **Google Drive:** documento nuevo «🔎 «Datos Actual. en vivo»: dónde guarda el juego la opción (9 oct 2026, 23:15)»,
   en la misma carpeta que los demás informes. Se volvió a abrir para comprobar que el contenido estaba completo.

---

### 10. Paso 8 · Opción A: la versión v0.17a (23:15–23:31)

#### 10.1 La idea

Cada vez que el juego **empieza** a leer `player.bin`, empieza una recarga de la base. En ese momento se mira el modo:

| Modo leído | Se apunta | Significa |
|---|---|---|
| 1 | ACTIVAR | Equipos de la base (`PlayerAssignment.bin`) |
| 0, sin un 1 en los últimos 15 segundos | NORMAL | Equipos del option file |
| 0, dentro de esos 15 segundos | Nada | Es la segunda recarga del mismo Activar |
| Otro valor | DESCONOCIDA | Por si aparece el modo 2 |
| No se pudo leer | Nada | No se inventa un dato |

#### 10.2 Cómo se construyó

1. Se partió de la v0.17m.
2. Se añadió el bloque «última recarga» (`anotarRecarga`, `ultimaFueActivar`, `textoUltima`).
3. Tres cambios pequeños: versión `0.17a`, la muestra de `player.bin` ahora alimenta `anotarRecarga`,
   y la línea **[ÚLTIMA RECARGA]** al final del overlay.
4. Se comprobó antes que `os.time()` existe en el Lua de Sider (otros módulos ya lo usan).

#### 10.3 La simulación (con reloj falso)

| Caso | Resultado esperado y obtenido |
|---|---|
| A1. Arranque, Activar, Activar | sin ver → NORMAL → ACTIVAR → ACTIVAR |
| A2. Activar, Editar → Cargar un minuto después, segunda recarga lenta (16 s) | ACTIVAR → NORMAL → NORMAL |
| A3. Exe distinto | No lee y no apunta nada |
| A4. Módulo recargado (Shift+R) | Empieza sin memoria; luego NORMAL |
| A5. Modo 2, reloj hacia atrás, gestor inexistente | DESCONOCIDA → NORMAL → no cambia |

Además se repitieron los 6 casos de la v0.17m con la v0.17a: bien. Escrituras: 0.
Guion: `sider/pruebas/simular_v017a.py`.

#### 10.4 Instalación (23:31)

1. FRALEX dio el «sí».
2. Juego cerrado: Sider anotó «All done» a las 23:31:02.
3. Respaldo de la v0.17m como `phoenix.lua.v017m`, en las dos carpetas.
4. Copia de la v0.17a a las dos carpetas.
5. Huella comprobada en el PC: `379dce7b…3306c` en las dos. Respaldos `v017` y `v017m` intactos.

#### 10.5 Límites conocidos de la opción A

- No ve las recargas que no releen la base (tecla L, «Ser una Leyenda»).
- Tras Shift+R empieza sin memoria hasta la siguiente recarga del juego.
- La ventana de 15 segundos es una elección: las dos recargas de Activar se separan unos 3 segundos.

#### 10.6 Resultado en el juego (23:34–23:43) · añadido a las 23:52

Sesión que arrancó a las 23:34:11. Sin errores de `phoenix.lua`.

| Hora | Qué hizo FRALEX | Modo leído | phoenix.lua apuntó |
|---|---|---|---|
| 23:34:11 | Abrir el juego (lectura n.º 1) | 0 | NORMAL |
| 23:34:39 | Segunda pasada del arranque (n.º 2) | 0 | NORMAL |
| 23:35:00 | **Activar** (n.º 3) | **1** | **ACTIVAR** |
| 23:35:03 | Segunda recarga de ese Activar (n.º 4) | 0 | No cambió |
| 23:36:33 | Entrar a Editar (n.º 5) | 0 | **NORMAL** |
| 23:41:55 | **Activar** (n.º 6) | **1** | **ACTIVAR** |
| 23:41:59 | Segunda recarga (n.º 7) | 0 | No cambió |
| 23:42:49 | Entrar y salir de Editar (n.º 8) | 0 | **NORMAL** |

**Capturas del overlay:**

- 23:35:38 → «[ÚLTIMA RECARGA] ACTIVAR → equipos de la base · 23:35:00 · ¿fue Activar? true», con «[MODO] DESACTIVADA (0)».
- 23:36:46 → «[ÚLTIMA RECARGA] NORMAL → equipos del option file · 23:36:33 · ¿fue Activar? false».

**Comprobación con Vinícius** (en el Barça solo en la base; en el Real Madrid en el option file):

| Después de… | Vinícius aparece en | Atlético de Madrid |
|---|---|---|
| Activar (23:41:55) | **Barça** | Giménez, Ruggeri, Nico González (plantilla de la base) |
| Entrar y salir de Editar (23:42:49) | **Real Madrid** | Julián Álvarez, Jonathan David, Sørloth (plantilla del option file) |

**Lo que se descubrió en esta prueba:**

1. La etiqueta de `phoenix.lua` coincide con los equipos que se ven en pantalla. Visto una vez.
2. Para volver al option file **basta con entrar a Editar**. No hizo falta pulsar Cargar.
3. El juego sigue diciendo «Selección actual: Desactivar» aunque los equipos sean los de la base. `phoenix.lua` lo recuerda por su cuenta.

**Dos incidencias, sin daño:**

- **«Guardar» por accidente (23:36).** En Editar, el cursor se fue sin querer a «Guardar». Foto de las 23:37: el option file
  es idéntico (huella `3507ce35…`) y ni su fecha cambió (19:59:59). No se guardó nada. Foto de las 23:43: igual.
- **Errores de otro módulo.** Durante la preparación del partido, `sider.log` anotó 96 errores de `StadiumServer.lua`
  (línea 705, «attempt to index a nil value»). No son de `phoenix.lua` y ese módulo no se tocó. No se sabe si ya ocurría
  antes, porque el registro se borra en cada arranque.

---

### 11. Lo que falta

1. Repetir otro día la prueba de la v0.17a con un jugador de referencia (hoy se vio una vez, con Vinícius).
2. **Opción B (experimento):** sin el parche C, ¿el modo se queda en 1 y la pantalla dice «Selección actual: Activar»?
   ¿Siguen llegando las stats y los fichajes?
3. Confirmar el valor 2 (¿Valoraciones generales uniformes?).
4. **Fase D:** saber desde Lua que se está en el menú principal y no en un partido ni en Editar.
5. Actualizar la carpeta `C:\dev\smash-soda-fork` del PC (`git pull --rebase`), con el visto bueno de FRALEX.

---

### 12. Cómo repetirlo en otro PC o en otro parche

1. Cerrar el juego. Hacer una foto de la carpeta de guardado con `herramientas/exe/foto_guardado.sh`.
2. Abrir el juego, pulsar Activar, cerrar. Otra foto. Comparar. (Aquí: ningún cambio.)
3. Copiar `PES2021.exe` a una carpeta de trabajo y comprobar su huella.
   - Si es `5e27a782…bc224a`, las direcciones de este informe valen.
   - Si es otra, repetir el rastro del apartado 6.2 con `lib_exe.py`, empezando por buscar los textos con «LiveData».
4. Simular el Lua antes de instalarlo (`simular_v017m.py`, `simular_v017a.py`).
5. Respaldar, instalar con el juego cerrado y comprobar la huella en el PC.
6. Pulsar Activar y leer en `sider.log` las líneas `[phoenix] modo` y `[phoenix] última recarga`.

---

### 13. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Resultado resumido | `base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` |
| Este informe | `base-conocimiento/19-OPCION-EN-VIVO-PASO-A-PASO.md` |
| Diario de pruebas | `PRUEBAS.md`, entradas del 2026-10-09 desde las 21:09 |
| Versión estable | `sider/phoenix.lua` = v0.17 (`d85e1071…`), sin cambios |
| Prueba «mirar la opción» | `sider/pruebas/phoenix-v0.17m-prueba.lua` (`57ff5ec0…`) |
| Prueba «última recarga» (instalada ahora) | `sider/pruebas/phoenix-v0.17a-prueba.lua` (`379dce7b…`) |
| Simulaciones | `sider/pruebas/simular_v017m.py` y `simular_v017a.py` (necesitan `pip install lupa`) |
| Lector del exe | `herramientas/exe/lib_exe.py` (necesita `pip install capstone numpy`) |
| Guion de las fotos | `herramientas/exe/foto_guardado.sh` |
| Respaldos en el PC | `phoenix.lua.v017` y `phoenix.lua.v017m` en las dos carpetas `modules` |

**Para volver a la v0.17:** con el juego cerrado, copiar `phoenix.lua.v017` encima de `phoenix.lua` en las dos carpetas `modules`.

---

## Parte 6 · 22 · Fase D: en qué parte del juego está el jugador y cuándo es seguro recargar

> Origen: `PhoenixSync/base-conocimiento/22-ZONA-DEL-JUEGO-MOMENTO-SEGURO.md` · último cambio cea786b 2026-10-10 · 14263 bytes. Copia íntegra.

**Investigación del 2026-10-10, de 00:09 a 00:25 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. Continúa el [17](17-OPCION-EN-VIVO-DONDE-ESTA.md) y el [19 · paso a paso](19-OPCION-EN-VIVO-PASO-A-PASO.md).

---

### 0. Resumen en 4 líneas

1. El juego guarda en la memoria un «cartel» con el modo en el que está el jugador. Es un número del 0 al 60.
2. El propio exe trae la lista de nombres: **7 = TOP_MENU (menú principal)**, 8 = EXHIBITION (Partido), 13 = EDIT (Editar), 19 = UEFA_ML (Liga Máster)…
3. `phoenix.lua` v0.17d lo lee de forma segura y dice si es un **momento seguro para recargar**. Probado en el juego: acertó en todos los sitios.
4. Esta versión **solo mira y anota**. No recarga nada.

---

### 1. Palabras nuevas

- **Gestor de modos:** el objeto del juego que sabe en qué modo se está (menú, Partido, Editar…).
- **Zona:** nombre que usamos para «la parte del juego donde está el jugador».
- **Momento seguro:** instante en el que recargar los datos no puede estropear nada: menú principal, sin cargas en marcha y fuera de Editar.
- **Evento de Sider:** aviso que Sider manda a los módulos cuando pasa algo (por ejemplo, «se eligieron los equipos»).

---

### 2. Dónde está el dato

| Qué | Dónde | Cómo se sabe |
|---|---|---|
| Puntero al gestor de modos | `exe+0x3704E38` (8 bytes) | Lo devuelve `0x149A470`: `mov rax, [exe+0x3704E38] ; ret`. La llaman 2.107 sitios |
| **Modo actual** | **número de 4 bytes en `[gestor+0xF0]`** | Lo lee `0x149F4F0`, que tiene 997 llamadores |
| Cambio de modo en curso | byte `[gestor+0x12C]` distinto de 0 | `0x149F510`: si no vale 0, usa el modo de `[gestor+0x130]` |
| Modo siguiente | 4 bytes en `[gestor+0x130]` | Igual que arriba |
| Tabla de nombres | `exe+0x34FC9C0`, 61 entradas de 16 bytes | Cada entrada: tipo (4 bytes), relleno y puntero al nombre |

#### La lista de modos (los que importan)

| N.º | Nombre en el exe | Qué es | Estado |
|---|---|---|---|
| 0 | UNKNOWN | Arrancando, todavía sin modo | ✅ visto |
| 5 | TITLE_DEMO_LOOP | Pantalla de título | ✅ visto |
| 6 | FIRST_SETTINGS | Carga inicial (aquí ocurre la segunda lectura de la base) | ✅ visto |
| **7** | **TOP_MENU** | **Menú principal** | ✅ visto |
| 8 | EXHIBITION | Partido | ✅ visto |
| 13 | EDIT | Editar | ✅ visto |
| 14 | EDIT_LIVEUPDATE | Editar → actualización en vivo | 🔎 leído en el exe |
| 19 | UEFA_ML | Liga Máster | 🔎 leído en el exe |
| 20 | UEFA_BL | Ser una Leyenda | 🔎 leído en el exe |
| 52 | FREE_TRAINING | Entrenamiento | 🔎 leído en el exe |

La lista completa (61 nombres) está dentro de `sider/pruebas/phoenix-v0.17d-prueba.lua`, en `NOMBRES_ZONA`.

---

### 3. La regla del «momento seguro»

`phoenix.lua` dice **SÍ** solo si se cumplen las cuatro condiciones a la vez:

| N.º | Condición | Dato que se lee |
|---|---|---|
| 1 | El modo es el menú principal | `[gestor de modos+0xF0] == 7` |
| 2 | No hay un cambio de modo en curso | `[gestor de modos+0x12C] == 0` |
| 3 | El gestor de edición no existe | `[exe+0x37F5C28] == 0` |
| 4 | La base está quieta | `[gestor de la base+0x30] == 4` |

Si falla una, dice **NO** y explica cuál. Si algún dato no se puede leer, también dice NO.

#### Antes de leer, se comprueba el código

| RVA | Bytes (hex) | Qué es |
|---|---|---|
| `0x149A470` | `48 8B 05 C1 A9 26 02 C3` | `mov rax, [exe+0x3704E38] ; ret` |
| `0x149F4F0` | `8B 81 F0 00 00 00 33 C9 83 F8 3D 0F 43 C1` | `mov eax, [rcx+0xF0] ; xor ecx, ecx ; cmp eax, 0x3D ; cmovae eax, ecx` |
| `0x149F510` | `80 B9 2C 01 00 00 00 74 08 8B 81 30 01 00 00 EB 06` | `cmp byte [rcx+0x12C], 0 ; je ; mov eax, [rcx+0x130] ; jmp` |
| `0x1EF2250` | `48 8B 05 D1 39 90 01 C3` | `mov rax, [exe+0x37F5C28] ; ret` |

Si alguno no coincide, no se lee nada y el overlay dice «exe distinto».

---

### 4. Cómo se encontró

1. En el estudio de la noche anterior ya había aparecido esta pareja de llamadas: `call 0x149A470` y luego `call 0x149F4F0`,
   justo antes de elegir entre `LiveDataSetFlowOnMode`, `…Exhibition` y `…OffMode` (función `0xFDF9D0`). Olía a «¿en qué modo estoy?».
2. Se desensamblaron las dos: son lectores muy simples. La primera devuelve un objeto global; la segunda lee `[objeto+0xF0]`
   y lo traduce con la tabla de `exe+0x34FC9C0`.
3. Se volcó la tabla: 61 entradas, cada una con un puntero a un texto. Los textos son los nombres de los modos.
4. Al lado está `0x149F510`, que muestra que durante un cambio de modo el juego usa `[+0x130]` si `[+0x12C]` no vale 0.
5. Para Editar ya se conocía el gestor de edición (`[exe+0x37F5C28]`, documento 07).
6. Para «la base está quieta» se reutilizó el estado del gestor de la base (documento 17).
7. Eventos de Sider: se miraron los nombres dentro de `sider.dll` 7.3.3. Existen `set_teams` y `context_reset`; no existe ningún evento de «entrar a Editar».

---

### 5. La versión de prueba v0.17d

Es la v0.17a más:

- la línea **[ZONA]** en el overlay;
- una muestra automática de la zona, como mucho **una por segundo**, cuando el juego lee algún archivo (solo después del arranque);
- los eventos `set_teams` y `context_reset` de Sider, que **solo se anotan**: no devuelven nada, así que no cambian nada del juego;
- una línea en `sider.log` cada vez que la zona cambia.

No escribe en la memoria. No recarga nada. No cambia los parches A, B y C.

#### Simulación (LuaJIT, memoria y relojes falsos)

| Caso | Resultado |
|---|---|
| D1. Menú → Partido → Editar → Liga Máster → menú | SÍ, NO, NO, NO, SÍ |
| D2. Menú principal pero base cargando, edición activa o cambio de modo | NO las tres veces; luego SÍ |
| D3. Exe distinto | No lee la zona nunca más |
| D4. Gestor de modos inexistente, puntero raro, zona ilegible | Avisa y no lee |
| D5. Eventos de Sider | No devuelven nada |
| D6. Ritmo: 200 archivos en el mismo segundo | Una sola muestra |
| D7. Gestor de la base ilegible | NO |

Más los 6 casos de la v0.17m y los 5 de la v0.17a: los 18 bien, 0 escrituras. Guion: `sider/pruebas/simular_v017d.py`.

#### Instalación (00:18)

1. FRALEX dio el «sí» a las 00:14, pero el juego estaba abierto. Se esperó.
2. 00:17:45 · Sider anotó «All done»: juego cerrado.
3. Respaldo de la v0.17a como `phoenix.lua.v017a`, en las dos carpetas `modules`.
4. Copia de la v0.17d a las dos carpetas.
5. Huella comprobada en el PC: `9755c842…17ffad` en las dos. Respaldos `v017`, `v017m` y `v017a` intactos.

---

### 6. Resultado en el juego (00:19–00:22)

Recorrido de FRALEX: abrir el juego → menú principal → Partido → menú → Editar → salir sin Guardar → menú.

| Hora | Zona leída | ¿Seguro? | Nota |
|---|---|---|---|
| 00:19:36 | UNKNOWN (0), base estado 1 | NO | Arranque |
| 00:19:45 | TITLE_DEMO_LOOP (5) | NO | Pantalla de título |
| 00:19:49 | FIRST_SETTINGS (6) | NO | Carga inicial |
| 00:19:51 | — | — | Segunda lectura de la base (modo 0) |
| 00:19:59 | **TOP_MENU (7)** | **SÍ** | Menú principal |
| 00:20:00 | **EXHIBITION (8)** | NO | Partido |
| 00:20:05 | — | — | Evento `context_reset` de Sider |
| 00:20:09 | **EDIT (13)**, edición no | NO | Entrando a Editar |
| 00:20:10 | EDIT (13), edición ACTIVA | NO | Editar ya cargado |
| 00:20:17 | — | — | Lectura de la base (modo 0) → «última recarga: NORMAL» |
| 00:21:07 | **TOP_MENU (7)** | **SÍ** | De vuelta en el menú |

Captura de FRALEX a las 00:22:02, en el menú principal:
«[ZONA] TOP_MENU (7) · edición no · base estado 4 · momento seguro: SÍ».

- `phoenix.lua`: 0 errores. Ningún error de otros módulos en esta sesión.
- El option file sigue idéntico (huella `3507ce35…`).

#### Lo que se aprendió

1. **El cartel funciona.** Cada nombre coincidió con el sitio donde estaba FRALEX.
2. **Solo dijo SÍ en el menú principal.** En Partido y en Editar dijo NO.
3. **La segunda lectura de la base del arranque ocurre en FIRST_SETTINGS.** Era la duda que quedó abierta en el documento 19.
4. **Al entrar a Editar, el cartel cambia antes que el gestor de edición** (un segundo de diferencia). Por eso conviene usar las dos señales.
5. **`SYSTEM00000000` cambia al guardar una carrera de Liga Máster.** Entre las 23:57 y las 00:11 aparecieron `ML00000003` y
   `ML00000004` (dos carreras de Liga Máster guardadas a esa hora), y `SYSTEM00000000` y `GRAPHICS000000` se reescribieron a las 00:11:41.
   No tiene que ver con «Datos Actual. en vivo».

#### Límite de la prueba (no del dato)

La muestra automática solo ocurre cuando el juego lee algún archivo. Al volver de Partido al menú (00:20:05) el juego no leyó
nada durante 4 segundos, y esa vuelta no quedó anotada. Con el overlay abierto sí se mira todo el tiempo.
Para el uso real no importa: `phoenix.lua` leerá el cartel en el momento en que quiera recargar.

---

### 7. Lo que falta

1. Ver el cartel en Liga Máster (19), Ser una Leyenda (20), Entrenamiento (52) y **dentro de un partido** (¿sigue en 8?).
2. Repetir el recorrido otro día: hoy se hizo una vez.
3. Decidir, con FRALEX, cómo usar el «SÍ»: por ejemplo, recargar solo si la última recarga fue ACTIVAR y el momento es seguro.
   Eso ya no sería solo lectura y necesita su aprobación.
4. Experimento B (sin el parche C).

---

### 8. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Versión instalada ahora | `sider/pruebas/phoenix-v0.17d-prueba.lua` (`9755c842…`) |
| Simulación | `sider/pruebas/simular_v017d.py` (junto a `simular_v017m.py`; necesita `pip install lupa`) |
| Versión estable | `sider/phoenix.lua` = v0.17 (`d85e1071…`), sin cambios |
| Respaldos en el PC | `phoenix.lua.v017`, `.v017m` y `.v017a`, en las dos carpetas `modules` |
| Diario de pruebas | `PRUEBAS.md`, entradas del 2026-10-10 de 00:18 y 00:19 |

**Para volver atrás:** con el juego cerrado, copiar `phoenix.lua.v017a` (o `phoenix.lua.v017`) encima de `phoenix.lua` en las dos carpetas `modules`.

---

### 9. Segundo recorrido (00:43–00:49) · añadido a las 00:55

Para cerrar los cabos sueltos del apartado 7. FRALEX dejó el overlay abierto casi todo el recorrido, así que la zona se miró
en cada cuadro y quedaron anotados todos los cambios. Sesión que arrancó a las 00:31:28.

| Hora | Zona leída | ¿Seguro? | Nota |
|---|---|---|---|
| 00:32:25 | TOP_MENU (7) | SÍ | Menú principal tras arrancar |
| 00:33:22 | **UEFA_ML (19)** | NO | Liga Máster (primera vez que se ve) |
| 00:43:51 | UEFA_ML (19), base estado 3 → 4 | NO | Saliendo de Liga Máster |
| 00:43:51 | TOP_MENU (7) | SÍ | Mismo segundo que el evento `context_reset` |
| 00:43:54 | EXHIBITION (8) | NO | Partido |
| 00:44:03 | TOP_MENU (7) | SÍ | Vuelta al menú |
| 00:44:39 | UEFA_ML (19), base estado 2 → 4 | NO | Liga Máster otra vez |
| 00:45:00 | TOP_MENU (7) | SÍ | Vuelta al menú |
| 00:45:04 | **UEFA_BL (20)**, base estado 2 → 4 | NO | Ser una Leyenda (primera vez que se ve) |
| 00:45:19 | UEFA_BL (20), edición ACTIVA | NO | Ser una Leyenda carga el gestor de edición |
| 00:46:23 | TOP_MENU (7) | SÍ | Vuelta al menú |
| 00:46:47 | EXHIBITION (8) | NO | Partido |
| 00:46:50 | — | — | Evento `set_teams`: 109 vs 172 (Real Madrid – Atlético) |
| 00:48:00 | **EXHIBITION (8)** | NO | **Dentro del partido, en pausa** (captura de FRALEX) |
| 00:49:05 | TOP_MENU (7) | SÍ | Vuelta al menú tras el partido |
| 00:49:09 | EDIT (13) | NO | Editar |
| 00:49:10 | EDIT (13), edición ACTIVA | NO | Editar ya cargado |
| 00:49:21 | TOP_MENU (7) | SÍ | Vuelta al menú (captura de FRALEX a las 00:49:27) |

#### Lo que confirma

1. **Liga Máster = 19 y Ser una Leyenda = 20**, como decía la tabla del exe. Ya están vistos en el juego.
2. **Dentro de un partido el cartel sigue en EXHIBITION (8).** No hay un número distinto para «pelota en juego».
3. **«Momento seguro: SÍ» salió 7 veces en esta sesión, siempre en el menú principal y nunca en otro sitio.** Sumadas a las 2 del primer recorrido: 9 de 9.
4. Al salir de un modo, el evento `context_reset` de Sider y el paso a TOP_MENU ocurren en el mismo segundo.
5. Estados nuevos del gestor de la base: **2** al entrar a Liga Máster o a Ser una Leyenda y **3** al salir. Duran menos de un segundo y luego vuelve a 4.
   En esos momentos el modo todavía no es TOP_MENU, así que la regla ya los cubre. (🔎 observado; no se estudió qué significan.)
6. Ser una Leyenda también activa el gestor de edición.
7. `SYSTEM00000000` se reescribió a las 00:49:03, al terminar el partido. El option file sigue idéntico (`3507ce35…`).

#### ⚠️ Hallazgo de seguridad: las teclas de prueba siguen vivas con el overlay abierto

Entre las 00:45:20 y las 00:45:35, con el overlay abierto, se pulsó la tecla **K**. `sider.log` anotó:
«NO se llamó: ya hay una relectura en curso (o no se pudo leer el gestor)».

- **No pasó nada:** el código vio que ya había una relectura en marcha y no llamó a ninguna función del juego.
- **Pero pudo pasar:** K es la «prueba B» de la v0.12/v0.13. Si se pulsa en el menú principal, crea el gestor de edición y llama a
  la relectura de la base (`0x1EEBBA0` y `0x1EF2FA0`). Eso ya no es solo leer.
- Lo mismo vale para **L** (escribe el interruptor de recarga), **P** (cambia 1 byte de código en memoria) y **U** (reaplica los
  parches A/B/C). **B** solo busca en la memoria. Solo actúan con el overlay abierto en el módulo Phoenix.
- Estas teclas están en **todas** las versiones desde la v0.11, también en la v0.17.
- **Recomendación (pendiente de aprobación de FRALEX):** una versión que desactive K, L, P, U y B, o que pida una combinación
  difícil de pulsar por accidente. Mientras tanto, no pulsar letras con el overlay abierto.

#### Aparte

`StadiumServer.lua` volvió a dar 32 errores (línea 705) al preparar el partido de las 00:46. Confirma que ese fallo es de la
preparación del partido y no de `phoenix.lua`.

---

## Parte 7 · 30 · Experimento B (phoenix.lua v0.17e-B) — cómo replicarlo desde cero

> Origen: `PhoenixSync/base-conocimiento/30-EXPERIMENTO-B-COMO-REPLICAR.md` · último cambio 97febd2 2026-10-10 · 9466 bytes. Copia íntegra.

Escrito el 2026-10-10 a las 09:55 (Lima), a pedido de FRALEX: «guarda perfectamente cómo se llega al resultado de la v0.17e-B para que otra IA lo replique si es necesario».

Esta guía está pensada para que **otra persona u otra IA** pueda reconstruir la v0.17e-B y obtener el mismo resultado sin haber visto la conversación.

---

### 1. Qué es, en una frase

La v0.17e-B es la v0.17e de phoenix.lua con **un solo cambio**: al arrancar el juego aplica dos de los tres parches del botón nativo «Datos Actual. en vivo» (A y B). El tercero (C) **no lo aplica**.

Resultado: al pulsar **Activar**, el juego carga los equipos de la base de Phoenix-DB **y se queda de verdad en «Activar»**. La pantalla muestra «Selección actual: Activar Actualización en vivo». Con la v0.17e (A+B+C), en cambio, siempre volvía a «Desactivar».

---

### 2. Requisitos (si algo no coincide, el resultado puede ser otro)

| Cosa | Valor exacto |
|---|---|
| Juego | PES 2021 (eFootball PES 2021 SEASON UPDATE), `PES2021.exe` sha256 `5e27a782…bc224a` |
| Dirección base del exe | `0x140000000` (las direcciones de abajo son relativas: `exe+…`) |
| Sider | 7.3.3, con módulos Lua (LuaJIT, **sin `pcall`**) |
| Archivo de partida | `sider/pruebas/phoenix-v0.17e-prueba.lua`, sha256 `114b6f2ee9d8ac64465a62549f23879179e125fc4178096295a64df8dc17c069` |
| Archivo final | `sider/pruebas/phoenix-v0.17e-B-prueba.lua`, sha256 `e276c72e7cb133c177db9c439501dc772591b32bfb689bb75c78cf870279a811` |
| Simulación | `sider/pruebas/simular_expB.py`, sha256 `dce0e5252769504dbe6e19696a0ce5e7b95ccbcb7182732eecf75878502f2952` |

Todos los parches se hacen **solo en la memoria** del juego. El `.exe` del disco nunca se toca. Al cerrar el juego, todo desaparece.

---

### 3. Los tres parches del botón nativo (para entender qué se quita)

Están en la tabla `PARCHES16` de phoenix.lua (función `botonEnElSitio`). Se aplican solos la primera vez que el juego lee un archivo de la base (`livecpk_read`, variable `autoHecho`).

Antes de escribir nada, phoenix.lua comprueba que **los tres** sitios tienen los bytes esperados. Si uno no coincide, no escribe nada.

#### Parche A — saltar el inicio de sesión en Konami
- Sitio: `exe+0x20AF73B` (flujo común `ProcessCmnLiveDataSetFlow`, estado 1).
- Bytes originales aceptados:
  - `48 83 bf 98 00 00 00 00 75 6b 33 db 89 5c 24 28 c7 44 24` (juego limpio), o
  - `c6 05 f7 64 74 01 01 c7 87 94 00 00 00 07 00 00 00 eb 6c` (parche viejo de la v0.15).
- Bytes nuevos: `c7 87 94 00 00 00 03 00 00 00 eb 73` → `mov dword [rdi+0x94], 3 ; jmp fin`.
- Efecto: va directo a crear `LiveDataSetFlow` sin pedir inicio de sesión (los servidores de Konami están cerrados).

#### Parche B — saltar la petición al servidor y mostrar «éxito»
- Sitio: `exe+0x20AC6B9` (`LiveDataSetFlow`, estado 5).
- Original: `e8 52 e2 09 ff 48 85 c0 74 10 48 8d 15 16 53`.
- Nuevo: `c7 87 94 00 00 00 16 00 00 00 e9 ba 25 00 00` → `mov dword [rdi+0x94], 0x16 ; jmp 0x20AEC82`.
- Efecto: sin internet, salta al diálogo de éxito (estado 22, mensaje `0xF90042`). En ese paso el juego recarga la base en **modo 1** (activado).

#### Parche C — el que el experimento B NO aplica
- Sitio: `exe+0x20AE664` (`LiveDataSetFlow`, estado 26, «editLoadDataInLiveDataSet»).
- Original: `66 c7 44 24 30 00 01` → `mov word [rsp+0x30], 0x100`.
- Nuevo (solo en la v0.17e): `66 c7 44 24 30 01 01` → `0x101`.
- Efecto con C: la carga del estado 26 también relee la **base**, pero en **modo 0**. Por eso, con A+B+C, Activar hace **dos** recargas (modo 1 y luego modo 0) y el juego termina diciendo «Desactivar».
- Sin C: el estado 26 carga solo el option file (`0x100`), sin base. **Una sola** recarga, y el modo se queda en 1.

---

### 4. Paso a paso para construir la v0.17e-B desde la v0.17e

Son exactamente 3 cambios (el `diff` completo tiene 25 líneas):

**Paso 1.** Cambiar la versión (línea 21):
```lua
local m = { version = "0.17e-B" }
```

**Paso 2.** Justo **antes** de `local function botonEnElSitio()`, añadir el interruptor del experimento:
```lua
-- EXPERIMENTO B: aplicar SOLO los parches A y B. El C no se escribe (sus bytes sí se comprueban).
-- Para volver al comportamiento de la v0.17e: poner false.
local EXPERIMENTO_B = true
```

**Paso 3.** Dentro de `botonEnElSitio()`, en el bucle «2) escribir y verificar», sustituir la línea
```lua
        if leerSeguro(base + pt.rva, #pt.nuevo) ~= pt.nuevo then
```
por
```lua
        if EXPERIMENTO_B and pt.nombre == "C" then
            if leerSeguro(base + pt.rva, #pt.nuevo) == pt.nuevo then
                log("[phoenix] experimento B: el parche C YA estaba puesto en esta sesión; no se quita (cerrar y abrir el juego)")
            else
                log("[phoenix] experimento B: parche C NO aplicado (a propósito)")
            end
        elseif leerSeguro(base + pt.rva, #pt.nuevo) ~= pt.nuevo then
```
Al final de la función, añadir al texto de `estadoBoton`:
```lua
        .. (EXPERIMENTO_B and "  ·  EXPERIMENTO B: sin parche C" or "")
```

**Importante:** el bucle «1) comprobar TODO» **no se cambia**. Los bytes de C se siguen comprobando, porque sirven para confirmar que el exe es el esperado.

**Paso 4.** Comprobar que el sha256 del archivo resultante es `e276c72e…a811` (sección 2). Si no coincide, compara con `diff` contra el archivo del repo.

---

### 5. Comprobación sin juego (simulación)

`python3 sider/pruebas/simular_expB.py` (necesita `lupa` con LuaJIT). Ejecuta phoenix.lua con una memoria falsa y cuenta las lecturas y escrituras.

Debe dar:
- Con bytes originales: escribe **solo** en `0x20AF73B` (A) y `0x20AC6B9` (B). C queda igual.
- La v0.17e, en la misma situación, escribe los tres.
- Si C ya estaba puesto: no lo toca (no lo quita).
- Los 18 casos anteriores y la prueba de teclas siguen bien.

---

### 6. Instalación (como se hizo el 2026-10-10 a las 09:25)

1. **Cerrar el juego** (en `sider.log`, Sider escribe «All done»).
2. Hacer una copia de respaldo del `phoenix.lua` instalado en **las dos** carpetas `SiderAddons\modules\` del parche ConmeGol, con nombre `phoenix.lua.v017e`.
3. Copiar la v0.17e-B con el nombre `phoenix.lua` en las dos carpetas.
4. Comprobar el sha256 en el PC: debe ser `e276c72e…a811` en las dos.
5. Comprobar que el respaldo `.v017` (la versión estable) sigue intacto.

---

### 7. Prueba en el juego y resultado esperado

1. Abrir el juego. En `sider.log` deben salir: «parche A aplicado», «parche B aplicado» y «experimento B: parche C NO aplicado (a propósito)».
2. Partido → Datos Actual. en vivo → **Activar**. Debe salir el mensaje de éxito.
3. Volver a entrar a «Datos Actual. en vivo»: debe decir **«Selección actual: Activar Actualización en vivo»**.
4. Overlay de Sider (módulo Phoenix): `[MODO] opción en vivo: ACTIVADA (1) · gestor estado 4`, `[ÚLTIMA RECARGA] ACTIVAR`, `[BOTÓN] … EXPERIMENTO B: sin parche C`.
5. En `sider.log`: **una sola** relectura de la base (28 archivos) en modo 1.

#### Resultado comprobado por FRALEX (2026-10-10 09:28–09:46), ya corregido

| Lo que pulsas | Equipos que salen | Qué dice la pantalla | Modo en memoria `[[exe+0x3705E10]+0x38]` |
|---|---|---|---|
| **Activar** | Base de Phoenix-DB (Barça con Vinícius y Mbappé; flechas de forma física) | «Activar» | 1 |
| **Editar** (solo entrar) | Option file `EDIT00000000` (Mbappé en el Barça, Vinícius en el Madrid). Relee 11 archivos | sigue diciendo «Activar» ⚠️ | sigue en 1 ⚠️ |
| **Desactivar** | Option file (Vinícius al Madrid; Mbappé en el Barça, de suplente). No relee archivos | «Desactivar» | 0 |

**Corrección registrada:** a las 09:42 se dijo que había una «tercera versión» del Barça. Era falso: Mbappé estaba más abajo en la lista. **Regla:** para saber si un jugador está en un equipo, mirar siempre la lista **completa**.

---

### 8. Problemas conocidos (por qué no es todavía la versión principal)

1. **Después de Editar, el modo dice 1 pero los equipos son del option file.** La «última recarga» de phoenix.lua (opción A, guía 19) se equivoca en ese caso. Posible arreglo, sin hacer todavía: detectar la relectura de 11 archivos de Editar, o la zona EDIT (13), y marcar NORMAL.
2. **Las stats nuevas sin C no están probadas.** Haría falta cambiar `Player.bin` de Phoenix-DB con el juego abierto y pulsar Activar.
3. **El Barça visitante** en partidos Barça–Barça: ❓ sin confirmar si es el del option file.

---

### 9. Cómo volver atrás

- **Rápido, sin tocar archivos:** cerrar el juego (los parches viven solo en la memoria). Si C ya se puso en una sesión, solo se quita cerrando el juego.
- **Al comportamiento de la v0.17e:** poner `EXPERIMENTO_B = false`, o instalar el respaldo `phoenix.lua.v017e` (sha256 `114b6f2e…`).
- **A la versión estable:** respaldo `phoenix.lua.v017` (sha256 `d85e1071…`), que es también `sider/phoenix.lua` del repo.

---

### 10. Dónde está cada prueba

- `PRUEBAS.md`: entradas del 2026-10-10 09:25, 09:26, 09:28–09:32, 09:32–09:42 y la corrección de 09:46.
- `REGISTRO.md`: líneas de las mismas horas.
- Guías relacionadas: 17 (dónde vive la opción), 19 (paso a paso y opción A), 22 (zona y momento seguro).

---

## Parte 8 · 31 · phoenix.lua v0.18 — los tres modos de recarga: dónde, cómo y cuándo se aplica cada uno

> Origen: `PhoenixSync/base-conocimiento/31-MODOS-DE-RECARGA.md` · último cambio 0387464 2026-10-10 · 12062 bytes. Copia íntegra.

Escrito el 2026-10-10 a las 10:25 (Lima). Pedido de FRALEX:
- 09:50: «quiero el que sea automático y el que haga los fichajes cuando le dé en Activar, y una tecla para elegir el modo».
- 10:10: «dependería de la elección del usuario, poniendo un botón en Link».
- 10:12: «deja más claro la diferencia de cada botón y cuál viene por defecto».

**Estado: escrita y simulada, SIN instalar.** En el PC sigue la v0.17e-B.

---

### 1. Los tres modos, en palabras simples

#### 🟢 Modo 1 · ACTIVAR · ⭐ VIENE POR DEFECTO
- **Qué haces tú:** Partido → Datos Actual. en vivo → **Activar**.
- **Qué hace el juego:** recarga los fichajes **solo cuando tú lo pides**.
- **Ejemplo:** una lámpara con interruptor. Solo se enciende si tú la pulsas.
- Es **exactamente la v0.17e**, ya probada. Después de Activar la pantalla dice «Desactivar», pero eso no importa.
- **Por qué es el de por defecto:** es el probado. Además, si no hay elección (falta `modo.txt` o dice algo raro), o si lo automático falla, el módulo usa este.

#### 🔵 Modo 2 · AUTO-FICHAJES (automático, solo con fichajes nuevos)
- **Qué haces tú:** nada (Activar sigue funcionando si lo quieres usar).
- **Qué hace el juego:** cuando Phoenix Sync avisa «hay fichajes nuevos», el juego recarga solo **la próxima vez que estés en el menú principal y entres a un modo**. Si no hay nada nuevo, no recarga.
- **Ejemplo:** un cartero. Solo toca el timbre cuando hay una carta.

#### 🟣 Modo 3 · AUTO-SIEMPRE (automático, siempre)
- **Qué haces tú:** nada (Activar sigue funcionando).
- **Qué hace el juego:** **cada vez** que vuelves al menú principal desde un modo y entras a otro, recarga, haya algo nuevo o no.
- **Ejemplo:** mirar el buzón cada vez que pasas por la puerta.
- La **primera** entrada después de abrir el juego no recarga, porque la base se acaba de cargar.
- El juego tarda un poco más en entrar a cada modo.

| Modo | ¿Recarga solo? | ¿Cuándo? |
|---|---|---|
| 🟢 ACTIVAR ⭐ (por defecto) | No | Solo al pulsar Activar |
| 🔵 AUTO-FICHAJES | Sí | Solo si hay fichajes nuevos |
| 🟣 AUTO-SIEMPRE | Sí | Cada vez que vuelves al menú y entras a un modo |

**En los tres, el botón Activar del juego sigue funcionando.**

---

### 2. DÓNDE se elige el modo

Todo vive en la carpeta **buzón** de Phoenix, la misma del archivo de avisos:

```
<juego>\SiderAddons\content\phoenix\
    avisos.txt      ← ya existía (mensajes de la web en el overlay)
    modo.txt        ← NUEVO: el modo elegido
    recargar.txt    ← NUEVO: el aviso «hay fichajes nuevos»
```

En el PC de FRALEX, la carpeta que usa el juego es `D:\Frank\Games_\Conmegol Patch\SiderAddons\content\phoenix\`. Ojo: el switcher de ConmeGOL copia todo desde `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`, así que conviene tener los archivos también ahí (ver guía VINCULO-TIEMPO-REAL).

#### `modo.txt`: la notita del modo
- **Quién lo escribe:** Phoenix Link, cuando el usuario pulsa el botón de modo. Mientras el botón no exista, `PhoenixModo.bat`.
- **Qué contiene:** una sola palabra: `ACTIVAR`, `AUTO-FICHAJES` o `AUTO-SIEMPRE`. Da igual mayúsculas o minúsculas; se acepta `_` en vez de `-`, y un BOM o un salto de línea al final.
- **Cómo se escribe:** de forma atómica (primero a `modo.tmp`, luego se renombra), en UTF-8 sin BOM, de 64 bytes como mucho.
- Si **no existe** o dice otra cosa → **ACTIVAR**.

#### `recargar.txt`: la notita de «hay fichajes nuevos»
- **Quién lo escribe:** Phoenix Sync, cada vez que mete fichajes nuevos en el option file. Mientras no lo haga, `PhoenixRecargar.bat`.
- **Qué contiene:** cualquier texto **distinto del anterior**, de 256 bytes como mucho (por ejemplo la fecha y hora). Lo que importa es que **cambie**.
- Escritura atómica igual que `modo.txt`.
- Lo que ya hubiera al abrir el juego **no cuenta** (la base se acaba de cargar). Con el modo ACTIVAR, los avisos se ignoran.

#### La tecla T: para probar
- Con el overlay de Sider abierto (Espacio, pasar al módulo Phoenix), **T** pasa de un modo al siguiente: ACTIVAR → AUTO-FICHAJES → AUTO-SIEMPRE → ACTIVAR.
- Vale **solo durante esa sesión de juego**. Si `modo.txt` cambia después (Link eligió otra vez), manda `modo.txt`.
- No es una tecla de prueba peligrosa: solo cambia una elección; no escribe en el juego por sí misma.

---

### 3. CÓMO funciona por dentro (para otra IA o una persona técnica)

Archivo: `PhoenixSync/sider/pruebas/phoenix-v0.18-prueba.lua` (sha256 en §6). Es la v0.17e más un bloque «v0.18».

1. **Arranque:** como la v0.17e. En la primera lectura de la base (`livecpk_read`, `autoHecho`), aplica los parches A, B y C del botón nativo (guía 30). Antes de eso, lo automático no hace nada.
2. **Evento nuevo `display_frame`:** Sider lo llama en cada cuadro de imagen. El trabajo real se hace **como mucho cada 2 segundos** (`os.clock`):
   - Lee `modo.txt` y `recargar.txt` (dos archivos diminutos).
   - **En ACTIVAR, no hace nada más:** ni lee ni escribe la memoria del juego.
   - En un modo automático, mira la zona del juego (solo lectura, guía 22) y el interruptor `exe+0x37F5C39` (solo lectura).
3. **Cuándo decide recargar:**
   - AUTO-FICHAJES: si `recargar.txt` cambió.
   - AUTO-SIEMPRE: si desde la última vez se vio una zona de juego (id > 7: Partido 8, Editar 13, Liga Máster 19…), o si llegó un aviso.
4. **Solo en el MOMENTO SEGURO** (guía 22): zona = TOP_MENU (7), sin cambio de modo en curso, gestor de edición `[exe+0x37F5C28]` = 0, gestor de la base en estado 4 (quieto). **Nunca en un partido ni en Editar.**
5. **Qué escribe:**
   - Primero comprueba los bytes de código del interruptor (`0xAEF770`, `0x1EFB440`) **antes de escribir nada**.
   - Luego hace lo mismo que la vieja tecla **P** (`recargaCompleta`, probada el 2026-10-09 a las 05:30): 1 byte de código `exe+0xAEF78E` 00→01 (con su propia comprobación de los 8 bytes de `0xAEF78A`; se hace una sola vez por sesión) y el interruptor `exe+0x37F5C39` 0→1.
   - Si el interruptor ya estaba en 1 (lo encendió el juego, p. ej. Ser una Leyenda), no escribe nada.
6. **Quién recarga de verdad:** el propio juego. Al entrar a un modo desde el menú principal ve el interruptor en 1, crea la tarea `editLoadDataInTopMenu` (la misma de Editar → Cargar) y lo pone a 0. Con el byte de la P, relee **EDIT + toda la base**. phoenix.lua nota el 1→0 y anota «recarga automática hecha».
7. **Seguridad:**
   - Bandera propia de errores en `display_frame`.
   - 3 fallos (error de Lua o interruptor que no se puede encender) → lo automático se **apaga hasta reiniciar el juego** y queda ACTIVAR.
   - Todo vive en la memoria: al cerrar el juego, desaparece.
8. **Overlay:** línea nueva `[MODO PHOENIX] <modo> · <de dónde salió la elección> · T = cambiar`, la explicación del modo y, en los automáticos, `[AUTO] <estado>`: esperando, pendiente (esperando el momento seguro), interruptor encendido HH:MM:SS o ✓ el juego hizo la recarga.
9. **sider.log:** líneas `[phoenix] v0.18 …` (como mucho 300): modo leído, tecla T, aviso de fichajes, interruptor encendido, recarga hecha, fallos.

---

### 4. CUÁNDO se aplica cada cosa (la línea de tiempo)

**Cambiar de modo (botón de Link, `.bat` o T):** el juego lo nota en **unos 2 segundos**, abierto o no (si está cerrado, al abrirlo).

**AUTO-FICHAJES, ejemplo:**
1. 10:00 · Estás en el menú principal. Phoenix Sync mete fichajes y cambia `recargar.txt`.
2. ~10:00:02 · phoenix.lua lo nota. Si es momento seguro, enciende el interruptor.
3. 10:01 · Entras a Partido → **el juego recarga** → ya juegas con los fichajes.

Si el aviso llega **mientras juegas un partido**, espera: cuando vuelves al menú principal enciende el interruptor, y la recarga ocurre al entrar al **siguiente** modo.

**AUTO-SIEMPRE, ejemplo:**
1. Abres el juego → menú → Partido: **no recarga** (la base es recién cargada).
2. Terminas → vuelves al menú → phoenix.lua enciende el interruptor.
3. Entras a Partido, Liga Máster o lo que sea → **recarga**.
4. Y así cada vuelta.

**ACTIVAR:** solo cuando pulsas Activar (recarga en el sitio, con el mensaje de éxito).

---

### 5. Lo que hay que comprobar en el juego (todavía no probado)

1. Que `display_frame` llegue también en el menú principal (si no, lo automático solo avanza con el overlay abierto o al leer archivos).
2. ACTIVAR: igual que la v0.17e (Activar, mensaje, fichajes).
3. AUTO-SIEMPRE con Partido: menú → Partido (no recarga) → menú → Partido (recarga; el log debe decir «interruptor encendido» y «recarga automática hecha»).
4. AUTO-FICHAJES: doble clic en `PhoenixRecargar.bat` en el menú → Partido → recarga. Sin aviso → no recarga.
5. **Qué equipos salen tras la recarga automática.** Es la misma recarga de la tecla P, que en las pruebas del 2026-10-09 trajo los fichajes del option file y las stats de la base. Hay que verlo con la lista **completa** de jugadores.
6. **Liga Máster: NO probar al principio.** Lo automático nunca escribe dentro de Liga Máster, pero sí puede dejar pedida una recarga para cuando vuelvas a entrar. Primero solo con Partido y sin guardar.

---

### 6. Archivos y huellas

| Archivo | Para qué | sha256 |
|---|---|---|
| `PhoenixSync/sider/pruebas/phoenix-v0.18-prueba.lua` | el módulo | `a39b54de9f6a579d539ecdc432e14bbc2b32b982c14f82d16e1f304651e4f361` |
| `PhoenixSync/sider/pruebas/simular_v018.py` | simulación (9 casos) | `5efb8b6f23294e3b1d449dd40709cd83939bfe3f37832f04dafb5aa2064d8b06` |
| `PhoenixSync/sider/PhoenixModo.bat` | elegir modo (1/2/3) mientras no hay botón en Link | `8e7002b662c2b0c4f17012e4f6aef262c8e0d42a254b13699ebf9529ec0ecb7b` |
| `PhoenixSync/sider/PhoenixRecargar.bat` | simular «hay fichajes nuevos» | `da68c792bd8336ab3f55aa5dab597c1a4d9667e891dfd0ffecac9d3cde58f7de` |

Los dos `.bat` van dentro de `content\phoenix\` y escriben en su propia carpeta (`%~dp0`), de forma atómica.

#### Simulación (sin juego, LuaJIT con memoria, archivos y reloj falsos): 9 de 9 bien
- S1 sin `modo.txt` → ACTIVAR: tras el arranque, 0 escrituras y **0 lecturas de memoria** en 18 pasos.
- S2 AUTO-SIEMPRE: 0 escrituras en el primer menú y en Partido; al volver al menú, `AEF78E` + `37F5C39`; no repite; en la segunda vuelta solo `37F5C39`.
- S3 AUTO-FICHAJES: el aviso que ya había no cuenta; aviso nuevo en Partido → espera; menú con la base cargando → espera; momento seguro → enciende; sin aviso nuevo → nada más.
- S4 `modo.txt` raro → ACTIVAR; T cambia de modo; cuando Link reescribe `modo.txt`, manda `modo.txt` (también con BOM).
- S5 bytes del interruptor distintos → **0 escrituras**; a los 3 fallos se apaga lo automático y queda ACTIVAR.
- S6 nunca escribe en Partido, Editar (con gestor de edición vivo), Liga Máster ni cambiando de modo.
- S7 interruptor ya encendido por el juego → no escribe.
- S8 antes del arranque → no toca la memoria.
- S9 300 cuadros en el mismo segundo = un solo paso; tras Shift+R funciona.
- Además pasan con la v0.18 las simulaciones viejas: v0.17m, v0.17a, v0.17d (7 de 7), v0.17e (teclas de prueba ignoradas) y la del experimento B (la v0.18 aplica A+B+C como la v0.17e).

---

### 7. Lo que falta para que el botón exista en Phoenix Link

Link es **el otro frente** (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`), así que desde aquí no se toca. Ya quedó:
- El **contrato** de `modo.txt` y `recargar.txt` en `COORDINACION.md` (sección «Contratos compartidos»).
- Un **prompt** para el chat de Link: `PhoenixSync/prompts/PROMPT-LINK-modo-recarga.md` (botón de tres opciones, con ACTIVAR por defecto y los textos de cada una).

Phoenix Sync escribirá `recargar.txt` cuando se conecte la entrega de fichajes (pendiente en este frente).

---
