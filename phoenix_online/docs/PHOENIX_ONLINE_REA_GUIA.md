# Phoenix Online — guía de ingeniería inversa del online de PES 2021 (método «sala limpia»)

Fecha: 26-oct-2026 · Hilo «Crear servidor Phoenix Online» · Estado: **plan + lectura de código MIT hecha; REA en la PC aún NO empezado** (falta sesión de Remote Control).

## 1. Qué se quiere lograr
Un servidor propio de PES 2021 (Steam, exe 1.07.02) donde dos jugadores jueguen online nativo con modos Phoenix, como hace PESBUL, pero nuestro y abierto. Se llega entendiendo el protocolo con REA/Ghidra sobre el **cliente** `PES2021.exe` (lo que el exe envía y espera), no copiando a nadie.

## 2. Reglas de sala limpia (no negociables)
- NO abrir, descargar ni descompilar la imagen de PESBUL (`noobsaibot301/pes2021-emulator`): sin licencia. Solo metadatos públicos ya anotados.
- SÍ permitido: código MIT de Nikow5, BSD de juce/fiveserver (como modelo de arquitectura, es otro protocolo), y el exe de Konami que Frank ya posee, analizado en su PC.
- Descargas solo desde github.com oficial y con permiso de Frank. Ojo con dominios falsos (duckdns) que circulan en foros.
- Nada de dinero real. Solo lectura en la PC mientras el hilo street use PES.

## 3. Lo que ya sabemos (resumen verificado leyendo el código de Nikow5, 1012 líneas)
Fuente: `github.com/Nikow5/Pes2021PrivateServer` (MIT). Es un mapa por prueba y error; llega a login/lista/EULA/matchmaking inicial, **sin lobby ni partido**.

**Transporte «NclMio» (TCP 10000)**
- Cabecera de 8 bytes, big-endian: `[cmd:2][largo:2][secuencia:4]` + datos.
- Todo el paquete va con XOR cíclico, clave `5B 9F 2E 64` (se aplica igual en ambos sentidos).
- Regla observada: el cliente deduce el comando como `largo_de_datos XOR 0x2E64`. Ejemplo: 96 B → 0x2E04; 144 B → 0x2EF4. Por eso **el tamaño de la respuesta importa**: 64 B daba 0x2E24 y el cliente fallaba con `C_IDGG_0002`.
- La respuesta repite la secuencia (`seq`) del paquete que contesta.

**Comandos vistos**
| Cmd | Nombre (del cliente) | Petición | Respuesta |
|---|---|---|---|
| 0x2EE0 | CmdLobbyInit | ticket de Steam, ~124 B | → se contesta con 0x2E04 |
| 0x2E04 | CmdMatchmaking/Auth | token 8 B | 96 B: byte +0x20 = 1 (AuthStatus), +0x21 = 1 (Ready), +0x24 = IP pública (4 B); primeros 8 B espejo del token |
| 0x2EE4 | CmdGetSvrList | <64 B, IP de STUN en bytes 4–8 | 4 entradas de 32 B, **comprimidas con zlib** |
| 0x2EF4 | CmdGetEulaAreaList | ≥140 B | 144 B: región ASCII en +0x10 («FRA»), versión EULA en +0x14; primeros 8 B espejo del token |

Entrada de servidor (32 B): vtable `0x14285B5B0` (8 B little-endian: `B0 5B 85 42 01 00 00 00`), id (4 B BE), tipo (4 B BE), IP (4 B), puerto (2 B **little-endian**), relleno.
Tercer servicio: STUN estándar RFC 5389 con cookie `0x2112A442`, UDP 5730–5739/3478; partido P2P por UDP 5739/5740. HTTP/Flask en :5000 para datos MyClub (sin implementar de fondo).

**Debilidades del código de Nikow5 (ya detectadas, no repetirlas)**
1. Reconoce comandos por **tamaño del paquete** (heurística), no por el campo cmd real: frágil. Primer trabajo de REA: confirmar si el cmd va en el header o se deriva del largo.
2. Un `recv()` = un paquete: no maneja paquetes partidos/pegados en TCP. Hay que hacer framing por `largo`.
3. La validación del ticket de Steam depende de la API de Steam (clave de Valve); Phoenix usará login de la cuenta Phoenix (Supabase), nunca contraseñas propias.
4. Direcciones del exe (1.07.02): `0x1411AEEC0` validador de paquetes · `0x140FE0C50` estado 5 · `0x140FE1FE0` estado 6 · `0x140FF3EC0` estado 10. Hay que **verificarlas en nuestro exe** (hash del exe primero).

## 4. fiveserver (BSD) — para qué sirve
Servidor completo de PES5/WE9/PES6 en Python/Twisted (MySQL, panel admin, API REST, 2v2 en PES6). **Protocolo distinto** (era PES6, no NclMio), así que no sirve para los bytes. Sí sirve como **modelo de arquitectura**: separar `protocol/` (paquetes), `logic.py` (salas/lobby), `model/` (perfiles, stats), `rating.py` (ranking), `web/` (panel). Lo copiaremos como diseño, con atribución BSD si se reutiliza algo.

## 5. Plan de REA (en orden; cada paso es de solo lectura salvo que se diga)
**R0 — Identificar el exe.** SHA-256 de `PES2021.exe`, versión (1.07.02), si tiene Denuvo/packer. Confirmar que Ghidra abre el mismo exe que usa Frank (con el proyecto completo ya existente).
**R1 — Anclas por cadenas.** Buscar en Ghidra: `konami.net`, `pes21-x64-gate`, `stun`, `C_IDGG_`, `NclMio`, `CmdGetSvrList`, `Lobby`. Los nombres de comando salen en cadenas de depuración → mapa de comandos casi gratis.
**R2 — Capa de paquetes.** Llegar a `0x1411AEEC0` (validador). Documentar: formato exacto de cabecera, de dónde sale el cmd, cálculo de XOR/clave (¿fija o por sesión?), checksum de `seq`, zlib.
**R3 — Máquina de estados de conexión.** Estados 5/6/8/9/10 → ¿cuántos hay hasta «en lobby», «en sala», «partido»? Tabla estado → comando enviado → comando esperado. Esto es lo que a Nikow5 le falta.
**R4 — Captura real sin servidor propio.** Con MCP phoenix-pes / Sider (Lua) o Frida solo en la PC de Frank y su propio PES: registrar el tráfico que el cliente **intenta** enviar (se ve el primer paquete en claro antes de cifrar). Sin tocar servidores de Konami (ya cerrados).
**R5 — Lobby y sala.** Seguir desde el estado «menú online» → crear/entrar sala (`menu::MOMSideSelect`, `TaskSyncSelectSideInMulti`, `sideMatchMembers.bin`, ya en cap_62 §5). Qué comando crea una sala, cuáles lista, qué estructura de miembro (id, nombre, equipo, lado).
**R6 — Negociación P2P.** Intercambio de IP/puerto por STUN, handshake UDP 5739/5740, y si hay relé. Anotar qué viaja en el partido (¿solo botones? ¿cuadro a cuadro?).
**R7 — Servidor mínimo propio** (Python o Go): framing correcto, tabla de comandos por cmd real, lobby con 2 jugadores, amistoso 1v1. Primer hito: **dos PES llegan a «Jugar partido» sin desconexión.**
**R8 — Modos Phoenix.** Hash del modo/Core en el emparejamiento; reglas deterministas (ver cap_70 §11.2).

## 6. Qué necesito de Frank (cuando toque)
1. Una sesión de Remote Control en su PC para R0–R2 (solo lectura; sin tocar PES si lo está usando el hilo street).
2. Permiso explícito para clonar `Nikow5/Pes2021PrivateServer` en la PC (ya está leído en el contenedor; para probar en local).
3. Más adelante, una segunda PC o un amigo para probar 1v1 real.

## 7. Coordinación y riesgos
- «Mapear el Fox Engine de PES»: puede cubrir parte de R1–R5; avisarle antes de duplicar (los menús online ya están en cap_62 §5).
- Riesgo legal: zona gris (emular servicios de Konami usando solo el cliente del jugador); sin cobros ni apuestas; consulta legal antes de monetizar.
- Riesgo técnico: desincronización con modos no deterministas; NAT; el relé de PESBUL sugiere que el P2P puro puede no bastar.
- Seguridad: cuentas con login Supabase, sin secretos en repos públicos.

## 8. Bitácora
- 26-oct 19:25 UTC — Hilo creado. Releídos cap_62 y cap_70 §11. Clonados en el contenedor (no en la PC) Nikow5 (MIT) y fiveserver (BSD); leído todo `server_tcp/` de Nikow5 (arriba). Sin acceso a la PC todavía.
- 28-oct 20:45 UTC — Recibida especificación v1 de sala limpia (cap_72 §11, hilo «PES · Plan de Phoenix Core»). Corrige a Nikow5: cabecera real de 24 B con MD5, Blowfish ECB con clave de 56 B del exe, cuerpo MessagePack con `msgid` en texto (no opcodes por tamaño).
- 28-oct 21:00 UTC — Servidor propio v0.1 escrito en `pes-online/servidor/` (ver su LEEME): trama, ConnectGate, login, salas 1v1, TURN; 13 pruebas en verde con clave falsa. Sesión en la PC extrayendo la clave y la lista CMD_* (solo lectura).

## 9. Investigación pública de PESBUL (28-oct-2026, búsquedas web)
Fuentes: hilos de evoweb.uk (106068 «PES 2021 Private Server / Online Emulator», 100632 «Building PES 2021 Team Play Lobby»), pes2021.pesbul.eu, guía de pesmodding.com (jul-2026). Desde la nube solo se ven los resúmenes de búsqueda: los tres sitios están bloqueados por la red del contenedor. Todo es información pública que publican ellos, no código.

### 9.1 Historia
- **NoobSaibot888** empezó hace ~1,5 años con «Team Play Lobby». Usó Wireshark para ver qué dominios y APIs llamaba el juego, los redirigió a su PC y armó el servidor con ayuda de IA. Él mismo lo describe como lento y con mucho error.
- Después tomó como base **Nikow5** (MIT) y le añadió Docker, panel web, inspector de paquetes y exportes de depuración. Dijo que publicaría el código en GitHub «cuando esté estable», y hasta hoy no lo ha hecho.
- **Bloqueos que contaron:** el cliente cerraba la conexión ~50 ms después de la respuesta de autenticación, porque la lista de servidores tenía mal el formato. Luego quedaron atascados en el paso entre la autenticación y el lobby. Las dos cosas coinciden con lo que ya cubre la especificación (MessagePack, binarios anidados).
- **Alfa → beta pública:** 17-ago-2026, en un VPS nuevo de 8 núcleos, 16 GB de RAM, 160 GB NVMe y 2 Gbps.

### 9.2 Cómo se conecta un jugador
- Descarga un **.BAT («PESBUL Connector»)** de su web, lo ejecuta como administrador y elige la opción 1 para instalar o la 2 para quitarlo y restaurar.
- Cuando cambian de servidor, **obligan a reinstalar el conector**, porque la IP queda escrita dentro del .BAT.
- No se publica qué toca exactamente (hosts o DNS). P: casi seguro el archivo hosts.

### 9.3 Qué funciona y qué no (según ellos)
- **Funciona:** solo el amistoso 1v1. Tienen ~13 jugadores conectados en la foto de su portada.
- **Team Play 11v11:** llega a la fase segura de GameRelay pero todavía no a la elección de lados. Ellos mismos no lo consideran funcional.
- **Co-op y clanes:** se pueden crear clanes y salas de co-op de clan. Aceptar solicitudes desde el juego no funciona y el partido de co-op no arranca.
- **Casi todo lo que no es el 1v1 depende de GameRelay**, el relé de Konami que ellos están imitando.
- **Lag:** el partido es P2P, así que el lag depende de la conexión entre los dos jugadores. Ellos piden no culpar al servidor.
- **Arreglo «1v1 Disconnect and Session Cleanup»:** tuvieron 40 intentos que terminaron en `DISCONN_P2P` antes del saque. Además, una limpieza tardía de la conexión vieja borraba de la sala la sesión nueva del jugador. Lo arreglaron solo en el servidor.

### 9.4 Cuentas y web
- **Se puede jugar sin cuenta web.** La cuenta «verificada» es opcional y sirve para perfil público, estadísticas, ranking y torneos.
- **Verificación:** la web da un código y el jugador lo pega en el campo «Equipo favorito» de su perfil. Es un truco para unir la cuenta web con la del juego.
- **Planeado y no hecho:** temporadas ELO, ligas, copas y torneos.

### 9.5 Cómo hacerlo MEJOR en Phoenix (ventajas que ya tenemos)
1. **Cuenta obligatoria y vinculada desde el inicio.** Decisión de Frank. Se vincula el SteamID con la cuenta Phoenix desde Link o la web, sin el truco del «Equipo favorito». Así no hay anónimos y los baneos son por SteamID. Ya está en el servidor v0.1 (`vinculos`, cerrado por defecto).
2. **Link como conector.** En vez de un .BAT, un interruptor en Link con respaldo y restauración automática. Que no escriba la IP: debe usar el dominio de Phoenix (DNS), para que cambiar de servidor no obligue a reinstalar.
3. **Liga, ELO y torneos desde el día 1.** La web ya tiene ligas, tablas y economía. El servidor guarda el resultado de los dos lados y solo lo confirma si coinciden, lo que es anti-trampas natural.
4. **TURN como respaldo desde el principio.** Ataca de frente su principal falla (`DISCONN_P2P` y `MXNV_0013`). La especificación ya lo incluye.
5. **Huella de parche.** Antes de emparejar se exige la misma versión de PES, datos en vivo y parche Phoenix. Así se evita `SYNC_FAILED`, que ellos sufrieron.
6. **Limpieza de sesión bien hecha.** Al reconectar, la sesión vieja nunca debe borrar a la nueva: es su bug del arreglo de cleanup. En v0.1, `cerrar()` ya solo quita el `pid` si sigue apuntando a esa misma sesión. En la sala también: la sesión nueva hereda el puesto (prueba `test_reconexion_no_la_borra_la_sesion_vieja`).
7. **Modos Phoenix.** Modos propios con Phoenix Core en los dos lados, verificados por hash, y salas filtradas por modo. Esto PESBUL no lo tiene.
8. **Panel y registros.** Captura JSONL por sesión, ya en v0.1, más un panel en la web.

### 9.6 Pendiente de esta investigación
- **Leer los hilos completos de evoweb y la web de PESBUL:** desde la nube están bloqueados. Opciones:
  - Que Frank permita esos dominios en la configuración de red del entorno.
  - Que la sesión de su PC los lea como texto.
- **Videos de YouTube sobre PESBUL:** no aparecieron. Los que salen son anteriores a PESBUL.

## 10. Resultados R0–R1 en la PC (28-oct-2026, solo lectura)
- Exe Conmegol = original (458.806.784 B, SHA-256 5e27a782…c224a), cadenas 1.07.01/1.07.02; protegido (secciones .xtext/.xcode, tipo Denuvo/VMProtect) pero red y textos legibles. Proyecto Ghidra: `PRUEBA_CANCHA\copias\ghidra_proyecto_completo\PEStrim`.
- Dominios: `pes21-x64-gate.cs.konami.net` (0x142880AA0), `pes21-x64-stun.cs.konami.net` (0x142880AF0), `info.service.konami.net/XME994-E1/info/` (0x142880AC0), GateInfo.php ntl/ntljp/ntleu/ntlus (0x142A64098…). Clases NclMio, NclCryptBlowfish, NclDaemon.
- **Trama confirmada (C):** clave Blowfish 56 B en .data 0x1434D61C0 (hash coincide; archivo solo en la PC); Blowfish ECB con cada mitad de 4 B invertida (= `blowfish_le`); XOR desde byte 0 sobre toda la trama; MD5 sobre cabecera 8 B + carga.
- 53 tipos de URL y 439 textos CMD_* en `D:\Frank\IA\CLAUDE\pes-online\cmd_lista.txt`.
- Las direcciones de Nikow5 (0x1411AEEC0, etc.) NO valen en este exe; `C_IDGG_` y `CmdLobbyInit` no existen como texto.
- Lectura pública de PESBUL guardada en `D:\Frank\IA\CLAUDE\pes-online\pesbul_publico.md` (evoweb págs. 1–3, portada, registro, pesmodding).

## 11. Huecos cerrados (28-oct, sesión PC; detalle en `D:\Frank\IA\CLAUDE\pes-online\huecos_servidor.md`)
- C: orden de ConnectGate = CmdGetUrlList → CmdGetSvrVersion → CmdGetSvrTime → CmdGetEulaNationList (FUN_140fddef0) → CmdGetSvrList. `CMD_GET_CHALLENGECODE` existe (P: justo antes del login). `CMD_SEND_PSID` lleva `psid_high`/`psid_low`. P: Steam se valida por la URL `AUTH_STEAM`, no por ConnectGate.
- C: `CMD_GET_SVRVERSION` (FUN_141d4af80) lee los campos como **texto**; valores esperados aún sin resolver (existen `ERR_DIFFERENT_*_VERSION`).
- C: `pname` = objeto {name, xuid_one, psn_user_id, account_id, online_id, is_valid_cache, is_deleted}. `date` = entero (P: Unix; el exe suma 9 h JST).
- C: `CMD_WATCH_ENTRY_GAME` lee `entry_count` y `entrygame` [{entryNum, gameNum}]. `CMD_WATCH_DECIDE_GAMEENV` no existe como texto CMD_ (solo la clase MOMLobbyCmdWatchDecideGameenv): revisar en la prueba.
- C: éxito = "NOERR"; rechazos = 189 textos `ERR_*` (ERR_LOGIN_FAILED, ERR_ROOMISFULL, ERR_ROOMNOTFOUND, ERR_SERVERNOTREADY, ERR_DIFFERENT_PATCHVERSION…). Aplicados ya en el servidor.
- ⚠ CONTRADICCIÓN a resolver en la primera prueba: la sesión PC ve plantillas «tipo JSON» en el exe; la spec (cap_72) dice MessagePack. El registro JSONL de la prueba lo aclarará; si fuera JSON, cambiar solo el serializador en `nclmio.py`.
- P (tipos de URL obligatorios): GET_TURN_SERVER_LIST, GET_GAMERELAY, TOPINFO, BANNER, EULA_PES, AUTH_STEAM, ONLINEID → ya en config.ejemplo.
- Idea mejor que PESBUL (P): redirigir dentro del proceso del juego con un módulo Sider/Phoenix Core en vez de tocar hosts/DNS del sistema → sin permisos de administrador y sin dejar cambios en la PC.

## 12. Huecos de la pasada final (29-oct, sesión PC)
- C: el cliente NO compara versiones con constantes: `ERR_DIFFERENT_*VERSION` lo decide el servidor. Las 9 versiones pueden ser cualquier texto no vacío → Phoenix puede usarlas para exigir su propio parche.
- Abierto: qué tipos de URL son obligatorios → enviar los 53 apuntando a nuestro servidor y recortar después.
- C: HTTP por **WinHTTP** (agente «WinningEleven/16.0»), con la validación de certificados normal de Windows (sin pinning). Hay 4 URLs GateInfo en http:// y 4 en https://. **Lo primero que pide el juego es `ntl.service.konami.net/ntl/api/GateInfo.php`** (443, con respaldo en 80).
- ⚠ Instalar una CA propia como raíz de confianza es tocar un ajuste de seguridad de Windows (límite fijo): NO se hace. Caminos: (a) que funcione el respaldo http:// (puerto 80; el servidor ya escucha ahí y lo registra todo), (b) gancho dentro del proceso (WinHttpOpen/WinHttpOpenRequest sin la bandera SECURE, getaddrinfo) desde Sider/Phoenix Core, sin administrador ni cambios en la PC → camino recomendado para el producto.
- P: ConnectGate = MessagePack (clase NclMsgPackMessage); el HTTP usa JSON o formulario.
- DLLs de red resueltas en ejecución: ws2_32, winhttp, wininet, crypt32, secur32.
- Herramienta: REA agotó el plazo dos veces con el exe de 60 MB; se pasó a `analyzeHeadless` sobre PEStrim (segundos).
- C (29-oct): la tabla de 53 tipos de URL solo la leen pantallas del menú (BANNER, P2PMATCH, MYCLUB_AGENT_BANNER, DMM_TOP, LIVEEVENT_BANNER, WEEKLY_INFO_BANNER, WATCHING_GAME). ConnectGate no exige ningún tipo: si falta uno, solo falla esa función. `ERR_CLIENT_SENDFAIL` no sale de las URLs. Los tipos de red (EULA_PES, TOPINFO, AUTH_STEAM, ONLINEID, GAMERELAY, TURN) se piden por índice variable. El config de la PC devuelve los 53 tipos apuntando a 127.0.0.1:5000.

## 13. Cruce de los informes de Gemini con lo comprobado (29-oct-2026)
Informes guardados en `pes-online/gemini/` (01 acotado, 02 investigación maestra de 85 ideas, 03 «revivir online en tu parche»). Son investigación web de terceros: cada dato se marca según lo que ya comprobamos en el exe.

### 13.1 Lo que SIRVE y ya se aplicó o se aplicará
- **Flujo del gate (P, informe 02):** CMD_GET_SVRLIST → `CMD_DISCONNECT {logoutstat:"MOV"}` → el cliente salta al servidor MENU. **Aplicado** en el servidor (commit fc17c02): responde NOERR y cierra esa conexión.
- **Más dominios a redirigir (P):** pesam.stun.service.konami.net, info.service.konami.net, cs.konami.net, ntl.service.konami.net, ntleu.service.konami.net, pes21-pc.konamionline.com. Se suman a ACTIVAR y a la futura redirección dentro del proceso.
- **Bugs que PESBUL ya sufrió (C según ellos), para no repetirlos:**
  - el anfitrión quedaba «Listo» solo al entrar el invitado (en el nuestro, `entry` vuelve a False al unirse);
  - salas fantasma tras una desconexión (`cerrar` + reconexión ya cubiertos);
  - «Connection lost» tras crear el perfil, porque faltaba una confirmación de cierre del perfil (pendiente: lo dirá la captura);
  - cámara lenta en WAN hasta que el balón sale (pendiente, fase de partido).
- **netblock.lua:** muchos sider.ini de parches sudamericanos lo traen y mata el online. Hay que revisar el de Conmegol antes de la prueba (solo leer; el sider.ini de Conmegol no se edita a mano).
- **Qué debe ser idéntico entre los dos jugadores (P, informe 03):** base de datos, EDIT y física/exe sí; caras, kits, estadios y audio no. Encaja con la «huella» del servidor: hay que medir esos archivos, no todo el parche.
- **Dos métodos que se complementan (informe 03):**
  - nativo 1v1: secreto táctico, cámara propia y sin video;
  - Parsec: 2v2+, espectadores y PCs modestas.
  - Mismo lector de resultados (`phoenix.lua`) para los dos.
- **Ideas fuertes que ya encajan con lo nuestro:**
  - redirección y auto-login dentro del proceso (Core), sin .BAT ni «Equipo favorito»;
  - actas por consenso con una línea de tiempo del marcador, para que el servidor detecte un 3-2 inventado;
  - OpenSkill (Weng-Lin, sin patentes) para el ranking;
  - TURN obligatorio en torneos, que oculta la IP contra DDoS;
  - dominio propio (gate.phoenix…) para cambiar de VPS sin reinstalar nada.

### 13.2 Lo que NO es cierto o está exagerado (corregir antes de usarlo)
- «439 comandos mapeados en Python con TURN funcionando»: falso. Hay 439 **textos** CMD_ listados del exe; el servidor maneja unos 40 y el TURN está programado, pero nunca se ha probado.
- «Opcodes 0x2EE4/0x2EF4/0x2E04 en la cabecera»: viene de las heurísticas de Nikow5. En nuestro exe la cabecera es tipo + largo + secuencia + MD5, y el comando va por nombre dentro de MessagePack.
- Direcciones de función (FUN_140ffb330, FUN_140fdf300…): son de un exe ajeno. Nikow5 tampoco coincidía con el nuestro, así que hay que verificarlas antes de usarlas.
- Lo legal (DMCA 1201(f), Ley 29733) es orientativo, no asesoría: la regla vigente sigue siendo abogado antes de cobrar.

### 13.3 Hallazgos para OTROS hilos (avisados al coordinador)
- **Link (seguridad):** Smash Soda puede dar ratón y teclado del anfitrión a los invitados. Hay que confirmar que en Phoenix Link esté apagado o eliminado, y limitar la entrada a mandos ViGEm.
- **Web:** OpenSkill para el ranking. Reglamento de la Ley 29733 (D.S. 016-2024-JUS): política de privacidad, aviso de incidentes y RLS.
- **Core/street:** «Phoenix Street online» exige reglas deterministas con hash. La cámara lenta en WAN se investigaría con x64dbg y un breakpoint de escritura en el icono de carga.

### 13.4 Falla honesta del servidor actual
- **Riesgo de suplantación:** el servidor acepta el SteamID que el propio cliente declara en `username`. Cualquiera que lo copie podría hacerse pasar por otro. Antes de abrirlo a otros jugadores hay que validar el ticket de Steam (URL `AUTH_STEAM` con la Web API de Steam, cuya clave va en una variable de entorno) o usar el auto-login de Core con un token de la web.

## 14. REA con GhidraMCP (29-oct-2026, proyecto PES_MCP_RECORTADO, solo lectura)
Detalle completo en la PC: `D:\Frank\IA\CLAUDE\pes-online\PHOENIX_ONLINE_REA_GUIA_bitacora.md` §14 y en cap_62.
- **A. Redirección (Core, rama `claude/online-phoenix-core`, commits 2781290 + c8e3d9f):** el protector copia una vez cada puntero de la IAT real a una tabla del juego en .rdata. Por eso el hook cambia por valor TODA casilla con la dirección real. Cubre getaddrinfo, el getaddrinfo tardío de Wspiapi (vía GetProcAddress), gethostbyname, WinHttpConnect, connect, sendto y WSASendTo. Las 4 IP fijas de Konami (AWS Tokio) también se redirigen. No hay DnsQuery ni GetAddrInfoW en el exe.
- **Override nativo de servidor (descartado):** la tabla de 6 archivos marcador (QACheckGate.bin, QCGate.bin, RatingCheckGate.bin, EnzymeCheckGate.bin, ReviewCodeGate.bin, OnlineModeCheck.bin) solo apunta a destinos fijos de QA de Konami. No sirve para nuestro servidor.
- **B. Login real (OnlineLoginTaskConnectAccount, fn 140fdb320):** CMD_AUTH_STEAM va por **HTTP** (CommandObjectHttp, URL tipo AUTH_STEAM; lleva el ticket de GetAuthSessionTicket en hex y el SteamID64 en claro) → CMD_GET_CHALLENGECODE → CMD_LOGIN_ACCOUNT (ERR_LOBBYUSERMAX hace probar el siguiente servidor; ERR_LOGIN_FAILED es fatal) → CMD_SEND_PSID («estado 8» de PESBUL) → CMD_GET_PLAYER_DATA (si has_data no está, vuelve a crear perfil en bucle).
- **Identidad:** la copia de Frank usa el shim CPY (steam_api64 sin firma, más CreamAPI). El SteamID sale de CPY.ini (239200 por defecto, el mismo en todas las copias) y el ticket es falso. **El SteamID no sirve como identidad.** Plan: un «pase» de la web que Phoenix Link escribe en la PC y el Core pone en lugar del ticket o del SteamID. El servidor lo valida contra Supabase (migración ≥162).
- **C. Perfil:** CreatePlayer NOERR + GetPlayerData con has_data → sale del bucle.
- **D. P2P:** puertos UDP locales 5730‥5739 (rotan). Ip/puerto/NAT por CMD_SEND_IPANDPORT_NTL; relé TURN por SEND/WATCH_TURN_ADDRESS_DATA. CMD_CHECK_LOCAL_GAMERELAY_PLAYER: claves leídas (fn 141b9cdc0).
- **Tipos de respuesta (lectores del cliente):** muchas claves que parecen números o bool se leen como **CADENA**. Tabla:
  - GET_EULA_NATION_LIST: elementos `nation_code`, `privacy_kind`, `has_area`, `name` (todo cadena), dentro de un array de bin.
  - GET_EULA_NATION_CODE: `nation_code`, `area_code` (cadena).
  - GET_CHALLENGECODE: `challenge_code` (cadena).
  - LOGIN_ACCOUNT y SEND_PSID: solo msgid, rqid y result.
  - GET_PLAYER_DATA: `has_data` (cadena), `pid` (número), `account_id` (número). `pname` es un bin con `name`, `online_id`, `is_valid_cache` e `is_deleted`, todo cadena.
  - Regla: si el cliente espera cadena y recibe otro tipo, lee basura o vacío.
- **Petición LOGIN_ACCOUNT (claves del cliente):** msgid, rqid, svrtype, client, np_account_id, username, patch_version, dlc_version, extradata_version, livedata_version, package_type, platform, net_link_type, pairwise_id.

## 15. Primera prueba real (29-oct-2026, 09:49–10:24 UTC; PES de Conmegol abierto SIN Sider + hosts temporal)
- **Sin Sider vuelve el botón online** (icono «EF» del menú principal). Conmegol, vía `livecpk\Menu`, reutiliza las casillas online de TopMenuNew.json. Sin Sider no carga el Core, así que esta prueba solo podía usar hosts.
- **Funcionó:** el hosts redirige y PES abre TCP a nuestro ConnectGate (10000).
- **Arreglo 1:** el descifrado necesita `blowfish_le: false` (palabras big-endian). Confirmado con la primera trama real (152 B, CMD_GET_SVRLIST) y la herramienta `probar_variantes.py`.
- **Orden real del gate:** GET_SVRLIST (lang ANY, patch 1.07.02, dlc 7.00) → GET_URLLIST (lang SP, REGION_EU) → GET_SVRVERSION → GET_SVRTIME → GET_EULA_NATION_LIST → GET_EULA_NATION_CODE → DISCONNECT MOV. Después, una 2.ª conexión con GET_URLLIST → GET_GLOBALINFOTITLELIST → DISCONNECT MOV.
- **Arreglo 2:** 53 URLs no caben en una trama (1128 B > 0x410). En la PC se recortaron a 14. **Pendiente:** el envío segmentado (NclMio::DivBufForEasyEncryptionVer).
- **Arreglo 3:** GET_SVRLIST debe incluir un servidor **ACCOUNT** además de MENU. Sin él, ConnectAccount da error fatal sin abrir red.
- **Arreglo 4:** los datos de la nación deben ser cadenas. Con eso aparece la pantalla **«País/Región: Peru» servida por nuestro servidor** (primer contacto visible).
- **Bloqueo abierto:** al confirmar Peru sale «Se ha producido un error.» (sin código) y vuelve al menú, sin más red. El código apunta a fn 140fe2920 / 140fe2fb0 (diálogo AreaSelectAgreementDialog, error 4). En estudio: valores de privacy_kind y has_area, y el recurso de interfaz.
- **Commits del servidor (rama `claude/project-thread-9ciw66`):** 972d383, 5907c14, eec3f68, 82082fe, e7e43ff, 6339108, 33f41b3, adcc2d1 (22 pruebas OK).
- **Seguridad:** la clave NclMio sigue solo en la PC. Al terminar se restaura hosts con RESTAURAR_HOSTS.ps1.
