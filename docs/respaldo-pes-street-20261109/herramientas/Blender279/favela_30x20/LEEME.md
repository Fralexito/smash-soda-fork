# Favela 30x20 — diseño y método (6-nov-2026)
**Diseño** (31,2 × 21,3 m): suelo cemento azul claro desgastado (pintura caída, grietas, manchas) con áreas de arco y franja exterior amarillas;
líneas blancas (borde, mitad, círculo, áreas, puntos); muro bajo 1,1 m azul fuerte con franja amarilla, descascarado y grafitis; reja ciclónica verde
oscuro con óxido hasta 3,3 m, postes de tubo cada ≤3 m, 2 tensores y riel superior; huecos de 5,2 m centrados en los lados cortos para arcos 4x2. Todo procedural, sin terceros.
**Método elegido (funcionó en el validador y la reimportación):** la pieza que ya carga es la malla `st081_pitch_1` (líneas, 8521 v) del `pitch_st002.fmdl`.
Se mantiene el FMDL idéntico (nodos, tabla de secciones, material fox3DDC_GGX con sus parámetros, flags alpha 80) y solo se reemplaza su geometría por
lineas + muro + reja (528 v) y sus 4 texturas por favela_bsm/nrm/srm/mtl (la reja usa alfa como las redes). `hacer_favela.py` → `paquete\`.
No hizo falta añadir mallas nuevas (la segunda opción queda como plan B).
**Texturas hermanas (hallazgo clave):** StadiumServer redirige TODO asset\model\bg\st002 y \common a la carpeta del estadio sin comprobar existencia;
el Camp Nou las trae (4 de st002 + 10 de common\pitch vía el AddOn FD TURF). Se copian al paquete (`completar.py`).
**Suelos:** `suelos\pitch_alp_FAVELA_azul_alfa255.ftex` y `pitch_alp_CALLE_oscura_alfa255.ftex` (cabecera 2.03 pura, bandera ftexs=0).
**Vistas:** vista_3_4.png, vista_aerea.png, vista_nivel_suelo.png (previews Blender; no es el render del juego). 3D: visor\cancha_favela.obj (+mtl, png).
**Ojo:** el alfa 255 del suelo (hipótesis V2) no corrigió el blanco de la Calle; ver calle_30x20\completo. Colisión/rebote NO sale de aquí (Core: cancha_paredes / hit_billboard.geom).
