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
