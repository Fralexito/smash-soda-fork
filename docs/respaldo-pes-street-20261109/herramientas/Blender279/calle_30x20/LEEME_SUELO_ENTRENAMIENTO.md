# Suelo de la Calle 30x20 — qué pinta el suelo, rutas y método (6-nov-2026)

Bitácora-guía para replicar. Todo lo de aquí se comprobó con `sider.log`, `SiderAddons\phoenix_core\peticiones.log` y los cpk del juego (solo lectura).

## 1. Cómo llega un archivo del estadio al juego
- `StadiumServer.lua` (`make_key` / `get_filepath`, líneas ~1013-1060) redirige **todo** archivo que cumpla `is_it_stadium_file` (`asset\model\bg\common*`, `asset\model\bg\stNNN*`, etc.) a la carpeta del estadio elegido, **sin comprobar que el archivo exista** allí. Si falta en la carpeta del estadio, el juego recibe una ruta inexistente: textura perdida = blanco.
- Por eso `Phoenix\Calle 30x20` debe contener todo lo que el Camp Nou contiene. Desde el 6-nov está instalada como **copia completa de `España\Spotify Camp Nou`** (2713 archivos, 789 MB) y encima nuestros archivos. Paquete: `calle_30x20\completo_total\Calle 30x20\`.
- La Calle se asigna al equipo 9901 en `map_teams.txt` (línea 780). Un partido 108 vs 109 con el menú renombrado «Camp Nou --> Phoenix - Calle 30x20» sigue siendo Camp Nou (el renombre es solo texto del menú).

## 2. Qué archivos pidió el juego (peticiones.log)
- Carga de entrenamiento del 6-nov 01:23: `st002_field_bsm_ed01.ftex`, `common\edit\pitch\...\grain_ed01_alp.ftex`, `st002_turf001_{bsm,nrm,srm,lym,lbm,sb_nrm}.ftex`, `common\turf3d\#Win\turf3dtexlodu4.fmdl`. **No** pidió `pitch_alp.ftex` ni `line_alp` ni `turf_bsm_alp_rgba32`.
- Cargas del 5-nov 02:37 y 02:55: sí pidió `pitch_alp.ftex`, `pitch_grain_alp`, `grass_slopetable_alp`, `turf_bsm_alp_rgba32`, `turf_nrm`, `line_alp` (material pes3DDF_Pitch2020 del `pitch_st002.fmdl`).
- `st002_field_bsm_ed01.ftex` **no existe en ningún cpk del juego**: `dt30_g4.cpk` contiene `Asset\model\bg\st002.cpk` (366 archivos, sin `field`/`ed01`) y `common.cpk` (2117 archivos, sin él). Es una petición a un archivo ausente (inocua en el Camp Nou original). Por eso no se puede «pintar» ese archivo con calle: no hay original que modificar. (La idea V6 sobre ese archivo queda descartada hasta nueva evidencia.)
- `turf3dtexlodu4.fmdl` (4,9 MB) sí existe en `common.cpk` (césped 3D por LOD). El Camp Nou lo sobrescribe con AddOn `FD TURF` y `Turf SG4`.

## 3. Cómo extraer del cpk del juego (sin herramientas nuevas)
1. Listas ya hechas: `PRUEBA_CANCHA\informes\datos\cpk\*.txt` (18 cpk) y las de los demás: `node PRUEBA_CANCHA\scripts\cpk_lista.js <cpk> <salida.txt>` (solo lectura, formato ruta/tamaño/tamaño comprimido/posición).
2. Un cpk dentro de otro (ej. `Asset/model/bg/st002.cpk` dentro de `Data\dt30_g4.cpk`, posición 244013056, tamaño 190817712): copiar esos bytes a un archivo aparte (Python, lectura por trozos) y volver a pasar `cpk_lista.js`.
3. Extraer archivos: `FootballLife-new-leagues\tools\cpkread.py` (`extract(cpk, destino, partes)`).

## 4. Formato FTEX (lo medido en pitch_alp original del Camp Nou)
- Cabecera de 64 bytes; luego tabla de mips (16 bytes por mip: offset, tamaño sin comprimir, tamaño comprimido, índice, nº ftexs, nº de trozos). Los mips van del más pequeño al más grande en el archivo.
- Cada mip se parte en trozos; cada trozo tiene una entrada de 8 bytes `<u16 tamaño_comprimido, u16 tamaño_sin_comprimir (0 = 65536), u32 offset_dentro_del_mip>` seguida de datos zlib (si comprimido == sin comprimir, está guardado crudo).
- `pitch_alp.ftex` original: BC3/DXT5 4096x2048, 11 mips, v2.04, 5.477.446 bytes, `ftexs=0`, **RGB negro** y alfa solo en las líneas. `FtexTool` falla leyendo estos archivos (cabecera 2.04): hay que decodificar a mano.

## 5. Cómo se hizo V5 (`diagnostico\variantes\hacer_v5.py`)
Objetivo: descartar que nuestro propio `ftex` se decodifique mal. Se parte del `pitch_alp` original bit a bit y solo se cambian los bloques BC3 de la mitad derecha (x ≥ 2048) en cada mip por la calle oscura (alfa 0, de `V3_oscura`). Cada trozo se recomprime con zlib nivel 9 y se rellena con ceros hasta el tamaño original del mip, de modo que **tamaño de archivo, cabecera, tabla de mips y offsets quedan idénticos** (`leer_ftex` igual al original). Mitad izquierda = original intacto (comprobado en mips 0, 3 y 6).

## 6. Variantes hechas (todas en `diagnostico\variantes\`)
- `V2_*`: alfa 255. `V3_oscura`: calle al 18 % y alfa 0. `V3_oscura+suelo`: V3 + quad de suelo añadido a la malla de líneas (hacer_suelo.py). `V5_mitades`: ver arriba. Ninguna se probó con el 9901 de local; las pruebas de 01:11 y 01:23 usaron Camp Nou, por eso no cambiaron nada.

## 7. Pendiente
Prueba con PES: entrenamiento libre, elegir la Calle, revisar `peticiones.log` (qué pide y de dónde). Para compartir con amigos, recortar `completo_total` a lo que el juego pide (302 archivos, 63,7 MB en la carga de las 01:23).

## 8. Hallazgos del 6-nov (tarde) — el blanco, V0, V7a y V9
- **El blanco = hueco de var_AB.** El pitch_st002.fpk «chico» de var_AB encogía la malla del césped (st081_pitch_0) a ±15,6 × ±10,5 m; fuera de ese rectángulo no había suelo. **V7a** (`v7_piso_completo\hacer_v7a.py`): fmdl ORIGINAL del Camp Nou (st081_pitch_0 completo, ±54,7 × ±36 m) + st081_pitch_1 (líneas) tomada de var_AB; misma estructura FMDL (validó). Instalado en `Phoenix\Calle 30x20` y en `content\phoenix\cancha_visual\...\pitch_st002.fpk` (este último es el que phoenix_marcador_free copia sobre el de España\Spotify Camp Nou al entrar al entrenamiento).
- **pitch_alp.ftex es un mapa de DATOS, no un color.** Decodificado con texconv (V8_diag\diag.py): mediana R≈129, G≈36, B=255 en la mayoría, alfa 0 salvo 61.529 px (líneas, máx 232). Con RGB casi negro (V2/V3/V5) el shader pes3DDF_Pitch2020 se quema a blanco. Se deja el ORIGINAL (`completo\pitch_alp_V0_original_CampNou.ftex`). El codificador BC3 no es el culpable: la mitad izquierda de V5 (bloques originales) salía verde y la derecha (nuestro BC3, idéntico byte a byte al de texconv) blanca.
- **Captura de éxito (07:26):** campo entero verde con líneas 30x20 limpias (V0 + V7a).
- **V9 (asfalto), `v9\hacer_v9.py`:** reconstruye `st002_turf001_bsm` (BC1), `_nrm` (BC3, DXT5nm casi plano) y `_srm` (BC1, mate), 1024x1024, 9 mips, con la MISMA cabecera, nº de mips y de trozos por mip que los originales. Se reescribe la tabla de mips y los offsets (de la mip pequeña a la grande, a continuación de la tabla de 16 bytes por mip); datos zlib nivel 9 (crudo si no comprime). Tile sin costuras (ruido periódico). Salida: `v9\Calle 30x20\Asset\model\bg\st002\sourceimages\tga\#windx11\`. Vista previa plana: `v9\vista_previa.png`. lym/lbm/sb_nrm se dejan originales. Sin instalar hasta la aprobación de Frank.
- **Límite del overlay de Sider 7.3.4: 2048 bytes** (hipótesis fuerte, no confirmada en el código de Sider): el panel de phoenix_marcador_free llegó a 2138 bytes con 37 opciones y el overlay solo pintó la cabecera. Solución instalada: páginas de 16 opciones (la página sigue a la opción elegida con 8/9/F10), cabecera corta, guarda a 1900 bytes. Respaldo: `respaldos\instalacion-20261106-020030`.
- **Briznas 3D (turf3dtexlodu4.fmdl):** si se ven verdes sobre el asfalto, método propuesto SIN fmdl: poner `grassfin_bsm_alp.ftex` (textura de las briznas, hoy de Super-Global) en gris dentro de un AddOn «Global» propio de la Calle (`Calle 30x20\AddOn\<mod>\Global\Asset\model\bg\common\turf3d\sourceimages\#windx11\`) y darlo de alta en el addon_config.ini de la Calle; el AddOn Global tiene prioridad sobre Super-Global (StadiumServer.lua, check_for_addon_override), así que no afecta a otros estadios.
- **turf_bsm_alp_rgba32.ftex (WESYS):** 16 bytes de cabecera (`00 10 01 57 'ESYS'`, u32 = tamaño del flujo = tamaño del archivo − 16, u32 = tamaño descomprimido) + flujo zlib que contiene un FTEX normal (944.969 bytes). Se puede desenvolver, editar y volver a envolver; no se tocó porque en las cargas de entrenamiento no se pide.
