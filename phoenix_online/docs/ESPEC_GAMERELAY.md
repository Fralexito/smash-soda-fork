# Especificación del gamerelay de PES 2021 (lo que hace falta para que nuestro servidor lo atienda)

Fecha: 4-nov-2026. Fuente: lectura de la copia local de PESBUL (`pesbul_local\app`, solo lectura) + lo visto en vivo en `events.log`
de hoy. Todo está descrito con palabras nuestras: no se copia código ni se incluyen claves (las dos claves RC4 de la capa cifrada son
constantes del propio cliente PES; se sacan del exe, no se pegan aquí).
Marcas: **C** = visto hoy en el registro del servidor; **L** = leído en su código/documentos; **P** = suposición.
Límite: `relay_server.py` pesa 455 KB y no se leyó entero; las reglas exactas de reenvío entre jugadores quedan en §6 marcadas como L parcial/P.

## 0. Panorama: qué piezas intervienen
Para que un partido pueda empezar PES necesita, además del TCP 10000/10001 y el HTTP 5000:
1. Saber DÓNDE está el relé (IP y puerto UDP): lo recibe por HTTP/TCP (`qualitychecklist` y `GET_GAMERELAY`).
2. Medir cada candidato abriendo una sesión UDP real contra él (sonda de calidad) y reportar el resultado por TCP.
3. Abrir otra sesión UDP, ya con la sala creada, contra el nodo asignado; el relé asocia al jugador con su sala y reenvía los datos entre pares.
Hoy (C) PES hizo 1 y 2 completos contra la copia local; 3 no se ha visto (un solo jugador, nunca se llegó a partido).

## 1. HTTP: POST /gamerelay/qualitychecklist (C, L)
- Ruta: `/gamerelay/qualitychecklist` (PES la saca de la URLLIST, clave exacta `GET_GAMERELAY_QUALITYCHECKLIST`; si falta falla en cliente antes de abrir socket). L
- Petición: formulario HTTP (o query) con `msgid=CMD_GET_GAMERELAY_QUALITYCHECKLIST`, `rqid` y `pid`; el servidor acepta también cuerpo msgpack. C (campos vistos: path, pid, rqid).
- Respuesta (msgpack, en este orden): `msgid`, `rqid`, `result` = NOERR; `quality_check_data_list` = lista de candidatos y `quality_check_data_num` = cuántos;
  `quality_threshold_1`, `quality_threshold_2` (umbrales en ms, política de ellos; PES los usa para clasificar la calidad); `quality_list` = mediciones ya conocidas y `quality_list_num`. L
- Cada candidato: `gamerelay_index` (entero ≥ 1, identifica al nodo), `ip_addr` (texto), `port` (UDP del relé) y `grid` (entero, 0 si no se usa). Ejemplo en LAN (C): índice 1, IP de la PC (192.168.100.236), puerto 5100, grid 0.
- `quality_list` vacía = «mídelo tú»: PES vuelve a medir. Con elementos: cada uno `{gamerelay_index, quality (ms), last_update (epoch en segundos)}`. `last_update` 0 = medir de nuevo. L
- Efecto lateral: el servidor AUTORIZA una sonda UDP desde esa IP para ese índice durante un tiempo (política suya: la sesión del pid, tope 86 400 s) y la revoca al desconectar el TCP del pid. C
- La misma petición existe también por TCP (mismo cuerpo); y el comando `CMD_GET_GAMERELAY` (sin «QUALITYCHECKLIST») devuelve `gamerelay_web_url` (la URL base del HTTP) y `gamerelay_num`/`gamerelay_list` (el nodo asignado a ese jugador). L

## 2. Capa UDP del relé (C, L)
Puerto por defecto 5100 UDP (también escucha TCP en el mismo número por si acaso). C (el nodo anunciado es `IP:5100`), L.
Pila, de abajo arriba:
1. **Cifrado exterior**: cada datagrama va cifrado con RC4 con «retroalimentación del texto cifrado» (cada byte se mezcla con el anterior cifrado; vector inicial 0xEA). Una clave constante de 256 bytes del cliente. L
2. **Marco**: `[flags (1 byte)][suma de control RFC-1071 en 2 bytes, big endian][cuerpo]`; mínimo 3 bytes; bit 7 de flags = «aceptado» (debe ir a 1); bit 6 = cuerpo comprimido (el servidor no lo usa). El checksum se calcula con ese campo a cero. L
3. **Intercambio de claves (KX)**: el cliente manda un marco de 7 bytes (3 + nonce de 4 bytes); el servidor responde un marco de 259 bytes (3 + 256 de material derivado, cifrado con RC4 y otra clave estática de 16 bytes del cliente). Tras eso comienzan los datagramas de transporte. L
4. **Cuerpo del transporte** (lo que queda tras descifrar, validado contra 96 datagramas reales): `[número de secuencia del datagrama u32 big-endian][00]` y a continuación una CONCATENACIÓN de secciones. L
5. **Sección** (una por canal activo): `[canal<<4][ (canal<<4) | 0x02 si trae lista A | 0x01 si trae lista B ][ack u32 BE]` y luego, si hay, la lista A y la lista B.
   - **Lista A** (elementos fiables): `[cuenta-1 (1 byte)]` y por elemento `[secuencia del elemento u32 BE][largo-1 u16 BE][carga]`.
   - La carga son «bloques» `([largo u8][bytes])*`.
   - El **ack** es «el siguiente que espero», no «el último recibido»: para confirmar el elemento 0 hay que mandar ack = 1; un ack mayor que lo enviado es error del cliente. L
   - El cliente rechaza el datagrama entero si no consume exactamente todos los bytes. L
   - Siempre van DOS secciones: canal 0 y canal 1 (aunque estén vacías). L
Ejemplo real (C, canal 5): `50 52 | 00000000 (ack) | 00 (1 elemento) | 00000000 (seq 0) | 0002 (3 bytes) | 02 1f c0` → una carga con un bloque de 2 bytes `1f c0`.

## 3. Canales (L, con C donde se vio)
- **Canal 0 (control, apretón de manos de 3 pasos)**: bits `SYN`=bit 0, `ACK`=bit 3; byte = `bit0<<7 | … | bit5<<2`.
  Cliente manda 0x80 (SYN) → el servidor responde 0x90 (SYN+ACK) → el cliente manda 0x10 (ACK) y entra en «establecido». Sin el 0x90 el cliente reenvía el SYN para siempre. L
  Después hay un par de «vida»: el cliente manda 0x08 (sonda) cada cierto tiempo y el servidor contesta 0x04. L (con aviso de que la respuesta 0x04 es la parte menos probada).
- **Canal 1 (control de flujos)**: mensajes de 2 bytes `[op][clave K]`: `80 K` = ABRIR (el cliente), `90 K` = ABRIR-ACEPTADO (el servidor, con la MISMA clave K que llegó), `40 K` cerrar, `50 K` cerrar-ack, `20 K` reset. Abrir con clave K crea el canal lógico K (K ≥ 2). L
- **Canal 5 (lista de participantes)**: la carga es una secuencia de bytes, UNO por participante:
  - bit 7 = 0 → el SERVIDOR; bit 7 = 1 y bit 6 = 1 → «YO» (el receptor); bit 7 = 1 y bit 6 = 0 → OTRO jugador;
  - bits 4..0 = id de 5 bits (0..31), ÚNICO dentro del mensaje (un id repetido hace que el cliente tire TODO el mensaje).
  - Debe incluir exactamente un registro de servidor y uno de «yo». Entonces la conexión pasa a estado 3 («establecida»), requisito para que PES cree la sesión de partido. L
  - Su cuerpo en la práctica (C, sonda): `1f c0` = servidor con id 31 (0x1F) y yo con id 0 (0xC0|0).
  - Reglas de ids: 5 bits, distintos entre sí, estables durante la sesión, y el MISMO participante debe llevar el MISMO id en la lista de cada receptor. El servidor reparte por orden de entrada a la sala (dueño 0, siguiente +1) y reserva el 31 para sí. Orden de emisión: servidor, yo, el resto. L
  - Fiabilidad: va por la lista A, el servidor RETRANSMITE hasta recibir el ack (visto: una retransmisión y el ack llega) y luego la «retira». C
- **Canal 6 (medición de calidad, solo en la sonda)**: el cliente manda registros de 5 bytes `[cabecera][4 aleatorios]`; la cabecera = `((cuenta-1) << 3) | (tamaño-1)`; visto `0x0c` (cuenta 2, tamaño 5) en la sonda de Team Play y `0xAC` (cuenta 22) en lobby normal. C
  El servidor debe devolver los bytes IDÉNTICOS en el mismo canal, y repetirlos `cuenta` veces (si no, la latencia medida sale absurda). Tras 5 s el cliente calcula la media en ms. L, C (8 ecos de 5 bytes vistos).
- **Canales de juego (claves 9..15)**: lo abre el cliente en partida real (claves 9, 10, 11, 12, 13, 14, 15). Si una sesión sin sala abre cualquiera de ellas deja de tratarse como sonda. El relé las usa para reenviar los datos del partido entre pares. L
- **Eco RTT**: en el canal de datos, a cada datagrama del cliente el servidor devuelve la misma secuencia (el cliente mide el tiempo de ida y vuelta); visto cada segundo mientras dura la comprobación. C

## 4. Qué decide si una sesión UDP es «sonda» o «partida» (L)
- **Partida**: el relé encuentra una fila «de emparejamiento» (IP del cliente ↔ pid/sala/orden), escrita cuando el cliente crea o entra a la sala (`CREATEJOIN_ROOM`). Con esa fila el jugador queda «ligado» y entra en el reenvío de su sala.
- **Sonda** (sin sala): solo si (a) NO existe fila de emparejamiento para esa IP, (b) el control TCP/HTTP autorizó recientemente una sonda de esa IP contra ese nodo, y (c) el cliente abre el canal 6. Entonces el servidor manda la lista de participantes de 2 bytes (servidor + yo) y hace de eco. No toca el emparejamiento. Hoy (C): `no_match_zero_candidates_and_qc_authorized_index_1` → ELIGIBLE → ROSTER_TX → CONFIRMED (canal 6 abierto) → eco de 8 registros → caduca ~27 s después. «GAMERELAY_PEER_BIND_FAILED» es el aviso de que no hay candidatos de pareja (normal en una sonda).

## 5. Comandos TCP relacionados (L, C)
| Comando | Quién | Campos | Notas |
|---|---|---|---|
| `GET_GAMERELAY_QUALITYCHECKLIST` | cliente→servidor (también HTTP) | `pid`, `rqid` | Respuesta de §1. C |
| `CMD_GET_GAMERELAY` | cliente→servidor | `pid`, `rqid` | Respuesta: `result`, `gamerelay_web_url` (base HTTP), `gamerelay_num`, `gamerelay_list` (nodo asignado: índice, IP, puerto, grid). Si no hay nodo: lista vacía. L |
| `CMD_SET_GAMERELAY_QUALITY` | cliente→servidor | `msgid`, `svrtype`, `client`, `quality_list[{gamerelay_index, quality(ms), last_update(epoch s)}]` | Cada item llega como un «bin» que contiene un mapa msgpack. Fallo = el cliente manda -1 como entero sin signo de 32 bits (0xFFFFFFFF). Respuesta solo `NOERR`. C (calidad 18 ms guardada por pid e índice) |
| Perfil «ralentización» (`OD_NATIVE_SLOWDOWN_PROFILE`) | servidor→cliente | NO es comando: es el campo `proto_opt` (cadena JSON) de la respuesta a `GET_SERVER_ENV`; lleva `{"P2P":{"Match":{<clave>: 60000000}}}` (60 s en microsegundos, ámbito RANKMATCH) | C (evento «SLOWDOWN_PROFILE_TX», duración 60 000 000 µs) |
| `CMD_CHECK_LOCAL_GAMERELAY_PLAYER` | cliente→servidor | — | En su código solo aparece en una lista de comandos observados; no vi manejador propio (P: contesta NOERR genérico). |
| GET_SVRLIST | — | — | NO anuncia el relé: los tipos de servicio son GATE/ACCOUNT/MENU (+VDPCHAT en local). El relé solo se conoce por el HTTP/TCP de §1 y `GET_GAMERELAY`. C/L |

## 6. Con 2 jugadores: reenvío entre pares (L parcial, P)
- Cada jugador abre su sesión UDP contra el nodo asignado y queda ligado a su sala por la fila de emparejamiento. El servidor envía a CADA uno su lista del canal 5 con los demás participantes (cada uno ve «yo» con su id y el resto como «otros»; ids coherentes en todas las listas).
- Los canales de juego (9..15) llevan los datos de la partida; el servidor reenvía cada flujo del emisor a los demás de la sala con su propia numeración/ARQ por receptor (cada par tiene su sesión fiable). Hay un modo de ritmo fijo opcional (tick por Hz) apagado por defecto. L
- Hay un «reloj de latencia nativo» (RTT echo) que el servidor responde siempre que el plano de datos está activo. L
- NO estudiado a fondo (P): tamaños máximos por datagrama, política de pérdida y ordenado, cuenta atrás de sala (`START_COUNTDOWN`), señal «GAME» que dispara la transición al partido. Su propio documento dice que parte del flujo post-sala sigue en investigación.

## 7. Puertos UDP 3478 / 5730 / 5739 / 5740 (L)
Son los del servidor STUN/descubrimiento, distintos del relé 5100. 3478 = STUN estándar (binding request/response). 5730, 5739, 5740 los trata como «candidatos de juego o descubrimiento de PES»: solo los escucha y registra, sin protocolo propio comprobado (P: PES los usa para pruebas de NAT). Hoy (C) no llegó ningún datagrama a 5739/5740.

## 8. Lo que implicaría a nuestro servidor (resumen)
1. HTTP + TCP `qualitychecklist` y `CMD_GET_GAMERELAY` con la estructura de §1 y autorización temporal de la sonda.
2. Un servicio UDP en el puerto del nodo con: descifrado RC4/feedback, marco con checksum, KX, secciones/ARQ de transporte, canal 0 (SYN+ACK y el par de vida), canal 1 (abrir/aceptado), canal 5 (lista de participantes), canal 6 (eco ×cuenta) y eco RTT.
3. Tabla de emparejamiento IP↔pid↔sala escrita al crear/entrar a la sala, y reenvío de los canales de juego entre pares.
4. `CMD_SET_GAMERELAY_QUALITY` (guardar la calidad por pid/índice) y `proto_opt` en `GET_SERVER_ENV`.
Es un trabajo grande (el módulo equivalente de PESBUL pesa cientos de KB): conviene hacerlo por fases: 1) HTTP/TCP + sonda (para que PES no falle la comprobación de red), 2) sala + lista de participantes, 3) reenvío de juego.
