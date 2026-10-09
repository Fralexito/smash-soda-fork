# Phoenix Mercado · Diario de pruebas (prueba y error)

**Para qué sirve:** dejar anotado cada intento: qué se probó, qué se vio en el juego, qué falló, por qué y cómo se corrigió.
Así se puede ver el avance y, cuando todo esté probado, convertirlo en el **PDF final**.

**Reglas:** solo se **añade** (no se borra lo que salió mal: es parte del aprendizaje). Lo escriben las dos cuentas de Claude.
Detalle técnico completo: `liga-master/ESTRUCTURA-ML.md` (estructura del archivo) y `REGISTRO.md` (bitácora de cambios).

**Cómo leerlo:** ✅ salió bien · ❌ hubo un error · ⏳ pendiente · 🔎 encontrado revisando los archivos (sin verlo aún en el juego)

**Ranuras:** en el menú *Cargar* de la Liga Máster, la ranura N es el archivo `ML0000000(N−1)`. Ejemplo: ranura 9 = `ML00000008`.

---

## Resumen (al 8 oct 2026, 13:40)

- **Pruebas mirando el juego:** 13 · ✅ 10 · ❌ 3 (las 3 ya corregidas y vueltas a probar)
- **Otros errores:** subida del catálogo a la web ❌ (corregido; ⏳ falta volver a ejecutar) · alineación del option file en
  amistosos 🔎 (⏳ por corregir) · venta bloqueada después de jugar un partido 🔎 (✅ corregido) · contratos dobles 🔎 (⏳ prueba pedida)
- **Pruebas automáticas:** 117 de 117 bien (en Linux) · compilación en Windows ⏳

---

## Parte 1 · Option file (`EDIT00000000`: lo que usan los amistosos y las salas)

**7 oct · Leer y volver a guardar el option file** — ✅
Se descifra y se vuelve a cifrar, y queda **idéntico** al original. 740 equipos leídos.

**7–8 oct · Mover a Lautaro Blanco (Boca → Sporting) en una copia** — ✅ en lo que se miró
Se vio en el juego en el Sporting **con su cara**.
> 🔎 **8 oct (13:00), revisando los archivos:** la **alineación de Boca se corrió un puesto**. En un amistoso, Boca habría
> salido con el portero **Marchesín de delantero** y cada jugador, desde el lateral izquierdo, un puesto más atrás; además el
> **capitán habría cambiado**. El Sporting estaba bien. Causa: cada equipo del option file también tiene una **lista de
> alineación** (orden de titulares + roles) y el programa no la tocaba. Es el mismo tipo de error que el de la Liga Máster
> (ver ❌ de la ranura 8). ⏳ Se corrige antes de seguir y se prueba en un amistoso.

**8 oct · Modo seguro** — ✅
Si el option file y la base de datos son de parches distintos, el programa se pone en modo seguro y no toca nada.

**8 oct · 49 pruebas en la PC de Fralex** — ✅ 49 de 49
Conexión con la web, cuenta, cifrado, catálogo, emparejamiento, copias de seguridad.

**8 oct (09:13) · Subir el catálogo a la web (`SUBIR.bat`)** — ❌
La web rechazó el primer lote: había un jugador **sin nombre**. Se corrigió el catálogo (commit `666a2b3`).
⏳ Falta volver a ejecutar `SUBIR.bat`.

---

## Parte 2 · Liga Máster interna (tu carrera guardada)

**Ranura 1 · v2 · Haaland City → Santos tocando solo la lista del club** — ❌
En la búsqueda Haaland quedaba **sin equipo**, pero seguía en la Estrategia y en la zona de ventas del City.
Aprendido: tu equipo guarda **más listas por jugador** (tablas, alineación) que hay que tocar todas a la vez.

**Ranura 2 · Copia limpia de control** — ✅
Sirve para comparar: Haaland en el City, todo normal.

**Ranura 3 · Mbappé Madrid → Santos (dos equipos de la IA, solo la lista del club)** — ✅ en lo que se miró
Mbappé aparecía bien en el Santos.
> 🔎 Después se supo (ranura 8) que la **alineación** del Madrid y del Santos tampoco se había tocado. Esa pantalla no se miró.

**Ranura 4 · Haaland City → Santos, solo listas** — ✅ a medias
Aparecía en el Santos; el lado del City no se revisó.

**Ranura 5 · v5 · Haaland City → Santos completo (listas + tablas + lista K)** — ❌
Haaland perfecto en el Santos, pero en la **Estrategia del City aparecía Rulli (portero) en el ataque**.
Causa: la alineación y los 6 roles (capitán, lanzadores) son **números de puesto en la plantilla**; al sacar a Haaland
esos números se corrían un lugar.

**Ranura 6 · v6 · Igual, recalculando alineación y roles (Neymar en lugar de Haaland)** — ✅
Estrategia bien (Neymar delantero, ningún portero arriba), City con 25, Haaland en el Santos.
Después Fralex **jugó un partido y guardó en la ranura 7** — ✅ el juego no corrigió nada por su cuenta.
> Nota: al guardar, el juego **sobrescribió** el archivo de la ranura 6. Lección: anotar la huella (md5) de cada archivo
> que se entrega, para saber después si es el nuestro o uno guardado por el juego.

**Ranura 8 · v7 · Primera prueba del programa en C++: Guéhi City → Real Madrid (Stones titular)** — ❌
✅ Pantalla Equipos (Madrid 31 con Guéhi, City 24), búsqueda y Estrategia del City.
❌ En la **Alineación del Madrid** salía un **jugador en blanco «DC 0»** en lugar de Guéhi.
Causa: **cada equipo de la IA tiene su propia lista de alineación** (bloque de 600 B) y no se actualizaba.

**Ranura 9 · v8 · Arreglo de la alineación de la IA + Isak Liverpool → Santos** — ✅
- Madrid: Guéhi al final de la lista, con el 30, sin jugador en blanco.
- Liverpool: **Ekitiké** (elegido solo, por posición) juega en el puesto de Isak.
- Santos: Isak al final de la lista, con el 99.
- City: sin cambios, Stones titular.
> Hallazgo: en esta partida la **banca es de 12**, no de 7. El programa ya no supone un tamaño fijo.
> ✅ 8 oct 13:07: Fralex jugó un partido y guardó en la **ranura 10**: todo se mantuvo (ver abajo).

---

## Parte 3 · Pruebas automáticas (las corre la computadora sola)

| Cuándo | Resultado | Qué cubren |
|---|---|---|
| 8 oct, mañana | 41 de 41 ✅ | Option file, web, copias, catálogo |
| 8 oct 11:35 | 41 de 41 ✅ | Lo mismo + el módulo de Liga Máster compilando |
| 8 oct 11:55 | 88 de 88 ✅ | Venta de tu equipo a la IA (datos de prueba) |
| 8 oct 12:36 | 112 de 112 ✅ | Alineaciones de la IA, traspasos IA ↔ IA, sustitutos |
| 8 oct 13:00 | 117 de 117 ✅ | Sustitutos sin suponer el tamaño de la banca |
| 8 oct 13:35 | 119 de 119 ✅ | La venta ignora la ficha del último partido |
| 8 oct 21:30 | 211 de 211 ✅ (sin archivos) · **295 de 295 ✅** (con option file, base, catálogo y guardado real) | Fichaje para el usuario **desde la sincronización con la web**: da **exactamente los mismos bytes** que el fichaje validado en el juego (prueba 19); condiciones opcionales de la web (monto, sueldo, cláusula, fin de contrato, dorsal) |
| — | ⏳ | Compilar en Windows (`COMPILAR_MERCADO.bat`) |

Además, en archivos reales: el programa en C++ da **exactamente el mismo archivo** que el prototipo que el juego aceptó
(ranura 6) y vuelve a generar la ranura 9 idéntica (misma huella).

---

## Parte 4 · Lecciones aprendidas

1. Mover a un jugador **solo en la lista del club no basta**: hay otras listas que dependen de ella.
2. La alineación y los roles son **números de puesto**: si alguien se va, hay que reacomodarlos.
3. **Los equipos de la IA también tienen alineación**, y en el option file también (amistosos).
4. No suponer tamaños fijos: la **banca** puede ser de 7 o de 12.
5. Cada prueba se hace en un **archivo nuevo** y se anota su huella.
6. Mirar **todas** las pantallas del jugador movido y de **los dos equipos**: Equipos, Alineación/Estrategia y búsqueda.

---

## Registro de nuevas pruebas (se añade abajo)

<!-- Formato: **Fecha · Ranura/archivo · Qué se probó** — ✅/❌/⏳ · qué se vio · causa · qué se cambió (commit) -->

**8 oct 13:07 · Ranura 9 → jugar un partido → guardar en la ranura 10** — ✅
Todo se mantuvo: Guéhi en el Madrid, Isak en el Santos, Ekitiké en el Liverpool, Stones titular del City. El técnico de la IA puso
a **Isak de titular** en el Santos por su cuenta (Haaland pasó a la banca). Todas las alineaciones de la IA siguen bien.
> Ojo: el juego **guardó solo** encima de la ranura 9 al terminar el partido. Por eso cada prueba se identifica por su huella.

**8 oct 13:20 · Revisión del guardado de la ranura 10 (después del partido)** — 🔎 ❌ → ✅
Al intentar vender con ese archivo, el programa **se negaba** («tabla dañada»). No rompía nada (se frenó solo), pero no dejaba vender.
Causa: al jugar, el juego crea una **ficha del partido** con los que jugaron; empieza igual que una lista de la plantilla y el programa
la confundía. Arreglo: solo cuenta como lista de la plantilla la que tiene a **todos** los jugadores; la ficha del partido no se toca.
Probado con tu archivo real: venta de Max Alleyne (reserva) y de Stones (titular, con Aké de sustituto), todo verificado.

**8 oct 13:30 · Contratos dobles** — 🔎 ⏳
La tabla I guarda los **contratos**. Tres jugadores del City tienen **dos** (Rulli, Bettinelli y Stones; el segundo de Stones dura
hasta 2028). El programa solo borra el primero al vender, así que el segundo quedaría como un contrato «fantasma».
Para hacerlo exactamente como el juego: **rescindir el contrato de Stones en el juego** (ranura 10), guardar en una ranura nueva y
comparar qué borra el juego. Mientras tanto, el archivo de prueba «v9 Stones al Madrid» **no se entrega**.

**8 oct 13:30 · Ranura 10 → «Despedir» a Stones → guardar en la ranura 11** — ✅ (prueba de referencia)
Stones quedó libre y el City con 23. Ahora sabemos qué borra **el propio juego** cuando un jugador se va: **sus dos contratos** (el
segundo era una oferta de otro club) y **la negociación abierta**. Nuestro programa solo borraba el primer contrato y no tocaba la
oferta → ⏳ hay que agregarlo antes de dar la venta por terminada. Detalle en ESTRUCTURA-ML §12.
> Dato: la ranura 11 se llama `ML0000000A` (el juego cuenta en hexadecimal).

**8 oct 13:55 · Escaneo de dinero y competiciones (sin probar aún en el juego)** — 🔎
Se encontró dónde están el **presupuesto de fichajes** y el **tope salarial** de tu club, y cómo se calcula el presupuesto salarial
(tope − sueldos). Los **sueldos y cláusulas** de cada jugador están en la tabla de contratos. Los clubes de la IA **no tienen dinero**
guardado. Las **ligas de la Liga Máster** tienen su lista de equipos con espacio de sobra. Detalle en ESTRUCTURA-ML §13.

**8 oct 14:10 · Ranura 12 (`ML0000000B`) · Presupuesto de fichajes 500 M y tope salarial 200 M** — ✅
En el juego: **Ppto. fichajes 500.000.000 €** y **Ppto. salarial 86.096.000 €** (= 200.000.000 − 113.904.000, la cifra exacta que
se había calculado). El dinero del club se puede fijar desde fuera.

**8 oct 14:10 · El «blob» era zlib** — 🔎
La zona de 1,2 MB que nadie entendía se descomprime (4,7 MB) y guarda el calendario y los resultados de todas las ligas. Detalle §14.

**8 oct 14:30 · Dentro del blob: la ficha de Liga Máster de cada jugador** — 🔎
16.422 fichas de 156 B con sueldo, valor de mercado (Dias 55 M = pantalla ✓), contrato, competiciones inscritas, valoración,
crecimiento y condición. Ver ESTRUCTURA-ML §15. El calendario de partidos todavía no se localizó.

**8 oct 15:00 · Búsqueda del calendario de partidos** — 🔎 ❌ (todavía)
Se probaron todas las formas habituales de guardar un calendario y ninguna aparece. Pista fuerte: cada jornada tiene **10 números
de partido** (ej. 285–294) en el calendario del usuario. Falta un experimento: guardar justo antes y justo después de jugar un
partido, el mismo día. Ver ESTRUCTURA-ML §16.

**8 oct 15:35 · Laboratorio: la venta del C++ comparada byte a byte con el despido de Stones hecho por el juego** — ✅
Se reprogramó la venta para hacer exactamente lo que hace el juego (huecos en A–F/J/M, compactar G/H/contratos, borrar todas las
entradas de contratos y la negociación abierta) y se comparó el resultado sobre la ranura 10 con la ranura 11 del juego: **todas las
tablas, los contratos y las negociaciones quedan idénticos** (solo difieren bytes de relleno que el juego limpia, y lo que no
replicamos aún: noticias, caja, medias del equipo). Antes de esto, la venta dejaba un contrato fantasma y la oferta abierta.
Detalle en ESTRUCTURA-ML §17. 146/146 pruebas sintéticas.

**8 oct 15:40 · Prueba 14 · Ranura 13 (`ML0000000C`) · Bettinelli City → Lanús (dorsal 99), desde la ranura 11 del juego** — ⏳ EN PRUEBA
Primera venta con la lógica nueva y partiendo de un guardado en el que el juego aún no había rehecho la lista K. Qué mirar:
plantilla del City (22), negociaciones (sin la de Lanús; sigue la del Deportivo por Rulli), contratos (sin Bettinelli; ppto. salarial
+≈2,3 M), Lanús con Bettinelli al final, ficha de Guéhi en el Madrid (club/contrato), jugar y guardar.

**8 oct 16:20 · Prueba 15 · Option file `EDIT_prueba_boca` (md5 05bb20fe…) · Lautaro Blanco (LB titular de Boca) → club 193, Braida de LB** — ⏳ EN PRUEBA
Primera prueba de la corrección de alineación en el option file (bloque de tácticas, `core/Alineacion.h`). Antes, al mover a Blanco
la formación de Boca se corría un puesto (Paredes de LB … Marchesín de delantero). Qué mirar en un amistoso con Boca: Montero de
arquero, Braida de LB, el resto del once igual, Marchesín en el banco. Archivo hecho desde `save\resplado\EDIT00000000` (md5 027b0fa2…).

**8 oct 17:05 · Resultado prueba 14 · Ranura 13 (`ML0000000C`)** — ✅
En el juego: **Bettinelli en CA Lanús** (último de la plantilla, 29/29; ficha: equipo CA Lanús, liga Torneo Apertura), y el
**Manchester City** con su alineación 4-1-3-2 completa y coherente (Donnarumma de arquero y capitán, sin huecos ni jugadores en
blanco; Rulli encabeza la banca). Primera venta con la lógica «igual que el juego» (contratos, ofertas, negociaciones, huecos)
confirmada en pantalla. Pendiente de esta ranura: jugar/guardar y el fichaje (ranuras 14/15).

**8 oct 17:20 · Prueba 16 (referencia del juego) · Fichaje de Yann Sommer (Inter → City, dorsal 12) desde la ranura 2, guardado en la 15** — 🔎
Fralex fichó en el juego (el mercado de la ranura 13 ya estaba cerrado el 31/8, por eso se usó una ranura de inicio de temporada). Se
comparó con la base (= respaldo r0): el juego añade a Sommer en el primer hueco de cada tabla del usuario, con datos propios del
jugador (curva de crecimiento, estadísticas, contrato, historial, dinero). Detalle en ESTRUCTURA-ML §19. Ojo: el juego guardó también
encima de la ranura 2.

**8 oct 18:32 · Resultado prueba 15 · Option file `EDIT_prueba_boca` (regenerado, md5 f91c6c45…) en un amistoso Boca–Huracán** — ✅
Alineación de Boca en la pantalla de estrategia: **Montero** de arquero, **Braida (Malcom) de LI** en el puesto de Blanco, el resto
del once igual (Di Lollo, Costa, Lozano, Paredes, Delgado, Villa, Aranda, Merentiel, Valencia) y **Marchesín primero en el banco**.
El error del portero suplente de delantero quedó corregido: el bloque de tácticas del option file se actualiza bien.

**8 oct 18:41 · PROBAR2.bat en la PC de Fralex (ejecutables cruzados con MinGW)** — ✅
Conexión con la web OK, cuenta OK (token de Phoenix Link), **271/271 pruebas** con el option file real y el catálogo, y la
**sincronización real con la web**: `GET /liga/cambios?desde=0` respondió, **la firma Ed25519 se verificó** con la clave incrustada y
la liga está en la versión 0 (sin traspasos aún). El original no cambió (huella igual). El paso 5 (modo código manager) dio
`NO_VINCULADO`, que es lo esperado: esa PC usa el token compartido.

**8 oct 19:05 · Prueba 17 · Ranura 1 (`ML00000000`, md5 92c9e6b2…) · Reescritura del blob comprimido** — ⏳ EN PRUEBA
Hecha desde la ranura 15 del juego (fichaje de Sommer). Único cambio: **valor de mercado de Sommer 2.500.000 → 77.700.000 €**
dentro del blob, que se recomprimió con miniz (el tramo cambiado; los demás copiados tal cual). Si el juego carga la partida y
muestra 77,7 M, la escritura del blob queda validada (necesaria para inscribir fichajes en competiciones).

**8 oct 19:45 · Prueba 18 · Ranura 2 (`ML00000001`, md5 054f2e78…) · El programa ficha a Sommer (Inter → City)** — ⏳ EN PRUEBA
Desde el respaldo de inicio de temporada (4/8/2026). Sommer al City con dorsal 12 (el de su selección: el 1 está ocupado), contrato
3.186.500 €/año hasta el 30/6/2027, pago 2.575.000 €, Josep Martínez a la portería del Inter. Comparado antes con el fichaje que hizo
el juego (§20). Qué mirar: plantilla, alineación, ficha y contrato de Sommer, presupuesto, el Inter; y jugar un partido oficial.

**8 oct 19:08 · Resultado prueba 17 · Ranura 1 · Blob reescrito por el programa** — ✅
La partida carga normal y la ficha de Sommer muestra **Valor de mercado 77.700.000 €** (antes 2.500.000). El presupuesto no cambió
(160.665.600 €): solo cambió el dato pedido. **El juego acepta la zona comprimida recomprimida por Phoenix Mercado (miniz).**

**8 oct 19:20 · Resultado prueba 18 · Ranura 2 (fichaje de Sommer hecho por el programa)** — ⚠️ CASI (se corrigió)
En pantalla todo bien: Sommer en el City (27/27), sueldo 3.186.500 €, contrato hasta 30/6/2027, presupuesto 160.665.600 €,
ppto. salarial 446.900 €, Inter con Josep Martínez de arquero. Fralex lo puso de titular, el partido **arrancó con Sommer** (está
bien inscrito); al salir del partido el juego volvió al menú sin guardar, pero antes había **autoguardado** la ranura 2 (13/8).
En ese autoguardado: todo sigue igual salvo que **el juego creó su propio contrato** para Sommer (nº 25, cláusula 2.400.000 €,
calculado desde la ficha del blob) y el nuestro (nº 28) quedó **duplicado** → la suma de sueldos supera el tope salarial.
**Corrección:** el programa ya no escribe el contrato en la tabla I; solo la ficha del blob (el juego hace el resto).

**8 oct 19:25 · Prueba 19 · Ranura 2 (`ML00000001`, md5 1ba38ab8…) · Sommer fichado v2 (sin contrato en la tabla I)** — ⏳ EN PRUEBA

**8 oct 19:26 · Resultado prueba 19 · Ranura 2 → partido simulado → ranura 3 (`ML00000002`, md5 432531d3…)** — ✅
Fecha 15/8/2026. Sommer sigue en el City (dorsal 12, titular de arquero), **jugó el partido** (tabla de estadísticas C: 1 partido),
**un solo contrato** (el que crea el juego, nº 25, 3.186.500 €/año, cláusula 2.400.000 €), inscrito en las 6 competiciones,
presupuesto de fichajes 160.665.600 € y **presupuesto salarial positivo (902.100 €**: el juego subió el tope a 147.515.200 €).
**Fichar PARA el equipo del usuario queda validado en el juego.** Además: el juego relee el archivo al cargar la ranura, así que
se puede cambiar un guardado con el juego abierto mientras no esté cargado (Fralex salió y entró a la ranura 2 y vio la versión nueva).

### 2026-10-09 · Sider · Prueba 1 «buzón» (phoenix.lua) — INSTALADA, ESPERANDO A FRALEX
- Instalada en la carpeta del modo `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`, porque el switcher copia esa carpeta encima del juego con robocopy:
  - `modules\phoenix.lua`;
  - la línea `lua.module = "phoenix.lua"` (línea 199, la última de los módulos);
  - `content\phoenix\avisos.txt`.
- Respaldo: `sider.ini.respaldo-phoenix-20261009`. El diff confirma que solo cambió 1 línea.
- Antes de instalar: sintaxis comprobada con LuaJIT y simulación con un contexto falso de Sider (lee el aviso con tildes y ñ).
- Los avisos en vivo se escriben en la raíz: `Conmegol Patch\SiderAddons\content\phoenix\avisos.txt`, porque `sider_dir` es la raíz.
- Resultado en el juego: (pendiente)
- **Resultado 1 (00:34): NO cargó.** En sider.log: `"init" function returned an error: phoenix.lua:145: attempt to call global 'pcall' (a nil value)` → `Module (phoenix.lua) is NOT activated`. El juego siguió funcionando normal.
  **Causa:** el Lua de Sider 7.3.3 **no trae `pcall`**. El volcado de env.lua en el mismo log lista los globales que sí existen: assert, pairs, ipairs, tostring, tonumber, type, error, unpack, collectgarbage, table, string, math, os, io, fs, ffi, jit, bit, zlib, memory, match, audio, input, log.
  **Arreglo (v0.2):** sin `pcall`. Los errores se cuentan con una «bandera» (Sider ya atrapa el error de cada evento) y el módulo se apaga solo tras 5 seguidos. Probado en LuaJIT con un entorno igual al de Sider (sin pcall) y con errores simulados.
  **Lección:** probar siempre con el entorno restringido de Sider, no con Lua normal.
- **Resultado 2 (00:39): FUNCIONÓ.** Fralex reinició el juego con el switcher (Shift+R no sirve aquí: solo recarga módulos que ya estaban activos). El overlay muestra «PHOENIX EVOLUTION · puente en vivo v0.2-prueba · conectado» con el aviso de bienvenida.
- **Resultado 3 (00:39): TIEMPO REAL CONFIRMADO.** Con el juego abierto se reemplazó `avisos.txt` en la raíz (tmp + mv, atómico). El overlay mostró el texto nuevo en ~1 s («último aviso 00:39:07» para un archivo escrito a las 00:39:06), sin cerrar el juego. Las tildes, la ñ y los símbolos ⚡ y → se ven bien con la fuente Consolas del overlay. El juego siguió estable (pantalla de título).

### 2026-10-09 · Sider · Prueba 2 «solo mirar» (phoenix.lua v0.3) — INSTALADA, ESPERANDO A FRALEX
- **Qué hace:** con el overlay abierto en PHOENIX EVOLUTION, la tecla **B** busca en la memoria del juego la ficha de Lamine Yamal (ID 162114). Usa dos patrones:
  - A = 58 bytes de Player.bin desde el ID (base viva olmosjr23, registro 16179);
  - B = el nombre «Lamine Yamal».
- **Cómo busca:** recorre las regiones COMMIT legibles con `VirtualQuery` (ya declarado por la librería memory de Sider) y salta las zonas con guarda o sin acceso. Revisa 24 MB por cuadro para no congelar el menú. **Solo lee; nunca escribe.**
- **Qué muestra:** por cada coincidencia, la dirección y 5 cualidades decodificadas. El detalle completo va a sider.log (líneas `[phoenix] sonda`).
- **Prueba previa:** simulación en LuaJIT con memoria falsa (3 regiones, una no legible). Encontró las 2 copias legibles con las cualidades correctas (Vel 90, Ace 93, Reg 93, Fin 81, Pase 82), ignoró la región sin acceso y no tocó nada.
- **Resultado en el juego:** (pendiente)
- **Resultado 1 (02:41, v0.3):** revisó 4407 MB en 3735 regiones en 32,7 s. El juego aguantó.
  - Patrón exacto de 58 bytes: **0 coincidencias**. El juego cambia bits al cargar la ficha (forma, lesión…), así que hay que buscar por ID y verificar.
  - Nombre «Lamine Yamal»: 6 coincidencias, pero todas eran textos nuestros (el overlay y el log) o zonas que ya habían cambiado.
- **Resultado 2 (02:43, v0.3): CRASH.** Fralex pulsó B otra vez y el juego se cerró.
  - dlss5-feed.log: `exception 0xC0000005 (reading address 0x6E0000) in sider.dll`, justo después de que ReShade recargara sus efectos (02:43:01) y liberara memoria.
  - **Causa:** la búsqueda leía DIRECTO zonas de memoria. Se averiguaba qué zonas eran legibles en un cuadro y se leían en los siguientes; otro hilo la liberó en medio → lectura de memoria inexistente.
  - **Daño:** ninguno. Solo se leía. Los guardados (EDIT, ML00000000-2, SYSTEM) tienen fecha anterior al crash.
  - **Acción inmediata:** se reinstaló phoenix.lua v0.2 (solo avisos) en la raíz y en la carpeta del modo.
- **Arreglo v0.4 (NO instalado, espera el OK de Fralex):**
  - nunca lee la memoria directo: la **copia con ReadProcessMemory** sobre el propio proceso (si la zona ya no existe, devuelve «falló» en vez de cerrar el juego) y busca sobre la copia;
  - solo revisa memoria privada (heap) y salta la mapeada (gráficos y ReShade);
  - busca el ID (4 bytes) y verifica 5 cualidades (≥ 4 iguales);
  - además busca «LAMINE YAMAL»;
  - quita duplicados.
  - **Simulado:** zona «liberada» a mitad de la búsqueda (sin crash, contada como lectura fallida), zona mapeada saltada, 2100 IDs falsos descartados, ficha con un bit de forma cambiado encontrada.
  - **Lección:** en el juego, otros hilos (ReShade, DLSS, carga de texturas) liberan memoria en cualquier momento. **Nunca leer memoria ajena directo; siempre copiar con una función que pueda fallar sin cerrar el juego.**
- **Resultado 3 (02:53, v0.4): FICHA ENCONTRADA, sin crash.**
  - 2728 MB (solo memoria privada) en 6,6 s, 0 lecturas fallidas, 24 candidatos de ID.
  - **1 ficha verificada en 0x7ff4d8f80640**, idéntica a Player.bin (8 bytes en cero + ID): Velocidad 90, Aceleración 93, Regate 93, Finalización 81, Pase raso 82. Foto de Fralex.
  - El nombre aparece además en **0x7ff4d932d570**, una segunda ficha cuyos bytes difieren (por ejemplo, el byte 66 es 0x31 en vez de 0x00). Puede ser la copia «viva» que usa el juego.
  - Siguiente paso: v0.5 lee y compara las dos copias con el archivo, todavía solo leyendo.
- **Resultado 4 (02:55, v0.5, Fralex en la pantalla de habilidades de Lamine):**
  - 2926 MB en 3,8 s, sin crash.
  - **Ficha verificada, ahora en 0x55b7380** (el juego la movió desde 0x7ff4d8f80640: las direcciones cambian, siempre hay que buscarla).
  - Las «Copias 1-5» **no eran fichas**: eran otras listas con el nombre repetido a la misma distancia. Sus cualidades salían en 40 porque no son registros de jugador. Arreglo: exigir el ID en +8 para llamarla «copia».
  - **Calibración:** las 15 cualidades que muestra el juego en esa pantalla coinciden exactamente con nuestro mapa de bits, **incluida Potencia de tiro 78 (bit 358), que pasa de «por confirmar» a CONFIRMADA**.

### 2026-10-09 · Sider · Prueba 3 «escribir un número» (phoenix.lua v0.6) — INSTALADA CON OK DE FRALEX
- **Teclas:** V = buscar la ficha y poner Velocidad 99 **solo en la memoria**; N = devolver el byte original.
- **Qué cambia:** **un solo byte**, el 38 de la ficha (Velocidad = bits 306-311 = bits 2-7 de ese byte). Los 2 bits bajos se conservan.
- **Antes de escribir, en el mismo instante:** se vuelve a leer la ficha y se comprueban ID + nombre + ≥4 cualidades.
- **Cómo escribe:** con WriteProcessMemory, que falla sin cerrar el juego si la zona ya no existe. Después lee de nuevo para confirmar.
- **N** solo devuelve el byte si todavía tiene el valor escrito por nosotros.
- **cdef con nombres por versión** (phx06_*), porque tras Shift+R el Lua es el mismo y no se puede redeclarar.
- **Simulado:** 2 fichas legibles cambiadas de 90 a 99, la zona «liberada» intacta; N las devolvió a 90.
- **Resultado en el juego:** (pendiente)
- **Resultado 1 (03:04, v0.6):** se escribió Velocidad 99 en 1 copia (0x55b7380), pero la pantalla de habilidades de Lamine (Día de partido, Arsenal–Barça) **siguió mostrando 90**.
  - El log muestra **4 copias** de la ficha (0x4c98a98, 0x55b7380, 0x6058ac0, 0x77b88ae8), con los primeros 96 bytes idénticos.
  - 3 se saltaron porque **no tienen el nombre en el byte 129**, y v0.6 lo exigía. Esas copias sin nombre son las que crea y usa el juego (aparecen nuevas al entrar a pantallas).
  - El aviso «[02:59] hola» llegó **desde la web** por Phoenix Link: **cadena web → Link → buzón → Sider confirmada de punta a punta.**
- **Arreglo v0.7:** para escribir se exige el ID en +8 y los bytes 12-65 idénticos a Player.bin, salvo el de Velocidad y los bytes 54-57 (forma, lesión y pie malo, que el juego varía). Así se escribe en todas las copias reales sin depender del nombre. Simulado OK (incluida una ficha con el bit de forma cambiado).
- **Resultado 2 (03:11, v0.7):** la pantalla de habilidades siguió mostrando **90**. Pero el log demuestra que **todas** las copias empaquetadas ya tienen 99, incluso las que el juego crea nuevas al entrar a menús (por ejemplo, 0x4e15c38). La escritura en la copia «madre» funciona y se propaga.
  - **Conclusión:** la pantalla lee **otra representación**, probablemente la ficha «desempaquetada» (una cualidad por byte o por número), creada antes y no regenerada.
- **v0.8 (solo lectura en B):** además de lo anterior, busca 6 formas desempaquetadas de las cualidades de Lamine (orden de pantalla y orden del archivo; valores tal cual y −40; bytes, u16 y u32), sin la velocidad. Simulado OK.
- **Idea de Fralex:** usar los menús del juego («Datos Actual. en vivo») en vez del overlay de Sider. Se investigó: los «live update» de los parches (por ejemplo, EPP) son bases de datos servidas por Sider al arrancar, no la función nativa del juego.
- **Resultado 3 (03:17, v0.8):** 0 coincidencias en las 6 formas desempaquetadas. La pantalla sigue en 90.
  - Hay **7 copias**: las nuevas nacen con 90 aunque las anteriores ya tenían 99, y todas tienen el formato exacto del archivo (8 ceros + ID).
  - **Nueva hipótesis:** el juego **vuelve a leer `common\etc\pesdb\Player.bin`** (vía Sider livecpk) cada vez que necesita al jugador, en vez de tener una copia madre.
  - Si se confirma, el camino limpio es servirle **nuestro** Player.bin con `livecpk_get_filepath`: sin tocar memoria, y cada pantalla nueva vería los cambios.
  - **Técnica vista en el parche:** `lib\CommonLib.lua` (zlac) usa el evento `livecpk_read(ctx, filename, addr, len, total_size, offset)` para leer CompetitionEntry.bin y CompetitionRegulation.bin.
- **v0.9 (espía pasivo):** registra `livecpk_read` y solo cuenta y anota cuándo empieza cada lectura de `pesdb\*.bin`. No lee ni cambia datos. Simulado OK.
