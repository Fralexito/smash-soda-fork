# Búsqueda de recursos públicos para un servidor online propio de PES 2021 (11 nov 2026)

Objetivo: encontrar todo lo público que nos ayude a montar nuestro propio servidor online de eFootball PES 2021 (PC, Steam 1.01.00) y lo relacionado con revivir el online de Konami (PES 2010–2021, eFootball, Winning Eleven, otros juegos de Konami).

Ya conocidos (solo se mencionan si hay algo nuevo): AzrakHQ/pes2021-online-server, PESBUL, Evoweb, pesmodding.com, Sider, Smoke Patch, Football Life.

## Cómo se hizo (y límites)

- Unas 45 búsquedas web (inglés, español, portugués, ruso, chino, árabe, turco, indonesio, polaco) + búsquedas directas de repos y de código en GitHub (`gh search repos` / `gh search code` con los nombres de comandos: `CMD_WATCH_MATCHING`, `CMD_START_MATCHING`, `CMD_GET_MATCHDAY_INFO`, `NclMio`, `GateInfo.php`, `pes21-x64-gate`, `CMD_GET_URLLIST`, `CMD_GET_SVRLIST`, etc.).
- **No se pudieron abrir** (bloqueados por la red de esta sesión): evoweb.uk, pesmodding.com, pes2021.pesbul.eu, patreon.com, web.archive.org, thirteenserver.online, reshax.com, mgsvmoddingwiki.github.io, yangfanbit.github.io. Lo que digo de ellos viene de los resúmenes del buscador, no de leer la página entera. Si se habilitan esos dominios, se puede verificar.
- No se descargó ni ejecutó nada. No se clonó ningún repo (solo se leyeron páginas de GitHub).

**Dato clave nuevo:** la búsqueda de código en GitHub confirma que los nombres `CMD_WATCH_MATCHING`, `CMD_START_MATCHING` y `CMD_GET_MATCHDAY_INFO` **solo aparecen en el repo de AzrakHQ** (en todo GitHub público). Nadie más tiene documentado el protocolo de PES 2021 a ese nivel. Lo demás que existe son "parientes" (otros PES y otros juegos de Konami que usan la misma familia de protocolo).

---

## Hallazgos, de más útil a menos útil

### 1. OpenEleven — servidor de PES 2010/2011/2012/2013 (PC) — MUY ÚTIL
- **URL:** https://github.com/moth1995/open-eleven-server (original) · fork: https://github.com/amir14a/open-eleven-server
- **Licencia:** AGPL-3.0 o posterior. **Actividad:** original 147 commits, actualizado 1 nov 2026; fork 106 commits. Lenguaje C# / .NET 9, con Docker y MySQL.
- **Qué contiene:** servidor completo que reimplementa el online de Konami de los PES 2010–2013. Tiene "gate" (puerta de entrada), lobby, menú, cuenta y chat; salas, partido rápido, resultados, amigos, competiciones; web de registro y panel de administración. Y lo mejor: una carpeta de notas de ingeniería inversa `docs/reverse-engineering/` con:
  - `pes2011-session-protocol.md`: formato de cada paquete (cabecera de 24 bytes: id, largo, contador y un MD5), capa XOR de 4 bytes + Blowfish-ECB con clave fija, cuerpos troceados en paquetes 0x70/0x71/0x72, compresión zlib, y la secuencia completa: gate → cuenta (`CMD_AUTH_GAMEID`, `CMD_LOGIN`) → lobby → sala → partido P2P.
  - `pes2011-cmd-reference.md/.json`: 246 comandos `CMD_*` del exe de PES 2011 con campos. Incluye la familia "WATCH" (el servidor empuja mensajes después de que el cliente se suscribe una vez: `CMD_WATCH_ROOMSTATE`, `CMD_WATCH_IPANDPORT`, `CMD_WATCH_DECIDE_GAMEENV`…), partido rápido (`CMD_START_QUICKMATCH`, `CMD_CANCEL_DECIDE_QUICKMATCH`), divisiones (`CMD_GET_DIVISIONUPDATE`, `ERR_DIVISIONREFUSED`), rankings, ligas de comunidad, fin de partido anormal/abandono, y NAT (`CMD_SEND_IPANDPORT` con ip/puerto externos e internos y tipo de NAT; `CMD_SEND_P2PFAILED`).
  - `pes2010_cmd_commands.md`, `pes2010_cmd_fields.md`.
- **Qué nos da:** es el **antepasado directo** del protocolo de PES 2021: mismos nombres `CMD_*`, misma idea de "WATCH" (suscribirse y recibir empujones), mismo `GateInfo.php` → gate → login. Sirve para entender el porqué de cosas que en PES 2021 vemos sin explicación (reintentos, códigos de error, suscripciones, fin de partido por abandono, flujo de NAT). También como **ejemplo de arquitectura** de un servidor serio: "Hub" que reenvía paquetes entre sesiones, avisos de sala al entrar/salir, límite de intentos de login, logs sin datos sensibles, panel admin con cookies seguras.
- **Ojo:** no tiene relay/TURN (solo P2P directo; dentro de la misma red funciona, entre routers distintos no siempre). AGPL: si copiamos código, nuestro servidor también tendría que publicar su código. Usarlo **como referencia de lectura** no obliga a nada.

### 2. Familia de servidores de Metal Gear Solid V / MGO3 / Metal Gear Survive (motor Fox, mismo "gate" de Konami) — MUY ÚTIL como referencia
Konami usó la misma plataforma de red (gate con `CMD_GET_SVRLIST`, `CMD_GET_URLLIST`, `CMD_GET_SVRTIME`, `CMD_AUTH_STEAMTICKET`, `CMD_REQAUTH_HTTPS`, `CMD_SEND_IPANDPORT`…) en sus juegos del motor Fox. La búsqueda de código `CMD_GET_SVRLIST` devuelve **solo** estos repos + AzrakHQ + OpenEleven.
- **alicealys/tpp-server-emulator** — https://github.com/alicealys/tpp-server-emulator — GPL-3.0, 418 commits, 51 estrellas, C/C++, activo. Reimplementación completa del servidor de MGSV:TPP y de MGO3 (multijugador). Tiene `docs/progress.md` con el estado de cada comando (gate, login con ticket de Steam, sesión, latidos, envío de IP/puerto). **Nos da:** cómo manejan el ticket de Steam, la clave de sesión que entrega el servidor y la sesión HTTPS; un servidor C++ robusto como modelo.
- **alicealys/mgv-server-emulator** — https://github.com/alicealys/mgv-server-emulator — GPL-3.0, 236 commits. Lo mismo para Metal Gear Survive (fork del anterior). Confirma que la plataforma se reusó en varios juegos.
- **alicealys/tpp-mod** — https://github.com/alicealys/tpp-mod — GPL-3.0, 465 commits, 42 estrellas, muy activo (9 nov 2026). Es un `dinput8.dll` que, entre otras cosas, permite apuntar el juego a **un servidor propio** (`net_custom_server`) y trae **"servidor dedicado" experimental para MGO3** (el juego corriendo sin pantalla como anfitrión). **Nos da:** modelo de "lanzador/DLL de redirección" sin tocar el archivo hosts, y la idea de un anfitrión dedicado. Solo soporta copias originales de Steam (bien).
- **unknown321/mgsv_emulator** — https://github.com/unknown321/mgsv_emulator — Python, 79 commits, archivado en 2025 (abandonado). Su README explica **las capas de cifrado de Konami** paso a paso: base64 → Blowfish con clave fija del exe → JSON → base64 → Blowfish con clave de sesión (si `session_crypto`) → zlib (si `compress`). También avisa de que su Blowfish **no es el estándar** (variante propia). **Nos da:** una comparación directa con nuestra capa NclMio; si en PES 2021 vemos algo raro con el cifrado, aquí hay pistas.
- **unknown321/fuse** — https://github.com/unknown321/fuse — servidor maestro de MGSV en Go (temprano). Tiene `sessionmanager/` con respuestas tipadas de `CMD_GET_URLLIST`.
- **35mpded/mgo3-server-emulator** — https://github.com/35mpded/mgo3-server-emulator — AGPL-3.0, Python/Flask; incluye `Blowfish.py`, `Decoder.py` y el `.proto` del ticket de Steam (`SteamAppTicket_pb.proto`). Temprano.
- **mxmstr/NucleusGameFixes** — https://github.com/mxmstr/NucleusGameFixes — tiene comandos `CMD_*` de MGSV en Python (para un co-op).
- **Ojo:** `alicealys/mgo3-rce` (CVE-2026-19874) es un fallo de seguridad de ejecución remota de código en MGO3. **No usar**, pero sirve de lección: los clientes de Konami confían ciegamente en el servidor; nuestro servidor no debe reenviar datos de un jugador a otro sin filtrarlos.

### 3. Nikow5/Pes2021PrivateServer — PES 2021 (v1.07.02) — útil solo como pista
- **URL:** https://github.com/Nikow5/Pes2021PrivateServer (rama `pes2021-server-emulator-1665992455930534835`)
- **Licencia:** MIT. 16 commits, 4 estrellas, actualizado ago 2026. Python.
- **Qué contiene:** servidor TCP "ConnectGate" (dice NclMio + XOR), STUN UDP (5730–5739 / 3478), API HTTP de MyClub; base SQLite; un script **Frida** (`trace.js`) para espiar las conexiones del cliente. El código es básicamente un intento a ciegas: adivina el tipo de mensaje por el **tamaño** del paquete, el ticket de Steam se acepta sin comprobar, respuestas fijas. Dice usar una clave XOR de 4 bytes `5B 9F 2E 64` y una lista de servidores comprimida con zlib.
- **Por qué importa:** la web de PESBUL dice que **este proyecto fue su punto de partida**. Útil para comparar la clave XOR y la estructura de la lista de servidores con lo que ya tenemos de AzrakHQ. El `trace.js` de Frida es una idea buena para depurar nuestro propio cliente.
- **Ojo:** apunta a la versión 1.07.02, no a la 1.01.00.

### 4. Novedades sobre PESBUL (ya conocido) — datos nuevos
- **Quién está detrás:** todo indica que PESBUL = el mismo autor de **NoobSaibot888** en GitHub (README en inglés y búlgaro; PESBUL = PES Bulgaria). Repos: https://github.com/NoobSaibot888/pes_server (9 estrellas, Python + JS; DNS propio, STUN, panel admin en `localhost:8000/admin`, login contra usuarios de WordPress, scripts de análisis de paquetes `pes_packets_detailed.json`) y https://github.com/NoobSaibot888/pes2021-teamplay-v2 (MIT según README; "WordPress Lobby + PES P2P", redirección por hosts, endpoints `/XME994-E1/info/info_en.txt`). Ambos dicen "aún no funciona" (son versiones viejas; el código actual no es público).
- **Imagen Docker pública:** `noobsaibot301/pes2021-emulator:latest` en Docker Hub (https://hub.docker.com/r/noobsaibot301/pes2021-emulator), ~350 MB, más de 10 mil descargas, actualizada hace horas. Es un binario cerrado: **no la ejecutamos ni descargamos**. Solo confirma que su servidor se distribuye en Docker (un post menciona TrueNAS).
- **Lo que hace su conector (.BAT):** según un blog chino que lo analizó (https://github.com/yangfanbit/yangfanbit.github.io, post `post/pes2021/`, 3 nov 2026), escribe **7 líneas en hosts**: `pes21-x64-gate.cs.konami.net`, `pes21-x64-stun.cs.konami.net`, `pesam.stun.service.konami.net`, `info.service.konami.net`, `cs.konami.net`, `ntl.service.konami.net`, `ntleu.service.konami.net` (todas a su VPS), y guarda copia `hosts.pesbul-backup`. **Ojo:** ese mismo blog usa un exe cambiado y CreamAPI → **no seguir ese blog para instalar**.
- **Requisitos de PESBUL:** exe **1.07.02** + DLC 1–7, juego limpio sin parches, misma base de datos en ambos jugadores (si no, error `C_NAJM_0022` o no se puede entrar a la sala).
- **Estado:** beta pública en VPS (8 núcleos, 16 GB). Lobby amistoso 1v1 ~99 %, clanes y co-op de clan parciales; 11v11 (Team Play) llega a la carga de **GameRelay** pero no a "Side Select" ni arranca. Dicen que **"casi el 90 % de los modos online dependen de GameRelay"**. La alfa vieja usaba túnel + relay en casa.
- **Ideas que podemos copiar (concepto, no código):**
  - **Verificación de identidad** sin pedir contraseñas: la web da un código y el jugador lo escribe en el juego en *Ajustes → Ajustes online → Editar perfil → Equipo favorito*; el servidor lo lee y une la cuenta web con el jugador del juego.
  - Mandan la **base de datos completa por el sistema Live Update** del propio juego (para que todos tengan la misma).
  - Anulan resultados y banean si se detecta una base de datos modificada en partidas con ranking.
- **Detalle técnico citado en Evoweb:** en una respuesta, la lista `ulist` tuvo que ir con cada elemento como **binario msgpack** y no como mapa directo; si no, el parser de PES 2021 fallaba. (Útil si nos pasa lo mismo.)
- **Hilo Evoweb:** https://evoweb.uk/threads/pes-2021-online-server-emulator-open-beta-test-server.106068/ (páginas 2, 3, 8, 9, 10 tienen detalles técnicos; en la 9 cuentan que PES 2018 acepta `CMD_GET_SVRLIST` y luego pide `CMD_GET_URLLIST`, `GET_SVRVERSION`, `GET_SVRTIME`, y se cierra a 1,4 s).

### 5. Novedades sobre AzrakHQ (ya conocido)
- El repo **antes se llamaba** `MohamedAliRashad/pes2021-private-server` (ese enlace redirige a AzrakHQ). Algunos sitios (indonesios, portugueses) aún lo citan con el nombre viejo.
- Ahora tiene 246 commits, sitio en `azrakhq.github.io/pes2021-online-server/` y un `docs/protocol.html`. Sigue sin forks. No encontré ningún otro repo que tenga sus nombres de comandos.

### 6. Sider: módulo `netblock.lua` (pequeño, pero confirma hosts)
- **URL:** https://github.com/pes-modding/sider7/blob/master/modules/netblock.lua (también en sider6 y sider4)
- **Qué hace:** busca en memoria los textos `pes21-x64-gate.cs.konami.net` y `pes21-x64-stun.cs.konami.net` y los sobrescribe con `0.0.0.0` (bloquea el online). En sider6 aparece `pes20-x64-gate…`.
- **Qué nos da:** un ejemplo de cómo **cambiar el host dentro del juego sin tocar el archivo hosts** (mismo truco, pero en vez de `0.0.0.0` se pondría el nombre de nuestro servidor). Útil para un lanzador sin permisos de administrador. (Sider ya lo conocíamos; el módulo concreto es lo nuevo.)

### 7. FoxDen — framework de mods para PES 2018–2021
- **URL:** https://github.com/Giovani1906/FoxDen — GPL-3.0, 1 commit, oct 2026. C++ (proxy `dinput8.dll`) + Lua.
- **Qué contiene:** se engancha al motor Lua 5.1.1 del juego, consola Lua en vivo (TCP 127.0.0.1:7777, solo en versión de desarrollo) y parches en memoria.
- **Qué nos da:** base para un **lanzador/DLL propio** que pueda leer el estado del juego en vivo (por ejemplo, saber qué modo eligió el jugador) o redirigir hosts. No tiene nada de red online.
- **Ojo:** la consola no tiene contraseña; nunca exponerla fuera de 127.0.0.1.

### 8. Fiveserver / Sixserver (PES 5 y PES 6) — modelo de comunidad que funciona hace 15 años
- **URL:** https://github.com/juce/fiveserver — BSD-2-Clause, 56 estrellas, 26 forks, Python 3 + MySQL, actualizado nov 2026.
- **Qué contiene:** servidor completo PES5/PES6 con 2 contra 2, cuentas, estadísticas, web de registro/estadísticas, panel admin y **API REST de estadísticas en vivo**. Hay Docker (`Volcommm/fiveserver-docker`) y forks como `pablolezcano/pes6-sixserver` (BSD-2) y `alejooroncoy/pes6-server-manager` (gestor).
- **Qué nos da:** ideas de producto (API de estadísticas para la web de la liga, panel admin), y una licencia permisiva (BSD) si quisiéramos copiar alguna pieza genérica. El protocolo es más viejo (no es el de PES 2021).

### 9. Servidores comunitarios de PES 2013 y PES 2011 (sin código público encontrado)
- **ThirteenServer** — https://thirteenserver.online/ — servidor de PES 2013 para PC (parche 1.04) y PS3 (cambiando DNS), con partidas con ranking, amistosos y Liga Máster. Hilo: https://evoweb.uk/threads/pes-2013-private-server-released-now-on-pc-and-ps3.106472/ . Otro: "PES 2013 Online — Community Server" (https://evoweb.uk/threads/pes-2013-online-%E2%80%94-community-server-master-league-online-leagues-custom-tournaments-and-many-more.107221/). Un dev dijo que `GateInfo.php` funciona con un servidor mínimo de relleno.
- **ELX11 (Evolution Legacy X 11)** — servidor de PES 2011 PC (PS3 "pronto"), con web y Discord; anunciado en el perfil "pesmaster" de soccergaming.com (https://soccergaming.com/forums/members/pesmaster.294606/recent-content).
- **Qué nos dan:** prueba de que la comunidad revive estos juegos; ideas de comunidad (ligas, Liga Máster online). No encontré su código.

### 10. PES 2018 en PS3 real — `HuziadOnTop/pes2018-ps3-server` (desaparecido)
- **URL:** https://github.com/HuziadOnTop/pes2018-ps3-server — **hoy da 404** (borrado o privado). Según el buscador: servidor local para PES 2018 PS3 (NPUB31864, parche 1.05.02), la PS3 hace POST a `GateInfo.php` (puerto 80) y recibe la dirección del gate (puerto 12000). Solo red local. Hubo hilo en ReSHax.
- **Qué nos da:** confirma que **PES 2018 usa el mismo `GateInfo.php` → gate** que PES 2021. Si alguien tiene una copia, valdría la pena.

### 11. Ruta HTTP `GateInfo.php` en varias versiones (dato de contexto)
- Informes públicos de análisis de archivos (hybrid-analysis.com, `PES2015.exe`) muestran rutas `…/ntl/api/GateInfo.php` en PES 2013 (`ntl.konamionline.com`) y PES 2015. En PES 2020 el gate era `pes20-x64-gate.cs.konami.net`.
- **Qué nos da:** confirmación de que la ruta y el flujo `GateInfo.php` → gate es igual desde ~2013 hasta 2021.

### 12. Charla "Cyber Necromancy" (método para revivir servidores muertos)
- **URL:** https://media.ccc.de/v/31c3_-_5956_-_en_-_saal_2_-_201412281400_-_cyber_necromancy_-_joseph_tartaro_-_matthew_halchyshak (31C3, 2014; también ToorCon y BruCON).
- **Qué contiene:** cómo el equipo SaveMGO revivió Metal Gear Online: empezar repitiendo capturas viejas y luego desensamblar el cliente para saber qué espera. Romper el cifrado de red fue lo único imprescindible.
- **Qué nos da:** método de trabajo y casos de error típicos. SaveMGO hoy tiene servidores de MGO1/MGO2/MGO3: `GHzGangster/Nomad` (MGO2, Java, AGPL-3.0), `curi0us/mgops` (MGO1, GPL-3.0), `GHzGangster/savemgo-guides`.

### 13. Seraprt/PES-online — plataforma online alrededor de PES 2017 (idea de producto)
- **URL:** https://github.com/Seraprt/PES-online — sin licencia visible, 6 commits, oct 2026.
- **Qué es:** no emula a Konami; es una web (React + FastAPI + Postgres + WebSockets) + una app de escritorio "compañera" para cuentas, salas, torneos, sincronización de Liga Máster. El partido se juega por otro lado.
- **Qué nos da:** idea parecida a nuestra web de liga + lanzador. Sin licencia → solo mirar.

### 14. Mod de eFootball móvil "ZiruOffline" (solo como idea; NO usar)
- **URLs:** https://github.com/hiuzz24/pes2027-mod (y copias: `tuanquayhuok/pes`, `Nguyenan2311/EfootballMod`, `iOSVNNews/efx-v17-build`).
- **Qué es:** inyecta una librería en eFootball de iOS, desvía las peticiones a un servidor local, y su modo "Friend Match" usa **servidor de señalización propio + relay coturn (TURN)** para 1v1 por internet. Usa msgpack-c.
- **⚠️ Evitar:** requiere IPA **desencriptada** (quitar protección), sube divisiones automáticamente y edita stats (trampa). Solo vale la **idea**: TURN estándar (coturn) como respaldo cuando el P2P falla, que es justo lo que nos falta para GameRelay/TURN.

### 15. Otros que aparecieron (poco útiles)
- `xLaugh/open-eleven` — "Destiny Eleven" open source (GPL-3.0). No es PES; sin relación.
- `pokmon23/pes2017revivalbypokmon` — vacío.
- `razwanrzgar81-crypto/Pes2018-realive` — solo un README promocional; habla de "hacer el juego offline" → **sospechoso, evitar**.
- `dzakyraf/pes2017-recomp` — recompilación de PES 2017 (Xbox 360) a Android; sin online.
- `killer2speed/efootball-server-analyzer`, `Angganoe/efootball-server-quality-detector` — medidores de ping para eFootball actual; nada de protocolo. El segundo pide bajar un .exe de "Releases" → **evitar**.
- RPCS3/RPCN — el emulador de PS3 tiene un "PSN privado" (RPCN) que puede conectarse a servidores de fans; no hay nada de PES aún.
- Discord públicos vistos: "PES 21 Patchs (PC)" (https://discord.com/servers/pes-21-patchs-pc-1239087815292358737), "Pes6Stars Official", "Pes Without Stress" (competiciones eFootball en Evoweb). Ninguno es de desarrollo de servidores.
- Reddit r/WEPES: solo recomienda Parsec para jugar online en PES nuevos; nada técnico.
- Búsquedas en ruso (4pda), chino (bilibili/tieba), árabe, turco, indonesio (Kaskus) y polaco: **todo apunta a PESBUL**; no hay proyectos propios de esas comunidades con código.

---

## ⚠️ Cosas a evitar (piratería, cracks, malware)
- Cualquier guía que cambie `PES2021.exe` por otra versión descargada o reemplace `steam_api64.dll` por **CreamAPI/Goldberg** (por ejemplo el blog chino del punto 4 y páginas tipo "PES 2020 crack bypass").
- "CPY", "repack", "LITE bypass": son cracks → no.
- Páginas SEO basura en `edx.ipeindia.org` (kitserver/patches "verified", "hot") → posible malware.
- `TopeHar/eFootball-2022-free-coins-generator`, `miqautidze/pes2021` ("free coins") → estafa.
- `alicealys/mgo3-rce` → exploit (no usar).
- ZiruOffline (punto 14) → IPA desencriptada + trampas.
- Imagen Docker de PESBUL → binario cerrado de terceros; no ejecutar.
- Cualquier `.bat`/`.exe` de terceros sin código fuente.

## Lo que NO se encontró
- Ningún otro repo con el protocolo de PES 2021 al nivel de AzrakHQ (Matchday, divisiones, co-op, GameRelay).
- Ningún disector de Wireshark para el protocolo de Konami.
- Ninguna captura de archive.org consultable desde aquí (dominio bloqueado).
- Ninguna documentación pública de **GameRelay/TURN** de PES 2021 fuera de AzrakHQ y los comentarios de PESBUL.

---

## Top 5 para estudiar después
1. **OpenEleven `docs/reverse-engineering/`** (pes2011-session-protocol.md + pes2011-cmd-reference.md): el antepasado del protocolo; explica WATCH, NAT, divisiones y fin de partido. https://github.com/moth1995/open-eleven-server
2. **tpp-server-emulator `docs/progress.md` y su código del gate/sesión** (ticket de Steam, clave de sesión, `CMD_SEND_IPANDPORT`): misma plataforma de Konami, servidor maduro. https://github.com/alicealys/tpp-server-emulator
3. **README de unknown321/mgsv_emulator** (capas de cifrado de Konami y Blowfish no estándar) para comparar con nuestro NclMio. https://github.com/unknown321/mgsv_emulator
4. **Hilo Evoweb de PESBUL, páginas 8–10** (GameRelay, Team Play, el detalle del `ulist` en binario msgpack, cierre del cliente tras el auth) — hay que habilitar evoweb.uk para leerlo entero. https://evoweb.uk/threads/pes-2021-online-server-emulator-open-beta-test-server.106068/page-10
5. **tpp-mod + netblock.lua de Sider** como modelos de lanzador/DLL que redirige hosts sin tocar el archivo hosts (y la idea del "servidor dedicado"); más **coturn** como relay de respaldo. https://github.com/alicealys/tpp-mod · https://github.com/pes-modding/sider7/blob/master/modules/netblock.lua
