# Comparador de Player.bin: Conmegol vs Sudamerican (10-nov-2026, solo lectura)

Fuentes (copias leídas, nada modificado):
- Conmegol: `D:\Frank\Games_\Conmegol Patch\download\CGP_database.cpk` → `common/etc/pesdb/Player.bin`
- Sudamerican: `D:\SP2026\download\SP_Subs.cpk` → `common/etc/pesdb/Player.bin` (existe también `SP_Subs_WC.cpk`, versión del Mundial; no comparada)

Cómo se lee: el .cpk tiene el índice (TOC); posición real = `ContentOffset`(2048) + `FileOffset`. El .bin lleva cabecera de 16 bytes (3 bytes + `WESYS` + tamaño comprimido LE32 + tamaño real LE32) y luego un flujo zlib. Dentro: registros de 312 bytes (mapa en `ESTUDIO_FL_MOD_STUDIO_PARA_NUESTRO_PARCHE.md`). Verificado con datos reales: nombres, edades, nacionalidades y atributos salen coherentes (Ronaldinho, Santa Cruz, Kawashima, Milner...).

Resultados:
- Conmegol: 29.997 jugadores. Sudamerican: 29.323.
- IDs en ambas bases: 14.911. Solo en Conmegol: 15.086. Solo en Sudamerican: 14.412.
- Mismo ID y mismo nombre: 14.200 (en esos, los atributos cambian casi siempre: ~12.500 tienen velocidad/definición/resistencia distintas, 3.756 distinta altura, 5.248 distinto peso, 303 distinta nacionalidad, 1.682 distinta posición registrada).
- Mismo ID pero nombre distinto: 711 (casi siempre escrito más corto o más largo, ej. «Flávio Silva» / «Flávio»).
- Mismo nombre+nacionalidad con ID distinto: 4.013 (el mismo jugador con otro ID; ver `mismo_jugador_distinto_id.csv`).
- Exclusivos (ni mismo ID ni mismo nombre+nacionalidad): Conmegol 9.934, Sudamerican 10.428 (`exclusivos_*.csv`).

Límites (decirlo claro): comparar por nombre es una primera pasada. Hay variantes de escritura («Gaby Torres» / «Gabriel Torres») y apodos que cuentan como «distinto». «Exclusivo» aquí incluye retirados, jugadores genéricos o editados. Los atributos «+40» son los que muestra el juego en la ficha.

Scripts: `cpkx.py` (índice), `ext.py` (extrae y descomprime), `cmp.py` (compara), `teams.py` (equipos). Correr con `python3 -I`; las rutas son las del entorno de la sesión, ajustarlas.

## Parte 2: equipos y asignaciones

Formatos usados (mapa del repositorio de Matozanato, confirmado con datos reales):
- `Team.bin`: registros de 1.532 bytes; ID de equipo en +0x08, nombre en +0x170 (70 bytes), abreviatura en +0x372 (3 bytes). Conmegol 749 equipos, Sudamerican 726 (los dos son múltiplo exacto de 1.532).
- `PlayerAssignment.bin`: 16 bytes por fila (id de fila, id de jugador, id de equipo, número y orden). Conmegol 19.612 filas, Sudamerican 17.214.

Resultados:
- Equipos con el mismo nombre en las dos bases: **292** (260 con el mismo ID). Solo en Conmegol: **455**. Solo en Sudamerican: **434**. (`equipos_comparacion.csv`)
- Exclusivos de Conmegol: 9.934 (3.915 sin equipo; el resto en clubes como «Atlético FC», «CD Estrella Roja», «Platense FC», más equipos «Classic» de 40 jugadores).
- Exclusivos de Sudamerican: 10.428 (7.338 sin equipo; el resto en «Sudamerican Project Club», Auckland City, Urawa Red Diamonds, Mantova, selecciones pequeñas como San Marino, Gibraltar, Faroe, etc.). (`exclusivos_*_con_equipo.csv`)

Límites: comparar equipos por nombre falla con variantes («Republic of Ireland» / «Ireland»). «Sin equipo» son jugadores en la base pero sin fila de asignación (agentes libres o relleno). Todavía NO se leyeron las ligas (`Competition*.bin`); falta el mapa de esos registros (en el repositorio de Matozanato está en `tools/mkleague.py`).

Los CSV completos están en Drive («Phoenix Mercado — Base de conocimiento PES 2021») y en `D:\Frank\IA\CLAUDE\pes-db\comparador`.
