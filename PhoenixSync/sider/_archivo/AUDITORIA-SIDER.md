# Auditoría de Sider en la PC de Fralex (8 oct 2026)

Antes de instalar cualquier módulo Phoenix, se leyó **cada módulo activo** de Sider: qué hace, qué memoria del juego toca, qué archivos
cambia, qué teclas usa y con quién choca. Detalle completo (módulo por módulo, con direcciones y bytes) en `auditoria/`:
`hallazgos_A_shirtless.md`, `hallazgos_B_memoria.md`, `hallazgos_C_camaras.md`, `hallazgos_D_archivos.md` e `inventario-modulos.json`.

**Límite importante:** el `sider.log` revisado es de una sesión **sin partido** (solo menús, cierre limpio). Lo que pasa *durante* un
partido se dedujo leyendo el código, no viéndolo. Falta un `sider.log` de una sesión con partido jugado.

## Cómo está montado

- El **switcher** de Fralex, al elegir «ConmeGOL Patch 26», **copia** `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons` (`sider.ini`,
  `modules`, `content`, `livecpk`, `olmosjr23`) **encima** de `SiderAddons` del juego. Lo que no está en la carpeta del parche se queda.
  - Consecuencia: el `sider.ini` que manda es el de **la carpeta del parche**. Un cambio solo en `SiderAddons` del juego se pierde (así
    se perdió la línea de `phoenix.lua`, que por eso nunca se activó).
  - La carpeta del parche trae 23 archivos de módulos; los otros 63 están solo en la del juego (añadidos a mano o de versiones viejas).
- Sider **7.3.3**. `sider.ini` pide **60 módulos**: **49 cargan**, **11 no existen** (tunnel, Stadium_Banner, Stadium_Board,
  Stadium_CornerFlag, GoalSongServer, Derbys, cr7_audio, BallBoysServer, tournament_anth_tunnel, Nets-Server — el archivo real se llama
  `nets.lua` —, CornerFlag). Sider los salta sin romper nada.
- Cada módulo corre en **su propio entorno** Lua: los nombres globales repetidos (init, home, away…) **no** chocan (comprobado con el
  volcado de `env.lua`). Lo único compartido: la tabla `string`, `math.randomseed` y las declaraciones `ffi.cdef` (VirtualAlloc, compatible).
- Teclas: casi todos usan 0, 6–9, -, +, RePág/AvPág y el stick derecho, pero Sider 7 solo manda las teclas al módulo **elegido en el
  overlay**, así que no chocan (supuesto respaldado por el log).

## Problemas encontrados (ya existían, no los puso Phoenix)

| # | Gravedad | Qué | Por qué importa | Arreglo propuesto |
|---|---|---|---|---|
| 1 | 🔴 Alta | **camera.lua y DynamicWideCam.lua parchean la MISMA instrucción** (0x1408a1fbe). Cada uno suma +0x2C: la cámara Dynamic Wide termina leyendo 0x1425985d0 y DynamicWideCam escribe 0.1 ahí, encima de bytes que eran texto (`6E 64 00 00`) — **comprobado en el log** (líneas 4468-4470 y 8020-8025). | Corrompe datos del juego; cada Shift+R lo empeora. Candidato serio a cierres raros. | Quitar `camera.lua` (lo único propio que aporta es activar/desactivar repeticiones). |
| 2 | 🔴 Alta | **GFX_lod.lua** escribe en una dirección **fija** (0x14297B0C1) pensada para otra versión del exe, **sin comprobar** qué hay ahí. | Escritura a ciegas. | Quitarlo o reescribirlo con búsqueda por patrón. |
| 3 | 🟠 Media-alta | **SoundServer.lua** sigue 4 punteros desde una dirección fija sin comprobar nulos y escribe el volumen cada fotograma con su overlay abierto. | Puede leer/escribir en la nada. | Revisar o quitar. |
| 4 | 🟠 Media | **CGP_SB_Addons_2.lua** (bytecode del parche) escribe colores del marcador en memoria **cada fotograma**, sin `pcall`, con desplazamientos fijos a un exe. | Del propio ConmeGOL; se desactiva si no encuentra la firma. | Dejarlo (es del parche) y vigilarlo en el log. |
| 5 | 🟠 Media | **shirtless_celebration.lua** (añadido a mano): 15 parches de código + 512 KB de memoria ejecutable; valida antes de escribir y se apaga si no coincide, **sin `pcall`**; copia 127 archivos (32,8 MB) en cada arranque y escribe un log línea a línea; redirige 17 modelos sin comprobar que existan. | Posibles tirones; cierre si faltan esos modelos al celebrar. | Comprobar los 17 archivos; apagar su log de depuración. |
| 6 | 🟠 Media | **real-coach-team.lua** fabrica un `.fpk` en disco al empezar cada partido y mira el disco 2 veces por cada archivo que carga el juego. | Lentitud en cargas; si falla el `.fpk`, banquillo raro. | Vigilar. |
| 7 | 🟠 Media | **SleeveBadge**, **Commentary-Server**, **Competition-Server**: punteros sin comprobar / patrón débil / sin validar. Commentary usa el idioma 0x18 (fuera de rango) como respaldo. | Riesgo bajo pero real. | Vigilar en el log con partido. |
| 8 | 🟡 Baja | Duplicados y conflictos de archivos: BroadCastCam y StadiumCam tocan el mismo valor (gana StadiumCam); FanViewCam y camera.lua igual; Entrance_fix gana a Entrance; IntroServer y movieintro renombran el mismo vídeo de intro (si el `.usm` no existe → pantalla negra). | Cosmético, salvo la intro. | Elegir uno de cada par. |
| 9 | 🟡 Baja | Módulos que **no hacen nada**: Chatty_AutoLineup (es para Football Life 2026, se negó a cargar), scoreboard-hexx (solo para un marcador EPL que no se usa), goalscreams (pide Sider 7.3.4+), Anti-Cheat (placebo: inventa campos que Sider no tiene; además está desactivado), env.lua (solo vuelca datos al log), matchset. | Ruido, carga inútil. | Quitar. |
| 10 | 🟡 Baja | `sider.ini`: «Menu Light» dos veces, un tabulador suelto, 11 módulos inexistentes; faltan `modules\IMAGENS\gervol.png` y `chantsserver.png` (SoundServer, Chants-Server); `UIColors` no encuentra `topModeSelectDMM.bin`. | Cosmético. | Limpiar. |
| ⚠️ | Desactivado | **attack_mentality.lua** (comentado) es de **alto riesgo** si se vuelve a activar: reescribe código del juego mientras se ejecuta. | — | No activarlo. |

Otros: `ReShade.log` tiene errores de compilación de algunos shaders (no afectan al juego); `dlss5-feed-crash.dmp` está **vacío** (0 B, 8 oct 22:10): no da información.

## Lo que esto significa para Phoenix

- **Ninguna** de las zonas de memoria que tocan los módulos actuales se cruza con lo que hace `phoenix.lua`: la prueba 1 **solo** usa
  el evento `overlay_on` (dibujar texto) y lee un archivo propio. No escribe memoria, no redirige archivos, no cambia `ctx`, no usa teclas.
  Es la superficie de choque más pequeña posible.
- **Certeza:** nadie puede dar un 100 % matemático con 49 módulos de terceros y un juego cerrado. Lo que sí se puede es: (1) no compartir
  nada con ellos (hecho por diseño), (2) arreglar los choques que ya existen (tabla), (3) probar de forma controlada — partido sin el
  módulo, partido con el módulo, comparar los dos `sider.log` — y (4) que el módulo se apague solo ante cualquier error.
- **Para módulos futuros:** `IntroServer.lua` **cambia `ctx.tournament_id`** (86/87→88, 89→90): un módulo Phoenix que lea la competición
  verá el valor cambiado. Los módulos CGP comprueban que ciertos equipos estén en ciertas competiciones (111, 128, 137, 90): Phoenix
  Mercado mueve **jugadores**, no equipos, así que no les afecta; si algún día mueve equipos entre competiciones, hay que respetarlo.
- **Dónde instalar:** en la carpeta del parche (`ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`), para que el switcher lo lleve.

## Plan propuesto (cada paso con permiso de Fralex)

1. Jugar **un partido** con el Sider actual (sin Phoenix) y guardar ese `sider.log` como referencia.
2. Limpieza con copia de seguridad: quitar `camera.lua`, `GFX_lod.lua` y los módulos que no hacen nada; quitar las 11 líneas de módulos
   inexistentes y el «Menu Light» repetido. Otro partido y comparar logs.
3. Recién entonces, añadir `phoenix.lua` (una línea, en la carpeta del parche) y otro partido de prueba.
