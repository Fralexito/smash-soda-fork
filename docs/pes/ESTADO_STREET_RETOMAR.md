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
