# Fichajes de Phoenix (lista de cambios)

Esta carpeta viaja con Phoenix Link (`<exe>\fichajes\fichajes.json`). **No lleva ninguna base de datos**: solo una lista corta de cambios «jugador → equipo». Link la aplica sobre la base REAL del PC (la del parche/modo activo) y deja el resultado en `SiderAddons\livecpk\Phoenix-DB\…\PlayerAssignment.bin`. Guía: `docs/FICHAJES-Y-SYNC.md («FICHAJES-COMO-CAMBIOS»)`.

Hoy la lista va **vacía a propósito**: la comparación de la carpeta Phoenix-DB de Fralex con su base (olmosjr23\Database) demostró que no había ningún fichaje dentro (era una copia idéntica de esa base). Los fichajes reales aparecerán aquí o en `%APPDATA%\Phoenix Mercado\fichajes\fichajes.json` (punto de entrada para Sync/Mercado: si existe, manda sobre este).

Formato (versión 1):
```json
{ "version": 1, "regla": "relajada",
  "cambios": [ { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto opcional" } ] }
```
- `regla`: «relajada» (por defecto, gana el cambio de la lista aunque el jugador ya no esté en `origen`) o «estricta» (si el origen no coincide, se salta).
- `origen` (opcional): equipo donde se espera al jugador; hace falta si el jugador sale en dos equipos (club y selección).
- Solo mueve o da de alta jugadores que existen en la base y en equipos que existen en ella, máximo 40 por equipo. No hay bajas.

## Versión 2 (opcional): por nombre
Como los `player_id` y `team_id` cambian entre parches, desde Link 0.2.47 un cambio puede ir por nombre (`"version": 2`):
```json
{ "version": 2, "cambios": [ { "jugador": "Cristiano Ronaldo", "equipo_origen": "Al-Nassr FC", "equipo_destino": "Real Madrid CF" } ] }
```
Se compara de forma exacta (sin mayúsculas ni acentos) contra la base de cada PC; si el nombre no es único se salta con el motivo. Detalle en `docs/FICHAJES-Y-SYNC.md («FICHAJES-COMO-CAMBIOS»)`.
