# Estudio de la copia local de PESBUL con el PES base (4-nov-2026) — vivo, se amplía

Fecha: 4-nov-2026. PES base: `C:\PES21\eFootball PES 2021\PES2021.exe`, sin Sider, CPY.ini del crack sin tocar.
Servidor: la imagen pública de PESBUL descargada de Docker Hub (digest df627d1c…) corriendo en la propia PC de Frank
(`D:\Frank\IA\CLAUDE\pesbul_local\app`), con `hosts` apuntando los 7 dominios de Konami a la IP LAN de la PC.
Reglas: descrito con palabras nuestras; sin copiar su código; sin claves; nada se sube (copias en
`pes-online\capturas_pesbul_local\` y `capturas_pesbul\local_20261104\`, fuera de repos/Drive).
Marcas: **C** = comprobado en vivo en `events.log`; **P** = suposición/lectura de documentos, sin comprobar.
Límite honesto: el servidor corre en modo «silencioso» (PES_QUIET_LOG=1): `events.log` guarda eventos resumidos (con campos
útiles) pero NO la captura completa de bytes (la carpeta `data\captures` quedó vacía), y los comandos que el servidor
trata de forma rutinaria no siempre dejan fila. Para ver cada byte habría que reiniciarlo con registro completo (no hecho).

## 0. Resumen de lo comprobado (C)
Con el mismo PES base y la misma cuenta del crack con la que el servidor público contestó `ERR_FORBIDDEN`, la copia local
ACEPTÓ el login (`REAL_CMD_LOGIN_RX` + `IDENTITY_STEAM_SESSION_BOUND`) y dejó recorrer: menú online, salón (sala), Partida rápida
(buscar y cancelar), comprobación de red/gamerelay (HTTP + UDP, canales 5 y 6, RTT) y entrar a Cooperativo / VS COM.
No hay rechazos ni errores del servidor; solo avisos de «peer bind failed» al no haber otro jugador.

## 1. Login (C)
- El gate recibe el `DISCONNECT logoutstat=MOV` del cliente y lo devuelve (eco con NOERR); el cliente se pasa a la sesión de menú.
- `CMD_LOGIN`: el `username` es el SteamID64 en **hexadecimal sin ceros a la izquierda**; el servidor lo convierte a decimal
  (`410000004100000` hex → `292733975847239680` dec) y liga la sesión por ese identificador (`session_bound`).
- Perfil: `PROFILE_RESOLVED` con resolución `session_bound_steam_exact`: busca/crea el usuario por ese SteamID exacto (db_user_id 1).
- La lista de URL incluye las claves de ranking (`RANK`, `DIVISION_RANK`, dos de clan) y la de calidad del gamerelay
  (`/gamerelay/qualitychecklist`): su código anota que si el cliente no encuentra la clave exacta falla el envío antes de abrir socket.
- Puertos: gate/cuenta TCP 10000, menú TCP 10001, HTTP 5000 (dashboard + servicios), UDP 3478/5730/5739/5740, gamerelay UDP 5100 (P: viene
  de la lista de nodos de `qualitychecklist`).
- Falta en nuestro `logica.py`: nada obligatorio para este tramo (nuestro servidor ya lo cumplía); sí comparar la resolución de perfil por SteamID.

## 2. Menú online (C)
Tras el login, PES pide en cadena: envío de direcciones de red (`SEND_IPANDPORT_NTL`, con host/reflexive/relayed y tipo NAT), lista de equipos
disponibles (`GET_GAME_TEAM_LIST`: 688 de 693 equipos del catálogo), entorno del servidor (`GET_SERVER_ENV`: `max_division` D12, el cliente sólo lee
texto) y análisis de partidos (`GET_MATCH_ANALYZE_RESULT`, tipo `ONLINE_DIVISIONS`, 0 partidos). Es el mismo recorrido que vimos con nuestro servidor.
- Falta en nuestro servidor (P): devolver una lista de equipos real (nosotros la mandamos vacía) y `MATCH_ANALYZE` con el tipo de datos correcto.

## 3. Salón de juego / sala (C)
Frank entró a «[1001]Salón FRALEX». Cliente: `CMD_ENTRY_GAME` (entry FALSE, lado HOME). Servidor: decide si la entrada cuenta, responde con
`entryflag` y `gameNum` (0 partidos) y difunde el resumen de la sala (`ROOM_BROADCAST`, un solo usuario vivo, sala 1001, lobby 1, tipo `OC_FREE`).
Pantalla (foto): Local FRALEX, Visitante vacío, texto «espera a tener suficientes usuarios». Botones: Empezar partido, Cargar datos de usuario,
Añadir Invitado, opciones de salón y generales, Salir. Al salir: `CMD_LEAVE_ROOM` y el servidor borra la sala y la difunde vacía (`ROOM_DELETE_REBROADCAST`).
- CORRECCIÓN (revisado en `logica.py`, 4-nov): nuestro servidor SÍ tiene salas: `c_cmd_createjoin_room`, `c_cmd_join_room`, `c_cmd_entry_game` y `c_cmd_leave_room`.
  Lo que falta (P) es compararlas con el comportamiento de PESBUL (difusión de sala a los demás, borrado y reenvío al salir, lados Local/Visitante, invitado) y,
  sobre todo, el gamerelay UDP que se usa al empezar el partido: ver `ESPEC_GAMERELAY.md`.

## 4. Partida rápida (C)
Orden: `GET_QUICKMATCHOPTION` (opciones: calidad de ping nivel 3, rating 1, reputación buena) → `START_MATCHING` (tipo `OC_QUICK`) → el servidor registra
la búsqueda en cola, acusa recibo con solo msgid/rqid/result y EMPUJA dos cosas: la condición de emparejamiento (`WATCH_MATCHING_CONDITION`: manner A..AAA,
rating 0..9999) y el estado «buscando» con la tarjeta propia (`WATCH_MATCHING SEARCHING`, member=[yo]). Cancelar: `CANCEL_MATCHING` → NOERR + `WATCH_MATCHING CANCEL`,
luego `LEAVE_ROOM`. Nos falta (P): lo que hace al encontrar pareja; no ocurrió porque no hay un segundo jugador.
- Nuestro servidor ya hace todo esto (lo implementamos igual hasta «buscando»); falta la pareja (FINISH + chequeo de conexión).

## 5. Comprobación de red / gamerelay, HTTP + UDP (C) — parte NUEVA para nosotros
1. TCP: `GET_GAMERELAY_QUALITY` / perfil de ralentización (`OD_NATIVE_SLOWDOWN_PROFILE`, 60 s, alcance RANKMATCH).
2. HTTP 5000: `POST /gamerelay/qualitychecklist` con el pid; el servidor responde una lista de nodos [índice, IP, puerto 5100, …] y autoriza una sonda
   (token válido para el pid, 86 400 s, hasta que se desconecte).
3. UDP al gamerelay (puerto del nodo): el cliente abre el canal 5 (lista de participantes: el servidor emite una lista de 2 miembros —«yo» y «servidor»— con
   número de secuencia y retransmite hasta recibir el ACK) y luego el canal 6 (eco de calidad: 8 paquetes de 5 bytes que el servidor devuelve idénticos).
   Con 1 solo jugador el servidor «falla cerrado» (`GAMERELAY_PEER_BIND_FAILED`, no hay candidatos) pero deja pasar la sonda porque la autorizó el HTTP.
4. Eco RTT cada segundo (el servidor devuelve la secuencia del datagrama) durante el chequeo; la sonda caduca a los ~27 s (≈542 trozos, 2 710 bytes de eco).
5. TCP: `GAMERELAY_QUALITY_RX/STORE/TX`: el cliente informa la calidad medida (18 ms) y el servidor la guarda por pid y contesta NOERR.
- Falta en nuestro servidor: TODO esto. Nuestro `SEND_IPANDPORT_NTL` anunciaba 127.0.0.1 y no teníamos relay UDP ni `qualitychecklist`. Para jugar con otra PC hay
  que implementar un relay UDP con canales (5/6) y el HTTP de calidad, o aceptar la limitación de «partida rápida sin partido».

## 6. Cooperativo / VS COM (C parcial)
Frank entró a Cooperativo; el servidor recibe 6 veces `CMD_GET_COOP_PROFILE` (3 campos: svrtype, client y un tercero) — descubrimiento de campos de la sala VS COM.
Aún no se ha observado la cuenta atrás de 30 s ni el partido (P: su documento dice que el flujo posterior a la sala sigue en investigación).
- Falta en nuestro servidor: el perfil de co-op y toda la máquina de estados de sala co-op.

## 7. Resumen de lo que le falta a `logica.py` (nuestro servidor) comparado con esto
1. Relay de juego UDP (canales 5 y 6, eco y RTT) + HTTP `qualitychecklist`/sonda (el punto crítico para jugar con otra PC).
2. Salas/salón (ids, dueño, lados, invitados, difusión y borrado).
3. Emparejamiento real de dos jugadores (FINISH, chequeo de conexión) y sus empujes.
4. Perfil de co-op (`GET_COOP_PROFILE`), `GET_MATCH_ANALYZE` con tipo `ONLINE_DIVISIONS`, lista de equipos real.
5. STUN que devuelva la dirección observada del cliente (el público contesta STUN real; la copia local usa 3478/5730/5739/5740).
6. Búsqueda por SteamID exacto (hex→decimal) para ligar sesión y perfil.

## 7b. Resumen final de la sesión del 4-nov (20:05-20:19 UTC)
Cronología (C): 20:10:58 el PES base conecta al gate local → 20:11:04 login aceptado (sesión ligada al SteamID del crack) → 20:11:21 menú online (red,
lista de equipos, entorno) → 20:12:08 entra al Salón [1001] (ENTRY_GAME) → 20:13:26 sale (LEAVE_ROOM, sala borrada) → 20:13:54 Partida rápida: START_MATCHING,
«buscando», 20:14:02 cancela → 20:14:18 comprobación de red y gamerelay (HTTP + UDP canales 5/6 + RTT) → 20:14:50 Cooperativo / VS COM (GET_COOP_PROFILE ×6) →
varias comprobaciones de red más → 20:18:55 el cliente cierra limpiamente (`client_eof`, sesión cerrada, sonda revocada).
Cierre de PES: sin crash (Windows no registra ningún evento 1000/1001 a las 15:18 locales; los dos APPCRASH de hoy son anteriores y de otra sesión).
No se llegó a: cuenta atrás VS COM de 30 s, partido, emparejamiento con otro jugador.
Lecciones: (1) el servidor público rechazaba el login de este PES; la copia local no (aquí «protected ids» va apagado); la causa del rechazo público sigue sin conocerse.
(2) El registro de la copia local, por defecto, es resumido y la captura completa se escribe en una ruta de Linux (`/app/data/captures`) que en Windows cae en
`D:\app\data\captures` (solo apareció `auth_steam_http`). Se preparó `iniciar_local_completo.cmd` (sin ejecutar) que pone registro completo y la captura en `data\captures`.

## 8. Cómo repetir
Servidor: `D:\Frank\IA\CLAUDE\pesbul_local\app\iniciar_local.cmd` (adaptado). Hosts: `pesbul_local\poner_hosts_local.ps1` (administrador) y `-Restaurar` para volver.
PES: `C:\PES21\eFootball PES 2021\PES2021.exe` sin Sider; entrar con Enter hasta el menú EF. Registros: `data\events.log` y copias en `capturas_pesbul_local\`.
