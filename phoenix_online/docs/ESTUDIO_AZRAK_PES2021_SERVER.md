# Estudio del repo AzrakHQ/pes2021-online-server (MIT), 11-nov

Repo: https://github.com/AzrakHQ/pes2021-online-server (licencia MIT; Frank autorizó usarlo, 11-nov 14:24 hora Lima).
Copia de lectura: `/mnt/project-files/pes-online/referencia_azrak_mit/`. La licencia MIT pide conservar el aviso de copyright (Mohamed Ali Rashad, 2026) si se copia texto largo. Nuestro código sigue siendo propio, con el mismo protocolo.

## Qué trae y nos sirve
1. **notes/re/functions/*.md**: 36 informes de ingeniería inversa estática de PES2021.exe 1.01.00 (el mismo exe que usamos), con direcciones. Leerlos ANTES de probar.
2. **data/commands.json**: los 433 comandos con claves de pedido, claves de respuesta y área. Contra nuestro `logica.py`: tenemos handler de 238 y faltan 195 (muchos son avisos WATCH_/SEND_ del partido).
3. **data/server_errors.tsv y server_error_table.txt**: 764 pares (comando, ERR_*) con el texto que muestra el juego.
4. **tools/**: `kb.py` (referencias cruzadas y claves de un comando), `pe.py`, `parsebench.py` (corre el parser del propio juego sobre nuestros bytes: dice si una respuesta es válida sin probar a ciegas), `pesflow.py` (pantalla actual y navegación sin teclas), `probe.py` y `probe.c` (sonda dentro del juego), `scenario.py`. Necesitan Python con capstone y numpy, y el exe.
5. **server/mod** (dinput8.dll con MinHook): redirige el servidor sin tocar el archivo hosts. Es el equivalente a nuestro «Core 0.5.66 desvio_online»; sirve de referencia de forma.
6. **server/pesserver/*.py**: servidor completo (matchday, coop, tourney, divisions, rooms, TURN/relay, results, replay, ranking).

## Hallazgo ya aplicado: botón gris de Matchday (matchday2.md)
`compe_info` va a dos niveles. `compe_id`, `group_info_a/b`, las fechas e `is_entry` deben ir dentro de `compe_info.info_overview`. Con claves planas el cliente leía 0 y `end_group_match_date` quedaba en 0. Con `matchday_progress` = GROUP_MATCH y «ahora ≥ fecha», el juego baja el estado a «fase terminada» (función 0x14105d370; mgr+0x7f4 es `info_overview.end_group_match_date`). Eso explica por qué escribir +0x7F4 a mano no sirvió: la respuesta INFO lo volvía a poner en 0.
- `all_reward_info` es un ARRAY (nunca un bin).
- Tras JOIN el juego lee la plantilla con GET_MATCHDAY_SP_GAMEPLAN (ya lo tenemos).
- Arreglo: commit 066485b. Pendiente: probar en el PES.

## Lo que nos falta y ellos ya resolvieron (comandos sin handler nuestro)
- Matchday (11): ranking, resultado final, recompensas de logro, pickup.
- Co-op (28): partido cooperativo. Leer coop.md, coop2.md y pescoop.py.
- PES League (5): ranking y premios. Clanes (5): aceptar o rechazar entrada, ranking.
- Partido real (Match 32 + sincronía 14): results.md, match.md, pesmatch.py, pesresults.py. Sirve para el partido entre 2 PC.
- Red: net.md, turn*.md, nettune.md, peslg_path.md.
- Tienda y plataforma (24), Divisiones y ranking (3), Social y chat (5).

## Orden propuesto
1. Probar Matchday con el arreglo.
2. Leer results.md y match.md y compararlos con nuestro partido (lo que falta para el partido real en 2 PC).
3. Eventos contra la COM: modes2.md sección B (dice que no tiene menú propio; se entra por myClub).
4. Regalos «máximo»: buscar la tabla de diálogos en menus.md y messages.md.
5. Cooperativo (partido): coop2.md.

## Lo que ya pasamos de AzrakHQ a Phoenix (11-nov, tarde)
Método: su código se LEE (nunca se ejecuta; su .exe es el mismo servidor en Python empaquetado con PyInstaller). `tools/formas_azrak_estatico.py` compara los tipos de respuesta con ast; `docs/COMPARACION_TIPOS_AZRAK.md` queda con 4 diferencias que solo aparecen con datos (reinicio anunciado, aviso urgente) o por la clave de sesión (no la cambiamos).

| Tema | Antes | Ahora (commit) |
|---|---|---|
| Cooperativo: búsqueda | sin handler | START/CANCEL_COOP_MATCHING, WATCH_COOP_MATCHING FINISH con 2 grupos, datos COOP (a3118c9) |
| Salas del amistoso | RELOAD empujado a todos (cierra el PES del que ya tenía la lista) y JOIN_ROOM (el PES manda JOIN_ROOM_NTL) | flujo probado por ellos con 2 PES: ADD/UPDATE/DEL, lista de jugadores solo tras ENTRY_GAME, inicio con SET_GAMEMEMBER, vuelta a la sala con LEAVE_GAME, traspaso de dueño (eee85e0) |
| Tiempos del partido | 60 s todo (un 2.º PC lento se cortaba) | 180/120/240/180 = valores del propio PES; matchrejectpid_max 400 (a0abdde) |
| Lista de bloqueo | siempre «llena» | SET_MATCHREJECTPID guarda y el emparejamiento lo respeta (a0abdde) |
| Chat / reconexión / direcciones | sin handler | SEND_TEXTCHAT, RECONNECT (misma IP), GET_IPANDPORT_NTL (a0abdde) |
| Veredicto del partido | solo Divisiones | al pitazo final para todos los modos y al instante con REQ_GAMEADDPOINT (2cc5ab3, 4623355) |
| Matchday | sin puntos tras el partido | puntos 3/1/0, pantalla de puntos, ranking del bando (a0abdde) |
| Torneo en línea | puntos siempre 0, cola común | 3/1/0 con G/E/P y goles, menú y ranking reales, cola por torneo (4623355) |
| Otros | — | GET_COOP_PROFILE, aviso urgente por config (`aviso_urgente`), mantenimiento, DLC, SET_CURRENTPLAYER con chat permitido |

Pendiente de revisar contra su código: abandono a mitad de partido (pesmanner), nivel cooperativo (pescoop.award), análisis del partido (pesresults), y su red TURN (turn*.md) frente a nuestro relé.
