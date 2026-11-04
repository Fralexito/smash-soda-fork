# ESPEC myClub (leído de PES2021.exe base, 04-nov-2026)

Lectura estática con Ghidra, solo lectura, sobre una copia del proyecto. PESBUL no tiene nada de myClub: solo 3 tablas vacías y ninguna respuesta.
Marcas: C = leído en el código; P = deducción.

## Método (repetible)
- Cada comando tiene su clase `commandautomake::CmdXxxResult`. El destructor es la entrada 0 de su vtable. Justo después vienen (1) el serializador de la petición y (2) el lector de la respuesta.
- Cómo se reconoce cada tipo en el lector:
  - copia a cadena (fn 14022e4c0) = texto
  - `cvttsd2si` = entero
  - vector (fn 14023d170) = arreglo
  - una nueva entrada al analizador (fn 141c190a0) = contenedor anidado (P: bin(msgpack) o mapa, por confirmar)
- Todas las respuestas llevan msgid (texto), rqid (entero) y result (texto).

## Comandos que el cliente manda al entrar a myClub
1. **GET_MYCLUB_SERVER_ENV**: lector 141cb67c0, petición 141cb60b0.
   - Listas de enteros: license_out_player_list, license_out_coach_list, sell_gp.
   - market_scout_list = lista de {agent_id int, skill{skill_category str, skill_id int}, grade int, count int, rate{rate_table [int], rate_player_num [int]}, target_level int}.
   - is_all_club_edition_set str.
2. **GET_MYCLUB_USER_INFO**: lector 141da5710, petición 141da5110.
   - Enteros: gp, pes_point, base_team_id, team_name_commentary_id, user_base_team_id, user_team_name_commentary_id, norma_point_division, norma_point_sim, norma_point_vscom, nation_id, select_add_uniform_id.
   - Textos: progress_status (enum, valores pendientes), team_name, team_name_short, user_team_name, user_team_name_short, label, name, is_iconic_moment_view.
   - home_town = objeto (claves pendientes); add_uniform_list [int].
   - P: «club sin crear» se deduce de progress_status o de los nombres vacíos. Existen CMD_CREATE_MYCLUB_TEAM, CREATE_MYCLUB_SQUAD y CREATE_MYCLUB_STAFF.
3. **GET_MYCLUB_FEATURE_PLAYER_LIST**: lector 141c5f1d0.
   - gameplayer_list [int], gameplayer_list_num int, hold_gameplayer_list [int], hold_gameplayer_list_num int.
4. **GET_MYCLUB_GAMEPLAYERLIST**: lector 141c91af0.
   - gameplayer_list [jugador], gameplayer_list_num int, gameplayer_list_max int.
   - main_edition_gameplayer_list [jugador], main_edition_gameplayer_list_num int.
   - Campos de cada jugador:
     - Enteros: gameplayer_unique_id, gameplayer_id, games, goal, assist, tiredness, contract_term, gameplayer_cost, rating, sell_gp, contract_gp, contract_pes_point, uniform_number, add_skill_id, default_overall, team_id, nation_id.
     - concept{transition_counter, transition_possession, build_up_long, build_up_short, attacking_area_side, attacking_area_center, mobility_fix, mobility_float, defence_type_forecheck, defence_type_retreat, press_rule_net_defence, press_rule_seal_off, first_df_aggressive, first_df_safety}, todos int.
     - Textos: is_rental, is_subscription, is_protected, default_rarity.
     - Listas de enteros: player_level_need_exp, parameters, enhanced_parameters.
     - level_info{level, level_limit, exp_total, exp_current_level, exp_to_next_level}.
     - redcard_info_list [{redcard_game_kind str, mcountLeft int}].
5. **GET_MYCLUB_COACHLIST**: lector 141c55d50.
   - coach_list_num int.
   - coach_list = lista de entrenadores. Cada uno lleva:
     - Enteros: coach_unique_id, coach_id, contract_term, overall, management, adaptability_rate, sell_gp, contract_gp, contract_pes_point, current_point_division, current_point_sim, current_point_vscom, team_id, nation_id.
     - offensive_main_concept y defensive_main_concept, cada uno con 12 enteros: transition, build_up, attacking_area, mobility, defence_type, press_rule, first_df, passsupport_dist, offence_num, defence_line, defence_compact, defence_num.
     - formation_data str; is_use_variable_formation y is_coach_texture_use str.
6. **GET_MYCLUB_SQUADLIST**: lector 141cac7d0.
   - squad_list y edition_squad_list, con squad_list_num y edition_squad_list_num.
   - Cada plantilla lleva:
     - squad_id int, name str.
     - member_list [{gameplayer_unique_id, gameplayer_id, uniform_number, player_level int; parameters [int]; default_rarity str}], member_num int.
     - is_entry_compe str; coach_id_list [int] y coach_unique_id_list [int].
     - gameplan_data_size int, gameplan_data str; is_club_edition str, club_edition_type str.
   - main_squad_id int, cloud_squad_id int.
7. **GET_MYCLUB_PROCURABLE_GAMEPLAYERLIST**: lector 141d96040.
   - gameplayer_list [{gameplayer_id, level_limit, team_id int; is_draw str}], gameplayer_list_num int.
8. **SET_MYCLUB_CLUB_EDITION_INFO**: lector 141cf1150. Solo lee msgid, rqid y result.
9. **SET_MYCLUB_VSCOM_GAMERESULT**: lector 141baf480.
   - player_data{pid int; pes_pname{name, xuid_one, psn_user_id{account_id, online_id, is_valid_cache}, is_deleted}; rating_type str; rating, oldrating int; rating_boost_type str; rating_boost_rate, rating_compe, team_id int; side str; contWin_num int; skill_rank str; is_rankup_skill_rank, is_first_skillrank, is_first str}.
   - gameend_reason str.
10. **REQWATCH_MYCLUB_AUCTION_REFUNDED_GP**: lector 141bb4220.
    - refunded_gp_list [{product_id int, skill{skill_category str, skill_id int}, gp int}], total_gp int.

## 11. Prueba en vivo (04-nov, 23:18, eb50e54)
- Al entrar a myClub, el orden fue: VSCOM_GAMERESULT → SERVER_ENV → USER_INFO → PROCURABLE → AUCTION_REFUNDED_GP → SET_CLUB_EDITION_INFO → FEATURE_PLAYER_LIST → GAMEPLAYERLIST → COACHLIST → SQUADLIST.
- Todas se aceptaron, sin cierre y sin error. Un segundo después el cliente manda LEAVE_ONLINE_MODE y vuelve al menú principal.
- No llegó ningún CREATE_MYCLUB_*. P: con progress_status vacío el cliente no abre la pantalla de crear club.

## 12. Creación del club y regla de anidados (C)
- **CREATE_MYCLUB_TEAM**: lector 141d8eba0, petición 141d8dd20.
  - La petición lleva pid, team_name, team_name_short, base_team_id, coach_id, rental_gameplayer_id, is_check_team_name, is_check_team_name_short, is_coach_texture_use, team_name_commentary_id, home_town{nation_id, label, name}, nation_id, label y name.
  - La respuesta solo lleva msgid, rqid y result. Errores posibles: ERR_ALREADYEXISTS, ERR_DATABASE.
- **CREATE_MYCLUB_SQUAD**: lector 141bc0500.
  - La petición lleva pid, coach_unique_id y name.
  - La respuesta lleva squad_id int.
- **CREATE_MYCLUB_STAFF**: lector 141c96dd0.
  - La petición lleva player_info_list [{gameplayer_unique_id, gameplayer_base_id, gameplayer_id, gameplayer_level, registared_position, position, stronger_foot, overall, ability, play_style, nation_id, club_team_id, is_use_cloud_match}].
  - La respuesta lleva staff_info_list [{id, base_player_info{las mismas 12}, base_exp}].
  - Errores posibles: ERR_DATABASE, ERR_COMMAND_BLOCKED, ERR_TEAM_MEMBER_LACK, ERR_TEAM_STAFF_FULL, ERR_PLAYER_PROTECTED.
- **home_town** = {nation_id int, label str, name str}.
- **Lector anidado** 141c190a0: envuelve un decodificador msgpack genérico (1421a5e30), en el que str y bin comparten el tipo 5.
  - Los lectores de myClub releen cada clave anidada desde un nodo de tipo 5.
  - **Regla:** todo objeto anidado y todo elemento de una lista de objetos va como bin(msgpack(mapa)). Las listas de enteros van en línea.
- **progress_status**:
  - El valor por defecto del objeto es «NotImplement».
  - Que el club no exista no es un error: viene dentro de un NOERR.
  - El consumidor del valor todavía no está ubicado. Clases de interfaz relacionadas: OnlineMyClubTeamSelect, OnlineMyClubTeamNameSetting, OnlineMyClubTeamUniform/Selector, OnlineMyClubLoginBonus, MyClubMainMenu*.

## Implementado (paso 1, logica.py)
- Respuestas vacías, pero con todas las claves y sus tipos. home_town y player_data van como bin.
- CREATE_MYCLUB_TEAM, CREATE_MYCLUB_SQUAD y CREATE_MYCLUB_STAFF guardan el club en la tabla `myclub` (JSON por cuenta).
- progress_status sale de config.json: `"myclub": {"progreso_sin_club": "...", "progreso_con_club": "...", "gp_inicial": 0}`. Así se prueban valores candidatos sin tocar el código.

## Pendiente
- Valores del enum progress_status y el flujo para crear el club.
- Claves de home_town.
- Confirmar en vivo si los anidados van como bin o como mapa.
- Club inicial: elegir los gameplayer_id de la base de datos del juego.
