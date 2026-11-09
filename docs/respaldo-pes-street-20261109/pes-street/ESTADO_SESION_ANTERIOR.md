# Estado de la sesión anterior de STREET (escrito 16:15, 6-nov-2026)

Supersede: nada (documento nuevo). Pedido por una línea de PES_EN_USO_STREET.txt (15:30:47) y útil para quien continúe.

## Qué está instalado AHORA en `SiderAddons\content\stadium-server\Phoenix\Calle 30x20\`
- `Asset\model\bg\st002\pitch\#Win\pitch_st002.fpk` = v24 (md5 5522f1de…): suelo de concreto con líneas pintadas, reja de 6 m (fondo y lado largo del fondo, red sobre los arcos, lado cámara 1 m), muros y gente de barrio. Copia idéntica en `SiderAddons\content\phoenix\cancha_visual\...`.
- `Asset\model\bg\common\pitch\sourceimages\#windx11\line_alp.ftex` = v24b (md5 1bc1c77a…), también en `AddOn\FD TURF\Global\...`. Base del concreto [52,48,55] (con Light SG7 queda gris con leve violeta). La v24 anterior con base [33,31,26] salió verde oliva oscuro: descartada.
- `Asset\model\bg\st002\#Win\st002.fpk` = pack_B2 (md5 6d08696a): 11 escenas del estadio sustituidas por stub (sin gradas ni techo).
- `common\demo\fixdemoobj\st002_bg\#Win\st002_h_all.fpk(.fpkd)` y `st002_h_all_choreo.fpk(.fpkd)`: stub de las piezas del estadio de la cámara de presentación (main, sub, outl, floor, polemodel, lightmodel, lght, name, hous). Origen: `v25_demo\w`. NO VERIFICADO que PES lo cargue desde esa ruta.
- `Asset\model\bg\common\audi\#Win\`: 6 audi stubs (B2) instalados 16:06 (antes solo A2 = seatOnly+audi_seat, que NO quitó los asientos flotantes). Resultado de B2: sin verificar.
- `AddOn\addon_config.ini`: 1 StadiumScreen_UCL, 2 FD TURF, 3 Light, 4 Goal Post Colour, 5 Light SG7 (cielo azul con nubes real). Exterior_Aerial_CampNou26 ya no está en la lista (lo quitó la sesión de las 15:10).
- Respaldo del piso anterior: `herramientas\Blender279\respaldos\v22-antes-v24\`.

## Listo pero NO instalado / ideas
- Fuentes: `herramientas\Blender279\calle_30x20\v24\` (hacer_v24.py, reja.py, texturas.py), `v25_demo\w` (fpk de demo con stub), `v21\stub2.fmdl`, `v21\audi_packs\{A2,B2}`.
- Para volver al piso v22: copiar los dos archivos de `respaldos\v22-antes-v24`.

## Lo que probé y resultado
- Piso: con Light SG7 el concreto v22 se veía celeste (luz azul). v24 base cálida [33,31,26] → verde oliva oscuro; v24b [52,48,55] → gris concreto con leve violeta (OK).
- Reja 6 m: visible en juego (captura a las 15:50).
- Cámara de presentación: antes de instalar el stub de demo mostraba el Camp Nou completo; tras instalarlo NO capturé la presentación (pantallas negras). Pendiente.
- Cámara lejana (captura de Frank 19:14 UTC): asientos flotantes del Camp Nou, pasto verde laterales, franja blanca abajo y rectángulo oscuro arriba. Pendientes: audi B2 (verificar), anillo de suelo exterior opaco (sin tocar st081_pitch_0), rectángulo oscuro (sin identificar), franja blanca de cielo sobre las casas en la cámara de juego.
- REA: open_binary OK, pero la primera búsqueda sobre el exe de 458 MB (Ghidra) se colgó 1 h. Alternativa: script directo sobre el exe. Hallazgo: el juego arma `model/bg/audi/area/st%03d/audi_area.xml`, `audi_normal.xml`, `audi_liberta.xml`, `audi_saudiarabia.xml` y `/Assets/pes16/model/bg/st%03d/audi/audiarea_*.fpk`; esos XML no aparecen en las listas TOC de los cpk de Data. Función que lo carga: aún no localizada (RVAs de cadenas: audi_area.xml 0x2b672f9, audiarea_ 0x2b6739e).
- StadiumServer.lua (`is_it_stadium_file`) redirige `asset\model\bg\common...`, `asset\model\bg\stNNN...` y `common\demo\fixdemo*`.

## Reglas que no se rompen
- No tocar st081_pitch_0 / pitch_ed.002 (deja la escena blanca). No Core. No pcall. REA solo sobre copia del exe.
- Los cambios del jugador/portero/final de partido/reja física son del hilo Core/IA (medidas de reja: x=±14,95, y=±9,95, hueco de arco ±2,4, altura 6 m).

## Estado de PES al escribir esto
- PES abierto desde 16:07 en el menú de título; las teclas del MCP llegan con mucho retraso y el foco saltaba entre pantallas (posible otra sesión también pulsando). No hay partido en curso.
