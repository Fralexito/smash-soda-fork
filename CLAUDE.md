# Phoenix Link / Phoenix Mercado — guía para Claude

Fork de Smash Soda (Parsec) convertido en **Phoenix Link**, la app de PC de Phoenix Evolution Series, más el módulo **Phoenix Mercado** (option file de PES 2021 ↔ Liga Máster de la web).

**Antes de tocar nada lee `COORDINACION.md`** (ramas, carpetas, contratos y protocolo de dos cuentas) y el final de tu bitácora:
- Frente LINK (rama `plan-ux` en el worktree `C:\dev\smash-soda-fork\_phoenix-link`, carpeta `SmashSoda/`; antes `rediseño-phoenix-portal`): `REGISTRO-LINK.md`.
- Frente SYNC (antes «Mercado»; rama `mercado-fase0` en la raíz `C:\dev\smash-soda-fork`, carpeta `PhoenixSync/`): `PhoenixSync/REGISTRO.md`.

**Compilar Link:** `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release --parallel`. **Nunca** `COMPILAR_PHOENIX.bat`: hace `git checkout --force` y borra lo no commiteado (21-oct-2026).

Fralex trabaja con **dos cuentas de Claude** sobre este repo: otra cuenta puede haber avanzado desde la última vez. `git fetch` + `git pull --rebase` siempre al empezar.

Reglas mínimas: no tocar la carpeta del otro frente; no hacer `git checkout` de otra rama en `C:\dev\smash-soda-fork` (Link compila en el worktree `_phoenix-link`); compilar sin errores antes de commitear; una línea en la bitácora por cambio; la web y Supabase se piden por prompt al chat WEB.

Frente LINK: trucos de entorno y cómo retomar en `docs/RETOMAR-LINK.md`.
