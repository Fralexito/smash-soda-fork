# PES Street (Phoenix Core): dónde quedamos, para retomarlo en otra cuenta

Escrito el 31-oct-2026 por Cuenta 1, hilo «PES · Modo street (Phoenix Core)». Recoge el último estado que vio este hilo, el 29-oct a las 19:50 UTC. Este hilo no ha tocado la PC después de esa hora.

## 1. Versiones
- **Instalada en la PC:** Core **0.4.99** (DLL MD5 `aadb1319…`) + Lua `phoenix_marcador_free.lua` de `621a6d4` (MD5 `74cb243b…`).
- **Última creada, NO instalada:** **0.4.100** (commit `1da19f4`, MD5 `58184cf8…`). Arregla el cierre de PES al volver al menú (ver §4).
- **Respaldos** en `D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\`:
  - `street_estable_0486`: la estable de 3 h con amigos. **Frank NO la quiere de vuelta** (29-oct).
  - `street_0499_actual` y `*.pre-0497/0498/0499`.
  - Lua: `*.pre-621a`.
- **Rama:** `smash-soda-fork` → `claude/project-thread-p1p1qc` (último commit `d9612fa`). Código en `paquetes/phoenix_core/`. Se compila en el worktree `C:\dev\phoenix-core-wt`.

## 2. Qué funciona (probado por Frank)
- Gol nativo, saque del centro, marcador y reloj nativos en Entrenamiento libre (Día de Partido → Entrenamiento → Entrenamiento libre).
- **Arcos fijos los 90 minutos:** cada equipo mantiene su arco, sin cambio de lado (decisión de Frank).
- Medio tiempo a los 45+2. **Final a los 90** con panel de estadísticas: usa la plantilla «Descanso», así que se parece al medio tiempo, pero es el final. Al cerrarlo, revancha.
- Reloj del 2.º tiempo de 45 a 90 (`reloj_visual_2t`).
- **Revancha:** `goles_reinicio` pone a 0 los 18 contadores nativos. El juego marcó 0-0 (verificado por log). Nuestro marcador Lua ahora se sincroniza cada segundo con el del juego.
- Cuadrado con potencia, muros, red, cancha chica, suplentes fuera, clima soleado y pases rápidos.
- Panel en vivo **PHOENIX STREET**: Espacio abre Sider, ` dos veces para llegar a la página, F10 pasa a la siguiente opción, F11 enciende o apaga, ←/→ cambian valores y F9 cambia entre panel y marcador.

## 3. Qué falla o está abierto
1. **Cierre de PES al volver al menú:** intermitente, 3 dumps (+0x3FD68B, +0x3FE920, +0x902B2). Arreglo en 0.4.100, **sin instalar ni probar**.
2. **El arquero despeja en vez de controlar:**
   - Ya está muerto con el parche C3: GKClear `FUN_142132B40`. En cancha chica siempre despejaba, porque la regla es rival a menos de 12-15 m.
   - Ya está muerto: GKPassLong `FUN_142133120`.
   - `FUN_142133950` siempre corre en modo 0, así que **no es la causa**.
   - La opción `arquero_controla` existe pero no hace nada útil. Déjala apagada.
   - **La fuente real sigue sin encontrarse.** Siguiente paso: líneas `[ARQUERO]`/`[GOLPE]` + REA sobre quién escribe el golpe del portero.
3. **Rebote L2 en los muros de los costados:** el muro no invierte vx. El balón vuelve hacia el propio arco por un toque posterior que aún no está identificado.
4. **`final_nativo`, experimental:** funciona, pero vuelven el árbitro, las faltas y las tarjetas. **Dejarlo APAGADO.**
5. Frank creyó ver goles viejos en el HUD de PES tras la revancha, pero los datos decían 0-0. **Sin confirmar:** que mire el marcador de arriba a la izquierda en la próxima revancha.
6. Arco chico: aplazado por Frank («más adelante»).

## 4. Diagnóstico del cierre (resumen de cap_73 §33)
- Los 3 cierres ocurren **5-7 s después** de `[IA_DATOS] … restaurado` (al salir del partido). Restaurar escribía en registros de IA ya liberados por el juego, y luego el teardown de PES (`FUN_1403fd360` → `FUN_14009027a`) leía basura.
- **0.4.100:** solo restaura si los valores siguen siendo los nuestros, y al salir de la zona se olvidan los punteros EQ.

## 5. Avisos importantes
- **Modo seguro:** si PES se cierra antes de 60 s de abrirlo, el Core arranca con todo apagado (ajustes del partido en gris y equipo parado). Hay que **esperar 1 min en el menú principal**.
- **Config del arquero CONGELADA (0468):** comparar con ella antes de cada instalación. Diferencias conocidas: `arquero_pase_corto=true` y `arquero_controla=false`.
- **hosts:** puede tener aún 5 líneas `127.0.0.1 *.konami.net` de la prueba online. Frank debe ejecutar `RESTAURAR_HOSTS.ps1` como administrador.
- **Antigravity:** su `sync-sider-street.js` (puerto 38888) escribe `phoenix_core.json` en vivo. Hay que coordinarse antes de instalar.
- **Herramienta RE:** Frank pidió **REA para todo** (29-oct; MCP `rea`, Ghidra 12.1.4). GhidraMCP 12.0.4 está listo como respaldo (`D:\Frank\IA\CLAUDE\ghidra_proyectos\COMO_REPETIR.md`, puerto 8765). `.impdata` = Denuvo: **no descifrar**. Frank pedirá permiso a Konami.
- Nunca dejar x64dbg ni vigilantes enganchados. Instalar **solo con PES cerrado** y después de que Frank escriba «cerré».

## 6. Archivos clave
- Repo `docs/pes/`:
  - `cap_73_guia_gol_nativo_entrenamiento.md`: documento largo, §30-33 de este hilo.
  - `cap_62_catalogo_hallazgos_re.md`.
  - `frente_W_portero_despeje.md`.
  - `COMO_REPETIR.md`.
- En la PC:
  - `D:\Frank\IA\CLAUDE\para-proyecto\pes\`: cap_62, cap_73 y `frentes_re\`.
  - Log `SiderAddons\phoenix_core\phoenix_core.log`.
  - Dumps en `%LOCALAPPDATA%\CrashDumps`.
- Integración pendiente:
  - Online Phoenix en la rama `claude/online-phoenix-core` (c8e3d9f), cuando street esté estable.
  - 16 filas de `phoenix_online/docs/cap_62_hallazgos_online_29oct.md` (rama `claude/project-thread-9ciw66`, 4b36014) por fusionar en cap_62.

## 7. Siguiente paso exacto
1. Con PES cerrado y después de «cerré»:
   - instalar **0.4.100** (dll de `1da19f4`);
   - respaldo `*.pre-0500`;
   - comparar el arquero con 0468;
   - borrar `arrancando.flag`.
2. Frank juega un partido completo, termina, entra en la revancha y vuelve al menú. Confirmar que no hay cierre y que la revancha va 0-0 en el HUD de PES.
3. Luego, con REA: la fuente del despeje del arquero, y después el rebote L2.
4. Frank quiere **«estables avanzadas»**: versiones probadas primero con un partido completo con la IA sola antes de dárselas.

## 8. Último estado BUENO conocido (antes de Antigravity)
Frank hizo cambios con Antigravity (Gemini) después del 29-oct y dañaron el street. Este es el último estado bueno hecho con Claude Cuenta 1:

- **Commit bueno:** `d9612fa` (código y Lua) / `aa16dd4` (con este documento), rama `claude/project-thread-p1p1qc`.
- **Core bueno:** 0.4.99 (DLL MD5 `aadb1319…`), más 0.4.100 (`1da19f4`, MD5 `58184cf8…`, arreglo del cierre).
- **Lua bueno:** `phoenix_marcador_free.lua` de `621a6d4` (MD5 `74cb243b58ca19506cf7a5d368682940`).
- **json bueno:** respaldo `street_0499_actual\phoenix_core.json` (MD5 `78f22567…`).
- **Respaldos completos** en `D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\`:
  - `street_0499_actual`: dll, json y Lua del 29-oct, el más reciente bueno.
  - `street_estable_0486`: Frank no la quiere.
  - `*.pre-0499`.
  - Config del arquero: `street_estable_0468`.

**Archivos que pudo tocar Antigravity:**
- `SiderAddons\phoenix_core\phoenix_core.dll` y `phoenix_core.json` (su `sync-sider-street.js`, puerto 38888, escribe el json en vivo).
- `SiderAddons\modules\phoenix_marcador_free.lua` y otros `modules\*.lua`.
- `sider.ini`.
- Archivos de Link en la rama `plan-ux` (`tools/sync-sider-street.js`, `street.js`).

**Cómo comparar y volver:**
1. Con PES cerrado, calcula el MD5 de dll, json y Lua instalados (`Get-FileHash -Algorithm MD5`) y compáralo con los de arriba.
2. Diff del json: `git diff --no-index street_0499_actual\phoenix_core.json phoenix_core.json`. Así ves qué claves cambió Antigravity.
3. Código: `git log --oneline d9612fa..origin/<rama de Antigravity>`. Luego `git diff d9612fa <su commit> -- paquetes/phoenix_core` para ver qué cambió en el Core.
4. Para volver: respalda lo actual como `antigravity_<fecha>`, copia dll, json y Lua de `street_0499_actual` (o compila `1da19f4` para tener la 0.4.100), compara el arquero con 0468 y borra `arrancando.flag`.
5. Apaga el demonio `sync-sider-street.js` antes de instalar, porque si no vuelve a escribir el json.

## 9. 31-oct (Cuenta 2): Street reparado y Core 0.4.101 instalado

**Estado al cerrar (31-oct ~19:35 Lima):** Street funciona y Frank lo confirmó («ahora sí está funcionando perfectamente»). **Instalado: Core 0.4.101** (`ec06849`, DLL MD5 `2b97e270…`, en `build095`), con 0.4.100 incluida.

### 9.1 Qué estaba roto y por qué (verificado)
1. **Pantalla negra al cargar el entrenamiento** (también con el Core apagado). **Causa: `Data\dt18_all.cpk` modificado el 30-oct 18:12**, durante la sesión de Antigravity: 81.920 B, SHA-256 `7477a230…`. Ese CPK contiene `common/match/constant/constant_*.bin` (informe 10). La carga se detenía justo después de `constant_match.bin`, antes de `constant_player/stadium/team` (visto en `phoenix_core\peticiones.log`).
   - **Arreglo:** se restauró el original desde `D:\SP2026\Data\dt18_all.cpk`: 81.305 B, SHA-256 `906cd7a3…`, el mismo que registra el informe 08.
   - El modificado está guardado en `phoenix_core\antigravity_20261031\dt18_all.cpk.modificado`.
   - **Regla:** nunca editar CPK originales de `Data\`. Las pruebas van por livecpk o por el Core.
2. **Json:** se restauró el de `street_0499_actual`. El de Antigravity tenía `pase_rapido_preciso` y `tiro_fuerte` en `true`.
   - La DLL 0.4.99 y el Lua no los tocó Antigravity (hashes iguales al respaldo).
   - `sider.ini` y `modules\*.lua` no cambian desde el 28-29 oct.
   - El flag `PHOENIX_CORE_OFF` que puso Antigravity ya no está.
3. **Cómo se aisló:** con `PHOENIX_CORE_OFF` seguía negro, así que no era el Core. Con REA/Frida, adjunto al PES congelado: dos hilos de carga quedaban esperando tras un `RaiseException` en `PES2021.exe+0x1b37dc6`, y un hilo seguía dibujando (driver NVIDIA). Encajaba con un archivo de datos que no cargaba.

### 9.2 Jugadores que se salían de la cancha (ya pasaba ANTES de Antigravity)
- Medido en `phoenix_core.log` ([JUG], cancha ±15,5 × ±10,5, sin aparcados):
  - 29-oct: 0–3 % de muestras fuera.
  - 30-oct: 0–5 %.
  - 31-oct antes del arreglo: 1 % (jugadores 8 y 12).
- **Causa, con REA/Frida en vivo:** `ActionBasePosition` = `FUN_140956C30` (RVA 0x956C30) da el destino de cada jugador de CAMPO en `param_2` (vec3) y lo devuelve.
  - El 50 % de sus salidas caían fuera de la minicancha. Ejemplo: (5.26, −18.17) para el jugador 8.
  - El portero ya tenía su recorte (`arquero_salida_corta`, `FUN_1409DC040`); los de campo no tenían ninguno.
- **Arreglo, Core 0.4.101: función nueva `jugadores_dentro_free`** (json `"jugadores_dentro_free": true`).
  - Gancho de 18 B de prólogo. Firma `40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 E1 48 81 EC C8 00 00 00 48 C7 45 D7 FE FF FF`, que aparece una sola vez en el exe.
  - Tras el original, solo con TrainingWork, recorta a |x| ≤ L−0,5 y |z| ≤ W−0,5. Registro en el log: `[JUG_DENTRO]`.
  - **Resultado:** 0 fuera en 912 muestras.

### 9.3 Respaldos de hoy (en `SiderAddons\phoenix_core\`)
- `antigravity_20261031\`: json, DLL, Lua, `sider.ini` y el `dt18_all.cpk.modificado`, tal como los dejó Antigravity.
- `street_0499_antes_0101\`: DLL 0.4.99 y json, justo antes de instalar la 0.4.101. Para volver atrás: copiar los dos y quitar `"jugadores_dentro_free"` del json.

### 9.4 Phoenix Link (rama aparte, sin push a `plan-ux`)
- Se quitó **solo** lo de Street que hizo Antigravity, en la rama `plan-ux-sin-street` (worktree `_phoenix-link`, subida como `cuenta2/link-sin-street`):
  - `street.js`;
  - el demonio `tools/sync-sider-street.js` y `INICIAR_SYNC_SIDER.bat`;
  - la barra y el modal de tuning;
  - la «regla de goles».
- El resto de Antigravity se conserva. `PhoenixLink.exe` está compilado desde esa rama.
- Respaldos: ramas `antigravity-respaldo-20261031`, `antigravity-wip-20261031` (lo que estaba sin commit) y `plan-ux-sin-antigravity` (versión descartada). También `x64\Release\ui.antigravity-20261031` y `PhoenixLink.antigravity-20261031.exe`.

### 9.5 Siguiente paso (pedido de Frank, 31-oct)
**1 contra 1 (solo porteros) y 2 contra 2 (portero y un jugador por lado) en el entrenamiento.** El menú no deja bajar de 3 en el lado del usuario.
- Según el frente F, el número del menú se guarda en `TW+0x3C` (local) y `TW+0x40` (rival), con `TW = [raíz+0x400]`. `FUN_140A59CA0` hace participar a quien tenga prioridad ≤ N. El mínimo de 3 solo lo pone el menú.
- **Plan:**
  1. Con REA/Frida, en vivo: leer esos valores y bajarlos a mano (2 y luego 1).
  2. Si funciona, pasarlo al Core como `street_jugadores_local` y `street_jugadores_rival` (0.4.102).
- **Riesgos:** el saque de centro mete a la fuerza a los pateadores (S+0x173C/+0x1740); falta saber si el portero cuenta dentro de N o como extra.

### 9.6 Pendientes que siguen igual
- Despeje del arquero (fuente sin hallar).
- Rebote L2 en los muros laterales.
- Árbitro y faltas en el final nativo.
- Fondo azul sin estadio en Street: Frank lo deja para después.
- Errores conocidos de Sider, que no son la causa de nada de lo anterior:
  - `StadiumServer.lua:705` (el equipo 108 sin entrada `random_num_team`);
  - `TournamentCornerFlags.lua:98`.

### 9.7 Resultados de la prueba 1 contra 1 / 2 contra 2 (31-oct, noche, REA/Frida en vivo)
Método: Frida intercepta `FUN_140A59CA0(tw, idx, límite, S)` y cambia el argumento «límite». No se escribió memoria.
- **El portero cuenta dentro de N.** Límite 2 = portero y un jugador; límite 1 = solo el portero.
- **2 contra 2 (límite 2): funciona.** Frank lo confirmó viendo el partido. Sin Frida, el juego vuelve solo al 3 contra 1 del menú.
- **1 contra 1 (límite 1): funciona a medias.** En juego solo participan i0 e i11, los dos porteros. Pero en el saque de centro el juego mete a la fuerza al jugador de campo i10 como pateador (560 llamadas). El cursor queda en él y, como no participa, lo mandan a la fila de banda. Frank lo vio así: «quiere controlar el balón y salir de la cancha a la vez». Al rato el cursor pasó solo al portero.
- **Falta para el 1 contra 1:** que saque el portero (pateadores en `S+0x173C/+0x1740`) y que el cursor vaya al portero. Es una ampliación de `arquero_propio_cursor`.
- **Siguiente paso:** pasar el límite al Core como `street_jugadores_local` y `street_jugadores_rival` (0 = lo del menú). Con esto el 2 contra 2 queda fijo; el 1 contra 1 sigue experimental hasta resolver el pateador.

### 9.8 Core 0.4.102: interruptor del Street desde Sider (rama `cuenta2/street-0.4.102`, commit b5c050f)
Frank quiere jugar normal sin que se ponga el Street.
- **Cómo se ve:** overlay de Sider → página del marcador (`phoenix_marcador_free.lua`), panel F9 → primera línea **«MODO STREET»**. Se cambia con F11 y se guarda en el json como `"street_activo"`; si falta la clave, está encendido.
- **Cómo funciona:**
  - `hayTrainingWork()` y `trainingWorkEstable()` responden «no» cuando el interruptor está apagado. Todo el Street se apoya en ellas (y en `g_tTrainGlob` / `g_tTrainRed`, que salen de ellas), así que el Entrenamiento queda como el original.
  - El cambio solo se aplica fuera del Entrenamiento (`streetVigilar`, cada 1 s), para no cortar un Street a medias.
- **Arreglo de la minicancha:** antes, el bloque de medidas del campo (52.56 x 34.06) se reescribía en memoria y nunca se devolvía. Ahora `hiloCancha` lo restaura tras 3 s seguidos fuera del Street (`restaurarBloquesCampo`).
- **Gol nativo atado al interruptor:** el parche de `gol_nativo_free` era fijo. Su cueva lee ahora un byte bandera (cueva+72): con el Street apagado hace lo de siempre (`mov edx,7`).
- **Instalado en la PC** con respaldo en `phoenix_core/street_0101_antes_0102/`. El Lua anterior está en `street_0499_antes_0101/phoenix_marcador_free.lua.antes_0102`.
- **Pendiente de probar en el juego:**
  1. Un Street normal (nada cambia).
  2. Apagar el Street en F9 desde el menú → el Entrenamiento es normal.
  3. Un Street y luego un amistoso sin cerrar el PES → cancha normal (`[cancha_chica_free] fuera del Street: N bloque(s)...` en el log).

### 9.9 La cancha dibujada sigue al interruptor (1-nov)
Frank apagó el Street y el Entrenamiento seguía con la cancha chica dibujada, aunque el Core sí estaba apagado (`[STREET] modo Street APAGADO`, `training=0`).
- **Causa:** el césped 30x20 y los arcos a ±15 (cap_73 §11–12, `instalar_cancha_visual.py`) estaban en el AddOn de StadiumServer `España\Spotify Camp Nou\AddOn\Goal Post Colour\Global`. Por eso salían siempre en el Camp Nou: con el Street apagado y también en partidos normales en ese estadio.
- **Arreglo:**
  - Los 16 archivos se movieron a `SiderAddons\content\phoenix\cancha_visual\`.
  - `phoenix_marcador_free.lua` sirve los 11 de dibujo (arcos, red y `pitch_st002.fpk`) solo en FREE_TRAINING con `marcador_free`, `cancha_chica_free` y `street_activo` distinto de false. Se apunta en `sider.log` como `[cancha_visual] sirvo …`.
  - La carpeta original quedó fuera de StadiumServer, en `phoenix_core\camp_nou_global_retirado\`, con copia en `phoenix_core\respaldo_camp_nou_global\`.
  - Para volver atrás: devolver esa carpeta como `...\Goal Post Colour\Global`.
- **Pendiente de probar:** si el PES vuelve a pedir esos archivos en cada entrada al Entrenamiento (cambio en vivo) o los guarda hasta cerrar el juego (entonces hará falta reiniciar tras cambiar el interruptor).

### 9.10 Core 0.4.103: pateador del saque en el 1 contra 1 (1-nov, rama `claude/street-1v1`)
- **Quién elige al pateador (RE con PEStrim, REA no llegó a abrir el exe: el MCP agotaba a los 330 s):** `FUN_140A3CAE0(S, equipo, jugador)` (27 B, RVA 0xA3CAE0) es el único setter: `S+0x173C+equipo*4 = jugador` (válido < 0x19 o 0xFF = ninguno). Lo leen `FUN_140A56000` (quién ejecuta los saques) y `FUN_140A59CA0`; `FUN_140414080` y `FUN_141E5ED10` solo copian. El cursor aún no se ha localizado con precisión; en la prueba seguía al pateador.
- **Cambio:** gancho que reemplaza la función entera (AOB `41 83 F8 19 7C 09 41 81 F8 FF 00 00 00 75 0B 48 63 C2 44 89`, desvío de 15 B). Opción `street_1v1_saque` (json + panel F9, valores ← →): 0 normal; 1 el portero saca del centro; 2 balón en el área del portero del equipo que recibió el gol (S+0x1558) 2,5 s; 3 sin pateador (0xFF, experimental). Solo con `street_activo`, función `street_1v1_portero` y 1 jugador por lado (o `street_1v1_siempre: true` para forzar). Registro `[1V1] pateador equipo N: jugador a -> b`.
- **Minicancha al reencender:** el log de la sesión muestra que tras el OFF/ON solo se sirven los 10 archivos de arcos y nunca `pitch_st002.fpk`; PES no vuelve a pedir el césped si ya cargó ese estadio (caché). Se añadió `[cancha_visual] pedido … NO se sirve` para distinguirlo del caso «zona aún no es FREE_TRAINING».

## 10. 1-nov (Cuenta 1, rama `claude/street-1v1`): Core 0.4.103 a 0.4.105, minicancha, TrainingWork y arcos

Resumen para retomar. Todo está instalado en la PC de Fralex (0.4.105, respaldos `*.pre-0103`, `*.pre-0104`, `*.pre-0105`). Estado probado por Frank: 2v2 perfecto, minicancha y arcos 4×2 funcionan bien. Sin probar: arcos 3×2 y 5×2,2, red nativa, 1v1 completo.

### 10.1 Qué hace cada versión
- **0.4.103 `street_1v1_saque` (0-3):** gancho de `FUN_140A3CAE0(S, equipo, jugador)`, el único setter de pateadores (`S+0x173C+equipo*4`). Con un equipo de 1 jugador el pateador pasa a ser su portero. Modos: 1 = portero saca del centro; 2 = balón en el área del portero que recibió el gol; 3 = balón al centro sin pateador. Hasta la 0.4.103 solo servía con 1 contra 1 exacto.
- **0.4.104 `street_jugadores_local` / `_rival` / `_lado` (0 = no tocar, 1-11):** hilo que escribe la cantidad de jugadores en `TrainingWork` por debajo del mínimo del menú, con log `[JUGADORES]`. Además el gancho del pateador pasó a mirar cada equipo por separado (sirve 2v1 y 1v3), y el log se limitó: `[SUB]`, `[BALL]` y `[JUG]` salen como máximo una vez por segundo y el log rota a `.1` al pasar de 20 MB (antes llegó a 113 MB).
- **0.4.105 `arco_chico_real` + `arco_medida` (0 original, 1 = 4×2, 2 = 3×2, 3 = 5×2,2):** gancho de `FUN_1404AA610` que escribe la boca del arco (`p[2]` = ancho/2, `p[3]` = alto) y el bloque de dimensiones ANTES de que se construya el volumen de la red nativa; log `[ARCO_REAL]` con `p[0..0x12]`. Corrige además `street_jugadores`: escribe las dos copias del registro (ver 10.3).

### 10.2 Minicancha y StadiumServer
- **Causa real de que no saliera:** `StadiumServer.lua` está antes que `phoenix_marcador_free.lua` en `sider.ini` y contesta él mismo cualquier archivo del estadio con su copia (`content\stadium-server\España\Spotify Camp Nou\Asset\...\pitch_st002.fpk`, MD5 91f27c84 = original). Con livecpk gana la primera respuesta, así que el césped chico (MD5 95187e39, en `cancha_visual`) nunca se servía; en `sider.log` solo salían los arcos. Antes funcionaba porque ese archivo era el chico y al moverlo a `cancha_visual` se rompió.
- **Arreglo (Lua):** con Street y `cancha_chica_free` en FREE_TRAINING se copia el césped chico encima del archivo de StadiumServer; se restaura desde `...pitch_st002.fpk.respaldo-20261026-013452` al salir de la zona, apagar el Street o arrancar PES. Log `[cancha_visual] césped CHICO puesto / ORIGINAL restaurado`. La copia ocurre al detectar la zona (menú), antes de cargar el estadio. Solo cubre el Camp Nou (estadio 2).
- Aviso: en el Lua de Sider NO existe `pcall` (daba un error por fotograma en `sincronizarConJuego`); ya se quitó.

### 10.3 TrainingWork: atacante y defensor
- Cadena: `[0x1436F3DC8]` → `+0x3F8` → `+0x10` = `dat`. Dos instancias de 0x103C8 B desde `dat+4`: son COPIAS del mismo registro (al cambiar el rol cambian igual las dos).
- En cada copia, `+0x3C` = cantidad del grupo ATACANTE y `+0x40` = cantidad del grupo DEFENSOR (no «mi equipo/rival»). Con Defensa el menú deja Jugadores usuario = 1 y COM = 3; el mínimo 3 es del grupo atacante, sea quien sea, y el defensor admite 1. Al cambiar Atacante/Defensor se intercambian los valores y cambian flags de rol (`+0x14/+0x18/+0x1C/+0x20`).
- Por eso la 0.4.104 solo bajaba al rival (escribía una cantidad en cada copia y el juego lee una). La 0.4.105 escribe las dos copias; `street_jugadores_lado` (0/1) indica si mi equipo es el grupo atacante (+0x3C) o el defensor (+0x40).
- El código del menú que impone el mínimo del atacante no se localizó. Las comparaciones `[reg+0x40] == 3` probadas (FUN_140821660, 1408262B0, 1408293A0…) son estados de otra cosa.

### 10.4 Arcos parametrizados
- `herr\escalar_arco.py <w> <h> <salida>` (en `D:\Frank\Games_\PRUEBA_CANCHA\trabajo_arcos\herr`) genera los 10 fpk/fpkd de modelo y red y los 5 `.geom` de colisión a partir de los de `cancha_visual`. Transforma cada triple float32 (x, y, z) con 13,5 < |x| < 19,5: z estira o encoge el travesaño y la red y desplaza los postes (conservando su grosor); y escala los palos y desplaza el travesaño. GzsTool para desempaquetar y reempaquetar; mismo tamaño de archivo.
- Carpetas `cancha_visual\arco_4x2`, `arco_3x2`, `arco_5x22` (15 archivos cada una). El Lua sirve la elegida con `arco_medida` (panel F9, «al reentrar»).
- No hay constantes 7.32 / 3.66 / 2.44 sueltas en el exe (barrido de floats = 0). Viven en el bloque vivo de dimensiones `*(raíz+0x2AA8)`: `[2]` = 3,66 y `[3]` = 2,44; el volumen de la red se construye una sola vez por partido (init de BallControl), de ahí el gancho.

### 10.5 Pendientes
1. **Red nativa:** golpe real de la tela con la pelota (Ruta A: hook de `FUN_1404560E0`, `GoalNet_Hit` 0x1EAE180 y `SetState` 0x1EAE330); quitar entonces `red_frena_free`. Riesgo: las anclas de la tela salen de `GoalPostInfo` (`[raíz+0x158]`, aún ±3,66). Entender `p[9]`, `p[0xd]`, `p[0xe]` con el log `[ARCO_REAL]`.
2. **Tamaños de cancha** (frente T): 3v3 30×20, 5v5 40×25, 7v7 50×30, 8v8 60×35. Parametrizar `escalar_arco.py` y `instalar_cancha_visual.py` (césped), y la selección por preset.
3. **Mínimo del menú:** localizar la comparación del grupo atacante contra 3 (probar con PES abierto o x64dbg con punto de escritura en `dat+0x40`, solo con permiso).
4. **Cursor en el 1v1:** el usuario no puede mover a su portero salvo al principio (`[CURSOR_PT]` lo devuelve al portero cada pocos segundos desde exe+0x4403D2). Idea: opción aparte `street_1v1_cursor_portero` sin tocar la config 0468.
5. **Probar** arcos 3×2 y 5×2,2, y el saque 1v1 con los 3 modos.

### 10.6 REA sin snapshot
El MCP agotaba a los 330 s al abrir el exe (cada reintento reinicia Ghidra). Se lanzó `rea analyze` por CLI (`ghidra_proyectos\rea_analizar.bat`, con `REA_GHIDRA_STARTUP_TIMEOUT_MS=3600000`); `rea_analyze.log` sigue en 0 bytes y `rea_recortado.snapshot` no existe todavía. Todo el RE de esta sesión salió de Ghidra sin pantalla sobre PEStrim (`scripts\LlamadoresDecomp.java`, `BuscarFloats.java`, `BuscarOffset.java`, `DecompilarLista.java`).
- Lección: no abrir `phoenix_core.log` con `tail -F` mientras corre PES (bloquea la escritura del Core y se pierde el log).

### 10.7 Actualización de la noche del 1-nov (Core 0.4.106 → 0.4.113) — guía completa en `cap_73` §34
- **Instalado en la PC de Fralex:** 0.4.112 (dll MD5 `3c2bbd3a…`, Lua del panel `422221a3…`; respaldos `*.pre-0106` … `*.pre-0112`). **Compilado y subido, sin instalar:** 0.4.113 (commit 1822c47, dll `c5325d59…`, Lua `4a77f5c9…`, opción `arcos_visuales`).
- **Qué se logró (probado por Frank):** minicancha (el Lua copia el césped chico sobre el archivo de StadiumServer), arcos 4×2 / 3×2 / 5×2,2 con física lógica y modelos propios (`herr\escalar_arco.py`), número de jugadores desde el panel y sincronizado con el menú (celdas +0x3C atacante / +0x40 defensor, dos copias), cohete arreglado (`red_rebote_factor`), arquero que solo se elige con el balón cerca y sin rival cerca (L1+R3 lo fuerza; R3 por DirectInput), tecla 7 = valor normal en el panel.
- **Diagnóstico de la malla (sin resolver):** `SetPost` entrega siempre (0,0,0) incluso en un amistoso normal (lo normal), y `GoalNet_Hit` llega bien con la posición del balón; en el amistoso con Street apagado hay 2 nodos de tela con estado 7, y en el Entrenamiento con Street ON no existe ningún nodo (0 hallazgos con [NODO_RED]). Falta separar «el Entrenamiento no crea la tela» de «nuestros arcos movidos impiden crearla»: probar Street apagado + Entrenamiento libre, y Street ON con `arcos_visuales = 0`.
- **Pendientes:** red nativa en el Entrenamiento (tela), mínimo 1 en ataque en el MENÚ (la comprobación de la interfaz no se ha localizado), cursor del portero en el 1v1 (`street_1v1_cursor_portero`), tamaños de cancha 3v3/5v5/7v7/8v8 (frente T: parametrizar `escalar_arco.py` e `instalar_cancha_visual.py`), saque trabado de la IA (sin evidencia aún), cambio de lados de los mandos 9-16 (rama `claude/mandos-fix`, hilo «PES · Mandos 9 a 16»).
- **Cuidado:** no abrir `phoenix_core.log` con `tail -F` mientras PES corre (bloquea la escritura del Core); instalar siempre con PES cerrado y borrar `arrancando.flag`.
