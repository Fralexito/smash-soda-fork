# Frente I: despeje automático de la IA en el entrenamiento street (solo lectura sobre PES2021.exe)

Base 0x140000000. Método: bodies.pkl (decompilado), vtables de clases_pes.csv, bytes del exe. Firmas comprobadas únicas en la sección de código. No se tocó ningún proceso.
Leyenda: CONFIRMADO (leído en decompilado/bytes) / PROBABLE / HIPÓTESIS.

## 1. Decisión de IA que elige el despeje

| VA | RVA | qué es | conf. |
|---|---|---|---|
| 0x142beef70 | 0x2beef70 | vftable match::ai::bp::ThinkUnitClear (3 ranuras: dtor 0x14210dd30, 0x141794410, **exec 0x142109eb0**) | CONFIRMADO |
| 0x142109eb0 | 0x2109eb0 | ThinkUnitClear::exec (59 B). Si `*(char*)(ctx[5]+0x31C1)!=0` escribe `out[0]=5` (intención Clear) y `out[2]=FUN_142108630(...)` (destino); si no, no escribe nada | CONFIRMADO |
| 0x142bef030 | 0x2bef030 | vftable ThinkUnitGKClear (exec 0x142132b40, 722 B): despeje del portero con balón (escribe tipo 5 o 6) | CONFIRMADO |
| 0x14210d320 | 0x210d320 | ctor de match::ai::bp::BallPlayer: embebe las ThinkUnit (Clear en +0x36C0, GKClear en +0x3D00, Shoot +0x3678, Dribble +0x33A0) | CONFIRMADO |
| 0x142110d30 | 0x2110d30 | BallPlayer::Update: construye la lista de unidades (FUN_142110370), las evalúa con FUN_14211eec0 (llama vtable+0x10 con out=unidad+0xC), filtra con FUN_142110ac0 y elige con FUN_14210f110 | CONFIRMADO |
| 0x142119390 | 0x2119390 | calcula el flag «hay que despejar» `ctx+0x31C1` (stores en 0x142119714 `88 86 C1 31 00 00` y 0x142119770 `C6 86 C1 31 00 00 01`). Viene de un «juez de presión» (FUN_14069A500 / FUN_140699960, en ctx+0x2124) y de la dimensión de cancha `[raíz+0x2AA8]` (largo): el balón del portador cerca de su propia portería con rival presionando | CONFIRMADO (lectura) / PROBABLE (semántica fina de las constantes) |

Lo que NO existe como tal: no hay clase «Clear» en match::anime::action::* (el despeje es un Kick con orden Clear, ordA=4; 0x58 es el id de acción de pulsación de Clear, categoría 3). Las clases player::ActionClear (0x1409798f0), ActionSliding, ActionTackle, ActionKeeperBlock/Deflect/Punching/Makeshift se instancian dentro del contenedor de IA por jugador (ctor FUN_14091edb0), pero el único productor de acción «despeje» a nivel de decisión del portador es ThinkUnitClear. En la IA no encontré una ruta «pelota a punto de entrar → defensor de campo despeja» separada de BallPlayer (el defensor que va a jugar la bola se convierte en BallPlayer y su unidad Clear dispara por el flag 0x31C1). Tijera/chilena/«sobre la línea»: no encontré clase ni rutina propia (las cadenas _legscissors_* y *_clear_* del exe son nombres de animaciones, no decisiones). HIPÓTESIS: el resto de despejes de campo son animaciones elegidas por el Kick con orden Clear.

Pasa por aquí (CONFIRMADO): FUN_14092c9f0 (decisión por jugador, 2958 B) → rama `param_5==NULL` (jugador sin acción de mando) → FUN_1421019d0 → BallPlayer::Update. La rama con `param_5!=NULL` (acción del mando) NO pasa por BallPlayer. Cadena superior: 14092a340 ← 140929a00 ← 1409257b0 ← 140928fc0 (productor de registros B). param_5 = `*(record+0x28)`.

## 2. Anulación solo en el entrenamiento

Parche de 1 byte (preferido): hacer que ThinkUnitClear::exec salte siempre al final sin escribir (= «no aplicable», la misma convención que usan las demás ThinkUnit cuando su condición falla: salida por defecto tipo 0 / unidad+0x28=-1, reseteada por FUN_14211F010).

```
VA 0x142109EC7   RVA 0x2109EC7   74 -> EB    (je +1C  ->  jmp +1C; destino = add rsp,20 / pop rbx / ret)
Restaurar: EB -> 74
AOB (25 B, ÚNICO, sin RIP-relativos; parchear el byte +23):
40 53 48 83 EC 20 49 8B 40 28 4D 8B C8 48 8B DA 80 B8 C1 31 00 00 00 74 1C
(inicio del patrón = 0x142109EB0 = inicio de la función; byte a cambiar en inicio+0x17)
```
- On/off: igual que los dos parches del usuario: el Core lo escribe al entrar a FREE_TRAINING zona 52 con cancha 30x20 y lo restaura al salir (CONFIRMADO que es 1 byte en .text, ejecutado en cada evaluación).
- Hilo/contexto: hilo de simulación del partido (cadena 140928fc0 → BallPlayer::Update, una vez por jugador y fotograma). Es la misma cadena de «Productor de registros»; HIPÓTESIS que es el hilo principal de partido (no verificado en runtime).
- Alternativa más agresiva (no recomendada): anular los dos stores del flag 0x31C1 (6 y 7 bytes, no es parche de 1 byte). Dejaría también a las unidades Pass* con el umbral «sin presión» (PassLong 0x14212AD40, PassSafety 0x14212B520, PassForward 0x142128A20 leen el flag). Con el parche de 1 byte el flag sigue calculándose, así que esos pases siguen modulados por presión como ahora.
- Portero: NO se toca ThinkUnitGKClear (0x142132B40). Las atajadas son player::ActionKeeper* / anime::action::goal_keeper::* (Catch, Block, Deflect, Punch, ...), rutas distintas; el parche no las alcanza (CONFIRMADO por separación de clases, PROBABLE por no haber probado en vivo). Si se quisiera quitar también el despeje del portero con balón en pies: GKClear tiene muchas ramas y escribe tipos 5 y 6; no hay parche de 1 byte obvio (no determinado).

Distinguir portero de jugador de campo: son unidades distintas (ThinkUnitClear vs ThinkUnitGKClear), así que el parche ya solo afecta al de campo. Por registros: portero = slot 0 de cada equipo (idx 0 y 11).

## 3. Riesgos

- Que la IA se quede sin opciones: baja. Una ThinkUnit que no escribe es un caso normal (todas tienen salidas tempranas). Seguirán Dribble, Pass*, Shoot, Feint. Riesgo residual: portador acorralado dribla/pierde la bola (es lo que se quiere). HIPÓTESIS que el chooser FUN_14210F110 nunca exige que exista una intención tipo 5 (leí ~40% de la función; los tipos que trata: 1, 2, 3 (Shoot), 5 (Clear), 7).
- Congelamiento: no observado en el código; probar en vivo con la IA sola presionada junto a su portería.
- Tipos de intención usados: 3 = Shoot, 5 = Clear (CONFIRMADO en 0x142121920/0x142109eb0 y en FUN_14210F110).
- Un jugador del usuario sin botón pulsado (param_5==NULL) también pasa por BallPlayer (HIPÓTESIS): el parche le afectaría igual (benigno).

## 4. Dato nuevo del coordinador: el rival (IA) despeja en vez de tirar en su mitad

Pregunta: ¿la IA usa las mismas unidades/despachador que el mando (match::pad::ThinkUnit*, FUN_1408AC500) y los parches del usuario (FUN_1408B9CE8, FUN_1408BB150) la cubren?

Respuesta: **NO la cubren; la IA tiene ruta propia** (CONFIRMADO).
1. Las pad::ThinkUnit* se despachan solo desde el pipeline del mando: FUN_1408B4A20 (ranura de pad, tabla de 0x74 B por mando en `[raíz+0x180]`, exige jugador asignado `+0x20/+0x24<0x16`) → 1408B4170 → 1408B1CF0 (que llama a 1408AF820 = lista de ids y 1408B3AC0). FUN_1408AC500 solo la llaman funciones de unidad del pad (1408B5DE0, 1408B5F10, 1408B8E50...). FUN_1408B9C80 (+0x88 de Shoot) y FUN_1408BB0D0 (+0x88 de Clear) no tienen llamadores directos: son ranuras de vftable de las unidades pad.
2. La IA decide en FUN_14092C9F0 con `param_5==NULL` → match::ai::bp::BallPlayer → ThinkUnit* de `match::ai::bp` (vtable 0x2beed70+). Esas clases son otras (Clear 0x2beef70, Shoot 0x2beed90) y no leen las ranuras +0x88 ni el chequeo «dir*pos.x<0».
3. Por qué la IA despeja en su mitad: en la IA no hay «habilitación por mitad». Gana Clear porque el flag `ctx+0x31C1` (presión + cercanía a la propia portería, FUN_142119390) hace que ThinkUnitClear escriba tipo 5, y el chooser FUN_14210F110 lo prefiere; ThinkUnitShoot (0x142121920, 15 KB) no tiene chequeo de mitad pero sí varios por distancia/ángulo a portería (retorna sin salida si el rival está lejos del arco, p. ej. `dist > _DAT_14253EE30`, y por línea de tiro/ocupación). Leí ~170 de ~440 líneas de Shoot; no vi un chequeo de signo/mitad (`0x116C` no aparece en Shoot).
4. Parche propuesto: el mismo de la sección 2 (0x142109EC7, 74→EB). Con eso la IA no puede producir la intención Clear y, si Shoot cumple sus propios umbrales de distancia, dispara; si no, regatea/pasa. NO hay forma de forzar que Shoot sea aplicable lejos de la portería rival con 1 byte sin leer el resto de Shoot: lo que decide es la distancia (constantes `_DAT_14253EE30`, `_DAT_14259BE70`, `_DAT_14259BE4C`); no determinado si en 30x20 m se cumplen (las constantes pueden estar en metros y la cancha lógica es pequeña, PROBABLE que sí).
5. Lo que quede: si tras anular Clear la IA sigue sin tirar en su mitad, el siguiente paso es leer el resto de 0x142121920 (líneas 260-440) y de FUN_14210F110 (resto) para localizar el umbral de distancia que la bloquea.

## 5. No determinado
- Hilo exacto y prueba en vivo del parche (el estudio es estático).
- Cualquier despeje de campo que no pase por BallPlayer (p. ej. defensor sin balón con acción Sliding/Tackle/GoalCover): player::ActionGoalCover (0x2639AE8), ActionStopGoal (0x2630EA0), ActionSliding no se leyeron; HIPÓTESIS que no son «despeje».
- Semántica de las constantes del juez de presión (FUN_140699960/FUN_14069A500) y del archivo common/match/constant/player/ballplayerClear.json (clase constant::BallplayerClear 0x2b4dd18), que podría tener umbrales editables en lugar de parchear código.
