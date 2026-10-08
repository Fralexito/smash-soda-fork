# Coordinación entre chats

Dos trabajos independientes dentro del mismo Phoenix Link:

| Trabajo | Rama | Carpeta | Regla |
|---|---|---|---|
| Rediseño de Phoenix Link (interfaz, nombre, logo) | `rediseño-phoenix-portal` | `SmashSoda/` | No tocar `PhoenixMercado/` |
| Phoenix Mercado (módulo aparte) | `mercado-fase0` | `PhoenixMercado/` | No añadir a la build de `SmashSoda/`; avisar antes de tocar `SmashSoda/` |

## Carpeta compartida en el PC de Fralex (importante)
`C:\dev\smash-soda-fork` tiene UNA sola rama activa y hoy es `mercado-fase0`. Phoenix Link NO compila ahí: `COMPILAR_PHOENIX.bat` crea una copia de trabajo aparte (`git worktree`) en `C:\dev\smash-soda-fork\_phoenix-link` con su propia build. Regla: nadie hace `git checkout` de otra rama en la carpeta principal por el otro chat, y nadie toca `_phoenix-link`. El Mercado se compila con su `build-mercado\`.

## Contratos compartidos (si cambian, avisar al otro chat)
- Token: `%APPDATA%\Trybuchet\Smash Soda\phoenix-token.dat`, cifrado DPAPI (`CryptProtectData`), sin entropía adicional. Mercado lo lee en solo lectura. Sin cambios de ubicación, nombre ni cifrado.
- Carpeta de ajustes `%APPDATA%\Trybuchet\Smash Soda`: sin cambios.
- Agente HTTP `PhoenixSoda/1.0` (`link/Http.cpp`): sin cambios.

## Registro de cambios que afectan al otro chat
- 2026-10-07 · Renombre a Phoenix Link: ejecutable `PhoenixLink.exe` (antes `PhoenixSoda.exe`), `OUTPUT_NAME` en `SmashSoda/CMakeLists.txt`. La etiqueta de texto del token pasó de "PhoenixSoda" a "PhoenixLink"; no afecta a la lectura.
- 2026-10-07 · `core/ProveedorSala.h` (interfaz interna de Phoenix Link) gana métodos de moderación: `esMod`, `esVip`, `alternarMod`, `alternarVip`, `banear`, `tecladoPermitido`, `ratonPermitido`, `permitirTeclado`, `permitirRaton`. Solo afecta si Mercado implementa o usa `ProveedorSala` (hoy no lo hace). La pantalla Gente se rediseñó (lista + ficha).
- 2026-10-08 · Interfaz nueva HTML (WebView2) en Phoenix Link. Junto al exe ahora van la carpeta `ui\` y `WebView2Loader.dll` (los copia el post-build de CMake). Archivos nuevos en `%APPDATA%\Trybuchet\Smash Soda\`: `phoenix-partidos.json`, `phoenix-perfiles-sala.json`, `phoenix-eventos.json`; perfil del navegador interno en `%LOCALAPPDATA%\PhoenixLink\WebView2`. `PhoenixLink` (link/) ahora también manda presencia, eventos (cola persistente), pide perfiles/cartas y amigos, y expone `versionLiga()`. **El token NO cambió** (mismo nombre, ubicación y DPAPI). Mercado podría entrar más adelante como una pestaña de esta interfaz (pedirlo al chat LINK).
- 2026-10-08 · Build: `COMPILAR_PHOENIX.bat` (no versionado) ahora compila en `_phoenix-link\` (worktree desacoplado de la rama activa de la carpeta principal). Salida: `_phoenix-link\x64\Release\PhoenixLink.exe`. Ya no mata `MSBuild`/`cl`/`link`, para no cortar compilaciones del Mercado.

## Dos cuentas de Claude (A y B) — protocolo obligatorio
Fralex usa 2 cuentas; cuando una se queda sin tokens sigue la otra. **La memoria es el repo, no el chat.**
| Frente | Rama | Carpeta | Bitácora |
|---|---|---|---|
| LINK (Phoenix Link, interfaz, overlay) | `rediseño-phoenix-portal` | `SmashSoda/` | `REGISTRO-LINK.md` (raíz, en esa rama) |
| MERCADO (Phoenix Mercado) | `mercado-fase0` | `PhoenixMercado/` | `PhoenixMercado/REGISTRO.md` (en esa rama) |
| WEB (página + Supabase) | repo `Fralexito/phoenixevolution`, `borrador` | — | `REGISTRO.md` de ese repo |

1. Al empezar: `git fetch` + `git pull --rebase` de TU rama y leer el final de tu bitácora y de este archivo.
2. Una línea por cambio, empujada enseguida (la bitácora solo se añade, nunca se edita ni se borra):
   `AAAA-MM-DD HH:MM (Lima) | Cuenta A/B | LINK/MERCADO | Qué cambió | Archivos/commit | HECHO / A MEDIAS / PENDIENTE | Siguiente paso`
   Si algo queda a medias: punto exacto y qué falta. Antes de quedarte sin tokens: commit + push + línea.
3. Si un cambio afecta al otro frente: una línea en la bitácora del otro frente (en su rama) además de la sección «Registro de cambios que afectan al otro chat» de este archivo.
4. Web y base de datos: NO se tocan desde aquí. Se escribe un prompt para el chat WEB y se anota como `PENDIENTE-WEB`. Contratos: `claude/contrato-v1.md` (Link) y `claude/mercado-api.md` (Mercado) en el proyecto de Claude; endpoints publicados no cambian, solo ganan campos opcionales.
5. Reglas de Fralex: «si algo se rompe, mejor no lo hagas»; compilar sin errores antes de cada commit; Windows 10 y 11; español; respuestas cortas; nunca pedir secretos; nada a `master` sin su permiso.
6. Protocolo completo y prompts de arranque: `claude/PROTOCOLO-MULTICUENTA.md` (proyecto) o `docs/PROTOCOLO-MULTICUENTA.md` del repo web.
