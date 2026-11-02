# Bitácora-guía: PES 2021 online contra un servidor propio (Phoenix Online) — de cero a «Buscando rival»

Fecha: 1-nov-2026 (noche). Sesión de pruebas con Frank. Amplía y reemplaza `bucle_create_player.md`.
Resultado: PES2021.exe (parche ConmeGOL, SIN Sider, shim Steam CPY) llega a la pantalla
«Matchmaking – Buscando rival» con la tarjeta del jugador (nombre FRALEX) contra el servidor Python
`phoenix_online` que corre en la misma PC. Captura: `docs/capturas/menu_online_02nov.png`.

Estado honesto: NO está probado emparejar a dos jugadores, la conexión de partida (UDP/STUN/TURN), ni jugar/cerrar un
partido. Hasta aquí es el «menú online». Los comandos de partido siguen desconocidos.

## 1. Qué se necesita (entorno)
- PES 2021 + parche ConmeGOL en `D:\Frank\Games_\Conmegol Patch`, lanzado con `PES2021.exe` DIRECTO (sin Sider).
  El cliente declara patch_version 1.07.02, dlc_version 7.00, sku SKU_EU1, region REGION_EU, lang SP.
- Shim de Steam CPY (steam_api64.dll): da un steamid fijo 239200 (hex 3a660). Dos jugadores con el mismo id serían el mismo jugador.
- Servidor: carpeta `D:\Frank\IA\CLAUDE\pes-online\phoenix_online`, `python -m phoenix_online.servidor config.json`.
  Escucha TCP 10000 (ConnectGate), 10001 (MENU), 5000 y 80 (HTTP), UDP 3478 y 5730 (STUN, nunca recibió nada).
  Código de referencia: rama `claude/project-thread-9ciw66` de Fralexito/smash-soda-fork, carpeta `phoenix_online/`.
  Clave NclMio (cifrado del canal): `D:\Frank\IA\CLAUDE\pes-online\nclmio_key.bin` (no copiar ni mostrar).
- hosts de Windows (bloque `# PHOENIX_PRUEBA_INICIO/FIN`, se quita con `RESTAURAR_HOSTS.ps1`), todos a 127.0.0.1:
  pes21-x64-gate.cs.konami.net, pes21-x64-stun.cs.konami.net, pesam.stun.service.konami.net, info.service.konami.net,
  cs.konami.net, ntl.service.konami.net, ntleu.service.konami.net, ntljp.service.konami.net.
  Para otra PC: apuntar a la IP del servidor (no 127.0.0.1) y que ese servidor sea alcanzable (VPN o puertos abiertos).
- Herramientas usadas: MCP phoenix-pes (teclas, capturas, memoria en solo lectura), Frida 17.24.3 (solo Interceptor),
  SendKeys de Windows para escribir el nombre (el MCP no tiene la L ni la X).

## 2. Paso a paso (cada muro, la causa y el arreglo del servidor)
Cada punto es un commit de la rama: lo que PES hacía, qué se vio y qué hubo que cambiar.
1. **Pantalla de país con error**: solo había 1 nación en config y faltaban claves en la URLLIST. Con 2 naciones (PE, AR)
   y «nacion»: «PE» sale «País/Región». Al confirmar «Sí» salía «Se ha producido un error».
2. **Falta de EULA** (5f3618b): tras confirmar el país, el cliente busca en la URLLIST `EULA_PES`, `EULA_MYCLUB`,
   `EULA_PRIVACY_POLICY`. Sin ellas «muere en silencio» (error genérico). Se añadieron y el HTTP sirve `GET /eula/*`.
3. **Términos de uso**: la casilla «Acepto» está bloqueada hasta pulsar Enter sobre CADA uno de los 3 documentos
   (el visor usa el overlay de Steam, que el shim no tiene: no se abre nada pero basta intentarlo). Luego casilla y «Aceptar».
4. **AUTH_STEAM** llega por HTTP: `POST //auth_steam` (doble barra) cuerpo `msgid=CMD_AUTH_STEAM&rqid=0&urltype=AUTH_STEAM&steamid=239200&package_type=DEFAULT`.
   Se responde con el sobre msgpack estándar {msgid, rqid, result NOERR}.
5. **Login rechazado** (12eb08c): CMD_LOGIN trae `username` = steamid en hex SIN ceros (`3a660`); la lista de cuentas lo tenía
   como `000000000003a660` → «Falló la verificación de la cuenta» (CODE:QIMJ_0014). Arreglo: rellenar a 16 dígitos.
6. **Crash en 0x141d3fe49** (fef9616): `psn_user_id` dentro de GET_PLAYER_DATA debe ser un MessagePack empaquetado
   (bin con account_id, online_id, is_valid_cache), no cadena vacía.
7. **Bucle de CREATE_PLAYER** (52cb3d0, 73a7901, d55b2ed, 863785d): PES repetía CREATE_PLAYER→SET_LANGUAGE→GET_PLAYER_DATA sin fin.
   Lo que lo corta (de PESBUL R7.3.121): `has_data` debe ser el STRING «YES» (o «NO» si no hay perfil), `pid`, y
   `pname` como bin; el perfil «rico» con muchos campos hace que el cliente se desconecte. Además PES manda luego
   `CMD_SET_CURRENTPLAYER` (sin guion bajo entre CURRENT y PLAYER) y hay que contestarlo. CREATE_PLAYER debe ser idempotente
   (UNIQUE steam_hex rompía el servidor y cortaba la conexión). El nombre mostrado es el que el cliente envió en CREATE_PLAYER.
8. **«Error al descargar los nuevos archivos de datos»** (dbed63a): la URLLIST necesita la clave exacta `FLOW` y las
   `LiveDataSetFlow/OnMode/OffMode/Exhibition/EditData`, y el HTTP `/livedata/*` debe responder msgpack «sin actualización»
   (versiones 1.07.02 / DP07 / 7.00). Si no, falla. Con eso aparece «No hay actualizaciones a través de Actualización en vivo».
9. **Tienda/logros** vacíos: GET_STORE_PRODUCT_LIST, GET_GP_ACHIEVEMENT, GET_STORE_TRANSACTION_STATE con listas como arrays vacíos.
10. **Lote del menú** (16a048f): GET_SERVER_ENV completo (max_division «D12», is_revision_check 0, timeouts…), MATCH_ANALYZE_ENV,
    MANNER_WARN {warn_kind 0}, GAME_TEAM_LIST {team_list [], num 0}, DMM_PLAYERINFO {is_present_inbox 0}, SEND_LOG.
    GET_DMM_NOTIFICATION, CHECK_CHEAT y GET_URGENT_ANNOUNCEMENT van con NOERR genérico.
11. **Crash en 0x140c5ec4b** (2399667): era `strlen(NULL)` al crear una ventana de aviso del menú online (TopMode);
    las claves `DMM_TOP`, `WEEKLY_INFO_BANNER`, `RANK`, `DIVISION_RANK` y `AVATAR` faltaban en la URLLIST (26 URLs, 687 B comprimidas).
    La URL nula de imagen DMM_TOP provocaba el crash. Con eso PES entra al modo online (JOIN_ONLINE_MODE QUICK).
12. **Matchmaking** (87cf63d, 54958ff, 4a48e48): GET_QUICKMATCHOPTION con opciones; START_MATCHING se acusa y se EMPUJA
    `CMD_WATCH_MATCHING` con matching_state SEARCHING y la tarjeta propia en `member`; CANCEL_MATCHING empuja matching_state CANCEL;
    avatar vacío por HTTP (GET_AVATAR_HTTP); el nombre se carga al LOGIN de la conexión MENU (si no, sale «Phoenix»).

## 3. Orden de la conversación del cliente (cuando funciona)
Gate TCP 10000: GET_URLLIST → GET_GLOBALINFOTITLELIST → el CLIENTE manda CMD_DISCONNECT logoutstat=MOV (normal: pasa al MENU).
MENU TCP 10001: GET_CHALLENGECODE → LOGIN → SEND_PSID → GET_PLAYER_DATA (→ CREATE_PLAYER/SET_LANGUAGE si no hay perfil) →
SET_CURRENTPLAYER → tienda/logros → SEND_LOG → SEND_IPANDPORT_NTL → GET_GAME_TEAM_LIST → GET_SERVER_ENV → … → JOIN_ONLINE_MODE QUICK
→ CHECK_CHEAT → GET_URGENT_ANNOUNCEMENT → MANNER_WARN → PRIVATEINFOTITLELIST → GET_QUICKMATCHOPTION → MAINTENANCE_INFO →
SEND_IPANDPORT_NTL → START_MATCHING. Al cancelar: CANCEL_MATCHING → LEAVE_ROOM (error_code EYRH791) → LEAVE_ONLINE_MODE.
HTTP 5000: POST //auth_steam, POST //livedata/LiveDataSetFlow, GET_AVATAR_HTTP.
Capturas de cada sesión: `capturas/*MENU*.jsonl` (rx/tx por comando) y `capturas/http.jsonl`.

## 4. Errores e intentos fallidos (para no repetirlos)
- Frida: NUNCA Stalker, ni hooks en vtables del shim Steam, ni dejar que el script haga detach: PES se cierra
  (frida-agent.dll c0000409). Un Interceptor simple en el exe aguanta mientras el script no termine.
- Hosts sin servir 443 (ntl.service.konami.net/GateInfo.php) no era la causa de nada; se añadió por precaución.
- Reiniciar el servidor mientras PES está en una prueba mata su conexión (error C_MHJX_0002): reiniciar solo con PES cerrado.
- Respuestas «inventadas» con campos extra no ayudan: PES espera EXACTAMENTE el esquema de PESBUL (claves y tipos).
- Escribir FRALEX con SendKeys: la primera letra sale minúscula; borrar 7 veces y repetir con Shift (+f +r +a +l +e +x).

## 5. Pendiente (no replicar como si estuviera hecho)
Emparejar 2 jugadores (WATCH_MATCHING FINISH + chequeo de conexión), STUN/TURN (PES anuncia 127.0.0.1:~14466, ntl_nat_type 2; ningún UDP
llegó a 3478/5730), jugar/cerrar partido, lobby/amistoso, vínculo con la web (Edge Function + steamids), servidor 24/7 y VPN.

## 6. Cómo comprobar y revertir
Comprobar: abrir PES2021.exe, pulsar Enter, esperar (país → Términos → usuario si pide), EF → Partida rápida → «Ir a Matchmaking»:
debe verse la tarjeta propia en «Local». Revertir: `RESTAURAR_HOSTS.ps1`, cerrar PES, parar el servidor. Copias `.antes_<commit>` de cada archivo cambiado.

## 7. Para tu propio parche
Esto es el cliente de PES 2021 original hablando con el servidor; no hace falta tocar el exe ni Sider. Si tu parche cambia el
exe/dll (versión, huellas CHECK_CHEAT, patch_version) el cliente enviará otros valores: el servidor hoy NO los valida, pero hay que
cuidar que el parche conserve los mismos nombres de dominio de Konami (el exe los usa para conectarse). Cualquier parche con el MISMO
exe base funciona igual; para la cara «cliente» solo se necesita el bloque de hosts y el shim Steam con steamid distinto por jugador.
