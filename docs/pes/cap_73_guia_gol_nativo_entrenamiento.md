# Cap. 73 — Guía para replicar el GOL NATIVO en el Entrenamiento libre (PES 2021 limpio o parche propio)

> Logro del 26/27-oct-2026 (hilo «modo street»). Resultado: en el Entrenamiento libre el gol se reconoce como gol, hay celebración y repetición, y el juego sigue. Primera vez en PES 2021.
> Esta guía está escrita para rehacerlo **en PES 2021 limpio** (base del parche propio de Frank), no solo en Conmegol.

## 1. Qué hace falta

| Pieza | Para qué | De dónde |
|---|---|---|
| PES 2021 (exe de Steam) | el juego | limpio |
| Sider con `luajit.ext.enabled = 1` | carga nuestro DLL | cualquier Sider |
| **Phoenix Core** (`phoenix_core.dll` + `phoenix_core_loader.lua` + `phoenix_core.json`) | los arreglos dentro del exe | repo `smash-soda-fork`, carpeta `paquetes/phoenix_core` |
| Llegar al Entrenamiento libre | el modo base del street | Día de Partido → Entrenamiento → Entrenamiento libre (cap_60_o §8.3–8.4) |

El exe en disco **no se toca**. Sin el DLL, PES es 100 % original.

## 2. Instalación (cualquier PES)
1. Compilar: `cmake -S paquetes/phoenix_core -B build -A x64` y `cmake --build build --config Release` (MSVC).
2. Copiar `phoenix_core.dll` a `paquetes/phoenix_core/SiderAddons/phoenix_core/`.
3. Con PES cerrado, ejecutar `INSTALAR.ps1 -Pes "<carpeta de PES>"`. Hace copia de `sider.ini`, activa luajit y añade `lua.module = "phoenix_core_loader.lua"` (antes de la línea de mandos si existe).
4. En `SiderAddons/phoenix_core/phoenix_core.json` poner `"gol_nativo_free": true`. Para apagarlo, `false`. `DESINSTALAR.ps1` lo quita todo.
5. Comprobar `SiderAddons/phoenix_core/phoenix_core.log`: cada función dice «OK» o «firma no encontrada».

## 3. Por qué el gol no contaba (la cadena completa)
1. El juego decide la «razón de pelota muerta» con el selector `FUN_1404526A0`: lee `k=[obj+0x1574]`, `sub=[obj+0x1554]` con `obj=[[global 0x1436F3DC8]+0x270]`. **k==3 = gol**.
2. En el Entrenamiento libre corre además el gestor de paradas del entrenamiento `FUN_1404A56C0` (lee TrainingWork `[[global]+0x400]`). Este **fuerza k=7** con dos `mov edx,7 ; call FUN_141B8B250`. Con k=7 y sub=3 el gol se convierte en **saque de puerta** (evento 5).
3. **Arreglo 1 (cueva):** en los dos `mov edx,7`, si el k original (`edi`) es 3, se mantiene 3; si no, 7 como siempre.
   - Firma: `BA 07 00 00 00 49 8B CE E8 ?? ?? ?? ?? BA 07 00 00 00 48 8B CB E8` (sitio 1 = inicio, sitio 2 = +13).
   - Cueva: `cmp edi,3 ; jne +7 ; mov edx,edi ; jmp vuelta ; mov edx,7 ; jmp vuelta`.
4. Con k=3, el juego pone el evento 0xA (gol) y llama a `ProcesarGol` → registra goleador, celebración, repetición.

## 4. Los dos crashes después del gol (y su arreglo)
- El entrenamiento tiene **menos de 11 por lado**. `FUN_140A62690(lado, p)` devuelve **0xFF** cuando no hay jugador, y `FUN_1405B1740` usa ese 0xFF como índice de ranura sin comprobarlo → acceso a una vtable nula → crash.
- **Arreglo 2 y 3 (cueva genérica `cuevaIndiceFF`):** `cmp eax,0FFh ; jne +5 ; jmp salida ; <bytes originales> ; jmp vuelta`.
  - Sitio A: firma `E8 ?? ?? ?? ?? 48 63 C8 41 C7 44 8E 34 09 00 00 00 48 8D 41 05`, sitio = firma+5, 12 bytes, salida = sitio+0xBE.
  - Sitio B: firma `E8 ?? ?? ?? ?? 48 8B 94 24 80 01 00 00 48 63 C8 83 7C 8A 34 09 0F`, sitio = firma+5, 11 bytes, salida = sitio+0xBE (epílogo).
- Ojo para el futuro: **cualquier modo con menos de 11 por lado** (street 3v3, 5v5) puede pisar este mismo fallo; el arreglo sirve para todos.
- Hay además un desvío de seguridad en `FUN_140504D20` (firma `48 89 54 24 10 48 89 4C 24 08 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC A8 00 00 00 48 8B 05`) que no llegó a hacer falta; es inofensivo.

## 5. El congelón al final de la celebración — causa SIN confirmar
- Síntoma (2 veces, Core 0.4.4 y 0.4.7): unos 7 s tras el gol, en la celebración, PES se queda congelado (sin crash). Los hilos del motor esperan una cola de carga (FUN_1403E0760) que nadie llena.
- Sospechoso: un **error de Lua del parche Conmegol**. `SiderAddons/modules/TournamentCornerFlags.lua` usa la global `CornerFlag`, que solo se rellena para torneos conocidos; en el entrenamiento (torneo 29) queda `nil` y `log("-- " .. CornerFlag)` falla en el callback de trofeo/entrada. En el amistoso ese camino no se activa y no hay error.
- **27-oct 01:55: DESCARTADO como causa.** El selector restaura el archivo original en cada arranque, y con el original el gol no se congeló. El congelón sigue sin causa conocida (quizá intermitente).
- **CORRECCIÓN 27-oct 01:40:** en la prueba que salió bien, el arreglo de ese Lua **no estaba puesto** (el archivo volvió a su versión original, sospecha: el selector restaura módulos al arrancar) y aun así NO se congeló. Así que el congelón puede ser intermitente y el Lua no está probado como causa. Sigue en observación con el vigilante (señal: sonda quieta ≥8 s).
- Arreglo preventivo (solo Conmegol, copia `.antes-fix`): línea 92 `if key and CornerFlag then`, línea 98 `log("-- " .. tostring(CornerFlag))`. En PES limpio ese archivo no existe.
- Regla para el parche propio: **todo módulo Lua que use datos del torneo debe aguantar `nil`** (el entrenamiento no tiene torneo).

## 5b. Tras el gol saca el arquero (saque de puerta) — `saque_centro_free`
- Amistoso: `EVENTO 0x1->0xA k=3 sub=1` (sub=1 = saque de centro). Entreno: `k=3 sub=3` (saque de puerta).
- Quien pone sub=3: una rama de FUN_1404A56C0 (0x1404A5B78) que solo corre con k==3 y recoloca el registro con la posición del arquero (FUN_14050D250(&pos,3,…), FUN_14102CAD0(rec,3)).
- Parche de 1 byte: firma `41 83 7E 04 01 75 69 83 FF 03 75 64 4C 8B 05 ?? ?? ?? ?? 4D 8D 4E 08 8B D7 48 8D 4D E7`, en firma+10 `75`→`EB` (la rama queda muerta, sub=1 como en el amistoso). Core 0.4.8, sin probar aún.

## 6. Direcciones fijas que quedan (para portar a PES limpio)
| Qué | Conmegol | Qué hacer en PES limpio |
|---|---|---|
| Global del partido `0x1436F3DC8` (`objPartido()`) | RVA fija | sacarla por firma de una instrucción RIP-relativa que la lea (pendiente, Core 0.4.8+) |
| Sección del código | «.trace» | Core ya busca en todas las secciones ejecutables |
| Funciones arregladas | por **firma AOB** | deberían servir tal cual; si una no aparece, el log lo dice |

## 7. Comprobación rápida (lista para la prueba)
1. `phoenix_core.log`: «gol_nativo_free OK» con 4 parches.
2. Gol en entrenamiento → `sonda_partido.txt` muestra `EVENTO 0x1->0xA k=3` y `GOL …`.
3. `sider.log`: tras el gol sale «Commentary file detected» y ningún `lua ERROR`.
4. Celebración + repetición y el juego continúa.

## 8. Pendiente (siguiente)
- Marcador y reloj en el Entrenamiento libre (función `marcador_free`, Core 0.4.8).
- Global por firma (portabilidad total).
- Colisión con la red, porteros en los dos lados.
- Idea de Frank (27-oct): paredes invisibles: apagar «fuera» (banda sub=2 y fondo) y rebote de la pelota en los bordes desde Core (leer posición/velocidad de la pelota cada fotograma e invertir la componente que cruza, con pérdida de fuerza). Visual opcional con estadio propio. Ruta alternativa A: colisión real con la geometría (.geom) de un estadio AddOn — validar con una pared de prueba. Ruta B (lógica en Core) primero; falta mapear posición y velocidad de la pelota.


## 9. SOLUCIÓN FINAL del congelón (27-oct, Core 0.4.15) — lo que hay que replicar
- El congelón NO era un Lua, ni un archivo de estadio, ni un paso concreto: tras el gol, el entreno monta una cola de pasos con **celebración (0x04) + repetición (0x1D, 11, 08, 23) + demo de fin de tiempo (0x07 + 10 26 45)**. El entreno no graba repetición y no tiene reloj de partido, así que la repetición y el «tiempo cumplido» se quedan esperando para siempre (deadlock juego↔render).
- Arreglo (`gol_directo_free`, solo datos, solo con TrainingWork): en el gancho del montador FUN_14044F8A0 (firma en cap_62/LEEME del paquete), DESPUÉS del original, reescribir la cola `job+0x878[]` y su nº `[job+0x8F1]` a:
  - con celebración (`gol_directo_celebracion`, por defecto): `03 04 16 12 13 14 15 0E 34 19 1A`
  - sin celebración: `03 16 12 13 14 15 0E 34 19 1A`
- Junto con `gol_nativo_free` (k=3) y `saque_centro_free` (sub=1) da: gol → celebración → saque de centro.
- ⚠ No enganchar FUN_14047C2D0: la protección del exe (.impdata) tumba el juego.
- Si tras muchos crashes todo falla, reiniciar la PC antes de culpar al código.

### 9b. Duración de la celebración y recorte (27-oct, Core 0.4.16–0.4.19)
- El paso 0x04 dura lo que la animación de celebración elegida (medido: 16 s, 31 s, 15 s, 12 s). Su contador es `job+0x27D0` (dword, ≈54/s desde el gol); el paso sale cuando el contador alcanza la duración de la animación Y ésta termina.
- **No saltar el índice de la cola** (`[job+0x8F0]`+1, fase 0): deja la demo/cámara sin liberar y el 2.º gol cae en el deadlock render (0.4.18). Poner solo la fase 3 (0.4.16) no acorta (~13 s más de espera).
- Opción estable (`celebracion_modo:"timer"`): a los N s (por defecto 8) escribir `job+0x27D0 = 4000` (valor mayor que cualquier animación). Resultado en prueba (3 goles): el paso termina ~4–7 s después de escribir (celebración total 12–15 s en vez de 16–31 s), sin deadlock. Json: `celebracion_modo` ("completa" | "timer" | "salto"), `celebracion_segundos`, `celebracion_timer_final`.
- Otros campos medidos: `+0x27C0/0x27C8/0x27CC/0x27D4` contadores de fotogramas; `+0x27D8` fase de escena; `+0x27DC` flags; `+0x900` nº de reinicios.
- ⚠ Modo seguro del Core: si PES se cierra en <60 s tras abrir, `arrancando.flag` se queda y el siguiente arranque va con TODAS las funciones apagadas (se ve como «saca el arquero otra vez»). Borrar el flag (o esperar un arranque normal) antes de probar.

## 10. Cancha lógica pequeña (`cancha_chica_free`, Core 0.4.20, 27-oct) — verificado 30×20
- El «fuera», la línea de gol lógica y la posición del portero salen de un bloque de 36 bytes (9 floats) que el juego reserva en el montón en CADA partido: `[0]` semilargo 52.56, `[1]` semiancho 34.06, `[2]` media portería 3.66, `[3]` travesaño 2.44, `[4]` 47.06, `[5]` 9.16 (área pequeña), `[6]` 36.14, `[7]` 20.12 (área grande), `[8]` 41.56 (punto de penal). Firma (36 B, válida en cualquier PES 2021 limpio): `70 3D 52 42 70 3D 08 42 71 3D 6A 40 F6 28 1C 40 70 3D 3C 42 5C 8F 12 41 5C 8F 10 42 C2 F5 A0 41 70 3D 26 42`.
- Hay 2 copias vivas por partido (más copias muertas inofensivas). Core (hilo, solo con TrainingWork activo, un barrido cada 3 s durante 75 s tras aparecer el entreno) las reescribe: `[0]=semilargo`, `[1]=semiancho`, áreas `[4]..[8]` escaladas (`[5]` y `[7]` con mínimo 4.0/4.5 para no quedar más estrechas que la portería); `[2]`,`[3]` intactas.
- Json: `"cancha_chica_free": true`, `"cancha_semilargo": 15`, `"cancha_semiancho": 10` (30×20 m). Resultado (Frank, 27-oct): sale «fuera» más cerca, gol cuenta con celebración y saque de centro, sin trabas. Lo DIBUJADO (líneas, red, vallas) no cambia (pendiente visual).
- TrainingWork estable (para detectar el entreno): raíz `[0x1436F3DC8]` → `+0x3F8` (objeto de referencia) → `+0x10` (datos) → byte 0 ≠ 0. `raíz+0x400` y `[raíz+0x270]` (partido) solo son no nulos durante accesos puntuales.

## 11. Arcos dibujados a ±15 m (27-oct) — verificado con captura
1. **Origen de los archivos:** el juego pide `Asset\model\bg\common\goal\#Win\goal_post.fpk/.fpkd` (postes) y `goalnet_l|r.fpk/.fpkd`, `goalnet_hangpole_l|r.fpk/.fpkd` (red y travesaños). Con Sider/StadiumServer se sirven desde `content\stadium-server\<país>\<estadio>\AddOn\Goal Post Colour\Global\Asset\model\bg\common\goal\#Win\` (los goalnet de otro estadio sirven de base; no hay herramienta para sacarlos de los CPK).
2. **La traslación del fox2 NO sirve:** los `StadiumModel` de `goal_post.fox2` tienen traslación 0 y el motor ignora una traslación escrita (prueba «diag2»: ±52,5 no movió nada).
3. **Lo que sí funciona:** los `.fmdl` (Fox v2.03) guardan los vértices como float32 xyz (paso 12 B) y sus volúmenes con |x|≈52,5. Mover TODO float32 con 44<|v|<60 una distancia de 37,5 hacia el centro (script `paquetes/phoenix_core/herramientas/mover_porteria_fmdl.py`) y reempaquetar el `.fpk` con GzsTool (mismo tamaño). Los `.fpkd` quedan originales. Resultado: arcos con red y postes a ≈14,4 m del centro.
4. Pasos: `GzsTool x.fpk` → `x_fpk\Assets\pes16\model\bg\common\goal\scenes\*.fmdl` → script sobre cada `.fmdl` → `GzsTool x.fpk.xml` (reescribe el `.fpk`). Para otro tamaño de cancha: dx = 52,5 − semilargo.
5. Pendiente: líneas pintadas del campo (textura `.ftex`), vallas y rótulo.

## 12. Cancha pequeña visual y red (27-oct, noche) — qué funciona y qué no
**Funciona (verificado con capturas MCP):** (a) bloque de dimensiones + cajas (Core 0.4.23 `cancha_chica_free`, firmas por contenido con tolerancia 1e-3: los valores reales son 4,354449/4,35472, no 4,3544/4,3547); (b) arcos visibles a ±15 (`mover_porteria_fmdl.py`); (c) césped escalado a 30×20 con TODAS las líneas pintadas (`instalar_cancha_visual.py`: float32 xyz paso 12 B de `pitch_st002.fmdl`: bloques 0x7c0/0xdb0/0x4118/0x43ec, x·15/52,56, z·10/34,06, más 0x4E0..0x538).
**No funciona todavía:** 
1. *Hueco blanco* donde estaba el césped grande y *césped muy oscuro* (el material usa Enlighten/AO horneado del modelo original; `EnlightenStadiumModel` con enlightenQuality=2). Las dos mallas (st081_pitch_0/1) llevan líneas; no hay suelo liso aparte. Preparado SIN instalar `cesped_segunda_instancia.py` → `trabajo_campo\pitch2\pitch_st002.fpk/.fpkd`: 2.ª entidad `StadiumModel` con copia sin escalar, 4 texturas de línea renombradas a `turf_nrm.tga` (misma longitud) y 3 cm más abajo.
2. *La pelota atraviesa la red* al tirar (y sigue rodando lejos, sin chocar ni con la red vieja ni con la tribuna): ni las cajas (`exe+0x37CF4C0`, solo las lee `FUN_141B0D290` = filtro del job `cobra::And::PhPreProcessJob`, vtable ctor `FUN_141B0D4E0`) ni los `.geom` (servidos por `phoenix_marcador_free` v0.4 desde `content\phoenix\cancha\…` porque StadiumServer solo intercepta `hit\stadium`; la malla desplazada está en memoria) lo arreglan; los `goal_hit/goal_l/goal_r.geom` de FoxAnim se cargan antes de conocer la zona y no están en memoria como floats. Hipótesis pendientes: la colisión de red no existe/está apagada en el entreno (bandera de TrainingWork o cuerpo con bit 0x22 de `[+0x1b8]`), o la física del «AndPh» (red como tela) se crea desde el modelo de red (goalnet*.fmdl locators) y necesita más datos desplazados.

## 13. Paredes de la cancha pequeña (prototipo, apagado por defecto)
`common\render\model\bg\hit\bill\st002\hit_billboard.geom` (dt15_x64.cpk) es la colisión de las vallas publicitarias (x≈±57,7, z≈±26…38; vec4 con x en offset≡0 mod 16 y z en ≡8 mod 16). Escalada x·15/52,56, z·10/34,06 queda a ±16,5 × ±11,2 (junto a la cancha 30×20). `phoenix_marcador_free` v0.4 la sirve desde `content\phoenix\cancha_paredes\…` solo con `"cancha_chica_free": true` Y `"cancha_paredes": true` (por defecto no está: sin cambio). StadiumServer solo intercepta `hit\stadium` (colisión de gradas, `hit_stand.geom`) por estadio; `hit\bill` y `hit\goal` no. Prueba pendiente: ¿la pelota rebota en las nuevas vallas?

## 14. Cuadrado = disparo en campo propio (siempre_disparo_free) — bitácora-guía (27-oct-2026)

**Qué se consiguió.** En el Entrenamiento libre (zona 52) el botón cuadrado dispara desde cualquier parte del campo, también dentro del área propia, donde el juego despeja. Opción de Phoenix Core `siempre_disparo_free` (json), solo con TrainingWork, apagable; el modo de la prueba se elige con `siempre_disparo_modo`. Estado final al cierre del día: patea (modo 4–7), pero con un toque la potencia sale al máximo y la barra se llena (ver §14.4); en curso.

**14.1 Cómo decide el juego «tirar o despejar» (hallazgos).**
- Cada mando tiene un `match::CommandPlayer` (ctor FUN_1408B3BC0, unidades en +0x30, fábrica FUN_1408AD0B0). Las unidades `ThinkUnitShoot` (vtable exe+0x260FB58) y `ThinkUnitClear` (exe+0x260FEE8) comparten el botón 0xF.
- Despachador común FUN_1408AC500 (ranura +0x40). Si el byte +0x18 de la unidad es 0 devuelve 0 (unidad apagada). Estado +0x1C: 1 pulsar, 2 mantener, 3 soltar; llama a las ranuras +0x60/+0x68/+0x70/+0x78. Shoot devuelve la acción 0x55 a los 0,148 s y al soltar; Clear devuelve 0x58 al pulsar.
- El predicado «tiro permitido» FUN_1406B45D0 (exe+0x6B45D0, único llamador FUN_1408AC500) da falso en campo/área propia: entonces Shoot devuelve 0 y gana Clear (despeje).

**14.2 Paso a paso de lo que funcionó (modo 4).**
1. Gancho de 20 B limpios en FUN_1406B45D0 (firma `48 8B C4 55 57 41 55 41 56 48 8D 68 A8 48 81 EC 38 01 00 00 8B BD 88 00 00 00 4C 8B F1 0F 29 70 C8`): con TrainingWork visto en los últimos 3 s devuelve 1.
2. Cambiar, en la vtable de Clear (datos de .rdata, no código), las ranuras +0x60 (FUN_1408AEEE0), +0x68 (FUN_1408AEF70) y +0x70 (FUN_140421F40) por funciones que devuelven 0x55 (acción de tiro) mientras el botón está pulsado y en el instante de soltar. Verificar antes que cada ranura apunta a la función esperada.

**14.3 Intentos fallidos (para no repetirlos).**
- Solo forzar el predicado: carga el tiro pero se cancela justo antes del contacto (finta).
- Anular Clear (devolver 0): la barra queda cargada y no patea; con X sale el regate cuadrado+X.
- Clear devuelve 0x55 al pulsar y mantener pero no al soltar: finta. Hay que mantener 0x55 también al soltar (ranura +0x70).
- Modos 5 y 6 (Clear solo al soltar; forzar +0x18=1 en Shoot): la barra sigue llena.
- x64dbg adjunto a PES: congela el juego y, al cerrar el depurador, PES muere; sin control de su ventana no sirve.

**14.4 Barra y potencia (causa, en curso).** La barra se dibuja en FUN_1408AAA30 con CommandPlayer+0x10 (carga). FUN_1408AB000 pone el medidor a 1,0 cuando el tipo de orden (cmd+0x1768) es 4; ese tipo pasa de 0x4E a 4 al pulsar cuadrado con Clear aunque Clear+0x14 valga 1 (log `[MEDIDOR]`); cmd+0x10 queda en 0 y FUN_1408AC9D0 (acumula la carga +0x20, tope 1,0) sale si +0x14 == 4. Pendiente: la instrucción que escribe cmd+0x1768 (vigía de escritura del Core 0.4.37, `vigia_escritura`) y la que fija la potencia real.

**14.5 Herramientas nuevas del Core para investigar sin depurador externo.** `[MEDIDOR]` (gancho FUN_1408AB000), `[SHOOT]`/`[CARGA]` (ranuras de las unidades), `[VIGIA]` (registros DR0–DR3 de escritura + manejador de excepciones, sin tocar código), `[JUG]` (posiciones de los 22 jugadores: registro = *(raíz+(i+0xBF)*0x28), posición en +0x55C/+0x564).

**14.6 Cosas a tener en cuenta.** Reiniciar PES para cada versión (la DLL no se recarga); borrar `arrancando.flag` si el cierre fue <60 s; no cerrar PES mientras Frank juega; instalador: `/c/Temp/instalar_043x.sh`.

## 15. Temporada fija en el entrenamiento (season_forzada) — bitácora-guía
El gestor de clima del Conmegol (Real-Life-Feel-Weather-Manager) elige la temporada por el mes real, así que cambia entre arranques. Con season 0 (verano) la cancha chica muestra vacío blanco y césped oscuro; con season 1 (invierno) vacío azul y césped claro. `phoenix_marcador_free.lua` v0.5+ registra `set_conditions` y, con `"season_forzada": 1` en phoenix_core.json y la zona FREE_TRAINING, pone `options.season = 1` (log «season 0 -> 1 (entrenamiento)»). El Lua v0.6 añade `"suplentes_cero": 1` (`set_match_settings`, substitutions = 0) como prueba para el banquillo.

## 16. Regla street: juego continuo contra los muros, sin saques (27-oct-2026) — decisión de Fralex
**Regla.** En el Entrenamiento libre con cancha chica no hay saques de banda, de esquina ni de puerta (estilo FIFA Street con paredes): la pelota rebota en los muros y el juego sigue; solo el gol reinicia (saque de centro, cola de pasos del gol).
**Cómo se consigue (0.4.45).** `muros_core`: en el gancho al paso analítico de la pelota (FUN_140876360, estado S: posición S[0..2], velocidad S[3..5]; la pelota del partido no es un cuerpo rígido y no colisiona con .geom) se refleja la componente de velocidad con restitución `muro_rebote` (0,6) y fricción tangencial `muro_friccion` (0,92) cuando |x| > semilargo+0,2 − r o |z| > semiancho+0,2 − r (r = 0,109 m); en la boca del arco (|z| < 4,2, y < 2,55) el límite es el fondo de la red (semilargo+3,4). Con la línea lógica de «fuera» en 15 × 10, la pelota entera no llega a pasarla (su centro queda en ≤ 10,09 y «fuera» exige > 10,11), así que no se declara fuera. `cancha_paredes: false` apaga los .geom de los muros (que no afectaban a la pelota y probablemente causaban el «trabarse»).
**Pendiente (frente G).** Asegurar que nada dispare un «fuera» o un saque al tocar el borde y anularlo solo en el entrenamiento con cancha chica (detector del job FUN_14050C3E0/C3C0/C490 y gestor de parada FUN_1404A56C0). Informe: mapa_fox\frente_G_saques_entreno.md.

## 17. Tamaño del arco (PENDIENTE, decidido por Fralex el 27-oct: no tocar por ahora)
**Observación.** Los arcos son reglamentarios de 11 contra 11 (media portería 3,66 → 7,32 m de ancho; travesaño 2,44 m) en una cancha lógica de 30 × 20 m: la boca ocupa más de un tercio del ancho (7,32 de 20 m), lo que favorece el gol fácil con 5 contra 5.
**Propuesta a probar primero:** 5,0 m de ancho × 2,2 m de alto (media portería 2,5; travesaño 2,2). **Alternativa:** 3 × 2 m (tamaño futsal; media portería 1,5; travesaño 2,0). Referencias de memoria (no verificadas): futsal 3 × 2 m en 40 × 20; FIFA Street con arcos de unos 3–5 m.
**Piezas a tocar (todas con interruptor y una sola prueba de tamaño antes de fijar):**
1. Boca lógica de gol: elementos [2] (media portería) y [3] (travesaño) del bloque de 9 floats de dimensiones ([raíz+0x2AA8], parcheado por `cancha_chica_free`).
2. Modelo dibujado del arco y la red (fmdl): escalar en ancho y alto con `mover_porteria_fmdl.py` (ya movido a ±15 y escalado en profundidad).
3. Postes y travesaño del Core (`muros_core`: 3,66 y 2,44 están fijos en el código; pasarlos a parámetros `arco_media` y `arco_alto`) y la boca de la malla (`malla_red_free`, `red_frena_free`: |z| < 3,66…).
4. Animaciones de parada del portero: su alcance es fijo en metros absolutos; con un arco más pequeño se verán generosas.
**Riesgo.** Si el arco dibujado y el lógico no coinciden hay goles «de mentira» o tiros por la boca que no cuentan.

## 18. Quién participa en el entrenamiento y por qué el portero solo entra con 11 (frente F, 27-oct)
`FUN_140A59CA0` (RVA 0xA59CA0, entrada `FUN_140A59EC0`) decide la participación: cada jugador tiene un rango (GK primero) y una prioridad leída de una de 3 tablas de 22 enteros en .rdata (T_ATAQUE 0x142644F10, T_DEFENSA 0x142644F80, T_ATAQUE_PORTEROS 0x142644FF0, ambos GK = 1); participa si prioridad ≤ N del lado (TW+0x3C / TW+0x40). La tabla se elige con TW+0x14 (modo) y TW+0x18 («Cursor de PT fijo»). En Ataque el GK propio vale 11: solo entra con N=11; con «Cursor de PT fijo = Sí» usa T_ATAQUE_PORTEROS. Core 0.4.49 `arquero_propio_free`: parche A (0x140A59DD5, `48 0F 45 CA` → NOP×4, por firma `80 7F 18 01 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 0F 45 CA`, offset 18): Ataque usa siempre T_ATAQUE_PORTEROS; parche B (dato 0x142644FAC, 11 → 1): GK rival dentro en Defensa. Solo con TrainingWork y restaurados al salir. Menú «S. pta./córner/banda» (M+0x3F31C → TW+0xC): solo 2 valores, ninguno quita los saques; el parche del Juez (`sin_saques_free`) sigue siendo necesario.

## 19. Modo street completo: resumen por logro, interruptores y cómo replicarlo (27-oct-2026)
Informes técnicos de cada frente (RVA, firmas AOB, confianza): `docs/libro-pes/frentes_re/frente_{B,C,D,E,F,G,I}_*.md` (copia en `para-proyecto/pes/frentes_re/`). Mapa de clases: `cap_74_mapa_clases_pes.md`. Todas las funciones del Core son interruptores del json (`phoenix_core.json`) y solo actúan con TrainingWork; los parches de código se localizan por firma AOB única y se restauran al salir.

| logro | interruptor | cómo funciona (resumen) | frente / sección |
|---|---|---|---|
| Gol nativo, celebración corta, saque de centro, sin repetición | `gol_nativo_free`, `saque_centro_free`, `gol_directo_free` | cola de pasos del gol reescrita (03 04 16 12 13 14 15 0E 34 19 1A) | §9–9b |
| Cancha lógica 30×20 | `cancha_chica_free` (`cancha_semilargo`, `cancha_semiancho`) | bloque de 9 floats de dimensiones y cajas de portería | §10 |
| Arcos y red visibles en ±15, césped | AddOn de Stadium Server | fmdl movidos/escalados; pitch variante AB | §11–12 |
| Temporada fija (vacío azul, césped claro) | `season_forzada` (Lua `set_conditions`) | season 1 en el entrenamiento | §15 |
| Cuadrado = tiro con potencia por carga en cualquier mitad | `siempre_disparo_free` + `siempre_disparo_modo` 8 | 2 parches de 1 byte (Shoot siempre aplicable, Clear nunca) en las unidades del mando | §14, frente D |
| Muros que rebotan (lados, fondos, postes, travesaño) | `muros_core` (`muro_rebote`, `muro_friccion`, `muro_x`, `muro_z`), `cancha_paredes: false` | gancho al paso analítico de la pelota FUN_140876360; la pelota no colisiona con .geom | §16, frente B |
| Red frena la pelota | `red_frena_free` (`red_factor`) | factor por paso en la red y rebote leve al fondo | frente B |
| Malla de la red se deforma | `malla_red_free` | GoalNet_Hit/GoalNet_SetState desde el gancho de la pelota | frente E |
| Sin saques (juego continuo) | `sin_saques_free` | 2 parches de 1 byte en el Juez (cc=2 puerta/córner, cc=3 banda) | §16, frente G |
| Suplentes ocultos | `suplentes_ocultos_free` | fila de banda FUN_140A597D0 → (0,0,100) | frente C |
| Jugadores dentro de la cancha | `jugadores_chica_free` + `jugadores_chica_modo` 1 | recorta las posiciones de reinicio (FUN_1409F5530) que caen fuera de ±(dim−0,5) | frente C |
| Portero propio con cualquier N | `arquero_propio_free` | cmovne de selección de tabla anulado + dato GK rival en Defensa | §18, frente F |
| IA sin despejes de emergencia | `sin_despeje_emergencia` | 1 byte en ThinkUnitClear de la IA (FUN_142109EB0) | frente I |
| Sondas de investigación | `jugadores_sonda`, `pelota_sonda`, `vigia_*` (apagar siempre: el vigía congeló el juego) | solo lectura / DR0-3 | cap_62 |

**Instalación (con backups):** `bash instalar_dll_solo.sh clave=valor …` (herramientas/ del repo): copia la DLL anterior, instala la nueva, añade claves al json, borra `arrancando.flag`. PES debe estar cerrado; el selector se abre aparte (`PES2021 Start.exe`); Frank pulsa Iniciar.
**Versiones:** 0.4.26 pelota_sonda; 0.4.31 SHOOT/CARGA; 0.4.36 MEDIDOR; 0.4.40 modo 8 (cuadrado); 0.4.41 red; 0.4.43 jugadores/suplentes; 0.4.45 muros_core; 0.4.46 malla; 0.4.47 sin_saques; 0.4.48 boca/postes; 0.4.49 arquero; 0.4.50 sin despeje IA.
**Lecciones:** ver cap_02 (herramienta mejor primero; x64dbg adjunto congela PES; la vigía de escritura congeló el juego en los goles).

## 20. Portero propio con cursor libre y portero que sale poco (27-oct-2026)
**Confirmado por Fralex (0.4.52):** con «Cursor de PT fijo = Sí» en el menú del entrenamiento y la función `arquero_propio_cursor` (3 parches por firma: B1 FUN_141FB9040 0x141FB9713 `0F 85`→`90 E9`, B2 FUN_14043EC70 0x14043EE2C `0F 84`→`90 E9`, B3 FUN_14087AC70 0x14087ADB7 `75`→`EB`; solo con TrainingWork) el portero propio participa con cualquier número de jugadores y el cursor queda libre («ya está fino»). La variante `arquero_propio_free` (tabla T_ATAQUE_PORTEROS sin el flag del menú) metía al portero pero quedaba pasivo, así que no se usa.
**Portero que sale poco (0.4.53):** `arquero_salida_corta` (`arquero_salida_max`, 3,5 m): gancho por firma a FUN_1409DC040 (ActionKeeperBasePosition, RVA 0x9DC040) que recorta la posición de salida a ≤ R m de la línea de gol; las salidas de 17–35 m del juego están en metros de cancha grande.
**Fallo y causa de los cierres con gol del rival (27-oct):** WER (ReportArchive, leído como archivo): PES2021.exe+0x476E1C, c0000005 = FUN_140476C50 (posición de reinicio por tipo de parada) con tipo inexistente → null. Causa más probable: `sin_saques_free` (parches del Juez que ponen cc=0). Sustituido por `muro_x`=15,0 / `muro_z`=10,0 (la pelota nunca llega a «fuera»).

## 21. Estado base estable del modo street: «street_estable_0454» (27-oct-2026, Fralex: «perfectísimo»)
Core 0.4.54 + Lua 0.6. Copia con nombre en `SiderAddons\phoenix_core\street_estable_0454\` (DLL, json, Lua, LEEME). **Cierre en el gol del rival resuelto con `reinicio_seguro`:** el fallo `PES2021.exe+0x476E1C` (c0000005; 4 informes WER el 27-oct) es FUN_140476C50 leyendo un byte de la búsqueda FUN_140A7CBA0 con clave nula; en la sesión de la 0.4.54 el log registró `[REINICIO] tipo=0x0 nulo, sustituido por la rama por defecto` dos veces (18:53:02 y 18:55:50) — es decir, dos cierres evitados — y el juego siguió. La clave nula (job+0x1F0 = 0) se produce al reiniciar tras ciertos goles del rival; la causa de fondo (quién deja el escenario a 0) sigue sin demostrar.
Flags del estado estable: `muros_core` (`muro_x` 15,0, `muro_z` 10,0), `cancha_paredes` false, `red_frena_free`, `sin_saques_free` false (los muros mantienen la pelota dentro de la línea de «fuera»), `malla_red_free` false (cerraba el juego con la pelota quieta en la red; rehacer), `arquero_propio_free` false, `arquero_propio_cursor` true (menú «Cursor de PT fijo = Sí»), `arquero_saque_corto`, `sin_despeje_emergencia`, `arquero_salida_corta` (`arquero_salida_max` 3,5 → 2,0 pedido), `reinicio_seguro`, `jugadores_chica_free` (modo 1), `suplentes_ocultos_free`, `siempre_disparo_modo` 8, `season_forzada` 1, vigías en 0.

## 22. Marcador y reloj nativos en el entrenamiento libre (28-oct-2026, Core 0.4.56 → 0.4.59) — LOGRADO

**Resultado (Fralex, con Core 0.4.59):** el marcador nativo de amistoso (equipo, escudos, goles) se ve en el entrenamiento, sube con los goles (1-0), el reloj corre (3:13 en pantalla) y el HUD sigue visible después del gol.

**Paso a paso (todo es json on/off, localizado por AOB, 1 byte, reversible; solo se puede replicar en un PES 2021 limpio con Phoenix Core):**
1. `marcador_nativo_free` — FUN_1420C9080 (RVA 0x20C93D7): `83 F8 0B 74 12 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 83 F8 0C 75 03 C6 07 00` byte +2: `0B`→`FF`. Evita que el HUD (match2D::Screen::Time, id 7) se ponga a cero para la clase de modo 0xB (entrenamiento libre, modo 52).
2. Pestillo del HUD (misma opción, 0.4.59) — RVA 0x20C90F4: `44 38 37 75 0F 45 38 B5 B2 11 00 00 45 0F B6 D1 45 0F 44 D6 0F B6 B4 24 B0 00 00 00` byte +3: `75`→`EB`. Sin él, al apagarse el HUD en la escena del gol con S+0x11B2==0 no volvía (en un amistoso lo reactiva el saque central normal; en el entrenamiento el saque lo lleva TrainingWork y ese byte queda en 0).
3. `reloj_street_free` — R1 (RVA 0x44CEE2, `74`→`EB`) y R1b (0x44CF02, `74`→`EB`) en FUN_14044CE80 (reloj por fotograma), con las firmas del frente J. El Core solo reinicia el reloj a 0 al llegar a `street_minutos` (json, defecto 5), así nunca se alcanzan los 45:00 del fin de tiempo.

**Errores e intentos fallidos:**
- 0.4.57/0.4.58: el Core sumaba 60·dt al float S+0x1174 desde un hilo. El log `[RELOJ]` mostró `transcurrido=0` durante minutos en `fase=1 sub=1`: el juego lo pisa cada fotograma; esa ruta NO funciona. Lección: medir con log antes de dar por buena una escritura de memoria; la ruta correcta salió del RE estático (R1/R1b).
- El HUD se perdía tras los goles; el RE estático (subagente) encontró el pestillo; con el parche (2) el log pasó a mostrar S+0x11B0=0x01010016 en juego.

**A tener en cuenta:** la duración sale fija (5 min) hasta habilitar «Duración del partido» del menú «Opciones gener.» (frente J2, pendiente); no hay pantalla de fin de tiempo (el reloj se reinicia); los parches de reloj/HUD se aplican siempre, pero fuera del entrenamiento esos chequeos no se cumplen. Informes: frentes_re/frente_J_marcador_reloj_nativo.md y frente_J3_hud_desaparece.md.

## 23. HUD forzado, reloj con duración del menú y menú «Opciones gener.» editable (28-oct-2026, Core 0.4.60) — LOGRADO

**Resultado (Fralex, 0.4.60):** «ya funciona todo bien el marcador y el contador por ahora». Respaldo: `SiderAddons\phoenix_core\street_estable_0460` (DLL + json + Lua).

**Paso a paso (todo json on/off, AOB, reversible):**
1. **HUD forzado** (`marcador_nativo_free`, parche A en FUN_1420C9080, solo con TrainingWork, restaurado al salir): firma `E8 ?? ?? ?? ?? 84 C0 74 0E 66 C7 07 00 01 C7 47 20 00 00 00 00 EB 29 80 3F 00 75 09 C7 47 20 00 00 00 00 EB 1B E8`; +7 `74 0E`→`EB 0E`; +23 `80 3F 00 75 09`→`C6 07 01 EB 09` (fuerza M+0x9C=1). El pestillo de 0.4.59 (RVA 0x20C90F4) arregló solo el primer caso: el log mostró S+0x11B2=1 en el segundo apagado.
2. **Menú editable** (`ajustes_partido_free`): FUN_140AA90A0 compara modo==0x34 y apaga la lista entera de «Opciones gener.»; NOP de 5 bytes en RVA 0xAA9261 (`40 88 7C 24 24`→`0F 1F 44 00 00`), firma `83 FD 34 75 09 40 88 7C 24 24 33 C9 EB ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 40 88 BB 87 01 00 00` offset 5.
3. **Duración** = byte de minutos en `*(*(exe+0x3705E10)+0x50)+0x35960+0x1C` (5-15, 20, 25, 30); el reloj reinicia al llegar a esos minutos en pantalla.

**Errores/lecciones:** (a) la lectura viva falló: en la escena del gol [raíz+0x270]=0, no hay estado legible; (b) un parche que arregla «a veces» no es la causa: medir con el log (S+0x11B0) antes de repetir; (c) el RE estático por subagente + AOB únicas resolvió lo que 2 iteraciones a ciegas no.
**A tener en cuenta:** la velocidad del reloj la fija el juego según la duración (no medida); el HUD ahora se ve también en celebración; sin pantalla de fin de tiempo. Informes: frente_J2_barra_ajustes.md, frente_J3_hud_desaparece.md.

## 24. Decisión de diseño: formatos de partido del modo street (28-oct-2026, Fralex) — PENDIENTE DE IMPLEMENTAR
**Pedido de Fralex:** cuatro formatos seleccionables, con el reloj SIEMPRE al ritmo nativo (45:00 de pantalla = la duración del menú; no se fuerza M=45):
1. `dos_tiempos`: medio tiempo a 45:00 (pausa corta, saque central del rival), final a 90:00 con resultado.
2. `un_tiempo`: final a 45:00.
3. `goles`: gana el primero en llegar a `street_goles` (por defecto 5); el reloj solo se muestra.
4. `gol_oro`: un tiempo; si hay empate a 45:00, el siguiente gol gana.
**Selector:** json `street_formato` = `"dos_tiempos"|"un_tiempo"|"goles"|"gol_oro"` + `street_goles`; más adelante un botón en Link o en el menú del juego.
**Orden de implementación:** maquinaria común (FINAL: congelar/mostrar resultado y luego revancha 0-0 con saque central; MEDIO TIEMPO propio sin la demo nativa que congelaba) → un_tiempo → gol_oro → goles → dos_tiempos.
**Estado base mientras tanto (Core 0.4.61):** `sin_fin_tiempo_free` (T1) deja pasar los 45:00 sin pitar; no hay fin de partido. Lo medido sobre el reloj de pantalla está en frentes_re/frente_J4_reloj_pantalla_real.md (base protegida + float; con M=5 la pantalla va 9× más rápido que el tiempo real, y eso es el ritmo nativo esperado).

## 25. El portero propio vuelve a ser portero: rama team+0x255 y bloqueo de cursor (28-oct-2026, Core 0.4.68) — LOGRADO
**Resultado (Fralex, 0.4.68):** «bien»: el portero se queda en su área y se puede seleccionar y controlar. Respaldo: `SiderAddons\phoenix_core\street_estable_0468`.
**Causa (dos cosas):**
1. `FUN_1409DC040` (posición base del portero) elige rama por `team+0x255`: 0 = rama normal `FUN_1409DE8C0`, ≠0 = rama alternativa `FUN_1409DF7D0` (~3 KB, portero «activo»). Log `[PORTERO_RAMA]`: bando del usuario = 1, rival (COM) = 0 → el rival se quedaba en su área y el nuestro salía. Arreglo `arquero_rama_normal` (RVA 0x9DC8CF, `74 13`→`EB 13`, firma `80 B8 55 02 00 00 00 74 13 48 8D 85 A0 00 00 00 48 89 44 24 20 E8 ?? ?? ?? ?? EB 05 E8`, byte +7), solo con TrainingWork.
2. Mi bloqueo del cursor de 0.4.64 (`cursor_bloqueo_pt`, ahora apagado por defecto) impedía seleccionar al portero: el gancho `FUN_140A571B0` con llamador `exe+0x4403D2` sirve tanto al cambio automático como al manual.
**Cómo se encontró:** `cursor_pt_log` (cueva solo lectura en `FUN_140A571B0`) mostró que el cursor sí iba al portero desde `0x4403D2` y `0x1FB98D2`, pero evitarlo no arregló nada; el análisis estático del frente L (§9) halló la rama por equipo y el log `[PORTERO_RAMA]` la confirmó.
**Errores:** la prueba con R1 apagado quedó confundida por el bloqueo del cursor (Fralex: «no me deja ni seleccionar»); lección: aislar UNA variable por prueba. También `arquero_salida_max` volvió a 3.5.

## 26. Regla de Fralex: la configuración del portero queda CONGELADA (28-oct-2026)
«Que no vuelva a descuadrarse la configuración del arquero.» Referencia = Core 0.4.68 (`street_estable_0468`, §25): arquero_rama_normal=true, cursor_bloqueo_pt=false, arquero_salida_max=3.5, arquero_propio_cursor=true (+ «Cursor de PT fijo = Sí» en el menú), arquero_salida_corta, arquero_saque_corto, sin_despeje_emergencia.
- Cada versión nueva: no tocar ninguna bandera ni parche del portero propio salvo pedido expreso; comparar el json del portero con street_estable_0468 antes de instalar y avisar si algo difiere; una sola variable por prueba.
- Frente M (portero que despeja tras el pase atrás, AMBOS bandos cuando lo maneja la IA): cambiar SOLO la decisión de despeje (pase corto a un compañero); no la posición ni el cursor. Si Fralex controla al portero, decide él.
- Frente N (formación inicial): excluir los slots de portero (0 y 11/21).

## 27. Estado visual y de juego confirmado por Fralex (28-oct-2026, Core 0.4.68–0.4.69)
Opinión de Fralex: «me gusta cómo se ve ahora, la cancha no sale oscura y a los lados no se ven reflectantes, se ven bien; mi arquero está bien, el gameplay está bien». Esto confirma los ajustes visuales del modo street: `season_forzada: 1` (césped claro, sin el vacío blanco/césped oscuro del verano) y la cancha chica con muros lógicos (`muros_core`, 15 x 10) sin vallas reflectantes. Pendientes: medio tiempo (0.4.69 por probar), formación inicial y pase corto del portero (0.4.71, sin instalar) y primera prueba por Parsec con amigos (PES corre solo en la PC de Fralex; Parsec solo transmite pantalla y mandos). Sin verificar: un segundo humano controlando al equipo rival en el Entrenamiento libre.

## 28. El arco con red que cede: ACEPTADO por Fralex (28-oct-2026, Core 0.4.74) — LOGRADO (parte lógica)
**Resultado (Fralex):** «ahora sí reacciona mejor el arco, por ahora me conformo con eso».
**Causa del arco «raro»** (frente O, RE estático): (1) la red nativa SÍ existe: cada fotograma una sonda (FUN_140456290 → FUN_1404AA790) pregunta si la pelota REAL está dentro del volumen de red y entonces llama a GoalNet_Hit (FUN_141EAE180) y a la física de red; pero el volumen se construye UNA vez al iniciar el partido y `cancha_chica_free` solo corrigió 9 de las dimensiones, así que seguía en ±52,5 y en street la sonda daba siempre «no» (sin golpe de tela ni física). (2) La red dibujada mide 3,0 m de fondo (goal_l_hit.geom: 52,5 línea; 53,97 aro intermedio; 55,445–55,505 fondo) y mi pared lógica estaba a +3,4 m: la «malla invisible» 0,4 m detrás del fondo dibujado. (3) La tela fallaba a 14 ms porque se llamaba desde el paso de la pelota (corre también en las 250 predicciones por fotograma y la IA, sin umbral de velocidad).
**Arreglo aplicado:** `red_resorte` (json; red_k=70, red_c=8, red_holgura=0,4 m): dentro de la boca del arco la pelota entra sin fuerza hasta la holgura y luego la red actúa como resorte + amortiguación (frena, rebota un poco, cae dentro); pared del fondo en línea + 3,05 m (la real). Postes y travesaño siguen con `muros_core`.
**Pendiente:** tela animada (ruta A del frente O: reemplazar el driver nativo FUN_1404560E0 por uno propio con la pelota real, un golpe por fotograma si v>1 m/s) y ruta B (enganchar la sonda para que devuelva «dentro» en street). Informe: frentes_re/frente_O_arco_nativo.md.

## 29. Medio tiempo NATIVO funcionando en el street (28-oct-2026, Core 0.4.86) — LOGRADO
**Resultado (Fralex, 0.4.86):** «ya perfecto, casi perfecto». A 45:00 sale el panel de estadísticas del descanso; X; menú nativo de descanso con «2.ª parte»; saque central del rival; marcador, formación, IA de cancha chica, portero y red OK; fondo azul (sin blanco). Respaldo: `SiderAddons\phoenix_core\street_estable_0486` (DLL + json + Lua).
**Cómo (paso a paso):** (1) el Core lee el reloj de pantalla (base decodificada + hash FNV-1a + float de tiempo muerto) y a t≥162000 (45:00) pide el descanso solo si han pasado >10 s del último gol (paso 0x04 de la cola); (2) con el job ocioso (rec==0) escribe fase=2, base del reloj a 0 y la cola [0x22,0x48,…]: el paso 0x22 (FUN_140449390) solo envía el mensaje 0x105001C que abre MatchStatsHalf; (3) el flujo se atascaba en MatchPreHalfTime porque su update (FUN_1413090F0) espera el byte [obj+0x290] (obj=*(u64*)([exe+0x3705E10]+0x58)) que pone a 1 el mensaje 0x105001B del paso 0x25 de la cola nativa: el gancho de PreHalfTime lo escribe y el flujo pasa a MatchHalfTime (menú nativo); (4) en ProcessMatchPostHalfTime::update (FUN_1412F7500) el Core marca «menú cerrado» y pone +0x89=1/+0x8A=0 para que emita «proceed» a MatchIdle; (5) el saque del 2.º tiempo lo da el «árbitro invisible»: se reproduce la preparación nativa del juez (FUN_14050A8A0 + FUN_14050B2B0 con un bloque persistente) y se llama a E770 (k=7) y se sustituye la cola nativa 0F… por la probada 03 16 12 13 14 15 0E 34 19 1A.
**Errores e intentos fallidos:** quitar fase=2 (0.4.78–0.4.81) hizo que el menú no saliera; detectar «menú cerrado» por pausa del job no sirvió (el job sigue ticando con el menú abierto); E770 original no hace nada con [juez+0x3B8]==0; el modo seguro (2 arranques seguidos <60 s) desactiva TODO y parece una regresión; la activación escalonada tardaba ~60 s (AOB lenta: arreglo con memchr en 0.4.87).
**Pendiente:** el reloj del 2.º tiempo arranca en 0:01 (se restaura la fase 1 y la base 0): corregir con fase 3 directa y base 0 (HUD 45:00-90:00), final a 90:00 de pantalla.

## 30. PENDIENTE: crash al cerrar PES en +0x3FD68B (28-oct-2026, 16:45, Core 0.4.93)
`PES2021.exe+0x3FD68B` (c0000005), ~6 s después de que el Core restaurara sus funciones (IA_DATOS, cancha_chica…) al cerrar el juego: es un crash AL SALIR, no durante el partido. Sesión con Core 0.4.93 (no 0.4.94, no reloj visual); sin desborde de tabla Lua en sider.log. Offset distinto de +0x476E1C, +0x1B5D052, +0x20EFFD8. Volcado: C:\Users\WinterOS\AppData\Local\CrashDumps\PES2021.exe.25444.dmp. Sin analizar.

## 31. Herramientas de análisis: proyecto Ghidra existente y Denuvo (29-oct-2026)
- **Proyecto Ghidra ya analizado:** `PEStrim` (Ghidra 12.1.4, 755 MB, creado el 15-oct) con el programa `/PES2021_recortado.exe` (exe recortado, sin la masa del protector): **281.513 símbolos**. Original: `D:\Frank\Games_\PRUEBA_CANCHA\copias\ghidra_proyecto_completo`. Copia de trabajo: `D:\Frank\IA\CLAUDE\ghidra_proyectos\PEStrim_copia`. Se consulta en solo lectura con `analyzeHeadless ... -process -noanalysis -readOnly -postScript` (scripts en `ghidra_proyectos\scripts`: `DecompilarLista.java`).
- **Por qué el análisis del exe completo nunca termina:** `PES2021.exe` (458 MB) tiene `.impdata` = 380 MB (RWX, entropía 6,7-7,0) = **Denuvo 17.0 [Steam]** (Detect It Easy 3.21); el código real está en `.trace` (37 MB, R-X). Ghidra intenta recorrer `.impdata` como código y se pierde (Constant Propagation; luego CreateFunctionCmd). Quitar el permiso de ejecución no basta; hay que eliminar el bloque (`QuitarImpdata.java`, solo en la copia de Ghidra). **No se descifra ni se desvirtualiza Denuvo** (Frank pedirá permiso a Konami si algún día hiciera falta).
- **Marcador (frente R, comprobado 29-oct):** el reinicio a 0 de los CAT_0..8 de los dos EQ (capturados por el gancho de `FUN_1408C9E90`, edx==0) SÍ funciona: tras reiniciar y marcar uno por equipo, CAT_0 vale 1 en cada EQ con hash válido. El HUD (`FUN_1420C5330` → `FUN_1420C9080`) copia `FUN_1408C9DF0(M+0x750D0+i*0x59650, 5)` a `gestor+0x18B6/7` solo si `S+0x116C != 2` (o `info+0x24 && info+0x28 == 8`); si `FUN_140A7F580` es true suma además un valor global. `phoenix_marcador_free.lua` lleva su propio contador que solo se reinicia en `set_teams`/`context_reset` (NO en la revancha propia): sigue acumulando (10-6 → 11-6 → 12-6 tras «partido nuevo»).

## 32. Bitácora del 29-oct-2026: Ghidra 12.0.4 + GhidraMCP, final a los 90, revancha, arquero y cierres (Core 0.4.94 → 0.4.99)
**Qué se consiguió**
1. **Exe mapeado y consultable desde Claude** (Ghidra 12.0.4 + GhidraMCP v0.2.2, puerto 8765, `claude mcp add -s user ghidra …`). Proyecto analizado `PES_MCP_RECORTADO` (975 MB) y proyecto previo `PEStrim` (12.1.4: 192.244 funciones, 91,4 % del `.trace` en funciones, 93,4 % desensamblado, `.trace` sha256 `cff7b2d9…9def` idéntico al exe actual; 88 de 89 RVAs del Core dentro de función). Guía de repetición: `D:\Frank\IA\CLAUDE\ghidra_proyectos\COMO_REPETIR.md` (copia en `docs/pes/`).
2. **Final a los 90' del street OK** con el panel propio (plantilla «Descanso» porque `final_nativo=false`; no es fallo). Con «Final nativo» sale «Final del partido» + Revancha nativa (0-0), pero esa revancha recrea el partido con reglas de partido normal (árbitro, faltas) y jugadores fuera de la cancha: por eso se deja apagado para jugar con amigos.
3. **Revancha propia:** contadores nativos reiniciados y verificados (`[GOLES] reinicio: 18 contadores a 0 en 2 bloques EQ` y `contadores EQ0=0 EQ1=0`; `match.stats()` = 0-0). El overlay (Lua `marcador_free`) se sincroniza ahora con `match.stats()` (commit 621a6d4). El HUD de PES con goles viejos NO está demostrado: los datos (EQ del HUD = EQ del gancho de goles, contadores a 0, stats 0-0) no lo respaldan; podría ser el refresco del HUD (`S+0x116C`) y falta una captura del HUD en ese momento.
4. **Arquero:** la fuente del despeje sigue desconocida. `FUN_142133950` corre SIEMPRE en modo 0 (`[GK_MODO]`: this+0x40=0, equipo+0x6D4=0, out[0]=0 en 54 lecturas), así que no es la causa en este partido; `GKClear` (FUN_142132B40, R = 12/15 m) y `GKPassLong` (FUN_142133120) ya están anulados con `C3`. Opción `arquero_controla` (apagada) lista, config del portero 0468 intacta.
5. **Cierres al salir:** `PES2021.exe+0x3FD68B` (28-oct 16:45, escritura en 0x31) y `+0x3FE920` (29-oct 01:35); volcados en CrashDumps; sin analizar.

**Cómo (método, tiempos, lecciones)**
- Denuvo 17.0 [Steam] (`.impdata` 380 MB RWX): el exe completo no se analiza; con el bloque eliminado en la copia de Ghidra, o con el exe recortado (60 MB), el análisis tardó ~85 min en 12.0.4. Medir CPU en dos instantes y `jstack` para saber si avanza.
- Plugin sin GUI: edición de `_code_browser.tcd` (respaldo `.pre-mcp`). Ghidra no es controlable por computer-use.
- Error de proceso que costó tiempo: scripts `.bat` generados con rutas con `\r`/`\a`; generar con `chr(92)` y CRLF.
- Core instalado y probado hoy: 0.4.97 ([GOLES], [LEE_STAT]), 0.4.98 (reinicio también de los EQ del HUD), 0.4.99 ([GK_MODO], `arquero_controla`).

## 33. Cierre al volver al menú principal (29-oct-2026, 13:33): diagnóstico y corrección 0.4.100
- **Volcado** `PES2021.exe.23696.dmp`: `RIP = PES2021.exe+0x902B2` (lectura en 0xC255D19D, puntero basura) dentro de `FUN_14009027a` (386 B: destructor que recorre una tabla de tablas y libera), llamado desde `FUN_14004A870` → `FUN_140042E70(&DAT_14372D588)` (limpieza global al destruir el partido). Pila: `+0x902B2 ← +0x4A886 ← +0x3FD80E ← … ← +0x3FB2F9`. `+0x3FD80E` está en `FUN_1403FD360`, la misma función que `+0x3FD68B` (28-oct); `+0x3FE920` (29-oct 01:35) está en la vecina `FUN_1403FE800`. Ningún marco es `phoenix_core.dll` ni `sider.dll`: familia de limpieza del juego (teardown).
- **Prueba circunstancial:** los tres cierres con volcado (28-oct 16:45:33, 29-oct 01:35:18 y 13:33:37) ocurren 5-7 s después de las 11 líneas `[IA_DATOS] … restaurado` (16:45:27, 01:35:11, 13:33:32). La restauración escribía floats en registros de IA del montón al salir; `escribirSeguro` solo atrapa memoria no mapeada, no un bloque liberado y reutilizado → corrupción del montón y lectura de basura en la limpieza posterior. Intermitente (otras salidas con la misma restauración fueron limpias: 20:20, 21:03, 21:28, 04:47). Causa PROBABLE, no demostrada.
- **Ganchos de 0.4.97-0.4.99 (LEE_STAT, GK_MODO, goles):** solo leen con `leerSeguro` o escriben únicamente si `arquero_controla` fuerza (apagado); no tocan memoria al salir.
- **Corrección 0.4.100** (commit 1da19f4, DLL MD5 58184cf8…): `restaurarGrupoIA` solo escribe si los campos siguen valiendo exactamente lo escrito por el Core (si no: «el registro ya no es el escalado: NO se escribe»); al salir de la zona de partido se olvidan `g_eqGol` y `g_leeStatVistos`. Pendiente de instalar y probar en partido.
- **Modo seguro (13:21):** abrir PES dos veces en menos de 60 s deja `arrancando.flag` y el Core arranca en MODO SEGURO (todas las funciones apagadas: menú de ajustes gris y equipo parado como en cancha grande). Se corrige abriendo PES otra vez y no cerrándolo en el primer minuto.
