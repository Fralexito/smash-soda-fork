# Frente C: posiciones iniciales y de reinicio de los jugadores (PES2021.exe, solo estático)

Base 0x140000000. RVA = VA - 0x140000000. Fuente: `bodies.pkl` (decompilado completo), `callers2.pkl`, `clases_pes.json`, bytes del exe de `copias\exe_pes2021`. No se tocó ningún proceso ni archivo fuera de esta carpeta.
Niveles: CONFIRMADO = leído en el decompilado/bytes y coherente; PROBABLE = deducido de varios indicios; HIPÓTESIS = por nombre/patrón.
Las firmas AOB son únicas en toda la sección de código (comprobadas con `unique_sig`). Los primeros 14 bytes de cada una no llevan call/jmp rel32 ni RIP-relativo; los saltos condicionales rel32 internos (`0F 84`) se marcan.

## 0. Resumen ejecutivo

1. **No hay fórmula "formación normalizada x dimensiones del campo".** La posición de reinicio de cada jugador sale de una **tabla de Vec3 en metros absolutos** del registro de equipo: `team + 0x4B4C + (idx%11)*12` (x,y,z). Es la cancha grande de 105x68 fija. Por eso parchear el bloque de dimensiones no la mueve. (CONFIRMADO la lectura; PROBABLE que sean metros absolutos; NO se encontró quién la rellena.)
2. **Punto de escalado recomendado (H1):** `FUN_1409F5530` (RVA 0x9F5530), la función que lee esa tabla y devuelve la posición de reinicio del jugador. Escalar la entrada de la tabla justo antes de llamarla y restaurarla después, o escalar la salida.
3. El teletransporte real ocurre en `FUN_1409232C0` (Player: fija `Player+0x624` y llama a `FUN_141EB5B30` = poner posición del actor `AnimePlayer`). Es el sitio para un recorte de seguridad o para recolocar a los "aparcados".
4. Los jugadores que **no participan** del ejercicio (Entrenamiento libre, `TrainingWork` activo) se mandan a una **fila en la banda** con `FUN_140A597D0`: `x=(L/20)*idx-L/2`, `z=W+2.5`, con L,W leídos del `FieldInfoRef` (otra copia de las dimensiones). A 15x10 queda en z=12,5 (fuera del muro). Hay además una **fila de aparcamiento por defecto** con constantes inline (x=(slot%11)*5-30, z=40/45).
5. No se pudo determinar quién escribe la tabla `team+0x4B4C` (carga de `positionKickOff_*.fox`) ni el significado del byte de escena `+0x11B0`. Ver sección 8.

## 1. Estructuras y accesores (CONFIRMADO, bytes leídos)

| Accesor | RVA | Fórmula | Nota |
|---|---|---|---|
| `FUN_140A48A30(root,i)` equipo | 0xA48A30 | `i<2 ? *(u64*)(root+(i+0xA1)*0x28) : *(u64*)(root+0x1928)` | `team`, i=0 local, i=1 visitante |
| `FUN_140A48A10(root,i)` | 0xA48A10 | `i<0x1A ? *(u64*)(root+(i+0xBF)*0x28) : [root+0x1DD8]` | registro de movimiento (pos lógica +0x55C) |
| `FUN_140A489F0(root,i)` | 0xA489F0 | `i<0x16 ? *(u64*)(root+(i+0xF3)*0x28) : [root+0x25F8]` | **objeto Player (IA)** de cada jugador |
| `FUN_140A48910(root,i)` | 0xA48910 | `i<0x1A ? *(u64*)(root+(i+0xA5)*0x28) : [root+0x19C8]` | otra tabla de jugador |
| dimensiones | | `*(float**)(root+0x2AA8)`: [0]=semilargo, [1]=semiancho, [2]=3.66, [4]=47.06, [6]=36.14, [7]=20.12, [8]=41.56 | usado con índices [0],[1],[2],[3],[4],[6],[7],[8],[0x0F],[0x10],[0x18] |
| estado de partido | | `S = *(u64*)(root+0x270)` | `+0x116C` fase, `+0x1168`, `+0x1554` sub-tipo de reinicio (1=centro, 3,4,5,7...), `+0x1558` equipo que saca, `+0x1564/+0x155C` punto, `+0x173C` pateador 1, `+0x1740` pateador 2, `+0x1775` byte de situación, `+0x11B0` byte de escena |

Equipo del jugador idx (0..21): `side = (idx>10 && idx<0x16) ? 1 : 0`; `slot = idx<0x16 ? idx%11 : 0`. Visto idéntico en 10+ funciones (p. ej. 0x140926C90, 0x1409F5530).

**Objeto Player (IA)** (CONFIRMADO por accesos): `+0x18` idx, `+0x624` posición inicial/destino (Vec3), `+0x630` copia, `+0x61C/+0x620` orientación, `+0x7D8` actor `AnimePlayer` (posición del actor en `actor+8/+0xC/+0x10`), `+0x7E0` objeto IA del equipo, `+0xACA0` posición de reinicio (Vec3, la escribe `FUN_14092E7E0`), `+0xACAC` posición previa, `+0xACB8` buffer de contexto, `+0xC76D` flag.

**Registro de equipo** (`team`) (CONFIRMADO lecturas): `+0x26C + slot*0x40` posición base de formación por slot (Vec3, la calcula la IA en vivo), `+0x4B4C + slot*0x0C` **posición de reinicio por slot (Vec3)**, `+0xA2A0 + slot*4` y `+0xB748 + slot*4` rol/acción por slot (0x36 = rol "posición de reinicio", 0x6D = hombre en el palo, 0x24/0x2B...), `+0x254` signo de ataque del equipo (±1), `+0x4C00 + slot*4` flag.

## 2. Cadena de colocación del jugador (CONFIRMADO leída; el orden por fotograma es PROBABLE)

```
FUN_140925750 (Player vtable, 87 B)  -> FUN_140925470 (Player_EntryDispatch, 730 B, por fotograma)
   -> FUN_140926B00 (Player_KindSwitch, 396 B)   switch(S+0x11B0):
        1        FUN_140927360   en juego: solo copia +0x624 -> +0xACAC
        5,7,9    FUN_140926E40   si el equipo es (o no es) el que saca: FUN_1409232C0(player, NULL, -1)  [aparcado por defecto]
        0x0A     FUN_140926EF0   sub==4 y rol 0x36: fuente = team+0x26C+slot*0x40; si no Player+0xACA0 -> FUN_140929610 -> FUN_1409232C0
        0x0C,0x0F FUN_140926690  solo aplica (FUN_14092E7E0 / 14092F5B0 / 14092D580)
        0x0D,0x10 FUN_140926C90  CENTRO si S+0x116C en {2,3} y S+0x1554==1: usa Player+0xACA0; si no, slot 0x36 -> team+0x26C
                                 -> FUN_140929610 -> FUN_1409232C0(player,+0x624,-1) -> FUN_14092E7E0 -> 14092F5B0/14092F320
        0x11     FUN_1409266E0   TrainingWork activo ([root+0x400] byte0!=0): si FUN_140A59EC0 (participa) -> nada;
                                 si no -> FUN_140926940: fila de banda (FUN_140A597D0) -> FUN_1409232C0
        0x14-0x16 FUN_1409270F0  entrada previa: fila de aparcamiento inline y FUN_141EB5B30 directo
   -> FUN_140928FC0 (1606 B) también llama a FUN_1409F5530 (calcula +0xACF0)
```
`FUN_14092E7E0(player, flag)` (1360 B, 9 llamadores) hace `Player+0xACA0 = FUN_1409F5530(&tmp, &player+0x18, player+0xACB8, teamAI)` y luego orientación (FUN_1406B44F0/B3F70/B3DB0/B3EE0 según sub-tipo). Rama rol 0x6D (CONFIRMADO código): x=±(dim[0]-0.5), z=±(dim[2]-0.7) segun signo del punto de reinicio: hombre en el palo (HIPÓTESIS de uso: córners).

## 3. Funciones clave (VA, RVA, tamaño, firma AOB, confianza)

| Rol | VA | RVA | Tam | Firma AOB (único) | Notas de hook | Conf. |
|---|---|---|---|---|---|---|
| **Posición de reinicio del jugador (H1)** | 0x1409F5530 | 0x9F5530 | 779 | `48 8B C4 53 41 57 48 83 EC 78 48 89 70 E8` | 14 B = 4 instrucciones completas. Firma (rcx=out Vec3*, rdx=&idx (uint*), r8=ctx, r9=teamAI). Devuelve rax=out. | CONFIRMADO |
| Ajuste del pateador en saque | 0x1409F5840 | 0x9F5840 | 3901 | `48 89 54 24 10 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24 E9 48 81` | llamado solo por F5530. Frontera de instrucción en 15 B. | CONFIRMADO |
| Calcula Player+0xACA0 | 0x14092E7E0 | 0x92E7E0 | 1360 | `48 89 5C 24 20 88 54 24 10 55 56 57 41 54` | 14 B ok | CONFIRMADO |
| Warp/colocación del Player | 0x1409232C0 | 0x9232C0 | 844 | `40 57 41 54 41 55 41 56 41 57 48 83 EC 60` | (player, float* pos|NULL, float angulo). Fija +0x624/+0x630 y llama FUN_141EB5B30(actor,+0x624,0). 6 llamadores | CONFIRMADO |
| Actor: poner posición | 0x141EB5B30 | 0x1EB5B30 | 116 | `48 83 EC 48 F3 0F 10 5A 08 F3 0F 10 52 04` | (actor, Vec3*, char flag). Ruta general de teletransporte; tiene 26 llamadores (también animaciones en juego), NO escalar aquí | CONFIRMADO |
| Aproximación a la posición (clamp final) | 0x140929610 | 0x929610 | 794 | `48 8B C4 48 89 58 08 48 89 70 10 57 48 81 EC 80 00 00 00 F2 0F 10` | (dst=+0x624, ref=+0xACAC, src, ratio, teamAI, idx). Termina con FUN_140A544D0 (recorte a dims) salvo pateadores. Frontera en 12/19 B | CONFIRMADO |
| Despacho por escena | 0x140926B00 | 0x926B00 | 396 | `48 89 5C 24 08 57 48 83 EC 20 40 32 FF 48` | frontera 13/16 B | CONFIRMADO |
| Entrada Player (fotograma) | 0x140925470 | 0x925470 | 730 | `40 55 41 54 41 55 41 56 41 57 48 83 EC 30 48 8D 6C 24` | | CONFIRMADO |
| Reinicio escena 0x0D/0x10 | 0x140926C90 | 0x926C90 | 421 | `48 85 D2 0F 84 9B 01 00 00 48 89 5C 24 10` | contiene `0F 84 rel32` en el byte 3 (reubicar) | CONFIRMADO |
| Reinicio escena 0x0A | 0x140926EF0 | 0x926EF0 | 509 | `48 85 D2 0F 84 F3 01 00 00 48 89 6C 24 20` | idem | CONFIRMADO |
| Fila de banda (entreno) | 0x140926940 | 0x926940 | 128 | `48 89 5C 24 08 57 48 83 EC 30 F2 0F 10 81` | corte de 14 B parte la instrucción F2 0F 10 81 disp32; usar 18 B | CONFIRMADO |
| Rama TrainingWork | 0x1409266E0 | 0x9266E0 | 422 | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 83 BA 68 11 00 00` | | CONFIRMADO |
| Punto de la fila de banda | 0x140A597D0 | 0xA597D0 | 466 | `48 8B C4 55 41 56 41 57 48 8D 68 A1 48 81 EC 90 00 00 00 48 C7 45 F7 FE FF FF FF 48 89 58 08 48 89 70 10 48 89 78 18 44` | (float* out, int idx) -> out=(x,0,z), ret 1. Frontera en 12/19 B. Lee FieldInfoRef (registro 0x142608810) + 4 | CONFIRMADO |
| Entrada con aparcamiento inline | 0x1409270F0 | 0x9270F0 | 609 | `40 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 58 48 8B DA 48 8B F9 48 8B 05 ?? ?? ??` | primeros 15 B limpios | CONFIRMADO |
| Posición del balón por tipo de reinicio | 0x14050D280 | 0x50D280 | 736 | `48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 80 00 00 00 FF CA 0F 29` | (out, tipo, dims, pos, equipo). Ya usa dims. 1/7=centro(0,0); 2=banda x=pos.x, z=±(dim[1]+0.25); 3=saque de puerta x=±(dim[4]+0.7), z=±2.6; 4=córner x=±(dim[0]+d), z=±(dim[1]+d'); 6=penal x=±dim[8], z=0. El wrapper FUN_14050D250 (48 B) solo reenvía | CONFIRMADO |
| Recorte a campo (dims) | 0x140A54560 / 0x140A544D0 | 0xA54560 / 0xA544D0 | 528 / 133 | `48 8B C4 48 89 58 10 48 89 70 18 55 48 8D 68 A9 48 81` / `48 83 EC 18 F3 0F 10 02 0F 57 ED F3 0F 58` | `(dims, pos, margenX, margenZ)`: \|x\|<=dim[0]+mx, \|z\|<=dim[1]+mz | CONFIRMADO |

## 4. La tabla / fórmula

`FUN_1409F5530(out, uint* pIdx, ctx, teamAI)` (CONFIRMADO, leído completo):
1. `team = A48A30(root, side(idx))`, `slot = idx%11` (idx>=22 -> 0).
2. `out = *(Vec3*)(team + 0x4B4C + slot*12)`; guarda (x,z); `signo = team+0x254`.
3. Si hay objeto tutorial/entrenamiento (`[root+0x3B0]`) activo y el jugador está en él: se sustituye con `FUN_140A43190` (posiciones del ejercicio). Rama `S+0x116C==1` (en juego): casos puntuales (tipos 0x12/0x15 del ejercicio).
4. Si `S+0x116C` es 2 o 3 (parada): `FUN_1409F5840(pIdx, teamAI, &x, &z, signo, ctx)` ajusta para el pateador `S+0x173C` y el segundo `S+0x1740` cuando `S+0x1554==1` (centro): x,z absolutos cerca del punto central. Constantes leídas del exe: ±0.25, ±0.45, 0.55, ±1.5, ±0.5, 0.1 (sin escala: son offsets junto al balón).
5. Al final, si `S+0x1168` está en {2,4,6,10} y `S+0x1554==7`: **fuerza (0, y, 100.0)** = jugador aparcado lejos. Es el único "ocultar" explícito encontrado.

No hay multiplicación por `dims`: cada coordenada se usa tal cual. Luego `FUN_140929610` hace aproximación y recorte `FUN_140A544D0(dims,pos,0)` (usa dims ya parcheadas -> mete al jugador dentro), pero el **primer teletransporte** (Player+0xACA0 -> actor) no se recorta. Eso explica el síntoma.

Tabla alternativa `team+0x26C+slot*0x40` (posición base de formación en vivo, rol 0x36): la leen 0x140926C90/EF0 (rama no-centro) y ~40 funciones de IA. No encontré escrituras directas; HIPÓTESIS: la calcula la IA de equipo (`match::ai::Team`, ctor 0x1404F0530; función grande 0x1404F0E50, 14468 B) con la tabla `constant::BasePosition` (JSON `team/basePosition.json`: `kickoffDfLineX` +0x214, `gklBaseX` +0x1C4, `gksBaseX` +0x1E8, `margin_restartX` +0x24C, etc.).

Datos .fox/JSON relacionados (CONFIRMADO por cadenas en .rdata, SIN xrefs directas -> Fox carga por hash):
- `./common/match/position/positionKickOff/positionKickOff_{4_1_2_3,4_2_3_1}_{offence,defence}.fox` (RVA 0x2597100, 0x2597150, 0x25971A0, 0x25971F0; array de cadenas de 0x50 B).
- `DevelopData/common/match/constant/positionCK/positionCK*.json`, `positionPK/positionPK_*.json`, `positionNone.json` (clase `constant::PositionNone`, vtable 0x2B5A678: `playerData[].{enable, playerNo, startPosX, startPosZ}`), `stadium/studiumPositionData.json` (`constant::StudiumPositionData`, vtable 0x2B54228: zonas de estadio, entrada, médicos, etc.), `team/basePosition.json`.

## 5. Dónde y cómo escalar (propuesta de hooks Phoenix Core)

Factores en vivo: `kx = dims[0] / 52.56f`, `kz = dims[1] / 34.06f` donde `dims = *(float**)(root+0x2AA8)` (ya 15/10 tras `cancha_chica_free`): kx=0.2854, kz=0.2936. Guardar los originales 52.56/34.06 como constantes.

**H1 (principal) FUN_1409F5530, envoltorio "escala fuente, llama, restaura":**
```
float* hk_F5530(float* out, uint* pIdx, void* ctx, void* teamAI){
  if(!street) return orig(out,pIdx,ctx,teamAI);
  uint i=*pIdx; int slot=(i<22)? i%11 : 0; int side=(i>10 && i<22);
  char* team=*(char**)(root+(side+0xA1)*0x28);            // = A48A30
  float* e=(float*)(team+0x4B4C+slot*12);
  float sx=e[0], sz=e[2]; e[0]=sx*kx; e[2]=sz*kz;          // fuente escalada
  float* r=orig(out,pIdx,ctx,teamAI);
  e[0]=sx; e[2]=sz;                                        // restaurar (la IA/otros leen la tabla cruda)
  return r;
}
```
Ventajas: el pateador del centro (+0x173C/+0x1740) se sobrescribe con offsets absolutos junto al balón dentro de F5840 (no se escala doble); el marcador (0,100) no se toca; idempotente. Alternativa de salida (post): `out[0]*=kx; out[2]*=kz` salvo pateadores y (0,100). Mejor la de fuente.
La función se ejecuta solo con la carga (`FUN_14092E7E0` y `FUN_140928FC0`), no hace falta tocar `team+0x4B4C` de forma permanente.

**H2 (seguridad) FUN_1409232C0:** envoltorio que, si `pos!=NULL`, recorta `pos` a `|x|<=dims0, |z|<=dims1` (o llama `FUN_140A544D0(dims,pos,0,0)`) antes de la llamada original. Cubre cualquier camino no escalado (incluida la tabla `+0x26C`). Para `pos==NULL` (fila de aparcamiento por defecto z=40/45): sustituir por una posición propia (p. ej. (0,0,100) como hace el juego, o (x=(slot%11)*0.75-7.5, z=dims1+3) si se quieren visibles en la banda nueva). Los valores 5.0/30.0/40.0/2.5/20.0 son constantes **compartidas** (1875/695/501/358/956 xrefs): NO parchearlas en .rdata.

**H3 FUN_140A597D0 (fila de banda del entreno):** tras la original, si devolvió 1: `out->x=(dims0/20.0f)*idx-dims0*0.5f; out->z=dims1+2.5f` (con las dims vivas) o `out=(0,0,100)` para ocultar. Atención: este lee su propia copia (FieldInfoRef+4); si esa copia sigue en 52.56/34.06 el resultado es z=36.56 (fuera del muro). Es el candidato a "suplentes en fila en la banda" (ver 6).

**H4 FUN_1409270F0 (entrada previa 0x14-0x16):** pone la fila de aparcamiento con constantes inline y llama a FUN_141EB5B30 directamente. Envoltorio post: tras la original, repetir `FUN_141EB5B30(*(actor@player+0x7D8), &posNueva, 1)` con la posición deseada.

## 6. Casos pedidos

- **Portero rival:** no hay código dedicado en el saque de centro: es el slot 0 (`idx%11==0`, idx 11 para el visitante) de la misma tabla `team+0x4B4C` (PROBABLE que el slot 0 sea el portero; la IA de equipo `ActionKeeperBasePosition` 0x1409DC040, 5164 B, calcula su posición en juego con dims). Se escala con H1. En penales/stoppage el rol 0x6D usa dims (sin cambios).
- **Jugador controlado en el saque de centro:** es `S+0x173C` (pateador 1) y `S+0x1740` (segundo), con `S+0x1554==1` y `S+0x1775` en {0,7}. `FUN_1409F5840` lo coloca junto al balón en offsets absolutos (±0.25/0.45 o 0.55/±0.25 y para el segundo ±0.1/±1.5 o ±0.5, multiplicados por el signo de ataque, tabla `S+0x14C8/0x14CC` elige variante). No necesita escalado; el resto del equipo viene de la tabla.
- **Suplentes en fila en la banda:** (a) en Entrenamiento libre son los jugadores que NO participan del ejercicio: `FUN_1409266E0` -> `FUN_140926940` -> `FUN_140A597D0` (fila a z=W+2.5, paso L/20). (b) el aparcamiento por defecto inline (`FUN_1409232C0` con pos NULL y `FUN_1409270F0`): x=(slot%11)*5-30, z=40 (equipo 0) / 45 (equipo 1). Ocultarlos: H2/H3/H4 con (0,0,100) (misma coordenada que usa el juego para "fuera") o sacarlos de la lista de participantes. No encontré una bandera "invisible" en el actor.
- **Saque de puerta / córner / banda / penal (posición del balón):** `FUN_14050D280` ya depende de dims, o sea ya escalada. Las posiciones de los jugadores en esas paradas vienen de `FUN_1409F5530` + `FUN_1409F5840` (mismo H1) y de los JSON `positionCK/PK`.

## 7. Qué se leyó

`FUN_140518FE0` (acción Kickoff: registro con tipos 2,3,4,5,6,9,0x10; dims en case 6), `FUN_140731D80`, `FUN_1406DE380`, `FUN_1406DEBB0`, `FUN_140956C30` (ActionBasePosition; accesos a `team+0x26C/0xB748`), `FUN_140A54560/A544D0`, `FUN_14050D280`, `FUN_1404A62B0`, `FUN_140A597D0`, `FUN_1409F0A60/F03C0` (ActionFreeTrainingBreak), `FUN_140925470`, `FUN_140926B00`, `FUN_140926C90`, `FUN_140926E40`, `FUN_140926EF0`, `FUN_140926690`, `FUN_140927360`, `FUN_1409270F0`, `FUN_1409266E0`, `FUN_140926940`, `FUN_1409232C0`, `FUN_140929610`, `FUN_14092E7E0`, `FUN_1409F5530`, `FUN_1409F5840` (parcial), `FUN_141EB5B30`, accesores A48A10/30/F0/910 (bytes), tablas de constantes JSON `BasePosition`/`StudiumPositionData`/`PositionNone`, cadenas de .rdata. Barridos de regex por todo el programa: escrituras a `+0x55C/+0x564`, `+0x26C`, `+0x4B4C`, `+0xACA0`, `+0x11B0`, `+0x2AA8`, xrefs RIP a las cadenas `positionKickOff_*`.

## 8. NO determinado / pendiente

1. **Quién rellena `team+0x4B4C` y `team+0x26C`.** Ninguna escritura directa (ni por puntero) en el decompilado; probable copia en bloque (p. ej. `FUN_1404161E0`, 5047 B, asignación del TeamAIInfo; ctor del registro en 0x1404123A0 / 0x14213C580). Las rutas `positionKickOff_*.fox` no tienen xrefs (carga por hash de Fox).
2. **Significado de los valores de `S+0x11B0`** (1,5,7,9,0xA,0xC,0xD,0xF,0x10,0x11,0x14-0x16) y cuál toma el Entrenamiento libre en el saque de centro: el decompilado solo los compara. `0x11` ↔ TrainingWork es PROBABLE; `0x0D/0x10` ↔ centro es PROBABLE (rama `S+0x1554==1`).
3. **Si `[root+0x2AA8]` y el dato de `FieldInfoRef` (+4) son la misma copia.** Si no lo son, `FUN_140A597D0` usará 52.56/34.06 sin parchear.
4. Coordenadas exactas de la tabla (¿ya en el lado del equipo, con x invertido para el visitante, o crudas y multiplicadas por `team+0x254`?). El código del pateador multiplica por el signo; la lectura directa de la tabla no.
5. `FUN_1409F5840` solo se leyó hasta la rama del segundo pateador; falta el tramo de córner/penal.
6. Verificación en vivo recomendada (lectura): con partido cargado, `team=*(u64*)(root+0xA1*0x28)`, volcar `team+0x4B4C` (11x3 floats) y `team+0x26C+slot*0x40`; comprobar si x llega a ±50 (confirma metros absolutos) y comparar `*(float**)(root+0x2AA8)` con el dato del FieldInfoRef.
