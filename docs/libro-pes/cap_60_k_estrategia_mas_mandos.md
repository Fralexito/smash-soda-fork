# Capítulo 60-k — Estrategia para jugar con más de 8 mandos (8 vs 8, y hasta 11 vs 11) (19-oct-2026)

> Pregunta de Fralex: «¿sería posible con los 11 en cada equipo? Con 8 por equipo me basta. ¿Qué estrategia usaríamos: un software aparte, solo inyección por Sider, o cómo?». Base técnica: capítulos 60-j (dónde está el 8) y 62 (catálogo). Todo lo de aquí sale de leer el exe; **nada probado en vivo**.

## 0. Respuesta corta
- **¿11 por equipo (22 humanos)?** **SÍ, el motor del partido está hecho para eso**: 24 cursores humanos (uno por futbolista + 2) y 24 bloques de entrada. Konami lo usó en el 11 vs 11 online. Lo que no existe es el camino «local»: hoy solo se alimentan los 8 primeros. Posibilidad: **real pero difícil**.
- **¿8 por equipo (16 humanos)?** Mismo trabajo que 11 por equipo (la diferencia es solo cuántos mandos se dan de alta). Si sale 16, sale 22.
- **¿Cómo?** **No con un programa externo solo, ni solo con Lua**: lo recomendado es **Link + módulo Sider + una DLL nativa de Phoenix cargada desde Sider**. Detalle en §2.

## 1. Las 5 piezas que hay que resolver (y su estado)
| # | Pieza | Qué hay que hacer | Estado del conocimiento |
|---|---|---|---|
| 1 | **Mandos virtuales** | Link crea los mandos extra (ViGEm, hoy 8 puestos) y apunta su GUID. | Link ya lo hace para 8; ampliar a 16–22 es trabajo de Link. |
| 2 | **Darlos de alta en el juego** | Por cada mando 9…N: `HidMan` (`0x1437F0A58`) +0xE8 (botones), +0x120 (activar), +0x118/+0x110 (GUID). | **Conocido** (direcciones exactas). `HidMan` admite hasta 32. |
| 3 | **Que el partido los escuche** | Cambiar `cmp …, 8` en `0x140A64816`, `0x140A648AF` (`FUN_140A64810/64850`) y `0x140A647E0`; dar «presente» y «sin configuración» a los bloques 8+ (las banderas/configs solo existen para 8). | **Conocido**; falta prueba. |
| 4 | **Asignar cada mando a un equipo/cursor** | Rellenar las ranuras de cursor (`[0x1436F3DC8]+0x180`, 24 × 0x74; `+0x10` = mando, `+0x20` = jugador que controla, `+0x0D` bandera, `+0x2C` otro jugador, `+0x68` temporizador) para los mandos 9+. Las funciones de cursor que las leen están en `0x1404362D0…0x14043FD10`. | **Falta** encontrar quién las llena al empezar el partido. Con prueba en vivo: punto de ruptura de escritura en `ranura+0x10` → se encuentra en minutos. |
| 5 | **Pantalla de lados** | Opción A: no usarla para los extra (Link decide el lado de cada uno). Opción B: ampliarla a 9–10 fichas. | A: depende de la pieza 4. B: techo 10 (claves `sideselect_0…9`), mucho trabajo. **Recomendado A.** |

Además, por confirmar en vivo: que el juego no tenga otro tope por equipo (por ejemplo, «máximo N humanos por lado» en algún modo) y que la IA no pelee con los cursores humanos extra.

## 2. Estrategia recomendada (arquitectura)
```
Phoenix Link (fuera del juego)                PES 2021 (dentro del proceso)
- crea 16–22 mandos virtuales (ViGEm)   ──►   settings.dat (8 casillas, como hoy)
- escribe phoenix_mandos_extra.dat            + phoenix_mandos_extra.dat (casillas 9…N)
  (GUID + botones + lado/equipo de cada uno)          │
- UI: quién juega en qué equipo                         ▼
                                              módulo Sider «phoenix_mandos.lua»
                                              (luajit.ext.enabled = 1 → FFI)
                                                │  ffi.load("phoenix_mandos.dll")
                                                ▼
                                              DLL nativa de Phoenix (C++)
                                              - parcha los 3 «cmp 8» (con copia y deshacer)
                                              - da de alta los mandos 9…N en HidMan
                                              - en cada partido: rellena las ranuras/cursores
```
- **Por qué no solo Lua:** Lua puede llamar funciones y escribir memoria, pero los ganchos por fotograma y los parches de varias instrucciones se hacen mejor y más seguros en código nativo.
- **Por qué no un programa aparte que escriba en la memoria de PES desde fuera:** funcionaría para pruebas (como `parchear_campo.exe`), pero llega tarde (cada fotograma) y es frágil. Sider ya está dentro del juego y en el instalador de Link; es el canal natural.
- **Por qué no el online (servidor tipo PESBUL):** exigiría una PC con PES por persona y un protocolo cifrado que nadie ha reconstruido aún.
- **Reversible:** quitar el módulo de Sider = juego original. Copias de `settings.dat` como hoy.

## 3. Plan por fases (cuando Fralex autorice pruebas)
1. **Fase 0 (10 min, en vivo):** con x64dbg o un módulo Sider de solo lectura, ver quién escribe `ranura+0x10` al empezar un amistoso con 2 mandos (pieza 4).
2. **Fase 1 — 9 mandos:** 1 mando extra: alta en `HidMan` + parches `cmp 8 → cmp 9` + ranura asignada a mano. Éxito = el 9.º mando mueve a un jugador.
3. **Fase 2 — 8 vs 8 (16):** Link con 16 puestos + archivo de extras + lados desde Link.
4. **Fase 3 — 11 vs 11 (22):** lo mismo con 22; vigilar rendimiento, colores de cursor repetidos y la IA.
5. (Opcional) Fichas 9–10 en la pantalla de lados.

## 4. Riesgos y límites conocidos
- Colores de cursor: el juego tiene colores para pocos mandos; con 16–22 habrá repetidos (se puede cambiar la textura del cursor por mando más adelante).
- Rendimiento por Parsec/Link: 22 invitados en una sala es mucho tráfico (Link hoy está pensado para 8).
- Actualizaciones: las direcciones son de este exe (ConmeGOL sobre PES 2021, PATCH10100). Otro exe = volver a buscar (con los patrones del cap. 62 es rápido).

## Ampliaciones
(Vacío.)

### 19-oct-2026 ~19:45 — ¿Puede la interfaz del juego mostrar más fichas? (pantalla de lados)
**SÍ/NO primero: SÍ hay caminos; ninguno es fácil. El más prometedor: el juego YA TRAE una pantalla de lados para muchos jugadores (la del 11 vs 11 online).**
- **Pantalla offline (la de siempre)** — `menu::SideSelect` + `SideSelectEach`: cada mando es una ventana propia que solo avanza su animación (`FUN_1423198D0` = vftable +0x38 del clip, «siguiente fotograma»; `FUN_140E4B8D0` = fotograma actual); la posición la pone la animación Flash `sideSelect.bin`. Clonar fichas por código: techo **10** (claves `sideselect_0…9`); además las posiciones 9–10 dependen del Flash, que **no tiene herramienta pública de edición** (formato AFP de Konami, nombres codificados).
- **Pantalla del 11 vs 11 online** — `menu::MOMSideSelect` (vftable `0x14277B3D8`, constructor `FUN_140D9C900`): carga **`common/menu/mode/sideMatchMembers.bin`** («miembros por lado», en `dt11_x64.cpk`, pos. 792576, 43262 bytes → 72688 descomprimido) + `cmnArrowLarge.bin`; guarda los miembros en una **lista dinámica** (`+0xF8`), recorre hasta 32 mandos, pone el avatar de cada miembro (`avater_tex_…`, `FUN_140D9DEC0`) y sincroniza por red con `TaskSyncSelectSideInMulti`. Los datos de los miembros salen del objeto de sala (`+0xD8`; `FUN_140FF3160/3280/32E0`). **Idea «maravilla»:** abrir esta pantalla en local y darle una sala falsa con nuestros mandos como miembros. Riesgo: depende del modo online (comprobación `FUN_14103B3A0`, tareas de red). Falta saber cuántas filas dibuja (el Flash está codificado; por su función, 11 por lado).
- **Motor gráfico** (para el futuro): reproductor Flash propio (`flash::MovieClip`, vftable `0x142BB2A70`; `flash::Window`, `flash::Page`, `flash::Root`, `flash::Choice`, **`flash::Duplicate`**). Al cargar un Flash, `FUN_141F9A070` reconoce clips con nombres especiales; `_duplicate` (`FUN_141F9A3A0`) registra una **plantilla clonable** (así se hacen las listas con filas variables).
- **Camino más seguro para una primera versión:** elegir el lado de los mandos extra **fuera del juego, en Link** (cada invitado elige equipo en Link con su mando o en la web) y que la DLL lo aplique al empezar el partido. Las 8 fichas originales siguen funcionando como siempre.
- **Plan de interfaz en orden:** (1) lados en Link (rápido, seguro) → (2) probar `MOMSideSelect` en local con sala falsa (la «maravilla») → (3) solo si (2) falla: clonar fichas offline hasta 10.

### 19-oct-2026 ~20:00 — ¿Cómo superar las 8 casillas de `settings.dat`? + Fase 0 preparada
**Respuesta:** no se amplía `settings.dat` en sí. Opciones estudiadas:
1. **Archivo propio de Phoenix (RECOMENDADO):** `settings.dat` sigue con sus 8 casillas (los mandos 1–8, como hoy con «Orden de mandos» de Link). Los mandos 9…N van en `phoenix_mandos_extra.dat` con **el mismo formato de casilla** (GUID de 16 bytes + 24 botones de 2 bytes = 0x40). Link lo escribe (ya sabe leer el GUID de cada puesto, `OrdenPes.cpp`) y el módulo Sider + DLL los da de alta en `HidMan` (+0xE8/+0x120/+0x118/+0x110).
2. **Casillas extra al final de `settings.dat`:** el juego solo lee los primeros 612 bytes (`FUN_140408C50`), así que no le molestan. Riesgo: `Settings.exe` reescribe el archivo y las borraría. Válido, pero peor que la 1.
3. **Parchear el juego para que lea un `settings.dat` más grande:** habría que cambiar el tamaño 0x264, mover la copia en memoria (`0x1436F0570`, sin hueco detrás), el bucle de 8 de `FUN_140402660` y su lista de 9 textos de botones en la pila. Demasiado invasivo: descartado.
**Fase 0 preparada (no instalada):** `modulo_sider/phoenix_mandos_sonda.lua` (solo lectura; vigila `HidMan` y las 24 ranuras; tecla J = marca) + `modulo_sider/GUIA_PRUEBA_MANDOS_FASE0.md`. Sintaxis comprobada con luaparse; `luajit.ext.enabled = 1` ya está en el `sider.ini` de Fralex.

### 19-oct-2026 ~19:30–20:00 — Resultado de la prueba en vivo (sonda v0.2) [CONFIRMADO EN VIVO]
- **Sin cierres** con la v0.2 (VirtualQuery + sin leer la cabecera del exe). La v0.1 coincidió con un cierre dentro del código de la protección (c0000005 en PES2021.exe +0xEE896B0).
- **HidMan en vivo:** 32 mandos lógicos y tabla de GUID de 32 entradas; `+0xC0` = 1 y `+0xC8` = 0x1 todo el rato (no es «mandos activos»; parece «mando principal»).
- **Ranuras del partido al empezar:** `ranura[i].mando = i` para i = 0…7 (mando N en la ranura N, fijo); **ranuras 8…23 con mando = 0xFFFFFFFF (vacías)**. Jugador controlado: ranura 0 (Fralex, Local) → jugador 10; ranura 1 (Usuario 2, Visitante) → jugador 21; ranuras de mandos en el centro → 255 (no juega). Al cambiar de jugador: ranura 1 pasó a 20 y `+0x2C` guardó 21 (= jugador anterior).
- **Consecuencia:** para el 9.º mando basta, en principio, con poner `ranura[8].mando = 8`, darle lado y que el partido lea su entrada (parches `cmp 8`). Falta el **campo del lado** dentro de la ranura → sonda **v0.3** (instalada) copia las ranuras 0–9 completas al pulsar J.
- **Pantalla de lados (capturas guardadas en `informes/datos/sonda_mandos/1-3.png`):** tabla de **8 filas, una por mando**, 3 columnas (Local · centro · Visitante); cada fila: nombre (FRALEX, Usuario 2, ARENZ…), mando DualSense con borde de color propio (cian, rosa, amarillo, verde, naranja, violeta, gris, beige) y flechas ‹ ›; huecos grises donde no está; botones abajo: Confirmar, Atrás, Editar datos personales, Modo entrenador, L2/R2 elegir datos personales. Para 16–22 mandos la versión Phoenix necesitará filas más bajas, dos columnas de filas o desplazamiento.

### 19-oct-2026 ~20:30 — Formato de la ranura de cursor (0x74 bytes) [CONFIRMADO EN VIVO, sonda v0.4]
| Offset | Qué es | Local (ranura 0) | Visitante (1) | Centro (2) | Vacía (8) |
|---|---|---|---|---|---|
| `+0x00` | **Lado: 0 = Local, 1 = centro/no juega, 2 = Visitante** | 0 | 2 | 1 | 1 |
| `+0x01` | Número de ranura | 0 | 1 | 2 | 8 |
| `+0x10` (u32) | Mando (bloque PadInput) | 0 | 1 | 2 | 0xFFFFFFFF |
| `+0x18` (u32) | Mando lógico de HidMan (32 = ninguno) | 0 | 1 | 2 | 0x20 |
| `+0x20` (u32) | Jugador controlado (0–21; 255 = ninguno) | 10 | 21 | 255 | 255 |
| `+0x28` (u32) | Primer jugador de su equipo (0 Local, 11 Visitante) | 0 | 11 | 0 | 0 |
| `+0x2C` (u32) | Jugador anterior (al cambiar de cursor) | 255 | 255 | 255 | 255 |
| `+0x68…+0x72` | Banderas/estado del cursor (activo = 1 en +0x68, +0x72) | 01…01 | 01…01 | 00… | 00… |
Datos crudos: `informes/datos/sonda_mandos/cambios_v04_20261019.txt`.

**Plan Fase 1 (9.º mando), sin llamar funciones del juego:**
1. Escribir el GUID del 9.º mando físico en la tabla de HidMan (`[0x1437F0A58]+0xD0`, entrada 8, bytes `+0x10…+0x1F`): `FUN_141B54F60` lo engancha solo en el siguiente fotograma.
2. Cambiar los `cmp …, 8` del llenado del partido (`0x140A64816`, `0x140A648AF`, `0x140A647E0`) para que el bloque 8 se llene; cuidado con las 8 banderas (`+0xBBE0`) y las 8 configuraciones (`+0xBBE8`), que no tienen hueco para el 9.º.
3. Al empezar el partido, rellenar `ranura[8]`: lado (+0x00), mando 8 (+0x10, +0x18), equipo (+0x28), jugador inicial (+0x20) y banderas de activo copiadas de una ranura que juegue.
Necesita un 9.º mando físico (por ejemplo, el mando propio del anfitrión, que hoy PES ignora porque no está en `settings.dat`).

### 19-oct-2026 ~21:00 — Fase 1 en vivo: resultado y siguiente paso
**Resultado [CONFIRMADO EN VIVO]:** con la sala de Link abierta (8 mandos virtuales) + el mando físico de Fralex:
- La Fase 1 encontró el mando físico en el árbol DirectInput (único mando fuera de `settings.dat`, GUID `b0ae4364…`), lo dio de alta como **mando lógico 8** en HidMan (métodos +0xE8/+0x120/+0x118/+0x110 → sin error) y aplicó/deshizo los parches de código **sin cierres**.
- En el partido llenó la ranura 8, pero **el juego la vacía una y otra vez** (8 rellenos en ~100 s) y no apareció un cursor nuevo. Además, un fallo del módulo asignó el jugador 11 (rival) desde el 2.º relleno (corregido en la copia local, no instalado).
**Por qué se vacía [CONFIRMADO en Ghidra]:** la tabla de ranuras es un objeto «registro» que el juego **copia** en muchos sitios (constructor `FUN_140A54C50` → `FUN_140A55980`, 13 referencias de creación): escribimos en una copia y se reemplaza por la versión oficial. El objeto tiene además: `+0xAE0` = 24 bloques de 0x3C por ranura; **`+0x1084` = 22 entradas «jugador → ranura» (24 = ninguna)**; `+0x10DC/+0x10E0/+0x10E4/+0x1148` contadores; `+0x10E8` = 24 estados (4 por defecto).
**Siguiente paso:** encontrar al **productor** de la versión oficial (el que pone `ranura[i].mando = i` para i < 8 a partir del resultado de la pantalla de lados) y ampliarlo a 9. Vías: (a) en vivo, sonda que vigile en qué fotograma cambia la tabla y desde qué copia; (b) Ghidra: funciones que escriben `+0x1084` y las 13 que crean el registro (`FUN_14041E3F0`, `FUN_14048B4E0`, `FUN_140A68FA0`, `FUN_141FAF980`, `FUN_141FD5A00`, `FUN_141FE10C0`, `FUN_1420CDA20`, `FUN_1420D7460`, `FUN_14214B0C0`, `FUN_142169220`, `FUN_14216A910`, `FUN_14216FD70`).
**Estado instalado en la PC de Fralex:** `phoenix_mandos_fase1.lua` v0.1 (inactivo hasta pulsar I). Volver atrás: `sider.ini.antes-mandos-20261019`.

### 20-oct-2026 — El productor oficial y la configuración del partido [CONFIRMADO en Ghidra]
- La tabla de ranuras es el registro **`match::registry::CursorInfo`** (doble búfer de 0x114C; ctor `FUN_14041E3F0` y otros 11). Escritores (`ScopedWrite`/`ScopedWAndC<CursorInfoRef>`): `FUN_141FBE2A0`, `FUN_141FE9830` (modo especial, opción `Def_OnMode_side`, bucle de 8 con pila de 8), `FUN_1421674D0`, `FUN_141FE66D0`, `FUN_142165F30`, `FUN_14214CC10`, `FUN_141FE3590`, `FUN_141FD6190`, `FUN_141FCF130`.
- **`FUN_140A571A0` (asignar mando a ranura) recorta: mando > 7 → vacío** (`83 FA 08 / 0F 43 D0`). Otro 8 escondido. Parche: `0x140A571A3` `83fa08 → 83fa18`.
- **Productor principal `FUN_141FB9040`**: recorre las 24 ranuras y toma de la **configuración del partido** S = `[[0x143705E10]+0x50]+0x3F308`:
  - `S+0x2D+ranura` = **lado** (24 bytes);
  - `S+0x45+ranura` = **mando lógico** de HidMan (24 bytes; 0x20 = ninguno; `FUN_1414BDB90`);
  - `S+0x180+ranura` = índice de mando local < 8 para banderas/config de PadInput (`FUN_1414BDBB0`);
  - `S+0x19E+equipo×11+jugador` = ranura del jugador (`FUN_1414BDC00`); `S+0x1B4/0x1B5` = ranuras especiales.
  - Datos personales por mando lógico: `[...]+0x50` + mando×0x980.
- **Fase 1 v0.2** (instalada): en vez de escribir la ranura, escribe `S+0x2D+8 = 0` (Local) y `S+0x45+8 = 8`, más el parche del recorte; así el propio juego crea y mantiene la ranura 8. Registra las tres listas de S para comprobar el formato en vivo.

### 20-oct-2026 ~10:30 — Fase 1 v0.3 en vivo [CONFIRMADO EN VIVO]
- **La lista de lado de los ajustes usa otro código:** `S+0x2D+ranura`: **0 = nadie, 1 = Local, 2 = Visitante** (el productor lo traduce a 1/0/2 en la ranura). Al empezar el partido el juego llenó `S+0x45` = `00 01 … 07` y respetó nuestro `08`.
- Con `S+0x2D+8 = 1` y `S+0x45+8 = 8`, **el propio juego creó la ranura 8 con mando 8, lado Local y jugador 10** (luego jugador 1 tras cambiar lados). Primer 9.º «usuario» dentro del partido.
- **Pero no se vio un 9.º cursor ni se movió un jugador extra**, y el juego **reabría la pantalla de lados sin parar** (también tras desactivar: el mando lógico 8 sigue dado de alta). Solo se arregla cerrando PES.
- Hipótesis: (a) la pantalla de lados se reabre porque PES detecta un mando activo que no cabe en sus 8 filas (o porque reescribimos los ajustes cada 0,5 s); (b) el cursor no se dibuja porque los indicadores/colores existen solo para 8; (c) la entrada del bloque 8 quizá no llega (falta verificar `PadInput` bloque 8).
- Siguiente: escribir los ajustes una sola vez justo después de confirmar lados; vigilar el bloque 8 de PadInput (¿cambia al mover el mando?); localizar quién reabre la pantalla de lados.

### 20-oct-2026 ~12:00 — Colores de flecha: ¿se pueden crear colores nuevos? **SÍ** [CONFIRMADO EN CÓDIGO; falta verlo en vivo]
- **Quién pone el color:** `FUN_1420BD9C0(salida, nº, variante)` devuelve un color **RGBA** (formato `R | G<<8 | B<<16 | A<<24`, empaquetado por `FUN_140490590` desde 4 floats). Tabla estática de **solo 6 colores** en `0x1437F9668` (se crea la 1.ª vez): **0 cian (0, 0.94, 1) · 1 rosa (1, 0.27, 0.45) · 2 amarillo (0.95, 0.93, 0) · 3 verde (0.31, 0.89, 0.02) · 4 naranja (1, 0.61, 0) · 5 violeta (0.95, 0.5, 1)**. Del 6 en adelante: **gris** (claro 0.83 si variante = 0, casi negro 0.098 si ≠ 0). O sea: **en el partido los mandos 7 y 8 ya salen grises**, y un 9.º también saldría gris.
- **Qué número usa la flecha:** `match2D::Screen::ModelCursor` (vftable `0x142BDE210`, 22 modelos, uno por futbolista) → método `+0x70` `FUN_1420DB740` → índice = byte `+0x0D` de la entrada de cursor 2D (= `ranura+0x10`, el **nº de mando/bloque PadInput**; `0xFF` si la bandera `ranura+0x0D` está puesta) y variante = `+0x0E` (= `ranura+0x08`). Online/cooperativo usa el nº de cursor.
- **El color se aplica al modelo** `d2_cursor_a` con `FUN_141F9F870` (bytes `+0x44..+0x47` = R, G, B, A del modelo). Como es un tinte, **sirve cualquier color**.
- **La capa 2D del partido maneja 24 cursores**, no 8: `FUN_1420C46F0` (opción `Def_Game_match2d_cursor_on`) recorre las 24 ranuras y copia a entradas de 0x20 bytes en `pantalla+0x19C8` (`+0x00` visible, `+0x04` lado, `+0x08` jugador, `+0x0C` 2D, `+0x0D` mando, `+0x0E` variante, `+0x10` mando lógico, `+0x1C` dato +0x2C de la ranura). Llamada desde `FUN_1420C5330`.
- Otros que piden color a la misma función (placas de nombre, radar…): `FUN_1420CC560`, `FUN_1420D1A40`, `FUN_1420D2310`, `FUN_1420D6980`, `FUN_1420D71D0`, `FUN_1420D7380`, `FUN_1420DF5F0` → al cambiarla, **todo el partido** usa el color nuevo de forma coherente.
- **Implementado en Fase 1 v0.4** (instalada; copia de la v0.3 en `modules\phoenix_mandos_fase1.lua.v03-20261020`): la entrada de `FUN_1420BD9C0` salta a un código propio (página `VirtualAlloc`) que, **solo para los mandos 8–23**, devuelve: 8 rojo, 9 azul, 10 blanco, 11 marrón, 12 verde oscuro, 13 granate, 14 azul marino, 15 negro (y se repiten). Los mandos 0–7 siguen por el código original sin cambios. Cuenta cuántas veces el juego pide cada color 8+ (**si el contador sube, el juego está dibujando la flecha del 9.º mando**). I = restaura los 13 bytes originales.
- v0.4 además: los ajustes del 9.º mando se escriben **una sola vez, con el partido en marcha**, y se devuelven a su valor original al salir del partido (3 s sin cursores) o al pulsar I → debería acabar el bucle de la pantalla de lados. K = escribirlos a mano.
- Lo que **no** cambia: la pantalla de lados (menú Flash) tiene 8 colores propios (`padColor_0..7`, cuadros de la animación) y 8 filas; eso irá en la pantalla Phoenix propia.

### 20-oct-2026 ~13:30 — Pantalla Phoenix «Elegir lado» (Fase 1 v0.5, instalada; falta verla en vivo)
- **Cómo se ve dentro de PES:** la ventana de Sider admite **una imagen** además del texto (`overlay_on` devuelve texto, ruta de imagen y opciones `image_width`, `image_alpha`…; doc local `informes/investigacion/FUENTE_sider7_scripting_doc.txt`). El módulo **dibuja la imagen él mismo** (BMP 1280×720) y la vuelve a escribir con otro nombre cada vez que algo cambia (para que Sider recargue la textura).
- **Diseño:** fondo estilo menú PES con 3 zonas **LOCAL (azul) · SIN LADO · VISITANTE (rojo)**, una fila por mando con su color (los 8 de PES + rojo, azul, blanco… para 9+), número, nombre y «sin conectar» atenuado; fila elegida con borde dorado y flechas ‹ ›; arriba a la derecha «Local N · Visitante N» y el aviso. Vista previa: `modulo_sider/phoenix_lados/vista_previa_20261020.png`.
- **Piezas:** `modulo_sider/herramientas/generar_pantalla_lados.ps1` (Windows/System.Drawing, letra Bahnschrift) crea `fondo.raw`, `fuente.raw` (atlas de letras), `mando.raw` y `fuente.lua` → instaladas en `SiderAddons/content/phoenix/lados/`. Vista previa sin PES: `herramientas/vista_previa_lados.js` (ejecuta el MISMO módulo Lua en wasmoon, sin ffi).
- **Controles:** con la ventana de Sider en este módulo (Espacio y luego `1` hasta llegar a él): ↑↓ elegir mando · ←→ cambiar de lado · ENTER (o A en el mando del anfitrión) confirmar · I activar/deshacer mandos extra. Mientras está abierta, `input.set_blocked(true)` evita que el juego reciba esas teclas.
- **Qué hace al confirmar:** mandos 1–8 → escribe su lado en la lista de ajustes (`S+0x2D`) en ese momento y otra vez al empezar el partido; mandos 9+ → los da de alta (TODOS los que no están en settings.dat, como mandos lógicos 8–15) y escribe lado + mando solo con el partido en marcha, devolviéndolos al salir.
- **Parches ampliados a 16:** `FUN_140A64810` y `FUN_140A64850` aceptan índices < 16 (antes < 9). Comprobado en Ghidra que `FUN_140A64850` solo usa el índice para pedir el mando lógico a HidMan (admite 32) → seguro.
- Nombres: `content/phoenix/mandos/nombres.txt` con líneas `9=PEPE` (Phoenix Link podrá escribirlo).
- **Límite conocido:** cada amigo no puede mover SU fila con SU mando todavía: Sider solo entrega la entrada de un mando (el del anfitrión). Siguiente: leer cada mando desde HidMan/PadInput o que Link mande los lados.

### 20-oct-2026 ~13:45 — RÉPLICA EXACTA de la pantalla «Local / Visitante» (Fase 1 v0.6, instalada; falta verla en vivo)
- Fralex: «quiero una réplica exacta del menú de PES, si no, no quiero nada» → se descarta el diseño propio de la v0.5.
- **Método:** se arma con los **píxeles reales de PES** recortados de su captura a 1920×1080 (`informes/datos/sonda_mandos/1.png`) por `modulo_sider/herramientas/generar_replica_lados.ps1`: fondo con la tabla vacía (solo se borra donde hay nombre/mando/flecha y el hueco se rellena interpolando la textura), los 8 DualSense de color reales, 8 más recoloreados para los mandos 9–16 (rojo, azul, blanco, marrón, verde oscuro, granate, azul marino, negro), mando gris, flechas encendida/apagada, cabecera y botones de abajo intactos. Cada pieza se pega como «fondo destino + (pieza − fondo origen)».
- **Medidas reales de la pantalla de PES:** filas de 90 px desde y=134 (8 filas, tabla x 249–1670); centros de columna 479 / 961 / 1439; nombre en y+8..y+26 (color ≈ #323232); mando en y+27..y+88 (94×62); flechas: ‹ en x 800 (centro) y 1279 (Visitante), › en x 629 (Local) y 1109 (centro), y+43..y+72; flechas encendidas si el mando está en un lado, apagadas en el centro; la fila del primer mando no lleva mando gris en el centro.
- **Letra:** la de PES es tipo DIN; la más parecida en Windows es **Bahnschrift SemiLight 26 px** (ajuste automático contra la captura); las mayúsculas de PES van ~1,9 px más separadas.
- **Comparación píxel a píxel** con la captura real (mismo estado): solo el **0,32 %** de los píxeles difiere (bordes suavizados de las letras). Imagen `modulo_sider/phoenix_lados_replica/comparacion_pes_vs_replica.png`.
- **Más de 8 mandos:** misma tabla, 8 filas a la vista y se desplaza con una barra fina (`vista_12_mandos.png`). Solo hay filas para mandos conectados, como en PES; nombres «Usuario N» o los de `content/phoenix/mandos/nombres.txt`.
- Se muestra a **pantalla completa** en la ventana de Sider (sin texto ni fondo de Sider). Piezas instaladas en `SiderAddons/content/phoenix/lados_replica/`.
- **Pendiente para que sea igual también en el manejo:** que cada amigo mueva su propia fila con su mando (Sider solo da el mando del anfitrión: hay que leer cada mando desde HidMan) y que salga sola en lugar de la de PES (hoy: Espacio y `1`).

### 20-oct-2026 ~13:45 — Pantalla en vivo + blindaje (Fase 1 v0.7)
- **En vivo:** con `image_width/height = 0.99999` la réplica sale a pantalla completa, se mueve y desplaza bien (16 mandos). Espacio la abre/cierra (es la ventana de Sider).
- **Causas de los fallos de hoy (todas corregidas):** (1) Sider no deja `pcall`, `loadstring`, `os.remove` en módulos; (2) `image_width = 1.0` = **1 píxel** (fracción solo si < 1); (3) crash al empezar el partido: el código de colores leía el color en `rax` y luego lo usaba como índice del contador → escritura fuera de sitio (c0000005 en página+0x1B). El cierre de las 13:06 (dentro de `free()` de PES, `FUN_1403FD360`, asignador tipo dlmalloc) encaja con esas escrituras sueltas: corrupción de memoria.
- **Blindaje v0.7 (Fralex: «blinda todo de antemano»):** código de colores probado **ejecutándolo** fuera del juego (`herramientas/probar_codigo_colores.ps1`: color correcto, contador correcto, sin cierre); la página se escribe y COMPRUEBA antes de desviar la entrada; si el desvío ya existe (recarga/versión vieja) se adopta y se reescribe con el código bueno; no se repite el alta de un mando que ya tiene ese GUID; la tecla I no hace nada con el partido en marcha; los mandos 1–8 ya no se tocan al empezar el partido (solo al confirmar con ENTER); los extra se devuelven solo tras 15 s reales sin ninguna ranura jugando (o 90 s sin cursor), con `os.time` (no `os.clock`, que es tiempo de CPU); el juego solo se bloquea si la pantalla se puede dibujar; «hay extra» se calcula por nº de mando, no por nº de filas.

### 20-oct-2026 ~14:30 — Fase 1 v0.8: los extra entran a tiempo + la pantalla sale SOLA
- **Por qué los extra no entraron (prueba de v0.6.5):** los ajustes se escribían con el partido ya empezado; el productor (`FUN_141FB9040`) ya había repartido mandos→jugadores. Registro: «peticiones de color 1/1/…» (solo al iniciar) y ranura 8 vacía. (En v0.3 sí entró el 9.º porque se escribía sin parar, también antes.)
- **Gancho del productor:** al entrar en `FUN_141FB9040` (prólogo `40555356574154415541564157488d6c2488`, 18 bytes sin direcciones relativas) un código propio escribe en S los lados+mandos 9–16 (y los lados 1–8 si se confirmaron en la pantalla Phoenix) → llegan exactamente a tiempo. Al salir del partido se devuelven (lado 0, mando 0x20).
- **Pantalla de lados de PES = `menu::SideSelect`** (vftable `0x142673558`; 2.ª `0x1426737B0`): **constructor `FUN_140ABEAD0`** (prólogo `4488442418488954241048894c240853555657`), **destructor `FUN_140ABF000`** (`40574883ec3048c7442420feffffff`; lee S y cuenta usuarios por lado con `FUN_1414BDC30(S, lado)` → si un lado tiene ≥2, `FUN_1414C5950(…,1)`). Ganchos que solo cuentan aperturas/cierres (+ guardan el objeto) → la pantalla Phoenix se abre SOLA encima de la de PES (pulsando las teclas de Sider, solo si PES está en primer plano) y se cierra sola al confirmar o cuando PES cierra la suya.
- **Probado fuera del juego ejecutando el código máquina** (`herramientas/sacar_codigos_ganchos.js` + `probar_ganchos.ps1`): 15/15 (apagado no toca nada, escribe solo donde debe, nativos solo si se confirmaron, punteros nulos sin caerse, contadores y objeto).

### 20-oct-2026 ~14:50 — v0.8 en vivo: ¡el juego creó al jugador del mando 9! + arreglo v0.8.1
- **En vivo (v0.8):** gancho del productor → «ranura 8 = mando 8 / jugador 10 / lado Local» y flechas de color nuevas pedidas (contador 3→19). Los extra **sí entran** al partido.
- **Fallo:** a mitad de partido (una pausa de ~15 s sin cursores) Lua devolvió los ajustes de los extra → PES notó el cambio, rehízo el reparto y **abrió su pantalla de lados en pleno partido** (varias veces) → la Phoenix salía sola encima.
- **v0.8.1:** Lua ya no toca S durante el partido; la limpieza la hace el gancho del **constructor de la pantalla de lados** (`FUN_140ABEAD0`): antes de que la pantalla lea S, pone lados 9–16 = 0 y mandos 9–16 = 0x20. La Phoenix no se abre sola si hay partido en curso. Pruebas del código ejecutándolo: 21/21.

### 20-oct-2026 ~15:40 — Por qué PES abre su pantalla de lados en pleno partido (v0.8.4 diagnóstico → v0.8.5)
- **Pila al abrirse** (siempre igual): `140ABF63A` (fábrica `FUN_140ABF5E0` → ctor SideSelect) ← `140A95522` (`FUN_140A95340`, sistema de menús) ← … ← `141FA0C71` (`FUN_141FA0BC0`: busca la fábrica por NOMBRE en la lista `DAT_1437F8998`) ← `141FA90ED` (`FUN_141FA8FA0`: transición de flujo). Es el **sistema de flujos JSON** (`dt16_all.cpk` → `common/script/flow/**`).
- **Flujo del partido:** `Match/MatchIdle` → evento `00_match_pause` → **`Match/MatchPrePause`** (proceso) con salidas `0_pause` → `MatchPause`, **`1_side_setting` → `MatchSideSetting`** (la pantalla de lados), `2_plan`. `MatchSideSetting` vuelve por `return` → `MatchPlayerSelectCheck` → `MatchPause`. `MatchPostPause` → `proceed` → `MatchIdle` (reanudar).
- **Conclusión:** PES elige `1_side_setting` cuando pide pausa un **mando sin equipo** (función «unirse al partido»): algún mando extra manda START. Sin cambios de dispositivos (diagnóstico DIAG vacío). Hipótesis de causa: la configuración de botones copiada a los extra era la del mando lógico 1.
- **v0.8.5:** (1) los extra copian la configuración de botones **más repetida** entre los mandos 1–8; (2) el módulo sirve un `MatchPrePause.json` propio (eventos `livecpk_make_key`/`livecpk_get_filepath`, archivo en `content/phoenix/flujo/`) idéntico al de PES salvo `1_side_setting` → `Match/MatchPostPause`: un START de un mando sin equipo ya no abre nada (cambiar de lado sigue en el menú de pausa, `08_side_setting`). Ojo: ConmeGOL ya sustituye ese archivo en `livecpk/Menu` (contenido idéntico al original); falta confirmar en vivo que el del módulo gana (registro «flujo de pausa Phoenix servido»).

### 20-oct-2026 ~16:05 — CAUSA REAL del START fantasma: un mando que no existe (v0.8.7)
- **v0.8.6 en vivo:** ya no salía la pantalla de lados, pero PES se trababa/parpadeaba: el START fantasma ahora iba a `MatchPostPause` → pausa/reanudar sin parar (el productor pasó 127 veces en 90 s).
- **Lectura de la memoria de PES en marcha** (solo lectura; `herramientas/leer_botones_pes.ps1`, `leer_dispositivos_pes.ps1`): las configuraciones de botones de los extra eran correctas (la de Link, 7 de 8; el mando 0 tiene otra de 806 car. = el DualSense). Pero en el árbol de dispositivos DirectInput de PES (`[0x1437F0AF8]+0x90`), el objeto del dispositivo `90760ff5…` tiene **la interfaz DirectInput en +0x20 = 0**: es una entrada vieja de un mando que ya no existe. Se había dado de alta como mando lógico 12 → PES leía basura (START) → «mando sin equipo pulsa START».
- Formato útil del objeto de dispositivo DI (nodo+0x30): `+0x20` puntero a la interfaz (0 = sin mando real) · `+0x28` 03 · `+0x29` tipo (1 mando, 2 ratón, 3 teclado) · `+0x2A` nº de orden · `+0x30` GUID de instancia.
- **v0.8.7:** solo se usan como extra (y solo se cuentan como conectados) los dispositivos con interfaz ≠ 0; el registro marca los demás «SIN mando real: se ignora». El diagnóstico marca «(muerto)».

### 20-oct-2026 ~16:30 — LOS EXTRA NUNCA SE ENLAZABAN con su mando físico (v0.8.8)
- **v0.8.7 en vivo:** seguía el bucle, y el mando que fallaba seguía siendo la **plaza lógica 12** aunque ya fuera otro dispositivo → el problema era de las plazas, no de un mando.
- **Tabla de HidMan leída en vivo** (`herramientas/leer_tabla_hidman.ps1`, `leer_enlaces_hidman.ps1`): `HidMan+0xD0` (vector begin/end, 32 × 0x20) es a la vez **fila de dispositivo** (`+0x00` objeto del mando físico, `+0x08` plaza lógica enlazada) y **plaza lógica** (`+0x0C` fila de dispositivo enlazada, `+0x10` GUID). GUID del mando físico en `[objeto+0x20]`. Las plazas 8–15 tenían su GUID pero **`+0x0C = −1` (sin enlazar)** y sus objetos de mando lógico (`HidMan+0x90` + i·0x68, `+0x00`) **= 0**; las filas de dispositivo de esos mandos tenían `+0x08 = −1`.
- **Por qué:** `FUN_141B54F60` solo enlaza por GUID cuando un mando APARECE (fila vacía); escribir el GUID con el mando ya conectado no dispara nada. Al quedar sin mando, PES veía «usuarios con el mando desconectado» → pausa/lados/bucle.
- **v0.8.8:** el módulo enlaza como PES (fila.+8 = plaza, plaza.+0xC = fila) al activar y revisa cada segundo; anota si se crean los objetos de mando lógico; **freno de emergencia**: más de 15 pasadas del productor en 10 s → los extra dejan de meterse en el partido.

### 20-oct-2026 ~15:35–16:50 — v0.9: la Phoenix es un ESPEJO de la pantalla de PES (se siente igual) [CONFIRMADO EN VIVO]
**Pedido de Fralex (antes de irse a comer):** «debe replicar exactamente la funcionalidad y diseño visual del apartado original… se debe sentir que es lo mismo literalmente».
**Problemas de la v0.8.13 que se vieron en vivo:** la réplica no copiaba la pantalla de PES (salía «Usuario 1» en el centro con FRALEX en Local debajo); nombres «Usuario N» en vez de los «Datos personales» (FRALEX, ARENZ, ROBERTO); la pantalla de PES se transparentaba (el «FRALEX» de PES se veía detrás); la barra de botones de abajo era fija (la de PES cambia según el estado); la entrada estaba bloqueada → solo el anfitrión movía filas; para abrirse sola recorría TODOS los módulos de Sider.
**Hallazgos (lectura de memoria en vivo, solo lectura):**
- **Filas de la pantalla de lados de PES:** objeto `SideSelect` (lo guarda el gancho del constructor en datos+0x30) `+0x90…+0xC8` = 8 punteros a fila (vftable `0x142715608`). Fila: `+0x98` nº de mando · `+0x9C` nº de perfil («Datos personales») · **`+0xA4` lado (0 Local · 1 centro · 2 Visitante)** (+0xAC lo repite). Medido moviendo el mando 1 con el teclado.
- **«Datos personales»:** lista de 32 perfiles de **0x980 bytes**; `+0x0` 0/1, **`+0x4` nombre** (0x2C bytes, UTF-8); va en SU PROPIO bloque de memoria (cabecera: `+8` = 0x4FFE0; los perfiles empiezan en +0x10). Perfil i ↔ mando i (también para los extra: en el partido el mando 9 salió como **MORGADO** = perfil 8 y el 12 como **BETO HORSEMAN** = perfil 11). Nombre vacío → «Usuario N». La dirección cambia en cada arranque y no hay puntero fijo corto → el módulo recorre las regiones de memoria una vez (≈3500 regiones, **53 ms**) y valida la lista.
- **Estado DirectInput de cada mando** (medido creando un mando virtual de prueba con ViGEm, `scratchpad/vgtest`): plaza lógica (HidMan+0xD0, idx×0x20) `+0xC` → fila de dispositivo → `+0` objeto → `[objeto+0x20]` (empieza por el GUID) **`+0x238` = DIJOYSTATE**: X (0 izq · 0x7FFF centro · 0xFFFF der), Y, …, cruceta `+0x20` (centésimas de grado; 0xFFFFFFFF suelta), botones `+0x30` (0x80 = pulsado; A = botón 0). Hay 3 copias seguidas cada 0x50.
- **Vtable del mando lógico** `0x142AC7660`: corta; `+0x38` = `FUN_141B5A160` reenvía a la vtable del dispositivo (no se usa).
- **`sider.ini`: `overlay.image-alpha-max = 0.8`** era la causa de la transparencia.
**v0.9.0–0.9.2 (instalada 0.9.2):**
1. **No se bloquea la entrada:** la pantalla de PES sigue debajo y recibe los mandos 1–8 (cada uno mueve su fila; «Confirmar» de PES funciona y **cierra las dos en un paso**).
2. **Espejo cada cuadro:** lado y nombre de 1–8 leídos de las filas de PES.
3. **Mandos 9–16 mueven SU fila** con su palanca o cruceta (un paso por pulsación); su lado va directo a los datos del gancho del productor.
4. **Nombres reales** de «Datos personales» (1–16); `nombres.txt` solo si alguien pone un nombre a mano.
5. **La imagen solo cubre la tabla (1920×870):** la barra de botones de abajo es la de PES (siempre exacta).
6. Más de 8 filas: ↑/↓ del teclado o la cruceta vertical de un extra desplazan la tabla; la fila del mando que se mueve se pone a la vista.
7. **Apertura automática** con «módulo ANTERIOR» (0xC0): este módulo es el último de sider.ini → 1 pulsación.
8. `sider.ini`: `overlay.image-alpha-max = 1.0` (copia `sider.ini.antes-alpha-20261020`; solo cambia esa línea).
9. Blindaje: las páginas de memoria «no legibles» ya no se recuerdan para siempre (la pantalla de PES se crea en memoria nueva cada vez); máximo de 200 variables locales de Lua respetado (la vista previa lo detectó antes de instalar).
**Prueba en vivo:** se abre sola y opaca; teclado → la fila 1 se mueve en PES y la Phoenix la copia; FRALEX/ARENZ/ROBERTO como en PES; ↓ muestra 9–15 con sus colores; **personas conectadas por Link movieron sus filas (2–6 nativos y 9/12 extra) y en el partido Liverpool tenía cursores FRALEX, MORGADO (mando 9) y BETO HORSEMAN (mando 12)** — `ajustes.lado` ranuras 0, 8 y 11 = Local. Confirmar cerró las dos pantallas a la vez.
**Pendiente:** menú de configuración por mando (□ «Editar Datos personales») cuando Fralex mande las capturas; que alguien pruebe mover a su jugador con un mando extra en pleno partido.

### 21-oct-2026 ~19:15 — Prueba «usuario 7» preparada (esperando a Fralex)
**SÍ/NO primero: NO hay resultado todavía; la herramienta está lista y PES estaba cerrado.**
- **Hipótesis:** PES mueve al jugador de una ranura según su «número de usuario local» (0–7), guardado en la lista `ajustes+0x180` (24 bytes, uno por ranura; `ajustes = [[0x143705E10]+0x50]+0x3F308`). Los mandos 9+ tienen ahí `FF`, por eso su jugador no se mueve aunque su entrada llegue al registro del partido.
- **Prueba:** Fralex en el puesto 8 de Link (mando lógico 7), puesto 13 conectado (VICTOR, ranura 12) y en un lado; amistoso en marcha. Se escribe `local[12] = 07`. Si el mando del puesto 8 mueve a VICTOR, la hipótesis queda confirmada y el siguiente paso es dar a cada mando 9+ su propio número (8…15) y ver si PES lo acepta o si hay que parchear el filtro `< 8`.
- **Herramienta:** `modulo_sider/herramientas/prueba_usuario7.ps1`. Sin parámetros solo lee y muestra las listas `lado`, `mando` y `local`; `-Escribir` pone el usuario (por defecto 7) en la ranura (por defecto 12); `-Deshacer` vuelve a poner `FF`. Solo memoria: no toca archivos y se pierde al cerrar PES. La v0.9.14 instalada no se cambió.
- **Ojo (registro del 20-oct, `content/phoenix/mandos/fase1.txt`, sesión v0.9.14 de las 21:45):** ya aparece `local[12] = 07` (t = 171 s y 784 s) y luego desaparece solo (t = 793 s) mientras la pantalla de lados cambiaba lados. Indica que **PES reescribe esa lista al confirmar lados**: hay que escribirla después de confirmar, ya con el balón en juego, y volver a leerla para ver si sigue puesta. No consta en el registro si en ese intento se movió VICTOR.

### 21-oct-2026 ~20:35 — Fecha límite para este frente
- **Decisión (Fralex aprobó aplicar la revisión general del proyecto):** los mandos 9–16 tienen fecha límite. La prueba del «usuario 7» decide.
  - **Si VICTOR se mueve** con el mando del puesto 8 y hay un camino claro y sin riesgo para dar a cada mando 9+ su número: se termina ese paso y se cierra.
  - **Si no se mueve, o el camino exige otra investigación larga:** el frente se **cierra en la v0.9.14** (punto seguro: pantalla de lados de 16, jugador y flecha para los 9+, sin que se muevan). No se abren nuevas líneas de ingeniería inversa sin preguntar a Fralex.
- **Para retomarlo algún día:** leer este capítulo desde «20-oct ~21:45» y el §21-oct; herramientas en `modulo_sider/herramientas/`; respaldo `RESPALDO_v0.9.14_punto_seguro_20261020.lua`.

### 23-oct-2026 ~04:18 — Resultado de la prueba «usuario 7» y CIERRE del frente en v0.9.14
**SÍ/NO primero: NO se movió. El frente queda cerrado en la v0.9.14, como se acordó el 21-oct.**

**Cómo se hizo (para repetirlo):**
1. PES con la v0.9.14 instalada (`sider.ini` línea 194: `lua.module = "phoenix_mandos_fase1.lua"`). Fralex en el puesto 8 de Link (mando lógico 7) y el puesto 13 conectado (ranura 12).
2. Amistoso en marcha. Lectura con `modulo_sider/herramientas/prueba_usuario7.ps1` (sin parámetros):
   - `lado  = 01 00 00 00 00 00 00 02 01 00 00 00 02 …` → ranura 0 Local, ranura 7 (Fralex) Visitante, ranura 8 Local, ranura 12 (fila 13) Visitante.
   - `mando = 00 … 07 08 09 0a 0b 0c 0d 0e 0f 20 …`
   - `local = 00 ff ff ff ff ff ff 07 ff …` → solo las ranuras de los mandos 1 y 8 tenían número de usuario.
3. `prueba_usuario7.ps1 -Escribir` con el balón en juego → `local[12] = 07`. Releído 3 s después: **se mantuvo** (el 20-oct PES lo borraba al cambiar lados; esta vez no).
4. Fralex movió la palanca del mando del puesto 8: **se movió su jugador, VICTOR (fila 13) no**.
5. `prueba_usuario7.ps1 -Deshacer` → `local[12] = ff` otra vez.

**Qué significa:**
- Cambiar la lista `ajustes+0x180` con el partido en marcha **no** basta para que un mando 9+ mueva a su jugador. O PES no mueve según ese número, o solo lo lee al crear las ranuras (antes del saque, en `FUN_141FB9040`) y después no vuelve a mirarlo.
- La segunda opción **no se probó**: habría que poner el número desde el gancho del productor, antes del saque. Eso es una versión nueva del módulo y más pruebas en vivo, o sea, otra línea de investigación. Por la fecha límite **no se abre sin preguntar a Fralex**.

**Error nuevo visto en la prueba (no es del módulo de mandos 9+):** con el mando del puesto 8, **L2 pausa el partido como si fuera START**. Es el mapa de botones de esa casilla (`settings.dat`, casilla 8, o la configuración de botones de Link para ese puesto). Se arregla asignando de nuevo los botones de ese mando en la herramienta de ajustes de PES (`Settings.exe`) o en Link. Pendiente de revisar.

**Estado final (punto seguro v0.9.14, respaldo `modulo_sider/RESPALDO_v0.9.14_punto_seguro_20261020.lua`):**
- Funciona: pantalla de lados con 16 mandos; los mandos 9–16 reciben ranura, jugador y flecha de color; su entrada llega al registro del partido.
- No funciona: el jugador de un mando 9+ no se mueve.
- Techos conocidos: 8 contra 8 = 16 humanos (tope de ViGEm, 16 mandos virtuales); 11 contra 11 = 22 y necesitaría otra forma de crear mandos virtuales.

**Para retomarlo algún día (orden):** (1) poner el número de usuario local de cada mando 9+ desde el gancho de `FUN_141FB9040`, antes del saque, y repetir esta prueba; (2) si sigue sin moverse, buscar dónde el partido filtra por usuario `< 8` (las funciones de `match::pad::ThinkUnit*`, en especial `ThinkUnitPadId`) con Ghidra.

### 23-oct-2026 ~04:35 — v0.9.15: número de usuario ANTES del saque — tampoco mueve
**SÍ/NO primero: NO. Con el número puesto antes del saque el jugador del mando 9+ sigue sin moverse. Se vuelve a la v0.9.14.**

**Qué se cambió (v0.9.15, guardada como `modulo_sider/phoenix_mandos_fase1.v0915_sin_efecto.lua`):**
- Datos del gancho `+0x38`: número de usuario local de los mandos 8–15 (= su propio número si tiene lado; si no, `FF`), puesto por `actualizarDatos()`.
- Gancho del productor `FUN_141FB9040`: además de lados y mandos, copia esos 8 bytes a `ajustes+0x180+8` justo al entrar (instrucciones `4d8b5a38` = `mov r11,[r10+0x38]` y `4c8998 <disp32>` = `mov [rax+disp32],r11`). Tamaño del código del gancho: 127 de 128 bytes.
- Gancho de «crear pantalla de lados»: además limpia `ajustes+0x180+8` a `FF` (los menús no ven números de usuario de los extra).
- Probado fuera del juego con `herramientas/probar_ganchos.ps1` (se añadieron 5 comprobaciones; resultado 27/27). Para ejecutarlo hace falta **PowerShell 7** (`pwsh`; con PowerShell 5 falla la conversión a `UIntPtr`) y el paquete `wasmoon` de Node para `sacar_codigos_ganchos.js` (se usó con `NODE_PATH` apuntando a una carpeta que ya lo tenía).

**Error de proceso que hay que evitar:** la primera respuesta «no se mueve» de Fralex fue con la **v0.9.14** todavía cargada: el módulo nuevo estaba solo en `modulo_sider` y PES no se había reiniciado. Siempre confirmar en `content/phoenix/mandos/fase1.txt` la cabecera `# fase1 v0.9.xx` de la sesión antes de dar por buena una prueba.

**Prueba válida (registro, sesión de las 04:3x):** pantalla confirmada con la fila 13 en Visitante → `ajustes.lado[12] = 02`, `ajustes.mando[12] = 0c`, **`ajustes.local[12] = 0c` antes del saque**; el productor pasó por el gancho. El del puesto 13 movió la palanca: **VICTOR no se movió**.

**Conclusión:** la lista `ajustes+0x180` (número de usuario local) **no es** el filtro que impide moverse a los mandos 9+, ni escrita en partido ni antes del saque. El filtro está en otra parte de la lógica del partido.

**Único camino que queda (no empezado; necesita permiso de Fralex):** investigar con Ghidra dónde el partido elige qué mando mueve a qué jugador (`match::pad::ThinkUnit*`, en especial `ThinkUnitPadId`, y el registro `match::registry::PadInput`), buscando una comparación `< 8` o una tabla de 8. Es ingeniería inversa larga, sin garantía.

### 23-oct-2026 ~04:50–05:00 — Lectura en vivo de ranuras y prueba de «modo de control» (NO mueve)
**SÍ/NO primero: NO. Copiar los ajustes de cursor del mando 1 al mando 13 no hizo que VICTOR se moviera.**

**Herramientas (scratchpad de la sesión; copiar a `modulo_sider/herramientas` si se retoma):** `ranuras.ps1` (lee las 16 primeras ranuras de `[[0x1436F3DC8]+0x180]`; ojo: hay que abrir el proceso con `0x410` y usar **PowerShell 7**; con el partido en pausa o en menús el puntero de la tabla vale 0), `perfiles.ps1` (lee `[[0x143705E10]+0x50]+0x48+i·0x980`, ajustes de cursor por mando) y `copiar_ajustes.ps1` (copia 0x18 bytes del mando 1 al 13 guardando el original; `-Deshacer`).

**Ranuras leídas en partido (v0.9.14, Fralex en Local con el mando 1, fila 13 en Visitante):**
| Campo | Ranura 0 (se mueve) | Ranura 12 (VICTOR, no se mueve) | Ranura 15 (puesto 16) |
|---|---|---|---|
| `+0x00` lado | 00 | 02 | 00 |
| `+0x10` / `+0x18` mando | 0 / 0 | 0x0C / 0x0C | 0x0F / 0x0F |
| `+0x20` jugador | 0x0A | 0x14 | 0x05 |
| `+0x28` | 0x09 | **0xFF** | 0x07 |
| `+0x2C` | 0x06 | 0x15 | 0x06 |
| `+0x34` | **01** | **02** | 02 |
| `+0x68…` | 01 01 04 02 00 00 00 00 00 01 00 | 01 01 04 01 00 00 01 00 01 01 02 | 00 01 04 03 00 00 01 00 01 01 00 |

**Productor `FUN_141FB9040` (Ghidra, `informes/datos/rea/mandos_98.txt`):** activa la bandera y la configuración del bloque de entrada **solo si el índice < 8** (`FUN_140a647e0` / `FUN_140a647f0`); el modo de control de la ranura sale de `FUN_1414c2250(ajustes de cursor del mando)` (0/1/2/3). La bandera solo la lee `FUN_140a64810`, que el módulo ya parchea, así que no es el freno.

**Prueba:** se copiaron los primeros 0x18 bytes de ajustes de cursor del mando 1 al 13 (dejando `+0x14` = nº de perfil del 13; el primer intento lo pisó y se corrigió). Fralex abrió y cerró la pantalla de lados (el productor volvió a pasar, registro t = 593 s). **VICTOR no se movió.** Cambio deshecho. No se pudo releer `+0x34` después porque la tabla de ranuras ya valía 0 (menú/pausa).

**Descartado hasta hoy:** usuario local `ajustes+0x180` (en partido y antes del saque), banderas de bloque `+0xBBE0`, ajustes de cursor del perfil. **Sin mirar:** `+0x28 = FF` en la ranura 12 (en las demás es un jugador) y quién lee el bloque de entrada de cada ranura en el partido (lo ideal: punto de vigilancia de lectura sobre el bloque 12 con Frida/depurador vía REA).

### 23-oct-2026 ~05:05–05:15 — Rastreo en vivo con REA (Frida): el partido solo lee UN bloque de entrada [HALLAZGO CLAVE]
**SÍ/NO primero: SÍ, ya sé dónde se pierde. El partido nunca lee el bloque de entrada del mando 13: solo lee uno (el del mando 1). La entrada llega, pero nadie la usa.**

**Cómo se hizo (repetible):**
1. REA conectado (si al abrir la sesión sale «connection timed out», reiniciar la sesión de Claude: REA 6.7.0 arranca en ~12 s y el límite es 30 s con muchos servidores a la vez).
2. `start_frida_session` modo `attach` al PID de `PES2021.exe` (dispositivo `local`). Solo lectura.
3. Script 1: `Interceptor.attach` en los 25 lectores del registro de entradas (los llamadores de `FUN_140a64790`, lista en `informes/datos/rea/mandos_95.txt`: `0x140a5d2e0`, `0x140a5f4f0`, `0x140a5cff0`, …). En cada llamada se lee `args[0].readPointer()` (= puntero al bloque de 0x7D4 bytes) y la dirección de retorno; cada 10–15 s se manda un resumen.
4. Fralex y el puesto 13 movieron la palanca a la vez durante ~30 s en un amistoso (fila 13 en un equipo).

**Resultado:**
- En el menú y en el partido, **el único bloque real leído es `0x7463bbb4`** (unas 20.000–40.000 lecturas cada 15 s, desde 26–79 sitios distintos). El resto de valores de `args[0]` son basura de lectores con otra firma.
- **Ningún otro bloque se lee nunca**, aunque el mando 13 se estaba moviendo (y su entrada sí está en su bloque, confirmado el 20-oct).
- Los lectores principales (`0x1408bb76f`, `0x1408ac9be`, `0x1408ac808`, `0x1408bb4a8`) reciben el bloque desde un **contexto**: `mov rcx,[ctx+0x8]` (`ctx` = 2.º argumento de su función). Hay 4 copias del acceso (direcciones de pila `…fbd0` de 4 hilos), todas apuntando al mismo bloque.

**Conclusión:** el freno no está en las ranuras ni en los ajustes, sino **en cuántos «contextos de mando» crea el partido**: solo crea el de los mandos de verdad (aquí, solo el del mando 1), y la lógica del jugador solo mira esos contextos. Los mandos 9+ tienen ranura, jugador y entrada, pero **no tienen contexto**, así que nadie lee su bloque.

**Siguiente paso para retomarlo:** buscar quién crea esos contextos (el objeto cuyo `+0x8` guarda el acceso al bloque, 2.º argumento de las funciones de `0x1408bb71a` y `0x1408ac96e`) y por qué solo para unos mandos (probablemente un recorrido de 8, o la lista de mandos activos de `HidMan`/`settings.dat`). Con eso se podría crear el contexto de cada mando 9+ desde el módulo. Herramienta: Frida vía REA (`Interceptor` sobre esas dos funciones, registrando `args[1]` y quién la llama).

### 23-oct-2026 ~05:20–05:47 — v0.9.16: ¡LOS MANDOS 9+ YA MUEVEN A SU JUGADOR! [CONFIRMADO EN VIVO]
**SÍ/NO primero: SÍ. Con la v0.9.16 el jugador del mando 13 se mueve (Fralex: «ya funcionó»). Queda abierto: algunos mandos se van solos a un lado, y falta el menú de configuración por mando.**

**La causa real (para no volver a buscarla):** `match::Command`, función **`FUN_1408A9180`**, recorre las 24 ranuras cada fotograma. Para cada una lee `mando = ranura+0x10` y:
- en `0x1408A94AC` hace `cmp dl,8` / `jnc` → **solo con mando < 8** arma la orden humana (bloque `mando*0x7D4`, configuración `FUN_140A647C0`, `CommandPlayer`+0x18);
- en `0x1408A99D0` hace `cmp dil,8` / `jnc` → **con mando ≥ 8 la ranura pasa a la CPU** (`CommandPlayer`+0x10 y `FUN_141FFB9D0`).
Los 24 objetos `match::CommandPlayer` (0x1840 bytes) existen siempre (fábrica `FUN_1408A9CD0`, lista global `0x1436F40B0`, y `match::Command+0x8620`); no hacía falta crearlos.

**Cómo se llegó (orden real, con los errores):**
1. Hipótesis «número de usuario local» (`ajustes+0x180`): falsa, probada en partido (21/23-oct) y antes del saque (v0.9.15).
2. Lectura en vivo de ranuras: diferencias en `+0x28` y `+0x34`; copiar los ajustes de cursor del mando 1 al 13: falso.
3. **REA (Frida), el paso que lo resolvió:** enganchar los 25 lectores del registro de entradas y ver qué bloques se leen → solo el del mando 1. Es decir, la entrada del 13 llegaba pero nadie la leía.
4. Ghidra: los lectores reciben el bloque desde un contexto con `+0x20` = ranura (`FUN_1408B97D0`); subiendo por tablas virtuales y constructores (`FUN_1408AD0B0` → `FUN_1408ACC00` → `FUN_1408B3D90` → `match::CommandPlayer` `FUN_1408B3BC0` → `FUN_1408A9CD0`) se llegó a `match::Command`. Buscar las instrucciones `CMP …,0x8` dentro de `FUN_1408A9180` (script nuevo **`scripts/ghidra/BuscarConstante.java`**, que encuentra constantes al final de la instrucción, cosa que `BuscarDesplazamiento` no hace) dio los dos topes.

**Parches de la v0.9.16 (en `PARCHES` del módulo; se comprueban los bytes originales del exe antes de escribir):**
| Dirección | Original | Nuevo | Por qué |
|---|---|---|---|
| `0x1408A94AC` | `80 fa 08` | `80 fa 10` | entrada humana hasta el mando 16 |
| `0x1408A99D0` | `40 80 ff 08` | `40 80 ff 10` | el mando ≥ 8 ya no pasa a la CPU |
| `0x140A647C0` (32 bytes, 18 + relleno `CC` hasta `0x140A647E0`) | `8bc2 488d04c0 488d80f45d0000 488d0441 c3` | `8bc2 83f808 7202 31c0 488d04c0 488d80f45d0000 488d0441 c3` | con N ≥ 8 leía configuración fuera de las 8; ahora usa la del mando 1 (como el parche de `FUN_140A64810`) |
- Probado fuera del juego: el código nuevo de `FUN_140A647C0` se ejecutó con datos falsos (0–7 igual que el original, 8+ = configuración del mando 1); `probar_ganchos.ps1`: todo bien salvo las 2 comprobaciones de la v0.9.15 (usuario local), que la v0.9.16 no incluye a propósito.
- La v0.9.16 parte de la v0.9.14 (no incluye lo de la v0.9.15, que no servía). Respaldo de la v0.9.14: `SiderAddons/modules/phoenix_mandos_fase1.v0914.lua`.

**Pendiente:** (1) algunos mandos se ponen solos en un lado; (2) menú de configuración por mando (el de «Datos personales»/cursor de PES) para los 9–16.

### 23-oct-2026 ~05:50–07:25 — Menús de antes del partido con mandos 9+ (v0.9.17 → v0.9.24)
**SÍ/NO primero: SÍ funcionan la estrategia y el mando del lado en «elegir equipo» (✕/○). NO funciona todavía subir/bajar en la lista de ligas/equipos con un mando 9+. La causa está localizada (gestor de foco de menús de 8) y el plan para resolverlo está abajo.**

**Lo que quedó hecho y probado en vivo:**
- v0.9.17: registro de qué mando mueve cada fila (`fila del mando N → lado L por su palanca (X… Y… cruceta …)`).
- v0.9.18/v0.9.21: lecturas vacías de mandos virtuales (`X 0000 Y 0000`) ya no cuentan; umbral de palanca 0x1000/0xF000; en el MENÚ cada pantalla de lados nueva empieza con los extra al centro (en la pausa del partido conservan su lado). Causa de «los mandos se ponen solos».
- v0.9.19: los lados de los extra se escriben en S al cerrar la pantalla de lados (antes solo en el saque) → **los mandos 9+ manejan la estrategia** (confirmado por Fralex).
- v0.9.20: 8 parches `cmp …,8 → 16` en los menús que buscan «el mando de cada lado» en la lista de lados (`0x140C9EFE7` TeamSelectMatchBase, `0x140ACB4A6` ExhibitionTeamDataLoad, `0x140A9E4F2`/`0x140A9E622` ExhibitionUserDataDoalog, `0x140AAC202/302/402/502` ExhibitionUserDataErrorDialog). El mando 16 recibe la pantalla de equipos del Visitante (✕ y ○ funcionan).
- Minicancha (resto de la prueba de cancha pequeña): césped original del Camp Nou devuelto y porterías de colisión movidas retiradas (copias en `copias/camp_nou_original/pitch_st002_instalado_antes_de_restaurar_20261023.fpk` y `copias/restos_cancha_pequena_retirados_20261023/`).

**Errores (para no repetirlos):**
- Una prueba con Frida escribió `+0x114 = 15` en un widget y la sesión se cerró antes de restaurarlo → PES se cerró al cambiar de pantalla. Regla: en pruebas en vivo, solo lectura; si se escribe, restaurar ANTES de cerrar la sesión de Frida.
- v0.9.22 agrandó el gestor de cursores de menú (`menusystem::PointingCursor`, singleton `[0x1436F9BC8]`, 8 entradas de 0x58; ctor `FUN_140E67580`, alta de 8 punteros gráficos «cmnPointer» en `FUN_140E67CB0`) a 16 → **PES no arrancó** (dos intentos). Desactivado en v0.9.23; copia `modulo_sider/phoenix_mandos_fase1.v0922_no_arranca.lua`.
- v0.9.24: en vez de agrandar, 13 funciones del PointingCursor usan `mando AND 7` (no se escribe fuera de la tabla). Arranca bien, pero **no era el freno** de subir/bajar.

**Causa real de que el mando 9+ no suba/baje en la lista (hallado con REA/Frida, solo lectura):**
- El manejador de arriba/abajo de la lista de equipos es `FUN_140C95E10` (lee los códigos 0x25/0x26 del mando de la lista). Solo actúa si la lista **tiene el foco**: `FUN_140E719B0(gestorFoco, lista)` busca la lista entre **8 huecos de foco, uno por mando** (`gestor+0xC8 + i·0x10`). Con el mando 15 la lista nunca entra en esos 8 huecos → la función sale antes de leer el mando (se ve en vivo: con el mando 1 pide el mando en `0x140C95E72`; con el 16 nunca).
- El gestor de foco (`FUN_140E71810` devuelve el objeto) tiene por mando: una pila de foco (`+0x08 + i·0x18`, vectores) y el foco actual (`+0xC8 + i·0x10`, `+0xD0 + i·0x10`), todo para `i < 8` (`FUN_140E71AE0` `cmp ebx,8` en `0x140E71B09`, `FUN_140E71BE0`, `FUN_140E718A0`).

**Plan para retomarlo (sin agrandar objetos, que ya falló):**
- Opción A (recomendada): el mando 9+ «toma prestado» en los menús un número de mando libre 0–7 (los menús solo necesitan un mando por lado): al cerrar la pantalla de lados, si un lado solo tiene extra, se escribe su lado en un hueco nativo libre de S y se enlaza en HidMan esa plaza lógica libre a su mando físico (el módulo ya sabe enlazar plazas: fila+8 / plaza+0xC). Al empezar el partido se deshace y el gancho del productor vuelve a poner el mando 9+. Así todos los menús (foco, cursores, equipos, datos personales) lo tratan como un mando nativo.
- Opción B: `mando AND 7` también en el gestor de foco (como en v0.9.24 con los cursores). Más parches y conflicto si el mando (n−8) está en el mismo menú.

### 23-oct-2026 ~07:25–07:50 — «Número prestado» en los menús: ¡el mando 9+ ya maneja los menús! (v0.9.25 → v0.9.28) [CONFIRMADO EN VIVO]
**SÍ/NO primero: SÍ. Con la v0.9.28 el mando 16 sube/baja y elige equipo (Fralex: «ya quedó»). Respaldo: `modulo_sider/RESPALDO_v0.9.28_menus_funcionan_20261023.lua`.**

**Idea:** los menús de PES (foco, cursores, listas) solo existen para los mandos 0–7, y agrandarlos rompe el arranque (v0.9.22). Pero los menús de antes del partido solo necesitan **un mando por equipo**. Así que, al cerrar la pantalla de lados (fuera del partido), si un lado solo tiene mandos 9+, el primero de ellos **toma prestado un número 0–7 sin lado** (se busca de 7 hacia abajo) hasta el saque.

**Cómo se hace (v0.9.28, funciones `prestarNumeros` / `devolverPrestados`):**
1. En la lista de objetos de mando de HidMan (`[0x1437F0A58]+0x48`, un puntero por mando; es lo que devuelve `FUN_141B52460` = vtable+0x18 «dame el mando N»), se **intercambian** los punteros `k` y `extra`. Así «el mando k» de los menús es el mando físico del extra.
2. En S (lista de lados `+0x2D`): lado del número prestado = lado del extra; lado del extra = 0.
3. En el saque, el gancho del productor escribe los lados 0–7 desde los datos (`d[8..15]` = S actual con el prestado en 0; `d[1] = 1`) y los 8–15 desde el plan → el extra entra al partido con su número 9+. En cuanto el módulo ve el partido en curso, deshace el intercambio. También se deshace al abrir una pantalla de lados nueva del menú.

**Errores en el camino (no repetir):**
- v0.9.25: usé `next()`; **Sider no tiene `next`** (`attempt to call global 'next'`) → el módulo fallaba cada cuadro y solo salía la pantalla de 8 de PES. Usar `for _ in pairs(t) do … end`. La vista previa con wasmoon (Lua 5.4) NO detecta esto: revisar siempre `sider.log` tras instalar.
- v0.9.26: «préstamo NO hecho (número libre nil)»: buscaba plazas sin mando enlazado, pero **Link deja enlazados los 8 mandos virtuales** aunque nadie los use. Hay que buscar números **sin lado**, no sin mando.
- v0.9.27: cambiar los enlaces fila↔plaza (`+0x08`/`+0x0C` de `HidMan+0xD0`) NO basta: el objeto del mando k seguía leyendo su mando de antes. Lo que cuenta es la lista de objetos `+0x48`.
- v0.9.27 también quitó lo de «los extra al centro en cada pantalla nueva» (v0.9.21): Fralex quiere que conserven su lado, como los mandos 1–8.

**Pendiente:** barra de potencia (pase/tiro) bajo el jugador de un mando 9+ en el partido; cambiar nombre / «Datos personales» por mando; capitán (△ y tecla C del anfitrión).

### 23-oct-2026 ~07:55 — v0.9.29: cada mando 9+ elige su «Datos personales» con L2/R2 [CONFIRMADO por Fralex]
- En la pantalla de lados (réplica Phoenix), un mando 9+ cambia de perfil con **LB/RB o los gatillos** (mando virtual tipo Xbox por DirectInput: botones 4/5 en `DIJOYSTATE+0x30`, gatillos en el eje Z `+0x08`). Solo al pulsar (flanco) y solo tras haberlo visto en el centro. La fila muestra el nombre del perfil.
- El perfil elegido (`PERFIL_DE[mando]`, por defecto el de su número) va a los datos del gancho `+0x38`; el gancho del productor lo copia en el saque a `S+0x180+8` («usuario local» = nº de perfil); el gancho de «crear pantalla de lados» lo vuelve a poner en `FF`. Mismo código de ensamblador que la v0.9.15 (que entonces no movía al jugador porque el freno real era otro).
- Nombres: se ponen en el menú principal de PES › Datos personales (32 perfiles).
- Respaldo: `modulo_sider/RESPALDO_v0.9.29_perfiles_20261023.lua`.

**Pendiente para la próxima sesión (en orden):**
1. Barra de potencia de pase/tiro bajo el jugador de un mando 9+ (la barra se calcula para las 24 ranuras en `FUN_1408AAA30`; el freno está en el dibujo, `match2D::Screen::ModelGauge`, o en la tabla 2D `+0x1B8`).
2. Capitán por equipo (△ el primero que lo pulse; tecla C del anfitrión), reutilizando el «número prestado».
3. Paquete independiente (sin Link) con README + paquete para que el hilo de Link lo instale desde «Módulos del juego».

### 23-oct-2026 ~16:30–17:00 — Capitán por equipo y paquete independiente (v0.9.30 → v0.9.31) [CAPITÁN CONFIRMADO con F5/F6]
- **Capitán:** `PHX.CAPITAN[1|2]` = mando que maneja los menús de ese equipo. Lo elige el anfitrión con **F5** (Local) / **F6** (Visitante) en el teclado (va pasando por los del lado), o un jugador con **L3** (botón 8 de `DIJOYSTATE+0x30`; el primero que lo pulsa; L3 otra vez lo deja). △ no se usa porque en esa pantalla de PES es «Modo Entrenador»; R3 (v0.9.30) se cambió a L3 a pedido de Fralex. Al cerrar la pantalla de lados, los demás mandos 1–8 de ese lado se **ocultan** en S (`PHX.OCULTOS`) hasta el saque (el gancho del productor los repone con `d[8+i]`); si el capitán es 9+, toma número prestado. La fila del capitán muestra «CAP».
- **Límite de Lua (otra vez):** al añadir tablas se pasó de 200 variables locales; se agruparon en `local PHX = { OCULTOS, CAPITAN, PERFIL_DE, PRESTADOS }`.
- **Paquete independiente** (regla de Fralex: cada mod instalable sin Phoenix Link): `PRUEBA_CANCHA\paquetes\phoenix_mandos_9a16\` + zip. Contiene el módulo, `content\phoenix\lados_replica\` (5 archivos, piezas de la pantalla), `livecpk\PhoenixMandos\…\MatchPrePause.json`, `LEEME.md` (qué hace, requisitos, instalar, desinstalar, límites) e `INSTALAR.ps1` (copia y ajusta `sider.ini` con copia de seguridad; **no se pudo ejecutar en esta sesión por permisos, revisado a mano**). Pasado al coordinador para que el hilo de Link lo integre en «Módulos del juego».
- Respaldo: `modulo_sider/RESPALDO_v0.9.31_capitan_20261023.lua`.
- **Pendiente:** barra de potencia de pase/tiro para los mandos 9+ (necesita partido en vivo).

### 23-oct-2026 ~22:15 — v0.9.38: ○ de los mandos 9+ en la pantalla de lados [CONFIRMADO en Ghidra; falta en vivo]
**SÍ/NO primero: hecho en código, sin probar en PES. ○ de un mando 9–16 en «Local/Visitante» (pausa y menú) llama al mismo «atrás» que usa PES con los mandos 1–8. Sin teclado.**

**Causa (Ghidra solo lectura, `informes/datos/rea/atras_01 … atras_11`; lanzadores `ss1.bat … ss6.bat` en el scratchpad):**
- Cada ventana de menú lee **su mando dueño** (`+0x88 → +0x114`) en `FUN_140E3C810`; con el código 1 (○) pone el sonido 2 (`SoundManager [0x1436F9A88] + 0x9E + 2 = 1`) y llama a `vtable+0x190`.
- En `menu::SideSelect` (vtable `0x142673558`, objeto de 0x118) solo las **8 filas** `SideSelectEach` (`+0x90…+0xC8`, vtable `0x142715608`, dueño = mando 0–7) tienen ese método: **`FUN_140C68350`** (está en `0x142715798` = vtable de fila + 0x190). Este comprueba `FUN_140ABFA40(padre)` y manda al padre (`fila+0x38`) el `sys::TaskMessage` **`0x1081002`** con `FUN_1414A0610` (con cerrojo del gestor de tareas). El padre lo atiende en **`FUN_140AC0700`** (vtable +0x110): en el menú, si cambiaste lados el primer ○ los deshace; si no (o en la pausa) `FUN_140ABFD80` + `FUN_140E42E60(…,"return")`.
- Las filas son 8 fijas dentro del objeto: **no hay un tope que subir** (opción «parchear el 8» descartada). El mando 9+ no tiene fila → nadie oye su ○. (`0x1081001` = confirmar, `FUN_140C683B0`, sí compara el mando con el de la fila; `0x1081003` = `FUN_140C68470`, Datos personales.)

**Arreglo (v0.9.38, `V9.atrasExtras` en `display_frame`, solo con `activo` y mandos extra):** flanco del botón 2 (`V9.boton(idx, 2)`, solo si antes se le vio suelto) de un mando 9+ con la pantalla de PES abierta (contadores `datos+0x24 ≠ +0x28`) → sonido 2 y `FUN_140C68350(fila)` por FFI sobre una fila del objeto guardado en `datos+0x30` (prefiere una fila con mando conectado, `fila+0x90 = 1`). Comprobaciones antes de llamar: vtables del objeto (`+0` y `+0x70 = 0x1426737B0`), fila con vtable de fila y `fila+0x38 = objeto`, `[0x142715798] = 0x140C68350`, y los 93 bytes de `FUN_140C68350` iguales a los de `copias/exe_pes2021/PES2021.exe` (verificado: coinciden con el exe real y el recortado). Como mucho un «atrás» cada 15 cuadros; si el mando 9+ es el mismo dispositivo que un 1–8, no se llama (PES ya lo oye). Si algo falla solo se anota en `fase1.txt`. Respaldo: `modulo_sider/phoenix_mandos_fase1.v0937.lua`.

**Verificado fuera del juego:** `sacar_codigos_ganchos.js` sin ERROR y `probar_ganchos.ps1` → TODO BIEN (los ganchos no cambian).
**Riesgos:** (1) se ejecuta lógica de menú desde `display_frame`; si no fuera el hilo del menú podría chocar con el cuadro de PES (la migración del foco v0.9.36 ya hace cosas parecidas desde ahí sin problemas). (2) No se copia el texto «return» a la cadena `+0x3B0` de la fila (lo hace `FUN_140E3C810` antes de `+0x190`); el padre usa su propio «return», así que debería ser solo cosmético. (3) Botón 2 = ○ supone el orden de botones de los mandos virtuales de Link (el mismo que usaba la v0.9.34).
**Prueba en vivo:** pausa › Local/Visitante con un mando 9+ → ○ → debe volver a la pausa y el registro decir `mando N pulsa ○ → «atrás» en la pantalla de lados de PES`. Repetir en el menú (con y sin cambiar lados: con cambios, el primer ○ los deshace, igual que con 1–8).

### v0.9.39 → v0.9.58 (23 al 25-oct): los mandos 9–16 «indistinguibles» de los 1–8

**Resultado (confirmado por Fralex 25-oct ~00:08):** con un mando 9–16 funcionan pausa › Local/Visitante (○ vuelve), Estrategia en la pausa, L2/R2 para elegir usuario (sin repetidos), □ para «Editar Datos personales» de SU usuario (también en la pausa), nombres iguales en la pantalla simulada y en la real, el usuario elegido llega al partido y, sobre el jugador, el «Tipo de cursor» (Datos personales / Nombre de jugador / Mando n.º / No) y la barra de potencia, también en los mandos 13–16. Paquete independiente: `paquetes\phoenix_mandos_9a16_v0.9.58.zip`.

**Errores y lo que se aprendió, en orden:**
1. **○ no volvía (v0.9.39→0.9.40).** El registro no tenía ni un «○»: el mando 16 manda ○ como **botón 1** de DirectInput (distribución tipo Xbox: ✕0 ○1 □2 △3 L1 4 R1 5 … L3 8; gatillos por **eje Z**). Medido con `scratchpad\botones16.ps1` mientras Fralex pulsaba. L2/R2 se leen ahora por botón 6/7 o por eje Z (> 0xD000 / < 0x3000); y se copiaban solo 0x24 bytes del estado DI (los botones están en +0x30): se leía fuera.
2. **Estrategia en la pausa (v0.9.41).** En la 1.ª pausa no dejaba, en la 2.ª sí: el lado elegido en pausa solo llegaba a S al reanudar (gancho del productor). Arreglo: al cerrar Local/Visitante en pausa se escriben los 9+ en S en el momento (sin préstamo). El préstamo de la pausa se devuelve también al ver ranuras jugando («reanudar»).
3. **«Usuario N» en todos (v0.9.42→0.9.43).** La lista de «Datos personales» (32 × 0x980, nombre en +0x14 del registro) sigue existiendo, pero su bloque está partido en regiones de 0x4000 (la 1.ª protección 0x04, otras 0x40) y la búsqueda exigía 0x14000 en la primera. Ahora basta 0x1000 + cabecera 0x4FFE0 + validación de los 32.
4. **Campos de la fila SideSelectEach cruzados (v0.9.47, v0.9.54).** `fila+0x98` = **perfil** (lo escribe FUN_140C68410/68440 con L2/R2, y lo lee □); el **mando** de la fila es su dueño `[fila+0x88]+0x114` (FUN_140E3D4D0). `+0x9C` NO es el mando: al volver de editar valía 10 y la Phoenix descartaba la fila («se pega el mando 1»). Encontrado con diferencias de memoria (foto de 2,2 GB, CAMILO 10 → BETO 11 → 10 → 11: quedan 3 direcciones, una en la fila).
5. **Repetidos (v0.9.48):** L2/R2 salta los perfiles que ya usa otro mando (nativos por su fila, extras por lo elegido), como PES.
6. **Pantalla «abierta» tras editar (v0.9.53/55).** PES crea la pantalla nueva ANTES de destruir la vieja (FUN_140ABF000 = destructor de menu::SideSelect): aperturas = cierres aunque esté abierta. El gancho de destruir guarda el objeto destruido en datos+**0x248** y «abierta» = contadores distintos o (creado ≠ destruido). ERROR propio: en la v0.9.53 lo guardaba en datos+0x38, que son los **perfiles 8–15** del productor (los pisaba). Otros dos errores propios: pasar de 200 locales (Sider salta el módulo: «main function has more than 200 local variables») y un corte de código mal hecho (`'<eof>' expected near 'end'`). Siempre mirar `sider.log` tras instalar.
7. **□ de un mando 9+ (v0.9.52/55/56).** PES ni recibe su □. Ghidra: □ de la fila = FUN_140C68470 → `DAT_1434AC580 = perfil`, `FUN_1414BF2D0(FUN_1414BEA50(), mando)` (apila el cursor del editor para ese mando), TaskMessage **0x1081003** al padre (FUN_1414A0610) y sonido FUN_140E41880(fila, 0x3D). El módulo lo replica con el perfil y el número del 9+ (comprobando antes 36 bytes de FUN_140C68470). El cierre que provoca NO es «confirmar»: si se tomaba como tal, el préstamo cambiaba el mando del editor y el ○ dejaba de llegar.
8. **Usuario en el partido (v0.9.57).** `S+0x45` no es solo «mando»: es el **usuario** de la ranura (al cambiar el usuario del teclado pasó a 0x08/0x0D). Con `i` el extra salía con el usuario de su número; ahora va el elegido (sigue moviéndose: confirmado).
9. **HUD sobre el jugador para 13–16 (v0.9.58).** match2D::Screen::ModelCursorName (vftable 0x142BDE5A8) y ModelGauge (0x142BDEF90) tienen tablas de **12** modelos (FUN_1420D5BF0: ranura ≥ 12 → −1). Mandos 9–12 ya salían. Parche «tabla espejo» (informe completo: `informes\datos\rea\hud16_informe.md`): reservas 0x118→0x198 y 0x168→0x1E8, tabla CN de 16 en +0x118, MG T1 crece en su sitio y T2 pasa a +0x168, ids nuevos 0xD0–0xDB (FUN_141F9E610 aprende su tipo), 7 ganchos a cuevas propias y topes 0xC→0x10. Cueva F: los usuarios sin nombre reciben la textura «Usuario N». Se aplica solo en `m.init` (sin partido) y comprobando todos los bytes originales.

**Pendiente:** pantalla Local/Visitante nativa de 16 filas (menu::SideSelect de 0x118 con 8 filas fijas) — Fralex: «mañana».
