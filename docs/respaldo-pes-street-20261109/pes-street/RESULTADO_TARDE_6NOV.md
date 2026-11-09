# Resultado de la tarde del 6-nov-2026 (Street / Calle 30x20)

Escrito a las 17:55 (hora PC) por la sesión nueva de Claude Code (arrancó 15:06). Complementa `ESTADO_SESION_ANTERIOR.md` (sesión anterior, 16:15) y `INFORME_NOCHE_6NOV.md`.

## 1. Qué hay instalado AHORA en `SiderAddons\content\stadium-server\Phoenix\Calle 30x20\` (solo Calle)

| Qué | Archivo | md5 | Estado |
|---|---|---|---|
| Suelo v24b + reja de 6 m + anillo opaco + red sobre el arco DETRÁS del fondo | `Asset\model\bg\st002\pitch\#Win\pitch_st002.fpk` | `626ff4f7b7d448d42b208b63f7b22928` | Instalado 17:43. Visto en juego (captura arco izq.). |
| Stubs vacíos de publicidad (`ad_st002_normal`, `_cl`, `_el`; material `phoenix_stub_nulo`) | `Asset\model\bg\common\ad\#Win\` | normal `0b98713c25cc8fd06bf2a01594474861` | Instalado 17:43. En partido no se ven cintas; la cámara lejana con ellos puestos AÚN NO verificada. |
| Estadio sin gradas ni techo (pack_B2) | `Asset\model\bg\st002\#Win\st002.fpk` | `6d08696a382ea5ed198bada25ea3da6e` | Instalado (sesión anterior). |
| Línea/atlas del suelo v24b | `...\common\pitch\sourceimages\#windx11\line_alp.ftex` | `1bc1c77a33b8b181be9c3013e5442b1f` | Instalado (sesión anterior). |
| Stub de la demo de presentación | `common\demo\fixdemoobj\st002_bg\#Win\st002_h_all(.choreo).fpk` | — | Instalado (sesión anterior). |
| 6 stubs de audi | `Asset\model\bg\common\audi\#Win\` | — | Instalado 16:06 (sesión anterior). |
| `addon_config.ini` | `AddOn\addon_config.ini` | — | Activos: StadiumScreen_UCL, FD TURF, Light, Goal Post Colour, Light SG7. **Exterior_Aerial_CampNou26 desactivado** (solo contenía cámaras aéreas y light_config de demos; no dibujaba cielo ni asientos). |

Respaldos de lo reemplazado hoy:
- `herramientas\Blender279\respaldos\v24-antes-v26` (pitch v24 antes del anillo)
- `...\respaldos\v26a-antes-nivel` (anillo sin reja nivelada)
- `...\respaldos\v26b-antes-arco-ad` (pitch + ad originales de Calle antes del arco y los stubs de publicidad)
- AddOn: `AddOn\addon_config.ini.antes-sin-exterior` (con Exterior_Aerial) y `.antes-lightSG7`.

## 2. Preparado pero NO instalado
- **Reja sin hueco lateral junto a los postes**: GAP 2,4 → 1,6 m en `reja.py` (red de 6 m hasta los postes). Build en `herramientas\Blender279\calle_30x20\v26_anillo\Calle 30x20\...\pitch_st002.fpk`, md5 `5da38b77678cdba483bfadd37ea58889`. Sin probar en juego.
- `v26_anillo\extra_df`: `st002_df.fpk` con `center2_df` vaciado (el original pesa solo 26 KB; probablemente no son las cintas).

## 3. Comprobado en juego hoy (capturas en `pes-street\capturas\`)
- Entrenamiento libre 3v3 con arquero fijo: `sin_exterior_aerial_OK_1520.png` (sin asientos), `core0519_v24b_reja_arco_1627.png`, `v26_anillo_juego_reja_1638.png` (reja lado cámara, jugador contra la reja sin atravesarla), `arco_izq_nuevo_core0520_1748.png` (red detrás, boca libre), `juego_reja_fondo_core0520_1749.png`.
- Cámara lejana / presentación 0:00: `lejana_presentacion_v26_nivel_1731.png` — cielo azul con nubes, SIN asientos, gradas ni techo; sobre la cancha suelo gris oscuro y pasto verde alrededor.
- Celebración de gol: `celebracion_gol_rival_v26_1731.png` (cielo azul de fondo).
- Log del Core: el balón rebotó en el techo de 6 m (≥29 rebotes) y los jugadores empujados contra la reja no la atraviesan (`no_mantenidas=0`).
- Panel final de Frank: «Final del partido» + Revancha funciona (captura de Frank).

## 4. Lo que falta
1. Cámara lejana: **cintas de publicidad** (FC BARCELONA / MÉS QUE UN CLUB, SPOTIFY CAMP NOU) — sospecha: `common\ad\ad_st002_*` (ya vaciadas, falta ver la cámara lejana); **puntos en filas** (¿audi/luces?); **rectángulo oscuro** semitransparente arriba (¿tv_st002 / StadiumScreen_UCL?); **pasto verde** alrededor (hay que agrandar el anillo opaco).
2. Instalar la reja sin hueco lateral (5da38b77…) y mirar los dos arcos.
3. Balón alto (techo a 40 m desde 0.5.20): no pude lanzar un globo con el teclado; lo prueba Frank con el mando.
4. FUGA del balón por la boca del arco (a x≈−19,7 salió de los muros a v≈−21 en un tiro fuerte): del Core (0.5.21 la corrige).
5. Público animado detrás de la reja (audiarea con REA): sin empezar. Hallazgo previo: el juego arma `audi_area.xml`, `audi_normal.xml`, `audiarea_*.fpk`; función cargadora sin localizar.
6. Presentación 0:00 con Revancha para ver el arco derecho.

## 5. Cosas a tener en cuenta
- Ruta de arranque: Día de Partido → bajar 12 → Entrenamiento → bajar 1 (Entrenamiento libre) → Enter×6; PT fijo Sí, usuario 3, COM 3. El mosaico «Entrenamiento» del menú principal NO sirve. Teclas del MCP con 1,5–2,5 s entre pulsaciones; si PES «no está al frente» el MCP se para (se recupera repitiendo).
- No cerrar PES con menos de 60 s abierto (una vez, por orden cruzado, se cerró a los 48 s sin daño).
- Un hilo anterior de Claude Code (PID 23776, resume 9d834472…) siguió trabajando tras desconectarlo; escribió `ESTADO_SESION_ANTERIOR.md` y terminó.
- El anillo opaco va en malla aparte (`st081_pitch_2`, material `phoenix_anillo_opaco`, fox3ddf_ggx/fox3DDF_GGX_LNM, alpha_flags 0). No meter celdas exteriores en el material de mezcla (fox3DDC_GGX, alpha_flags 80): pinta sobre los muros y causa triángulos grises.

## 6. Cómo volver atrás
Con PES cerrado, copiar de vuelta los archivos de `respaldos\v26b-antes-arco-ad` (pitch y ad) a `...\Calle 30x20\Asset\model\bg\st002\pitch\#Win\` y `...\common\ad\#Win\`.

## Herramientas REA usadas
Ninguna hoy (no se llegó al punto del público). Se usó Blender 2.79b + pes-fmdl y GzsTool para construir los paquetes.
