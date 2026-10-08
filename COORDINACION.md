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
- 2026-10-08 · Build: `COMPILAR_PHOENIX.bat` (no versionado) ahora compila en `_phoenix-link\` (worktree desacoplado de la rama activa de la carpeta principal). Salida: `_phoenix-link\x64\Release\PhoenixLink.exe`. Ya no mata `MSBuild`/`cl`/`link`, para no cortar compilaciones del Mercado.
