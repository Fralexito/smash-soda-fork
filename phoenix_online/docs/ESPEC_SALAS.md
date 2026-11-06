# Especificación de salas (vestíbulo amistoso) — formato exacto observado en PESBUL (4-nov-2026)

Fuente: lectura del código de la copia local de PESBUL (`server_tcp/lobby_core.py`, `main.py`) y de su `events.log` de hoy. Solo lectura, descrito con palabras propias; sin copiar su código.
Marcas: **L** = leído en su código; **C** = visto en vivo; **P** = suposición.
Motivo: con nuestro servidor, PES2021 se cerró solo (APPCRASH, `msvcrt.dll`, 0xc0000005, offset 0x7b212) ~1 s después de que respondimos a `CMD_CREATEJOIN_ROOM` en el vestíbulo amistoso (15:48:04 local). Complementa `ESPEC_GAMERELAY.md`.

## 0. Regla general del cliente (L)
- Todo campo anidado (lista o mapa) va como **bin que contiene msgpack** (empaquetado a mano y puesto como bytes), nunca como mapa/lista en línea. Un mapa en línea hace que el lector copie un valor creyendo que es un puntero → acceso inválido (un crash de este tipo lo vieron ellos varias veces).
- Todo **indicador/enumerado va como TEXTO** (`"YES"`/`"NO"`/`"HOME"`/`"AWAY"`/`"WAITING"`…). **Un entero o un booleano donde se espera texto cierra el juego**; un texto desconocido se toma como 0 sin error.
- Los contadores/identificadores sí son enteros.

## 1. Fila de sala (CMD_REQWATCHL_ROOMLIST, un elemento de `list_data`) (L)
Cada elemento = **bin( msgpack(mapa) )**, con estas claves (valores para una sala recién creada por un solo jugador, sala de amistoso `OC_FREE`):

| Clave | Tipo en el cable | Valor de ejemplo |
|---|---|---|
| slot | int | 0 (posición 0..N-1 contigua) |
| room_id | int | id de la sala (distinto de 0) |
| name, room_name, roomName | str | nombre de la sala (los tres iguales) |
| status | **str** | "WAITING" |
| game_phase | **str** | "DATASYNC" |
| gameenv | **bin(mapa)** | ver §1b |
| gamer_num | int | 1 |
| gamer | array de **bin(mapa)** | un elemento por jugador (§1c) |
| is_room_owner | **str** | "YES" si quien pregunta es el dueño, si no "NO" |
| enter_no | int | nº de entrada del que pregunta (1 si es el dueño) |
| game_no | int | 1 |
| is_passwd | **str** | "NO" / "YES" |
| max_players | int | 2 |
| is_invite_limit | **str** | "NO" (se devuelve lo que mandó el cliente) |
| dlcontent | str | "7.00" |
| is_original_player | str | lo que mandó el cliente en `use_original_player`, "YES" por defecto |
| vs_com | **str** | "PL_VS_PL" (o "PL_VS_COM"), eco del cliente |
| owner_pname | **bin(mapa)** | ver §1d |
| owner_lang | str | "EN" |
| room_user_count | int | nº de jugadores (1) |
| is_anyplayers | str | "NO" (eco del cliente) |
| is_select_gk | str | "NO" (eco) |
| auto_start_num | int | 0 |
| room_number | int | igual a room_id |
| game_rating_limit | str | "NONE" |

Sobre de la respuesta/empuje (L):
`{msgid:"CMD_REQWATCHL_ROOMLIST", rqid:<rqid del watch original>, result:"NOERR", list_type:"RELOAD", list_num:<n>, list_key_name:"slot", list_data:[bin,...]}`.
- `list_type` debe ser **"RELOAD"** (con "ADD" el cliente deja el modelo vacío y acaba en el error VZLN_0002; con entero 0 no limpia). Para vaciar se manda RELOAD con `list_num:0, list_data:[]`.
- `list_key_name` debe ser **"slot"**, no "room_id".
- La **primera** respuesta (cuando el cliente abre el watch) es el mismo sobre con la lista de salas actuales (vacía = RELOAD, 0, []).

### 1b. gameenv (L)
bin(mapa) con claves: cpuLevel, gametime, injury, ball_type, exGame, pkOnOff, substitution, exSubstitution, limitTime. Valor por defecto de la fila de sala (enumerados en texto): cpuLevel "HARDEST", gametime 10 (int), injury "YES", ball_type 22 (int, 0x16), exGame "NO", pkOnOff "NO", substitution 3 (int), exSubstitution "YES", limitTime "MIDDLE". (Hay otro `default_gameenv()` con enteros: cpuLevel 2, gametime 10, injury 0, ball_type 0, exGame 0, pkOnOff 1, substitution 3, exSubstitution 0, limitTime 0. Es el estado interno; **lo que sale al cable en la fila es el de texto**.)

### 1c. gamer[] (L)
Cada elemento = bin(mapa): pid (int), is_room_owner ("YES"/"NO"), side ("HOME" el primero, "AWAY" el segundo), enter_no (int, 1..), game_no (int, 1).

### 1d. owner_pname (L)
bin(mapa): name (str, nombre del jugador), xuid_one (str; PESBUL usa 76561197960287930 + (pid-1) como texto), psn_user_id (**bin(mapa)**: account_id int = pid, online_id str = nombre, is_valid_cache str "YES"), is_deleted (str "NO").

### Qué mandó NUESTRO servidor (C) y por qué cae
Nuestra fila llevaba: `status:"WAITING"` (bien), `game_phase:0` (**int; debe ser "DATASYNC"**), `gameenv:0` (**int; debe ser bin(mapa)**), `is_passwd:false` (**bool; debe ser "NO"**), `owner_pname:"FRALEX"` (**str; debe ser bin(mapa)**), `vs_com:false` (**bool; debe ser "PL_VS_PL"**), sin `gamer`, `gamer_num`, `slot`, `enter_no`, etc.; y el sobre con `list_type:"ADD"` y `list_key_name:"room_id"`. Varios de esos campos explican el acceso inválido.

## 2. Orden de mensajes tras CMD_CREATEJOIN_ROOM (L, parcialmente C)
1. El cliente envía `CMD_CREATEJOIN_ROOM` {name, match_type OC_FREE, lang, max_players, is_invite_limit, password, enable_guest, vs_com, use_original_player, is_anyplayers, is_select_gk, auto_start_num, game_rating_limit}.
2. El servidor responde **primero**: `{msgid, rqid, result:"NOERR", room_id}`.
3. **Después** empuja al propio creador, por su watch de ROOMLIST ya abierto, un `CMD_REQWATCHL_ROOMLIST` (rqid = el del watch original, **sin comprimir**) con `list_type:"RELOAD"`, `list_num:1`, `list_key_name:"slot"` y la fila de §1. Nosotros mandamos el empuje **antes** de la respuesta y como "ADD": orden inverso.
4. A los demás clientes que miran el vestíbulo les llega el mismo RELOAD con todas las salas del vestíbulo (en la prueba de hoy: 0 destinatarios).
5. El cliente abre luego el watch de jugadores: `CMD_REQWATCHL_ROOMPLAYERLIST_NTL` → respuesta de §3, y entra a la sala; ya dentro manda `CMD_ENTRY_GAME` y el servidor responde (§4).

## 3. Fila de jugador de CMD_REQWATCHL_ROOMPLAYERLIST_NTL (L)
Sobre: `{msgid, rqid, result:"NOERR", list_type:"RELOAD", list_num:<n>, list_key_name:"pid" (nombre por defecto del parser), list_data:[bin,...]}`. Literales de list_type aceptados: `DEL`, `ADD`, `UPDATE`, `RELOAD`; PESBUL envía RELOAD.
Cada elemento = bin(mapa) con:
- NAT (opcionales): host_addr, reflexive_addr, relayed_addr, cada uno = bin(mapa{address_family:int, sa_data:str}); `ntl_nat_type` int (0 si no hay datos).
- **Obligatorios (int; si falta o es nil el lector aborta):** pid, room_player_id (= pid), room_entry_no (nº de entrada), game_entry_no.
- pname = bin(mapa{name str, xuid_one str, psn_user_id bin(mapa{account_id int, online_id str, is_valid_cache str}), is_deleted str}); en PESBUL salen cadenas vacías excepto name y account_id del dueño.
- **Obligatorios (str):** lang ("US" o el idioma del cliente), is_owner ("YES"/"NO"), side ("" hasta que se elige lado), is_quality_bad (""), has_guest ("").
- guest_info = bin(lista vacía) (presente, vacío).

## 4. Respuesta a CMD_ENTRY_GAME (C, L)
`{msgid:"CMD_ENTRY_GAME", rqid, result:"NOERR", entryflag:"TRUE" o "FALSE" (eco del campo entry del cliente), gameNum:<n>}`. En la sesión de hoy el cliente mandó entry FALSE y side HOME. Con un solo jugador no se empuja lista de «entrada a partido» (entry_count 0). Al salir (`CMD_LEAVE_ROOM`) el servidor borra la sala y difunde un RELOAD vacío (`list_num 0`).

## 5. Otras respuestas del vestíbulo (C, L)
- `CMD_JOIN_BLOCK`: `{msgid, rqid, result:"NOERR"}` (solo eso; PESBUL lo marca como respuesta de diagnóstico, esquema exacto no demostrado).
- `CMD_CHECK_STRING`: `{msgid, rqid, result:"NOERR"}`; `"ERR_NGWORD"` si el texto contiene una palabra prohibida.
- `CMD_GET_BLOCKLIST`: `{msgid, rqid, result:"NOERR", count:1, bklist:[bin(mapa{name:"Main", player_num:0, max_player_num:100, block_id:<id de lobby>})]}`.
- `CMD_JOIN_LOBBY`: `gamerelay_web_url` no vacío (`http://<IP>:5000/gamerelay`).
- Cada fila del `GET_LOBBYLIST` lleva `gamerelay_index` = índice del nodo del relé (1), no el id del lobby.

## 6. Comparación de las 4 respuestas con PESBUL (por qué no se lanzó el qualitychecklist) (C+L)

| Comando | Nuestra respuesta (captura de hoy) | PESBUL |
|---|---|---|
| JOIN_ONLINE_MODE | {msgid, rqid, result NOERR} | misma forma (no hallé campos extra; P: solo se distingue por la lógica de RANKMATCH) |
| SEND_IPANDPORT_NTL | {msgid, rqid, result NOERR} | misma forma; además **guarda** host/reflexive/relayed y ntl_nat_type por sesión (lo usa luego en JOIN_ROOM_NTL y ROOMPLAYERLIST_NTL) |
| GET_SERVER_ENV | proto_opt como JSON (ya corregido) y el resto igual: max_division "D12", is_revision_check 0, quality_check_wait_sec 0, trigger_keep_alive 30, matchrejectpid_max 0, cuatro timeouts de partida 60, is_check_myclub_level 0, log_update_level 0, lobby_ping_good 50 / normal 150, quick_ping_good 50 / normal 150, konamiid_api_retry_sec 5, teamlobby_game_rating_border_normal y hard 0 | **idéntico campo por campo** (valor y tipo); la única diferencia que había era proto_opt |
| CHECK_CHEAT | {msgid, rqid, result NOERR} | idéntico |

Conclusión: **no hay diferencia de campos en esas 4 respuestas**. La comprobación de red de PESBUL llegó (20:14:19 UTC) con el cliente en modo QUICK, ~1 s después de CHECK_CHEAT. Con nuestro servidor el cartel «Comprobando entorno de red» sí apareció, pero el cliente **no hizo ninguna petición HTTP** (http.jsonl hasta 15:47:26 local: solo auth_steam y LiveDataSetFlow) ni UDP al 5100; solo STUN a 3478 cada 5 s. La URL anunciada en el gate es exacta (`http://192.168.100.236:5000/gamerelay/qualitychecklist`).

**Causa sin demostrar (P).** Hipótesis a probar: (a) en el vestíbulo el cliente no dispara el HTTP hasta tener sala; (b) el HTTP sale a otra ruta o puerto que nuestro servidor no registra (comprobar que se loguea TODO POST/GET); (c) depende del `ntl_nat_type` y las direcciones que guarda SEND_IPANDPORT_NTL. Recomendación: repetir tras corregir las salas y con registro de todas las peticiones HTTP/TCP.
