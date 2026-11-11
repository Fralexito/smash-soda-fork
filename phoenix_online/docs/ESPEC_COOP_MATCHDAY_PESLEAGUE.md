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
