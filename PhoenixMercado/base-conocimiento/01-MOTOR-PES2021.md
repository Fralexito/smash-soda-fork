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

**Error conocido (pendiente de corregir):** al mover un jugador solo en la plantilla, la alineación del equipo que lo pierde queda
corrida (en un amistoso el portero suplente puede salir de delantero). Hay que actualizar el bloque de tácticas igual que en la LM.

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
| Tabla de clubes (16 B) | 0xC016C0, 263 registros | equipo u16 en +8 y un valor decreciente en +12/+14 (¿prestigio?) | ❓ |
| **Blob comprimido** | cabecera 0x1140364, zlib desde 0x11403D4 | ver §H | 🔎 |
| Contratos (tabla I) | tras el blob (se desplaza con su tamaño) | ver §E | ✅ |
| Historial de traspasos | ~0x12B430F, registros de 36 B | jugador, monto, fecha | 🔎 |

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
titular, una reserva ocupa su puesto. Bloque 627 = el equipo del usuario (su orden es el de §E); 628 y el bloque original del club
del usuario no se mantienen.

## E. El equipo del usuario: tablas, alineación y contratos

| Qué | Cómo se localiza | Estado |
|---|---|---|
| **12 tablas alineadas** con la plantilla (A, A2, B, C, D, E, F, G, H, M, I, J y L) | por ancla: los 5 primeros `(reg, pid)` de la plantilla a paso fijo; pasos 24, 24, 24↓, 44, 368, 192, 52, 108×3, 5628, 48, 16 | ✅ |
| Registro de cada tabla | empieza 4 B antes de `reg`: `[x][reg][pid]…`; vacío = reg 0/0xFFFF con pid 0 | ✅ |
| Fichajes recientes en C y D | van **al final**, tras 32 juveniles (no en su índice): buscar por `(reg, pid)` | ✅ |
| Ficha del último partido | 16 B, solo los que jugaron: **no es tabla de plantilla**, no se toca | ✅ |
| **Orden de formación** (40 B) + **roles** (6 B) | permutación de 0…n−1 + relleno 0xFF, con roles < n, cruzado con K | ✅ |
| **Lista K** (Estrategia) | 16 B por puesto `[flag, reg, pid, 0]` espejo del orden; K0 flag 0, usados 0xC0; libre 0xC0 (25) / 0xC1 (26); después 0xC7 | ✅ |
| **Tabla I = contratos** (paso 48) | tras `[x][reg][pid]`: +0 sueldo anual/100 · +4 cláusula/100 · +8 fin (año u16, mes, día) · +0x14 nº único · +0x18 tipo (18/22/154 vigente; 125/255 **oferta abierta**) · +0x1C inicio | ✅ (cifras = pantalla) |
| Contratos dobles | un jugador con oferta abierta tiene 2 registros; al salir, el juego borra **los dos** y la negociación | ✅ (visto al despedir) |
| Lista de ofertas/negociaciones | tras el blob (0x1279F74 en el guardado de referencia): registros con club, jugador y club que oferta | 🔎 |

## F. Dinero del usuario (todo en u32 × 100 €)

| Campo | Dónde | Estado |
|---|---|---|
| **Presupuesto de fichajes actual** | 0xC7DD00 | ✅ (500 M visto en pantalla) |
| Presupuesto de fichajes inicial de temporada | 0xC7DD10 | 🔎 |
| **Tope salarial** | 0xC7DD14 | ✅ |
| Presupuesto salarial en pantalla | = tope − Σ sueldos (tabla I, un contrato por jugador) | ✅ (dos comprobaciones exactas) |
| Clubes de la IA | **no tienen presupuesto** guardado; fichan por reglas | ❌ |

## G. Lo que NO está donde se esperaba

- Valor de mercado: no está en la ficha de 596 B; **sí** en la ficha de 156 B del blob (§H).
- Habilidades de los jugadores en la LM: **no** están en el blob (se buscó con el mapa de bits de Player.bin y por fuerza bruta).
- Calendario de partidos: **no** aparece como rondas de pares de equipos ni como rejilla (se buscó exhaustivamente). Pista: los 10 IDs
  de partido por jornada del calendario del usuario. Experimento pendiente: guardar justo antes y justo después de un partido.

## H. El blob comprimido (zlib)

| Qué | Dato | Estado |
|---|---|---|
| Formato | 18 tramos zlib de 256 KB descomprimidos, separados por 12 B (`00 04 00 00`, tamaño comprimido, acumulado); palabra de tamaño total en 0x11403A8 | ✅ (herramienta `blob.py`) |
| Tamaño descomprimido | **4.678.743 B, fijo** | ✅ |
| **Fichas de LM por jugador** | desde +0x1E, **16.422 registros de 156 B**, uno por `reg` | 🔎 |
| · competiciones inscritas | pares (id u16, banderas u16) tras `[u16][reg][pid]`, relleno 0xFFFF | 🔎 |
| · sueldo anual /100 | +0x56 (u32) | ✅ (= tabla I) |
| · **valor de mercado /100** | +0x5A (u32) | ✅ (= pantalla) |
| · nº único del contrato, fin e inicio | +0x62, +0x66, +0x6A | ✅ |
| · ¿valoración media ×10? | +0x4B (u8) | ❓ |
| · ¿experiencia/crecimiento? | +0x74 (u16), sube ~+11 por partido jugado | ❓ |
| · ¿condición? | +0x7A (u8), 100 al inicio de temporada | ❓ |
| · estadísticas de temporada | +0x16…+0x29 (cambian en ~1.300 jugadores por fecha) | ❓ |
| Después de las fichas | registros de temporada por equipo con **puntos de liga** (+3 al ganar) y listas de inscritos por competición | 🔎 |
| Para escribirlo | recomprimir por tramos y actualizar la palabra de tamaño y las cabeceras de tramo | ❓ (no hecho) |

## I. Qué hace el juego por su cuenta (para no pelear con él)

- Tras un partido reescribe: cabecera 0x30, la zona posterior al blob (contratos, calendario, historial), fichas de 156 B de los
  que jugaron, puntos de liga; **no** corrige plantillas ni alineaciones editadas.
- Al avanzar el día hace fichajes de la IA (cientos de cambios de plantilla en el último día de mercado).
- Al salir un jugador del usuario: borra sus contratos y negociación, vacía o compacta sus registros en las tablas y pone a un
  suplente en su puesto; deja K y A2 para más tarde.
