# Player.bin (PES 2021 base): habilidades de cada jugador, decodificadas (05-nov)

Archivo: `common/etc/pesdb/Player.bin` de `dt10_x64.cpk` del PES base (no el de Phoenix/Conmegol), sacado con CriPakTools y descomprimido (cabecera `WESYS` + zlib). 17 781 registros de 312 B desde el offset 8 de lo descomprimido.

## Cómo se lee (C, comprobado con 5 jugadores)
- Los primeros 64 B del registro se leen como un entero little-endian de 512 bits (`v = int.from_bytes(registro[:64], "little")`).
- Cada habilidad es un campo de **6 bits** que guarda **valor − 40** (el juego muestra 40..99). `valor = ((v >> bit_inicio) & 63) + 40`.
- Los bits se cuentan desde el inicio del registro (el id del jugador son los bits 0..31).

## Tabla de bits (bit_inicio, ancho 6, desplazamiento +40)
| Habilidad (texto del juego) | bit_inicio |
|---|---|
| Actitud ofensiva | 306 |
| Control de balón | 217 |
| Drible | 288 |
| Posesión del balón | 352 |
| Pase al ras | 199 |
| Pase bombeado | 338 |
| Finalización | 332 |
| Cabeceador | 224 |
| Balón parado | 186 |
| Efecto | 268 |
| Actitud defensiva | 211 |
| Recuperación de balón | 248 |
| Agresividad | 320 |
| Actitud de portero | 262 |
| Atajar (PT) | 300 |
| Despejar (PT) | 205 |
| Reflejos (PT) | 256 |
| Cobertura (PT) | 236 |
| Velocidad | 242 |
| Aceleración | 280 |
| Potencia de tiro | 294 |
| Salto | 230 |
| Contacto físico | 326 |
| Equilibrio | 312 |
| Resistencia | 274 |

Ya conocidos de antes: posición registrada = bits 370-373 (0 PT, 1 DFC, 2 LI, 3 LD, 4 MCD, 5 MC, 6 MDI, 7 MDD, 8 MO, 9 EI, 10 ED, 11 SD, 12 DC); nación = bits 169-175.

## Cómo se comprobó
- Valores leídos de la ficha de detalle de PES (Nv.2/30; coinciden con raw+40 sin ningún aumento por nivel) de 5 jugadores: L. Messi (7511), M. Ter Stegen (61672), Piqué (8639), A. Griezmann (42316) y Neto (43202).
- Las 20 habilidades de campo salen de forma única con 4 jugadores y se verificaron con un quinto (Neto, las 20 coinciden). Las 5 de portero se separaron con Ter Stegen y Neto (los porteros que difieren): Actitud de portero 262, Atajar 300, Despejar 205, Reflejos 256, Cobertura 236.
- Los valores de Messi, Ter Stegen, Piqué, Griezmann y Neto usados están en el scratchpad de la sesión (`gt.txt`); aquí solo se citan nombres de campos y bits.

## Pendiente
- **Media (overall) por posición:** no está guardada. Es una suma ponderada de estas habilidades que varía según la posición; con 5 jugadores no basta para ajustarla. Falta una fórmula (o leerla de la función del exe que la calcula).
- Otros campos de la ficha (pie hábil, uso/precisión de pie malo, estabilidad 1-8, resistencia a lesiones 1-3, altura, peso, estilo de juego) no están ubicados todavía: se pueden buscar igual con los valores vistos (Messi pie izq., pie malo 1/4 y 3/4, estabilidad 8/8, lesiones 2/3, 170 cm; Ter Stegen pie der., 2/4 y 3/4, 5/8, 2/3, 187 cm; Piqué pie der., 2/4 y 2/4, 6/8, 3/3, 194 cm; Griezmann pie izq., 2/4 y 2/4, 6/8, 3/3, 176 cm; Neto pie der., 1/4 y 1/4, 5/8, 1/3, 190 cm).

## Código mínimo
```python
import zlib
raw = open("Player.bin", "rb").read()
i = raw.find(b"WESYS")
d = next(zlib.decompress(raw[k:]) for k in range(i, i + 40) if _ok(raw, k))  # primer k donde zlib abre
v = int.from_bytes(d[off:off + 64], "little")   # off = 8 + 312 * n
val = lambda bit: ((v >> bit) & 63) + 40
control_balon = val(217)
```

## Uso en el catálogo (06-nov)
- `tools/catalogo_pesdb.py` ya guarda en cada jugador `habilidades` (las 25 de la tabla) y una media estimada `rating` con `rating_estimado = True`.
- P: `estimar_media` = promedio de las habilidades clave del grupo de puestos (PT, defensa, medio, ataque) + un ajuste por grupo, calibrado con Messi 94, Piqué 87 y Ter Stegen 89 (los tres salen exactos). No es la fórmula de PES: puede fallar por 1–3 puntos en otros jugadores. Sirve para rareza, precio y filtros de agentes (media_min). Un `--extra` con `rating` la reemplaza.
- Para que la PC lo use: volver a correr `catalogo_pesdb.py PlayerAssignment.bin --player Player.bin -o catalogo.json` con el Player.bin del PES base.
