# Hallazgos D — Módulos de archivos (livecpk), bytecode y sider.ini

Auditoría de solo lectura de Sider 7.3.3 (juce66) para PES 2021, instalación "Conmegol Patch".
Fuentes: `ConmeGOL Patch 26/SiderAddons/sider.ini` (activo), `SiderAddons/modules/` (copia del PC),
`arbol.txt` (árbol del parche, 2 niveles), `SiderAddons/sider.log` (10 851 líneas).

Nota: las copias de módulos dentro de `ConmeGol Extras/ConmeGOL Patch 26/SiderAddons/modules/`
tienen el mismo MD5 que las de `SiderAddons/modules/`. No hay versiones distintas del mismo archivo.

---

## 0. Resumen corto

- **La sesión del log terminó limpia.** Hay `DLL detaching ... All done`. No se jugó ningún partido
  (no hay `set_teams`, ni `trophy_rewrite`, ni reinicio de contexto). Solo se vio menú y overlay.
  Por eso el log **no muestra** problemas que solo aparecen dentro de un partido.
- **No hay errores Lua en tiempo de ejecución** en el log. Los "PROBLEM" son 11 módulos que no existen,
  2 imágenes del overlay que faltan y 1 .bin de UIColors que falta.
- **Riesgo de crash más alto (archivos):** `real-coach-team.lua` (arma .fpk en disco y sirve
  pares fpk/fpkd de staff), `IntroServer.lua` y `movieintro.lua` (renombran vídeos .usm),
  `POTM_Server.lua` (cambia animaciones .fdc del final del partido).
- **Riesgo de crash más alto (memoria):** `CGP_SB_Addons.lua` y `CGP_SB_Addons_2.lua` (bytecode) escriben
  en la RAM del juego con `memory.write` sin `pcall`. Tienen comprobación de firma antes de escribir.
  Los `FWC_*.lua` escriben aún más en RAM, pero **no están cargados**.
- **Efecto lateral importante:** `IntroServer.lua` **modifica `ctx.tournament_id` y `ctx.match_info`**,
  y todos los módulos que vienen después leen el valor alterado.
- **Candado de ConmeGol:** los 3 módulos CGP verifican que ciertas competiciones contengan ciertos equipos
  (detalle en 1.1). Si Phoenix Mercado mueve esos equipos, los CGP se desactivan solos.

---

## 1. Módulos en bytecode LuaJIT (empiezan con `\x1bLJ`)

Método: `luajit -bl` (listado completo de instrucciones) y un volcado de constantes de tabla con `jit.util.funck`.
El bytecode está **despojado de símbolos**: no tiene nombres de variables ni números de línea, pero sí las constantes.

| Módulo | ¿Cargado? | Eventos registrados | memory.read / write | safe_search | ffi | pcall |
|---|---|---|---|---|---|---|
| CGP_data.lua | sí (pos 10) | ninguno | 0 | 0 | no | – |
| CGP_SB_Addons.lua | sí (pos 11) | overlay_on, key_down, set_teams, livecpk_data_ready, livecpk_read | ~29 / ~28 | 0 | **sí** (`cdef CreateDirectoryA`, `cast`) | 0 |
| CGP_SB_Addons_2.lua | sí (pos 12) | set_teams, livecpk_data_ready, overlay_on, **display_frame** | ~15 / ~14 (al menos 10 son `memory.write` confirmados) | 5 | `cast` | 0 |
| CGP_ScoreboardServer.lua | sí (pos 25) | livecpk_make_key, livecpk_get_filepath, set_teams, overlay_on, key_down, after_set_conditions, livecpk_data_ready | 0 en RAM (los `write` son de archivo: config.ini) | 0 | no | 0 |
| BannerServerHome.lua | sí (pos 20) | livecpk_make_key, livecpk_get_filepath, set_teams, key_down, overlay_on, trophy_rewrite | 0 | 0 | no | 0 |
| BannerServerAway.lua | sí (pos 21) | livecpk_make_key, livecpk_get_filepath, trophy_rewrite | 0 | 0 | no | 0 |
| FWC_Calendario.lua | **NO** | display_frame, key_up | lee y escribe | 1 | no | – |
| FWC_Expansion.lua | **NO** | ninguno (parchea al iniciar) | 1 write | search_process | no | – |
| FWC_Grupos.lua | **NO** | display_frame, livecpk_make_key, livecpk_data_ready, key_up, overlay_on | lee y escribe | 1 | no | – |
| FWC_Llaves.lua | **NO** | display_frame, key_up | lee y escribe | 4 | no | – |

### 1.1 CGP_data.lua
Es solo una tabla de datos que se guarda en `ctx.CGP_data`. Contiene:
- IDs de banderas.
- Nombres cortos en hexadecimal.
- Colores de texto por equipo.
- Desplazamientos (`offsets1/offsets2`) para los marcadores (variantes `GloboA`, `PLNBC`, `LFPpk`, `LPFstat`, `bundes1/2`, `gerSC`…).

No registra eventos. **Sin riesgo propio.**

### 1.2 CGP_SB_Addons.lua ("5.5 EDWARD7777's edition")
- **`livecpk_read`**: modifica en memoria los datos de archivos al leerse. Le interesan estos archivos:
  - `game2dItaly.bin` y `game2dItalyEnter.bin`
  - `licenceTextureItaly.bin` (logos por equipo, función `dispatch_wave_logos`)
  - `common\render\symbol\flag`
  - los flujos `common\script\flow\...json`: TopMenuNew, CmnMatchResult*, CmnPostMatchUserCheck,
    CmnUpdatePostMatch, CmnUpdateSkipMatch, BLMainMenu, MLMainManu, CompeMainMenu, LeagueMainMenu y MatchMenuEnd.
- **`livecpk_data_ready`**: detecta `common\etc\pesdb\competitionentry*.bin`.
- **`key_down`**: Home/End cambia la jornada (Matchday), PageUp/PageDown cambia la competición,
  la tecla 9 activa o desactiva la "simulación de final".
- **Escribe en disco**: crea `content\CGP_SB_Addons\` con `ffi CreateDirectoryA` y escribe `last_selected_competition.txt`.
- **Necesita `luajit.ext.enabled = 1`**. Está activo en el ini, así que funciona.
- **Candado "cgp check"** (el mismo en los 3 módulos CGP, volcado de constantes):
  - la competición **111** debe contener los equipos **2205 y 2218**
  - la competición **128** debe contener los equipos **1261 y 1260**
  - la competición **137** debe contener los equipos **2283 y 2502**
  - la competición **90** debe contener los equipos **2577 y 1489**

  Lee esto de `ctx.common_lib.teams_in_playable_leagues_map`, que sale de CompetitionEntry.bin vía CommonLib.
  Si algún equipo no está, la variable queda en `false` y el módulo apaga sus funciones.
  **Relevante para Phoenix Mercado:** no sacar esos equipos de esas competiciones en el option file.
- Lista interna de 40 o más competiciones, con `comp_id`, `sider_tid`, `fake_tid` (10 o 27) y grupo por país.
- Riesgo: escribe en buffers de archivos recién leídos (`livecpk_read`). Es lo normal para ese evento.
  No usa `pcall`. **Riesgo medio-bajo.**

### 1.3 CGP_SB_Addons_2.lua ("1.2 EDWARD7777's edition")
- Busca firmas en memoria (`safe_search`, `Search_base`) para encontrar estos punteros del marcador en RAM:
  - punteros de Logo
  - punteros de Goal
  - punteros de Lineup (S1–S5)
- Luego **escribe colores en RAM** con `memory.write`:
  - `[lineup] WRITE`, `[goalcolor]`, `[goalcontrast]`
  - texturas de tiempo HALF / FULL / EXTRA TIME
  - `[ScoreUpdate] ... LE bytes written`
- Tiene tablas con desplazamientos fijos (por ejemplo `{57855752, 632, 16, 72, 528}`, es decir 0x372CF08 + cadena).
  Eso lo ata a **una versión concreta del exe**.
- Protecciones que sí tiene:
  - "Logo/Goal signature mismatch" (no escribe si la firma no coincide)
  - "pointer corrected with rematch delta"
  - "Rematch: skipping lineup resolution"
- Usa `display_frame` (se ejecuta en cada fotograma).
- Riesgo: escribir en un puntero obsoleto (tras una revancha o al salir del partido) puede corromper la memoria. **Riesgo medio.**
- Dato relacionado: `Chatty_AutoLineup` registró `Unsupported build: PE image-size mismatch`, o sea que el exe
  no es la compilación que ese módulo espera. Conviene verificar que los offsets fijos de CGP coincidan con este exe.

### 1.4 CGP_ScoreboardServer.lua ("1.9.2, CGP Version")
- Raíz: `content\scoreboard-server` (contiene config.ini, map_competitions.txt y team_colors.ini).
- **`make_key`/`get_filepath`**: reclama todo archivo cuyo nombre (en minúsculas) coincida con
  `common\menu`, `movie\fade` o `common\render\symbol`, siempre que haya un marcador activo.
  Subcarpetas que usa:
  - `contrast_color`, `contrast_names`, `contrast_names_teams`
  - `flag_teams`, `teams_color`, `lineup_color`
  - `tvbroadcast`, `Logos`
- **`livecpk_data_ready`**: reproduce `intro_anthem.mp3` (`audio.new`) cuando ve los .fdc de entrada
  (`ent_007_st..`, `passage_..`) y lo apaga en MatchEnd o MatchDiscontinue.
  - **La primera línea de esta función comprueba el candado CGP.** Si no está verificado, hace
    `log("CGP not verified")` y sale.
  - En el log aparece **810 veces** (una por cada archivo leído), incluso después de cargar
    CompetitionEntry.bin (línea 10316).
  - Como no hubo partido (sin `set_teams`), no se puede afirmar que el candado falle de verdad.
    Pero si sigue apareciendo dentro de un partido, el himno y la lógica del marcador quedan apagados.
    Además, el log se llena (escribe en disco en cada archivo).

### 1.5 BannerServerHome.lua / BannerServerAway.lua
- Lee `content\Banners-Server\map_teams.txt` con formato `teamid,carpeta` y lo guarda en `ctx.banners`.
  - Home lo recarga en cada `set_teams`.
  - La tecla 0 (0x30 = 48) lo recarga a mano.
- **`make_key`**: si `ctx.banners[home_team]` (o `away_team`) existe y no es "None", devuelve
  `"<carpeta>:<archivo>"` **para cualquier archivo**, sin filtrar por tipo.
- **`get_filepath`**: devuelve `Banners-Server\<carpeta>\home\<archivo>` (o `\away\`).
  Si ese archivo no existe, Sider sigue con el siguiente módulo.
- `trophy_rewrite` registrado con una función vacía (`RET0`). **No hace nada.**
- `overlay_on` muestra "CONMEGOL PATCH Prohibida su venta…".
- Efecto: como están en las posiciones 20 y 21, **tapan** a StadiumServer, kserv, Entrance y los demás para cualquier archivo
  que exista en la carpeta de banners de ese equipo. Es cosmético.
- Costo: un `string.format` por cada archivo que pide el juego mientras hay un equipo mapeado.

### 1.6 FWC_*.lua (no cargados)
Son parches de RAM para el Mundial (portugués/español, "GOGOSZ_PATCH"):
- **FWC_Calendario**: busca la "partida 24" y aplica el calendario.
- **FWC_Grupos**: reemplaza el payload de equipos de los grupos al detectar `CompeGroupings.json`.
- **FWC_Llaves**: intercambia slots del cuadro de eliminatorias, partidos 47–57.
- **FWC_Expansion**: intercambio directo de bytes con `search_process` y `memory.write`.

Todos escriben en RAM desde `display_frame`. **Riesgo alto si se activan**, sobre todo con un option file
distinto del que esperan. Hoy no están en sider.ini, así que no afectan.

---

## 2. Módulos que redirigen archivos (livecpk)

Regla de Sider (comprobada en la práctica): para cada archivo que pide el juego, Sider pregunta a los módulos
**en orden de carga**:
1. `livecpk_make_key(ctx, filename)`. Si el módulo devuelve nil o no tiene make_key, **la clave es el propio nombre del archivo**.
2. Después `livecpk_get_filepath(ctx, filename, key)`.

El primer módulo que devuelve una ruta **que existe en disco** gana. Si la ruta no existe, Sider sigue con el siguiente.

La prueba: BannerServer, TournamentCornerFlags, IntroServer y MasterLeague devuelven rutas para *todos*
los archivos y el juego carga igual. Por eso, devolver rutas inexistentes no rompe nada, solo cuesta CPU.

Orden real de `livecpk_get_filepath` (según el log):
real-coach-team(14) → shirtless_celebration(17) → BannerServerHome(20) → BannerServerAway(21) → StadiumServer(22)
→ SleeveBadge(23) → kserv(24) → CGP_ScoreboardServer(25) → IntroServer(29) → RefKitServer(30) → BallServer(31)
→ Entrance_fix(33) → Entrance(34) → POTM_Server(35) → MasterLeague(36) → Commentary-Server(40) → trophy(41)
→ MenuServer(42) → MiniFaceServer(43) → TournamentCornerFlags(54) → movieintro(56).

### 2.1 Ficha por módulo

| Módulo (pos) | Propósito | Qué archivos reclama | Escribe en disco | Trabajo por archivo | Manejo de errores / bugs |
|---|---|---|---|---|---|
| **real-coach-team** (14) | Entrenadores reales (6 ligas). Arma un .fpk "composite" con DT local y visitante | `staff_st###.fpk`, `staff_st###_cl.fpk` → `content\real-coach-team\packages\<pkg>`. **Todo** `staff_st###.fpkd` → `common\staff_st009.fpkd` (fpkd donante). Texturas de cara y pelo de DTs concretos (Moyes, Jaissle, Jakirovic, Cesc…) | **Sí**: al iniciar escribe `packages\_crosscache\.__rct_write_probe.tmp`. En cada `set_teams` con 2 DTs soportados **genera un .fpk nuevo** (cabecera foxfpk, índices, md5) en `_crosscache` | Durante el partido, `make_key` **y** `get_filepath` llaman a `resolve_active_filepath` → `staff_kind` (lower, gsub, match) + `io.open` para comprobar existencia. Son **2 accesos a disco por cada archivo** que pide el juego. Con Everton, además arma una tabla nueva en cada llamada | Sin `pcall`. Los errores de armado se registran ("CROSSMATCH BUILD FAILED") y el módulo se desactiva. Tiene "drain mode" para la salida del partido. **Riesgo de crash medio-alto**: sus propios comentarios dicen que un fpk custom sin su fpkd par no deja entrar al partido. Va antes que todo, así que gana siempre |
| **StadiumServer** (22) | Estadios custom y AddOns por clima | `asset\model\bg\common`, `asset\model\bg\st###`, `common\bg\model\bg\{bill,tv,draw_parameter,...}`, config.xml de vallas, miniatura `common\render\thumbnail\stadium\st###.dds` | config.ini (solo si falta) | Solo cuando `stadium_switched`: lower + varios match, y `check_for_addon_override` (prefijos por clima) | Usa `filename ~= key` como guarda. Log: `map_comp_finals.txt not found` (informativo) |
| **kserv** (24) | Equipaciones | Solo nombres de archivo exactos en `kfile_remap` (uniformes `.ftex`) y `*_realUni.bin` | config de kits (al exportar o con tecla) | Un `string.match` y una búsqueda en tabla. **Barato** | Bug menor: `local is_gk = ktype == "GK"` usa `ktype` sin definir (la variable es `ktyp`), así que `is_gk` siempre es false. `data_ready` de Team.bin quita del mapa los equipos sin licencia |
| **IntroServer** (29) | Vídeo intro por competición o derbi | `livecpk_rewrite`: `movie\intro\*.usm` → `movie\intro\<intro_file>.usm`. `get_filepath`: `content\IntroServer\<intro_folder>\<archivo>` para **todos** los archivos (sin `make_key`, la clave = nombre). Con `intro_folder` nil la ruta queda como `...\nil\...` | no | Calcula todo en `livecpk_data_ready`: una cadena de 60 o más `elseif` por cada archivo. Hay comparaciones redundantes (`tid == 200 or tid == 200 ...` ×10) | **Modifica `ctx`:** `ctx.match_info += 55/17/1/2` y `ctx.tournament_id += 1/2` cuando se lee `CmnUpdateDayEnd.json` (ver 3.4). Si el .usm renombrado no existe en ningún CPK ni livecpk, el juego pide un vídeo inexistente (**posible pantalla negra o cuelgue**) |
| **RefKitServer** (30) | Kits de árbitro por competición | `uniform\texture\#windx11\referee`, `nocloth\#win\referee`, `nocloth\#win\pants` | config | lower + 3 match | Guarda `filename ~= key` |
| **BallServer** (31) | Balón por competición | `asset\model\ball\ball##\`, `common\render\thumbnail\ball\ball_##.dds` | config | lower + 2 match | Guarda correcta |
| **Entrance_fix** (33) | Arreglo de túneles en estadios por defecto (PES 2020) | **Cualquier archivo** si `ctx.stadium` está en su lista → `content\Entrance_fix\{Final|Common}\st###\...` | no | Cadena de if por estadio. Barato | La variable `Entrance_fix` es global del módulo y se arrastra entre llamadas, pero se recalcula siempre |
| **Entrance** (34) | Entradas, trofeos y animaciones por competición o equipo | **Cualquier archivo** cuando hay `tid` → `content\Entrance\<carpeta>\<archivo>`. `trophy_rewrite` devuelve `entry` (16 o 42) | no | **El más pesado**: ~600 líneas de `if/elseif` con listas de 20 a 120 comparaciones `home == x or ...`, en **cada** archivo que pide el juego | Código muerto: `elseif tid == 34 or tid == 35` (Final/Knockout del Mundial) nunca se alcanza porque la rama anterior ya cubre 34/35, así que siempre usa "Group Stage". Lo mismo con `tid == 83`, `tid == 142`, `tid == 45` duplicados. Si `tid` no está en la lista, `entry = nil` pero **`Entrance` conserva el valor del partido anterior** y sigue devolviendo claves con esa carpeta vieja (cosmético) |
| **POTM_Server** (35) | Jugador del partido al final | Archivos de `timeup` (`tu_pickup*`, `tu_full_*`), y en la "lógica normal" **cualquier archivo** que exista en `content\POTM\<competición>\` | no | Activo entre `MatchHalfTime.json` y `MatchEnd.json`/`MatchStatsResult.json`: decenas de `filename:match` + **`io.open` por cada archivo** pedido en el 2º tiempo | **Bugs**: (1) `make_key` devuelve una **tabla** (`{}` o lista), no un string. Sider lo ignora, pero crea basura en cada llamada. (2) En los pasos 1 y 1.5 devuelve una ruta **relativa** del juego (`common\demo\...tu_pickup_combi_a_cut_st000_left.fdc`), no del disco. Normalmente no existe, Sider sigue, pero las banderas internas ya cambiaron. (3) El bloque "activación de emergencia" nunca se ejecuta (está después de `return`). (4) Escribe `_G.potm_done` (global compartido). Cambia animaciones .fdc: **riesgo medio** de cámara o animación inconsistente |
| **MasterLeague** (36) | Selector de "Press Room" o club en ML | `common\demo\fixdemoobj\`, `Asset\model\character\parts\`, `common\demo\prop\` (sensible a mayúsculas) → `content\Master-League-Selector\<club>\...` | config | 3 match por archivo + **un `log()` por cada archivo que coincide** (mucho log) | `get_filepath` sin guarda: devuelve ruta para todos los archivos (que no existe, se ignora) |
| **trophy.lua** (41) | Celebraciones de trofeo para copas sin ceremonia (juce v1.3) | `trophy_rewrite`: 203→42 y 303→42 ("FIFA World Cup"). Después, **cualquier archivo** que exista en `content\trophy-server\FIFA World Cup\` | no | concatenación de string | Sin errores |
| **MenuServer** (42) | Menús por competición | `common\menu\system`, `common\menu\general`, `common\script\flow`, `common\render\symbol\blank`, `\flag` | no | lower + 5 match | `get_common_lib(ctx)` usa `_empty` sin definir: si falta CommonLib → error Lua en `set_teams` de exhibición. Hoy CommonLib está cargado |
| **MiniFaceServer** (43) | Minicaras de selecciones | **Cualquier archivo** si local o visitante es selección (lista de 116 IDs) → `content\miniface_server\MiniFaces\National\` | no | `has_value` lineal ×2 (hasta 232 comparaciones) **por archivo** | `trophy_rewrite` = solo log, devuelve nil |
| **TournamentCornerFlags** (54) | Banderines de córner por torneo | **Cualquier archivo** en cuanto hay `tid` → `content\TournamentCornerFlags\<carpeta>\`. Si la competición no está en la lista, la carpeta es `"nil"` (en LuaJIT `string.format("%s", nil)` = "nil") | no | cadena de if | **Bug**: `trophy_rewrite` hace `log("-- " .. CornerFlag)`. Si `CornerFlag` es nil → **error Lua** "attempt to concatenate a nil value". Sider lo atrapa, así que no hay crash, pero ensucia el log. Lista con errores de escritura (`8149`, `4104` repetido en dos listas) |
| **movieintro** (56) | Clon de IntroServer con otros nombres de .usm (en1, spa1, ita1_a, mls, ensc…) | Igual que IntroServer: `rewrite` de `.usm` y `get_filepath` para todos | no | igual que IntroServer | Registra `overlay_off`, que **no existe** (WARN en el log; su overlay nunca aparece). Sus escrituras en `ctx` son `x = x` (no hacen nada). Mismo riesgo .usm |
| **Chants-Server** (no redirige) | Cánticos por equipo (audio.new) | Solo `livecpk_data_ready`: detecta `common\sound\match\awb\Chant\` y los flujos MatchEnd/Discontinue/Rematch para parar | `chants settings` (volume_home/away) | **Crea una tabla de 6 rutas y hace 6 `string.match` en cada llamada de data_ready** (que se dispara por cada trozo leído). Hace log con emojis | Imagen de ayuda `.\modules\IMAGENS\chantsserver.png` **no existe** |

Fuera de mi alcance, pero dentro de la cadena:
- `shirtless_celebration` (17) tiene `livecpk_rewrite` + `get_filepath` sobre una raíz virtual propia.
- `SleeveBadge` (23) solo reclama `uniform\badge\#windx11\badge####.ftex`.
- `Commentary-Server` (40) nunca devuelve ruta (solo detecta archivos de sonido).

---

## 3. Conflictos (quién gana según el orden de sider.ini)

### 3.1 Entrance_fix.lua (33) vs Entrance.lua (34)
- Los dos devuelven clave para **todo** archivo: Entrance_fix por estadio, Entrance por competición o equipo.
- **Gana Entrance_fix** cuando el archivo existe en `content\Entrance_fix\...`. En estadios por defecto
  (st001, st002, st011, st014, st030, st045…) y en finales (`match_info == 53`), el túnel o la entrada del estadio
  le gana a la entrada específica de la competición.
- **Cosmético**, no hay crash.
- Si se quiere que gane la entrada de la competición, hay que poner Entrance.lua **antes** de Entrance_fix.lua.

### 3.2 trophy_rewrite: BannerServerHome(20) → BannerServerAway(21) → Entrance(34) → trophy(41) → MiniFaceServer(43) → TournamentCornerFlags(54)
- Banner Home/Away: función vacía, devuelven nil.
- **Entrance**: devuelve **16** (casi todas las ligas y copas de clubes) o **42** (Mundial, Euro, Copa América, Asia).
  Así fuerza la escena de trofeo de esos torneos para **todas** las competiciones de su lista.
- **trophy.lua** solo actúa cuando Entrance devuelve nil (competición fuera de su lista).
  En la práctica solo para 203/303 (Mundial 2014 clásico → 42). Entrance no tiene rama para 203/303, así que no chocan.
- **MiniFaceServer**: solo log, devuelve nil.
- **TournamentCornerFlags**: llega aquí si nadie antes cambió el tid. Si el torneo no está en su lista → **error Lua** por nil.
  No hay crash, solo una línea de error.
- **Resumen:** Entrance manda en los trofeos. Si un trofeo sale "equivocado", la causa es Entrance, no trophy.lua.

### 3.3 Archivos de trofeo, demo y animaciones (`common\demo\fixdemo*`, `timeup`)
Compiten Entrance(34), POTM(35), MasterLeague(36, `fixdemoobj`/`demo\prop`), trophy(41) y TournamentCornerFlags(54).
- **Gana el primero que tenga el archivo en su carpeta.** Entrance va antes que POTM, así que si
  `content\Entrance\<comp>\` trae archivos `timeup`, **POTM nunca se ve**.
- MasterLeague le gana a trophy.lua en `fixdemoobj`.
- Riesgo: mezclar archivos .fdc de cámara (una carpeta) con .fpk de animación (otra) puede dar
  animaciones descuadradas o un cuelgue en la ceremonia. **Riesgo medio-bajo.**

### 3.4 IntroServer.lua (29) vs movieintro.lua (56)
- **Los dos** registran `livecpk_rewrite` sobre `movie\intro\*.usm`. **Gana IntroServer** cuando tiene `intro_file`.
  movieintro solo actúa cuando IntroServer no tiene intro para esa competición (por ejemplo tid 80 → "spa2", MLS → "mls").
- Los dos devuelven `content\<raíz>\<intro_folder>\<archivo>` para todo archivo. IntroServer va primero.
- **Efecto lateral real:** IntroServer cambia `ctx.tournament_id`:
  - 86 → 88
  - 87 → 88
  - 89 → 90

  También suma +55/+17/+1/+2 a `ctx.match_info`. Pasa cuando se lee `CmnUpdateDayEnd.json`.

  Todos los módulos que leen `ctx` después ven el valor cambiado hasta que Sider lo vuelva a fijar:
  - Entrance, Entrance_fix, POTM, TournamentCornerFlags
  - Commentary, Scoreboard, MenuServer
  - y cualquier módulo de Phoenix

  Ejemplo: la Supercopa Argentina (86) pasa a verse como 88 (Supercopa Ecuador).
  **Cosmético**, pero confunde cualquier lógica por torneo.
- **Riesgo de crash:** si el .usm destino (por ejemplo `eng_premier_league_1.usm`, `fifa_World_Cup_2026.usm`) no existe en
  `livecpk\Intros` ni en el CPK, el juego intenta abrir un vídeo que no existe. **Riesgo medio**: hay que verificar que existan los .usm.

### 3.5 kserv.lua (24) vs RefKitServer.lua (30) — equipaciones
- **No chocan.** kserv solo atiende nombres exactos de `kfile_remap` (texturas de uniforme de equipos)
  y `*_realUni.bin`. RefKitServer solo atiende rutas de `referee` y `nocloth\#win\pants`.
- Posible solape con cpk.root `Kits` / `slotkits`: los módulos Lua se consultan antes que los cpk.root,
  así que kserv gana. Esto es por diseño de Sider, según lo que sé; no se verificó en este log.

### 3.6 CGP_ScoreboardServer (25) vs MenuServer (42) vs UIColors (`livecpk_read`) vs cpk.root Menu / Menu Light / GraphicsMenus / Logos
- Scoreboard reclama **todo** `common\menu*` y `common\render\symbol*` cuando hay un marcador activo.
  **Le gana a MenuServer** en `common\menu\system`, `common\menu\general` y `render\symbol\flag|blank` si el archivo existe en
  la carpeta del marcador.
- **Cosmético** (banderas o menús de otra fuente).

### 3.7 BannerServerHome/Away (20/21) vs StadiumServer (22) y todo lo posterior
- Cualquier archivo que exista en `content\Banners-Server\<equipo>\home|away\` tapa a StadiumServer (vallas y lonas),
  a kserv, etc. Es intencional.
- **Cosmético.**

### 3.8 real-coach-team (14) vs StadiumServer / cpk.root `Managers`
- RCT reclama **todos** los `staff_st###.fpk/.fpkd` de cualquier estadio y sirve siempre `staff_st009.fpkd` como par.
- Como está en la posición 14, le gana a cualquier staff de StadiumServer y a `livecpk\Managers`.
- **Riesgo de crash** si el .fpk generado no corresponde con el fpkd donante, o con un estadio cuyo staff espera otra estructura.

### 3.9 TournamentCornerFlags (54)
- Va último entre los que reclaman "cualquier archivo". Solo sirve lo que no sirvieron los anteriores.
- Si Entrance (34) trae banderines en su carpeta, **TournamentCornerFlags no se ve**.

---

## 4. Revisión de sider.ini

### 4.1 Los 11 `lua.module` que no existen (log: `PROBLEM: Unable to open file`)
| # | Módulo | Línea del log |
|---|---|---|
| 1 | tunnel.lua | 3934 |
| 2 | Stadium_Banner.lua | 3937 |
| 3 | Stadium_Board.lua | 3940 |
| 4 | Stadium_CornerFlag.lua | 3943 |
| 5 | GoalSongServer.lua | 3946 |
| 6 | Derbys.lua | 3949 |
| 7 | cr7_audio.lua | 3952 |
| 8 | BallBoysServer.lua | 4455 |
| 9 | tournament_anth_tunnel.lua | 9698 |
| 10 | Nets-Server.lua | 9708 (existe `nets.lua`, con otro nombre) |
| 11 | CornerFlag.lua | 9711 |

Son inofensivos: Sider los salta y carga el resto ("Active modules: 49").
Conviene comentarlos con `;` para limpiar el log.

### 4.2 Archivos que existen en `modules\` pero **no** se cargan
- **Comentados** en el ini:
  - FoulScenes.lua
  - Real-Life-Feel-Weather-Manager.lua
  - Real-Time-Match-Manager.lua
  - attack_mentality.lua
  - Anti-Cheat.lua
- **No están** en el ini:
  - FWC_Calendario.lua, FWC_Expansion.lua, FWC_Grupos.lua, FWC_Llaves.lua
  - WeatherConditions.lua, SubBoard.lua, competitions.lua
  - cpkdump.lua, filedump.lua, face-relinker.lua
  - stadswitch.lua, tracer.lua, nets.lua
  - **phoenix.lua** (puente de prueba de Phoenix, solo lectura, overlay)

### 4.3 cpk.root duplicados
- `".\livecpk\Menu Light"` aparece **dos veces**: antes de `Menu` y después de `UIColors`.
  El segundo no aporta nada (Sider usa el primero que encuentra), pero confunde la prioridad.
  Hay que quitar uno, el que no se quiera como prioridad.
- La línea `Referee Faces-Male"` tiene un **tabulador al final**. Sider la leyó bien (está en el log).
- 110 líneas `cpk.root` en el ini, y 110 "Using cpk.root" en el log.

### 4.4 cpk.root que no aparecen en arbol.txt
`arbol.txt` es el árbol **del parche**, no el del PC. Estas carpetas pueden existir en el PC; hay que verificarlo allí.
Sider no avisa si una carpeta cpk.root no existe.

Carpetas que no aparecen (20):
- `boot-root`, `Faces`, `Graphics`, `Kits`
- `Intros` (importante para el riesgo de .usm de 3.4)
- `Scoreboard`, `Announcer`, `MenuAnthems`, `BannerStadium`
- `Leyendas`, `Entrance`, `Comentarios`, `Managers`
- `GraphicsMenus`, `control PS5`
- `UIColors` (el log confirma que falta `livecpk\UIColors\...\topModeSelectDMM.bin`)
- `Musica`, `Worn Turf`, `Turfs`, `Edit Mod`

Los 31 cpk.root de 4 niveles no se pueden verificar con arbol (2 niveles); su carpeta padre sí existe:
- `Gloves-Brands\*`
- `Grip Sock-Brands\*`
- `Shirt-Adidas\*`

Al revés: existen en arbol pero **sin** cpk.root:
- `livecpk\RefColor`
- `livecpk\Servidor de Var` (comentado)
- `PRDX...\000. CUSTOMIZE YOUR TAPE` (normal)

Carpetas `content\` que usan los módulos y **no** vienen en el parche (sí deben estar en el PC, porque los módulos cargaron):
- Banners-Server, Entrance, Entrance_fix, IntroServer, movieintro
- TournamentCornerFlags, miniface_server, trophy-server
- Master-League-Selector, CGP_SB_Addons

### 4.5 Errores "Cannot load texture"
| Imagen | Módulo responsable | Línea de código |
|---|---|---|
| `.\modules\IMAGENS\gervol.png` | **SoundServer.lua** | SoundServer.lua:210 (`HELP_IMG_PATH`) |
| `.\modules\IMAGENS\chantsserver.png` | **Chants-Server.lua** | Chants-Server.lua:698 |

Aparecen en el log justo al pasar el overlay por esos módulos (líneas 10781/10783 y 10815/10817).
La carpeta `modules\IMAGENS\` **no existe**. Es solo cosmético: falta la imagen de ayuda del overlay.

---

## 5. sider.log: problemas agrupados y orden de inicialización

### 5.1 Avisos, problemas y errores por módulo
| Módulo | Línea(s) | Mensaje | Gravedad |
|---|---|---|---|
| goalscreams.lua | 340 | `WARN: trying to register for unknown event: "goal_scored"` | Esa función nunca se ejecuta |
| shirtless_celebration.lua | 440 | `missing profile fields use default body, skin, and hands` | Informativo |
| Chatty_AutoLineup_v0.8.lua | 456 | `Unsupported build: PE image-size mismatch` | **El módulo se autodesactiva.** El exe no es el que espera |
| StadiumServer.lua | 1227 | `INFO: map_comp_finals.txt not found` | Informativo |
| UIColors.lua | 3850–3851 | No puede copiar ni cargar `livecpk\UIColors\(DO_NOT_EDIT\)common\menu\general\topModeSelectDMM.bin` | Ese menú queda sin color custom |
| env.lua | 3896 | `error: function: builtin#19` | **Falso positivo**: env.lua imprime los globales y uno se llama `error` |
| (Sider) 11 módulos | 3934–9711 | `PROBLEM: Unable to open file` | Ver 4.1 |
| Commentary-Server.lua | 3963–4153 | **57** `WARN: Line ignored in map_competitions.txt`. IDs sin idioma: 133–137, 18, 24, 82, 89, 20, 81, 26, 88, 141, 147–151, 142, 22, 117, 28, 91, 115, 155–159, 172, 122, 128, 52, 55, 97, 162, 164, 165, 119, 160, 161, 168, 169, 131, 203, 303. Seis líneas sin ID (`,	#Liga`…). Cuatro líneas mal formadas: **`51ags,ger`**, **`166ags,ger`**, **`167ags,ger`**, **`54ags,ger`** (falta la coma tras el ID) | Esas competiciones usan el comentario por defecto |
| movieintro.lua | 9732 | `WARN: trying to register for unknown event: "overlay_off"` | Su overlay no aparece |
| CGP_ScoreboardServer.lua | 9755–10763 | `CGP not verified` ×**810** | Ver 1.4 |
| SoundServer.lua | 10781, 10815 | `Cannot load texture ... gervol.png` | Cosmético |
| Chants-Server.lua | 10783, 10817 | `Cannot load texture ... chantsserver.png` | Cosmético |

Hay memory patches aplicados sin error (no son de mi alcance, solo como referencia):
- `awaygoals.lua`: 11 bytes en 0x14042dabb…0x1415151d8
- `StartingYearChanger.lua`: año 2020→2026 en 0x1412695f0

### 5.2 Orden de inicialización ("OK: Lua module initialized")
| Pos | Módulo | Línea log |
|---|---|---|
| 9 | lib\CommonLib.lua | 296 |
| 10 | CGP_data.lua | 299 |
| 11 | CGP_SB_Addons.lua | 307 |
| 12 | CGP_SB_Addons_2.lua | 314 |
| 13 | lib\nesalib.lua | 317 |
| 14 | real-coach-team.lua | 326 |
| 15 | sifflet.lua | 337 |
| 16 | goalscreams.lua | 345 |
| 17 | shirtless_celebration.lua | 443 |
| 18 | PES ID.lua | 448 |
| 19 | Chatty_AutoLineup_v0.8.lua | 457 |
| 20 | BannerServerHome.lua | 466 |
| 21 | BannerServerAway.lua | 472 |
| 22 | StadiumServer.lua | 1241 |
| 23 | SleeveBadge-ArmbandServer.lua | 1536 |
| 24 | kserv.lua | 2914 |
| 25 | CGP_ScoreboardServer.lua | 3285 |
| 26 | scoreboard-hexx.lua | 3299 |
| 27 | SoundServer.lua | 3306 |
| 28 | Chants-Server.lua | 3314 |
| 29 | IntroServer.lua | 3321 |
| 30 | RefKitServer.lua | 3523 |
| 31 | BallServer.lua | 3760 |
| 32 | UIColors.lua | 3852 |
| 33 | Entrance_fix.lua | 3857 |
| 34 | Entrance.lua | 3864 |
| 35 | POTM_Server.lua | 3870 |
| 36 | MasterLeague.lua | 3877 |
| 37 | Competition-Server.lua | 3880 |
| 38 | GFX_lod.lua | 3883 |
| 39 | env.lua | 3931 |
| 40 | Commentary-Server.lua | 4164 |
| 41 | trophy.lua | 4174 |
| 42 | MenuServer.lua | 4446 |
| 43 | MiniFaceServer.lua | 4452 |
| 44 | camera.lua | 4475 |
| 45 | BroadCastCam.lua | 6998 |
| 46 | CommonCam.lua | 8011 |
| 47 | DynamicWideCam.lua | 8031 |
| 48 | FanViewCam.lua | 8048 |
| 49 | PenaltyCam.lua | 8067 |
| 50 | ReplayCam.lua | 8087 |
| 51 | StadiumCam.lua | 9671 |
| 52 | VerticalCam.lua | 9691 |
| 53 | matchset.lua | 9695 |
| 54 | TournamentCornerFlags.lua | 9705 |
| 55 | awaygoals.lua | 9729 |
| 56 | movieintro.lua | 9736 |
| 57 | StartingYearChanger.lua | 9747 |

Total "Active modules: 49". Inicialización en 6,657 s.

### 5.3 Mapa de eventos (orden real del log)
- **livecpk_make_key** (16 módulos):
  real-coach-team, BannerServerHome, BannerServerAway, StadiumServer, kserv, CGP_ScoreboardServer, RefKitServer,
  BallServer, Entrance_fix, Entrance, POTM_Server, MasterLeague, trophy, MenuServer, MiniFaceServer, TournamentCornerFlags.
- **livecpk_get_filepath** (21 módulos): la cadena de la sección 2.
- **livecpk_rewrite**: shirtless_celebration → IntroServer → movieintro.
- **livecpk_read**: CommonLib → CGP_SB_Addons → UIColors.
- **livecpk_data_ready**: CGP_SB_Addons, CGP_SB_Addons_2, shirtless_celebration, PES ID, kserv, CGP_ScoreboardServer,
  scoreboard-hexx, Chants-Server, IntroServer, movieintro.
- **trophy_rewrite**: BannerServerHome, BannerServerAway, Entrance, trophy, MiniFaceServer, TournamentCornerFlags.
- **display_frame**: CGP_SB_Addons_2, sifflet.

---

## 6. Recomendaciones (de menor a mayor esfuerzo)
1. Comentar con `;` los 11 `lua.module` que no existen. Quitar el `Menu Light` duplicado.
   Quitar el tabulador de `Referee Faces-Male`.
2. Crear `SiderAddons\modules\IMAGENS\` con `gervol.png` y `chantsserver.png`, o ignorar el aviso.
3. Arreglar `map_competitions.txt` de commentary-server: poner la coma en `51,ags,ger`, `166,ags,ger`, `167,ags,ger`, `54,ags,ger`.
4. Verificar que existan en el PC los .usm que IntroServer y movieintro renombran, y la carpeta `livecpk\Intros`.
   Si no se usan, comentar `movieintro.lua`: duplica IntroServer y su overlay está roto.
5. Si se van a hacer pruebas de Phoenix que dependan de `ctx.tournament_id`, hay que tener en cuenta que **IntroServer lo modifica**.
   Una opción es cargar el módulo propio **antes** de IntroServer y guardar el valor en `set_teams`.
6. Phoenix Mercado: no sacar los equipos 2205/2218 de la competición 111, 1261/1260 de la 128,
   2283/2502 de la 137, ni 2577/1489 de la 90. Si se sacan, el candado CGP apaga el marcador y los extras de ConmeGol.
7. Para buscar crashes dentro del partido, hace falta un sider.log **con un partido jugado**
   (este log no tiene ninguno). Sospechosos en orden:
   1. real-coach-team (fpk generado en `_crosscache`)
   2. CGP_SB_Addons_2 (memory.write en display_frame)
   3. IntroServer y movieintro (.usm)
   4. POTM (fdc del final)
8. No activar los `FWC_*.lua` junto con el option file actual sin probarlos aparte: escriben en RAM con patrones fijos.
