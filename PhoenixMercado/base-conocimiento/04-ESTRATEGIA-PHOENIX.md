# 04 · Informe estratégico: ConmeGOL y Sudamerican por dentro, y cómo superarlos

Fecha: 8 oct 2026. Basado en el análisis de las dos instalaciones de la PC de Fralex (bases de datos extraídas y comparadas, los dos
Sider auditados, estructura de carpetas, logs) y en la investigación de `03-INVESTIGACION-PARCHES.md`.
Leyenda de viabilidad: 🟢 **ya se puede** (con lo que dominamos) · 🟡 **se puede con investigación** (camino conocido) · 🔴 **frontera** (nadie lo ha hecho; puede no salir).

---

## 1. Los dos parches por dentro

### 1.1 Base de datos (lo que define «el mundo»)

| | ConmeGOL 26 | Sudamerican Project 2026 |
|---|---|---|
| Archivo base | `CGP_database.cpk` (3,7 MB) | `SP_Subs.cpk` (3,9 MB) + `SP_Subs_WC.cpk` (Mundial, no cargado) |
| Competiciones | **90** | **80** |
| Equipos | **749** (techo del juego: 750) | **726** |
| Jugadores | **29.997** (techo: 30.000) | **29.323** |
| Asignaciones a plantillas | 20.398 | 17.214 |
| Archivos «4» (variantes de Konami) | no | sí (`Team4`, `Player4`, `Competition4`…) |
| Idioma del option file | español | portugués |
| Equipos con el mismo ID en los dos | 467 (274 con el mismo nombre) | |

**Lo que trae cada uno y el otro no:**
- **Solo ConmeGOL:** Libertadores, Sudamericana y Recopa con nombre CONMEBOL; ligas de **Venezuela, Uruguay (copa y 2.ª), Paraguay
  (copa), Costa Rica, Honduras, MLS y Arabia**; 2.ª de Colombia y Ecuador; «Clásicos» y «Leyendas».
- **Solo Sudamerican:** **Alemania** y **Países Bajos** completas, Europa League y Supercopa de Europa, 2.ª de Inglaterra, Supercopa de
  España, copas de Ecuador/Paraguay/Uruguay/México/EE. UU., y **2.ª de Perú con 14 equipos** (en ConmeGOL está vacía).

**Perú (lo que más nos importa):**
- **Liga 1 2026 (18 equipos):** los dos la tienen igual: Alianza Atlético, Alianza Lima, Atlético Grau, FC Cajamarca, Cienciano,
  Comerciantes Unidos, Cusco FC, Garcilaso, Moquegua, Juan Pablo II, Los Chankas, Melgar, Sport Boys, Sport Huancayo, Sporting Cristal,
  ADT, UTC y Universitario.
- **Copa (32):** los dos la tienen (con 2.ª división incluida).
- **Liga 2:** solo Sudamerican (Cantolao, ADA Jaén, Alianza Universidad, Ayacucho, Bentín Tacna, Mannucci, Comerciantes FC, Estudiantes
  CNI, San Marcos, Piratas, Santos Nazca, Unión Minas, César Vallejo, San Martín).
- **Supercopa:** ConmeGOL Universitario–Alianza; Sudamerican Universitario–Cusco.

**Lección:** los dos van **pegados al techo** del juego (750 equipos, 30.000 jugadores). Cada liga nueva obliga a **sacar** otra. Phoenix
tiene que **elegir** qué ligas trae (calidad antes que cantidad) y dejar espacio libre para crecer.

### 1.2 Cómo entregan el contenido

| | ConmeGOL | Sudamerican |
|---|---|---|
| Modelo | **Casi todo suelto por Sider** (`livecpk` + `content`) | **Clásico de CPK**: 34 CPK en DpFileList |
| Caras | por Sider | **13 CPK de 5–11 GB** cada uno |
| Estadios | `stadium-server`: ~9 GB en más de 11.000 archivos | `stadium-server`: 313 carpetas |
| Kits | `kit-server` por país | `kit-server`: 36 países, 157 equipos |
| Balones | `ball-server` (34) | `ball-server` (**1.582**) |
| Otros pesados | Turfs 2,4 GB, narración de estadio 1,8 GB, gráficos «next gen» 1,7 GB, cuerpos PRDX ~1 GB (6.000+ archivos) | narradores en CPK (Bambino Pons, Solabarrieta, Miguel Simón) y en Sider (Closs, Palma, Barril, Tapia, Romano, Vignolo) |
| Archivos diminutos | `slotkits`: ~9.900 archivos casi vacíos | — |
| Sider | 7.3.3, 49 módulos activos | **7.3.4**, 56 módulos (normal) / 66 líneas (Mundial) |
| Lanzador | switcher que **copia** carpetas encima | 7 `.bat` (uno por letra de disco) |

*(El recuento de archivos de ConmeGOL es parcial: la carpeta es tan grande que leerla entera por el puente tarda horas; las cifras de
arriba son las ya contadas.)*

### 1.3 Sus estrategias para ser estables (lo que hacen BIEN)

1. **Base de datos pequeña y aparte** (3,7–3,9 MB): la parte delicada cabe en un archivo; el resto es decorado.
2. **Sider como capa de contenido** (sobre todo ConmeGOL): actualizar = copiar carpetas, sin reempaquetar CPK gigantes.
3. **Variantes por switcher** (ConmeGOL: normal / Clásicos / Perú-Chile-Uruguay B; Sudamerican: normal / Mundial): cada modo tiene su
   propia base y su propio `sider.ini`, en vez de meter todo junto.
4. **Módulos de la comunidad muy probados** (kserv, StadiumServer, BallServer…): no reinventan lo básico.
5. **Comprobaciones de identidad**: los módulos CGP solo se activan si encuentran ciertos equipos en ciertas competiciones (un «candado»
   para no funcionar con otra base).
6. **Comunidad y créditos** (Sudamerican: más de 30 colaboradores, Discord): mantener un parche es trabajo de equipo.

### 1.4 Sus fallos (lo que hacen MAL, comprobado en tu PC)

| Fallo | ConmeGOL | Sudamerican | Consecuencia |
|---|---|---|---|
| `camera.lua` + `DynamicWideCam.lua` parchean la **misma instrucción** | sí (log) | sí (log, mismas direcciones) | Escriben encima de datos del juego; cierres raros |
| Escrituras a ciegas (`GFX_lod`) | sí | sí | Ídem |
| Módulos que **no existen** en `sider.ini` | 11 | 3 | Ruido; señal de desorden |
| Módulos que **fallan al arrancar** | — | 3 (`randomMenu`, `TurfLoader`, `LogoHD`) | Funciones que el usuario cree tener y no tiene |
| Módulos **cargados dos veces** | — | `MenuServer`; en el Mundial `env`, `etrace`, `camera`, `netblock` | Doble parcheo |
| Módulos de **prueba** activos (`env`, `etrace`, `jittest`, `zlibtest`) | `env` | varios | Logs gigantes, más carga |
| Módulos que no hacen nada / de otra versión (Chatty para FL26, scoreboard EPL, Anti-Cheat placebo, goalscreams) | sí | parcial | Falsa sensación de función |
| `netblock.lua` | — | sí | Bloquea cualquier online |
| Contenido duplicado entre modos (cada variante copia todo) | sí (~234 GB el pack completo) | sí (dos Sider casi iguales) | Pesado, lento de actualizar |
| CPK de caras de 5–11 GB | — | sí | Actualizar una cara = bajar 10 GB |
| Miles de archivos diminutos (`slotkits`, cuerpos PRDX) | sí | — | Cada archivo cuesta una búsqueda de Sider |
| Instalación por `.bat` con letra de disco | — | sí | Frágil; el antivirus desconfía |
| Lanzador que **pisa** carpetas sin avisar | sí | — | Perdemos cambios (nos pasó con `phoenix.lua`) |
| Copias del juego no originales | sí | sí | Sin PESBUL; riesgo de cargas infinitas |

---

## 2. Cómo superarlos en estabilidad (🟢 todo esto ya se puede)

1. **«Sider limpio» certificado:** cada módulo que entre al parche pasa la auditoría (memoria, archivos, teclas, choques). Prohibido: dos
   módulos que toquen lo mismo, escrituras sin comprobar, módulos de prueba, duplicados.
2. **«Phoenix Doctor»:** la auditoría que hicimos a mano, convertida en **herramienta automática** dentro del lanzador: lee `sider.ini` y
   `sider.log`, detecta módulos que faltan, duplicados, fallos de arranque y choques conocidos, y propone la corrección. *(Ningún parche
   tiene esto.)*
3. **Lanzador único** (Phoenix Link): sin `.bat`; comprueba versión del juego, Sider, addons, archivo de paginación y espacio; **nunca pisa
   sin copia**; muestra qué cambió.
4. **Una base, un `sider.ini` por modo**, generados por el programa (no a mano) y con su huella SHA-256 comprobada en cada arranque.
5. **Pruebas automáticas** antes de publicar (ya tenemos 271 para el Mercado) + la lista de pruebas en el juego de `PRUEBAS.md`.
6. **Reglas de base de datos:** no cambiar el tamaño de las ligas sin parche del exe; dejar huecos libres de IDs; usar IDs reales de
   Konami (los facepacks encajan solos).

## 3. Cómo superarlos en rendimiento (🟢/🟡)

1. **Menos archivos, más grandes** (🟢): juntar los miles de archivos diminutos (`slotkits`, cuerpos) en pocos paquetes; cada archivo
   suelto es una búsqueda de Sider por cada carga.
2. **Contenido solo de lo que juegas** (🟢): addons por liga (si no juegas la MLS, no cargas sus caras ni estadios).
3. **Texturas bien comprimidas** (🟢): DDS con compresión adecuada y tamaños razonables (no 4K donde no se ve) → menos VRAM, menos
   archivo de paginación, menos pantallas negras.
4. **Módulos ligeros** (🟢): nada que trabaje en cada fotograma sin necesidad; nada que mire el disco por cada archivo (como hace
   `real-coach-team`); un solo módulo Phoenix en vez de diez pequeños.
5. **Caché de búsquedas de Sider** bien ajustada (🟡): `lookup-cache`, `cache.size` y el orden de las raíces `cpk.root` (las pequeñas y
   específicas primero).
6. **Medición** (🟡): un módulo que mida tiempos de carga y FPS y los anote, para comparar versiones con números y no a ojo.

## 4. Funciones nuevas: hasta dónde podemos llegar

### 🟢 Ya se puede (con lo que ya probamos)

| Función | Qué es | Por qué nadie la tiene |
|---|---|---|
| **Mercado conectado a la web** | Fichajes reales de la liga (web firmada) aplicados al option file y a la Liga Máster | Ya funciona (pruebas 14–19); los parches solo traen plantillas fijas |
| **Liga Máster «viva»** | La web fija presupuestos, mete fichajes o ventas en tu carrera entre partidos | Nadie había descifrado la Liga Máster (no hay editor público) |
| **Option file por sala** | Phoenix Link carga el option file de la liga/sala elegida automáticamente | Los parches usan un único EDIT para todo |
| **Avisos en el juego** | Mensajes de la web dentro del juego (puente `avisos.txt`) | Prueba 1 hecha, falta probarla en el juego |
| **Resultado automático** | Módulo que lee el marcador (`match.stats()`) y lo deja en un archivo para que Phoenix Link lo suba | Las ligas actuales usan capturas a mano |
| **Phoenix Doctor** | Diagnóstico automático del Sider y del juego | Ver §2 |
| **Verificador del anfitrión** | Antes de una partida de liga, se comprueban las huellas del parche y del option file | Base: el verificador que ya planeabas |
| **Partidos «de liga» en amistoso** | Hacer que un amistoso se vea como partido oficial (menú, marcador, intro de la liga Phoenix), como hace `CustomExhibitionMatch.lua` | Existe para Euro/Asia; nadie lo usa para una liga propia |

### 🟡 Se puede con investigación (el camino es conocido)

| Función | Qué hace falta |
|---|---|
| **Estadísticas completas a la web** (goleadores, tarjetas, posesión) | `match.stats()` no las da: leerlas de la memoria (tabla de Cheat Engine de xAranaktu como punto de partida) |
| **Cambios en vivo dentro de la Liga Máster** (sin salir del juego) | Mapa de la Liga Máster en la memoria (presupuestos ya localizados por xAranaktu) |
| **Fabricar nuestros propios CPK** | Portar el formato de `pes-file-tools` a C++ |
| **Competiciones propias** (Liga Phoenix, copas de la web) | Descifrar `Competition*`/`CompetitionRegulation` comparando antes/después (como hicimos con la Liga Máster) |
| **Calendario de la Liga Máster desde la web** | Encontrar dónde se guarda el calendario (pista: los 10 IDs de partido por jornada) |
| **Actualizaciones por partes** (solo lo que cambió) | Paquetes pequeños + manifiesto con huellas (la web ya publica el option file así) |
| **Ligas de más de 20 equipos o formatos nuevos** | Parches del exe localizados por patrón (como hace FL) |
| **Informe de errores** | Que el lanzador, con permiso, mande `sider.log` cuando el juego se cierre, para arreglar fallos de todos |

### 🔴 Frontera (nadie lo ha hecho; puede no salir)

| Idea | Por qué es difícil |
|---|---|
| **Liga Máster online asíncrona**: varios usuarios, cada uno con su club en su propia carrera, compartiendo un mismo mercado y una misma tabla en la web | Hay que sincronizar las carreras (resultados de la IA, calendario, fichajes) sin un servidor dentro del juego. Con lo que ya sabemos de la Liga Máster es **la idea más original y la más cercana**. |
| **Estado del mundo real en el juego**: tabla, lesiones o forma de la Liga 1 real aplicadas cada semana | Fuentes de datos, y escribir forma o estado en la base o en la memoria |
| **Servidor propio estilo PESBUL** | Protocolo y login de Steam; meses o años de trabajo; PESBUL aún no tiene 11 contra 11 |
| **Espectadores en vivo**: marcador y datos del partido en la web mientras se juega | Leer memoria en vivo + subir datos sin afectar al juego |
| **Árbitro/VAR o reglas propias de la liga** | Lógica del juego dentro del exe: requiere parches de código |

---

## 5. Hasta dónde se puede «jugar» con el motor (límites honestos)

- **Archivos de la carpeta `save`** (option file, Liga Máster): **control casi total** — ya lo demostramos.
- **Base de datos (`pesdb`)**: control total en cuanto descifremos los archivos que faltan; límites de 750 equipos y 30.000 jugadores.
- **Contenido (caras, kits, estadios, menús, sonidos)**: control total vía Sider/CPK.
- **Memoria del juego en vivo (Sider + Lua)**: se puede leer y escribir; el riesgo crece con cada escritura → solo con patrón + verificación + pruebas.
- **Reglas «cableadas» en el exe** (calendarios, tamaños de liga, IA, físicas): solo con parches de código, que se rompen si cambia el exe.
- **Red dentro del juego**: Sider no trae red; **todo lo online pasa por Phoenix Link** (fuera del juego). Es una limitación y a la vez
  una ventaja: el juego no se expone a Internet.

## 6. Ruta completa propuesta

| Fase | Qué | Entregable |
|---|---|---|
| **0. Terreno** | Instalación limpia de referencia (Steam, 1.07.02 + DP 7.0, Sider fijado) + partido de referencia del Sider actual | Base de pruebas confiable |
| **1. Núcleo** | Base de datos Phoenix (partir de lo mejor de cada parche: Liga 1 + **Liga 2 de Sudamerican** + Copa, con permiso de sus autores) · option file oficial de la web · `sider.ini` limpio | Versión 0.1 (jugable, pequeña, estable) |
| **2. Herramientas** | Fabricador de CPK y DpFileList en Phoenix Mercado · Phoenix Doctor · lanzador | Instalar/actualizar sin `.bat` |
| **3. Conexión** | Avisos (prueba 1) → resultado automático → verificador del anfitrión → option file por sala | Primeras partidas de liga «online» con amigos |
| **4. Contenido** | Addons por liga (caras, kits, estadios) con versión; permisos y créditos | Parche completo, modular |
| **5. Más allá** | Competiciones propias, estadísticas completas, Liga Máster viva, y la gran apuesta: **Liga Máster online asíncrona** | Lo que nadie tiene |

**Créditos y permisos:** casi todo el contenido visual de los dos parches es obra de terceros (facemakers, kitmakers, Olmos Jr 23,
Afandix…). Para publicarlo dentro de Phoenix hace falta su permiso y su crédito. Lo propio de Phoenix (base de datos, herramientas,
conexión con la web) es 100 % de Fralex.

---

## 7. Proyección a futuro: cuando los parches se actualicen (y lleguen otros)

Se esperan pronto: **ConmeGOL** (la versión instalada es la **26**, base del 19/8/2026; ya existe la **27**, sept. 2026) y
**Sudamerican** (en una semana). Más adelante, **FL27** (inicios de noviembre) y otros. La idea: que cada actualización **no rompa
nada nuestro** y que además **nos enseñe algo**.

### 7.1 Lo que suele cambiar en una actualización de parche

| Qué cambia | Cómo afecta a Phoenix |
|---|---|
| Base de datos (fichajes, equipos nuevos, IDs que se mueven, competiciones nuevas) | El **catálogo** del Mercado queda viejo → regenerarlo y subirlo a la web; revisar si cambian IDs de equipos/jugadores |
| Option file nuevo | Las plantillas cambian; el option file oficial de la web tiene que rehacerse sobre la nueva base |
| Módulos Sider nuevos o actualizados, `sider.ini` nuevo | **Volver a auditar** (choques, escrituras a ciegas); el switcher puede pisar lo que hayamos instalado |
| Carreras de Liga Máster | Muchos parches piden **empezar una carrera nueva** tras actualizar la base; las carreras viejas pueden dar plantillas raras |
| Estructura (más CPK, nuevas carpetas livecpk) | Revisar DpFileList (orden, cantidad) y espacio en disco |

**Lo que NO debería romperse:** Phoenix Mercado localiza todo **por anclas** (busca la forma de los datos, no posiciones fijas) y valida
antes de escribir; si algo cambió de forma, **se frena** en vez de estropear el archivo. Esa decisión de diseño es justo para este momento.

### 7.2 Huella del «antes» (hecha hoy, 8 oct 2026, 21:15)

Guardada en la PC en `_PhoenixMercado_prueba\_versiones\2026-10-08\` (y la lista en `datos/huella-2026-10-08.txt`):
- ConmeGOL: `CGP_database.cpk` (md5 d2d62b4d…, 3.699.240 B, 19/8/2026), `DpFileList.bin`, `sider.ini` (md5 80ad499f…), option file
  (md5 027b0fa2…), lista de módulos y CPK con tamaño y fecha.
- Sudamerican: `SP_Subs.cpk` (11/5/2026), `SP_Subs_WC.cpk`, `DpFileList.bin`, los dos `sider.ini`, option file (md5 6c612a4c…).
- El mismo `PES2021.exe` en los dos (md5 d4760962…).
Con esto, después de actualizar se puede decir **exactamente** qué cambió.

### 7.3 Protocolo del «día de actualización» (para mañana y para siempre)

**Antes** (Fralex):
1. Copiar la carpeta `save` del parche (option file y carreras) a un lugar seguro.
2. Avisar: «voy a actualizar ConmeGOL / Sudamerican».

**Después** (Claude, con la PC conectada):
1. Nueva huella y **comparación** con la anterior: qué archivos cambiaron.
2. **Base de datos:** equipos que entran/salen, IDs que cambian, competiciones nuevas, jugadores movidos (con atención a Perú).
3. **Sider:** módulos nuevos o cambiados → auditoría solo de esos; comprobar si siguen los choques de cámaras, etc.
4. **Option file:** comparar plantillas.
5. **Mercado:** regenerar el catálogo, correr las pruebas (`PROBAR2.bat`), subir el catálogo nuevo a la web.
6. **Liga Máster:** probar que el programa lee una carrera nueva de la versión nueva (y decidir qué pasa con las viejas).
7. Anotar todo en `parches/<parche>.md` como **«changelog técnico»** (lo que el autor no cuenta), en `PRUEBAS.md` y en Drive.

### 7.4 Automatizarlo: el «Auditor de parches» (fase 2)

Lo que hoy hacemos a mano se convierte en un comando de Phoenix Mercado:
- `auditar <carpeta del juego>` → huella completa (base, DpFileList, Sider, option file), comparación con la última, informe en Markdown.
- Comparador de bases de datos (equipos, competiciones, plantillas) y **tabla de equivalencias de IDs entre versiones** (reutilizando el
  emparejamiento ConmeGOL ↔ Sudamerican que ya existe).
- Auditoría automática de `sider.ini` y módulos (Phoenix Doctor).
- Un **registro de parches** (nombre, versión, carpeta del juego, carpeta `save`, número de Steam) para que el Mercado y Phoenix Link
  sepan siempre con qué parche y qué versión trabajan, y no mezclen archivos (ConmeGOL y Sudamerican comparten la carpeta de documentos).

### 7.5 Parches futuros (FL27, Gogosz, nuevas versiones)

Cada parche nuevo que entre a la PC recibe la misma ficha: identificación, base de datos (comparada con las demás), contenido, Sider
auditado, estrategias, fallos, y qué copiar o evitar. Con varias fichas se ve **la tendencia** (qué hace la comunidad cada temporada)
y Phoenix puede adelantarse.

### 7.6 El propio Phoenix, preparado para su futuro

- **Versiones con número claro** y cada actualización marcada: «no afecta a tu carrera» / «necesita carrera nueva» / «se aplica la
  próxima temporada» (como FL).
- **Huella publicada** de cada archivo (como el option file oficial de la web) y lanzador que comprueba y repara.
- **Las carreras y los datos de la web sobreviven** a las actualizaciones: la web guarda la verdad (fichajes, presupuestos) y el
  Mercado puede **reaplicarla** sobre una base nueva.
