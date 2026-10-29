# Frente F: portero siempre presente en el Entrenamiento libre (PES2021.exe, solo estático)

Base 0x140000000; RVA = VA - 0x140000000. Código en la sección `.trace` (RVA 0x1000, tamaño 0x252d800); tablas en `.rdata`.
Niveles: CONFIRMADO = leído en bytes/desensamblado Ghidra y coherente; PROBABLE = deducido de varios indicios; HIPÓTESIS = por patrón.
Método: `bodies.pkl` + bytes del exe (`pe.py`) + 3 pasadas cortas de Ghidra headless (`Desensamblar.java`, solo lectura; salidas `f_dis1/2/3.txt`, bats `_gh_F1..F3.bat` en esta carpeta). No se tocó ningún proceso ni el exe.
Nota: el decompilado de `FUN_140a59ca0` en `bodies.pkl` está TRUNCADO (le faltan la lectura de las tablas y el resultado booleano); lo correcto es el desensamblado de `f_dis1.txt`. Las funciones diminutas (accesores/setters de TrainingWork) no están en `bodies.pkl`; se leyeron los bytes.

## 0. Resumen ejecutivo

1. **Causa (CONFIRMADO):** quién participa lo decide UNA sola función, `FUN_140A59CA0` (RVA 0xA59CA0, "TW_Participa(tw, idx, limite, S)", entrada pública vía `FUN_140A59EC0`). Cada jugador tiene un *rango* (posición en la lista ordenada de su equipo: GK primero) y una *prioridad* leída de una de TRES tablas de 22 enteros en `.rdata`; participa si `prioridad <= nº de jugadores elegido para ese lado`. En la tabla de modo ATAQUE (`0x142644F10`) el portero propio (índice 0) tiene prioridad **11** (el último en entrar): sólo juega con 11 jugadores. En modo DEFENSA (`0x142644F80`) el portero propio vale 1 (primero) pero el portero rival (índice 11) vale 11. Es diseño del juego: atacantes vs defensa+portero.
2. **Arreglo de datos mínimo (2 dwords en `.rdata`):** `0x142644F10 : 0B 00 00 00 -> 00 00 00 00` (portero propio en Ataque) y `0x142644FAC : 0B 00 00 00 -> 00 00 00 00` (portero rival en Defensa). Prioridad 0 = entra siempre y como EXTRA (no resta a los N elegidos). Variante B (6 dwords) más abajo para cubrir también N=0 del rival y el modo "con porteros".
3. Alternativa de código (cueva a mitad de función, `0x140A59DE2`) en la sección 5; peor que el parche de datos (más frágil).
4. Número de jugadores: menú → `settings+0x3F320` (lado 0) y `+0x3F324` (lado 1) → `FUN_140A5A300` guarda `TW+0x3C/+0x40`; ese valor es el umbral de la tabla (sección 4).

## 1. Cómo decide el ejercicio quién participa (pregunta 1)

### 1.1 Cadena (CONFIRMADO salvo lo marcado)

```
Menú ejercicio (FUN_1404CB0B0, item i -> settings M = [func_0x1414b6a60()+0x50])
   M+0x3F32C  modo (item 0, <2)      M+0x3F30C ejercicio (item 1, <7)    M+0x3F328 byte (item 2, ==1)
   M+0x3F320  nº lado0 (item 3)      M+0x3F324 nº lado1 (item 4, <0xC)   M+0x3F310/318/31C (items 5,6,7)
FUN_141FBA600 (alta de TrainingWork; si M+0x3F308!=... rama iVar27 = M[0xFCC2]==1 = ejercicio activo)
   FUN_141FC14E0(TW, M+0x3F32C)  -> FUN_140A59F40: TW+0x14 = modo (0/1)
   FUN_141F9F8D0(TW, M+0x3F328)  -> byte TW+0x18
   FUN_140A5A300(TW,0,M+0x3F320) / (TW,1,M+0x3F324) -> TW+0x3C / TW+0x40 = nº de jugadores por lado
   FUN_140A5A3C0(TW, M+0x3F32C==1) -> TW+0x1C
   FUN_141FC1420/1400/14C0, FUN_140A59F00(TW, FUN_141FC09D0(M+0x3F310)) -> TW+4 / otros
   FUN_140A59F90(TW, equipoA, equipoB, flag) rellena las formaciones TW+0x44 y TW+0x8204 (11 posiciones cada una; categoría 0..9 por jugador: GK=0)
Por jugador (idx 0..21):
   FUN_140A59EC0(TW, idx, S)  [bytes 4D 8B C8 83 FA 0A 77 09 44 8B 41 3C E9..]:
        idx<=10 -> r8d=[TW+0x3C] ; idx 11..21 -> r8d=[TW+0x40] ; si no -> 0 ; salta a FUN_140A59CA0(TW, idx, limite=r8d, S=r9)
   FUN_140A59CA0 -> bool (AL)
```
Consumidores de la decisión (CONFIRMADO, llaman a `FUN_140A59EC0`): `FUN_1409266E0` (si participa no se toca; si no -> `FUN_140926940` fila de banda), `FUN_141FB9E20` y `FUN_141FD1B90`/`FUN_140A56000` (bucle 0..21 que vuelca el resultado con `FUN_140A3CAC0(S, idx, flag)`), más 0x140448760, 0x14044D2E0, 0x1404A7D60, 0x140927D50, 0x140A19290, 0x142103A20. Todos pasan por la misma función, así que cambiar la tabla es coherente en todos.

`FUN_140A3CAC0(S, idx, flag)` = `if(idx<=0x15) S[0x1748+idx] = (flag!=0)` (bytes `83 FA 15 77 12 45 84 C0 48 63 C2 41 0F 95 C0 44 88 84 08 48 17 00 00 C3`): byte de "jugador activo" en el estado de partido (S = [root+0x270]). CONFIRMADO. Fuera del entrenamiento (rama `M[0xFCC2]==0`) `FUN_141FBA600` pone los 22 a 1.

### 1.2 Lógica exacta de FUN_140A59CA0 (desensamblado `f_dis1.txt`, CONFIRMADO)

Firma: `(rcx=TW, edx=idx, r8d=limite, r9=S)`.
1. `idx > 0x15` -> false.
2. Lista de equipo = `FUN_140A40CA0(TW+0x44 si idx<11 si no TW+0x8204, buf, side, 1)`: ids (side*11 + slot) agrupados por categoría de puesto (0 -> 1 -> 3 -> 6 -> 9; GK=categoría 0 va PRIMERO) y ordenados dentro de cada categoría; se copian hasta 11 ids.
3. `rango` = posición de idx en esa lista (0x19 si no está); `pos = rango` (idx<11) o `rango+11` (idx>=11); `pos > 0x15` -> false.
4. `prio = tabla[pos]` donde `tabla = (TW+0x14 != 0) ? 0x142644F80 : (byte TW+0x18 == 1 ? 0x142644FF0 : 0x142644F10)` (LEA en 0x140A59DC7/DCE/DDB).
5. En parada de balón (`S+0x116C` en {2,3}): si idx es el pateador `S+0x173C` o `S+0x1740` (mismo lado) -> `prio = 0` (siempre entra).
6. Resultado: `prio <= limite`, con una comprobación extra de reinicio (`S+0x1554==5`, `S+0x1486`, `FUN_140A3C1C0`) sólo para el lado de la defensa (lado 1 si TW+0x14==0, lado 0 si !=0).

### 1.3 Las tres tablas (22 int32 cada una; leídas del exe, CONFIRMADO)

| Tabla | VA | RVA | Offset fichero | índices 0..10 (lado 0) | índices 11..21 (lado 1) | Se usa cuando |
|---|---|---|---|---|---|---|
| T_ATAQUE | 0x142644F10 | 0x2644F10 | 0x2643D10 | 11,10,9,8,7,6,5,4,3,2,1 | 1,2,3,4,5,6,7,8,9,10,11 | TW+0x14==0 y TW+0x18!=1 |
| T_DEFENSA | 0x142644F80 | 0x2644F80 | 0x2643D80 | 1,2,3,...,11 | 11,10,9,...,1 | TW+0x14!=0 |
| T_ATAQUE_PORTEROS | 0x142644FF0 | 0x2644FF0 | 0x2643DF0 | 1,11,10,9,...,2 | 1,2,3,...,11 | TW+0x14==0 y TW+0x18==1 |

Lectura: TW+0x14==0 = modo ATAQUE (lado 0 = usuario = atacantes, entran primero los delanteros; lado 1 = defensa, entra primero el portero). TW+0x14!=0 = DEFENSA (al revés). La tabla T_ATAQUE_PORTEROS pone AMBOS porteros con prioridad 1: el juego ya contempla "los dos porteros a la vez" (nativo). La relación TW+0x14==0 <-> "Ataque" es PROBABLE (coincide con tus observaciones: Defensa muestra portero propio, Ataque no); el resto es CONFIRMADO.

Comprobación con tu observación 5v6 (Defensa por defecto): lado 0 con N=5 -> índices con prio<=5 = 0..4 (GK+4 defensas) y lado 1 con N=6 -> prio<=6 = índices 16..21 (6 atacantes) : coincide EXACTAMENTE con "participan idx 0..4 y 16..21" (CONFIRMADO por coherencia; implica además que el modo por defecto observado era Defensa).

Sólo hay 3 referencias RIP a las tablas en todo `.trace` (0x140A59DC7, DCE, DDB, todas dentro de `FUN_140A59CA0`; barrido de LEA/MOV rip-rel). No hay otro lector.

## 2. Dónde se asigna el portero y por qué en Ataque no entra el propio (pregunta 2)

- No existe en el TrainingWork ningún "rol portero": el portero es simplemente el jugador con categoría 0 de la formación del TW (`FUN_140A59F90`: puesto 0 de la alineación -> categoría 0; el resto 1..9) y por tanto **rango 0** en `FUN_140A40CA0`. (CONFIRMADO para la lógica de rango; que la categoría 0 sea el GK por el mapeo del switch de 0xA59F90 es PROBABLE, pero cuadra con los datos 5v6.)
- El rol de portero dentro del partido (slot 0 de cada equipo: idx 0 y 11; `team+0x4B4C` posición de reinicio, `ActionKeeperBasePosition` 0x1409DC040) es independiente del entrenamiento (PROBABLE, ver frente C). El entrenamiento sólo decide si el jugador está activo o aparcado en la banda.
- Por qué falta el portero propio en Ataque: T_ATAQUE[0]=11 -> `11 <= N0` sólo si N0=11. CONFIRMADO (bytes + lógica). Con T_ATAQUE_PORTEROS (`M+0x3F328==1`) sí aparecería (prio 1), pero ese byte además fuerza ejercicio 0 y desactiva el item de ejercicio (`FUN_1404CB220`: si `M+0x3F32C==0 && M+0x3F328==1` -> `M+0x3F30C=0`).
- Portero rival en Defensa: T_DEFENSA[11]=11 -> sólo con N1=11.
- Los mínimos de jugadores por lado (CONFIRMADO, `FUN_140A5A300`, desensamblado `f_dis2/3.txt`): lado atacante: ejercicio 0 -> >=2; 1,3,4 -> >=4; 2,5,6 -> sin mínimo; lado defensor: ejercicio 1 -> >=5; resto >=1; máximo 11. `[TW+0x3C+side*4]=N` sólo si cumple (si no, no cambia). El menú usa la tabla `UNK_1425ADA40` (7 enteros por modo: `3,4,1,4,4,3,3 | 1,5,1,1,1,1,1`) como base de las opciones de los items 3/4.

## 3. Cambio de datos mínimo (pregunta 3a) — RECOMENDADO

Parche en memoria (el `.rdata` es de sólo lectura: VirtualProtect RW, escribir, restaurar; o parche del fichero).

**Variante A (2 dwords): portero EXTRA (no resta de los N elegidos).**

| Qué | VA | RVA | Offset fichero | Original | Nuevo |
|---|---|---|---|---|---|
| T_ATAQUE[0] (portero propio en Ataque) | 0x142644F10 | 0x2644F10 | 0x2643D10 | `0B 00 00 00` | `00 00 00 00` |
| T_DEFENSA[11] (portero rival en Defensa) | 0x142644FAC | 0x2644FAC | 0x2643DAC | `0B 00 00 00` | `00 00 00 00` |

Efecto: con el modo de ataque, N atacantes + tu portero (N+1 en tu lado); en defensa, N atacantes rivales + su portero. No altera los mínimos del ejercicio porque el portero no ocupa plaza de N. Quedan intactos los porteros ya presentes (rival en Ataque, propio en Defensa).

**Variante B (6 dwords): garantía incluso con N=0 del rival o con `M+0x3F328==1`.** Además de A: T_ATAQUE[11] (0x142644F3C, off 0x2643D3C) `01 00 00 00 -> 00 00 00 00`; T_DEFENSA[0] (0x142644F80, off 0x2643D80) idem; T_ATAQUE_PORTEROS[0] (0x142644FF0, off 0x2643DF0) y [11] (0x14264501C, off 0x2643E1C) idem. (Con prio 0 el portero entra aunque `N=0` porque `0 <= N` siempre.) Coste: el portero ya existente pasa de "cuenta dentro de N" a "extra" (N+1 en ese lado).

Firmas AOB únicas (comprobadas en todo el fichero) para localizar las tablas en memoria sin depender de la base:
- T_DEFENSA (inicio de la tabla): `01 00 00 00 02 00 00 00 03 00 00 00 04 00 00 00 05 00 00 00 06 00 00 00 07 00 00 00 08 00 00 00 09 00 00 00 0A 00 00 00 0B 00 00 00 0B 00 00 00 0A 00 00 00` -> VA 0x142644F80 (único). La celda [11] está en +0x2C.
- T_ATAQUE_PORTEROS: `01 00 00 00 0B 00 00 00 0A 00 00 00 09 00 00 00 08 00 00 00 07 00 00 00 06 00 00 00 05 00 00 00` -> 0x142644FF0 (único).
- T_ATAQUE = T_DEFENSA - 0x70 (= ATAQUE_PORTEROS - 0xE0). Firma completa única de 88 bytes: `0B 00 00 00 0A 00 00 00 09 00 00 00 08 00 00 00 07 00 00 00 06 00 00 00 05 00 00 00 04 00 00 00 03 00 00 00 02 00 00 00 01 00 00 00 01 00 00 00 02 00 00 00 03 00 00 00 04 00 00 00 05 00 00 00 06 00 00 00 07 00 00 00 08 00 00 00 09 00 00 00 0A 00 00 00 0B 00 00 00`.
- Con las 32 primeras bytes de T_ATAQUE (`0B 0A 09 ... 04`, dwords) hay 5 coincidencias: NO usarla sola.

## 4. Cómo cambia la lista de participantes con el número de jugadores (pregunta 4)

- Entradas: `N0 = TW+0x3C`, `N1 = TW+0x40` (de `M+0x3F320`, `M+0x3F324`), modo `TW+0x14`, byte `TW+0x18`.
- Participantes del lado s = jugadores con `tabla[pos] <= Ns` (+ los pateadores durante una parada, prio forzada a 0).
- ATAQUE (T_ATAQUE): lado 0 = los N0 más adelantados (rango 10, 9, ...), el GK sólo con N0=11; lado 1 = GK (rango 0) + los N1-1 siguientes por orden de rango (defensas primero).
- DEFENSA (T_DEFENSA): lado 0 = GK + los N0-1 siguientes por rango; lado 1 = los N1 más adelantados, GK sólo con N1=11.
- Por tanto con "pocos jugadores" el orden de entrada es GK -> defensas -> mediocampo -> delanteros en el lado defensor, y delanteros -> hacia atrás en el lado atacante. "Rango" no es el índice de alineación: es la posición tras agrupar por categoría de puesto y ordenar por la coordenada de la formación (`FUN_140A40CA0`/`FUN_140A79640`), por eso para alineaciones normales rango 0 = idx 0 / 11 (PROBABLE).
- El no participante recibe la fila de banda (`FUN_140926940` -> `FUN_140A597D0`), lo que ya hookea Phoenix. Un portero que ahora participa NO pasa por esa rama (se evalúa antes en `FUN_1409266E0`), así que el hook de "aparcar idx 5..15 en (0,0,100)" no lo afecta aunque el idx 11 (portero visitante) esté en ese rango, siempre que ese hook sólo actúe dentro de `FUN_140A597D0`. Comprobar.

## 5. Alternativa de código (si se prefiere no tocar `.rdata`): cueva en FUN_140A59CA0

Sitio: **VA 0x140A59DE2 (RVA 0xA59DE2)**, justo tras calcular `RAX = pos` y antes de usar `EDI = tabla[pos]`. Bytes originales (9 B, 2 instrucciones completas, sin RIP ni saltos): `8B 3C 81 | 8B 86 6C 11 00 00` (`mov edi,[rcx+rax*4]` ; `mov eax,[rsi+0x116C]`). Retorno a 0x140A59DEB (`83 C0 FE`).
Cueva propuesta: ejecutar `mov edi,[rcx+rax*4]`; `test eax,eax; jz z; cmp eax,11; jne s; z: xor edi,edi; s: mov eax,[rsi+0x116C]; jmp 0x140A59DEB` (RAX/RCX/RSI son los de la función; no se tocan flags usados después: el siguiente código hace `add eax,-2` que recalcula). Añadir condición "TrainingWork activo" no es necesario porque la función sólo se llama con TW.
AOB (28 B, sin RIP, único en `.trace`): `8B 3C 81 8B 86 6C 11 00 00 83 C0 FE 83 F8 01 77 77 8B 8E 3C 17 00 00 8B 96 40 17 00 00`.
Firma de entrada de la función (16 B limpios, único): `4C 8B DC 53 55 56 57 41 56 48 81 EC C0 00 00 00`; el primer RIP está en el byte 16 (`48 8B 05 ?? ?? ?? ??`). Un hook de ENTRADA sirve de envoltorio pero no puede conocer el rango del jugador, por eso no se recomienda.
Otras firmas útiles (únicas, sin RIP en el tramo dado): `FUN_140A59EC0` `4D 8B C8 83 FA 0A 77 09 44 8B 41 3C` (el byte 12 es `E9 rel32`; no cortar a 14 B); `FUN_140A3CAC0` `83 FA 15 77 12 45 84 C0 48 63 C2 41 0F 95 C0 44 88 84 08 48 17 00 00` (23 B; el corte a 14 B partiría `41 0F 95 C0`); `FUN_140A5A300` `8B 41 14 45 33 C9 4C 8B D9 41 BA 01 00 00 00 85 C0 75 2E 85 D2 74 34`; setter modo `FUN_140A59F40` `83 FA 01 77 03 89 51 14 C3`.
Un hook en `FUN_140A3CAC0` (forzar flag=1 para idx 0 y 11) NO basta: `FUN_1409266E0` consulta `FUN_140A59EC0` directamente y aparcaría igualmente al portero.

## 6. Qué se leyó

Datos: `frente_C_posiciones_jugadores.md`; `bodies.pkl`: `FUN_1409266E0`, `FUN_140926940`, `FUN_140A59CA0` (truncado), `FUN_140A59B20/B86/C3B` (fragmentos solapados del mismo código), `FUN_140A40CA0`, `FUN_140A59F90`, `FUN_141FB9E20`, `FUN_141FBA600`, `FUN_141FC14E0`, `FUN_1404CB0B0`, `FUN_1404CB220`, `FUN_140A59770`; bytes (pe.py) de `FUN_140A59EC0`, `140A59F00..F80` (setters de TW: +0x10, +8, +4, +0xC, +0x14, vec +0x30, byte +1, byte +0x2C), `FUN_140A3CAC0`, `FUN_141F9C3B0` (`movzx eax,[rcx]`=activo), `FUN_141F9F8D0` (`mov [rcx+0x18],dl`), `FUN_140A5A3C0` (`+0x1C`), `FUN_1408E1C50` (`+4`), `FUN_141F8CE70` (`+0xC`), `FUN_140A86010` (byte+1), las tres tablas y `UNK_1425ADA40`; Ghidra: desensamblado de `FUN_140A59CA0` completo, `FUN_140A5A300`, `FUN_140A5A3C0`.

## 7. NO determinado / pendiente

1. Verificación en vivo (lectura, sin escribir): con ejercicio activo, `TW=[root+0x400]`; leer `TW+0x14` (modo), `TW+0x18`, `TW+0x3C/+0x40`, `S+0x1748..+0x175D` (flags de actividad) y comprobar que los participantes coinciden con la tabla elegida. También `M=[func 0x1414B6A60()+0x50]`: `M+0x3F32C/3F328/3F320/3F324`.
2. Qué valor de `M+0x3F32C` es "Ataque" y cuál "Defensa" no se leyó en las cadenas de menú (inferido de las tablas + tus observaciones: TW+0x14==0 = Ataque, PROBABLE).
3. Si las rutinas de colocación del ejercicio (posiciones específicas por ejercicio, `FUN_1404A56C0/1404A62B0`, `FUN_140A56000`) tienen casos especiales que no esperan un portero extra en ese lado (p. ej. tiros que cuenten jugadores del lado). Hay indicio a favor: la tabla T_ATAQUE_PORTEROS prueba que el juego soporta ambos porteros a la vez en Ataque, pero no se probó el portero rival en Defensa (no hay tabla nativa para ello).
4. Que rango 0 sea siempre el portero cuando la alineación del equipo del usuario tiene al portero fuera del puesto 0 o dos jugadores en categoría 0 (el orden dentro de la categoría depende de `FUN_140A79640`, no leída).
5. Quién rellena las categorías (`FUN_140A425A0/140A42590`, `FUN_1408C51F0/1408C5310`) no se leyó; ya se vio el switch de mapeo en `FUN_140A59F90` (flag param_4).
6. `FUN_140A59B20`, `B86`, `C3B` aparecen en `bodies.pkl` como funciones propias con las mismas tablas, pero el barrido de bytes sólo encuentra los LEA en 0x140A59DC7..DDB; son artefactos del decompilador (código solapado de `FUN_140A59CA0`).

---

# ADENDA (segunda ronda, pistas del usuario)

Menú «Menú de entrenamiento» ↔ campos (CONFIRMADO por `FUN_1404CB0B0`/`FUN_1404CB2F0`/`FUN_141FBA600`/`FUN_141FCDA30` + orden de filas de la captura):

| Fila del menú | Ajuste M = [func 0x1414B6A60()+0x50] | Campo TrainingWork | Opciones |
|---|---|---|---|
| Atacante/Defensor | M+0x3F32C (<2) | TW+0x14 (`FUN_141FC14E0`→`FUN_140A59F40`) | 2 (0 = Ataque, 1 = Defensa; PROBABLE) |
| Tipo | M+0x3F30C (<7) | TW+4 (`FUN_141FC1420`→`FUN_140A59F20`) | 0..6 (FREE_TRAINING = 0) |
| **Cursor de PT fijo** | **M+0x3F328** (byte, `==1`) | **TW+0x18** (`FUN_141F9F8D0`; getter `FUN_14213E2F0`) | No/Sí |
| Jugadores usuario / COM | M+0x3F320 / M+0x3F324 | TW+0x3C / TW+0x40 (`FUN_140A5A300`) | mínimos por tipo (§2) |
| Nivel de partido | M+0x3F310 (<6) | TW+0x10 (`FUN_141FC09D0`→`FUN_140A59F00`) | 6 |
| Faltas | M+0x3F318 (<2) | TW+8 (`FUN_141FC1400`→`FUN_140A59F10`) | 2 (el gestor ignora `k==5` si vale 0: frente G) |
| **S. pta./córner/banda** | **M+0x3F31C** (<2) | **TW+0xC** (`FUN_141FC14C0`→`FUN_140A59F30`; getter genérico `FUN_141F8CE70`) | **2** |

El texto del menú no está en el exe: son ids de localización (`0x160015` cursor PT; `0x20079` S.pta.; `0xA0002` faltas; `0x3DE00EB` nivel). No se buscó la cadena «Cursor de PT fijo» en recursos; las filas se identificaron por ORDEN y por el recuento de opciones (PROBABLE el emparejamiento fila↔campo, CONFIRMADO el de Cursor fijo = M+0x3F328 porque es el único booleano que acopla Ataque y fuerza Tipo 0).

## 8. «Cursor de PT fijo»: separar (a) portero participante de (b) cursor bloqueado

El flag M+0x3F328 → TW+0x18 hace DOS cosas independientes:
- **(a) Participación**: `FUN_140A59CA0` (0x140A59DC3: `CMP byte [RDI+0x18],1` + `CMOVNZ`): con modo Ataque y TW+0x18==1 usa T_ATAQUE_PORTEROS (ambos porteros prio 1). CONFIRMADO. (El menú además fuerza «Tipo» = 0 con Ataque + flag Sí, `FUN_1404CB220`.)
- **(b) Cursor fijo del usuario en el portero** (lectura CONFIRMADA, efecto PROBABLE):
  1. `FUN_141FB9040` (alta de mandos): `if (M+0x3F308==1 && M+0x3F328!=0)` y el mando principal (`FUN_141B52380` = máscara de mandos) coincide con el registro -> busca el primer slot libre del equipo (`team+0x4C00`, slot 0 = GK) y se lo asigna (`reg+0x24 = idx`, modo de cursor 3 = fijo).
  2. `FUN_14043EC70` (por fotograma): `TW activo (FUN_141F9C3B0) && FUN_14213E2F0(TW)` -> misma asignación del mando principal.
  3. `FUN_14087AC70`: `TW activo && byte1==1 && TW+0x18==1` -> `[RBX+0x10]=1` (estado del cursor).
  Hay ~20 llamadores más del getter `FUN_14213E2F0` (p. ej. 0x140BDCD52, 0x140BEBE43, 0x140CECEDB, 0x14137xxxx, 0x1416A6F55) NO leídos: pueden ser más efectos de (b) (HUD/cámara/selección) o lecturas de otras clases (el getter es código idéntico compartido). HIPÓTESIS.

### 8.1 Vía recomendada (no toca (b)): forzar (a) sin activar el flag
Dejar «Cursor de PT fijo = No» (TW+0x18=0, nadie bloquea el cursor) y hacer que `FUN_140A59CA0` elija T_ATAQUE_PORTEROS siempre en Ataque:

| Parche | VA / RVA | Original | Nuevo |
|---|---|---|---|
| P-A: anular el `CMOVNZ` que cambia a T_ATAQUE | 0x140A59DD5 / 0xA59DD5 (4 B) | `48 0F 45 CA` | `90 90 90 90` |

AOB único (RIP enmascarado; el parche cae en el byte +18): `80 7F 18 01 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 0F 45 CA` -> inicio VA 0x140A59DC3 (único). Tras el parche: Ataque (`TW+0x14==0`) usa SIEMPRE `0x142644FF0` (GK=1 en ambos lados, el GK cuenta dentro de N). Es una configuración nativa (equivale a Ataque + «Cursor fijo = Sí» + Tipo 0). Defensa (`TW+0x14!=0`) sigue en T_DEFENSA: añadir el parche de datos del GK rival (§3): `0x142644FAC : 0B 00 00 00 -> 01 00 00 00` (cuenta dentro de N) o `00 00 00 00` (extra). El portero propio lo maneja la IA y el cursor cambia con normalidad porque (b) nunca se arma. NO probado en vivo.

### 8.2 Si se quiere «Cursor de PT fijo = Sí» y anular sólo (b)
Parches de 1 a 2 bytes (no afectan a (a), que lee TW+0x18 por su cuenta). `rel32` queda idéntico porque el nuevo `E9 rel32` termina en la misma dirección que el `0F 8x rel32` original:

| # | Función | VA del byte | Original | Nuevo | AOB único (VA de inicio) |
|---|---|---|---|---|---|
| B1 | `FUN_141FB9040` | 0x141FB9713 | `0F 85 CC 00 00 00` (JNZ) | `90 E9 CC 00 00 00` (JMP, mismo destino 0x141FB97E5) | `41 83 7D 00 01 0F 85 ?? ?? ?? ?? 45 38 65 20 0F 84` -> 0x141FB970E |
| B2 | `FUN_14043EC70` | 0x14043EE2C | `0F 84 C5 00 00 00` (JZ) | `90 E9 C5 00 00 00` | `48 8B CB E8 ?? ?? ?? ?? 84 C0 0F 84 C5 00 00 00 E8 ?? ?? ?? ?? 33 C9` -> 0x14043EE22 |
| B3 | `FUN_14087AC70` | 0x14087ADB7 | `75 09` | `EB 09` | `48 8B CE E8 ?? ?? ?? ?? 3C 01 75 09 C7 43 10 01 00 00 00` -> 0x14087ADAD |

Riesgos: B1 evita la asignación inicial del mando al GK; B2 la reasignación por fotograma; B3 evita forzar `+0x10=1`. Los ~20 usos restantes del getter pueden seguir viendo TW+0x18=1 (no auditados).
Propuesta para Phoenix: `arquero_propio_free` = P-A (8.1) + parche de datos de Defensa (§3); variante `arquero_propio_free_cursor` = B1+B2+B3 si se insiste en el flag Sí. Son bytes estáticos (código/.rdata): no llevan chequeo de contexto propio; aplicarlos al entrar al modo street y revertirlos al salir, o condicionarlos con un hook (`[root+0x400]!=0 && byte0(TW)!=0`).

## 9. «S. pta./córner/banda» (TW+0xC): valores y lectura

- Sólo 2 valores: `M+0x3F31C < 2` (menú) y `FUN_140A59F30` rechaza `>1` (`83 FA 01 77 03 89 51 0C C3`). CONFIRMADO. El usuario aporta las 2 etiquetas «Solo usuario» / «Ambos equipos»; **no existe valor «ninguno»**. Cuál etiqueta es 0 y cuál 1 no se leyó (ids de texto `0x2007D`, `0xD0001`); por la lógica de abajo «Ambos equipos» = 1 es PROBABLE.
- Lectura (CONFIRMADO, `FUN_1404A56C0`, frente G §3-4): `TW+0xC` sólo se usa en la rama de salida de balón con TW+4==0: si `==1` remapea los tipos del Juez 4↔3 (córner↔saque de puerta) y 6↔5 (penal↔falta), es decir cambia la perspectiva atacante/defensor; si `!=1` sólo reescribe «saque de centro de `rec+4==1` con `k==3`» como saque de puerta. En ningún caso anula el saque: siempre publica escena (`k=7`). Los parches del Juez del frente G (`FUN_14050E4A0` RVA 0x50E65F y 0x50E68D) siguen siendo necesarios. Lo único nativo que ignora paradas: «Faltas = No» (`TW+8==0` ignora `k==5`) y `byte1 (TW+1)!=0` (return temprano; estado interno de reinicio, no es ajuste de menú).

## 10. Regla de recorte de 11 a N («Jugadores usuario»)

CONFIRMADO (§1.2-1.3 + observación del usuario): NO se recorta la lista quitando el slot 0 ni los últimos índices. Cada jugador tiene una prioridad (según modo/flag) y entra si `prioridad <= N` de su lado. En Ataque (T_ATAQUE, lado usuario = lado 0) el portero tiene prioridad 11 y los delanteros 1..: el orden de entrada es delantero más adelantado -> ... -> portero (último). Con N<11 el portero queda fuera; con N=11 entran todos. Con «Cursor fijo = Sí» se usa T_ATAQUE_PORTEROS (GK=1: primero) y por eso aparece. En Defensa el portero propio ya es prioridad 1.
Para que el slot 0/GK participe SIEMPRE con cualquier N y lo maneje la IA: (1) §8.1 P-A (Ataque -> tabla con portero a prioridad 1 en ambos lados) + parche de datos T_DEFENSA[11]; o (2) §3 Variante A (prioridad 0: el portero es extra y N cuenta sólo jugadores de campo). Los mínimos de jugadores del lado atacante (`FUN_140A5A300`: Tipo 0 -> >=2) hacen que la Variante A (prioridad 0) sea la que menos distorsiona los N elegidos; con prioridad 1 el GK ocupa una plaza de N.
IA del portero: no se encontró código específico en el entrenamiento; el slot 0 conserva su rol de partido (frente C) cuando participa. Fuera del cursor fijo, el cursor lo asigna el flujo normal de `FUN_141FB9040`.

## 11. Pendiente tras la adenda
1. Probar en vivo P-A (8.1) y el parche de datos de Defensa; vigilar que el ejercicio no cuente al portero como atacante.
2. Auditar los ~20 llamadores restantes de `FUN_14213E2F0` (más efectos del flag Sí).
3. Etiquetas exactas 0/1 de «S. pta./córner/banda» y «Cursor de PT fijo» (ids de texto `0x20079`, `0x160015`).

---

# 12. GK propio presente pero "pasivo" en Ataque (seguimiento tras probar P-A en vivo)

Resultado honesto: **la causa del portero pasivo NO se pudo fijar en estático**. Lo que sí quedó establecido y descartado:

## 12.1 Qué diferencia Ataque/Defensa en el código (TW+0x14), CONFIRMADO lectura
TW+0x14 (getter `FUN_141030AB0` = `mov eax,[rcx+0x14]`) sólo lo leen funciones del gestor de entrenamiento (rango 0x1404A5xxx-0x1404A9xxx) y `FUN_1420C8140`:
- `FUN_140A59CA0`: tabla de prioridad (ya cubierto, §1).
- `FUN_1404A5580` (RVA 0x4A5580, "¿hay que reiniciar el ejercicio?"): `restart_team = S+0x1558`; compara con `(TW+0x14 != 0)` -> el equipo "atacante" del ejercicio es el lado 0 en Ataque y el lado 1 en Defensa (los tipos de ejercicio 1..6 se comparan con `S+0x1554`).
- `FUN_1404A7BA0` y `FUN_1404A67B0` (RVA 0x4A7BA0 / 0x4A67B0): recortan una posición (balón/punto de reinicio, `float* pos`) a UNA mitad del campo: Ataque -> `x >= 0` (se invierte el signo al final), Defensa -> `x <= 0`; más recorte en z con `dims[1]`. NO tocan a los jugadores ni al portero. (Los llaman `FUN_1404A7830/786C/788C/792B`.)
- `FUN_1420C8140`: signo (+1/-1) según modo; cámara/HUD (PROBABLE, no leída entera).
- `FUN_1404A8620/8930/90D0/93A0`: copian la formación de un equipo a un buffer del TW (`FUN_140A40CA0`, `FUN_140A411C0`); no hay rama por modo para el portero.
No se encontró ningún sitio donde el modo active/desactive el "think" del portero, un flag de equipo (`team+0x254` sólo es el signo de ataque) ni un `ActionKeeper*`. NO existe, en lo leído, una "IA solo ataque" para el equipo del usuario.

## 12.2 Qué más hace «Cursor de PT fijo» (TW+0x18), CONFIRMADO lectura
- Escribe: `M+0x3F328` (menú) -> `FUN_141FCDA30`/`FUN_141FBA600` -> `FUN_141F9F8D0` (`mov [rcx+0x18],dl`). Lee: (a) tabla en `FUN_140A59CA0`; (b) `FUN_141FB9040` (mando principal -> slot GK, B1), `FUN_14043EC70` (por fotograma, B2), `FUN_14087AC70` (estado de cursor, B3); (c) `FUN_141FCDA30` (sincroniza TW con el menú); (d) estadísticas `FUN_1404CBDA0` (+0x3527C++).
- La asignación del slot GK (B1) busca el primer slot con `team+0x4C00+4k == 0`. `team+0x4C00` = categoría de puesto por slot (misma estructura de formación que `TW+0x44`, offset `+0x3FB8`; GK = categoría 0). CONFIRMADO por `FUN_14062D350` (cuenta jugadores con categoría != 0).
- Los ~20 llamadores restantes de `FUN_14213E2F0` (0x140BDCD52, 0x140BEBE43, 0x140CECEDB, 0x140CF600D, 0x140D6EF21, 0x140D8D3E3, 0x141002812, 0x141069DE4, 0x1410BDC4C, 0x1411209F2, 0x141379498…0x1416A9FA0) se leyeron: el argumento es `*(ctx+0x40)` con `ctx = FUN_140FF0A20()` (contexto del partido), es decir pantallas/HUD/menú (pausa, marcador, cámara del modo entrenamiento). No hay ninguno en IA de jugador/equipo. PROBABLE: sólo presentación. => **«Cursor fijo = Sí» no activa ningún think/IA de portero ni modo manual en el código leído: sólo (a) tabla y (b) asignar el mando al GK.** Por tanto B1+B2+B3 con flag Sí dejan al GK en el mismo estado que flag No (COM), salvo HUD.

## 12.3 Qué activa el cerebro del portero (lo leído), y por qué no basta con la participación
- El jugador sólo actúa si `FUN_1409250A0` (Player::Update, RVA 0x9250A0) llega a la rama `+0x7C9 != 0`. `+0x7C9` lo fija `FUN_140926B00` según la escena `S+0x11B0`: en juego (escena 1) `FUN_140927360` hace `+0x7C9 = (+0x7CD != 0)` y `+0x7CD = 1` cuando `S+0x11C3 == 1`; escena 0x11 (entreno) -> `FUN_1409266E0`: el participante queda como está (no se toca `+0x7C9`), el no participante va a la banda. Dentro de `FUN_1409250A0` hay tres cortes: `TW+0x2C != 0` (`FUN_140E23890`, setter `FUN_140A59F80`; congela a TODOS: espera de reinicio de ejercicio), flags `S+0x1748+idx` y `S+0x175E+idx` ambos 0 sin `param_1[0xFC]` (jugador "fuera del partido") y `S+0x137C`. Ninguno distingue GK ni modo. => si el GK fuera pasivo por esto, lo sería también en Defensa.
- La conducta del portero sale de acciones `match::player::ActionKeeper*` y de think-units de mando `match::pad::ThinkUnitKeeper*` (AutoMove, Block, TrapMove, PickupBall…, ctor `FUN_1408AD0B0`). Las think-units se crean por mando (pad), no por equipo; en lo leído no hay condición de entrenamiento/modo.
- **Cambio de cursor a un portero** = `ThinkUnitCursorChangeKeeper` (vtable 0x14260EC80): su método de entrada `FUN_1408B7D70` devuelve la acción 0x4F sólo si el botón #10 del mando (`FUN_140A5F4F0(pad,10)`) está pulsado. O sea: en PES el cursor NO rota hasta el portero con el botón normal de cambio; hace falta ese botón especial (cambio a portero). Es PROBABLE que "el usuario no puede cambiar el cursor a él" sea el comportamiento normal salvo que se use ese botón; no se leyó el manejador de la acción 0x4F (puede exigir balón en el área propia).

## 12.4 Hipótesis para "no ataja / se aparta" (sin confirmar)
H1 (la más plausible): no hay disparos reales contra la portería del usuario en Ataque (el rival sólo defiende: tabla T_ATAQUE_PORTEROS pone defensores rivales + su GK), así que el GK propio nunca entra en las acciones de parada, y "se aparta del balón" es la conducta normal de un GK con balón propio lejos del área. H2: el GK propio queda fuera de la zona de juego por el recorte a una mitad (§12.1; sólo afecta a puntos de reinicio, no a jugadores) o por las posiciones de reinicio del ejercicio (`FUN_140A43190` del TW, no leída). H3: la IA de equipo (`FUN_14062D350`, formación con pocos jugadores) lo trata como jugador de campo suelto (cuenta de jugadores de campo activos con `categoría != 0` vs `FUN_14063FA70`). No se encontró base para H2/H3 en estático.

## 12.5 Parche mínimo propuesto (SOLO si H1 no explica lo visto)
No hay parche "de un byte" que active un cerebro de portero: no existe tal flag. Opciones reales, de menor a mayor riesgo:
1. **Diagnóstico de lectura (sin escribir)**, con GK propio presente en Ataque y luego en Defensa, comparar: `S=[root+0x270]`: `S+0x1748+0`, `S+0x175E+0`, `S+0x11B0`, `S+0x11C3`, `S+0x1554`, `S+0x1558`; `player(idx0)=FUN_140A489F0(root,0)`: `+0x7C9`, `+0x7CB`, `+0x7CD`, `+0x18`; `TW+0x14/+0x18/+0x2C/+0x3C/+0x40`; `team0+0x4C00..+0x4C28`. Si `+0x7C9` del GK es 1 y aun así no actúa, la causa es de IA de equipo/pad, no de participación.
2. Probar B1+B2+B3 (§8.2) con «Cursor fijo = Sí» ("arquero_propio_free_cursor"): si el GK queda COM y reacciona, la causa estaba en una lectura de TW+0x18 que no pude ver; si queda igual que con flag No, queda descartado.
3. Probar el botón de cambio a portero (#10 del mando, mismo botón que usa PES para "Change to keeper") con el balón en el área propia.
4. Alternativa de datos: evitar el problema usando Defensa (donde el GK propio funciona, confirmado por el usuario) y que el usuario "ataque" cambiando el campo de juego: no es un parche, es un modo de uso.

## 12.6 Pendiente
`FUN_140A43190`/posiciones del ejercicio del TW para participantes; manejador de la acción 0x4F (cambio a portero); `FUN_140928310` (decisión de acción del jugador no controlado) y `FUN_14092C9F0` (15 KB, usa `FUN_140A59AF0` = prioridad sin excepción de pateadores); si `team+0xA2A0/+0xB748` (rol por slot) se rellena distinto con TW activo en Ataque.
