# Estudio profundo de fuentes — Phoenix Online (11 nov)

Este documento continúa `BUSQUEDA_RECURSOS_ONLINE_11NOV.md`. Esa búsqueda fue amplia; aquí se leyó a fondo lo que ya habíamos encontrado.

**Cómo se hizo:** todo es **lectura estática**. Se clonaron los repositorios con `git clone --depth 1` en `scratchpad/fuentes/` y se leyeron con cat, grep y sed. No se ejecutó, importó ni compiló nada de lo clonado.

**Cómo se citan las fuentes:** cada dato lleva su archivo dentro del repo. Lo que es una suposición mía lo dice con la palabra **(suposición)**.

## Repos leídos

| Nombre corto | Repo | Qué es |
|---|---|---|
| `open-eleven` | moth1995/open-eleven-server (+ fork amir14a) | Servidor de PES 2011 en C#. Usa la misma trama NclMio que PES 2021, pero lleva texto clave=valor dentro en vez de msgpack. |
| `tpp-server` | tpp-server-emulator | Servidor de MGSV (motor Fox de Konami), en C++. |
| `mgsv-emu` | mgsv_emulator | Otro servidor de MGSV, en Python. |
| `tpp-mod` | tpp-mod | DLL cliente para MGSV que redirige el juego. |
| `sider7` | sider7 | Sider para PES 2021, incluido el módulo `modules/netblock.lua`. |
| `nikow5` | Nikow5/Pes2021PrivateServer | Rama `pes2021-server-emulator-…`. |
| `fiveserver` | juce/fiveserver | Servidor de PES5/PES6, en Python. |
| `AzrakHQ` | Nuestra referencia, ya portada | Se usa aquí como «diccionario» de PES 2021: `data/commands.json` (433 comandos), `schemas.json` y `notes/re/functions/*.md`. |

El fork amir14a es más viejo (26 oct, frente a 29 oct del original) y es un subconjunto: no trae nada extra.

**Cifras del cruce de comandos:**
- OpenEleven: 318 nombres CMD.
- PES 2021 según AzrakHQ: 436 nombres (schemas, commands.json y server_errors.tsv).
- Nuestro servidor: 323 nombres (funciones `c_cmd_`, alias y menciones en `logica.py`).
- PES 2021 menos lo nuestro menos lo de AzrakHQ: **120 comandos que nadie atiende**.
- De esos 120, unos 60 son de tienda, myClub, cooperativo o Matchday y no hacen falta para la liga.

---

## 1. Comandos que nos faltan

Solo están los que existen en PES 2021 según `AzrakHQ data/commands.json` y que no atendemos ni nosotros ni AzrakHQ. Los campos salen de `AzrakHQ schemas.json` o `commands.json`.

**Prioridad:**
- **A** = hacerlo ya, afecta a partidos de liga.
- **B** = útil.
- **C** = basta con un NOERR mínimo.
- **—** = ignorar.

| comando | fuente | qué hace | campos | prioridad |
|---|---|---|---|---|
| CMD_SET_PLAYDATA_MATCH (HTTP, urltype=PLAYDATA_MATCH) | AzrakHQ `notes/re/functions/results.md:124`; `commands.json` (15 veces visto en capturas) | Al acabar el partido, cada cliente manda un JSON con todo el partido. **Hoy lo perdemos:** `respuesta_http` en nuestro `logica.py` (~6530-6600) responde NOERR y lo descarta. | Pide `pid`, `play_data`. Dentro de `play_data`: `gamemode`, `scorer[]`, `win_lose`, `team[2]{team_id, member[]{player_id, play_time, rating, player_overall}}`. Responde NOERR. | **A** |
| CMD_WATCH_CHECK_ALIVE_PLAYER (aviso que manda el servidor) | AzrakHQ `notes/re/functions/match.md:297-333`, `results.md:225` | El servidor pregunta a cada cliente «¿a quién sigues viendo?». Sirve para saber **quién** se desconectó. | `pid` | **A** |
| CMD_SEND_ALIVE | AzrakHQ `match.md:297-333`; `commands.json` | Respuesta del cliente al aviso de arriba, con el motivo. | Pide `pid`, `reason`: DISCONN_SC (15) = se cayó el enlace P2P; DEGRADE_SC (14) = conexión degradada. | **A** |
| CMD_SEND_ALIVEPLAYER | AzrakHQ `match.md:297-333` | El cliente manda la lista de rivales que sigue viendo. | `pid`, `pid_list`, `pid_list_num` | **A** |
| CMD_WATCH_PUNCHING_DATA (aviso) | AzrakHQ `notes/re/functions/net.md:136`; `schemas.json` | El servidor pasa a un cliente las direcciones del rival para «perforar» el NAT. | `address_family`, `host_addr`, `reflexive_addr`, `sa_data`, `ntl_nat_type` | **B** (suposición: quizá ayude con los DISCONN_P2P antes del saque) |
| CMD_SET_GUEST | `open-eleven docs/reverse-engineering/pes2011-cmd-reference.md`; AzrakHQ `commands.json` | Avisa de que el jugador trae un invitado (2.º mando) a la sala. | `has_guestplayer`, `guest_info{pname, name, xuid_one, psn_user_id, account_id, online_id, is_valid_cache, is_deleted, is_platform_guest}` | **B** |
| CMD_GET_GAME_SCORE | AzrakHQ `notes/re/functions/rooms.md`, `lobby2.md`; `schemas.json` | Marcador del partido de la sala. | Pide `pid`. Responde `score[]{team, score_1st, score_2nd, score_ex1st, score_ex2nd, score_pk}` | **B** |
| CMD_GET_ENTRY_GAME | AzrakHQ `rooms.md`; `schemas.json` | Partidos en los que el jugador está inscrito. | Pide `pid`. Responde `entry_count`, `entrygame[]`, `entryNum`, `gameNum` | **B** |
| CMD_GET_ROOM_BESTPLAYER | AzrakHQ `lobby2.md`; `schemas.json` | Mejor jugador por puesto en la sala. | Pide `pid`, `room_id`. Responde `position_best_player[]{pid, rating, game_count}` (vale vacío) | **C** |
| CMD_GET_HEROSTATS | AzrakHQ `schemas.json` | Totales del jugador: partidos, victorias, goles, racha. | Pide `pid`. Responde `division_match_num`, `division_win_num`, `is_current_match_win/lose`, `total_goal_num`… | **B** (sale de nuestra base de datos) |
| CMD_SET_TRACKRECORD | AzrakHQ `commands.json` (sin esquema) | Guarda el historial del jugador. | Sin datos | **C** |
| CMD_SEND_KICK_REQUEST | AzrakHQ `notes/re/functions/survey.md:53-59` | Petición de expulsar a alguien (juego limpio). | Sin esquema (suposición: `pid` + objetivo) | **B** (guardarlo en el registro de moderación) |
| CMD_WATCH_MATCHING_CONDITION (aviso) | AzrakHQ `schemas.json` | El servidor dice qué rango de rating y de deportividad acepta el emparejamiento. | `rating_min`, `rating_max`, `manner_min`, `manner_max` | **B** |
| CMD_REQWATCHL_BLOCK_MATCH_LIST | AzrakHQ `schemas.json` | Lista de jugadores bloqueados para emparejar. | Lista con `list_type` (ADD, DEL, UPDATE), `list_num`, `list_data[]{pid, name, …}` | **C** (lista vacía con `list_type`) |
| CMD_SEND_DOWNLOAD_STATS_DATA | AzrakHQ `commands.json` | Estadísticas de una descarga de archivo. | `pid`, `filename`, `file_hash`, `download_type`, `download_time`, `downloaded_size`, `average_bps`, `max_bps`, `min_bps`, `result_kind` | **C** (registrar el `file_hash`: sirve para saber qué parche lleva cada uno) |
| CMD_SEND_MYCLUB_MATCH_RATING | AzrakHQ `commands.json` | Notas de los jugadores tras un partido de myClub. | `pid`, `rating_list`, `opponent_rating_list`, `rating_num`, … | **C** |
| CMD_SEND_COOP_ALIVE | AzrakHQ `commands.json` | Prueba de vida en un grupo cooperativo. | `pid`, `group_id` | **C** |
| CMD_GET/SET_PLATFORM_SESSION, CMD_GET/SET_COOP_ROOM_PLATFORM_SESSION | AzrakHQ `schemas.json` | Sesión de la plataforma (Steam) ligada a la sala. | `pid`, `room_id` / `data_size`, `session` | **C** |
| CMD_SET_COOP_ROOM_GUEST y el resto de CO-OP (START/CANCEL_COOP_GROUP_MATCHING, WATCH_COOP_*) | AzrakHQ `commands.json` | Salas cooperativas 2v2 y 3v3. | Ver `faltan_detalle.tsv` | **C** |
| Matchday (START_MATCHDAY_PICKUP_MATCHING, WATCH_MATCHDAY_PICKUP_*, GET_MATCHDAY_FINAL_RESULT_INFO…) | AzrakHQ `commands.json` | Evento Matchday de Konami. | Ver `faltan_detalle.tsv` | **—** |
| TEAM_DATA_SYNC, SELECT_SIDE, COMPLETE/UPDATE_TEAM_SELECT, CMD_LACK, CMD_SENDFAILED, LOST_GUEST, SYNC_COMMENTARY, USER_DATA_SYNC, REVISION_CHECK, BL_DATA_SYNC | AzrakHQ `survey.md` («In-match sync») | **Suposición fuerte:** son mensajes P2P entre los dos jugadores, no comandos al servidor. CMD_LACK aparece como motivo de fin anormal. | — | **—** (no implementar) |
| Tienda y prueba (START/END_STEAM_STORE, *_STORE_TRANSACTION, *_E_FOOTBALL_POINT, LOGIN_TRIAL, GET_TRIAL_URLLIST, KONAMIID_*) | AzrakHQ `commands.json` | Compras, monedas y versión de prueba. | — | **—** (ver «Evitar») |

**Lista completa de los 120**, con área, transporte y campos: `scratchpad/estudio/faltan_detalle.tsv`.

### Comandos de OpenEleven que NO existen en PES 2021

En `AzrakHQ data/commands.json` no hay ninguna coincidencia para estos. **No hay que copiarlos tal cual.**

| Comando de PES 2011 | Qué lo sustituye en PES 2021 |
|---|---|
| SEND_P2PFAILED, SEND_P2PSTATUS | `error_code` de SEND_MATCHING_RESULT y SET_GAMERELAY_CHECK_RES |
| SEND/GET/WATCH_IPANDPORT | Las versiones `_NTL` |
| WATCH_ALIVE | La prueba de vida (WATCH_CHECK_ALIVE_PLAYER) |
| WATCH_DISCON_PLAYERENV, WATCH_DISCON_PLAYERMATCH, SEND_DISCON_* | Motivo de SEND_ABNORMALEND |
| FORFEIT*, INJURYGAMEEND, OWNGOALGAMEEND, WATCH_ABNORMALEND | Motivo de SEND_ABNORMALEND |
| GET_GAMEADDPOINT | REQ/WATCH_GAMEADDPOINT |
| KICK_ROOMMEMBER, CHANGE_ROOMNAME, UPDATE_ROOMSTATE, WATCH_FORCELEAVEROOM, WATCH_MAINTETIME, WATCH_EMERGENCY | — |

**(suposición)** Las equivalencias salen solo de la tabla de AzrakHQ y hay que comprobarlas.

De los 47 comandos que comparten PES 2011 y PES 2021, solo nos faltan **SEND_ALIVE** y **SET_GUEST**.

---

## 2. Ideas de flujo que nos faltan

1. **Decidir quién abandonó.** Hoy usamos `_programar_derrota`: 15 s de gracia, y si se va uno solo, pierde.
   - Al detectar un fin anormal, mandar WATCH_CHECK_ALIVE_PLAYER a los dos.
   - Cruzar las respuestas SEND_ALIVE (`reason`) y SEND_ALIVEPLAYER (`pid_list`):
     - quien sigue viendo al rival y sigue conectado **no** es el culpable;
     - si los dos siguen conectados pero el P2P se rompió, el partido **se anula** (no es derrota).
   - Fuentes:
     - AzrakHQ `match.md:297-333`.
     - Regla en `open-eleven src/OpenEleven.Server/State/MatchResults.cs` (ForfeitGrace = 15 s, «P2P roto pero ambos conectados = nulo»).
     - `fiveserver lib/fiveserver/protocol/pes5.py` ~395-430 («MUTUAL DISCONNECT» → partido descartado).
2. **Reconexión que no borre la sesión nueva.** Así arreglaron 40 casos de DISCONN_P2P antes del saque:
   - el problema: una limpieza retrasada de la conexión vieja borraba la sesión nueva;
   - la solución: identificar la **sesión exacta**, no solo el `pid`.
   - Fuente: evoweb, hilo PESBUL, página 9.
   - En nuestro código, `cerrar()` ya limpia por objeto de sesión. Aun así, conviene revisar que `c_cmd_reconnect` (~1683) y los temporizadores de derrota comprueben la sesión y no solo el pid **(suposición: no lo verifiqué paso a paso)**.
3. **Elegir la dirección interna si están en la misma red.** Si los dos jugadores tienen la misma IP externa, hay que darles la IP y el puerto internos (`in_ip`/`in_port`). Si no, el router no hace «hairpin» y el P2P falla.
   - Fuente: `open-eleven src/OpenEleven.Server/State/Hub.cs:66-83` (EndpointsFor).
4. **Usar la IP real del socket.** Al guardar la dirección del jugador, sustituir la `ex_ip` que dice el cliente por la IP de origen TCP. Evita direcciones falsas o mal detectadas.
   - Fuentes: `tpp-server src/server/platforms/tppstm/endpoints/main/commands/cmd_send_ipandport.cpp` (`use_real_client_ip`) y `open-eleven …/Handlers/ModernLobbyHandlers.cs:113`.
5. **Mensajes troceados (0x70/0x71/0x72).** Nuestro comentario dice que PES «no los aceptó» (05-nov). Hay tres diferencias con OpenEleven:
   - OpenEleven trocea **sin comprimir** (flag 0), en trozos de ~1000 bytes, con un máximo de 0x19000 por mensaje (`open-eleven src/OpenEleven.Protocol/Framing/InnerBody.cs`, WrapChunked).
   - OpenEleven pone a **cada trama un contador nuevo**, `session.NextCounter()` (`open-eleven …/Transport/GameConnection.cs:295`).
   - Nosotros (`nclmio.py`, `_troceado`) usamos zlib (flag 1) y repetimos la misma `secuencia` en todos los trozos.
   - Formato de los trozos (`pes2011-session-protocol.md` §1.5): 0x70 = [total X][C][Y inflado][flag][C bytes]; 0x71/0x72 = [C][bytes]. Abrir un 0x70 nuevo con otro abierto es error fatal.
   - **(suposición)** Probar con flag 0 y contador nuevo por trozo podría ser lo que faltaba.
6. **Inactividad y latido.** Hoy `inactividad_max_s = 0`, es decir, sin límite.
   - Valores de otros servidores:
     - OpenEleven: 300 s sin tráfico, y el latido mantiene la conexión (`GameConnection.cs`, WatchIdleAsync).
     - tpp-server: latido 60 s y caducidad 200 s (`tpp-server docs/usage.md`).
   - Propuesta: 300 s, para limpiar sesiones colgadas sin cortar partidos.
7. **Orden de la puerta.** Según PESBUL, el cliente pide SVRLIST → URLLIST → SVRVERSION → SVRTIME.
   - Una URLLIST mínima válida es `{result, url_num: 0, ulist: []}`.
   - Los elementos de `ulist` deben ir como msgpack **BIN**, no como texto.
   - Fuente: evoweb PESBUL, página 9. Comprobar que lo cumplimos.
8. **Ajustes de red que manda el servidor.** OpenEleven los envía al entrar (`open-eleven …/Handlers/AccountHandlers.cs` ~380-405):
   - `p2ptimeout 30000`;
   - `red_ping 65535`;
   - `delay_frame_*`;
   - `max_input_delay_frames 90`;
   - `lossframe_match 30`.
   - **(suposición)** PES 2021 podría tener equivalentes en la respuesta de entorno; conviene buscarlos en `AzrakHQ schemas.json`.
9. **GameRelay es clave.** Según PESBUL (página 10):
   - unos 90 % de los modos dependen de GameRelay;
   - en conexión WAN va a unos 20-22 de 54 pasos por segundo;
   - solo el amistoso 1v1 funciona con P2P puro.
   - En la página 8, su beta en un VPS (8 núcleos, 16 GB) llega al intercambio de claves de GameRelay en 11v11, pero no a la elección de lado.

---

## 3. Lanzador sin archivo hosts

Hay tres formas de redirigir el juego sin tocar `hosts` y sin modificar el .exe en disco.

### a) Módulo Lua de Sider (sin permisos de administrador)

**Cómo funciona `netblock.lua`** (`sider7 modules/netblock.lua`):
- busca en memoria `"pes21-x64-gate.cs.konami.net\0"` y `"pes21-x64-stun.cs.konami.net\0"` con `memory.search_process`;
- escribe encima `"0.0.0.0\0"` con `memory.write`.

**Detalles de las funciones** (`sider7 modules/memory.lua` y `doc/scripting.html`):
- `memory.write` cambia la protección con VirtualProtect y copia los bytes;
- `search_process` devuelve **solo la primera** coincidencia.

**Qué haría falta para redirigir a nuestro servidor:**
- **repetir la búsqueda** hasta no encontrar más, porque los ntl* salen dos veces en el exe recortado (cap_62);
- escribir nuestra IP o un nombre **igual de largo o más corto**, terminado en `\0`.

**Prueba de que se usa:** en evoweb, hilo PESBUL, página 8, usan un módulo Sider con `local replacer = "151.247.209.104"` como alternativa al conector .BAT.
- Si el nombre nuevo es más largo, marqisspes5 propone apuntar el **puntero** a un búfer propio, como hacía kitserver5.

**Nombres de PES 2021 que hay que cubrir:**

| Nombre | De dónde sale |
|---|---|
| `pes21-x64-gate.cs.konami.net` | Puerta; netblock.lua y AzrakHQ `server/mod/pes2021online.c` |
| `pes21-x64-stun.cs.konami.net` | STUN; ídem |
| `ntl.service.konami.net`, `ntleu.service.konami.net`, `ntlus.service.konami.net`, `ntljp.service.konami.net` | HTTPS de NTL; AzrakHQ `pes2021online.c` |
| `info.service.konami.net` | Ya en `huecos_servidor.md` |
| `pesam.stun.service.konami.net`, `pes21-pc.konamionline.com` | Ya en `huecos_servidor.md`; 0 veces en el exe recortado |

**Cuidados:**
- Los ntl* van por **HTTPS**. Cambiar el nombre no basta: el juego tiene que aceptar nuestro certificado. AzrakHQ lo resuelve con un gancho que acepta el certificado propio solo para ntl* (`pes2021online.c`). Con Sider solo **(suposición)** fallaría la parte HTTPS.
- Hay que **dejar netblock apagado**. Si no, pone 0.0.0.0 antes que nosotros.

### b) DLL proxy dinput8 con ganchos de red (lo que ya hacen AzrakHQ y nuestro Core)

**AzrakHQ** (`server/mod/pes2021online.c`) usa MinHook sobre `getaddrinfo`, `GetAddrInfoW/ExW` y `gethostbyname`.
- Además verifica el servidor con una firma Ed25519.

**Nuestro Core** ya documenta ganchos parecidos en `huecos_servidor.md:242-258` y `PHOENIX_ONLINE_REA_GUIA.md:135-181`.

### c) Idea de tpp-mod: cambiar un puntero en memoria (`tpp-mod src/client/component/backend_server.cpp`)

- No toca el .exe en disco. Es una DLL proxy que parchea en memoria.
- `utils::hook::inject(<dirección>+3, custom_url)` hace que la instrucción que carga la URL apunte a **un búfer propio**. Así cabe una URL de cualquier largo.
- Funciones extra que nos sirven de ideas:
  - **datos de guardado separados por servidor**, en la carpeta `server-<sha1(url)[:8]>`, con ganchos de lectura y escritura de archivos;
  - un gancho al codificador HTTP que **vuelca cada petición y respuesta como JSON** (`net_server_logging`), muy útil para depurar;
  - **reintento de hasta 3 errores** antes de soltar la sesión (`net_update_session_failure_stub`).
- Solo acepta las versiones originales de Steam, comprobadas por SHA1.
- Las direcciones son de MGSV y **no sirven para PES 2021**. Solo vale la técnica.

**Recomendación (suposición):**
- seguir con el Core (gancho de `getaddrinfo` más certificado para ntl*);
- ofrecer el módulo Sider como plan B sin administrador, solo para la puerta y el STUN.

---

## 4. Criptografía/puerta

- **PES 2011 y PES 2021 usan la misma trama** (`open-eleven docs/reverse-engineering/pes2011-session-protocol.md` §1.4):
  - XOR 5B 9F 2E 64;
  - cabecera: tipo, largo y contador, más MD5;
  - cuerpo en Blowfish-ECB.
  - Esto confirma nuestro `nclmio.py`.
- **Nikow5** (`nikow5 server_tcp/main.py`):
  - **lo correcto:** la clave XOR, que es la misma que la nuestra;
  - **lo que le falta:** MD5, Blowfish y msgpack;
  - **lo que adivina:** el tipo de mensaje por el tamaño del paquete;
  - **el error:** mete un puntero de vtable (0x14285B5B0) dentro de la lista de servidores.
  - `steam_auth.py` acepta cualquier ticket.
  - `trace.js` (solo para v1.07.02) engancha el validador `0x1411AEEC0` (C_IDGG_0002) y los estados 5, 6 y 10 del cliente, pero solo los registra.
  - **Conclusión: nada nuevo frente a AzrakHQ.**
- **Puerta del motor Fox (MGSV)** (`tpp-server src/server/platforms/common/gate_handler.cpp`, `main_handler.cpp`; `mgsv-emu README.md`, `emulator/blowfish.py`):
  - el cuerpo va como `httpMsg=`, cifrado con Blowfish estático;
  - dentro hay un JSON `{compress, data, original_size, session_crypto, session_key}`;
  - `data` va en zlib más base64, o en Blowfish de sesión;
  - el Blowfish de Konami **invierte los bytes de las S-box** (swap32).
  - **(suposición)** Si alguna petición HTTP de PES 2021 (ntl*) usa algo parecido, este es el primer sitio donde mirar. No copiar sus archivos de clave (`static_key*.bin`).
- **Comprobar el ticket de Steam sin internet** (`tpp-server src/server/utils/steam_ticket.cpp` y `src/server/database/auth.cpp`, `verify_ticket_offline`):
  - leer el ticket: sección GC de 20 bytes, cabecera de sesión de 24, ticket de propiedad de 46 o más, y firma RSA de 0x80 bytes;
  - comprobar el `app_id`, la caducidad y la firma RSA con la clave pública de Steam;
  - comprobar que el SteamID coincide;
  - hay además modos de autenticación (offline, hybrid, nosteam…) y listas `allow_list.json` y `deny_list.json`.
  - Nuestro `CMD_AUTH_STEAM` aún no verifica la firma ("pendiente").
  - **(suposición)** El app_id de PES 2021 es 1259970; hay que confirmarlo.
- **Orden de entrada en PES 2011** (`pes2011-session-protocol.md` §2 y §4):
  1. REG_SVRLIST
  2. GET_SVRVERSION
  3. GET_SVRLIST
  4. GET_HTTP
  5. GET_SVRTIME
  6. GET_URLLIST
  7. DISCONNECT
  8. AUTH_GAMEID
  9. LOGIN, con `parahash`, `entryhash`, `patchver` y `dlcver`.
  - Sirve para comparar con nuestro orden.

---

## 5. Otras ideas

- **Contra el «farmeo» de puntos:**
  - máximo **3 partidos puntuados por rival y por día**;
  - solo se puntúa si los dos tienen el **mismo hash de archivos**.
  - Fuentes: `open-eleven src/OpenEleven.Server/ServerOptions.cs:204/207`, `State/LiveSettingCatalog.cs:61-62,74` y `State/MatchResults.cs`.
  - Nosotros ya usamos `file_hash_list` para emparejar, pero no encontré un tope diario por rival.
  - PESBUL banea por manipular el rating («boosting»), con registros, cuentas vinculadas y partidos cortados repetidos (evoweb, página 10).
- **Opciones de servidor que vale la pena copiar** (`fiveserver etc/conf/sixserver.yaml`, `lib/fiveserver/protocol/pes5.py` ~545-580, ~148-158):
  - sala de **entrenamiento sin estadísticas** (noStats);
  - **«desconexión cuenta como derrota»**, con marcador configurable (p. ej. 0-3);
  - contador de desconexiones por perfil;
  - lista de IPs baneadas;
  - tope de usuarios.
- **API de administración** (`fiveserver lib/fiveserver/admin.py`):
  - ver usuarios y quién está conectado;
  - estadísticas;
  - bloquear o echar a un usuario;
  - añadir o quitar baneos;
  - cambiar el tope de usuarios;
  - ver registros.
  - Idea para el panel de la web de la liga.
- **Base de datos original obligatoria.** Fiveserver bloquea la entrada si el hash de la plantilla no coincide (`fiveserver lib/fiveserver/protocol/pes6.py:750-765`). PESBUL exige la base de datos original (evoweb, página 10).
- **Límite de entradas:** 20 logins por minuto por dirección (`open-eleven State/LoginThrottle.cs`).
- **Defensas** (`open-eleven docs/security-audit.md`):
  - tope contra bombas zlib;
  - límite de profundidad al leer mensajes;
  - ocultar datos sensibles en los registros.
- **Fórmula de puntos y divisiones** (umbrales 250, 450, 600 y 750): `fiveserver lib/fiveserver/rating.py`. Sirve de referencia para nuestro ELO.
- **Web de PESBUL** (pes2021.pesbul.eu):
  - centro de partidos en vivo, con minuto, goleadores y asistencias;
  - máximos goleadores;
  - ELO y divisiones;
  - estado del servidor y ping;
  - «Clasificación validada».
  - Con SET_PLAYDATA_MATCH guardado, podríamos tener algo parecido en la web de la liga.
- **Deportividad:** escala de 1 a 5 y recuperación tras 10 partidos (`open-eleven State/MatchResults.cs`). Nosotros usamos A/B/C.

---

## 6. Evitar

- **Crack, CPY, cambio de .exe, CreamAPI, Goldberg y «monedas gratis».** En el hilo de evoweb (página 10) se habla de CPY: no se tocó. No usar nada de eso.
- **Denuvo y anti-depuración.** `tpp-mod src/client/component/denuvo.cpp` existe; **no se abrió** y no hay que estudiarlo.
- **Archivos de clave** de mgsv-emu (`static_key*.bin`) y la clave estática de 16 bytes de tpp-mod: no copiarlos ni reproducirlos.
- **Comandos de tienda y compras** (STEAM_STORE, STORE_TRANSACTION, E_FOOTBALL_POINT, TRIAL, KONAMIID): no imitar compras; responder como mucho con un error o NOERR neutro.
- **Puntero de vtable de Nikow5** dentro de la lista de servidores: es un error. No copiarlo.
- **Usar las direcciones fijas de tpp-mod y de trace.js:** son de otras versiones o de otro juego.
- **Dejar netblock.lua activo junto con nuestro redireccionador:** se pisan entre sí.

---

## Anexo — dónde está el material de trabajo

En `scratchpad/estudio/`:

| Archivo | Contenido |
|---|---|
| `oe_all.txt` | Los 318 CMD de OpenEleven |
| `pes21_all.txt` | Los 436 nombres de PES 2021 |
| `ours_total.txt` | Nuestros 323 |
| `azrak_handled.txt` | Los 118 que atiende AzrakHQ |
| `pes21all_faltan.txt` y `faltan_detalle.tsv` | Los 120 que nos faltan |

**Páginas web guardadas** (cargaron con HTTP 200, sin bloqueo): `scratchpad/web/evoweb_p8/9/10.html` y `pesbul.html`.
