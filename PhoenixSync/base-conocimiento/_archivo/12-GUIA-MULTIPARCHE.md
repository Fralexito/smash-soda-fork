# 12 · Guía multiparche: llevar todo lo descubierto a cualquier parche de PES 2021

**Para FRALEX · 9 de octubre de 2026.**
Todo lo de los informes **05, 08, 09, 10 y 11** se descubrió y se probó sobre **ConmeGOL Patch 26**.
Esta guía dice **qué vale para todos los parches**, **qué cambia en cada uno**, **cómo encontrarlo** y **qué riesgos hay**.
Los informes anteriores no se tocaron: esta guía es su «traductor» para cualquier parche.

---

## 0. La idea en simple

Imagina que PES 2021 es **una casa** y cada parche es **una mudanza distinta** dentro de esa casa.

- **La casa** (las paredes, las puertas, cómo se abren los cajones) la pone **el juego**. Es igual en todas las mudanzas.
- **Los muebles** (qué jugadores hay, en qué cajón está la base, qué número tiene cada equipo) los pone **el parche**. Cambian en cada mudanza.

Phoenix Sync aprendió a abrir **los cajones de la casa**. Por eso funciona con cualquier parche.
Lo único que hay que hacer en cada parche es **averiguar dónde dejó sus muebles**.

Símbolos: ✅ probado en el juego · 🔎 visto en archivos · ⏳ sin probar todavía · ⚠️ riesgo.

---

## 1. Lo que es IGUAL en todos los parches (lo pone el juego)

| Qué | Por qué vale para todos |
|---|---|
| **El sobre y la llave** de los guardados (option file y Liga Máster) | La llave es de PES 2021, no del parche |
| **El formato del option file** (plantillas de 284 B, tácticas, jugadores editados) | Lo define el juego |
| **El formato de `Player.bin`** (312 B por jugador, cabecera WESYS + zlib) y de `PlayerAssignment.bin` (16 B) | Lo define el juego |
| **La estructura de la Liga Máster** (bloques de equipo, alineaciones, tablas de tu equipo, contratos, blob) | Lo define el juego. Phoenix Sync la encuentra **por anclas**, sin direcciones fijas |
| **Editar → Cargar** relee la base y el option file | Es un menú del juego |
| **La regla de «Datos Actual. en vivo»**: activada → plantillas de la base; desactivada → del option file | Es una opción del juego |
| **Los jugadores editados** del option file mandan sobre `Player.bin` | Comportamiento del juego |
| **El autoguardado** encima de la ranura cargada, y **el contrato** que crea el juego al fichar | Comportamiento del juego |
| **Los límites**: 750 equipos, 40 jugadores por equipo, 30.000 jugadores | Límites del juego (salvo parches con exe modificado, ver §4) |

---

## 2. Lo que CAMBIA en cada parche (y cómo encontrarlo)

### 2.1 La carpeta de guardados 🧩

- **Qué es:** `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<número>\save\`. Cada parche puede usar **su propio número**.
- **Ejemplos en tu PC:** ConmeGOL → `239200`. Sudamerican → `292733975847239680`.
- **Cómo encontrarla en otro parche:**
  1. Abre el juego con ese parche y guarda algo (por ejemplo, Editar → Guardar en un archivo de prueba, o una carrera nueva).
  2. Mira cuál carpeta `save` cambió de fecha **justo en ese momento**.
- ⚠️ **Riesgo:** en tu PC hay notas que dicen que ConmeGOL y Sudamerican **comparten** la carpeta de guardados. Compruébalo siempre antes de tocar nada: si dos parches comparten carpeta, **un option file de un parche puede romper el otro**. Copia la carpeta `save` antes de cambiar de parche.

### 2.2 Dónde está la base de datos que el juego usa de verdad 🧩

**En simple:** el juego lee varias cajas en orden, y **gana la primera** que tenga el archivo. Hay que saber cuál gana en cada parche.

| Estilo de parche | Ejemplo | Dónde está la base | Cómo encontrarla |
|---|---|---|---|
| **Sider / livecpk** (carpetas sueltas) | ConmeGOL | Una carpeta servida por Sider (en ConmeGOL: `olmosjr23\Database\common\etc\pesdb\`) | En `sider.ini`, la **primera** línea `cpk.root` cuya carpeta tenga `common\etc\pesdb\Player.bin` |
| **Clásico de CPK** | Sudamerican | Dentro de un `.cpk` en `download\` (en Sudamerican: `SP_Subs.cpk`) | En `download\DpFileList.bin`, el **último** CPK de la lista que tenga `pesdb` (los de abajo ganan). Ojo: si además usa Sider con una base suelta, **gana Sider** |
| **Juego «propio»** (exe, lanzador e instalador propios) | Football Life | Su propio sistema | ⏳ Sin estudiar. Hay que auditarlo antes |
| **Sider + base** | Evoweb | Una raíz de Sider | Igual que el primer caso |

- ⚠️ **Riesgo 1:** si se fabrica Phoenix-DB desde la base **equivocada** (por ejemplo, el `.cpk` cuando gana Sider), el juego recibe datos viejos. Pasó en ConmeGOL: el `.cpk` era del 19/8 y la base real del 5/9, con **5.894 jugadores** de diferencia.
- ⚠️ **Riesgo 2:** cuando el parche se actualiza, Phoenix-DB **tapa** la base nueva. Hay que **regenerarla** antes de jugar.

### 2.3 Cómo poner Phoenix-DB en cada parche 🧩

| Caso | Qué hacer | Riesgo |
|---|---|---|
| El parche usa **un** Sider | Añadir `cpk.root = ".\livecpk\Phoenix-DB"` **antes** de las demás raíces | Bajo |
| El parche usa **varios** Sider o modos (ConmeGOL tiene modos con switcher; Sudamerican tiene Sider normal y Sider Mundial) | Ponerlo en **cada** `sider.ini` y en **cada** carpeta de modo | ⚠️ Si falta en uno, ese modo usará la base del parche sin avisar |
| Un **switcher o lanzador** copia carpetas encima (ConmeGOL usa robocopy; Sudamerican usa `.bat` por letra de disco) | Poner Phoenix-DB también en la carpeta de **origen** que se copia | ⚠️ Una actualización del parche puede borrar la línea. Phoenix Link debe **comprobar** `sider.ini` en cada arranque y avisar |
| El parche **no usa Sider** | Opción A: instalar Sider aparte. Opción B: un `.cpk` propio al final de `DpFileList.bin` | ⚠️ Medio/alto. La opción B necesita el fabricador de CPK (🟡 aún no hecho). **No hacerlo sin respaldo completo** |

### 2.4 Los IDs de equipos y jugadores 🧩

- Cada parche puede numerar distinto. Ejemplos de ConmeGOL: Real Madrid = ID 109, Santos = 1254, Lamine Yamal = 162114.
- Los parches que usan **IDs reales de Konami** (como Evoweb) se parecen mucho entre sí. Los que crean jugadores propios, menos.
- **Cómo se resuelve:** Phoenix Sync hace un **catálogo por parche** y un **emparejamiento** entre parches. Ya se probó ConmeGOL ↔ Sudamerican: 10.676 jugadores emparejados solos, 3.849 para revisar a mano y 3.716 sin pareja.
- ⚠️ **Riesgo:** nunca usar el catálogo de un parche en otro. Ante la duda, el emparejamiento **no adivina**: lo manda a revisión.

### 2.5 Los jugadores editados del option file 🧩

- En ConmeGOL el option file trae **21** jugadores editados. Otros parches pueden traer **miles**.
- **Por qué importa:** los editados **mandan** sobre `Player.bin`. Si un parche edita a Lamine en su option file, cambiar su velocidad en Phoenix-DB **no se verá**.
- **Qué hacer:** antes de cambiar stats, Phoenix Sync debe **comprobar** si el jugador está editado. Si lo está, hay que cambiarlo **dentro del option file** (⏳ falta programarlo).

### 2.6 La Liga Máster en otros parches 🧩

- La estructura es del juego, pero **los tamaños** cambian con el parche: en ConmeGOL hay 700 bloques de equipo, 629 alineaciones y 16.422 fichas.
- Phoenix Sync **no supone esos números**: busca todo por anclas y **se niega a escribir** si algo no cuadra (en vez de estropear el archivo).
- ⚠️ **Riesgo:** **no está probado** en otro parche (⏳). En cada parche nuevo hay que repetir las pruebas clave en una **carrera nueva**:
  1. Un traspaso entre dos equipos de la IA.
  2. Una venta de tu equipo.
  3. Un fichaje para tu equipo.
  4. Cambiar el presupuesto.
  5. Jugar un partido y guardar en otra ranura.
- ⚠️ **Riesgo 2:** muchos parches piden **empezar una carrera nueva** tras actualizar la base. Una carrera vieja con base nueva puede dar plantillas raras.

### 2.7 La opción «Datos Actual. en vivo» 🧩

- La regla es del juego, pero **un parche puede dejarla activada o desactivada** por defecto, o tener un módulo que la cambie.
- **Qué hacer:** pon los fichajes en **los dos sitios** (option file y `PlayerAssignment.bin` de Phoenix-DB). Así se ven siempre.
- ⚠️ **Riesgo:** si un parche trae su propio `PlayerAssignment.bin` más nuevo y Phoenix-DB se quedó con uno viejo, con la opción activada verías plantillas viejas. Regenerar siempre al actualizar.

---

## 3. Guía paso a paso para un parche nuevo

Haz los pasos **en orden**. Si uno falla, **no sigas**: vuelve al respaldo.

1. **Ficha del parche.** Crea `parches/<parche>.md` con: nombre y versión, carpeta del juego, carpeta `save`, estilo (Sider / CPK / propio), dónde está la base, cuántos `sider.ini` hay y si hay switcher o lanzador.
2. **Huella del «antes».** Anota la huella de la base, `DpFileList.bin`, cada `sider.ini` y el option file. Así sabrás exactamente qué cambió después.
3. **Respaldo completo** de la carpeta `save` y de cada `sider.ini`.
4. **Localiza la base real** (§2.2) y descomprímela.
5. **Catálogo** del parche (`PhoenixSync catalogo …`) y, si vienes de otro parche, **emparejamiento** (`PhoenixSync emparejar …`).
6. **Phoenix-DB** (§2.3): primero con la base **original** recomprimida, sin cambios. Arranca el juego y comprueba que **todo sigue igual**.
7. **Prueba de stats:** un jugador conocido, un cambio visible (por ejemplo, Velocidad). Editar → Cargar → mira su ficha. Comprueba antes que **no esté editado** (§2.5).
8. **Prueba de fichaje:** mueve un jugador en el option file **y** en `PlayerAssignment.bin`. Editar → Cargar → amistoso. Prueba con la opción en vivo **activada y desactivada**.
9. **Liga Máster:** carrera nueva y las 5 pruebas de §2.6, cada una en **otra ranura**.
10. **Anota todo** en `PRUEBAS.md` (salga bien o mal) y en la ficha del parche.

---

## 4. Tabla de riesgos por estilo de parche

| Estilo | Qué funciona seguro | Riesgos principales |
|---|---|---|
| **Sider / livecpk** (ConmeGOL) | ✅ Todo lo de los informes 05, 08, 09 y 10 (probado aquí) | Switcher que pisa `sider.ini`; base real distinta del `.cpk` |
| **Clásico de CPK** (Sudamerican) | 🔎 Option file y emparejamiento. Lo demás ⏳ | Varios Sider (normal y Mundial); módulos cargados dos veces; carpeta `save` compartida; `netblock.lua` bloquea el online |
| **Juego «propio» con exe modificado** (Football Life) | ⏳ Nada probado | El exe cambia límites (ligas de 30, Mundial de 48): la Liga Máster podría tener **otros tamaños**. PESBUL no funciona con exe modificado. **Auditar antes de tocar nada** |
| **Sider + base con IDs reales** (Evoweb) | ⏳ Debería ser el más fácil (IDs de Konami) | No convive con otra base de datos: **nunca mezclar** con otra Phoenix-DB |
| **Paquetes enormes** (Gogosz, ~200 GB) | ⏳ | Espacio en disco y tiempo para respaldos |

**Riesgos que valen para todos:**
- Mezclar archivos de dos parches (base de uno, option file de otro) → jugadores «sin nombre, media 40». Phoenix Sync tiene un **modo seguro** que lo detecta y se niega a escribir.
- Actualizar un parche y no regenerar Phoenix-DB → datos viejos.
- Pulsar **Guardar** en Editar durante una prueba → el option file de prueba se queda para siempre (por eso, siempre respaldo).

---

## 5. Cómo leer los informes anteriores en otro parche

| Informe | Lo que es solo de ConmeGOL | Cómo se traduce a otro parche |
|---|---|---|
| **05 · Puerta en vivo** | La raíz `olmosjr23\Database`; las dos carpetas por el switcher; la carpeta `239200` | La base real del parche (§2.2); cada `sider.ini` del parche (§2.3); su carpeta `save` (§2.1) |
| **08 · Liga Máster: informe** | City = bloque 154, Real Madrid = 135, Santos = 66; ranuras de tu PC | Otros números de bloque: Phoenix Sync los busca solo. Repetir las pruebas (§2.6) |
| **09 · Liga Máster: cómo lo hicimos** | Direcciones de ejemplo (`0xc7dd00`, `0x9D4648`…) y jugadores de prueba (Haaland, Sommer, Guéhi) | El **método** sirve igual: copia → abrir → verdad del juego → comparar → programar → probar. Las direcciones exactas pueden moverse: por eso todo va por anclas |
| **10 · Cómo se abrió el guardado** | La carpeta `239200` y el texto «Manchester City / Premier League» | La llave y el sobre son **iguales en todos los parches** ✅. Solo cambia la carpeta |
| **11 · Súper informe** | Los ejemplos marcados con 🧩 | Esta guía |

---

## 6. Estado por parche (hoy)

| Parche | Option file | Base (Phoenix-DB) | Liga Máster | Comentario |
|---|---|---|---|---|
| **ConmeGOL 26** | ✅ | ✅ | ✅ | Todo probado en el juego |
| **ConmeGOL 27** | ⏳ | ⏳ | ⏳ | Seguir el protocolo del «día de actualización» (informe 04, §7.3) |
| **Sudamerican 2026** | 🔎 (emparejamiento hecho) | ⏳ | ⏳ | Auditoría de carpeta hecha |
| **Football Life 27** | ⏳ | ⏳ | ⏳ | Exe propio: el de más riesgo |
| **Evoweb, Gogosz y otros** | ⏳ | ⏳ | ⏳ | Sin estudiar |

---

## 7. Regla para las guías futuras

Toda guía nueva de Phoenix debe:
1. Separar **lo que pone el juego** (vale para todos) de **lo que pone el parche** (marcado con 🧩).
2. Dar los ejemplos del parche donde se probó, **y** decir cómo encontrar el dato en otro parche.
3. Incluir los **riesgos** de cada paso.
4. Decir en qué parches está **probado** y en cuáles no.
