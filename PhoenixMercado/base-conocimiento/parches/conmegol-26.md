# Parche: ConmeGOL Patch 26 (PES 2021) — auditoría del 8 oct 2026

## Identificación

| Qué | Dato |
|---|---|
| Nombre interno usado por Mercado | `conmegol-26` |
| Carpeta del parche | `D:\Frank\Games_\Conmegol Patch\` |
| Base de datos (CPK) | `ConmeGol Extras\ConmeGOL Patch 26\download\CGP_database.cpk` (dentro: `Player.bin`) |
| Carpeta de guardados | `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save` (número de Steam **239200**) |
| Comparte la carpeta de guardados con | Sudamerican Project (misma carpeta `save`: cuidado con mezclar) |
| Texto que muestra el juego en Cargar | `Manchester City FC / Premier League · fecha · competición` |
| Nombre del parche en la LM | «Conmegol» (no «Conmebol») |

## Tamaños (sirven para detectar una versión nueva)

| Qué | Valor |
|---|---|
| Catálogo (option file + CPK) | **711 clubes, 18.304 jugadores** (7 lotes para la web) |
| Option file: equipos con plantilla | 749 (u16 en +100) |
| Option file: jugadores editados | 21 (u16 en +96) |
| Liga Máster: bloques de equipo | 700 (627 con nombre; selecciones desde el índice 528) |
| Liga Máster: bloques de alineación de la IA | 629 |
| Liga Máster: fichas de 596 B | 5.759 |
| Blob: fichas de 156 B | 16.422 |
| Guardado descifrado (datos) | 19.759.355 B al inicio (4/8/2026) → 19.795.679 B el 31/8/2026 (crece con el blob) |

## IDs y referencias útiles (índice de bloque en la LM · ID en el option file)

| Equipo | Índice | ID option | Nota |
|---|---|---|---|
| Manchester City (equipo del usuario) | 154 | 173 original; en la LM figura como **−11** (0xFFFFFFF5) | su alineación va en el bloque 627 |
| Real Madrid | 135 | 109 | |
| Liverpool | 153 | — | |
| Santos | 66 | 1254 | |
| Boca Juniors | — | 139 | prueba de Lautaro Blanco |
| Sporting CP | — | 193 | |

Jugadores de referencia (pid del parche): Haaland 133543 (reg 4340) · Neymar 40352 (reg 1936) · Guéhi 131286 · Stones 47785 ·
Mbappé 110718 · Isak 115079 · Lautaro Blanco 141350.

## Competiciones dentro de la Liga Máster (listas de equipos, bloques de 3000 B)

| Lista | Equipos | Qué parece ser |
|---|---|---|
| 0xB0D870 | 30 | Liga argentina (Primera) |
| 0xB0E428 | 16 | Liga ecuatoriana |
| 0xB0EFE0 | 20 | Liga colombiana |
| 0xB0FB98 | 16 | Liga chilena |
| 0xB10750 | 20 | Brasileirão |
| 0xB11308 | 18 | Liga 1 Perú |
| 0xB11EC0 | 20 | LaLiga |
| 0xB12A78 | 20 | **Premier League (la del usuario; ID de competición 9)** |
| 0xB13630 | 20 | Serie A |
| 0xB141E8 | 16 | Chile segunda (Cobreloa…) |
| 0xB14DA0 | 18 | Ligue 1 |
| 0xB15958 / 0xB17C80 | 12 | Costa Rica (dos listas) |
| 0xB193F0 / 0xB19FA8 | 47 / 44 | Copas sudamericanas (Sudamericana / Libertadores, fases) |
| 0xB1B718 | 19 | Champions (clubes europeos) |
| 0xB1E5F8 | 25 | Concacaf |
| 0xB1F1B0 | 66 | Argentina ampliada (Primera + Nacional) |
| 0xB1FD68 / 0xB20920 / 0xB214D8 / 0xB22090 / 0xB22C48 | 36 / 40 / 26 / 32 / 32 | Colombia, Brasil, Ecuador, Chile y Perú ampliadas (con segundas) |
| 0xB23800 | 18 | Liga saudí |
| 0xB243B8 | 28 | MLS |
| 0xB28A08 … 0xB2B8E8 | 48 / 24 / 16 / 15 / 16 | Selecciones (Mundial, Eurocopa, Concacaf, Asia, África) |
| 0xB2D058 … 0xB2F380 | 36 / 20 / 16 / 20 | Segundas divisiones (Argentina, Brasil, Ecuador, Colombia) |

IDs de competición vistos en el calendario del usuario: **9** = Premier · **63** = ventana de fichajes · 60, 62, 11, 7 = otros eventos.

## Rarezas de este parche

- El equipo del usuario conserva un bloque de alineación «viejo» con el ID 173 (bloque 154) que el juego ya no actualiza.
- Las tablas C y D del usuario tienen 32 juveniles (regs ≥ 16358) entre el primer equipo y los fichados.
- Banca de 12 en la carrera auditada.
- La primera subida del catálogo a la web falló por un jugador sin nombre (ya saneado).

## Pruebas hechas en el juego con este parche

Ver `../PRUEBAS.md`: 13 pruebas (10 ✅, 3 ❌ corregidas). Traspasos probados: Haaland City→Santos, Mbappé Madrid→Santos, Guéhi
City→Madrid, Isak Liverpool→Santos; presupuesto 500 M / tope 200 M.
