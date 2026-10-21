# Phoenix Link · Campo 2D en vivo, Phoenix Estadio y árbitro

Lectura en vivo de las posiciones de los 22 jugadores y del balón (campo 2D en Link y en la web) y el árbitro y la pausa de Link con el módulo `phoenix_estadio.lua`.

Incluye cómo se investigó para que otra persona o IA lo replique, y todos los errores del camino.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 4 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Phoenix Estadio + Árbitro de Link — documento completo para replicarlo](#parte-1-phoenix-estadio-arbitro-de-link-documento-completo-para-replicarlo) — origen `docs/PHOENIX-ESTADIO-ARBITRO.md` (1f308b5 2026-10-11)
- [Parte 2 · Campo 2D en vivo — investigación de posiciones en PES 2021](#parte-2-campo-2d-en-vivo-investigacion-de-posiciones-en-pes-2021) — origen `docs/CAMPO-2D-INVESTIGACION.md` (bee8ae6 2026-10-11)
- [Parte 3 · Campo 2D en vivo (Partido › Campo)](#parte-3-campo-2d-en-vivo-partido-campo) — origen `docs/CAMPO-2D.md` (547ddf1 2026-10-14)
- [Parte 4 · Cómo replicar la investigación del campo 2D (para otro humano u otra IA)](#parte-4-como-replicar-la-investigacion-del-campo-2d-para-otro-humano-u-otra-ia) — origen `docs/CAMPO-2D-REPLICAR.md` (5da51b8 2026-10-16)

---

## Parte 1 · Phoenix Estadio + Árbitro de Link — documento completo para replicarlo

> Origen: `docs/PHOENIX-ESTADIO-ARBITRO.md` · último cambio 1f308b5 2026-10-11 · 40822 bytes. Copia íntegra.

**Fecha:** 2026-10-10 · **Autor:** chat LINK (Claude), a pedido de Fralex · **Rama:** `rediseño-phoenix-portal` (worktree `C:\dev\smash-soda-fork\_phoenix-link`)
**Estado:** Link compila sin errores y está probado en el simulador. El módulo Lua está escrito y revisado a mano, pero **no se ha ejecutado** (no hay intérprete de Lua en la PC) y **no está instalado**. Nada está commiteado.

Este documento explica **qué se hizo, cómo, por qué y con qué cuidados**, para poder rehacerlo desde cero. Las instrucciones cortas de instalación están en `SmashSoda/phoenix/sider/LEEME.md`, y el protocolo de la interfaz en `SmashSoda/phoenix/web/PROTOCOLO.md`.

---

### 1. Qué es

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

### 2. Reglas y contexto que se tomaron en cuenta

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

### 3. El paquete de Luas: cómo conviven `phoenix.lua` y `phoenix_estadio.lua`

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

### 4. Investigación previa: cómo se supo lo que permite Sider

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

### 5. Flujo

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

### 6. Contrato de archivos (`<juego>\SiderAddons\content\phoenix\`)

#### `estado.json` (lo escribe el Lua; Link solo lo lee)
Una línea, UTF-8, sin BOM:
```json
{"v":2,"fase":"en_juego","minuto":33,"segundo":12,"periodo":1,"anadido":0,"reloj_corre":true,
 "local":101,"visita":102,"nombre_local":"Universitario","nombre_visita":"Alianza Lima",
 "goles_local":2,"goles_visita":1,"pk_local":0,"pk_visita":0,
 "goles":[{"m":12,"l":"l"},{"m":20,"l":"v"},{"m":30,"l":"l"}],"completo":false,"torneo":0,"seq":40}
```
- `fase`: `menu` | `en_juego` | `pausado` | `descanso` | `final`. Link rechaza cualquier otra.
- `goles[].l`: `"l"` = local del juego, `"v"` = visita. `m` = minuto (tope 45/90/105/120 según el periodo).
- **v1.1 (Sider 7.3.4+):** cada gol puede traer también `"j"` (nombre de la camiseta, máx. 24 bytes UTF-8), `"id"` (id del jugador en el juego) y `"pp":true` (en propia puerta). Ver §17.
- `completo`: solo cuenta con `fase:"final"`. Es `true` si se llegó al 89' de la 2.ª parte o a la prórroga o los penales.
- **Validez en Link:** el archivo existe, tiene ≤ 5 s (fecha de modificación) y se entiende. Si no, «sin datos del juego» y todo vuelve a manual.
- Link también entiende la **v1** (sin los campos nuevos).

#### `resultado.json` (lo escribe el Lua)
Lo mismo con `fase:"final"`, `completo:true` y `"fin":<hora Unix>`. Se escribe una vez al terminar un partido completo, para subirlo a la web más adelante (hoy Link no lo usa).

#### `sala.txt` (lo escribe Link de forma atómica; el Lua solo lo lee)
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

#### `avisos.txt`
Ya existía: lo escribe Link y lo lee `phoenix.lua`. No cambió.

---

### 7. El módulo `phoenix_estadio.lua` (`SmashSoda/phoenix/sider/`)

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

### 8. Phoenix Link (C++): qué se cambió y por qué

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

#### El árbitro (`tickArbitro`, 2 veces por segundo; soltar Start se revisa en cada frame)
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

### 9. Interfaz (dentro del diseño existente; no se rehízo nada)
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

### 10. Cómo se probó
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

### 11. Decisiones y por qué
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

### 12. Riesgos y supuestos sin verificar en el juego
1. Que `match.stats()` pase a `nil` al salir del partido. Si sigue dando datos en las pantallas posteriores, el final automático no llega y hay que pulsar TERMINAR.
2. Que el descanso deje el reloj quieto en el 45' con periodo 1. Si el juego cambia al periodo 2 al instante, no se anuncia el descanso (no rompe nada).
3. Que el Start de un mando ViGEm **libre** (sin dueño) pause PES. Si PES solo hace caso a los mandos asignados, la pausa del juego no ocurre (el marcador sí se pausa).
4. Que el lado A de Link sea el local de PES (mandos 1..equipoLocal = local).
5. Que el minuto del gol (`clock_minutes + 1`, con tope por periodo) coincida con lo que muestra PES.
6. Que el Lua no tenga un error de sintaxis. Si lo tuviera, Sider no lo carga y el juego sigue normal (pasó con la v0.1 de `phoenix.lua`).

Cada prueba se anota en `PhoenixSync/PRUEBAS.md`.

---

### 13. Cómo replicarlo desde cero (orden)
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

### 14. Instalación, prueba y desinstalación
Ver `SmashSoda/phoenix/sider/LEEME.md`.
- **Instalar (desde el 10-oct, con un botón):** Phoenix Link › SYNC › Puente › tarjeta «MÓDULOS DEL JUEGO» › INSTALAR, con PES cerrado (sección 16).
- **Instalar a mano (si hace falta):** copiar el `.lua` a `modules\` en las dos carpetas (modo de ConmeGOL y raíz), guardar una copia de `sider.ini` y añadir `lua.module = "phoenix_estadio.lua"` debajo de `phoenix.lua`.
- **Comprobar:** `[estadio] v1.0 listo` en `sider.log`. La tarjeta también lo dice («Cargó en el último arranque»).
- **Desinstalar:** el botón APAGAR (comenta la línea), o quitarla a mano.

---

### 16. Instalador «Módulos del juego» y cambio de regla (2026-10-10)

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

**Apagar** (`quitar`): comenta la línea (`;lua.module = …`), con copia previa. El `.lua` se queda. Solo para los módulos que lleva Link.
> **Actualización (2026-10-10, commits 0fdbba1 y a3558b6, otra sesión de LINK, a pedido de Fralex):** Link ahora **también lleva e instala `phoenix.lua` v0.18** (en `SmashSoda/phoenix/sider/`), pensado para la PC de un amigo. Además:
> - crea `content\phoenix` y `livecpk\Phoenix-DB\common\etc\pesdb` vacías;
> - añade `cpk.root ".\livecpk\Phoenix-DB"` antes de la primera raíz con la base del parche (informe 27 de Sync);
> - guarda `phoenix.lua.antes-<fecha>`.
>
> Ojo: la v0.18 aún no está probada en el juego y parchea código en memoria. Mejor con el modo ACTIVAR. El detalle está en `COORDINACION.md` y en `ModulosJuego.h`.

**Estado que muestra la tarjeta:** `instalado`, `desactualizado` (Link lleva otra versión), `apagado`, `a_medias` (no está igual en todas las carpetas), `no_instalado` o `solo_lectura`. Detalle por carpeta: ● activo, ○ comentado, · sin línea. Además, lo que dijo el último `sider.log`: «Cargó» o «Module (…) is NOT activated».

**Pruebas (10-oct):**
- `modulos_test.exe`, compilado con MSVC, **TODO OK**: lógica pura; instalar en 2 carpetas con 2 copias; segunda instalación sin cambios; apagar; versión nueva → actualizar; rechaza `phoenix.lua` y nombres raros.
- Con una **copia** del `sider.ini` real de ConmeGOL, el resultado difiere del original en **exactamente una línea** (la 201, debajo de `phoenix.lua`).
- La tarjeta se ve en el simulador.
- **No se instaló nada en el juego real.**

**Futuro:** actualizaciones automáticas desde la web, solo con módulos **firmados** (Ed25519, como `/liga/cambios`), porque un Lua ejecuta código dentro del juego.

---

### 17. Goleador del partido (phoenix_estadio v1.1 · Link 0.2.9 · 2026-10-11)

**Qué se investigó** (para no adivinar direcciones de memoria):
- La documentación oficial de Sider (`doc/scripting.html`) dice que `match.stats()` solo da el marcador, los penales, el periodo, el reloj y el añadido. **No da goleadores, tarjetas, cambios, posesión ni tiros.**
- El código fuente oficial (`github.com/pes-modding/sider7`, `sider.cpp`) tiene desde la **7.3.4** (16-feb-2026) un evento nativo **`goal_scored(ctx, info)`**. Trae `player_id`, `player_offset`, `home_or_away` y `player_name_on_shirt` (leído en `player_info + 0x174`). No sale en el historial de versiones, pero está en el código de las etiquetas v7.3.4, v7.4.0 y v7.4.1, y no en la v7.3.3.
- Comparando las etiquetas v7.3.3 y v7.4.1, el **único evento nuevo** es `goal_scored`. No hay eventos de tarjetas, cambios ni faltas.
- En la PC de Fralex, Sider es la **7.3.3** (`sider.log`). Su `goalscreams.lua` ya registra `goal_scored` y el log dice `WARN: trying to register for unknown event: "goal_scored"`: hoy no funciona.
- En 7.3.3, registrar un evento desconocido solo escribe ese WARN (código de `sider_context_register`); no rompe nada.

**Qué hace ahora:**
1. **Lua:**
   - `ctx.register("goal_scored", …)`. El nombre se limpia (sin controles; UTF-8 válido o se descarta; máx. 24 bytes).
   - Se junta con el gol que marca `match.stats()` dentro de una ventana de 12 s, llegue antes o después.
   - Si en ese lado no hay un gol sin nombre, se busca en el otro y se marca `pp` (gol en propia puerta).
   - En la tanda de penales se ignora.
   - Además, corrige un fallo de la v1.0: pasados 60 goles el marcador dejaba de seguirse. Ahora el marcador siempre se sigue y solo se dejan de apuntar minutos pasados 40 goles.
2. **Link:**
   - `EstadoJuego` lee `j`, `id` y `pp`. Corta los nombres sin partir una letra UTF-8, porque un UTF-8 roto haría fallar el `json::dump` de lo que va a la web. El tope de lectura sube a 8 KB.
   - El caso más grande que escribe el Lua (40 goles con nombre) ocupa 3.356 bytes: también cabe en un Link viejo, que lee 4 KB.
3. **Chat:** «GOL 34' de Fralex (MESSI). Fralex 1-0 Mirko». Si falta el nombre, Link lo espera como mucho 3 s. Con Sider 7.3.3 sale igual, sin nombre.
4. **Web:**
   - `liga_vivo.goles`, `detalle.juego.goles` y `partida_fin.juego.goles` llevan `j`/`jugador` y `pp`/`propia`.
   - El «en vivo» se reenvía cuando llega un nombre.
   - Función `liga` v2 desplegada: guarda `j` cortado por letras (máx. 32) y `pp`.

**Pruebas:**
- `pruebas-link/estadojuego_test.cpp` (MSVC): TODO BIEN, sin avisos con /W4.
- Simulador: goles con nombre y «en propia», y relato en el chat.
- Build de Release sin errores.
- **El Lua aún no se ha ejecutado en el juego** (ni hay intérprete Lua en la PC).

**Para que funcione en el juego:**
- Instalar `phoenix_estadio.lua` v1.1 desde Sync › Módulos del juego.
- **Actualizar Sider a 7.3.4.** Es lo mínimo: solo trae la corrección de Windows 11 y este evento.
- No se recomienda 7.4.x por ahora: rompe el truco de memoria de *colorsdemo* y los módulos `CGP_*` de ConmeGOL vienen compilados, así que no se puede comprobar si lo usan.
- Con Sider 7.3.4, además empieza a funcionar `goalscreams.lua` de ConmeGOL, que ya está instalado.

**Instalado en la PC de Fralex (11-oct-2026, con su permiso):** Sider **7.3.4** oficial (`sider-7.3.4.zip`, sha256 `0fcb4b04…`, igual al que publica GitHub) en `D:FrankGames_Conmegol PatchSiderAddons`. Solo se cambiaron `sider.dll` y `sider.exe`. La copia de la 7.3.3 está en `SiderAddonssider-7.3.3-respaldo` (sha256 `5c77dd83…` y `5eb05e97…`). Para volver: `SiderAddonsVOLVER-A-SIDER-7.3.3.bat` (con PES cerrado). Falta comprobar en `sider.log` la primera vez que se abra el juego.

**Lo que sigue sin poderse saber** sin ingeniería inversa (direcciones de memoria por exe/parche, frágil): tarjetas, cambios, asistencias, posesión, tiros y faltas. Se haría con eventos personalizados de Sider (`custom-events.html`, x64dbg), y solo después de probar lo básico en el juego.

---

### 15. Archivos de este trabajo
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

---

### 18. Phoenix Referee y pantalla del juego (v1.2 · Link 0.2.19 · web ronda 225 · 2026-10-11)

**Nombre nuevo.** Fralex pidió que el Lua se llame «referee», porque hace de árbitro. Ahora es `SmashSoda/phoenix/sider/phoenix_referee.lua` (v1.2); antes era `phoenix_estadio.lua`. El resto del documento conserva el nombre viejo como historia.
- **Al instalar desde «Módulos del juego»** (`ModulosJuego.cpp`, mapa `kReemplaza`), en la misma escritura del `sider.ini` (con copia):
  - se quita la línea del viejo;
  - después se aparta su `.lua` como `phoenix_estadio.lua.reemplazado-<fecha>`.
  - Nunca corren los dos: escribirían el mismo `estado.json`.
- **Paquete de Link:** al compilar, la carpeta `sider\` se vacía antes de copiar (`CMakeLists.txt`), así no queda el viejo.
- **Phoenix Doctor:** revisa `phoenix_referee.lua`, y `phoenix_estadio.lua` solo si sigue ahí.
- **Prueba:** `pruebas-link/modulos_test.cpp`, sobre una copia del `sider.ini` real: instalar el nuevo quita la línea vieja, aparta el archivo y el viejo deja de salir en la lista. TODO OK.

**Pantalla del juego.** Fralex quiere que la web diga en qué parte del juego están.
- **Lo que da Sider** (documentación oficial, `doc/scripting.html`):
  - `context_reset` → vuelta al menú principal;
  - `set_stadium_choice` / `set_teams` → eligiendo equipos;
  - `set_match_settings` → ajustes del partido;
  - `after_set_conditions` → cargando.
  - Ninguno de los manejadores devuelve nada, así que no cambian lo que eligen otros módulos (StadiumServer, kserv…).
- **Dentro del partido**, la pantalla sale del reloj:
  - `saliendo` = periodo 0, o 1.er tiempo con el reloj aún sin correr;
  - luego `en_juego`, `descanso`, `pausa` y `final`.
- **El Lua escribe `pantalla.json`** (`{v:1, pantalla, t}`) cada 2 s y al momento si cambia, **también fuera del partido**.
  - Va **aparte** de `estado.json` a propósito. Si `estado.json` existiera en los menús, cambiaría el árbitro: el modo competitivo sigue «estado válido» y `abandono_juego` usa `fase:"menu"`.
- **Link** (`EstadoJuego::leerPantalla`, `PhoenixLink::pantallaJuego`):
  - la da por perdida a los 6 s;
  - una lectura a medias deja la última buena durante 3 s.
- **El árbitro** (`Arbitro.cpp` 2d):
  - manda `pantalla` con el «en vivo»;
  - con los lados ya elegidos en Link (`Listo`), antes del pitazo, manda la previa (`fase:"previa"`, 0-0) con su pantalla.
  - Se borra de la web si se cancela.
- **Web:**
  - migración **103** (`liga_vivo.pantalla`) y Edge `liga` v3;
  - la tarjeta dice «Eligiendo equipos», «Ajustes del partido», «Cargando el partido», «Saliendo al campo», «34′ · 2.º tiempo», «67′ · En pausa», «Descanso» o «Final», y muestra «VS» antes del pitazo.

**Sin evento (investigación pendiente):** «viendo estrategia» y «pausa haciendo formación o cambios».
- La idea es una sesión de prueba con un Lua espía que anote los archivos que lee el juego (`livecpk_make_key`) al entrar en cada pantalla.
- Si hay archivos propios de esas pantallas, se pueden detectar. Si no, haría falta leer la memoria del juego, que es frágil.

**Sin probar en el juego todavía:**
- el orden real de los eventos en cada pantalla;
- si `match.stats()` ya existe mientras sale el equipo al campo.
- El Lua solo anota la pantalla y escribe un archivo: si algo sale en otro orden, la web muestra otra etiqueta, pero no se rompe nada.

---

## Parte 2 · Campo 2D en vivo — investigación de posiciones en PES 2021

> Origen: `docs/CAMPO-2D-INVESTIGACION.md` · último cambio bee8ae6 2026-10-11 · 16683 bytes. Copia íntegra.

> Estado (11 oct 2026): **las posiciones de los jugadores SÍ son legibles** desde fuera del juego (solo lectura).
> Falta cerrar: de qué equipo es cada fichita y dónde está el balón. Todo lo de aquí es **ingeniería inversa propia**
> sobre el `PES2021.exe` de ConmeGOL; no hay nada publicado por la comunidad (ver «Fuentes»).

### 1. Fuentes buscadas (y qué dieron)

| Fuente | Resultado |
|---|---|
| Búsquedas web «PES 2021 player position / ball position address Cheat Engine» | Nada con direcciones de posiciones en campo. Solo tablas de edición de jugadores/ML. |
| [PES-2021-Cheat-Table de xAranaktu](https://github.com/xAranaktu/PES-2021-Cheat-Table) y el *Live Editor* de Ruls | Editores de datos de jugadores (habilidades, contratos). No tocan el partido. |
| Módulos de Sider instalados (`camera.lua`, `*Cam.lua`, `etrace.lua`) | Usan `memory.search_process` para parchear código de cámaras; no leen posiciones. |
| Sider 7.3.x (`match.stats()`, eventos) | Marcador, minuto, goles: **no entrega posiciones**. |

Conclusión: había que encontrarlas. No se copió código ajeno.

### 2. Método (todo en `tools/radar/`, ninguna pieza forma parte de Link)

1. `escaner.cpp` — búsqueda tipo Cheat Engine sobre `PES2021.exe` con `ReadProcessMemory` (solo `PROCESS_VM_READ`;
   para el modo por pasos también `PROCESS_SUSPEND_RESUME`, que es lo mismo que pausar en un depurador).
   - Floats «con pinta de coordenada» (0.02 ≤ |v| ≤ 6000).
   - **Modo por pasos**: el juego avanza ~50 ms, se congela, se leen TODOS los candidatos con el juego quieto. Las posiciones
     de un jugador cambian poco entre pasos; casi todo el ruido salta o no cambia. Pasaron de 50 M a ~380 mil «suaves».
   - **Arrays de punteros**: busca tablas de ≥20 punteros cuyos objetos tengan, todos al mismo desplazamiento, un candidato
     suave. Esto fue lo que dio con la estructura (el análisis por «paso constante» no sirvió: los objetos están en un pool con
     paso 0x280 pero repartidos en muchos bloques).
2. `sonda.cpp` / `objeto.cpp` — lectura en vivo de un array de punteros y volcado de los objetos.
3. `entes.cpp` — **escáner por firma** del objeto jugador (ver §3). Tarda ~5 s sobre 5 GB; las direcciones son estables
   mientras dura el partido (comprobado: 48/48 iguales entre el minuto 1 y el 3).
4. `buscaptr.cpp`, `analiza.cpp`, `bola.cpp` — auxiliares (quién apunta a una dirección, análisis offline, búsqueda del balón).

Las pausas del juego durante la investigación se hicieron con `NtSuspendProcess`/`NtResumeProcess` (siempre con `finally`
para reanudar). **Link NO hará eso**: la lectura final no congela nada.

### 3. Lo encontrado

#### 3.1 Objeto «actor de jugador» (0x280 = 640 bytes)

Posiciones dentro del objeto (offsets en bytes desde el inicio del objeto):

| Offset | Contenido | Notas |
|---|---|---|
| +208 | `float4 (x, y, z, 1.0)` | **Posición.** x = largo del campo, z = ancho, y ≈ 0.9 (altura de la cadera), w = 1.0 exacto. |
| +112 | otra copia de la posición (idéntica a +208 en reposo) | |
| +248 | `u32` tipo: `0x18` = jugador físico en el campo | `0x8` = «cerebro» de IA (y = 0), `0x14`, `0xF`, `0x26`: otros |
| +312 | `u32` 1 o 2 | en las pruebas partía los 22 en 11/11 pero **no es el equipo** (ver §4) |
| +440 | `u32` = `(id de equipo << 16) \| 3` | 0x2C0003 = 44 (Barcelona), 0x430003 = 67 (Man City) |
| +492 | `u32` = `0x80000 \| 1..2` en jugadores en campo | parte de la firma |

- Unidades: **metros**, centro del campo = (0,0), x ∈ [−52.5, 52.5], z ∈ [−34, 34] (las posiciones de los jugadores en
  pantalla coinciden con el dibujo en planta; velocidades de 2–3 m/s al trotar).
- Hay **dos listas** (una por «equipo de IA»): cada lista tiene 23 actores con las posiciones de **todos** los jugadores
  (22 + árbitro). La lista con código 44 y la lista con código 67 dan las mismas posiciones: por eso salían «parejas» a
  0.3–0.8 m. Para dibujar basta **una** lista.
- Los objetos están en el heap (direcciones distintas en cada partido y en cada arranque): hay que **redescubrirlos con la
  firma** al empezar el partido; las direcciones valen mientras dura el partido.
- Verificación hecha: congelando el juego, captura de pantalla y volcado de memoria en el mismo instante; el dibujo en
  planta de las 23 posiciones coincide con la disposición de la jugada (grupo de jugadores junto al área, árbitro en la banda).

#### 3.2 Firma de búsqueda (la que usa `entes.cpp`)

```
b = dirección alineada a 16
u32(b+248) == 0x18
u32(b+312) en {1,2}  y  u32(b+492) == 0x80000 | u32(b+312)
float4(b+208) = (x,y,z,w): w == 1.0, 0 < y < 3, |x| < 80, |z| < 60
```

### 4. Qué falta (y cómo se va a cerrar)

1. **Equipo de cada fichita.** El campo +312 NO es el equipo (en pruebas con 22 objetos daba 11/11, pero con la lista completa
   no). Plan: en el saque de centro (juego congelado) el equipo local ocupa un lado; fijar la pertenencia por *posición de
   formación al saque* y mantenerla por la identidad del objeto (las direcciones no cambian en el partido). El lado de cada
   equipo en cada tiempo sale del reloj (`match.stats`) y de quién saca.
2. **Balón.** Aún no identificado. Plan: con el balón quieto en el centro (saque inicial) buscar el triple (≈0, 0.11, ≈0) y
   comprobar qué direcciones se mueven a >5 m/s justo después (`bola.cpp`).
3. **Ruta estable.** Los punteros que llegan al array (pool genérico en 0x46xxxxxx) no cuelgan de una dirección fija del
   ejecutable; por eso se prefiere la firma.

### 5. Protección (regla de Fralex: «si algo se rompe, mejor no lo hagas»)

- Solo lectura. Sin escribir, sin hooks, sin inyectar.
- La lectura se activa solo si: el proceso es `PES2021.exe`, su **SHA-256** coincide con una lista conocida
  (hoy: `5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A`, 458 806 784 bytes — es el mismo exe del
  parche ConmeGOL y del SP2026), y la firma devuelve 22 jugadores plausibles (dentro del campo, finitos).
- Si algo no cuadra (otro ejecutable, 0 jugadores, valores fuera del campo, NaN), la función se apaga sola y el juego sigue normal.
- Coste: 1 escaneo de ~5 s al empezar el partido y después ≈ 20 lecturas/s de ~1 KB (despreciable).

### 6. Datos útiles para el futuro (extra)

- `PES2021.exe`: base de carga `0x140000000`, tamaño de imagen 462 999 552 bytes. Regiones RW del proceso en partido: ~3 200 MB.
- IDs de equipo en memoria = IDs PES (Barcelona 44, Man City 67 en ConmeGOL).
- El proceso **se pausa/minimiza solo** si pierde el foco (no corre con la ventana minimizada): hace falta que esté al frente.
- Teclado: en el menú funcionan Enter/flechas; en el campo el teclado no controla al equipo (solo el mando).
- Sider overlay: la barra espaciadora lo muestra/oculta.

### 7. Hallazgos del 11 oct por la noche (equipo y balón: aún sin cerrar)

Comprobado con un saque de centro congelado (captura + memoria en el mismo instante, 0:00, Barça–City):

- Cada lista de «equipo de IA» (códigos 44 y 67) trae **23 actores = 20 jugadores de campo + el árbitro + 2 asistentes**
  (los asistentes van pegados a la banda, z = ±35.4). Los **porteros NO están en esas listas**: son actores aparte
  (código distinto, `+312 = 1`, `+492 = 0x80001`, uno por portero; en el saque salió el del Barça en x = −47).
  Para tener los 22 hay que sumar los porteros.
- Posiciones verificadas contra la pantalla: el que saca (cursor «FRALEX») está en (0.6, 0.0); los demás caen donde se ven.
  En el saque inicial el local ocupa x < 0 (excepto quien saca).
- **El equipo NO está en el actor.** Probado: +312 (es la «lista», no el equipo), +228, +440 (lista), punteros +16/+24
  (enlaces entre actores), tabla de registros de +416 y contexto de +400. Ningún campo parte los 21 en dos equipos.
  Siguen abiertas: (a) fijar el equipo con el saque de centro (por lado y pidiendo al que saca) y mantenerlo por
  identidad de objeto, (b) seguir buscando dónde guarda PES la plantilla en pantalla.
- **Balón: candidatos, sin confirmar.** Hay actores «tipo 0» (`kind=0`, código 5/8/1045) con y ≈ 0.10–0.11 (el radio del
  balón) y varias copias idénticas; ejemplo (−20.0, 0.11, 6.1) en cuatro direcciones. Falta verlos moverse con el juego
  en marcha (cuando se probó, el juego estaba pausado por minimizado).
- Los actores de un partido anterior pueden quedar en memoria con códigos viejos; el lector se queda con la lista que tiene 18–24 miembros.
- Mecánica útil: tecla **Z** = «Atrás» y **X** = «Confirmar» en los menús del juego; Enter/flechas también.
  Con X se saca de centro. El juego **se pausa solo si se minimiza** y a veces el Explorador pasa al frente.
- Penales: en Opciones gener. (antes del partido) «Pen.» y «Prórroga» están en «Sí», así que con un 0–0 y 5 min se llega a una tanda
  (el balón queda quieto en el punto penal = referencia perfecta para el balón); no se llegó a probar.
- Herramientas nuevas en `tools/radar/`: `vigilante.cpp` (congela el juego al detectar un saque), `entes3.cpp` (firma relajada: porteros, árbitro, balón).

### 8. 11 oct por la noche (segunda tanda): balón, porteros, equipo y TANDA DE PENALES — cerrado

Partido Barça–City en ventana (0:00 → 1-1 → tanda de penales), con captura congelada del saque y 3 minutos de grabación
de la tanda a 20 lecturas/s (`tools/radar/sondaL.cpp`, capturas cada 4 s).

- **Balón del partido = un actor fijo todo el partido**: `+248 = 0`, `+312 = 1`, `+492 = 0x10001`, posición en +208 con
  `y ≈ 0.11` (radio) y hasta ~1 m al golpearlo. Se **aparca en z = 100** durante las repeticiones y vuelve a (0, 0.11, 0)
  en el saque. Hay otros balones (`code` 6 y 9, de los recogepelotas) con la misma forma: el del partido es **el que se mueve**.
- **Porteros**: actores aparte de las listas (`+248 = 0x18`, `+312 = 1`, `+492 = 0x80001` o `0x10001`), dos copias por
  portero; en el saque están en x = ±47.
- **Equipo**: no está en el actor; se reparte en el saque de centro (izquierda → A, derecha → B; los del círculo central
  → el equipo que saca = el que tiene menos jugadores en su mitad) y se recuerda por dirección. Comprobado con la foto del
  saque: los rivales se pegan a la línea de medio campo (x ≈ 1–2 m), por eso «centro» es solo el círculo del balón (< 2.5 m).
- **Árbitro**: en el saque se coloca clavado en (±4.8, ±10.0) (dos partidos, misma décima). Un rival puede quedar cerca
  ((4.0, −9.3)): tolerancia de 0.7 m en total y, si hay varios, el más exacto.
- **Tras el pitido / en la tanda** `+312` de los jugadores pasa de 2 a **0** (y el de los porteros de 1 a 0): la firma lo admite.
  Los que esperan se colocan en el círculo central o **fuera de la banda (z ≈ 40–45)**: el límite de posición válida sube a |z| ≤ 50.
- **Tanda de penales** (4 lanzamientos grabados, todos gol): el balón se queda en el punto penal (−41.6, 0.11, 0.0); el pateador
  es un actor normal que camina desde el círculo (su identidad se mantiene → su equipo); el portero que ataja es el que está
  en la línea (|x| > 50, |z| < 4) y los dos se alternan; al golpear, el balón cruza x ≤ −52.5 con |z| < 3.66 y y < 2.44 (gol)
  y vuelve al punto para el siguiente. En Link: `core/CampoVivo.h` clase `Tanda` (probada con un penal real en `campo_test`).
- **Nombres de los lanzadores**: aún no (no hay nombre ni dorsal reconocible en el actor; la tabla de registros de +416 no
  lo separa). Pendiente.

### 9. 11 oct (noche tardía): identidad de cada jugador y estadísticas — lo que se probó y las pistas abiertas

**Objetivo pedido**: al hacer clic en una fichita, ver su dorsal y nombre; y contar disparos a puerta, córners, faltas, tarjetas, cambios (quién entra/sale), asistencias, etc.

Lo que NO funcionó (para no repetirlo):
- **Campos del actor**: ninguno de los 640 bytes distingue equipo ni jugador (probado con 3 saques/2 listas, con partición por lado conocida). Los punteros `+16/+24` son enlaces entre actores (marcaje/vecinos), no identidad.
- **Quién apunta al actor** (`tools/radar/identidad.cpp`): ~500 punteros por 42 actores; las ventanas de ±0x100 bytes alrededor traen tablas de IA con enteros pequeños (formación/zonas), ningún id de jugador distinto por actor.
- **Registros de jugador con nombre** (`buscatexto.exe`): hay estructuras con **dorsal en +0x8C, nombre en +0x90** y el id de jugador en +0x08/+0x30 (p. ej. LAMINE YAMAL, dorsal 99, id 162114 = el `player_id` que da Sider en `goal_scored`), pero están en pools estáticos de la pantalla de alineación/intro y **no tienen puntero al actor** ni cerca de él. También hay una caché de textos de nombres (64 bytes por registro) usada para dibujar.
- **Estadísticas del equipo** (las del menú de pausa): buscadas por valores exactos (posesión, tiros, pases…) como u8/u16/u32 (`buscaserie.exe`) sin éxito: el juego no las guarda como enteros planos contiguos (o las calcula al mostrarlas).

Pistas abiertas (en orden de probabilidad):
1. **Sider `goal_scored`** entrega `player_id`, `player_offset` y un puntero `player_info` (visto en las cadenas de `sider.dll`): `player_offset` parece el índice del jugador dentro de su equipo. Un módulo Lua que registre ese evento y escriba `player_offset` junto al gol dejaría ver el hueco de la plantilla; con la plantilla (Phoenix-DB / `PlayerAssignment`) saldría el nombre. No sirve para los 22 en general.
2. **Eventos por nombre de escena**: ConmeGOL ya intercepta `livecpk_get_filepath` para escenas de falta (`common\demo\fixdemo\foul\cut_data\foul_pk_card_y01.fdc` = amarilla, `..._r01` = roja, `foul_beJudged` = falta…). Un módulo observador que **solo registre** los nombres de los `.fdc` que el juego pide permitiría contar faltas, tarjetas, lesiones y cambios por los nombres de escena (hay que listarlos en un partido real). Requiere activar un módulo en `sider.ini` (lo hace el instalador de módulos de Link, con PES cerrado).
3. **Identidad por geometría en el saque**: emparejar cada actor con el jugador de la alineación por su posición base de formación (la alineación se ve en la pantalla de «Estrategia»); necesita la formación del equipo y el orden de la plantilla.
4. **Estadísticas derivadas de lo que ya se lee** (posiciones + balón): posesión por cercanía, distancia recorrida, velocidad máxima, tiros por velocidad/dirección del balón, saques de banda/esquina por salida del balón. Se pueden construir y validar contra la pantalla de estadísticas finales (ya hay un partido de referencia: Barça 7-0 City, posesión 56/44, tiros 12(8)/0(0), córners 1/0, faltas 2/0).

#### 9.1 Estadísticas del juego: referencia capturada y lo hallado en memoria (11 oct, partido Barça 7-0 City, Alevín, 6 min)

Pantalla «Final del partido» (equipo, local / visita): posesión 56 % / 44 %; tiros (a puerta) 12 (8) / 0 (0); faltas (fuera de juego) 2 (2) / 0 (0); córners 1 / 0; tiros libres 0 / 2; pases (completados) 56 (51) / 44 (37); centros 1 / 0; pases interceptados 5 / 3; entradas 8 / 1; paradas 0 / 1.
Pantalla «Datos individuales del jugador» (por jugador): goles, goles de penalti, goles de tiro libre, asistencias, tiros (a puerta), pases (completados), centros, faltas (fuera de juego), tiros libres, córners, pases interceptados, minutos jugados, balones recibidos, distancia regateando, velocidad media (km/h), entradas (ganadas), despejes; portero: tiros recibidos, paradas, zona de juego. Y la «Valoración de jugadores» (5.0–7.5) de los 11 titulares.
Ejemplo para validar: Joan García (portero): pases 1 (1), minutos 90, balones recibidos 1, velocidad media 7.2 km/h; valoraciones del Barça en orden de alineación: 5.0, 5.0, 6.0, 6.0, 5.0, 6.0, 6.0, 6.0, 7.0, 7.0, 7.5.

**Hallado**: la lista de valoraciones de la pantalla (filas de 64 bytes: `+0` valoración float, `+4` nombre UTF-8 hasta 20 bytes, `+0x24` índice del hueco) vive mientras se ve la pantalla (p. ej. `0x757865E4`, 11 filas = alineación en orden, útil para saber **quién es quién en la alineación**). Es la fila de la interfaz, no el almacén de estadísticas.
**No hallado todavía**: el almacén por jugador (goles, pases, minutos, velocidad media, distancia…). Búsquedas por valores (minutos = 90 + velocidad 7.2/2.0 m/s) dan demasiado ruido. Método propuesto: **diferencia de memoria antes/después de un evento conocido** (un pase, un tiro, un córner) con el juego congelado, filtrando por «sube exactamente 1» como en el escaneo de posiciones; con la pantalla individual de un jugador abierta, comparar contra lo que muestra.

---

## Parte 3 · Campo 2D en vivo (Partido › Campo)

> Origen: `docs/CAMPO-2D.md` · último cambio 547ddf1 2026-10-14 · 29856 bytes. Copia íntegra.

Muestra en Phoenix Link, en tiempo real, las fichitas de los jugadores de PES 2021 sobre un campo 2D, como el radar del modo entrenador.
Cómo se encontró todo (fuentes, estructura de memoria, pruebas): [CAMPO-2D-INVESTIGACION.md](CAMPO-2D-INVESTIGACION.md).
Para replicarlo paso a paso (humano o IA): [CAMPO-2D-REPLICAR.md](CAMPO-2D-REPLICAR.md).

### Cómo se usa

1. Abre **Partido › CAMPO** en Link.
2. Abre PES (el modo de ConmeGOL sirve) y empieza el partido.
3. Tarda unos 15 s en «encontrar» a los jugadores la primera vez (barre la memoria del juego una vez) y luego se mueven en vivo, ~20 veces por segundo.

Colores: cian = local, violeta = visitante, amarillo = asistentes de línea, gris = equipo aún desconocido. El balón es la bolita blanca.

### Qué hace Link (y qué NO)

- **Solo lee.** Abre PES2021.exe con permiso de lectura (`PROCESS_VM_READ`) y nada más: no escribe, no inyecta, no hookea, no pausa el juego, no puede desincronizar un partido.
- **Solo funciona con el ejecutable conocido.** Antes de leer comprueba tamaño y SHA-256 de `PES2021.exe` (hoy: 458 806 784 bytes, `5E27A782…BC224A`, el del parche ConmeGOL y SP2026). Con otro ejecutable la tarjeta dice «no compatible» y no toca nada.
- **Se apaga sola** si lo que lee no tiene sentido (jugadores fuera del campo, valores raros, más de 3 jugadores que dejan de ser válidos): vuelve a buscar con calma o se queda esperando.
- **Coste cero fuera de la tarjeta.** Con la tarjeta cerrada no hay hilo leyendo ni evento alguno. Con la tarjeta abierta: ~20 lecturas por segundo de 16 bytes por jugador.
- Si PES no está abierto, la tarjeta dice «Esperando a que empiece un partido en PES».

### Piezas

| Pieza | Dónde |
|---|---|
| Firma y validación (lógica pura, con pruebas) | `SmashSoda/phoenix/core/CampoVivo.{h,cpp}` · `pruebas-link/campo_test.cpp` |
| Lector del juego (hilo propio, Windows) | `SmashSoda/phoenix/host/CampoVivo.{h,cpp}` |
| Acción `campo.vivo`, evento `campo`, estado `campo` | `web/AccionesMandosWeb.cpp`, `web/InterfazWeb.cpp`, `web/EstadoWeb.cpp`, `web/PROTOCOLO.md` |
| Vista (SVG, interpolación suave) | `web/ui/js/vistas/campo.js` (pestaña CAMPO de Partido) |
| Simulador | `web/ui/js/simulador.js` (22 fichitas + balón + 2 asistentes en movimiento) |
| Prueba manual con el juego abierto | `pruebas-link/campo_vivo_real.cpp` |

### Pendiente

- **Equipo de cada fichita** (hoy salen en gris salvo los asistentes): se fija con el saque de centro.
- **Balón** (hoy la bolita no se dibuja con datos reales; en el simulador sí).
- ~~Mini-radar en el overlay~~ (descartado por Fralex: el campo va a la web).

### 0.2.21 — equipos, porteros, balón y tanda de penales

- **Colores por equipo**: A (cian) = el que ocupaba la izquierda en el primer saque de centro que vio Link; B (violeta); árbitros en amarillo. Como en el descanso se cambian los lados, A/B se mantienen por identidad (no por lado). Si abres Link a mitad de un tiempo, las fichitas van en gris hasta el siguiente saque (gol o descanso).
- **Balón**: la bolita blanca; se agranda un poco cuando va por el aire. Desaparece durante las repeticiones (el juego la aparca).
- **Porteros**: ya cuentan (24 fichitas: 22 + 2 árbitros… y el árbitro principal cuando el saque lo identifica).
- **Tanda de penales**: debajo del campo aparece la tanda en vivo: una fila por equipo con sus lanzamientos (✓ gol, ✗ fallo o parada), el marcador de la tanda y «A punto de patear: Equipo A/B». Los nombres de los lanzadores todavía no se leen del juego (pendiente).

### 0.2.22 — mini-radar en el overlay (RETIRADO: ver la sección siguiente; el campo va a la web)

- En **Partido › CAMPO** hay un interruptor «Mini-radar en el overlay». Encendido, Link lee el juego aunque esa tarjeta esté cerrada y manda el campo al overlay (WebSocket, evento `phoenix:campo`, ~10/s).
- En el **overlay › Widgets › Mini-radar** se enciende el widget y se elige la esquina (apagado por defecto). Se puede mover/redimensionar como los demás. Si dejan de llegar datos (juego cerrado, repetición larga) se esconde solo a los 4 s.
- Coste: lo mismo que la tarjeta (≈ 20 lecturas/s de 16 bytes por actor) y solo mientras el interruptor esté encendido.
- Código: overlay `frontend/src/components/overlay/widgets/OverlayRadarWidgetView.vue` + `models/widgets/RadarWidget.ts` (commit `2f740db` en `phoenix-overlay`, sin push); Link `web/InterfazWeb.cpp` (envío) y `PhoenixPrefs.radarOverlay`.

### 2026-10-11 · El campo va a la web, no al overlay

Fralex: «en el overlay no tiene que aparecer el campo 2D; debe aparecer en la web». El mini-radar del overlay (0.2.22, `campo.radarOverlay`) se quitó. Su interruptor ahora es **«Campo en vivo en la web»** (`campo.web`, preferencia `campoWeb`, apagada por defecto). Con un partido publicado en juego, `web/CampoWeb.cpp` manda el campo y la posesión a `POST /v1/liga/campo` como mucho 1/s, y la web lo enseña en el tablero que se abre al tocar «En vivo ahora» (`docs/PROMPT-WEB-tablero-partido.md`). El overlay ya no recibe `phoenix:campo`.

### 0.2.29 · Repeticiones, cinemáticas y pausas (fase del campo)

Problema (Fralex, repetición de un gol): las fichitas desaparecían. Causa: durante la repetición el juego aparca el balón (z = 100) y las posiciones que se leen son de la película, y si dejaban de leerse el lector se daba por perdido y volvía a buscar (~20 s «sin señal»).

- El lector clasifica cada lectura en una **fase** (`FotoCampo.fase`, evento `campo` campo `f`, solo si ≠ vivo; en `POST /v1/liga/campo` siempre como `fase`):
  - `vivo`: juego en directo.
  - `repeticion`: el balón lleva ≥ 0,5 s aparcado (|z| > 40 en todos los candidatos). Se **congelan las últimas posiciones buenas**, el balón se oculta, no se asignan equipos ni se cuenta posesión ni se alimenta la tanda. Vuelve a `vivo` en la primera lectura con balón en juego (< 0,1 s; si hay salto grande las fichitas se colocan sin deslizar).
  - `otro`: más de 3 fichitas dejaron de leerse; se congela igual y se reintenta hasta 30 s antes de volver a escanear (antes: escaneo inmediato).
  - `pausa`: todo quieto ≥ 4 s; solo informativo (posiciones reales).
- Tarjeta Campo: etiqueta sobre el campo «REPETICIÓN», «SIN LECTURA EN VIVO» o «PAUSA». El simulador muestra «REPETICIÓN» 4 s cada 32 s.
- Web: el tablero debe hacer lo mismo con `fase` (≠ `vivo`: congelar y mostrar la etiqueta). La posesión no se calcula fuera de `vivo`.
- Lados automáticos no reciben lecturas de `repeticion`/`otro`.
- La celebración de gol no tiene fase propia: si el balón sigue en juego (|z| ≤ 40) se ve como `vivo` con las posiciones reales; si el juego lo aparca, como `repeticion`. No verificado con partido real (pendiente probar con una repetición y celebración en vivo).

### 0.2.30 · Colores de camiseta por equipo

Fralex: «adapta el color de cada equipo según su color referente». Cada equipo se pinta con el color de su camiseta principal, sacado del **nombre del equipo que da el juego** (`phoenix_referee`: `nombreLocal`/`nombreVisita`).
- Tabla en `core/ColoresClub.{h,cpp}` (≈100 entradas: selecciones de CONMEBOL y grandes, clubes peruanos, sudamericanos y europeos; color principal + secundario; busca por trozo del nombre sin tildes, y por palabra entera si la clave es corta: «usa», «roma», «inter»). Prueba: `pruebas-link/colores_test.cpp`.
- Reglas: un color demasiado oscuro (negro, azul marino) se sustituye por el secundario o por gris azulado; si los dos equipos quedan parecidos (distancia RGB < 110) el segundo usa su secundario y, si sigue parecido, el violeta/cian/amarillo más lejano; nombre desconocido = cian (A) / violeta (B) de siempre. Árbitros siguen amarillos.
- **A = local del juego** (se asume; si Link detectara que A es en realidad el visitante, los colores irían cruzados: es el mismo asunto abierto que «lados» y «eq_local»).
- Protocolo: el evento `campo` y `POST /v1/liga/campo` traen `c:["#rrggbb","#rrggbb"]` (A, B) solo si al menos un nombre se reconoce. Tarjeta de Link: fichitas y leyenda con esos colores. Web: Edge `liga` 1.3.1 (v5) deja pasar `c` y el tablero (`features/liga/campoVivo.js`) los usa (repo `pes-liga-automatica`, commit `7f2b9c2`).
- Para añadir un club: una línea en `kTabla` (clave normalizada en minúsculas sin tildes, principal, secundario) y un caso en la prueba.

### 0.2.31 · «Viendo desde la web» en la sala

Fralex: al tocar un partido en la web se ve el tablero (y las estadísticas) sin entrar a la sala; quien quiera puede entrar; y en la sala debe verse quién mira desde la web.
- **Web** (repo `pes-liga-automatica`, ronda 237): el tablero de «En vivo» / «Ahora mismo» / Inicio tiene el botón **Entrar a la sala** (lee `salas_enlace`, que la base solo da a quien puede ver esa sala) y un panel de estadísticas. Con sesión, avisa cada ~30 s que alguien mira (tabla `liga_mirando`, migración 104; el nombre sale del perfil en el servidor).
- **Link:** el hilo de la liga pregunta cada ~10 s `GET /v1/liga/mirando?sala_id=` (Edge `liga` 1.4.0), solo con el marcador de la sala publicado. Resultado: (1) lista «Viendo desde la web» en **Gente**, (2) un mensaje del bot en el chat de la sala por persona («X está viendo el partido desde la web.»), que se vuelve a decir si se va y vuelve. Código: `link/PhoenixLink.cpp` (`mirandoWeb()`), `web/InterfazWeb.cpp`, `web/EstadoWeb.cpp` (`mirandoWeb`), `ui/js/vistas/gente.js`.
- Límites: los visitantes sin sesión no aparecen (no hay nombre que mostrar). Los espectadores de la web no entran al Parsec salvo que pulsen «Entrar a la sala».

### 0.2.37 · Poseedor del balón y retraso del campo (medición y arreglos)

Fralex: «a veces no aparece marcado el jugador que tiene el balón» y «a veces se tarda». Método: medir cada tramo, atacar el que más aporta, probar la lógica con pruebas puras y verificar con un partido real.

#### Tramos del retraso y qué se encontró (por lectura del código; las cifras reales se rellenan con el diagnóstico)
| Tramo | Antes | Ahora |
|---|---|---|
| Lector de memoria | `dormir(50)` en rebanadas de 25 ms con el temporizador de Windows a 15,6 ms → cada pasada de ≈ 62 ms (≈ 16 lecturas/s) con tirones; `reloj` sumaba 0,05 fijo | `timeBeginPeriod(1)` + horario fijo de 33 ms (≈ 30 lecturas/s) y reloj real |
| Publicación a la interfaz de Link | evento ≤ 20/s, atado al fotograma de Link | evento ≤ 30/s |
| Suavizado en la interfaz | 70 ms | 45 ms |
| Envío a la web | 1 mensaje/s, y cada envío pasaba por 3 consultas en la Edge (≈ 100–300 ms) | 1/s sin espectadores con sesión, **4/s si alguien mira** (liga_mirando); la Edge guarda en caché la PC (60 s) y la sala (30 s) |
| Suavizado en el navegador | 0,3 s fijo | ≈ medio intervalo de llegada (60–500 ms) |

El tramo que más aportaba era el **envío a la web a 1/s** (el tablero siempre iba ≥ 1 s por detrás por diseño). Un envío a 4/s lo deja en ≈ 0,25–0,4 s + red.
Coste: cada mensaje es una invocación de la función (plan FREE ≈ 500 000 al mes). Por eso solo se sube a 4/s mientras alguien mira con sesión; un visitante sin sesión ve 1/s.

#### Diagnóstico visible
- **Link › Partido › CAMPO** (bajo el campo): lecturas del juego por segundo y duración de la pasada (media y máxima = tirones del lector), retraso del campo hasta la interfaz (hora de la lectura en el evento `ts` frente al reloj de la interfaz; mismo PC, así que es exacto), eventos/s, envíos a la web por segundo, ms por envío, fallos y cuántos miran.
- **Web:** añadir `?diag` a la dirección del tablero enseña mensajes/s y retraso desde la lectura (`ts`; solo es exacto si el visor y el host tienen la hora bien puesta).

#### Poseedor del balón (`core/CampoVivo` → clase `Poseedor`)
La heurística vieja (el más cercano a ≤ 2 m, calculada solo para la posesión de la web) no marcaba nada en el campo y fallaba con balón alto, pases y disputas. Reglas nuevas, con histéresis:
- Lo **adquiere** el más cercano a ≤ 1,4 m, con el balón a ≤ 2,4 m de altura y sin ir lanzado (> 13 m/s), sostenido ≥ 80 ms.
- Lo **conserva** mientras esté a ≤ 2,8 m; si se aleja, el balón sube o sale disparado, lo sostiene 350 ms y luego el balón queda **libre** (-1).
- **Disputa:** otro se lo quita solo si está a ≤ 1,0 m y el poseedor ya está a más de 1,8 m. Dos casi empatados no parpadean.
- Sin balón visible (repetición/pausa) conserva 350 ms y luego libre. Árbitros y asistentes no cuentan.
- Se dibuja un **halo blanco** sobre el poseedor en Link y en la web; el protocolo lleva `p` (índice en `j`; -1 = libre; ausente = Link viejo). La posesión % de la web usa ahora al poseedor.
- Casos cubiertos por `pruebas-link/campo_test.cpp` (sintéticos): regate sin parpadeo, pase y recepción, balón por el aire, balón libre, disputa con histéresis, repetición, árbitros, cambio de número de fichas.
- **No resuelto con datos reales todavía:** si PES guarda en memoria quién controla el balón (un puntero o bandera), sería mejor que la heurística; se busca con un partido real (ver abajo). Mientras tanto, el estado «libre» y el halo salen de la geometría.

#### Pendiente de verificar con un partido real (3–5 min contra la CPU)
Comparar captura del juego frente al tablero (balón en el aire, saques, córners, repeticiones, pausa y vuelta al juego) y rellenar las cifras de retraso medidas.

### 0.2.38 · Porteros y estadísticas del partido

#### Porteros
Fralex: «no me reconocía los porteros». Los volcados reales (`ko_entes.txt`, `entes_raw.txt`) enseñan que el juego guarda **dos copias de cada portero** (a 640 bytes una de otra, `+312 = 1`, `+492 = 0x80001`) y que, hasta ahora, Link cogía la de menor dirección y un solo portero por lado. Si esa copia es la que se queda quieta en el sitio del saque, el portero salía parado (o no salía). Ahora, por cada lado, Link lee **todas las copias** y se queda con la que **se mueve** (cambia solo si la elegida lleva > 1 s quieta y la otra se movió hace < 0,4 s: sin parpadeo). También escribe `phoenix-campo-escaneo.txt` en la carpeta temporal (jugadores, copias de porteros con su posición y dirección) para diagnosticar sin volver a abrir el juego, y la tarjeta CAMPO enseña «porteros: N/2 (X copias)».
Aún **no verificado con un partido real**: los volcados antiguos solo traen las copias del portero izquierdo; el derecho no aparece en ellos. Si con partido real sale «porteros 1/2», pásame `phoenix-campo-escaneo.txt`.

#### Estadísticas derivadas (`core/CampoVivo` → `Estadisticas`)
El juego no deja leer sus estadísticas (la pantalla «Datos del jugador» solo existe al final del partido y no se encontró su almacén en memoria). Se calculan con las posiciones y el poseedor del balón; son **aproximadas** y se rotulan así:
| Estadística | Cómo se calcula |
|---|---|
| Posesión | segundos con el balón por equipo |
| Pases completados | el balón pasa a otro jugador del mismo equipo en ≤ 3 s |
| Pérdidas / recuperaciones | pasa al otro equipo en ≤ 3 s |
| Tiros / a puerta | balón lanzado > 14 m/s hacia la portería rival, a ≤ 32 m, que apunta a |z| < 7 (a puerta: < 4,2 al llegar a la línea); un pase largo muy tenso hacia el área puede contarse como tiro |
| Córners / saques de puerta | el balón sale por la línea de fondo (no es gol): si lo tocó el defensor, córner del rival; si lo tocó el atacante, saque de puerta |
| Saques de banda | sale por la banda: saque para el equipo que no lo tocó |
El lado que defiende A se saca de dónde está su portero (cambia en el descanso). Pruebas sintéticas en `pruebas-link/campo_test.cpp` (posesión por tiempo, pase, pérdida, tiro/desvío, córner, saque de puerta, banda, gol, lado desconocido).
**No se pueden leer todavía:** faltas, tarjetas amarillas y rojas, cambios (quién entra y quién sale), asistencias, fueras de juego. Opciones: (a) un módulo de Sider que observe los archivos de escena que carga el juego (`foul_pk_card_y01`…) — requiere tocar `sider.ini` con PES cerrado, decisión de Fralex; (b) buscar el almacén de estadísticas en memoria comparando antes/después de un evento con el juego congelado (necesita partido real).
Protocolo: el evento `campo` y `POST /v1/liga/campo` traen `est:{pos,pas,per,rec,tir,pue,cor,ban,met}` (pares [A,B]); la web lo pinta en una tabla bajo el campo (Edge `liga` 1.6.0; repo web, commit `456d58e`).

### 0.2.39 · Prueba de las dos vías para faltas, tarjetas y cambios

Fralex: «probemos ambas cosas para encontrar lo más eficiente». Hallazgos antes de probar (leyendo la instalación de ConmeGOL, solo lectura):
- Sider es la 7.3.4. El parche trae `modules\FoulScenes.lua`, que engancha el **selector de escenas** del juego y recibe en cada escena `RCX` = código de escena y `RBX` = lado (0 = visita, 1 = local). Códigos que usa: `0x0002000F` amarilla, `0x00020010` roja, `0x00020011`/`0x00020014` falta sin tarjeta, `0x00020012` aviso, `0x00020013` roja (alt.), `0x00050001` cambio real, `0x00050006` cambio (VAR). Además el juego pide por nombre los archivos de escena (`foul_pk_card_y01.fdc`, `foul_pk_card_r01.fdc`…) y `FoulScenes.lua` los intercepta con `livecpk_rewrite`.
- `FoulScenes.lua` solo está activo en el sider.ini del modo **ConmeGOL Patch 26**, no en el 27 (el modo actual de Fralex).

Dos vías, ambas **solo lectura** y metidas en `phoenix_referee.lua` v1.3 (el módulo que Link ya instala; no se toca sider.ini):
| Vía | Qué anota | Alcance |
|---|---|---|
| **A · nombres de escena** (`livecpk_rewrite`) | cada archivo «demo/foul/card/corner/throw/offside/change/subst/injur/penal/kick/var_» que el juego pide, con la hora | cualquier parche con Sider 7.3.x (también el modo 27) |
| **C · selector** (evento `custom:pk_var_supersub_redirect`) | código de escena + lado (RBX) | solo si `FoulScenes.lua` está activo (modo 26) |
Ambas escriben `<juego>\SiderAddons\content\phoenix\escenas.log` (`hora|archivo` o `hora|SEL|0xCÓDIGO|rsi=..|rbx=..`), se vacía en cada partido y se detiene a las 4000 líneas.
**Cómo se prueba:** (1) cerrar PES; (2) en Link, SYNC › Puente › Módulos del juego → actualizar (copia el nuevo `phoenix_referee.lua` en la raíz y en cada modo, con copia de seguridad); (3) jugar 5 min contra la CPU provocando faltas, una amarilla, una roja, un cambio, un córner, un saque de banda y un fuera de juego, anotando la hora de cada uno; (4) leer `escenas.log` y cruzar con las horas anotadas.
**Cómo se decide la más eficiente:** la que dé nombre de evento fiable para los 3 casos (amarilla/roja/cambio) con menos falsos positivos, sin leer memoria y en todos los modos. Si A solo da los archivos pero no el lado, el equipo se infiere con la posesión (el que cometió la falta suele ser el que no llevaba el balón); si C da el lado, se usa C donde exista.
Vía de memoria (buscar el almacén de estadísticas comparando antes/después) queda como tercera opción, solo si ninguna de las dos basta: necesita partido real y comparar volcados de varios GB.

### 0.2.40 · Equipos que juegan, color de club y dorsales

- **Quién contra quién:** la tarjeta CAMPO pone «FC BARCELONA VS BAYERN…» (nombres que da el juego en `estado.json`; A = local) y la leyenda usa esos nombres. El color de cada equipo ya salía de la tabla de camisetas (0.2.30).
- **Dorsal en cada ficha: NO se puede todavía.** Cada actor del campo (objeto de 640 bytes) no trae su dorsal ni su jugador: se comprobó con los volcados reales (`ko_entes.txt`): en los 22 actores de una lista, ningún campo es un entero 1..99 distinto por jugador; solo hay posiciones, punteros y banderas. Por eso las fichas no pueden llevar número ni nombre con lo que se lee hoy.
- **Herramienta para encontrarlo** (`tools/radar/cadena.cpp`, solo lectura): escanea como Link, toma los 22 actores y sigue los punteros de cada uno hasta 2 niveles buscando (a) enteros 1..99 distintos entre jugadores (dorsal) y (b) textos de ≥ 4 letras distintos (nombre). Imprime las rutas «+off1 → +off2» que mejor separan a los jugadores. Se ejecuta con el partido en juego (`cad\cadena.exe`) y no pausa ni escribe nada.
- **Plan B si la herramienta no encuentra nada:** inferirlo por formación. Al saque de centro cada jugador está en su casilla de la formación; la alineación (orden de titulares) y los dorsales salen de los datos del equipo (Phoenix-DB / Player.bin con los ids de equipo `local`/`visita` de `estado.json`). Hay que casar casillas con posiciones, y los cambios mueven los dorsales: solo servirá con la alineación inicial y los cambios que se lean con `escenas.log`.

### 0.2.76 · Cartel de repetición/pausa por el reloj del juego (y medición de movimiento)

Fralex: «que muestre cuando es repetición y cuando está en pausa» y «hay inconsistencias de movimiento y posición exacta».
- Antes la pausa solo se notaba si **todas** las fichas estaban casi quietas (< 2 cm en 2,7 s), pero en pausa el juego sigue moviendo animaciones y nunca se cumplía; la repetición solo se detectaba si el balón estaba aparcado.
- Ahora Link también usa el **reloj del juego** (`estado.json`: `relojCorre`, `fase`): si el reloj lleva > 0,8 s parado con partido en curso, el lector **congela las últimas posiciones en vivo** y publica la fase `detenido` («REPETICIÓN O PAUSA»), o `descanso` («DESCANSO»). Al volver a correr el reloj, vuelve solo a vivo. Sin datos frescos del juego (estado.json > 5 s) no se congela nada. Protocolo y web: Edge `liga` 1.6.1 y tablero (commit web `005b16d`).
- **Medición** (`tools/radar/medir.cpp`, solo lectura): con el partido en juego mide el ritmo real de cambio de posición de cada actor, saltos imposibles y cuánto se parecen/desfasan las dos listas de jugadores del juego (en los volcados viejos difieren ≈ 0,5 m). Con esas cifras se decide si Link debe leer la otra lista, promediar o suavizar. **Pendiente de un partido real.**
- Los porteros siguen pendientes de partido real (ver 0.2.38: «porteros N/2» en el diagnóstico).

### 0.2.86 · Medición con partido real: ritmo del juego, dos copias por jugador y porteros

Mediciones con PES2021 en un partido real (14-oct-2026, solo lectura, herramientas `tools/radar/medir.cpp` y `medir_balon.cpp`):
- **Ritmo del juego:** cada ficha cambia de posición ≈ 75–140 veces por segundo (mediana 7 ms entre cambios, hueco máximo 16 ms), sin saltos imposibles. El lector de Link a ~30/s va muy por debajo del ritmo del juego: no es el cuello.
- **Cada jugador existe DOS veces** (una por lista de equipo de IA: códigos 44 y 67; objetos contiguos a ±0x280 bytes). Las dos copias difieren **≈ 0,6 m** de media, sin dirección fija (dx −0,06 / dz −0,12 m, desviación ≈ 0,44 m) y sin desfase en el tiempo (proyectado sobre la carrera: 0,04 m; mejor desfase ≈ −35…−10 ms).
- **Ninguna copia es la buena:** en dos mediciones de 40 s la lista 44 estaba más cerca del balón el 56 % y la 67 el 58 % de las veces. Con el balón a < 2 m del jugador más cercano (≈ 3400 momentos) los percentiles 10/25/50 de la distancia fueron: lista 44 = 0,56/0,92/1,58 m · lista 67 = 0,45/0,71/1,70 m · **promedio de las dos = 0,50/0,69/1,57 m** (tan bueno como la mejor copia en cada tramo).
- **Cambio en Link (0.2.86):** cada ficha se lee en sus dos copias y se promedia (si las copias se separan más de 3 m se usa solo la primera). Emparejado por dirección contigua + cercanía en el escaneo.
- **Porteros:** hay 3 copias de portero vivas (dos gemelas pegadas, 369/378, y una tercera, 370); en una de las mediciones las tres estaban en la portería izquierda, y no se encontró el portero del otro lado con la firma actual. Como además hay actores con la misma firma que NO son porteros (medio campo, con el partido cargado), 0.2.77 solo acepta copias cerca de portería (|x| ≥ 30, |z| ≤ 22). **Sin resolver:** por qué falta una copia de portero y si los porteros cambian de actor en el descanso. Pendiente: medir con `medir.exe` cada segundo (posición de cada copia + balón) durante una jugada de portería y el descanso.

### 0.2.87 · Actores de partidos anteriores y porteros con otra marca (medido con partido real, 14-oct)

Con el partido en juego se vio que **no reconocer a los jugadores / los porteros** tenía dos causas:
1. **El juego deja en memoria los actores de los partidos ANTERIORES**, con la misma firma y quietos. Tras varios partidos había 59 actores de campo (en vez de 46): las dos listas salían con 30 y 29 miembros, y `elegirLista` (que espera 18–24) no elegía ninguna → «buscando» para siempre. Arreglo: tras escanear, Link observa hasta 30 s y se queda **solo con los actores que se mueven** (si no se mueven ≥ 18, el partido aún no corre y espera con el motivo «esperando a que el partido se mueva»).
2. **El portero de un lado llevaba la marca `+492 = 0x10002`** (lado 1), distinta de las `0x80001`/`0x10001` que se aceptaban. Ahora se acepta también. Copias de porteros del partido medido: dos pares gemelos vivos (uno por portería; se mueven) y un par viejo quieto de otro partido (sin cambios). Por eso se elige por lado la copia que se mueve (0.2.38).
Pruebas: caso nuevo en `pruebas-link/campo_test.cpp` (0x10002 sí, 0x10003 no). Pendiente: promediar también las dos copias de cada portero (hoy solo se promedian los jugadores de campo) y confirmar en un partido nuevo con el `.exe`.

### Análisis de una tanda real (14-oct-2026, Arsenal 3-3 Barcelona, `tools/radar/tanda_log.cpp`)

Registro cada 0,25 s de la portería izquierda durante 3 lanzamientos (solo lectura):
- **Preparado:** balón en el punto penal `(−41,6; 0,0; y = 0,11)`; el pateador, el actor más cercano, a 0,7 m `(−42,2; −0,4)`; todo lo demás quieto (`mov = 0`). El portero vivo (pareja de copias gemelas) en la línea `(≈ −52,3; ±0,5)`; la pareja del portero del otro equipo espera en la esquina del área `(≈ −52; −20,4)`.
- **Lanzamiento 1 (t ≈ 8 s):** balón `(−50,4; −7,2)` → `(−49,7; −13,9)` → `(−49,4; −17,2)`: **fuera por un lado** (nunca cruza la línea entre palos, |z| < 3,66). Después el balón **desaparece ≈ 5 s** (`balón = NO`: aparcado) y vuelve al punto para el siguiente.
- **Lanzamiento 2 (t ≈ 22 s):** `(−47,8; −7,0)` → `(−43,8; −15,8)` → `(−41,1; −22,0)`: atajado/desviado hacia fuera (el portero se tira: en la captura se ve tendido).
- **Lanzamiento 3 (t ≈ 27 s):** el balón vuelve al punto.
- Con esto la lógica de `Tanda` (balón quieto en el punto, pateador = más cercano, gol = cruza la línea entre palos, fallo = si no en 10 s o balón aparcado) es coherente con los datos reales. Falta comprobarla dentro de Link: el Link abierto durante esta medición (versión anterior a 0.2.87) mostraba el campo **vacío** en plena tanda porque había 58 actores (restos de partidos anteriores) y no elegía lista; con 0.2.87 se descartan los que no se mueven. Nota: en una tanda casi todo está quieto (`mov = 0`) y los actores solo se mueven al lanzar: la espera de 0.2.87 (≥ 18 actores moviéndose, hasta 30 s) puede NO cumplirse en una tanda ya empezada si Link arranca en ese momento. Pendiente de arreglar: aceptar también el estado «tanda» (balón en el punto con lado = 0).

### 0.2.89 · Luz del color del mando en el poseedor; prueba del minimapa de PES (opciones A y B)

**Luz por mando (0.2.89).** El halo de quien lleva el balón toma el color del mando que lo maneja (como el lightbar del DualShock: mando 1 azul, 2 rojo, 3 verde, 4 rosa, 5 naranja, 6 morado, 7 blanco, 8 celeste; 9 = mando físico del anfitrión; tabla `COLOR_MANDO` en `campo.js`). Además un anillo fino del mismo color marca cada fichita que maneja un mando (como el cursor de PES), y debajo del campo sale la leyenda «Luz del balón: ● Mando 1 …». Cómo se sabe qué mando maneja a quién: `core/DeteccionLados.cpp` → `Detector::controlados()` (correlación sostenida del stick con la carrera de cada fichita) alimentado por `web/LadosAuto.cpp` → `controlFoto()` (detector aparte, no depende de «lados automáticos»); el evento `campo` trae `m:[[ficha, asiento],…]`. Limitaciones: necesita equipos conocidos (saque de centro visto), unos segundos de stick apretado y un partido con mandos de Link; el color real del cursor en PES lo decide el juego (no se pudo leer), así que la paleta es la del DualShock y se puede cambiar. Si nadie maneja al poseedor (CPU), la luz es blanca. La web aún no recibe `m` (falta pasarlo por la Edge `liga`).

**Minimapa del juego (probado el 14-oct con partido real):**
- **A · buscar en memoria la lista de puntos del radar** (`tools/radar/radar_mem.cpp`: busca matrices de pares de float que cumplan u = a·x + b, v = c·z + d con las posiciones reales, 8 elementos, pasos de 8 a 64 bytes, en el orden de la lista A y de la B): **0 hallazgos** con partido en movimiento. Negativo: el radar no guarda sus puntos en ese orden/forma (o los dibuja directo desde los actores). No se descarta del todo (podría usar enteros o un orden por alineación).
- **B · leer el radar de la pantalla:** viable a ojo: el minimapa muestra los 22 puntos con color de equipo (rojo/gris-azul), los porteros en cada área y el balón en amarillo. Aún no implementado en Link (haría falta un fotograma de la ventana de PES y mapear cada punto a su actor por posición). Sería el respaldo para saber el equipo de cada ficha a mitad de partido sin esperar un saque.
- **Equipo de cada ficha:** se comprobó que ningún campo del actor lo trae (ninguna posición del objeto de 640 bytes separa a los equipos en un saque real). Sigue dependiendo del saque de centro; arrancar Link a mitad de partido deja las fichas en gris hasta el siguiente saque (gol o descanso).

---

## Parte 4 · Cómo replicar la investigación del campo 2D (para otro humano u otra IA)

> Origen: `docs/CAMPO-2D-REPLICAR.md` · último cambio 5da51b8 2026-10-16 · 7341 bytes. Copia íntegra.

Objetivo: encontrar en la memoria de `PES2021.exe` datos del partido en vivo (posiciones, y mañana otros) **sin escribir nada** en el juego.
Resultado ya obtenido: ver [CAMPO-2D-INVESTIGACION.md](CAMPO-2D-INVESTIGACION.md). Uso en Link: [CAMPO-2D.md](CAMPO-2D.md).

### 0. Requisitos y reglas

- Windows, Visual Studio Build Tools (usa `vcvars64.bat`), PES 2021 abierto con un partido contra la CPU.
- **Solo lectura.** `OpenProcess(PROCESS_VM_READ …)` y nada más. Pausar el proceso (`NtSuspendProcess`/`NtResumeProcess`) solo para investigar, siempre con reanudación garantizada; Link nunca lo hace.
- Los nombres de los `.exe` de prueba no pueden contener «instal» (Windows pide elevación).
- Mismo ejecutable de referencia: 458 806 784 bytes, SHA-256 `5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A`. Con otro ejecutable todo lo de abajo hay que volver a comprobarlo.
- El juego **se pausa si se minimiza**: déjalo al frente (o en ventana) todo el rato. Teclas en menús: X = confirmar, Z = atrás, Enter y flechas también.

### 1. Método general (sirve para cualquier dato, no solo posiciones)

1. **Escaneo inicial:** leer toda la memoria privada escribible del proceso y guardar los floats con pinta del dato (p. ej. 0.02 ≤ |v| ≤ 6000). ~45 millones de candidatos.
2. **Filtrar por pasos de tiempo controlado:** reanudar el juego ~50 ms, congelarlo, releer **todos** los candidatos con el juego quieto, comparar con la lectura anterior. 40 pasos. Quedarse con lo que cambia poco entre pasos (para una posición: |Δ| ≤ 0.5 + 0.02·|v|) y cambia en ≥ 1/3 de los pasos. Pasa de 50 M a ~380 mil.
3. **Buscar estructura, no valores sueltos:** el paso por «arrays de punteros» (≥ 20 punteros consecutivos cuyos objetos tienen, todos al mismo desplazamiento, un candidato suave) dio con los jugadores; el análisis por «paso constante» NO sirvió (objetos repartidos en bloques).
4. **Reconocer la forma del dato:** un vector `(x, y≈0.9, z, 1.0)` con w = 1.0 exacto es una posición con altura de cadera. Probar columnas (x,z), (x,y,z)… y exigir: dentro del campo, finitos, varias fichitas distintas.
5. **Verificar con la pantalla:** congelar el juego, hacer captura de pantalla y volcar la memoria en el mismo instante; dibujar las posiciones en planta y compararlas. Un **saque de centro** (reloj 0:00) es la mejor verificación: formaciones conocidas, balón en (0,0).
6. **Firma propia:** escribir una regla que reconozca el objeto sin depender de direcciones (tipo, marcas, vec4 plausible). Las direcciones cambian en cada partido; la firma no.
7. **Protección:** versión del ejecutable (tamaño + SHA-256), valores plausibles, lista completa (18–24), reintento con calma y apagado automático. Nada de escribir.

### 2. Herramientas (todas en `tools/radar/`) y comandos

Compilar cada una: `cl /O2 /EHsc /std:c++17 <archivo>.cpp user32.lib winmm.lib` (tras `vcvars64.bat`). Salidas de ejemplo en `C:\Temp\claude\radar` (no se suben).

| Herramienta | Qué hace | Ejemplo |
|---|---|---|
| `escaner.cpp` | Búsqueda tipo Cheat Engine. Se maneja con un archivo `cmd.txt` (o teclado numérico): `0` escaneo inicial, `6` pasos de 50 ms, `8` arrays de punteros, `3` análisis | `echo 0 > cmd.txt` … `echo 6 > cmd.txt` … `echo 8 > cmd.txt` |
| `analiza.cpp` | Análisis sin conexión de `hist.bin` | `analiza hist.bin out.txt 120` |
| `sonda.cpp` / `objeto.cpp` | Lee en vivo un array de punteros / vuelca los objetos | `sonda 4631D540 44 208 5 300` |
| `buscaptr.cpp` | Quién apunta a una dirección (para buscar rutas estables) | `buscaptr 4631C000 800` |
| `entes.cpp` | **Escáner por firma de los actores** (~5 s); `entes.exe raw` vuelca las 160 palabras de cada objeto | `entes.exe raw > actores.txt` |
| `entes3.cpp` | Firma relajada: porteros, árbitro y otros tipos (`kind`, código, lado) | `entes3.exe > otros.txt` |
| `bola.cpp`, `bola2.cpp` | Búsqueda del balón: candidatos `(x,y,z[,1.0])` con y de balón, y seguimiento por pasos | `bola2 vec4` luego `bola2 pasos bola_cands.txt 100 50` |
| `vigilante.cpp` | Congela el juego solo cuando detecta un saque de centro y lanza `captura.bat` | `vigilante dirs.txt 900` |

Congelar y capturar a mano (PowerShell): `NtSuspendProcess` → `CopyFromScreen` → `entes.exe raw` → `NtResumeProcess` (ver `docs/CAMPO-2D-INVESTIGACION.md`, §2).

### 3. Receta completa para repetir el hallazgo de las posiciones

1. Abre un partido y déjalo en marcha, juego al frente.
2. `escaner.exe` → `echo 0 > cmd.txt` (espera «Escaneo inicial») → `echo 6 > cmd.txt` (≈ 1 min) → `echo 8 > cmd.txt`.
3. En `punteros.txt` busca bloques con 22–24 valores `(x, ~0.9, z, 1.0)` en el mismo desplazamiento: sale el array en `0x4631D540` (en tu sesión será otra) con `+208` = posición.
4. Comprueba con `sonda` que se mueven suave (2–3 m/s). Confirma con una captura congelada.
5. Para uso real: no hace falta nada de lo anterior; Link ya trae la firma (`core/CampoVivo.cpp`) y `host/CampoVivoHost.cpp`.

### 4. Errores que ya nos costaron tiempo (evítalos)

- Clasificar como «parado» los segundos de transición al pausar/reanudar descarta justo los datos buenos.
- El análisis por paso constante entre candidatos no encuentra objetos grandes repartidos en bloques.
- `RAW` de `entes.exe`: la palabra `w` está en la columna `3+w` de awk (`$(3+w)`), en PowerShell `Split(' ')[2+w]`.
- Guardar objetos de 640 bytes cruzando el límite de un trozo de 1 MB: leer 640 bytes de solape.
- El juego puede quedar suspendido si algo falla entre `Suspend` y `Resume`: usar siempre `try/finally`.
- Escanear solo la memoria privada escribible (`MEM_PRIVATE` + `PAGE_READWRITE`): 12 s; todo el espacio: 45 s.
- `.bat` creados desde Git Bash salen con LF y `cmd` los interpreta mal: convertir a CRLF.

### 5. Qué otra información valiosa se puede sacar con el mismo método (ideas)

Todas sin escribir en el juego; cada una necesita su propia verificación contra la pantalla.

1. **Balón** (en curso): posición, altura y velocidad → radar completo, tiros/pases, zona de peligro.
2. **Equipo de cada jugador** (en curso) y **quién lleva el balón**.
3. **Velocidades y distancias recorridas** por jugador (derivadas de las posiciones): mapa de calor, kilómetros, sprints.
4. **Posesión real por zonas**, líneas de pase, formaciones en vivo (4-3-3 → 4-4-2).
5. **Eventos**: fuera de juego, saques de banda/esquina/penal por posición del balón; refuerza el árbitro de Link (marcador, minuto y goles ya vienen de Sider).
6. **Penales**: balón quieto en un punto conocido → referencia perfecta, y datos del lanzador y el portero.
7. **Repeticiones para streaming/overlay**: las mismas posiciones sirven para un mini-radar en PhoenixGlass o para el stream del torneo.
8. **Datos de IA** (tablas con nombres de huesos y registros por jugador): estructuras que quedaron sin descifrar.

### 6. Para continuar donde lo dejamos

Pendiente por orden: (1) confirmar el balón (candidatos `kind=0`, y ≈ 0.10–0.11, ver §7 de la guía de investigación) con el juego en marcha; (2) equipo por jugador con un saque de centro congelado; (3) probar la conexión en el PhoenixLink.exe real; (4) mini-radar en el overlay. Registro de cambios: `REGISTRO-LINK.md`.

---
