# 09 · Cómo se logró el control de la Liga Máster, paso a paso (qué se hizo, cómo y con qué herramienta)

**Para FRALEX · 9 de octubre de 2026.** Este informe cuenta el **camino real** del trabajo del 8 de octubre: cada paso, la herramienta usada, qué falló y cómo se arregló.
El resumen y la guía de uso están en `08-LIGA-MASTER-INFORME.md`. El detalle de bytes está en `liga-master/ESTRUCTURA-ML.md` (§1–§20). El diario de pruebas está en `../PRUEBAS.md` (Parte 2).

---

## 0. Palabras que se usan aquí

| Palabra | Qué significa |
|---|---|
| **Guardado** o **ranura** | Una carrera de Liga Máster. La ranura N del menú Cargar es el archivo `ML0000000(N−1)`, contado en hexadecimal (la ranura 11 es `ML0000000A`). |
| **Cifrar / descifrar** | El juego guarda el archivo «revuelto». Descifrar = dejarlo legible. Cifrar = volver a revolverlo para que el juego lo acepte. |
| **Byte** | La unidad mínima de un archivo. Una «dirección» (por ejemplo `0xc7dd00`) es la posición de un byte dentro del archivo. |
| **reg** | El número fijo de un jugador **dentro de la carrera** (Haaland = 4340). No cambia aunque cambie de club. |
| **pid** | El ID del jugador en el option file y en la base (Haaland = 133543). |
| **Ancla** | Algo conocido que se busca para ubicarse (por ejemplo, los 5 primeros jugadores de tu plantilla). Sirve para encontrar las tablas sin depender de direcciones fijas. |
| **Comparar byte a byte** | Poner dos archivos lado a lado y listar **cada byte que cambió**. Es la herramienta principal de todo el trabajo. |
| **Huella (md5 / sha256)** | Un código que identifica un archivo exacto. Si cambia un solo byte, cambia la huella. |
| **Verdad del juego** | Un guardado que hizo **el propio juego** (por ejemplo, tú fichando o despidiendo a alguien). Se usa como modelo a copiar. |

---

## 1. Las herramientas que se usaron

| Herramienta | Qué es | Para qué se usó |
|---|---|---|
| **libpesXcrypter** (`terceros/pesxcrypter`) | Biblioteca libre en C que conoce el cifrado de PES 2021 | Base de todo: abrir y cerrar los guardados |
| **`dec`** (`liga-master/herramientas/dec.c`) | Programa propio en C | Descifra un guardado y **comprueba la ida y vuelta**: lo vuelve a cifrar y mira que salga idéntico byte a byte |
| **`info`** (`herramientas/info.c`) | Programa propio en C | Muestra cabecera, tamaños, nombre interno (128 B), texto info y serial |
| **`enc2`** (`herramientas/enc2.c`) | Programa propio en C | Cifra el archivo editado usando la cabecera del original y **cambia el texto que ves en el menú Cargar** (para reconocer cada ranura de prueba) |
| **Python 3** (`liga-master/prototipos/`) | Scripts de prueba rápida | Primeros cambios, comparaciones y lectura del blob (ver §2) |
| **Python `zlib`** | Descompresor estándar | Abrir el «blob» comprimido |
| **C++ (`core/LigaMaster.cpp`, `core/Alineacion.h`, `core/BlobLM.*`)** | El motor definitivo de Phoenix Sync | Hace todo **por anclas**, con comprobaciones, y sirve para cualquier carrera |
| **miniz** | Compresor zlib pequeño, incluido en el C++ | Volver a comprimir el blob después de editarlo |
| **`pruebas/main.cpp`** | Pruebas automáticas | Más de 290 comprobaciones que corren solas antes de cada entrega |
| **gcc / MinGW** | Compiladores | Compilar en la nube (Linux) y **cruzado para Windows** (`PROBAR2.bat` en tu PC) |
| **Enlace con tu PC** (el puente de la app de escritorio) | Copia archivos entre tu PC y la nube | Traer tus guardados a la nube y devolver las ranuras de prueba a tu carpeta `save` |
| **md5 / sha256** | Huellas | Saber si un archivo era el nuestro o si el juego ya había guardado encima |
| **Tú, en el juego** | La prueba final | Ninguna parte se dio por buena hasta que la viste bien en pantalla |

Los prototipos de Python, uno por uno:

| Script | Qué hace |
|---|---|
| `mlparse.py` | Lee los 700 bloques de equipo: nombre, ID option, plantilla `(reg, pid)`, dorsales y contador |
| `mlsquad.py` | Conoce las tablas de tu equipo (A…M, K, L) con las direcciones de **un** guardado |
| `build_c.py` | Primer «fichaje» completo: saca a un jugador de tu equipo y lo pone en un club de la IA |
| `build_v6.py` | `build_c` + corrección del orden de formación y de los 6 roles |
| `ref_v7_juego.py` | La misma lógica sobre otro guardado (ranura 7). Sirvió de **regla para medir** el C++ |
| `diff_gt.py` | Compara un guardado limpio con uno hecho por el juego y lista qué cambió y en qué tabla |
| `blob.py` | Descomprime los 18 tramos zlib del blob |

---

## 2. El método (siempre el mismo ciclo)

Cada parte se logró repitiendo estos 6 pasos:

1. **Traer a la nube** el guardado de tu PC (copiado; el original nunca se toca).
2. **Descifrar** con `dec` y comprobar que la ida y vuelta sale idéntica.
3. **Conseguir una «verdad del juego»:** tú hacías la operación real en el juego (fichar, despedir, jugar) y guardabas en **otra** ranura.
4. **Comparar byte a byte** el antes y el después del juego para ver **exactamente** qué toca el juego y dónde.
5. **Programar lo mismo** (primero en Python, después en C++) y **comparar nuestro resultado con el del juego** hasta que salga igual.
6. **Cifrar** con `enc2` (o con el C++), ponerle un texto visible en el menú Cargar, **anotar la huella**, dejarlo en una ranura libre de tu PC y **probarlo en el juego**. Se anota el resultado en `PRUEBAS.md`, salga bien o mal.

> Regla que nació aquí: **un cambio solo se da por bueno cuando lo ves en el juego**. Lo que dicen los archivos es «observado»; lo que ves en pantalla es «probado».

---

## 3. Paso a paso, en el orden en que pasó (8 de octubre)

### Paso 1 · Abrir el archivo
- **Qué:** entender el envoltorio del guardado.
- **Cómo:** se compiló `dec`, `info` y `enc2` con gcc contra libpesXcrypter. `dec` descifró `ML00000000` (19,7 MB legibles) y lo volvió a cifrar idéntico.
- **Descubrimiento clave:** la función de cifrado **no recalcula** la huella de la cabecera, y aun así **el juego acepta el archivo editado**.
- **Descubrimiento 2:** el menú Cargar **no muestra** el nombre interno de 128 B, sino el **texto info** (`Equipo / Liga · Fecha · Competición`). Por eso `enc2` cambia la primera línea de ese texto: así reconocías «PRUEBA 4», «PRUEBA 5»… en el menú.

### Paso 2 · Encontrar las plantillas de los 700 equipos
- **Cómo:** se compararon tus guardados antes y después de fichar a **Neymar**, y una zona con **246 traspasos que hizo la IA sola**.
- **Resultado:** 700 bloques de **1680 B** desde el byte `0x50`. Cada uno: nombre, ID del option file, lista de 40 `(reg, pid)`, 40 dorsales y un contador. City = bloque 154, Santos = 66, Real Madrid = 135.
- **Herramienta:** `mlparse.py`.

### Paso 3 · Primer traspaso entre dos equipos de la IA (ranura 3)
- **Qué:** Mbappé, Real Madrid → Santos.
- **Cómo:** solo se cambió lo que cambiaba en los 246 traspasos del juego: lista de plantilla (el que llega va **al final**), dorsales y contador.
- **Resultado:** ✅ Mbappé aparecía en el Santos.
- **Lo que no se miró:** la pantalla *Alineación*. Esto volvió a salir en el paso 8.

### Paso 4 · Quitar a un jugador de TU equipo (ranuras 1, 4 y 5)
- **Ranura 1 (v2):** solo la lista de plantilla de Haaland. ❌ Haaland salía «sin equipo» en la búsqueda, pero **seguía** en tu Estrategia y en la zona de ventas.
- **Por qué:** tu equipo guarda **muchas más tablas por jugador**. Comparando byte a byte se encontraron: tablas **A, B, C, D, E, F, G, H, M** (datos por jugador), la tabla **L** (al revés) y la lista **K** (la que lee Estrategia).
- **Ranura 5 (v5):** se tocaron todas. Haaland perfecto en el Santos, pero ❌ **Rulli (portero) aparecía en el ataque** de tu Estrategia.
- **Por qué:** la alineación y los 6 roles (capitán, lanzadores) guardan **números de puesto**. Al sacar a Haaland, todos se corrían un lugar.
- **Herramientas:** `mlsquad.py`, `build_c.py`, `diff_gt.py`.

### Paso 5 · Corregir la alineación y los roles (ranura 6)
- **Cómo:** `build_v6.py` recalcula el **orden de formación** y los **roles**; Neymar entra en el puesto de Haaland.
- **Resultado:** ✅ Estrategia bien, ningún portero arriba. Jugaste un partido y guardaste: ✅ el juego no corrigió nada.
- **Lección:** el juego **guardó encima** de la ranura 6. Desde aquí, **cada archivo entregado se identifica por su huella**, no por su nombre.

### Paso 6 · Pasar de Python a C++ «por anclas»
- **Problema:** las direcciones de Python solo valían para **un** guardado. El blob crece con la temporada (+16.792 B en la ranura 7) y **desplaza** las tablas que van detrás (I y J).
- **Cómo se resolvió:** en `core/LigaMaster.cpp`, la función `tablasDe` busca cada tabla por los **5 primeros `(reg, pid)`** de tu plantilla. El orden de formación y la lista K se reconocen por su **forma**, no por su dirección.
- **Comprobación:** el C++ (`moverUsuarioAIA`) dio **el mismo archivo byte a byte** que `build_v6.py` en el respaldo y que `ref_v7_juego.py` en la ranura 7.

### Paso 7 · Primera prueba del C++ (ranura 8): el «jugador en blanco»
- **Qué:** Guéhi, City → Real Madrid.
- **Resultado:** ✅ Equipos, búsqueda y tu Estrategia. ❌ En la **Alineación del Madrid** salía un jugador en blanco **«DC 0»**.
- **Por qué:** cada equipo de la IA tiene **su propia alineación**, guardada **aparte**.

### Paso 8 · Encontrar las alineaciones de la IA (ranura 9)
- **Cómo:** se buscó una serie de bloques numerados 0, 1, 2, 3… cada 600 B con el ID option de cada equipo. Así aparecieron **629 bloques de 600 B**: nombre del técnico, **orden de formación** en `+0x220` y **roles** en `+0x248`.
- **Comparando 431 equipos** que la IA había cambiado sola: si se va un **titular**, una reserva de su posición ocupa su lugar; si se va un suplente, los de atrás suben; el que llega va **al final**.
- **Programado en el C++:** `Alineacion.h` y `sugerirSustituto` (reservas primero; misma posición > misma línea > cualquiera; **un portero solo lo cubre otro portero**).
- **Prueba (ranura 9):** Guéhi al Madrid + Isak, Liverpool → Santos. ✅ Guéhi sin jugador en blanco, **Ekitiké** (elegido solo) en el puesto de Isak, Isak con el 99 en el Santos.
- **Hallazgo:** en esa partida la **banca es de 12**, no de 7. El C++ dejó de suponer tamaños.

### Paso 9 · ¿Aguanta un partido? (ranura 9 → ranura 10)
- **Cómo:** jugaste y guardaste en la 10. Se comparó con nuestra versión.
- **Resultado:** ✅ **todo se mantuvo**. El técnico de la IA reordenó a su gusto (Isak titular en el Santos), y hubo 332 fichajes de la IA (último día de mercado).
- **Problema encontrado:** al jugar aparece una «**ficha del último partido**» que se parece a una tabla de plantilla. El C++ la confundía y se negaba a vender («TABLA_DANADA»).
- **Arreglo:** una tabla de plantilla **debe tener a todos** tus jugadores; si no, se ignora.

### Paso 10 · La «verdad del juego» al despedir (ranura 10 → ranura 11)
- **Qué hiciste:** «Despedir» a Stones en el juego y guardar en la 11 (`ML0000000A`).
- **Qué se descubrió comparando:**
  - la tabla **I** son los **contratos** (registro de 48 B); un jugador puede tener **dos** (su contrato + una **oferta** de otro club);
  - existe una **lista de negociaciones abiertas** (60 B por registro);
  - el juego deja **huecos** en A, B, C, D, E, F, J y M, y **compacta** G, H y los contratos;
  - el **ID interno de club** = `(ID option << 14) | índice del bloque` (City = `0x2b409a`).
- **Programado y medido:** se reescribió la venta del C++ para hacer **exactamente** eso. Sobre la ranura 10 dio **lo mismo que la ranura 11 del juego**, tabla por tabla (solo cambian bytes de relleno que el juego limpia).

### Paso 11 · El dinero (ranura 12)
- **Cómo:** se compararon las cifras de la pantalla con los números del archivo (en céntimos ÷ 100). Bloque en `0xc7dd00`: presupuesto actual, presupuesto inicial (`+0x10`) y **tope salarial** (`+0x14`).
- **Fórmula comprobada:** presupuesto salarial en pantalla = **tope − suma de sueldos** de los contratos.
- **Por ancla:** ese bloque está siempre a `0x97d38` de tu tabla A (comprobado en 6 guardados). Funciones `GuardadoLM::finanzas` y `fijarFinanzas` (rechazan un tope menor que los sueldos).
- **Prueba (ranura 12):** presupuesto **500.000.000 €** y tope 200 M → en pantalla, presupuesto salarial **86.096.000 €**, la cifra exacta calculada. ✅
- **Además:** los clubes de la IA **no tienen dinero guardado**: fichan por reglas.

### Paso 12 · Abrir el «blob»
- **Qué era:** 1,2 MB que parecían ruido.
- **Cómo:** se buscó la firma de zlib (`78 9c`) y aparecieron **18 tramos** de 256 KB separados por 12 B. `blob.py` los descomprime: **4,68 MB**.
- **Dentro:** una **ficha de Liga Máster de 156 B por jugador** (16.422) en `0x1e + 156·reg`: sueldo (`+0x56`), **valor de mercado** (`+0x5a`), contrato, competiciones inscritas. Comprobado: Rúben Dias 55.000.000 € = pantalla.
- **Escribir:** `core/BlobLM` lo lee y lo vuelve a comprimir con **miniz** (solo el tramo cambiado), y corrige las palabras de tamaño. Ida y vuelta idéntica en 4 guardados.
- **Prueba 17 (ranura 1):** valor de mercado de Sommer 2,5 M → **77,7 M**. ✅ en pantalla. El juego acepta el blob recomprimido.

### Paso 13 · La «verdad del juego» al fichar (prueba 16)
- **Qué hiciste:** fichar a **Sommer** (Inter → City) en el juego y guardar en la 15.
- **Qué se descubrió:** el juego crea un registro en el **primer hueco** de cada tabla, con datos del jugador: fecha de llegada (A), **curva de medias por edad** (E), datos de desarrollo (D), bloque grande (M), contrato, historial, dinero y la ficha del blob.

### Paso 14 · Fichar PARA tu equipo desde el programa (pruebas 18 y 19)
- **Cómo (`GuardadoLM::ficharParaUsuario`):** se toma como **molde** a un compañero de la misma posición, se crea el registro en el primer hueco de cada tabla, se **genera** la curva E, se completa la ficha del blob (competiciones, sueldo, fechas), se pone al final del orden y de K, se descuenta el dinero y se pone un sustituto en el club de origen.
- **Comparado con el fichaje real de Sommer:** iguales plantillas, orden, K, dinero, tabla A, contrato y ficha del blob.
- **Prueba 18:** ⚠️ en pantalla todo bien y **jugó de titular**, pero al autoguardar **el juego creó su propio contrato** y el nuestro quedó **duplicado** (los sueldos superaban el tope).
- **Arreglo:** el programa **ya no escribe el contrato**; deja la ficha del blob y el juego crea el contrato solo.
- **Prueba 19:** ✅ partido jugado, **un solo contrato**, inscrito en las 6 competiciones, presupuesto salarial positivo.
- **Extra:** el juego **relee** el archivo al cargar la ranura. Se puede cambiar un guardado con el juego abierto, **mientras esa ranura no esté cargada**.

### Paso 15 · Unirlo con la web
- **Cómo:** la sincronización con la web llama a `ficharParaUsuario` con las condiciones opcionales de la web (monto, sueldo, cláusula, fin de contrato, dorsal).
- **Comprobado:** da **exactamente los mismos bytes** que el fichaje validado en la prueba 19. Pruebas automáticas: **295 de 295 ✅**.
- **En tu PC** (`PROBAR2.bat`, compilado con MinGW): conexión con la web OK y firma Ed25519 verificada.

---

## 4. Tabla de todas las pruebas en el juego

| Ranura / prueba | Qué se probó | Resultado |
|---|---|---|
| 1 (v2) | Haaland fuera del City, solo la lista | ❌ seguía en tu Estrategia |
| 2 | Copia limpia de control | ✅ |
| 3 | Mbappé Madrid → Santos (IA ↔ IA) | ✅ |
| 4 | Haaland → Santos, solo listas | ✅ a medias |
| 5 (v5) | Haaland → Santos con todas las tablas | ❌ portero en el ataque |
| 6 (v6) | + alineación y roles corregidos | ✅ y aguantó un partido |
| 8 (v7, C++) | Guéhi → Madrid | ❌ «DC 0» en la alineación del Madrid |
| 9 (v8) | + alineaciones de la IA + Isak → Santos | ✅ y aguantó un partido (ranura 10) |
| 11 | Tú despides a Stones (verdad del juego) | Modelo para la venta |
| 12 | Presupuesto 500 M y tope 200 M | ✅ |
| 13 (prueba 14) | Bettinelli City → Lanús con la venta «igual que el juego» | ✅ |
| 15 (prueba 16) | Tú fichas a Sommer (verdad del juego) | Modelo para el fichaje |
| 1 (prueba 17) | Valor de mercado de Sommer 77,7 M (blob recomprimido) | ✅ |
| 2 (prueba 18) | El programa ficha a Sommer | ⚠️ contrato duplicado |
| 2 → 3 (prueba 19) | Fichaje sin escribir el contrato | ✅ un solo contrato, jugó |

---

## 5. Errores que enseñaron algo (y cómo se arreglaron)

| Error | Causa | Arreglo |
|---|---|---|
| Jugador «sin equipo» pero vivo en tu Estrategia | Solo se tocó la lista del club | Tocar las 12 tablas de tu equipo y la lista K |
| Portero en el ataque | Los puestos de la alineación se corrieron | Recalcular orden y roles con un sustituto |
| Jugador en blanco «DC 0» | La alineación de la IA está aparte | Editar el bloque de 600 B de cada club |
| El programa se negaba a vender | Confundía la «ficha del último partido» con una tabla | Una tabla válida debe tener a todos |
| Contrato «fantasma» | Solo se borraba el primer contrato | Borrar todos los contratos, ofertas y negociaciones |
| Contrato duplicado | El juego crea el suyo desde la ficha del blob | No escribir el contrato |
| Ranura sobrescrita | El juego autoguarda la ranura cargada | Anotar la huella de cada entrega y guardar en otra ranura |
| Direcciones que dejan de servir | El blob crece y desplaza las tablas | Buscar todo por anclas |

---

## 6. Qué NO se usó (para que quede claro)

- No se tocó el ejecutable del juego para esta parte. Todo es **edición del archivo de guardado**, como hace un editor de partidas.
- No se usaron programas de terceros de edición de Liga Máster: el mapa se sacó comparando tus guardados.
- Los guardados y los `.bin` descifrados **no se suben a GitHub** (son tus datos).

---

## 7. Lo que falta (igual que en el 08)

1. Stats dentro de una carrera ya empezada (prueba de reiniciar el juego con un `Player.bin` nuevo).
2. Agentes libres.
3. Calendario de partidos (pista: 10 números de partido por jornada; falta guardar antes y después de un partido el mismo día).
4. Que un traspaso aprobado en la web se aplique solo a la carrera elegida, con aviso de Phoenix Link.
