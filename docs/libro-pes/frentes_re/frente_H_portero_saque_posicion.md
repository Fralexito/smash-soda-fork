# Frente H: portero en cancha chica (H1 saques demasiado fuertes, H2 portero dentro de la red al sacar, H3 salidas del portero)

Análisis estático de PES2021.exe (base 0x140000000; RVA = VA - 0x140000000). Solo lectura: bodies.pkl (decompilado), vftables (clases_pes.json + lectura de ranuras en el exe), bytes del .text vía fox.py/pe.py, frentes B, C, D, E, G. No se tocó ningún proceso. El informe del frente F (portero propio en Ataque) no existía al escribir esto; no se repitió.
Niveles: CONFIRMADO = visto en decompilado/bytes; PROBABLE = deducido con evidencia coherente; HIPÓTESIS = sin verificar.

## 0. Resumen

1. Hay DOS capas de clases de portero, y confundirlas hace perder tiempo:
   - Capa de decisión/orden (`match::player::ActionKeeper*`, 8 o 23 ranuras, código en 0x1409Cxxxx-0x1409Exxxx): decide a dónde se mueve el portero y a dónde/cuánto fuerte patea; escribe un contexto de orden `ctx` (param_3 de estas funciones). CONFIRMADO.
   - Capa de animación (`match::anime::action::goal_keeper::*`, Throw/PuntKick/AfterCatchBase/PickupBall/DropBall, código 0x140744xxx-0x14077xxxx): solo elige el clip por ángulo/distancia (FUN_140749E70, FUN_140748E70 = ángulo de giro al objetivo) y reproduce. No contiene fórmula de velocidad de pelota. CONFIRMADO.
2. H2: causa más probable = `ActionKeeperAfterCatchMove` (portero tras atrapar, camino a su punto de saque) usa `dims[6]+1,0` y `dims[7]-1,0` (bloque de dimensiones, floats +0x18 y +0x1C) como límite/objetivo en X/Z. En la cancha original dims[6]=36,14 (línea del área grande). Si el Core no reescala [6]/[7] el objetivo cae en |x|=37 -> el portero va al fondo, dentro de la red. Hipótesis verificable con una lectura en vivo (ver §2.2). El clamp a 14,5 que ya existe en `ActionKeeperBasePosition` NO cubre AfterCatchMove.
3. H1: la potencia del saque NO sale de una constante de la capa de animación. En la capa de decisión el portero fija `ctx+0xE68` (potencia 0..1, solo Punt) y el objetivo `ctx+0xE8C..0xE94` (Punt y Throw), con distancias en metros absolutos de cancha grande (potencia = (dist-3)/32 para órdenes tipo 4, saturando a 1,0 a 35 m). La conversión potencia/objetivo -> velocidad inicial de la pelota NO se localizó (§1.5). Se propone un hook por acción y un plan B genérico en el modelo de pelota.
4. H3: salida del portero = `ActionKeeperBasePosition` (FUN_1409DC040) -> FUN_1409DE8C0 (rama normal) con umbrales de distancia pelota-portería codificados en metros de cancha grande (17, 22, 27, 30, 35 m). En 30x20 la pelota siempre está dentro de 17-22 m de la portería, es decir, en el régimen de máxima agresividad. Hook propuesto: clamp de la posición de salida a R metros de la línea de gol (§3).

## 1. H1: saques del portero

### 1.1 Qué clases generan el saque (CONFIRMADO por RTTI + lectura de vtables)

| Capa | Clase | vtable | Funciones relevantes |
|---|---|---|---|
| decisión | `match::player::ActionKeeperPuntKick` | 0x142631D78 (8 ranuras) | ranura 4 = FUN_1409CEF10 (lee el mensaje de orden y escribe `action+0x20` potencia y `+0x24` ángulo); ranura 7 (update por fotograma) = **FUN_1409CE630** (2270 B) |
| decisión | `match::player::ActionKeeperThrow` | 0x142631DC0 | ranura 4 = FUN_1409D0950; ranura 7 = **FUN_1409D04A0** (1186 B) |
| decisión | `match::player::ActionKeeperThroughThrow` | 0x142631E08 | ranura 4 = FUN_1409D1350; ranura 7 = **FUN_1409D0E80** (1219 B) |
| decisión | `match::player::ActionGoalKickLong` | 0x142635708 | ranura 7 = FUN_140978D50 (214 B); no aplica en street (no hay saque de puerta) |
| decisión (IA/pad) | `match::pad::ThinkUnitKeeperAutoPuntkick` | 0x14260F1D8 | ranura 8 = FUN_1408AC500 (la misma máquina de estados de las ThinkUnit de frente D) |
| IA | `match::ai::bp::ThinkUnitGKClear/GKPassLong/GKPassShort/GKDribble` | 0x142BEF030/EF010/EEFF0/EEFD0 (3 ranuras) | Son las unidades evaluadoras de `match::ai::bp::BallPlayer` (la ranura 2 = evaluador; ver §H4). Deciden QUÉ hacer (pase corto, pase largo, despeje, conducir) y a QUIÉN; la acción ejecutora es luego Throw/PuntKick |
| animación | `match::anime::action::goal_keeper::Throw` (0x1425E0AA8, 32 ranuras), `PuntKick` (0x1425E08B8), `AfterCatchBase`, `PickupBall`, `DropBall` | | ranura 28 (FUN_1407743F0 Throw / FUN_14074A3E0 Punt) elige el clip por tabla `&UNK_1425E8980/E6F40` según ángulo y distancia; FUN_140749E70 lo reproduce. Ranura 29 devuelve el id de acción (Throw=0x42, Punt=0x41) |

`constant::BallplayerGk` (vtable 0x142B516C8, 3 ranuras, dtor FUN_141E70290 que restaura la vtable de `BallplayerGkBase` 0x142B4D958), `constant::Gk` (0x142B57CD8) y `constant::GoalKick` (0x142B58A18) existen como `ConstantData`, pero **no encontré ninguna referencia de código (lea ni puntero en .rdata/.data) a esas vtables** (la búsqueda de lea RIP-relativo y de qword apuntando a la vtable dio 0). Se instancian por un mecanismo de registro genérico que no seguí. Los valores de BallplayerGk, por tanto, NO se leyeron. HIPÓTESIS: ahí viven velocidad máxima de despeje/lanzamiento.

### 1.2 Qué escribe el portero al ordenar el saque (CONFIRMADO en decompilado)

FUN_1409CE630 (PuntKick, update) con `param_1`=acción, `param_2`=jugador/orden (idx en `param_2+4`), `param_3`=ctx:
- Dirección/objetivo: `action+0x10..0x18` (vec3 objetivo) y `action+0x24` (ángulo, `FUN_140A92950(180 - 90*dir)`); id de receptor `action+0x1C` (0xFF = nadie).
- Potencia: `action+0x20` (float). Es 1,0 en el modo auto (`ctx`/equipo con `+0xB8A2` y `action+0x20<=0`) o el valor del mando mantenido (viene de la orden). Cada fotograma copia: `ctx+0xE68 = action+0x20` (potencia), `ctx+0xE60 = action+0x24` (ángulo), `ctx+0xE74 = receptor`, `ctx+0xE8C..E94 = objetivo`.
- Antes de copiar el objetivo hace `FUN_140A544D0(dims,&objetivo,-1,-1)` (recorte de |x|<=dims[0]-1, |z|<=dims[1]-1). CONFIRMADO: el objetivo del Punt ya queda dentro de 14x9 en street. Aun así la pelota "atraviesa la cancha" porque no depende solo del objetivo (ver §1.5).
- Cuando no hay compañero (0xFF) y la distancia² a la cancha es > 2500 (`_DAT_1425C65F8`=50² m), apunta con `FUN_140A8F7C0(pos, ángulo, 50,0 m)` (`_DAT_14259BE34`): objetivo "a 50 m" que luego se recorta con dims.
FUN_1409D04A0 (Throw, update): solo escribe objetivo `ctx+0xE8C..E94` (copia de `action+0x10..0x18`) y `ctx+0xE74`, `ctx+0xFA9=0`. No escribe `ctx+0xE68`: la potencia del lanzamiento de mano NO está en este registro. `action+0x28` = ángulo.
FUN_1409D0E80 (ThroughThrow): idem con rama `ctx+0xFAC & 0x20` y `ctx+0xE68 >= 0,4` (`_DAT_14259B89C`) como condición de pase al hueco.

Origen de `ctx+0xE68` para órdenes de pase/despeje en general: FUN_140518FE0 (1392 B, es la ranura 4 de ActionClear/ActionCentering y la llaman las ranuras 4 de PuntKick/Throw/ThroughThrow, DropBall, Makeshift), caso 4 del mensaje: `power = (dist(jugador->objetivo) - 3,0) * 0,03125` si dist<35 (`_DAT_14259A760`=35, `_DAT_1425C5938`=1/32), 1,0 si dist>=35, 0 si dist<=3. Caso 3: la potencia viene de la orden (mando o IA). CONFIRMADO. Es decir, los umbrales 3 y 35 m y el divisor 32 son metros de cancha grande: en 30x20 un pase de 25 m (de lado a lado) ya da 0,69.

### 1.3 Frente D vs. este frente

Frente D (cmd+0x1768/ThinkUnitClear) trata de la potencia del tiro propio (Clear=id 4 pone la carga a 1,0 en FUN_1408B1490 rama 3). El portero NO usa ese camino (usa `match::player::ActionKeeper*` de la capa de decisión). No hay interacción conocida; el portero de IA (COM) tampoco pasa por los ThinkUnit del usuario salvo `ThinkUnitKeeperAutoPuntkick` (portero con saque automático).

### 1.4 Hook propuesto (capa de decisión; PROBABLE que reduzca la fuerza, ver §1.5)

Común a los tres: envolver la función original con la técnica de siempre (jmp al detour, trampoline con los bytes robados, `call trampoline` con 32 B de shadow space; al volver ejecutar la lógica y `ret`). Convención: rcx=param_1 (acción), rdx=param_2 (orden; idx jugador = `[rdx+4]`), r8=param_3 (ctx); devuelven AL=1 (Punt/Throw devuelven `undefined8` 1). Guardar r8 antes de llamar (volátil). Aplicar solo si el flag street/entreno-cancha-chica está activo (el mismo de `cancha_chica_free`) y solo para el jugador cuyo `[rdx+4]` % 11 == 0 (portero, PROBABLE).

| Función | VA | RVA | Prólogo (24 B) | Firma AOB única, sin RIP | Bytes a robar (jmp 14 B abs. / jmp 5 B rel32) |
|---|---|---|---|---|---|
| ActionKeeperPuntKick::slot7 FUN_1409CE630 | 0x1409CE630 | 0x9CE630 | `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC B8 00 00 00` | `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC B8 00 00 00 4C 8B 7A 38 4C 8B` (32 B) | 15 / 5 (frontera 5 = tras `48 8B C4 55`... verificar: `48 8B C4`=3, `55`=4, `53`=5) |
| ActionKeeperThrow::slot7 FUN_1409D04A0 | 0x1409D04A0 | 0x9D04A0 | `48 8B C4 55 53 57 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 98 00 00 00` | `48 8B C4 55 53 57 41 55 41 56 41 57 48 8D` (14 B, única) | 16 / 5 |
| ActionKeeperThroughThrow::slot7 FUN_1409D0E80 | 0x1409D0E80 | 0x9D0E80 | `48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70` | `48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 4C 8B` (22 B) | 14 / 5 |

Los tres prólogos empiezan por `mov rax,rsp` o `mov [rsp+20h],rbx` y guardan registros con `[rax+..]`: el trampoline debe invocarse con CALL desde el detour (no con JMP) para que `rax` valga el rsp del frame; ningún byte robado es RIP-relativo (verificado: sin `48 8D 05`/`8B 05` en los primeros 32 B de las tres).

Lógica post-original (tras volver; AL=1):
- PuntKick (FUN_1409CE630): `if (*(float*)(ctx+0xE68) > 0) *(float*)(ctx+0xE68) *= k` con k = 0,35-0,5 (configurable en JSON). Es estable (no se compone) porque la función recalcula `ctx+0xE68 = action+0x20` en cada fotograma.
- Throw / ThroughThrow: acercar el objetivo al portero: sea P = (`rec+0x55C`,`rec+0x564`) con `rec = *(raíz + (idx+0xBF)*0x28)` y T = (`ctx+0xE8C`,`ctx+0xE94`); si |T-P| > Rmax (p.ej. 7 m) hacer T = P + (T-P)*Rmax/|T-P|, y copiar también a `action+0x10..0x18`. Hipótesis: el lanzamiento calcula la velocidad para llegar al objetivo; acortarlo la reduce (no verificado, ver §1.5).

### 1.5 Plan B (genérico, independiente de dónde salga la potencia): tope de velocidad por último tocador

El modelo de pelota (frente B) tiene un consumidor de contactos/eventos `FUN_14086D6F0` (RVA 0x86D6F0, 8784 B, llamada por el bucle de BallControl vía vtable) que escribe `BD+0x70/74/78` (velocidad) y ya aplica topes (`_DAT_1425C48BC` = 19,44 m/s y `_DAT_14260883C` = 13,89 m/s vertical). En ese contacto guarda quién tocó: `*(int*)(BD+0x28C) = (short)evento+2` (idx de jugador, 0xFF = otro) y `BD+0x290`. Propuesta: envolver FUN_14086D6F0; al volver, si street y `BD+0x28C` es 0 u 11 (portero) y |v| > Vmax (p.ej. 8-9 m/s en 15x10) escalar `BD+0x70/74/78` por Vmax/|v| y poner `BD+0x294 |= 1` (bit0 = "recalcular predicción de 250 fotogramas", consumido por FUN_140872570; frente B). Estado: HIPÓTESIS (la semántica de `BD+0x28C` como "último tocador" es probable por las escrituras `case 2/4/7` pero no se vio un lector; y el golpe de pie/mano puede entrar por otra ruta que no pase por esta función).
- FUN_14086D6F0: VA 0x14086D6F0, RVA 0x86D6F0. Prólogo `48 8B C4 55 53 41 55 41 56 41 57 48 8D A8 48 FC FF FF 48 81 EC 90 04 00 00`. Firma única 14 B: `48 8B C4 55 53 41 55 41 56 41 57 48 8D A8` (sin RIP). Robar 18 B (jmp 14) o 5 B... frontera: 3,4,5,7,9,11,18 -> con jmp rel32 robar 5. Parámetro: rcx=BD (objeto "BD", +0x64 pos, +0x70 vel).
- Inconveniente: topa también un despeje del portero que va a un compañero (no hay forma de distinguir Throw/Punt de Deflect/Punch aquí); aceptable en street.

### 1.6 Lo que NO se pudo determinar en H1

- La función que convierte `ctx+0xE68`/`ctx+0xE8C` (o la animación) en la velocidad inicial de la pelota del portero. Se buscaron lectores de `+0xE68`, `+0xE8C`: solo aparecen en la capa de decisión (acciones) y un registrador de repetición (FUN_1421722D0, copia 0xE0C..0xEA8 al replay). El consumidor está tras una copia del ctx a otro objeto (p.ej. `ctx+0x10C8`/`+0x10DC` en FUN_14094E6F0). No lo seguí.
- Los valores de `constant::BallplayerGk` / `Gk` / `GoalKick` (sin referencias de código; no se hallaron en JSON).
- Si el saque de mano (Throw) usa potencia aparte: FUN_1409D04A0 no escribe `ctx+0xE68`; no está claro de dónde sale su velocidad (HIPÓTESIS: del objetivo y de un valor fijo de `BallplayerGk`).
- El saque "rápido" del usuario (mando) llega por mensaje caso 3 con potencia de la orden; no se trazó qué botón lo genera ni si pasa por PuntKick o por otra acción.
- Si `[S+0x116C]` en Entrenamiento libre vale 1 (varias funciones de §2 lo exigen).

## 2. H2: posición del portero al sacar (dentro de la red)

### 2.1 Quién lo coloca (CONFIRMADO leído; el vínculo con el síntoma es PROBABLE)

`match::player::ActionKeeperAfterCatchMove` (vtable 0x1426325E8, 23 ranuras) es la acción del portero tras atrapar:
- ranura 4 = **FUN_1409E8880** (1364 B): devuelve el vec3 objetivo en `param_2` (rdx). Solo calcula si `[S+0x116C]==1` (S = `*(raíz+0x270)`) y `[S+0x1168] != 9`; si no, devuelve la posición actual del portero.
- ranura 16 = **FUN_1409E8F50** (1251 B): paso de movimiento hacia el objetivo (usa ángulo y límites, escribe `param_5`).
- ranura 13 = FUN_1409E8DE0 (estado/tipo 2-4), ranura 20 = FUN_1409E86C0 (posición de referencia: `lVar3+0x55C` pos del jugador o `S+0x155C`).

Fórmula de FUN_1409E8880 (dims = `*(float**)(raíz+0x2AA8)`; dir = `*(char*)(equipo+0x254)` = ±1; xx = pos.x del portero):
```
f24 = dims[6] + 1,0          (offset +0x18)
f27 = -f24
si f27 <= xx*dir + 4,0:  x_obj = -(f24*dir)      ; el portero va al fondo de SU lado
si no:                   x_obj = xx + dir*4,0
z_obj: si |z| >= dims[5] (offset +0x14) -> z - 2*signo; y luego recorte a |z| <= dims[7]-1,0 (offset +0x1C)
```
y FUN_1409E8F50 usa `limite = (dims[6]+1,0) * -dir` y `dims[7]-1,0`. En la cancha original dims[6]=36,14 y dims[7]=20,12 (frente C, §tabla dimensiones), de modo que el portero iría al borde del área grande (|x|=37,1). En street, si el Core solo escribió [0],[1] (semilargo 15, semiancho 10) y dejó [6]=36,14, el objetivo es x=-dir*37,1: **fuera de la cancha y de la red** (el recorte posterior de 14,5 de `ActionKeeperBasePosition` no actúa aquí). Esto coincide con "se coloca tan atrás que se mete en el arco".

Qué dice el log del Core: «parchea el bloque de 36 B de dimensiones (2 bloques) … 15.00 x 10.00». No consta qué valores escribe en los índices [4..8]. NO DETERMINADO (por eso es PROBABLE y no CONFIRMADO).

Verificación barata (lectura en vivo): leer `*(float**)(raíz+0x2AA8)` y volcar 9 floats. Si [6] ≈ 36,14 o [7] ≈ 20,12 la hipótesis queda confirmada (y también explica por qué la sonda de frente G con 15x10 es suficiente para el resto, que solo usa [0],[1],[2],[3],[4]).

### 2.2 Soluciones

Opción 0 (la más limpia, sin hook): que el Core escale también los demás campos del bloque: [4] (47,06), [5] (?), [6] (36,14), [7] (20,12), [8] (41,56) por kx=dims[0]/52,56 (0,2854) y kz=dims[1]/34,06 (0,2936). Riesgo: las funciones del Juez (`FUN_140A72E70`, bits de área 0x10..0x4000 con dims[2..7]) y la zona de portero se calculan con esos campos; escalarlos acerca el "área" al tamaño de street y es coherente con lo que ya hace para [2],[3]. No probado.

Opción 1 (hook, salida): envolver FUN_1409E8880 y recortar la salida.
- FUN_1409E8880: VA 0x1409E8880, RVA 0x9E8880. Prólogo `48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC A0 00 00 00 0F 29 70 B8`. Firma única 22 B (sin RIP): `48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC A0`. Robar 14 B (frontera tras `41 55`; para jmp rel32 de 5 B robar 7 B: `48 8B C4 48 89 58 08`).
- Convención: rcx=action, rdx=out (vec3), r8=ctx de jugador (`[r8+4]`=idx); devuelve rax=rdx. Tras la original: si street, leer `x=[rdx]`, `z=[rdx+8]` y hacer `x = clamp(x, -14,5, 14,5)` y `z = clamp(z, -9,5, 9,5)` (14,5 = dims[0]-0,5 como ya hace la IA; 9,5 = dims[1]-0,5).
- Cubrir además FUN_1409E8F50 (ranura 16): `40 57 41 55 41 56 48 81 EC 00 01 00 00 48` (14 B, única, sin RIP; VA 0x1409E8F50, RVA 0x9E8F50; robar 14 B, frontera tras `41 56`... real: 2,4,6,13 -> robar 13 B `40 57 41 55 41 56 48 81 EC 00 01 00 00` + el siguiente `48 8B 05` es RIP: NO incluirlo, así que con jmp de 14 B hay que robar 20 B incluyendo `48 8B 05 DC 23 B2 02` relocatable; usar jmp rel32 de 5 B: robar 6 B `40 57 41 55 41 56`). El valor útil es el recorte de `param_5` (posición del portero que se pasa): menos importante si se corrige la ranura 4.
- Opción 1b (más general, coste mayor): recortar la salida de FUN_1409DC040 (ver H3) a |x|<=14,5 ya está; añadir este mismo clamp en AfterCatchMove cierra el hueco.

Nota sobre la tabla de reinicio (FUN_1409F5530/F5840, ya con Core hook): no intervienen en el post-atrape (solo reinicios por stoppage); no los toqué.

### 2.3 Lo que NO se pudo determinar en H2

- Valores reales de dims[4..8] en street (ver §2.1).
- Valor de `[S+0x116C]`/`[S+0x1168]` en Entrenamiento libre (si 0x116C != 1, FUN_1409E8880 devuelve la posición actual y la causa del síntoma sería otra acción: ActionKeeperPickupBall/Makeshift/SeenOff/DropBall; sus funciones de objetivo son FUN_140516740 (2001 B, ranura 18 de varias), FUN_140519550 (4089 B), FUN_140515610 (4389 B, SeenOff). No se leyeron).
- Si el portero llega a x=±37 y la colisión del Core lo detiene en 15,2 (muro) o lo deja pasar: no se sabe; la descripción "dentro del arco/red" sugiere que el muro no afecta a los jugadores.

## 3. H3: que el portero salga menos de su área (salida/rush/sweeper)

### 3.1 Mecanismo (CONFIRMADO lectura, PROBABLE interpretación)

`ActionKeeperBasePosition` (vtable 0x142632090, 23 ranuras), ranura 4 = **FUN_1409DC040** (5164 B; `param_2`=vec3 salida): despacha por modo `[S+0x116C]` y `[S+0x1554]`:
- Juego normal -> FUN_1409DE8C0 (3854 B, rama general) o, si `[equipo+0x255] != 0`, FUN_1409DF7D0 (3002 B, rama alternativa; PROBABLE: portero "barredor/activo" o controlado). Otras: FUN_1409DE4D0 (1001 B, posición fija a 1,5/135º), FUN_1409DD470 (4178 B, jugada a balón parado).
- FUN_1409E0390 (2141 B) calcula la posición base `fVar34` (x del portero) que FUN_1409DE8C0 usa; ahí estarán `gklBaseX/gksBaseX` (tabla `team+0x4B4C` slot 0). NO LEÍDA en detalle.
- FUN_1409DE8C0, final (líneas ~330-395): `x = ±dims[0]*t + (1-t)*xbase`, con `t = clamp((d - 22,0)*0,125, 0, 1)` y `d` = distancia pelota-portería; luego `FUN_140470EB0(pos, ángulo, dist)` adelanta al portero hacia la pelota con `dist = d*(1 - s)*7,0*… + …`, `s = clamp((d-17,0)/10, 0, 1)` hasta 27 m, y límites 35/45/50/55 m. Constantes (todas metros de cancha grande, en .rdata como floats compartidos): `_DAT_14259D614`=17, `_DAT_14259D618`=22,5, `_DAT_1425C50FC`=27, `_DAT_1425C48C0`=22, `_DAT_14259A760`=35, `_DAT_14259BE30`=45, `_DAT_14259BE34`=50, `_DAT_14259BE38`=55, `_DAT_14259BDF0`=7. No se pueden parchear en .rdata sin afectar a decenas de funciones.
- Al final de FUN_1409DC040 (etiqueta LAB_1409DC35C, CONFIRMADO) ya hay un clamp: `|x| <= dims[0]-0,5` (14,5 en street) en el X de salida. No hay clamp de salida hacia adelante: el portero puede avanzar hasta el centro.

Consecuencia: en 30x20 la pelota siempre cae en d < 22 m de la portería, por lo que el portero está en régimen de salida máxima de forma permanente.

### 3.2 Hook propuesto (clamp de la salida): `ActionKeeperBasePosition::slot4`

- FUN_1409DC040: VA 0x1409DC040, RVA 0x9DC040. Prólogo `48 8B C4 48 89 58 08 4C 89 48 20 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0`. Firma única 32 B, sin RIP: `48 8B C4 48 89 58 08 4C 89 48 20 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0 48`. Robar 15 B (frontera tras `48 89 50 10`; jmp rel32: robar 7 B `48 8B C4 48 89 58 08`).
- Args: rcx=action, rdx=out, r8=jugador/orden (idx = `[r8+4]`), r9=param_4. Devuelve rax=out (vec3 float en `[out+0]`,`[out+4]`,`[out+8]`).
- Tras la original (street y portero propio o ambos): dir_lado s = signo de la x devuelta (el portero siempre está en su mitad; si x==0, usar el signo de la x actual del jugador `rec+0x55C`); `xa = |x|`; `xa = clamp(xa, 15,0 - R, 14,5)` con R = 3,5 (parámetro JSON `gk_salida_m`, rango 3-4); `x = s*xa`. En Z: `z = clamp(z, -Zmax, Zmax)` con Zmax = 5,0 (ancho de la boca 3,66 + 1,3). Solo se modifica el vec3 devuelto; el resto de la acción no cambia (el portero camina al punto recortado).
- Interruptor: bandera JSON `gk_clamp_salida: true/false` leída por el Core; sin ella el hook llama a la original y retorna.
- Alternativa equivalente "escalar": en lugar de clamp absoluto, `xa = 15 - (15 - xa)*kx_gk`, con kx_gk = 0,285 (la salida original desde la línea de gol se escala al tamaño de la cancha), pero el clamp absoluto es más predecible.

El mismo recorte hay que aplicarlo a FUN_1409E8880 (H2) para que la salida tras atrapar no se vaya hacia delante/atrás.

### 3.3 Lo que NO se pudo determinar en H3

- Cuál de las dos ramas (DE8C0 vs DF7D0) usa el portero propio con 4 usuarios en entrenamiento (`[equipo+0x255]`).
- La lógica fina de FUN_1409E0390 (`gklBaseX/gksBaseX`) y la tabla `team+0x4B4C` slot 0: no es necesaria para el clamp, pero sí si se prefiere escalar la base en origen.
- El valor concreto de R que da buen juego (3-4 m es una sugerencia; ajustar con prueba).

## 4. Resumen de hooks (todos sin RIP en la firma)

| Uso | Función | VA | RVA | Firma AOB | Robar (jmp 14 / rel32) |
|---|---|---|---|---|---|
| H3 salida base | FUN_1409DC040 | 0x1409DC040 | 0x9DC040 | `48 8B C4 48 89 58 08 4C 89 48 20 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0 48` | 15 / 7 |
| H2 punto tras atrapar | FUN_1409E8880 | 0x1409E8880 | 0x9E8880 | `48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC A0` | 14 / 7 |
| H1 potencia Punt | FUN_1409CE630 | 0x1409CE630 | 0x9CE630 | `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC B8 00 00 00 4C 8B 7A 38 4C 8B` | 15 / 5 |
| H1 objetivo Throw | FUN_1409D04A0 | 0x1409D04A0 | 0x9D04A0 | `48 8B C4 55 53 57 41 55 41 56 41 57 48 8D` | 16 / 5 |
| H1 objetivo ThroughThrow | FUN_1409D0E80 | 0x1409D0E80 | 0x9D0E80 | `48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 4C 8B` | 14 / 5 |
| H1 plan B | FUN_14086D6F0 | 0x14086D6F0 | 0x86D6F0 | `48 8B C4 55 53 41 55 41 56 41 57 48 8D A8` | 18 / 5 |

Verificadas únicas en .text por `fox.unique_sig` (patrón con comodines para rel32/disp RIP; en ninguna hay `??` en los primeros 32 B).

## 5. Pruebas en vivo recomendadas (lectura, sin escribir)

1. Volcar los 9 floats de `*(float**)(raíz+0x2AA8)`: confirma/descarta §2.1 (dims[6]).
2. Leer `[S+0x116C]` y `[S+0x1168]` (S = `*(raíz+0x270)`) en Entrenamiento libre: si 0x116C != 1, la causa de H2 es otra acción.
3. Con DR de hardware sobre `ctx+0xE68` del portero (ctx = `[player+...]`, visible en la llamada a FUN_1409CE630 como r8) y sobre `BD+0x70`: ver quién escribe la velocidad en el instante del saque. Eso cerrará §1.6.
4. Tras instalar el hook de H3, comprobar que `rec+0x55C` del portero (slot 0, idx 0 y 11) no pasa de |x|=14,5 ni baja de |x|=11,5.

## 6. Qué se leyó

bodies.pkl: FUN_140749130, 140748B20, 140749200, 140749E70, 140748E70, 140748CC0, 1406DE990, 140774330, 1407743F0, 14074A2D0, 14074A3E0, 14074A2A0, 14074A250, 1409CE630, 1409CEF10, 1409D04A0, 1409D0950, 1409D0E80, 140518FE0, 1409DC040, 1409DE8C0 (parte), 1409E8880, 1409E8DE0, 1409E86C0, 1409E8F50, 14086D6F0 (parte), 1440F35F0, 1421722D0, 140833CD0 (solo llamadas), 1406D9540 (solo estructura: es cálculo de distancia a portería, no de potencia); frentes B, C, D, E, G (secciones relevantes); clases_pes.json (nombres y ranuras de todas las clases ActionKeeper*, goal_keeper::*, constant::*); lectura de ranuras de vtables directamente del exe; floats de .rdata de las constantes citadas.
Corrección a la premisa: FUN_1406D9540 NO calcula potencia sino distancia a portería; FUN_140833CD0 es la selección de animación del Kick de campo (45 KB), no la velocidad; FUN_14080EDE0 no se relaciona con el saque.

## H4. Decisión de la IA del portero al sacar: por qué elige largo y cómo forzar corto (arquero_saque_corto)

### H4.1 Mecanismo (CONFIRMADO lectura salvo lo marcado)

- Las ThinkUnit `ThinkUnitGK*` son unidades del motor de decisión `match::ai::bp::BallPlayer` (vtable 0x142BEF090; ctor FUN_14210D320; hay 25 unidades, ids 0..24; id -> unidad por el switch FUN_1421105B0). Ids de portero (CONFIRMADO, leído del jump table de FUN_1421105B0 y de los offsets del ctor): **18 = GKClear (+0x3D00), 19 = GKDribble (+0x3B30), 20 = GKPassShort (+0x3C70), 21 = GKPassLong (+0x3CB8)**, 22 = GKCoaching, 23 = OnePattern.
- Cada fotograma el driver FUN_142110D30 (1143 B) llama FUN_142110370: para cada id en orden de prioridad (tabla `BallPlayer+0x3E20`, 25 entradas) evalúa la unidad (FUN_142111610 -> FUN_14211EEC0 -> ranura 2 de la unidad, que escribe el registro de decisión `unit+0xC`: `[0]=tipo` (2 = pasar a jugador), `[2]=idx receptor`, `[3]=flag`). Después FUN_14210F110 (4539 B) recorre la lista por prioridad y se queda con la primera decisión válida (filtros por modo `[S+0x116C]`, tipo de zona, etc.). Si no hay decisión y el portero es quien tiene el balón (`FUN_140A638C0`), FUN_142110D30 evalúa por defecto el id 19 (GKDribble) o el 16.
- Evaluadores de portero (ranura 2 de cada vtable):
  - GKPassShort **FUN_1421333F0** (215 B): si el contador de presión `(BP+0x2124)[...]+0x10 > 1` exige flag `state+0x6C` y que el tiempo `state+0xC >= fps` (1 s); luego `receptor = FUN_142132660(...)`; si `receptor < 0x16` -> decisión tipo 2 a ese receptor.
  - GKPassLong **FUN_142133120** (711 B): `bVar12` = (pronóstico de posición del portero vs. atacante rival a distancia² < 225 (`_DAT_14259BE70`) en fps/4 fotogramas) **o** tiempo con balón > 3,0 s (`_DAT_14258C264`=3 x fps). Si hay presión o pasaron 3 s, elige receptor lejano con FUN_142132020 (1587 B) y decide tipo 2 con flag 1. Si no hay presión espera hasta 1 s / 2,5 s (`_DAT_14259BDE4`) y vuelve a decidir.
  - GKClear **FUN_142132B40** (722 B): devuelve tipo 5 (despeje) cuando el rival más cercano está dentro de 12-15 m (`_DAT_14259BE04`/`BE0C`) del balón; tipo 6 en otra rama.
  - GKDribble **FUN_142132E20** (754 B): conducir. No leído en detalle.
- Selección de receptor corto, FUN_142132660 (1245 B, CONFIRMADO lectura): para cada compañero j (mismo equipo, j != portero, 11 candidatos) exige: en el cuadro de IA de equipo (`world+0xB6C`) estado ==0xB (disponible), una medida de libertad/presión `rec+0xE8/+0xEC` por encima de umbrales (>= 15, 90 ó 140, 20 según un flag), cuenta de rivales (`BP+0x2124`) < 2 ó 3, y **distancia² del compañero al punto previsto del balón entre 25 y 1600** (`_DAT_14259BE20`=25, `_DAT_14259CE40`=1600, es decir 5 m a 40 m), más un test de ángulo (>80º descarta si el balón sale del campo). Devuelve el mejor idx o >=0x16 (ninguno).
- Consecuencia probable (PROBABLE, no observado en vivo): en 30x20 los umbrales de libertad/presión (15, 90, 140, 20 en unidades de IA de cancha grande) casi nunca se cumplen con rivales a 5-10 m, así que GKPassShort no encuentra receptor; pasa 1-3 s con balón y GKPassLong (o Clear) se activa por tiempo/presión -> patada larga. Es exactamente el síntoma descrito.

### H4.2 Soluciones

Solución A (parche de 1 byte, la más simple; recomendada): anular las unidades Largo y Despeje del portero en el entrenamiento chico, para que solo queden PassShort/Dribble. En el evaluador ponemos `C3` (ret) como primer byte; las funciones son `void` y solo escriben `param_2` cuando deciden, así que sin escribir quedan "sin decisión".
- GKPassLong FUN_142133120: VA 0x142133120, RVA 0x2133120. AOB única 26 B sin RIP: `48 89 5C 24 18 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 83 EC 50 49 8B 00 33`. Parche: byte +0: `48` -> `C3`.
- GKClear FUN_142132B40: VA 0x142132B40, RVA 0x2132B40. AOB única 22 B sin RIP: `48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60 49 8B`. Parche: byte +0: `48` -> `C3`.
- (Opcional) GKDribble FUN_142132E20, RVA 0x2132E20, AOB 26 B: `48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 81 EC B0 00 00 00 49 8B 00`; no tocar salvo que el portero conduzca fuera.
- El Core restaura el byte original (`48`) al salir del entreno. JSON: `arquero_saque_corto: true/false`.
- Riesgo: si PassShort no encuentra receptor el portero se queda con el balón hasta que lo fuerce otra regla (conducción GKDribble o timeout de 6 s de las reglas del árbitro / `ThinkUnitKeeperAutoPuntkick` en el portero de usuario). Por eso conviene la Solución B.

Solución B (complemento; da "si no hay compañero libre, saca corto igual"): hook de FUN_142132660 (selección de receptor corto) con fallback.
- FUN_142132660: VA 0x142132660, RVA 0x2132660. AOB única 26 B sin RIP: `48 8B C4 48 89 58 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0 02 00 00 0F`. Robar 14 B (frontera tras `41 55`; con jmp rel32 5 B: robar 7 B `48 8B C4 48 89 58 10`).
- Firma: `uint f(ctx* rcx)`; ctx[0] -> int* idx del portero; ctx[1] -> objeto IA; ctx[5] -> mundo IA. Devuelve idx receptor (EAX) o >=0x16.
- Lógica del hook: llamar a la original; si EAX < 0x16 devolver; si EAX >= 0x16 y street y `arquero_saque_corto`: elegir el compañero j en [side*11 .. side*11+10], j != idx_portero, que minimice la distancia a la posición del portero (usar `rec = *(raíz+(j+0xBF)*0x28)`, pos en `rec+0x55C/+0x564`) con distancia >= 3 m; devolver j. Opcional: preferir j con distancia al rival más cercano mayor.
- Con la Solución A, además de esto, el flujo es: PassShort (con fallback) -> tipo 2, receptor cercano -> acción Throw/PuntKick con objetivo cerca -> potencia (dist-3)/32 baja (§1.2): corto y suave.

Solución C (alternativa sin hooks de IA): subir la bandera de tiempo: no existe constante parcheable seguro (todas las constantes son floats compartidos en .rdata).

### H4.3 Relación con H1 (para no duplicar)

- H4 decide el TIPO y el RECEPTOR (corto cercano vs. largo lejano). H1 (§1.4) limita la POTENCIA/objetivo de la acción ya elegida (Punt: `ctx+0xE68`, objetivo `ctx+0xE8C`). Con H4-A+B el objetivo es un compañero cercano, y la potencia por distancia ya será baja de forma natural ((d-3)/32), por lo que el hook H1 pasa a ser una red de seguridad (si el portero de usuario saca a mano o por mando "rápido", H1 sigue siendo necesario, porque el portero de usuario NO pasa por BallPlayer GK units sino por mando/`ThinkUnitKeeperAutoPuntkick`).
- Plan B de H1 (tope de velocidad por último tocador) cubre los despejes de ambos tipos de portero.

### H4.4 Lo que NO se pudo determinar en H4

- Cuál es la acción final para un decisión tipo 2 del portero (Throw vs. PuntKick vs. ThroughThrow): la elección entre ellas está en el traductor decisión->acción (no localizado; las ranuras 4 de las tres acciones leen un mensaje con tipo 3/4). Con receptor cercano lo más probable es Throw a mano.
- Orden exacto de prioridad de la tabla `BallPlayer+0x3E20` (se rellena en otra función; no leída): la Solución A no depende de él.
- Si el parche `C3` deja un registro de decisión viejo en una unidad que ya había decidido antes del parche (aplicar al entrar al entreno).
- Qué hace exactamente la unidad id 16 (`+0x33A0`) usada por defecto.
- Cómo se comporta el portero de USUARIO (mando): no usa estas unidades (no confirmado); se cubre con H1.
