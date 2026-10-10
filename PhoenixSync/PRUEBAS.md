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

### 2026-10-09 · Espía de lecturas (v0.9) — RESULTADOS y prueba 4 «nuestro libro»
- **Cuándo lee el juego `pesdb\Player.bin`** (log del espía): al arrancar (03:28:16), en «Datos del sistema: Cargando» (03:28:29) y **al entrar y salir del modo EDITAR (03:33:02 → lectura #3)**. No lo relee al entrar a la pantalla de habilidades, al cambiar «Datos Actual. en vivo» ni al entrar a la Liga Máster (que lee `installversionplayer.bin`).
  - ⇒ **Botón nativo para recargar la base sin reiniciar: entrar y salir de EDITAR.**
- **«Datos Actual. en vivo → Activar»:** el juego responde «Los servicios en línea de este título finalizaron el 25/08/2022». La actualización en vivo nativa depende del servidor de Konami (cerrado), así que no sirve como canal.
- **Prueba 4 instalada:** nueva raíz `livecpk\Phoenix-DB` puesta **antes** de `olmosjr23\Database` en sider.ini (raíz y carpeta del modo; respaldo `sider.ini.respaldo-phoenixdb-20261009`; diff = 1 línea).
  - Sirve un Player.bin propio: base viva de olmosjr23 descomprimida, **1 solo byte cambiado** (Lamine, byte 38 → Velocidad 99) y recomprimida WESYS (zlib 9, cabecera original con tamaños nuevos; verificada de ida y vuelta).
  - En `_PhoenixMercado_prueba\db\` hay variantes v90, v95 y v99 para la prueba en vivo.
  - **Plan:** (a) reiniciar → la pantalla debe mostrar 99; (b) con el juego abierto, cambiar el archivo a v95 → entrar y salir de EDITAR → la pantalla debe mostrar 95.
- **Resultado A (03:41): ✅ EL JUEGO USA NUESTRO PLAYER.BIN.** Tras reiniciar, la pantalla de habilidades de Lamine muestra **Velocidad 99**, servida por la raíz `livecpk\Phoenix-DB` con 1 byte cambiado. **Cambiar stats «al reiniciar» queda probado.**
- **Resultado B (03:42-03:46): ❓ sin confirmar.**
  - Se cambió el archivo a v95 con el juego abierto (03:41:22). El juego releyó Player.bin al salir de Editar (03:41:57, mismo tamaño).
  - Pero en un amistoso nuevo (Inter–Barça) siguió 99 y no apareció ninguna copia con 95.
  - **Ensuciado:** se pulsó V (escribir en memoria) y volvió a poner 99 en una copia que tenía 90 («byte c8 -> ec»).
- **v0.10:**
  - **V y N desactivadas:** el camino elegido es el archivo, no la memoria.
  - **Huella de cada lectura de player.bin:** suma cada 64 bytes del trozo que entrega Sider, con su offset y largo. Así se sabe si el juego leyó v95 o v99.
  - Prueba limpia: arrancar con v95 → debe verse 95. Cambiar a v90 con el juego abierto → Editar → amistoso nuevo → mirar y comparar huellas.

### 🏆 2026-10-09 03:53 · CAMBIO DE STATS EN VIVO, SIN REINICIAR — CONSEGUIDO
- **Huella (v0.10):** al arrancar, el juego leyó **exactamente** el Player.bin v95 (sumas 1785225260 / 304097887). La pantalla mostró **95**.
- **03:51:21:** con el juego abierto se reemplazó el archivo por **v90** (tmp + mv).
- **Solo entrar y salir de EDITAR (03:51:51):** el juego relee Player.bin y la huella es la de **v90** (3958249664 / 1038543739). **Pero el amistoso sigue en 95.** Editar carga la base en un borrador propio y lo descarta al salir.
- **EDITAR → CARGAR** (cargar EDIT00000000, sin guardar; 03:53:45, huella v90): amistoso nuevo → **Velocidad 90**. ✅
- **MÉTODO PROBADO:**
  1. Phoenix Sync genera Player.bin con los cambios (base viva + bytes cambiados, WESYS/zlib).
  2. Se coloca de forma atómica en la raíz `livecpk\Phoenix-DB`, que va antes de `olmosjr23\Database`.
  3. El jugador pulsa **Editar → Cargar** (un botón nativo del juego).
  4. En el siguiente partido o pantalla nueva se ven los cambios. Sin reiniciar y sin tocar la memoria.
- **Estado al cerrar la prueba:** Phoenix-DB tiene v90, cuyos datos son idénticos a los originales de olmosjr23 (solo recomprimido), en la raíz y en la carpeta del modo. Lamine queda como en el parche.
- **Cuidado futuro:** la raíz Phoenix-DB **tapa** el Player.bin de olmosjr23. Cuando el parche se actualice, hay que regenerar nuestro archivo desde la base nueva (Auditor de parches), o se servirían datos viejos.

### 2026-10-09 04:20 · Calibración con la ficha de Joan García (4 pantallas del juego)
- **Habilidad 3/4 del juego:** Actitud de portero 92, Atajar 90, Despejar 89, Reflejos 92, Cobertura 91.
  - Player.bin: 269 = 89, 300 = 91, 320 = 92, 326 = 92, 364 = 90.
  - ⇒ **269 = Despejar, 300 = Cobertura, 364 = Atajar** ✅. 320 y 326 = Actitud o Reflejos; los dos valen 92, así que falta una foto de **Livakovic** (320 = 84, 326 = 82).
- **Uso de pie malo 2, precisión 2, regularidad 4, resistencia a lesiones 2.** Guardado: 454 = 1, 462 = 1, 438 = 3, 458 = 1 ⇒ **lo que muestra el juego = valor guardado + 1** ✅ (Lamine, guardado 2/2/4/1, cuadra con 3/3/5/2).
- **Habilidad 2/4:** las 17 cualidades de campo coinciden con el mapa, incluida Potencia de tiro 81 ✅.
- **Habilidad especial:** «Patadón en largo (PT)». Sin estilos COM. Sirve para empezar a calibrar los bits 480-531.
- **04:23 · Livakovic (4 pantallas):** Actitud 82, Atajar 75, Despejar 77, Reflejos 84, Cobertura 86.
  - Player.bin: 269 = 77, 300 = 86, **320 = 84 ⇒ Reflejos**, **326 = 82 ⇒ Actitud de portero**, 364 = 75 ⇒ **portero 100 % mapeado** ✅.
  - Pie malo 1/2, regularidad 6, lesiones 3; guardado 0/1/5/2 ⇒ +1 confirmado otra vez ✅.
- **Habilidades especiales:**
  - Joan: solo «Patadón en largo (PT)», bit 496 encendido ⇒ **496 = Patadón en largo** ✅.
  - Livakovic: Patadón + «Pase largo portero» + «Parapenaltis», bits 496, 502 y 522 ⇒ 502/522 son esas dos (el orden se resuelve con Szczęsny: 486, 490, 502).

### 🏆 2026-10-09 04:47 · FICHAJE EN VIVO, SIN REINICIAR — CONSEGUIDO (Lamine Yamal → Real Madrid)
- **Intento 1 (04:35-04:41): ❌ por el archivo de la base.** Se sirvió un `PlayerAssignment.bin` propio desde `livecpk\Phoenix-DB` (fila de Lamine: equipo 108 → 109, dorsal 26, orden 27). Editar → Cargar. El espía confirma que el juego **sí leyó nuestro archivo** (155 323 bytes, lectura #6 a las 04:39:49), **pero Lamine siguió en el Barça**.
  - **Motivo:** al pulsar Cargar, las **plantillas vienen del option file** (`EDIT00000000`, bloque de plantillas de 284 B por equipo, 749 equipos). El option file manda sobre `PlayerAssignment.bin`. Las stats sí funcionaron por archivo porque Lamine **no está entre los jugadores editados** del option file; las plantillas, en cambio, están completas en el option file.
  - Phoenix-DB quedó después con el PlayerAssignment original (misma suma que olmosjr23).
- **Intento 2 (04:44-04:47): ✅ por el option file.** Con el juego abierto se reemplazó `239200\save\EDIT00000000` por una copia modificada con `mover_of` (núcleo de Phoenix Sync, `OptionFile::mover`): Lamine sale del Barça (sustituto en el once: Roony Bardghji) y entra en el Real Madrid con el **dorsal 26**. Solo cambian **95 bytes** (plantillas de los dos clubes + alineación del Barça). Respaldo en `save\EDIT00000000.respaldo-fichaje-20261009` y en `_PhoenixMercado_prueba\db\`.
  - **Editar → Cargar → amistoso Real Madrid–Barça:** **Lamine titular en el Madrid (87) y ausente en el Barça.** Sin reiniciar, sin memoria, con un botón nativo.
  - Curioso: el juego lo puso **titular** aunque entró como última reserva; el juego reordena el once del Madrid por su cuenta (orden identidad en el option file).
- **Lo aprendido del formato `PlayerAssignment.bin` (16 B):** `[índice u32][jugador u32][equipo u32][dorsal−1 u8][orden<<2 u8][banderas u8: 0x20 = capitán?][0]`. Courtois 0 → dorsal 1, Mbappé 9 → 10, Lamine 9 → 10.
- **Regla nueva:** **stats → archivo de la base (Phoenix-DB); plantillas (fichajes) → option file.** Las dos cosas se aplican con el mismo botón: Editar → Cargar.
- **Dos carpetas de guardado en el PC:** la activa es `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save` (SYSTEM y ML00000000 cambian hoy); la de `292733975847239680\save` no se usa.
- **Herramienta:** `mover_of` ahora acepta un 7.º argumento (dorsal). Antes ponía 99 por defecto.

### 2026-10-09 04:53 · ¿Las stats en vivo llegan a la Liga Máster? → ❌ NO (con Editar → Cargar)
- Phoenix-DB con **Player_v99** (Lamine Velocidad 99). Editar → Cargar a las **04:51:34**: la huella es **exactamente v99** (3611839843 / 3265817782). FRALEX lo hizo bien.
- Entró a su Liga Máster (Borussia Dortmund) a las 04:52:00 (el juego solo lee `installversionplayer.bin`). Ficha de Lamine en la carrera: **Velocidad 90**.
- **Conclusión:** la Liga Máster **no** toma las stats de la copia que rehace «Cargar». Usa su propia copia (la del arranque o la de su guardado).
- **Siguiente prueba para separarlo:** reiniciar el juego con v99 puesto y entrar a la misma carrera. Si sale 99 → la LM usa la base del arranque (stats nuevas al reiniciar). Si sigue 90 → las stats viven dentro del guardado `ML0000000N` y habría que editarlas ahí (Phoenix Sync ya edita ese archivo).
- Phoenix-DB queda con **v99** a propósito para esa prueba.
- **Ejecutable:** `PES2021.exe` pesa 437 MB, con una sección `.impdata` de 380 MB (capa de protección típica tipo Denuvo) y el código en `.trace` (37 MB). Importa para la idea de llamar funciones internas.

### 2026-10-09 05:10 · Estudio de PES2021.exe y phoenix.lua v0.11 (tecla L: recarga nativa) — INSTALADO, falta probar
- El código del exe (`.trace`, 37 MB) **no está cifrado**. Mapa completo en `base-conocimiento/07-MOTOR-POR-DENTRO.md`.
- **Interruptor nativo** `exe+0x37F5C39`: si vale 1, al volver al menú principal el juego crea la tarea `editLoadDataInTopMenu` (misma fábrica que Editar → Cargar) y lo pone a 0. Konami lo usa en «Ser una Leyenda».
- **Actualización en vivo de Konami** = máquina de 38 estados. El 5 pide la lista al servidor y el 26 carga el EDIT (`editLoadDataInLiveDataSet`).
- **v0.11:** tecla L → comprueba 7 bytes de código en `0xAEF770` y `0x1EFB440` → escribe 1 en el interruptor → el overlay muestra el valor y avisa cuando el juego lo consume. Simulación LuaJIT OK (incluido el caso «exe distinto → no toca nada»).
- **Instalado** en las dos carpetas `modules` (respaldo `phoenix.lua.v010`). Phoenix-DB con **Player_v95** (huella 1785225260 / 304097887) para ver el cambio.
- **Plan:** Shift+R → overlay → L → salir de la Liga Máster al menú principal → ¿«EL JUEGO RECARGÓ»? → amistoso: ¿Lamine 95?

### 2026-10-09 05:08 · Tecla L (interruptor nativo) — ✅ el juego obedece, pero recarga SOLO el option file
- 05:08:02: L → interruptor 0 → 1 ✓ (base del exe 0x140000000, bytes de código comprobados). 05:08:05: **el juego lo consumió (1 → 0)** al entrar a un modo (pantalla de amistoso). Sin crash.
- **Pero la base no se releyó:** el espía no ve ninguna lectura de `pesdb` (solo `installversionplayer.bin`). Lamine sigue en **99** (la base del arranque de las 05:01, que se hizo con v99). El v95 puesto después no se leyó.
- ⇒ `editLoadDataInTopMenu` recarga **solo el EDIT**. Editar → Cargar hace **dos cosas**: la tarea del EDIT **y** `StartReloadPesdb(gestor, 0)`.
- **Hallado en el exe (código C++ legible):**
  - `0x1EF2250` = obtener el gestor de datos de edición; `0x1EEBBA0(1)` = crearlo si no existe.
  - `0x1EF2FA0(gestor, tipo)` = **iniciar la relectura de la base** (tipo 0 = la de Editar → Cargar). Crea un objeto de 0x48 B que el gestor avanza en cada cuadro (`0x1F08820`, lee con `0x125F310` «cpk_dat/common/etc/pesdb/%s»). Listas de archivos por tipo en `0x3529900`.
  - `0x13E4580` (proceso de Editar → Cargar): `0x1EEBBA0(1)` → `gestor = 0x1EF2250()` → `0x1EF2FA0(gestor, 0)`.
  - El constructor de la tarea de carga del EDIT (`0x1EFAE20`) está **virtualizado por la protección** (salta a `.impdata`): no se puede leer, solo usar.
- **Arranque de las 05:01 con v99:** FRALEX entró a la Liga Máster a las 05:01:20 → **falta saber qué Velocidad vio ahí** (resuelve si la LM toma la base del arranque).
- **Siguiente prueba (A, fichajes):** se devolvió el option file **original** (Lamine en el Barça; sha cb6e176e…) con el juego abierto. L → entrar a un modo → ¿Lamine vuelve al Barça? Si sí: **fichajes en vivo sin Editar → Cargar**.
- **Prueba B (stats, más adelante y con OK):** llamar desde Sider a `0x1EF2FA0(gestor, 0)`. Es llamar a una función del juego desde otro hilo, así que hay riesgo de cierre (sin daño a los datos).

### 🏆 2026-10-09 05:13 · FICHAJE EN VIVO **SIN EDITAR → CARGAR** — CONSEGUIDO (tecla L)
- Option file original puesto con el juego abierto (Lamine en el Barça). FRALEX: menú principal → overlay → **L** (05:13:17; marcaba «1 → 1» porque el interruptor ya estaba en 1 de una pulsación anterior) → entrar a Partido → **«✓ EL JUEGO RECARGÓ» (05:13:25)**.
- Amistoso Barça–Atlético: **Lamine Yamal vuelve a ser titular en el Barça (ID, 90)** y Roony Bardghji vuelve al banquillo. Sin entrar a Editar, sin reiniciar, sin crash.
- **MÉTODO PROBADO (fichajes):**
  1. Phoenix Sync cambia el option file (`OptionFile::mover`) y lo coloca de forma atómica.
  2. Sider pone a 1 el interruptor nativo `exe+0x37F5C39` (con comprobación de bytes del exe).
  3. El juego recarga el option file él solo **al entrar a un modo** (menu::ModeFlowCmnInitFunctor).
- **Estado al cerrar:** option file = original (Lamine en el Barça, sha cb6e176e…). El Madrid ya no tiene a Lamine.
- **Pendiente:** stats sin Editar (llamar `0x1EF2FA0(gestor, 0)`, prueba B) y la Velocidad que vio FRALEX en la Liga Máster tras el reinicio de las 05:01.
- **05:15 · confirmación:** después de la recarga con L, la ficha de Lamine (ya en el Barça) sigue con **Velocidad 99**, la base del arranque de las 05:01. Confirma que el interruptor recarga el option file pero **no** la base de stats.

### 2026-10-09 05:20 · Prueba B preparada: phoenix.lua v0.12 (tecla K = releer la base sin Editar) — INSTALADO, falta probar
- OK de FRALEX a las 05:17, aceptando el riesgo de cierre.
- **K:** comprueba bytes (`0x1EF2FA0` 25 B, `0x1EF2250` 8 B, `0x13E4580` 32 B) → lee el gestor `[exe+0x37F5C28]` → exige `[gestor+0x88] = 0` → **llama a `exe+0x1EF2FA0(gestor, 0)`**, igual que Editar → Cargar. El overlay muestra «relectura: EN CURSO / libre» y «✓ BASE RELEÍDA».
- El gestor está justo al lado del interruptor (`0x37F5C28`); el lector `0x1EF2250` es solo `mov rax, [exe+0x37F5C28] ; ret`.
- Simulación LuaJIT OK: llama una sola vez; no llama si hay relectura en curso, si el exe es distinto o si el gestor no existe.
- Instalado en las dos carpetas `modules` (respaldo `phoenix.lua.v011`). Phoenix-DB con **v95** (el juego tiene v99 del arranque).
- **Plan:** Shift+R → menú principal → overlay → **K** → ¿«BASE RELEÍDA»? (el espía debe ver player.bin con la huella de v95) → ¿Velocidad 95 ya? Si no: **L** + entrar a un modo → ¿95?

### 2026-10-09 05:21 · Prueba B, intento 1 → «NO se llamó: el gestor no existe» (sin riesgo, no se llamó nada)
- En el menú principal `[exe+0x37F5C28] = 0`: el gestor de edición **solo existe** mientras se edita o se recarga.
  - Lo crea `0x1EEBBA0(1)`: reserva 0x310 B, lo guarda en el global y lo cuelga del árbol de tareas (grupo 2), así que el juego lo avanza solo.
  - Lo destruye `0x1EEDAA0` (estado 0xC del menú principal).
- **v0.13:** si el gestor no existe, K llama primero a `0x1EEBBA0(1)` (comprobando sus 34 bytes), igual que Editar → Cargar (`0x13E4580`: crear gestor → releer base). Simulación OK. Instalado (respaldo `phoenix.lua.v012`).
- El interruptor quedó en 1 (FRALEX pulsó L a las 05:20:54): la próxima entrada a un modo recargará el EDIT.

### 2026-10-09 05:23 · Prueba B, intento 2 (v0.13) → ✅ ¡el juego relee la base sin Editar! (falta ver si se aplica)
- K a las 05:23:02:
  - `0x1EEBBA0(1)` creó el gestor (0x37779130);
  - `0x1EF2FA0(gestor, 0)` respondió 1;
  - el juego leyó **boots, glove, player y country**, y la huella de player.bin es **exactamente v95** (1785225260 / 304097887). **Sin crash.**
- La lista del tipo 0 (en `0x3529900` → `0x142B9D3C0`) es justo `[2, 11, 15, 9]` = Boots, Glove, Player, Country (28 = fin). **La relectura se completó entera.** (Tipo 2, usado por la carga del EDIT: `[2, 11, 15, 9, 25, 16, 3, 5, 8, 6, 19]`.)
- El overlay sigue en «EN CURSO»: el objeto de relectura no se libera fuera de Editar (`[gestor+0x88] ≠ 0`). Las pulsaciones extra de K se rechazaron bien («ya hay una relectura en curso»).
- **Siguiente:** con el interruptor en 1, entrar a Partido → ¿Lamine 95?

### 2026-10-09 05:27 · Prueba B, intentos 3–4 → la base se LEE pero NO se APLICA (Lamine sigue en 99)
- Secuencia limpia (log): K a las 05:26:40 (gestor creado, player.bin v95 leído) → interruptor consumido a las 05:27:00 **con el gestor vivo** → gestor destruido a las 05:27:01. Ficha de Lamine: **99**.
- ⇒ «leer la base» (tipo 0) no basta. Hace falta que la **carga del EDIT** la aplique.
- **Clave hallada (05:30):** la tabla de procesos en `.data 0x34DA048` = { `0x130F220`, «ProcessEditDataLoad::CreateReloadPesdb», «Edit/Load/EditDataLoadProcess» }. Esa fábrica construye el proceso de **Editar → Cargar** con la bandera `[+0x8C] = 1` (la otra fábrica, `0x130F1C0`, usa 0). `0x130F280` pasa esa bandera como **primer byte de parámetros** de la tarea «editLoadData» → parámetros **1,1,0,0,0**.
  - El menú principal (`0xAEF78A: mov dword [rsp+0x20], 0x100`) pasa **0,1,0,0,0**. Por eso recarga el EDIT pero **no** la base.
- **v0.14 · tecla P (recarga completa):** comprueba los 8 bytes de `0xAEF78A` → VirtualProtect → cambia **1 byte de código en memoria** (`exe+0xAEF78E`: 00 → 01, o sea 0x100 → 0x101) → restaura la protección → verifica → pone el interruptor. El exe en disco **no se toca**; el cambio dura hasta cerrar el juego. Simulación OK (casos: ya activo, exe distinto → no toca nada).
- Instalado (respaldo `phoenix.lua.v013`). Phoenix-DB sigue con v95.

### 🏆🏆 2026-10-09 05:31 · RECARGA COMPLETA SIN EDITAR → CARGAR — CONSEGUIDA (tecla P)
- 05:30:14: P → parche en memoria `exe+0xAEF78E` 00 → 01 aplicado (la página ya era 0x40 = RWX) + interruptor 0 → 1.
- 05:31:49: FRALEX volvió al menú principal y entró a Partido → el juego consumió el interruptor y **releyó TODA la base** (el espía ve ball, boots, coach, competition*, country, derby, glove, player, playerassignment, stadium, tactics, team, weekly…). Huella player.bin = **v95** (1785225260 / 304097887).
- Ficha de Lamine en el amistoso: **Velocidad 95** (antes 99). ✅ **Stats en vivo sin entrar a Editar, sin reiniciar y sin crash.**
- **MÉTODO FINAL (stats + fichajes):**
  1. Phoenix Sync coloca el `Player.bin` nuevo (Phoenix-DB) y/o el option file nuevo.
  2. Sider: parche de 1 byte en memoria (0x100 → 0x101, la recarga del menú pasa a ser «como Cargar») + interruptor nativo `exe+0x37F5C39` = 1. Todo con comprobación previa de bytes.
  3. El jugador entra a cualquier modo → el juego recarga **EDIT + base** solo.
- La tecla K (v0.12–0.13) queda como herramienta de diagnóstico: lee la base pero no la aplica. No hace falta.

### 2026-10-09 05:45 · La «puerta» del botón nativo, encontrada — v0.15 (tecla U) instalada, falta probar
- **Ruta del botón** «Partido → Datos Actual. en vivo → Activar» (tabla de procesos en .data):
  - `Exhibition/LiveData/LiveDataSet` → `ProcessLiveDataSet` (0x1308000/0x1308090);
  - → «ProcessCmnLiveDataSetFlow» (crea 0x1350E10, actualización `0x20AF620`, estado en `[+0x94]`).
  - Hay también `LiveDataCheck` (0x1303F70) y `LiveDataRemove` (0x1308DD0).
- **Estados del flujo común** (`0x20AF620`):
  - 0 → 1: crea «LiveDataLogin» (**inicio de sesión en Konami = la puerta**);
  - 2: espera. El resultado lo procesa `0x20AF7D0`: evento 0x10D0002 → la decisión `0x1350E90` está **virtualizada** por la protección → si falla, estado 5 = **diálogo de error** («servicios finalizados»);
  - 3: crea «LiveDataSetFlow» (los 38 pasos);
  - 7: termina y avisa al padre (evento 0x10A0001, resultado `[+0xA8]`).
- **Parche v0.15 (tecla U, en memoria, 19 B en `exe+0x20AF73B`, inicio del estado 1):** `mov byte [exe+0x37F5C39], 1 ; mov dword [rdi+0x94], 7 ; jmp 0x20AF7BA`. Así el botón **no inicia sesión ni da error**: enciende nuestro interruptor y termina bien. Con la recarga completa (U también aplica el byte de la P), el siguiente modo que se abra relee EDIT + base.
  - Comprobado con capstone que los bytes son esas 3 instrucciones (rel → 0x37F5C39). Bytes originales comprobados antes de escribir. Simulación OK.
- Instalado (respaldo `phoenix.lua.v014`). **Phoenix-DB con Player_v90** (el juego tiene v95).
- **Plan:** Shift+R → U → Partido → Datos Actual. en vivo → Activar → ¿sin error? ¿interruptor = 1? → volver al menú → Partido → ¿Lamine 90?

### 2026-10-09 05:41 · v0.15, intento 1 → ❌ error de Lua en la tecla U (sin escritura, sin daño)
- `sider.log`: «attempt to call upvalue 'VP' (a nil value)». Después de Shift+R, `parchear()` devolvía «YA activa» (el byte de la P seguía puesto desde la v0.14) **sin preparar VirtualProtect**, y `escribirBytes` lo llamaba vacío. Sider atrapó el error y **no se escribió nada**.
- **Error de Claude:** la simulación siempre empezaba con el parche de la P sin poner, así que no cubría el caso «módulo recargado con el parche ya activo».
- **Arreglo v0.15b:** `prepararVP()` se llama dentro de `escribirBytes` y de `parchear`. Nombre FFI con versión (`phx15b_VP`). Simulación OK. Instalado.
- **Hallazgo colateral (05:40:08):** el juego hizo una relectura de la **lista tipo 2** (boots, glove, player, country, team, playerassignment, coach, competition, competitionregulation, competitionentry, stadium) con la huella de **v90**. La causa más probable es que FRALEX tocara «Datos Actual. en vivo» sin el parche. Aun así, la ficha siguió en 95: esa relectura no aplica a la plantilla del amistoso. Falta confirmarlo.
- **05:45 · v0.15, intento 2:** el overlay seguía en «v0.15-prueba» y el log repetía el error de VP. **El archivo arreglado nunca llegó al PC:** al reenviarlo con la misma ruta de salida (`outputs/v015/`), el puente dejó la versión vieja (sha 49c3…). **Error de Claude: no se verificó el sha en el PC después de copiar.** Se reenvió desde una ruta nueva (`outputs/v015b/`) y ahora el sha en el PC es el de v0.15b (3867…).
  - **Regla nueva:** después de cada `device_commit_files`, comprobar el sha256 en el PC.
- Sin el parche, el botón nativo hizo lo de siempre: «Los servicios en línea de este título finalizaron… 25/08/2022» y luego «No se han podido implementar los Datos de Actualización en vivo». Lamine siguió en 95.

### 🏆🏆🏆 2026-10-09 05:46 · EL BOTÓN NATIVO «Datos Actual. en vivo» CARGA NUESTROS DATOS — CONSEGUIDO
- v0.15b de verdad cargado (log: «v0.15b-prueba listo»). 05:46:22: U → «parche botón nativo aplicado en exe+20AF73B (19 B)».
- FRALEX: Partido → **Datos Actual. en vivo → Activar** → menú → Partido.
- **Nadie pulsó L ni P:** el log no tiene ningún «pedir recarga». El interruptor lo encendió **el botón nativo del juego** (nuestro parche en el estado 1 del flujo común).
- 05:46:35: el juego consumió el interruptor y releyó TODA la base. Huella player.bin = **v90** (3958249664 / 1038543739).
- Ficha de Lamine: **Velocidad 90** (antes 95). ✅
- **Resultado:** la opción nativa de Konami, apagada desde 2022, vuelve a funcionar y ahora carga los datos de Phoenix.
- Falta confirmar con FRALEX si al pulsar Activar ya no salió ningún mensaje de error.
- **05:50 · FRALEX:** al pulsar Activar con v0.15b **no salió ningún mensaje**. Pide que salga el mensaje de éxito y que la recarga sea **ahí mismo**, sin volver al menú.

### 2026-10-09 05:55 · v0.16: botón nativo «en el sitio» (con mensaje de éxito) — INSTALADO, falta probar
- **LiveDataSetFlow**, estados ya mapeados:
  - 22 = diálogo «LiveDataSetDialog» con el mensaje **0xF90042** (el error del flujo común era 0xF90043);
  - 23 = espera ≥ 2 s;
  - **26 = «editLoadDataInLiveDataSet»** (la recarga en el sitio);
  - 36/37 = avisar al flujo común (0x10A0001) y terminar.
- **Constructor** `0x20AB7C0`: `[+0x90]` = modo (Partido = 2) y `[+0x22C]` = 5.º argumento (Partido = 0). Así el estado 26 **sí** hace la recarga, porque solo se desvía si `[+0x22C] = 2`.
- **Parches v0.16 (tecla U, en memoria):**
  - **A** `0x20AF73B`: `mov [rdi+0x94], 3 ; jmp 0x20AF7BA` (sin inicio de sesión → crear LiveDataSetFlow). Acepta el original o el parche de la v0.15.
  - **B** `0x20AC6B9`, estado 5: `mov [rdi+0x94], 0x16 ; jmp 0x20AEC82` (sin internet → diálogo de éxito).
  - **C** `0x20AE664`: `mov word [rsp+0x30], 0x100 → 0x101` (la recarga del estado 26 incluye la base).
  - Se comprueban los tres antes de escribir; si uno no cuadra, no se escribe ninguno. Decodificados con capstone. Simulación OK.
- **Riesgo:** los estados 0–4 (dos objetos de espera y la comparación de versión DLC) se ejecutan tal cual y aún no se han estudiado. Pueden pedir algo de red o dar «versión antigua».
- Instalado (sha verificado en el PC; respaldo `phoenix.lua.v015b`). Phoenix-DB con **v99** (el juego tiene v90).

### 🏆🏆🏆🏆 2026-10-09 05:53 · BOTÓN NATIVO EN EL SITIO, CON MENSAJE NATIVO — CONSEGUIDO (v0.16)
- 05:52:44: U → parches A, B y C aplicados. FRALEX: **Partido → Datos Actual. en vivo → Activar**.
- **Sin inicio de sesión ni error.** Apareció el **diálogo nativo** de la actualización en vivo: «No hay cambios de la Actualización en vivo en la Valorac. de forma física…», con las 5 flechas de forma.
- El juego releyó **toda la base dos veces** (huella **v99** en las dos: 3611839843 / 3265817782) y aplicó los datos **sin salir de la pantalla**. Lamine: **Velocidad 99** (antes 90).
- **Resultado:** el botón de Konami vuelve a funcionar como en 2021: pulsas Activar → mensaje → datos nuevos al momento. Sin teclas de recarga, sin Editar, sin volver al menú.
- **Para revisar:** la plantilla del Atlético en esa pantalla cambió (aparecen José María Giménez, Matteo Ruggeri y Nico González). Posible causa: en modo «Datos en vivo» el juego toma las plantillas de la **base** (PlayerAssignment de olmosjr23) en vez de las del option file. Hay que confirmarlo antes de usarlo para fichajes.
- El mensaje habla de la «forma física» (PlayerWeekly). En el futuro, sirviendo nuestro `PlayerWeekly.bin` se podría mostrar la forma de Phoenix (Phoenix Weekly).

### 2026-10-09 06:05 · v0.17: el botón nativo se conecta SOLO al arrancar (sin tecla U) — INSTALADO, falta probar
- La primera vez que el juego lee la base (`livecpk_read` de un `pesdb\*.bin`, durante el arranque), `phoenix.lua` aplica los parches A/B/C con su comprobación de bytes. Si ya estaban puestos, no reescribe nada. La tecla U sigue funcionando como respaldo.
- Simulación OK (arranque nuevo → la primera lectura aplica; la segunda no repite). Instalado, sha verificado en el PC (d85e…). Respaldo `phoenix.lua.v016`.
- **Plan:** cerrar el juego → abrir → el overlay debe mostrar «[BOTÓN] … EN EL SITIO ✓» sin pulsar nada → Partido → Datos Actual. en vivo → Activar → mensaje nativo + datos.
- Prompt para el chat LINK: `prompts/PROMPT-LINK-boton-nativo.md` (colocar las entregas de Sync y avisar «pulsa Datos Actual. en vivo»).

### 2026-10-09 06:25 · Prueba de ida y vuelta: Lamine al Manchester City → «Datos Actual. en vivo» lo devuelve al Barça
- Option file actual = original (Lamine en el Barça, sha cb6e176e…), respaldado en `_PhoenixMercado_prueba\db\EDIT00000000.barca-original`.
- `mover_of` (OptionFile::mover): Lamine 108 → **173 (Manchester City FC)**, **dorsal 7** (el 10 está ocupado en el City); en el once del Barça entra Roony Bardghji. 95 bytes cambiados. sha ccb8e3e1… (copia en `db\EDIT00000000.lamineCity`).
- **Paso 1:** City puesto en `save\EDIT00000000`. FRALEX: Editar → Cargar → comprobar a Lamine en el City.
- **Paso 2 (cuando confirme):** volver a poner el original en disco → FRALEX pulsa Activar → Lamine debe volver al Barça.
- **06:28 · nueva petición de FRALEX:** Julián Álvarez (ID 126624, Atlético 172, dorsal 19) **al Barça**, y que entre al pulsar «Datos Actual. en vivo».
  - Preparado desde el original (Lamine en el Barça): Julián 172 → 108 con el **19** (libre en el Barça); en el once del Atlético entra Alexander Sørloth. sha 96cdf0d9… en `db\EDIT00000000.julianBarca`.
  - En disco sigue el del City (ccb8e3e1…) hasta que FRALEX confirme que ya lo cargó. Entonces se coloca el de Julián y FRALEX pulsa Activar ⇒ **Lamine vuelve al Barça y llega Julián**, las dos cosas con un solo clic.
- **06:41:** colocado en disco el option file de Julián + Lamine en el Barça (sha 96cdf0d9…). Falta que FRALEX pulse Activar.

### 2026-10-09 06:54 · Julián NO llegó al Barça con «Datos Actual. en vivo» → hallazgo: en el modo vivo las plantillas salen de la BASE
- 06:53:53 y 06:53:56: Activar → el juego releyó `playerassignment.bin` + `player.bin` (Phoenix-DB). En pantalla, Julián sigue en el Atlético.
- **Pista:** las plantillas en pantalla son las de la **base** (`PlayerAssignment.bin` de olmosjr23), no las del option file. En el Barça aparecen Kochen y J. Caicedo; en el Atlético, Giménez, Ruggeri y Nico González (lo mismo que ya vimos en la prueba de las 05:53).
- ⇒ **Con la actualización en vivo activada, el juego usa las plantillas de la base (como hacía Konami con sus datos en vivo).** El option file solo manda con la opción desactivada (Editar → Cargar, prueba 04:47).
- **Arreglo:** `PlayerAssignment.bin` propio en Phoenix-DB, con Julián 172 → 108, dorsal 19 (byte 18) y orden 24 (último del Barça). sha 7fb34a54…, en las dos carpetas Phoenix-DB (copia en `db\PlayerAssignment_julianBarca.bin`).
- **Siguiente:** Activar otra vez → ¿Julián en el Barça?

### 2026-10-09 ~19:43 · CONFIRMADO: «Activar» (v0.17, en el sitio) NO aplica las plantillas del option file
- FRALEX: option file en disco con Lamine en el Real Madrid (juego abierto) → Partido → Datos Actual. en vivo → Activar → **Lamine no apareció en el Real Madrid**.
- Coincide con la prueba de Julián (06:54). Con la recarga «en el sitio» (estado 26, `editLoadDataInLiveDataSet`) mandan las plantillas de la BASE. En cambio, la recarga al volver al menú principal (interruptor exe+0x37F5C39, tecla L) SÍ aplicó fichajes del option file (prueba 05:30).
- **Siguiente:** tecla L → menú principal → Partido, para ver si el option file manda aun después de haber usado «Activar». Si sale bien, la v0.18 hará las dos cosas: el mensaje nativo y, además, encender el interruptor.
- **19:45 · Julián vuelve al Atlético:** option file = original (sha cb6e176e…, Lamine en el Barça, Julián en el Atlético) y PlayerAssignment de Phoenix-DB = original (d1cf73c6…) en las dos carpetas. FRALEX aplica con Editar → Cargar.
- **19:50 · Mbappé (110718) al Barça:** OptionFile::mover 109 → 108, **dorsal 19** (libre en el Barça); en el once del Madrid entra Carlos Espí. Sigue en Francia (selección, equipo 8). sha 31851a25… (copia en `db\EDIT00000000.mbappeBarca`). FRALEX aplica con Editar → Cargar.
- **20:00 · Mbappé al Barça en los dos sitios:** dorsal **28** (libre tanto en la base como en el option file; el 19 estaba ocupado en la base). PlayerAssignment de Phoenix-DB (1775e688…) en las dos carpetas + option file (3507ce35…). Copias en `db\PlayerAssignment_mbappeBarca.bin` y `db\EDIT00000000.mbappeBarca28`.
- **20:06 · Prueba de fuente (Vinícius):** Vinícius (117047) Real Madrid → Barça, dorsal **29**, **solo** en el PlayerAssignment de Phoenix-DB (1d27f010…, con Mbappé incluido), en las dos carpetas. El option file NO se tocó (sigue 3507ce35…: Mbappé sí, Vinícius no). Si en el juego aparece Vinícius en el Barça ⇒ las plantillas que se ven salen de la base; si no aparece ⇒ salen del option file.
- **20:08 · FRALEX: «funcionó»** → Vinícius aparece en el Barça. Con la configuración actual del juego, las plantillas en pantalla salen de la **base** (PlayerAssignment de Phoenix-DB), no del option file.

### ✅ 2026-10-09 ~20:12 · CONFIRMADO: con «Activar», los fichajes entran por la BASE (PlayerAssignment.bin)
- Vinícius → Barça (dorsal 29) **solo** en `PlayerAssignment.bin` de Phoenix-DB (las dos carpetas). Option file sin tocar (sin Vinícius). Con «Datos Actual. en vivo → Activar» **Vinícius apareció en el Barça** ✅ (Mbappé ya estaba de antes).
- ⇒ Para fichar con Activar, Sync debe escribir cada fichaje también en `PlayerAssignment.bin` (siempre a partir del de olmos), y Link debe aceptar ese archivo en sus entregas.

**2026-10-09 20:58 · Análisis (sin juego) · Liga Máster: agentes libres, fichas 0xdb65 y el blob** — 🔎 + corrección ✅ en pruebas automáticas
Con 6 guardados ya abiertos: el blob es **solo** el arreglo de fichas (30.000 plazas); el calendario está fuera. Los agentes libres
se reconocen por un bit de su ficha (94 de Konami + Stones). Las fichas de los jugadores `0xdb65…` estaban justo después de las del
parche: el programa no las encontraba y ya está corregido (0 jugadores sin ficha en 4 guardados reales; 211/211 sin archivos y
225/225 con archivos). Detalle en ESTRUCTURA-ML §21. Faltan 3 pruebas en el juego (ver §21).

**2026-10-09 21:11 · Pruebas automáticas · Blob encontrado «por su forma» (multiparche)** — ✅ (sin juego)
El programa buscaba el blob en una posición fija del ConmeGOL 26. Ahora, si no está ahí, lo busca por su cabecera y lo valida
entero. Prueba: se metieron 64 bytes de relleno antes del blob en 4 guardados reales (como si fuera otro parche) → lo encontró,
lo leyó igual, lo reescribió y lo volvió a leer bien. Resultados: 211/211 sin archivos; 230/230 (r0, jA, fD) y 221/221 (g19) con
archivos. Compila también para Windows (MinGW). ⏳ Falta probarlo con un guardado real de otro parche.

**2026-10-09 21:15 · Análisis (sin juego) · Liga Máster: 🏆 calendario de partidos encontrado** — 🔎 (falta verlo en pantalla)
Cada partido tiene un número global y su registro guarda local, visitante, competición y jornada. Comprobado en dos carreras:
City (ConmeGOL, 4/8–22/9) y Barça (ranura 1 del PC, 4/3/2026). Barça: jornada 1 Barça – Real Sociedad (en casa), jornada 2
Deportivo – Barça, jornada 10 Real Madrid – Barça. City: jornada 1 City – Crystal Palace, jornada 2 Everton – City. Los
resultados todavía no aparecen (no van en el registro del partido). **Qué mirar en el juego:** carrera del Barça → Calendario
de LaLiga: ¿coinciden esas jornadas? Detalle: ESTRUCTURA-ML §22.

**2026-10-09 21:19 · Análisis (sin juego) · Liga Máster: 🏆 tablas de posiciones descifradas** — 🔎 (falta verlo en pantalla)
Cada liga guarda su tabla (puesto, puntos, G-E-P, goles a favor y en contra, jugados, goles y victorias de visitante) y la de la
jornada anterior. Comprobado: goles a favor = goles en contra en todas las tablas, y los 10 partidos de la jornada 1 de la Premier
(carrera del City, 22/9) cuadran 10 de 10 con el calendario (City 3–0 Crystal Palace, Hull 4–1 Man United…). **Qué mirar en el
juego:** carrera del Barça (ranura 1) → tabla de la liga francesa: Lille 1º 22 pts (8 PJ, 7-1-0, 19–4), PSG 20, Lens 19,
Lorient 16. Detalle: ESTRUCTURA-ML §23.

**2026-10-09 23:01 · Pruebas automáticas · Lector de temporada + goleadores + prefijo por carrera** — ✅ (sin juego)
Nuevo módulo de solo lectura (TemporadaLM): calendario, tablas y goleadores. Con 6 guardados reales: todos los partidos de la
jornada 1 cuadran con su tabla (35/35, 152/152, 38/38) y los goleadores de la Premier suman los 27 goles de la jornada, club por
club. **Error encontrado y corregido:** en la carrera del Barça los jugadores añadidos usan otro prefijo (0xdbdf en vez de 0xdb65) y
el programa no encontraba 1.156 fichas; ahora 0 sin ficha en los 6 guardados. 211/211 sin archivos; 225–234 con cada guardado.
Compila para Windows (MinGW). Detalle: ESTRUCTURA-ML §24.

### 2026-10-09 21:09–22:44 · Fase A: ¿«Datos Actual. en vivo» se guarda en disco? → ❌ NO (solo lectura, sin tocar nada)
- **Método:** «fotos» (sha256 de cada archivo) de `…\239200\save`, de las otras carpetas de guardado y de `settings.dat`, comparadas entre sí. Guion: `herramientas/exe/foto_guardado.sh`.
- **Fotos:** 21:09 inicial · 22:02 juego cerrado · 22:10 juego abierto (arrancó 22:07) · 22:23 tras Activar (22:22) · 22:31 · 22:32 juego cerrado (cierre 22:31:48) · 22:39 abierto de nuevo + Desactivar · 22:41 tras Activar · 22:44 tras Desactivar.
- **Resultado:** las 9 fotos son idénticas. Abrir el juego, Activar, Desactivar y cerrar **no escriben ningún archivo de guardado**. En la carpeta del juego solo cambian los diarios de Sider.
- **Al reiniciar, la opción sale en «Desactivar»** aunque se pulsara Activar antes de cerrar (22:39, visto una vez).
- **Desactivar, estando ya en «Desactivar», no recarga nada** (22:44: ninguna lectura nueva de la base).
- **Hallazgo de FRALEX (capturas 22:39 y 22:41):** el juego muestra el estado en «Selección actual: …» y tiene tres posiciones (Activar, Desactivar, Valoraciones generales uniformes). **Después de pulsar Activar con la v0.17, sigue diciendo «Selección actual: Desactivar».**
- Aviso: la carpeta `C:\dev\smash-soda-fork` del PC estaba atrasada respecto a GitHub. No se hizo `git pull`; se leyó de una copia temporal.

### 2026-10-09 22:45–22:55 · Estudio del exe (copia de solo lectura): candidato para la opción
- `0x14B6A60` devuelve el **gestor de la base** (`[exe+0x3705E10]`). `0x14B7560(gestor, bandera, modo)` recarga la base y guarda el **modo** en `[gestor+0x38]`. El juego compara ese número con 1 en 12 sitios (entre ellos `ProcessLiveDataCheck`, `0x13040CE`).
- Activar con la v0.17 = dos recargas: estado 22 (`0x20AE529`, modo **1**) y luego la carga del EDIT con el parche C (`0x1EFAFB0`, modo **0**). Explica las dos lecturas de la base que ve el espía y que la pantalla diga «Desactivar».
- Desactivar = `LiveDataRemoveFlow` (`0x20AF320`): tarea 0x1B y `0x14B6F20(gestor, 0)`.
- Detalle completo: `base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md`. Ayudantes: `herramientas/exe/lib_exe.py`.

### 2026-10-09 22:57 · phoenix.lua v0.17m (prueba «mirar la opción», SOLO LECTURA) — INSTALADA con el OK de FRALEX
- Es la v0.17 más: tecla **M** (muestra al log), línea **[MODO]** en el overlay y una muestra automática al empezar cada lectura de `player.bin`. No escribe en la memoria. Comprueba antes 3 trozos de código (`0x14B6A60`, `0x13040C9`, `0x14B7587`); si no coinciden, no lee. Usa copia segura a un búfer propio. No cambia los parches A/B/C.
- Simulación LuaJIT (lupa) con memoria falsa: 6 casos (normal 0→1→2, exe distinto, gestor inexistente, zona ilegible/puntero raro, lecturas durante Activar, módulo recargado): 6/6 OK, 0 escrituras. `sider/pruebas/simular_v017m.py`.
- **Antes de instalar:** la v0.17 (sha d85e1071…) verificada en 6 sitios; respaldo nuevo `phoenix.lua.v017` en las dos carpetas `modules`; juego cerrado (Sider: «All done» 22:57:04).
- **Instalación:** copiada a las dos carpetas `modules`; sha256 en el PC = 57ff5ec0…52805 en las dos; respaldos intactos.

### ✅ 2026-10-09 23:00–23:04 · ENCONTRADO: la opción es el modo de carga del gestor de la base (`[[exe+0x3705E10]+0x38]`), 1 = activada
- Sesión que arrancó a las 23:00:09. Muestras automáticas (lectura de `player.bin` → modo):
  - n.º 1 23:00:09 (arranque) → **0** · n.º 2 23:01:54 (segunda pasada del arranque) → **0**
  - n.º 3 23:02:06 (Activar) → **1** · n.º 4 23:02:09 → **0**
  - n.º 5 23:04:06 (Activar) → **1** · n.º 6 23:04:09 → **0**
  - n.º 7 23:04:18 (Activar) → **1** · n.º 8 23:04:21 → **0**
- **3 de 3 repeticiones iguales**, sin errores de Lua, juego normal. Coincide con lo deducido del código.
- Overlay a las 23:04:30 (captura de FRALEX): «[MODO] opción en vivo: DESACTIVADA (0) · gestor estado 4 · +90..93 = 0 1 0 0», con el juego mostrando «Selección actual: Desactivar».
- **Conclusión:** con la v0.17 la opción vale 1 solo unos 3 segundos por cada Activar; el parche C la devuelve a 0. Leerla sola no basta para saber «estoy en modo vivo».
- **Sin probar todavía:** el valor 2 (¿Valoraciones generales uniformes?), ver «Selección actual: Activar» en pantalla (haría falta que el modo se quede en 1) y la tecla M.
- **Siguiente (decidido por FRALEX a las 23:07):** primero la opción A (phoenix.lua recuerda la última recarga), luego el experimento B (sin el parche C).
**2026-10-09 23:25 · Prueba en el juego · ¿Las stats de Phoenix-DB llegan a una Liga Máster ya empezada (tras reiniciar el juego)?** — ❌ NO
Phoenix-DB con `Player.bin` v99 (Lamine Velocidad 99) desde las 5:51; juego abierto a las 23:00. Capturas de Fralex:
**amistoso → Lamine Velocidad 99, media 90** ✅ · **Liga Máster (Lamine en el Barça) → Velocidad 90, media 88**, Pase raso 83
(base 82), Contacto físico 77 (base 76) ❌. Conclusión: cada carrera usa **su propia copia** de las habilidades, que además evoluciona.
Investigado sin éxito (todo anotado en ESTRUCTURA-ML §25): no hay copia de su registro de `Player.bin` en el guardado; ni en sus
fichas de tu equipo (A–M), ni en su ficha de 596 B, ni empaquetadas en 5–8 bits. El juego, al entrar a la LM, lee
`installversionplayer.bin` (23:17 y 23:21; viene de Konami, `dt80_700E_x64.cpk`, 13.223 B): son 7.426 parejas
«jugador → versión» (sin habilidades; Lamine no está). **Siguiente:** experimento de dos carreras nuevas (v99 y v90) para comparar.

### 2026-10-09 23:31 · phoenix.lua v0.17a (opción A: «recordar la última recarga», SOLO LECTURA) — INSTALADA con el OK de FRALEX, falta probar
- Decisión de FRALEX (23:07): primero la opción A, luego el experimento B (sin el parche C).
- **Qué añade a la v0.17m:** al empezar cada lectura de `player.bin` mira el modo y apunta el tipo de recarga: modo 1 → **ACTIVAR**; modo 0 sin un 1 en los últimos 15 s → **NORMAL**; modo 0 dentro de esos 15 s → no cambia (2.ª recarga del mismo Activar). Línea nueva en el overlay: **[ÚLTIMA RECARGA]**. Log: `[phoenix] última recarga: …`. No escribe en la memoria ni cambia los parches A/B/C.
- **Simulación LuaJIT con reloj falso:** 5 casos nuevos (arranque + Activar ×2; Activar → Editar/Cargar → 2.ª recarga lenta; exe distinto; Shift+R; modo 2, reloj hacia atrás y gestor inexistente) + los 6 de la v0.17m: todos bien, 0 escrituras. `sider/pruebas/simular_v017a.py`.
- **Instalación:** juego cerrado (Sider «All done» 23:31:02); respaldo `phoenix.lua.v017m` en las dos carpetas `modules`; v0.17a copiada a las dos; sha256 en el PC = 379dce7b…3306c en las dos; respaldos `v017` (d85e1071…) y `v017m` (57ff5ec0…) intactos.
- **Límites:** no ve recargas que no releen la base (tecla L, «Ser una Leyenda»); tras Shift+R empieza sin memoria.
- **Plan de prueba:** Activar → overlay «ACTIVAR → equipos de la base»; Editar → Cargar (sin Guardar) → «NORMAL → equipos del option file». Comprobación con Vinícius si los archivos siguen como a las 20:08 (Barça con Activar, Real Madrid con Editar → Cargar).
- **Corrección a la entrada de las 22:45:** el juego lee `[gestor+0x38]` en 12 sitios y en 11 de ellos lo compara con 1 (no en los 12).
- Informe paso a paso de toda la sesión: `base-conocimiento/19-OPCION-EN-VIVO-PASO-A-PASO.md` (archivo nuevo).

### ✅ 2026-10-09 23:34–23:37 · v0.17a (opción A) PROBADA en el juego: reconoce ACTIVAR y vuelve a NORMAL
- Sesión que arrancó a las 23:34:11 (log: «v0.17a listo»). Sin errores de Lua.
- 23:34:11 lectura n.º 1 (arranque), modo 0 → «última recarga: NORMAL». 23:34:39 lectura n.º 2, modo 0 → NORMAL.
- **23:35:00 Activar**, lectura n.º 3, modo **1** → «última recarga: **ACTIVAR**». 23:35:03 lectura n.º 4, modo 0 → **no cambia** (2.ª recarga del mismo Activar, dentro de los 15 s).
- Captura de FRALEX a las 23:35:38: «[ÚLTIMA RECARGA] ACTIVAR → equipos de la base · 23:35:00 · ¿fue Activar? true», con «[MODO] DESACTIVADA (0)».
- **23:36:33** lectura n.º 5, modo 0 → «última recarga: **NORMAL**». FRALEX había entrado a Editar. Captura a las 23:36:46 (ya en el menú principal): «[ÚLTIMA RECARGA] NORMAL → equipos del option file · 23:36:33 · ¿fue Activar? false».
- **Susto sin daño:** FRALEX avisó de que en Editar el cursor se fue sin querer a «Guardar». Foto de las 23:37: `EDIT00000000` idéntico (sha 3507ce35…, misma fecha 19:59:59) y el resto de la carpeta de guardado igual. No se guardó nada.
- **Sin comprobar todavía:** que la etiqueta coincide con las plantillas en pantalla (Vinícius en el Barça tras Activar y en el Real Madrid tras la recarga normal), y Editar → Cargar pulsado a propósito.


**2026-10-09 23:38 · Prueba en el juego · Carrera del Barça (ranura 1, 4/3/2026): calendario, tabla y goleadores leídos del archivo** — ✅ PROBADO
Capturas de Fralex comparadas con lo que leyó el programa del archivo:
- **Goleadores Ligue 1:** Igamane (Lille), Édouard (Lens), Avom (Lorient) 8 · Diop (Niza), Panichelli (Estrasburgo) 5 → **idéntico**.
- **Tabla Ligue 1:** Lille 22 (7-1-0, 19–4), PSG 20 (6-2-0, 17–4), Lens 19 (6-1-1, 20–8), Lorient 16 (5-1-2, 19–11), Marsella 14
  (4-2-2, 11–8), Mónaco 14 (4-2-2, 9–6) → **idéntico**. Las flechas (Marsella y Mónaco suben; Lyon y Niza bajan) cuadran con la
  segunda tabla guardada (la de la jornada anterior).
- **LaLiga sin empezar:** todos con 0, en el mismo orden que el archivo (Barça, Real Sociedad, Deportivo, Celta…) → **idéntico**.
- **Calendario:** próximo partido de LaLiga Barça – Real Sociedad (11/4/2026, en casa) = jornada 1 leída del archivo → **idéntico**.
ESTRUCTURA-ML §22–§24 pasan de 🔎 a ✅.
