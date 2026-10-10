# 29 · Informe maestro: subir y bajar habilidades en una Liga Máster ya empezada (barras de crecimiento) — versión completa y final

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

## Índice

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

## 0. Resumen en una página

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

## 1. Glosario: cada palabra explicada

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

## 2. La historia completa: los 6 experimentos

### Experimento 1 · ¿Llegan las stats de Phoenix-DB a una Liga Máster? (9 oct, guías 18 y 21)

1. Pusimos a Lamine con Velocidad **99** en `Player.bin` (Phoenix-DB). En la base original tiene 90.
2. En un amistoso salió con **99**. ✅ En la carrera del Barça ya empezada salió con **90**. ❌
3. Creamos **dos carreras nuevas** iguales:
   - ranura 4, con la base 99 → Lamine sale con 99;
   - ranura 5, con la base 90 → Lamine sale con 90.
4. Prueba decisiva: con la base en **90**, cargamos la ranura 4 → Lamine sigue con **99**. ✅

**Conclusión:** la carrera **copia** las habilidades al crearse y desde ahí usa su copia. ✅

Buscamos el número «99» de esa copia dentro del guardado de muchas formas (literal, empaquetado en 6, 7 u 8 bits, en orden y sin orden). **No apareció**. El valor absoluto de las habilidades sigue sin localizar. ⏳

### Experimento 2 · Descubrir las barras (10 oct, 00:50; guía 23)

1. Se compararon **la misma carrera en dos fechas**: ranura 4 (1/1/2026) y ranura 6 (16/3/2026). Así hay mucho menos «ruido» que entre dos carreras distintas: 191.549 bytes distintos, frente a 1,77 millones.
2. En el registro de 368 B de Lamine había unos **30 números entre 0 y 9.999** que habían **subido casi todos**.
3. Uno, el n.º 6, pasó de **9.765 a 551**: había llegado al tope y había empezado de nuevo.
4. **Predicción**, hecha antes de mirar la pantalla: «en la ranura 6, Lamine tendrá exactamente una habilidad con +1».
5. **Pantalla:** Conservación del balón **92 → 93**, y todo lo demás igual. ✅

→ Son barras de crecimiento, y la **barra 6 = Conservación del balón**.

### Experimento 3 · La palanca (10 oct, 01:05–01:11; guía 24)

1. Respaldo de la ranura 6.
2. En una copia, la barra 6 de Lamine pasó de **551 a 9.999**. Solo cambiaron 2 bytes. Esa copia quedó en la ranura 6.
3. FRALEX la cargó: Conservación **93** (no cambia todavía, porque no ha avanzado nada).
4. Avanzó varios días: Conservación **94**. ✅
5. En la ranura 7 (11/4/2026), guardada después, el archivo mostró que a Lamine le dieron la vuelta las barras **1, 6, 22 y 25**. En pantalla le subieron **4** habilidades: Actitud ofensiva, Conservación, Equilibrio y Salto. ✅ Cuadra.

### Experimento 4 · El mapa de las 20 habilidades de campo (10 oct, 01:20–01:50; guía 25)

1. Desde la ranura 7 se hizo la **ranura 8**, con **60 barras en 9.999** repartidas entre 7 jugadores jóvenes. Es el «código secreto» de la sección 7.1.
2. FRALEX sacó capturas **antes**, avanzó unos 11 días, sacó capturas **después**, y guardó en la **ranura 9**.
3. **Archivo (ranura 9):** las 60 barras dieron la vuelta, **60 de 60**. Además, 4 dieron la vuelta solas por crecimiento normal.
4. **Pantalla:** a cada jugador le subieron **exactamente** las habilidades que predicen sus barras: **64 de 64**. Comprobado también con un programa.
5. Con el trío de jugadores de cada habilidad se dedujo su barra. Dos parejas empatadas (8/12 y 13/24) se separaron con Marc Bernal, que tenía naturalmente una de las dos barras de cada pareja pero no la otra.

**Resultado:** las 20 barras de campo quedaron mapeadas (1, 2, 4–14, 20–26).

### Experimento 5 · Porteros y barras lentas, ronda 1 (10 oct, 09:05–09:30; `PRUEBAS.md`)

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

### Experimento 6 · Porteros, ronda 2 (10 oct, 09:36–09:37; `PRUEBAS.md`)

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

## 3. Resultado final: el mapa completo de barras y las reglas

### 3.1 · El mapa — 28 de 30 barras (COMPLETO)

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

### 3.2 · Reglas comprobadas

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

## 4. Especificación técnica: dónde y cómo están las barras en el archivo

### 4.1 · Cadena completa

```
ML0000000n (disco, cifrado)
  └─ SobrePes::abrir()          → datos descifrados (~19,8 MB)   [guía 10]
       └─ equipo del usuario    → bloque de equipo con la marca −11 (kMarcaUsuario) en kOfsIdOption
            └─ plantilla        → lista de (reg, pid) de sus jugadores
                 └─ Tabla D     → 1 registro de 368 B por jugador, mismo orden que la plantilla
                      └─ barras → 30 × u16 desde el byte +54 del registro
```

### 4.2 · Formato de un registro de la tabla D (368 bytes)

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

### 4.3 · Cómo encontrar la tabla D en CUALQUIER guardado (sin posiciones fijas)

El método general ya está programado en `GuardadoLM::tablasDe(k)` (`PhoenixSync/core/LigaMaster.cpp`):

1. Busca el equipo del usuario: el bloque de equipo que lleva la marca **−11** (`kMarcaUsuario`).
2. Lee su plantilla: la lista de pares `(reg, pid)`.
3. Después de los bloques de equipo (`kFinBloques`), busca el par `(reg, pid)` del **primer** jugador (8 bytes seguidos: reg y luego pid).
4. En cada sitio donde aparezca, comprueba que los **4 jugadores siguientes** de la plantilla están a distancias de **368 bytes** (admite huecos vacíos: `pid == 0` y `reg` en 0, 0xffff o vacío).
5. Si cuadra, es la tabla D. Así obtienes `ofsReg0` = posición del reg del registro 0.

**Comprobado varias veces** (incluida la ranura 9, 10 oct 02:05): `tablasDe` encuentra el equipo del usuario y su tabla D con `stride = 368` correctamente, sin usar ninguna posición fija de antemano. ✅

El registro del jugador número *i* de la plantilla empieza en `ofsReg0 − 4 + 368·i`. Puede haber huecos: lo seguro es recorrer los registros y quedarse con el que tenga el `reg`/`pid` buscado.

**Cuidado:** el `reg` cambia de prefijo según la carrera. No hay que suponer el prefijo, solo compararlo tal cual con el de la plantilla.

### 4.4 · Comportamiento de una barra

- Valor: u16 de **0 a 9.999**. Nunca escribir ≥ 10.000.
- Al avanzar el tiempo, el juego suma (jóvenes) o resta (veteranos) un poco cada día y con los partidos.
- **Dar la vuelta hacia arriba:** valor viejo alto → valor nuevo bajo → habilidad **+1**.
- **Dar la vuelta hacia abajo:** valor viejo bajo (puesto en 0) → valor nuevo alto (cerca de 9.999) → habilidad **−1**. ✅
- **Detectarlo en un programa, con cuidado:** una primera aproximación es `nuevo < viejo − 3000` (vuelta hacia arriba) o `nuevo > viejo + 3000` (vuelta hacia abajo). **Esta aproximación falla si pasó mucho tiempo (varios días) entre las dos lecturas**, porque el crecimiento natural de una barra joven puede superar los 3.000 puntos sin haber completado ninguna vuelta. La forma segura de confirmar una vuelta es siempre cruzar con el cambio visto en pantalla (la habilidad correspondiente subió o bajó exactamente 1).
- Si la habilidad está en **99**, la barra no avanza.

---

## 5. Guía de réplica, paso a paso (para humanos)

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

## 6. Guía de réplica, paso a paso (para una IA o un programa)

### 6.1 · Herramientas que ya existen

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

### 6.2 · Algoritmo

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

### 6.3 · Código de referencia (Python, sobre datos ya descifrados)

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

### 6.4 · Comprobación con el juego (obligatoria)

Un cambio **no** está probado hasta que se ve en pantalla. Hay que dejar anotados:
- valor antes (captura);
- valor después de avanzar (captura);
- guardado del después en ranura nueva;
- lectura de las barras del después, para comprobar que dieron la vuelta las esperadas **y cruzarlo con la pantalla**, no solo con el umbral numérico (sección 4.4).

---

## 7. Métodos para mapear varias barras en una sola prueba

### 7.1 · El «código secreto» (para muchas barras con muchos jugadores)

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

### 7.2 · El «cruce de dos jugadores» (para pocas barras con pocos jugadores)

**Problema.** Cuando solo quedan 1 o 2 barras por resolver y solo hay 1 o 2 jugadores del tipo adecuado (por ejemplo, solo 2 porteros en la plantilla), el código secreto de tríos no se puede armar.

**Idea, usada en el experimento 6:** con solo 2 jugadores A y B y 2 barras X e Y por resolver:
- A: sube X (a 9.999), baja Y (a 0).
- B: baja X (a 0), sube Y (a 9.999).

Después de avanzar, la habilidad que **sube en A y baja en B** es X; la que **baja en A y sube en B** es Y. Esto confirma cada barra **dos veces en el mismo paso** (una vez por cada dirección), lo que es más fuerte que solo subir o solo bajar en un único jugador.

**Si una dirección no llega a completarse** (el avance fue corto y la barra no llegó a dar la vuelta), y ya no queda ningún otro candidato posible para esa habilidad entre las que faltan, se puede asignar **por descarte lógico** — como pasó con la barra 18 = Cobertura en el experimento 6: no se la vio moverse directamente, pero no quedaba ninguna otra habilidad de portero sin barra.

**Generalización:** este método funciona con cualquier número de barras desconocidas N, siempre que haya al menos 2 jugadores del tipo adecuado: basta con que cada barra tenga una combinación distinta de «sube en estos jugadores, baja en estos otros» (como el código secreto, pero con dos direcciones en vez de una sola presencia/ausencia). Con 2 jugadores y 2 direcciones por jugador, se pueden distinguir hasta 4 combinaciones (subir, bajar, subir+subir no tiene sentido en el mismo jugador... en la práctica, con 2 jugadores se resuelven cómodamente 2 barras por ronda; para más barras con pocos jugadores, se necesitarían más rondas o más jugadores).

---

## 8. Lista de comprobación de 0 errores

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

## 9. Multiparche 🧩

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

## 10. Riesgos, límites y cosas que NO hacer

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

## 11. Lo que falta descubrir y cómo hacerlo

| Qué | Cómo |
|---|---|
| **«Uso de pie malo»** (sin barra) | Ya no quedan barras libres (solo 29 y 30, siempre 0). Dos hipótesis: (a) sube por otro mecanismo del juego, no por barra — habría que buscarlo aparte, quizás comparando más guardados con mucha diferencia de tiempo y mirando si ese número cambia sin que ninguna barra conocida lo explique; (b) de verdad no sube de forma automática en este juego. |
| **+N o −N de golpe, o más rápido** | Probar si avanzar varios días seguidos con la barra recargada entre medias es lo único posible, o si hay un contador que limite y permita adelantarlo. |
| **Otros equipos** | Buscar si los jugadores de equipos que no son del usuario tienen barras en otro sitio del guardado (sin confirmar todavía). |
| **Valor absoluto** | Sigue sin localizarse dónde guarda la carrera el número «93» de cada habilidad en sí (aparte de la barra de progreso). Con el mapa de barras completo, una búsqueda nueva tiene muchas más pistas: comparar dos guardados donde una habilidad conocida subió o bajó 1, y buscar qué otro número del registro cambió exactamente en 1 al mismo tiempo. |
| **Confirmación a nivel de archivo de la ronda 2** | Falta que FRALEX guarde el «después» de la ronda 2 en una ranura nueva, para comparar también byte a byte (como se hizo con la ronda 1). No es urgente: la pantalla ya es suficiente evidencia. |
| **Conectar con la web** | Diseñar qué pide la web (jugador + habilidad + dirección), qué responde Phoenix Sync, y cómo se entrega el archivo modificado de vuelta al jugador, de forma segura y auditable. |

---

## 12. Archivos, herramientas y huellas

### 12.1 · En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

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

### 12.2 · En el PC de FRALEX

- **Guardados:** `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`.
- **Respaldos:** `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\respaldos_ranuras\`.

### 12.3 · Huellas (md5/sha256) de todos los guardados usados

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

## 13. Anexo: datos crudos de todas las pruebas

### A.1 · Barras de Lamine (registro en `0xc51228`, carrera inicial) en tres fechas

| Barra | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1/1 (ranura 4) | 8432 | 2340 | 0 | 3308 | 6527 | 9765 | 1342 | 3487 | 1060 | 6598 | 1713 | 5701 | 4671 | 3280 | 8588 | 5874 | 4418 | 8727 | 2415 | 6666 | 8797 | 2707 | 5118 | 3815 |
| 16/3 (ranura 6) | 9969 | 3233 | 0 | 4134 | 7956 | 551 | 2555 | 5024 | 2273 | 7412 | 2322 | 6310 | 6208 | 4817 | 8691 | 5874 | 5631 | 9940 | 3775 | 7492 | 9690 | 3891 | 5118 | 3993 |
| 11/4 (ranura 7) | 595 | 3549 | 0 | 4520 | 8592 | 327 | 3026 | 5650 | 2744 | 7699 | 2542 | 6530 | 6834 | 5443 | 8752 | 5874 | 6102 | 411 | 4296 | 7878 | 6 | 4307 | 5118 | 4086 |

Barras 15–18 = 0 y 29–30 = 0 en las tres fechas (antes de la ronda 1, Lamine todavía no era portero obviamente, así que esas barras nunca se tocaron en él).

### A.2 · Habilidades de los 7 jóvenes, antes (ranura 8) y después (ranura 9) — código secreto de campo

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

### A.3 · Ronda 1 (porteros y lentas) — plan exacto

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

### A.4 · Ronda 2 (Reflejos/Cobertura) — plan exacto y resultado

| Jugador | Barra | Offset (hex, datos descifrados) | Antes | Cambio pedido | Pantalla antes | Pantalla después |
|---|---|---|---|---|---|---|
| Joan García | 17 | `0xc5086e` | 7307 | → 9999 (sube) | Reflejos 92 | Reflejos **93** |
| Joan García | 18 | `0xc50870` | 5220 | → 0 (baja) | Cobertura 91 | Cobertura 91 (sin cambio esta vez) |
| Szczęsny | 17 | `0xc5183e` | 5311 | → 0 (baja) | Reflejos 87 | Reflejos **86** |
| Szczęsny | 18 | `0xc51840` | 7717 | → 9999 (sube) | Cobertura 87 | Cobertura 87 (sin cambio esta vez) |

Verificado: solo 8 bytes del archivo cambiaron entre el original y el preparado para la ranura 12 (los 4 números u16, 2 bytes cada uno). sha256 del archivo final = `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375`.

### A.5 · El aviso del puente de archivos, con números exactos

- Archivo local (correcto), tamaño: **19.831.781 bytes**, sha256 `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375`.
- Mismo archivo, después de mandarlo **directo** (sin comprimir) por el puente al PC: **19.813.994 bytes** (faltaban 17.787 bytes), sha256 distinto. Sin ningún mensaje de error de la herramienta de copiado.
- El mismo archivo comprimido en `.zip` (19.834.976 bytes) viajó **exacto**, sha256 idéntico en origen y destino.
- Descomprimido ya en el PC: **19.831.781 bytes**, sha256 `4a9fafbb6b8f1df0d8eb13f550d57fef5573a5a797ec7c1fe9bf018be0d74375` — igual al original. ✅
- **Conclusión práctica:** cualquier archivo sin extensión (que un sistema pueda confundir con texto) debe viajar siempre dentro de un `.zip` por este puente, nunca directo.
