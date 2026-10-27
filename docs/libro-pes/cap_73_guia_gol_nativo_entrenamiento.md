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
