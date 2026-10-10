# Liga Máster interna de PES 2021 (`ML0000000N`) — estructura descubierta

Ingeniería inversa hecha comparando guardados reales (antes/después de fichar a Neymar, ventas, 246 traspasos de la IA) y
**probada dentro del juego** con ranuras de prueba. Etiquetas: **[PROBADO]** visto en el juego · **[OBSERVADO]** medido en archivos,
sin prueba visual · **[HIPÓTESIS]** pendiente. Todas las direcciones son del guardado de referencia (City de Fralex, ConmeGOL Patch 26).

## 1. Archivo y envoltura
- Carpeta `…\<SteamID del parche>\save` (ConmeGOL = 239200): `ML00000000`, `ML00000001`… (la ranura N+1 del menú Cargar es el archivo `ML0000000N`),
  `EDIT00000000` (option file), `SYSTEM00000000`, `GRAPHICS000000`.
- Cifrado libpesXcrypter (`terceros/pesxcrypter`, `FileDescriptorNew`): descripción 384 B, logo 17.315 B, serial 44 B.
  `encryptWithKeyNew` no recalcula el hash de la cabecera y **el juego acepta los archivos editados [PROBADO]**.
- **Descripción:** bytes 0–127 = nombre de 128 B (**el menú Cargar de Liga Máster NO lo muestra [PROBADO]**); bytes 128–383 = texto info
  `Equipo / Liga\nFecha\nCompetición` — **es lo que muestra el menú Cargar**. Para distinguir ranuras de prueba hay que cambiar la
  primera línea del texto info (`herramientas/enc2.c`, 5.º argumento).
- El tamaño del archivo cambia al fichar (+3.662 B en el fichaje de Neymar): la herramienta definitiva debe re-cifrar con tamaño variable.

## 2. Contenido descifrado (19.759.355 B en la referencia)
- Cabecera de 80 B (`10, 0x50, 22768964, 10702, 700, 700…`). Los 32 B de 0x30–0x4f cambian en cada guardado y el juego no los valida.
- **700 bloques de equipo** de 1680 B en `0x50 + 1680·k` (hasta 0x11f210). Equipo = índice k: City 154, Santos 66, Real Madrid 135; k ≥ 528 son selecciones
  (Haaland también está en Noruega: se deja). Dentro de cada bloque (s = base + 0x14c):

| Qué | Dónde |
|---|---|
| Nombre | base + 4 |
| ID de equipo del option file | base + 656 (u32) |
| Plantilla: 40 entradas `(reg u32, pid u32)`, relleno `(65535, 0)` | s |
| Dorsales: 40 × u16 | s + 4 + 0x14a |
| Contador de jugadores (1 byte) | s + 4 + 0x2d6 |

- `reg` = ranura global permanente del jugador (no cambia al traspasar: Haaland 4340, Neymar 1936). `pid` = ID de jugador del option file/base
  (Haaland 133543, Neymar 40352, Mbappé 110718).

## 3. Traspaso entre dos equipos de la IA — **[PROBADO]**
Huella validada en 246 movimientos reales del juego y confirmada en juego (Mbappé → Santos aparece perfecto): solo cambian, en origen y destino,
la lista de plantilla (los que se quedan conservan el orden; el nuevo va al final), el array de dorsales y el contador. Nada más.

> **CORRECCIÓN (8 oct, tarde): falta la ALINEACIÓN de cada equipo (§10).** Esa huella solo miró los 700 bloques de equipo. En la
> prueba Mbappé → Santos nadie miró la pantalla *Alineación*. Sin actualizar el bloque de alineación, el que llega sale como un
> jugador en blanco con valoración 0 (visto con Guéhi en el Real Madrid, slot 8).

## 4. Equipo DEL USUARIO: tablas alineadas con la plantilla — **[OBSERVADO; probado en juego solo para quitar]**
El equipo del usuario guarda datos extra por jugador en tablas **con el mismo orden que su lista de plantilla**. Cada registro empieza 8 B antes del pid:
`[x][reg][pid]…`; vacío = `reg` 0 o 0xffff con `pid` 0. Direcciones del guardado «después» (Haaland = índice 9):

| Tabla | pid de Haaland | Stride | Nota |
|---|---|---|---|
| A | 0xbe60a4 | 24 | antes del 1.er registro hay 2 contadores que pasaron 0 → 0x55/0x56 al fichar (sin interpretar) |
| B | 0xc063f4 | 44 | |
| C / D | 0xc51510 / 0xc7a618 | 368 / 192 | 25 primer equipo + 32 juveniles (regs ≥ 16358); los fichajes se añaden al índice 57 |
| E | 0xc7fb38 | 52 | |
| F / G / H | 0xcab55c / 0xcacb38 / 0xcae114 | 108 | |
| M | 0x105611c | 5628 | registros enormes por jugador |
| I / J | 0x1263d4f / 0x12d46ab | 48 / 16 | **después del blob**: la dirección se desplaza con su tamaño |
| **L** | 0xc06178 | 24 | **orden inverso** (crece hacia direcciones bajas): posición p = 25 − índice de plantilla |
| **K = alineación** | 0xb7f960 (K10) | 16 | `[flag, reg, pid, 0]`. K0–K10 = XI en orden de formación, luego la banca (**12 en este guardado**: K11–K22) y las reservas. Flags: K0 = 0, usados 0xc0; primer libre c0 (25 jugadores) → c1 (26); más allá c7 |

Qué pantalla lee qué: **Estrategia lee K**; **Plantilla lee A–M**; **Búsqueda avanzada lee las listas de plantilla** de los bloques de equipo.
Por eso el primer intento (solo la lista de plantilla) dejaba a Haaland «sin equipo» en la búsqueda pero vivo en Estrategia y zona de ventas.

## 5. Sección posterior y blob
Cabecera de sección en 0x1140364 con la palabra de tamaño en 0x11403a8 (0x122969 antes, 0x1237b7 después de fichar) y un blob de ~1,19 MB de
alta entropía (comprimido o mezclado), idéntico entre guardados vecinos. **No se toca [OBSERVADO]**; puede guardar historial viejo (¿inofensivo?, sin confirmar).
La cabecera del blob tiene contadores que pasaron de 25 a 34 al fichar (sin interpretar). El registro de eventos N (0x10b95cc, pares reg/pid + fecha) es histórico: no se toca.

## 6. Resultados en el juego (8 oct 2026, menú Cargar de ConmeGOL)
| Menú | Archivo | Contenido | Resultado visto por Fralex |
|---|---|---|---|
| 1 | `ML00000000` | v2: solo lista de plantilla, pares mal alineados | ✗ Haaland sin equipo en búsqueda; sigue en Estrategia y zona de ventas del City |
| 2 | `ML00000001` | limpia (control) | ✓ Haaland en el City |
| 3 | `ML00000002` | Mbappé → Santos (solo huella IA↔IA) | ✓ Mbappé perfecto en Santos |
| 4 | `ML00000003` | Haaland → Santos, solo listas (sin tablas) | Haaland aparece en Santos; el lado City no se informó |
| 5 | `ML00000004` | Haaland → Santos completo (listas + A…M + L + K) | ✓ Haaland perfecto en Santos, Mbappé sigue en el Madrid; **pendiente**: Estrategia/plantilla del City y que aguante jugar un partido |

## 7. Pendiente
1. Cerrar la prueba de la partida 5 (ver arriba) y, tras jugar, comparar el guardado del juego con la candidata (`diff_gt.py`) para detectar qué retoca el juego (contadores, blob).
2. Sentido inverso (IA → usuario): usar como plantilla los registros del fichaje de Neymar (guardado «antes» vs «después»).
3. Localizar las tablas **por anclas** (la plantilla del usuario conocida) y no por direcciones fijas; equipos de usuario distintos del City; tablas I/J tras el blob.
4. Regla del sustituto en la alineación (hoy: primer jugador de K con índice ≥ 18 y posición ofensiva): revisar para porteros y defensas.
5. Módulo C++ en `core/` (leer bloques, mover IA↔IA, quitar/añadir al usuario, re-cifrar con tamaño variable) con pruebas sobre estos guardados.

## 8. Cómo reproducir (prototipos en `prototipos/`, herramientas en `herramientas/`)
```
dec  <ML0000000N> salida.bin reencriptado.bin                       # descifrar (y comprobar ida y vuelta)
python3 prototipos/build_c.py r0.bin v5.bin catalogo.json 154 66 133543 19   # City → Santos: Haaland, dorsal 19
enc2 <ML original> v5.bin <ML nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'   # cifrar + etiqueta visible en el menú
python3 prototipos/diff_gt.py base.bin guardado_por_el_juego.bin catalogo.json | head -80
```
`enc2` exige mismo tamaño de datos que el original (limitación a quitar en el módulo definitivo). Los `.bin` descifrados son datos del juego: no se versionan.

## 9. Hallazgos del 8 oct (tarde) con el módulo C++ (`core/LigaMaster.cpp`, todo por anclas)
- **El juego aceptó el slot 6 (v6) y jugó un partido** (`ML00000006`). [PROBADO]
  > **CORRECCIÓN:** `ML00000005` ya NO es la candidata v6: el juego la sobrescribió a las 10:53 (Lima) al guardar Fralex en esa
  > ranura. Comparar `ML00000005` con `ML00000006` es comparar dos guardados del juego con 5 min de diferencia, no «candidata vs
  > juego». Entre esos dos solo cambió: cabecera 0x30–0x4f, 6 B en 0xafa2b4, 3 banderas de 1 B y la zona posterior al blob.
- **Las direcciones de §4 solo valen para un guardado**: el blob crece con la temporada (0x1237b7 en el respaldo → 0x12794f en
  `ML00000006`, +16.792 B) y desplaza las tablas I y J. Por eso el C++ no usa direcciones: ancla cada tabla con los 5 primeros
  (reg, pid) de la plantilla, y el orden de formación/K por permutación + espejo.
- **Tabla «A2»** (paso 24, justo después de A, en 0xbe639c): vacía (0xffff) en el respaldo y rellena con los 25 de la plantilla en
  el guardado posterior. El C++ la trata como una tabla alineada más (se compacta igual). [OBSERVADO]
- **C y D: los fichajes NO van en su índice de plantilla** sino al final, tras los 32 juveniles (Neymar, índice 24 de plantilla,
  está en el registro 57). Por eso el C++ busca al jugador por (reg, pid) dentro de cada tabla y compacta ahí.
- **Tabla I (paso 48, tras el blob)** tiene 28 registros: los 24 del primer equipo y luego 11, 24, 14, 10 (repetidos). El juego
  reescribe su contenido tras cada partido. El C++ quita solo la PRIMERA aparición (igual que v6) y anota los repetidos.
- **Flags de K**: K0 = 0 y todos los usados 0xc0 (26 y 25 jugadores); primer libre 0xc1 (26) / 0xc0 (25); después 0xc7.
- **Comprobado byte a byte**: `moverUsuarioAIA` == `build_v6.py` sobre el respaldo (Haaland→Santos, Neymar sustituto) y ==
  `ref_v7_juego.py` sobre `ML00000006` (Guéhi→Real Madrid dorsal 30, Stones sustituto; entregado como `ML00000007`, slot 8).

## 10. Alineación de los equipos de la IA (bloques de 600 B) — **[OBSERVADO; prueba en juego: slot 9]**
- Arreglo de **629 bloques de 600 B** justo después de los bloques de equipo (0x133a30 en estos guardados; el C++ lo busca: índices
  0, 1, 2, 3… cada 600 B con el ID option de cada equipo detrás). Bloque k:

| Qué | Dónde (desde el inicio del bloque) |
|---|---|
| Índice del bloque (= k) | +0 (u32) |
| ID del equipo en el option file | +4 (u32) |
| Nombre del técnico (UTF-8) | +8 |
| Formaciones (varias copias de coordenadas) | … |
| **Orden de formación**: 40 índices de plantilla (0–10 titulares; luego la banca —12 en este guardado— y las reservas) | +0x220 |
| **Roles** (capitán, lanzadores): 6 índices de plantilla | +0x248 |

- Bloques 0–626 = equipos 0–626 (clubes y selecciones). **627 = equipo del usuario** (su orden es el de 0x18f9d8, el que lee
  Estrategia; ID −11). 628 = otra copia del usuario y 154 = el City original (ID 173): el juego **no** los mantiene (iguales en
  todos los guardados aunque cambie la plantilla), así que no se tocan.
- **El juego lee las primeras n entradas del orden (n = plantilla).** Si una de ellas es 0xff, la pantalla *Alineación* muestra un
  jugador en blanco «DC 0» (slot 8: Guéhi en el Madrid; el orden tenía 30 entradas y la plantilla 31). [PROBADO]
- Formatos que aparecen: **compacto** (n entradas + 0xff hasta 40, es lo que escribe el juego), «identidad de 40» (0…39) y
  «0…n−1, ff, n…» en equipos que nunca se reordenaron. Se aceptan los tres si las primeras n entradas son una permutación; al
  editar se escribe siempre en compacto. Si no son una permutación, el C++ **se niega** a tocar ese equipo.
- **Qué hace el juego** (respaldo → guardado posterior, 431 equipos de la IA con cambios): los que se quedan conservan su orden
  relativo; los nuevos van al final de la plantilla; el orden queda siempre compacto con n = plantilla. Si se va un **titular**, una
  **reserva** ocupa su puesto (en las pruebas, de su misma posición) y el resto conserva el orden; si se va un **suplente o una
  reserva**, los de atrás suben un puesto. A los que llegan el técnico de la IA los coloca después (Haaland acabó titular en el Santos).
- **Qué hace el C++:** destino → el que llega entra como **última reserva**; origen → si era titular o tenía un rol, un **sustituto**
  ocupa su puesto (`sugerirSustituto`: reservas primero, misma posición > misma línea > cualquiera; un portero solo lo cubre otro
  portero); si no, los de atrás suben. Roles: los del que se va pasan al sustituto; los índices mayores bajan uno.
- **Fichas por jugador de 596 B** (0x329b00, una por `reg`, 5759): el campo +4 parecía el club pero **no lo es** (no cambió en
  ninguno de 169 traspasos hechos por el juego; Neymar sigue con el código del Santos estando en el City). No se tocan.

| Menú | Archivo | Contenido | Resultado |
|---|---|---|---|
| 8 | `ML00000007` | C++ v7: Guéhi City → Real Madrid (Stones sustituto), sin bloque de alineación | Equipos/búsqueda ✓; City ✓; **Alineación del Madrid: jugador en blanco «DC 0»** ✗ |
| 9 | `ML00000008` | C++ v8: lo mismo + alineación del Madrid (1 byte: orden[30] = 30) + Isak Liverpool → Santos (Ekitiké sustituto, dorsal 99) | ✓ Madrid: Guéhi última reserva con el 30, sin jugador en blanco · ✓ Liverpool: Ekitiké en el puesto de Isak · ✓ Santos: Isak última reserva con el 99 · ✓ City sin cambios (Stones titular). Partido jugado: ✓ (ver §11). Ojo: el juego **autoguardó** encima de esta ranura |
| 10 | `ML00000009` | Guardado por el juego tras el partido de la ranura 9 (31/8/2026) | ✓ base de las pruebas siguientes |

- **La banca es de 12 en este guardado** (visto en las pantallas *Alineación* del slot 9: la raya entre banca y reservas cae
  después del puesto 22). Por eso el C++ ya no supone «11–17 banca»: solo distingue **titulares (0–10)** del resto, y al buscar
  sustituto recorre la lista desde el final (reservas) hacia arriba (banca), sin necesitar el tamaño de la banca.

## 11. Después de jugar un partido (ranura 9 → ranura 10, 31/8/2026) — **[OBSERVADO]**
- **Nuestros cambios se mantuvieron:** Guéhi en el Madrid, Isak y Haaland en el Santos, Ekitiké en el Liverpool, Stones y Neymar en el City.
  Las 627 alineaciones de la IA siguen válidas. El técnico de la IA **reordenó** a su gusto: Isak pasó a titular del Santos (Haaland a la
  banca) y el Madrid dejó a Guéhi de reserva. Además hubo 332 cambios de plantilla de la IA (último día de fichajes).
- **El juego autoguarda** en la ranura cargada al terminar el partido (`ML00000008` quedó sobrescrito 11 s antes de que Fralex guardara
  en la 10). Por eso cada archivo de prueba se identifica por su huella (md5), nunca solo por su nombre.
- **Ficha del último partido** (aparece al jugar): dentro del arreglo de fichas de 596 B (registro 5531, en 0x64e7dc): dos listas de 17
  registros de 16 B `[x][reg][pid][y]` con **los que jugaron** de cada equipo, en orden de plantilla. Como empieza igual que una tabla
  de la plantilla (0, 1, 2, 3, 4…), `tablasDe` la confundía y la venta se **negaba** («TABLA_DANADA»). Arreglo: una tabla de la
  plantilla debe tener a **todos** los jugadores; si no, se descarta y no se toca (es historial).
- **Tabla I (paso 48) = CONTRATOS** [OBSERVADO]: cada registro lleva cantidades (¿sueldo?), una fecha de fin (`eb 07 08 1f` = 31/08/2027),
  una de inicio (`e4 07 07 01` = 01/07/2020), un número de registro único y un tipo (05 / 03). **Un jugador puede tener DOS registros**:
  en el respaldo no había ninguno doble; en los guardados posteriores sí (Rulli, Bettinelli y Stones; el segundo de Stones termina en 2028).
  Hoy el C++ quita solo el primero → el segundo quedaría como un contrato «fantasma». ⏳ Para hacerlo igual que el juego: Fralex
  rescinde el contrato de Stones en el juego y se compara qué borra el juego.

## 12. Lo que borra el PROPIO juego al despedir a un jugador (Stones, ranura 10 → ranura 11 `ML0000000A`) — **[VERDAD DEL JUEGO]**
- Las ranuras pasan a **hexadecimal**: la ranura 11 es `ML0000000A` (no `ML00000010`).
- Fralex usó **«Despedir»** (Stones quedó libre, en ningún club). Stones tenía una **oferta abierta** de otro club: su segundo
  registro de contrato era esa oferta.
- El juego: compacta la plantilla, dorsales y contador del City; mete a **Rúben Dias (banca)** en el puesto de Stones del XI y sube a
  los demás; quita a Stones de **las dos** entradas de la tabla I (contratos: 27 → 25) y de la J; **borra la negociación abierta** de una
  lista de ofertas tras el blob (registros con el club, el jugador y el club que ofertaba) y la compacta; añade a Stones a dos listas
  de «salidas» (0xbe34b4 y 0xc8018c); vacía **en su sitio** (sin correr a los demás) su registro en A, B, C, D, E, L y M, y **compacta**
  F, G y H. **No** toca la lista K ni la tabla A2 (las actualiza más tarde). También cambian unos bytes del bloque del equipo (+0x5da,
  parecen medias del equipo) y del dinero.
- **Consecuencias para el C++:** (1) al vender hay que borrar **todos** los contratos del jugador en la tabla I, no solo el primero;
  (2) hay que borrar sus **negociaciones abiertas**; (3) compactar o dejar hueco funciona en ambos casos (el juego aceptó nuestra
  compactación en las ranuras 6 y 9), así que se mantiene la compactación. ⏳ (1) y (2) pendientes de programar y probar.

## 13. Dinero, contratos y competiciones (escaneo del 8 oct, tarde) — **[OBSERVADO]**
**Dinero del usuario** (bloque en 0xc7dd00, valores en u32 ×100 €; comprobado con las pantallas y con el despido de Stones):
| Qué | Dónde | Ejemplo |
|---|---|---|
| Presupuesto de fichajes ACTUAL | 0xc7dd00 | 1.632.406 → 163.240.600 € (bajó 666.800 € al despedir a Stones) |
| Presupuesto de fichajes inicial de la temporada | 0xc7dd10 | 1.632.406 |
| **Tope salarial** | 0xc7dd14 | 1.475.206 → 147.520.600 € |
| Otros (sin interpretar) | 0xc7dd1c, 0xc7dd2c, 0xc7dd30, 0xc7dd34 | 3,6 M · 171,2 M · 122,1 M · 18,6 M |
- **Presupuesto salarial en pantalla = tope salarial − suma de sueldos** de la tabla I (comprobado en dos guardados:
  147.520.600 − 117.238.000 = 30.282.600 ✓; tras despedir a Stones 147.520.600 − 113.904.000 = 33.616.600 ✓).
- **Tabla I (contratos, paso 48)**, por registro tras `[x][reg][pid]`: +0 sueldo anual /100 · +4 cláusula de rescisión /100 ·
  +8 fecha de fin (año u16, mes, día) · +0x14 número único · +0x18 tipo (18/22/154 = contrato vigente; 125/255 = oferta
  pendiente) · +0x1c fecha de inicio. Stones: sueldo 3.334.000 €, cláusula 12.000.000 € ✓ (pantalla).
- **Clubes de la IA: NO tienen presupuesto en el guardado.** Solo hay dos arreglos por equipo (bloques de 1680 B y de 600 B)
  y ninguno guarda dinero; la IA ficha por reglas, no por caja. Las fichas de jugador de 596 B tampoco guardan el valor de mercado
  (lo calcula el juego).
**Competiciones dentro de la Liga Máster:** bloques de **3000 B por competición** (desde ~0xb08000), cada uno con la **lista de
equipos** (IDs u32, relleno `0x0003ffff`): liga argentina 30, Premier 20, Brasileirão 20, Liga 1 Perú 18, segundas divisiones
hasta 66… **Capacidad holgada** (los bloques de 3000 B admiten muchos más IDs). El tamaño de una liga en la LM se podría cambiar
en esa lista, pero el **calendario** (zona tras el blob) se genera con ese tamaño: habría que regenerarlo o cambiarlo antes de empezar
la temporada. También hay listas de selecciones y de grupos de copas (0x1f468c…).

## 14. El «blob» NO es un misterio: es zlib — **[OBSERVADO, 8 oct tarde]**
- Desde 0x11403d4: **18 tramos zlib** (firma `78 9c`), cada uno de 256 KB descomprimidos, separados por 12 B
  (`00 04 00 00 | tamaño comprimido | desplazamiento acumulado`). Descomprimido mide **4.678.743 B, siempre igual** (estructura
  fija). Herramienta: `prototipos/blob.py`. Para escribir habrá que volver a comprimir por tramos y actualizar la palabra de tamaño
  (0x11403a8) y las cabeceras de tramo; por eso el tamaño del archivo cambia entre guardados.
- Contiene **calendario y resultados de todas las competiciones**: entre el guardado del 21/8 y el del 31/8 (una fecha jugada + 10 días)
  cambiaron 11.144 zonas; al despedir a Stones sin jugar, solo 96 B (0xa588c). Hay zonas densas de fechas (0x1a330–0x83b24 con 2.646,
  0xeda68–0x197e50 con 4.477) y miles de pares de IDs de equipo. ⏳ Falta decodificar el formato de partido (local, visitante, fecha, goles).
- Fuera del blob también hay: **calendario del usuario mes a mes** (0xa8d6d8…: 1 registro de 708 B por día, con fecha, ID de
  competición y rival; comp 9 = Premier, 63 = ventana de fichajes…), **listas de equipos por competición** (§13) y un **historial de
  traspasos** (0x12b430f, registros de 36 B con jugador, monto y fecha).
- **Tamaños de liga en la LM:** el guardado ya tiene una liga de **30 equipos** (Argentina) y segundas de hasta 66: el motor acepta
  ligas grandes si el parche las define (CPK de competiciones). Lo que fija el tamaño al empezar la carrera es el CPK; después, el
  calendario del blob queda armado con ese tamaño.

## 15. Dentro del blob: la FICHA DE LIGA MÁSTER de cada jugador (156 B) — **[OBSERVADO, 8 oct tarde]**
- Desde el byte 0x1e del blob descomprimido: **16.422 registros de 156 B, uno por `reg`** (todos los jugadores del universo de la
  carrera, incluidos libres y juveniles). Registro: `[u16][reg u32][pid u32]` + lista de **competiciones en las que está inscrito**
  (pares u16 id · u16 banderas, relleno 0xffff) + campos de temporada. Campos ya identificados:
  | Desplazamiento | Qué | Comprobado con |
  |---|---|---|
  | +0x56 (u32) | **sueldo anual /100** | igual al de la tabla I (24/24 jugadores del City) |
  | +0x5a (u32) | **valor de mercado /100** | Rúben Dias 550.000 → 55.000.000 € = pantalla ✓ |
  | +0x62 (u32) | número único del contrato | el mismo de la tabla I |
  | +0x66, +0x6a | fin e inicio del contrato (año u16, mes, día) | 31/8/2027 · 1/7/2020 ✓ |
  | +0x4b (u8) | ¿valoración media de la temporada ×10? | 0 al inicio; 64–91 tras 2 partidos (Marmoush 91) |
  | +0x74 (u16) | ¿experiencia / crecimiento? | sube ~+11/+12 por partido en todos los que juegan |
  | +0x7a (u8) | ¿condición física? | 100 en todos al inicio de temporada; 85–100 y otros valores después |
  | +0x16…+0x29 | estadísticas de temporada (cambian en 1.000–1.400 jugadores por fecha) | por decodificar |
- **Potencial:** leer de aquí goles, valoraciones y minutos para mandarlos solos a la web; fijar valor de mercado, sueldo y
  condición desde la web; ver en qué competiciones está inscrito cada jugador.
- Después del arreglo (desde 0x271746) hay más registros con **puntos de liga** (+3 al ganar, se ve en decenas de equipos) y
  otros datos de temporada por equipo, y listas de jugadores inscritos por competición. ⏳ **El calendario de partidos** (local,
  visitante, fecha, goles) aún no se localizó: no aparece como pares de IDs ni de índices; probablemente use el número de plaza
  dentro de cada competición. Siguiente pista: comparar dos guardados consecutivos de la MISMA fecha jugada (sin avanzar días).

## 16. Búsqueda del calendario de partidos (8 oct, tarde) — **[NEGATIVO por ahora, con pistas]**
Se buscó en el blob y fuera de él, con escaneos sistemáticos, cualquiera de estas formas: (a) rondas de 10 partidos con índices de
plaza 0–19, registros de 2–200 B y campos en cualquier posición; (b) rondas con índices de bloque (140–159) o IDs option; (c) rejillas
20×20 de jornadas. **Ninguna aparece.** Lo que sí se encontró:
- **IDs de partido:** el calendario del usuario de enero (708 B por día) trae en los días de liga **10 números u16 seguidos** (2 ene:
  285–294; 31 ene: 360–369): son los partidos de esa jornada numerados globalmente. El registro de partido debe estar indexado por
  ese número, en algún arreglo aún no identificado (quizá con los equipos como «plaza dentro de la competición» y no como ID).
- **Tabla de equipos de 16 B** en 0xc016c0 (263 registros, orden alfabético por liga): `[00 00][05 00][4 B][equipo u16][00 00][v u16][v u16]`
  con `v` decreciente a lo largo de la lista (1009 → 857 City → …): parece un orden/prestigio de clubes. Sin uso todavía.
- **Lo que no está en el blob:** las habilidades de los jugadores (se probó el mapa de bits de Player.bin y búsqueda por fuerza bruta:
  no están). El crecimiento de la LM se guarda aparte (probablemente como puntos de experiencia, +0x74 de la ficha de 156 B).
**Experimento que falta para cerrar el calendario:** dos guardados de la MISMA fecha, uno justo ANTES de jugar un partido del
usuario y otro justo DESPUÉS (sin avanzar el día). La diferencia aislará el registro del partido jugado (goles, estado) y con él
el formato de todo el calendario.

## 17. Contratos, negociaciones y huecos: formato exacto y réplica byte a byte (8 oct, 15:30) — **[VERDAD DEL JUEGO]**
Comparando la ranura 10 (`ML00000009`) con la 11 (`ML0000000A`, el juego despide a Stones) se cerró lo que faltaba del §12.
- **ID interno de club** = `(ID option << 14) | índice del bloque de equipo`. City = 0x2b409a → ID 173 (0xad), índice 154 (0x9a).
  Deportivo A Coruña 0x1bc07d (111 | 125), River 0x228016 (138 | 22), Lanús 0x1e24012 (1929 | 18). Es el ID que usan los
  contratos, las negociaciones y la tabla de 16 B de 0xc016b0 (que resulta ser un **ranking de clubes**: `[?][ID club][puesto][v u16 ×2]`,
  Real Madrid 1.º 1015, PSG 2.º 1009, City 3.º 1002…). El bloque de equipo del usuario lleva **-11** (0xfffffff5) en +656 en vez
  de su ID option (marca de «lo lleva el usuario»); su ID real se recupera de los contratos (`club >> 14`).
- **Tabla I = contratos, registro de 48 B que empieza 28 B ANTES de `reg`:**
  `[índice u8+relleno][ID club u32][tipo u8+relleno][inicio (año u16, mes, día)][-1][-1] | [x u16][reg][pid][sueldo/100][cláusula/100][fin]`.
  Tipo 5 = contrato vigente con el club del registro; **tipo 3 = oferta** de otro club por ese jugador (sin fecha de inicio). Un
  jugador puede tener su contrato (tipo 5, club del usuario) y una o más ofertas (tipo 3, club que oferta). City tenía 24 contratos
  + Bettinelli←Lanús, Stones←River, Rulli←Deportivo. Los bytes de relleno llevan basura de memoria; el juego los limpia a 0 cuando
  reescribe la tabla. El «número único» del §13 era en realidad el ID del club.
- **Lista de negociaciones abiertas** (tras el blob; 0x1279f18 en la ranura 10), registro de **60 B**, `reg` en +32:
  `[estado u16][ID club que oferta][banderas u8…][0xffff][-1][-1][0xffff u16] | [ID club del jugador][reg][pid][monto/100][monto/100][0xffff][0xffff][0]`.
  Stones: River 6.240.000 €, Bettinelli: Lanús 260.000 €, Rulli: Deportivo 3.120.000 €. Registro vacío: estado 0xffff, clubes -1,
  reg 0xffff. El juego **borra la del que se va y compacta**.
- **Qué hace el juego con cada tabla del usuario cuando un jugador se va** (y ahora el C++ hace lo mismo, comprobado byte a byte
  sobre la ranura 10 → resultado idéntico a la ranura 11 salvo relleno):
  | Tabla | Qué hace el juego | Detalle del hueco |
  |---|---|---|
  | A (24) | hueco en su sitio | todo a 0, reg 0xffff, **fecha vacía 0xffff en +16** |
  | A2 (24, a 976 B de A) | **no la toca** (la rehace después; su campo anterior a `reg` es la valoración del último partido en float). Está **vacía en el respaldo r0** (carrera recién empezada): el C++ no la exige (firma 24↑ ×1) | — |
  | B (24↓) | hueco | reg 0xffff, pid 0, +8 a 0, **conserva la constante de +10** (0x324f) |
  | C (44) | hueco | registro = [reg…reg+44): todo a 0 y reg 0xffff; el campo anterior a `reg` es el final del registro anterior |
  | D (368), E (192), J (16) | hueco | igual que un registro nunca usado (reg 0xffff, resto 0) |
  | F (52) | hueco | como nunca usado pero **+40 a 0** (el nunca usado lleva 1) |
  | M (5628) | hueco | como nunca usado; **y 0 en la palabra anterior al siguiente `reg`** (es el final de su propio registro) |
  | G, H ×3 (108) | **compacta** | — |
  | I (48, contratos) | **compacta** quitando **todos** los registros del jugador (contrato + ofertas) con la frontera de 28 B | — |
  | K | no la toca al momento (queda atrasada; el C++ la reescribe porque Estrategia la lee) | — |
  Además el juego: anota al jugador en una lista de noticias (0xbe34b4: `[reg][pid]…[6][fecha]`) y en la **caja** (0xc8018c, registros
  de 36 B `[reg][pid]…[monto i32 ×100]`, −666.800 € de indemnización), baja el presupuesto (0xc7dd00), recalcula las medias del
  equipo (bloque del City +0x5da…+0x608) y añade un par (reg 3807, pid 61147) a una lista de pares en 0xc07160 (¿objetivos?). ⏳ Lo
  de noticias/caja/medias no se replica todavía (el juego lo recalcula o es informativo).
- **El orden de formación del usuario es el bloque 627 del arreglo de alineaciones** (0x18f9d8 = bloque 627 + 0x220): es el primer
  bloque con el ID del usuario (-11) y «técnico» = nombre del mánager (Lionel Messi); el 628 es su reserva (orden identidad). El
  bloque 154 es la copia del City «como equipo de la IA» (ID 0xad, orden identidad). Ya no hace falta buscar el orden por fuerza bruta.
- **Lista K atrasada:** tras despedir a Stones el juego dejó K con 24 entradas (Stones incluido) y cambió el flag de Gvardiol de
  0xc0 a **0xc4** (bit desconocido). El C++ ahora localiza K por su estructura (flag 0 + registros 0xc0–0xc6 + libre + 0xc7),
  conserva el flag de cada jugador y la reconstruye desde el orden nuevo.
- **Jugadores creados por el juego (regens/canteranos de la IA):** `reg = 0xdb65xxxx`, pid > 126.000, no están en el catálogo
  del parche; 771 al empezar la carrera y 1.127 un mes después. Están en las plantillas de la IA (hasta 3–4 por club). El C++ los
  admite como fichas válidas. ⏳ Falta ver dónde viven sus fichas de 156 B (no caben en el arreglo de 16.422).
- **Las tablas del usuario NO van por índice de plantilla**: el juego las busca por (reg, pid). Por eso tolera huecos y por eso los
  fichajes nuevos van al final (Neymar está después de los 32 juveniles en D). El ancla del C++ (`tablasDe`) ya salta huecos y
  fichas de jugadores que ya no están (hasta 8).

## 18. Dinero por ancla (8 oct, 16:10) — **[PROBADO en el juego con direcciones fijas; ancla verificada en 6 guardados]**
- El bloque de dinero (§13: 0xc7dd00 actual, +0x10 inicial, +0x14 tope) está a **0x97d38 de la tabla A** del usuario en todos los
  guardados vistos (r0 respaldo, v6…v9, ranuras 9/10/11/12). Es parte de la estructura fija «club del usuario» del motor, que no se
  desplaza con el blob (el blob está después). El C++ (`GuardadoLM::finanzas` / `fijarFinanzas`) lo localiza así y **valida** que el
  tope cubra la suma de sueldos de los contratos vigentes (tipo 5 del club del usuario) y que las cifras no pasen de 40.000 M.
- Lecturas reales: r0 (respaldo) fichajes 163.240.600 / tope 147.060.000 / sueldos 143.426.600 → ppto salarial 3.633.400;
  ranura 11 fichajes 162.573.800 / tope 147.520.600 / sueldos 113.904.000 → 33.616.600 (= lo que muestra la pantalla).
- `fijarFinanzas` solo escribe los campos pedidos (0 = no tocar), en múltiplos de 100 €, y rechaza un tope menor que los sueldos.


## 19. Lo que escribe el juego al FICHAR para el usuario (Sommer, Inter → City, 8 oct 17:20) — **[VERDAD DEL JUEGO]**
Base: ranura 2 (carrera recién empezada, igual a r0 salvo 90 B); resultado: ranura 15 (`ML0000000E`, md5 b1db56aa…). El juego
también guardó solo encima de la ranura 2 (mismo contenido que la 15 salvo 68 KB de estado). Sommer: reg 5895, pid 36627, dorsal 12.
- **Plantillas:** sale del bloque del Inter (k 213, índice 0; la lista se compacta) y entra **al final** del City (índice 26). En Suiza
  (selección, k 563) sigue igual.
- **Tablas del usuario:** en TODAS aparece un registro nuevo en el **primer hueco libre** (A, B, C, F, G, H×3, M, J: registro 26; C y D:
  registro 58 = 26 + 32 juveniles). Nada se desplaza. Pero el registro **no es un molde vacío**: lleva datos del jugador:
  A `[0][reg][pid][6][?][?][fecha de llegada 2026-08-17]`; B `[reg][pid][0x1e][?]`; C `[reg][pid][2]…[2]`; D estadísticas iniciales
  (+12: 0x451d repetido, luego una tabla de u16); **E = curva de 0x3c→0x57→0x51 (≈ progresión por edad/mes, 50+ bytes)**; F `[ffff][reg][pid][0x3c0]`;
  G/H vacíos salvo reg/pid; M (5.628 B) `[1][reg][pid][club City 0x2b409a][0][0x00080314][ffff][fecha][…][0x1dc0d5 = Inter (k 213)][5]…`.
- **Contrato (tabla I):** registro nuevo índice 30, formato §17: club City, tipo 5, inicio 17/8/2026, sueldo 3.561.100 €/año,
  cláusula 2.200.000 €, fin 31/8/2027.
- **Historial de traspasos:** registro con fecha 17/8/2026, reg/pid, club destino City, club origen Inter (0x1dc0d5), **monto 2.575.000 €**.
- **Lista K:** se añade K[26] con flag **0xc1** (no 0xc0) y el libre sigue con 0xc1. **Orden de formación:** el 26 al final (última reserva).
- **Dinero:** fichajes 163.240.600 → 160.665.600 (−2.575.000 = el monto); tope salarial 147.060.000 → 147.333.100; sueldos +3.561.100.
- Hay más listas tocadas (0xc00de0, 0xc01564, 0xc07170, 0x10bab98, 0x12dddad) por identificar.
**Conclusión:** fichar PARA el usuario se puede replicar, pero hay que **rellenar** varios registros con datos del jugador (curva de
E, estadísticas de D, bloque M). Siguiente paso: averiguar de dónde salen (la ficha de 156 B del blob, el Player.bin) para generarlos.
- **(17:50) Tabla E = historial de medias:** `[edad u8][media cada medio año…]`, longitud = 2 × (edad − 9) valores, el último = media
  actual (comprobado en los 27 del City: Donnarumma 27 → 36 valores, Sommer 37 → 56). Antes del fichaje esta curva **no existía en el
  archivo**: el juego la **genera** al fichar (sube desde ~55–60; en veteranos llega a un pico y baja hasta la media actual).
- **Tabla D (368 B):** `+8` 14 B con un patrón `xx 45` (0x14 para la plantilla de inicio, 0x1d para Sommer: ¿semana de llegada?);
  `+0x2c` 3 × u16 con un 10000 según el tipo de puesto (portero en el 3.º, central en el 1.º); después ~17 u16 entre 0 y 9.999 de
  aspecto **aleatorio** (semillas o pesos de desarrollo por puesto). También los genera el juego.
- **Listas laterales que toca el fichaje** (parecen noticias/historial; ❓ si son obligatorias): 0xc00de0 `[0x15ff][reg][pid]` (12 B,
  «llegados»), 0xc01564 `[reg][pid][fecha][monto/100 u16][?][1]` (historial con fecha, se inserta arriba), 0xc07170 `[reg][pid]` (8 B),
  0x10bab98 (noticia con fecha 29/8 y los dos clubes), lista de 24 B tras el blob (0x12dddad) `[-1][0][reg][pid][0x0d][0][ffff][-1]`.
- **Plan para programarlo:** registro nuevo en el primer hueco de cada tabla, **clonando** el de un compañero del mismo puesto y
  cambiando reg/pid/edad/fechas; curva E generada (edad y media del catálogo); contrato y historial con el formato §17; K con flag
  0xc1; dinero. Probar en el juego primero SIN las listas laterales; si el juego se queja, añadirlas.

## 20. Fichar PARA el usuario programado y comparado con el juego (8 oct, 19:40) — **[OBSERVADO; prueba en juego: ranura 2]**
- **Blob:** la zona real mide `tamaño + 12` (la palabra de 0x11403a8 no cuenta los 12 B finales) y descomprime **4.680.156 B**;
  separadores `[desc BE][comp BE][fin BE]` con `fin = rel + comp` (0 en el último); cabecera rel +0x14/+0x18 = sumas (BE). La ficha
  de 156 B del jugador está en `0x1e + 156·reg`. `core/BlobLM` lee y reescribe (ida y vuelta idéntica en 4 guardados).
- **Fecha actual de la partida:** 0xacc61c (u32 fecha) con `[día del año u16][año u16][365]` delante (0xacc614); luego el club del usuario.
- **Ficha del blob al fichar:** se añaden las competiciones del club que le faltan (las mismas de sus compañeros), sueldo (+0x56),
  club anterior (+0x5e), fin (+0x66) e inicio (+0x6a). Los 6 B de +0x4a se copian a la tabla A (+8) junto con la fecha (+16).
- **`GuardadoLM::ficharParaUsuario`**: molde = compañero del mismo puesto (el más reciente), registro nuevo en el primer hueco de cada
  tabla (lo de delante de `reg` se toma del registro 1), E con historial de medias generado, F a 0 (contador y marca), M con club,
  fechas, club de origen y bit 0x80000 en +0x14; contrato al final de la tabla I (nº = mayor + 1); K con el flag del libre; orden +n;
  dinero; IA de origen con sustituto. Comparado con el fichaje de Sommer hecho por el juego: **iguales** plantillas, orden, K, dinero,
  A, contrato y ficha del blob; difieren solo valores que el juego genera/actualiza con el tiempo (B +10, D +8, F contador, M +0x14).
- No se escriben (¿decoración?): historial de negociación (3 × 80 B en 0xbe90a8…), lista de 60 B en 0xbe8a54, llegados (0xc00de0),
  eventos con fecha (0xc01564), 0xc07170, 0xc80414, noticia 0x10bab98, lista tras el blob. Si el juego los necesitara, la prueba lo dirá.

## 21. Agentes libres, fichas `0xdb65` y qué es de verdad el blob (9 oct, 21:00) — **[OBSERVADO en 6 guardados; corrección en C++ con pruebas]**
Guardados usados: r0 (4/8), g18 (19/8), g19 y juego (21/8), j9 y jA (31/8, antes y después de que el juego despidiera a Stones), fD (22/9).
- **El blob es SOLO el arreglo de fichas de jugador.** Cabecera de 0x1e B + **30.000 plazas de 156 B** (= el límite de jugadores del
  juego) y una cola corta; 26.255 ocupadas en estos guardados. Después de las fichas solo hay plazas vacías, y esa zona **no cambia**
  en un mes de temporada. **CORRECCIÓN de §14:** el blob **no** guarda el calendario ni los resultados; lo que cambiaba con cada
  jornada eran las estadísticas de temporada dentro de las fichas. El calendario y los resultados están **fuera del blob**.
- **Fichas `0xdb65xxxx` (resuelve el pendiente de §17):** siguen en el mismo arreglo, justo después de las del parche. La posición usa
  los 16 bits bajos: ficha en `0x1e + 156·(reg & 0xffff)`. El primero es `0xdb654026` (= índice 16.422, justo donde acaban las del
  parche) y en estos guardados llegan a `0xdb65668f` (9.834 consecutivas). Las 788 (g19) a 1.174 (fD) fichas `0xdb65` que están en
  plantillas tienen su ficha ahí con el mismo pid. No son solo regens: muchas están en selecciones (bloques ≥ 528) con pid bajos.
  `BlobLM::fichaDe` ahora usa esa regla (antes devolvía −1 para ellas: no se podía fichar a esos jugadores para el usuario).
  Prueba nueva con datos reales: «toda plaza de plantilla tiene ficha en el blob» → r0 17.121 / g19 17.551 / jA 17.900 / fD 17.947 con
  ficha, **0 sin ficha**.
- **Agente libre = ficha con el bit 0x20 de +0x75 Y fin de contrato vacío (+0x66 = `ffff 0000`) Y en ningún bloque de equipo.**
  j9: 94; jA: 95 (+ Stones). Los 94 son los libres de Konami (pid 55272…, L. Manas, J. Rekarte…; club anterior −1).
  Al despedir a Stones el juego cambió en su ficha: quitó las competiciones del club (0x1e, 0x3b, 0x5c; dejó 0x22, 0x29, 0x24),
  puso el **club anterior** (+0x5e) = City (`0x2b409a`), vació el fin de contrato (+0x66) y pasó +0x75 de 0x05 a **0x25**. +0x62 sigue
  con el club (City). Fuera del blob quedó en: K (atrasada), noticias (0xbe34b4), A2, historial de negociación (0xbe9058), la lista de
  12 B por reg de 0xc00d44 (`[reg][pid][0x15ff]`, sin cambios), la caja (0xc8018c) y la lista de 24 B tras el blob (con River, que ofertaba).
- Dos casos raros con el bit 0x20 **dentro** de un club (bloques 617 y 618, Bin Changhun y A. Jabir, +0x75 = 0x21): sin interpretar.
- **649 fichas sin club y sin el bit** (regs 14.872–16.325, contrato hasta 31/1/2028, +0x75 = 0x1x o 0x4x; ej. Naser Aliji, Jasir Asani):
  están en el catálogo pero en ningún bloque. [HIPÓTESIS] jugadores de clubes que no entran en la Liga Máster.
- **Fuera del blob, entre el 19/8 y el 21/8** (un partido del usuario) cambiaron, además de lo conocido, registros de 20 B en
  0xccdbf0–0xcce9fc con `[reg][pid][u32 empaquetado]…` (¿jugadores destacados o valoraciones de partidos?) y la zona 0xcd0ce8. ⏳
- ⚠️ **Multiparche:** la frontera 16.422 depende del parche (cuántos jugadores trae); la regla `reg & 0xffff` no. `BlobLM::kOfsTam`
  (0x11403a8) es una **dirección fija** del ConmeGOL 26: en un parche con otro número de equipos o jugadores podría moverse. Pendiente:
  localizarla por ancla.
- **Experimentos que faltan (los hace Fralex):** (1) fichar a un agente libre en el juego y guardar en otra ranura (verdad del juego
  para programar el fichaje de libres); (2) guardar justo antes y justo después de un partido, el mismo día (calendario y resultados);
  (3) reiniciar el juego con un `Player.bin` cambiado y cargar una carrera (stats en la Liga Máster).
- **(9 oct, 21:25) Actualización:** `BlobLM::leer` ya no depende solo de 0x11403a8: si ahí no está, busca la cabecera
  (`8c 45 07 00`, única en el archivo) y valida la zona entera; `aplicar` reescribe en la posición encontrada (`posicionTam()`).

## 22. 🏆 El calendario de partidos (9 oct, 21:45) — **[OBSERVADO en 2 carreras: City/ConmeGOL y Barça; falta verlo en pantalla]**
- **Número global de partido:** cada partido de la carrera tiene un número (0…5.758 en la carrera del City). Cada competición
  ocupa un **rango** seguido: ej. comp 17 = 0–434 (435 = liga de 30 a una vuelta), comp 18 = 435–814 (380 = liga de 20),
  comp 30 (Premier) = 5.531–5.720. Son los mismos números que aparecen (de 10 en 10) en el calendario del usuario por días.
- **Registro de partido de liga (paso 32 B, en ~0x1f0000–0x28c000):**
  `[+0 u32][+4 8 B: datos del partido del USUARIO (ffff f707 si no es suyo)][+12 local: ID interno de club u32]`
  `[+16 visitante u32][+20 nº de partido u16][+22 ffff][+24 nº de competición u16][+26 código u16: jornada = bits 0–5, orden dentro de la jornada = bits 6+]`.
  ID interno de club = `(ID option << 14) | índice de bloque` (§17); el club del usuario se escribe con su ID real (City 0x2b409a, Barça 0x1b007a).
- **Carrera del City (r0 y fD):** Premier (comp 30) con **190** registros = 19 jornadas × 10 → solo la primera vuelta guardada.
  Jornada 1: City – Crystal Palace; 2: Everton – City; 3: City – Brentford; 4: Brighton – City; 5: City – Newcastle.
- **Carrera del Barça (ranura 1 del PC, 4/3/2026, md5 7b800612…):** LaLiga (comp 29) con **380** registros (38 jornadas, ids 0–379).
  Jornada 1: Barça – Real Sociedad; 2: Deportivo – Barça; 3: Barça – Celta; 10: Real Madrid – Barça.
- **Los cruces no cambian al jugarse** (r0 → g19 → fD iguales): el **marcador no está** en este registro. Solo los partidos del
  usuario llevan 8 B extra (ej. `0a 00 00 60 1d 00 00 00`): parecen enlazar con su agenda. ⏳ **Resultados: sin localizar.**
- **Copas:** en la misma zona hay bloques de fase con los números de partido de cada grupo (6 por grupo de 4) y sus 4 clubes; entre
  el 4/8 y el 22/9 se rellenaron (sorteo de grupos de la Libertadores/Sudamericana).
- **Descartado como resultados:** 0x66fa2c… (registros de 16 B `[reg][pid][x][fecha?]`: ¿lesiones o sanciones?) y 0x10c0000…
  (una **imagen** en bloques de textura, se vacía con el tiempo).
- ⚠️ 🧩 **Multiparche:** la forma del registro es del juego. Los números de competición y los rangos de partido dependen del parche
  y de la carrera (en el Barça LaLiga es la 29; en el City la Premier es la 30). Siempre se buscan por la forma, nunca fijos.
- Aclaración: que los números de 10 en 10 del calendario del usuario por días sean estos mismos números globales es 🔎 por confirmar (encajan en los rangos, p. ej. 285–294 cae en la comp 17).

## 23. 🏆 Tablas de posiciones (9 oct, 22:10) — **[OBSERVADO y cruzado con el calendario; falta verlo en pantalla]**
- Zona ~0xcb0000–0xcf2000 (ConmeGOL). **Fila de 20 B por club:** `[club: ID interno u32][puesto u32 (empates comparten; ffff ffff = liga sin empezar)][C u32][A u32][B u32]`.
  - **C:** puntos = bits 0–7 · ganados = bits 8–13 · perdidos = bits 14–19 · empatados = bits 20–25 · ganados de visitante = bits 26+.
  - **A:** goles a favor = bits 0–11 · goles en contra = bits 12–23 · partidos jugados = bits 24–31.
  - **B:** goles de visitante.
- **Dos tablas por liga, seguidas:** la actual y la de la jornada anterior (ej. Ligue 1 en la carrera del Barça: PJ 8 y PJ 7).
- **Comprobaciones:** (1) en cada tabla, suma de goles a favor = suma de goles en contra (Ligue 1 195/195 y 171/171; otras
  212/212, 220/220, 64/64…); (2) **cruce con el calendario (§22):** los 10 partidos de la jornada 1 de la Premier (carrera del City,
  fD 22/9) cuadran 10/10 con la tabla: City 3–0 Crystal Palace, Ipswich 2–2 Everton, Bournemouth 0–3 Brentford, Liverpool 0–0
  Brighton, Forest 0–1 Newcastle, Sunderland 0–2 Leeds, Arsenal 0–1 Chelsea, Villa 3–1 Coventry, Fulham 2–2 Tottenham,
  Hull 4–1 Man United. Los ganados de visitante (Brentford, Leeds, Chelsea, Newcastle) y los goles de visitante también cuadran.
- **Carrera del Barça (ranura 1, 4/3/2026):** LaLiga todavía vacía (puesto ffff ffff); Ligue 1 con 8 jornadas: Lille 22 pts
  (7-1-0, 19–4), PSG 20, Lens 19, Lorient 16, Marsella 14, Mónaco 14.
- Las tablas de 40 filas de la misma zona tienen otro formato (no son tablas de liga): sin interpretar.
- **Resultados partido a partido:** siguen sin aparecer como tabla propia; con el calendario + dos tablas seguidas se puede deducir
  el resultado de la última jornada cuando cada club jugó un partido (como se hizo arriba). ⏳ Buscar si existen guardados.
- 🧩 Multiparche: el formato es del juego; la posición de la zona y el orden de las ligas dependen del parche → buscar por la forma.

## 24. Goleadores y asistencias, prefijo de jugador por carrera y lector `TemporadaLM` (9 oct, 23:20) — **[OBSERVADO + pruebas automáticas]**
- **Rankings (fila de 20 B):** `[reg][pid][club: ID interno][puesto u32][valor u32]`, ordenados por puesto (empates comparten).
  Varias listas seguidas por competición. **Premier, carrera del City (fD 22/9, 1 jornada):** la 1.ª lista suma **27** = los 27 goles
  de la jornada, y club por club coincide con los goles a favor de la tabla (City 3: Foden 2 + Semenyo 1; Hull 4; Villa 3; Brentford 3…)
  → **goleadores** ✅ (en archivos). La 2.ª (15) parece **asistencias**. La 3.ª (41) y la 4.ª (21): sin identificar (¿tarjetas?).
  Community Shield: Marmoush 3, Guéhi, Semenyo, Gyökeres (City 5 – Arsenal 1). En la Liga de Brasil: Haaland en el Santos con 3
  (el traspaso que hicimos en la ranura 9).
- **Corrección de §21:** el prefijo de los jugadores añadidos **cambia en cada carrera** (0xdb65 en la del City, **0xdbdf** en la del
  Barça). La regla es la misma: ficha en `0x1e + 156·(reg & 0xffff)`. `BlobLM::fichaDe` y `regPlausible` ya aceptan cualquier prefijo
  (16 bits altos ≥ 0x8000, ≠ 0xffff, índice < 30.000). Antes, en la carrera del Barça, 1.156 jugadores no tenían ficha para el programa.
- **`core/TemporadaLM` (solo lectura):** lee partidos, tablas (filas coherentes: pts = 3G + E, PJ = G + E + P, ΣGF = ΣGC) y rankings
  buscando por la forma. Pruebas con 6 guardados reales: jornada 1 de cada liga con 1 sola jornada jugada cruzada con su tabla →
  35/35 (g19), 152/152 (jA), 38/38 (fD); hay siempre una lista de goleadores que suma exactamente los goles de esas tablas.
  Partidos leídos: 5.258–5.294 (City), 2.440 (Barça: otro reparto de competiciones).
- Aclaración de §23 (9 oct, 23:40): varias de las «tablas de 40 filas» de esa zona son en realidad **listas de goleadores/asistencias** (§24): en la carrera del Barça, 0xcb5bf0 = goleadores de la Ligue 1 (Igamane, Édouard, Avom 8) y 0xcb5f1c = asistencias (João Neves 6).

## 25. Habilidades dentro de la Liga Máster (9 oct, 23:50) — **[PROBADO: no llegan; ubicación SIN ENCONTRAR]**
- **Prueba en el juego:** Phoenix-DB con Player.bin v99 (Lamine Vel. 99) y el juego reiniciado: amistoso 99; carrera (Barça) **90**,
  media 88, Pase raso 83 y Contacto 77 (la base tiene 82 y 76). La carrera tiene su copia propia y la hace evolucionar.
- **Buscado y descartado** (carreras Barça y City): copia literal del registro de 312 B de Player.bin (ventanas de 16 B: 0); las 12
  tablas del usuario y su ficha M (5.628 B) con búsqueda empaquetada 5–8 bits y desplazamientos 0/20/25/30/40 (±1, orden de pantalla);
  su ficha de 596 B; la ficha de 156 B del blob (§16). La tabla E es la curva de medias (81…84), no las habilidades.
- **InstallVersionPlayer.bin** (lo lee el juego al entrar a la LM; Sider lo sirve desde `download\dt80_700E_x64.cpk`, también hay
  versiones en dt80_100E…600E y en `Data\dt10_x64.cpk`): WESYS + zlib → 59.408 B = 7.426 × `[id jugador u32][versión u32]` (0x64 = 100…).
  Sin habilidades; Lamine (162114) no figura. Parece la lista de qué versión de datos «instalada» usa cada jugador.
- **Herramienta nueva en el PC (VM):** `~/phx/cpkls.py` lista el índice de un CPK sin cargarlo (portado de `BaseDatosParche.cpp`).
- **Experimento propuesto:** dos carreras NUEVAS idénticas (mismo club y ajustes), una con Phoenix-DB v99 y otra con v90 (original),
  guardadas en ranuras libres. La diferencia entre las dos señala dónde guarda la carrera la Velocidad.
