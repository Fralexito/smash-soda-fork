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

## 5. El congelón al final de la celebración — NO era del juego
- Síntoma: unos 7 s tras el gol, cuando se juntan los 3 jugadores, PES se queda congelado (sin crash).
- Causa: un **error de Lua del parche Conmegol**, no del exe ni de Core. `SiderAddons/modules/TournamentCornerFlags.lua` usa la global `CornerFlag`, que solo se rellena para torneos conocidos. En el entrenamiento (torneo 29) queda `nil`, y `log("-- " .. CornerFlag)` falla dentro del callback de trofeo/entrada → el motor se queda esperando.
- Arreglo (solo Conmegol, copia en `.antes-fix`): línea 92 `if key and CornerFlag then`, línea 98 `log("-- " .. tostring(CornerFlag))`.
- **En PES limpio este archivo no existe → no hace falta.** En el parche propio: **todo módulo Lua que use datos del torneo debe aguantar `nil`** (el entrenamiento no tiene torneo).
- Cómo se encontró: comparar `sider.log` de un gol en amistoso (no se congela, sale «Commentary file detected») contra uno en entrenamiento (salía `lua ERROR … CornerFlag` y nunca llegaba el comentario).

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
