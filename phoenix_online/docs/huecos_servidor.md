# Huecos del servidor propio — lectura Ghidra de PES2021.exe (2026-10-28, solo lectura)
Base 0x140000000, programa PES2021_recortado.exe (1.07.02, P). C = confirmado en el código; P = deducido. Cada dato lleva la función del exe que lo lee.

## Cómo son las cargas (C)
- Cada comando va como texto tipo JSON con claves: el cliente lo recorre con un analizador (fn 141c190a0) y compara claves por nombre. Tipos de valor del analizador: 2 = entero, 3/4 = número, 5 = cadena, 9 = arreglo (C, por las comprobaciones `==2/4/5/9` en las funciones de abajo; el significado exacto de cada código es P).
- Cada comando tiene una pareja de funciones: constructor de la petición (clave `msgid` + campos de petición) y lector de la respuesta (`msgid`, `rqid`, `result` + campos). Se ubican justo después del nombre CMD_ en .rdata.
- C `result` se lee como CADENA (se copia con fn 14022e4c0), no como número.

## 1) CMD_GET_SVRVERSION (lector de respuesta fn 141d4af80; petición fn 141d4a7a0 solo con msgid)
Campos que lee (C), todos como CADENA (copiados a std::string del objeto en +0x98…+0x1b8): `msgid`, `result`, `version` (+0xb8), `patch_version` (+0xd8), `dlc_version` (+0xf8), `eula_version` (+0x118), `eula_myclub_version` (+0x138), `livedata_version` (+0x158), `eula_privacy_policy_version` (+0x178), `eula_privacy_notice_version`, `eula_privacy_policy_california_version`.
- Con qué valor se comparan: NO está resuelto. C que los errores existen: ERR_DIFFERENT_PATCHVERSION (fn 140fdc040 y 140fe0b60), ERR_DIFFERENT_EXTRADATA_VERSION, ERR_DIFFERENT_LIVEDATA_VERSION (tabla fn 14021fa30). P: patch_version se compara con la versión del parche del propio exe (cadena 1.07.02 / "PATCH10100") y livedata_version con la del live update; el valor exacto habría que sacarlo del que mande el servidor original o de la fn que lee +0xd8 (pendiente).
- La petición del lobby (fn 141d47be0) manda `svrtype`, `client`, `timeout`, `patch_version`, `dlc_version` (C): el cliente envía su propia patch/dlc al pedir CMD_GET_SVRLIST.
## 2) Tipos de URL obligatorios (NO resuelto del todo)
- C Lista completa de los 53 tipos en cmd_lista.txt (tabla fn 14021ee70). C ERR_CLIENT_SENDFAIL lo emiten fn 140fdebf0 (CmdGetSvrList) y fn 141173170/1411740b0 (conexión), no una búsqueda de URL concreta.
- P Imprescindibles para iniciar sesión (los usa la secuencia de ConnectGate): el JSON de CMD_GET_URLLIST (lector fn 141d4ce30: solo claves msgid/result, las URL vienen en claves dinámicas) debe traer al menos los tipos que usa el flujo: GET_TURN_SERVER_LIST, GET_GAMERELAY, TOPINFO, BANNER, EULA_PES, AUTH_STEAM, ONLINEID. Falta confirmar uno a uno cuáles dan error si faltan (siguiente paso: xrefs a 0x1437F3A20..0x1437F3BC0).
## 3) Antes de CMD_LOGIN y orden (C, fn 140fddef0, OnlineLoginTaskConnectGate.cpp:0x1a9)
1. CmdGetUrlList → CmdGetSvrVersion → CmdGetSvrTime → CmdGetEulaNationList (en ese orden, creados en un bucle; si falta alguno, se marca error 0xb y se corta).
2. (P) CmdGetEulaNationCode / CmdGetEulaAreaListSaveData.
3. CmdGetSvrList (fn 140fdebf0; ERR_CLIENT_SENDFAIL si falla).
4. CmdGetChallengecodeAccount existe (fn 140fdbb80; CMD_GET_CHALLENGECODE, lector fn 141d287e0: `msgid`, `result`, `challenge_code`) — C que existe; P que va justo ANTES de CmdLoginAccount (fn 140fdc040).
5. CMD_LOGIN: petición con `msgid` y `np_account_id` (fn 141d57490), respuesta `msgid`/`result` (fn 141d58420). Errores que mapea CmdLoginAccount (C): ERR_LOGIN_FAILED, ERR_LOBBYUSERMAX, ERR_DIFFERENT_PATCHVERSION, ERR_SERVERNOTREADY.
6. CMD_SEND_PSID: petición con `psid_high` y `psid_low` (fn 141d655f0), respuesta `result` (fn 141d65d10) — C.
7. Después: CmdGetPlayerData → CmdCreatePlayer → CmdSetCurrentPlayer → CmdLoginMenu (nombres confirmados en el exe; el orden es el que publica PESBUL, P).
- Steam: C existen los tipos de URL AUTH_STEAM, STEAM_STORE_*, AUTH_XSTS. P: el cliente autentica con Steam contra la URL AUTH_STEAM y el resultado se manda en CMD_LOGIN (np_account_id); no vi un paso Steam dentro de ConnectGate. Falta leer quién llama a la URL AUTH_STEAM.
## 4) pname y date
- C CMD_GET_PLAYER_DATA, lector fn 141d3f450: claves `msgid`, `rqid`, `result`, `has_data`, `pid`, `pname` (OBJETO con claves internas `name`, `xuid_one`, `psn_user_id`, `account_id`, `online_id`, `is_valid_cache`, `is_deleted`), y `account_id` a nivel superior. (El "bin pname" es en realidad ese objeto; P: puede venir también como lista de ellos.)
- C CMD_GET_SVRTIME, lector fn 141d49930: claves `msgid`, `rqid`, `result`, `date`. `date` se lee como NÚMERO y se guarda como entero de 32 bits en +0xb8 (cast de double). P: son segundos Unix (época); la fn 14184ffd0 suma 0x7E90 (=32400 s = 9 h, JST) para formar la hora local, lo que apoya la época Unix.
## 5) CMD_WATCH_ENTRY_GAME y WATCH_DECIDE_GAMEENV
- C CMD_WATCH_ENTRY_GAME: constructor fn 141d0e620; lector fn 141d0eeb0: `msgid`, `rqid` (entero), `result`, `entry_count` (entero), `entrygame` (arreglo; cada elemento con `entryNum` y `gameNum`, números).
- C CMD_WATCH_DECIDE_GAMEENV NO existe como texto CMD_ en el exe (ni _FREE). Solo existen las clases del lobby `MOMLobbyCmdWatchDecideGameenv` (fn 141128150/1411281e4, "Online/MOMLobby/…") y el comando `CMD_SET_GAMEENV`. P: se resuelve dentro del cliente como mensaje del lobby, no como comando del servidor; pendiente decompilar fn 141128150.
## 6) Valores de «result» de rechazo (C)
- Tabla fn 14021fa30: 189 nombres. El éxito es el texto `NOERR`; los rechazos son los nombres `ERR_*`. P: el servidor manda ese texto en `result` (el cliente lo convierte con fn 1403d2a20 a su índice). Ejemplos: sin cuenta/login = ERR_LOGIN_FAILED; sala llena = ERR_ROOMISFULL; sala inexistente = ERR_ROOMNOTFOUND; estado de sala distinto = ERR_ROOMSTATNOTMATCH; servidor no listo = ERR_SERVERNOTREADY; lobby lleno = ERR_LOBBYUSERMAX; versión distinta = ERR_DIFFERENT_PATCHVERSION; modo bloqueado = ERR_MODE_BLOCKED; nombre repetido = ERR_ALREADYEXISTS; letras inválidas = ERR_INVALIDLETTER; palabra prohibida = ERR_NGWORDINPNAME; trampa = ERR_CHEAT.
- Índices (posición en la tabla, P, 0 = éxito):
0 NOERR
1 ERR_CLIENT_CANTCONNECT
2 ERR_CLIENT_TIMEOUT
3 ERR_CLIENT_SVRADDR
4 ERR_CLIENT_SVRDISCONNECT
5 ERR_CLIENT_SENDFAIL
6 ERR_CLIENT_NONSUPPORT
7 ERR_CLIENT_OPTFORMAT
8 ERR_CLIENT_SVRFORMAT
9 ERR_CLIENT_UNKNOWN
10 ERR_OPTION_OUTOFRANGE
11 ERR_RESULT_OUTOFRANGE
12 ERR_ONLINEPASS
13 ERR_DATABASE
14 ERR_INVALIDARG
15 ERR_READMSGFAIL
16 ERR_MSGFORMAT
17 ERR_NGWORD
18 ERR_TOOMANYLIST
19 ERR_CANTADDGUEST
20 ERR_TOOMANYGUEST
21 ERR_NOGUEST
22 ERR_FORBIDDEN
23 ERR_NOTREGISTERED
24 ERR_ALREADYREGISTERED
25 ERR_DATANOTFOUND
26 ERR_INTERNAL
27 ERR_EMPTY
28 ERR_SERVERNOTREADY
29 ERR_NGWORDINPNAME
30 ERR_LOGIN_FAILED
31 ERR_INVALID_ACCOUNT
32 ERR_ALREADYLOGGEDIN
33 ERR_DEFCLIENTVER
34 ERR_PARAMCHECK_FAILED
35 ERR_ALREADYEXISTS
36 ERR_INVALIDLETTER
37 ERR_NOPLAYER
38 ERR_DIFFERENT_PATCHVERSION
39 ERR_INVALID_TICKET
40 ERR_LOBBYUSERMAX
41 ERR_LOBBYNOTFOUND
42 ERR_TARGETISNOTLOGIN
43 ERR_TARGETINGAME
44 ERR_CHATENABLE
45 ERR_ISNOTGM
46 ERR_BLOCKNOTEXIST
47 ERR_BLOCKISFULL
48 ERR_NOBLOCKBELONG
49 ERR_DIVISIONREFUSED
50 ERR_NGWORDINRNAME
51 ERR_ROOMTOOMANY
52 ERR_ROOMISFULL
53 ERR_ROOMCREATELIMIT
54 ERR_ALREADYINROOM
55 ERR_ROOMNOTFOUND
56 ERR_NOTINROOM
57 ERR_ROOMSTATNOTMATCH
58 ERR_NOTROOMOWNER
59 ERR_NOTGAMEHOST
60 ERR_TIMEOUT
61 ERR_PASSWDINCORRECT
62 ERR_PASSWDEMPTY
63 ERR_NOTFOUNDCLIENT
64 ERR_PLAYERISNOTENOUGH
65 ERR_PLAYERISNOTGAMER
66 ERR_PLAYERISWATCHER
67 ERR_PROHIBITEDTIME
68 ERR_NGWORDININTRO
69 ERR_INVALIDDATE
70 ERR_DIFFDCVERSION
71 ERR_RATING_NOTMET
72 ERR_GAMERELAY_NOTENOUGH
73 ERR_CHEAT
74 ERR_NOTJOINGAMEUSER
75 ERR_EXISTINGAMEUSER
76 ERR_DIFFROOMUSER
77 ERR_SELECTOWN
78 ERR_SELECTROOMOWNER
79 ERR_JOINKICKEDROOM
80 ERR_PLAYERNOTEXIST
81 ERR_TOOMANYRECORDS
82 ERR_NOTBEGINNERLEVEL
83 ERR_ALREADY_ENDGAME
84 ERR_GROUPUSERMAX
85 ERR_TOOMANYMEMBERS
86 ERR_COMPE_JOIN_SANCTION
87 ERR_COMPE_FULL
88 ERR_COMPE_NOTFOUND
89 ERR_COMPE_NOTENTRY
90 ERR_COMPE_MATCH_NOTFOUND
91 ERR_COMPE_BEFORE_MATCHTIME
92 ERR_COMPE_AFTER_MATCHTIME
93 ERR_COMPE_BEFORE_ENTRYTIME
94 ERR_COMPE_AFTER_ENTRYTIME
95 ERR_TEAM_NOTFOUND
96 ERR_REFUSED
97 ERR_PING_REFUSAL
98 ERR_PING_FAILED
99 ERR_STUN_PORTRANGE
100 ERR_STUN_NETWORKSETTING
101 ERR_STUN_UDPBLOCKED
102 ERR_STUN_SYMMETRICNAT
103 ERR_STUN_ALREADYUSEDPORT
104 ERR_STUN_UPNP_PORTOPEN
105 ERR_STUN_UPNP_BIND
106 ERR_QMATCH_STARTMATCHING
107 ERR_QMATCH_NOTICE
108 ERR_QMATCH_CANCELED
109 ERR_MATCHING_FAILED
110 ERR_MATCHING_TIMEOUT
111 ERR_MATCHING_CANCELED
112 ERR_NOTFOUND
113 ERR_DATASIZEOVER
114 ERR_MD5
115 ERR_NOTPERMISSION
116 ERR_COMPE_JOIN_MANNER
117 ERR_COMPE_JOIN_RATING
118 ERR_COMPE_JOIN_DIVISION
119 ERR_ENABLE_BUY_NUM
120 ERR_LICENCE
121 ERR_TEAM_MEMBER_LACK
122 ERR_SNS_NOTFOUND_EVENT
123 ERR_RETRY_LIMIT
124 ERR_MODE_BLOCKED
125 ERR_SERVICE_UNAVAILABLE
126 ERR_INVALID_GRANT
127 ERR_EXPIRED_ACCESS_TOKEN
128 ERR_INSUFFICIENT_PES_POINT
129 ERR_INSUFFICIENT_GP
130 ERR_TEAM_COACH_FULL
131 ERR_TEAM_COACH_DUPLICATE
132 ERR_REFRESH_LIST
133 ERR_RECV_PRESENT_ALL
134 ERR_PLAYER_NOTFOUND
135 ERR_COMMAND_BLOCKED
136 ERR_FREE_TO_PLAY_FULL
137 ERR_DIFFPLAYER_SEND
138 ERR_DIFFERENT_LIVEDATA_VERSION
139 ERR_TEAM_STAFF_FULL
140 ERR_PLAYER_LEVEL_MAX
141 ERR_PLAYER_DUPLICATE
142 ERR_RECV_PRESENT_FULL
143 ERR_UNUSABLE_ITEM_RECOVERY
144 ERR_INSUFFICIENT_TENDER_GP
145 ERR_PLAYER_PROTECTED
146 ERR_TENDER_TIMEOUT
147 ERR_BID_USER_MAX
148 ERR_INSUFFICIENT_ITEM
149 ERR_ADAPTABILITY_MAX
150 ERR_MANAGEMENT_MAX
151 ERR_CONCEPT_ALL_MAX
152 ERR_WATCH_LIST_MAX
153 ERR_WISH_LIST_MAX
154 ERR_WISH_LIST_ALREADY_ADD
155 ERR_ADD_SKILL_FULL
156 ERR_ADD_POSITION_FULL
157 ERR_GROUP_NOTFOUND
158 ERR_SQUAD_NOTFOUND
159 ERR_NOT_SUBSCRIPTION
160 ERR_INSUFFICIENT_EFP_POINT
161 ERR_SELF_REQUEST_FULL
162 ERR_OPPONENT_REQUEST_FULL
163 ERR_ALREADY_REQUESTED
164 ERR_ALREADY_CANCEL
165 ERR_ALREADY_ACCEPT
166 ERR_REJECT_LIST_FULL
167 ERR_NOT_EXISTS
168 ERR_FOLLOW_USER_LOCKED
169 ERR_LOCK_USER_FULL
170 ERR_SELF_CLAN_MEMBER_FULL
171 ERR_SELF_JOIN_CLAN_FULL
172 ERR_OPPONENT_CLAN_MEMBER_FULL
173 ERR_OPPONENT_JOIN_CLAN_FULL
174 ERR_TEAMPOWERLEVEL_NOTMET
175 ERR_COOPLEVEL_NOTMET
176 ERR_NOTCLANMEMBER
177 ERR_COOPMANNER_NOTMET
178 ERR_PESLEAGUE_MEMBER
179 ERR_PESLEAGUE_CLAN
180 ERR_ALREADY_CLAN_MEMBER
181 ERR_KONAMIID_MAINTENANCE
182 ERR_KONAMIID_INVALID_REQUEST
183 ERR_KONAMIID_ALREADY_LINK
184 ERR_KONAMIID_ALREADY_USE_ADDRESS
185 ERR_KONAMIID_UNDERAGE
186 ERR_DIFFERENT_EXTRADATA_VERSION
187 ERR_ALREADY_PICKUP
188 ERR_NOT_PICKUP_USER

---
# Cierre de huecos — 2.ª lectura (28-oct-2026 tarde, solo lectura; Ghidra 12.1.4 headless sobre PEStrim; C = visto en el código, P = deducido)
Nota de método: la herramienta REA (import completo de 60 MB) agota su plazo de 330 s dos veces; se cambió a `analyzeHeadless -noanalysis -readOnly` sobre el proyecto ya analizado (PEStrim), que responde en segundos. Scripts: `PRUEBA_CANCHA\scripts\ghidra\DecompilarFunciones.java` y `BuscarTextos.java`.

## (1) Valores de CMD_GET_SVRVERSION → mejor respuesta que lo anterior
- C `ERR_DIFFERENT_PATCHVERSION` solo aparece como TEXTO comparado contra el campo `result` que devuelve el servidor, en las respuestas de login (fn 140fdc040, OnlineLoginTaskConnectAccount.cpp) y de menú (fn 140fe0b60 → 140cf3990, OnlineLoginTaskConnectMenu.cpp). No hay una comparación numérica del cliente contra una constante en ConnectGate (fn 140fddef0 solo crea y espera los 4 comandos CmdGetUrlList, CmdGetSvrVersion, CmdGetSvrTime, CmdGetEulaNationList; error 0xb si falta uno, 0xc si alguno marca fallo).
- P Conclusión: el cliente guarda los 9 textos de versión y los reenvía (patch_version/dlc_version van en la petición del lobby); quien decide «versión distinta» es el SERVIDOR devolviendo `result = ERR_DIFFERENT_PATCHVERSION`. Por tanto los valores de `versiones` pueden ser cualquier texto no vacío para la primera prueba; el servidor propio debe compararlos con lo que el cliente mande, no al revés. Falta (menor): ver si livedata_version/eula_* se comparan con datos locales; la prueba real lo dirá (aparecería un mensaje de error de versión).
## (2) Tipos de URL obligatorios — SIGUE ABIERTO (P)
- Sin cambios: ERR_CLIENT_SENDFAIL no sale de una búsqueda de URL concreta. Salida práctica: devolver los 53 tipos de cmd_lista.txt apuntando al servidor propio; y recortar uno a uno cuando el servidor corra.
## (3) HTTPS y certificados (C salvo lo marcado)
- C El cliente HTTP usa **WinHTTP** (cadenas «[HTTP] WinHttpOpen/Connect/OpenRequest/SendRequest», agente `WinningEleven/16.0`; fn 14116f910 abre, 141170e90 arma la petición, 141171c50 es la devolución de llamada). Si el esquema es `https` pone el indicador WINHTTP_FLAG_SECURE (0x00800000) en WinHttpOpenRequest; método POST si hay cuerpo, GET si no; cabecera `Content-Type: application/json` o `application/x-www-form-urlencoded` según un indicador del objeto (+0x15c).
- C En esas tres funciones NO hay llamada a WinHttpSetOption (ni SECURITY_FLAG_IGNORE_*). El fallo de certificado llega por la devolución WINHTTP_CALLBACK_STATUS_SECURE_FAILURE (0x10000) y solo marca el estado 3 de la petición (error) → P: la validación es la normal de Windows (cadena de confianza del almacén de certificados), SIN pinning visible y SIN opción para desactivarla en el cliente.
- P Para HTTPS propio: crear una CA propia, instalarla en «Entidades de certificación raíz de confianza» del equipo (acción de administrador, reversible) y emitir un certificado para ntl.service.konami.net con esa CA; o ver si el cliente cae a la variante `http://` (hay 4 URL GateInfo http y 4 https en la tabla 143516158; la https de ntl es la primera, fn 141232ff0). No hay otra validación propia (el exe además lleva OpenSSL embebido, pero su uso es para NclMio/relay, no para estas peticiones HTTP — P).
- STUN (pes21-x64-stun) y GATE (pes21-x64-gate) NO son HTTP: gate = TCP con tramas NclMio; stun = UDP/TCP propios (P). TURN lo da la lista GET_TURN_SERVER_LIST (HTTP).
## (4) Formato de las cargas — MessagePack para NclMio (P fuerte), JSON para HTTP (C)
- C El exe tiene la clase RTTI `fox::ncl::NclMsgPackMessage` (pero no hay ninguna cadena «msgpack» de biblioteca) y las clases de cifrado `NclMio::DivBufForEasyEncryptionVer` / `NclEnhancedMio::DivBufForEnhancedEncryptionVer`. → las tramas NclMio llevan mensajes MessagePack.
- C El HTTP (GateInfo.php, listas) usa JSON o formulario según indicador (ver 3). Hay además una biblioteca interna «property-read-json.c / property-write-json.c» (lector/escritor JSON propio de Konami) que alimenta al analizador fn 141c190a0 (búfer de 0x2000). Esto explica que el texto anterior diga «tipo JSON»: el árbol de propiedades es el mismo para ambos formatos. Lo que va por el cable en ConnectGate sigue siendo MessagePack (P) → a confirmar con la primera captura real (capturas\*.jsonl).
## (5) Redirigir dominios desde dentro del proceso y DLLs de red
- C Importa/usa **ws2_32** (getaddrinfo, gethostbyname, connect), **winhttp** (todo el HTTP), **wininet** (nombre presente; InternetSetOption), **crypt32**, **secur32**, **iphlpapi**, **wlanapi**. Pero las funciones se resuelven en tiempo de ejecución por nombre (tabla de punteros DAT_14252f…; p. ej. getaddrinfo se pide con GetProcAddress en fn 1411987b0 y se guarda en 1434d6398) → NO hay tabla de importación estática que enganchar con IAT; sí se puede enganchar con Detours/MinHook las funciones ya cargadas (`getaddrinfo` de ws2_32, `WinHttpConnect` de winhttp, `connect`) desde un módulo de Sider o Phoenix Core una vez cargadas las DLL. El exe está protegido (Denuvo/VMProtect‑like, ver bitácora), así que enganchar WinHttpConnect (cambiar el nombre de servidor) es la vía más limpia: evita tocar el archivo hosts y evita permisos de administrador. P: factible; no probado.

## (2) Tipos de URL obligatorios — resultado (28-oct noche, lectura estática)
- C La tabla de 53 entradas (0x1437F3A20, indice = tipo) solo se lee por la función FUN_141b9a250(tipo) (devuelve la cadena guardada). Sus 20 llamadores usan SOLO estos tipos: 5 BANNER, 7 P2PMATCH, 17 MYCLUB_AGENT_BANNER, 18 DMM_TOP, 39 MYCLUB_LIVEEVENT_BANNER, 45 WEEKLY_INFO_BANNER, 46 WATCHING_GAME. Son pantallas del menú (banners, partidos vistos), no el inicio de sesión.
- P Ningún tipo se comprueba en ConnectGate (fn 140fddef0 solo espera a que terminen los 4 comandos). Una URL ausente deja la cadena vacía y falla la función que la use, NO el login. ERR_CLIENT_SENDFAIL no viene de aquí. Los tipos de uso directo en red (EULA_PES 3, TOPINFO 9, AUTH_STEAM 21, ONLINEID 25, GET_GAMERELAY 48, GET_TURN_SERVER_LIST 50) se piden con índice variable (no hay referencia directa), así que se verán en la prueba real.
- Decisión: config.json del servidor ahora devuelve los 53 tipos (cada uno a http://127.0.0.1:5000/<tipo en minúscula>); el espía HTTP del servidor (capturas/http.jsonl) mostrará cuáles pide el juego de verdad.

---
# Lo que aportan los 3 informes de investigación (Gemini, 29-oct) — verificado contra nuestra lectura
Aviso: son informes generados por IA con fuentes de foros; cada dato abajo se marca C (coincide con lo que vimos en el exe/servidor) o P (por probar).
1. **Dominios del conector que SÍ funciona (PESBUL)** (informe 3, §3): pes21-x64-gate, pes21-x64-stun, **pesam.stun.service.konami.net, info.service.konami.net, cs.konami.net**. NO usan ntl.service.konami.net. → C/P: el flujo funciona sin HTTPS propio (la lectura mía del GateInfo https era un camino que PESBUL no necesita). ACTIVAR_PRUEBA_ONLINE.ps1 ya cambiado a esos 5 dominios (antes llevaba ntl* y pes18-trial, que se quitaron; faltaban pesam.stun y cs.konami.net).
2. **Puertos** (informe 3): 10000/tcp = ConnectGate (el nuestro ✓); 8088/tcp externo → 80 interno = pasarela HTTP (nuestro 80 ✓); 5000/tcp = panel (✓ nuestro http); **5730/udp = STUN propio; 5739/udp y 5740/udp = candidatos P2P**. FALTA en nuestro servidor: un STUN en UDP 5730 (hoy solo hay un espía TCP en 443). Para la primera prueba no hace falta; sí antes de jugar un 1v1 real.
3. **Trampas ya conocidas por PESBUL (nos ahorran días):** (a) el 1.er paquete tras conectar es CMD_GET_SVRLIST (opcode 0x2EE4 → respuesta 0x2EF4) y el 2.º es CMD_DISCONNECT con `logoutstat: "MOV"` (cierra la consulta y pasa al servidor de sesión); si se responde con bytes genéricos el cliente corta ~50 ms después (POST_AUTH_CLOSE). Nuestro servidor responde SVRLIST pero CMD_DISCONNECT cae en «comando sin manejador → NOERR» (logica.py): hay que revisar que ese caso no cierre la conexión TCP del menú. (b) CmdLoginAccount (fn 140fdf300) solo acepta el estado 8; 0/2/3 = login falla por campos que faltan en la respuesta. (c) Tras «Crear perfil» hay que mandar la confirmación de cierre del casillero de perfil (fns 1411E4390/1411E3BF0/1412402B0) o el cliente se queda enviando latidos y pierde conexión. (d) Bug «anfitrión Listo solo»: limpiar el estado de sala al entrar un invitado y al terminar. (e) Online con tráfico `post-Secure` comprimido es el bloqueo de PESBUL para 11v11; en 1v1 va P2P directo.
4. **Redirección en memoria** (informe 3, idea 1): MinHook sobre `getaddrinfo` (ws2_32) devolviendo la IP del servidor para `*.konami.net`; y también `connect` por si el juego usa IP fija (18.165.61.88, 40.86.103.106). Coincide con lo que vi (getaddrinfo se resuelve dinámicamente en fn 1411987b0). Nadie en PES lo ha hecho (lxllxl lo intentó con netblock.rar). Encaja con la regla de Frank: opción encender/apagar en un PES limpio.
5. **Auto-login sin «Favorite Team»:** inyectar el SteamID/token en el campo np_account_id de CMD_LOGIN desde nuestra DLL (nuestro servidor ya mapea SteamID → cuenta Phoenix en vinculos.json).
6. **Para jugar en parche propio** (informe 2): la sincronización exige misma DB/EDIT/exe en ambos lados; sider.ini sin netblock.lua; exe 1.07.02 original de Steam. Los gráficos/audio por Sider no afectan. Phoenix Link puede comparar hashes antes de abrir el lobby.
7. **Riesgos del informe** que ya cumplimos: no CA, no ejecutar .BAT ajenos, solo copias legales, sin dinero real; evitar imágenes Docker sin fuente (noobsaibot301) y .rar sueltos (netblock.rar, PES 2021 Launcher.rar): nunca ejecutarlos.
8. **Cámara lenta en WAN** hasta que el balón sale (informe 3): resincronización de reloj; idea para Core (microajuste ±1 % medio segundo). Idea nueva a registrar en el catálogo de hallazgos.
