# Phoenix Pantalla 0.1.0 (prueba 2) — paquete aparte, con on/off

Dibuja un panel con texto («Phoenix», acentos, 3 líneas) sobre el menú principal de PES 2021 cuando la zona del juego pasa de 7 (TOP_MENU) a 49 (ONLINE_SETTINGS), es decir, al pulsar «Phoenix» en Ajustes (con el evento 20 en ida y vuelta por TopMenu/TopMenu). **Solo dibuja**: no cambia el juego, no toca el flujo, no lee/bloquea mandos. NO es Phoenix Core: DLL y módulo de Sider propios, sin dependencias (HLSL con d3dcompiler_47.dll y texto con GDI, ambos de Windows).

## Contenido
- `SiderAddons\phoenix_pantalla\phoenix_pantalla.dll` — la DLL (Release x64, /MT).
- `SiderAddons\modules\phoenix_pantalla_loader.lua` — módulo de Sider que la carga y llama a `phx_pantalla_iniciar(carpeta)` una vez.
- `prueba_arnes_fuera_de_PES.png` — captura del arnés (ventana + swapchain propios, sin PES): así se ve.
- Código fuente: rama `claude/pantallas-propias-menus`, carpeta `parche-v01/menu-phoenix-e1/pantalla/` (y worktree `C:\dev\phoenix-pantalla-wt`, rama `claude/core-pantalla`).

## Instalar en Conmegol (SOLO con PES cerrado y con el OK de Frank)
1. Backup: copiar `SiderAddons\sider.ini` a `sider.ini.antes_pantalla_AAAAMMDD`.
2. Copiar la carpeta `SiderAddons\phoenix_pantalla\` y `SiderAddons\modules\phoenix_pantalla_loader.lua` a `D:\Frank\Games_\Conmegol Patch\SiderAddons\`.
3. En `SiderAddons\sider.ini`, junto a las demás líneas `lua.module`, añadir:  `lua.module = "phoenix_pantalla_loader.lua"`
4. `Phoenix-Menu` ya remapea el evento 20 («Phoenix») a ida y vuelta por TopMenu/TopMenu (prueba 1).

## Probar
Abrir Conmegol (con su Sider) → menú principal. Tres formas de ver el panel:
- **Ajustes → «Phoenix» → Enter:** el juego hace un parpadeo, vuelve al menú y aparece el panel.
- **F7** en el menú principal: abre/cierra el panel a mano (para probar el dibujo sin depender de la zona).
- Se cierra con **F8**, o sola a los 20 s, o si la zona deja de ser 7/49 (por ejemplo, al entrar a un partido).
Revisar `SiderAddons\phoenix_pantalla\phoenix_pantalla.log` (línea «Present enganchado», «[pantalla] ABIERTA…») y `cargador.txt`.

## Apagar / deshacer
- Poner `;` delante de la línea `lua.module = "phoenix_pantalla_loader.lua"` en `sider.ini`, **o** crear el archivo vacío `SiderAddons\phoenix_pantalla\PHOENIX_PANTALLA_OFF`.
- Deshacer del todo: borrar la carpeta `phoenix_pantalla\` y el `.lua`, y restaurar el `sider.ini` del backup.

## Probado
- Compila sin errores ni avisos propios (VS 2026, x64, /MT).
- Fuera de PES: carga por ctypes, engancha `Present` (vtable índice 8), dibuja velo + panel + texto con acentos (captura adjunta), y restaura viewport y RTV (arnés `herramientas\pantalla_arnes`).
- NO probado aún dentro de PES.

## Límites y riesgos conocidos (ver DISENO_PANTALLA_PROPIA_CORE.md)
- Puntitos sueltos bajo algunas letras (sangrado entre celdas del atlas GDI): se afina en la siguiente versión.
- Las pulsaciones llegan igualmente al menú de debajo (esta versión no las bloquea).
- Falla en el dibujo o formato de backbuffer no compatible → se apaga sola y lo anota en el log.
- Si la firma del exe no coincide (otro PES), la zona queda desconocida y la pantalla no se activa sola (F7 tampoco).
