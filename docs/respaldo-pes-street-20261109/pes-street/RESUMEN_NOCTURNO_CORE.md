# Resumen nocturno del Core (6-nov-2026, ~05:05 hora PC)

Estado honesto: **solo la 0.5.9 está instalada** y **ninguna versión de esta noche se ha visto cargar en un arranque real** (no pude lanzar PES: el sistema de permisos denegó «PES2021 Start.exe» y no lo rodeé). Todo lo demás está compilado, NO instalado.

## Qué hay instalado ahora (SiderAddons\phoenix_core)
- `phoenix_core.dll` = 0.5.9 (508 928 B). Respaldo: `phoenix_core.dll.pre-059` (0.5.8).
- `phoenix_core.json`: añadidos `"gk_limite_area": true`, `"gk_area_radio": 5`. Respaldo `.json.pre-059`. Antes (0.5.8): `ia_presion_k` 2.0.
- Panel F9 (`modules\phoenix_marcador_free.lua`): 2 líneas tras `arquero_controla` (gk_limite_area, gk_area_radio). Respaldo `.lua.pre-059`.

## DLLs compiladas SIN instalar (build\Release de C:\dev\phoenix-core-wt\paquetes\phoenix_core)
Cada una incluye todas las anteriores (se instala UNA, la más nueva que se quiera probar; para ir de a una función, instalar en orden y comprobar el log tras cada arranque).

| DLL | Versión | Qué cambia | Casilla json (por defecto) |
|---|---|---|---|
| `phoenix_core_0510.dll` | 0.5.10 (E2) | Escala `margin_restartX` (BasePosition +0x24C) y `kickoffDfLineX` (+0x214) como eje x de la minicancha, para que en el saque de arco no esperen tras una línea de cancha grande. **Hipótesis, sin verificar.** Registra los valores originales en `[IA_DATOS] BasePosition_saque`. | `saque_margen_chica` (true) |
| `phoenix_core_0511.dll` | 0.5.11 (E3) | Factor EXTRA solo sobre los radios de «encarar»: Defence +0x20, DefenceMark +0x8C, Matchup +0x2C (encima de `ia_presion_k`). 1.0 = apagado. | `ia_encarar_k` (1.3, rango 0.8–2.0) |
| `phoenix_core_0512.dll` | 0.5.12 (E5 parcial) | Las 2 llamadas a funciones por RVA fija (0x8AC9D0 y 0xA48A30) solo se ejecutan si el exe es el conocido (fecha 0x5F5ED4BF / checksum 0x1B59007B). Con otro exe: aviso `[RVA_FIJAS]` en el log y esa rama se omite. Código: `exeConocidoRva()`. | (automático) |

| `phoenix_core_0513.dll` | 0.5.13 (E6, pedido del hilo Cancha) | Orden `street_fin_pedido`: lanza el final nativo con revancha (mismo camino que el final por reloj de `hiloDosTiempos`: aviso, pausa 8 s, estadísticas y revancha desde 0:00) cuando se pone a 1 y Street está activo. Se puede pedir de 2 formas: (a) clave `"street_fin_pedido": 1` (o true) en `phoenix_core.json`, que el Core lee cada ~1 s desde `procesarOrdenes` y **pone a 0 al atenderla**; (b) línea `street_fin_pedido=1` en `phoenix_core\ordenes.txt` (canal que ya usa el panel; el Core borra el archivo al leerlo). Registra `[FIN_PEDIDO]` (atendido / ignorado con motivo: Street apagado, ya en final o fase 2). Recomendado para Lua: usar `ordenes.txt` (evita carreras al escribir el json). Sin verificar. | (orden, sin casilla) |

| `phoenix_core_0514.dll` | 0.5.14 | `[GK_AREA_DIAG]` cada 5 s (llamadas al gancho del portero, correcciones, destinos dentro, último destino, máx. distancia al segmento). **INSTALADA 05:46 y verificada por el hilo Cancha (05:47-05:55): modo normal, 0 errores, llamadas=2254, correcciones=0, maxDist=2.2 (<R=5).** | — |
| `phoenix_core_0515.dll` | 0.5.15 | `[BALL_DIAG]` (máx. 12): con 0 candidatos de pelota anota patrón relajado + histograma de +492 + volcado de `raiz+0x2970` (BallInfo); `[SAQUE_ARCO]` ya no gasta líneas sin pelota ni sub válido. **INSTALADA 05:59, a la espera de un arranque de verificación con partida real y saque de arco.** | — |

## Hallazgos de la noche (log del arranque 0.5.14)
- **La localización de la pelota falla**: «[BALL] escaneo: 0 candidatos» durante toda la sesión de 05:47-05:55. Deja ciegos a L1_SIN_PT (pelota_nula), street_saque_trabado y [SAQUE_ARCO] (581/600 líneas con balon=?). Es la prioridad (0.5.15 recoge datos para decidir la 0.5.16: patrón relajado de respaldo o BallInfo `raiz+0x2970`).
- **Esperas largas de reinicio**: [SUB] sub=1 k=7 duró 109 s, 27 s y 67 s con la IA sola y sin goles (posible causa de «esperan tras la mediacancha»; street_saque_trabado no actúa sin pelota).
- **saque_margen_chica**: se aplicó (margin_restartX 6.00→3.21; kickoffDfLineX −18.00→−9.62) pero sin pelota no se pudo medir el efecto.
- **E4 (portero despeja)**: ya hay 4 capas activas y verificadas en el log de arranque: `arquero_saque_corto` (anula GKPassLong y GKClear), `sin_despeje_emergencia` (ThinkUnitClear), `arquero_pase_corto` y `arquero_controla` (todas «activa · ok»). [GK_MODO] siempre da efectivo=0. Por tanto el despeje que ve Frank no viene de GKClear/GKPassLong; queda por identificar con evidencia en partida (candidatos: ActionKeeperPuntKick/Throw tras atrapar, ActionKeeperPunch/Deflect, o un golpe de jugador de campo). No se creó `gk_saca_jugando` porque no hay causa demostrada: sería adivinar. Siguiente paso: registrar qué ActionKeeper* se ejecuta (FUN_1409CE630 PuntKick slot7, FUN_1409D04A0 Throw, FUN_1409D0E80 ThroughThrow, firmas en frente_H §firmas) cuando el balón sale del portero.
- **E5 completo**: no hecho. Hace falta desensamblar las 8 RVAs de datos listadas arriba (fichero del exe en disco o REA); el guard de las 2 llamadas ya está en 0.5.12.

(Actualización 05:59: `saque_margen_chica`, `ia_encarar_k` y `street_fin_pedido`=0 YA están en el json y en el panel, salvo `street_fin_pedido`, que es una orden y no lleva casilla; los .pre-NNN de respaldo existen para cada instalación.) Texto original: las claves nuevas no estaban en el json: el código usa su valor por defecto si no están. Para el panel F9 había que añadir (con copia `.pre-NNN` del lua y partiendo del archivo ACTUAL, que también toca el hilo Cancha): `saque_margen_chica` (bool), `ia_encarar_k` (num, paso 0.1, 0.8–2.0).

Código fuente: `src\phoenix_core.cpp` (acumula 0.5.9→0.5.12). Respaldos: `.antes-056`, `.antes-057`, `.antes-058`, `.antes-059` (antes de E1). `phoenix_core.cpp.antes-0510-ya-con-e2` es 0.5.9 **más** E2 (copia hecha tarde, nombre engañoso).

## Orden de instalación recomendado (con PES cerrado, cierre limpio, y PES >60 s abierto tras cada instalación)
1. Ya instalada: 0.5.9. Verificar en `phoenix_core.log`: cabecera `==== Phoenix Core 0.5.9`, «arranque terminado: modo normal», `arquero_salida_corta` instalada, líneas `[GK_AREA]` en partido. Si cae en modo seguro o falla: copiar `.pre-059` sobre DLL/json/lua.
2. 0.5.10 (copiar DLL actual a `.pre-0510`, poner `phoenix_core_0510.dll` como `phoenix_core.dll`). Comprobar `[IA_DATOS] BasePosition_saque` con los valores originales.
3. 0.5.11 y 4. 0.5.12, igual.

## Etapas NO hechas
- **E4 (portero despeja en vez de salir jugando):** no hecha. Hace falta REA/Ghidra con el exe (rama ThinkUnitGKClear). Dato ya medido: en 712/712 muestras de `[GK_MODO]` el modo efectivo es 0, así que `arquero_controla` no actúa sobre lo que Frank ve.
- **E5 completo:** solo el guard de llamadas. Quedan RVAs fijas de DATOS (lecturas seguras con `leerSeguro`): 0x260FEE8, 0x260FB58, 0x36F3DC8, 0x37F4940, 0x2644FAC, 0x3705E10, 0x28CCCB8 (vtable), 0x37F0A58. Con otro exe leerían basura sin caer, pero las funciones que dependen de ellas (arco real, duración, R3 DirectInput) podrían actuar mal. Convertirlas a firma AOB necesita desensamblar cada sitio (REA).
- **E3, parte «cambiar en caliente rol Atacante/Defensor»:** no hecha (sin datos).

## Qué NO está verificado
Todo: el comportamiento en partido (círculo del portero, saque de arco, presión) y que 0.5.9–0.5.12 carguen en modo normal. Solo se sabe que compilan sin errores.

## Cómo volver atrás
Copiar los `.pre-NNN` correspondientes sobre `phoenix_core.dll`, `phoenix_core.json` y `modules\phoenix_marcador_free.lua`. Cierre limpio de PES; si PES se cierra en menos de 60 s tras abrir, el siguiente arranque entra en MODO SEGURO (se cura solo en el arranque siguiente).

## Actualización 6-nov 17:45 PC (Core 0.5.20 instalada) — hallazgos nuevos para el catálogo
- **Rol del Entrenamiento libre**: TrainingWork+0x14 (0 = Ataque, 1 = Defensa) = 0 en Street ([ROL_IA] 16:36). Con street_rol_ia=true se escribió 1 en caliente y en esa partida el rival jugó sin portero (hipótesis; con street_rol_ia=false el portero rival idx 11 estuvo en la raya todo el rato según [JUGV]). Dejar street_rol_ia=false.
- **Posición REAL del jugador** (rec = [raiz+(idx+0xBF)*0x28]): +0x55C/+0x564 (x/z) se recalculan cada paso desde +0x568/+0x570; las copias +0x5B4/+0x5C0 (x) y +0x5BC/+0x5C8 (z), +0x5F8 las siguen. Escribir solo +0x55C no frena a un jugador. 0.5.20 desplaza todas las copias del bloque 0x540..0x600 que valen x o z ±0.12 (verificar con [JUG_DENTRO_DIAG]).
- **Suplentes aparcados** del Street: z = 40…100 (los marca ganchoFilaBanda); no deben tocarse (0.5.19 los arrastró a la reja).
- **«45 min» en el panel del final**: lo escribe el Core (reloj_visual_2t dato=45 en la 2.ª parte + reloj del job a 0 en el final); 0.5.19d/0.5.20 añaden final_reloj_90 (dato=90 mientras g_descansoFinal). Sin verificar visualmente.
- **Panel del final**: con street_fin_fase=1 sale «Final del partido» (confirmado por Frank): el título «Descanso» dependía de la fase 2 escrita en el job.
- **Muros**: no había altura máxima en la lógica de la pelota. 0.5.20: muro_alto = altura de la reja (6 m, por encima la pelota pasa), muro_techo = techo del rebote vertical (40 m).
- **Valoraciones de jugador en el panel final** (petición de Frank): el catálogo NO documenta dónde calcula/guarda PES la nota por jugador (solo clases online *PositionRating* y MatchPlayerStats); ver plan en el hilo.

## Hallazgos REA 7-nov (exe recortado, Ghidra 12.1.4 vía MCP rea) — final real y penales
- **FUN_14102CAD0 (RVA 0x102CAD0) = `*(uint32*)p1 = p2`**: es un setter PLANO (sin hash). La fase del job (job+0x2790) la escribe el propio juego con él (paso 0x1C). Escribirla con memcpy (como hace el Core) es equivalente: el cierre B7AAA con fase 3/10 NO viene de ese setter.
- **Paso de cola 0x1C = FUN_1404494E0(job, ctx)** (solo si `[[0x1436F3DC8]+0x270]+0x1775 == 0`): si `job+0x49C0 == 7` (árbitro: tiempo cumplido): emite 0x1050056; `job+0x17850=1`; lee la fase `[[0x1436F3DC8]+0x270]+0x1168`: 1→2, 3→FUN_1404493D0(job), 5→6, 7→FUN_1404499D0(job); luego `FUN_14102CAD0(job+0x2790, fase)`, pone a 0 `job+0x2790` vía FUN_140A3CBC0/140A3CD70/140A3CBB0 y FUN_140440F90, llama FUN_1404455F0 y emite 0x105005B; después FUN_1404537B0(job).
- **Regla de empate al final del 2.º tiempo = FUN_1404493D0(job)** (fase 3): compara marcador local (`rules+0x6B` + goles categoría 5) con visita (`rules+0x6A` + …); si hay regla de goles fuera (`rules+0x68`) usa la ponderada; si EMPATA: `rules+0x66 != 0` → **prórroga** (FUN_140443E30(job, rules+0x65), devuelve fase 4); si no, `rules+0x67 != 0` → **penales (devuelve fase 8)**; si no → fase 10 (fin). `rules = [[0x1436F3DC8]+0x2C0]`. Si hay desempate decidido → fase 10.
- Consecuencia para Street: la tanda de penales nativa existe en la misma máquina de fases (fase 8) y depende de los flags `rules+0x66` (prórroga) y `rules+0x67` (penales) — los mismos que Sider expone como `extra_time` y `penalties` en ctx (valen 1 en el ctx de la sonda). Requiere fase 3 (Street usa fase 1) y los objetos de PK (`PkMatch` en job+0x17DA0, pantalla MatchPkOrder): sin verificar que existan/funcionen en Entrenamiento libre.
