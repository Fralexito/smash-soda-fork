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

## 13. Estado del progreso guiado por el servidor (C) y pruebas 04-nov 23:30
- CMD_SEND_MYCLUB_PROGRESS_STATUS (cadena 1428a61e8, petición 141dad7b0, lector 141dade70).
  - La petición lleva pid y `status` (texto); la respuesta lleva `next_status` (texto).
  - El cliente no compara esos textos y en .rdata no hay ninguna tabla de etiquetas. Son etiquetas propias del servidor.
- CMD_GET_MYCLUB_IS_FIRST_MEMBERLIST y CMD_GET_MYCLUB_IS_FIRST_AGENT_MULTIPLE: la petición lleva pid y la respuesta `is_first` (YES/NO). Lectores 141dafeb0 y 141daee90.
  - P: los tres comandos se piden ya dentro del menú principal de myClub.
- En vivo: con progress_status «NotImplement» y con «NONE» pasa lo mismo que con el vacío. Llega LEAVE_ONLINE_MODE justo después de SQUADLIST y no aparece ningún comando de creación.
- Siguiente hipótesis (P): el cliente sale porque cree que no hay club. Se prueba fingiendo un club ya creado con `"myclub": {"club_de_prueba": {...}}` en config.json.

## 14. El interruptor de «salir» (C, Ghidra estático, 05-nov)
- La carga de myClub es una máquina de estados (código en 1413d5000..1413d652e, sin funciones definidas en el proyecto recortado).
  - El estado vive en [tarea+0x90]. Cada estado crea un comando por nombre y lo sondea con 1413c7ab0 (0 = pendiente, 1 = ok, otro = error).
  - Orden: UserInfo, Gameplayerlist, Coachlist, Squadlist y después SquadGameplan, SetMyclubMainSquad, CloudSquad, Itemlist, PlayerNumberFixation…
- **Estado 0x15 = salir** (1413d5101). Se llega ahí si un comando no da 1, y también por dos comprobaciones después de SQUADLIST (estado 0x11, 1413d5fbb..1413d6014):
  1. Una estructura que llenan COACHLIST/SQUADLIST no debe ser nula (P).
  2. El número de elementos de un vector de 944 bytes por elemento, que P = la lista de JUGADORES, no debe ser 0.
- Después recorre los elementos: +0x36 == 1 (P: «en plantilla»), +0x1c ≥ 90 (P: valoración o nivel), +8 = id que se busca en la estructura del paso 1.
- **Conclusión:** con 0 jugadores el cliente sale. Hacen falta jugadores, al menos un entrenador y una plantilla que referencie a esos jugadores.
- Implementado: `"myclub": {"jugadores": [...], "entrenadores": [...]}` en config.json arma GAMEPLAYERLIST, COACHLIST y SQUADLIST. main_squad_id = 1. Todo elemento va como bin. Faltan gameplayer_id y coach_id reales del juego.

## 15. ¡myClub abre! (05-nov, 00:10, commit 57cbc5a)
- Con 11 jugadores, 1 entrenador y 1 plantilla, el PES base entra a la «Sede de club»: formación 4-2-3-1, calidad 2197, jugadores 11/250, DT 1/50. Sin error y sin cierre.
- Ids usados (datos del juego, no son secretos):
  - Jugadores: PlayerAssignment.bin de dt10_x64.cpk, team_id 108 (FC Barcelona), filas 0-10: 61672, 8639, 104418, 42892, 40425, 38568, 108662, 36625, 42316, 34881, 43202.
  - Entrenador: coach_id 121552, la 1.ª fila de MyclubCoach.bin.
  - Ejemplo en config.ejemplo.json → "myclub".
- Comandos que llegan después y aún no tienen manejador (la pantalla abre igual):
  - Al cargar: GET_MYCLUB_SQUAD_GAMEPLAN, SET_MYCLUB_MAIN_SQUAD, SET_MYCLUB_CLOUD_SQUAD, GET_MYCLUB_ITEMLIST, GET_MYCLUB_PLAYER_NUMBER_FIXATION, GET_MYCLUB_SCOUTLIST, GET_MYCLUB_WEEKLY_RANKING_BASIC_INFO.
  - Al abrir el menú: GET_MYCLUB_LOGIN_BONUS, GET_MYCLUB_CLOUDMATCH_BONUS, GET_MYCLUB_MAINMENU_INFO.

### 15.1–15.3 (C, lectores leídos 05-nov)
La petición de estos tres solo trae pid. Todo objeto anidado va como bin.
- **GET_MYCLUB_MAINMENU_INFO** (lector 141bfd5f0):
  - Enteros: com_countWin_num, com_match_num, oppo_level, weekly_group_rank, weekly_group_rank_total_num, rank, rank_total_num, rank_coach, rank_coach_total_num, present_num, scount_num, staff_num.
  - Textos: cpu_level (P), is_free_enable, is_coop_enable, is_auction_state_changed.
  - Objetos:
    - achievement_info{achievement_not_recieved_num, achievement_num, achievement_total_num}.
    - agent_panel_file_info y liveevent_panel_file_info, cada uno {filename, hash_str}.
    - matchday_emblem_info{is_use_emblem, is_auto_make_compe_name, matchday_group_type, team_list_group_a [int], team_list_group_b [int], compe_db_id}.
    - game_result_wdl y game_result: forma todavía dudosa, se omiten.
- **GET_MYCLUB_LOGIN_BONUS** (lector 141bf4a40):
  - login_bonus_list = lista de {present_id, name, name_jp, title, body, entity_id, grade, skill{skill_category, skill_id}, type, player_level, contract_term, team_id, present_type, item{item_type, item_variation, item_effect_arg, count, pes_point_price}, receipt_deadline, count}.
  - login_bonus_list_num, today_bonus_number, is_bonus.
- **GET_MYCLUB_ITEMLIST** (lector 141bed520): item_list [{item_type, item_variation, item_effect_arg, count, pes_point_price}] e item_list_num.
- Direcciones de los otros 7 (petición/lector):

  | Comando | Petición | Lector |
  |---|---|---|
  | SQUAD_GAMEPLAN | 141ca7890 | 141ca7ef0 |
  | SET_MAIN_SQUAD | 141bebaf0 | 141bec4a0 |
  | SET_CLOUD_SQUAD | 141bdfef0 | 141be0740 |
  | PLAYER_NUMBER_FIXATION | 141c5c110 | 141c5c710 |
  | SCOUTLIST | 141c3d920 | 141c3df20 |
  | WEEKLY_RANKING_BASIC_INFO | 141c714e0 | 141c71ae0 |
  | CLOUDMATCH_BONUS | 141c424b0 | 141c42b10 |
