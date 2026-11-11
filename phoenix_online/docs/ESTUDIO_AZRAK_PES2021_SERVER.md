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
