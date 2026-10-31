# Frente K: celebracion de gol en el Entrenamiento libre con cancha chica (PES2021.exe, solo estatico)

Base 0x140000000; RVA = VA - base. Fuente leida: `bodies.pkl` (decompilado completo), `callers2.pkl`, `clases_pes.csv`, bytes del exe de `copias\exe_pes2021` (pe.py), `unique_sig` de fox.py, notas cap_62/cap_74 y frentes C y G. No se toco ningun proceso ni archivo fuera de esta carpeta. No se uso Ghidra (los `func_0x...` indefinidos se leyeron a mano en bytes).
Niveles: CONFIRMADO = leido en decompilado/bytes y coherente; PROBABLE = deducido de varios indicios; HIPOTESIS = por patron o nombre.
Terminologia: `root = [exe+0x36F3DC8]`; `S = [root+0x270]` (estado de partido); `dims = [root+0x2AA8]` (9 floats); `job` = el trabajo grande del partido (ctor FUN_140441490).

## 0. Resumen ejecutivo

1. El destino de la celebracion NO sale de una sola constante. Hay tres fuentes (A, B, C). Dos de ellas son leibles en el exe (A y B); la tercera (C) esta en datos de demo que no estan en el exe.
   - A. Ancla de la escena de celebracion (posicion absoluta donde arranca/centra la demo): `FUN_140476C50` elige por una tabla de "tipo de ancla" (1..7). Casi todos los tipos usan `dims` vivas (ya escaladas por la cancha chica). Excepciones con metros absolutos del ESTADIO REAL: tipo 4 (`registro_estadio+0x130..0x138`) y tipo 7 (limites del registro de estadio). CONFIRMADO.
   - B. Carrera del goleador / celebrantes: la accion de jugador `GoalEvent` (estado 0x54, `FUN_1407ED3A0`) calcula el punto final como la INTERSECCION del rumbo del jugador con el borde del "area de demo" del estadio, que sale del registro de estadio por jugador (8 puntos Vec3 en `registro+0x38..0x97`, vienen de `demoarea_stNNN.json`). Esas coordenadas son metros absolutos del estadio real (105x68 y mas alla, hasta vallas/tribunas). CONFIRMADO. Esta es la causa mas probable de "salen del campo chico hacia tribunas/banderin/banquillo".
   - C. Movimiento guionizado de la demo (`dml_goal_celebrate_NNNN`, ~300 clips; ver 3.3): trayectorias relativas al ancla dentro de los datos de la demo. NO determinable estaticamente. HIPOTESIS.
2. Gancho recomendado (no es un hook de codigo, es un parche de DATOS idempotente, mas seguro): reducir en memoria los 8 puntos de area de demo del registro del estadio actual a +-(dims-0,5) mientras dure el street, y restaurarlos al salir. Complemento opcional: hook de salida de ancla en `FUN_14047C500` (clamp de x,z a +-(dims-0,5)). Detalles en la seccion 4.
3. Alternativas sin hook (seccion 5): editar el byte "tipo de ancla" de los registros de 0x3C bytes (datos); recortar por timer (ya existe); NO saltar el paso 0x04 (deadlock conocido).
4. NO se pudo determinar (seccion 7): el valor de `S+0x134C` en FREE_TRAINING (decide que rama de `GoalEvent` corre), si los registros de estadio se reconstruyen en cada partido, y la trayectoria guionizada de cada demo.

## 1. Cadena de la celebracion (de la cola de escena a los actores)

| Paso | Funcion | VA | RVA | Que hace | Conf. |
|---|---|---|---|---|---|
| Montador de la cola | `FUN_14044F8A0` | 0x14044F8A0 | 0x44F8A0 | Arma `job+0x878[]` (indice de escritura `job+0x8F1`): 0x1D (si `FUN_140A3C020`==0), luego 03 04 16 12 13 14 15 0E 34 19 1A (+0x17 si `S+0x1574`==5; +11 variantes 03 11 08 si `FUN_1434A2D40.vf+0x88`!=1). Rama alternativa 43 07 44. | CONFIRMADO |
| Despachador de pasos | `FUN_14044C880` | 0x14044C880 | 0x44C880 | `switch(job+0x878[job+0x8F0])`; guarda el paso en `job+0x2B68`. Casos 3..0xB -> `FUN_140446390`. Avanza `+0x8F0` y limpia `+0x900/+0x8F2/+0x8F8/+0x8F4` cuando el paso devuelve true. | CONFIRMADO |
| Familia 3..0xB | `FUN_140446390` | 0x140446390 | 0x446390 | switch: 3 -> `FUN_140447230`; **4 -> `FUN_140446B20`**; 5 -> 446960; 6 -> 446520; 7 -> 447290; 8 -> 446F80; 9 -> 4466F0; 0xA -> 446DD0; 0xB -> 447690. | CONFIRMADO |
| Paso 0x04 (celebracion) | `FUN_140446B20` | 0x140446B20 | 0x446B20 | Maquina por `job+0x8F2`: 0 = arranque (llama `FUN_140478190(job+0x1E8, ...)`, `FUN_14047DCF0`, `FUN_14044E460`, `FUN_14047ADC0`, `FUN_140453AB0`); 1 = evento 0x1050042; 2/1 = espera con `FUN_14047C2D0` y por fotograma `FUN_140472640` + `FUN_14047B460`; 3 = cierre (`FUN_14047DD10`). Sale con `S+0x1775 != 0` (retorna 1 al instante). | CONFIRMADO |
| Montador de la escena de gol | `FUN_140478190` | 0x140478190 | 0x478190 | Suma goles (S/`FUN_140A557C0`), elige celebracion del jugador (mapa `FUN_140A7CC00()+0x438`, clave `FUN_140A62D90(...,2)`; si no hay, clave 0) -> `param_1[0x4E]`; llama `FUN_140477570` (nivel 0..2), `FUN_140477BC0` (lista de jugadores participantes) y **`FUN_140476C50` (ancla)**; escribe la ancla en `param_1+0x30` y la pasa a `FUN_14047C500`. | CONFIRMADO |
| Ancla | `FUN_140476C50` | 0x140476C50 | 0x476C50 | Ver 2.1. | CONFIRMADO |
| Arranque de demo | `FUN_14047C500` | 0x14047C500 | 0x47C500 | Escribe en el gestor de demos (`FUN_141F500D0()`, singleton 0x52E0 B, global `0x1437F5D50`) el id de escena, la ancla (xyz en `+0x5208..+0x5210`) y crea la demo con `FUN_141F5E730` (o 141F620D0 si id 0x80001). 7 llamadores. | CONFIRMADO |
| Objeto "gol" por partido | `[root+0x1A8]` | | | `+0` id de escena de celebracion (getter `FUN_1421B7D40` = `mov eax,[rcx]`), `+4` indice del goleador (getter `FUN_140A73E20` = `lea rax,[rcx+4]`; valido si <0x16), `+0x6C` byte (getter 140A73E30), `+0x98/+0x9C/+0xA0` ANCLA xyz (getter `FUN_140A605A0` copia `+0x98` (8 B) y `+0xA0`). | CONFIRMADO (bytes) |
| Estados de actor | tabla RTTI (PARTE1) | | | `GoalMove` 0x49 (`FUN_1407F0A20`), `GoalEvent` 0x54 (`FUN_1407ED3A0`), `GoalEventRunningAction` 0x55 (`FUN_1407EE9E0`), `GoalEventFinishAction` 0x56 (`FUN_1407EF4A0`), `DemoMove` 0x4A, `DemoAction` 0x4B, `DemoTimeUp` 0x4C. Estado en `actor+0xAC8`; gestor de acciones en `actor+0xAB8`. | CONFIRMADO (RTTI + setstate_llamadas.csv) |
| Test de llegada (IA) | `FUN_140A0BCB0` | 0x140A0BCB0 | 0xA0BCB0 | Solo si el actor esta en estado 0x54 (`FUN_1406B4C10(x,0x54)`), `S+0x116C==2` y `S+0x1574` en {3,5}: decide si el celebrante "ya llego" (devuelve 1) comparando contra el borde del area de demo (mismos `registro+0x40/0x44/0x64/0x70/0x74`) o contra la ancla (modo 3, distancia `ai+0x134/0x138/0x13C`). | CONFIRMADO (lectura); semantica de uso PROBABLE |

### 1.1 Quien es quien (distinguir celebrantes)
- Goleador: `scorer = *(uint*)([root+0x1A8]+4)` (CONFIRMADO; en `FUN_140A0BCB0` se compara `*piVar10 == *(int*)(param_1+4)` con el indice del jugador de la accion de IA). En escena 0x300F5 el goleador tiene tratamiento especial.
- Lista de participantes de la escena: `FUN_140477BC0(param_1, &vec, scorer)` (vector de ids, 4 B cada uno) -> se vuelca a `param_3[...]` con tipo 0x1B (CONFIRMADO). Esos ids son los que reciben el estado 0x54/0x49/0x4B.
- Companeros: PROBABLE que corran con `GoalMove` (0x49): su `Actualizar` (`FUN_1407F0A20`) hace `FUN_141EB34B0(actor+0x4D0, actor+0x20, &angulo, frames,...)` con el angulo hacia la posicion del goleador (`FUN_140439E50(.., actor+8, pos_goleador)`), o sea giran/avanzan hacia el goleador; no toman destino absoluto propio. Si el goleador se sale, ellos lo siguen.
- Distinguir por actor en un hook: estado en `actor+0xAC8` (0x54/0x55/0x56 = celebrante activo; 0x49 = sigue al goleador; 0x4A/0x4B/0x4C = demo), e indice de jugador por el gestor de acciones (`actor+0xAB8`, vf+8).

## 2. De donde sale el destino

### 2.1 Ancla de la escena (`FUN_140476C50`, 6018 B de texto, 1104 B de codigo)
Firma `(this=rcx, out=rdx (x,y,z como 3 floats), jugador=r8, flag=r9b)`, `void`. `this+8` = id de escena (0x300xx).
- Con `flag!=0`: usa una copia de datos del jugador (`jugador->[0x18]+0x24F2C...`) y `FUN_140476190`.
- Con `flag==0`: `tipo = *FUN_140A7CBA0(FUN_140A7CC00(), id_escena)` -> `switch(tipo)`:

| Tipo | Origen del ancla | Escala con dims? | Conf. |
|---|---|---|---|
| 1 y default (con condicion sobre `this+0xBC`) | `FUN_140476190` / `FUN_1404B6ED0` (posicion del goleador/zona relativa; usan `dims[0..1]`, `dims[4]`, `dims[5]`, margenes `_DAT_14259CE3C` etc.) | SI (dims vivas) | CONFIRMADO (lectura parcial) |
| 3 | `FUN_1404B77D0`: x = sig(x_jugador)*(dims[0]+`_DAT_14259CE3C`), z = +-(dims[1]-c); si `FUN_1404B9300` y escena en {0x300DC..} usa esquinas de estadio (`registro+0x58/+0x7C`, `FUN_1404B6390`) | MIXTO (dims + registro del estadio) | CONFIRMADO |
| **4** | `registro_estadio[ [ [root+0x2C0]+0x20 ] ]` -> **`+0x130,+0x134,+0x138`** (xyz absolutos) | **NO** | CONFIRMADO |
| 5 | posicion del goleador `(jug+0x55C)` desplazada en x por `lado_equipo*_DAT_142588210` (`lado = FUN_140A48A30(...)+0x254`) | no hace falta (relativa al jugador) | CONFIRMADO |
| 6 | posicion de otro jugador (`S+0x162C`) si cumple flags; si no, `(0,0,0)` | no hace falta | CONFIRMADO |
| **7** | `FUN_140476010`: x,z de la demo 0x30007 (`FUN_141F4FC30`); si |valor| >= `_DAT_14253EE30` se recorta a **`registro+0x44/+0x80` (x) y `+0x40/+0x64` (z)** | **NO** | CONFIRMADO |

Registro de estadio: `rec = FUN_141E5D4C0(NULL, id_estadio)`, `id_estadio = *(int*)([root+0x2C0]+0x20)`. `FUN_141E5D4C0` = `mov rcx,[0x1437F4808]; test; jz->0; jmp FUN_141E5CA20`; `FUN_141E5CA20(P,id) = *(*(*P + id*8)+8)` (con comprobacion `id < FUN_141E5DE80()`). Global de la tabla: **RVA 0x37F4808**. CONFIRMADO (bytes).

### 2.2 Carrera del celebrante (`GoalEvent::Actualizar`, `FUN_1407ED3A0`, 4154 B)
Decide por `m = S+0x134C` (entero). Primera fase (`gestor_acciones.vf+0x20` devuelve 0):
- `0 <= m < 3` o `m == 4` (y `m` no es 3/7): el destino es el punto donde la recta del rumbo del actor corta el borde del area de demo: se arman dos puntos con `x = (actor.x>=0) ? rec+0x74 : rec+0x44` (rama "x") o `z = (actor.z>=0) ? rec+0x64 : rec+0x40` (rama "z", cuando `m==2`), el otro eje con +-100 / `_DAT_14253EE30`; luego `FUN_140A8F7C0` (punto a distancia) y **`FUN_140A90D30` (interseccion segmento/recta)**, y el angulo con `FUN_140439E50`. Caso especial estadio id 0xB2: recorta con `dims[5]`. CONFIRMADO.
- `m == 3`: `x = sig(x_actor)*dims[0]`, `z = sig(z_actor)*dims[1]` = la esquina de la cancha LOGICA (ya escalada). CONFIRMADO.
- `m == 7`: usa la posicion del goleador (`FUN_140A73E20`) y `FUN_1406B03D0`; sin metros absolutos visibles. CONFIRMADO (lectura parcial).
- cualquier otro `m` (incluido `m<0`): `FUN_1406EEBB0(actor, 2)` (vuelve a estado 2 = movimiento normal; el celebrante NO corre). CONFIRMADO. => si FREE_TRAINING tuviera `m==8` (valor que aparece en ~15 comparaciones `== 8` del programa) esta rama no correria. NO se sabe el valor real (seccion 7).
- `GoalEventRunningAction` (0x55, `FUN_1407EE9E0`) mide contra `dims[0]` (+margen) y `dims[2]` (3,66, anchura de porteria): ya escalado. `GoalEventFinishAction` (0x56) sin lecturas de dims/estadio. CONFIRMADO.

### 2.3 El "area de demo" del estadio (la fuente absoluta)
Evidencia de que `registro+0x38..0x97` son 8 puntos Vec3 (stride 0xC) de esquinas, de `demoarea_stNNN.json` / `demoarea_bill_*.json` (cadenas en .rdata 0x2B4B410..): getter `FUN_1404771F0(.., out, jugador, pos, flagB)`:

| Cuadrante (signo x, z) | Conjunto A (flag=0): (x,z) | Conjunto B (flag!=0): (x,z) | Punto |
|---|---|---|---|
| (-,-) | (+0x38, +0x40) | (+0x44, +0x4C) | P0 / P1 |
| (-,+) | (+0x5C, +0x64) | (+0x50, +0x58) | P3 / P2 |
| (+,+) | (+0x68, +0x70) | (+0x74, +0x7C) | P4 / P5 |
| (+,-) | (+0x8C, +0x94) | (+0x80, +0x88) | P7 / P6 |

Es decir `Pk = rec + 0x38 + 0xC*k`, k=0..7; A = {P0,P3,P4,P7}, B = {P1,P2,P5,P6}. CONFIRMADO (los offsets cuadran con 1404771F0, con la fusion min/max de `FUN_1407331A0` y con `GoalEvent`).
Otros lectores del mismo registro (afectados por el parche de datos): `FUN_1407331A0` (= `CornerKickLoop` slot 12, vftable 0x25DF5C8: recorta el destino a [min(+0x44,+0x50), max(+0x74,+0x80)] x [min(+0x40,+0x94), max(+0x64,+0x70)] +-`_DAT_142565600`), `FUN_1407E3050`/`FUN_1407E4F50` (`OutofPlay`), `FUN_140793D40`, `FUN_140943E90`, `FUN_1409EF270`, `FUN_142177AE0` (sistema de efecto/spray con +-1 m de margen), `FUN_1405ACA50`, `FUN_1406139xx/1406301xx`. Nota: parchear estos puntos tambien achica "hasta donde sale" un jugador en saques de esquina/banda (deseable en street, no verificado).

## 3. Clases y datos relacionados

### 3.1 RTTI (clases_pes.csv)
`match::anime::action::GoalMove` 0x25E3120, `GoalEvent` 0x25E2D80, `GoalEventRunningAction` 0x25E2E68, `GoalEventFinishAction` 0x25E2F50, `DemoAction` 0x25DCF98, `DemoMove` 0x25E6428, `DemoTimeUp` 0x25E2C98, `InplayDemo*` 0x25E28F8..0x25E2BB0, `PKDemo` 0x25E32F0, `match::player::ActionGoalGet` 0x2631BB0, `ActionDemo` 0x263CE20, `match::pad::ThinkUnitInteractiveGoalDemo` 0x260FAC0, `match2D::Screen::GoalPerformance` 0x2BE1A88, `match::registry::DemoControlRef/DemoInfoRef/DemoPlayerInfoRef/FixDemoInfoRef/CornerFlagInfoRef`. No existen clases `Celebration`, `Joy` ni `Banner` de partido.

### 3.2 Corrección importante sobre `constant::StudiumPositionData` (vtable 0x2B54228)
NO es la tabla de posiciones de estadio de la celebracion. Su deserializador (`FUN_141E6BC60`, escribe floats en `+8..+0x88`) lee campos de IA defensiva (`adjustAngleOneSideCutRev`, `coverDist`, `checkDistBaseToCover`, `diffTargetLineX`...). Lo que si define las areas de celebracion/vallas son los JSON `demoarea_stNNN.json` y `demoarea_bill_*.json` (ver 2.3). CONFIRMADO (cadenas + funcion leida). Esto corrige la hipotesis del enunciado.

### 3.3 Datos de celebracion fuera del exe
- Clips `dml_goal_celebrate_0001..0314` (~300 nombres en .rdata 0x2B94BD0+) y `dm_oop_gk_*`; cadenas `**GOALDEMO**%d:`, `GOALDEMO_MOVELAYER layer Use Both/One`, `PLAY_DATA_DEMO_INTERACTIVE_CELEBRATION`, `OFFSIDE_GOAL_CELEBRATE`. No tienen referencias directas por LEA (se cargan por hash/puntero en tablas). La trayectoria de cada clip es dato, no codigo.
- Tabla de "tipo de ancla" por escena: `FUN_140A7CBA0(tabla, id)`: si `(id & 0xFF0000)==0x30000` indexa `(id&0xFFFF)-1` en un vector de registros de **0x3C B** en `tabla+0x448..0x450` (singleton `FUN_140A7CC00`, global **RVA 0x36F4FB8**); `registro[0]` = tipo (1..7), `registro[1]` = flag. Si el indice >= cuenta devuelve **NULL** y `FUN_140476C50` hace `switch(*NULL)` => cierre. CONFIRMADO (bytes de 0xA7CBA0).

## 4. Gancho propuesto (celebracion_en_chica)

### 4.1 Principal: parche de datos del registro de estadio (sin hook de codigo)
Activar solo en street (cancha chica) y con json `celebracion_en_chica: true`. Pseudocodigo (idempotente, por tick o al entrar en el paso 0x04):
```
id  = *(int32*)(*(u64*)(root+0x2C0) + 0x20)
P   = *(u64*)(exe+0x37F4808);  if(!P) return
A   = *(u64*)P;               E = *(u64*)(A + id*8);  if(!E) return
R   = *(u64*)(E + 8);          if(!R) return          // validar id<512
if (R != guardado.R) { guardado.R = R; copiar R+0x38..0x97 (96 B) y R+0x130..0x13B (12 B) }
dx = dims[0]-0.5f;  dz = dims[1]-0.5f
sx = {-1,-1,-1,-1,+1,+1,+1,+1};  sz = {-1,-1,+1,+1,+1,+1,-1,-1}    // k=0..7
for k: *(float*)(R+0x38+0xC*k+0) = sx[k]*dx;  *(float*)(R+0x38+0xC*k+8) = sz[k]*dz   // y (+4) intacta
R+0x130 = clamp(R+0x130, -dx, dx);  R+0x138 = clamp(R+0x138, -dz, dz)               // ancla tipo 4
al salir del street / descargar el Core: restaurar las copias
```
- Cubre: carrera de `GoalEvent` (rama m<3/4), test de llegada `FUN_140A0BCB0`, ancla tipos 3/4/7, y saques de esquina/fuera (`CornerKickLoop`, `OutofPlay`). Margen: `GoalEvent`/`FUN_1407331A0` suman `_DAT_142565600`; si el celebrante queda pegado al muro invisible (15,2 / 10,2) subir el margen a -1,0.
- Obtencion del registro: llamar a `FUN_141E5D4C0(NULL, id)` o leer la cadena como arriba. Firma de `FUN_141E5D4C0` (unica, 32 B, lleva RIP-relativo en bytes 3..6 y un E9 rel32): `48 8B 0D ?? ?? ?? ?? 48 85 C9 75 03 33 C0 C3 E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC`. Alternativa: leer la global por el desplazamiento RIP de ese `mov` (RVA 0x37F4808).
- Riesgos: (a) si el juego reconstruye el registro en cada partido el parche se pierde: por eso se re-aplica por puntero. (b) no dejar parchado al volver a un partido normal en el mismo estadio. (c) escritura desde el hilo del Core mientras el hilo de juego lee floats: sin bloqueo, pero escritura de 4 B es atomica; aun asi aplicar fuera del paso 0x04 (antes de la cola) si se puede. CONFIANZA de que el parche mueve el destino: PROBABLE (la lectura es CONFIRMADA; que el camino concreto sea el de la fuga visible se debe medir en vivo, ver 6).

### 4.2 Complemento: clamp de salida de la ancla (hook de codigo)
`FUN_14047C500` (RVA 0x47C500; `void(int* this, u64 jugador, uint* st, int tipo, int flag)`) recibe `st[0]=id, st[1]=x, st[2]=y, st[3]=z`. Hook pre-llamada: `st[1]=clamp(st[1],-dx,dx)`, `st[3]=clamp(st[3],-dz,dz)`; `kx/kz` no hacen falta (clamp, no escala). Firma unica de 32 B, sin RIP en los primeros 14 B ni call:
`40 56 57 41 56 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 60 48 89 6C 24 68 4D 8B F0 48`
Alternativa en la fuente: post-hook de `FUN_140476C50` (clamp de `out[0]` y `out[2]`):
`48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 00 01 00 00 48 C7 44 24 40 FE FF FF FF 48 89 58 20 0F 29` (40 B; ver riesgo de clave nula en 6). Pero 476C50 tiene 8 llamadores, mejor 47C500 (embudo unico hacia el gestor de demos).
La ancla viva tambien se reescribe por fotograma (`FUN_140472640`, escenas 0x3013D..0x3013F y 0x300F4 siguen a la pelota/goleador): quedan dentro del campo si los actores lo estan.

### 4.3 Si hiciera falta recortar al actor (ultimo recurso, hook de codigo)
`GoalEvent::Actualizar` (`FUN_1407ED3A0`, RVA 0x7ED3A0, firma 32 B unica: `48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 98 48 81 EC 30 01 00 00 48 C7 45 D0 FE FF FF`; args `(rcx=this, rdx=actor, r8=u64*)`). Envolverla con "parchear registro -> original -> restaurar" tiene el mismo efecto que 4.1 pero solo durante la celebracion; coste: 96 B por actor por fotograma. Otras firmas utiles (todas de `unique_sig`):

| Funcion | RVA | VA | Firma AOB |
|---|---|---|---|
| GoalMove::Actualizar | 0x7F0A20 | 0x1407F0A20 | `48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC E0 00 00 00 0F 29 70` |
| GoalEventRunningAction::Actualizar | 0x7EE9E0 | 0x1407EE9E0 | `40 53 55 56 57 41 54 41 56 41 57 48 81 EC 90 00 00 00 4C 8D B2 B8` |
| GoalEventFinishAction::Actualizar | 0x7EF4A0 | 0x1407EF4A0 | `48 89 5C 24 18 55 56 57 41 56 41 57 48 83 EC 60 4C 8D` |
| Test de llegada FUN_140A0BCB0 | 0xA0BCB0 | 0x140A0BCB0 | `48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC B0 00 00 00 48 C7 45 B7 FE FF FF FF 48` |
| Montador gol FUN_140478190 | 0x478190 | 0x140478190 | `48 8B C4 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 D8` |
| Getter areas FUN_1404771F0 | 0x4771F0 | 0x1404771F0 | `40 55 48 8B EC 48 83 EC 50 48 C7 45 D8 FE` |
| Ancla tipo 3 FUN_1404B77D0 | 0x4B77D0 | 0x1404B77D0 | `48 8B C4 55 57 41 56 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 A7` |
| Ancla tipo 7 FUN_140476010 | 0x476010 | 0x140476010 | `40 57 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 40 48 89 74 24 50 48 8B FA 33 C0 48 89` |
| Despachador de pasos FUN_14044C880 | 0x44C880 | 0x14044C880 | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 30 45 33 FF 4C` |
| Interseccion FUN_140A90D30 | 0xA90D30 | 0x140A90D30 | `48 89 5C 24 08 57 48 83 EC 30 48 8B 44 24 60 33 FF 48` |
| Tabla de tipos FUN_140A7CC00 | 0xA7CC00 | 0x140A7CC00 | `48 83 EC 38 48 C7 44 24 20 FE FF FF FF 48 8B 05 ?? ?? ?? ?? 48 85 C0 75 34 C7 44 24 40 19 00 00 00 45 33 C0 BA 60 04 00` |

## 5. Alternativas sin hook

- E1 (recomendada con 4.1): el parche de datos de 4.1 ya es "sin hook".
- E2 (HIPOTESIS, experimento barato): sobrescribir el byte `registro[0]` (tipo de ancla) de los registros de 0x3C B (`tabla = *(u64*)(exe+0x36F4FB8)`, vector `[tabla+0x448, tabla+0x450)`) por 5 (ancla = junto al propio goleador, relativa al jugador y por tanto siempre dentro). Ventaja: no hay claves nulas (solo cambia un valor valido). No toca la carrera de `GoalEvent`, ni el guion C.
- E3: variantes "sin carrera": no se encontro una accion estatica `ActionCelebration*`/`ActionJoy*` (no existen en el RTTI). La unica rama estatica es "cualquier `S+0x134C` fuera de {0,1,2,3,4,7}" que manda `GoalEvent` a estado 2 (no corre). Cambiar `S+0x134C` es PELIGROSO: lo leen ~15 sitios mas (IA de portero, saques, `FUN_14092A340`, etc.). NO recomendado.
- E4: acortar por timer (`job+0x27D0=4000` a los ~8 s, ya en el Core). No evita la salida inicial.
- E5: NO saltar `job+0x8F0` en el paso 0x04 (deadlock render ya documentado en cap_62).
- E6 (descartada): reemplazar JSON `demoarea_stNNN.json` en el CPK por uno chico. Seria el cambio de datos "oficial" pero afecta a todos los modos y requiere el formato exacto (no verificado). Se menciona solo porque el parche en memoria de 4.1 es su equivalente.

## 6. Riesgos y como distinguir celebrantes

- Cierre con clave nula (FUN_140476C50): `puVar9 = FUN_140A7CBA0(tabla, *(this+8))` puede devolver NULL (indice >= cuenta o clase de id distinta de 0x03xxxx) y el codigo hace `switch(*puVar9)`. Nunca llamar a 476C50/47C500 con un `this` de prueba ni cambiar el id de escena a un valor fuera de [0x30001, 0x30000+cuenta]. CONFIRMADO (bytes de 0xA7CBA0 + decompilado).
- Congelon por salto de pasos: saltar `job+0x8F0` durante 0x04 deja un deadlock juego<->render (cap_62). Tambien `S+0x1775 != 0` hace que el paso 0x04 salga al instante (no usar como atajo sin entender).
- Parche del registro: si se deja parchado, otros modos con el mismo `id_estadio` heredan areas chicas (corner/out of play). Restaurar siempre; guardar los originales por puntero `R`.
- `FUN_140A90D30` (interseccion) con un celebrante ya fuera de las nuevas esquinas: devuelve "sin interseccion" y el codigo cae a `x=borde, z=z_actual` (ramas con `fStack_c4 == _DAT_142588218`): no hay division por cero visible. PROBABLE.
- Distinguir celebrantes: ver 1.1 (goleador por `[root+0x1A8]+4`; estados 0x54/0x55/0x56 = corren al borde; 0x49 = siguen al goleador; 0x4A/0x4B/0x4C = demo).

## 7. Lo que NO se pudo determinar

1. Valor de `S+0x134C` en FREE_TRAINING (zona 52): decide entre "corre al borde" (0,1,2,4), "esquina logica" (3), "no corre" (resto). Sin ese valor no se sabe si la fuga que ve Frank viene de B (registro de estadio) o de C (guion). Lectura en vivo, solo lectura: `pes_memoria_leer` de `S+0x134C`.
2. La trayectoria y desplazamiento de cada demo `dml_goal_celebrate_*` (C): estan en datos del CPK, no en el exe. Si C domina, el parche 4.1 solo la corta a medias y habra que clamplear la posicion del actor por fotograma (hook en `FUN_141EB5B30`, ya descartado para uso general en el frente C porque tiene 26 llamadores).
3. Si el registro de estadio y la tabla de tipos de 0x3C B se reconstruyen por partido (o viven en el heap de carga).
4. Valores reales de los 8 puntos para el estadio usado en street y del `tipo` de ancla por escena.
5. Semantica exacta de los tipos 2 (default) y 6 de ancla, y de `FUN_1404B9300` (condicion de esquinas de estadio en tipo 3).

### Plan de medicion minimo (lectura, sin escribir) para cerrar 1-4
En el paso 0x04 (con `job+0x8F0` apuntando a un 4): leer `S+0x134C`, `[root+0x1A8]+0/+4/+0x98/+0x9C/+0xA0` (id de escena, goleador, ancla), `id_estadio`, los 8 Vec3 de `R+0x38`, `R+0x130..0x138`, y el byte `tabla.vec[(id&0xFFFF)-1][0]` (tipo de ancla). Comparar la ancla con los 8 puntos y con la posicion final de los actores 0x54/0x49.
