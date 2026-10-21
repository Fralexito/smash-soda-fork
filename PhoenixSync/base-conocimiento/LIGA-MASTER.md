# Base de conocimiento · Liga Máster: estructura, calendario, tablas, goleadores, stats y barras de crecimiento

Todo lo de la Liga Máster de PES 2021, del 9 al 10-oct-2026:
- su estructura completa;
- cómo se hizo;
- agentes libres, calendario, tablas y goleadores;
- cómo llegan las stats;
- verificación en pantalla;
- el experimento de dos carreras;
- las barras de crecimiento, desde la guía 23 hasta el informe maestro 29 (la versión final);
- los traspasos para otra IA.

**Para replicarlo empieza por** la parte «29 · Informe maestro» (barras) y la parte «20 · Traspaso Liga Máster v2».

**Referencia técnica aparte:** `PhoenixSync/liga-master/ESTRUCTURA-ML.md`, el cuaderno de la estructura del guardado que cita el código (§2, §10, §14…). Se deja en su sitio a propósito.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 19 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · 08 · Informe y guía: fichajes y dinero en la Liga Máster interna de PES 2021](#parte-1-08-informe-y-guia-fichajes-y-dinero-en-la-liga-master-interna-de-pes-2021) — origen `PhoenixSync/base-conocimiento/08-LIGA-MASTER-INFORME.md`
- [Parte 2 · 09 · Cómo se logró el control de la Liga Máster, paso a paso (qué se hizo, cómo y con qué herramienta)](#parte-2-09-como-se-logro-el-control-de-la-liga-master-paso-a-paso-que-se-hizo-como-y-con-que-herramienta) — origen `PhoenixSync/base-conocimiento/09-LIGA-MASTER-COMO-LO-HICIMOS.md`
- [Parte 3 · 11 · Súper informe: todo lo descubierto, las posibilidades y mis consejos](#parte-3-11-super-informe-todo-lo-descubierto-las-posibilidades-y-mis-consejos) — origen `PhoenixSync/base-conocimiento/11-SUPER-INFORME.md`
- [Parte 4 · 13 · Guía exacta: agentes libres, fichas `0xdb65` y qué es el blob (Liga Máster)](#parte-4-13-guia-exacta-agentes-libres-fichas-0xdb65-y-que-es-el-blob-liga-master) — origen `PhoenixSync/base-conocimiento/13-GUIA-AGENTES-LIBRES-Y-FICHAS.md`
- [Parte 5 · 14 · Guía exacta: cómo se encontró el calendario de la Liga Máster](#parte-5-14-guia-exacta-como-se-encontro-el-calendario-de-la-liga-master) — origen `PhoenixSync/base-conocimiento/14-GUIA-CALENDARIO-LM.md`
- [Parte 6 · 15 · Guía exacta: cómo se descifraron las tablas de posiciones de la Liga Máster](#parte-6-15-guia-exacta-como-se-descifraron-las-tablas-de-posiciones-de-la-liga-master) — origen `PhoenixSync/base-conocimiento/15-GUIA-TABLAS-DE-POSICIONES.md`
- [Parte 7 · 16 · Guía exacta: goleadores, el «lector de temporada» y el error del prefijo](#parte-7-16-guia-exacta-goleadores-el-lector-de-temporada-y-el-error-del-prefijo) — origen `PhoenixSync/base-conocimiento/16-GUIA-GOLEADORES-Y-LECTOR-TEMPORADA.md`
- [Parte 8 · 17 · Traspaso para otra IA: cómo replicar TODO el trabajo de Liga Máster (PES 2021)](#parte-8-17-traspaso-para-otra-ia-como-replicar-todo-el-trabajo-de-liga-master-pes-2021) — origen `PhoenixSync/base-conocimiento/17-TRASPASO-LIGA-MASTER-PARA-OTRA-IA.md`
- [Parte 9 · 18 · Guía exacta: ¿llegan las stats de Phoenix-DB a una Liga Máster? (prueba y búsqueda)](#parte-9-18-guia-exacta-llegan-las-stats-de-phoenix-db-a-una-liga-master-prueba-y-busqueda) — origen `PhoenixSync/base-conocimiento/18-GUIA-STATS-EN-LIGA-MASTER.md`
- [Parte 10 · 19 · Guía exacta: verificación en pantalla del calendario, la tabla y los goleadores](#parte-10-19-guia-exacta-verificacion-en-pantalla-del-calendario-la-tabla-y-los-goleadores) — origen `PhoenixSync/base-conocimiento/19-GUIA-VERIFICACION-EN-PANTALLA.md`
- [Parte 11 · 20 · Bitácora completa de la sesión del chat «Player.bin» (9 y 10 de octubre de 2026)](#parte-11-20-bitacora-completa-de-la-sesion-del-chat-playerbin-9-y-10-de-octubre-de-2026) — origen `PhoenixSync/base-conocimiento/20-BITACORA-SESION-9-10-OCT-2026.md`
- [Parte 12 · 20 · Traspaso para otra IA (versión 2): cómo continuar TODO el trabajo de Liga Máster (PES 2021)](#parte-12-20-traspaso-para-otra-ia-version-2-como-continuar-todo-el-trabajo-de-liga-master-pes-2021) — origen `PhoenixSync/base-conocimiento/20-TRASPASO-LIGA-MASTER-v2.md`
- [Parte 13 · 21 · Guía exacta: el experimento de las dos carreras (stats en la Liga Máster)](#parte-13-21-guia-exacta-el-experimento-de-las-dos-carreras-stats-en-la-liga-master) — origen `PhoenixSync/base-conocimiento/21-GUIA-EXPERIMENTO-DOS-CARRERAS.md`
- [Parte 14 · 23 · Guía exacta: las «barras de crecimiento» de la Liga Máster](#parte-14-23-guia-exacta-las-barras-de-crecimiento-de-la-liga-master) — origen `PhoenixSync/base-conocimiento/23-GUIA-BARRAS-DE-CRECIMIENTO.md`
- [Parte 15 · 24 · Guía exacta: la «palanca» para subir habilidades en una Liga Máster ya empezada](#parte-15-24-guia-exacta-la-palanca-para-subir-habilidades-en-una-liga-master-ya-empezada) — origen `PhoenixSync/base-conocimiento/24-GUIA-PALANCA-SUBIR-HABILIDADES.md`
- [Parte 16 · 25 · Guía exacta: el mapa de las barras (qué barra sube qué habilidad)](#parte-16-25-guia-exacta-el-mapa-de-las-barras-que-barra-sube-que-habilidad) — origen `PhoenixSync/base-conocimiento/25-GUIA-MAPA-DE-BARRAS.md`
- [Parte 17 · 26 · Informe y guía completa: subir habilidades en una Liga Máster ya empezada (barras de crecimiento)](#parte-17-26-informe-y-guia-completa-subir-habilidades-en-una-liga-master-ya-empezada-barras-de-crecimiento) — origen `PhoenixSync/base-conocimiento/26-INFORME-Y-GUIA-COMPLETA-BARRAS-DE-CRECIMIENTO.md`
- [Parte 18 · 28 · Mapa final y completo de las barras de crecimiento (28 de 30)](#parte-18-28-mapa-final-y-completo-de-las-barras-de-crecimiento-28-de-30) — origen `PhoenixSync/base-conocimiento/28-MAPA-FINAL-BARRAS-DE-CRECIMIENTO.md`
- [Parte 19 · 29 · Informe maestro: subir y bajar habilidades en una Liga Máster ya empezada (barras de crecimiento) — versión completa y final](#parte-19-29-informe-maestro-subir-y-bajar-habilidades-en-una-liga-master-ya-empezada-barras-de-crecimiento-version-completa-y-final) — origen `PhoenixSync/base-conocimiento/29-INFORME-MAESTRO-BARRAS-DE-CRECIMIENTO.md`

---

## Parte 1 · 08 · Informe y guía: fichajes y dinero en la Liga Máster interna de PES 2021

> Origen: `PhoenixSync/base-conocimiento/08-LIGA-MASTER-INFORME.md` · 7969 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026.** Resume el trabajo del 8 de octubre (y lo aprendido el 9) sobre los guardados de la Liga Máster.
Detalle técnico completo: `liga-master/ESTRUCTURA-ML.md` (§1–§20) y `01-MOTOR-PES2021.md` (§C–§I). Diario de pruebas: `../PRUEBAS.md` (Parte 2).

---

### 1. La idea en simple

Cada carrera de Liga Máster es **un archivo propio** en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`:

- `ML00000000` = ranura 1 del menú «Cargar», `ML00000001` = ranura 2… (la ranura N es el archivo `ML0000000(N−1)`, contado en hexadecimal: la ranura 11 es `ML0000000A`).

Ese archivo es como **el cuaderno de la carrera**: guarda las plantillas de los 700 equipos, tu equipo con todos sus datos, contratos, dinero, calendario y una «ficha de Liga Máster» por jugador.

**Por eso los cambios del option file o de la base NO llegan a una carrera que ya existe**: la carrera tiene su propio cuaderno. Para cambiar una carrera, Phoenix Sync escribe **directamente en ese cuaderno**.

---

### 2. Lo que logramos (todo probado dentro del juego)

| Qué | Estado | Prueba |
|---|---|---|
| Abrir y volver a cerrar el archivo (está cifrado) | ✅ | El juego acepta los guardados editados |
| Traspaso entre dos equipos de la IA (por ejemplo, Isak: Liverpool → Santos) | ✅ | Ranura 9: plantilla, dorsal y alineación correctos |
| Vender un jugador de **tu** equipo a la IA (por ejemplo, Guéhi: City → Real Madrid) | ✅ | Ranuras 8–10: tu Estrategia sin huecos ni porteros en el ataque |
| Fichar **para** tu equipo desde la IA (por ejemplo, Sommer: Inter → City) | ✅ | Pruebas 18 y 19: jugó de titular, un solo contrato |
| Cambiar tu **presupuesto de fichajes** y el **tope salarial** | ✅ | Ranura 12: 500 M en pantalla |
| Cambiar el **valor de mercado** de un jugador | ✅ | Prueba 17: Sommer valía 77,7 M en pantalla |
| Que los cambios **aguanten un partido** y un guardado | ✅ | Ranura 9 jugada y guardada en la 10: el juego no deshizo nada |
| Stats (velocidad, etc.) dentro de una carrera ya empezada | ❌ todavía no | Con Editar → Cargar no llegaron (prueba del 9 oct, 04:53). Falta la prueba de reiniciar el juego |

---

### 3. Cómo está hecho el cuaderno (lo justo para entender)

1. **700 bloques de equipo.** Cada uno tiene su lista de hasta 40 jugadores, sus dorsales y un contador.
2. **La alineación de cada equipo de la IA** (titulares, banca y 6 roles: capitán, lanzadores…). Se guarda **aparte**, en otro bloque.
3. **Tu equipo** guarda mucho más: **12 tablas** con un registro por jugador (datos de plantilla, historial, estado…), tu orden de formación, la lista de la Estrategia, los **contratos** y las **negociaciones abiertas**.
4. **El dinero:** presupuesto de fichajes y tope salarial. El presupuesto salarial que ves en pantalla es el tope menos la suma de sueldos.
5. **Un bloque comprimido** («blob») con la **ficha de Liga Máster de cada jugador**: sueldo, valor de mercado, contrato, competiciones…

Cada pantalla del juego lee una parte distinta. Por eso hay que tocar **todas las partes a la vez**: si no, el jugador queda «medio fichado».

---

### 4. Guía: cómo se hace un fichaje en la Liga Máster

#### Lo que hace Phoenix Sync (el programa)
1. **Copia de seguridad** del guardado antes de tocar nada.
2. **Abre** (descifra) el archivo.
3. **Localiza todo por «anclas»** (busca a los jugadores conocidos de tu plantilla), no por direcciones fijas. Así funciona con cualquier carrera y con cualquier equipo.
4. **Mueve al jugador** según el caso:
   - **IA → IA:** sale de la lista del club de origen y entra al final del de destino. Se corrigen las dos alineaciones: si se va un titular, entra un sustituto de su misma posición (un portero solo lo cubre otro portero).
   - **Tu equipo → IA (venta):** además vacía o compacta sus registros en tus 12 tablas igual que lo hace el juego, borra **todos** sus contratos y negociaciones, y arregla tu orden de formación, los roles y la Estrategia.
   - **IA → tu equipo (fichaje):** crea sus registros en tus tablas copiando a un compañero de su posición, lo pone al final de la formación, descuenta el dinero y prepara su ficha del blob. **El contrato no se escribe**: el juego lo crea solo desde la ficha. Si se escribe, queda duplicado.
5. **Dorsal** del que llega: el que se pida → el que tenía en su club → el de su selección → el más alto libre.
6. **Cierra** (vuelve a cifrar) y verifica el archivo.

#### Lo que haces tú (FRALEX)
1. **No tengas cargada esa ranura** en el juego mientras se cambia (puedes estar en el menú).
2. Ve a **Liga Máster → Cargar** y elige la ranura.
3. Revisa tres pantallas: **Equipos** (plantilla), **Alineación** del club que recibió al jugador y tu **Estrategia**.
4. Juega un partido y **guarda en otra ranura**, para tener siempre la anterior como respaldo.

---

### 5. Reglas de oro (aprendidas a golpes)

1. **Siempre trabajar sobre una copia** y guardar el original. El juego **autoguarda encima de la ranura cargada** al terminar un partido.
2. **Anotar la huella (md5/sha256)** de cada archivo entregado: así se sabe si es el nuestro o uno que ya reescribió el juego.
3. **Tocar todas las partes a la vez:** listas, dorsales, contador, alineaciones, tablas de tu equipo, contratos y Estrategia.
4. **Sustitutos por posición:** la alineación guarda «números de puesto»; si se quita a alguien sin corregirlos, se corre todo (así apareció un portero en el ataque, ranura 5).
5. **Las alineaciones de la IA van aparte:** si no se actualizan, el que llega sale como un jugador en blanco «DC 0» (ranura 8).
6. **No suponer tamaños:** la banca puede ser de 7 o de 12 según la partida.
7. **Después de un partido** aparece la «ficha del último partido» (solo los que jugaron): no es una tabla de plantilla y no se toca.
8. **El menú Cargar** muestra el «texto info» del guardado, no su nombre: para distinguir ranuras de prueba se cambia la primera línea de ese texto.

---

### 6. Lo que el juego hace por su cuenta (para no pelear con él)

- Tras un partido reescribe partes del archivo (contratos, calendario, fichas de quienes jugaron, puntos), pero **no deshace** las plantillas ni las alineaciones editadas.
- Al avanzar los días hace sus propios fichajes de la IA (cientos en el último día de mercado).
- Crea jugadores propios (los «regens») y los reparte entre la IA.
- Los clubes de la IA **no tienen presupuesto guardado**: fichan por reglas.

---

### 7. Lo que falta

1. **Stats dentro de una carrera ya empezada:** comprobar si cambian al **reiniciar** el juego con un `Player.bin` nuevo en Phoenix-DB. Si no, buscar dónde guarda la carrera las habilidades (en el blob no están; se buscó a fondo).
2. **Agentes libres** (jugadores sin club) en la Liga Máster.
3. **Calendario de partidos:** aún sin localizar del todo.
4. **Conectar con la web:** que un traspaso aprobado en tu página se aplique solo a la carrera elegida (el motor de sincronización firmado ya existe en Phoenix Sync).
5. Que **Phoenix Link** avise: «Fichaje aplicado en tu carrera. Carga la ranura X».

---

### 8. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Programa (C++) | `PhoenixSync/core/LigaMaster.cpp` y `.h`, `core/Alineacion.h`, `core/BlobLM.*` |
| Pruebas automáticas | `PhoenixSync/pruebas/main.cpp` (más de 290 comprobaciones) |
| Estructura completa | `PhoenixSync/liga-master/ESTRUCTURA-ML.md` |
| Prototipos (Python) | `PhoenixSync/liga-master/prototipos/` |
| Herramientas de cifrado | `PhoenixSync/liga-master/herramientas/` (`dec`, `enc2`, `info`) |
| Diario de pruebas | `PhoenixSync/PRUEBAS.md`, Parte 2 |
| Guardados de referencia en tu PC | `…\239200\save\_referencias_mercado\` |

---

## Parte 2 · 09 · Cómo se logró el control de la Liga Máster, paso a paso (qué se hizo, cómo y con qué herramienta)

> Origen: `PhoenixSync/base-conocimiento/09-LIGA-MASTER-COMO-LO-HICIMOS.md` · 18557 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026.** Este informe cuenta el **camino real** del trabajo del 8 de octubre: cada paso, la herramienta usada, qué falló y cómo se arregló.
El resumen y la guía de uso están en `08-LIGA-MASTER-INFORME.md`. El detalle de bytes está en `liga-master/ESTRUCTURA-ML.md` (§1–§20). El diario de pruebas está en `../PRUEBAS.md` (Parte 2).

---

### 0. Palabras que se usan aquí

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

### 1. Las herramientas que se usaron

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

### 2. El método (siempre el mismo ciclo)

Cada parte se logró repitiendo estos 6 pasos:

1. **Traer a la nube** el guardado de tu PC (copiado; el original nunca se toca).
2. **Descifrar** con `dec` y comprobar que la ida y vuelta sale idéntica.
3. **Conseguir una «verdad del juego»:** tú hacías la operación real en el juego (fichar, despedir, jugar) y guardabas en **otra** ranura.
4. **Comparar byte a byte** el antes y el después del juego para ver **exactamente** qué toca el juego y dónde.
5. **Programar lo mismo** (primero en Python, después en C++) y **comparar nuestro resultado con el del juego** hasta que salga igual.
6. **Cifrar** con `enc2` (o con el C++), ponerle un texto visible en el menú Cargar, **anotar la huella**, dejarlo en una ranura libre de tu PC y **probarlo en el juego**. Se anota el resultado en `PRUEBAS.md`, salga bien o mal.

> Regla que nació aquí: **un cambio solo se da por bueno cuando lo ves en el juego**. Lo que dicen los archivos es «observado»; lo que ves en pantalla es «probado».

---

### 3. Paso a paso, en el orden en que pasó (8 de octubre)

#### Paso 1 · Abrir el archivo
- **Qué:** entender el envoltorio del guardado.
- **Cómo:** se compiló `dec`, `info` y `enc2` con gcc contra libpesXcrypter. `dec` descifró `ML00000000` (19,7 MB legibles) y lo volvió a cifrar idéntico.
- **Descubrimiento clave:** la función de cifrado **no recalcula** la huella de la cabecera, y aun así **el juego acepta el archivo editado**.
- **Descubrimiento 2:** el menú Cargar **no muestra** el nombre interno de 128 B, sino el **texto info** (`Equipo / Liga · Fecha · Competición`). Por eso `enc2` cambia la primera línea de ese texto: así reconocías «PRUEBA 4», «PRUEBA 5»… en el menú.

#### Paso 2 · Encontrar las plantillas de los 700 equipos
- **Cómo:** se compararon tus guardados antes y después de fichar a **Neymar**, y una zona con **246 traspasos que hizo la IA sola**.
- **Resultado:** 700 bloques de **1680 B** desde el byte `0x50`. Cada uno: nombre, ID del option file, lista de 40 `(reg, pid)`, 40 dorsales y un contador. City = bloque 154, Santos = 66, Real Madrid = 135.
- **Herramienta:** `mlparse.py`.

#### Paso 3 · Primer traspaso entre dos equipos de la IA (ranura 3)
- **Qué:** Mbappé, Real Madrid → Santos.
- **Cómo:** solo se cambió lo que cambiaba en los 246 traspasos del juego: lista de plantilla (el que llega va **al final**), dorsales y contador.
- **Resultado:** ✅ Mbappé aparecía en el Santos.
- **Lo que no se miró:** la pantalla *Alineación*. Esto volvió a salir en el paso 8.

#### Paso 4 · Quitar a un jugador de TU equipo (ranuras 1, 4 y 5)
- **Ranura 1 (v2):** solo la lista de plantilla de Haaland. ❌ Haaland salía «sin equipo» en la búsqueda, pero **seguía** en tu Estrategia y en la zona de ventas.
- **Por qué:** tu equipo guarda **muchas más tablas por jugador**. Comparando byte a byte se encontraron: tablas **A, B, C, D, E, F, G, H, M** (datos por jugador), la tabla **L** (al revés) y la lista **K** (la que lee Estrategia).
- **Ranura 5 (v5):** se tocaron todas. Haaland perfecto en el Santos, pero ❌ **Rulli (portero) aparecía en el ataque** de tu Estrategia.
- **Por qué:** la alineación y los 6 roles (capitán, lanzadores) guardan **números de puesto**. Al sacar a Haaland, todos se corrían un lugar.
- **Herramientas:** `mlsquad.py`, `build_c.py`, `diff_gt.py`.

#### Paso 5 · Corregir la alineación y los roles (ranura 6)
- **Cómo:** `build_v6.py` recalcula el **orden de formación** y los **roles**; Neymar entra en el puesto de Haaland.
- **Resultado:** ✅ Estrategia bien, ningún portero arriba. Jugaste un partido y guardaste: ✅ el juego no corrigió nada.
- **Lección:** el juego **guardó encima** de la ranura 6. Desde aquí, **cada archivo entregado se identifica por su huella**, no por su nombre.

#### Paso 6 · Pasar de Python a C++ «por anclas»
- **Problema:** las direcciones de Python solo valían para **un** guardado. El blob crece con la temporada (+16.792 B en la ranura 7) y **desplaza** las tablas que van detrás (I y J).
- **Cómo se resolvió:** en `core/LigaMaster.cpp`, la función `tablasDe` busca cada tabla por los **5 primeros `(reg, pid)`** de tu plantilla. El orden de formación y la lista K se reconocen por su **forma**, no por su dirección.
- **Comprobación:** el C++ (`moverUsuarioAIA`) dio **el mismo archivo byte a byte** que `build_v6.py` en el respaldo y que `ref_v7_juego.py` en la ranura 7.

#### Paso 7 · Primera prueba del C++ (ranura 8): el «jugador en blanco»
- **Qué:** Guéhi, City → Real Madrid.
- **Resultado:** ✅ Equipos, búsqueda y tu Estrategia. ❌ En la **Alineación del Madrid** salía un jugador en blanco **«DC 0»**.
- **Por qué:** cada equipo de la IA tiene **su propia alineación**, guardada **aparte**.

#### Paso 8 · Encontrar las alineaciones de la IA (ranura 9)
- **Cómo:** se buscó una serie de bloques numerados 0, 1, 2, 3… cada 600 B con el ID option de cada equipo. Así aparecieron **629 bloques de 600 B**: nombre del técnico, **orden de formación** en `+0x220` y **roles** en `+0x248`.
- **Comparando 431 equipos** que la IA había cambiado sola: si se va un **titular**, una reserva de su posición ocupa su lugar; si se va un suplente, los de atrás suben; el que llega va **al final**.
- **Programado en el C++:** `Alineacion.h` y `sugerirSustituto` (reservas primero; misma posición > misma línea > cualquiera; **un portero solo lo cubre otro portero**).
- **Prueba (ranura 9):** Guéhi al Madrid + Isak, Liverpool → Santos. ✅ Guéhi sin jugador en blanco, **Ekitiké** (elegido solo) en el puesto de Isak, Isak con el 99 en el Santos.
- **Hallazgo:** en esa partida la **banca es de 12**, no de 7. El C++ dejó de suponer tamaños.

#### Paso 9 · ¿Aguanta un partido? (ranura 9 → ranura 10)
- **Cómo:** jugaste y guardaste en la 10. Se comparó con nuestra versión.
- **Resultado:** ✅ **todo se mantuvo**. El técnico de la IA reordenó a su gusto (Isak titular en el Santos), y hubo 332 fichajes de la IA (último día de mercado).
- **Problema encontrado:** al jugar aparece una «**ficha del último partido**» que se parece a una tabla de plantilla. El C++ la confundía y se negaba a vender («TABLA_DANADA»).
- **Arreglo:** una tabla de plantilla **debe tener a todos** tus jugadores; si no, se ignora.

#### Paso 10 · La «verdad del juego» al despedir (ranura 10 → ranura 11)
- **Qué hiciste:** «Despedir» a Stones en el juego y guardar en la 11 (`ML0000000A`).
- **Qué se descubrió comparando:**
  - la tabla **I** son los **contratos** (registro de 48 B); un jugador puede tener **dos** (su contrato + una **oferta** de otro club);
  - existe una **lista de negociaciones abiertas** (60 B por registro);
  - el juego deja **huecos** en A, B, C, D, E, F, J y M, y **compacta** G, H y los contratos;
  - el **ID interno de club** = `(ID option << 14) | índice del bloque` (City = `0x2b409a`).
- **Programado y medido:** se reescribió la venta del C++ para hacer **exactamente** eso. Sobre la ranura 10 dio **lo mismo que la ranura 11 del juego**, tabla por tabla (solo cambian bytes de relleno que el juego limpia).

#### Paso 11 · El dinero (ranura 12)
- **Cómo:** se compararon las cifras de la pantalla con los números del archivo (en céntimos ÷ 100). Bloque en `0xc7dd00`: presupuesto actual, presupuesto inicial (`+0x10`) y **tope salarial** (`+0x14`).
- **Fórmula comprobada:** presupuesto salarial en pantalla = **tope − suma de sueldos** de los contratos.
- **Por ancla:** ese bloque está siempre a `0x97d38` de tu tabla A (comprobado en 6 guardados). Funciones `GuardadoLM::finanzas` y `fijarFinanzas` (rechazan un tope menor que los sueldos).
- **Prueba (ranura 12):** presupuesto **500.000.000 €** y tope 200 M → en pantalla, presupuesto salarial **86.096.000 €**, la cifra exacta calculada. ✅
- **Además:** los clubes de la IA **no tienen dinero guardado**: fichan por reglas.

#### Paso 12 · Abrir el «blob»
- **Qué era:** 1,2 MB que parecían ruido.
- **Cómo:** se buscó la firma de zlib (`78 9c`) y aparecieron **18 tramos** de 256 KB separados por 12 B. `blob.py` los descomprime: **4,68 MB**.
- **Dentro:** una **ficha de Liga Máster de 156 B por jugador** (16.422) en `0x1e + 156·reg`: sueldo (`+0x56`), **valor de mercado** (`+0x5a`), contrato, competiciones inscritas. Comprobado: Rúben Dias 55.000.000 € = pantalla.
- **Escribir:** `core/BlobLM` lo lee y lo vuelve a comprimir con **miniz** (solo el tramo cambiado), y corrige las palabras de tamaño. Ida y vuelta idéntica en 4 guardados.
- **Prueba 17 (ranura 1):** valor de mercado de Sommer 2,5 M → **77,7 M**. ✅ en pantalla. El juego acepta el blob recomprimido.

#### Paso 13 · La «verdad del juego» al fichar (prueba 16)
- **Qué hiciste:** fichar a **Sommer** (Inter → City) en el juego y guardar en la 15.
- **Qué se descubrió:** el juego crea un registro en el **primer hueco** de cada tabla, con datos del jugador: fecha de llegada (A), **curva de medias por edad** (E), datos de desarrollo (D), bloque grande (M), contrato, historial, dinero y la ficha del blob.

#### Paso 14 · Fichar PARA tu equipo desde el programa (pruebas 18 y 19)
- **Cómo (`GuardadoLM::ficharParaUsuario`):** se toma como **molde** a un compañero de la misma posición, se crea el registro en el primer hueco de cada tabla, se **genera** la curva E, se completa la ficha del blob (competiciones, sueldo, fechas), se pone al final del orden y de K, se descuenta el dinero y se pone un sustituto en el club de origen.
- **Comparado con el fichaje real de Sommer:** iguales plantillas, orden, K, dinero, tabla A, contrato y ficha del blob.
- **Prueba 18:** ⚠️ en pantalla todo bien y **jugó de titular**, pero al autoguardar **el juego creó su propio contrato** y el nuestro quedó **duplicado** (los sueldos superaban el tope).
- **Arreglo:** el programa **ya no escribe el contrato**; deja la ficha del blob y el juego crea el contrato solo.
- **Prueba 19:** ✅ partido jugado, **un solo contrato**, inscrito en las 6 competiciones, presupuesto salarial positivo.
- **Extra:** el juego **relee** el archivo al cargar la ranura. Se puede cambiar un guardado con el juego abierto, **mientras esa ranura no esté cargada**.

#### Paso 15 · Unirlo con la web
- **Cómo:** la sincronización con la web llama a `ficharParaUsuario` con las condiciones opcionales de la web (monto, sueldo, cláusula, fin de contrato, dorsal).
- **Comprobado:** da **exactamente los mismos bytes** que el fichaje validado en la prueba 19. Pruebas automáticas: **295 de 295 ✅**.
- **En tu PC** (`PROBAR2.bat`, compilado con MinGW): conexión con la web OK y firma Ed25519 verificada.

---

### 4. Tabla de todas las pruebas en el juego

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

### 5. Errores que enseñaron algo (y cómo se arreglaron)

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

### 6. Qué NO se usó (para que quede claro)

- No se tocó el ejecutable del juego para esta parte. Todo es **edición del archivo de guardado**, como hace un editor de partidas.
- No se usaron programas de terceros de edición de Liga Máster: el mapa se sacó comparando tus guardados.
- Los guardados y los `.bin` descifrados **no se suben a GitHub** (son tus datos).

---

### 7. Lo que falta (igual que en el 08)

1. Stats dentro de una carrera ya empezada (prueba de reiniciar el juego con un `Player.bin` nuevo).
2. Agentes libres.
3. Calendario de partidos (pista: 10 números de partido por jornada; falta guardar antes y después de un partido el mismo día).
4. Que un traspaso aprobado en la web se aplique solo a la carrera elegida, con aviso de Phoenix Link.

---

## Parte 3 · 11 · Súper informe: todo lo descubierto, las posibilidades y mis consejos

> Origen: `PhoenixSync/base-conocimiento/11-SUPER-INFORME.md` · 19167 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026.**
Este informe junta **en un solo lugar** lo que descubrimos sobre PES 2021, lo que eso permite hacer y lo que te recomiendo.
Si quieres el detalle de una parte, al final (sección 7) está la lista de documentos.

> **Multiparche.** Todo se descubrió y se probó sobre **ConmeGOL Patch 26**, pero casi todo lo pone **el juego**, no el parche, así que vale para cualquier parche de PES 2021. Donde algo depende del parche, se marca con 🧩 y se explica. La guía para llevarlo a otro parche (con sus riesgos) es el informe **12 · Guía multiparche**.

---

### 0. Cómo leer este informe

- Cada parte empieza con **la idea en simple** y después da los datos.
- Los símbolos significan:
  - ✅ = **probado en el juego** (lo viste en pantalla).
  - 🔎 = **visto en los archivos**, todavía sin prueba en pantalla.
  - ❌ = **probado y no funciona**.
  - 🟢 = ya se puede hacer.
  - 🟡 = se puede, pero falta investigar.
  - 🔴 = muy difícil; puede que no salga.
  - 🧩 = depende del parche.

#### Palabras que se repiten

| Palabra | Qué significa, en simple |
|---|---|
| Option file (`EDIT00000000`) | El archivo de «Editar» del juego. Guarda **las plantillas** (qué jugador está en qué equipo), nombres de equipos y jugadores editados |
| Base de datos (`pesdb`) | Los archivos internos del juego: `Player.bin` (stats de cada jugador), `PlayerAssignment.bin` (plantillas «de fábrica»), `Team.bin`, `Country.bin`… |
| Sider | Programa que se pone delante del juego y le «entrega» archivos nuestros en lugar de los originales, sin tocar el juego |
| Phoenix-DB | Nuestra carpeta dentro de Sider. Lo que pongas ahí **gana** sobre la base del parche |
| Liga Máster (`ML0000000N`) | Cada carrera es un archivo propio, como un cuaderno de esa partida |
| Cifrado | El juego guarda sus archivos «revueltos». Hay que abrirlos con la llave y volver a cerrarlos |
| Ancla | Algo conocido que se busca para ubicarse dentro de un archivo (por ejemplo, tus 5 primeros jugadores) |
| Huella (sha256 / md5) | Código que identifica un archivo exacto. Si cambia un byte, cambia la huella |

---

### 1. El mapa general (las 4 piezas)

Piensa en un restaurante:

| Pieza | Papel en el restaurante | Qué es de verdad |
|---|---|---|
| **La web** (Phoenix Evolution, Supabase) | **La receta**: dice qué fichajes se aprobaron | Tu página. Publica los cambios de la liga **firmados** (con un sello que no se puede falsificar) |
| **Phoenix Sync** | **El cocinero**: prepara los archivos | Programa en C++ que lee la receta y fabrica el option file, la base o la carrera nueva |
| **Phoenix Link** | **El mesero**: lleva el plato y avisa | Tu app de PC. Coloca los archivos y avisa dentro del juego (buzón `avisos.txt`) |
| **El juego + Sider** | **El comensal** | PES 2021, que lee lo que le dejamos |

**La regla del restaurante:** el cocinero **nunca** cocina encima del plato anterior. Siempre hace uno nuevo, lo prueba (huella) y guarda el viejo.

---

### 2. Todo lo descubierto

#### A. Abrir y cerrar los archivos del juego ✅

**En simple:** todos los guardados de PES 2021 van en el **mismo tipo de sobre** y con **la misma llave**. Esto vale **para todos los parches**.

- Herramienta: **libpesXcrypter**, una biblioteca libre de la comunidad de PES.
- El sobre tiene 5 piezas: cabecera · etiqueta (384 B) · logo (17.315 B) · **datos** · serial (44 B).
- Abrir y cerrar sin cambios da un archivo **idéntico** ✅.
- El juego **acepta archivos editados**: no comprueba la huella de la cabecera ✅.
- El menú *Cargar* muestra el **texto info** (desde el byte 128 de la etiqueta), no el nombre ✅.
- Se puede cerrar con **otro tamaño** de datos ✅ (la Liga Máster crece al fichar).
- Detalle: informe **10**.

#### B. El option file ✅

**En simple:** es la «libreta de plantillas» que el juego usa cuando juegas amistosos y modos normales.

| Qué guarda | Dónde | Estado |
|---|---|---|
| Plantillas: 284 B por equipo (40 jugadores + 40 dorsales); 🧩 cuántos equipos depende del parche (749 en ConmeGOL; el límite es 750) | desde `0x9D4648` | ✅ (fichajes probados: Lamine, Julián, Mbappé) |
| Tácticas: orden de formación y 6 roles por equipo (628 B) | desde `0xA09880` | ✅ (prueba Boca: Braida entró de lateral, sin porteros en el ataque) |
| Jugadores editados (312 B) | desde `0x7C` | ✅ |
| Equipos (588 B, nombre y abreviatura) | desde `0x8ED2FC` | ✅ |

- Phoenix Sync lo cambia con `OptionFile::mover` (con dorsal y **sustituto por posición**: un portero solo lo cubre otro portero).

#### C. La base de datos (`Player.bin` y compañía) ✅

**En simple:** es la «ficha técnica» de cada jugador: velocidad, tiro, altura, posiciones…

- Cada jugador ocupa **312 B**. Ya están mapeadas: las 19 habilidades de campo, **potencia de tiro**, las **5 de portero**, pie malo, regularidad, resistencia a lesiones, país, altura, peso, edad y parte de las habilidades especiales ✅ (comprobado contra las pantallas con Lamine, Joan García y Livakovic).
- Formato del archivo: cabecera **WESYS** de 16 B + zlib. El juego acepta nuestra compresión ✅.
- **Phoenix-DB:** si nuestra carpeta va **antes** que la del parche en `sider.ini`, el juego usa **nuestro** archivo ✅.
  - 🧩 Tiene que estar en **cada** `sider.ini` que use tu parche. Ejemplos: en ConmeGOL, en la carpeta del juego **y** en la del modo (el switcher copia el modo encima); en Sudamerican hay **dos** Sider (normal y Mundial). Si un parche **no usa Sider**, hay que verlo aparte (informe 12).
- `PlayerAssignment.bin` (16 B por fila): jugador, equipo, dorsal, orden y banderas ✅.
- Límites de la base: **750 equipos, 40 jugadores por equipo, 30.000 jugadores**.
- 🧩 **Ojo:** cada parche guarda su base en un sitio distinto, y no siempre es el que parece. En ConmeGOL 26, la que de verdad usa el juego es la que sirve Sider desde `olmosjr23\Database` (no el `.cpk`). En Sudamerican está dentro de `SP_Subs.cpk`. **Phoenix-DB siempre se fabrica desde la base que el juego usa de verdad.**

#### D. Cómo y cuándo lee el juego esos archivos ✅

**En simple:** el juego no mira los archivos todo el tiempo. Solo los relee en momentos concretos.

| Momento | ¿Relee la base y el option file? |
|---|---|
| Al arrancar el juego | ✅ todo |
| **Editar → Cargar** | ✅ Player, PlayerAssignment, Team, Coach, Competition… y el option file |
| Al armar un amistoso, abrir habilidades | ❌ no |
| Al entrar a la Liga Máster | ❌ no lee `Player.bin` |

**Las 3 reglas de oro** (son del juego: valen para todos los parches):
1. **Stats** → van por `Player.bin` (Phoenix-DB).
2. **Plantillas** → dependen de la opción **«Datos Actual. en vivo»** del juego:
   - **Activada:** el juego toma las plantillas de la **base** (`PlayerAssignment.bin`).
   - **Desactivada:** las toma del **option file**.
   - Por eso, para que un fichaje se vea **siempre**, se pone en **los dos** (así lo hicimos con Mbappé).
3. Si un jugador está entre los **editados** del option file, **mandan sus datos editados**, no los de `Player.bin`.

**El botón para aplicar todo:** **Editar → Cargar** (sin pulsar *Guardar*).

#### E. La Liga Máster ✅

**En simple:** cada carrera es un cuaderno propio. Los cambios del option file o de la base **no llegan** a una carrera ya empezada: hay que escribir en su cuaderno.

| Qué controlamos | Estado |
|---|---|
| Traspasos entre dos equipos de la IA (plantilla + alineación) | ✅ |
| Vender un jugador de **tu** equipo a la IA (igual que lo hace el juego) | ✅ |
| Fichar **para** tu equipo desde la IA | ✅ (Sommer jugó de titular, un solo contrato) |
| Presupuesto de fichajes y tope salarial | ✅ (500 M en pantalla) |
| Valor de mercado de un jugador | ✅ (Sommer 77,7 M) |
| Que todo aguante un partido y un guardado | ✅ |
| Stats dentro de una carrera ya empezada | ❌ todavía no (falta la prueba de reiniciar) |
| Agentes libres | 🟡 pendiente |
| Calendario de partidos | 🟡 pista encontrada (10 números de partido por jornada) |

Lo que hay dentro del cuaderno:
- **Los equipos** (plantillas y dorsales) y **las alineaciones** de la IA, guardadas aparte. 🧩 En ConmeGOL son 700 y 629; en otro parche pueden cambiar. Phoenix Sync los **busca por anclas** y no supone números fijos.
- **Tu equipo**: 12 tablas por jugador, contratos, negociaciones, orden de formación y Estrategia.
- **Dinero**: solo lo tiene tu club. **La IA no tiene dinero guardado**: ficha por reglas.
- **El «blob»**: 18 trozos comprimidos con una **ficha por jugador** (sueldo, valor, contrato, competiciones) y resultados de la temporada.
- **Competiciones**: listas de equipos con espacio de sobra (ya hay una liga de 30 equipos).
- 🧩 Probado solo en ConmeGOL 26. En otro parche hay que repetir las pruebas en una carrera nueva (informe 12).
- Detalle: informes **08** y **09**.

#### F. Lo que el juego hace por su cuenta

- **Autoguarda encima** de la ranura cargada al terminar un partido → siempre se trabaja en otra ranura y se anota la huella.
- Hace sus propios fichajes de la IA (cientos el último día de mercado).
- Crea jugadores nuevos (los «regens»).
- **Crea el contrato** de un fichaje a partir de la ficha del blob → nosotros **no** lo escribimos (si no, queda duplicado).
- **Relee el guardado** al cargar la ranura → se puede cambiar con el juego abierto, si esa ranura no está cargada.

#### G. La conexión con la web ✅

- La web publica los cambios de la liga **firmados (Ed25519)**. Phoenix Sync comprueba la firma con la clave pública incrustada: si alguien cambia algo por el camino, se rechaza ✅.
- `PhoenixSync sincronizar` aplica los cambios al option file **y/o** a la Liga Máster, **todo o nada**, y siempre en archivos nuevos.
- Probado en tu PC con `PROBAR2.bat`: conexión OK, firma OK, **295 de 295** pruebas automáticas ✅.
- Phoenix Link ya tiene el **buzón de avisos** (`avisos.txt` → mensaje dentro del juego) ✅.

#### H. Qué vale para todos los parches y qué no 🧩

| Lo pone el juego (igual en todos los parches) | Cambia en cada parche |
|---|---|
| El sobre y la llave de los guardados | La **carpeta de guardados** (número de Steam: ConmeGOL 239200, Sudamerican 292733975847239680) |
| El formato del option file y de `Player.bin` | **Dónde está la base** (Sider o CPK) y su orden de carga |
| La estructura de la Liga Máster | Los **IDs** de equipos y jugadores, y cuántos hay |
| Editar → Cargar, la regla de «Datos Actual. en vivo», los editados mandan | Si usa **Sider**, cuántos `sider.ini` tiene y si un switcher o lanzador los pisa |
| El autoguardado y el contrato que crea el juego | Cuántos jugadores trae **editados** en su option file |
| — | Si trae un **exe modificado** (por ejemplo, Football Life) |

Detalle y guía paso a paso: informe **12**.

#### I. Lo que NO funciona (para no repetirlo)

| Intento | Por qué no |
|---|---|
| Fichajes solo con `PlayerAssignment.bin` y la opción desactivada | Con la opción desactivada mandan las plantillas del option file |
| Cambiar el option file o la base para una carrera ya empezada | La carrera tiene su propio cuaderno |
| Cambiar stats escribiendo en la memoria del juego | La pantalla lee otra copia; además, leer memoria a lo bruto cerraba el juego |
| La actualización en vivo oficial de Konami | Sus servidores cerraron en 2022 |
| Cambiar el número de equipos de una liga sin más | El calendario depende de ese tamaño (en foros, la Liga Máster se colgaba en la temporada 2) |
| Escribir el contrato al fichar en la Liga Máster | El juego crea el suyo y queda duplicado |

---

### 3. Las posibilidades

#### 🟢 Ya se puede (con lo que ya está probado)

1. **Mercado de fichajes conectado a la web.** Un fichaje aprobado en tu página → Phoenix Sync lo prepara → Phoenix Link lo coloca y avisa → tú pulsas **Editar → Cargar**.
2. **Liga Máster «viva».** La web mete fichajes, ventas, presupuesto o valor de mercado en una carrera entre partido y partido.
3. **Phoenix Weekly.** Stats ajustadas cada semana (por ejemplo, la forma real del jugador) con `Player.bin`, siempre con tu aprobación.
4. **Liga online justa.** Todos con el mismo option file y la misma base, comprobados por huella antes de jugar (verificador del anfitrión).
5. **Option file por sala o por liga.** Phoenix Link carga el que toca según la sala.
6. **Avisos dentro del juego.** «Fichaje aplicado», «nueva jornada», «tu partido es hoy».
7. **Resultado automático.** Un módulo de Sider deja el marcador final en un archivo y Phoenix Link lo sube a la web.
8. **Phoenix Doctor.** Revisa `sider.ini` y `sider.log` y te dice qué está mal antes de jugar.
9. **Eventos.** Jugador de la semana, castigos por lesión, homenajes, todo con `Player.bin`.

#### 🟡 Se puede, pero falta investigar

1. **Stats dentro de la Liga Máster** (primero, la prueba de reiniciar el juego).
2. **Agentes libres** en la Liga Máster.
3. **Calendario de la Liga Máster** desde la web.
4. **Crear jugadores que no existen** (juveniles, jugadores reales que faltan): registros nuevos en `Player.bin` + su fila de plantilla + su cara.
5. **Competiciones propias** (Liga Phoenix, copas de la web): descifrar los archivos `Competition*` comparando antes y después, como hicimos con la Liga Máster.
6. **Estadísticas completas a la web** (goleadores, tarjetas, posesión).
7. **Fabricar nuestros propios CPK** (paquetes del juego) y **actualizaciones por partes** (solo lo que cambió). También abriría la puerta a parches que no usan Sider.
8. **Cambiar más archivos de la base en vivo:** entrenadores (`Coach.bin`), clubes (`Team.bin`), estadios, botines.
9. **Phoenix en todos los parches:** probar todo en Sudamerican, ConmeGOL 27, Football Life y otros (guía 12).

#### 🔴 La frontera (lo que nadie ha hecho)

1. **Liga Máster online asíncrona.** Varios jugadores, cada uno con su club en su propia carrera, compartiendo **un mismo mercado y una misma tabla** en la web. Es la idea **más original** y, con lo que ya sabemos de la Liga Máster, **la más cercana**.
2. **El mundo real dentro del juego:** lesiones y forma de la Liga 1 real aplicadas cada semana.
3. **Espectadores en vivo:** marcador del partido en la web mientras se juega.
4. **Servidor propio** estilo PESBUL: meses o años de trabajo.

---

### 4. Mis consejos (en orden)

0. **Piensa siempre en multiparche.** Antes de cada guía o herramienta, pregúntate: ¿esto lo pone el juego o el parche? Lo del parche va en su **ficha** (`parches/<parche>.md`), nunca escrito a fuego en el programa.
1. **Primero, estabilidad.** Antes de sumar cosas nuevas, deja el sistema **limpio y probado**:
   - vuelve `phoenix.lua` a la versión **v02** (solo el buzón de avisos) en las dos carpetas `modules`;
   - deja Phoenix-DB con datos **originales** salvo lo que estés probando;
   - guarda la huella de cada archivo que funciona.
2. **Termina el «repartidor» de Phoenix Link** (el prompt ya está hecho): que coloque los archivos, haga respaldo y avise. Es la pieza que une todo.
3. **Usa siempre Editar → Cargar** como el botón oficial de tu sistema. Es nativo, estable y existe en todos los parches.
4. **Fichajes en los dos sitios** (option file + `PlayerAssignment.bin`). Así se ven con la opción en vivo activada o desactivada.
5. **Haz la prueba de stats en la Liga Máster** (reiniciar el juego con un `Player.bin` nuevo). Es la duda más importante que queda en la Liga Máster.
6. **Una base por parche, y sin mezclar.** Cada parche tiene su propia base, su catálogo y su option file (en ConmeGOL hoy es la de `olmosjr23`). Si el parche se actualiza, o cambias de parche, **regenera** Phoenix-DB desde esa base **antes** de jugar (si no, servirías datos viejos o de otro parche).
7. **Una prueba a la vez.** Cambia una sola cosa, pruébala, anótala en `PRUEBAS.md`. Si algo falla, vuelves al respaldo sin perder nada.
8. **Nunca pulses «Guardar» en Editar** durante las pruebas, y **juega la Liga Máster en otra ranura** que la de prueba.
9. **Pide permiso y da crédito** por el contenido de otros (caras, kits, base de Olmos). Lo tuyo (herramientas, web, conexión) es 100 % tuyo.
10. **Apunta a la frontera, pero paso a paso.** La Liga Máster online asíncrona es tu gran carta: empieza por «un jugador, una carrera, conectada a la web», y después suma más.

> Sobre los experimentos de parches en memoria del ejecutable (el «botón nativo»): **no forman parte de este informe**. No los recomiendo para tu sistema ni les doy soporte. El camino recomendado es **Editar → Cargar**.

---

### 5. Cómo trabajamos (el método que funcionó)

1. **Copia** del archivo original (nunca se toca el original).
2. **Abrir** y comprobar la ida y vuelta.
3. **Verdad del juego:** tú haces la acción real en el juego y guardas en otra ranura.
4. **Comparar** el antes y el después, byte a byte.
5. **Programar** lo mismo y comparar con lo del juego hasta que salga igual.
6. **Probar en el juego** y anotar el resultado, salga bien o mal.

**Un cambio solo se da por bueno cuando lo ves en el juego.** Y en cada parche nuevo, se vuelve a probar.

---

### 6. Las reglas que nunca se rompen

1. Siempre respaldo; siempre archivo nuevo; nunca sobrescribir.
2. Siempre comprobar la huella del archivo entregado **en tu PC**.
3. Si hay un error, **no se avanza**.
4. Si algo puede romperse, **mejor no se hace**.
5. Los guardados y los datos abiertos **no se suben a GitHub**.
6. Los cambios de la web y de Supabase se piden por prompt al chat WEB.
7. Toda guía es **multiparche**: separa lo del juego de lo del parche y dice sus riesgos.

---

### 7. Dónde está cada cosa (carpeta `PhoenixSync/base-conocimiento/`)

| Documento | De qué trata |
|---|---|
| 01 · Motor de PES 2021 | Todos los formatos: option file, Liga Máster, `Player.bin` |
| 02 · Parche Phoenix | De qué está hecho un parche y qué falta para el tuyo |
| 03 · Investigación de parches | Causas de crash y reglas Phoenix |
| 04 · Estrategia Phoenix | ConmeGOL y Sudamerican por dentro, y cómo superarlos |
| 05 · Puerta en vivo | Cambiar la base sin reiniciar (Editar → Cargar) |
| 06 · Ruta del descubrimiento | La noche del 8 al 9 de octubre, etapa por etapa |
| 08 · Liga Máster: informe y guía | Qué controlamos y cómo se usa |
| 09 · Liga Máster: cómo lo hicimos | Paso a paso, con herramientas |
| 10 · Cómo se abrió el guardado | El sobre, la llave y cómo se cierra |
| 11 · Este súper informe | Todo junto + posibilidades + consejos |
| 12 · Guía multiparche | Cómo llevar todo a cualquier parche, con riesgos |

Además: `PhoenixSync/PRUEBAS.md` (diario de pruebas), `PhoenixSync/REGISTRO.md` (bitácora) y `liga-master/ESTRUCTURA-ML.md` (detalle de bytes).

---

## Parte 4 · 13 · Guía exacta: agentes libres, fichas `0xdb65` y qué es el blob (Liga Máster)

> Origen: `PhoenixSync/base-conocimiento/13-GUIA-AGENTES-LIBRES-Y-FICHAS.md` · 7433 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 21:00 (Lima).**
Esta guía cuenta **cada paso** del avance de hoy: qué se hizo, **cómo se comprobó** y **con qué herramienta**.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §21. Diario: `PRUEBAS.md` (entrada del 9 oct, 21:00).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

### 0. En simple

Cada jugador de una carrera tiene una **ficha de Liga Máster** (como un carnet) dentro de la zona comprimida del guardado (el «blob»).
Hoy aprendimos tres cosas:

1. **Cómo se reconoce a un agente libre** (jugador sin club).
2. **Dónde están los carnets** de unos 1.000 jugadores especiales que el programa no encontraba (y se arregló).
3. **Que el blob es solo la caja de carnets**: el calendario está en otra parte.

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| 7 guardados de tu carrera **ya abiertos** en la nube (copias; tu PC no se tocó) | r0 (4/8), g18 (19/8), g19 y «juego» (21/8), j9 (31/8, antes de despedir a Stones), jA (31/8, después), fD (22/9) |
| `prototipos/blob.py` | Descomprimir el blob de cada guardado |
| Python 3 (scripts de una sola vez) | Contar, comparar byte a byte y cruzar datos |
| `catalogo_ok.json` | Ponerle **nombre** a cada ID de jugador |
| `core/BlobLM.cpp` (C++) | El arreglo del programa |
| `pruebas/main.cpp` + CMake + g++ | Compilar y correr las pruebas automáticas con guardados reales |

**Cómo se supo la fecha de cada guardado:** leyendo la fecha de la partida, que está en el byte `0xacc61c` de los datos (año, mes y día).

---

### 2. Paso a paso

#### Paso 1 · Buscar a Stones antes y después de despedirlo
- **Qué:** j9 (antes) y jA (después de que **el juego** lo despidiera, la «verdad del juego»).
- **Cómo:**
  1. Se leyó su número en la carrera desde la plantilla del City: **reg 4346**, ID **47785**.
  2. Se buscaron todas las veces que aparece la pareja `(4346, 47785)` en los dos archivos.
- **Resultado:** antes aparecía en **20** sitios; después, en **7**. Ya no está en el bloque del City, pero sí en listas de noticias, caja, historial y negociaciones.

#### Paso 2 · Comparar su carnet (ficha de 156 B) antes y después
- **Cómo:** se descomprimió el blob de los dos guardados (`blob.py`) y se compararon los 156 bytes del carnet de Stones, uno por uno.
- **Resultado (solo cambiaron 21 bytes en todo el blob):**

| Parte del carnet | Antes | Después |
|---|---|---|
| Competiciones inscritas | 6 | 3 (se quitaron las del City) |
| Club anterior (+0x5e) | vacío | **City** |
| Fin de contrato (+0x66) | 31/8/2027 | **vacío** |
| Byte +0x75 | 0x05 | **0x25** (se encendió el bit 0x20) |

#### Paso 3 · ¿Ese bit marca a TODOS los libres?
- **Cómo:** se recorrieron **todos los carnets** de la carrera y se cruzaron tres datos: ¿está en algún equipo?, ¿tiene el bit 0x20?, ¿tiene fin de contrato?
- **Resultado:**

| Guardado | Sin equipo + bit 0x20 + sin fin de contrato |
|---|---|
| j9 (antes) | **94** |
| jA (después) | **95** (los 94 + Stones) |

- **Con el catálogo** se vio quiénes son los 94: los agentes libres que trae Konami (L. Manas, J. Rekarte, E. Gois…), sin club anterior.
- **Conclusión:** agente libre = **bit 0x20 de +0x75** + **fin de contrato vacío** + **en ningún equipo**. ✅ (en archivos; en pantalla, ⏳)

#### Paso 4 · Encontrar el segundo grupo de carnets
- **Qué pasó:** al mirar justo después de los 16.422 carnets conocidos, apareció **otro grupo de 9.834 carnets** del mismo tamaño, con números de jugador que empiezan por `0xdb65`.
- **Cómo se comprobó:**
  1. Los números van **seguidos**, del `0xdb654026` al `0xdb65668f`.
  2. `0x4026` = **16.422**: el segundo grupo empieza **justo** donde acaba el primero.
  3. Se tomaron **todos** los jugadores `0xdb65` que están en alguna plantilla (788 en g19) y se buscó su carnet con la regla «posición = parte baja del número». **Los 788 coincidieron** con su ID.
- **Conclusión:** es **la misma lista**; para esos jugadores se usan los 16 bits bajos de su número. ✅

#### Paso 5 · Arreglar el programa
- **Problema:** `BlobLM::fichaDe` usaba el número completo (`0xdb65…`), calculaba una posición enorme y respondía «no está». Con eso **no se podía fichar** a esos jugadores para tu equipo.
- **Arreglo:** si el número empieza por `0xdb65`, se usa su parte baja (`reg & 0xffff`). Es **una línea**.
- **Prueba nueva** (`pruebas/main.cpp`): «todo jugador de toda plantilla tiene su carnet en el blob».
- **Cómo se comprobó:**
  1. Se compiló con CMake + g++ **sin errores**.
  2. Pruebas sin archivos: **211/211** ✅.
  3. Pruebas con 4 guardados reales:

| Guardado | Con carnet | Sin carnet | Jugadores `0xdb65` |
|---|---|---|---|
| r0 (4/8) | 17.121 | **0** | 378 |
| g19 (21/8) | 17.551 | **0** | 793 |
| jA (31/8) | 17.900 | **0** | 1.127 |
| fD (22/9) | 17.947 | **0** | 1.174 |

  Antes del arreglo, los de la última columna habrían salido «sin carnet».

#### Paso 6 · Descubrir que el blob es solo la caja de carnets
- **Cómo:**
  1. Se midió el blob: cabecera de 30 B + espacio para **30.000 carnets** de 156 B (= el **límite de jugadores** del juego). Ocupados: 26.255.
  2. Se comparó la zona **después** de los carnets entre el 19/8 y el 21/8, y entre el 4/8 y el 22/9 (casi dos meses de temporada).
- **Resultado:** esa zona **no cambia nunca** (solo carnets vacíos).
- **Conclusión:** el blob **no** guarda el calendario. Lo que cambiaba con cada jornada eran las **estadísticas de temporada** dentro de los carnets. ✅ Esto **corrige** una idea anterior (ESTRUCTURA-ML §14).

#### Paso 7 · Primera mirada fuera del blob
- **Cómo:** se comparó el resto del guardado entre el 19/8 y el 21/8 (con un partido tuyo por medio).
- **Resultado:** además de lo conocido (plantillas, alineaciones, tus tablas), cambió una zona de registros de 20 B con jugador e ID (cerca de `0xccdbf0`). 🔎 Parece ser «jugadores destacados» o valoraciones de partido, **no** el calendario.

---

### 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| El bit 0x20 de agente libre y el fin de contrato vacío | ✅ Lo pone el juego |
| La regla `reg & 0xffff` para los `0xdb65` | ✅ Lo pone el juego |
| Que el segundo grupo empiece en **16.422** | 🧩 Depende de cuántos jugadores trae el parche. La regla no cambia |
| El blob = 30.000 carnets | ✅ Límite del juego |
| La **posición** del blob en el archivo (`0x11403a8`) | ⚠️ 🧩 Está **fija** en el programa (medida en ConmeGOL 26). En un parche con otro número de equipos o jugadores podría moverse. **Pendiente:** buscarla por ancla |

**Riesgo:** nada de esto se probó todavía en otro parche ni en la pantalla del juego.

---

### 4. Lo que falta (pruebas tuyas en el juego, en ranuras de prueba)

1. **Fichar a un agente libre** y guardar en otra ranura → para programar el fichaje de libres.
2. **Guardar justo antes y justo después de un partido, el mismo día** → para encontrar el calendario y los resultados.
3. **Reiniciar el juego con un `Player.bin` cambiado** y cargar una carrera → stats dentro de la Liga Máster.

---

## Parte 5 · 14 · Guía exacta: cómo se encontró el calendario de la Liga Máster

> Origen: `PhoenixSync/base-conocimiento/14-GUIA-CALENDARIO-LM.md` · 6736 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 21:45 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §22. Diario: `PRUEBAS.md` (9 oct, 21:45).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

### 0. En simple

El calendario de una carrera es como **un cuaderno de partidos numerados**:

- Cada partido de la temporada tiene **un número** (en la carrera del City, del 0 al 5.758).
- Cada competición tiene **su tramo de números**. Por ejemplo, la Premier va del 5.531 al 5.720.
- Cada partido guarda: **local**, **visitante**, **competición**, **jornada** y su **número**.

Lo que **todavía no** encontramos es dónde se apuntan **los goles** (los resultados).

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Guardados del City ya abiertos en la nube: r0 (4/8), g18 (19/8), g19 (21/8), fD (22/9) | Comparar la carrera en distintas fechas |
| **Tu PC** (enlace de la app): `ML00000000` (ranura 1) y `ML00000001` (ranura 2), copiados a la nube sin tocar los originales | Tener una **segunda carrera** (Barça, marzo de 2026) para comprobar |
| `dec` | Abrir los guardados (con ida y vuelta idéntica ✅) |
| `info` | Leer la etiqueta del guardado («FC Barcelona / LaLiga EA Sports · 4/3/2026 · UEFA Champions League») |
| Python 3 + NumPy | Buscar, contar y comparar byte a byte |
| `prototipos/mlparse.py` | Nombres e IDs de los 700 equipos |

---

### 2. Paso a paso

#### Paso 1 · Descartar el blob
- Del avance anterior (guía 13): el blob es **solo** la caja de carnets de jugador. El calendario tenía que estar **fuera**.

#### Paso 2 · Usar el «ID interno de club» como pista
- **Qué es:** un número que junta el ID del option file y el número de bloque del equipo: `ID option × 16.384 + bloque`.
- **Cómo:** en una zona que cambiaba después de un partido aparecieron números raros. Al «desarmarlos» con esa fórmula salieron **clubes de verdad**: Blooming, César Vallejo, Ayacucho, Cantolao, Mannucci… (Liga 1 de Perú). ✅
- **Conclusión:** el juego usa ese ID en las tablas de la temporada.

#### Paso 3 · Buscar ese ID en todo el guardado
- **Cómo:** se recorrió el archivo entero buscando cualquier número que sea un ID interno válido (27.287 apariciones) y se contó cuántas caen en cada zona.
- **Resultado:** una zona muy densa entre `0x1f0000` y `0x28c000`.

#### Paso 4 · Encontrar los registros de partido
- **Cómo:** dentro de esa zona se buscaron **dos IDs de club seguidos** (local y visitante).
- **Resultado:** aparecieron **Sunderland – Everton**, **Arsenal – Brentford**, **Aston Villa – Brighton**… con un número detrás (5.683, 5.684, 5.685…).
- **Forma del registro** (cada 32 bytes):

| Parte | Qué guarda |
|---|---|
| Local | ID interno del club |
| Visitante | ID interno del club |
| Número de partido | Global (único en la carrera) |
| Competición | Número de la competición (Premier = 30 en la carrera del City) |
| Código | Jornada (6 bits) + orden dentro de la jornada |
| 8 bytes «del usuario» | Solo se rellenan en **tus** partidos |

#### Paso 5 · Mapa completo de competiciones
- **Cómo:** se leyeron **todos** los registros (5.273 de liga) y se agruparon por competición.
- **Resultado (carrera del City):** cada competición ocupa un tramo seguido. Ejemplos:

| Competición (nº) | Partidos | Tramo | Qué parece |
|---|---|---|---|
| 17 | 435 | 0–434 | Liga de 30 equipos a una vuelta (Argentina) |
| 18, 19, 80, 82 | 380 cada una | — | Ligas de 20 equipos a dos vueltas |
| 30 | 190 | 5.531–5.720 | **Premier** (primera vuelta) |

- **Tus partidos de Premier (City):** jornada 1 **City – Crystal Palace**, 2 **Everton – City**, 3 **City – Brentford**, 4 **Brighton – City**, 5 **City – Newcastle**.

#### Paso 6 · Comprobar en tu otra carrera (Barça)
- **Cómo:**
  1. Se copió `ML00000000` de tu PC a la nube (el original no se tocó).
  2. Se abrió con `dec`: **ida y vuelta idéntica** ✅.
  3. `info` mostró que es **otra carrera**: Barça, 4/3/2026.
  4. Se corrió la **misma búsqueda**.
- **Resultado:** LaLiga completa (**380** partidos, 38 jornadas, números 0–379), con el mismo formato. ✅

| Jornada | Partido |
|---|---|
| 1 | **Barça – Real Sociedad** |
| 2 | Deportivo – Barça |
| 3 | Barça – Celta |
| 10 | **Real Madrid – Barça** |

#### Paso 7 · ¿Dónde van los goles?
- **Cómo:** se compararon los mismos registros al inicio (4/8), el 21/8 y el 22/9 de la carrera del City.
- **Resultado:** **no cambian** al jugarse. El marcador **no** va en el registro del partido.
- **Zonas descartadas como resultados:**
  - `0x66fa2c…`: registros de 16 B con jugador y algo parecido a una fecha (¿lesiones o sanciones?).
  - `0x10c0000…`: una **imagen** guardada dentro del archivo.
- **Estado:** ⏳ resultados sin localizar.

#### Paso 8 · Copas
- En la misma zona están los **grupos** de las copas: 4 clubes por grupo y 6 números de partido por grupo. Entre el 4/8 y el 22/9 se rellenaron (el sorteo).

---

### 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| La forma del registro de partido (32 B) | ✅ La pone el juego |
| El ID interno de club | ✅ Lo pone el juego |
| Los **números de competición** (Premier 30, LaLiga 29…) | 🧩 Dependen del parche y de la carrera |
| Los **tramos** de números de partido | 🧩 Dependen de qué competiciones trae el parche |
| La zona donde está (`0x1f0000–0x28c000`) | ⚠️ 🧩 Medida en ConmeGOL. El programa debe **buscar la forma**, no la dirección |

---

### 4. Qué tienes que mirar en el juego (para pasar de 🔎 a ✅)

1. Carga la carrera del **Barça** (ranura 1), entra a **Calendario** de LaLiga y mira si:
   - jornada 1: Barça – Real Sociedad (en casa);
   - jornada 2: Deportivo – Barça (fuera);
   - jornada 10: Real Madrid – Barça (fuera).
2. **Para encontrar los resultados:** el día de un partido, guarda en una ranura **justo antes** de jugar, juega y guarda en **otra ranura** justo después (sin avanzar el día).

---

### 5. Qué abre este avance

- **Leer el calendario** de cualquier carrera y mandarlo a la web (próximos partidos de cada liga).
- 🟡 Con los resultados localizados: tablas, goleadores y resultados en la web, solos.
- 🔴 Más adelante: **cambiar** el calendario desde la web (por ejemplo, para una liga propia). Riesgo alto: no tocar sin pruebas.

---

## Parte 6 · 15 · Guía exacta: cómo se descifraron las tablas de posiciones de la Liga Máster

> Origen: `PhoenixSync/base-conocimiento/15-GUIA-TABLAS-DE-POSICIONES.md` · 5259 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 22:10 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §23. Diario: `PRUEBAS.md` (9 oct, 22:10). Viene de la guía 14 (calendario).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

### 0. En simple

Cada liga de la carrera guarda **su tabla de posiciones**, como la que ves en el juego:

- puesto, puntos, ganados, empatados, perdidos;
- goles a favor, goles en contra, partidos jugados;
- goles y victorias **de visitante**.

Y guarda **dos tablas**: la de **ahora** y la de **la jornada anterior** (para saber quién subió o bajó).

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Carrera del City (fD, 22/9) ya abierta en la nube | La Premier con 1 jornada jugada |
| Carrera del Barça (ranura 1 de tu PC, 4/3/2026), copiada sin tocar el original | Ligas con muchas jornadas (Ligue 1 con 8) |
| El calendario descifrado (guía 14) | Para **cruzar** cada partido con la tabla |
| Python 3 | Buscar, leer y comprobar |
| `prototipos/mlparse.py` | Nombres de los equipos |

---

### 2. Paso a paso

#### Paso 1 · Buscar al City en las zonas de la temporada
- **Cómo:** se buscó el **ID interno del City** (`0x2b409a`) en la zona que cambia con la temporada (`0xb00000–0xd40000`).
- **Resultado:** apareció en una lista de **20 clubes de la Premier**, uno cada 20 bytes, **ordenados** (Hull, City, Brentford, Aston Villa…), cada uno con un número de puesto (1, 2, 2, 4, 5, 6, 6, 8…). Los empates comparten puesto, igual que en el juego.

#### Paso 2 · Descifrar los números de cada fila
- Cada fila tiene **20 bytes**: club, puesto y tres números «empaquetados» (varios datos dentro de un mismo número).
- **Cómo:** se probaron cortes de bits hasta que todo tuvo sentido:

| Número | Qué guarda dentro |
|---|---|
| C | puntos · ganados · perdidos · empatados · ganados de visitante |
| A | goles a favor · goles en contra · partidos jugados |
| B | goles de visitante |

#### Paso 3 · Primera comprobación: los goles tienen que cuadrar
- **Idea:** en una liga, todos los goles que alguien marca los recibe otro. Así que **suma de goles a favor = suma de goles en contra**.
- **Resultado:** cuadra en **todas** las tablas (por ejemplo, 195 = 195, 171 = 171, 212 = 212, 220 = 220, 64 = 64). ✅

#### Paso 4 · Segunda comprobación: cruzar con el calendario
- **Cómo:** se tomaron los 10 partidos de la **jornada 1** de la Premier (del calendario, guía 14). Para cada partido se comparó: goles a favor del local = goles en contra del visitante, y al revés.
- **Resultado: 10 de 10 cuadran** ✅:

| Partido | Resultado |
|---|---|
| City – Crystal Palace | 3–0 |
| Ipswich – Everton | 2–2 |
| Bournemouth – Brentford | 0–3 |
| Liverpool – Brighton | 0–0 |
| Nottingham Forest – Newcastle | 0–1 |
| Sunderland – Leeds | 0–2 |
| Arsenal – Chelsea | 0–1 |
| Aston Villa – Coventry | 3–1 |
| Fulham – Tottenham | 2–2 |
| Hull – Manchester United | 4–1 |

- Además, los que ganaron **de visitante** (Brentford, Leeds, Chelsea, Newcastle) tienen marcado «ganado de visitante», y los goles de visitante coinciden. ✅

#### Paso 5 · Las dos tablas
- **Cómo:** en la carrera del Barça, la Ligue 1 aparece dos veces seguidas: una con **8** partidos jugados y otra con **7**.
- **Conclusión:** la primera es la **actual** y la segunda la de la **jornada anterior**. ✅ (en archivos)

#### Paso 6 · Tabla de la Ligue 1 en tu carrera del Barça
| Puesto | Club | PJ | G | E | P | GF | GC | Pts |
|---|---|---|---|---|---|---|---|---|
| 1 | Lille | 8 | 7 | 1 | 0 | 19 | 4 | 22 |
| 2 | PSG | 8 | 6 | 2 | 0 | 17 | 4 | 20 |
| 3 | Lens | 8 | 6 | 1 | 1 | 20 | 8 | 19 |
| 4 | Lorient | 8 | 5 | 1 | 2 | 19 | 11 | 16 |
| 5 | Marsella | 8 | 4 | 2 | 2 | 11 | 8 | 14 |
| 6 | Mónaco | 8 | 4 | 2 | 2 | 9 | 6 | 14 |

- En esa carrera, **LaLiga todavía no empezó** (su tabla está vacía).

---

### 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| La forma de la fila (20 B) y cómo se empaquetan los números | ✅ La pone el juego |
| Las dos tablas (actual y anterior) | ✅ Lo hace el juego |
| Dónde está la zona y en qué orden van las ligas | 🧩 Depende del parche → el programa debe buscar por la forma |

---

### 4. Qué tienes que mirar en el juego (para pasar de 🔎 a ✅)

En tu carrera del **Barça** (ranura 1), abre la **tabla de la liga francesa** y mira si coincide con la tabla del paso 6.

---

### 5. Qué abre este avance

- **Tablas de todas las ligas de una carrera, directo a tu web**, sin capturas de pantalla.
- Con el calendario (guía 14): **próximos partidos** y, comparando dos guardados, **resultados**.
- ⏳ Falta: dónde se guarda cada resultado partido a partido (si es que se guarda), goleadores (hay una pista: registros con jugador + club + número) y las tablas de 40 filas que todavía no entendemos.

---

## Parte 7 · 16 · Guía exacta: goleadores, el «lector de temporada» y el error del prefijo

> Origen: `PhoenixSync/base-conocimiento/16-GUIA-GOLEADORES-Y-LECTOR-TEMPORADA.md` · 6418 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 23:20 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §24. Diario: `PRUEBAS.md` (9 oct, 23:20). Viene de las guías 14 (calendario) y 15 (tablas).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

### 0. En simple

1. **Goleadores:** cada competición guarda listas de jugadores con su puesto y una cantidad. La primera lista son los **goleadores**; la segunda parece ser de **asistencias**.
2. **Lector de temporada:** el programa Phoenix Sync ya sabe **leer solo** el calendario, las tablas y los goleadores de cualquier carrera. **Solo lee**: no cambia nada.
3. **Un error encontrado y arreglado:** en tu carrera del Barça, el programa no encontraba la ficha de 1.156 jugadores. Ya está corregido.

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| 6 guardados reales: r0 (4/8), g19 (21/8), jA (31/8), fD (22/9) de la carrera del City, y las ranuras 1 (Barça) y 2 (City) de tu PC | Probar con carreras y fechas distintas |
| `catalogo_ok.json` | Poner nombre a cada jugador |
| Python 3 | Encontrar y entender las listas |
| C++ (`core/TemporadaLM`, `core/BlobLM`, `core/LigaMaster`) | El lector nuevo y el arreglo |
| `pruebas/main.cpp` + CMake + g++ | Pruebas automáticas con los 6 guardados |
| MinGW | Comprobar que compila para Windows |
| Tu PC (solo mirar) | Ver qué `Player.bin` hay en Phoenix-DB (v99) y cuándo arrancó el juego |

---

### 2. Paso a paso

#### Paso 1 · Encontrar listas de jugadores
- **Cómo:** se buscaron filas de 20 bytes con la forma `[número del jugador][ID][club][puesto][cantidad]`, una detrás de otra.
- **Resultado:** 74 listas en la carrera del City (22/9), **varias por competición**.

#### Paso 2 · Saber cuál es la de goleadores
- **Cómo:** se miró la Premier de esa carrera, que tenía **una sola jornada jugada** (27 goles en total, según la tabla de la guía 15).
- **Resultado:** la primera lista suma **27**, y **club por club** coincide con los goles de la tabla:

| Club | Goles en la tabla | Goleadores en la lista |
|---|---|---|
| City | 3 | Foden 2 + Semenyo 1 |
| Hull | 4 | 4 jugadores con 1 |
| Aston Villa | 3 | Rogers, Buendía y otro |
| Brentford | 3 | Schade 2 + Milambo 1 |

- **Conclusión:** la primera lista = **goleadores** ✅ (en archivos).
- La segunda suma 15 y parece de **asistencias** 🔎. La tercera (41) y la cuarta (21) todavía no se sabe qué son (¿tarjetas?) ⏳.
- **Detalles que encajan:** en la Community Shield, City 5 – Arsenal 1 (Marmoush 3). En Brasil, **Haaland en el Santos con 3 goles**: el traspaso que hicimos nosotros.

#### Paso 3 · Pasarlo al programa (solo lectura)
- **Qué:** un módulo nuevo, `core/TemporadaLM`, que lee **partidos**, **tablas** y **listas**.
- **Cómo evita equivocarse:** solo acepta una tabla si sus números **cuadran entre sí**:
  - puntos = 3 × ganados + empatados;
  - jugados = ganados + empatados + perdidos;
  - goles a favor de todos = goles en contra de todos.
- **Multiparche:** todo se busca **por la forma**, nunca por una dirección fija.

#### Paso 4 · Probarlo con tus 6 guardados
- **Prueba:** en cada liga con **una sola jornada jugada**, cada partido de la jornada 1 tiene que cuadrar con la tabla (goles del local = goles en contra del visitante, y al revés).
- **Resultado:**

| Guardado | Partidos cruzados | Cuadran |
|---|---|---|
| g19 (21/8) | 35 | **35** ✅ |
| jA (31/8) | 152 | **152** ✅ |
| fD (22/9) | 38 | **38** ✅ |

- Y siempre hay una lista de goleadores que suma **exactamente** los goles de esas tablas. ✅

#### Paso 5 · El error de la carrera del Barça
- **Qué pasó:** la prueba «todo jugador tiene su ficha» **falló** en la carrera del Barça: **1.156** jugadores sin ficha.
- **Por la regla «si hay un error, no avanzamos»**, se investigó antes de seguir.
- **Causa:** los jugadores añadidos por el juego llevan un **prefijo** en su número. En la carrera del City es `0xdb65`; en la del Barça es **`0xdbdf`**. **El prefijo cambia en cada carrera.** El programa solo conocía el primero.
- **Comprobación:** el jugador `0xdbdf5827` tiene su ficha en el lugar **22.567**, y `0x5827` = 22.567. La regla de siempre (los 16 bits bajos) sirve igual.
- **Arreglo:** el programa acepta **cualquier** prefijo y siempre comprueba que número e ID coincidan.
- **Resultado:** **0 jugadores sin ficha** en los 6 guardados. Todas las pruebas pasan: 211/211 sin archivos y entre 225 y 234 con cada guardado. Compila también para Windows. ✅

---

### 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| Forma de las listas de goleadores (20 B) | ✅ La pone el juego |
| El prefijo de los jugadores añadidos | 🧩 Cambia en **cada carrera**: el programa no supone ninguno |
| Qué competición es cada lista | 🧩 Depende del parche (se deduce por los clubes) |

---

### 4. Qué tienes que mirar en el juego

1. **Stats en la Liga Máster (sin cambiar nada):** en tu PC, Phoenix-DB tiene ahora el `Player.bin` **v99** (Lamine Yamal con **Velocidad 99**; su valor normal es **90**). Si abriste el juego después de las 5:51 de esta mañana, entra a la carrera del **Barça** (ranura 1) y mira la **Velocidad de Lamine**:
   - si ves **99** → las stats de Phoenix-DB **sí llegan** a la Liga Máster al reiniciar el juego;
   - si ves **90** (u otro número) → la carrera guarda **sus propias** stats.
2. **Goleadores:** en la carrera del Barça, abre las estadísticas de la **liga francesa**. Según el archivo deberías ver:
   - **Goleadores:** Hamza Igamane (Lille), Odsonne Édouard (Lens) y Arthur Avom (Lorient), los tres con **8 goles**; luego Sofiane Diop (Niza) y Joaquín Panichelli (Estrasburgo) con 5.
   - **Asistencias (🔎):** João Neves (PSG) **6**, Désiré Doué (PSG) 4.

---

### 5. Qué abre este avance

- **Tablas, calendario y goleadores de toda una carrera, leídos solos** por Phoenix Sync, listos para mandarlos a tu web.
- ⏳ Falta: resultados partido a partido, qué son las listas 3 y 4, y fichar agentes libres (hace falta tu prueba en el juego).

---

## Parte 8 · 17 · Traspaso para otra IA: cómo replicar TODO el trabajo de Liga Máster (PES 2021)

> Origen: `PhoenixSync/base-conocimiento/17-TRASPASO-LIGA-MASTER-PARA-OTRA-IA.md` · 11150 bytes. Copia íntegra.

**Para:** cualquier chat/IA que continúe el trabajo de FRALEX. **Fecha:** 9 oct 2026.
**Si eres una IA nueva, lee este documento completo antes de tocar nada.** Aquí está el método, el orden de lectura, los comandos exactos, las reglas y un prompt de arranque.

---

### 0. Quién es el usuario y cómo hay que tratarlo

- Se llama **FRALEX** (no uses su nombre oficial). Habla español.
- Se marea con muchos números/símbolos juntos. Explica **simple, ordenado, espaciado, paso a paso**, define cada palabra nueva desde cero y **repite lo importante**.
- Quiere **0 errores**: si aparece un error, **no avances** hasta resolverlo.
- Avisa cada cierto tiempo con mensajes cortos de qué estás haciendo.

### 1. Qué es el proyecto (en 6 líneas)

1. **Phoenix Evolution** = web (repo Fralexito/phoenixevolution, Supabase).
2. **Phoenix Link** = fork de Smash Soda con Parsec.
3. **Phoenix Sync** (antes «Phoenix Mercado») = programa en C++ que lee/edita datos de PES 2021. Carpeta `PhoenixSync/` en `C:\dev\smash-soda-fork`, rama `mercado-fase0`.
4. Meta: un parche propio de PES 2021 con juego online propio.
5. Esta guía cubre la **Liga Máster** (archivos `ML0000000N`) y la ficha de jugador (`Player.bin`).
6. Parche de trabajo: **ConmeGOL 26** (guardados en carpeta Steam `239200`). Otros parches (Sudamerican `292733975847239680`, Football Life 27, Evoweb, Gogosz) están **pendientes**.

### 2. Reglas de trabajo (obligatorias)

1. **Nunca** trabajes sobre el original del guardado: siempre una **copia**.
2. **Siempre** haz la ida y vuelta (descifrar y volver a cifrar) antes de editar. Si no sale idéntico, **no sigas**.
3. **Siempre** cierra en un archivo **nuevo**. Jamás sobrescribas.
4. **Anota la huella** (md5/sha256) de cada archivo que entregues. El juego autoguarda sobre la ranura cargada.
5. **Un cambio solo es «probado» cuando FRALEX lo ve en el juego.** Lo medido en archivos es «observado».
6. Mantén **PRUEBAS.md** y **REGISTRO.md** (en `PhoenixSync/`) al día. Mantén la carpeta de Drive `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI` actualizada.
7. Los guardados y los `.bin` descifrados **no se suben a GitHub** (son datos del usuario).
8. Nunca pidas contraseñas ni secretos. Contenido de otros modders solo con permiso y crédito. Nunca distribuyas el juego.
9. Antes de trabajar en el repo: `git fetch` + `git pull --rebase` (hay **dos cuentas de Claude**). **No** hagas `git checkout` a otra rama en `C:\dev\smash-soda-fork`. **No** hagas commit/push si no te lo pide. Nada a `master` sin permiso.
10. La web/Supabase solo se toca con prompts para el chat WEB.
11. No edites archivos del juego ni option files sin respaldo y aprobación.
12. Si el git del puente da «Operation not permitted» al borrar, quedan candados `.git/index.lock`: **muévelos** (`mv`), no los fuerces.

### 3. Qué leer y en qué orden

Todo en `PhoenixSync/base-conocimiento/` (copia en la carpeta de Drive):

| Orden | Archivo | Para qué |
|---|---|---|
| 1 | `00-INDICE.md` | Mapa de todo |
| 2 | **este (17)** | Método y comandos |
| 3 | `10-COMO-SE-ABRIO-EL-GUARDADO.md` | Cómo se abre/cierra el guardado |
| 4 | `09-LIGA-MASTER-COMO-LO-HICIMOS.md` | Camino paso a paso (traspasos, venta, dinero, blob, fichar) |
| 5 | `../liga-master/ESTRUCTURA-ML.md` | Bytes exactos (§1–§23) |
| 6 | `13`, `14`, `15` | Agentes libres/fichas, calendario, tablas de posiciones |
| 7 | `01-MOTOR-PES2021.md` y `16-PLAYER-BIN-FICHA-COMPLETA.md` | Motor y ficha de jugador |
| 8 | `12-GUIA-MULTIPARCHE.md`, `parches/*.md` | Qué cambia entre parches |
| 9 | `11-SUPER-INFORME.md`, `PRUEBAS.md`, `REGISTRO.md` | Estado y diario |

### 4. Entorno y acceso

- La IA corre en la nube (Linux). El PC de FRALEX se alcanza por el **puente de la app de escritorio** (`device_bash`, `device_list_dir`, `device_stage_files`, `device_commit_files`). Si el puente no está, pídele que abra el chat en la app de escritorio.
- Guardados: `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` (`ML00000000`… = ranura N+1 del menú Cargar; `EDIT00000000` = option file). Respaldos en `save\_referencias_mercado\`.
- Carpeta del parche: `D:\Frank\Games_\Conmegol Patch\`. `Player.bin` vivo: `…\SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin`.
- Repo: `C:\dev\smash-soda-fork\PhoenixSync\`. Pasa archivos del PC a la nube con **stage** y de vuelta con **commit** (copian, no mueven).

### 5. Herramientas y comandos exactos

Código en `PhoenixSync/liga-master/herramientas/` (`dec.c`, `info.c`, `enc2.c`) y la biblioteca pública en `terceros/pesxcrypter/` (`crypt.c`, `masterkey.c`, `mt19937ar.c`).

**Compilar** (gcc en Linux; MinGW en Windows), desde `liga-master/herramientas/`:

```
T=../../terceros/pesxcrypter
for t in dec info enc2; do gcc -O2 -o $t $t.c $T/crypt.c $T/masterkey.c $T/mt19937ar.c -I$T; done
```

**Usar:**

```
dec  <ML0000000N> salida.bin reencriptado.bin     # descifra y comprueba la ida y vuelta
info <ML0000000N>                                  # cabecera, tamaños, etiqueta, serial (si dudas de los argumentos, lee info.c)
enc2 <ML original> <datos_editados.bin> <ML_nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'
                                                   # cifra; el 5.º argumento es el texto info (3 líneas) que muestra el menú Cargar
```

Otras piezas (en `PhoenixSync/`): `liga-master/prototipos/*.py` (`mlparse.py`, `mlsquad.py`, `build_c.py`, `build_v6.py`, `ref_v7_juego.py`, `diff_gt.py`, `blob.py`), y el motor C++ definitivo en `core/` (`LigaMaster.cpp`, `Alineacion.h`, `BlobLM.*`, `SobrePes.*`). Ejemplos:

```
python3 prototipos/build_c.py r0.bin v5.bin catalogo.json 154 66 133543 19
python3 prototipos/diff_gt.py base.bin guardado_por_el_juego.bin catalogo.json | head -80
```

**Compilar Phoenix Sync completo** (Windows, raíz del repo; o `COMPILAR_SYNC.bat`):

```
cmake -S PhoenixSync -B build-sync -A x64
cmake --build build-sync --config Release
build-sync\Release\PhoenixSyncPruebas.exe
```

Las pruebas automáticas deben terminar sin fallos (la última vez: 295 de 295).

### 6. El método (repítelo en cada parte nueva)

1. **Copiar** el guardado del PC a la nube (stage). El original no se toca.
2. **Descifrar** con `dec`; exigir ida y vuelta idéntica.
3. **Conseguir una «verdad del juego»:** FRALEX hace la acción real en el juego (fichar, despedir, jugar un partido) y guarda en **otra ranura**.
4. **Comparar byte a byte** el antes y el después; listar qué bytes cambian y dónde.
5. **Reproducir** lo mismo en Python y luego en C++ **por anclas** (no por direcciones fijas) y comprobar que sale el mismo archivo que el juego.
6. **Cifrar** (enc2/SobrePes) con texto visible en el menú Cargar, **anotar la huella**, devolverlo a una ranura **libre** del PC.
7. **FRALEX lo prueba** en el juego. Anotar resultado (bueno o malo) en `PRUEBAS.md`.
8. Añadir una línea a `REGISTRO.md` y, si hay un hallazgo nuevo, documentarlo y avisar a FRALEX.

### 7. Resumen de lo ya descubierto (detalle en los archivos citados)

- **Sobre cifrado:** 5 piezas (cabecera, descripción 384 B, logo 17.315 B, datos ~19,7 MB, serial 44 B). El juego **no** valida la huella de la cabecera. El menú Cargar muestra el **texto info** (desde el byte 128 de la descripción), no el nombre de 128 B.
- **Plantillas:** 700 bloques de 1.680 B desde `0x50` (40 pares `(reg, pid)`, dorsales, contador). City = bloque 154.
- **Alineación de la IA:** 629 bloques de 600 B (orden en `+0x220`, roles en `+0x248`).
- **Tu equipo:** tablas A–H, M, L y lista K; contratos en tabla I (48 B); negociaciones 60 B. ID interno de club = `(ID option << 14) | índice de bloque`.
- **Dinero:** `0xc7dd00` (presupuesto, inicial, tope). Presupuesto salarial = tope − sueldos. Se halla por ancla a `0x97d38` de la tabla A.
- **Blob:** zlib, 18 tramos de 256 KB, 30.000 fichas de 156 B (sueldo `+0x56`, valor `+0x5a`). Jugadores `0xdb65…` usan `reg & 0xffff`. Posición actual fija `0x11403a8` (riesgo en otros parches).
- **Agentes libres:** bit `0x20` en `+0x75` de la ficha + fin de contrato vacío + sin equipo.
- **Calendario:** registros de 32 B en `0x1f0000–0x28c000`. **Tablas:** filas de 20 B en `0xb00000–0xd40000` (actual y anterior). **Resultados partido a partido: no encontrados.**
- **Fichar para el usuario:** programa no escribe el contrato (el juego lo crea desde la ficha del blob).

### 8. Trampas conocidas

| Trampa | Qué hacer |
|---|---|
| Solo editar la lista del club deja al jugador vivo en Estrategia | Tocar todas las tablas del equipo y la lista K |
| Porteros en el ataque | Recalcular orden de formación y 6 roles |
| «DC 0» en blanco | Editar la alineación de 600 B del club de la IA |
| Contrato duplicado | No escribir el contrato al fichar |
| El juego guarda encima de la ranura | Anotar huellas; usar otra ranura |
| Direcciones fijas dejan de servir (el blob crece) | Buscar por anclas |
| Idea errónea: calendario en el blob | No está ahí (zona posterior al blob no cambia) |

### 9. Pendientes

1. Ver en pantalla: agentes libres, calendario, tablas (guías 13, 14, 15).
2. Resultados/goleadores: guardar justo antes y después de un partido el mismo día, en ranuras distintas.
3. Stats dentro de una carrera empezada: reiniciar el juego con un `Player.bin` cambiado.
4. Buscar la posición del blob por ancla; repetir todo en Sudamerican y otros parches.
5. Escribir **un** valor de **un** jugador en una copia de `Player.bin` (con respaldo y aprobación).
6. Aplicar un traspaso aprobado en la web solo a la carrera elegida, con aviso de Phoenix Link.

### 10. Prompt de arranque (pégalo al abrir un chat nuevo)

```
Eres un ingeniero de ingeniería inversa que continúa el proyecto Phoenix Evolution de FRALEX (PES 2021, parche ConmeGOL 26).
Antes de hacer nada: lee PhoenixSync/base-conocimiento/17-TRASPASO-LIGA-MASTER-PARA-OTRA-IA.md completo y luego los archivos en el orden de su sección 3.
Habla en español, simple, ordenado, paso a paso, define cada palabra nueva, repite lo importante, avisa seguido con mensajes cortos. Meta: 0 errores; si hay un error, no avances.
Reglas: trabaja siempre sobre COPIAS, haz ida y vuelta antes de editar, cierra en archivo NUEVO, anota huellas, un cambio es «probado» solo cuando FRALEX lo ve en el juego, actualiza PRUEBAS.md y REGISTRO.md, no hagas commit/push ni checkout de otra rama sin permiso, haz fetch + pull --rebase antes de trabajar en el repo, no subas guardados ni .bin, nunca pidas contraseñas.
Primero confirma al usuario en 5 líneas qué entendiste y qué vas a hacer, y empieza por la tarea que él te pida. Si no sabes algo, dilo: no lo inventes.
```

### 11. Cómo registrar un hallazgo nuevo

1. Etiqueta: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.
2. Escribe: qué hiciste, cómo lo comprobaste, con qué herramienta, en qué guardado, resultado.
3. Añádelo a `PRUEBAS.md`, `REGISTRO.md` y, si procede, a un `NN-GUIA-….md`; copia el doc a la carpeta de Drive.
4. Avisa a FRALEX en lenguaje simple.

---

## Parte 9 · 18 · Guía exacta: ¿llegan las stats de Phoenix-DB a una Liga Máster? (prueba y búsqueda)

> Origen: `PhoenixSync/base-conocimiento/18-GUIA-STATS-EN-LIGA-MASTER.md` · 5493 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 23:50 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25. Diario: `PRUEBAS.md` (9 oct, 23:50).

Símbolos: ✅ comprobado · ❌ comprobado que no · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

### 0. En simple

- En **amistosos**, lo que ponemos en Phoenix-DB **sí** se ve: Lamine con **Velocidad 99**. ✅
- En una **Liga Máster ya empezada**, **no** se ve: Lamine sigue con **Velocidad 90**, aunque reinicies el juego. ❌
- Cada carrera tiene **su propia copia** de las habilidades de los jugadores, y además la **va cambiando** con el tiempo (Pase raso 83 en vez de 82, Contacto 77 en vez de 76).
- Todavía **no** encontramos dónde guarda la carrera esa copia. Para encontrarla hace falta un experimento sencillo (sección 4).

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tus 2 capturas (Lamine en Liga Máster y en amistoso) | El resultado de la prueba |
| Tu PC (solo leer): Phoenix-DB y `sider.log` | Ver qué `Player.bin` había (v99) y qué leyó el juego, y cuándo |
| El «espía» de Sider (lo que ya registraba `phoenix.lua`) | Saber qué archivos de la base lee el juego al entrar a la Liga Máster |
| `~/phx/cpkls.py` (nuevo, en tu PC) | Leer **solo el índice** de los paquetes de Konami, sin cargar los gigas |
| Python 3 + NumPy (en la nube) | Buscar la copia de las habilidades en tus guardados |

---

### 2. Paso a paso

#### Paso 1 · Comprobar qué había en Phoenix-DB
- **Cómo:** se calculó la huella del `Player.bin` de Phoenix-DB en las **dos** carpetas (juego y modo) y se comparó con las versiones de prueba.
- **Resultado:** las dos eran la **v99** (Lamine con Velocidad 99), colocada a las 5:51 de la mañana. ✅

#### Paso 2 · Comprobar que el juego la leyó
- **Cómo:** en `sider.log` se vio que el juego arrancó a las **23:00** y leyó `player.bin` desde la raíz **Phoenix-DB**.
- **Resultado:** el juego tenía la v99 cargada. ✅

#### Paso 3 · Tu prueba en el juego
| Dónde | Velocidad de Lamine | Media |
|---|---|---|
| Amistoso | **99** ✅ | 90 |
| Liga Máster (Lamine en el Barça) | **90** ❌ | 88 |

- **Conclusión:** la Liga Máster **no** usa el `Player.bin` de Phoenix-DB para los jugadores de una carrera ya empezada.

#### Paso 4 · ¿Qué lee el juego al entrar a la Liga Máster?
- **Cómo:** el registro del espía mostró que al entrar a la Liga Máster (23:17 y 23:21) el juego leyó **`installversionplayer.bin`**, y no `player.bin`.
- Ese archivo **no** viene en ConmeGOL. Con `cpkls.py` se encontró en los paquetes de Konami: el más nuevo es `download\dt80_700E_x64.cpk`, de 13.223 bytes, el **mismo tamaño** que leyó el juego.
- Se copió solo ese trocito y se abrió (WESYS + zlib).
- **Resultado:** son **7.426 parejas «jugador → versión»** (por ejemplo, versión 100). **No** trae habilidades y **Lamine no está**. 🔎
- **Conclusión:** no es la copia que buscamos. Parece una lista de «qué versión de datos usa cada jugador».

#### Paso 5 · Buscar la copia dentro del guardado
Se buscaron los valores de tu captura (83, 89, 93, 92, 83, 84, 81, 62…) de muchas formas. **Ninguna dio resultado:**

| Dónde se buscó | Cómo | Resultado |
|---|---|---|
| Todo el guardado | Trozos de la ficha original de Lamine de `Player.bin` | ❌ no hay copia literal |
| Las 12 fichas de tu equipo (incluida la gigante «M») | Valores «empaquetados» de 5 a 8 bits, en el orden de la pantalla, con margen de ±1 | ❌ |
| Su ficha de 596 bytes | Igual | ❌ |
| Su carnet del blob (156 bytes) | Ya se había buscado antes | ❌ |

- Solo apareció su **curva de medias** (81, 82, 83, 84…), que ya conocíamos: no son las habilidades.

---

### 3. Multiparche 🧩
- Que la Liga Máster use **su propia copia** lo hace **el juego**: pasará igual en todos los parches.
- `InstallVersionPlayer.bin` viene de **Konami**, no del parche: es igual en todos los parches que usen los mismos paquetes de Konami.

---

### 4. El experimento que falta (para encontrar la copia)

**Idea:** hacer **dos carreras nuevas iguales**. En una, Lamine tiene Velocidad 99; en la otra, 90. Lo único distinto entre los dos guardados será **dónde está esa velocidad**.

1. **Con el juego como está ahora (v99):** crea una **carrera nueva** (por ejemplo, con el Barça y los ajustes por defecto). Mira la Velocidad de Lamine y **guarda en una ranura libre** (la 4).
2. **Cierra el juego y avísame.** Yo pongo en Phoenix-DB la versión original (**v90**), con respaldo.
3. **Abre el juego** y crea **la misma carrera** (mismo club, mismos ajustes). Mira a Lamine y **guarda en otra ranura libre** (la 5).
4. Yo comparo las dos carreras byte a byte.

**Lo que nos dice también:** si en la carrera nueva del paso 1 Lamine sale con **99**, entonces la carrera **copia** las stats de Phoenix-DB **al crearse**. Eso ya es muy útil: con una carrera nueva tus cambios sí entran.

---

### 5. Qué significa para tu proyecto, hoy

- Stats nuevas en **amistosos, salas y modos normales:** ✅ funcionan (Phoenix-DB + Editar → Cargar o reiniciar).
- Stats nuevas en una **carrera ya empezada:** ❌ todavía no. Hace falta encontrar la copia (sección 4) y escribir en ella.

---

## Parte 10 · 19 · Guía exacta: verificación en pantalla del calendario, la tabla y los goleadores

> Origen: `PhoenixSync/base-conocimiento/19-GUIA-VERIFICACION-EN-PANTALLA.md` · 3206 bytes. Copia íntegra.

**Para FRALEX · 9 de octubre de 2026, 23:45 (Lima).**
Cómo se comprobó en el juego que lo que Phoenix Sync lee del archivo es **lo mismo** que muestra la pantalla.
Viene de las guías 14 (calendario), 15 (tablas) y 16 (goleadores). Diario: `PRUEBAS.md` (9 oct, 23:45).

---

### 0. En simple

Antes, el calendario, las tablas y los goleadores estaban «vistos en archivos» 🔎.
Ahora tú los miraste en el juego y **coinciden uno por uno**. Pasan a **probado** ✅.

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu carrera del **Barça** (ranura 1, 4/3/2026), copiada a la nube sin tocar el original | De ahí se leyeron los datos |
| El lector de temporada (`core/TemporadaLM`, guía 16) y los scripts de Python | Leer tabla, goleadores y calendario |
| Tus 4 capturas: goleadores de la Ligue 1, tabla de la Ligue 1, pantalla principal de la carrera y la ficha de Lamine | Comparar con la pantalla |

---

### 2. Paso a paso

#### Paso 1 · Leer del archivo (antes de ver tus capturas)
Se dejó escrito en la guía 16 lo que el archivo decía, **antes** de que tú miraras el juego. Así la comparación es justa.

#### Paso 2 · Comparar los goleadores
| Jugador | Club | Archivo | Pantalla |
|---|---|---|---|
| Hamza Igamane | Lille | 8 | 8 ✅ |
| Odsonne Édouard | Lens | 8 | 8 ✅ |
| Arthur Avom | Lorient | 8 | 8 ✅ |
| Sofiane Diop | Niza | 5 | 5 ✅ |
| Joaquín Panichelli | Estrasburgo | 5 | 5 ✅ |

#### Paso 3 · Comparar la tabla
| Puesto | Club | Archivo (Pts · G-E-P · GF-GC) | Pantalla |
|---|---|---|---|
| 1 | Lille | 22 · 7-1-0 · 19-4 | ✅ igual |
| 2 | PSG | 20 · 6-2-0 · 17-4 | ✅ igual |
| 3 | Lens | 19 · 6-1-1 · 20-8 | ✅ igual |
| 4 | Lorient | 16 · 5-1-2 · 19-11 | ✅ igual |
| 5 | Marsella | 14 · 4-2-2 · 11-8 | ✅ igual |
| 6 | Mónaco | 14 · 4-2-2 · 9-6 | ✅ igual |

- **Las flechas también encajan:** el juego muestra que Marsella y Mónaco **suben** y que Lyon y Niza **bajan**. En la segunda tabla guardada (la de la jornada anterior), Niza era 5.º y Lyon 6.º. Eso confirma que esa segunda tabla es la **anterior**. ✅

#### Paso 4 · Comparar LaLiga y el calendario
- La tabla de LaLiga en el juego está **toda en 0**, en el **mismo orden** que en el archivo (Barça, Real Sociedad, Deportivo, Celta…). ✅
- Tu próximo partido de LaLiga es **Barça – Real Sociedad, el 11/4/2026, en casa**: es la **jornada 1** que se encontró en el archivo. ✅

---

### 3. Resultado

| Qué | Antes | Ahora |
|---|---|---|
| Calendario (guía 14) | 🔎 | ✅ probado |
| Tablas de posiciones (guía 15) | 🔎 | ✅ probado |
| Goleadores (guía 16) | 🔎 | ✅ probado |
| Asistencias | 🔎 | 🔎 (falta mirar esa pantalla) |

---

### 4. Multiparche 🧩
- Probado en **ConmeGOL 26**, en dos carreras distintas (City y Barça). En otro parche hay que repetir esta misma comparación una vez (guía 12).

---

### 5. Qué abre
- Phoenix Sync puede mandar a tu web, **sin capturas**, la tabla, los goleadores y el calendario de cualquier liga de una carrera. ✅

---

## Parte 11 · 20 · Bitácora completa de la sesión del chat «Player.bin» (9 y 10 de octubre de 2026)

> Origen: `PhoenixSync/base-conocimiento/20-BITACORA-SESION-9-10-OCT-2026.md` · 8473 bytes. Copia íntegra.

**Para FRALEX.** Registro cronológico de todo lo que se hizo, con herramienta, resultado y dónde quedó guardado. Etiquetas: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ⚠️ riesgo.
Cuenta/chat: «Cuenta A (Player.bin)». Fecha de zona horaria: Lima.

---

### 1. Resumen en 8 líneas

1. Se calibró **Player.bin** (la ficha de cada jugador) con capturas del juego (experimento «Vestuario Barça»): 20 estadísticas, 5 de portero, pie malo/forma/lesiones, 13 posiciones y unas 45 habilidades/COM.
2. Se retiró la idea de que los ±1 dependen de la edad. Explicación más probable (sin confirmar): la Liga Máster guarda su propia copia de los jugadores.
3. Se leyeron los documentos de Drive y GitHub: el proyecto ahora se llama **Phoenix Sync** y avanzó mucho más de lo que decía el primer informe.
4. Se actualizó el informe en Docs y se guardó la ficha completa de Player.bin en repo, Drive y Proyecto.
5. Se escribió la **guía de cómo se descubrió la Liga Máster** (herramientas y 24 pasos) y el **traspaso para otra IA** (documento 17).
6. Se hizo el **panorama completo** (documento 18) uniendo lo de los otros chats con lo de este.
7. Se investigó **cuántos equipos puede tener una liga** (documento 19): hasta 30 ya funciona; más de 30 es posible pero no probado.
8. No se escribió nada en Player.bin, ni en archivos del juego, ni en guardados. No se hizo commit ni push.

### 2. Línea de tiempo

| # | Qué se hizo | Herramienta | Resultado |
|---|---|---|---|
| 1 | Calibrar Player.bin con capturas (Gordon, Balde, Bernal, Eric García, Cancelo, Szczęsny, Isak, Mbappé, Dembélé, João Pedro ×2, Pedro) | Python sobre `Player.bin` descifrado (29.997 × 312 B), búsqueda por 20 estadísticas con tolerancia ±1 | ✅ campos mapeados; IDs de jugador localizados |
| 2 | Resolver habilidades especiales (bits 480–531) | Comparar bits encendidos con la pantalla de cada jugador | ✅ unas 45 resueltas; sin resolver: Cortada, bit 514, COM Centrador, 479/516, 498 |
| 3 | Retirar la hipótesis de la edad | Comparar Gordon/Balde/Bernal/Eric García | Cuatro jugadores casi idénticos ⇒ la edad no explica los ±1 |
| 4 | Leer Drive (carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`, ~45 docs) y repo `PhoenixSync/` | Drive MCP, `device_bash` | Descubierto el cambio de nombre a **Phoenix Sync** |
| 5 | Arreglar el candado de git | `device_bash`: `mv .git/index.lock .git/stale-index-lock-claude` | ✅ sin borrar nada; rama `mercado-fase0` en el commit `540fa1a`; archivos 08–15 sin seguir |
| 6 | Guardar la ficha de Player.bin | Nuevo `16-PLAYER-BIN-FICHA-COMPLETA.md` en repo; Drive; Proyecto | ✅ 3 copias |
| 7 | Actualizar el informe de Docs | Docs MCP: corregir ±1, añadir sección de clasificación por parche | ✅ informe `383547a4…` rev 9 |
| 8 | Guía «cómo descubrimos la Liga Máster» | Docs MCP, a partir de docs 08, 09, 10, 13, 14, 15 | ✅ `20a8e26b…` |
| 9 | Documento 17: traspaso para otra IA | Escrito, copiado al repo y al Proyecto, y a Drive | ✅ 11.150 B, tamaño idéntico en repo y Proyecto |
| 10 | Documento 18: panorama completo | Drive | ✅ |
| 11 | Investigación «más equipos por liga» | Ver `19-INVESTIGACION-MAS-EQUIPOS-POR-LIGA.md` | ✅ informe en Docs `4884bc86…` + documento 19 |
| 12 | Este registro (20) y el 19 | Repo, Drive, Proyecto | ✅ |

### 3. Procedimiento de calibración de Player.bin (para repetirlo)

1. Poner la captura del jugador en pantalla (1/9 a 9/9).
2. Leer los 20 valores de estadística.
3. Recorrer los 29.997 registros: el registro es un entero little-endian; campo = `(registro >> bit) & máscara`; las estadísticas ocupan 6 bits y el valor mostrado es el crudo + 40.
4. Elegir el registro cuyos 20 valores coinciden (tolerancia ±1).
5. Listar los bits 480–531 encendidos y compararlos con las habilidades que muestra la pantalla; repetir con varios jugadores para resolver cada bit.

Mapa completo de bits: ver `16-PLAYER-BIN-FICHA-COMPLETA.md`.

### 4. Errores y correcciones durante la sesión

| Error | Corrección |
|---|---|
| Actualizar el informe con el identificador corto de bloque (`.62`) fue rechazado | Usar el identificador completo (`sesión.reloj`) |
| La carpeta `PhoenixMercado` no existía | Estaba renombrada a `PhoenixSync` |
| La búsqueda por igualdad exacta no encontraba a Mbappé, Dembélé ni João Pedro | Usar tolerancia ±1 |
| Un mensaje del usuario («A») no se entendió | Se pidió aclararlo |
| `git merge --ff-only` dejó un candado `.git/index.lock` | Se movió el candado; no se forzó |
| Primera versión de mis respuestas: «el juego muestra +1» y plantilla del Barça vieja | Corregido con el usuario antes |
| Un comando `md5sum` con `grep -r` sobre el parche agotó el tiempo | Se repitió por partes con `sha256sum` (3,7 s) |
| El proceso en segundo plano se perdió entre llamadas | Se ejecutó cada paso en una sola llamada |
| Texto del documento 19: «MLS en 0xB531B8» no estaba verificado | Verificado después: coincide exactamente con la MLS (30 de 30) |
| Texto del documento 19: «Argentina 2.ª en 0xB1FD68» era falso (es la Copa Colombia) | Verificado con la base de datos: la 2.ª argentina está en 0xB2D058; corregido en repo y Proyecto |
| Los documentos 17, 18 y 19 en Drive mostraban símbolos rotos (🔎, 🧩, 💭 salían como «ð») | Se subieron versiones corregidas con palabras en vez de esos símbolos; los viejos se renombraron «OBSOLETO» |

### 5. Dónde quedó todo guardado

**Repo local** (`C:\dev\smash-soda-fork\PhoenixSync\base-conocimiento\`, sin commit ni push):
`16-PLAYER-BIN-FICHA-COMPLETA.md` · `17-TRASPASO-LIGA-MASTER-PARA-OTRA-IA.md` · `19-INVESTIGACION-MAS-EQUIPOS-POR-LIGA.md` · `20-BITACORA-SESION-9-10-OCT-2026.md`. Líneas nuevas en `REGISTRO.md` y `PRUEBAS.md`.

**Drive** (carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`): documentos 16 (`1TXCzG5tUfLHUv3QKkOzZRlBKbpC68LpxUf7WRgLWh3o`), 17 (`1wYHK1gcjkWHvA_en8AQAgEpXQCvZwj1dmhSYJbKjLQ0`), 18 (`1Ptoq3ntmBOR_DeWapO_PO9qPu0B-xn8FHmcI0218DLg`), 19 y 20.

**Proyecto «Phoenix Evolution FADG»:** `claude/player-bin-ficha-completa.md`, `claude/traspaso-liga-master-para-otra-ia.md`, y copias de 19 y 20.

**Docs de Claude:**
- Informe actualizado: https://claude.ai/code/artifact/383547a4-cf70-474b-9d12-030a816d9e43
- Guía de la Liga Máster: https://claude.ai/code/artifact/20a8e26b-3e35-467c-bad6-0634a51c0715
- Investigación de equipos por liga: https://claude.ai/code/artifact/4884bc86-e1fa-49f8-9704-07aeb714d484

### 6. Resultados de la investigación de equipos por liga (resumen; detalle en el 19)

- La base de datos (`CompetitionEntry.bin`, 1.359 filas de 12 B: equipo · clave · `(puesto<<8)|competición`) ya define **Argentina 1.ª con 30 equipos, MLS con 30, Argentina 2.ª con 36, Copa Argentina con 66**.
- El guardado de Liga Máster guarda cada lista en bloques de **750 puestos**; los números coinciden con la base de datos.
- El .exe actual es idéntico al de `Sudamerican_Backup`; contra `Conmegol Backup` solo cambian 15 bytes de cabecera y la tabla de exportaciones.
- Otros parches indican que el problema está en el **calendario de la Liga Máster y la 2.ª temporada**.
- Posibilidades (estimación mía): hasta 30 ya funciona · 31–36: 40–55% · 37–48: 20–35% · más de 48: menos de 15%.
- Plan de prueba de 30 a 32 equipos: preparado, **esperando aprobación de FRALEX**.

### 7. Pendientes (todos)

1. Aprobar y ejecutar la prueba de 30 a 32 equipos.
2. Descifrar `CompetitionRegulation.bin` (dónde está el número de equipos por etapa).
3. Ver en pantalla agentes libres, calendario y tablas de posiciones (guías 13, 14, 15).
4. Encontrar los resultados partido a partido: guardar justo antes y después de un partido, el mismo día, en ranuras distintas.
5. Confirmar la causa de los ±1 de Player.bin (reiniciar el juego con un `Player.bin` cambiado y cargar una carrera).
6. Escribir **un** valor de **un** jugador en una copia de `Player.bin`, con respaldo y aprobación.
7. Resolver Cortada, el bit 514, COM Centrador, Rol, Personalidad, tácticas y la curva de progreso.
8. Actualizar la página «Vestuario Barça» (ya no están Livakovic, Hamza Abdelkarim ni Gabriel Jesús).
9. Repetir lo mismo en Sudamerican 2026, Football Life 27, Evoweb y Gogosz.
10. Decidir cuándo hacer commit y push de los documentos nuevos (no se hizo).

---

## Parte 12 · 20 · Traspaso para otra IA (versión 2): cómo continuar TODO el trabajo de Liga Máster (PES 2021)

> Origen: `PhoenixSync/base-conocimiento/20-TRASPASO-LIGA-MASTER-v2.md` · 17722 bytes. Copia íntegra.

**Para:** cualquier chat o IA que continúe el trabajo de FRALEX. **Fecha:** 9 de octubre de 2026, 23:55 (Lima).
**Versión 2.** Actualiza la guía de traspaso anterior (la que está en el proyecto de claude.ai como `claude/traspaso-liga-master-para-otra-ia.md`). La versión 1 **no se tocó**; esta la reemplaza en contenido.
**Si eres una IA nueva, lee este documento completo antes de tocar nada.**

---

### 0. Quién es el usuario y cómo hay que tratarlo

- Se llama **FRALEX** (no uses su nombre oficial). Habla español.
- Se marea con muchos números o símbolos juntos. Explica **simple, ordenado, espaciado y paso a paso**. Define cada palabra nueva desde cero y **repite lo importante**.
- Quiere **0 errores**: si aparece un error, **no avances** hasta resolverlo.
- Avisa cada cierto tiempo, con mensajes cortos, de qué estás haciendo.

---

### 1. Qué es el proyecto (en 6 líneas)

1. **Phoenix Evolution** = la web (repo Fralexito/phoenixevolution, Supabase).
2. **Phoenix Link** = la app de PC (fork de Smash Soda con Parsec). Carpeta `SmashSoda/`, rama `rediseño-phoenix-portal`. **No es de este frente.**
3. **Phoenix Sync** (antes «Phoenix Mercado») = programa en C++ que lee y edita los datos de PES 2021. Carpeta `PhoenixSync/` del repo Fralexito/smash-soda-fork, rama **`mercado-fase0`**.
4. Meta: un parche propio de PES 2021, conectado a la web, con liga online propia.
5. Esta guía cubre la **Liga Máster** (archivos `ML0000000N`) y su relación con la base (`Player.bin`).
6. Parche de trabajo: **ConmeGOL 26** (guardados en la carpeta Steam `239200`). Todo debe pensarse **multiparche** (sección 9).

---

### 2. Reglas de trabajo (obligatorias)

#### Sobre los archivos
1. **Nunca** trabajes sobre el original de un guardado: siempre una **copia**.
2. **Siempre** haz la ida y vuelta (descifrar y volver a cifrar) antes de editar. Si no sale idéntico, **no sigas**.
3. **Siempre** cierra en un archivo **nuevo**. Jamás sobrescribas.
4. **Anota la huella** (md5/sha256) de cada archivo que entregues, y **compruébala en el PC** después de copiarlo. El juego autoguarda encima de la ranura cargada.
5. Los guardados y los `.bin` descifrados **no se suben a GitHub**: son datos del usuario.
6. No edites archivos del juego ni option files sin **respaldo** y aprobación de FRALEX. «Si algo se rompe, mejor no lo hagas».

#### Sobre lo que se da por bueno
7. Un cambio solo es **«probado» (✅)** cuando FRALEX lo ve en el juego. Lo medido en archivos es **«observado» (🔎)**.

#### Sobre los registros (decisión de FRALEX, 9 oct)
8. **Anota siempre**, sin que te lo pida: cada prueba en el juego o análisis va **al final** de `PhoenixSync/PRUEBAS.md`, y cada cambio, en **una línea** al final de `PhoenixSync/REGISTRO.md`.
9. **Nunca borres ni sobrescribas** nada en esos archivos ni en los documentos: solo **añadir al final**, o crear un archivo **nuevo** con otro nombre.
10. **GitHub:** FRALEX pidió mantenerlo **actualizado**. Haz commit y push de lo que añadas (documentos, código que compila, registros) en la rama `mercado-fase0`. Nada a `master` sin permiso.
11. **Drive:** después de **cada avance importante**, escribe una **guía exacta** (qué hiciste, cómo lo comprobaste, con qué recurso) en `PhoenixSync/base-conocimiento/NN-GUIA-….md` y súbela como **documento nuevo** a la carpeta de Drive `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`. Fuera de eso, **no subas nada al Drive** si FRALEX no lo pide. Nunca reemplaces un documento del Drive.
12. Las guías siempre **multiparche**: separa lo que pone el juego de lo que pone el parche (🧩) y escribe los riesgos.

#### Sobre el repositorio
13. Antes de trabajar: `git fetch` + `git pull --rebase`. Hay **dos cuentas de Claude** trabajando en el mismo repo.
14. Si al subir hay conflicto en `PRUEBAS.md` o `REGISTRO.md`: **se quedan las dos partes** (primero la de la otra cuenta y luego la tuya). Nunca descartes la otra parte.
15. **No** hagas `git checkout` de otra rama en `C:\dev\smash-soda-fork` (Link compila en el worktree `_phoenix-link`). **No** toques `SmashSoda/`.
16. **Compila sin errores** antes de cada commit de código, y corre las pruebas automáticas.
17. La web y Supabase **solo** se tocan con prompts para el chat WEB.
18. Nunca pidas contraseñas ni secretos. Contenido de otros modders solo con permiso y crédito.
19. Si el git del puente da «Operation not permitted» al borrar y quedan candados `.git/index.lock`: **muévelos** (`mv`), no los fuerces.

#### Numeración de guías
20. Antes de crear una guía, mira qué números existen: las dos cuentas numeran en la misma carpeta. Ya hay **dos «17»** (el traspaso v1 se pensó como 17, y en el repo está `17-OPCION-EN-VIVO-DONDE-ESTA.md`) y **dos «19»** (`19-GUIA-VERIFICACION-EN-PANTALLA.md` y `19-OPCION-EN-VIVO-PASO-A-PASO.md`). No se renombraron para no romper nada. Usa siempre el siguiente número libre.

---

### 3. Qué leer y en qué orden (todo en `PhoenixSync/base-conocimiento/`, copia en el Drive)

| Orden | Archivo | Para qué |
|---|---|---|
| 1 | `00-INDICE.md` | Mapa general |
| 2 | **este (20)** | Método, reglas, estado y comandos |
| 3 | `10-COMO-SE-ABRIO-EL-GUARDADO.md` | Cómo se abre y cierra un guardado |
| 4 | `09-LIGA-MASTER-COMO-LO-HICIMOS.md` | Traspasos, venta, dinero, blob, fichar (paso a paso) |
| 5 | `08-LIGA-MASTER-INFORME.md` | Resumen y guía de uso |
| 6 | `13` → `14` → `15` → `16` → `18` → `19-GUIA-VERIFICACION-EN-PANTALLA` | Agentes libres y fichas, calendario, tablas, goleadores y lector de temporada, stats en la LM, verificación en pantalla |
| 7 | `../liga-master/ESTRUCTURA-ML.md` (§1–§25) | Bytes exactos de todo |
| 8 | `01-MOTOR-PES2021.md` y `05-PUERTA-EN-VIVO.md` | Motor y base de datos (Player.bin, Editar → Cargar) |
| 9 | `12-GUIA-MULTIPARCHE.md` y `parches/*.md` | Qué cambia entre parches |
| 10 | `11-SUPER-INFORME.md`, `../PRUEBAS.md`, `../REGISTRO.md` | Estado general y diarios |

---

### 4. Entorno y acceso

- La IA corre en la nube (Linux). El PC de FRALEX se alcanza por el **puente de la app de escritorio** (`device_bash`, `device_list_dir`, `device_stage_files`, `device_commit_files`). En la VM del puente, las carpetas están en `$HOME/mnt/`: `239200--save`, `Conmegol Patch`, `_PhoenixMercado_prueba`, `SP2026`… Si el puente no está, pide a FRALEX abrir el chat en la app de escritorio.
- **Guardados:** `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` → `ML00000000` = ranura 1 del menú Cargar, `ML00000001` = ranura 2… (en hexadecimal: la ranura 11 es `ML0000000A`). `EDIT00000000` = option file. Respaldos en `save\_referencias_mercado\`.
- **Carpeta del juego/parche:** `D:\Frank\Games_\Conmegol Patch\`. Base que usa de verdad el juego: la que sirve Sider (`SiderAddons\olmosjr23\Database\…\Player.bin`), y **antes** la nuestra: `SiderAddons\livecpk\Phoenix-DB\…` (también en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\…`).
- **Repo en el PC:** `C:\dev\smash-soda-fork\PhoenixSync\`.
- **Estado del PC al cerrar esta versión (9 oct, 23:55):** Phoenix-DB tiene el `Player.bin` **v99** de prueba (Lamine 162114, Velocidad 99) en las dos carpetas. Ranura 1 = carrera del **Barça** (4/3/2026, md5 7b800612…); ranuras 2 y 3 = carrera del **City** (21/8/2026).

---

### 5. Herramientas y comandos

**Cifrado** (biblioteca libre `terceros/pesxcrypter/`; herramientas en `liga-master/herramientas/`):
```
T=../../terceros/pesxcrypter
for t in dec info enc2; do gcc -O2 -o $t $t.c $T/crypt.c $T/masterkey.c $T/mt19937ar.c -I$T; done
dec  <ML0000000N> salida.bin reencriptado.bin     # descifra y comprueba la ida y vuelta
info <ML0000000N>                                  # cabecera, tamaños, etiqueta, serial
enc2 <ML original> <datos.bin> <ML nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'   # el 5.º = texto del menú Cargar
```
`dec` a veces termina con error **después** de escribir los archivos: comprueba la ida y vuelta, no el código de salida.

**Prototipos Python** (`liga-master/prototipos/`): `mlparse.py`, `mlsquad.py`, `build_c.py`, `build_v6.py`, `ref_v7_juego.py`, `diff_gt.py`, `blob.py`.

**Motor C++** (`core/`): `LigaMaster.*` (traspasos, venta, fichaje para el usuario, dinero), `Alineacion.h`, `BlobLM.*` (blob: lectura, reescritura, ficha por jugador), `SobrePes.*` (cifrar con cualquier tamaño, nunca sobrescribe), **`TemporadaLM.*`** (solo lectura: calendario, tablas, goleadores), `OptionFile.*`, `BaseDatosParche.*` (CPK y Player.bin), `Sincronizacion.*` (cambios firmados de la web).

**Compilar y probar:**
```
cmake -S PhoenixSync -B build-sync -A x64        (Windows)   ·   cmake -S PhoenixSync -B <carpeta> (Linux)
cmake --build build-sync --config Release
PhoenixSyncPruebas                                          → 211/211 sin archivos
PM_ML_DATOS=<ML descifrado> PM_CATALOGO=catalogo.json PhoenixSyncPruebas   → 225–234 según el guardado
```
Para Windows desde Linux: `x86_64-w64-mingw32-g++ -std=c++20` (necesita C++20).

**En el PC (VM del puente):** `~/phx/cpkls.py <texto> <archivos.cpk>` lista el índice de un CPK sin cargarlo entero.

---

### 6. El método (repítelo en cada parte nueva)

1. **Copiar** el guardado del PC a la nube (stage). El original no se toca.
2. **Descifrar** con `dec` y exigir ida y vuelta idéntica.
3. **Conseguir una «verdad del juego»:** FRALEX hace la acción real (fichar, despedir, jugar) y guarda en **otra ranura**.
4. **Comparar byte a byte** el antes y el después.
5. **Reproducir** lo mismo en Python y luego en C++, **buscando por la forma** (anclas), nunca por direcciones fijas, hasta que salga igual que el juego.
6. **Cifrar** con texto visible en el menú Cargar, **anotar la huella** y dejarlo en una ranura **libre** del PC.
7. **FRALEX lo prueba** en el juego. Anotar el resultado, bueno o malo.
8. Registrar (sección 2, reglas 8–11) y avisar a FRALEX en lenguaje simple.

**Para leer datos** (sin editar) vale también: leer del archivo, **escribir la predicción antes** y que FRALEX compare con la pantalla (así se probó la guía 19).

---

### 7. Estado de lo descubierto (9 oct, 23:55)

| Tema | Estado | Dónde |
|---|---|---|
| Abrir y cerrar el guardado (el juego no valida la huella de cabecera; menú Cargar = texto info) | ✅ | guía 10 |
| Traspaso entre dos equipos de la IA (plantilla + alineación de 600 B) | ✅ | guía 09, §3 y §10 |
| Vender de tu equipo a la IA (igual que el juego) | ✅ | guía 09, §17 |
| Fichar para tu equipo desde la IA (el contrato **no** se escribe: lo crea el juego) | ✅ | guía 09, §19–§20 |
| Presupuesto y tope salarial (por ancla a `0x97d38` de la tabla A) | ✅ | §13, §18 |
| Valor de mercado (ficha del blob) | ✅ | §15 |
| **Blob** = solo fichas de jugador: cabecera + **30.000** plazas de 156 B | ✅ (archivos) | guía 13, §21 |
| Blob localizado **por su forma** (ya no por la dirección fija 0x11403a8) | ✅ (pruebas) | §21 |
| Jugadores con **prefijo** en su número (**cambia en cada carrera**: 0xdb65 en el City, 0xdbdf en el Barça): ficha en `0x1e + 156·(reg & 0xffff)` | ✅ (pruebas) | §21, §24 |
| Agentes libres: bit 0x20 de +0x75 + fin de contrato vacío + sin equipo | 🔎 | guía 13 |
| **Calendario** (registro de partido de 32 B, número global, competición, jornada) | ✅ **en pantalla** | guías 14 y 19, §22 |
| **Tablas de posiciones** (fila de 20 B; actual + jornada anterior) | ✅ **en pantalla** | guías 15 y 19, §23 |
| **Goleadores** (fila de 20 B) | ✅ **en pantalla** | guías 16 y 19, §24 |
| Asistencias (2.ª lista) | 🔎 | §24 |
| Listas 3 y 4 de cada competición | ⏳ sin identificar | §24 |
| Lector `TemporadaLM` (calendario, tablas, goleadores; cruces 35/35, 152/152, 38/38) | ✅ (pruebas) | guía 16 |
| **Stats de Phoenix-DB en una carrera YA empezada** | ❌ **no llegan** (Lamine 90 en la carrera, 99 en amistoso) | guía 18, §25 |
| Dónde guarda la carrera su copia de las habilidades | ⏳ no encontrada | guía 18, §25 |
| `InstallVersionPlayer.bin` (lo lee el juego al entrar a la LM) = 7.426 «jugador → versión», sin habilidades | 🔎 | §25 |
| Resultados partido a partido | ⏳ no encontrados (se deducen de tabla + calendario en la última jornada) | §23 |
| Fichar un agente libre | ⏳ falta la verdad del juego | guía 13 |

---

### 8. Trampas conocidas

| Trampa | Qué hacer |
|---|---|
| Solo editar la lista del club deja al jugador vivo en Estrategia | Tocar todas las tablas de tu equipo y la lista K |
| Porteros en el ataque | Recalcular orden de formación y los 6 roles |
| Jugador en blanco «DC 0» | Editar la alineación de 600 B del club de la IA |
| Contrato duplicado | No escribir el contrato al fichar |
| El juego guarda encima de la ranura cargada | Anotar huellas y usar otra ranura |
| Direcciones fijas que dejan de servir | Buscar por la forma (anclas) |
| Creer que el calendario está en el blob | No está: el blob es solo fichas |
| Suponer que el prefijo de jugador es siempre 0xdb65 | Cambia en cada carrera |
| Creer que Phoenix-DB cambia una carrera empezada | No: solo amistosos y modos que leen la base |
| El panel derecho de la ficha de un jugador | Muestra el equipo que estás mirando (por ejemplo, tu próximo rival), no el club del jugador |
| Dos cuentas numerando guías | Mirar los números libres antes de crear una guía |

---

### 9. Multiparche (resumen de la guía 12)

- **Lo pone el juego (igual en todos):** el cifrado, el formato del option file, de `Player.bin` y de la Liga Máster, Editar → Cargar, el autoguardado, la regla de «Datos Actual. en vivo», que la Liga Máster tenga su propia copia de las habilidades.
- **Lo pone el parche (🧩):** la carpeta de guardados, dónde está la base de verdad, los IDs y la cantidad de equipos y jugadores, los números de competición, si usa Sider (y cuántos `sider.ini`), si trae exe modificado.
- **Estado por parche:** ConmeGOL 26 ✅ · Sudamerican 2026 🔎 (option file y emparejamiento) · ConmeGOL 27, Football Life 27, Evoweb, Gogosz ⏳.

---

### 10. Pendientes, en orden

1. **Experimento de stats en la Liga Máster** (guía 18, sección 4): FRALEX crea una carrera nueva con Phoenix-DB **v99** y guarda en la ranura 4; luego se pone la **v90** (con respaldo) y crea **la misma** carrera en la ranura 5; se comparan. Si en la primera Lamine sale con 99, las stats **sí entran al crear** una carrera.
2. **Resultados partido a partido:** guardar justo antes y justo después de un partido, el mismo día, en ranuras distintas.
3. **Fichar un agente libre:** FRALEX ficha uno en el juego y guarda en otra ranura.
4. Ver en pantalla las **asistencias** y averiguar las listas 3 y 4.
5. Repetir lo esencial en **Sudamerican** y en las versiones nuevas de los parches.
6. Llevar a la web lo que ya se lee (tablas, calendario, goleadores) y aplicar los traspasos aprobados en la web a la carrera elegida, con aviso de Phoenix Link.

---

### 11. Límite que NO se cruza

No se ayuda a modificar el ejecutable del juego ni su código en memoria para saltarse sus protecciones (por ejemplo, la puerta de inicio de sesión de «Datos Actual. en vivo»), ni a construir instaladores, conmutadores o documentación de ese camino. Todo lo de esta guía es **edición de archivos de datos y guardados** (modding normal). El camino recomendado para aplicar cambios es **Editar → Cargar**.

---

### 12. Prompt de arranque (pégalo al abrir un chat nuevo)

```
Eres un ingeniero de ingeniería inversa que continúa el proyecto Phoenix Evolution de FRALEX (PES 2021, parche ConmeGOL 26, rama mercado-fase0).
Antes de hacer nada: lee PhoenixSync/base-conocimiento/20-TRASPASO-LIGA-MASTER-v2.md completo y luego los archivos en el orden de su sección 3.
Habla en español, simple, ordenado, paso a paso; define cada palabra nueva; repite lo importante; avisa seguido con mensajes cortos. Meta: 0 errores; si hay un error, no avances.
Reglas: trabaja siempre sobre COPIAS; ida y vuelta antes de editar; cierra en archivo NUEVO; anota y comprueba huellas; un cambio es «probado» solo cuando FRALEX lo ve en el juego.
Registra siempre al FINAL de PRUEBAS.md y REGISTRO.md, sin borrar ni sobrescribir nada; mantén GitHub actualizado (commit y push en mercado-fase0; nada a master); tras cada avance importante, escribe una guía exacta nueva y súbela al Drive como documento nuevo; fuera de eso no subas nada al Drive sin que FRALEX lo pida.
Haz fetch + pull --rebase antes de trabajar; si hay conflicto en PRUEBAS/REGISTRO, conserva las dos partes. No hagas checkout de otra rama ni toques SmashSoda/. Compila y prueba antes de cada commit de código. No subas guardados ni .bin. Nunca pidas contraseñas. Guías siempre multiparche.
Primero confirma al usuario en 5 líneas qué entendiste y qué vas a hacer, y empieza por la tarea que él te pida. Si no sabes algo, dilo: no lo inventes.
```

---

### 13. Cómo registrar un hallazgo nuevo

1. Etiqueta: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ❌ comprobado que no · ⚠️ riesgo · 🧩 depende del parche.
2. Escribe: qué hiciste, cómo lo comprobaste, con qué herramienta, en qué guardado (con su huella) y el resultado.
3. Añádelo al final de `PRUEBAS.md` y `REGISTRO.md` y, si es un avance importante, a una guía nueva `NN-GUIA-….md` (copia en el Drive).
4. Avisa a FRALEX en lenguaje simple.

---

## Parte 13 · 21 · Guía exacta: el experimento de las dos carreras (stats en la Liga Máster)

> Origen: `PhoenixSync/base-conocimiento/21-GUIA-EXPERIMENTO-DOS-CARRERAS.md` · 5328 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 00:40 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de la guía 18.
Diario: `PRUEBAS.md` (9 oct 23:58 → 10 oct 00:40). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · ❌ comprobado que no · ⏳ falta · 🧩 depende del parche.

---

### 0. En simple

Imagina que cada carrera es un **álbum de fotos** de los jugadores:

- Cuando **creas** la carrera, el juego **saca una foto** de cada jugador tal como está en la base en ese momento.
- Desde ahí, la carrera usa **su foto**, no la base. Aunque cambies la base después, la foto no cambia.
- Con el tiempo, la carrera **retoca su foto**: el jugador crece o baja.

**Lo que ya sabemos hacer:** si pones las stats en Phoenix-DB **antes de crear** una carrera, la carrera nace con ellas. ✅
**Lo que falta:** encontrar **dónde** guarda la carrera su «foto», para poder cambiarla en carreras ya empezadas. ⏳

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu juego y tus capturas de Lamine | La verdad en pantalla |
| Phoenix-DB en tu PC: `Player.bin` v99 (Lamine Velocidad 99) y v90 (original) | Las dos versiones de la base |
| Respaldo: `_PhoenixMercado_prueba\db\Player_v99.bin` y `Player_v90.bin` (huellas `2d1a854f…` y `de702dff…`) | Cambiar de versión con seguridad |
| `dec` (abrir guardados, con ida y vuelta) | Abrir las dos carreras nuevas |
| Python 3 + NumPy | Comparar las dos carreras byte a byte y bit a bit |

---

### 2. Paso a paso

#### Paso 1 · Carrera nueva con la base v99
- **Tú:** creaste una carrera con el Barça y la guardaste en la **ranura 4** (`ML00000003`, huella `6cbe94ff…`).
- **Pantalla:** Lamine **Velocidad 99**, media 90. ✅

#### Paso 2 · Cambiar la base a v90 (yo, con el juego cerrado)
- Copié la v90 original a las **dos** carpetas de Phoenix-DB (primero como `.tmp` y luego renombrando) y comprobé la huella: `de702dff…` en las dos. ✅
- La v99 quedó respaldada, idéntica, en `_PhoenixMercado_prueba\db\Player_v99.bin`.

#### Paso 3 · La misma carrera nueva con la base v90
- **Tú:** creaste la misma carrera y la guardaste en la **ranura 5** (`ML00000004`, huella `006dd17a…`).
- **Pantalla:** Lamine **Velocidad 90**, media 87. ✅

#### Paso 4 · La prueba que lo decide
- **Tú:** con la base todavía en **v90**, cargaste la **ranura 4**.
- **Pantalla:** Lamine **Velocidad 99**. ✅
- **Conclusión firme:** la carrera **guarda su propia copia** («su foto»), tomada **al crearse**.

| Carrera | Base cuando se creó | Base al mirarla | Velocidad |
|---|---|---|---|
| Ranura 4 | v99 | v99 | 99 |
| Ranura 5 | v90 | v90 | 90 |
| Ranura 4 | v99 | **v90** | **99** |
| Barça vieja (ranura 1) | anterior | v99 | **90** |

#### Paso 5 · Buscar la «foto» comparando las ranuras 4 y 5
Se abrieron las dos (ida y vuelta idéntica) y se compararon. **Nada de esto encontró la velocidad:**

| Qué se probó | Resultado |
|---|---|
| Bytes con 99 o 59 en una y 90 o 50 en la otra | ❌ solo ruido |
| Como número decimal o de 2 bytes | ❌ |
| Empaquetado en trozos de 6, 7 y 8 bits, antes y después del blob | ❌ solo ruido |
| Grupos con sus 17 habilidades en cualquier orden | ❌ solo ruido |
| Trozos de su ficha original de `Player.bin` | ❌ no están |
| Su ficha gigante «M» | idéntica en las dos ❌ |
| Su carnet del blob | solo cambia el **sueldo** (es mejor jugador, cobra más) |

- **Lo que sí cambia entre las dos:** su **curva de medias** (84, 85, 86… frente a 82, 83, 84…) y unos números **al azar** que el juego sortea al crear la carrera.
- **Zona descartada:** desde `0x10c0000` hay una **imagen** (bloques de textura), no datos de jugadores.
- **Por qué es difícil:** al crear una carrera el juego sortea muchas cosas (juveniles, sorteos, semillas). Entre dos carreras nuevas cambian **1,77 millones** de bytes, y la velocidad de un solo jugador se pierde entre tanto ruido.

---

### 3. Qué significa para tu proyecto, hoy

1. **Para una liga o temporada nueva:** pon las stats en Phoenix-DB **antes de que cada jugador cree su carrera**. Así todas nacen con los mismos datos. ✅ Ya funciona.
2. **Para carreras ya empezadas:** todavía no se puede. ⏳
3. **Ojo:** si cambias la base **después**, las carreras viejas **no** se enteran. Las nuevas sí.

---

### 4. Multiparche 🧩
- Que la carrera «saque una foto» al crearse lo hace **el juego**: pasa igual en todos los parches.
- La base de la que saca la foto es la que el juego usa de verdad en cada parche (guía 12, §2.2).

---

### 5. Próximo plan para encontrar la «foto»

Necesitamos una comparación con **mucho menos ruido**. Dos ideas:

1. **Misma carrera, una semana después.** Guarda la ranura 4, avanza unos días **sin jugar partidos** y guarda en otra ranura. Entre esas dos habrá pocos cambios. Si a algún jugador le cambia una habilidad (crecimiento), esa diferencia señala dónde está la «foto».
2. **Un jugador que crece mucho.** Con un jovencito de tu equipo (por ejemplo, un juvenil), juega un partido y mira si sube alguna habilidad. Así sabemos qué número buscar.

---

## Parte 14 · 23 · Guía exacta: las «barras de crecimiento» de la Liga Máster

> Origen: `PhoenixSync/base-conocimiento/23-GUIA-BARRAS-DE-CRECIMIENTO.md` · 4138 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 00:55 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de las guías 18 y 21.
Diario: `PRUEBAS.md` (10 oct, 00:50 y 00:55). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

### 0. En simple

Cada jugador de **tu equipo** tiene, dentro de la carrera, unas **30 barras de progreso**, como las de un videojuego de rol:

- Cada barra va de **0 a 9.999**.
- Con los días y los partidos, las barras **se van llenando**.
- Cuando una barra **llega a 10.000**, vuelve a empezar y **esa habilidad sube +1**.

**Probado en el juego:** entre el 1/1 y el 16/3 de tu carrera, **solo una** barra de Lamine se llenó, y en pantalla **solo una** habilidad subió: **Conservación del balón, de 92 a 93**. ✅

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| La misma carrera en dos fechas: ranura 4 (1/1/2026, huella `6cbe94ff…`) y ranura 6 (16/3/2026, huella `1feef3a4…`) | Comparar con **poco ruido** |
| Tus capturas de Lamine en las ranuras 4 y 6 | La verdad en pantalla |
| `dec` (abrir, con ida y vuelta) y Python 3 | Comparar byte a byte |

---

### 2. Paso a paso

#### Paso 1 · Comparar la misma carrera en dos fechas
- **Por qué:** entre dos carreras **nuevas** cambian 1,77 millones de bytes (todo lo que el juego sortea). Entre la **misma** carrera en dos fechas, solo **191.549**: unas 10 veces menos ruido.

#### Paso 2 · Mirar las fichas de Lamine
- En su ficha de **368 bytes** (la de tu equipo que llamamos C/D) hay unos **30 números de 0 a 9.999** que, entre el 1/1 y el 16/3, **subieron casi todos**.
- **Uno de ellos** (el n.º 6) pasó de **9.765 a 551**: subió, llegó a 10.000 y volvió a empezar.

#### Paso 3 · La predicción (antes de ver la pantalla)
- «Si son barras de progreso, en la ranura 6 Lamine tendrá **exactamente una habilidad con +1**».

#### Paso 4 · Tu captura de la ranura 6
- **Conservación del balón: 92 → 93.** Todo lo demás igual. ✅
- **Conclusión:** son barras de crecimiento, y la **barra n.º 6 = Conservación del balón**.

#### Paso 5 · Otro hallazgo de la comparación: fichas de partido
- Las fichas de **596 bytes** (5.759) no son de jugadores: son de **partidos**. Cada una lleva el número del partido, la competición, la fecha y la lista de jugadores que jugaron. 5.759 = los partidos de la temporada. 🔎 (Sirve para los resultados partido a partido.)

#### Paso 6 · Lo que todavía no aparece
- El **número de la habilidad** en sí (el 92 o el 93, el 99 de velocidad) **no está guardado como número normal**: se buscó tal cual, empaquetado en 6, 7 y 8 bits y cruzando las dos pruebas. ⏳
- Tampoco está en su carnet del blob, ni en sus fichas L y F (esas guardan datos de estado).

---

### 3. Qué abre este avance

- **Ya se puede LEER** cómo va creciendo cada jugador de tu equipo (cuánto le falta a cada barra).
- **Posible palanca para CAMBIAR stats en carreras empezadas (por probar, ⏳):** dejar una barra en 9.999 para que la habilidad suba **+1** en el siguiente avance del juego. Es lento (+1 cada vez), pero usa el **propio** sistema del juego.
- **Falta:** saber qué habilidad es cada una de las otras ~29 barras. Se aprende igual: comparar dos fechas y mirar qué habilidad subió.

---

### 4. Multiparche 🧩
- Las barras de crecimiento son del **juego**: iguales en todos los parches.
- Qué jugadores crecen y cuánto depende de la base de cada parche (edad, potencial).

---

### 5. Próximos pasos

1. **Mapear las barras:** cada vez que juegues o avances días en una carrera, guarda **antes y después** en ranuras distintas y saca captura del jugador. Cada habilidad que suba me dice qué barra es.
2. **Probar la palanca (con tu permiso y en una ranura de prueba):** poner la barra de una habilidad en 9.999, avanzar un día y mirar si sube +1.
3. Seguir buscando dónde está el número de la habilidad.

---

## Parte 15 · 24 · Guía exacta: la «palanca» para subir habilidades en una Liga Máster ya empezada

> Origen: `PhoenixSync/base-conocimiento/24-GUIA-PALANCA-SUBIR-HABILIDADES.md` · 5283 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 01:15 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de la guía 23 (barras de crecimiento).
Diario: `PRUEBAS.md` (10 oct, 01:05 y 01:11). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

### 0. En simple

Cada habilidad de un jugador de tu equipo tiene una **barra de progreso** de 0 a 9.999.
Cuando la barra se llena, el juego sube esa habilidad **+1**.

**La palanca:** nosotros llenamos la barra **a mano**, casi hasta el tope (9.999).
Luego el juego, al avanzar, la termina de llenar y **él mismo** sube la habilidad.

**Probado en el juego:** Lamine, Conservación del balón **93 → 94**. ✅

Es como llenar casi entera la barra de experiencia de un personaje de videojuego: con un poquito más, **sube de nivel solo**.

---

### 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu ranura 6 (Barça, 16/3/2026, huella `1feef3a4…`) | La carrera de prueba |
| Respaldo en `_PhoenixMercado_prueba\respaldos_ranuras\ML00000005_ranura6_original_16-3-2026` | Poder volver atrás |
| `herramientas/palanca/palanca.cpp` (nueva) | Abrir el guardado, cambiar **un** número y volver a cerrarlo |
| `SobrePes` (Phoenix Sync) | Descifrar y cifrar el guardado con su envoltura original |
| Tus 2 capturas de Lamine (al cargar y varios días después) | La verdad en pantalla |

---

### 2. Paso a paso

#### Paso 1 · Respaldo (antes de tocar nada)
- Se copió tu ranura 6 original a `respaldos_ranuras` y se comprobó que la copia es **idéntica** (misma huella).

#### Paso 2 · Cambiar un solo número
- En la ficha de 368 bytes de Lamine, la **barra n.º 6** (Conservación del balón) estaba en **551**.
- La herramienta **comprueba primero** que el valor es 551 (si no, no toca nada) y lo cambia a **9.999**.
- Texto en «Cargar»: «PRUEBA PALANCA (Barca) / 16/3/2026 / Lamine barra 6 = 9999».

#### Paso 3 · Comprobar el archivo nuevo
- Se volvió a abrir y se comparó con el original: **solo 2 bytes distintos**, los de esa barra. ✅
- Se llevó a tu PC y se comprobó la huella (`f220841e…`) **antes y después** de copiarlo a la ranura 6. ✅

#### Paso 4 · Tu prueba en el juego
| Momento | Conservación del balón |
|---|---|
| Al cargar | **93** (aún no cambia: falta que el juego avance) ✅ |
| Varios días después | **94** ✅ |

- **Conclusión:** la palanca **funciona**. El juego aceptó el cambio sin quejarse, y la carrera sigue normal (media 90, todo lo demás coherente).

#### Paso 5 · Regalo extra: otras barras que estaban casi llenas
En esos días también subieron, por crecimiento normal:

| Habilidad | Antes → después | Barra probable (estaba casi llena) |
|---|---|---|
| Actitud ofensiva | 83 → 84 | **n.º 1** (9.969) 🔎 casi seguro |
| Salto | 71 → 72 | n.º 22 (9.940) o n.º 25 (9.690) 🔎 |
| Equilibrio | 87 → 88 | n.º 22 o n.º 25 🔎 |

Para confirmarlo hace falta el guardado **de después**, en una ranura nueva. ⏳

---

### 3. Qué abre este avance

- **Ya se puede subir +1** cualquier habilidad cuya barra conozcamos, en una carrera **ya empezada**, usando el sistema del propio juego. ✅
- **Es de +1 por vez:** la barra vuelve a empezar desde 0 al llenarse. Para subir +3 habría que repetir la palanca 3 veces, avanzando entre medias. ⏳ Sin probar si se puede más rápido.
- **Falta:** saber qué habilidad es cada una de las ~30 barras (hoy: n.º 6 seguro, n.º 1 casi seguro).
- **No sirve (todavía) para BAJAR** una habilidad ni para ponerla en un número exacto de golpe.

---

### 4. Riesgos y cuidados

- **Siempre respaldo antes**, y comprobar la huella del archivo en el PC.
- Cambiar **solo** la barra (un número de 0 a 9.999). Nunca poner 10.000 o más: no sabemos qué haría el juego.
- La herramienta **se niega** a cambiar si el valor viejo no es el esperado: así no se toca nada por error.
- Las barras solo existen para los jugadores de **tu equipo** (ficha de 368 bytes). Para los demás equipos, ⏳.

---

### 5. Multiparche 🧩

- Las barras y la forma de subir +1 son **del juego**: valen igual en ConmeGOL, Sudamerican y cualquier parche de PES 2021.
- Lo que cambia por parche es **dónde** cae la ficha de cada jugador dentro del guardado. Por eso la herramienta **no** usa posiciones fijas a ciegas: comprueba el valor antes de tocar. En otro parche, primero hay que buscar la ficha del jugador (como en la guía 23) y luego aplicar la palanca.
- Probado solo en **ConmeGOL 26**, carrera del Barça.

---

### 6. Próximos pasos

1. **Guardar el «después»** en una ranura nueva → confirmo qué barras son Actitud ofensiva, Salto y Equilibrio.
2. **Mapear todas las barras de una vez:** a cada jugador de tu equipo le lleno **una barra distinta** (al jugador A la n.º 2, al B la n.º 3, al C la n.º 4…). Avanzas, y en cada jugador miras qué habilidad subió: así cada jugador nos «dice» una barra, sin confundirlas.
3. Probar la palanca en **dos barras a la vez** y **dos veces seguidas** (+2).

---

## Parte 16 · 25 · Guía exacta: el mapa de las barras (qué barra sube qué habilidad)

> Origen: `PhoenixSync/base-conocimiento/25-GUIA-MAPA-DE-BARRAS.md` · 5188 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 01:55 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de las guías 23 (barras) y 24 (palanca).
Diario: `PRUEBAS.md` (10 oct, 01:35 → 01:52). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

### 0. En simple

Cada jugador de tu equipo tiene unas 30 **barras de progreso**. Ya sabíamos que, si llenamos una barra a mano, el juego sube **+1** esa habilidad (guía 24).

Faltaba saber **qué barra es de qué habilidad**. Ahora ya lo sabemos para **las 20 habilidades de jugador de campo**. ✅

---

### 1. El mapa

| Barra | Habilidad | | Barra | Habilidad |
|---|---|---|---|---|
| 1 | Actitud ofensiva | | 13 | Balón parado |
| 2 | Actitud defensiva | | 14 | Efecto |
| 4 | Regate | | 20 | Velocidad |
| 5 | Control de balón | | 21 | Contacto físico |
| 6 | Conservación del balón | | 22 | Equilibrio |
| 7 | Finalización | | 23 | Potencia de tiro |
| 8 | Pase raso | | 24 | Aceleración |
| 9 | Pase bombeado | | 25 | Salto |
| 10 | Cabeceo | | 26 | Resistencia |
| 11 | Recuperación de balón | | | |
| 12 | Agresividad | | | |

**Sin saber todavía:**
- Barras 3, 15, 16, 17, 18: solo se usan en **porteros** (seguramente las 5 de portero). ⏳
- Barras 19, 27, 28: crecen muy poquito (decenas al mes). ¿Pie malo, regularidad, lesiones? ⏳
- Barras 29 y 30: siempre en 0.

---

### 2. Recursos usados

| Recurso | Para qué |
|---|---|
| Ranura 7 (11/4/2026) | Punto de partida |
| Ranura 8 = copia de la 7 con 60 barras en 9.999 (`herramientas/palanca/palanca_varias.cpp`) | La prueba |
| Ranura 9 (22/4/2026), guardada por ti después de avanzar | Ver en el archivo qué barras se llenaron |
| 28 capturas tuyas (7 jugadores × páginas 2 y 3 × antes y después) | La verdad en pantalla |
| Respaldos en `_PhoenixMercado_prueba\respaldos_ranuras` | Poder volver atrás |
| Python | Comparar barras y decodificar |

---

### 3. Paso a paso

#### Paso 1 · El truco del «código secreto»
- En vez de probar una barra cada vez (20 pruebas), se hizo **una sola prueba**.
- 7 jugadores jóvenes: Bardghji, Caicedo, Gavi, Fermín, Lamine, Bernal, Cubarsí.
- Cada barra se llenó en **3 de esos 7**, con un trío **distinto** para cada barra.
- Así, el trío de jugadores en los que sube una habilidad es como su **huella digital**: dice qué barra es.

#### Paso 2 · Preparar la ranura 8
- 60 barras cambiadas a 9.999. La herramienta comprueba cada valor viejo antes de tocarlo (si uno no cuadra, no escribe nada).
- Comprobado: **120 bytes** distintos (60 números), todos donde se planeó. Respaldo de la ranura 7 antes.

#### Paso 3 · Tu prueba
- Cargaste la ranura 8, sacaste capturas (antes), avanzaste ~11 días, sacaste capturas (después) y guardaste en la ranura 9.

#### Paso 4 · Comprobar en el archivo
- En la ranura 9, **las 60 barras se llenaron** (dieron la vuelta). ✅
- Además se llenaron 4 solas, por crecimiento normal: Lamine (barras 4, 8, 24) y Bernal (barra 25).

#### Paso 5 · Comprobar en la pantalla
- A cada jugador le subieron **exactamente** tantas habilidades como barras llenas tenía: **64 de 64**. ✅
- Con el trío de cada habilidad se sacó su barra. Dos parejas quedaron empatadas (8/12 y 13/24) y se desempataron con las capturas de Bernal: a él le subieron **Pase raso** y **Aceleración**, pero **no** Agresividad ni Balón parado.

---

### 4. Qué abre este avance

- Ya se puede **subir +1 cualquier habilidad de campo** de un jugador de tu equipo, eligiendo la barra correcta. ✅
- Phoenix Sync podrá hacerlo **desde la web**: «+1 Velocidad a X» → barra 20 en 9.999.
- Sigue siendo de **+1 por vez**, y solo para jugadores **de tu equipo**.

---

### 5. Riesgos y cuidados

- Respaldo **siempre** antes, y comprobar la huella del archivo en el PC.
- Nunca poner más de 9.999 (la herramienta no lo permite).
- Si la habilidad ya está en **99**, la barra no se mueve (lo vimos con la Velocidad de Lamine): no sirve llenarla.
- Al llenar muchas barras a la vez, el jugador sube varias habilidades de golpe y su **media** puede subir (Fermín 86→88, Bernal 78→80). En una liga real hay que decidir cuántas subidas se permiten.

---

### 6. Multiparche 🧩

- El mapa (qué barra es qué habilidad) es **del juego**, no del parche: vale igual en ConmeGOL, Sudamerican y cualquier parche de PES 2021.
- Lo que cambia por parche y por carrera es **dónde** está la ficha de cada jugador en el guardado. La herramienta no adivina: comprueba el valor viejo antes de tocar.
- Probado en **ConmeGOL 26**, carrera del Barça.

---

### 7. Próximos pasos

1. **Porteros:** misma prueba con los porteros (barras 3, 15, 16, 17, 18).
2. **Barras lentas** (19, 27, 28): llenarlas en 9.999 y mirar la página 3 (pie malo, regularidad, lesiones).
3. Probar si una barra que **baja de 0** resta −1 (veteranos).
4. Meterlo en Phoenix Sync como función «subir habilidad».

---

## Parte 17 · 26 · Informe y guía completa: subir habilidades en una Liga Máster ya empezada (barras de crecimiento)

> Origen: `PhoenixSync/base-conocimiento/26-INFORME-Y-GUIA-COMPLETA-BARRAS-DE-CRECIMIENTO.md` · 41919 bytes. Copia íntegra.

**Para FRALEX, y para cualquier persona o IA que quiera repetirlo.**
**Fecha:** 10 de octubre de 2026, 02:05 (Lima).
**Juego:** PES 2021 (eFootball PES 2021 Season Update), PC.
**Parche de la prueba:** ConmeGOL 26. Las diferencias con otros parches están en la sección 9.
**Proyecto:** Phoenix Evolution Series → Phoenix Sync (repo `Fralexito/smash-soda-fork`, rama `mercado-fase0`, carpeta `PhoenixSync/`).

Este documento junta en uno solo lo que cuentan las guías 21, 23, 24 y 25, y añade todo lo que hace falta para **repetirlo desde cero**:
- definiciones;
- posiciones exactas en el archivo;
- código;
- comprobaciones;
- datos crudos.

**Cómo leerlo:**
- Si eres **humano**, lee las secciones **0 a 3** (qué es, qué se logró y cómo se hizo, en simple). El resto es el detalle.
- Si eres una **IA** que quiere replicarlo, lee **todo**, sobre todo las secciones **4, 5, 6 y 7** y el **Anexo**.

Símbolos:
- ✅ comprobado en el juego (con pantalla);
- 🔎 visto en archivos, sin mirar la pantalla;
- ⏳ falta;
- 🧩 puede cambiar según el parche.

---

### Índice

0. Resumen en una página
1. Glosario: cada palabra explicada
2. La historia completa: los 4 experimentos
3. Resultado final: el mapa de barras y las reglas
4. Especificación técnica: dónde y cómo están las barras en el archivo
5. Guía de réplica, paso a paso (para humanos)
6. Guía de réplica, paso a paso (para una IA o un programa)
7. El método del «código secreto» para mapear muchas barras en una sola prueba
8. Lista de comprobación de 0 errores
9. Multiparche: qué cambia en otros parches
10. Riesgos, límites y cosas que NO hacer
11. Lo que falta descubrir y cómo hacerlo
12. Archivos, herramientas y huellas
13. Anexo: datos crudos de las pruebas

---

### 0. Resumen en una página

**El problema.**
Cuando cambiamos las habilidades de un jugador en la base del parche (Phoenix-DB, `Player.bin`), el cambio se ve en amistosos. Pero **no** se ve en una Liga Máster ya empezada.

**Por qué.**
Al **crear** una carrera, el juego hace una copia («foto») de las habilidades de todos los jugadores, y desde ahí usa **su** copia. ✅ (guía 21)

**Lo que encontramos.**
Dentro del guardado de la carrera, cada jugador del **equipo del usuario** tiene unas **30 barras de progreso**, una por habilidad.
- Cada barra es un número de **0 a 9.999**.
- Con los días y los partidos, el juego **llena** las barras de los jóvenes y **vacía** las de los veteranos.
- Cuando una barra pasa de **9.999**, vuelve a empezar desde 0 y **esa habilidad sube +1**. ✅

**La palanca.**
Nosotros ponemos a mano una barra en **9.999**. Cuando el juego avanza, la llena y él mismo sube la habilidad **+1**.
Se probó con Lamine Yamal: Conservación del balón **93 → 94**. ✅

**El mapa.**
Con una prueba de 7 jugadores y 60 barras, se averiguó qué barra corresponde a cada una de las **20 habilidades de jugador de campo**.
- Resultado: **64 subidas de 64** cuadraron entre archivo y pantalla. ✅
- Las 60 barras puestas dieron la vuelta, y además hubo 4 subidas naturales.

**Qué se puede hacer hoy.**
Subir **+1** cualquier habilidad de campo de cualquier jugador **del equipo del usuario**, en una carrera ya empezada, usando el propio sistema del juego.

**Qué falta.**
- Las 5 habilidades de portero.
- Tres barras lentas, que podrían ser pie malo, regularidad o resistencia a lesiones.
- Bajar habilidades.
- Jugadores de otros equipos.
- El valor absoluto de cada habilidad (dónde guarda el juego el «93»).

---

### 1. Glosario: cada palabra explicada

| Palabra | Qué significa, en simple |
|---|---|
| **Liga Máster (LM)** | El modo carrera de PES 2021: eres entrenador de un club durante varias temporadas. |
| **Carrera** | Una partida concreta de Liga Máster. |
| **Ranura** | Cada «casilla» de guardado del menú Cargar. La ranura **n** es el archivo `ML0000000(n−1)` en la carpeta `save`. Ejemplo: la ranura 6 es `ML00000005`. |
| **Carpeta save** | En el PC de FRALEX: `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`. Normalmente: `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<id>\save`. |
| **Guardado cifrado** | El archivo `ML…` tal como está en el disco. Está protegido (cifrado y comprimido): no se lee a simple vista. |
| **Sobre (envoltura)** | La capa de protección del guardado. Phoenix Sync la quita y la vuelve a poner con `SobrePes` (basado en libpesXcrypter). Guía 10. |
| **Datos descifrados** | El guardado ya sin el sobre: unos 19,8 MB de datos que se pueden leer byte a byte. |
| **Byte** | La unidad mínima de un archivo: un número de 0 a 255. |
| **u16 / u32** | Un número guardado en 2 bytes (de 0 a 65.535) o en 4 bytes. Se guardan «al revés» (*little-endian*): el byte de menos peso va primero. Ejemplo: 9.999 = `0x270F` se escribe `0F 27`. |
| **Hex (0x…)** | Forma de escribir números en base 16, la habitual para posiciones en archivos. `0xc51228` = 12.915.240. |
| **Posición (offset)** | Dónde está un dato dentro de los datos descifrados, contando desde 0. |
| **pid** | Número de identidad de un jugador en la base del juego. Lamine Yamal = **162114**. |
| **reg** | Número interno del jugador **dentro de la carrera**. Lamine en esta carrera = `0xdb1`. |
| **Plantilla** | Lista de jugadores de un equipo. |
| **Equipo del usuario** | El club que manejas tú en la carrera (aquí, el FC Barcelona). |
| **Tablas alineadas** | Listas que el juego guarda **solo para el equipo del usuario**, con un registro por jugador, en el mismo orden que su plantilla. |
| **Tabla D** | Una de esas tablas. Cada registro mide **368 bytes**. Ahí viven las barras. |
| **Barra de crecimiento** | Cada uno de los 30 números u16 (0–9.999) de un registro de la tabla D. Es el «progreso» hacia el siguiente +1 de una habilidad. |
| **Dar la vuelta** | Cuando una barra pasa de 9.999, vuelve a empezar desde un número pequeño: la habilidad sube +1. |
| **Palanca** | Nuestro truco: poner una barra en 9.999 a mano para que el juego suba la habilidad en el siguiente avance. |
| **Huella (md5 / sha256)** | Un «DNI» del archivo, calculado a partir de su contenido. Si cambia un solo byte, la huella cambia. Sirve para comprobar que un archivo llegó idéntico. |
| **Respaldo** | Copia de seguridad, siempre con un nombre nuevo, para poder volver atrás. |
| **Phoenix-DB** | La carpeta de la base del parche que usa Sider (`Player.bin`, etc.). |
| **Multiparche** | Que funcione en cualquier parche de PES 2021, no solo en ConmeGOL. |

---

### 2. La historia completa: los 4 experimentos

#### Experimento 1 · ¿Llegan las stats de Phoenix-DB a una Liga Máster? (9 oct, guías 18 y 21)

1. Pusimos a Lamine con Velocidad **99** en `Player.bin` (Phoenix-DB). En la base original tiene 90.
2. En un amistoso salió con **99**. ✅ En la carrera del Barça ya empezada salió con **90**. ❌
3. Creamos **dos carreras nuevas** iguales:
   - ranura 4, con la base 99 → Lamine sale con 99;
   - ranura 5, con la base 90 → Lamine sale con 90.
4. Prueba decisiva: con la base en **90**, cargamos la ranura 4 → Lamine sigue con **99**. ✅

**Conclusión:** la carrera **copia** las habilidades al crearse y desde ahí usa su copia. ✅

Buscamos el número «99» de esa copia dentro del guardado de muchas formas (literal, empaquetado en 6, 7 u 8 bits, en orden y sin orden). **No apareció**.
El valor absoluto de las habilidades sigue sin localizar. ⏳

#### Experimento 2 · Descubrir las barras (10 oct, 00:50; guía 23)

1. Se compararon **la misma carrera en dos fechas**:
   - ranura 4, del 1/1/2026;
   - ranura 6, del 16/3/2026.

   Así hay mucho menos «ruido» que entre dos carreras distintas: 191.549 bytes distintos, frente a 1,77 millones.
2. En el registro de 368 B de Lamine había unos **30 números entre 0 y 9.999** que habían **subido casi todos**.
3. Uno, el n.º 6, pasó de **9.765 a 551**: había llegado al tope y había empezado de nuevo.
4. **Predicción**, hecha antes de mirar la pantalla: «en la ranura 6, Lamine tendrá exactamente una habilidad con +1».
5. **Pantalla:** Conservación del balón **92 → 93**, y todo lo demás igual. ✅

→ Son barras de crecimiento, y la **barra 6 = Conservación del balón**.

#### Experimento 3 · La palanca (10 oct, 01:05–01:11; guía 24)

1. Respaldo de la ranura 6 (`respaldos_ranuras\ML00000005_ranura6_original_16-3-2026`, md5 `1feef3a4…`).
2. En una copia, la barra 6 de Lamine pasó de **551 a 9.999**. Solo cambiaron 2 bytes. Esa copia quedó en la ranura 6 (md5 `f220841e…`).
3. FRALEX la cargó: Conservación **93**. Es lo esperado, porque aún no ha avanzado nada.
4. Avanzó varios días: Conservación **94**. ✅
5. En la ranura 7 (11/4/2026), guardada después, el archivo mostró que a Lamine le dieron la vuelta las barras **1, 6, 22 y 25**. En pantalla le subieron **4** habilidades: Actitud ofensiva, Conservación, Equilibrio y Salto. ✅ Cuadra.

#### Experimento 4 · El mapa completo (10 oct, 01:20–01:50; guía 25)

1. Desde la ranura 7 se hizo la **ranura 8**, con **60 barras en 9.999** repartidas entre 7 jugadores jóvenes. Es el «código secreto» de la sección 7.
2. FRALEX:
   - sacó capturas **antes** (páginas 2/4 y 3/4 de Habilidad);
   - avanzó unos 11 días, hasta el 22/4/2026;
   - sacó capturas **después**;
   - guardó en la **ranura 9**.
3. **Archivo (ranura 9):** las 60 barras dieron la vuelta, **60 de 60**. Además, 4 dieron la vuelta solas por crecimiento normal: Lamine las barras 4, 8 y 24, y Bernal la 25.
4. **Pantalla:** a cada jugador le subieron **exactamente** las habilidades que predicen sus barras: **64 de 64**. Esto se comprobó también con un programa (Anexo A.4).
5. Con el trío de jugadores de cada habilidad se dedujo su barra. Dos parejas empatadas se separaron con Marc Bernal.

---

### 3. Resultado final: el mapa de barras y las reglas

#### 3.1 · El mapa ✅

| Barra | Habilidad (página 2/4 o 3/4 de «Habilidad») |
|---|---|
| 1 | Actitud ofensiva |
| 2 | Actitud defensiva |
| 3 | *(solo porteros)* ⏳ |
| 4 | Regate |
| 5 | Control de balón |
| 6 | Conservación del balón |
| 7 | Finalización |
| 8 | Pase raso |
| 9 | Pase bombeado |
| 10 | Cabeceo |
| 11 | Recuperación de balón |
| 12 | Agresividad |
| 13 | Balón parado |
| 14 | Efecto |
| 15, 16, 17, 18 | *(solo porteros)* ⏳ |
| 19 | *(crece muy lento)* ⏳ |
| 20 | Velocidad |
| 21 | Contacto físico |
| 22 | Equilibrio |
| 23 | Potencia de tiro |
| 24 | Aceleración |
| 25 | Salto |
| 26 | Resistencia |
| 27, 28 | *(crecen muy lento)* ⏳ |
| 29, 30 | siempre 0 |

#### 3.2 · Reglas comprobadas

| Regla | Estado |
|---|---|
| Barra que pasa de 9.999 → esa habilidad **+1** | ✅ 64/64 |
| Poner la barra en 9.999 **no** cambia nada hasta que el juego avance | ✅ |
| El juego acepta el guardado modificado sin quejarse | ✅ 3 veces (ranuras 6, 8 y 9) |
| La barra no se mueve si la habilidad ya está en **99** (Velocidad de Lamine, barra 20 quieta) | 🔎 muy probable |
| Las barras de los veteranos **bajan** con el tiempo | 🔎 visto (Rodri, Raphinha, Cancelo…) |
| Barra que baja de 0 → habilidad **−1** | ⏳ sin comprobar en pantalla |
| Si suben varias habilidades, la **media** puede subir (Fermín 86→88, Bernal 78→80) | ✅ |
| Barras 3 y 15–18 distintas de 0 solo en porteros | 🔎 |
| Crecimiento típico de un joven: 300–1.000 por barra y mes (10–35 por día) | 🔎 |

---

### 4. Especificación técnica: dónde y cómo están las barras en el archivo

#### 4.1 · Cadena completa

```
ML0000000n (disco, cifrado)
  └─ SobrePes::abrir()          → datos descifrados (~19,8 MB)   [guía 10]
       └─ equipo del usuario    → bloque de equipo con la marca −11 (kMarcaUsuario) en kOfsIdOption
            └─ plantilla        → lista de (reg, pid) de sus jugadores
                 └─ Tabla D     → 1 registro de 368 B por jugador, mismo orden que la plantilla
                      └─ barras → 30 × u16 desde el byte +54 del registro
```

#### 4.2 · Formato de un registro de la tabla D (368 bytes)

| Desde el inicio del registro | Tamaño | Qué es |
|---|---|---|
| +0 | 4 B | campo `x` (sin interpretar) |
| +4 | u32 | **reg** del jugador |
| +8 | u32 | **pid** del jugador |
| +12 … +53 | 42 B | otros datos (sin interpretar del todo; ver ESTRUCTURA-ML §25) |
| **+54** | **30 × u16** | **barras 1 a 30** (barra *k* en **+54 + 2·(k−1)**) |
| +114 … +367 | resto | sin interpretar |

**Fórmula:** `posición de la barra k = inicio del registro + 54 + 2·(k − 1)`, o lo que es lo mismo, `= posición del reg + 50 + 2·(k − 1)`.

**Ejemplo real** (ranura 6, datos descifrados):
- registro de Lamine: empieza en `0xc51228`, con reg `0xdb1` en `0xc5122c` y pid 162114;
- barra 1 → `0xc5125e`;
- barra 6 → `0xc5125e + 10` = **`0xc51268`** ✅ (es la que se cambió en la palanca).

En esta carrera la tabla D empieza en `0xc50818` (registro índice 0) y Lamine es el índice 7. Los registros van **seguidos**, cada 368 bytes.

#### 4.3 · Cómo encontrar la tabla D en CUALQUIER guardado (sin posiciones fijas)

Las posiciones de 4.2 son **de esta carrera**: no hay que copiarlas a ciegas. El método general ya está programado en `GuardadoLM::tablasDe(k)` (`PhoenixSync/core/LigaMaster.cpp`):

1. Busca el equipo del usuario: el bloque de equipo que lleva la marca **−11** (`kMarcaUsuario`).
2. Lee su plantilla: la lista de pares `(reg, pid)`.
3. Después de los bloques de equipo (`kFinBloques`), busca el par `(reg, pid)` del **primer** jugador (8 bytes seguidos: reg y luego pid).
4. En cada sitio donde aparezca, comprueba que los **4 jugadores siguientes** de la plantilla están a distancias de **368 bytes** (admite huecos vacíos: `pid == 0` y `reg` en 0, 0xffff o vacío).
5. Si cuadra, es la tabla D. Así obtienes `ofsReg0` = posición del reg del registro 0.

**Comprobado (10 oct, 02:05):** `tablasDe` sobre la ranura 9 encuentra el equipo del usuario (bloque **k = 122**) y su tabla D con `stride = 368` y `ofsReg0 = 0xc5081c`. Es justo 4 bytes después del inicio del registro 0 (`0xc50818`). ✅

El registro del jugador número *i* de la plantilla empieza en `ofsReg0 − 4 + 368·i`. Puede haber huecos: lo seguro es recorrer los registros y quedarse con el que tenga el `reg`/`pid` buscado.

**Cuidado:** el `reg` cambia de prefijo según la carrera. En la carrera del City era `0xdb65xxxx`, en la del Barça `0xdbdfxxxx`, o valores bajos como `0xdb1`. Hay que compararlo **tal cual** con el de la plantilla, sin suponer el prefijo.

#### 4.4 · Comportamiento de una barra

- Valor: u16 de **0 a 9.999**. Nunca escribir ≥ 10.000.
- Al avanzar el tiempo, el juego suma (jóvenes) o resta (veteranos) un poco cada día y con los partidos.
- **Dar la vuelta hacia arriba:** valor viejo alto → valor nuevo bajo. Ejemplos reales: 9.999 → 327, 9.969 → 595, 9.690 → 6. → habilidad **+1**.
- **Detectarlo en un programa:** `nuevo < viejo − 3000` = dio la vuelta hacia arriba. `nuevo > viejo + 3000` = dio la vuelta hacia abajo (se espera −1, ⏳).
- Si la habilidad está en **99**, la barra no avanza.

---

### 5. Guía de réplica, paso a paso (para humanos)

**Objetivo de ejemplo:** subir +1 la Velocidad (barra 20) de un jugador de tu equipo en una carrera empezada.

**Lo que necesitas:**
- el juego cerrado;
- la carrera guardada en una ranura;
- una ranura **libre** para la prueba;
- la herramienta `palanca` o `palanca_varias` (sección 12).

1. **Elige la carrera y el jugador.** Mira en el juego su habilidad actual (página 2/4 o 3/4 de «Habilidad») y apúntala. Si ya está en 99, no se puede subir.
2. **Cierra el juego.**
3. **Haz un respaldo** del guardado (`ML0000000n`). Cópialo con **otro nombre** a una carpeta de respaldos y comprueba que la huella es la misma.
4. **Busca el registro del jugador** en la tabla D y lee el valor actual de su barra 20 (sección 6, pasos 3 y 4).
5. **Cambia solo esa barra a 9.999**, y guarda el resultado en un **archivo nuevo** destinado a una **ranura libre**. Pon un texto que se vea en «Cargar», por ejemplo «PRUEBA +1 VELOCIDAD».
6. **Comprueba el archivo nuevo:** al abrirlo otra vez, solo deben ser distintos **2 bytes** respecto al original.
7. **Cópialo a la carpeta save**, en la ranura libre, y comprueba la huella allí.
8. **Abre el juego** y carga esa ranura. La habilidad sigue igual. Eso es lo esperado.
9. **Avanza** al menos un día. Lo seguro es hasta el siguiente entrenamiento o partido.
10. **Mira al jugador:** la Velocidad tiene que haber subido **+1**.
11. **Guarda en otra ranura nueva**, nunca encima del original. Así queda la prueba del «después».
12. **Anota todo** en PRUEBAS.md (qué, cuándo, huellas, resultado) y una línea en REGISTRO.md.

**Para subir +2 o +3:** repite los pasos 3 a 11 sobre el nuevo guardado. La barra vuelve a empezar después de cada +1.

---

### 6. Guía de réplica, paso a paso (para una IA o un programa)

#### 6.1 · Herramientas que ya existen

| Herramienta | Dónde | Qué hace |
|---|---|---|
| `SobrePes` | `PhoenixSync/core/SobrePes.h/.cpp` | `abrir(ruta)` descifra; `datos()` da los bytes; `guardarComo(ruta, datos)` cifra con la envoltura original (falla si el archivo ya existe y verifica releyendo); `ponerTextoInfo(texto)` cambia el texto de «Cargar». |
| `GuardadoLM` | `PhoenixSync/core/LigaMaster.h/.cpp` | Plantillas, equipo del usuario, `tablasDe(k)` (localiza las tablas alineadas, incluida la D de 368 B). |
| `palanca` | `PhoenixSync/herramientas/palanca/palanca.cpp` | Cambia **un** u16: `palanca <ML> <nuevo> <ofsHex> <viejo> <nuevo> [texto]`. Se niega si el valor actual ≠ viejo. |
| `palanca_varias` | `PhoenixSync/herramientas/palanca/palanca_varias.cpp` | Varios cambios, todo o nada: `palanca_varias <ML> <nuevo> "texto" ofsHex:viejo:nuevo …`. Rechaza valores > 9.999. |

**Compilar** (Linux o MinGW, C++20), habiendo compilado antes la biblioteca de Phoenix Sync (`build-sync`):

```
g++ -std=c++20 -I PhoenixSync palanca_varias.cpp \
    build-sync/libPhoenixSyncCore.a build-sync/libPhoenixSyncTerceros.a -o palanca_varias
```

#### 6.2 · Algoritmo

```
ENTRADA: ruta del ML original, pid del jugador, lista de barras k a llenar, ruta de salida (no existe)

1. s = SobrePes::abrir(ML)                         // falla → parar
2. d = s.datos()                                   // bytes descifrados
3. g = GuardadoLM::desdeDatos(d)
   u = índice del equipo del usuario (marca −11)
   T = la tabla de g.tablasDe(u) con stride == 368
4. Recorrer registros r = 0,1,2…: ofs = T.ofsReg0 + 368·r
      si u32(d, ofs+4) == pid  → reg_ofs = ofs; parar     // u32 en ofs = reg, en ofs+4 = pid
   (no encontrado → parar: el jugador no es del equipo del usuario)
5. Para cada barra k:
      p = reg_ofs + 50 + 2·(k−1)
      viejo = u16(d, p)
      comprobar 0 ≤ viejo ≤ 9999                    // si no, parar: no es una barra
      escribir u16(d, p) = 9999
6. s.ponerTextoInfo("PRUEBA …")                     // visible en «Cargar»
7. s.guardarComo(salida, d)
8. VERIFICAR: abrir(salida) y comparar con el original:
      - mismo tamaño descifrado
      - bytes distintos == 2 × (número de barras cuyo valor viejo ≠ 9999)
      - todos los distintos dentro de las posiciones p
9. Respaldo del original ANTES de copiar al PC; huella (md5/sha256) antes y después de copiar.
```

#### 6.3 · Código de referencia (Python, sobre datos ya descifrados)

```python
import struct

TAM_REG = 368          # tamaño del registro de la tabla D
INI_BARRAS = 54        # las barras empiezan en el byte +54 del registro
MAPA = {1:"Actitud ofensiva", 2:"Actitud defensiva", 4:"Regate", 5:"Control de balón",
        6:"Conservación del balón", 7:"Finalización", 8:"Pase raso", 9:"Pase bombeado",
        10:"Cabeceo", 11:"Recuperación de balón", 12:"Agresividad", 13:"Balón parado",
        14:"Efecto", 20:"Velocidad", 21:"Contacto físico", 22:"Equilibrio",
        23:"Potencia de tiro", 24:"Aceleración", 25:"Salto", 26:"Resistencia"}

def barras(datos, inicio_registro):
    """Devuelve las 30 barras (u16) de un registro de la tabla D."""
    return [struct.unpack_from('<H', datos, inicio_registro + INI_BARRAS + 2*i)[0] for i in range(30)]

def poner_barra(datos: bytearray, inicio_registro, k, valor_viejo, valor_nuevo=9999):
    assert 1 <= k <= 30 and 0 <= valor_nuevo <= 9999
    p = inicio_registro + INI_BARRAS + 2*(k-1)
    actual = struct.unpack_from('<H', datos, p)[0]
    if actual != valor_viejo:
        raise ValueError(f'barra {k}: hay {actual}, se esperaba {valor_viejo}; no se toca nada')
    struct.pack_into('<H', datos, p, valor_nuevo)

def vueltas(antes, despues):
    """Barras que dieron la vuelta entre dos guardados (arriba = +1, abajo = −1 esperado)."""
    arriba = [k+1 for k in range(30) if despues[k] < antes[k] - 3000]
    abajo  = [k+1 for k in range(30) if despues[k] > antes[k] + 3000]
    return arriba, abajo
```

#### 6.4 · Comprobación con el juego (obligatoria)

Un cambio **no** está probado hasta que se ve en pantalla. Hay que dejar anotados:
- valor antes (captura);
- valor después de avanzar (captura);
- guardado del después en ranura nueva;
- lectura de las barras del después, para comprobar que dieron la vuelta las esperadas.

---

### 7. El método del «código secreto» para mapear muchas barras en una sola prueba

**Problema.** Para saber qué habilidad es cada barra, lo obvio es llenar una barra, avanzar y mirar qué sube. Con 20 barras serían 20 pruebas.

**Idea.** Usar **varios jugadores a la vez** y darle a cada barra una «huella» distinta: el conjunto de jugadores en los que se llena.

**Diseño usado:**
- **7 jugadores jóvenes** que crecen rápido: Bardghji, Caicedo, Gavi, Fermín, Lamine, Bernal, Cubarsí.
- **20 barras a mapear:** 1, 2, 4–14 y 20–26.
  - Se dejaron fuera las de portero (3, 15–18), porque en un jugador de campo casi no se ven.
  - También las lentas (19, 27, 28) y las siempre a 0 (29, 30).
- A cada barra se le asignó un **trío distinto** de los 7 jugadores. Hay C(7,3) = 35 tríos posibles, más que suficientes para 20.
- Restricciones:
  - no darle a Lamine la barra 20, porque su Velocidad está en 99 y no podría subir;
  - repartir para que cada jugador tenga 8 o 9 barras.

**Por qué tríos (3 de 7) y no parejas.**
- Si un jugador no sube una habilidad (tope de 99) o sube una de más (crecimiento natural), el conjunto observado tendrá **2 o 4** jugadores, y eso **se nota**: no es un trío válido.
- Con tríos, un error no se confunde con otra barra.

**Decodificación:**
1. Para cada habilidad, anotar en qué jugadores subió (comparando capturas antes y después).
2. Si el conjunto es un trío del código, esa es su barra.
3. Si sale un conjunto con un jugador de más, mirar en el **guardado del después** qué barras dieron la vuelta **solas** (no puestas por nosotros) en ese jugador, y descontarlo.
4. Si quedan empates (dos habilidades con el mismo conjunto observado), desempatar con el jugador que tenga **solo una** de las dos barras.
   - Aquí pasó con Lamine: tuvo de forma natural la barra 8 (además de la 12, que era nuestra) y la 24 (además de la 13).
   - Bernal tenía la 8 y la 24, pero no la 12 ni la 13, así que con él se separaron.

**Cómo elegir los tríos** (lo que hizo el script): probar órdenes al azar de los 35 tríos y quedarse con el reparto más equilibrado que cumpla las restricciones. El plan exacto está en `herramientas/palanca/plan_mapeo_2026-10-10.json`.

---

### 8. Lista de comprobación de 0 errores

Antes de tocar nada:
- [ ] El juego está **cerrado**.
- [ ] `git pull --rebase` (dos cuentas trabajan en el repo).
- [ ] La ranura de destino está **libre**, o el usuario pidió expresamente usar una ocupada (entonces, respaldo).
- [ ] **Respaldo** del original con nombre nuevo, y huella comprobada igual.

Al preparar:
- [ ] El jugador es del **equipo del usuario** y su registro está en la tabla D (reg y pid coinciden).
- [ ] Cada valor viejo está entre 0 y 9.999, y coincide con lo que se esperaba.
- [ ] Ningún valor nuevo pasa de 9.999.
- [ ] El texto de «Cargar» deja claro que es una **prueba**.

Después de preparar:
- [ ] Al abrir otra vez el archivo nuevo: mismo tamaño y **solo** los bytes planeados distintos.
- [ ] Huella del archivo en el PC igual a la de la nube.

En el juego:
- [ ] La ranura carga sin errores.
- [ ] Capturas **antes**, avanzar, capturas **después**, guardar en una **ranura nueva**.
- [ ] El archivo del después: dieron la vuelta las barras esperadas.
- [ ] Pantalla y archivo cuadran 1 a 1.

Al terminar:
- [ ] PRUEBAS.md (al final, sin borrar nada), REGISTRO.md (una línea) y, si cambia la estructura, ESTRUCTURA-ML.md.
- [ ] Commit y push a `mercado-fase0`.
- [ ] Guía nueva (número libre) y copia en Drive como documento **nuevo**.

---

### 9. Multiparche 🧩

| Qué | ¿Cambia con el parche? | Explicación |
|---|---|---|
| Que la carrera copie las habilidades al crearse | No | Es del juego. |
| Que existan las barras y que +1 al pasar de 9.999 | No | Es del juego. |
| El **mapa** (qué barra es qué habilidad) | No (casi seguro) | Es del motor del juego, no de la base. Conviene repetir **una** palanca de control en cada parche nuevo (por ejemplo, barra 6 → Conservación). |
| El registro de 368 B y las barras en +54 | No (casi seguro) | Es el formato del guardado de PES 2021. Comprobar con la búsqueda de 4.3. |
| **Dónde** está la tabla D dentro del guardado | **Sí** | Depende de la carrera, del parche y del tamaño de otras partes. Usar siempre la búsqueda de 4.3. |
| El `reg` de cada jugador (prefijo) | **Sí** | Cambia por carrera. Compararlo tal cual. |
| Los `pid` de los jugadores | **Sí**, según el parche | Cada parche puede numerar distinto (jugadores añadidos, como Josué Caicedo en ConmeGOL). |
| El sobre del guardado | No | Es del juego (libpesXcrypter). |
| Cuánto crece cada jugador | **Sí** | Depende de la edad y el potencial que trae la base del parche. |

**Riesgo en parches que modifican el guardado o el exe:** si un parche cambiara el formato de la Liga Máster (raro), la búsqueda de 4.3 **no encontraría** la tabla y la herramienta **no tocaría nada**. Es el comportamiento seguro.

**Probado:** solo en ConmeGOL 26, carrera del FC Barcelona. Pendiente repetir en Sudamerican y otros (guía 12).

---

### 10. Riesgos, límites y cosas que NO hacer

- **No** escribir valores ≥ 10.000 en una barra: no sabemos qué haría el juego.
- **No** sobrescribir la ranura original sin respaldo comprobado.
- **No** subir guardados (`ML…`, `.bin`) a GitHub.
- **No** tocar nada con el juego abierto.
- **Límite:** +1 por barra y por avance. Para +N hay que repetirlo N veces, avanzando entre medias. ⏳ Sin probar si hay atajo.
- **Límite:** solo jugadores del **equipo del usuario**. Los demás equipos no tienen tabla D. ⏳
- **Límite:** no sirve para bajar habilidades (por ahora). Bajar una barra solo retrasa el siguiente +1.
- **Efecto secundario:** muchas subidas a la vez suben la **media** y pueden cambiar el valor de mercado y el sueldo. En una liga competitiva hay que poner reglas (por ejemplo, máximo de subidas por jornada).
- **Juego limpio:** lo que hace la palanca es adelantar el crecimiento que el propio juego aplica. Conviene que la web registre cada uso, para que sea transparente entre los participantes.

---

### 11. Lo que falta descubrir y cómo hacerlo

| Qué | Cómo |
|---|---|
| **Barras de portero** (3, 15, 16, 17, 18) | Mismo «código secreto», con 3 porteros (Joan García, Szczęsny y otro) y cada barra en una combinación distinta de 2 porteros. Mirar la página 3/4 (Actitud de portero, Atajar, Despejar, Reflejos, Cobertura). |
| **Barras lentas** (19, 27, 28) | Ponerlas en 9.999 en 3 jugadores y mirar la página 3/4: uso de pie malo, precisión de pie malo, regularidad, resistencia a lesiones (escalas de 1 a 8). |
| **Bajar −1** | Poner una barra de un veterano en 0 o 1 y avanzar. Si da la vuelta hacia abajo, ¿la habilidad baja −1? |
| **+N de golpe** | Probar si avanzar varios días seguidos con la barra recargada entre medias es lo único posible, o si hay un contador que limite. |
| **Otros equipos** | Buscar si los jugadores de equipos que no son del usuario tienen barras en otro sitio (guía 21, búsquedas pendientes). |
| **Valor absoluto** | Sigue sin localizarse dónde guarda la carrera el número de cada habilidad. Con el mapa de barras, una búsqueda nueva tiene más pistas (comparar ranuras 8 y 9: cada habilidad que subió +1 tiene que estar en algún sitio). |

---

### 12. Archivos, herramientas y huellas

#### 12.1 · En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

| Ruta | Contenido |
|---|---|
| `PhoenixSync/core/SobrePes.*` | Abrir y cerrar el sobre del guardado |
| `PhoenixSync/core/LigaMaster.*` | Equipo del usuario, plantillas, `tablasDe` |
| `PhoenixSync/herramientas/palanca/palanca.cpp` | Cambiar 1 barra |
| `PhoenixSync/herramientas/palanca/palanca_varias.cpp` | Cambiar varias barras (todo o nada) |
| `PhoenixSync/herramientas/palanca/plan_mapeo_2026-10-10.json` | Plan exacto de la prueba de 60 barras |
| `PhoenixSync/herramientas/palanca/antes_mapeo_2026-10-10.json` | Habilidades «antes» (capturas) |
| `PhoenixSync/herramientas/palanca/antes_despues_mapeo_2026-10-10.json` | Habilidades antes y después de los 7 |
| `PhoenixSync/PRUEBAS.md` | Diario de pruebas (10 oct, 00:50 → 02:05) |
| `PhoenixSync/REGISTRO.md` | Una línea por cambio |
| `PhoenixSync/liga-master/ESTRUCTURA-ML.md` §25 | Formato técnico |
| `PhoenixSync/base-conocimiento/21, 23, 24, 25, 26` | Guías |

#### 12.2 · En el PC de FRALEX

- **Guardados:** `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`.
- **Respaldos:** `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\respaldos_ranuras\`.

#### 12.3 · Huellas (md5) de todos los guardados usados

| Ranura | Archivo | Fecha en el juego | Qué es | md5 |
|---|---|---|---|---|
| 4 | ML00000003 | 1/1/2026 | Carrera nueva con la base v99 | `6cbe94ff…` |
| 5 | ML00000004 | 1/1/2026 | Carrera nueva con la base v90 | `006dd17a…` |
| 6 (original) | respaldo `ML00000005_ranura6_original_16-3-2026` | 16/3/2026 | La de la ranura 4, avanzada | `1feef3a47c37e86e164a409a5c4999b5` |
| 6 (palanca) | ML00000005 | 16/3/2026 | Barra 6 de Lamine = 9.999 | `f220841e2842d2e74e237160d9d9f70b` |
| 7 | ML00000006 | 11/4/2026 | Después de la palanca | `af613ecdaa1474886fb60a8aa891fa94` |
| 8 | ML00000007 | 11/4/2026 | 60 barras en 9.999 (mapeo) | `ac573b6b859b7322c8eeadd28c174905` |
| 9 | ML00000008 | 22/4/2026 | Después del mapeo | `e3eb871064b17a770ccd2e12d3c3f5f6` |

---

### 13. Anexo: datos crudos de las pruebas

#### A.1 · Barras de Lamine (registro en `0xc51228`) en tres fechas

| Barra | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1/1 (ranura 4) | 8432 | 2340 | 0 | 3308 | 6527 | 9765 | 1342 | 3487 | 1060 | 6598 | 1713 | 5701 | 4671 | 3280 | 8588 | 5874 | 4418 | 8727 | 2415 | 6666 | 8797 | 2707 | 5118 | 3815 |
| 16/3 (ranura 6) | 9969 | 3233 | 0 | 4134 | 7956 | 551 | 2555 | 5024 | 2273 | 7412 | 2322 | 6310 | 6208 | 4817 | 8691 | 5874 | 5631 | 9940 | 3775 | 7492 | 9690 | 3891 | 5118 | 3993 |
| 11/4 (ranura 7) | 595 | 3549 | 0 | 4520 | 8592 | 327 | 3026 | 5650 | 2744 | 7699 | 2542 | 6530 | 6834 | 5443 | 8752 | 5874 | 6102 | 411 | 4296 | 7878 | 6 | 4307 | 5118 | 4086 |

Barras 15–18 = 0 y 29–30 = 0 en las tres fechas.
- Entre el 1/1 y el 16/3 dio la vuelta la barra 6 → Conservación 92→93 ✅.
- Entre el 16/3 (con la barra 6 puesta a mano en 9.999) y el 11/4 dieron la vuelta las barras 1, 6, 22 y 25 → Actitud ofensiva, Conservación, Equilibrio y Salto ✅.
- La barra 20 (Velocidad) está **quieta** en 5874: su Velocidad está en 99.

#### A.2 · Cómo se leyeron las capturas

- Páginas **2/4** (17 habilidades) y **3/4** (Actitud defensiva, Recuperación, Agresividad, 5 de portero, pie malo ×2, regularidad, resistencia a lesiones) de la pantalla «Habilidad» de cada jugador.
- Todas las diferencias antes→después fueron de **0 o +1**: ninguna bajó y ninguna subió +2. Comprobado con un programa.

#### A.3 · Habilidades de los 7 jugadores, antes (ranura 8) y después (ranura 9)

Formato de cada celda: antes → después. En **negrita**, las que subieron.

| Habilidad | Roony Bardghji | Josué Caicedo | Gavi | Fermín López | Lamine Yamal | Marc Bernal | Pau Cubarsí |
|---|---|---|---|---|---|---|---|
| **Media** | 78 → 78 | 58 → 59 | 83 → 83 | 86 → 88 | 90 → 90 | 78 → 80 | 87 → 88 |
| Actitud ofensiva | 70 | **76 → 77** | 72 | 78 | 84 | **70 → 71** | **70 → 71** |
| Control de balon | **82 → 83** | 56 | 85 | 88 | **89 → 90** | 73 | **74 → 75** |
| Regate | 90 | **63 → 64** | 86 | **85 → 86** | **93 → 94** | 73 | **70 → 71** |
| Conservacion del balon | 82 | 60 | 88 | **87 → 88** | 94 | **76 → 77** | **73 → 74** |
| Pase raso | **62 → 63** | 56 | 84 | **88 → 89** | **82 → 83** | **76 → 77** | 75 |
| Pase bombeado | **64 → 65** | 62 | 80 | 80 | 84 | **74 → 75** | **73 → 74** |
| Finalizacion | 74 | **60 → 61** | **69 → 70** | **82 → 83** | 81 | 66 | 60 |
| Cabeceo | **60 → 61** | 64 | 53 | 68 | **62 → 63** | **68 → 69** | 87 |
| Balon parado | 68 | 49 | **57 → 58** | 73 | **73 → 74** | 72 | **66 → 67** |
| Efecto | **77 → 78** | **44 → 45** | 75 | 76 | **85 → 86** | 72 | 70 |
| Velocidad | 88 | **77 → 78** | **76 → 77** | 84 | 99 | 76 | **80 → 81** |
| Aceleracion | 92 | 78 | **82 → 83** | 86 | **93 → 94** | **74 → 75** | **77 → 78** |
| Potencia de tiro | 72 | **65 → 66** | **69 → 70** | 83 | **78 → 79** | 77 | 81 |
| Salto | 63 | 76 | **67 → 68** | **65 → 66** | **72 → 73** | **73 → 74** | 87 |
| Contacto fisico | 61 | 63 | 67 | **66 → 67** | **76 → 77** | 83 | **85 → 86** |
| Equilibrio | 84 | 61 | **88 → 89** | **90 → 91** | 88 | **75 → 76** | 70 |
| Resistencia | **67 → 68** | **56 → 57** | **83 → 84** | 80 | 82 | 79 | 90 |
| Actitud defensiva | **53 → 54** | **40 → 41** | 64 | 63 | 55 | **75 → 76** | 90 |
| Recup. de balon | **49 → 50** | **64 → 65** | 70 | **65 → 66** | 55 | 78 | 89 |
| Agresividad | **51 → 52** | 50 | 85 | **71 → 72** | **67 → 68** | 77 | 89 |
| Actitud de portero | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Atajar | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Despejar | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Reflejos | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Cobertura | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Uso pie malo | 2 | 2 | 4 | 2 | 3 | 2 | 2 |
| Prec. pie malo | 3 | 3 | 3 | 2 | 3 | 2 | 2 |
| Regularidad | 5 | 5 | 3 | 3 | 5 | 3 | 2 |
| Resist. lesiones | 2 | 3 | 2 | 2 | 2 | 2 | 2 |

#### A.4 · Comprobación jugador por jugador

| Jugador | Barras puestas en 9.999 | Barras que dieron la vuelta (archivo) | Habilidades que subieron (pantalla) | ¿Cuadra? |
|---|---|---|---|---|
| Roony Bardghji | 2, 5, 8, 9, 10, 11, 12, 14, 26 | 2, 5, 8, 9, 10, 11, 12, 14, 26 | Control de balon, Pase raso, Pase bombeado, Cabeceo, Efecto, Resistencia, Actitud defensiva, Recup. de balon, Agresividad (9) | ✅ |
| Josué Caicedo | 1, 2, 4, 7, 11, 14, 20, 23, 26 | 1, 2, 4, 7, 11, 14, 20, 23, 26 | Actitud ofensiva, Regate, Finalizacion, Efecto, Velocidad, Potencia de tiro, Resistencia, Actitud defensiva, Recup. de balon (9) | ✅ |
| Gavi | 7, 13, 20, 22, 23, 24, 25, 26 | 7, 13, 20, 22, 23, 24, 25, 26 | Finalizacion, Balon parado, Velocidad, Aceleracion, Potencia de tiro, Salto, Equilibrio, Resistencia (8) | ✅ |
| Fermín López | 4, 6, 7, 8, 11, 12, 21, 22, 25 | 4, 6, 7, 8, 11, 12, 21, 22, 25 | Regate, Conservacion del balon, Pase raso, Finalizacion, Salto, Contacto fisico, Equilibrio, Recup. de balon, Agresividad (9) | ✅ |
| Lamine Yamal | 5, 10, 12, 13, 14, 21, 23, 25 | 4, 5, 8, 10, 12, 13, 14, 21, 23, 24, 25 | Control de balon, Regate, Pase raso, Cabeceo, Balon parado, Efecto, Aceleracion, Potencia de tiro, Salto, Contacto fisico, Agresividad (11) | ✅ |
| Marc Bernal | 1, 2, 6, 8, 9, 10, 22, 24 | 1, 2, 6, 8, 9, 10, 22, 24, 25 | Actitud ofensiva, Conservacion del balon, Pase raso, Pase bombeado, Cabeceo, Aceleracion, Salto, Equilibrio, Actitud defensiva (9) | ✅ |
| Pau Cubarsí | 1, 4, 5, 6, 9, 13, 20, 21, 24 | 1, 4, 5, 6, 9, 13, 20, 21, 24 | Actitud ofensiva, Control de balon, Regate, Conservacion del balon, Pase bombeado, Balon parado, Velocidad, Aceleracion, Contacto fisico (9) | ✅ |

#### A.5 · Tabla del código (qué 3 jugadores recibieron cada barra)

| Barra | Jugadores | Habilidad que salió |
|---|---|---|
| 1 | Caicedo, Bernal, Cubarsí | Actitud ofensiva |
| 2 | Bardghji, Caicedo, Bernal | Actitud defensiva |
| 4 | Caicedo, Fermín, Cubarsí | Regate |
| 5 | Bardghji, Lamine, Cubarsí | Control de balón |
| 6 | Fermín, Bernal, Cubarsí | Conservación del balón |
| 7 | Caicedo, Gavi, Fermín | Finalización |
| 8 | Bardghji, Fermín, Bernal | Pase raso |
| 9 | Bardghji, Bernal, Cubarsí | Pase bombeado |
| 10 | Bardghji, Lamine, Bernal | Cabeceo |
| 11 | Bardghji, Caicedo, Fermín | Recuperación de balón |
| 12 | Bardghji, Fermín, Lamine | Agresividad |
| 13 | Gavi, Lamine, Cubarsí | Balón parado |
| 14 | Bardghji, Caicedo, Lamine | Efecto |
| 20 | Caicedo, Gavi, Cubarsí | Velocidad |
| 21 | Fermín, Lamine, Cubarsí | Contacto físico |
| 22 | Gavi, Fermín, Bernal | Equilibrio |
| 23 | Caicedo, Gavi, Lamine | Potencia de tiro |
| 24 | Gavi, Bernal, Cubarsí | Aceleración |
| 25 | Gavi, Fermín, Lamine | Salto |
| 26 | Bardghji, Caicedo, Gavi | Resistencia |

#### A.6 · Posición exacta de cada cambio (archivo descifrado de la ranura 8)

| Jugador | Índice en la tabla | Barra | Posición (hex) | Valor viejo | Valor nuevo |
|---|---|---|---|---|---|
| Josué Caicedo | 23 | 1 | 0xc5295e | 6176 | 9999 |
| Marc Bernal | 17 | 1 | 0xc520be | 8930 | 9999 |
| Pau Cubarsí | 1 | 1 | 0xc509be | 8857 | 9999 |
| Roony Bardghji | 22 | 2 | 0xc527f0 | 7772 | 9999 |
| Josué Caicedo | 23 | 2 | 0xc52960 | 7112 | 9999 |
| Marc Bernal | 17 | 2 | 0xc520c0 | 9286 | 9999 |
| Josué Caicedo | 23 | 4 | 0xc52964 | 5450 | 9999 |
| Fermín López | 9 | 4 | 0xc51544 | 9033 | 9999 |
| Pau Cubarsí | 1 | 4 | 0xc509c4 | 7432 | 9999 |
| Roony Bardghji | 22 | 5 | 0xc527f6 | 5479 | 9999 |
| Lamine Yamal | 7 | 5 | 0xc51266 | 8592 | 9999 |
| Pau Cubarsí | 1 | 5 | 0xc509c6 | 695 | 9999 |
| Fermín López | 9 | 6 | 0xc51548 | 1799 | 9999 |
| Marc Bernal | 17 | 6 | 0xc520c8 | 7914 | 9999 |
| Pau Cubarsí | 1 | 6 | 0xc509c8 | 5092 | 9999 |
| Josué Caicedo | 23 | 7 | 0xc5296a | 386 | 9999 |
| Gavi | 19 | 7 | 0xc523aa | 2078 | 9999 |
| Fermín López | 9 | 7 | 0xc5154a | 6926 | 9999 |
| Roony Bardghji | 22 | 8 | 0xc527fc | 7653 | 9999 |
| Fermín López | 9 | 8 | 0xc5154c | 8477 | 9999 |
| Marc Bernal | 17 | 8 | 0xc520cc | 521 | 9999 |
| Roony Bardghji | 22 | 9 | 0xc527fe | 9808 | 9999 |
| Marc Bernal | 17 | 9 | 0xc520ce | 4866 | 9999 |
| Pau Cubarsí | 1 | 9 | 0xc509ce | 4225 | 9999 |
| Roony Bardghji | 22 | 10 | 0xc52800 | 7338 | 9999 |
| Lamine Yamal | 7 | 10 | 0xc51270 | 7699 | 9999 |
| Marc Bernal | 17 | 10 | 0xc520d0 | 1712 | 9999 |
| Roony Bardghji | 22 | 11 | 0xc52802 | 9728 | 9999 |
| Josué Caicedo | 23 | 11 | 0xc52972 | 4404 | 9999 |
| Fermín López | 9 | 11 | 0xc51552 | 691 | 9999 |
| Roony Bardghji | 22 | 12 | 0xc52804 | 8360 | 9999 |
| Fermín López | 9 | 12 | 0xc51554 | 7276 | 9999 |
| Lamine Yamal | 7 | 12 | 0xc51274 | 6530 | 9999 |
| Gavi | 19 | 13 | 0xc523b6 | 1349 | 9999 |
| Lamine Yamal | 7 | 13 | 0xc51276 | 6834 | 9999 |
| Pau Cubarsí | 1 | 13 | 0xc509d6 | 598 | 9999 |
| Roony Bardghji | 22 | 14 | 0xc52808 | 8889 | 9999 |
| Josué Caicedo | 23 | 14 | 0xc52978 | 4968 | 9999 |
| Lamine Yamal | 7 | 14 | 0xc51278 | 5443 | 9999 |
| Josué Caicedo | 23 | 20 | 0xc52984 | 7418 | 9999 |
| Gavi | 19 | 20 | 0xc523c4 | 9664 | 9999 |
| Pau Cubarsí | 1 | 20 | 0xc509e4 | 3990 | 9999 |
| Fermín López | 9 | 21 | 0xc51566 | 1909 | 9999 |
| Lamine Yamal | 7 | 21 | 0xc51286 | 6102 | 9999 |
| Pau Cubarsí | 1 | 21 | 0xc509e6 | 5403 | 9999 |
| Gavi | 19 | 22 | 0xc523c8 | 4243 | 9999 |
| Fermín López | 9 | 22 | 0xc51568 | 3227 | 9999 |
| Marc Bernal | 17 | 22 | 0xc520e8 | 6732 | 9999 |
| Josué Caicedo | 23 | 23 | 0xc5298a | 4119 | 9999 |
| Gavi | 19 | 23 | 0xc523ca | 6377 | 9999 |
| Lamine Yamal | 7 | 23 | 0xc5128a | 4296 | 9999 |
| Gavi | 19 | 24 | 0xc523cc | 5867 | 9999 |
| Marc Bernal | 17 | 24 | 0xc520ec | 3627 | 9999 |
| Pau Cubarsí | 1 | 24 | 0xc509ec | 2093 | 9999 |
| Gavi | 19 | 25 | 0xc523ce | 5899 | 9999 |
| Fermín López | 9 | 25 | 0xc5156e | 9819 | 9999 |
| Lamine Yamal | 7 | 25 | 0xc5128e | 6 | 9999 |
| Roony Bardghji | 22 | 26 | 0xc52820 | 2350 | 9999 |
| Josué Caicedo | 23 | 26 | 0xc52990 | 6581 | 9999 |
| Gavi | 19 | 26 | 0xc523d0 | 7895 | 9999 |

---

## Parte 18 · 28 · Mapa final y completo de las barras de crecimiento (28 de 30)

> Origen: `PhoenixSync/base-conocimiento/28-MAPA-FINAL-BARRAS-DE-CRECIMIENTO.md` · 8578 bytes. Copia íntegra.

**Para FRALEX · 10 de octubre de 2026, 09:40 (Lima).**
Cierre de la investigación de las guías 23, 24, 25 y 26. Aquí está **todo resuelto**: ya no falta ninguna barra de portero ni de las lentas.
Diario paso a paso: `PRUEBAS.md` (10 oct, 09:30 a 09:37). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado en pantalla y en el archivo · 🔎 comprobado solo en pantalla · 🧩 depende del parche.

---

### 0. En simple

Hace unas horas (guía 26) el mapa tenía **26 de 28 barras** conocidas. Faltaban dos: la 17 y la 18, que tenían que ser **Reflejos** y **Cobertura** (las dos habilidades de portero que quedaban sin barra), pero no sabíamos cuál era cuál.

Se hizo una prueba más (la «ronda 2»): a **Joan García** se le subió la barra 17 y se le bajó la 18; a **Szczęsny**, al revés (17 abajo, 18 arriba). Resultado en pantalla:

| Portero | Reflejos | Cobertura |
|---|---|---|
| Joan García | **92 → 93** (subió) | 91 → 91 (igual) |
| Szczęsny | **87 → 86** (bajó) | 87 → 87 (igual) |

Reflejos subió exactamente donde se plantó la barra 17 hacia arriba (Joan) **y** bajó exactamente donde se plantó la barra 17 hacia abajo (Szczęsny). Eso confirma, por partida doble: **barra 17 = Reflejos**.

Cobertura no se movió en esta prueba (probablemente porque esta vez se avanzó menos tiempo en el juego que en la ronda 1, y su barra no llegó a completar la vuelta). Pero como ya no queda ninguna otra habilidad de portero sin dueño, por descarte: **barra 18 = Cobertura**.

Con esto, **las 28 barras que el juego realmente usa ya tienen dueño conocido.**

---

### 1. La tabla final completa (28 de 30 barras)

| Barra | Habilidad | Cómo se confirmó |
|---|---|---|
| 1 | Actitud ofensiva | 🔎 código secreto (guía 25) |
| 2 | Actitud defensiva | ✅ código secreto |
| 3 | Actitud de portero | ✅ ronda 1 (guía 26) |
| 4 | Regate | ✅ código secreto |
| 5 | Control de balón | ✅ código secreto |
| 6 | Conservación del balón | ✅ palanca (guía 24) + código secreto |
| 7 | Finalización | ✅ código secreto |
| 8 | Pase raso | ✅ código secreto |
| 9 | Pase bombeado | ✅ código secreto |
| 10 | Cabeceo | ✅ código secreto |
| 11 | Recuperación de balón | ✅ código secreto |
| 12 | Agresividad | ✅ código secreto |
| 13 | Balón parado | ✅ código secreto |
| 14 | Efecto | ✅ código secreto |
| 15 | Atajar (portero) | ✅ ronda 1 (sube en Joan, baja en Szczęsny) |
| 16 | Despejar (portero) | ✅ ronda 1 |
| **17** | **Reflejos (portero)** | **✅ ronda 2 (sube en Joan, baja en Szczęsny) — NUEVO** |
| **18** | **Cobertura (portero)** | **🔎 ronda 2, por descarte (único candidato que quedaba) — NUEVO** |
| 19 | Precisión con el pie malo | ✅ ronda 1 |
| 20 | Velocidad | ✅ código secreto |
| 21 | Contacto físico | ✅ código secreto |
| 22 | Equilibrio | ✅ código secreto |
| 23 | Potencia de tiro | ✅ código secreto |
| 24 | Aceleración | ✅ código secreto |
| 25 | Salto | ✅ código secreto |
| 26 | Resistencia | ✅ código secreto |
| 27 | Regularidad | ✅ ronda 1 |
| 28 | Resistencia a lesiones | ✅ ronda 1 |
| 29 | *(sin usar, siempre 0)* | — |
| 30 | *(sin usar, siempre 0)* | — |

**Lo único que queda sin resolver de todo este tema:** la habilidad **«Uso de pie malo»** no tiene barra conocida — ya no quedan números libres para ella (solo la 29 y la 30, que siempre están en 0 en todos los jugadores probados). Lo más probable es que esta habilidad **no suba por barra**, sino por otro mecanismo del juego que no hemos buscado todavía (o que de verdad no suba nunca de forma automática). Para la gran mayoría de lo que Phoenix necesita (subir/bajar cualquier otra habilidad desde la web), esto no es un obstáculo: ya se puede hacer con 28 de las ~29 habilidades «de barra».

---

### 2. Cómo se hizo la ronda 2, paso a paso (para humano o IA)

1. **Base:** la ranura 11 (`ML0000000A`, guardado del 2/5/2026, ya confirmado byte a byte contra la ronda 1).
2. **Respaldo primero**, con huella sha256 comprobada igual al original, en `_PhoenixMercado_prueba/respaldos_ranuras/`.
3. **Cambios con `palanca_varias`** (verifica cada valor viejo antes de escribir cualquier cosa; si uno no coincide, no se toca nada):
   - Joan García: barra 17 (offset `0xc5086e`) de 7307 → **9999**; barra 18 (offset `0xc50870`) de 5220 → **0**.
   - Szczęsny: barra 17 (offset `0xc5183e`) de 5311 → **0**; barra 18 (offset `0xc51840`) de 7717 → **9999**.
   - Se comprobó que **solo 8 bytes** del archivo cambiaron (los 4 números, de 2 bytes cada uno). Nada más se tocó.
4. **Entrega al PC:** por el puente de archivos, pero **comprimido en `.zip`** (ver aviso de la sección 3) y guardado en la ranura 12 (`ML0000000B`). Huella sha256 comprobada igual en los dos lados.
5. **FRALEX:** cargó la ranura 12, miró la página 3/4 de Joan García y Szczęsny (capturas «antes»), avanzó, y volvió a mirar la misma página (capturas «después»).
6. **Lectura del resultado:** la habilidad que sube en el jugador donde se plantó la barra hacia arriba **y** baja en el jugador donde se plantó hacia abajo, es esa barra. Si ninguna de las dos se mueve pero ya no queda otro candidato, se asigna por descarte.

**Para que una IA repita esto con otro juego de barras desconocidas:** el truco de «subir en un jugador, bajar en otro, al mismo tiempo» es más fuerte que solo subir o solo bajar, porque confirma la barra **dos veces en el mismo paso** (sube donde debía subir, baja donde debía bajar) — y si por algún motivo una de las dos direcciones no llega a completarse a tiempo, la otra dirección (o el descarte, si ya no quedan candidatos) igual resuelve el misterio.

---

### 3. ⚠️ Aviso importante para cualquiera que use el puente de archivos hacia el PC

Al mandar `ML0000000B` (un archivo de guardado, **sin extensión** en el nombre) directo por el puente, **llegó dañado**: en el contenedor pesaba 19.831.781 bytes y en el PC llegó con solo 19.813.994 (le faltaban 17.787 bytes), sin que la herramienta avisara de ningún error.

**Causa probable:** un archivo sin extensión (como todos los `MLxxxxxxxx`) puede ser tratado como texto por el puente, y le quita bytes que por casualidad se parecen a saltos de línea de Windows, dentro de los datos cifrados.

**Arreglo que sí funciona:** comprimir el archivo en `.zip` antes de mandarlo (eso viaja byte por byte sin tocarlo) y descomprimirlo ya en el PC. Se comprobó con huella sha256 en cada paso.

**Regla para todo lo que venga:** nunca mandar un `MLxxxxxxxx` (o cualquier archivo sin extensión) directo por el puente. Siempre comprimir primero, y siempre comprobar tamaño y huella en el PC después — no basta con que la herramienta no muestre error.

---

### 4. Qué abre esto para Phoenix Sync

- Ya se puede, en una carrera **ya empezada**, subir o bajar **en +1/−1** casi cualquier habilidad de un jugador del equipo del usuario (de las 28 de 30 barras), de forma controlada y verificada.
- Es la pieza que faltaba para que la web pueda pedir, por ejemplo, «sube la Conservación del balón de este jugador» y que Phoenix Sync lo traduzca en el cambio correcto dentro del guardado.
- Sigue pendiente (no es parte de este cierre): diseñar cómo se conecta esto con la web (qué pide la web, qué responde Phoenix Sync, cómo se entrega el archivo al jugador) y decidir qué pasa con «Uso de pie malo».

---

### 5. Multiparche 🧩

- El mecanismo de las barras (0 a 9.999, +1 al completar hacia arriba, −1 al completar hacia abajo) es del motor del juego: vale igual en cualquier parche de PES 2021.
- Lo que cambia por parche es **dónde** cae la ficha de cada jugador dentro del guardado — por eso todo esto se hace siempre verificando el valor antes de tocar, nunca con posiciones fijas a ciegas.
- Probado solo en **ConmeGOL 26**, carrera del Barça de FRALEX.

---

### 6. Pendiente

1. Averiguar si «Uso de pie malo» tiene alguna forma de subir (otra barra en otro lugar del archivo, o un mecanismo distinto).
2. Confirmar la ronda 2 también a nivel de archivo (ranura 13, cuando FRALEX guarde el «después»).
3. Probar si se puede subir **más de +1** de una vez, o hacerlo más rápido que una barra a la vez.
4. Diseñar la función real «subir/bajar habilidad» dentro de Phoenix Sync, conectada a la web.

---

## Parte 19 · 29 · Informe maestro: subir y bajar habilidades en una Liga Máster ya empezada (barras de crecimiento) — versión completa y final

> Origen: `PhoenixSync/base-conocimiento/29-INFORME-MAESTRO-BARRAS-DE-CRECIMIENTO.md` · 52106 bytes. Copia íntegra.

**Para FRALEX, y para cualquier persona o IA que quiera repetirlo desde cero, en cualquier momento.**
**Fecha de cierre:** 10 de octubre de 2026, 09:45 (Lima).
**Juego:** PES 2021 (eFootball PES 2021 Season Update), PC.
**Parche de la prueba:** ConmeGOL 26. Las diferencias con otros parches están en la sección 9.
**Proyecto:** Phoenix Evolution Series → Phoenix Sync (repo `Fralexito/smash-soda-fork`, rama `mercado-fase0`, carpeta `PhoenixSync/`).

**Este documento reemplaza y amplía al informe 26.** Junta todo lo que cuentan las guías 21, 23, 24, 25, 26, 27 (solo la parte de aviso de archivos) y 28, más los datos que faltaban, en **un solo lugar**, para que nadie tenga que ir saltando entre documentos. Si solo vas a leer uno de todos los informes de barras de crecimiento, que sea este.

**No falta nada por descubrir en el mapa de barras.** De las 30 barras que tiene cada jugador, **28 están identificadas**. Las otras 2 siempre están en 0 y no se usan. Lo único que queda abierto es si «Uso de pie malo» tiene algún otro mecanismo de subida (sección 11).

**Cómo leerlo:**
- Si eres **humano** y quieres entender qué se logró, lee las secciones **0 a 3**.
- Si eres una **IA** que quiere repetir esto paso a paso (en este guardado o en otro), lee **todo**, sobre todo las secciones **4, 5, 6, 7 y el Anexo**.

Símbolos:
- ✅ comprobado en el juego (con pantalla) **y** en el archivo;
- 🔎 comprobado solo en uno de los dos (pantalla o archivo), o por descarte lógico;
- ⏳ falta comprobar;
- 🧩 puede cambiar según el parche.

---

### Índice

0. Resumen en una página
1. Glosario: cada palabra explicada
2. La historia completa: los 6 experimentos
3. Resultado final: el mapa completo de barras y las reglas
4. Especificación técnica: dónde y cómo están las barras en el archivo
5. Guía de réplica, paso a paso (para humanos)
6. Guía de réplica, paso a paso (para una IA o un programa)
7. Métodos para mapear varias barras en una sola prueba («código secreto» y «cruce de dos jugadores»)
8. Lista de comprobación de 0 errores (incluye el aviso del puente de archivos)
9. Multiparche: qué cambia en otros parches
10. Riesgos, límites y cosas que NO hacer
11. Lo que falta descubrir y cómo hacerlo
12. Archivos, herramientas y huellas
13. Anexo: datos crudos de todas las pruebas

---

### 0. Resumen en una página

**El problema.**
Cuando cambiamos las habilidades de un jugador en la base del parche (Phoenix-DB, `Player.bin`), el cambio se ve en amistosos. Pero **no** se ve en una Liga Máster ya empezada.

**Por qué.**
Al **crear** una carrera, el juego hace una copia («foto») de las habilidades de todos los jugadores, y desde ahí usa **su** copia. ✅ (guía 21)

**Lo que encontramos.**
Dentro del guardado de la carrera, cada jugador del **equipo del usuario** tiene unas **30 barras de progreso**, una por habilidad.
- Cada barra es un número de **0 a 9.999**.
- Con los días y los partidos, el juego **llena** las barras de los jóvenes y **vacía** las de los veteranos.
- Cuando una barra pasa de **9.999** (hacia arriba), vuelve a empezar desde un número bajo y **esa habilidad sube +1**. ✅
- Cuando una barra baja de **0** (hacia abajo), vuelve a empezar desde un número alto (cerca de 9.999) y **esa habilidad baja −1**. ✅ (comprobado el 10 de octubre con Szczęsny: Atajar 82→81, Despejar 83→82).

**La palanca.**
Nosotros ponemos a mano una barra en **9.999** (para que suba) o en **0** (para que baje). Cuando el juego avanza, termina de llenar o vaciar la barra, y él mismo sube o baja la habilidad.
Primer caso probado: Lamine Yamal, Conservación del balón **93 → 94**. ✅

**El mapa — COMPLETO.**
Con varias pruebas (ver sección 2), se identificaron las **28 de 30 barras** que el juego usa de verdad:
- Las 20 habilidades de jugador de campo (código secreto con 7 jóvenes).
- Las 5 habilidades de portero (Actitud de portero, Atajar, Despejar, Reflejos, Cobertura).
- 3 barras «lentas» (Precisión de pie malo, Regularidad, Resistencia a lesiones).
- Las 2 barras restantes (29 y 30) siempre están en 0: no se usan.

**Qué se puede hacer hoy.**
Subir **+1** o bajar **−1** casi cualquier habilidad de cualquier jugador **del equipo del usuario**, en una carrera ya empezada, usando el propio sistema del juego — sin tocar el ejecutable ni la memoria del juego, solo el archivo de guardado.

**Qué falta (ya no es sobre el mapa, son afinamientos):**
- Si «Uso de pie malo» tiene barra en otro lugar, o si no sube por barra.
- Subir/bajar más de 1 de una vez, o más rápido.
- Jugadores de otros equipos (no tienen tabla de barras, por ahora sin alternativa encontrada).
- El valor absoluto de cada habilidad (dónde guarda el juego el «93» en sí, aparte de la barra de progreso).
- Conectar esto con la web de Phoenix Evolution.

---

### 1. Glosario: cada palabra explicada

| Palabra | Qué significa, en simple |
|---|---|
| **Liga Máster (LM)** | El modo carrera de PES 2021: eres entrenador de un club durante varias temporadas. |
| **Carrera** | Una partida concreta de Liga Máster. |
| **Ranura** | Cada «casilla» de guardado del menú Cargar. La ranura **n** es el archivo `ML0000000(n−1)` en la carpeta `save`, escrito en hexadecimal (después del 9 sigue la letra A, luego B…). Ejemplo: la ranura 6 es `ML00000005`; la ranura 11 es `ML0000000A`; la ranura 12 es `ML0000000B`. |
| **Carpeta save** | En el PC de FRALEX: `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`. Normalmente: `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<id>\save`. |
| **Guardado cifrado** | El archivo `ML…` tal como está en el disco. Está protegido (cifrado y comprimido): no se lee a simple vista. |
| **Sobre (envoltura)** | La capa de protección del guardado. Phoenix Sync la quita y la vuelve a poner con `SobrePes` (basado en libpesXcrypter). Guía 10. |
| **Datos descifrados** | El guardado ya sin el sobre: unos 19,8 MB de datos que se pueden leer byte a byte. |
| **Byte** | La unidad mínima de un archivo: un número de 0 a 255. |
| **u16 / u32** | Un número guardado en 2 bytes (de 0 a 65.535) o en 4 bytes. Se guardan «al revés» (*little-endian*): el byte de menos peso va primero. Ejemplo: 9.999 = `0x270F` se escribe `0F 27`. |
| **Hex (0x…)** | Forma de escribir números en base 16, la habitual para posiciones en archivos. `0xc51228` = 12.915.240. |
| **Posición (offset)** | Dónde está un dato dentro de los datos descifrados, contando desde 0. |
| **pid** | Número de identidad de un jugador en la base del juego. Lamine Yamal = **162114**. |
| **reg** | Número interno del jugador **dentro de la carrera**. Cambia de carrera a carrera (en la del Barça de esta investigación: Lamine = `0xdb1`). |
| **Plantilla** | Lista de jugadores de un equipo. |
| **Equipo del usuario** | El club que manejas tú en la carrera (aquí, el FC Barcelona). |
| **Tablas alineadas** | Listas que el juego guarda **solo para el equipo del usuario**, con un registro por jugador, en el mismo orden que su plantilla. |
| **Tabla D** | Una de esas tablas. Cada registro mide **368 bytes**. Ahí viven las barras. |
| **Barra de crecimiento** | Cada uno de los 30 números u16 (0–9.999) de un registro de la tabla D. Es el «progreso» hacia el siguiente +1 (o −1) de una habilidad. |
| **Dar la vuelta (hacia arriba)** | Cuando una barra pasa de 9.999, vuelve a empezar desde un número pequeño: la habilidad sube +1. |
| **Dar la vuelta (hacia abajo)** | Cuando una barra baja de 0, vuelve a empezar desde un número grande (cerca de 9.999): la habilidad baja −1. |
| **Palanca** | Nuestro truco: poner una barra en 9.999 (o en 0) a mano para que el juego suba (o baje) la habilidad en el siguiente avance. |
| **Huella (md5 / sha256)** | Un «DNI» del archivo, calculado a partir de su contenido. Si cambia un solo byte, la huella cambia. Sirve para comprobar que un archivo llegó idéntico. |
| **Respaldo** | Copia de seguridad, siempre con un nombre nuevo, para poder volver atrás. |
| **Phoenix-DB** | La carpeta de la base del parche que usa Sider (`Player.bin`, etc.). |
| **Puente de archivos** | La conexión que usa Claude para leer y escribir archivos directamente en el PC de FRALEX. **Cuidado especial con archivos sin extensión** (ver sección 8 y 10). |
| **Multiparche** | Que funcione en cualquier parche de PES 2021, no solo en ConmeGOL. |

---

### 2. La historia completa: los 6 experimentos

#### Experimento 1 · ¿Llegan las stats de Phoenix-DB a una Liga Máster? (9 oct, guías 18 y 21)

1. Pusimos a Lamine con Velocidad **99** en `Player.bin` (Phoenix-DB). En la base original tiene 90.
2. En un amistoso salió con **99**. ✅ En la carrera del Barça ya empezada salió con **90**. ❌
3. Creamos **dos carreras nuevas** iguales:
   - ranura 4, con la base 99 → Lamine sale con 99;
   - ranura 5, con la base 90 → Lamine sale con 90.
4. Prueba decisiva: con la base en **90**, cargamos la ranura 4 → Lamine sigue con **99**. ✅

**Conclusión:** la carrera **copia** las habilidades al crearse y desde ahí usa su copia. ✅

Buscamos el número «99» de esa copia dentro del guardado de muchas formas (literal, empaquetado en 6, 7 u 8 bits, en orden y sin orden). **No apareció**. El valor absoluto de las habilidades sigue sin localizar. ⏳

#### Experimento 2 · Descubrir las barras (10 oct, 00:50; guía 23)

1. Se compararon **la misma carrera en dos fechas**: ranura 4 (1/1/2026) y ranura 6 (16/3/2026). Así hay mucho menos «ruido» que entre dos carreras distintas: 191.549 bytes distintos, frente a 1,77 millones.
2. En el registro de 368 B de Lamine había unos **30 números entre 0 y 9.999** que habían **subido casi todos**.
3. Uno, el n.º 6, pasó de **9.765 a 551**: había llegado al tope y había empezado de nuevo.
4. **Predicción**, hecha antes de mirar la pantalla: «en la ranura 6, Lamine tendrá exactamente una habilidad con +1».
5. **Pantalla:** Conservación del balón **92 → 93**, y todo lo demás igual. ✅

→ Son barras de crecimiento, y la **barra 6 = Conservación del balón**.

#### Experimento 3 · La palanca (10 oct, 01:05–01:11; guía 24)

1. Respaldo de la ranura 6.
2. En una copia, la barra 6 de Lamine pasó de **551 a 9.999**. Solo cambiaron 2 bytes. Esa copia quedó en la ranura 6.
3. FRALEX la cargó: Conservación **93** (no cambia todavía, porque no ha avanzado nada).
4. Avanzó varios días: Conservación **94**. ✅
5. En la ranura 7 (11/4/2026), guardada después, el archivo mostró que a Lamine le dieron la vuelta las barras **1, 6, 22 y 25**. En pantalla le subieron **4** habilidades: Actitud ofensiva, Conservación, Equilibrio y Salto. ✅ Cuadra.

#### Experimento 4 · El mapa de las 20 habilidades de campo (10 oct, 01:20–01:50; guía 25)

1. Desde la ranura 7 se hizo la **ranura 8**, con **60 barras en 9.999** repartidas entre 7 jugadores jóvenes. Es el «código secreto» de la sección 7.1.
2. FRALEX sacó capturas **antes**, avanzó unos 11 días, sacó capturas **después**, y guardó en la **ranura 9**.
3. **Archivo (ranura 9):** las 60 barras dieron la vuelta, **60 de 60**. Además, 4 dieron la vuelta solas por crecimiento normal.
4. **Pantalla:** a cada jugador le subieron **exactamente** las habilidades que predicen sus barras: **64 de 64**. Comprobado también con un programa.
5. Con el trío de jugadores de cada habilidad se dedujo su barra. Dos parejas empatadas (8/12 y 13/24) se separaron con Marc Bernal, que tenía naturalmente una de las dos barras de cada pareja pero no la otra.

**Resultado:** las 20 barras de campo quedaron mapeadas (1, 2, 4–14, 20–26).

#### Experimento 5 · Porteros y barras lentas, ronda 1 (10 oct, 09:05–09:30; `PRUEBAS.md`)

1. Desde la ranura 9 (actualizada hasta la ranura 10 con los cambios), se prepararon **7 cambios en 5 jugadores**:
   - **Joan García** (portero joven): barra 3 → 9.999, barra 15 → 9.999.
   - **Wojciech Szczęsny** (portero veterano): barra 15 → **0**, barra 16 → **0**. (Esta era, a la vez, **la primera prueba de bajar una barra**.)
   - **Lamine Yamal**: barra 28 → 9.999.
   - **Fermín López**: barra 27 → 9.999.
   - **Alejandro Balde**: barra 19 → 9.999.
2. FRALEX cargó la ranura 10 sin avanzar (confirmó que cargaba bien), avanzó unos **10 días** (del 22/4 al 2/5/2026), y guardó en la **ranura 11**.
3. **Pantalla (página 3/4):**
   - Joan García: Actitud de portero **92→93**, Atajar **90→91**.
   - Szczęsny: Atajar **82→81**, Despejar **83→82**. ✅ **Primera confirmación de que bajar una barra de 0 resta −1.**
   - Lamine: Resistencia a lesiones **2→3**.
   - Fermín: Regularidad **3→4**.
   - Balde: Precisión con el pie malo **2→3**.
4. **Decodificación:** Atajar subió en Joan **y** bajó en Szczęsny → coincide con la barra 15 puesta en ambos sentidos → **barra 15 = Atajar** (confirmado dos veces). Solo Joan tenía la barra 3 → **barra 3 = Actitud de portero**. Solo Szczęsny tenía la barra 16 → **barra 16 = Despejar**.
5. **Archivo (ranura 11):** se comparó, barra por barra, contra la ranura 10. **Las 7 barras plantadas dieron la vuelta exactamente como se esperaba, 7 de 7.** ✅ Las demás barras de estos 5 jugadores también cambiaron, pero por crecimiento natural del juego en los 10 días transcurridos — en jóvenes como Lamine y Fermín ese crecimiento natural puede ser de varios miles en una barra, así que un chequeo automático que solo mire «el número cambió mucho» ya no basta para detectar una vuelta real: hay que cruzar siempre con la pantalla (ver nota en sección 10).

**Resultado:** 6 barras nuevas mapeadas (3, 15, 16, 19, 27, 28). Quedaban 17 y 18 (Reflejos y Cobertura, sin saber cuál es cuál).

#### Experimento 6 · Porteros, ronda 2 (10 oct, 09:36–09:37; `PRUEBAS.md`)

1. Sobre la ranura 11, se prepararon 4 cambios **cruzados** entre los dos únicos porteros disponibles:
   - **Joan García**: barra 17 → **9.999** (sube), barra 18 → **0** (baja).
   - **Szczęsny**: barra 17 → **0** (baja), barra 18 → **9.999** (sube).
2. Se guardó en la **ranura 12**. FRALEX cargó sin avanzar (capturas «antes»), avanzó, y volvió a mirar (capturas «después»).
3. **Pantalla (página 3/4):**
   - Joan García: Reflejos **92→93** (subió), Cobertura 91→91 (igual).
   - Szczęsny: Reflejos **87→86** (bajó), Cobertura 87→87 (igual).
4. **Decodificación:** Reflejos subió donde se plantó la barra 17 hacia arriba (Joan) **y** bajó donde se plantó la barra 17 hacia abajo (Szczęsny) → **barra 17 = Reflejos**, confirmado dos veces en el mismo paso.
5. Cobertura no se movió en ninguno de los dos jugadores esta vez — probablemente porque el tiempo avanzado fue menor que en la ronda 1 y esa barra concreta no llegó a completar la vuelta. Pero como ya no queda ninguna otra habilidad de portero sin barra asignada, **por descarte: barra 18 = Cobertura**. 🔎 (es la única barra del mapa confirmada por descarte lógico y no por observación directa de su propio cambio; sigue siendo razonable, porque no hay ningún otro candidato posible).

**Resultado:** las últimas 2 barras quedaron resueltas. **Mapa completo: 28 de 30 barras.**

---

### 3. Resultado final: el mapa completo de barras y las reglas

#### 3.1 · El mapa — 28 de 30 barras (COMPLETO)

| Barra | Habilidad | Cómo se confirmó |
|---|---|---|
| 1 | Actitud ofensiva | 🔎 código secreto |
| 2 | Actitud defensiva | ✅ código secreto |
| 3 | Actitud de portero | ✅ ronda 1 (experimento 5) |
| 4 | Regate | ✅ código secreto |
| 5 | Control de balón | ✅ código secreto |
| 6 | Conservación del balón | ✅ palanca (experimento 3) + código secreto |
| 7 | Finalización | ✅ código secreto |
| 8 | Pase raso | ✅ código secreto |
| 9 | Pase bombeado | ✅ código secreto |
| 10 | Cabeceo | ✅ código secreto |
| 11 | Recuperación de balón | ✅ código secreto |
| 12 | Agresividad | ✅ código secreto |
| 13 | Balón parado | ✅ código secreto |
| 14 | Efecto | ✅ código secreto |
| 15 | Atajar (portero) | ✅ ronda 1 (sube en Joan, baja en Szczęsny) |
| 16 | Despejar (portero) | ✅ ronda 1 |
| 17 | Reflejos (portero) | ✅ ronda 2 (sube en Joan, baja en Szczęsny) |
| 18 | Cobertura (portero) | 🔎 ronda 2, por descarte (único candidato que quedaba) |
| 19 | Precisión con el pie malo | ✅ ronda 1 |
| 20 | Velocidad | ✅ código secreto |
| 21 | Contacto físico | ✅ código secreto |
| 22 | Equilibrio | ✅ código secreto |
| 23 | Potencia de tiro | ✅ código secreto |
| 24 | Aceleración | ✅ código secreto |
| 25 | Salto | ✅ código secreto |
| 26 | Resistencia | ✅ código secreto |
| 27 | Regularidad | ✅ ronda 1 |
| 28 | Resistencia a lesiones | ✅ ronda 1 |
| 29 | *(sin usar, siempre 0 en todos los jugadores probados)* | — |
| 30 | *(sin usar, siempre 0 en todos los jugadores probados)* | — |

**Sin barra conocida:** «Uso de pie malo». Es la única habilidad de las ~29 «de barra» que no tiene número asignado — y ya no quedan huecos libres para ella. Ver sección 11.

#### 3.2 · Reglas comprobadas

| Regla | Estado |
|---|---|
| Barra que pasa de 9.999 (hacia arriba) → esa habilidad **+1** | ✅ 64/64 (campo) + 7/7 (ronda 1) + 1/1 (ronda 2, Joan) |
| Barra que baja de 0 (hacia abajo) → esa habilidad **−1** | ✅ 2/2 (Szczęsny, ronda 1) + 1/1 (Szczęsny, ronda 2) |
| Poner la barra en 9.999 o en 0 **no** cambia nada hasta que el juego avance | ✅ |
| El juego acepta el guardado modificado sin quejarse | ✅ 5 veces (ranuras 6, 8, 9, 11, 12) |
| La barra no se mueve si la habilidad ya está en **99** | 🔎 muy probable (Velocidad de Lamine, barra 20, quieta en 99) |
| Las barras de los veteranos **bajan** con el tiempo de forma natural, las de los jóvenes **suben** | ✅ visto repetidamente |
| Si suben varias habilidades, la **media** del jugador puede subir | ✅ (Fermín 86→88, Bernal 78→80, Joan García 84→85) |
| Barras 3 y 15–18 distintas de 0 **solo** en porteros | 🔎 |
| Con 10 días de diferencia, el crecimiento natural de una barra puede superar los 3.000 puntos (en jóvenes) | ✅ visto en la ronda 1: un chequeo automático simple (número que cambia mucho) ya no distingue una vuelta real de crecimiento rápido normal; hay que cruzar siempre con la pantalla |
| Crecimiento típico de un joven: 300–1.000 por barra y mes en condiciones normales, pero puede ser mucho más si hay mucha actividad (partidos, entrenamientos) en el período | 🔎 |

---

### 4. Especificación técnica: dónde y cómo están las barras en el archivo

#### 4.1 · Cadena completa

```
ML0000000n (disco, cifrado)
  └─ SobrePes::abrir()          → datos descifrados (~19,8 MB)   [guía 10]
       └─ equipo del usuario    → bloque de equipo con la marca −11 (kMarcaUsuario) en kOfsIdOption
            └─ plantilla        → lista de (reg, pid) de sus jugadores
                 └─ Tabla D     → 1 registro de 368 B por jugador, mismo orden que la plantilla
                      └─ barras → 30 × u16 desde el byte +54 del registro
```

#### 4.2 · Formato de un registro de la tabla D (368 bytes)

| Desde el inicio del registro | Tamaño | Qué es |
|---|---|---|
| +0 | 4 B | campo `x` (sin interpretar) |
| +4 | u32 | **reg** del jugador |
| +8 | u32 | **pid** del jugador |
| +12 … +53 | 42 B | otros datos (sin interpretar del todo; ver ESTRUCTURA-ML §25) |
| **+54** | **30 × u16** | **barras 1 a 30** (barra *k* en **+54 + 2·(k−1)**) |
| +114 … +367 | resto | sin interpretar |

**Fórmula:** `posición de la barra k = inicio del registro + 54 + 2·(k − 1)`, o lo que es lo mismo, `= posición del reg + 50 + 2·(k − 1)`.

**Ejemplo real** (ranura 6, datos descifrados):
- registro de Lamine: empieza en `0xc51228`, con reg `0xdb1` en `0xc5122c` y pid 162114;
- barra 1 → `0xc5125e`; barra 6 → `0xc51268`.

**Otro ejemplo real** (ranura 11, misma carrera, carrera ya avanzada): la posición del **reg** de cada jugador para la tabla D era:
- Joan García: `0xc50818` → base de barras en `0xc50818 + 50 = 0xc5084a` (barra k en `base + 2·(k−1)`).
- Szczęsny: `0xc516e6` → base de barras en `0xc51718`.

(Estas posiciones son **de esta carrera y este momento**: nunca copiarlas a ciegas a otro guardado. Ver 4.3.)

#### 4.3 · Cómo encontrar la tabla D en CUALQUIER guardado (sin posiciones fijas)

El método general ya está programado en `GuardadoLM::tablasDe(k)` (`PhoenixSync/core/LigaMaster.cpp`):

1. Busca el equipo del usuario: el bloque de equipo que lleva la marca **−11** (`kMarcaUsuario`).
2. Lee su plantilla: la lista de pares `(reg, pid)`.
3. Después de los bloques de equipo (`kFinBloques`), busca el par `(reg, pid)` del **primer** jugador (8 bytes seguidos: reg y luego pid).
4. En cada sitio donde aparezca, comprueba que los **4 jugadores siguientes** de la plantilla están a distancias de **368 bytes** (admite huecos vacíos: `pid == 0` y `reg` en 0, 0xffff o vacío).
5. Si cuadra, es la tabla D. Así obtienes `ofsReg0` = posición del reg del registro 0.

**Comprobado varias veces** (incluida la ranura 9, 10 oct 02:05): `tablasDe` encuentra el equipo del usuario y su tabla D con `stride = 368` correctamente, sin usar ninguna posición fija de antemano. ✅

El registro del jugador número *i* de la plantilla empieza en `ofsReg0 − 4 + 368·i`. Puede haber huecos: lo seguro es recorrer los registros y quedarse con el que tenga el `reg`/`pid` buscado.

**Cuidado:** el `reg` cambia de prefijo según la carrera. No hay que suponer el prefijo, solo compararlo tal cual con el de la plantilla.

#### 4.4 · Comportamiento de una barra

- Valor: u16 de **0 a 9.999**. Nunca escribir ≥ 10.000.
- Al avanzar el tiempo, el juego suma (jóvenes) o resta (veteranos) un poco cada día y con los partidos.
- **Dar la vuelta hacia arriba:** valor viejo alto → valor nuevo bajo → habilidad **+1**.
- **Dar la vuelta hacia abajo:** valor viejo bajo (puesto en 0) → valor nuevo alto (cerca de 9.999) → habilidad **−1**. ✅
- **Detectarlo en un programa, con cuidado:** una primera aproximación es `nuevo < viejo − 3000` (vuelta hacia arriba) o `nuevo > viejo + 3000` (vuelta hacia abajo). **Esta aproximación falla si pasó mucho tiempo (varios días) entre las dos lecturas**, porque el crecimiento natural de una barra joven puede superar los 3.000 puntos sin haber completado ninguna vuelta. La forma segura de confirmar una vuelta es siempre cruzar con el cambio visto en pantalla (la habilidad correspondiente subió o bajó exactamente 1).
- Si la habilidad está en **99**, la barra no avanza.

---

### 5. Guía de réplica, paso a paso (para humanos)

**Objetivo de ejemplo:** subir +1 la Velocidad (barra 20) de un jugador de tu equipo en una carrera empezada. (Para bajar una habilidad, es exactamente igual pero llevando la barra a **0** en vez de a 9.999.)

**Lo que necesitas:**
- el juego cerrado;
- la carrera guardada en una ranura;
- una ranura **libre** para la prueba;
- la herramienta `palanca` o `palanca_varias` (sección 12).

1. **Elige la carrera y el jugador.** Mira en el juego su habilidad actual (página 2/4 o 3/4 de «Habilidad») y apúntala. Si ya está en 99, no se puede subir. Si ya está en el mínimo, no se puede bajar.
2. **Cierra el juego.**
3. **Haz un respaldo** del guardado (`ML0000000n`). Cópialo con **otro nombre** a una carpeta de respaldos y comprueba que la huella es la misma.
4. **Busca el registro del jugador** en la tabla D y lee el valor actual de su barra (sección 6, pasos 3 y 4).
5. **Cambia solo esa barra** (a 9.999 para subir, o a 0 para bajar), y guarda el resultado en un **archivo nuevo** destinado a una **ranura libre**. Pon un texto que se vea en «Cargar», por ejemplo «PRUEBA +1 VELOCIDAD».
6. **Comprueba el archivo nuevo:** al abrirlo otra vez, solo deben ser distintos **2 bytes** respecto al original (por cada barra cambiada).
7. **Entrega el archivo al PC.** ⚠️ **Si lo mandas por un puente de archivos remoto (como el que usa Claude para escribir en el PC), el archivo `ML0000000n` NO tiene extensión y puede llegar dañado si se manda directo** (ver sección 8 y 10 para el porqué). Lo seguro es **comprimirlo en `.zip` primero**, entregarlo, y descomprimirlo ya en el PC — y comprobar el tamaño/huella en el PC después, sin confiar en que «no dio error».
8. **Cópialo a la carpeta save**, en la ranura libre, y comprueba la huella allí.
9. **Abre el juego** y carga esa ranura. La habilidad sigue igual. Eso es lo esperado.
10. **Avanza** al menos un día. Lo seguro es hasta el siguiente entrenamiento o partido.
11. **Mira al jugador:** la habilidad tiene que haber subido (o bajado) **1**.
12. **Guarda en otra ranura nueva**, nunca encima del original. Así queda la prueba del «después».
13. **Anota todo** en PRUEBAS.md (qué, cuándo, huellas, resultado) y una línea en REGISTRO.md.

**Para subir/bajar +2 o +3:** repite los pasos 3 a 12 sobre el nuevo guardado. La barra vuelve a empezar después de cada vuelta.

**Truco para resolver 2 barras desconocidas con solo 2 jugadores en 1 solo paso** (usado en el experimento 6): al jugador A, súbele la barra X y bájale la barra Y a la vez; al jugador B, hazlo al revés (baja X, sube Y). La habilidad que sube en A y baja en B es X; la que baja en A y sube en B es Y. Si por algún motivo una de las dos direcciones no llega a completar la vuelta a tiempo, y ya no queda ningún otro candidato posible para esa habilidad, se puede asignar **por descarte**.

---

### 6. Guía de réplica, paso a paso (para una IA o un programa)

#### 6.1 · Herramientas que ya existen

| Herramienta | Dónde | Qué hace |
|---|---|---|
| `SobrePes` | `PhoenixSync/core/SobrePes.h/.cpp` | `abrir(ruta)` descifra; `datos()` da los bytes; `guardarComo(ruta, datos)` cifra con la envoltura original (falla si el archivo ya existe y verifica releyendo); `ponerTextoInfo(texto)` cambia el texto de «Cargar». |
| `GuardadoLM` | `PhoenixSync/core/LigaMaster.h/.cpp` | Plantillas, equipo del usuario, `tablasDe(k)` (localiza las tablas alineadas, incluida la D de 368 B). |
| `palanca` | `PhoenixSync/herramientas/palanca/palanca.cpp` | Cambia **un** u16: `palanca <ML_original> <ML_nuevo> <ofsHex> <valorViejo> <valorNuevo> [textoInfo]`. Se niega si el valor actual ≠ viejo. |
| `palanca_varias` | `PhoenixSync/herramientas/palanca/palanca_varias.cpp` | Varios cambios, todo o nada: `palanca_varias <ML_original> <ML_nuevo> "texto" ofsHex:viejo:nuevo [ofsHex:viejo:nuevo ...]`. Rechaza valores > 9.999, y si **cualquier** valor viejo no coincide, no escribe nada. |

**Compilar** (Linux o similar, necesita C++20 porque `Tipos.h` usa `std::u8string`):

```bash
# Desde la carpeta PhoenixSync/
SP=<carpeta de salida>
gcc -c -O2 -I. terceros/pesxcrypter/crypt.c       -o $SP/crypt.o
gcc -c -O2 -I. terceros/pesxcrypter/masterkey.c   -o $SP/masterkey.o
gcc -c -O2 -I. terceros/pesxcrypter/mt19937ar.c   -o $SP/mt19937ar.o
g++ -std=c++20 -O2 -I. -c herramientas/palanca/palanca_varias.cpp -o $SP/palanca_varias.o
g++ -std=c++20 -O2 -I. -c core/SobrePes.cpp                        -o $SP/SobrePes.o
g++ -std=c++20 -O2 -I. -c core/Sha256.cpp                           -o $SP/Sha256.o
g++ $SP/palanca_varias.o $SP/SobrePes.o $SP/Sha256.o \
    $SP/crypt.o $SP/masterkey.o $SP/mt19937ar.o -o $SP/palanca_varias
```

**Nota:** los archivos `.c` de `terceros/pesxcrypter/` son C puro y hay que compilarlos con `gcc`, no con `g++` (si se compilan con `g++` da errores de conversión de tipos, porque g++ los trata como C++ estricto).

#### 6.2 · Algoritmo

```
ENTRADA: ruta del ML original, pid del jugador, lista de (barra k, valor nuevo) a aplicar, ruta de salida (no existe)

1. s = SobrePes::abrir(ML)                         // falla → parar
2. d = s.datos()                                   // bytes descifrados
3. g = GuardadoLM::desdeDatos(d)
   u = índice del equipo del usuario (marca −11)
   T = la tabla de g.tablasDe(u) con stride == 368
4. Recorrer registros r = 0,1,2…: ofs = T.ofsReg0 + 368·r
      si u32(d, ofs+4) == pid  → reg_ofs = ofs; parar     // u32 en ofs = reg, en ofs+4 = pid
   (no encontrado → parar: el jugador no es del equipo del usuario)
5. Para cada (barra k, valor nuevo):
      p = reg_ofs + 50 + 2·(k−1)
      viejo = u16(d, p)
      comprobar 0 ≤ viejo ≤ 9999                    // si no, parar: no es una barra válida
      comprobar 0 ≤ valor nuevo ≤ 9999
      escribir u16(d, p) = valor nuevo               // 9999 para subir, 0 para bajar
6. s.ponerTextoInfo("PRUEBA …")                     // visible en «Cargar»
7. s.guardarComo(salida, d)
8. VERIFICAR: abrir(salida) y comparar con el original:
      - mismo tamaño descifrado
      - bytes distintos == 2 × (número de barras cambiadas)
      - todos los distintos dentro de las posiciones p
9. Respaldo del original ANTES de copiar al PC; huella (md5/sha256) antes y después de copiar.
10. Si la entrega es por un puente de archivos remoto: comprimir en .zip, entregar el .zip,
    descomprimir en destino, comparar huella del archivo final contra la huella local. (Sección 8.)
```

#### 6.3 · Código de referencia (Python, sobre datos ya descifrados)

```python
import struct

TAM_REG = 368          # tamaño del registro de la tabla D
INI_BARRAS = 54        # las barras empiezan en el byte +54 del registro

MAPA = {
    1: "Actitud ofensiva", 2: "Actitud defensiva", 3: "Actitud de portero",
    4: "Regate", 5: "Control de balón", 6: "Conservación del balón",
    7: "Finalización", 8: "Pase raso", 9: "Pase bombeado", 10: "Cabeceo",
    11: "Recuperación de balón", 12: "Agresividad", 13: "Balón parado", 14: "Efecto",
    15: "Atajar (portero)", 16: "Despejar (portero)", 17: "Reflejos (portero)",
    18: "Cobertura (portero)", 19: "Precisión con el pie malo",
    20: "Velocidad", 21: "Contacto físico", 22: "Equilibrio", 23: "Potencia de tiro",
    24: "Aceleración", 25: "Salto", 26: "Resistencia", 27: "Regularidad",
    28: "Resistencia a lesiones",
    # 29, 30: sin usar, siempre 0
}

def barras(datos, inicio_registro):
    """Devuelve las 30 barras (u16) de un registro de la tabla D."""
    return [struct.unpack_from('<H', datos, inicio_registro + INI_BARRAS + 2*i)[0] for i in range(30)]

def poner_barra(datos: bytearray, inicio_registro, k, valor_viejo, valor_nuevo):
    """valor_nuevo = 9999 para pedir un +1; = 0 para pedir un -1."""
    assert 1 <= k <= 30 and 0 <= valor_nuevo <= 9999
    p = inicio_registro + INI_BARRAS + 2*(k-1)
    actual = struct.unpack_from('<H', datos, p)[0]
    if actual != valor_viejo:
        raise ValueError(f'barra {k}: hay {actual}, se esperaba {valor_viejo}; no se toca nada')
    struct.pack_into('<H', datos, p, valor_nuevo)

def vueltas_simples(antes, despues, umbral=3000):
    """Primera aproximación: barras que dieron la vuelta entre dos guardados.
    OJO: con muchos días de diferencia esto puede dar falsos positivos en barras
    de jóvenes que crecen rápido sin haber completado ninguna vuelta real.
    Usar solo como primer filtro; confirmar siempre contra la pantalla."""
    arriba = [k+1 for k in range(30) if despues[k] < antes[k] - umbral]
    abajo  = [k+1 for k in range(30) if despues[k] > antes[k] + umbral]
    return arriba, abajo
```

#### 6.4 · Comprobación con el juego (obligatoria)

Un cambio **no** está probado hasta que se ve en pantalla. Hay que dejar anotados:
- valor antes (captura);
- valor después de avanzar (captura);
- guardado del después en ranura nueva;
- lectura de las barras del después, para comprobar que dieron la vuelta las esperadas **y cruzarlo con la pantalla**, no solo con el umbral numérico (sección 4.4).

---

### 7. Métodos para mapear varias barras en una sola prueba

#### 7.1 · El «código secreto» (para muchas barras con muchos jugadores)

**Problema.** Para saber qué habilidad es cada barra, lo obvio es llenar una barra, avanzar y mirar qué sube. Con 20 barras serían 20 pruebas.

**Idea.** Usar **varios jugadores a la vez** y darle a cada barra una «huella» distinta: el conjunto de jugadores en los que se llena.

**Diseño usado (20 habilidades de campo, guía 25):**
- **7 jugadores jóvenes** que crecen rápido: Bardghji, Caicedo, Gavi, Fermín, Lamine, Bernal, Cubarsí.
- A cada barra se le asignó un **trío distinto** de los 7 jugadores. Hay C(7,3) = 35 tríos posibles, más que suficientes para 20.
- Restricciones: no darle a un jugador una barra cuya habilidad ya está en 99; repartir para que cada jugador tenga 8 o 9 barras.

**Por qué tríos (3 de 7) y no parejas:** si un jugador no sube una habilidad (tope de 99) o sube una de más (crecimiento natural), el conjunto observado tendrá 2 o 4 jugadores, y eso se nota: no es un trío válido. Con tríos, un error no se confunde con otra barra.

**Decodificación:**
1. Para cada habilidad, anotar en qué jugadores subió (comparando capturas antes y después).
2. Si el conjunto es un trío del código, esa es su barra.
3. Si sale un conjunto con un jugador de más, mirar en el **guardado del después** qué barras dieron la vuelta **solas** (no puestas por nosotros) en ese jugador, y descontarlo.
4. Si quedan empates (dos habilidades con el mismo conjunto observado), desempatar con el jugador que tenga **solo una** de las dos barras.

#### 7.2 · El «cruce de dos jugadores» (para pocas barras con pocos jugadores)

**Problema.** Cuando solo quedan 1 o 2 barras por resolver y solo hay 1 o 2 jugadores del tipo adecuado (por ejemplo, solo 2 porteros en la plantilla), el código secreto de tríos no se puede armar.

**Idea, usada en el experimento 6:** con solo 2 jugadores A y B y 2 barras X e Y por resolver:
- A: sube X (a 9.999), baja Y (a 0).
- B: baja X (a 0), sube Y (a 9.999).

Después de avanzar, la habilidad que **sube en A y baja en B** es X; la que **baja en A y sube en B** es Y. Esto confirma cada barra **dos veces en el mismo paso** (una vez por cada dirección), lo que es más fuerte que solo subir o solo bajar en un único jugador.

**Si una dirección no llega a completarse** (el avance fue corto y la barra no llegó a dar la vuelta), y ya no queda ningún otro candidato posible para esa habilidad entre las que faltan, se puede asignar **por descarte lógico** — como pasó con la barra 18 = Cobertura en el experimento 6: no se la vio moverse directamente, pero no quedaba ninguna otra habilidad de portero sin barra.

**Generalización:** este método funciona con cualquier número de barras desconocidas N, siempre que haya al menos 2 jugadores del tipo adecuado: basta con que cada barra tenga una combinación distinta de «sube en estos jugadores, baja en estos otros» (como el código secreto, pero con dos direcciones en vez de una sola presencia/ausencia). Con 2 jugadores y 2 direcciones por jugador, se pueden distinguir hasta 4 combinaciones (subir, bajar, subir+subir no tiene sentido en el mismo jugador... en la práctica, con 2 jugadores se resuelven cómodamente 2 barras por ronda; para más barras con pocos jugadores, se necesitarían más rondas o más jugadores).

---

### 8. Lista de comprobación de 0 errores

Antes de tocar nada:
- [ ] El juego está **cerrado**.
- [ ] `git pull --rebase` (dos cuentas trabajan en el repo).
- [ ] La ranura de destino está **libre**, o el usuario pidió expresamente usar una ocupada (entonces, respaldo).
- [ ] **Respaldo** del original con nombre nuevo, y huella comprobada igual.

Al preparar:
- [ ] El jugador es del **equipo del usuario** y su registro está en la tabla D (reg y pid coinciden).
- [ ] Cada valor viejo está entre 0 y 9.999, y coincide con lo que se esperaba.
- [ ] Ningún valor nuevo pasa de 9.999 (y no hay valores negativos: el mínimo es 0).
- [ ] El texto de «Cargar» deja claro que es una **prueba**.

Después de preparar:
- [ ] Al abrir otra vez el archivo nuevo: mismo tamaño y **solo** los bytes planeados distintos.
- [ ] Huella del archivo en el **contenedor/computadora de trabajo** anotada, antes de entregarlo.

**⚠️ Al entregar el archivo al PC por un puente de archivos remoto:**
- [ ] **Si el archivo no tiene extensión** (como todos los `MLxxxxxxxx` de guardado), **comprimirlo en `.zip` antes de mandarlo**. Mandarlo directo puede dañarlo sin que la herramienta avise de ningún error (visto el 10 de octubre de 2026: un archivo de 19.831.781 bytes llegó con solo 19.813.994, le faltaban 17.787 bytes, probablemente por una normalización de saltos de línea que el puente aplica a archivos que detecta como texto).
- [ ] Descomprimir el `.zip` ya en el PC.
- [ ] Comprobar **tamaño y huella** del archivo final en el PC contra el de origen. No basta con que la herramienta de copiar no haya mostrado error.

En el juego:
- [ ] La ranura carga sin errores.
- [ ] Capturas **antes**, avanzar, capturas **después**, guardar en una **ranura nueva**.
- [ ] El archivo del después: dieron la vuelta las barras esperadas (comparado con la pantalla, no solo con el umbral numérico — ver sección 4.4 si pasaron varios días).
- [ ] Pantalla y archivo cuadran 1 a 1.

Al terminar:
- [ ] PRUEBAS.md (al final, sin borrar nada), REGISTRO.md (una línea) y, si cambia la estructura, ESTRUCTURA-ML.md.
- [ ] Commit y push a `mercado-fase0`.
- [ ] Guía nueva (número libre) y copia en Drive como documento **nuevo**, y en el Proyecto de Claude.

---

### 9. Multiparche 🧩

| Qué | ¿Cambia con el parche? | Explicación |
|---|---|---|
| Que la carrera copie las habilidades al crearse | No | Es del juego. |
| Que existan las barras, y las reglas de +1 (vuelta arriba) y −1 (vuelta abajo) | No | Es del motor del juego. |
| El **mapa** (qué barra es qué habilidad) | No (casi seguro) | Es del motor del juego, no de la base. Conviene repetir **una** palanca de control en cada parche nuevo (por ejemplo, barra 6 → Conservación) para confirmarlo. |
| El registro de 368 B y las barras en +54 | No (casi seguro) | Es el formato del guardado de PES 2021. Comprobar con la búsqueda de 4.3. |
| **Dónde** está la tabla D dentro del guardado | **Sí** | Depende de la carrera, del parche y del tamaño de otras partes. Usar siempre la búsqueda de 4.3, nunca posiciones fijas. |
| El `reg` de cada jugador (prefijo) | **Sí** | Cambia por carrera. Compararlo tal cual. |
| Los `pid` de los jugadores | **Sí**, según el parche | Cada parche puede numerar distinto (jugadores añadidos). |
| El sobre del guardado | No | Es del juego (libpesXcrypter). |
| Cuánto crece (o decrece) cada jugador por día | **Sí** | Depende de la edad y el potencial que trae la base del parche, y de cuánto se juega. |

**Riesgo en parches que modifican el guardado o el exe:** si un parche cambiara el formato de la Liga Máster (raro), la búsqueda de 4.3 **no encontraría** la tabla y la herramienta **no tocaría nada**. Es el comportamiento seguro.

**Probado:** solo en ConmeGOL 26, carrera del FC Barcelona de FRALEX. Pendiente repetir en Sudamerican y otros (guía 12).

---

### 10. Riesgos, límites y cosas que NO hacer

- **No** escribir valores fuera de 0–9.999 en una barra.
- **No** sobrescribir la ranura original sin respaldo comprobado.
- **No** subir guardados (`ML…`, `.bin`) a GitHub.
- **No** tocar nada con el juego abierto.
- **No** mandar un archivo sin extensión (como un `ML…`) directo por un puente de archivos remoto sin comprimirlo antes (ver sección 8).
- **No** confiar en un chequeo automático simple («el número cambió mucho») para decidir si una barra dio la vuelta, cuando pasaron varios días entre las dos lecturas: siempre cruzar con la pantalla.
- **Límite:** +1 o −1 por barra y por avance. Para +N o −N hay que repetirlo N veces, avanzando entre medias. ⏳ Sin probar si hay atajo.
- **Límite:** solo jugadores del **equipo del usuario**. Los demás equipos no tienen tabla D conocida. ⏳
- **Efecto secundario:** muchas subidas a la vez suben la **media** y pueden cambiar el valor de mercado y el sueldo. En una liga competitiva hay que poner reglas (por ejemplo, máximo de subidas por jornada).
- **Juego limpio:** lo que hace la palanca es adelantar (o retrasar) el crecimiento que el propio juego aplica. Conviene que la web registre cada uso, para que sea transparente entre los participantes.

---

### 11. Lo que falta descubrir y cómo hacerlo

| Qué | Cómo |
|---|---|
| **«Uso de pie malo»** (sin barra) | Ya no quedan barras libres (solo 29 y 30, siempre 0). Dos hipótesis: (a) sube por otro mecanismo del juego, no por barra — habría que buscarlo aparte, quizás comparando más guardados con mucha diferencia de tiempo y mirando si ese número cambia sin que ninguna barra conocida lo explique; (b) de verdad no sube de forma automática en este juego. |
| **+N o −N de golpe, o más rápido** | Probar si avanzar varios días seguidos con la barra recargada entre medias es lo único posible, o si hay un contador que limite y permita adelantarlo. |
| **Otros equipos** | Buscar si los jugadores de equipos que no son del usuario tienen barras en otro sitio del guardado (sin confirmar todavía). |
| **Valor absoluto** | Sigue sin localizarse dónde guarda la carrera el número «93» de cada habilidad en sí (aparte de la barra de progreso). Con el mapa de barras completo, una búsqueda nueva tiene muchas más pistas: comparar dos guardados donde una habilidad conocida subió o bajó 1, y buscar qué otro número del registro cambió exactamente en 1 al mismo tiempo. |
| **Confirmación a nivel de archivo de la ronda 2** | Falta que FRALEX guarde el «después» de la ronda 2 en una ranura nueva, para comparar también byte a byte (como se hizo con la ronda 1). No es urgente: la pantalla ya es suficiente evidencia. |
| **Conectar con la web** | Diseñar qué pide la web (jugador + habilidad + dirección), qué responde Phoenix Sync, y cómo se entrega el archivo modificado de vuelta al jugador, de forma segura y auditable. |

---

### 12. Archivos, herramientas y huellas

#### 12.1 · En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

| Ruta | Contenido |
|---|---|
| `PhoenixSync/core/SobrePes.*` | Abrir y cerrar el sobre del guardado |
| `PhoenixSync/core/LigaMaster.*` | Equipo del usuario, plantillas, `tablasDe` |
| `PhoenixSync/herramientas/palanca/palanca.cpp` | Cambiar 1 barra |
| `PhoenixSync/herramientas/palanca/palanca_varias.cpp` | Cambiar varias barras (todo o nada) |
| `PhoenixSync/herramientas/palanca/plan_mapeo_2026-10-10.json` | Plan exacto de la prueba de 60 barras (código secreto, campo) |
| `PhoenixSync/herramientas/palanca/antes_mapeo_2026-10-10.json`, `antes_despues_mapeo_2026-10-10.json` | Habilidades «antes» y «antes/después» de los 7 jóvenes |
| `PhoenixSync/herramientas/palanca/plan_porteros_lentas_2026-10-10.json` | Plan de la ronda 1 (porteros y lentas) |
| `PhoenixSync/herramientas/palanca/antes_porteros_lentas_2026-10-10.json` | Valores «antes» de la ronda 1 |
| `PhoenixSync/PRUEBAS.md` | Diario de pruebas, con **todos** los detalles paso a paso (10 oct, 00:50 en adelante) |
| `PhoenixSync/REGISTRO.md` | Una línea por cambio |
| `PhoenixSync/liga-master/ESTRUCTURA-ML.md` §25 | Formato técnico |
| `PhoenixSync/base-conocimiento/21, 23, 24, 25, 26, 28, 29` | Guías (esta, la 29, es la versión consolidada y final) |

#### 12.2 · En el PC de FRALEX

- **Guardados:** `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`.
- **Respaldos:** `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\respaldos_ranuras\`.

#### 12.3 · Huellas (md5/sha256) de todos los guardados usados

| Ranura | Archivo | Fecha en el juego | Qué es | Huella |
|---|---|---|---|---|
| 4 | ML00000003 | 1/1/2026 | Carrera nueva con la base v99 | md5 `6cbe94ff…` |
| 5 | ML00000004 | 1/1/2026 | Carrera nueva con la base v90 | md5 `006dd17a…` |
| 6 (original) | respaldo `ML00000005_ranura6_original_16-3-2026` | 16/3/2026 | La de la ranura 4, avanzada | md5 `1feef3a4…` |
| 6 (palanca) | ML00000005 | 16/3/2026 | Barra 6 de Lamine = 9.999 | md5 `f220841e…` |
| 7 | ML00000006 | 11/4/2026 | Después de la palanca | md5 `af613ecd…` |
| 8 | ML00000007 | 11/4/2026 | 60 barras en 9.999 (mapeo de campo) | md5 `ac573b6b…` |
| 9 | ML00000008 | 22/4/2026 | Después del mapeo de campo | md5 `e3eb8710…` |
| 10 | ML00000009 | 22/4/2026 | 7 barras cambiadas (ronda 1, porteros/lentas) | — |
| 11 | ML0000000A | 2/5/2026 | Después de la ronda 1 | sha256 `6cc3419a…` |
| 12 | ML0000000B | 2/5/2026 | Ronda 2 (Reflejos/Cobertura cruzadas) | sha256 `4a9fafbb…` |

---

### 13. Anexo: datos crudos de todas las pruebas

#### A.1 · Barras de Lamine (registro en `0xc51228`, carrera inicial) en tres fechas

| Barra | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1/1 (ranura 4) | 8432 | 2340 | 0 | 3308 | 6527 | 9765 | 1342 | 3487 | 1060 | 6598 | 1713 | 5701 | 4671 | 3280 | 8588 | 5874 | 4418 | 8727 | 2415 | 6666 | 8797 | 2707 | 5118 | 3815 |
| 16/3 (ranura 6) | 9969 | 3233 | 0 | 4134 | 7956 | 551 | 2555 | 5024 | 2273 | 7412 | 2322 | 6310 | 6208 | 4817 | 8691 | 5874 | 5631 | 9940 | 3775 | 7492 | 9690 | 3891 | 5118 | 3993 |
| 11/4 (ranura 7) | 595 | 3549 | 0 | 4520 | 8592 | 327 | 3026 | 5650 | 2744 | 7699 | 2542 | 6530 | 6834 | 5443 | 8752 | 5874 | 6102 | 411 | 4296 | 7878 | 6 | 4307 | 5118 | 4086 |

Barras 15–18 = 0 y 29–30 = 0 en las tres fechas (antes de la ronda 1, Lamine todavía no era portero obviamente, así que esas barras nunca se tocaron en él).

#### A.2 · Habilidades de los 7 jóvenes, antes (ranura 8) y después (ranura 9) — código secreto de campo

Formato de cada celda: antes → después. En **negrita**, las que subieron.

| Habilidad | Roony Bardghji | Josué Caicedo | Gavi | Fermín López | Lamine Yamal | Marc Bernal | Pau Cubarsí |
|---|---|---|---|---|---|---|---|
| **Media** | 78 → 78 | 58 → 59 | 83 → 83 | 86 → 88 | 90 → 90 | 78 → 80 | 87 → 88 |
| Actitud ofensiva | 70 | **76 → 77** | 72 | 78 | 84 | **70 → 71** | **70 → 71** |
| Control de balon | **82 → 83** | 56 | 85 | 88 | **89 → 90** | 73 | **74 → 75** |
| Regate | 90 | **63 → 64** | 86 | **85 → 86** | **93 → 94** | 73 | **70 → 71** |
| Conservacion del balon | 82 | 60 | 88 | **87 → 88** | 94 | **76 → 77** | **73 → 74** |
| Pase raso | **62 → 63** | 56 | 84 | **88 → 89** | **82 → 83** | **76 → 77** | 75 |
| Pase bombeado | **64 → 65** | 62 | 80 | 80 | 84 | **74 → 75** | **73 → 74** |
| Finalizacion | 74 | **60 → 61** | **69 → 70** | **82 → 83** | 81 | 66 | 60 |
| Cabeceo | **60 → 61** | 64 | 53 | 68 | **62 → 63** | **68 → 69** | 87 |
| Balon parado | 68 | 49 | **57 → 58** | 73 | **73 → 74** | 72 | **66 → 67** |
| Efecto | **77 → 78** | **44 → 45** | 75 | 76 | **85 → 86** | 72 | 70 |
| Velocidad | 88 | **77 → 78** | **76 → 77** | 84 | 99 | 76 | **80 → 81** |
| Aceleracion | 92 | 78 | **82 → 83** | 86 | **93 → 94** | **74 → 75** | **77 → 78** |
| Potencia de tiro | 72 | **65 → 66** | **69 → 70** | 83 | **78 → 79** | 77 | 81 |
| Salto | 63 | 76 | **67 → 68** | **65 → 66** | **72 → 73** | **73 → 74** | 87 |
| Contacto fisico | 61 | 63 | 67 | **66 → 67** | **76 → 77** | 83 | **85 → 86** |
| Equilibrio | 84 | 61 | **88 → 89** | **90 → 91** | 88 | **75 → 76** | 70 |
| Resistencia | **67 → 68** | **56 → 57** | **83 → 84** | 80 | 82 | 79 | 90 |
| Actitud defensiva | **53 → 54** | **40 → 41** | 64 | 63 | 55 | **75 → 76** | 90 |
| Recup. de balon | **49 → 50** | **64 → 65** | 70 | **65 → 66** | 55 | 78 | 89 |
| Agresividad | **51 → 52** | 50 | 85 | **71 → 72** | **67 → 68** | 77 | 89 |

(La tabla A.5 del informe 26 tiene, para referencia, el trío exacto de jugadores asignado a cada una de estas 20 barras, y la A.6 la posición exacta en hexadecimal de cada uno de los 60 cambios — se mantienen igual, no se repiten aquí para no alargar este documento; están en `base-conocimiento/26-INFORME-Y-GUIA-COMPLETA-BARRAS-DE-CRECIMIENTO.md`, secciones A.5 y A.6.)

#### A.3 · Ronda 1 (porteros y lentas) — plan exacto

Formato: `(índice en la plantilla, nombre, barra, posición-hex-del-reg-en-ranura10, valor viejo, valor nuevo)`.

```
(0, "Joan García",      3, 0xc5086e-50=..., 5925, 9999)
(0, "Joan García",     15, ...,              6325, 9999)
(11,"Wojciech Szczęsny",15, ...,             2372,    0)
(11,"Wojciech Szczęsny",16, ...,             8060,    0)
(7, "Lamine Yamal",    28, ...,              4164, 9999)
(9, "Fermín López",    27, ...,              4855, 9999)
(4, "Alejandro Balde", 19, ...,              2488, 9999)
```

(El archivo exacto, con los offsets completos en hexadecimal, es `plan_porteros_lentas_2026-10-10.json`.)

**Resultado en el archivo (ranura 11 contra ranura 10), las 7 barras plantadas:**

| Jugador | Barra | Antes (ranura 10) | Después (ranura 11) | ¿Dio la vuelta como se esperaba? |
|---|---|---|---|---|
| Joan García | 3 | 9999 | 728 | ✅ arriba |
| Joan García | 15 | 9999 | 532 | ✅ arriba |
| Szczęsny | 15 | 0 | 9833 | ✅ abajo |
| Szczęsny | 16 | 0 | 9833 | ✅ abajo |
| Lamine | 28 | 9999 | 23 | ✅ arriba |
| Fermín | 27 | 9999 | 73 | ✅ arriba |
| Balde | 19 | 9999 | 67 | ✅ arriba |

#### A.4 · Ronda 2 (Reflejos/Cobertura) — plan exacto y resultado

| Jugador | Barra | Offset (hex, datos descifrados) | Antes | Cambio pedido | Pantalla antes | Pantalla después |
|---|---|---|---|---|---|---|
| Joan García | 17 | `0xc5086e` | 7307 | → 9999 (sube) | Reflejos 92 | Reflejos **93** |
| Joan García | 18 | `0xc50870` | 5220 | → 0 (baja) | Cobertura 91 | Cobertura 91 (sin cambio esta vez) |
| Szczęsny | 17 | `0xc5183e` | 5311 | → 0 (baja) | Reflejos 87 | Reflejos **86** |
| Szczęsny | 18 | `0xc51840` | 7717 | → 9999 (sube) | Cobertura 87 | Cobertura 87 (sin cambio esta vez) |

Verificado: solo 8 bytes del archivo cambiaron entre el original y el preparado para la ranura 12 (los 4 números u16, 2 bytes cada uno). sha256 del archivo final = `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375`.

#### A.5 · El aviso del puente de archivos, con números exactos

- Archivo local (correcto), tamaño: **19.831.781 bytes**, sha256 `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375`.
- Mismo archivo, después de mandarlo **directo** (sin comprimir) por el puente al PC: **19.813.994 bytes** (faltaban 17.787 bytes), sha256 distinto. Sin ningún mensaje de error de la herramienta de copiado.
- El mismo archivo comprimido en `.zip` (19.834.976 bytes) viajó **exacto**, sha256 idéntico en origen y destino.
- Descomprimido ya en el PC: **19.831.781 bytes**, sha256 `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375` — igual al original. ✅
- **Conclusión práctica:** cualquier archivo sin extensión (que un sistema pueda confundir con texto) debe viajar siempre dentro de un `.zip` por este puente, nunca directo.

---
