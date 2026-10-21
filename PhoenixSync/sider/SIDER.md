# Phoenix Sync · Sider: auditoría, riesgos y vínculo en tiempo real

Todo lo que Sync sabe de Sider 7: qué permite, qué riesgos tiene y cómo se une en tiempo real con `phoenix.lua`. El instalador completo está en `base-conocimiento/MOTOR-Y-PARCHES-PES2021.md` (parte «27 · Informe instalador Sider»).

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 3 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Auditoría de Sider en la PC de Fralex (8 oct 2026)](#parte-1-auditoria-de-sider-en-la-pc-de-fralex-8-oct-2026) — origen `PhoenixSync/sider/AUDITORIA-SIDER.md` (f75f677 2026-10-09)
- [Parte 2 · Puente en vivo con Sider — riesgos y cómo se evitan](#parte-2-puente-en-vivo-con-sider-riesgos-y-como-se-evitan) — origen `PhoenixSync/sider/RIESGOS-SIDER.md` (f75f677 2026-10-09)
- [Parte 3 · ¿Se puede cambiar el juego en tiempo real desde la web? — análisis (2026-10-09)](#parte-3-se-puede-cambiar-el-juego-en-tiempo-real-desde-la-web-analisis-2026-10-09) — origen `PhoenixSync/sider/VINCULO-TIEMPO-REAL.md` (f75f677 2026-10-09)

---

## Parte 1 · Auditoría de Sider en la PC de Fralex (8 oct 2026)

> Origen: `PhoenixSync/sider/AUDITORIA-SIDER.md` · último cambio f75f677 2026-10-09 · 7608 bytes. Copia íntegra.

Antes de instalar cualquier módulo Phoenix, se leyó **cada módulo activo** de Sider: qué hace, qué memoria del juego toca, qué archivos
cambia, qué teclas usa y con quién choca. Detalle completo (módulo por módulo, con direcciones y bytes) en `auditoria/`:
`hallazgos_A_shirtless.md`, `hallazgos_B_memoria.md`, `hallazgos_C_camaras.md`, `hallazgos_D_archivos.md` e `inventario-modulos.json`.

**Límite importante:** el `sider.log` revisado es de una sesión **sin partido** (solo menús, cierre limpio). Lo que pasa *durante* un
partido se dedujo leyendo el código, no viéndolo. Falta un `sider.log` de una sesión con partido jugado.

### Cómo está montado

- El **switcher** de Fralex, al elegir «ConmeGOL Patch 26», **copia** `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons` (`sider.ini`,
  `modules`, `content`, `livecpk`, `olmosjr23`) **encima** de `SiderAddons` del juego. Lo que no está en la carpeta del parche se queda.
  - Consecuencia: el `sider.ini` que manda es el de **la carpeta del parche**. Un cambio solo en `SiderAddons` del juego se pierde (así
    se perdió la línea de `phoenix.lua`, que por eso nunca se activó).
  - La carpeta del parche trae 23 archivos de módulos; los otros 63 están solo en la del juego (añadidos a mano o de versiones viejas).
- Sider **7.3.3**. `sider.ini` pide **60 módulos**: **49 cargan**, **11 no existen** (tunnel, Stadium_Banner, Stadium_Board,
  Stadium_CornerFlag, GoalSongServer, Derbys, cr7_audio, BallBoysServer, tournament_anth_tunnel, Nets-Server — el archivo real se llama
  `nets.lua` —, CornerFlag). Sider los salta sin romper nada.
- Cada módulo corre en **su propio entorno** Lua: los nombres globales repetidos (init, home, away…) **no** chocan (comprobado con el
  volcado de `env.lua`). Lo único compartido: la tabla `string`, `math.randomseed` y las declaraciones `ffi.cdef` (VirtualAlloc, compatible).
- Teclas: casi todos usan 0, 6–9, -, +, RePág/AvPág y el stick derecho, pero Sider 7 solo manda las teclas al módulo **elegido en el
  overlay**, así que no chocan (supuesto respaldado por el log).

### Problemas encontrados (ya existían, no los puso Phoenix)

| # | Gravedad | Qué | Por qué importa | Arreglo propuesto |
|---|---|---|---|---|
| 1 | 🔴 Alta | **camera.lua y DynamicWideCam.lua parchean la MISMA instrucción** (0x1408a1fbe). Cada uno suma +0x2C: la cámara Dynamic Wide termina leyendo 0x1425985d0 y DynamicWideCam escribe 0.1 ahí, encima de bytes que eran texto (`6E 64 00 00`) — **comprobado en el log** (líneas 4468-4470 y 8020-8025). | Corrompe datos del juego; cada Shift+R lo empeora. Candidato serio a cierres raros. | Quitar `camera.lua` (lo único propio que aporta es activar/desactivar repeticiones). |
| 2 | 🔴 Alta | **GFX_lod.lua** escribe en una dirección **fija** (0x14297B0C1) pensada para otra versión del exe, **sin comprobar** qué hay ahí. | Escritura a ciegas. | Quitarlo o reescribirlo con búsqueda por patrón. |
| 3 | 🟠 Media-alta | **SoundServer.lua** sigue 4 punteros desde una dirección fija sin comprobar nulos y escribe el volumen cada fotograma con su overlay abierto. | Puede leer/escribir en la nada. | Revisar o quitar. |
| 4 | 🟠 Media | **CGP_SB_Addons_2.lua** (bytecode del parche) escribe colores del marcador en memoria **cada fotograma**, sin `pcall`, con desplazamientos fijos a un exe. | Del propio ConmeGOL; se desactiva si no encuentra la firma. | Dejarlo (es del parche) y vigilarlo en el log. |
| 5 | 🟠 Media | **shirtless_celebration.lua** (añadido a mano): 15 parches de código + 512 KB de memoria ejecutable; valida antes de escribir y se apaga si no coincide, **sin `pcall`**; copia 127 archivos (32,8 MB) en cada arranque y escribe un log línea a línea; redirige 17 modelos sin comprobar que existan. | Posibles tirones; cierre si faltan esos modelos al celebrar. | Comprobar los 17 archivos; apagar su log de depuración. |
| 6 | 🟠 Media | **real-coach-team.lua** fabrica un `.fpk` en disco al empezar cada partido y mira el disco 2 veces por cada archivo que carga el juego. | Lentitud en cargas; si falla el `.fpk`, banquillo raro. | Vigilar. |
| 7 | 🟠 Media | **SleeveBadge**, **Commentary-Server**, **Competition-Server**: punteros sin comprobar / patrón débil / sin validar. Commentary usa el idioma 0x18 (fuera de rango) como respaldo. | Riesgo bajo pero real. | Vigilar en el log con partido. |
| 8 | 🟡 Baja | Duplicados y conflictos de archivos: BroadCastCam y StadiumCam tocan el mismo valor (gana StadiumCam); FanViewCam y camera.lua igual; Entrance_fix gana a Entrance; IntroServer y movieintro renombran el mismo vídeo de intro (si el `.usm` no existe → pantalla negra). | Cosmético, salvo la intro. | Elegir uno de cada par. |
| 9 | 🟡 Baja | Módulos que **no hacen nada**: Chatty_AutoLineup (es para Football Life 2026, se negó a cargar), scoreboard-hexx (solo para un marcador EPL que no se usa), goalscreams (pide Sider 7.3.4+), Anti-Cheat (placebo: inventa campos que Sider no tiene; además está desactivado), env.lua (solo vuelca datos al log), matchset. | Ruido, carga inútil. | Quitar. |
| 10 | 🟡 Baja | `sider.ini`: «Menu Light» dos veces, un tabulador suelto, 11 módulos inexistentes; faltan `modules\IMAGENS\gervol.png` y `chantsserver.png` (SoundServer, Chants-Server); `UIColors` no encuentra `topModeSelectDMM.bin`. | Cosmético. | Limpiar. |
| ⚠️ | Desactivado | **attack_mentality.lua** (comentado) es de **alto riesgo** si se vuelve a activar: reescribe código del juego mientras se ejecuta. | — | No activarlo. |

Otros: `ReShade.log` tiene errores de compilación de algunos shaders (no afectan al juego); `dlss5-feed-crash.dmp` está **vacío** (0 B, 8 oct 22:10): no da información.

### Lo que esto significa para Phoenix

- **Ninguna** de las zonas de memoria que tocan los módulos actuales se cruza con lo que hace `phoenix.lua`: la prueba 1 **solo** usa
  el evento `overlay_on` (dibujar texto) y lee un archivo propio. No escribe memoria, no redirige archivos, no cambia `ctx`, no usa teclas.
  Es la superficie de choque más pequeña posible.
- **Certeza:** nadie puede dar un 100 % matemático con 49 módulos de terceros y un juego cerrado. Lo que sí se puede es: (1) no compartir
  nada con ellos (hecho por diseño), (2) arreglar los choques que ya existen (tabla), (3) probar de forma controlada — partido sin el
  módulo, partido con el módulo, comparar los dos `sider.log` — y (4) que el módulo se apague solo ante cualquier error.
- **Para módulos futuros:** `IntroServer.lua` **cambia `ctx.tournament_id`** (86/87→88, 89→90): un módulo Phoenix que lea la competición
  verá el valor cambiado. Los módulos CGP comprueban que ciertos equipos estén en ciertas competiciones (111, 128, 137, 90): Phoenix
  Mercado mueve **jugadores**, no equipos, así que no les afecta; si algún día mueve equipos entre competiciones, hay que respetarlo.
- **Dónde instalar:** en la carpeta del parche (`ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`), para que el switcher lo lleve.

### Plan propuesto (cada paso con permiso de Fralex)

1. Jugar **un partido** con el Sider actual (sin Phoenix) y guardar ese `sider.log` como referencia.
2. Limpieza con copia de seguridad: quitar `camera.lua`, `GFX_lod.lua` y los módulos que no hacen nada; quitar las 11 líneas de módulos
   inexistentes y el «Menu Light» repetido. Otro partido y comparar logs.
3. Recién entonces, añadir `phoenix.lua` (una línea, en la carpeta del parche) y otro partido de prueba.

---

## Parte 2 · Puente en vivo con Sider — riesgos y cómo se evitan

> Origen: `PhoenixSync/sider/RIESGOS-SIDER.md` · último cambio f75f677 2026-10-09 · 5532 bytes. Copia íntegra.

Este documento acompaña al módulo `phoenix.lua` (prueba 1, solo lectura) y a todo lo que venga después con Sider.
Regla de oro: **cada paso se prueba primero en la PC de Fralex, en partidos offline, y solo después con amigos.**

### Qué se sabe del entorno (revisado el 2026-10-08 en la PC de Fralex)

- 🔎 Sider **7.3.3**; `sider.ini` pide 60 módulos Lua: 49 cargan y 11 no existen. Auditoría completa en `AUDITORIA-SIDER.md`.
- 🔎 El Lua de Sider (LuaJIT 5.1) trae: `io` (leer y escribir archivos), `fs` (`find_files`, `make_dirs`), `memory` (leer/escribir/buscar en la memoria del juego), `ffi`, `zlib`, `match`, `audio`, `os` (solo `date`, `time`, `clock`). **No trae red**: el juego no habla con Internet desde Lua (salvo por `ffi`, que no usaremos para eso).
- 🔎 El overlay de Sider se abre con **Espacio** (`overlay.vkey.toggle = 0x20`), se cambia de módulo con **1** y **º/`**. Solo se dibuja (y solo llama a los módulos) mientras está abierto.
- 🔎 Si un módulo de la lista no existe, Sider lo anota (`PROBLEM: Unable to open file`) y sigue: hoy hay 7 así en `sider.ini` y el juego funciona.
- 🔎 Hay **PESBUL** (`PESBUL_2021_Server_Connector.bat`): servidor comunitario que imita el online de Konami cambiando el archivo `hosts` de Windows. Es decir, el online "de verdad" ya lo da la comunidad; el puente Phoenix tiene que convivir con él.
- 🔎 Otros enganches en el mismo juego: ReShade (`dxgi.dll`), DLSS5 (addons de ReShade), `PES21_Hook`, CreamAPI. Cualquiera puede chocar con algo nuevo; por eso lo nuevo entra de a poco.

### Cómo es el puente

```
Web Phoenix -> Phoenix Link / Phoenix Sync (PC) -> archivo avisos.txt -> phoenix.lua (dentro del juego) -> overlay
```
El juego no se conecta a nada: solo lee un archivo pequeño que la app de PC deja en `SiderAddons\content\phoenix\`.

### Riesgos y medidas

| # | Riesgo | Qué podría pasar | Medida tomada |
|---|---|---|---|
| 1 | Error dentro del módulo | Mensaje de error, o en el peor caso que el juego se cierre | Todo el trabajo va dentro de `pcall`; tras 5 errores seguidos el módulo se apaga solo y lo anota en `sider.log`. Prueba 1 no usa `memory` ni hooks del partido. |
| 2 | Archivo leído a medio escribir | Texto cortado o raro | La app escribe en `avisos.tmp` y luego **reemplaza** `avisos.txt` de una vez (con reintentos si el juego lo está leyendo). |
| 3 | Archivo gigante o con basura | Lentitud o caracteres extraños | Se leen como máximo 4 KB, 14 líneas, 110 bytes por línea; si no es UTF-8 válido no se muestra; se quitan caracteres de control. |
| 4 | Bajón de rendimiento (FPS) | Tirones | Solo trabaja con el overlay abierto y mira el archivo cada 2 s como mucho. Con el overlay cerrado su coste es cero. |
| 5 | Choque de teclas con otros módulos | Una tecla hace dos cosas | La prueba 1 no usa ninguna tecla propia (MasterLeague usa 0, 6–9, RePág/AvPág, Supr; BallServer, F3…). |
| 6b | Instalar en la carpeta equivocada | El switcher pisa el cambio (pasó el 8 oct: la línea se perdió y el módulo nunca cargó) | Instalar en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`, que es la que copia el switcher. |
| 6 | Romper `sider.ini` | Sider no carga ningún mod | Antes de tocarlo se guarda `sider.ini.respaldo-phoenix`; el cambio es **una sola línea al final** de la lista de módulos. Desinstalar = quitar esa línea o restaurar el respaldo. |
| 7 | Una actualización de ConmeGOL pisa `sider.ini` | El módulo deja de cargarse (no rompe nada) | Documentado: hay que volver a añadir la línea. El instalador de Phoenix lo comprobará en el futuro. |
| 8 | Online con PESBUL / partidas entre dos PCs | Desincronización si los dos juegos no tienen lo mismo | La prueba 1 **solo dibuja texto**: no cambia nada del partido, así que no puede desincronizar. Para módulos que escriban memoria: **apagados en online** y probados solo offline hasta demostrar que son seguros. |
| 9 | Jugar con amigos | Que algo se rompa en sus PCs | Con **Phoenix Link (Parsec)** el juego corre **solo en la PC del anfitrión**: los amigos no instalan nada. Es el modo recomendado para las pruebas. |
| 10 | Escribir memoria en sitios equivocados (fases futuras) | El juego se cierra o guarda datos malos | Antes de escribir: localizar por patrón (AOB) **y** comprobar que lo leído tiene sentido (como hacen StartingYearChanger y Chatty_AutoLineup); nunca escribir con el juego guardando; probar cada valor en una partida desechable; guardados de respaldo. |
| 11 | Juego trampa / ventaja injusta | Desconfianza en la liga | Lo que cambie el partido (dinero, plantillas, reglas) lo decide la **web** (firmado con Ed25519, como `/liga/cambios`), nunca el jugador. |
| 12 | Archivos con nombre raro / rutas | No encuentra el archivo | La ruta se arma desde `ctx.sider_dir` (la que Sider da); la app comprueba que exista `sider.ini` antes de escribir. |

### Fases

1. **Prueba 1 (ahora): leer archivo → overlay.** No toca memoria. Objetivo: comprobar que el puente funciona y que el juego sigue estable.
2. **Prueba 2:** Phoenix Link escribe los avisos solo (resultado de la sala, fichajes confirmados por la web).
3. **Prueba 3:** leer (no escribir) datos del partido con `match` / `ctx` (marcador, equipos) y dejarlos en un archivo para que la app los suba a la web.
4. **Prueba 4:** escribir **un** valor conocido en memoria (p. ej. dinero de la Liga Máster), solo offline, con verificación.

Cada prueba se anota en `PhoenixSync/PRUEBAS.md`.

---

## Parte 3 · ¿Se puede cambiar el juego en tiempo real desde la web? — análisis (2026-10-09)

> Origen: `PhoenixSync/sider/VINCULO-TIEMPO-REAL.md` · último cambio f75f677 2026-10-09 · 3719 bytes. Copia íntegra.

### Cómo funciona el switcher de ConmeGOL («PES2021 Start.exe»)
- Programa en Python 3.14 empaquetado con PyInstaller y hecho con Gemini (script `gemini-code-1785843135404.pyw`, interfaz tkinter).
- Opciones: ConmeGOL Patch 26 · Latam (PE, CL, UY B) · Clásicos (No LM).
- Al elegir una opción:
  1. ejecuta `robocopy "ConmeGol Extras\<modo>" "<carpeta del juego>" /E /IS /IT`, que **copia y sobrescribe todo** (CPY.ini, SiderAddons, conmegol_info.txt, download);
  2. escribe el nombre del modo en `version_actual.txt`;
  3. abre `SiderAddons\sider.exe`, espera 2 segundos y abre `PES2021.exe`.
- **No usa internet**: no actualiza nada ni se conecta a ningún sitio.
- Consecuencia: lo que pongamos en la raíz se borra al volver a usar el switcher. Lo nuestro debe vivir dentro de `ConmeGol Extras\<modo>\SiderAddons`, o en un modo propio «Phoenix».

### Lo que permite Sider en este parche (sider.dll, LuaJIT 2.1)
- **Eventos**: `display_frame` (se llama en cada cuadro de imagen), `key_down`, `gamepad_input`, `overlay_on`, `set_teams`, `set_conditions`, `set_match_settings`, `set_match_time`, `livecpk_get_filepath`, `livecpk_data_ready`, `livecpk_rewrite`, `trophy_rewrite` y otros.
- **Librerías**: `io` (leer y escribir archivos), `memory` (leer y escribir la memoria del juego), `ffi` (llamar funciones de Windows).
- **Sin internet**: no trae nada de red.
- **Precedente en la carpeta**: `Real-Time-Match-Manager.lua` (de heazebell) lee un .ini y cambia el clima en vivo por memoria. Ojo: está hecho para FL_2026.exe y Sider 7.4 y está desactivado en sider.ini; su autor dejó en su lista negra `require(ffi)`.
- **Recarga en vivo**: Shift+R recarga los módulos Lua sin cerrar el juego (vkey.reload).

### Arquitectura viable
Web (Supabase) ⇄ **Phoenix Link/Mercado** (programa en la PC, hace todo lo de internet) ⇄ **buzón** (archivos en `content\phoenix\`) ⇄ **módulo Sider `phoenix.lua`**, que revisa el buzón 1 vez por segundo dentro de `display_frame`, sin red y sin `ffi`.

### Qué se puede cambiar y cuándo se ve
| Qué | Cómo | Cuándo se ve | Riesgo |
|---|---|---|---|
| Avisos de la web en pantalla | buzón → overlay | al momento | muy bajo |
| Resultados del partido → web | `match.stats()` → archivo → Link lo sube | al terminar el partido | bajo |
| Rival, estadio, clima, hora y reglas del próximo partido | `set_teams`, `set_conditions`, `set_match_settings` | en el siguiente partido | bajo (los módulos de ConmeGOL ya lo hacen) |
| Caras, uniformes, escudos, botines, cánticos | `livecpk_get_filepath` → archivo nuevo | la próxima vez que el juego cargue esa imagen (menú o partido) | bajo |
| Habilidades, posición, dorsal y altura de un jugador | `livecpk_data_ready` parchea Player.bin mientras el juego lo carga | al **reiniciar** el juego | bajo |
| Lo mismo, pero sin reiniciar | escribir en la tabla de jugadores en memoria | en el menú, antes del partido | medio-alto: hay que mapear la memoria; solo en menús, nunca a mitad de partido |
| Dinero, contratos y fichajes de la Liga Máster | Phoenix Sync edita la partida guardada **con el juego cerrado** | al cargar la partida | bajo (ya probado) |
| Lo mismo en memoria, con el juego abierto | memoria | — | alto: no se hace |

### Reglas de seguridad
- Nada de internet dentro del juego: el programa de la PC hace la red y Sider solo lee archivos pequeños.
- Escritura atómica del buzón (ya hecha en PhoenixAviso.ps1).
- Todo dentro de `pcall`; si hay 5 errores seguidos, el módulo se apaga solo.
- Ninguna escritura en memoria durante un partido.
- Instalar en la carpeta del modo, no en la raíz.

---
