# Tabla maestra Phoenix (v0, 10-nov-2026): IDs propios y equivalencias entre Conmegol y Sudamerican

Archivos nuevos, hechos solo leyendo las bases (nada modificado). Salen de `pes-db/comparador/` (ver su LEEME para el método).

## Qué es
Tres tablas CSV. Cada fila es UNA entidad Phoenix con su ID propio y los IDs que tiene en cada base:
- `maestra_jugadores.csv`: **40.955** jugadores Phoenix (`PXP-000001`…). 18.365 están en las dos bases, 11.632 solo en Conmegol, 10.958 solo en Sudamerican.
- `maestra_equipos.csv`: **1.181** equipos (`PXT-00001`…). 292 en ambas, 455 solo Conmegol, 434 solo Sudamerican.
- `maestra_ligas.csv`: **111** competiciones (`PXL-001`…), por código de texto (`ENGLAND_D1_LEAGUE`), porque los números de competición cambian entre bases.

## Reglas que se usaron para decir «es el mismo»
Jugador: (1) mismo ID en las dos bases = mismo jugador (`mismo_id_mismo_nombre` 14.200; `mismo_id_nombre_distinto` 711, marcados «revisar» porque el nombre cambia, ej. «Flávio Silva»/«Flávio»); (2) mismo nombre y misma nacionalidad, ID distinto, solo si el nombre es único en ambas bases y el ID no aparece ya en la otra (`mismo_nombre_id_distinto` 3.454); (3) los 37 «parecidos» NO se unen, solo se anotan en la columna `nota` para revisar a mano. Hay 1.119 combinaciones nombre+nacionalidad repetidas (homónimos) que no se unen para no mezclar personas.
Equipo: mismo nombre normalizado (sin tildes ni símbolos, en minúsculas). Falla con variantes («Republic of Ireland» / «Ireland»).

## Cómo usarla
- `id_phoenix` es el ID estable propio. Nunca se reutiliza. Cuando exista Phoenix Studio, el ID dentro del juego (máx. 262.143 para jugador y equipo, 18 bits) se asigna aparte; esta tabla dice qué ID del juego corresponde en cada parche.
- Para sacar los exclusivos: filtrar `estado = solo_conmegol` o `solo_sudamerican`.
- Para cambiar de parche un jugador, usar `id_conmegol`/`id_sudamerican` de la misma fila. Un campo puede tener varios IDs separados por `;` (homónimos o duplicados).

## Límites
Es la versión 0 y automática: faltan los ~700 casos «revisar», los homónimos, y la comparación de caras/uniformes. Los nombres que usa son el nombre impreso (`printed`) del registro. No incluye todavía datos con permiso de autores (ratings reescritos, caras, escudos): esos se resuelven aparte.

## Versión v1: casos «revisar» resueltos (10-nov-2026)

Los 711 jugadores con el mismo ID y nombre distinto se resolvieron con reglas (misma nacionalidad, edad ±3 y nombre contenido, similitud ≥ 0,7, misma camiseta o mismo apellido). Archivo `maestra_jugadores_v1.csv` (reemplaza a la v0) y `revisar_resueltos.csv` (lista con el veredicto de cada caso). Script: `revisar2.py`.
- **368** son el mismo jugador con el nombre escrito distinto (ej. «Gaby Torres» / «Gabriel Torres»): quedan unidos.
- **343** son **personas distintas que comparten el mismo ID** en cada base (ej. «T. Gresham» en Conmegol y «Dubar Enríquez» en Sudamerican, mismo ID, distinta nacionalidad y altura). Se separaron en dos entradas Phoenix. Hallazgo importante: en las dos bases hay IDs «de relleno» (jugadores generados y editados) que significan cosas distintas, así que **el ID del juego no basta para decir que es el mismo jugador**.
- Total v1: **41.298** jugadores Phoenix (18.022 en ambas, 11.975 solo Conmegol, 11.301 solo Sudamerican).
- Límite: la regla de «nombre contenido» puede unir de más casos como «Josep Antoni» / «Josep Gomes» (comparten solo el primer nombre) y separar de más a quien cambió de nacionalidad («Guilherme Dos Santos», nac. 30 vs 146). Son pocos; quedan como revisión manual opcional. Las notas de los 37 «parecidos» siguen sin unir.
