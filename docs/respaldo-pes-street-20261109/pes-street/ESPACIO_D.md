# Espacio en D: (8-nov-2026, 02:40 PC) — NO se borró nada

D: 952 GB, 100 % usado. Libre medido: 2,1 GB (01:15) → 600 MB (02:07) → 580 MB (02:38). Lectura solo; todo lo de abajo lo decide Frank.

## Lo grande (medido con du, en MB)

| Ruta | Tamaño | Edad / nota | ¿Candidato? |
|---|---|---|---|
| `Games_\Conmegol Patch\SiderAddons\content` | 114 630 MB | el parche/escenarios del juego | NO (es el juego) |
| `IA\CLAUDE\herramientas` | 9 460 MB | herramientas de RE y Blender | ver abajo |
| `IA\CLAUDE\herramientas\Blender279` | 6 409 MB | | ver abajo |
| `…\Blender279\calle_30x20` | 5 102 MB | versiones de la cancha Calle, 6–7-nov | SÍ, las viejas |
| `…\calle_30x20\completo_total` | 743 MB | 6-nov | revisar con Frank |
| `…\calle_30x20\v27_param` | 369 MB | 7-nov (activa, tamaños S/M/L) | NO |
| `…\calle_30x20\diagnostico` | 347 MB | 6-nov | probable |
| `…\calle_30x20\v20` / `v21` | 253 / 193 MB | 6-nov | probable |
| `…\calle_30x20\v26_anillo` (+`_bak` 144 MB) | 161 MB | 7-nov (instalada) | NO (el `_bak` sí) |
| `…\calle_30x20\v18, v18b…v18m, v19, v22, v23` | ≈ 140 MB c/u, ≈ 2 500 MB en total (≈ 18 carpetas) | 6-nov, versiones intermedias superadas por v24/v26 | SÍ, principal ahorro |
| `…\calle_30x20\v24` | 143 MB | 6-nov (base de v26) | conservar |
| `…\Blender279\respaldos` | 477 MB | respaldos de paquetes instalados (rollback) | NO hasta cerrar Street |
| `…\Blender279\blender-2.79b-windows64.zip` | 111 MB | instalador (ya existe la carpeta descomprimida de 318 MB) | SÍ el zip |
| `…\Blender279\cancha_playa, favela_30x20, analisis_estadio, prueba_etapa1*…` | ≈ 350 MB | pruebas viejas de otras canchas | revisar |
| `IA\CLAUDE\herramientas\Ghidra-12.0.4` | 1 342 MB | herramienta | NO |
| `IA\CLAUDE\herramientas\PES21FX / ImHex / RenderDoc / x64dbg / REA` | 427 / 320 / 297 / 234 / 118 MB | herramientas | NO |
| `Games_\PRUEBA_CANCHA` | 4 530 MB | investigación PES (copias de exe, Ghidra, informes) | NO sin Frank |
| `IA\CLAUDE\pes-street` | 422 MB | paquetes de amigos, STAGING_05xx, capturas | parte: STAGING viejos |
| `IA\CLAUDE\pes-borrador` | 243 MB | copia VIEJA de la web (12-oct) | SÍ (ya marcada obsoleta) |
| `…\SiderAddons\phoenix_core` | 313 MB | Core + logs | ver logs |

## Logs del Core (dentro de phoenix_core)

| Archivo | Tamaño | Nota |
|---|---|---|
| `sonda_partido.txt` | 110 MB y crece ≈ 3 MB / 10 min | el Core 0.5.44 lo apaga y lo mueve a `.1` |
| `peticiones.log` | 69 MB | idem |
| `phoenix_core.log.antes-0536-13h` | 15 MB | log cortado, ya no hace falta |
| `phoenix_core_sesion_18xx/19xx_cierre*.log` | ≈ 8–10 MB cada uno (≈ 40 MB) | de sesiones del 6-nov |
| `phoenix_core.log.1` | 20 MB | 6-nov |
| `phoenix_core.dll.pre-05xx / phoenix_core.json.pre-05xx` | ≈ 0,5 MB c/u (decenas) | copias de deshacer, baratas |

## Propuesta (para pedir permiso a Frank)

1. **≈ 2,5 GB**: las carpetas `v18*`, `v19`, `v22`, `v23` de `calle_30x20` (versiones intermedias del 6-nov; la buena es v24→v26→v27).
2. **≈ 0,25 GB**: `pes-borrador` (copia vieja de la web, ya marcada «no trabajar en ella»).
3. **≈ 0,1 GB**: `blender-2.79b-windows64.zip` (hay carpeta descomprimida).
4. **≈ 0,2 GB**: `sonda_partido.txt`, `peticiones.log`, `phoenix_core.log.antes-0536-13h` y los `sesion_*_cierre*.log` (tras instalar 0.5.44).
5. Revisar con Frank: `completo_total` (743 MB), `diagnostico` (347 MB), `v20`/`v21` (446 MB), pruebas viejas de Blender (≈ 350 MB).

Total prudente sin tocar nada importante: **≈ 3 GB**. No se borra nada hasta que Frank diga que sí.

## Qué NO explica la bajada
En 1,5 h el libre cayó ≈ 1,5 GB y los ficheros del Core solo crecieron ≈ 10–20 MB. El resto lo consume algo fuera de mis carpetas (otro hilo, Drive para escritorio, Windows, capturas del MCP, etc.); no lo localicé.
