# Diseño — pantalla propia de Phoenix dibujada por Phoenix Core (escalón 3a)

Fecha: 10-nov-2026. Solo estudio de lectura: no se descargó nada, no se instaló Core 0.5.65/0.5.66, no se abrió PES para este documento.
Base de partida: el botón «Phoenix» (escalón 1) ya funciona en Conmegol: ver `RESULTADO_E1_BOTON_PHOENIX.md`.

## 0. Resumen
1. **SÍ:** el Core ya tiene casi todo para pintar sobre el juego sin ImGui ni MinHook: engancha `IDXGISwapChain::Present` por la vtable (índice 8, encadenado con el gancho de Sider), compila shaders con `d3dcompiler_47.dll` (DLL de Windows), guarda/restaura todo el estado del contexto DX11 y dibuja un triángulo de pantalla completa. Está en `C:\dev\phoenix-core-wt\paquetes\phoenix_core\src\nitidez.h` (524 líneas, probado en juego en las 0.5.x).
2. **Falta:** (a) pipeline de rectángulos y texto; (b) una fuente (se puede hornear un atlas con GDI desde una fuente de Windows, sin dependencia); (c) «estado de pantalla» (abierta/cerrada, ítem); (d) entradas; (e) un nodo de flujo del juego donde aparcar al jugador mientras nuestra pantalla está abierta.
3. **Detección:** `zonaId()` ya existe en Core (`[exe+0x3704E38]` → `u32 [gestor+0xF0]`, con firma del exe). Zonas vistas: 7 TOP_MENU, 49 ONLINE_SETTINGS (sale al pulsar nuestro botón, evento 20), 25 ONLINE_QUICK, 52 FREE_TRAINING, 8 EXHIBITION, 5 TITLE_DEMO_LOOP, 4 BOOT_STEP.
4. **Flujo recomendado:** evento 20 → viaje de ida y vuelta por un nodo `process` (`TopMenu/TopMenu`) y Core, al ver el flanco a zona 49, dibuja su pantalla encima del menú principal. «Atrás» solo la cierra. Es la parte más incierta (§3).
5. **Entradas:** teclado `GetAsyncKeyState` (ya usado), mando XInput (`xinput1_4.dll`, de Windows), ratón `GetCursorPos`. Falta resolver cómo evitar que el juego reciba también las pulsaciones.
6. **Alternativa mínima (sin tocar Core):** overlay de texto de Sider (`overlay_on` + `key_down`/`gamepad_input` + `input.set_blocked`), pero choca con `CGP_SB_Addons.lua` de Conmegol, que ya controla el overlay. Sirve de prototipo.
7. **Dependencia a pedirle a Frank:** **ninguna** para la fase 3a. Dear ImGui (MIT) solo si algún día se quiere interfaz rica; requeriría su «SÍ» explícito a descargar. MinHook no hace falta.
8. **Riesgos principales:** «Atrás» sin salida, orden de ganchos de `Present`, formato del backbuffer, y que el juego reciba las pulsaciones a la vez.
9. **Orden de pruebas:** (1) ida y vuelta sin dibujar; (2) rectángulo fijo; (3) texto con atlas GDI; (4) teclado; (5) mando; (6) acción real; (7) bloqueo de entrada; (8) estrés.
10. Apagado por defecto, con interruptor `PHOENIX_CORE_OFF`; todo fallo apaga la función, nunca tumba PES.

## 1. Qué tiene hoy Phoenix Core (rama `phoenix-core-nitidez`, commit `483e998`, Core 0.5.66; el instalado en el PES base es 0.5.64b)

| Pieza | ¿Existe? | Dónde / cómo |
|---|---|---|
| Hook de `Present` | **Sí** | `src/nitidez.h`: vtable de `IDXGISwapChain`, índice 8, por `VirtualProtect`; guarda lo que había (el gancho de Sider) como «siguiente» y se pone delante; hilo con reintentos; se apaga solo si falla |
| Detours propios | **Sí, a mano** | `phoenix_core.cpp`: cuevas a <2 GB (`VirtualAlloc` cerca), `jmp rel32` (0xE9), `FlushInstructionCache`, `escribirRel32`, `leerSeguro`. No usa MinHook |
| Compilar shaders | **Sí** | `LoadLibraryA("d3dcompiler_47.dll")` + `D3DCompile`; si no está, se apaga solo |
| Dibujo sobre el backbuffer | **Sí (pase de pantalla completa)** | `nit::aplicar`/`pasada`: copia el backbuffer, dibuja un triángulo con el pixel shader CAS y **guarda y restaura** IA, VS/PS/GS/HS/DS, SRV, CB, RTV/DSV, blend, depth-stencil, rasterizer y viewports |
| Texto / fuente | **No** | Solo captura a BMP para pruebas |
| Rectángulos con alpha | **No (trivial)** | Ya se crea un `ID3D11BlendState` |
| Teclas | **Sí** | `GetAsyncKeyState` (Ctrl+F9…F12), con detección de flanco |
| Leer la zona | **Sí** | `zonaId()` (caché 100 ms), `enExhib()`, `zonaFirmaOk()` |
| Cargador | **Sí** | `modules/phoenix_core_loader.lua` (Sider) → `LoadLibraryA` por ffi, llama `phx_core_tick` cada fotograma |
| Config / estado | **Sí** | `phoenix_core.json`, `phoenix_core_estado.json`, `phoenix_core.log` |
| ImGui / MinHook | **No** | Nada de terceros enlazado |

Conclusión: la pantalla propia es **un archivo nuevo `src/pantalla.h`**, incluido como `nitidez.h` (dentro del espacio de nombres anónimo de `phoenix_core.cpp`), y una función nueva `pantalla_phoenix_activa` (por defecto `false`) en `phoenix_core.json`.

## 2. Detectar «Ajustes → Phoenix» y cuándo mostrar la pantalla
Hechos (10-nov, Conmegol):
- La lista de **Ajustes** (pestaña superior, 3ª) son eventos de `TopMenuNew.json`: Editar (`17`), Aplicar actualización (`23`), «Opciones en línea» (`20`, hoy rotulada «Phoenix»), Opciones del sistema (`21`), Opciones (`22`).
- Al pulsar el ítem 20 la sonda pasa a **zona 49**. La zona la fija el **id del evento**, no la pantalla destino (con destino Entrenamiento libre seguía diciendo 49).
- En zona 7 no se sabe qué ítem tiene el foco (el cursor de la lista no está localizado en memoria).

Diseño:
1. **Disparador = flanco de subida a zona 49.** No hace falta saber el foco: el juego avisa cuando se pulsa «Phoenix». Para evitar falsos positivos, exigir que la zona anterior fuera 7.
2. Core mantiene `g_pant = {cerrada, abierta}` y el ítem elegido.
3. El evento 20 apunta a un nodo inerte (§3).
4. Al cerrar (Atrás/Esc/B) pasa a `cerrada`.
5. Con firma de exe distinta (`zonaFirmaOk()==false`) la pantalla no se activa.

## 3. Dónde «aparcar» al juego y cómo volver (lo más incierto)
Datos:
- Un nodo `process` **sin salida** deja una pantalla azul vacía que **no responde a Esc** (visto con ONLINE_QUICK; hubo que cerrar PES). Sirve de lienzo pero no tiene «atrás».
- Los nodos de lista sí atan `return` al botón Atrás del juego.
- `TopMenu/TopMenu` es un nodo `process` que vuelve a `TopMenu/TopMenuNew`; Conmegol ya apunta ahí huecos muertos (09, 10, 13, 14, 19, 26–30, 32) sin problemas vistos.
- `Match/TrainingMatchInit` cierra PES sin datos previos; `Match/MatchTrainingTeamSel` funciona.

Opciones:
- **A. Ida y vuelta (recomendada para empezar).** Evento 20 → `TopMenu/TopMenu` (o nodo propio `TopMenu/TopMenuPhoenix`, `proceed → TopMenu/TopMenuNew`). Pulsar «Phoenix» hace un parpadeo y el juego vuelve solo al menú; Core ya abrió su pantalla encima. «Atrás» solo la cierra. Sin riesgo de quedar atrapado. Inconveniente: las pulsaciones llegan al menú de debajo (§4).
- **B. Nodo de lista propio con ítem inerte.** Más pulido, más difícil de aislar.
- **C. Nodo propio con eventos por ítem** (`00_entrenamiento → Match/TrainingDirectInit`…) y Core solo rotula: ruta del escalón 4.
- **D. Emular Atrás desde Core** (inyectar la pulsación): frágil; solo si A y B fallan.

## 4. Entradas
- **Teclado:** `GetAsyncKeyState` con PES al frente. Flechas, Enter, Esc/Retroceso.
- **Mando:** `XInputGetState` (`xinput1_4.dll`, de Windows, solo lectura). Con mandos virtuales (Link/ViGEm, hasta 16) leer solo el índice configurado y con antirrebote. PES también lee DirectInput.
- **Ratón:** `GetCursorPos` + coordenadas de la ventana (opcional).
- **Que el juego no reciba la pulsación:** Sider tiene `input.set_blocked(true)` y, en módulos Lua, `key_down` y `gamepad_input`, **pero** esos eventos solo salen cuando el overlay está encendido y el módulo tiene el control del overlay; hoy lo tiene `CGP_SB_Addons.lua`. Salidas, de menos a más riesgo: (1) con la variante A, aceptar que flechas solo mueven el foco del menú y **bloquear/ignorar solo Enter**; (2) llamar a `input.set_blocked(true)` desde `phoenix_core_loader.lua` mientras `g_pant==abierta` (flag expuesto por ffi); (3) detour propio a la lectura de entrada del juego (investigación nueva).
- **Mapa:** Arriba/Abajo = elegir, Enter/A = aceptar, Esc/B = atrás, Izq/Der = cambiar valor.

## 5. Dibujo sin ImGui (recomendado)
1. **Dónde:** en el mismo gancho de `Present`, tras nitidez y antes de llamar a Sider (el overlay de Sider queda encima como ahora), con el bloque de guardado/restauración de `nit::aplicar`.
2. **Rectángulos:** vertex shader que genera el quad desde `SV_VertexID` y una constante `{x,y,w,h,r,g,b,a}`; blend alfa estándar.
3. **Texto sin librerías:** atlas hecho con GDI al iniciar (`CreateFontW` con una fuente de Windows → DIB 8 bits → textura `R8_UNORM` ~512×512 con ASCII + á é í ó ú ñ ü ¿ ¡; anchos por carácter). Un quad por carácter con el mismo shader.
4. **Escala:** por `GetDesc()` del backbuffer (se vio 1280×720 y 1920×1080).
5. **Estilo:** réplica básica de los menús de PES (fondo azul oscuro semitransparente, fila blanca resaltada con triángulo a la derecha). Regla «Menús de PES: réplica exacta»: usar colores medidos de capturas.
6. **Límites del atlas GDI:** sin suavizado de subpíxel ni fuentes de Konami; suficiente para 3a.
- **ImGui** (MIT, ~1 MB + backend DX11) daría ventanas, scroll, deslizadores, pero: descarga (necesita «SÍ» de Frank), enlazarlo en CMake y gestionar igualmente estado DX11 e input. No compensa para 5–10 ítems.

## 6. Arquitectura (`src/pantalla.h`)
- `pant::iniciar(sc)`: crea VS/PS, atlas, buffer constante y blend en el hilo del `Present` (como `nit::iniciarRecursos`).
- `pant::tick()`: lee `zonaId()`; flanco 7→49 = abrir; entradas; navegación; Esc = cerrar.
- `pant::dibujar(ctx, bb)`: guarda estado → rectángulos y texto → restaura.
- Ítems en el json (`pantalla_items`: texto + acción). Iniciales: «Entrenamiento libre» (hoy lo hace el flujo; pasaría a ser ítem) y «Cerrar».
- Una sola entrada en la vtable: dentro, primero nitidez y luego pantalla.
- Registro `[pantalla]` en `phoenix_core.log` y estado en `phoenix_core_estado.json`.
- Paquete suelto y on/off; funciona en un PES 2021 limpio; con `PHOENIX_CORE_OFF` no hace nada.

## 7. Riesgos
| Riesgo | Efecto | Mitigación |
|---|---|---|
| Nodo de aparcado sin «atrás» (B/C) | Juego atrapado | Empezar por A |
| Enter llega al menú de debajo | Se activa un mosaico | `set_blocked` o ignorar Enter; probar con el foco en una pestaña inofensiva |
| Orden de ganchos de `Present` (Sider/ReShade/Core) | Pantalla duplicada o ninguna | Reusar el encadenado de nitidez; log; apagado automático |
| Backbuffer en formato no soportado | Sin dibujo | `formatoValido` (R8G8B8A8/B8G8R8A8/R10G10B10A2, sin MSAA) |
| Estado DX11 sin restaurar | Gráficos rotos | Copiar el bloque probado de `nit::aplicar` |
| Excepción al dibujar | Cierre de PES | `__try/__except` + apagado automático |
| Carreras entre hilos | Cuelgues | Recursos solo en el hilo de `Present`; `volatile LONG`/`Interlocked` |
| Mandos virtuales leídos por error | La pantalla se mueve con el mando de otro | Un solo índice; ignorar durante partido |
| Zona 49 = «ONLINE_SETTINGS» en un PES con online real | Choque con lo online | Solo en Conmegol/offline y con la función encendida |
| Falsos positivos si otro código dispara el evento 20 | Pantalla inesperada | Exigir zona previa 7 |
| Acentos mal en el atlas | Texto ilegible | Probar «ñ á é í ó ú ü ¿ ¡» |

## 8. Orden de pruebas (sin online; PES cerrado entre cambios; backups antes)
0. Backup de `sider.ini` y `phoenix_core.json`.
1. **Solo JSON:** evento 20 → `TopMenu/TopMenu`; comprobar parpadeo y vuelta al menú sin cierres; ver zona 49 en la sonda.
2. **Core 0.5.67 en una copia de prueba:** `pantalla_phoenix_activa=true`; rectángulo fijo semitransparente en zona 49; Esc lo cierra.
3. Texto con atlas GDI («Phoenix» con acentos).
4. Lista de 3 ítems navegable con teclado y resaltado.
5. Mando (XInput, un índice).
6. Acción real: «Entrenamiento libre».
7. Bloqueo de entrada y Enter seguro.
8. Estrés: abrir/cerrar 20 veces, alt-tab, pantalla completa/ventana, 1280×720 y 1920×1080.
Entre pasos: revisar `phoenix_core.log`, `sider.log` y `phoenix_core_estado.json`. Si falla: restaurar `sider.ini` y `phoenix_core.json`.

## 9. Dependencias
| Dependencia | ¿Hace falta? | Origen | ¿Pedir permiso? |
|---|---|---|---|
| `d3dcompiler_47.dll` | Sí (ya la usa Core) | Windows | No |
| `xinput1_4.dll` | Para mando | Windows | No |
| GDI / fuentes de Windows | Para texto | Windows | No |
| Dear ImGui (MIT, github.com/ocornut/imgui, ~1 MB) | **No** (opcional futuro) | Descarga | **Sí, «SÍ» explícito de Frank** |
| MinHook | **No** | — | — |

→ Para la fase 3a **no hay que pedirle nada a Frank**.

## 10. Alternativa de menor esfuerzo: overlay de Sider (sin Core)
`overlay_on(ctx)` devuelve texto, imagen y opciones (colores, «top»/«bottom»); `key_down`/`gamepad_input` + `input.set_blocked` dan entrada y bloqueo. Una lista de texto en 1–2 horas, **pero** (a) solo un módulo controla el overlay y hoy es `CGP_SB_Addons.lua`; (b) solo texto monoespaciado en una franja, sin rectángulos; (c) ya es el que se ve arriba. Útil como prototipo de la navegación, no como pantalla final.

## 11. Hallazgos para el catálogo (cap. 62)
- Core ya engancha `Present` (vtable, índice 8) y se encadena con Sider: `nitidez.h`.
- La zona del juego la decide el id del **evento** del flujo (zona 49 con el evento 20), no la pantalla destino.
- Los huecos 13/14 de `TopMenuNew.json` no son botones visibles; los ítems de **Ajustes** son los eventos 17, 23, 20, 21, 22.
- `Match/TrainingMatchInit` cierra PES sin datos previos; `Match/MatchTrainingTeamSel` funciona como destino directo.
- Un nodo `process` sin salida deja una pantalla azul vacía que no responde a Esc.
- Los textos del menú están en `spa/string/all.str` (cabecera + zlib en el CPK; sin comprimir en `livecpk\Comentarios`); se reescriben **en sitio** manteniendo longitud.
- El primer `cpk.root` del `sider.ini` gana.

## 12. Siguiente paso sugerido
Pruebas 1 y 2: ida y vuelta por JSON y rectángulo fijo. Para la 2, compilar Core 0.5.67 en la rama `phoenix-core-nitidez` con `cmake --build` (nunca con `COMPILAR_PHOENIX.bat`, que es de Link) y probarlo primero en una copia, no en el Conmegol de Frank.

## 13. Estado de la prueba 2 (10-nov, ~22:30) — preparada y compilada; falta probarla dentro de PES
- **Decisión de diseño:** en vez de meter la pantalla en `phoenix_core.dll` (el Conmegol de Frank corre Core 0.5.58; el código fuente disponible es la 0.5.66 y no se debe instalar 0.5.65/0.5.66), la pantalla salió como **DLL independiente `phoenix_pantalla.dll`** + módulo de Sider propio (`phoenix_pantalla_loader.lua`), con on/off (línea del `sider.ini` o archivo `PHOENIX_PANTALLA_OFF`). Core queda intacto. La lógica (`src/pantalla.h`) es la misma que habría ido en Core, así que pasarla a Core más adelante es incluir el archivo.
- **Qué incluye:** `src/pantalla.h` (HLSL propio, atlas de fuente GDI, guardado/restauración de estado DX11 incl. constantes del VS y sampler, apertura por flanco de zona 7→49, F7 abre/cierra a mano, F8 cierra, cierre automático a los 20 s), `src/pantalla_dll.cpp` (lectura de zona con firma del exe, gancho de `Present` por la vtable, API `phx_pantalla_iniciar`), cargador Lua y arnés `herramientas/pantalla_arnes`.
- **Probado fuera de PES (arnés con ventana + swapchain propios):** la DLL carga, engancha `Present`, compila shaders, hornea el atlas y dibuja velo + panel + título + texto con acentos (áéíóú ñ ü ¿¡) correctamente; el viewport y el RTV del «juego» se restauran. Captura: `phoenix_pantalla_0.1.0\prueba_arnes_fuera_de_PES.png`.
- **Detalle por pulir:** puntitos sueltos bajo algunas letras (sangrado entre celdas del atlas; ampliar celda o recortar).
- **No hecho aún (según el orden de §8):** bloqueo de entrada, mando, acciones reales, y la prueba dentro de PES (instalar con el OK de Frank, PES cerrado, backup del `sider.ini`).
- **Paquete:** `D:\Frank\IA\CLAUDE\pes-menus\phoenix_pantalla_0.1.0\` (LEEME con instalar/probar/apagar). Código: rama `claude/pantallas-propias-menus` (`parche-v01/menu-phoenix-e1/pantalla/`) y worktree `C:\dev\phoenix-pantalla-wt`, rama `claude/core-pantalla` (commit `eb7925b`).

## 14. Primera prueba en PES: FALLÓ (10-nov, 22:36) — causa probable y arreglo (v0.1.1 de la DLL, aún sin probar en PES)
**Qué pasó.** Con `phoenix_pantalla_loader.lua` v0.1.1 y la DLL 0.1.0 en Conmegol: Sider cargó el módulo sin error (el primer intento había fallado por `pcall` inexistente en Sider; corregido), la DLL arrancó y enganchó `Present` a las 22:36:20.494 y **PES se cerró 1,5 s después**: Windows (evento 1000) registró `d3d11.dll+0x3850d` (c0000005) a las 22:36:22.006 y `PES2021.exe+0x4114f0` (c0000005) a las 22:36:24.629. Esos dos desplazamientos no habían aparecido en los cierres de los 4 días anteriores (que eran de otros: `PES2021.exe+0x3fe2cd`, `+0x1b5d052`, `+0x3db255`, `phoenix_core.dll+0x579c4`…). Sin partido y sin dibujar nada (el panel estaba cerrado): solo el gancho de `Present` y el dispositivo de prueba.
**Causa más probable.** La carpeta raíz de Conmegol trae su propio `d3d11.dll` (1,9 MB, del 6-nov): **es ReShade como proxy** (`ReShade.log1/2`: «Redirecting CreateDXGIFactory1/2…»). El método de Core (`nitidez.h`) y mi DLL buscan la vtable de `IDXGISwapChain` con `GetModuleHandle("d3d11.dll")` + `D3D11CreateDeviceAndSwapChain` sobre una ventana de 64×64; en Conmegol eso pasa por el proxy de ReShade, que crea su «runtime» para esa swapchain de mentira; al destruirla, el estado de ReShade/Sider queda colgando y d3d11 falla. En el arnés (sin ReShade) funcionaba. **Nota para Core:** el filtro de nitidez de Core usa el mismo método y estaba apagado en Conmegol, por lo que nunca se había ejercitado con ReShade: no activarlo allí sin esta corrección.
**Arreglo en v0.1.1 (probado solo en el arnés):**
1. Crear el dispositivo de prueba con el **`d3d11.dll` del sistema por ruta completa** (`System32\d3d11.dll`), no con el del juego; si hay proxy, lo anota en el log.
2. **No enganchar hasta que el juego esté estable:** zona 7 (menú) mantenida 8 s y 25 s desde el arranque del módulo; si no llega en 5 min, no engancha.
3. **Candado anti-cierres:** antes de enganchar crea `phoenix_pantalla\ENGANCHE_EN_CURSO`; lo borra a los 60 s con el gancho puesto. Si PES se cierra antes, el siguiente arranque NO engancha (aparece «CANDADO» en el log) hasta que alguien borre ese archivo. Un fallo cuesta como mucho un cierre.
4. Interruptor `ARNES` (archivo en la carpeta) solo para pruebas fuera de PES.
**Arnés:** pasa (gancho, dibujo, restauración de estado, candado que impide un segundo enganche). **Pendiente:** prueba en PES con OK de Frank; si vuelve a fallar con el dispositivo del sistema, alternativas: (a) tomar el `IDXGISwapChain*` real del juego (escaneo, o gancho de `CreateSwapChain` antes de que el juego lo cree), (b) ventana superpuesta transparente (no sirve en pantalla completa exclusiva), (c) overlay de Sider.
**Estado:** línea `phoenix_pantalla_loader` COMENTADA (`;`) en el `sider.ini` de Conmegol; un `sider.exe` (PID 22220) quedó colgado tras el cierre.

## 15. Segundo fallo en PES y cambio de enfoque: pantalla SIN enganchar Present (11-nov, 00:50)
### 15.1 Fallo 2 (v0.1.1, Conmegol, 11-nov 00:49)
- Con el `d3d11.dll` del sistema, esperando 73 s en el menú (zona 7) y con el candado puesto: `Present enganchado` a las **00:49:42.202**, y PES se cerró a las **00:49:43.129** con **`sider.dll` 7.3.4.0 c0000005**, y otro evento **c000041d** (excepción no controlada en un callback) a las 00:49:48.817. El fallo ya no está en `d3d11.dll` sino en `sider.dll`, 0,9 s después de tocar la vtable.
- Lectura: **no era solo el proxy de ReShade.** En Conmegol conviven tres actores sobre `IDXGISwapChain`: ReShade (proxy `d3d11.dll`), Sider (parchea `Present` y rastrea la swapchain/overlay) y nosotros. Crear una swapchain de mentira y/o parchear `vtable[8]` por encima de Sider desestabiliza a Sider. El candado funcionó (en el siguiente arranque no volvió a enganchar).
- Efecto colateral: tras el cierre quedó **un `sider.exe` huérfano** (PID 29276) que, al arrancar otro PES (el base del hilo online, `C:\PES21`, 00:50:12), le **inyectó el `sider.dll` de Conmegol**. Lección: tras un cierre de PES con Sider, comprobar que no queda `sider.exe` antes de abrir NINGÚN otro PES; y ya hay varios `sider.exe` posibles (uno por instalación).
- Conclusión: **dejar de enganchar `Present` en Conmegol.** El método (dispositivo de prueba + vtable) queda como válido solo para un PES sin ReShade/Sider activos sobre DXGI, y el filtro de nitidez de Core tiene el mismo riesgo en Conmegol.

### 15.2 Alternativas, ordenadas de MENOS a MÁS riesgo con ReShade + Sider en Conmegol
| # | Enfoque | Qué es | Riesgo | Alcance | Notas |
|---|---|---|---|---|---|
| 1 | **Pantalla NATIVA reutilizada (solo JSON + `all.str`)** | «Phoenix» (evento 20) lleva a una pantalla de lista que Konami ya programó y que Conmegol no usa; se re-cablean sus ítems a destinos reales y se rotulan con `all.str` en sitio. Candidatas: `TopMenu/TopMenuOptionSub` (10 ítems: 00_personaldata, 02_soundtrack, 03_resultcom, 04_credit, 06_versionInfo, 07_saveSystemdata, 08…, 09…; return → TopMenu/TopMenu) o `TopMenu/TopMenuOnlineSettingSub` (5 ítems 0_profile…4_konamiid, return → Online/Setting/…; **evitar**: destinos online). | **Muy bajo** (cero código inyectado; mismo mecanismo ya probado: re-cablear eventos y rotular textos) | Lista navegable real, con el aspecto EXACTO del juego, teclado y mando nativos, «Atrás» nativo. Ítems → pantallas que ya existen (Entrenamiento libre `Match/TrainingUgcCheckJoinMode`, selección de equipos `Match/MatchTrainingTeamSel`, créditos `Option/OpCredit`, info de versión `Option/OpVersionInformation`…) | No hay texto libre ni datos en vivo; nº de ítems y orden los fija el dibujo `.bin`; las pantallas son compartidas por nombre (si Conmegol ya usa `TopMenuOptionSub` en el 23 «Aplicar actualización», ese ítem pasaría a ser Phoenix: aceptable) |
| 2 | **Pantallas nativas de solo texto** | `Option/OpVersionInformation` y `Option/OpCredit` muestran texto (de `all.str`/`staff_list.str`): «Acerca de Phoenix», créditos, versiones | Muy bajo | Información estática | Se puede rotular desde el ítem de §1 |
| 3 | **Sider `livecpk_rewrite` dinámico (Lua)** | El módulo Lua decide, en cada petición de archivo, qué archivo sirve (ya lo hacen `MenuServer.lua` y otros de Conmegol para fondos/menús). Permite servir un `.str` o `.bin` distinto según estado (p. ej. texto de estado) | Bajo/medio (API propia de Sider, sin ganchos nuestros) | Contenido semi-dinámico, aún con aspecto nativo | Hay que ver si el juego vuelve a pedir el archivo cada vez que abre la pantalla |
| 4 | **Overlay de Sider (texto)** | `overlay_on(ctx)` devuelve texto/imagen/colores/posición; `key_down`/`gamepad_input`/`input.set_blocked` para entrada | Bajo/medio (API propia de Sider; ya está en uso por `CGP_SB_Addons.lua`) | Panel de texto monoespaciado arriba/abajo, con imagen opcional (PNG) a la derecha | Solo un módulo controla el overlay a la vez (se cambia con `1` / `` ` `` y se activa con Espacio): no aparece solo; no es réplica del juego |
| 5 | Escribir en memoria el texto/estado de una pantalla nativa (Lua `memory.write`) | Cambiar cadenas ya cargadas en la pantalla abierta | Medio/alto (direcciones por descubrir; posible cierre) | Texto en vivo | Solo con REA/phoenix-pes y en copia |
| 6 | Tomar el `IDXGISwapChain*` real y dibujar | Obtener el puntero del juego (escaneo de memoria o gancho propio de `CreateSwapChain` antes de que el juego lo cree) | **Alto** (ReShade+Sider ya hookean ahí; crasheó 2 veces) | Dibujo libre | Solo si 1–4 no bastan |
| 7 | Ventana superpuesta transparente (Win32) | Ventana encima de PES | Medio (no sirve en pantalla completa exclusiva; roba el foco) | Dibujo libre en ventana/borderless | No recomendada |

### 15.3 Recomendación y orden de pruebas (sin Present)
1. **Prueba A (solo JSON):** `20_onlinesettings` → `TopMenu/TopMenuOptionSub` (copia propia en `Phoenix-Menu`, con `return` → `TopMenu/TopMenu`; ítems por defecto de Konami). Comprobar que la lista se abre, se navega y «Atrás» vuelve al menú, sin cierres (zona esperada en la sonda: 7 → 49 → …). Dejar los ítems peligrosos de Konami (borrar/guardar datos `07/08/09`) **re-cableados a `TopMenu/TopMenu`** ANTES de la primera prueba.
2. **Prueba B (`all.str` en sitio):** rotular 4–6 ítems («Entrenamiento libre», «Estado de Phoenix», «Acerca de»…), misma longitud, relleno con ceros.
3. **Prueba C:** cablear cada ítem a su destino real (`Match/TrainingUgcCheckJoinMode`, `Option/OpCredit`, `Option/OpVersionInformation`…).
4. Si se quiere texto dinámico: **Prueba D** con `livecpk_rewrite` (§3) sobre el `.str` o con el overlay de Sider (§4) como panel de estado manual.
5. Solo si todo lo anterior se queda corto: §6 con una investigación previa (cómo hace Sider su gancho) y en copia.
Reglas: siempre con candado/backup de `sider.ini`; comprobar antes que no hay ningún `sider.exe`/PES abierto; un solo PES a la vez (coordinar con Online en TRASPASO.md).
