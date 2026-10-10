# 19 · «Datos Actual. en vivo»: informe paso a paso de todo lo que se hizo

**Sesión del 2026-10-09, de 21:04 a 23:35 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. El resultado resumido está en [17 · dónde está la opción](17-OPCION-EN-VIVO-DONDE-ESTA.md).
Este documento cuenta **el camino completo**, en orden, con las horas, los errores y lo que se decidió en cada momento.

---

## 0. La pregunta y la respuesta

**Pregunta de FRALEX:** ¿cómo puede saber `phoenix.lua`, de forma segura y solo leyendo, si «Datos Actual. en vivo» está activado?

**Respuesta en 4 líneas:**

1. La opción no está en ningún archivo. Está en la memoria del juego.
2. Es un número dentro del «gestor de la base»: 1 = activada, 0 = desactivada.
3. Con la v0.17, ese número vale 1 solo unos 3 segundos cada vez que se pulsa Activar.
4. Por eso `phoenix.lua` ahora **recuerda él mismo** cuál fue la última recarga (versión de prueba v0.17a).

---

## 1. Palabras que se usan

- **Huella (sha256):** código que sale de un archivo. Si el archivo cambia en 1 byte, la huella cambia.
- **Foto:** apuntar la huella de todos los archivos de una carpeta en un momento, para comparar después.
- **Registro de Sider (`sider.log`):** el diario donde Sider anota lo que pasa, con la hora.
- **Espía:** la parte de `phoenix.lua` que anota cada vez que el juego lee un archivo de la base.
- **Exe:** el archivo `PES2021.exe`.
- **Desensamblar:** traducir los bytes del exe a instrucciones legibles. No cambia el archivo.
- **RVA:** posición dentro del exe. Se escribe `exe+0x…`.
- **Puntero:** un dato que guarda la dirección de otro dato.
- **Gestor de la base:** el objeto del juego que carga los archivos de datos.
- **Copia segura:** leer la memoria con `ReadProcessMemory`. Si la zona no existe, falla sin cerrar el juego.
- **Simular:** ejecutar el Lua fuera del juego, con una memoria falsa, para probarlo sin riesgo.

---

## 2. Reglas que se respetaron

- Nada se cambió sin el «sí» de FRALEX.
- Nunca se escribió en la memoria del juego.
- El exe nunca se modificó: se estudió una copia.
- Respaldo antes de cada instalación, y huella comprobada en el PC después de copiar.
- No se pidieron contraseñas ni tokens.
- No se hizo `git pull` ni `git checkout` en `C:\dev\smash-soda-fork`.

**Lo único que cambió en el PC de FRALEX en toda la sesión:**

| Archivo | Carpetas | Qué pasó |
|---|---|---|
| `phoenix.lua` | las dos `modules` | v0.17 → v0.17m (22:57) → v0.17a (23:31) |
| `phoenix.lua.v017` | las dos `modules` | Nuevo. Respaldo de la v0.17 (22:54) |
| `phoenix.lua.v017m` | las dos `modules` | Nuevo. Respaldo de la v0.17m (23:31) |

Las dos carpetas `modules` son:

- `D:\Frank\Games_\Conmegol Patch\SiderAddons\modules\`
- `D:\Frank\Games_\Conmegol Patch\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\`

No se tocó `sider.ini`, ni la carpeta de guardado, ni el option file, ni Phoenix-DB.

---

## 3. Paso 1 · Leer la documentación (21:04–21:08)

1. Se pidió acceso a `C:\dev\smash-soda-fork`. FRALEX lo aprobó.
2. Se leyeron `CLAUDE.md`, `COORDINACION.md`, el final de `PhoenixSync/REGISTRO.md` y de `PRUEBAS.md`, `07-MOTOR-POR-DENTRO.md` y `sider/phoenix.lua`.
3. **Problema:** `INFORME-COMPLETO-2026-10-09.md` no estaba en la carpeta.
4. **Causa:** la carpeta del PC estaba atrasada. Estaba en el commit `540fa1a` y GitHub iba varios commits por delante.
   Se comprobó con `git ls-remote origin`, que solo pregunta y no cambia nada.
5. **Solución sin tocar el PC:** se bajó una copia de la rama `mercado-fase0` a una carpeta temporal de trabajo
   (fuera de las carpetas de FRALEX) y se leyó desde ahí el informe, `SYNC-COMPARTIDO.md` y las entradas nuevas de `PRUEBAS.md`.
6. Se comprobó que el `phoenix.lua` instalado en el juego era el mismo que el del repo (misma huella, `d85e1071…`).

---

## 4. Paso 2 · Fase A: ¿se guarda en disco? (21:08–22:44)

### 4.1 Encontrar la carpeta de guardado

1. Los documentos decían que la carpeta activa es `…\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
2. En la carpeta de usuario no hay «Documents»: está en `D:\Users\Alexander\Documents`.
3. Se pidió acceso solo a `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE`. FRALEX lo aprobó.
4. Dentro hay varias carpetas de guardado: `239200` (la activa: option file de las 19:59), `292733975847239680`, `48` y `SP`.

### 4.2 El guion de las fotos

Se escribió `herramientas/exe/foto_guardado.sh`. En cada foto:

- calcula la huella de cada archivo de `239200\save`, de las otras carpetas de guardado y de `settings.dat`;
- guarda un listado con tamaño y hora de todos los archivos, también los de las subcarpetas;
- copia a la carpeta temporal `SYSTEM00000000`, `GRAPHICS000000`, `EDIT00000000` y `settings.dat`, para poder comparar byte a byte.

Solo lee. No escribe nada en el PC.

### 4.3 Las fotos, una por una

| N.º | Hora | Qué había pasado | Resultado |
|---|---|---|---|
| 0 | 21:09 | Foto inicial | — |
| 1 | 22:02 | FRALEX confirma que el juego está cerrado | Idéntica a la 0 |
| 2 | 22:10 | El juego ya estaba abierto (arrancó a las 22:07:47) | Idéntica |
| 3 | 22:23 | Un minuto después de pulsar Activar (22:22) | Idéntica |
| 4 | 22:31 | Nueve minutos después de ese Activar | Idéntica |
| 5 | 22:32 | Juego cerrado (Sider anotó «All done» a las 22:31:48) | Idéntica |
| 6 | 22:39 | Juego abierto de nuevo (22:33:59) y Desactivar pulsado | Idéntica |
| 7 | 22:41 | Después de otro Activar (22:41:03) | Idéntica |
| 8 | 22:44 | Después de otro Desactivar | Idéntica |

En las fotos 3 y 5 se compararon además los cuatro archivos copiados byte a byte: 0 bytes distintos.
En la carpeta del juego, en ese tiempo, solo cambiaron los diarios de Sider.

### 4.4 Lo que se aprendió por el camino

**a) El registro de Sider sirve de reloj.** Al principio dependíamos de que FRALEX contara qué había hecho y cuándo.
A las 22:26 FRALEX conectó la carpeta del juego. Ahí se vio que el espía de `phoenix.lua` anota en `sider.log`
cada lectura de la base con su hora. Desde entonces se pudo saber sin preguntar:

- cuándo arrancó el juego (primera lectura de la base);
- cuándo se pulsó Activar (dos lecturas completas de la base, con 3 a 5 segundos de diferencia);
- cuándo se cerró («All done»).

**b) El juego dice el estado en pantalla.** La captura de las 22:39 mostró el cuadro
«Datos Actual. en vivo/Valoración uniformes», con la línea **«Selección actual: Desactivar Actualización en vivo»**
y tres posiciones: Activar, Desactivar y Valoraciones generales uniformes.

**c) Al reiniciar, la opción vuelve a «Desactivar».** FRALEX había pulsado Activar a las 22:22 y cerrado a las 22:31.
Al abrir de nuevo, la selección salía en «Desactivar» por defecto.

**d) Después de Activar, la pantalla sigue diciendo «Desactivar».** Las tres capturas de las 22:41 lo muestran:
«Actualización en vivo en curso», luego el mensaje de la forma física, y al volver a entrar, «Selección actual: Desactivar».
Este fue el dato que cambió la investigación.

**e) Desactivar, estando ya en «Desactivar», no hace nada.** A las 22:44 no hubo ninguna lectura nueva de la base.

### 4.5 Errores y dudas de esta fase

- **Sospecha equivocada:** se pensó que la opción estaría en `SYSTEM00000000` (el archivo de ajustes). No cambió nunca.
- **Duda que hubo que resolver:** que Activar no escribiera nada podía deberse a que la opción «ya estaba activada de antes».
  La captura de las 22:39 lo descartó: estaba en «Desactivar».
- **Sin determinar:** el juego lee la base una segunda vez un rato después de arrancar (16 segundos, 4 minutos
  y 1 minuto 45 segundos en las tres sesiones). No se sabe qué pantalla la dispara. En esa lectura el modo vale 0.

**Conclusión de la Fase A:** la opción no se guarda en disco.

---

## 5. Paso 3 · Decisión de FRALEX (22:44)

Se le explicaron dos caminos:

- **Camino 1:** que `phoenix.lua` lleve él mismo la cuenta de la última recarga.
- **Camino 2:** buscar en la memoria el dato que dibuja «Selección actual».

FRALEX eligió: «intenta el camino 2 y luego, si no funciona, vemos el 1».

---

## 6. Paso 4 · Estudiar el exe (22:44–22:53)

En vez de fotografiar a ciegas 7,5 MB de memoria, primero se leyó el código para llegar a un candidato concreto.

### 6.1 Preparación

1. En la carpeta temporal se instalaron `capstone` (desensamblador) y `pefile` (lector de exe) con `pip`.
2. Se copió `PES2021.exe` a la carpeta temporal. Tamaño: 458.806.784 bytes. Huella: `5e27a782…bc224a`.
3. Se leyó la tabla de secciones: el código está en `.trace` (RVA `0x1000`), los textos en `.rdata` (`0x252F000`)
   y los datos en `.data` (`0x339B000`).
4. Se escribió `herramientas/exe/lib_exe.py`, con estas funciones:
   - `show` y `brief`: desensamblar una zona, completa o resumida;
   - `xrefs`: encontrar qué instrucciones apuntan a una dirección;
   - `callers`: encontrar quién llama a una función;
   - `func` y `funcs_in`: límites de cada función, sacados de la tabla `.pdata` (158.648 funciones);
   - `cstr`: leer un texto de `.rdata`.

### 6.2 El rastro, eslabón por eslabón

1. **Buscar textos con «LiveData».** Salieron 58. Entre ellos `Exhibition/LiveData/LiveDataCheck`, `…/LiveDataSet`,
   `…/LiveDataRemove`, `LiveDataRemoveFlow` y `ProcessCmnLiveDataSetFlow`.
2. **Tabla de procesos** en `.data 0x34D7E70`: cada entrada tiene la fábrica del proceso y su nombre.
   - LiveDataCheck → `0x1303F70`
   - LiveDataSet → `0x1308080`
   - LiveDataRemove → `0x1308DD0`
3. **Seguir «Desactivar»**, porque es lo más corto. `0x1308DE0` crea `LiveDataRemoveFlow` con `0x20AF2D0`.
   Su función de pasos es `0x20AF320`.
4. **Dentro de ese flujo** aparece `0x14B6A60`, que es solo `mov rax, [exe+0x3705E10] ; ret`.
   Devuelve un objeto global: el **gestor de la base**. Luego llama a `0x14B6F20(gestor, 0)`.
5. **`0x14B6F20`** pone a 0 el byte `[gestor+0x90+n]`. Tiene 5 llamadores; los otros 4 la llaman solo si ese byte no vale 0.
6. **`ProcessLiveDataCheck`** (`0x13040C0`) contiene la pista clave:
   `call 0x14B6A60 ; cmp dword [rax+0x38], 1`. El juego pregunta «¿el número en `+0x38` vale 1?».
7. **¿Quién escribe ese número?** Se buscó en las funciones del gestor: `0x14B7587` hace `mov [rcx+0x38], r8d`,
   dentro de la función `0x14B7560(gestor, bandera, modo)`. Es «recargar la base con este modo».
8. **¿Quién llama a `0x14B7560` y con qué modo?** Cinco sitios:
   - `0x20AE529` → modo 1 (estado 22 de `LiveDataSetFlow`);
   - `0x20AE705` → modo 2 (estado 24 del mismo flujo);
   - `0x1EFAFB0`, `0x13A4A5D` y `0xE9F708` → modo 0 (carga del EDIT y Editar → Cargar).
9. **Leer los estados 22 a 26** de `LiveDataSetFlow` (tabla de saltos en `0x20AECC0`, 38 estados):
   - estado 22: crea el diálogo de éxito y luego llama a `0x14B7560(gestor, 0, 1)` → **modo 1**;
   - estado 23: espera a que el gestor termine y al menos 2 segundos;
   - estado 26: crea la tarea `editLoadDataInLiveDataSet`.
10. **La tarea de carga del EDIT** (`0x1EFAF60`), si su bandera `[tarea+0x71]` no vale 0, llama a
    `0x14B7560(gestor, 0, 0)` → **modo 0**. Con el parche C esa bandera vale 1.
11. **Comprobación final:** de los 5.413 sitios que llaman a `0x14B6A60`, 12 leen `[gestor+0x38]`,
    y 11 de ellos lo comparan con 1.

### 6.3 La predicción

Con eso se pudo predecir, antes de probar nada:

- pulsar Activar con la v0.17 pone el modo en **1** (estado 22) y unos segundos después en **0** (parche C);
- por eso el espía ve la base leída dos veces;
- por eso la pantalla dice «Selección actual: Desactivar».

---

## 7. Paso 5 · La versión de prueba v0.17m (22:53–23:04)

### 7.1 Cómo se construyó

1. Se partió del `phoenix.lua` instalado (v0.17, huella `d85e1071…`).
2. Se añadió un bloque nuevo antes de `m.key_down`, y cuatro cambios pequeños:
   - versión `0.17` → `0.17m`;
   - tecla **M** en `key_down`;
   - una muestra al empezar cada lectura de `player.bin`, en `livecpk_read`;
   - la línea **[MODO]** al final del overlay.
3. Los cambios se aplicaron con un guion que exige que cada texto buscado aparezca **exactamente una vez**.
   Si no, se detiene. Así no se cambia nada por error.

### 7.2 Qué hace el bloque nuevo, en orden

1. Comprueba tres trozos del código (`0x14B6A60`, `0x13040C9`, `0x14B7587`). Si alguno no coincide, no lee nunca más.
2. Lee 8 bytes en `exe+0x3705E10` con copia segura.
3. Si el puntero vale 0, o es raro, no sigue.
4. Lee con copia segura el trozo `[gestor+0x30 … +0x93]` a un búfer **propio** de 256 bytes.
5. Saca el estado (`+0x30`), el modo (`+0x38`) y las banderas (`+0x90` a `+0x93`).
6. Lo muestra en el overlay y lo anota en `sider.log`.

Nunca escribe en la memoria.

### 7.3 La simulación

Se instaló `lupa` (trae LuaJIT) y se montó un entorno falso de Sider: `log`, `memory`, y un `ffi` con
`ReadProcessMemory`, `WriteProcessMemory` y `VirtualProtect` falsos que **cuentan** cada llamada.

| Caso | Resultado |
|---|---|
| 1. Normal: modo 0, luego 1, luego 2 | Bien |
| 2. Exe distinto | No lee nada más |
| 3. El gestor no existe (puntero 0) | Avisa y no lee |
| 4. Zona ilegible y puntero raro | Avisa y no lee |
| 5. Lecturas de `player.bin` durante Activar | Anota 0, 1, 0 |
| 6. Módulo cargado dos veces (Shift+R) | Bien |

Escrituras contadas en los 6 casos: 0. Guion: `sider/pruebas/simular_v017m.py`.

### 7.4 Respaldo e instalación

1. **22:54** · FRALEX pidió guardar bien la v0.17. Se creó `phoenix.lua.v017` en las dos carpetas `modules`
   y se le envió una copia por el chat. La v0.17 quedó en 5 sitios, todos con la misma huella.
2. **22:57** · FRALEX dio el «sí». Antes de instalar:
   - se comprobó la v0.17 en 6 sitios: 6 de 6 con la huella correcta;
   - se comprobó que el juego estaba cerrado (Sider: «All done» a las 22:57:04).
3. Se copió la v0.17m a las dos carpetas.
4. Se comprobó la huella en el PC: `57ff5ec0…52805` en las dos. Respaldos intactos.

### 7.5 El resultado en el juego

Sesión que arrancó a las 23:00:09. FRALEX pulsó Activar tres veces.

| Hora | Lectura de `player.bin` | Modo |
|---|---|---|
| 23:00:09 | n.º 1, arranque | 0 |
| 23:01:54 | n.º 2, segunda pasada | 0 |
| 23:02:06 | n.º 3, Activar | **1** |
| 23:02:09 | n.º 4 | 0 |
| 23:04:06 | n.º 5, Activar | **1** |
| 23:04:09 | n.º 6 | 0 |
| 23:04:18 | n.º 7, Activar | **1** |
| 23:04:21 | n.º 8 | 0 |

- Tres de tres, igual que la predicción. Sin errores de Lua.
- Captura de FRALEX a las 23:04:30: el overlay dice «[MODO] opción en vivo: DESACTIVADA (0)» y el juego
  «Selección actual: Desactivar». Coinciden.

---

## 8. Paso 6 · Decisión de FRALEX (23:07)

Leer el dato funciona, pero con la v0.17 casi siempre dirá 0. Se le explicaron dos opciones:

- **A:** `phoenix.lua` recuerda la última recarga. No toca los parches.
- **B:** experimento sin el parche C, para que el juego se quede en «Activar» de verdad.

FRALEX eligió: «guarda primero, luego empezamos con la A y luego hacemos la B para experimentar».

---

## 9. Paso 7 · Guardar (23:07–23:15)

1. **Problema:** desde el PC no había credenciales para subir a GitHub.
2. **Solución:** se conectó el repo `Fralexito/smash-soda-fork` a la sesión de Claude y se clonó la rama `mercado-fase0`
   en el espacio de trabajo de la nube. El PC no se tocó.
3. Los guiones se pasaron del PC a la nube empaquetados, y se comprobó la huella de cada uno al llegar.
4. Se escribieron o ampliaron:
   - `base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` (nuevo);
   - `PRUEBAS.md` (4 entradas añadidas al final);
   - `REGISTRO.md` (1 línea añadida);
   - `base-conocimiento/00-INDICE.md` (1 línea añadida);
   - `sider/pruebas/` y `herramientas/exe/` (carpetas nuevas).
5. Se volvió a pasar la simulación con los archivos ya guardados: bien.
6. `git fetch`, `git rebase`, commit `ff88f3d` y `git push`. Se comprobó con `git ls-remote` que GitHub tenía ese commit.
7. **Google Drive:** documento nuevo «🔎 «Datos Actual. en vivo»: dónde guarda el juego la opción (9 oct 2026, 23:15)»,
   en la misma carpeta que los demás informes. Se volvió a abrir para comprobar que el contenido estaba completo.

---

## 10. Paso 8 · Opción A: la versión v0.17a (23:15–23:31)

### 10.1 La idea

Cada vez que el juego **empieza** a leer `player.bin`, empieza una recarga de la base. En ese momento se mira el modo:

| Modo leído | Se apunta | Significa |
|---|---|---|
| 1 | ACTIVAR | Equipos de la base (`PlayerAssignment.bin`) |
| 0, sin un 1 en los últimos 15 segundos | NORMAL | Equipos del option file |
| 0, dentro de esos 15 segundos | Nada | Es la segunda recarga del mismo Activar |
| Otro valor | DESCONOCIDA | Por si aparece el modo 2 |
| No se pudo leer | Nada | No se inventa un dato |

### 10.2 Cómo se construyó

1. Se partió de la v0.17m.
2. Se añadió el bloque «última recarga» (`anotarRecarga`, `ultimaFueActivar`, `textoUltima`).
3. Tres cambios pequeños: versión `0.17a`, la muestra de `player.bin` ahora alimenta `anotarRecarga`,
   y la línea **[ÚLTIMA RECARGA]** al final del overlay.
4. Se comprobó antes que `os.time()` existe en el Lua de Sider (otros módulos ya lo usan).

### 10.3 La simulación (con reloj falso)

| Caso | Resultado esperado y obtenido |
|---|---|
| A1. Arranque, Activar, Activar | sin ver → NORMAL → ACTIVAR → ACTIVAR |
| A2. Activar, Editar → Cargar un minuto después, segunda recarga lenta (16 s) | ACTIVAR → NORMAL → NORMAL |
| A3. Exe distinto | No lee y no apunta nada |
| A4. Módulo recargado (Shift+R) | Empieza sin memoria; luego NORMAL |
| A5. Modo 2, reloj hacia atrás, gestor inexistente | DESCONOCIDA → NORMAL → no cambia |

Además se repitieron los 6 casos de la v0.17m con la v0.17a: bien. Escrituras: 0.
Guion: `sider/pruebas/simular_v017a.py`.

### 10.4 Instalación (23:31)

1. FRALEX dio el «sí».
2. Juego cerrado: Sider anotó «All done» a las 23:31:02.
3. Respaldo de la v0.17m como `phoenix.lua.v017m`, en las dos carpetas.
4. Copia de la v0.17a a las dos carpetas.
5. Huella comprobada en el PC: `379dce7b…3306c` en las dos. Respaldos `v017` y `v017m` intactos.

### 10.5 Límites conocidos de la opción A

- No ve las recargas que no releen la base (tecla L, «Ser una Leyenda»).
- Tras Shift+R empieza sin memoria hasta la siguiente recarga del juego.
- La ventana de 15 segundos es una elección: las dos recargas de Activar se separan unos 3 segundos.

**Resultado en el juego:** se anota en `PRUEBAS.md` cuando FRALEX haga la prueba.

---

## 11. Lo que falta

1. Probar la v0.17a en el juego: Activar → debe decir ACTIVAR; Editar → Cargar → debe decir NORMAL.
2. **Opción B (experimento):** sin el parche C, ¿el modo se queda en 1 y la pantalla dice «Selección actual: Activar»?
   ¿Siguen llegando las stats y los fichajes?
3. Confirmar el valor 2 (¿Valoraciones generales uniformes?).
4. **Fase D:** saber desde Lua que se está en el menú principal y no en un partido ni en Editar.
5. Actualizar la carpeta `C:\dev\smash-soda-fork` del PC (`git pull --rebase`), con el visto bueno de FRALEX.

---

## 12. Cómo repetirlo en otro PC o en otro parche

1. Cerrar el juego. Hacer una foto de la carpeta de guardado con `herramientas/exe/foto_guardado.sh`.
2. Abrir el juego, pulsar Activar, cerrar. Otra foto. Comparar. (Aquí: ningún cambio.)
3. Copiar `PES2021.exe` a una carpeta de trabajo y comprobar su huella.
   - Si es `5e27a782…bc224a`, las direcciones de este informe valen.
   - Si es otra, repetir el rastro del apartado 6.2 con `lib_exe.py`, empezando por buscar los textos con «LiveData».
4. Simular el Lua antes de instalarlo (`simular_v017m.py`, `simular_v017a.py`).
5. Respaldar, instalar con el juego cerrado y comprobar la huella en el PC.
6. Pulsar Activar y leer en `sider.log` las líneas `[phoenix] modo` y `[phoenix] última recarga`.

---

## 13. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Resultado resumido | `base-conocimiento/17-OPCION-EN-VIVO-DONDE-ESTA.md` |
| Este informe | `base-conocimiento/19-OPCION-EN-VIVO-PASO-A-PASO.md` |
| Diario de pruebas | `PRUEBAS.md`, entradas del 2026-10-09 desde las 21:09 |
| Versión estable | `sider/phoenix.lua` = v0.17 (`d85e1071…`), sin cambios |
| Prueba «mirar la opción» | `sider/pruebas/phoenix-v0.17m-prueba.lua` (`57ff5ec0…`) |
| Prueba «última recarga» (instalada ahora) | `sider/pruebas/phoenix-v0.17a-prueba.lua` (`379dce7b…`) |
| Simulaciones | `sider/pruebas/simular_v017m.py` y `simular_v017a.py` (necesitan `pip install lupa`) |
| Lector del exe | `herramientas/exe/lib_exe.py` (necesita `pip install capstone numpy`) |
| Guion de las fotos | `herramientas/exe/foto_guardado.sh` |
| Respaldos en el PC | `phoenix.lua.v017` y `phoenix.lua.v017m` en las dos carpetas `modules` |

**Para volver a la v0.17:** con el juego cerrado, copiar `phoenix.lua.v017` encima de `phoenix.lua` en las dos carpetas `modules`.
