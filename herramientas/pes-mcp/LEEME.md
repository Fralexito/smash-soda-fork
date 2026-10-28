# MCP de PES 2021 · «phoenix-pes» (v0.2.0, 28-oct-2026) · historia y errores: GUIA-MCP-PES.md

Deja que Claude mire PES 2021 **en vivo** desde tu PC (menú en el que estás, marcador, minuto, goleadores, equipos, estadio, ajustes, eventos de Sider, memoria del juego, foto de la ventana) y, si lo enciendes, que pulse teclas en el juego. Sirve para investigar y probar mucho más rápido, sin capturas manuales.

Es un paquete **suelto y opcional**: funciona en un PES 2021 limpio con Sider 7.3.x. ConmeGOL es solo el banco de pruebas.

## Qué trae

| Archivo | Para qué |
|---|---|
| `servidor.mjs` | El servidor MCP. Lo arranca Claude solo (Node 22, sin instalar nada más). |
| `ayudante.ps1` | Lo usa el servidor en Windows: ver si PES está abierto, leer memoria, foto de la ventana y teclas. Crea `ayudante.dll` la primera vez (compilado por Windows, sin descargas). |
| `sider/phoenix_sonda.lua` | Módulo de Sider. Escribe `content\phoenix\sonda.json` una vez por segundo con todo lo que Sider sabe del juego. |
| `config.json` | Rutas e interruptores: `acciones` y `escribir_memoria` (apagados), `acciones_con_sala_abierta`. |

## Herramientas que ve Claude (18)

**Lectura (siempre encendidas, no cambian nada):**
- `pes_estado`: foto rápida. Incluye zona (TOP_MENU, EXHIBITION, FREE_TRAINING…), la pantalla del juego (último «flow» que pidió, de `peticiones.log` de Core), marcador, equipos, ajustes, eventos de Sider, versión y modo de Phoenix Core y la sala de Link.
- `pes_partido`: el partido en detalle. Marcador y reloj de Sider y de Core, goleadores, pelota (posición y paso) y los 22 jugadores (en el campo / aparcados), más street y el último resultado. Cada dato dice hace cuántos segundos se midió.
- `pes_eventos`: eventos **en vivo** de Sider, Core y las pantallas, ordenados por hora. Devuelve un `cursor` para pedir solo lo nuevo; con `esperar` (hasta 90 s) se queda escuchando.
- `pes_salud`: ¿está vivo, cargando o congelado? Responde, ventana colgada, CPU, minimizada, edad de la sonda y de Core, cierres y cuelgues del Visor de eventos de Windows (módulo y desplazamiento) y volcados `.dmp`.
- `pes_log`: últimas líneas de `sider`, `core`, `peticiones`, `sonda_partido`, `cargador`, `escenas` o `avisos`, con filtro (`a|b` = cualquiera).
- `pes_captura`: foto **solo** de la ventana de PES; con `veces` saca hasta 6 seguidas. Minimizado no hay foto (el juego no dibuja); al fondo puede salir vieja.
- `pes_memoria_leer`: lee memoria con expresiones como `[exe+0x3704E38]+0xF0` o `phoenix_core.dll+0x1234`. El proceso se abre **sin permiso de escribir**. Dice en qué módulo cae la dirección.
- `pes_memoria_firma`: busca una firma de bytes con comodines (`48 8B 05 ?? ?? ?? ?? C3`) en el código del exe o de una DLL y calcula el destino de instrucciones relativas. Sirve para que todo sea portable a un PES limpio.
- `pes_modulos`: DLL cargadas en PES con base y tamaño (sider.dll, phoenix_core.dll…).
- `pes_escenas`, `pes_hallazgos_buscar`, `pes_herramienta`: como en la 0.1.
- `pes_core` con `estado` o `versiones`: versión, modo y funciones de Core; copias guardadas (`street_estable_*`, `.bak`, respaldos del MCP).

**Acciones (apagadas hasta que pongas `"acciones": true` en `config.json`):**
- `pes_tecla`: pulsa teclas de una lista cerrada (sin X).
- `pes_navegar`: recorre menús con una orden. Ejecuta pasos de teclas, mira la zona y la pantalla tras cada uno y para al llegar al objetivo. Las rutas que funcionan se guardan en `rutas.json` y se repiten por nombre.
- `pes_recargar_modulos`: Shift+R.
- `pes_core` con `instalar`, `revertir`, `funcion`, `apagar` o `encender`: **solo con PES cerrado** y siempre con copia previa en `phoenix_core\_mcp_respaldos\`. `instalar` copia la DLL y el json, y reemplaza los `.lua` de esa versión que ya estén en `modules` (nunca añade módulos nuevos). `revertir` deshace el último cambio. `apagar` crea `PHOENIX_CORE_OFF`.
- Si Link tiene una **sala abierta con gente**, nada de esto actúa.

**Escribir memoria (apagado hasta que pongas `"escribir_memoria": true` en `config.json`, a mano):**
- `pes_memoria_escribir`: escribe hasta 256 bytes, pero solo si lo que hay ahora es igual a `esperado`. Sirve para probar un parche o un valor sin recompilar Core. Cada escritura queda en `escrituras.jsonl`.
- `pes_memoria_deshacer`: vuelve a poner los bytes de antes (la última escritura o todas) en el PES abierto ahora.

**Lo que NO hace nunca:** tocar `sider.ini`, abrir o cerrar PES, descargar nada.

## Instalar (una vez, con PES cerrado)
1. Copiar la carpeta `pes-mcp` a `D:\Frank\IA\CLAUDE\pes-mcp`.
2. Copiar `sider\phoenix_sonda.lua` a `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\` (y a la carpeta del modo de ConmeGOL que copia el switcher, igual que `phoenix_referee.lua`).
3. En `sider.ini` debe quedar `lua.module = "phoenix_sonda.lua"` **antes** de `lua.module = "phoenix_mandos_fase1.lua"`, porque el de mandos 9–16 tiene que ser siempre el último. Desde Link 0.3.51 lo pone el instalador («Módulos del juego»), con PES cerrado. A mano solo si no tienes Link: guarda antes una copia de `sider.ini`.
4. Registrar el MCP en Claude Code (PowerShell):
   `claude mcp add phoenix-pes -s user -- node D:\Frank\IA\CLAUDE\pes-mcp\servidor.mjs`
5. Comprobar sin Claude: `node D:\Frank\IA\CLAUDE\pes-mcp\servidor.mjs --prueba` (lista las herramientas y el estado).
6. Abrir PES con el selector. En `sider.log` debe salir `[sonda] v1.0.1 listo … (con zona)`.

**Desinstalar:** quitar la línea de `sider.ini` y ejecutar `claude mcp remove phoenix-pes -s user`.

## Fases
- **Fase 1 (0.1, 25-oct):** leer todo lo que ya se sabe leer, la foto de la ventana y las teclas.
- **0.2 (28-oct):** partido completo con Core, eventos en vivo, salud y cierres de Windows, firmas, módulos, escribir memoria con deshacer, navegar menús y versiones de Core.
- **Fase 2:** leer el «estado del partido» (SetState, `partido+0xAC8`) sin buscarlo a mano. La sonda ya guarda `stats.ptr` para comprobar si ese puntero lleva al objeto `partido`.
- **Fase 3:** estadísticas del partido (posesión, tiros, faltas) desde `record::Observer`, y las alineaciones.
- **Siguiente:** foto con PES al fondo (función de captura en Phoenix Core, o la captura moderna de Windows), y luego x64dbg y Ghidra (pedirle permiso a Frank antes de descargar nada).

**Actualizar de 0.1 a 0.2:** copiar `servidor.mjs` y `ayudante.ps1` encima (la 0.1 queda en `respaldo_0.1.0`). El ayudante se recompila solo la primera vez. Reiniciar Claude Code para ver las 18 herramientas.
