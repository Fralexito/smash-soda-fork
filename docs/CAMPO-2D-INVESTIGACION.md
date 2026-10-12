# Campo 2D en vivo — investigación de posiciones en PES 2021

> Estado (11 oct 2026): **las posiciones de los jugadores SÍ son legibles** desde fuera del juego (solo lectura).
> Falta cerrar: de qué equipo es cada fichita y dónde está el balón. Todo lo de aquí es **ingeniería inversa propia**
> sobre el `PES2021.exe` de ConmeGOL; no hay nada publicado por la comunidad (ver «Fuentes»).

## 1. Fuentes buscadas (y qué dieron)

| Fuente | Resultado |
|---|---|
| Búsquedas web «PES 2021 player position / ball position address Cheat Engine» | Nada con direcciones de posiciones en campo. Solo tablas de edición de jugadores/ML. |
| [PES-2021-Cheat-Table de xAranaktu](https://github.com/xAranaktu/PES-2021-Cheat-Table) y el *Live Editor* de Ruls | Editores de datos de jugadores (habilidades, contratos). No tocan el partido. |
| Módulos de Sider instalados (`camera.lua`, `*Cam.lua`, `etrace.lua`) | Usan `memory.search_process` para parchear código de cámaras; no leen posiciones. |
| Sider 7.3.x (`match.stats()`, eventos) | Marcador, minuto, goles: **no entrega posiciones**. |

Conclusión: había que encontrarlas. No se copió código ajeno.

## 2. Método (todo en `tools/radar/`, ninguna pieza forma parte de Link)

1. `escaner.cpp` — búsqueda tipo Cheat Engine sobre `PES2021.exe` con `ReadProcessMemory` (solo `PROCESS_VM_READ`;
   para el modo por pasos también `PROCESS_SUSPEND_RESUME`, que es lo mismo que pausar en un depurador).
   - Floats «con pinta de coordenada» (0.02 ≤ |v| ≤ 6000).
   - **Modo por pasos**: el juego avanza ~50 ms, se congela, se leen TODOS los candidatos con el juego quieto. Las posiciones
     de un jugador cambian poco entre pasos; casi todo el ruido salta o no cambia. Pasaron de 50 M a ~380 mil «suaves».
   - **Arrays de punteros**: busca tablas de ≥20 punteros cuyos objetos tengan, todos al mismo desplazamiento, un candidato
     suave. Esto fue lo que dio con la estructura (el análisis por «paso constante» no sirvió: los objetos están en un pool con
     paso 0x280 pero repartidos en muchos bloques).
2. `sonda.cpp` / `objeto.cpp` — lectura en vivo de un array de punteros y volcado de los objetos.
3. `entes.cpp` — **escáner por firma** del objeto jugador (ver §3). Tarda ~5 s sobre 5 GB; las direcciones son estables
   mientras dura el partido (comprobado: 48/48 iguales entre el minuto 1 y el 3).
4. `buscaptr.cpp`, `analiza.cpp`, `bola.cpp` — auxiliares (quién apunta a una dirección, análisis offline, búsqueda del balón).

Las pausas del juego durante la investigación se hicieron con `NtSuspendProcess`/`NtResumeProcess` (siempre con `finally`
para reanudar). **Link NO hará eso**: la lectura final no congela nada.

## 3. Lo encontrado

### 3.1 Objeto «actor de jugador» (0x280 = 640 bytes)

Posiciones dentro del objeto (offsets en bytes desde el inicio del objeto):

| Offset | Contenido | Notas |
|---|---|---|
| +208 | `float4 (x, y, z, 1.0)` | **Posición.** x = largo del campo, z = ancho, y ≈ 0.9 (altura de la cadera), w = 1.0 exacto. |
| +112 | otra copia de la posición (idéntica a +208 en reposo) | |
| +248 | `u32` tipo: `0x18` = jugador físico en el campo | `0x8` = «cerebro» de IA (y = 0), `0x14`, `0xF`, `0x26`: otros |
| +312 | `u32` 1 o 2 | en las pruebas partía los 22 en 11/11 pero **no es el equipo** (ver §4) |
| +440 | `u32` = `(id de equipo << 16) \| 3` | 0x2C0003 = 44 (Barcelona), 0x430003 = 67 (Man City) |
| +492 | `u32` = `0x80000 \| 1..2` en jugadores en campo | parte de la firma |

- Unidades: **metros**, centro del campo = (0,0), x ∈ [−52.5, 52.5], z ∈ [−34, 34] (las posiciones de los jugadores en
  pantalla coinciden con el dibujo en planta; velocidades de 2–3 m/s al trotar).
- Hay **dos listas** (una por «equipo de IA»): cada lista tiene 23 actores con las posiciones de **todos** los jugadores
  (22 + árbitro). La lista con código 44 y la lista con código 67 dan las mismas posiciones: por eso salían «parejas» a
  0.3–0.8 m. Para dibujar basta **una** lista.
- Los objetos están en el heap (direcciones distintas en cada partido y en cada arranque): hay que **redescubrirlos con la
  firma** al empezar el partido; las direcciones valen mientras dura el partido.
- Verificación hecha: congelando el juego, captura de pantalla y volcado de memoria en el mismo instante; el dibujo en
  planta de las 23 posiciones coincide con la disposición de la jugada (grupo de jugadores junto al área, árbitro en la banda).

### 3.2 Firma de búsqueda (la que usa `entes.cpp`)

```
b = dirección alineada a 16
u32(b+248) == 0x18
u32(b+312) en {1,2}  y  u32(b+492) == 0x80000 | u32(b+312)
float4(b+208) = (x,y,z,w): w == 1.0, 0 < y < 3, |x| < 80, |z| < 60
```

## 4. Qué falta (y cómo se va a cerrar)

1. **Equipo de cada fichita.** El campo +312 NO es el equipo (en pruebas con 22 objetos daba 11/11, pero con la lista completa
   no). Plan: en el saque de centro (juego congelado) el equipo local ocupa un lado; fijar la pertenencia por *posición de
   formación al saque* y mantenerla por la identidad del objeto (las direcciones no cambian en el partido). El lado de cada
   equipo en cada tiempo sale del reloj (`match.stats`) y de quién saca.
2. **Balón.** Aún no identificado. Plan: con el balón quieto en el centro (saque inicial) buscar el triple (≈0, 0.11, ≈0) y
   comprobar qué direcciones se mueven a >5 m/s justo después (`bola.cpp`).
3. **Ruta estable.** Los punteros que llegan al array (pool genérico en 0x46xxxxxx) no cuelgan de una dirección fija del
   ejecutable; por eso se prefiere la firma.

## 5. Protección (regla de Fralex: «si algo se rompe, mejor no lo hagas»)

- Solo lectura. Sin escribir, sin hooks, sin inyectar.
- La lectura se activa solo si: el proceso es `PES2021.exe`, su **SHA-256** coincide con una lista conocida
  (hoy: `5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A`, 458 806 784 bytes — es el mismo exe del
  parche ConmeGOL y del SP2026), y la firma devuelve 22 jugadores plausibles (dentro del campo, finitos).
- Si algo no cuadra (otro ejecutable, 0 jugadores, valores fuera del campo, NaN), la función se apaga sola y el juego sigue normal.
- Coste: 1 escaneo de ~5 s al empezar el partido y después ≈ 20 lecturas/s de ~1 KB (despreciable).

## 6. Datos útiles para el futuro (extra)

- `PES2021.exe`: base de carga `0x140000000`, tamaño de imagen 462 999 552 bytes. Regiones RW del proceso en partido: ~3 200 MB.
- IDs de equipo en memoria = IDs PES (Barcelona 44, Man City 67 en ConmeGOL).
- El proceso **se pausa/minimiza solo** si pierde el foco (no corre con la ventana minimizada): hace falta que esté al frente.
- Teclado: en el menú funcionan Enter/flechas; en el campo el teclado no controla al equipo (solo el mando).
- Sider overlay: la barra espaciadora lo muestra/oculta.

## 7. Hallazgos del 11 oct por la noche (equipo y balón: aún sin cerrar)

Comprobado con un saque de centro congelado (captura + memoria en el mismo instante, 0:00, Barça–City):

- Cada lista de «equipo de IA» (códigos 44 y 67) trae **23 actores = 20 jugadores de campo + el árbitro + 2 asistentes**
  (los asistentes van pegados a la banda, z = ±35.4). Los **porteros NO están en esas listas**: son actores aparte
  (código distinto, `+312 = 1`, `+492 = 0x80001`, uno por portero; en el saque salió el del Barça en x = −47).
  Para tener los 22 hay que sumar los porteros.
- Posiciones verificadas contra la pantalla: el que saca (cursor «FRALEX») está en (0.6, 0.0); los demás caen donde se ven.
  En el saque inicial el local ocupa x < 0 (excepto quien saca).
- **El equipo NO está en el actor.** Probado: +312 (es la «lista», no el equipo), +228, +440 (lista), punteros +16/+24
  (enlaces entre actores), tabla de registros de +416 y contexto de +400. Ningún campo parte los 21 en dos equipos.
  Siguen abiertas: (a) fijar el equipo con el saque de centro (por lado y pidiendo al que saca) y mantenerlo por
  identidad de objeto, (b) seguir buscando dónde guarda PES la plantilla en pantalla.
- **Balón: candidatos, sin confirmar.** Hay actores «tipo 0» (`kind=0`, código 5/8/1045) con y ≈ 0.10–0.11 (el radio del
  balón) y varias copias idénticas; ejemplo (−20.0, 0.11, 6.1) en cuatro direcciones. Falta verlos moverse con el juego
  en marcha (cuando se probó, el juego estaba pausado por minimizado).
- Los actores de un partido anterior pueden quedar en memoria con códigos viejos; el lector se queda con la lista que tiene 18–24 miembros.
- Mecánica útil: tecla **Z** = «Atrás» y **X** = «Confirmar» en los menús del juego; Enter/flechas también.
  Con X se saca de centro. El juego **se pausa solo si se minimiza** y a veces el Explorador pasa al frente.
- Penales: en Opciones gener. (antes del partido) «Pen.» y «Prórroga» están en «Sí», así que con un 0–0 y 5 min se llega a una tanda
  (el balón queda quieto en el punto penal = referencia perfecta para el balón); no se llegó a probar.
- Herramientas nuevas en `tools/radar/`: `vigilante.cpp` (congela el juego al detectar un saque), `entes3.cpp` (firma relajada: porteros, árbitro, balón).

## 8. 11 oct por la noche (segunda tanda): balón, porteros, equipo y TANDA DE PENALES — cerrado

Partido Barça–City en ventana (0:00 → 1-1 → tanda de penales), con captura congelada del saque y 3 minutos de grabación
de la tanda a 20 lecturas/s (`tools/radar/sondaL.cpp`, capturas cada 4 s).

- **Balón del partido = un actor fijo todo el partido**: `+248 = 0`, `+312 = 1`, `+492 = 0x10001`, posición en +208 con
  `y ≈ 0.11` (radio) y hasta ~1 m al golpearlo. Se **aparca en z = 100** durante las repeticiones y vuelve a (0, 0.11, 0)
  en el saque. Hay otros balones (`code` 6 y 9, de los recogepelotas) con la misma forma: el del partido es **el que se mueve**.
- **Porteros**: actores aparte de las listas (`+248 = 0x18`, `+312 = 1`, `+492 = 0x80001` o `0x10001`), dos copias por
  portero; en el saque están en x = ±47.
- **Equipo**: no está en el actor; se reparte en el saque de centro (izquierda → A, derecha → B; los del círculo central
  → el equipo que saca = el que tiene menos jugadores en su mitad) y se recuerda por dirección. Comprobado con la foto del
  saque: los rivales se pegan a la línea de medio campo (x ≈ 1–2 m), por eso «centro» es solo el círculo del balón (< 2.5 m).
- **Árbitro**: en el saque se coloca clavado en (±4.8, ±10.0) (dos partidos, misma décima). Un rival puede quedar cerca
  ((4.0, −9.3)): tolerancia de 0.7 m en total y, si hay varios, el más exacto.
- **Tras el pitido / en la tanda** `+312` de los jugadores pasa de 2 a **0** (y el de los porteros de 1 a 0): la firma lo admite.
  Los que esperan se colocan en el círculo central o **fuera de la banda (z ≈ 40–45)**: el límite de posición válida sube a |z| ≤ 50.
- **Tanda de penales** (4 lanzamientos grabados, todos gol): el balón se queda en el punto penal (−41.6, 0.11, 0.0); el pateador
  es un actor normal que camina desde el círculo (su identidad se mantiene → su equipo); el portero que ataja es el que está
  en la línea (|x| > 50, |z| < 4) y los dos se alternan; al golpear, el balón cruza x ≤ −52.5 con |z| < 3.66 y y < 2.44 (gol)
  y vuelve al punto para el siguiente. En Link: `core/CampoVivo.h` clase `Tanda` (probada con un penal real en `campo_test`).
- **Nombres de los lanzadores**: aún no (no hay nombre ni dorsal reconocible en el actor; la tabla de registros de +416 no
  lo separa). Pendiente.

## 9. 11 oct (noche tardía): identidad de cada jugador y estadísticas — lo que se probó y las pistas abiertas

**Objetivo pedido**: al hacer clic en una fichita, ver su dorsal y nombre; y contar disparos a puerta, córners, faltas, tarjetas, cambios (quién entra/sale), asistencias, etc.

Lo que NO funcionó (para no repetirlo):
- **Campos del actor**: ninguno de los 640 bytes distingue equipo ni jugador (probado con 3 saques/2 listas, con partición por lado conocida). Los punteros `+16/+24` son enlaces entre actores (marcaje/vecinos), no identidad.
- **Quién apunta al actor** (`tools/radar/identidad.cpp`): ~500 punteros por 42 actores; las ventanas de ±0x100 bytes alrededor traen tablas de IA con enteros pequeños (formación/zonas), ningún id de jugador distinto por actor.
- **Registros de jugador con nombre** (`buscatexto.exe`): hay estructuras con **dorsal en +0x8C, nombre en +0x90** y el id de jugador en +0x08/+0x30 (p. ej. LAMINE YAMAL, dorsal 99, id 162114 = el `player_id` que da Sider en `goal_scored`), pero están en pools estáticos de la pantalla de alineación/intro y **no tienen puntero al actor** ni cerca de él. También hay una caché de textos de nombres (64 bytes por registro) usada para dibujar.
- **Estadísticas del equipo** (las del menú de pausa): buscadas por valores exactos (posesión, tiros, pases…) como u8/u16/u32 (`buscaserie.exe`) sin éxito: el juego no las guarda como enteros planos contiguos (o las calcula al mostrarlas).

Pistas abiertas (en orden de probabilidad):
1. **Sider `goal_scored`** entrega `player_id`, `player_offset` y un puntero `player_info` (visto en las cadenas de `sider.dll`): `player_offset` parece el índice del jugador dentro de su equipo. Un módulo Lua que registre ese evento y escriba `player_offset` junto al gol dejaría ver el hueco de la plantilla; con la plantilla (Phoenix-DB / `PlayerAssignment`) saldría el nombre. No sirve para los 22 en general.
2. **Eventos por nombre de escena**: ConmeGOL ya intercepta `livecpk_get_filepath` para escenas de falta (`common\demo\fixdemo\foul\cut_data\foul_pk_card_y01.fdc` = amarilla, `..._r01` = roja, `foul_beJudged` = falta…). Un módulo observador que **solo registre** los nombres de los `.fdc` que el juego pide permitiría contar faltas, tarjetas, lesiones y cambios por los nombres de escena (hay que listarlos en un partido real). Requiere activar un módulo en `sider.ini` (lo hace el instalador de módulos de Link, con PES cerrado).
3. **Identidad por geometría en el saque**: emparejar cada actor con el jugador de la alineación por su posición base de formación (la alineación se ve en la pantalla de «Estrategia»); necesita la formación del equipo y el orden de la plantilla.
4. **Estadísticas derivadas de lo que ya se lee** (posiciones + balón): posesión por cercanía, distancia recorrida, velocidad máxima, tiros por velocidad/dirección del balón, saques de banda/esquina por salida del balón. Se pueden construir y validar contra la pantalla de estadísticas finales (ya hay un partido de referencia: Barça 7-0 City, posesión 56/44, tiros 12(8)/0(0), córners 1/0, faltas 2/0).
