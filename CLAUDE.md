# Phoenix Link / Phoenix Mercado — guía para Claude

Fork de Smash Soda (Parsec) convertido en **Phoenix Link**, la app de PC de Phoenix Evolution Series, más el módulo **Phoenix Mercado** (option file de PES 2021 ↔ Liga Máster de la web).

**Antes de tocar nada lee `COORDINACION.md`** (ramas, carpetas, contratos y protocolo de dos cuentas) y el final de tu bitácora:
- Frente LINK (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`): `REGISTRO-LINK.md`.
- Frente MERCADO (rama `mercado-fase0`, carpeta `PhoenixMercado/`): `PhoenixMercado/REGISTRO.md`.
  Además, **cada prueba hecha en el juego** (salga bien o mal) se anota en `PhoenixMercado/PRUEBAS.md` (diario de prueba y error; al final se convierte en el PDF de documentación).

Fralex trabaja con **dos cuentas de Claude** sobre este repo: otra cuenta puede haber avanzado desde la última vez. `git fetch` + `git pull --rebase` siempre al empezar.

Reglas mínimas: no tocar la carpeta del otro frente; no hacer `git checkout` de otra rama en `C:\dev\smash-soda-fork` (Link compila en el worktree `_phoenix-link`); compilar sin errores antes de commitear; una línea en la bitácora por cambio; la web y Supabase se piden por prompt al chat WEB.
