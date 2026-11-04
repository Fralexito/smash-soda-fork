# Inventario: todo lo que hace PESBUL y lo que tenemos (04-nov-2026)

Fuente: lectura (sin ejecutar) de la imagen pública noobsaibot301/pes2021-emulator r7.3.743.
Solo se anotan HECHOS (qué existe y para qué); nada de su código se copia.
Tamaño de referencia: ~52 000 líneas en su servidor TCP, ~8 500 en su relé UDP, ~200 módulos de apoyo.

Comandos del juego: PESBUL nombra 194; nosotros atendemos 78. Faltan ~115, agrupados abajo.

Leyenda: ✅ hecho y probado en vivo · 🟡 a medias · ❌ falta.

## Avance 04-nov noche (lo que va con un solo PES)
- ✅ (en pruebas automáticas, falta verlo en PES) JOIN_BLOCK, LEAVE_BLOCK, LEAVE_LOBBY, CHECK_STRING (ERR_NGWORD), SET_PLAYERPROFILE (se guarda en la tabla perfiles), GET_PLAYERINFO (perfil completo), GET_DIVISION_POINT_INFO (12 filas), GET_GAMERESULTS (vacío con no_match), GET_OFTEN_USED_TEAM_LIST, GET_RANKING / GET_DIVISION_RANKING (+_BYID).
- ✅ WATCH_SYSTEM_NOTICE, WATCH_SYNC_NOTICE, WATCH_TEXTCHAT quedan abiertos, igual que en PESBUL.
- ✅ CHECK_CHEAT, GET_DMM_NOTIFICATION, GET_URGENT_ANNOUNCEMENT, FINISH_SEASON: PESBUL también responde solo NOERR, así que ya estamos igual.

## 1. Entrada (ya cubierta)
- ✅ País, EULA, AUTH_STEAM, login, crear usuario, live update, menú online.
- ✅ HTTP (URL list, EULA, qualitychecklist), STUN básico.
- 🟡 Identidad propia (pase web en Supabase): planificado, no hecho.

## 2. Partida rápida (1 contra 1 al azar)
- ✅ Buscar y cancelar (llega a «Buscando oponente»).
- ❌ WATCH_MATCHING_CONDITION, emparejar a 2 personas, GET_MATCHID, SEND_MATCHING_RESULT, lista de rechazados (GET_MATCHREJECTLIST, SET_MATCHREJECTPID).

## 3. Relé de juego (gamerelay UDP)
- ✅ Prueba de red completa (18 ms, igual que PESBUL).
- ❌ Fase 2/3: unir a los 2 jugadores en el relé y reenviar los canales de juego 9–15. PESBUL tiene aquí casi todo su trabajo fino (orden, ritmo, reintentos, telemetría del canal 9).
- ❌ SET_GAMERELAY_MATCH_QUALITY, SET_LOCAL_GAMERELAY_LIST, GET/SEND_IPANDPORT_NTL, TURN.

## 4. Salones amistosos (vestíbulo)
- ✅ Vestíbulo, crear salón, lista de salones.
- ❌ Entrar al salón de otro (JOIN_ROOM_NTL), JOIN_BLOCK/LEAVE_BLOCK, LEAVE_LOBBY, lados y equipos (SET_GAMETEAM, SET_GAMEMEMBERENV), CHANGE_GAMEPHASE, chat (SEND_TEXTCHAT, WATCH_TEXTCHAT), mejor jugador del salón.

## 5. Durante y después del partido
- ❌ ADD_SCORE, ADD_FOUL, SEND_GAME_SEQUENCE_DATA, SEND_STATS_DATA, SEND_MATCH_TEAM_INFO, SEND_MATCH_ANALYZE, GET_GAMERESULTS, SET_GAMEPLAYER_PLAYTIME, REQ/WATCH_GAMEADDPOINT, CHECK_CHEAT.
- ❌ Guardar el resultado y estadísticas en base de datos.

## 6. Divisiones en línea
- ✅ Entrar, prueba de red, pantalla D12 con 300 puntos, «Seleccionar equipo».
- ❌ Puntos tras el partido, subir/bajar, fin de temporada (FINISH_SEASON), ranking (GET_DIVISION_RANKING, _BYID, POINT_INFO), equipos más usados, desconexiones y abandono (castigo AFK).

## 7. Cooperativo (2c2 / 3c3) y VS COM
- ❌ Todo: ~35 comandos COOP (salas, emparejar grupo, listo, rechazos, perfil, nivel, ranking, VS COM).

## 8. Clanes
- ❌ Todo: crear, invitar, aceptar, expulsar, salir, ajustes, listas (~17 comandos).

## 9. PES League y torneos
- ❌ PESLEAGUE (menú 1c1, competiciones, ranking, entrar/salir).
- ❌ Torneos en línea de PESBUL (calendario, inscripción, resultados).

## 10. Avisos y otros
- ❌ Avisos del sistema (SEND/WATCH_SYSTEM_NOTICE, SYNC_NOTICE, SEND_NOTICE), DMM, DLC filelist, CHECK_STRING (palabras prohibidas), GET_PLAYERINFO, perfil (SET_PLAYERPROFILE), valoración de posiciones, ranking general.

## 11. Fuera del juego (herramientas de PESBUL)
- Panel web de administración (jugadores, partidos, divisiones, torneos, nodos de relé, baneos por IP, avisos).
- API pública (historial de divisiones, torneos).
- «Clubs» propio con imágenes.
- Bot que crea un salón y espera: permite probar SOLO lo de entrar a un salón sin un segundo humano (no juega partidos).
- Nosotros: la web Phoenix ya hará gran parte de esto (Supabase).

## 12. Lo que PESBUL tampoco tiene
- myClub con datos reales (sus tablas existen, pero el modo no da datos).

## Orden recomendado
1. Emparejar 2 personas + relé de juego (bloques 2 y 3). Sin esto no hay partido.
2. Fin de partido y puntos de Divisiones (5 y 6).
3. Salón amistoso con 2 personas (4).
4. Avisos, chat, rankings (10).
5. Coop, clanes, PES League (7, 8, 9).

Para el paso 1 hacen falta DOS juegos conectados a la vez: una segunda PC en la misma red, un amigo, o probar si un segundo PES abre en la misma PC.
