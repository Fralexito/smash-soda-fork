# Phoenix Link · Lo que Link le pide a la web (prompts, ranking, sistema de jugadores, contrato y traspaso del 17-oct)

Todo lo que Link necesitaba que hiciera la web y Supabase: prompts para el chat WEB, diseño del ranking 1v1 y del sistema de jugadores, el contrato «Cambios oficiales» v1 y el traspaso del 17-oct (frentes 125–128, terminados el 18-oct).

La versión viva del contrato web↔PES está también en el repo de la web (`docs/web-pes-link/CONTRATO-WEB-PES.md`). Si no coinciden, manda la de la web.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 13 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · PROMPT para el chat WEB · Marcador en vivo y validación de la liga](#parte-1-prompt-para-el-chat-web-marcador-en-vivo-y-validacion-de-la-liga) — origen `docs/PROMPT-WEB-liga-vivo.md` (001b77d 2026-10-11)
- [Parte 2 · PROMPT para el chat WEB · Marcador «En vivo ahora» que se queda colgado](#parte-2-prompt-para-el-chat-web-marcador-en-vivo-ahora-que-se-queda-colgado) — origen `docs/PROMPT-WEB-liga-vivo-huerfano.md` (08632a5 2026-10-11)
- [Parte 3 · PROMPT para el chat WEB · Presencia: «Activo» solo en la web, «En sala» con Phoenix Link](#parte-3-prompt-para-el-chat-web-presencia-activo-solo-en-la-web-en-sala-con-phoenix-link) — origen `docs/PROMPT-WEB-presencia.md` (d121a58 2026-10-11)
- [Parte 4 · Ranking de jugadores 1 contra 1 (Phoenix Link → web)](#parte-4-ranking-de-jugadores-1-contra-1-phoenix-link-web) — origen `docs/RANKING-1V1.md` (87b6fc8 2026-10-11)
- [Parte 5 · PROMPT para el chat WEB · Ranking de jugadores 1 contra 1](#parte-5-prompt-para-el-chat-web-ranking-de-jugadores-1-contra-1) — origen `docs/PROMPT-WEB-ranking-1v1.md` (87b6fc8 2026-10-11)
- [Parte 6 · Sistema de jugadores: estadísticas, ranking, rangos, prestigio y logros](#parte-6-sistema-de-jugadores-estadisticas-ranking-rangos-prestigio-y-logros) — origen `docs/SISTEMA-JUGADORES.md` (142c11c 2026-10-11)
- [Parte 7 · PROMPT para el chat WEB · Sistema de jugadores (rangos, prestigio, fair play, logros, temporadas)](#parte-7-prompt-para-el-chat-web-sistema-de-jugadores-rangos-prestigio-fair-play-logros-temporadas) — origen `docs/PROMPT-WEB-sistema-jugadores.md` (98172b7 2026-10-11)
- [Parte 8 · PROMPT para el chat WEB / Sync · Lista de fichajes como cambios](#parte-8-prompt-para-el-chat-web-sync-lista-de-fichajes-como-cambios) — origen `docs/PROMPT-WEB-fichajes-lista.md` (442861a 2026-10-12)
- [Parte 9 · PROMPT para el chat WEB · Tablero del partido al tocar «En vivo ahora» (campo 2D + estadísticas)](#parte-9-prompt-para-el-chat-web-tablero-del-partido-al-tocar-en-vivo-ahora-campo-2d-estadisticas) — origen `docs/PROMPT-WEB-tablero-partido.md` (ed9a255 2026-10-12)
- [Parte 10 · PROMPT para el chat WEB · Traspasos en vivo — **OBSOLETO: implementado directamente**](#parte-10-prompt-para-el-chat-web-traspasos-en-vivo-obsoleto-implementado-directamente) — origen `docs/PROMPT-WEB-traspasos-vivo.md` (cdd4b5f 2026-10-12)
- [Parte 11 · Contrato WEB ↔ PES · «Cambios oficiales» (v1)](#parte-11-contrato-web-pes-cambios-oficiales-v1) — origen `docs/CONTRATO-WEB-PES.md` (5ae6b2b 2026-10-17)
- [Parte 12 · Traspaso 17-oct-2026 · Web ↔ PES (para continuar con la otra cuenta)](#parte-12-traspaso-17-oct-2026-web-pes-para-continuar-con-la-otra-cuenta) — origen `docs/TRASPASO-17-OCT-WEB-PES.md` (3678426 2026-10-17)
- [Parte 13 · PROMPT para el chat WEB · Plan de experiencia de Phoenix Link (19-oct-2026)](#parte-13-prompt-para-el-chat-web-plan-de-experiencia-de-phoenix-link-19-oct-2026) — origen `docs/PROMPT-WEB-plan-ux.md` (053c502 2026-10-19)

---

## Parte 1 · PROMPT para el chat WEB · Marcador en vivo y validación de la liga

> Origen: `docs/PROMPT-WEB-liga-vivo.md` · último cambio 001b77d 2026-10-11 · 7132 bytes. Copia íntegra.

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`). Origen: chat LINK (árbitro), 2026-10-11. La base ya está hecha y aplicada en Supabase. Está en la rama **`liga-automatica`** (commit `5b69fad`, ya subida). Diseño completo: `docs/LIGA-AUTOMATICA.md` de esa rama. Protocolo de Link: `SmashSoda/phoenix/web/PROTOCOLO.md` (repo `smash-soda-fork`, rama `rediseño-phoenix-portal`).

### Qué quiere Fralex
1. Ver **en tiempo real** en la web el marcador de los partidos que se juegan en salas de Phoenix Link.
2. Que el resultado de un partido oficial lo **valide el staff o un árbitro designado, nunca el host**. Los partidos pueden ser en host neutral.
3. Que los partidos **contra la máquina (CPU)** salgan en el historial, pero que no cuenten para el ranking.

### Lo que ya existe (no lo rehagas)
- **Rama `liga-automatica`**:
  - migraciones `102_liga_automatica.sql` y `102b_liga_automatica_datos.sql`, **ya aplicadas en producción**;
  - Edge Function `supabase/functions/liga` (v1 desplegada);
  - `docs/LIGA-AUTOMATICA.md`.
  - **Antes de nada, júntala con la rama de trabajo actual** (`borrador`/fase-beta), sin pisar nada. La próxima migración tuya es la **103**.
- **Tablas** (lectura pública por RLS; nadie escribe desde el navegador):
  - `liga_ediciones`, `liga_participantes` (`usuario` = cuenta de la web del DT).
  - `liga_partidos`:
    - `estado`: `pendiente | propuesto | confirmado | disputa | anulado`;
    - `origen`: `copafacil | link | staff`;
    - otras columnas: `arbitro`, `detalle` (jsonb con goles y minuto, penales, `doctor`, `reclamo`), `resuelto_por`, `resuelto_en`.
  - `liga_vivo`: una fila por sala mientras dura el partido.
    - Columnas: `partido_id` (null = amistoso), `local`, `visitante`, `goles_local`, `goles_visita`, `minuto`, `periodo`, `fase` (`previa | en_juego | descanso | pausado | final`), `goles` `[{m, l:"l"|"v", j?, pp?}]` y `actualizado`. `j` = goleador (nombre de la camiseta) y `pp:true` = en propia puerta; solo llegan con Sider 7.3.4+, si no, el gol va sin nombre. Muéstralo como «34′ MESSI» o «80′ ARAUJO (p.p.)». Lo mismo en `liga_partidos.detalle.juego.goles` y en `partida_fin` (`juego.goles[{m, lado, jugador?, propia?}]`).
    - La función la borra al terminar.
- **Realtime** está activado en `liga_vivo` y `liga_partidos`. Se suscribe con la anon key; no hace falta consultar cada X segundos.
- **Vista `liga_tabla`**: `edicion_id, nombre, club, pj, g, e, p, gf, gc, dg, pts`. Solo cuenta los resultados `confirmado`.
- **Funciones RPC** (security definer; llámalas con `supabase.rpc`):
  - `liga_resolver_resultado(p_partido, p_aceptar, p_goles_local?, p_goles_visita?)` → texto. Solo la usan el staff o el árbitro designado de ese partido (si no, error 42501).
    - `true` = confirmado; puede corregir el marcador.
    - `false` = disputa.
  - `liga_reclamar_resultado(p_partido, p_motivo)`: solo los DT de ese partido. Pasa a `disputa` y guarda `detalle.reclamo`.
  - `liga_designar_arbitro(p_partido, p_arbitro uuid)`: solo el staff.
- **Interruptor** `phoenix_config.liga_resultados` (en `integracion`): apaga la recepción de resultados desde Link.
- **Datos ya cargados**: Galaxy edición 1, 22 fechas, 132 partidos, 52 confirmados. La tabla coincide con la de la web (Victor 27, Beto 24, Degox 21…).

### Qué hay que hacer

#### 1) Marcador en vivo (Inicio y la página «En vivo» si existe)
- Al cargar: `select * from liga_vivo`. Después, suscríbete a `postgres_changes` de `liga_vivo` (insert, update y delete).
- Tarjeta por partido, dentro del diseño actual de la web (no lo rehagas):
  - local – visitante, marcador grande y minuto;
  - fase (EN JUEGO / DESCANSO / PAUSA / FINAL);
  - los goles con su minuto;
  - un chip **OFICIAL** si `partido_id` no es null (con la fecha y la edición sacadas de `liga_partidos`), o **AMISTOSO** si es null.
- Sin filas: no muestres nada, o una línea discreta «No hay partidos en juego».
- Al llegar un `delete` (fin del partido): deja la tarjeta unos 30 s con «FINAL» y luego quítala.
- Link solo manda los amistosos si la sala no es privada. No hay que filtrar nada más.

#### 2) Panel de validación del staff (zona de staff/admin que ya exista)
- Lista de partidos `propuesto` y `disputa` (los más nuevos primero). Cada uno muestra:
  - marcador, quién lo propuso (`origen`, sala) y la hora;
  - los goles con su minuto (`detalle`);
  - el **reclamo** si hay uno (`detalle.reclamo.motivo`);
  - el resumen del **Phoenix Doctor** (`detalle.doctor`: huella, parche, avisos). Es **solo informativo**: aún no hay parche oficial, así que muéstralo como dato, sin aprobar ni rechazar automáticamente nada.
- Botones:
  - **CONFIRMAR** → `liga_resolver_resultado(id, true)`;
  - **CORREGIR Y CONFIRMAR** (dos números) → `(id, true, gl, gv)`;
  - **EN DISPUTA** → `(id, false)`.
- **Designar árbitro**: buscador de perfiles → `liga_designar_arbitro(id, uuid)`. Disponible también en los `pendiente`, para designarlo antes de que se juegue.
- **Vista del árbitro designado** (no es staff): «Mis partidos para validar» = `liga_partidos where arbitro = auth.uid()`, con los mismos botones.
- Muestra el error de la RPC tal cual: el texto ya viene en español.

#### 3) Botón «Reclamar» para los DT
- En el detalle de un partido `propuesto` o `confirmado`, si el usuario es uno de sus DT (`liga_participantes.usuario = auth.uid()` para `local` o `visitante`): botón **RECLAMAR** con un motivo de 300 caracteres como máximo → `liga_reclamar_resultado`.

#### 4) Página de la liga
- Tabla de posiciones desde **`liga_tabla`**, en vez del cálculo local, para las ediciones que ya estén en la base (hoy Galaxy edición 1). El resto sigue igual.
- Resultados por fecha desde `liga_partidos`:
  - `propuesto` con la marca «pendiente de validar»;
  - `disputa` con la marca «en disputa»;
  - `anulado` tachado o sin mostrar.
- Suscríbete a `liga_partidos` para que una confirmación del staff se vea al momento.

#### 5) Partidos contra la máquina (CPU) · evento `partida_fin` de Link
- Link manda ahora `datos.contra_ia: true` y `datos.modo: "contra_ia"`. El jugador de la máquina va como `{ "parsec_id": "0", "nombre": "Máquina", "es_ia": true }`.
- Guárdalos en el historial de partidos (si existe `partidos_link` del ranking 1v1) con una marca 🤖 «vs CPU».
- **No cuentan para el ranking 1v1** ni para ninguna estadística competitiva.
- `parsec_id "0"` nunca se resuelve contra `cuentas_parsec`.
- Si el ranking ya filtra por `modo = '1v1'`, basta con eso. Compruébalo y añade una prueba.

### Reglas
- No toques `main`. Trabaja sobre la rama de trabajo y pide permiso a Fralex antes de hacer commit o merge.
- Las migraciones nuevas van desde la **103**.
- Nada de escribir en estas tablas desde el navegador: todo por RPC (ya están los `revoke`).
- Mantén el diseño actual de la web; Fralex no quiere rediseños sin pedirlos.
- Al terminar: una línea en la bitácora de la web y aviso al chat LINK si cambia algún contrato (`/v1/liga/*`, `partida_fin`).

---

## Parte 2 · PROMPT para el chat WEB · Marcador «En vivo ahora» que se queda colgado

> Origen: `docs/PROMPT-WEB-liga-vivo-huerfano.md` · último cambio 08632a5 2026-10-11 · 2630 bytes. Copia íntegra.

> Repo `Fralexito/phoenixevolution` (rama `borrador`). Origen: chat LINK, 2026-10-11. Fralex: «se quedó así, hace horas que se dejó de jugar» (Arensz 2-1 Erfuzz, 90' · 2.º tiempo).

### Por qué pasa
`liga_vivo` solo se borra cuando Link manda `POST /v1/liga/vivo {terminado:true}` (o con `/v1/liga/resultado`). Link solo lo mandaba si cerraba el partido en su marcador con la sala aún publicada. Si el host cerraba la sala o Phoenix Link, cerraba PES o el partido se abandonaba, la fila se quedaba para siempre.
- `sala/cerrar` y la marca de sala `caida` (3 min sin latido) **no** borran `liga_vivo`.
- Las salas no se borran: el `on delete cascade` nunca llega a actuar.
- `vivo.js` muestra todas las filas, sin mirar `actualizado`.

### Ya hecho en Phoenix Link (rama `rediseño-phoenix-portal`)
- Al cerrar la sala o la app, Link manda `terminado:true` **antes** de `sala/cerrar`.
- Mientras hay partido y el juego da datos, Link reenvía el marcador **al menos cada 2 minutos** (aunque no cambie nada). Así `actualizado` sirve de latido.

### Qué hay que hacer en la web
1. **Limpiar ya** (una vez). Usa los estados reales de `salas`:
   ```sql
   delete from public.liga_vivo v
   using public.salas s
   where s.id = v.sala_id and (s.estado <> 'abierta' or v.actualizado < now() - interval '10 minutes');
   ```
2. **Función `phoenix`**: en `POST /v1/sala/cerrar`, y donde se marca una sala como `caida`, borrar también su fila de `liga_vivo` (`delete … where sala_id = $1`).
3. **Limpieza periódica** (pg_cron cada 5 min, o dentro del proceso que ya marca las salas caídas): borrar las filas de `liga_vivo` con `actualizado` de más de 10 minutos, o cuya sala no esté `abierta`.
4. **`src/js/features/liga/vivo.js`**: no mostrar filas con `actualizado` de más de 10 minutos, y volver a comprobarlo cada minuto (sin esperar a Realtime), así una tarjeta vieja desaparece aunque nadie la borre. Al recibir un DELETE por Realtime, quitar la tarjeta (comprobar que ya lo hace).

### Criterios de aceptación
1. Tras el paso 1, la tarjeta «Arensz 2-1 Erfuzz» ya no sale.
2. Abrir sala, jugar y **cerrar la sala sin terminar el partido** → la tarjeta desaparece en segundos.
3. Cerrar Phoenix Link a la fuerza (sin avisar) → la tarjeta desaparece como mucho en 10 minutos.
4. Un partido con una pausa larga en el juego **no** desaparece: Link manda el latido cada 2 minutos.
5. Nada de lo publicado en el contrato cambia.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK para quitar el `PENDIENTE-WEB`.

---

## Parte 3 · PROMPT para el chat WEB · Presencia: «Activo» solo en la web, «En sala» con Phoenix Link

> Origen: `docs/PROMPT-WEB-presencia.md` · último cambio d121a58 2026-10-11 · 3040 bytes. Copia íntegra.

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). Origen: chat LINK, 2026-10-11.

### Qué quiere Fralex
«Que salga **activo** en Phoenix si está dentro de la página web, y que salga **en sala** si tiene abierto Phoenix Link.»

Hoy un jugador sale **ACTIVO** (`disponible`) solo por tener Phoenix Link abierto, aunque no esté en la web. Eso cambia.

### Estados (los mismos nombres de siempre)
| Estado | Cuándo | Quién lo marca |
|---|---|---|
| `disponible` («ACTIVO») | Tiene la **página web** abierta y visible | **La web** (nuevo) |
| `en_sala` («EN SALA») | Tiene **Phoenix Link** abierto | Link, por `POST /v1/presencia` |
| `en_partida` («JUGANDO») | Partido en curso o PES abierto | Link, por `POST /v1/presencia` |
| `desconectado` | Nada de lo anterior en los últimos ~2 min | Caduca solo |

Si hay varios a la vez, gana el más fuerte: `en_partida` > `en_sala` > `disponible`.

### Qué cambia en Phoenix Link (ya hecho, rama `rediseño-phoenix-portal`)
`POST /v1/presencia` (mismo endpoint, contrato §presencia):
```json
{ "estado": "en_sala", "origen": "link", "sala_id": "…" }      // Link abierto
{ "estado": "en_partida", "origen": "link", "sala_id": "…" }   // jugando
```
- `en_sala` reemplaza al `disponible` que mandaba Link. `origen` es un campo nuevo y opcional.
- Si la web responde **400** a eso, Link vuelve solo a mandar `{"estado":"disponible"}` sin `origen`, como antes (no se pierde la presencia). Por eso conviene **aceptar `en_sala` y `origen`** antes de que Fralex reparta el nuevo Link.

### Qué tiene que hacer la web
1. **Aceptar** en `POST /v1/presencia` el estado `en_sala` y el campo opcional `origen` (`"link"`). No romper con valores desconocidos: ignorarlos.
2. **Latido de la página web:** mientras la pestaña está abierta y visible (`document.visibilityState === "visible"`), marcar `disponible` cada ~60 s (la misma tabla de presencia, `origen: "web"`). Al ocultarse o cerrarse, dejar de marcarlo (caduca a los ~2 min, o `navigator.sendBeacon` al cerrar).
3. **Guardar por origen:** la presencia de Link y la de la web no se pisan. Lo que se muestra es la más fuerte de las que no han caducado.
4. **Mostrar:** en la lista de jugadores, perfiles y amigos:
   - «ACTIVO» (verde) solo con `disponible` de la web;
   - «EN SALA» con `en_sala` de Link;
   - «JUGANDO» con `en_partida`.
   `GET /v1/presencia/amigos` sigue devolviendo `estado` con esos mismos nombres (Link ya los pinta: `disponible` = ACTIVO, `en_sala` = EN SALA, `en_partida` = JUGANDO).
5. **Compatibilidad:** un Link viejo sigue mandando `disponible` sin `origen`. Tratarlo como `en_sala` (viene de Link, no de la web).

### Hecho cuando
- Con solo Phoenix Link abierto, el jugador sale «EN SALA», no «ACTIVO».
- Con solo la web abierta, sale «ACTIVO».
- Con las dos abiertas, sale «EN SALA». Jugando, sale «JUGANDO».
- Al cerrar todo, pasa a desconectado en ~2 min.

---

## Parte 4 · Ranking de jugadores 1 contra 1 (Phoenix Link → web)

> Origen: `docs/RANKING-1V1.md` · último cambio 87b6fc8 2026-10-11 · 9385 bytes. Copia íntegra.

Estado: **Link HECHO** (envía `partida_fin` v2). **Web PENDIENTE-WEB** (`docs/PROMPT-WEB-ranking-1v1.md`).

### 1. Flujo de hoy (gol → Link → historial → web)

```
PES 2021 + phoenix_estadio.lua (Sider)
   │  estado.json cada ~1 s: fase, minuto, goles_local/visita, goles[{m,l}], pk_*, completo, equipos
   ▼
PhoenixLink (hilo propio)  link/EstadoJuego.cpp · solo lectura; >5 s = «sin datos»
   │  PhoenixLink::estadoJuego()
   ▼
Árbitro (web/Arbitro.cpp, 2 veces/s)
   ├─ arranque automático: lados «listo» + reloj corriendo → Partido::iniciar + evento partida_inicio
   ├─ goles del juego → Partido::fijarGoles (lado A = local salvo «Cambiar lados»)
   ├─ chat: «GOL 34' de Mirko. Mirko 2-1 Kaiser», descanso
   └─ fase final + completo → finalizarPartido()
                                   │ (también el botón FINALIZAR: partido.finalizar)
                                   ▼
web/AccionesPartidoWeb.cpp · finalizarPartido()
   ├─ Partido::finalizar → RegistroPartido
   ├─ historial local %APPDATA%\Trybuchet\Smash Soda\phoenix-partidos.json (últimos 200)
   └─ PhoenixLink::evento("partida_fin", datos)  → cola phoenix-eventos.json (máx. 500, persistente)
                                   │ cada 15 s, lotes de 50
                                   ▼
POST /v1/eventos {sala_id, eventos[{clave uuid, tipo, ocurrido, datos}]}  (contrato §6, token del PC)
```

**Antes de este cambio** `partida_fin` solo llevaba `{"marcador":"2-1"}` (o `"cancelado"`). La web no sabía quién jugó, en qué lado, qué goles ni si el partido terminó de verdad.

Quién juega dónde: `partido.preparar{a[],b[]}` guarda en cada lado los jugadores (parsecId + nombre). Esos son los puestos de mando que el host elige en Partido. La cuenta web **no** la decide Link: la web la resuelve con `cuentas_parsec` (la misma que usa `/v1/perfiles`). Link solo tiene una copia de los perfiles para mostrarlos, y la web no debe fiarse de ella.

### 2. Qué faltaba para un ranking fiable → qué manda ahora Link

| Necesidad | Antes | Ahora (`partida_fin` v2) |
|---|---|---|
| Identidad de cada jugador | — | `a/b.jugadores[].parsec_id` (texto de dígitos). La web lo pasa a `usuario_id` |
| Resultado | `marcador` | `a.goles`, `b.goles`, `ganador`, `juego.penales_a/b` |
| Goles por persona | — | 1v1: los goles del lado son de su único jugador. `juego.goles[{m,lado}]` con minuto |
| Válido o abandonado | — | `cierre`: `completo` (el juego confirmó el final) · `manual` (lo cerró el host) · `cancelado`. Y `abandono_juego` |
| Partido corto | — | `duracion_seg` (reloj real sin pausas) y `juego.minuto` |
| Misma persona en los dos lados | Link ya lo impide (`Partido::preparar`) | La web lo vuelve a comprobar con `usuario_id` |
| Marcador tocado a mano | — | `correcciones` (veces que se usó +/−) y `juego.coincide` (marcador de Link = marcador del juego) |
| Jugador que se fue | — | `jugadores[].presente` al terminar |
| Host que juega y cierra él | — | `jugadores[].es_host`, `host_parsec` |
| Duplicados | — | `partido_id` (= `inicio_ms`, el mismo id que el historial local) + `clave` del evento |

Partidos con varios por lado: se mandan igual con `modo:"equipos"`. Se guardan, pero **no se atribuyen goles a personas** (eso lo investiga otra sesión).

### 3. Contrato: `datos` de `partida_fin` v2

Mismo endpoint y mismo tipo (`POST /v1/eventos`, tipo `partida_fin`). Solo crecen los `datos`, que el contrato deja libres (≤ 4 KB). Si no caben, Link quita `juego.goles`. `marcador` se mantiene, así que la web vieja no se rompe.

```json
{
  "v": 2,
  "marcador": "2-1",                 // "cancelado" si se canceló
  "partido_id": "1760140000000",     // epoch ms del inicio; único por sala
  "inicio_ms": 1760140000000, "fin_ms": 1760140720000,
  "duracion_seg": 690,               // reloj real de Link sin pausas
  "cierre": "completo",              // completo | manual | cancelado
  "modo": "1v1",                     // 1v1 (1 persona por lado) | equipos
  "ganador": "a",                    // a | b | empate | "" (cancelado)
  "a": { "nombre": "Mirko", "goles": 2, "local": true,
         "jugadores": [ { "parsec_id": "111", "nombre": "Mirko", "presente": true, "es_host": true } ] },
  "b": { "nombre": "Kaiser", "goles": 1, "local": false,
         "jugadores": [ { "parsec_id": "222", "nombre": "Kaiser", "presente": true, "es_host": false } ] },
  "correcciones": 0, "pausas": 1,
  "lados": { "origen": "host", "correcciones": 0 },   // host = los eligió el anfitrión; mandos = Link los armó solo al pitazo
  "host_parsec": "111",
  "abandono_juego": false,
  "juego": {                         // solo si había estado.json fresco al cerrar
    "fase": "final", "minuto": 90, "completo": true,
    "equipo_a": 101, "equipo_b": 102, "nombre_equipo_a": "Real Madrid", "nombre_equipo_b": "Barcelona",
    "goles_a": 2, "goles_b": 1, "coincide": true,
    "penales_a": 4, "penales_b": 3,  // solo si hubo tanda
    "goles": [ { "m": 12, "lado": "a" }, { "m": 34, "lado": "b" }, { "m": 80, "lado": "a" } ]
  }
}
```

- `local`: el lado A juega con el equipo local, salvo con los mandos cruzados (tras «Cambiar lados»). `equipo_*`, `goles_*`, `penales_*` y `goles[].lado` ya vienen pasados a lados A/B.
- `presente`: el jugador seguía en la sala al cerrar (el host siempre).
- Sin `v` (Link viejo) = solo marcador: la web lo guarda como evento y no cuenta para el ranking.
- `partida_inicio` y `pausa` no cambian.

Local: `phoenix-partidos.json` y `partido.historial` ganan `correcciones`, `pausas` y `cierre` (los registros viejos se leen igual).

### 4. Cómo lo guarda y calcula la web (propuesta)

Detalle (SQL, RLS, criterios) en `docs/PROMPT-WEB-ranking-1v1.md`. Resumen:

- **`partidos_link`**: una fila por `partida_fin` v2 (todos los modos, también cancelados). Única por `(sala_id, partido_id)`. Guarda `datos` en bruto, el `estado_ranking` (`valido` | `revisar` | `excluido`) y sus `motivos`.
- **`partidos_link_jugadores`**: lado, `parsec_id`, `usuario_id` resuelto **al recibirlo** (foto fija), nombre, presente, es_host y `goles` (solo 1v1).
- **`partidos_link_goles`**: minuto, lado y `usuario_id` (solo 1v1; en `equipos`, nulo).
- **`ranking_1v1`**: por usuario: elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5. Se recalcula desde cero, en orden de fecha, con `recalcular_ranking_1v1()` cada vez que entra o se revisa un partido 1v1 (a esta escala es barato y deja corregir la historia).

#### Reglas de validez (las aplica la web, no Link)

Excluido (se guarda, no cuenta):
- `modo` ≠ `1v1`, `cierre` = `cancelado`, o no es `v` 2.
- Algún jugador sin cuenta web vinculada, o el mismo `usuario_id` en los dos lados.
- `duracion_seg` < 180.

Por revisar (no cuenta hasta que lo apruebe el staff):
- `cierre` = `manual` (el juego no confirmó el final).
- `lados.origen` = `mandos` y `lados.correcciones` = 0: Link armó los lados según la formación y nadie los confirmó (pueden estar al revés). Esta regla se quita cuando Link detecte el lado de cada mando en el juego (llegará `origen: "juego"`).
- `correcciones` > 0, `juego.coincide` = false o `abandono_juego`.
- Algún jugador con `presente` = false.
- Más de 3 partidos válidos de la misma pareja en 24 h (anti-farmeo): del 4.º en adelante.

Válido: todo lo demás (lo normal: `completo`, sin tocar, los dos presentes).

#### Puntuación: Elo + tabla de puntos

- **Orden del ranking: Elo.** Empieza en 1000. E = 1 / (1 + 10^((Rrival − R)/400)). S = 1 / 0,5 / 0. K = 40 en los 10 primeros partidos válidos y 24 después. Por diferencia de goles, ×1 (1 gol), ×1,5 (2 goles), ×(11+N)/8 (3 o más).
  - Por qué Elo: en una liga pequeña, ganarle al mejor vale más que ganarle a un novato. Además, farmear contra cuentas débiles casi no da puntos. Con puntos puros, gana quien más juega.
- **Columnas de la tabla (fáciles de leer):** PJ, PG, PE, PP, GF, GC, DG, Pts (3/1/0) y racha (`G3`, `E1`, `P2`).
- Penales: cuentan como empate para Elo, PG, PE y Pts. La tanda se guarda y se muestra aparte.
- Con menos de 5 PJ el jugador sale como «provisional», debajo o con marca.

### 5. Archivos de Link tocados

- `SmashSoda/phoenix/web/AccionesPartidoWeb.cpp`: `ladoWeb()` y `datosResultado()`. `finalizarPartido()` y `partido.cancelar` mandan `partida_fin` v2. El historial guarda `cierre`.
- `SmashSoda/phoenix/web/Partido.h/.cpp`: cuenta `correcciones` (+/− que cambian el marcador) y `pausas`. Expone `inicioMs()`, `correcciones()` y `pausas()`. `RegistroPartido` gana `correcciones`, `pausas` y `cierre`.
- No se tocó el árbitro, los mandos ni la UI. `completo` se lee de `estadoJuego()` dentro de `finalizarPartido`, que el árbitro llama justo en `final` + `completo`.

### 6. Riesgos y pendientes

- Sin probar con PES real (igual que phoenix_estadio.lua). Sin `estado.json`, todo sale como `manual` y queda «por revisar».
- Si se cruzan los mandos a mitad de partido, `goles[].lado` usa el cruce del final. En 1v1 casi no pasa.
- El `usuario_id` lo decide la web en el momento de recibir el partido. Si alguien vincula su Parsec después, sus partidos viejos no se le asignan (se podría rehacer con una función de staff).
- Goles por persona en partidos con varios por lado: lo investiga otra sesión.

---

## Parte 5 · PROMPT para el chat WEB · Ranking de jugadores 1 contra 1

> Origen: `docs/PROMPT-WEB-ranking-1v1.md` · último cambio 87b6fc8 2026-10-11 · 10200 bytes. Copia íntegra.

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). Origen: chat LINK, 2026-10-11. Diseño completo: `docs/RANKING-1V1.md` en `smash-soda-fork`, rama `rediseño-phoenix-portal`.

### Qué quiere Fralex
Un ranking de jugadores en la web, empezando por los partidos 1 contra 1 que se juegan en salas de Phoenix Link.

### Qué ya manda Phoenix Link (no hay endpoint nuevo)
Al terminar cada partido, Link manda el evento `partida_fin` por el `POST /v1/eventos` de siempre (contrato §6). Ahora sus `datos` traen `"v": 2` y el resultado completo. Un Link viejo solo manda `{"marcador":"2-1"}`: eso se sigue aceptando y se ignora para el ranking.

```json
{
  "v": 2, "marcador": "2-1", "partido_id": "1760140000000",
  "inicio_ms": 1760140000000, "fin_ms": 1760140720000, "duracion_seg": 690,
  "cierre": "completo",            // completo | manual | cancelado
  "modo": "1v1",                   // 1v1 | equipos
  "ganador": "a",                  // a | b | empate | ""
  "a": { "nombre": "Mirko", "goles": 2, "local": true,
         "jugadores": [ { "parsec_id": "111", "nombre": "Mirko", "presente": true, "es_host": true } ] },
  "b": { "nombre": "Kaiser", "goles": 1, "local": false,
         "jugadores": [ { "parsec_id": "222", "nombre": "Kaiser", "presente": true, "es_host": false } ] },
  "correcciones": 0, "pausas": 1, "host_parsec": "111", "abandono_juego": false,
  "lados": { "origen": "host", "correcciones": 0 },   // host | mandos (Link los armó solo) | juego (futuro: detectado en el juego)
  "juego": { "fase": "final", "minuto": 90, "completo": true,
             "equipo_a": 101, "equipo_b": 102, "nombre_equipo_a": "Real Madrid", "nombre_equipo_b": "Barcelona",
             "goles_a": 2, "goles_b": 1, "coincide": true, "penales_a": 4, "penales_b": 3,
             "goles": [ { "m": 12, "lado": "a" }, { "m": 34, "lado": "b" } ] }
}
```
- `juego` solo viene si había datos del juego al cerrar. `penales_*` solo si hubo tanda. Si `datos` pasa de ~3,8 KB, Link quita `juego.goles`.
- Los partidos con varios por lado (`modo: "equipos"`) también llegan. **Se guardan, pero sin atribuir goles a personas.**
- `parsec_id` es texto de dígitos. El `usuario_id` lo resuelve la web con `cuentas_parsec`, igual que en `/v1/perfiles`. Lo que Link crea saber no vale.

### Qué hay que hacer

#### 1) Migración (adapta los nombres a los reales: `cuentas_parsec`, `perfiles`, `salas`, etc.)
```sql
create table public.partidos_link (
  id              bigserial primary key,
  sala_id         uuid not null,
  partido_id      text not null,               -- datos.partido_id
  evento_clave    text not null unique,        -- clave del evento
  host_usuario_id uuid,                        -- dueño del token que lo envió
  inicio          timestamptz,
  fin             timestamptz not null,
  duracion_seg    int not null,
  cierre          text not null check (cierre in ('completo','manual','cancelado')),
  modo            text not null check (modo in ('1v1','equipos')),
  goles_a         int not null, goles_b int not null,
  penales_a       int, penales_b int,
  ganador         text not null default '',    -- a | b | empate | ''
  correcciones    int not null default 0,
  pausas          int not null default 0,
  abandono_juego  boolean not null default false,
  datos           jsonb not null,              -- tal cual llegó
  estado_ranking  text not null default 'excluido' check (estado_ranking in ('valido','revisar','excluido')),
  motivos         text[] not null default '{}',
  revisado_por    uuid, revisado_en timestamptz,
  creado_en       timestamptz not null default now(),
  unique (sala_id, partido_id)
);
create table public.partidos_link_jugadores (
  partido    bigint not null references public.partidos_link(id) on delete cascade,
  lado       text not null check (lado in ('a','b')),
  parsec_id  text not null,
  usuario_id uuid,                              -- foto fija al recibirlo
  nombre     text not null default '',
  presente   boolean not null default true,
  es_host    boolean not null default false,
  goles      int,                               -- solo 1v1; null en equipos
  primary key (partido, lado, parsec_id)
);
create table public.partidos_link_goles (
  partido    bigint not null references public.partidos_link(id) on delete cascade,
  orden      int not null,
  minuto     int not null,
  lado       text not null check (lado in ('a','b')),
  usuario_id uuid,                              -- solo 1v1
  primary key (partido, orden)
);
create table public.ranking_1v1 (
  usuario_id uuid primary key,
  elo int not null default 1000,
  pj int not null default 0, pg int not null default 0, pe int not null default 0, pp int not null default 0,
  gf int not null default 0, gc int not null default 0, pts int not null default 0,
  racha text not null default '',               -- 'G3', 'E1', 'P2'
  ultimos text not null default '',             -- últimos 5, el más reciente a la izquierda: 'GGEPG'
  ultimo_partido timestamptz,
  actualizado timestamptz not null default now()
);
```
RLS: activado en las cuatro tablas. Solo `service_role` escribe (la función edge y las funciones `security definer`). El público lee **solo por vistas**, sin `datos` ni `parsec_id`: `ranking_1v1_publico` y `partidos_1v1_publico` (fecha, jugadores con nombre y avatar, marcador, penales y estado). El staff lee las tablas.

#### 2) Función `registrar_partido_link(p_sala uuid, p_clave text, p_ocurrido timestamptz, p_host uuid, p_datos jsonb)` (security definer)
- Sin `p_datos->>'v' = '2'` → no hace nada.
- Inserta en `partidos_link` con `on conflict (sala_id, partido_id) do nothing`. Si ya existía, no hace nada más (idempotente).
- Por cada jugador de `a` y `b`: resuelve `usuario_id` con `cuentas_parsec.parsec_id` e inserta en `partidos_link_jugadores`. En 1v1, `goles` = goles de su lado.
- `juego.goles[]` → `partidos_link_goles` (en 1v1 con el `usuario_id` del lado).
- Calcula `estado_ranking` y `motivos`:
  - **excluido**: `modo` ≠ `1v1` (`equipos`), `cierre = 'cancelado'` (`cancelado`), un jugador sin `usuario_id` (`sin_cuenta`), el mismo `usuario_id` en los dos lados (`misma_persona`), `duracion_seg < 180` (`muy_corto`).
  - **revisar** (si no está excluido): `lados.origen = 'mandos'` con `lados.correcciones = 0` (`lados_sin_confirmar`: Link armó los lados solo y pueden estar al revés; `origen = 'juego'` llegará más adelante y vale como `host`), `cierre = 'manual'` (`cierre_manual`), `correcciones > 0` (`marcador_tocado`), `juego.coincide = false` (`no_coincide_juego`), `abandono_juego` (`abandono`), algún `presente = false` (`jugador_ausente`), ya hay 3 o más partidos `valido` de la misma pareja en las 24 h anteriores (`farmeo`).
  - **valido**: el resto.
- Si es 1v1 y no `excluido` → `perform recalcular_ranking_1v1()`.

#### 3) Función `recalcular_ranking_1v1()` (security definer): desde cero, en orden de `fin`, solo `estado_ranking = 'valido'`
- Elo inicial 1000. Esperado E = 1 / (1 + 10^((R_rival − R)/400)). S = 1 / 0,5 / 0. K = 40 si el jugador lleva menos de 10 partidos válidos; si no, 24.
- Multiplicador por diferencia de goles N: 1 si N ≤ 1, 1,5 si N = 2, (11+N)/8 si N ≥ 3. Delta = round(K · G · (S − E)).
- **Los penales cuentan como empate** (Elo, PE y Pts). Se muestran aparte.
- Pts 3/1/0. GF/GC son los goles del lado. Racha = letra del último resultado + cuántos seguidos. `ultimos` = 5 últimos.
- Borra y reescribe `ranking_1v1` en una transacción. Usa un bloqueo consultivo para que dos recálculos no se pisen.
- Vista pública `ranking_1v1_publico`: une con `perfiles` (nombre, avatar, país) y añade `dg = gf - gc` y `provisional = pj < 5`. Ordena por elo desc, luego pj desc.

#### 4) Función edge `phoenix` · `POST /v1/eventos`
- Por cada evento aceptado con `tipo = 'partida_fin'` y `datos.v = 2`, llama a `registrar_partido_link(sala_id, clave, ocurrido, <usuario del token>, datos)`.
- **Si falla el registro, el evento se acepta igual**: se loguea y no va a `rechazados`. Si no, Link lo descartaría o lo reintentaría para siempre.
- Duplicados (misma `clave`): no se vuelve a registrar.
- No cambia nada publicado. Sube el contrato a **1.11.0** (cambio compatible) y documenta los `datos` v2 de `partida_fin` en §6.

#### 5) Página de la web
- «Ranking 1v1»: posición, avatar, nombre, Elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5. Los provisionales van marcados o en una pestaña aparte.
- En el perfil de un jugador: sus últimos partidos 1v1 (rival, marcador, penales, fecha, válido o por revisar).
- Staff: lista de partidos «por revisar» con sus `motivos`, y botones Aprobar → `valido` o Excluir → `excluido`, que guardan `revisado_por` y `revisado_en` y recalculan.

### Criterios de aceptación
1. Un `partida_fin` v2 1v1 completo, con los dos jugadores vinculados y presentes, deja una fila `valido` en `partidos_link`, 2 jugadores y N goles con `usuario_id`, y actualiza `ranking_1v1` (cada uno con PJ 1).
2. Reenviar el mismo evento (misma `clave`) u otro con el mismo `(sala_id, partido_id)` no duplica nada ni cambia el Elo.
3. Un `partida_fin` sin `v` (Link viejo) se acepta como evento y no crea partido.
4. `modo: "equipos"` se guarda con sus jugadores y goles por lado, pero sin `usuario_id` en los goles ni `goles` por jugador, y no toca el ranking.
5. `cierre: "manual"` o `correcciones > 0` → `revisar`, y no cuenta hasta que el staff lo apruebe. Al aprobarlo, el ranking se recalcula.
6. El mismo usuario en los dos lados, un jugador sin cuenta o `duracion_seg < 180` → `excluido` con su motivo.
7. El 4.º partido válido de la misma pareja en 24 h queda en `revisar` con `farmeo`.
8. Un empate a goles con penales 4-3 suma un empate a los dos (PE +1, Pts +1).
9. Un error dentro de `registrar_partido_link` no hace fallar `POST /v1/eventos` ni manda el evento a `rechazados`.
10. La vista pública no expone `datos` en bruto ni `parsec_id`.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK con la versión del contrato (1.11.0) para quitar el `PENDIENTE-WEB` de `COORDINACION.md`.

---

## Parte 6 · Sistema de jugadores: estadísticas, ranking, rangos, prestigio y logros

> Origen: `docs/SISTEMA-JUGADORES.md` · último cambio 142c11c 2026-10-11 · 10125 bytes. Copia íntegra.

Estado: **DISEÑADO, SIN APLICAR** (decisión de Fralex, 2026-10-11).
- Link: el recolector de fichas está hecho y compila, pero **apagado** (`kFichaJugadorActiva = false` en `SmashSoda/phoenix/web/EstadisticasPartido.h`). Mientras esté apagado, `partida_fin` sale igual que con el ranking 1v1.
- Web: `docs/PROMPT-WEB-sistema-jugadores.md` está escrito, **sin enviar**. Va después de `docs/PROMPT-WEB-ranking-1v1.md`.
- Base: `docs/RANKING-1V1.md` (flujo, `partida_fin` v2, reglas de validez y Elo).

### 1. Hasta dónde se puede llegar (el techo)

| Nivel | Fuente | Qué da, por persona | Fiabilidad | Estado |
|---|---|---|---|---|
| 1a | Partido de Link + `phoenix_estadio.lua` (`match.stats()`) | Resultado, goles del lado (1v1 = suyos), minuto de cada gol, penales, equipo del juego, rival, cómo terminó, duración | Alta | **Se envía ya** (`partida_fin` v2) |
| 1b | Anfitrión de Parsec (Link) | Su mando, pulsaciones, pulsaciones por minuto, Start pulsados, inactividad, ping medio y máximo, segundos con lag, caídas, tiempo fuera | Alta (Link lo ve todo) | **Hecho, apagado** (ficha) |
| 2 | Sider sin tocar memoria | Nada más: `match.stats()` no da goleador, tarjetas, tiros ni posesión | — | Techo del modo seguro |
| 3 | Leer la memoria de PES (Sider `memory` / ReadProcessMemory, como `phoenix.lua`) | Lo que PES muestra al final del partido: goleador y asistente, tiros, posesión, pases, faltas, tarjetas y qué futbolista lleva cada mando | Media: depende de direcciones de memoria que cambian con cada versión | **Solo investigación** (§7) |

Límites que ningún nivel supera:
1. Solo existen los partidos hospedados con Phoenix Link y con `phoenix_estadio.lua` instalado en la PC del anfitrión.
2. Solo cuenta quien tenga su Parsec vinculado a su cuenta de la web.
3. Los datos salen de la PC del anfitrión. Se puede dificultar y vigilar la trampa, pero no eliminarla del todo.
4. Cada evento admite 4 KB. Hoy cabe, porque Link quita primero la lista de goles y luego las fichas. El nivel 3 necesitaría una ruta nueva en la web.
5. En partidos con varios por lado, los goles no se atribuyen a personas hasta tener el nivel 3 (otra sesión investiga qué mando controla a cada futbolista).

### 2. Ficha por jugador y partido (nivel 1b)

`EstadisticasPartido.cpp` mira la sala 20 veces por segundo, y solo con el partido **en juego** (la pausa no cuenta). Se guarda por `parsecId` y se añade a cada jugador de `partida_fin` como `ficha`:

```json
"ficha": {
  "mando": 1, "cambios_mando": 0,
  "seg_mando": 640,            // segundos en juego con mando
  "seg_fuera": 0, "caidas": 0, // fuera de la sala
  "pulsaciones": 2890, "pulsaciones_min": 271,
  "start": 1,                  // veces que pulsó Start
  "seg_inactivo": 35, "inactivo_max_seg": 9,   // nada pulsado y sticks en reposo
  "ping_medio": 48, "ping_max": 130, "seg_lag": 0   // solo invitados (> 150 ms)
}
```
- Solo lee lo que recibe el mando virtual. No toca mandos, sala ni juego.
- Una pulsación de menos de 50 ms se puede perder, así que las pulsaciones son aproximadas. Sirven para comparar, no como cifra exacta.
- No se guarda nada en bruto: solo el resumen. Hay que avisar a los jugadores de que se registra su actividad (página de privacidad de la web).

### 3. Identidad: ¿obligar a entrar a la sala por la web?

**Recomendación: no obligar a entrar siempre por la web. Sí obligar a vincular el Parsec una vez para que cuenten las estadísticas.**

- El ID de Parsec no cambia. Una vez vinculado en la web (`cuentas_parsec`), Link y la web saben quién es en cualquier sala, entre por donde entre. Pasar cada vez por la página no añade seguridad y sí molestias: amigos que se pasan el enlace, reconexiones a mitad de partido, gente que solo quiere mirar.
- Los partidos de quien no esté vinculado se guardan igual, pero no puntúan (motivo `sin_cuenta`). Si se vincula después, el staff puede reasignarlos.
- **HECHO (2026-10-11, apagada por defecto): «Sala clasificatoria»** (Partido › Armar; `SmashSoda/phoenix/web/SalaClasificatoria.*`, acción `partido.clasificatoria`, cuenta según `/v1/perfiles` con `PhoenixLink::cuentaWeb()`). Quien no tiene cuenta y toma un mando vuelve a «Mirando» (`liberarMando`) con un aviso en el chat (como mucho uno por minuto). `partido.preparar` no deja meterlo en un lado. Un partido ya empezado no se toca: solo se avisa de que no puntuará. Quien aún no se sabe puede jugar. Si alguien se vincula estando en la sala, Link lo nota en ≤ 10 min (caché de perfiles) o al volver a entrar. Diseño original: Un interruptor del anfitrión con dos efectos:
  1. Solo quien tenga cuenta vinculada puede tomar un mando. Los demás entran como espectadores y reciben un aviso con el enlace para vincularse.
  2. Al empezar, Link avisa en el chat si alguien no puntuará.

  Esto se haría en los puestos de mando (`PhoenixRoles`), en una sesión aparte, porque esos archivos los lleva otra. Con eso se consigue lo que buscas (que cuente todo de cada uno) sin obligar a nadie a pasar por la web cada vez.
- La entrada por la web (radar, «Ver», rival aceptado) sigue existiendo y es la recomendada para retos y torneos.

### 4. Qué se construye en la web con estos datos

#### 4.1 Ranking (ya diseñado)
Elo, PJ, PG, PE, PP, GF, GC, DG, puntos y racha. Reglas en `RANKING-1V1.md` §4.

#### 4.2 Rangos (según el Elo de la temporada)
| Rango | Elo |
|---|---|
| Bronce | < 900 |
| Plata | 900 – 1049 |
| Oro | 1050 – 1199 |
| Platino | 1200 – 1349 |
| Diamante | 1350 – 1499 |
| Leyenda | ≥ 1500 |
| Élite (insignia) | top 10 de la temporada, con 20 o más partidos |
Con menos de 5 partidos válidos: «Sin clasificar».

#### 4.3 Prestigio (XP que nunca baja)
- Por partido válido: +10 jugar, +5 ganar, +2 empatar. Racha de victorias: +3 por cada victoria desde la 3.ª (máximo +15). Partido limpio (ver fair play): +3.
- Nivel = floor(√(XP / 25)) + 1. Ejemplo: 100 XP → nivel 3; 2500 XP → nivel 11.
- **Estrellas de prestigio**: una por temporada jugada con 20 o más partidos, y otra extra por terminar en Diamante o mejor.
- Por qué existe aparte del Elo: el Elo mide lo bueno que eres y puede bajar; el prestigio premia jugar y ser constante, y nunca se pierde.

#### 4.4 Fair play (0–100, empieza en 100)
| Hecho (de `partida_fin` y la ficha) | Efecto |
|---|---|
| Partido abandonado (`abandono_juego`, o `presente: false` al final) | −15 |
| Caída durante el partido (`caidas`) | −3 cada una (máximo −9 por partido) |
| Inactivo 60 s o más seguidos (`inactivo_max_seg`) | −5 |
| 4 o más Start en un partido 1v1 (`start`) | −2 |
| Partido limpio (completo, sin caídas, sin inactividad larga) | +2, hasta 100 |
Por debajo de 60: marca visible y el staff revisa sus partidos. Por debajo de 40: sus partidos van a «revisar» automáticamente.

#### 4.5 Logros (insignias)
Primer partido · Primera victoria · Triplete (3 goles o más en un 1v1) · Goleada (ganar por 4 o más) · Portería a cero · Remontada (ir perdiendo por 2 y ganar, según `juego.goles`) · Gol en el descuento (minuto ≥ 90) · Racha 5 / Racha 10 · Invicto 10 · Matagigantes (ganar a alguien con 200 o más de Elo por encima) · Rey de los penales (5 tandas ganadas) · Fiel a su club (50 partidos con el mismo equipo) · Trotamundos (10 equipos distintos) · Centenario (100 partidos) · Juego limpio (20 partidos limpios seguidos) · Incansable (300 o más pulsaciones por minuto de media en 10 partidos).

#### 4.6 Temporadas
Una temporada dura lo que decida el staff (por ejemplo, un mes o «Temporada 1»). Al cerrarla se guarda una foto del ranking (posición, rango y Elo) y se hace un reinicio suave: Elo = 1000 + (Elo − 1000) / 2. El prestigio y los logros no se reinician.

#### 4.7 Ficha pública del jugador y cara a cara
- **Ficha:** rango, Elo, nivel y estrellas, fair play, logros, últimos 10 partidos, equipo favorito, goles por partido, % de victorias, ping medio y pulsaciones por minuto de media.
- **Cara a cara (A vs B):** partidos, victorias de cada uno, empates, goles y último resultado.

### 5. Contrato de Link (cuando se encienda)
- Sin cambios de endpoint: todo va en `partida_fin` v2. Cada `a/b.jugadores[]` gana `ficha` (opcional).
- Orden de recorte si `datos` pasa de 3,8 KB: primero `juego.goles`, después las `ficha`. El resultado siempre llega.

### 6. Cómo se activa (en este orden, cuando Fralex lo diga)
1. Chat WEB: aplicar `PROMPT-WEB-ranking-1v1.md`, y después `PROMPT-WEB-sistema-jugadores.md`.
2. Link: poner `kFichaJugadorActiva = true` en `EstadisticasPartido.h`, compilar y probar un 1v1 offline (comprobar que la ficha llega en el evento).
3. Instalar `phoenix_estadio.lua` (Módulos del juego) y probar con un amigo.
4. Quitar los `PENDIENTE-WEB` de `COORDINACION.md`.

### 7. Nivel 3 (memoria del juego): plan de investigación, sin implementar
Va contra «si algo se rompe, mejor no lo hagas» mientras no esté probado, así que solo se investiga:
1. Solo lectura y offline, nunca en partidos online. Con el mismo método de seguridad que `phoenix.lua`: comprobar los bytes antes de leer y apagarse solo tras varios errores.
2. Buscar en PES 2021 (la misma versión del ejecutable, con su sha256) las estructuras de estadísticas del partido: goles con autor, asistencias, tiros, posesión y tarjetas. Contrastarlas con la pantalla de estadísticas de 5 partidos reales.
3. Si sale bien, `phoenix_estadio.lua` escribiría `estadisticas.json` al final del partido y Link lo enviaría por una ruta nueva (pedírsela al chat WEB), porque no cabe en 4 KB.
4. La atribución por persona en partidos con varios por lado depende de saber qué mando controla a cada futbolista (lo investiga otra sesión).

### 8. Archivos
- `SmashSoda/phoenix/web/EstadisticasPartido.h/.cpp` (nuevos): recolector y `fichaJugador()`.
- `SmashSoda/phoenix/web/AccionesPartidoWeb.cpp`: `ladoWeb()` añade la ficha y recorta si no cabe.
- `SmashSoda/phoenix/web/InterfazWeb.cpp`: una línea, `tickEstadisticas(in)` después de `tickDatos(in)`.

---

## Parte 7 · PROMPT para el chat WEB · Sistema de jugadores (rangos, prestigio, fair play, logros, temporadas)

> Origen: `docs/PROMPT-WEB-sistema-jugadores.md` · último cambio 98172b7 2026-10-11 · 9279 bytes. Copia íntegra.

> **NO ENVIAR TODAVÍA** (Fralex: «aún no lo apliques»). Va **después** de `docs/PROMPT-WEB-ranking-1v1.md`: usa sus tablas (`partidos_link`, `partidos_link_jugadores`, `partidos_link_goles`, `ranking_1v1`) y su función `recalcular_ranking_1v1()`. Diseño completo: `docs/SISTEMA-JUGADORES.md` (repo `smash-soda-fork`, rama `rediseño-phoenix-portal`).

### Qué llega de Phoenix Link
El mismo `partida_fin` v2. Cuando Fralex encienda el sistema, cada `a/b.jugadores[]` traerá además `ficha` (opcional; si el evento no cabe en 4 KB, Link la quita):
```json
"ficha": { "mando": 1, "cambios_mando": 0, "seg_mando": 640, "seg_fuera": 0, "caidas": 0,
           "pulsaciones": 2890, "pulsaciones_min": 271, "start": 1,
           "seg_inactivo": 35, "inactivo_max_seg": 9,
           "ping_medio": 48, "ping_max": 130, "seg_lag": 0 }
```
`ping_*` y `seg_lag` no vienen para el anfitrión. Sin `ficha`, todo funciona igual: solo dejan de sumar los cálculos que dependen de ella.

### 1) Migración
```sql
-- Ficha por jugador y partido (columnas tipadas para poder consultar)
alter table public.partidos_link_jugadores
  add column ficha jsonb,
  add column seg_mando int, add column caidas int, add column seg_fuera int,
  add column pulsaciones int, add column pulsaciones_min int, add column starts int,
  add column seg_inactivo int, add column inactivo_max_seg int,
  add column ping_medio int, add column ping_max int, add column seg_lag int,
  add column equipo_juego int, add column nombre_equipo text;   -- de datos.juego.equipo_a/b

create table public.temporadas (
  id serial primary key, nombre text not null,
  inicio timestamptz not null, fin timestamptz,          -- fin null = temporada en curso (solo una)
  creado_en timestamptz not null default now()
);
create unique index temporadas_una_abierta on public.temporadas ((fin is null)) where fin is null;
alter table public.partidos_link add column temporada int references public.temporadas(id);

-- El ranking pasa a ser por temporada (la clave pasa a ser temporada + usuario)
alter table public.ranking_1v1 add column temporada int references public.temporadas(id);
alter table public.ranking_1v1 drop constraint ranking_1v1_pkey, add primary key (temporada, usuario_id);
-- (antes: crear la «Temporada 1» y rellenar temporada en las filas que ya haya)
create table public.ranking_1v1_historico (                -- foto al cerrar cada temporada
  temporada int not null references public.temporadas(id), usuario_id uuid not null,
  posicion int not null, elo int not null, rango text not null, pj int not null, pg int not null,
  pe int not null, pp int not null, gf int not null, gc int not null, pts int not null,
  primary key (temporada, usuario_id)
);

-- Progreso que nunca se reinicia
create table public.jugador_progreso (
  usuario_id uuid primary key,
  xp int not null default 0, nivel int not null default 1, estrellas int not null default 0,
  fair_play int not null default 100,
  partidos_limpios_seguidos int not null default 0,
  actualizado timestamptz not null default now()
);

create table public.logros (
  clave text primary key, nombre text not null, descripcion text not null, icono text, orden int not null default 0
);
create table public.jugador_logros (
  usuario_id uuid not null, logro text not null references public.logros(clave),
  partido bigint references public.partidos_link(id), conseguido_en timestamptz not null default now(),
  primary key (usuario_id, logro)
);
```
RLS: solo `service_role` escribe. El público lee **solo por vistas**, sin `parsec_id`, `datos` ni `ficha` en bruto.
- `jugador_publico`: nombre, avatar, rango, Elo, nivel, estrellas, fair play, logros y medias.
- `cara_a_cara(a uuid, b uuid)`: función o vista.
- `partidos_1v1_publico`.

Las medias de ping y pulsaciones se muestran **solo en la ficha del propio jugador y al staff**, no en público.

### 2) Al registrar un partido (`registrar_partido_link`, ampliar)
- Copiar `ficha` a las columnas tipadas y el equipo de cada lado (`juego.equipo_a`/`nombre_equipo_a` para A, y lo mismo para B).
- Asignar `temporada` = la abierta según `fin`.
- Reglas de validez nuevas (además de las de `ranking-1v1`):
  - jugador con `jugador_progreso.fair_play` < 40 → `revisar` (`fair_play_bajo`);
  - `inactivo_max_seg` ≥ 120 de un jugador → `revisar` (`inactivo`).
- Después de `recalcular_ranking_1v1()`, llamar a `recalcular_progreso(usuario)` para cada jugador del partido.

### 3) `recalcular_ranking_1v1()` por temporada
Igual que el diseño base, pero solo con los partidos `valido` de la temporada abierta. Al cerrar una temporada, `cerrar_temporada(nombre_nueva)` (solo staff):
- copia `ranking_1v1` en `ranking_1v1_historico`;
- da estrellas: +1 a quien jugó 20 o más partidos, +1 extra si terminó en Diamante o Leyenda;
- cierra la temporada (`fin = now()`) y abre la nueva;
- Elo inicial de la nueva para quien ya jugaba: 1000 + (Elo final − 1000) / 2. Guárdalo como semilla en el recálculo.

### 4) Rango (función `rango_de(elo int, pj int)`)
`pj < 5` → «Sin clasificar». Si no: < 900 Bronce · 900–1049 Plata · 1050–1199 Oro · 1200–1349 Platino · 1350–1499 Diamante · ≥ 1500 Leyenda. «Élite» es una insignia aparte: top 10 de la temporada con 20 o más PJ.

### 5) `recalcular_progreso(usuario)` desde cero, en orden de `fin`, solo partidos `valido`
- **XP:** +10 por jugar, +5 por ganar, +2 por empatar. +3 por cada victoria seguida desde la 3.ª, con un máximo de +15 por partido. +3 si el partido fue limpio. Nivel = floor(sqrt(xp/25)) + 1.
- **Fair play** (sobre **todos** los partidos 1v1 del jugador, también `revisar`; no los `excluido` por `cancelado`). Empieza en 100 y se limita a 0–100:
  - abandono (`abandono_juego`, o él con `presente = false`): −15;
  - −3 por caída (máximo −9 por partido);
  - `inactivo_max_seg` ≥ 60: −5;
  - `starts` ≥ 4: −2;
  - partido limpio: +2.
  - Limpio = `cierre = 'completo'`, sin caídas y con `inactivo_max_seg` < 60.
- **Logros** (insertar si no existen, con el partido que lo dio):

| clave | regla |
|---|---|
| `primer_partido` | 1 PJ válido |
| `primera_victoria` | 1 PG |
| `triplete` | 3 goles o más propios en un 1v1 |
| `goleada` | ganar por 4 o más |
| `porteria_cero` | ganar sin recibir goles |
| `remontada` | según `partidos_link_goles` en orden, ir perdiendo por 2 o más y ganar |
| `descuento` | marcar un gol en el minuto 90 o después |
| `racha_5`, `racha_10` | victorias seguidas |
| `invicto_10` | 10 partidos seguidos sin perder |
| `matagigantes` | ganar a un rival con 200 o más de Elo por encima en ese momento (guarda el Elo previo de cada partido en el recálculo del ranking) |
| `rey_penales` | 5 tandas ganadas |
| `fiel_club` | 50 PJ con el mismo `equipo_juego` |
| `trotamundos` | 10 `equipo_juego` distintos |
| `centenario` | 100 PJ |
| `juego_limpio` | 20 partidos limpios seguidos |
| `incansable` | media de `pulsaciones_min` ≥ 300 en sus últimos 10 PJ con ficha |

Carga el catálogo `logros` con esos nombres en español, una descripción corta y un icono.

### 6) Páginas
- **Ranking:** selector de temporada (en curso e históricas), rango con su color, Elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5.
- **Ficha del jugador:**
  - pública: rango, Elo, nivel, barra de XP, estrellas, fair play, logros (conseguidos en color, los demás en gris), últimos 10 partidos, equipo favorito, % de victorias y goles por partido;
  - privada (el propio jugador y el staff): ping medio y pulsaciones por minuto medias.
- **Cara a cara:** elegir dos jugadores → PJ, victorias de cada uno, empates, goles y últimos 5 enfrentamientos.
- **Staff:**
  - partidos por revisar, con motivos y botones Aprobar o Excluir;
  - lista de jugadores con fair play < 60;
  - cerrar temporada;
  - reasignar partidos de un `parsec_id` a una cuenta que se vinculó después (re-resolver `usuario_id` y recalcular).
- **Privacidad:** una línea en la página de privacidad: «En las salas de Phoenix Link se registra, resumida por partido, tu actividad con el mando (pulsaciones, inactividad) y tu conexión (ping, caídas) para el ranking y el fair play».

### Criterios de aceptación
1. Un partido válido con `ficha` llena las columnas tipadas, suma XP (jugar + ganar) a los dos y recalcula nivel y rango.
2. Sin `ficha`, el partido cuenta igual para el ranking, la XP y los logros que no dependen de ella.
3. Un abandono baja 15 de fair play al que se fue. Con fair play < 40, sus siguientes partidos entran como `revisar`.
4. Cerrar la temporada guarda la foto, da las estrellas, hace el reinicio suave del Elo y empieza un ranking vacío de la nueva temporada. La XP, el nivel y los logros no cambian.
5. Los logros no se duplican, y recalcular desde cero da el mismo resultado (idempotente).
6. Ninguna vista pública expone `parsec_id`, `datos`, `ficha`, ping ni pulsaciones de otros.
7. Reasignar un `parsec_id` a una cuenta recalcula ranking y progreso, y los partidos de esa persona pasan a contar si cumplen las reglas.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK para que encienda `kFichaJugadorActiva`.

---

## Parte 8 · PROMPT para el chat WEB / Sync · Lista de fichajes como cambios

> Origen: `docs/PROMPT-WEB-fichajes-lista.md` · último cambio 442861a 2026-10-12 · 3607 bytes. Copia íntegra.

> Pégalo en el chat WEB o en el de Phoenix Sync. Origen: chat LINK, 2026-10-12. Diseño completo: `docs/FICHAJES-COMO-CAMBIOS.md` en `smash-soda-fork` (rama `rediseño-phoenix-portal`). **Link ya está hecho (0.2.46); lo que falta es solo de la web/Sync.**

### Qué quiere Fralex
Que los fichajes viajen como una **lista corta de cambios** y no como una base de datos completa: Phoenix Link los aplica en cada PC sobre la base REAL de ese PC (ConmeGOL 26, 27, UEFAGOL 27, Clásicos…). Así no se distribuye ningún `Player.bin` ni `PlayerAssignment.bin` de otros modders y los fichajes funcionan sobre cualquier parche.

### Qué hace Link hoy (no hay endpoint nuevo)
Link lee `%APPDATA%\Phoenix Mercado\fichajes\fichajes.json`. Si no existe usa el que lleva Link (vacío). El botón «Aplicar fichajes de Phoenix» (SYNC › Puente) genera el `PlayerAssignment.bin` de Phoenix-DB sobre la base de cada modo.

### Lo que tiene que hacer Sync / la web
1. Dejar ese archivo en esa carpeta (escritura atómica: `.tmp` y renombrar), con este formato:
```json
{
  "version": 1,
  "regla": "relajada",
  "nombre": "Mercado 2026-10-12",
  "cambios": [
    { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto corto opcional" }
  ]
}
```
2. Reglas del formato: `version` debe ser 1 o 2 (Link rechaza otras; la 2 es compatible con la 1 y solo añade nombres, ver el punto 2b); `regla` «relajada» (gana la lista si el jugador ya no está en `origen`) o «estricta» (se salta); `player_id` y `team_id` son enteros > 0; `origen` (opcional) es el `team_id` donde se espera al jugador, necesario si el jugador sale en dos equipos (club y selección); como máximo 5.000 cambios; solo altas y movimientos (no hay bajas).
2b. **Versión 2 (opcional): por nombre en vez de por id.** Los `player_id`/`team_id` cambian entre parches, así que, cuando se pueda, es mejor mandar nombres, que Link resuelve contra la base de cada PC con coincidencia EXACTA (sin mayúsculas, acentos ni signos) y, si hay duda, **salta** el cambio y dice por qué:
```json
{ "version": 2, "cambios": [
  { "jugador": "Cristiano Ronaldo", "equipo_origen": "Al-Nassr FC", "equipo_destino": "Real Madrid CF" },
  { "player_id": 114506, "equipo_destino": "FC Barcelona" } ] }
```
   Cada campo va por id **o** por nombre (no los dos). `jugador` = nombre completo tal como sale en el juego; `equipo_destino`/`equipo_origen` = nombre del equipo como sale en el juego (p. ej. «Real Madrid CF», «FC Barcelona»). Si el nombre de un jugador sale varias veces, hay que dar `equipo_origen` (o `player_id`). El mismo archivo vale para todos los parches; en cada PC Link aplica solo lo que existe allí.
3. Los ids son los de PES 2021 (`player_id` de Player.bin, `team_id` de PlayerAssignment.bin). Un cambio que apunte a un jugador o equipo que no existe en la base de ese PC **se salta con motivo** (nada se rompe), y Link muestra «N aplicados, M saltados».
4. No hace falta que Sync sepa qué parche tiene el amigo: la lista es la misma para todos; Link decide qué se aplica en cada modo.
5. (Opcional, más adelante) un estado de vuelta: Link guarda `fichajes_aplicados.json` dentro de `SiderAddons\livecpk\Phoenix-DB` de cada modo (fecha, huella de la lista, aplicados/saltados). Si Sync quiere mostrarlo, hay que acordar cómo lo recoge; hoy no se envía a la web.

### Qué NO hay que hacer
- No mandar bases completas, ni `Player.bin`, ni `PlayerAssignment.bin` ya hechos.
- No escribir dentro del juego: eso lo hace Link, con PES cerrado y copia de seguridad.

---

## Parte 9 · PROMPT para el chat WEB · Tablero del partido al tocar «En vivo ahora» (campo 2D + estadísticas)

> Origen: `docs/PROMPT-WEB-tablero-partido.md` · último cambio ed9a255 2026-10-12 · 7141 bytes. Copia íntegra.

> **HECHO (2026-10-12, hecho por el chat LINK en `pes-liga-automatica`, rama `borrador`, commit `39658cb`, ronda 236):** Edge `liga` 1.3.0 desplegada (v4) con `POST /v1/liga/campo` (Broadcast `campo:<sala_id>`, sin escribir en la base) y tablero desplegable en las tarjetas de «En vivo» (campo, posesión, penales, fases REPETICIÓN/PAUSA). Este prompt queda solo como referencia; no hay nada que pedirle al chat web.

> Repo `Fralexito/phoenixevolution`, rama `borrador`. Origen: chat LINK, 2026-10-11.
> Fralex: «en el overlay no tiene que aparecer el campo 2D, debe aparecer en la web: por encima sale la tarjeta como ahora y, al darle clic, se abre el tablero con las demás estadísticas del partido».

> **CAMBIO (Fralex, 2026-10-12): «donde se verán los resultados del partido en tiempo real, ponlo en la sección de En vivo».** El tablero NO es un modal suelto: vive **dentro de la sección «En vivo»** de la web (la página/pestaña donde hoy salen las tarjetas de «En vivo ahora»). Concretamente:
> - En la sección «En vivo», cada partido en juego se muestra como la tarjeta de siempre; al pulsarla **se despliega en el mismo sitio** (acordeón o panel a ancho completo de esa sección, sin salir de la página) con el tablero: cabecera, campo 2D, posesión, goles y penales. Se puede plegar otra vez.
> - Si solo hay **un** partido en vivo, el tablero aparece **ya desplegado** al entrar en la sección (sin tener que pulsar); con varios, desplegado el primero y los demás plegados.
> - Todo se actualiza en tiempo real en esa misma sección (marcador, minuto, goles y campo). En el móvil, el panel ocupa el ancho y el campo se adapta.
> - Se mantiene lo demás de este prompt (ruta `POST /v1/liga/campo`, canal Broadcast, suscripción solo con el panel desplegado, rendimiento y criterios). Donde abajo dice «modal / hoja a pantalla completa», léase «panel desplegable dentro de la sección En vivo».

### Qué ya hace Phoenix Link (rama `rediseño-phoenix-portal`)
- La tarjeta sigue igual: `liga_vivo` vía `POST /v1/liga/vivo` (marcador, minuto, fase, pantalla, goles con goleador).
- **Nuevo:** con «Campo en vivo en la web» encendido en Link (Partido › CAMPO, apagado por defecto) y el partido en juego en una sala que ya sale en «En vivo ahora», Link manda **como mucho 1 vez por segundo**:
  ```
  POST {base}/functions/v1/liga/v1/liga/campo     (mismo token y cabeceras que /v1/liga/vivo)
  { "sala_id": "uuid", "v": 1,
    "j": [[x, z, eq], …],          // 20–25 fichitas: metros, centro = (0,0); x ∈ [-52.5, 52.5] (largo), z ∈ [-34, 34] (ancho)
                                    // eq: 0 = equipo A, 1 = equipo B, 2 = árbitro/asistente, -1 = aún sin equipo
    "b": [x, z, y] | null,         // balón (y = altura en m); null si no se ve
    "t": { "l": [[eq, gol], …], "e": eq, "listo": bool, "vuelo": bool },   // solo durante una tanda de penales
    "equipos": true,               // false = los colores aún no se saben (llegan en el siguiente saque de centro)
    "posesion": [55.3, 44.7] }     // % equipo A / B (aproximada: quién está más cerca del balón); falta los primeros segundos
  ```
  ~0,5–1 KB por mensaje. Si la ruta no existe (404), Link espera 5 min antes de volver a probar.
- **Equipo A/B frente a local/visita:** A = el equipo que estaba a la **izquierda** en el primer saque de centro que vio Link. No se ha comprobado que eso sea siempre el local (otra sesión lo está investigando). En el tablero, pon a A como local y a B como visita, con una nota pequeña «colores aproximados», hasta que Link mande `"eq_local"` (vendrá en una versión posterior: si llega, úsalo).

### Qué hay que hacer

#### 1) Función `liga`: ruta `POST /v1/liga/campo`
- Autenticar igual que `/v1/liga/vivo` (`autenticar`, `salaDe`). Validar: `j` array ≤ 30 de `[num, num, int]`, `b` null o 3 números, cuerpo ≤ 4 KB, `posesion` 2 números entre 0 y 100.
- Límite: 2 por segundo por sala (más → `429` con `reintentar_en`).
- **No escribir en la base.** Reenviar por **Realtime Broadcast** al canal `campo:<sala_id>`, evento `campo`, con el cuerpo sin `sala_id`. Desde la Edge Function: `sb.channel('campo:' + sala).send({ type: 'broadcast', event: 'campo', payload })` (supabase-js v2; usa el envío HTTP sin suscribirse) o la REST `POST {SUPABASE_URL}/realtime/v1/api/broadcast` con la service key.
- Respuesta `{ ok: true }`. Contrato: documentarla en `docs/LIGA-AUTOMATICA.md`; sube la versión de la función.
- Canal **público de solo lectura** (cualquiera que vea la tarjeta puede mirar el tablero; son datos del partido, sin nombres de cuentas ni Parsec).

#### 2) Tarjeta → tablero (dentro de la sección «En vivo»)
- `src/components/MarcadorVivo.astro` y `src/js/features/liga/vivo.js`: toda la tarjeta se puede pulsar (y con teclado: `role="button"`, Enter/Espacio). Abre un **tablero** (modal en escritorio, hoja a pantalla completa en el móvil; cerrar con ✕, Esc o deslizando) con:
  1. **Cabecera** igual que la tarjeta: tipo (amistoso u oficial), equipos, marcador, minuto y fase o pantalla, en vivo con la misma suscripción a `liga_vivo`.
  2. **Campo 2D en vivo:** SVG del campo (105 × 68 con líneas, áreas y círculo central). Las fichitas con los colores del equipo A, el B y los árbitros, y el balón un poco más grande (si `y > 2`, con sombra). Interpola entre mensajes (~1 s) para que se mueva suave, sin saltos.
  3. **Posesión:** barra A/B con los porcentajes (o «calculando…» si aún no llega).
  4. **Goles:** línea de tiempo con el minuto, el goleador y el lado (de `liga_vivo.goles`).
  5. **Tanda de penales** si llega `t`: fila de círculos por equipo (gol / fallo / pendiente).
  6. Si en 5 s no llega ningún `campo`: «El campo en vivo no está activado en esta sala» (el resto del tablero sigue funcionando).
- Suscribirse a `campo:<sala_id>` **solo con el tablero abierto**, y desuscribirse al cerrarlo. Si la fila de `liga_vivo` se borra (el partido terminó), mostrar «Partido terminado» con el marcador final y no cerrar de golpe.
- Rendimiento: un solo `requestAnimationFrame` para la interpolación; nada de re-render por cada mensaje si el tablero está oculto.

### Criterios de aceptación
1. La tarjeta se ve igual que hoy y, al pulsarla, se despliega el tablero dentro de la sección «En vivo» (escritorio y móvil); con un solo partido en vivo ya sale desplegado.
2. Con Link enviando, las fichitas y el balón se mueven suaves (1 mensaje/s interpolado) y la posesión se actualiza.
3. `POST /v1/liga/campo` no escribe en la base (comprobar que no crece ninguna tabla) y rechaza a quien no es dueño de la sala.
4. Sin campo (Link viejo o interruptor apagado), el tablero muestra la cabecera, los goles y el aviso.
5. Cerrar el tablero cierra la suscripción al canal.
6. Accesible: foco atrapado dentro del modal, Esc cierra, contraste AA.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK (para quitar el `PENDIENTE-WEB` y, si hace falta, ajustar el formato).

---

## Parte 10 · PROMPT para el chat WEB · Traspasos en vivo — **OBSOLETO: implementado directamente**

> Origen: `docs/PROMPT-WEB-traspasos-vivo.md` · último cambio cdd4b5f 2026-10-12 · 2245 bytes. Copia íntegra.

> **No hace falta pegar nada.** Fralex pidió que la parte web se hiciera desde el chat principal de Link: la función **`traspasos`** (migración 105, rama `borrador` de `pes-liga-automatica`, commit `bddb627`) ya está **desplegada** y probada contra la web real (6 rutas, 29 comprobaciones). El contrato vigente está en `C:\dev\pes-liga-automatica\docs\TRASPASOS-EN-VIVO.md` y resumido en `docs/TRASPASOS-EN-VIVO.md` (§3) de este repo.
>
> La versión anterior de este prompt (canal por anfitrión, aprobación de seguidores, rutas `/v1/traspasos/*` dentro de «liga») **ya no vale**: el modelo real es **código de 8 caracteres por anfitrión aprobado**, seguir con el código (`prueba:true` para seguirse a uno mismo), `seq` global y la función aparte `traspasos`.

### Contrato vigente (resumen)
Base: `https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/traspasos/v1` · `Authorization: Bearer phx_…` + `X-Phoenix-Version` + `X-Phoenix-Build` · respuestas `{ok:true,…}` o `{ok:false,codigo,mensaje,reintentable,reintentar_en?}`.

| Ruta | Quién | Cuerpo | Respuesta |
|---|---|---|---|
| `POST /traspasos` | anfitrión (host aprobado) | `{id 24hex, huella 64hex, creado ISO, autor≤40, parche≤60, base_huella≤64, cambios[1–60]{jugador_id>0, jugador≤48, desde_id≥0, desde≤48, hasta_id>0, hasta≤48}}` | `{seq, repetido}` (idempotente por `(anfitrión,id)`); 12/min; ≤32 KB |
| `GET /traspasos?desde=<seq>&limite=<1-50>` | amigo | — | `{lotes:[{seq,id,anfitrion,anfitrion_id,huella,creado,autor,parche,base_huella,cambios}],ultimo_seq}` (los de quien sigue y los propios); 40/min |
| `POST /traspasos/codigo` | anfitrión | `{rotar?}` | `{codigo,seguidores:[{nombre}]}` |
| `POST /traspasos/seguir` | amigo | `{codigo,prueba?}` | `{anfitrion,anfitrion_id}` |
| `POST /traspasos/dejar` | amigo | `{anfitrion_id}` | `{}` |
| `GET /traspasos/siguiendo` | amigo | — | `{sigo:[{anfitrion_id,nombre}],me_siguen:[{nombre}]}` |

Errores propios: `CODIGO_INVALIDO` 404 · `SEGUIR_A_UNO_MISMO` 409 · `HOST_NO_AUTORIZADO` 403. Los de siempre: `TOKEN_*` 401, `DEMASIADOS_INTENTOS` 429, `LIGA_PAUSADA` 503, `RUTA_NO_EXISTE` 404.

---

## Parte 11 · Contrato WEB ↔ PES · «Cambios oficiales» (v1)

> Origen: `docs/CONTRATO-WEB-PES.md` · último cambio 5ae6b2b 2026-10-17 · 14606 bytes. Copia íntegra.

Estado: **v1 publicado el 17-oct-2026** (sesión BASE, migración 125). Copia idéntica en los dos repos:
`smash-soda-fork/docs/CONTRATO-WEB-PES.md` (rama `rediseño-phoenix-portal`) y `phoenixevolution/docs/CONTRATO-WEB-PES.md` (rama `borrador`).
Si cambias algo, cambia **las dos copias** y sube `version` si un PC viejo dejaría de entenderlo.

Pedido de Fralex: «que cualquier cambio que haga en mi web se refleje en el PES y que tenga todo el registro e historial de todos los
usuarios que usan mi sistema en sus PES», con **un solo formulario** donde cada cambio se aplica a ☐ Base normal (amistosos) ☐ Liga Máster ☐ Las dos.

Quién hace qué:
| Parte | Sesión | Migración |
|---|---|---|
| Formulario único, cambio oficial, base normal (amistosos) por el canal oficial de traspasos, registro de PCs | BASE (`ws-amistosos-*`) | **125** |
| Tipos `stats` e `info` (Player.bin) | web-pes-estadisticas | 126 |
| Destino `lm` (fichajes, dinero, valor y stats de Liga Máster) | web-pes-liga-master | 127 |

---

### 1. Qué es un «cambio»

Una fila de `public.pes_cambios`. **Un cambio = un solo registro en el historial**, aunque vaya a los dos destinos: si es `ambos`
tiene **dos resultados** (columna `estado_base` y columna `estado_lm`, más los resultados por PC en `pes_eventos` con `destino` = `base` o `lm`).

| Campo | Tipo | Qué es |
|---|---|---|
| `id` | uuid | Identidad del cambio. Nunca cambia. |
| `seq` | bigint | Orden global (1, 2, 3…). Los PCs leen «lo que venga después de mi último `seq`». |
| `version` | int | Versión de **este contrato** con la que se escribió (hoy `1`). |
| `tipo` | text | `fichaje` · `stats` · `info` · `dinero` · `valor` · … (lista viva en `public.pes_tipos_cambio`, ver §2). |
| `destino` | text | `base` · `lm` · `ambos`. |
| `datos` | jsonb | Lo que cambia; la forma depende del `tipo` (§2). |
| `autor` / `autor_nombre` | uuid / text | Quién lo hizo en la web (staff). |
| `creado` | timestamptz | Cuándo. |
| `firma` | text (64 hex) | `sha256("pes-cambio|v<version>|<id>|<tipo>|<destino>|<datos::text>")`. Integridad: si alguien toca `datos` a mano la firma deja de cuadrar. **No** es una firma criptográfica de clave privada: el canal Liga Máster puede añadir su firma Ed25519 propia (la de `mercado/v1`). |
| `estado` | text | `activo` · `anulado` (anulado = no se vuelve a mandar; lo ya aplicado se deshace con un cambio nuevo en sentido contrario). |
| `estado_base` | text | `no_aplica` · `pendiente` · `publicado` · `error`. |
| `base_lote` / `base_seq` | text / bigint | El lote del canal oficial de traspasos que lleva la parte base (§3). |
| `estado_lm` | text | `no_aplica` · `pendiente` · `tomado` · `publicado` · `error`. |
| `lm_ref` | text | Referencia que deja el motor de Liga Máster (su id o versión de cambios). |
| `nota` | text | Comentario libre del staff (≤ 300). |

Reglas:
- Se crea **solo** con la función `public.pes_cambio_crear(p_tipo, p_destino, p_datos, p_nota)` (staff `admin`; la web la llama con la sesión del usuario). Valida el tipo y que el destino esté permitido para ese tipo, calcula la firma y, si toca, publica la parte base en el mismo momento.
- `destino = base` → `estado_lm = no_aplica`. `destino = lm` → `estado_base = no_aplica`. `ambos` → los dos empiezan en `pendiente`.
- Nadie borra cambios: se anulan (`public.pes_cambio_anular(p_id, p_motivo)`).

### 2. Tipos y la forma de `datos`

Catálogo: `public.pes_tipos_cambio (tipo pk, etiqueta, destinos text[], motor text, activo bool)`. Cada sesión **añade o enciende su fila** en su migración
(`insert … on conflict (tipo) do update`). La 125 deja `fichaje` encendido (destinos `base`, `lm`, `ambos`) y reserva, **apagados**
(`activo = false`) hasta que su migración los encienda: `stats`, `info` (126; `base`, `lm`, `ambos`) y `dinero`, `valor` (127; solo `lm`).
Si el tipo **no** es `fichaje`, `pes_cambio_crear` solo guarda el cambio (y su parte base queda `pendiente` para el motor de ese tipo,
igual que la parte `lm`); cada motor marca su resultado con `public.pes_cambio_marcar(p_id, p_lado, p_estado, p_ref)` (§3b).

#### `fichaje` (v1)
```json
{ "jugador_id": 162114, "jugador": "Lamine Yamal",
  "desde_id": 101,  "desde": "FC Barcelona",
  "hasta_id": 102,  "hasta": "Arsenal FC",
  "lm": { "jugador_id": 57, "club_id": 3 } }
```
- Nombres: obligatorios (1–48 **bytes** UTF-8: Link los corta por bytes al leer y, si no, la huella no cuadra; una tilde cuenta doble). Es lo que de verdad usa cada PC, porque **la base de cada PC puede ser otra** (otro parche, otro PC).
- `jugador_id` / `desde_id` / `hasta_id`: el id de PES si se sabe (la web lo saca de `lm_jugadores.pes_id`). **Si no se sabe**, se pone un
  **id sintético** `90000000 + (número sacado del sha256 del nombre en minúsculas, entre 0 y 9.999.998)`; ids ≥ 90.000.000 significan «no es un id real, aplícalo por nombre».
  Lo calcula la función SQL `public.pes_id_sintetico(nombre)`: la web no tiene que hacerlo.
  `desde_id` puede ser `0` (sin equipo / agente libre).
- `desde` es **obligatorio** salvo agente libre: un jugador está a la vez en su club y en su selección y hay que decir de cuál sale (bug real de Sync, ver `PhoenixSync/REGISTRO.md`, mover_cli).
- `lm` (opcional): ids de la web de Liga Máster (`lm_jugadores.id`, `lm_clubes.id`) para el motor 127. La parte base lo ignora.

#### `stats`, `info`, `dinero`, `valor`
Los define cada sesión en este mismo archivo (añadir una sección `### <tipo> (v1)`), sin tocar lo de arriba.

#### `stats` y `info` (v1) — sesión 126 (estadísticas e información, Player.bin)
```json
{ "jugador_id": 162114, "jugador": "Lamine Yamal",
  "valores": { "velocidad": 95, "finalizacion": 90 },
  "antes":   { "velocidad": 90 },
  "deshace": "<uuid de otro cambio>",
  "lm": { "jugador_id": 57 } }
```
- `jugador` obligatorio (1–48 bytes). `jugador_id` opcional (id de PES, entero 1–99.999.999; ≥ 90.000.000 = sintético → por nombre).
  Cada PC usa el id **solo si en su base ese id tiene ese nombre**; si no, busca el nombre (único). Nunca toca a un jugador distinto.
- `valores`: 1–46 campos; claves y rangos en `public.pes_fichas_campos` (copia en Link `link/Fichas.cpp` y en la web `core/fichasCampos.js`).
  `stats` = 20 de campo + 5 de portero (40–99). `info` = pie malo uso/precisión (1–4), forma (1–8), resistencia a lesiones (1–3),
  13 posiciones `pos_*` (0 = C, 1 = B, 2 = A), `altura` (150–210), `peso` (45–110), `edad` (15–45), `posicion_registrada` (0–12: PT DFC LI LD MCD MC MI MD MO EI ED SD DC).
  Valor `null` = «vuelve al valor de la base del parche» (lo usa el deshacer).
- `antes` (opcional): lo que la web tenía antes, solo para el historial. `deshace` (opcional): este cambio deshace a otro (que queda `anulado`).
- La web valida los datos al crear (trigger de la 126) y deja `estado_base = 'publicado'`, `base_ref = 'v1/cambios'` al instante:
  la parte base **no** viaja por `liga_traspasos`, cada Link la baja de **§3c** (`GET /v1/cambios?destino=base`).
- **Cómo lo aplica cada Link (≥ 0.3.18):** junta todos los `stats`/`info` activos en orden de `seq` (el último valor de cada jugador+campo gana; `null` lo quita)
  y genera `Phoenix-DB\common\etc\pesdb\Player.bin` = Player.bin de **su** base + esa lista (solo cambian los bits de esos campos; copia antes; todo o nada).
  Quitar o deshacer = el campo vuelve al de la base. Informa por §4 con `destino: "base"` y `motivo` = «campo antes→después, …» (o el código del salto).
- **Destino `lm`:** el motor 127 lee `estado_lm = 'pendiente'` como para cualquier tipo. En Liga Máster las stats no salen de Player.bin: se suben/bajan
  con las barras de crecimiento de la partida (PhoenixSync, guías 24–29). Un `null` en `lm` = devolver el valor que 127 guardó como «antes» al aplicar el cambio de `deshace`.

### 3. Cómo lo baja cada PC

#### 3a. Destino `base` de un `fichaje` (amistosos) → canal OFICIAL de traspasos
Al crear el cambio, la web mete **un lote** en `public.liga_traspasos` con `oficial = true` y `cambio = <id del cambio>`. El lote tiene
el **formato de Link** (ver `docs/TRASPASOS-EN-VIVO.md` §3):
```json
{ "seq": 812, "id": "<24 hex>", "huella": "<sha256>", "creado": "2026-10-17T20:00:00Z", "autor": "Phoenix (oficial)",
  "parche": "", "base_huella": "",
  "anfitrion": "Phoenix (oficial)", "anfitrion_id": "00000000-0000-4000-8000-0000000f0e11",
  "oficial": true, "cambio_id": "<uuid del cambio>",
  "cambios": [ { "jugador_id": 162114, "jugador": "Lamine Yamal", "desde_id": 101, "desde": "FC Barcelona", "hasta_id": 102, "hasta": "Arsenal FC" } ] }
```
- `huella` = `sha256` de una línea por cambio `jugador_id|desde_id|hasta_id|jugador|desde|hasta\n` (la misma de Link, `huellaDeCambios`), `id` = primeros 24 hex de `sha256("oficial|<cambio_id>")`.
- `base_huella` **vacía a propósito**: así cada Link lo aplica **por nombres** sobre su propia base (vale en cualquier PC y parche). Nunca por ids.
- `anfitrion_id` es **fijo** (`00000000-0000-4000-8000-0000000f0e11`), no el del admin: así el PC del propio admin también lo aplica (no lo toma por «propio»).
- **Ruta:** la misma de siempre, `GET /functions/v1/traspasos/v1/traspasos?desde=<seq>&limite=<1-50>`, devuelve los lotes de quien sigues **más** todos los oficiales.
  Con `&solo_oficial=1` devuelve **solo** los oficiales (lo usa un Link con «Recibir de amigos» apagado). No hace falta código ni seguir a nadie:
  **todo PC vinculado sigue el canal oficial**. Un Link viejo que no conoce `oficial` lo aplica igual si ya recibe de amigos (es un lote normal).
- Link (≥ 0.3.17): interruptor «Recibir los cambios oficiales de Phoenix», **encendido por defecto**. Los oficiales no piden aprobación, no se
  pueden «dejar de seguir» y van a la Phoenix-DB (se quedan al cerrar PES), igual que un traspaso de amigo.

#### 3b. Destino `lm` (Liga Máster) → motor de la sesión 127 (y cualquier otro motor)
La 125 **no** manda nada al PC por este lado: deja `estado_lm = 'pendiente'`. El motor LM:
1. Lee los pendientes: `select * from public.pes_cambios where estado = 'activo' and estado_lm = 'pendiente' order by seq` (service role) o la ruta genérica de §3c.
2. Los traduce a su formato `lm_*` firmado y los publica por su canal (`/functions/v1/mercado/v1/liga/cambios`).
3. Marca el resultado con `public.pes_cambio_marcar(p_id, 'lm', p_estado, p_ref)` (`tomado` · `publicado` · `error`), solo service role o admin.
   El motor de stats/info (126) hace lo mismo con `p_lado = 'base'` para sus tipos.

#### 3c. Ruta genérica (para cualquier motor o un Link futuro)
`GET /functions/v1/traspasos/v1/cambios?desde=<seq>&destino=base|lm&limite=<1-100>` (token de PC `phx_…`, cualquier PC vinculado) →
`{ ok, cambios:[{seq,id,version,tipo,destino,datos,creado,firma,estado}], ultimo_seq }`. Solo cambios `activo` cuyo destino incluye el pedido.

### 4. Cómo informa cada PC (registro de usuarios)

`POST /functions/v1/traspasos/v1/pc/informe` (token `phx_…` + cabeceras `X-Phoenix-Version` / `X-Phoenix-Build`). Cuerpo (≤ 32 KB):
```json
{ "link_version": "0.3.17", "link_build": 317, "parche": "ConmeGOL 26", "base_huella": "a1b2c3d4e5f6",
  "juego": true, "pes_abierto": false, "oficial": true, "recibir": true, "publicar": false,
  "resultados": [ { "cambio_id": "<uuid>", "destino": "base", "resultado": "aplicado", "motivo": "", "jugadores": 1, "cuando": "2026-10-17T20:01:00Z" } ] }
```
- `resultado`: `aplicado` · `ya_estaba` · `parcial` · `saltado` · `error` · `deshecho` · `pendiente` · `por_aprobar` · `reemplazado`.
- Se manda: al arrancar, cada ~10 min (latido) y en cuanto cambia el resultado de un lote oficial. Máx. 50 resultados por informe.
- **Ningún dato personal ni ruta de disco**: versión, parche, huella corta de la base (12 hex) y resultados. El usuario ya es el dueño del token.
- La web (función SQL `public.pes_registrar_informe(p_dispositivo, p_usuario, p_informe)`, solo `service_role`):
  - `public.pes_pcs` — una fila por PC (`dispositivo` = `dispositivos_host.id`): usuario, versión, build, parche, huella de base, último contacto, primer contacto, contadores aplicados/saltados/errores.
  - `public.pes_eventos` — historial **solo-añadir**: una fila por cada `(PC, cambio, destino)` cuyo resultado **cambió** (repetir el mismo resultado no añade fila), más eventos `contacto` cuando cambia la versión, el parche o la base del PC.
  - Contesta `{ ok, guardados, ignorados }`. Resultados con `cambio_id` que no existe se ignoran (cuentan en `ignorados`).
- El motor LM (127) informa sus resultados por la **misma ruta** con `destino: "lm"` (su PC tiene el mismo token de dispositivo) o, desde el servidor, con `public.pes_registrar_informe` (solo service_role).

### 5. Quién ve qué (RLS)
| Tabla | admin / moderador | el dueño | resto |
|---|---|---|---|
| `pes_cambios`, `pes_tipos_cambio` | leer | — | — |
| `pes_pcs`, `pes_eventos` | leer todo | leer los suyos | nada |

Escribir: solo por las funciones (`pes_cambio_crear`, `pes_cambio_anular`, `pes_cambio_marcar`, `public.pes_registrar_informe` (solo service_role)).
Páginas web: **/cambios-oficiales/** (formulario único + historial) y **/usuarios-pes/** (PCs, versión, último contacto, historial por usuario y PC).

### 6. Enchufes del formulario (para 126 y 127)
`src/js/core/cambiosOficiales.js` exporta `registrarTipo({ tipo, etiqueta, destinos, pintar(contenedor, ctx), leer() → datos | lanza Error })`.
Cada sesión añade su archivo `src/js/features/cambios/<tipo>.js` que llama a `registrarTipo(...)` y lo importa en `src/js/pages/cambiosOficiales.js`.
Las casillas Base normal / Liga Máster / Las dos son comunes; `ctx.destino` dice cuál está marcada para pintar los campos extra (p. ej. club de LM).

### 7. Límites
- `liga_traspasos` sigue con su retención de 30 días: un PC que pase más de 30 días sin abrir Link no recibe los oficiales viejos por el canal
  de traspasos (sí los ve en `GET /v1/cambios`). Los oficiales **no** cuentan para el tope de 200 lotes por anfitrión.
- 1 fichaje por cambio (el formulario puede crear varios seguidos).
- Lo que el PC hace en el juego (recargar Phoenix-DB) es igual que en traspasos en vivo: ver `docs/TRASPASOS-EN-VIVO.md` §4.

### 8. Historial del contrato
- v1 (17-oct-2026, sesión BASE): primera versión.

---

## Parte 12 · Traspaso 17-oct-2026 · Web ↔ PES (para continuar con la otra cuenta)

> Origen: `docs/TRASPASO-17-OCT-WEB-PES.md` · último cambio 3678426 2026-10-17 · 3282 bytes. Copia íntegra.

Fralex pidió: «sube todo a GitHub rápido para no dejar a medias; ya usaré mi otra cuenta para avanzar».
Las 4 sesiones de trabajo se cortaron por el **límite de uso** (vuelve a las 9:10 pm, hora de Lima). Todo su trabajo está en GitHub.

### Qué es cada frente
Un solo formulario en la web («Cambios oficiales»): cada cambio va a **base normal (amistosos)**, **Liga Máster** o **las dos**, llega solo a cada PC y cada PC informa el resultado al **registro de usuarios**.
Contrato: `docs/CONTRATO-WEB-PES.md` v1 (en este repo, rama `rediseño-phoenix-portal`, y en la web `docs/`, rama `borrador`).

| Frente | Migración | Estado | Dónde está |
|---|---|---|---|
| Amistosos + registro + formulario único (BASE) | 125 | Contrato publicado. Link 0.3.19 y web (formulario, registro) **subidos a las ramas principales**. Faltaban rutas de la función web y terminar el seguimiento automático del canal oficial en Link (según la sesión de estadísticas) | Link `rediseño-phoenix-portal` (5ae6b2b) · web `borrador` (6b384ad) · respaldo `respaldo/ws-amistosos-*` |
| Estadísticas e información | 126 | **Hecho y subido**: Link 0.3.18 aplica stats/info en el Player.bin de Phoenix-DB; web con migración 126, formulario, historial y deshacer; pruebas en verde. Falta: rutas de la sesión BASE y probar en un PES real | web `borrador` (9d4ee85) · respaldo `respaldo/ws-stats-*` |
| Liga Máster | 127 | **A medias** (Sync: CambiosLM, PartidasLM, LigaMasterWeb, pruebas lm127; web: migración 127 y panel) | `respaldo/ws-lm-sync`, `respaldo/ws-lm-link` (repo Link) · `respaldo/ws-lm-web` (repo web) |
| Botón ACTUALIZAR (auto-actualización de Link) | 128 | **A medias** (Link: actualizador y EMPAQUETAR-RELEASE.ps1; web: migración 128, página link-versiones, pruebas) | `respaldo/ws-actualizar-link` (repo Link) · `respaldo/ws-actualizar-web` (repo web) |

Las ramas `respaldo/*` llevan un commit «RESPALDO (a medias)»: **no están terminadas ni probadas enteras**. No se han unido a las ramas principales a propósito.

### SQL de Supabase (todavía NO pegado)
Orden obligatorio: **125 → 126 → 127**; la **128** va aparte, en cualquier momento. Cada una trae su prueba `supabase/tests/12X_*_test.sql`.
- 125 y 126: en `borrador` (terminadas por sus sesiones; revisar antes de pegar).
- 127 y 128: solo en sus ramas `respaldo/*` (a medias: **no pegar** hasta terminarlas).

### Cómo seguir (otra cuenta)
1. `git fetch origin` en los dos repos.
2. Para cada frente a medias: `git worktree add C:\dev\ws-<nombre> respaldo/<rama>` y continuar leyendo su commit de respaldo y el contrato.
3. Al terminar: pasar los cambios a la rama principal (`git pull --rebase origin <rama>` + `git push origin HEAD:<rama>`), subir kVersion y anotar en `REGISTRO-LINK.md` / `REGISTRO.md`.
4. Nunca subir `SmashSoda/phoenix/PhoenixBetaSello.h`.

### Lo último de Link en esta cuenta (antes de los frentes)
0.3.12–0.3.16 (traspasos sin papel; aprobar al enviar y al recibir; «Mantener mis fichajes en mi juego»: los fichajes propios van a la Phoenix-DB y no se pierden al cerrar PES). Falta probar en el PES de Fralex: la primera vez con PES cerrado para crear Phoenix-DB y la línea del sider.ini.

---

## Parte 13 · PROMPT para el chat WEB · Plan de experiencia de Phoenix Link (19-oct-2026)

> Origen: `docs/PROMPT-WEB-plan-ux.md` · último cambio 053c502 2026-10-19 · 6144 bytes. Copia íntegra.

> Copia todo este archivo en el chat WEB (repo `C:\dev\pes-liga-automatica`, rama `borrador`).
> Regla de siempre: nada se publica hasta que Fralex diga «Súbelo». Lo de Supabase, como migración nueva + SQL listo para pegar.

### Contexto

Link aplicó el plan de experiencia (`plan-phoenix-link.html`) en la rama `plan-ux` (0.3.21 – 0.3.34): pantalla INICIO con
JUGAR CON UN AMIGO / BUSCAR RIVAL, Semáforo Phoenix único, asistente de bienvenida, sentado automático, revancha, glosario
(«Conectar mi cuenta», «Mi juego», «Jugar»). Lo que sigue **no se puede hacer desde Link**: depende de la web. Link ya está
preparado para cada punto (se dice cuál). Orden sugerido: 1 → 2 → 4 → 6 → el resto.

### 1. Aprobación de anfitrión (bloqueo P0)

Hoy, sin aprobación manual del staff (`HOST_NO_AUTORIZADO`), un usuario nuevo no puede abrir sala publicada, enviar traspasos ni salir en el radar.
Fralex aún no eligió; propón y deja listas **las dos** opciones detrás de un interruptor del admin, por defecto la A:
- **A · Automática con límites:** al vincular la primera PC, la cuenta queda como anfitrión «básico» (salas de amigos y públicas con tope
  de plazas, sin oficiales) y el staff la revisa después (lista «por revisar» en el admin).
- **B · Prueba de 7 días:** anfitrión completo durante 7 días desde la primera vinculación; luego pasa a pedir aprobación.
- En ambos casos: `POST /v1/anfitrion/solicitar` (o el nombre que prefieras) para que Link pida la aprobación desde la app.

Link ya explica la pausa: lee `codigo` en el error y dice «Tu cuenta aún no está aprobada como anfitrión. Mientras tanto puedes jugar
con amigos y unirte a salas de otros». Si añades la ruta de solicitud, dímelo y Link pone el botón «Pedir aprobación».

### 2. Conectar la PC por el navegador (sin copiar códigos)

Hoy: código de instalación (24 h) + código de 6 cifras (10 min). Pedido: flujo de dispositivo.
1. Link `POST /v1/pc/emparejar/iniciar` → `{ id, url, caduca_seg }` (sin token).
2. Link abre `url` en el navegador (página de la web con la sesión iniciada): «¿Conectar Phoenix Link en este PC?» [Conectar].
3. Link sondea `GET /v1/pc/emparejar/estado?id=…` cada 2 s → `{ estado: "esperando" | "listo" | "caducado", token?, usuario? }`.
Mismo token que hoy da `web.vincular`. El código de 6 cifras sigue valiendo como respaldo.
En la web, renombra «Mi perfil › vincular PC» a **«Conectar Phoenix Link»** (Link ya dice «Conectar mi cuenta» en todas partes).

### 3. PC conectada sin ser anfitrión

Permitir conectar una PC aunque la cuenta no sea anfitrión, para lo de invitado: presencia, avisos, amigos, recibir traspasos y cambios
oficiales, salas abiertas. Las rutas de anfitrión (abrir sala publicada, enviar traspasos, resultados oficiales) siguen pidiendo aprobación.

### 4. Versiones de Link e instalador

- `link_versiones` está **vacía**: el botón ACTUALIZAR (0.3.20) nunca encuentra nada. Publica la primera versión firmada cuando Fralex
  apruebe esta rama (Link: `tools/EMPAQUETAR-RELEASE.ps1 -Subir -Notas "…"`; web: `/link-versiones/`).
- Un **instalador .exe** único (VC++, ViGEmBus, WebView2 y Parsec si faltan) en lugar del ZIP del código fuente para «Quiero ser anfitrión».
- Cabecera con la versión real: Link puede mandar `X-Phoenix-Link-Version: 0.3.34` además de `X-Phoenix-Version: 7.0.4` (que sigue igual
  para no romper `version_app_min`). Confirma que la web la acepta y la guarda por PC, y Link la añade.

### 5. Un solo interruptor de pausa

Hoy liga y traspasos se pausan con una clave y Phoenix con otra. Unifícalo (o documenta cuál manda) para que Link enseñe una sola causa.

### 6. Buscar rival: «Jugar ahora» y cuántos buscan

- Web: botón **«Jugar ahora»** que entra a la primera sala libre de la región con buen ping (sobre `radar_salas`).
- Para Link: en `GET /v1/salas` (o ruta aparte) añade `buscando: N` (cuántos están en la cola de la Sala de Duelos / `retos_matchmaking`)
  y, si es posible, entrar a la cola desde Link (`POST /v1/retos/buscar`, `DELETE` para salir). Link enseña «N personas buscan partido» en
  INICIO › BUSCAR RIVAL en cuanto exista el campo.
- Orden de salas: Link ya ordena igual que la web (invitado → amigo → con sitio → ping). Si la web cambia el criterio, avisa.

### 7. Amigos en los perfiles (sentado automático «solo mis amigos»)

`POST /v1/perfiles` devuelve la carta por `parsec_id`, pero **no el id de usuario**. Link hoy reconoce a un amigo comparando el nombre del
perfil con el de tu lista de amigos. Añade a cada perfil `usuario_id` (o mejor `es_amigo: true|false` respecto a la cuenta de la PC que pregunta).
Link ya lee `usuario_id`/`usuarioId`/`id` si llega.

### 8. Resultado en disputa → «En revisión»

Cuando dos propuestas no coinciden la web marca disputa. Devuelve el estado del partido oficial en `/v1/liga` (o donde Link ya lee los
partidos) con `estado: "disputa"` y un `url` de la ficha, y Link enseñará «En revisión» con el enlace en la tarjeta de resultado.

### 9. Fichajes: un solo camino

Decisión por defecto (Fralex no contestó): **el canal de Link** (traspasos en vivo + cambios oficiales, migraciones 125/126) es el camino
de los fichajes. Las rutas de Phoenix Sync para grupos y foto oficial quedan **dormidas** (no se borran). Publica la página
«Cambios oficiales» si sigue solo en `borrador` y haz el primer cambio oficial de prueba (hoy hay 0) para probarlo con Link en una PC real.

### 10. Lo que Link ya hace (para que la web no lo repita)

- Semáforo Phoenix, asistente de bienvenida y errores con causa + botón (`estado.web.codigo`).
- INICIO: JUGAR CON UN AMIGO (abre sala de amigos + copia enlace + `amigos.invitar` → `/v1/invitar`), BUSCAR RIVAL (sala pública).
- Sentado automático al pulsar el mando (opción «solo mis amigos»).
- Tarjeta de resultado con REVANCHA y CAMBIAR LADOS.

Cuando termines cada punto, deja en `docs/` de la web qué ruta/campo quedó y su forma exacta, para que el chat LINK lo conecte.

---
