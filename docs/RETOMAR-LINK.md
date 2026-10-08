# Retomar el frente LINK sin perder nada

(Movido desde CLAUDE.md para no chocar con la rama del Mercado, que también edita CLAUDE.md.)

## Trucos de entorno
- La nube es Linux: NO compila (solo `g++ -fsyntax-only` con cabeceras falsas). Compila Fralex en su PC con `COMPILAR_PHOENIX.bat` (copia versionada en `tools/COMPILAR_PHOENIX.bat`; debe vivir en la RAÍZ de `C:\dev\smash-soda-fork`, no en `tools\`). Compila en `_phoenix-link\` (worktree) y deja `log_phoenix.txt`.
- Prototipo de diseño aprobado (fuente de verdad visual): `docs/prototipo/phoenix-portal-v3.html` + capturas `docs/prototipo/v3-*.png`. Enlace: https://claude.ai/artifact/Co2ryf61vzSc5yYyLS3ZiL
- Archivos con CRLF + BOM: editar con Python conservando `\r\n`, no con `sed`. Nombres de rama con «ñ»: en .bat usar el SHA, no el nombre.
- Tras cada `git push`: `git fetch origin "refs/heads/rediseño-phoenix-portal:refs/remotes/origin/rediseño-phoenix-portal"` para que el aviso de «commits sin subir» se cierre.
- Puente al PC (`mcp__remote-devices__*`): no puede borrar; si queda `.git/index.lock`, moverlo a `.git/stale-index-lock`. Si el puente se desconecta se pierden los permisos de carpeta: revisar `get_device_info`.
- Estado vivo y pendientes siempre al final de `REGISTRO-LINK.md`: ahí se lee primero qué compila, qué está a medias y qué sigue.
