# Frente W: el portero (IA) despeja un balón que debería controlar (street 30x20)

Estático, solo lectura (bodies.pkl, clases_pes, frentes H/I/M, phoenix_core.cpp). No se tocó el juego ni Conmegol Patch. No se leyó el .exe: los valores de las constantes de .rdata no están medidos.
Niveles: CONFIRMADO (leído en el decompilado) / PROBABLE / HIPÓTESIS.

## 1. Resumen en 5 líneas
1. La decisión «despejar» del portero IA con balón vive en **FUN_142132B40 = ThinkUnitGKClear** (RVA 0x2132B40, unidad id 18 de `BallPlayer`). CONFIRMADO.
2. Su regla es casi la contraria a la tuya: escribe «despejar» (tipo 5) cuando el **rival más cercano al balón está a menos de ~12-15 m** (`_DAT_14259BE04` cerca del arco / `_DAT_14259BE0C` lejos). En 30x20 (diagonal 36 m) eso es SIEMPRE verdad. CONFIRMADO el mecanismo; los metros exactos son del frente H (sin medir de nuevo).
3. Ese es el **único punto** donde forzar «controlar»: si la unidad no escribe nada, el portero cae en PassShort (pase corto) o GKDribble (conducir/controlar). No toca posición, cursor ni salida. CONFIRMADO.
4. Hoy el Core ya lo anula con `arquero_saque_corto` (primer byte `48`->`C3`): eso quita el despeje SIEMPRE, no condicionado. Tu regla (despejar solo si balón < 4.5 m del arco Y rival < 2.5 m) se logra **cambiando ese `C3` por un gancho** con la condición.
5. Si el síntoma persiste con ese `C3` activo (el log del 28-oct lo mostraba activo), la causa NO es GKClear; ver §5 (HIPÓTESIS, hay que medir en vivo).

## 2. Dónde decide la IA (cadena) — CONFIRMADO
- IA del portero: `PlayerGKAuto` (vtable 0x14262FD78, ctor FUN_14091EDB0, contiene todas las `ActionKeeper*`) -> FUN_14092C9F0 (param_5==NULL, la IA decide) -> `BallPlayer::Update` FUN_142110D30 -> FUN_142110370 (evalúa unidades por prioridad, tabla `BallPlayer+0x3E20`, 25 ids) -> chooser FUN_14210F110 (elige la primera decisión válida).
- Ids de portero: 18 GKClear, 19 GKDribble, 20 GKPassShort, 21 GKPassLong. Sin decisión y portero con balón: FUN_142110D30 evalúa por defecto el id 0x13 (GKDribble) o el 0x10.
- El registro de decisión de cada unidad: `[0]=tipo` (2 = pase a jugador, 3 = conducir, 5 = despejar, 6 = otro), `[2]=receptor/objetivo`. «No escribir» = «no aplicable».
- El portero de usuario (mando) no pasa por esto: ya cumple «si Frank controla, decide él».
- Fuera de BallPlayer, las acciones `ActionKeeper*` (Catching, PickupBall, Punching, Deflect, Block, PuntKick, Throw...) ejecutan lo decidido; no eligen despejar vs controlar (frente M, H).

## 3. Qué condición exacta elige despejar — FUN_142132B40 (722 B)
Leído del decompilado:
- `rival = FUN_140A4EE40(obj, ladoRival, &posBalón, 0)` = jugador rival más cercano al balón; `posBalón = ctx[4]+0x40 (x), +0x48 (z)`.
- `d2 = (distancia rival-balón)²`. `distArco = distancia(punto de gol propio FUN_140A62780, balón)`. Umbral `R = (distArco < _DAT_1425A2670) ? _DAT_14259BE04 : _DAT_14259BE0C`.
- **Si `R² > d2` -> `out[0]=5` (despejar)** con objetivo FUN_142108630. Si no, solo una rama menor (`out[0]=6`) bajo condiciones de intercepción.
- Rama previa (despeje incondicional por rival): si el objeto de modo (`world+0x400`) lo pide, `ctx[4]+0x6C != 0` y pasó cierto tiempo con balón (comparación `FUN_140A3E840` contra `FUN_141E532E0`, parecido al «timeout» de 3-6 s): `out[0]=5`.
- Conclusión: NO mira distancia absoluta al arco como condición; solo cambia el radio de presión. Con `R` = 12-15 m en una cancha de 15x10 de semi-medidas, despeja siempre que haya un rival, aunque esté en el otro córner. Esto explica «despeja de la nada».

Otros dos generadores de «despeje» (no son del portero pero la IA los puede evaluar también):
- ThinkUnitClear de campo (FUN_142109EB0): ya neutralizado por `sin_despeje_emergencia` (0x142109EC7). CONFIRMADO. Su flag (`ctx[5]+0x31C1`, FUN_142119390) mezcla presión y cercanía al arco.
- Filtro del chooser FUN_14210F110: tiene lógica propia para decisiones tipo 5 (rechaza o acepta según rivales por zona `BP+0xC58`, umbrales `_DAT_14259BE5C/BE60`); es el segundo punto posible (§6, plan B).

## 4. Parche mínimo propuesto (plan A) — gancho en GKClear con tu regla
Nombre sugerido: `arquero_despeje_cond` (JSON: `gk_despeje_dist_m` = 4.5, `gk_despeje_presion_m` = 2.5). Reemplaza la mitad GKClear de `arquero_saque_corto`. NO toca posición, cursor, salida corta, ganchoReceptor ni sin_despeje_emergencia.

- Función: `void FUN_142132B40(rcx=?, rdx=uint32* out, r8=void** ctx)`. VA 0x142132B40, RVA 0x2132B40.
- AOB única (ya validada en el Core, 22 B, sin RIP): `48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60 49 8B`. 14 B de prólogo limpios (frontera tras `41 56`): usar `crearDesvio(f, m, 14, gancho, &orig)`, igual que `ganchoReceptor`.
- Lógica del gancho (solo con `g_tTrainGlob` reciente y portero IA, como los demás):
  1. Leer `g = **(int**)ctx[0]`; si no es 0 ni 11 -> `orig(...)` y salir (no debería pasar).
  2. Leer balón: `bx = *(float*)(ctx[4]+0x40)`, `bz = *(float*)(ctx[4]+0x48)`.
  3. Arco propio: `sg = signo(x del portero)` (usar `posJugPC(raiz, g, gx, gz)` que ya existe), centro del arco = `(sg*g_canchaLargo, 0)`. `dArco = hypot(bx - sg*L, bz)`.
  4. Rival más cercano al balón: recorrer los 11 del otro bando con `posJugPC(raiz, i, x, z)` (i = (g<11 ? 11 : 0) + 0..10) y tomar `dRival = min hypot(x-bx, z-bz)`.
  5. Si `dArco < gk_despeje_dist_m` **Y** `dRival < gk_despeje_presion_m` -> `orig(rcx, out, ctx)` (deja despejar). Si no -> `return` sin escribir (controlar).
- Cuando el gancho está activo hay que **no aplicar el `C3` de GKClear** (`g_pdGK2.dir = nullptr` en `gkCortoInstalar`), porque jmp y `C3` chocan en el mismo byte. GKPassLong (FUN_142133120) se queda con `C3`.
- Salida «con pase corto»: sin decisión de GKClear quedan GKPassShort (FUN_1421333F0 + `ganchoReceptor` ya instalado, receptor más cercano 3-10 m) y GKDribble. Con presión (>1 rival) y sin `state+0x6C`, PassShort espera 1 s y mientras tanto entra GKDribble (control). Aceptable.
- Esfuerzo: ~0,5 día (~40 líneas + log). Riesgo: bajo (función de una sola entrada, solo escribe en `out`). Nivel: PROBABLE de que resuelva el síntoma, CONFIRMADO de que es el único sitio de la regla.
- Log recomendado en el gancho (primeras 40 veces): `g, dArco, dRival, original escribió tipo X, decisión final`. Así se ve si el despeje que Frank observa pasa por aquí.

Lecturas necesarias (todas ya usadas por el Core, sin offsets nuevos salvo el balón):
- Posición del portero y de rivales: `raiz = *g_globalPartido`, `rec = *(raiz+(i+0xBF)*0x28)`, pos en `rec+0x55C/+0x564` (`posJugPC`). CONFIRMADO (Core).
- Balón en el gancho: `ctx[4]+0x40 / +0x48`. CONFIRMADO (lo lee el propio GKClear: `param_3[4]+0x40/+0x48`).
- Largo de cancha: `g_canchaLargo` (ya existe).

## 5. Si el despeje sigue con GKClear anulado — HIPÓTESIS (a medir)
El frente M ya notó que el síntoma aparecía con GKClear y GKPassLong anulados. Candidatos, de más a menos probable:
1. **El «balón que llega» todavía no es del portero**: `BallPlayer` pre-evalúa unidades (fases `+0x14/+0x18`, máscaras 0x1F/0x01/0x06/0x18) antes del control; el despeje saldría de otra unidad evaluada para él (ThinkUnitClear de campo ya anulado; PassLong 0x14212AD40 / PassSafety 0x14212B520 de campo no consta filtro por rol). PROBABLE-bajo.
2. **Acciones de parada**: `ActionKeeperDeflect / Punching / Block / SnapUnder / ScoopOut` (cuando el balón viene con velocidad hacia el arco la IA lo trata como disparo y «despeja» con manos o puño, no «controla»). No se localizó la función que elige entre Catching y Deflect/Punching (están en el selector de PlayerGKAuto, FUN_14092A340 15 KB / FUN_14092C9F0 15 KB, sin leer completa). HIPÓTESIS.
3. Que `arquero_saque_corto` no estuviera activo en la prueba donde se vio el despeje (el gancho nuevo lo cubriría igual). PROBABLE.
Prueba que discrimina (una sola corrida): con el gancho de §4 en modo solo-log (siempre llama a `orig`), anotar tipo escrito y en paralelo loguear cuando el portero ejecuta Kick de orden Clear. Si el despeje ocurre sin que GKClear escriba 5, es el caso 1 o 2.

## 6. Plan B (más general, más riesgo) — HIPÓTESIS
Gancho de salida del chooser FUN_14210F110 (devuelve id de unidad, 0x19 = ninguna): si el id elegido tiene tipo 5, el jugador es 0 u 11 y no se cumple tu regla, devolver 0x19. Cubre CUALQUIER fuente de «despeje» del portero (GKClear, ThinkUnitClear, PassSafety...). Pero: la AOB de FUN_14210F110 no está en los informes (hay que sacarla del .exe), el stride del registro de decisión no está medido, y es función caliente de 4,5 KB. Esfuerzo ~1 día. Solo si el plan A no basta.

## 7. Descartado / no hacer
- Cambiar constantes `_DAT_14259BE04/BE0C/A2670`: son floats compartidos en .rdata (no se puede sin afectar decenas de funciones).
- Escribir un flag en el objeto: GKClear no lee ningún flag de «permitir despeje»; el flag `+0x31C1` solo gobierna ThinkUnitClear de campo.
- Tocar FUN_1409DC040 / team+0x255 / FUN_140A571B0 / arquero_salida_corta: fuera de alcance, no hace falta.

## 8. No determinado
- Valores reales de `_DAT_14259BE04`, `BE0C`, `1425A2670` (se leen en vivo desde el gancho: `exe+0x259BE04` etc., o del .exe).
- Qué hace el tipo 6 de GKClear.
- Quién arma la tabla de prioridad `BallPlayer+0x3E20` y si el portero evalúa unidades de campo.
- Si el balón en vuelo hacia el portero activa `BallPlayer` antes de que lo posea (lo sugiere la máscara de fases; sin confirmar).

## 8. VERIFICADO con GhidraMCP sobre el exe real (29-oct-2026, solo lectura)
- `FUN_142132b40` (722 B, 0x142132B40..0x142132E11): el decompilado de PES_MCP_RECORTADO (Ghidra 12.0.4) coincide con §3. **Sin llamadores directos** (callers=[]): se invoca por la vtable de la unidad `ThinkUnitGKClear` (id 18), por eso un gancho en la función es el punto correcto.
- **Constantes medidas en .rdata del exe actual:** `DAT_14259be04 = 12,0` (R con el balón cerca del arco), `DAT_14259be0c = 15,0` (R lejos), `DAT_1425a2670 = 400,0` (umbral de «cerca del arco» sobre `FUN_140A8F890`, que parece devolver distancia al cuadrado: 20 m), `DAT_14256ae1c = 1,0`. Regla: despejar (`out[0]=5`) si `R² > d²(rival más cercano, balón)`, o sea rival a menos de 12 m (cerca) / 15 m (lejos). En 30x20 siempre se cumple. Por eso `arquero_saque_corto` pone `C3` al inicio de GKClear.
- Segunda vía de despeje (rama previa): modo `world+0x400` activo y `ctx[4]+0x6C != 0` y pasó el tiempo de posesión (`FUN_141E532E0 < FUN_140A3E840`) y `FUN_140A3BFC0 < 2` → `out[0]=5` incondicional. Con el `C3` la función ni se ejecuta, así que tampoco entra.
- **Propuesta `arquero_controla` (APAGADA por defecto; NO instalada):** conmutador aparte de la config congelada del portero 0468. Sustituye el `C3` por un gancho (`crearDesvio` de 14 B, AOB del §4) que llama al original SOLO si balón a < 4,5 m del arco propio y rival a < 2,5 m; si no, no escribe nada (el portero cae en GKPassShort/GKDribble). Claves JSON: `arquero_controla` (bool), `gk_despeje_dist_m`=4.5, `gk_despeje_presion_m`=2.5. Hay que medir antes en vivo con `[ARQUERO]` (0.4.93+) qué pasa con el `C3` activo; el síntoma de «despeja de la nada» puede venir también de PassLong/otra unidad (§5).

## 9. La «otra unidad» del portero: FUN_142133950 (GhidraMCP, 29-oct-2026, solo lectura)
- **Tabla de unidades:** `FUN_142132B40` (GKClear) está referenciada UNA vez en `.rdata`, en `0x142BEF040` (descriptores de 0x20 B: `[función Think, puntero nombre, ...]`). Vecinas: `0x142BEF000` → `FUN_1421333F0`; `0x142BEF020` → `FUN_142133120` (= **GKPassLong**, la que el Core ya anula con `C3`); `0x142BEF040` → `FUN_142132B40` (GKClear, `C3`); `0x142BEF060` → **`FUN_142133950`**; `0x142BEF080` → `FUN_142133C80` (envoltorio sin decisiones propias, llama a `FUN_142133ED0`/`FUN_142133CD0`). Después de eso la tabla acaba (datos float).
- **`FUN_142133950` NO está parcheada** y escribe las tres intenciones que preocupan. Actúa según un «modo» `m = *(int*)(this+0x40)` que se SOBRESCRIBE con `*(int*)(equipo+0x6D4)` cuando este es distinto de 0 (la estrategia del equipo para el portero): `m=1..4` → `out[0]=2` (pase a un jugador; `out[3]`=0,1,3 = tipos de pase); **`m=5` → `out[0]=1` (patada a una posición: saque largo/«punt»; destino de `FUN_14211F090`)**; **`m=6` → `out[0]=5` (DESPEJAR; objetivo `FUN_142108630`)**.
- **Condición:** NINGUNA de distancia: solo el modo. Si el valor de estrategia (`equipo+0x6D4`) o el modo por defecto del objeto es 5 o 6, el portero saca largo o despeja siempre, aunque el Core ya haya anulado GKClear y GKPassLong. Es la candidata a explicar el despeje con `C3` activo.
- **Propuesta (dentro de `arquero_controla`, apagada por defecto, sin instalar):** enganchar `FUN_142133950` (prólogo a medir con `pes_memoria_leer`/AOB propia) y, si el modo efectivo es 5 o 6, forzar 2 (pase a jugador) salvo que balón < 4,5 m del arco y rival < 2,5 m (entonces dejar el original); leer antes en vivo `equipo+0x6D4` para confirmar el valor real durante un partido (registro nuevo `[GK_MODO]`).
