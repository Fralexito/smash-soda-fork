# Guía de pasos para probar el PES base contra Phoenix Online (11-nov-2026)

Receta probada por Claude (hilo ONLINE) para abrir el PES base (`C:\PES21\eFootball PES 2021`), recorrer sus menús con el teclado del MCP `phoenix-pes` y comprobar qué comandos llegan al servidor. Cada paso dice lo que se ve. Úsala antes de inventar rutas nuevas.

## 1. Antes de abrir
1. **Un solo PES a la vez.** Revisar `D:\Frank\IA\CLAUDE\pes-online\PES_EN_USO_ONLINE.txt` y `para-proyecto\TRASPASO.md`; si otro hilo tiene el PES (p. ej. Conmegol), esperar su «LIBRE». Comprobar con `tasklist | grep -i "PES2021\|sider"`: no debe haber `PES2021.exe` ni `sider.exe`.
2. **Un `sider.exe` colgado de Conmegol se inyecta en el PES base** y ensucia la prueba (el título muestra «ConmeGOL Patch» y salen mensajes de Sider). Cerrarlo (con permiso del hilo dueño) y comprobar que no queda ninguno.
3. Anotar el lock: línea en `PES_EN_USO_ONLINE.txt` y en `TRASPASO.md`. Al terminar, cerrar el PES y anotar «LIBRE».
4. Servidor arriba: `http://localhost:5000/estado` responde 200 (tras reiniciar tarda 30–120 s; usar un `until` que espere). Respaldar base y `logica.py` antes de instalar un commit.

## 2. Abrir y traer al frente
- Lanzar: `Start-Process 'C:\PES21\eFootball PES 2021\PES2021.exe' -WorkingDirectory 'C:\PES21\eFootball PES 2021'` y esperar a que `MainWindowHandle ≠ 0`.
- Windows bloquea `SetForegroundWindow`: llamar varias veces a un script que haga `ShowWindow(h, 9)` + `SetForegroundWindow(h)` (`restaurar.ps1`). Si el MCP dice «está minimizada» o «dejó de estar al frente», repetirlo; suele ser que otra ventana (Claude, un navegador) tomó el foco. No pulsar teclas mientras otra app tenga el foco.
- Capturas: `pes_captura` (ventana del PES, puede recortar la barra superior). Para ver **todo** (insignias de la bandeja, cabecera con GP y monedas) usar una captura de pantalla física a 1920×1080 (`captura_fisica.ps1`) y leer el PNG.

## 3. Teclas (MCP `pes_tecla`)
Disponibles: Enter (A), Esc (B), Espacio, Tab, Retroceso, flechas, Shift, Ctrl, Q W E R A S D Z C V F, 1, 2. **No existe la tecla Y** (por eso «Recibir todo» no se puede probar por teclado). `Z` = «volver al Menú principal» (pregunta Sí/No; con Derecha + Enter se acepta). Esperar entre pulsaciones: las pantallas tardan 5–12 s en cargar; una tecla pulsada antes de tiempo se pierde.

## 4. Rutas (todas verificadas)
- **Arranque:** pantalla de título («Pulsa cualquier botón») → Enter → aviso «No hay actualizaciones a través de Actualiz. en vivo» → Enter → **menú principal** (iconos: Empezar partido, eFootball, Editar, Opciones; abajo myClub, Master League, Be a Legend).
- **myClub:** en el menú principal, `Abajo` (1.er icono → myClub) → Enter → «Empezar» → Enter → aviso de actualización → Enter → aviso «plantillas actualizadas» → Enter → pantalla de myClub (pestaña «Sede de club»), con ~10 s de espera entre pulsaciones.
  - Pestañas: Partido | Sede de club | Contrato | Estadíst. | Configuración. `Izquierda` ×7 llega a «Partido» y `Derecha` ×1 a su primera tarjeta («En curso»).
  - **Bandeja entrada / Registro de myClub / Tienda myClub:** botones de la fila inferior de «Sede de club» (y de «Configuración»): `Abajo` desde las tarjetas baja a la fila (Registro → Bandeja → Tienda con `Derecha`). La insignia roja muestra el número de regalos o logros pendientes.
  - **Bandeja:** `A` cobra el regalo elegido (el servidor acredita GP/monedas/objetos y el regalo desaparece). El aviso «No puedes recibir este objeto porque ya tienes el máximo permitido» sale en cada cobro aunque se acredita (pendiente de arreglar).
  - **Registro de myClub (logros):** `A` cobra el logro; da monedas («10 Monedas de myClub»).
  - **Cabecera (GP / monedas):** no se refresca sola; salir con `Z` → Sí y volver a entrar a myClub.
  - **Entrenamiento:** tarjeta «Sede de club» → 3.ª. Contadores experiencia/habilidades/posicional.
- **myClub → «En curso»** (primera tarjeta de «Partido»): 3 tarjetas: Matchday, eFootball League de myClub, Desafío online (copas). Desafío online → «Copa Phoenix» → «¿Inscribirte en este Campeonato?» → Derecha = Sí.
- **Menú eFootball** (icono E, `Derecha` desde el primero, Enter): Partido rápido / Divisiones en línea / Cooperativo en línea / Matchday / Torneo en línea (Abajo ×0..4 y Enter). Cooperativo → Partido informal, Partido de clan, Estadísticas, Opciones. Divisiones → «División 12».
- Para salir de una pantalla online: Esc repetido; al final pregunta «¿Regresar al Menú principal?».

## 5. Pantallas de aviso y qué significan
- «Campeonato no encontrado»: lista vacía de torneos (Matchday/Torneo/Eventos sin datos).
- «No cumples los requisitos para participar en este Campeonato» (Copa Phoenix): chequeo local aún sin resolver.
- «En este momento, la inscripción para este Campeonato está cerrada» (Torneo en línea): chequeo local por fechas/cupo/división (ver ESPEC_COOP_MATCHDAY_PESLEAGUE D.5).
- **Matchday, tras «¿Inscribirte? Sí»:** (1) pantalla «Términos de Uso» (EULA del juego; **el botón Aceptar no se puede elegir hasta hacer clic sobre el texto de los términos y marcar «Acepto»**), (2) «Elige un bando» A/B, (3) «Seleccionar equipo» (liga → equipo), (4) «No podrás cambiar de equipo/bando después. ¿Continuar?», (5) envía `JOIN_MATCHDAY`. Si `GET_MATCHDAY_INFO` no devuelve el detalle, llega con `compe_id: 0` y dice «No se ha encontrado el Campeonato».

## 6. Cómo mirar los comandos
- `capturas\*.jsonl` (mayor mtime, sin `http`): campos `dir` (rx/tx) y `msg`. Mostrar solo `msgid` y recortar a ~300 caracteres para no saturar.
- `registro\servidor.log`: avisos «comando sin manejador». **Las líneas viejas se quedan**: filtrar por hora de la prueba.
- HTTP (cobro de regalos, etc.): `capturas\http.jsonl` (solo la petición; la respuesta hay que verla en el servidor).
- Observar sin tocar el juego: `observar_v10.exe <segundos> x<RVA>[,x<RVA>] <salida.json>` (PRUEBA_CANCHA\scripts); solo lectura, hasta 4 direcciones, guarda registros y pila del primer disparo.

## 7. Errores y trampas ya vistos
- Arrancó el PES con pantalla negra y se cerró solo: lo causó un `sider.exe` colgado de Conmegol; no reintentar sin revisar procesos.
- El PES se cierra con APPCRASH si `SQUAD_BONUS_TARGET_PLAYERLIST` manda `gameplayer_id_list` como enteros: debe ser lista de bin `{gameplayer_id: n}`.
- `myclub.objetos` y otras claves de config necesitan **reiniciar el servidor** (no se recargan en caliente).
- `pes_navegar` con varios Enter seguidos puede perder pulsaciones: mejor una tecla por llamada con captura entre pasos.
- Las pantallas de Matchday/Eventos solo modifican datos de nuestro servidor (reversible); los «Términos de Uso» son de la pantalla del juego, no un acuerdo real.

## 8. Torneo en línea y Matchday: qué los destrabó y cómo repetirlo (11-nov, Claude hilo ONLINE)

**Logro.** Torneo en línea (PES League 1 vs 1) y Matchday ya se pueden inscribir y abren su pantalla en el PES base. Torneo: relojes y «Espera a que abra el período de Matchmaking». Matchday: «Opciones de Matchmaking → Ir a Matchmaking». Antes: «inscripción cerrada», «Se calcularon los resultados finales», «Finalizó la fase de partidos de grupo», «No se ha encontrado el Campeonato».

**Regla que lo explica todo: si el servidor manda las fechas/duraciones en 0 (o todas iguales), el PES da el campeonato por TERMINADO.** El texto de «terminado» sale aunque el nombre y la lista se vean bien. Hay que mandar fechas coherentes y duraciones positivas.

**Torneo (PES League).** Tras `JOIN_PESLEAGUE_COMPE{compe_id, team_id}` el PES pide `GET_PESLEAGUE_COMPE_TEAM_LIST` y `GET_PESLEAGUE_1VS1_MAINMENU_INFO`. La segunda debe llevar (todo en la raíz, enteros salvo los dos textos): `matching_first_date`, `matching_time` (3600), `matching_interval` (86400), `compe_end_time`, `team_id`, `team_name`, `point`, `win_count`, `lose_count`, `draw_count`, `match_num`, `regulation_match_num` (>0), `rank_type`. Fuente: D.6. Commit `3594702` (después afinado por el coordinador).

**Matchday.** Dos causas seguidas, probadas en el PES:
1. `info_overview` NO fija el compe_id: tras inscribirse el PES manda casi todo con `compe_id: 0` (INFO, SP_GAMEPLAN, MAINMENU_INFO, AD_FILE, WATCHING_MATCH_LIST). El servidor debe resolver 0 → último torneo de la sesión, el inscrito o el único (`_cid_torneo`), y `JOIN_MATCHDAY` debe devolver `compe_id`. Commits `5ea6c3d` y siguientes.
2. Con `end_group_match_date`, `pickup_member_date`, `start_match_limit_date`, `entry_pickup_match_start_date` y `watch_match_start_date` todas = fin del torneo, y `group_match_time`/`pickup_match_time`/`watch_wait_time` = 0, sale «Finalizó la fase de partidos de grupo». Con las fechas escalonadas (grupo = fin-2 d, pickup = fin-1,5 d, gran final = fin-1 d) la cuenta atrás se movió exactamente un día (el cliente sí las lee), pero seguía igual hasta que se dio duración a `group_match_time` (≥ 2 días). Entonces abre «Opciones de Matchmaking». Función `_fechas_matchday` del servidor.

**Paso a paso para repetirlo (PES base, servidor con esos commits).**
1. Revisar el lock (`PES_EN_USO_ONLINE.txt`), que no haya `PES2021.exe`/`sider.exe` ajenos y que `/estado` responda 200.
2. Abrir el PES; pantalla de título → Enter ×2 → menú principal.
3. `Derecha` + Enter (icono eFootball) → aviso «Actualiz. en vivo» → Enter. **Mirar la captura antes de cada tecla**: el cursor recuerda la última entrada (puede estar en «Cooperativo»).
4. Matchday: 4.º elemento; Torneo en línea: 5.º. Enter → lista → Enter → «¿Inscribirte? Sí» (`Derecha`, Enter).
5. Términos de Uso (si aparecen): Enter sobre el texto, `Abajo` hasta «Acepto», Enter (marca), `Abajo` a los botones, `Derecha` hasta «Aceptar», Enter. Si se desmarca, mirar captura: Abajo desde «Acepto» puede caer directo en «Aceptar» y la Derecha siguiente cae en «No aceptar».
6. Elegir bando/liga → equipo → «¿Continuar/Participar? Sí».

**Errores e intentos fallidos (para no repetirlos).**
- El **puntero del ratón sobre la ventana del PES mueve el cursor del menú** (caía en Master League). Antes de probar: `Cursor.Position = (1900,1070)` o el ratón fuera de la ventana. Frank también lo toca sin querer.
- **Dos sesiones pulsando teclas en el mismo PES se pisan**; y abrir un segundo PES con la misma cuenta o reiniciar el servidor corta la prueba de la otra. Siempre: lock, una sola ventana, avisar.
- Esc en la pantalla principal de Matchday/Torneo cerró el PES dos veces sin volcado (nota del lock, 05:40): para salir de Torneo usar `Z` → Sí; no Esc.
- Con solo las fechas escalonadas (sin `group_match_time`) Matchday seguía «Finalizó…»: no basta una de las dos cosas.
- `GET_PESLEAGUE_1VS1_MAINMENU_INFO` aún sin respuesta propia = NOERR vacío = «resultados finales» + relojes `-- : --`.

**Cómo comprobar y revertir.** Comprobar: Torneo muestra reloj de matchmaking y «Mejor resultado en 10 partidos»; Matchday abre «Opciones de Matchmaking». Revertir: cada instalación tiene respaldo `logica.py.antes_<commit>`, `config.antes_<commit>.json` y `respaldos/phoenix_online.antes_<commit>.db`; copiar de vuelta y reiniciar el servidor (PES fuera del online, o sale C_MHJX_0002).

## 9. Regalos con `regalos_respuesta = "vacio"` (prueba 11-nov 07:47, PES base) y Clanes
- **Regalos:** regalo creado con `admin_myclub.py config.json regalo 0410000004100000 gp 5000 "Prueba vacio"` (ojo: hay dos «FRALEX» en la lista; el que tiene club es el `steam_hex` 0410000004100000). En myClub → Sede de club → `Abajo` (fila inferior) → `Derecha` → Enter abre la «Bandeja entrada» (el GP aparece como «Monedas de myClub 5,000»). Cobrar: elegir la fila y **Enter** (la tecla `A` no llegó a mandar `RECV_MYCLUB_PRESENT` en esta prueba). Resultado: GP acreditado (23 040 → 28 040) **pero el aviso «No puedes recibir este objeto porque ya tienes el máximo permitido» SIGUE saliendo con «vacio»**.
- **Pista (L, Ghidra):** el lector de `CMD_RECV_MYCLUB_PRESENT` (`FUN_141cd7f00`) lee, además de `msgid/rqid/result` y `receive_result_list_num`, los elementos de `receive_result_list` con las claves de ficha de regalo (`present_id`, `name`, `name_jp`, `title`, `body`, `entity_id`, `grade`, `skill`, `type`, `player_level`, `contract_term`, `team_id`, `present_type`, `item{item_type, item_variation, item_effect_arg, count, pes_point_price}`, `receipt_deadline`) y **NO tiene una clave `result` por elemento**: lo que mandamos en `e["result"]` se ignora, así que probar «vacio/cero/sin_result» no cambia nada. El aviso sale de `ERR_RECV_PRESENT_FULL`/`ERR_RECV_PRESENT_ALL` (tabla de errores 0x14021fa30) o de otra comparación local. **Siguiente prueba:** respuesta con `result: "NOERR"` y elementos SIN el bloque `item` en regalos de GP, o con `item` completo (`item_type "STORE"`, `count` = cantidad); y `receive_result_list_num` igual al número de regalos pedidos.
- **Clanes:** ver D.9/D.9b en `ESPEC_COOP_MATCHDAY_PESLEAGUE.md` (ruta: eFootball → Cooperativo en línea → Opciones de Cooperativo).
