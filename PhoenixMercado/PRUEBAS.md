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
