# Tamaños de cancha Street (7-nov-2026, 09:35 PC)

Para el hilo IA (tabla del Core). Medidas en metros, origen = centro del campo, x = largo (arcos en ±x), y = ancho (cámara del lado y<0), z = altura. Las medidas **S** salen de lo instalado (scripts `v26_anillo`: `geometria.py`, `reja.py`, `ambiente.py`, `layout.py`); **M y L** son las acordadas con Frank y escaladas con las mismas reglas que S (marcas de campo = línea; reja = 0,16 m por dentro de la línea de fondo / 0,06 m por la banda; muro de ladrillo 1,35–2,3 m detrás de la reja). Todo lo de M y L es PROVISIONAL hasta construir y probar el fpk.

| Dato | S (instalado) | M (4v4–6v6) | L (7v7–11v11, 75×48) |
|---|---|---|---|
| Carpeta de estadio | `Phoenix\Calle 30x20` | `Phoenix\Calle 42x25` | `Phoenix\Calle 75x48` |
| Líneas de juego (largo × ancho) | 30,22 × 20,02 (LX 15,11, LY 10,01) | 42,0 × 25,0 (LX 21,0, LY 12,5) | 75,0 × 48,0 (LX 37,5, LY 24,0) |
| Reja visual / muro lógico del Core | x ±14,95, y ±9,95 | x ±20,85, y ±12,45 | x ±37,35, y ±23,95 |
| Altura de la reja | 6,0 m fondo y lados de arco; lado cámara (y<0) 1,0 m | igual | igual (6,0 / 1,0) |
| Techo lógico (balón) | 40 m (Core 0.5.21+) | 40 m | 60 m (propuesto) |
| Boca del arco (ancho × alto) | 3,0 × 2,0 (postes en y ±1,5; hueco de reja ±1,6 visual; hueco del Core ±2,4) | 3,0 × 2,0 | 7,32 × 2,44 (postes y ±3,66; hueco de reja ±3,8) — PROVISIONAL |
| Red detrás del arco | 1,6 m detrás de la línea de fondo, desde 2,6 m (sobre el travesaño) hasta 6 m | igual, desde 2,6 m | desde 2,8 m hasta 6 m |
| Muro de fondo (ladrillo, y>0) | y = +11,3, alto 8 m | y = +13,8 | y = +25,5 |
| Muros laterales (detrás de los arcos) | x = ±17,2 (desde y = −14,2 hasta el fondo) | x = ±22,9 (desde y = −16,8) | x = ±40,0 (desde y = −28) |
| Losa de suelo con textura detallada | ±17,1 × ±11,4 | ±24,0 × ±14,0 | ±43,0 × ±27,0 |
| Anillo opaco exterior (z = 0,10) | hasta ≈ ±216 × (−300 / +180) | igual (celdas grandes) | igual |
| Centro del campo | (0, 0), círculo radio 3,0 | círculo radio 4,0 | círculo radio 8,0 |
| Área del portero | semicírculo radio 5,0 desde los postes; marca a 5,0 y 8,0 m de la línea de fondo | radio 6,0; marcas a 6,0 y 9,0 m | área grande 14,0 m × 34,0 m; área chica 5,0 × 16,0; penal a 10 m |
| Esquinas | arco de esquina 0,25 m | igual | igual |
| Zona de cambios (marcas en y = −LY) | x = ±5 y ±10 | x = ±7 y ±14 | x = ±12,5 y ±25 |
| Farolas / banco | x = −12, 0, 12; banco x −6,4…−4,4 (junto al muro de fondo) | x = −18, −6, 6, 18; banco x −8…−5 | x = −30, −15, 0, 15, 30; 2 bancos |

Nota (14:30 UTC): Frank pidió que L quede MUY por debajo de la cancha clásica de PES (105×68): L pasa de 90×55 a **75×48** (≈ 3600 m², 50 % del área clásica; ~160 m² por jugador con 22).

## Qué entrega el escenario / qué necesita del Core
- El escenario NO pone colisiones: son medidas visuales. El Core debe usar las mismas para: muro de jugadores, techo, caja del portero, saques, `cajas` de `cancha_chica_free` y red de seguridad detrás del arco.
- Selección: `content\phoenix\cancha_tamano.ini` (S/M/L) escrita por el Core; el parche `street_auto_estadio` de StadiumServer elige la carpeta (S: «Calle 30x20», M: «Calle 42x25», L: «Calle 75x48»). Si el ini falta o es inválido → S.
- Mínimo recomendado de jugadores por equipo: S 2–3, M 4–6, L 7–11 (con menos, la cancha queda grande y se pierde el juego de calle).
- Cámara: en M y L hace falta más alejada (S usa la del entrenamiento tal cual). Lo decide el Core.
- Estado de construcción: S instalado; M y L irán en `herramientas\Blender279\calle_30x20\v27_M` y `v27_L` (todavía sin construir ni instalar).

## Arcos: 3 tamaños independientes de la cancha (acordado 14:33 UTC)
- Tamaños: **A = 3,0 × 2,0** (por defecto en S), **B = 5,0 × 2,2** (por defecto en M), **C = 7,32 × 2,44** (por defecto en L). Boca (postes en y): A ±1,5 · B ±2,5 · C ±3,66.
- Selección independiente de la cancha: fila F9 «Tamaño de arco: auto / 3×2 / 5×2 / 7,32×2,44» que escribe el Core (clave `arco=auto|A|B|C` junto a `cancha_tamano.ini`). `auto` = el de la tabla anterior. Se puede poner arco chico en cancha grande y viceversa.
- Reja y red detrás del arco: **una malla de pitch por combinación (cancha × arco)**, cada una con su hueco exacto de reja (A ±1,6 · B ±2,6 · C ±3,8) y la red 1,6 m detrás, desde 0,2 m por encima del travesaño (A 2,6 · B 2,8 · C 2,9) hasta 6 m. Es mejor que un hueco único de 7,32 en todas (en una cancha con arco chico quedaría una reja abierta a ambos lados del arco).
- Carpeta de cada combinación: `content\phoenix\canchas\<S|M|L>\arco_<A|B|C>\pitch_st002.fpk(.fpkd)` (9 archivos de ≈0,7 MB) + `...\arcos\<A|B|C>\goal_post.fpk(.fpkd)` (geometría de postes por tamaño). El módulo `phoenix_arcos.lua` copia el par correcto a Calle al elegir equipos (según `cancha_tamano.ini` + `arco=`) y las 5 texturas de diseño aleatorio (rojo/blanco, negro/blanco, blanco liso, azul/blanco, amarillo/negro) valen para los tres tamaños de arco.
- Estado: sin construir; NO se instala nada hasta que el Core soporte tamaño de cancha y de arco. Pendiente de localizar: dónde queda instalada hoy la geometría de los palos (`goal_post.fpk`; en Calle la carpeta common\goal solo trae la textura).
