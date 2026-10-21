# 21 · Investigación: ¿se pueden añadir MÁS LIGAS y MÁS COMPETICIONES a PES 2021? (por separado)

**Para FRALEX · 10 de octubre de 2026 · Solo lectura: no se cambió ningún archivo del juego.**
Etiquetas: [PROBADO] visto funcionando · [OBSERVADO] visto en archivos · [FALTA] no comprobado · [ESTIMACIÓN] juicio mío, no medido.

---

## 0. Respuesta corta

- **Cambiar qué liga ocupa un espacio:** SÍ, ya está hecho en tu parche. [OBSERVADO]
- **Tener MÁS espacios de liga que los del juego original:** NADIE lo ha demostrado. Tu parche no lo hace: tiene 29 filas de liga, el original 30. [OBSERVADO]
- **Tener MÁS espacios de competición (copas, torneos):** tu parche añadió **4 filas nuevas** (IDs 145, 197, 198, 199) y el juego arranca con ellas. [OBSERVADO] Pero son de tipo «especial» (partido/exhibición), no ligas ni copas jugables. Que funcionen como torneo de verdad: [FALTA].
- **Inventar un FORMATO nuevo de torneo:** muy difícil sin tocar el .exe. [ESTIMACIÓN] menos de 10%.

## 1. Cómo se investigó (para repetirlo)

| Paso | Herramienta | Resultado |
|---|---|---|
| Abrir los .bin de tu parche | Python `zlib` (cabecera WESYS de 16 B + zlib) | 90 competiciones, 1.359 filas de equipos |
| Encontrar la base ORIGINAL de Konami | Escaneo de `Data\dt10_x64.cpk` buscando bloques WESYS | `Competition.bin` original = 91 filas; `CompetitionEntry.bin` = 1.291 filas; `CompetitionRegulation.bin` = 498.624 B |
| Comparar original contra ConmeGOL 26 | Python (`cmp.py`) | Tablas de abajo |
| Comparar con Sudamerican 2026 | `datos/competiciones-cgp-vs-sp.json` del repo | 80 filas; mismo mecanismo |
| Leer el módulo que modifica competiciones | `SiderAddons\modules\Competition-Server.lua` y `data.csv` | Tabla en memoria de registros de 264 B |
| Buscar esa tabla dentro del .exe | Búsqueda del patrón del módulo en `PES2021.exe` | Solo está la 1.ª fila; el resto se llena al arrancar (no se puede leer del archivo) |
| Web | Competition-Server V2.1, Custom Competition Style, SP Expansion, UML 2024, hilo de Evoweb | Ver sección 6 |

Aviso honesto: `dt10_x64.cpk` es la mejor «base original» que hay en tu carpeta, pero no tengo un disco de Konami para confirmar que sea 100% intacta.

## 2. LIGAS: lo que se vio

### 2.1 Los espacios se reutilizan [OBSERVADO]
Cada competición tiene un ID. El original y ConmeGOL usan **los mismos IDs**, pero con otro contenido:

| ID | Original (Konami) | ConmeGOL 26 |
|---|---|---|
| 9 | Inglaterra 1.ª | Argentina 1.ª |
| 10 | Italia 1.ª | Colombia 1.ª |
| 11 | España 1.ª | Brasil 1.ª |
| 21 | Brasil 1.ª | España 1.ª |
| 22 | Argentina 1.ª | Inglaterra 1.ª |
| 39 a 44 | «Liga falsa» Europa/Sudamérica/Asia | Chile 2.ª, Francia, Costa Rica, copas |
| 125 | China 1.ª | MLS |

- De los 86 IDs que comparten, **76 conservan el mismo código de región**. Cambia el país, no el «cajón».
- Sudamerican 2026 hace lo mismo con otros nombres (ID 2 = Champions en uno y Libertadores en el otro). [OBSERVADO]
- Conteo de filas de liga (sin playoffs): original **30**, ConmeGOL **29**. No hay aumento neto.

### 2.2 Dónde sí hubo «más» [OBSERVADO]
| Cosa | Original (máximo) | ConmeGOL 26 |
|---|---|---|
| Liga de 1.ª con más equipos | 24 (Argentina, Inglaterra 2.ª) | **30** (Argentina, MLS) |
| MLS (en el espacio de China) | 16 | 30 |
| Copa nacional | 44 | **66** (Copa Argentina) |
| Torneo continental | 45 (Europa League) | **47** (Libertadores) |

Esto responde en parte al informe 19: el motor ya ha aceptado ligas más grandes que las de Konami (no probado más allá de 30).

### 2.3 Segundas divisiones nuevas [OBSERVADO]
Tu parche tiene 2.ª división de Argentina (36), Brasil, Ecuador, Colombia, Chile, Perú y Uruguay. El original solo traía 2.ª de Inglaterra, España, Francia, Italia y Brasil. Se logra con el módulo **Competition-Server** (Gerlamp), que escribe en una tabla en memoria cuántos suben y bajan (máximo 4) y qué clasifica a qué copa. [OBSERVADO en el módulo y su `data.csv`]

### 2.4 Lo que NO se pudo comprobar [FALTA]
- Si existe una **liga nueva en un ID nuevo** (por ejemplo 142 o 146). Ningún parche revisado la tiene.
- Dónde está la tabla de «formato» de cada liga: se llena al arrancar el juego, no está en el archivo del .exe.
- Cuántos registros tiene esa tabla. El `data.csv` de tu parche llega hasta el registro 163 (México 2.ª), pero esa liga no está en `Competition.bin`, así que la tabla es más grande que la lista de competiciones.

## 3. COMPETICIONES: lo que se vio

### 3.1 Filas nuevas que sí añadió ConmeGOL [OBSERVADO]
| ID | Nombre | Tipo | Región | Equipos asignados |
|---|---|---|---|---|
| 145 | MATCHDAY_EFOOTBALL_OPEN | 99 | 0 (código nuevo) | 118 |
| 197 | CLASICO_EQUIPOS_CLUBES | 96 | 240 (código nuevo) | 0 |
| 198 | CLASICO_EQUIPOS_AMERICA | 96 | 240 | 0 |
| 199 | LEYENDAS_DEL_FUTBOL | 96 | 240 | 0 |

- El nombre «Leyendas» aparece 20 veces (20 idiomas) en `CompetitionRegulation.bin` de ConmeGOL y 0 en el original. [OBSERVADO] Eso confirma cómo se añade una competición: **fila en Competition.bin + nombre en 20 idiomas en CompetitionRegulation.bin (+ equipos en CompetitionEntry.bin)**.
- Faltan en ConmeGOL, respecto al original, 5 IDs: 42, 82, 118, 120, 121 (copas falsas, copa suiza, copa y supercopa turca).
- IDs libres entre 1 y 200: unos 100, por ejemplo 6, 7, 25, 26, 38, 48, 50 a 55, 130 a 136, 142 a 144, 146 a 196. [OBSERVADO]

### 3.2 `CompetitionRegulation.bin` NO guarda formatos [OBSERVADO]
Lo que contiene son los **nombres** de cada competición y etapa en 20 idiomas, con una cabecera de 20 bytes por registro. No se vio ahí el número de equipos ni el formato. (Corrige la pista del informe 19, que lo marcaba como el lugar probable del límite.)

### 3.3 Qué decide el formato [OBSERVADO en el módulo]
El módulo Competition-Server deja cambiar, en la tabla de memoria: tipo (`Division1`, `Division2`, `Cup`, `Knockout`, `Split`, `SuperCup`, `UEL`, `LIB`, `CC_FCWC`), cuántos suben/bajan (0 a 4), a qué copa clasifica, comentarista y música. No crea formatos nuevos: elige entre los que el .exe ya trae.

## 4. Posibilidades (estimación mía, no medida)

| Qué | Posibilidad | Por qué |
|---|---|---|
| Liga que ocupa un espacio existente | Ya funciona | Todos los parches |
| 2.ª división nueva con ascenso y descenso | Ya funciona | Tu parche tiene 7 |
| Liga en un ID nuevo, en partido de exhibición | 30% a 45% | Las 4 filas nuevas arrancan, pero no son ligas |
| Liga en un ID nuevo, jugable en Liga Máster | 10% a 20% | El calendario y la 2.ª temporada dependen del .exe |
| Torneo/copa nuevo con formato ya conocido (copa de eliminación) | 25% a 40% | Hay copas grandes (66) que funcionan en la base |
| Torneo con formato inventado (grupos distintos, etc.) | menos de 10% | Hay que modificar el .exe |

## 5. Anomalía abierta (importante) [FALTA]
En el guardado de Liga Máster de FRALEX, el calendario marca la competición **ID 9** como la Premier. En la base actual de ConmeGOL el ID 9 es **Argentina** y la Premier es el 22. En la base original el ID 9 sí era Inglaterra. Hipótesis A: el guardado se creó con otra base (marzo 2026). Hipótesis B: el calendario usa IDs internos del .exe, distintos de los de `Competition.bin`. Si B es cierta, **cambiar la base a mitad de carrera puede dañar la Liga Máster**; hay que resolverlo antes de añadir competiciones.

## 6. Fuentes web
- Competition-Server V2.1: https://www.pesmodding.com/2024/01/pes-2021-competition-server-v21-by.html (dice que añade 2.ª divisiones con ascenso y descenso; no habla de IDs libres ni de Liga Máster)
- Custom Competition Style: https://www.pesmodding.com/2023/12/pes-2021-custom-competition-style-for.html (cambia solo la presentación de un partido de exhibición; no crea competiciones)
- SP Expansion: https://www.pessmokepatch.com/p/spexpansion.html (22 ligas editables que ocupan las «ligas falsas»; mínimo 14 equipos, máximo 18)
- UML 2024: https://www.pesmodding.com/2023/11/pes-2021-uml-patch-2024-fully-updated.html (MLS, EuroLeague y Saudi Pro League; no explica cómo)
- Hilo de Evoweb: https://evoweb.uk/threads/create-a-new-league-in-pes-2021.86342/ (nadie reporta haberlo logrado; uno dice que «el editor de Ejogc permite añadir ligas» sin haberlo probado)

## 7. Pruebas seguras propuestas (NO ejecutadas; esperan aprobación)

1. **Prueba 0 (sin tocar nada):** en el juego, mirar si «Clásicos» y «Leyendas del fútbol» aparecen en Exhibición y si se pueden jugar.
2. **Prueba 1:** carpeta de prueba con una fila nueva ID 146 (copia de una copa nacional de eliminación), con su nombre en 20 idiomas y 16 equipos. Cargar con Phoenix-DB. Ver si aparece y si se juega una copa completa en Exhibición.
3. **Prueba 2:** igual con una liga nueva (ID 142, tipo 34, región nueva) de 16 equipos. Exhibición primero.
4. **Prueba 3:** solo si 1 y 2 funcionan: Liga Máster nueva con esa liga; pasar a la 2.ª temporada.
5. Antes de todo: resolver la anomalía de la sección 5 y hacer respaldo de `Database` y `SiderAddons`.
6. Parar si el juego se cierra, si falta el nombre de la competición, o si los partidos no cuadran.

## 8. Lo que NO se hizo
No se ejecutó el juego, no se modificó ningún archivo, no se descifró el calendario del .exe, no se probó ninguna competición nueva.
