# Frente J — Marcador y reloj NATIVOS en el Entrenamiento libre (zona 52)

## PRIMERA VERSIÓN (probar esto primero: solo HUD de marcador con goles)
**Condición que lo oculta (CONFIRMADO):** `FUN_1420C9080` (rellenador del gestor de pantallas 2D) termina con `clase = FUN_14149F4F0(modo)`; `if (clase==0xB || clase==0xC) byte_visible(gestor+0x18B4) = 0`. El modo 52 (FREE_TRAINING) tiene clase 11 (0xB) en la tabla del exe (VA 0x1434FC9C0). La pantalla del marcador (`match2D::Screen::Time`, id 7) solo existe si ese byte es ≠ 0 (`FUN_1420CD220`, caso 7).
**Parche (1 byte):** RVA **0x20C93D7** (VA 0x1420C93D7, archivo 0x20C8FD7): `0B` → `FF`. Firma AOB (`??` = rel32 de call; única en Conmegol y en la copia; el byte a cambiar es el 3.º): `83 F8 0B 74 12 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 83 F8 0C 75 03 C6 07 00` (empieza en RVA 0x20C93D5). Escribirlo al entrar a la zona 52 y restaurar `0B` al salir. json: `marcador_nativo_free`.
**Goles (CONFIRMADO que ya cuentan):** el HUD lee los bytes `gestor+0x18B6/0x18B7` = stat 5 de cada equipo (`[raíz+0x150]+0x750D0+i·0x59650`), alimentado por los observadores de partido, no por `ProcesarGol` (éste solo arma el evento). Con `gol_nativo_free` (k=3 conservado) ya cuenta: `estado.json` real muestra `goles_local:1` en el entrenamiento vía `match.stats()`. No hace falta hook ni escritura para los goles.
**Reloj en la primera versión:** el HUD mostrará el reloj parado (7:35 o 0:00); aceptable. Reloj después: §2.5 (Core escribe el float `S+0x1174`) o parche R1.
**Riesgos/pendientes de la primera versión:** puede haber otras condiciones que mantengan oculto el HUD (`S+0x1168==10`, `S+0x177F≠0`, `W+0x13014==1`, objeto `[raíz+0x388]`); se comprueba leyendo el byte `gestor+0x18B4` tras el parche (sonda §6.1). Si ese byte sigue 0 o la pantalla no carga sus nodos (el AFP de zona 52 podría no traerlos), pasar al plan B (§5). El nativo NO se complica de momento.

Análisis ESTÁTICO (bodies.pkl + bytes del exe + 1 pasada de Ghidra en solo lectura: `frente_J_datos\J1_dis.txt`). No se tocó ningún proceso ni el exe.
RVA = VA − 0x140000000. Offset de archivo en código = RVA − 0xA00. Todas las firmas de abajo salen **1 sola vez** en `Conmegol Patch\PES2021.exe` y en `copias\exe_pes2021\PES2021.exe` (mismos RVA).
Etiquetas: CONFIRMADO = leído en bytes/decompilado; PROBABLE = deducido con coherencia; HIPÓTESIS = sin prueba directa.

## 0. Resumen en 10 líneas
1. El marcador+reloj de partido es la pantalla 2D `match2D::Screen::Time` (id 7 del gestor `match2D::Screen::Manager`). CONFIRMADO.
2. La oculta, en zona 52, una comprobación explícita: `clase(zona) ∈ {0xB, 0xC}` → byte de visibilidad = 0. Zona 52 → clase 11 (0xB). Parche de 1 byte en **RVA 0x20C93D7**. CONFIRMADO (código y tabla).
3. El marcador SÍ se cuenta en el entrenamiento: Phoenix Link/`phoenix_referee.lua` lee `match.stats()` y sale `goles_local:1`, minuto 7:35, periodo 1. CONFIRMADO (estado.json real). Lo que falta es el HUD.
4. El reloj no corre porque la función por fotograma del reloj `FUN_14044CE80` salta TODO si TrainingWork (`[raíz+0x400]` con byte0≠0) está activo (y también si `[raíz+0x3B0]+0x9C≠0`). CONFIRMADO. Parche de 1 byte en **RVA 0x44CEE2** (y otro en 0x44CF02 por el objeto 0x3B0).
5. Al activar el reloj, el fin de tiempo lo decide `FUN_14050B9B0` (Juez, por fotograma desde `FUN_14050C3E0`): sale `true` cuando reloj ≥ 45:00 (162000 unidades) + añadido. Anulable con 1 byte en **RVA 0x50BA3F** (`84`→`32`). PROBABLE (la decisión es CONFIRMADA; que sea el único camino a la demo 0x07 es HIPÓTESIS).
6. Alternativa MÁS SEGURA para el reloj (sin parches de código): que el Core escriba cada fotograma el float `S+0x1174` (`S=[raíz+0x270]`) con `segundos×60`; el HUD y el Juez leen `base + [S+0x1174]`. Con t < 162000 nunca hay fin de tiempo. PROBABLE.
7. Plan B (overlay de Sider) es viable: `overlay_on` puede devolver texto **e imagen con alfa y posición** (ya lo hace `phoenix_mandos_fase1.lua`), pero solo se ve con la ventana del overlay abierta; los módulos CGP/hexx NO sirven para dibujar uno propio (solo recolorean texturas del AFP nativo cuando éste se carga).

---

## 1. Qué es el marcador y qué lo oculta

### 1.1 Elemento del HUD (CONFIRMADO)
- Fábrica de pantallas `FUN_1420CCCD0(id, …)` (switch 0..0x28): **case 7 → construye `FUN_1420EF910` = vtable `match2D::Screen::Time` (0x142BE70B8)**, objeto de 0x168 bytes.
- Esa pantalla pide a la capa AFP/GUI estos nodos (cadenas del exe, tablas en 0x1435419D8…0x143541AD0): `teamName_home/away`, `/score/score_contents/teamName_home|away/numbers_0|1` (goles, 2 dígitos), `…/losstime_progress/losstime_min_02|01|00` + `losstime_sec_01|00` (reloj, 3 dígitos de minuto), `losstime_title` / `/losstime/losstime_contents/losstime_total/numbers` (añadido), `/aggregate/…aggregate_numbers_home|away` (global), `teamColor/firstTeamColor_*`. El nombre "losstime" es solo el prefijo de los nodos del reloj; la pantalla es el marcador completo.
- Actualización por fotograma: `FUN_1420F10D0` → `FUN_1420EFEC0` (lee el estado y rellena una estructura local) → `FUN_1420F12D0` (goles), `FUN_1420F1750` (reloj; es el `pattern_copy_clock` de Sider, RVA 0x20F1932), `FUN_1420F1210` (global).
- Los módulos de Sider `CGP_ScoreboardServer.lua` (bytecode LuaJIT) y `scoreboard-hexx.lua` NO crean nada: sirven por `livecpk` los `common/menu/licence/game2dPes.bin|game2dEngland.bin|…Enter.bin` (AFP) y, en `livecpk_data_ready`, reescriben bytes de textura/color en memoria. Es decir, **si la pantalla nativa se activa en el entrenamiento, hereda gratis los marcadores de Conmegol** (siempre que el AFP elegido para zona 52/torneo 29 contenga esos nodos: HIPÓTESIS, ver §6).

### 1.2 Quién decide que la pantalla exista (CONFIRMADO)
`FUN_1420C2F60` (actualización del gestor, vtable[0]) recorre ids 0..0x28: si `FUN_1420CD220(info, gestor+0x1818, id)` es true y no existe la crea con `FUN_1420CCCD0`; si ya existe y devuelve false la destruye.
`FUN_1420CD220`, para id 7: `return *(char*)(gestor+0x1818+0x9C) != 0` (llamemos **M** al bloque `gestor+0x1818`; M+0x9C = `gestor+0x18B4`).
Filtros previos comunes a todos los ids (en `FUN_1420CD220`, devuelven false):
 a) `S+0x177F ≠ 0` (función `FUN_140A3C020` = `movzx eax,[rcx+0x177F]`) y id≠3.
 b) `*(W+0x3F308)==3` (W=`*(FUN_1414B6A60()+0x50)`) y id ∉ {0xB,0x16}.
 c) `S+0x1168 == 10` y id fuera de la máscara {0xA,0xB,0xD,0x16,0x26} → el marcador (id 7) se oculta en "fase 10". (S+0x1168 = fase del partido: 1/3/5/7 = tiempos en juego, 2/4/6/8 descansos, 9 penales, 10 = terminado; ver `FUN_140A74D00`, `FUN_1420BE780`.) El valor en la zona 52 NO se midió (probablemente 1).

### 1.3 Quién escribe M+0x9C (CONFIRMADO) — aquí está el interruptor del entrenamiento
`FUN_1420C9080(gestor, Screen2dInfo, equipos, p4=bStack_67, p5)` (llamada desde `FUN_1420C5330`, el "rellenador" del gestor):
```
*(M+0x9C) = p4;                                  // p4 = permitido por la lógica de escena
if (W+0x13014 == 1 || S+0x1168 == 9 || FUN_1420CFFD0(M)==0) *(M+0x9C) = 0;   // PK, etc.
...
if (Screen2dInfo+0x24 != 0 && Screen2dInfo+0x28 - 9 < 2) *(M+0x9C) = 0;
clase = FUN_14149F4F0(FUN_14149A470());          // clase del modo de juego
if (clase == 0xB || clase == 0xC) *(M+0x9C) = 0; // <--- AQUÍ
```
`FUN_14149F4F0` = `tabla16[ modo<0x3D ? modo : 0 ]` con tabla en **VA 0x1434FC9C0** (paso 16 bytes, 1.er dword = clase), `modo = [objeto+0xF0]`. Valores leídos del exe: **modo 52 (0x34) → clase 11 (0xB)**; modo 3 → 11; modo 51 → 12 (0xC); modo 2 → 14; modos 54-58 → 28; etc. Es decir el Entrenamiento libre pertenece a la clase "sin marcador" 0xB (junto con el modo 3) y el 0xC (modo 51).
Bytes en el exe (VA 0x1420C93D5): `83 F8 0B 74 12 E8 .. .. .. .. 48 8B C8 E8 .. .. .. .. 83 F8 0C 75 03 C6 07 00` → `cmp eax,0Bh ; je → mov byte [rdi],0` y luego `cmp eax,0Ch ; jne skip ; mov byte [rdi],0`.

También hay un escalón anterior en `FUN_1420C5330` (CONFIRMADO leído, efecto en entrenamiento NO medido): `p4` (bStack_67) se pone a false si `[raíz+0x388]` byte0 ∈ {1..4} (`FUN_140A866E0`: `byte-1 ≤ 3`; es el objeto de escena/demostración), o si los flags del gestor `+0x45B5/+0x10CC` están activos.

### 1.4 Parche del marcador (propuesta; NO probado)
- **P1 (1 byte)**: RVA **0x20C93D7**, VA 0x1420C93D7, archivo 0x20C8FD7... (= RVA−0xA00 = 0x20C8 FD7): `0B` → `FF` (así `cmp eax,0xFF` nunca es verdadero para la clase 11; sigue ocultando la clase 12 = modo 51, que no usamos).
- Firma AOB (RIP-relativos con `??`; única en ambos exe; el byte a cambiar es el 3.º, offset +2): `83 F8 0B 74 12 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 83 F8 0C 75 03 C6 07 00` (match en RVA 0x20C93D5).
- Efecto colateral: es GLOBAL (no mira la zona). Como ya hacen los demás parches del Core (cancha_chica_free, etc.), **escribirlo al entrar a FREE_TRAINING (zona 52) y restaurar `0B` al salir**. Con el byte restaurado el modo 3 (otro entrenamiento) sigue sin marcador.
- on/off por json: `marcador_nativo_free: true|false` (junto a `marcador_free`). Si es false, no se escribe y se queda el overlay Lua actual.
- No basta si además se cumplen (a)(b)(c) de §1.2, `W+0x13014==1` o `p4==false` por el objeto 0x388. Comprobación segura (solo lectura): tras aplicar P1, leer el byte `gestor+0x18B4` (M+0x9C); si sigue 0, localizar cuál de esas condiciones lo anula.

---

## 2. Cómo se alimentan marcador y reloj

### 2.1 Goles (CONFIRMADO en bytes; fuente del contador PROBABLE)
- La pantalla lee `M+0x9E` y `M+0x9F` (bytes, tope 99; `FUN_1420EFEC0`). Los escribe `FUN_1420C9080`: para i=0,1: `FUN_1408C9DF0(base_equipo_i, 5)` con `base_equipo_i = *([raíz+0x150]) + 0x750D0 + i*0x59650`; el stat 5 de `FUN_1408C8FC0` es la suma de 7 contadores de gol por tipo (`FUN_1408C9190(…,0..6)`). Equipo 0 = local.
- `ProcesarGol FUN_140478190` NO incrementa ese marcador: arma el evento de gol (goleador, cámara/secuencia). El gol se registra por los observadores `match::record::Observer*`: `ObserverGoal` (`FUN_140A13EE0`, vtable 0x142640040) dispara cuando `S+0x116C` pasa de 1 a 2 con `S+0x1574 == 3` (k=3 = gol) → evento 0x1F con los datos de `S+0x15C0`. Con el parche `gol_nativo_free` (k=3 conservado, cuevas 0x4A5921/0x4A592E) esto ya ocurre.
- Prueba viva: `content\phoenix\estado.json` (escrito por `phoenix_referee.lua` leyendo `match.stats()`): `"goles_local":1,"minuto":7,"segundo":35,"periodo":1,"reloj_corre":false`. O sea el contador de goles y el reloj existen y se leen en el entrenamiento; solo el HUD está apagado y el reloj parado. No hace falta tocar el contador.
- Quién incrementa físicamente el stat 5 no se localizó (cadena observador → registrador `FUN_140A115A0`/`FUN_140485100`, sin seguir). No es necesario para la propuesta.

### 2.2 Reloj — dónde vive y cómo se lee (CONFIRMADO salvo lo marcado)
- Reloj del partido = estructura de 0x30 bytes en **S+0x1168** (S=`[raíz+0x270]`): `+0` fase (`S+0x1168`), `+4` sub-fase (`S+0x116C`: 1 en juego, 2 parada, 3 …), `+0xC` **float "transcurrido"** (`S+0x1174`, escritura plana sin protección: setter `FUN_140A3CD70` = `movss [rcx+0xC],xmm1`), `+0x10..+0x2F` **base protegida** con hash FNV-1a (`FUN_140A3C070` la decodifica; `FUN_140A3CBC0`/`FUN_140A6C050` la codifican; si el hash no cuadra ejecuta `int3`). **Tiempo = `FUN_140A3C050(S+0x1168)` = base + float[S+0x1174]**.
- **NO escribir nunca la base** (4 slots + hash + contador): cuelga el juego. El float `+0xC` sí es libre.
- Unidades: 60 unidades = 1 segundo de reloj ("fotogramas a 60 Hz"). `FUN_140A6BD00(t, fase, &seg, &min)`: seg = (t/60)%60; min = ceil(t/3600) + {0, 45, 90, 105 según fase 1/3/5/7}. Es decir la 1.ª parte muestra 0:00→45:00 con t=0→162000. El tope (clamp) de `t` es 21600.0 en el helper `FUN_140A6BFA0` (a 0x142647C60 = 21600.0; el valor 162000.0 está en 0x142647C68). El HUD (`FUN_1420EFEC0`) suma la base de la fase vía `FUN_1420BE140` (45 para el 2.º tiempo, etc.) y el "añadido" por `FUN_1420BE050`.
- Duración de la parte: la fija el ajuste "minutos" `[ajustes+0x58]` (float, 1..30, relleno por `FUN_141FC35B0` — es donde actúan `pattern_set_min_time/max_time` de Sider). Incremento por fotograma (`FUN_140A6C050`): `90.0 / (((minutos / (x5C·(60/fps))) / x60))` con `x = [raíz+0x2C0]` (objeto de velocidad: +0x58 minutos, +0x5C escala, +0x60 velocidad). Con minutos=M, 45:00 de reloj se alcanzan en M minutos reales → el HUD avanza 45/M minutos de reloj por minuto real (con M=30, ×1.5; el tope M≤30 impide 1:1).

### 2.3 Quién avanza el reloj y por qué no corre (CONFIRMADO)
`FUN_14044CE80(job)` (función por fotograma, 0x1069 B; su prólogo `40 55 41 56 41 57 48 83 EC 40 48 8D 6C 24 20 …`) avanza el reloj del JOB (`job+0x2790`, misma estructura: fase `+0x2790`, modo `+0x2794`, transcurrido `+0x279C`):
```
FUN_140A3CDA0(job+0x27C0, job[0x2794], job[0x27DB]!=0)           // contador
if ([raíz+0x400] != 0 && FUN_141F9C3B0(TW))  return;             // TrainingWork byte0 != 0  <<< SALTA TODO
if ([raíz+0x3B0] != 0 && FUN_1423E7FD0(obj)) return;             // [obj+0x9C] != 0
if (FUN_141863350() == 0) {                                      // siempre 0
   … si S+0x116C==1: …; si ==3 y S+0x1574∉{0,9}: …
   if (FUN_140A6BF50(S,0)) {         // true si S+0x116C ∈ {1,3} (y 2 con condiciones)
       modo = job[0x2794]
       modo==1 → FUN_140A6C050(job+0x2790, speedObj)        // suma a la base protegida
       modo∈{2,3} y fase∈{1,3,5,7} → dt = 90·0.5/…; FUN_140A3CD70(clock, dt + job[0x279C]); FUN_140A3CBB0(clock, …)
   }
}
```
Por eso en el Entrenamiento libre (TW activo) el reloj nunca avanza → `reloj_corre:false`, 7:35 congelado (los 7:35 vienen de antes de que TW se activara o de una fase inicial: no medido).
- Cómo llega el reloj del job al S+0x1168 que lee el HUD/`match.stats()`: **no se localizó** la sincronización (HIPÓTESIS: ambos son la misma estructura en objetos distintos y `FUN_140A3C180(MatchInfo)` lee `MatchInfo+0x1168`; el job usa `FUN_140422F20(job+0x17DC8)` = MatchInfo, probablemente S). Hay que confirmarlo con la sonda (ver §6).

### 2.4 Parches del reloj (propuestos; NO probados)
- **R1 (1 byte)**: RVA **0x44CEE2**, VA 0x14044CEE2, archivo 0x44C4E2: `74` → `EB`. Salta la comprobación de TrainingWork (el `je` pasa a `jmp` y cae justo en la siguiente comprobación, `rax` sigue válido). Firma (única): `80 B9 DB 27 00 00 00 48 8B F9 8B 91 94 27 00 00 41 0F 95 C0 48 81 C1 C0 27 00 00 E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8B 88 00 04 00 00 48 85 C9 74 14` (inicio RVA 0x44CEB1; el byte es el offset +49).
- **R1b (1 byte, solo si hace falta)**: RVA **0x44CF02**, VA 0x14044CF02, archivo 0x44C502: `74` → `EB` (comprobación de `[raíz+0x3B0]`). Firma: `48 8B 88 B0 03 00 00 48 85 C9 74 0D E8 ?? ?? ?? ?? 84 C0 0F 85 D0 01 00 00 E8 ?? ?? ?? ?? 84 C0 0F 85 C3 01 00 00` (inicio RVA 0x44CEF8; byte +10).
- on/off json: `reloj_nativo_free`. Solo con zona 52 + cancha chica, restaurar `74` al salir.
- Riesgos del reloj nativo: (1) el fin de la parte 1 (45:00) dispara la cadena de fin de tiempo (§3); (2) la fase "descanso" y las demos `tu_half_*`; (3) `ObserverMatchTime` anota un evento por minuto (solo cuando `S+0x116C==1`): nuevo ruido en la sonda pero inocuo; (4) los "contadores" `job+0x27C0/0x27C8…` y `job+0x27D0` (celebración) que el Core ya mide comparten esta zona: R1 NO debería alterarlos (`FUN_140A3CDA0` corre antes del salto).
- Duración configurable `street_minutos` (NATIVO): como el HUD muestra "reloj de partido" (0→45), la forma limpia es fijar el ajuste de minutos `x+0x58` (1..30) — p. ej. con `set_match_settings`/`options.match_time` de Sider o escribiendo el float `[raíz+0x2C0]+0x58` — y que el Core termine el partido (reinicio) cuando el reloj mostrado llegue a `street_minutos`. Para ver minutos 1:1 con el tiempo real no hay valor válido (tope 30 ⇒ ×1.5); alternativa en §2.5.

### 2.5 Reloj dirigido por el Core (RECOMENDADA como primera prueba: sin parches de código para el reloj) — PROBABLE
Dejar `FUN_14044CE80` como está y que el Core (hilo propio, ya lo hace con sondas) escriba cada ~16 ms `*(float*)(S+0x1174) = 60.0f * segundos_street − base`, donde `base = decode(S+0x1168)` (normalmente 0 en la fase 1; se puede leer el HUD/`match.stats()` para comprobar que `base=0`). Ventajas: el reloj queda en minutos reales exactos y es el Core quien decide cuándo acaba (`street_minutos`), sin pasar nunca por 162000 (45:00) → el Juez nunca marca fin de tiempo → no hay demo 0x07. Reiniciar = escribir 0.0. Inconvenientes: carrera entre el hilo del Core y el juego (solo escritura de un float alineado: segura); y si el juego sincroniza el reloj del job hacia S en cada fotograma pisará el valor (HIPÓTESIS; en ese caso hay que escribir `job+0x279C`, offset conocido por el Core vía `job+0x2790`). Tiene que probarse con la sonda (§6).

---

## 3. Fin de tiempo seguro (congelones)

### 3.1 Qué decide el time-up (CONFIRMADO la decisión; el resto PROBABLE)
- `FUN_14050C3E0(juez)` (por fotograma): si `S+0x1168` NO está en {2,6,9,10} (máscara 0x644) → `if (juez[0x9C]) FUN_14050B490; if (FUN_14050B9B0(juez) || juez[0xDC]) { juez[0xD0]=0; juez[0xD8]=0; FUN_14050EEE0(juez); }`.
- `FUN_14050B9B0` (bool, 11555 B): calcula `t = FUN_140A3C050(S+0x1168)` y llama a `FUN_140A6BEF0(t, añadido, ET)` = `t ≥ 162000 + añadido·3600` (o 54000/…para prórroga); si es false → `return false`. Si es true sigue con las condiciones de "no cortar una jugada peligrosa" (ventaja, distancia a portería, contador `juez[0xD0] > fps·60` …) y devuelve true para pitar.
- `FUN_14050EEE0` es un trampolín a la VM de `.impdata` (`E9 0B 6B A4 03 …`): el "pitar fin" está virtualizado; no se puede leer estáticamente. Desde ahí el job recibe el registro de parada con `sub==7` (`FUN_14102CAD0(rec,7)`; `FUN_140452E00` monta la cola `0x10, 0x26, 0x45`, y `FUN_1404494E0`, caso 0x1C de `FUN_14044C880`, calcula la fase siguiente: 1→2, 3→`FUN_1404493D0`, 5→6, 7→`FUN_1404499D0`). Los pasos 0x07 (demo `tu_half_*`), 0x10, 0x26, 0x1D (repetición) y 0x45 pertenecen a ese ejecutor de pasos `FUN_14044C880` (switch del paso en `job+0x878[job+0x8F0]`).

### 3.2 Cómo anularlo (propuesta; NO probada)
- **T1 (1 byte)**: RVA **0x50BA3F**, VA 0x14050BA3F, archivo 0x50B03F: `84` → `32` (`test al,al` pasa a `xor al,al`; ZF=1 ⇒ el `je` siguiente salta a `return false`). `FUN_14050B9B0` deja de devolver true por tiempo; `juez[0xDC]` (otra causa) sigue. Firma (única): `44 0F B6 C3 0F 28 CE E8 ?? ?? ?? ?? 84 C0 0F 84 60 08 00 00 83 BE 20 29 00 00 04` (inicio RVA 0x50BA33; el byte es +12).
- on/off json: `sin_fin_tiempo_free`. Solo en zona 52 (restaurar al salir).
- Con T1 el reloj puede pasar de 45:00 sin pitar (sigue sumando hasta el tope de 6 h); en el HUD los dígitos de minuto son 3 (hasta 999).

### 3.3 Qué pasa al acabar `street_minutos` (diseño, sin la demo)
1. Ruta recomendada: el Core compara el reloj mostrado con `street_minutos` (lee `match.stats().clock_minutes/seconds` o `S+0x1174`).
2. Pitido final: no hay función nativa "solo pitido" localizada. Opción: el módulo Lua reproduce un wav propio (el overlay de Sider ya usa `click.wav`; `SoundServer` existe) o el Core lo delega a Link. HIPÓTESIS (no hay API nativa mirada).
3. Resultado: mostrar el marcador final unos segundos con el overlay/Link (`content\phoenix\resultado.json` ya existe) y congelar con la pausa de gol existente.
4. Reinicio sin demos: `goles=0` y `reloj=0`. El reloj se reinicia con la escritura plana de `S+0x1174` (y `job+0x279C`), nunca tocando la base. El marcador (`team stat 5`, byte por contador de tipo, estructura `[raíz+0x150]+0x750D0+i·0x59650`) NO tiene escritura documentada: el módulo `scoreboard-hexx.lua` hace `stats = match.stats(); stats.home_score = 0; stats.away_score = 0` (`reset_scores`) → Sider permite ESCRIBIR el marcador de `match.stats()`. Ese es el camino de reinicio (llamarlo en el hilo de Lua, ya que Sider lo hace en su ciclo). CONFIRMADO que existe en `scoreboard-hexx.lua`; que funcione en zona 52 es HIPÓTESIS.
5. Partido continuo (alternativa): no hacer nada tras `street_minutos`; el reloj sigue y el marcador acumula.

---

## 4. Nivel de riesgo de congelón
- P1 (HUD): MEDIO. Crea una pantalla nueva en un modo que nunca la crea: puede intentar cargar nodos de AFP inexistentes (el `FUN_1420F04C0` hace `FUN_141F8B550(nodoPadre, ruta)` y comprueba `!=0` antes de usar cada nodo, lo que juega a favor) y el `FUN_1420F1750` manda mensajes de estado al AFP (`vtbl+0x80`). La experiencia previa (deadlock juego↔render en la repetición, `FUN_141717150`) vino de cargar escenas/cámaras, no de pantallas 2D, pero NO se puede descartar. Medida: activar P1 solo cuando el partido ya está en juego (no durante la carga) y mantener el vigilante de bucles del Core.
- R1 (reloj nativo): MEDIO-ALTO por el fin de tiempo y fases; con T1 baja a MEDIO. §2.5 (reloj del Core) es la opción de menor riesgo.
- T1: BAJO (cambia un salto a "return false" dentro del Juez; sin tocar memoria ni VM).
- NO usar: parchear `FUN_14047C2D0` (lección previa) ni tocar la base protegida del reloj (int3).
- Cierre con clave nula en `FUN_140476C50` (mencionado por el coordinador): `FUN_140476C50(S-ish)` lee `lVar6 = *(param_3+0x18)` y `lVar10 = [S]`; el reloj/HUD no la llaman. No hay relación directa hallada; es un riesgo propio del gol nativo, no del HUD.

## 5. Plan B — overlay de Sider (si lo nativo no es viable)
Qué se vio en el código real:
1. `phoenix_marcador_free.lua` (v0.6) ya cuenta goles (`goal_scored`) y reloj (`display_frame`, `os.clock()`), pero lo muestra solo en `overlay_on` (texto) y vuelca `marcador_free.json` para un widget de Phoenix Overlay.
2. `overlay_on(ctx)` puede devolver `texto, rutaImagen, {image_width, image_height, image_alpha, image_hmargin, image_vmargin, background_color}` (ver `phoenix_mandos_fase1.lua` líneas 2450-2467): imagen BMP generada por el módulo con alfa y posición en fracción de pantalla (OJO: solo valores <1; 1.0 = 1 píxel). En partido (`P.modoPartido`) la usa translúcida. Es decir: SE PUEDE dibujar un marcador propio (BMP con dígitos/nombres) con alfa en la esquina superior.
3. Limitación: el overlay solo se pinta cuando la ventana de Sider está abierta (`overlay.vkey.toggle=0x20`, `next-module=0x31`, `prev-module=0xC0`, ubicación `top`). `phoenix_mandos_fase1.lua` lo resuelve con `AUTO_PANTALLA`: pulsa esas teclas con `phxf1_tecla` solo si PES tiene el foco, y hay una sola ventana por módulo ("only module that currently controls overlay can block input"). Habría que coordinar con la pantalla de lados para no pelear por el overlay. No hay otra API de dibujo permanente (`display_frame` es solo un evento por fotograma; en `sider.dll` no hay "draw").
4. Reutilizar el scoreboard de CGP/hexx: NO como lienzo. Son AFP `game2d*.bin` que el motor carga en la pantalla nativa y que Sider parchea en memoria (`livecpk_data_ready`); sin la pantalla nativa activa no se pintan. Lo que sí se reutiliza: los `game2dPes*.bin` de `livecpk\Scoreboard` si se activa el nativo (P1).
5. Alternativa sin Sider: Phoenix Overlay externo leyendo `marcador_free.json` (ya soportado) o Phoenix Link (`estado.json` ya trae minuto/segundo/goles).
6. Recomendación de campos para el overlay: marcador de `match.stats().home_score/away_score` (ya existe en entrenamiento) en vez del conteo propio por `goal_scored`, y reloj de `stats.clock_minutes/seconds` o el cronómetro propio mientras el nativo siga parado.

## 6. Qué NO se pudo determinar (y cómo cerrarlo, sin riesgo)
1. Valor de `S+0x1168` (fase) y `S+0x116C` en la zona 52 y si `W+0x13014 == 1` o `W+0x3F308 == 3` o `S+0x177F≠0` ahí (cualquiera de ellos ocultaría la pantalla aunque se parchee P1). Sonda de solo lectura: loguear cada 2 s `S+0x1168, S+0x116C, S+0x177F, W+0x13014, W+0x3F308, gestor+0x18B4`.
2. Quién sincroniza el reloj del job (`job+0x2790`) con `S+0x1168`, y si el HUD reflejará el avance tras R1/§2.5. Prueba: con R1 activo, logear `S+0x1174` y `job+0x279C` a 10 Hz.
3. Qué AFP (`game2dPes.bin` u otro) se usa en el torneo 29/zona 52 y si contiene `/score/score_contents/*` y `/losstime/*`; se ve en `peticiones.log` (ya hay registro de petición de archivos).
4. Quién incrementa físicamente el stat de goles por equipo (no hace falta: ya cuenta).
5. Pitido final nativo sin demo: no localizado.
6. El interior de `FUN_14050EEE0` (virtualizado en `.impdata`): no analizable estáticamente.
7. No se ejecutó nada en PES: todo lo anterior es estático; los parches P1/R1/R1b/T1 NO están probados.

## 7. Resumen de parches propuestos (todos 1 byte, en .text; restaurar al salir de la zona 52)
| id | RVA | VA | cambio | json | confianza |
|---|---|---|---|---|---|
| P1 HUD marcador | 0x20C93D7 | 0x1420C93D7 | `0B`→`FF` | `marcador_nativo_free` | CONFIRMADO la causa; efecto sin probar |
| R1 reloj (TrainingWork) | 0x44CEE2 | 0x14044CEE2 | `74`→`EB` | `reloj_nativo_free` | CONFIRMADO la causa |
| R1b reloj (objeto 0x3B0) | 0x44CF02 | 0x14044CF02 | `74`→`EB` | idem | PROBABLE (solo si 0x3B0 existe en zona 52) |
| T1 sin fin de tiempo | 0x50BA3F | 0x14050BA3F | `84`→`32` | `sin_fin_tiempo_free` | PROBABLE |
Orden de prueba sugerido: (1) sonda de §6.1; (2) P1 solo; (3) §2.5 reloj del Core; (4) si hace falta R1 + T1.
Archivos de apoyo: `frente_J_datos\J1_dis.txt` (desensamblados de las hojas), `frente_J_datos\_gh_J1.bat`.
