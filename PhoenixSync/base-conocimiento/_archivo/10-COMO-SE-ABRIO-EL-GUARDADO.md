# 10 · Cómo se abrió el guardado de la Liga Máster, paso a paso

**Para FRALEX · 9 de octubre de 2026.** Este informe cuenta solo **la apertura** del archivo de la Liga Máster (`ML0000000N`): cómo se pasó de un archivo «revuelto» a uno que se puede leer y editar, y cómo se volvió a cerrar para que el juego lo acepte.
Lo que se hizo **después** de abrirlo (fichajes, dinero, blob) está en `09-LIGA-MASTER-COMO-LO-HICIMOS.md`.

---

## 0. La idea con un ejemplo sencillo

Imagina que el guardado es **una carta dentro de un sobre cerrado con candado**.

- **El sobre** = el cifrado. Desde fuera solo se ven letras sin sentido.
- **La llave** = la clave de PES 2021. Es **la misma para todos los guardados** del juego (option file, Liga Máster, etc.).
- **La carta** = los datos de tu carrera (plantillas, dinero, contratos…).
- **La etiqueta del sobre** = el texto que ves en el menú *Cargar* (equipo, fecha, competición).

Abrir el guardado = **abrir el sobre con la llave, leer la carta, cambiarla y volver a meterla en un sobre igual**, con la misma etiqueta (o una nueva), para que el juego no note la diferencia.

---

## 1. Palabras que se usan aquí

| Palabra | Qué significa |
|---|---|
| Cifrar | Revolver los datos con una llave para que no se puedan leer |
| Descifrar | Usar la llave para dejarlos legibles otra vez |
| Byte | El trocito más pequeño de un archivo (un número del 0 al 255) |
| Cabecera | La primera parte del archivo, que dice qué es y cuánto mide cada pieza |
| Ida y vuelta | Abrir y volver a cerrar sin cambiar nada, y comprobar que sale idéntico |
| Huella (md5 / sha256) | Un código que identifica un archivo exacto. Si cambia un solo byte, cambia la huella |
| Compilar | Convertir un programa escrito en texto (código) en un programa que se puede ejecutar |

---

## 2. Las herramientas que se usaron

| Herramienta | Qué es | Dónde está |
|---|---|---|
| libpesXcrypter | Biblioteca libre y de dominio público (licencia *Unlicense*) hecha por la comunidad de PES. Sabe abrir y cerrar los guardados de PES. Trae la llave de PES 2021 | `PhoenixSync/terceros/pesxcrypter/` |
| gcc | Compilador de C (en la nube, Linux) | — |
| MinGW | El mismo compilador, pero produce programas para Windows (tu PC) | — |
| `dec` | Programa propio: abre el guardado, lo vuelve a cerrar y comprueba que salga idéntico | `liga-master/herramientas/dec.c` |
| `info` | Programa propio: lee la etiqueta y la cabecera, sin cambiar nada | `liga-master/herramientas/info.c` |
| `enc2` | Programa propio: cierra los datos editados en un sobre nuevo y puede cambiar la etiqueta | `liga-master/herramientas/enc2.c` |
| `SobrePes` | La versión definitiva dentro de Phoenix Sync (C++): abre, cierra con cualquier tamaño y nunca sobrescribe | `core/SobrePes.h` y `.cpp` |
| El enlace con tu PC | Copia archivos entre tu PC y la nube | App de escritorio de Claude |
| md5 / sha256 | Para comprobar que cada archivo es exactamente el que se cree | — |

> Importante: **no se tocó el juego** para abrir el guardado. Se usó una biblioteca pública que la comunidad de editores de PES usa desde hace años.

---

## 3. Paso a paso

### Paso 1 · Darse cuenta de que ya teníamos la llave
- **Qué pasaba:** antes de la Liga Máster, Phoenix Sync ya abría el **option file** (`EDIT00000000`) para hacer fichajes.
- **Qué se pensó:** PES 2021 usa **el mismo tipo de sobre** para todos sus guardados. Si la llave abre el option file, debería abrir también la Liga Máster.
- **Herramienta:** la misma biblioteca libpesXcrypter que ya estaba en `terceros/pesxcrypter`.

### Paso 2 · Traer una copia del guardado a la nube
- **Qué:** se copió `ML00000000` (tu carrera con el City) desde
  `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` a la nube.
- **Cómo:** con el enlace con tu PC (copia, no mueve). **El original nunca se tocó.**
- **Además:** se guardó una copia de respaldo en tu PC (`save\_referencias_mercado\`).

### Paso 3 · Escribir y compilar `dec` (el abridor)
- **Qué hace, en orden:**
  1. Lee el archivo cifrado completo.
  2. Prepara un «sobre vacío» (en el código, `createFileDescriptorNew`).
  3. Lo abre con la llave de PES 2021 (`decryptWithKeyNew`).
  4. Muestra qué tipo de archivo es, la versión del juego y cuánto miden los datos.
  5. Guarda **los datos descifrados** en un archivo nuevo (`salida.bin`).
  6. **Vuelve a cerrar** esos datos con la misma llave (`encryptWithKeyNew`) en otro archivo nuevo.
  7. Compara el cerrado nuevo con el original **byte a byte** y dice si son idénticos.
  8. Lo abre una segunda vez y comprueba que los datos sean iguales a los primeros.
- **Cómo se compiló** (desde la carpeta de herramientas):
  ```
  gcc -O2 -o dec dec.c (+ los 3 archivos de la biblioteca: crypt.c, masterkey.c, mt19937ar.c)
  ```
- Lo mismo para `info` y `enc2`.

### Paso 4 · Abrirlo por primera vez
- **Resultado:** se abrió sin problemas. Salieron **19.759.355 bytes** de datos legibles (unos 19,7 MB).
- **Ida y vuelta:** el archivo volvió a cerrarse **idéntico**, y los datos de la segunda apertura eran iguales a los de la primera. ✅
- **Qué significa:** la llave es correcta y el sobre se puede **abrir y cerrar sin dañar nada**.

### Paso 5 · Entender las piezas del sobre (con `info`)
`info` mostró que el guardado tiene **5 piezas**, en este orden:

| Pieza | Tamaño | Qué es |
|---|---|---|
| Cabecera | fija | Tipo de archivo, versión del juego, tamaño de cada pieza y una huella |
| Descripción | 384 bytes | La «etiqueta» del guardado |
| Logo | 17.315 bytes | La imagen que acompaña al guardado |
| Datos | ~19,7 MB | Tu carrera (lo que nos interesa) |
| Serial | 44 bytes | Un código de identificación |

### Paso 6 · Descubrir qué muestra el menú *Cargar*
- **Primera idea:** la etiqueta son los primeros 128 bytes de la descripción (un «nombre»). Se cambió para reconocer las ranuras de prueba.
- **Prueba en el juego:** ❌ el menú *Cargar* **no mostraba** ese nombre.
- **Segunda mirada con `info`:** a partir del byte 128 de la descripción hay un **texto de 3 líneas**:
  ```
  Equipo / Liga
  Fecha
  Competición
  ```
- **Prueba en el juego:** ✅ **eso es lo que muestra** el menú *Cargar*.
- **Arreglo:** `enc2` recibió un 5.º dato para cambiar la **primera línea** de ese texto. Así tus ranuras de prueba salían como «PRUEBA 4», «PRUEBA 5»…

### Paso 7 · Escribir `enc2` (el que vuelve a cerrar)
- **Qué hace, en orden:**
  1. Vuelve a abrir el **original** (para reutilizar su cabecera, logo y serial).
  2. Cambia los datos por los **datos editados**.
  3. Si se pide, cambia el nombre de 128 bytes y el texto info.
  4. Lo cierra con la llave y lo guarda como **archivo nuevo** (nunca encima del original).
- **Uso típico:**
  ```
  enc2 <original> <datos_editados.bin> <ML_nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'
  ```

### Paso 8 · La gran duda: ¿el juego acepta un sobre «rehecho»?
- **El riesgo:** la cabecera trae una **huella** del contenido. Al cerrar, la biblioteca **no la recalcula**. Si el juego la comprobara, rechazaría el archivo editado.
- **Prueba en el juego:** se cerró un guardado **con cambios** y lo cargaste.
- **Resultado:** ✅ **el juego lo aceptó**. No comprueba esa huella.
- **Qué significa:** se puede editar la carrera y el juego la carga normal.

### Paso 9 · Mirar por dentro los datos ya abiertos
- Con los datos legibles se vio el comienzo:
  - una **cabecera interna de 80 bytes** (contiene el número 700, la cantidad de equipos);
  - después, los **700 bloques de equipo**, empezando en el byte 80 (`0x50`).
- **Detalle:** 32 bytes de esa cabecera interna cambian en cada guardado y **el juego no los valida**.
- Desde aquí empezó el trabajo de fichajes (ver el informe 09).

### Paso 10 · El problema del tamaño
- **Qué se vio:** al fichar a Neymar, el guardado del juego **creció 3.662 bytes**. Y el «blob» (la zona comprimida) crece con la temporada.
- **El límite:** `enc2` solo sabía cerrar datos **del mismo tamaño** que el original.
- **Solución:** en el programa definitivo (`core/SobrePes.cpp`) se cambia el bloque de datos por uno nuevo **del tamaño que haga falta** y se **actualiza el tamaño en la cabecera** antes de cerrar.
- **Prueba:** fichajes y blob recomprimido (pruebas 17, 18 y 19) cargaron bien en el juego. ✅

### Paso 11 · Hacerlo seguro para siempre (`SobrePes`)
El cerrador definitivo de Phoenix Sync sigue estas reglas:
1. **Comprueba el tipo** de archivo antes de abrir (que sea un `ML` y no otra cosa).
2. **Rechaza** tamaños imposibles.
3. **Nunca sobrescribe**: si el archivo de destino ya existe, se niega.
4. Escribe primero un **archivo temporal**, lo **vuelve a abrir para comprobarlo** y solo entonces le pone el nombre final.
5. Conserva la cabecera, el logo y el serial **del original**.

### Paso 12 · Llevarlo a tu PC
- Las ranuras de prueba se devolvieron a tu carpeta `save` con el enlace con tu PC.
- **Siempre se anotó la huella** de cada archivo entregado. Así se supo después si el archivo era el nuestro o si el juego ya había guardado encima.
- Las pruebas de Windows se compilaron con **MinGW** y se corrieron en tu PC con `PROBAR2.bat`.

---

## 4. Resumen de resultados

| Qué | Resultado |
|---|---|
| Abrir el guardado con la llave de PES 2021 | ✅ |
| Abrir y cerrar sin cambios → archivo idéntico | ✅ |
| El juego acepta un guardado editado (sin recalcular la huella) | ✅ probado en el juego |
| Cambiar la etiqueta del menú *Cargar* | ✅ (texto info, no el nombre de 128 bytes) |
| Cerrar con un tamaño distinto | ✅ (`SobrePes`) |
| Nunca sobrescribir el original | ✅ |

---

## 5. Errores y cómo se arreglaron

| Error | Causa | Arreglo |
|---|---|---|
| El menú *Cargar* no mostraba la etiqueta de prueba | Se cambió el nombre de 128 bytes, que el menú no usa | Cambiar el texto info (desde el byte 128) |
| `enc2` no podía cerrar datos más grandes | Exigía el mismo tamaño que el original | `SobrePes` cierra con cualquier tamaño y corrige la cabecera |
| A veces `dec` se cerraba con error al final | Ocurre después de escribir los archivos (causa exacta sin investigar) | Se comprobó que los archivos salían completos (ida y vuelta idéntica). No afecta al resultado |
| El juego guardaba encima de la ranura de prueba | Autoguardado al terminar un partido | Anotar la huella de cada entrega |

---

## 6. Reglas que quedaron

1. **Nunca** trabajar sobre el original: siempre una copia.
2. **Siempre** hacer la ida y vuelta antes de editar: si no sale idéntico, no se sigue.
3. **Siempre** cerrar en un archivo **nuevo**.
4. **Siempre** anotar la huella de lo que se entrega.
5. Los guardados y los datos abiertos (`.bin`) **no se suben a GitHub**: son tus datos.
