# Estudio del cliente de PESBUL 2021 (análisis estático, sin ejecutar nada)

Fecha: 4-nov-2026. Objetivo (Frank): entender cómo funciona PESBUL, sin mezclarlo con lo nuestro.
Reglas: solo estático; no se ejecuta nada del paquete; no se toca hosts; no se copian claves ni código de PESBUL
(solo se describe el comportamiento). Lo nuestro (servidor Phoenix Online y bloque PHOENIX_PRUEBA de hosts) quedó APAGADO
el 4-nov para no mezclar resultados (ver «Cómo reactivar» en la bitácora hermana).

## 1. Parte 1: `PESBUL_2021_Server_Connector.bat` (recibido por el chat)
Archivo de texto, 244 líneas, título «PESBUL 2021 SERVER EMULATOR». Es TODO el «cliente» que ven los jugadores en esta parte:
un menú con 2 opciones, [1] CONNECT y [2] DISCONNECT. No descarga nada, no instala servicios, no toca el exe ni steam_api,
no usa Sider, no abre puertos, no mete certificados. Solo edita el archivo hosts de Windows.

Qué hace, paso a paso:
1. Se auto-eleva a administrador (si `fltmc` falla, se relanza con PowerShell `Start-Process -Verb RunAs`).
2. CONNECT: crea una copia de seguridad `hosts.pesbul-backup` (solo si aún no existe) y vuelve a escribir hosts quitando TODA
   línea que contenga «konami.net» (¡borra cualquier otra entrada de ese dominio que tuvieras!) o «# PESBUL 2021»;
   si el resultado queda vacío pone `127.0.0.1 localhost`. Después añade un bloque «# PESBUL 2021» con 7 entradas:
   un ÚNICO IP de su servidor (guardado en el .bat como 4 variables P1..P4 para no verse como cadena; aquí no se repite)
   apuntando a estos 7 dominios de Konami:
   - pes21-x64-gate.cs.konami.net  (gate: ConnectGate TCP, el primero que consulta PES)
   - pes21-x64-stun.cs.konami.net  (STUN)
   - pesam.stun.service.konami.net (STUN)
   - info.service.konami.net
   - cs.konami.net
   - ntl.service.konami.net  (NTL: GateInfo.php por HTTPS 443)
   - ntleu.service.konami.net (NTL EU: ReportLog.php por HTTPS 443)
   Verifica cada línea con `findstr` y hace `ipconfig /flushdns`. Mensaje final: «Start eFootball PES 2021».
3. DISCONNECT: vuelve a quitar «konami.net» y «# PESBUL 2021» (no restaura el backup; conserva el resto) y flushdns.

## 2. Lo que se deduce (hechos y comparación con lo nuestro)
- Un solo servidor (una IP) atiende TODO: gate, menú, HTTP, STUN y NTL. No hay split por región ni balanceo visible.
- Son 7 dominios; nosotros usamos 8 (además `ntljp.service.konami.net`). Con PESBUL no hizo falta ntljp para llegar a jugar.
- No hay parche del exe ni de la DLL: el cliente es PES 2021 + el shim Steam (CPY) normal; el resto va en el servidor.
  Esto confirma lo que descubrimos: se puede llegar hasta Matchmaking sin tocar el exe.
- Los dominios ntl/ntleu por HTTPS no se redirigen a un servidor HTTP propio con certificado: en el servidor de ellos
  solo hay un «registrador» (según lo que vio el coordinador antes); basta con que la llamada NO llegue al Konami real.
- La limpieza es burda (borra todo «konami.net»); para evitar efectos laterales nosotros usamos un bloque delimitado.
- NO está hecho ningún acceso a su servidor: no se sondea ni se prueba su IP sin permiso expreso de Frank.

## 3. Parte 2: tráfico real PES base <-> servidor de PESBUL (captura pktmon + descifrado, 4-nov-2026)
Método: `tools/captura_pesbul.ps1` (pktmon, filtro por la IP de su servidor, elevado) y `tools/descifrar_captura.py`
(reensambla TCP, usa nuestro `nclmio.py` y la clave local: NUNCA se imprime). Frank ejecutó el `.bat` (CONNECT) y abrió
`C:\PES21\eFootball PES 2021\PES2021.exe` sin Sider (no hay sider.dll ni phoenix_core entre los módulos vistos). Se descifraron
104 mensajes con 0 errores: **su cifrado es exactamente el nuestro (misma clave NclMio, misma trama XOR+Blowfish+MD5)**.
Capturas y JSONL completos: `D:\Frank\IA\CLAUDE\pes-online\capturas_pesbul\` (fuera del repo, no se suben).

### 3.1 Orden de la conversación (gate, TCP 10000; todo contestado NOERR)
1. `GET_SVRLIST {svrtype GATE, timeout 0, lang ANY, patch_version 1.07.02, dlc_version 7.00}` → `server_num 3`:
   `{GATE, puerto 10000, gid 1}`, `{ACCOUNT, puerto 10000, gid 2}`, `{MENU, puerto 10001, gid 3}` (max_player_num 100 cada uno, mismo IP).
   OJO: el LOGIN (ACCOUNT) va por el 10000 y solo el MENU usa el 10001. (Comando que nuestro servidor no recibió nunca.)
2. `GET_URLLIST {lang MX, region REGION_EU, patch_version, dlc_version}` → `url_num 20` (ver 3.2).
3. `GET_SVRVERSION` → `version 1.07.02, patch_version 1.07.02, dlc_version 7.00, eula_version '1', eula_myclub_version '1',
   livedata_version '0', eula_privacy_policy_version '1', eula_privacy_notice_version '0',
   eula_privacy_policy_california_version '0', eula_privacy_policy_japan_version '0'`.
4. `GET_SVRTIME` → `date` = hora unix. 5. `GET_EULA_NATION_LIST {lang MX}` → 33 naciones
   (BG DE GB FR IT GR ES NL PT PL RO VN PS MA DZ TN LY EG SA AE IQ TR TH ID PH MY IN JP KR BR AR US MX), cada una con
   `nation_code, privacy_kind PRIVACY_POLICY, has_area NO, name`.
6. El CLIENTE manda `DISCONNECT logoutstat MOV` y reabre: `GET_URLLIST` + `GET_GLOBALINFOTITLELIST {lang MX, region REGION_EU,
   package_type DEFAULT, kind DEFAULT}` → 3 avisos («Welcome to PESBUL 2021», «Online Server Development Progress»,
   «Important Test Notice»; announce_id 2021201..203, prioridades 30/20/10, popup_type VIEW_DETAIL) → otro `DISCONNECT MOV`.
7. HTTP (puerto 5000, no 80): `POST //auth/steam HTTP/1.0` (193 B) → `200 OK` + msgpack `{msgid CMD_AUTH_STEAM, rqid, result NOERR}`.
   Ojo: la ruta es `/auth/steam` (con barra, no `auth_steam`) y sigue la doble barra inicial.
8. TCP: `SEND_HEARTBEAT` → `GET_CHALLENGECODE {svrtype ACCOUNT}` → `NOERR challenge_code '00112233445566778899AABBCCDDEEFF'`
   (**CONSTANTE, no aleatorio**) → `LOGIN {username '410000004100000', platform PC, patch 1.07.02, dlc 7.00, sku SKU_EU1,
   np_account_id 0, pairwise_id NotImplement…}` → **`ERR_FORBIDDEN`** (3 intentos, ~0,8 s de respuesta) → el cliente manda
   `DISCONNECT MOV` ~4 s después y cierra (cliente FIN + RST; el servidor no cierra por su cuenta).
   PES muestra «Ha ocurrido un problema inesperado al establecer la conexión con el servidor PES 2021. CODE:C_NAJM_0022».
   Interpretación: C_NAJM_0022 coincide con el login rechazado. En el código de PESBUL (según el coordinador), ERR_FORBIDDEN
   solo sale si la IP o el perfil están bloqueados; **no se evade (ni SteamID ni IP)**: hay que pedirles acceso.
9. UDP: PES hace STUN real con su servidor: puerto local 9483 ↔ 3478, 34 peticiones de 64 B y 34 respuestas de 88 B (binding),
   antes del login. Nuestro servidor nunca recibió UDP por usar otra secuencia.

### 3.2 La URLLIST de PESBUL (20 claves, todas en su servidor puerto 5000; rutas sin la IP)
EULA_PES `/eula/pes`; EULA_MYCLUB `/eula/myclub`; EULA_PRIVACY_POLICY `/eula/privacy-policy`; AUTH_STEAM `/auth/steam`;
LiveDataSetFlow / LiveDataSetFlowOnMode / LiveDataSetFlowOffMode / LiveDataSetFlowExhibition / LiveDataSetEditData
→ `/livedata/<mismo nombre>`; FLOW → `/livedata/LiveDataSetFlow`; AVATAR → `/avatar/service/AVATAR`;
CMD_GET_AVATAR_HTTP → `/avatar/service/CMD_GET_AVATAR_HTTP`; GET_GAMERELAY → `/gamerelay`;
GET_GAMERELAY_QUALITYCHECKLIST → `/gamerelay/qualitychecklist`; RANK → `/ranking/RANK`; DIVISION_RANK → `/ranking/DIVISION_RANK`;
GET_TURN_SERVER_LIST → `/ntl/turn/serverlist`; DMM_TOP → `/asset/dmm-top`; WEEKLY_INFO_BANNER → `/asset/weekly-info-banner`;
BANNER → `/asset/pesleague-banner`.
Comparación con la nuestra (26): ellos NO tienen TOPINFO, ONLINEID, P2PMATCH, WATCHING_GAME, WEB_API, LOCAL_GAMERELAY ni ADFILE;
sí tienen CMD_GET_AVATAR_HTTP; usan rutas con subcarpetas. Aun así PES funciona con ambas listas (la nuestra llegó a Matchmaking).

### 3.3 Qué se aprende / qué copiar (descripción, sin copiar su código)
- Mismo cifrado y trama que nosotros: lo que ya implementamos es compatible con el cliente real.
- Responden `GET_SVRLIST` con 3 servidores (GATE/ACCOUNT/MENU) en el mismo IP: nosotros no lo atendíamos; conviene implementarlo igual.
- Tienen control de acceso por cuenta/IP en el LOGIN (ERR_FORBIDDEN): útil para nuestro servidor (vinculos.json ya lo hace).
- STUN real en 3478 con respuestas de 88 B: nuestro `stun.py` debería devolver la dirección observada del cliente.
- Versiones EULA '1' y livedata '0': por eso PES no vuelve a pedir Términos si ya los aceptó.

## 4. Pendiente
No se vio el resto del flujo (menú, matchmaking, partido) porque el login fue rechazado. No hay paquete de archivos aparte del `.bat`
(no apareció `pesbul_cliente`). Opcional si Frank lo autoriza: REA/Detect-It-Easy sobre el servidor de PESBUL (no disponible) o
repetir la captura con una cuenta autorizada por ellos. Cruzar con `BITACORA_PES_ONLINE_HASTA_MATCHMAKING.md`.
