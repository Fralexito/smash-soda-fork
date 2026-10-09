# 01 · El motor de PES 2021 (vale para cualquier parche)

Versión del juego: eFootball PES 2021 SEASON UPDATE (PC/Steam). Todo lo de abajo se descubrió sobre el ConmeGOL Patch 26, pero
la **forma** de los archivos la pone el juego, no el parche: el parche solo cambia el contenido (jugadores, clubes, competiciones).
Lo que dependa del parche está en `parches/`.

Las posiciones (offsets) se dan en hexadecimal y son las del guardado de referencia. Todo lo que el programa usa de verdad lo
localiza **por anclas** (buscando la forma de los datos), no por posición fija.

---

## A. Cifrado y envoltura (option file y guardados)

| Qué | Dato | Estado |
|---|---|---|
| Cifrado | libpesXcrypter con la clave de PES 2021; se descifra y se vuelve a cifrar **idéntico** | ✅ |
| Envoltura | cabecera + descripción (384 B) + logo (17.315 B) + DATOS + serial (44 B) | ✅ |
| Descripción | bytes 0–127: nombre (el menú Cargar NO lo muestra) · 128–383: «texto info» (lo que SÍ muestra el menú) | ✅ |
| Hash de cabecera | el juego **no lo valida**: acepta archivos editados | ✅ |
| Tamaño de datos variable | el guardado de LM crece/encoge; el C++ re-cifra con cualquier tamaño | ✅ |
| Ranuras del menú Cargar | ranura N = archivo `ML0000000(N−1)` **en hexadecimal** (ranura 11 = `ML0000000A`) | ✅ |
| Autoguardado | al terminar un partido el juego **sobrescribe la ranura cargada**; identificar archivos por huella (md5) | ✅ |

## B. Option file (`EDIT00000000`): amistosos, salas y carreras nuevas

| Qué | Dónde | Estado |
|---|---|---|
| Nº de jugadores editados | u16 en +96 | ✅ |
| Nº de equipos | u16 en +100 | ✅ |
| Jugadores editados | desde 0x7C, 312 B cada uno (mismo formato que `Player.bin` del CPK) | ✅ |
| Equipos | desde 0x8ED2FC, 588 B cada uno (nombre en +104, abreviatura en +174) | ✅ |
| Plantillas | desde 0x9D4648, 284 B por equipo: ID equipo u32 · 40 IDs de jugador u32 · 40 dorsales u16 · 40 B («Unknown A») | ✅ (mover probado en juego) |
| **Tácticas por equipo** | desde 0xA09880, **628 B por equipo**, mismo orden que las plantillas: ID equipo u32 · formaciones (varias copias de coordenadas) · **orden de formación 40 B en +0x1E4** (índices de plantilla; 0–10 titulares) · **6 roles en +0x20C** | 🔎 (ver §G) |
| Bits de habilidades (Player.bin / editados) | ataque 370, control 281, regate 352, regate ajustado 416, pase raso 263, pase bombeado 402, finalización 396, cabezazo 288, balón parado 250, efecto 332, velocidad 306, aceleración 344, equilibrio 376, contacto físico 390, salto 294, resistencia 338, defensa 275, recuperación 312, agresividad 384 (6 bits, +40) · altura 216 (8 bits, +100) · peso 256 (7, +30) · edad 408 (6, +15) · posición 434 (4) · nacionalidad 233 (9) | ✅ |

**Corregido (8 oct, 16:30):** al mover un jugador el programa actualiza también el bloque de tácticas de los dos equipos, con la
misma lógica que la Liga Máster (`core/Alineacion.h`): el que se va sale del orden y de los roles, los índices mayores bajan uno,
si era titular o tenía rol hace falta sustituto (elegido por posición con el catálogo; a un portero solo lo cubre otro portero) y el
que llega entra como última reserva. Comprobado en el option file real del ConmeGOL 26: **749 bloques**, todos con el ID del
equipo en +0; 711 equipos con el orden como **identidad 0…39** (así lo deja el editor del juego) y 38 equipos vacíos con todo
0xFF. El formato (cola identidad o 0xFF) se conserva. Pendiente la prueba en el juego (amistoso con un option file temporal).
Antes de esto, al mover un jugador solo en la plantilla la alineación quedaba corrida (el portero suplente salía de delantero).

## C. Guardado de Liga Máster (`ML0000000N`): estructura general

| Zona | Dónde | Qué | Estado |
|---|---|---|---|
| Cabecera | 0x00–0x4F (80 B) | `10, 0x50, …, 700, 700`; los 32 B de 0x30 cambian en cada guardado y no se validan | ✅ |
| **700 bloques de equipo** | 0x50 + 1680·k | nombre +4 · ID option +656 · plantilla +0x14C (40 × `reg u32, pid u32`, relleno `65535,0`) · dorsales +0x29A (40 × u16) · contador +0x426 (1 B). k ≥ 528 = selecciones | ✅ |
| Alineaciones de la IA | ~0x133A30, **629 bloques de 600 B** | ver §D | ✅ |
| Fichas de jugador (596 B) | 0x329B00, una por `reg` (5.759) | pocos campos usados; **no** llevan el club ni el valor | 🔎 |
| Dinero del usuario | 0xC7DD00 | ver §F | ✅ |
| Tablas del equipo del usuario | 0xBE5FC8 … 0x1049B3C | ver §E | ✅ |
| Competiciones | desde ~0xB08000, **bloques de 3000 B** | lista de equipos de cada competición (IDs u32, relleno `0x0003FFFF`) | 🔎 |
| Calendario del usuario (meses especiales) | 0xA8D6D8 (enero), 0xAAB288 (junio) | 1 registro de 708 B por día: fecha · ID competición · rival; en días de liga, **10 IDs de partido u16** | 🔎 |
| **Ranking de clubes** (16 B) | 0xC016B0, 263 registros | `[?][ID interno de club u32][puesto u32][v u16 ×2]`; Real Madrid 1.º (1015), PSG 2.º, City 3.º… | 🔎 |
| **Blob comprimido** | cabecera 0x1140364, zlib desde 0x11403D4 | ver §H | 🔎 |
| Contratos (tabla I) | tras el blob (se desplaza con su tamaño) | ver §E | ✅ |
| Negociaciones abiertas | tras el blob (0x1279F18 en la referencia), registros de **60 B** | ver §E | ✅ |
| Historial de traspasos | ~0x12B430F, registros de 36 B | jugador, monto, fecha | 🔎 |
| Noticias / caja | 0xBE34B4 (`[reg][pid]…[tipo][fecha]`) · 0xC8018C (36 B, `[reg][pid]…[monto i32 ×100]`) | el juego anota ahí cada salida y cada pago | 🔎 |
| Jugadores creados por el juego | en las plantillas de la IA: `reg = 0xDB65xxxx`, pid > 126.000 | regens/canteranos; 771 al empezar, 1.127 un mes después; no están en el catálogo del parche | 🔎 |

## D. Alineación de los equipos de la IA (bloque de 600 B)

| Campo | Dónde | Estado |
|---|---|---|
| Índice del bloque (= k) | +0 (u32) | ✅ |
| ID option del equipo | +4 (u32) | ✅ |
| Nombre del técnico | +8 (UTF-8) | ✅ |
| Formaciones (coordenadas, varias copias) | +0x28… | 🔎 |
| **Orden de formación**: 40 índices de plantilla; 0–10 titulares, luego banca (7 o 12) y reservas; relleno 0xFF | +0x220 | ✅ |
| **Roles** (capitán, lanzadores): 6 índices de plantilla | +0x248 | ✅ |

Reglas vistas en el juego: el juego lee las **primeras n entradas** (n = plantilla); si una es 0xFF sale un jugador en blanco «0».
Formatos válidos: compacto (n + 0xFF), identidad (0…39) y «0…n−1, FF, n…». El que llega entra como **última reserva**; si se va un
titular, una reserva ocupa su puesto. **Bloque 627 = el equipo del usuario**: lleva su ID (-11), de técnico el nombre del mánager, y
su orden es el de verdad (§E). El 628 es su reserva (orden identidad) y el bloque del índice del club del usuario (154 en la
referencia) es la copia «como equipo de la IA» (ID real 0xAD, orden identidad): no se usan.

## E. El equipo del usuario: tablas, alineación y contratos

| Qué | Cómo se localiza | Estado |
|---|---|---|
| **12 tablas** con un registro por jugador (A, A2, B, C, D, E, F, G, H×3, M, I, J) | por ancla: los 5 primeros `(reg, pid)` de la plantilla a paso fijo, **saltando huecos** y fichas de gente que ya no está (hasta 8); pasos 24, 24, 24↓, 44, 368, 192, 52, 108×3, 5628, 48, 16 | ✅ |
| Cómo las usa el juego | **por (reg, pid), no por índice**: admite huecos en medio; los fichajes nuevos van al final (en D, tras los 32 juveniles) | ✅ |
| Registro de cada tabla | empieza 4 B antes de `reg` (`[x][reg][pid]…`) salvo contratos (28 B antes); vacío = reg 0/0xFFFF con pid 0 | ✅ |
| **Qué hace el juego cuando un jugador se va** | **hueco** en A, B, C, D, E, F, J, M (A: fecha vacía 0xFFFF en +16; B conserva +10; F +40 a 0; M también 0 en la palabra anterior al siguiente `reg`); **compacta** G, H y contratos; **no toca** A2 ni K al momento | ✅ (réplica idéntica byte a byte) |
| A2 | a 976 B de A; su campo anterior a `reg` es un **float** (valoración del último partido). **Vacía (0xFFFF) en una carrera recién empezada**: el programa no la exige | 🔎 |
| Ficha del último partido | 16 B, solo los que jugaron: **no es tabla de plantilla**, no se toca | ✅ |
| **Orden de formación** (40 B) + **roles** (6 B) | bloque 627 de alineaciones (+0x220 / +0x248): el primer bloque con el ID del usuario | ✅ |
| **Lista K** (Estrategia) | 16 B por puesto `[flag, reg, pid, 0]`; K0 flag 0, usados 0xC0–0xC6 (flags por jugador: p. ej. 0xC4), libre 0xC0/0xC1, después 0xC7. Se localiza por estructura; puede estar **atrasada** respecto al orden (el juego la rehace después) | ✅ |
| **ID interno de club** | `(ID option << 14) \| índice del bloque` (City 0x2B409A = 173, 154); el bloque del usuario lleva -11 en vez del ID | ✅ |
| **Contratos** (tabla I, 48 B; el registro empieza **28 B antes** de `reg`) | `[índice u8][ID club][tipo u8][inicio][-1][-1] \| [x u16][reg][pid][sueldo/100][cláusula/100][fin]` · tipo **5** = contrato vigente · tipo **3** = **oferta** de otro club (sin inicio) | ✅ (cifras = pantalla) |
| Contratos dobles | un jugador con ofertas tiene varios registros (5 + 3…); al salir se borran **todos** | ✅ |
| **Negociaciones abiertas** (60 B, `reg` en +32) | `[estado u16][ID club que oferta][banderas][0xFFFF][-1][-1][0xFFFF] \| [ID club del jugador][reg][pid][monto/100][monto/100][0xFFFF][0xFFFF][0]`; vacío = -1/0xFFFF; al salir el jugador se borra y se compacta | ✅ |

## F. Dinero del usuario (todo en u32 × 100 €)

| Campo | Dónde | Estado |
|---|---|---|
| **Presupuesto de fichajes actual** | 0xC7DD00 | ✅ (500 M visto en pantalla) |
| Presupuesto de fichajes inicial de temporada | 0xC7DD10 | 🔎 |
| **Tope salarial** | 0xC7DD14 | ✅ |
| Presupuesto salarial en pantalla | = tope − Σ sueldos (tabla I, un contrato por jugador) | ✅ (dos comprobaciones exactas) |
| Cómo lo localiza el programa | **por ancla**: el bloque está a 0x97D38 de la tabla A del usuario (estructura fija del motor, vista igual en 6 guardados, también en una carrera recién empezada) y se valida contra los contratos (tope ≥ Σ sueldos) | ✅ (`finanzas` / `fijarFinanzas` en C++) |
| Clubes de la IA | **no tienen presupuesto** guardado; fichan por reglas | ❌ |

## G. Lo que NO está donde se esperaba

- Valor de mercado: no está en la ficha de 596 B; **sí** en la ficha de 156 B del blob (§H).
- Habilidades de los jugadores en la LM: **no** están en el blob (se buscó con el mapa de bits de Player.bin y por fuerza bruta).
- Calendario de partidos: **no** aparece como rondas de pares de equipos ni como rejilla (se buscó exhaustivamente). Pista: los 10 IDs
  de partido por jornada del calendario del usuario. Experimento pendiente: guardar justo antes y justo después de un partido.

## H. El blob comprimido (zlib)

| Qué | Dato | Estado |
|---|---|---|
| Formato | desde 0x11403AC: 28 B de cabecera (`0x0007458c`…; +0x14 y +0x18 = sumas BE de los tramos) y **18 tramos zlib**, cada uno con separador `[descomprimido BE][comprimido BE][fin BE]` (fin = rel + comprimido; 0 en el último) | ✅ |
| Palabra de tamaño (0x11403A8) | tamaño de la zona **menos 12** (el último tramo termina 12 B después) | ✅ |
| Tamaño descomprimido | **4.680.156 B, fijo** | ✅ |
| **Escribirlo** | se recomprimen solo los tramos cambiados (miniz nivel 9) y se rehacen separadores, sumas y tamaño; el archivo cambia de tamaño y lo de detrás se desplaza | ✅ **probado en el juego** (prueba 17: valor de Sommer 77,7 M en pantalla) |
| **Fichas de LM por jugador** | en `0x1E + 156·reg` (una por `reg`, 16.422) | ✅ |
| · competiciones inscritas | +10: 12 pares (id u16, banderas u16), libre = 0xFFFF; al fichar se añaden las del club | ✅ |
| · 6 B que van a la tabla A al fichar | +0x4A | ✅ |
| · sueldo anual /100 | +0x56 (u32) | ✅ (el juego crea el contrato de la tabla I desde aquí) |
| · **valor de mercado /100** | +0x5A (u32) | ✅ (escrito y visto en pantalla) |
| · club anterior, fin e inicio del contrato | +0x5E, +0x66, +0x6A | ✅ |
| · ¿experiencia/crecimiento? | +0x74 (u16), sube ~+11 por partido jugado | ❓ |
| · ¿condición? | +0x7A (u8), 100 al inicio de temporada | ❓ |
| Después de las fichas | registros de temporada por equipo con **puntos de liga** (+3 al ganar) y listas de inscritos por competición | 🔎 |
| Fecha actual de la partida | **0xACC61C** (u32 fecha), con `[día del año][año][365]` delante | ✅ |

## H2. Fichar PARA el equipo del usuario (IA → usuario) — ✅ PROBADO EN EL JUEGO (pruebas 18 y 19)

Comparado con el fichaje de Sommer hecho por el propio juego y luego probado jugando un partido:
- Plantillas: sale del club de la IA (lista compactada, su alineación con sustituto) y entra al final del equipo del usuario.
- Dorsal: el pedido → el de su club → el de su selección → el más alto libre (Sommer: el 1 ocupado → 12, el de Suiza).
- Tablas del usuario: registro nuevo en el primer hueco de cada una, copiando el de un compañero del mismo puesto; datos propios en A
  (6 B de la ficha + fecha de llegada), E (edad + historial de medias, que el juego inventa al fichar: 2 valores por año desde los 9),
  F (contador a 0), M (club, fechas, club de origen, bit 0x80000 «fichado esta temporada»).
- Orden de formación +n y lista K con el flag del libre. Dinero: se descuenta el monto; el tope salarial nunca por debajo de los sueldos.
- **Contrato: NO se escribe en la tabla I** — el juego lo crea solo desde la ficha del blob (con su cláusula). Si se escribe, queda duplicado.
- Blob: competiciones del club, sueldo, fechas y club anterior.
- Resultado en el juego: Sommer jugó de titular, un solo contrato, presupuesto salarial positivo.
- El juego **relee el archivo al cargar la ranura**: se puede cambiar un guardado con el juego abierto si esa ranura no está cargada.

## I. Qué hace el juego por su cuenta (para no pelear con él)

- Tras un partido reescribe: cabecera 0x30, la zona posterior al blob (contratos, calendario, historial), fichas de 156 B de los
  que jugaron, puntos de liga; **no** corrige plantillas ni alineaciones editadas.
- Al avanzar el día hace fichajes de la IA (cientos de cambios de plantilla en el último día de mercado).
- Al salir un jugador del usuario: borra sus contratos y negociación, vacía o compacta sus registros en las tablas (ver §E) y pone a un
  suplente en su puesto; deja K y A2 para más tarde; anota noticia y pago en caja; recalcula las medias del equipo (bloque +0x5DA).
- Crea jugadores propios (regens, `reg 0xDB65xxxx`) y los reparte entre los clubes de la IA a lo largo de la temporada.

## J. `Player.bin` (base de datos) — mapa ampliado (8 oct, 22:10)

Registro de 312 B; **NO** tiene el mismo orden que el jugador editado del option file (ese sí sigue el mapa de 4ccEditor, licencia zlib:
`pes20.cpp`, `fill_player_entry20`). Posiciones en bits desde el inicio del registro (`id` u32 en +8):

| Campo | Bits | Estado |
|---|---|---|
| 19 habilidades de campo (6 bits, valor − 40) | ver §B | ✅ |
| **Potencia de tiro** | 358 (6, −40) | ✅ (2026-10-09: Lamine 78 en la pantalla del juego = valor leído; las 15 cualidades visibles coinciden) |
| **5 habilidades de portero** | **269 = Despejar (PT)** ✅ · **300 = Cobertura (PT)** ✅ · **364 = Atajar (PT)** ✅ · 320 y 326 = Actitud de portero y Reflejos (PT), falta saber cuál es cuál (6, −40) | ✅/🔎 (2026-10-09, Joan García en el juego: Actitud 92, Atajar 90, Despejar 89, Reflejos 92, Cobertura 91 = 320:92, 364:90, 269:89, 326:92, 300:91. Se resuelve con Livakovic, que tiene 320=84 y 326=82) |
| **Posiciones jugables** (2 bits: 0 no, 1 B, 2 A) | PT 350 · DFC 468 · LI 318 · LD 474 · MCD 414 · MC 456 · MI 466 · MD 460 · MO 464 · EI 472 · ED 476 · SD 478 (probable) · DC 470 | 🔎 (la posición registrada vale 2 en el 99 %) |
| Pie malo uso / precisión, forma («Regularidad»), resistencia a lesiones | 454 (2) / 462 (2), 438 (3), 458 (2). **Lo que muestra el juego = valor guardado + 1** | ✅ (2026-10-09, Joan García: guardado 1/1/3/1 → juego 2/2/4/2) |
| Habilidades especiales + estilos COM | bits 480–531 (52 bits) | ❓ en calibración (490, 496, 502, 522 solo en porteros: despejes/saques/penales de portero) |
| País | 233 (9) → `Country.bin` (1420 B por país: ID en los bits 10–18, nombre en español en +148) | ✅ |

**Hallazgo importante:** en ConmeGOL 26 la base que usa el juego **no es** `CGP_database.cpk` (19/8) sino la que sirve Sider desde
`SiderAddons\olmosjr23\Database\common\etc\pesdb\` (5/9): **5.894 jugadores distintos y 64 cambiados**. El catálogo del Mercado
se generó con la base vieja → hay que regenerarlo desde la de Sider.

**Imágenes:** minifaces en `livecpk\Logos\common\render\symbol\player\<pid>.dds` (2.638; DXT, 180×180) y escudos en
`…\symbol\flag\e_<6 dígitos del ID>_r_l.png` (Barça: `e_000108_r_l.png`).

**Guardado recién creado:** una carrera guardada en cuanto se crea todavía no tiene fecha (1/1, día 0), ni alineación, ni contratos
definitivos: el juego los rellena al empezar la temporada.
