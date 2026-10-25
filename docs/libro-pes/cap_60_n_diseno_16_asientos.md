# Capítulo 60 n — Diseño para pasar a 16 mandos el gestor de CURSORES y el gestor de FOCO de los menús (sin agrandar en sitio)

> 23-oct-2026. Ingeniería inversa **solo lectura** (Ghidra headless sobre `copias/exe_pes2021/PES2021_recortado.exe`; bytes con `pebytes.py`). No se abrió PES, no se tocó `Conmegol Patch` ni el módulo (solo se **leyó** `fase1.txt` y el módulo para ver cómo aplica parches).
> Salidas nuevas: `informes/datos/rea/diseno_01 … diseno_25` (lanzadores `dis1.bat … dis4.bat` en el scratchpad de la sesión). `diseno_23` = verificación con Ghidra del código máquina escrito a mano; `diseno_24` = tablas de parches con bytes; `diseno_25` (+ `diseno_25_ens.py`) = bytes del código propio y el script que los genera.
> **[CONFIRMADO]** = visto en código/bytes · **[HIPÓTESIS]** = deducido, falta prueba en vivo.
> Contexto: cap_60_m (inventario) y cap_60_k (23-oct, «número prestado», que sigue siendo el producto).

## 0. Respuesta corta

1. **La v0.9.22 no rompía por una carrera, sino por un byte que faltaba** [CONFIRMADO]. En el constructor del gestor de cursores, `mov esi,8` (`0x140E675D7`) se usa para el número de entradas **y también para el tamaño de cada una**: `lea edx,[rsi+0x50]` (`0x140E675DF`, `8D 56 50`) = 8 + 0x50 = 0x58. Con `esi = 16` el tamaño pasaba a **0x60**: el `eh_vector_constructor` montaba 16 entradas de 0x60 desde `+0x80` → hasta `+0x680`, **0x80 bytes más allá del bloque de 0x600** → montón roto → cierre en `FUN_1403FD830/FE800`. Arreglo: `8D 56 50` → `8D 56 48`. Además, `fase1.txt` prueba que en `m.init` (0,02 s) el gestor **aún no existía** (se parcheó porque `g == 0`) y PES cayó 12 s después.
2. **Gestor de cursores (PointingCursor)**: está colgado de la lista de tareas (`OtherRootTask`) y en el anillo de «tareas vivas»; el recorrido de cada cuadro (`FUN_14149FD10`) llama a `vtable+0` **del objeto viejo**. Diseño: **no moverlo de las listas**; crear un objeto nuevo de 0x640 en memoria propia, cambiar **solo el puntero a la vtable del viejo** por una vtable «apoderada» cuyo `+0` y `+8` llaman a las funciones del juego con el **nuevo**, y apuntar `DAT_1436F9BC8` al nuevo. Todo lo delicado (copiar, montar entradas 8–15, crear los 8 punteros Flash, parchear topes) se hace **dentro del hilo del juego**, en el primer cuadro, con un pequeño intérprete de operaciones escrito en código máquina (ya verificado con Ghidra).
3. **Gestor de foco (WindowManager)**: **sí es viable con solo topes y desplazamientos**, salvo **una** función que hay que reescribir (`FUN_140E718A0`, desenrollada a 8; 134 bytes nuevos ya escritos). `FUN_140E71BE0` (array `char[8]` en pila con comprobación de rango) **no** hace falta reescribirla: el array se mueve a la zona «home» libre de la propia función (`rsp+0x60`, 16 bytes). Nuevo objeto de **0x288** (16 pilas en `+0x08`, 16 focos en `+0x188`); 36 parches de uso + 8 de crear/destruir. Ningún llamador guarda el puntero: todos pasan por `FUN_140E71810`.
4. Orden: (0) lecturas → (1) instalar el ejecutor con lista vacía → (2) cursores a 16 → (3) foco a 16 → (4) dueños ≥ 8 y bucles «primer mando» → (5) prueba funcional sin «número prestado». Cada paso es un arranque de PES y tiene su comprobación (canarios, `RESULT`, Visor de eventos).

---

## 1. PointingCursor (`menusystem::PointingCursor`, singleton `[0x1436F9BC8]`)

### 1.1 Cuándo se crea [CONFIRMADO salvo lo marcado]
- Se crea **de forma perezosa** en `FUN_140E679E0` la primera vez que alguno de sus 15 llamadores lo pide (inventario_05): colocar cursor en un elemento `FUN_140E39DA0`, «qué hay bajo el cursor» `FUN_140E3DCA0`, `FUN_140E3A7A0`, `FUN_140E3BB90`, `FUN_140E44350`, el gestor de foco al perder/cambiar foco (`FUN_140E71620`, `FUN_140E71E10`, `FUN_140E71BE0`), la pantalla de estrategia (`FUN_140901C50`, `…29B0`, `…3230`, `…4B00`, `…54D0`, `…18890`) y `FUN_140DE5080`.
- Requisito: que exista la tarea raíz 2 (`[DAT_143704E88+0x28+2·8]` = `"OtherRootTask"`), creada en `FUN_140403860` al arrancar el juego (diseno_11).
- **Momento real**: después de que Sider carga los módulos. En `fase1.txt` (v0.9.22, 02:05:40) el módulo escribió los parches a los 0,02 s, y solo lo hacía si `[0x1436F9BC8] == 0`; PES cayó a las 02:05:52 (12 s). Igual en el segundo arranque (02:06:32 → 02:06:43). → El gestor se crea entre 0 y ~12 s tras cargar los módulos, en los primeros menús (portada). Qué llamador es exactamente el primero: [HIPÓTESIS] uno de los de ventanas (`FUN_140E39DA0`/`FUN_140E71E10`) en la portada; se puede fijar con una lectura en vivo de `[0x1436F9BC8]` cada cuadro.
- **Consecuencia para el diseño**: a los ~8 s puede que exista o no. El módulo debe **esperar a que exista** (`[0x1436F9BC8] ≠ 0`) antes del paso 1.

### 1.2 Dónde más está su puntero (FUN_14149FC70 y la lista de tareas) [CONFIRMADO]
| Sitio | Qué es | Fuente |
|---|---|---|
| `DAT_1436F9BC8` | el singleton (solo lo leen/escriben `FUN_140E679E0` y el destructor) | inventario_09 |
| Anillo de hijos de `OtherRootTask` | `FUN_14149FC70(padre, obj)`: `obj+0x38` = padre, inserta en el anillo doble de hermanos `+0x48` (anterior) / `+0x50` (siguiente), ordenado por prioridad `+0x30`; si es el primero, `padre+0x40` = obj. Pone `obj+0x28 = 0` | diseno_01/inventario_06 |
| Anillo de «tareas vivas» del gestor | el constructor de `sys::TaskUnit` (`FUN_14149FA80`) llama a `FUN_14149B460(gestor, obj)` → `FUN_14149FC50(gestor+0x48 cabeza, obj)`: enlaces `+0x58/+0x60` | diseno_01/11 |
| Recorrido de cada cuadro | **`FUN_14149FD10`**: copia los hijos en una cola temporal (`FUN_14149FE80`) y, para cada uno con `+0x2C == 0` y `+0x28 == 1`, llama a **`(*obj->vtable)(obj)`** y luego recursa | diseno_22 |
| Nadie más | la vtable `0x1427BE7E0` solo la usan el ctor y el dtor; `FUN_140E67CB0` solo la llama el ctor | diseno_18/19 |

**Destrucción** [CONFIRMADO el mecanismo; HIPÓTESIS el momento]: «matar» = `vtable+0x20` → `FUN_1414A0130` → `FUN_1414A02B0`: avisa al padre, se desengancha del anillo de hermanos, mata a sus hijos (`FUN_1414A0190`), llama a `vtable+8` (**terminar** = `FUN_140E678F0`, libera los nodos de los punteros), sale del anillo de vivos (`FUN_14149BAD0`) y entra en la papelera (`FUN_14149B4B0`, anillo `gestor+0x50`); después se llama a `vtable+0x18` = `FUN_140E677F0` (pone `DAT_1436F9BC8 = 0` y libera 0x340 en el montón 0x13). No se encontró ningún «matar» dirigido al cursor: muere con su padre `OtherRootTask`, [HIPÓTESIS] solo al cerrar PES. Si alguna vez muere antes, `FUN_140E679E0` lo vuelve a crear (por eso también se parchea la creación, §1.4).

### 1.3 Todo lo que usa la tabla de 8 (`+0x80`, 0x58 por entrada) [CONFIRMADO]
Funciones con índice de mando `N·0x58` (diseno_15): `FUN_140E67970`, `67A70`, `67AA0`, `67B30`, `67B40`, `67B50`, `67B70`, `67B90`, `67BE0`, `67C40`, `67C70`, `67D50`, `67D70` (las 13 con `AND 7` de la v0.9.24), **`67EA0`** (solo lectura, `cmp [..+0xBC],0`; no estaba en la lista), `67EC0`, `67EE0`, `68370`. Bucles: ctor `67580`, cada cuadro `67870`, terminar `678F0`, crear punteros `67CB0`, destructor `677F0`. Los 15 llamadores solo usan estas funciones (no tocan la tabla directamente: diseno_10).

**Parches del cursor (bytes del exe; cambian 1 byte cada uno)** — tabla completa en `diseno_24`:
| Dir. | Función | Original | Nuevo | Qué |
|---|---|---|---|---|
| `0x140E67A24` | 679E0 | `BA 40 03 00 00` | `BA 40 06 00 00` | tamaño al crear 0x340 → **0x640** (basta 1 byte; ≥ 0x600) |
| `0x140E675D7` | 67580 | `BE 08 00 00 00` | `BE 10 00 00 00` | 16 entradas |
| `0x140E675DF` | 67580 | `8D 56 50` | `8D 56 48` | **tamaño de entrada sigue en 0x58** (lo que faltó en la v0.9.22) |
| `0x140E678DB` | 67870 | `83 FF 08` | `83 FF 10` | cada cuadro |
| `0x140E67906` | 678F0 | `BF 08 00 00 00` | `BF 10 00 00 00` | terminar |
| `0x140E67CC6` | 67CB0 | `BE 08 00 00 00` | `BE 10 00 00 00` | crear punteros Flash |
| `0x140E67EC3` | 67EC0 | `83 FA 08` | `83 FA 10` | «¿activo?» |
| `0x140E682C1` | 67EE0 | `41 83 FE 08` | `41 83 FE 10` | mover con el mando N |
| `0x140E6782E` | 677F0 | `44 8D 42 B0` | `44 8D 42 B8` | destructor: 16 |
| `0x140E67846` | 677F0 | `BA 40 03 00 00` | `BA 40 06 00 00` | cosmético (la liberación ignora el tamaño) |
| 13 sitios `AND 7` (dir. de `PARCHES_CURSOR_AND7`) | 67970…67D70 | `83 E2 07 …` | `83 E2 0F …` | byte `dir+2`: 07 → 0F. Si la v0.9.24 aún no se aplicó, escribir la versión `AND 7` entera con `0F` en el tercer byte |
| `0x140E67EA0` (opcional) | 67EA0 | `8B C2 48 6B D0 58` | `83 E2 0F 6B D2 58` | acota también la lectura |

### 1.4 Procedimiento seguro recomendado: «vtable apoderada + ejecutor en el hilo del juego»
Mejor que la opción (a) del enunciado (sustituir el puntero en las listas): el objeto **sigue registrado tal cual**; solo cambia su vtable. Así no hay que tocar los dos anillos ni el `padre+0x40`, y la cola temporal del recorrido (que guarda el viejo) no importa.

**Memoria propia** (VirtualAlloc, el módulo ya lo hace):
- Página `P` de 0x3000, `PAGE_EXECUTE_READWRITE`:
  - `P+0x000` u32 `PEND` (1 = hay lista que ejecutar) · `P+0x004` u32 `RESULT` (0xFFFFFFFF pendiente, 0 bien, k = falló la comprobación k) · `P+0x008` u64 `OPS` (lista a ejecutar) · `P+0x010` u64 `CUR` (objeto que usan los stubs; al principio = viejo) · `P+0x018` 8 bytes temporales (punto central).
  - `P+0x040` copia de `[0x1427BE7D8]` (RTTI, vtable[-1]) · `P+0x048` **vtable apoderada** (6 entradas): `+0` = `P+0x100` (stub_upd), `+8` = `P+0x180` (stub_term), `+0x10` = `0x140CA1A50`, `+0x18` = `0x140E677F0` (destructor original, sobre el viejo), `+0x20` = `0x1414A0130`, `+0x28` = `0x141794410` (copiar las 6 de `0x1427BE7E0` y cambiar solo `+0` y `+8`).
  - `P+0x080` stub `wm_dtor` · `P+0x088` copia de `[0x1427C1560]` · `P+0x090` vtable del foco nuevo (1 entrada = `P+0x080`) — ver §2.
  - `P+0x100` stub_upd (81 B) · `P+0x180` stub_term (26 B) · `P+0x200` intérprete `M` (167 B) · `P+0x300` `FUN_140E718A0` nueva (134 B).
  - `P+0x400` lista A (cursor) · `P+0x1000` lista B (foco) · `P+0x2000` 0x100 bytes a cero.
- `NUEVO_CUR` = VirtualAlloc 0x1000 RW: objeto en `+0…+0x63F`; **canario** `0xCD` en `+0x640…+0xFFF`.

**Código propio** (bytes exactos en `diseno_25`; desensamblado con Ghidra en `diseno_23`, [CONFIRMADO] que decodifica como se diseñó; falta ejecutarlo):
```
stub_upd (vtable+0, rcx = viejo):            stub_term (vtable+8):
  mov rax,P ; cmp dword [rax],0 ; jne do       mov rax,P ; mov rcx,[rax+0x10]
go: mov rcx,[rax+0x10]                         mov rax,0x140E678F0 ; jmp rax
  mov rax,0x140E67870 ; jmp rax
do: push rcx ; sub rsp,0x20                  wm_dtor (vtable+0 del foco nuevo):
  mov rcx,[rax+8] ; mov rax,P+0x200 ; call rax   mov rax,rcx ; ret   (no libera: es VirtualAlloc)
  mov rdx,P ; mov [rdx+4],eax ; mov dword [rdx],0
  add rsp,0x20 ; pop rcx ; mov rax,rdx ; jmp go
```
**Intérprete M** (`rcx` = lista; operaciones de 32 bytes `{u32 tipo, u32 n, u64 a, u64 b, u64 c}`):
`1 COMPROBAR` (n ≤ 16 bytes en `+0x10` deben estar en `[a]`), `2 PARCHEAR` (escribe esos n bytes en `[a]`), `3 COPIAR` (`a` ← `b`, n bytes), `4 LLAMAR` (`a(rcx=b, rdx=c)`), `5 GUARDAR64` (`[a] = b`), `0 FIN`. Primera pasada: solo las COMPROBAR; si una falla devuelve su número y **no cambia nada**. Segunda pasada: el resto en orden. Devuelve 0.

**Lista A (cursor a 16)** — la construye Lua; todo se ejecuta en el hilo del juego, dentro de `vtable+0` del viejo (ningún código del gestor de cursores ni del de foco está a medias en ese momento):
1. COMPROBAR `[0x1436F9BC8]` = `VIEJO` (8 bytes) y `[VIEJO]` = `P+0x48`.
2. COMPROBAR los bytes originales de todos los sitios de §1.3 (los `AND 7` con lo que haya: original o `83 E2 07`).
3. COPIAR `NUEVO ← VIEJO`, 0x340. GUARDAR64 `[NUEVO] = 0x1427BE7E0`.
4. Para i = 8…15, `E = NUEVO+0x80+i·0x58`: LLAMAR `0x140E67720(E)` (pone a cero y llama a `FUN_140E67DD0`: nodo 0, Flash 0, estado 5, velocidades 1.0, banderas `0x100`); LLAMAR `0x140E67940(NUEVO+0x70, P+0x18)` (centro del rectángulo de pantalla); COPIAR `E+0x14 ← P+0x18`, 8 (posición inicial = centro, como hace el ctor).
5. PARCHEAR todos los sitios de §1.3 (incluidos tamaño y ctor, para que una recreación posterior también sea de 16).
6. GUARDAR64 `[0x1436F9BC8] = NUEVO`; GUARDAR64 `[P+0x10] = NUEVO`.
7. LLAMAR `0x140E67CB0(NUEVO)` → crea los punteros Flash «cmnPointer» y los nodos de menú **solo** de las entradas con nodo 0 (8–15), los deja en estado 3 (ocultar) → el cuadro siguiente pasan a 5.
8. FIN.

**Armar desde Lua** (cualquier hilo): (i) `VirtualProtect(0x40)` de las páginas de código `0x140E39000`, `0x140E3D000`, `0x140E67000`, `0x140E68000`, `0x140E71000`; (ii) escribir página, listas y canarios; (iii) comprobar `[VIEJO] == 0x1427BE7E0` y escribir `[VIEJO] = P+0x48` (escritura de 8 bytes alineada = atómica); (iv) `OPS = P+0x400`, `RESULT = 0xFFFFFFFF`, y **por último** `PEND = 1`. (v) Esperar `RESULT`; volver a `PAGE_EXECUTE_READ (0x20)`.

**Quién libera qué** [CONFIRMADO por el código; HIPÓTESIS que solo ocurre al salir]:
- Viejo (montón del juego): sigue siendo la tarea. Al morir: `vtable+8` = stub_term → `FUN_140E678F0(NUEVO)` libera los **16** nodos; `vtable+0x18` = destructor original sobre el viejo → `DAT_1436F9BC8 = 0` y libera el viejo. Correcto.
- Nuevo (VirtualAlloc): **nunca** lo libera el juego (nadie llama a su destructor). Se pierden 4 KB al morir la tarea. No llamar nunca a `FUN_140E677F0(NUEVO)`: su cabecera `TaskUnit` es copia de la del viejo y `FUN_14149FB40` desengancharía enlaces ajenos.
- Si luego se recrea (global a 0), sale de 16 y coherente gracias a los parches de tamaño/ctor/dtor.

**Riesgos**: (1) si el recorrido no llama nunca a la tarea (`+0x28 ≠ 1`), la lista no se ejecuta → `RESULT` queda en `FFFFFFFF` (inocuo; se deshace volviendo a escribir `[VIEJO] = 0x1427BE7E0`). (2) Un llamador que tenía el puntero viejo en un registro justo durante el cambio escribe en el viejo (memoria válida, no se libera): como mucho un fallo visual de un cuadro. (3) La tabla `PARCHES` del módulo espera `83 E2 07` en los 13 sitios: tras el paso 2, `aplicarParches(true)` diría «bytes distintos» → actualizar esa tabla (o no deshacer esos 13).

**Vía alternativa más simple, si se acepta parchear al cargar el módulo** [HIPÓTESIS de que funcione; el fallo conocido ya está explicado]: la v0.9.22 **con** el `8D 56 48` añadido (y el tamaño 0x640) aplicada en `m.init`, cuando el gestor aún no existe (comprobado en `fase1.txt`). Riesgo residual: que PES cree el gestor justo durante la escritura (µs). No sirve para el foco (creado al arrancar el juego).

---

## 2. WindowManager (`menusystem::WindowManager`, `DAT_1436F9C10`, 0x148)

### 2.1 Quién lo usa [CONFIRMADO]
- El global solo lo tocan `FUN_140E71810` (dame), `FUN_140E71570` (crear, desde `FUN_140E36AD0` ← `FUN_140403860`, al arrancar) y `FUN_140E71A80` (borrar, desde `FUN_140E36D50` ← `FUN_140403FE0`, cierre de la GUI).
- Los 21 llamadores de `FUN_140E71810` + los 2 sitios sueltos `0x140E3ECAE`/`0x140E3ED35` piden el gestor **cada vez** y solo llaman a su API (`71260`, `71490`, `71520`, `716A0`, `71820`, `718A0`, `719B0`, `719E0`, `71BE0`): **nadie lee sus campos ni guarda el puntero** (diseno_09, diseno_16). Su vtable `0x1427C1568` solo la usan ctor/dtor (diseno_21).
- Función que faltaba en el inventario: **`FUN_140E719E0`** (llamada por `FUN_140E3FA80` y `FUN_140E42920`): usa el dueño de la ventana como índice de foco **sin tope**.
- Elementos: pila = `std::vector` de pares {ventana, nivel} (0x18, cero = vacía, `FUN_1414D0670`); foco = {ventana u64, nivel u32} (0x10, cero = vacío, `FUN_140846490`) (diseno_17).

### 2.2 Nuevo objeto de 16: **0x288** bytes [CONFIRMADO el cálculo]
`+0x000` vtable · `+0x008…+0x187` 16 pilas × 0x18 (las 8 primeras en su sitio de siempre) · `+0x188…+0x287` 16 focos × 0x10 (ventana `+0x188+N·0x10`, nivel `+0x190+N·0x10`).
Las pilas se quedan en `+0x08` (sus desplazamientos son de 1 byte, `+0x08/+0x10/+0x18`, y no se podrían ampliar); los focos se mueven +0xC0, y **todos** sus accesos usan desplazamiento de 4 bytes o el truco `(N+0xD)·16` con un inmediato de 1 byte → se cambian sin cambiar el largo de ninguna instrucción.

### 2.3 ¿Solo topes y desplazamientos? **Sí, menos una función** [CONFIRMADO]
| Función | Cambios (dir.: original → nuevo) |
|---|---|
| `71260` dar foco | `0x140E712A6` `4C 8D B1 C8 00 00 00`→`…88 01…` · `0x140E712D0`/`0x140E71338` `83 FD 08`→`10` · `0x140E71346` `41 83 FC 07`→`0F` · `0x140E713F8` `49 8D 9D D0 00 00 00`→`…90 01…` · `0x140E71457` `83 FE 08`→`10` |
| `71490` pedir foco | `0x140E714D5` `41 83 FA 08`→`10` |
| `71520` contar | `0x140E71526` `83 FA 08`→`10` |
| `71620` (sin tope) | `0x140E7162F` `48 89 94 C1 C8 00 00 00`→`…88 01…` · `0x140E71637` `48 8D 43 0D`→`19` |
| `71820` | `0x140E71827` `83 FA 08`→`10` |
| `719B0` ¿tiene foco? (el freno de la lista de equipos) | `0x140E719B3` `48 8D 81 C8 00 00 00`→`…88 01…` · `0x140E719CC` `41 83 F8 08`→`10` |
| `719E0` | `0x140E71A2F` `48 8B 94 CF C8 00 00 00`→`…88 01…` · `0x140E71A46` `48 83 C0 0D`→`19` |
| `71AE0` quitar de pila N | `0x140E71B09` `83 FB 08`→`10` · `0x140E71B90` `4A 39 94 18 C8 00 00 00`→`…88 01…` · `0x140E71B9C` `4A 89 8C 18 C8 00 00 00`→`…88 01…` · `0x140E71BA4` `48 8D 43 0D`→`19` |
| `71BE0` quitar de todas | `0x140E71C22` `48 8D 8D C8 00 00 00`→`…88 01…` · `0x140E71C42` `83 FA 08`→`10` · `0x140E71C90` `49 83 FA 08`→`10` (comprobación de rango) · **array a la zona home**: `0x140E71C9A` `46 88 64 14 20`→`…60`, `0x140E71CDF` `42 C6 44 14 20 01`→`…60 01`, `0x140E71DB2` `48 8D 74 24 20`→`…60` · `0x140E71C9F`/`0x140E71CF2` `41 83 FB 08`→`10` · `0x140E71D1E` `4C 89 A4 C5 C8 00 00 00`→`…88 01…` · `0x140E71D26` `49 8D 47 0D`→`19` · `0x140E71DB7` `48 8D BD D0 00 00 00`→`…90 01…` · `0x140E71DF5` `83 FB 08`→`10` |
| `71E10` recalcular foco N | `0x140E71E58` `41 0F 10 87 C8 00 00 00`→`…88 01…` · `0x140E71EAB` `49 8B 8F C8 00 00 00`→`…88 01…` · `0x140E71ED2` `41 0F 11 87 C8 00 00 00`→`…88 01…` · `0x140E71EEE` `49 8D 45 0D`→`19` · `0x140E71EF5` `49 89 8F C8 00 00 00`→`…88 01…` |
| **`718A0` foco del mando principal** | **reescrita** (8 comparaciones desenrolladas `+0xD0…+0x140`): entrada `0x140E718A0` `40 53 48 83 EC 20 48 8B D9 E8 D2 0A` → `48 B8 <P+0x300> FF E0` |
| Recreación/destrucción (por si PES vuelve a crear el gestor) | `0x140E7159A` `BA 48 01 00 00`→`BA 88 02 00 00` · ctor `0x140E710F1` `44 8D 42 F0`→`F8` (16 pilas), `0x140E71113` `44 8D 42 F8`→`00` (16 focos), `0x140E71117` `48 8D 8F C8 00 00 00`→`…88 01…` · dtor `0x140E71203` `48 81 C1 C8 00 00 00`→`…88 01…`, `0x140E71216` `44 8D 42 F8`→`00`, `0x140E71230` `44 8D 42 F0`→`F8`, `0x140E7123F` `BA 48 01 00 00`→`BA 88 02 00 00` |

Por qué vale la zona home en `FUN_140E71BE0` [CONFIRMADO]: marco = `push rbx; push rbp; sub rsp,0x48` → `rsp+0x58` retorno, `rsp+0x60…+0x7F` su zona home; usa `+0x70` (RSI) y `+0x78` (RDI); **`+0x60…+0x6F` (16 bytes) no se usan nunca**. El cookie de pila (`rsp+0x28`) queda intacto.
Las escrituras del ctor parcheado sobre `+0xC8…+0x140` (ahora pilas 8–15) solo escriben ceros = pilas vacías: inocuo.

**Reescritura de `FUN_140E718A0`** (en `P+0x300`; verificada en `diseno_23`):
```
p = bit más bajo de máscara HidMan (FUN_141B52380), 0 si no hay; si p ≥ 16 → return 0
mejor = 0; nivel = foco[0].nivel
si foco[p].nivel ≥ nivel → mejor = p; nivel = foco[p].nivel
para i = 1..15: si foco[i].nivel > nivel → mejor = i; nivel = foco[i].nivel
return foco[mejor].ventana          // foco[i] en +0x188+i·16, nivel en +0x190+i·16
```
(misma regla que el original: empates a favor del principal frente al 0 y del primero frente a los demás).

### 2.4 Construir y colgar el nuevo (lista B, también dentro del hilo del juego)
`NUEVO_WM` = VirtualAlloc 0x1000 (ceros = 16 pilas y 16 focos vacíos), canario `0xCD` en `+0x288…+0xFFF`; Lua escribe antes `[NUEVO_WM] = P+0x090` (vtable propia: su única entrada, el destructor, es `wm_dtor` = no liberar, porque el bloque no es del montón del juego).
1. COMPROBAR `[0x1436F9C10]` = `VIEJO_WM` y `[VIEJO_WM]` = `0x1427C1568`.
2. COMPROBAR los 44 sitios de §2.3 y los 12 bytes de entrada de `0x140E718A0`.
3. COPIAR `NUEVO_WM+0x08 ← VIEJO_WM+0x08`, 0xC0 (las 8 pilas: se **trasladan** los vectores con sus búferes) y `NUEVO_WM+0x188 ← VIEJO_WM+0xC8`, 0x80 (los 8 focos).
4. PARCHEAR los 44 sitios y la entrada de `718A0`.
5. GUARDAR64 `[0x1436F9C10] = NUEVO_WM`.
6. COPIAR `VIEJO_WM+0x08 ← P+0x2000`, 0xC0 (el viejo queda con pilas vacías: nadie podrá liberar dos veces los búferes).
7. FIN.
**Liberación**: el viejo no se libera nunca (se pierden 0x148 bytes). Al cerrar PES, `FUN_140E71A80` llama a `wm_dtor` del nuevo → no hace nada (los búferes de las pilas se pierden al salir; inocuo). Si PES recrease el gestor después, sale de 0x288 con ctor/dtor de 16, en su montón, coherente.
**Riesgos**: (1) `FUN_140E719E0` y `FUN_140E71E10` no tienen tope: con dueño 16–31 (HidMan admite 32) escribirían fuera; hoy los dueños vienen de mandos que existen y ViGEm da 16 → [HIPÓTESIS] no ocurre; con canario se vería. (2) Llamada desde otro hilo justo durante el cambio (diálogo de desconexión `FUN_141313430`, partido 2D `FUN_1420C2E60/D11E0`): leería el viejo, que sigue siendo memoria válida. (3) Dependencia: con el foco a 16, `71620/71BE0/71E10` llaman a `FUN_140E67AA0(cursor, N≥8)` → **hacer antes el cursor** (con solo `AND 7` ocultaría el cursor del mando N−8).

### 2.5 Lo que queda a 8 fuera de los dos gestores (para que los mandos 9–16 manejen menús de verdad)
- Dueño > 7 → la ventana no usa cursor: `0x140E39DE4` y `0x140E3DCBE` `83 B8 14 01 00 00 07` → `…0F` (en la lista A o en un paso aparte).
- «Primer mando activo» (cap_60_m §1.C, bytes allí): `0x140E3BACE`/`…BAD3`, `0x140E3BD59`/`…BD5F`, `0x140E430A7`/`…30AC`, `0x140E43237`/`…323C`, `0x140E43699`/`…369E`, `0x140E43C68`/`…3C6D`: `08` → `10` (páginas `0x140E3B000`, `0x140E43000`).
- Lo que asigna el dueño `+0x114` de cada ventana (las pantallas): ya a 16 en las 8 de la v0.9.20; el resto se verá en la prueba funcional.

---

## 3. Orden de pasos (uno por arranque de PES) y cómo comprobar cada uno

| Paso | Qué hace | Comprobación (si falla, el paso siguiente no se hace) |
|---|---|---|
| 0 | Solo lecturas a los ~8 s: `[0x1436F9BC8]`, `[0x1436F9C10]`, sus vtables, y los bytes de **todos** los sitios (deben coincidir con `diseno_24`, salvo los `AND 7` si la v0.9.24 ya está). Anotar en `fase1.txt`. | Todo coincide; anotar a qué segundo aparece el cursor (fija §1.1). |
| 1 | Página `P` + vtable apoderada en el cursor viejo, con **lista vacía** (solo FIN). | `RESULT` pasa a 0 en < 1 s; `PEND` a 0; los cursores animan igual; el Visor de eventos sin cierres (`eventos_pes.ps1`); salir de PES normal (prueba stub_term + destructor original). Deshacer = `[VIEJO] = 0x1427BE7E0`. |
| 2 | Lista A (cursor a 16). | `RESULT = 0`; `[0x1436F9BC8] = NUEVO`; en `NUEVO`: entradas 0–7 iguales que el viejo, 8–15 con nodo y Flash ≠ 0 y estado 5; canario `0xCD` intacto (mirarlo cada pocos segundos); menús con mandos 1–8 normales; entrar a un partido y volver; salir de PES sin cierre. |
| 3 | Lista B (foco a 16). | `RESULT = 0`; `[0x1436F9C10] = NUEVO_WM`; pilas 0–7 y focos 0–7 iguales a los del viejo antes del cambio (Lua los copia antes para comparar); `[NUEVO_WM+0x188+i·16]` cambia al moverse por menús; canario intacto; abrir/cerrar diálogos, lista de equipos, estrategia, partido y vuelta; salir sin cierre. |
| 4 | Dueños > 7 y bucles «primer mando» (§2.5). | Igual que 3; nada cambia con 1–8. |
| 5 | Prueba funcional: desactivar el «número prestado» para un mando (p. ej. 16 en Visitante) y comprobar que sube/baja en la lista de equipos (`FUN_140C95E10` → `719B0` ya ve 16 focos) y que su cursor aparece. | Si funciona, el «número prestado» puede quedar como respaldo. |

Reglas comunes: cada paso solo si el anterior arrancó y salió limpio; leer siempre `sider.log` (Sider sin `next`, límite de 200 locales) y el Visor de eventos; si `RESULT ≠ 0`, nada cambió (primera pasada solo compara) y el número dice qué comprobación falló. Los objetos nuevos están en memoria propia: un desbordamiento pisa el canario o cae en una dirección de **nuestra** página (fácil de reconocer en el Visor de eventos), no corrompe en silencio el montón del juego como en la v0.9.22.

## 4. Lo que falta saber (para cerrar las [HIPÓTESIS])
- Qué llamador crea el cursor primero y en qué segundo (paso 0).
- Que el recorrido de tareas llama de verdad a `vtable+0` del cursor cada cuadro (paso 1: `RESULT` pasa a 0).
- Si `OtherRootTask` o la GUI (`FUN_140E36D50`) se destruyen alguna vez antes de salir (los parches de recreación lo cubren en cualquier caso).
- El código máquina está verificado por desensamblado, **no ejecutado**: probarlo antes fuera del juego como `herramientas\probar_codigo_colores.ps1` (ejecutar M con una lista de prueba sobre memoria propia).

## 5. Archivos
`diseno_01` sistema de tareas (`FUN_14149B460/BAD0/…`, `FUN_1414A02B0` matar) · `_02` funciones del foco descompiladas · `_03` desensamblado completo `0x140E710B0…0x140E72200` · `_05/_06/_13` quién crea/borra el foco · `_07` llamadores de `FUN_14149FC70` · `_08/_16` sitios sueltos `0x140E3ECAE/ED35` · `_09` los 21 llamadores del foco · `_10` los 15 llamadores del cursor · `_11` `FUN_14149FC50`, `FUN_140403860` (tareas raíz), asignador · `_12` llamadores de «matar» · `_14` referencias al gestor de tareas · `_15` desensamblado del cursor `0x140E67940…` · `_17` constructores de elementos · `_18–_21` llamadores/vtables · `_22` zona del gestor de tareas (recorrido `FUN_14149FD10`) · `_23` verificación del código propio · `_24` tablas de parches · `_25` bytes del código propio (+ `_25_ens.py`).
