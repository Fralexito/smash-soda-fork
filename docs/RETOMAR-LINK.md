# Retomar el frente LINK sin perder nada

(Movido desde CLAUDE.md para no chocar con la rama del Mercado, que también edita CLAUDE.md.)

## Documentos de Link: uno por tema (desde el 21-oct-2026)

Por pedido de Fralex («cada documento por tema y que tenga absolutamente completo»), los 50 documentos sueltos de `docs/` se juntaron **completos** en 6. Los originales siguen intactos en `docs/_archivo/`. **Lo nuevo de cada tema se añade al documento de su tema**, como una parte más: qué se logró, paso a paso, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir. No se crean documentos sueltos.

| Tema | Documento | Junta (nombre viejo) |
|---|---|---|
| Mandos | `docs/MANDOS.md` | ORDEN-MANDOS-PES, TU-MANDO, MAPEO-MANDOS, MEDIDOR-RETRASO, INVESTIGACION-CURSOR-MANDO, EQUIPOS-A-B, DETECCION-LADOS, INVESTIGACION-MANDOS-OCULTOS-Y-COLOR, INVESTIGACION-DATOS-PERSONALES-32, PERFILES-DE-AMIGOS, MODO-CEDIDO, MAS-DE-8-MANDOS |
| Campo 2D y árbitro | `docs/CAMPO-2D-Y-ARBITRO.md` | PHOENIX-ESTADIO-ARBITRO, CAMPO-2D-INVESTIGACION, CAMPO-2D, CAMPO-2D-REPLICAR |
| Fichajes y Sync | `docs/FICHAJES-Y-SYNC.md` | PLAN-PAQUETE-LUAS-PHOENIX, PREPARAR-JUEGO-AMIGO, FICHAJES-COMO-CAMBIOS, SYNC-ESCENARIOS, PARCHE-CONMEGOL-27, TRASPASOS-EN-VIVO, CUENTA-Y-SYNC-SENCILLO, GUIA-COMPLETA-FICHAJES-EN-VIVO |
| Interfaz | `docs/INTERFAZ.md` | REDISEÑO-UI, AVISO-ENTRADA, OVERLAY-PREMIUM, PLAN-UX-ESTADO, GUIA-MENU-NUEVO, PASADA-VISUAL, VOLVER-A-0.3.20 |
| Betas e instalación | `docs/BETAS-E-INSTALACION.md` | BETA-1-NOTAS, BETA-1-GUIA-AMIGO, BETA-2-NOTAS, BETA-PRIVADA, BETA-2-GUIA-AMIGO, PRUEBA-ONLINE-CON-AMIGO |
| Pedidos a la web | `docs/PEDIDOS-A-LA-WEB.md` | los 10 PROMPT-WEB-*, RANKING-1V1, SISTEMA-JUGADORES, CONTRATO-WEB-PES, TRASPASO-17-OCT-WEB-PES |

Las citas del código y de los LEEME se cambiaron a la forma `docs/MANDOS.md («DETECCION-LADOS»)`: busca esa parte en el índice del documento. Las citas antiguas de `REGISTRO-LINK.md` se dejaron como estaban, porque es historia; ese nombre viejo está en `docs/_archivo/`.

## Trucos de entorno
- La nube es Linux: NO compila (solo `g++ -fsyntax-only` con cabeceras falsas). En la PC de Fralex se compila con `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release --parallel`. **Nunca con `COMPILAR_PHOENIX.bat`** (21-oct-2026): hace `git checkout --force` y borra lo que no tiene commit. La copia en `tools/` queda solo como historia.
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
