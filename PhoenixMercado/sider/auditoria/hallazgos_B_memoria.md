# Hallazgos B — Módulos Sider que escriben memoria del juego

Auditoría de solo lectura. Juego: PES2021.exe (ConmeGol Patch) con Sider 7.3.3 y LuaJIT 2.1.0-beta3.
Archivos revisados: `SiderAddons/modules/*.lua`, `lib/*.lua`, el `sider.ini` activo y `sider.log`, que tiene 10 851 líneas.

**Qué cubre el log:** la sesión registrada se quedó en los menús. No se jugó ningún partido, porque no aparece ningún mensaje de `set_teams` en ningún módulo. Por eso los parches que se aplican al empezar un partido (SleeveBadge, Commentary-Server y SoundServer vía set_teams) **no se llegaron a ejecutar** en ese log. La sesión terminó limpia: líneas 10840-10851, con `DLL detaching ... All done.`

---

## 0. Hechos del entorno que cambian el análisis

1. **Cada módulo tiene su propio entorno global.** `env.lua`, cargado en la posición 39, volcó `_G` en las líneas 3886-3913 del log. Ahí solo aparecen las funciones base: `input, assert, pairs, ... memory, ipairs, _FILE=env.lua`. No aparece **ninguna** global definida por módulos anteriores: ni `init` de GFX_lod, ni `get_compentry_data` de CommonLib, ni `EPL`/`offsets` de scoreboard-hexx.
   Conclusión: las funciones o variables globales que se declaran sin `local` **no chocan entre módulos**.
   Lo que sí se comparte por referencia son las tablas de librería (`string`, `table`, `math`, `io`, `os`, `memory`, `bit`, `jit`) y el espacio de nombres C de `ffi`, que es único para todos. También se comparte `ctx`.
2. El entorno **no incluye** `pcall`, `xpcall`, `select`, `next`, `setmetatable`, `print` ni `require`. Ningún módulo de este alcance los usa, así que no hay problema.
   Un error de Lua dentro de un evento lo atrapa Sider y no tumba el juego. Un error dentro de `init` hace que el módulo no se cargue, pero **los parches que ya escribió antes del error se quedan puestos**.
3. Lo que sí puede cerrar el juego es un **acceso de memoria inválido**: un `memory.read` o `memory.write` a una dirección no mapeada, como un puntero nulo + offset. Eso es un fallo nativo (access violation) que Lua no puede atrapar.
4. `key_down` solo llega al módulo que está **activo en el overlay**. Por eso las teclas repetidas entre módulos (`0`, `DEL`, `-`, `+`) no chocan.
   El overlay se abre y cierra con **Espacio** (`overlay.vkey.toggle = 0x20`), pasa de módulo con `1`/`` ` ``, y Sider recarga todos los módulos con `Shift+R` (`vkey.reload-1=0x10`, `reload-2=0x52`).
5. Los mandos están desactivados (`gamepad.dinput/xinput.enabled = 0`), así que el evento `gamepad_input` no se dispara.
6. En este parche la **competición con tid 17 es la Liga Profesional Argentina (CGP LPF)**, según el log línea 2921, no la Premier League. Esto afecta a scoreboard-hexx (ver §2).

---

## 1. Chatty_AutoLineup_v0.8.lua

1. **Propósito:** pone automáticamente la alineación nativa (FORM/ABILITY) para los dos equipos. Está hecho para **Football Life 2026 (FL_2026.exe)**, no para PES2021 ConmeGol.
2. **Eventos:**
   - `set_teams` y `context_reset`: reinician el ejecutor de un solo uso.
   - `after_set_conditions`: comprueba el estado.
   - `overlay_on`: se ejecuta en cada frame mientras el overlay está abierto en este módulo, pero solo lee (`refresh_runner_status`).
   - `key_down`.
3. **Memoria:**
   - No usa AOB. Usa direcciones fijas `IMAGE_BASE 0x140000000` + RVA (entry 0x8F02A0, etc.).
   - Antes de tocar nada valida la cabecera PE (timestamp, SizeOfImage, base, relocs) y huellas de 11 regiones de código.
   - Si todo cuadra, escribiría un hook de 18 bytes (`FF 25 ...` + NOPs) con la regla "comparar antes de escribir", en una cueva creada con `memory.allocate_codecave(2048)`. Restaura los bytes originales al desinstalar (teclas END, -/+ o fallo).
   - **En este juego se niega:** log línea 456, `Unsupported build: PE image-size mismatch`. Queda `failure_latched = true` y **no escribe nada**. Solo lee la cabecera PE en 0x140000000, que siempre está mapeada.
4. **ffi/cuevas:** `memory.allocate_codecave`. No usa VirtualProtect directamente. No toca `custom_evt_rbx`.
5. **Archivos:** solo lee `modules\Chatty_AutoLineup.ini`.
6. **Errores:** no usa pcall. Valida todo y, si algo no cuadra, se apaga sin escribir (fail-closed).
7. **Teclas:** `-` 0xBD, `+` 0xBB, `8` 0x38, END 0x23, HOME 0x24. Con el fallo enclavado no hacen nada.
8. **Log:** 450-457, en particular 456 `Unsupported build`.
9. **Riesgo: BAJO.** No hace nada en este exe. **Recomendación:** quitarlo del `sider.ini` porque es un módulo de FL26.

## 2. scoreboard-hexx.lua (v2.0.7 EPL)

1. **Propósito:** cambia en memoria los colores y texturas del marcador "Afandix Premier League".
2. **Eventos:**
   - `set_teams` (`teams_selected`): llama a `EPL(ctx)` y, si da verdadero, a `reset_scores`, que solo modifica una tabla Lua de `match.stats()` y no la memoria del juego.
   - `livecpk_data_ready`: se ejecuta con cada archivo que carga el juego. Hace un `tostring` del puntero y vuelve enseguida si `EPLInGame` es falso.
   - `key_down`.
   - `overlay_on`: en cada frame, si `ctx.mis` existe y `EPL()` da verdadero, **lee** `ctx.mis+0x13c`, `+0x7cc`, `+0xe60` y `+0x10b8`.
3. **Escrituras** (solo si el marcador activo termina en `Afandix Premier League`):
   - En los **búferes de archivo recién cargados** (`game2dEngland(Enter).bin`, `game2dPes(Enter).bin`, `licenceTexture*.bin`, `pauseMenu.bin`): escribe colores de 3 bytes y bytes sueltos en `data + offset`, con offsets sacados de `content\scoreboard-hexx\all_offset.ini`. **No comprueba que el offset quepa dentro de `len`**. También lee `data+0x231864` sin comprobar el tamaño del archivo.
   - Busca en el **heap** con `memory.safe_search` en el rango `[addr−0x25000000, addr+0x1000000]` o `[ctx.mis−0x40000000, ctx.mis]`, es decir, hasta 1 GB:
     - patrón `0C 00 C0 1F 26 00 00 00 01 00 36 00 0C 00 BC 05`, del que se resta 0x3faf (lineup);
     - patrón `0C 00 C0 1F 46 00 00 00 01 00 9F 00 06 00 0B 00`, del que se resta 0x65d7 (goalplate).
   - Después, durante el partido, escribe 4 u 8 bytes en `game2dEnglandAddress − 1 + offset` (CardTime, CardScorer, CardTeamLogo, stats, etc.) cada vez que se carga cierto archivo (.fdc/.json/.awb).
   - La dirección se guarda entre eventos y solo se borra con `MatchEnd`/`MatchDiscontinue`. **Si ese búfer se libera, escribe sobre memoria del heap que ya no es suya.** No restaura nada.
   - Lecturas `memory.read(data+0x2a/0x25/0x2d/0x21, 5)` para buscar la firma `P.apk`.
4. **ffi:** `ffi.cast("intptr_t"/"char*")` sobre `addr` y `ctx.mis`. No hay cuevas.
5. **Archivos:** solo lee `colors.ini`, `all_offset.ini` y `teams_str.ini`.
6. **Errores:** sin pcall. `writeColorFunction` registra un aviso si falta un color, pero `changeColorAway`/`changeGoalColor` llaman a `colorHexToLittleEndianBytes(nil)` → error de Lua (atrapado por Sider).
7. **Teclas:** `0` (recarga los .ini), `DEL`.
8. **Log:** 3287-3299 (carga correcta). No hay líneas "It's EPL" porque no hubo partido.
9. **Riesgo: BAJO en esta instalación**, porque el marcador activo es CGP y entonces `Afandix_epl_sb = false` y no escribe.
   **ALTO si alguien activa un marcador "Afandix Premier League"**: búsqueda en heap de hasta 1 GB (tirón al cargar), escrituras sin comprobar límites y punteros guardados de un partido a otro.
   Ojo: como tid 17 aquí es Argentina, `EPL()` da verdadero en partidos de la LPF (y en amistosos entre los IDs 100-107, etc.), así que el overlay lee `ctx.mis`. Es solo lectura, pero no tiene sentido para este parche. **Recomendación:** desactivarlo.
   Globales sin `local` (más de 60, como `EPL`, `offsets`, `failed`, `i`, `j`...): quedan confinadas a su propio entorno.

## 3. SleeveBadge-ArmbandServer.lua (v3.4, juce/Hawke)

1. **Propósito:** pone parches en la manga y el brazalete de capitán según la competición o el equipo.
2. **Eventos:**
   - `livecpk_get_filepath`: se ejecuta con **cada archivo**. Solo compara texto.
   - `set_teams` y `set_home_team_for_kits` (en modo edición): llaman a `set_patches`, que quita todos los parches y vuelve a poner los que hagan falta.
   - `overlay_on`: en cada frame, solo cuenta entradas.
   - `key_down`.
3. **Escrituras** (todas son **parches de código**):

| Qué | AOB (inicio) | Offset | Bytes nuevos | Original restaurado |
|---|---|---|---|---|
| Brazalete | `83 FB 21 74 59 48 C7 47 18 0F 00 00 00` | +0 → **0x141ea1fb8** | `90 90 90 EB` | `83 FB 21 74` (fijo en el código) |
| Parche derecho | `66 85 FF 74 52 4C 8B CB 4C 8D 85 C8 00 00 00` | +0 → **0x141ea44e9** | 5×`90` | `66 85 FF 74 52` |
| Parche izquierdo | (badge + 0x5C) | **0x141ea4545** | 5×`90` | `66 85 FF 74 53` |
| HARL | `49 8B F8 48 8B F2 66 83 F9 64 73 6F 0F B7 C1` | +0x1A → **0x141ea4c11** | `48 B8 <cueva u64> FF D0 90 90 90` (15 B) | `89 44 24 20 4C 8B C3 48 8D 4C 24 30 8D 53 21` |

   - **Cueva:** `ffi.C.VirtualAlloc(nil, 512, MEM_COMMIT|RESERVE, PAGE_EXECUTE_READWRITE)`, en **0xaf0000** según el log. Copia 63 bytes que recalculan el ID del parche (+0x1000 visitante, +0x2000 izquierdo, +0x4000) y repiten las instrucciones que fueron reemplazadas. Nunca se libera.
   - **Validación:** cada AOB tiene que encontrarse en `init`; si no aparece, `error()` y el módulo no carga. **No comprueba duplicados**: usa la primera coincidencia. Las desinstalaciones escriben bytes originales fijos sin comparar antes.
   - Los parches se escriben en `set_teams` mientras corre el juego (escritura no atómica de 15 bytes). El riesgo de que otro hilo esté ejecutando ese código justo entonces es bajo, porque se hace en el menú.
4. **ffi:** `ffi.cdef` de `typedef LPVOID/SIZE_T/DWORD/BYTE` + `VirtualAlloc`. **shirtless_celebration.lua declara la misma función `VirtualAlloc`** en el espacio ffi compartido, con firma equivalente (`uint8_t*`, `uint64_t`). No dio error (ambos módulos cargan, líneas 443 y 1536), pero las typedefs genéricas `LPVOID`/`DWORD` podrían chocar con otro módulo que las declare distinto.
5. **Archivos:** solo lectura de `content\badge-server\map.txt` y `map_teams.txt`.
6. **Errores (bugs reales):**
   - a) En `set_patches`, si la competición no es amistoso, no está en `map.txt` y no se resuelve otra → `content_root .. folder` con `folder = nil` → **error en set_teams** en cada partido de esa competición. El módulo ya quitó los parches antes del error, así que el estado queda coherente y no hay cuelgue.
   - b) `has_a_badge` es **global y nunca se vuelve a false**: después del primer partido con parche propio, el parche HARL se instala en todos los partidos siguientes. Es cosmético: los parches pueden desaparecer.
   - c) `load_map` sin el archivo y con `required = false` → `f:close()` sobre nil → error.
   - d) Al recargar con Shift+R, los patrones del brazalete y del parche ya están modificados → no se encuentran → `error` y el módulo no se recarga. Los parches y la cueva anteriores siguen activos.
7. **Teclas:** `0` (recarga los mapas).
8. **Log:** 1243-1249 (las 4 direcciones encontradas y la cueva), 1440 (190 entradas), 1530 (89 entradas), 1536 OK.
9. **Riesgo: MEDIO.** Bien validado con AOB, pero parchea código y crea cueva RWX, y tiene los bugs a) y b). No hubo partido en el log para comprobarlo en marcha.

## 4. SoundServer.lua (GOGOSZ, traducido)

1. **Propósito:** ajusta 6 volúmenes (menú, narración, público, anuncios, música del estadio, efectos).
2. **Eventos:**
   - `set_teams` y `set_conditions`: escriben los 6 floats.
   - **`overlay_on`: en cada frame mientras su overlay está abierto escribe los 6 floats** (`apply_settings`).
   - `key_down`.
3. **Memoria:**
   - **No usa AOB.** Usa una dirección fija **0x143AA1178** y sigue una cadena de punteros `[+0xB8] → [+0xA8] → [+0x170] → +0x500`, **sin comprobar en ningún paso si el puntero es 0**.
   - Escribe floats en `base−0x50`, `+0x40`, `+0x50`, `+0xA0`, `+0xF0` y `+0x190`. Son datos, no código, y no se restauran.
   - Las bases se calculan **una sola vez** (`settings == nil`) y se guardan para siempre: si el motor de sonido vuelve a reservar ese objeto, escribe sobre memoria del heap que ya no es suya.
   - Siempre impone los valores del .ini (todos 1.00), **por encima de los ajustes de volumen del juego**, en cada set_teams y set_conditions.
4. **ffi:** no.
5. **Archivos:** escribe `modules\SoundServer.ini` con cada `+`, `-` o `9`.
6. **Errores:** sin pcall. Un puntero nulo en la cadena provoca un **acceso inválido nativo, y eso sí cierra el juego**.
7. **Teclas:** RePág 0x21, AvPág 0x22, `-` 0xBD, `+` 0xBB, `9` 0x39.
8. **Log:** 3301-3306 (carga), 10779-10781 y 10813-10815: su overlay se abrió y la cadena se leyó y escribió **sin cuelgue** en el menú. `PROBLEM: Cannot load texture ... gervol.png` es una imagen que falta y no tiene importancia.
9. **Riesgo: MEDIO-ALTO.** Dirección fija ligada a una versión concreta del exe, sin comprobar nulos, punteros guardados para siempre y escritura en cada frame.

## 5. UIColors.lua (Zlac v1.1)

1. **Propósito:** cambia la tabla de colores del interfaz que está dentro del exe y además modifica archivos .bin del interfaz.
2. **Eventos:**
   - `overlay_on`: solo texto.
   - `key_down`.
   - `livecpk_read`: se ejecuta con cada bloque leído de cada archivo, pero la función está vacía.
3. **Memoria:**
   - AOB `10 10 10 FF FF FF FF FF 00 44 93 FF`, con `search_process` (primera coincidencia). En el log: **0x1434c08d0**.
   - Escribe 4 bytes en `addr + (índice−1)*4`, con índice entre 1 y 80. El rango máximo es 0x1434c08d0 a 0x1434c0a0f. Son **datos**.
   - Al recargar con la tecla `0` compara la memoria con los colores aplicados la última vez (`validate_memory`) y se niega si no cuadran.
   - No restaura nada. Si no encuentra el patrón, `error()` en init.
4. **ffi:** no.
5. **Archivos:** **escribe en `livecpk\UIColors\*.bin`** (copia desde `DO_NOT_EDIT\` y parchea) en cada init y en cada recarga.
6. **Errores:**
   - `_empty` no está definido (sería nil si faltara CommonLib, pero CommonLib carga primero).
   - `memory.unpack("b", ...)` funciona en 7.3.3: se evalúa en init y no falló.
7. **Teclas:** `0` 0x30, `DEL` 0x2E.
8. **Log:** 3762-3852. Aplica los índices 1, 2, 3, 4, 79 y 80 y termina con "Patching complete" (3846-3847). En 3850-3851 falta `topModeSelectDMM.bin` en DO_NOT_EDIT (error de contenido, inofensivo).
9. **Riesgo: BAJO.**

## 6. Competition-Server.lua (GRAND v2.1)

1. **Propósito:** reescribe la tabla estática de competiciones (formato, ascensos y descensos, comentarios, música...) a partir de `content\Competition-Server\data.csv`.
2. **Eventos:** ninguno. Todo ocurre en `init`.
3. **Memoria:**
   - AOB de 32 bytes, casi todos ceros: `00×8 01 00 00 00 4F 00×19`. Usa la primera coincidencia en todo el proceso. **No comprueba nil:** si no lo encuentra, `nil + 8` da error. Tampoco comprueba duplicados, y un patrón tan genérico podría coincidir en otro sitio.
   - `Addr = found + 8`. Por cada fila del CSV escribe en `Addr + col1 + offsets[cabecera]` el valor `values[cabecera][valor]`, de 1 a 9 bytes. Son **datos**.
   - No comprueba rangos ni hace copia de los bytes originales. Una cabecera o valor desconocido da error a mitad del init, **y lo que ya escribió se queda**.
4. **ffi:** no.
5. **Archivos:** solo lee `data.csv`.
6. **Errores:** sin pcall, sin mensajes de log. **No registra nada**, así que no se puede saber qué escribió ni dónde.
7. **Teclas:** ninguna.
8. **Log:** 3879-3880 (cargó OK, en silencio). No se puede confirmar si `data.csv` existe porque la carpeta `content` no se entregó.
9. **Riesgo: MEDIO.** Patrón débil, escrituras sin validar y sin registro. Una configuración de competición corrupta suele dar fallos en ML o Liga, no en exhibición.

## 7. GFX_lod.lua

1. **Propósito:** fija el LOD gráfico mínimo y máximo.
2. **Eventos:** ninguno. Escribe directamente en `init`, que además se declara **global** (sin efecto fuera de su entorno).
3. **Memoria:** **direcciones absolutas fijas** 0x14297B0C1 ← `0x20` y 0x14297B0C2 ← `0x40`, "para PES2021 patch 1.01.01". Sin AOB, sin leer antes, sin restaurar. Si el exe no es 1.01.01, escribe 2 bytes en datos arbitrarios del exe.
4. **ffi:** no. 5. **Archivos:** ninguno. 6. **Errores:** ninguno visible. 7. **Teclas:** ninguna.
8. **Log:** 3882-3883 (OK, sin mensajes; no hay forma de verificarlo desde el log).
9. **Riesgo: ALTO (no se puede verificar).** Es la única escritura "a ciegas" del conjunto. Además, Commentary-Server dice requerir el exe 1.3.0+ y SoundServer usa otra dirección fija de una versión más moderna, así que lo más probable es que **este exe no sea 1.01.01**. **Recomendación:** desactivarlo o convertirlo a AOB con comprobación de bytes antes de escribir.

## 8. awaygoals.lua (juce v1.0)

1. **Propósito:** desactiva la regla del gol de visitante.
2. **Eventos:**
   - `overlay_on`: en cada frame lee 11 bytes. Es solo lectura.
   - `key_down`.
3. **Memoria (código):** 6 AOB, cada uno con su comprobación de que el byte vale el original o el nuevo antes de escribir (si no, `error`):

| # | Patrón | Offset → dirección (log) | Original → nuevo |
|---|---|---|---|
| 1 | `41 8D 0C 50 42 8D 04 4B 3B C1` | +3 0x14042dabb, +7 0x14042dabf | 50→10, 4B→0B |
| 2 | `43 8D 0C 41 43 8D 04 56 3B C1` | +3 0x140449472, +7 0x140449476 | 41→01, 56→16 |
| 3 | `80 7D 69 00 74 08 8D 04 09` | +7 0x140449aa8, +8 0x140449aa9 | 04→01, 09→90 |
| 4 | `8D 0C 41 8B 84 24 94 00 00 00 89 4C 24 28 8D 34 00` | +2 0x141478309, +15/16 0x141478316/17 | 41→01, 34→30, 00→90 |
| 5 | `0F B6 4E 1F 0F B6 50 1F 89 4D 38 89 55 F0 3B CA 74 70` | +16 0x141515114 | 74→EB |
| 6 | `74 48 8B 4D 38 85 C9 74 41 48 8B 75 E8` | +0 0x1415151d8 | 74→EB |

   - Restaura con la tecla `0` (toggle). Usa la primera coincidencia y no comprueba duplicados.
   - Al recargar con Shift+R, como los patrones ya incluyen bytes modificados, no se encuentran → `error`. El parche sigue puesto (gol de visitante desactivado).
4. **ffi:** no. 5. **Archivos:** ninguno. 6. **Errores:** errores claros en init. La global `all_found` no se usa.
7. **Teclas:** `0` 0x30.
8. **Log:** 9713-9729. Los 11 bytes aplicados, "away-goals is now: OFF".
9. **Riesgo: BAJO.**

## 9. StartingYearChanger.lua + .ini

1. **Propósito:** cambia el año de inicio de ML/BAL. El .ini dice `StartingYear=2026`.
2. **Eventos:** `overlay_on` (solo texto), `key_down`.
3. **Memoria (código, inmediato de `mov eax, imm32`):**
   - AOB `8B 70 48 48 8D 96 1E 2A 64 01 B8 <año u16> 00 00 66`, buscando con el año actual. En el log: **0x1412695f0**.
   - Reescribe los 16 bytes; solo cambian +11 y +12 (E4 07 → EA 07).
   - Si no lo encuentra → `error("unable to find magic string")`. No guarda un original aparte: el "original" es el año anterior.
   - Al recargar con Shift+R vuelve a buscar el año 2020 (`year_curr` se reinicia), no lo encuentra → error y el módulo no carga. El año 2026 sigue aplicado.
4. **ffi:** no.
5. **Archivos:** **escribe `modules\StartingYearChanger.ini`** con cada pulsación de 6 o 7.
6. **Errores:** si el .ini falta o está vacío, `load_ini` devuelve `{}` y no `nil`, así que `default_year` nunca se aplica → `memory.pack("u16", nil)` da error. La global `s` es inofensiva.
7. **Teclas:** `6` 0x36, `7` 0x37.
8. **Log:** 9738-9747: "found magic string at 0x1412695f0", "starting year was 2020", "new starting year: 2026".
9. **Riesgo: BAJO.** Cambiar el año a mitad de una ML guardada puede crear incoherencias de datos, pero no cuelgues.

## 10. PES ID.lua (v1.6a)

1. **Propósito:** en el menú Editar, muestra en el overlay el ID del equipo y del jugador y las rutas del face/textura.
2. **Eventos:**
   - `livecpk_data_ready`: con cada archivo; compara texto y, en modo edición, lee memoria.
   - `overlay_on`: devuelve texto ya preparado.
3. **Memoria (solo lectura):**
   - AOB `C7 40 14 FF FF FF FF 48 83 C4 28 C3` y `48 85 C0 74 33 3B 90 88 00 00 00`. Calcula el puntero como `addr + u32[addr−4]`. Es un desplazamiento relativo que **se lee como u32 sin signo**; si fuera negativo daría una dirección fuera de la imagen.
   - En edición: `subpTN = [pointerTN]` (se lee **una sola vez** y se guarda para siempre) y `subpSP = [pointerSP]`. Después lee `subpTN+16`, `+32`, `subpSP+196` y `+216`.
   - Comprueba que `subpSP` no sea 0, **pero no comprueba `subpTN`**: si valía 0 al entrar en EditPlTeamSelect, la siguiente carga de un face lee la dirección 0x10 → **acceso inválido**.
4. **ffi:** no. 5. **Archivos:** ninguno.
6. **Errores:** si falta un AOB, `error` en init.
7. **Teclas:** ninguna.
8. **Log:** 445-448 (AOB encontrados, sin mensajes).
9. **Riesgo: BAJO-MEDIO.** Solo lee, pero puede leer de un puntero nulo o viejo en Editar → Jugadores.

## 11. Commentary-Server.lua (v2.0)

1. **Propósito:** elige el idioma de la narración por competición.
2. **Eventos:**
   - `set_teams`: aplica el parche.
   - `livecpk_get_filepath`: con cada archivo, compara contra 17 patrones. Detecta el idioma del usuario con `00_TEAM.acb` y **quita el parche cuando se pide `40_MAIN.awb`**.
   - `overlay_on`, `key_down`, `key_up` (vacío).
3. **Memoria:**
   - AOB A `45 0F B6 F6 83 F8 13 45 0F 44 F4 48 8B 0D` → +14 → `structure_addr = pos + 4 + i32` (variable global del exe).
   - AOB B `48 8B 47 70 48 8B 00 8B 48 18 E8` → `read_language_addr` (**código**). Ninguna de las dos direcciones se escribe en el log.
   - En `set_teams` y con la tecla `2`: lee el puntero `[structure_addr]` **sin comprobar que no sea 0** y escribe `08` en `ptr+0x94` (dato; **nunca se restaura**). Después escribe 10 bytes en `read_language_addr`: `48 B9 <lang> 00×7` (mov rcx, imm64), que sustituyen a `mov rax,[rdi+70]; mov rax,[rax]; mov ecx,[rax+18]`.
   - Restaura con `48 8B 47 70 48 8B 00 8B 48 18` cuando se carga `40_MAIN.awb`.
   - Si el idioma no está en la tabla, usa el código **0x18**, que está fuera del rango 0x00-0x17. Eso puede dejar sin narración o, en el peor caso, provocar un índice inválido en el juego.
   - Al recargar con Shift+R con el parche puesto, el AOB B no se encuentra → `error` y el idioma queda fijo.
4. **ffi:** no.
5. **Archivos:** solo lee `content\commentary-server\map_competitions.txt`.
6. **Errores:** errores en init si falta un AOB. `math.randomseed(os.time())` al cargar cambia la semilla del `math` compartido (inofensivo).
7. **Teclas:** `0`, `DEL`, `2` (cambiar idioma), `3` (modo automático).
8. **Log:**
   - 3954-4164: 139 IDs mapeados y **57 avisos** "Line ignored" entre 3963 y 4153.
   - Varios son errores del .txt: `51ags,ger`, `166ags,ger`, `167ags,ger` y `54ags,ger` (falta una coma; líneas 4039-4042), líneas `,	#Liga"` y entradas sin idioma.
   - 10214-10237: idioma del usuario detectado como `ags`.
9. **Riesgo: MEDIO.** Parche de código con restauración, pero desreferencia un puntero sin comprobar nulo y escribe un dato que nunca restaura.

## 12. lib/CommonLib.lua (v1.1)

1. **Propósito:** librería que se comparte vía `ctx.common_lib`. Mapea equipo → liga y competición → tid leyendo `CompetitionEntry.bin` y `CompetitionRegulation.bin`.
2. **Eventos:** `livecpk_read`, que se dispara con **cada bloque leído de cada archivo**. Hace `lower` + `match` dos veces; es un coste pequeño.
3. **Memoria:** solo `memory.read(addr, len)` del búfer que da Sider. **No escribe.**
4. **ffi:** no. 5. **Archivos:** ninguno.
6. **Errores:** `zlib.uncompress` puede fallar con un archivo corrupto (lo atrapa Sider). Las globales `get_compentry_data`, `get_compreg_data` y `read_file_contents` quedan confinadas.
7. **Teclas:** ninguna.
8. **Log:** 294-296. Nota: `inventario.json` marca `lib\CommonLib.lua` como "existe: false", pero el log demuestra que sí carga (falso negativo por la barra invertida en la ruta).
9. **Riesgo: BAJO.**

## 13. lib/nesalib.lua (v1.1)

1. **Propósito:** ayudante que comparten los módulos de cámara (`ctx.nesalib`): .ini, overlay, teclas y caché de patrones.
2. **Eventos:** lo registra el módulo que lo usa: `after_set_conditions`, `set_teams` (con `load_ini`) y, a través del módulo, `overlay_on`, `key_down` y `gamepad_input`.
3. **Memoria:**
   - `apply_settings` escribe floats o bytes en `bases[x] + offs` (direcciones que da cada cámara; las cubre otro auditor).
   - `cache.find_pattern` lee `memory.read(addr_cacheada, #pattern)` con una dirección sacada de `<módulo>.cache`, sin validarla. Si el .cache está corrupto, puede leer una dirección no mapeada.
   - `search_process` local, con búsqueda por secciones a partir de `start_from`.
4. **ffi:** no.
5. **Archivos:** **escribe** `<sider_dir>\<módulo>.cache` (`t.save`, sin comprobar `io.open`) y el .ini de cada módulo (`save_ini`).
6. **Errores / bugs:**
   - `find_pattern` sin `cache_id` devuelve nil sin buscar.
   - El log de `set_teams` usa la global `key`, que no existe (imprime "nil").
   - Las globales `sname` y `addr` quedan confinadas.
7. **Teclas** (en el módulo que lo usa): `7` modo, `8` restaurar, `9`/`0` elegir, `-`/`+` valor, stick derecho del mando.
8. **Log:** 316-317 y sus mensajes aparecen como `[lib\nesalib.lua]` en las cámaras, por ejemplo 8013-8031.
9. **Riesgo: BAJO** por sí mismo.

## 14. env.lua

Solo es de diagnóstico: en `init` vuelca `_G`, `ctx` y `os` al log. No escribe memoria ni archivos y no tiene eventos.
**Log:** 3885-3931. La línea 3896 `[env.lua] error: function: builtin#19` **no es un error**: es la variable global llamada `error`.
**Riesgo: BAJO.** Se puede quitar. Ha resultado útil porque prueba que cada módulo tiene su propio entorno.

## 15. matchset.lua

Evento `set_match_settings`. Para tid 65535 pone `difficulty`, `extra_time` y `penalties` = nil, que significa "lo que elija el juego". Para otros tid pone `substitutions = nil`.
**En la práctica no hace nada.** No escribe memoria (las opciones las aplica el núcleo de Sider).
**Log:** 9693-9695. **Riesgo: BAJO.**

---

## CROSS-MODULE

### A. Mapa de ubicaciones parcheadas

| Dirección (log) | Patrón / origen | Tipo | Módulo |
|---|---|---|---|
| 0x14042dabb / 0x14042dabf | `41 8D 0C 50 42 8D 04 4B 3B C1` +3/+7 | código | awaygoals |
| 0x140449472 / 0x140449476 | `43 8D 0C 41 43 8D 04 56 3B C1` +3/+7 | código | awaygoals |
| 0x140449aa8-aa9 | `80 7D 69 00 74 08 8D 04 09` +7/+8 | código | awaygoals |
| 0x141478309, 0x141478316-317 | `8D 0C 41 8B 84 24 94 00 ...` +2/+15/+16 | código | awaygoals |
| 0x141515114 | `0F B6 4E 1F ... 3B CA 74 70` +16 | código | awaygoals |
| 0x1415151d8 | `74 48 8B 4D 38 85 C9 74 41 48 8B 75 E8` +0 | código | awaygoals |
| 0x1412695f0 (+11..12) | `8B 70 48 48 8D 96 1E 2A 64 01 B8 <año> 00 00 66` | código (imm) | StartingYearChanger |
| 0x141ea1fb8 (4 B) | `83 FB 21 74 59 48 C7 47 18 0F 00 00 00` | código | SleeveBadge |
| 0x141ea44e9 (5 B) | `66 85 FF 74 52 4C 8B CB 4C 8D 85 C8 00 00 00` | código | SleeveBadge |
| 0x141ea4545 (5 B) | badge + 0x5C | código | SleeveBadge |
| 0x141ea4c11 (15 B) → cueva 0xaf0000 | `49 8B F8 48 8B F2 66 83 F9 64 73 6F 0F B7 C1` +0x1A | código + cueva RWX | SleeveBadge |
| ? (no registrado), 10 B | `48 8B 47 70 48 8B 00 8B 48 18 E8` | código | Commentary-Server |
| [ptr en `45 0F B6 F6 83 F8 13 45 0F 44 F4 48 8B 0D`+14] + 0x94 | puntero de datos | heap | Commentary-Server |
| 0x1434c08d0 … 0x1434c0a0f | `10 10 10 FF FF FF FF FF 00 44 93 FF` | datos | UIColors |
| 0x14297B0C1-C2 | **dirección fija** | ¿datos? | GFX_lod |
| [[[[0x143AA1178]+B8]+A8]+170]+500 ±… | **dirección fija** + cadena de punteros | heap | SoundServer |
| ? (no registrado) found + 8 + CSV | `00×8 01 00 00 00 4F 00×19` | datos | Competition-Server |
| búferes de archivos .bin y heap (safe_search) | `0C 00 C0 1F 26/46 ...` | heap | scoreboard-hexx (inactivo aquí) |
| 0x1408F02A0 (RVA 0x8F02A0) | PE + huellas | código | Chatty (se negó, no escribió) |

### B. Solapamientos y casi-solapamientos

- **Ningún solapamiento real** entre los módulos de este alcance. Las parejas más cercanas son:
  - SleeveBadge 0x141ea4c11 contra el hook del núcleo de Sider `hook_call_rdx` 0x141EA5317: separados 0x706.
  - SleeveBadge 0x141ea44e9 contra el patrón 22 de Sider en 0x141EA3240: separados 0x12A9.
  - awaygoals 0x1415151d8 contra el hook de Sider 0x14150F5A0: separados 0x5C38.
  - UIColors 0x1434c08d0 contra BroadCastCam 0x1434a00c0 y StadiumCam 0x1434a00cc: unos 0x20800.

  Ninguna está a menos de 64 bytes ni en la misma instrucción.
- Dentro de SleeveBadge, el brazalete, el parche y el HARL están en la **misma zona de código** (0x141ea1fb8-0x141ea4c1f), pero son del mismo módulo y están coordinados.
- **Fuera de este alcance, pero se ve en el log:** `camera.lua` y `DynamicWideCam.lua` **parchean la misma instrucción** 0x1408a1fb9 (Dynamic Wide angle read).
  - camera la redirige a 0x1425985a4 (líneas 4470-4472).
  - DynamicWideCam toma esa dirección como "original" y la redirige otra vez a 0x1425985d0 (8019-8021), escribiendo 0.1 sobre un dword que no era cero (`3.6e-41 → 0.1`, línea 8025).
  - Se lo dejo al auditor de cámaras.
- **Commentary-Server** cambia el idioma por código. Ningún otro módulo de los que se pueden leer como texto toca esos patrones. Los módulos en bytecode (CGP_*, Banner*, FWC_*) no contienen esas cadenas AOB.

### C. Nombres globales y estado compartido

- **No hay colisiones efectivas de globales**, porque cada módulo tiene su propio entorno (§0.1).
- Globales que se repiten entre módulos **activos**, todas confinadas y por tanto inofensivas:

| Nombre global | Módulos que lo definen |
|---|---|
| `init` | GFX_lod, Entrance, Entrance_fix, IntroServer, movieintro |
| `home` / `away` | Entrance, IntroServer, MiniFaceServer, TournamentCornerFlags, movieintro |
| `tid` | IntroServer, MiniFaceServer, movieintro, trophy |
| `make_key` | Entrance, Entrance_fix, TournamentCornerFlags |
| `check`, `data_ready`, `intro_file`, `intro_folder`, `leg`, `overlay_on`, `rewrite` | IntroServer, movieintro |
| `addr` | camera, lib\nesalib |
| `helper` | BroadCastCam, FanViewCam |
| `custom_contrast`, `team_main_color`, `team_text_colors` | CGP_SB_Addons, CGP_SB_Addons_2 |
| `logResult`, `make_log` | MiniFaceServer (y SubBoard, que no está cargado) |

- `t2s` y `startsWith` son globales **solo en MasterLeague.lua**. kserv tiene su propio `t2s` local. No hay choque.
- Lo que **sí se comparte de verdad**:
  - `function string:split` en **MasterLeague.lua** (WeatherConditions, no cargado, define otra). Modifica la tabla `string` de todos los módulos. Solo MasterLeague la usa, así que es inofensivo.
  - `math.randomseed(os.time())` en Commentary-Server, BallServer, Entrance, MenuServer, RefKitServer y StadiumServer. Solo cambia la semilla, inofensivo.
  - **Espacio ffi C compartido:** `VirtualAlloc` declarado por **SleeveBadge** (con typedefs `LPVOID`, `SIZE_T`, `DWORD`, `BYTE`) y por **shirtless_celebration**. Las firmas son compatibles y no hubo error al cargar, pero es el único punto de colisión real posible si otro módulo declara esas typedefs de otra forma.
  - En `ctx`: `ctx.common_lib` (CommonLib; lo usan SleeveBadge, scoreboard-hexx, Commentary, UIColors y MenuServer), `ctx.nesalib`, `ctx.ui_colors` / `ctx.ui_colors_exe` (UIColors), `ctx.scoreboard_server` (CGP; lo lee scoreboard-hexx) y `ctx.mis` (lo leen scoreboard-hexx y CGP_SB_Addons).
- **Teclas:** `0` la usan awaygoals, SleeveBadge, UIColors, scoreboard-hexx, Commentary y nesalib; `-`/`+` la usan Chatty, SoundServer y nesalib; `DEL` la usan UIColors, scoreboard-hexx y Commentary. No chocan porque `key_down` solo llega al módulo activo en el overlay.

### D. Riesgo de recarga con Shift+R

Al recargar, Sider vuelve a ejecutar los `init`:

- **No se recargan** (error de AOB porque el código ya está parcheado), aunque los parches anteriores siguen activos: SleeveBadge, awaygoals, StartingYearChanger y Commentary-Server (este último si había un partido en curso).
- SleeveBadge reservaría una cueva nueva y la antigua quedaría perdida en memoria.
- No cuelga el juego, pero deja módulos medio cargados. Evitar Shift+R con el juego abierto.

### E. Otros datos del log (fuera de este alcance)

- 11 módulos del `sider.ini` **no existen** en la carpeta: tunnel, Stadium_Banner, Stadium_Board, Stadium_CornerFlag, GoalSongServer, Derbys, cr7_audio, BallBoysServer, tournament_anth_tunnel, Nets-Server y CornerFlag (log 3934-3952, 4455, 9698, 9708, 9711). Son inofensivos, pero conviene quitarlos.
- `goal_scored` (340) y `overlay_off` (9732) son eventos que Sider 7.3.3 no reconoce.

### F. Prioridad de acciones

1. **GFX_lod**: desactivarlo o verificar la dirección para este exe. Es la única escritura a ciegas.
2. **SoundServer**: añadir comprobación de nulos y recalcular la base en cada `set_teams`, o desactivarlo si no se usa.
3. **Competition-Server**: comprobar que existe `data.csv` y que el AOB es único. Añadir log.
4. **Commentary-Server**: comprobar que el puntero no es 0 antes de escribir en `+0x94` y corregir las líneas `NNags,ger` del .txt.
5. **SleeveBadge**: corregir el `folder` nil en `set_patches` y volver a poner `has_a_badge` a false (y declararla `local`).
6. Quitar **Chatty_AutoLineup** (es de FL26), **scoreboard-hexx** (es para Afandix EPL y no se usa aquí) y **env.lua**.
