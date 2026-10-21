# 26 · Informe y guía completa: subir habilidades en una Liga Máster ya empezada (barras de crecimiento)

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

## Índice

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

## 0. Resumen en una página

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

## 1. Glosario: cada palabra explicada

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

## 2. La historia completa: los 4 experimentos

### Experimento 1 · ¿Llegan las stats de Phoenix-DB a una Liga Máster? (9 oct, guías 18 y 21)

1. Pusimos a Lamine con Velocidad **99** en `Player.bin` (Phoenix-DB). En la base original tiene 90.
2. En un amistoso salió con **99**. ✅ En la carrera del Barça ya empezada salió con **90**. ❌
3. Creamos **dos carreras nuevas** iguales:
   - ranura 4, con la base 99 → Lamine sale con 99;
   - ranura 5, con la base 90 → Lamine sale con 90.
4. Prueba decisiva: con la base en **90**, cargamos la ranura 4 → Lamine sigue con **99**. ✅

**Conclusión:** la carrera **copia** las habilidades al crearse y desde ahí usa su copia. ✅

Buscamos el número «99» de esa copia dentro del guardado de muchas formas (literal, empaquetado en 6, 7 u 8 bits, en orden y sin orden). **No apareció**.
El valor absoluto de las habilidades sigue sin localizar. ⏳

### Experimento 2 · Descubrir las barras (10 oct, 00:50; guía 23)

1. Se compararon **la misma carrera en dos fechas**:
   - ranura 4, del 1/1/2026;
   - ranura 6, del 16/3/2026.

   Así hay mucho menos «ruido» que entre dos carreras distintas: 191.549 bytes distintos, frente a 1,77 millones.
2. En el registro de 368 B de Lamine había unos **30 números entre 0 y 9.999** que habían **subido casi todos**.
3. Uno, el n.º 6, pasó de **9.765 a 551**: había llegado al tope y había empezado de nuevo.
4. **Predicción**, hecha antes de mirar la pantalla: «en la ranura 6, Lamine tendrá exactamente una habilidad con +1».
5. **Pantalla:** Conservación del balón **92 → 93**, y todo lo demás igual. ✅

→ Son barras de crecimiento, y la **barra 6 = Conservación del balón**.

### Experimento 3 · La palanca (10 oct, 01:05–01:11; guía 24)

1. Respaldo de la ranura 6 (`respaldos_ranuras\ML00000005_ranura6_original_16-3-2026`, md5 `1feef3a4…`).
2. En una copia, la barra 6 de Lamine pasó de **551 a 9.999**. Solo cambiaron 2 bytes. Esa copia quedó en la ranura 6 (md5 `f220841e…`).
3. FRALEX la cargó: Conservación **93**. Es lo esperado, porque aún no ha avanzado nada.
4. Avanzó varios días: Conservación **94**. ✅
5. En la ranura 7 (11/4/2026), guardada después, el archivo mostró que a Lamine le dieron la vuelta las barras **1, 6, 22 y 25**. En pantalla le subieron **4** habilidades: Actitud ofensiva, Conservación, Equilibrio y Salto. ✅ Cuadra.

### Experimento 4 · El mapa completo (10 oct, 01:20–01:50; guía 25)

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

## 3. Resultado final: el mapa de barras y las reglas

### 3.1 · El mapa ✅

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

### 3.2 · Reglas comprobadas

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
- barra 1 → `0xc5125e`;
- barra 6 → `0xc5125e + 10` = **`0xc51268`** ✅ (es la que se cambió en la palanca).

En esta carrera la tabla D empieza en `0xc50818` (registro índice 0) y Lamine es el índice 7. Los registros van **seguidos**, cada 368 bytes.

### 4.3 · Cómo encontrar la tabla D en CUALQUIER guardado (sin posiciones fijas)

Las posiciones de 4.2 son **de esta carrera**: no hay que copiarlas a ciegas. El método general ya está programado en `GuardadoLM::tablasDe(k)` (`PhoenixSync/core/LigaMaster.cpp`):

1. Busca el equipo del usuario: el bloque de equipo que lleva la marca **−11** (`kMarcaUsuario`).
2. Lee su plantilla: la lista de pares `(reg, pid)`.
3. Después de los bloques de equipo (`kFinBloques`), busca el par `(reg, pid)` del **primer** jugador (8 bytes seguidos: reg y luego pid).
4. En cada sitio donde aparezca, comprueba que los **4 jugadores siguientes** de la plantilla están a distancias de **368 bytes** (admite huecos vacíos: `pid == 0` y `reg` en 0, 0xffff o vacío).
5. Si cuadra, es la tabla D. Así obtienes `ofsReg0` = posición del reg del registro 0.

**Comprobado (10 oct, 02:05):** `tablasDe` sobre la ranura 9 encuentra el equipo del usuario (bloque **k = 122**) y su tabla D con `stride = 368` y `ofsReg0 = 0xc5081c`. Es justo 4 bytes después del inicio del registro 0 (`0xc50818`). ✅

El registro del jugador número *i* de la plantilla empieza en `ofsReg0 − 4 + 368·i`. Puede haber huecos: lo seguro es recorrer los registros y quedarse con el que tenga el `reg`/`pid` buscado.

**Cuidado:** el `reg` cambia de prefijo según la carrera. En la carrera del City era `0xdb65xxxx`, en la del Barça `0xdbdfxxxx`, o valores bajos como `0xdb1`. Hay que compararlo **tal cual** con el de la plantilla, sin suponer el prefijo.

### 4.4 · Comportamiento de una barra

- Valor: u16 de **0 a 9.999**. Nunca escribir ≥ 10.000.
- Al avanzar el tiempo, el juego suma (jóvenes) o resta (veteranos) un poco cada día y con los partidos.
- **Dar la vuelta hacia arriba:** valor viejo alto → valor nuevo bajo. Ejemplos reales: 9.999 → 327, 9.969 → 595, 9.690 → 6. → habilidad **+1**.
- **Detectarlo en un programa:** `nuevo < viejo − 3000` = dio la vuelta hacia arriba. `nuevo > viejo + 3000` = dio la vuelta hacia abajo (se espera −1, ⏳).
- Si la habilidad está en **99**, la barra no avanza.

---

## 5. Guía de réplica, paso a paso (para humanos)

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

## 6. Guía de réplica, paso a paso (para una IA o un programa)

### 6.1 · Herramientas que ya existen

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

### 6.2 · Algoritmo

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

### 6.3 · Código de referencia (Python, sobre datos ya descifrados)

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

### 6.4 · Comprobación con el juego (obligatoria)

Un cambio **no** está probado hasta que se ve en pantalla. Hay que dejar anotados:
- valor antes (captura);
- valor después de avanzar (captura);
- guardado del después en ranura nueva;
- lectura de las barras del después, para comprobar que dieron la vuelta las esperadas.

---

## 7. El método del «código secreto» para mapear muchas barras en una sola prueba

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

## 8. Lista de comprobación de 0 errores

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

## 9. Multiparche 🧩

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

## 10. Riesgos, límites y cosas que NO hacer

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

## 11. Lo que falta descubrir y cómo hacerlo

| Qué | Cómo |
|---|---|
| **Barras de portero** (3, 15, 16, 17, 18) | Mismo «código secreto», con 3 porteros (Joan García, Szczęsny y otro) y cada barra en una combinación distinta de 2 porteros. Mirar la página 3/4 (Actitud de portero, Atajar, Despejar, Reflejos, Cobertura). |
| **Barras lentas** (19, 27, 28) | Ponerlas en 9.999 en 3 jugadores y mirar la página 3/4: uso de pie malo, precisión de pie malo, regularidad, resistencia a lesiones (escalas de 1 a 8). |
| **Bajar −1** | Poner una barra de un veterano en 0 o 1 y avanzar. Si da la vuelta hacia abajo, ¿la habilidad baja −1? |
| **+N de golpe** | Probar si avanzar varios días seguidos con la barra recargada entre medias es lo único posible, o si hay un contador que limite. |
| **Otros equipos** | Buscar si los jugadores de equipos que no son del usuario tienen barras en otro sitio (guía 21, búsquedas pendientes). |
| **Valor absoluto** | Sigue sin localizarse dónde guarda la carrera el número de cada habilidad. Con el mapa de barras, una búsqueda nueva tiene más pistas (comparar ranuras 8 y 9: cada habilidad que subió +1 tiene que estar en algún sitio). |

---

## 12. Archivos, herramientas y huellas

### 12.1 · En el repositorio (`Fralexito/smash-soda-fork`, rama `mercado-fase0`)

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

### 12.2 · En el PC de FRALEX

- **Guardados:** `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`.
- **Respaldos:** `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba\respaldos_ranuras\`.

### 12.3 · Huellas (md5) de todos los guardados usados

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

## 13. Anexo: datos crudos de las pruebas

### A.1 · Barras de Lamine (registro en `0xc51228`) en tres fechas

| Barra | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1/1 (ranura 4) | 8432 | 2340 | 0 | 3308 | 6527 | 9765 | 1342 | 3487 | 1060 | 6598 | 1713 | 5701 | 4671 | 3280 | 8588 | 5874 | 4418 | 8727 | 2415 | 6666 | 8797 | 2707 | 5118 | 3815 |
| 16/3 (ranura 6) | 9969 | 3233 | 0 | 4134 | 7956 | 551 | 2555 | 5024 | 2273 | 7412 | 2322 | 6310 | 6208 | 4817 | 8691 | 5874 | 5631 | 9940 | 3775 | 7492 | 9690 | 3891 | 5118 | 3993 |
| 11/4 (ranura 7) | 595 | 3549 | 0 | 4520 | 8592 | 327 | 3026 | 5650 | 2744 | 7699 | 2542 | 6530 | 6834 | 5443 | 8752 | 5874 | 6102 | 411 | 4296 | 7878 | 6 | 4307 | 5118 | 4086 |

Barras 15–18 = 0 y 29–30 = 0 en las tres fechas.
- Entre el 1/1 y el 16/3 dio la vuelta la barra 6 → Conservación 92→93 ✅.
- Entre el 16/3 (con la barra 6 puesta a mano en 9.999) y el 11/4 dieron la vuelta las barras 1, 6, 22 y 25 → Actitud ofensiva, Conservación, Equilibrio y Salto ✅.
- La barra 20 (Velocidad) está **quieta** en 5874: su Velocidad está en 99.

### A.2 · Cómo se leyeron las capturas

- Páginas **2/4** (17 habilidades) y **3/4** (Actitud defensiva, Recuperación, Agresividad, 5 de portero, pie malo ×2, regularidad, resistencia a lesiones) de la pantalla «Habilidad» de cada jugador.
- Todas las diferencias antes→después fueron de **0 o +1**: ninguna bajó y ninguna subió +2. Comprobado con un programa.

### A.3 · Habilidades de los 7 jugadores, antes (ranura 8) y después (ranura 9)

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

### A.4 · Comprobación jugador por jugador

| Jugador | Barras puestas en 9.999 | Barras que dieron la vuelta (archivo) | Habilidades que subieron (pantalla) | ¿Cuadra? |
|---|---|---|---|---|
| Roony Bardghji | 2, 5, 8, 9, 10, 11, 12, 14, 26 | 2, 5, 8, 9, 10, 11, 12, 14, 26 | Control de balon, Pase raso, Pase bombeado, Cabeceo, Efecto, Resistencia, Actitud defensiva, Recup. de balon, Agresividad (9) | ✅ |
| Josué Caicedo | 1, 2, 4, 7, 11, 14, 20, 23, 26 | 1, 2, 4, 7, 11, 14, 20, 23, 26 | Actitud ofensiva, Regate, Finalizacion, Efecto, Velocidad, Potencia de tiro, Resistencia, Actitud defensiva, Recup. de balon (9) | ✅ |
| Gavi | 7, 13, 20, 22, 23, 24, 25, 26 | 7, 13, 20, 22, 23, 24, 25, 26 | Finalizacion, Balon parado, Velocidad, Aceleracion, Potencia de tiro, Salto, Equilibrio, Resistencia (8) | ✅ |
| Fermín López | 4, 6, 7, 8, 11, 12, 21, 22, 25 | 4, 6, 7, 8, 11, 12, 21, 22, 25 | Regate, Conservacion del balon, Pase raso, Finalizacion, Salto, Contacto fisico, Equilibrio, Recup. de balon, Agresividad (9) | ✅ |
| Lamine Yamal | 5, 10, 12, 13, 14, 21, 23, 25 | 4, 5, 8, 10, 12, 13, 14, 21, 23, 24, 25 | Control de balon, Regate, Pase raso, Cabeceo, Balon parado, Efecto, Aceleracion, Potencia de tiro, Salto, Contacto fisico, Agresividad (11) | ✅ |
| Marc Bernal | 1, 2, 6, 8, 9, 10, 22, 24 | 1, 2, 6, 8, 9, 10, 22, 24, 25 | Actitud ofensiva, Conservacion del balon, Pase raso, Pase bombeado, Cabeceo, Aceleracion, Salto, Equilibrio, Actitud defensiva (9) | ✅ |
| Pau Cubarsí | 1, 4, 5, 6, 9, 13, 20, 21, 24 | 1, 4, 5, 6, 9, 13, 20, 21, 24 | Actitud ofensiva, Control de balon, Regate, Conservacion del balon, Pase bombeado, Balon parado, Velocidad, Aceleracion, Contacto fisico (9) | ✅ |

### A.5 · Tabla del código (qué 3 jugadores recibieron cada barra)

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

### A.6 · Posición exacta de cada cambio (archivo descifrado de la ranura 8)

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
