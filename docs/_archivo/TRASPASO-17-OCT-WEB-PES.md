# Traspaso 17-oct-2026 · Web ↔ PES (para continuar con la otra cuenta)

Fralex pidió: «sube todo a GitHub rápido para no dejar a medias; ya usaré mi otra cuenta para avanzar».
Las 4 sesiones de trabajo se cortaron por el **límite de uso** (vuelve a las 9:10 pm, hora de Lima). Todo su trabajo está en GitHub.

## Qué es cada frente
Un solo formulario en la web («Cambios oficiales»): cada cambio va a **base normal (amistosos)**, **Liga Máster** o **las dos**, llega solo a cada PC y cada PC informa el resultado al **registro de usuarios**.
Contrato: `docs/CONTRATO-WEB-PES.md` v1 (en este repo, rama `rediseño-phoenix-portal`, y en la web `docs/`, rama `borrador`).

| Frente | Migración | Estado | Dónde está |
|---|---|---|---|
| Amistosos + registro + formulario único (BASE) | 125 | Contrato publicado. Link 0.3.19 y web (formulario, registro) **subidos a las ramas principales**. Faltaban rutas de la función web y terminar el seguimiento automático del canal oficial en Link (según la sesión de estadísticas) | Link `rediseño-phoenix-portal` (5ae6b2b) · web `borrador` (6b384ad) · respaldo `respaldo/ws-amistosos-*` |
| Estadísticas e información | 126 | **Hecho y subido**: Link 0.3.18 aplica stats/info en el Player.bin de Phoenix-DB; web con migración 126, formulario, historial y deshacer; pruebas en verde. Falta: rutas de la sesión BASE y probar en un PES real | web `borrador` (9d4ee85) · respaldo `respaldo/ws-stats-*` |
| Liga Máster | 127 | **A medias** (Sync: CambiosLM, PartidasLM, LigaMasterWeb, pruebas lm127; web: migración 127 y panel) | `respaldo/ws-lm-sync`, `respaldo/ws-lm-link` (repo Link) · `respaldo/ws-lm-web` (repo web) |
| Botón ACTUALIZAR (auto-actualización de Link) | 128 | **A medias** (Link: actualizador y EMPAQUETAR-RELEASE.ps1; web: migración 128, página link-versiones, pruebas) | `respaldo/ws-actualizar-link` (repo Link) · `respaldo/ws-actualizar-web` (repo web) |

Las ramas `respaldo/*` llevan un commit «RESPALDO (a medias)»: **no están terminadas ni probadas enteras**. No se han unido a las ramas principales a propósito.

## SQL de Supabase (todavía NO pegado)
Orden obligatorio: **125 → 126 → 127**; la **128** va aparte, en cualquier momento. Cada una trae su prueba `supabase/tests/12X_*_test.sql`.
- 125 y 126: en `borrador` (terminadas por sus sesiones; revisar antes de pegar).
- 127 y 128: solo en sus ramas `respaldo/*` (a medias: **no pegar** hasta terminarlas).

## Cómo seguir (otra cuenta)
1. `git fetch origin` en los dos repos.
2. Para cada frente a medias: `git worktree add C:\dev\ws-<nombre> respaldo/<rama>` y continuar leyendo su commit de respaldo y el contrato.
3. Al terminar: pasar los cambios a la rama principal (`git pull --rebase origin <rama>` + `git push origin HEAD:<rama>`), subir kVersion y anotar en `REGISTRO-LINK.md` / `REGISTRO.md`.
4. Nunca subir `SmashSoda/phoenix/PhoenixBetaSello.h`.

## Lo último de Link en esta cuenta (antes de los frentes)
0.3.12–0.3.16 (traspasos sin papel; aprobar al enviar y al recibir; «Mantener mis fichajes en mi juego»: los fichajes propios van a la Phoenix-DB y no se pierden al cerrar PES). Falta probar en el PES de Fralex: la primera vez con PES cerrado para crear Phoenix-DB y la línea del sider.ini.
