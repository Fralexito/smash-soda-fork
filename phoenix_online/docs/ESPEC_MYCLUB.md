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
