# Phoenix Link · Fichajes, traspasos en vivo, Sync y preparar el PES de un amigo

Cómo llegan los fichajes de la web al PES de cada jugador:
- el paquete de Luas (`phoenix.lua`) y cómo preparar el juego de un amigo;
- los fichajes como cambios sobre la base de cada PC;
- los traspasos en vivo;
- el parche ConmeGOL 27;
- la vinculación de cuenta más sencilla (0.3.41–0.3.45).

**Guía práctica más reciente:** la Parte «Guía completa… pasar un fichaje de un PES a otro». **Base de datos:** la Phoenix-DB actual es PROVISIONAL (copia de olmosjr23). El código de Sync está en la rama `mercado-fase0` (`PhoenixSync/`).

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 8 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Paquete de Luas Phoenix: qué ya tienes y hasta dónde se puede llegar](#parte-1-paquete-de-luas-phoenix-que-ya-tienes-y-hasta-donde-se-puede-llegar) — origen `docs/PLAN-PAQUETE-LUAS-PHOENIX.md` (35f9a8f 2026-10-10)
- [Parte 2 · Preparar PES 2021 de un amigo para Phoenix (phoenix.lua v0.18b)](#parte-2-preparar-pes-2021-de-un-amigo-para-phoenix-phoenixlua-v018b) — origen `docs/PREPARAR-JUEGO-AMIGO.md` (ef7fad3 2026-10-10)
- [Parte 3 · Fichajes como CAMBIOS (generados en cada PC sobre SU base)](#parte-3-fichajes-como-cambios-generados-en-cada-pc-sobre-su-base) — origen `docs/FICHAJES-COMO-CAMBIOS.md` (442861a 2026-10-12)
- [Parte 4 · Sync y PC del amigo: análisis de escenarios (beta 1 · 0.2.45)](#parte-4-sync-y-pc-del-amigo-analisis-de-escenarios-beta-1-0245) — origen `docs/SYNC-ESCENARIOS.md` (4102fd7 2026-10-12)
- [Parte 5 · Parche ConmeGOL 27: cómo funciona y cómo se instala Phoenix (2026-10-12, Link 0.2.26)](#parte-5-parche-conmegol-27-como-funciona-y-como-se-instala-phoenix-2026-10-12-link-0226) — origen `docs/PARCHE-CONMEGOL-27.md` (18d9f54 2026-10-13)
- [Parte 6 · Traspasos EN VIVO entre PCs con Phoenix](#parte-6-traspasos-en-vivo-entre-pcs-con-phoenix) — origen `docs/TRASPASOS-EN-VIVO.md` (47fc971 2026-10-13)
- [Parte 7 · Link 0.3.41 a 0.3.45: vincular la cuenta y Sync más sencillo (21-oct-2026)](#parte-7-link-0341-a-0345-vincular-la-cuenta-y-sync-mas-sencillo-21-oct-2026) — origen `docs/CUENTA-Y-SYNC-SENCILLO.md` (eee096a 2026-10-21)
- [Parte 8 · Guía completa de Phoenix Link: instalar y pasar un fichaje de un PES a otro (versión 0.3.12)](#parte-8-guia-completa-de-phoenix-link-instalar-y-pasar-un-fichaje-de-un-pes-a-otro-version-0312) — origen `docs/GUIA-COMPLETA-FICHAJES-EN-VIVO.md` (d14a128 2026-10-21)

---

## Parte 1 · Paquete de Luas Phoenix: qué ya tienes y hasta dónde se puede llegar

> Origen: `docs/PLAN-PAQUETE-LUAS-PHOENIX.md` · último cambio 35f9a8f 2026-10-10 · 14240 bytes. Copia íntegra.

**Fecha:** 2026-10-10. Lo escribió el chat LINK después de leer todo el proyecto:
- `PhoenixSync/` (rama mercado-fase0): base-conocimiento, PRUEBAS, REGISTRO, MANUAL, sider/auditoria;
- la instalación real de ConmeGOL y Sudamerican;
- la web `phoenixevolution` (docs, supabase, contrato v1.10.0);
- `phoenix-overlay`, Phoenix Link y unos 70 Google Docs de la carpeta «Phoenix Mercado — Base de conocimiento PES 2021».

Leyenda: ✅ probado en el juego · 🧪 hecho, sin probar en el juego · 🔎 investigado, sin probar · 🆕 idea nueva.

---

### 1. Lo que ya lograste (la base sobre la que se construye)

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

### 2. Los límites reales (hasta dónde NO se llega)

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

### 3. La idea central: un **paquete de Luas**, no un Lua gigante

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

#### Los módulos, de menor a mayor riesgo

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

### 4. Lo que puedes construir juntando todo (los «productos»)

#### A. Liga oficial 100 % automática (el salto más grande, y todo es posible)
1. La web genera la fecha (hoy falta la tabla de fixtures: **PENDIENTE-WEB**).
2. Link abre la sala solo para los 2 DT de la fecha y les da su puesto y su mando.
3. `phoenix_reglas.lua` fija los equipos, la duración, el estadio y el clima de la fecha.
4. `phoenix_doctor.lua` certifica que el anfitrión usa el parche oficial.
5. El árbitro bloquea Start, pausa si alguien se cae y lleva el marcador solo.
6. `resultado.json` (+ goleadores si existe `phoenix_stats.lua`) → Link → web (**PENDIENTE-WEB**: `POST /v1/partido/resultado` ligado a la fecha o al reto, con confirmación de los dos DT, como `resultados_duelo`).
7. La web actualiza la tabla, el ELO, `partido_goles`, el «jugador de la fecha» y las monedas. Las noticias salen solas en «Última hora» y llegan al juego por el buzón.

**Ya existe** cerca del 70 %: sala, árbitro, buzón, `resultados_duelo`, ranking, `partido_goles`, noticias.
**Falta:** fixtures en la base de datos, el endpoint de resultado, `phoenix_reglas.lua` y `phoenix_doctor.lua`.

#### B. Espectadores y transmisión
- El minuto y el marcador en vivo en la web («Salas en vivo») como campos opcionales del latido de Link. Ya los tiene `estado.json`.
- PhoenixGlass (overlay de escritorio para streamers) recibe marcador, goles y nombres por el WebSocket de Link: un marcador de transmisión con la marca Phoenix.
- Twitch ya se detecta en la web: «partido de liga en vivo ahora».

#### C. Mercado de fichajes vivo (ya casi está)
Fichaje aprobado en la web → Sync genera Player.bin/PlayerAssignment/EDIT → Link lo coloca → el botón nativo «Activar» lo carga → aviso en el juego.
**Falta:** la prueba real con 2 PCs, que Link acepte PlayerAssignment y que la web guarde contratos (`sueldo`, `clausula` y `fin_contrato` siguen en `null`).

#### D. Liga Máster online asíncrona (tu «gran carta»)
Cada DT lleva su club en su carrera. El mercado y la tabla son compartidos en la web (firmados).
**Ya está:** la lectura y edición de la Liga Máster con el juego cerrado, las firmas, el mercado web y `lm_*`.
**Lo que aportarían los Luas:**
- la «zona» dice cuándo estás en la Liga Máster;
- un HUD de tu club en la web (presupuesto, ofertas) leído de un archivo que deja Sync;
- un resultado.json de la Liga Máster.

**Límite:** la carrera hace una «foto» de las stats al crearse. Las stats compartidas se cargan antes de crearla, o con la «palanca».

#### E. Juego limpio
- Doctor (huella real de lo que lee el juego) + árbitro (Start bloqueado, pausa automática) + `exigir_build` de la web + Parsec en una sola PC.
- Sanciones de la web aplicadas en la sala: un suspendido no recibe puesto (**PENDIENTE-WEB**: exponer las sanciones en `/v1`).

#### F. Lo que nadie tiene
- 🆕 **Jugadores de la comunidad dentro del juego:** un registro nuevo en Player.bin con un ID libre, más la fila de PlayerAssignment y la cara (miniface-server). La carta de la web pasa a ser el jugador del juego. 🔎 No investigado a fondo.
- 🆕 **Eventos de la semana:** «jugador de la semana» con un +1 servido por Phoenix-DB; lesiones o sanciones de la web reflejadas en la forma (Phoenix Weekly).
- 🆕 **Retos en el partido:** la web propone «gana por 2» o «sin recibir goles»; el árbitro lo comprueba con `estado.json` y paga monedas automáticamente.
- 🆕 **Ruleta de reglas** (ya existe en la web): su resultado se aplica de verdad con `phoenix_reglas.lua` (duración, clima, estadio).
- 🆕 **Relato en vivo**: los goles con minuto van al chat general de la web, no solo al de la sala.

---

### 5. Orden recomendado (de lo seguro y valioso a lo ambicioso)

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

---

## Parte 2 · Preparar PES 2021 de un amigo para Phoenix (phoenix.lua v0.18b)

> Origen: `docs/PREPARAR-JUEGO-AMIGO.md` · último cambio ef7fad3 2026-10-10 · 4618 bytes. Copia íntegra.

Esta guía es para cuando **tu amigo también hostea**, o sea, corre PES y Phoenix Link en su PC. Si solo entra a tu sala por Parsec, no necesita nada de esto: mira `PRUEBA-ONLINE-CON-AMIGO.md`.

**Lo que tiene que tener antes:**
- PES 2021 con **ConmeGOL Patch 26** y su switcher, funcionando.
- Phoenix Link instalado con `PhoenixLink-instalar.bat`. Solo hace falta si va a usar Link.

**Lo que se instala (en cada carpeta de Sider del juego):**

| Qué | Dónde |
|---|---|
| `phoenix.lua` v0.18b | `SiderAddons\modules\` |
| `lua.module = "phoenix.lua"` | `sider.ini`, debajo del último `lua.module` activo |
| `cpk.root = ".\livecpk\Phoenix-DB"` | `sider.ini`, **justo antes** de `cpk.root = ".\olmosjr23\Database"` (la base del parche) |
| carpeta `content\phoenix\` (vacía) | buzón de avisos de la web |
| carpeta `livecpk\Phoenix-DB\common\etc\pesdb\` (vacía) | aquí llegan los fichajes desde Phoenix Sync |

Las carpetas de Sider son **dos**, y hay que tocar las dos:
1. `<juego>\SiderAddons`
2. `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`

Cada vez que cambias de modo, el switcher copia la segunda encima de la primera. Si solo tocas la primera, Phoenix desaparece en el próximo cambio.

> **No copies tu `Player.bin` ni tu Phoenix-DB** a la PC del amigo. Esos datos salen de la base de olmosjr23 y no son nuestros para repartir. La carpeta Phoenix-DB vacía funciona bien: los datos le llegan por Phoenix Sync.

---

### Opción 1 — Con Phoenix Link (lo más fácil)

1. **Cierra PES.**
2. Si es la primera vez, abre PES **una vez** con Link abierto, para que Link aprenda dónde está el juego. Después ciérralo.
3. En Link ve a **SYNC › Puente › Módulos del juego**.
4. En `phoenix.lua v0.18b` pulsa **INSTALAR**. Si ya tenía otra versión, el botón dice **ACTUALIZAR**.
5. Debe decir **INSTALADO**, con ● en las dos carpetas.

Link también:
- hace una copia de `sider.ini` antes de tocarlo (`sider.ini.phoenix-<fecha>`);
- guarda la versión vieja como `phoenix.lua.antes-v<versión>`.

### Opción 2 — Sin Link: `PhoenixJuego-instalar.bat`

1. Pásale `PhoenixJuego-instalar.bat`. Mejor junto con `phoenix.lua` en la misma carpeta; si no lo lleva, el .bat lo baja de GitHub.
2. **Cierra PES.**
3. Doble clic en el .bat y elige la carpeta del juego (la que tiene `PES2021.exe` y `SiderAddons`).
4. Debe terminar con `LISTO: phoenix.lua v0.18b en 2 carpeta(s).`

El .bat:
- hace exactamente lo mismo que el botón de Link (mismas copias de seguridad);
- comprueba el sha256 de `phoenix.lua`; si no es la v0.18b exacta, **no toca nada**;
- se puede ejecutar dos veces sin problema: la segunda no cambia nada.

### Opción 3 — A mano

En **cada una** de las dos carpetas de Sider:

1. Copia `sider.ini` a `sider.ini.respaldo` (por si acaso).
2. Copia `phoenix.lua` (v0.18b) en `modules\`. Si ya había uno, renómbralo antes a `phoenix.lua.viejo`.
3. Abre `sider.ini` con el Bloc de notas:
   - busca la línea `cpk.root = ".\olmosjr23\Database"` y **justo encima** escribe:
     ```
     cpk.root = ".\livecpk\Phoenix-DB"
     ```
   - baja hasta la lista de `lua.module = ...` y **al final** añade:
     ```
     lua.module = "phoenix.lua"
     ```
   - guarda.
4. Crea las carpetas vacías `content\phoenix` y `livecpk\Phoenix-DB\common\etc\pesdb`.

Nunca dejes **dos** líneas de Phoenix-DB ni dos de `phoenix.lua`.

---

### Comprobar que funciona

1. Abre PES con el switcher, como siempre.
2. Abre `SiderAddons\sider.log` con el Bloc de notas:
   - debe aparecer `[phoenix.lua]`;
   - **no** debe decir `Module (phoenix.lua) is NOT activated`.
3. En Link, **SYNC › Puente › Módulos del juego** debe decir «✅ Cargó en el último arranque del juego».
4. En el juego, el panel de Sider (barra espaciadora) muestra los avisos de la web, y en Partido aparece el botón «Datos Actual. en vivo».
5. **Modo de recarga:** viene en **«Automático: siempre»**: los fichajes entran solos al volver al menú principal y entrar a un modo, sin pulsar nada. Se cambia en SYNC › Puente (escribe `contentphoenixmodo.txt`) o en el juego con la tecla T.

### Extra opcional: el módulo del partido

`phoenix_estadio.lua` lleva el marcador y los goles del juego a Link. Solo se instala desde Link: **SYNC › Puente › Módulos del juego › INSTALAR**, con PES cerrado.

### Volver atrás

- Restaura `sider.ini.phoenix-<fecha>` (o tu `sider.ini.respaldo`) como `sider.ini` en las dos carpetas.
- O en Link pulsa **APAGAR**: comenta la línea `lua.module` y deja el archivo por si lo vuelves a encender.

---

## Parte 3 · Fichajes como CAMBIOS (generados en cada PC sobre SU base)

> Origen: `docs/FICHAJES-COMO-CAMBIOS.md` · último cambio 442861a 2026-10-12 · 12502 bytes. Copia íntegra.

Estado: **implementado en Phoenix Link 0.2.47** (2026-10-12; la 0.2.46 trajo los fichajes y la 0.2.47 el detector de parche/base por evidencia y la lista por nombres). Probado con bases falsas, estructuras falsas de cada caso y COPIAS de las bases reales de Fralex; **sin probar todavía dentro de PES** (ver §7). Este documento sustituye al informe de investigación de la mañana del 12-oct, cuya conclusión principal estaba equivocada (§2).

### 0. En una frase
Link ya no lleva ni copia ninguna base de datos. Lee la base REAL del PC (la del modo/parche activo), le aplica una lista corta de cambios «jugador → equipo» (`fichajes.json`) y deja solo `PlayerAssignment.bin` en `SiderAddons\livecpk\Phoenix-DB\…\pesdb\`. Player.bin queda el de la base. Se quita en un clic.

### 1. Formato de las bases (verificado con las bases reales)
Cada `.bin` de `CGP_database.cpk` (CPK sin comprimir, tabla `@UTF` enmascarada; el tamaño del bloque va en **little endian**) y de las carpetas `olmosjr23\Database` / `Phoenix-DB` tiene cabecera de 16 bytes `FF 10 81 'WESYS'` + `u32 tamaño comprimido` + `u32 tamaño real` + flujo **zlib** (`78 DA`). `Country.bin` va sin esa cabecera.

**PlayerAssignment.bin**: sin cabecera, filas de **16 bytes** = 4 × `u32` little endian:

| campo | significado |
|---|---|
| 0 | id de fila (único; no es estable entre bases) |
| 1 | **player_id** (un jugador puede salir en 2 filas: club + selección) |
| 2 | **team_id** |
| 3 | empaquetado: `>>10` = puesto en la plantilla (único por equipo), `&1023` = número bajo (no siempre único; probable dorsal, sin confirmar) |

Filas: 26 (olmosjr23) = 19.747, CPK 26 = 20.398, 27 = 19.612, UEFAGOL 27 = 19.489, Clásicos = 15.562. En ninguna base hay un equipo con más de 40 filas.

**Player.bin**: cabecera de 8 bytes + registros de 312 bytes (id = primeros 4 bytes; 0 = hueco); el archivo termina con el último registro recortado. **Team.bin** no se decodifica: un equipo es válido si la base le da al menos una fila.

### 2. Corrección: Phoenix-DB no tenía fichajes
La investigación inicial comparó Phoenix-DB con la CPK del 26 y encontró 175 «cambios de Fralex» (12 altas reales, 37 «Player Identidad», 126 bajas). **Era un error de base:** los modos 26 no usan la CPK, usan `olmosjr23\Database` (esa raíz va en su `sider.ini`). Comparado con esa base, el `PlayerAssignment.bin` de Phoenix-DB es **idéntico** (mismo md5 `7958bbaf…`) y su `Player.bin` también (idéntico al descomprimir; solo cambia la compresión: 1.769.252 vs 1.777.004 bytes). Es decir: Phoenix-DB es una **copia de la base de olmosjr23**, sin ningún fichaje. El único fichaje que pasó por Sync fue una prueba («Jugador 114506: equipo 101», 10-oct) que se deshizo.

Consecuencias:
- La lista de fichajes que lleva Link va **vacía** (`SmashSoda\phoenix\fichajes\fichajes.json`). Los fichajes reales los pondrá Fralex (o vendrán de la web/Mercado, §5).
- Las 175 diferencias eran solo la distancia entre la CPK 26 y olmosjr23: no hay que guardarlas.
- La base «real» que ve el juego depende del modo: **26 → olmosjr23\Database**; **27, UEFAGOL 27, Clásicos → el CGP_database.cpk del modo**.

### 3. Diseño implementado
- **Código:** `SmashSoda\phoenix\link\Fichajes.h/.cpp` (inflate propio, lector de CPK, formato `.bin`, reglas, aplicar/quitar), `web\AccionesFichajesWeb.cpp` (acciones `fichajes.estado` y `fichajes.accion`), `ModulosJuego` (`raicesActivas`, `carpetasModos`, `asegurarRaizPhoenixDB`), UI en SYNC › Puente › «Fichajes de Phoenix».
- **Sitios:** la raíz del juego y cada modo del cambiador (`ConmeGol Extras\<modo>`). El lanzador copia `SiderAddons` del modo sobre la raíz al iniciar, así que el resultado de cada modo viaja con él.
- **Base de cada sitio (detector por evidencia, no por nombres de ConmeGOL).** Para cada archivo (`PlayerAssignment.bin`, `Player.bin`, `Team.bin`) Link hace lo mismo que el juego, en este orden:
  1. recorre las `cpk.root` activas del `sider.ini` que manda ahí (sin Phoenix-DB), en orden de prioridad, y toma la primera cuya carpeta `common/etc/pesdb` trae el archivo (la carpeta puede llamarse como sea: `olmosjr23/Database`, `livecpk/Mi Base`…);
  2. si ninguna lo trae, mira los CPK: los `.cpk` de la carpeta `download` del modo (o de la raíz) y de `Data`, leyendo solo la cabecera y la tabla del CPK (los `dt80_*.cpk` pesan GB), y se queda con el de **mejor prioridad** según `DpFileList.bin` (1 = la más alta; el CPK propio de un modo manda sobre el resto). Si dos CPK empatan en prioridad con bases distintas, **no se adivina**.
  Lo que se encuentra se **valida por estructura** (cabecera `WESYS`, zlib, filas de 16 bytes, jugadores y equipos con sentido, máx. 40 por equipo). Si no se lee o el formato es otro: **no se crea nada** y el mensaje dice «qué detecté» y «qué hacer».
- **Parche detectado (nombre legible):** por la huella del `PlayerAssignment.bin` (ConmeGOL 26 olmosjr23, Patch 26, Patch 27, UEFAGOL 27, Clásicos), si no por el nombre de la carpeta del modo, por `version_actual.txt` (raíz) o por el CPK oficial («PES 2021 original»). Si no lo reconoce por nombre pero la base se lee y valida: «parche desconocido (…)» y se usa como **base genérica**, y la interfaz lo dice. Cada análisis guarda la evidencia («cómo lo supe»).

| Estructura | ¿Detectada? |
|---|---|
| Base en `cpk.root` con cualquier nombre de carpeta; varias bases (gana la primera) | Sí |
| Parche con su CPK en `download` (cualquier nombre de `.cpk`, con o sin `DpFileList.bin`) | Sí (empate de prioridad con bases distintas: no se adivina) |
| Cambiador de modos (varios `ConmeGol Extras/<modo>`), cada uno con su base | Sí, cada modo por separado |
| Parche desconocido con base válida (Sudamerican, PESEdit…; no probados con copias reales) | Sí, como «base genérica», si la base está en formato WESYS |
| PES original sin parche (`dt80_*.cpk`, `Data/dt*.cpk`) | **No:** esos `.bin` van cifrados con un formato propio del juego. Se reconoce «PES 2021 original» y no se crea nada |
| CPK con los `.bin` comprimidos (CRILAYLA), bases con otro formato, sin Sider | No (no se crea nada y se explica por qué) |
- **Qué escribe:** solo `PlayerAssignment.bin` (cabecera WESYS y zlib «almacenado», válido para cualquier lector zlib) y `fichajes_aplicados.json` (estado: fecha, huella de la base, de la lista y de lo escrito). Si Phoenix-DB tiene un `Player.bin` que **no** es el de la base (p. ej. la copia vieja del 26 en un modo 27), se **aparta** a `Phoenix-DB\_antes-de-fichajes-<sello>\` (no se borra) para no tapar a los jugadores del parche.
- **sider.ini:** si falta, añade una línea `cpk.root = ".\livecpk\Phoenix-DB"` (antes de la base si la hay; si no, antes de la primera `cpk.root`), con copia `sider.ini.phoenix-<sello>` y vuelta atrás.

### 4. Lista de cambios (`fichajes.json`, versión 1)
```json
{ "version": 1, "regla": "relajada",
  "cambios": [ { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto opcional" } ] }
```
- **Versión 2 (compatible con la 1): referirse por NOMBRE.** Como los `player_id` y `team_id` cambian entre parches, cada cambio puede ir por nombres, resueltos contra la base de ESE PC:
  ```json
  { "version": 2, "cambios": [
    { "jugador": "Cristiano Ronaldo", "equipo_origen": "Al-Nassr FC", "equipo_destino": "Real Madrid CF", "nota": "texto" },
    { "player_id": 114506, "equipo_destino": "FC Barcelona" } ] }
  ```
  El jugador (`jugador` o `player_id`), el destino (`equipo_destino`/`equipo` o `team_id`) y el origen opcional (`equipo_origen` u `origen`) pueden mezclarse, pero cada campo va por id **o** por nombre. La comparación es **exacta** (sin mayúsculas, acentos, espacios ni signos: «Kylian Mbappé» = «kylian mbappe») contra el nombre completo del jugador (`Player.bin`) y el nombre del equipo (`Team.bin`). Si el nombre de un jugador sale varias veces, el equipo de origen lo desambigua; si sigue sin ser único, o un equipo tiene el nombre repetido, **se salta** con el motivo (`jugador_ambiguo`, `equipo_ambiguo`, `jugador_no_encontrado`, `equipo_no_encontrado`, `nombres_no_disponibles`). La versión 1 solo admite ids.
- Orden de búsqueda: `%APPDATA%' + B + 'Phoenix Mercado' + B + 'fichajes' + B + 'fichajes.json` (punto de entrada para Sync/Mercado) y, si no existe, `<exe>\fichajes\fichajes.json` (el que lleva Link).
- **Reglas** por cada cambio: el jugador debe existir en `Player.bin` de la base y el equipo en ella; máximo 40 por equipo; si ya está en el equipo destino no hace nada; sin `origen` mueve su única fila (si tiene dos, se salta por ambiguo); con `origen` mueve esa fila. **Relajada** (por defecto): si el jugador ya no está en `origen`, gana el cambio de la lista; **estricta**: se salta. No hay bajas (no se puede sacar a un jugador de un equipo) ni se usan equipos que no existan (p. ej. 9107/9108 del 26).
- Fila movida: conserva su id; puesto = primer hueco libre del equipo destino; número bajo = el suyo si está libre, si no el primero libre. Altas: id = máximo+1.
- **Validación antes de escribir (todo o nada):** ids de fila únicos, sin pares (jugador, equipo) repetidos, puesto único por equipo, ≤ 40 por equipo; después de escribir se vuelve a leer y se comprueba el sha256.

### 5. Quién guarda y quién aplica
- **Link aplica** (esto). **Quién guarda la lista:** Sync/Mercado deberá dejar `fichajes.json` en `%APPDATA%\Phoenix Mercado\fichajes\`; el formato y las reglas están en `docs/PROMPT-WEB-fichajes-lista.md` (para pegar en el chat WEB/Sync). Mientras tanto la lista se edita a mano. La entrega antigua de Sync (`PlayerAssignment.bin` ya hecho dentro de Phoenix-DB) sigue funcionando; si pisa un archivo generado por Link, el estado pasa a «lo cambió otra herramienta» y Link no lo borra al quitar.

### 6. Seguridad, copia y vuelta atrás
- Solo con PES y `sider.exe` cerrados. Copia de lo que había antes (`_antes-de-fichajes-<sello>`) y de cada `sider.ini`. Escritura atómica (`.tmp` + reemplazo).
- **Quitar** (un clic): borra solo lo que generó Link (si su sha coincide con el estado guardado), deja la copia y el juego vuelve a la base original del parche. El `sider.ini` conserva su línea Phoenix-DB (inofensiva sin archivos).
- **Doctor / diagnóstico:** el informe para depurar y el diagnóstico en claro llevan «Fichajes: N aplicados, M saltados en K sitios» y una línea por modo con los motivos de lo saltado.

### 7. Pruebas
- `pruebas-link\correr_fichajes.bat` (MSVC; el .exe se llama `fichajes_test`, sin «instal»): **88/88 bien** con las copias reales de las bases de Fralex (CPK 26/27/UEFAGOL 27/Clásicos y olmosjr23): zlib real con códigos dinámicos, bin, CPK, reglas, 40 por equipo, idempotente (aplicar dos veces = mismos bytes), quitar, base dañada o ilegible → no se crea nada. El juego real solo se lee; todo se copia a una carpeta temporal.
- 0.2.47: `correr_fichajes.bat` **148/148** (con y sin las bases reales): estructuras falsas de cada caso (vanilla con `dt*.cpk`, parche con base en carpeta, parche con CPK propio y prioridad por `DpFileList.bin`, cambiador con varios modos, varias bases con prioridades, empate de prioridad, base dañada, nombre desconocido con formato válido, sin ninguna base), un CPK falso generado por el propio test (tabla `@UTF` enmascarada), la lista por nombres, y las copias reales (26 olmosjr23, 27, UEFAGOL 27, Clásicos): parche detectado por huella, base por `cpk.root` en el 26 y por CPK en el resto, idempotente. La data original de PES (`Data/dt*.cpk`, abierta en solo lectura) se reconoce y no se crea nada.
- `correr_escenarios.bat`: 55/55 (se actualizó E13: la advertencia de «Phoenix-DB sobre una base 27» ahora exige un Player.bin dentro de Phoenix-DB).
- `modulos_test`, `parche27_test` y `quitar_modulo_test`: **TODO OK** con `pruebas-link/correr_modulos.bat`. (Antes dije que fallaban: era mi forma de lanzarlos, con argumentos equivocados. Con el `sider.ini` real de la raíz del juego de Fralex, `parche27_test` falla porque ese `sider.ini` ya lleva las líneas de Phoenix; la prueba espera el del parche sin Phoenix.)
- **Sin probar en PES (pendiente de Fralex):** que el juego acepte el `.bin` generado (zlib sin compresión), que Sider cargue la raíz Phoenix-DB que se añade al `sider.ini` del 27, y el significado de puesto/número bajo (un fichaje de prueba: ver `docs/BETA-1-GUIA-AMIGO.md`, «Probar un fichaje»).

---

## Parte 4 · Sync y PC del amigo: análisis de escenarios (beta 1 · 0.2.45)

> Origen: `docs/SYNC-ESCENARIOS.md` · último cambio 4102fd7 2026-10-12 · 9902 bytes. Copia íntegra.

12 oct 2026. Pregunta de Fralex: «¿cómo me aseguro de que a mi amigo le funcionará bien el Sync?». Aquí van los escenarios, qué pasa hoy, qué se arregló y **cómo se probó**.

**Estados:** ✅ cubierto y probado (prueba automática en carpetas falsas) · 🟡 cubierto, sin probar (por código o por diseño) · 🔴 pendiente en la PC real del amigo.
**Pruebas:** `pruebas-link\correr_escenarios.bat` (54 comprobaciones, 0 fallos, usa el mismo código de Link y juegos FALSOS) y las pruebas de los `.bat` hechas a mano en carpetas temporales. El juego real de Fralex solo se **leyó**.
Regla general: **o todo o nada** (se comprueba todo antes de escribir; si algo falla a medias, los `sider.ini` ya escritos vuelven a su copia) y **Phoenix-DB solo con base de parche reconocida**.

### A. Juego del amigo

| # | Escenario | Qué pasa | Arreglo / cómo queda | Estado y prueba |
|---|---|---|---|---|
| A1 | Ruta con espacios, acentos, ñ, OneDrive, otro disco | Funciona | Prueba E01; el informe avisa si está en OneDrive o en Archivos de programa | ✅ E01 (ruta `OneDrive - … ñandú\Juegos é\…`) y `.bat` con acentos |
| A2 | Carpeta protegida (Archivos de programa) o de solo lectura | Escribir falla | Mensaje: «¿PES abierto, antivirus, disco lleno o solo lectura? Prueba como administrador»; el `.bat` rechaza un `sider.ini` de solo lectura sin pisarlo; todo vuelve atrás | ✅ E09 (solo lectura, vuelta atrás) y `.bat` solo lectura · 🔴 Archivos de programa real |
| A3 | PES sin Sider (no hay `SiderAddons\sider.ini`) | Se niega, no crea nada | Mensaje claro; el informe lo explica («ese juego no usa Sider») | ✅ E02 |
| A4 | Sider 7.3.3 (sin `goal_scored`) | Marcador sí; goleador no | El informe lee la versión de `sider.log` y lo explica | 🟡 (lectura de versión sin probar con un log antiguo) · 🔴 en su PC |
| A5 | PES vanilla / otro parche sin cambiador, sin base | Instala módulos; no pone Phoenix-DB | Mensaje «Phoenix-DB no se puso… es normal» | ✅ E07 |
| A6 | Parche 26, 27 o ambos, con cambiador, varios modos, modo activo ≠ modo instalado | Se instala en la raíz y en cada modo; Phoenix-DB solo donde hay base | Estado «instalado» aunque falte base; reconoce modos 26 (propios) y 27 (raíz) | ✅ E06, E13 |
| A7 | `sider.ini` vacío o sin lista de módulos | Se niega, sin tocar | Mensaje «no tiene lista de módulos» | ✅ E03 |
| A8 | `sider.ini` con BOM / LF / CRLF | Se respeta el formato | **Bug corregido:** con BOM, una línea insertada arriba dejaba el BOM a mitad del archivo | ✅ E04 |
| A9 | Líneas de Phoenix duplicadas, comentadas, en otra posición, editadas a mano | Queda **una** línea; la comentada se enciende | **Arreglado:** antes se dejaban duplicadas | ✅ E05 |
| A10 | Falta la carpeta `modules` en un modo | Se niega, nada cambia | Mensaje con el modo | ✅ E10 |
| A11 | Nombre de módulo raro (`..\x.lua`) | Se niega | — | ✅ E14 |

### B. Estado del juego y transacciones

| # | Escenario | Qué pasa | Arreglo | Estado y prueba |
|---|---|---|---|---|
| B1 | PES/Sider abierto al instalar | Se niega con mensaje | Link y los dos `.bat` | ✅ E08 (proceso falso) y `.bat` |
| B2 | PES se abre justo después | Sider lee los módulos al arrancar | Nada que hacer: arranca ya con ellos | 🟡 |
| B3 | Antivirus/Windows bloquea o archivo en uso | Falla sin romper | `.bat`: mensaje claro y nada cambia; Link: mensaje + vuelta atrás | ✅ E09 (en uso) y `.bat` con archivo bloqueado |
| B4 | Disco lleno | La copia o la escritura falla | Mensaje «¿disco lleno?» y vuelta atrás | 🔴 (no se puede simular con seguridad) |
| B5 | Actualización del parche borra las líneas | Estado «perdido» → REINSTALAR | Tarjeta «Qué funciona en tu juego» lo dice con el paso | ✅ E12 |
| B6 | Instalar dos veces | Sin cambios, sin copias nuevas | — | ✅ E05, E11 y `.bat` |
| B7 | Desinstalar y reinstalar | `sider.ini` vuelve idéntico al original | Los `.lua` se apartan, no se borran | ✅ E11 y `.bat` |
| B8 | Volver a una versión anterior | Copias con sello `sider.ini.phoenix-…` y `.lua.antes-v…` | Se comprueba que existe una copia idéntica al original | ✅ E11 |
| B9 | Fallo a medias en varios modos | Vuelta atrás de los ya escritos | — | ✅ E09 · `.bat` 🟡 (probado el aviso; la restauración por código, no forzada) |

### C. Datos (fichajes / Phoenix-DB)

| # | Escenario | Qué pasa | Arreglo | Estado y prueba |
|---|---|---|---|---|
| C1 | Base no reconocida (parche 27, Clásicos…) | Phoenix-DB **no** se pone ni se crea su carpeta | Mensaje explicativo («no es un error»); Sync › «Fichajes: desactivados en este parche (próximamente)» | ✅ E06, E07 |
| C2 | Phoenix-DB ya puesta sobre un 27 (instalación vieja) | Taparía jugadores | Advertencia «desinstala y reinstala» | ✅ E13 |
| C3 | Player.bin / PlayerAssignment.bin de otra versión | Solo llegan si hay Phoenix-DB; sin base no hay dónde | El repartidor de datos nunca crea carpetas del juego | 🟡 (pruebas previas de `entrega_test`) |
| C4 | Orden de las raíces `cpk.root` | Phoenix-DB va justo antes de la base | El Doctor avisa si queda detrás | ✅ E04/E05 (posición) |
| C5 | Conflicto con otro módulo (DR_KitMixer, kserv, Chatty…) | Phoenix no toca esos módulos; solo añade sus líneas debajo del último | Los demás módulos quedan intactos | ✅ E05 (los demás intactos) · 🔴 conflicto real en partido |
| C6 | Límite de módulos de Sider | Se añaden solo 2 líneas | — | 🔴 |

### D. Red y cuenta (Link ↔ web)

| # | Escenario | Qué pasa | Estado |
|---|---|---|---|
| D1 | Sin internet / web caída / proxy o VPN | Link muestra «SIN CONEXIÓN» y reintenta con calma; el juego y la sala local siguen | 🟡 por diseño (sin probar en esta sesión) |
| D2 | Ruta nueva aún sin desplegar (404) | Link espera 5 min antes de reintentar | 🟡 (según REGISTRO-LINK) |
| D3 | Token caducado / Parsec no vinculado / reloj mal / sin cuenta web | Se ve en CUENTA WEB y en Diagnóstico; la sala funciona sin cuenta web | 🔴 |
| D4 | Sala privada vs pública | Ajuste de visibilidad existente | 🔴 |

### E. Link del amigo

| # | Escenario | Qué pasa | Arreglo | Estado |
|---|---|---|---|---|
| E1 | Sin ViGEmBus | Nadie puede jugar | El instalador lo instala si falta; Diagnóstico lo marca en rojo | ✅ (rama «ya instalado» del instalador, probada) · 🔴 instalación real |
| E2 | Sin WebView2 | Interfaz ImGui de respaldo | El instalador lo instala (instalador oficial de Microsoft) | 🟡 · 🔴 |
| E3 | Sin VC++ runtime | — | Se compila en su PC con Visual Studio, que ya lo trae | 🟡 |
| E4 | Windows 10 vs 11, antivirus, SmartScreen | — | Se compila en su PC: sin marca de «descargado»; si un antivirus borra el `.exe`, el instalador no podrá corregirlo | 🔴 |
| E5 | Dos copias de Link abiertas | Se pelean por mandos y sala | El informe lo detecta y lo explica. **No hay bloqueo automático** (no lo añadí: tocar el arranque es arriesgado) | 🟡 detección · bloqueo pendiente |
| E6 | Configuración de una versión vieja | Faltan claves → valores por defecto; archivo dañado → copia `.bak` | Guardado atómico + `.bak` | 🟡 (código; sin prueba automática) |
| E7 | Ruta de configuración con acentos/espacios, usuario sin permisos en AppData | Diagnóstico «Carpeta de configuración» en rojo | — | 🟡 |
| E8 | Resolución/escala pequeña, idioma | Interfaz responsive; ES/EN | — | 🟡 (se vio en el simulador) |
| E9 | Instalar sobre una instalación existente | Se actualiza, no toca ajustes | — | ✅ (mensaje de actualización y copia sobre carpeta existente) |

### F. Overlay

| # | Escenario | Qué pasa | Estado |
|---|---|---|---|
| F1 | PhoenixGlass vs Smash Glass viejo | El instalador baja **PhoenixGlass.exe** (sha256) y no toca un `SmashGlass.exe` existente; Link prefiere PhoenixGlass | ✅ descarga con sha bueno/malo/404 · 🔴 hasta publicar el archivo en GitHub |
| F2 | Sin WebView2 | El overlay no abre; Link sigue | 🟡 |
| F3 | PES a pantalla completa | Solo se ve el aviso dentro del juego (panel de Sider) | 🔴 |
| F4 | Puerto del WebSocket ocupado | Diagnóstico › Overlay lo marca | 🔴 |

### G. Diagnóstico («qué pasa» + «qué hacer»)

**COPIAR INFORME** (Sala › Conexión › Diagnóstico del PC) ahora incluye una sección «Qué pasa y qué hacer (en claro)» con un aviso por cada caso detectable: juego no encontrado, sin Sider, PES abierto, Archivos de programa u OneDrive, `sider.ini`/`modules` de solo lectura, versión de Sider, módulos perdidos/viejos/sin instalar/no cargados, Phoenix-DB no puesto (explicado como normal), advertencias de parche y varias copias de Link. Tapa tu nombre de usuario de Windows en las rutas y no lleva contraseñas ni claves. Estado: 🟡 (compila; se vio el texto en el simulador; falta verlo en Link real).

### Lista corta para la primera sesión con el amigo
1. Antes de nada: Sala › Conexión › Diagnóstico del PC en verde (WebView2, ViGEmBus, Parsec).
2. SYNC › «Qué funciona en tu juego»: juego conocido, módulos al día. Si algo está en ámbar/rojo, **COPIAR INFORME**.
3. Con PES **cerrado**, INSTALAR ambos módulos → «INSTALADO». Abrir PES y mirar `sider.log`: `[phoenix.lua]` sin «is NOT activated», y la versión de Sider (7.3.4).
4. Fichajes: en parche 27 deben salir «desactivados (próximamente)»; en parche 26, activos.
5. Probar a mano en su PC: carpeta con acentos/espacios, OneDrive, cerrar Link y abrirlo dos veces, desinstalar y reinstalar.
6. Anotar y mandar el informe si algo falla.

### Qué NO hice (y por qué)
- No bloqueé la segunda copia de Link (riesgo en el arranque); queda detectada y explicada.
- No simulé disco lleno ni antivirus: peligroso en una PC con datos.
- No toqué el juego real de Fralex.

---

## Parte 5 · Parche ConmeGOL 27: cómo funciona y cómo se instala Phoenix (2026-10-12, Link 0.2.26)

> Origen: `docs/PARCHE-CONMEGOL-27.md` · último cambio 18d9f54 2026-10-13 · 8008 bytes. Copia íntegra.

Fralex instaló la actualización del parche (modos **ConmeGOL Patch 27**, **UEFAGOL 27** y **ConmeGOL Clásicos**). Esta guía dice qué cambió, con la evidencia, y qué hace Phoenix Link ahora.

### 1. Cómo funciona el lanzador nuevo (PES2021 Start.exe)

**Evidencia:** se leyó el código del lanzador (es un programa de Python empaquetado con PyInstaller; solo se leyó, no se modificó nada).

Al pulsar «Iniciar PES 2021» el lanzador hace, en este orden:

1. Comprueba que esté puesta la ruta de guardados de Konami.
2. Ejecuta `robocopy "ConmeGol Extras\<modo>" "<carpeta del juego>" /E /IS /IT`.
   Copia **todo** el contenido de la carpeta del modo encima de la raíz: `CPY.ini`, `download\CGP_database.cpk` (la base de datos del modo) y `SiderAddons\` (content, livecpk, modules y, si el modo lo trae, `sider.ini`).
   No lleva `/MIR` ni `/PURGE`: **no borra nada** de la raíz. Lo que no está en el modo se queda como estaba (por ejemplo, nuestras luas en `SiderAddons\modules`).
3. Escribe el nombre del modo en `version_actual.txt`.
4. Arranca `SiderAddons\sider.exe` (con esa carpeta como directorio de trabajo) y luego `PES2021.exe`.

Solo ofrece 3 tarjetas: Patch 27, UEFAGOL 27 y Clásicos. Los modos 26 siguen en `ConmeGol Extras`, pero este lanzador ya no los muestra.

### 2. Qué `sider.ini` usa cada modo

- Los modos **27 no traen `sider.ini`** en su `SiderAddons`. Por el punto 2 de arriba, al iniciarlos queda el `sider.ini` que ya hay en la raíz. El de la raíz es el que dejó la actualización (3455 bytes, fecha 09-oct 22:17: el del paquete del parche), sin líneas de Phoenix.
- Los modos **26 traen su `sider.ini`** y, al iniciarlos con el lanzador viejo, se copia sobre la raíz (por eso antes la raíz tenía las líneas de Phoenix: venían del modo).
- Sider siempre lee `SiderAddons\sider.ini` y carga los módulos de `SiderAddons\modules\`. Eso se ve en el `sider.log`: `sider dir: …\SiderAddons\`.

**Conclusión:** para los modos 27 la raíz es el único sitio que cuenta. Ahí tienen que estar `phoenix.lua` y `phoenix_referee.lua` y sus dos líneas `lua.module`.

**Suposición, no verificada:** si alguien arranca un modo 26 y después uno 27, la raíz se queda con el `sider.ini` del 26 (los 27 no lo reemplazan). Se deduce del `robocopy`; no se probó arrancando el juego. Link lo avisa (ver §4).

### 3. La base de datos del parche 27

- No hay ningún `Player.bin` suelto del 27. Va **dentro** de `ConmeGol Extras\<modo>\download\CGP_database.cpk`, que el lanzador copia a `download\` de la raíz al iniciar. (La raíz todavía tiene la del 26 hasta que se inicie un modo.)
- Se leyó la tabla de cada `.cpk` (solo lectura). El `Player.bin` de cada uno:

| Base | Tamaño de Player.bin |
|---|---|
| Patch 26 | 1.769.541 |
| **Patch 27** | **1.803.705** |
| **UEFAGOL 27** | **1.828.345** |
| Clásicos 27 | 936.160 (igual que «Clasicos (No LM)») |
| **Phoenix-DB** (los fichajes de Fralex, hechos sobre la base 26) | 1.769.252 |

- Cada modo tiene su propia base. Phoenix-DB sale de la 26: si se pusiera en el 27, **taparía a los jugadores del parche nuevo** con los del viejo.
- **Actualizado (Link 0.2.46/0.2.47):** ya no se copia ninguna base. Los fichajes se generan sobre la base real de cada PC, que Link detecta sola (ver `docs/FICHAJES-COMO-CAMBIOS.md`); Phoenix-DB solo lleva un `PlayerAssignment.bin` generado, nunca un `Player.bin`. Lo que sigue es el estado anterior (hasta la 0.2.45): la regla era que **sin una base reconocida en ese `sider.ini`, Phoenix-DB NO se pone.** La raíz del 27 no tiene ninguna `cpk.root` con `Player.bin` ni la de `olmosjr23\Database`, así que no se puso. No se copió ningún `Player.bin` de Fralex a otros sitios.
- ~~Para tener fichajes sobre el parche 27 hace falta una base propia por parche~~: ya no hace falta; los fichajes son cambios sobre la base del parche activo (0.2.46).

### 4. Qué cambió en Phoenix Link (0.2.26)

- **Detecta la pérdida.** Si el `.lua` está en `modules\` pero su línea ya no está en el `sider.ini` (ni comentada) y hay pruebas de que Phoenix ya estuvo ahí (otro destino sí la tiene, o hay copias `sider.ini.phoenix-…`), «Módulos del juego» muestra **«QUEDÓ SIN CARGAR TRAS UNA ACTUALIZACIÓN: REINSTALAR»** y el botón **REINSTALAR**.
- **Phoenix-DB solo con base.** `phoenix.lua` ya no exige la raíz Phoenix-DB en un `sider.ini` sin base reconocida (antes quedaba «a medias» para siempre) ni crea su carpeta vacía ahí. El mensaje dice dónde no se puso.
- **Muestra la estructura del parche:** último modo lanzado, modos que usan el `sider.ini` de la raíz y modos con el suyo.
- **Avisa** si se juega un modo 27 y la raíz tiene Phoenix-DB o la base `olmosjr23\Database` de un modo 26.
- **Cierre más estricto:** además de PES2021.exe, también exige cerrar `sider.exe`.
- **Copias de seguridad sin pisarse:** si dos cambios caen en el mismo segundo, la segunda copia lleva `-2` (antes la pisaba y se perdía el `sider.ini` original). Lo encontró la prueba nueva.
- Las reglas de siempre siguen: solo con el juego cerrado, copia con sello antes de cada cambio, una línea por módulo, nada más del `sider.ini` cambia, y «Desinstalar» disponible.

Los modos 27 no se tratan como destinos propios (no tienen `sider.ini`); los cubre la raíz. Los modos 26 siguen siendo destinos mientras existan.

### 5. Lo hecho en el juego de Fralex (2026-10-12 01:23)

Con PES2021.exe, sider.exe, Settings.exe y «PES2021 Start.exe» cerrados (comprobado con `tasklist`), usando el código del instalador:

- `SiderAddons\sider.ini` de la raíz: se añadieron **2 líneas** (`lua.module = "phoenix_referee.lua"` y `"phoenix.lua"`, debajo del último módulo activo). Un `diff` contra la copia confirma que no cambió nada más.
- Copias: `sider.ini.phoenix-20261012-012310` (idéntica al `sider.ini` del parche 27, byte a byte) y `…-2` (con la línea de referee).
- `phoenix.lua` (v0.18b, sha1 `0e6aebecc923`) y `phoenix_referee.lua` (v1.2, `b3d7b1312723`) ya estaban en `modules\` y eran idénticos: no se copiaron.
- **Phoenix-DB: no se puso** (sin base reconocida), como corresponde.
- Los 3 modos 26 no se tocaron (ya tenían una línea de cada).
- Falta ver en `sider.log`, tras el próximo arranque, `Loading module: phoenix.lua` / `phoenix_referee.lua`.

### 6. Pruebas

- `pruebas-link/parche27_test.cpp` imita la estructura 27 en una copia y también corre con copias de los `sider.ini` reales (el 27 y el de un modo 26): TODO OK.
- `modulos_test` y `quitar_modulo_test` siguen en TODO OK.
- `pruebas-link/phoenix_en_juego_real.cpp`: herramienta para ver o aplicar el paquete en el juego real con el mismo código del instalador.

### 6. Actualizar el parche pegando una carpeta nueva (observado el 2026-10-13)

Fralex pegó una carpeta **ConmeGol Extras** nueva sin borrar la anterior y reemplazó archivos de la raíz. Lo que se vio en su PC (solo lectura):

- **Extras** quedó con **3 modos** (ConmeGOL Clasicos, ConmeGOL Patch 27, UEFAGOL 27); los modos 26 desaparecieron. Ya no hay destinos de modos 26: solo la raíz.
- El **sider.ini de la raíz volvió al del paquete** (3455 bytes, fecha 09-oct 22:17): **sin las líneas de Phoenix**. Los .lua seguían en `modules`, así que Link los marcó «perdido». Es decir: **pegar una actualización encima borra las líneas de Phoenix del sider.ini**, aunque el lanzador por sí solo no lo haga. Se arregla con Sync › PREPARAR TODO › ARREGLAR TODO (PES y Sider cerrados, con copia de sider.ini).
- La raíz y el modo Patch 27 tienen el mismo CGP_database.cpk (md5 igual); UEFAGOL 27 y Clásicos tienen otro cada uno.
- El Phoenix-DB de la raíz (Player.bin 9-oct y PlayerAssignment.bin 10-oct) **no lo generó el motor actual** (no hay fichajes_aplicados.json): no se activa sin más.
- Recomendación para quien actualiza: borrar `ConmeGol Extras` antes de pegar la nueva, y después pulsar ARREGLAR TODO.

---

## Parte 6 · Traspasos EN VIVO entre PCs con Phoenix

> Origen: `docs/TRASPASOS-EN-VIVO.md` · último cambio 47fc971 2026-10-13 · 18015 bytes. Copia íntegra.

Estado: **0.2.79 (beta, apagado por defecto)**. Los lotes viajan **por la web de Phoenix (Supabase, función `traspasos`, ya desplegada)** por defecto; la carpeta compartida queda como alternativa avanzada. Pedido de Fralex: «que yo haga un movimiento en Editar de PES (base normal; Liga Máster más adelante) y que automáticamente en el PC de mi amigo se haga el traspaso también, en tiempo real, en un momento seguro».

### 0. En una frase
Fralex hace un traspaso en **Editar** y **guarda**; Link (su PC) ve que el option file cambió, saca *qué jugador cambió de equipo* y lo publica como un **lote** diminuto; el Link del amigo lo recoge, lo aplica sobre **su propia base** (`Phoenix-DB\PlayerAssignment.bin`, con el sistema de Fichajes) y avisa a `phoenix.lua`, que recarga **solo en un momento seguro** según el modo del amigo. Solo viajan cambios «jugador → equipo», **nunca bases**.

### 1. Qué está probado y qué no (honesto)
| | Estado | Evidencia |
|---|---|---|
| Leer el option file real y sacar quién está en cada plantilla | ✅ probado con **copias de los option files reales de Fralex** | 749 equipos, 19.364 jugadores; comparando sus respaldos detecta exactamente los traspasos que Sync hizo (p. ej. 162114 Barça→Arsenal) y 0 cuando no hay cambios |
| Comparar dos fotos → traspasos (ignora altas/bajas/convocatorias/dorsales) | ✅ | pruebas `traspasos_test` (93/93) |
| Lote, huella, idempotencia, transporte en carpeta, transporte web (hub **falso**) | ✅ lógica | 93/93 |
| Aplicar en el PC del amigo (por ids, por nombres, otro parche, PES abierto/cerrado, deshacer) | ✅ en **juegos falsos** y con **copias de las bases reales** (modos 26, 27 y raíz) | Yamal Barça→Arsenal: publicado desde un option file real, nombres reales «Lamine Yamal / FC Barcelona / Arsenal FC», aplicado en los 3 sitios del amigo y deshecho |
| **PES guarda el option file al guardar/salir de Editar** | ❌ **sin probar** | Sospecha fuerte: `EDIT00000000` de Documents no cambia solo (la última vez fue Sync, 11-oct 16:53, y el juego se usó después). Falta que Fralex haga un traspaso real y guarde |
| El juego del amigo **recarga** el `PlayerAssignment.bin` nuevo | 🟡 ya probado antes para fichajes (Phoenix-DB + recarga) pero **no con este flujo** | ver `PLAN-PAQUETE-LUAS-PHOENIX.md` («Lo que ya lograste») |
| **Cliente web de Link** (código por anfitrión, seguir con el código, lotes, esperas) | ✅ lógica probada contra un hub falso que **imita el contrato real** | 153/153: 404, 401, 403 no-host, 429, 503, timeouts con backoff, 500, respuesta corrupta, lotes repetidos/alterados, páginas, carpeta→web |
| **La web real** (función `traspasos`, migración 105) | 🟡 **desplegada y probada por la sesión principal** (6 rutas, 29 comprobaciones); desde Link solo comprobado **sin token** (401 `TOKEN_FALTANTE` / 404 `RUTA_NO_EXISTE`) | falta la prueba de Link **con token real** de punta a punta con Fralex |

### 2. Fase 1 · CAPTURA (anfitrión, solo lectura)
**Elegida: vigilar el option file** (`Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<id>\save\EDIT00000000`).
- Link lo lee cada ~3 s **solo si cambió** (tamaño + fecha) y **solo cuando lleva 2 pasos sin cambiar** (PES ya terminó de escribir). Lo descifra (libpesXcrypter, dominio público; copiada a `SmashSoda\phoenix\terceros\pesxcrypter`) y saca, por equipo, los 40 jugadores de la plantilla (posiciones verificadas por Phoenix Sync).
- **Traspaso** = un jugador que **sale de un equipo y entra en otro** respecto a la foto anterior. Todo lo demás se cuenta como «ignorado» (altas/convocatorias, bajas, varios a la vez). Dorsales, tácticas, nombres, Liga Máster (`ML…`): no se miran.
- La primera vez solo guarda la **foto de partida** (no publica nada). Si Fralex cambia de modo de ConmeGOL y el option file se reemplaza entero (más de 60 traspasos a la vez), **no se publica solo**: queda «pendiente masivo» para descartarlo.
- «Al toque» es **al guardar / salir de Editar**: PES no escribe el archivo hasta entonces, así que antes no hay nada que ver (límite de PES, no de Link).
- **Seguridad:** un archivo cortado o a medio guardar **cerraba Link** (el descifrador original se fía de los tamaños de la cabecera). Se añadió `pesxvalidar.c`: comprueba la cabecera contra el tamaño real antes de descifrar. Probado con basura y con el archivo cortado a la mitad: no se cierra, no se publica nada y reintenta.
- **Alternativas descartadas:** leer la tabla de asignación de la **memoria** (no hay dirección verificada; es lo que cerró PES el 9-oct; y cambiaría al instante pero sin guardar, lo que daría traspasos «fantasma» que el usuario aún puede deshacer en Editar). Vigilar `PES21_Hook\EDIT00000000`: es el option file que **trae** el parche, no el que PES guarda.

### 3. Fase 2 · TRANSPORTE (decisión de Fralex: SOLO la web; la carpeta es secundaria)
**Por defecto: la web de Phoenix (Supabase), función aparte `traspasos`** (migración 105, ya **desplegada**; contrato en `C:\dev\pes-liga-automatica\docs\TRASPASOS-EN-VIVO.md`). Base `https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/traspasos/v1`; rutas `POST /traspasos` (publicar), `GET /traspasos?desde=&limite=` (recoger), `POST /traspasos/codigo`, `/seguir`, `/dejar`, `GET /traspasos/siguiendo`. Misma auth que «liga» (token `phx_…` + `X-Phoenix-Version/Build`). Funciona aunque el amigo tenga Link apagado (se pone al día al abrir).
- **Cómo se enlazan anfitrión y amigo (sin carpetas):** el anfitrión (cuenta de **host aprobado**) tiene un **código de 8 caracteres** que comparte (puede **rotarlo**: el viejo deja de servir, quienes ya siguen siguen). El amigo lo **escribe** en su Link y empieza a recibir. Solo reciben quienes tienen el código; el contenido son solo ids y nombres de jugador/equipo. El amigo **no** necesita ser host (basta un dispositivo vinculado). Anfitrión y amigo son cuentas distintas; el modo «Prueba» se sigue a sí mismo (`prueba:true`) y sus lotes salen como «TUYO».
- **Cliente en Link:** `TransporteWeb` + `ClienteTraspasosWeb` sobre `PhoenixLink::peticionTraspasos` (token de Link + cabeceras `X-Phoenix-*`; sin token no se manda nada). El lote que se envía lleva **solo** los campos del contrato (`id, huella, creado, autor, parche, base_huella, cambios`); los nombres se recortan a los límites de la web (48/40/60) **antes** de calcular la huella.
- **Esperas y honestidad:** `404` sin código / `RUTA_NO_EXISTE` = «La web aún no tiene los traspasos en vivo» → espera **5 min** (el botón «Volver a comprobar» la salta); `401`/`TOKEN_*` → «vuelve a vincular tu cuenta» (60 s); `403 HOST_NO_AUTORIZADO` → «esta cuenta no está aprobada como anfitrión» (60 s); `429` → lo que pida `reintentar_en`; `503 LIGA_PAUSADA` → «la web tiene la liga en pausa» (60 s); red caída o 5xx → backoff 10, 20, 40, 80, 120 s; respuesta corrupta → «la web contestó algo raro» (30 s). Un error **de la acción** (código incorrecto, seguirse a uno mismo) se muestra tal cual **sin** estropear el estado de la conexión. Nada de esto rompe ni borra nada; el traspaso detectado **espera** y se publica cuando la web vuelve. `webDisponible` es **real**: `false` solo tras un 404, `true` solo tras una respuesta buena.
- **Sondeo suave** (el amigo pregunta cada ~8 s, **solo si «Recibir» está encendido** y ya sigue a alguien; si aún no sigue a nadie no llama a la web). Si llegan lotes, vuelve a preguntar enseguida hasta que la respuesta venga vacía («ponerse al día»). Idempotente por `id` de lote; orden por `seq` (**global** de la web); un lote duplicado o alterado (huella) no estropea nada.
- **Cada buzón tiene su propia numeración `seq`** (cada carpeta, y «la web»): Link recuerda hasta dónde leyó **en cada uno**; al cambiar de carpeta a web no se salta ni se repite nada (probado).
- **Alternativa avanzada: carpeta compartida** (Google Drive, OneDrive, red local; también «carpeta de prueba»): bloque plegado «Avanzado». Cada lote es `00000001-<id>.json`. Sigue funcionando y probada, pero no es el camino recomendado.
- Límites de la web: 12 publicaciones/min, 40 lecturas/min, 10 seguir/min, ≤32 KB, ≤100 seguidores, retención 30 días / 200 lotes por anfitrión, 1–60 cambios por lote (más de 60 en un guardado = «cambio masivo»: no se publica solo).
- **Lote** (el de Link): `{id (24 hex), huella (sha256 de los cambios), creado (UTC), autor, parche, base_huella, cambios:[{jugador_id, jugador, desde_id, desde, hasta_id, hasta}]}`; la web añade `seq`, `anfitrion` (nombre visible) y `anfitrion_id`. El mismo contenido enviado dos veces = mismo `id`: no se duplica (`repetido:true`). Un lote alterado (huella distinta), con ids inválidos o con origen = destino **se rechaza**. Los nombres salen de la **base real del anfitrión** (`leerNombresDeBase`), no del option file.

### 4. Fase 3 · APLICACIÓN (amigo)
1. Trae lotes con `seq` mayor al último visto (desordenados o repetidos no importan: se ordenan por `seq` y se descartan los ya vistos). Los lotes **propios** no se aplican.
2. **Cómo se aplica en su PC:** si su base es la misma que la del anfitrión (huella igual) → por ids; si es otro parche → **por nombres** (jugador y equipo, coincidencia exacta; si hay duda se salta con motivo); si es otro parche **y el lote no trae nombres** → se salta, **no se adivinan ids**.
3. Construye la lista efectiva = su `fichajes.json` manual + todos los lotes no deshechos (varios traspasos del mismo jugador se colapsan: primer origen → último destino) y se la pasa a **Fichajes** (`aplicarAlJuego`: todo o nada, con copia). Si queda vacía, quita lo generado.
4. **Con PES abierto** solo se reemplaza el `PlayerAssignment.bin` de Phoenix-DB (escritura atómica). **No** se toca `sider.ini` ni se aparta ningún `Player.bin`: si falta la línea Phoenix-DB o hay un `Player.bin` ajeno, **espera a que cierre PES** (el lote queda «pendiente» con el motivo). Con PES cerrado, queda para el próximo arranque.
5. Avisa a `phoenix.lua` escribiendo `content\phoenix\recargar.txt` (cambia en cada lote). Lo que pasa después lo decide **su modo de recarga**: *AUTO-SIEMPRE / AUTO-FICHAJES* → recarga sola al volver al menú y entrar a un modo (nunca en un partido ni en Editar, momento seguro de `phoenix.lua`); *ACTIVAR* → queda listo hasta que pulse «Datos Actual. en vivo → Activar».
6. **Por jugador** se guarda el resultado: `aplicado | ya_estaba | saltado (motivo) | reemplazado por uno posterior`; por lote: `aplicado | parcial | saltado | deshecho`. **DESHACER el último lote** regenera Phoenix-DB sin él (o lo quita si no queda nada) y vuelve a avisar.
- **Link del amigo apagado:** al abrirlo trae todos los lotes que se perdió, en orden.
- **Conflicto (el amigo también edita):** con «datos en vivo» activado manda la **base** (Phoenix-DB) sobre su option file para los jugadores del lote; el resto de su Editar no se toca. Si el amigo también **publica**, un traspaso que él recibió **no se vuelve a publicar** (anti-bucle: se compara con lo recibido).
- Límite honesto: Link **no sabe** si el juego ya recargó (phoenix.lua no se lo dice a Link); por eso en modo ACTIVAR el lote figura «aplicado» aunque el juego aún no lo haya cargado. Sí lo sabe el amigo en el overlay.

### 5. Interfaz (SYNC › TRASPASOS EN VIVO)
Selector **«¿Qué eres tú?»** (apagado / anfitrión / amigo / prueba) y tres pasos. **PASO 1 · CONEXIÓN:** por defecto **la web** con estado claro (conectado · la web aún no lo tiene + «Volver a comprobar» · sin cuenta vinculada + botón a Sync › Cuenta web · sin conexión · liga en pausa…). El **anfitrión** ve **su código** (copiar, «Código nuevo») y los nombres de quienes lo siguen; el **amigo** ve **«¿A qué anfitrión sigues?»**: escribe el código (o ve a quién sigue, con «Dejar de seguir»). Un bloque plegado **«Avanzado: usar una carpeta compartida en vez de la web»** (con la carpeta de prueba). **PASO 2 (anfitrión):** estado, confirmación antes de enviar, traspasos detectados con **PUBLICAR / DESCARTAR**, ajustes (option file a mano, olvidar la foto de partida). **PASO 3 (amigo):** modo de recarga, historial de lotes con el resultado de cada jugador y **DESHACER el último lote**. Estado en `%APPDATA%\Trybuchet\Smash Soda\phoenix-traspasos\`. Acciones `traspasos.estado|config|accion` (ver `SmashSoda\phoenix\web\PROTOCOLO.md`). El simulador (`js/simulador.js`) imita todo; con `?traspasos404` imita una web que aún no lo tiene. **Lista «Antes de la prueba»** (0.2.79): según el rol, ✓/✗ con una frase de qué hacer, actualizada sola. Anfitrión: cuenta vinculada · cuenta aprobada como host (código obtenido) · option file detectado (ruta y fecha del `EDIT00000000` que vigila) · foto de partida · conectado a la web. Amigo: cuenta vinculada · siguiendo a un anfitrión (nombre) · juego encontrado · Phoenix-DB en el `sider.ini` · `phoenix.lua` instalado (versión) · aviso de si PES está abierto o cerrado y qué pasará con el lote. Tras aplicar un lote el amigo ve en verde **«Fichaje aplicado: X pasa de A a B»** más qué hacer (cerrar y abrir PES, o la recarga automática / «Activar» según su modo) y, si PES está abierto, el mismo aviso sale en el overlay del juego. Todo apagado por defecto.

### 6. Pruebas
`pruebas-link\correr_traspasos.bat ["…\save\EDIT00000000" ["D:\…\Conmegol Patch"]]` (MSVC; el .exe se llama `traspasos_test`). **170/170** con una copia del option file real y las copias de las bases reales; **69/69** sin nada (lógica pura). El hub falso **imita el contrato real** (lotes con `seq` global, un código por anfitrión aprobado, `prueba:true`, idempotencia por `(anfitrión,id)`, rechaza campos que no existen en el contrato). Cubre: lote/huella/validación; transporte carpeta; **transporte web** (código estable y rotación, código incorrecto sin estropear el estado, seguir, `desde_id` 0, intruso sin código, host/no host, nombres largos recortados antes de la huella, lotes repetidos/alterados, páginas, dejar de seguir, **404 sin código (5 min), 401, 429 `reintentar_en`, 503 `LIGA_PAUSADA`, timeout (backoff 10→120 s), 500, HTML corrupto**, PC sin vincular, amigo que aún no sigue a nadie); colapso de lotes; anfitrión; amigo; **cadena completa por la web** (cuentas distintas; intruso; cuenta no-host que intenta publicar; 404 sin insistir y «volver a comprobar»; web caída; ponerse al día con páginas + duplicado; **carpeta → web sin mezclar numeraciones**; modo Prueba con lote «TUYO»); y la cadena con **bases reales** (Yamal Barça→Arsenal en 3 modos). Además comprobé contra la **web real** (sin token, que no es un secreto) que `GET /traspasos` responde `401 TOKEN_FALTANTE` y una ruta rara `404 RUTA_NO_EXISTE`, tal como Link los clasifica. **Lo que NO se ha probado:** Link con un token real contra la web (lo hará la sesión principal con Fralex) ni nada en PES real.

### 7. Prueba real con Fralex y su amigo (con copia antes)
**Antes de empezar:** en SYNC › TRASPASOS EN VIVO mirad la lista **«Antes de la prueba»**: cada punto sale ✓ o ✗ y, si es ✗, una frase dice qué hacer. Se actualiza sola. Haced copia de `…\save\EDIT00000000` (Fralex) y de `SiderAddons\livecpk\Phoenix-DB` si existe (el amigo).
**Fralex (anfitrión) — 3 líneas:** 1) SYNC › Traspasos en vivo › **ANFITRIÓN**: espera ✓ en «cuenta aprobada como anfitrión» (sale **tu código de 8 caracteres**) y ✓ en «foto de partida hecha»; pásale el código a tu amigo. 2) En PES: Editar → **un** fichaje → **Guardar** y salir. 3) En Link: «Detecté traspasos» → **PUBLICAR** (o sale solo si quitaste la confirmación).
**Amigo — 3 líneas:** 1) SYNC › Traspasos en vivo › **AMIGO**: escribe el código en «¿A qué anfitrión sigues?»; mira que la lista diga ✓ (si falta Phoenix-DB en el sider.ini, con PES cerrado se añade solo al aplicar el primer lote; si falta phoenix.lua, instálalo en Sync › Puente › Módulos del juego). 2) Cuando Fralex publique verás en verde **«Fichaje aplicado: X pasa de A a B»** (y, si PES está abierto, lo mismo en el overlay del juego). 3) **Cierra y vuelve a abrir PES** (o, sin cerrar, espera la recarga automática al volver al menú / pulsa Partido → Datos Actual. en vivo → Activar según tu modo) y mira a X en B.
Si no tenéis dos cuentas, probad **PRUEBA (LOS DOS)** en una sola PC. Probad también **Código nuevo** (el amigo ya enlazado sigue recibiendo), **Dejar de seguir** y **DESHACER el último lote**. Si la web va mal, el motivo sale en el PASO 1 y en la lista (✗) y «Volver a comprobar» reintenta.

### 8. Archivos
`link\Captura.h/.cpp` (fase 1), `link\Traspasos.h/.cpp` (lote, transportes, motor), `link\ServicioTraspasos.h/.cpp` (hilo + ajustes), `web\AccionesTraspasosWeb.cpp`, `web\ui\js\vistas\traspasos.js`, `terceros\pesxcrypter\` (+ `pesxvalidar.c`), `pruebas-link\traspasos_test.cpp` y `correr_traspasos.bat`. CMake: lenguaje C activado para los 4 `.c` (fuera de unity y sin `pch.h`). No se tocó `PhoenixSync/` ni `Fichajes.cpp/.h` (solo se usa su API pública: `analizar`, `aplicarAlJuego`, `quitarDelJuego`, `leerNombresDeBase`).

### 9. Pendiente
- **Prueba real en PES** (§7) — bloquea dar el traspaso por bueno (¿PES guarda el option file al guardar/salir de Editar? ¿recarga del amigo con este flujo?).
- **Prueba de punta a punta con la web real y token real** (anfitrión + amigo, cuentas distintas) con Fralex. Desde Link solo se comprobó la web real **sin token**; el resto, contra un hub falso que imita su contrato.
- La cuenta del anfitrión debe estar **aprobada como host** en la web (si no, la lista dice «sin permiso»).
- Link no sabe cuándo el juego recargó (phoenix.lua no se lo dice): el aviso explica cómo verlo, pero no confirma que ya se vea.
- Liga Máster (más adelante: otro archivo `ML…` y otras reglas).

---

## Parte 7 · Link 0.3.41 a 0.3.45: vincular la cuenta y Sync más sencillo (21-oct-2026)

> Origen: `docs/CUENTA-Y-SYNC-SENCILLO.md` · último cambio eee096a 2026-10-21 · 7237 bytes. Copia íntegra.

Guía paso a paso para Fralex, testers y cualquier IA que retome el trabajo. Rama `plan-ux` del repo `Fralexito/smash-soda-fork` (worktree `C:\dev\smash-soda-fork\_phoenix-link`).

### 1. Qué cambió

| Versión | Qué | Commit |
|---|---|---|
| 0.3.41–0.3.43 | Hasta 16 puestos (experimental): «Puestos máximos», puestos 9+ en `phoenix_mandos_extra.dat`, contador de Xbox hasta 16. Guía: `docs/MAS-DE-8-MANDOS.md`. | `06a673d` |
| 0.3.44 | Arreglo: no se podía vincular la cuenta (la web lo rechazaba todo). | `06a673d` |
| 0.3.45 | SYNC › TRASPASOS más sencillo: arriba solo el código y recibir; los interruptores juntos en «Ajustes de traspasos». | `d14a128` |

### 2. Vincular la cuenta (arreglo 0.3.44)

**Qué pasaba.** Desde el 19-oct (~17:00 Lima) la web contestaba `CAMPO_INVALIDO «x-phoenix-build»` a todo lo que mandaba Link: vincular, chat, presencia, salas, buzón del juego.

**Por qué.** La cabecera `X-Phoenix-Build` lleva la huella SHA-256 del exe (64 caracteres). Se guardaba en un texto compartido sin candado. Al arrancar, varios hilos la pedían a la vez y cada uno le añadía sus 64 caracteres. Salía una huella de 128 o más, y la web la rechaza.

**Arreglo** (`SmashSoda/phoenix/link/PhoenixLink.cpp`, `huellaExe`). Se calcula una sola vez con un estático local de C++11, que es seguro entre hilos. Si no salen 64 caracteres, se manda vacía, porque la web también acepta eso.

**Cómo se comprobó (21-oct).**
- Petición a `/functions/v1/phoenix/v1/emparejar` con la huella del exe nuevo y un código falso. La web responde `404 CODIGO_NO_ENCONTRADO` («Código incorrecto»), que es lo normal: ya no rechaza la cabecera.
- La misma petición con la huella duplicada (128 caracteres) da `422 CAMPO_INVALIDO`. Esto confirma la causa.
- Registros de Supabase: los rechazos `CAMPO_INVALIDO` paran el 20-oct a las 18:14 UTC. El último intento real de Fralex fue a las 13:14 Lima, y el exe arreglado se compiló a las 13:21 Lima. Todavía no se había probado con la versión buena.

**Pasos para vincular.**
1. Cierra Link del todo y ábrelo con el acceso directo «PhoenixLink» del menú Inicio. Apunta a `_phoenix-link\x64\Release\PhoenixLink.exe`.
2. Comprueba que pone la versión 0.3.45 (abajo a la izquierda).
3. En la web de la liga: *Mi perfil › vincular PC*. Saca un código nuevo, porque caducan.
4. En Link: SYNC › MI JUEGO › «Tu cuenta de la liga». Pega el código y pulsa CONECTAR.
5. Debe salir «PC vinculada a <tu nombre>». La cuenta se guarda en `%APPDATA%\Trybuchet\Smash Soda\phoenix-token.dat` (cifrada con DPAPI).
6. Si sale otro mensaje, apúntalo tal cual: el texto viene de la web y dice la causa.

### 3. SYNC › TRASPASOS más sencillo (0.3.45)

**Pedido de Fralex:** «Confunde más las tantas opciones y ventanas que tiene el sync».

**Antes.** Había una tarjeta entera de «Cambios oficiales», dos interruptores dentro de ENVIAR, uno dentro de RECIBIR y otro «Más opciones» abajo. En total, 24 botones a la vista.

**Ahora.**
1. Arriba, «PASAR FICHAJES ENTRE AMIGOS» con una línea: «Los cambios oficiales de Phoenix llegan solos, sin código».
2. **ENVIAR MIS FICHAJES:** tu código en grande, COPIAR CÓDIGO y CAMBIAR CÓDIGO, el fichaje detectado con PUBLICAR o DESCARTAR, y «Mi archivo de guardado» (antes «Más opciones») para pegar la ruta si Link no la encuentra.
3. **RECIBIR FICHAJES DE UN AMIGO:** pega su código y pulsa RECIBIR. Debajo, el último fichaje aplicado y «Fichajes recibidos».
4. Abajo, «¿Algo no funciona?» y **«Ajustes de traspasos»** con todos los interruptores juntos:
   - recibir los cambios oficiales de Phoenix;
   - aprobar cada fichaje antes de enviarlo;
   - mantener mis fichajes en mi juego;
   - aprobar cada fichaje antes de ponerlo en mi juego;
   - modo prueba, carpeta compartida y APAGAR TRASPASOS.

Lo normal es no tocar los ajustes.

**Solo cambia la interfaz** (`SmashSoda/phoenix/web/ui/js/vistas/traspasos.js`). Las acciones `traspasos.estado`, `traspasos.config` y `traspasos.accion` son las mismas.

**Cómo se probó.** En el simulador (`python -m http.server 8791 --directory SmashSoda/phoenix/web/ui`, abrir `#sync` y la pestaña TRASPASOS) no hay errores de consola y los 7 ajustes están en «Ajustes de traspasos». Compilado con `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release --parallel` sin errores; la copia de `ui\` junto al exe quedó al día.

### 4. Cómo replicarlo desde cero

1. `git fetch origin plan-ux` en `C:\dev\smash-soda-fork\_phoenix-link` (o en un clon con la rama `plan-ux`).
2. Compila con cmake, como en el punto 3. **Nunca** con `COMPILAR_PHOENIX.bat`: hace `git checkout --force` y borra lo que no tenga commit.
3. Con Link cerrado, para que la copia de `ui\` no falle.
4. Para probar solo la interfaz, usa el simulador del punto 3.

### 5. Pendiente

- Que Fralex vincule la cuenta con el exe 0.3.45 (punto 2).
- Seguir con el «modo sencillo» (Jugar, Ser anfitrión, Mi cuenta): la pestaña AVANZADO de Sync y el resto de menús.
- Los capítulos del libro personal de mandos 9+ (`cap_60_j/k/l`, `cap_62`, `00_BITACORA_VIVA`) no se copiaron al repo público. Falta que Fralex diga si quiere publicarlos.

### 6. Link se quedaba en la pantalla de carga (arreglo 0.3.46)

**Qué pasaba (21-oct, 15:39).** Fralex abrió Link 0.3.45 y no pasaba de la pantalla de carga. La duda era si algo se había corrompido.

**Qué se comprobó, paso a paso.**
1. Los archivos de la interfaz junto al exe (`x64\Release\ui\`) son idénticos al código (`diff -rq`): nada corrupto.
2. Se abrió Link con `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=93xx` para mirar la página por DevTools (protocolo CDP, `http://127.0.0.1:93xx/json/list`).
3. Arranque limpio: la página carga en 4–8 s con 4 o con 16 mandos virtuales (se descartaron los 16 mandos como causa, cambiando `xboxPuppetCount` en `config.json` con copia y volviéndolo a 16).
4. **Abrir Link justo después de cerrar otro**: WebView2 no llega a lanzar su proceso durante 60–100 s o más (ni con la 0.3.40, así que no lo causó ningún cambio reciente). El motor web del Link anterior aún ocupa su carpeta (`%LOCALAPPDATA%\PhoenixLink\WebView2`), y a veces quedan procesos `msedgewebview2.exe` muertos pero listados (los retiene otro programa, p. ej. Process Lasso).
5. Intento fallido: matar los restos con `taskkill /F` o `Stop-Process`. Quedan como procesos ya terminados que no se pueden cerrar y la espera sigue.

**Arreglo** (`SmashSoda/phoenix/web/InterfazWeb.cpp`): si el motor web sigue en «Creando» a los 10 s, Link lo cierra y reintenta con una carpeta de reserva (`WebView2-reserva`), hasta dos veces. Luego sigue valiendo el vigilante de 30 s. Solo se pierde el localStorage de la página, que guarda cosas menores como si «Mandos en vivo» está plegado.

**Resultado medido.** Antes, al reabrir enseguida: 60–100 s o nunca. Con 0.3.46: carga en 19 s, y en arranque normal en 4–11 s.

**A tener en cuenta.** Si alguna vez vuelve a quedarse cargando, espera un minuto con Link cerrado y ábrelo otra vez. Para depurar la página real usa el truco del punto 2.

---

## Parte 8 · Guía completa de Phoenix Link: instalar y pasar un fichaje de un PES a otro (versión 0.3.12)

> Origen: `docs/GUIA-COMPLETA-FICHAJES-EN-VIVO.md` · último cambio d14a128 2026-10-21 · 14533 bytes. Copia íntegra.

> Actualizada al menú del plan UX (Link 0.3.34): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES. Mapa completo en `docs/GUIA-MENU-NUEVO.md`.

Probado de verdad el 13 de octubre de 2026: un anfitrión hizo fichajes en Editar de PES 2021, los guardó, Link los publicó y el PES de su amigo los recibió. Esta guía junta todo el recorrido, paso a paso, y los errores que pueden salir con la forma de evitarlos.

### 1. Qué vas a conseguir
- Tú (el **anfitrión**) haces un fichaje en **Editar** de PES y lo guardas.
- Tu Phoenix Link lo ve, lo convierte en un aviso diminuto («jugador X pasa del equipo A al B») y lo sube a la web de Phoenix.
- El Phoenix Link de tu **amigo** lo recoge solo y lo aplica en **su** base de datos. Cuando él cierra y abre PES, el fichaje está.
- Solo viajan nombres e identificadores de jugador y de equipo. No se copia ninguna base de datos ni ningún dato personal.

### 2. Qué necesitas antes de empezar
Los dos:
- Windows de 64 bits e internet.
- PES 2021 con Sider y un parche instalado (ConmeGOL u otro).
- Una cuenta de la liga en la web de Phoenix.
- El mismo número de versión de Phoenix Link (esta guía es para la 0.2.82).

Solo el anfitrión:
- Su cuenta debe estar **aprobada como host** en la web. Si no lo está, no obtiene código de traspasos.

Solo el amigo:
- El código de 8 letras y números del anfitrión.

No hace falta instalar nada a mano: el instalador pone lo que falte (Visual C++, ViGEmBus para los mandos virtuales y WebView2 para la interfaz).

### 3. Instalar Phoenix Link (los dos)
1. Pídele a quien te lo pasó el archivo `PhoenixLink-Setup-0.2.82.exe` (unos 13 MB). Guárdalo donde quieras.
2. Ábrelo con doble clic. Windows puede mostrar «Windows protegió su PC» (el instalador no está firmado): pulsa **Más información** y luego **Ejecutar de todas formas**.
3. Acepta el permiso de administrador (ventana de Windows con «Sí»).
4. Se abre el asistente:
   - **Bienvenida:** pulsa **Siguiente**.
   - **¿Dónde instalar?:** déjalo en `C:\Program Files\Phoenix Link` salvo que quieras otra carpeta. Deja marcado el acceso directo del escritorio. Pulsa **Instalar**.
   - **Instalando:** una barra de progreso. Si falta algo de Microsoft o ViGEmBus, lo descarga (hace falta internet) y lo instala solo. Tarda de 1 a 5 minutos.
   - **Final:** pulsa **Finalizar** (puedes dejar marcado «Abrir Phoenix Link ahora»).
5. Si ya tenías Phoenix Link, el instalador lo **actualiza** y no toca tus datos ni tu configuración.

### 4. Primer arranque: dejar todo listo (los dos)
Sync tiene **3 pestañas**, siempre en este orden: **MI JUEGO** (¿está listo mi juego para los fichajes?), **TRASPASOS** (pasar un fichaje a un amigo) y **AVANZADO** (opciones técnicas, solo si algo falla).

Hazlo con **PES 2021 cerrado**.
1. Abre Phoenix Link y entra en **SYNC › MI JUEGO**.
2. Arriba verás un mensaje: **«Todo listo»** (verde) o **«Falta algo: N cosas»** (ámbar). Debajo, una lista con ✓ (bien) y ✗ (falta algo). Cada ✗ trae una frase corta y un botón.
3. Si **Tu cuenta de la liga** está en ✗: en la web de la liga entra a *Mi perfil › vincular PC*, copia los 6 dígitos, escríbelos en esa fila y pulsa **CONECTAR**. Es lo único que haces tú.
4. Pulsa **ARREGLAR TODO**. Hace en orden, solo lo que es seguro:
   - busca tu juego (solo lee nombres de carpetas; si hay varios, te pregunta cuál);
   - instala los archivos de Phoenix en el juego (con copia previa de lo que cambia);
   - deja que el juego lea los fichajes (repone una línea de configuración, con copia);
   - deja la recarga automática activada.
5. Cuando arriba diga **«Todo listo»**, sigue. Si algo se queda en ✗, mira la sección 9.

### 5. Tu código (para enviar tus fichajes)
Desde la 0.3.12 **no hay que elegir papel**: en **SYNC › TRASPASOS** enviar y recibir van juntos y se encienden solos.
1. En el bloque **ENVIAR MIS FICHAJES** sale **tu código en grande** (8 letras y números). Pulsa **COPIAR CÓDIGO** y mándaselo a tu amigo. **CAMBIAR CÓDIGO** pone uno nuevo (el viejo deja de servir para unirse; quien ya lo recibe sigue).
2. Si no sale el código, la pantalla dice por qué: cuenta sin conectar (botón **CONECTAR MI CUENTA**), cuenta no aprobada como anfitrión (puedes recibir igual; para enviar, pídeselo al staff) o la web sin conexión (**VOLVER A COMPROBAR**).
3. Debajo hay **«¿Algo no funciona?»** (se abre sola si hay un ✗) y **«Ajustes de traspasos»**.
4. **Desde la 0.3.45 la pantalla es más corta.** Arriba solo quedan tu código (ENVIAR) y el campo para pegar el de tu amigo (RECIBIR). Todos los interruptores están juntos en **«Ajustes de traspasos»**, abajo del todo: recibir los cambios oficiales de Phoenix, aprobar cada fichaje antes de enviarlo, mantener mis fichajes en mi juego, aprobar cada fichaje antes de ponerlo en mi juego, modo prueba, carpeta compartida y APAGAR TRASPASOS. Lo normal es no tocarlos. Si Link no encuentra tu archivo de guardado, la ruta se pone en **«Mi archivo de guardado»**, dentro de ENVIAR.

### 6. Recibir los fichajes de un amigo
1. En el bloque **RECIBIR FICHAJES DE UN AMIGO**, pega el código de tu amigo y pulsa **RECIBIR**. Verás su nombre con **RECIBIENDO** y el botón **DEJAR DE RECIBIR**.
2. Puedes recibir de varios amigos («Recibir también de otro amigo») y a la vez enviar los tuyos.
3. Link consulta la web cada unos 8 segundos. Si tu amigo ya envió fichajes antes, **los recibirás al pegar su código** (la web los guarda 30 días, hasta 200 por persona).

### 7. Hacer el fichaje (anfitrión)
1. Abre PES 2021 y entra a **Editar**.
2. Haz **un solo** traspaso para empezar (un jugador pasa de un equipo a otro).
3. **Guarda y sal de Editar.** PES solo escribe el archivo al guardar; antes de eso Link no puede ver nada.
4. Espera unos segundos. Link lee el archivo cada ~3 s y confirma cuando PES terminó de escribir. En **Traspasos en vivo** aparece el traspaso detectado.
5. Pulsa **PUBLICAR** (por defecto Link pregunta antes de enviar).
6. En el Link del amigo debe salir **«Fichaje aplicado: X pasa de A a B»** con lo que hacer.

### 8. Ver el fichaje en el PES del amigo
1. Con la recarga automática, el juego recarga solo al volver al menú y entrar a un modo (nunca en medio de un partido ni en Editar).
2. Si no quieres esperar: que el amigo **cierre PES y lo vuelva a abrir**, o use el botón de activar/recargar que indica el aviso.
3. Que busque al jugador en su equipo nuevo.
4. Si algo salió mal, el amigo puede pulsar **DESHACER el último lote** y la base vuelve a como estaba.

Reglas del traspaso (para entender qué detecta Link):
- Cuenta como traspaso un jugador que **sale de un equipo y entra en otro**.
- Altas, bajas, convocatorias y dorsales se ignoran.
- Más de 60 cambios de golpe se consideran «cambio masivo» (por ejemplo, cambiar de modo del parche) y no se publican solos.
- La primera vez Link solo guarda la «foto de partida» y no publica nada.

### 9. Errores posibles y cómo evitarlos

#### Al instalar
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «Windows protegió su PC» | El instalador no está firmado digitalmente | Más información › Ejecutar de todas formas |
| «Phoenix Link está abierto» | El instalador no puede reemplazar archivos en uso | Cierra Phoenix Link y el overlay (también en la bandeja) y pulsa Reintentar |
| «No se pudo bajar…» (Visual C++, ViGEmBus, WebView2) | Sin internet, o un antivirus o cortafuegos lo bloquea | Revisa el internet y vuelve a ejecutar; el instalador retoma lo que falte |
| Error de permisos en la carpeta | Instalaste en una carpeta protegida sin ser administrador | Ejecuta el instalador con administrador o elige otra carpeta |
| El antivirus lo bloquea | Programa nuevo sin firma | Permite el instalador y la carpeta de Phoenix Link en tu antivirus |

#### Al abrir Link
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «No se pudo crear la vista web (0x80080005)» | Quedó un proceso del motor web de una sesión anterior que se cerró mal | Desde la 0.2.74 Link lo limpia solo al reabrir; usa **Reintentar**. Si sigue, reinicia el PC |
| Se abre la interfaz de siempre y no la nueva | WebView2 no está instalado o falló | Reinstala con el instalador (instala WebView2) |
| Link se cierra solo | Fallo interno (hubo varios antes de la 0.2.78) | Manda el archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-fallos.log`; Link anota ahí la causa y las funciones implicadas |
| El overlay no aparece | La sala está cerrada, o PES va en pantalla completa «de verdad» | Abre la sala, pon PES en ventana o sin bordes. Panel de amigos: **Ctrl+Alt+A** |

#### En Sync › MI JUEGO
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| ✗ carpeta del juego | Link no encontró PES o hay varios | Elige la carpeta en la lista (por ejemplo la de tu parche) |
| ARREGLAR TODO se detiene: «cierra PES» | Escribir en el juego con PES abierto es peligroso | Cierra PES y pulsa de nuevo |
| ✗ archivos de Phoenix o «perdido» | Actualizaste el parche pegando carpetas encima y se borraron las líneas de Phoenix en `sider.ini` | Pulsa **ARREGLAR TODO**; repone las líneas |
| ✗ «Que el juego lea los traspasos» | El `sider.ini` no tiene la línea que lee la carpeta de fichajes (pasa si pegaste una actualización del parche encima) | Con PES cerrado, pulsa **ARREGLAR** o **ARREGLAR TODO**; Link hace copia antes |
| Faltan archivos del módulo | Copia incompleta | **ARREGLAR TODO** de nuevo; Link guarda copia de `sider.ini` antes de tocarlo |

#### En Sync › Traspasos (anfitrión)
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| ✗ «sin permiso» / «aprobada como anfitrión» | Tu cuenta no está aprobada como host | Pídele al staff que apruebe tu cuenta como host en la web |
| «La web no reconoce este Link» | Cuenta no vinculada o token vencido | **Sync › Mi juego › Tu cuenta de la liga**: vuelve a conectarla con un código nuevo (o mira el detalle en Avanzado › Cuenta web detallada) |
| «Sin conexión con la web [motivo]» | Internet, cortafuegos, antivirus o la web caída. El motivo técnico va entre corchetes | Link reintenta solo (10, 20, 40, 80 y 120 s). Si persiste, manda una captura del Paso 2 de Traspasos y de Avanzado › Detalles técnicos |
| «La web aún no tiene los traspasos en vivo» | La función de la web no estaba disponible | Link prueba de nuevo cada 5 minutos; botón **VOLVER A COMPROBAR** |
| La liga está en pausa | El staff apagó los resultados/traspasos | Espera a que la reactiven |
| Guardas en Editar y no sale nada | No saliste de Editar o no guardaste; la carpeta vigilada no es la tuya (hay varias carpetas de guardado) | Guarda y sal de Editar; mira la ruta del archivo en la lista y corrígela si no es la tuya |
| «Cambio masivo, pendiente» | Más de 60 traspasos de golpe | Descártalo; no se publica solo |
| No detecta nada con Liga Máster | Link solo mira el archivo de edición (`EDIT…`), no el de Liga Máster (`ML…`) | De momento no se soporta |

#### En Traspasos en vivo (amigo)
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «Código incorrecto» (CODIGO_INVALIDO) | Se escribió mal, o el anfitrión lo cambió | Pide el código actual; cópialo en vez de escribirlo |
| No puedes seguirte a ti mismo | Usaste tu propio código | Solo en modo **Prueba** |
| «Demasiados intentos» | Más de 10 intentos de seguir en un minuto | Espera un minuto |
| El lote se «salta» con un motivo | Su base es distinta a la del anfitrión y no hay coincidencia exacta de nombres | Lee el motivo; con parches distintos Link aplica por nombres de jugador y equipo, y si hay duda no toca nada |
| «Esperando a cerrar PES» | Con PES abierto no se toca nada salvo la base de Phoenix; si falta la línea de Phoenix-DB o hay un `Player.bin` ajeno, espera | Cierra PES; se aplica solo |
| Aplicado pero no se ve en el juego | El juego aún no recargó | Cierra y abre PES, o espera la recarga automática. Link no sabe si el juego ya recargó |
| Amigo y anfitrión con versiones distintas | Cambios entre versiones | Los dos con la misma versión |
| El amigo también edita | Hay conflicto entre su edición y la del anfitrión | Con «datos en vivo» activado manda la base de Phoenix para los jugadores del lote; el resto de su Editar no se toca |

#### Desinstalar
- **Configuración › Aplicaciones › Phoenix Link › Desinstalar**, o `Desinstalar.exe` dentro de la carpeta.
- Casilla opcional: **borrar también mis datos y ajustes** (cuenta vinculada, perfiles de amigos, configuración).
- No toca la carpeta de PES. Si instalaste el módulo del juego y quieres quitarlo, hazlo **antes** desde Link: SYNC › Avanzado › Archivos de Phoenix en el juego.

### 10. Qué está probado y qué no
**Probado de verdad (13 oct 2026):**
- Instalar con el `.exe` en un PC distinto.
- Fichaje en Editar, guardado, detección, publicación, llegada al amigo y aplicación en su PES.
- Conexión con la web: 166 peticiones, 0 fallos, máximo 2,7 s.
- Detección de requisitos y copia real de archivos del instalador, y pantallas del asistente.

**Sin probar todavía en el juego real:**
- Desinstalar de verdad en un PC instalado.
- Overlay encima de PES y panel de amigos con una invitación real.
- Cambios del HUD de Sider: lado según el puesto, ping en vivo y goles en columnas.
- Lados automáticos A/B y campo 2D en vivo.
- ARREGLAR TODO con varios candidatos de juego.

### 11. Si algo falla, manda esto
1. Una captura de **SYNC › MI JUEGO** y de **SYNC › TRASPASOS** (los 3 pasos y «¿Algo no funciona?» abierto). Si hay un error de la web, también **Avanzado › Detalles técnicos**.
2. El archivo `%TEMP%\PhoenixLink-instalador.log` (si falló al instalar).
3. El archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-fallos.log` (si Link se cerró).
4. El archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-traspasos\estado-actual.json` (estado de los traspasos; no lleva contraseñas).

### 12. Glosario
- **Anfitrión:** quien hace los fichajes y los publica.
- **Amigo:** quien sigue al anfitrión y los recibe.
- **Lote:** el aviso de uno o varios traspasos.
- **Foto de partida:** copia de cómo estaban las plantillas, para saber qué cambió.
- **Phoenix-DB:** la base con los fichajes que Link deja en el juego del amigo.
- **Sider:** el programa que carga los módulos de Phoenix dentro de PES.

---
