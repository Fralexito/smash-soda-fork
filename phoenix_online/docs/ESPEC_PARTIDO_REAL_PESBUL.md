# Del chequeo de conexión al partido real: lo que dice PESBUL (estudio de solo lectura, 10-nov-2026)

Fuente: lectura de la copia local de PESBUL (`D:\Frank\IA\CLAUDE\pesbul_local\app`), sin ejecutar nada y sin copiar código. Todo está dicho con palabras nuestras. Marcas: **L** = leído en PESBUL (comentarios con «MEASURED/PROVEN» de clientes reales), **C** = comprobado por nosotros con el PES base, **P** = hipótesis o duda.
Complementa `ESPEC_GAMERELAY_FASE23.md` §5.3 (el chequeo de conexión ya está resuelto: REVISION_CHECK + regla del pid).

## 1. Dónde estamos
Con un PES real y el rival falso (`tools/rival_falso.py --revision`) llegamos hasta: chequeo NOERR → `SEND_MATCHING_RESULT` → `CHANGE_GAMEPHASE` → intercambio de `READY_FLAG`/`ALLREADY_FLAG`/`USER_DATA_SYNC` por el canal 15 → pantalla morada → el PES aborta a ~66 s con `SEND_ABNORMALEND GNCX_0011_0x20000003`. Eso es justo la **carga del partido** (GAMELOAD, canal 10), que un rival falso no puede generar. Hace falta un segundo PES real (otra PC o un amigo).

## 2. Respuesta a CMD_GET_MATCH_ANALYZE (L)
- El PES la manda dos veces en esta fase con `{analyze_data_type, client, msgid, pid, rqid, svrtype, target_pid}`. El `target_pid` es el rival (en PESBUL también en la variante sin «_BYID»).
- PESBUL contesta con el resultado completo, no con el sobre vacío: `{msgid, rqid, result 'NOERR', analyze_data_size N, analyze_data_list [N elementos], game_res}`. Las dos claves que lee el parser del cliente son `analyze_data_list` y `analyze_data_size`. Con el sobre vacío genérico (`{msgid, rqid, result}`) el menú de información de divisiones «se salía» (L, r7.3.626).
- Cada elemento de la lista es un **bin** con un mapa msgpack de 45 contadores del partido más `game_res` (texto `WIN`/`LOSE`/`DRAW`). Claves (todas propias del parser del cliente): posession, attack_time, deffence_time, attack_area_left/middle/right, shoot_count, inframe_shoot_count, suffer_shoot_count, suffer_inframe_shoot_count, pass_count, pass_success_count, pass_distance, pass_cut_count, cross_count, cross_success_count, dribble_distance, match_up_distance, intercept_area, tackle_count, tackle_success_count, foul_count, offside_count, score_pass_count, score_through_pass_count, score_cross_count, score_dribble_count, score_set_play_count, score_other, score_loss_pass_count, score_loss_through_pass_count, score_loss_cross_count, score_loss_dribble_count, score_loss_set_play_count, score_loss_other, concept_arrange_offence_first/second, concept_arrange_defence_first/second, favorite_play_offence_first/second/third, favorite_play_defence_first/second/third.
- Contenido: PESBUL solo devuelve estadísticas **subidas realmente** por clientes (el cliente las manda con `CMD_SEND_MATCH_ANALYZE`), de toda la vida del jugador (el cliente se queda con la cola de la lista). Sin historial: lista vacía y `game_res 'DRAW'`. Para el partido real basta con `analyze_data_size 0`, `analyze_data_list []`, `game_res 'DRAW'`. **P:** no se midió si el PES acepta la lista vacía en esta fase concreta (antes del partido), pero es lo que PESBUL hace con jugadores nuevos.
- Nuestro estado: sin manejador → NOERR vacío sin esas claves. **Hacer:** manejador con las tres claves.

## 3. CMD_GET_PLAYER_PARAMETERLIST (L/P)
PESBUL **no tiene manejador propio**: cae en el relleno genérico de todos los `CMD_GET_*` (solo `{msgid, rqid, result NOERR}`). El nombre no aparece en ningún otro sitio de su código ni de sus documentos, así que no hay formato medido. El PES la pide en esta fase (~1 s después de GET_MATCH_ANALYZE) y no se quejó con el NOERR vacío (C: el aborto llegó 60 s después, por GAMELOAD, no por esto). **P:** más adelante, con el partido en marcha, el cliente podría esperar una lista de parámetros de jugador (la de myClub lleva `parameters`); con lo que sabemos no hay contenido definido. Recomendación: dejarlo con NOERR vacío y medir cuando dos PES reales lleguen a pedirlo.

## 4. Comandos TCP del partido que PESBUL responde solo con sobre NOERR (L)
`CMD_CHANGE_GAMEPHASE`, `CMD_SET_GAMEMEMBER`, `CMD_SET_GAMEMEMBERENV`, `CMD_SET_GAMETEAM`, `CMD_SET_GAMEPLAYER_PLAYTIME`, `CMD_SEND_SYNC_NOTICE`, `CMD_SEND_SYSTEM_NOTICE`, `CMD_SEND_TURN_ADDRESS_DATA`, `CMD_START_SEND_GAME_SEQUENCE_DATA`, `CMD_SEND_GAME_SEQUENCE_DATA`, `CMD_ADD_FOUL`, `CMD_ADD_SCORE`, `CMD_SEND_MATCH_TEAM_INFO`, `CMD_SEND_ABNORMALEND`, `CMD_REQ_GAMEADDPOINT`, `CMD_LEAVE_GAME`, `CMD_SEND_GAME_RESULT`, `CMD_SEND_STATS_DATA`, `CMD_SEND_TEXTCHAT`, `CMD_WATCH_MATCHING_CONDITION`.
Sin respuesta la cadena de inicio se atasca («CHANGE_GAMEPHASE sin contestar → la selección de equipo vence», «SET_GAMEMEMBER sin contestar → el cargador se queda en el estado 2»). Además del ACK, PESBUL usa: `CHANGE_GAMEPHASE` (anota la fase), `ADD_SCORE` (cuenta goles), y `SEND_GAME_RESULT`/`REQ_GAMEADDPOINT`/`SEND_STATS_DATA` cierran el partido y dan puntos/rating (ambos clientes suben el **mismo** resultado; es fin anormal solo si `abnormalend_reason` no es `NOERR`/vacío). Nuestro servidor ya contesta estos con NOERR (no salieron avisos «sin manejador» salvo MATCH_ANALYZE).

## 5. Cómo fluye la carga y el partido entre dos PES por el relé (L; tiempos medidos por PESBUL con clientes reales)
1. Chequeo de conexión y READY/ALLREADY/USER_DATA_SYNC (canal 15) — **ya lo hacemos (C)**.
2. `CMD_SET_GAMEMEMBER`, `CMD_SET_GAMETEAM`, `CMD_CHANGE_GAMEPHASE` (TCP, ACK).
3. **GAMELOAD**: cada cliente emite un bloque grande por el **canal 10** (nativo UDP, y copia por TCP `SEND_GAME_SEQUENCE_DATA` con `channelId 10`). PESBUL midió 9 registros de canal 10 por cliente. El relé los reenvía al otro. Si el otro no los recibe: `abnormalend_reason LOAD_TIMEOUT` en ambos (medido varias veces; es el aborto que vemos nosotros, con otro token, `GNCX_0011`).
4. Unos 9 s después de GAMELOAD ambos mandan por TCP `CMD_SEND_MATCH_TEAM_INFO` (ACK).
5. El primer registro de **canal 9** (partido jugándose: entradas de mandos en lockstep) llega unos **46 s** tras GAMELOAD. Hasta ahí, pantalla de carga/presentación.
6. Canal 12: estado del partido. Canal 13: MatchControlInfo (cada 250 ms durante el juego, en ambas direcciones; PESBUL lo trata aparte y de forma opcional).

**¿Tiene el servidor que hacer algo más que reenviar? (L, con matices)**
- Reenviar fielmente **10, 12 y 15** con cabecera `0x4000 | (id_emisor << 4)` (cada bloque, un mensaje) y **9** con fan-out a todos, incluido el emisor (lockstep). Nuestro relé ya lo hace (fases 2-3).
- **Cada mensaje llega por dos vías** (UDP nativo y copia TCP). Hay que entregar **una sola vez** (deduplicar por sala + generación + emisor + canal + contador de aplicación). Nuestro relé ya deduplica; las copias TCP con `fromId` ajeno se rechazan (normal).
- GAMELOAD es el punto frágil: PESBUL perdió partidos porque los registros de canal 10 se descartaban (primero solo aceptaba canal 15; después se perdían entre servidores). Con **un solo servidor** (nuestro caso) basta con reenviar todo lo del canal 10 por la vía nativa y la TCP sin tirar nada.
- Tamaños: registro TCP de 256 B con cabecera de 9 B; el mensaje real va troceado en bloques de ≤255 B por canal nativo (cada bloque un elemento ARQ con su clave). El relé debe respetar el control ARQ por canal (acks por clave) para que la ventana del cliente no se bloquee durante la carga.
- Vida: el intento de partida no debe expirar (nuestro tope es 3 h); la sesión UDP de un jugador se olvida tras 90 s sin datos. Durante la carga hay silencios de unos 5 s, no debería afectar; conviene el latido del canal 0 (`VIDA`) cada pocos segundos.
- No hay «inteligencia» del servidor en el cálculo del partido: es P2P lockstep sobre el relé.

## 6. Qué espera el PES en USER_DATA_SYNC del otro lado (L/C)
PESBUL no lo interpreta: es un mensaje de aplicación del canal 15 que el servidor solo reenvía. Comprobado con nuestro rival: cada PES manda `{msgid CMD_USER_DATA_SYNC, rqid 1, pid, platform_id{np_account_id 0, xuid_one ''}, name, pad_no, personal_data, side, guest_num, guest_list, f2p, net_link_type, game_mode}` y espera el del otro. `side` = HOME para el anfitrión (id 0, buscó primero) y AWAY para el invitado; `pid` es el entero nativo (32 bits bajos del pid de FINISH); `game_mode` 24 en Divisiones. El PES avanza (READY_FLAG 3 → 7, ALLREADY 1 → 2) cuando recibe el del otro. **Con dos PES reales no hay nada que fabricar**: solo relayar. Pendiente de comprobar con dos PES: que `side` salga opuesto en cada uno (lo deciden `is_owner YES/NO` y `matching_order 0/1` del FINISH).

## 7. Tareas para el servidor (por orden de utilidad)
1. `CMD_GET_MATCH_ANALYZE` (y `_BYID`): responder con `analyze_data_size`, `analyze_data_list` y `game_res` (vacío = 0 / `[]` / `DRAW`).
2. Prueba con **dos PES reales** (Frank + otra PC, por Tailscale o LAN). El resto (GAMELOAD, TEAM_INFO, canal 9) ya lo reenvía el relé; mirar el log por «canal 10 ->» y por avisos de fromId.
3. Registrar en el log (como ya hacemos con el 15) los primeros mensajes de los canales 10, 9 y 12 por par, para medir tamaños y tiempos contra los de PESBUL (GAMELOAD +0 s, TEAM_INFO ~+9 s, primer canal 9 ~+46 s).
4. Cierre del partido: `SEND_GAME_RESULT` (ambos suben el mismo), `REQ_GAMEADDPOINT`, `SEND_STATS_DATA`, `SEND_MATCH_ANALYZE`: guardar resultado y actualizar puntos/rating.
5. `CMD_GET_PLAYER_PARAMETERLIST`: medir con partido real antes de inventar formato.

## 8. Qué NO está comprobado (P)
- Comportamiento con el PES como anfitrión (id 0) y el otro como invitado (nuestras pruebas fueron PES invitado).
- Si el cliente acepta `analyze_data_list` vacío en esta fase.
- Tamaño y ritmo reales del GAMELOAD entre dos PES de la **base** de Frank (los de PESBUL eran de otros parches). Los checksums `param_check_sum` y `entry_check_sum` del REVISION_CHECK deben coincidir en ambos PES: con ficheros distintos el chequeo debería fallar (por eso hace falta la misma base en las dos PC).
