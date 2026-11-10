# Divisiones en línea y myClub extra: lo que dice PESBUL y lo que sabemos nosotros (10-nov-2026)

Fuente: lectura de solo lectura de la copia local de PESBUL (`D:\Frank\IA\CLAUDE\pesbul_local\app`), sin ejecutar nada ni copiar código, y nuestras capturas reales del PES base (`phoenix_online\capturas`). Todo con palabras nuestras.
Marcas: **L** = leído en PESBUL (con «PROVEN/MEASURED»: sale de ingeniería inversa del cliente o de capturas reales), **C** = comprobado por nosotros con el PES base, **P** = hipótesis o dato que falta.
Complementa `ESPEC_PARTIDO_REAL_PESBUL.md` (flujo del partido) y `ESPEC_MYCLUB.md` (myClub ya implementado).

---
# PARTE A. Progreso de Divisiones en línea (OC_RANK, 1 contra 1)

## A.0 Reglas por defecto de PESBUL (política de PESBUL, NO regla recuperada de Konami; todo editable)
- **Divisiones:** D12 (la más baja; todo jugador nuevo empieza ahí) a D1 (la más alta). Temporada = **10 partidos** en todas.
- **Puntos de temporada (SP) por partido** = `SP del equipo × multiplicador del resultado`; multiplicadores: victoria 1,00, empate 0,33, derrota 0,00; redondeo al más cercano (0,5 sube). El «SP del equipo» (Team SP Reward) sale de una tabla por equipo (300 por defecto si no hay dato; semilla por estrellas: 5★=300, 4,5★=425, 4★=525, 3,5★=625, 3★=750, 2,5★=825, 2★=900, 1,5★=975, 1★=1050: a menos estrellas, más puntos).
- **Umbrales (SP al final de la temporada):**

| División | Se queda si SP ≥ | Asciende si SP ≥ |
|---|---|---|
| D12 | (no desciende) | 300 |
| D11 | 300 | 800 |
| D10 | 500 | 1200 |
| D9 | 800 | 1650 |
| D8 | 1000 | 2000 |
| D7 | 1500 | 2500 |
| D6 | 2000 | 3000 |
| D5 | 2500 | 3500 |
| D4 | 3000 | 4000 |
| D3 | 3500 | 4500 |
| D2 | 4000 | 5000 |
| D1 | 4500 | (no asciende) |

  Regla: SP < «se queda» → desciende una división; «se queda» ≤ SP < «asciende» → se queda; SP ≥ «asciende» → asciende una. El ascenso doble existe pero está apagado.
- **Fin de temporada:** al cerrar (comando `CMD_FINISH_SEASON`) se archiva la temporada (SP, W/D/L, rating final, resultado), se aplica el ascenso/descenso, el contador de partidos vuelve a 0 y empieza la siguiente. El **rating personal NO se reinicia**. El menú del cliente deja pasar a la siguiente temporada en cuanto se ha ganado el ascenso aunque no se hayan jugado los 10; el servidor no avanza una temporada vacía ni una sin terminar que no haya ascendido.
- **Rating (Elo personal):** inicial 1000, escala 400, K = 32, mínimo 0; resultado 1 / 0,5 / 0. Nuevo = viejo + K·(resultado − esperado), redondeado al entero más cercano; el rival recibe el espejo.
- **Cortesía («manner»):** token `AA` por defecto (valores `AA, A, B, C, D`); si va vacío el cliente pinta «-».

## A.1 CMD_SEND_GAME_RESULT (L)
- **Lo mandan los dos clientes** al terminar. **Petición:** `side` (HOME/AWAY del que reporta), `home_score`, `away_score`, `home_team_id`, `away_team_id`, `game_phase` (p. ej. `2ND_30MIN`) y `abnormalend_reason`. No trae identificador de partido: el servidor lo liga con la sesión autenticada y el par registrado al emparejar.
- **Respuesta:** solo sobre `{msgid, rqid, result NOERR}`.
- **Qué hace el servidor:** el **primer** informe completo aplica a **los dos jugadores a la vez**: rating, SP, victorias/empates/derrotas, racha de victorias (y máxima), historial, equipos usados recientemente (5) y récords de carrera. El **segundo** informe se compara con el primero (si los marcadores difieren se anota `second_report_mismatch`) pero **no se vuelve a aplicar**.
- **Resultado desde el marcador:** según el lado del que reporta: tus goles > rival = WIN, < = LOSS, igual = DRAW. Equipo propio y rival salen de `home/away_team_id` según el lado.
- **Fin normal o anormal (C/L):** `abnormalend_reason` vacío, `NotImplement`, `NOERR` o `NORMAL_END` = normal (un partido completo trae `NOERR`; PESBUL tuvo un error que descartaba todas las victorias por esto). Cualquier otro valor es fin anormal. Medidos en archivos reales: `CMD_LACK`, `OUTPLAY_LONG`, `LOAD_TIMEOUT`, `SYNC_FAILED`, `NO_OPERATION`, `DISCONN_P2P`, `SLOWDOWN`. El cliente tiene una enumeración de 28 motivos; el primero es `NORMAL_END`.

## A.2 Abandono, desconexión y resultados que no coinciden (L, política de PESBUL)
- **Salida limpia durante el partido (CLEAN_LEAVE):** si un jugador sale con `CMD_LEAVE_ROOM` **con el partido en juego** (se comprueba por la primera actividad del canal 9) es **abandono**: pierde el que sale, gana el otro **3-0** (se conservan los goles del superviviente con mínimo 3 y se borran los del que sale). Pasa por el mismo cálculo normal (rating, SP, historial); sin castigo extra de Elo ni de cortesía.
- **Fin anormal reportado por el cliente:** con la política `no_count` (por defecto) un partido con fin anormal y marcador 0-0 **no cuenta** para nadie. Con marcador real distinto de 0-0, PESBUL espera hasta 180 s al informe del otro y decide: si el motivo es `NO_OPERATION` (el que reporta dejó de tocar los mandos) **pierde quien lo reporta**; `CMD_LACK`/`OUTPLAY_LONG` (se quedó esperando al otro) acusan al otro solo si hay marca clara. Todo lo ambiguo (`DISCONN_*`, `SYNC_FAILED`, `LOAD_TIMEOUT`), ambos con motivo de abandono, marcadores distintos o solo una marca de superviviente = **no cuenta** (no se adivina). En archivos reales los marcadores de los dos clientes coincidieron siempre.
- **Si los dos resultados no coinciden:** manda el primero y se anota la discrepancia; en decisiones de abandono, marcadores distintos → no cuenta.
- La victoria 3-0 por abandono y la espera de 180 s son decisiones de PESBUL (no de Konami): sirven pero podemos cambiarlas.

## A.3 CMD_REQ_GAMEADDPOINT y CMD_WATCH_GAMEADDPOINT (L, medido con el cliente)
- `CMD_REQ_GAMEADDPOINT`: lo manda el cliente al acabar; PESBUL contesta solo con sobre `NOERR` y la usa como marca de fin de partido.
- `CMD_WATCH_GAMEADDPOINT`: **el cliente NO la pide; la tiene que EMPUJAR el servidor** (suscripción por nombre, `rqid 0`), **una vez por participante que siga conectado**, justo después de aplicar el resultado. Es lo único que arma la pantalla «Cambios en el rating»; sin ella esa pantalla no sale. Un participante que ya se fue no recibe nada, y no se repite (se anota por partido y jugador).
- **Contenido** (nombres del parser del cliente): `{msgid 'CMD_WATCH_GAMEADDPOINT', rqid 0, result 'NOERR' (obligatorio), player_count N, player_list [N entradas, HOME primero], winner_side 'WINNER_HOME'|'WINNER_AWAY'|'WINNER_NONE', gameend_reason 'NORMAL_END', is_pk 'NO'}`. Cada entrada: `pid` (entero), `pes_pname` (bin con la identidad `{name, xuid_one 'NotImplement', psn_user_id bin{account_id 0, online_id nombre, is_valid_cache 'NO'}, is_deleted 'NO'}`), `rating_type 'RANKMATCH'`, `rating` (el NUEVO), `oldrating` (el ANTERIOR), `rating_boost_type ''`, `rating_boost_rate 0`, `rating_compe 0`, `team_id`, `side 'HOME'|'AWAY'`, `contWin_num` (racha actual), `skill_rank 'SKILL_RANK_NONE'`, `is_rankup_skill_rank 'NO'`, `is_first_skillrank 'NO'`. La diferencia (delta) **no se manda**: el cliente la calcula con los dos ratings.

## A.4 CMD_GET_DIVISION_INFO (L: contrato recuperado del cliente)
- **Petición:** incluye `is_season_end` (YES/NO). **Respuesta exacta** (las anidaciones van como **bin** de un msgpack, no como mapas directos):
```
{ msgid, rqid, result 'NOERR',
  pname:         bin(packb({name:str, xuid_one:str, psn_user_id: bin(packb({account_id:int64, online_id:str, is_valid_cache:str})), is_deleted:str})),
  division_info: bin(packb({
      division: str                ('D12'..'D1'),
      demotion_point: int          (0 si no hay descenso o es D12),
      promotion_point: int         (99999 si no hay ascenso o es D1: «deshabilitado»),
      doublerank_promotion_point: int (99999 = sin ascenso doble),
      other_match_num: int         (número del próximo partido 1..10 con la temporada abierta; 0 = temporada terminada),
      match_record: bin(packb({win_count, draw_count, lose_count, win_s_max, rating})),
      match_history_list: [ bin(packb({division_point:int, enemy_team_id:int, home_score:int, away_score:int,
                              enemy_side:str 'HOME'|'AWAY', enemy_name:str(≤32), is_finished_match:str 'YES'|'NO',
                              win_lose:str 'WIN'|'LOSS'|'DRAW', enemy_cpu_level:str})), ... ] })) }
```
  - Valores para un jugador nuevo: `D12`, `demotion_point 0`, `promotion_point 300`, `doublerank_promotion_point 99999`, `other_match_num 1`, récord a 0 con `rating 1000`, historial vacío (un array vacío es válido).
  - El historial lleva los **últimos 20** partidos más un elemento «acarreo» para que la suma de SP que calcula el cliente siga bien. `division_point` de cada entrada = SP ganados en ese partido.
  - No existen aquí `is_season_end`, `division`, `pre_season_rank` ni `division_change_type` (se probaron y se rechazaron).
  - `is_finished_match` y `enemy_cpu_level` son **texto** aunque el nombre sugiera booleano/entero.
  - `other_match_num == 0` es lo que hace que el cliente muestre «fin de temporada».

## A.5 CMD_GET_DIVISION_POINT_INFO (L: parser recuperado)
- **Petición:** sin campos propios. **Respuesta:** `{msgid, rqid, result 'NOERR', server_num 1, division_list [12 bins]}`; cada elemento es `bin(packb({division_type:str 'D12'.., point:int, rank:int}))`.
- Es dato de **carrera** (récords «mejores puntos y mejor posición por división»), no de la temporada actual. **Siempre doce filas, de D12 a D1** (la tabla del cliente es posicional: fila i = elemento i; con una sola, las demás quedan en blanco y la primera se pinta «División 0»). `point` = mejor SP en esa división (en la actual, el máximo entre el récord y los SP en curso); `rank` = mejor puesto, **0 si no hay**.

## A.6 CMD_GET_GAMERESULTS (L: parser recuperado; delicado)
- **Respuesta:** `{msgid, rqid, result 'NOERR', rlist_num N, rlist [N bins]}`; sin historial: `rlist_num 0`, `rlist []` y `no_match 'YES'`. **Máximo 20 resultados**, el más reciente primero, y el total debe caber en un marco (~0x410 B cifrados): PESBUL descarta las filas que no caben.
- **Cada elemento** (bin de msgpack; el parser declara 33 claves): `date` (**entero UNIX en segundos**; sin él el cliente enseña 1/1/1970), `match_category 'ONLINE_DIVISIONS'`, `ex_type 'NONE'`, `gameend_reason` (token de la enumeración de 28; `NORMAL_END` en partidos normales; «NORMAL» no existe), `pk_flag 'NO'`, `score` = **array de dos objetos** `[HOME, AWAY]` (cada uno con `score_1st`, `score_2nd`, `score_ex1st`, `score_ex2nd`, `score_pk`; total mostrado = 1.ª + 2.ª + ex1 + ex2, todos ≥ 0), `player` = array de dos jugadores (**HOME primero**) con `pname` (bin identidad), `pid`, `side`, `is_com 'NO'`, `cpu_level`, `is_follow 'NO'`, `is_guest 'NO'`, y **`team` como array a nivel de elemento** `[id_HOME, id_AWAY]` con el id **crudo** (el cliente lo desplaza <<14).
- El WIN/LOSS/DRAW lo calcula el cliente comparando totales y mirando su propio pid; **no hay campo de resultado**.
- **Aviso:** un elemento con claves en el nivel equivocado llegó a **tumbar el cliente**; hay que respetar exactamente este anidamiento.

## A.7 Ranking de Divisiones (L; sirve de analogía para B.4)
`CMD_GET_RANKING` / `CMD_GET_DIVISION_RANKING` (+`_BYID`): `{msgid, rqid, result, date (unix), total_rank_num, rank_list}`; cada elemento es un **mapa directo** (no bin): `{rank (1..), point (SP de temporada), user:{pid, pname:{name, xuid_one, psn_user_id:{account_id, online_id, is_valid_cache}, is_deleted}, division, manner, favoriteTeam, favoritePlayer, intro, match_record:{win_count, draw_count, lose_count, win_s_max, rating}}}`. Orden: rating descendente, luego SP, luego pid. Offset base cero; máximo 100 por respuesta; la petición trae `division` (filtro), `start_index` y `view_num`.

## A.8 Qué guarda el servidor (L)
Por jugador: división, rating, temporada (número, SP, partidos jugados, W/D/L, racha y racha máxima, historial completo), equipo último y 5 recientes, carrera (mejor SP y mejor puesto por división, temporadas archivadas, últimas 200), cortesía. Más un registro **solo de añadir** del historial para reconstruir si se pierde el archivo. Los umbrales se «congelan» al empezar cada temporada (los cambios valen para temporadas nuevas salvo que se pida aplicarlos ya).

## A.9 Sin comprobar (P)
Los umbrales reales de Konami (los de arriba son de PESBUL, calibrados en pantalla en D12); el significado de `enemy_cpu_level` y de los tokens de `ex_type`/`match_category`; la fórmula de ranking real de Konami (PESBUL ordena por rating y luego SP).

---
# PARTE B. myClub extra (ingeniería inversa del cliente, solo lectura, 10-nov-2026)

**PESBUL no implementa nada de myClub.** Todo lo de esta parte sale de leer el **ejecutable del PES base** (proyecto de Ghidra ya analizado `PES2021_recortado.exe`, mismas direcciones que el exe original) con un script de solo lectura: para cada comando se localizaron las clases `Cmd…Option` (petición) y `Cmd…Result` (respuesta), y de su tabla virtual se descompilaron el serializador (petición) y el deserializador (respuesta, ranura 7). Marcas: **L** = leído directamente del código del cliente; **C** = confirmado con una captura real; **P** = inferido (se dice cómo).
No se modificó el exe ni el proyecto (`-readOnly`).

## B.0 Cómo leer los parsers (convenciones del cliente)
- **Sobre de petición** de los mensajes msgpack (TCP, comandos de «tipo mapa»): `msgid`, `rqid`, `pid`, `svrtype`, `client` + los campos propios del comando (el mapa lleva `5 + n` entradas; la cabecera del mapa lo confirma).
- **Respuesta:** `msgid`, `rqid`, `result` + campos propios. Un subobjeto que el cliente abre con su desempaquetador interno de «subflujo» es un **bin(packb(...))**; un subobjeto leído sin él es un **mapa directo**. Esa distinción se dedujo de los puntos donde el deserializador llama al desempaquetador de subflujo (cuenta y clave padre; L).
- **Cinco comandos NO van por el mapa TCP sino por HTTP** (petición en formulario `msgid=…&rqid=…&urltype=…&pid=…` y respuesta msgpack, como `GET_GAMERELAY`): `CMD_GET_MYCLUB_RANKING`, `CMD_GET_MYCLUB_RANKING_BYID`, `CMD_GET_MYCLUB_WEEKLY_RANKING`, `CMD_GET_MYCLUB_WEEKLY_RANKING_BYID` y `CMD_RECV_MYCLUB_PRESENT`. El `urltype` es el NOMBRE del tipo de URL (p. ej. `urltype=MYCLUB_RANKING`; ver B.12) y esos tipos ocupan estos índices de la tabla de la lista de URLs que manda `CMD_GET_URLLIST`: `MYCLUB_RANKING`=14, `MYCLUB_RANKING_BYID`=15, `MYCLUB_WEEKLY_RANKING`=37, `MYCLUB_WEEKLY_RANKING_BYID`=38, `RECV_MYCLUB_PRESENT`=51 (y `EULA_MYCLUB`=16, `MYCLUB_AGENT_BANNER`=17, `MYCLUB_LIVEEVENT_BANNER`=39). **Hallazgo importante:** la lista de URLs que manda hoy nuestro servidor tiene 14 tipos y **ninguno de myClub**, así que el cliente no tiene a dónde llamar para esos cinco comandos (probablemente por eso no se capturaron nunca). Para activarlos hay que añadir esas URLs (apuntando a `http://{ip}:5000/…`) y responder en HTTP. Lo mismo vale para `DIVISION_RANK`=11 y `DIVISION_RANK_BYID`=12 (rankings de Divisiones, parte A.7).

## B.1 Cambio de nombre del club: `CMD_SET_MYCLUB_TEAM_NAME` (L, TCP mapa)
- **Petición** (mapa de 10 entradas): `msgid`, `rqid`, `pid`, `svrtype`, `client`, `team_name` (texto), `team_name_short` (texto, el nombre corto), `is_check_team_name` (texto, token tipo `YES`/`NO`; por defecto del objeto `NotImplement`), `is_check_team_name_short` (texto, ídem), `team_name_commentary_id` (entero; id del nombre en la locución).
- **Respuesta:** solo `msgid`, `rqid`, `result` (el deserializador no lee nada más) → sobre `NOERR` y nada más. **Obligatoria:** `result`.
- Servidor: guardar `team_name` y `team_name_short` en el club (y el id de locución) y contestar `NOERR`. Los `is_check_*` indican si el cliente pide **validar** el nombre. **P:** un token de error de nombre mostraría el aviso de nombre no válido; no se ha capturado.

## B.2 Cambio de nombre de la plantilla: `CMD_SET_MYCLUB_SQUAD_NAME` (L, TCP mapa)
- **Petición** (mapa de 7 entradas): `msgid`, `rqid`, `pid`, `svrtype`, `client`, `squad_id` (entero), `name` (texto: el nombre nuevo de esa plantilla).
- **Respuesta:** solo `msgid`, `rqid`, `result` → sobre `NOERR`.
- Servidor: guardar el nombre en la plantilla `squad_id` del club y contestar `NOERR`.

## B.3 Historial: `CMD_GET_MYCLUB_HISTORYLIST` (L, TCP mapa)
- **Petición** (mapa de 6): sobre + `start_index` (entero; **C:** `{pid, start_index 0}`; paginación por índice).
- **Respuesta** (todas las claves son las que lee el cliente):
```
{ msgid, rqid, result 'NOERR',
  myclub_history_list_total: int       (total de registros en el servidor),
  index: int                           (índice de inicio de esta página; el que pidió el cliente),
  myclub_history_list_num: int         (cuántos elementos trae esta página),
  myclub_history_list: [ bin(packb({
        history_data_kind: str         (tipo de movimiento: token de una enumeración del cliente; sin capturar),
        history_data_in:  bin(packb({  data_type: str, entity_id: int, count: int,
                                       item_data: bin(packb({item_type: str, item_variation: int, item_effect_arg: int, count: int, pes_point_price: int})) })),
        history_data_out: bin(packb({  count: int,
                                       item_data: bin(packb({item_type, item_variation, item_effect_arg, count, pes_point_price})) })) })) , ... ] }
```
  La anidación es L: cada elemento de la lista es un **bin**; dentro, `history_data_in` y `history_data_out` son **bin**, y dentro de cada uno `item_data` es otro **bin**. La lista admite **hasta 100 elementos** por página (el objeto reserva 100 registros de 0x158 bytes). `history_data_in` = lo que el club **dio** y `history_data_out` = lo que **recibió** (**P**, por los nombres; no se ha visto un registro real).
- **Valores por defecto para un club nuevo (P, seguros para que el menú no rebote):** `myclub_history_list_total 0`, `index` = el pedido, `myclub_history_list_num 0`, `myclub_history_list []` (un array vacío es válido: el bucle se salta con cuenta 0).
- **No cerrado (P):** los tokens de `history_data_kind`, `data_type` e `item_type` (el cliente los compara con tablas de cadenas). Mientras no se saquen conviene devolver lista vacía.

## B.4 Regalos: `CMD_GET_MYCLUB_PRESENTLIST` (L, TCP mapa) y `CMD_RECV_MYCLUB_PRESENT` (L, HTTP)
- **PRESENTLIST petición:** solo el sobre (**C:** `{pid}`). **Respuesta:** `{ result, present_list_num: int, present_list: [ bin(packb({ present_id: int, name_jp: str, title: str, entity_id: int, grade: str, skill: bin(…skill_category, skill_id…), player_level: int, contract_term: int, team_id: int, present_type: str, <bin con los datos del objeto: item_type, item_variation, item_effect_arg, count, pes_point_price>, receipt_deadline: int (fecha límite), count: int })) ] }`. El deserializador abre **tres subflujos bin** (elemento, y dos dentro: el de `skill` y el del objeto, L por la cuenta; cuál es cuál se dedujo por el orden de claves, **P**).
- **RECV_MYCLUB_PRESENT petición (HTTP, `urltype` 51):** `pid`, `present_id` (entero), `is_all` (SÍ/NO: recibir todos). **Respuesta:** `{ result, receive_result_list_num: int, receive_result_list: [ mapa directo { name_jp, title, entity_id, grade, skill, skill_category, skill_id, player_level, contract_term, team_id, present_type, item_type, item_variation, item_effect_arg, count, pes_point_price, receipt_deadline, count, result } ] }` (aquí **no** hay subflujos bin: los elementos son mapas directos; el `result` de cada elemento es el resultado de recibir ese regalo).
- Servidor: la lista con los regalos pendientes (vacía = «sin regalos»); al recibir, mover el objeto al club y marcar el regalo.

## B.5 Logros: `CMD_GET_MYCLUB_ACHIEVEMENTLIST` y `CMD_SEND_MYCLUB_READ_ACHIEVEMENTLIST` (L, TCP mapa)
- **ACHIEVEMENTLIST petición:** solo el sobre (**C:** `{pid}`). **Respuesta:** `{ result, achievement_list_num: int, achievement_list: [ bin(packb({ achievement_category: str, received_all, is_new, milestone_list_num: int, milestone_list: [ bin(packb({ target_value: int, value: int, received_milestone_level: int, present: bin(packb({ present_id, name_jp, title, entity_id, grade, skill, player_level, contract_term, team_id, present_type, <objeto…>, receipt_deadline, count })) })) ] })) ] }`. L por las claves y por los 6 subflujos bin (lista → elemento → `milestone_list` → hito → `present` → `skill`/objeto); si `received_all`/`is_new` son texto o entero es **P**.
- **READ_ACHIEVEMENTLIST petición:** sobre + `achievement_category_list` (lista de categorías vistas). La clase de respuesta no se volcó; por analogía con los otros `SEND_*` es el sobre `NOERR` (**P**).
- Valores por defecto: `achievement_list_num 0`, `achievement_list []`.

## B.6 Rankings de club (L, HTTP)
- **`CMD_GET_MYCLUB_RANKING` petición:** `msgid`, `rqid`, `urltype` (=14), `pid`, `start_index`, `sort`, `order`, `view_num`, `is_coach` (SÍ/NO: ranking de entrenadores). **`..._BYID`:** `urltype` (=15), `pid`, `xuid_one`, `np_account_id`, `sort`, `order`, `view_num`, `is_coach`.
- **Respuesta (ambas):** `{ result, total_rank_num: int, rank_list: [ mapa directo { pname: {name, xuid_one, psn_user_id:{account_id, online_id, is_valid_cache}, is_deleted}, division, favoriteTeam, favoritePlayer, intro, manner, match_record:{win_count, draw_count, lose_count, win_s_max, rating}, team_id, team_name, + posición/puntos } ] }`. El deserializador solo abre el subflujo bin del mensaje entero: **los elementos y `pname`/`match_record` son mapas directos** (misma familia que el ranking de Divisiones, A.7, con `team_id` y `team_name` del club añadidos). Las claves de posición y puntos por elemento no salieron como literales en este volcado (**P**: probablemente claves cortas escritas desde datos, como `rank`/`point` en Divisiones; resolverlas con el mismo método que `rqid`/`pid`).

## B.7 Ranking semanal (L, HTTP) y su inscripción (L, TCP mapa)
- **`CMD_ENTRY_MYCLUB_WEEKLY_RANKING` (TCP mapa):** petición = sobre + `is_entry` (SÍ/NO: inscribirse); respuesta = solo `msgid`/`result` (sobre `NOERR`).
- **`CMD_GET_MYCLUB_WEEKLY_RANKING` petición (HTTP, `urltype` 37):** `pid`, `group_search_target_pid`, `start_index`, `view_num`, `is_group_ranking` (SÍ/NO: solo mi grupo), `is_old_season` (SÍ/NO: temporada anterior). **`_BYID` (38):** `pid`, `xuid_one`, `np_account_id`, `view_num`, `is_group_ranking`, `is_old_season`.
- **Respuesta:** `{ result, total_rank_num, max_weekly_point (int), group_number (int), division (str), rank_list [mapas directos con pname y sus puntos semanales], reward_list: [ mapa directo { reward: {present_id, name_jp, title, entity_id, grade, skill{skill_category, skill_id}, player_level, contract_term, team_id, present_type, item_type, item_variation, item_effect_arg, count, pes_point_price, receipt_deadline} } ] }`. Los `reward_list` son los premios de la semana (mapas directos, sin subflujos bin).

## B.8 Qué hacer en el servidor (por valor)
1. **Cambio de nombre del club y de la plantilla** (B.1, B.2): guardar y contestar `NOERR` (lo que más se nota; sin captura previa; probar abriendo el editor de nombre en el PES).
2. **Historial** (B.3): contestar con la estructura vacía válida (`total 0`, `index`, `num 0`, `list []`); registros reales cuando se conozcan los tokens.
3. **Regalos y logros** (B.4, B.5): estructuras vacías válidas; luego regalos reales (los premios de la semana ya salen en `WEEKLY_RANKING_BASIC_INFO.reward`).
4. **Añadir al `CMD_GET_URLLIST` las URLs** de `MYCLUB_RANKING`, `MYCLUB_RANKING_BYID`, `MYCLUB_WEEKLY_RANKING`, `MYCLUB_WEEKLY_RANKING_BYID`, `RECV_MYCLUB_PRESENT` (y `DIVISION_RANK`, `DIVISION_RANK_BYID`) y responder esos endpoints HTTP (B.0, B.6, B.7).

## B.10 Enumeraciones (tokens) leídas del cliente (L; qué campo usa cada una es P cuando se dice)
Cada enumeración es una tabla de cadenas que el cliente registra en una función y consulta por índice (el índice 0 es la dirección más baja de la tabla). Los campos del servidor van como **texto** (el nombre del token).
- **`item_type`** (8 valores; registrador 0x14021ED40, tabla 0x1437F2AF0): `PRE_MAIN`, `USE_SCOUT`, `USE_TOP_AGENT`, `USE_SPECIAL_AGENT`, `CONTRACT`, `RECOVERY`, `STORE`, `SUBSCRIPTION`. **C:** `CONTRACT` y `RECOVERY` ya se ven en vivo en `GET_MYCLUB_ITEMLIST` (boleto de contrato y recuperación). Valor por defecto del cliente: `NotImplement`.
- **Tipo de dato / entidad** (10 valores; registrador 0x14021BEC0, tabla 0x1437F2BC0): `PES_POINT`, `GP`, `COACH`, `AGENT`, `SCOUT`, `RENTAL_PLAYER`, `ITEM`, `UNIFORM`, `PLAYER`, `E_FOOTBALL_POINT`. **P:** es la enumeración de `data_type` (lo que son `history_data_in`/`out`: moneda, entrenador, agente, ojeador, jugador de alquiler, objeto, uniforme, jugador, puntos eFootball).
- **Tipo de movimiento** (15 valores; registrador 0x140221B20, tabla 0x1437F4600): `NO_TYPE`, `PES_POINT`, `GP`, `PLAYER`, `RENTAL_PLAYER`, `COACH`, `AGENT`, `SCOUT`, `TRAINING_COACH`, `PLAYER_EXPERIENCE`, `SIGN_PLAYER`, `SIGN_COACH`, `ITEM`, `UNIFORM`, `SQUAD`. **P:** es la enumeración de `history_data_kind` (qué pasó: fichar jugador o entrenador, entrenar, dar experiencia, comprar objeto o uniforme, plantilla…), por contener verbos (`SIGN_*`, `TRAINING_COACH`, `PLAYER_EXPERIENCE`) y el comodín `NO_TYPE`. Falta confirmar con una captura real qué token de cada tabla usa cada campo; hasta entonces conviene devolver historial vacío.
- Otras tablas vistas en la misma zona (no pedidas): categorías de sorteo (`DRAW_CARD_CATEGORY_MAIN/SUB1/SUB2`), aptitud (`APTITUDE_HIGH/LOW`, `NOT_APTITUDE`), tipos de partida de myClub (`MYCLUB_RANK`, `MYCLUB_COM`, `MYCLUB_MATCHDAY`, `MYCLUB_DIVISION`, …) y estado de subasta (`AUCTION_USER_STATE_*`).

## B.11 Claves cortas que faltaban (L; leídas de la memoria del exe)
Algunas claves del cliente se escriben desde datos y no como literal; resueltas: `rqid`, `pid`, `name`, `rank`, `date`, `user`, `body`, `type`, `item`.
- **Rankings de club (`RANKING` y `_BYID`):** respuesta `{msgid, rqid, result, date (unix), total_rank_num, rank_list:[{rank:int, user:{pid:int, pname:{name, xuid_one, psn_user_id:{account_id, online_id, is_valid_cache}, is_deleted}, division, favoriteTeam, favoritePlayer, intro, manner, match_record:{win_count, draw_count, lose_count, win_s_max, rating}, team_id, team_name}}]}`. Es la misma forma que el ranking de Divisiones (A.7) con `team_id`/`team_name`. No hay clave `point`: el orden lo da `rank`.
- **Ranking semanal:** `rank_list` con `rank` y el nombre de jugador (`pname`) por elemento; los puntos semanales por elemento no salen como clave aparte (**P**: solo se ve `max_weekly_point` a nivel de respuesta). `reward_list:[{reward: regalo}]`.
- **Objeto «regalo»** (presentes, logros, recompensas semanales y respuesta de recibir): `{present_id, name, name_jp, title, body, entity_id, grade, skill:{skill_category, skill_id}, type, player_level, contract_term, team_id, present_type, item:{item_type, item_variation, item_effect_arg, count, pes_point_price}, receipt_deadline, count}` (coincide con el «Regalo» de `ESPEC_MYCLUB.md`). En presentes y logros va como **bin**; en las recompensas semanales y en la respuesta de recibir va como **mapa directo**.

## B.12 Cómo llama el cliente a los comandos HTTP (C con capturas reales; L para los urltype nuevos)
- **Petición (C):** `POST //<ruta> HTTP/1.0` (doble barra inicial: el cliente antepone `/` a la ruta de la URL configurada), `Content-Type: application/x-www-form-urlencoded`, cuerpo `msgid=CMD_…&rqid=N&urltype=NOMBRE&…` (ejemplo capturado: `msgid=CMD_AUTH_STEAM&rqid=0&urltype=AUTH_STEAM&steamid=239200&package_type=DEFAULT`). El orden de los campos es el del serializador (L): `msgid`, `rqid`, `urltype`, `pid`, luego los propios del comando.
- **`urltype` es el NOMBRE del tipo de URL** (no el índice), p. ej. `urltype=MYCLUB_RANKING`. La ruta la da el servidor en `CMD_GET_URLLIST` (nuestras URLs actuales son del estilo `http://{ip}:5000/<ruta>`, p. ej. `RANK`, `GET_GAMERELAY`, `AVATAR`). El cliente solo hace la llamada si el tipo está en esa lista; hoy no están `MYCLUB_RANKING`, `MYCLUB_RANKING_BYID`, `MYCLUB_WEEKLY_RANKING`, `MYCLUB_WEEKLY_RANKING_BYID`, `RECV_MYCLUB_PRESENT`, `DIVISION_RANK` ni `DIVISION_RANK_BYID` (índices 14, 15, 37, 38, 51, 11, 12 de la tabla de tipos).
- **Respuesta:** cuerpo msgpack (como el resto de respuestas HTTP que ya servimos: `GET_GAMERELAY`, `AVATAR`, livedata), con `msgid`, `rqid`, `result 'NOERR'` y los campos de cada comando (B.4, B.6, B.7). **P:** el tipo de contenido que espera es `application/octet-stream` como en las respuestas HTTP actuales; no se ha visto una llamada real a estos siete tipos.
- **Campos propios por tipo:** `MYCLUB_RANKING` → `start_index`, `sort`, `order`, `view_num`, `is_coach`; `MYCLUB_RANKING_BYID` → `xuid_one`, `np_account_id`, `sort`, `order`, `view_num`, `is_coach`; `MYCLUB_WEEKLY_RANKING` → `group_search_target_pid`, `start_index`, `view_num`, `is_group_ranking`, `is_old_season`; `MYCLUB_WEEKLY_RANKING_BYID` → `xuid_one`, `np_account_id`, `view_num`, `is_group_ranking`, `is_old_season`; `RECV_MYCLUB_PRESENT` → `present_id`, `is_all`. En todos va también `pid`.

## B.9 Cómo se obtuvo (para replicar)
`analyzeHeadless <proyecto> PEStrim -process -noanalysis -readOnly -postScript DumpCmd.java` (busca las cadenas `CMD_…` y descompila las funciones que las referencian), luego `DumpVt.java` (para una clase, descompila las funciones de las ranuras 0-12 de su `vftable`); las claves cortas escritas desde datos (`rqid`, `pid`, `name`) se resolvieron con `PrintStr.java`. La petición con campos extra usa la ranura 6 (mapa msgpack) o la ranura 3 (formulario HTTP `clave=valor&…`); la respuesta usa la ranura 7. Los scripts están en el scratchpad de la sesión (no subidos).

## B.13 Partidos contra la COM y plantilla en la nube (L, 10-nov; probado en el PES: sin manejador al abrir myClub)
Ambos comandos los pide el cliente al abrir myClub (pestaña Estadíst.) y salían como «sin manejador». Petición de los dos (mapa de 5 entradas, sin campos propios): `msgid`, `rqid`, `pid`, `svrtype` (MENU), `client` (NETCLIENT).
- **`CMD_GET_MYCLUB_COM_MATCH_INFO`** — respuesta (mapa): `msgid`, `rqid`, `result` (token `NOERR`), `match_stats`. **`match_stats` es un bin(packb(mapa))** (el cliente lo abre con su desempaquetador de subflujo, como `history_data`), y ese mapa lleva **8 claves enteras** (el cliente acepta entero o decimal; un valor nulo se ignora): `match_num` (partidos), `win_num`, `lose_num`, `draw_num`, `goal` (goles a favor), `lostGoal` (goles en contra), `contWin_num` (racha actual), `contWinMax_num` (mejor racha). Valores seguros para un club nuevo: todo `0`. Servidor: contar los partidos del club contra la COM (misma cuenta que W/D/L vs COM).
- **`CMD_GET_MYCLUB_CLOUD_SQUADID`** — respuesta (mapa): `msgid`, `rqid`, `result` (`NOERR`), `squad_id` (entero, id de la plantilla guardada en la nube). Valor seguro: `0` (**P**: sin plantilla en la nube; no se ha capturado qué hace el cliente con otro valor).
- **Dónde está el cambio de nombre del club (CMD_SET_MYCLUB_TEAM_NAME):** NO está en «Historial myClub → Información y estadísticas → Información del club» (es solo lectura: «Nom. del equipo», «Ciudad de origen», «Aclamación»; Enter no abre nada). **P:** vive en un menú de opciones del club (no localizado: la ventana del PES sale recortada y no se vio un botón de opciones). Pendiente: probar los iconos/menú superior derecho de myClub.
- **Comandos «sin manejador» nuevos vistos al navegar** (modo eFootball League dentro de la pestaña Estadíst. → «Historial de eFootball League», respuesta del juego: «Campeonato no encontrado»): `CMD_JOIN_PESLEAGUE_COMPE_MODE` (`compe_mode`), `CMD_GET_PESLEAGUE_MAINMENU_INFO` (`mode`), `CMD_GET_PESLEAGUE_COMPE_LIST` (`is_end`, `is_join_only`, `mode`, `start_index`), `CMD_LEAVE_PESLEAGUE_COMPE_MODE`. Sin respuesta, el cliente muestra ese aviso y no se cierra.
- **Cómo se obtuvo:** mismo método de B.9 (`DumpVt.java` sobre `CmdGetMyclubComMatchInfoResult/Option` y `CmdGetMyclubCloudSquadidResult/Option`, ranura 7 = deserializador; claves en la memoria del exe, 142b1f8d8 = «goal»/«lostGoal»), siempre `-readOnly`.
- **Ranking de club: dónde lee el cliente `team_id` y `team_name` (L + C, 10-nov).** En el deserializador de `MYCLUB_RANKING` (`CmdGetMyclubRankingResult`), `team_id` (entero) y `team_name` (texto) se leen **al nivel del ELEMENTO de `rank_list`** (hermanos de `rank` y `user`), NO dentro de `user`; el valor por defecto del nombre es `NotImplement`, que es lo que sale si la clave no está a ese nivel. Confirmado en el PES con 55ee411: el Ranking del club muestra «Phoenix FC» con su escudo.

## B.14 Historial, regalos y logros: tipos de cada clave y valores de texto (L, 10-nov; solo lectura del cliente)
Se clasificó, clave por clave, qué hace el deserializador con cada valor: leerlo como **texto**, como **entero** (acepta entero o decimal; un valor nulo hace que el cliente se salte el campo) o abrirlo como **bin anidado** (subflujo). Esto **corrige** B.3–B.5 en tres puntos.
**Correcciones:** (1) `item_variation` es **texto**, no entero (en historial, regalos y logros). (2) `grade` del regalo es un **entero**, no texto. (3) `received_all` e `is_new` del logro son **texto** (tokens tipo SÍ/NO; el valor exacto es P). Lo demás de B.3–B.5 se confirma.
**Contadores y listas (nombres exactos):**
- Historial: lista `myclub_history_list` (bin), contador `myclub_history_list_num`, total `myclub_history_list_total`, e `index` (enteros).
- Regalos: lista `present_list` (bin), contador `present_list_num` (entero).
- Logros: lista `achievement_list` (bin), contador `achievement_list_num` (entero); dentro de cada logro, `milestone_list` (bin) y `milestone_list_num` (entero).
**Tipo de cada clave de elemento:**
- Historial (cada elemento es un bin): `history_data_kind` texto; `history_data_in` bin → `data_type` texto, `entity_id` entero, `count` entero, `item_data` bin; `history_data_out` bin → `count` entero, `item_data` bin; `item_data` (bin) → `item_type` texto, `item_variation` texto, `item_effect_arg` entero, `count` entero, `pes_point_price` entero.
- Regalo (cada elemento es un bin; igual dentro de un hito de logro, en la clave `present`): `present_id` entero, `name_jp` texto, `title` texto, `entity_id` entero, `grade` **entero**, `skill` bin (con `skill_category` y `skill_id`; sus tipos no se leyeron: P), `player_level` entero, `contract_term` entero, `team_id` entero, `present_type` **texto**, objeto (`item_type` texto, `item_variation` texto, `item_effect_arg` entero, `count` entero, `pes_point_price` entero), `receipt_deadline` entero, `count` entero.
- Logro (cada elemento es un bin): `achievement_category` texto, `received_all` texto, `is_new` texto, `milestone_list_num` entero, `milestone_list` bin de hitos; cada hito (bin): `target_value` entero, `value` entero, `received_milestone_level` entero, `present` bin (un regalo).
**Valores de texto.** Los tres deserializadores **guardan los textos tal cual, sin compararlos con ninguna tabla** (`present_type`, `history_data_kind`, `data_type`, `item_type`, `achievement_category`...): quien los compara es el código de pantalla que los usa después. Por eso un valor desconocido no hace fallar la lectura. Lo que sí se localizó en el exe son las **tablas de textos registradas por el cliente**, y se listan como candidatas (**P**: no se pudo atar cada tabla a su campo sin seguir el código de las pantallas):
- `item_type` (L, B.10): `PRE_MAIN`, `USE_SCOUT`, `USE_TOP_AGENT`, `USE_SPECIAL_AGENT`, `CONTRACT`, `RECOVERY`, `STORE`, `SUBSCRIPTION`. Ninguno es dinero.
- `data_type` y tipo de movimiento (B.10): ahí sí aparecen `PES_POINT` y `GP`; son monedas, pero van en `history_data_in/out.data_type` (y su cantidad en `count`/`pes_point_price`), **no** en el `item_type` del regalo. Un regalo en GP o monedas **no se ha visto**; con las tablas leídas, lo más probable (P) es registrarlo como regalo con `present_type` de moneda o en el historial con `data_type GP`.
- Candidatas a `achievement_category` (una tabla contigua, registrada junto a `ACHIEVEMENT_ID_000`/`ACHIEVEMENT_ID_001`): `ELECTED_MYPLAYER_NUM`, `SQUAD_BONUS`, `SPECIAL_MATCH`, `EVENT`, `GAME_RANK`, `TEAM_LEVEL`, `HIGH_SUCCESS`, `SUCCESSIVE5`, `POWERFUL_ENEMY`, `DIFFERENT20`, `MATCH100`, `MATCH50`. Los nombres (`MATCH50`, `MATCH100`, `SUCCESSIVE5`, `DIFFERENT20`, `POWERFUL_ENEMY`) apuntan a logros de «50/100 partidos», «5 victorias seguidas», «20 rivales distintos» y «ganar a un rival fuerte».
- Candidatas a `present_type` (tabla vecina): `KONAMISTAFF`, `EVENTER`, `NORMAL`, `COACH`, `TEAMSTRENGTH`, `TEAMSPIRIT`, `NATIONAL`, `NOTYPE`; y los rangos de habilidad `SKILL_RANK_1`..`SKILL_RANK_12` y `SKILL_RANK_NONE` (para `skill`).
**Qué enviar de forma segura:** mientras no haya capturas reales, listas **vacías** (contador 0) siguen siendo lo seguro. Si se quiere probar un regalo o un logro, usar un único elemento con textos de las tablas de arriba y todos los enteros presentes (un entero ausente se queda en 0; un texto ausente queda vacío o `NotImplement`).
