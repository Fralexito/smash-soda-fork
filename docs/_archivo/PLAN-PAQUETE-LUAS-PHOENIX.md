# Paquete de Luas Phoenix: qué ya tienes y hasta dónde se puede llegar

**Fecha:** 2026-10-10. Lo escribió el chat LINK después de leer todo el proyecto:
- `PhoenixSync/` (rama mercado-fase0): base-conocimiento, PRUEBAS, REGISTRO, MANUAL, sider/auditoria;
- la instalación real de ConmeGOL y Sudamerican;
- la web `phoenixevolution` (docs, supabase, contrato v1.10.0);
- `phoenix-overlay`, Phoenix Link y unos 70 Google Docs de la carpeta «Phoenix Mercado — Base de conocimiento PES 2021».

Leyenda: ✅ probado en el juego · 🧪 hecho, sin probar en el juego · 🔎 investigado, sin probar · 🆕 idea nueva.

---

## 1. Lo que ya lograste (la base sobre la que se construye)

**Dentro del juego (Sider + archivos)**
- ✅ Avisos de la web en el overlay en ~1 s (`avisos.txt` → `phoenix.lua`).
- ✅ Stats en vivo sin reiniciar: Player.bin propio en `livecpk\Phoenix-DB` + Editar → Cargar.
- ✅ Fichajes en vivo sin reiniciar, por el option file (Lamine al Madrid) y por la base (PlayerAssignment).
- ✅ Recarga nativa sin Editar (interruptor `exe+0x37F5C39`) y recarga completa (1 byte).
- ✅ **Botón de Konami «Datos Actual. en vivo → Activar» revivido sin internet**, con el diálogo nativo. Desde la v0.17 se aplica solo al arrancar.
- ✅ Regla de qué manda: con «en vivo» activado, la base; desactivado, el option file. Las stats salen siempre de Player.bin.
- ✅ Lectura segura de la memoria (copia con ReadProcessMemory): en qué parte del juego estás («zona»: menú, partido, Editar, Liga Máster…), si es momento seguro para recargar y si la última recarga fue «Activar».
- 🧪 `phoenix_estadio.lua` + árbitro de Link: marcador y goles con minuto hacia Link, bloqueo de Start a los invitados, marcador automático y HUD.

**Archivos del juego dominados**
- ✅ Option file (descifrar y cifrar de forma idéntica, plantillas, tácticas, fichajes con dorsal).
- ✅ Player.bin (20 stats, portero, posiciones, ~45 habilidades).
- ✅ Liga Máster con el juego cerrado: traspasos, dinero, tope salarial, valor de mercado, blob zlib aceptado, «palanca» de las barras de crecimiento.
- 🔎 CompetitionEntry, número de equipos por competición, calendario y tablas de la Liga Máster.

**Fuera del juego**
- ✅ Web + Supabase:
  - duelos con resultado confirmado por los dos, rankings y ELO;
  - `partido_goles` (goleador y asistente, a mano), sanciones;
  - economía con dos monedas, tienda, torneos propios, clanes y logros;
  - Liga Máster web firmada con Ed25519, mercado, vestuario;
  - buzón del juego, chat global, última hora;
  - Twitch, Discord.
- ✅ Phoenix Link:
  - sala por Parsec, roles, mandos, turnos;
  - chat general, amigos y presencia;
  - repartidor de datos de Sync (Player.bin y EDIT, con respaldo y deshacer), multiparche.
- ✅ PhoenixGlass (overlay de escritorio): ping, chat, última hora, mandos, temas y plugins.
- ✅ (simulado) Sync entre PCs (operaciones firmadas, anti-bucle, 369 pruebas).

**Lo que más pesa a tu favor:** con Parsec, **PES corre solo en la PC del anfitrión**. Todo lo que cambie el juego (archivos, reglas del partido, incluso la memoria) **no puede desincronizar a nadie**, porque no hay otra copia del juego. Solo afectaría a partidas por PESBUL, y allí se apaga.

---

## 2. Los límites reales (hasta dónde NO se llega)

| Límite | Consecuencia | Cómo se rodea |
|---|---|---|
| El Lua de Sider **no tiene red** | El juego nunca habla con la web | Buzón de archivos con Link (ya funciona en los dos sentidos) |
| Sin `pcall`, `os.rename` ni `require` | Un error en `init` deja el módulo sin cargar | Bandera de errores; varios módulos pequeños en vez de uno gigante |
| `match.stats()` solo da marcador, reloj y penales | No hay goleadores, tarjetas ni posesión | Leer la memoria (tabla de Cheat Engine de xAranaktu) o que el goleador se elija en Link o la web |
| Sider 7.3.3 no tiene `goal_scored` | Sin evento de gol nativo | Se detecta por el cambio del marcador (ya hecho). Subir a 7.3.4 es posible, pero cambia todo el parche |
| Todas las direcciones valen solo para **este** exe (PATCH10100) | Si Konami o el parche cambian el exe, lo que lee la memoria se apaga solo | Comprobar los bytes antes (ya es la regla) |
| Escribir stats en memoria no se ve | El juego usa otras copias | Archivos por Phoenix-DB (ya probado) |
| La Liga Máster empezada no recibe stats nuevas | Hace una «foto» al crearse | Cargar Phoenix-DB **antes** de crear la carrera, o usar la «palanca» de barras |
| Contenido visual nuevo (caras, escudos, kits, estadios) | Necesita archivos hechos por alguien | Con permiso y crédito; los módulos ya existen (kserv, stadium, miniface…) |
| Supabase gratis (500k invocaciones al mes) | Muchos sondeos lo agotan | Archivos locales y eventos por lotes, no sondeos nuevos |
| La Galaxy League **no está en la base de datos** (sale estática de CopaFácil) | No hay «rival de la fecha» ni tabla automática | PENDIENTE-WEB: tabla de fixtures + endpoint |

---

## 3. La idea central: un **paquete de Luas**, no un Lua gigante

Lo más completo y lo más seguro no es un solo archivo enorme, sino **una familia de módulos pequeños** con el mismo protocolo de archivos (`content\phoenix\`). Así:
- un error apaga una pieza, no todo;
- cada pieza se activa o desactiva con una línea de `sider.ini`;
- lo que toca la memoria queda separado de lo que solo lee.

```
content\phoenix\   (el «buzón» común; Link es el cartero y la web la fuente de verdad)
  avisos.txt     web → juego     (phoenix.lua)              ✅
  estado.json    juego → Link    (phoenix_estadio.lua)      🧪
  resultado.json juego → Link    (phoenix_estadio.lua)      🧪
  sala.txt       Link → juego    (phoenix_estadio.lua)      🧪
  partido.json   web/Link → juego (phoenix_reglas.lua)      🆕
  doctor.json    juego → Link    (phoenix_doctor.lua)       🆕
  sonidos.txt    Link → juego    (phoenix_sonido.lua)       🆕
  stats.json     juego → Link    (phoenix_stats.lua)        🆕 (memoria, solo lectura)
```

### Los módulos, de menor a mayor riesgo

| # | Módulo | Qué hace | Riesgo | Depende de |
|---|---|---|---|---|
| 1 | `phoenix.lua` (existe, v0.17e) | Avisos + botón nativo + zona | Ya probado | — |
| 2 | `phoenix_estadio.lua` (existe, v1.0) | Partido ↔ Link, HUD, árbitro | Bajo (solo lectura) | Tu OK para instalarlo |
| 3 | 🆕 `phoenix_doctor.lua` | **Verificador del anfitrión / antitrampa.** Al arrancar comprueba qué parche y modo hay, que Phoenix-DB esté antes de olmos en `sider.ini`, la huella (suma) de Player.bin y del option file que el juego **realmente lee** (ya se hace con `livecpk_read`), y la versión de cada módulo Phoenix. Lo escribe en `doctor.json`; Link lo muestra («Parche OK / distinto al oficial») y lo manda a la web con el resultado. | Bajo | Nada |
| 4 | 🆕 `phoenix_reglas.lua` | **Reglas del partido desde la web** (la «fase 2»): lee `partido.json` y fija equipos, duración, prórroga y penales, estadio, clima y hora, uniformes y balón, con `set_teams`, `set_match_settings`, `set_conditions`, `set_stadium`, `set_kits`. Los módulos de ConmeGOL ya usan esos mismos eventos. | Medio-bajo (cambia el partido, pero solo en el anfitrión) | Fixtures en la web |
| 5 | 🆕 `phoenix_sonido.lua` | **Identidad sonora**: himno de la Phoenix League al entrar al campo, grito propio de gol, silbato o aviso cuando Link lo pide (descanso, pausa automática, «último minuto»). Usa `audio.new` (probado en `sifflet.lua`). | Bajo | Archivos de audio propios o con permiso |
| 6 | 🆕 `phoenix_identidad.lua` (o configurar los servidores existentes) | **El juego con la cara de Phoenix**: marcador, banners del estadio, trofeo de la Galaxy League, balón oficial, menús. Se hace con `livecpk_get_filepath`, como `kserv`, `StadiumServer`, `TrophyServer` y `MenuServer`, sin tocar la memoria. | Bajo (redirige archivos) | Arte con permiso |
| 7 | 🆕 `phoenix_stats.lua` | **Goleadores, asistentes, tarjetas y posesión** leídos de la memoria con copia segura (punto de partida: la tabla de xAranaktu, comprobando los bytes). Completa `partido_goles` de la web sin moderador. | Medio (solo lectura, pero direcciones nuevas; hay que investigarlas) | Investigación en la VM, como se hizo con la zona |
| 8 | 🆕 `phoenix_weekly.lua` | **Phoenix Weekly**: sirve un `PlayerWeekly.bin`/`TeamWeekly.bin` propio (forma de cada jugador según la vida real o la liga) por Phoenix-DB. | Medio (formato 🔎) | Mapear PlayerWeekly |
| 9 | Escritura de memoria (Liga Máster en vivo, competiciones nuevas, ligas de más de 30) | Lo más ambicioso | **Alto**: solo offline, con tu OK prueba a prueba | Investigación 🔎 |

---

## 4. Lo que puedes construir juntando todo (los «productos»)

### A. Liga oficial 100 % automática (el salto más grande, y todo es posible)
1. La web genera la fecha (hoy falta la tabla de fixtures: **PENDIENTE-WEB**).
2. Link abre la sala solo para los 2 DT de la fecha y les da su puesto y su mando.
3. `phoenix_reglas.lua` fija los equipos, la duración, el estadio y el clima de la fecha.
4. `phoenix_doctor.lua` certifica que el anfitrión usa el parche oficial.
5. El árbitro bloquea Start, pausa si alguien se cae y lleva el marcador solo.
6. `resultado.json` (+ goleadores si existe `phoenix_stats.lua`) → Link → web (**PENDIENTE-WEB**: `POST /v1/partido/resultado` ligado a la fecha o al reto, con confirmación de los dos DT, como `resultados_duelo`).
7. La web actualiza la tabla, el ELO, `partido_goles`, el «jugador de la fecha» y las monedas. Las noticias salen solas en «Última hora» y llegan al juego por el buzón.

**Ya existe** cerca del 70 %: sala, árbitro, buzón, `resultados_duelo`, ranking, `partido_goles`, noticias.
**Falta:** fixtures en la base de datos, el endpoint de resultado, `phoenix_reglas.lua` y `phoenix_doctor.lua`.

### B. Espectadores y transmisión
- El minuto y el marcador en vivo en la web («Salas en vivo») como campos opcionales del latido de Link. Ya los tiene `estado.json`.
- PhoenixGlass (overlay de escritorio para streamers) recibe marcador, goles y nombres por el WebSocket de Link: un marcador de transmisión con la marca Phoenix.
- Twitch ya se detecta en la web: «partido de liga en vivo ahora».

### C. Mercado de fichajes vivo (ya casi está)
Fichaje aprobado en la web → Sync genera Player.bin/PlayerAssignment/EDIT → Link lo coloca → el botón nativo «Activar» lo carga → aviso en el juego.
**Falta:** la prueba real con 2 PCs, que Link acepte PlayerAssignment y que la web guarde contratos (`sueldo`, `clausula` y `fin_contrato` siguen en `null`).

### D. Liga Máster online asíncrona (tu «gran carta»)
Cada DT lleva su club en su carrera. El mercado y la tabla son compartidos en la web (firmados).
**Ya está:** la lectura y edición de la Liga Máster con el juego cerrado, las firmas, el mercado web y `lm_*`.
**Lo que aportarían los Luas:**
- la «zona» dice cuándo estás en la Liga Máster;
- un HUD de tu club en la web (presupuesto, ofertas) leído de un archivo que deja Sync;
- un resultado.json de la Liga Máster.

**Límite:** la carrera hace una «foto» de las stats al crearse. Las stats compartidas se cargan antes de crearla, o con la «palanca».

### E. Juego limpio
- Doctor (huella real de lo que lee el juego) + árbitro (Start bloqueado, pausa automática) + `exigir_build` de la web + Parsec en una sola PC.
- Sanciones de la web aplicadas en la sala: un suspendido no recibe puesto (**PENDIENTE-WEB**: exponer las sanciones en `/v1`).

### F. Lo que nadie tiene
- 🆕 **Jugadores de la comunidad dentro del juego:** un registro nuevo en Player.bin con un ID libre, más la fila de PlayerAssignment y la cara (miniface-server). La carta de la web pasa a ser el jugador del juego. 🔎 No investigado a fondo.
- 🆕 **Eventos de la semana:** «jugador de la semana» con un +1 servido por Phoenix-DB; lesiones o sanciones de la web reflejadas en la forma (Phoenix Weekly).
- 🆕 **Retos en el partido:** la web propone «gana por 2» o «sin recibir goles»; el árbitro lo comprueba con `estado.json` y paga monedas automáticamente.
- 🆕 **Ruleta de reglas** (ya existe en la web): su resultado se aplica de verdad con `phoenix_reglas.lua` (duración, clima, estadio).
- 🆕 **Relato en vivo**: los goles con minuto van al chat general de la web, no solo al de la sala.

---

## 5. Orden recomendado (de lo seguro y valioso a lo ambicioso)

| Paso | Qué | Frente | Riesgo |
|---|---|---|---|
| 0 | Ordenar la casa: commit de Link, subir la **v0.17e** a git, y quitar el **PlayerAssignment.bin de prueba** (Mbappé y Vinícius en el Barça) que según Drive quedó en Phoenix-DB | LINK / SYNC | Ninguno |
| 1 | Instalar y probar `phoenix_estadio.lua` (amistoso contra la CPU) | LINK | Bajo |
| 2 | `phoenix_doctor.lua` (verificador) + Link lo muestra | LINK (+ SYNC revisa) | Bajo |
| 3 | Web: tabla de fixtures de la Galaxy League + `POST /v1/partido/resultado` + `GET /v1/liga/fecha` | WEB (por prompt) | Bajo |
| 4 | Link sube el resultado (con huella del doctor) y confirma con los dos DT | LINK | Bajo |
| 5 | `phoenix_reglas.lua` (`partido.json` desde la web) | LINK + SYNC | Medio-bajo |
| 6 | Marcador del partido en «Salas en vivo» de la web y en PhoenixGlass | LINK / WEB | Bajo |
| 7 | Investigar `phoenix_stats.lua` (goleadores por memoria, solo lectura) | SYNC (VM) | Medio |
| 8 | Sonido e identidad visual (con material propio o con permiso) | LINK | Bajo |
| 9 | Phoenix Weekly, jugadores de la comunidad, Liga Máster online | SYNC + WEB | Medio-alto |

**Hasta dónde se puede llegar:** a una liga en la que, salvo jugar, **nadie tiene que hacer nada a mano**:
- la web arma la fecha;
- la sala y el juego se configuran solos;
- el árbitro vigila;
- el resultado, los goleadores, la tabla, el ranking, las monedas y las noticias se actualizan solos;
- los fichajes de la web aparecen en el juego con el botón de Konami.

Con un parche verificado y sin poder hacer trampa con Start ni con archivos distintos. El techo duro está en lo que exige escribir en la memoria durante el partido (no se hace) y en contenido visual que necesita permisos.
