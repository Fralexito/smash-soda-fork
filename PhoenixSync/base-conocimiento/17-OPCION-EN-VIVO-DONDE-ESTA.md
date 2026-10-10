# 17 · «Datos Actual. en vivo»: dónde guarda el juego si la opción está activada

**Investigación del 2026-10-09, de 21:04 a 23:10 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. Amplía el [07 · El motor por dentro](07-MOTOR-POR-DENTRO.md).

---

## 0. Resumen en 5 líneas

1. La opción **no se guarda en disco**. Ningún archivo de la carpeta de guardado cambia al abrir el juego, al pulsar Activar o Desactivar, ni al cerrar.
2. La opción vive **en la memoria**: es un número dentro del «gestor de la base» del juego. **1 = activada, 0 = desactivada.**
3. `phoenix.lua` ya lo puede leer de forma segura (solo lectura, con comprobación previa del código). Probado 3 veces.
4. Con la v0.17, el número vale 1 **solo unos 3 segundos** al pulsar Activar. Luego nuestro parche C lo devuelve a 0. Por eso la pantalla dice siempre «Selección actual: Desactivar».
5. Al reiniciar el juego, la opción vuelve sola a «Desactivar».

---

## 1. Palabras nuevas

- **Huella (sha256):** código que se calcula a partir de un archivo. Si el archivo cambia aunque sea en 1 byte, la huella cambia.
- **Foto:** calcular la huella de todos los archivos de una carpeta en un momento dado, para comparar después.
- **RVA:** posición dentro del `PES2021.exe`. Se escribe `exe+0x…`.
- **Puntero:** un dato que guarda la dirección de otro dato.
- **Gestor de la base:** el objeto del juego que carga los archivos de datos (`Player.bin`, `PlayerAssignment.bin`…).
- **Modo de carga:** el número que el gestor guarda para saber si la actualización en vivo está aplicada.
- **Copia segura:** leer la memoria con `ReadProcessMemory`. Si la zona ya no existe, la lectura falla sin cerrar el juego.

---

## 2. El resultado: dónde está el dato

| Qué | Dónde | Cómo se sabe |
|---|---|---|
| Puntero al gestor de la base | `exe+0x3705E10` (8 bytes) | Lo devuelve la función `0x14B6A60`: `mov rax, [exe+0x3705E10] ; ret` |
| **Modo de carga (la opción)** | **número de 4 bytes en `[gestor + 0x38]`** | Lo escribe `0x14B7560`. El juego lo compara con 1 en 12 sitios |
| Estado del gestor | 4 bytes en `[gestor + 0x30]` | 4 = quieto, 1 = cargando (visto en el juego) |
| Bandera «carga terminada» | byte `[gestor + 0x91]` | 0 mientras carga, 1 al terminar (visto en el overlay; 🔎 observado) |

**Valores del modo de carga:**

| Valor | Significa | Estado |
|---|---|---|
| 0 | Desactivada | ✅ visto en el juego, coincide con «Selección actual: Desactivar» |
| 1 | Activada | ✅ visto en el juego durante Activar (3 de 3). En pantalla no se llega a ver, porque dura 3 segundos |
| 2 | ¿Valoraciones generales uniformes? | ❓ hipótesis. El código usa el valor 2 en otro camino (`0x20AE705`). Sin probar |

### Cómo leerlo con seguridad (así lo hace la v0.17m)

1. Comprobar estos bytes del código. Si alguno no coincide, **no leer nada**:

| RVA | Bytes (hex) | Qué es |
|---|---|---|
| `0x14B6A60` | `48 8B 05 A9 F3 24 02 C3` | `mov rax, [exe+0x3705E10] ; ret` |
| `0x13040C9` | `E8 92 29 1B 00 83 78 38 01 74 34` | `call 0x14B6A60 ; cmp dword [rax+0x38], 1 ; je` |
| `0x14B7587` | `44 89 41 38 45 85 C0 75 07` | `mov [rcx+0x38], r8d ; test r8d, r8d ; jne` |

2. Leer 8 bytes en `exe+0x3705E10` con copia segura. Si valen 0, el gestor todavía no existe: no leer más.
3. Comprobar que el puntero es razonable (ni muy bajo ni fuera del espacio de usuario).
4. Leer con copia segura el trozo `[gestor+0x30 … +0x93]` a un búfer propio.
5. El modo es el número de 4 bytes en la posición `+0x38`.

Nunca se escribe en la memoria.

---

## 3. Por qué la pantalla dice «Desactivar» después de pulsar Activar

Con la v0.17 (parches A, B y C), pulsar Activar hace **dos recargas seguidas**:

| Paso | Qué hace el juego | Modo que queda |
|---|---|---|
| Estado 22 de `LiveDataSetFlow` (`0x20AE523`) | Llama a `0x14B7560(gestor, 0, 1)`: recarga toda la base en **modo 1**. Es el paso original de Konami | **1** |
| Estado 26 (`editLoadDataInLiveDataSet`) con el parche C | La tarea de carga del EDIT llama a `0x14B7560(gestor, 0, 0)` (`0x1EFAFB0`) cuando su bandera `[tarea+0x71]` no vale 0: recarga la base otra vez en **modo 0**. 🔎 Se deduce que esa bandera es el primer parámetro, el que el parche C pone a 1 | **0** |

Por eso el espía ve la base leída **dos veces**, con unos 3 segundos de diferencia, y el modo termina en 0.

**Consecuencia:** con la v0.17, el juego nunca se queda en «Activar». Para él, la opción está siempre desactivada.

### Lo que hace cada función (leído en el código, sin tocar el exe)

- `0x14B6A60` → devuelve el gestor de la base. La llaman 5.413 sitios del juego.
- `0x14B6A70(gestor)` → responde «¿está quieto?» (`[gestor+0x30] == 4`).
- `0x14B7560(gestor, bandera, modo)` → «recargar la base con este modo». Guarda el modo en `[gestor+0x38]` y pone el estado en 1.
  - La llaman 5 sitios: `0x20AE529` (modo 1), `0x20AE705` (modo 2), `0x1EFAFB0`, `0x13A4A5D` y `0xE9F708` (modo 0).
- `0x14B6F20(gestor, n)` → «quitar datos en vivo»: pone a 0 el byte `[gestor+0x90+n]`. La usa Desactivar (`LiveDataRemoveFlow`, `0x20AF320`) y otros 4 sitios, siempre solo si ese byte no vale 0.
- `ProcessLiveDataCheck` (`0x13040C0`): al entrar a Partido, si el modo ya es 1 sigue de largo. Si no, mira otro dato, el byte `[[exe+0x37F9648]+0xA]` (❓ parece la respuesta del jugador a «¿aplicar la actualización en vivo?»), y si vale 1 lanza el flujo de Activar.

### Por qué con Activar los equipos salen de la base (❓ hipótesis)

La tarea `editLoadDataInLiveDataSet` recibe los parámetros `0 (1 con el parche C), 1, 1, 0, 0`. Editar → Cargar usa `1, 1, 0, 0, 0`.
La diferencia es el **tercer parámetro**. Lo más probable es que signifique «no apliques las plantillas del option file».
No está comprobado.

---

## 4. Fase A · ¿Se guarda en disco? → No

Carpeta activa: `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
Cada foto incluye todos los archivos de esa carpeta, las otras carpetas de guardado (`292733975847239680`, `48`, `SP`) y `settings.dat`.

| Hora | Momento | ¿Cambió algo? |
|---|---|---|
| 21:09 | Foto inicial | — |
| 22:02 | Juego cerrado (confirmado por FRALEX) | No |
| 22:10 | Juego abierto (arrancó a las 22:07) | No |
| 22:23 | Un minuto después de Activar (22:22) | No |
| 22:31 | Nueve minutos después de Activar | No |
| 22:32 | Juego cerrado (Sider anotó el cierre a las 22:31:48) | No |
| 22:39 | Juego abierto de nuevo y Desactivar pulsado | No |
| 22:41 | Después de otro Activar (22:41) | No |
| 22:44 | Después de otro Desactivar | No |

- En la carpeta del juego, en ese tiempo, solo cambiaron los diarios de Sider.
- **Al reiniciar, la opción sale en «Desactivar»** aunque se hubiera pulsado Activar antes de cerrar (visto una vez, 22:39).
- Pulsar Desactivar estando ya en «Desactivar» **no recarga nada** (22:44: ninguna lectura nueva de la base).
- La pantalla muestra el estado en la línea **«Selección actual: …»**. La opción tiene tres posiciones: Activar, Desactivar y Valoraciones generales uniformes.

---

## 5. Fase B y C · En la memoria → encontrado y confirmado

Método: primero se leyó el código del exe (copia de solo lectura, Python + capstone) hasta dar con un candidato. Después se
instaló `phoenix.lua` **v0.17m**, que es la v0.17 más una parte que **solo lee**:

- tecla **M**: toma una muestra y la anota en `sider.log`;
- línea **[MODO]** en el overlay;
- una muestra automática cada vez que el juego empieza a leer `player.bin`.

Antes de instalar se simuló en LuaJIT con memoria falsa (6 casos: normal, exe distinto, gestor inexistente, zona ilegible,
durante Activar y módulo recargado con Shift+R): 6 de 6 bien, 0 escrituras.

**Resultado en el juego (sesión que arrancó a las 23:00:09):**

| Hora | Lectura de `player.bin` | Modo leído |
|---|---|---|
| 23:00:09 | n.º 1, arranque | 0 |
| 23:01:54 | n.º 2, segunda pasada del arranque | 0 |
| 23:02:06 | n.º 3, Activar (primera recarga) | **1** |
| 23:02:09 | n.º 4, Activar (segunda recarga) | 0 |
| 23:04:06 | n.º 5, Activar | **1** |
| 23:04:09 | n.º 6 | 0 |
| 23:04:18 | n.º 7, Activar | **1** |
| 23:04:21 | n.º 8 | 0 |

- Tres repeticiones, las tres iguales. Ningún error de Lua. El juego siguió normal.
- A las 23:04:30 el overlay mostraba «[MODO] opción en vivo: DESACTIVADA (0) · gestor estado 4» y el juego «Selección actual: Desactivar». Coinciden.

---

## 6. Riesgos y otros parches

- **Otra versión del exe:** las direcciones cambian. La lectura comprueba antes los bytes del código; si no coinciden, no lee y muestra «exe distinto». Degrada sin romper nada.
- **Otro parche con el mismo exe** (por ejemplo Sudamerican, si usa el mismo `PES2021.exe`): debería funcionar igual, porque el dato es del juego y no del parche. Sin probar.
- **El gestor puede no existir** muy al principio del arranque: la lectura lo detecta (puntero 0) y no lee.
- **Exe estudiado:** 458.806.784 bytes, sha256 `5e27a78246dbe4f1e66cb60ec56e86a739303708b02d4048f6fec6b570bc224a`.
- **Secciones:** `.trace` (código) en RVA `0x1000`; `.rdata` en `0x252F000`; `.data` en `0x339B000` (tamaño en memoria `0x72A000`); `.bss` en `0x3C98000`; `.data1` en `0x3CA1000`.

---

## 7. Propuesta para phoenix.lua (sin instalar; pendiente de aprobación)

**Opción A · Recordar la última recarga (no toca los parches).**
Cada vez que el juego empieza a leer `player.bin`, phoenix.lua lee el modo:

- si vale **1** → apunta «última recarga: ACTIVAR» y la hora;
- si vale **0** y no hubo un 1 en los últimos 15 segundos → apunta «última recarga: normal» (arranque, Editar → Cargar o menú principal).

Con eso sabe en qué situación está el jugador, sin escribir nada.
Límites: si se recarga el módulo con Shift+R, pierde la memoria hasta la siguiente recarga del juego.

**Opción B · Experimento sin el parche C.**
Aplicar solo A y B. La recarga del estado 22 ya relee toda la base en modo 1, así que el modo se quedaría en 1 y la pantalla
debería decir «Selección actual: Activar». Dudas a comprobar: si las stats y los fichajes siguen llegando, y cómo se portan
otros menús con la opción activada de verdad. El cambio vive solo en la memoria y desaparece al cerrar el juego.

**Pendiente (Fase D):** cómo saber desde Lua que se está en el menú principal y no en un partido ni en Editar.

---

## 8. Dónde está todo

| Qué | Dónde |
|---|---|
| Versión de prueba instalada el 2026-10-09 a las 22:57 | `sider/pruebas/phoenix-v0.17m-prueba.lua` (sha256 `57ff5ec0…52805`) |
| Simulación de la versión de prueba | `sider/pruebas/simular_v017m.py` (necesita `pip install lupa`) |
| Versión estable | `sider/phoenix.lua` = v0.17 (sha256 `d85e1071…bfc69c`), sin cambios |
| Respaldos de la v0.17 en el PC | `phoenix.lua.v017` en `SiderAddons\modules\` (raíz) y en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\` |
| Ayudantes para leer el exe | `herramientas/exe/lib_exe.py` (necesita `pip install capstone numpy`; se ejecuta junto a una copia de `PES2021.exe`) |
| Guion de las fotos del guardado | `herramientas/exe/foto_guardado.sh` |
| Diario de las pruebas | `PRUEBAS.md`, entradas del 2026-10-09 de 21:09 a 23:04 |

**Para volver a la v0.17:** con el juego cerrado, copiar `phoenix.lua.v017` encima de `phoenix.lua` en las dos carpetas `modules`.
