# Búsqueda profunda para Phoenix Online (12 nov 2026)

Continúa los informes del 11-nov (`BUSQUEDA_RECURSOS_ONLINE_11NOV.md` y `ESTUDIO_PROFUNDO_FUENTES_11NOV.md`). Aquí no se repite lo que ya estaba; solo lo nuevo o lo que ahora se ha leído de verdad.

## Cómo se hizo y límites

- Solo se leyeron páginas y se buscó en GitHub. No se descargó ni ejecutó nada, no se clonó nada.
- Herramientas: buscador web (solo EE. UU., resultados a veces pobres), búsqueda de código y de repos de GitHub (`gh search`), y lectura de páginas de `github.com` y `raw.githubusercontent.com`. Esos dos dominios SÍ se pueden leer desde esta sesión.
- No se pudieron abrir (la red de esta sesión no los resuelve): evoweb.uk, pesmodding.com, web.archive.org, tailscale.com, generationamiga.com, emulation.gametechwiki.com. Lo que digo de ellos viene solo del resumen del buscador y va marcado «sin verificar».
- No se puso el nombre ni el correo de Frank en ninguna búsqueda.
- Marca de confianza: **Alta** = lo leí yo en el texto de la fuente. **Media** = la fuente lo dice, pero es análisis estático o con una sola prueba. **Baja / sin verificar** = deducción mía o dato de un resumen del buscador.

---

## 1. Resumen en 10 líneas (lo más útil primero)

1. **Lo más valioso ya estaba en AzrakHQ, pero en sus carpetas de notas, no en el código:** `notes/re/functions/*.md` (31 archivos) responde casi todos nuestros problemas (PES League, Live Update, TURN, co-op, anti-trampas). El informe de ayer solo leyó una parte. Índice: https://github.com/AzrakHQ/pes2021-online-server/tree/main/notes/re/functions
2. **(b) Términos de uso del Torneo:** el cliente decide pedir el EULA con una regla exacta: pide `eula_agree` solo si `entry_compe` ≠ "YES" **y** `eula_url` no está vacío. El `team_id` NO es lo que lo decide. Si nuestro arreglo fue mandar `team_id`, puede haber sido casualidad: hay que comprobar `eula_url` (ver §2-b).
3. **(d) Live Update propio:** hay una receta completa y probada en vivo (CPK + SHA-256 + `version_wu.bin` + `dlc_version` como texto entero tipo "101"). Y un detalle que rompe muchos intentos: hay que **seguir listando la actualización cada vez** que el cliente la pida (§2-d).
4. **(a) Red/VPN:** nadie documenta Tailscale con PES 2021. Pero AzrakHQ documenta el problema de fondo (la dirección «reflexiva» sale como IP de LAN) y da un truco de diagnóstico: GateInfo con `LOG_ACTIVE:1` + `PUT_LOG_URL` hace que los clientes suban sus logs de red (§2-a).
5. **(e) Co-op 2v2:** AzrakHQ lo investigó y concluye que **2v2 no se puede con P2P**; necesita el «game relay» (`game_sycom:"SC"`) cuyo protocolo nadie ha descifrado. El 1v1 co-op sí funciona con un parche del juego («mod 0.5.0»). No hay nadie que lo haya resuelto (§2-e).
6. **(c) Regalos:** el comando real se llama `CMD_RECV_MYCLUB_PRESENT` (no `CMD_RECV_PRESENT`; este último no existe en todo GitHub). Se listan sus campos, pero **nadie documenta el aviso de «máximo»** (§2-c).
7. **(f) Lanzadores:** hay patrones nuevos y con licencia MIT que no tocan el archivo hosts: Pocket Relay (Mass Effect 3), OpenWF Enabler (Warframe), yo3-revival (Yu-Gi-Oh! Online de Konami, con «Validar juego» y enlaces de invitación) (§2-f).
8. **(g) Anti-trampas:** lo mejor aplicable es lo que ya hace el propio juego: las sumas `param_check_sum` / `entry_check_sum` que cada cliente manda, el doble informe de resultado y `SET_PLAYDATA_MATCH` (§2-g).
9. **Idea de futuro n.º 1:** «Mercado de fichajes de la liga»: el panel web edita plantillas, el servidor genera el CPK y todos lo reciben por Live Update (§3-1). Idea n.º 2: módulo Sider con `match.stats()` para leer el marcador en vivo y cruzarlo con lo que reporta el servidor (§3-2).
10. **Nadie más** tiene el protocolo de PES 2021 público. Las búsquedas en chino, ruso, español y portugués solo devuelven PESBUL y AzrakHQ otra vez.

---

## 2. Hallazgos por problema

### a) 2 jugadores reales por Tailscale / VPN / LAN (salas de amigos, gamerelay, TURN, puertos P2P, hairpin)

**a.1 `net.md` de AzrakHQ — sección «Same-NAT, LAN, and hairpin»**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/net.md
- Qué da (Confianza: Media, hay pruebas en logs):
  - Si el servidor está en la LAN de un jugador, el STUN ve la dirección de LAN como «reflexiva» (ej. 192.168.1.100 para host y reflexive). El jugador remoto recibe una dirección privada inalcanzable.
  - Truco sin TURN: reescribir la IP reflexiva del jugador de LAN por la IP pública del servidor, conservando el puerto. Solo funciona si el router conserva el puerto de origen.
  - Dos copias en la misma LAN: el enlace directo subió en 0,3 s.
  - Hairpin si coturn está detrás de un router de casa: puede hacer falta; un VPS público lo evita.
  - **El documento NO habla de VPN, Tailscale ni de IP pública compartida.** Nadie lo ha probado públicamente.
- Estrategia de sesión (muy útil): si la lista TURN está vacía, el juego usa solo hole punching y, si no hay camino directo, el partido falla. Con lista TURN no vacía usa P2P + TURN con failover. Esto no depende del tipo de NAT.
- Puertos: STUN UDP 3478; TURN sobre TLS TCP 5349 (`tls_port`); rango de relay sugerido UDP 49160–49200; UDP 7600 solo para el modo SC (y nunca ha recibido un paquete). Los puertos locales del cliente no están documentados.

**a.2 `nettune.md` — bits de `ntl_nat_type` y failover P2P→TURN**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/nettune.md
- Qué da (Confianza: Media):
  - Bits: 0x10 UDP ok; 0x100000 puerto conservado; 0x10000 hairpin (nunca se probó); 0x20000 mapeo variable (simétrico); 0x80 «sin NAT» (solo si hay máscara 0x100).
  - El cliente **no usa** ese valor para decidir P2P o TURN. Se sugiere que el servidor use `&0x20000` como pista de NAT simétrico para preferir TURN (suposición del autor).
  - Failover a TURN: P2P no conectado, buffer crítico >10 s, sin recibir por P2P en 7,5 s, etc. Vuelta a P2P cuando recibe en 7,5 s.
  - Para depurar: la respuesta de GateInfo con `API_STATUS:1`, `LOG_ACTIVE:1` y `PUT_LOG_URL` hace que los clientes suban sus logs NTL. **Esto es oro para saber por qué falla una sala entre dos amigos.** (Confianza: Media; probado por el autor.)
  - `STATUS` de GateInfo debe ser 200.

**a.3 `livetests.md` y `turnlinkup.md` / `turncycle.md` / `ktcdtls.md` (TURN)**
- URL base: https://github.com/AzrakHQ/pes2021-online-server/tree/main/notes/re/functions
- Qué da (Confianza: Media-Alta, probado en vivo 5–6 nov):
  - El TURN solo se mantiene si el atributo SOFTWARE de la respuesta del relay es una «directiva de doble ruta» `HOST:PUERTO,<texto>,0`. Con puerto 0 reinicia cada segundo.
  - El juego abre una segunda pata TURN sobre DTLS 1.2 (UDP) a ese host:puerto; el relay debe atender DTLS además de TLS en 5349.
  - La capa «ktc» de Konami añade 2 bytes a cada datagrama y espera un byte final 0x00. Un servidor DTLS normal descarta todo; el relay propio tiene que quitar y poner esos bytes.
  - Reinicio cada 21,4 s: el juego recrea el socket DTLS cada 10 s si DSCP está activo y cambia el puerto de origen. Arreglo: «itinerancia de sesión» por claves, o `DscpEnable:false` en `proto_opt`.
  - Blob de dirección TURN: 20 bytes: `00`, puerto relayed (big-endian), IPv4 (little-endian), ceros.
  - Orden de etiquetas: solo conecta el lado cuya etiqueta `ip:puerto` ordena más bajo.
  - Un corte de servidor de ~7 s en partida se recupera con `CMD_RECONNECT` (`key_size 16`); uno de ~15 s termina el partido con `APLR_0002`.

**a.4 `coop2.md` — GET_GAMERELAY**
- Con `game_sycom:"P2P"` el juego descarta ip/puerto del relay y no usa UDP 7600. Con cadena vacía lo trata como modo 0 (sin direcciones de pares). (Confianza: Alta, es lo que dice el texto.)

**a.5 Qué falta (honesto)**
- Tailscale con PES 2021: **sin fuente**. Mi deducción (sin verificar): Tailscale da a cada PC una dirección 100.x; si el servidor, el STUN y los dos jugadores están en la misma «tailnet», el STUN verá direcciones 100.x y el tráfico UDP directo entre las 100.x debería pasar (Tailscale ya hace el hole punching por su cuenta). El riesgo es que PES elija otra tarjeta de red para sus candidatos. Prueba concreta: dos PCs, hosts apuntando a la IP 100.x del servidor, `LOG_ACTIVE:1` y mirar qué `host_addr` mandan en `CMD_SEND_IPANDPORT_NTL`.
- Busqué Hamachi / Radmin con PES 2021 en español: solo salen guías de otros juegos (sin verificar para PES).
- El sitio de documentación de Tailscale no se pudo abrir desde aquí.

---

### b) PES League / Torneo: pide otra vez los Términos de Uso

**b.1 `peslg_path.md` de AzrakHQ**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/peslg_path.md
- Qué da (Confianza: Media-Alta, viene del desensamblado del manejador 0x140da8120 y se confirmó con pruebas):
  - Al unirse a una competición, el cliente elige el siguiente paso con esta regla: **`eula_agree`** si `entry_compe` ≠ "YES" **y** `eula_url` no está vacío; si no, **`compe_join`** si es myClub o tipo 2 ó 5; si no, **`team_select`**.
  - Para el lobby 1v1 el camino correcto es `team_select`, con `eula_url:""` y `entry_compe:"NO"`.
  - `CMD_GET_PESLEAGUE_COMPE_INFO` pide `{pid, compe_id}` y devuelve: `compe_id`, `compe_name`, `is_myclub "NO"`, `match_type "PES_LEAGUE_MATCH_1VS1_LOBBY"`, `start_date`, `end_date`, `entry_limit_date`, `match_end_date` (futuro), `entry_user_limit "NO"`, `entry_user_num`, `entry_user_max`, `entry_limit_division "NO"`, `banner_file_info`, `is_exist_overview_info`, `is_exist_participation_right`, `compe_db_id`, `use_ball_id`, `is_e_sports`, `entry_compe`, `team_id`, `clan_info{}`, `compe_regulation{}`, `eula_url`.
  - Trampas: `match_type` desconocido cae en 1VS1 y rompe el camino; `match_end_date` pasada da el error OBMB107; `result` debe ser "NOERR".
- **Qué significa para Frank:** su arreglo con `team_id` puede ser indirecto. La regla real es `eula_url` + `entry_compe`. Si el servidor manda `eula_url` con texto, el aviso es normal y es el comportamiento de Konami: lo correcto es o dejar `eula_url:""` (nuestro servidor no tiene EULA de Konami) o, si quiere mostrar sus propias normas, mandar `entry_compe:"YES"` al jugador que ya aceptó (esto último es deducción mía, sin verificar).

**b.2 `tourney.md`**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/tourney.md
- Qué da (Confianza: Media): flujo completo del torneo 1v1 (`JOIN_PESLEAGUE_COMPE_MODE` → `GET_PESLEAGUE_MAINMENU_INFO` → `COMPE_LIST` → `COMPE_INFO` + `GET_ONLINE_AD_FILE` + `COMPE_TEAM_LIST` → `JOIN_PESLEAGUE_COMPE {compe_id, team_id}` → `1VS1_MAINMENU_INFO`), formatos de `RANKING`, `RESULT_LIST`, `GET_GAMERESULTS {game_res_type "PESLEAGUE"}`; `compe_regulation` debe ir como blob de primer nivel (si no, el partido dura ~2 min); `matching_time` es el fin de ventana, no el tiempo restante; ranking vacío da «Error occurred while loading Ranking Data»; el «Random Selection» no está construido. **Los términos/EULA no se tratan ahí.**
- Búsqueda de código en GitHub de `CMD_GET_PESLEAGUE_COMPE_INFO`: solo aparece en AzrakHQ. Nadie más lo documenta.

---

### c) Regalos: aviso falso de «máximo / límite alcanzado»

- **Hecho clave:** `CMD_RECV_PRESENT` no existe en ningún repo público de GitHub (0 resultados). El comando real de PES 2021 es **`CMD_RECV_MYCLUB_PRESENT`** (y la lista es `CMD_GET_MYCLUB_PRESENTLIST`). Si el servidor de Frank usa otro nombre, es el suyo. (Confianza: Alta.)
- Fuentes: `server/pesserver/schemas.json`, `notes/re/functions/data/commands.json`, `server_errors.tsv` y `tools/result_table.json` de AzrakHQ.
  https://github.com/AzrakHQ/pes2021-online-server/blob/main/server/pesserver/schemas.json
- Campos de la respuesta de `CMD_RECV_MYCLUB_PRESENT`: `body` (NUM), `item`, `msgid`, `name`, `name_jp`, `present_id` (NUM), **`receive_result_list`** (y `receive_result_list_num`), `result`, `rqid`, `title`. Clase de resultado `CmdRecvMyclubPresentResult`, analizador en 0x141cd7f00.
- Error documentado: `ERR_DATABASE` → pantalla `BABK_0013` «Unable to claim present.»; también `ERR_KONAMIID_MAINTENANCE` (BABK_0181).
- `CMD_GET_MYCLUB_PRESENTLIST` devuelve: `present_list` (lista), `present_id`, `present_type` (objeto), `title`, `body`, `name`, `name_jp`, `grade`, `contract_term`, `player_level`, `skill`, `skill_id`, `skill_category`, `team_id`, `entity_id`, `type`.
- `DMM_PLAYERINFO` tiene `is_present_inbox` ("YES"/"NO") para el globito de regalos (`menus.md`).
- **Lo que NO se encontró:** ninguna explicación del aviso de «límite». Mi hipótesis (sin verificar): el aviso nace de lo que ponga cada elemento de `receive_result_list` (resultado por regalo) o de que `receive_result_list_num` no coincida con la lista. Para averiguarlo con seguridad, AzrakHQ tiene `parsebench.md` («cómo ejecutar los analizadores del juego con bytes propios, rastreo de mensajes»):
  https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/parsebench.md (solo apuntado, no leído).
- Confianza general en este apartado: Baja para la causa; Alta para los nombres de campos.

---

### d) Live Update / LiveData (CMD_GET_DLC_FILELIST, LiveDataSetFlow)

**d.1 `live.md` de AzrakHQ — receta completa**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/live.md
- Confianza: **Alta** (verificado en vivo el 5–6 nov: cambio de nombre corto en `Team.bin` visto en el marcador; `PlayerAssignment.bin` movió `entry_check_sum` al valor que predice `pesdb.py`).
- Cómo funciona:
  1. El cliente manda un POST `CMD_GET_DLC_FILELIST` (`file_type=LIVEDATA`) al entrar y otra vez al entrar a un modo online (siempre `pid=0`).
  2. Usa solo el elemento [0] de `dlcfile_list`. Descarga su `url` con un GET HTTP simple, comprueba SHA-256, lo guarda en el slot 0x1b y lo monta como CPK con prioridad 5.
  3. Cada elemento debe ser un **mapa msgpack real** con: `name`, `size` (bytes exactos), `url`, `hash` (64 hex en minúscula), `dlc_version` (cadena que se compara con `atoi`: para 1.01 se manda "101"). Los `*_count` deben coincidir con la longitud de la lista; listas vacías van como `[]`, no `""`.
  4. Es «nueva» solo si `atoi(dlc_version)` > número guardado en `version_wu.bin` (0 si no hay datos guardados).
  5. El CPK debe llevar `common/etc/version/version_wu.bin` (30 bytes: tres campos de 10 bytes ASCII: versión de contenido "1.01", versión de programa "1.01.00", cadena de build) y las tablas cambiadas `common/etc/pesdb/*.bin` (Team.bin y PlayerAssignment.bin confirmadas; Player, PlayerWeekly, Tactics, Coach según la tabla del doc).
  6. El servidor responde el archivo con 200, `Content-Length` exacto y el cuerpo crudo. Límite de transferencia 120 s; un cambio de plantillas de ~1,5 MB cabe.
- **Trampas (cada una cuesta horas):**
  - Hay que **seguir listando la actualización siempre** que el cliente la pida. Si la segunda lista llega vacía, el cliente calcula `CHECK_CHEAT` con datos de base y las sumas dejan de coincidir.
  - El `dlc_version` de `GET_SVRVERSION` debe quedar en "1.00" o menos, si no sale el error SGJC392 («DP version old»).
  - La `url` de descarga debe usar la **misma dirección por la que llegó la petición**; con la dirección pública salió `SGJC_0007_0`.
  - Sin la URL tipo `FLOW` en la lista de URLs el cliente falla en local («New data asset download unsuccessful»). Con listas vacías y "1.00" dice «No new Live Update available» y sigue.
  - Subir `dlc_version` en cada republicación.
  - Sin el `version_wu.bin`, se descarga de nuevo en cada login.
  - Si `is_revision_check` está en "NO", una diferencia de sumas no bloquea. Con "YES" sí.
- Herramientas del repo que se pueden estudiar: `tools/mklivedata.py`, `tools/pesmod/cpkwrite.py`, `server/pesserver/liveupdate.py`, `cpkread.py`. Licencia MIT.
- Qué falta: contenido «Variation» y «Extra» (no hay datos de prueba), el significado de los campos 2 y 3 de `version_wu.bin`.
- Sobre forma de servirlo desde **nuestra web** (Frank dijo «desde nuestro servidor o web»): el cliente solo necesita un GET HTTP plano con la `url` que le demos; la `url` puede apuntar a cualquier host al que el cliente llegue. Pero ojo con la regla de «misma dirección por la que llegó» (sin verificar si un dominio distinto también falla; hay que probarlo). Y es HTTP, no HTTPS.

**d.2 Dato extra de otro repo:** `tstirrat/peshead` (Apache-2.0) tiene una función con `CMD_GET_DLC_FILELIST` en `functions/konami.ts`, pero es de PES 2018 y el archivo no se pudo leer (404 por la rama). Útil solo como pista de que PES 2018 usa el mismo comando. https://github.com/tstirrat/peshead (Confianza: Baja).

---

### e) Co-op 2v2 online (Team Play)

- **`coop2.md` de AzrakHQ** — https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/coop2.md (Confianza: Media-Alta).
  - Por qué se paraba el co-op tras el emparejamiento: (1) `GET_GAMERELAY` con `game_sycom` vacío → el juego lo trata como modo 0 y no rellena pares (error MXNV_0010). Arreglo del servidor: responder `game_sycom:"P2P"`. (2) El co-op arma la lista de pares pero nunca la pasa a la capa NTL (MXNV_0013). Eso no lo arregla el servidor: AzrakHQ lo corrige con un parche del juego («mod 0.5.0»). Además hacen falta `use_team_id` reales (el marcador 262143 en ambos lados bloquea la carga).
  - **2v2 con P2P no es posible:** la capa NAT admite como máximo 2 pares y 2v2 necesita 3. Haría falta el game relay (`game_sycom:"SC"` + ip:puerto) con el protocolo CsClient de Konami, **no descifrado** (UDP 7600 nunca ha recibido un paquete en ningún log).
  - Pueden aparecer más problemas con el vector TURN de «exactamente un elemento» en grupos de más de 1v1 (suposición del autor).
- **`livetests.md`**: el co-op de 2 jugadores se jugó hasta el final del partido con nivel y puntos en pantalla. Hay que devolver `SYNC_PROGRESS` a su emisor.
- Contexto ya conocido (PESBUL): Team Play 11v11 llega a cargar GameRelay pero no a «Side Select»; «casi el 90 % de los modos dependen de GameRelay».
- **Nadie lo ha resuelto en público.** Para 2v2 la única vía sería descifrar el protocolo SC: capturar con el servidor devolviendo `"SC"` y mirar qué manda el cliente al puerto del relay (el propio AzrakHQ nunca lo hizo; sin verificar si PESBUL lo consiguió).

---

### f) Lanzadores / DLL en vez del archivo hosts

Ya sabíamos: Sider `netblock.lua`, tpp-mod, el DLL de AzrakHQ. Nuevo:

| Proyecto | URL | Qué hace y qué nos da | Licencia | Confianza |
|---|---|---|---|---|
| **Pocket Relay EmbeddedClient** (Mass Effect 3) | https://github.com/PocketRelay/EmbeddedClient | DLL (`binkw32.dll` en Rust, 32 bits) que **parchea la resolución de nombres**, así no hace falta ser administrador ni tocar `hosts`. Al arrancar sale una ventana para escribir la URL del servidor; si se ignora, el juego va a los servidores oficiales. Se puede desconectar del servidor privado con el juego abierto. | MIT | Alta (README); el cómo exacto no se explica |
| **OpenWF Enabler** (Warframe) | https://github.com/mskimi7/openopenwf | Lanzador que solo redirige peticiones a un servidor OpenWF. Lee `server_host`, `http_port`, `https_port` y `disable_nrs_connection` (se salta la prueba P2P que dispara el aviso del firewall). Solo hace redirección, nada de parches de contenido. | no comprobada | Alta |
| **yo3-revival** (Yu-Gi-Oh! ONLINE 3 de **Konami**, servidor en Rust) | https://github.com/usersina/yo3-revival y `docs/launcher.md` | El lanzador carga una **DLL de conexión** al darle a Start, sin cambiar el .exe ni el .ini. Tiene botón **«Validate game»** que comprueba la versión/firmas del juego antes de arrancar. La DLL asigna los puertos locales del juego. Las conexiones van con TLS y **guardan la identidad del servidor** (como una huella) para verificarla en las siguientes. Tiene **invitaciones privadas** (dirección + clave) para unirse. | MIT | Alta (README y launcher.md); el mecanismo interno no se describe |
| **Chaos Launcher** (otro Yu-Gi-Oh! Online) | https://github.com/I-TD-I9/Chaos-Launcher | Lanzador .NET que «nunca cambia la carpeta del juego»; incluye una librería de terceros y **comprueba su hash fijado** al bajarla. | GPL-3.0 | Media |
| **RetroDNS** | https://github.com/PSRewired/RetroDNS | Proxy DNS pequeño: un archivo de dominios `nombre=ip://1.2.3.4` o `=dns://8.8.8.8` para el resto. Sirve si un día se quiere redirigir por DNS en vez de hosts (una consola o PC apunta su DNS al PC). | sin licencia visible | Media |
| **OpenSpy Core / BF2Hub / PRMasterServer** (GameSpy) | https://github.com/openspy/openspy-core · https://github.com/novice-rb/PRMasterServer | Redirigen con archivo hosts o con parches/herramientas («BF2 Migrator»). Confirman que el hosts es el método normal también en proyectos grandes. | varias | Baja-Media (resumen del buscador) |
| **Wiimmfi / WiiLink WFC** | https://github.com/WiiLink24/wfc-server | Servidor de reemplazo de Nintendo WFC (AGPL-3.0, Go + PostgreSQL). Los clientes llegan por **DNS propio**; hay DNS público y quien no puede usarlo corre su propio DNS en el PC. El README no habla de baneos ni anti-trampas. | AGPL-3.0 | Alta para lo leído |
| **CnC-Online**, **servidores de osu! (Hook)**, **Internet Games Server fork** (gancho a `GetAddrInfoW`) | (solo por el buscador; sin verificar) | Mismo patrón: DLL proxy + gancho a funciones de red, sin hosts. | varias | Baja |

- Qué podría construir Frank (lo que ya hace su «Core» pero con mejoras):
  1. **Ventana/campo «Servidor»** como Pocket Relay: si se ignora, el juego va a oficial (y falla) — evita dejar el hosts sucio.
  2. **«Validar juego»** como yo3-revival: comprobar SHA-256 del exe (1.01.00), del `DpFileList.bin`, de los DLC y del `sider.ini` antes de arrancar, y explicar en español qué falta.
  3. **Invitación de liga**: un código o enlace con dirección del servidor + clave pública del certificado (fijar la huella, como hace yo3-revival, para la parte HTTPS de `ntl*`).
  4. **Perfiles de guardado por servidor** (idea de tpp-mod, ya conocida) para no mezclar el save de Phoenix con otro.

---

### g) Anti-trampas y validación de resultados

**g.1 `datacheck.md` — comprobación de DATAPACK entre jugadores**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/datacheck.md
- Confianza: Media (análisis estático + algunas pruebas en vivo).
- Cada cliente manda `CmdRevisionCheck` por P2P con `param_check_sum` (estadísticas y fichajes) y `entry_check_sum` (plantillas y dorsales). Se comparan exactamente. Nombres, equipaciones y caras NO cambian las sumas.
- `is_revision_check` viene de `GET_SERVER_ENV`; solo la cadena "YES" lo activa; si falta la clave, queda activo.
- Plazo: 60 s (`MXNV_0015`); error de datos `MXNV_0017`; en vivo: `GNCX_0004` (60 s) y `GNCX_0006` (sumas distintas).
- Un servidor puede recibir ambas sumas en `CHECK_CHEAT` y **negar el partido antes de empezar**, no cuando ya falla.
- Limitación importante (el propio autor): las sumas son lo que dice el cliente; no hay nada que las autentique. No sirve como anti-trampas único. Con `is_revision_check:"NO"` una prueba de 16 min con datos distintos no mostró desincronización.

**g.2 `results.md` — doble informe de resultado**
- URL: https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/results.md
- Confianza: Media. Regla recomendada: guardar los dos `CMD_SEND_GAME_RESULT`; si ambos dicen END con el mismo marcador, es definitivo; si uno reporta desconexión y el otro END con el mismo marcador, vale el END; si los marcadores difieren, partido «en disputa». El que abandona pierde 3-0 (o conserva el marcador si ya ganaba). `is_cheat` y el motivo `CHEAT` existen en el informe de fin de partido.
- `SET_PLAYDATA_MATCH` (HTTP): JSON con `gamemode`, `match_time`, `win_lose`, `scorer`, `team[]` con `player_id`, `play_time`, `rating`, `player_overall`. Es la mejor fuente para goleadores y notas, y sirve de segundo informe para cruzar marcador.

**g.3 Reglas de otros servidores** (del informe de ayer, para completar): tope de 3 partidos puntuados por rival y día y mismo hash de archivos (OpenEleven); «desconexión = derrota» configurable, baneo de IPs, contador de desconexiones (fiveserver).

**g.4 Sider + anti-cheat**
- Existe un post «efootball pes 2021 anti cheat sider lua» en pesmodding.com (2026/01) — **no se pudo leer** (solo desde la PC de Frank). Qué hace es sin verificar.

**g.5 Ideas generales** (no hay fuente de servidores de fans que lo cubra; las búsquedas de «anti-farming» no devolvieron nada): rendimientos decrecientes por rival repetido, límite por día, ignorar resultados entre cuentas que juegan casi solo entre sí. Es diseño propio.

---

## 3. Ideas para el futuro (más allá del online)

### 3-1. «Mercado de fichajes de la liga» por Live Update
- Fuente: receta de `live.md` (https://github.com/AzrakHQ/pes2021-online-server/blob/main/notes/re/functions/live.md) + herramientas de guardado: https://github.com/kickoffsage/pes2021-transfer-tool (MIT, edita equipo-jugador en el guardado y exporta CSV; en desarrollo; usa pesXdecrypter con su propia licencia) y https://github.com/felipemikeias/pes21-ml-editor (MIT o Apache-2.0; formato de guardado en `SAVE_FORMAT.md`; hace copia de seguridad antes de escribir).
- Propuesta: en la web de la liga, un panel de admin donde se eligen traspasos/forma de jugadores → el servidor genera el CPK de `Team.bin` / `PlayerAssignment.bin` (+ `version_wu.bin` con la versión subida) → todos lo reciben al entrar. Misma base de datos para todos = menos `C_NAJM_0022` y sumas iguales. Prioridad: Alta (resuelve a la vez d y la regla «misma base de datos»).
- Aviso: son datos del juego; el cumplimiento legal es de Frank. AzrakHQ avisa: «nunca publicar datos del juego».

### 3-2. Módulo Sider «Telemetría Phoenix» (marcador en vivo)
- Fuente: doc de scripting de Sider 7 https://github.com/pes-modding/sider7/blob/master/doc/scripting.html y ejemplo `modules/mstats.lua` https://github.com/pes-modding/sider7/blob/master/modules/mstats.lua
- `match.stats()` devuelve `home_score`, `away_score`, `pk_home_score`, `pk_away_score`, `period`, `clock_minutes`, `clock_seconds`, `added_minutes`. Se activa con `match-stats.enabled = 1` en `sider.ini` y está «experimental». Eventos útiles: `set_teams`, `set_match_time`, `set_match_settings`, `context_reset`, `overlay_on`. No hay evento de «resultado final» ni acceso a notas de jugadores (no documentado). (Confianza: Alta en lo leído.)
- Propuesta: un módulo que, durante partidas online de liga, muestre en un overlay «PHOENIX · Jornada X · Rival» y escriba un archivo JSON con equipos, marcador y reloj. Sirve para: (1) cruzar contra lo que reporta el servidor (anti-trampas suave), (2) mostrar marcador en un stream. Cuidado: no probado que Lua pueda escribir archivos (la doc muestra `fs` solo para carpetas/búsqueda; sin verificar `io.open`). Prioridad: Media.

### 3-3. Plataforma de liga: tabla, desempates y repeticiones
- Fuente: https://github.com/Aric-byte/pes-league (MIT; Spring Boot + MySQL, en chino; hecha para un juego móvil pero la lógica sirve).
- Qué copiar (conceptos): puntos 3/1/0; desempate por diferencia de goles, goles a favor y resultado directo; empate en eliminatoria exige penales distintos; el administrador puede corregir un marcador y se recalcula todo; los jugadores suben su repetición (MP4, comprimida con FFmpeg a 1280 px) y la web la borra tras un plazo; ranking de goleadores y asistencias.
- Propuesta: añadir a la web de Phoenix «Reportar disputa» (subir clip de 20 s de la repetición), ranking de goleadores sacado de `SET_PLAYDATA_MATCH`, y un botón de admin «corregir marcador». En Supabase, los clips van en Storage con borrado automático (plan FREE tiene poco espacio: poner tope).

### 3-4. Cuadros de torneo listos
- Fuente: brackets-manager.js (Drarig29, MIT) y brackets-viewer.js: https://www.npmjs.com/package/brackets-manager · https://cdn.jsdelivr.net/npm/brackets-viewer@1.9.0/README.md
- Soporta liga todos contra todos, eliminatoria simple/doble y etapas combinadas (grupos → eliminatoria). Es solo datos/lógica; el viewer pinta el cuadro. (Confianza: Media, resumen del buscador; leer el README antes de elegir versión.)
- Propuesta: usar la parte «manager» para generar los emparejamientos de la Galaxy League y el viewer en la página del torneo (cabe en JS vanilla). Prioridad: Media.

### 3-5. ELO mejor: Glicko-2 + anti-«farmeo»
- Fuente: https://github.com/mmai/glicko2js (MIT) y https://pypi.org/project/glicko2-py/ (sin dependencias). Resumen del buscador: Elo o Glicko-2 para 1v1; TrueSkill para equipos (útil para co-op 2v2).
- Propuesta: nuestro ELO guarda además la «incertidumbre» del jugador (Glicko-2), así los nuevos suben rápido y los inactivos pierden confianza. Añadir el tope de 3 partidos puntuados por rival y día (idea de OpenEleven) y rendimientos decrecientes por rival repetido.

### 3-6. Estadísticas de partido en la web (estilo PESBUL)
- Fuente: `results.md` (SET_PLAYDATA_MATCH) y la web de PESBUL (ya conocida).
- Propuesta: guardar el JSON de `SET_PLAYDATA_MATCH` tal cual (Supabase, tabla `partidos_json`) y construir: «Jugador de la jornada» (mejor `rating`), goleadores, minutos jugados, historial por rival. Hoy lo perdemos porque se responde NOERR y se descarta (informe de ayer, prioridad A).

### 3-7. Probar sin tener dos PCs: clientes sin pantalla y en Linux
- Fuentes: https://github.com/eloymelo/pes2021-sider-headless (instrucciones, sin binarios; Proton Experimental en Debian, Xvfb + VNC por túnel SSH, Sider arranca primero con `start.game = "PES2021.exe"`) y https://github.com/agentkyo/spfl26-pes2021-linux (SPFL 2026 en Linux con GE-Proton11-3; instalar en ruta `C:\`, `WINEDLLOVERRIDES="lsteamclient=d"`; cerrar Steam por el mando).
- Propuesta: un PC de pruebas con dos copias del juego (cada una necesita su propia licencia) para repetir «sala de amigos» después de cada cambio del servidor, como hace AzrakHQ (`tools/testserver`). Son notas de terceros sin verificar para red; no dicen nada de online.

### 3-8. Registro de peticiones del servidor, copiado de tpp-mod
- Fuente: https://github.com/alicealys/tpp-mod (ya conocido) → `net_server_logging` vuelca cada petición y respuesta como JSON; reintenta hasta 3 errores antes de soltar la sesión.
- Propuesta: nuestro servidor ya lo ve todo; añadir un «modo captura» que guarde cada petición/respuesta de un jugador en un archivo con su pid (sin datos sensibles) para analizar fallos de amigos sin pedirles nada. Además combinarlo con `LOG_ACTIVE:1` de GateInfo (§2-a).

### 3-9. Bot de Discord de la liga (el usuario lo tiene DIFERIDO; solo guardar la idea)
- Fuentes: https://github.com/williamsantosa/Not-Clash (Python, ELO + SQLite), https://github.com/lekro/elosensei (sin mantenimiento), https://github.com/fedebenelli/eloBOT (en desarrollo). Ninguno es de PES.
- Propuesta para cuando se retome: slash-commands `/resultado`, `/tabla` y `/proximo` que lean Supabase. Empezar solo con lectura.

### 3-10. Módulos Sider de competición (solo desde la PC de Frank, sin leer)
- Fuentes por el buscador (no verificadas): «Competition-Server» de Gerlamp (ascensos y descensos de liga/copa), «Tournament StaffServer», «Scoreboard Server» de zlac (marcadores). Todos en pesmodding/instructions.pesmodding.com.
- Propuesta: ver si alguno sirve para un modo «Liga offline» con el calendario de la web. Hay que leerlos en la PC antes de decidir.

### 3-11. Relay propio: librerías
- Fuentes: https://github.com/pion/turn (Go, para empotrar un TURN), https://github.com/paullouisageneau/libjuice (C, ICE/STUN/TURN, MPL-2.0), coturn (servidor completo; es lo que AzrakHQ probó en vivo).
- Propuesta: no sustituir coturn; pero si el relay propio de Python da problemas con el «trozo extra de 2 bytes» de Konami (§2-a.3), mirar pion/turn como código de referencia de DTLS sobre TURN. (Sin verificar que ninguno soporte lo especial de Konami.)

### 3-12. Datos de ligas nuevas
- Fuente: https://github.com/Matozanato/FootballLife-new-leagues (MIT, Football Life 2026/2027 sobre motor PES 2021: nuevas ligas, clubes y países en Master League). Sin leer a fondo. Idea: si se quiere una «base de datos de la liga» propia, ver cómo organizan los datos.

---

## 4. Sitios que solo se pueden leer desde la PC de Frank

(Esta sesión no puede abrirlos. Lo de aquí sale del buscador.)

- **evoweb.uk**, hilos:
  - PESBUL, 106068, páginas 2–10: https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/ (GameRelay, `ulist` en BIN, DISCONN_P2P).
  - **«Building PES 2021 Team Play Lobby online server», hilo 100632 — NUEVO:** https://evoweb.uk/threads/building-pes-2021-team-play-lobby-online-server.100632/ (con mensajes sueltos /post-4133824 y /post-4131773). Es el origen del «WordPress Lobby + PES P2P» y de Team Play: muy relevante para el problema (e).
  - «Will there ever be something like Fiveserver/Sixserver for a modern PES title?»: https://evoweb.uk/threads/will-there-ever-be-something-like-fiveserver-sixserver-for-a-modern-pes-title.87617
  - «I want to play PES 2017 online with my friends»: https://evoweb.uk/threads/i-want-to-play-pes-2017-online-with-my-friends-what-should-i-do.104645/ (posible mención de VPN/Radmin).
  - ThirteenServer (PES 2013): hilos 106472 y 107221 (ya conocidos).
- **pesmodding.com**: «how to enable pes 2021 online mode» (2026/07) https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html ; «efootball pes 2021 anti cheat sider lua» (2026/01) https://www.pesmodding.com/2026/01/efootball-pes-2021-anti-cheat-sider-lua.html ; «competition server v21», «tournament staffserver», «player of match server», «dynamic broadcast cameras». También los «Scoreboard Server» en instructions.pesmodding.com.
- **pes2021.pesbul.eu**, **web.archive.org** (ya conocidos).
- **tailscale.com** (docs de tipos de conexión y NAT): útil para el apartado (a). Debería comprobarse en una PC con internet normal. También **generationamiga.com** (artículo del 19 jul 2026 sobre servidores de FIFA 13/14/17 con RPCS3; solo el titular, sin verificar) y **emulation.gametechwiki.com** (lista de proyectos de revival).
- **soccergaming.com** (hilos «FIFA World Revival Project» y «CG File Server 16 Python port»): no probado; el buscador los devolvió.
- **mapote.com/doc/sider/** (doc oficial de Sider): no probado.

---

## 5. Lo que NO sirve o hay que evitar

- **Cheat tables y trampas:** `xAranaktu/PES-2021-Cheat-Table` (66 estrellas; editar memoria del juego), `Deniseroipes/efootball-pes-2021-hack-coins-mod` (estafa de «monedas gratis»). No usar.
- **Bots que juegan solos o macros:** `rein-baremetal/PitchBot` (juega eFootball leyendo el radar), `leguims/PS4Macro.PES2018Lite`. Es automatizar partidas: contrario al juego limpio de una liga.
- **Descifrado de paquetes de eFootball móvil:** `nyandev-55/efbmobiledecrypt`. No relevante a PES 2021 PC y roza la ingeniería de un servicio vivo.
- **Mods de eFootball iOS con IPA desencriptada:** `hiuzz24/pes2027-mod` (ya listado ayer).
- **CreamAPI / Goldberg / cambiar el exe / CPY:** cualquier guía que lo pida (ya listado ayer). `razwanrzgar81-crypto/Pes2018-realive` sigue sospechoso.
- **Spam:** repos tipo `pxvr-official/1`, `hiteshsuthar01/OK-` y páginas `edx.ipeindia.org` que salieron en búsquedas de PES: basura.
- **Imagen Docker de PESBUL** (binario cerrado): no ejecutar.
- **Copiar código con licencia fuerte** sin querer publicar el nuestro: OpenEleven (AGPL-3.0), WiiLink WFC (AGPL-3.0), tpp-mod y Chaos Launcher (GPL-3.0). Leerlos como guía no obliga a nada; copiarlos sí. Licencias permisivas: AzrakHQ (MIT), yo3-revival (MIT), Pocket Relay (MIT), fiveserver (BSD-2), brackets-manager (MIT), glicko2js (MIT).
- **No tocar:** `denuvo.cpp` de tpp-mod y los exploits (`mgo3-rce`) ya apuntados ayer.
- **Mensaje que da mal el buscador:** que «los servidores oficiales siguen activos» o guías de Tecnobits/Tailscale genéricas para PES 2021: no son fuente. Tampoco hay evidencia de Hamachi/Radmin/Tailscale con PES 2021; no asumir que funciona.
- **Aviso sobre el hallazgo (b):** no quitar el aviso de términos de uso «a ciegas». Lo correcto es decidir si Phoenix tiene sus propias normas; si las tiene, mostrarlas una vez y marcar `entry_compe:"YES"`.

---

## Apéndice — ¿qué buscas en GitHub y qué da cero resultados?

Búsqueda de código (GitHub, exacto) hecha hoy:
- `CMD_GET_PESLEAGUE_COMPE_INFO` → solo AzrakHQ.
- `CMD_GET_DLC_FILELIST` → AzrakHQ y `tstirrat/peshead` (PES 2018).
- `LiveDataSetFlow` → solo AzrakHQ.
- `CMD_SET_GAMERELAY_CHECK_RES` → solo AzrakHQ.
- `CMD_RECV_PRESENT`, `CMD_GET_PRESENT_LIST`, `CMD_RECEIVE_PRESENT`, `CMD_GET_GAMERELAY_INFO` → **0 resultados**.
- `CMD_RECV_MYCLUB_PRESENT` → solo AzrakHQ.

Repos de PES 2021 nuevos vistos (no listados antes): `agentkyo/spfl26-pes2021-linux`, `eloymelo/pes2021-sider-headless`, `Gustavohps10/PES2021-scripts` (Lua, MIT: nombres de jugador, música de gol, tono de llamada; poco que aprender), `kickoffsage/pes2021-transfer-tool`, `felipemikeias/pes21-ml-editor`, `martha889/pes-ml-sim` (simula la Liga Máster), `Matozanato/FootballLife-new-leagues`, `Jonas29/PES2021_Sider_Proton_LOG`. Y de Konami fuera de PES: `usersina/yo3-revival`, `I-TD-I9/Chaos-Launcher`, `ShuAkirasora/tokimeki-memorial-online-revival` (Apache-2.0, servidor local de Tokimeki Memorial ONLINE; sin leer).
