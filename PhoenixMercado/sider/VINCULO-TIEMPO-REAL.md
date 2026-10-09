# ¿Se puede cambiar el juego en tiempo real desde la web? — análisis (2026-10-09)

## Cómo funciona el switcher de ConmeGOL («PES2021 Start.exe»)
- Programa en Python 3.14 empaquetado con PyInstaller y hecho con Gemini (script `gemini-code-1785843135404.pyw`, interfaz tkinter).
- Opciones: ConmeGOL Patch 26 · Latam (PE, CL, UY B) · Clásicos (No LM).
- Al elegir una opción:
  1. ejecuta `robocopy "ConmeGol Extras\<modo>" "<carpeta del juego>" /E /IS /IT`, que **copia y sobrescribe todo** (CPY.ini, SiderAddons, conmegol_info.txt, download);
  2. escribe el nombre del modo en `version_actual.txt`;
  3. abre `SiderAddons\sider.exe`, espera 2 segundos y abre `PES2021.exe`.
- **No usa internet**: no actualiza nada ni se conecta a ningún sitio.
- Consecuencia: lo que pongamos en la raíz se borra al volver a usar el switcher. Lo nuestro debe vivir dentro de `ConmeGol Extras\<modo>\SiderAddons`, o en un modo propio «Phoenix».

## Lo que permite Sider en este parche (sider.dll, LuaJIT 2.1)
- **Eventos**: `display_frame` (se llama en cada cuadro de imagen), `key_down`, `gamepad_input`, `overlay_on`, `set_teams`, `set_conditions`, `set_match_settings`, `set_match_time`, `livecpk_get_filepath`, `livecpk_data_ready`, `livecpk_rewrite`, `trophy_rewrite` y otros.
- **Librerías**: `io` (leer y escribir archivos), `memory` (leer y escribir la memoria del juego), `ffi` (llamar funciones de Windows).
- **Sin internet**: no trae nada de red.
- **Precedente en la carpeta**: `Real-Time-Match-Manager.lua` (de heazebell) lee un .ini y cambia el clima en vivo por memoria. Ojo: está hecho para FL_2026.exe y Sider 7.4 y está desactivado en sider.ini; su autor dejó en su lista negra `require(ffi)`.
- **Recarga en vivo**: Shift+R recarga los módulos Lua sin cerrar el juego (vkey.reload).

## Arquitectura viable
Web (Supabase) ⇄ **Phoenix Link/Mercado** (programa en la PC, hace todo lo de internet) ⇄ **buzón** (archivos en `content\phoenix\`) ⇄ **módulo Sider `phoenix.lua`**, que revisa el buzón 1 vez por segundo dentro de `display_frame`, sin red y sin `ffi`.

## Qué se puede cambiar y cuándo se ve
| Qué | Cómo | Cuándo se ve | Riesgo |
|---|---|---|---|
| Avisos de la web en pantalla | buzón → overlay | al momento | muy bajo |
| Resultados del partido → web | `match.stats()` → archivo → Link lo sube | al terminar el partido | bajo |
| Rival, estadio, clima, hora y reglas del próximo partido | `set_teams`, `set_conditions`, `set_match_settings` | en el siguiente partido | bajo (los módulos de ConmeGOL ya lo hacen) |
| Caras, uniformes, escudos, botines, cánticos | `livecpk_get_filepath` → archivo nuevo | la próxima vez que el juego cargue esa imagen (menú o partido) | bajo |
| Habilidades, posición, dorsal y altura de un jugador | `livecpk_data_ready` parchea Player.bin mientras el juego lo carga | al **reiniciar** el juego | bajo |
| Lo mismo, pero sin reiniciar | escribir en la tabla de jugadores en memoria | en el menú, antes del partido | medio-alto: hay que mapear la memoria; solo en menús, nunca a mitad de partido |
| Dinero, contratos y fichajes de la Liga Máster | Phoenix Mercado edita la partida guardada **con el juego cerrado** | al cargar la partida | bajo (ya probado) |
| Lo mismo en memoria, con el juego abierto | memoria | — | alto: no se hace |

## Reglas de seguridad
- Nada de internet dentro del juego: el programa de la PC hace la red y Sider solo lee archivos pequeños.
- Escritura atómica del buzón (ya hecha en PhoenixAviso.ps1).
- Todo dentro de `pcall`; si hay 5 errores seguidos, el módulo se apaga solo.
- Ninguna escritura en memoria durante un partido.
- Instalar en la carpeta del modo, no en la raíz.
