# 22 · Fase D: en qué parte del juego está el jugador y cuándo es seguro recargar

**Investigación del 2026-10-10, de 00:09 a 00:25 (Lima).** PC de FRALEX, PES 2021 con ConmeGOL Patch 26 y Sider 7.3.3.
Archivo nuevo. No sustituye a ningún otro. Continúa el [17](17-OPCION-EN-VIVO-DONDE-ESTA.md) y el [19 · paso a paso](19-OPCION-EN-VIVO-PASO-A-PASO.md).

---

## 0. Resumen en 4 líneas

1. El juego guarda en la memoria un «cartel» con el modo en el que está el jugador. Es un número del 0 al 60.
2. El propio exe trae la lista de nombres: **7 = TOP_MENU (menú principal)**, 8 = EXHIBITION (Partido), 13 = EDIT (Editar), 19 = UEFA_ML (Liga Máster)…
3. `phoenix.lua` v0.17d lo lee de forma segura y dice si es un **momento seguro para recargar**. Probado en el juego: acertó en todos los sitios.
4. Esta versión **solo mira y anota**. No recarga nada.

---

## 1. Palabras nuevas

- **Gestor de modos:** el objeto del juego que sabe en qué modo se está (menú, Partido, Editar…).
- **Zona:** nombre que usamos para «la parte del juego donde está el jugador».
- **Momento seguro:** instante en el que recargar los datos no puede estropear nada: menú principal, sin cargas en marcha y fuera de Editar.
- **Evento de Sider:** aviso que Sider manda a los módulos cuando pasa algo (por ejemplo, «se eligieron los equipos»).

---

## 2. Dónde está el dato

| Qué | Dónde | Cómo se sabe |
|---|---|---|
| Puntero al gestor de modos | `exe+0x3704E38` (8 bytes) | Lo devuelve `0x149A470`: `mov rax, [exe+0x3704E38] ; ret`. La llaman 2.107 sitios |
| **Modo actual** | **número de 4 bytes en `[gestor+0xF0]`** | Lo lee `0x149F4F0`, que tiene 997 llamadores |
| Cambio de modo en curso | byte `[gestor+0x12C]` distinto de 0 | `0x149F510`: si no vale 0, usa el modo de `[gestor+0x130]` |
| Modo siguiente | 4 bytes en `[gestor+0x130]` | Igual que arriba |
| Tabla de nombres | `exe+0x34FC9C0`, 61 entradas de 16 bytes | Cada entrada: tipo (4 bytes), relleno y puntero al nombre |

### La lista de modos (los que importan)

| N.º | Nombre en el exe | Qué es | Estado |
|---|---|---|---|
| 0 | UNKNOWN | Arrancando, todavía sin modo | ✅ visto |
| 5 | TITLE_DEMO_LOOP | Pantalla de título | ✅ visto |
| 6 | FIRST_SETTINGS | Carga inicial (aquí ocurre la segunda lectura de la base) | ✅ visto |
| **7** | **TOP_MENU** | **Menú principal** | ✅ visto |
| 8 | EXHIBITION | Partido | ✅ visto |
| 13 | EDIT | Editar | ✅ visto |
| 14 | EDIT_LIVEUPDATE | Editar → actualización en vivo | 🔎 leído en el exe |
| 19 | UEFA_ML | Liga Máster | 🔎 leído en el exe |
| 20 | UEFA_BL | Ser una Leyenda | 🔎 leído en el exe |
| 52 | FREE_TRAINING | Entrenamiento | 🔎 leído en el exe |

La lista completa (61 nombres) está dentro de `sider/pruebas/phoenix-v0.17d-prueba.lua`, en `NOMBRES_ZONA`.

---

## 3. La regla del «momento seguro»

`phoenix.lua` dice **SÍ** solo si se cumplen las cuatro condiciones a la vez:

| N.º | Condición | Dato que se lee |
|---|---|---|
| 1 | El modo es el menú principal | `[gestor de modos+0xF0] == 7` |
| 2 | No hay un cambio de modo en curso | `[gestor de modos+0x12C] == 0` |
| 3 | El gestor de edición no existe | `[exe+0x37F5C28] == 0` |
| 4 | La base está quieta | `[gestor de la base+0x30] == 4` |

Si falla una, dice **NO** y explica cuál. Si algún dato no se puede leer, también dice NO.

### Antes de leer, se comprueba el código

| RVA | Bytes (hex) | Qué es |
|---|---|---|
| `0x149A470` | `48 8B 05 C1 A9 26 02 C3` | `mov rax, [exe+0x3704E38] ; ret` |
| `0x149F4F0` | `8B 81 F0 00 00 00 33 C9 83 F8 3D 0F 43 C1` | `mov eax, [rcx+0xF0] ; xor ecx, ecx ; cmp eax, 0x3D ; cmovae eax, ecx` |
| `0x149F510` | `80 B9 2C 01 00 00 00 74 08 8B 81 30 01 00 00 EB 06` | `cmp byte [rcx+0x12C], 0 ; je ; mov eax, [rcx+0x130] ; jmp` |
| `0x1EF2250` | `48 8B 05 D1 39 90 01 C3` | `mov rax, [exe+0x37F5C28] ; ret` |

Si alguno no coincide, no se lee nada y el overlay dice «exe distinto».

---

## 4. Cómo se encontró

1. En el estudio de la noche anterior ya había aparecido esta pareja de llamadas: `call 0x149A470` y luego `call 0x149F4F0`,
   justo antes de elegir entre `LiveDataSetFlowOnMode`, `…Exhibition` y `…OffMode` (función `0xFDF9D0`). Olía a «¿en qué modo estoy?».
2. Se desensamblaron las dos: son lectores muy simples. La primera devuelve un objeto global; la segunda lee `[objeto+0xF0]`
   y lo traduce con la tabla de `exe+0x34FC9C0`.
3. Se volcó la tabla: 61 entradas, cada una con un puntero a un texto. Los textos son los nombres de los modos.
4. Al lado está `0x149F510`, que muestra que durante un cambio de modo el juego usa `[+0x130]` si `[+0x12C]` no vale 0.
5. Para Editar ya se conocía el gestor de edición (`[exe+0x37F5C28]`, documento 07).
6. Para «la base está quieta» se reutilizó el estado del gestor de la base (documento 17).
7. Eventos de Sider: se miraron los nombres dentro de `sider.dll` 7.3.3. Existen `set_teams` y `context_reset`; no existe ningún evento de «entrar a Editar».

---

## 5. La versión de prueba v0.17d

Es la v0.17a más:

- la línea **[ZONA]** en el overlay;
- una muestra automática de la zona, como mucho **una por segundo**, cuando el juego lee algún archivo (solo después del arranque);
- los eventos `set_teams` y `context_reset` de Sider, que **solo se anotan**: no devuelven nada, así que no cambian nada del juego;
- una línea en `sider.log` cada vez que la zona cambia.

No escribe en la memoria. No recarga nada. No cambia los parches A, B y C.

### Simulación (LuaJIT, memoria y relojes falsos)

| Caso | Resultado |
|---|---|
| D1. Menú → Partido → Editar → Liga Máster → menú | SÍ, NO, NO, NO, SÍ |
| D2. Menú principal pero base cargando, edición activa o cambio de modo | NO las tres veces; luego SÍ |
| D3. Exe distinto | No lee la zona nunca más |
| D4. Gestor de modos inexistente, puntero raro, zona ilegible | Avisa y no lee |
| D5. Eventos de Sider | No devuelven nada |
| D6. Ritmo: 200 archivos en el mismo segundo | Una sola muestra |
| D7. Gestor de la base ilegible | NO |

Más los 6 casos de la v0.17m y los 5 de la v0.17a: los 18 bien, 0 escrituras. Guion: `sider/pruebas/simular_v017d.py`.

### Instalación (00:18)

1. FRALEX dio el «sí» a las 00:14, pero el juego estaba abierto. Se esperó.
2. 00:17:45 · Sider anotó «All done»: juego cerrado.
3. Respaldo de la v0.17a como `phoenix.lua.v017a`, en las dos carpetas `modules`.
4. Copia de la v0.17d a las dos carpetas.
5. Huella comprobada en el PC: `9755c842…17ffad` en las dos. Respaldos `v017`, `v017m` y `v017a` intactos.

---

## 6. Resultado en el juego (00:19–00:22)

Recorrido de FRALEX: abrir el juego → menú principal → Partido → menú → Editar → salir sin Guardar → menú.

| Hora | Zona leída | ¿Seguro? | Nota |
|---|---|---|---|
| 00:19:36 | UNKNOWN (0), base estado 1 | NO | Arranque |
| 00:19:45 | TITLE_DEMO_LOOP (5) | NO | Pantalla de título |
| 00:19:49 | FIRST_SETTINGS (6) | NO | Carga inicial |
| 00:19:51 | — | — | Segunda lectura de la base (modo 0) |
| 00:19:59 | **TOP_MENU (7)** | **SÍ** | Menú principal |
| 00:20:00 | **EXHIBITION (8)** | NO | Partido |
| 00:20:05 | — | — | Evento `context_reset` de Sider |
| 00:20:09 | **EDIT (13)**, edición no | NO | Entrando a Editar |
| 00:20:10 | EDIT (13), edición ACTIVA | NO | Editar ya cargado |
| 00:20:17 | — | — | Lectura de la base (modo 0) → «última recarga: NORMAL» |
| 00:21:07 | **TOP_MENU (7)** | **SÍ** | De vuelta en el menú |

Captura de FRALEX a las 00:22:02, en el menú principal:
«[ZONA] TOP_MENU (7) · edición no · base estado 4 · momento seguro: SÍ».

- `phoenix.lua`: 0 errores. Ningún error de otros módulos en esta sesión.
- El option file sigue idéntico (huella `3507ce35…`).

### Lo que se aprendió

1. **El cartel funciona.** Cada nombre coincidió con el sitio donde estaba FRALEX.
2. **Solo dijo SÍ en el menú principal.** En Partido y en Editar dijo NO.
3. **La segunda lectura de la base del arranque ocurre en FIRST_SETTINGS.** Era la duda que quedó abierta en el documento 19.
4. **Al entrar a Editar, el cartel cambia antes que el gestor de edición** (un segundo de diferencia). Por eso conviene usar las dos señales.
5. **`SYSTEM00000000` cambia al guardar una carrera de Liga Máster.** Entre las 23:57 y las 00:11 aparecieron `ML00000003` y
   `ML00000004` (dos carreras de Liga Máster guardadas a esa hora), y `SYSTEM00000000` y `GRAPHICS000000` se reescribieron a las 00:11:41.
   No tiene que ver con «Datos Actual. en vivo».

### Límite de la prueba (no del dato)

La muestra automática solo ocurre cuando el juego lee algún archivo. Al volver de Partido al menú (00:20:05) el juego no leyó
nada durante 4 segundos, y esa vuelta no quedó anotada. Con el overlay abierto sí se mira todo el tiempo.
Para el uso real no importa: `phoenix.lua` leerá el cartel en el momento en que quiera recargar.

---

## 7. Lo que falta

1. Ver el cartel en Liga Máster (19), Ser una Leyenda (20), Entrenamiento (52) y **dentro de un partido** (¿sigue en 8?).
2. Repetir el recorrido otro día: hoy se hizo una vez.
3. Decidir, con FRALEX, cómo usar el «SÍ»: por ejemplo, recargar solo si la última recarga fue ACTIVAR y el momento es seguro.
   Eso ya no sería solo lectura y necesita su aprobación.
4. Experimento B (sin el parche C).

---

## 8. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Versión instalada ahora | `sider/pruebas/phoenix-v0.17d-prueba.lua` (`9755c842…`) |
| Simulación | `sider/pruebas/simular_v017d.py` (junto a `simular_v017m.py`; necesita `pip install lupa`) |
| Versión estable | `sider/phoenix.lua` = v0.17 (`d85e1071…`), sin cambios |
| Respaldos en el PC | `phoenix.lua.v017`, `.v017m` y `.v017a`, en las dos carpetas `modules` |
| Diario de pruebas | `PRUEBAS.md`, entradas del 2026-10-10 de 00:18 y 00:19 |

**Para volver atrás:** con el juego cerrado, copiar `phoenix.lua.v017a` (o `phoenix.lua.v017`) encima de `phoenix.lua` en las dos carpetas `modules`.
