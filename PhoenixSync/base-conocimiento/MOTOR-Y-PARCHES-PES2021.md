# Base de conocimiento · Motor de PES 2021, parches, ligas e instalador de Sider

Lo que se descubrió del motor de PES 2021 y de los parches (ConmeGOL 26/27):
- más equipos por liga;
- nuevas ligas y competiciones;
- cómo define el motor las ligas;
- la estrategia del Parche Phoenix;
- la guía multiparche;
- el informe completo del instalador de Sider para Link.

Cada parte está en el orden de la numeración original (01–32).

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 13 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · 01 · El motor de PES 2021 (vale para cualquier parche)](#parte-1-01-el-motor-de-pes-2021-vale-para-cualquier-parche) — origen `PhoenixSync/base-conocimiento/01-MOTOR-PES2021.md` (76d506b 2026-10-09)
- [Parte 2 · 02 · Hacia el parche Phoenix (parche propio, vinculado a la web, a Phoenix Link y al Mercado)](#parte-2-02-hacia-el-parche-phoenix-parche-propio-vinculado-a-la-web-a-phoenix-link-y-al-mercado) — origen `PhoenixSync/base-conocimiento/02-PARCHE-PHOENIX.md` (f75f677 2026-10-09)
- [Parte 3 · 03 · Lo que enseñan los demás parches (y cómo hacer el Phoenix el más completo y sin errores)](#parte-3-03-lo-que-ensenan-los-demas-parches-y-como-hacer-el-phoenix-el-mas-completo-y-sin-errores) — origen `PhoenixSync/base-conocimiento/03-INVESTIGACION-PARCHES.md` (f75f677 2026-10-09)
- [Parte 4 · 04 · Informe estratégico: ConmeGOL y Sudamerican por dentro, y cómo superarlos](#parte-4-04-informe-estrategico-conmegol-y-sudamerican-por-dentro-y-como-superarlos) — origen `PhoenixSync/base-conocimiento/04-ESTRATEGIA-PHOENIX.md` (f75f677 2026-10-09)
- [Parte 5 · 06 · La ruta del descubrimiento (noche del 8 al 9 de octubre de 2026)](#parte-5-06-la-ruta-del-descubrimiento-noche-del-8-al-9-de-octubre-de-2026) — origen `PhoenixSync/base-conocimiento/06-RUTA-DEL-DESCUBRIMIENTO.md` (82116b5 2026-10-09)
- [Parte 6 · 07 · El motor por dentro: recargar sin «Editar → Cargar» y el botón nativo de actualización](#parte-6-07-el-motor-por-dentro-recargar-sin-editar-cargar-y-el-boton-nativo-de-actualizacion) — origen `PhoenixSync/base-conocimiento/07-MOTOR-POR-DENTRO.md` (9e5f289 2026-10-09)
- [Parte 7 · 12 · Guía multiparche: llevar todo lo descubierto a cualquier parche de PES 2021](#parte-7-12-guia-multiparche-llevar-todo-lo-descubierto-a-cualquier-parche-de-pes-2021) — origen `PhoenixSync/base-conocimiento/12-GUIA-MULTIPARCHE.md` (50b9c93 2026-10-09)
- [Parte 8 · 19 · Investigación: ¿se puede aumentar el número de equipos de una liga en PES 2021?](#parte-8-19-investigacion-se-puede-aumentar-el-numero-de-equipos-de-una-liga-en-pes-2021) — origen `PhoenixSync/base-conocimiento/19-INVESTIGACION-MAS-EQUIPOS-POR-LIGA.md` (8f50132 2026-10-10)
- [Parte 9 · 21 · Investigación: ¿se pueden añadir MÁS LIGAS y MÁS COMPETICIONES a PES 2021? (por separado)](#parte-9-21-investigacion-se-pueden-anadir-mas-ligas-y-mas-competiciones-a-pes-2021-por-separado) — origen `PhoenixSync/base-conocimiento/21-INVESTIGACION-NUEVAS-LIGAS-Y-COMPETICIONES.md` (8f50132 2026-10-10)
- [Parte 10 · 22 · Ficha del parche ConmeGOL 26 y plantilla para comparar parches](#parte-10-22-ficha-del-parche-conmegol-26-y-plantilla-para-comparar-parches) — origen `PhoenixSync/base-conocimiento/22-FICHA-PARCHE-CONMEGOL-26-Y-PLANTILLA-DE-COMPARACION.md` (8f50132 2026-10-10)
- [Parte 11 · 23 · Guía: cómo el motor de PES 2021 define ligas y competiciones, y cómo se podría añadir más](#parte-11-23-guia-como-el-motor-de-pes-2021-define-ligas-y-competiciones-y-como-se-podria-anadir-mas) — origen `PhoenixSync/base-conocimiento/23-GUIA-COMO-EL-MOTOR-DEFINE-LIGAS-Y-COMPETICIONES.md` (8f50132 2026-10-10)
- [Parte 12 · 27 · Informe completo: módulos Lua de Sider, carpetas y sider.ini, para construir el instalador de Phoenix Link](#parte-12-27-informe-completo-modulos-lua-de-sider-carpetas-y-siderini-para-construir-el-instalador-de-phoenix-link) — origen `PhoenixSync/base-conocimiento/27-INFORME-INSTALADOR-SIDER.md` (712d3c0 2026-10-10)
- [Parte 13 · 32 · Idea: un "mundo compartido" entre varias Ligas Máster conectadas](#parte-13-32-idea-un-mundo-compartido-entre-varias-ligas-master-conectadas) — origen `PhoenixSync/base-conocimiento/32-IDEA-MUNDO-COMPARTIDO-ENTRE-LIGAS-MASTER.md` (4353058 2026-10-10)

---

## Parte 1 · 01 · El motor de PES 2021 (vale para cualquier parche)

> Origen: `PhoenixSync/base-conocimiento/01-MOTOR-PES2021.md` · último cambio 76d506b 2026-10-09 · 17130 bytes. Copia íntegra.

Versión del juego: eFootball PES 2021 SEASON UPDATE (PC/Steam). Todo lo de abajo se descubrió sobre el ConmeGOL Patch 26, pero
la **forma** de los archivos la pone el juego, no el parche: el parche solo cambia el contenido (jugadores, clubes, competiciones).
Lo que dependa del parche está en `parches/`.

Las posiciones (offsets) se dan en hexadecimal y son las del guardado de referencia. Todo lo que el programa usa de verdad lo
localiza **por anclas** (buscando la forma de los datos), no por posición fija.

---

### A. Cifrado y envoltura (option file y guardados)

| Qué | Dato | Estado |
|---|---|---|
| Cifrado | libpesXcrypter con la clave de PES 2021; se descifra y se vuelve a cifrar **idéntico** | ✅ |
| Envoltura | cabecera + descripción (384 B) + logo (17.315 B) + DATOS + serial (44 B) | ✅ |
| Descripción | bytes 0–127: nombre (el menú Cargar NO lo muestra) · 128–383: «texto info» (lo que SÍ muestra el menú) | ✅ |
| Hash de cabecera | el juego **no lo valida**: acepta archivos editados | ✅ |
| Tamaño de datos variable | el guardado de LM crece/encoge; el C++ re-cifra con cualquier tamaño | ✅ |
| Ranuras del menú Cargar | ranura N = archivo `ML0000000(N−1)` **en hexadecimal** (ranura 11 = `ML0000000A`) | ✅ |
| Autoguardado | al terminar un partido el juego **sobrescribe la ranura cargada**; identificar archivos por huella (md5) | ✅ |

### B. Option file (`EDIT00000000`): amistosos, salas y carreras nuevas

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

### C. Guardado de Liga Máster (`ML0000000N`): estructura general

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

### D. Alineación de los equipos de la IA (bloque de 600 B)

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

### E. El equipo del usuario: tablas, alineación y contratos

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

### F. Dinero del usuario (todo en u32 × 100 €)

| Campo | Dónde | Estado |
|---|---|---|
| **Presupuesto de fichajes actual** | 0xC7DD00 | ✅ (500 M visto en pantalla) |
| Presupuesto de fichajes inicial de temporada | 0xC7DD10 | 🔎 |
| **Tope salarial** | 0xC7DD14 | ✅ |
| Presupuesto salarial en pantalla | = tope − Σ sueldos (tabla I, un contrato por jugador) | ✅ (dos comprobaciones exactas) |
| Cómo lo localiza el programa | **por ancla**: el bloque está a 0x97D38 de la tabla A del usuario (estructura fija del motor, vista igual en 6 guardados, también en una carrera recién empezada) y se valida contra los contratos (tope ≥ Σ sueldos) | ✅ (`finanzas` / `fijarFinanzas` en C++) |
| Clubes de la IA | **no tienen presupuesto** guardado; fichan por reglas | ❌ |

### G. Lo que NO está donde se esperaba

- Valor de mercado: no está en la ficha de 596 B; **sí** en la ficha de 156 B del blob (§H).
- Habilidades de los jugadores en la LM: **no** están en el blob (se buscó con el mapa de bits de Player.bin y por fuerza bruta).
- Calendario de partidos: **no** aparece como rondas de pares de equipos ni como rejilla (se buscó exhaustivamente). Pista: los 10 IDs
  de partido por jornada del calendario del usuario. Experimento pendiente: guardar justo antes y justo después de un partido.

### H. El blob comprimido (zlib)

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

### H2. Fichar PARA el equipo del usuario (IA → usuario) — ✅ PROBADO EN EL JUEGO (pruebas 18 y 19)

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

### I. Qué hace el juego por su cuenta (para no pelear con él)

- Tras un partido reescribe: cabecera 0x30, la zona posterior al blob (contratos, calendario, historial), fichas de 156 B de los
  que jugaron, puntos de liga; **no** corrige plantillas ni alineaciones editadas.
- Al avanzar el día hace fichajes de la IA (cientos de cambios de plantilla en el último día de mercado).
- Al salir un jugador del usuario: borra sus contratos y negociación, vacía o compacta sus registros en las tablas (ver §E) y pone a un
  suplente en su puesto; deja K y A2 para más tarde; anota noticia y pago en caja; recalcula las medias del equipo (bloque +0x5DA).
- Crea jugadores propios (regens, `reg 0xDB65xxxx`) y los reparte entre los clubes de la IA a lo largo de la temporada.

### J. `Player.bin` (base de datos) — mapa ampliado (8 oct, 22:10)

Registro de 312 B; **NO** tiene el mismo orden que el jugador editado del option file (ese sí sigue el mapa de 4ccEditor, licencia zlib:
`pes20.cpp`, `fill_player_entry20`). Posiciones en bits desde el inicio del registro (`id` u32 en +8):

| Campo | Bits | Estado |
|---|---|---|
| 19 habilidades de campo (6 bits, valor − 40) | ver §B | ✅ |
| **Potencia de tiro** | 358 (6, −40) | ✅ (2026-10-09: Lamine 78 en la pantalla del juego = valor leído; las 15 cualidades visibles coinciden) |
| **5 habilidades de portero** | **269 = Despejar · 300 = Cobertura · 320 = Reflejos · 326 = Actitud de portero · 364 = Atajar** (6 bits, −40) | ✅ (2026-10-09: Joan García 89/91/92/92/90 y Livakovic 77/86/84/82/75 coinciden con las pantallas del juego) |
| **Posiciones jugables** (2 bits: 0 no, 1 B, 2 A) | PT 350 · DFC 468 · LI 318 · LD 474 · MCD 414 · MC 456 · MI 466 · MD 460 · MO 464 · EI 472 · ED 476 · SD 478 (probable) · DC 470 | 🔎 (la posición registrada vale 2 en el 99 %) |
| Pie malo uso / precisión, forma («Regularidad»), resistencia a lesiones | 454 (2) / 462 (2), 438 (3), 458 (2). **Lo que muestra el juego = valor guardado + 1** | ✅ (Joan 1/1/3/1 → 2/2/4/2; Livakovic 0/1/5/2 → 1/2/6/3) |
| Habilidades especiales + estilos COM | bits 480–531 (1 bit cada una). **496 = Patadón en largo (PT)** ✅ (Joan: solo 496; Livakovic: 496). **502 y 522 = Pase largo portero y Parapenaltis** (Livakovic tiene los dos; el orden se resuelve con Szczęsny, que tiene 502 sin 522). Szczęsny además: 486, 490 | ✅/🔎 en calibración |
| País | 233 (9) → `Country.bin` (1420 B por país: ID en los bits 10–18, nombre en español en +148) | ✅ |

**Hallazgo importante:** en ConmeGOL 26 la base que usa el juego **no es** `CGP_database.cpk` (19/8) sino la que sirve Sider desde
`SiderAddons\olmosjr23\Database\common\etc\pesdb\` (5/9): **5.894 jugadores distintos y 64 cambiados**. El catálogo del Mercado
se generó con la base vieja → hay que regenerarlo desde la de Sider.

**Imágenes:** minifaces en `livecpk\Logos\common\render\symbol\player\<pid>.dds` (2.638; DXT, 180×180) y escudos en
`…\symbol\flag\e_<6 dígitos del ID>_r_l.png` (Barça: `e_000108_r_l.png`).

**Guardado recién creado:** una carrera guardada en cuanto se crea todavía no tiene fecha (1/1, día 0), ni alineación, ni contratos
definitivos: el juego los rellena al empezar la temporada.

---

## Parte 2 · 02 · Hacia el parche Phoenix (parche propio, vinculado a la web, a Phoenix Link y al Mercado)

> Origen: `PhoenixSync/base-conocimiento/02-PARCHE-PHOENIX.md` · último cambio f75f677 2026-10-09 · 6559 bytes. Copia íntegra.

**Objetivo declarado (8 oct 2026):** sacar un parche propio, el más ambicioso posible, que sea **uno solo con el ecosistema**:
la web (liga, mercado, perfiles), Phoenix Link (salas) y Phoenix Sync (lo que entra al juego). Todo lo que se descubre
auditando ConmeGOL y otros parches alimenta este plan. Esta página es el **mapa de lo que hay que dominar** y el estado real
de cada pieza. Lo marcado ❓ está por verificar: no se da por sabido.

### A. De qué está hecho un parche de PES 2021 (anatomía)

| Pieza | Dónde vive | Qué contiene | Cómo se fabrica / edita | Estado nuestro |
|---|---|---|---|---|
| **Archivos CPK** | `<juego>\download\*.cpk` | paquetes CRI con todo el contenido: base de datos, caras, uniformes, estadios, balones, botines, logos, menús | se arman con herramientas CPK (CriPakTools / CPK File Builder); el juego los carga por `DpFileList.bin` | leemos la base (`.cpk` → `Player.bin`) ✅; **no armamos CPK todavía** ⏳ |
| **`DpFileList.bin`** | `<juego>\download\` | lista de CPK que el juego carga y en qué orden (el último pisa a los anteriores) | «DpFileList Generator» o generador propio | 🔎 formato leído del ConmeGOL (ver `parches/conmegol-26.md`): 16 B + 48 B por CPK |
| **Base de datos** (`common\etc\pesdb\`) | dentro del CPK de base (ConmeGOL: `CGP_database.cpk`) | `Player.bin` (jugadores y habilidades, mapa de bits), `Team.bin`, `PlayerAssignment.bin` (plantillas), `Competition*.bin`, `Coach.bin`, `Stadium.bin`, `Ball.bin`, `Boots.bin`… | editores de base (PES editor / 4ccEditor) o escritura directa con el mapa de bits | `Player.bin` ✅ (mapa de bits en 01 §B); el resto ❓ |
| **Option file** (`EDIT00000000`) | carpeta `save` de Steam | jugadores/equipos editados, plantillas, dorsales, tácticas, uniformes editados, competiciones editadas | el juego (modo Editar) o Phoenix Sync | plantillas, dorsales y tácticas ✅; uniformes/competiciones ❓ |
| **Caras y cuerpos** | CPK (`common\character0\model\character\face\real\<id>\`) | modelos `.fmdl`, texturas `.ftex`, `face.fpk` | Facemaker / Blender + herramientas de la comunidad | solo se reutilizan (viajan con el ID del jugador) |
| **Uniformes** | CPK (`…\uniform\team\<id>\`) + `kit_config` | texturas y configuración por equipo; también vía Sider (kserv) | Kit Studio / plantillas PSD + `kit_config` | ❓ |
| **Estadios, balones, botines, guantes, adboards, marcadores** | CPK | modelos y texturas; muchos se sirven mejor por **Sider** (stadium server, ball server…) | herramientas de la comunidad | ❓ |
| **Logos, banderas, menús** | CPK (`Asset\...`, `common\menu\...`) | emblemas de equipo y competición, fondos, menú | editores de imágenes + empaquetado CPK | ❓ rutas exactas por confirmar |
| **Sider** (juce66) | carpeta del juego: `sider.exe`, `sider.ini`, `modules\*.lua` | carga contenido suelto sin reempaquetar (**LiveCPK**: `content\live-cpk\`), módulos Lua (uniformes, estadios, marcadores, cámara, **jugabilidad**), opción `save.folder` (7.4.0) | se escribe en Lua con la API de Sider | se usará como vehículo ✅ (decidido); módulos propios ⏳ |
| **Guardado de Liga Máster** (`ML0000000N`) | carpeta `save` | la carrera: 700 equipos, tablas del usuario, alineaciones, contratos, dinero, blob (calendario/resultados) | Phoenix Sync | ampliamente documentado en 01 §C–§I ✅ |

### B. Lo que el ecosistema ya tiene y encaja

| Pieza | Estado |
|---|---|
| La web guarda la verdad de la liga (clubes, plantillas, traspasos) y la **firma** (Ed25519) | ✅ (docs/mercado-api.md) |
| Option file **oficial** publicado por el staff, con huella SHA-256 (`/option/actual`) y descarga verificada | ✅ (`PhoenixSync bajar`) |
| Lista de cambios de liga (`/liga/cambios`), firmada, con versión incremental | ✅ web · ✅ Mercado (`sincronizar`, 8 oct) |
| Aplicar los cambios al **option file** (amistosos/salas) con alineación correcta | ✅ (probado en archivo; prueba en juego pendiente) |
| Aplicar los cambios a la **Liga Máster** del usuario: usuario→IA e IA→IA | ✅ (probado en juego: ranuras 9, 10, 13) |
| Aplicar a la Liga Máster: IA→usuario y agentes libres | ⏳ (quedan como «pendientes» en el informe hasta tener la referencia del juego) |
| Interruptores: option file / Liga Máster / ambos | ✅ (`Alcance`) |
| Presupuesto del club del usuario desde fuera (p. ej. el que fija la web) | ✅ (`fijarFinanzas`) |
| Catálogo del parche (jugadores, posiciones, habilidades) subido a la web | ✅ |
| Equivalencias entre parches (jugar con otro parche sin romper la liga) | ✅ (emparejamiento + huella) |

### C. Lo que falta dominar para un parche propio (por orden sugerido)

1. **Empaquetar CPK y generar `DpFileList.bin`** desde Phoenix Sync (o un script). `DpFileList.bin` ya está entendido 🔎; el CPK se puede **evitar casi del todo con LiveCPK de Sider** (así hace ConmeGOL: solo su base va en un `.cpk` de 3,7 MB; el resto, carpetas sueltas). ❓ armar el `.cpk` de la base
2. **Escribir la base de datos** (`Team.bin`, `PlayerAssignment.bin`, `Competition*.bin`) y no solo leer `Player.bin`:
   crear/renombrar equipos, cambiar plantillas de fábrica, competiciones y calendarios base. ❓ (formatos por documentar;
   el mapa de bits de `Player.bin` ya está).
3. **Uniformes y logos** con `kit_config` y emblemas: el mínimo visual para que un club de la liga «exista». ❓
4. **Sider**: módulos propios (p. ej. cargar el option/plantillas de la sala en caliente, marcador con el nombre de la liga,
   `save.folder` por usuario para el «switcher»). ⏳
5. **Distribución y actualización**: el parche se baja/actualiza desde la web igual que el option oficial (huella SHA-256,
   descarga verificada, nunca se sobrescribe sin copia). ⏳ (la mecánica ya existe para el option).
6. **Ligas propias**: fuera de la Liga Máster primero (competiciones del option file / base), dentro después (01 §C/§I). ❓

### D. Qué auditar en cada parche que se estudie (para copiar lo bueno)

- Qué CPK trae y en qué orden los carga (`DpFileList.bin`).
- Cómo organiza la base (`pesdb`), cuántos jugadores/equipos/competiciones, qué IDs usa (rangos libres para crear).
- Si usa Sider y qué módulos (para no reinventarlos).
- Rarezas del option file (jugadores «Player Identidad», equipos vacíos, formatos de tácticas).

**Regla:** cada hallazgo se anota con su estado (✅ 🔎 ❓ ❌) y, si se probó en el juego, con su entrada en `PRUEBAS.md`.

---

## Parte 3 · 03 · Lo que enseñan los demás parches (y cómo hacer el Phoenix el más completo y sin errores)

> Origen: `PhoenixSync/base-conocimiento/03-INVESTIGACION-PARCHES.md` · último cambio f75f677 2026-10-09 · 9183 bytes. Copia íntegra.

Investigación del 8 oct 2026: parches grandes de PES 2021, modo online, herramientas y formatos. Informes completos, con todas las
fuentes, en `investigacion/`:
- `parches-pes2021.md`: ficha de cada parche, las 14 causas de crash más reportadas con su solución y buenas prácticas.
- `modo-online.md`: servidores oficiales, PESBUL, alternativas, Sider para resultados, Parsec desde Perú y arquitectura recomendada.
- `herramientas-y-formatos.md`: cómo fabricar CPK, formatos de la base de datos, kits, caras, option file y memoria (Cheat Engine).

### A. El mapa de los parches en 2026

| Parche | Estilo | Lo que hay que copiar | Lo que hay que evitar |
|---|---|---|---|
| **Football Life** (SmokePatch; FL26 15/10/2025, FL27 a inicios de nov. 2026) | Juego «propio» sobre el motor: exe, lanzador, instalador y switcher propios | Lanzador que comprueba versiones; updates clasificadas según si rompen la carrera; ligas de 30 y Mundial de 48 **parcheando el exe** | Exe modificado sin online (PESBUL no funciona con FL) |
| **Evoweb Patch** (2024 v3.0) | Sider + base de datos | IDs **reales** de Konami para que cualquier facepack encaje | No convive con ninguna otra base de datos |
| **ConmeGOL** (27, sept. 2026) | Casi todo por Sider/livecpk; base en un `.cpk` de 3,7 MB | Base pequeña + contenido suelto (actualizar es copiar carpetas) | AIO de ~234 GB; `sider.ini` con módulos que faltan y pares que se pisan (ver `../sider/AUDITORIA-SIDER.md`) |
| **Sudamerican Project 2026** | «Clásico» de CPK: 34 CPK, 13 de caras de 5–11 GB | Créditos y comunidad (Discord, 30+ colaboradores) | CPK gigantes; módulos cargados dos veces; `netblock.lua` (bloquea el online) |
| **Gogosz** (World 12.5 / CONMEBOL 4.5, oct. 2026) | AIO enorme (~200 GB) | Cobertura de ligas | El tamaño |
| **PESBUL** (beta, jul. 2026) | Servidor de la comunidad que imita al de Konami | Que el online «de verdad» es posible | Solo amistoso 1 vs 1; exige el juego de Steam **sin modificar** |

**Hueco libre:** no existe un parche de PC dedicado a la **Liga 1 de Perú** y la zona andina; hoy solo aparece dentro de ConmeGOL. Ahí entra Phoenix.

### B. Las causas de crash que más se repiten (y la regla Phoenix para cada una)

| Causa | Regla Phoenix |
|---|---|
| Copias repack/modificadas, restos de otros parches, DpFileList para el DLC equivocado | Base única declarada: **PES 2021 de Steam, exe 1.07.02 + Data Pack 7.0**; el lanzador lo comprueba y no arranca si no coincide |
| Mezclar bases de datos (jugadores «sin nombre, media 40») | **Una sola** base; Phoenix Sync escribe solo en la de Phoenix y detecta otras |
| Option file de otra versión | El EDIT lleva su versión; `save.folder` de Sider 7.4.1 para que Phoenix tenga **su propio** EDIT |
| Falta memoria virtual (pantalla negra con muchas caras) | El lanzador revisa el archivo de paginación y avisa; caras en DDS comprimido |
| Módulos Sider viejos, duplicados o que se pisan | **Un solo `sider.ini` limpio**, auditado; Sider **fijado** (7.4.1) y distribuido con el parche |
| Intel 12.ª gen+ (DRM), drivers, antivirus, OneDrive | Lista de comprobación en el lanzador; hashes publicados; nada de `.bat` (lanzador `.exe`) |
| Cambiar el número de equipos de una liga (Evoweb: la Liga Máster se colgaba en la temporada 2) | **No cambiar** el número de equipos de una liga sin parchear el exe; el calendario está dentro del exe |
| Audio/mods de narración incompatibles | Addons con versión; se instalan de a uno y el lanzador comprueba la versión |

Límites documentados de la base de datos (fuente de foro): **750 equipos, 40 jugadores por equipo, 30.000 jugadores**.
(Cuadra con lo visto: 749 equipos en ConmeGOL y 29.997 jugadores en `Player.bin`.)

### C. Modo online: lo realista

- Los servidores oficiales de Konami están **cerrados** (2022). PES 2021 de PC **no tiene modo LAN**: Radmin/ZeroTier/Hamachi solos no sirven.
- **PESBUL**: rehace el lobby de Konami (amistoso 1 vs 1). El partido va directo entre los dos PC. **Exige el juego de Steam sin
  modificar, sin Sider ni mods de jugabilidad**, y abrir los puertos UDP 5739–5740. Las dos carpetas de juego de la PC de Fralex
  no cumplen esas condiciones: llevan Sider con muchos módulos y archivos que no son de la instalación original de Steam
  (`CPY.ini`, `steamclient64.dll`). Es la explicación más probable de que PESBUL no conectara. Su base de código abierto es
  el servidor de **Nikow5** (licencia MIT).
- **Antecedente bonito:** en PES 6 hubo un servidor propio unido a una web de ligas, de código abierto (`kinj1987/evo-league`). Es la
  idea de «Phoenix online» hecha hace años.

**Arquitectura recomendada para Phoenix online (se puede hacer ya):**
1. La **web** crea el partido (liga, rivales).
2. **Phoenix Link** (Parsec) en el PC anfitrión deja entrar solo a esos jugadores y abre el juego con el option file oficial.
3. Un módulo **Sider** lee el marcador con `match.stats()` (marcador, penales, periodo y reloj; **no** da goleadores ni tarjetas) y
   lo deja en un archivo `resultado.json` (el «buzón», al revés que `avisos.txt`).
4. Phoenix Link sube el resultado a la web con una captura automática; los dos jugadores lo confirman.
5. El verificador revisa solo al anfitrión (sus archivos y su option file).

Así no hay desincronización (solo hay un juego corriendo) y no se depende de Konami ni de PESBUL.
**Largo plazo (meses o años, investigación):** servidor propio a partir del de Nikow5; haría falta entender el protocolo y el login de Steam.

**Parsec desde Perú:** el anfitrión necesita ≥ 10 Mbps de subida (30 recomendados); ping bueno < 30 ms, aceptable < 60 ms; ajustes
para fútbol: 60 fps, 720p o 900p, H.265 si todos lo soportan, cable, V-Sync apagado, sin VPN. No funciona detrás de CGNAT (internet móvil).

### D. Herramientas y formatos que ya tenemos documentados para fabricar el parche

- **Crear CPK:** `the4chancup/pes-file-tools` (`cpk.py`, unas 250 líneas): archivos desde 0x800, cada uno alineado a 0x800, sin
  comprimir, ordenados por nombre en mayúsculas; tablas en big-endian cifradas con XOR (empieza en 0x5F, × 0x15 por byte).
  Fácil de pasar a C++ (sin licencia declarada: usar como referencia y escribir código propio).
- **DpFileList.bin:** filas de 48 B con el nombre del CPK en +0x10 (ya entendido en `parches/conmegol-26.md`).
- **WESYS** (envoltorio de los `.bin` de la base): cabecera de 16 B (3 bytes de versión, «WESYS», tamaño comprimido, tamaño original) + zlib.
- **Tamaños de registro** (`pes-db-generator`, GPL-3): Team 1532, Coach 100, PlayerAssignment 16, CompetitionEntry 12, PlayerAppearance 60.
  **Sin documentación pública:** Competition, CompetitionKind, CompetitionRegulation, Stadium, Derby, Tactics, Ball, Boots, Glove →
  hay que descifrarlos comparando antes/después (como hicimos con la Liga Máster).
- **Kits:** TeamColor.bin 16 B por equipo; UniColor.bin 85 B por equipo; UniformParameter documentado; kit config de 0x78 B (de `kitinfo.cpp` de Sider).
- **Caras:** `pes-file-tools` (.fpk/.ftex/.fsop), `pes-fmdl-blender` 0.7.2 (mayo 2026), GzsTool/FtexTool de Atvaark (MIT).
- **Option file:** el mapa de 4ccEditor (C++, licencia zlib) coincide **exactamente** con el nuestro (jugadores 0x7C × 0x138, equipos
  0x8ED2FC × 0x24C, plantillas 0x9D4648 × 0x11C, tácticas 0xA09880 × 0x274): confirma nuestro trabajo.
- **Liga Máster:** no existe ningún editor de guardados de código abierto ni el formato documentado en público: **lo nuestro
  (`01-MOTOR-PES2021.md`) es, hasta donde se encontró, lo más completo que hay.**
- **Memoria (para el puente en vivo):** tabla de Cheat Engine de xAranaktu (MIT): presupuesto de fichajes en `[rdi+0x016ECBF4]`,
  salarial en `+0x016ECC08`; jugador en RAM de ~0x17C B (sueldo +0x15C, valor +0x174, contrato +0x138).
- **Sider 7.4.1:** código en GitHub (`pes-modding/sider7`) — leer, no copiar (licencia). Novedad clave: `save.folder`.

### E. Receta del Phoenix Evolution Patch (propuesta)

1. **Base fija:** PES 2021 de Steam, exe 1.07.02 + DP 7.0, Sider 7.4.1 incluido, `save.folder` propio.
2. **Núcleo pequeño:** base de datos Phoenix en un `.cpk` chico (como ConmeGOL) + option file oficial publicado por la web (ya existe).
3. **Contenido por Sider/livecpk**, en addons con versión (caras, kits, estadios, narración), con **un `sider.ini` auditado** sin duplicados.
4. **Lanzador propio** (Phoenix Link): comprueba exe/DLC/Sider/addons, archivo de paginación, antivirus y OneDrive; muestra avisos de update.
5. **Mercado y Liga Máster** siempre sobre la base Phoenix, con copia de seguridad automática antes de cada escritura (ya lo hacemos).
6. **Online:** modelo Parsec + web + buzón Sider (sección C).
7. **Identidad:** Liga 1 y ligas andinas como punto fuerte; IDs reales de Konami para que los facepacks encajen.
8. **Pruebas antes de cada versión:** instalación limpia; arranque, amistoso, Liga Máster (temporada completa y mercado), copa,
   Ser una Leyenda, edit, option file y partido por Phoenix Link. Todo anotado en `PRUEBAS.md`.

---

## Parte 4 · 04 · Informe estratégico: ConmeGOL y Sudamerican por dentro, y cómo superarlos

> Origen: `PhoenixSync/base-conocimiento/04-ESTRATEGIA-PHOENIX.md` · último cambio f75f677 2026-10-09 · 19632 bytes. Copia íntegra.

Fecha: 8 oct 2026. Basado en el análisis de las dos instalaciones de la PC de Fralex (bases de datos extraídas y comparadas, los dos
Sider auditados, estructura de carpetas, logs) y en la investigación de `03-INVESTIGACION-PARCHES.md`.
Leyenda de viabilidad: 🟢 **ya se puede** (con lo que dominamos) · 🟡 **se puede con investigación** (camino conocido) · 🔴 **frontera** (nadie lo ha hecho; puede no salir).

---

### 1. Los dos parches por dentro

#### 1.1 Base de datos (lo que define «el mundo»)

| | ConmeGOL 26 | Sudamerican Project 2026 |
|---|---|---|
| Archivo base | `CGP_database.cpk` (3,7 MB) | `SP_Subs.cpk` (3,9 MB) + `SP_Subs_WC.cpk` (Mundial, no cargado) |
| Competiciones | **90** | **80** |
| Equipos | **749** (techo del juego: 750) | **726** |
| Jugadores | **29.997** (techo: 30.000) | **29.323** |
| Asignaciones a plantillas | 20.398 | 17.214 |
| Archivos «4» (variantes de Konami) | no | sí (`Team4`, `Player4`, `Competition4`…) |
| Idioma del option file | español | portugués |
| Equipos con el mismo ID en los dos | 467 (274 con el mismo nombre) | |

**Lo que trae cada uno y el otro no:**
- **Solo ConmeGOL:** Libertadores, Sudamericana y Recopa con nombre CONMEBOL; ligas de **Venezuela, Uruguay (copa y 2.ª), Paraguay
  (copa), Costa Rica, Honduras, MLS y Arabia**; 2.ª de Colombia y Ecuador; «Clásicos» y «Leyendas».
- **Solo Sudamerican:** **Alemania** y **Países Bajos** completas, Europa League y Supercopa de Europa, 2.ª de Inglaterra, Supercopa de
  España, copas de Ecuador/Paraguay/Uruguay/México/EE. UU., y **2.ª de Perú con 14 equipos** (en ConmeGOL está vacía).

**Perú (lo que más nos importa):**
- **Liga 1 2026 (18 equipos):** los dos la tienen igual: Alianza Atlético, Alianza Lima, Atlético Grau, FC Cajamarca, Cienciano,
  Comerciantes Unidos, Cusco FC, Garcilaso, Moquegua, Juan Pablo II, Los Chankas, Melgar, Sport Boys, Sport Huancayo, Sporting Cristal,
  ADT, UTC y Universitario.
- **Copa (32):** los dos la tienen (con 2.ª división incluida).
- **Liga 2:** solo Sudamerican (Cantolao, ADA Jaén, Alianza Universidad, Ayacucho, Bentín Tacna, Mannucci, Comerciantes FC, Estudiantes
  CNI, San Marcos, Piratas, Santos Nazca, Unión Minas, César Vallejo, San Martín).
- **Supercopa:** ConmeGOL Universitario–Alianza; Sudamerican Universitario–Cusco.

**Lección:** los dos van **pegados al techo** del juego (750 equipos, 30.000 jugadores). Cada liga nueva obliga a **sacar** otra. Phoenix
tiene que **elegir** qué ligas trae (calidad antes que cantidad) y dejar espacio libre para crecer.

#### 1.2 Cómo entregan el contenido

| | ConmeGOL | Sudamerican |
|---|---|---|
| Modelo | **Casi todo suelto por Sider** (`livecpk` + `content`) | **Clásico de CPK**: 34 CPK en DpFileList |
| Caras | por Sider | **13 CPK de 5–11 GB** cada uno |
| Estadios | `stadium-server`: ~9 GB en más de 11.000 archivos | `stadium-server`: 313 carpetas |
| Kits | `kit-server` por país | `kit-server`: 36 países, 157 equipos |
| Balones | `ball-server` (34) | `ball-server` (**1.582**) |
| Otros pesados | Turfs 2,4 GB, narración de estadio 1,8 GB, gráficos «next gen» 1,7 GB, cuerpos PRDX ~1 GB (6.000+ archivos) | narradores en CPK (Bambino Pons, Solabarrieta, Miguel Simón) y en Sider (Closs, Palma, Barril, Tapia, Romano, Vignolo) |
| Archivos diminutos | `slotkits`: ~9.900 archivos casi vacíos | — |
| Sider | 7.3.3, 49 módulos activos | **7.3.4**, 56 módulos (normal) / 66 líneas (Mundial) |
| Lanzador | switcher que **copia** carpetas encima | 7 `.bat` (uno por letra de disco) |

*(El recuento de archivos de ConmeGOL es parcial: la carpeta es tan grande que leerla entera por el puente tarda horas; las cifras de
arriba son las ya contadas.)*

#### 1.3 Sus estrategias para ser estables (lo que hacen BIEN)

1. **Base de datos pequeña y aparte** (3,7–3,9 MB): la parte delicada cabe en un archivo; el resto es decorado.
2. **Sider como capa de contenido** (sobre todo ConmeGOL): actualizar = copiar carpetas, sin reempaquetar CPK gigantes.
3. **Variantes por switcher** (ConmeGOL: normal / Clásicos / Perú-Chile-Uruguay B; Sudamerican: normal / Mundial): cada modo tiene su
   propia base y su propio `sider.ini`, en vez de meter todo junto.
4. **Módulos de la comunidad muy probados** (kserv, StadiumServer, BallServer…): no reinventan lo básico.
5. **Comprobaciones de identidad**: los módulos CGP solo se activan si encuentran ciertos equipos en ciertas competiciones (un «candado»
   para no funcionar con otra base).
6. **Comunidad y créditos** (Sudamerican: más de 30 colaboradores, Discord): mantener un parche es trabajo de equipo.

#### 1.4 Sus fallos (lo que hacen MAL, comprobado en tu PC)

| Fallo | ConmeGOL | Sudamerican | Consecuencia |
|---|---|---|---|
| `camera.lua` + `DynamicWideCam.lua` parchean la **misma instrucción** | sí (log) | sí (log, mismas direcciones) | Escriben encima de datos del juego; cierres raros |
| Escrituras a ciegas (`GFX_lod`) | sí | sí | Ídem |
| Módulos que **no existen** en `sider.ini` | 11 | 3 | Ruido; señal de desorden |
| Módulos que **fallan al arrancar** | — | 3 (`randomMenu`, `TurfLoader`, `LogoHD`) | Funciones que el usuario cree tener y no tiene |
| Módulos **cargados dos veces** | — | `MenuServer`; en el Mundial `env`, `etrace`, `camera`, `netblock` | Doble parcheo |
| Módulos de **prueba** activos (`env`, `etrace`, `jittest`, `zlibtest`) | `env` | varios | Logs gigantes, más carga |
| Módulos que no hacen nada / de otra versión (Chatty para FL26, scoreboard EPL, Anti-Cheat placebo, goalscreams) | sí | parcial | Falsa sensación de función |
| `netblock.lua` | — | sí | Bloquea cualquier online |
| Contenido duplicado entre modos (cada variante copia todo) | sí (~234 GB el pack completo) | sí (dos Sider casi iguales) | Pesado, lento de actualizar |
| CPK de caras de 5–11 GB | — | sí | Actualizar una cara = bajar 10 GB |
| Miles de archivos diminutos (`slotkits`, cuerpos PRDX) | sí | — | Cada archivo cuesta una búsqueda de Sider |
| Instalación por `.bat` con letra de disco | — | sí | Frágil; el antivirus desconfía |
| Lanzador que **pisa** carpetas sin avisar | sí | — | Perdemos cambios (nos pasó con `phoenix.lua`) |
| Copias del juego no originales | sí | sí | Sin PESBUL; riesgo de cargas infinitas |

---

### 2. Cómo superarlos en estabilidad (🟢 todo esto ya se puede)

1. **«Sider limpio» certificado:** cada módulo que entre al parche pasa la auditoría (memoria, archivos, teclas, choques). Prohibido: dos
   módulos que toquen lo mismo, escrituras sin comprobar, módulos de prueba, duplicados.
2. **«Phoenix Doctor»:** la auditoría que hicimos a mano, convertida en **herramienta automática** dentro del lanzador: lee `sider.ini` y
   `sider.log`, detecta módulos que faltan, duplicados, fallos de arranque y choques conocidos, y propone la corrección. *(Ningún parche
   tiene esto.)*
3. **Lanzador único** (Phoenix Link): sin `.bat`; comprueba versión del juego, Sider, addons, archivo de paginación y espacio; **nunca pisa
   sin copia**; muestra qué cambió.
4. **Una base, un `sider.ini` por modo**, generados por el programa (no a mano) y con su huella SHA-256 comprobada en cada arranque.
5. **Pruebas automáticas** antes de publicar (ya tenemos 271 para el Mercado) + la lista de pruebas en el juego de `PRUEBAS.md`.
6. **Reglas de base de datos:** no cambiar el tamaño de las ligas sin parche del exe; dejar huecos libres de IDs; usar IDs reales de
   Konami (los facepacks encajan solos).

### 3. Cómo superarlos en rendimiento (🟢/🟡)

1. **Menos archivos, más grandes** (🟢): juntar los miles de archivos diminutos (`slotkits`, cuerpos) en pocos paquetes; cada archivo
   suelto es una búsqueda de Sider por cada carga.
2. **Contenido solo de lo que juegas** (🟢): addons por liga (si no juegas la MLS, no cargas sus caras ni estadios).
3. **Texturas bien comprimidas** (🟢): DDS con compresión adecuada y tamaños razonables (no 4K donde no se ve) → menos VRAM, menos
   archivo de paginación, menos pantallas negras.
4. **Módulos ligeros** (🟢): nada que trabaje en cada fotograma sin necesidad; nada que mire el disco por cada archivo (como hace
   `real-coach-team`); un solo módulo Phoenix en vez de diez pequeños.
5. **Caché de búsquedas de Sider** bien ajustada (🟡): `lookup-cache`, `cache.size` y el orden de las raíces `cpk.root` (las pequeñas y
   específicas primero).
6. **Medición** (🟡): un módulo que mida tiempos de carga y FPS y los anote, para comparar versiones con números y no a ojo.

### 4. Funciones nuevas: hasta dónde podemos llegar

#### 🟢 Ya se puede (con lo que ya probamos)

| Función | Qué es | Por qué nadie la tiene |
|---|---|---|
| **Mercado conectado a la web** | Fichajes reales de la liga (web firmada) aplicados al option file y a la Liga Máster | Ya funciona (pruebas 14–19); los parches solo traen plantillas fijas |
| **Liga Máster «viva»** | La web fija presupuestos, mete fichajes o ventas en tu carrera entre partidos | Nadie había descifrado la Liga Máster (no hay editor público) |
| **Option file por sala** | Phoenix Link carga el option file de la liga/sala elegida automáticamente | Los parches usan un único EDIT para todo |
| **Avisos en el juego** | Mensajes de la web dentro del juego (puente `avisos.txt`) | Prueba 1 hecha, falta probarla en el juego |
| **Resultado automático** | Módulo que lee el marcador (`match.stats()`) y lo deja en un archivo para que Phoenix Link lo suba | Las ligas actuales usan capturas a mano |
| **Phoenix Doctor** | Diagnóstico automático del Sider y del juego | Ver §2 |
| **Verificador del anfitrión** | Antes de una partida de liga, se comprueban las huellas del parche y del option file | Base: el verificador que ya planeabas |
| **Partidos «de liga» en amistoso** | Hacer que un amistoso se vea como partido oficial (menú, marcador, intro de la liga Phoenix), como hace `CustomExhibitionMatch.lua` | Existe para Euro/Asia; nadie lo usa para una liga propia |

#### 🟡 Se puede con investigación (el camino es conocido)

| Función | Qué hace falta |
|---|---|
| **Estadísticas completas a la web** (goleadores, tarjetas, posesión) | `match.stats()` no las da: leerlas de la memoria (tabla de Cheat Engine de xAranaktu como punto de partida) |
| **Cambios en vivo dentro de la Liga Máster** (sin salir del juego) | Mapa de la Liga Máster en la memoria (presupuestos ya localizados por xAranaktu) |
| **Fabricar nuestros propios CPK** | Portar el formato de `pes-file-tools` a C++ |
| **Competiciones propias** (Liga Phoenix, copas de la web) | Descifrar `Competition*`/`CompetitionRegulation` comparando antes/después (como hicimos con la Liga Máster) |
| **Calendario de la Liga Máster desde la web** | Encontrar dónde se guarda el calendario (pista: los 10 IDs de partido por jornada) |
| **Actualizaciones por partes** (solo lo que cambió) | Paquetes pequeños + manifiesto con huellas (la web ya publica el option file así) |
| **Ligas de más de 20 equipos o formatos nuevos** | Parches del exe localizados por patrón (como hace FL) |
| **Informe de errores** | Que el lanzador, con permiso, mande `sider.log` cuando el juego se cierre, para arreglar fallos de todos |

#### 🔴 Frontera (nadie lo ha hecho; puede no salir)

| Idea | Por qué es difícil |
|---|---|
| **Liga Máster online asíncrona**: varios usuarios, cada uno con su club en su propia carrera, compartiendo un mismo mercado y una misma tabla en la web | Hay que sincronizar las carreras (resultados de la IA, calendario, fichajes) sin un servidor dentro del juego. Con lo que ya sabemos de la Liga Máster es **la idea más original y la más cercana**. |
| **Estado del mundo real en el juego**: tabla, lesiones o forma de la Liga 1 real aplicadas cada semana | Fuentes de datos, y escribir forma o estado en la base o en la memoria |
| **Servidor propio estilo PESBUL** | Protocolo y login de Steam; meses o años de trabajo; PESBUL aún no tiene 11 contra 11 |
| **Espectadores en vivo**: marcador y datos del partido en la web mientras se juega | Leer memoria en vivo + subir datos sin afectar al juego |
| **Árbitro/VAR o reglas propias de la liga** | Lógica del juego dentro del exe: requiere parches de código |

---

### 5. Hasta dónde se puede «jugar» con el motor (límites honestos)

- **Archivos de la carpeta `save`** (option file, Liga Máster): **control casi total** — ya lo demostramos.
- **Base de datos (`pesdb`)**: control total en cuanto descifremos los archivos que faltan; límites de 750 equipos y 30.000 jugadores.
- **Contenido (caras, kits, estadios, menús, sonidos)**: control total vía Sider/CPK.
- **Memoria del juego en vivo (Sider + Lua)**: se puede leer y escribir; el riesgo crece con cada escritura → solo con patrón + verificación + pruebas.
- **Reglas «cableadas» en el exe** (calendarios, tamaños de liga, IA, físicas): solo con parches de código, que se rompen si cambia el exe.
- **Red dentro del juego**: Sider no trae red; **todo lo online pasa por Phoenix Link** (fuera del juego). Es una limitación y a la vez
  una ventaja: el juego no se expone a Internet.

### 6. Ruta completa propuesta

| Fase | Qué | Entregable |
|---|---|---|
| **0. Terreno** | Instalación limpia de referencia (Steam, 1.07.02 + DP 7.0, Sider fijado) + partido de referencia del Sider actual | Base de pruebas confiable |
| **1. Núcleo** | Base de datos Phoenix (partir de lo mejor de cada parche: Liga 1 + **Liga 2 de Sudamerican** + Copa, con permiso de sus autores) · option file oficial de la web · `sider.ini` limpio | Versión 0.1 (jugable, pequeña, estable) |
| **2. Herramientas** | Fabricador de CPK y DpFileList en Phoenix Sync · Phoenix Doctor · lanzador | Instalar/actualizar sin `.bat` |
| **3. Conexión** | Avisos (prueba 1) → resultado automático → verificador del anfitrión → option file por sala | Primeras partidas de liga «online» con amigos |
| **4. Contenido** | Addons por liga (caras, kits, estadios) con versión; permisos y créditos | Parche completo, modular |
| **5. Más allá** | Competiciones propias, estadísticas completas, Liga Máster viva, y la gran apuesta: **Liga Máster online asíncrona** | Lo que nadie tiene |

**Créditos y permisos:** casi todo el contenido visual de los dos parches es obra de terceros (facemakers, kitmakers, Olmos Jr 23,
Afandix…). Para publicarlo dentro de Phoenix hace falta su permiso y su crédito. Lo propio de Phoenix (base de datos, herramientas,
conexión con la web) es 100 % de Fralex.

---

### 7. Proyección a futuro: cuando los parches se actualicen (y lleguen otros)

Se esperan pronto: **ConmeGOL** (la versión instalada es la **26**, base del 19/8/2026; ya existe la **27**, sept. 2026) y
**Sudamerican** (en una semana). Más adelante, **FL27** (inicios de noviembre) y otros. La idea: que cada actualización **no rompa
nada nuestro** y que además **nos enseñe algo**.

#### 7.1 Lo que suele cambiar en una actualización de parche

| Qué cambia | Cómo afecta a Phoenix |
|---|---|
| Base de datos (fichajes, equipos nuevos, IDs que se mueven, competiciones nuevas) | El **catálogo** del Mercado queda viejo → regenerarlo y subirlo a la web; revisar si cambian IDs de equipos/jugadores |
| Option file nuevo | Las plantillas cambian; el option file oficial de la web tiene que rehacerse sobre la nueva base |
| Módulos Sider nuevos o actualizados, `sider.ini` nuevo | **Volver a auditar** (choques, escrituras a ciegas); el switcher puede pisar lo que hayamos instalado |
| Carreras de Liga Máster | Muchos parches piden **empezar una carrera nueva** tras actualizar la base; las carreras viejas pueden dar plantillas raras |
| Estructura (más CPK, nuevas carpetas livecpk) | Revisar DpFileList (orden, cantidad) y espacio en disco |

**Lo que NO debería romperse:** Phoenix Sync localiza todo **por anclas** (busca la forma de los datos, no posiciones fijas) y valida
antes de escribir; si algo cambió de forma, **se frena** en vez de estropear el archivo. Esa decisión de diseño es justo para este momento.

#### 7.2 Huella del «antes» (hecha hoy, 8 oct 2026, 21:15)

Guardada en la PC en `_PhoenixMercado_prueba\_versiones\2026-10-08\` (y la lista en `datos/huella-2026-10-08.txt`):
- ConmeGOL: `CGP_database.cpk` (md5 d2d62b4d…, 3.699.240 B, 19/8/2026), `DpFileList.bin`, `sider.ini` (md5 80ad499f…), option file
  (md5 027b0fa2…), lista de módulos y CPK con tamaño y fecha.
- Sudamerican: `SP_Subs.cpk` (11/5/2026), `SP_Subs_WC.cpk`, `DpFileList.bin`, los dos `sider.ini`, option file (md5 6c612a4c…).
- El mismo `PES2021.exe` en los dos (md5 d4760962…).
Con esto, después de actualizar se puede decir **exactamente** qué cambió.

#### 7.3 Protocolo del «día de actualización» (para mañana y para siempre)

**Antes** (Fralex):
1. Copiar la carpeta `save` del parche (option file y carreras) a un lugar seguro.
2. Avisar: «voy a actualizar ConmeGOL / Sudamerican».

**Después** (Claude, con la PC conectada):
1. Nueva huella y **comparación** con la anterior: qué archivos cambiaron.
2. **Base de datos:** equipos que entran/salen, IDs que cambian, competiciones nuevas, jugadores movidos (con atención a Perú).
3. **Sider:** módulos nuevos o cambiados → auditoría solo de esos; comprobar si siguen los choques de cámaras, etc.
4. **Option file:** comparar plantillas.
5. **Mercado:** regenerar el catálogo, correr las pruebas (`PROBAR2.bat`), subir el catálogo nuevo a la web.
6. **Liga Máster:** probar que el programa lee una carrera nueva de la versión nueva (y decidir qué pasa con las viejas).
7. Anotar todo en `parches/<parche>.md` como **«changelog técnico»** (lo que el autor no cuenta), en `PRUEBAS.md` y en Drive.

#### 7.4 Automatizarlo: el «Auditor de parches» (fase 2)

Lo que hoy hacemos a mano se convierte en un comando de Phoenix Sync:
- `auditar <carpeta del juego>` → huella completa (base, DpFileList, Sider, option file), comparación con la última, informe en Markdown.
- Comparador de bases de datos (equipos, competiciones, plantillas) y **tabla de equivalencias de IDs entre versiones** (reutilizando el
  emparejamiento ConmeGOL ↔ Sudamerican que ya existe).
- Auditoría automática de `sider.ini` y módulos (Phoenix Doctor).
- Un **registro de parches** (nombre, versión, carpeta del juego, carpeta `save`, número de Steam) para que el Mercado y Phoenix Link
  sepan siempre con qué parche y qué versión trabajan, y no mezclen archivos (ConmeGOL y Sudamerican comparten la carpeta de documentos).

#### 7.5 Parches futuros (FL27, Gogosz, nuevas versiones)

Cada parche nuevo que entre a la PC recibe la misma ficha: identificación, base de datos (comparada con las demás), contenido, Sider
auditado, estrategias, fallos, y qué copiar o evitar. Con varias fichas se ve **la tendencia** (qué hace la comunidad cada temporada)
y Phoenix puede adelantarse.

#### 7.6 El propio Phoenix, preparado para su futuro

- **Versiones con número claro** y cada actualización marcada: «no afecta a tu carrera» / «necesita carrera nueva» / «se aplica la
  próxima temporada» (como FL).
- **Huella publicada** de cada archivo (como el option file oficial de la web) y lanzador que comprueba y repara.
- **Las carreras y los datos de la web sobreviven** a las actualizaciones: la web guarda la verdad (fichajes, presupuestos) y el
  Mercado puede **reaplicarla** sobre una base nueva.

---

## Parte 5 · 06 · La ruta del descubrimiento (noche del 8 al 9 de octubre de 2026)

> Origen: `PhoenixSync/base-conocimiento/06-RUTA-DEL-DESCUBRIMIENTO.md` · último cambio 82116b5 2026-10-09 · 12734 bytes. Copia íntegra.

**Qué es:** el relato **en orden**, paso a paso, de cómo se llegó a cambiar el juego en vivo: qué se intentó, por qué, qué salió bien, **qué salió mal** (incluidos los errores de Claude) y qué se aprendió en cada paso.
Sirve para **no repetir errores** y para entender **por qué** el método final es como es.
- Detalle técnico de cada prueba: `../PRUEBAS.md`.
- Método final: `05-PUERTA-EN-VIVO.md`.
- Resumen general: `../MANUAL-PARCHE-PHOENIX.md`.

**Participantes:** FRALEX (juega y prueba en su PC, toma las fotos y decide) y Claude (analiza, programa, instala con respaldo y documenta).

---

### Etapa 1 · Entender el terreno (00:10–00:30)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 1 | Se abrió por dentro el **switcher** de ConmeGOL (`PES2021 Start.exe`). Es un programa de Python empaquetado con PyInstaller; se extrajo y se desensambló con Python 3.14. | ✅ Hace robocopy de la carpeta del modo encima del juego, abre Sider, espera 2 s y abre el juego. **No usa internet.** |
| 2 | Revisión de lo que trae Sider: `sider.dll`, eventos, librerías y el módulo Real-Time-Match-Manager. | ✅ Sider tiene `io`, `memory`, `ffi` y eventos como `display_frame` y `livecpk_read`. **No tiene red.** Existe un precedente: un módulo que cambia la lluvia en vivo leyendo un `.ini`. |
| 3 | **Decisión de arquitectura:** web ⇄ Phoenix Link (internet) ⇄ archivos («buzón») ⇄ Sider. | ✅ El juego nunca toca internet. |

**Acierto:** abrir el switcher explicó por qué el primer intento de instalar `phoenix.lua` (días antes) nunca apareció: el switcher lo pisaba.

### Etapa 2 · El buzón: avisos en vivo (00:30–00:45)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 4 | Se instaló `phoenix.lua` v0.1 **en la carpeta del modo**, con respaldo de `sider.ini` y una sola línea nueva. | ❌ **No cargó.** `sider.log`: «attempt to call global 'pcall' (a nil value)». |
| 5 | **Error de Claude:** dio por hecho que el Lua de Sider traía `pcall`, como cualquier Lua. Lo había probado con Lua normal, no con el entorno real de Sider. | Lección: **probar siempre con los globales exactos de Sider**. El volcado de `env.lua` en el log los lista. |
| 6 | v0.2 sin `pcall` (errores contados con una «bandera»). FRALEX pulsó Shift+R. | ❌ «Reloaded modules: 0». **Error de Claude:** creyó que Shift+R cargaría un módulo que había fallado; solo recarga los que ya estaban activos. |
| 7 | Se reinició el juego. | ✅ «PHOENIX EVOLUTION · conectado». |
| 8 | Se cambió el aviso con el juego abierto. | ✅ **Apareció en 1–2 s** («hola causa»). Tildes, ñ, ⚡ y → se ven bien. |

### Etapa 3 · Nombre y organización (00:45–02:20)

- Se prepararon prompts para el chat WEB (buzón con ETag) y para el chat LINK (el «cartero»).
- **Phoenix Mercado pasó a llamarse Phoenix Sync** por decisión de FRALEX. Se renombraron la carpeta, los exe y los textos, pero se mantuvo lo que otros usan: la API `/mercado/v1`, el token y la rama. Compiló, con 295/295 pruebas OK.
- Más tarde, el chat LINK construyó el cartero y **el «hola» escrito en la web apareció en el juego (02:59)**: la cadena completa web → juego quedó confirmada.

### Etapa 4 · Buscar al jugador en la memoria (02:35–03:00)

FRALEX quería cambios **en tiempo real**. Primera idea: encontrar la ficha de Lamine Yamal en la memoria del juego.

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 9 | v0.3: tecla B. Recorre la memoria por regiones y busca los 58 bytes exactos de la ficha. | 🟡 Revisó 4,4 GB en 33 s **sin crash**, pero **0 coincidencias**: el juego cambia bits al cargar (forma, lesión). |
| 10 | FRALEX pulsó B por segunda vez. | ❌ **CRASH** (0xC0000005 dentro de sider.dll). **Error de Claude:** leía memoria **directo**, averiguando qué zonas existían en un cuadro y leyéndolas en los siguientes. Justo entonces ReShade recargó sus efectos y liberó memoria. **No se dañó nada** (solo lectura; los guardados tenían fecha anterior). Se reinstaló la versión segura de inmediato. |
| 11 | v0.4: **copia segura** con `ReadProcessMemory`, que si la zona ya no existe responde «falló» en vez de cerrar el juego. Solo memoria privada. Busca el ID y **verifica** 5 cualidades. | ✅ **Ficha encontrada sin crash** (Velocidad 90, Aceleración 93…). La versión de v0.4 se instaló solo **con el OK de FRALEX**, tras el crash. |
| 12 | v0.5: comparar copias. FRALEX estaba en la pantalla de habilidades de Lamine. | ✅ **Calibración gratis:** las 15 cualidades de la pantalla coinciden con el mapa, y **Potencia de tiro (358) queda confirmada**. Las «copias 1–5» eran falsas alarmas (listas con el nombre). Se filtraron exigiendo el ID. |

### Etapa 5 · Escribir en la memoria: el callejón sin salida (03:00–03:20)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 13 | v0.6: tecla V. Cambia **un byte** (Velocidad 99) con `WriteProcessMemory`, tras volver a comprobar la ficha. | ❌ La pantalla siguió en 90. Solo se escribió 1 de 4 copias. **Error de Claude:** exigía el nombre dentro de la ficha, y las copias que usa el juego no lo llevan. |
| 14 | v0.7: verificar por ID + bytes de cualidades. | ❌ Siguió en 90, aunque **todas** las copias ya tenían 99. |
| 15 | v0.8: buscar la ficha «desempaquetada» (6 formatos). | ❌ 0 coincidencias. |
| 16 | **Pista clave:** cada búsqueda encontraba **más copias**, y las nuevas nacían con 90 y con el formato exacto del archivo. Hipótesis: el juego **vuelve a leer el archivo**. | → Cambio de estrategia: **dejar la memoria e ir al archivo**. |

**Lección:** escribir en la memoria es frágil y no sirve, porque el juego rehace sus copias desde la base. **Mejor archivo que memoria.**

### Etapa 6 · El espía y el botón nativo (03:20–03:35)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 17 | v0.9: **espía** pasivo con `livecpk_read`, que cuenta cada lectura de `pesdb\*.bin` (técnica vista en `CommonLib.lua` de zlac). | ✅ Al arrancar se lee todo dos veces. Al abrir pantallas, **no**. |
| 18 | Idea de FRALEX: usar los **botones del propio juego** en vez de Sider. Primero se probó «Datos Actual. en vivo». | ❌ «Los servicios en línea finalizaron el 25/08/2022»: depende de Konami. |
| 19 | FRALEX entró y salió de **Editar** y de la Liga Máster. | ✅ **Entrar a Editar relee Player.bin** (lectura #3). La Liga Máster no. |

**Acierto de FRALEX:** buscar un botón nativo. Ese fue el camino ganador.

### Etapa 7 · Nuestro libro (03:35–03:55)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 20 | Se fabricó un **Player.bin propio**: la base de olmosjr23 descomprimida, **1 byte** cambiado (Velocidad 99), recomprimida WESYS + zlib y verificada de ida y vuelta. Se sirvió desde una raíz nueva, `livecpk\Phoenix-DB`, puesta **antes** de olmosjr23 (respaldo y una línea). | ✅ **Tras reiniciar: Velocidad 99.** El juego acepta nuestra base. |
| 21 | Se cambió a v95 con el juego abierto; FRALEX entró y salió de Editar. | ❓ Siguió 99. Además **se pulsó V** (la prueba vieja de escribir en memoria) y **ensució** el resultado. **Error de Claude:** dejar activa una tecla de una prueba ya descartada. Se quitó en v0.10. |
| 22 | v0.10: **huella** de cada lectura (suma cada 64 bytes del trozo que entrega Sider). Prueba limpia: arranque con v95. | ✅ Pantalla 95, y la huella coincide **exactamente** con v95. |
| 23 | Con el juego abierto: v90 → **solo Editar** (entrar y salir). | 🟡 La huella demuestra que leyó v90, **pero la pantalla siguió en 95**: Editar usa un borrador y lo descarta. |
| 24 | **Editar → Cargar** (sin guardar) → amistoso nuevo. | 🏆 **VELOCIDAD 90. Cambio en vivo sin reiniciar, con un botón nativo del juego.** |

### Etapa 8 · Cierre y calibración (03:55–04:25)

- Phoenix-DB quedó con **datos idénticos a los originales** (v90), en la raíz del juego y en la carpeta del modo.
- Se documentó todo: `05-PUERTA-EN-VIVO.md`, el **Manual**, el PDF de 47 páginas y Drive.
- **Calibración con fotos de FRALEX:**
  - Joan García y Livakovic ⇒ **portero 100 % mapeado** (269 Despejar, 300 Cobertura, 320 Reflejos, 326 Actitud, 364 Atajar);
  - **pie malo, regularidad y lesiones = valor guardado + 1**;
  - **496 = Patadón en largo**; 502/522 = Pase largo portero / Parapenaltis.

### Etapa 9 · El fichaje en vivo (04:33–04:47)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 25 | Se sirvió un `PlayerAssignment.bin` propio (Lamine: Barça → Real Madrid) desde Phoenix-DB. Editar → Cargar. | ❌ El espía demuestra que el juego **leyó nuestro archivo**, pero Lamine siguió en el Barça. **Error de Claude:** suponer que las plantillas salen de la base; al cargar, **las plantillas las manda el option file**. |
| 26 | Se cambió el **option file** con `OptionFile::mover` (núcleo de Phoenix Sync, ya probado días antes), con respaldo, y Editar → Cargar. | 🏆 **Lamine titular en el Real Madrid con el 26, fuera del Barça. Fichaje en vivo sin reiniciar.** |

**Lección:** stats → archivo de la base; plantillas → option file. Mismo botón para las dos: Editar → Cargar.

### Etapa 10 · Liga Máster y el motor por dentro (04:50–05:15)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 27 | Stats v99 + Editar → Cargar (huella v99 confirmada) → entrar a la Liga Máster. | ❌ En la carrera Lamine sigue en 90: la LM usa su propia copia. Falta la prueba de reinicio. |
| 28 | Idea de FRALEX: usar la opción nativa de actualización en vivo y «hacer invisible» la parte de internet. Se leyó `PES2021.exe` por dentro (capstone en la VM del PC). | ✅ El código (37 MB) **no está cifrado**. La actualización en vivo de Konami es una máquina de 38 pasos: el 5 pide los datos al servidor y **el 26 carga el EDIT**. La idea de FRALEX era exactamente cómo lo hacía Konami. |
| 29 | Hallazgo extra: un **interruptor nativo** (`exe+0x37F5C39`) que hace que el juego recargue EDIT + base al volver al menú principal (lo usa «Ser una Leyenda»). | 🟡 `phoenix.lua` v0.11 (tecla L) instalado con comprobación de bytes. Falta la prueba en el juego. |

**Acierto de FRALEX:** no conformarse con «un botón de Editar» y apuntar a la opción nativa. Eso llevó a leer el exe.

---

### Resumen de aciertos

1. **Abrir el switcher** antes de instalar nada (explicó el fallo antiguo).
2. **Buzón por archivo:** simple, seguro y rápido (1–2 s).
3. **Copia segura de memoria** tras el crash: nunca más se cerró el juego.
4. **El espía de lecturas:** convirtió suposiciones en datos.
5. **La huella:** prueba sin dudas de qué archivo leyó el juego.
6. **Cambiar de estrategia a tiempo:** de la memoria al archivo.
7. **El botón nativo** (idea de FRALEX) → Editar → Cargar.
8. **Cada cambio con respaldo, una línea y documentado.** Ningún archivo de FRALEX quedó dañado.

### Resumen de errores (y su lección)

| Error | Consecuencia | Lección / regla |
|---|---|---|
| Suponer que Sider tenía `pcall` | El módulo no cargó | Probar con los globales reales de Sider |
| Suponer que Shift+R cargaría un módulo fallido | Una vuelta de más | Shift+R solo recarga módulos activos |
| Leer memoria directo a lo largo de varios cuadros | **Crash del juego** (sin daños) | Siempre `ReadProcessMemory`; nunca memoria ajena directo |
| Buscar la ficha byte a byte | 0 resultados | Buscar por ID y verificar cualidades |
| Exigir el nombre para escribir | Se saltaron las copias reales | Verificar por lo que de verdad identifica (ID + cualidades) |
| Insistir en la memoria (v0.6–v0.8) | 3 intentos sin éxito | Si el juego rehace copias, el problema está en la **fuente** (el archivo) |
| Dejar activa la tecla V | Ensució una prueba | Desactivar las herramientas de pruebas descartadas |
| Creer que «entrar a Editar» bastaba | Una prueba a medias | Medir con huella; el paso correcto es **Cargar** |
| Suponer que las plantillas salen de `PlayerAssignment.bin` | Un intento fallido | Al cargar, **el option file manda** en plantillas; las stats van por la base solo si el jugador no está editado |

### Cómo se trabajó (método que funcionó)

1. **Hipótesis pequeña → prueba mínima → medir → anotar.**
2. **Solo lectura primero; escribir después**, y solo con el OK de FRALEX.
3. **Simular antes de instalar:** LuaJIT con el entorno de Sider y memoria falsa, incluida una zona que desaparece.
4. **Instalar con respaldo** y una sola línea, comprobada con `diff`.
5. **Si algo falla, parar:** primero la versión segura y después el análisis («si hay un error no avanzamos»).
6. **Documentar en el momento:** PRUEBAS, REGISTRO, GitHub y Drive.

---

## Parte 6 · 07 · El motor por dentro: recargar sin «Editar → Cargar» y el botón nativo de actualización

> Origen: `PhoenixSync/base-conocimiento/07-MOTOR-POR-DENTRO.md` · último cambio 9e5f289 2026-10-09 · 9483 bytes. Copia íntegra.

**Estudio del 2026-10-09 (05:00–05:15, Lima)** sobre `PES2021.exe` del PC de Fralex (437 MB, versión final `PATCH10100`).
Herramientas: Python + capstone en la VM del PC, directamente sobre el exe. **No se modificó el exe.**
Ayudantes guardados en la VM: `~/re/xref.py`, `~/re/desm.py`, `~/re/quien.py`, `~/re/llamas.py`, `~/re/refsa.py`.

---

### 1. Cómo está hecho el ejecutable

| Sección | Tamaño | Qué es |
|---|---|---|
| `.trace` | 37 MB | **El código del juego, sin cifrar** (entropía 6,5, normal en código x64). Se puede leer y desensamblar. |
| `.impdata` | 380 MB | Capa de protección (tipo Denuvo). El punto de entrada está en `.sbss`, dentro de la capa. |
| `.rdata` | 14 MB | Textos y tablas: nombres de clases (RTTI), rutas del código fuente de Konami y mensajes de depuración. |
| `.pdata` | 1,8 MB | Tabla de funciones: **156 556 funciones** en `.trace` con su inicio y su fin. |

- El motor es **Fox Engine** (`FoxCore`), con Lua propio para escenas. **Los menús son clases C++** (`Process…`, `…Flow`, `…Functor`).
- Base de carga 0x140000000. Las direcciones de este documento son **RVA** (exe + RVA).

### 2. La fábrica de «cargar EDIT» (la que usa Editar → Cargar)

- **`0x1EFB1F0` = crear tarea de carga del option file** `(nombre, parámetros)`. Se usa en 7 sitios, entre ellos:

| Sitio | Nombre de la tarea | Cuándo |
|---|---|---|
| `0x130F280` | `editLoadData` | Menú Editar → **Cargar** |
| `0xAEF7A3` | `editLoadDataInTopMenu` | Al **volver al menú principal**, solo si el interruptor vale 1 |
| `0x20AE682` | `editLoadDataInLiveDataSet` | Último paso de la **actualización en vivo de Konami** |
| `0xCDD8F1` | `EditLoadForBLPlayerSave` | «Ser una Leyenda» |

- La tarea se cuelga de su padre con `0x149FC70(padre, tarea)`.
- Hay textos de proceso que confirman la relectura de la base: `ProcessEditDataLoad::CreateReloadPesdb`, `Edit/Load/EditDataLoadProcess`.

### 3. ⭐ El interruptor nativo: recargar al volver al menú principal

- **Byte `exe+0x37F5C39`** (zona de datos, empieza en 0).
- Lo lee `menu::ModeFlowCmnInitFunctor` (vtable `0x2684BB0`, función `0xAEF2E0`). Esa función se ejecuta al **salir de un modo al menú principal** (también llama a `CmdLeaveOfflineMode`):
  ```
  0xAEF770  cmp byte [exe+0x37F5C39], 0      ; 80 3D C2 64 D0 02 00
            je  seguir
            crear tarea "editLoadDataInTopMenu" (parámetros 0x100, igual que Cargar)
  0xAEF7CE  llamar 0x1EFB440 con cl=0        ; vuelve a poner el byte a 0
  ```
- **`0x1EFB440`** = `mov [exe+0x37F5C39], cl ; ret` (88 0D F3 A7 8F 01 C3). Lo llaman el menú principal (para ponerlo a 0) y `0xCDD820` (tras guardar en «Ser una Leyenda», lo pone a 1).
- **Idea:** escribir **1** en ese byte. La próxima vez que el jugador vuelva al menú principal, el juego recarga EDIT + base **solo**. Sin Editar → Cargar.
- **Prueba:** `phoenix.lua` v0.11, tecla **L**. Antes de escribir, comprueba los 7 bytes de `0xAEF770` y de `0x1EFB440` (si el exe fuera otro, no toca nada). Muestra el valor en el overlay y avisa cuando el juego lo consume (1 → 0). Simulado OK en LuaJIT con el entorno de Sider. **Resultado en el juego: ver PRUEBAS.md.**

#### 3.1 Resultado de la prueba (05:08) y la relectura de la base
- ✅ El juego **consume el interruptor** al entrar a un modo (1 → 0), sin crash. **Recarga solo el EDIT, no la base** (el espía no ve lecturas de pesdb).
- Editar → Cargar hace dos cosas: la tarea del EDIT + **`0x1EF2FA0(gestor, 0)` = iniciar la relectura de la base**.
  - El gestor se obtiene con `0x1EF2250()` (y se crea con `0x1EEBBA0(1)` si falta). Así lo hace el proceso `0x13E4580`.
  - El gestor avanza la relectura cuadro a cuadro (`0x1F08820` → `0x125F310`, «cpk_dat/common/etc/pesdb/%s»). Listas de archivos por tipo (0–2) en `0x3529900`.
- El constructor de la tarea del EDIT (`0x1EFAE20`) está **virtualizado** por la protección: se puede usar, no leer.
- 🏆 **05:13 · probado:** option file original + L + entrar a Partido ⇒ Lamine volvió al Barça. **Fichajes en vivo sin Editar → Cargar.**
- **Consecuencia:** fichajes (option file) → interruptor ✅; stats (base) → falta disparar `0x1EF2FA0(gestor, 0)`.

#### 3.2 🏆 La recarga completa (05:31): stats + fichajes sin Editar
- Editar → Cargar = proceso «ProcessEditDataLoad::CreateReloadPesdb» (tabla `.data 0x34DA048` → fábrica `0x130F220`, bandera `[+0x8C] = 1`) → tarea «editLoadData» con parámetros **1,1,0,0,0**.
- El menú principal (`0xAEF78A`: `mov dword [rsp+0x20], 0x100`) pasa **0,1,0,0,0** → recarga el EDIT sin la base.
- **Parche en memoria de 1 byte:** `exe+0xAEF78E` 00 → 01 (queda 0x101, igual que Cargar). No se toca el exe en disco. La página ya era RWX (0x40).
- **Probado:** tecla P (parche + interruptor) → entrar a Partido → el juego relee TODA la base (huella v95) → **Lamine Velocidad 95**. ✅
- `0x1EF2FA0` (tecla K) solo **lee** 4 archivos (tipo 0: Boots, Glove, Player, Country) y **no aplica**. Sirve para diagnóstico, no para el producto.

### 4. La actualización en vivo de Konami por dentro (`LiveDataSetFlow`)

- **Se crea con** `0x20AC030(nombre, modo, 1, 2)`. Por ejemplo, desde Editar → Gestión de datos → Actualización en vivo (`0x13AB293`, «ProcessEditLiveUpdate_LiveDataSetFlow»).
- **Actualización:** `0x20AC3F0`. Máquina de **38 estados** en `[flujo+0x94]`, tabla de saltos en `0x20AECC0`:

| Estados | Qué hace |
|---|---|
| 0–4 | Preparar y comparar la versión (DLC VERSION) |
| **5** (`0x20AC6B9`) | **Pide la lista al servidor** (`CmdGetLivedataList`). Si falla → estado 30 (error) |
| 6–10 | Descargar archivos, comprobar el hash |
| 11–12 | Guardar |
| 13–18 | «Bind» (aplicar los datos) |
| 19–25 | Diálogos y resumen |
| **26 (0x1A)** (`0x20AE5FA`) | **`editLoadDataInLiveDataSet`: carga el EDIT** y termina (estado 27) |
| 30 (0x1E) | Diálogo de error |

- **Konami hacía exactamente lo que quiere Fralex:** descargar y luego **cargar el EDIT**.
- **Plan para el «botón nativo»:** al llegar al estado 5, saltar al 26 (sin red). Hay dos formas:
  - (a) un parche de 15 bytes al inicio del estado 5: `mov dword [rdi+0x94], 0x1A ; jmp 0x20AEC82`;
  - (b) hallar el objeto del flujo y cambiar su estado desde Lua.
- **Falta averiguar:** si antes del flujo hay una **puerta de inicio de sesión** (`ProcessEditLiveUpdateLogin`, `OnlineFlowLogin`) que corta con el mensaje «los servicios finalizaron el 25/08/2022». Habría que encontrar esa puerta y dejarla pasar.

#### 4.1 🏆 La puerta y el botón nativo funcionando (05:46)
- **Ruta del botón** «Partido → Datos Actual. en vivo → Activar»: `Exhibition/LiveData/LiveDataSet` (`0x1308090`) → «ProcessCmnLiveDataSetFlow» (crea `0x1350E10`, actualización `0x20AF620`).
- **Estados del flujo común:**
  - 1 = crear «LiveDataLogin» (**la puerta**: inicio de sesión en Konami);
  - 2 = esperar. `0x20AF7D0` recibe el resultado; la decisión `0x1350E90` está **virtualizada**. Si falla, va al estado 5 = diálogo «servicios finalizados» + «No se han podido implementar…»;
  - 3 = crear LiveDataSetFlow;
  - 7 = terminar.
- **Parche de 19 B** en `exe+0x20AF73B` (inicio del estado 1): `mov byte [exe+0x37F5C39], 1 ; mov dword [rdi+0x94], 7 ; jmp 0x20AF7BA`. Original: `48 83 BF 98 00 00 00 00 75 6B 33 DB 89 5C 24 28 C7 44 24`.
- **Con el byte de la recarga completa** (`0xAEF78E` = 1): Activar → sin inicio de sesión → interruptor = 1 → al entrar a Partido el juego relee EDIT + toda la base. **Probado: Lamine 95 → 90 sin pulsar L ni P.**

#### 4.2 🏆 En el sitio, con el mensaje nativo (v0.16, 05:53)
- **Parche A** (`0x20AF73B`): `mov [rdi+0x94], 3 ; jmp 0x20AF7BA` → sin inicio de sesión, crea LiveDataSetFlow.
- **Parche B** (`0x20AC6B9`, estado 5): `mov [rdi+0x94], 0x16 ; jmp 0x20AEC82` → sin internet, al diálogo nativo (0xF90042).
- **Parche C** (`0x20AE664`): `0x100 → 0x101` → la recarga del estado 26 incluye la base.
- **Probado:** Activar → diálogo nativo («No hay cambios… forma física», con flechas) → el juego relee toda la base y la aplica **sin salir de la pantalla** (Lamine 90 → 99).
- Los estados 0–4 pasaron sin problemas (la comparación de versión no dio error).
- **Pendiente:** en esta recarga, las plantillas del Atlético cambiaron. ¿En modo vivo manda la base sobre el option file? Hay que confirmarlo.

### 5. Viabilidad (honesta) tras el estudio

| Meta | Antes | Ahora | Por qué |
|---|---|---|---|
| Recargar sin entrar a Editar | ~35–50 % | **✅ PROBADO: fichajes (L) y stats + fichajes (P)** | El juego ya trae el interruptor; es 1 byte de datos |
| Que el botón «actualización en vivo» cargue nuestros datos | ~25–35 % | **✅ PROBADO (05:46)** | Ya tenemos el mapa completo del flujo; falta la puerta de inicio de sesión |
| Que la Liga Máster reciba stats en vivo | ? | Pendiente | Editar → Cargar no llega (prueba 04:53); falta la prueba de reinicio |

### 6. Reglas para seguir

1. Antes de escribir algo, **comprobar los bytes exactos del código** (versión del exe). Si no coinciden, no se toca nada.
2. Solo bytes de **datos** primero (como el interruptor). Parchear código va después y con el OK de Fralex.
3. Nunca durante un partido. Nunca pulsar «Guardar» en Editar durante las pruebas.
4. Todo con respaldo: `phoenix.lua.v010` en las dos carpetas `modules`.

---

## Parte 7 · 12 · Guía multiparche: llevar todo lo descubierto a cualquier parche de PES 2021

> Origen: `PhoenixSync/base-conocimiento/12-GUIA-MULTIPARCHE.md` · último cambio 50b9c93 2026-10-09 · 13361 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026.**
Todo lo de los informes **05, 08, 09, 10 y 11** se descubrió y se probó sobre **ConmeGOL Patch 26**.
Esta guía dice **qué vale para todos los parches**, **qué cambia en cada uno**, **cómo encontrarlo** y **qué riesgos hay**.
Los informes anteriores no se tocaron: esta guía es su «traductor» para cualquier parche.

---

### 0. La idea en simple

Imagina que PES 2021 es **una casa** y cada parche es **una mudanza distinta** dentro de esa casa.

- **La casa** (las paredes, las puertas, cómo se abren los cajones) la pone **el juego**. Es igual en todas las mudanzas.
- **Los muebles** (qué jugadores hay, en qué cajón está la base, qué número tiene cada equipo) los pone **el parche**. Cambian en cada mudanza.

Phoenix Sync aprendió a abrir **los cajones de la casa**. Por eso funciona con cualquier parche.
Lo único que hay que hacer en cada parche es **averiguar dónde dejó sus muebles**.

Símbolos: ✅ probado en el juego · 🔎 visto en archivos · ⏳ sin probar todavía · ⚠️ riesgo.

---

### 1. Lo que es IGUAL en todos los parches (lo pone el juego)

| Qué | Por qué vale para todos |
|---|---|
| **El sobre y la llave** de los guardados (option file y Liga Máster) | La llave es de PES 2021, no del parche |
| **El formato del option file** (plantillas de 284 B, tácticas, jugadores editados) | Lo define el juego |
| **El formato de `Player.bin`** (312 B por jugador, cabecera WESYS + zlib) y de `PlayerAssignment.bin` (16 B) | Lo define el juego |
| **La estructura de la Liga Máster** (bloques de equipo, alineaciones, tablas de tu equipo, contratos, blob) | Lo define el juego. Phoenix Sync la encuentra **por anclas**, sin direcciones fijas |
| **Editar → Cargar** relee la base y el option file | Es un menú del juego |
| **La regla de «Datos Actual. en vivo»**: activada → plantillas de la base; desactivada → del option file | Es una opción del juego |
| **Los jugadores editados** del option file mandan sobre `Player.bin` | Comportamiento del juego |
| **El autoguardado** encima de la ranura cargada, y **el contrato** que crea el juego al fichar | Comportamiento del juego |
| **Los límites**: 750 equipos, 40 jugadores por equipo, 30.000 jugadores | Límites del juego (salvo parches con exe modificado, ver §4) |

---

### 2. Lo que CAMBIA en cada parche (y cómo encontrarlo)

#### 2.1 La carpeta de guardados 🧩

- **Qué es:** `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<número>\save\`. Cada parche puede usar **su propio número**.
- **Ejemplos en tu PC:** ConmeGOL → `239200`. Sudamerican → `292733975847239680`.
- **Cómo encontrarla en otro parche:**
  1. Abre el juego con ese parche y guarda algo (por ejemplo, Editar → Guardar en un archivo de prueba, o una carrera nueva).
  2. Mira cuál carpeta `save` cambió de fecha **justo en ese momento**.
- ⚠️ **Riesgo:** en tu PC hay notas que dicen que ConmeGOL y Sudamerican **comparten** la carpeta de guardados. Compruébalo siempre antes de tocar nada: si dos parches comparten carpeta, **un option file de un parche puede romper el otro**. Copia la carpeta `save` antes de cambiar de parche.

#### 2.2 Dónde está la base de datos que el juego usa de verdad 🧩

**En simple:** el juego lee varias cajas en orden, y **gana la primera** que tenga el archivo. Hay que saber cuál gana en cada parche.

| Estilo de parche | Ejemplo | Dónde está la base | Cómo encontrarla |
|---|---|---|---|
| **Sider / livecpk** (carpetas sueltas) | ConmeGOL | Una carpeta servida por Sider (en ConmeGOL: `olmosjr23\Database\common\etc\pesdb\`) | En `sider.ini`, la **primera** línea `cpk.root` cuya carpeta tenga `common\etc\pesdb\Player.bin` |
| **Clásico de CPK** | Sudamerican | Dentro de un `.cpk` en `download\` (en Sudamerican: `SP_Subs.cpk`) | En `download\DpFileList.bin`, el **último** CPK de la lista que tenga `pesdb` (los de abajo ganan). Ojo: si además usa Sider con una base suelta, **gana Sider** |
| **Juego «propio»** (exe, lanzador e instalador propios) | Football Life | Su propio sistema | ⏳ Sin estudiar. Hay que auditarlo antes |
| **Sider + base** | Evoweb | Una raíz de Sider | Igual que el primer caso |

- ⚠️ **Riesgo 1:** si se fabrica Phoenix-DB desde la base **equivocada** (por ejemplo, el `.cpk` cuando gana Sider), el juego recibe datos viejos. Pasó en ConmeGOL: el `.cpk` era del 19/8 y la base real del 5/9, con **5.894 jugadores** de diferencia.
- ⚠️ **Riesgo 2:** cuando el parche se actualiza, Phoenix-DB **tapa** la base nueva. Hay que **regenerarla** antes de jugar.

#### 2.3 Cómo poner Phoenix-DB en cada parche 🧩

| Caso | Qué hacer | Riesgo |
|---|---|---|
| El parche usa **un** Sider | Añadir `cpk.root = ".\livecpk\Phoenix-DB"` **antes** de las demás raíces | Bajo |
| El parche usa **varios** Sider o modos (ConmeGOL tiene modos con switcher; Sudamerican tiene Sider normal y Sider Mundial) | Ponerlo en **cada** `sider.ini` y en **cada** carpeta de modo | ⚠️ Si falta en uno, ese modo usará la base del parche sin avisar |
| Un **switcher o lanzador** copia carpetas encima (ConmeGOL usa robocopy; Sudamerican usa `.bat` por letra de disco) | Poner Phoenix-DB también en la carpeta de **origen** que se copia | ⚠️ Una actualización del parche puede borrar la línea. Phoenix Link debe **comprobar** `sider.ini` en cada arranque y avisar |
| El parche **no usa Sider** | Opción A: instalar Sider aparte. Opción B: un `.cpk` propio al final de `DpFileList.bin` | ⚠️ Medio/alto. La opción B necesita el fabricador de CPK (🟡 aún no hecho). **No hacerlo sin respaldo completo** |

#### 2.4 Los IDs de equipos y jugadores 🧩

- Cada parche puede numerar distinto. Ejemplos de ConmeGOL: Real Madrid = ID 109, Santos = 1254, Lamine Yamal = 162114.
- Los parches que usan **IDs reales de Konami** (como Evoweb) se parecen mucho entre sí. Los que crean jugadores propios, menos.
- **Cómo se resuelve:** Phoenix Sync hace un **catálogo por parche** y un **emparejamiento** entre parches. Ya se probó ConmeGOL ↔ Sudamerican: 10.676 jugadores emparejados solos, 3.849 para revisar a mano y 3.716 sin pareja.
- ⚠️ **Riesgo:** nunca usar el catálogo de un parche en otro. Ante la duda, el emparejamiento **no adivina**: lo manda a revisión.

#### 2.5 Los jugadores editados del option file 🧩

- En ConmeGOL el option file trae **21** jugadores editados. Otros parches pueden traer **miles**.
- **Por qué importa:** los editados **mandan** sobre `Player.bin`. Si un parche edita a Lamine en su option file, cambiar su velocidad en Phoenix-DB **no se verá**.
- **Qué hacer:** antes de cambiar stats, Phoenix Sync debe **comprobar** si el jugador está editado. Si lo está, hay que cambiarlo **dentro del option file** (⏳ falta programarlo).

#### 2.6 La Liga Máster en otros parches 🧩

- La estructura es del juego, pero **los tamaños** cambian con el parche: en ConmeGOL hay 700 bloques de equipo, 629 alineaciones y 16.422 fichas.
- Phoenix Sync **no supone esos números**: busca todo por anclas y **se niega a escribir** si algo no cuadra (en vez de estropear el archivo).
- ⚠️ **Riesgo:** **no está probado** en otro parche (⏳). En cada parche nuevo hay que repetir las pruebas clave en una **carrera nueva**:
  1. Un traspaso entre dos equipos de la IA.
  2. Una venta de tu equipo.
  3. Un fichaje para tu equipo.
  4. Cambiar el presupuesto.
  5. Jugar un partido y guardar en otra ranura.
- ⚠️ **Riesgo 2:** muchos parches piden **empezar una carrera nueva** tras actualizar la base. Una carrera vieja con base nueva puede dar plantillas raras.

#### 2.7 La opción «Datos Actual. en vivo» 🧩

- La regla es del juego, pero **un parche puede dejarla activada o desactivada** por defecto, o tener un módulo que la cambie.
- **Qué hacer:** pon los fichajes en **los dos sitios** (option file y `PlayerAssignment.bin` de Phoenix-DB). Así se ven siempre.
- ⚠️ **Riesgo:** si un parche trae su propio `PlayerAssignment.bin` más nuevo y Phoenix-DB se quedó con uno viejo, con la opción activada verías plantillas viejas. Regenerar siempre al actualizar.

---

### 3. Guía paso a paso para un parche nuevo

Haz los pasos **en orden**. Si uno falla, **no sigas**: vuelve al respaldo.

1. **Ficha del parche.** Crea `parches/<parche>.md` con: nombre y versión, carpeta del juego, carpeta `save`, estilo (Sider / CPK / propio), dónde está la base, cuántos `sider.ini` hay y si hay switcher o lanzador.
2. **Huella del «antes».** Anota la huella de la base, `DpFileList.bin`, cada `sider.ini` y el option file. Así sabrás exactamente qué cambió después.
3. **Respaldo completo** de la carpeta `save` y de cada `sider.ini`.
4. **Localiza la base real** (§2.2) y descomprímela.
5. **Catálogo** del parche (`PhoenixSync catalogo …`) y, si vienes de otro parche, **emparejamiento** (`PhoenixSync emparejar …`).
6. **Phoenix-DB** (§2.3): primero con la base **original** recomprimida, sin cambios. Arranca el juego y comprueba que **todo sigue igual**.
7. **Prueba de stats:** un jugador conocido, un cambio visible (por ejemplo, Velocidad). Editar → Cargar → mira su ficha. Comprueba antes que **no esté editado** (§2.5).
8. **Prueba de fichaje:** mueve un jugador en el option file **y** en `PlayerAssignment.bin`. Editar → Cargar → amistoso. Prueba con la opción en vivo **activada y desactivada**.
9. **Liga Máster:** carrera nueva y las 5 pruebas de §2.6, cada una en **otra ranura**.
10. **Anota todo** en `PRUEBAS.md` (salga bien o mal) y en la ficha del parche.

---

### 4. Tabla de riesgos por estilo de parche

| Estilo | Qué funciona seguro | Riesgos principales |
|---|---|---|
| **Sider / livecpk** (ConmeGOL) | ✅ Todo lo de los informes 05, 08, 09 y 10 (probado aquí) | Switcher que pisa `sider.ini`; base real distinta del `.cpk` |
| **Clásico de CPK** (Sudamerican) | 🔎 Option file y emparejamiento. Lo demás ⏳ | Varios Sider (normal y Mundial); módulos cargados dos veces; carpeta `save` compartida; `netblock.lua` bloquea el online |
| **Juego «propio» con exe modificado** (Football Life) | ⏳ Nada probado | El exe cambia límites (ligas de 30, Mundial de 48): la Liga Máster podría tener **otros tamaños**. PESBUL no funciona con exe modificado. **Auditar antes de tocar nada** |
| **Sider + base con IDs reales** (Evoweb) | ⏳ Debería ser el más fácil (IDs de Konami) | No convive con otra base de datos: **nunca mezclar** con otra Phoenix-DB |
| **Paquetes enormes** (Gogosz, ~200 GB) | ⏳ | Espacio en disco y tiempo para respaldos |

**Riesgos que valen para todos:**
- Mezclar archivos de dos parches (base de uno, option file de otro) → jugadores «sin nombre, media 40». Phoenix Sync tiene un **modo seguro** que lo detecta y se niega a escribir.
- Actualizar un parche y no regenerar Phoenix-DB → datos viejos.
- Pulsar **Guardar** en Editar durante una prueba → el option file de prueba se queda para siempre (por eso, siempre respaldo).

---

### 5. Cómo leer los informes anteriores en otro parche

| Informe | Lo que es solo de ConmeGOL | Cómo se traduce a otro parche |
|---|---|---|
| **05 · Puerta en vivo** | La raíz `olmosjr23\Database`; las dos carpetas por el switcher; la carpeta `239200` | La base real del parche (§2.2); cada `sider.ini` del parche (§2.3); su carpeta `save` (§2.1) |
| **08 · Liga Máster: informe** | City = bloque 154, Real Madrid = 135, Santos = 66; ranuras de tu PC | Otros números de bloque: Phoenix Sync los busca solo. Repetir las pruebas (§2.6) |
| **09 · Liga Máster: cómo lo hicimos** | Direcciones de ejemplo (`0xc7dd00`, `0x9D4648`…) y jugadores de prueba (Haaland, Sommer, Guéhi) | El **método** sirve igual: copia → abrir → verdad del juego → comparar → programar → probar. Las direcciones exactas pueden moverse: por eso todo va por anclas |
| **10 · Cómo se abrió el guardado** | La carpeta `239200` y el texto «Manchester City / Premier League» | La llave y el sobre son **iguales en todos los parches** ✅. Solo cambia la carpeta |
| **11 · Súper informe** | Los ejemplos marcados con 🧩 | Esta guía |

---

### 6. Estado por parche (hoy)

| Parche | Option file | Base (Phoenix-DB) | Liga Máster | Comentario |
|---|---|---|---|---|
| **ConmeGOL 26** | ✅ | ✅ | ✅ | Todo probado en el juego |
| **ConmeGOL 27** | ⏳ | ⏳ | ⏳ | Seguir el protocolo del «día de actualización» (informe 04, §7.3) |
| **Sudamerican 2026** | 🔎 (emparejamiento hecho) | ⏳ | ⏳ | Auditoría de carpeta hecha |
| **Football Life 27** | ⏳ | ⏳ | ⏳ | Exe propio: el de más riesgo |
| **Evoweb, Gogosz y otros** | ⏳ | ⏳ | ⏳ | Sin estudiar |

---

### 7. Regla para las guías futuras

Toda guía nueva de Phoenix debe:
1. Separar **lo que pone el juego** (vale para todos) de **lo que pone el parche** (marcado con 🧩).
2. Dar los ejemplos del parche donde se probó, **y** decir cómo encontrar el dato en otro parche.
3. Incluir los **riesgos** de cada paso.
4. Decir en qué parches está **probado** y en cuáles no.

---

## Parte 8 · 19 · Investigación: ¿se puede aumentar el número de equipos de una liga en PES 2021?

> Origen: `PhoenixSync/base-conocimiento/19-INVESTIGACION-MAS-EQUIPOS-POR-LIGA.md` · último cambio 8f50132 2026-10-10 · 13651 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026 · Estado: investigación terminada, SIN pruebas en el juego.**
Resumen en una frase: hasta **30 equipos ya funciona** en ConmeGOL 26; pasar de 30 es **posible pero nadie lo ha documentado**, y el límite probable está en el **calendario de la Liga Máster**, no en la base de datos.

Etiquetas: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche · 💭 estimación mía.

---

### 1. La pregunta y las reglas de esta investigación

- Pregunta de FRALEX: qué posibilidad hay de aumentar la cantidad de equipos que se pueden tener en **una sola liga** del juego. Libertad total en su PC.
- Todo fue **solo lectura**. No se cambió ningún archivo del juego, del parche ni del repo (salvo estos documentos nuevos).
- Se trabajó con **copias en carpetas temporales** de la máquina de trabajo, y se borraron al terminar.
- Ningún dato se inventó: lo que no se pudo comprobar está en la sección 9.

### 2. Herramientas usadas (y para qué)

| Herramienta | Para qué se usó |
|---|---|
| Puente de la app de escritorio (`device_bash`, `device_list_dir`) | Leer los archivos de `D:\Frank\Games_\Conmegol Patch\` y del repo, sin modificarlos |
| Python 3.10 + NumPy 2.2 (en la máquina de trabajo) | Descomprimir los `.bin`, contar filas, escanear el .exe |
| `zlib` de Python | Abrir los `.bin` de la base de datos (cabecera WESYS + zlib) |
| `sha256sum` | Identificar exactamente cada .exe |
| gcc + `dec.c` del repo (`liga-master/herramientas/`) con libpesXcrypter | Descifrar una **copia** de un guardado de Liga Máster; ida y vuelta idéntica ✅ |
| WebSearch / WebFetch | Leer páginas de Football Life, Evoweb, Competition-Server y foros |
| Docs, Drive y Proyecto de Claude | Guardar los informes |

### 3. Archivos de la base de datos analizados 🔎

Carpeta: `D:\Frank\Games_\Conmegol Patch\SiderAddons\olmosjr23\Database\common\etc\pesdb\` (la base viva de ConmeGOL 26).

**Cabecera WESYS (16 B):** `ff 10 81 'W' 'E' 'S' 'Y' 'S'` (8 B) + tamaño comprimido u32 + tamaño descomprimido u32; luego zlib. Se comprobó en 4 archivos (por ejemplo `Competition.bin`: 976 B totales = 16 + 960 comprimidos; 3.240 B descomprimidos).

| Archivo | Comprimido | Descomprimido | Registros |
|---|---|---|---|
| `Competition.bin` | 976 B | 3.240 B | 90 × 36 B |
| `CompetitionEntry.bin` | 8.094 B | 16.308 B | 1.359 × 12 B |
| `CompetitionRegulation.bin` | 9.184 B | 493.920 B | sin decodificar (ver 3.3) |
| `Team.bin` | 42.618 B | 1.147.468 B | 749 × 1.532 B |

#### 3.1 `Competition.bin` (90 competiciones)
Registro de 36 B: bytes 0–1 = `0xC864` · byte 3 = región (por ejemplo `0x10` Argentina) · **byte 5 = ID de la competición** · byte 6 = tipo · desde el byte 8, el nombre ASCII (28 B). Ejemplos: ID 9 = `ARGENTINA_D1_LEAGUE`, 10 = Colombia, 11 = Brasil, 12 = Ecuador, 13 = Chile, 14 = Perú, 15 = `ARGENTINA_D1_CUP`, 22 = Inglaterra, 23 = Italia, 21 = España, 66 = `ARGENTINA_D2_LEAGUE`, 125 = `MLS_D1_LEAGUE`.

#### 3.2 `CompetitionEntry.bin`: quién juega qué ✅ (decodificado)
Cada fila de 12 B = tres u32:
1. **ID del equipo** (el mismo del option file).
2. **Clave única** de la fila (va de 3.014 a 8.405; 1.359 valores distintos).
3. **Empaquetado:** `(puesto << 8) | ID de competición`. El puesto va 1, 2, 3… **sin saltos** en todas las competiciones revisadas.

Contando las filas por ID de competición se obtiene el **tamaño de cada competición** (ConmeGOL 26):

| Competición | Equipos | | Competición | Equipos |
|---|---|---|---|---|
| Matchday eFootball Open | 118 | | Perú 1.ª | 18 |
| Copa Argentina | 66 | | Arabia 1.ª | 18 |
| Mundial (fase final) | 48 | | México 1.ª | 18 |
| Libertadores | 47 | | Francia 1.ª | 18 |
| Sudamericana | 44 | | Ecuador 1.ª | 16 |
| Copa Brasil | 40 | | Chile 1.ª | 16 |
| **Argentina 2.ª** | **36** | | Uruguay 1.ª | 16 |
| Copa Colombia | 36 | | Bolivia 1.ª | 16 |
| **Argentina 1.ª** | **30** | | Venezuela 1.ª | 14 |
| **MLS** | **30** | | Paraguay 1.ª | 12 |
| Brasil 1.ª, Brasil 2.ª, Colombia 1.ª y 2.ª, Inglaterra, Italia, España | 20 cada una | | Costa Rica, Honduras | 12 cada una |

(La lista completa incluye además supercopas de 2 equipos, copas de selecciones y torneos de otras regiones.)

#### 3.3 `CompetitionRegulation.bin` ⏳ (NO decodificado)
- 493.920 B. Lo que se ve son registros de **115 B** con el nombre de la competición en el byte +20, y de vez en cuando registros de 135 B (por ejemplo «KNOCKOUT»).
- Cada competición aparece **20 veces** (por ejemplo «Copa Intercontinental» ×20, «UEFA Champions League» ×300): son los **nombres en 20 idiomas**, por etapa.
- Conclusión honesta: no se encontró todavía dónde está el número de equipos por etapa. Es la mejor pista pendiente.

#### 3.4 `Team.bin`
749 equipos × 1.532 B. Un tutorial citado en la base de conocimiento dice que tiene **750 puestos**, o sea que queda **1 libre**. No hace falta crear equipos: un equipo existente puede cambiar de liga.

#### 3.5 El módulo Competition-Server del propio parche 🔎
`ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\content\Competition-Server\data.csv` tiene las columnas `Offset, TwoSeason, Type1, Type2, Type3, IntComp, TOTS, PromoteNumber, DemoteNumber, SplitFormat, Eligibility, Commentary, Music`. **No tiene columna de número de equipos.** Los desplazamientos de las ligas de 1.ª división van de 264 en 264 bytes (Argentina 4224, Colombia 4488, Brasil 4752, Ecuador 5016, Chile 5280, Perú 5544), o sea un registro de **264 B por competición**, con índice = ID de competición + 7.

### 4. El guardado de Liga Máster: dónde se guarda la lista de equipos 🔎

Copia usada: `_PhoenixMercado_prueba\respaldos_ranuras\ML00000005_ranura6_original_16-3-2026` (19.829.355 B). `dec` lo abrió: 19.811.040 B de datos, **ida y vuelta idéntica ✅**. Se trabajó sobre una copia en la máquina temporal.

- Cada competición guarda su lista de equipos en un **bloque de 3.000 B = 750 puestos de u32**, relleno con `0x0003FFFF`. Los bloques van de 3.000 en 3.000 B.
- Ejemplos medidos en esa copia: Argentina 1.ª en `0xB0D870` = **30** · MLS en `0xB531B8` = **30** · Copa Argentina en `0xB1F1B0` = 66 · Argentina 2.ª en `0xB2D058` = 36 · Copa Colombia en `0xB1FD68` = 36 · Premier `0xB12A78` = 20 · Perú `0xB11308` = 18.
- **Verificado (10 oct, 01:25):** para cada lista se comparó el conjunto de IDs de equipo con el de cada competición de `CompetitionEntry.bin`, y **coincide exactamente** (mismos equipos, mismo número) en las 7 listas anteriores. Esto confirma que `CompetitionEntry.bin` se lee bien.
- Corrección: una primera versión de este documento decía que la lista de `0xB1FD68` era la 2.ª argentina; en realidad es la **Copa Colombia**. La 2.ª argentina está en `0xB2D058`.
- Conclusión: el guardado **no** limita una liga a 20 o 30. El espacio es de 750.

### 5. El .exe del juego 🔎

| Archivo | Tamaño | SHA-256 (inicio) |
|---|---|---|
| `Conmegol Patch\PES2021.exe` (actual) | 458.806.784 B | `5e27a782…bc224a` |
| `Backup\Sudamerican_Backup\PES2021.exe` | 458.806.784 B | `5e27a782…bc224a` (**idéntico**) |
| `Backup\Conmegol Backup\PES2021.exe` | 458.909.184 B | `8aae34ac…abd1` |

- El actual y el de Sudamerican_Backup son idénticos byte a byte.
- Contra el de Conmegol Backup: **15 bytes distintos** en la cabecera y una tabla de exportaciones (`.edata`) más grande (0x3C7B8 contra 0x236D0). El código del juego es el mismo.
- Los tres tienen la misma marca de compilación (`0x5F5ED4BF`, septiembre de 2020) y 19 secciones (`.trace`, `.rdata`, `.data`, `.pdata`, `.xtext`, `.tls`, `.bss`, `.data1`, `.impdata`, `.reloc`, `.xcode`, `.rsrc`, `.text1`, `.xtls`, `.idata`, `.sxdata`, `.sbss`, `.tls$`, `.edata`).
- **Lectura:** ConmeGOL 26 corre ligas de 30 equipos **sin un .exe con parches propios para eso**. ⚠️ No hay un .exe original de Konami para comparar, así que no se puede asegurar que sea 100% el de Konami.
- **Búsqueda de tablas de calendario:** se escaneó `.rdata`, `.data` y `.data1` buscando secuencias que parezcan rondas de un calendario de n equipos (bloques consecutivos de n bytes que son permutaciones de 0..n−1). Resultado: solo hay candidatos para n = 4 y n = 8 (probablemente otros datos); **ninguno para n ≥ 10**. No prueba nada: las tablas podrían estar en u16/u32 o en pares.

### 6. Lo que dicen otros parches y foros 🔎 (con enlaces)

- **Football Life 27** ([página](https://www.pessmokepatch.com/2026/07/FL27.html)): la liga argentina «estaba limitada a 26 equipos» y en FL27 tendrá los 30; la MLS tenía 20 de sus 30 y meter más «bugearía el calendario»; Francia tenía «un límite duro de 39 equipos» entre la 1.ª y la 2.ª; habla de «restricciones ocultas del motor» que causaban fallos o calendarios rotos y de «los primeros avances importantes», **sin explicar cómo**. Dice que la parte de jugabilidad del .exe está casi toda mapeada y que el .exe «requiere una actualización anual».
- **Evoweb 2024 v10** ([página](https://www.pesmodding.com/2024/01/pes-2021-evoweb-patch-2024-version-10.html)): Ligue 2 de 20 a 22 equipos, Suiza 10 en vez de 12, Argentina 26 en vez de 28, Bélgica 18 en vez de 16. Dice que casi no se puede cambiar el número de equipos porque causa fallos o calendarios equivocados, y que reestructuraron ligas para una Liga Máster sin fallos.
- **Evoweb 2022** ([página](https://www.pesmodding.com/2022/05/pes-2021-evoweb-patch-2022-update.html)): tuvieron que volver la liga turca a **21 equipos** porque «el juego fuerza el calendario desde el exe y no se puede cambiar». Con otro número, la Liga Máster fallaba al pasar a la 2.ª temporada; solo se arregla en carreras nuevas.
- **Competition-Server V2.1 de Gerlamp** ([página](https://www.pesmodding.com/2024/01/pes-2021-competition-server-v21-by.html)): módulo Lua para Sider que añade segundas divisiones y competiciones propias con ascensos/descensos. No dice nada de aumentar equipos.
- **Hilo de Evoweb «Create a new league»** ([hilo](https://evoweb.uk/threads/create-a-new-league-in-pes-2021.86342/)): nadie dice haber pasado de 20 equipos.
- En la base de conocimiento del repo (`investigacion/parches-pes2021.md`, `herramientas-y-formatos.md`) ya figuraban: tope de 750 equipos y 40 jugadores por equipo, y «una liga de 40 equipos con parche de exe: NO ENCONTRADO».

### 7. Interpretación 💭 (mía, no medida)

Probabilidad de que una liga funcione en **Liga Máster**, incluida la 2.ª temporada:

| Tamaño | Posibilidad | Razón |
|---|---|---|
| Hasta 30 | Ya funciona 🔎 | Argentina y MLS lo tienen en ConmeGOL 26 |
| 31 a 36 | 40% a 55% | La base de datos lo permite; falta el calendario |
| 37 a 48 | 20% a 35% | Sin documentación; el «39» de Francia sin explicar |
| Más de 48 | menos de 15% | Sin evidencia |

Dónde puede estar el límite, de más a menos probable: (1) el calendario de la Liga Máster; (2) reglas por competición en `CompetitionRegulation.bin`; (3) un tope por grupo de país o región (el 39 de Francia); (4) un arreglo de tamaño fijo en la memoria del .exe; (5) pantallas y menús.
Pista pendiente: el calendario del usuario guarda **10 números de partido por día** (registros de 708 B), pero una liga de 30 equipos juega 15 partidos por jornada. Como funciona, la estructura debe ser más flexible; no está verificado.

### 8. Plan de prueba seguro (30 a 32 equipos) — pendiente de aprobación de FRALEX

1. **Respaldo** de `Database` y `SiderAddons` del parche, con fecha.
2. Claude prepara un `CompetitionEntry.bin` de prueba en una **carpeta nueva** (2 equipos de la 2.ª argentina pasan a la 1.ª: 36→34 y 30→32, puestos 31 y 32). Se mueven, no se copian: un equipo no puede estar en dos ligas a la vez. Hay que rehacer la cabecera WESYS (tamaños comprimido y descomprimido).
3. FRALEX carga esa carpeta con el mecanismo de Phoenix-DB (Sider `cpk.root` antes del parche).
4. Revisión 1: Editar → Estructura de competición debe mostrar 32.
5. Revisión 2: liga de exhibición, 2 o 3 jornadas.
6. Revisión 3 (la importante): Liga Máster **nueva**; guardar en ranura de prueba; Claude cuenta los partidos: 32 equipos a una vuelta = **496**.
7. Revisión 4: simular la temporada completa y pasar a la **2.ª temporada**.
8. Si pasa: repetir con 34, 36 y 40. Anotar cada paso en `PRUEBAS.md`.
9. **Parar** si el juego se cierra, si faltan o sobran partidos, si se repiten rivales o si falla la 2.ª temporada: volver al respaldo y anotar dónde falló.

### 9. Lo que NO se sabe

- Si el .exe es exactamente el de Konami.
- Cómo lograron 30 equipos ConmeGOL y Football Life 27.
- Qué mide el «límite duro de 39» de Francia.
- Dónde está el número de equipos por etapa en `CompetitionRegulation.bin`.
- Qué pasa en la 2.ª temporada con 30 equipos en este parche (se deduce, no se jugó).
- Cómo guarda el calendario una liga de 15 partidos por jornada.
- Si Sudamerican 2026 y otros parches tienen los mismos límites (⏳, 🧩).

### 10. Qué se hizo y qué no

- **Se hizo:** leer y decodificar `Competition.bin`, `CompetitionEntry.bin`, `Team.bin`; abrir una copia de un guardado; comparar tres .exe; escanear el .exe; leer cinco páginas web y los documentos del repo.
- **No se hizo:** ejecutar el juego, cambiar archivos del parche, descifrar `CompetitionRegulation.bin`, encontrar el calendario dentro del .exe.
- **Limpieza:** las copias temporales (`/tmp/lm` con el guardado descifrado, los `.dec` y el script de escaneo) se borraron.

---

## Parte 9 · 21 · Investigación: ¿se pueden añadir MÁS LIGAS y MÁS COMPETICIONES a PES 2021? (por separado)

> Origen: `PhoenixSync/base-conocimiento/21-INVESTIGACION-NUEVAS-LIGAS-Y-COMPETICIONES.md` · último cambio 8f50132 2026-10-10 · 9358 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026 · Solo lectura: no se cambió ningún archivo del juego.**
Etiquetas: [PROBADO] visto funcionando · [OBSERVADO] visto en archivos · [FALTA] no comprobado · [ESTIMACIÓN] juicio mío, no medido.

---

### 0. Respuesta corta

- **Cambiar qué liga ocupa un espacio:** SÍ, ya está hecho en tu parche. [OBSERVADO]
- **Tener MÁS espacios de liga que los del juego original:** NADIE lo ha demostrado. Tu parche no lo hace: tiene 29 filas de liga, el original 30. [OBSERVADO]
- **Tener MÁS espacios de competición (copas, torneos):** tu parche añadió **4 filas nuevas** (IDs 145, 197, 198, 199) y el juego arranca con ellas. [OBSERVADO] Pero son de tipo «especial» (partido/exhibición), no ligas ni copas jugables. Que funcionen como torneo de verdad: [FALTA].
- **Inventar un FORMATO nuevo de torneo:** muy difícil sin tocar el .exe. [ESTIMACIÓN] menos de 10%.

### 1. Cómo se investigó (para repetirlo)

| Paso | Herramienta | Resultado |
|---|---|---|
| Abrir los .bin de tu parche | Python `zlib` (cabecera WESYS de 16 B + zlib) | 90 competiciones, 1.359 filas de equipos |
| Encontrar la base ORIGINAL de Konami | Escaneo de `Data\dt10_x64.cpk` buscando bloques WESYS | `Competition.bin` original = 91 filas; `CompetitionEntry.bin` = 1.291 filas; `CompetitionRegulation.bin` = 498.624 B |
| Comparar original contra ConmeGOL 26 | Python (`cmp.py`) | Tablas de abajo |
| Comparar con Sudamerican 2026 | `datos/competiciones-cgp-vs-sp.json` del repo | 80 filas; mismo mecanismo |
| Leer el módulo que modifica competiciones | `SiderAddons\modules\Competition-Server.lua` y `data.csv` | Tabla en memoria de registros de 264 B |
| Buscar esa tabla dentro del .exe | Búsqueda del patrón del módulo en `PES2021.exe` | Solo está la 1.ª fila; el resto se llena al arrancar (no se puede leer del archivo) |
| Web | Competition-Server V2.1, Custom Competition Style, SP Expansion, UML 2024, hilo de Evoweb | Ver sección 6 |

Aviso honesto: `dt10_x64.cpk` es la mejor «base original» que hay en tu carpeta, pero no tengo un disco de Konami para confirmar que sea 100% intacta.

### 2. LIGAS: lo que se vio

#### 2.1 Los espacios se reutilizan [OBSERVADO]
Cada competición tiene un ID. El original y ConmeGOL usan **los mismos IDs**, pero con otro contenido:

| ID | Original (Konami) | ConmeGOL 26 |
|---|---|---|
| 9 | Inglaterra 1.ª | Argentina 1.ª |
| 10 | Italia 1.ª | Colombia 1.ª |
| 11 | España 1.ª | Brasil 1.ª |
| 21 | Brasil 1.ª | España 1.ª |
| 22 | Argentina 1.ª | Inglaterra 1.ª |
| 39 a 44 | «Liga falsa» Europa/Sudamérica/Asia | Chile 2.ª, Francia, Costa Rica, copas |
| 125 | China 1.ª | MLS |

- De los 86 IDs que comparten, **76 conservan el mismo código de región**. Cambia el país, no el «cajón».
- Sudamerican 2026 hace lo mismo con otros nombres (ID 2 = Champions en uno y Libertadores en el otro). [OBSERVADO]
- Conteo de filas de liga (sin playoffs): original **30**, ConmeGOL **29**. No hay aumento neto.

#### 2.2 Dónde sí hubo «más» [OBSERVADO]
| Cosa | Original (máximo) | ConmeGOL 26 |
|---|---|---|
| Liga de 1.ª con más equipos | 24 (Argentina, Inglaterra 2.ª) | **30** (Argentina, MLS) |
| MLS (en el espacio de China) | 16 | 30 |
| Copa nacional | 44 | **66** (Copa Argentina) |
| Torneo continental | 45 (Europa League) | **47** (Libertadores) |

Esto responde en parte al informe 19: el motor ya ha aceptado ligas más grandes que las de Konami (no probado más allá de 30).

#### 2.3 Segundas divisiones nuevas [OBSERVADO]
Tu parche tiene 2.ª división de Argentina (36), Brasil, Ecuador, Colombia, Chile, Perú y Uruguay. El original solo traía 2.ª de Inglaterra, España, Francia, Italia y Brasil. Se logra con el módulo **Competition-Server** (Gerlamp), que escribe en una tabla en memoria cuántos suben y bajan (máximo 4) y qué clasifica a qué copa. [OBSERVADO en el módulo y su `data.csv`]

#### 2.4 Lo que NO se pudo comprobar [FALTA]
- Si existe una **liga nueva en un ID nuevo** (por ejemplo 142 o 146). Ningún parche revisado la tiene.
- Dónde está la tabla de «formato» de cada liga: se llena al arrancar el juego, no está en el archivo del .exe.
- Cuántos registros tiene esa tabla. El `data.csv` de tu parche llega hasta el registro 163 (México 2.ª), pero esa liga no está en `Competition.bin`, así que la tabla es más grande que la lista de competiciones.

### 3. COMPETICIONES: lo que se vio

#### 3.1 Filas nuevas que sí añadió ConmeGOL [OBSERVADO]
| ID | Nombre | Tipo | Región | Equipos asignados |
|---|---|---|---|---|
| 145 | MATCHDAY_EFOOTBALL_OPEN | 99 | 0 (código nuevo) | 118 |
| 197 | CLASICO_EQUIPOS_CLUBES | 96 | 240 (código nuevo) | 0 |
| 198 | CLASICO_EQUIPOS_AMERICA | 96 | 240 | 0 |
| 199 | LEYENDAS_DEL_FUTBOL | 96 | 240 | 0 |

- El nombre «Leyendas» aparece 20 veces (20 idiomas) en `CompetitionRegulation.bin` de ConmeGOL y 0 en el original. [OBSERVADO] Eso confirma cómo se añade una competición: **fila en Competition.bin + nombre en 20 idiomas en CompetitionRegulation.bin (+ equipos en CompetitionEntry.bin)**.
- Faltan en ConmeGOL, respecto al original, 5 IDs: 42, 82, 118, 120, 121 (copas falsas, copa suiza, copa y supercopa turca).
- IDs libres entre 1 y 200: unos 100, por ejemplo 6, 7, 25, 26, 38, 48, 50 a 55, 130 a 136, 142 a 144, 146 a 196. [OBSERVADO]

#### 3.2 `CompetitionRegulation.bin` NO guarda formatos [OBSERVADO]
Lo que contiene son los **nombres** de cada competición y etapa en 20 idiomas, con una cabecera de 20 bytes por registro. No se vio ahí el número de equipos ni el formato. (Corrige la pista del informe 19, que lo marcaba como el lugar probable del límite.)

#### 3.3 Qué decide el formato [OBSERVADO en el módulo]
El módulo Competition-Server deja cambiar, en la tabla de memoria: tipo (`Division1`, `Division2`, `Cup`, `Knockout`, `Split`, `SuperCup`, `UEL`, `LIB`, `CC_FCWC`), cuántos suben/bajan (0 a 4), a qué copa clasifica, comentarista y música. No crea formatos nuevos: elige entre los que el .exe ya trae.

### 4. Posibilidades (estimación mía, no medida)

| Qué | Posibilidad | Por qué |
|---|---|---|
| Liga que ocupa un espacio existente | Ya funciona | Todos los parches |
| 2.ª división nueva con ascenso y descenso | Ya funciona | Tu parche tiene 7 |
| Liga en un ID nuevo, en partido de exhibición | 30% a 45% | Las 4 filas nuevas arrancan, pero no son ligas |
| Liga en un ID nuevo, jugable en Liga Máster | 10% a 20% | El calendario y la 2.ª temporada dependen del .exe |
| Torneo/copa nuevo con formato ya conocido (copa de eliminación) | 25% a 40% | Hay copas grandes (66) que funcionan en la base |
| Torneo con formato inventado (grupos distintos, etc.) | menos de 10% | Hay que modificar el .exe |

### 5. Anomalía abierta (importante) [FALTA]
En el guardado de Liga Máster de FRALEX, el calendario marca la competición **ID 9** como la Premier. En la base actual de ConmeGOL el ID 9 es **Argentina** y la Premier es el 22. En la base original el ID 9 sí era Inglaterra. Hipótesis A: el guardado se creó con otra base (marzo 2026). Hipótesis B: el calendario usa IDs internos del .exe, distintos de los de `Competition.bin`. Si B es cierta, **cambiar la base a mitad de carrera puede dañar la Liga Máster**; hay que resolverlo antes de añadir competiciones.

### 6. Fuentes web
- Competition-Server V2.1: https://www.pesmodding.com/2024/01/pes-2021-competition-server-v21-by.html (dice que añade 2.ª divisiones con ascenso y descenso; no habla de IDs libres ni de Liga Máster)
- Custom Competition Style: https://www.pesmodding.com/2023/12/pes-2021-custom-competition-style-for.html (cambia solo la presentación de un partido de exhibición; no crea competiciones)
- SP Expansion: https://www.pessmokepatch.com/p/spexpansion.html (22 ligas editables que ocupan las «ligas falsas»; mínimo 14 equipos, máximo 18)
- UML 2024: https://www.pesmodding.com/2023/11/pes-2021-uml-patch-2024-fully-updated.html (MLS, EuroLeague y Saudi Pro League; no explica cómo)
- Hilo de Evoweb: https://evoweb.uk/threads/create-a-new-league-in-pes-2021.86342/ (nadie reporta haberlo logrado; uno dice que «el editor de Ejogc permite añadir ligas» sin haberlo probado)

### 7. Pruebas seguras propuestas (NO ejecutadas; esperan aprobación)

1. **Prueba 0 (sin tocar nada):** en el juego, mirar si «Clásicos» y «Leyendas del fútbol» aparecen en Exhibición y si se pueden jugar.
2. **Prueba 1:** carpeta de prueba con una fila nueva ID 146 (copia de una copa nacional de eliminación), con su nombre en 20 idiomas y 16 equipos. Cargar con Phoenix-DB. Ver si aparece y si se juega una copa completa en Exhibición.
3. **Prueba 2:** igual con una liga nueva (ID 142, tipo 34, región nueva) de 16 equipos. Exhibición primero.
4. **Prueba 3:** solo si 1 y 2 funcionan: Liga Máster nueva con esa liga; pasar a la 2.ª temporada.
5. Antes de todo: resolver la anomalía de la sección 5 y hacer respaldo de `Database` y `SiderAddons`.
6. Parar si el juego se cierra, si falta el nombre de la competición, o si los partidos no cuadran.

### 8. Lo que NO se hizo
No se ejecutó el juego, no se modificó ningún archivo, no se descifró el calendario del .exe, no se probó ninguna competición nueva.

---

## Parte 10 · 22 · Ficha del parche ConmeGOL 26 y plantilla para comparar parches

> Origen: `PhoenixSync/base-conocimiento/22-FICHA-PARCHE-CONMEGOL-26-Y-PLANTILLA-DE-COMPARACION.md` · último cambio 8f50132 2026-10-10 · 11557 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026.** Objetivo: tener TODO lo de ConmeGOL clasificado en un solo lugar (qué trae, qué hace bien, qué hace mal, qué bugs tiene) y una plantilla para compararlo con otros parches y decidir qué copiar al parche Phoenix.
Etiquetas: [OBSERVADO] visto en tus archivos · [PROBADO] visto funcionando · [AUDITORÍA] viene de la auditoría del 8 de oct (`sider/AUDITORIA-SIDER.md`) · [FALTA] sin comprobar · [ESTIMACIÓN] juicio mío.

---

### 1. Identidad

| Dato | Valor |
|---|---|
| Nombre | CONMEGOL PATCH 2026 («ConmeGOL Patch 26») |
| Tipo de parche | Estilo moderno: casi todo suelto por Sider (`livecpk`), base de datos pequeña en un `.cpk` |
| Juego base | eFootball PES 2021 SEASON UPDATE, `PES2021.exe` de 458.806.784 B (16/4/2023) |
| Steam ID usado | 239200 (variante «Clásicos» 239203) |
| Carpeta de guardado | `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\saveConmeGOL Patch 26` |
| Option file | `PES21_Hook\EDIT00000000` (11.026.776 B) |
| Base de datos | `download\CGP_database.cpk` (3,7 MB) + `SiderAddons\olmosjr23\Database` (copia suelta de la base, autor de la base: Olmos Jr 23) |
| Sider | 7.3.3, 61 líneas de módulo, 37 carpetas en `livecpk`, 34 carpetas en `content` |
| Variantes en `ConmeGol Extras` | (1) **ConmeGOL Patch 26** (la normal, con Liga Máster) · (2) **Clásicos (No LM)** (incluye el Mundial de 48 y equipos clásicos; sin Liga Máster) · (3) **ConmeGOL Patch 26 (Perú, Chile, Uruguay B)** (otra mezcla de ligas) |
| Cambio entre variantes | El «switcher» de FRALEX copia la carpeta `SiderAddons` de la variante elegida encima de la del juego |

### 2. Contenido de la base de datos [OBSERVADO]

| Concepto | Original Konami | ConmeGOL 26 | Sudamerican 2026 |
|---|---|---|---|
| Competiciones (`Competition.bin`) | 91 | 90 | 80 (edición Mundial: 81) |
| Filas de equipos por competición | 1.291 | 1.359 | 1.166 (Mundial: 682) |
| Equipos (`Team.bin`) | 663 | 749 | 726 (Mundial: 103) |
| Jugadores (`Player.bin`) | 17.782 | 29.997 | 29.323 (Mundial: 29.471) |
| Liga de 1.ª con más equipos | 24 | 30 (Argentina, MLS) | 30 (Argentina, EE. UU.) |
| Copa más grande | 44 | 66 (Copa Argentina) | 60 (Copa Argentina) |
| Segundas divisiones | 5 (Ing., Esp., Fra., Ita., Bra.) | 7 (Arg. 36, Bra., Ecu., Col., Chi., Per., Uru.) | 5 (Arg. 32, Per., Bra., Chi., Ing.) |
| Torneos continentales | Champions, Europa, Libertadores, AFC, Concacaf | Libertadores 47, Sudamericana 44, Recopa, Champions, Concacaf | Champions, Europa, Libertadores 33, Concacaf |
| Ligas europeas | Ing., Ita., Esp., Fra., Hol., Por., Bél., Rus., Sui., Tur., Din., Esc. | Ing., Ita., Esp., Fra. (solo las 4 grandes + Arabia) | Ing., Esp., Ita., Ale., Fra., Hol. |
| Ligas de otros continentes | China, Tailandia | MLS, México, Arabia Saudita, Costa Rica, Honduras | México, EE. UU. |

Cómo lo hace: **reutiliza los mismos espacios (IDs) del original con otro contenido** (ver informe 21). El ID 9 era Inglaterra y ahora es Argentina, etc.

### 3. Virtudes (lo bueno)

| # | Virtud | Evidencia |
|---|---|---|
| V1 | **Mucho contenido sudamericano licenciado/ordenado:** 7 segundas divisiones, Copa Argentina de 66, Libertadores de 47, Sudamericana de 44 | `Competition.bin` y `CompetitionEntry.bin` |
| V2 | **Ligas de 30 equipos** funcionando con el .exe estándar | Base de datos; el motor lee el número desde un dato con tope 127 |
| V3 | **Parche liviano:** base de datos de 3,7 MB; el resto va suelto por Sider, fácil de cambiar sin reempaquetar | `sider.ini` con `cpk.root` |
| V4 | **Competition-Server** activo: ascensos/descensos y clasificación a copas | `modules/Competition-Server.lua`, `content/Competition-Server/data.csv` |
| V5 | **Mundial de 48 equipos** (variante «Clásicos») con módulos de grupos, llaves, calendario | `FWC_*.lua`; `FWC_Expansion.lua` contiene los parches de Gogosz y los otros tres siguen el mismo esquema [OBSERVADO en `FWC_Expansion`, supuesto en los demás] |
| V6 | **Muchos módulos de presentación:** estadios, kits, balones, cánticos, himnos, comentaristas, cámaras, VAR, banners, marcadores, intros, entradas al campo | Carpeta `modules`, 60 módulos pedidos en `sider.ini` |
| V7 | **Selector de club de Liga Máster** y sala de prensa por club | `MasterLeague.lua`, `content/Master-League-Selector` |
| V8 | **Tres variantes** para distintos usos (Liga Máster, Clásicos/Mundial, otra mezcla de ligas) | `ConmeGol Extras` |
| V9 | Compatible con Liga Máster guardable y descifrable (probado en esta sesión) | Informes 9 a 11 |
| V10 | **Módulo Phoenix** ya integrado en `modules` (versiones 010 a 017m) | `phoenix.lua*` |

### 4. Falencias, bugs y riesgos

| # | Gravedad | Falencia | Evidencia |
|---|---|---|---|
| F1 | Alta | `camera.lua` y `DynamicWideCam.lua` parchean la **misma** instrucción del juego y se pisan | [AUDITORÍA] problema 1 |
| F2 | Alta | `GFX_lod.lua` escribe en una dirección fija pensada para otra versión del .exe, sin comprobar | [AUDITORÍA] problema 2 |
| F3 | Media-alta | `SoundServer.lua` sigue punteros desde una dirección fija sin comprobar y escribe cada fotograma | [AUDITORÍA] problema 3 |
| F4 | Media | Módulos que escriben en memoria cada fotograma sin protección (`CGP_SB_Addons_2`), parches de código grandes (`shirtless_celebration`) | [AUDITORÍA] problemas 4 y 5 |
| F5 | Media | **11 módulos pedidos en `sider.ini` no existen** (tunnel, Derbys, cr7_audio, CornerFlag…) y hay entradas repetidas («Menu Light») | [AUDITORÍA] problemas 8 y 10 |
| F6 | Media | **Estadios asignados dos veces** a la misma competición (grupos del Mundial 2026, Euro, MLS): el módulo avisa «Skipping duplicated assignment» más de 25 veces | `sider.log` (10 oct) |
| F7 | Baja | Falta la configuración de kits de **Real Betis** (g4 y g5) | `sider.log`: «unable to load kit config» |
| F8 | Baja | `UIColors` no encuentra `topModeSelectDMM.bin`; `env.lua` da error | `sider.log` |
| F9 | Media | **Perdió ligas europeas del original** (Países Bajos, Portugal, Bélgica, Rusia, Suiza, Turquía, Dinamarca, Escocia, China, Tailandia) para dar espacio a las sudamericanas | Comparación con `dt10_x64.cpk` |
| F10 | Media | **El Mundial de 48 no se puede usar con Liga Máster** (variante «No LM») | `ConmeGol Extras\Clasicos (No LM)` |
| F11 | Baja | Una misma liga con 2 versiones de nombre entre parches (por ejemplo IDs reutilizados), lo que complica mezclar option files | Informe 21 |
| F12 | Info | **Anomalía abierta:** el calendario de la Liga Máster de FRALEX marca la Premier como competición 9, pero en la base actual el 9 es Argentina | Informe 21 |
| F13 | Info | El `sider.log` revisado antes era de una sesión sin partido; falta log con partido jugado | [AUDITORÍA] |
| F14 | Info | Los módulos de Sudamerican (netblock, jittest, zlibtest…) no aparecen aquí; en ConmeGOL sí están `Anti-Cheat` (placebo, apagado) y `attack_mentality` (apagado, alto riesgo) | [AUDITORÍA] |

### 5. Cosas únicas o curiosas
- El módulo `Competition-Server` y los `FWC_*` son de la comunidad (Gerlamp, Gogosz). ConmeGOL no los escribió; los integró.
- La mayor parte de los datos de estadios, kits y comentaristas viven en `content/` en forma de archivos de texto (`map_competitions.txt`), no dentro del juego.
- Los IDs que ven los módulos de Sider (`tournament_id`) son distintos de los de la base de datos (informe 23, sección 2.4).
- ConmeGOL incluye 4 competiciones con IDs nuevos (145, 197, 198, 199) de tipo «especial».

### 6. Comparación rápida con Sudamerican 2026 [OBSERVADO]

| Tema | ConmeGOL 26 | Sudamerican 2026 |
|---|---|---|
| Estilo | Sider (suelto) | `.cpk` clásicos: 34 archivos; las caras son 13 archivos de 0,7 a 11 GB |
| Tamaño de `download` | ~9,3 GB (7 archivos `dt80` + 3,7 MB de base) | ~135 GB |
| Sider | 1, 60 módulos pedidos | 2 («normal» 56 módulos y «WC26» 66 módulos) |
| Módulo de Liga Máster | `MasterLeague.lua` (selector de club) | `MasterLeague.lua` y `ML.lua` (v1.6, mezcla menús) |
| Mundial de 48 | Variante «Clásicos (No LM)» con `FWC_*` | Edición aparte `Sider WC26` + `SP_Subs_WC.cpk` con `GogoszPatch*` |
| Online | No bloquea | `netblock.lua` **bloquea la red** |
| Módulos de prueba activos | No | `jittest`, `zlibtest`, `etrace` |
| Módulos cargados dos veces | Algunos pares que se pisan | `MenuServer`, `env`, `etrace`, `camera`, `netblock` |
| Errores vistos en el log | Avisos de estadios duplicados, kits Betis, UIColors | `SoundtrackServer` (valor nulo, 5 veces), `randomMenu`, `TurfLoader`, `LogoHD` fallan al iniciar |
| Mismo .exe | Sí | Sí |
| Documentación | Poca | `LEER.txt` con créditos de 30+ personas, Discord oficial |

### 7. Qué conviene copiar o evitar en Phoenix

**Copiar:**
1. Base de datos en un `.cpk` pequeño y todo lo demás por Sider (de ConmeGOL).
2. Competition-Server para ascensos y descensos (ya está en ambos).
3. Técnica de Gogosz (memoria por anclas) para estructuras de torneo propias, **solo si se prueba aparte de Liga Máster**.
4. Variantes separadas por uso (Liga Máster / Mundial / Clásicos).
5. Fichero de créditos y Discord (de Sudamerican).

**Evitar:**
1. Duplicar módulos o dejar módulos de prueba activos (Sudamerican).
2. `netblock.lua` si habrá juego en línea.
3. Pares de módulos que parchean la misma instrucción (cámaras).
4. Escribir a direcciones fijas sin comprobar (GFX_lod).
5. Reutilizar IDs de competición sin anotar la tabla de equivalencias (riesgo para guardados de Liga Máster).

### 8. Plantilla para fichar CUALQUIER otro parche (copia y rellena)

```
PARCHE: ____________  VERSIÓN: ______  FECHA: ______  AUTOR/ES: ______
CARPETA: ______  TAMAÑO TOTAL: ____  CPK PROPIOS: ____  USA SIDER: sí/no (versión __)
.EXE: tamaño ____  sha256 ____  ¿igual al de ConmeGOL? sí/no

BASE DE DATOS (abrir los .bin con Python/zlib):
 competiciones __ | filas de equipos __ | equipos __ | jugadores __
 liga más grande __ | copa más grande __ | segundas divisiones __
 IDs nuevos respecto al original: __ | IDs reutilizados: __

LIGA MÁSTER: ¿funciona? ¿2.ª temporada? ¿guardado descifrable? ¿ligas jugables? __
MÓDULOS (sider.ini): pedidos __ | existen __ | no existen __ | duplicados __
 de competición/torneo: __  de cámara: __  de estadios: __  de kits: __  de sonido: __
LOG (sider.log con partido jugado): errores __ | avisos __
VIRTUDES (con evidencia): __
FALENCIAS Y BUGS (con evidencia y gravedad): __
COSAS ÚNICAS: __
QUÉ COPIAR / QUÉ EVITAR: __
PENDIENTE: __
```

**Cómo sacar los datos (comandos mínimos):**
- Abrir un `.bin` WESYS: leer el archivo, saltar 16 bytes, `zlib.decompress`.
- Competition.bin = filas de 36 B (byte 5 = ID, byte 3 = región, nombre ASCII desde el byte 8). CompetitionEntry.bin = filas de 12 B (el tercer número: `(puesto<<8)|ID`). Team.bin = 1.532 B. Player.bin = 312 B.
- Si la base está dentro de un `.cpk`, buscar la marca `ff 10 81 'WESYS'` y descomprimir cada bloque.
- Módulos de Sider: leer `sider.ini` (líneas `lua.module` y `cpk.root`), comparar con la carpeta `modules`.
- Errores: filtrar `sider.log` por `error`, `fail`, `cannot`, `PROBLEM`.

### 9. Pendiente para completar la comparación
- Evoweb 2024, Football Life 27 y UML 2024 (no están en tu PC: hay que bajarlos o describirlos por la web).
- Sudamerican: terminar la comparación de equipos y jugadores entre ambos parches, y revisar su `sider.log` con partido.
- Repetir la plantilla en cada parche nuevo.

---

## Parte 11 · 23 · Guía: cómo el motor de PES 2021 define ligas y competiciones, y cómo se podría añadir más

> Origen: `PhoenixSync/base-conocimiento/23-GUIA-COMO-EL-MOTOR-DEFINE-LIGAS-Y-COMPETICIONES.md` · último cambio 8f50132 2026-10-10 · 10036 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026 · Solo lectura: no se cambió nada del juego.**
Continúa el informe 21. Aquí está lo que se encontró al mirar DENTRO del .exe, los módulos Lua de tu parche y los del Sudamerican.
Etiquetas: [OBSERVADO] visto en archivos/código · [PROBADO] funciona en el juego (lo hacen otros parches) · [FALTA] sin comprobar · [ESTIMACIÓN] juicio mío.

---

### 1. Lo más importante en 8 líneas

1. **El número de equipos de un torneo está escrito en el código del .exe**, no solo en la base de datos. [OBSERVADO] Para los torneos de selecciones hay una lista fija: Mundial 32, Euro 24, Copa América 12, Copa Asia 16, Copa África 14. Para todo lo demás, el número sale de un dato del juego (un byte con tope de 127).
2. **Eso se puede cambiar con parches de memoria desde Sider.** El módulo del Mundial de 48 equipos (Gogosz) lo hace: cambia 24 a 48 en el código y reescribe las tablas de grupos y partidos. [OBSERVADO en el código de los módulos]
3. El truco de Gogosz: **usar el espacio de la Euro (ID 33) para alojar el Mundial de 48** y dejar el Mundial (ID 27) con 24. [OBSERVADO]
4. Tu parche trae esos módulos (`FWC_Grupos`, `FWC_Expansion`, `FWC_Calendario`, `FWC_Llaves`) pero **solo los enciende en la variante «Clásicos (No LM)»**. «No LM» = no sirve para Liga Máster. Es una pista fuerte de que tocar las tablas rompe la Liga Máster. [OBSERVADO]
5. Los espacios de competición se reutilizan: el original de Konami tiene 91 y ConmeGOL 90. Solo hay 4 IDs realmente nuevos (145, 197, 198, 199). [OBSERVADO]
6. Dos parches independientes (ConmeGOL y Sudamerican) ya usan ligas de **30 equipos** con el mismo .exe. Sudamerican además tiene una Copa Argentina de 60 y una 2.ª división de 32. [OBSERVADO]
7. El tope probable de «127» sugiere que **no hay un límite duro en 30**. El límite real estaría en el calendario de la Liga Máster y la 2.ª temporada. [ESTIMACIÓN]
8. Nadie ha demostrado una liga nueva en un ID nuevo jugable en Liga Máster. Es la gran oportunidad, y también el mayor riesgo.

### 2. Qué se encontró en el .exe [OBSERVADO]

Archivo: `PES2021.exe` (458.806.784 B, el mismo en ConmeGOL y Sudamerican). El código está en la sección `.trace` (en este archivo la sección de código se llama así). Para pasar de «posición en el archivo» a «dirección en memoria»: dirección = 0x140000000 + posición + 0x400.

#### 2.1 La función que fija el número de equipos
En la posición 0x12F0230 (dirección 0x1412F0630) hay una cadena de comparaciones sobre un campo que contiene el **ID de competición** (campo en `[rbx+0x1787B58]`). Según el ID, escribe el número de equipos en `[rbx+0x1787AB0]`:

| ID de competición | Nombre en el original | Equipos escritos por el código |
|---|---|---|
| 27 (0x1B) | Mundial | 32 |
| 33 (0x21) | Euro | 24 |
| 34 (0x22) | Copa América | 12 |
| 35 (0x23) | Copa Asia | 16 |
| 36 (0x24) | Copa África | 14 |
| 37 (0x25) | FAKE_KONAMI | 32 |
| 5 (0x05) | Copa Libertadores (original) | 38 |
| cualquier otro | — | `byte en [competición+0x30A] AND 0x7F` (máximo 127) |

Segundo sitio, en 0xABB7CB: si el modo es «liga» (valor 1) pone **20** equipos; si no, **32** (parece ser la liga/copa que uno crea en el juego).

#### 2.2 El módulo Gogosz (Mundial de 48)
Lo traen Sudamerican WC26 (`GogoszPatch.lua`, `_1`, `_2`, `_3`) y ConmeGOL (`FWC_*.lua`). Comprobado: `FWC_Expansion.lua` de ConmeGOL tiene los mismos parches que `GogoszPatch_1.lua` (incluso el texto «GOGOSZ_PATCH» en sus mensajes); `FWC_Grupos.lua` trae las mismas tablas de grupos. De `FWC_Calendario` y `FWC_Llaves` se supone lo mismo pero no se abrieron. Hace cuatro cosas:
- **`_1` (parches directos al código):** cambia el 24 de la Euro por 48 y el 32 del Mundial por 24; intercambia listas de etapas («EUROSWAP»).
- **`GogoszPatch.lua` (grupos):** busca en memoria las tablas de grupos (12 grupos, 196 bytes cada uno, identificadores de etapa `29 04`, `29 08`… `29 30`) y escribe en ellas los 48 equipos.
- **`_2` (partidos):** reescribe los registros de partido (596 bytes cada uno) para tener los partidos de grupos y la ronda de 32.
- **`_3` (llaves):** reescribe las etapas de eliminación, con 32 equipos de partida, usando «anclas» (secuencias de bytes únicas para encontrar la tabla en memoria).
- Se busca la memoria por patrón en rangos `0x7FF4D1000000` en adelante, y se escribe dentro de un tiempo corto después de cargar el torneo.

**Lectura:** las estructuras de un torneo (grupos, partidos, etapas) están en memoria normal del proceso. Un módulo Lua puede **encontrarlas y reescribirlas**. No hace falta reescribir el .exe.

#### 2.3 El módulo Competition-Server [OBSERVADO]
Modifica una tabla de **registros de 264 bytes por competición** que está en memoria (se encuentra buscando una firma). Campos que deja cambiar: tipo de competición (`Division1`, `Division2`, `Cup`, `Knockout`, `Split`, `SuperCup`, `UEL`, `LIB`, `CC_FCWC`), cuántos suben y bajan (0 a 4), a qué copa clasifica, comentarista y música. No crea registros nuevos: modifica los que ya existen. En el archivo `.exe` la tabla solo trae la primera fila; **el resto se llena al arrancar**.

#### 2.4 Los IDs «de torneo» que ven los módulos [OBSERVADO]
Los módulos Lua reciben un `tournament_id` que combina el número de grupo y la etapa: `(grupo << 10) | etapa`. Ejemplo: 1065 = grupo 1 de la etapa 0x29 (41); 2089 = grupo 2 de la misma etapa. Por eso en el `sider.log` aparecen números como 1065, 2089, 5161, 6185. Es distinto del tercer número de `CompetitionEntry.bin`, que es `(puesto << 8) | ID de competición`.

### 3. Lo que dice la base de datos

- **Base original (dt10_x64.cpk):** 91 competiciones. Incluye 3 «ligas falsas» de Konami (Europa, Sudamérica, Asia, IDs 39, 40, 41) con sus copas y supercopas (42 a 44, 82, 84). [OBSERVADO]
- **ConmeGOL 26:** 90. Reutiliza esos mismos espacios. **Sudamerican:** 80, y su edición Mundial 2026 (`SP_Subs_WC.cpk`): 81 y solo 103 equipos (solo selecciones). [OBSERVADO]
- En la edición Mundial de Sudamerican, la **Euro (ID 33) tiene 48 equipos** y el Mundial (ID 27) 43: es el truco del punto 1.3 del informe. [OBSERVADO]
- `CompetitionRegulation.bin` guarda **nombres en 20 idiomas**, no formatos. [OBSERVADO]
- Un módulo de Sudamerican, `Custom Euro and AFC CL.lua`, **cambia el tournament_id en el momento** para que una competición use el marcador y la presentación de otra. [OBSERVADO]

### 4. Qué significa para añadir ligas y competiciones

| Idea | Qué se sabe | Posibilidad (ESTIMACIÓN) |
|---|---|---|
| Liga más grande que 30 (por ejemplo 32 a 36) | El número sale de un dato con tope 127; hay 2 parches con 30 | 50% a 65% |
| Liga de 37 a 40 | Un parche habla de «límite duro de 39» en Francia | 30% a 45% |
| Más de 40 | Sin evidencia | 10% a 20% |
| Liga en un ID nuevo, en exhibición | Los 4 IDs nuevos arrancan, pero no son ligas | 30% a 45% |
| Liga en un ID nuevo, en Liga Máster | Los módulos de Gogosz se marcan «No LM» | 10% a 20% |
| Torneo de formato nuevo (grupos + eliminatoria distintos) | Gogosz lo logró para el Mundial usando memoria | 35% a 50% para selecciones; 10% a 25% para clubes |
| Más de 4 ascensos/descensos | El módulo solo ofrece 0 a 4 | Sin datos |

### 5. Cosas interesantes que salieron de este análisis

1. El **Mundial de 48** de ConmeGOL está separado en otra variante porque **rompe la Liga Máster**.
2. Konami dejó **3 ligas «falsas»** (Europa, Sudamérica, Asia) con sus copas. Todos los parches las reutilizan para ligas reales.
3. **Sudamerican y ConmeGOL usan exactamente el mismo .exe.** Todo lo que los diferencia son datos y módulos Lua.
4. ConmeGOL llevó la 1.ª de Argentina de 24 (original) a 30; Sudamerican también a 30; la MLS pasó de 16 (en el espacio de China) a 30.
5. El Mundial puede usar el espacio de la Euro. **Se pueden «prestar» espacios entre competiciones.**
6. En el `sider.log` de ConmeGOL aparecen asignaciones de estadios «duplicadas» (por ejemplo para los grupos del Mundial 2026): el módulo de estadios pisa lo que ya había puesto.

### 6. Plan de pruebas por etapas (NO ejecutado; cada paso con permiso de FRALEX)

**Etapa A, solo mirar (riesgo cero):**
- A1. Un módulo Sider de solo lectura que escriba en el log, al abrir el menú de Liga Máster, el byte `competición+0x30A` y el número de equipos de cada liga. Así se confirma que el número sale de los datos.
- A2. En el juego, ver si «Clásicos», «Leyendas» y «Matchday» aparecen en Exhibición.

**Etapa B, base de datos de prueba (carpeta aparte, con respaldo):**
- B1. Liga de 30 a 32 equipos (plan del informe 19).
- B2. Una copa nueva en el ID 146 con 16 equipos (nombre en 20 idiomas), en Exhibición.
- B3. Una liga nueva en el ID 142 con 16 equipos, en Exhibición.

**Etapa C, Liga Máster (solo si B pasó):**
- C1. Liga Máster nueva con la liga de 32. Contar partidos (496).
- C2. Pasar a la 2.ª temporada.
- C3. Probar la liga nueva del ID 142 en Liga Máster.

**Regla de parada:** si el juego se cierra, faltan o sobran partidos, o falla la 2.ª temporada, volver al respaldo y anotar dónde falló.

### 7. Cómo repetir lo que se hizo (herramientas)
- Python 3 (`zlib`, `struct`, `mmap`) para abrir los `.bin` WESYS y buscar patrones en el .exe.
- `capstone` (pip) para desensamblar el código del .exe.
- Escaneo de `.cpk` buscando la marca `ff 10 81 'WESYS'`.
- Lectura de módulos Lua (algunos son bytecode de LuaJIT: se leen las cadenas de texto y los números).

### 8. Lo que NO se sabe
- Si el calendario de una liga de más de 30 equipos funciona en la 2.ª temporada.
- Qué dato exacto guarda `competición+0x30A` y quién lo llena.
- Si una liga nueva puede ser elegida en el menú de Liga Máster (la lista de ligas del menú podría estar fija).
- Por qué el calendario de FRALEX marca la Premier como competición 9 (anomalía del informe 21).

---

## Parte 12 · 27 · Informe completo: módulos Lua de Sider, carpetas y sider.ini, para construir el instalador de Phoenix Link

> Origen: `PhoenixSync/base-conocimiento/27-INFORME-INSTALADOR-SIDER.md` · último cambio 712d3c0 2026-10-10 · 47270 bytes. Copia íntegra.

**Para FRALEX y para la IA que construya el instalador.**
**Fecha:** 10 de octubre de 2026, 05:30 (Lima).
**Autor:** chat SYNC, cuenta B, rama `mercado-fase0`.
**Fuentes:**
- el diario `PhoenixSync/PRUEBAS.md` (8 → 10 oct);
- `PhoenixSync/sider/` (`RIESGOS-SIDER.md`, `VINCULO-TIEMPO-REAL.md`, `AUDITORIA-SIDER.md`);
- `PhoenixSync/core/RutasJuego.cpp`;
- las guías 05, 12 y `parches/*.md`;
- una lectura **en vivo** del PC de FRALEX hecha hoy a las 05:15 (carpetas, `sider.ini`, `sider.log` y huellas).

**Etiquetas:**
- ✅ **PROBADO**: visto funcionar en el juego.
- 🔎 **VISTO**: comprobado en archivos o en el log, sin prueba en pantalla.
- **[DEDUCIDO]**: lo deduzco de cómo funciona algo, pero no lo vi pasar.
- **[NO PROBADO]**: no lo sé o no lo comprobé.

---

### 0. Antes de empezar: qué cubre este informe y qué no

Hay dos familias de `phoenix.lua`, y son muy distintas.

| Familia | Versiones | Qué hace | ¿Entra en el instalador? |
|---|---|---|---|
| **Puente de avisos (solo lectura)** | **v0.2** | Lee un archivo de texto (`avisos.txt`) y lo muestra en el overlay de Sider. No toca la memoria del juego ni sus archivos. | ✅ **Sí.** Es la que documento entera, con su código (sección 8). |
| **Versiones de memoria** | v0.3 a v0.17e (y las de prueba `v0.17a`, `v0.17d`, `v0.17m`) | Leen y **escriben la memoria del juego** con funciones de Windows. Las de la v0.11 en adelante, además, **modifican código del juego en memoria** al arrancar, para activar el botón nativo «Datos Actual. en vivo», que depende del servidor de Konami. | ❌ **No.** No voy a dar instrucciones para instalarlas ni su código. Si las incluyo, el instalador estaría repartiendo a otros PCs un módulo que altera el código del juego mientras corre. Aparte de esa razón mía, es la parte de más riesgo: una lectura directa ya cerró el juego una vez (v0.3, 9 oct, 02:43). |

**Lo que sí funciona sin esas versiones**, y es lo que este informe sí cubre:
- avisos de la web en el juego (v0.2);
- stats y plantillas nuevas con la raíz **Phoenix-DB** + el botón del propio juego **Editar → Cargar** (✅ 9 oct, 03:53);
- todo lo de la Liga Máster, que se hace sobre el guardado con el juego cerrado.

**Recomendación para el instalador:** instalar **solo** `phoenix.lua` v0.2. Hoy en el PC de FRALEX está instalada otra versión, la v0.17e. Que el instalador la sustituya o no lo decide FRALEX. Ver sección 1.3.

---

### 1. MÓDULOS LUA

#### 1.1 · Lista de TODOS los archivos `.lua` de Phoenix

##### En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

| Archivo | Versión | Familia | Estado |
|---|---|---|---|
| `PhoenixSync/sider/phoenix.lua` | 0.17 | memoria | Archivo de trabajo de la otra cuenta. Fuera del instalador. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17a-prueba.lua` | 0.17a | memoria | Prueba. Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17d-prueba.lua` | 0.17d | memoria | Prueba. Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17e-prueba.lua` | 0.17e | memoria | Prueba. **Instalada hoy en el PC** (sha256 `114b6f2e…`). Fuera. |
| `PhoenixSync/sider/pruebas/phoenix-v0.17m-prueba.lua` | 0.17m | memoria | Prueba. Fuera. |
| **v0.2 en el historial de git**, commit `4683988`, ruta `PhoenixMercado/sider/phoenix.lua` | **0.2-prueba** | **avisos** | ✅ **La probada en el juego** (sha256 `116b40a6…`, 5.806 bytes). |
| v0.2 en el historial, commit `f75f677`, ruta `PhoenixSync/sider/phoenix.lua` | 0.2-prueba | avisos | Idéntica a la anterior, salvo **una línea de comentario** (dice «Phoenix Sync» en vez de «Phoenix Mercado»). sha256 `974257ed…`. [NO PROBADO] en el juego, aunque el código es el mismo. |

No hay ningún otro `.lua` de Phoenix. Los otros ~60 módulos de la carpeta `modules` son **del parche** (ConmeGOL) o los añadió FRALEX a mano: el instalador no debe tocarlos.

##### En el PC (las dos carpetas `modules`, raíz y modo)

- `phoenix.lua`: hoy es la v0.17e.
- Copias de respaldo con sufijo:
  - `phoenix.lua.v02`: la **v0.2 probada**, sha256 `116b40a6…`;
  - `.v010`, `.v011`, `.v012`, `.v013`, `.v014`, `.v015b`, `.v016`, `.v017`, `.v017a`, `.v017d`, `.v017m`.
  - Sider **no** carga esas copias, porque solo carga lo que nombra `sider.ini`.

#### 1.2 · Para qué sirve la v0.2 y qué pasa si falta

**Para qué sirve:** muestra en el overlay de Sider los avisos que Phoenix Link deja en un archivo. Es el «buzón» web → juego.

**Cómo se ve:**
1. **Espacio** abre el overlay.
2. La tecla **1** pasa al módulo siguiente, y **º** (`0xC0`) al anterior, hasta llegar a «PHOENIX EVOLUTION».

**Si falta el archivo `phoenix.lua`:**
- Sider anota `PROBLEM: Unable to open file` en `sider.log` y sigue funcionando. 🔎 (7 módulos de ConmeGOL faltan hoy y el juego va bien.)
- Solo se pierden los avisos.

**Si falta la línea `lua.module = "phoenix.lua"` en `sider.ini`:** el módulo no se carga. Ese error ya pasó (ver 6.4).

**Si falta `avisos.txt`:** el overlay dice «sin archivo de avisos». No pasa nada más.

#### 1.3 · Versión recomendada hoy

| Versión | Recomendación |
|---|---|
| **0.2-prueba** (sha256 `116b40a6…`) | ✅ **Recomendada para el instalador.** |
| 0.1 (primera, con `pcall`) | ❌ Descartada: no carga en Sider 7.3.3 (ver 6.4). |
| 0.3 – 0.17e | Fuera de este informe (sección 0). |

#### 1.4 · Qué lee y escribe la v0.2, y cada cuánto

| Qué | Detalle |
|---|---|
| **Lee** | `<sider_dir>content\phoenix\avisos.txt`. `sider_dir` es la carpeta del `sider.exe` que está corriendo (en ConmeGOL, `<juego>\SiderAddons\`). Si Sider no la da, usa `.\`. |
| **Cuánto lee** | Como máximo **4.096 bytes**. |
| **Cada cuánto** | Como mucho cada **2 segundos**, y **solo** mientras el overlay está abierto y Sider le pide el texto. Con el overlay cerrado, **no hace nada**. Usa `os.time()`, que va en segundos enteros, así que en la práctica son 2–3 s. |
| **Escribe** | **Nada** en disco. Solo líneas en `sider.log` (función `log`): una al arrancar y, si hay errores, una por cada error contado. |
| **Memoria del juego** | No la lee ni la escribe. |
| **Eventos de Sider** | Solo `overlay_on`. |
| **Teclas** | Ninguna propia. |
| **Variables y funciones de Sider que usa** | `ctx.sider_dir`, `ctx.register`, `log`, `io.open`, `os.time`, `os.date`, `string`, `table`. |
| **Defensa ante errores** | El Lua de Sider **no trae `pcall`**. El módulo usa una «bandera»: si encuentra que la vez anterior se cortó a mitad, lo cuenta como error. Con **5 errores seguidos** se apaga hasta reiniciar el juego y lo anota en el log. |

---

### 2. CARPETAS Y RUTAS

Todas las rutas son **relativas a la carpeta donde está `PES2021.exe`**. En el PC de FRALEX esa carpeta es `D:\Frank\Games_\Conmegol Patch\`.

#### 2.1 · Árbol necesario (parche ConmeGOL 26)

```
<juego>\                                          ← carpeta de PES2021.exe
├─ SiderAddons\                                   ← la que USA el juego (la «raíz»)
│  ├─ sider.exe, sider.dll, sider.ini             ← del parche (sider.ini lo editamos: 2 líneas)
│  ├─ modules\
│  │  └─ phoenix.lua                              ← nuestro (v0.2)
│  ├─ content\
│  │  └─ phoenix\
│  │     └─ avisos.txt                            ← lo escribe Phoenix Link (buzón)
│  ├─ livecpk\
│  │  └─ Phoenix-DB\
│  │     └─ common\etc\pesdb\
│  │        ├─ Player.bin                         ← lo genera Phoenix Sync (stats)
│  │        └─ PlayerAssignment.bin               ← lo genera Phoenix Sync (plantillas); opcional
│  └─ olmosjr23\Database\common\etc\pesdb\…       ← base del PARCHE: NO TOCAR (Phoenix-DB la tapa)
└─ ConmeGol Extras\
   └─ ConmeGOL Patch 26\                          ← carpeta «de modo» que copia el switcher
      └─ SiderAddons\                             ← COPIA IDÉNTICA de lo nuestro (si no, el switcher lo pisa)
         ├─ sider.ini                             ← con las mismas 2 líneas
         ├─ modules\phoenix.lua
         ├─ content\phoenix\avisos.txt            ← solo el aviso de bienvenida (ver 2.3)
         └─ livecpk\Phoenix-DB\common\etc\pesdb\Player.bin (+ PlayerAssignment.bin)
```

#### 2.2 · Cada carpeta: qué va dentro, de dónde sale y quién lo crea

| Carpeta (relativa a `<juego>`) | Archivo | De dónde sale | Quién lo crea |
|---|---|---|---|
| `SiderAddons\modules\` | `phoenix.lua` | Paquete del instalador (v0.2) | **Instalador** |
| `SiderAddons\content\phoenix\` | (la carpeta) | — | **Instalador**. Phoenix Link **no** debe crearla: si no existe, Link entiende «puente no instalado» (regla del prompt de Link). |
| `SiderAddons\content\phoenix\` | `avisos.txt` | Primero, un aviso de bienvenida; después, los avisos de la web | Bienvenida: **instalador**. Después: **Phoenix Link** (o `PhoenixAviso.bat` a mano). |
| `SiderAddons\content\phoenix\` | `avisos.tmp` | Archivo temporal de la escritura atómica | **Phoenix Link**. Dura milisegundos. |
| `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\` | `Player.bin` | Generado desde la base del parche (`olmosjr23\Database\…\Player.bin` en ConmeGOL) con los cambios de la web. Formato: cabecera WESYS + zlib. | **Phoenix Sync**. La primera vez, el instalador puede poner una copia **idéntica en datos** a la del parche (ver 2.4). |
| `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\` | `PlayerAssignment.bin` | Igual, desde el del parche | **Phoenix Sync**. Opcional: si no está, el juego usa el del parche. |
| `ConmeGol Extras\<modo>\SiderAddons\…` | lo mismo que arriba | Copia | **Instalador** (y Phoenix Sync, cada vez que cambie la base: «en las dos carpetas»). |

**Cómo encuentra Phoenix Sync las carpetas Phoenix-DB** (`core/RutasJuego.cpp`, función `buscarPlayerBin`):
- Recorre hasta **4 niveles** bajo `<juego>`, buscando carpetas llamadas `SiderAddons`, sin distinguir mayúsculas.
- Se salta `download` y `dt`.
- En cada una mira si existe `livecpk\Phoenix-DB\common\etc\pesdb\Player.bin`.
- La de `<juego>\SiderAddons` se llama modo «principal».
- Las demás se llaman por su ruta relativa, por ejemplo `ConmeGol Extras/ConmeGOL Patch 26`.

**El nombre del parche** lo lee de `<juego>\version_actual.txt`, la primera línea (quitando el BOM). Si ese archivo no existe, usa el nombre de la carpeta.

#### 2.3 · Archivos que Phoenix Link escribe o lee, con su formato exacto

##### `avisos.txt` (Link lo escribe; `phoenix.lua` lo lee)

| Propiedad | Valor |
|---|---|
| Ruta | `<juego>\SiderAddons\content\phoenix\avisos.txt`. **La raíz, no la carpeta del modo**: Sider corre desde la raíz. ✅ Así se probó: el aviso se escribió en la raíz y salió en ~1 s. |
| Codificación | **UTF-8 sin BOM**. Si trae BOM, el módulo lo quita. Si no es UTF-8 válido, **no se muestra** y sale «(el aviso no está en UTF-8: no se muestra)». |
| Saltos de línea | `\n`. Si vienen `\r`, el módulo los quita. |
| Límites del módulo | Lee **4.096 bytes** como máximo. Muestra **14 líneas** (si hay más, pone «(...)»). Cada línea tiene un máximo de **110 bytes**: corta sin partir una letra con tilde y añade «...». |
| Límites que debe respetar Link (prompt LINK, buzón) | **14 líneas y 1.500 bytes** como máximo. Avisos del más nuevo al más viejo, cada uno como `[hh:mm] texto` en hora de Lima, con una línea vacía entre avisos. Sin avisos: `Sin avisos nuevos de Phoenix Evolution.` |
| Caracteres | Se borran los de control (salvo el salto de línea). Tildes, ñ, ⚡ y → ✅ se ven bien con la fuente Consolas. |
| Escritura (obligatoria) | **Atómica:** escribir `avisos.tmp` en la misma carpeta. Luego `ReplaceFileW(avisos.txt, avisos.tmp)`, o `MoveFileExW(tmp, txt, MOVEFILE_REPLACE_EXISTING \| MOVEFILE_WRITE_THROUGH)` si `avisos.txt` aún no existe. Si falla porque el juego lo está leyendo, **reintentar hasta 10 veces cada 100 ms**. |
| No reescribir | Si el contenido es igual al último escrito. |
| Archivo vacío | El módulo muestra «(aviso vacío)». |

##### Otros archivos

| Archivo | Quién lo escribe | Quién lo lee |
|---|---|---|
| `Player.bin` / `PlayerAssignment.bin` de Phoenix-DB | Phoenix Sync | El juego, a través de Sider, **solo** al arrancar, en «Datos del sistema: Cargando» y al entrar y salir de Editar. Se aplica con **Editar → Cargar** ✅. |
| `sider.log` | Sider | Phoenix Link puede leerlo para comprobar la instalación (sección 6.3). |

**Por qué hay que escribir Phoenix-DB en las DOS carpetas:** el switcher copia la carpeta del modo encima de la raíz **sobrescribiendo** (sección 5). Si solo se cambia la raíz, el siguiente cambio de variante devuelve la versión vieja. [DEDUCIDO] de las opciones de robocopy; por eso hasta hoy siempre se ha escrito en las dos.

#### 2.4 · Estado real HOY en el PC de FRALEX (05:15) ⚠️

| Archivo | Huella | Qué es |
|---|---|---|
| `SiderAddons\livecpk\Phoenix-DB\…\Player.bin` (raíz y modo) | sha256 `de702dff…` | **v90**: mismos datos que el del parche, solo recomprimido. Correcto. |
| `SiderAddons\livecpk\Phoenix-DB\…\PlayerAssignment.bin` (raíz y modo) | sha256 `1d27f010…` | ⚠️ **De una prueba** (Mbappé y Vinícius en el Barça). **No es el original.** El original es `d1cf73c6…`, igual al de `olmosjr23`; hay copia en `_PhoenixMercado_prueba\db\PlayerAssignment_original.bin`. **El instalador no debe copiar este archivo de prueba.** |
| `SiderAddons\olmosjr23\Database\…\Player.bin` | sha256 `a1e59895…` | Base original del parche. No se toca. |
| `sider.ini` (raíz y modo) | md5 `93ae9604…` | Iguales entre sí, con las 2 líneas de Phoenix. |
| `modules\phoenix.lua` (raíz y modo) | sha256 `114b6f2e…` | v0.17e (familia de memoria). |

---

### 3. sider.ini

#### 3.1 · Las líneas exactas que hay que añadir

**Formato del archivo:** ASCII, finales de línea **CRLF** (`\r\n`), **sin BOM**, una sola sección `[sider]`. Conservar exactamente ese formato al escribir.

##### Línea 1: raíz de la base Phoenix

```ini
cpk.root = ".\livecpk\Phoenix-DB"
```

- **Dónde:** **justo antes** de la línea de la base del parche. En ConmeGOL 26 es `cpk.root = ".\olmosjr23\Database"`.
- Hoy queda así (líneas 89–91):
  ```ini
  cpk.root = ".\livecpk\boot-root"
  cpk.root = ".\livecpk\Phoenix-DB"
  cpk.root = ".\olmosjr23\Database"
  ```
- **Por qué antes:** en Sider, la raíz que va **antes** gana si dos raíces tienen el mismo archivo. ✅ Probado: con esta línea antes de `olmosjr23`, el juego mostró la Velocidad de nuestro `Player.bin` (9 oct, 03:41).
- **Regla general (multiparche):** antes de la **primera** línea `cpk.root` cuya carpeta contenga `common\etc\pesdb\Player.bin`. Si ninguna la contiene (parche «de CPK», como Sudamerican), antes de la primera `cpk.root`. [NO PROBADO] en un parche de CPK.

##### Línea 2: el módulo

```ini
lua.module = "phoenix.lua"
```

- **Dónde:** como **última** línea `lua.module` de la lista.
- En ConmeGOL 26 va justo después de `lua.module = "StartingYearChanger.lua"` (línea 200) y antes del bloque `overlay.*`.
- **Requisito:** `lua.enabled = 1`. En ConmeGOL ya está en la línea 126.

**Nada más.** No hace falta tocar `lua.path`, `overlay.*` ni otras claves. `overlay.enabled = 1` y las teclas (`overlay.vkey.toggle = 0x20`, `next-module = 0x31`, `prev-module = 0xc0`) ya vienen en el parche.

#### 3.2 · Cómo saber si ya están puestas (para no duplicarlas)

Leer línea a línea, **sin distinguir mayúsculas**, ignorando los espacios de los bordes.

- **Ya está la raíz:** hay una línea que **no empieza por `;`** y cumple:
  `^\s*cpk\.root\s*=\s*"\.\\livecpk\\Phoenix-DB\\?"\s*$`
- **Ya está el módulo:** hay una línea que **no empieza por `;`** y cumple:
  `^\s*lua\.module\s*=\s*"phoenix\.lua"\s*$`
- **Si la línea existe pero con `;` delante** (desactivada): quitar el `;`. **No** añadir otra.
- **Comprobación cruzada:** la raíz Phoenix-DB tiene que ir **antes** de la raíz de la base del parche. Si está después, moverla.

#### 3.3 · Riesgos conocidos y cómo se deshace

| Riesgo | Qué pasa | Cómo evitarlo o arreglarlo |
|---|---|---|
| Editar solo la raíz `SiderAddons\sider.ini` | El switcher la **pisa** con la del modo y el módulo deja de cargar. ✅ Pasó el 8 oct. | Editar **también** `ConmeGol Extras\<modo>\SiderAddons\sider.ini`. |
| Una actualización del parche trae otro `sider.ini` | Se pierden las 2 líneas. No se rompe nada, pero Phoenix deja de funcionar sin avisar. | Phoenix Link debe **comprobar en cada arranque** (sección 3.2) y avisar o reponer. |
| Romper el formato (BOM, LF, otra codificación) | [NO PROBADO]. Riesgo de que Sider no lea bien la configuración. | Conservar ASCII + CRLF; escribir con `.tmp` y reemplazar. |
| Línea duplicada | 🔎 El parche ya trae «Menu Light» dos veces y funciona. No se sabe qué hace Sider con un módulo duplicado. [NO PROBADO] | No duplicar (sección 3.2). |
| Módulo nombrado pero archivo ausente | 🔎 Sider anota `PROBLEM: Unable to open file` y sigue. | Inofensivo, pero el instalador debe poner el archivo. |
| Phoenix-DB con datos viejos tras actualizar el parche | La raíz Phoenix-DB **tapa** la base nueva del parche: se servirían jugadores viejos. | Regenerar `Player.bin` desde la base nueva (Phoenix Sync) o quitar la línea hasta regenerarlo. |

**Copias de respaldo:**
- Antes de editar, copiar `sider.ini` a `sider.ini.respaldo-phoenix-AAAAMMDD`, al lado, en **cada** carpeta. Ya existen `sider.ini.respaldo-phoenix-20261009` y `sider.ini.respaldo-phoenixdb-20261009`.
- **No** sobrescribir un respaldo que ya exista: añadir la hora o un número.

**Cómo deshacer:**
- **(a)** Quitar las 2 líneas, o ponerles `;` delante.
- **(b)** O restaurar el respaldo.
- En las **dos** carpetas.
- Comprobado: la diferencia entre el respaldo más viejo y el actual son **exactamente esas 2 líneas** 🔎.

---

### 4. DIFERENCIAS ENTRE PARCHES

#### 4.1 · Tabla por parche

| | **ConmeGOL 26** (variante «ConmeGOL Patch 26») | **ConmeGOL 26 · Perú, Chile, Uruguay B** | **ConmeGOL 26 · Clásicos (No LM)** | **Sudamerican Project 2026** | **Original sin parche** |
|---|---|---|---|---|---|
| Carpeta en el PC | `D:\Frank\Games_\Conmegol Patch\` | la misma (variante) | la misma (variante) | `D:\SP2026\` | [NO PROBADO]: no lo he visto |
| Dónde está Sider | `<juego>\SiderAddons\` (copiada desde `ConmeGol Extras\<variante>\SiderAddons\`) | igual | igual | **Dos** Sider: `<juego>\Sudamerican Project Sider\` y `<juego>\Sudamerican Project Sider WC26\` | No trae Sider [NO PROBADO] |
| Versión de Sider | **7.3.3** 🔎 (`sider.log`) | 7.3.3 [DEDUCIDO]: el mismo `sider.dll` de la raíz | 7.3.3 [DEDUCIDO] | **7.3.4** 🔎 (los dos) | — |
| Cómo arranca | `PES2021 Start.exe` (switcher, sección 5) | igual | igual | `SP LAUNCHER <letra>.bat`: entra en la carpeta de Sider, abre `sider.exe`, espera 2 s y abre `PES2021.exe` con `/affinity 7F` | — |
| ¿livecpk o content? | **livecpk** para casi todo (~110 `cpk.root`) y `content\` para datos de módulos | livecpk (menos raíces: no trae PRDX) | livecpk | livecpk (107 y 98 `cpk.root`) + CPK clásicos | — |
| Base de datos que usa el juego (`Player.bin`) | **`SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin`** (raíz de Sider; gana a la del CPK). También hay una copia en `download\CGP_database.cpk`. | La misma raíz `.\olmosjr23\Database` 🔎 en su `sider.ini`. ⚠️ Su carpeta de variante **no trae** `olmosjr23`: usa la que dejó la variante 26 en la raíz. [DEDUCIDO] | Igual que Perú (raíz `olmosjr23`, sin carpeta propia) [DEDUCIDO] | Dentro de **`download\SP_Subs.cpk`** (CPK clásico). Sus `sider.ini` no tienen raíz de base. 🔎 | Los CPK de Konami (`download\dt80_*`) [NO PROBADO] |
| `download\CGP_database.cpk` | md5 `d2d62b4d…` | md5 `f525fd4c…` (**distinto**) | md5 `cfbad462…` (**distinto**) | — | — |
| Módulos Lua en `sider.ini` | 66 líneas (con `phoenix.lua`) | 54 | 57 | 57 y 72 | — |
| Carpeta de guardados | `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save` | la misma | la misma | `…\292733975847239680\save` | [NO PROBADO] |
| **Phoenix instalado hoy** | ✅ Sí (2 líneas + módulo + Phoenix-DB) | ❌ No | ❌ No | ❌ No | — |
| Qué cambia respecto a ConmeGOL 26 | — | Otras ligas, otro CPK de base y otro `sider.ini` | Sin Liga Máster, otro CPK y otro `sider.ini` | Otro estilo («de CPK»), **dos** `sider.ini`, otra carpeta de guardados, `netblock.lua` (bloquea la red: choca con PESBUL), módulos cargados dos veces | Sin Sider: no hay dónde poner el módulo ni Phoenix-DB sin instalar Sider antes |

**Otros parches** (Football Life, Evoweb, Gogosz, SmokePatch): solo los he estudiado por internet (`investigacion/parches-pes2021.md`). **No están en este PC**: todo [NO PROBADO]. Football Life usa un exe propio: es el de más riesgo.

#### 4.2 · Cómo detectar cada tipo automáticamente

Se mira dentro de `<juego>`, la carpeta de `PES2021.exe`. Phoenix Link la saca del proceso del juego (`QueryFullProcessImageNameW`) o de la configuración.

| Si existe… | Es… | Seguridad |
|---|---|---|
| `PES2021 Start.exe` **y** `ConmeGol Extras\` **y** `version_actual.txt` | **ConmeGOL**. La primera línea de `version_actual.txt` dice la variante: `ConmeGOL Patch 26`, `ConmeGOL Patch 26 (Peru, Chile, Uruguay B)` o `Clasicos (No LM)`. | 🔎 (hoy dice «ConmeGOL Patch 26») |
| `conmegol_info.txt` | Confirma ConmeGOL | 🔎 |
| `Sudamerican Project Sider\sider.ini` y/o `Sudamerican Project Sider WC26\` **y** `download\SP_Subs.cpk` | **Sudamerican 2026** | 🔎 |
| `LEER.txt` + `DpFileList Generator\` | Pista extra de Sudamerican | 🔎 |
| Ninguna carpeta con `sider.exe` + `sider.ini` | Juego **sin Sider** (original u otro parche de CPK) | [NO PROBADO] |
| Una carpeta `SiderAddons` o con `sider.ini` que no encaja en lo anterior | Parche desconocido: hacer solo **diagnóstico** y no instalar nada | regla de prudencia |

**Cuál es la base que usa el juego** (guía 12, §2.2):
- Parche de Sider: la **primera** `cpk.root` cuya carpeta tenga `common\etc\pesdb\Player.bin`.
- Parche de CPK: el **último** CPK de `download\DpFileList.bin` que tenga `pesdb`.
- Si existen las dos cosas, **gana Sider**.

**Cuántos Sider hay:** buscar todos los `sider.ini` hasta 4 niveles bajo `<juego>` (sin entrar en `download`).

#### 4.3 · Qué parches NO son compatibles (hoy) y por qué

| Parche | Por qué no |
|---|---|
| **Original sin Sider** | No hay dónde cargar `phoenix.lua` ni la raíz Phoenix-DB. Habría que instalar Sider aparte: [NO PROBADO], no lo he hecho. |
| **Sudamerican 2026** | **No incompatible, pero sin probar.** Haría falta: las 2 líneas en **sus dos** `sider.ini`; el módulo en sus dos `modules`; y un `Player.bin` generado desde **su** base (`SP_Subs.cpk`, con otros IDs). Además: `netblock.lua` bloquea la red del juego. Esto no afecta a Phoenix, que no usa red dentro del juego, pero sí al online. [NO PROBADO] |
| **ConmeGOL Perú/Chile/Uruguay B y Clásicos** | Compatibles en teoría, pero hoy **no tienen** Phoenix. Y su `CGP_database.cpk` es distinto: un `Player.bin` de Phoenix-DB hecho desde la variante 26 **taparía** su base con datos de otra variante. Cada variante necesita su propio `Player.bin`. [DEDUCIDO] |
| **Football Life** (exe propio) | Exe y lanzador propios. Sin estudiar. **No instalar.** [NO PROBADO] |

---

### 5. CAMBIADOR DE PARCHES (switcher) de ConmeGOL

**Qué es:** `PES2021 Start.exe`, en la carpeta del juego. Es del parche, no de Phoenix: un programa de Python empaquetado con PyInstaller, con interfaz tkinter. 🔎 (`VINCULO-TIEMPO-REAL.md`)

#### 5.1 · Qué hace al elegir una variante (en este orden)

1. Ejecuta `robocopy "ConmeGol Extras\<variante>" "<juego>" /E /IS /IT`.
   - `/E`: copia subcarpetas.
   - `/IS /IT`: **copia y sobrescribe incluso los archivos iguales**.
   - Copia `CPY.ini`, `SiderAddons\` (con `sider.ini`, `modules`, `content`, `livecpk` y, en la variante 26, `olmosjr23`), `conmegol_info.txt` y `download\`.
2. Escribe el nombre de la variante en `<juego>\version_actual.txt`.
3. Abre `<juego>\SiderAddons\sider.exe`, espera 2 s y abre `PES2021.exe`.

No usa internet.

#### 5.2 · Qué pasa con SiderAddons al cambiar de variante

- **No borra nada** de la raíz. Robocopy sin `/PURGE` solo añade y sobrescribe. [DEDUCIDO] de las opciones.
  - Por eso los archivos que no están en la carpeta de la variante **se quedan** en la raíz: por ejemplo `modules\phoenix.lua`, `livecpk\Phoenix-DB\` y `olmosjr23\`.
- **Sí reemplaza** `sider.ini` por el de la variante. Si esa variante no tiene las 2 líneas, Phoenix **deja de cargarse**, aunque los archivos sigan ahí. ✅ Pasó el 8 oct con la variante 26, antes de instalar también en la carpeta del modo.
- **Sí reemplaza** `content\phoenix\avisos.txt` de la raíz por el de la carpeta de la variante (si lo tiene): vuelve el aviso de bienvenida hasta que Link escriba otro. [DEDUCIDO]
- **Sí reemplaza** `livecpk\Phoenix-DB\…\Player.bin` de la raíz por el de la carpeta de la variante (si lo tiene). Si Phoenix Sync solo actualizó la raíz, se pierde el cambio. [DEDUCIDO] → **Escribir siempre en las dos.**

#### 5.3 · Qué hay que reinstalar o comprobar después de cada cambio

| Cambio a… | Qué hacer |
|---|---|
| **ConmeGOL Patch 26** (ya instalada en su carpeta) | Nada, si el instalador puso todo también en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`. Comprobar las 2 líneas en `<juego>\SiderAddons\sider.ini`. |
| **Perú/Chile/Uruguay B** o **Clásicos** | Hoy Phoenix no funciona en ellas. Para que funcione, el instalador tendría que instalar también en **su** carpeta (`ConmeGol Extras\<variante>\SiderAddons\`): 2 líneas, el módulo, y un Phoenix-DB **generado desde la base de esa variante**. [NO PROBADO] |
| Cualquier variante, siempre | Phoenix Link, al arrancar y al detectar que `version_actual.txt` cambió, debe comprobar la sección 3.2 en `<juego>\SiderAddons\sider.ini` y avisar si falta algo. |

**Para que un módulo nuevo cargue** hay que **reiniciar el juego**. Lo más sencillo es con el switcher. `Shift+R` (`vkey.reload-1 = 0x10`, `vkey.reload-2 = 0x52`) solo recarga módulos **que ya estaban activos**: no carga uno recién añadido. ✅ Visto el 9 oct.

---

### 6. PASO A PASO MANUAL ACTUAL (ConmeGOL 26)

Así se dejó listo el PC de FRALEX el 8 y 9 de octubre, solo con la parte que cubre este informe.

#### 6.1 · Pasos

1. **Cerrar el juego** y Sider.
2. **Respaldos** (en cada carpeta, con nombre nuevo):
   - `<juego>\SiderAddons\sider.ini` → `sider.ini.respaldo-phoenix-AAAAMMDD`;
   - `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\sider.ini` → igual.
3. **Módulo:** copiar `phoenix.lua` (v0.2) a:
   - `<juego>\SiderAddons\modules\phoenix.lua`;
   - `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\phoenix.lua`.
4. **Buzón:** crear la carpeta `content\phoenix\` en las **dos** `SiderAddons` y poner un `avisos.txt` de bienvenida (UTF-8 sin BOM). El que se usó:
   ```
   Hola FRALEX: el puente Phoenix ya está instalado.
   Si lees esto dentro del juego, la prueba 1 funcionó.
   ```
5. **Base Phoenix-DB:** crear `livecpk\Phoenix-DB\common\etc\pesdb\` en las **dos** `SiderAddons` y poner un `Player.bin`.
   - El de partida tiene **los mismos datos** que `olmosjr23\Database\common\etc\pesdb\Player.bin` (recomprimido; sha256 `de702dff…`).
   - Copia en `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\Player_v90.bin`.
   - `PlayerAssignment.bin` es opcional. Si se pone, que sea el **original** (`PlayerAssignment_original.bin`, sha256 `d1cf73c6…`).
6. **`sider.ini`** en las **dos** carpetas:
   - añadir `cpk.root = ".\livecpk\Phoenix-DB"` justo antes de `cpk.root = ".\olmosjr23\Database"`;
   - añadir `lua.module = "phoenix.lua"` como última `lua.module`.
   - Comprobar que la diferencia con el respaldo es solo de **2 líneas**.
7. **Arrancar** con `PES2021 Start.exe`, eligiendo «ConmeGOL Patch 26».

#### 6.2 · Cómo se comprueba que quedó bien

**En el juego:**
1. **Espacio** → **1** varias veces → debe salir:
   `PHOENIX EVOLUTION  ·  puente en vivo v0.2-prueba  ·  conectado`
   seguido del texto del aviso. ✅
2. **Tiempo real:** con el juego abierto, reemplazar `avisos.txt` de la raíz (de forma atómica). El texto nuevo sale en **1–3 s** con el overlay abierto, y la cabecera dice «último aviso hh:mm:ss». ✅
3. **Phoenix-DB:** si `Player.bin` lleva un cambio de prueba, debe verse en la pantalla de habilidades del jugador **tras reiniciar**, o **tras Editar → Cargar** sin reiniciar. ✅ (Lamine 99 → 95 → 90.) Con el `Player.bin` de partida (datos originales) no se ve nada distinto. Es lo correcto.

**En `<juego>\SiderAddons\sider.log`** (las rutas cambian según el PC):
```
Using cpk.root: <juego>\SiderAddons\.\livecpk\Phoenix-DB\
Loading module: phoenix.lua ...
[phoenix.lua] [phoenix] v0.2-prueba listo (solo lectura). Archivo: <juego>\SiderAddons\content\phoenix\avisos.txt
```
- 🔎 Las dos primeras líneas tienen ese formato exacto en el log de hoy.
- La tercera sale del código de la v0.2 con el mismo formato que el log real de otra versión: `[phoenix.lua] [phoenix] v0.17d listo …`.

**Señales de fallo en el log:**
- `Module (phoenix.lua) is NOT activated`: el módulo dio error al iniciar.
- `PROBLEM: Unable to open file`: falta el archivo.
- `[phoenix] demasiados errores seguidos`: se apagó solo.

#### 6.3 · Comprobaciones que puede automatizar el instalador

- [ ] Las 2 líneas en **cada** `sider.ini` del parche, y en el orden correcto (sección 3.2).
- [ ] `modules\phoenix.lua` con la huella de la v0.2 en cada carpeta.
- [ ] Existen `content\phoenix\` y `livecpk\Phoenix-DB\common\etc\pesdb\Player.bin` en cada carpeta.
- [ ] Tras el primer arranque: `sider.log` contiene `Loading module: phoenix.lua` y `[phoenix] v0.2-prueba listo`, y **no** contiene `phoenix.lua) is NOT activated`.

#### 6.4 · Errores que ya vimos y su solución

| # | Error | Cómo se vio | Solución |
|---|---|---|---|
| 1 | El módulo no cargaba: `attempt to call global 'pcall' (a nil value)` → `Module (phoenix.lua) is NOT activated` | `sider.log`, 9 oct 00:34 | El Lua de Sider 7.3.3 **no trae `pcall`**. v0.2 sin `pcall` (bandera de errores). ✅ 00:39 |
| 2 | La línea del módulo desaparecía y nunca cargaba | 8 oct | Se había editado solo la raíz y el switcher la pisó. Instalar **también en la carpeta del modo**. |
| 3 | `Shift+R` no cargaba el módulo nuevo | 9 oct | `Shift+R` solo recarga módulos ya activos. **Reiniciar el juego.** |
| 4 | Aviso escrito en la carpeta equivocada | — | Escribir en `<juego>\SiderAddons\content\phoenix\`, la raíz. `sider_dir` es la raíz. |
| 5 | El juego se cerró al leer memoria directamente | v0.3, 9 oct 02:43 | Se volvió a la v0.2. Es una razón más para que el instalador use solo la v0.2. |
| 6 | Entrar y salir de Editar no aplicaba el `Player.bin` nuevo | 9 oct 03:51 | El método correcto es **Editar → Cargar**. ✅ |
| 7 | «Datos Actual. en vivo → Activar» decía que los servicios en línea terminaron | 9 oct | Es la opción nativa, que depende del servidor de Konami (cerrado). No forma parte de este instalador. |
| 8 | Phoenix-DB **tapa** la base del parche | — | Si el parche se actualiza, hay que regenerar `Player.bin` desde la base nueva, o se servirán datos viejos. |
| 9 | `PlayerAssignment.bin` de prueba olvidado en Phoenix-DB | Hoy (sección 2.4) | Antes de empaquetar, usar el **original**. El instalador no debe copiar archivos de `Phoenix-DB` del PC de FRALEX sin comprobar su huella. |

---

### 7. SEGURIDAD Y REVERSA

#### 7.1 · Qué archivos se pisan o se crean

| Archivo | ¿Existía? | Acción del instalador | Respaldo |
|---|---|---|---|
| `SiderAddons\sider.ini` (cada carpeta) | Sí | Se **edita**: +2 líneas | `sider.ini.respaldo-phoenix-AAAAMMDD[-hhmm]` al lado |
| `SiderAddons\modules\phoenix.lua` | Puede existir (hoy: v0.17e) | Se **crea o reemplaza** | Si existe, renombrarlo a `phoenix.lua.respaldo-AAAAMMDD-hhmm` (no borrarlo) |
| `SiderAddons\content\phoenix\avisos.txt` | Puede existir | Se crea solo si no existe | — |
| `SiderAddons\livecpk\Phoenix-DB\…\Player.bin` (y `PlayerAssignment.bin`) | Puede existir | Se crea; si existe, lo gestiona Phoenix Sync, que ya hace sus respaldos en `%APPDATA%\Phoenix Mercado\respaldos\…` | Copia antes de reemplazar |

#### 7.2 · Cómo se restaura

1. Cerrar el juego.
2. En **cada** carpeta (raíz y modo):
   - restaurar `sider.ini` desde su respaldo;
   - o quitar las 2 líneas.
3. Opcional: quitar `modules\phoenix.lua`, `content\phoenix\` y `livecpk\Phoenix-DB\`.
   - Sin la línea `cpk.root`, Phoenix-DB **no se usa** aunque la carpeta exista.
   - Sin la línea `lua.module`, el módulo no carga.
4. Arrancar con el switcher.

#### 7.3 · Lo que JAMÁS se debe tocar

- `PES2021.exe` y cualquier `.dll` del juego o de Sider (`sider.dll`, `sider.exe`).
- **El option file** (`…\save\EDIT00000000`) y los guardados de Liga Máster (`ML0000000x`). Los toca **solo** Phoenix Sync, con el juego cerrado y con respaldo. Nunca el instalador.
- `download\*.cpk`, `download\DpFileList.bin`, `Data\*.cpk`.
- La base del parche: `SiderAddons\olmosjr23\Database\…`. Phoenix-DB la **tapa**; no se cambia.
- Los otros módulos del parche y sus líneas en `sider.ini`, aunque tengan fallos conocidos (`AUDITORIA-SIDER.md`). Limpiarlos es otra tarea, con permiso de FRALEX.
- `PES2021 Start.exe`, `version_actual.txt`, `CPY.ini`, `conmegol_info.txt`.
- El archivo `hosts` de Windows (PESBUL).
- **Ningún módulo que lea o escriba la memoria del juego** (sección 0).

---

### 8. ARCHIVOS REALES

#### 8.1 · Dónde están (PC de FRALEX)

| Qué | Ruta completa | Huella | ¿Empaquetar? |
|---|---|---|---|
| **`phoenix.lua` v0.2 (probada)** | `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\phoenix.lua.v02` | sha256 `116b40a67b6e44a34ae96822ff597e4f456b50a435635ab763d5e00596ff1e0e`, 5.806 B, UTF-8, saltos LF | ✅ Sí. Renombrar a `phoenix.lua` al instalar. |
| La misma, en el repo | commit `4683988`, `PhoenixMercado/sider/phoenix.lua` | igual | (fuente) |
| `PhoenixAviso.ps1` | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\PhoenixAviso.ps1` y repo `PhoenixSync/sider/PhoenixAviso.ps1` | — | Opcional: herramienta manual para enviar avisos |
| `PhoenixAviso.bat` | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\PhoenixAviso.bat` y repo `PhoenixSync/sider/PhoenixAviso.bat` | — | Opcional |
| `avisos.txt` de bienvenida | `D:\Frank\Games_\Conmegol Patch\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\content\phoenix\avisos.txt` (105 B) | — | Opcional. Mejor generarlo. |
| `Player.bin` de partida (= datos del parche) | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\Player_v90.bin` | sha256 `de702dff…` | ⚠️ **No conviene empaquetarlo:** vale **solo** para ConmeGOL 26 en su versión actual. Lo correcto es que Phoenix Sync lo genere en cada PC desde la base de **su** parche. |
| `PlayerAssignment.bin` original | `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\db\PlayerAssignment_original.bin` | sha256 `d1cf73c6…` | Igual que el anterior: mejor generarlo. |
| Respaldos de `sider.ini` | `D:\Frank\Games_\Conmegol Patch\SiderAddons\sider.ini.respaldo-phoenix-20261009` (sin ninguna línea Phoenix) y `…respaldo-phoenixdb-20261009` (con el módulo, sin Phoenix-DB). También en `…\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`. | md5 `80ad499f…` / `e09d13da…` | No (son del PC de FRALEX) |

#### 8.2 · Contenido de `phoenix.lua` v0.2 (el que se empaqueta)

Es la versión exacta probada en el juego (sha256 `116b40a6…`). Hay que guardarla como **UTF-8**. Sider la aceptó con saltos LF.

```lua
-- =============================================================================
--  Phoenix Evolution · Puente en vivo para Sider (PES 2021)
--  Prueba 1 — SOLO LECTURA: lee un archivo de texto y lo muestra en el overlay
--  de Sider. NO lee ni escribe la memoria del juego, NO registra hooks del
--  partido, NO cambia archivos del juego ni teclas. Si algo falla, se apaga
--  solo y lo anota una vez en sider.log.
--
--  Archivo que lee:  <SiderAddons>\content\phoenix\avisos.txt  (UTF-8, ≤ 4 KB)
--  Quién lo escribe: Phoenix Link / Phoenix Mercado (o PhoenixAviso.bat)
--  Cómo se ve:       Espacio abre el overlay de Sider; con 1 / º se pasa de
--                    módulo hasta llegar a «Phoenix Evolution».
--  Desinstalar:      quitar (o poner ; delante de) la línea
--                    lua.module = "phoenix.lua"  en sider.ini
-- =============================================================================

local m = { version = "0.2-prueba" }

local CADA_SEG    = 2      -- cada cuántos segundos se vuelve a mirar el archivo (solo con el overlay abierto)
local MAX_BYTES   = 4096   -- nunca se lee más que esto
local MAX_LINEAS  = 14
local MAX_ANCHO   = 110    -- bytes por línea (se corta sin partir letras con tilde)
local MAX_FALLOS  = 5      -- tras 5 errores internos seguidos, el módulo se apaga

local ruta = nil
local ultimoChequeo = -1000
local contenido = "Esperando el primer aviso..."
local estado = "sin datos"
local actualizado = ""
local firmaAnterior = nil
local fallos = 0
local apagado = false

-- Corta una cadena UTF-8 a como mucho n bytes sin dejar una letra a medias.
local function cortarUtf8(s, n)
    if #s <= n then return s end
    local fin = n
    -- retroceder mientras el byte siguiente sea de continuación (10xxxxxx)
    while fin > 0 do
        local b = s:byte(fin + 1)
        if not b or b < 0x80 or b >= 0xC0 then break end
        fin = fin - 1
    end
    return s:sub(1, fin) .. "..."
end

-- ¿Es UTF-8 válido? Si no, no se muestra (evita basura en pantalla si el archivo se escribió mal).
local function esUtf8(s)
    local i, n = 1, #s
    while i <= n do
        local b = s:byte(i)
        local extra
        if b < 0x80 then extra = 0
        elseif b >= 0xC2 and b <= 0xDF then extra = 1
        elseif b >= 0xE0 and b <= 0xEF then extra = 2
        elseif b >= 0xF0 and b <= 0xF4 then extra = 3
        else return false end
        for k = 1, extra do
            local c = s:byte(i + k)
            if not c or c < 0x80 or c > 0xBF then return false end
        end
        i = i + extra + 1
    end
    return true
end

-- Deja el texto seguro para mostrar: sin BOM, sin \r, sin caracteres de control, líneas y ancho limitados.
local function limpiar(s)
    if not esUtf8(s) then return "(el aviso no está en UTF-8: no se muestra)" end
    s = s:gsub("^\239\187\191", "")
    s = s:gsub("\r", "")
    s = s:gsub("[%z\1-\8\11-\31\127]", "")
    local salida, n = {}, 0
    for linea in (s .. "\n"):gmatch("(.-)\n") do
        n = n + 1
        if n > MAX_LINEAS then salida[#salida + 1] = "(...)" break end
        salida[#salida + 1] = cortarUtf8(linea, MAX_ANCHO)
    end
    while #salida > 0 and salida[#salida] == "" do salida[#salida] = nil end
    if #salida == 0 then return "(aviso vacío)" end
    return table.concat(salida, "\n")
end

local function leerArchivo()
    local f = io.open(ruta, "rb")
    if not f then return nil end
    local s = f:read(MAX_BYTES + 1) or ""
    f:close()
    if #s > MAX_BYTES then
        -- archivo demasiado grande: se recorta sin partir una letra
        local fin = MAX_BYTES
        while fin > 0 do
            local b = s:byte(fin + 1)
            if not b or b < 0x80 or b >= 0xC0 then break end
            fin = fin - 1
        end
        s = s:sub(1, fin)
    end
    return s
end

local function refrescar()
    local ahora = os.time()
    if ahora - ultimoChequeo < CADA_SEG then return end
    ultimoChequeo = ahora
    local s = leerArchivo()
    if s == nil then
        estado = "sin archivo de avisos"
        return
    end
    estado = "conectado"
    if s ~= firmaAnterior then
        firmaAnterior = s
        contenido = limpiar(s)
        actualizado = os.date("%H:%M:%S")
    end
end

local function textoOverlay()
    refrescar()
    local cab = string.format("PHOENIX EVOLUTION  ·  puente en vivo v%s  ·  %s", m.version, estado)
    if actualizado ~= "" then cab = cab .. "  ·  último aviso " .. actualizado end
    return cab .. "\n\n" .. contenido
end

-- Ojo: el Lua de Sider NO trae pcall (lo confirma el volcado de env.lua en sider.log).
-- Sider ya atrapa los errores de cada evento y sigue funcionando; para contar fallos usamos una
-- «bandera»: se levanta al entrar y se baja al salir bien. Si al entrar la encontramos levantada,
-- es que la vez anterior hubo un error (Sider la cortó a la mitad).
local enCurso = false

function m.overlay_on(ctx)
    if apagado then return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)" end
    if enCurso then
        fallos = fallos + 1
        log("[phoenix] la vez anterior hubo un error (" .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[phoenix] demasiados errores seguidos: el módulo se detiene hasta reiniciar el juego")
            return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)"
        end
    end
    enCurso = true
    local res = textoOverlay()
    enCurso = false
    fallos = 0
    return res
end

function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    ruta = base .. "content\\phoenix\\avisos.txt"
    ctx.register("overlay_on", m.overlay_on)
    log("[phoenix] v" .. m.version .. " listo (solo lectura). Archivo: " .. ruta)
end

return m
```

#### 8.3 · Contenido de `PhoenixAviso.ps1` (herramienta manual; Phoenix Link hace lo mismo por dentro)

```powershell
# Phoenix Evolution · escribe un aviso para el módulo phoenix.lua de Sider (prueba 1).
# Escritura "atómica": primero a avisos.tmp y luego se reemplaza avisos.txt de una vez,
# así el juego nunca lee un archivo a medio escribir.
param(
    [string]$Texto = "",
    [string]$Carpeta = ""
)
$ErrorActionPreference = 'Stop'
if ($Carpeta -eq "") { $Carpeta = Join-Path $PSScriptRoot '..\SiderAddons\content\phoenix' }
$Carpeta = [IO.Path]::GetFullPath($Carpeta)
if (-not (Test-Path (Join-Path $Carpeta '..\..\sider.ini'))) {
    Write-Host "No encuentro sider.ini dos carpetas arriba de: $Carpeta" -ForegroundColor Red
    Write-Host "Pasa la carpeta correcta con -Carpeta '...\SiderAddons\content\phoenix'"
    exit 1
}
New-Item -ItemType Directory -Force -Path $Carpeta | Out-Null
if ($Texto -eq "") { $Texto = Read-Host 'Escribe el aviso (usa | para pasar a otra línea)' }
$Texto = ($Texto -replace '\s*\|\s*', "`n").Trim()
if ($Texto.Length -gt 1500) { $Texto = $Texto.Substring(0, 1500) }
$tmp = Join-Path $Carpeta 'avisos.tmp'
$dst = Join-Path $Carpeta 'avisos.txt'
[IO.File]::WriteAllText($tmp, $Texto + "`n", (New-Object Text.UTF8Encoding $false))
$hecho = $false
for ($i = 0; $i -lt 20 -and -not $hecho; $i++) {
    try {
        if (Test-Path $dst) { [IO.File]::Replace($tmp, $dst, $null) } else { [IO.File]::Move($tmp, $dst) }
        $hecho = $true
    } catch { Start-Sleep -Milliseconds 100 }   # el juego lo estaba leyendo justo en ese instante: se reintenta
}
if (-not $hecho) { Write-Host "No se pudo reemplazar avisos.txt (¿bloqueado?)." -ForegroundColor Red; exit 1 }
Write-Host "Aviso enviado. En el juego: Espacio para abrir el overlay y 1 hasta ver PHOENIX EVOLUTION." -ForegroundColor Green
```

#### 8.4 · Contenido de `PhoenixAviso.bat`

```bat
@echo off
rem Phoenix Evolution · enviar un aviso al juego (modulo phoenix.lua de Sider). Se puede usar con el juego abierto.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PhoenixAviso.ps1" %*
pause
```

**Ojo con `PhoenixAviso.ps1`:**
- busca la carpeta del buzón en `..\SiderAddons\content\phoenix`, relativa a donde esté el script;
- comprueba que exista `sider.ini` dos carpetas más arriba;
- se puede pasar otra carpeta con `-Carpeta`;
- corta el texto a 1.500 caracteres;
- convierte `|` en salto de línea;
- reintenta 20 veces cada 100 ms.

---

### 9. Lo que NO sé o no pude comprobar

1. **Original sin parche:** no lo he visto. No sé dónde estaría Sider (no lo trae), ni la carpeta de guardados, ni cómo instalar Sider. [NO PROBADO]
2. **Sudamerican 2026:** no he instalado nada. Faltan:
   - las 2 líneas en sus dos `sider.ini`;
   - generar un `Player.bin` desde `SP_Subs.cpk`;
   - comprobar que la raíz Phoenix-DB gana al CPK (debería, porque Sider gana a los CPK).

   [NO PROBADO]
3. **Variantes Perú/Chile/Uruguay B y Clásicos:** sin Phoenix. No he comprobado en el juego qué pasa al cambiar a ellas y volver. Lo de la sección 5.2 está deducido de las opciones de robocopy. [DEDUCIDO / NO PROBADO]
4. **Sider con una línea duplicada** (`cpk.root` o `lua.module` dos veces): no sé si da error, si carga el módulo dos veces o si lo ignora. [NO PROBADO]
5. **`sider.ini` con BOM o con saltos LF:** no sé si Sider lo lee bien. [NO PROBADO]
6. **Llamada al overlay:** no he medido si Sider llama a `overlay_on` de **todos** los módulos o solo del elegido con el overlay abierto. Por el diseño, el coste es mínimo en los dos casos. [NO PROBADO]
7. **Versión exacta de Sider** en las variantes Perú y Clásicos: deduzco 7.3.3 porque el `sider.dll` que se ejecuta es el de la raíz, y las variantes no traen `sider.dll` propio. [DEDUCIDO]
8. **Los saves de la otra carpeta** (`292733975847239680`): son de Sudamerican, pero no he comprobado si ese parche guarda siempre ahí. [NO PROBADO]
9. **Instalación limpia en un PC de amigo:** nunca se ha hecho. Todo lo probado es en el PC de FRALEX. [NO PROBADO]
10. **Antivirus o permisos de Windows** al escribir en `D:\…\SiderAddons`: no vi problemas en este PC; en otros, no lo sé. [NO PROBADO]
11. **Actualización de ConmeGOL (27 u otra):** no sé si su instalador pisa `ConmeGol Extras\`, y con ello las 2 líneas. [NO PROBADO]
12. **El instalador de Link:** no existe todavía. Tampoco existe en Link el código del «cartero» de avisos: está pedido por prompt (`PhoenixSync/prompts/PROMPT-LINK-buzon-juego.md`), pero no he comprobado si ya está hecho en la rama de Link. [NO PROBADO]
13. **Versiones de memoria (v0.3–v0.17e):** las mantiene la otra cuenta. No las documento aquí (sección 0). Tampoco he comprobado si conviven bien con la v0.2: no pueden estar las dos a la vez, porque las dos se llaman `phoenix.lua`.

---

## Parte 13 · 32 · Idea: un "mundo compartido" entre varias Ligas Máster conectadas

> Origen: `PhoenixSync/base-conocimiento/32-IDEA-MUNDO-COMPARTIDO-ENTRE-LIGAS-MASTER.md` · último cambio 4353058 2026-10-10 · 11215 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 21:49 (Lima). Actualizado 22:23.**
Esto **no está construido todavía para la Liga Máster** — es una idea pensada en conversación, para dejarla anotada antes de olvidarla. No se tocó ningún archivo del juego ni se escribió código nuevo en esta guía. Cuando se decida construirla, hay que volver a leer esta guía y planear la implementación paso a paso (y pedir el OK de FRALEX antes de tocar nada).

> **⚠️ Aviso importante (añadido 22:23):** mientras se pensaba esta idea, **la otra cuenta/frente ya construyó y probó una base real** para exactamente este tipo de "mundo compartido", pero a nivel de **option file** (fuera de una carrera de Liga Máster ya empezada): ver `PhoenixSync/PLAN-CONTROL-TOTAL.md` y el motor `core/SyncCompartido.cpp` — grupos de varias PCs, fichajes que viajan de una PC a otra, una "foto oficial" que publica un admin, rechazo de cambios que no coinciden (muy parecido a la regla de "gana el que llega primero" de la sección 2 de aquí), y un comando para deshacer. **Esta guía 32 sigue siendo útil específicamente para la parte de Liga Máster** (una carrera ya empezada, con las barras de crecimiento) — esa parte sí sigue sin programarse, está anotada como pendiente en `PLAN-CONTROL-TOTAL.md` §2 y §4.4. Antes de construir nada de esta guía, hay que leer primero `PLAN-CONTROL-TOTAL.md` para no reinventar el motor de sincronización que ya existe — esta idea debería encajar **dentro** de `SyncCompartido`, no al lado.

---

### 0. La pregunta que lo originó

FRALEX preguntó: si él tiene su Liga Máster con el Barcelona y un amigo tiene la suya con el Real Madrid (cada uno en su propia PC, cada uno jugando su propia carrera), ¿se puede hacer que un fichaje hecho en una de las dos Ligas Máster — incluso uno hecho por la CPU, no solo por la persona — se refleje también en la Liga Máster del otro? ¿Es posible simular que los dos "juegan el mismo online" aunque en realidad sean dos partidas separadas?

Respuesta corta: **sí es posible**, reusando piezas que ya existen (Phoenix Sync, la web, el mecanismo de "Mercado"), pero aparecen problemas nuevos que hay que resolver con reglas claras, no solo con más código.

---

### 1. La idea central: dos capas, no una

Ya existen, dentro del propio juego, dos capas distintas en cada Liga Máster:

- **Capa profunda:** la carrera real de cada persona — su dinero, sus contratos, el calendario, las decisiones de toda la temporada. Esto es privado de cada uno, vive dentro de su guardado (`MLxxxxxxxx`) y de su option file.
- **Capa de encima ("datos en vivo"):** el mecanismo que ya usa Phoenix-DB (`PlayerAssignment.bin` + el botón "Activar") para decir "este jugador se muestra en este equipo", sin tocar el dinero ni los contratos de nadie.

**La idea clave:** lo que se comparte entre Ligas Máster conectadas es **solo la capa de encima** — nunca la capa profunda. Esto no es una limitación resignada, es la solución a varios problemas a la vez (ver sección 3).

---

### 2. Cómo funcionaría, paso a paso

1. **La web guarda "la verdad" de un mundo compartido:** una lista en Supabase de "este jugador está en este equipo ahora", parecida a lo que ya hace el Mercado con los fichajes manuales.
2. **Cada Phoenix Sync conectado vigila su propia Liga Máster.** Compara "antes" contra "después" (la misma técnica que ya usamos para encontrar las barras de crecimiento) para notar cuándo la CPU local movió a un jugador de equipo.
3. **Cuando nota un cambio, intenta "anotarlo" en la web**, diciendo cuál era el equipo viejo que él tenía anotado (no solo el nuevo). Esto es igual que la regla de seguridad de `palanca_varias`: comprobar el valor viejo antes de escribir.
4. **La web acepta o rechaza ese cambio:**
   - Si el equipo viejo que mandó coincide con lo que la web ya tenía → se acepta, se actualiza la lista compartida.
   - Si no coincide (porque otra Liga Máster ya cambió a ese jugador primero) → se rechaza. Gana el que llegó primero.
5. **Cada Phoenix Sync, al revisar la web, aplica la versión más reciente** sobre la capa "de encima" de su propio juego — igual que ya hace con un fichaje normal del Mercado.

---

### 3. Los tres problemas que aparecen, y su solución

#### 3.1 El dinero y los "efectos en cadena"

Si el fichaje de la CPU de alguien se rechaza, ¿qué pasa con el dinero que ya gastó, o los ajustes de plantilla que ya hizo pensando en ese jugador?

**Solución: no hay que corregir nada, porque nunca se toca.** Como solo se sincroniza la capa "de encima" (qué equipo se *ve*), la capa profunda de cada persona (su dinero, sus contratos) queda intacta y privada siempre. El fichaje de la CPU "pasó" de verdad dentro de su propio mundo — eso no se borra ni se corrige —, pero en la capa compartida ese jugador se ve en otro equipo. Es como llevar cada uno su propio cuaderno de cuentas, mientras en la pizarra del salón (lo que todos ven) solo se anota "quién juega dónde ahora".

#### 3.2 Que se sienta raro ver cambiar a un jugador de la nada

**Solución: un aviso simple.** Cuando Phoenix Sync aplique un cambio que vino de otra Liga Máster, debería dejar una nota visible en Phoenix Link, por ejemplo: *"Mbappé cambió al Real Madrid — fichaje hecho en la Liga Máster de [nombre]."* Así se siente como una noticia del mundo compartido, no como un error.

#### 3.3 Demasiados choques si se sincronizan todos los jugadores

**Solución elegida (FRALEX la prefiere): sincronizar solo una lista corta de "jugadores conectados".**
No se comparten los 500+ jugadores de todas las ligas — eso generaría choques por jugadores que a nadie le importan (un suplente cualquiera). En cambio:
- Se define una lista corta de jugadores "del mundo compartido" (por ejemplo, solo los de nivel alto, las estrellas que de verdad emocionan cuando cambian de equipo).
- Solo esos jugadores entran al mundo compartido y pueden generar un choque.
- Todos los demás jugadores siguen siendo 100% privados de cada Liga Máster — su CPU puede hacer lo que quiera con ellos sin que le importe a nadie más.

Esto reduce muchísimo la cantidad de choques posibles, porque ya no se vigilan cientos de jugadores, solo un puñado.

---

### 4. Cómo quedaría la regla completa, junta

1. Se define una lista corta de jugadores "del mundo compartido".
2. Solo esos jugadores se sincronizan entre Ligas Máster conectadas, usando la capa "de encima" (sin tocar el dinero ni los contratos de nadie).
3. Si hay un choque (dos CPUs mueven al mismo jugador casi al mismo tiempo), gana el que llegó primero a la web — regla de "comprobar el valor viejo antes de aceptar", igual que en `palanca_varias`.
4. El que pierde el choque recibe un aviso simple la próxima vez que sincronice ("este jugador cambió de equipo en otra Liga Máster"), y su capa "de encima" se corrige para coincidir con la web.
5. Todo lo demás (jugadores comunes, dinero, contratos, calendario) se queda privado en cada Liga Máster, sin tocar nunca.

---

### 5. Importante: no es tiempo real como jugar un partido juntos

No se va a ver el cambio en medio de un partido. Se va a ver la próxima vez que el juego revise los datos en vivo (por ejemplo, al entrar al menú o activar esa opción). Tampoco hace falta estar en la misma red de la casa — esto funciona por internet, a través de la web, igual que ya funciona el Mercado.

---

### 6. Idea extra (10 oct, 21:56–22:31): ¿y si la CPU no ficha sola?

FRALEX preguntó si se podía **desactivar los fichajes automáticos de la CPU**, para no tener que resolver choques en absoluto. Se investigó:

- El juego sí tiene un ajuste oficial al crear la Liga Máster, **"Transfer Frequency"** (en el ConmeGOL Patch, en español: **"Frecuencia de traspasos"**, dentro de "Opciones generales"), con 5 niveles — fuente: [evoweb.uk](https://evoweb.uk/threads/most-realistic-fun-ml-setup-pes-2021.93394). **No se confirmó** por internet que ese ajuste controle los traspasos **IA-contra-IA** (solo se documentó que afecta las ofertas que **te llegan a ti** por tus propios jugadores) — fuente: [pesmastery.com](https://pesmastery.com/pes-master-league-transfers/).
- **✅ Confirmado en el juego de FRALEX (22:30, captura de pantalla):** la pantalla real de "Opciones generales" del ConmeGOL Patch muestra **"Frecuencia de traspasos"** con niveles de **Muy baja** a **Muy alta** (5 niveles: Muy baja, Baja, Normal, Alta, Muy alta — por el patrón de la UI). **No hay ningún nivel "Ninguno" o "Apagado"** visible en el menú. El menú no deja elegir "cero" traspasos.
- **Idea para ir más allá del menú (sin probar todavía, ⏳ pendiente):** el menú probablemente guarda ese ajuste como **un número pequeño** dentro de las opciones de la Liga Máster (por ejemplo 1 a 5, uno por cada nivel). El menú solo deja **elegir** entre esos 5, pero el número de adentro podría aceptar otros valores que el juego nunca ofrece, como un **0**. Esto se podría investigar con el mismo método de siempre:
  1. Crear **dos Ligas Máster de prueba** (nunca la carrera real): una en "Muy baja" y otra en "Muy alta".
  2. Comparar los dos archivos **byte a byte** para encontrar el número exacto que cambia entre niveles (misma técnica que ya usamos para las barras de crecimiento y para los traspasos).
  3. Una vez encontrado, probar — **solo en una Liga Máster de prueba, con respaldo** — ponerle un valor fuera del rango normal (por ejemplo 0) y ver qué hace el juego: puede que funcione de verdad, puede que el juego lo ignore y lo corrija solo, o puede que dé un error al cargar esa ranura. **No se sabe hasta probarlo con cuidado.**
- **No es necesario para seguir avanzando:** la sección 3.3 de esta guía (la lista corta de jugadores "del mundo compartido") y la regla de "gana el que llega primero" ya resuelven el problema de los choques, sin necesitar ningún ajuste nuevo ni tocar la memoria del juego. Esto es un experimento opcional, para más adelante, no un paso obligatorio. Mientras tanto, usar **"Muy baja"** en las Ligas Máster conectadas ya debería reducir bastante la cantidad de traspasos de la CPU, de forma simple y sin ningún riesgo.

---

### 7. Pendiente (nada construido todavía)

1. Decidir qué tan larga es la lista de "jugadores del mundo compartido" (¿solo los más famosos? ¿un número fijo por equipo?).
2. Diseñar la tabla en Supabase para esta lista compartida (parecida a la del Mercado, pero con un "contador de versión" por jugador para la regla de choques).
3. Decidir cada cuánto tiempo revisa Phoenix Sync si hay cambios (¿automático? ¿con un botón?).
4. Diseñar el aviso/noticia que se le muestra a cada usuario cuando se aplica un cambio que vino de otra Liga Máster.
5. Probarlo primero con solo 2 personas conectadas (bajo riesgo de choques) antes de pensar en más.
6. (Opcional) Investigar si "Frecuencia de traspasos" se puede forzar más allá de "Muy baja" editando el archivo directamente (sección 6).

---
