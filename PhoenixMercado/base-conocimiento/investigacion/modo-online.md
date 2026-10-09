# Investigación: modo ONLINE para el parche Phoenix (PES 2021 PC)

Fecha: 8 de octubre de 2026
Alcance: solo investigación web (no se tocó código).

Cómo leer este documento:
- **[confirmado]** = lo dice una fuente que se pudo leer.
- **[probable]** = lo dicen usuarios de foros, o es deducción razonable.
- **[sin verificar]** = no se encontró fuente; es conocimiento general.

---

## 0. Resumen en 6 líneas

1. Los servidores oficiales de Konami para PES 2021 **ya no existen** (desde 2022).
2. PESBUL es un "servidor falso" hecho por la comunidad. Funciona **solo para el PES 2021 original de Steam, sin modificar**. Football Life 26/27 **no puede conectarse**, porque esa versión trae la parte online quitada.
3. Eso explica casi seguro por qué PESBUL no te funcionó (tu base es FL 27).
4. PES 2021 en PC **no tiene modo LAN**. Radmin/ZeroTier/Hamachi no sirven por sí solos.
5. Lo más realista hoy es esto: **el anfitrión corre el juego y el amigo lo ve por streaming** (tu Phoenix Link, basado en Parsec). A eso se le suma **la web** y un **puente Sider** que lee el marcador.
6. Hacer tu propio servidor emulador es un proyecto de **meses o años** de ingeniería inversa.

---

## 1. Estado de los servidores oficiales de Konami

- PES 2021 se retiró de las tiendas digitales el **9 de diciembre de 2021**. En ese momento Konami dijo que la fecha de cierre de servidores se anunciaría después. **[confirmado]**
  - https://larepublica.pe/videojuegos/2021/11/09/pes-2021-sera-retirado-de-las-tiendas-digitales-y-cerrara-sus-servidores
  - https://delistedgames.com/efootball-pes-2021-to-be-removed-from-steam-december-9th/
- En Steam hay un hilo titulado "Servers are now officially down forever", del **24 de agosto de 2022**. **[probable]** No encontré el comunicado oficial de Konami con la fecha exacta.
  - https://steamcommunity.com/app/1259970/discussions/0/3323114398565714368
- Por eso hoy no hay myClub, Matchday, partidos online, Online Co-op ni actualizaciones en vivo ("Live Update").
- **Consecuencia:** todo lo online que exista hoy depende de la **comunidad**.

---

## 2. Servidores emuladores de la comunidad

### 2.1 Qué es un "server emulator" (explicado simple)

Imagina que el juego, al entrar a "Online", **llama por teléfono** a un número de Konami.

- Ese número ya no contesta (los servidores cerraron).
- Un emulador hace dos cosas:
  1. **Cambia la agenda** del PC para que el número de Konami lleve a otra máquina. Esto se hace con el archivo `hosts` de Windows o con un DNS propio.
  2. En esa otra máquina hay un programa que **contesta igual que Konami**. Le habla al juego en su mismo "idioma" (protocolo) y con el mismo "candado" (cifrado).
- Si una sola respuesta sale distinta a lo que el juego espera, el juego **cuelga la llamada**.

### 2.2 PESBUL (PES 2021) — el más avanzado hoy

**Qué es** **[confirmado]**
- Es un proyecto comunitario. Web: https://pes2021.pesbul.eu/
- Hilo técnico en Evoweb: https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/
- Empezó el 24/06/2026. La beta pública salió el 29–30/07/2026.
- Se basa en el proyecto de código abierto **Pes2021PrivateServer de Nikow5** (licencia MIT). Ese proyecto apunta a la **versión 1.07.02** del juego.

**Cómo funciona** **[confirmado]**
- Un `.bat` (ejecutar como administrador) escribe en `hosts` los dominios de Konami. Ejemplos: `pes21-x64-gate.cs.konami.net` y `pes21-x64-stun.cs.konami.net`.
- El servidor tiene tres piezas:
  1. **ConnectGate (TCP):** login, lista de servidores y sesión. El protocolo se llama "NclMio" (cifrado XOR) y el contenido va en MessagePack.
  2. **STUN (UDP):** ayuda a que los dos PCs se encuentren a través del router (NAT).
  3. **HTTP:** datos (myClub, plantillas).
- Puertos citados: TCP 10000 / 5739, UDP 5730, 5739 y 5740.
- **El partido es P2P (de PC a PC).** El servidor solo arma la sala; el partido **no pasa por el servidor** y no hay relay.

**Qué funciona (agosto–octubre 2026)** **[confirmado]**
- Lobby amistoso 1v1: "~99 % funcional".
- "Online Divisions" en beta. Usan ELO y temporadas propias.
- Team Play 11v11: **no funciona**. Se queda antes del saque inicial, y la simulación corre a ~20 pasos por segundo en vez de ~54.
- Clan Co-op: no funciona. myClub: no está planeado (hay temas legales).
- Para enlazar tu cuenta de la web, metes un código en el juego: *Ajustes → Online → Editar perfil → Equipo favorito*. **Idea útil para Phoenix.**
  - https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html

**Por qué falla (lo que reportan los usuarios)** **[confirmado en el hilo, páginas 3, 7, 9 y 10]**
1. **Parches grandes:** el staff dice que FL26 "no está soportado". Un usuario explica que FL 2026/2027 **no pueden conectar** porque el CPY del Smoke Team **quitó la parte online**.
   - https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/page-7
2. **Hay que usar el juego original:** Steam 1.07.02 sin tocar (Data Pack 7.0). La base de datos debe ser la original; editarla puede costar un **baneo**.
3. **Sider, VirtuaRED o módulos de gameplay:** se entra al lobby, pero el partido no arranca o se desincroniza.
4. **NAT o CGNAT:** datos móviles, hotspot, VPN, redes públicas. El consejo es poner el puerto UDP **manual** en Settings.exe (5739/5740), abrirlo en el router (o activar UPnP) y permitir PES2021.exe en el firewall.
5. **Cambio de VPS:** el 17/08/2026 avisaron que el `.bat` viejo ya **no conecta** al servidor nuevo. Si tu `hosts` apunta a una IP vieja (por ejemplo 151.247.209.104), puede que ya no sea la correcta. **[probable]**
6. **Versión CPY:** el ID de Steam que trae por defecto está **baneado**; hay que poner uno real en `cpy.ini`.
7. **"Unable to connect" al empezar el partido:** sigue pasando a algunos hasta septiembre de 2026. Es un fallo del lado del servidor o del P2P.

**Cómo encaja con tu caso** (base FL 27 + option files propios)
- Por los puntos 1, 2 y 3, **PESBUL no puede servir hoy** como "online" de tu parche. **[probable, con bastante seguridad]**
- Aunque llegaras a conectar, PESBUL es de otra gente: **no controlas** sus reglas, su base de datos ni sus baneos.

### 2.3 Otros juegos (para comparar)

**PES 6:** es el gran ejemplo de que sí se puede.
- **Fiveserver/Sixserver** (escrito por juce y reddwarf en Python) es un servidor completo para PES 6.
- **evo-league** lo unió con una **web de ladder** (PHP + MySQL). El servidor reportaba los resultados a la web con tareas programadas (cron).
- Es código abierto (MIT): https://github.com/kinj1987/evo-league
- **[confirmado]** Es justo la arquitectura "juego ↔ web" que buscas, pero en un juego de 2006 mucho más simple.

**PES 2018:** hay intentos (cifrado XOR → MD5 → Blowfish → MessagePack) que cuelgan el juego. **[confirmado, mismo hilo, página 9]**

**PES 2019 / PES 2020:** no encontré emuladores activos. **[sin verificar]**

### 2.4 Riesgos legales y de ToS (resumen honesto)
- Emular un servidor y modificar el juego va contra el EULA de Konami. En la práctica, Konami no persigue PES 2021. Aun así, el riesgo existe. **[sin verificar]**
- En Evoweb **prohíben compartir enlaces del CPY** de PES 2021. **[confirmado, página 10]**
- **Lo más seguro para ti:** que cada jugador tenga el juego legal de Steam. Tu web y tu app **no deben distribuir el juego**, solo tus archivos (option file, módulos).

---

## 3. Alternativas para jugar PES 2021 en PC "online"

### 3.1 ¿PES 2021 PC tiene modo LAN?
- **No.** No encontré ningún modo LAN en PES 2021 PC. **[probable]**
- Los modos de 2 PCs (amistoso online, Co-op) necesitan el **servidor de matchmaking** (antes Konami, ahora PESBUL).
- **Por eso Radmin VPN, ZeroTier o Hamachi no sirven por sí solos.** Esos programas crean una "LAN falsa", pero el juego no tiene nada que buscar en una LAN.
  - Solo ayudarían junto con un emulador, por ejemplo si un día montas un PESBUL propio dentro de esa red. Ojo: PESBUL dice que las **VPN pueden empeorar** el P2P.

### 3.2 Streaming: "uno juega, el otro mira y manda el mando"

**Parsec / tu Phoenix Link**
- El anfitrión corre el juego. El amigo recibe video y envía su mando (ViGEm crea un mando virtual en el PC del anfitrión).
- **Ventajas:**
  - Solo el anfitrión necesita el juego y el parche, así que **no hay desincronización posible**.
  - Funcionan **todos** los modos locales: Liga Máster, Copa, amistoso.
  - Sider funciona entero.
- **Desventajas:**
  - El invitado tiene más lag (video + red). Los pases rápidos se sienten más lentos.
  - Depende de la subida de internet del anfitrión.

**Steam Remote Play Together:** es la misma idea, de Valve. No pude confirmar si PES 2021 tiene la etiqueta oficial **[sin verificar]**. Además no lo controlas tú: no puedes integrarlo a tu web ni a tu app.

**Moonlight + Sunshine:** buen streaming, pero está pensado para **tu propio PC**. No tiene invitados ni sistema de amigos. Para una liga, Parsec (tu fork) es mejor.

**"Netplay" tipo RetroArch:** no existe para PES 2021. Esa técnica sirve para emuladores de consolas viejas.

---

## 4. Cómo organizan ligas las comunidades hoy

Lo que se ve en foros y Discord **[probable; hay pocas fuentes formales]**:
- **Discord** como centro: canales de calendario, de resultados y de reclamos.
- **Resultados con captura de pantalla** del marcador final (o clip). Un admin o un bot de Discord lo anota en una tabla o en una web.
- **Versión igual para todos:** se dice "todos con el parche X versión Y". Se comprueba a mano o, a lo mucho, con hash de archivos. Es justo tu idea del verificador FL 27.
- **Antitrampa:** sobre todo reglas (difícil de vigilar). Por eso en Parsec el **anfitrión neutral** ayuda: es un solo PC con una sola configuración.
- **PESBUL Online Divisions:** su servidor ve el partido, así que guarda el resultado **automáticamente**. Es la ventaja de tener servidor propio.
- **Lección de evo-league (PES 6):** el resultado automático solo llegó cuando hubo **servidor propio**. Antes de eso, la web usaba "reportar victoria" a mano.

---

## 5. Sider 7.x: lo que sirve para conectar el juego con la web

Fuentes: tu propia auditoría del repo (`PhoenixMercado/sider/RIESGOS-SIDER.md` y `PhoenixMercado/sider/auditoria/*.md`) y la página de Sider 7 (https://www.pesmodding.com/2020/09/sider-for-pes-2021-season-update.html).

**Qué tiene el Lua de Sider** (LuaJIT 5.1)
- **`io`:** leer y escribir archivos.
- **`fs`:** buscar archivos y crear carpetas.
- **`memory`:** leer y escribir la memoria del juego.
- **`ffi`:** llamar funciones de Windows desde Lua.
- **`zlib`, `audio`, y `os`** (solo `date`, `time` y `clock`).
- **`match`** **[confirmado en la guía oficial "Sider 7 Lua Module Programmers Guide", para sider.dll 7.4]**:
  - Es experimental y viene **apagada**. Para encenderla hay que poner esta línea en `sider.ini`: `match-stats.enabled = 1`
  - `match.stats()` devuelve `nil` si no hay partido. Si hay partido, devuelve una tabla con:
    - `home_score`, `away_score` (el marcador);
    - `pk_home_score`, `pk_away_score` (los penales);
    - `period` (0 = desconocido, 1 = primer tiempo, 2 = segundo tiempo, 3 y 4 = prórroga, 5 = penales);
    - `clock_minutes`, `clock_seconds` (el reloj);
    - `added_minutes` (el tiempo añadido).
  - **No trae** goleadores, tarjetas ni posesión.
  - El módulo de ejemplo es `modules/mstats.lua`. `sifflet.lua` también lo usa en cada frame.
- **`ctx`** (el "contexto" del partido):
  - `ctx.home_team` y `ctx.away_team` (IDs de los equipos).
  - `ctx.tournament_id`, `ctx.match_info`, `ctx.stadium`, `ctx.season`.
  - `ctx.sider_dir` (la carpeta de Sider).
  - **Ojo:** `IntroServer.lua` **cambia** `ctx.tournament_id`.

**Eventos útiles**
- `set_teams`: cuando se eligen los equipos.
- `after_set_conditions`: una vez por partido.
- `display_frame`: cada frame.
- `overlay_on`, `key_down`, `gamepad_input`.
- `set_match_settings`, `context_reset` (desde 7.3.0) y los **eventos personalizados** (desde 7.3.0).
- **No hay** un evento oficial de "fin del partido". Para saber cuándo terminó, se mira `match.stats()` desde `display_frame`: cuando `period` llega al final y el reloj se detiene, o cuando `match.stats()` vuelve a dar `nil`. **[deducción]**
- `goal_scored`: `goalscreams.lua` dice que necesita **Sider 7.3.4 o más**, pero ese evento **no aparece** en la lista oficial de la guía 7.4. Puede ser un evento personalizado que crea otro módulo. **[sin verificar]** Tu instalación tiene 7.3.3, así que hoy **no existe** en tu PC.

**Red:** Sider **no trae red**.
- No hay LuaSocket ni HTTP.
- Con `ffi` se podría llamar a WinHTTP, pero es arriesgado y puede trabar el juego.
- **La forma segura es un "buzón de archivos":**
  1. El módulo Lua escribe `resultado.json` en una carpeta.
  2. **Phoenix Link** (C++) vigila esa carpeta y lo **sube a Supabase**.
  - Es el mismo patrón que ya usas al revés para `avisos.txt`.
- Truco para no leer un archivo a medio escribir: escribir primero `.tmp` y luego renombrar.

**Módulos que ya mandan datos fuera del juego:** no encontré ningún módulo de Discord Rich Presence para PES 2021 Sider. **[sin verificar]** Si existiera, haría lo mismo: Lua escribe un archivo y un programa externo lo envía.

**Límite importante:**
- `match.stats()` da el marcador, los penales y el reloj, y **nada más** **[confirmado]**.
- Para goleadores, tarjetas o posesión habría que leer memoria (offsets que **cambian con cada exe**: FL 27 ≠ Steam), o pedir al jugador una **captura del resumen**.

---

## 6. Parsec desde Perú (latencia y ajustes)

**Lo que dice Parsec** **[confirmado]**
- https://support.parsec.app/hc/en-us/articles/32381460716180-Parsec-Connectivity-Requirements
- https://support.parsec.app/hc/en-us/articles/32381352822804-Troubleshooting-Lag-Latency-and-Quality-Issues
- Es **P2P directo**: el video **no pasa** por los servidores de Parsec. Usa STUN (UDP 3478). El relay solo existe para clientes Enterprise.
- **No funciona bien detrás de doble NAT ni de CGNAT.**
- Para 1080p a 60 fps:
  - El **anfitrión** necesita mínimo **10 Mbps de subida** (recomendado 30, con cable). Con 2 o más invitados, 50 Mbps o más, mejor fibra.
  - El **invitado** necesita mínimo 10 Mbps de bajada y un ping **menor a 30 ms** (ideal menor a 15 ms).
- Ping **menor a 30 ms = bueno**, **menor a 60 ms = aceptable**.
- Codificar y decodificar cada cuadro debe tomar **15 ms o menos** a 60 fps.
- Si se ven cortes, poner el **límite de ancho de banda en 3 Mbps** y subirlo poco a poco.

**Ajustes recomendados para fútbol (PES)**
1. **60 fps siempre.** En fútbol, la fluidez importa más que la nitidez.
2. **Resolución 720p o 900p** si la subida del anfitrión es menor a 20 Mbps.
3. **H.265 activado** si todos los PCs lo soportan (ayuda con poca banda). Si se ve mal, apagarlo.
4. **Cable de red** en los dos lados, o al menos Wi-Fi de 5 GHz. Nada de repetidores, PLC ni mesh.
5. **V-Sync apagado** (si no hay tirones).
6. **Decodificador por hardware** en el invitado (que no diga "Software").
7. **Juego en ventana sin bordes** y NVIDIA en "máximo rendimiento".
8. **Sin VPN** (Radmin, etc.): suman lag.

**Realidad en Perú** **[probable / sin verificar]**
- Dentro de Lima, entre fibras de distintos operadores, se suele ver un ping de 10 a 30 ms. Eso es jugable.
- Lima ↔ provincias o Lima ↔ otro país sube a 40–100 ms o más. Se nota en el pase y el tiro.
- El **CGNAT** es común en internet móvil y en algunos planes de cable. Ese jugador quizá **no pueda ser anfitrión**.
- **Regla práctica para la liga:** que sea anfitrión el jugador con **fibra y mejor subida**. Si no, un **anfitrión neutral** (un PC "servidor de la liga") donde los dos son invitados. Así el lag es justo para ambos.

---

## 7. Recomendación concreta para "Phoenix online"

### Arquitectura realista (hacerla YA)

```
[Web Phoenix (Astro + Supabase)]
   |  1. crea el partido (duelo/fecha de liga) y un código de sala
   v
[Phoenix Link en el PC anfitrión]  (tu fork de Smash Soda / Parsec)
   |  2. abre sala, deja entrar SOLO a los 2 jugadores del partido (login con cuenta Phoenix)
   |  3. lanza PES 2021 con el option file OFICIAL de la liga (switcher)
   v
[PES 2021 + Sider + módulo phoenix.lua]
   |  4. al terminar: escribe resultado.json (equipos, marcador, minuto, fecha)
   v
[Phoenix Link]  5. lee el .json, adjunta captura automática del marcador
   |            y lo sube a Supabase  ->  la web actualiza tabla, mercado, perfiles
```

**Por qué esta**
- **No depende de ningún servidor de Konami ni de PESBUL.**
- **No hay desincronización:** el juego corre en un solo PC.
- **El anfitrión controla el parche:** FL 27 + tu option file. Tu verificador solo tiene que revisar al **anfitrión**, no a todos.
- **Ya tienes el 70 % hecho:** Phoenix Link, ViGEm, la web, Supabase y el puente de `avisos.txt`.

**Puntos a cuidar**
- Que el **lag sea justo**: usar un anfitrión neutral, o turnar el anfitrión (ida y vuelta).
- Que el resultado sea **confiable**:
  - El `.json` de Sider + una **captura automática** que toma Phoenix Link.
  - Además, **los dos jugadores confirman en la web**. Si no están de acuerdo, decide un admin.
- Para el marcador **no hace falta subir de versión**. Basta con encender `match-stats.enabled = 1` en `sider.ini` y probar offline que no cause problemas (es una función experimental).

### Mediano plazo
- Un "modo liga" en Phoenix Link que **bloquee las teclas de Sider** y los módulos que no sean oficiales mientras dura el partido.
- Más datos del partido (goleadores, tarjetas) leyendo memoria. **Solo para el exe de FL 27**, y probándolo offline primero.
- Usar el truco del **"equipo favorito con código"** (como PESBUL) para comprobar que el perfil del juego es el de la cuenta web.

### Proyecto de investigación a largo plazo (no es para ahora)
- **Tu propio servidor emulador**, partiendo del código MIT de Nikow5 o de la futura "versión comunitaria" de PESBUL. Haría falta:
  - Ingeniería inversa (Ghidra, Frida) del protocolo NclMio y de la autenticación con Steam.
  - Arreglar el P2P y el NAT.
  - Lo más difícil: **devolver la parte online a FL 27**, que el Smoke Team quitó.
- Hasta PESBUL, con un equipo y meses de trabajo, sigue sin 11v11 ni Co-op en octubre de 2026.
- **Si algún día PESBUL publica su servidor para auto-alojar**, revisarlo: podrías montar un "PESBUL Phoenix" solo para tu liga. Pero seguiría exigiendo el **juego vanilla de Steam**, no FL 27.

### Riesgos
1. **Lag del invitado** en Parsec. Es el costo del modelo de streaming. Se reduce con buena fibra y un anfitrión cercano.
2. **CGNAT o mala subida** de algunos jugadores: no pueden ser anfitriones.
3. **Trampa del anfitrión:** el anfitrión tiene el juego y podría tocar algo. Se cuida con el verificador de hashes, la captura automática y la confirmación de los dos jugadores.
4. **Legal:** no distribuir el juego ni el CPY. Solo tus archivos.
5. **Escribir memoria con Sider** puede trabar el juego o cambiar el gameplay. Hay que mantener la regla: primero **solo leer**, y probar offline.
6. **Depender de terceros** (PESBUL, Parsec): Parsec puede cambiar su SDK o sus términos. PESBUL puede cambiar de IP o cerrar (ya cambió de VPS en agosto de 2026).

---

## Fuentes principales
- PESBUL, hilo Evoweb (págs. 1, 2, 3, 7, 9 y 10): https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/
- Web PESBUL: https://pes2021.pesbul.eu/
- Guía PESBUL: https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html
- Nikow5, Pes2021PrivateServer (README leído desde raw.githubusercontent): https://github.com/Nikow5/Pes2021PrivateServer
- evo-league / Sixserver (PES 6): https://github.com/kinj1987/evo-league
- Retiro de tiendas: https://larepublica.pe/videojuegos/2021/11/09/pes-2021-sera-retirado-de-las-tiendas-digitales-y-cerrara-sus-servidores
- Hilo de Steam sobre el cierre (agosto de 2022): https://steamcommunity.com/app/1259970/discussions/0/3323114398565714368
- Sider 7: https://www.pesmodding.com/2020/09/sider-for-pes-2021-season-update.html
- Parsec, requisitos: https://support.parsec.app/hc/en-us/articles/32381460716180-Parsec-Connectivity-Requirements
- Parsec, lag y calidad: https://support.parsec.app/hc/en-us/articles/32381352822804-Troubleshooting-Lag-Latency-and-Quality-Issues
- Parsec, poca banda: https://support.parsec.app/hc/en-us/articles/32361399881620-How-To-Use-Parsec-On-Low-Bandwidth-Connections
