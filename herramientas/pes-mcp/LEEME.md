# MCP de PES 2021 · «phoenix-pes» (v0.1.0, 25-oct-2026)

Deja que Claude mire PES 2021 **en vivo** desde tu PC (menú en el que estás, marcador, minuto, goleadores, equipos, estadio, ajustes, eventos de Sider, memoria del juego, foto de la ventana) y, si lo enciendes, que pulse teclas en el juego. Sirve para investigar y probar mucho más rápido, sin capturas manuales.

Es un paquete **suelto y opcional**: funciona en un PES 2021 limpio con Sider 7.3.x. ConmeGOL es solo el banco de pruebas.

## Qué trae

| Archivo | Para qué |
|---|---|
| `servidor.mjs` | El servidor MCP. Lo arranca Claude solo (Node 22, sin instalar nada más). |
| `ayudante.ps1` | Lo usa el servidor en Windows: ver si PES está abierto, leer memoria, foto de la ventana y teclas. Crea `ayudante.dll` la primera vez (compilado por Windows, sin descargas). |
| `sider/phoenix_sonda.lua` | Módulo de Sider. Escribe `content\phoenix\sonda.json` una vez por segundo con todo lo que Sider sabe del juego. |
| `config.json` | Rutas y los dos interruptores de acciones (apagados). |

## Herramientas que ve Claude

**Lectura (siempre encendidas, no cambian nada):**
- `pes_estado`: foto del juego ahora. Incluye la zona (TOP_MENU, EXHIBITION, EDIT, UEFA_ML…), la pantalla (menu, equipos, en_juego, descanso…), el marcador, el minuto y los goles con su goleador. También los equipos, el estadio, el clima, los ajustes, los últimos eventos de Sider y la sala de Link.
- `pes_log`: últimas líneas de `sider.log`, con filtro.
- `pes_captura`: foto **solo** de la ventana de PES.
- `pes_memoria_leer`: lee memoria con expresiones tipo `[exe+0x3704E38]+0xF0`. El proceso se abre **sin permiso de escribir**.
- `pes_escenas`: escenas del partido (tarjetas, faltas, cambios…) de `escenas.log`.
- `pes_hallazgos_buscar`: busca en los informes de `PRUEBA_CANCHA\informes` antes de investigar algo de nuevo.
- `pes_herramienta`: lanza las herramientas de `PRUEBA_CANCHA\scripts` que **solo leen**: pelota, estado_partido, estado_log, instantanea, buscar, buscar_valor, volcar info/leer, grabar y vigilar_arbitraje. Las que usan depurador (`observar*`) o congelan el juego (`buscar_fisica`) **no** están.

**Acciones (apagadas hasta que pongas `"acciones": true` en `config.json`):**
- `pes_tecla`: pulsa teclas en la ventana de PES, de una lista cerrada. La X no está, porque abre «Guardar repetición».
- `pes_recargar_modulos`: Shift+R (recarga los Lua de Sider sin cerrar el juego).
- Si Link tiene una **sala abierta con gente**, no pulsa nada, aunque las acciones estén encendidas.

**Lo que NO hace nunca:** escribir en la memoria del juego, tocar `sider.ini`, abrir o cerrar PES.

## Instalar (una vez, con PES cerrado)
1. Copiar la carpeta `pes-mcp` a `D:\Frank\IA\CLAUDE\pes-mcp`.
2. Copiar `sider\phoenix_sonda.lua` a `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\` (y a la carpeta del modo de ConmeGOL que copia el switcher, igual que `phoenix_referee.lua`).
3. En `sider.ini` debe quedar `lua.module = "phoenix_sonda.lua"` **antes** de `lua.module = "phoenix_mandos_fase1.lua"`, porque el de mandos 9–16 tiene que ser siempre el último. Desde Link 0.3.51 lo pone el instalador («Módulos del juego»), con PES cerrado. A mano solo si no tienes Link: guarda antes una copia de `sider.ini`.
4. Registrar el MCP en Claude Code (PowerShell):
   `claude mcp add phoenix-pes -s user -- node D:\Frank\IA\CLAUDE\pes-mcp\servidor.mjs`
5. Comprobar sin Claude: `node D:\Frank\IA\CLAUDE\pes-mcp\servidor.mjs --prueba` (lista las herramientas y el estado).
6. Abrir PES con el selector. En `sider.log` debe salir `[sonda] v1.0 listo … (con zona)`.

**Desinstalar:** quitar la línea de `sider.ini` y ejecutar `claude mcp remove phoenix-pes -s user`.

## Fases
- **Fase 1 (esta):** leer todo lo que ya se sabe leer, la foto de la ventana y las teclas.
- **Fase 2:** leer el «estado del partido» (SetState, `partido+0xAC8`) sin buscarlo a mano. La sonda ya guarda `stats.ptr` para comprobar si ese puntero lleva al objeto `partido`.
- **Fase 3:** estadísticas del partido (posesión, tiros, faltas) desde `record::Observer`, y las alineaciones.
