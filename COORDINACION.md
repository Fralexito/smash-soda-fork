# Coordinación entre chats

Dos trabajos independientes dentro del mismo Phoenix Link:

| Trabajo | Rama | Carpeta | Regla |
|---|---|---|---|
| Rediseño de Phoenix Link (interfaz, nombre, logo) | `rediseño-phoenix-portal` | `SmashSoda/` | No tocar `PhoenixMercado/` |
| Phoenix Mercado (módulo aparte) | `mercado-fase0` | `PhoenixMercado/` | No añadir a la build de `SmashSoda/`; avisar antes de tocar `SmashSoda/` |

## Contratos compartidos (si cambian, avisar al otro chat)
- Token: `%APPDATA%\Trybuchet\Smash Soda\phoenix-token.dat`, cifrado DPAPI (`CryptProtectData`), sin entropía adicional. Mercado lo lee en solo lectura. Sin cambios de ubicación, nombre ni cifrado.
- Carpeta de ajustes `%APPDATA%\Trybuchet\Smash Soda`: sin cambios.
- Agente HTTP `PhoenixSoda/1.0` (`link/Http.cpp`): sin cambios.

## Registro de cambios que afectan al otro chat
- 2026-10-07 · Renombre a Phoenix Link: ejecutable `PhoenixLink.exe` (antes `PhoenixSoda.exe`), `OUTPUT_NAME` en `SmashSoda/CMakeLists.txt`. La etiqueta de texto del token pasó de "PhoenixSoda" a "PhoenixLink"; no afecta a la lectura.
