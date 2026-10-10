# Phoenix Estadio + Árbitro de Link — documento completo para replicarlo

**Fecha:** 2026-10-10 · **Autor:** chat LINK (Claude), a pedido de Fralex · **Rama:** `rediseño-phoenix-portal` (worktree `C:\dev\smash-soda-fork\_phoenix-link`)
**Estado:** Link compila sin errores y está probado en el simulador. El módulo Lua está escrito y revisado a mano, pero **no se ha ejecutado** (no hay intérprete de Lua en la PC) y **no está instalado**. Nada está commiteado.

Este documento explica **qué se hizo, cómo, por qué y con qué cuidados**, para poder rehacerlo desde cero. Las instrucciones cortas de instalación están en `SmashSoda/phoenix/sider/LEEME.md`, y el protocolo de la interfaz en `SmashSoda/phoenix/web/PROTOCOLO.md`.

---

## 1. Qué es

Phoenix Link hace de **árbitro del partido** de PES 2021 **desde fuera del juego**, y el juego le cuenta a Link lo que pasa en el partido.

| Pieza | Dónde corre | Qué hace |
|---|---|---|
| `phoenix_estadio.lua` (nuevo) | Dentro de PES (Sider) | Lee el marcador y el reloj (`match.stats()`) y lo escribe en `estado.json` / `resultado.json`. Muestra un HUD en el overlay con lo que Link le manda en `sala.txt`. |
| Árbitro (`Arbitro.cpp`, nuevo) | Phoenix Link (hilo de la ventana) | Modo competitivo (Start/Back/Guía bloqueados a los invitados), marcador automático (arranque, goles con minuto en el chat, descanso, final con historial y web) y pausa automática si alguien se cae o tiene mucho ping. |
| Lector de `estado.json` + escritor de `sala.txt` | Phoenix Link (hilo de `PhoenixLink`) | Lee el archivo del juego ~1 vez por segundo y escribe `sala.txt` de forma atómica. |

**Lo que se vive:**
- **En el juego:** página «PHOENIX ESTADIO» del overlay, con marcador y nombres reales, minuto, goles con minuto, jugadores de cada lado con su mando y su ping, árbitro y chat.
- **En la sala:** «¡Arrancó!», «GOL 41' de Kaiser. Mirko 2-2 Kaiser», «Descanso: …», «Final: … Goles: 12' Mirko, 20' Kaiser…».
- **Para los invitados:** no pueden pausar ni salir con Start/Back/Guía mientras se juega.

---

## 2. Reglas y contexto que se tomaron en cuenta

1. **Parsec:** PES corre **solo en la PC del anfitrión**. Los invitados juegan con mandos virtuales ViGEm que Link controla. Por eso el bloqueo y el «pulsar Start» se hacen en Link, no en el juego.
2. **Frentes (COORDINACION.md):**
   - LINK trabaja en `SmashSoda/` (rama del rediseño, worktree `_phoenix-link`);
   - SYNC es dueño de `PhoenixSync/` y de `phoenix.lua`. **`phoenix.lua` no se tocó.** Lo nuevo es un módulo aparte;
   - a SYNC solo se le escribe por prompt (`PhoenixSync/prompts/PROMPT-SYNC-estado-partido.md`) y con una línea en su `REGISTRO.md`.
3. **Fralex:**
   - español y respuestas cortas;
   - «si algo se rompe, mejor no lo hagas»;
   - nada a `master` ni commits sin permiso;
   - **no rehacer pantallas ni cambiar secciones o pestañas**: solo se añadieron una línea en Partido › En vivo y una tarjeta en Mandos › Bloqueo.
4. **Compilar:**
   - siempre con `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release`;
   - **nunca `COMPILAR_PHOENIX.bat`**, porque hace `git checkout --force` y borra lo que no está commiteado.
5. **Cambios ajenos sin commitear en el worktree** (interfaz original con mejoras, y `ajustes.overlayMenu` de otra sesión en `AccionesAjustesWeb.cpp`): se releyó cada archivo antes de editarlo y no se pisó nada. **`AccionesAjustesWeb.cpp` no es de este trabajo.**
6. **Seguridad en el juego** (de `PhoenixSync/sider/RIESGOS-SIDER.md`): solo lectura, nada de memoria, coste cero fuera del partido, apagado automático tras 5 errores, escritura de archivos lo más segura posible y probar offline primero.
7. **Preferencias por defecto:**
   - modo competitivo **encendido** (es inofensivo);
   - marcador automático **encendido** (sin el módulo no hace nada);
   - pausa automática **apagada** hasta probarla en el juego, porque pulsa un botón de verdad.

---

## 3. El paquete de Luas: cómo conviven `phoenix.lua` y `phoenix_estadio.lua`

Los dos son módulos del **mismo paquete de Sider** de Phoenix: viven en `SiderAddons\modules\` y se cargan con una línea cada uno en `sider.ini`. **No se fusionan, a propósito.**

| | `phoenix.lua` v0.17e (SYNC) | `phoenix_estadio.lua` v1.0 (LINK) |
|---|---|---|
| Para qué | Avisos de la web en el overlay. Botón nativo «Datos Actual. en vivo» (recarga los datos que coloca el repartidor de Link). Lecturas de investigación (opción en vivo, última recarga, «zona»). | El partido: marcador, reloj y goles hacia Link, y HUD del partido con datos de Link. |
| Toca la memoria | **Sí**: 3 parches en el código del juego al arrancar (A, B y C, con comprobación de bytes). Lecturas con `ffi` + ReadProcessMemory. | **No.** Solo `match.stats()` y `ctx`. Sin `ffi`. |
| Cuándo trabaja | Arranque (primera lectura de la base), overlay abierto, `set_teams` / `context_reset` (solo los anota). | Solo con partido (`display_frame` cada 0,5 s) y con su página del overlay abierta. |
| Archivos en `content\phoenix\` | **Lee** `avisos.txt`. | **Escribe** `estado.json` y `resultado.json`. **Lee** `sala.txt`. |
| Teclas | M (solo mirar). B, K, L, P y U están desactivadas en la «e». | Ninguna. |
| Eventos de Sider | `overlay_on`, `key_down`, `livecpk_read`, `set_teams`, `context_reset` | `display_frame`, `set_teams`, `overlay_on` |
| Página del overlay | «Phoenix Evolution» | «PHOENIX ESTADIO» |

**Por qué no chocan:**
- No comparten ningún archivo: comparten la carpeta, no los nombres.
- Sider entrega `set_teams` a todos los módulos que lo registran. Ninguno de los dos devuelve nada, así que ninguno cambia los equipos.
- `overlay_on` y `key_down` solo llegan al módulo cuya página está abierta.
- Cada uno tiene su propio contador de errores. Si uno se apaga, el otro sigue.

**Por qué no fusionarlos:** la v0.17e es delicada (parchea código del juego). Mantener separado lo que solo lee permite activar, desactivar o depurar cada parte sin arriesgar la otra.

**Instalación conjunta en `sider.ini`, al final de la lista de módulos:**
```
lua.module = "phoenix.lua"
lua.module = "phoenix_estadio.lua"
```
**Dónde:** en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\` (la carpeta que el switcher copia encima del juego con robocopy) **y** en `<juego>\SiderAddons\`. Si solo se pone en la raíz, el switcher lo pisa (pasó el 8 oct, RIESGOS-SIDER #6b).

**Pendiente de SYNC:** la v0.17e (61 KB, 10-oct 03:43) **solo existe en el juego**. En git sigue la v0.17 (45 KB). Hay que subirla.

---

## 4. Investigación previa: cómo se supo lo que permite Sider

Todo se comprobó en la instalación real (`D:\Frank\Games_\Conmegol Patch\`), no de memoria:

1. **Globales del Lua de Sider 7.3.3:** volcado de `env.lua` en `SiderAddons\sider.log`:
   `assert, pairs, ipairs, tostring, tonumber, type, error, unpack, collectgarbage, table, string, math, os, io, fs, ffi, jit, bit, zlib, memory, match, audio, input, log`.
   → **No hay `pcall`, `select`, `next`, `setmetatable` ni `print`.** `os` solo trae `date`, `time` y `clock`: **no hay `os.rename` ni `os.remove`**.
   (El primer prompt para SYNC pedía `pcall` y `os.rename`. Estaba mal y se corrigió.)
2. **`match.stats()` está activo:** `sider.ini`, línea 7: `match-stats.enabled = 1`. Devuelve `nil` sin partido, o `home_score, away_score, pk_home_score, pk_away_score, period (0-5), clock_minutes, clock_seconds, added_minutes`. No da goleadores, tarjetas ni pausa.
3. **`display_frame` existe en 7.3.3:** lo usa `modules\sifflet.lua` (activo), que además usa `audio.new(...)`, `:set_volume`, `:when_done`, `:play`. Un evento desconocido solo deja un `WARN` en el log, como `goal_scored` u `overlay_off`: no rompe nada.
4. **Campos de `ctx`** usados por los módulos instalados: `home_team`, `away_team`, `tournament_id`, `match_info`, `stadium`, `sider_dir`… (recuento con `grep` sobre `modules\*.lua`).
5. **Nombres de equipos:** `content\kit-server\map.txt` (de olmosjr23), con el formato `id, "País\Liga\Equipo"`, UTF-8 con BOM. Es el mismo que lee `kserv.lua` (`load_map`). El nombre es el último tramo de la ruta.
6. **Auditoría previa de SYNC:** `PhoenixSync/sider/auditoria/*.md` (eventos por módulo, quién escribe memoria), `RIESGOS-SIDER.md` y `VINCULO-TIEMPO-REAL.md`.
7. **Lectura completa de la v0.17e**, para asegurar que no choca (sección 3).

---

## 5. Flujo

```
           PES 2021 (PC del anfitrión, Sider)                          Phoenix Link
 ┌─────────────────────────────────────────────┐        ┌───────────────────────────────────────────┐
 │ phoenix_estadio.lua                          │        │ hilo PhoenixLink (~1 s)                   │
 │  display_frame c/0,5 s → match.stats()       │ escribe│  detectarJuego() c/5 s → carpeta buzón    │
 │  → estado.json (1/s, y al momento en gol/fase)├───────►│  juego::leer(estado.json) (≤5 s = válido) │
 │  → resultado.json (fin de partido completo)  │        │  guarda el último bueno 3 s si sale a medias│
 │                                              │   lee  │  escribe sala.txt atómico (cambio / 4 s)  │
 │  overlay_on → HUD ◄── sala.txt ◄─────────────┼────────┤                                           │
 └─────────────────────────────────────────────┘        │ hilo ventana: tickDatos → tickArbitro     │
 phoenix.lua v0.17e ◄── avisos.txt ◄─────────────────────┤  competitivo · goles · relato · final     │
                                                         │  pausa auto (Start en mando ViGEm libre)  │
                                                         │  arma el texto de sala.txt                │
                                                         └───────────────────────────────────────────┘
```

---

## 6. Contrato de archivos (`<juego>\SiderAddons\content\phoenix\`)

### `estado.json` (lo escribe el Lua; Link solo lo lee)
Una línea, UTF-8, sin BOM:
```json
{"v":2,"fase":"en_juego","minuto":33,"segundo":12,"periodo":1,"anadido":0,"reloj_corre":true,
 "local":101,"visita":102,"nombre_local":"Universitario","nombre_visita":"Alianza Lima",
 "goles_local":2,"goles_visita":1,"pk_local":0,"pk_visita":0,
 "goles":[{"m":12,"l":"l"},{"m":20,"l":"v"},{"m":30,"l":"l"}],"completo":false,"torneo":0,"seq":40}
```
- `fase`: `menu` | `en_juego` | `pausado` | `descanso` | `final`. Link rechaza cualquier otra.
- `goles[].l`: `"l"` = local del juego, `"v"` = visita. `m` = minuto (tope 45/90/105/120 según el periodo).
- `completo`: solo cuenta con `fase:"final"`. Es `true` si se llegó al 89' de la 2.ª parte o a la prórroga o los penales.
- **Validez en Link:** el archivo existe, tiene ≤ 5 s (fecha de modificación) y se entiende. Si no, «sin datos del juego» y todo vuelve a manual.
- Link también entiende la **v1** (sin los campos nuevos).

### `resultado.json` (lo escribe el Lua)
Lo mismo con `fase:"final"`, `completo:true` y `"fin":<hora Unix>`. Se escribe una vez al terminar un partido completo, para subirlo a la web más adelante (hoy Link no lo usa).

### `sala.txt` (lo escribe Link de forma atómica; el Lua solo lo lee)
Líneas `clave=valor`, UTF-8:
```
v=1
abierta=1
sala=Phoenix Galaxy
local=Mirko
visita=Kaiser
marcador=en_juego
goles=2-1
ja=Mirko|20|1
jb=Kaiser|50|3
arbitro=Start/Back/Guía bloqueados a los invitados · pausa automática
chat=Mirko: buenas
t=1791638000
```
- `ja=` / `jb=` son `nombre|ping|mando` del lado local y del visitante. El ping va redondeado a 10 ms para que el archivo no cambie por nada.
- Los mandos `1..equipoLocal` son del local, igual que la formación del juego.
- `t` es la hora Unix. Si tiene más de 10 s, el HUD dice «Phoenix Link sin datos».
- Los textos se limpian: sin saltos, tabuladores, `|` ni caracteres de control, y cortados sin partir letras UTF-8. Hasta 4 líneas `chat=`.

### `avisos.txt`
Ya existía: lo escribe Link y lo lee `phoenix.lua`. No cambió.

---

## 7. El módulo `phoenix_estadio.lua` (`SmashSoda/phoenix/sider/`)

**Estructura** (406 líneas):

| Función | Qué hace |
|---|---|
| `cortarUtf8`, `esUtf8` | Cortes y validación UTF-8 (copiados del estilo de `phoenix.lua`). |
| `jsonTexto` | Escapa `"`, `\` y caracteres de control (`\u00XX`). |
| `escribir(nombre, texto)` | `io.open(..., "wb")` + `write` + `close`. Si falla (la carpeta no existe o Link tiene el archivo abierto), devuelve false y se reintenta en el siguiente segundo. |
| `cargarNombres`, `nombreDe` | Lee `kit-server\map.txt` una sola vez (≤ 1 MB) y guarda id → nombre (≤ 32 bytes). |
| `jsonPartido`, `escribirEstado` | Arman y escriben `estado.json`. `seq` sube en cada escritura. |
| `empezar` / `terminar` | Abren y cierran el partido. `terminar` decide si está `completo`, escribe el estado final y, si está completo, `resultado.json`, y deja un resumen para el HUD. |
| `gol` / `quitarGol` | Añaden o quitan goles con minuto cuando el marcador sube o baja (un gol anulado lo quita). |
| `mirar` | El corazón: lee `match.stats()`, detecta revancha, actualiza el reloj, los goles y la fase, y escribe. |
| `m.display_frame` | Cada 0,5 s: «bandera» de errores + `mirar()`. |
| `m.set_teams` | Guarda los ids de local y visita. |
| `leerSala`, `textoPing`, `columna`, `textoHud`, `m.overlay_on` | El HUD. |
| `m.init` | Rutas (`ctx.sider_dir`) y registro de `set_teams`, `display_frame` y `overlay_on`. |

**Cómo se deduce la fase** (`match.stats()` no dice si hay pausa):
- `nil` → no hay partido. Si había uno, `terminar()`.
- `period == 0` → `menu` (previa: Link no bloquea ni arranca nada).
- Reloj quieto ≥ 3 s en el 45' (periodo 1) o en el 105' (periodo 3) → `descanso`.
- Reloj quieto ≥ 45 s → `pausado` (las repeticiones y celebraciones duran menos).
- En cualquier otro caso → `en_juego`. `reloj_corre` es `true` si el reloj cambió en los últimos 1,5 s.
- **Revancha sin salir:** si ya se jugó la 2.ª parte y el reloj vuelve más de 10 minutos atrás, se cierra el partido anterior y se abre uno nuevo.
- **No se usa** «reloj quieto en el 90'» como final: un gol en el añadido (con repetición) lo dispararía antes de tiempo. El final llega cuando `match.stats()` pasa a `nil`.

**Cadencia y coste:**
- `match.stats()` cada 0,5 s (también fuera del partido, como hace `sifflet.lua` en cada frame).
- `estado.json` como mucho 1 vez por segundo, y al momento en un gol o un cambio de fase.
- `sala.txt` solo se lee con la página del overlay abierta, como mucho 1 vez por segundo y ≤ 4 KB.

**Errores sin `pcall`:** una bandera `enCurso` se levanta al entrar y se baja al salir bien. Si al entrar sigue levantada, es que la vez anterior falló (Sider cortó el evento). Con 5 seguidos el módulo se apaga y lo anota en `sider.log` (el mismo método que `phoenix.lua`).

**HUD:** columnas de ancho fijo que cuentan letras, no bytes (las tildes no las descuadran). Ping con nota «bien» (≤ 60), «justo» (≤ 100) o «ALTO». Al pie, el estado del archivo.

---

## 8. Phoenix Link (C++): qué se cambió y por qué

| Archivo | Cambio |
|---|---|
| `phoenix/link/EstadoJuego.h/.cpp` (**nuevo**) | `juego::EstadoPartidoJuego` + `parsear()` + `leer()`. Lee con `CreateFileW` y `FILE_SHARE_READ|WRITE|DELETE` (no estorba al juego), ≤ 4 KB, quita el BOM y valida la fase y los rangos (goles 0–99, minuto ≤ 200, ≤ 40 goles, nombres ≤ 48 bytes sin controles). Edad por `last_write_time`. |
| `SmashSoda/CMakeLists.txt` | `EstadoJuego.cpp` va en `SKIP_UNITY_BUILD_INCLUSION` (incluye `windows.h` y tiene un espacio de nombres anónimo; en la build unity podría chocar). |
| `phoenix/link/BuzonJuego.h/.cpp` | `escribirAtomicoComo(carpeta, nombre, extension, …)`: el mismo `.tmp` + `ReplaceFileW`/`MoveFileExW` con 10 reintentos. `escribirAtomico` (avisos) ahora lo usa por dentro. |
| `phoenix/link/PhoenixLink.h/.cpp` | En `bucle()`: con la detección de PES cada 5 s se guarda `_estadoCarpeta` (solo si el puente está «Listo»); en cada vuelta (~1 s) se lee `estado.json`. **Si la lectura sale a medias** (archivo reciente pero ilegible, porque el Lua no puede renombrar), se queda el último dato bueno hasta 3 s. Se escribe `sala.txt` si cambió o cada 4 s, añadiendo `t=`. Nuevos `estadoJuego()` (inválido si el hilo lleva más de 4 s sin leer, por ejemplo esperando a la web) y `salaJuego(texto)`. |
| `phoenix/PhoenixPrefs.h/.cpp` | `competitivo=true`, `pausaAuto=false`, `marcadorAuto=true` en `phoenix-ui.json`. |
| `phoenix/web/Partido.h/.cpp` | `fijarGoles(a, b)`: el marcador entero, 0–99, solo en juego o en pausa. |
| `phoenix/web/InterfazWebInterno.h` | `struct Interno::Arbitro` (estado del árbitro) + declaraciones de `tickArbitro`, `mascaraBotonesPropia`, `bloqueoBotonesPropio`, `alternarBloqueoBotones`, `reaplicarArbitro` y `finalizarPartido`. |
| `phoenix/web/Arbitro.cpp` (**nuevo**) | El árbitro (abajo). |
| `phoenix/web/AccionesPartidoWeb.cpp` | `finalizarPartido(in, anunciar, extra)` común (historial, `partidosSesion`, `marcarPartido(false)`, evento web `partida_fin` y mensaje del bot). La acción `partido.finalizar` lo usa, y el árbitro también. |
| `phoenix/web/AccionesMandosWeb.cpp` | `mandos.competitivo`, `mandos.pausaAuto`, `mandos.marcadorAuto` (guardan la preferencia). `herramienta bloquearBotones` → `alternarBloqueoBotones`. `botonesBloq` → `reaplicarArbitro`. |
| `phoenix/web/EstadoWeb.cpp` | Llama a `tickArbitro(in)` en `tickDatos`, después de las muestras de red y presencia. Publica `juego{…}` y `arbitro{…}`. `mandos.bloqueoBotones` y `botonesBloq` muestran lo **del anfitrión**, sin lo que añade el modo competitivo. |

### El árbitro (`tickArbitro`, 2 veces por segundo; soltar Start se revisa en cada frame)
1. **Fase:** si `estado.json` es válido, manda el juego. Si no, manda el marcador de Link (`origen` = `juego` / `marcador`).
2. **Modo competitivo:** se activa con sala abierta, preferencia encendida y fase `en_juego`.
   - Al activarse guarda el interruptor del anfitrión (`GamepadClient::lockButtons`). La máscara ya está guardada en `Config::cfg.input.lockedGamepad*`.
   - Pone `lockButtons = true` y la máscara = la del anfitrión + `0x0010|0x0020|0x0400` (Start, Back, Guía).
   - **Si el anfitrión tenía el bloqueo apagado**, la máscara es solo esos 3 botones: sus gatillos y sticks marcados no se bloquean.
   - Al terminar vuelve exactamente lo anterior. **Nunca se escribe en `Config` ni en disco.**
   - Es el mismo camino que «Bloquear botones» (`GamepadClient.cpp`), que solo filtra la entrada de los **invitados**: el mando físico del anfitrión no se toca.
3. **Goles:** cuando cambian en el juego, `fijarGoles`. El lado A = local del juego, al revés tras «Cambiar lados» (`mandosInvertidos`). La corrección manual dura hasta el siguiente cambio en el juego.
4. **Arranque automático** (`marcadorAuto`): con el partido `listo` (lados elegidos), la sala abierta, `en_juego` y el reloj corriendo → `iniciar`, evento `partida_inicio` (`auto:true`) y «¡Arrancó!».
5. **Relato** (`marcadorAuto`):
   - goles nuevos de `goles[]` → «GOL 41' de X. A 2-2 B». Al sincronizar por primera vez, o si se anula un gol, no se anuncia nada;
   - `en_juego` → `descanso` → «Descanso: …»;
   - `final` + `completo` → `finalizarPartido` con « Goles: 12' A, 20' B…»;
   - `en_juego` → `menu` (abandono) → aviso una sola vez. El marcador queda abierto.
6. **Pausa automática** (`pausaAuto`): un jugador de un lado (no el anfitrión) **sale de la sala** o tiene **5 muestras seguidas > 150 ms** en `MonitorRed` (1 por segundo) →
   - aviso en el chat, `partido.pausa` y evento `pausa` (`auto:true`);
   - **solo si el juego dice `en_juego` y el reloj corre**, Start durante ~150 ms (`ProveedorSala::inyectar`) en un **mando virtual conectado y libre**; si no hay, en el del anfitrión; **nunca** en el que el anfitrión maneja con Ctrl+Alt (`MandoHost` lo pisaría);
   - si el reloj está quieto (repetición o celebración: Start la saltaría), espera hasta 30 s a que vuelva a correr;
   - sin `estado.json` no se pulsa nada;
   - cada problema se avisa una vez, hasta que el jugador se recupera.
7. **`sala.txt`:** `textoSalaJuego()` lo arma con `ProveedorSala::asientos`, `PhoenixPrefs::equipoLocal/mandosActivos`, los lados del partido y `Hosting::getMessageLog()`.

Todo va dentro de `try/catch`: el árbitro nunca tumba la interfaz.

---

## 9. Interfaz (dentro del diseño existente; no se rehízo nada)
- **Partido › En vivo**, debajo del marcador:
  - «Juego: en juego · 34'» (o «sin datos del juego»);
  - los equipos del juego;
  - «Start/Back/Guía bloqueados a los invitados» cuando aplica;
  - «Goles: 12' Local · 20' Visita…».
  (`ui/js/vistas/partido.js`, componente `LineaJuego`.)
- **Mandos › Bloqueo**, columna izquierda, debajo de «Bloqueo general»: tarjeta **ÁRBITRO DEL PARTIDO** con «Modo competitivo», «Marcador automático», «Pausa automática (prueba)», la marca «START BLOQUEADO» y la última pausa automática. (`ui/js/vistas/botones.js`, componente `Arbitro`.)
- **Simulador** (`ui/js/simulador.js`):
  - `?estado=estado-prueba.json` lee ese archivo cada segundo como si fuera `estado.json`;
  - imita el árbitro: goles → marcador, relato de goles y final automático;
  - acciones `mandos.competitivo|pausaAuto|marcadorAuto`.

---

## 10. Cómo se probó
1. **Compilación:** `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release` → 0 errores (4 veces a lo largo del trabajo). Sale `x64\Release\PhoenixLink.exe` con `ui\` copiada.
2. **Simulador:**
   - servidor estático propio con node (puerto 8771; el `.mjs` necesita `text/javascript`), en `ui/index.html?escenario=llena&estado=estado-prueba.json`;
   - `estado-prueba.json` escrito a mano en `ui/` y **borrado** al terminar;
   - lo que se comprobó:
     - `en_juego` 2-1 → el marcador pasa a lo del juego (3-1), «Juego: en juego · 34'» y «START BLOQUEADO»;
     - `pausado` → el bloqueo se quita;
     - sin archivo → «sin datos del juego» y el bloqueo sigue por el marcador manual;
     - GOL manual → se respeta;
     - v2 con 3 goles → «Goles: 12' Local · 20' Visita · 30' Local», sin anunciar los viejos;
     - gol 41' nuevo → «GOL 41' de Visita. Local 2-2 Visita» en el chat;
     - `final` + `completo` con gol 88' → anuncio del gol y final automático. El partido vuelve a «armar lados».
3. **Lua:** revisado a mano contra los globales reales de Sider (sección 4). **No se ejecutó**: no hay intérprete de Lua en la PC y no se descargó ninguno.

---

## 11. Decisiones y por qué
| Decisión | Por qué |
|---|---|
| Módulo Lua aparte, no dentro de `phoenix.lua` | `phoenix.lua` es de SYNC y parchea código; lo nuevo solo lee. Separados se apagan y depuran por separado. |
| Fase deducida del reloj | `match.stats()` no informa de la pausa. El juego tampoco expone un evento de final en 7.3.3 (`goal_scored` no existe). |
| Final solo al salir del partido | El «reloj quieto en el 90'» fallaría con repeticiones de goles en el añadido. |
| Start solo con el reloj corriendo | Un Start durante una repetición la salta en vez de pausar. |
| Mando libre antes que el del anfitrión, nunca el de Ctrl+Alt | `MandoHost::tick` reescribe ese mando en cada frame. |
| Guardar el último `estado.json` bueno 3 s | El Lua no puede renombrar: una lectura puede caer a mitad de una escritura. |
| No tocar `Config` en el modo competitivo | Nada queda guardado en disco si la app se cierra en pleno partido. |
| Ping redondeado a 10 ms en `sala.txt` | Evita escribir el archivo por cambios mínimos. |
| Nombres de equipo desde `kit-server\map.txt` | Ya existe en el parche y es fiable. Leer la memoria para eso iría contra la regla de solo lectura. |
| Pausa automática apagada por defecto | Pulsa un botón real en el juego: primero hay que verla funcionar. |

---

## 12. Riesgos y supuestos sin verificar en el juego
1. Que `match.stats()` pase a `nil` al salir del partido. Si sigue dando datos en las pantallas posteriores, el final automático no llega y hay que pulsar TERMINAR.
2. Que el descanso deje el reloj quieto en el 45' con periodo 1. Si el juego cambia al periodo 2 al instante, no se anuncia el descanso (no rompe nada).
3. Que el Start de un mando ViGEm **libre** (sin dueño) pause PES. Si PES solo hace caso a los mandos asignados, la pausa del juego no ocurre (el marcador sí se pausa).
4. Que el lado A de Link sea el local de PES (mandos 1..equipoLocal = local).
5. Que el minuto del gol (`clock_minutes + 1`, con tope por periodo) coincida con lo que muestra PES.
6. Que el Lua no tenga un error de sintaxis. Si lo tuviera, Sider no lo carga y el juego sigue normal (pasó con la v0.1 de `phoenix.lua`).

Cada prueba se anota en `PhoenixSync/PRUEBAS.md`.

---

## 13. Cómo replicarlo desde cero (orden)
1. Leer `CLAUDE.md`, `COORDINACION.md`, el final de `REGISTRO-LINK.md`, `PhoenixSync/sider/RIESGOS-SIDER.md`, `VINCULO-TIEMPO-REAL.md` y la `phoenix.lua` instalada en el juego.
2. Confirmar el entorno en el juego real: los globales en `sider.log` (volcado de `env.lua`), `match-stats.enabled = 1` en `sider.ini`, `sifflet.lua` usando `display_frame` y el `map.txt` de kit-server.
3. Link, lector: `EstadoJuego.*` (parseo estricto + lectura compartida) + `SKIP_UNITY` en CMake. Engancharlo en `PhoenixLink::bucle` (carpeta del buzón cada 5 s, lectura en cada vuelta, último bueno 3 s).
4. Link, preferencias: `competitivo`, `pausaAuto` y `marcadorAuto` en `PhoenixPrefs`.
5. Link, árbitro: `Interno::Arbitro`, `Arbitro.cpp` (secciones 8.1–8.7), `Partido::fijarGoles`, `finalizarPartido` común y las llamadas en `tickDatos`.
6. Link, acciones y estado: `AccionesMandosWeb.cpp` y `EstadoWeb.cpp` (mostrar lo del anfitrión sin lo competitivo).
7. Link, `sala.txt`: `escribirAtomicoComo` + `PhoenixLink::salaJuego` + `textoSalaJuego`.
8. UI: `LineaJuego` en `partido.js`, tarjeta `Arbitro` en `botones.js`, simulador. Sin crear secciones ni pestañas.
9. Compilar con `cmake --build … --config Release` (nunca el `.bat`) y probar en el simulador con un `estado.json` escrito a mano. Borrarlo después.
10. Lua: `phoenix_estadio.lua` con solo los globales de la sección 4, sin `pcall` ni `os.rename`, con bandera de errores y lectura solo de `match.stats()` y `ctx`.
11. Documentar: `PROTOCOLO.md`, `sider/LEEME.md`, este documento, una línea en `REGISTRO-LINK.md` y otra en `PhoenixSync/REGISTRO.md`, y el prompt para SYNC.
12. Instalar (con el OK de Fralex) y probar según `sider/LEEME.md`, en orden: offline → con Link → con amigos. La pausa automática, al final.

---

## 14. Instalación, prueba y desinstalación
Ver `SmashSoda/phoenix/sider/LEEME.md`.
- **Instalar (desde el 10-oct, con un botón):** Phoenix Link › SYNC › Puente › tarjeta «MÓDULOS DEL JUEGO» › INSTALAR, con PES cerrado (sección 16).
- **Instalar a mano (si hace falta):** copiar el `.lua` a `modules\` en las dos carpetas (modo de ConmeGOL y raíz), guardar una copia de `sider.ini` y añadir `lua.module = "phoenix_estadio.lua"` debajo de `phoenix.lua`.
- **Comprobar:** `[estadio] v1.0 listo` en `sider.log`. La tarjeta también lo dice («Cargó en el último arranque»).
- **Desinstalar:** el botón APAGAR (comenta la línea), o quitarla a mano.

---

## 16. Instalador «Módulos del juego» y cambio de regla (2026-10-10)

**Decisión de Fralex:** se quita la regla «Link nunca toca `sider.ini`», **solo para este instalador**. El buzón y el repartidor siguen igual: no crean carpetas ni tocan `sider.ini`. Anotado en `COORDINACION.md`.

**Piezas:**

| Archivo | Qué hace |
|---|---|
| `phoenix/link/ModulosJuego.h/.cpp` (**nuevo**, `SKIP_UNITY`) | Catálogo, estado, instalar y apagar. Lógica pura de `sider.ini` (`activarLinea`, `comentarLinea`, `buscarLinea`, `versionDe`). |
| `phoenix/web/AccionesModulosWeb.cpp` (**nuevo**) | Acciones `sync.modulos` (lista) y `sync.modulo{archivo, accion: instalar\|quitar}`. |
| `ui/js/vistas/sync.js` | Tarjeta «MÓDULOS DEL JUEGO» en SYNC › Puente, columna derecha, debajo de «Últimos avisos». No cambia secciones ni pestañas. |
| `SmashSoda/CMakeLists.txt` | Copia `phoenix/sider/` a `<exe>\sider\` al compilar: el paquete que instala Link. |
| `pruebas-link/modulos_test.cpp` (**nuevo**) | Prueba de la lógica pura y de una instalación completa sobre una **copia** de juego. |

**Cómo instala** (`instalar`):
1. **Comprueba todo antes de escribir nada:** que exista `modules\` en cada destino, que `sider.ini` se pueda leer y que tenga una lista `lua.module`. Si algo falta, sale «no se tocó nada».
2. **Copia el `.lua`** a `modules\` de cada destino (raíz del juego + cada `<juego>\<carpeta>\<modo>\SiderAddons` con `sider.ini`), de forma atómica (`.tmp` + reemplazo) y comprobando el sha256. Sin la línea en `sider.ini` el archivo todavía no hace nada.
3. **Una línea por `sider.ini`:** `lua.module = "<archivo>"` debajo del último `lua.module` activo (en ConmeGOL, debajo de `phoenix.lua`). Si estaba comentada, le quita el `;`. Se respetan el BOM y los finales de línea (CRLF).
   - Antes de escribir se copia `sider.ini.phoenix-AAAAMMDD-HHMMSS`.
   - Se escribe de forma atómica.
   - Si un destino falla, los ya escritos vuelven a su copia.
4. **Requisitos:** PES cerrado (lo comprueba la acción) y la carpeta del juego conocida (la del PES abierto, o la última guardada en `PhoenixPrefs::carpetaJuego`).

**Apagar** (`quitar`): comenta la línea (`;lua.module = …`), con copia previa. El `.lua` se queda. Solo para los módulos que lleva Link: **`phoenix.lua` nunca se toca** (se muestra como «LO GESTIONA SYNC»).

**Estado que muestra la tarjeta:** `instalado`, `desactualizado` (Link lleva otra versión), `apagado`, `a_medias` (no está igual en todas las carpetas), `no_instalado` o `solo_lectura`. Detalle por carpeta: ● activo, ○ comentado, · sin línea. Además, lo que dijo el último `sider.log`: «Cargó» o «Module (…) is NOT activated».

**Pruebas (10-oct):**
- `modulos_test.exe`, compilado con MSVC, **TODO OK**: lógica pura; instalar en 2 carpetas con 2 copias; segunda instalación sin cambios; apagar; versión nueva → actualizar; rechaza `phoenix.lua` y nombres raros.
- Con una **copia** del `sider.ini` real de ConmeGOL, el resultado difiere del original en **exactamente una línea** (la 201, debajo de `phoenix.lua`).
- La tarjeta se ve en el simulador.
- **No se instaló nada en el juego real.**

**Futuro:** actualizaciones automáticas desde la web, solo con módulos **firmados** (Ed25519, como `/liga/cambios`), porque un Lua ejecuta código dentro del juego.

---

## 15. Archivos de este trabajo
**Nuevos:**
- `SmashSoda/phoenix/link/ModulosJuego.h/.cpp`, `SmashSoda/phoenix/web/AccionesModulosWeb.cpp`, `pruebas-link/modulos_test.cpp` (instalador, §16)
- `SmashSoda/phoenix/sider/phoenix_estadio.lua`
- `SmashSoda/phoenix/sider/LEEME.md`
- `SmashSoda/phoenix/link/EstadoJuego.h`
- `SmashSoda/phoenix/link/EstadoJuego.cpp`
- `SmashSoda/phoenix/web/Arbitro.cpp`
- `docs/PHOENIX-ESTADIO-ARBITRO.md` (este documento)
- `PhoenixSync/prompts/PROMPT-SYNC-estado-partido.md` (en la carpeta principal, rama `mercado-fase0`)

**Modificados:**
- `SmashSoda/CMakeLists.txt`
- `phoenix/PhoenixPrefs.h/.cpp`
- `phoenix/link/BuzonJuego.h/.cpp`
- `phoenix/link/PhoenixLink.h/.cpp`
- `phoenix/web/Partido.h/.cpp`
- `phoenix/web/InterfazWebInterno.h`
- `phoenix/web/EstadoWeb.cpp`
- `phoenix/web/AccionesMandosWeb.cpp`
- `phoenix/web/AccionesPartidoWeb.cpp`
- `phoenix/web/PROTOCOLO.md`
- `ui/js/simulador.js`
- `ui/js/vistas/botones.js`
- `ui/js/vistas/partido.js`
- `REGISTRO-LINK.md`
- `PhoenixSync/REGISTRO.md`

**No son de este trabajo, aunque estén sin commitear en el mismo worktree:**
- `AccionesAjustesWeb.cpp` (`ajustes.overlayMenu`, otra sesión);
- los demás `ui/js/*` (mejoras de la interfaz original);
- `docs/REDISEÑO-UI.md` y `docs/rediseño-capturas/`.
