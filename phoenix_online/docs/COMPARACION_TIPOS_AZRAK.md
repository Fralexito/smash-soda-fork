# Tipos de respuesta: AzrakHQ (lectura estática de su código) contra Phoenix

Su código se LEE con ast (no se ejecuta). Solo claves de primer nivel de sus `return {...}`.

- Igual forma: 53
- Con diferencias: 4
- Sin handler nuestro: 3

| Comando | Su archivo | Tipo distinto | Faltan en el nuestro |
|---|---|---|---|
| CMD_GET_MAINTENANCE_INFO | announce.py |  | maintenance_time, maintenance_warn |
| CMD_GET_URGENT_ANNOUNCEMENT | announce.py |  | announcement_data, is_announcement |
| CMD_JOIN_ROOM_NTL | pesrooms.py | nuestro responde ERR_ROOMNOTFOUND |  |
| CMD_SET_CURRENTPLAYER | pescmds.py |  | chat_probi, is_select_code_changed, nation_code_server, subscription_active, subscription_expire_date |

## Sin handler nuestro

CMD_GET_DLC_FILELIST, CMD_GET_TURN_SERVER_LIST, CMD_WATCH_MATCHING
