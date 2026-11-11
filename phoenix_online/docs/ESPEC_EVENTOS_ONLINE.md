# ESPEC · Eventos de competición online (COMPEV) — parte C

Estudio en solo lectura del cliente (Ghidra `-readOnly`, mismas direcciones que `PES2021.exe`). Marcas: **L** = leído del código del cliente; **P** = probable, deducido; **N** = no se pudo comprobar. Ninguno de estos comandos se ha visto aún en una captura real.

## C.1 Resumen
- Hay 11 comandos de la familia: `CMD_GET_COMPEV_MAINMENU_INFO`, `_LIST`, `_INFO`, `_MYENTRY_COMPEID`, `CMD_JOIN_COMPEV`, `CMD_LEAVE_COMPEV`, `CMD_JOIN_COMPEV_MODE`, `CMD_LEAVE_COMPEV_MODE`, `CMD_REENTRY_COMPEV`, `CMD_SYNC_COMPEV_PROGRESS` y `CMD_GET_COMPLG_MYENTRY_COMPEID` (existe). **L**
- Todos los campos de modo, estado, dificultad y resultado viajan como **texto** (el nombre del token), nunca como número. **L** (los lectores del cliente guardan esos campos con la rutina de asignar cadena).
- Cada comando lleva además `msgid`, `rqid` y, cuando se ve, `pid`/`svrtype`/`client` en el pedido; y `msgid`, `rqid`, `result` en la respuesta (`result:'NOERR'` si todo va bien). Los que dicen «sin campos propios» responden solo eso.

## C.2 Pedido y respuesta de cada comando (L salvo lo marcado)

| Comando | Pedido (campos propios) | Respuesta (campos propios) |
|---|---|---|
| `GET_COMPEV_MAINMENU_INFO` | `compe_mode` (texto), `compe_id` (entero), `target_pid` (entero) | `status` (texto), `compe_result` (texto), `win_point` (int), `progress_game_count` (int), `vscom_team_id` (int), `cpu_level` (texto), `clear_count` (int) |
| `GET_COMPEV_LIST` | `compe_list_kind` (texto), `compe_mode` (texto) | `compe_list_kind` (texto, eco), `compe_num` (int), `compe_list` (**bin**), `banner_file_info` (bin) |
| `GET_COMPEV_INFO` | `compe_mode` (texto), `compe_id` (int) | `compe_data` (**bin**), `compe_info` (**bin**), `banner_file_info` (bin), `event_compe_reward_list` (bin), `compe_reward` (bin), `squad_bonus` (bin), `reward` (bin), más los escalares de C.3 |
| `GET_COMPEV_MYENTRY_COMPEID` | `compe_mode` (texto) | `compe_mode` (texto), `compe_id` (int), `compe_cpu_level` (texto), `progress_game_count` (int) |
| `JOIN_COMPEV` | `compe_mode` (texto), `compe_id` (int), `compe_cpu_level` (texto) | sin campos propios |
| `LEAVE_COMPEV` | `compe_mode` (texto) | sin campos propios |
| `JOIN_COMPEV_MODE` | `compe_mode` (texto) | sin campos propios |
| `LEAVE_COMPEV_MODE` | sin campos propios | sin campos propios |
| `REENTRY_COMPEV` | `compe_id` (int) | sin campos propios |
| `SYNC_COMPEV_PROGRESS` | `target_pid` (int), `room_id` (int) | sin campos propios |
| `GET_COMPLG_MYENTRY_COMPEID` | sin campos propios | sin campos propios que el lector reconozca (**N**: la respuesta parece vacía; devolver `result` y, por si acaso, `compe_id`) |

Notas: `compe_id` y `target_pid` son enteros; `compe_mode` en `MYENTRY` vuelve en la respuesta, así que el servidor puede devolver el mismo texto que recibió. **L**

## C.3 Escalares de la respuesta de `GET_COMPEV_INFO` y de cada elemento de `compe_list`
Cada elemento de `compe_list` (y el bloque `compe_info`) es un submapa con (tipo): `compe_id` (int), `compe_name` (texto), `compe_kind` (texto), `compe_start_time` / `compe_end_time` (int, unix), `compe_cpu_level` (texto), `difficulty` (texto), `is_cleared`, `is_locked`, `is_coop_match`, `is_squad_bonus` (**texto** `"true"/"false"` o equivalente; **P**: el lector los guarda como cadena), `add_limit_vscom_win_rate`, `add_limit_rating_min`, `add_limit_rating_max`, `add_limit_match_num_min`, `add_limit_match_num_max` (int), `filename` y `hash_str` (texto, del banner), `banner_file_info` (bin, con `filename`+`hash_str`), `list_banner_file_info` (bin).
Solo en `GET_COMPEV_INFO` (dentro de `compe_data`/`compe_info`): `entry_type` (texto), `entry_national_id`, `entry_team_id`, `entry_player_num`, `entry_teamspirit_min/max`, `entry_teamstrength_min/max`, `entry_coach_id`, `player_num`, `clear_match_num`, `clear_win_point`, `cleared_count`, `precondition_compe_id` (int), `precondition_compe_name` (texto), `ad_file_name`, `logo_file_name` (texto), `is_compe_reward_exist`, `is_same_team_power_level`, `is_limited_matching_target`, `is_exist_unlocked_compe_by_cleared`, `is_pesleague_stadium`, `is_efootball_pro_stadium` (texto booleano), y `squad_bonus` (bin).
Los premios (`event_compe_reward_list`, `compe_reward`, `reward`) usan el objeto «regalo» de B.11 (`present_id, name, name_jp, title, body, entity_id, grade, skill, type, player_level, contract_term, team_id, present_type, item…`) **L**. `present_type` es texto (`NORMAL`, `GM`, `EVENTER`, `KONAMISTAFF`).
Qué va como bin y qué como mapa directo: **P** — `compe_list` es una lista de submapas empaquetados (bin); `compe_data`, `compe_info`, `squad_bonus`, `reward` y los premios también bin. Si el cliente no los reconoce, devolver bin con el mismo patrón que ya funciona en logros/regalos (B.14, B.19).

## C.4 Textos (enums) que usa el cliente
- **Tipo de partida/modo** (16 valores, orden = índice): `RANK_MATCH`, `RANK_MATCH_COACH`, `COMPE_MATCH`, `COMPE_MATCH_COACH`, `VSCOM_MATCH`, `VSCOM_COMPE`, `EVENT_COMPE`, (un valor `FREE…` no legible), `PESLEAGUE`, `MATCHDAY`, `RANK_COOP`, `VSCOM_COOP`, `EVENT_COMPE_COOP`, `VSCOM_COMPE_COOP`, `PESLEAGUE_COOP`, `MATCHDAY_COOP`. **L** (tabla en 0x1437F22C8).
- **Tipo de partida de myClub** (7 valores): (`COOP…`), `MYCLUB_COOP`, `PESLEAGUE`, `MYCLUB_PESLEAGUE_COOP`, `EVENT_COMPE`, `EVENT_COM_COMPE`, `FOLLOW`. **L**
- **`compe_mode`**: no hay comparación visible en los lectores (el cliente solo lo guarda y lo reenvía); el valor lo pone la pantalla. **P**: para este frente es `EVENT_COMPE` (o `EVENT_COM_COMPE` en el modo contra CPU). **Recomendación:** aceptar cualquier texto y devolverlo tal cual.
- **Dificultad (`compe_cpu_level` / `cpu_level` / `difficulty`)**, 6 valores: `COMPE_DIFFICULTY_NONE`, `COMPE_DIFFICULTY_1` … `COMPE_DIFFICULTY_5`. **L** (el valor por defecto del cliente es `COMPE_DIFFICULTY_NONE`).
- **Tipo de competición** (5 valores): `ONEMATCH`, (`KO`), `LEAGUE`, `COMEVENT`, `EXHIBITION`. **P** para `compe_kind` (los nombres coinciden con eventos/ligas).
- **Resultado de partido** (3 valores): `NORESULT`, `FINISHWIN`, `FINISHLOSE`. **P** para `compe_result` en `MAINMENU_INFO`.
- **`status`** de `MAINMENU_INFO`, `compe_list_kind` y `entry_type`: **N** (no se halló la tabla; no hay comparación literal). Devolver texto corto y probar; empezar con `status:"ENTRY"` si ya está inscrito y vacío si no.

## C.5 Códigos de error `result` propios de estos comandos (L)
- `JOIN_COMPEV`: `ERR_COMPE_BEFORE_ENTRYTIME` (aún no abre), `ERR_COMPE_AFTER_ENTRYTIME` (ya cerró), `ERR_COMPE_FULL`, `ERR_COMPE_JOIN_MANNER`, `ERR_COMPE_NOTFOUND`.
- `LEAVE_COMPEV`: `ERR_COMPE_NOTENTRY`, `ERR_DATABASE`. `LEAVE_COMPEV_MODE`: `ERR_DATABASE`.
- `REENTRY_COMPEV`: `ERR_COMPE_AFTER_ENTRYTIME`, `ERR_COMPE_JOIN_MANNER`, `ERR_COMPE_NOTFOUND`, `ERR_DATABASE`, `ERR_NOTREGISTERED`.
- Otros de la familia: `ERR_COMPE_AFTER_MATCHTIME`, `ERR_COMPE_BEFORE_MATCHTIME`, `ERR_COMPE_MATCH_NOTFOUND`, `ERR_COMPE_JOIN_SANCTION`, `ERR_COMPE_ENTRY`.

## C.6 Tope por trama y menú de entrada
- **Tope de la lista:** no hay tope literal en el lector; la lista trae `compe_num` + `compe_list` en una sola respuesta. **N**. Conviene no pasar de ~20 eventos por trama (límite práctico de los demás listados del servidor).
- **Entrada desde el menú:** `Online/MyClub/MenuEventCategoryList` (categorías de eventos) → `Online/MyClub/EvCompe/MenuMainEvCompe` (menú del evento) → `MenuCompeList` (lista) → `MenuCompeDetail` (detalle). Después del partido: `MenuAfterMatch`, `MenuRewardDialog`, `MenuCommendation`; las pantallas `MenuSquadBonus…` muestran las bonificaciones de plantilla (`squad_bonus`) y `Information/MenuNoticeCompe` el aviso del evento. **L** (nombres de pantalla); el enlace entre pantallas va por tablas de flujo, así que el orden exacto es **P**.
- **Primer comando probable:** `GET_COMPEV_MAINMENU_INFO` al abrir `MenuMainEvCompe`, luego `GET_COMPEV_LIST` al entrar en la lista y `GET_COMPEV_INFO` al abrir el detalle; `JOIN_COMPEV_MODE` al entrar al modo y `JOIN_COMPEV` al inscribirse. **P**.

## C.7 Cómo implementarlo en el servidor (propuesta)
1. Primero manejadores vacíos que devuelvan `result:'NOERR'` para `JOIN/LEAVE_COMPEV_MODE`, `LEAVE_COMPEV`, `SYNC_COMPEV_PROGRESS`, `GET_COMPLG_MYENTRY_COMPEID`.
2. `GET_COMPEV_LIST` con `compe_num:0` y `compe_list` vacía (pantalla vacía sin romper); luego una competición de prueba con `compe_id`, `compe_name`, `compe_kind:"COMEVENT"`, fechas y `difficulty:"COMPE_DIFFICULTY_NONE"`.
3. `GET_COMPEV_INFO` devolviendo los mismos datos más premios (objeto regalo, B.11). `MYENTRY`: tabla `inscripciones(pid, compe_mode, compe_id, cpu_level, progreso)`.
4. Para torneos creados desde la web: una tabla `eventos_compe` en SQLite que alimente la lista y el detalle; los premios usan el mismo cobro que los logros (B.17).
5. Probar con el PES base entrando por myClub → Eventos y mirar `comandos_sin_manejador` y las capturas `*.jsonl` para confirmar las marcas **P/N** de este documento.

## C.8 Cómo se juega un partido de evento contra la COM (L salvo lo marcado)
**Resumen: no hay comandos propios de evento para jugar.** El partido de evento usa los mismos dos comandos que el partido normal contra la COM de myClub, y el servidor debe recordar en qué evento está inscrito el jugador (`JOIN_COMPEV`/`MYENTRY_COMPEID`).
- **Al empezar:** `CMD_GET_MYCLUB_VSCOM_OPPONENT`. Pedido: `is_coach_mode`, `is_team_power_lv` (sin `compe_id` ni `compe_mode`). Respuesta: la del rival de siempre (`team_name`, `team_name_short`, `base_team_id`, `team_data`, `squad_id`, `member_list`, `gameplan_data(_size)`, `concept_data`, `coach_unique_id_list`, `cpu_level`…) más **`is_entry_compe`** (texto): la marca de que el partido cuenta para un evento. **P:** poner `is_entry_compe:"true"` cuando el jugador tenga evento inscrito; y `cpu_level` con la dificultad del evento (`COMPE_DIFFICULTY_n`, el que se dio en `JOIN_COMPEV`).
- **Al terminar:** `CMD_SET_MYCLUB_VSCOM_GAMERESULT`. Pedido: `cpu_level`, `gameend_reason`, `winner_side`, `ex_flag`, `pk_flag`, `score` (+ `score_1st/2nd/ex1st/ex2nd/pk`), listas de goleadores, asistentes, amarillas, rojas y lesionados. Tampoco lleva `compe_id`. Respuesta: `rating`, `oldrating`, `rating_type`, `rating_boost_type/rate`, **`rating_compe`**, `contWin_num`, `skill_rank`, `is_rankup_skill_rank`, `is_first_skillrank`, `is_first`, `gameend_reason`, `team_id`, `player_data` (bin). No contiene premio ni progreso del evento.
- **Dónde se informan `win_point`, `progress_game_count`, `clear_count`:** solo en la respuesta de `GET_COMPEV_MAINMENU_INFO` (C.2); el cliente la pide al volver al menú del evento tras el partido (`MenuAfterMatch`/`MenuMainEvCompe`). Por tanto el servidor, al recibir `SET_MYCLUB_VSCOM_GAMERESULT` con evento inscrito, debe sumar el partido a su progreso (`progress_game_count`+1; `win_point` según resultado) y devolverlo luego en `MAINMENU_INFO`. **P** (deducido: ningún otro comando los transporta).
- **Premio (`MenuRewardDialog`, `MenuCommendation`):** no viaja en el resultado del partido. Se muestra con los premios de `GET_COMPEV_INFO` (`compe_reward`, `event_compe_reward_list`, `reward`, `is_compe_reward_exist`, `cleared_count`) y, si son objetos, llegan al buzón como regalos (`CMD_GET_MYCLUB_PRESENTLIST`/`RECV_MYCLUB_PRESENT`, B.14). **P.**
- **«Superado»:** **P** — se ve cuando el cliente recibe `GET_COMPEV_MAINMENU_INFO` con `compe_result:"FINISHWIN"` y `clear_count` mayor que 0 (y/o `progress_game_count` ≥ `clear_match_num`, `win_point` ≥ `clear_win_point` de `GET_COMPEV_INFO`), y en la lista/detalle con `is_cleared:"true"` (y `cleared_count` > 0). Los umbrales `clear_match_num` y `clear_win_point` son los que decide el servidor en `GET_COMPEV_INFO`: así se define «superado» (p. ej. `clear_match_num:1`, `clear_win_point:3`).
- **Logro relacionado:** la categoría 8 `JOIN_EVENT_COMPE` de los logros (B.19) cuenta inscripciones a eventos; la 3 `JOIN_VSCOM_COMPE` las de vs-COM.
- **No comprobado (N):** ninguna de las respuestas anteriores se ha visto en captura; el orden exacto de comandos al volver al menú (si pide `MAINMENU_INFO` o `INFO` primero) se verá en la prueba con el PES base.
