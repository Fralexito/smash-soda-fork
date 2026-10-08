# Retomar el frente LINK sin perder nada

(Movido desde CLAUDE.md para no chocar con la rama del Mercado, que también edita CLAUDE.md.)

## Trucos de entorno
- La nube es Linux: NO compila (solo `g++ -fsyntax-only` con cabeceras falsas). Compila Fralex en su PC con `COMPILAR_PHOENIX.bat` (copia versionada en `tools/COMPILAR_PHOENIX.bat`; debe vivir en la RAÍZ de `C:\dev\smash-soda-fork`, no en `tools\`). Compila en `_phoenix-link\` (worktree) y deja `log_phoenix.txt`.
- Prototipo de diseño aprobado (fuente de verdad visual): `docs/prototipo/phoenix-portal-v3.html` + capturas `docs/prototipo/v3-*.png`. Enlace: https://claude.ai/artifact/Co2ryf61vzSc5yYyLS3ZiL
- Archivos con CRLF + BOM: editar con Python conservando `\r\n`, no con `sed`. Nombres de rama con «ñ»: en .bat usar el SHA, no el nombre.
- Tras cada `git push`: `git fetch origin "refs/heads/rediseño-phoenix-portal:refs/remotes/origin/rediseño-phoenix-portal"` para que el aviso de «commits sin subir» se cierre.
- Puente al PC (`mcp__remote-devices__*`): no puede borrar; si queda `.git/index.lock`, moverlo a `.git/stale-index-lock`. Si el puente se desconecta se pierden los permisos de carpeta: revisar `get_device_info`.
- Estado vivo y pendientes siempre al final de `REGISTRO-LINK.md`: ahí se lee primero qué compila, qué está a medias y qué sigue.

## Interfaz nueva (HTML dentro de la app, 2026-10-08)
- Código: `SmashSoda/phoenix/web/` (C++: anfitrión WebView2, puente JSON, acciones) y `SmashSoda/phoenix/web/ui/` (HTML/CSS/JS sin compilación). Protocolo y lista de acciones: `SmashSoda/phoenix/web/PROTOCOLO.md`.
- Diseño de referencia (Claude Design v4): `docs/prototipo/v4/*.dc.html` + capturas `app-*.png`. Lienzo: https://claude.ai/artifact/Q2kZjfujwFXrYR94QQrZML
- Vista previa sin Windows: `cd SmashSoda/phoenix/web/ui && python3 -m http.server 8765` y abrir `http://localhost:8765/` (usa `js/simulador.js`, mismos nombres de acciones y forma de estado que el C++). Capturas con Playwright (`/opt/pw-browsers/chromium`).
- Si se agrega una acción en C++: agregarla también en `js/simulador.js` y en `PROTOCOLO.md`.
- Pruebas del C++ puro (puente, red, partido, perfiles): `bash pruebas-link/correr.sh` (g++ en Linux).
- Chequeo con cabeceras reales de Windows (sin MSVC): zig 0.13 (`pip install ziglang`) apuntando a mingw-w64; ver historial del chat LINK. Los únicos errores que salen son de código original del autor con extensiones de MSVC (AudioSource.cpp, XInputReader.cpp, AutoMod.cpp), no de Phoenix.
- Respaldo automático: sin WebView2 Runtime (Windows 10 viejo) la app usa la interfaz ImGui de Phoenix y muestra «Instalar componente de Microsoft».
