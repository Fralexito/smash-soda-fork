# Guía-bitácora: MCP de PES 2021 «phoenix-pes» (25 y 26-oct-2026)

## 1. Qué se consiguió
Claude ya puede **ver y manejar PES 2021 en vivo** en la PC de Fralex con un servidor MCP propio. Se probó con el juego abierto el 25-oct:
- **Lectura de memoria:** `[exe+0x3704E38]+0xF0` = 7, es decir TOP_MENU.
- **Teclas:** Fralex vio moverse la selección del menú.
- **Foto de la ventana:** muestra la pantalla real (la selección cambia entre «Become a Legend» y «Liga Máster»).
- **Archivos de Phoenix:** se leen `estado.json`, `pantalla.json`, `sala.txt` y `sider.log`.

Pendiente: la sonda de Sider. La v1.0 cargó, pero falló al primer fotograma; la 1.0.1 está corregida y espera a que el instalador de Link la reinstale.

Pedido de Fralex: «¿Es posible crear un MCP para conectar PES 21 con mi página?» → «Ambas cosas» (investigar más rápido y dar más datos a la web) → «Entonces mejor el MCP» (eligió el más completo, que lee y actúa).

## 2. Piezas
| Pieza | Dónde | Qué hace |
|---|---|---|
| `servidor.mjs` | `D:\Frank\IA\CLAUDE\pes-mcp\` | Servidor MCP por stdio (JSON-RPC, una línea por mensaje). Node 22, sin dependencias. 9 herramientas. |
| `ayudante.ps1` | igual | Lo usa el servidor: `proceso`, `leer` (memoria **solo lectura**), `captura` y `tecla`. El C# se compila la primera vez a `ayudante.dll` (con `ayudante.firma` = SHA256 del código; si cambia, se recompila). |
| `config.json` | igual | Rutas e interruptores: `acciones` (teclas y Shift+R) y `acciones_con_sala_abierta`. |
| `sider/phoenix_sonda.lua` | `SiderAddons\modules\` | Escribe `content\phoenix\sonda.json` cada 1 s: ctx, match.stats (con `ptr`), zona del gestor de modos y los últimos 40 eventos de Sider. |
| `PROPUESTA-DATOS-WEB.md` | igual | Qué más datos puede mandar Link a la web (bloques A, B y C). |

Herramientas MCP:
- **Lectura:** `pes_estado`, `pes_log`, `pes_captura`, `pes_memoria_leer`, `pes_escenas`, `pes_hallazgos_buscar`, `pes_herramienta`. Esta última solo lanza las herramientas de `PRUEBA_CANCHA\scripts` que leen: no las de depurador (`observar*`) ni las que congelan el juego (`buscar_fisica`).
- **Acciones:** `pes_tecla` y `pes_recargar_modulos`.

## 3. Candados de seguridad
- La memoria se abre con `PROCESS_VM_READ | QUERY_INFORMATION`: Windows no deja escribir con ese permiso.
- Nunca toca `sider.ini` ni abre o cierra PES.
- Las teclas vienen **apagadas** por defecto. Con una sala de Link abierta (`sala.txt` `abierta=1`, de menos de 30 s) no se pulsa nada.
- Las teclas son de una lista cerrada, sin **X** (en las repeticiones abre «Guardar repetición»).
- Solo se pulsa con PES **al frente**. Si no lo está, se para sin pulsar, porque las teclas irían a otra ventana.
- La sonda no devuelve nada en ningún evento, no escribe memoria y se apaga sola tras 5 errores seguidos.

## 4. Paso a paso para replicarlo (otra PC u otra IA)
1. Copiar `herramientas/pes-mcp` (de este repo) a `D:\Frank\IA\CLAUDE\pes-mcp` y ajustar las rutas de `config.json`.
2. Registrar el MCP **desde PowerShell** (no desde Git Bash):
   `claude mcp add phoenix-pes -s user -- node D:\Frank\IA\CLAUDE\pes-mcp\servidor.mjs`
   Comprobar con `claude mcp list` → `phoenix-pes … ✔ Connected`.
3. Prueba sin Claude: `node servidor.mjs --prueba`. Lista las 9 herramientas y el estado.
4. Sonda: la instala Phoenix Link 0.3.51+ («Módulos del juego»), con PES cerrado. La línea `lua.module = "phoenix_sonda.lua"` va **antes** de `phoenix_mandos_fase1.lua`, porque mandos va siempre el último. En `sider.log` debe salir `[sonda] v1.0.1 listo … (con zona)`.
5. Para encender las teclas, Fralex lo hace a mano en PowerShell (ver el error 4):
   `(Get-Content config.json) -replace '"acciones": false','"acciones": true' | Set-Content config.json`

## 5. Errores e intentos fallidos (en orden)
1. **Git Bash se come las barras invertidas.** `claude mcp add … node D:\Frank\…` quedó registrado como `D:FrankIACLAUDE…` y daba «Failed to connect». Se arregló quitándolo y volviéndolo a añadir con barras normales. Usar siempre PowerShell o `/`.
2. **Leer el puntero durante el arranque.** Mientras PES está en FIRST_SETTINGS, `[exe+0x3704E38]` ya apunta al gestor, pero el modo vale 0. En el menú vale 7. No es un fallo: hay que esperar al menú.
3. **La foto con PES minimizado** salía de 159×27 (solo la barra de título). Ahora el ayudante lo detecta (`IsIconic`) y lo avisa.
4. **El sistema de permisos de la PC bloqueó que Claude encendiera sus propias teclas** («Self-Modification»), aunque Fralex lo aprobó en el chat. Es una protección y no se quitó. Fralex lo hizo con una línea de PowerShell.
5. **Teclas con la ventana minimizada o al fondo:** respondían «ok», pero no llegaban al juego. Arreglo: poner PES al frente (un Alt suelto + `SetForegroundWindow`), esperar 2,5 s si estaba minimizado y comprobar el frente antes de cada pulsación.
6. **La foto congelada:** `PrintWindow` en PES (DX11) devolvía siempre el mismo fotograma (5 capturas con el mismo SHA256, aunque Fralex veía moverse el menú). Arreglo: con PES al frente se copia ese rectángulo de la pantalla (`CopyFromScreen`). `pes_captura` dice el método y avisa si la imagen puede ser vieja.
7. **La sonda 1.0 falló:** `attempt to call global 'rawget' (a nil value)`. El Lua de Sider no trae `rawget` ni `_G`. Se apagó sola a los 5 errores, sin afectar al juego. La 1.0.1 guarda la marca de «ya declarado» en la tabla `ffi`, que se comparte entre recargas, y ya no usa `table.sort`.
8. **Descargas:** cada archivo bajado de GitHub a la PC necesitó el «sí» de Fralex (regla de los 5 límites). El 25-oct dio permiso para «descargar todo lo que debas» durante la prueba.

## 6. Cosas a tener en cuenta
- Otros hilos usan PES: **preguntar al coordinador antes de usarlo** y no pulsar nada durante sus pruebas.
- Remote Control y las herramientas de carpeta no van a la vez: mientras hay una sesión de Remote Control, las carpetas no responden.
- Las teclas quedaron **encendidas** el 25-oct «para un día de prueba». Para apagarlas: `"acciones": false`.
- Siguiente (fase 2): con la sonda viva, comprobar si `stats.ptr` lleva al objeto `partido` (estado en `+0xAC8`). Fase 3: estadísticas en `record::Observer` y alineaciones.
- Código y commits: PR #1 de `smash-soda-fork`, rama `claude/project-thread-80uzld`.

## 7. Primer partido con la sonda 1.0.1 (26-oct, muestra del hilo de Link)
- Funciona: `en_partido: true`, zona `EXHIBITION (8)`, reloj, marcador y eventos `set_teams`, `set_stadium_choice` (65533 = estadio del local), `set_conditions` ×2 y `goal_scored`.
- `goal_scored` trae `player_name_on_shirt`, `player_id`, `home_or_away` y además **`player_offset`** (6216 = 0x1848 en la muestra): probablemente el desplazamiento del jugador dentro de la estructura del equipo. Es una pista para las alineaciones (fase 3).
- `set_conditions` llega **dos veces** seguidas: la primera con verano, día y despejado; la segunda con invierno, noche y lluvia. La segunda son las condiciones finales (otro módulo o el propio juego las cambia).
- Problemas que arregla la **1.0.2**:
  1. `ctx` lo comparten todos los módulos: CGP_data, nesalib, scoreboard_server… El archivo pesaba unos 20 KB y traía textos en latin-1 que rompían el UTF-8. Ahora solo se guardan los campos de Sider, más `marcador_activo` y la lista de nombres de los otros módulos (`otros`).
  2. `stats.ptr` salía como texto («userdata: 0x…»). Ahora va también `ptr_hex` (número con ffi), para leerlo desde el MCP y comprobar si es el objeto `partido` (fase 2).

## 8. Versión 0.2.0 (28-oct): el MCP «que cubra todo lo posible»
Pedido de Frank: «SIEMPRE USA EL MCP para complementar todo lo que hacemos… es posible hacer un MCP mucho más completo que cubra absolutamente todo lo posible».

**Qué se añadió (de 9 a 18 herramientas):**
| Herramienta | De dónde saca el dato |
|---|---|
| `pes_partido` | sonda (marcador, reloj, goleadores), `marcador_free.json`, y las líneas `[BALL]`, `[JUG]`, `[TIEMPOS]`, `[RELOJ]`, `[VIGILANTE]` de `phoenix_core.log` |
| `pes_eventos` | eventos de la sonda, etiquetas de Core (`MARCADOR`, `CELEBRACION`, `SAQUE`, `DESCANSO`, `SHOOT`, `MURO`, `RED`, errores; con `detalle` también `PASO`, `PORTERO`…), `street_estado.json` y los «flows» de `peticiones.log` (= pantalla que abre el juego) |
| `pes_salud` | proceso (Responding, `IsHungAppWindow`, CPU en 1 s), edad de sonda y Core, `[VIGILANTE]`, Visor de eventos de Windows (ids 1000/1001/1002) y `.dmp` |
| `pes_memoria_firma` | busca la firma en las secciones de código leyendo la cabecera PE en memoria (trozos de 4 MB; si una página no se deja leer, en bloques de 64 KB) |
| `pes_modulos` | `Process.Modules` |
| `pes_memoria_escribir` / `pes_memoria_deshacer` | `WriteProcessMemory` + `VirtualProtectEx` + `FlushInstructionCache`; solo si los bytes actuales = `esperado`; registro `escrituras.jsonl` |
| `pes_navegar` | teclas paso a paso + zona (sonda o memoria) + flow actual; rutas en `rutas.json` |
| `pes_core` | `phoenix_core_estado.json`, carpetas `street_*`, respaldos en `_mcp_respaldos`, `PHOENIX_CORE_OFF` |
También: `pes_log` lee 7 registros, `pes_captura` saca series y `pes_memoria_leer` acepta `modulo.dll+0x…`.

**Candados nuevos:** escribir memoria tiene su propio interruptor (`escribir_memoria`, apagado; lo enciende Frank a mano). Las acciones de Core exigen PES cerrado y guardan copia antes. Nada actúa con una sala de Link abierta.

**Prueba (28-oct, PC de Frank, solo lectura, con PES abierto en Entrenamiento libre por el hilo de street):**
- `--prueba`: 18 herramientas y el ayudante recompilado sin errores. `pes_estado` y `pes_partido` correctos (3-3, equipos 108/109).
- `salud`: minimizada, no colgada, 136 hilos, CPU 4,9 s por segundo.
- `fallos`: 6 cierres de PES (0xc0000005) el 28-oct. El de las 14:38 fue en `phoenix_core.dll+0x1cfc4`; los otros en `PES2021.exe` (+0x1b5d052 dos veces, +0xb7aaa, +0x4b712b) y en `sider.dll` 7.3.4. Se avisó al hilo de Core.
- `modulos`: 128 módulos; `phoenix_core.dll` cargada.
- `leer [exe+0x3704E38]+0xF0` = 0x34 (52 = FREE_TRAINING). Correcto.
- `firma "48 8B 05 C1 A9 26 02 C3"`: 1 coincidencia en exe+0x149A470, con destino exe+0x3704E38 (el gestor de modos). 3,1 s para recorrer el código. Con la firma genérica `48 8B 05 ?? ?? ?? ?? C3` hay muchas coincidencias: hace falta una firma más larga o subir `max`.
- No se probaron (a propósito, porque otro hilo usaba PES): teclas, navegar, escribir memoria, ni la foto (PES estaba minimizado).

**Foto con PES minimizado:** imposible con cualquier herramienta, porque Windows no dibuja una ventana minimizada y el juego deja de pintar. Con PES al fondo sí se puede de dos formas: (1) una función de Phoenix Core que copie la imagen en el gancho DX11 `Present`, propuesta al hilo de Core; (2) la captura moderna de Windows (Windows Graphics Capture), que es el plan B del MCP.

**Errores de esta ronda:**
1. La copia de `servidor.mjs` en la PC tenía un arreglo local (los `eventos` de la sonda a veces llegan como objeto, no como lista). Se integró en la 0.2 con `comoLista`.
2. La firma genérica con `max` 5 no llega a exe+0x149A470 (hay muchas antes). Lección: usar los bytes exactos o una firma con más contexto.
