# Divisiones en línea y myClub extra: lo que dice PESBUL y lo que sabemos nosotros (10-nov-2026)

Fuente: lectura de solo lectura de la copia local de PESBUL (`D:\Frank\IA\CLAUDE\pesbul_local\app`), sin ejecutar nada ni copiar código, y nuestras capturas reales del PES base (`phoenix_online\capturas`). Todo con palabras nuestras.
Marcas: **L** = leído en PESBUL (con «PROVEN/MEASURED»: sale de ingeniería inversa del cliente o de capturas reales), **C** = comprobado por nosotros con el PES base, **P** = hipótesis o dato que falta.
Complementa `ESPEC_PARTIDO_REAL_PESBUL.md` (flujo del partido) y `ESPEC_MYCLUB.md` (myClub ya implementado).

---
# PARTE A. Progreso de Divisiones en línea (OC_RANK, 1 contra 1)

## A.0 Reglas por defecto de PESBUL (política de PESBUL, NO regla recuperada de Konami; todo editable)
- **Divisiones:** D12 (la más baja; todo jugador nuevo empieza ahí) a D1 (la más alta). Temporada = **10 partidos** en todas.
- **Puntos de temporada (SP) por partido** = `SP del equipo × multiplicador del resultado`; multiplicadores: victoria 1,00, empate 0,33, derrota 0,00; redondeo al más cercano (0,5 sube). El «SP del equipo» (Team SP Reward) sale de una tabla por equipo (300 por defecto si no hay dato; semilla por estrellas: 5★=300, 4,5★=425, 4★=525, 3,5★=625, 3★=750, 2,5★=825, 2★=900, 1,5★=975, 1★=1050: a menos estrellas, más puntos).
- **Umbrales (SP al final de la temporada):**

| División | Se queda si SP ≥ | Asciende si SP ≥ |
|---|---|---|
| D12 | (no desciende) | 300 |
| D11 | 300 | 800 |
| D10 | 500 | 1200 |
| D9 | 800 | 1650 |
| D8 | 1000 | 2000 |
| D7 | 1500 | 2500 |
| D6 | 2000 | 3000 |
| D5 | 2500 | 3500 |
| D4 | 3000 | 4000 |
| D3 | 3500 | 4500 |
| D2 | 4000 | 5000 |
| D1 | 4500 | (no asciende) |

  Regla: SP < «se queda» → desciende una división; «se queda» ≤ SP < «asciende» → se queda; SP ≥ «asciende» → asciende una. El ascenso doble existe pero está apagado.
- **Fin de temporada:** al cerrar (comando `CMD_FINISH_SEASON`) se archiva la temporada (SP, W/D/L, rating final, resultado), se aplica el ascenso/descenso, el contador de partidos vuelve a 0 y empieza la siguiente. El **rating personal NO se reinicia**. El menú del cliente deja pasar a la siguiente temporada en cuanto se ha ganado el ascenso aunque no se hayan jugado los 10; el servidor no avanza una temporada vacía ni una sin terminar que no haya ascendido.
- **Rating (Elo personal):** inicial 1000, escala 400, K = 32, mínimo 0; resultado 1 / 0,5 / 0. Nuevo = viejo + K·(resultado − esperado), redondeado al entero más cercano; el rival recibe el espejo.
- **Cortesía («manner»):** token `AA` por defecto (valores `AA, A, B, C, D`); si va vacío el cliente pinta «-».

## A.1 CMD_SEND_GAME_RESULT (L)
- **Lo mandan los dos clientes** al terminar. **Petición:** `side` (HOME/AWAY del que reporta), `home_score`, `away_score`, `home_team_id`, `away_team_id`, `game_phase` (p. ej. `2ND_30MIN`) y `abnormalend_reason`. No trae identificador de partido: el servidor lo liga con la sesión autenticada y el par registrado al emparejar.
- **Respuesta:** solo sobre `{msgid, rqid, result NOERR}`.
- **Qué hace el servidor:** el **primer** informe completo aplica a **los dos jugadores a la vez**: rating, SP, victorias/empates/derrotas, racha de victorias (y máxima), historial, equipos usados recientemente (5) y récords de carrera. El **segundo** informe se compara con el primero (si los marcadores difieren se anota `second_report_mismatch`) pero **no se vuelve a aplicar**.
- **Resultado desde el marcador:** según el lado del que reporta: tus goles > rival = WIN, < = LOSS, igual = DRAW. Equipo propio y rival salen de `home/away_team_id` según el lado.
- **Fin normal o anormal (C/L):** `abnormalend_reason` vacío, `NotImplement`, `NOERR` o `NORMAL_END` = normal (un partido completo trae `NOERR`; PESBUL tuvo un error que descartaba todas las victorias por esto). Cualquier otro valor es fin anormal. Medidos en archivos reales: `CMD_LACK`, `OUTPLAY_LONG`, `LOAD_TIMEOUT`, `SYNC_FAILED`, `NO_OPERATION`, `DISCONN_P2P`, `SLOWDOWN`. El cliente tiene una enumeración de 28 motivos; el primero es `NORMAL_END`.

## A.2 Abandono, desconexión y resultados que no coinciden (L, política de PESBUL)
- **Salida limpia durante el partido (CLEAN_LEAVE):** si un jugador sale con `CMD_LEAVE_ROOM` **con el partido en juego** (se comprueba por la primera actividad del canal 9) es **abandono**: pierde el que sale, gana el otro **3-0** (se conservan los goles del superviviente con mínimo 3 y se borran los del que sale). Pasa por el mismo cálculo normal (rating, SP, historial); sin castigo extra de Elo ni de cortesía.
- **Fin anormal reportado por el cliente:** con la política `no_count` (por defecto) un partido con fin anormal y marcador 0-0 **no cuenta** para nadie. Con marcador real distinto de 0-0, PESBUL espera hasta 180 s al informe del otro y decide: si el motivo es `NO_OPERATION` (el que reporta dejó de tocar los mandos) **pierde quien lo reporta**; `CMD_LACK`/`OUTPLAY_LONG` (se quedó esperando al otro) acusan al otro solo si hay marca clara. Todo lo ambiguo (`DISCONN_*`, `SYNC_FAILED`, `LOAD_TIMEOUT`), ambos con motivo de abandono, marcadores distintos o solo una marca de superviviente = **no cuenta** (no se adivina). En archivos reales los marcadores de los dos clientes coincidieron siempre.
- **Si los dos resultados no coinciden:** manda el primero y se anota la discrepancia; en decisiones de abandono, marcadores distintos → no cuenta.
- La victoria 3-0 por abandono y la espera de 180 s son decisiones de PESBUL (no de Konami): sirven pero podemos cambiarlas.

## A.3 CMD_REQ_GAMEADDPOINT y CMD_WATCH_GAMEADDPOINT (L, medido con el cliente)
- `CMD_REQ_GAMEADDPOINT`: lo manda el cliente al acabar; PESBUL contesta solo con sobre `NOERR` y la usa como marca de fin de partido.
- `CMD_WATCH_GAMEADDPOINT`: **el cliente NO la pide; la tiene que EMPUJAR el servidor** (suscripción por nombre, `rqid 0`), **una vez por participante que siga conectado**, justo después de aplicar el resultado. Es lo único que arma la pantalla «Cambios en el rating»; sin ella esa pantalla no sale. Un participante que ya se fue no recibe nada, y no se repite (se anota por partido y jugador).
- **Contenido** (nombres del parser del cliente): `{msgid 'CMD_WATCH_GAMEADDPOINT', rqid 0, result 'NOERR' (obligatorio), player_count N, player_list [N entradas, HOME primero], winner_side 'WINNER_HOME'|'WINNER_AWAY'|'WINNER_NONE', gameend_reason 'NORMAL_END', is_pk 'NO'}`. Cada entrada: `pid` (entero), `pes_pname` (bin con la identidad `{name, xuid_one 'NotImplement', psn_user_id bin{account_id 0, online_id nombre, is_valid_cache 'NO'}, is_deleted 'NO'}`), `rating_type 'RANKMATCH'`, `rating` (el NUEVO), `oldrating` (el ANTERIOR), `rating_boost_type ''`, `rating_boost_rate 0`, `rating_compe 0`, `team_id`, `side 'HOME'|'AWAY'`, `contWin_num` (racha actual), `skill_rank 'SKILL_RANK_NONE'`, `is_rankup_skill_rank 'NO'`, `is_first_skillrank 'NO'`. La diferencia (delta) **no se manda**: el cliente la calcula con los dos ratings.

## A.4 CMD_GET_DIVISION_INFO (L: contrato recuperado del cliente)
- **Petición:** incluye `is_season_end` (YES/NO). **Respuesta exacta** (las anidaciones van como **bin** de un msgpack, no como mapas directos):
```
{ msgid, rqid, result 'NOERR',
  pname:         bin(packb({name:str, xuid_one:str, psn_user_id: bin(packb({account_id:int64, online_id:str, is_valid_cache:str})), is_deleted:str})),
  division_info: bin(packb({
      division: str                ('D12'..'D1'),
      demotion_point: int          (0 si no hay descenso o es D12),
      promotion_point: int         (99999 si no hay ascenso o es D1: «deshabilitado»),
      doublerank_promotion_point: int (99999 = sin ascenso doble),
      other_match_num: int         (número del próximo partido 1..10 con la temporada abierta; 0 = temporada terminada),
      match_record: bin(packb({win_count, draw_count, lose_count, win_s_max, rating})),
      match_history_list: [ bin(packb({division_point:int, enemy_team_id:int, home_score:int, away_score:int,
                              enemy_side:str 'HOME'|'AWAY', enemy_name:str(≤32), is_finished_match:str 'YES'|'NO',
                              win_lose:str 'WIN'|'LOSS'|'DRAW', enemy_cpu_level:str})), ... ] })) }
```
  - Valores para un jugador nuevo: `D12`, `demotion_point 0`, `promotion_point 300`, `doublerank_promotion_point 99999`, `other_match_num 1`, récord a 0 con `rating 1000`, historial vacío (un array vacío es válido).
  - El historial lleva los **últimos 20** partidos más un elemento «acarreo» para que la suma de SP que calcula el cliente siga bien. `division_point` de cada entrada = SP ganados en ese partido.
  - No existen aquí `is_season_end`, `division`, `pre_season_rank` ni `division_change_type` (se probaron y se rechazaron).
  - `is_finished_match` y `enemy_cpu_level` son **texto** aunque el nombre sugiera booleano/entero.
  - `other_match_num == 0` es lo que hace que el cliente muestre «fin de temporada».

## A.5 CMD_GET_DIVISION_POINT_INFO (L: parser recuperado)
- **Petición:** sin campos propios. **Respuesta:** `{msgid, rqid, result 'NOERR', server_num 1, division_list [12 bins]}`; cada elemento es `bin(packb({division_type:str 'D12'.., point:int, rank:int}))`.
- Es dato de **carrera** (récords «mejores puntos y mejor posición por división»), no de la temporada actual. **Siempre doce filas, de D12 a D1** (la tabla del cliente es posicional: fila i = elemento i; con una sola, las demás quedan en blanco y la primera se pinta «División 0»). `point` = mejor SP en esa división (en la actual, el máximo entre el récord y los SP en curso); `rank` = mejor puesto, **0 si no hay**.

## A.6 CMD_GET_GAMERESULTS (L: parser recuperado; delicado)
- **Respuesta:** `{msgid, rqid, result 'NOERR', rlist_num N, rlist [N bins]}`; sin historial: `rlist_num 0`, `rlist []` y `no_match 'YES'`. **Máximo 20 resultados**, el más reciente primero, y el total debe caber en un marco (~0x410 B cifrados): PESBUL descarta las filas que no caben.
- **Cada elemento** (bin de msgpack; el parser declara 33 claves): `date` (**entero UNIX en segundos**; sin él el cliente enseña 1/1/1970), `match_category 'ONLINE_DIVISIONS'`, `ex_type 'NONE'`, `gameend_reason` (token de la enumeración de 28; `NORMAL_END` en partidos normales; «NORMAL» no existe), `pk_flag 'NO'`, `score` = **array de dos objetos** `[HOME, AWAY]` (cada uno con `score_1st`, `score_2nd`, `score_ex1st`, `score_ex2nd`, `score_pk`; total mostrado = 1.ª + 2.ª + ex1 + ex2, todos ≥ 0), `player` = array de dos jugadores (**HOME primero**) con `pname` (bin identidad), `pid`, `side`, `is_com 'NO'`, `cpu_level`, `is_follow 'NO'`, `is_guest 'NO'`, y **`team` como array a nivel de elemento** `[id_HOME, id_AWAY]` con el id **crudo** (el cliente lo desplaza <<14).
- El WIN/LOSS/DRAW lo calcula el cliente comparando totales y mirando su propio pid; **no hay campo de resultado**.
- **Aviso:** un elemento con claves en el nivel equivocado llegó a **tumbar el cliente**; hay que respetar exactamente este anidamiento.

## A.7 Ranking de Divisiones (L; sirve de analogía para B.4)
`CMD_GET_RANKING` / `CMD_GET_DIVISION_RANKING` (+`_BYID`): `{msgid, rqid, result, date (unix), total_rank_num, rank_list}`; cada elemento es un **mapa directo** (no bin): `{rank (1..), point (SP de temporada), user:{pid, pname:{name, xuid_one, psn_user_id:{account_id, online_id, is_valid_cache}, is_deleted}, division, manner, favoriteTeam, favoritePlayer, intro, match_record:{win_count, draw_count, lose_count, win_s_max, rating}}}`. Orden: rating descendente, luego SP, luego pid. Offset base cero; máximo 100 por respuesta; la petición trae `division` (filtro), `start_index` y `view_num`.

## A.8 Qué guarda el servidor (L)
Por jugador: división, rating, temporada (número, SP, partidos jugados, W/D/L, racha y racha máxima, historial completo), equipo último y 5 recientes, carrera (mejor SP y mejor puesto por división, temporadas archivadas, últimas 200), cortesía. Más un registro **solo de añadir** del historial para reconstruir si se pierde el archivo. Los umbrales se «congelan» al empezar cada temporada (los cambios valen para temporadas nuevas salvo que se pida aplicarlos ya).

## A.9 Sin comprobar (P)
Los umbrales reales de Konami (los de arriba son de PESBUL, calibrados en pantalla en D12); el significado de `enemy_cpu_level` y de los tokens de `ex_type`/`match_category`; la fórmula de ranking real de Konami (PESBUL ordena por rating y luego SP).

---
# PARTE B. myClub extra

**PESBUL no implementa nada de myClub** (solo aparecen los nombres de comando en listas de símbolos del cliente). Aquí solo hay lo que sabemos nosotros (capturas reales del PES base, `ESPEC_MYCLUB.md`, la lista de comandos del ejecutable `cmd_lista.txt`) y lo que hay que sacar por ingeniería inversa. Nada de esto está escrito en PESBUL.

## B.1 Comandos y qué sabemos de cada uno
| Comando | Petición real capturada (C) | Respuesta hoy | Qué falta |
|---|---|---|---|
| `CMD_GET_MYCLUB_HISTORYLIST` | `{pid, start_index 0}` (paginación por índice) | sobre `NOERR` vacío | claves del parser (movimientos del club) |
| `CMD_GET_MYCLUB_PRESENTLIST` | `{pid}` | sobre `NOERR` vacío | claves del parser (regalos pendientes) |
| `CMD_RECV_MYCLUB_PRESENT` | no capturado | — | campos y qué entrega |
| `CMD_GET_MYCLUB_ACHIEVEMENTLIST` | `{pid}` | sobre `NOERR` vacío | claves del parser (logros con estado) |
| `CMD_SEND_MYCLUB_READ_ACHIEVEMENTLIST` | no capturado | — | marca de «logros vistos» |
| `CMD_GET_MYCLUB_RANKING` y `_BYID` | no capturado | — | petición y parser |
| `CMD_ENTRY_MYCLUB_WEEKLY_RANKING` | no capturado | — | inscripción en el ranking semanal |
| `CMD_GET_MYCLUB_WEEKLY_RANKING` y `_BYID` | no capturado | — | parser de la lista semanal |
| `CMD_GET_MYCLUB_WEEKLY_RANKING_BASIC_INFO` | `{pid}` | resuelto en `ESPEC_MYCLUB.md`: `basic_info{eura_url, is_beginner, is_join, is_rejected, is_season_end, reward, division, division_change_type, is_no_play_season, is_group_rank_1st}` | ampliar premios |
| **Cambio de nombre del club** | — | — | **`CMD_SET_MYCLUB_TEAM_NAME`** |
| **Cambio de nombre de la plantilla** | — | — | **`CMD_SET_MYCLUB_SQUAD_NAME`** |

Direcciones de las cadenas de comando en el ejecutable (base 0x140000000), para buscar el parser por referencias cruzadas: HISTORYLIST 0x1428ABD18, PRESENTLIST 0x1428AC000, RECV_MYCLUB_PRESENT 0x1428A0C40, ACHIEVEMENTLIST 0x1428AB400, SEND_MYCLUB_READ_ACHIEVEMENTLIST 0x1428A6258, MYCLUB_RANKING 0x1428AC130, MYCLUB_RANKING_BYID 0x1428AC180, ENTRY_MYCLUB_WEEKLY_RANKING 0x1428A7FC0, WEEKLY_RANKING 0x1428AC548, WEEKLY_RANKING_BASIC_INFO 0x1428AC5E0, WEEKLY_RANKING_BYID 0x1428AC620, SET_MYCLUB_TEAM_NAME 0x1428A8A80, SET_MYCLUB_SQUAD_NAME 0x1428A89D0. Otros cercanos de identidad del club: `SET_MYCLUB_CLUB_EDITION_INFO` 0x1428A85F8, `SET_MYCLUB_HOMETOWN` 0x1428A86A8, `SET_MYCLUB_ADD_UNIFORM_ID` 0x1428A8520, `SET_MYCLUB_ACHIEVEMENT` 0x1428A84B0.

## B.2 Cambio de nombre (P)
Los comandos son **`CMD_SET_MYCLUB_TEAM_NAME`** (nombre del club) y **`CMD_SET_MYCLUB_SQUAD_NAME`** (nombre de una plantilla guardada). La petición debería llevar el texto nuevo y, para la plantilla, su `squad_id`; la respuesta casi seguro es solo el sobre `NOERR`, como el resto de los `SET_MYCLUB_*` ya implementados (**P**, por analogía). No hay captura: hay que provocarlo en el PES (myClub → editar nombre) y capturar la petición.

## B.3 Regalos (PRESENTLIST / RECV_PRESENT) (P)
Por el nombre y por los premios ya vistos en `WEEKLY_RANKING_BASIC_INFO` (`reward`): la lista llevaría regalos con tipo, cantidad (GP, monedas, objetos o jugadores) y fecha límite, y `RECV_MYCLUB_PRESENT` los marca como recibidos y suma al saldo. Sin parser no se puede dar la forma exacta.

## B.4 Rankings (P, con apoyo de PESBUL como analogía)
El ranking de club es probablemente de la **misma familia de parsers** que el de Divisiones (A.7): `date`, `total_rank_num` y `rank_list` con `rank`, `point` y un bloque de usuario con `pid` e identidad (mapas directos, **no bin**). Es razonable que `CMD_GET_MYCLUB_RANKING` tenga esa forma con un bloque de club, **pero no está comprobado**. La petición de PESBUL en rankings trae `start_index` y `view_num` (y `division` en Divisiones); `_BYID` lleva un identificador de jugador. El semanal añade inscripción previa (`ENTRY_...`) y premios al terminar la semana (`is_season_end`, `reward`, `division_change_type` ya vienen en `BASIC_INFO`).

## B.5 Cómo sacar los parsers que faltan (plan, no hecho)
1. Usar el proyecto de Ghidra ya analizado `D:\Frank\Games_\PRUEBA_CANCHA\copias\ghidra_proyecto_completo` (solo lectura). Abrir el ejecutable entero en REA tardó y se colgó una hora la última vez.
2. De la cadena `CMD_...` (direcciones de arriba) subir por referencias cruzadas hasta la clase `Cmd...Result` y su lector; leer en orden las cadenas de claves que compara (como se hizo con Divisiones: «parser 0x1D32570»). Para la petición, la clase `Cmd...` de escritura.
3. Provocar cada pantalla en el PES y capturar la petición real con las capturas del servidor.
Orden por valor: renombrar club/plantilla → `GET_MYCLUB_HISTORYLIST` → `GET_MYCLUB_ACHIEVEMENTLIST` → regalos → rankings.
