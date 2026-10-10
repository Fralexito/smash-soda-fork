# 27 · Informe completo: módulos Lua de Sider, carpetas y sider.ini, para construir el instalador de Phoenix Link

**Para FRALEX y para la IA que construya el instalador.**
**Fecha:** 10 de octubre de 2026, 05:30 (Lima).
**Autor:** chat SYNC, cuenta B, rama `mercado-fase0`.
**Fuentes:**
- el diario `PhoenixSync/PRUEBAS.md` (8 → 10 oct);
- `PhoenixSync/sider/` (`RIESGOS-SIDER.md`, `VINCULO-TIEMPO-REAL.md`, `AUDITORIA-SIDER.md`);
- `PhoenixSync/core/RutasJuego.cpp`;
- las guías 05, 12 y `parches/*.md`;
- una lectura **en vivo** del PC de FRALEX hecha hoy a las 05:15 (carpetas, `sider.ini`, `sider.log` y huellas).

**Etiquetas:**
- ✅ **PROBADO**: visto funcionar en el juego.
- 🔎 **VISTO**: comprobado en archivos o en el log, sin prueba en pantalla.
- **[DEDUCIDO]**: lo deduzco de cómo funciona algo, pero no lo vi pasar.
- **[NO PROBADO]**: no lo sé o no lo comprobé.

---

## 0. Antes de empezar: qué cubre este informe y qué no

Hay dos familias de `phoenix.lua`, y son muy distintas.

| Familia | Versiones | Qué hace | ¿Entra en el instalador? |
|---|---|---|---|
| **Puente de avisos (solo lectura)** | **v0.2** | Lee un archivo de texto (`avisos.txt`) y lo muestra en el overlay de Sider. No toca la memoria del juego ni sus archivos. | ✅ **Sí.** Es la que documento entera, con su código (sección 8). |
| **Versiones de memoria** | v0.3 a v0.17e (y las de prueba `v0.17a`, `v0.17d`, `v0.17m`) | Leen y **escriben la memoria del juego** con funciones de Windows. Las de la v0.11 en adelante, además, **modifican código del juego en memoria** al arrancar, para activar el botón nativo «Datos Actual. en vivo», que depende del servidor de Konami. | ❌ **No.** No voy a dar instrucciones para instalarlas ni su código. Si las incluyo, el instalador estaría repartiendo a otros PCs un módulo que altera el código del juego mientras corre. Aparte de esa razón mía, es la parte de más riesgo: una lectura directa ya cerró el juego una vez (v0.3, 9 oct, 02:43). |

**Lo que sí funciona sin esas versiones**, y es lo que este informe sí cubre:
- avisos de la web en el juego (v0.2);
- stats y plantillas nuevas con la raíz **Phoenix-DB** + el botón del propio juego **Editar → Cargar** (✅ 9 oct, 03:53);
- todo lo de la Liga Máster, que se hace sobre el guardado con el juego cerrado.

**Recomendación para el instalador:** instalar **solo** `phoenix.lua` v0.2. Hoy en el PC de FRALEX está instalada otra versión, la v0.17e. Que el instalador la sustituya o no lo decide FRALEX. Ver sección 1.3.

---

## 1. MÓDULOS LUA

### 1.1 · Lista de TODOS los archivos `.lua` de Phoenix

#### En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

| Archivo | Versión | Familia | Estado |
|---|---|---|---|
| `PhoenixSync/sider/phoenix.lua` | 0.17 | memoria | Archivo de trabajo de la otra cuenta. Fuera del instalador. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17a-prueba.lua` | 0.17a | memoria | Prueba. Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17d-prueba.lua` | 0.17d | memoria | Prueba. Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17e-prueba.lua` | 0.17e | memoria | Prueba. **Instalada hoy en el PC** (sha256 `114b6f2e…`). Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17m-prueba.lua` | 0.17m | memoria | Prueba. Fuera. |
| **v0.2 en el historial de git**, commit `4683988`, ruta `PhoenixMercado/sider/phoenix.lua` | **0.2-prueba** | **avisos** | ✅ **La probada en el juego** (sha256 `116b40a6…`, 5.806 bytes). |
| v0.2 en el historial, commit `f75f677`, ruta `PhoenixSync/sider/phoenix.lua` | 0.2-prueba | avisos | Idéntica a la anterior, salvo **una línea de comentario** (dice «Phoenix Sync» en vez de «Phoenix Mercado»). sha256 `974257ed…`. [NO PROBADO] en el juego, aunque el código es el mismo. |

No hay ningún otro `.lua` de Phoenix. Los otros ~60 módulos de la carpeta `modules` son **del parche** (ConmeGOL) o los añadió FRALEX a mano: el instalador no debe tocarlos.

#### En el PC (las dos carpetas `modules`, raíz y modo)

- `phoenix.lua`: hoy es la v0.17e.
- Copias de respaldo con sufijo:
  - `phoenix.lua.v02`: la **v0.2 probada**, sha256 `116b40a6…`;
  - `.v010`, `.v011`, `.v012`, `.v013`, `.v014`, `.v015b`, `.v016`, `.v017`, `.v017a`, `.v017d`, `.v017m`.
  - Sider **no** carga esas copias, porque solo carga lo que nombra `sider.ini`.

### 1.2 · Para qué sirve la v0.2 y qué pasa si falta

**Para qué sirve:** muestra en el overlay de Sider los avisos que Phoenix Link deja en un archivo. Es el «buzón» web → juego.

**Cómo se ve:**
1. **Espacio** abre el overlay.
2. La tecla **1** pasa al módulo siguiente, y **º** (`0xC0`) al anterior, hasta llegar a «PHOENIX EVOLUTION».

**Si falta el archivo `phoenix.lua`:**
- Sider anota `PROBLEM: Unable to open file` en `sider.log` y sigue funcionando. 🔎 (7 módulos de ConmeGOL faltan hoy y el juego va bien.)
- Solo se pierden los avisos.

**Si falta la línea `lua.module = "phoenix.lua"` en `sider.ini`:** el módulo no se carga. Ese error ya pasó (ver 6.4).

**Si falta `avisos.txt`:** el overlay dice «sin archivo de avisos». No pasa nada más.

### 1.3 · Versión recomendada hoy

| Versión | Recomendación |
|---|---|
| **0.2-prueba** (sha256 `116b40a6…`) | ✅ **Recomendada para el instalador.** |
| 0.1 (primera, con `pcall`) | ❌ Descartada: no carga en Sider 7.3.3 (ver 6.4). |
| 0.3 – 0.17e | Fuera de este informe (sección 0). |

### 1.4 · Qué lee y escribe la v0.2, y cada cuánto

| Qué | Detalle |
|---|---|
| **Lee** | `<sider_dir>content\phoenix\avisos.txt`. `sider_dir` es la carpeta del `sider.exe` que está corriendo (en ConmeGOL, `<juego>\SiderAddons\`). Si Sider no la da, usa `.\`. |
| **Cuánto lee** | Como máximo **4.096 bytes**. |
| **Cada cuánto** | Como mucho cada **2 segundos**, y **solo** mientras el overlay está abierto y Sider le pide el texto. Con el overlay cerrado, **no hace nada**. Usa `os.time()`, que va en segundos enteros, así que en la práctica son 2–3 s. |
| **Escribe** | **Nada** en disco. Solo líneas en `sider.log` (función `log`): una al arrancar y, si hay errores, una por cada error contado. |
| **Memoria del juego** | No la lee ni la escribe. |
| **Eventos de Sider** | Solo `overlay_on`. |
| **Teclas** | Ninguna propia. |
| **Variables y funciones de Sider que usa** | `ctx.sider_dir`, `ctx.register`, `log`, `io.open`, `os.time`, `os.date`, `string`, `table`. |
| **Defensa ante errores** | El Lua de Sider **no trae `pcall`**. El módulo usa una «bandera»: si encuentra que la vez anterior se cortó a mitad, lo cuenta como error. Con **5 errores seguidos** se apaga hasta reiniciar el juego y lo anota en el log. |

---

## 2. CARPETAS Y RUTAS

Todas las rutas son **relativas a la carpeta donde está `PES2021.exe`**. En el PC de FRALEX esa carpeta es `D:\Frank\Games_\Conmegol Patch\`.

### 2.1 · Árbol necesario (parche ConmeGOL 26)

```
<juego>\                                          ← carpeta de PES2021.exe
├─ SiderAddons\                                   ← la que USA el juego (la «raíz»)
│  ├─ sider.exe, sider.dll, sider.ini             ← del parche (sider.ini lo editamos: 2 líneas)
│  ├─ modules\
│  │  └─ phoenix.lua                              ← nuestro (v0.2)
│  ├─ content\
│  │  └─ phoenix\
│  │     └─ avisos.txt                            ← lo escribe Phoenix Link (buzón)
│  ├─ livecpk\
│  │  └─ Phoenix-DB\
│  │     └─ common\etc\pesdb\
│  │        ├─ Player.bin                         ← lo genera Phoenix Sync (stats)
│  │        └─ PlayerAssignment.bin               ← lo genera Phoenix Sync (plantillas); opcional
│  └─ olmosjr23\Database\common\etc\pesdb\…       ← base del PARCHE: NO TOCAR (Phoenix-DB la tapa)
└─ ConmeGol Extras\
   └─ ConmeGOL Patch 26\                          ← carpeta «de modo» que copia el switcher
      └─ SiderAddons\                             ← COPIA IDÉNTICA de lo nuestro (si no, el switcher lo pisa)
         ├─ sider.ini                             ← con las mismas 2 líneas
         ├─ modules\phoenix.lua
         ├─ content\phoenix\avisos.txt            ← solo el aviso de bienvenida (ver 2.3)
         └─ livecpk\Phoenix-DB\common\etc\pesdb\Player.bin (+ PlayerAssignment.bin)
```

### 2.2 · Cada carpeta: qué va dentro, de dónde sale y quién lo crea

| Carpeta (relativa a `<juego>`) | Archivo | De dónde sale | Quién lo crea |
|---|---|---|---|
| `SiderAddons\modules\` | `phoenix.lua` | Paquete del instalador (v0.2) | **Instalador** |
| `SiderAddons\content\phoenix\` | (la carpeta) | — | **Instalador**. Phoenix Link **no** debe crearla: si no existe, Link entiende «puente no instalado» (regla del prompt de Link). |
| `SiderAddons\content\phoenix\` | `avisos.txt` | Primero, un aviso de bienvenida; después, los avisos de la web | Bienvenida: **instalador**. Después: **Phoenix Link** (o `PhoenixAviso.bat` a mano). |
| `SiderAddons\content\phoenix\` | `avisos.tmp` | Archivo temporal de la escritura atómica | **Phoenix Link**. Dura milisegundos. |
| `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\` | `Player.bin` | Generado desde la base del parche (`olmosjr23\Database\…\Player.bin` en ConmeGOL) con los cambios de la web. Formato: cabecera WESYS + zlib. | **Phoenix Sync**. La primera vez, el instalador puede poner una copia **idéntica en datos** a la del parche (ver 2.4). |
| `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\` | `PlayerAssignment.bin` | Igual, desde el del parche | **Phoenix Sync**. Opcional: si no está, el juego usa el del parche. |
| `ConmeGol Extras\<modo>\SiderAddons\…` | lo mismo que arriba | Copia | **Instalador** (y Phoenix Sync, cada vez que cambie la base: «en las dos carpetas»). |

**Cómo encuentra Phoenix Sync las carpetas Phoenix-DB** (`core/RutasJuego.cpp`, función `buscarPlayerBin`):
- Recorre hasta **4 niveles** bajo `<juego>`, buscando carpetas llamadas `SiderAddons`, sin distinguir mayúsculas.
- Se salta `download` y `dt`.
- En cada una mira si existe `livecpk\Phoenix-DB\common\etc\pesdb\Player.bin`.
- La de `<juego>\SiderAddons` se llama modo «principal».
- Las demás se llaman por su ruta relativa, por ejemplo `ConmeGol Extras/ConmeGOL Patch 26`.

**El nombre del parche** lo lee de `<juego>\version_actual.txt`, la primera línea (quitando el BOM). Si ese archivo no existe, usa el nombre de la carpeta.

### 2.3 · Archivos que Phoenix Link escribe o lee, con su formato exacto

#### `avisos.txt` (Link lo escribe; `phoenix.lua` lo lee)

| Propiedad | Valor |
|---|---|
| Ruta | `<juego>\SiderAddons\content\phoenix\avisos.txt`. **La raíz, no la carpeta del modo**: Sider corre desde la raíz. ✅ Así se probó: el aviso se escribió en la raíz y salió en ~1 s. |
| Codificación | **UTF-8 sin BOM**. Si trae BOM, el módulo lo quita. Si no es UTF-8 válido, **no se muestra** y sale «(el aviso no está en UTF-8: no se muestra)». |
| Saltos de línea | `\n`. Si vienen `\r`, el módulo los quita. |
| Límites del módulo | Lee **4.096 bytes** como máximo. Muestra **14 líneas** (si hay más, pone «(...)»). Cada línea tiene un máximo de **110 bytes**: corta sin partir una letra con tilde y añade «...». |
| Límites que debe respetar Link (prompt LINK, buzón) | **14 líneas y 1.500 bytes** como máximo. Avisos del más nuevo al más viejo, cada uno como `[hh:mm] texto` en hora de Lima, con una línea vacía entre avisos. Sin avisos: `Sin avisos nuevos de Phoenix Evolution.` |
| Caracteres | Se borran los de control (salvo el salto de línea). Tildes, ñ, ⚡ y → ✅ se ven bien con la fuente Consolas. |
| Escritura (obligatoria) | **Atómica:** escribir `avisos.tmp` en la misma carpeta. Luego `ReplaceFileW(avisos.txt, avisos.tmp)`, o `MoveFileExW(tmp, txt, MOVEFILE_REPLACE_EXISTING \| MOVEFILE_WRITE_THROUGH)` si `avisos.txt` aún no existe. Si falla porque el juego lo está leyendo, **reintentar hasta 10 veces cada 100 ms**. |
| No reescribir | Si el contenido es igual al último escrito. |
| Archivo vacío | El módulo muestra «(aviso vacío)». |

#### Otros archivos

| Archivo | Quién lo escribe | Quién lo lee |
|---|---|---|
| `Player.bin` / `PlayerAssignment.bin` de Phoenix-DB | Phoenix Sync | El juego, a través de Sider, **solo** al arrancar, en «Datos del sistema: Cargando» y al entrar y salir de Editar. Se aplica con **Editar → Cargar** ✅. |
| `sider.log` | Sider | Phoenix Link puede leerlo para comprobar la instalación (sección 6.3). |

**Por qué hay que escribir Phoenix-DB en las DOS carpetas:** el switcher copia la carpeta del modo encima de la raíz **sobrescribiendo** (sección 5). Si solo se cambia la raíz, el siguiente cambio de variante devuelve la versión vieja. [DEDUCIDO] de las opciones de robocopy; por eso hasta hoy siempre se ha escrito en las dos.

### 2.4 · Estado real HOY en el PC de FRALEX (05:15) ⚠️

| Archivo | Huella | Qué es |
|---|---|---|
| `SiderAddons\livecpk\Phoenix-DB\…\Player.bin` (raíz y modo) | sha256 `de702dff…` | **v90**: mismos datos que el del parche, solo recomprimido. Correcto. |
| `SiderAddons\livecpk\Phoenix-DB\…\PlayerAssignment.bin` (raíz y modo) | sha256 `1d27f010…` | ⚠️ **De una prueba** (Mbappé y Vinícius en el Barça). **No es el original.** El original es `d1cf73c6…`, igual al de `olmosjr23`; hay copia en `_PhoenixMercado_prueba\db\PlayerAssignment_original.bin`. **El instalador no debe copiar este archivo de prueba.** |
| `SiderAddons\olmosjr23\Database\…\Player.bin` | sha256 `a1e59895…` | Base original del parche. No se toca. |
| `sider.ini` (raíz y modo) | md5 `93ae9604…` | Iguales entre sí, con las 2 líneas de Phoenix. |
| `modules\phoenix.lua` (raíz y modo) | sha256 `114b6f2e…` | v0.17e (familia de memoria). |

---

## 3. sider.ini

### 3.1 · Las líneas exactas que hay que añadir

**Formato del archivo:** ASCII, finales de línea **CRLF** (`\r\n`), **sin BOM**, una sola sección `[sider]`. Conservar exactamente ese formato al escribir.

#### Línea 1: raíz de la base Phoenix

```ini
cpk.root = ".\livecpk\Phoenix-DB"
```

- **Dónde:** **justo antes** de la línea de la base del parche. En ConmeGOL 26 es `cpk.root = ".\olmosjr23\Database"`.
- Hoy queda así (líneas 89–91):
  ```ini
  cpk.root = ".\livecpk\boot-root"
  cpk.root = ".\livecpk\Phoenix-DB"
  cpk.root = ".\olmosjr23\Database"
  ```
- **Por qué antes:** en Sider, la raíz que va **antes** gana si dos raíces tienen el mismo archivo. ✅ Probado: con esta línea antes de `olmosjr23`, el juego mostró la Velocidad de nuestro `Player.bin` (9 oct, 03:41).
- **Regla general (multiparche):** antes de la **primera** línea `cpk.root` cuya carpeta contenga `common\etc\pesdb\Player.bin`. Si ninguna la contiene (parche «de CPK», como Sudamerican), antes de la primera `cpk.root`. [NO PROBADO] en un parche de CPK.

#### Línea 2: el módulo

```ini
lua.module = "phoenix.lua"
```

- **Dónde:** como **última** línea `lua.module` de la lista.
- En ConmeGOL 26 va justo después de `lua.module = "StartingYearChanger.lua"` (línea 200) y antes del bloque `overlay.*`.
- **Requisito:** `lua.enabled = 1`. En ConmeGOL ya está en la línea 126.

**Nada más.** No hace falta tocar `lua.path`, `overlay.*` ni otras claves. `overlay.enabled = 1` y las teclas (`overlay.vkey.toggle = 0x20`, `next-module = 0x31`, `prev-module = 0xc0`) ya vienen en el parche.

### 3.2 · Cómo saber si ya están puestas (para no duplicarlas)

Leer línea a línea, **sin distinguir mayúsculas**, ignorando los espacios de los bordes.

- **Ya está la raíz:** hay una línea que **no empieza por `;`** y cumple:
  `^\s*cpk\.root\s*=\s*"\.\\livecpk\\Phoenix-DB\\?"\s*$`
- **Ya está el módulo:** hay una línea que **no empieza por `;`** y cumple:
  `^\s*lua\.module\s*=\s*"phoenix\.lua"\s*$`
- **Si la línea existe pero con `;` delante** (desactivada): quitar el `;`. **No** añadir otra.
- **Comprobación cruzada:** la raíz Phoenix-DB tiene que ir **antes** de la raíz de la base del parche. Si está después, moverla.

### 3.3 · Riesgos conocidos y cómo se deshace

| Riesgo | Qué pasa | Cómo evitarlo o arreglarlo |
|---|---|---|
| Editar solo la raíz `SiderAddons\sider.ini` | El switcher la **pisa** con la del modo y el módulo deja de cargar. ✅ Pasó el 8 oct. | Editar **también** `ConmeGol Extras\<modo>\SiderAddons\sider.ini`. |
| Una actualización del parche trae otro `sider.ini` | Se pierden las 2 líneas. No se rompe nada, pero Phoenix deja de funcionar sin avisar. | Phoenix Link debe **comprobar en cada arranque** (sección 3.2) y avisar o reponer. |
| Romper el formato (BOM, LF, otra codificación) | [NO PROBADO]. Riesgo de que Sider no lea bien la configuración. | Conservar ASCII + CRLF; escribir con `.tmp` y reemplazar. |
| Línea duplicada | 🔎 El parche ya trae «Menu Light» dos veces y funciona. No se sabe qué hace Sider con un módulo duplicado. [NO PROBADO] | No duplicar (sección 3.2). |
| Módulo nombrado pero archivo ausente | 🔎 Sider anota `PROBLEM: Unable to open file` y sigue. | Inofensivo, pero el instalador debe poner el archivo. |
| Phoenix-DB con datos viejos tras actualizar el parche | La raíz Phoenix-DB **tapa** la base nueva del parche: se servirían jugadores viejos. | Regenerar `Player.bin` desde la base nueva (Phoenix Sync) o quitar la línea hasta regenerarlo. |

**Copias de respaldo:**
- Antes de editar, copiar `sider.ini` a `sider.ini.respaldo-phoenix-AAAAMMDD`, al lado, en **cada** carpeta. Ya existen `sider.ini.respaldo-phoenix-20261009` y `sider.ini.respaldo-phoenixdb-20261009`.
- **No** sobrescribir un respaldo que ya exista: añadir la hora o un número.

**Cómo deshacer:**
- **(a)** Quitar las 2 líneas, o ponerles `;` delante.
- **(b)** O restaurar el respaldo.
- En las **dos** carpetas.
- Comprobado: la diferencia entre el respaldo más viejo y el actual son **exactamente esas 2 líneas** 🔎.

---

## 4. DIFERENCIAS ENTRE PARCHES

### 4.1 · Tabla por parche

| | **ConmeGOL 26** (variante «ConmeGOL Patch 26») | **ConmeGOL 26 · Perú, Chile, Uruguay B** | **ConmeGOL 26 · Clásicos (No LM)** | **Sudamerican Project 2026** | **Original sin parche** |
|---|---|---|---|---|---|
| Carpeta en el PC | `D:\Frank\Games_\Conmegol Patch\` | la misma (variante) | la misma (variante) | `D:\SP2026\` | [NO PROBADO]: no lo he visto |
| Dónde está Sider | `<juego>\SiderAddons\` (copiada desde `ConmeGol Extras\<variante>\SiderAddons\`) | igual | igual | **Dos** Sider: `<juego>\Sudamerican Project Sider\` y `<juego>\Sudamerican Project Sider WC26\` | No trae Sider [NO PROBADO] |
| Versión de Sider | **7.3.3** 🔎 (`sider.log`) | 7.3.3 [DEDUCIDO]: el mismo `sider.dll` de la raíz | 7.3.3 [DEDUCIDO] | **7.3.4** 🔎 (los dos) | — |
| Cómo arranca | `PES2021 Start.exe` (switcher, sección 5) | igual | igual | `SP LAUNCHER <letra>.bat`: entra en la carpeta de Sider, abre `sider.exe`, espera 2 s y abre `PES2021.exe` con `/affinity 7F` | — |
| ¿livecpk o content? | **livecpk** para casi todo (~110 `cpk.root`) y `content\` para datos de módulos | livecpk (menos raíces: no trae PRDX) | livecpk | livecpk (107 y 98 `cpk.root`) + CPK clásicos | — |
| Base de datos que usa el juego (`Player.bin`) | **`SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin`** (raíz de Sider; gana a la del CPK). También hay una copia en `download\CGP_database.cpk`. | La misma raíz `.\olmosjr23\Database` 🔎 en su `sider.ini`. ⚠️ Su carpeta de variante **no trae** `olmosjr23`: usa la que dejó la variante 26 en la raíz. [DEDUCIDO] | Igual que Perú (raíz `olmosjr23`, sin carpeta propia) [DEDUCIDO] | Dentro de **`download\SP_Subs.cpk`** (CPK clásico). Sus `sider.ini` no tienen raíz de base. 🔎 | Los CPK de Konami (`download\dt80_*`) [NO PROBADO] |
| `download\CGP_database.cpk` | md5 `d2d62b4d…` | md5 `f525fd4c…` (**distinto**) | md5 `cfbad462…` (**distinto**) | — | — |
| Módulos Lua en `sider.ini` | 66 líneas (con `phoenix.lua`) | 54 | 57 | 57 y 72 | — |
| Carpeta de guardados | `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save` | la misma | la misma | `…\292733975847239680\save` | [NO PROBADO] |
| **Phoenix instalado hoy** | ✅ Sí (2 líneas + módulo + Phoenix-DB) | ❌ No | ❌ No | ❌ No | — |
| Qué cambia respecto a ConmeGOL 26 | — | Otras ligas, otro CPK de base y otro `sider.ini` | Sin Liga Máster, otro CPK y otro `sider.ini` | Otro estilo («de CPK»), **dos** `sider.ini`, otra carpeta de guardados, `netblock.lua` (bloquea la red: choca con PESBUL), módulos cargados dos veces | Sin Sider: no hay dónde poner el módulo ni Phoenix-DB sin instalar Sider antes |

**Otros parches** (Football Life, Evoweb, Gogosz, SmokePatch): solo los he estudiado por internet (`investigacion/parches-pes2021.md`). **No están en este PC**: todo [NO PROBADO]. Football Life usa un exe propio: es el de más riesgo.

### 4.2 · Cómo detectar cada tipo automáticamente

Se mira dentro de `<juego>`, la carpeta de `PES2021.exe`. Phoenix Link la saca del proceso del juego (`QueryFullProcessImageNameW`) o de la configuración.

| Si existe… | Es… | Seguridad |
|---|---|---|
| `PES2021 Start.exe` **y** `ConmeGol Extras\` **y** `version_actual.txt` | **ConmeGOL**. La primera línea de `version_actual.txt` dice la variante: `ConmeGOL Patch 26`, `ConmeGOL Patch 26 (Peru, Chile, Uruguay B)` o `Clasicos (No LM)`. | 🔎 (hoy dice «ConmeGOL Patch 26») |
| `conmegol_info.txt` | Confirma ConmeGOL | 🔎 |
| `Sudamerican Project Sider\sider.ini` y/o `Sudamerican Project Sider WC26\` **y** `download\SP_Subs.cpk` | **Sudamerican 2026** | 🔎 |
| `LEER.txt` + `DpFileList Generator\` | Pista extra de Sudamerican | 🔎 |
| Ninguna carpeta con `sider.exe` + `sider.ini` | Juego **sin Sider** (original u otro parche de CPK) | [NO PROBADO] |
| Una carpeta `SiderAddons` o con `sider.ini` que no encaja en lo anterior | Parche desconocido: hacer solo **diagnóstico** y no instalar nada | regla de prudencia |

**Cuál es la base que usa el juego** (guía 12, §2.2):
- Parche de Sider: la **primera** `cpk.root` cuya carpeta tenga `common\etc\pesdb\Player.bin`.
- Parche de CPK: el **último** CPK de `download\DpFileList.bin` que tenga `pesdb`.
- Si existen las dos cosas, **gana Sider**.

**Cuántos Sider hay:** buscar todos los `sider.ini` hasta 4 niveles bajo `<juego>` (sin entrar en `download`).

### 4.3 · Qué parches NO son compatibles (hoy) y por qué

| Parche | Por qué no |
|---|---|
| **Original sin Sider** | No hay dónde cargar `phoenix.lua` ni la raíz Phoenix-DB. Habría que instalar Sider aparte: [NO PROBADO], no lo he hecho. |
| **Sudamerican 2026** | **No incompatible, pero sin probar.** Haría falta: las 2 líneas en **sus dos** `sider.ini`; el módulo en sus dos `modules`; y un `Player.bin` generado desde **su** base (`SP_Subs.cpk`, con otros IDs). Además: `netblock.lua` bloquea la red del juego. Esto no afecta a Phoenix, que no usa red dentro del juego, pero sí al online. [NO PROBADO] |
| **ConmeGOL Perú/Chile/Uruguay B y Clásicos** | Compatibles en teoría, pero hoy **no tienen** Phoenix. Y su `CGP_database.cpk` es distinto: un `Player.bin` de Phoenix-DB hecho desde la variante 26 **taparía** su base con datos de otra variante. Cada variante necesita su propio `Player.bin`. [DEDUCIDO] |
| **Football Life** (exe propio) | Exe y lanzador propios. Sin estudiar. **No instalar.** [NO PROBADO] |

---

## 5. CAMBIADOR DE PARCHES (switcher) de ConmeGOL

**Qué es:** `PES2021 Start.exe`, en la carpeta del juego. Es del parche, no de Phoenix: un programa de Python empaquetado con PyInstaller, con interfaz tkinter. 🔎 (`VINCULO-TIEMPO-REAL.md`)

### 5.1 · Qué hace al elegir una variante (en este orden)

1. Ejecuta `robocopy "ConmeGol Extras\<variante>" "<juego>" /E /IS /IT`.
   - `/E`: copia subcarpetas.
   - `/IS /IT`: **copia y sobrescribe incluso los archivos iguales**.
   - Copia `CPY.ini`, `SiderAddons\` (con `sider.ini`, `modules`, `content`, `livecpk` y, en la variante 26, `olmosjr23`), `conmegol_info.txt` y `download\`.
2. Escribe el nombre de la variante en `<juego>\version_actual.txt`.
3. Abre `<juego>\SiderAddons\sider.exe`, espera 2 s y abre `PES2021.exe`.

No usa internet.

### 5.2 · Qué pasa con SiderAddons al cambiar de variante

- **No borra nada** de la raíz. Robocopy sin `/PURGE` solo añade y sobrescribe. [DEDUCIDO] de las opciones.
  - Por eso los archivos que no están en la carpeta de la variante **se quedan** en la raíz: por ejemplo `modules\phoenix.lua`, `livecpk\Phoenix-DB\` y `olmosjr23\`.
- **Sí reemplaza** `sider.ini` por el de la variante. Si esa variante no tiene las 2 líneas, Phoenix **deja de cargarse**, aunque los archivos sigan ahí. ✅ Pasó el 8 oct con la variante 26, antes de instalar también en la carpeta del modo.
- **Sí reemplaza** `content\phoenix\avisos.txt` de la raíz por el de la carpeta de la variante (si lo tiene): vuelve el aviso de bienvenida hasta que Link escriba otro. [DEDUCIDO]
- **Sí reemplaza** `livecpk\Phoenix-DB\…\Player.bin` de la raíz por el de la carpeta de la variante (si lo tiene). Si Phoenix Sync solo actualizó la raíz, se pierde el cambio. [DEDUCIDO] → **Escribir siempre en las dos.**

### 5.3 · Qué hay que reinstalar o comprobar después de cada cambio

| Cambio a… | Qué hacer |
|---|---|
| **ConmeGOL Patch 26** (ya instalada en su carpeta) | Nada, si el instalador puso todo también en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`. Comprobar las 2 líneas en `<juego>\SiderAddons\sider.ini`. |
| **Perú/Chile/Uruguay B** o **Clásicos** | Hoy Phoenix no funciona en ellas. Para que funcione, el instalador tendría que instalar también en **su** carpeta (`ConmeGol Extras\<variante>\SiderAddons\`): 2 líneas, el módulo, y un Phoenix-DB **generado desde la base de esa variante**. [NO PROBADO] |
| Cualquier variante, siempre | Phoenix Link, al arrancar y al detectar que `version_actual.txt` cambió, debe comprobar la sección 3.2 en `<juego>\SiderAddons\sider.ini` y avisar si falta algo. |

**Para que un módulo nuevo cargue** hay que **reiniciar el juego**. Lo más sencillo es con el switcher. `Shift+R` (`vkey.reload-1 = 0x10`, `vkey.reload-2 = 0x52`) solo recarga módulos **que ya estaban activos**: no carga uno recién añadido. ✅ Visto el 9 oct.

---

## 6. PASO A PASO MANUAL ACTUAL (ConmeGOL 26)

Así se dejó listo el PC de FRALEX el 8 y 9 de octubre, solo con la parte que cubre este informe.

### 6.1 · Pasos

1. **Cerrar el juego** y Sider.
2. **Respaldos** (en cada carpeta, con nombre nuevo):
   - `<juego>\SiderAddons\sider.ini` → `sider.ini.respaldo-phoenix-AAAAMMDD`;
   - `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\sider.ini` → igual.
3. **Módulo:** copiar `phoenix.lua` (v0.2) a:
   - `<juego>\SiderAddons\modules\phoenix.lua`;
   - `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\phoenix.lua`.
4. **Buzón:** crear la carpeta `content\phoenix\` en las **dos** `SiderAddons` y poner un `avisos.txt` de bienvenida (UTF-8 sin BOM). El que se usó:
   ```
   Hola FRALEX: el puente Phoenix ya está instalado.
   Si lees esto dentro del juego, la prueba 1 funcionó.
   ```
5. **Base Phoenix-DB:** crear `livecpk\Phoenix-DB\common\etc\pesdb\` en las **dos** `SiderAddons` y poner un `Player.bin`.
   - El de partida tiene **los mismos datos** que `olmosjr23\Database\common\etc\pesdb\Player.bin` (recomprimido; sha256 `de702dff…`).
   - Copia en `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\Player_v90.bin`.
   - `PlayerAssignment.bin` es opcional. Si se pone, que sea el **original** (`PlayerAssignment_original.bin`, sha256 `d1cf73c6…`).
6. **`sider.ini`** en las **dos** carpetas:
   - añadir `cpk.root = ".\livecpk\Phoenix-DB"` justo antes de `cpk.root = ".\olmosjr23\Database"`;
   - añadir `lua.module = "phoenix.lua"` como última `lua.module`.
   - Comprobar que la diferencia con el respaldo es solo de **2 líneas**.
7. **Arrancar** con `PES2021 Start.exe`, eligiendo «ConmeGOL Patch 26».

### 6.2 · Cómo se comprueba que quedó bien

**En el juego:**
1. **Espacio** → **1** varias veces → debe salir:
   `PHOENIX EVOLUTION  ·  puente en vivo v0.2-prueba  ·  conectado`
   seguido del texto del aviso. ✅
2. **Tiempo real:** con el juego abierto, reemplazar `avisos.txt` de la raíz (de forma atómica). El texto nuevo sale en **1–3 s** con el overlay abierto, y la cabecera dice «último aviso hh:mm:ss». ✅
3. **Phoenix-DB:** si `Player.bin` lleva un cambio de prueba, debe verse en la pantalla de habilidades del jugador **tras reiniciar**, o **tras Editar → Cargar** sin reiniciar. ✅ (Lamine 99 → 95 → 90.) Con el `Player.bin` de partida (datos originales) no se ve nada distinto. Es lo correcto.

**En `<juego>\SiderAddons\sider.log`** (las rutas cambian según el PC):
```
Using cpk.root: <juego>\SiderAddons\.\livecpk\Phoenix-DB\
Loading module: phoenix.lua ...
[phoenix.lua] [phoenix] v0.2-prueba listo (solo lectura). Archivo: <juego>\SiderAddons\content\phoenix\avisos.txt
```
- 🔎 Las dos primeras líneas tienen ese formato exacto en el log de hoy.
- La tercera sale del código de la v0.2 con el mismo formato que el log real de otra versión: `[phoenix.lua] [phoenix] v0.17d listo …`.

**Señales de fallo en el log:**
- `Module (phoenix.lua) is NOT activated`: el módulo dio error al iniciar.
- `PROBLEM: Unable to open file`: falta el archivo.
- `[phoenix] demasiados errores seguidos`: se apagó solo.

### 6.3 · Comprobaciones que puede automatizar el instalador

- [ ] Las 2 líneas en **cada** `sider.ini` del parche, y en el orden correcto (sección 3.2).
- [ ] `modules\phoenix.lua` con la huella de la v0.2 en cada carpeta.
- [ ] Existen `content\phoenix\` y `livecpk\Phoenix-DB\common\etc\pesdb\Player.bin` en cada carpeta.
- [ ] Tras el primer arranque: `sider.log` contiene `Loading module: phoenix.lua` y `[phoenix] v0.2-prueba listo`, y **no** contiene `phoenix.lua) is NOT activated`.

### 6.4 · Errores que ya vimos y su solución

| # | Error | Cómo se vio | Solución |
|---|---|---|---|
| 1 | El módulo no cargaba: `attempt to call global 'pcall' (a nil value)` → `Module (phoenix.lua) is NOT activated` | `sider.log`, 9 oct 00:34 | El Lua de Sider 7.3.3 **no trae `pcall`**. v0.2 sin `pcall` (bandera de errores). ✅ 00:39 |
| 2 | La línea del módulo desaparecía y nunca cargaba | 8 oct | Se había editado solo la raíz y el switcher la pisó. Instalar **también en la carpeta del modo**. |
| 3 | `Shift+R` no cargaba el módulo nuevo | 9 oct | `Shift+R` solo recarga módulos ya activos. **Reiniciar el juego.** |
| 4 | Aviso escrito en la carpeta equivocada | — | Escribir en `<juego>\SiderAddons\content\phoenix\`, la raíz. `sider_dir` es la raíz. |
| 5 | El juego se cerró al leer memoria directamente | v0.3, 9 oct 02:43 | Se volvió a la v0.2. Es una razón más para que el instalador use solo la v0.2. |
| 6 | Entrar y salir de Editar no aplicaba el `Player.bin` nuevo | 9 oct 03:51 | El método correcto es **Editar → Cargar**. ✅ |
| 7 | «Datos Actual. en vivo → Activar» decía que los servicios en línea terminaron | 9 oct | Es la opción nativa, que depende del servidor de Konami (cerrado). No forma parte de este instalador. |
| 8 | Phoenix-DB **tapa** la base del parche | — | Si el parche se actualiza, hay que regenerar `Player.bin` desde la base nueva, o se servirán datos viejos. |
| 9 | `PlayerAssignment.bin` de prueba olvidado en Phoenix-DB | Hoy (sección 2.4) | Antes de empaquetar, usar el **original**. El instalador no debe copiar archivos de `Phoenix-DB` del PC de FRALEX sin comprobar su huella. |

---

## 7. SEGURIDAD Y REVERSA

### 7.1 · Qué archivos se pisan o se crean

| Archivo | ¿Existía? | Acción del instalador | Respaldo |
|---|---|---|---|
| `SiderAddons\sider.ini` (cada carpeta) | Sí | Se **edita**: +2 líneas | `sider.ini.respaldo-phoenix-AAAAMMDD[-hhmm]` al lado |
| `SiderAddons\modules\phoenix.lua` | Puede existir (hoy: v0.17e) | Se **crea o reemplaza** | Si existe, renombrarlo a `phoenix.lua.respaldo-AAAAMMDD-hhmm` (no borrarlo) |
| `SiderAddons\content\phoenix\avisos.txt` | Puede existir | Se crea solo si no existe | — |
| `SiderAddons\livecpk\Phoenix-DB\…\Player.bin` (y `PlayerAssignment.bin`) | Puede existir | Se crea; si existe, lo gestiona Phoenix Sync, que ya hace sus respaldos en `%APPDATA%\Phoenix Mercado\respaldos\…` | Copia antes de reemplazar |

### 7.2 · Cómo se restaura

1. Cerrar el juego.
2. En **cada** carpeta (raíz y modo):
   - restaurar `sider.ini` desde su respaldo;
   - o quitar las 2 líneas.
3. Opcional: quitar `modules\phoenix.lua`, `content\phoenix\` y `livecpk\Phoenix-DB\`.
   - Sin la línea `cpk.root`, Phoenix-DB **no se usa** aunque la carpeta exista.
   - Sin la línea `lua.module`, el módulo no carga.
4. Arrancar con el switcher.

### 7.3 · Lo que JAMÁS se debe tocar

- `PES2021.exe` y cualquier `.dll` del juego o de Sider (`sider.dll`, `sider.exe`).
- **El option file** (`…\save\EDIT00000000`) y los guardados de Liga Máster (`ML0000000x`). Los toca **solo** Phoenix Sync, con el juego cerrado y con respaldo. Nunca el instalador.
- `download\*.cpk`, `download\DpFileList.bin`, `Data\*.cpk`.
- La base del parche: `SiderAddons\olmosjr23\Database\…`. Phoenix-DB la **tapa**; no se cambia.
- Los otros módulos del parche y sus líneas en `sider.ini`, aunque tengan fallos conocidos (`AUDITORIA-SIDER.md`). Limpiarlos es otra tarea, con permiso de FRALEX.
- `PES2021 Start.exe`, `version_actual.txt`, `CPY.ini`, `conmegol_info.txt`.
- El archivo `hosts` de Windows (PESBUL).
- **Ningún módulo que lea o escriba la memoria del juego** (sección 0).

---

## 8. ARCHIVOS REALES

### 8.1 · Dónde están (PC de FRALEX)

| Qué | Ruta completa | Huella | ¿Empaquetar? |
|---|---|---|---|
| **`phoenix.lua` v0.2 (probada)** | `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\phoenix.lua.v02` | sha256 `116b40a67b6e44a34ae96822ff597e4f456b50a435635ab763d5e00596ff1e0e`, 5.806 B, UTF-8, saltos LF | ✅ Sí. Renombrar a `phoenix.lua` al instalar. |
| La misma, en el repo | commit `4683988`, `PhoenixMercado/sider/phoenix.lua` | igual | (fuente) |
| `PhoenixAviso.ps1` | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\PhoenixAviso.ps1` y repo `PhoenixSync/sider/PhoenixAviso.ps1` | — | Opcional: herramienta manual para enviar avisos |
| `PhoenixAviso.bat` | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\PhoenixAviso.bat` y repo `PhoenixSync/sider/PhoenixAviso.bat` | — | Opcional |
| `avisos.txt` de bienvenida | `D:\Frank\Games_\Conmegol Patch\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\content\phoenix\avisos.txt` (105 B) | — | Opcional. Mejor generarlo. |
| `Player.bin` de partida (= datos del parche) | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\Player_v90.bin` | sha256 `de702dff…` | ⚠️ **No conviene empaquetarlo:** vale **solo** para ConmeGOL 26 en su versión actual. Lo correcto es que Phoenix Sync lo genere en cada PC desde la base de **su** parche. |
| `PlayerAssignment.bin` original | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\PlayerAssignment_original.bin` | sha256 `d1cf73c6…` | Igual que el anterior: mejor generarlo. |
| Respaldos de `sider.ini` | `D:\Frank\Games_\Conmegol Patch\SiderAddons\sider.ini.respaldo-phoenix-20261009` (sin ninguna línea Phoenix) y `…respaldo-phoenixdb-20261009` (con el módulo, sin Phoenix-DB). También en `…\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`. | md5 `80ad499f…` / `e09d13da…` | No (son del PC de FRALEX) |

### 8.2 · Contenido de `phoenix.lua` v0.2 (el que se empaqueta)

Es la versión exacta probada en el juego (sha256 `116b40a6…`). Hay que guardarla como **UTF-8**. Sider la aceptó con saltos LF.

```lua
-- =============================================================================
--  Phoenix Evolution · Puente en vivo para Sider (PES 2021)
--  Prueba 1 — SOLO LECTURA: lee un archivo de texto y lo muestra en el overlay
--  de Sider. NO lee ni escribe la memoria del juego, NO registra hooks del
--  partido, NO cambia archivos del juego ni teclas. Si algo falla, se apaga
--  solo y lo anota una vez en sider.log.
--
--  Archivo que lee:  <SiderAddons>\content\phoenix\avisos.txt  (UTF-8, ≤ 4 KB)
--  Quién lo escribe: Phoenix Link / Phoenix Mercado (o PhoenixAviso.bat)
--  Cómo se ve:       Espacio abre el overlay de Sider; con 1 / º se pasa de
--                    módulo hasta llegar a «Phoenix Evolution».
--  Desinstalar:      quitar (o poner ; delante de) la línea
--                    lua.module = "phoenix.lua"  en sider.ini
-- =============================================================================

local m = { version = "0.2-prueba" }

local CADA_SEG    = 2      -- cada cuántos segundos se vuelve a mirar el archivo (solo con el overlay abierto)
local MAX_BYTES   = 4096   -- nunca se lee más que esto
local MAX_LINEAS  = 14
local MAX_ANCHO   = 110    -- bytes por línea (se corta sin partir letras con tilde)
local MAX_FALLOS  = 5      -- tras 5 errores internos seguidos, el módulo se apaga

local ruta = nil
local ultimoChequeo = -1000
local contenido = "Esperando el primer aviso..."
local estado = "sin datos"
local actualizado = ""
local firmaAnterior = nil
local fallos = 0
local apagado = false

-- Corta una cadena UTF-8 a como mucho n bytes sin dejar una letra a medias.
local function cortarUtf8(s, n)
    if #s <= n then return s end
    local fin = n
    -- retroceder mientras el byte siguiente sea de continuación (10xxxxxx)
    while fin > 0 do
        local b = s:byte(fin + 1)
        if not b or b < 0x80 or b >= 0xC0 then break end
        fin = fin - 1
    end
    return s:sub(1, fin) .. "..."
end

-- ¿Es UTF-8 válido? Si no, no se muestra (evita basura en pantalla si el archivo se escribió mal).
local function esUtf8(s)
    local i, n = 1, #s
    while i <= n do
        local b = s:byte(i)
        local extra
        if b < 0x80 then extra = 0
        elseif b >= 0xC2 and b <= 0xDF then extra = 1
        elseif b >= 0xE0 and b <= 0xEF then extra = 2
        elseif b >= 0xF0 and b <= 0xF4 then extra = 3
        else return false end
        for k = 1, extra do
            local c = s:byte(i + k)
            if not c or c < 0x80 or c > 0xBF then return false end
        end
        i = i + extra + 1
    end
    return true
end

-- Deja el texto seguro para mostrar: sin BOM, sin \r, sin caracteres de control, líneas y ancho limitados.
local function limpiar(s)
    if not esUtf8(s) then return "(el aviso no está en UTF-8: no se muestra)" end
    s = s:gsub("^\239\187\191", "")
    s = s:gsub("\r", "")
    s = s:gsub("[%z\1-\8\11-\31\127]", "")
    local salida, n = {}, 0
    for linea in (s .. "\n"):gmatch("(.-)\n") do
        n = n + 1
        if n > MAX_LINEAS then salida[#salida + 1] = "(...)" break end
        salida[#salida + 1] = cortarUtf8(linea, MAX_ANCHO)
    end
    while #salida > 0 and salida[#salida] == "" do salida[#salida] = nil end
    if #salida == 0 then return "(aviso vacío)" end
    return table.concat(salida, "\n")
end

local function leerArchivo()
    local f = io.open(ruta, "rb")
    if not f then return nil end
    local s = f:read(MAX_BYTES + 1) or ""
    f:close()
    if #s > MAX_BYTES then
        -- archivo demasiado grande: se recorta sin partir una letra
        local fin = MAX_BYTES
        while fin > 0 do
            local b = s:byte(fin + 1)
            if not b or b < 0x80 or b >= 0xC0 then break end
            fin = fin - 1
        end
        s = s:sub(1, fin)
    end
    return s
end

local function refrescar()
    local ahora = os.time()
    if ahora - ultimoChequeo < CADA_SEG then return end
    ultimoChequeo = ahora
    local s = leerArchivo()
    if s == nil then
        estado = "sin archivo de avisos"
        return
    end
    estado = "conectado"
    if s ~= firmaAnterior then
        firmaAnterior = s
        contenido = limpiar(s)
        actualizado = os.date("%H:%M:%S")
    end
end

local function textoOverlay()
    refrescar()
    local cab = string.format("PHOENIX EVOLUTION  ·  puente en vivo v%s  ·  %s", m.version, estado)
    if actualizado ~= "" then cab = cab .. "  ·  último aviso " .. actualizado end
    return cab .. "\n\n" .. contenido
end

-- Ojo: el Lua de Sider NO trae pcall (lo confirma el volcado de env.lua en sider.log).
-- Sider ya atrapa los errores de cada evento y sigue funcionando; para contar fallos usamos una
-- «bandera»: se levanta al entrar y se baja al salir bien. Si al entrar la encontramos levantada,
-- es que la vez anterior hubo un error (Sider la cortó a la mitad).
local enCurso = false

function m.overlay_on(ctx)
    if apagado then return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)" end
    if enCurso then
        fallos = fallos + 1
        log("[phoenix] la vez anterior hubo un error (" .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[phoenix] demasiados errores seguidos: el módulo se detiene hasta reiniciar el juego")
            return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)"
        end
    end
    enCurso = true
    local res = textoOverlay()
    enCurso = false
    fallos = 0
    return res
end

function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    ruta = base .. "content\\phoenix\\avisos.txt"
    ctx.register("overlay_on", m.overlay_on)
    log("[phoenix] v" .. m.version .. " listo (solo lectura). Archivo: " .. ruta)
end

return m
```

### 8.3 · Contenido de `PhoenixAviso.ps1` (herramienta manual; Phoenix Link hace lo mismo por dentro)

```powershell
# Phoenix Evolution · escribe un aviso para el módulo phoenix.lua de Sider (prueba 1).
# Escritura "atómica": primero a avisos.tmp y luego se reemplaza avisos.txt de una vez,
# así el juego nunca lee un archivo a medio escribir.
param(
    [string]$Texto = "",
    [string]$Carpeta = ""
)
$ErrorActionPreference = 'Stop'
if ($Carpeta -eq "") { $Carpeta = Join-Path $PSScriptRoot '..\SiderAddons\content\phoenix' }
$Carpeta = [IO.Path]::GetFullPath($Carpeta)
if (-not (Test-Path (Join-Path $Carpeta '..\..\sider.ini'))) {
    Write-Host "No encuentro sider.ini dos carpetas arriba de: $Carpeta" -ForegroundColor Red
    Write-Host "Pasa la carpeta correcta con -Carpeta '...\SiderAddons\content\phoenix'"
    exit 1
}
New-Item -ItemType Directory -Force -Path $Carpeta | Out-Null
if ($Texto -eq "") { $Texto = Read-Host 'Escribe el aviso (usa | para pasar a otra línea)' }
$Texto = ($Texto -replace '\s*\|\s*', "`n").Trim()
if ($Texto.Length -gt 1500) { $Texto = $Texto.Substring(0, 1500) }
$tmp = Join-Path $Carpeta 'avisos.tmp'
$dst = Join-Path $Carpeta 'avisos.txt'
[IO.File]::WriteAllText($tmp, $Texto + "`n", (New-Object Text.UTF8Encoding $false))
$hecho = $false
for ($i = 0; $i -lt 20 -and -not $hecho; $i++) {
    try {
        if (Test-Path $dst) { [IO.File]::Replace($tmp, $dst, $null) } else { [IO.File]::Move($tmp, $dst) }
        $hecho = $true
    } catch { Start-Sleep -Milliseconds 100 }   # el juego lo estaba leyendo justo en ese instante: se reintenta
}
if (-not $hecho) { Write-Host "No se pudo reemplazar avisos.txt (¿bloqueado?)." -ForegroundColor Red; exit 1 }
Write-Host "Aviso enviado. En el juego: Espacio para abrir el overlay y 1 hasta ver PHOENIX EVOLUTION." -ForegroundColor Green
```

### 8.4 · Contenido de `PhoenixAviso.bat`

```bat
@echo off
rem Phoenix Evolution · enviar un aviso al juego (modulo phoenix.lua de Sider). Se puede usar con el juego abierto.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PhoenixAviso.ps1" %*
pause
```

**Ojo con `PhoenixAviso.ps1`:**
- busca la carpeta del buzón en `..\SiderAddons\content\phoenix`, relativa a donde esté el script;
- comprueba que exista `sider.ini` dos carpetas más arriba;
- se puede pasar otra carpeta con `-Carpeta`;
- corta el texto a 1.500 caracteres;
- convierte `|` en salto de línea;
- reintenta 20 veces cada 100 ms.

---

## 9. Lo que NO sé o no pude comprobar

1. **Original sin parche:** no lo he visto. No sé dónde estaría Sider (no lo trae), ni la carpeta de guardados, ni cómo instalar Sider. [NO PROBADO]
2. **Sudamerican 2026:** no he instalado nada. Faltan:
   - las 2 líneas en sus dos `sider.ini`;
   - generar un `Player.bin` desde `SP_Subs.cpk`;
   - comprobar que la raíz Phoenix-DB gana al CPK (debería, porque Sider gana a los CPK).

   [NO PROBADO]
3. **Variantes Perú/Chile/Uruguay B y Clásicos:** sin Phoenix. No he comprobado en el juego qué pasa al cambiar a ellas y volver. Lo de la sección 5.2 está deducido de las opciones de robocopy. [DEDUCIDO / NO PROBADO]
4. **Sider con una línea duplicada** (`cpk.root` o `lua.module` dos veces): no sé si da error, si carga el módulo dos veces o si lo ignora. [NO PROBADO]
5. **`sider.ini` con BOM o con saltos LF:** no sé si Sider lo lee bien. [NO PROBADO]
6. **Llamada al overlay:** no he medido si Sider llama a `overlay_on` de **todos** los módulos o solo del elegido con el overlay abierto. Por el diseño, el coste es mínimo en los dos casos. [NO PROBADO]
7. **Versión exacta de Sider** en las variantes Perú y Clásicos: deduzco 7.3.3 porque el `sider.dll` que se ejecuta es el de la raíz, y las variantes no traen `sider.dll` propio. [DEDUCIDO]
8. **Los saves de la otra carpeta** (`292733975847239680`): son de Sudamerican, pero no he comprobado si ese parche guarda siempre ahí. [NO PROBADO]
9. **Instalación limpia en un PC de amigo:** nunca se ha hecho. Todo lo probado es en el PC de FRALEX. [NO PROBADO]
10. **Antivirus o permisos de Windows** al escribir en `D:\…\SiderAddons`: no vi problemas en este PC; en otros, no lo sé. [NO PROBADO]
11. **Actualización de ConmeGOL (27 u otra):** no sé si su instalador pisa `ConmeGol Extras\`, y con ello las 2 líneas. [NO PROBADO]
12. **El instalador de Link:** no existe todavía. Tampoco existe en Link el código del «cartero» de avisos: está pedido por prompt (`PhoenixSync/prompts/PROMPT-LINK-buzon-juego.md`), pero no he comprobado si ya está hecho en la rama de Link. [NO PROBADO]
13. **Versiones de memoria (v0.3–v0.17e):** las mantiene la otra cuenta. No las documento aquí (sección 0). Tampoco he comprobado si conviven bien con la v0.2: no pueden estar las dos a la vez, porque las dos se llaman `phoenix.lua`.
