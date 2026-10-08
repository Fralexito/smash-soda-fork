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
| **K = alineación** | 0xb7f960 (K10) | 16 | `[flag, reg, pid, 0]`. K0–K10 = XI en orden de formación, K11–K17 banca, luego reservas. Flags: K0 = 0, usados 0xc0; primer libre c0 (25 jugadores) → c1 (26); más allá c7 |

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
