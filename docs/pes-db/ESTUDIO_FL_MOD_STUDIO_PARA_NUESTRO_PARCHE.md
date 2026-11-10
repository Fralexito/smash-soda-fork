# Estudio de FL Mod Studio: qué copiar a nuestro parche (como opción on/off)

Fecha: 10-nov-2026. Estado: ESTUDIO de solo lectura. Fuente: repositorio `Matozanato/FootballLife-new-leagues` (MIT), leído en el código y en `docs/` (README, `research/how-it-works.md`, `research/limits.md`, `research/player-record.md`, `developers/fork/world-file.md`, `developers/fork/code-map.md`). No se tocó nada en la PC. Ojo: el instalador no se pudo bajar (GitHub redirige el zip a un dominio bloqueado en la sesión), así que NADA de esto se ha probado en el juego. Añadir a CREDITOS.txt: Matozanato (FL Mod Studio, MIT).

## 1. Resultado corto

- FL Mod Studio NO sirve tal cual en el Sudamerican: sus módulos Sider verifican byte a byte `FL_2026.exe` (458.910.720 bytes, SHA `7c27ecb3…`). El exe de `D:\SP2026` (`PES2021.exe`, 458.806.784 bytes, SHA `5e27a78246dbe4f1e66cb60ec56e86a739303708b02d4048f6fec6b570bc224a`) es otro. Con otro exe los módulos se apagan solos y no cambian nada.
- Pero la herramienta es un mapa muy completo de CÓMO se agregan ligas, clubes y jugadores a un PES 2021 sin cambiar el exe, y buena parte se puede copiar. Hay 3 piezas, de más fácil a más difícil.

## 2. Piezas copiables

### A. Datos por `livecpk` (sin tocar el exe) — fácil, se puede probar ya

Una liga es una fila en cada una de tres tablas (`Competition.bin`, `CompetitionRegulation.bin`, `CompetitionEntry.bin`) y Sider las reemplaza en tiempo de ejecución con `livecpk`. Ya lo hacemos para otras cosas. Dentro de los límites de fábrica (750 clubes, 300 reglamentos, 100 competiciones) esto funciona sin parche de exe. Es la ruta mínima para «ligas nuevas» en Conmegol o en el Sudamerican.
- Herramienta a copiar: `tools/pesdb.py` (desempaca y empaca los `.bin` de la carpeta `common\etc\pesdb`).
- Opción on/off: un módulo/livecpk «Liga X» que se enciende o apaga en `sider.ini`.

### B. Mapa del registro de jugador (312 bytes) — fácil y muy valioso para la base propia

`docs/research/player-record.md` da el mapa de `Player.bin`: registro de 312 bytes, ID de jugador en +0x08 (32 bits), campos como bits (altura, peso, nacionalidad, estilo de juego, todos los atributos con su offset, ancho y suma), cuatro nombres de 61 bytes en +0x44. `tools/playeredit.py` lee y escribe 97 campos por nombre. Esto resuelve lo que `PLAN_BASE_DE_DATOS_PROPIA.md` §5 decía que nadie había mapeado. Falta comprobar que el formato es idéntico en el PES 2021 de Conmegol y del Sudamerican (el motor es el mismo; los bytes no los hemos medido nosotros).
- Prueba propuesta (solo lectura): abrir `Player.bin` de una COPIA con `pesdb.py` y comparar 5 jugadores conocidos contra lo que muestra el juego.
- Sirve directo para el comparador de parches (Studio, módulo 1): si leemos los `.bin` de Conmegol y Sudamerican con este mapa, comparamos por ID, nombre, nacionalidad y atributos y sacamos exclusivos.

### C. IDs sin preocuparse — idea clave, hay que verificarla

- Límite duro medido por ellos: el motor guarda el ID de jugador en 18 bits (máximo 262.143); el ID de equipo tiene el mismo manejo. Los nuevos jugadores reciben «el primer ID libre por encima del más alto» (`tools/lbplayers.py`) y se puede fijar el ID a mano (columnas Player ID, Team ID, League ID).
- Regla: el ID de reglamento decide si una liga se juega. Si el ID no está en una lista compilada en el exe, la liga existe pero nunca juega. Solución de ellos: parche «date spread» y `fl26join.dll` (ver D). Para ligas dentro del rango ya fechado no hace falta nada.
- Idea para nosotros: un archivo de texto de «mundo» (`fl26world.txt`, un hecho por línea, ver `docs/developers/fork/world-file.md`) que describe ligas, clubes y reglas, y que los módulos leen. Es el puente entre la herramienta y el juego, y es copiable como formato propio (IDs Phoenix en una tabla maestra, la herramienta decide los IDs libres).

### D. Ampliar los límites del exe — difícil (opus), el motor es el mismo pero los bytes no

`sider/fl26caps.lua` aplica 3.510 cambios de bytes: clubes 750→2.400, entrenadores 1.300→3.400, reglamentos 300→600, jugadores 30.001→61.905, partidos por temporada 13.000→65.000, competiciones 100→255, regiones de menú 29→128, slots de Select Team 124→250. Más módulos aparte: `fl26join.dll` (mete las ligas nuevas en la temporada y las cierra al final), 7+ «null guards» (evitan cuelgues con mundos grandes), módulos para menús y listas.
- Por qué NO se copia directo: sus direcciones y bytes son del exe de FL26.
- Por qué SÍ se puede portar: los parches NO están escritos a mano; los genera `tools/patchset.py` leyendo `patches/layout.json` (dónde está cada tabla) y `patches/attrib.json` (cada instrucción del exe que toca una tabla, hallada por desensamblado con capstone). Para PES2021.exe de CPY habría que volver a correr esa atribución sobre nuestro exe. Es trabajo de ingeniería inversa (Ghidra/REA), pero con receta.
- Ojo: nuestro exe pesa 104 KB menos que el de FL26; eso indica diferencias reales entre ambos, no solo la cabecera. Medirlo antes de prometer nada.
- Opción on/off: un único módulo Sider «Phoenix Caps» que verifica los bytes y se apaga solo si el exe no coincide (igual que ellos). Un exe distinto = no rompe nada.

## 3. Otras ideas útiles del repositorio

- `siderini.py`: lee y escribe `sider.ini` línea por línea, byte a byte, con orden de módulos. Base para el «Sider visual» pendiente.
- `backups.py` y `worlds.py`: puntos de restauración antes de cada cambio y «mundos» que se cambian con un clic (cada uno con sus propios guardados de Master League). Justo lo de Link/Studio.
- `installer.py`: instala un zip/7z/carpeta/.lua/.cpk y reparte cada parte donde Sider la quiere.
- NewLife Database: eliges una liga real y se arma con clubes y plantillas reales. Formato en `docs/developers/fork/newlife-format.md` (sha256 de `players.csv` y `clubs.csv`). Sus datos salen de una fuente externa; el formato sí se puede imitar para nuestra base.
- `kitpics.py`/`mkkits.py`: carpetas de kits desde imágenes; `lbstadiums.py`: estadios por Stadium Server y `map_teams.txt`.
- `checks.py` y diagnóstico: filas de salud (nivel, área, texto). Es el módulo 3 de Studio («Salud de la base»).
- 97 campos de jugador, formato CSV de importación de fichajes completos: copiable a la edición masiva de Studio.

## 4. Orden propuesto (todo con copias, nada sobre el PES base ni sobre tus parches)

1. Leer `Player.bin`/`Team.bin` de una copia de Conmegol y de Sudamerican con `pesdb.py` + el mapa de 312 bytes. Confirmar que el formato coincide. Producto: CSV de cada base y comparador (iguales, parecidos, exclusivos).
2. Agregar UNA liga de prueba por `livecpk` (parte A) sobre una copia del Conmegol, encendida/apagada desde sider.ini.
3. Solo si hace falta pasar de 750 clubes o 100 competiciones: portar los parches de límites (parte D) con Ghidra/REA sobre `PES2021.exe`. Aquí sí conviene Opus.
4. Después, formato propio de «mundo» + IDs Phoenix, y conectarlo con Studio/Link.

## 5. Lo que NO sabemos

- Si el formato de `Player.bin` del repositorio coincide con el de Conmegol y Sudamerican (probable, sin medir).
- Si el «port a PES 2021 vanilla» de papijonn existe como herramienta: en el repositorio solo aparece como crédito, sin código ni guía.
- Cómo se comporta el programa en el juego: no se pudo descargar ni probar.
- Si una liga nueva por solo `livecpk`, sin los parches de fechas, llega a jugarse en nuestro exe (la regla del ID de reglamento dice que depende del ID elegido).
