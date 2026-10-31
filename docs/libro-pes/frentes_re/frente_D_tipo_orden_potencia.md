# Frente D: tipo de orden cmd+0x1768 y potencia del tiro (Entrenamiento libre)

Análisis estático de PES2021.exe (base 0x140000000). RVA = VA - 0x140000000.
Fuentes leídas: bodies.pkl (decompilado: FUN_1408B3D90, B3BC0, B4170, B1CF0, B3AC0, B35A0, B20D0, B1F90, B2750, B3220, B2240, B2A50, B33F0, B1490, AAA30, AB000, AC500, AC7A0, AC860, AC9D0, AC960, AEF70, B9C80, BB0D0, BB1B0, B9D40, B9DE0, AF820, B08A0, B0C30, AD0B0, ACC00, 1406B45D0), bytes crudos del .text (fox.fbytes/CODE) y vftables de ThinkUnitShoot/Clear.
Niveles: CONFIRMADO = visto en decompilado + bytes; PROBABLE = deducido con evidencia fuerte; HIPÓTESIS = sin verificar.

## 0. Corrección clave de la premisa

- **cmd+0x1768 NO es un "tipo de tiro" copiado de un campo.** Es `ordA`: el **ID de tabla de la ThinkUnit ganadora** (CONFIRMADO). 4 = ThinkUnitClear, 3 = ThinkUnitShoot (NO 1). 1 = ThinkUnitLongPass. Por eso "hacer que valga 1" no es lo buscado: hay que lograr que valga **3**.
- Tabla de ids (FUN_1408ACC00 + FUN_1408AD0B0; el puntero de unidad es p+0x14B8+id*8, p = cmd+0x40): 0 ShortPass, 1 LongPass, 2 ThroughPass, **3 Shoot (vt 0x14260FB58, unidad en p+0x1F0)**, **4 Clear (vt 0x14260FEE8, unidad en p+0x230)**, 5 Press ... 0x17 KickFeint, 0x36 AdvanceShoot. (CONFIRMADO)
- El campo unidad+0x14 (Shoot=1, Clear=4, "modo de carga") es OTRA cosa: no es el id. El id es el índice en la tabla. (CONFIRMADO: AD0B0 pone +0x14=1 en Shoot y =4 en Clear.)

## 1. Quién escribe cmd+0x1768 y cómo se interpreta

Layout (CONFIRMADO): el objeto cmd (CommandPlayer, 0x1840 B, ctor FUN_1408B3BC0, 24 instancias) tiene en cmd+0x40 el "gestor de unidades" p. El ctor llama FUN_1408B3D90 con rcx=cmd+0x10 (`add rcx,0x10`), y ésta usa rbx=rcx+0x30=cmd+0x40. Por eso los offsets del decompilado de B3D90 salen 0x10 desplazados. Los accesores (B1CF0, B1F90, B2750...) reciben p=cmd+0x40 y usan p+0x1728 = cmd+0x1768.

Registro R en p+0x1728 (cmd+0x1768):
- p+0x1728 (cmd+0x1768) = ordA = id de la unidad del grupo A (unidad+0xC==2: botones, entre ellos Shoot y Clear). 0x4E = ninguna. Init 0x4E en B3D90 (0x1408B3DCC).
- p+0x172C (cmd+0x176C) = ordB (unidades de grupo != 2).
- p+0x1744/0x174C/0x1754 (cmd+0x1784/0x178C/0x1794) = registro de parámetros de acción (ver sección 2).

Escrituras (aquí deben ir los DR de hardware):

| Qué | VA | RVA | Instrucción | Notas |
|---|---|---|---|---|
| ordA := id ganador | 0x1408B204D | 0x8B204D | `89 BE 28 17 00 00` mov [rsi+1728h],edi | en FUN_1408B1F90; edi=id de unidad; rsi=p=cmd+0x40. ESTE es el escritor que pasa de 0x4E a 4 (o 3). CONFIRMADO |
| ordB := id | 0x1408B2020 | 0x8B2020 | `89 BE 2C 17 00 00` mov [rsi+172Ch],edi | grupo != 2 (y id != 0x19) |
| ordA := 0x4E (reset/liberar) | 0x1408B27B5 | 0x8B27B5 | `C7 83 28 17 00 00 4E 00 00 00` | FUN_1408B2750 (cuando id==ordA o id>0x4D) |
| ordB := 0x4E | 0x1408B2782 | 0x8B2782 | `C7 81 2C 17 00 00 4E 00 00 00` | B2750 |
| init 0x4E | 0x1408B3DCC / 0x1408B3DD7 | 0x8B3DCC | `C7 83 28 17 00 00 4E..` / `48 C7 83 2C 17 00 00 4E..` | ctor |

Por eso la búsqueda de `[reg+0x1768]` solo halló otras clases: la base real es cmd+0x40 con disp 0x1728 (reg = rsi/rbx/rcx según la función). Para buscar lectores/escritores hay que buscar disp32 0x1728/0x172C, no 0x1768. Los lectores directos con disp 0x1768 (FUN_1408AAA30 en 0x1408AAD97, FUN_1408AB000 en 0x1408AB04C, A9180, A9D60) usan como base cmd completo (objeto devuelto por la llamada virtual = el CommandPlayer; CONFIRMADO por +0x10/+0x18 que coinciden con el reset del ctor).

Condiciones de B1F90 para escribir ordA (CONFIRMADO): unidad(id).+0x14 != 0 Y unidad.+0x1C (estado) != 0 Y unidad.+0xC == 2 Y ordA != id.

Lectura del medidor, FUN_1408AB000 (0x1408AB049 `48 63 87 68 17 00 00` + `83 F8 36 77`): switch(ordA): casos 0,1,3,0x17,0x36 = barra normal (sigue la carga cmd+0x10); 2 y 0x35 = umbral; **4 (Clear) = barra al 100% (`mov dword [param+4],3F800000h`)**; otros ids = barra oculta. Con ordA=3 (Shoot) la barra es proporcional. CONFIRMADO.

Cómo se decide quién gana (el origen real de "tipo 4"):
1. FUN_1408AF820 reconstruye cada fotograma la lista de ids de unidades candidatas (p[0]=cuenta, p[1..]=ids) con un builder por rol; el rol 1 (portador) es FUN_1408B08A0, que empuja 0 (ShortPass), 1 (LongPass), **4 (Clear) y después 3 (Shoot)** (también 0x36 / 0x35 / 2). Clear va ANTES que Shoot. (CONFIRMADO)
2. FUN_1408B3AC0 recorre la lista llamando FUN_1408B35A0 por unidad; la primera que devuelve acción y supera FUN_1408B20D0 toma ordA y bloquea a las demás de grupo A (B20D0: si ordA<0x4E y ordA != id => falso, salvo ordA in {1,3,0x36} con id in {2,0x35}). (CONFIRMADO)
3. Quien habilita a cada unidad es su ranura **+0x88** (predicado de zona), usada por la máquina de estados FUN_1408AC7A0 (estado 0 -> 1 solo si +0x88 devuelve verdadero; botón = unidad+8 = 0xF):
   - **Shoot +0x88 = FUN_1408B9C80**: si `[mode+0x116C]==1` y `dir*pos.x < 0` (jugador en su propio campo) devuelve 0; si no, FUN_1408AC960 (botón pulsado).
   - **Clear +0x88 = FUN_1408BB0D0**: solo si `dir*pos.x < 0` (campo propio) y (`mode==1` o (`mode==3` y flag +0x2D)) devuelve FUN_1408AC960; si no, 0.
   Donde mode = `*(int*)(*(_DAT_1436F3DC8+0x270)+0x116C)`; dir = `*(char*)(equipo+0x254)` (+-1); pos.x = `*(float*)(jugador+0x55C)`. (CONFIRMADO en bytes/decompilado.) Es decir: en campo propio (mitad, no solo área) Shoot se apaga y Clear se enciende; el "tipo 4" es simplemente que Clear está vivo y Shoot no.
   Nota: la condición real es "mitad propia" (comiss con 0), no el área. Qué valor de 0x116C tiene Entrenamiento libre: NO DETERMINADO (si Clear gana en entreno, entonces es 1, o 3 con flag).
4. Dato extra: Clear devuelve 0x58 en el PRIMER fotograma de pulsación (FUN_1408AEEE0: `mov [rcx+20h],3DCCCCCDh (0.1); mov eax,58h`) sin esperar a soltar; Shoot en pulsar devuelve 0 (su +0x60 es un thunk a +0x68=B9AF0 que acumula carga y devuelve 0) y dispara en estado 3 (soltar, o automático cuando la carga llega a 1.0: AC7A0 estado 2 -> 3 si `unidad+0x20 >= 1.0`) con +0x70=0x140C0B9A0 -> 0x55. Por eso con Clear no puede haber potencia proporcional al tiempo: la acción ya salió al pulsar.

## 2. De dónde sale la potencia

Cadena de la carga (CONFIRMADO):
- unidad+0x20 (float 0..1) se acumula en FUN_1408AC9D0 (llamada desde B9AF0 en estado 2 de Shoot). Modo = unidad+0x14: 0 no acumula; 1/2 acumulan solo mientras unidad+0x29 (contador de pulsaciones) sea 0/1 (o <=2); **4 sale sin acumular (Clear)**; 3 acumula siempre. Tope 1.0 (`_DAT_14256AE1C`).
- FUN_1408B2240 (llamada desde B3220) construye el registro de acción y pone en `param_2[2]` = carga (unidad+0x20, o la de la unidad ordA si la acción es 0x5B). B3220 lo guarda en p+0x1744 (dirección/estick), **p+0x174C (carga, float en cmd+0x178C)** y p+0x1754.
- FUN_1408B33F0 (via B2A50) copia a los punteros de salida: para la unidad con id==ordA del grupo A: `*(cmd+0x10) = unidad+0x20` (carga que ve el medidor y FUN_1408AAA30) y `*(cmd+0x18) = unidad+0x19` (flags). Requiere unidad+0x14 != 0 y unidad+0x20 > 0.
- cmd+0x10 se pone a 0 al inicio de cada fotograma (FUN_1408B1CF0 `*(float*)param_4[3]=0`) y se vuelve a copiar solo si hay unidad ganadora con carga > 0. Por eso en vivo se observa 0.0 en fotogramas sin carga.

**Dónde se fija la potencia a tope (CONFIRMADO en bytes):** FUN_1408B1490 (emisor de la acción), rama `case 3` (acciones de categoría 3: 0x55 y 0x58), código:
```
0x1408B158D  F2 0F 10 8F 54 17 00 00   movsd xmm1,[rdi+1754h]
             F2 0F 11 4D B7
             0F 10 87 44 17 00 00      movups xmm0,[rdi+1744h]
             0F 11 45 A7
0x1408B15A5  83 FB 04                  cmp ebx,4        ; ebx = id de unidad (param_5)
0x1408B15A8  75 0A                     jne +0Ah         ; usa carga real [rbp-51h] (= p+174Ch)
0x1408B15AA  F3 0F 10 15 xx xx xx xx   movss xmm2,[_DAT_14256AE1C]  ; 1.0
```
Si el id de unidad es 4 (Clear), el parámetro de potencia/carga de la acción se **sustituye por la constante 1.0**; para cualquier otro id usa la carga real (cmd+0x178C). Esto explica "aun forzando 0x55, sale fuerte": el id sigue siendo 4. Con ordA=3 este camino no se toma. Esa carga entra a FUN_140A66290 (constructor de parámetros de la acción de categoría 3), y la potencia final la calcula FUN_140833CD0/FUN_1406D9540 a partir de ese parámetro (PROBABLE: no se re-leyó 833CD0 en detalle en esta pasada).

## 3. Recomendación

Principio: no forzar Clear a ser tiro (Clear dispara al pulsar con carga 0.1 y potencia 1.0 por id==4), sino lograr que **Shoot (id 3) gane** en campo propio. Entonces ordA=3, la barra es proporcional (AB000 caso 3), la carga se acumula y dispara al soltar con el comportamiento normal de fuera del campo propio, y la potencia usa la carga real (B1490: id 3 != 4).

Requiere dos cambios (ambos son parches de 1 byte a aplicar solo en Entrenamiento libre; el Core puede escribirlos al entrar y restaurarlos al salir):

A. Shoot aplicable en campo propio, FUN_1408B9C80 (+0x88 de Shoot):
```
VA 0x1408B9CE8  (RVA 0x8B9CE8)  75 22  ->  EB 22     (jne -> jmp: salta la comprobación de mitad propia)
AOB (único, 17 B, sin RIP): 41 83 BE 6C 11 00 00 01 75 22 0F BE 80 54 02 00 00    ; parchear el byte +8 (75 -> EB). Inicio del patrón: 0x1408B9CE0
```
B. Clear no aplicable, FUN_1408BB0D0 (+0x88 de Clear):
```
VA 0x1408BB150  (RVA 0x8BB150)  76 34  ->  EB 34     (jbe -> jmp al "xor al,al": devuelve siempre 0)
AOB (único, 18 B, sin RIP): 76 34 8B 87 6C 11 00 00 83 F8 01 74 19 83 F8 03 75 24    ; parchear el byte +0
```
Si solo se hace A, sigue ganando Clear (se empuja antes que Shoot y su predicado también pasa en campo propio). Si solo B, no hay tiro. Alternativa por detour (más limpia, condicional): enganchar las entradas de las dos funciones y devolver 0 / ejecutar original según bandera de entreno:
- Clear +0x88: `FUN_1408BB0D0` VA 0x1408BB0D0 (RVA 0x8BB0D0), AOB único de 32 B sin RIP: `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 48 8B E9 48 8B DA`.
- Shoot +0x88: `FUN_1408B9C80` VA 0x1408B9C80 (RVA 0x8B9C80), AOB único de 32 B: `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 48 8B F1 48 8B DA`. (Detour en training: saltar directo a FUN_1408AC960 con los mismos rcx/rdx, firma `48 89 5C 24 08 57 48 83 EC 40 83 79 14 02`, VA 0x1408AC960; firma de ranura: this=rcx, ctx=rdx, r8d=param_3.)

Ambas funciones solo son alcanzables por vtable (ranura +0x88 de ThinkUnitShoot/Clear, llamada desde FUN_1408AC7A0); no tienen llamadores directos, por lo que el detour/parche no afecta a otras rutas. Las comparaciones usan el modo 0x116C, así que fuera de Entrenamiento habría que restaurar.

Opción alternativa (no recomendada, HIPÓTESIS): cambiar `*(int*)(*(_DAT_1436F3DC8+0x270)+0x116C)` a un valor distinto de 1 y 3 haría que Shoot pase siempre y Clear no aplique, pero ese campo lo usan decenas de funciones; riesgo alto.

Parche paliativo sobre Clear (solo para confirmar el diagnóstico): FUN_1408B1490 VA 0x1408B15A8 `75 0A` -> `EB 0A` (AOB único: `F2 0F 10 8F 54 17 00 00 F2 0F 11 4D B7 0F 10 87 44 17 00 00 0F 11 45 A7 83 FB 04 75 0A`, inicio 0x1408B158D, parchear el byte +0x1B). Quita el 1.0 forzado, pero la acción de Clear sale al pulsar con carga ~0.1 (no proporcional al tiempo) y la barra seguiría al 100% por AB000 caso 4. No cumple el objetivo.

## 4. Qué esperar de los puntos de escritura (DR)

- cmd+0x1768 (dword): escritor normal `mov [rsi+1728h],edi` en 0x1408B204D (edi=3 o 4) y reset a 0x4E en 0x1408B27B5 / 0x1408B3DCC. Todos tamaño 4. No se halló copia de bloque ni rep movs en esta ruta (ninguna función del rango 0x1408A0000-0x1408D0000 tiene disp 0x1728 de escritura fuera de estas).
- cmd+0x10 (float): el cero por fotograma está en FUN_1408B1CF0 (`mov dword [rax],0` vía puntero guardado en la pila, 4 B); la carga se escribe en FUN_1408B33F0 (`**(float**)(param_4+0x18) = unidad+0x20`, movss/mov 4 B vía puntero). Es escritura por puntero indirecto; un DR sobre cmd+0x10 la capturará igual.
- unidad+0x20 (carga): escritores = 0x3DCCCCCD en 0x1408AEEE0 (Clear press), AC9D0 (acumula), B2240 (reescribe `*(float*)(lVar10+0x20)=fVar19`) y AF110 (reset a 0).
- Validación: DR en cmd+0x1768; en campo propio debe escribir edi=4 desde 0x1408B204D; tras los parches A+B debe escribir edi=3.

## 5. Lo que NO se determinó

- El valor de `[mode+0x116C]` en Entrenamiento libre (1 vs 3+flag) y por qué en el experimento previo cmd+0x10 seguía en 0.0 con Clear acumulando +0x20 (B33F0 debería copiarlo si id==ordA, unidad+0x14!=0 y +0x20>0; no se explica estáticamente; posible muestreo fuera del fotograma, o B2A50 no alcanzado por B20D0). HIPÓTESIS.
- La fórmula final de potencia (FUN_140833CD0/FUN_1406D9540) a partir del parámetro de carga: no re-leída aquí; se asume proporcional a la carga (PROBABLE).
- Si fuera del campo propio Shoot dispara sin restricciones adicionales en entrenamiento: no se probó (el tiro natural fuera del área sí funciona según la observación del usuario).
- Efectos secundarios del parche A+B sobre otros roles (solo se revisó el rol 1/portador; B0160 rol 2 también empuja ids 3 y 4) y sobre porteros u otras unidades.
