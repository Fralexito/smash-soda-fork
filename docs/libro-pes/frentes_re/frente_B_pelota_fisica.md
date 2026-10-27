# Frente B: cuerpo físico y velocidad de la pelota (análisis estático de PES2021.exe)

Método: solo lectura de `bodies.pkl` (decompilado Ghidra), `clases_pes.csv`, `callers2.pkl`, bytes del exe vía `fox.py`/`pe.py`. No se usó Ghidra headless, ni depurador, ni se tocó ningún proceso. Base 0x140000000; RVA = VA - 0x140000000.
Niveles: CONFIRMADO = leído directamente en el código; PROBABLE = deducido de varias lecturas coherentes; HIPÓTESIS = inferencia sin lectura directa.

## 0. Resumen ejecutivo

1. **La pelota del partido NO es un cuerpo rígido de fox::ph / AndPh.** Su movimiento lo calcula un modelo analítico propio (gravedad 9,80665, arrastre, Magnus por efecto, rebote en el plano del suelo, rodadura) que integra un "estado de pelota" S de 272 B (0x110). CONFIRMADO (lectura completa de FUN_140876360). `cobra::And::*` es el módulo de animación/ragdoll de personajes (AnimControl, Ik, Ragdoll); su filtro `PhPreProcessJob` y los `.geom` de portería no intervienen en el paso de la pelota (PROBABLE; no se encontró ninguna referencia desde el código de la pelota).
2. **No hay resolución de colisión pelota contra geometría estática (.geom) en el modelo de la pelota.** El único "choque" del integrador es contra el plano del suelo (y = radio ≈ 0,1087 m) y un suelo hundido (-2,44 m) fuera de límites en cierto estado. Por eso mover los `.geom` no frena la pelota. CONFIRMADO para FUN_140876360; no se localizó qué (si algo) emite los contactos con postes/travesaño (ver §6).
3. **La pelota se simula por adelantado 250 fotogramas** (historial/predicción `BallInfo`). Cada fotograma se avanza 1 paso al final de la cola y se desplaza el historial; cuando un golpe cambia la velocidad se recalculan los 250 pasos. Hay que enganchar el **paso único** (`FUN_140876360`), porque lo usan ambas rutas.
4. **Punto de enganche recomendado: `FUN_140876360` (RVA 0x876360, VA 0x140876360), firma `48 8B C4 F3 0F 11 40 08 55 53 56 48 8D 68 88 48 81 EC 60 01 00 00` (22 B, sin reubicaciones RIP; única en .text).** Robar los 15 primeros bytes. Envoltorio: llamar a la original y después multiplicar S[3..5] (velocidad) si la posición S[0..2] está dentro de la red.

## 1. Cadena de punteros y offsets (objeto pelota -> estado -> velocidad)

| Elemento | Dirección / offset | Contenido | Confianza |
|---|---|---|---|
| Singleton `match::BallControl` | variable global `exe+0x36F4090` (VA 0x1436F4090), la escribe el constructor FUN_1404245E0 (`puRam00000001436f4090 = param_1`). vtable 0x1425988B0 (RVA 0x25988B0, 18 ranuras) | instancia de la clase "pelota" del partido | CONFIRMADO |
| Actor de pelota "BD" | `*(BallControl + 0x20)` | objeto grande (>0x580 B; contiene referencias de registro en +0x4E0, +0x508, +0x530...) | CONFIRMADO (FUN_140425A90 lo lee de param_1+0x20 y lo pasa a FUN_140872570/172170/872C10) |
| BD + 0x64 / 0x68 / 0x6C | posición (x, y vertical, z) actual interpolada, float | la escribe FUN_140870E90 a partir del historial | CONFIRMADO |
| **BD + 0x70 / 0x74 / 0x78** | **velocidad lineal (m/s)** (desplazamiento por fotograma * escala `exe+0x351AE54`, 54,0 estático) | FUN_140870E90 la deriva del historial; FUN_14086D6F0 (consumidor de eventos de golpe) la recorta con tope 13,89 m/s en una rama y la fija tras golpes | CONFIRMADO (lecturas), semántica "velocidad" PROBABLE |
| BD + 0x7C..0x84 | vec3 de giro/velocidad angular (copia del vec3 del historial) | | PROBABLE |
| BD + 0x64 .. 0x174 (272 B) | **estado "semilla" S0**; se copia a una pila y se pasa a FUN_140876020 para recalcular los 250 pasos | | CONFIRMADO (FUN_140872570) |
| BD + 0x170 .. 0x280 (272 B) | **estado "cola" S_tail**: lo avanza FUN_140876200 1 paso por fotograma | | CONFIRMADO (FUN_140870E90: `FUN_140876200(param_1 + 0x170, BallInfo)`) |
| BD + 0x294 (bits) | bit0 = "reiniciar predicción" (FUN_140872570 lo consume y recalcula); bit1, bit2, bit3 = otros avisos | | CONFIRMADO |
| BD + 0x298 | estado de pelota (2 tras golpe) | | PROBABLE |
| BD + 0x1E4 / 0x1E8 | S[0x1d] byte "en el suelo"; S[0x1e] tipo de superficie (0..5) que indexa las tablas de la config | | CONFIRMADO (FUN_140876360 lee param_4[0x1e]; FUN_1408723D0 escribe BD+0x1E8) |
| **BallInfo** | `*( *(exe+0x36F3DC8) + 0x40 )` (global del mundo del partido `_DAT_1436F3DC8`); también accesible como registro `BallInfo` en `registry + 0x2970` (FUN_1404335E0 = getter de ref) | historial de 250 entradas de 40 B (0x28) desde BallInfo+0x88: [+0x00 pos xyz][+0x0C cuaternión xyzw][+0x1C vec3]; entrada 0 = ahora, 249 = cola (BallInfo+0x2770). Getter FUN_140A6CF70(info, i) = info + 0x88 + 40*i (i>=250 -> entrada 249) | CONFIRMADO |
| BallInfo + 0x2948/0x294C/0x2950 | copia de BD+0x70..0x78 (la rellena FUN_140872C10) | | CONFIRMADO |
| BallInfo + 0x2954, +0x2960 | copias de BD+0x04..0x0C y BD+0x10..0x18 | | CONFIRMADO |

### Layout del estado S (float[68], 272 B) -- CONFIRMADO por FUN_140876360
- S[0], S[1], S[2]: posición. **Ejes: x = largo del campo, y = vertical, z = ancho.** (y se compara con el radio 0,10869 m en `exe+0x2598958`; límites exterior |x|>60,0 `exe+0x259A764` y |z|>39,7 `exe+0x2608840`.)
- S[3], S[4], S[5]: **velocidad lineal** (vx, vy, vz). Integración semi-implícita: `v += a*dt; p += dt*v`.
- S[6], S[7], S[8]: velocidad angular (efecto).
- byte S[0x1d] (offset 0x74): contacto con suelo; int S[0x1e] (offset 0x78): superficie; S[0x1f..0x22] (0x7C..0x88): cuaternión.
- Aceleración: FUN_140874470 (RVA 0x874470, 895 B): arrastre ∝ velocidad, `- g` (9,80665 en `exe+0x25A9678`) en y, más término Magnus (giro x velocidad).
- Paso temporal dt = 1/f; f = float en `exe+0x351AE54` (54,0 en .data estático; HIPÓTESIS que en ejecución valga 54 o 60, no verificado en vivo).

## 2. Función que escribe la velocidad cada fotograma

| Función | RVA / VA | Papel | Confianza |
|---|---|---|---|
| **FUN_140876360** | 0x876360 / 0x140876360, 4400 B | paso único del modelo de pelota (dt en xmm0, rdx=0, r8=1, **r9 = S**, [rsp+0x28] = config de física, [rsp+0x30] = flag "primer paso"). Es EL escritor de S[3..5] | CONFIRMADO |
| FUN_140873EE0 | 0x873EE0 / 0x140873EE0, 535 B | bucle: copia S a un temporal de pila, llama callbacks y N veces FUN_140876360. Solo lo usan los dos constructores de abajo (es decir, solo la pelota real, no la IA) | CONFIRMADO |
| FUN_140876200 | 0x876200, 343 B | "UpdateLast": 1 paso sobre S_tail (rcx = BD+0x170) y empuja la entrada 249 del historial | CONFIRMADO |
| FUN_140876020 | 0x876020, 467 B | "UpdateAll": 250 pasos desde la semilla, rellena todo el historial, calcula puntos de caída/cima | CONFIRMADO |
| FUN_140872570 | 0x872570, 1690 B | si BD+0x294 bit0: copia BD+0x64.. a S0 y llama FUN_140876020 | CONFIRMADO |
| FUN_140870E90 | 0x870E90, 3256 B | por fotograma: interpola el historial, escribe BD+0x64 (pos), BD+0x70 (vel), llama FUN_140876200 y desplaza el historial (FUN_140A6CE90/CEC0) | CONFIRMADO |
| FUN_140424BC0 | 0x424BC0, 197 B | ranura 2 de BallControl; llama FUN_140870E90 | CONFIRMADO |
| FUN_140425A90 | 0x425A90, 818 B | llamada desde FUN_140424D70/ranura 4; llama FUN_140872570 y FUN_140872C10 (copia BD -> BallInfo) | CONFIRMADO |
| FUN_14086D6F0 | 0x86D6F0, 8784 B | consume la cola de eventos de golpe (`*(ctx+0x2c0)+0x20`, tipos 2,4,6,7,9,0xA,0xB,0xD,0xE,0x10), fija velocidad y marca BD+0x294 | CONFIRMADO (lectura parcial) |

Otros llamadores de FUN_140876360 (IA que simula trayectorias): FUN_140731D80, 140793D40, 140817AF0, 1408183C0, 1408397A0, 14085C550, 14085E1C0, 14085E3C0, 140861170, 140862AE0, 140862CE0, 140865F20, 140866B60, 140868500, 140868AD0, 140869500, 140869D10, 14086A3F0, 14086AB90, 14086B690, 140943E90. (Mismo S; si se amortigua dentro del paso también afecta a la IA salvo que se filtre; ver §4.)

### Instrucciones de escritura dentro de FUN_140876360 (para puntos de escritura de hardware / DR0-3)
Hallado por búsqueda de bytes `F3 [44] 0F 11` con base rsi (=&S[3]) y rbx (=S):
- Velocidad (rsi, +0, +4, +8): RVA 0x876524/0x87652C/0x876535 (integración v += a*dt); 0x87732C/0x877331/0x877336 (salida con rebote/suelo); 0x877385/0x877389/0x87738E (sin contacto: v = (p - p_prev)/dt); 0x877457/0x87745B/0x877460 (rama param_6, snap).
- Posición (rbx, +0, +4, +8): 0x87655A/0x87655E/0x876563 (p += dt*v); 0x87682D (S[1] = suelo); 0x87743F/0x877443/0x877448.
- Otros escritores esperables de la velocidad "viva": FUN_140870E90 (BD+0x70/74/78, tres `movss [reg+70h/74h/78h]`), FUN_14086D6F0 (golpes; escalado por tope 13,89), FUN_1408722C0 (pone BD+0x70/78 a cero al limpiar), copias de 272 B por `FUN_140731BB0` (movups). Para un DR sobre BD+0x70 espera sobre todo FUN_140870E90 cada fotograma.
- Limitación: no se desensambló a nivel de instrucción más allá de estas búsquedas por bytes; no hay lista completa de instrucciones.

## 3. Resolución de colisión contra geometría estática

- **Dentro del modelo de pelota: solo el plano del suelo** (y <= `exe+0x2598958` = 0,10869; rebote con coeficientes de la config, indexados por S[0x1e]; suelo alternativo `exe+0x2608C0C` = -2,441 cuando `*(short*)(*(_DAT_1436F3DC8+0x2C0)+0x1A)==10` y |x|>60 o |z|>39,7). CONFIRMADO.
- No hay llamadas a fox::geo ni colisión con `.geom` en FUN_140876360, FUN_140874470, FUN_140873EE0, FUN_140876200, FUN_140876020. CONFIRMADO.
- El mundo `fox::ph` (RigidBody vtable 0x2A53DC8, RigidBodyCore 0x2A5CB00, IslandSolverJob 0x2A5FD98, IslandGeoCollisionJob 0x2A5E7F8 que usa `fox::geo::GeoStaticCollision` 0x29F24B8) lo instancia `cobra::And::PhManager` (ctor FUN_141AEA910, gravedad por defecto -9,8 en `WorldParam`, exe+0x2A59F64). Los cuerpos primitivos (FUN_141B98150 -> FUN_141B0D930/0D830/0E050) son del módulo de animación. HIPÓTESIS: sirven para ragdolls y cloth de personajes/redes, no para la pelota. No se trazó qué crea cuerpos en ese mundo.
- Eventos de contacto: la cola `*(ctx+0x2C0)+0x20` (nodos con tipo en +4, posición en +0xC..0x14, vector en +0x18.., +0x24) alimenta a FUN_14086D6F0; los productores (FUN_140791EC0, 140793D40, 140943E90 la leen; el que la llena no se localizó). El tipo 0xA se compara contra un puntero de portería (`FUN_14086FF30`, `_DAT_142608844`), así que podría ser contacto con poste/portería. HIPÓTESIS, no confirmado.

## 4. Punto de enganche recomendado

### Gancho principal: paso único
- Función: `FUN_140876360`, **RVA 0x876360**, VA 0x140876360.
- Firma AOB (22 B, todo fijo, sin desplazamientos RIP ni rel32; verificada única en .text): `48 8B C4 F3 0F 11 40 08 55 53 56 48 8D 68 88 48 81 EC 60 01 00 00`
- Bytes a robar: **15** (`48 8B C4 | F3 0F 11 40 08 | 55 | 53 | 56 | 48 8D 68 88`), frontera de instrucción en 15, relocatables (sin RIP-relativo). Retorno en +15.
- Convención: `void step(float dt /*xmm0*/, u64 a2 /*rdx*/, u64 a3 /*r8*/, float* S /*r9*/, void* cfg /*[rsp+0x28]*/, u64 flag /*[rsp+0x30]*/)`. Retorna void.

### Segundo gancho para limitarlo a la pelota real (opcional)
- `FUN_140873EE0`, RVA 0x873EE0, firma 17 B: `48 8B C4 4C 89 48 20 55 57 41 56 41 57 48 83 EC 58` (frontera en 17, relocatable, única). Sus únicos llamadores son UpdateAll/UpdateLast, o sea solo la pelota real. Un flag de hilo `in_ball_sim` puesto a 1 antes de llamar a la original y a 0 después, y comprobado en el gancho del paso, evita afectar a las simulaciones de la IA.
- Alternativa de reconocimiento de S real: `FUN_140876200` (RVA 0x876200, firma 22 B `48 8B C4 55 48 8D A8 F8 FE FF FF 48 81 EC 00 02 00 00 48 C7 45 D0`) recibe en rcx = BD+0x170, de ahí se obtiene BD = rcx - 0x170. No sirve como único gancho: no cubre el recálculo de 250 pasos.

### Esquema C++ (HIPÓTESIS de uso; no probado)
```cpp
// S: float[68]. Ejes: [0]=x largo, [1]=y arriba, [2]=z ancho. [3..5]=velocidad.
static void __fastcall hk_step(float dt, uint64_t a2, uint64_t a3, float* S, void* cfg, uint64_t flag) {
    orig_step(dt, a2, a3, S, cfg, flag);          // deja S ya integrado
    if (!tls_in_ball_sim) return;                  // opcional (ver gancho 2)
    float ax = fabsf(S[0]);
    if (ax > 15.0f && fabsf(S[2]) < 3.66f + 0.3f && S[1] < 2.44f + 0.3f) {
        S[3] *= k; S[4] *= k; S[5] *= k;           // k por paso (no por cruce): 0.1 es muy brusco por paso; usar p. ej. 0.85-0.95 si se quiere deceleración continua
    }
}
```
Notas: (a) se aplica a cada paso, también dentro de los 250 pasos de la predicción, por tanto el historial (lo que se dibuja) queda coherente; (b) el integrador recalcula la velocidad a partir de la posición cuando no hay rebote (0x877385), pero lo hace después de integrar con la velocidad previa, así que escalar S[3..5] tras el paso persiste en el siguiente; (c) el estado S es de 272 B y se copia de vuelta a BD+0x170 al terminar FUN_140873EE0.

### Alternativa solo-datos (sin gancho)
Escribir BD+0x70/0x74/0x78 (velocidad) y poner el bit0 de BD+0x294 fuerza que FUN_140872570 recalcule los 250 pasos desde BD+0x64 con la nueva velocidad. PROBABLE (deducido de FUN_140872570/FUN_140870E90); condición de carrera con el hilo del juego; no probado.

## 5. Qué esperar al afinar puntos de escritura de hardware (DR0-3)
- Sobre BD+0x70/74/78: escritor dominante cada fotograma = FUN_140870E90 (tres `movss`/`mov`); en golpes FUN_14086D6F0.
- Sobre S_tail (BD+0x17C..0x184 = vel): FUN_140876360 en 0x876524.., 0x87732C.., 0x877385.. (RVA arriba), por cada paso; y la copia de 272 B (FUN_140731BB0) al terminar FUN_140873EE0.
- Sobre BallInfo + 0x2770 (entrada 249): callback `BallInfoUpdateFutureDataLast` ranura 5 (FUN_140873E40) y UpdateAll por índice (FUN_1408743A0).
- Para localizar BD en ejecución sin depurador: leer `[exe+0x36F4090]` (BallControl) y luego `[+0x20]`. Para BallInfo: `[[exe+0x36F3DC8]+0x40]`; posición actual = BallInfo+0x88, velocidad por diferencia entre entradas 0 y 1 multiplicada por la escala (`exe+0x351AE54`).

## 6. Lo que NO se pudo determinar
- Relación del objeto de 640 B del usuario (firma +248=0, +312<=1, +492=0x10001/0x80001, pos en +0x70) con BD: BD NO es de 640 B (>0x580) y su +0x70 es la VELOCIDAD, no una traslación. El objeto de 640 B es otro (HIPÓTESIS: el nodo de escena/render o un objeto de red de mensajes; BallImpl `cobra::rc::scene::BallImpl` vtable 0x2C6B048 es una orden de render creada por FUN_141ECB530 con id 0x55E, sin física). No se encontró la llamada que lo rellena.
- Qué genera los contactos con poste/travesaño/red (productor de la cola de eventos) y qué hace que en el juego original la pelota se quede quieta en la red (¿animación `GoalnetFilm`/estado de gol?). No hay ningún choque pelota-geometría estática en el modelo.
- Valor real en ejecución de dt (`exe+0x351AE54`) y si la config de física (`FUN_141E5CA20(_DAT_1437F4808, 4) + 8`, tablas de coeficientes) es modificable.
- No se verificó en vivo ninguna de estas direcciones (todo estático); no hay lista de instrucciones completa (solo búsquedas por bytes en FUN_140876360).
- Qué usa el mundo AndPh con `PhPrimitiveCollisionCallback` (vtable 0x2ABF3D0) ni si hay un cuerpo de pelota allí.

## 7. Anexo: lo que se leyó
FUN_140876360 (completa), FUN_140874470 (mitad), FUN_140873EE0, FUN_140876020, FUN_140876200, FUN_140872570 (inicio), FUN_140872C10 (inicio), FUN_140870E90 (parcial), FUN_1408723D0, FUN_14086D6F0 (parcial), FUN_140425A90, FUN_1404245E0, FUN_140873E40, FUN_1408743A0, FUN_140875E60, FUN_140A6CF70 (bytes), FUN_141E532D0 (bytes), FUN_140A57450/57650, FUN_140A178D0 (observador de bote), FUN_1404A67B0, FUN_141AEA910 (inicio), FUN_141B98150, FUN_141B0D5F0, vtables de BallControl/BallInfoUpdate*/RigidBody/Island*. Scripts de trabajo: `C:\Temp\claude\D--Frank-Games--Conmegol-Patch\4623fc40-d358-4228-accc-6b7aa03f8252\scratchpad\frenteB\`.
