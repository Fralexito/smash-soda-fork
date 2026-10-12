# PROMPT para el chat WEB / Sync · Lista de fichajes como cambios

> Pégalo en el chat WEB o en el de Phoenix Sync. Origen: chat LINK, 2026-10-12. Diseño completo: `docs/FICHAJES-COMO-CAMBIOS.md` en `smash-soda-fork` (rama `rediseño-phoenix-portal`). **Link ya está hecho (0.2.46); lo que falta es solo de la web/Sync.**

## Qué quiere Fralex
Que los fichajes viajen como una **lista corta de cambios** y no como una base de datos completa: Phoenix Link los aplica en cada PC sobre la base REAL de ese PC (ConmeGOL 26, 27, UEFAGOL 27, Clásicos…). Así no se distribuye ningún `Player.bin` ni `PlayerAssignment.bin` de otros modders y los fichajes funcionan sobre cualquier parche.

## Qué hace Link hoy (no hay endpoint nuevo)
Link lee `%APPDATA%\Phoenix Mercado\fichajes\fichajes.json`. Si no existe usa el que lleva Link (vacío). El botón «Aplicar fichajes de Phoenix» (SYNC › Puente) genera el `PlayerAssignment.bin` de Phoenix-DB sobre la base de cada modo.

## Lo que tiene que hacer Sync / la web
1. Dejar ese archivo en esa carpeta (escritura atómica: `.tmp` y renombrar), con este formato:
```json
{
  "version": 1,
  "regla": "relajada",
  "nombre": "Mercado 2026-10-12",
  "cambios": [
    { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto corto opcional" }
  ]
}
```
2. Reglas del formato: `version` debe ser 1 (Link rechaza otras); `regla` «relajada» (gana la lista si el jugador ya no está en `origen`) o «estricta» (se salta); `player_id` y `team_id` son enteros > 0; `origen` (opcional) es el `team_id` donde se espera al jugador, necesario si el jugador sale en dos equipos (club y selección); como máximo 5.000 cambios; solo altas y movimientos (no hay bajas).
3. Los ids son los de PES 2021 (`player_id` de Player.bin, `team_id` de PlayerAssignment.bin). Un cambio que apunte a un jugador o equipo que no existe en la base de ese PC **se salta con motivo** (nada se rompe), y Link muestra «N aplicados, M saltados».
4. No hace falta que Sync sepa qué parche tiene el amigo: la lista es la misma para todos; Link decide qué se aplica en cada modo.
5. (Opcional, más adelante) un estado de vuelta: Link guarda `fichajes_aplicados.json` dentro de `SiderAddons\livecpk\Phoenix-DB` de cada modo (fecha, huella de la lista, aplicados/saltados). Si Sync quiere mostrarlo, hay que acordar cómo lo recoge; hoy no se envía a la web.

## Qué NO hay que hacer
- No mandar bases completas, ni `Player.bin`, ni `PlayerAssignment.bin` ya hechos.
- No escribir dentro del juego: eso lo hace Link, con PES cerrado y copia de seguridad.
