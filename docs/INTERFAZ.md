# Phoenix Link · Interfaz: rediseño, plan de experiencia, pasada visual, aviso de entrada y overlay

Historia completa de la interfaz de Link, del 10 al 19-oct-2026.

**Regla de Fralex:** mejorar DENTRO del diseño de Smash Soda + Galaxy; nunca rehacerla desde cero. Dos rediseños se rechazaron, y las partes de abajo explican por qué.

**Estado:** plan UX 0.3.21–0.3.34 y pasada visual 0.3.35–0.3.40. Para volver a la 0.3.20, ver la parte correspondiente.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 7 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Rediseño de la interfaz de Phoenix Link — Fase 0: análisis](#parte-1-rediseno-de-la-interfaz-de-phoenix-link-fase-0-analisis) — origen `docs/REDISEÑO-UI.md` (35f9a8f 2026-10-10)
- [Parte 2 · Aviso cuando alguien entra a la sala (desde 0.2.39)](#parte-2-aviso-cuando-alguien-entra-a-la-sala-desde-0239) — origen `docs/AVISO-ENTRADA.md` (8228c78 2026-10-12)
- [Parte 3 · Overlay premium: marcador de fútbol y panel de amigos (Link 0.2.73)](#parte-3-overlay-premium-marcador-de-futbol-y-panel-de-amigos-link-0273) — origen `docs/OVERLAY-PREMIUM.md` (4b42ac9 2026-10-12)
- [Parte 4 · Plan de experiencia en Link: qué se hizo, qué falta y qué probar (19-oct-2026)](#parte-4-plan-de-experiencia-en-link-que-se-hizo-que-falta-y-que-probar-19-oct-2026) — origen `docs/PLAN-UX-ESTADO.md` (053c502 2026-10-19)
- [Parte 5 · Phoenix Link: el menú nuevo (plan UX, 0.3.21 – 0.3.34)](#parte-5-phoenix-link-el-menu-nuevo-plan-ux-0321-0334) — origen `docs/GUIA-MENU-NUEVO.md` (053c502 2026-10-19)
- [Parte 6 · Pasada visual (Link 0.3.35 – 0.3.40, 19-oct-2026)](#parte-6-pasada-visual-link-0335-0340-19-oct-2026) — origen `docs/PASADA-VISUAL.md` (81e2846 2026-10-19)
- [Parte 7 · Volver a Phoenix Link 0.3.20 (antes del plan de experiencia)](#parte-7-volver-a-phoenix-link-0320-antes-del-plan-de-experiencia) — origen `docs/VOLVER-A-0.3.20.md` (9b13230 2026-10-19)

---

## Parte 1 · Rediseño de la interfaz de Phoenix Link — Fase 0: análisis

> Origen: `docs/REDISEÑO-UI.md` · último cambio 35f9a8f 2026-10-10 · 53274 bytes. Copia íntegra.

> Estado final (2026-10-10, 07:55): **se descartó el rediseño** y se volvió a la interfaz original (commit `4857aa8`) **con mejoras de funcionamiento**. Fralex: la versión nueva «le quitó el alma», era un fork de Soda Arcade / Smash Soda y parecía algo ajeno. Ver la sección «Versión final» al final. Las Fases 1–3 de abajo describen la versión descartada, que queda guardada en `_respaldo-rediseño-ui-2026-10-10/`.
> Analizado el 2026-10-10 sobre el worktree `_phoenix-link` (HEAD `60fe44a`, igual a `origin/rediseño-phoenix-portal`) **más los cambios sin commitear** que hay encima (25 archivos, incluido `vistas/guia.js` sin seguimiento). Es decir: se analizó lo que Fralex ve hoy en el simulador, no solo lo que está en git.

Archivos leídos: `CLAUDE.md`, `COORDINACION.md`, final de `REGISTRO-LINK.md`, `docs/RETOMAR-LINK.md`, `SmashSoda/phoenix/web/PROTOCOLO.md`, todo `ui/` (index.html, css/app.css, js/*.js, js/vistas/*.js) y, para confirmar campos, `EstadoWeb.cpp` y `Acciones*Web.cpp`.

---

### 0. Hallazgos previos (antes de diseñar)

1. **PROTOCOLO.md está desactualizado.** Le faltan 16 acciones que el motor ya registra y la UI ya usa: `sala.anchoAuto`, `teclado.asignar|crear|reiniciar|borrar`, `marionetas.motor|actualizar|maestro|titere|tipo`, `mandos.botonesBloq`, `chatGlobal.abierto|enviar`, `ajustes.modoPes`, `ajustes.avisosJuego`, `sync.deshacerEntrega`; y los campos/eventos `chatGlobal`, `noticias`, `buzon`, `pes`, `entrega`, `marionetas`, `teclado` y el evento `chatglobal`. La lista de verdad sale del C++ (sección a). No toco PROTOCOLO.md (regla del encargo): queda como **pendiente de documentación**.
2. **Todas las acciones del motor tienen al menos un control en la UI.** Ninguna está huérfana.
3. **Código muerto:** `vistas/mandos.js` conserva `Puestos`, `Puesto`, `Teclado`, `Marionetas` y `Bloqueo` antiguos (reemplazados por `puestos.js`, `teclado.js`, `marionetas.js`, `botones.js`). Solo `Turnos`, `Pendientes` y `ElegirJugador` siguen vivos.
4. **El motor solo manda algunos datos según lo que se mira** (`ui.seccion{seccion,pestana}`, una sola a la vez). Esto condiciona la nueva navegación — ver sección e.
5. **`RUN_BUILD.bat` está roto:** apunta a `C:\Users\WinterOS\smash-soda-fork\BUILD_AND_FIX.ps1`, que no existe. El script real es `C:\dev\smash-soda-fork\COMPILAR_PHOENIX.bat` (compila en `_phoenix-link\`). En la Fase 3 se compilará con ese.
6. **Trabajo sin commitear y sin compilar** en el worktree (bitácora: «A MEDIAS, SIN compilar con MSVC»). La bitácora ya registra una vez que se perdieron cambios sin commit. Recomiendo que Fralex compile y commitee ese estado **antes** de la Fase 2, para que el rediseño parta de una base guardada.
7. El prototipo «aprobado» (`docs/prototipo/v3`, `v4`) es la fuente de verdad visual actual. Este rediseño la sustituye (sin esquinas cortadas, sin neón): hay que darlo por superado de forma explícita.

---

### a) Inventario funcional

Leyenda de «Hoy»: `Sección › Pestaña › control`. «Resultado» = campo del estado `{t:"estado"}` (o respuesta) donde se ve el efecto.
La columna **Nuevo lugar** se completa en la Fase 3; cada fila debe terminar con ✔.

#### ui.* — aplicación

| Acción | Datos | Hoy (vista › control) | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `ui.seccion` | seccion, pestana | Automático en cada cambio de sección/pestaña (`tienda.irA/elegirPestana`) | Activa `audio`, `amigos`, `teclado`, `marionetas`, `red[].serie` completa | ✔ Automático: cada sección y cada panel declara su pareja (`tienda.js` › `VISTA_MOTOR` y `pedirVista`) |
| `ui.tema` | tema galaxy\|sudario | Barra › pastilla «GALAXY/LIGA B»; Ajustes › General › tarjetas de tema; Paleta | `app.tema` | ✔ Ajustes › Idioma y tema (segmentos); paleta Ctrl+K |
| `ui.idioma` | idioma | Ajustes › Interfaz › selector | `app.idioma` | ✔ Ajustes › Idioma y tema |
| `ui.interfaz` | modo phoenix\|clasica | Ajustes › Interfaz › «Usar la interfaz anterior» (+ diálogo) | Cambia a ImGui | ✔ Ajustes › Avanzado › Panel clásico e interfaz › «Usar la interfaz anterior» |
| `ui.panelClasico` | seccion 0-3, pestana | Sala › Juegos › Biblioteca «Editar» (3,5); Ajustes › Avanzado «Más opciones avanzadas» (3,6) | Abre panel clásico | ✔ Ajustes › Sala › «Editar la biblioteca en el panel clásico» (3,5); Ajustes › Avanzado › Panel clásico › «Abrir» (3,6) |
| `ui.copiar` | texto | Sala › Actividad › botón copiar | Portapapeles | ✔ Chat › Registro › botón copiar |
| `ui.abrir` | url https | Ajustes › General › botón Discord | Navegador del sistema | ✔ Ajustes › Sala › botón Discord |
| `ui.recargar` | — | Ajustes › Interfaz › «Recargar interfaz» | Recarga WebView | ✔ Ajustes › Avanzado › Panel clásico e interfaz › «Recargar» |

#### sala.* y perfilesSala.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `sala.abrir` | — | Sala › Resumen › «ABRIR SALA» (héroe); Guía paso 4; Paleta | `sala.abierta`, `sala.enlace`, `sala.segundos` | ✔ Inicio (sala cerrada) › «Abrir sala» (acción principal); paleta |
| `sala.cerrar` | — | Sala › Resumen › «DETENER SALA» (+ diálogo); Paleta | `sala.abierta=false` | ✔ Inicio (sala abierta) › menú ⋯ › «Cerrar sala» (+ diálogo); paleta |
| `sala.copiarEnlace` | — | Héroe «COPIAR»; Entrenador «COPIAR ENLACE»; Paleta | Portapapeles + aviso | ✔ Inicio (sala abierta) › «Copiar enlace» (acción principal); paleta |
| `sala.opciones` | nombre, plazas, limitador, limite, biblioteca, quiosco, overlay, turnos | Sala › Opciones (nombre, plazas, limitador, límite, turnos, overlay, quiosco); Sala › Juegos › Biblioteca (biblioteca); Ajustes › Overlay › interruptor (overlay) | `sala.opciones.*`, `pendiente` | ✔ Inicio › «Cambiar» / ⋯ › panel Ajustes de la sala (nombre, plazas); Ajustes › Sala (limitador, límite, quiosco, biblioteca); Ajustes › Overlay (overlay). `turnos` ya no se usa (ver decisión 4) |
| `sala.aplicar` | — | Héroe «APLICAR CAMBIOS» y banner de Sala › Opciones (si `pendiente`) | `sala.opciones.pendiente=false` | ✔ Inicio › línea «Aplicar» (y en el panel Ajustes de la sala y el menú ⋯) |
| `sala.phoenix` | visibilidad, espectadores, limiteEspectadores, entradaParsec, juego, parche, region | Sala › Opciones (visibilidad, espectadores, máx., entrada Parsec); Sala › Juegos (juego, parche, región) | `sala.phoenix.*` | ✔ Inicio › panel Ajustes de la sala (visibilidad, juego, parche, región, espectadores, entrada por Parsec) |
| `sala.calidad` | fps, mbps | Tarjeta Calidad (presets, FPS, Mbps) en Sala › Opciones **y** Ajustes › Video; Paleta (3 presets) | `sala.calidad.fps/mbps` | ✔ Ajustes › Calidad de transmisión (presets, FPS, Mbps) |
| `sala.anchoAuto` | auto, porPersona, subida | Tarjeta Calidad (interruptor, Mbps/persona, subida) en los mismos 2 sitios | `sala.calidad.auto/porPersona/subida` | ✔ Ajustes › Calidad de transmisión (automática, Mbps por persona, subida) |
| `perfilesSala.lista` | — | Sala › Opciones › Perfiles (al montar) | respuesta `{perfiles}` | ✔ Ajustes › Sala › Perfiles de sala |
| `perfilesSala.guardar` | nombre | Sala › Opciones › Perfiles «GUARDAR» | respuesta | ✔ Ajustes › Sala › Perfiles › «Guardar actual» |
| `perfilesSala.aplicar` | nombre | Sala › Opciones › Perfiles «APLICAR» | estado completo | ✔ Ajustes › Sala › Perfiles › «Aplicar» |
| `perfilesSala.borrar` | nombre | Sala › Opciones › Perfiles «×» (+ diálogo) | respuesta | ✔ Ajustes › Sala › Perfiles › «Borrar» (+ diálogo) |

#### web.* (cuenta de la liga) y sync.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `web.vincular` | codigo (6) | Sync › Cuenta web › campo + «VINCULAR» | `web.estado/usuario/mensaje` | ✔ Sync › Cuenta de la liga › código + «Vincular» |
| `web.reintentar` | — | Sync › Cuenta web › «REINTENTAR AHORA» | `web.estado` | ✔ Sync › Cuenta de la liga › «Reintentar ahora» |
| `web.desvincular` | — | Sync › Cuenta web › «DESVINCULAR» (+ diálogo) | `web.estado=sin_vincular` | ✔ Sync › Cuenta de la liga › «Desvincular» (+ diálogo) |
| `web.soltarRival` | — | Sync › Cuenta web › «SOLTAR RIVAL DEL RADAR» (solo sala abierta y publicada) | respuesta diferida | ✔ Sync › Cuenta de la liga › «Soltar rival del radar» (sala abierta y publicada) |
| `sync.deshacerEntrega` | — | Sync › Fichajes › «Deshacer última entrega» | `entrega.estado` | ✔ Sync › Datos y fichajes › «Deshacer última entrega» (ahora con diálogo) |
| `ajustes.avisosJuego` | valor | Sync › Puente › interruptor «Mostrar avisos de la web en el juego» | `buzon.activo/estado` | ✔ Sync › Puente con el juego › interruptor de avisos |
| `ajustes.modoPes` | valor off\|publicar\|estricto | Sync › Puente › «Solo PES 2021 (prueba)» segmentos | `pes.modo` | ✔ Sync › Solo PES 2021 (prueba) (plegable) |

#### mandos.*, solicitud.*, espera.*, turnos.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `mandos.asignar` | indice, parsecId | Mandos › Puestos (arrastrar persona a puesto; «ASIGNAR» → diálogo ElegirJugador); Gente › En sala › ficha «DAR MANDO…» | `mandos.lista[].ocupado/jugador`, `invitados[].mando` | ✔ Selector de puesto en la tarjeta de persona (Inicio) y en su ficha; arrastrar a la tira de puestos de Inicio |
| `mandos.liberar` | indice | Puestos «QUITAR» / soltar en «Mirando»; Gente › ficha «QUITAR MANDO» | idem | ✔ Selector de puesto › «Espectador» (Inicio y ficha) |
| `mandos.intercambiar` | a, b | Puestos › arrastrar puesto sobre puesto | `mandos.lista` | ✔ Selector de puesto hacia un puesto ocupado; arrastrar a un puesto ocupado (Inicio) |
| `mandos.tomar` | numero 0-8 | Diálogo ElegirJugador «Yo (anfitrión)»; «QUITAR» en el puesto del host (numero 0) | `mandos.host` | ✔ Selector de puesto de la fila «Tú» (Inicio y ficha) |
| `mandos.formacion` | local, visitante | Puestos › «+ AÑADIR PUESTO / − QUITAR UNO» por equipo | `mandos.formacion`, `lista[].equipo` | ✔ Jugadores › Puestos y equipos › Formación (Local / Visita) |
| `mandos.bloquear` | indice | Puestos › candado por fila; Mandos › Bloqueo › «Por mando» | `lista[].bloqueado` | ✔ Jugadores › Puestos y equipos › columna «Bloqueado» |
| `mandos.conectar` / `desconectar` | indice | Puestos › icono enlace por fila | `lista[].conectado` | ✔ Jugadores › Puestos y equipos › columna «Enchufado» |
| `mandos.herramienta` `ordenar` | nombre | Puestos › «ORDENAR» | `mandos.lista` | ✔ Jugadores › Puestos y equipos › «Ordenar» |
| `mandos.herramienta` `reiniciar` | nombre | Puestos › «REINICIAR» (+ diálogo); Paleta | `mandos.reiniciando` | ✔ Jugadores › Puestos y equipos › «Reiniciar mandos» (+ diálogo); paleta |
| `mandos.herramienta` `desconectarTodos` | nombre | Puestos › «QUITAR TODOS» (+ diálogo) | `mandos.lista` | ✔ Jugadores › Puestos y equipos › «Quitar el puesto a todos» (+ diálogo) |
| `mandos.herramienta` `bloquearTodo` | nombre | Bloqueo › «Bloquear todos los mandos»; Paleta | `mandos.bloqueoGlobal` | ✔ Jugadores › Puestos y equipos › «Bloquear todos»; paleta |
| `mandos.herramienta` `bloquearBotones` | nombre | Bloqueo › «Bloqueo de botones encendido» | `mandos.bloqueoBotones` | ✔ Ajustes › Avanzado › Bloqueo de botones › interruptor |
| `mandos.cantidad` | xbox, ds4 (≤8) | Puestos › steppers XBOX/PLAYSTATION; Mandos › Marionetas › «Mandos virtuales» | `mandos.xbox/ds4`, `lista[].tipo` | ✔ Ajustes › Avanzado › Mandos virtuales |
| `mandos.botonesBloq` | mascara, lt, rt, lx, ly, rx, ry | Bloqueo › mando dibujado + «Solo Start·Back·Guía / Bloquear todo / Liberar todo» | `mandos.botonesBloq` | ✔ Ajustes › Avanzado › Bloqueo de botones (mando dibujado y atajos) |
| `solicitud.aceptar` / `rechazar` | parsecId | Mandos › Puestos › tarjeta «pide el mando N» | `solicitudes[]` | ✔ Inicio › tarjeta «pide el puesto N» sobre la lista (Aceptar / Rechazar); aviso con «Ver» |
| `espera.decidir` | parsecId, como jugador\|espectador\|expulsar | Mandos › Puestos › tarjeta «JUEGA / MIRA / EXPULSAR» | `espera[]`, `invitados[]` | ✔ Inicio › tarjeta «quiere entrar» (Aceptar / Espectador / Rechazar); aviso con «Ver» |
| `turnos.activar` | si | Mandos › Turnos › interruptor del título | `turnos.activo` | ✔ Ajustes › Avanzado › Turnos (único interruptor) |
| `turnos.ajustes` | juegoMin, reinicioMin, recordatorioMin | Mandos › Turnos › 3 steppers | `turnos.*` | ✔ Ajustes › Avanzado › Turnos (3 steppers) |

#### teclado.* y marionetas.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `teclado.asignar` | userId, boton, tecla, nombre | Mandos › Teclado › mando dibujado (tocar botón + tecla) | `teclado.perfiles` (*) | ✔ Ajustes › Avanzado › Mapa de teclas |
| `teclado.crear` | userId, nombre | Mandos › Teclado › «+ invitado» | idem | ✔ Ajustes › Avanzado › Mapa de teclas › «+ invitado» |
| `teclado.reiniciar` | userId | Mandos › Teclado › «Volver a las de fábrica» | idem | ✔ Ajustes › Avanzado › Mapa de teclas › «Volver a las de fábrica» |
| `teclado.borrar` | userId | Mandos › Teclado › «Borrar este perfil» | idem | ✔ Ajustes › Avanzado › Mapa de teclas › «Borrar este perfil» |
| `marionetas.motor` | sdl | Mandos › Marionetas › segmentos SDL/XINPUT | `marionetas.motor` (*) | ✔ Ajustes › Avanzado › Marionetas |
| `marionetas.actualizar` | — | Mandos › Marionetas › «ACTUALIZAR» | `marionetas.maestros` | ✔ Ajustes › Avanzado › Marionetas › «Actualizar» |
| `marionetas.maestro` | indice | Mandos › Marionetas › «SER MAESTRO» | `marionetas.maestro` | ✔ Ajustes › Avanzado › Marionetas › «Hacer maestro» |
| `marionetas.titere` | indice, si | Mandos › Marionetas › botones 01-08 | `marionetas.titeres[]` | ✔ Ajustes › Avanzado › Marionetas › puestos 01-08 |
| `marionetas.tipo` | indice | Mandos › Marionetas › «MAPEO» | `marionetas.maestros[].tipo` | ✔ Ajustes › Avanzado › Marionetas › «Mapeo» |

(*) Solo llega con `ui.seccion = mandos/teclado` o `mandos/marionetas`.

#### gente.*, moderacion.*, amigos.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `gente.vip` | parsecId, nombre | Gente › En sala › ficha «HACER/QUITAR VIP» | `invitados[].vip` | ✔ Ficha de la persona › Roles › VIP |
| `gente.mod` | parsecId, nombre | Gente › ficha «HACER/QUITAR MODERADOR» | `invitados[].mod` | ✔ Ficha de la persona › Roles › Moderador |
| `gente.expulsar` | parsecId, nombre | Gente › ficha «EXPULSAR» (+ diálogo) | `invitados[]` | ✔ Ficha de la persona › «Expulsar» (+ diálogo) |
| `gente.banear` | parsecId, nombre | Gente › ficha «BANEAR» (+ diálogo; deshabilitado si `cop`) | `invitados[]` | ✔ Ficha de la persona › «Banear» (+ diálogo; deshabilitado si `cop`) |
| `gente.teclado` | parsecId, si | Gente › ficha «Permitir teclado»; Mandos › Teclado › lista por invitado | `invitados[].teclado` | ✔ Ficha de la persona › Permisos › Teclado (único lugar) |
| `gente.raton` | parsecId, si | Gente › ficha «Permitir ratón» | `invitados[].raton` | ✔ Ficha de la persona › Permisos › Ratón |
| `moderacion.listas` | — | Gente › Moderación (al montar y cada 5 s) | respuesta `{baneados, mods, vips, historial}` | ✔ Jugadores › Moderación (al abrir y cada 5 s) |
| `moderacion.desbanear` | parsecId | Moderación › Baneados «DESBANEAR» (+ diálogo) | respuesta | ✔ Jugadores › Moderación › Baneados › «Desbanear» (+ diálogo) |
| `moderacion.motivo` | parsecId, motivo | Moderación › Baneados › campo motivo | respuesta | ✔ Jugadores › Moderación › Baneados › campo motivo |
| `moderacion.quitarMod` / `quitarVip` | parsecId | Moderación › Moderadores / VIP «QUITAR» | respuesta | ✔ Jugadores › Moderación › Moderadores / VIP › «Quitar» |
| `moderacion.banear` | parsecId, nombre | Moderación › Historial «BANEAR» (+ diálogo) | respuesta | ✔ Jugadores › Moderación › «Pasaron por tu sala» › «Banear» (+ diálogo) |
| `amigos.invitar` | usuarioId | Gente › Amigos › «INVITAR A MI SALA» (solo sala abierta) | respuesta diferida | ✔ Inicio › «Invitar amigo» (panel lateral); Jugadores › Amigos (misma lista) |

#### chat.* y chatGlobal.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `chat.enviar` | texto | Panel Chat › pestaña SALA (con autocompletado `!`) | evento `chat` | ✔ Chat › Sala (sin cambios) |
| `chatGlobal.enviar` | texto (≤300) | Panel Chat › pestaña GENERAL (anti-spam con cuenta atrás) | evento `chatglobal` | ✔ Chat › General (sin cambios) |
| `chatGlobal.abierto` | abierto | Automático: panel abierto en GENERAL | ritmo de sondeo del motor | ✔ Automático (sin cambios) |

#### ajustes.*, sfx.*, diag.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `ajustes.general` chatbot, welcomeMessage, discord | clave, valor | Ajustes › General › «Chat y bot» | `ajustes.general.*` | ✔ Ajustes › Sala › Chat y bot |
| `ajustes.general` disableGuideButton, autoIndex, ipBan, blockVPN | clave, valor | Ajustes › General › «Entrada y seguridad» | idem | ✔ Ajustes › Sala › Entrada y seguridad |
| `ajustes.general` disableKeyboard | clave, valor | Ajustes › General **y** Mandos › Teclado | idem | ✔ Ajustes › Avanzado › Mapa de teclas (único lugar) |
| `ajustes.general` flashWindow, messageNotification, ttsEnabled, bonkEnabled | clave, valor | Ajustes › General › «Avisos» | idem | ✔ Ajustes › Sala › Avisos |
| `ajustes.general` socketEnabled, socketPort, parsecLogs, devMode | clave, valor | Ajustes › Avanzado | idem | ✔ Ajustes › Avanzado › WebSocket y registros |
| `ajustes.permisos` | grupo, clave useBB\|useSFX\|changeControls, valor | Gente › Permisos (tabla 3×3); Ajustes › Sonidos (solo useSFX) | `ajustes.permisos` | ✔ Jugadores › Permisos por rol (único control; Ajustes › Sonidos muestra quién puede, con «Cambiar») |
| `ajustes.video` | clave gpu\|monitor\|captura\|resolucion\|lanczos\|ritmo, valor | Ajustes › Video › «Captura» | `ajustes.video` | ✔ Ajustes › Video |
| `ajustes.videoListas` | — | Ajustes › Video › «ACTUALIZAR» | respuesta → `info.pantallas/gpus/wgc` | ✔ Ajustes › Video › «Volver a buscar pantallas y tarjetas» |
| `ajustes.audio` | canal parlantes\|mic, clave activo\|dispositivo\|volumen, valor | Ajustes › Audio (2 canales) | `audio` (*) | ✔ Ajustes › Audio (pide `ajustes/audio` al abrirse) |
| `ajustes.overlay` | clave monitor\|tema\|chat.*\|mandos.*\|invitados.*, valor | Ajustes › Overlay | `ajustes.overlay` | ✔ Ajustes › Overlay |
| `ajustes.overlayMenu` *(nueva, la añadió otra sesión el 10-oct mientras se hacía el rediseño)* | — | Ajustes › Overlay › «Abrir configuración del overlay» + atajos | ventana del overlay | ✔ Ajustes › Overlay › «Abrir configuración» (solo con sala abierta y overlay encendido) + lista de atajos |
| `sfx.lista` | — | Ajustes › Sonidos (al montar) | respuesta `{sonidos}` | ✔ Ajustes › Sonidos del chat |
| `sfx.recargar` | — | Ajustes › Sonidos › «Buscar sonidos nuevos» | respuesta | ✔ Ajustes › Sonidos › «Buscar sonidos nuevos» |
| `sfx.espera` | ruta, segundos | Ajustes › Sonidos › stepper por sonido | respuesta | ✔ Ajustes › Sonidos › stepper por sonido |
| `diag.ejecutar` | — | Sala › Conexión › Diagnóstico (al montar + «Revisar de nuevo») | respuesta `{chequeos}` | ✔ Ajustes › Diagnóstico (al abrir + «Revisar de nuevo»); Inicio enlaza ahí si hay un problema |

(*) `audio` solo llega con `ui.seccion = ajustes/audio`; además el motor arranca la vista previa de captura de audio con esa pestaña.

#### partido.*

| Acción | Datos | Hoy | Resultado | Nuevo lugar |
|---|---|---|---|---|
| `partido.preparar` | a[], b[], nombreA, nombreB, asignar | Partido › En vivo (fase libre) › A/B por persona, «SEGÚN MANDOS», nombres, «Sentar a cada uno», «PREPARAR PARTIDO» | `partido.fase=listo` | ✔ Inicio › Marcador › «Preparar partido» (con los puestos); Partidos › Armar a mano |
| `partido.iniciar` | anunciar | Partido › «¡EMPEZAR!» | `partido.fase=en_juego` | ✔ Inicio › Marcador › «Empezar» |
| `partido.gol` | lado, delta | Partido › «+ GOL» / «−» por lado | `partido.a/b.goles` | ✔ Inicio › Marcador › «+ Gol» / «−» por lado (el número salta) |
| `partido.pausa` | si | Partido › «PAUSA» / «REANUDAR» | `partido.fase` | ✔ Inicio › Marcador › «Pausa» / «Reanudar» |
| `partido.cambiarLados` | anunciar | Partido › «CAMBIAR LADOS» | `partido.invertido` | ✔ Inicio › Marcador › ⋯ › «Cambiar lados» |
| `partido.finalizar` | anunciar | Partido › «TERMINAR» (+ diálogo) | respuesta `{registro, guardado}` | ✔ Inicio › Marcador › «Finalizar» (+ diálogo) |
| `partido.cancelar` | — | Partido › «CANCELAR» (+ diálogo) | `partido.fase=libre` | ✔ Inicio › Marcador › ⋯ › «Cancelar partido» (+ diálogo) |
| `partido.historial` | max | Partido › Historial (al montar) | respuesta `{partidos}` → tabla calculada en la UI | ✔ Partidos › Últimos partidos y Tabla de la sala |

#### Funciones solo de interfaz (sin acción propia, también cuentan)

| Función | Hoy | Nuevo lugar |
|---|---|---|
| Paleta Ctrl+K (ir a pantallas + acciones rápidas) | Global | ✔ Global, con la navegación nueva y las 15 secciones de Ajustes |
| Atajos: Ctrl+Espacio (chat), Esc (cerrar), Alt+1…6 (secciones) | Global | ✔ Global; Alt+1…5 (5 secciones). Al cambiar de sección el foco pasa al contenido |
| Avisos automáticos (`vigia.js`: entra alguien, pide mando, espera, web, lag, sala) | Global, toasts | ✔ Global; los de solicitud y espera traen botón «Ver» → Inicio |
| Barra «Última hora» (noticias + últimos mensajes del chat general) | Barra superior | ✔ Cabecera (más discreta) |
| Chat: pestañas General / Sala / Registro, contadores sin leer, lista de comandos `!`, autocompletado | Panel lateral | ✔ Panel lateral (General / Sala / Registro) |
| Guía «Tu sala en 4 pasos» + glosario + «Cómo empezar» | Sala › Resumen (sala cerrada) | ✔ Inicio (sala cerrada) › «¿Primera vez? Así funciona» (panel con pasos + glosario) |
| Entrenador «Siguiente paso» | Sala › Resumen (sala abierta) | ✔ Sustituido: Inicio muestra el siguiente paso por sí mismo (la acción principal cambia, las solicitudes salen arriba, el marcador dice qué falta) |
| Lista «Antes de abrir» (Parsec, mandos, web, conexión) | Héroe con sala cerrada | ✔ Inicio (sala cerrada) › una sola línea solo si algo falla (Parsec, ViGEmBus, modo estricto, ahorro de energía) → Diagnóstico |
| Ping de la sala por persona + ping medio + calidad | Sala › Resumen | ✔ Inicio › tarjeta de persona (punto + ms) |
| Red detallada: semáforo, minigráfica 2 min, media/máx/jitter/picos | Sala › Conexión | ✔ Ficha de la persona › Conexión (gráfica 2 min, media, máx., jitter, picos) |
| Actividad: últimas 6, completa con filtro y copiar | Sala › Resumen / Actividad; Chat › Registro | ✔ Chat › Registro (filtro y copiar) |
| Carta de jugador de la liga (perfiles) | Gente › ficha | ✔ Ficha de la persona |
| Tabla de la sala (puntos calculados del historial) | Partido › Historial | ✔ Partidos |
| Prueba de mando en vivo (Gamepad API del navegador) | Mandos › Marionetas | ✔ Ajustes › Avanzado › Marionetas |
| Diálogo propio de confirmación | Global | ✔ Global (role=alertdialog) |
| Pantalla de carga hasta la bienvenida | Global | ✔ Global |
| Celebración de gol (ráfaga + número que salta) | Partido | ✔ Inicio › Marcador: el número salta (220 ms). La ráfaga de confeti se quitó (decorativa) — **pide confirmación** |
| Estado del puente Sider, juego/parche detectados, últimos 5 avisos | Sync › Puente | ✔ Sync › Puente con el juego + Últimos avisos; pastilla en la cabecera |
| Estado de la última entrega de Phoenix Sync | Sync › Fichajes | ✔ Sync › Datos y fichajes; chapa «!» en el menú si está rechazada o esperando |
| Presencia de amigos con conteo (activo / jugando / en sala) | Gente › Amigos | ✔ Jugadores › Amigos y panel «Invitar amigo» |
| Turnos: quién está en turno, restante, descanso | Mandos › Turnos | ✔ Ajustes › Avanzado › Turnos |

---

### b) Inventario del estado

**Ritmo:** `{t:"estado"}` se construye hasta 5 veces por segundo y se envía **solo si cambió** (PROTOCOLO). En la práctica, con la sala abierta cambia cada segundo como mínimo (`sala.segundos`, pings).

**Bienvenida** `{t:"bienvenida", datos}`: `protocolo`, `idiomas[{codigo,nombre}]`, `resoluciones[]`, `pantallas[]`, `gpus[]`, `wgc`, `temasOverlay[]`, `chat[]` y `actividad[]` (historial), `chatGlobal` (no documentado), `estado` (primer estado completo).

**Eventos** `{t:"evento", nombre, datos}`:
- `chat` → `{reinicio, lineas[]}` (chat de la sala, texto «Nombre: mensaje» o líneas del bot).
- `actividad` → `{reinicio, lineas[]}` (registro de la sala).
- `chatglobal` → `{cargado, pausado, esperaSeg, rev, error, mensajes[{id, usuarioId, nombre, texto, hora, rol, propio}]}` (no documentado; solo cuando cambia).

**Campos del estado** (confirmados en `EstadoWeb.cpp`; formas internas según `simulador.js`, que replica el C++):

| Campo | Tipo / contenido | Condición |
|---|---|---|
| `v` | 1 | siempre |
| `app` | `{version, idioma, tema, dev, soda}` | siempre |
| `web` | `{estado conectado\|vinculando\|sin_conexion\|pausado\|sin_vincular, usuario, mensaje, publicada, jugadoresLista, versionLiga, eventosEnCola}` | siempre |
| `ajustes` | `{general{…17 claves}, permisos{guest,vip,moderator}{useBB,useSFX,changeControls}, video{monitor,gpu,captura,resolucion,lanczos,ritmo,fps,mbps}, overlay{monitor,tema,corriendo,chat{activo,historial,posicion},mandos{activo,posicion},invitados{activo,latencia,posicion}}}` | siempre |
| `sala` | `null` si no hay Parsec; si no `{abierta, lista, enlace, nombre, plazas, invitados, cuentaHost, hostId, hostNombre, segundos, opciones{nombre,plazas,limitador,limite,biblioteca,juegos[],pendiente,turnos,quiosco,overlay}, phoenix{visibilidad,espectadores,limiteEspectadores,entradaParsec,juego,parche,region}, calidad{fps,mbps,auto,porPersona,subida,perdida‰,energia}, sesion{pico,entradas,partidos}}` | si `sala=null` el resto **no se envía** |
| `mandos` | `{lista[{n,conectado,ocupado,bloqueado,jugador,parsecId,ping,equipo local\|visitante\|fuera,tipo xbox\|ds4}], formacion{local,visitante}, host, bloqueoGlobal, bloqueoBotones, esclavo, xbox, ds4, reiniciando, botonesBloq{mascara,lt,rt,lx,ly,rx,ry}}` | siempre (con sala) |
| `teclado` | `{perfiles[{userId,nombre,teclas[{b,v,e}]}]}` | solo `mandos/teclado` |
| `marionetas` | `{motor sdl\|xinput, maestro, maestros[{n,nombre,tipo,activo}], titeres[{n,activo}]}` | solo `mandos/marionetas` |
| `solicitudes` | `[{parsecId, nombre, mando}]` (!cambio / !equipo) | siempre |
| `espera` | `[{parsecId, nombre, ping}]` (entró por Parsec sin estar en la lista web) | siempre |
| `invitados` | `[{parsecId, nombre, ping, mando, mod, vip, teclado, raton, rolWeb jugador\|espectador\|no_listado, cop, falso}]` | siempre |
| `perfiles` | `{parsecId: {nombre, avatar_url, carta{media,posicion,club,rareza,pais,apodo,foto_url,stats{rit,tir,pas,reg,def,fis}}}}` | siempre |
| `red` | `[{parsecId, nombre, presente, ultimo, media, maximo, jitter, picos, alerta, semaforo verde\|ambar\|rojo, serie[]}]` | `serie` = 120 muestras con `sala/red` o `partido`; 30 en el resto |
| `buzon` | `{activo, estado apagado\|cerrado\|noinstalado\|conectado\|sinconexion, ultimo, juego, parche, avisos[{id,texto,hora,escrito}]}` | siempre |
| `chatGlobal` | `{cargado, pausado, esperaSeg, rev, error}` (la lista va por evento) | siempre |
| `noticias` | `{cargado, lista[{id, texto, nivel info\|importante\|urgente, enlace, hora}]}` | siempre |
| `pes` | `{modo, abierto}` | siempre |
| `entrega` | `{estado ninguna\|esperando\|colocada\|rechazada\|deshecha, id, resumen, motivo, fecha, puedeDeshacer}` | siempre |
| `partido` | `{fase libre\|listo\|en_juego\|pausado, a{nombre,jugadores[{parsecId,nombre}],goles}, b{…}, segundos, inicioMs, invertido}` | siempre |
| `turnos` | `{activo, corriendo, usuarios[{parsecId,nombre,enAsiento,enfriamiento,restante,restanteEnfriamiento}], juegoMin, reinicioMin, recordatorioMin}` | siempre |
| `audio` | `{parlantes{activo,volumen,dispositivo,dispositivos[],nivel}, mic{…}}` | solo `ajustes/audio` |
| `amigos` | `{cargados, lista[{usuarioId,nombre,avatar,estado en_sala\|en_partida\|disponible\|ausente\|desconectado,salaId,desde}]}` | solo `gente/*` o `partido/*` (el sondeo a la web corre siempre que la ventana se ve) |

Estado local de la UI (`tienda.js`): `conectado`, `info`, `chat[]`, `actividad[]`, `noLeidos`, `chatGlobal`, `chatPestana`, `leidoGlobal`, `chatAbierto`, `seccion`, `pestanas`, `avisos[]`, `dialogo`, `paleta`. `localStorage`: guía oculta, noticias cerradas.

---

### c) Duplicados y confusiones

#### Mismo control en dos o más sitios

| # | Qué | Dónde está hoy | Propuesta (a confirmar en Fase 1) |
|---|---|---|---|
| 1 | **Turnos (hotseat) on/off** — y con **dos acciones distintas** (`sala.opciones{turnos}` y `turnos.activar{si}`) | Sala › Opciones; Mandos › Turnos | Un solo interruptor en Ajustes › Avanzado › Turnos con `turnos.activar` |
| 2 | **Tarjeta de calidad completa** (auto, Mbps/persona, subida, presets, FPS, Mbps) — mismo componente | Sala › Opciones; Ajustes › Video | Solo Ajustes › Calidad de transmisión; en Inicio, resumen de solo lectura + «Cambiar» |
| 3 | **Calidad/internet** en forma resumida | Héroe «Tu red para jugar»; Guía paso 3; «Antes de abrir › Conexión»; Indicadores «Calidad»; Paleta (presets) | Una línea de resumen en Inicio |
| 4 | **Visibilidad** | Héroe (lectura); Sala › Opciones (control); subtítulo de perfiles | Control en el panel lateral de la línea de resumen de Inicio |
| 5 | **Plazas** | Héroe (lectura); Sala › Opciones | Idem |
| 6 | **Ping** por persona | Resumen (pastillas + ping medio + mini-puestos), Conexión (tarjetas con gráfica), Gente lista y ficha, Puestos filas, Mirando, diálogo ElegirJugador, tarjeta de espera, Partido (minigráficas) — **10 sitios** | Uno: la tarjeta de persona en Inicio (punto de color + ms). Detalle (gráfica/jitter) al desplegar esa tarjeta |
| 7 | **Overlay on/off** (`sala.opciones{overlay}`) | Sala › Opciones; Ajustes › Overlay (cabecera) | Solo Ajustes › Overlay |
| 8 | **Teclado de invitados**: global (`disableKeyboard`) | Ajustes › General; Mandos › Teclado | Ajustes › Avanzado › Mapa de teclas (global) |
| 9 | **Teclado por invitado** (`gente.teclado`) | Gente › ficha; Mandos › Teclado | Menú de la persona (Jugadores) |
| 10 | **Cantidad de mandos Xbox/DS4** (`mandos.cantidad`) | Mandos › Puestos (cabecera); Mandos › Marionetas | Solo Ajustes › Mandos virtuales |
| 11 | **Bloquear un mando** (`mandos.bloquear`) | Puestos (candado); Bloqueo › Por mando | Menú de la persona/puesto |
| 12 | **Sentar / quitar mando** (`mandos.asignar/liberar`) | Puestos (arrastrar, «Asignar» + diálogo); Gente › ficha (selector); Partido › «Sentar a cada uno» (`asignar`) | Selector de puesto en la tarjeta de persona (Inicio y Jugadores, mismo componente) |
| 13 | **Formación local/visita** | Puestos (+/− por equipo); Partido › preparar (`asignar` cambia formación); perfiles de sala | Jugadores › formación; el partido la usa |
| 14 | **Permiso !sfx** (`ajustes.permisos useSFX`) | Gente › Permisos (tabla); Ajustes › Sonidos | Jugadores › Permisos por rol (tabla); en SFX solo lectura |
| 15 | **Tema** | Barra (pastilla con acento); Ajustes › General; Paleta | Ajustes › Idioma y tema + Paleta (la barra deja de tenerlo) |
| 16 | **Actividad / registro** (mismos datos `s.actividad`) | Sala › Resumen (6 últimas); Sala › Actividad (filtro, copiar); Chat › Registro | Una sola vista: pestaña «Registro» del chat con filtro y copiar |
| 17 | **Aplicar cambios pendientes** (`sala.aplicar`) | Héroe; banner de Sala › Opciones | Un aviso único en Inicio |
| 18 | **Vincular la web** (puntos de entrada) | Barra (pastilla), Guía «Opcional», «Antes de abrir», Gente › Amigos vacío, aviso del chat | Destino único Ajustes › Web de la liga; los demás son enlaces |
| 19 | **Juego y parche** | Sala › Juegos (control); Guía paso 2; Héroe (lectura) | Línea de resumen de Inicio → panel lateral |
| 20 | **Diagnóstico** | Sala › Conexión; Guía paso 1 y «Antes de abrir» llevan ahí | Ajustes › Diagnóstico; en Inicio solo aparece si hay un error |

#### Mismo concepto con nombres distintos

| # | Variantes hoy | Problema |
|---|---|---|
| 21 | **Puesto / Mando / Asiento** («PUESTOS», «QUITAR MANDO», «Puesto libre», «Mando libre», «DAR MANDO…») | Tres palabras para lo mismo |
| 22 | **Espectador / Mirando / Mira / Fuera** | «Fuera» es además un estado de puesto sin equipo |
| 23 | **Local / Visita** (Mandos) vs **Lado A / Lado B** (Partido) vs **Equipo** | El partido y los mandos no hablan igual |
| 24 | **Gente / Invitados / Jugadores / Personas** | La sección «Gente» y «Mandos» hablan de las mismas personas |
| 25 | **Sync / Web / Cuenta web / Web de la liga / Phoenix · Web de la liga** | La cuenta vive en «Sync» aunque no tiene que ver con Phoenix Sync |
| 26 | **Calidad / Ancho de banda / Mbps / Tu red / Tu internet / Conexión** | «Conexión» en «Antes de abrir» muestra FPS/Mbps pero lleva a la pantalla de ping |
| 27 | **Actividad / Registro** | Mismos datos |
| 28 | **Detener sala / Cerrar sala / Sala cerrada** | Verbo distinto entre botón y aviso |
| 29 | **Parche** declarado (Sala › Juegos) vs **parche detectado** (Sync › Puente) | Dos «parches» sin explicar la diferencia |
| 30 | **Panel clásico** (`ui.panelClasico`, ida y vuelta) vs **interfaz anterior** (`ui.interfaz clasica`, cambio permanente) | Dos «clásicos» distintos |
| 31 | **Liga B / Sudario** | El tema se llama `sudario` en el código y «Liga B» en pantalla (aceptable, pero que la UI nunca diga «Sudario») |

#### Incoherencias visibles

- **Umbrales de ping:** `colorPing` usa 50/120 ms; el semáforo del motor y la leyenda de Conexión usan 60/100 ms + jitter. Una persona puede salir ámbar en un sitio y verde en otro. → Un único criterio (el del motor cuando exista `semaforo`).
- **Referencias rotas:** «Revisa Ajustes › Diagnóstico» (Puestos vacío, Marionetas) — el diagnóstico ya no está en Ajustes. «Alt+1…5» en Ajustes › Interfaz — son 6 secciones.
- **La paleta no lista** Mandos › Teclado, Mandos › Marionetas ni Ajustes › Sonidos.
- **El aviso de solicitud** dice «Revísalo en Mandos» pero no lleva a ningún sitio.
- **La barra tiene 5 pastillas** (sala, web, chat, Ctrl K, tema); tres parecen botones y el tema usa el color de acento, compitiendo con «Abrir sala».
- **Densidad:** Mandos › Puestos con la sala abierta muestra ~40 controles a la vez (4 botones de cabecera + 2 steppers + 2×(+/−) + por fila candado, enlace y quitar).
- **Mayúsculas:** ~600 cadenas en MAYÚSCULAS en `js/` (≈300 etiquetas × 2 idiomas) y el acento en el título de casi cada tarjeta (`lab acc`). 31 `box-shadow` y ~20 brillos en el CSS.

---

### d) Flujo real del anfitrión

```
 ┌───────────┐   ┌──────────────┐   ┌────────────┐   ┌───────────────┐   ┌─────────────┐
 │ Abrir sala│──▶│ Compartir /  │──▶│ Gente entra│──▶│ Sentarla en   │──▶│ Armar       │
 │           │   │ invitar      │   │ (aceptar)  │   │ un mando      │   │ partido     │
 └───────────┘   └──────────────┘   └────────────┘   └───────────────┘   └──────┬──────┘
       ▲                                                                         │
       │         ┌──────────────┐   ┌────────────┐   ┌───────────────┐          │
       └─────────│ Cerrar sala  │◀──│ ¿Repetir?  │◀──│ Cerrar partido│◀── Jugar ◀┘
                 └──────────────┘   └─────┬──────┘   └───────────────┘
                                          └──── sí ──▶ Armar partido
```

#### Hoy (desde que abre la app en Sala › Resumen, sala cerrada)

| Paso | Dónde | Clics | Cambios de pantalla |
|---|---|---|---|
| 1. Abrir sala | Sala › Resumen › «ABRIR SALA» | 1 | 0 |
| 2a. Copiar enlace | Héroe › «COPIAR» | 1 | 0 |
| 2b. Invitar a un amigo | Menú Gente → pestaña Amigos → «INVITAR» | 3 | 1 (+1 pestaña) |
| 3. Alguien entra por Parsec sin estar en la lista / pide mando | Aviso flotante (sin enlace) → menú Mandos → «JUEGA»/«ACEPTAR» | 2 | 1 |
| 4. Sentar a 2 personas | Mandos › Puestos → arrastrar ×2 (o «ASIGNAR» + elegir ×2 = 4) | 2–4 | 0 (ya en Mandos) |
| 5. Armar el partido | Menú Partido → «SEGÚN MANDOS» → «PREPARAR PARTIDO» → «¡EMPEZAR!» | 4 | 1 |
| 6. Jugar | «+ GOL» por gol | n | 0 |
| 7. Cerrar el partido | «TERMINAR» → confirmar | 2 | 0 |
| 8a. Repetir | «PREPARAR PARTIDO» → «¡EMPEZAR!» (el borrador se conserva) | 2 | 0 |
| 8b. Cerrar la sala | Menú Sala → «DETENER SALA» → confirmar | 3 | 1 |
| **Total (un partido, con invitación)** | | **≈ 18 clics + goles** | **5 cambios**: Sala → Gente›Amigos → Mandos → Partido → Sala |

Observaciones:
- Una persona aparece en **4 pantallas distintas** (Resumen, Gente › En sala, Mandos › Puestos › Mirando, Partido › En la sala) con controles diferentes en cada una.
- Para saber si alguien tiene lag mientras se arma el partido hay que volver a Sala.
- El paso 3 depende de un aviso que desaparece a los 7–8 s y no tiene botón.

#### Objetivo del rediseño (estimación)

| Paso | Dónde (todo en Inicio) | Clics |
|---|---|---|
| Abrir sala | Botón principal | 1 |
| Copiar enlace / invitar | Botón principal / «Invitar amigo» → amigo | 1 / 2 |
| Aceptar a quien espera | Tarjeta sobre la lista de personas | 1 |
| Sentar a 2 personas | Selector de puesto en su tarjeta (o arrastrar) | 2 |
| Armar e iniciar el partido | Marcador › «Preparar con los mandos» → «Empezar» | 2 |
| Cerrar el partido | «Finalizar» → confirmar | 2 |
| Cerrar la sala | «Cerrar sala» → confirmar | 2 |
| **Total** | | **≈ 13 clics, 0 cambios de pantalla** |

---

### e) Restricciones técnicas que condicionan el diseño

#### `ui.seccion` sin cambiar el motor
El motor compara cadenas exactas. La UI nueva debe declarar, para cada vista, la pareja que activa los datos que necesita:

| Necesito | Enviar `ui.seccion` | Cuándo (UI nueva) |
|---|---|---|
| `amigos` | `gente` + cualquier pestaña | Al abrir «Invitar amigo» en Inicio y en Jugadores › Amigos |
| `red[].serie` de 2 min | `sala` + `red` (o `partido`) | Al desplegar el detalle de ping de una persona |
| `teclado` | `mandos` + `teclado` | Al abrir Ajustes › Avanzado › Mapa de teclas |
| `marionetas` | `mandos` + `marionetas` | Al abrir Ajustes › Avanzado › Marionetas |
| `audio` (y la vista previa de captura) | `ajustes` + `audio` | Al abrir Ajustes › Audio |
| nada especial | `sala` + `resumen` | Inicio en reposo |

Problema: el motor solo guarda **una** pareja. En una página de Ajustes con secciones plegables, si se abren Audio y Marionetas a la vez, solo una recibe datos. Solución sin tocar C++: las secciones que piden datos «pesados» se comportan como acordeón exclusivo (abrir una cierra la otra) y la última abierta manda su pareja.
→ **Pendiente de motor:** que `ui.seccion` acepte una lista de suscripciones (`{ver:["audio","marionetas"]}`), y que `amigos` se publique también con la sala abierta.

#### Otras
- CSP sin cambios: nada externo, nada inline en scripts (los `style=""` inline están permitidos por `'unsafe-inline'` de estilos, pero el rediseño debería moverlos a clases).
- `i18n.js` es `t(es, en)` en línea, sin diccionario: «todo pasa por i18n» = todo texto envuelto en `t()`.
- Temas existentes: `galaxy` (por defecto) y `sudario` (Liga B). Hay que revisar contraste AA en ambos tras cambiar fondos y grises.
- El simulador hoy arranca con la sala abierta; hay que añadir los tres escenarios (cerrada, abierta vacía, abierta con 6 personas) — p. ej. con `?escenario=` en la URL.

---

### f) Decisiones de Fralex (2026-10-10)

1. **Base de trabajo: SÍ.** Compilado con `cmake --build` (sin el `checkout --force` de `COMPILAR_PHOENIX.bat`, que habría borrado lo no commiteado) y guardado en el commit `4857aa8`, ya subido.
2. **El prototipo v3/v4 queda reemplazado: SÍ.**
3. **Phoenix Sync NO se esconde en Ajustes.** Es una pieza fundamental del ecosistema (web ⇄ Link ⇄ juego): avisos en el juego vía Sider, entregas de `Player.bin` / option file / `PlayerAssignment.bin`, fichajes compartidos entre PCs y el botón nativo «Datos Actual. en vivo → Activar» revivido (ver `PhoenixSync/INFORME-COMPLETO-2026-10-09.md`, `SYNC-COMPARTIDO.md` y los documentos de Drive). → **Sync mantiene sección propia** y además gana una pastilla en el grupo de estado de la cabecera.
4. **Turnos: solo `turnos.activar`. SÍ.**

---

## Fase 1 — Principios del nuevo diseño

### Navegación: 5 secciones, cero pestañas anidadas
| # | Sección | Qué contiene | `ui.seccion` que manda |
|---|---|---|---|
| 1 | **Inicio** | Pantalla según el estado: sala cerrada (abrir + línea de resumen editable) o sala abierta (enlace, personas con su puesto, solicitudes, marcador) | `sala/resumen`; con «Invitar amigo» abierto `gente/amigos`; con la ficha de una persona abierta `sala/red` |
| 2 | **Jugadores** | Personas en la sala (ficha con moderación, permisos, teclado, ping), puestos y equipos (formación, bloqueo y enchufe por puesto, herramientas), amigos, moderación, permisos por rol | `gente/sala` (trae `amigos`) |
| 3 | **Partidos** | Armar a mano, historial y tabla. El partido en vivo vive en Inicio | `partido/historial` |
| 4 | **Sync** | Cuenta de la liga (vincular), puente con el juego (estado, juego y parche detectados, avisos), datos y fichajes (última entrega, deshacer), «Solo PES 2021» | `sync/puente` |
| 5 | **Ajustes** | Una página con secciones plegables: Sala · Calidad de transmisión · Video · Audio · Overlay · Sonidos (!sfx) · Idioma y tema · Diagnóstico · **Avanzado** (cerrado): Mandos virtuales, Marionetas, Bloqueo de botones, Mapa de teclas, Turnos, WebSocket y registros, Panel clásico | `ajustes/general`; Audio → `ajustes/audio`; Marionetas → `mandos/marionetas`; Mapa de teclas → `mandos/teclado` (estas tres se abren de una en una) |

La cuenta de la liga vive en **Sync** (la vinculación es la raíz del puente con la web); los demás sitios solo enlazan ahí.

### Reglas
1. **Una sola acción principal con acento:** sala cerrada → «Abrir sala»; sala abierta → «Copiar enlace». Todo lo demás en grises.
2. **Inicio cambia según el estado**, no según funciones. Nada de pantallas vacías: si algo depende de la sala abierta, no aparece o es una línea con el botón que lo habilita.
3. **Cada cosa en un solo lugar** (resuelve la tabla c):
   - Sentar a alguien: selector de puesto de la persona (Inicio y su ficha: el mismo componente).
   - Ping: punto de color + ms en la persona; gráfica y jitter en su ficha. Un único criterio de color: el `semaforo` del motor; si no hay, 60/100 ms (los del motor).
   - Calidad: Ajustes › Calidad. En Inicio, solo lectura dentro de la línea de resumen.
   - Visibilidad, plazas, juego, parche, región, espectadores: panel lateral de la línea de resumen de Inicio.
   - Overlay on/off, mandos virtuales, teclado global, turnos: en Ajustes, una vez.
   - Registro de actividad: solo en el chat (pestaña Registro, con filtro y copiar).
   - Tema: Ajustes › Idioma y tema y la paleta (sale de la cabecera).
4. **Un solo vocabulario:** *puesto* (no mando/asiento), *espectador*, *Local / Visita* (también en el partido), *cerrar sala*, *cuenta de la liga*, *registro*.
5. **Identidad Galaxy con calma** *(corregido el 10-oct por Fralex: la primera versión gris «perdió totalmente la identidad»)*: vuelven la marca en degradado cian→púrpura, el fondo nebulosa azul noche (estático, sin aurora), las tarjetas con esquina cortada y borde degradado, Chakra Petch en mayúsculas espaciadas para cejas, menú, botones y números, y los colores de equipo (Local cian, Visita púrpura; en Liga B dorado y azul). Se mantiene de la calma: un solo botón relleno de acento por pantalla, nada que se mueva sin motivo, estructura sin pestañas. Versión anterior de esta regla: **Calma visual:** acento cian solo para la acción principal y «en vivo»; sin bordes neón ni esquinas cortadas; tarjetas separadas por espacio y un fondo apenas más claro; minúsculas normales (mayúsculas solo en insignias de 1–2 palabras); Plex Sans de cuerpo, Chakra Petch solo en títulos, Plex Mono solo para enlace, ping, Mbps y códigos.
6. **Cabecera:** un grupo de estado pequeño (Sala · Web · Juego/Sync) que no parece botón; quedan como botones Chat y Ctrl+K. La barra de «Última hora» se mantiene, más discreta.
7. **Movimiento con sentido:** entra una persona → su tarjeta se desliza; gol → el número salta; el ping cambia de color con transición. 100–220 ms, transform/opacity, `prefers-reduced-motion` respetado. Se quitan la aurora del fondo, los brillos que barren chips y los latidos decorativos.

---

## Fase 2 — Qué se hizo

- **Navegación:** 5 secciones (Inicio · Jugadores · Partidos · Sync · Ajustes), cero pestañas anidadas; Alt+1…5. Antes: 6 secciones y 26 pestañas.
- **Archivos nuevos** en `ui/js/vistas/`: `inicio.js`, `jugadores.js`, `partidos.js`, `personas.js` (TarjetaPersona, SelectorPuesto, TiraPuestos, Pendientes, PanelPersona, ListaAmigos/PanelAmigos, Carta). **Reescritos:** `app.js`, `tienda.js`, `ui.js` (Pastilla, SeccionPlegable, PanelLateral, MenuAcciones, Ping, Linea), `ajustes.js`, `sync.js`, `paleta.js`, `css/app.css`. **Adaptados:** `chat.js` (registro con filtro y copiar), `teclado.js`, `marionetas.js`, `botones.js`, `ultimahora.js`, `vigia.js` (avisos con «Ver»), `i18n.js`, `simulador.js` (3 escenarios). **Borrados:** `sala.js`, `mandos.js`, `puestos.js`, `gente.js`, `partido.js`, `guia.js`.
- **`ui.seccion`**: `tienda.js` traduce cada sección a la pareja que entiende el motor y los paneles piden la suya mientras están abiertos (`pedirVista`). Sin cambios en C++.
- **Simulador:** `index.html?escenario=cerrada` · `?escenario=vacia` · `?escenario=llena` (por defecto: 6 personas, 2 solicitudes, 1 en espera, pings de 18 a 132 ms, partido en juego 2–1).
- No se tocó C++ ni `PROTOCOLO.md`; la CSP de `index.html` sigue igual.

## Fase 3 — Verificación

| Prueba | Resultado |
|---|---|
| Inventario (sección a) | 113 filas, todas con ✔ y su lugar nuevo. Ninguna acción eliminada. |
| Tres escenarios en el simulador | OK, sin errores de consola. Capturas en `docs/rediseño-capturas/` (`cerrada`, `vacia`, `llena` a 1280×720 y 1920×1080, hechas con Edge sin interfaz). |
| 1280×720 y 1920×1080 | Ninguna sección con scroll horizontal ni elementos fuera de pantalla; la acción principal visible sin desplazarse en los tres escenarios; un solo botón con acento por pantalla. |
| Teclado | Tab recorre: saltar al contenido → última hora → Chat → Ctrl K → menú → acciones principales. Ctrl+K abre la paleta, busca y Enter ejecuta; Esc cierra paleta, paneles, menús y diálogos. Al cambiar de sección el foco pasa al contenido. |
| `prefers-reduced-motion` | Con movimiento reducido las animaciones quedan en ~0 ms (comprobado en Edge). Nota: el Windows de este PC tiene las animaciones desactivadas, así que **en este PC la app no se moverá**; el movimiento se ve en un Windows con animaciones activas. |
| Contraste AA | Medido con la paleta Galaxy final: el par más bajo es 5.9:1 en Galaxy (púrpura de Visita sobre la superficie más clara) y 5.5:1 en Liga B (rojo sobre la superficie más clara). Todos ≥ 4.5:1. |
| Compilación | `cmake --build` en `_phoenix-link` (Release): **0 errores**. La carpeta `ui/` se copia junto al exe (se borraron a mano de `x64/Release/ui` las 6 vistas viejas, porque `copy_directory` no borra). |
| App real | `PhoenixLink.exe` abre la interfaz nueva con los datos reales (captura `app-real-sala-cerrada.png`). |
| Respaldo ImGui | Su código (`InterfazWeb::renderImGui`) no se tocó y compila; **no se pudo forzar** aquí (haría falta quitar WebView2). |

**Pendiente de confirmar por Fralex:** la ráfaga de confeti al marcar un gol se quitó por ser decorativa (queda el número que salta).

## Pendiente de motor (C++) — ideas que la UI resolvió con lo que hay

1. **`ui.seccion` con varias suscripciones** (`{ver:["audio","marionetas","amigos","serie"]}`). Hoy solo hay una: por eso en Ajustes, Audio / Marionetas / Mapa de teclas se abren de una en una, y el panel «Invitar amigo» o la ficha de una persona cambian la suscripción mientras están abiertos.
2. **Publicar `amigos` siempre que la sala esté abierta** (hoy solo con `gente/*` o `partido/*`).
3. **Fichajes del grupo en Link** (Phoenix Sync compartido: pendientes de autorizar, «Aplicar ahora», conflictos). La sección Sync ya tiene el sitio («Datos y fichajes»); falta que el motor publique esa cola.
4. **`PROTOCOLO.md`** desactualizado (16 acciones y varios campos sin documentar, más `ajustes.overlayMenu`). No se tocó por regla del encargo.
5. **`RUN_BUILD.bat`** apunta a una carpeta que no existe y **`COMPILAR_PHOENIX.bat`** hace `checkout --force` (borra lo no commiteado): conviene arreglar ambos.

---

## Versión final: interfaz original + mejoras (la que vale)

**Por qué se descartó el rediseño.** Partió de cero en vez de partir de la app: quitó la sección MANDOS y las tarjetas grandes de puesto (el corazón de Smash Soda es repartir mandos), escondió el panel del anfitrión (tiempo en vivo, ping, stats y actividad del bot), fusionó secciones heredadas y calmó la estética Galaxy. **Lección: mejorar dentro del diseño existente, no reemplazarlo.**

**Qué cambia respecto a `4857aa8`** (solo funcionamiento; el diseño, las secciones y las pestañas son las de siempre):
| Mejora | Dónde |
|---|---|
| Solicitudes de mando y entradas por Parsec también en la portada (Juega / Mira / Expulsar, Aceptar / Rechazar) | Sala › Resumen, columna derecha (`Pendientes` reutilizado) |
| Avisos de solicitud y espera con botón «VER» que lleva a la portada | `vigia.js`, `tienda.avisar(…, boton)`, `app.js` |
| Tocar el ping de alguien en la portada abre su ficha | Sala › Resumen → Gente › En sala (`gente.abrirFicha`) |
| «DAR PUESTO…» en cada persona de «Mirando», además de arrastrar | Mandos › Puestos |
| Un solo lugar: turnos (Mandos › Turnos), overlay (Ajustes › Overlay), calidad (Sala › Opciones), teclado para todos (Mandos › Teclado), permiso de teclado por persona (su ficha), quién usa !sfx (Gente › Permisos), candado por mando (cada puesto), cantidad de mandos (Mandos › Marionetas). Donde estaba el duplicado queda una línea con «IR A…» | sala.js, ajustes.js, teclado.js, botones.js, puestos.js |
| Ping con un solo criterio: 60 / 100 ms, igual que el semáforo del motor | `ui.colorPing` |
| Referencias rotas corregidas («Ajustes › Diagnóstico» → «Sala › Conexión»; «Alt+1…5» → «Alt+1…6») | puestos.js, mandos.js, marionetas.js, ajustes.js |
| Paleta Ctrl+K con Mandos › Teclado, Mandos › Marionetas y Ajustes › Sonidos | paleta.js |
| Configurar el overlay (`ajustes.overlayMenu`, de la otra sesión) | Ajustes › Overlay |
| Simulador con `?escenario=cerrada | vacia | llena` | simulador.js |

Ninguna acción del motor se quitó. Compila sin errores (`cmake --build`), sin errores de consola en el simulador. Capturas actualizadas en `docs/rediseño-capturas/`.

---

## Parte 2 · Aviso cuando alguien entra a la sala (desde 0.2.39)

> Origen: `docs/AVISO-ENTRADA.md` · último cambio 8228c78 2026-10-12 · 1618 bytes. Copia íntegra.

Para enterarte de que alguien se unió aunque no estés mirando Link (por ejemplo, con PES abierto).

### Qué hace
Cuando un invitado entra, Link puede avisarte de tres formas, y cada una se enciende o apaga por separado:
1. **Sonido:** una campanita corta que genera Link (no necesita archivos). Si guardas un archivo llamado **`entrada.wav`** en la carpeta **`SFX`** que está junto a `PhoenixLink.exe`, suena ese en su lugar (tu sonido propio).
2. **Notificación de Windows:** un globo en la esquina con «Fulano entró a la sala» (~6 s).
3. **Aviso dentro del juego:** sale en el overlay de PES, si el puente de Phoenix está instalado y los «avisos en el juego» están encendidos (Sync › Módulos del juego).

### Dónde se configura
**Ajustes › Sonidos › «Cuando alguien entra»**: interruptor general, y uno por cada aviso. El botón **PROBAR** lanza los avisos activos con un invitado de ejemplo.

### Detalles
- Solo avisa de invitados, no de ti mismo.
- Si PES está a pantalla completa y Windows no muestra el globo (modo concentración, juegos exclusivos), el sonido y el aviso dentro del juego siguen funcionando.
- Los avisos viven en `SmashSoda/phoenix/core/AvisoEntrada.*`; Hosting.cpp solo tiene una línea (`phoenix::aviso::entro`) tras `addNewGuest`.
- El aviso para el overlay del juego se encola (`PhoenixLink::avisoLocalDiferido`) y lo escribe el hilo de Link, para no mezclar hilos.
- Preferencias en `phoenix-ui.json`: `avisoEntrada`, `avisoEntradaSonido`, `avisoEntradaNotif`, `avisoEntradaJuego` (todas encendidas por defecto).

---

## Parte 3 · Overlay premium: marcador de fútbol y panel de amigos (Link 0.2.73)

> Origen: `docs/OVERLAY-PREMIUM.md` · último cambio 4b42ac9 2026-10-12 · 1920 bytes. Copia íntegra.

### Qué trae
- **Marcador de partido** (abajo al centro, pequeño): equipos, goles, minuto y estado (en vivo, pausa, descanso, final). No estorba: asoma 8 s al empezar, 10 s con cada gol, se queda en el descanso y el final con el resumen de goles (con goleador si el juego lo dice) y en juego solo reaparece 6 s cada 90 s.
- **Cartel «¡GOOOL!»** con equipo, goleador, minuto y marcador.
- **Panel de amigos** (como el de Discord): `Shift + |` o `Ctrl + Alt + A`. Se abre encima del juego, lista a tus amigos de la web por estado (jugando, en sala, activos, ausentes, desconectados), con buscador y botón **Invitar**. `Esc`, el mismo atajo o un clic fuera lo cierran. Sin sala abierta avisa y no deja invitar.

### Cómo funciona
- Link manda al overlay (WebSocket local): `phoenix:partido` (lo que lee del juego, ver `Arbitro.cpp › marcadorOverlay`) y `phoenix:amigos` (solo con el panel abierto), y recibe `phoenix:panel {abierto}` (latido cada 4 s mientras está abierto: así Link sondea amigos aunque su ventana no se vea) y `phoenix:invitar {usuarioId}`. El resultado vuelve en `phoenix:invitacion`.
- La invitación usa la misma petición que Gente › Amigos (`/v1/invitar`).
- Overlay: `frontend/src/components/overlay/widgets/PhoenixMarcadorView.vue` y `PhoenixAmigosPanelView.vue`, estilos en `phoenix-futbol.css`.

### Probar sin PES
En `phoenix-overlay/frontend`: `npx vite --config vite.preview.config.ts` y abrir `http://127.0.0.1:8767/preview/index.html?esc=futbol&panel=1&gol=1` (también `fase=descanso|final`, `sala=0`).

### Pendiente de probar en el juego real
Marcador con un partido de PES de verdad, atajo `Shift+|` dentro de PES y la invitación a un amigo conectado.

### Nota
Antes de esto el overlay **no arrancaba nunca**: su manifiesto tenía `version="0.1.0"` (Windows exige cuatro números). Corregido a `1.0.0.0`.

---

## Parte 4 · Plan de experiencia en Link: qué se hizo, qué falta y qué probar (19-oct-2026)

> Origen: `docs/PLAN-UX-ESTADO.md` · último cambio 053c502 2026-10-19 · 3395 bytes. Copia íntegra.

Rama `plan-ux` (desde la etiqueta `antes-plan-ux-0.3.20`). Un commit por cambio, 0.3.21 → 0.3.34. Volver atrás: `docs/VOLVER-A-0.3.20.md`.
Mapa del menú nuevo: `docs/GUIA-MENU-NUEVO.md`. Lo que depende de la web: `docs/PROMPT-WEB-plan-ux.md`.

### Hecho (probado en el simulador, compilado sin errores)

| Versión | Fase | Cambio |
|---|---|---|
| 0.3.21 | 0 | Mensajes que mandan a otra pantalla llevan el botón que la abre; ViGEmBus con botón |
| 0.3.22 | 0 | Conexión y Juegos fuera de «MÁS» |
| 0.3.23 | 0 | «Soltar rival» pasa a Otras salas (hoy Jugar) |
| 0.3.24 | 1 | Semáforo Phoenix único (guía paso 1, Antes de abrir, Diagnóstico y Mi PC) |
| 0.3.25 | 1 | La pausa de la web dice su causa (`estado.web.codigo`); ARREGLAR TODO espera a que cierres PES y sigue solo |
| 0.3.26 | 1 | Asistente de bienvenida |
| 0.3.27 | 2 | Pantalla INICIO: jugar con un amigo, buscar rival, amigos, entrar a la mejor sala |
| 0.3.28 | 2 | Sentado automático con «solo mis amigos» |
| 0.3.29 | 2 | Tarjeta de resultado con Revancha |
| 0.3.30 | 2 | «Otras salas» → «Jugar», orden por invitación/amigos/ping |
| 0.3.31 | 3 | Glosario: «Conectar mi cuenta», «Mi juego» |
| 0.3.32 | 3 | Mismos nombres en el menú y en el título |
| 0.3.33 | 3 | «Ajuste fino» plegado en Transmisión |
| 0.3.34 | 3 | Moderación y Permisos junto a Gente; «Lo fundamental» arriba en Ajustes |

Pruebas del motor (MSVC): escenarios 55/55, módulos OK, fichajes 121/121, fichas 73/73, traspasos 107/107, actualización 42/42.
No se corrieron `correr_perfiles` (pide una copia del SYSTEM00000000) ni `correr_usuarios_vivo` (pide PES abierto).

### Del plan que NO se hizo en Link (y por qué)

- Vincular por navegador, aprobación automática de anfitrión, PC de invitado, «Jugar ahora», cuántos buscan rival, «En revisión», instalador .exe,
  publicar versiones: **dependen de la web** → `PROMPT-WEB-plan-ux.md`.
- Medidor de subida automático: necesita medir la subida desde Link (hay un `/v1/diagnostico` de red que podría servir); queda para otra vez.
- Ping y retraso del mando por puesto con «Arreglar mando», «Ocultar el mando repetido» de DS4Windows, perfiles de sala de un toque
  («Amigos/Pública/Torneo»; hoy hay perfiles guardados a mano en Opciones), «Iniciar con Windows»: no estaban en el encargo de esta ronda.
- Desconexión a mitad («esperar 2 min / seguir sin él»): depende del árbitro, aún sin probar en real.

### Sin probar en la app real (solo simulador)

1. Que la interfaz cargue en el WebView2 de Link 0.3.34 sin errores (abrir Link y mirar INICIO).
2. JUGAR CON UN AMIGO de punta a punta con Parsec real: abre sala, copia enlace, invitación que llega a la web/al amigo.
3. Sentado automático con un invitado real que pulsa su mando estando en «Mirando» (y con «solo mis amigos»: depende del nombre del perfil).
4. Semáforo con datos reales: ViGEmBus ausente, Parsec sin sesión, web en pausa (`HOST_NO_AUTORIZADO` en una cuenta nueva).
5. ARREGLAR TODO esperando a que se cierre PES de verdad (detecta el cierre y sigue).
6. Revancha tras un partido real (con el árbitro automático y a mano).
7. Asistente de bienvenida en una PC limpia (sin cuenta / sin ViGEmBus).
8. Botón INSTALAR VIGEMBUS: que el navegador baje el instalador oficial 1.22.0.

---

## Parte 5 · Phoenix Link: el menú nuevo (plan UX, 0.3.21 – 0.3.34)

> Origen: `docs/GUIA-MENU-NUEVO.md` · último cambio 053c502 2026-10-19 · 3771 bytes. Copia íntegra.

Guía corta para jugar y para encontrar cada cosa. Sustituye a las pantallas que nombraban las guías viejas
(«Otras salas», «Mi PC», «Sala › Más › Conexión», «Ajustes › Moderación»…). Para volver a la versión anterior: `docs/VOLVER-A-0.3.20.md`.

### Jugar en dos clics

**Anfitrión, con un amigo**
1. Abre Link. Se abre en **INICIO**.
2. Pulsa **JUGAR CON UN AMIGO**. Link abre tu sala, copia el enlace y te enseña «Invitar a…» con tus amigos de la web.
3. Pega el enlace en WhatsApp o Discord (o pulsa INVITAR junto a tu amigo).
4. Tu amigo pulsa un botón de su mando y **se sienta solo** en el primer puesto libre.
5. El partido arranca solo al pitazo. Al terminar sale el resultado con **REVANCHA**.

**Anfitrión, contra quien esté libre:** INICIO › **BUSCAR RIVAL** (tu sala pública sale en el radar de la web).
**Entrar a la sala de otro:** INICIO › «Salas abiertas ahora» › **ENTRAR A LA MEJOR**, o JUGAR › Salas abiertas.

Si algo falta, **no hay que buscarlo**: el **Semáforo Phoenix** (arriba de INICIO y en Sala › Conexión) dice qué es en una frase
y tiene el botón que lo arregla. Verde «Todo listo para jugar», ámbar «Listo, con un aviso» (se juega igual), rojo «Falta algo».

### La primera vez

Si a la PC le falta la cuenta o los mandos virtuales, Link abre solo el **asistente de bienvenida**:
1. Conectar mi cuenta (opcional: con amigos se juega igual).
2. Preparar mi juego (el Semáforo + ARREGLAR TODO; si PES está abierto, espera a que lo cierres y sigue solo).
3. Listo (y, si quieres, el recorrido de 1 minuto).
Se vuelve a ver en Ajustes › Interfaz › VER LA BIENVENIDA.

### El menú

| Menú | Pestañas a la vista | Detrás de MÁS |
|---|---|---|
| **INICIO** | Jugar con un amigo · Buscar rival · amigos conectados · salas abiertas · tu último partido | — |
| **SALA** | Resumen · Transmisión · Gente · Opciones · Conexión · Juegos | Moderación · Permisos · Actividad |
| **PARTIDO** | Marcador · Campo · Historial | — |
| **MANDOS** | Puestos (arriba, el sentado automático) · En vivo · Árbitro y bloqueo · Tu mando | Mapeo · Teclado · Turnos · Dónde juego yo |
| **JUGAR** (antes «Otras salas») | Salas abiertas (con «Entrar a la mejor» y «Soltar rival») · Amigos · Perfil PES | — |
| **SYNC** (sección propia) | Mi juego (antes «Mi PC») · Traspasos · Avanzado | — |
| **AJUSTES** | General (arriba «Lo fundamental»: idioma y sentado automático) · Overlay · Sonidos chat · Interfaz | Avanzado |

- **Transmisión:** a la vista solo LIGERO / EQUILIBRADO / MÁXIMA y «Automático». FPS, Mbps por persona, tu subida, ancho a mano y captura están en **Ajuste fino** (plegado).
- **Conexión:** Semáforo Phoenix, Detalles técnicos del PC (con COPIAR INFORME) y el ping de cada invitado.

### Palabras

| Antes | Ahora |
|---|---|
| Vincular / Unir esta PC | **Conectar mi cuenta** |
| Desvincular / Separar | **Desconectar** |
| Sync › Mi PC | **Sync › Mi juego** |
| Otras salas | **Jugar** |
| Antes de abrir · Diagnóstico · paso 1 de la guía | **Semáforo Phoenix** |
| Partido › En vivo | **Partido › Marcador** |

En la web el botón todavía se llama «Mi perfil › vincular PC» (pedido a la web cambiarlo).

### Errores con su botón

- **Falta ViGEmBus** → INSTALAR VIGEMBUS (baja el instalador oficial; luego cierra y abre Link).
- **La web está en pausa** → dice por qué (cuenta sin aprobar como anfitrión, versión vieja, PC suspendida) y REINTENTAR / ACTUALIZAR.
- **PES abierto al arreglar** → «ESPERANDO A QUE CIERRES PES…»: sigue solo al cerrarlo y te avisa.
- Cada mensaje que habla de otra pantalla lleva el botón que la abre.

---

## Parte 6 · Pasada visual (Link 0.3.35 – 0.3.40, 19-oct-2026)

> Origen: `docs/PASADA-VISUAL.md` · último cambio 81e2846 2026-10-19 · 5061 bytes. Copia íntegra.

Pedido de Fralex sobre 0.3.34: «más visual, más dinámico, más intuitivo, más premium y profesional, quiero dinamismo».
Rama `plan-ux`. Volver a 0.3.34: etiqueta `antes-pasada-visual-0.3.34` o la copia `x64\Release\PhoenixLink-0.3.34.exe`
(carpeta completa en `C:\dev\PhoenixLink-respaldo\Link-0.3.34-completo\`). Volver a 0.3.20: `docs/VOLVER-A-0.3.20.md`.

### En toda la app (lo que vale para todos los apartados)

| Qué seguía confuso o plano | Qué cambió |
|---|---|
| Debajo de las pestañas había una línea gris que decía *qué hay* en la pantalla, no *qué hacer* | Franja «¿qué hago aquí?»: icono de la sección + una frase de acción según lo que pasa ahora + el botón principal si no está ya grande en la pantalla. Verde si va bien, ámbar si hay algo que mirar (0.3.35) |
| «Conectando…» / «Cargando…» en texto plano | Esqueletos: tarjetas con la forma del contenido y un brillo que se desliza (0.3.36) |
| Los cambios de estado pasaban sin que se notaran | Destello de ~0,9 s cuando algo cambia: puesto ocupado/libre, color del semáforo, ping que empeora, pastillas de arriba (0.3.37, 0.3.40) |
| Los avisos desaparecían de golpe | Entran y salen deslizándose (0.3.37) |
| Botones e interruptores sin respuesta al tacto | Brillo que cruza los botones llenos, halo al pasar, anillo con el teclado, interruptor que se estira al pulsar (0.3.37) |
| «¿Cuánto falta?» no se veía | Barras de progreso que se llenan: semáforo («4 de 5 listos») y Mi juego («N de 6»); contadores animados (0.3.38) |
| Pantallas vacías con solo texto | Icono en un círculo + frase + el botón que lo resuelve (0.3.39) |
| Verdes/ámbar/rojos distintos según la pantalla | Siempre las variables del tema (--ok, --warn, --bad), en Galaxy y Liga B (0.3.39) |

Todo el movimiento es corto (≤ 0,9 s), solo transform/opacity/sombras, se apaga con `prefers-reduced-motion` y se para con la
ventana sin foco (PES delante). Ya existían y se mantienen: entrada de cada pantalla, aparición escalonada de tarjetas, rebote de la pestaña activa.

### Apartado por apartado

| Apartado | Seguía confuso (una línea) | Cambio |
|---|---|---|
| Barra superior | No se notaba cuándo la sala, la web o el juego cambiaban | Cada pastilla destella al cambiar; la de la cuenta es CONECTAR MI CUENTA (desde 0.3.31) |
| INICIO | El semáforo era solo texto; los contadores, números fijos | Barra «N de M» bajo el semáforo, contadores animados de amigos y salas, frase «Elige: con un amigo o buscar rival» |
| Sala › Resumen | La descripción no decía el siguiente paso | Frase según el estado: cerrada «abre tu sala y manda el enlace», abierta «comparte el enlace» |
| Sala › Transmisión | En manual no se avisaba arriba | Franja ámbar «la calidad está en manual» con PONER AUTOMÁTICO; carga del audio con esqueleto |
| Sala › Gente | Con la sala cerrada mandaba a «Ir a sala» | Icono + JUGAR CON UN AMIGO (va a INICIO) |
| Sala › Opciones | APLICAR se perdía abajo | Franja ámbar «hay cambios sin aplicar» |
| Sala › Conexión | Sin datos, pantalla gris | Icono + JUGAR CON UN AMIGO; semáforo con barra y destello |
| Sala › Juegos / Actividad / Moderación / Permisos | Descripción en vez de acción | Frase de acción; Moderación carga con esqueleto |
| Partido › Marcador | Sin sala o sin mandos, texto sin salida | Icono + JUGAR CON UN AMIGO / IR A PUESTOS; frase según la fase (listo, en juego, automático) |
| Partido › Campo / Historial | Descripción técnica | Frase de acción; Historial vacío con trofeo; carga con esqueleto |
| Mandos › Puestos | Un cambio de puesto pasaba desapercibido | El puesto destella al sentarse o irse alguien; franja ámbar si alguien pide jugar; contador animado de mandos en uso |
| Mandos › En vivo / Árbitro / Tu mando / Mapeo / Teclado / Turnos / Dónde juego yo | Descripciones de qué es, no de qué hacer | Frase de acción en cada una; Mapeo vacío con icono |
| Jugar | «Buscando salas…» y vacío sin salida | Esqueleto; vacío con BUSCAR RIVAL; frase «la mejor sala para ti es la de X» |
| Sync › Mi juego | No se veía el avance de ARREGLAR TODO | Barra «N de 6» que se llena paso a paso; colores del tema |
| Sync › Traspasos / Avanzado | Avanzado no decía que es solo para cuando algo falla | Frase de acción; colores del tema |
| Ajustes | Descripciones de qué hay | Frase de acción por pestaña («cambia lo fundamental…») |
| Chat y diálogos | Ya tenían entrada animada | Sin cambios propios: avisos con salida animada y botones con microinteracciones |

### Sin probar en la app real

- Que el movimiento vaya fluido en el WebView2 de Link con PES delante (sin foco se para; con foco, comprobar que no hay tirones).
- Destellos con datos reales (alguien que entra y se sienta, la web que se conecta, el juego que se conecta).
- Esqueletos con la carga real (en el simulador todo llega en milisegundos).
- Barra de Mi juego avanzando durante un ARREGLAR TODO real.

---

## Parte 7 · Volver a Phoenix Link 0.3.20 (antes del plan de experiencia)

> Origen: `docs/VOLVER-A-0.3.20.md` · último cambio 9b13230 2026-10-19 · 3034 bytes. Copia íntegra.

El plan de experiencia (`plan-phoenix-link.html`) se aplicó en la rama **`plan-ux`**, con un commit pequeño por cambio.
Antes de tocar nada se dejó guardada la versión anterior de tres formas. Usa la que te sea más fácil.

### 1. Lo más rápido: la copia del programa (sin compilar)

Hay una copia completa de la carpeta de Link 0.3.20 (programa + interfaz + todo lo que va al lado):

- Carpeta completa: `C:\dev\PhoenixLink-respaldo\Link-0.3.20-completo\`
- Solo el programa: `C:\dev\PhoenixLink-respaldo\PhoenixLink-0.3.20.exe`
- Y junto al programa nuevo: `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\PhoenixLink-0.3.20.exe`

Pasos:
1. Cierra Phoenix Link.
2. Copia **todo** el contenido de `C:\dev\PhoenixLink-respaldo\Link-0.3.20-completo\` encima de la carpeta donde tienes Link
   (en el PC de desarrollo: `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\`). Acepta reemplazar.
3. Abre `PhoenixLink.exe`. Abajo a la izquierda debe decir **PHOENIX LINK 0.3.20**.

Importante: copia la carpeta completa, no solo el `.exe`. La interfaz vive en la carpeta `ui\` de al lado, y un 0.3.20 con la `ui\` nueva
mostraría las pantallas nuevas.

Tus ajustes, tu cuenta unida y tus fichajes **no** se tocan: viven en `%APPDATA%` y en la carpeta del juego, no aquí.

### 2. Compilar 0.3.20 desde la etiqueta

La etiqueta **`antes-plan-ux-0.3.20`** marca el commit exacto de antes del plan (también está en GitHub).

```bat
cd /d C:\dev\smash-soda-fork\_phoenix-link
git status
git switch rediseño-phoenix-portal
cmake --build build --config Release --parallel
```

- `git status` primero: si sale algo sin guardar, guárdalo antes (commit) para no perderlo.
- La rama `rediseño-phoenix-portal` sigue exactamente donde estaba (el plan solo está en `plan-ux`), así que al volver a ella ya tienes 0.3.20.
- Si quieres el commit exacto sin ramas: `git switch --detach antes-plan-ux-0.3.20`.
- **Nunca** uses `COMPILAR_PHOENIX.bat` ni `--force`: borran lo que no esté guardado en git.
- CMake copia la carpeta `ui\` al lado del programa, pero no borra archivos viejos. Las pantallas nuevas son archivos que 0.3.20 no usa,
  así que no molestan; si quieres dejarla limpia, borra `x64\Release\ui\` antes de compilar.

### 3. Quedarte con parte del plan

Cada cambio del plan es un commit aparte en `plan-ux` (lista en `REGISTRO-LINK.md`, versiones 0.3.21 en adelante).
Para quitar solo uno: `git revert <commit>` dentro de `plan-ux` y compilar. Para ver la lista: `git log --oneline antes-plan-ux-0.3.20..plan-ux`.

### Qué NO se toca al volver

- La web y Supabase: el plan no cambió nada allí (lo de la web va como prompt en `docs/PROMPT-WEB-plan-ux.md`).
- Tus ajustes de Link (`%APPDATA%\Phoenix Link`, `phoenix-ui.json`), tu cuenta unida y tus fichajes en el juego.
- Lo poco que la interfaz nueva guarda aparte (si ya viste la bienvenida, el sentado automático…) vive en el almacenamiento interno de la ventana.
  0.3.20 no lo lee, así que no estorba.

---
