# Frente G — "Fuera" y saques en el Entrenamiento libre con cancha chica (solo lectura)

Base 0x140000000; RVA = VA - base. Fuente: `bodies.pkl` (decompilado), `callers2.pkl`, bytes del exe de `copias\exe_pes2021` (lectura con pe.py), `LEEME.md` de marcador_street, sondas `sonda_*.txt`, cap_60 y cap_73 del libro. No se tocó ningún proceso ni archivo fuera de esta carpeta. Un intento de abrir el exe con REA/Ghidra venció por tiempo (330 s) y se cerró; el resto es estático con las herramientas locales. Los `func_0x...` del decompilado son funciones que Ghidra no definió; leí sus bytes a mano cuando hizo falta.

Decisión del usuario (mensaje del coordinador): street SIN banda/esquina/puerta, solo el gol reinicia. Esta es la opción (a); la (b) se resume al final.

## 1. Resumen ejecutivo

1. HOY (cancha 15x10 parcheada, sin muros): la pelota que cruza la línea lógica SÍ se declara "fuera" y se ejecuta un saque real. Es una clasificación analítica por posición contra el bloque de dimensiones; no depende de los muros ni de la física. CONFIRMADO por código + capturas/sondas (ver §3).
2. Para que NADA dispare un "fuera": neutralizar en el Juez (`match::ai::Judge`, `FUN_14050E4A0`) las dos escrituras que convierten "pelota fuera" en "parada por línea de banda" (cc=3) y "por línea de fondo fuera del arco" (cc=2). Son dos parches de 1 byte (§5). El gol (cc=1) y su detector independiente (`FUN_140A18510`) no se tocan.
3. Margen: el detector marca "fuera" cuando |x|-r > 15 (o |z|-r > 10), con r = 0,10869 (float en 0x142598958). O sea, el CENTRO de la pelota en |x| > 15,1087 / |z| > 10,1087. Un muro en 15,2 que actúe sobre el centro la deja pasar a "fuera" por 0,09 m; solo es seguro si el muro actúa sobre la superficie (centro máx 15,091). Con el parche del §5 esto deja de importar.
4. Riesgo de congelón: BAJO para el parche (retira rutas, no monta escenas nuevas). No se pudo demostrar que no haya congelón por pelota perdida fuera de los muros (§7).

## 2. Cadena de detección "pelota fuera" (de la física al saque)

| # | Paso | Función / dato | Confianza |
|---|---|---|---|
| 1 | Cada fotograma la pelota (clase de simulación, `FUN_140872C10`, 15 323 B) calcula el área: `[bola_info+0x2798] = FUN_140A72E70(dims, x - sgn(x)*r, z - sgn(z)*r, ...)`. `bola_info = FUN_1404335E0(ball+0x4E0)`; `dims = FUN_140871D30(ball+0x2980)` = puntero a la copia del bloque de 9 floats vía FieldInfoRef (`*(ref+0x10)+4`) | CONFIRMADO (leído) |
| 2 | `FUN_140A72E70` (RVA 0xA72E70): `fuera si |x|-r > dims[0] o |z|-r > dims[1]`. Devuelve máscara: dentro = 1; fuera por x = 10 (0b1010); fuera por z = 6 (0b0110); bit 1 (valor 2) = "fuera". Añade bits de cuadrante/área (0x10..0x4000) con dims[2..7] y dims[0x11] | CONFIRMADO |
| 3 | Si antes bit0 y ahora bit1: guarda la posición de salida en `bola_info+0x27B8` (x,y,z) y la anterior en `+0x27C4` (`FUN_140872C10`, tramo "uVar4&1 && now&2") | CONFIRMADO |
| 4 | `ObserverBallArea` (vtable 0x142640D90, slot1 = `FUN_140A17810`): si `bola_info+0x2798` cambió y `[S+0x11B4]==0` publica EVENTO tipo 10 con `ev+0x14 = máscara` (`FUN_140A3D060(ev,10,...)`) | CONFIRMADO |
| 5 | Receptor de eventos del job `FUN_1404514F0` (llamado por `FUN_140423000`): al final siempre hace `Judge.bind(FUN_14050B2B0(job+0x908, job+0x49C0, ...))`, `FUN_14050E4A0(job+0x908, ev)`, `unbind(0x14050C3C0)` | CONFIRMADO |
| 6 | `FUN_14050E4A0` = `Judge::OnEvent` (objeto `job+0x908` = `match::ai::Judge`, vtable 0x1425C50F0, ctor `FUN_14050B260`). Para ev==10: exige `[S+0x116C]==1` y `ev.flags&2`. Con `|salida.x| <= dims[0]` (salió por banda) y `|z_pelota| > dims[1]` pone `job+0xCC = 3`. Si salió por x: `FUN_140A51CB0(27B8,27C4,27D0,dims)` (¿la trayectoria entra en la boca? ver nota) → `job+0xCC = 1` (gol) salvo excepciones por último toque → `job+0xCC = 2` (fondo fuera) | CONFIRMADO el flujo; la semántica de 51CB0 es HIPÓTESIS (función no definida en Ghidra, no leída) |
| 7 | `FUN_14050C3E0` (por fotograma, desde el gestor): `FUN_14050D610`; si `job+0x9C` llama `FUN_14050B490` (despacha por `job+0xCC`); `FUN_14050B9B0` es el detector de "estancamiento" (contador `job+0xD0`, 60 s sin progreso, etc.), NO el de fuera | CONFIRMADO |
| 8 | `FUN_14050B490` según `job+0xCC`: 1 → `FUN_14050E110` (gol: `e770(team, tipo1, k=3)`, `41 B9 03` en 0x50E1FB); 2 → `FUN_14050E020` (tipo 3 puerta si `ballx*dir>=0` si no tipo 4 córner; k=4); 3 → `FUN_14050E320` (tipo 2 banda; k=4). Luego pone `job+0xCC=0` | CONFIRMADO (bytes: `41 B9 04 00 00 00` + `lea r8d,[r9-2]` en e320; `lea r8d,[r9-1]` en e020) |
| 9 | `FUN_14050E770(job, equipo, tipo, k, pos...)` rellena el registro `rec = job+0x3C0 -> job+0x49C0`: pos (`FUN_140A3CD10`), k=rec+0x20 (`FUN_141B8B250`), sub=rec+0 (`FUN_14102CAD0`=tipo), equipo (`FUN_140A3CD90`). Con TrainingWork activo (`[root+0x400]` y su byte0) NO exige `S+0x1775`: registra la parada siempre que tipo != 0. Posición: `FUN_14050D280`/`D250(tipo,[root+0x2AA8],...)`, y recorta banda a `|x| <= dims[0]-c` y penal en z | CONFIRMADO |
| 10 | Gestor `FUN_1404A56C0` (ver §4) lee el registro, fuerza k=7 y publica a `FUN_1404526A0` | CONFIRMADO |
| 11 | `FUN_14050C490` NO es el detector de fuera: elige tipo de reinicio por `S+0x1775` (0x13 banda=2, 0x14 córner=4, 0x15 puerta=3, 0x16 penal=6, otros 1) con k=0 (`45 33 C9`); la llaman `FUN_140449B00` (bucle TW), `FUN_140454C00`, `FUN_1404A5CB0` | CONFIRMADO. Quién escribe `S+0x1775` no se encontró (ninguna escritura literal en el decompilado) |

Nota gol: hay un segundo detector de gol independiente, `FUN_140A18510` (RVA 0xA18510): si `bola_info+0x2798&2` y `FUN_140A51CB0(...)` devuelve 1 = gol; si no, recorre 250 muestras de trayectoria contra `dims[0]`, `dims[2]` (media portería), `dims[3]` (travesaño) y `dims[0xF]/[0x10]`. Se llama desde el observador de toque (`FUN_140A17B30`). Los parches de §5 no lo tocan.

Dimensiones: bloque de 9 floats (cap_73): [0]=semilargo, [1]=semiancho, [2]=media portería 3,66, [3]=travesaño 2,44, [4]=línea del área pequeña, [5],[6],[7]=áreas, [8]=punto de penal. Hay dos copias vivas: la del montón que lee `[root+0x2AA8]` (Juez, posiciones D280, AI) y la del FieldInfoRef que lee la pelota/`A597D0`. El Core parchea "2 bloques"; la sonda de Frank con 15x10 mostró la banda lógica en 10 m, así que ambas lecturas ven 15/10 (PROBABLE; no verifiqué en vivo que la copia de la pelota sea una de las dos).

## 3. Qué ocurre hoy al cruzar ±15 / ±10 dentro de las paredes

CONFIRMADO por código (cadena §2) y por evidencia empírica:
- Captura de Fralex con 15x10 (cap_60 línea 185): "saque de banda casi en el centro (la banda lógica está a 10 m)", suplentes en fila sobre la banda nueva, y "el balón que cruza la línea cerca del portero a ~15 m acaba en saque del arquero".
- Sondas acumuladas hasta el 27-oct 03:53 (`sonda_cuadrado_0353.txt`, entreno): transiciones `0x1->0x6` (banda, k=7 sub=2) ×19, `0x1->0x5` (puerta, sub=3) ×12, `0x1->0x4` (córner, sub=4) ×6, `0x1->0x3` (centro) ×37, `0x1->0xA` (gol) ×52 (k=3). Casi todas vuelven (`->0xE`/`->0x9`): banda 17/19, puerta 12/12, córner 6/6, gol 37/52 en la ventana de 3 eventos (las que faltan pueden ser fin de archivo o cola larga; no verifiqué). No se puede fechar cuáles fueron con cancha chica (log acumulado).
- Tabla por tipo en el Entrenamiento libre (gestor FUN_1404A56C0, TW+4==0 inferido de que las escenas 4/5/6 ocurren; con TW+4 != 0 el gestor NO publica escena: reinicia el ejercicio, ver §4):

| Tipo del Juez (sub) | k original | Efecto hoy en Entrenamiento libre |
|---|---|---|
| 1 centro (tras gol, cc=1) | 3 | Gestor: con `TW+0xC != 1` y `rec+4==1` y k==3 lo reescribe a sub=3 (saque de puerta); el Core ya lo evita (gol_nativo_free + saque_centro_free) |
| 2 banda | 4 | k forzado a 7; escena evento 6 (`0x44F7A4`); posición `D280`: x=pos.x, z=±(dims[1]+0,25) |
| 3 puerta | 4 | k=7; escena evento 5 (`0x44F444`); posición `D280`: x=±(dims[4]+0,7), z=±2,6 |
| 4 córner | 4 | k=7; escena evento 4 (`0x44F1E4`); posición `D280`: x=±(dims[0]+d), z=±(dims[1]+d') |
| 5 / 6 (falta / penal) | — | no los produce la ruta de salida; con "Faltas: No" no se generan |
| con `TW+0xC==1` | | remapea sub 4→3, 3→4, 6→5, 5→6 (cambio de perspectiva atacante/defensor) |

Todas las posiciones de balón salen de `FUN_14050D280` con las dims vivas, o sea ya están escaladas (CONFIRMADO).

## 4. Gestor de parada `FUN_1404A56C0` (6511 B, RVA 0x4A56C0), lectura del flujo

(`TW = [root+0x400]`; `iVar3=FUN_1408E1C50(TW)` = TW+4; `iVar4=FUN_141F8CE70(TW)` = TW+0xC; `cVar2=FUN_140A86010(TW)` = byte1)
- Cada fotograma: bind Juez, `FUN_14050C3E0`, unbind, copia `rec -> job+0x58A0`.
- `rec.sub == 0` (sin parada): flujo normal `FUN_140444A20`.
- `rec.sub != 0`: si `cVar2` (byte1) != 0 → return. Si `[rbp+0x77]==0` (valor `FUN_141052E30(TW)`) y `rec.k==5` → se ignora (RVA 0x4A5910..0x4A591F). Si no: `rec.k=7` en ambos registros (`mov edx,7` en 0x4A5921 y 0x4A592E). Si `TW+4 != 0`: limpia y arma reinicio de ejercicio (`job+0x5818=0x14`) SIN escena. Si `TW+4==0`: aplica los remapeos de §3 y publica a `FUN_140422F20(job+0x17DC8)+0x1554` → `FUN_1404526A0` (k==7 → switch sub: 1→ev3, 2→ev6, 3→ev5, 4→ev4, 5/6→ev7).
- El gestor NO mira zona ni tamaño de cancha. CONFIRMADO.

## 5. Regla street recomendada (a): sin saques, solo gol — PARCHES

Parches de 1 byte en `Judge::OnEvent` (no tocan el gol cc=1). Dejan `job+0xCC=0` (estado de reposo habitual; `FUN_14050B490` lo pone a 0 al final de cada paso). Los dos están en `FUN_14050E4A0` (RVA 0x50E4A0).

| Parche | RVA | VA | Bytes | Efecto |
|---|---|---|---|---|
| P1 (fondo fuera de la boca → cc=2) | 0x50E65F | 0x14050E65F | `02` → `00` | no hay puerta/córner |
| P2 (banda → cc=3) | 0x50E68D | 0x14050E68D | `03` → `00` | no hay saque de banda |

Firmas AOB (únicas en todo el exe, comprobadas; sin RIP):
- P1 (el byte a cambiar es el nº 19, offset +0x12): `C7 87 CC 00 00 00 01 00 00 00 EB 38 C7 87 CC 00 00 00 02 00 00 00 EB 2C 48 8B CB E8 ?? ?? ?? ?? F3 0F 10 40 08` (inicia en RVA 0x50E64D; los primeros 24 B no tienen comodines y ya son únicos).
- P2 (byte nº 14, offset +0x0D): `41 0F 2F 47 04 76 0A C7 87 CC 00 00 00 03 00 00 00 4C 8B 64 24 40 48 8B 5C 24 48` (inicia en RVA 0x50E680).
Original (para restaurar): P1 imm `02`, P2 imm `03`. Instrucciones: `mov dword [rdi+0xCC], imm32`.

Cuándo aplicarlos: el código es compartido con partidos normales. Aplicar solo mientras `cancha_chica_free` y TrainingWork estén activos y revertir al salir (misma disciplina que el barrido de dims del Core; las cuevas 0x4A5921/0x4A592E del gol nativo ya se aplican así). Alternativa: cueva sobre cada `mov` que haga `mov rax,[root]; mov rax,[rax+0x400]; test rax,rax; jz store; cmp byte [rax],0; je store; skip` (replica la condición de entrenamiento del propio `FUN_14050E770`); más frágil porque cap_73 dice que `raíz+0x400` solo es no nulo en accesos puntuales, aunque el gestor y e770 lo leen cada fotograma. HIPÓTESIS sobre esa fragilidad.

Cinturón y tirantes (opcional, 1 byte, NO recomendado salvo que lo anterior falle): `FUN_1404A56C0` RVA 0x4A5912, `05`→`04` en `83 FF 05` hace que el gestor ignore paradas con k original 4 (todas las de banda/puerta/córner) en vez de k==5. Firma: `45 84 ED 0F 85 71 03 00 00 83 7D 77 00 41 8B 7E 20 75 11 83 FF 05 75 0C 44 88 AE D0 58 00 00 E9 56 03 00 00 BA 07 00 00 00` (única). Limitaciones: solo actúa si `[rbp+0x77]==0` (si no, saltaría directo a "forzar k=7": habría que cambiar también `75 11` en 0x4A590E a `90 90`); quita la regla actual de ignorar k==5; y deja el registro `rec` con sub!=0 hasta que `FUN_14050B9B0`/`FUN_14050EEE0` (un thunk a código protegido, ilegible) lo limpie, con la pelota viva. Por eso es peor que parchear el origen.

Qué pasa con el "fuera por la línea de fondo fuera de la boca": sin P1 es cc=2 → puerta o córner (según lado) en la cancha chica; con P1 no pasa nada: la pelota sigue su curso y debe contenerla el muro lógico. La clasificación de área seguirá marcando bit "fuera" (lo leen `FUN_14094E3A0` para recortar objetivos de IA a dims+margen y `FUN_140A18510`, sin efecto de parada).

Cosas que el parche NO cubre y conviene vigilar:
- Gol: sigue por cc=1 y por `FUN_140A18510`; tras P1 los casos excepcionales de cc=2 con la pelota dentro de la red (último toque tipo 0x10/sub 2 u otro) dejan de generar parada; la pelota quedaría en la red (HIPÓTESIS: no vi ese caso en sondas).
- La boca debe seguir transitable: gol exige que la pelota cruce |x|=15,109 con |z|<3,66 y altura<2,44 (dims[2],[3] sin tocar). Si los muros del Core no dejan hueco en la boca, no habrá gol.
- `FUN_14050B9B0` (estancamiento, 60 s sin progreso) puede disparar `FUN_14050EEE0` con la pelota viva; efecto exacto desconocido.
- Posibles escritores de `S+0x1775` = 0x13/0x14/0x15 (tipo de reinicio) no localizados; si existen, `FUN_14050C490` (por TW byte1 / reinicio de ejercicio) podría reponer un tipo de saque. Se espera 1 (centro) tras gol.

## 6. Margen de las comprobaciones de fuera (para el diseño de los muros)

Todas las comprobaciones leen el bloque de dims con comparaciones estrictas y radio:
- Clasificación: fuera si `|x|-0,10869 > dims[0]` o `|z|-0,10869 > dims[1]` (`FUN_140A72E70`).
- Juez: `|salida.x| <= dims[0]` decide banda vs fondo (`FUN_14050E4A0`); banda exige además `|z_actual| > dims[1]`.
- IA/objetivos: recortan a `dims + _DAT_1425E5AB8` (`FUN_14094E3A0`).
Con muro en 15,2 sobre el centro la pelota queda "fuera" por 0,09 m, o sea cualquier toque al muro dispara el Juez sin el parche. Muro a 15,0 + r (centro máx 15,1087) o menos es seguro; ideal: centro máx 15,09 / 10,09, o muro a la superficie en 15,2/10,2 (equivale a centro 15,091/10,091, margen 0,018 m: justo, no recomendado).

## 7. Riesgo de congelón

- El parche P1+P2 solo hace que no se cree la parada (rutas retiradas). Las escenas de banda/puerta/córner no se montan, así que no añade casos de deadlock. Los congelones documentados (LEEME) vienen de la repetición del gol (0x1D) y la demo de fin de tiempo (0x07), no de las escenas 4/5/6; las sondas muestran 4/5/6 completando su ciclo en entreno sin congelón registrado (estas sondas no miden cuelgues de render, así que no es prueba).
- Riesgos residuales: (1) pelota fuera de los muros sin reposición (no hay "pelota perdida" nativa salvo el temporizador de 60 s que llama a un thunk ilegible); (2) cc=2 excepcional con gol en la red; (3) borrar la parada deja a jugadores y reloj sin la fase "pelota muerta" (estados 0x3A/0x4A); no se vio ningún código que espere esa fase en entreno, pero no está demostrado. Recomendación: Core con "recuperación de pelota" (si |pos| > 15,5 o 10,5, teleport al centro) como red de seguridad, en vez de depender de una parada.
- Evidencia que me falta: prueba en vivo con P1/P2 (no la hago: análisis estático).

## 8. Opción (b), por si el usuario cambia de idea

Los saques reales ya funcionan en cancha chica: las posiciones de balón (`FUN_14050D280`) y el Juez usan `dims` vivas, y se vio banda a 10 m. Lo que quedaría: posiciones de jugadores (frente C: `FUN_1409F5530/5840`, rol 0x6D usa dims; `FUN_140A597D0` lee la copia FieldInfoRef; fila de aparcamiento z=40/45 fija), menú "S. pta./córner/banda" del entrenamiento, y el remapeo de `TW+0xC==1`. Congelón: las escenas 4/5/6 ya corrieron en entreno.

## 9. No determinado

- Semántica exacta de `FUN_140A51CB0` y `FUN_14050EEE0` (no definidas / thunk protegido).
- Si la copia de dims que lee la pelota (FieldInfoRef+4) es una de las dos que parchea el Core (inferido por la captura, no medido).
- Valor de `TW+4`, `TW+0xC` y `[rbp+0x77]` en Entrenamiento libre, y de `[S+0x116C]` (debe ser 1 para que el Juez actúe; las sondas implican que lo es).
- Quién escribe `S+0x1775`; qué produce k==5.
- Efecto en vivo de P1/P2 (sin probar).
