# Informe de la noche del 6-nov-2026 - Phoenix Street (bitácora-guía)

Resumen (SÍ/NO primero):
- ¿Street funciona? SÍ: entrenamiento libre 3 vs 3 con arquero fijo, cancha de calle, reja gris, arcos de futsal; verificado en el juego con capturas.
- ¿Está instalado lo nuevo de hoy? SÍ: la reja gris sin verde (V17) y el AMBIENTE DE BARRIO V18h (muro con ventanas, puerta y mural, zócalo de ladrillo, farolas y banco), verificado en el juego sin artefactos (captura `capturas\V18h_OK_muro_farola_banco_0805.png`). ACTUALIZACIÓN: instalada V18m (muros laterales + gente), ver Nota final E.
- ¿Paquete para amigos? SÍ, hecho y probado en seco (no en otro PC): `PhoenixStreet_amigos_v3.zip` (incluye ambiente V18m y las reglas de calle apagadas; v2 = sin reglas, v1 = ambiente V18h, v0 = sin ambiente).
- ¿Triángulos del escenario resueltos? SÍ (ver Nota final D): eran celdas de suelo exterior lejanas dibujadas sobre el muro.
- ¿Botón «Phoenix Street» del menú? Ver sección 7.

## 1. Qué había al empezar y qué quedó
- Estado estable inicial: respaldo `respaldos\street_estable_inicio_noche_0502` (163 MB).
- Instalado ahora: V16->V18h (fpk md5 0ac6bf13; antes V16 2c296dea, suelo de cemento BC7, líneas de futsal, reja con red) + V17 (solo cambia `line_alp.ftex`, md5 60e4481d: la red de la reja pasa de verdosa a gris neutro oscuro). Arcos 3x2 con palos rectangulares (v17_palos). `StadiumServer.lua` con estadio automático, panel paginado en `phoenix_marcador_free.lua`.
- Respaldo estable final: `respaldos\street_estable_fin_noche` (si no existe, no se llegó a hacer).

## 2. Cómo llegar a la cancha (ruta verificada 5 veces)
Ver `D:\Frank\IA\CLAUDE\pes-mcp\ruta_entrenamiento_libre.txt`. En resumen: menú principal -> Día de Partido -> bajar 12 veces hasta «Entrenamiento» -> «Entrenamiento libre» (bajar 1) -> equipos (Enter x4) -> Menú de entrenamiento: PT fijo = Sí, usuario 3, COM 3 -> OK.
ADVERTENCIA: el mosaico «Entrenamiento» del menú principal lanza otro modo (MyClub/habilidades). Las teclas del MCP llegan con ~2 s de retraso; esperar 6 s tras cargar pantallas y comprobar la zona en `sonda.json` (FREE_TRAINING). Si las teclas se desfasan se cae en el tutorial de habilidades (Esc no sale; cerrar PES y reabrir).

## 3. Lo verificado en el juego (capturas en `pes-street\capturas\`)
- V16: suelo gris realista sin cuadros, líneas blancas continuas, círculo y áreas de futsal, reja a la altura de la línea sin tapar la cámara, arco con palo rojo/blanco, exterior gris uniforme (sin verde ni púrpura).
- V17: reja gris sin puntitos verdes (`V17_reja_gris_0651_OK.png`).
- Core 0.5.9, 0.5.14 y 0.5.15 (hilo IA): modo normal, 0 errores en `phoenix_core.log`; `[GK_AREA_DIAG]` correcciones=0; `[GK_AREA]` no existe (solo el DIAG).

## 4. Ambiente de barrio (V18 a V19): qué se hizo y por qué no quedó
Cinco versiones probadas en el juego (cada una con captura): muro de fondo con ventanas, puertas, zócalo de ladrillo y colores por tramo; muros laterales; 3 farolas y un banco; atlas ampliado a 4096x4096 (BC7, 8,4 MB).
Resultado: el muro se ve bien, pero aparecen triángulos grises largos por delante del muro. No cambian al mover el muro (12,6 a 10,6 m), quitar farolas, quitar muros laterales ni corregir la UV degenerada de los postes (`uv_poste` daba 4 UV iguales; corregido en v19 aunque no era la causa). Por eso NO se instaló en ese momento (RESUELTO después: ver Nota final D).
Paquetes (NO instalados salvo v17): `calle_30x20\v17` (reja gris, INSTALADO), `v18`, `v18b` (sin farolas, muro oscuro), `v18c` (solo muro de fondo), `v18d` (muro a 10,6 m), `v19` (completo + UV corregida).
Aprendizajes: albedo del muro x0,58 para compensar el multiplicador del material (~2,4); el exportador FMDL acepta geometría extra en `st081_pitch_1`; un atlas 4096x4096 BC7 carga bien.

## 5. Triángulos del escenario (RESUELTO al final: ver Nota final D)
Evidencia: no se mueven con el muro (están más cerca de la cámara), se ven por detrás de la malla de la reja, son grises y ya existían en V16 pero se confundían con el suelo gris.
Descartado: el fmdl del suelo (solo `st081_pitch_0` y `st081_pitch_1`, planos) y `light_st002_af.fpk` (es un sky_dome).
Sin probar: `effect_st002_df.fpk`, `home_big_flag_st002.fpk`, `flagarea_st002.fpk`, `tv_st002.fpk`, `scarecrow`, `audiarea_st002.fpk` y el principal `st002_df.fpk`. En el entrenamiento el juego pide (peticiones.log): `st020\#Win\st020.fpk`, `st020_df.fpk`, `light_st020_af`, `effect_st020_df`, `big_flag`, `standsFlag`, `audi`, `tv`, `cheer`, `ad_st020_normal`, `common\audi\*`.
Plan (coordinador): con el muro V18c puesto, servir en blanco la mitad de los candidatos por arranque.

## 6. Reglas de calle (Lua, apagadas por defecto; INSTALADAS el 6-nov 08:50, ver Nota final F)
`calle_30x20\v18_reglas\phoenix_street_reglas.lua`: «primero a N goles» y «gol de oro», con la orden `street_fin_pedido=1` en `ordenes.txt` (el Core 0.5.13+ la atiende). Claves `street_regla_meta` (0 = apagado, 3 a 15) y `street_gol_oro`. Falta añadir sus filas al panel F9 y probarlas en el juego.

## 7. Modo amistoso y botón «Phoenix Street»
Investigación en `INVESTIGACION_MODO_AMISTOSO.md`. Recomendado el Camino A (botón en un hueco muerto del menú que entra directo al entrenamiento). Si no hay nota al final de este informe, la implementación no se llegó a hacer.

## 8. Paquete para amigos
`PhoenixStreet_amigos_v1.zip` (32 MB, con el ambiente V18h) / carpeta `PhoenixStreet_paquete`; el `v0.zip` (28 MB) es la versión sin ambiente. Instalador con copia de seguridad (`_backup_phoenix_street_<fecha>`), desinstalador y LEEME. Prueba en seco en una carpeta falsa: OK; se corrigió un fallo (el desinstalador no borraba la carpeta creada). Pendiente: probarlo en otro PC; regenerar el zip si el Core cambia (0.5.16+).

## 9. Permisos y turnos de PES
Al principio no pude abrir PES: mi comando llevaba la ruta entre comillas y un `echo` delante y no coincidía con la regla de permiso que ya existía (`abrir_pes.ps1:*`). Con el comando exacto «powershell -NoProfile -ExecutionPolicy Bypass -File D:/Frank/IA/CLAUDE/pes-graficos/abrir_pes.ps1» funciona; cerrar igual con `cerrar_pes.ps1`. No se cambió ninguna configuración de permisos.
Turnos para el hilo IA: Core 0.5.14 (05:47-05:55) y 0.5.15 (06:06-06:11), ambos con cierre limpio. Bitácora: `PES_EN_USO_STREET.txt`.

## 10. Cómo deshacer
- Volver a V16: copiar `calle_30x20\v16\Calle 30x20\...\line_alp.ftex` (md5 873acede) sobre las dos rutas de `stadium-server\Phoenix\Calle 30x20` (AddOn\FD TURF\Global\Asset\... y Asset\...). Respaldo: `respaldos\v16-ftex-antes-v17`.
- Volver al estado de la noche anterior: `respaldos\street_estable_inicio_noche_0502`.
- Quitar el estadio automático: `estadio_auto=0` en `content\phoenix\estadio_auto.ini` o apagar la fila del panel.

## 11. Pendientes
1. (HECHO) Triángulos resueltos e instalado V18h. Siguiente en ambiente: gente tras la reja, muros laterales (recortando también el suelo lateral), más grafitis.
2. Instalar y probar las reglas de calle (filas de panel).
3. Botón «Phoenix Street» y panel «Táctica Street».
4. Probar el paquete de amigos en otro PC.
5. Sombras de jugadores sobre el suelo (sin resolver).
6. Probar de nuevo con Core 0.5.16+ cuando el hilo IA lo instale (turno de arranque) y regenerar el zip de amigos si cambia el Core.

## Nota final A - Botón «Phoenix Street» por el hueco 06_myclub (probado 07:01-07:09, REVERTIDO)
Qué se hizo: carpeta `SiderAddons\livecpk\Phoenix-Street-Boton\common\script\flow\TopMenu\TopMenuNew.json` = el de Conmegol con UN solo cambio (`06_myclub`: `Tutorial/TutorialUgcCheckJoinMode` -> `Match/TrainingUgcCheckJoinMode`) y una línea `cpk.root` en `sider.ini` antes de `\livecpk\Menu` (respaldo `sider.ini.antes-boton-street-20261106`).
Resultado en el juego: el mosaico «Entrenamiento» del menú principal SÍ lleva al flujo del entrenamiento (Empezar -> Local/Visitante -> estrategia -> menú), pero la ZONA del juego queda en `MYCLUB` (id 30), no en `FREE_TRAINING` (id 52). Street (estadio automático, marcador, Core) solo se activa en FREE_TRAINING, así que esa ruta NO sirve tal cual. La zona la fija el elemento de menú elegido (Día de Partido -> Entrenamiento -> Entrenamiento libre dispara el evento `05_freeTraining`), no el JSON de destino.
Revertido: `sider.ini` idéntico al original (la carpeta `Phoenix-Street-Boton` queda sin usar).
Siguientes ideas: (a) pedir al hilo IA que el Core y los módulos también activen Street cuando la zona sea MYCLUB y la pantalla sea del flujo de entrenamiento (riesgo: MyClub real; en Conmegol el hueco está reutilizado); (b) hacer que otro hueco muerto dispare el evento `05_freeTraining` (hay que averiguar si el evento por sí solo fija la zona); (c) atajo por Lua que lleve de «Día de Partido» directo a «Entrenamiento libre» (teclas virtuales).

## Nota final B - Opción (b): disparar el evento 05_freeTraining desde un hueco (analizado, NO posible solo con JSON)
`TopMenuNew.json` (Conmegol) solo contiene `events` (evento -> pantalla de destino), `data` (type, bgId, mark) y `string` (títulos). No hay ningún campo de zona ni de evento por mosaico. Los eventos `04_tutorial`, `05_freeTraining` y `06_myclub` ya existen en el JSON; QUÉ evento dispara cada mosaico y cada ítem de la lista de «Día de Partido» lo decide el dibujo del menú (`topModeSelectDMM.bin`, Flash compilado) y el código del exe; y la zona del juego (`FREE_TRAINING`=52, `STEPUP_TUTORIAL`=51, `MYCLUB`=30) sale del evento que se dispara. Cambiar el destino de un evento (JSON) no cambia la zona. Para que un mosaico del menú principal dispare `05_freeTraining` hay que editar el `.bin` (cap. 60-p, nivel 3, dificultad alta) o usar un atajo por Lua (teclas virtuales) o que el Core/Lua traten MYCLUB + flujo de entrenamiento como Street.

## Nota final C - Búsqueda de los triángulos por mitades (7 arranques, 07:11-07:47; TODO REVERTIDO)
Todas las pruebas con el muro V18c/V18f puesto y captura cada vez (carpeta `pes-street\capturas\bisect_*.png`):
1. Mitad A en cuarentena (st002: effect, scarecrow, cheer, cheer1, big_flag, standsFlag, audi) -> triángulos siguen. (Primer intento con atlas desfasado por error mío: V18c fpk + atlas V17; repetido bien.)
2. Mitad B (st002: tv, staff, cornerflag; common: ad, bill, standsFlag, cornerflag) -> siguen.
3. `arcos_visuales=0` (sin arcos/redes de Street) -> siguen.
4. V18f: muro SIN reja -> siguen (no son sombras ni geometría de la reja).
5. `turf3d` (césped 3D, en Calle y en AddOn FD TURF) en cuarentena -> siguen.
6. `clima_forzado=1` (nublado) -> cambia el tono, los triángulos siguen (no son sombras solares).
Conclusión: no dependen de ninguno de esos elementos. Hipótesis que queda: orden de dibujado/profundidad del material `line_alp_dds2.001` (alpha_flags 80, mezcla): el suelo exterior lejano (celdas «ext») se dibuja después del muro y lo tapa en forma de triángulos porque el material no escribe profundidad. Próximo experimento concreto: (a) poner el muro en una malla/material aparte opaco (copia del material con alpha_flags de opaco) o (b) reordenar el índice (muro antes que el suelo) o (c) recortar las celdas exteriores del suelo detrás del muro (y>muro) para que no haya suelo que dibujar allí.
Estado al terminar: fpk V16 (2c296dea), line_alp V17 (60e4481d), clima_forzado=0, arcos_visuales=1, todas las cuarentenas devueltas a su sitio (carpetas vacías `_cuarentena_triangulos_*` dentro de Calle 30x20, solo con MANIFIESTO.txt, se pueden borrar). Respaldos: `phoenix_core.json.antes-bisect-arcos`, `...-clima`.


## Nota final D - TRIÁNGULOS RESUELTOS y ambiente de barrio INSTALADO (V18g -> V18h, 07:49-08:05)
Hipótesis confirmada: las celdas de suelo exterior LEJANAS (cuadros de 4 m de la malla `st081_pitch_1`, más allá del muro) se dibujaban por encima del muro porque el material `line_alp_dds2.001` (alpha_flags 80, mezcla) no escribe profundidad. Solución: no generar celdas de suelo con y >= 11,3 (detrás del muro de fondo). V18g (muro + sin suelo detrás) -> triángulos desaparecieron (`capturas\V18g_sin_triangulos_0753.png`).
Pulido en V18h: muro de fondo pegado al borde de la losa (y = 11,3 m, sin hueco verde), altura 8 m (tapa el césped original de más allá), 3 farolas (poste, brazo y cabeza luminosa) y un banco delante del muro. Sin muros laterales (el suelo exterior de los costados se seguiría dibujando encima).
Verificado en el juego (08:04, 3 vs 3, PT fijo, Core 0.5.15 normal): muro con 8 tramos de color, ventanas, puertas, mural, zócalo de ladrillo, reja gris sin verde, farola central, banco; sin triángulos ni franjas verdes. INSTALADO (fpk md5 0ac6bf13 en las dos rutas, line_alp md5 46dcc8d3 = atlas 4096x4096).
Cómo deshacer el ambiente: copiar `calle_30x20\v16\Calle 30x20\Asset\model\bg\st002\pitch\#Win\pitch_st002.fpk` (md5 2c296dea) sobre las dos copias del fpk (`stadium-server\Phoenix\Calle 30x20\Asset\model\bg\st002\pitch\#Win\` y `content\phoenix\cancha_visual\Asset\model\bg\st002\pitch\#Win\`) y `calle_30x20\v17\Calle 30x20\...line_alp.ftex` (md5 60e4481d) sobre las dos rutas de line_alp (Asset\model\bg\common\pitch\sourceimages\#windx11\ y AddOn\FD TURF\Global\Asset\model\bg\common\pitch\sourceimages\#windx11\). Respaldos del paso previo: `respaldos\v16fpk-antes-bisect`, `respaldos\v18-instalada-antes-v18b`.
Lecciones: (1) el fpk y el atlas deben ser de la MISMA versión (V18 usa atlas 4096x4096; mezclarlos con el de V17 desordena las UV); (2) con este material el suelo exterior lejano tapa lo que esté delante: hay que recortarlo; (3) las cajas pequeñas (farolas, banco) no daban problema, la causa era el suelo; (4) la UV degenerada de `uv_poste` (4 puntos iguales) se corrigió en v19 pero V18h usa la reja de v18 (sin ese cambio) y se ve bien.
Pendientes del ambiente: público/gente tras la reja, muros laterales (habría que recortar también el suelo lateral), más grafitis, sombras de jugadores.


## Nota final E - Muros laterales y gente tras la reja: V18i a V18m (08:09-08:47), INSTALADA V18m
Qué se añadió sobre V18h: (1) muros laterales a x = +-17,2 m detrás de los arcos, de y = -14,2 a 11,3 m y 8 m de alto; (2) se recortó el suelo exterior lateral (celdas con |x| >= 17,0 y y1 > -11,0) para que no se dibuje sobre ellos (lección de los triángulos); (3) 12 siluetas de personas de pie entre la reja y el muro (cartulinas con transparencia, 8 variantes de ropa y piel apagadas, altura 1,6 a 1,85 m), dibujadas al final del mesh; (4) farolas y banco ya estaban.
Errores en el camino: V18i dejó las siluetas con 4 variantes y casi todas con camisa azul (se vio de cartón) -> V18j 8 variantes apagadas; seguía un parche verde a la izquierda -> V18k (celda de la esquina SO conservada) -> V18l (muro lateral hasta y=-14,2) -> la causa real: los DOS muros laterales tenían el orden de vértices al revés (visto desde dentro eran cara trasera y el juego los descartaba, por eso nunca se veían y el suelo recortado dejaba ver el césped original). V18m = orden corregido (CCW visto desde dentro); el muro lateral izquierdo aparece y el parche verde desaparece.
Verificado en el juego (08:46, 3 vs 3, PT fijo, Core 0.5.15): muro de fondo + muro lateral izquierdo, ventanas, puertas, mural, zócalo de ladrillo, farola, banco, gente, reja gris; sin triángulos. INSTALADO: fpk md5 92d66d3f (Calle y cancha_visual), line_alp md5 4a32f4b9 (Asset y AddOn FD TURF). Capturas: `capturas\V18m_OK_muros_laterales_gente_0846.png`, `V18i_...`, `V18j_...`, `V18k_...`, `V18l_...`.
Limitaciones: la gente es ESTÁTICA (no animada): animar personas de los estadios del juego base exige añadir modelos con esqueleto y animación (`staff_st002.fpk` etc.) al mesh o servir otro fpk, y no se probó; la reja y el muro no proyectan sombras sobre el suelo; el muro lateral derecho no se pudo comprobar bien en la captura (queda fuera de cuadro).
Deshacer solo esta tanda: copiar `calle_30x20\v18h\Calle 30x20\Asset\model\bg\st002\pitch\#Win\pitch_st002.fpk` (md5 0ac6bf13) sobre las dos copias del fpk Y `calle_30x20\v18h\...line_alp.ftex` (md5 46dcc8d3) sobre las dos rutas de line_alp (el atlas de V18h no tiene las 8 siluetas; fpk y atlas deben ser de la misma versión). Paquete amigos: `PhoenixStreet_amigos_v2.zip` (32 MB).


## Nota final F - Reglas de calle INSTALADAS (apagadas) con filas en el panel (08:50-09:05)
Qué se instaló: `modules\phoenix_street_reglas.lua` (v0.1) y una línea `lua.module = "phoenix_street_reglas.lua"` justo después de `phoenix_marcador_free.lua` en `sider.ini` (respaldos: `sider.ini.pre-reglas-20261106`, `modules\phoenix_marcador_free.lua.pre-reglas-20261106`). La configuración vive en `content\phoenix\reglas_street.ini` (`street_regla_meta=0`, `street_gol_oro=0` = APAGADAS), porque el Core no conoce esas claves; el panel la escribe directo, igual que ya hace con `estadio_auto`.
Panel F9: dos filas nuevas al final de la página 3 de 3: «Regla: gana el primero a N goles (0 = apagada)» (numérica 0 a 15) y «Regla: gol de oro (empate al final: el siguiente gol gana)» (ON/OFF). Página 3 ocupa ~720 de 1900 bytes (límite del overlay respetado); páginas 1 y 2 sin cambios (1314 y 577).
Cómo funciona: al llegar un equipo a N goles (o, con gol de oro, al marcar con el reloj en minuto 90 o más y marcador desigual) el módulo añade una sola vez la línea `street_fin_pedido=1` a `phoenix_core\ordenes.txt`; el Core 0.5.13+ la atiende (final nativo con revancha), borra el archivo, y pasados 3 s el módulo se rearma con marcador a 0. Solo actúa en FREE_TRAINING con Street activo.
Verificación: (1) en el juego (Core 0.5.15) sider.log: el módulo de reglas y el del panel cargan sin errores (`OK: Lua module initialized`); (2) banco de pruebas en Lua (`scratchpad\prueba_reglas`, entorno simulado): apagado no actúa; meta=1 pide fin una sola vez aunque haya más goles; tras atender el Core se rearma; meta=3 solo en el tercer gol; gol de oro solo desde el minuto 90; fuera de FREE_TRAINING no hace nada: TODO OK.
NO verificado: (a) las filas en pantalla (el MCP solo tiene las teclas 1 y 2 y no puede mover la selección del panel, que usa 8/9/0 y F9/F10; la página 3 no se llega a ver) y (b) el final nativo con revancha del Core ([FIN_PEDIDO]): el partido se queda parado en el saque (3 vs 3, mi jugador quieto), en ~10 minutos de reloj no hubo goles y no se pudo provocar uno. Cuando alguien juegue con mando: encender la meta con N=1 desde el panel (8/9 hasta la fila, 0 para subir) y comprobar `[FIN_PEDIDO]` en `phoenix_core.log`.
Errores no fatales vistos en sider.log al empezar el partido (set_teams 108 vs 109): `StadiumServer.lua:705 attempt to index a nil value` (49 veces, módulo `module_stadium_name`) y `TournamentCornerFlags.lua:98 CornerFlag nil` (1 vez); son de módulos de Conmegol, el estadio se pone igual; pendiente comparar con una sesión anterior para saber si ya existían.
Cómo deshacer: quitar la línea de `sider.ini` (o copiar `sider.ini.pre-reglas-20261106`), restaurar `phoenix_marcador_free.lua.pre-reglas-20261106` y borrar `modules\phoenix_street_reglas.lua` y `content\phoenix\reglas_street.ini`. Paquete amigos v3 incluye las reglas apagadas y el instalador añade su línea.

## Nota final G - Los avisos de sider.log «StadiumServer.lua:705» y «TournamentCornerFlags.lua:98» NO son nuestros (analizado 09:10)
- La línea 705 de `StadiumServer.lua` es `stadium_path = team_assignment_map[ctx.home_team][random_num_team][3]`, idéntica y en el mismo número de línea en el ORIGINAL de Conmegol (respaldo `respaldos\estadio-auto-20261106-032503\StadiumServer.lua`, anterior a nuestro parche). Nuestro parche solo añade `street_auto_estadio` (líneas 1081-1113, antes de `set_stadium`).
- El `sider.log` del 1-nov (`diag_minicancha_1415\sider.log`, 14:35, mucho antes del parche del 6-nov) contiene los MISMOS 49 avisos de la línea 705 (`module_stadium_name`) y el de `TournamentCornerFlags.lua:98`. Causa: `module_stadium_name` consulta `random_num_team` antes de que `set_stadium` lo asigne (queda nil al hacer `[nil][3]`). Es de Conmegol y no afecta: el estadio se pone igual.
- Decisión: solo anotado, sin tocar el módulo (no es nuestro y el parche se deja mínimo).
