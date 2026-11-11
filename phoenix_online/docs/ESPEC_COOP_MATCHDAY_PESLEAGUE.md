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

## D.6a Respuestas de `GET_PESLEAGUE_COMPE_TEAM_LIST` y `GET_PESLEAGUE_1VS1_MAINMENU_INFO` (Ghidra solo lectura, 11-nov, L salvo lo marcado)

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

## D.7 Qué llena los textos de la pantalla principal de Matchday (Ghidra solo lectura, 11-nov) (L salvo lo marcado)
Prueba en el PES base con 5ea6c3d: `GET_MATCHDAY_INFO` y `JOIN_MATCHDAY` salen bien (compe_id 11), pero la pantalla principal (`GET_MATCHDAY_MAINMENU_INFO`, 508 bytes) sale con textos vacíos («- - -», sin bando, sin ranking, sin premios). Lector del resultado: `FUN_141ceaaa0` (17 220 bytes); lector de `GET_MATCHDAY_INFO`: `FUN_141cc3cd0` (29 782 bytes). Claves obtenidas de las cadenas que cada función referencia, en orden de lectura; el **anidado** es **P** (por el orden y los bucles de decodificación).

**`GET_MATCHDAY_MAINMENU_INFO`** — raíz: `rqid`, `msgid`, `result`, **`mainmenu_info`** (bin). Dentro:
- `matchday_progress` (texto), `group_match_info` (bin), `is_hide_graph`, `pickup_user_info` (bin), `is_final_reward`, `final_reward_list` (lista), `group_result` (texto), `is_get_final_reward`, `is_watch_match`, `pickup_reward_type` (texto), `pickup_reward_list` (lista).
- **`matchday_progress` — tokens válidos (todos existen como cadenas del cliente):** `MATCHDAY_PROGRESS_GROUP_MATCH`, `MATCHDAY_PROGRESS_END_GROUP_MATCH`, `MATCHDAY_PROGRESS_PICKUP_USERS`, `MATCHDAY_PROGRESS_PICKUP_MATCH_MATCHING`, `MATCHDAY_PROGRESS_PICKUP_MATCH`, `MATCHDAY_PROGRESS_END_PICKUP_MATCH`, `MATCHDAY_PROGRESS_RESULT`. Nuestro `""` no es ninguno de ellos: es la causa más probable de los textos vacíos (**P**; empezar por `MATCHDAY_PROGRESS_GROUP_MATCH`).
- **`group_match_info` (bin):** `matchday_point`, `matchday_rank`, `matchday_ranking_user_num`, `matchday_ratio`, `total_matchday_point_group_a`, `total_matchday_point_group_b`, `pickup_match_score_group_a`, `pickup_match_score_group_b`. Hoy mandamos `{}`.
- **`pickup_user_info` (bin):** `pickup_users_group_a` y `pickup_users_group_b` (listas; cada usuario: `pid`, `pname`, `name`, `xuid_one`, `psn_user_id`, `account_id`, `online_id`, `is_valid_cache`, `is_deleted`), `pickup_user_group_a_num`, `pickup_user_group_b_num`, `is_failed_pickup_users`.
- **`group_result`/bando:** tokens `MATCHDAY_GROUP_A`, `MATCHDAY_GROUP_B`, `MATCHDAY_GROUP_NONE` (**P**: también valen para `joined_matchday_group`).
- **Elemento de premio** (en `final_reward_list` y `pickup_reward_list`): `present_id`, `name`, `name_jp`, `title`, `body`, `entity_id`, `grade`, `skill` (bin), `skill_category`, `skill_id`, `type`, `player_level`, `contract_term`, `team_id`, `present_type`, `item` (bin) con `item_type`, `item_variation`, `item_effect_arg`, `count`, `pes_point_price`, y `receipt_deadline`.
- `pickup_reward_type`: `MATCHDAY_PICKUP_REWARD_WINLOSE`, `MATCHDAY_PICKUP_REWARD_COMMON`, `MATCHDAY_PICKUP_REWARD_NONE`.

**`GET_MATCHDAY_INFO`** (`compe_info` → `info_overview`): `compe_id`, `base_id`, `compe_name`, `is_disp_compe_name`, **`group_info_a`** y **`group_info_b`** (cada uno con `team_list` (lista), `team_list_num`, `group_name`, `group_desc`, `is_entry_restricted`, `pre_pickup_users_pid_list`), `match_type`, `squad_type` (`MATCHDAY_SQUAD_MYCLUB`/`MATCHDAY_SQUAD_SPECIAL`), `group_type` (`MATCHDAY_GROUP_TYPE_OTHER`/`NATIONAL`/`LEAGUE`/`TEAM`), `rating_rank` (`MATCHDAY_RATING_RANK_ELITE`/`PRO`/`ACADEMY`/`NONE`), fechas (`start_date`, `entry_limit_date`, `end_group_match_date`, `pickup_member_date`, `end_date`), `entry_user_num`, `entry_user_max`, `is_end`, `banner_file_info`, `list_banner_file_info`, `is_use_emblem(_list)`, `is_entry`, `watch_match_reward_state`, `compe_db_id`, `is_possible_watch`, `start_match_limit_date`, `entry_pickup_match_start_date`, `watch_match_start_date`, `is_check_eula`, `eula_url`, `ball_id`, `group_match_time`, `pickup_match_time`, `watch_wait_time`, `use_stadium_id`, `fixed_timezone`, `fixed_weather`, `fixed_season`, `is_fix_condition`, `is_equalization`, `is_pesleague_stadium`, `is_efootball_pro_stadium`, `joined_matchday_group`, `pickup_select` (`MATCHDAY_PICKUP_SELECT_RANDOM`/`RANKING`), `team_id`, `pickup_cut_off_rank`, `all_reward_info` (con `achievement_reward_list` (`is_repeat`, `require_point`, `reward_list`), `required_point_final_reward`, `win_reward_list`, `lose_reward_list`, `watching_reward_list`, `pickup_win_reward_list`, `pickup_lose_reward_list`, `pickup_reward_type`), `squad_rating_info` (`sp_squad_rating`, `rating_kind_player_bonus_value_list`, `evaluation_a`, `evaluation_b`, `superiority_border_ratio`). Los elementos de premio llevan las mismas claves que arriba.

**Propuesta:** en `c_cmd_get_matchday_mainmenu_info` mandar `matchday_progress:"MATCHDAY_PROGRESS_GROUP_MATCH"`, `group_match_info` con las 8 claves a 0, `pickup_user_group_b_num: 0`, `group_result:"MATCHDAY_GROUP_A"`, `pickup_reward_type:"MATCHDAY_PICKUP_REWARD_NONE"`. Si aún sale vacío, probar los demás tokens de uno en uno (config) y mirar el nombre del bando en `group_info_a.group_name`.

## D.8 Clanes y salas cooperativas: pedido y respuesta (Ghidra solo lectura, 11-nov) (L salvo lo marcado)
Método: Ghidra 12.0.4 headless, solo lectura, `PES2021_recortado.exe`; para cada comando, el constructor del pedido y el lector del resultado (`msgid`, `rqid`, `result` van siempre; no se repiten). Clave común **`user_info`/`owner_info`** (bin de usuario): `user_id`, `pname`, `xuid_one`, `psn_user_id`, `account_id`, `online_id`, `is_valid_cache`, `is_deleted`; en listas de miembros se añaden `coop_level`, `is_follow`, `last_login`, `current_online_mode`, `is_join_online_mode`.

**Clanes**
| Comando | Pedido | Respuesta (aparte de `result`) |
|---|---|---|
| `CREATE_CLAN` (`FUN_141de82f0`) | `clan_name` | solo `result` |
| `SET_CLAN_SETTINGS` (`FUN_141de9210`) | `clan_basic_settings` (bin: `clan_name`, `owner_info`, `enable_request`, `clan_comment`, `is_banned`) | solo `result` |
| `GET_CLAN_SETTINGS` (`FUN_141deb550`) | — | `clan_basic_settings` (las mismas claves) |
| `GET_CLAN_INFO` (`FUN_141dedec0`) | `owner_id` (+ datos de usuario) | `clan_info` (bin): `basic_settings` (`clan_name`, `owner_info`, `enable_request`, `clan_comment`, `is_banned`), `clan_members` (lista) y `clan_member_num`, `clan_stats_online` (`current_division`, `current_rating`, `season_match_stats`{`match_count`, `win_count`, `draw_count`, `lose_count`, `win_s`, `win_s_max`, `lost_goal`}, `total_match_stats`, `season_win_count`, `residual_div_first_count`), `clan_stats_myclub` |
| `GET_CLAN_SIMPLE_INFO_LIST` | por dueños: `owner_pid_list`; búsqueda: `search_options` (`clan_name`, `enable_request`, `is_joined_clan`), `start_index` | por dueños: `clan_simple_info_list` (+`clan_simple_info_list_num`, elementos con `clan_name`, `owner_info`); búsqueda: `clan_list` (como `clan_info`), `clan_list_num`, `all_list_num`, `start_index` |
| `GET_CLAN_REQUEST_LIST` / `GET_CLAN_PENDING_LIST` | — | `request_info_list` / `pending_info_list` (cada elemento: `status`, `is_new`, `appliciant_user_id` + datos de usuario, `is_follow`, `applicant_clan_name`) y `…_list_num` |
| `ENTRY_CLAN` | `target_pid` | solo `result` |
| `INVITE_CLAN` | `target_id` (+ datos de usuario) | solo `result` |
| `LEAVE_CLAN` | `owner_pid` | solo `result` |
| `EXPEL_CLAN_MEMBER` | `target_pid` | solo `result` |
| `GET_CLAN_MEMBER_MODE` | — | `clan_members` (lista de miembros con los datos de usuario ampliados) y `clan_member_num` |
| `GET_COOP_MAINMENU_INFO` | `online_mode`, `is_check_season_end` | `is_coop_banned`, `is_season_end`, `division`, `pre_season_rank`, `pre_season_rank_all_clan_num`, `division_change_type`, `is_exist_owner_clan`, `is_exist_new_request`, `is_exist_new_pending` |
| `FOLLOW_USER` / `GET_FOLLOW_LIST` / `LOCK_FOLLOW_USER` | `follow_user_id_list`+`unfollow_user_id_list` / — / `target_pid`+`is_lock` | solo `result` / `follow_list` (`user_id`+usuario, `is_locked`, `is_clan_member`) y `follow_list_num` / solo `result` |
| `SET_COOP_REJECT` / `GET_COOP_REJECT_LIST` | `set_reject_pid_list`, `unset_reject_pid_list` / — | `reject_list` (usuarios) y `reject_list_num` |

**Salas cooperativas**
| Comando | Pedido | Respuesta |
|---|---|---|
| `GET_COOP_ROOMLIST` | `search_options` (`room_id`, `use_password`, `vs_setting`, `team_level`, `coop_level`, `coop_manner`, `cpu_level_min`, `cpu_level_max`, `can_entry`, `compe_id`), `start_index` | `room_list` (cada sala: `basic_settings`{`room_id`, `room_name`, `clan_name`, `clan_owner_info`, `is_follow`, `is_com_match`, `is_use_chat`, `is_same_level`, `team_level`, `coop_level`, `is_same_manner`, `coop_manner`, `room_entry_restriction`, `password`}, `match_settings`{`user_team_id`, `enemy_team_id`, `cpu_level`}, `user_num`, `compe_id`, `is_game`), `room_list_num`, `all_room_num` |
| `CREATEJOIN_COOP_ROOM` | `basic_settings` y `match_settings` (las claves de arriba) | `room_id` |
| `JOIN_COOP_ROOM` | `room_id`, `password`, `is_invite` | solo `result` |
| `LEAVE_COOP_ROOM` | `room_id` | solo `result` |
| `SET_COOP_MATCH_READY` | `room_id`, `is_ready` | solo `result` |
| `SET_COOP_ROOM_OPTION` | `room_id`, `basic_settings`, `match_settings` | solo `result` |
| `GET_COOP_ROOM_OPTION` | `room_id` | **P** (reader no abierto): `basic_settings` + `match_settings` como en `SET` |
| `GET_COOP_ROOM_MEMBER` | `room_id` | `list_data` (cada miembro: `player_index`, `user_info`, `is_owner`, `coop_level`, `is_clan_member`, `is_follow`, `is_guest`, `guest_info`, `is_platform_guest`, `is_ready`), `list_num` |

**Respuestas mínimas para que abran sin fallo (P):** `GET_CLAN_INFO` sin clan → `ERR` propio no visto; mandar `clan_info` con `clan_members: []`, `clan_member_num: 0` y `basic_settings` con `clan_name` vacío; `GET_COOP_ROOMLIST` → `room_list: []`, `room_list_num: 0`, `all_room_num: 0`; `GET_COOP_MAINMENU_INFO` → los booleanos en `"NO"`, `division` texto de división (la de PES League, p. ej. `"D12"`), `division_change_type` token por descubrir. Los booleanos de estos comandos viajan como texto (`"YES"/"NO"`, como en el resto de la familia).

### D.7b Prueba en vivo de `matchday_progress` (11-nov, PES base; 04e2a97 instalado) (L: observado en pantalla)
Método: parche local de prueba (el servidor lee el token de un archivo en cada `GET_MATCHDAY_MAINMENU_INFO`; el cliente lo vuelve a pedir cada ~30 s). Los textos salen en los avisos del PES, a veces un ciclo de 30 s después del cambio.
| Token enviado | Qué hace la pantalla de Matchday |
|---|---|
| `MATCHDAY_PROGRESS_GROUP_MATCH` | «Finalizó la fase de partidos de grupo. Se están recopilando los resultados. Espera a que se abra la entrada en la gran final.» + «Resultados de la fase de partidos de grupo en 2505575 s». «Empezar partido» en gris. **No es la fase de jugar.** |
| `MATCHDAY_PROGRESS_END_GROUP_MATCH` | Igual que el anterior («Finalizó la fase de partidos de grupo»). |
| `MATCHDAY_PROGRESS_PICKUP_USERS` | Aviso «Se recopilaron los resultados. Veamos los resultados de la fase de partidos de grupo.»; el botón principal pasa a «A la gran final»; marcador 0 – 0 con barra azul/rosa, «Tus estadísticas» (Puntos de evento 0, Progreso recomp. result. Completado, Clasificación del bando –, Condición representante –) y «Ya está abierta la entrada a la gran final para los usuarios elegibles…»; al entrar: «Esta vez no te ganaste el derecho a inscribirte en la gran final. ¡Pero puedes ver el partido como espectador…!» |
| `MATCHDAY_PROGRESS_PICKUP_MATCH` | Aviso «Se cerró la ventana de entrada a la gran final. ¡Pero puedes ver el partido como espectador!» |
Sin probar aún: `PICKUP_MATCH_MATCHING`, `END_PICKUP_MATCH`, `RESULT`. **Conclusión:** con las 8 claves de `group_match_info`, bandos y fechas escalonadas, ningún token probado deja «Empezar partido» activo. **P:** el estado jugable depende de un dato que aún no mandamos: que el club tenga un grupo/partido de grupo asignado (`group_result`/`joined_matchday_group`, pickup, `team_list` de `group_info_a/b`) o de `start_match_limit_date` frente a la hora del cliente. Siguiente: probar con `joined_matchday_group = MATCHDAY_GROUP_A` y `team_list` con el equipo del club en `group_info_a`.

## D.9 Cierre del PES al crear un clan: `user_id` es un bin, no un entero (volcado 11-nov, 07:15:50) (L)
- **Síntoma:** Cooperativo → Opciones → Crear/Editar clan → «¿Crearás tu propio clan?» Sí. Llega `CREATE_CLAN{clan_name}` → NOERR y enseguida `GET_CLAN_SETTINGS`; 6 s después el PES desaparece.
- **Volcado** (`CrashDumps\PES2021.exe.26568.dmp`): excepción `0xC0000005`, lectura de `0xFFFFFFFFFFFFFFFF` en **`msvcrt.dll+0x7B212`** (copia con puntero −1). La pila del hilo tiene la dirección de retorno **`0x141DEBDA8`**, dentro del lector de `GET_CLAN_SETTINGS` (`FUN_141deb550`), entre las comprobaciones de las claves `user_id` (0x141debcee) y `pid` (0x141debdde).
- **Causa (leída en la decompilación):** en `clan_basic_settings.owner_info` la clave **`user_id` se decodifica como bin** (`FUN_141c190a0`) y de él se lee la clave `pid` (**entero de 32 bits**, se guarda en el objeto +0xD8). Mandábamos `user_id` como entero de 64 bits (`292733975847239680`): el cliente lo trataba como bin y se cerraba. Lo mismo ocurre en todos los lectores que usan el bloque de usuario: `GET_CLAN_INFO`, `GET_CLAN_SIMPLE_INFO_LIST`, `GET_CLAN_MEMBER_MODE`, `GET_FOLLOW_LIST` y `appliciant_user_id` de las listas de solicitudes (todos comprobados: bin).
- **Formato correcto:** `user_id: bin{ pid: int32 }` con `pid` = los 32 bits bajos del número de cuenta (p. ej. `0x0410000004100000` → `68157440`, el mismo `pid` que el cliente manda en cada pedido). `pname` sigue siendo `bin{name, xuid_one, psn_user_id{account_id, online_id, is_valid_cache}, is_deleted}`. Arreglo en `_usuario_clave` (logica.py).
- **Otros campos de `owner_info` sin mandar aún (P):** `coop_level`, `is_follow`, `last_login`, `current_online_mode`, `is_join_online_mode` (los pide el lector; faltar no cierra el juego).

### D.9b Clanes probados en el PES base tras el arreglo (11-nov, 07:35; servidor con a0347ca) (L: observado)
- **Crear clan:** Cooperativo en línea → «Opciones de Cooperativo» → «Crear/Editar clan» → «¿Crearás tu propio clan? Sí». Pedido real: `CREATE_CLAN{clan_name:"Clan de FRALEX"}` (nombre por defecto «Clan de» + el nombre del jugador); sin cierre una vez `user_id` es bin.
- **El bloque de usuario es `user_id = bin{ pid:int32, pname: bin{ name, xuid_one, psn_user_id{account_id, online_id, is_valid_cache}, is_deleted } }`** (el PES lo manda así en `GET_CLAN_INFO.owner_id`). Con `pname` al lado de `user_id` el clan se creaba pero «Dueño» salía «NotImplement». Corregido en a0347ca; ahora «Tu lista de clanes» muestra «Clan de FRALEX — Dueño FRALEX».
- **«Perfil del clan»** (`GET_CLAN_INFO`): Dueño FRALEX, Estado «Reclutando», Miembros/Capacidad 1/11, Comentario «No registrado».
- **«Editar clan»** (`GET_CLAN_SETTINGS`): Nombre, «Solicitar» (Aceptar/Declinar, se cambia con Izquierda/Derecha) y Comentario; al salir llega `SET_CLAN_SETTINGS` (NOERR). No se probó escribir el comentario (hace falta teclado en pantalla).
- **«Buscar clan»** → «No hay datos» (correcto: no hay otro clan; la búsqueda no devuelve el propio). **«Solicitudes recibidas»** → «No hay solicitudes recibidas». Sin «sin manejador» ni cierres en estas pantallas.
- Pendiente de probar: «Solicitudes hechas», «Lista de seguimiento», «Lista de rechazados», entrar a un clan ajeno (`ENTRY_CLAN`) y «Partido de clan».

## D.10 Salas cooperativas en el PES base: qué llega y por qué la pantalla queda en blanco (11-nov, 08:07–08:26) (L observado / P hipótesis)
- **Ruta:** Cooperativo en línea → **Partido de clan** («Partido informal» es solo matchmaking `Opciones de Matchmaking`) → «Lista de Salón de juego» (vacía, botón «Crear Salón de juego ()») → «Opciones de Salón de juego» (Nombre de la sala «Salón de juego de », Opciones de rival 3 vs. 3 / 3 vs. COM, Chat Sí, Nivel de fortaleza, Nivel de Cooperativo, Valorac. deportiv., Seguridad, Contraseña) → Siguiente → «Ficha y opciones de Nivel de partido» (Usuario Al azar, Rival COM, Nivel) → Siguiente. La UI va con un paso de retraso: pulsar `Abajo` una vez cada 1,5 s y comprobar el foco antes de Enter.
- **Comandos en orden:** `GET_COOP_ROOMLIST` (vacía OK) · `CHECK_STRING{str:"Salón de juego de "}` NOERR · **`CREATEJOIN_COOP_ROOM{basic_settings, match_settings}` → `{room_id}`** · `GET_COOP_ROOM_OPTION{room_id}` · **`REQWATCHL_COOP_ROOM_MEMBER{room_id}`** (antes «sin manejador») · `GET_CLAN_INFO`. También `GET_COOP_QUICKMATCHOPTION` (sin manejador, solo al entrar en «Partido informal»).
- **`REQWATCHL_COOP_ROOM_MEMBER`:** lector `FUN_141e29680`; respuesta = lista vigilada `list_type` (texto libre, mandamos `"ALL"`), `list_num`, `list_key_name` (`"player_index"`) y `list_data` (miembros: `player_index`, `user_info`, `is_owner`, `coop_level`, `is_clan_member`, `is_follow`, `is_guest`, `guest_info`, `is_platform_guest`, `is_ready`). Implementado en 17c06ad.
- **Forma del bloque de usuario según el lector:** `user_info` (miembros de sala), `reject_list` y `clan_owner_info` = `bin{pid, pname{…}}` **directo**; `owner_info`, `clan_members` y `follow_list` = `bin{user_id: bin{pid, pname{…}}, …}`. Con la forma equivocada en la sala el nombre no se lee. Corregido en 89e14fa.
- **Resultado:** con ambos arreglos la pantalla **sigue en blanco** (el PES no cae ni manda más comandos; sin volcado). **P:** la sala espera un empuje del servidor que no mandamos (candidatos en el exe: `CMD_WATCH_COOP_ROOM_OPTION` (`FUN_141e23980`), `CMD_WATCH_BREAKUP_COOP_ROOM` (`141e26730`), `CMD_WATCH_COOP_ROOM_TO_MATCHING_MEMBER` (`141e274f0`)) o un `list_type` concreto distinto de `"ALL"`. Siguiente paso: leer `FUN_141e28470` (pedido, trae `player_index`) y el lector que consume el empuje; probar `list_type` = `"INIT"/"ADD"/"FULL"` por config (`myclub.coop_lista_tipo`).

### D.7c Matchday: más pruebas en vivo (11-nov, 11:30–12:15, PES base con 04e2a97) — «Empezar partido» sigue en gris
Tokens que faltaban de `matchday_progress` (L, observado): **`PICKUP_MATCH_MATCHING`, `END_PICKUP_MATCH` y `RESULT`** pintan todos la misma pantalla: título «Ver la gran final», botón «Ver el partido», marcador «VS» vacío, aviso «Se cerró la ventana de entrada a la gran final. ¡Pero puedes ver el partido como espectador!» y «La gran final empieza dentro de 41771 min». Sólo `GROUP_MATCH` y `END_GROUP_MATCH` muestran el botón «Empezar partido» (gris) con «Finalizó la fase de partidos de grupo… Espera a que se abra la entrada en la gran final»; `PICKUP_USERS` → «A la gran final» (D.7b).
Cambios probados SIN efecto sobre el estado «grupo finalizado»:
1. `team_list` de `group_info_a/b` con los equipos inscritos (el club figura en el bando A con `team_id 174`, que el PES manda en `JOIN_MATCHDAY{entry_matchday_group:"MATCHDAY_GROUP_A", team_id:174}`) — a3aa872.
2. Manejador de **`GET_MATCHDAY_SP_GAMEPLAN`** (`gameplan_data_size`, `gameplan_data`, `sp_member_list`; el PES manda `sp_gameplan_data` y `sp_member_list` en el JOIN, ahora se guardan) y `SET_MATCHDAY_SP_GAMEPLAN` — 077ab34.
3. `group_match_time` de 2 días a 60 días (hipótesis: fase = `start_date` + `group_match_time`).
El contador «Resultados de la fase de partidos de grupo en N s» sigue siendo ≈ `entry_pickup_match_start_date` − ahora (29 d), y «Part. grupo acaba en 28d0h» de la tarjeta sale de `end_group_match_date`: el cliente lee bien las fechas y aun así decide «finalizó». **P:** el estado jugable depende de un dato que viene por otro comando o de un contador de partidos de grupo del usuario (p. ej. algún campo de `group_match_info`/`matchday_ranking_user_num` distinto de 0, o `is_watch_match`), no de las fechas ni del token. Siguiente intento: decompilar el lector del panel principal (consumidor de `matchday_progress`, `FUN_141ceaaa0` guarda el texto en +0xB8) y buscar qué activa el botón.

### D.10b Sala cooperativa: `list_type` probado (11-nov, 12:30)
Con `REQWATCHL_COOP_ROOM_MEMBER` respondido con miembros en forma `{pid, pname}` correcta, la pantalla de la sala sigue en blanco con `list_type` = `"ALL"` y `"LIST_ADD"`; no responde a Esc (hay que cerrar el PES). Los textos `RELOAD`/`UPDATE` del exe pertenecen a la descarga (`DL_*`), no a `list_type`. Sin probar: cadena vacía y empujes `WATCH_COOP_ROOM_OPTION`/`WATCH_COOP_ROOM_TO_MATCHING_MEMBER`. Pendiente de lectura en Ghidra: el consumidor del objeto que guarda `list_type` (+offsets del lector `FUN_141e29680`) y el proceso de la pantalla «sala» (`process::…CoopRoom…`).

### D.7d Matchday: quién deja gris «Empezar partido» y dice «Finalizó la fase de partidos de grupo» (11-nov, 12:50; Ghidra + memoria en vivo, PES base)
**Causa (confirmada).** `FUN_14105d370` (lector de `matchday_progress`) convierte el texto en estado con `FUN_14105aea0`. La tabla de nombres está en `exe+0x37F3698` (se leyó en vivo): 0 `GROUP_MATCH`, 1 `END_GROUP_MATCH`, 2 `PICKUP_USERS`, 3 `PICKUP_MATCH_MATCHING`, 4 `PICKUP_MATCH`, 5 `END_PICKUP_MATCH`, 6 `RESULT` (3 y 4 dan el estado 4). **Si llega `GROUP_MATCH` (0), el cliente lee `sesión+0x7F4` (uint, hora) y, si es `<=` la hora actual (`FUN_14115fe20`), cambia el estado a 1 = `END_GROUP_MATCH`.** El mismo chequeo está en la máquina de estados de la pantalla (`FUN_14112d390`). Con estado 1 la pantalla muestra «Finalizó la fase…», «Espera a que se abra la entrada en la gran final» y el botón queda gris.
**Prueba en vivo.** Sesión = `[[exe+0x36FCED8]+0x128]`; `+0xC58` = objeto de Matchday (primer uint = estado: **1** aunque mandamos `GROUP_MATCH`). Con fechas centinela distintas por clave, el bloque `sesión+0x7F0..0x800` quedó: `+0x7F0 = start_match_limit_date`, **`+0x7F4 = 0`**, `+0x7F8 = entry_pickup_match_start_date`, `+0x7FC = entry_pickup + 90`, `+0x800 = watch_match_start_date`. `end_group_match_date` y `pickup_member_date` no se guardan ahí (la cuenta atrás «Resultados… en N s» sí usa `end_group_match_date`: 1581209 s exactos). Cambiar `group_match_time`, `pickup_match_time` y `watch_wait_time` (7000001/1234/5678) **no** movió `+0x7F4` ni `+0x7FC`.
**Conclusión.** El campo que decide es `+0x7F4` y vale 0 (siempre «ya pasó»). No viene de ninguna clave de fecha de `compe_info` que mandemos, ni de `MAINMENU_INFO` (su lector, `FUN_141ceaaa0`, no tiene ninguna fecha). Sigue sin encontrarse quién lo escribe (escritura por copia de estructura, `FUN_1407a3490`); falta un punto de parada de escritura de hardware en `+0x7F4` (x64dbg) o la prueba de escribir un valor futuro con `pes_memoria_escribir` para confirmar que el botón se activa.
**Respuesta exacta que hay que mandar:** aún sin determinar (se sabe qué campo del cliente hay que dejar en el futuro, no qué clave del servidor lo llena). Descartadas: listas de recogida con el propio usuario, `team_list`, `SP_GAMEPLAN`, todas las fechas y duraciones de `compe_info`.

### D.6b Torneo en línea: período de Matchmaking abierto (11-nov 13:55, prueba en el PES base)
`matching_time` y `matching_interval` van en **segundos**. Con ambos = 86400 el PES mostraba «Espera a que abra el período de Matchmaking · abre en 59:42» (sin botón); con **3600 / 3600** (`matching_first_date` = hora de inicio en el pasado) la pantalla pasa a **«Selecciona "Empezar partido" para que comience el partido»** y «El período de Matchmaking cierra en 0:59:41». Defecto del servidor ahora 3600 (config `matching_interval` / `matching_time` por torneo lo cambian). Nombre de equipo confirmado: «Nom. del equipo ANDERLECHT».
