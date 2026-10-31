# Frente E: deformación de la malla de la red (goalnet) del arco

Análisis estático de PES2021.exe (base 0x140000000). Los bytes se verificaron idénticos en `copias\exe_pes2021\PES2021.exe` y en `Conmegol Patch\PES2021.exe`.
Fuentes leídas: `bodies.pkl` (decompilado), `callers2.pkl`, `clases_pes.csv`, bytes crudos del exe (PE API), búsqueda de xrefs rel32 y punteros en .rdata.
Niveles: CONFIRMADO = leído en el decompilado y/o bytes; PROBABLE = deducido con buena base; HIPÓTESIS = sin leer el consumidor.

## 0. Resumen

1. La red NO se anima por evento de gol ni por una animación FoxAnim `goal_net`. Es una **simulación de tela (cloth) en tiempo real** (CONFIRMADO), con clases `cobra::rd::SceneGoalnetNode(Impl)` (vtable 0x142aa2880), `cobra::rd::GoalnetHandle` (0x142a8cae8), `GoalnetClothPoint/PointLink/PointTriList`, `cobra::cfg::GoalnetConfig` (los parámetros de tela que toca `nets.lua`). `match::GoalnetFilm` NO es la animación: es el **volumen de triángulos invisible** con el que se detecta que la pelota está dentro de la red (sección 2).
2. La tela se deforma por **golpes puntuales (esfera: posición + radio)** que el juego le encola cada frame. El disparo es continuo por posición de la pelota, no por evento de gol.
3. La detección "pelota dentro de la red" (`FUN_1404aa790`) usa el volumen construido con las **dimensiones vivas** (bloque `_DAT_1436f3dc8+0x2aa8`), una sola vez por partido en la init de `match::BallControl` (`FUN_1404248f0`). Si en street esas dimensiones ya valen ±15 antes de esa init, el motor lo dispara solo; si no, la sonda devuelve falso y no hay golpe.
4. Las posiciones que usa el golpe son las **posiciones de mundo de la pelota tal cual** (sin transformar) (CONFIRMADO en la cadena de abajo). La malla vive en el espacio de vértices del fmdl (mundo), así que con el modelo en ±15 el golpe tiene que ir con x≈±15..±17. Con posición original (±52,5) no golpea nada.

## 1. Cadena completa del golpe (de la pelota a la malla)

```
match::BallControl vtable 0x1425988b0
  slot2 (0x140424f70) -> FUN_140424d70 -> FUN_140425e20  "chequeo pelota/red"   RVA 0x425e20
       arma 9 floats (pos/prev de la pelota, objeto pelota = *(this+0x20), +0x64/+0x70/+0x7c)
       tipo = 1 o 2 (float 1.4e-45 / 2.8e-45; según pos de la pelota vs bloque de dimensiones pfVar1[2],[3],[0xf],[0x10])
       -> FUN_140456290(netObj=*(this+0x28), pos9, tipo)    "sonda"             RVA 0x456290
              lado = (pos.x >= 0)   [FUN_141eadde0: comiss 0,[rcx]; setbe]
              netObj+0x14..0x1c = pos ; netObj+0x20 = tipo
              FUN_1404aa790(netObj[lado], pos, tipo) -> bool "dentro del volumen de red"
              si true: netObj+0x10 = 1 ; si false: netObj+0x10 = 0
       si true, después: func_0x140872240 (respuesta física de la pelota en red) y func_0x140872210(pelota, estado 6..0xB)
  slot5 (0x1404249d0) FUN_1404249d0  "driver por frame"                          RVA 0x4249d0
       (a) si this+0x18 y this+0x28: para los 22 jugadores (iVar<0x16) y sus 5 puntos corporales
            (+0xc4,+0x94,+0xa0,+0xac,+0xb8) llama FUN_140456230 -> FUN_141eae180 (golpe, radio 0.1087)
       (b) FUN_1404560e0(netObj)  "driver de la red"                              RVA 0x4560e0
            si netObj+0x10 != 0: lado=signo(netObj+0x14.x);
                 FUN_141eae180(lado, tipo(netObj+0x20), &netObj+0x14, radio=_DAT_142598958=0.10868593f [,flag])
                 FUN_141eae330(lado, 1)      ; estado 1 = red activa
            si no: si |x|(netObj+0x14) < 50.0 (_DAT_14259be34) o |z|(netObj+0x1c) > 5.0 (_DAT_14259222c)
                 FUN_141eae330(0,2); FUN_141eae330(1,2)   ; estado 2 = ambas redes "dormidas"
```

Envoltorio del juego (lado hilo de juego), array global `0x1437f4940` (2 punteros `draw::GoalNet`, uno por portería) con mutex en `0x1437f4960 + idx*0x30`:

| Función | RVA | Firma | Efecto |
|---|---|---|---|
| `FUN_141eae180` GoalNet_Hit | 0x1EAE180 | `(int lado, int tipo, float* pos3, float radio, char flag)`; tipo 0 = ignorar | bloquea mutex, `FUN_141eae100`, publica comando de render 0x567 (0x28 bytes: +0x10 tipo, +0x14..0x1c xyz, +0x20 radio, +0x24 flag) via `0x1423d44c0` |
| `FUN_141eae330` GoalNet_SetState | 0x1EAE330 | `(int lado, int estado)`; 1 = activar (cmd 0x568), 2 = dormir (cmd 0x569). Solo actúa si cambia el estado (`*(obj+0x18) != estado`) | |
| `FUN_141eae080` GoalNet_Reset | 0x1EAE080 | `(int lado)` | cmd 0x56A, `obj+0x18 = 2` |
| `FUN_141eae2b0` GoalNet_SetPost | 0x1EAE2B0 | `(int lado, int tipo0..2, float* pos)` | cmd 0x56B: matriz de ancla (3 por portería: poste izq., poste der., travesaño) |

Lado de render (hilo de render): `cobra::rd::impl::SceneGoalnetNodeImpl` (vtable 0x142aa2880):
- slot 13 (`0x141A323E0`): tail-call a `FUN_14199db50` si estado==7 y hay handle (`this+0x190`). Es el consumidor del cmd 0x567. CONFIRMADO (bytes: `83 B9 80 00 00 00 07 75 1A 48 8B 89 90 01 00 00 48 85 C9 74 0E 0F B6 44 24 38 88 44 24 38 E9 ...`).
- `FUN_14199db50` Handle::AddHit `(handle, int tipo, float x,y,z, float radio, char flag)`: encola en un anillo (`handle+0x40..0x60`, máx. 0x14 = 20 pendientes, entradas de 0x30 bytes). Con flag!=0 guarda también la posición anterior (barrido). CONFIRMADO.
- slot 14 (`0x141A33970`) = update por frame: `FUN_14199ea20` (paso de tela, dt fijo 0x3c888889 = 1/60, `_DAT_143516c04` iteraciones de restricciones) -> `FUN_14199ce10` saca los golpes de la cola y llama `FUN_1419e01d0` (colisión esfera/malla contra los puntos de tela). Luego `FUN_14199ec40` calcula normales y `FUN_1423c8ae0` sube los vértices al buffer de render (triple buffer). CONFIRMADO.
- slot 15 (`0x141A32410`) = `FUN_14199dc90` reset de tela (vacía la cola, reposo). slot 16 (`0x141A32430`) = recibe matriz de ancla y la pasa a `func_0x14199e890` (`FUN_14199e900` aplica la matriz 3x4 a los puntos fijados al poste).

Estado de la máquina del nodo (`FUN_141a32a20`, offset +0x80): 1 carga datos Fox de la red, 2 espera, 3 resuelve entidades (3 por portería, hashes 0x2daa3de18900 / 0x544bd7e97372 / 0x75b19cfb486f), 4 crea tela (`FUN_14199d380`) con los vértices del fmdl, 5-6 crea el modelo/buffer, **7 = operativo**. Hasta estar en 7 los golpes se descartan (slot 13 hace `cmp [rcx+0x80],7`).

Cobertura de replay: `match::ReplayBall` slot 3 (`FUN_14217c9b0`) y `match::ReplayHuman` slot 3 (`FUN_14217f2f0` -> `FUN_1421807c0`, 8 puntos corporales) llaman a `FUN_141eae180` con las posiciones grabadas. Por eso la red también se mueve en repeticiones. CONFIRMADO.

## 2. Qué posiciones usa (pregunta 2)

- **Golpe (hit)**: coordenadas de mundo de la pelota, tal cual (CONFIRMADO: ni `FUN_141eae180`, ni el cmd 0x567, ni el thunk del nodo, ni `FUN_14199db50` aplican transformación). La tela está en el espacio de vértices del fmdl; como el fmdl tiene vértices en coordenadas de mundo absolutas (por eso editaste los float32 de ±44..60), un golpe en x=±15..17 deforma la red movida a ±15 (PROBABLE, coherente con `FUN_14199d380` que arma los puntos desde los vértices; no leí `FUN_1419e01d0` entera).
- **Detección de "dentro de la red"**: `FUN_1404aa790`/`FUN_1404beda0` intersecan el movimiento de la pelota con triángulos de `match::GoalnetFilmSheaf(In|Out)` (vtables 0x1425ab110/0x1425ab0f8; tablas estáticas `0x1434465f0` y `0x1434472d0` en .bss, contadas por refcount `0x1436f1b68/69`). Esas tablas se construyen en `FUN_1404be8c0` a partir de **dimensiones vivas**: `FUN_1404aa610(lado, p=_DAT_1436f3dc8+0x2aa8)` pasa `p[9]`, `p[2]+0.046875`, `p[0]-0.1`, `p[0xd]`, `p[0xe]` a `FUN_1404be0a0/1a0`. CONFIRMADO que se leen del bloque vivo y que se reconstruyen en cada init de `BallControl` (`FUN_1404248f0`/`FUN_140424cb0` destruyen `this+0x28` con `FUN_140456090` y llaman a `FUN_1404561e0`). HIPÓTESIS: que el refcount global de las tablas estáticas impida reconstruir si una instancia previa sigue viva (antes de entrar al entrenamiento, p. ej. escena de menú), dejando el volumen con ±52,5.
- **Anclas de la tela (postes/travesaño)**: `match::MatchControl` slot 5 (`FUN_1404219d0`, vtable 0x1425984c0) llama a `FUN_141eae2b0` 4 veces con posiciones leídas de `*(_DAT_1436f3dc8+0x158)` (registro `GoalPostInfo`, `func_0x140a5bc60(obj,i,out)`). Si ese registro conserva las posiciones ORIGINALES (±52,5) podrían estar desplazando los puntos fijos de la tela (el nodo resta el vector de reposo `(0, 2.874, 0, 1)` en `0x142aa2910`, así que lo que importa es la traslación respecto a ese reposo). HIPÓTESIS sin verificar: depende de si el registro se actualiza con las dimensiones street. Si la red se ve rara (puntos fijos desplazados, malla tensa o colgando), esta es la sospechosa número 1.

## 3. Por qué no se deforma hoy en street (diagnóstico)

Orden de causas probables:
1. `FUN_1404aa790` devuelve falso: el volumen/film se construyó con dimensiones originales (o la pelota se evalúa fuera del volumen) -> `netObj+0x10 = 0` -> nunca se llama al golpe. PROBABLE.
2. Aun con golpe, `FUN_1404560e0` apaga la red (estado 2 en ambas) cada frame mientras `|x| < 50`: con la cancha ±15 SIEMPRE se cumple `|x| < 50` cuando el flag es 0, así que la red queda dormida en cuanto un frame no tiene contacto. PROBABLE (leído en el decompilado; no leí qué hace el estado 2 en el consumidor, HIPÓTESIS que "dormir" detiene la simulación y el repliegue).
3. Anclas con posición original (sección 2). HIPÓTESIS.
4. La parte (a) de `FUN_1404249d0` envía golpes de jugadores, no de la pelota; no sustituye a la pelota.

## 4. Propuestas de hook (en orden de preferencia)

Todas las firmas verificadas únicas en `.text` con `fox.unique_sig` (comodines `??` en disp32 RIP-relativo y rel32).

### Opción A (la más limpia): llamar tú mismo a la API de golpe desde tu hook de la pelota (FUN_140876360)

Cuando la pelota esté dentro de tu volumen de red street (|x| entre 15 y ~15+profundidad, |z| <= 3,66+margen, y <= 2,44+margen):

```
lado = (p.x >= 0)                                  // 1 = portería +x
GoalNet_SetState(lado, 1)                          // RVA 0x1EAE330   (void)(int,int)
GoalNet_Hit(lado, tipo, &p, 0.1086859f, flag)      // RVA 0x1EAE180   (void)(int,int,float*,float,char)
```
- `tipo`: 1 (golpe normal) o 2 (golpe fuerte). 0 se ignora. Elegir 2 si |v| alto (PROBABLE, así lo calcula `FUN_140425e20`).
- `pos3`: puntero a 3 floats **alineado a 16** (el cmd 0x567 hace `movaps xmm1,[r8]`; la estructura vec es de 16 bytes con w=0). Pasar un `float[4]` alineado.
- `radio`: 0.10868593 (`_DAT_142598958`) o la mitad del radio real de la pelota (~0.11).
- `flag`: 1 para golpes consecutivos (barrido con posición anterior, evita túneles a velocidad alta), 0 para el primero (PROBABLE; en el decompilado de `FUN_1404560e0` el 5º argumento no se ve, en `FUN_140456230` es 0 explícito).
- Hay un tope de 20 golpes pendientes por frame (`Handle_AddHit`), mandar uno por frame de la pelota.
- Problema a resolver: `FUN_1404560e0` (cada frame) llama `GoalNet_SetState(0,2)/(1,2)` cuando `netObj+0x10 == 0` y `|x| < 50`. Eso apaga la red justo después de que la activas (toggle por frame: cmd 0x568 y 0x569 en el mismo frame; el último gana). Hay que neutralizarlo (opción B o C).

### Opción B: dejar que el motor lo dispare solo: hacer verdadera la sonda `FUN_140456290`

Hook en `FUN_140456290` (RVA 0x456290; VA 0x140456290):
- Firma AOB (22 bytes, sin reubicaciones en los primeros 27): `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B D9 41 8B F0 48`
- Bytes crudos: `48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 20 48 8b d9 41 8b f0 48 8b ca 48 8b fa e8 ...` (primera reubicación en el byte 28, `e8`). Límite de instrucción en 15 bytes -> se puede robar 15 bytes (salto absoluto de 14 bytes: `FF 25 00000000 + addr8`) sin partir instrucciones.
- Firma: `char __fastcall Probe(void* netObj /*rcx*/, float* pos9 /*rdx*/, int tipo /*r8d*/)`.
- Lógica del hook (solo en street): si `pos9[0..2]` está dentro del volumen street de la red: escribir `netObj[+0x14..0x1c] = pos`, `netObj[+0x20] = tipo`, `*(char*)(netObj+0x10) = 1` y devolver 1 SIN llamar al original. Si no, llamar al original.
- RIESGO: al devolver verdadero, `FUN_140425e20` ejecuta la respuesta física de la pelota en red (`func_0x140872240`) y fija estados de la pelota 6..0xB (`func_0x140872210`) usando dimensiones del bloque `pfVar1`. Con tu propio frenado analítico podrías tener doble frenado o estados inesperados (PROBABLE). Si te molesta, usa la opción C.

### Opción C: reemplazar/neutralizar el driver `FUN_1404560e0` (la más controlada, recomendada junto a A)

- RVA 0x4560E0, VA 0x1404560E0.
- Firma AOB (22 bytes, con comodines en la cookie RIP): `40 56 48 83 EC 50 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 40 80`
- ATENCIÓN: los bytes 6..12 incluyen `mov rax,[rip+disp32]` (cookie de pila), así que el trampolín debe reubicar ese disp32 o el hook debe reemplazar la función completa sin llamar al original. Firma: `void __fastcall NetDriver(void* netObj /*rcx*/)`.
- Reemplazo completo (más simple): en street no hacer nada (no apagar la red), y tu hook A es el único que activa/desactiva. En no-street llamar al original (con trampolín reubicado) o dejarlo intacto con un salto condicional.
- Alternativa sin tocar la función: poner `netObj+0x14.x` fuera de `<50` no funciona (sale por la rama de apagado). Se puede parchear el umbral `_DAT_14259be34` (float 50.0 en .rdata `0x14259BE34`) a ~10.0 y `_DAT_14259222c` (5.0) si no se comparte con otros usos (no verifiqué sus otros usos; HIPÓTESIS, mejor usar C).

### Opción D (parche de datos, sin hook): reconstruir el volumen
Verificar que el bloque de dimensiones `_DAT_1436f3dc8+0x2aa8` valga la cancha street ANTES de `FUN_1404248f0` (init de `BallControl`, RVA 0x4248F0, y `FUN_140424cb0`). Si lo vale, la opción B no hace falta (el motor ya detecta y manda `GoalNet_Hit` con la posición real). Primera prueba barata: leer en vivo `*(char*)( *(this_BallControl+0x28) + 0x10 )` con la pelota dentro de la red; si es 1 el motor ya golpea; si es 0, la sonda falla (causa 1).

## 5. Direcciones clave (RVA / VA)

| Qué | RVA | VA |
|---|---|---|
| GoalNet_Hit (envoltorio de juego) | 0x1EAE180 | 0x141EAE180 |
| GoalNet_SetState | 0x1EAE330 | 0x141EAE330 |
| GoalNet_Reset | 0x1EAE080 | 0x141EAE080 |
| GoalNet_SetPost | 0x1EAE2B0 | 0x141EAE2B0 |
| Lado = (x>=0) | 0x1EADDE0 | 0x141EADDE0 (`33 C0 0F 57 C0 0F 2F 01 0F 96 C0 C3`) |
| Array draw::GoalNet*[2] / mutex | 0x37F4940 / 0x37F4960 (+0x30*i) | 0x1437F4940 / 0x1437F4960 |
| Cmd render golpe (0x567) | 0x23D44C0 | 0x1423D44C0 |
| Nodo: thunk golpe (slot13) | 0x1A323E0 | 0x141A323E0 |
| Nodo: update/sim (slot14) | 0x1A33970 | 0x141A33970 |
| Nodo: máquina de estados | 0x1A32A20 | 0x141A32A20 |
| Handle::AddHit | 0x199DB50 | 0x14199DB50 |
| Paso de tela | 0x199EA20 | 0x14199EA20 |
| Consumo de golpes | 0x199CE10 | 0x14199CE10 |
| Colisión esfera-tela | 0x19E01D0 | 0x1419E01D0 |
| Sonda pelota/red | 0x456290 | 0x140456290 |
| Driver de red por frame | 0x4560E0 | 0x1404560E0 |
| BallControl slot5 | 0x4249D0 | 0x1404249D0 |
| Chequeo pelota/red | 0x425E20 | 0x140425E20 |
| Volumen de red (test) | 0x4AA790 | 0x1404AA790 |
| Construcción de volúmenes | 0x4AA610 | 0x1404AA610 |
| Init de BallControl | 0x4248F0 | 0x1404248F0 |
| MatchControl slot5 (anclas) | 0x4219D0 | 0x1404219D0 |
| Replay pelota / humano | 0x217C9B0 / 0x21807C0 | 0x14217C9B0 / 0x1421807C0 |

Constantes (.rdata): radio de golpe `0x142598958` = 0.10868593f; umbral `|x|` `0x14259BE34` = 50.0f; umbral `|z|` `0x14259222C` = 5.0f; factor espejo `0x142588218` = -1.0f; reposo de ancla `0x142AA2910` = (0, 2.874, 0, 1).

## 6. Lo que NO se pudo determinar

- Consumidor del cmd 0x568/0x569 (qué hace exactamente el estado "dormido"): el despachador de comandos de render se hace por tabla y no lo localicé por texto; HIPÓTESIS que 2 = sin simulación.
- Si `*(_DAT_1436f3dc8+0x158)` (posiciones de postes pasadas a `GoalNet_SetPost`) toma las posiciones originales o las vivas en street (sin leer `func_0x140a5bc60` ni el registro GoalPostInfo).
- Cuál es el 5º argumento (`flag`) real que pasa `FUN_1404560e0` a `GoalNet_Hit` (no visible en el decompilado).
- Semántica exacta del `tipo` 1/2 en la tela (probablemente solo cambia fuerza/amortiguación); `FUN_1419e01d0` (11 KB) no se leyó entera.
- Si la tela usa la matriz del nodo del fox2 (que la traslación del fox2 se ignora, según tus notas); asumo espacio de vértices fmdl.
- Esta vía no necesitó Ghidra ni procesos en ejecución; no se tocó PES.
