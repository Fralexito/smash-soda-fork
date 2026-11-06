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

### 15.4–15.10 (C)
- **SQUAD_GAMEPLAN**: la petición lleva squad_id; la respuesta gameplan_data_size int y gameplan_data texto.
- **SET_MYCLUB_MAIN_SQUAD**: la petición lleva matchday_squad_rating_info, squad_id, team_level, team_power y team_spirit; la respuesta solo msgid, rqid y result.
- **SET_MYCLUB_CLOUD_SQUAD**: la petición lleva cloud_squad_id, is_use_cloud_match, team_level y team_power; la respuesta solo msgid, rqid y result.
- **PLAYER_NUMBER_FIXATION**: player_number_list [{player_id, number}] y player_number_list_num.
- **SCOUTLIST**: scout_list [{agent_id, skill{skill_category, skill_id}, grade, count, rate{rate_table [int], rate_player_num [int]}, target_level, sell_gp}] y scout_list_num.
- **WEEKLY_RANKING_BASIC_INFO**: basic_info{eura_url, is_beginner, is_join, is_rejected, is_season_end, reward (regalo), division, division_change_type, is_no_play_season, is_group_rank_1st}.
- **CLOUDMATCH_BONUS**:
  - cloud_bonus (regalo); match_num, win_num, lose_num y draw_num.
  - match_list = lista de {date, ex_type, pk_flag, no_match, player{pid, pname{…}, side, is_com, cpu_level, is_follow, is_guest, team}, score{score_1st, score_2nd, score_ex1st, score_ex2nd, score_pk}, match_category, gameend_reason}; match_list_num.
- **«Regalo»** = {present_id, name, name_jp, title, body, entity_id, grade, skill{…}, type, player_level, contract_term, team_id, present_type, item{item_type, item_variation, item_effect_arg, count, pes_point_price}, receipt_deadline, count}.

### Cómo se sacaron los ids del juego (repetible)
- Jugadores: en Data\dt10_x64.cpk, el archivo common/etc/pesdb/PlayerAssignment.bin.
  - Cabecera FF 10 81 «WESYS» y después zlib.
  - Filas de 16 bytes = 4 × u32 LE: [fila][player_id][team_id][puesto<<10 | número].
- Entrenadores: MyclubCoach.bin, 21 filas de 192 bytes con [índice 6..27][coach_id].

## 16. Mercado y subastas (C, 05-nov)
- **GET_MYCLUB_MARKET_AGENTLIST** (lector 141c20550):
  - market_agent_list = [{agent{agent_id, name, name_jp, target_level, type, token_list [{level, count}], token_list_num, bgm_id, uniform, get_player_num, is_draw_checked, is_buy_gp_pes_point, pes_point, agent_end_time, agent_token_end_time, banner_file_info{filename, hash_str}, skill{…}, limit_count, use_count}}]. P: cómo se agrupan exactamente.
  - market_agent_list_num; market_subscription{end_time}.
  - Sin respuesta, la pantalla Representantes vuelve al menú.
- **GET_MYCLUB_MARKET_COACHLIST** (lector 141d93950):
  - market_coach_list = [{coach_id, pes_point, overall, offensive_main_concept{12 int}, defensive_main_concept{12 int}, formation_data, is_locked, management, adaptability_rate, is_use_variable_formation}].
  - market_coach_list_num, now_time, end_time.
  - Los DT en venta salen de config myclub.mercado_dt (ids de MyclubCoach.bin).
- **GET_MYCLUB_REFINE_SEARCH_AUCTION_LIST** (lector 141cd53f0):
  - La petición lleva refine_search_auction{filtros} y start_index.
  - La respuesta lleva auction_info_list [{product_id, scout_id, grade, skill{…}, price_now, bid_num, time_limit, finalround_end_time, auction_state, is_bid_user_max, update_time, is_highest_bider, is_register_watchlist, is_bid}] y total_auction_info_list_num.
- **GET_MYCLUB_AUCTION_WATCH_LIST** (lector 141cddb60): la misma respuesta que la anterior.
- Pantallas ya vistas en vivo:
  - Entrenamiento: los 11 jugadores en Nv. 1/30.
  - Fichar jugador: «Asignar cazatalentos».
  - Casa de subastas y Lista de interés: «No hay datos».
  - El nombre del DT sale vacío (pendiente).
- **GET_MYCLUB_AUCTION_WISH_LIST** (lector 141c22fa0): wish_info_list [{wishlist_id, skill{…}, grade, is_exhibited}] y total_wish_info_list_num. Con la lista vacía, el cliente muestra «La lista está vacía».
- **GET_MYCLUB_AUCTION_PRICE_RANKING** (lector 141c6ccc0; la petición lleva ranking_kind): auction_price_ranking [{product_id, rank, scout_id, grade, skill{…}, price, end_time}] y auction_price_ranking_num.
- **GET_MYCLUB_RENTAL_GAMEPLAYERLIST** (lector 141da24f0): rental_gameplayer_list [{gameplayer{la misma ficha que GAMEPLAYERLIST}, pes_point, open_date, close_date}] y rental_gameplayer_list_num. Sin respuesta da C_NDZS_0112.
- **GET_MATCHREJECTLIST** (lector 141c4f160): reject_list [{pid, pname{…}}] y reject_list_num.
- Pantallas que se resuelven en el propio cliente, sin mensajes: Buscar subasta, Vender cazatalentos, Base de datos, Negociar. Contrato tiene 5 tarjetas: Representantes, Cazatalentos, Directores técnicos, Acuerdo de préstamo y Negociar.
- Herramientas en la PC: x64dbg ya estaba instalado en herramientas\x64dbg\programa\release\x64\ (snapshot 2026.05.27). También está Cheat Engine 7.7.
- **GET_EVENT_CATEGORY_LIST** (lector 141ba19f0): event_category_list [{event_category, open_event_num, not_cleared_event_num, recent_event_start_time, banner_file_info{…}}] y matchday_emblem_info{…}.
- **GET_PESLEAGUE_COMPE_RESULT_LIST** (lector 141c11b00): result_list [{compe_id, compe_name, banner_file_info, start_date, end_date, player_rank, entry_user_num, clan_name, clan_owner_pid, clan_owner_name{pname}, is_participation_get, update_division, before_division, is_confirm, is_get_reward, compe_db_id}] y result_list_num.
- **Regla de la alineación (C, en vivo):**
  - Para jugar hacen falta el XI más el banquillo, 18 como mínimo.
  - El orden de member_list decide la colocación: índices 0..10 = posiciones de la formación, y de 11 en adelante = banquillo.
  - El ejemplo de config trae 23 jugadores del Barcelona en el orden de PlayerAssignment (Messi = 7511 en el hueco 8).
- Nombre del club: va en myclub.club_de_prueba (team_name y team_name_short). Si no, «Nom. del equipo» sale vacío.
- Costo: la suma de gameplayer_cost de los 18 convocados debe ser ≤ management del DT (70). Por defecto, el costo ahora es 3.

## 17. Alineación inscrita = gameplan (C/P, 05-nov)
- En vivo: «No puedes elegir la estrategia ya que tu Plantilla tiene menos de 11 jugadores inscritos», aunque member_list trae 23 jugadores.
  - El «!» rojo = jugador no inscrito.
  - P: la alineación inscrita (XI + banquillo) está en gameplan_data (SQUAD_GAMEPLAN), que hoy va vacío.
- Enum del cliente para validar la plantilla: NOERR, NOT_ENOUGH_PLAYER, INCLUDES_RENTAL, RED_CARD, CONTRACT (fn 14113f940, tabla 1437f2560). P: es el result de SET_MYCLUB_MAIN_SQUAD.
- Implementado:
  - SET_MYCLUB_SQUAD_GAMEPLAN guarda la petición tal cual, por squad_id; los bytes van en base64 dentro del JSON del club.
  - GET_SQUAD_GAMEPLAN y SQUADLIST la devuelven.
  - Toda orden MYCLUB sin manejador se registra completa en el log.
- Capturado en vivo al cambiar un jugador y salir. El cliente manda, en orden:
  1. SET_MYCLUB_SQUAD_GAMEPLAYER: squad_id, member_list (18 elementos bin de {gameplayer_unique_id, gameplayer_id, uniform_number, player_level, parameters, default_rarity}), member_list_num 18, is_over_cost, team_power, team_level «TEAM_POWER_LV4», team_spirit y matchday_squad_rating_info.
  2. SET_GP_ACHIEVEMENT.
  3. SET_MYCLUB_SQUAD_GAMEPLAN: 536 bytes = 3 bloques de táctica de 0xA0 + tabla de huecos 0..17 + FF.
  4. MAINMENU_INFO.
- Implementado:
  - Se guarda el orden de los 18 inscritos.
  - SQUADLIST manda los 18 en ese orden.
  - GAMEPLAN_DEFECTO (el plan capturado) se usa si no hay ninguno guardado.
  - SET_GP_ACHIEVEMENT responde NOERR.

## 18. Enums con prefijo (C, 05-nov), causa probable del «!»
- default_rarity ∈ PLAYER_RARITY_{BLACK, GOLD, SILVER, BRONZE, WHITE} (tabla 142ad2c50, fn 14021d4c0). Si mandamos «WHITE» solo, el cliente lo lee como NotImplement.
  - Ahora se toma de la valoración (P: negro ≥85, oro ≥80, plata ≥75, bronce ≥70).
- Otros enums:
  - team_level: TEAM_POWER_LV1..LV4 o NONE.
  - Nivel de valoración: RATING_LEVEL_NONE/1..4.
  - Valoración de plantilla: SQUAD_RATING_PLAYER_NORMAL, _SLAYER, _FP, _FP_SLAYER, _ENEMY, _FP_ENEMY.
  - Tiempo: WEATHER_TYPE_*. Ganador: WINNER_HOME/AWAY/NONE.
  - Tipos de partido, sin prefijo: PESLEAGUE, MYCLUB_FREE, MULTI, OC_FREE, OC_RANKING, MATCHDAY, MYCLUB_VSCOM_COOP, ONLINE_VSCOM_COOP.
- La comprobación antes del partido la hace el propio cliente: flujos ProcessCheckMatchSquad*.json en dt16_all.cpk, con los resultados NOERR, NOT_ENOUGH_PLAYER, INCLUDES_RENTAL, RED_CARD y CONTRACT.
- parameters y enhanced_parameters: listas libres; vacías también valen.
- El nombre del DT sale de los datos locales por coach_id. Sigue sin resolver.
- **El DT queda resuelto:** coach_id = primera columna de Coach.bin (dt10_x64.cpk → common/etc/pesdb/Coach.bin; 221 registros de 300 B; u32[0] = id, nombre ASCII en +54). Ejemplos: 8 ALEXANDRE GAMA, 24 G. IACHINI, 37 R. BOSCAGLIA, 104 RAFAEL BENÍTEZ, 325 O. SOLSKJÆR, 375 F. INZAGHI.
  - Los ids de MyclubCoach.bin (121xxx) no sirven.
  - La formación sale de los datos del DT (5-2-1-2 para el 8).
  - El gameplan guardado sí se aplica.

## 19. Hacia el partido vs COM (05-nov)
- El «!» rojo = contrato corto. Con contract_term 100 desaparece. Ahora 100 es el valor por defecto.
- Partido igualado avanza y pide GET_PLAYER_PARAMETERLIST y GET_MYCLUB_VSCOM_OPPONENT. Sin respuesta, PES se cierra a propósito (c0000409, FAST_FAIL 5).
- **GET_PLAYER_PARAMETERLIST** (lector 141d8a120):
  - La petición lleva is_equalization y player_list [{gameplayer_unique_id, gameplayer_id}].
  - La respuesta lleva param_playerlist [{gameplayer_id_info{unique_id, id}, parameters [int], is_cost_over}], un elemento por jugador pedido.
- **GET_MYCLUB_VSCOM_OPPONENT** (lector 141da7b80):
  - La petición lleva is_coach_mode e is_team_power_lv.
  - La respuesta lleva side, cpu_level, team_name, team_name_short, base_team_id, team_data{plantilla como SQUADLIST}, pes_pname{…}, pid, coach_adaptability_rate, concept_data{concept{14}} y team_name_commentary_id.
  - Se arma desde config myclub.rival. P: los anidados van como bin.

## 20. Rival COM: tipos exactos (C, 05-nov, crash 20:57)
- Con b75a83e, PES se cerraba unos 4,5 s después de VSCOM_OPPONENT con c0000005.
  - El fallo estaba dentro del decodificador msgpack genérico (1421a5e9a).
  - RSI contenía el texto «ansition», de «transition_counter»: el lector había recibido un tipo distinto del que esperaba.
- Orden y tipos reales al final del lector 141da7b80:
  - pes_pname{name, xuid_one, psn_user_id{account_id, online_id, is_valid_cache}, is_deleted}
  - pid int
  - gameplan_data_size int y gameplan_data, también en el nivel superior, además de dentro de team_data
  - coach_adaptability_rate = **lista de int**: [100]
  - concept_data = **lista**, un bin por jugador del rival: {gameplayer_id int, concept bin{14 claves int}}
  - team_name_commentary_id int
- Enums:
  - cpu_level = MATCH_LEVEL_{BEGINNER, AMATEUR, REGULAR, PROFESSIONAL, TOPPLAYER, SUPERSTAR} (tabla 142645fd0). Por defecto: MATCH_LEVEL_REGULAR.
  - side = HOME o AWAY.
- Corregido en c9d66a0.
- Config de prueba en la PC: myclub.rival = 18 ids del Real Madrid (team 109 de PlayerAssignment, por puesto), coach_id 24 (G. IACHINI).

## 21. Club propio por cuenta y GP (05-nov)
- Si una cuenta no tiene club y config trae myclub.club_de_prueba, el servidor le crea **su propia copia** y la guarda en la tabla myclub. La copia lleva club, jugadores y DT de config, y gp de club_de_prueba (o gp_inicial).
  - Desde ahí, cada cuenta tiene sus propios GP, orden de plantilla, gameplan y partidos.
- CREATE_MYCLUB_TEAM, el flujo real de crear club, también copia el plantel de config. Así el club nuevo no queda con 0 jugadores, que haría salir al cliente.
- SET_MYCLUB_VSCOM_GAMERESULT:
  - Guarda el partido en club.vscom (partidos, racha de victorias e historial de los últimos 50).
  - Suma GP según config myclub.gp_por_partido {WIN, DRAW, LOSE, ""}.
  - MAINMENU_INFO muestra com_match_num y com_countWin_num.
- P: todavía no se sabe en qué campo viene el resultado.
  - _desenlace_vscom acepta un texto WINNER_* con side, o pares de goles (score/opponent_score, home_score/away_score).
  - Hay que confirmarlo con la captura real y ajustarlo.
- _sumar_gp(s, n) suma o resta GP y se niega si el saldo no alcanza. Ya está listo para contratos y cazatalentos cuando se capturen sus comandos.
- **Límite actual:** todo va por steam_hex. Con CPY, todas las copias comparten el mismo SteamID, así que dos personas con CPY verían el mismo club. El club será de verdad de cada persona cuando llegue la identidad propia: el pase web de Link, validado en Supabase.

## Cómo replicarlo desde cero
1. config.json: bloque myclub con jugadores (18 o más ids de PlayerAssignment, contract_term 100), entrenadores [{coach_id de Coach.bin}], club_de_prueba, rival, y opcionalmente gp_por_partido.
2. Con PES cerrado: `python -m phoenix_online.servidor config.json`.
3. Abrir PES (base) → online → myClub.
4. Si PES se cierra: mirar el último comando del log (las peticiones MYCLUB se registran completas) y el dump en CrashDumps. Luego leer su lector en Ghidra.

## 22. Respuesta del rival demasiado grande (C, 05-nov 02:03)
- Con c9d66a0, PES ya no se cerraba: pasaba a la pantalla de carga del partido.
- Pero el servidor lanzaba ErrorTrama: la respuesta medía 1080 B cifrados, más que el máximo de una trama (0x410). El servidor cortaba la conexión y PES mostraba «Conexión al servidor perdida. CODE:VNBP_0002».
- Arreglo 1: concept_data = [] por defecto (vector vacío), y queda en unos 744 B. Para mandar un concepto por jugador: myclub.rival.con_conceptos = true.
- Arreglo 2 (P, sin probar en PES): nclmio.codificar ya no lanza el error. Trocea la respuesta con el mismo formato que el cliente usa hacia nosotros:
  - Primera trama 0x0070: [total][largo][zlib=1][4 bytes 0][trozo].
  - Tramas siguientes 0x0071, y la última 0x0072: [largo][trozo].
  - Todas llevan la misma secuencia.
  - Si un día PES no lo acepta, mirar NclMio::DivBufForEasyEncryptionVer en el exe.

## 23. ¡Partido vs COM jugado de punta a punta! (C, 05-nov, a26e7e0)
- Flujo completo:
  1. myClub → Partido → vs. COM.
  2. MAINTENANCE_INFO → PLAYER_PARAMETERLIST → VSCOM_OPPONENT.
  3. Pantallas de mandos y de alineaciones (Phoenix FC vs Real Madrid), y la carga.
  4. GET_MYCLUB_COMMENTARY_INFO {match_category VSCOM_MATCH, team_spirit, team_power_level} → GET_GP_ACHIEVEMENT → SEND_MATCH_TEAM_INFO {match_team_info}. Hoy los dos sin manejador reciben NOERR a secas, y alcanza.
  5. Partido → SET_MYCLUB_VSCOM_GAMERESULT → «Final del partido 3-0» y el aviso de GP.
- Campos de SET_MYCLUB_VSCOM_GAMERESULT:
  - winner_side ∈ WINNER_HOME / WINNER_AWAY / WINNER_NONE (empate).
  - gameend_reason (NORMAL_END, INJURY…), cpu_level «NORMAL», ex_flag, pk_flag, is_coach_mode, error_code, is_interrupt.
  - pes_srv_rtt_* (nums, mean, variance, min, max), pid y past_game_played_pid.
  - **No trae goles.**
- El servidor suma los GP de myclub.gp_por_partido. El cliente mostró «240» por la victoria, así que el ejemplo usa WIN 240.
- Arreglado después: el rival salía 11 veces «T. Courtois» porque sus gameplayer_unique_id (1..18) chocaban con los del club propio. Ahora el rival usa 900001 en adelante.
- Pendiente: escudo del rival (sale «MD» genérico con base_team_id 109) y respuestas propias para COMMENTARY_INFO y SEND_MATCH_TEAM_INFO.

## 24. Barra de GP, comentarista y alineación (C, 05-nov)
- La barra superior mostraba «G 0» después del partido. Venía de GET_GP_ACHIEVEMENT.all_gp, que estaba fijo en 0 y que el cliente pide antes del partido. Ahora all_gp = saldo del club.
- GET_MYCLUB_COMMENTARY_INFO (lector 141c4ced0) responde:
  - team_spirit_prematch, team_power_level_prematch, total_match_num, cont_win_num, cont_win_max, coach_match_num.
  - is_coach_norma_clear «NO», coach_extended «NO».
  - gameplayer_extended_list [], division_stats [], get_player_result bin{num}.
- SEND_MATCH_TEAM_INFO (lector 141d63cc0) solo mira el result. La alineación jugada (match_team_info) se guarda en la sesión.

## 25. Fichar un DT (C, 05-nov)
- Contrato → Directores técnicos: aparecen los 3 DT de mercado_dt con nombre y formación. La ficha carga y aparece la confirmación «Pagar con GP / Monedas».
- «Pagar con GP» manda CHECK_MYCLUB_STORE (lector 141c46300). Sin respuesta propia, el cliente decía «Esta función no está disponible».
  - Ahora responde is_enable_myclub_coin «NO» e is_enable_subscription «NO».
  - Las Monedas de myClub son dinero real y quedan siempre apagadas.
- mercado_dt acepta ids sueltos o {coach_id, gp, pes_point}.
  - P: el campo del precio en GP todavía no está confirmado; «gp» es de prueba.
- Siguiente: capturar el comando de compra que llega después.
- **Confirmado:** el precio en GP es el campo «gp» (int) de cada DT de MARKET_COACHLIST. Con gp 5000 y pes_point 300, el cliente muestra «Pagar 5,000 GP» y deja deshabilitado el pago con monedas.
- **BUY_MYCLUB_MARKET_COACH** (lector 141d8ba10):
  - Petición: {coach_id, use_pes_point, is_coach_texture_use}.
  - Respuesta: gp (saldo nuevo), pes_point (saldo nuevo), coach (bin con la ficha completa del DT, como en COACHLIST) e is_first.
  - El servidor cobra, agrega el DT al club con un coach_unique_id nuevo y lo quita del mercado de ese club (dt_comprados).
  - Errores: ERR_INSUFFICIENT_GP si no alcanza; ERR_FORBIDDEN si se intenta pagar con monedas; ERR_NOTFOUND si el DT no está en venta.
- Antes de que existiera este manejador, el cliente igual decía «Contrataste…» y abría la renovación con un DT vacío.
- **GET_MYCLUB_COACH_CONTRACT_NORMA** (lector 141c5b360): current_point_* y norma_point_* (division, sim, vscom) e is_contract_end «NO».

## 26. Crash en la carga del partido: COMMENTARY_INFO (C, 05-nov 21:44 local)
- Volcado c0000005 en memmove, llamado desde 141c4e427, dentro del lector de GET_MYCLUB_COMMENTARY_INFO.
- Causa: get_player_result es una **lista** de bin {gameplayer_id, num}, y nosotros mandábamos un solo mapa. Arreglado: [].
- Otros tipos del mismo lector: gameplayer_extended_list = lista de bin {gameplayer_unique_id, gameplayer_id}; division_stats = lista de bin; division_num = lista de int.
- Lección: antes de mandar un objeto suelto, confirmar en el lector si es una lista.
- Diff del gameplan guardado por el cliente contra GAMEPLAN_DEFECTO: son idénticos. El gameplan no explica los «(PT)».
- A/B: con «inscritos» guardado y DT 24, el Barcelona sale con sus posiciones; con club nuevo (sin «inscritos», DT 8), todos salen «(PT)». Se sigue investigando.

## 27. CAUSA de los 11 Courtois, los «(PT)» y el partido que acaba solo (C, 05-nov, x64dbg en vivo, solo lectura)
- El cliente guarda bien los 18 miembros del rival: 18 llamadas a 141da8d25, con unique_id 900001..900018 y gameplayer_id y dorsales distintos.
- **gameplan_data es TEXTO base64.** El lector lo copia como cadena terminada en NUL, y el cliente lo manda así: 716 caracteres, con gameplan_data_size = 536, el tamaño en bytes antes del base64.
- Nosotros devolvíamos bytes crudos que empiezan por 0x00, así que como cadena quedaban vacíos. Sin tabla de huecos, los 11 huecos apuntaban al miembro 0: 11× Courtois (PT)(C), y en un club nuevo todos los propios salían «(PT)».
- gameend_reason INJURY = un equipo con menos de 7 jugadores válidos. Con 11 veces el mismo jugador, el partido acababa solo, con victoria 3-0 para el local.
- Tabla de enum de gameend_reason: 1402190c0 (NORMAL_END, DISCONN, … INJURY, GIVEUP …).
- Arreglo: _plan_texto() devuelve siempre (536, texto base64) en SQUADLIST, GET_MYCLUB_SQUAD_GAMEPLAN y el rival (team_data y nivel superior). Un plan guardado por el cliente se devuelve tal cual.
- La respuesta del rival queda en unos 880 B cifrados: cabe en una sola trama.

## 28. Partido vs COM de verdad + catálogo automático (05-nov)
- **Con 8658db3 el partido se juega de verdad** (lo vieron Frank y la PC):
  - Alineación del Madrid con 11 jugadores distintos (Courtois, Carvajal, Varane, Casemiro, Marcelo, Modrić, Isco, Benzema, Hazard…). El Barcelona sale con sus posiciones.
  - Saque inicial «PHX 0–0 RMA 0:00» y los jugadores corriendo.
- **Catálogo** (tools/catalogo_pesdb.py):
  - Lee PlayerAssignment.bin de TU juego y saca {equipos: {team_id: [jugadores en orden]}, jugadores: {id: {team_id, …extra}}}.
  - Con --extra se agregan media y nación, mientras no sepamos leer Player.bin.
  - El archivo queda solo en la PC: .gitignore incluye catalogo*.json y *.bin.
- Config:
  - myclub.catalogo = ruta del catálogo.
  - Sin myclub.jugadores, el club nuevo toma el equipo club_de_prueba.base_team_id del catálogo.
  - myclub.rival.equipos = [{base_team_id, team_name, team_name_short, coach_id, cpu_level}…]: el rival rota con cada partido vs COM. Sin «jugadores», salen los 18 primeros de ese equipo del catálogo.
- Pendiente: leer Player.bin (media, posición, nación) para la rareza y el precio reales, y usar el catálogo en Cazatalentos y Representantes.
- C: al ENTRAR a myClub, el cliente manda un SET_MYCLUB_VSCOM_GAMERESULT vacío: NORMAL_END, WINNER_NONE, pes_srv_rtt_nums 0, past_game_played_pid 0. Se responde sin guardar nada ni dar GP. Esto explica los «DRAW» raros de las pruebas. Un resultado real trae rtt_nums > 0 y past_game_played_pid = pid.

## 29. Después del partido, catálogo real y tablas del juego (C, 05-nov)
- Partido completo: unos 16 min reales, NORMAL_END, WINNER_HOME, pes_srv_rtt_nums 30. El servidor dio WIN +240 GP y PES terminó sin errores.
- Comandos que llegan después del partido (ya respondidos):
  - **GET_MYCLUB_GAME_BONUS** (lector 141c7a820):
    - Campos: gain_gp, gp (saldo), base_gp, game_res (WIN/DRAW/LOSE), match_num, new_cpu_level/old_cpu_level (MATCH_LEVEL_*), is_first_win.
    - Listas: gp_bonus_list [{bonus_kind, bonus_rate}] + _num, gp_bonus_elected_myplayer_num, special_match_bonus_list [regalo], squad_bonus_item_list [regalo].
    - scout{…} (premio, sin mandar por ahora).
    - El servidor guarda club.vscom.ultimo con el resultado.
  - **GET_MYCLUB_GAMEPLAYER_EXPERIENCE** (candidato 141bf0cf0): gameplayer_list = fichas completas con level_info actualizado.
  - **GET_MYCLUB_MAIN_SQUAD** {target_pid} (lector 141c06500): squad (como SQUADLIST) + gameplayer_list + coach_list + team_name + base_team_id. Hoy solo devuelve la plantilla propia.
- **Experiencia (P):** cada partido suma exp_total a los 18 inscritos según myclub.exp_por_partido (WIN 60, DRAW 40, LOSE 30). Nivel = 1 + exp//100, hasta level_limit (30). También suma games. Hay que confirmar contra la curva real.
- **Catálogo real:** 661 equipos y 17 583 jugadores desde PlayerAssignment (1,66 MB). `--player Player.bin` agrega los nombres.
- Tablas del juego (PES base, dt10_x64.cpk):
  - Player.bin: WESYS+zlib, registros de 312 B desde +8; id u32 +0; nombre japonés +60; apellido +121; nombre largo +182; nombre de camiseta +243 (61 B c/u). Los bytes +4..+59 son campos en bits; la media no está guardada.
  - Team.bin: 662 registros de 1532 B desde +84; nombre en inglés +284. El u32 de +1476 no es un id fiable.
- team_id de clubes grandes: 100 Man United, 101 Arsenal, 102 Chelsea, 103 Liverpool, 108 Barcelona, 109 Real Madrid sin licencia («Madrid Chamartín»; su escudo «MD» es el oficial del juego, no un fallo), 114 PSG, 119 Inter, 120 Juventus, 121 Milan, 127 Bayern, 172 Atlético, 173 Man City, 327 Napoli. Los team_id menores de ~60 son selecciones.
- config.ejemplo: rival.equipos rota entre 7 clubes grandes.

## 30. Administrar GP (05-nov)
- `python -I tools/admin_myclub.py config.json lista`: muestra a las personas con su club, sus GP y sus partidos.
- `dar-gp <nombre|steam_hex> <monto>` suma (o resta, si es negativo). `poner-gp <nombre> <monto>` deja el saldo exacto. Nunca deja el saldo negativo.
- Cada cambio queda anotado en club.admin, con la hora, el saldo de antes y el de después.
- Funciona con el servidor encendido: el cambio se ve la próxima vez que PES pida el club.
- Lo que se gana por partido se ajusta en myclub.gp_por_partido.
- Monedas: hoy apagadas (en Konami eran dinero real). Opción propuesta a Frank: monedas solo de premio y gratis, que nunca se venden.

## 31. Monedas gratis (decisión de Frank, 05-nov)
- Con myclub.monedas_gratis = true:
  - Las Monedas de myClub existen solo como premio: monedas_por_partido o admin_myclub.py dar-monedas/poner-monedas.
  - **Nunca se venden.** La compra de monedas de la tienda no tiene manejador y no se implementará.
- Con monedas activas:
  - CHECK_MYCLUB_STORE responde is_enable_myclub_coin «YES».
  - USER_INFO y GP_ACHIEVEMENT muestran el saldo en pes_point.
  - BUY_MYCLUB_MARKET_COACH con use_pes_point YES cobra el «pes_point» del DT. Si no alcanza: ERR_INSUFFICIENT_PES_POINT.
- Con false (lo predeterminado), todo sigue como antes: monedas en 0 y deshabilitadas.

## 32. Respuestas que no caben en una trama (C, 05-nov 04:00)
- La prueba completa salió bien: club tomado del catálogo, partido 3-2 contra Madrid Chamartín, WIN +240, y la pantalla de recompensas («Primera victoria del día»).
- Pero después del OK, GET_MYCLUB_MAIN_SQUAD (18 fichas + DT, unos 1600 B cifrados) fue la primera respuesta mandada **troceada** (0x0070) a un PES real. El cliente se quedó en blanco unos 60 s y mostró C_WNJJ_0002. **El formato troceado hacia el cliente no es el correcto (o le falta algo).**
- Arreglo provisional: MAIN_SQUAD manda solo la plantilla, sin fichas ni DT (unos 700 B). Para volver a lo completo: myclub.main_squad_completo = true.
- **Riesgo:** GAMEPLAYERLIST con un club grande, de 30 o más jugadores con datos variados, también puede pasar de 0x410. Pendiente de RE: cómo arma el cliente las respuestas troceadas (NclMio::DivBufForEasyEncryptionVer y el lector de tipos 0x0070-72 del lado del cliente).
- **Límite real (C, en vivo, 04:08):** una respuesta de 984 B cifrados (trama de 1008) entra; una de 1000 (trama de 1024 = 0x400) el cliente la rechaza, con C_JZZX_0002 en GAMEPLAYERLIST.
  - Ejemplo: un club del catálogo con 27 jugadores pasó el límite apenas se sumaron exp_total y games después del partido.
  - MAX_CIFRADO_CLIENTE ahora es 0x3D8.
- El envío troceado está en prueba: config «tipos_troceado» (p. ej. ["0x1070","0x1071","0x1072"]) permite probar otros tipos sin tocar el código. **Mientras el troceado no funcione, un club grande no entra.** Es la prioridad número 1.
- **Formato troceado correcto (C, 05-nov, receptor del cliente FUN_141853120 → decodificador FUN_1418566c0):**
  - Tipos 0x10/0x60 = mensaje entero; 0x20-22 y 0x70-72 = troceado (Easy, MD5); 0x1060/0x1070-72 = variante Enhanced (SHA-256, cabecera 0x38).
  - INICIO 0x70: interno = [total u32][largo_trozo u32][**tamaño_original u32**][zlib u8] + trozo.
  - SIGUE 0x71 / ÚLTIMO 0x72: [largo_trozo u32] + trozo. Al llegar el 0x72, el cliente une todo y descomprime.
  - Límite del largo cifrado de cada trama: 0x410.
  - Nuestro error: poníamos [total][largo][zlib][4 ceros]. El cliente leía tamaño_original = 0x01000000 y zlib = 0, y no descomprimía. Corregido, tanto en el envío como en la lectura.
  - Ojo: el rechazo con 1000 B cifrados tal vez no fue por tamaño (el límite en el código es 0x410), sino por el contenido (exp_total/games). Falta separarlo con una prueba.
- **CONFIRMADO en vivo (04:25Z, 67e9e25):** un GAMEPLAYERLIST de 992 B cifrados salió troceado con el formato corregido y PES cargó myClub sin errores (27 jugadores con exp_total). Es el primer troceado servidor→cliente que funciona en un PES real. Ya no hay límite de tamaño.

## 33. Fichar jugadores: Representantes, Cazatalentos y renovar contratos (C, 05-nov)
Lectores encontrados en el juego (RE estático, sin abrir PES). Todo lo de dentro va en bin(msgpack).
- **Representantes (agentes).** Config `myclub.agentes`: `[{agent_id, name, gp, pes_point, get_player_num, equipos | jugadores, media_min, posiciones}]`.
  - `GET_MYCLUB_MARKET_AGENTLIST` → `market_agent_list` (fichas de agente) + `market_subscription`.
  - `GET_MYCLUB_AGENT_BOX_INFO` → `agent_info` (box_size = candidatos que quedan).
  - `GET_MYCLUB_AGENT_DRAW_RATE` → main/sub1/sub2 (tablas vacías por ahora).
  - `USE_MYCLUB_AGENT {agent_id, use_pes_point}` → cobra GP (o monedas gratis), elige al azar de la bolsa y devuelve `gameplayer_list`, `agent_bonus_item_list` [], `gp` y `pes_point` nuevos. Errores: ERR_INSUFFICIENT_GP, ERR_INSUFFICIENT_PES_POINT, ERR_FORBIDDEN (monedas apagadas), ERR_NOTFOUND.
  - La bolsa sale del catálogo y nunca repite a un jugador que el club ya tiene. `posiciones` filtra por la posición registrada (§34), p. ej. ["DC","EI","ED"].
- **Cazatalentos.** Se guardan por club en `club.scouts`; un club nuevo los toma de `myclub.cazatalentos` `[{agent_id, grade, count, sell_gp, equipos | jugadores}]`.
  - `GET_MYCLUB_SCOUTLIST` → `scout_list`.
  - `GET_MYCLUB_PROCURABLE_GAMEPLAYERLIST_SCOUT {scout_list:[bin]}` → hasta 50 candidatos.
  - `USE_MYCLUB_SCOUT {scout_list:[bin]}` → UNA ficha en `gameplayer`. Resta 1 a `count` o quita el cazatalentos.
  - `SELL_MYCLUB_SCOUT` → `gain_gp` y `gp`.
- **Renovar.** `EXTEND_MYCLUB_GAMEPLAYER {gameplayer_unique_id_list, use_pes_point}` suma `renovacion_partidos` (10) por `renovacion_gp` GP (o `renovacion_monedas`) a cada jugador. Devuelve `contract_term_list` (unique_id ENTERO, no texto: con texto PES se congela). `EXTEND_MYCLUB_COACH {coach_unique_id}` hace lo mismo con el DT.
- Un jugador fichado entra con unique_id nuevo y contrato 100.
- **Falta probar en vivo** que PES muestre al jugador nuevo y los saldos. Faltan también subastas (AUCTION_SCOUT*), IS_FIRST_AGENT_MULTIPLE real y las tablas de rareza.

## 34. Player.bin: posición y nación (C, 05-nov)
- Se leen los primeros 64 B de cada registro (312 B desde +8) como un entero little-endian de 512 bits. El id son los bits 0-31.
- **Posición registrada:** bits 370-373. Valores: 0 PT, 1 DFC, 2 LI, 3 LD, 4 MCD, 5 MC, 6 MDI, 7 MDD, 8 MO, 9 EI, 10 ED, 11 SD, 12 DC.
  - Verificado con Isco, Hazard, Messi, Salah, Benzema y Lewandowski.
  - 6, 7 y 11 salen del orden estándar; no se vio un ejemplo.
  - El `puesto` de PlayerAssignment es el hueco en la plantilla, no esta posición.
- **Nación:** bits 169-175. Es el índice de nación del juego, p. ej. ESP 108, BRA 18, FRA 80, ARG 16, POR 100.
  - Coincide con la selección nacional en el 89 % (1173 de 1311). El resto son jugadores con asignación mixta.
- **La media no está guardada**, porque el juego la calcula con las habilidades. Los bytes +4..+59 tienen altura, peso, pie y habilidades (unos 8 bits por atributo), y aún no están separados uno a uno. El cliente pinta la media desde su propia base por gameplayer_id.
- `tools/catalogo_pesdb.py --player Player.bin` ya guarda `posicion`, `posicion_txt` y `nacion` en el catálogo.

## 35. Abandonar el partido vs COM (05-nov)
- C: el flujo del juego trae la salida: `Match/MatchPause.json` (`02_exit`) → `Match/MatchDiscontinue` → `4_toMyClubMain` → `Online/CpuMatch/ProcessMatchEnd`.
- Falta ver en vivo si el icono aparece en la pausa y, si no aparece, qué lo esconde. Puede ser una regla de Konami.
- **Decisión de Frank:** abandonar cuenta como derrota. Si `gameend_reason` está en `myclub.motivos_abandono` (por defecto ["GIVEUP"]), el resultado es LOSE, con los GP de derrota. Se apaga con `myclub.abandono_es_derrota = false`.
- DISCONN no cuenta por defecto, porque puede ser culpa de la red o del servidor.
- **Observado en vivo (05-nov, partido vs. COM, pausa con Start ≡):** opciones Ayuda, Estrategia, Repetición (gris), Config. de cámara, Ajustes de sistema (y una cortada a la izquierda). **No hay Salir ni Abandonar.**
- **Decisión de Frank:** dejarlo así, como el PES 2021 original. No se fuerza la opción de abandono. `abandono_es_derrota` / `motivos_abandono` (GIVEUP) se quedan por si algún día llega ese `gameend_reason`; hoy no se puede probar.

## 36. Logros de GP durante el partido (C, 05-nov)
- En vivo, durante el partido salen avisos como «Arrancaste PES 2021 · 240», «Pared · 600» o «Dribble en una baldosa · 1200». Llegan como `CMD_SET_GP_ACHIEVEMENT`. Antes se contestaba NOERR sin sumar nada.
- Lectores 141bb27a0 y 141bb2e40.
  - Petición: `new_achievement_bit_array`, una lista de palabras de bits de los logros de la sesión. **No trae importes.**
  - Respuesta: `all_gp` (el saldo después de sumar) y `all_achievement_bit_array` (todos los logros ya cobrados, para que el cliente no vuelva a avisar).
- Servidor:
  - Guarda los bits en `club.logros_bits`.
  - Cada bit nuevo, con número = palabra×32 + bit, paga `myclub.gp_logros["<n>"]`, o `myclub.gp_logro_defecto` (300) si no está en la tabla.
  - Nunca paga dos veces el mismo bit.
  - Queda anotado en `club.logros_cobrados`.
  - `GET_GP_ACHIEVEMENT` devuelve también los bits.
- Falta: la tabla real de importes por logro, buscándola en los .cpk. El log guarda las palabras crudas para armarla.

## 37. Representantes: por qué volvía al menú (C, 05-nov)
- «Representantes» → `ProcessPreMarketAgentList` (clase OnlineMyClubAgentListPre, ejecución FUN_1413d8ae0).
  - Muestra 2 anuncios y envía MARKET_AGENTLIST.
  - Si el lector 141c20550 marca la respuesta como error, emite «return» → `ProcessGetMainMenuInfoShort`. Ese es el rebote que se veía: LOGIN_BONUS seguido de MAINMENU_INFO.
- **Forma real de cada elemento de `market_agent_list`:** `bin({agent: bin({agent_id, name, name_jp, type, target_level, token_list:[bin({level,count})], token_list_num, bgm_id, uniform, get_player_num, is_draw_checked, is_buy_gp_pes_point, pes_point, gp, agent_end_time, agent_token_end_time}), banner_file_info: bin({filename, hash_str}), skill: bin({skill_category, skill_id, limit_count, use_count})})`.
  - `banner_file_info` y `skill` son hermanos de `agent`, no van dentro de él.
  - Nosotros mandábamos los campos sueltos. Corregido.
- `AGENT_BOX_INFO` (lector 141c2e880) tiene DOS niveles: `agent_info: bin({agent_info: bin({agent, banner_file_info, skill}), box_size, box_pack_num, reset_remain_count, main/sub1/sub2_player_rarity_info: [bin({rarity:int,count:int})] (LISTA), is_use_reset})`. Con un solo nivel, o con rarity_info como bin suelto, PES se cerraba al entrar al agente (vivo 05:45 y 05:58).
- AGENT_DRAW_RATE (141ca9080): main/sub1/sub2 = bin({rate_table: [int…], rate_player_num}). Tras AGENT_BOX_INFO la pantalla espera: 1_use_pack → USE_AGENT, 2_emission_ratio → DRAW_RATE, 3_reset → RESET_AGENT_BOX.
- Falta: los valores válidos de `type`. El valor por defecto del cliente es «NotImplement», y las ramas 0_box y 1_single sugieren BOX o SINGLE.

## 38. Pruebas en vivo, 05-nov 05:30-05:55
- Vender cazatalentos FUNCIONA en el juego: se marca la casilla con Enter, se confirma con «C», sale «¿Vender por 500 GP?», se elige Sí y aparece «Recibiste 500 GP» (SELL_MYCLUB_SCOUT).
- La barra de GP después de USE_MYCLUB_SCOUT ya muestra el saldo bien (560d088).
- Teclas útiles para pruebas con el MCP:
  - C = acción del contexto: Vender, Clasificar.
  - V = cambiar entre lista y rejilla.
  - Z = desmarcar o atrás.
  - Esc = atrás.
- Ojo: con Esc ×2 desde Miembros se vuelve al menú principal del juego. Teclas de más pueden entrar al vestíbulo online (SEND_IPANDPORT_NTL y GET_LOBBYLIST), que el servidor aún no maneja, y sale «No se ha podido enviar ni recibir datos». No es un fallo del servidor.
- Renovar contrato: falta ver qué acción la abre (flow MenuEstimateExtendList).
- 06:05: con 6089f7f PES ya no se cierra en la caja del agente. Al usarlo sale «No puedes usar este Representante porque ya caducó».
  - No se envía ningún comando: lo decide el cliente con los datos del agente.
  - Sospechas: agent_token_end_time = 0, skill.limit_count = 0, o tiempos en otra unidad.
  - Prueba en curso: tiempos a +10 años y limit_count 99.
- **Causa del «caducó»:** RE del texto: msg 0x22602be en string/all.str. Lo muestra solo el constructor de la caja, FUN_140d46f90.
  - Sale si el vector de pares {rarity, count} está vacío y además un byte (+0x75) vale 0, o si unos contadores (+0x78/+0x7c) valen 0.
  - No tiene que ver con los tiempos: las pruebas con +10 años fallaron.
  - Arreglo: cada `*_player_rarity_info` lleva al menos un elemento `{rarity, count = candidatos}`.
  - Otro aviso del mismo constructor («caja sin fichajes», 0x226028a/b) se evita con `is_use_reset` «NO» y box_size > 0.
- Después del «caducó» (0f320ec), PES se cerraba 0,4 s después de AGENT_BOX_INFO.
  - Causa: FUN_140d491c0 carga el banner del agente desde la URL tipo 17, `MYCLUB_AGENT_BANNER`, y GET_URLLIST no la traía. La URL quedaba NULL y daba el mismo crash que DMM_TOP el 02-nov.
  - Arreglo: se añadió `MYCLUB_AGENT_BANNER` → `/asset/myclub-agent-banner`, que devuelve un 404 limpio.
- **06:25, x64dbg (solo lectura):** el crash era dentro del lector, en `sub1_player_rarity_info` (retorno 141c3055f). Forma real:
  - `main_player_rarity_info` = LISTA de bin({rarity, count}), con al menos un elemento.
  - `sub1_player_rarity_info` y `sub2_player_rarity_info` = UN bin cada una. Se guardan en +0x228/+0x22c y en +0x230/+0x234.
  - Con 3 bins el crash era en main; con 3 listas, en sub1.
  - El banner no era la causa de este crash, pero la URL igual hace falta.
- **06:30, con 9956374:** sin crash, pero vuelve el «caducó». El vector de 8 B en +0x50 no es la rareza: casi seguro es `token_list` ({level, count}), que también usa token_use_dialog (FUN_140d48520).
  - Prueba: `token_list` = [bin({level:1, count:1})], `token_list_num` 1 e `is_draw_checked` «YES». Los dos se pueden cambiar con token_level, token_count e is_draw_checked en myclub.agentes.
- **06:42 Lima 01:42: REPRESENTANTES FUNCIONA DE PUNTA A PUNTA (40f2bb9).**
  - Recorrido: caja «Phoenix Estrellas» (154/154) → usar → animación → USE_MYCLUB_AGENT → «Nuevo fichaje con representante»: L. Goretzka (Bayern, MC 85) → la caja queda en 153/153.
  - GP: −10 000 del agente y +300 de un logro, con la barra correcta. La causa del «caducó» era el token_list vacío.
  - Logro real visto: bit 237 (palabra 7, bit 13), +300 por gp_logro_defecto.
  - Detalles menores por arreglar:
    - Sale «1/3» en la pantalla de fichaje.
    - Sale «Finaliza en 0 min»: la caja usa otro campo de tiempo.
    - Los iconos de la tarjeta muestran 0 GP y 0 monedas.
- **06:50, RE de los detalles (PC):**
  - «1/3» en «Nuevo fichaje con representante» NO es un contador de jugadores: son 3 PÁGINAS (jugadores, objetos extra, bonus), con los botones LT y RT. No es un fallo.
  - «G 0 / ★ 0» en la tarjeta sería el coste por tirada. Con token e is_draw_checked YES sale gratis para el cliente. El servidor es quien cobra los 10 000 GP.
  - «Finaliza en 0 min»: prueba barata con `agent_token_end_time` y `market_subscription.end_time` = ahora + 7 días.
  - Mapa de offsets del objeto resultado de AGENT_BOX_INFO: agent_id +0xb8, target_level +0x100, token_list +0x128, get_player_num +0x144, pes_point +0x188, gp +0x18c, agent_end_time +0x190, agent_token_end_time +0x194, skill_id +0x1f8, limit_count +0x200, use_count +0x204, box_size +0x208, box_pack_num +0x20c, reset_remain_count +0x210, main_rarity +0x218, sub1 +0x228, sub2 +0x230.
  - Renovar contrato: Miembros → evento «01_extend» (1 jugador) o «05_extend_list» (varios) → CMD_EXTEND_MYCLUB_GAMEPLAYER. Sale del menú de acciones del jugador, que se abre con X, y el MCP bloquea esa tecla.
- **07:00, prueba conjunta con 896e60e (depurador, cero excepciones):**
  - «Finaliza en 0 min» sigue igual. No son `agent_end_time` (probado a +7 días y a +10 años), `agent_token_end_time` ni `market_subscription.end_time`. Queda como detalle cosmético sin localizar.
  - La 2.ª caja «Delanteros» (54/54) funciona: el agente 2 cobra 6 000 GP y sale M. Greenwood (Man United). El club queda con 30 jugadores.
  - **Aviso sobre `posiciones`:** el catálogo lo ve como DC (posición registrada en Player.bin), pero el juego lo muestra como EXD. El filtro por posiciones es aproximado y puede sacar un extremo en lugar de un delantero centro. Falta conocer la posición que usa el juego para mostrarla.
  - DRAW_RATE sin probar: se abre con X (Perfil), y el MCP bloquea esa tecla.

## 39. Renovar contratos: tope de duración (05-nov 07:20)
- Frank pulsó X en Miembros y NO salió renovar/extender. Nuestros jugadores tenían `contract_term` 100.
- **Corrección (07:35):** la hipótesis del tope era FALSA para que aparezca la opción. Aparece también con contrato 100; lo que la hizo aparecer fue mandar `contract_gp` y `contract_pes_point` en la ficha (precio 0 antes). El tope sí existe: Messi con 100 pagó 200 GP y siguió en 100.
- Textos del juego: «renovar por 10 partidos más… con GP o Monedas», y «No se pudieron renovar los contratos de todos… algunos ya tienen la MÁXIMA DURACIÓN POSIBLE». Hay un tope, y 100 casi seguro lo alcanza, así que el menú no ofrece nada.
- Prueba: `tools/admin_myclub.py config.json poner-contrato <persona> <partidos> <unique_id…>` baja el contrato de jugadores de prueba (30, 5…) sin reiniciar el servidor.
- El servidor ahora limita la renovación con `myclub.renovacion_maximo` (por defecto 100).
- **07:25, precio de renovar:** no hay comando de precio. «Comprobar estado» (MenuEstimateExtendGameplayer) lee de la FICHA del jugador, que ya cargó GAMEPLAYERLIST.
  - `contract_gp` = precio en GP, `contract_pes_point` = precio en monedas y `sell_gp` = lo que se cobra al vender.
  - Antes iban en 0, y por eso salía «Puntos necesarios 0» con «Pagar con GP» apagado.
  - Ahora salen de `myclub.renovacion_gp`, `myclub.renovacion_monedas` y `myclub.venta_gp`, a menos que el jugador traiga los suyos.
- **07:35, congelado al renovar:** PES se quedó en bucle al 100 % de CPU esperando el fin de EXTEND_MYCLUB_GAMEPLAYER.
  - Causa: mandábamos `gameplayer_unique_id` como TEXTO en `contract_term_list`, y el lector (141bf8200) lo lee como ENTERO de 64 bits. Cualquier otro tipo lo deja en 0, y el cliente no encuentra al jugador renovado.
  - Forma correcta: `{result, contract_term_list: [bin({gameplayer_unique_id: int, gameplayer_id: int, contract_term: int})], gp: int, pes_point: int}`.
  - Corregido. Antes de este arreglo el servidor sí cobró (−200 GP) y sumó +10 al contrato; el cliente no llegó a verlo.
- **Renovar a quien ya está en el tope no cobra** (07:40). Probar el resultado visible con un jugador de contrato 30 (unique_id 1 y 2) y otro de 5 (unique_id 3): deben quedar en 40 y 15.
- **07:35, RENOVAR CONTRATO FUNCIONA en vivo (f4eadf2).** «M. Ter Stegen renovó su contrato.» sin congelarse. Contrato 30 → 40, −200 GP.
  - Además se cobró un logro: bit 225 («renovar un contrato», +300), así que el saldo quedó en 24 100.
  - Falta ver a Lenglet (5 → 15) y el contrato nuevo en la lista de miembros.

### 39.x Hallazgo: contract_term < 100 deja al jugador inelegible para vs. COM (05-nov 09:05)
- Síntoma: vs. COM mostraba «No puedes disputar el partido porque no tienes jugadores suficientes… jugadores inelegibles», aunque había 18 inscritos sin huecos (tabla del gameplan 00..11 completa).
- En Dirección plantilla, Ter Stegen, Lenglet y Piqué (contract_term 40/15/30 en la base, puestos bajos a mano con poner-contrato para probar renovar) mostraban «Contrato restante 0» con «!» rojo. Los jugadores con contract_term 100 (o sin clave: el servidor manda 100 por defecto) muestran «Contrato restante 1».
- Conclusión: la unidad de contract_term NO es «partidos»; PES parece mostrar/validar algo como contract_term // 100 (o trata <100 como 0). Un «!» = inelegible para vs. COM.
- Arreglo (05-nov): contract_term de uid 1, 2, 3 puesto a 100 en la base (copia previa: phoenix_online.antes_contratos100.db), sin reiniciar servidor ni PES.
- Pendiente: confirmar con REA el campo y la escala que lee la pantalla «Contrato restante». No usar valores < 100 en contract_term hasta entonces.

## 40. Menú del jugador: bloquear, liberar y entrenador de experiencia (C, 05-nov 07:50)
Leído en el juego (sin abrir PES). Los ids van como ENTEROS.
- **Bloquear / Desbloquear:** `CMD_SET_MYCLUB_PROTECTED_ENABLE` y `..._DISABLE`.
  - Petición: `gameplayer_unique_id_list` (lista de int). Respuesta: solo NOERR.
  - Se guarda en la ficha como `is_protected` «YES» o «NO».
- **Liberar:** `CMD_SELL_MYCLUB_GAMEPLAYER`.
  - Petición: `gameplayer_unique_id_list` e `is_use_cloud_match`. Respuesta: `gain_gp` y `gp`.
  - El precio sale de `sell_gp` de la ficha, o de `myclub.venta_gp` si el jugador no lo trae.
  - El servidor rechaza con ERR_FORBIDDEN a los bloqueados y a los inscritos en la plantilla.
- **Convertir en Entrenador de experiencia:** `CMD_CREATE_MYCLUB_STAFF`.
  - Petición: `player_info_list` (bins de 12 campos). Respuesta: `staff_info_list` = lista de bin({id, base_player_info, base_exp}).
  - El jugador desaparece de los miembros. La lista se pide con `CMD_GET_MYCLUB_STAFF_LIST`, que devuelve `staff_list_num` y `staff_list`.
- **Pendiente (leído, sin hacer):**
  - `CMD_USE_MYCLUB_STAFF`, para dar experiencia a un jugador.
    - Petición: player_info, staff_info_list y use_item_list.
    - Respuesta: exp_added, exp_info_pre/post, sell_gp, contract_gp, contract_pes_point y traning_result.
  - `CMD_GET_MYCLUB_ITEMLIST`: lista de bin({item_type, item_variation, item_effect_arg, count, pes_point_price}). Sirve para el «Boleto de contrato».

## 41. Subastas de cazatalentos, AUCTION_SCOUT* (C, RE estático 05-nov 08:00)
Un «producto» es un cazatalentos a subasta (`scout_id` = agent_id, `grade` = estrellas 1-5). Todo lo anidado va en bin. Pantallas: MenuScoutMainMenu → «Casa de Subastas», buscar, seguimiento (watchlist), lista de deseos, buscar jugador y ranking de precios.
- **AUCTION_INFO** (elemento de listas): bin({product_id, scout_id, grade, skill: bin({skill_category, skill_id}), price_now, bid_num, time_limit, finalround_end_time, auction_state (texto), is_bid_user_max, update_time, is_highest_bider [sic], is_register_watchlist, is_bid}).
- **DETAIL** (auction_scout_detail): AUCTION_INFO más price_start, auction_start_time, auction_end_time, is_highest_bidder [sic «bidder»], is_reservation, second_price, third_price, my_price y reservation_price.
- **Comandos:**
  - `GET_MYCLUB_REFINE_SEARCH_AUCTION_LIST`: petición con refine_search_auction (filtros), start_index e is_auction_state_changed. Respuesta: auction_info_list y total_auction_info_list_num.
  - `GET_MYCLUB_AUCTION_WATCH_LIST`, y `ADD/DELETE_..._WATCH_LIST` con product_id (borrar también lleva is_all).
  - `GET_MYCLUB_AUCTION_WISH_LIST` (wish_info_list con wishlist_id, skill, grade, is_exhibited) y `ADD/DELETE_..._WISH_LIST`.
  - `GET_MYCLUB_AUCTION_SCOUT_DETAIL {product_id}` devuelve el detalle. `WATCH_..._SCOUT_DETAIL` sondea con update_auction_scout_detail. `LEAVE_...` no devuelve nada.
  - `TENDER_MYCLUB_AUCTION_SCOUT {product_id, tender_gp, is_finalround}` (pujar) devuelve el detalle y gp. Hay variantes `..._FINALROUND_RESERVATION` y `..._CANCEL` (esta también devuelve gain_gp).
  - `GET_MYCLUB_AUCTION_SCOUT_TENDER_USERINFOLIST`, `GET_MYCLUB_AUCTION_REPORT_LIST` («Mis pujas»), `GET_MYCLUB_AUCTION_SUCCESSFUL_BID_INFO`, `GET_MYCLUB_AUCTION_PRICE_RANKING {ranking_kind}` y `REQWATCH_MYCLUB_AUCTION_REFUNDED_GP` (refunded_gp_list y total_gp).
- **Faltan por RE:** los valores de `auction_state` y de `ranking_kind`, y el elemento de la lista de SUCCESSFUL_BID.
- **Diseño del servidor:** una tabla de subastas con product_id, vendedor, scout, precio inicial, fin y pujas. Al perder una puja se devuelve el GP. Con una sola persona hacen falta subastas del sistema para que no esté vacío. El mercado entre personas depende de la identidad propia por cuenta.
- **Aclaraciones de las subastas (RE, 08:10):**
  - `auction_state` parece ser el estado del USUARIO: AUCTION_USER_STATE_{NONE, BID_PARTICIPATION, HIGHEST_BIDER, FINAL_ROUND, NOT_BE_FINAL_ROUND_BID, REJECTED}. Empezar con «AUCTION_USER_STATE_NONE». Cerca hay otros posibles estados del producto: NORMAL, 1ST_30MIN, 2ND_15MIN, 2ND_30MIN, CANCEL, FINISH, FOUND.
  - Errores que el cliente conoce: ERR_INSUFFICIENT_TENDER_GP, ERR_BID_USER_MAX, ERR_TENDER_TIMEOUT, ERR_WATCH_LIST_MAX, ERR_WISH_LIST_MAX, ERR_WISH_LIST_ALREADY_ADD, ERR_PLAYER_PROTECTED, ERR_TEAM_STAFF_FULL, ERR_FORBIDDEN.
  - `ranking_kind` no tiene enum propio: se ignora y se devuelve siempre el mismo ranking.
  - `auction_successful_bid_list` es una lista de ENTEROS (no de bins), más auction_successful_bid_list_num, best_bid_amount_price, best_bid_amount_day y average_bid_price.
- **AGENT_DRAW_RATE:** `main`, `sub1` y `sub2` = bin({rate_table: [int], rate_player_num: [int]}), dos listas paralelas con una fila por rareza. Antes mandábamos `rate_player_num` como un entero suelto, lo que podía romper la pantalla. Ahora lleva una fila: [10000] y [candidatos]. Por probar con X en la caja.

## 42. Prueba en vivo del menú del jugador (05-nov)
- Bloquear/Desbloquear (SET_MYCLUB_PROTECTED_ENABLE/DISABLE): OK. Candado en la lista; el cliente deshabilita por sí solo Bloquear, Convertir y Liberar en un jugador protegido.
- Liberar (SELL_MYCLUB_GAMEPLAYER): OK, +100 GP, 30 -> 29 jugadores.
- Convertir en Entrenador (CREATE_MYCLUB_STAFF): OK, sale de la plantilla; el cliente lo permite también con titulares (Messi).
- Caja de Representante: X abre «Información de Caja» (155/155) sin caída; fichaje de Fabinho Nv.1 sin coste (token).
- Avisos de logro muestran el GP real: bit 231 «Vender jugador» = 800, bit 237 = 240, convertir en entrenador = 600 (bit por identificar). Añadidos 231 y 237 a gp_logros.
- Pendiente: devolver a Messi (admin), logro de convertir (bit), abandono en vs COM.

## 43. Convertir a un titular en entrenador (05-nov 09:20)
- Visto en vivo: el cliente deja convertir a un inscrito (Messi). El servidor lo dejaba en inscritos["1"] y PES guardaba la plantilla con un hueco (tabla de huecos con 0xFF) → «No tienes jugadores suficientes».
- Arreglo (`_sacar_de_plantilla`): al convertir, el jugador sale de jugadores y de cada plantilla inscrita; su mismo puesto lo ocupa el primer jugador libre (así la tabla de huecos del gameplan sigue siendo válida). Si no hay libres, se quita el puesto.
- PES guarda la lista vieja en memoria: tras convertir a un titular hay que salir de myClub y volver a entrar para ver el reemplazo.
- Prueba: test_myclub_entrenador_titular_no_deja_hueco.

## 44. Dificultad de la COM según rachas (05-nov noche)
- Config `myclub.dificultad` = {activa, niveles [fácil→difícil], inicial, victorias_para_subir (def. 3), derrotas_para_bajar (def. 2; 0 = nunca baja)}. Apagada por defecto.
- Se guarda por club en vscom.nivel_idx; tras cada partido `_ajustar_dificultad` sube un nivel cada N victorias seguidas y baja con M derrotas seguidas.
- VSCOM_OPPONENT.cpu_level = nivel del club (salvo rival.cpu_level_fijo = true). GAME_BONUS: old_cpu_level = nivel antes del partido, new_cpu_level = nivel nuevo (el cliente muestra el cambio).
- C (REA 05-nov noche, tabla 0x142645fd0): MATCH_LEVEL_ 0..5 = BEGINNER, AMATEUR, REGULAR, PROFESSIONAL, TOPPLAYER, SUPERSTAR. GAME_BONUS (lector 0x141c7a820): new/old_cpu_level TEXTO (+0x188/+0x1a8), game_res texto, match_num int, is_first_win texto. Activada en config.ejemplo con inicial REGULAR (igual que antes hasta ganar 3 seguidas).
- Logro «Convertir en Entrenador» = bit 244 (600 GP, P por orden de acciones); «Liberar» = 231 (800).
- Prueba: test_myclub_dificultad_sube_y_baja.

## 45. Contrato y cansancio por partido (05-nov noche)
- `myclub.contrato_por_partido` (0 = apagado): cada partido vs COM resta ese valor al contract_term de los 18 inscritos (nunca baja de 0). Ojo con la escala: PES muestra 100 como «1» y <100 como «0» (inelegible, §39.x); no encender hasta confirmar la escala con REA.
- `myclub.cansancio` = {activo, sube, baja, maximo}: los inscritos suben `sube` de tiredness (hasta `maximo`); los que no juegan bajan `baja` (hasta 0). Apagado hasta confirmar el rango real.
- Prueba: test_myclub_contrato_y_cansancio_por_partido.

## 46. Reglas de myClub en caliente (05-nov noche)
- Config raíz `reglas_myclub` (ruta de un JSON) y `reglas_cada_s` (def. 30). El servidor relee el archivo cuando cambia y mezcla su contenido sobre `cfg["myclub"]` (lo nuevo manda), sin reiniciar. JSON roto = aviso en el log y se reintenta.
- Sirve ya para cambiar precios, GP, dificultad, agentes… mientras PES está abierto. Más adelante el espejo de Supabase escribirá este archivo desde el panel web (PLAN_MULTIUSUARIO §3).
- Prueba: test_reglas_myclub_en_caliente.

## 47. Escala real del contrato (REA 05-nov noche) — CORRIGE §39 y §45
- C (0x140d36fe0): PES calcula «Contrato restante» = contract_term / 90 (entero) y solo deja jugar con contract_term >= 90. P: la unidad son minutos (un partido = 90).
- Por eso 100 se veía «1» y 40/30/15 «0» + «!» (inelegibles).
- Servidor: la base guarda contract_term en la unidad del juego; la config habla en PARTIDOS: `contrato_inicial` (10), `renovacion_partidos` (10), `renovacion_maximo` (50), `contrato_por_partido` (1, ya encendido). `_partidos_a_unidad` multiplica x90.
- Migración automática (una vez por club, marca `contrato_x90`): los valores viejos (en «partidos», máx. 100) pasan a min(v, renovacion_maximo) x 90. Jugadores sin contract_term = contrato_inicial.
- `admin_myclub.py poner-contrato <persona> <partidos> <uid…>` ahora recibe partidos.
- Prueba: test_contrato_escala_y_migracion.

## 48. Entrenador de experiencia, Boleto de contrato y recuperar cansancio (05-nov noche)
Formas sacadas con REA (RC, `RE_NOCHE_05NOV.md`). Hecho en el servidor; falta probarlo en vivo.
- **CMD_USE_MYCLUB_STAFF** (petición 0x141ca2480, respuesta 0x141ca40d0):
  - Petición: `player_info` (bin con las 12 claves de la ficha), `staff_info_list` (lista de bin {id, base_player_info, base_exp}), `use_item_list` (lista de bin {item_variation, count}).
  - Respuesta: gameplayer_unique_id, exp_added, `exp_info_pre` y `exp_info_post` (UN bin cada uno: level, level_limit, exp_total, exp_current_level, exp_to_next_level, parameters, enhanced_parameters, add_skill_id), sell_gp, contract_gp, contract_pes_point, traning_result [] y traning_result_num 0.
  - P: cada entrenador da su base_exp (hoy = su media al convertirlo) x `myclub.staff_exp_factor` (def. 1). El entrenador usado se borra de `club["staff"]`. Jugador o entrenador inexistente = ERR_NOTFOUND.
  - Curva de nivel: la provisional (100 exp por nivel, `EXP_POR_NIVEL`) hasta conocer la real.
- **CMD_EXTEND_MYCLUB_ITEM_GAMEPLAYER** (Boleto de contrato):
  - Petición {gameplayer_unique_id}. Respuesta {contract_term: UN bin {gameplayer_unique_id, gameplayer_id, contract_term}, extend_ticket: boletos que quedan}.
  - Los boletos viven en `club["boletos_contrato"]`; se dan con `admin_myclub.py dar-boletos`. Sin boletos = ERR_FORBIDDEN. Suma `renovacion_partidos` hasta `renovacion_maximo` (en partidos x 90).
  - Falta: el texto real de item_type/item_variation con que PES cuenta los boletos en el menú (solo se ve en vivo). Si el menú sigue diciendo «x0», hay que mandar el conteo en la lista de objetos.
- **CMD_USE_MYCLUB_ITEM_RECOVERY** (recuperar cansancio):
  - Petición {gameplayer_unique_id_list, is_all, use_pes_point}. Respuesta {pes_point}.
  - Pone tiredness = 0 a los elegidos (o a todos con is_all = YES). Coste `myclub.recuperar_monedas` monedas gratis por jugador (def. 0 = gratis; nunca dinero real).
- Pruebas: test_myclub_usar_entrenador_de_experiencia, test_myclub_boleto_de_contrato, test_myclub_recuperar_cansancio.

## 49. Subastas de la casa (05-nov noche)
Con una sola persona no hay quien venda, así que la «casa» pone cazatalentos a subasta. Hecho en el servidor; falta probarlo en vivo. Formas: §41 y `RE_NOCHE_05NOV.md` §7.
- **Config `myclub.subastas`:** activa, cantidad (6 a la vez), duracion_min (60), incremento (100 GP), precio_por_estrella (1000; o `precio_inicial` en cada producto), seguimiento_max (20), deseos_max (10), `productos` (lista de cazatalentos; si falta se usan los de `myclub.cazatalentos`), `rival_casa` {activo (false), probabilidad, tope_factor}. Todo se puede cambiar en caliente (§46).
- **Estado** en `club["subastas"]`: productos abiertos, seguimiento, deseos, reembolsos e historial (últimos 100). Cada vez que PES pide algo de subastas, el servidor cierra las vencidas y repone hasta `cantidad`.
- **Pujar** (TENDER_MYCLUB_AUCTION_SCOUT {product_id, tender_gp}): P: tender_gp = puja total. La primera puja puede ser el precio inicial; las siguientes, precio + incremento. El GP queda apartado. Errores: puja baja o ya vas ganando = ERR_INSUFFICIENT_TENDER_GP; sin GP = ERR_INSUFFICIENT_GP; vencida = ERR_TENDER_TIMEOUT. Responde auction_scout_detail y gp.
- **Rival de la casa** (apagado): con esa probabilidad la casa sube tu puja al momento; tu GP vuelve y queda en REQWATCH_MYCLUB_AUCTION_REFUNDED_GP.
- **Al vencer:** si ibas ganando, el cazatalentos pasa a tus Cazatalentos (count 1, mismas estrellas). El GP ya se había pagado al pujar.
- **Comandos hechos:** REFINE_SEARCH_AUCTION_LIST (start_index, de 50 en 50), AUCTION_WATCH_LIST + ADD/DELETE (is_all), AUCTION_WISH_LIST + ADD/DELETE (P: petición sin leer; se aceptan skill y grade, y wishlist_id / wishlist_id_list), AUCTION_SCOUT_DETAIL, WATCH_ (devuelve update_auction_scout_detail) y LEAVE_, TENDER y las dos variantes de ronda final (P: sin ronda final; la reserva cuenta como puja normal, cancelar devuelve gain_gp 0), AUCTION_REPORT_LIST («Mis pujas»), SUCCESSFUL_BID_INFO (precios finales del mismo cazatalentos), PRICE_RANKING (historial ordenado por precio; ranking_kind se ignora) y REQWATCH_REFUNDED_GP.
- **Supuestos por confirmar en vivo (P):** time_limit / auction_end_time / finalround_end_time = segundos Unix absolutos; is_bid y demás = YES/NO; refunded_gp_list = lista de montos. Si la Casa de subastas falla, apagar con `subastas.activa = false` en las reglas y avisar.
- Falta: TENDER_USERINFOLIST (forma sin leer) y la ronda final real; el mercado entre personas necesita la identidad propia (PLAN_MULTIUSUARIO).
- Pruebas: test_myclub_subastas_de_la_casa, test_myclub_subasta_rival_de_la_casa_devuelve_gp.

## 50. Prueba en vivo de la noche (06-nov, sin Frank) y lista de objetos
- **Visto en vivo (RC con capturas, servidor 5bec999):**
  - Contratos: un jugador sin clave muestra «Contrato restante 10» y uno migrado (4500) muestra «50». Ningún «!» ni 0.
  - Casa de subastas: «Subastas 1/6» con 6 cazatalentos a 3 000 GP y 1 h. El detalle abre; el juego propone 4 000 como mínimo (suma su propio incremento) y la puja llega como TENDER_MYCLUB_AUCTION_SCOUT sin error. Pantalla: «Oferta más alta», Ofertas 1, cuenta atrás. GP 21 540 → 17 840 (−4 000 y +300 de un logro).
  - Menú del jugador (Dirección plantilla, botón X): «Usar objeto» muestra «Recuperar resistencia (uno) ×0» y «Boleto de contrato ×0».
- **Causa del ×0:** GET_MYCLUB_ITEMLIST respondía la lista vacía. Ahora manda un bin por objeto de `myclub.objetos` = [{clave, item_type, item_variation, item_effect_arg, pes_point_price, mostrar_vacio}], con count = club[clave]. P: valores por defecto CONTRACT / RECOVERY (tabla de categorías 0x142ad6780); si el juego sigue con ×0, probar otros textos en caliente (§46) sin reiniciar.
- Recuperar cansancio gasta primero objetos (`club["objetos_recuperar"]`, uno por jugador) y si no hay, `recuperar_monedas`. Admin: `dar-recuperar`.
- **Teclado de Frank (settings.dat):** en los menús de myClub, LB = tecla A; RT = W; botón X del pad = tecla C (abre el Menú del jugador en Dirección plantilla). La captura del MCP sale cortada por el DPI: capturar con SetProcessDPIAware + CopyFromScreen 1920x1080.
- Comandos sin manejador vistos: CMD_GET_MYCLUB_CLOUD_SQUADID {pid} y CMD_GET_ONLINE_AD_FILE (los dos reciben NOERR vacío; sin efecto visible).
- Prueba: test_myclub_itemlist_cuenta_boletos_y_objetos.
