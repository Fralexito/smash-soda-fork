# Frente T — 3 tamaños de cancha por jugadores (diseño del lado del Core)

Estado: SOLO DISEÑO (7-nov-2026). Nada implementado. Medidas de partida: `TAMANOS_CANCHA.md` (PC de Cancha). Cambio de Frank: **L = 75×48** (carpeta «Phoenix\Calle 75x48»), arco de L provisional.

## 1. Tabla propuesta por tamaño (semi-medidas desde el centro)
| Campo (hoy) | S (2–3v3) | M (4–6) | L (7–11) | Dónde vive hoy |
|---|---|---|---|---|
| Largo × ancho (líneas) | 30×20 | 42×25 | 75×48 | json `cancha_semilargo/semiancho` → `g_canchaLargo/Ancho` (15/10) |
| Semilargo L / semiancho W | 15 / 10 | 21 / 12.5 | 37.5 / 24 | idem |
| Muro (reja) x / z | 15 / 10 | 21 / 12.5 | 37.5 / 24 | json `muro_x/muro_z`; la reja de jugadores usa muro−0.4 |
| Altura de reja / techo | 6 / 40 | 6 / 40 | 6 / 60 | `muro_alto`, `muro_techo` |
| Arco (boca) | 3×2 (`arco_medida`=2) | 3×2 | 7.32×2.44 real (`arco_medida`=0), provisional | `arco_medida` + tabla T |
| Fondo de red | largo+3.3 | largo+3.3 | largo+3.0 | `red_fondo_m` |
| Área del portero | radio 5 | radio 6 | `gk_limite_area`=false (área grande nativa) | `gk_area_radio`, `g_gkLimiteArea` |
| Salida máx. portero | 3.5 | 4.5 | nativa (`arquero_salida_corta` apagada) | `arquero_salida_max` |
| Piso de las tablas de IA | 1.5 | 2.0 | 3.0 | `ia_piso_m` |
| Factores ia_presion_k/ataque_k/base_k | 2.0 / 1.4 / 1.0 | 1.5 / 1.3 / 1.0 | 1.0 / 1.0 / 1.0 (la escala nativa ya es ≈ real) | json |
| Mínimo de jugadores por equipo | 2 | 4 | 7 | menú (TW+0x3C/+0x40), `street_jugadores_*` |
| Cámara | actual | más lejana | más lejana | lado Cancha |

## 2. Qué es FIJO a 30×20 hoy en el Core
1. `g_canchaLargo = 15.0, g_canchaAncho = 10.0` (l.708) y su validación 8–80 / 6–50 (l.3850); L=37.5/24 cabe.
2. Bloque de 9 floats (l.722–727): f = {L, W, aw·0.5, ah, 47.06·kx, max(4, 9.16·kz), 36.14·kx, max(4.5, 20.12·kz), 41.56·kx} con kx = L/52.56, kz = W/34.06: ya escala con L/W; los dos primeros son L/W y [2]/[3] el arco (solo con `arco_medida`>0). Con `arco_medida`=0 (L real) esos dos NO se escriben (correcto).
3. **Cajas de detección de gol `cajasNuevas` (l.762–768)**: x = ±(largo−0.12) hasta ±(largo+2.94); **z = ±4.35472 y altura 2.903 FIJAS** (vienen del arco original 7.32×2.44 + margen) y la caja del campo `largo, 50, ancho+1.44`. Con boca de 3×2 la caja de gol es MUCHO más ancha (±4.35 vs ±1.5) y más alta (2.9 vs 2.0) que la boca visible: ver §3 «Discrepancia ±2.4».
4. Arco: `hw`/`hh` por defecto 3.66/2.44 y T[4][2] = {7.32×2.44, 4×2, 3×2, 5×2.2}; `anclaZ/anclaY` usan 3.66 y 2.44 (anclas de la red); `ARCO_REAL` p[0..0x12].
5. Portero: segmento de postes ±2.0 **fijo** en `gk_limite_area` (debería ser ±hw), `|z|≤5` fijo en el recorte rectangular, `arquero_salida_max` 3.5, radio 5.
6. IA: `ia_cancha_chica` escala con dims=[raiz+0x2AA8] (d/52.56)^e: se adapta sola; `ia_piso_m` y los factores están afinados a 30×20.
7. `formacionSaque`: fracciones fijas f_def .50 / f_del .17 / default .33 y solo slots 1–4 y 7–10 especiales: para 11 jugadores hay que cubrir los 10 de campo.
8. Reloj/tiempos/suplentes: no dependen del tamaño.

## 3. Discrepancia «arco ±2.4» (pedida por el hilo principal)
- En el CÓDIGO del Core no existe ningún literal ±2.4. Sí existen: boca lógica `hw = aw·0.5 = 1.5` (arco_medida 2; ARCO_REAL p[2]=1.5), reja/hueco visual ±1.6 y la **caja de gol ±4.35 (z) × 2.903 (y) heredada del arco original**. Si el ±2.4 de Cancha viene de otro sitio (p. ej. el arco de la `arco_chico_free` antigua: `g_arcoAncho=4.0` → ±2.0, o 4.35·0.55), no es de esta versión del Core.
- Efecto de la caja ancha: la pelota que rebota en el muro de fondo JUNTO al poste (z entre 1.6 y 4.35, y<2.9) queda dentro de la caja de gol (x ≥ largo−0.12 = 14.88 y el rebote ocurre en 14.89), así que puede contar como gol sin haber entrado por la boca. Pendiente de confirmar con una partida (buscar goles con la pelota en |z|>1.6 en [GOLES]/[MURO]).
- Red de atrás (`red_fondo_m` 3.3) y portero (±2.0 fijo) también deberían derivar de la boca real.
- Propuesta (con on/off): `gol_caja_boca` (true) → `cajasNuevas` usa z = ±(hw + 0.15) y altura hh + 0.1 y la boca de `arco_medida`; el segmento de postes del portero = ±hw; `anclaZ/anclaY` ya reciben hw/h. Probar con 1 turno de PES (tiro al poste y a la reja lateral). No tocar lo instalado hasta entonces.

## 4. Diseño de la solución
- `struct TamanoCancha { const char* nombre; double semiL, semiW, muroX, muroZ; float muroAlto, muroTecho, redFondo, gkRadio, salidaMax, iaPiso, iaPresion, iaAtaque; int arcoMedida; bool gkLimiteArea, salidaCorta; int minJug; }` + `kTam[3] = {S, M, L}`.
- `aplicarTamano(int t)`: asigna los globales de la tabla (salvo claves fijadas a mano con `cancha_tamano_manual`), reparchea los bloques/cajas y escribe `content\phoenix\cancha_tamano.ini` (`tamano=S|M|L`) que `street_auto_estadio` de StadiumServer.lua lee para servir la carpeta.
- Selección: json `cancha_tamano` = auto|S|M|L (fila F9). `auto`: N = max(local, rival) → ≤3 S, 4–6 M, ≥7 L.
- Cuándo: al crear el partido (el hook «[JUGADORES] … al entrar» ya corre por partido y revancha), con `g_rearmaCancha=1` para repachear. NO en caliente.
- **Problema de orden**: el estadio se elige al escoger equipos, antes de que el menú de entrenamiento fije N. Hay que escribir `cancha_tamano.ini` en cuanto cambie N en el menú (el Core ya lo sigue) y que StadiumServer la relea al cargar el estadio. A confirmar con Cancha.
- Riesgos: (1) bloques de dimensiones/cajas se parchean al EMPEZAR el partido: `g_canchaLargo/Ancho` ya deben valer el tamaño nuevo; (2) en L varias funciones «chicas» (`jugadores_dentro_free`, recortes, `arquero_salida_corta`, `gk_limite_area`) deben apagarse; (3) la IA nativa asume 11v11 en L: factores a 1.0; (4) `formacionSaque` para 10 de campo; (5) no tocar fases (B7AAA).

## 5. Esfuerzo y orden
1. Tabla + `aplicarTamano` + fila F9 + `cancha_tamano.ini`: 2–3 h (código, sin PES).
2. `gol_caja_boca` + portero ±hw + repachear en cada partido + apagado de rutas chicas en L: 2–3 h + 1 turno de PES por tamaño (medir `[ARCO_REAL]`, `[JUG_DENTRO]`, `[GK_AREA_DIAG]`, goles junto al poste).
3. Formación 11v11 e IA en L: 2–3 h + prueba larga.
Dependencias: medidas finales (L 75×48 y su arco), cámara (Cancha), cuándo lee StadiumServer el `.ini`.

## 6. No hecho
Nada de código; falta TAMANOS_CANCHA.md actualizado para L=75×48 y el arco de L.

## 7. Tamaño de arco independiente de la cancha (pedido de Frank, 7-nov 14:32)
- Defaults: S = 3×2, M = 5×2, L = 7.32×2.44; cambiable a cualquier medida en cualquier cancha. Fila F9 «Tamaño de arco: auto / 3x2 / 5x2 / 7,32x2,44».
- Implementación: reutilizar `arco_medida` (hoy 0 = 7.32×2.44 original, 1 = 4×2, 2 = 3×2, 3 = 5×2.2) y añadir `arco_tamano` = auto|3x2|5x2|real; `auto` lo deduce del tamaño de cancha (S→2, M→3 [5×2.2 o añadir 5×2 exacto a la tabla T], L→0). Todos los consumidores ya salen de `medidaWH()`: boca lógica/ARCO_REAL, `anclaZ/anclaY`, bloque de 9 floats ([2],[3]), y desde la 0.5.24 la caja de gol (`gol_caja_boca`) y el segmento de postes del portero. Hay que pasar también por `medidaWH` la red (`red_fondo_m`, que es fondo y no ancho) y el hueco de la reja (lado Cancha: el arco visual 3x2/5x2/real son los `arco_3x2/4x2/5x22` de `cancha_visual`).
- Con `arco_medida`=0 (real) el bloque de 9 floats NO escribe [2],[3] (queda el original 7.32×2.44) y las cajas de gol usan z=±(3.66+0.15), alto 2.54: correcto.
- La 0.5.24 (gol_caja_boca) lee la boca de `arco_medida` ACTUAL: no necesita más cambios para esta petición; el valor `auto` y la fila F9 van con el frente T.
