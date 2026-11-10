# Phoenix Online — estado para retomar (29-oct-2026, 19:50 UTC)

**Qué es:** un servidor online propio para PES 2021, al estilo PESBUL, ligado a las cuentas de la web de Frank. Código en `phoenix_online/` (Python, sin dependencias salvo `cryptography`). Tiene 22 pruebas (`python -m unittest discover -s tests`).

## Ramas (repo Fralexito/smash-soda-fork)
- `claude/project-thread-9ciw66` (PR #3, borrador): servidor, guía y documentación (`phoenix_online/docs/`).
- `claude/online-phoenix-core` (2781290 + c8e3d9f): opción del Core «online_phoenix», apagada por defecto. Hace la redirección dentro del juego. El hilo street ya la revisó, pero **todavía no está integrada ni instalada**.

## Dónde quedó la prueba real (29-oct)
1. PES (Conmegol) se abre **sin Sider**, porque Conmegol esconde los botones online con livecpk\Menu. El botón online es el icono «EF».
2. Con el hosts temporal, PES se conecta a nuestro ConnectGate y completa 7 comandos. Aparece la pantalla «País/Región: Peru», servida por nosotros.
3. **Bloqueo:** al confirmar Peru sale «Se ha producido un error.» y vuelve al menú, sin más red. Sospechosos:
   - fn 140fe2920 / 140fe2fb0 (diálogo AreaSelectAgreementDialog, error 4);
   - valores de `privacy_kind` y `has_area` (¿"0" o "1"? ¿"true"?);
   - un recurso de interfaz que falta sin Sider.
   Siguiente paso: decompilar esas fns con GhidraMCP y probar variantes en el config.

## Pendiente (en orden)
1. Resolver el error tras elegir país. Después: AUTH_STEAM (HTTP, ver qué formato de respuesta espera), CHALLENGECODE, LOGIN_ACCOUNT, SEND_PSID, PLAYER_DATA, crear perfil y lobby.
2. Envío segmentado de respuestas mayores de 0x410 (NclMio::DivBufForEasyEncryptionVer). Hoy la lista de URLs está recortada a 14.
3. Identidad: el SteamID de las copias CPY es falso y es el mismo para todos (239200). Plan: un «pase» de la web, escrito por Phoenix Link y metido por el Core. Validación en Supabase (migración ≥162).
4. Integrar online_phoenix en el Core. Volver a dar el botón online con Sider sin romper Conmegol: una casilla propia en el menú.
5. Prueba entre dos PC (P2P UDP 5730‥5739, TURN).

## Reglas
- La clave NclMio (56 B) vive **solo** en `D:\Frank\IA\CLAUDE\pes-online\nclmio_key.bin`. Nunca va en el chat, los documentos ni los repos.
- La división en dos hilos (sala limpia) se retiró el 30-oct, pero sigue la regla de Frank de **no usar nada de PESBUL directamente** (ni ejecutarlo ni copiar sus respuestas). Solo nuestro servidor, el tráfico y la memoria de PES, y RE del exe.
- Al terminar una prueba, restaurar hosts con `RESTAURAR_HOSTS.ps1` (como administrador). **A las 19:50 del 29-oct seguían puestas las 5 líneas**: confirmar con Frank.
- Configuración que funciona: `blowfish_le: false`. GET_SVRLIST devuelve ACCOUNT y MENU. Los campos de nación y de perfil van como cadena.

## Para la próxima prueba (nota del 29-oct, 20:10 UTC)
- El servidor de la PC está **apagado** porque la PC se reinició. Para arrancarlo: en `D:\Frank\IA\CLAUDE\pes-online\phoenix_online`, ejecutar `python -m phoenix_online.servidor config.json`. Ya tiene instalado adcc2d1, con su config.json y vinculos.json locales.
- Revisar el hosts: si siguen las 5 líneas PHOENIX_PRUEBA, quitarlas con `RESTAURAR_HOSTS.ps1` (como administrador) en cuanto acabe la prueba.
- Pista del error del país: leído por memoria, el vector de naciones del login manager estaba **vacío** en el momento de confirmar. Sospecha: la lista que se dibuja no es la misma que usa fn 140fe2fb0, o se vacía al cerrar la conexión (DISCONNECT MOV) antes de que se elija el país.
- **Prueba del 29-oct a las 20:19** (con has_area "1" y la zona Lima): el error sigue igual y no llega CMD_GET_EULA_AREA_LIST. La pantalla de zonas (MenuSelectEulaArea, fn 140fe23a0 → 140fe2bc0 pide las zonas) nunca se abre, porque el error 4 salta antes, en 140fe2920. **Siguiente paso:** con REA, averiguar la condición que elige 140fe3f50 o 140fe2920 (estado del manager en +0x94 == 2) y qué dato del servidor la pone.

## 02-nov (con permiso de Frank para estudiar PESBUL)
- Arreglado el error del país: `has_area` debe ser "YES"/"NO" y `privacy_kind` "PRIVACY_POLICY" (commit a30538a). Ahora sale «Lista de imágenes: no hay datos», porque la lista de avisos estaba vacía. Desde c91ff89 se manda siempre 1 aviso (NO_BANNER_INFO) y se responde CMD_GET_INFOBODY. **Falta probarlo.**
- El .BAT de PESBUL redirige 7 dominios. Faltan en nuestro hosts y en el Core los de NTL: `ntl.service.konami.net`, `ntleu.service.konami.net`, `ntljp.service.konami.net`. Añadirlos a ACTIVAR_PRUEBA_ONLINE.ps1 y a redireccion_online (ya cubre *.konami.net).

## 02-nov noche — HITO: matchmaking de Partida rápida (un jugador) completo
Recorrido que ya funciona en la PC de Frank, sin Sider, con hosts temporal: país → Términos → AUTH_STEAM → login → crear usuario (FRALEX) → actualización en vivo («no hay actualizaciones») → menú online → equipos → «Opciones de Matchmaking» → «Buscando rival» con la tarjeta FRALEX en Local → Cancelar vuelve al menú. Después, PES recuerda país, Términos y usuario.

Claves aprendidas (detalle en el código, comentarios C/P):
- URLLIST debe traer EULA_PES/MYCLUB/PRIVACY_POLICY, FLOW + LiveDataSet*, DMM_TOP, WEEKLY_INFO_BANNER, BANNER, RANK, DIVISION_RANK y AVATAR. Si falta una clave, la URL es NULL y el juego falla en silencio o se cae.
- GET_PLAYER_DATA: has_data "YES" + pid + pname (bin). psn_user_id va como bin, nunca "".
- Matchmaking: START_MATCHING solo lleva msgid/rqid/result. El estado viaja por un EMPUJE CMD_WATCH_MATCHING (rqid 0, result NOERR): SEARCHING (con tu tarjeta), FINISH (pareja, con side HOME/AWAY) y CANCEL. Luego, cuando los dos mandan SET_GAMERELAY_CHECK_RES, se empuja CMD_WATCH_GAMERELAY_CHECK_RES con error_code NOERR. La pareja de dos todavía NO está probada.
- La conexión MENU no pide GET_PLAYER_DATA: el perfil se carga en el LOGIN (4a48e48).
- Avatar: base64 de una imagen 128x128x3 (49152 B), con md5 de los bytes decodificados. Si no hay avatar, va vacío.
- Al cancelar, PES manda LEAVE_ROOM con error_code EYRH791. Es normal.
- Frida: hacer detach de un script cierra PES (c0000409 en frida-agent.dll). No usar Frida con detach durante una prueba. Stalker también tumba PES.
- No reiniciar el servidor durante una prueba (da C_MHJX_0002).

Pendiente:
1. Prueba con 2 PCs (Partida rápida y amistoso). Hace falta que el servidor sea alcanzable (VPS o la PC de Frank abierta).
2. NAT: con el servidor en 127.0.0.1, PES publica 127.0.0.1:puerto_local como host/reflexive y no manda UDP a nuestro STUN (3478/5730). Con el servidor remoto hay que comprobar qué publica. PESBUL documenta listeners en 3478/5730/5739/5740.
3. Después: TURN, saltar el onboarding, el «pase» de identidad (Link + Core + Supabase ≥162), redirección del Core sin hosts, VPS y envío segmentado.
4. Hosts: siguen activas 5+3 líneas. Restaurar con RESTAURAR_HOSTS.ps1 cuando Frank termine.


## 04-nov-2026 — PES base + relé propio (leer primero)
- Se estudió una copia local de PESBUL (solo lectura, en la PC de Frank, fuera de repos) y se reimplementó con palabras propias.
- Hecho y probado en vivo con el PES base: vestíbulo (BLOCKLIST con un bloque, JOIN_LOBBY con gamerelay_web_url, gamerelay_index del nodo), salas con fichas en formato correcto (anidados en bin, enumerados en texto; antes PES se cerraba en msvcrt), Divisiones (GET_DIVISION_INFO cuenta nueva; proto_opt JSON con perfil RANKMATCH), tramas troceadas 0x0070-0x0072, y el relé UDP completo para la sonda (KX, clave M, marcos zlib, sal de 6 bits, listas A/B, eco por elemento) → **18 ms**.
- Pendiente: la sonda se repite cada ~9 s en Divisiones (sin daño); promotion_point 99999 se ve raro; respuestas de myClub; **fase 2/3 del relé (dos jugadores y reenvío del partido)**; VPS; web.
- Estado de la PC: hosts apunta a 192.168.100.236 (copias hosts.antes_pesbul_local*); servidor propio encendido; copia local de PESBUL apagada.

## 04-nov noche (2): lo que va con un solo PES
- Inventario completo de PESBUL frente a lo nuestro: docs/INVENTARIO_PESBUL.md (194 órdenes en PESBUL, 78 nuestras antes de esta ronda).
- Nuevos manejadores (logica.py): vestíbulo (JOIN_BLOCK, LEAVE_BLOCK, LEAVE_LOBBY, CHECK_STRING), perfil (SET_PLAYERPROFILE, GET_PLAYERINFO), registros (GET_DIVISION_POINT_INFO, GET_GAMERESULTS, GET_OFTEN_USED_TEAM_LIST, GET_RANKING y variantes). Avisos y chat quedan como vigilancias abiertas.
- datos.py: tabla nueva `perfiles` (equipo favorito, jugador favorito, presentación) y `todos()` para el ranking.
- Tipos que importan: en GET_PLAYERINFO, favoriteTeam/favoritePlayer/intro van arriba; rankmatch_stats y manner van como bin; division es texto «D12». En el ranking, los elementos son mapas, no bin. En DIVISION_POINT_INFO siempre van 12 filas.
- Pruebas: 48 OK. Pendiente verlo en PES: Editar perfil, Registros de Divisiones, entrar y salir del vestíbulo.

## 05-nov madrugada — myClub navegable, camino al partido vs COM (leer primero)
- Funciona en el PES base (C:\PES21) contra nuestro servidor:
  - Sede de club, plantilla de 18 sin «!», DT con nombre, Miembros, Contrato, Cazatalentos, subastas, Estadíst.
  - «Partido igualado» llega hasta pedir el rival.
- Commits:
  - 203c107: parámetros y rival.
  - b75a83e: MATCH_LEVEL_REGULAR.
  - c9d66a0: tipos del rival corregidos tras el crash, ver ESPEC_MYCLUB §20.
  - Último: club propio por cuenta, GP por partido e historial vs COM, ver §21.
- Siguiente:
  1. Confirmar que el partido arranca con c9d66a0+.
  2. Capturar SET_MYCLUB_VSCOM_GAMERESULT y ajustar _desenlace_vscom.
  3. Capturar los comandos de Contrato y Cazatalentos y cobrar con _sumar_gp.
  4. Agentes.
  5. La identidad propia, para que cada persona tenga su club. Hoy CPY comparte SteamID.
- El config real de la PC tiene myclub.rival con 18 ids del Real Madrid. No subirlo nunca.

### 05-nov 06:10, myClub: fichajes
- **Funciona en vivo:**
  - Cazatalentos: fichar (L. Millar), vender (+500 GP) y la barra de GP.
  - Partido vs COM completo, con experiencia (Nv.2) y MAIN_SQUAD completa.
- **Hecho en el servidor:**
  - Logros de GP (SET_GP_ACHIEVEMENT).
  - Abandonar = derrota.
  - Posición y nación desde Player.bin.
- **Representantes:** la lista y la caja ya abren (0d4fa63, d7ac264, 6089f7f). Falta el aviso «caducó» (ESPEC §37-38).
- **Falta:** renovar contrato (no se encuentra la acción en el juego), pausa y abandono en vs COM, subastas, la tabla real de GP por logro.
- Ideas de Frank en PLAN_MYCLUB: arquitectura con la web, intercambios, torneos con el club de cada uno.

## 05-nov 08:10 — Prueba en vivo del menú del jugador
Bloquear/Desbloquear, Liberar, Convertir en Entrenador y caja de Representante (X) probados con Frank: todo OK, sin excepciones. Detalle en ESPEC_MYCLUB §42. gp_logros con 231=800 y 237=240. Pendiente: abandono vs COM (en curso), devolver a Messi (convertido por error), bit del logro de convertir (600), USE_MYCLUB_STAFF, ITEMLIST, subastas.

## 05-nov 10:40 — Noche sola: todo lo que faltaba en el servidor (leer primero)
- **Probado en vivo antes de dormir:** menú del jugador completo, convertir a un titular sin hueco (§43), vs COM con contratos OK. La pausa de vs COM no tiene «Salir»: así es el juego original, se deja.
- **Hecho en el servidor, con pruebas (75 OK), sin probar en PES:**
  - Dificultad de la COM por rachas (§44).
  - Contrato en la unidad real (partidos x 90) con migración automática y gasto por partido (§45, §47).
  - Reglas en caliente desde `reglas_myclub.json` (§46).
  - Entrenador de experiencia, Boleto de contrato y recuperar cansancio (§48).
  - Subastas de cazatalentos de la casa (§49).
  - `admin_myclub.py dar-boletos`.
- **La PC todavía corre el código viejo (4b8a9b4).** Para instalar el nuevo: con PES CERRADO, `git pull` en la carpeta del servidor y reiniciar el servidor. La migración de contratos corre sola la primera vez que PES pide el club. Copiar al config real las claves nuevas de config.ejemplo.json (staff_exp_factor, recuperar_monedas, subastas) si se quieren cambiar.
- **Prueba en vivo sugerida (en este orden):** 1) Plantilla: los contratos se ven bien (ej. «10»). 2) Un partido vs COM: el contrato baja 1. 3) Entrenamiento: usar un entrenador de experiencia. 4) `dar-boletos` y usar el Boleto. 5) Casa de subastas: ver 6 cazatalentos y pujar.
- **Sin resolver:** bits de habilidades de Player.bin (media/rareza reales), texto de item_type de los boletos, escala del cansancio, partido entre 2 personas (relé), identidad propia y Supabase (162 sin aplicar, necesita el SÍ de Frank).

## 06-nov madrugada (hecho en la PC con PES abierto, vía MCP phoenix-pes + capturas)
- **Despliegue en la PC**: servidor reiniciado con `diag_gamerelay.py` (logs `servidor_gamerelay_05nov_z14..z19.log`); código instalado hasta `b000a91`; `config.json` con `reglas_myclub` (30 s) y `reglas_myclub.json` ({} por defecto). Copias `*.antes_<commit>` de los archivos reemplazados. Pruebas 76 OK.
- **Contratos** (§39): `contract_term` se divide entre 90; tras la migración ×90 los jugadores se ven «Contrato restante» 10 (defecto) o 50 (migrados a 4500), sin «!».
- **Capturas fiables**: el MCP captura 1536×864 de una pantalla de 1920×1080 (DPI). Script `cs.ps1` del scratchpad (SetProcessDPIAware + CopyFromScreen 0,0,1920,1080) da la imagen completa. Teclas (de `settings.dat`): LB = A (la pestaña da la vuelta: 3×A desde Partido → Contrato), RT = W, botón X del pad = tecla C (abre «Menú del jugador» en Dirección plantilla), Y/RB sin resolver. Esc = B salvo en la pantalla de asignación de mandos (sin «Atrás»; no entrar a la pestaña Partido).
- **Casa de subastas**: 6 cazatalentos a 3,000 GP; oferta mínima 4,000 GP funciona (TENDER + DETAIL sin error); saldo −4,000 +300 logro. Faltan Mis pujas, Lista de interés, Ranking.
- **Entrenador de experiencia**: convertir (CREATE_MYCLUB_STAFF), contador en el menú (db50587), usar en un jugador (USE_MYCLUB_STAFF) funcionan; +600 GP de logro. Bug abierto: la carta de «Resultados del entrenamiento» sale «PT 40» con todo 0; la petición trae `ability` de 6 enteros y `use_item_list` SMALL/MEDIUM/LARGE; el `member_list` de SET_MYCLUB_SQUAD_GAMEPLAYER no trae `parameters` (log: «parameters de ejemplo (uid 12): None»).
- **Boleto de contrato ×0**: Recuperar sí cuenta (RECOVERY/RECOVERY); Boleto no con CONTRACT/CONTRACT, CONTRACT/NORMAL, EXTEND_PLAYER_CONTRACT ni item_effect_arg 10. `extend_ticket` solo está en el lector de EXTEND_ITEM y en dos menús (0x140c77312, 0x140e01e92). Pendiente.
- **Player.bin del base**: habilidades de 6 bits (valor − 40), tabla de bits en `RE_PLAYERBIN_HABILIDADES.md`; catálogo regenerado con `rating_estimado` (exacto en porteros/defensas, −2 a −5 en delanteros/medios).
- **RE de la noche** en `RE_NOCHE_05NOV.md` (niveles de la COM, USE_STAFF, objetos, subastas, bit del logro de convertir = 244).
- Sin salida/abandono en la pausa de vs COM (decisión de Frank: dejarlo como el original).


## 10-nov — Online completo y blindado (leer primero)
- PES del online: SOLO el PES base `C:\PES21\eFootball PES 2021`.
- b7450fe + be92e24: blindaje (identidad provisional por IP con SteamID compartido, IP anunciada «auto» y URLs con `{ip}`,
  capturas con tope, SQLite WAL + respaldo cada 6 h, keepalive, tope por IP, HTTP con espera/tope, arrancador que reabre,
  mantenimiento, `GET /estado`). Instalado en la PC y comprobado (el arrancador reabrió el servidor en 10 s).
- b2a3f7e: `docs/ESPEC_GAMERELAY_FASE23.md` (estudio de solo lectura del relé de 2 jugadores).
- acc9664 + df27d20: relé fases 2 y 3 (partida = sala sintética 700 000 000+n al emparejar; ligado por NTL; lista de 3;
  reenvío 9/10/12/15; GameSeq TCP deduplicado; eco RTT; tope 2048 B); `tools/rival_falso.py`. 88 pruebas OK.
- Auditoría con los 20 riesgos: `docs/AUDITORIA_ONLINE_10NOV.md`.
- Siguiente: prueba con `rival_falso.py` + PES real (Divisiones) → prueba real con un amigo por Tailscale.
