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
