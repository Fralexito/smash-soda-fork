# ESPEC · Cooperativo, Clanes, Matchday y PES League (Torneo en línea) — parte D

Lectores de PEDIDO y RESPUESTA leídos del cliente (Ghidra `-readOnly`, 11-nov). Marcas: **L** leído; **P** probable; **N** no resuelto. Convenciones del cliente: los «booleanos» van como texto `YES`/`NO`; `bin` = subflujo empaquetado (`{"bin": {...}}` en lista o suelto); todos los comandos llevan además `msgid`, `rqid` y en la respuesta `result`. Comandos vistos sin manejador al recorrer el PES base el 11-nov (menú eFootball → Cooperativo / Matchday / Torneo; myClub → «En curso»).

## D.1 Cooperativo (online)
| Comando | Pedido | Respuesta |
|---|---|---|
| `GET_COOP_MAINMENU_INFO` | `online_mode` (texto), `is_check_season_end` (texto SÍ/NO) | `is_coop_banned` (texto), `is_season_end` (texto), `division` (texto), `pre_season_rank` (int), `pre_season_rank_all_clan_num` (int), `division_change_type` (texto), `is_exist_owner_clan`, `is_exist_new_request`, `is_exist_new_pending` (texto) |
| `GET_COOP_PROFILE` | sin campos propios | `user_info` (**bin**) con `pname` (bin: `name, xuid_one, psn_user_id{account_id, online_id, is_valid_cache}, is_deleted`), `coop_profile_data` (**bin**), `coop_manner` (texto), `match_data` (texto) |
| `GET_IS_JOIN_COOP_ROOM` | sin campos propios | `is_join_coop_room` (texto SÍ/NO) |
| `GET_COOP_ROOMLIST` | `search_options` (bin/mapa) con `room_id` (int), `use_password`, `vs_setting`, `team_level`, `coop_level`, `coop_manner`, `cpu_level_min`, `cpu_level_max`, `can_entry` (texto), `compe_id` (int); `start_index` (int) | `room_list` (**lista de bin**) y `room_list_num`, `all_room_num` (int). Elemento: `room_id` (int), `room_name`, `clan_name` (texto), `clan_owner_info` (bin con `pname`), `basic_settings` (bin: `is_follow`, `is_com_match`, `is_use_chat`, `is_same_level`, `team_level`, `coop_level`, `is_same_manner`, `coop_manner`, `room_entry_restriction`, `password`), `match_settings` (bin), `user_num` (texto), `compe_id` (int), `is_game` (texto) |
| `GET_COOP_REJECT_LIST` | sin campos propios | `reject_list` (**lista de bin** con `pname`…) y `reject_list_num` (int) |
| `GET_CLAN_LIST` | `search_options` (bin) con `clan_name`, `enable_request`, `is_joined_clan` (texto); `start_index` (int) | `clan_list` (**lista de bin**), `clan_list_num`, `all_list_num`, `start_index` (int). Elemento: `basic_settings`/`clan_name` (texto), `owner_info` (bin `user_id{pname}`), `coop_level` (int), `is_follow`, `last_login` (int), `current_online_mode`, `is_join_online_mode`, `enable_request`, `clan_comment`, `is_banned` (texto), `clan_members` (bin) y `clan_member_num` (int), `clan_stats_online`, `season_match_stats`, `total_match_stats`, `clan_stats_myclub` (bin) |

Respuesta mínima para abrir limpio (**P**): `GET_COOP_MAINMENU_INFO` con todo en NO/vacío y `division:""`; `GET_COOP_PROFILE` sin `user_info` (la pantalla mostró «NotImplement», Nv.0, con valores por defecto); `GET_IS_JOIN_COOP_ROOM` con `NO`; listas vacías con su `*_num` en 0.

## D.2 Matchday
| Comando | Pedido | Respuesta |
|---|---|---|
| `JOIN_MATCHDAY_MODE` / `LEAVE_MATCHDAY_MODE` | sin campos propios | sin campos propios (basta `NOERR`) |
| `GET_MATCHDAY_LIST` | `is_myclub` (texto), `start_index` (int) | `compe_list` (**lista de bin**), `list_num`, `list_max` (int), `is_no_check` (texto) |

Elemento de `compe_list` (L): `compe_id` (int), `base_id` (int), `compe_name` (texto), `is_disp_compe_name`, `group_name`, `group_desc` (texto), `group_info_a`, `group_info_b` (bin), `is_entry_restricted` (texto), `match_type`, `squad_type`, `group_type`, `rating_rank` (texto; tokens de tablas del cliente: `MATCHDAY_RATING_RANK_ELITE/PRO/ACADEMY/NONE`; `MATCHDAY_GROUP_TYPE_LEAGUE/NATIONAL/OTHER/TEAM`; `MATCHDAY_SQUAD_MYCLUB/SPECIAL`), `start_date`, `entry_limit_date`, `end_group_match_date`, `pickup_member_date`, `end_date` (int, unix), `entry_user_num`, `entry_user_max` (int), `is_end` (texto), `banner_file_info`, `list_banner_file_info` (bin `{filename, hash_str}`), `is_use_emblem`, `is_use_emblem_list` (texto), `is_entry` (texto), `watch_match_reward_state` (texto), `compe_db_id` (int), `is_possible_watch` (texto), `add_limit_rating` (int).
Entrada: «Matchday» en eFootball (`is_myclub:"NO"`) y en «En curso» de myClub (`is_myclub:"YES"`, **P**). Con lista vacía el cliente dice «Campeonato no encontrado» (aviso limpio, sin cierre).

## D.3 PES League («Torneo en línea» y «eFootball League de myClub»)
| Comando | Pedido | Respuesta |
|---|---|---|
| `JOIN_PESLEAGUE_COMPE_MODE` | `compe_mode` (texto) | sin campos propios |
| `LEAVE_PESLEAGUE_COMPE_MODE` | sin campos propios | sin campos propios |
| `GET_PESLEAGUE_MAINMENU_INFO` | `mode` (texto) | `division` (texto), `new_result` (texto SÍ/NO) |
| `GET_PESLEAGUE_COMPE_LIST` | `mode` (texto), `is_end` (texto), `is_join_only` (texto), `start_index` (int) | `compe_list` (**lista de bin**), `list_num`, `list_max` (int), `is_no_check` (texto) |

Elemento de `compe_list` (L, claves vistas): `entry_limit_division`, `entry_limit_division_min`, `entry_limit_division_max` (texto, división tipo `D12`), `is_end`, `is_entry`, `is_gm_account` (texto), `compe_db_id` (int), `banner_file_info`, `list_banner_file_info` (bin `{filename, hash_str}`). Además llevará `compe_id`/`compe_name`/fechas como el resto de competiciones (**P**; esas claves usan literales compartidos que el lector no muestra de forma aislada). **N:** no leí los tokens de `compe_mode`/`mode` (devolver el mismo texto que llegue).
Entrada: «Torneo en línea» (eFootball) y «eFootball League de myClub» (myClub → En curso) usan esta familia; ambos dan «Campeonato no encontrado» con lista vacía.

## D.4 Contadores de la tarjeta «Entrenamiento» (0/0/0) — hallazgo parcial
- `GET_MYCLUB_MAINMENU_INFO` solo trae `staff_num` (además de `present_num`, `scount_num`…); **no** hay tres contadores de entrenador en esa respuesta (**L**).
- «Entrenador de experiencia» = los **jugadores convertidos en staff** (`CMD_CREATE_MYCLUB_STAFF` / `CMD_GET_MYCLUB_STAFF_LIST` con `staff_list:[bin{id, base_player_info, base_exp}]`, `staff_list_num`). El servidor ya los maneja.
- «Entrenador de habilidades» y «Entrenador posicional»: son los objetos `ADD_SKILL` / `ADD_POSITION_APTITUDE` (comandos `USE_MYCLUB_ADD_SKILL_ITEM` / `…ADD_POSITION_APTITUDE_ITEM`) pero **no** salen de `GET_MYCLUB_ITEMLIST` aunque se envíen con `item_type` STORE, USE_SPECIAL_AGENT, PRE_MAIN, USE_TOP_AGENT, USE_SCOUT o SUBSCRIPTION (probado en vivo: la tarjeta sigue en 0 y al elegir sale «No tienes entrenadores de habilidades. Puedes conseguirlos en eventos exclusivos…»). **P:** se cuentan por un comando de lista de entrenadores por jugador (`Online/MyClub/Training/ProcessGetStaffListByPlayer`, clave `staff_list_num`) y/o por items con otra `item_variation` aún no hallada.

## D.5 Detalle e inscripción de Matchday y PES League (L; Ghidra solo lectura, 11-nov)
| Comando | Pedido | Respuesta |
|---|---|---|
| `JOIN_MATCHDAY` | `compe_id` (int), `entry_matchday_group` (texto), `team_id` (int), `sp_gameplan_data` (texto/bin) + `sp_gameplan_data_size` (int), `sp_member_list` (lista de int) | `squad_rating_info` (**bin**) |
| `LEAVE_MATCHDAY` | sin campos propios | sin campos propios |
| `GET_MATCHDAY_INFO` | `compe_id` (int) | `compe_info` (**bin**) y `info_overview` (**bin**); dentro: los campos del elemento de `GET_MATCHDAY_LIST` (D.2) más `start_match_limit_date`, `entry_pickup_match_start_date`, `watch_match_start_date` (int, unix), `is_check_eula`, `eula_url` (texto), `ball_id`, `group_match_time`, `pickup_match_time`, `watch_wait_time`, `use_stadium_id` (int), `fixed_timezone`, `fixed_weather`, `fixed_season`, `is_fix_condition`, `is_equalization`, `is_pesleague_stadium`, `is_efootball_pro_stadium` (texto), `joined_matchday_group`, `pickup_select` (texto), `team_id`, `pickup_cut_off_rank` (int), `team_list_num` (int), y los bloques de premios (**bin**): `all_reward_info`, `achievement_reward_list`, `win_reward_list`, `lose_reward_list`, `watching_reward_list`, `pickup_win_reward_list`, `pickup_lose_reward_list` (cada premio = objeto «regalo» de B.11), y `squad_rating_info` (bin: `sp_squad_rating`, `rating_kind_player_bonus_value_list`, `evaluation_a`, `evaluation_b`, `superiority_border_ratio`) |
| `GET_MATCHDAY_MAINMENU_INFO` | `compe_id` (int) | `mainmenu_info` (**bin**) con `matchday_progress` (texto), `group_match_info` (bin), `is_hide_graph` (texto), `pickup_user_info` (bin: `pickup_users_group_a`/`_b` listas de bin con `pname`, `pickup_user_group_a_num`, `is_failed_pickup_users`), `is_final_reward`, `is_get_final_reward`, `is_watch_match` (texto), `final_reward_list`, `pickup_reward_list` (bin), `group_result`, `pickup_reward_type` (texto) |
| `JOIN_PESLEAGUE_COMPE` | `compe_id` (int), `team_id` (int) | sin campos propios |
| `GET_PESLEAGUE_COMPE_PARTICIPATION_RIGHT` | `compe_id` (int) | `participation_right` (**bin**): `compe_id`, `compe_name`, `banner_file_info`, `start_date`, `end_date` (int), `need_match_num`, `need_point`, `player_point` (int), `clan_name`, `clan_owner_pid`, `clan_owner_name` (bin `pname`), `is_get` (texto) |
| `GET_PESLEAGUE_COMPE_INFO` | `compe_id` (int) | `compe_data` (**bin**): `compe_id`, `compe_name`, `is_myclub`, `match_type`, `start_date`, `end_date`, **`entry_limit_date`**, `match_end_date` (int), **`entry_user_limit`** (texto), `entry_user_num`, `entry_user_max` (int), **`entry_limit_division`, `entry_limit_division_min`, `entry_limit_division_max`** (texto), `banner_file_info`, `is_exist_overview_info`, `is_exist_participation_right`, `is_exist_rewardInfo` (texto), `compe_db_id`, `use_ball_id` (int), `is_e_sports`, `is_pesleague_stadium`, `is_efootball_pro_stadium`, `entry_compe` (texto), `team_id` (int), `clan_info` (bin como una entrada de `GET_CLAN_LIST`), `compe_regulation` (bin), `eula_url` |
| `GET_PESLEAGUE_COMPE_OVERVIEW_INFO` | `compe_id` (int) | `overview_info` (texto/bin, **N**: el lector no muestra claves internas) |

**Qué decide «la inscripción para este Campeonato está cerrada»** (**P**, el texto del aviso es el de `ERR_COMPE_AFTER_ENTRYTIME`/`BEFORE_ENTRYTIME`; el cliente lo muestra antes de mandar `JOIN`): se evalúa con los datos del elemento de la lista y de `GET_PESLEAGUE_COMPE_INFO`: (1) fecha: `entry_limit_date` (y `start_date`/`end_date`) frente a la hora del servidor; (2) `entry_user_limit`/`entry_user_num` frente a `entry_user_max`; (3) rango de división: `entry_limit_division_min` ≤ **tu división** ≤ `entry_limit_division_max`, donde tu división viene de `GET_PESLEAGUE_MAINMENU_INFO.division` (si esa va vacía el cliente puede considerar que no pertenece al rango → «cerrada»; por eso 64d2a05 manda la división real). Para abrir sin trabas: `entry_limit_date` en el futuro, `entry_user_limit:"NO"`, rango de división que incluya la tuya y `entry_user_num` < `entry_user_max`.

## D.6 Respuestas de `GET_PESLEAGUE_COMPE_TEAM_LIST` y `GET_PESLEAGUE_1VS1_MAINMENU_INFO` (Ghidra solo lectura, 11-nov, L salvo lo marcado)

Prueba en el PES base (60d0374): el Torneo ya deja inscribirse (`JOIN_PESLEAGUE_COMPE{compe_id 12, team_id 174}` → NOERR) y luego manda `GET_PESLEAGUE_COMPE_TEAM_LIST{compe_id}` y `GET_PESLEAGUE_1VS1_MAINMENU_INFO{compe_id}`. Nosotros contestábamos NOERR sin campos. El PES muestra «Se calcularon los resultados finales» y los dos relojes («El período de Matchmaking abre en» / «El campeonato finaliza en») en `-- : --`.

**`GET_PESLEAGUE_COMPE_TEAM_LIST` (Result, lector `FUN_141bdf3b0`)** — claves en la raíz, sin bloques anidados:
| Clave | Tipo | Nota |
|---|---|---|
| `team_list` | lista de **int** (cada elemento int/double → int) | ids de equipo ya usados en el torneo |
| `team_list_num` | int | cantidad |
(más `msgid`, `rqid`, `result`). Con lista vacía y `team_list_num` 0 basta; no es lo que dispara el aviso de «resultados finales».

**`GET_PESLEAGUE_1VS1_MAINMENU_INFO` (Result, lector `FUN_141c36530`)** — TODAS las claves en la raíz (sin bin):
| Clave | Tipo | Dónde va (offset en el objeto) | Lectura probable (**P**) |
|---|---|---|---|
| `matching_first_date` | int | +0xB8 | inicio del primer período de matchmaking (hora) |
| `matching_time` | int | +0xBC | duración/ventana del período |
| `matching_interval` | int | +0xC0 | intervalo entre períodos |
| `compe_end_time` | int | +0xC4 | fin del campeonato (hora) |
| `team_id` | int | +0xC8 | equipo con el que se inscribió |
| `team_name` | texto | +0xD0 | nombre del equipo |
| `point` | int | +0xF0 | puntos |
| `win_count` / `lose_count` / `draw_count` | int | +0xF4 / +0xF8 / +0xFC | G/P/E |
| `match_num` | int | +0x100 | partidos jugados |
| `regulation_match_num` | int | +0x104 | partidos de reglamento (tope) |
| `rank_type` | texto | +0x108 | tipo de ranking |
Los enteros se leen como int o como double (los dos valen). Sin ninguno de ellos todo queda en 0.

**Causa probable de «Se calcularon los resultados finales» (P, sin probar):** al recibir `matching_first_date` = 0 y `compe_end_time` = 0, el cliente trata el campeonato como terminado y enseña los relojes en `-- : --`. La pantalla de la prueba lo confirma: ambos relojes vacíos y `Puntos 0 · G/E/P 0/0/0` (los contadores vienen de `point`/`win_count`… que tampoco mandamos). **Propuesta:** responder `GET_PESLEAGUE_1VS1_MAINMENU_INFO` con `compe_end_time` y `matching_first_date` en el futuro (mismos valores que el `end_date`/`start_date` del torneo en `GET_PESLEAGUE_COMPE_INFO`), `matching_time` y `matching_interval` positivos (p. ej. 3600 y 86400), `team_id`/`team_name` del club inscrito y los contadores en 0, y `regulation_match_num` > 0. Si el aviso persiste, mirar los campos de fecha de `GET_PESLEAGUE_COMPE_INFO` (`start_date`, `end_date`, `match_end_date`) que ya enviamos: deben ser coherentes con esos.
No se halló el texto «resultados finales» en este lector: es una decisión local del cliente a partir de esas fechas (**N**: falta confirmar con una prueba en vivo).
