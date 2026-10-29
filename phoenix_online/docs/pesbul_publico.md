# PESBUL 2021 — resumen técnico público (2026-10-28)
Fuentes (solo texto, nada descargado ni ejecutado): [E1] https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/ (págs. 1–3 leídas; la 3 llega a 30-jul-2026, no confirmé si hay más), [E2] https://evoweb.uk/threads/building-pes-2021-team-play-lobby-online-server.100632/, [P1] https://pes2021.pesbul.eu/ (portada) y /registration/, [M1] https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html. No pude abrir páginas de reglas ni noticias sueltas (solo vi los titulares de la portada). C = dicho en la fuente; P = deducido por mí.

## Conector en la PC
- C [M1, jul-2026] Es un .BAT que se baja del sitio ("Connect Now") y se ejecuta como administrador; opción 1 instala/activa, opción 2 quita la configuración y restaura. Reversible.
- C [E1, 29-jul-2026] Un jugador reinstaló el .BAT y sigue con "Server Ping: Unavailable"; no hay arreglo publicado en lo leído.
- P Ninguna fuente dice qué toca el .BAT. Por lo que redirigen (dominios de Konami → su servidor) lo más probable es archivo hosts y/o DNS, y quizá un certificado raíz. NO comprobado: no se descargó. Para saberlo habría que leer el .BAT como texto con permiso de Frank.

## Dominios
- C [E1] Redirigen los de Konami: gate (ConnectGate), EULA, auth, y el extra pesam.stun.service.konami.net (STUN). El exe trae pes21-x64-gate.cs.konami.net y pes21-x64-stun.cs.konami.net (C, ver bitácora REA). No seguí los enlaces duckdns.
- C [P1, 28-oct-2026] Servidor propio en VPS (8 CPU, 16 GB, 2 Gbps), sin túnel doméstico; el sitio es WordPress.

## Estado técnico del emulador
- C [E1] Superaron DNS, ConnectGate, EULA y auth; el servidor controla pantallas reales (país/región, Online Information, lista de servidores) con el protocolo cifrado original.
- C [E1/M1, jul-2026] 1v1 (Friendly Match Lobby ~99 %) funciona. Team Play 11v11 NO: llega a GameRelay con intercambio de claves seguro y tráfico cifrado+comprimido; falta Side Select y empezar el partido.
- C [M1] Los partidos son P2P (no pasan por el servidor); el lag viene de distancia/Wi‑Fi/NAT. GameRelay: el servidor entrega un endpoint y el cliente se conecta a él (11v11) [E1].

## Errores de jugadores
- "Unable to connect" al iniciar partido desde una sala; luego "Server Ping: Unavailable" aunque reinstale el BAT (E1 p3, 29-jul-2026). "No funciona" sin detalle (p3). En versiones viejas el cliente cortaba justo tras la auth (E1 p1).
- C Piden plantilla 26-27 y tener una plantilla online aparte de la de Sider (E1 p3, jul-2026).

## Cuentas, ELO y ligas
- C [M1] Cuenta web separada; verificar el perfil es opcional (código generado en el panel y puesto en Ajustes → Online → Editar perfil). Verificado = temporadas ELO, ligas, copas, perfil público, historial, logros.
- C [P1, 28-oct-2026] Más de 3000 jugadores en la BD; PESBUL League 4.0, EvoWeb Cup 2026 (https://pes2021.pesbul.eu/league/?competition=15), clanes y co‑op en beta; rankings y resultados "validados".

# AMPLIACIÓN 2026-10-28 (tarde) — investigación abierta sobre PESBUL
Lo que encontré: no hay vídeos de YouTube ni hilos de Reddit sobre PESBUL (búsqueda sin resultados; no lo doy por inexistente). Las fuentes reales son 5: el hilo de evoweb (versiones "private-server-online-emulator", "open-alpha" y "open-beta", mismas páginas 1–4 con distinto título), el hilo antiguo de evoweb sobre un lobby Team Play, la web pes2021.pesbul.eu (portada, /registration/, /the-next-era-of-pesbul-2021-starts-now/, perfiles públicos de jugador), el artículo de pesmodding, y los repos de GitHub citados. No abrí GitHub, Docker, Discord ni duckdns (no se descarga ni se ejecuta nada).

## Línea de tiempo (C = dicho en la fuente)
- C [evoweb 100632, hilo antiguo, 2025] Un primer autor se rindió: "casi imposible sin código ni capturas de un servidor que funcione". Otro usuario (marqisspes5) redirigió DNS a localhost con servidores node.js y vio 3 hosts: pes21-x64-gate (relay TCP básico), pes21-x64-stun (HTTP/2 según él) y ntl.service.konami.net (HTTPS). Al elegir una opción online el juego hace POST a ntl.service.konami.net/ntl/api/GateInfo.php y luego manda paquetes cifrados a gate. Hay otro repo previo (NoobSaibot888/pes_server, "sin éxito").
- C [evoweb 106068, 2026] El proyecto PESBUL se apoya en un proyecto previo de "Nikow5/Pes2021PrivateServer" (según el propio hilo) y añadió Docker, imagen pública y despliegue en TrueNAS. Hitos: DNS → ConnectGate → EULA → auth (el cliente cerraba la conexión ~40 ms tras la respuesta de auth) → pantallas reales (país, anuncio Online Information, lista de servidores) → 1v1 funcionando → alpha pública en casa (con túnel/relay) → 17-ago-2026 beta pública en VPS.
- C Flujo de login que ellos confirman con Ghidra: ConnectGate → CmdGetUrlList, CmdGetSvrVersion, CmdGetSvrTime, CmdGetEulaNationList (+EulaNationCode/EulaAreaListSaveData) → ConnectAccount → CmdLoginAccount → CmdSendPsid → CmdGetPlayerData → CmdCreatePlayer → CmdSetCurrentPlayer → CmdLoginMenu. Opcodes vistos: 0x2EE4 petición EULA/área, 0x2EF4 respuesta, 0x2E04 auth. Descifran con "XOR/NclMio" (coincide con lo que leí en el exe: XOR 5B 9F 2E 64 + MD5 + Blowfish; ver bitácora REA).
- C [beta] Funciona: salas 1v1 (Friendly Match Lobby), estado Ready de cada jugador, reutilizar sala tras el partido, clanes (crear, ver info) y sala de clan co-op (el partido aún no arranca; falta aceptar solicitudes desde el juego). 11v11: llega a GameRelay con intercambio de claves seguro y tráfico cifrado+comprimido que ya pueden decodificar; falta la respuesta de la capa de aplicación, Side Select y empezar el partido.
- C Infraestructura: antes PC doméstico + túnel/relay; ahora VPS (8 núcleos, 16 GB, 160 GB NVMe, 2 Gbps), BD con >3000 jugadores y sistema de migraciones, panel de administración. Web en WordPress con tema propio (rankings validados, resultados, salas en vivo, "match center").
- C Competiciones: ELO por temporadas, ligas (PESBUL League 4.0), copas (EvoWeb Cup 2026), perfiles públicos, historial; la cuenta de la web es aparte y verificar el perfil con código es opcional.

## Cómo funciona (resumen)
1. Un .BAT (admin) redirige los dominios de Konami a su VPS (P: hosts o DNS local; no comprobado). 2. El juego habla con "ConnectGate" (TCP, tramas NclMio cifradas) y con una API HTTPS (GateInfo.php) y un STUN. 3. Salas/lobby los lleva el servidor; el partido es P2P entre jugadores (lag = conexión de ellos). 4. 11v11 usa un GameRelay (servidor intermedio) con claves propias.

## Errores de jugadores (C, evoweb p3, jul-2026)
"Unable to connect" al empezar partido desde una sala; "Server Ping: Unavailable" aun reinstalando el BAT; "no me funciona". Sin solución publicada. P: causas habituales serían NAT/UPnP, firewall, antivirus bloqueando el BAT, DNS cacheado o hosts no aplicado; no confirmado.

## Qué haría falta para replicarlo (P, mi análisis)
- Pieza A, ya resuelta por nosotros en el exe: trama NclMio (XOR+MD5+Blowfish, clave en el exe) → cmd_lista.txt (439 CMD_ y 53 URL). Falta, por comando, el formato de campos (tarea de huecos_servidor.md).
- Pieza B: servidor ConnectGate (TCP) + API HTTPS GateInfo/URLs + STUN (UDP) + EULA/anuncios; el cliente hace HTTPS, por tanto necesita un certificado que el juego acepte (P: el BAT instala certificado raíz o el exe no valida; hay que mirar en Ghidra cómo valida TLS antes de decidir).
- Pieza C: lobby 1v1 + estados + P2P por STUN. Es lo que ya funciona en PESBUL.
- Pieza D (difícil): GameRelay/11v11 y co-op; ellos tampoco lo tienen.
## Cómo hacerlo mejor (ideas P, para Fralex)
- Instalador sin .BAT opaco: un módulo de Sider/Phoenix que desvíe solo en el proceso (hook en la resolución de nombres) en vez de tocar hosts/DNS del sistema ni instalar certificados; reversible y seguro.
- Integrarlo con Phoenix: cuentas únicas con la web de Fralex (sin otra cuenta aparte), ELO/ligas que lean el resultado del propio juego (eventos del Core) en vez de depender de "validación" externa, y plantillas/rosters Phoenix sincronizados para los dos jugadores.
- Telemetría de calidad de conexión (ping/jitter) en el lobby y emparejamiento por región; reconexión; reglas contra abandono.
- Servidor autoritativo mínimo: guardar resultados firmados por ambos clientes para evitar trampas.
- Licencia/riesgo: Konami cerró los servidores en 2022; los proyectos son no oficiales. No se ha visto una postura de Konami (no confirmado).
