# Especificación del relé de juego, fases 2 y 3 (partida entre 2 PES) — estudio de solo lectura

Fecha: 10-nov-2026. Fuente: lectura (sin ejecutar nada) de la copia local `D:\Frank\IA\CLAUDE\pesbul_local\app`:
`server_gamerelay/*` (relay_server, ch0_handshake, participants, roster, game_sequence, quality_probe, native_departure, paced_relay,
coop_check_route, startup_*), `server_common/*` (gamerelay_pairing, gamerelay_assignment, ocrank_relay_pairing, ocrank_check_barrier,
accepted_match) y `server_tcp/main.py` (33 000 líneas; solo los trozos que tocan el relé y la sala). Todo está descrito con palabras
nuestras: no se copia código, ni claves, ni el contenido de `data\`. Se complementa con `ESPEC_GAMERELAY.md` (fase 1: sonda, KX, canales 0/1/5/6),
que sigue vigente; aquí solo se añade lo que falta para que dos PES jueguen.
Marcas: **L** = leído en el código (archivo, función, línea aproximada); **P** = suposición razonable. Las líneas son aproximadas (el
`relay_server.py` ronda las 8 500 líneas y los comentarios son largos).
**Hallazgo que condiciona todo el plan (L):** el relé de juego NO lo usan todos los modos. El amistoso 1 contra 1 de sala libre (OC_FREE) va por
P2P directo con NTL/TURN y el servidor no lleva datos de partido (`R7.3.613_FRIENDLY_1V1_NRG_TURN_FIX.md` §5). El relé (modo «servidor central»)
lo usan Team Play y Cooperativo, y también las parejas de Divisiones en línea y Partida rápida (OC_RANK / OC_QUICK), que PESBUL registra en una «sala
sintética» sin sala real (`ocrank_relay_pairing.py`, `main.py` `_r73645_register_ocrank_pair` ~3063). Por tanto «un partido entre 2 PES por el relé» en
la práctica = pareja de Divisiones/Partida rápida (el camino que ya recorre nuestro servidor hasta «buscando rival»).

---

## 1. Cómo una sesión UDP queda ligada a una pareja/sala, y cuándo deja de ser «sonda»

### 1.1 Qué identifica una sesión (L)
- Cada «par» UDP se guarda con clave `protocolo|ip:puerto` de origen (`peer_state`, relay_server ~422). El KX lleva solo un nonce de 4 bytes, **sin
  identidad** (`gamerelay_pairing.py` cabecera); el `pid` no viaja por UDP en ningún momento. Tampoco hay token ni cookie en el relé.
- Por eso la identidad sale del TCP: el cliente declara sus direcciones con `CMD_SEND_IPANDPORT_NTL` (campos `host_addr`, `reflexive_addr`, `relayed_addr`,
  cada una un mapa con `sa_data` = 14 bytes en base64: puerto u16 BE, 2 de relleno, IPv4 de 4 bytes, 6 ceros; `gamerelay_pairing._decode_sa_data` ~55,
  `endpoint_from_ntl` ~117). El lobby (`main.py` ~21824) normaliza esa declaración (cambia hosts inservibles por la IP TCP observada) y la guarda en el
  participante de la sala. El UDP que luego llega al relé viene EXACTAMENTE de esa dirección declarada (medido por PESBUL con dos clientes en una IP).
- La tabla inversa `ip:puerto → (room_id, pid, enter_no, fuente)` la escribe el proceso del lobby en un archivo (`gamerelay_pairing.json`, `build_table`
  ~331, `write_table` ~523) cada vez que alguien entra/sale de una sala, y el relé la relee cuando cambia su fecha (`PairingIndex`, ~542).
  Cuando un mismo `ip:puerto` lo reclaman dos pid, pasa a «ambiguo» y no liga a nadie (falla cerrado).

### 1.2 Orden de resolución al ligar (`bind_peer_identity`, relay_server ~1006) (L)
1. Coincidencia exacta del endpoint observado con la declaración, mirando primero `reflexive_addr` y luego `host_addr` (el `relayed_addr`, TURN, se ignora).
   Hay dos alias que también cuentan como exactos: el «self-host» (el PES que corre en la misma máquina que el servidor publica la IP pública pero sus
   datagramas salen de la IP LAN) y el «LAN» (IP TCP observada + puerto declarado).
2. Si no hay exacta y el protocolo es UDP: «IP única». Se mira, sobre TODA la tabla (no solo una sala), cuántos participantes son alcanzables desde esa
   IP (IP declarada o IP TCP). Si hay exactamente uno y no está ya ligado por otro par vivo, se liga a él. Con 2 o más candidatos se rechaza (`ambiguous`),
   porque nada en el protocolo distingue a dos PES tras una misma IP pública. No se usa orden de llegada, ni de pid, ni de enter_no.
3. Un tercer paso solo para la red de varios nodos (alias de puerto privado); no aplica a un solo nodo.
- Si todo falla: se registra `GAMERELAY_PEER_BIND_FAILED` (razón y nº de candidatos) y NO se emite lista de participantes a ese par.
- Una vez ligado, queda cacheado en el estado del par (`peer_bound`, `peer_identity`). «Un pid = un par»: si otro endpoint del mismo pid ya estaba
  ligado y lleva callado un rato, se le libera («reemplazo de holder»); si sigue vivo, el nuevo se rechaza (`already_bound`). También hay migración de
  endpoint cuando un cliente cambia de puerto (`_r73581_try_endpoint_migration`, ~593; el detalle no se estudió: P).

### 1.3 Cuándo se intenta ligar (L)
- No al primer datagrama. La cadena de la sesión (ya implementada por nosotros hasta el canal 5) es: KX → canal 0 (SYN, SYN+ACK, ACK) → el cliente abre por
  el canal 1 la clave 5 (y 14/15 a la vez cuando llega a estado 3; ver §3) → el servidor contesta `90 K` y espera a que el cliente CONFIRME ese
  `OPEN-ACCEPT` (se retira de su almacén: `ch1_open_retired`) → solo entonces `maybe_arm_participant_list` (~1176) intenta ligar y armar la lista del canal 5.
- Si ligar falla, ese mismo punto abre la puerta a la sonda (§1.4). Si liga, nunca se vuelve a evaluar como sonda.

### 1.4 «Sonda» frente a «partida» (L; coincide con ESPEC_GAMERELAY §4, con matices)
- **Sonda** (`quality_probe.py`, `_quality_probe_classify` ~1297): solo si el ligado normal falló con razón `no_match` y CERO candidatos, o con la razón
  `probe_authorized_no_live_attempt` (IP única, pero esa IP tiene una sonda autorizada y la sala del candidato NO tiene intento vivo), y además el
  control (HTTP/TCP `qualitychecklist`) autorizó recientemente esa IP para ese índice de nodo. Lista de participantes de 2 bytes (servidor 31 + «yo» 0).
  Estados: provisional (caduca a los 10 s si no abre el canal 6), confirmada (al abrir el canal 6; caduca tras 10 s sin registros de calidad) o rechazada
  (si abre cualquier canal de juego 9..15). Los plazos son variables de entorno: 15 s de vigencia de la autorización, 10 s provisional, 10 s inactividad.
- **Partida**: hay fila de emparejamiento y un intento de partida vivo (§1.5). Entonces la lista tiene 3 bytes (§2) y el par entra al reenvío.
- Matiz respecto a nuestro doc de fase 1: el criterio no es «no existe fila para esa IP» a secas; también se trata como sonda a quien tiene fila pero cuya sala
  no tiene intento vivo y tiene sonda autorizada. Además, `auth_ttl` por defecto es 15 s (variable `PES_QUALITY_PROBE_AUTH_TTL_SEC`); nuestro doc dice «hasta
  86 400 s, política del pid»: puede ser otra capa (la del archivo de autorización del HTTP). **Duda P #1.**

### 1.5 «Intento de partida» (match attempt) — el interruptor de la fase 2 (L)
- El lobby (proceso TCP) avisa al relé con un mensaje «MATCH_ATTEMPT_OPENED(room_id, generation, member_pids, roster, pairing_rooms)» por un puente local
  (IPC en `gameseq_ipc.py`; en PESBUL son dos procesos, en nuestro caso un solo proceso: se sustituye por una llamada directa). Lo dispara:
  - Sala libre/Team Play: la transición ESPERANDO→JUEGO de la sala (`_r73325_set_room_game`, `main.py` ~294), que se hace en `CMD_ENTRY_GAME` (la «salida» de sala) y/o
    `CMD_START_SEND_GAME_SEQUENCE_DATA` / `CMD_SET_GAMEMEMBERENV` (`main.py` ~21302-21333). `_r73466_match_attempt_started` (~2383) sube la «generación» de la sala (empieza en 1),
    la registra y llama al relé.
  - Pareja de Divisiones/Partida rápida: al emparejar (el `FINISH` del `WATCH_MATCHING`), `_r73645_register_ocrank_pair` (~3063) crea una «sala sintética» con id
    700 000 000 + contador (reservado, no choca con las salas reales), con dos jugadores: el que esperaba = enter_no 0 (anfitrión, HOME) y el que llegó = enter_no 1
    (invitado), copia el estado NTL de cada uno, republica el archivo de emparejamiento y abre el intento por el camino normal.
- Qué hace el relé al recibirlo (`_r74331_attempt_opened`, ~6997): congela la **lista de miembros del intento** (pid + enter_no) ANTES de marcar a los pares. Este congelado
  tiene prioridad sobre el archivo de emparejamiento (que es solo respaldo). Para los pares que ya estaban ligados a un intento anterior marca un «reinicio pendiente»: no
  borra nada al instante, se aplica en el siguiente KX o primer SYN del cliente (el cliente reinicia su transporte en cada partida). Idempotente por (sala, generación, par).
- Cierre: `MATCH_GENERATION_CLOSED` marca de nuevo a los pares (reinicio pendiente), borra la lista congelada y libera la asignación; es lo que hace el lobby cuando
  todos vuelven a la sala (`_r73466_*`) o la sala se borra. Los intentos viejos (generación menor) se ignoran.
- Estado que guarda el relé por sala/intento: lista congelada, generación y huella (token) del intento, rutas del par coop (no aplica a 2), listas de «departed»
  (retiradas), y la caché de deduplicación (§3.4). Por jugador (por `ip:puerto`): ver §1.6.

### 1.6 Estado por par (L, `fresh_transport_state` ch0_handshake ~1026)
Clave y material KX (`kx_complete`, `kx_material`), contador de secuencia de datagramas saliente (`ch0_dg_seq`), canal 0 (almacén de salida, siguiente esperado, siguiente propio),
canal 1 (igual + mapa de claves abiertas), canal 5 (lista armada, almacén, elemento, «retirada»), y por cada canal de juego K: almacén de salida `{clave de elemento: blob}`, siguiente
clave propia, siguiente clave esperada del cliente (= el ack que se le manda), y un búfer de reordenación acotado (§3.2). Más: `peer_bound`/`peer_identity`, `r73549_attempt_token`
(huella del intento al que pertenece), `r73545_reset_pending`, eco RTT pendiente, y marcas de actividad (`last_seen_ts`).

---

## 2. Lista de participantes del canal 5 con 2 jugadores

### 2.1 Formato byte a byte (L; `roster.py` + `participants.py`, validado por PESBUL contra el decodificador del cliente)
Es una carga de transporte de **un byte por participante**, sin cabecera ni relleno (se mete como un único «bloque» `[largo u8=n][bytes]` en un elemento de la lista A del canal 5):
- bit 7 = 0 → el SERVIDOR (levanta una bandera de «servidor visto» en la conexión del cliente).
- bit 7 = 1 y bit 6 = 1 → «YO» (el receptor de la lista; levanta «propio visto»).
- bit 7 = 1 y bit 6 = 0 → OTRO jugador (va al conjunto de pares).
- bits 4..0 = id de 5 bits (0..31). El bit 5 no se usa y va a 0. **Un id repetido hace que el cliente tire TODO el mensaje** (lo decodifica con un descarte total), y el
  mensaje debe consumirse entero. Hace falta exactamente un servidor y un «yo»; cuando llegan ambos el cliente pasa de estado 2 a 3 («establecida») y es entonces cuando abre las claves 14/15.
- Ids (L, `participants.allocate` ~46): se ordena la lista congelada por (`enter_no`, `pid`) y se asigna 0,1,2… por orden. El id **31 (0x1F) queda reservado al servidor**.
  Máximo 31 participantes. Son locales al transporte (no se usan para nada fuera: ni slot de sala ni nada) pero deben ser **estables** y **los mismos en la lista de cada receptor**.
- Orden de emisión: servidor, «yo», y luego los demás en orden de id ascendente. El receptor nunca aparece repetido como «otro». (`payload_for` ~72.)
- Ejemplo con 2 jugadores (anfitrión A enter_no 0, invitado B enter_no 1):
  - lista para A: `1F  C0  81` (servidor 31; yo id 0; otro id 1)
  - lista para B: `1F  C1  80` (servidor 31; yo id 1; otro id 0)
  (`roster.build` produce bytes crudos; el valor `0x1F` sale de la constante del servidor, no del valor por defecto 0 de `roster.py`, que no se usa en producción.)

### 2.2 Cuándo se manda y se actualiza (L)
- **Primer envío:** `maybe_arm_participant_list` (§1.3) encola UN elemento en el canal 5 del par, una sola vez (`ch5_armed`), cuando: el OPEN de la clave 5 ya fue aceptado y confirmado, el par se
  ligó a (sala, pid) y la lista congelada del intento se puede asignar (id del receptor presente). Fuente de la lista: la congelada del intento y, si no hay, la del archivo de emparejamiento.
  Como la lista sale del intento y no de quién haya llegado ya al UDP, **cada jugador recibe la lista completa de 2 aunque el otro todavía no haya conectado por UDP**.
- **Retransmisión:** como todo elemento de lista A, se vuelve a incluir en cada datagrama saliente de ese par hasta que su `ack` lo supere («retirada»). Sin ack no se avanza (visto: una retransmisión y llega el ack).
- **Actualización:** solo en la ruta de «partida abandonada nativa» (§4.3): se encola otro elemento del canal 5 con los restantes cuando el que se va ha entregado su mensaje de salida
  y el receptor ha confirmado el anterior; los bytes de servidor y «yo» no cambian, solo se quita al «otro». Esa ruta exige que queden ≥2 humanos vivos, por lo que
  **con 2 jugadores el superviviente no recibe lista nueva** (L).
- Si la asignación falla (id repetido, sala vacía, receptor ausente) se registra `CH5_PARTICIPANT_LIST_REFUSED` y no se manda nada (falla cerrado).
- «Yo» marcado: es el bit 6 del byte; **no** significa nada más (no es «anfitrión»: el id de «yo» sale de enter_no).
- Matiz de fase 1: la sonda usa «yo» = id 0 con el servidor 31; en partida real «yo» = su id de la tabla congelada.

---

## 3. Reenvío de los canales de juego (9 a 15)

### 3.1 Panorama de canales (L, `ch0_handshake.py` constantes ~133-218)
Claves válidas del flujo: 5..15 (el validador del cliente es «clave−5 < 11»). Una clave `K` se «abre» por el canal 1 con `80 K`; el servidor contesta `90 K`. Los datos de la clave K van en secciones con nibble de canal = K.
| Canal | Qué es (según PESBUL) | ¿Se reenvía al otro? |
|---|---|---|
| 0 | control del transporte (SYN/ACK/vida) | no; lo responde el servidor |
| 1 | control de flujos (abrir/cerrar) | no; lo responde el servidor (solo OPEN; CLOSE/RESET se registran y NO se contestan en esta versión) |
| 5 | lista de participantes | no; la emite el servidor |
| 6 | calidad (solo sonda) | no; eco al propio emisor |
| 9 | «órdenes en línea» = la entrada de los mandos por paso del juego (cerrojo de 24 ranuras, lockstep) | SÍ, a todos los de la sala, **incluido el propio emisor** (eco propio) con una etiqueta de 1 byte |
| 10 | carga de partida (GAMELOAD) | SÍ (vía rápida nativa + copia por TCP, §3.4) |
| 11 | contrato inicial de fuentes/comandos (el cliente lo manda; el servidor lo observa) | NO se reenvía; solo se lee para la salida nativa (§4.3) |
| 12 | tráfico en partido | SÍ (vía rápida, §3.4) |
| 13 | MatchControlInfo (informe del cliente, 3 bytes) | NO se reenvía; el servidor lo observa y le INYECTA su propio mensaje (§3.6) |
| 14 | flujo de sesión | el cliente abre 14 y 15 a la vez en estado 3; solo se emite algo para el chequeo de conexión (§5.2); 14 direccionado para chat de demo |
| 15 | secuencia de juego («GameSeq»: marcador, lados, listo, equipos, datos de preparación) | SÍ (vía rápida + copia por TCP) |

### 3.2 Formato de transporte y ARQ por receptor (L)
Cuerpo tras descifrar el marco: `[dgseq u32 BE][flags u8]` y secciones hasta el final del cuerpo (debe terminar exacto; si no, el cliente descarta el datagrama).
- `flags` (L, `parse_body` ~768): bit 0x80 = «el cliente pide eco RTT»; bit 0x40 = «este datagrama trae eco»: entonces siguen `echo_seq u32 BE` + `hold_ms u8` (el
  tiempo que el servidor retuvo la petición, 0..255). Nuestro doc de fase 1 solo decía «devuelve la misma secuencia»; el detalle correcto es este (§4.1).
- Sección: `[canal<<4][ (canal<<4) | 0x02 si lista A | 0x01 si lista B ][ack u32 BE]`; el nibble bajo del primer byte debe ser 0. Siempre van las secciones 0 y 1 (aunque vacías), luego la 5 si la hay, luego cada canal de juego «vivo» en orden ascendente.
- **Lista A (fiable):** `[n-1 u8]` y n elementos `[clave u32 BE][largo-1 u16 BE][carga]`, hasta 256 por lista. La carga es un «blob» `([largo u8 (1..255)][bytes])*`.
  **Cada bloque del blob consume UNA clave ARQ** (no cada elemento): un elemento de clave K con 3 bloques cubre K..K+2 y el siguiente esperado pasa a K+3 (`arq_span/arq_advance` ~910). Es el error más fácil de cometer.
- **Ack** = «la siguiente clave que espero» (estricto: confirmar la clave 0 es mandar 1); un ack mayor que lo enviado es error del cliente. Retira del almacén todo lo menor que el ack (`apply_peer_ack_stream`).
- **Lista B (no fiable, sin clave):** un byte de grupos (`0x80` grupo 0, `0x40` grupo 1; los 6 bits bajos deben ser 0), y por cada grupo presente (en ese orden) `[n-1 u8]` y n elementos `[largo-1 u16 BE][carga]`.
  El relé la lee pero no reenvía nada de ella (L). Ojo: nuestro `gamerelay.py` lee solo UN grupo con cabecera u16 `0x8000|(n-1)` y cada elemento con una clave u32 delante (visto en el canal 6): equivale al grupo `0x80`, pero un grupo `0x40` se leería mal.
- **Cada receptor tiene su propia sesión fiable.** El reenvío NO copia las claves del emisor: el mensaje se mete en el almacén de salida del RECEPTOR (`enqueue_stream_message` ~3244, `arm_stream_message`, `arq_queue` ~936) con su propio contador de claves por canal, un bloque por clave.
  Un mensaje de más de 255 bytes se parte en trozos de ≤255 (la anchura del largo), en claves consecutivas; el receptor los concatena.
- **Recepción ordenada (L, `consume_stream` ~2172):** solo se «acepta» el trozo con la clave igual a la esperada; los trozos bajo la ventana son repetición (se ack-ean y no reenvían); los de clave futura se **guardan** en un búfer por (par, canal) con tope (65 535 bytes y 4 096 claves,
  por seguridad de memoria) y se liberan cuando se cierra el hueco; el ack nunca cruza un hueco. Lo recién aceptado (y solo eso) se entrega al reenviador, por lo que una retransmisión del cliente no duplica el reenvío.
- **Ritmo de envío (L, `pack_reply_body` ~1250):** el servidor NO tiene temporizador general. Cada datagrama del cliente dispara UNA respuesta (`ch0_handshake_reply`, relay_server ~7442) que lleva, por canal vivo, TODOS los elementos aún sin ack **en orden ascendente de clave**, hasta llenar el cuerpo máximo de **2048 bytes (0x800)**
  (control primero: canales 0, 1, 5 nunca se desplazan por el atraso de juego). Lo que no cabe se queda en el almacén y sale en un datagrama posterior (siempre el prefijo no confirmado más antiguo). Un elemento que supera el máximo por sí solo bloquea y se registra (no se manda nada).
  Sin nada pendiente, tras la ráfaga inicial el servidor ni siquiera responde (salvo eco RTT pendiente). Además hay un **«empuje inmediato»** (`r73572_try_immediate_push`, ~5002): al reenviar el servidor manda ya un datagrama al receptor sin esperar su siguiente sondeo; si algo falla se vuelve al reparto por sondeo y nada se pierde. En la configuración de producción (compose VPS) está activado para el canal 9 (`PES_TEAMPLAY_CH9_IMMEDIATE_PUSH=on`); por defecto del compose comunitario está apagado.
- **Reintentos/ventanas:** no hay temporizador de reenvío propio; la retransmisión es «reenviar lo no confirmado en cada datagrama que se manda». El tope práctico lo dan los 2048 B por datagrama y los 256 elementos por lista. (P: el cliente real manda datagramas con mucha frecuencia —sondea cada canal miles de veces por partido— y por eso funciona sin reloj.)
- **Ritmo fijo opcional (L, `paced_relay.py`):** `PES_GAMERELAY_TICK_HZ>0` pone un hilo por sala que, a cada tick, entrega un registro por participante (el real si lo hay, y si no un «hold» `0xFA` a los demás y `0x80` neutro al dueño). Está **apagado** (0) en todas las configuraciones vistas; el motivo es que el cliente va en «paso a paso»
  (lockstep): cada cliente avanza a la velocidad de la ida y vuelta del más lejano. P: para una LAN/pareja no hace falta.

### 3.3 Canal 9 (entrada de mandos) paso a paso (L, `r73557_relay_native_channel9` ~3448)
1. Lo recién aceptado por la recepción del canal 9 de un par ligado queda en una lista pendiente del par. Si el par no está ligado → `CH9_NATIVE_REFUSED sender_not_bound`.
2. Receptores: todos los pares ligados de la misma sala/generación (los de la lista congelada), con el **emisor primero**. El emisor **también recibe su propio registro** (eco propio) porque el cerrojo de barrera de su propia ranura solo avanza con un registro etiquetado con su ranura; solo se suprime si el operador declara que ese cliente
  tiene bucle local nativo (`PES_GAMERELAY_LOOPBACK_PIDS`, lista explícita, nunca se deduce).
3. Trama que recibe cada uno: **un byte de etiqueta `(id_del_emisor << 3)` seguido del trozo tal cual** (`r73563_modecs_frame` ~3366; el id es el mismo de la lista del canal 5; el trozo no puede quedar vacío ni superar 254 bytes + etiqueta). Sin la etiqueta el cliente descarta un registro de 1 byte (su decodificador exige etiqueta + al menos 1 byte). El destino «31» que ve el cliente en lo que manda es el servidor, no un receptor.
4. El mensaje va al almacén del receptor en su canal 9 (su numeración propia) y se intenta el empuje inmediato. Hay un cerrojo «de arranque» (`startup_*`, solo con `PES2021_RELAY_STARTUP_CREDIT=full_lite_single_controller`, un mando por cliente) que retiene la entrada cruzada durante el arranque; P: opcional, calidad de arranque, no de corrección.
5. Contenido útil que conviene conocer (L, comentarios): el registro es opaco para el relé; los bytes `0xFA` (repetir último estado) y `0x80`/`0xE3..E8` (alias de comandos) son del juego. El relé no los interpreta salvo en el modo de ritmo fijo y en el de «salida nativa».

### 3.4 Canales 15, 10 y 12 (vía rápida nativa + copia TCP) (L, `r73562_relay_native_stream` ~6512, `relay_game_sequence` ~5899, `game_sequence.py`)
- Cada cliente manda estos mensajes DOS veces: como trozos nativos por el canal UDP y como registros TCP `CMD_SEND_GAME_SEQUENCE_DATA` (`game_seq_data` = lista de mapas `{type, fromId, channelId, sequenceNumber, data, data_len}`; `data` es base64 de un buffer de 256 bytes con una cabecera de 9 bytes
  `[tag u8][fuente u8][canal u8][contador u32 BE][largo u16 BE]` seguida del mensaje y relleno de ceros; solo se reenvía `type == 2` en los canales 15/10/12; el resto se cuenta y se ignora).
  Medido por PESBUL: la copia nativa llega antes (mediana +0,3…+2,9 s, máx. ~5 s) porque la TCP sale por un «reloj de respaldo» de 5 s del cliente.
- **Mensaje que recibe el otro:** cabecera de 2 bytes `0x40 | (id_emisor<<4)>>8…` = u16 BE `0x4000 | (id_emisor & 0x1F) << 4`, y detrás la carga **verbatim** (`stream_message`, `game_sequence.py` ~111). Es decir: dueño id 0 → `40 00 …`; invitado id 1 → `40 10 …`. Se parte en trozos de ≤255 como siempre. (Es un formato distinto de la etiqueta de 1 byte del canal 9: **cada canal tiene su propia trama de receptor**.)
- **Deduplicación entre las dos vías:** identidad del evento = (sala, generación, pid del emisor, canal, «contador de aplicación»); el `sequenceNumber` de TCP es el mismo contador por (emisor, canal) que la clave nativa; si el nativo ya se entregó a ese receptor, la copia TCP se suprime (y al revés). Además se exige igualdad exacta de bytes; ante cualquier discrepancia se entrega de más (nunca se pierde nada). Caché de 4 096 eventos, FIFO.
  La respuesta TCP a `CMD_SEND_GAME_SEQUENCE_DATA` es el ACK simple `{msgid, rqid, result:NOERR}` en cualquier caso (la fiabilidad va en el relé).
- **Canal 15 con id «fromId»:** el `fromId` de cada registro tipo 2 debe coincidir con el id del emisor de la lista del canal 5 (anclaje de validación; si no, se rechaza `fromid_mismatch`).
- En parejas de Divisiones (sin sala real) la rama TCP necesitaba un parche («sala sintética de reserva») o los registros se perdían y el cliente moría a los ~62 s con `LOAD_TIMEOUT` (`main.py` ~1283, `_r73651_ocrank_gameseq_room`). **Es obligatorio en nuestro diseño.**

### 3.5 Lo que NO se reenvía o se trata aparte (L)
Canales 0, 1, 5, 6; la lista B; el canal 11 (solo observado); el 13 (observado + inyección propia); el 14 solo para chequeo de conexión/chat de demo (`addressed_relay.py`: entrega direccionada y acotada a la sala; no estudiado en detalle: P); y todo `type != 2` o `channelId` fuera de 10/12/15 en la copia TCP. `edge_fabric.py` / `remote_room_rpc.py` / `fabric_lifetime.py` / `lobby_runtime.py`
existen solo para repartir salas entre varios nodos/procesos: **no necesarios para 2 jugadores en un nodo** (L: `lobby_runtime` y `backend_runtime` solo enrutan; con un nodo la ruta es local).

### 3.6 Canal 13 y «MatchControlInfo» (L, relay_server ~3966-4800, `r73592_*`)
El cliente manda por la clave 13 un informe de 3 bytes (tiempo desde el último informe y cuántos pasos de simulación hizo). El servidor, con `PES_GAMERELAY_KEY13_BOOTSTRAP=on` (producción), observa el primer elemento y le **inyecta** un elemento de 1 byte `0x80 | V` (V en 1..15; defecto del compose 1; `PES_GAMERELAY_MCI_V`),
que sube el «retardo/amortiguación de entrada» del cliente. Con el valor 1 o 2 el sistema es estable (PESBUL midió 143 s). Es un ajuste de calidad, **no requisito de la fase 2**; P: conviene tenerlo tras tener el reenvío del 9 funcionando, porque sin él el cliente real puede quedarse parado en el arranque (no estudiado exhaustivamente).

---

## 4. Eco RTT, vida, timeouts y desconexión

### 4.1 Eco RTT (L, `r73595_*` ch0_handshake ~366-530)
- El **cliente** pone el bit 0x80 en el byte de flags de SUS datagramas cuando quiere medir; el servidor recuerda (secuencia del datagrama, instante de recepción) en una cola de máx. 16 por par (descarta la más vieja).
- En el siguiente datagrama que sale hacia ese par pone flags `0x40` y los 5 bytes extra: `echo_seq` (la secuencia pedida) y `hold_ms` (ms entre recibirla y mandar el eco, 0..255; las peticiones con más de 255 ms se descartan sin responder). Si hay un eco pendiente se manda un datagrama aunque no haya nada más (la edad del eco es lo que mide el cliente).
- Solo activo con `PES_GAMERELAY_RTT_ECHO=on`; en el yml de producción del VPS está en `on`. Nuestro servidor responde a cada datagrama pero **sin cabecera de eco**: en la sonda eso bastó (18 ms medidos por el canal 6); para partida **P** conviene implementarlo.
- El servidor nunca pide eco al cliente (no pone 0x80).

### 4.2 Vida / «keepalive» (L)
- Canal 0, tras el apretón: el cliente manda `0x08` por temporizador (visto a T+10 s y se rinde a T+15 s si no hay respuesta) y espera `0x04` (`PROBE_REPLY_BITS`); nuestro código ya lo contesta.
- No hay temporizador de silencio que expulse a un jugador de la partida: la lista de participantes de **partida** se congela al abrir el intento. `PEER_TTL_SEC = 90 s` solo afecta a la lista de la ruta «eco/roster simple» antigua (`roster_for`, ~8126) y a la limpieza (L); la sonda caduca a los 10 s sin datos (§1.4);
  la conexión TCP del relé (si la usan) caduca por inactividad (`GAMERELAY_TCP_IDLE_TIMEOUT`, ~8353). El cliente real, si deja de recibir ack/datos, abandona por su cuenta («Unable to connect»/MXNV_0013 es la queja medida cuando el relé no ligó al par o no emitió lista a tiempo).

### 4.3 Qué hace al desconectarse uno (L, `_r74349_participant_left` ~6685, `native_departure.py`)
- Quien cierra la sala/abandona (llega `CMD_LEAVE_GAME`/`CMD_LEAVE_ROOM`/`CMD_SEND_GAME_RESULT`/`CMD_SEND_ABNORMALEND` por TCP, o se cae su TCP) lo comunica el LOBBY al relé como «participante retirado» (`MATCH_PARTICIPANT_LEFT(room, generación, pid)`), con 2 reintentos.
  El relé lo marca como retirado y **borra su estado de par** (se descarta su entrada, su ligado y su transporte). No manda ningún datagrama de cierre al otro.
- Solo si quedan ≥2 humanos vivos y el contrato inicial del canal 11 estaba cualificado, el relé inyecta por el canal 9 un registro de «salida» `0xE5` etiquetado con el slot del que se va (para que el cliente retire a ese jugador) y luego actualiza la lista del canal 5 de los demás (§2.2).
  **Con 2 jugadores esa ruta no se activa** (`fewer_than_two_surviving_humans`): el superviviente no recibe aviso por UDP; su cliente lo descubre por ausencia de datos/por TCP (P: el «forfeit» lo decide el lobby, no el relé).
- La generación se cierra (`MATCH_GENERATION_CLOSED`) cuando todos han vuelto a la sala o se borra: se marca «reinicio pendiente» en los pares (se aplica en su siguiente KX), se borra la lista congelada. Los reintentos de la sala sintética de Divisiones caducan a los 900 s como máximo (`OCRANK_TTL_S`).
- El relé NO cierra «la sala» por su cuenta ni envía un mensaje de cierre; el cierre es un hecho del lobby.

---

## 5. Comandos TCP que acompañan al relé durante un partido entre 2

Pregunta del usuario: nombres como `SET_GAMERELAY_MATCH_QUALITY`, `SET_LOCAL_GAMERELAY_LIST`, `START_COUNTDOWN`, `READY_FLAG`, `SELECT_SIDE`. Lo leído (L): **solo `SET_GAMERELAY_QUALITY` y `SET_GAMERELAY_CHECK_RES` tienen tratamiento propio**; `CMD_SET_GAMERELAY_MATCH_QUALITY` y `CMD_SET_LOCAL_GAMERELAY_LIST` solo aparecen en la lista de observación del lobby
(`lobby_observatory.py` ~107): no hay manejador dedicado (P: o no se envían en la ruta que probó PESBUL, o caen en la respuesta genérica; **no hay forma de saber el campo de respuesta**). `READY_FLAG`, `SELECT_SIDE` y `START_COUNTDOWN` (`MOMROOM_START_COUNTDOWN`) **no son comandos TCP**: son mensajes de aplicación del juego que viajan como registros del canal 15 (UDP nativo o TCP GameSeq, §3.4);
el servidor no los interpreta, solo los reenvía (salvo `START_COUNTDOWN` de Team Play, que PESBUL observa y puede retransmitir para sincronizar la cuenta de 30 s: no aplica a una pareja de Divisiones).

### 5.1 Tabla de comandos y respuestas (L salvo indicación)
Respuesta estándar de todos: `{msgid, rqid, result:"NOERR"}` (eco de msgid y rqid). Se indican solo los campos extra.
| Comando (cliente → servidor) | Campos de la petición que importan | Qué responde / hace el servidor |
|---|---|---|
| `CMD_SEND_IPANDPORT_NTL` | `host_addr`, `reflexive_addr`, `relayed_addr` (mapas con `sa_data`), tipo de NAT | solo `NOERR` (se debe responder: sin ack el cliente se queda ~53 s y se desconecta). El efecto real: guarda la declaración (§1.1) para el emparejamiento UDP. |
| `CMD_GET_GAMERELAY_QUALITYCHECKLIST` / `CMD_GET_GAMERELAY` | `pid` | ver ESPEC_GAMERELAY §1/§5 (ya hecho) |
| `CMD_SET_GAMERELAY_QUALITY` | `quality_list[{gamerelay_index, quality ms, last_update}]` | guarda por pid/índice y `NOERR` (hecho) |
| `CMD_SET_GAMERELAY_CHECK_RES` | `pid`, `error_code` (NOERR / ER_CONNECT / ER_PING / ER_CANCEL / ER_MATCHING_RETRY / ER_SERVER_GONE), `failed_reason` (0 = bien) | `NOERR` normal; **y** (solo en parejas de Divisiones/Partida rápida) cuando AMBOS jugadores han informado `NOERR`, el servidor EMPUJA a cada uno `CMD_WATCH_GAMERELAY_CHECK_RES {msgid, rqid:0, result:NOERR, error_code:NOERR}` (`_r73649_build_check_res_watch`, ~8572; barrera en `ocrank_check_barrier.py`; compresión activada). Si no se empuja nada, el cliente espera 30 s y sigue con error. |
| `CMD_SEND_MATCHING_RESULT` | `error_code`, `quality_select_kind` (esperado `SELECTED`), calidad elegida | `NOERR` (P: lo tratan como ack genérico + registro en `accepted_match`). Marca al miembro como «selected». |
| `CMD_SET_GAMEMEMBER`, `CMD_SET_GAMEMEMBERENV`, `CMD_SET_GAMEENV` | miembros/entorno de la partida | `NOERR` (ack simple; respuesta a tiempo es imprescindible: SET_GAMEENV sin responder bloquea ~53 s). |
| `CMD_SET_GAMETEAM` | `home_team_id`, `away_team_id` (enteros < 262143) | `NOERR`; se guarda y ambos jugadores deben mandar el mismo par para considerar la partida «aceptada». |
| `CMD_CHANGE_GAMEPHASE` | `game_phase` (MATCHMENU, …; en juego: `1ST`, `2ND`, `EX1ST`, `EX2ND`, `PK`) | `NOERR`; actualiza la fase en vivo. |
| `CMD_START_SEND_GAME_SEQUENCE_DATA` | (marca el inicio) | `NOERR`; en salas pasa la sala a estado JUEGO y abre el intento (§1.5). |
| `CMD_SEND_GAME_SEQUENCE_DATA` | `game_seq_data[]` (§3.4) | `NOERR` inmediato; el reenvío al otro es efecto lateral. |
| `CMD_SET_GAMEPLAYER_PLAYTIME` | tiempo jugado del jugador sustituido (cambios, fin de tiempo) | `NOERR`; **si no se responde el cliente falla a los ~55 s con ABNORMALEND y echa a los dos**. |
| `CMD_ADD_SCORE`, `CMD_ADD_FOUL`, `CMD_SEND_MATCH_TEAM_INFO`, `CMD_SEND_STATS_DATA`, `CMD_SEND_MATCH_ANALYZE`, `CMD_REQ_GAMEADDPOINT`, `CMD_SEND_SYNC_NOTICE`, `CMD_SEND_SYSTEM_NOTICE`, `CMD_SEND_TEXTCHAT` | marcador/estadísticas/avisos | `NOERR` (+ captura); sirven al resultado y al panel. |
| `CMD_SEND_GAME_RESULT` | resultado final (goles, etc.) | `NOERR`; el lobby lo usa para cerrar la partida y puntuar. En Divisiones además se acumulan evidencias de fin (`CMD_SET_GAMEPLAYER_PLAYTIME`, `SEND_MATCH_ANALYZE`) antes del resultado final (`od_end_handoff`). |
| `CMD_SEND_ABNORMALEND` | motivo (`reason`, `code`) | `NOERR`; marca fin anormal; ver §4.3. |
| `CMD_LEAVE_GAME`, `CMD_LEAVE_ROOM` | — | `NOERR`; disparan el retiro del participante y el cierre de la generación (§4.3). |
| `CMD_JOIN_ONLINE_MODE` / `CMD_LEAVE_ONLINE_MODE` | tipo de modo | `NOERR` simple. |
| `CMD_WATCH_MATCHING` (empuje servidor→cliente) | — | estados `SEARCHING`, `FINISH` (con `side` HOME/AWAY y `member`), `CANCEL`; **el cliente no lo pide: lo empuja el servidor** (nuestro doc de traspaso ya lo sabe). |
| `CMD_WATCH_ROOMSTATE` (empuje) | `event:"StateChanged"`, `state:"GAME"`/`"WAITING"` | en salas (amistoso/Team Play): `GAME` al empezar, `WAITING` cuando todos volvieron. |
| `CMD_GET_TURN_SERVER_LIST` | `pid`, `turn_account_param`, `turn_time_limit` | **solo amistoso P2P** (`proto_opt=1`): lista de servidores TURN + id/pw/certificado (RFC 5766). No es del relé. |
Orden en una pareja de Divisiones (L, `ocrank_check_barrier.py`, `accepted_match.py`, `main.py` ~8480): `WATCH_MATCHING FINISH` → el cliente abre el relé (UDP) → `SET_GAMERELAY_CHECK_RES` → (empuje `WATCH_GAMERELAY_CHECK_RES`) → `SEND_MATCHING_RESULT` → `SET_GAMETEAM` →
`CHANGE_GAMEPHASE MATCHMENU` → carga (`START_SEND_GAME_SEQUENCE_DATA` + GameSeq 15/10 por TCP+UDP) → juego (canal 9/12) → `SET_GAMEPLAYER_PLAYTIME`/`SEND_GAME_RESULT`/`LEAVE_*`.

### 5.2 El chequeo de conexión en el relé (L, `ch0_handshake._r73646_*`, `coop_check_route.py`)
Para una pareja de Divisiones el cliente, tras el estado 3, abre las claves 14 y 15. El relé las responde con `90 K` y las mantiene «vivas» (puede mandar ack/almacén en ellas) mientras el intento siga abierto; medido: sin intento vivo (par sin sala sintética) las claves se cerraban y el cliente informaba `ER_CONNECT failed_reason 6` a los ~60 s.
En Cooperativo el chequeo cruza mensajes entre las dos salas (rutas `PAIRED_STREAM_CHANNELS` = 9, 10, 15). Lo que significa «la prueba de conexión» en sí (qué bytes viajan por 14/15) **no está claro (P)**: PESBUL solo prueba que mantener las claves abiertas y ligar al par bastaba, y que el servidor debe empujar el `CHECK_RES` cuando los dos informan bien (política suya, no regla de Konami).

### 5.3 Qué hace falta de verdad para pasar «Revisando estado de conexión» (C = comprobado en vivo el 10-nov-2026 con el PES base y `tools/rival_falso.py --revision`)

**Resultado en una frase:** el chequeo de conexión de Divisiones NO es solo mantener abiertas las claves 14 y 15. Cada PES manda por el canal 15 un `CMD_REVISION_CHECK` al otro y espera el del otro con un `pid` que case con el que el servidor le dio en el roster. Con eso el PES manda `CMD_SET_GAMERELAY_CHECK_RES` con `NOERR` en 10-20 s.

**Cómo se llegó (para replicarlo y no repetir errores).**
1. Pruebas 1-5: el PES se ligaba bien al relé (KX, canal 0, claves 5/14/15, lista `1F C1 80` / `1F C0 81`), pero vencía a los ~80 s con `ER_CONNECT` motivo 6 y luego `SEND_MATCHING_RESULT` `GNCX_0004`. Un rival falso solo por TCP, o uno que por UDP solo abría claves, no cambiaba nada.
2. Lo que el PES manda por el canal 15 (relé con cada bloque como mensaje propio, commit 9d2ca4f). Framing: primer trozo `[0x40][largo total comprimido u8][hasta 14 bytes]`, trozos siguientes `[0x80|n][n bytes]` (n ≤ 15). Concatenado y descomprimido con zlib sale un mapa msgpack. El primero es `CMD_REVISION_CHECK` (221 B comprimidos, 231 descomprimidos): `{msgid, rqid 15, pid 68157440, platform_id (bin: np_account_id 0, xuid_one ''), name 'FRALEX', revision 'unknown/unknown', param_check_sum 269B101E0AA5231E2760C44A90BF3FE1, entry_check_sum 23A4AEB624268875E0A16449669DC753, is_request 1}`. PESBUL lo documenta igual (un REVISION_CHECK por par; sin él, «Revison check TIMEOUT»).
3. Si el rival devuelve una copia con otro `pid` inventado (68157441) el PES contesta con `is_request 0` (acepta el formato) pero el chequeo sigue venciendo. Con `pid` = 32 bits bajos del pid de 64 bits que el servidor dio a ese jugador en `CMD_WATCH_MATCHING FINISH` (rival: 0x0110000100001234 → 0x1234 = 4660) el PES pasa. Ese fue el hallazgo clave.

**Regla del pid (C).** El `pid` de 64 bits que el servidor pone en cada miembro de `FINISH` (p. ej. 292733975847239680 = 0x0410000004100000 para el PES) tiene como 32 bits bajos el entero nativo que ese cliente usa en sus mensajes del canal 15 (0x04100000 = 68157440). El otro PES valida que el `pid` del `REVISION_CHECK` del par coincida con los 32 bits bajos que el servidor dijo para ese par. Consecuencia: los pids de 64 bits que damos deben ser coherentes con lo que cada cliente cree ser, y dos PES con el mismo SteamID compartido darían el mismo pid nativo (hay que dar low32 distintos por PC).

**Prueba A/B (10-nov, 17:28).** Con `identidad.pid_forzado {"192.168.100.236": "0110000100005678"}` el PES mandó `pid: 22136` (= 0x5678) en su REVISION_CHECK y pasó el chequeo igual → **caso B: el PES usa el pid que le da el servidor** (no el de su SteamID). Basta con que el servidor dé low32 distintos por clave (commit ae851ea).

**Secuencia completa observada desde FINISH (un PES real invitado id 1 + rival falso anfitrión id 0).**
- 0 s: TCP `CMD_WATCH_MATCHING FINISH` (miembros con `matching_order` 0/1, `is_owner` YES/NO, `pid` de 64 bits). El servidor abre la partida sintética 700000000+n y el relé congela la lista.
- +0,1 s: HTTP `POST /get_gamerelay` (msgid, pid, rqid, urltype) → msgpack con `gamerelay_web_url`, `gamerelay_num`, `gamerelay_list` y los campos SUELTOS `gamerelay_index`, `ip_addr`, `port`, `grid` (d49f06e; sin ellos el resultado queda «NotImplement»). TCP `CMD_GET_MATCHID` → `match_id` TEXTO «PHOENIX-<10 dígitos>» + `upload YES`.
- +0,3 s: UDP del PES: KX nuevo, canal 0 (SYN/SYN-ACK/ACK), ligado a la partida, abre claves 5, 14, 15. Lista del canal 5.
- +0,3…1 s: canal 15, PES → par: `CMD_REVISION_CHECK` (petición, `is_request 1`); el par responde con `is_request 0` y manda su propia petición; el PES contesta `is_request 0`.
- +1…10 s: el PES, con la revisión aceptada, manda TCP `CMD_SET_GAMERELAY_CHECK_RES` NOERR. Cuando ambos informaron NOERR, el servidor empuja `CMD_WATCH_GAMERELAY_CHECK_RES` {rqid 0, result NOERR, error_code NOERR}. Después TCP `CMD_SEND_MATCHING_RESULT` (error_code vacío = éxito).
- Pantallas del PES: «Revisando estado de conexión» → «Ver rival – Estado de conexión con rival. ¿Continuar?» (3 barras; pide pulsar Continuar) → «Se están enviando y recibiendo los datos necesarios para jugar» (Local rival / Visitante PES).
- Canal 15, mensajes de aplicación (msgpack comprimido con el mismo framing; el servidor solo los reenvía): `CMD_READY_FLAG {rqid 0, flag 1}` y `CMD_ALLREADY_FLAG {rqid 0, flag 0}`; `CMD_USER_DATA_SYNC {rqid 1, pid, platform_id, name, pad_no 0, personal_data 0, side 'AWAY' (invitado; el anfitrión 'HOME'), guest_num 0, guest_list [], f2p 'NO', net_link_type 'NET_LINK_TYPE_WIRED', game_mode 24}`; luego `READY_FLAG 3` + `ALLREADY 1` y `READY_FLAG 7` + `ALLREADY 2`: cada paso avanza cuando el par devuelve el suyo (el rival falso devuelve copias).
- TCP en esa fase: `CMD_CHANGE_GAMEPHASE` (NOERR), `CMD_GET_MATCH_ANALYZE` ×2 con `{analyze_data_type, client, pid, rqid, svrtype, target_pid}` (SIN manejador: se responde NOERR vacío), `CMD_GET_PLAYER_PARAMETERLIST` (NOERR vacío), heartbeats, `CMD_START_SEND_GAME_SEQUENCE_DATA` y varios `CMD_SEND_GAME_SEQUENCE_DATA` (copias TCP de los mensajes del 15 con `fromId` propio).
- Pantalla morada con logo PES2021 unos 60 s y, a ~66 s, `CMD_SEND_ABNORMALEND` con `GNCX_0011_0x20000003`: aborta porque la CARGA del partido (canales 9/10/12) no llega; un rival falso no la puede generar. Hace falta un segundo PES real.

**Lo que NO hizo falta.** En 1 contra 1 de Divisiones no se necesitó inyectar nada por la clave 13 ni mandar mensajes por la 14. Bastó: ack vacío en las claves 14/15 (nuestro relé ya lo manda en cada respuesta), reenviar fielmente el canal 15 (cada bloque un mensaje, cabecera `0x4000|id<<4`) y descartar solo las copias TCP cuyo `fromId` no sea el del emisor (el PES también sube copias de lo recibido con el `fromId` del par: es normal; avisos «GameSeq con fromId 0 distinto del id 1»).

**Mecanismo explicado por PESBUL (L, `ch0_handshake` r7.3.646).** Abrir una clave NO engancha la «compuerta de envío» del cliente por canal; solo la engancha el primer ack recibido en ese canal (aunque vaya vacío). Sin ese ack el PES no puede transmitir su chequeo (PESBUL medía 60 s de silencio y `ER_CONNECT` 6). Nosotros ya mandamos secciones de ack para las claves abiertas, por eso el PES sí transmite.

**Dependencia de anfitrión/invitado (C).** El anfitrión es el que buscó primero (`enter_no` 0, id 0, `is_owner YES`, `side HOME`); el invitado es id 1, `side AWAY`. En las pruebas 6a-6g y A/B el PES fue invitado. Con el PES como anfitrión (PES busca primero) todavía no se probó con `--revision`.

---

## 6. Lo que falta en nuestro servidor y plan «Fase 2-3: lo que hay que implementar»

### 6.1 Estado actual de `phoenix_online/gamerelay.py` (L, leído hoy)
Tiene: capa exterior RC4+retroalimentación, marco con checksum (y zlib), KX con clave M, transporte con ARQ por canal (una clave por elemento), canal 0 (SYN/SYN+ACK/ACK/vida), canal 1 (abrir/cerrar), canal 5 (lista de 2 bytes solo para sonda), canal 6 (eco × cuenta) y respuesta a cada datagrama.
Diferencias con PESBUL que hay que corregir antes de reenviar nada:
1. **Clave ARQ por bloque, no por elemento.** Nuestro `esperado += 1` por elemento falla con elementos de varios bloques (típico en 9/15). Hay que contar bloques (`[largo][bytes]`) y avanzar por cada uno.
2. No hay búfer de claves futuras (descartamos y esperamos retransmisión): funciona pero convierte cada reordenación en un tirón; conviene el búfer acotado.
3. No hay tope de 2048 bytes por datagrama ni selección de prefijo: hoy mandamos TODOS los pendientes en cada respuesta; con el juego real un datagrama grande lo tira el cliente entero (y entonces nunca confirma). Hay que empaquetar hasta 0x800 en orden de clave.
4. Lista B: solo un grupo; PESBUL admite dos (`0x80` y `0x40`).
5. Sin cabecera de eco RTT (flags 0x80 del cliente → respuesta con 0x40 + echo_seq + hold_ms).
6. Sin envío espontáneo: solo respondemos a datagramas del cliente. Para el reenvío necesitamos «empuje inmediato» al receptor (con respaldo por sondeo).
7. Sin emparejamiento (NTL → sala/pid), sin intento de partida (generación/lista congelada), sin sala sintética para parejas de Divisiones, sin reenvío 9/10/12/15, sin la rama TCP de GameSeq ni la barrera de `CHECK_RES`.
8. `sonda_autorizada` mira solo IP; la ligadura de partida necesita (ip,puerto) exacto con respaldos.

### 6.2 Fase 2 (dos jugadores ligados, lista de 3 bytes, chequeo de conexión) — en orden de dependencia
1. **Parches de transporte (§6.1 puntos 1-5)** y pruebas con datagramas sintéticos; sin esto lo demás se rompe en silencio.
2. **Guardar la declaración NTL** al recibir `CMD_SEND_IPANDPORT_NTL` (ya se acusa; falta decodificar `sa_data` y almacenar host/reflexive por sesión+pid, más la IP TCP).
3. **Tabla de emparejamiento** `(ip:puerto) → (sala, pid, enter_no)` en memoria (un solo proceso) con: coincidencia exacta reflexiva→host, «IP única» sobre toda la tabla, ambiguo = rechazo, alias LAN (IP TCP + puerto declarado). Prueba específica: dos PES con la misma IP pública (casos reales de LAN doméstica) → exige endpoint exacto.
4. **Sala sintética + intento de partida para la pareja de Divisiones/Partida rápida:** al emitir `WATCH_MATCHING FINISH`, crear la sala sintética (anfitrión enter_no 0, invitado 1), congelar la lista y abrir el intento con generación incremental; al terminar, cerrar generación (reinicio de pares en su próximo KX). Para salas libres/Team Play (si se quisieran) disparar en la transición a JUEGO.
5. **Lista del canal 5 de 3 bytes** `1F, C0|id_propio, 80|id_otro` desde la lista congelada, tras el OPEN de la clave 5 confirmado; reglas de ids (0..n-1 por (enter_no, pid), 31 servidor, sin repetidos), retransmisión hasta ack.
6. **Claves 14 y 15 (y las de juego que abra el cliente):** responder `90 K` (ya), mantener acks, y que una sesión sin intento vivo siga siendo sonda (no mezclar).
7. **Barrera `CHECK_RES`:** al recibir `SET_GAMERELAY_CHECK_RES` con `NOERR` de los dos de la pareja, empujar `WATCH_GAMERELAY_CHECK_RES` (compresión activada, `rqid 0`, `result NOERR`, `error_code NOERR`) a cada uno, una sola vez por generación. Confirmar con el cliente: aparece `SEND_MATCHING_RESULT`.
8. **Acks TCP del resto de comandos** de §5.1 (varios ya están: comprobar que ninguno quede sin respuesta, sobre todo `SET_GAMEPLAYER_PLAYTIME`, `SET_GAMEENV`, `SET_GAMEMEMBER`, `SET_GAMETEAM`, `CHANGE_GAMEPHASE`, `SEND_MATCH_ANALYZE`).
9. **Eco RTT completo** y keepalive del canal 0 verificados con el cliente real.

### 6.3 Fase 3 (reenvío del partido)
10. **Rama TCP `CMD_SEND_GAME_SEQUENCE_DATA`:** decodificar `game_seq_data` (base64 → cabecera de 9 bytes), validar `type 2`, canales 15/10/12, `fromId` = id del emisor; construir `0x4000|(id<<4)` + carga y encolarla en el almacén del OTRO jugador, troceada en ≤255.
11. **Reenvío nativo de los canales 15, 10, 12:** lo recién aceptado por la recepción de cada par se encola en el otro con la misma trama del punto anterior (2 bytes de cabecera + carga), numeración propia por receptor.
12. **Deduplicación nativo↔TCP** por (sala, generación, emisor, canal, contador): primera copia entrega, la otra se suprime si los bytes coinciden (en duda, entregar de más).
13. **Canal 9:** reenvío a todos (incluido el emisor, salvo bucle local declarado) con **un byte de etiqueta `id_emisor<<3`**; un trozo por clave de receptor; empuje inmediato al receptor.
14. **Observar y, si hace falta, inyectar el canal 13 (MatchControlInfo V=1)** y la política de arranque; empezar sin ella y activarla si el cliente se queda parado al arranque.
15. **Retirada de participante y cierre de generación:** `LEAVE_GAME`/`LEAVE_ROOM`/`SEND_GAME_RESULT`/`SEND_ABNORMALEND`/caída TCP → borrar el estado del par y cerrar generación; el superviviente decide por TCP (forfeit) — sin mensaje UDP de cierre.
16. **Pruebas en vivo con dos PES** (otra PC o dos instancias en red): registrar byte a byte (captura completa en el log del servidor), porque el contenido real de 9/12/15 es la parte menos validada.

### 6.4 Lo que se puede dejar fuera (L: es para varias salas/nodos o muchos jugadores)
`edge_fabric`, `remote_room_rpc`, `fabric_lifetime`, `lobby_runtime` (rutas remotas), `backend_runtime`, la sala coop de dos salas, el modo de ritmo fijo (`paced_relay`), `native_departure` (solo ≥3 humanos), `startup_credit/measured/prefix` (afinado de arranque con varios mandos). Con un solo nodo y 2 jugadores no hacen falta.

---

## Tabla final: Pendiente / dudas (P)
| # | Duda o pendiente | Por qué importa | Cómo resolverlo |
|---|---|---|---|
| 1 | Vigencia de la autorización de la sonda: 15 s (variable del relé) frente a «hasta 86 400 s» (nuestro doc) | sondas que caducan antes de abrir el canal 6 | ver el archivo de autorización del HTTP o medirlo en vivo |
| 2 | ¿El amistoso 1v1 de sala libre usa realmente solo P2P+TURN? (L en PESBUL; no probado por nosotros) | decide si necesitamos TURN además del relé | prueba con dos PES en amistoso y mirar si llega UDP al relé |
| 3 | ¿La entrada de mandos de Divisiones 1v1 viaja por el canal 9 del relé o por P2P? | todo el reenvío del 9 depende de ello | capturar la partida real y ver qué claves abre el cliente tras la carga |
| 4 | Respuesta y campos de `SET_GAMERELAY_MATCH_QUALITY` y `SET_LOCAL_GAMERELAY_LIST` | si el cliente los envía y esperan algo concreto | capturar en vivo; hoy solo `NOERR` genérico |
| 5 | Qué viaja por las claves 14 y 15 en el chequeo de conexión | podría haber un intercambio entre pares que no hemos visto | captura completa de un chequeo real |
| 6 | Cuántos y cuáles hacen falta del canal 13 (MatchControlInfo) para que el cliente arranque | si falta, el partido se queda parado | probar sin inyección y con V=1 |
| 7 | Contenido y tamaño máximo real de los mensajes de 9/12/15 | dimensiona el empaquetado (2048 B) y los trozos | captura byte a byte en la primera prueba con dos PES |
| 8 | Qué hace el cliente superviviente al irse el otro (forfeit, cierre de sesión) | resultado y puntuación | probar abandono real; mirar `SEND_ABNORMALEND`/`LEAVE_GAME` |
| 9 | Migración de endpoint (cliente que cambia de puerto) y reintentos tras reinicio del par | robustez en redes reales | pruebas con NAT real; revisar `_r73581_try_endpoint_migration` |
| 10 | Respuesta exacta a `SEND_MATCHING_RESULT` y cuándo la espera el cliente | si no se contesta, error a los 30 s | observar en vivo junto con el `CHECK_RES` |
| 11 | Formato exacto de la clave tras el KX si se reinicia la sesión en mitad de partida | pérdida de sesión | ya cubierto en fase 1 (M); verificar en partida larga |
| 12 | Con 2 jugadores en LAN con la misma IP pública, ¿exactitud del endpoint basta? | ligado falla por ambigüedad | prueba con dos PCs en la misma red doméstica |
