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
