# Tablero del online Phoenix (se actualiza con cada avance)

Última actualización: 11-nov-2026, 21:55 (hora del servidor UTC).
PES del online: solo el base `C:\PES21\eFootball PES 2021`.

## Resumen
- **Probado en el PES y funcionando:** 25 de 55 apartados (**45 %**).
- **Programado (falta probarlo en el PES):** 20 más. Con ellos, **45 de 55 ya están hechos (82 %)**.
- **A medias** (abre bien pero sin contenido todavía, o hecho solo en parte): 7 (Matchday y PES League pasaron aquí: ya abren en el PES, falta la inscripción).
- **Falta hacer:** 3.
- Comandos del juego que el servidor ya controla: **281 de 439 (64 %)**. Cooperativo, Matchday, PES League y clanes ya tienen respuesta; solo falta empezar la tienda.

**Leyenda:** ✅ probado en el PES · 🟢 programado, sin probar en el PES · 🟡 a medias (abre, pero vacío) · ❌ falta · 🚫 no se hará (a propósito)

**Lo que más falta:** la prueba con **2 PES reales**. Es lo único que confirma un partido online completo (los apartados 9 y 20).

## 1. Entrar al online
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 1 | Conectar, aceptar condiciones, crear usuario | ✅ | desde el 02-nov |
| 2 | Menú online y avisos (incluido el «código Phoenix») | ✅ | |
| 3 | Perfil y «Editar perfil» | ✅ | |
| 4 | Datos en vivo (sin actualización pendiente) | ✅ | |
| 5 | Mantenimiento, misma versión y mismo parche obligatorio | 🟢 | apagado hasta probar con 2 PES |

## 2. Partida rápida y Divisiones (1 contra 1)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 6 | Buscar rival | ✅ | |
| 7 | «Revisando estado de conexión» | ✅ | superado el 10-nov |
| 8 | «Enviando y recibiendo datos» | ✅ | con el rival de prueba |
| 9 | Carga y partido completo | 🟢 | el relé está listo; **falta la prueba con 2 PES** |
| 10 | Resultado y cierre del partido | 🟢 | |
| 11 | Pantalla de división (D12, partido x de 10) | ✅ | probada el 10-nov: D12, partido 1/10, 300 para ascender |
| 12 | Puntos, rating, ascenso y descenso de D12 a D1 | 🟢 | reglas cambiables sin tocar código |
| 13 | Pantalla «Cambios en el rating» | 🟢 | |
| 14 | Récords por división | 🟢 | |
| 15 | Resultados de partidos | ✅ | abre bien (vacío hasta jugar partidos) |
| 16 | Ranking de Divisiones | ✅ | abre; arreglado que salían nombres repetidos |
| 17 | Análisis del rival | 🟡 | vacío hasta jugar partidos |
| 18 | Abandono durante el partido (3-0) | 🟢 | quien sale con el partido en juego pierde 3-0; un corte de conexión no cuenta |

## 3. Amistoso con un amigo (salas)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 19 | Crear sala, unirse, lista de salas | ✅ | 11-nov: flujo cambiado al probado por AzrakHQ con 2 PES (sin RELOAD empujado, JOIN_ROOM_NTL, vuelta a la sala tras el partido). **Volver a probar crear sala con 1 PES** |
| 20 | Partido en la sala | 🟢 | va directo entre las dos PC; **falta la prueba con 2 PES** |

## 4. myClub
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 21 | Entrar a myClub y club propio | ✅ | |
| 22 | Plantilla, alineación y plan de juego | ✅ | |
| 23 | Partido contra la COM | ✅ | primer partido completo el 05-nov; récord contra la COM guardado |
| 24 | GP y premios por partido | ✅ | |
| 25 | Fichajes: agentes, cazatalentos y subastas | ✅ | con los jugadores de tu parche |
| 26 | Contratos, entrenadores y experiencia | ✅ | |
| 27 | Cambiar el nombre del club y de la plantilla | ✅ | probado el 10-nov: myClub → quinta pestaña → «Cambiar nombre del equipo» |
| 28 | Historial | 🟡 | programado (cada movimiento de GP queda anotado). Su pantalla parece estar dentro de la tienda de monedas, que no abrimos (nada de dinero real) |
| 29 | Regalos | ✅ | probado el 11-nov: la «Bandeja entrada» (pestaña Configuración) muestra los regalos con su número y se cobran: GP, monedas y objetos llegan al club. Falta quitar un aviso falso («ya tienes el máximo») que sale al cobrar |
| 30 | Logros | ✅ | probado el 11-nov: 5 logros reales con progreso; cobro probado (+1000 GP). Desde el 11-nov el premio es en monedas gratis |
| 31 | Ranking de clubes | ✅ | probado: muestra «Phoenix FC» con escudo |
| 32 | Ranking semanal | 🟡 | vacío; el PES lo bloquea hasta jugar 10 partidos 1 contra 1 |
| 33 | myClub contra otra persona | ❌ | |
| 34 | Monedas gratis (interruptor) | ✅ | probado el 11-nov: los logros pagan en monedas (cobrado «+10 monedas») y los regalos también. Falta ver la moneda por victoria contra la COM |
| 35 | Gestión del club: entrenador de cada plantilla, dorsales, copiar o borrar plantilla, despedir entrenador, intercambio de jugadores, objetos de entrenamiento | 🟡 | **probado en el PES:** copiar y borrar plantilla, plantilla activa, plantilla para la nube y ciudad del club. Programado, sin probar: entrenador por plantilla, despedir o renovar entrenador, dorsales, intercambio de cartas repetidas, objetos (habilidad, posición, gestión y adaptación del DT) y entrenamiento táctico. Los objetos se dan desde admin; falta saber cómo los nombra el PES en su lista |
| — | Tienda con dinero real | 🚫 | nada de dinero real |

## 5. Otros modos del online
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 36 | Cooperativo (2 contra 2, 3 contra 3) | 🟡 | probado el 11-nov: abre (Partido informal, de clan, Estadísticas, Perfil) y **crear salón de clan funciona** (antes quedaba en blanco): título, temporizador, tabla del clan y lista con FRALEX (Dueño). Detalle: el nombre de la sala sale cortado. Falta el partido en sí |
| 37 | Matchday | 🟡 | probado el 11-nov: inscripción funciona (bando y equipo) y abre la pantalla del torneo sin cerrarse; faltan textos (bando, clasificación, premios) |
| 38 | Eventos y competiciones online | ✅ | probado el 11-nov: inscripción en la Copa Phoenix funciona (antes «No cumples los requisitos»: el servidor mandaba los datos sin empaquetar); falta el partido |
| 39 | PES League | 🟡 | probado el 11-nov: inscripción funciona («Torneo Phoenix» con equipo), abre su pantalla y ya no sale el aviso «resultados finales» (ahora dice «Espera a que abra el período de Matchmaking»). Programado, sin probar: nombre del equipo y matchmaking siempre abierto. Falta jugar el partido |
| 40 | Clanes | 🟢 | programado el 11-nov: crear clan, buscarlo, entrar, ajustes (nombre, comentario, abierto), expulsar, salir o disolver y lista de miembros. Falta probarlo en el PES |

## 6. Lo propio de Phoenix (fuera del juego)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 41 | Launcher: «Jugar Offline» | ✅ | probado el 10-nov |
| 42 | Launcher: «Jugar Online» con la cuenta de la web | 🟢 | falta probarlo de punta a punta |
| 43 | Tu web recibe los clubes de myClub | ✅ | conectado el 10-nov, sin la llave maestra |
| 44 | Tu web recibe las Divisiones (ranking online) | 🟢 | migración 165 aplicada; falta la tabla en la página |
| 45 | Vincular el PES con la cuenta de la web | 🟢 | el servidor está listo; falta el botón en la web |
| 46 | Dar GP y monedas desde la web (admin) | 🟢 | |
| 47 | Desvío del online con Phoenix Core (sin «hosts») | 🟢 | Core 0.5.66 listo; se instala después de la prueba de Street |
| 48 | Amigos por Tailscale | ❌ | falta instalar Tailscale (con tu permiso) |
| 49 | Servidor en internet (VPS, sin tu PC encendida) | ❌ | |
| 50 | Relé TURN propio (partido entre 2 PC aunque el router bloquee) | 🟢 | probado por AzrakHQ con 2 PES; se enciende con `turn_propio.activo` + 2 paquetes de Python |
| 51 | Actualización en vivo propia (todos con la misma base del parche) | 🟢 | carpeta `actualizacion/` + `tools/armar_actualizacion.py` |
| 52 | No emparejar PES con datos distintos («error DATAPACK») | 🟢 | sumas de CHECK_CHEAT + versión |
| 53 | Dirección del PES de tu PC para un amigo de otra casa | 🟢 | host_addr con tu IP pública/Tailscale |
| 54 | Reconexión a tiempo sin derrota; abandono por sesión exacta | 🟢 | |
| 55 | Tope de partidos con puntos contra el mismo rival por día | 🟢 | opción, apagada (`max_partidos_rival_dia`) |

## Historial de este tablero
- 11-nov 21:55: comparación completa con AzrakHQ y lectura a fondo de OpenEleven/MGSV/Sider. Nuevo: nivel cooperativo, perfil de desconectados, avatar guardado, ranking con orden, lista de bloqueo, filtro de palabras sin falsos rechazos, resumen del partido, TURN propio, actualización en vivo, emparejar solo datos iguales, reconexión. 135 pruebas en verde.
- 10-nov 23:20: primera versión.
- 10-nov 23:35: probadas en el PES la pantalla de división, resultados y los dos rankings (ningún cierre); arreglos de ranking; abandono 3-0 programado.
- 10-nov 23:50: ranking de clubes con nombre real y escudo; récord contra la COM en myClub; al abrir myClub ya no queda ningún comando sin respuesta.
- 10-nov 23:55: nuevo apartado 35 (gestión del club), que faltaba en el tablero; por eso los porcentajes bajan un punto.
- 11-nov 00:05: probado el cambio de nombre del club; programada gran parte de la gestión del club (entrenadores, plantillas, dorsales, ciudad).
- 11-nov 00:30: probadas en el PES copiar y borrar plantilla, la plantilla activa, la plantilla para la nube y la ciudad del club (arreglado su formato). Ningún cierre.
- 11-nov 00:55: logros programados (apagados hasta probarlos). Encontré y corregí a tiempo un choque con los logros de GP que ya existían; no llegó a afectar nada.
- 11-nov 01:10: historial de GP programado (apagado hasta probarlo).
- 11-nov 02:45: Regalos probado; Logros ya muestra filas en el PES (el PES solo acepta 24 tipos de logro con nombre fijo).
- 11-nov 03:10: Logros completo en el PES (progreso real y cobro). La Tienda myClub abre (vacía, sin venta). El Historial sigue sin encontrarse en el menú.
- 11-nov 03:20: empezamos a estudiar los Eventos online (torneos).
- 11-nov 03:20: Eventos programados de punta a punta (12 comandos). Falta la prueba en el PES base.
- 11-nov 03:23: programados el intercambio de cartas, los objetos de entrenamiento, el entrenamiento táctico y la valoración por posición (8 comandos).
- 11-nov 03:50: monedas gratis encendidas. La prueba de Eventos en el PES se paró en «Campeonato no encontrado» (el PES no reconoce el nombre de la categoría); probando varios nombres a la vez.
- 11-nov 06:34: regalos de verdad programados (bandeja con GP, monedas y objetos, y cobro). Eventos: se ve la Copa, pero la inscripción sigue bloqueada por un chequeo del juego; buscándolo con REA y x64dbg.
- 11-nov 06:59: regalos probados en el PES: se ven y se cobran (GP, monedas y objetos). Queda un aviso falso al cobrar. Los objetos de entrenamiento aún no los cuenta el juego (probando otro tipo). Inscripción en Eventos aparcada por decisión de Frank.
- 11-nov 07:08: monedas por logros probadas en el PES (+10 monedas). El aviso falso de los regalos y la inscripción en Eventos quedan para después.
- 11-nov 07:26: Cooperativo y Clanes abren en el PES (vacíos); Matchday y PES League programados con sus listas (13 comandos nuevos).
- 11-nov 08:46: Matchday probado hasta elegir bando y equipo. Se corrigió un cierre del PES (la lista de premios iba con la forma equivocada). Falta confirmar la inscripción final y el Torneo en línea; la prueba se pausó porque se cortó el enlace con la PC.
- 11-nov 09:31: inscripción probada en el PES en Torneo en línea y en Matchday; los dos abren su pantalla. Faltan datos de esas pantallas.
- 11-nov 10:05: Eventos destrabado: inscripción en la Copa Phoenix probada en el PES. Matchday: pantalla principal programada con sus datos (D.7), sin probar.
- 11-nov 11:15: Clanes programados de punta a punta (12 comandos nuevos, se guardan en la base). Falta probarlos en el PES.
- 11-nov 11:30: salas de Cooperativo programadas (7 comandos): crear, entrar, «listo» y salir. El partido cooperativo todavía no.
- 11-nov 13:20: sala de Cooperativo probada en el PES: ya no queda en blanco (forma de la sala corregida comparando con PESBUL).
- 11-nov 11:35: seguir jugadores y lista de rechazados del Cooperativo programados (4 comandos).
- 14:40 (Lima): repo AzrakHQ (MIT) estudiado: causa exacta del botón gris de Matchday aplicada (066485b, por probar); 195 comandos por revisar. Ver ESTUDIO_AZRAK_PES2021_SERVER.md.
- 14:50 (Lima): ranking de PES League (inscritos) + 6 avisos de Clanes + ranking de clanes vacío + premios PES League vacíos (forma de AzrakHQ, MIT). 120 pruebas OK. Por probar en el PES.
- 15:05 (Lima): Matchday con la receta completa de AzrakHQ (0206876); avisos SEND_NOTICE/SYNC/SYSTEM reenviados y game_sycom P2P en salas de Cooperativo (coop2.md); comparación automática de campos de 280 comandos (docs/COMPARACION_CAMPOS_AZRAK.md). 121 pruebas OK.
- 11-nov 15:20 (Lima, sesion online nueva): probado en el PES base. (1) Matchday con 0206876: ya NO se cierra y entra a «Jornada Phoenix»; «Empezar partido» ACTIVO; START_MATCHING MATCHDAY → pareja con `rival_falso.py MATCHDAY --revision` (sin --revision queda en «Revisando estado de conexion») → barrera de chequeo liberada → pantalla de carga; el PES abandona por error imprevisto: mismo techo que PES League (la carga necesita un segundo PES real). Con 1af12bc el PES se cerraba (0xc0000409) al recibir la LIST; 0206876 lo arregla. (2) Torneo Phoenix (compe 12): Terminos de Uso SIN raton: Abajo, Enter (marca «Acepto»), Abajo, Derecha, Enter; equipo ANDERLECHT (team 174), JOIN_PESLEAGUE_COMPE NOERR, pantalla muestra usuario FRALEX y equipo; «Empezar partido» manda START_MATCHING PES_LEAGUE (SEARCHING). (3) Ranking de Torneo: no alcanzado — en esa pantalla el menu lateral queda fuera de la captura/foco y las flechas no cambian nada visible; hace falta la ruta de teclas. Clanes, Coop y contadores de Entrenamiento: pendientes.
- 11-nov 15:55 (Lima, sesion online nueva, PES base): CLANES: Opciones de Cooperativo (7 entradas), Editar clan (GET/SET_CLAN_SETTINGS), Perfil del clan (1/11, «No reclutando»), Solicitudes hechas y Lista de seguimiento vacias con su aviso: sin cierres. ARREGLO ac8100e: «Tu lista de clanes» salia vacia porque el PES pide is_joined_clan=YES + enable_request=YES y filtrabamos por abierto=YES (el clan quedo con abierto=NO al guardar «Editar clan»); ahora lista tus clanes sin filtrar por abierto. Invitar/expulsar/solicitudes recibidas con otro usuario: sin probar (hace falta un segundo jugador). COOP: Partido de clan → Crear Salon → CREATEJOIN_COOP_ROOM (room 1) → sala «[1]Clan de FRALEX» con FRALEX (Dueno) Nv.1; «Empezar partido» pide confirmar matchmaking; Esc → «¿Salir de Salon de juego?» → Si → lista vacia. Sin cierres. BANDEJA: 3 regalos de monedas; al cobrar el servidor acredita (GP de 36.04x a 38.16x) pero el PES sigue mostrando «No puedes recibir este objeto porque ya tienes el maximo permitido» (sigue pendiente; es comprobacion del cliente tras acreditar). Tienda de Monedas: «No hay ninguna Moneda disponible a la venta» (correcto, cerrada). RANKING del Torneo: no alcanzado (el foco siempre vuelve a «Empezar partido»). REA 6.8.0 al dia; su MCP responde.
- 11-nov 17:00 (Lima, sesion online nueva): SERVIDOR DEJADO LISTO. Corre el codigo de la rama al dia (0b3c6e8; 135 pruebas OK), reiniciado 16:58 con el PES fuera de partido; puertos 80/5000/10000/10001 + gamerelay UDP 5100; espejo Supabase activo; copia de la base automatica al arrancar. La prueba de 2 jugadores queda para el FINAL (Frank). Para esa prueba: un segundo PES real (otra PC o amigo) con el hosts apuntando al servidor, mismo tipo de busqueda (Matchday / Torneo / Partida rapida); el rival falso solo llega hasta la carga. TURN propio (turn_propio.py/turnrelay.py/turndtls.py) esta en el codigo pero APAGADO: falta instalar pyOpenSSL en el Python del servidor (el sistema de permisos de la PC bloqueo el pip; Frank debe pegarlo: `"C:\Users\WinterOS\AppData\Local\Programs\Python\Python311\python.exe" -m pip install pyopenssl`, luego poner "turn_propio": {"activo": true} en config.json y reiniciar). Sin ese paquete el modulo se apaga solo y no afecta al resto.
- 11-nov 17:40 (Lima): revision del upstream (AzrakHQ = MohamedAliRashad, mismo repo; el .exe de Frank coincide con su SHA256SUMS; no hay commits nuevos desde nuestra lectura). Identidad: ya separamos copias con el mismo SteamID por IP; añadido aviso de identidad repetida (otra IP, misma clave) y partidos de la misma identidad no puntuan (136 pruebas OK). Analizado el mod dinput8 (anexo 2 del estudio): lo mas valioso para el Core son los 2 hooks de co-op (el exe es el mismo 1.01.00), luego PESID y certificado propio sin CA.
