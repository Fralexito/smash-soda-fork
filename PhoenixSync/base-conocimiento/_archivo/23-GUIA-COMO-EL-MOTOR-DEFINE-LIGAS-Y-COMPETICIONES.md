# 23 · Guía: cómo el motor de PES 2021 define ligas y competiciones, y cómo se podría añadir más

**Para FRALEX · 10 de octubre de 2026 · Solo lectura: no se cambió nada del juego.**
Continúa el informe 21. Aquí está lo que se encontró al mirar DENTRO del .exe, los módulos Lua de tu parche y los del Sudamerican.
Etiquetas: [OBSERVADO] visto en archivos/código · [PROBADO] funciona en el juego (lo hacen otros parches) · [FALTA] sin comprobar · [ESTIMACIÓN] juicio mío.

---

## 1. Lo más importante en 8 líneas

1. **El número de equipos de un torneo está escrito en el código del .exe**, no solo en la base de datos. [OBSERVADO] Para los torneos de selecciones hay una lista fija: Mundial 32, Euro 24, Copa América 12, Copa Asia 16, Copa África 14. Para todo lo demás, el número sale de un dato del juego (un byte con tope de 127).
2. **Eso se puede cambiar con parches de memoria desde Sider.** El módulo del Mundial de 48 equipos (Gogosz) lo hace: cambia 24 a 48 en el código y reescribe las tablas de grupos y partidos. [OBSERVADO en el código de los módulos]
3. El truco de Gogosz: **usar el espacio de la Euro (ID 33) para alojar el Mundial de 48** y dejar el Mundial (ID 27) con 24. [OBSERVADO]
4. Tu parche trae esos módulos (`FWC_Grupos`, `FWC_Expansion`, `FWC_Calendario`, `FWC_Llaves`) pero **solo los enciende en la variante «Clásicos (No LM)»**. «No LM» = no sirve para Liga Máster. Es una pista fuerte de que tocar las tablas rompe la Liga Máster. [OBSERVADO]
5. Los espacios de competición se reutilizan: el original de Konami tiene 91 y ConmeGOL 90. Solo hay 4 IDs realmente nuevos (145, 197, 198, 199). [OBSERVADO]
6. Dos parches independientes (ConmeGOL y Sudamerican) ya usan ligas de **30 equipos** con el mismo .exe. Sudamerican además tiene una Copa Argentina de 60 y una 2.ª división de 32. [OBSERVADO]
7. El tope probable de «127» sugiere que **no hay un límite duro en 30**. El límite real estaría en el calendario de la Liga Máster y la 2.ª temporada. [ESTIMACIÓN]
8. Nadie ha demostrado una liga nueva en un ID nuevo jugable en Liga Máster. Es la gran oportunidad, y también el mayor riesgo.

## 2. Qué se encontró en el .exe [OBSERVADO]

Archivo: `PES2021.exe` (458.806.784 B, el mismo en ConmeGOL y Sudamerican). El código está en la sección `.trace` (en este archivo la sección de código se llama así). Para pasar de «posición en el archivo» a «dirección en memoria»: dirección = 0x140000000 + posición + 0x400.

### 2.1 La función que fija el número de equipos
En la posición 0x12F0230 (dirección 0x1412F0630) hay una cadena de comparaciones sobre un campo que contiene el **ID de competición** (campo en `[rbx+0x1787B58]`). Según el ID, escribe el número de equipos en `[rbx+0x1787AB0]`:

| ID de competición | Nombre en el original | Equipos escritos por el código |
|---|---|---|
| 27 (0x1B) | Mundial | 32 |
| 33 (0x21) | Euro | 24 |
| 34 (0x22) | Copa América | 12 |
| 35 (0x23) | Copa Asia | 16 |
| 36 (0x24) | Copa África | 14 |
| 37 (0x25) | FAKE_KONAMI | 32 |
| 5 (0x05) | Copa Libertadores (original) | 38 |
| cualquier otro | — | `byte en [competición+0x30A] AND 0x7F` (máximo 127) |

Segundo sitio, en 0xABB7CB: si el modo es «liga» (valor 1) pone **20** equipos; si no, **32** (parece ser la liga/copa que uno crea en el juego).

### 2.2 El módulo Gogosz (Mundial de 48)
Lo traen Sudamerican WC26 (`GogoszPatch.lua`, `_1`, `_2`, `_3`) y ConmeGOL (`FWC_*.lua`). Comprobado: `FWC_Expansion.lua` de ConmeGOL tiene los mismos parches que `GogoszPatch_1.lua` (incluso el texto «GOGOSZ_PATCH» en sus mensajes); `FWC_Grupos.lua` trae las mismas tablas de grupos. De `FWC_Calendario` y `FWC_Llaves` se supone lo mismo pero no se abrieron. Hace cuatro cosas:
- **`_1` (parches directos al código):** cambia el 24 de la Euro por 48 y el 32 del Mundial por 24; intercambia listas de etapas («EUROSWAP»).
- **`GogoszPatch.lua` (grupos):** busca en memoria las tablas de grupos (12 grupos, 196 bytes cada uno, identificadores de etapa `29 04`, `29 08`… `29 30`) y escribe en ellas los 48 equipos.
- **`_2` (partidos):** reescribe los registros de partido (596 bytes cada uno) para tener los partidos de grupos y la ronda de 32.
- **`_3` (llaves):** reescribe las etapas de eliminación, con 32 equipos de partida, usando «anclas» (secuencias de bytes únicas para encontrar la tabla en memoria).
- Se busca la memoria por patrón en rangos `0x7FF4D1000000` en adelante, y se escribe dentro de un tiempo corto después de cargar el torneo.

**Lectura:** las estructuras de un torneo (grupos, partidos, etapas) están en memoria normal del proceso. Un módulo Lua puede **encontrarlas y reescribirlas**. No hace falta reescribir el .exe.

### 2.3 El módulo Competition-Server [OBSERVADO]
Modifica una tabla de **registros de 264 bytes por competición** que está en memoria (se encuentra buscando una firma). Campos que deja cambiar: tipo de competición (`Division1`, `Division2`, `Cup`, `Knockout`, `Split`, `SuperCup`, `UEL`, `LIB`, `CC_FCWC`), cuántos suben y bajan (0 a 4), a qué copa clasifica, comentarista y música. No crea registros nuevos: modifica los que ya existen. En el archivo `.exe` la tabla solo trae la primera fila; **el resto se llena al arrancar**.

### 2.4 Los IDs «de torneo» que ven los módulos [OBSERVADO]
Los módulos Lua reciben un `tournament_id` que combina el número de grupo y la etapa: `(grupo << 10) | etapa`. Ejemplo: 1065 = grupo 1 de la etapa 0x29 (41); 2089 = grupo 2 de la misma etapa. Por eso en el `sider.log` aparecen números como 1065, 2089, 5161, 6185. Es distinto del tercer número de `CompetitionEntry.bin`, que es `(puesto << 8) | ID de competición`.

## 3. Lo que dice la base de datos

- **Base original (dt10_x64.cpk):** 91 competiciones. Incluye 3 «ligas falsas» de Konami (Europa, Sudamérica, Asia, IDs 39, 40, 41) con sus copas y supercopas (42 a 44, 82, 84). [OBSERVADO]
- **ConmeGOL 26:** 90. Reutiliza esos mismos espacios. **Sudamerican:** 80, y su edición Mundial 2026 (`SP_Subs_WC.cpk`): 81 y solo 103 equipos (solo selecciones). [OBSERVADO]
- En la edición Mundial de Sudamerican, la **Euro (ID 33) tiene 48 equipos** y el Mundial (ID 27) 43: es el truco del punto 1.3 del informe. [OBSERVADO]
- `CompetitionRegulation.bin` guarda **nombres en 20 idiomas**, no formatos. [OBSERVADO]
- Un módulo de Sudamerican, `Custom Euro and AFC CL.lua`, **cambia el tournament_id en el momento** para que una competición use el marcador y la presentación de otra. [OBSERVADO]

## 4. Qué significa para añadir ligas y competiciones

| Idea | Qué se sabe | Posibilidad (ESTIMACIÓN) |
|---|---|---|
| Liga más grande que 30 (por ejemplo 32 a 36) | El número sale de un dato con tope 127; hay 2 parches con 30 | 50% a 65% |
| Liga de 37 a 40 | Un parche habla de «límite duro de 39» en Francia | 30% a 45% |
| Más de 40 | Sin evidencia | 10% a 20% |
| Liga en un ID nuevo, en exhibición | Los 4 IDs nuevos arrancan, pero no son ligas | 30% a 45% |
| Liga en un ID nuevo, en Liga Máster | Los módulos de Gogosz se marcan «No LM» | 10% a 20% |
| Torneo de formato nuevo (grupos + eliminatoria distintos) | Gogosz lo logró para el Mundial usando memoria | 35% a 50% para selecciones; 10% a 25% para clubes |
| Más de 4 ascensos/descensos | El módulo solo ofrece 0 a 4 | Sin datos |

## 5. Cosas interesantes que salieron de este análisis

1. El **Mundial de 48** de ConmeGOL está separado en otra variante porque **rompe la Liga Máster**.
2. Konami dejó **3 ligas «falsas»** (Europa, Sudamérica, Asia) con sus copas. Todos los parches las reutilizan para ligas reales.
3. **Sudamerican y ConmeGOL usan exactamente el mismo .exe.** Todo lo que los diferencia son datos y módulos Lua.
4. ConmeGOL llevó la 1.ª de Argentina de 24 (original) a 30; Sudamerican también a 30; la MLS pasó de 16 (en el espacio de China) a 30.
5. El Mundial puede usar el espacio de la Euro. **Se pueden «prestar» espacios entre competiciones.**
6. En el `sider.log` de ConmeGOL aparecen asignaciones de estadios «duplicadas» (por ejemplo para los grupos del Mundial 2026): el módulo de estadios pisa lo que ya había puesto.

## 6. Plan de pruebas por etapas (NO ejecutado; cada paso con permiso de FRALEX)

**Etapa A, solo mirar (riesgo cero):**
- A1. Un módulo Sider de solo lectura que escriba en el log, al abrir el menú de Liga Máster, el byte `competición+0x30A` y el número de equipos de cada liga. Así se confirma que el número sale de los datos.
- A2. En el juego, ver si «Clásicos», «Leyendas» y «Matchday» aparecen en Exhibición.

**Etapa B, base de datos de prueba (carpeta aparte, con respaldo):**
- B1. Liga de 30 a 32 equipos (plan del informe 19).
- B2. Una copa nueva en el ID 146 con 16 equipos (nombre en 20 idiomas), en Exhibición.
- B3. Una liga nueva en el ID 142 con 16 equipos, en Exhibición.

**Etapa C, Liga Máster (solo si B pasó):**
- C1. Liga Máster nueva con la liga de 32. Contar partidos (496).
- C2. Pasar a la 2.ª temporada.
- C3. Probar la liga nueva del ID 142 en Liga Máster.

**Regla de parada:** si el juego se cierra, faltan o sobran partidos, o falla la 2.ª temporada, volver al respaldo y anotar dónde falló.

## 7. Cómo repetir lo que se hizo (herramientas)
- Python 3 (`zlib`, `struct`, `mmap`) para abrir los `.bin` WESYS y buscar patrones en el .exe.
- `capstone` (pip) para desensamblar el código del .exe.
- Escaneo de `.cpk` buscando la marca `ff 10 81 'WESYS'`.
- Lectura de módulos Lua (algunos son bytecode de LuaJIT: se leen las cadenas de texto y los números).

## 8. Lo que NO se sabe
- Si el calendario de una liga de más de 30 equipos funciona en la 2.ª temporada.
- Qué dato exacto guarda `competición+0x30A` y quién lo llena.
- Si una liga nueva puede ser elegida en el menú de Liga Máster (la lista de ligas del menú podría estar fija).
- Por qué el calendario de FRALEX marca la Premier como competición 9 (anomalía del informe 21).
