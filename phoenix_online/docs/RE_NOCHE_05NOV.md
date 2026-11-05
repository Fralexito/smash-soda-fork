# RE de la noche, 05-nov (lectura estática con REA/Ghidra, PES cerrado)

Binario: `PES2021_recortado.exe` (SHA-256 ebd6a786…9284), base 0x140000000. Solo lectura. Marcas: C = visto en el código, P = deducción. Sin datos en bruto de Konami: solo nombres de campos, tipos, direcciones y enumerados.

Nota de método: REA abre el exe con Ghidra sin problemas con `ghidra.startup_timeout_ms` = 3 600 000 (valor de arranque). Las cadenas de CAMPOS tienen xrefs de código; las de nombres `CMD_…` casi no (solo apuntan a la tabla de nombres 0x1428a5640). Las pseudo-descompilaciones pasan de 90 000 caracteres: se guardan en un archivo y se filtran con Python.

## 1. Nivel de la COM (C)
- Enumerado `MATCH_LEVEL_*`, tabla de cadenas desde 0x142645fd0, en este orden (índice 0 a 5):
  0 `MATCH_LEVEL_BEGINNER` (0x142645fd0) · 1 `MATCH_LEVEL_AMATEUR` (0x142645fe8) · 2 `MATCH_LEVEL_REGULAR` (0x142646000) · 3 `MATCH_LEVEL_PROFESSIONAL` (0x142646018) · 4 `MATCH_LEVEL_TOPPLAYER` (0x142646038) · 5 `MATCH_LEVEL_SUPERSTAR` (0x142646050).
- `GET_MYCLUB_GAME_BONUS`, lector **0x141c7a820**. Campos de la respuesta que lee tras `gain_gp`/`gp`:
  - `game_res`: TEXTO (copia con FUN_14022e4c0 al objeto +0x160).
  - `match_num`: ENTERO (+0x180).
  - `new_cpu_level`: TEXTO (+0x188). Debe ser uno de los `MATCH_LEVEL_*`.
  - `old_cpu_level`: TEXTO (+0x1a8). Igual.
  - `is_first_win`: TEXTO (+0x1c8), «YES»/«NO».
- Para el servidor: mandar `new_cpu_level` y `old_cpu_level` como cadenas con el prefijo completo. Para subir de nivel basta con que `new` sea el siguiente índice de la lista que `old`. Qué hace el cliente con la diferencia (animación de «subiste de nivel») es P y sin verificar.

## 6. Bit del logro de «Convertir en Entrenador» (C + P)
Registros del servidor de hoy (hora de la PC), `SET_GP_ACHIEVEMENT`, palabras crudas (`new_achievement_bit_array`, 16 palabras de 32 bits; n = palabra×32 + bit):
- 03:01:53 → palabra 7 = 128 = 2^7 → **bit 231** (+300 por defecto; el cliente mostró 800: «Vender / Liberar jugador»).
- 03:02:44 → palabra 7 = 1 048 576 = 2^20 → **bit 244**: es el siguiente logro nuevo, justo tras «Convertir en Entrenador» (el cliente mostró 600 GP). **Bit 244 = convertir** (P: se deduce del orden de las acciones en la prueba; la acción de liberar fue la anterior).
- 03:03:53 y siguientes: palabra 7 = 8192 = 2^13 → bit 237 (ya cobrado, no es nuevo); 03:05:57 y 03:06:02: palabra 8 = 1024 = 2^10 → bit 266 (sin identificar, no es nuevo para el cobro).
- Para `gp_logros`: `"244": 600` (y confirmar 231 = 800, 237 = 240 como ya anotó §42).

## Hallazgo clave (punto 5): contract_term se divide entre 90 (C)
En 0x140d36fe0, `contractPeriod_value = contract_term / 0x5A` (÷90 entero). Por eso contract_term 100 se ve «Contrato restante 1» y 40/30/15 se ven «0» con «!» (inelegible). La elegibilidad usa el mismo campo con umbral ≥ 90. P: la unidad es el minuto y un partido son 90; el servidor debería mandar múltiplos de 90 (y «renovar 10 partidos» quizá deba sumar 900; sin probar).

## Punto 8: habilidades de Player.bin (EN CURSO, sin tabla todavía)
- Datos: Player.bin de la base de Phoenix (copia de olmosjr23, 29 996 registros, 312 B desde +8). Nota: no es el Player.bin del PES base; los valores de referencia de Frank son del base, así que pueden diferir.
- Medido (P): en los bytes +4..+59 de cada registro, con ventanas de 7 bits, los candidatos con aspecto de atributo (valores 25-99, desviación 15-20 sobre todos los jugadores) empiezan en los bits (contando desde el byte +4) 166, 184, 191, 197, 209, 241, 247, 255, 261, 279, 293, 305, 319, 338, 361. Los espacios no son regulares, así que los campos no son 7 bits seguidos: puede haber campos de otros anchos entre ellos.
- Los campos de los bits 0-19 (valores casi constantes) son cabecera/versión, no atributos.
- Pendiente: tabla de bits por atributo y fórmula de media. Hace falta una de dos: (a) los atributos individuales de 2-3 jugadores del PES base en pantalla (modo edición), o (b) localizar en REA la función que calcula la media (buscar usos de player parameters / «overall» en el lector de la ficha) y leer la fórmula.

# Parte del agente (puntos 2, 3, 4, 5 y 7)


Lectura estática con REA/Ghidra sobre PES2021_recortado.exe (base 0x140000000). Solo lectura. Marcas: C = visto en el código; P = deducción.
Método de tipos: copia a cadena (14022e4c0) = texto; cvttsd2si = entero; 14023d170 = vector; 141c190a0 = anidado (bin). En serializadores: 141151670 = entero u32, 1411b0c40 = entero u64, 141156b20 = cabecera de lista, 141156c10 = cabecera de mapa, 141151580 = cabecera bin.

## 2) CMD_USE_MYCLUB_STAFF (entrenador de experiencia)

### Petición (serializador 0x141ca2480, C)
Campos de nivel superior: msgid, svrtype, client, player_info, staff_info_list, use_item_list.
- **player_info** = bin(mapa de 12 claves):
  - gameplayer_unique_id: entero de 64 bits.
  - gameplayer_base_id, gameplayer_id, gameplayer_level: enteros.
  - registared_position: texto.
  - position: LISTA de textos (elementos de 0x20 bytes, P que cada uno es texto).
  - stronger_foot: texto.
  - overall: entero.
  - ability: LISTA de enteros.
  - play_style: texto.
  - nation_id, club_team_id: enteros.
- **staff_info_list** = lista; cada elemento es bin(mapa de 3 claves): id (entero), base_player_info (bin con las MISMAS 12 claves de arriba) y base_exp (entero). Coincide con la respuesta de CREATE_MYCLUB_STAFF.
- **use_item_list** = lista; cada elemento es bin(mapa de 2 claves): item_variation (TEXTO, no entero) y count (entero). No lleva item_type.

### Respuesta (lector 0x141ca40d0, C)
- msgid (texto), result (texto).
- gameplayer_unique_id: entero de 64 bits (+0xb8).
- exp_added: entero (+0xc0).
- **exp_info_pre** y **exp_info_post**: UN solo bin cada uno (no lista), con 8 claves:
  - level, level_limit, exp_total, exp_current_level, exp_to_next_level: enteros.
  - parameters: lista de enteros.
  - enhanced_parameters: lista de enteros.
  - add_skill_id: entero.
  - Offsets en el objeto: pre +0xc8..+0xd8 (enteros), parameters +0xe0, enhanced +0xf0, add_skill_id +0x100; post +0x108..+0x118, +0x120, +0x130, +0x140.
- sell_gp (+0x148), contract_gp (+0x14c), contract_pes_point (+0x150): enteros.
- **traning_result** (así, con el error de ortografía): LISTA de bin; cada bin es un mapa de claves enteras con prefijo m_: m_offense_decision, m_trap, m_dribble, m_ball_touch, m_short_pass, m_long_pass, m_shot, m_place_kicking, m_ball_spin_control, m_heading, m_defense_decision, m_intercept, m_aggressiveness, m_kick_power, m_speed, m_agility, m_body_control, m_body_balance, m_jump, m_gk_decision, m_catching, m_clearances, m_collapsing, m_deflecting, m_stamina (25 nombres, en este orden de aparición). P: cada elemento es el aumento por atributo.
- **traning_result_num**: entero (+0x168).
- Orden de campos en el lector: msgid, result, gameplayer_unique_id, exp_added, exp_info_pre, exp_info_post, sell_gp, contract_gp, contract_pes_point, traning_result, traning_result_num.

## 3) Objetos, "Usar objeto" y boleto de contrato

### GET_MYCLUB_ITEMLIST (lector 0x141bed520, C)
- item_list: lista de bin; cada uno con item_type (TEXTO), item_variation (TEXTO), item_effect_arg (entero), count (entero), pes_point_price (entero). Valor por defecto de los dos textos: "NotImplement". item_list_num: entero.
- **No existe en el exe ninguna cadena ITEM_TYPE_* ni lista de enumerados de item_type/item_variation** (búsqueda literal y por regex). El lector copia los dos textos tal cual, sin pasarlos por una tabla de enumerados. Los valores válidos viven en datos del juego (menús/flujos de los .cpk) o solo se pueden capturar en vivo. Pendiente.
- P: hay un enumerado de CATEGORÍAS de interfaz (tabla en 0x142ad6780): FREE, RANKMATCH, QUICK, SPECIAL, SCOUT, DRAW_CARD_CATEGORY_SUB2/SUB1/MAIN, APTITUDE_HIGH, APTITUDE_LOW, NOT_APTITUDE, SUBSCRIPTION, STORE, RECOVERY, CONTRACT, USE_SPECIAL_AGENT, USE_TOP_AGENT, USE_SCOUT, PRE_MAIN, NONE. Parece la clasificación de pestañas de objetos (RECOVERY y CONTRACT serían los de recuperación y contrato), pero NO está probado que sean los valores de item_type.

### Comandos de "usar objeto" que existen (C, cadenas CMD_*)
CMD_USE_MYCLUB_ADAPTABILITY_RATE_ITEM, CMD_USE_MYCLUB_ADD_POSITION_APTITUDE_ITEM, CMD_USE_MYCLUB_ADD_SKILL_ITEM, CMD_USE_MYCLUB_ITEM_RECOVERY, CMD_USE_MYCLUB_MANAGEMENT_ITEM, CMD_USE_MYCLUB_TACTICAL_TRAINING, CMD_USE_MYCLUB_STAFF, CMD_USE_MYCLUB_AGENT, CMD_USE_MYCLUB_SCOUT, CMD_EXTEND_MYCLUB_ITEM_GAMEPLAYER, CMD_EXTEND_MYCLUB_GAMEPLAYER, CMD_EXTEND_MYCLUB_COACH, CMD_UPDATE_MYCLUB_COACH_CONTRACT. **No existe CMD_USE_MYCLUB_ITEM** genérico.
Errores del cliente relacionados: ERR_INSUFFICIENT_ITEM, ERR_INVALID_TICKET, ERR_UNUSABLE_ITEM_RECOVERY.

### Boleto de contrato = CMD_EXTEND_MYCLUB_ITEM_GAMEPLAYER (C/P)
- Petición (serializador 0x141c00870): msgid, svrtype, client y **gameplayer_unique_id** (entero de 64 bits). No manda ni item_type ni cantidad (C).
- Respuesta (lector 0x141c00ed0, C): result (texto), **contract_term** = bin único {gameplayer_unique_id (u64), gameplayer_id (entero), contract_term (entero)} y **extend_ticket** (entero, +0xc8; P: boletos que quedan).
- Es distinto de CMD_EXTEND_MYCLUB_GAMEPLAYER, que usa contract_term_list (lista). Aquí es un solo bin (P: el lector parsea una vez, sin bucle).
- La clase interna se llama CmdExtendMyclubItemGameplayerOption (ctor 0x141c004e0).

### CMD_USE_MYCLUB_ITEM_RECOVERY (recuperar cansancio; C)
- Petición (serializador 0x141c81660): gameplayer_unique_id_list (lista de u64), **is_all** (texto, P: YES/NO), **use_pes_point** (texto, P: YES/NO).
- Respuesta (lector 0x141c81e80): solo result y **pes_point** (entero, +0xb8). No devuelve la ficha: P que el cliente vuelve a pedir GET_MYCLUB_GAMEPLAYERLIST.
- Enumerados de interfaz cercanos: RECOVERY_ALL, RECOVERY_ONE, RECOVERY_ITEM_USE (0x142ad5148..58f8).

## 4) tiredness (cansancio)
- Lector GET_MYCLUB_GAMEPLAYERLIST 0x141c91af0: tiredness es entero, en el objeto temporal del jugador (0x3b0 bytes) en +0x388; contract_term en +0x38c, gameplayer_cost +0x390, rating +0x394, sell_gp +0x410, contract_gp +0x414 (C). Se lee igual en gameplayer_list y en main_edition_gameplayer_list.
- Rango: **P: 0 a 4** (cinco niveles). Evidencia: el exe tiene un enumerado CONDITION_{WORST, BAD, NORMAL, GOOD, BEST} (tabla construida en 0x1402198a0; texto→entero 0..4 con BEST=0, GOOD=1, NORMAL=2, BAD=3, WORST=4 si el orden del arreglo es ascendente en memoria, P) y los iconos de la interfaz condition_0..condition_4 (cmnIconCondition, 0x1427bb9b0..). También hay condition_5..9 en los recursos, pero solo 5 niveles en el enumerado. No encontré dónde compara el cliente el valor entero de tiredness (C: no visto); queda como P.
- Comando de recuperación: **CMD_USE_MYCLUB_ITEM_RECOVERY** (ver §3). No hay otro comando de descanso/recuperación (búsqueda de RECOVER/TIRED/REST/CONDITION).

## 5) Escala de contract_term ("Contrato restante")
- **C:** en la función 0x140d36fe0 (diálogo con playerName_value, gp_value, pesPoint_value, playerLv_value), el marcador de interfaz `contractPeriod_value` se rellena con `*(uint*)(jugador + 0x1c) / 0x5a`. 0x5a = **90**, división entera sin signo (llamada a 140ea39f0 en 0x140d373ed).
- Resultado: 100/90 = 1; 40, 30 y 15 → 0. Es exactamente el síntoma observado.
- **C (ESPEC §14):** el chequeo antes de jugar compara el mismo +0x1c con ≥ 90 (vector de 944 bytes por jugador). Por tanto +0x1c del jugador es contract_term y **la unidad mínima válida es 90**.
- **P:** la unidad es el minuto: un partido = 90 minutos. Con 100 se muestra "1 partido" y alcanza el umbral. Un valor sano sería múltiplos de 90 (90, 180, 270…). Esto cambia la renovación: "renovar 10 partidos" equivaldría a sumar 900 (P, sin confirmar en el comando). Pendiente comprobar si el cliente suma por su cuenta o solo muestra lo que devuelve EXTEND_MYCLUB_GAMEPLAYER.
- Consejo para el servidor (P): mandar contract_term como múltiplos de 90; para "N partidos restantes" mandar N*90 (o N*90+algo ≤ 89 de resto). Los valores < 90 dejan al jugador inelegible (el "!" rojo).
- No encontré la función que arma el resultado CONTRACT (ProcessCheckMatchSquad vive en flujos .json); 14113f940 solo traduce el texto del flujo a enumerado (0..4) y lo guarda en [obj+0x130]+0x504.
- Enumerado CONTRACT: aparece en tablas de resultados (0x142ad3c90 y 0x142ad6868).

## 7) Subastas (AUCTION_SCOUT*)
Lectores y tipos (C, cada uno con result y msgid como texto):

### Enumerado auction_state (texto; el cliente lo convierte con la tabla de 0x1402173c0)
Valores: AUCTION_USER_STATE_NONE (0), _FINAL_ROUND (1), _BID_PARTICIPATION (2), _HIGHEST_BIDER (3), _NOT_BE_FINAL_ROUND_BID (4), _REJECTED (5). El orden numérico asume tabla ascendente en memoria (P). Para la petición y respuesta es TEXTO (C).

### AUCTION_INFO (elemento de listas, lector de REFINE_SEARCH 0x141cd53f0)
Elemento = bin. Tipos (C):
- Enteros: product_id, scout_id, grade, price_now, bid_num, time_limit, finalround_end_time, update_time.
- skill = bin anidado: skill_category (TEXTO), skill_id (entero).
- Textos: auction_state, is_bid_user_max, is_highest_bider (sic), is_register_watchlist, is_bid (P: YES/NO).
- Respuesta: auction_info_list (lista de bin) y total_auction_info_list_num (entero, +0xc8).

### GET_MYCLUB_AUCTION_SCOUT_DETAIL (lector 0x141c49960)
result + auction_scout_detail = un solo bin con: product_id, scout_id, grade, skill{skill_category texto, skill_id}, price_now, price_start, bid_num (enteros), auction_state (texto), auction_start_time, auction_end_time, finalround_end_time (enteros), is_bid, is_highest_bidder (aquí escrito "bidder"), is_bid_user_max, is_reservation (textos), second_price, third_price, my_price, reservation_price (enteros). No lleva time_limit ni update_time.

### Lector 0x141c53010 (C)
auction_scout_detail (mismas claves que arriba, sin los campos de producto repetidos en los dos nombres de ranking) más **gain_gp** (entero, +0x1c4). P: es TENDER_..._CANCEL (devuelve detalle y gain_gp). El de TENDER normal devuelve el detalle y gp (ESPEC §41, no re-leído).

### GET_MYCLUB_AUCTION_REPORT_LIST (lector 0x141c31b00, C)
auction_report_list = lista de bin con EXACTAMENTE los campos de AUCTION_INFO (mismos tipos) y total_auction_report_list_num (entero, +0xc8).

### GET_MYCLUB_AUCTION_SUCCESSFUL_BID_INFO (lector 0x141c62750, C)
- auction_successful_bid_list: lista de ENTEROS en línea (no hay bins). auction_successful_bid_list_num, best_bid_amount_price, best_bid_amount_day, average_bid_price: enteros (+0xc8, +0xcc, +0xd0, +0xd4).

### GET_MYCLUB_AUCTION_PRICE_RANKING
- Petición (serializador 0x141c6c600): msgid, svrtype, client y **ranking_kind = TEXTO** (C). No encontré ningún enumerado AUCTION_*RANKING* en el exe: las cadenas de ranking existentes (MYCLUB_RANKING, MYCLUB_WEEKLY_RANKING, CLAN_RANKING, *_BYID, RANK_MATCH…) son de otros rankings. El valor que manda el cliente sale de datos/flujos. Pendiente capturarlo en vivo (el servidor puede ignorarlo, como ya hace).
- Respuesta (lector 0x141c6ccc0): auction_price_ranking = lista de bin {product_id, scout_id, grade (enteros), skill{skill_category texto, skill_id entero}, price (entero), end_time (entero)} y auction_price_ranking_num (entero, +0xc8). Nota: **no hay campo rank** dentro del elemento en este lector (ESPEC §16 listaba "rank"; no apareció como cadena de este lector; P: el orden de la lista es el ranking).

### Confirmaciones de peticiones
- start_index, refine_search_auction (filtros), tender_gp, is_finalround, is_all, wishlist_id, is_exhibited existen como cadenas de campos (C). Sus tipos exactos de petición para ADD/DELETE WATCH/WISH y TENDER no se re-leyeron esta noche (ver Pendiente).

## Pendiente
- Valores válidos de item_type / item_variation de GET_MYCLUB_ITEMLIST (no hay cadenas en el exe; capturar en vivo o buscar en los .cpk de menús, p. ej. itemUseDialog.bin).
- Dónde compara el cliente el entero de tiredness y su rango real (solo P 0..4).
- Comprobar si el cliente multiplica por 90 al renovar o si el servidor debe sumar 900 por 10 partidos; función que calcula CONTRACT (inelegible).
- Valores de ranking_kind de la petición de PRICE_RANKING.
- Tipos de petición de ADD/DELETE_..._WATCH_LIST y _WISH_LIST, TENDER (lector sin ubicar) y variantes FINALROUND_RESERVATION y CANCEL; GET_MYCLUB_AUCTION_SCOUT_TENDER_USERINFOLIST.
- Orden numérico exacto de los enumerados CONDITION_* y AUCTION_USER_STATE_* (solo importa si el servidor mandara enteros; hoy es texto).
- Confirmar que play_info.position en la petición USE_STAFF es lista de textos y no de enteros.
