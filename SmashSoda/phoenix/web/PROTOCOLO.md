# Interfaz nueva de Phoenix Link (HTML + WebView2)

## Piezas
| Pieza | Dónde | Qué hace |
|---|---|---|
| Anfitrión | `AnfitrionWeb.*` | Crea WebView2 dentro de la ventana principal (carga `WebView2Loader.dll` en tiempo de ejecución). Sirve `ui/` desde `https://phoenix.local/` (nada sale a internet salvo imágenes de avatar `https:`). |
| Puente | `Puente.*` | JSON entre C++ y JS: peticiones con id, respuestas, estado y eventos. |
| Interfaz | `InterfazWeb.*`, `EstadoWeb.cpp`, `Acciones*Web.cpp` | Ciclo de vida, estado que se publica (5 veces/s, solo si cambió) y acciones. |
| Respaldo | `InterfazWeb::renderImGui` | Si WebView2 falta o falla (30 s sin «hola», o el proceso muere 3 veces en 2 min), vuelve la interfaz ImGui de Phoenix con un aviso «Reintentar / Instalar». |
| UI | `ui/` (copiada junto al exe al compilar) | Preact 10 + htm (sin compilación), fuentes locales (OFL). Abrir `ui/index.html` en un navegador muestra la **vista previa** con `js/simulador.js`. |

## Mensajes
- JS → C++: `{t:"hola"}` · `{t:"pedir", id, accion, datos}`
- C++ → JS: `{t:"bienvenida", datos:{protocolo, idiomas, resoluciones, chat, actividad, pantallas, gpus, wgc, temasOverlay, estado}}` · `{t:"resp", id, ok, datos | error:{codigo, mensaje}}` · `{t:"estado", datos}` · `{t:"evento", nombre:"chat"|"actividad", datos:{reinicio, lineas}}`
- Errores comunes: `ACCION_DESCONOCIDA`, `DATOS_INVALIDOS`, `FUERA_DE_RANGO`, `SIN_SALA`, `SALA_CERRADA`, `MANDOS_OCUPADOS`, `ERROR_INTERNO`.
- Respuestas diferidas (esperan a la web de la liga, hasta 30 s): `amigos.invitar`, `web.soltarRival`.

## Acciones (todas usan el mismo camino que el control original)
- **App:** `ui.seccion{seccion,pestana}` (el motor solo manda serie de red, audio y amigos de lo que se mira), `ui.tema{tema}`, `ui.idioma{idioma}`, `ui.interfaz{modo:phoenix|clasica}`, `ui.panelClasico{seccion 0-3,pestana}`, `ui.copiar{texto}`, `ui.abrir{url https}`, `ui.recargar`.
- **Sala:** `sala.abrir`, `sala.cerrar`, `sala.copiarEnlace`, `sala.opciones{nombre,plazas,limitador,limite,biblioteca,quiosco,overlay,turnos}`, `sala.aplicar`, `sala.phoenix{visibilidad,espectadores,limiteEspectadores,entradaParsec,juego,parche,region}`, `sala.calidad{fps,mbps}`, `perfilesSala.lista|guardar|aplicar|borrar{nombre}`.
- **Web de la liga:** `web.vincular{codigo}`, `web.reintentar`, `web.desvincular`, `web.soltarRival`.
- **Mandos:** `mandos.conectar|desconectar|bloquear|liberar{indice}`, `mandos.asignar{indice,parsecId}`, `mandos.intercambiar{a,b}`, `mandos.formacion{local,visitante}`, `mandos.tomar{numero 0-8}`, `mandos.herramienta{nombre:reiniciar|desconectarTodos|ordenar|bloquearTodo|bloquearBotones}`, `mandos.cantidad{xbox,ds4}` (≤ 8), `solicitud.aceptar|rechazar{parsecId}`, `espera.decidir{parsecId,como:jugador|espectador|expulsar}`, `turnos.activar{si}`, `turnos.ajustes{juegoMin,reinicioMin,recordatorioMin}`.
- **Gente:** `gente.mod|vip|expulsar|banear{parsecId}`, `gente.teclado|raton{parsecId,si}`, `moderacion.listas`, `moderacion.desbanear|quitarMod|quitarVip|banear{parsecId}`, `moderacion.motivo{parsecId,motivo}`, `chat.enviar{texto}`, `amigos.invitar{usuarioId}`.
- **Ajustes:** `ajustes.general{clave,valor}`, `ajustes.permisos{grupo,clave,valor}`, `ajustes.video{clave,valor}`, `ajustes.videoListas`, `ajustes.audio{canal,clave,valor}`, `ajustes.overlay{clave,valor}`, `diag.ejecutar`.
- **Partido:** `partido.preparar{a[],b[],nombreA,nombreB,asignar}`, `partido.iniciar{anunciar}`, `partido.gol{lado,delta}`, `partido.pausa{si}`, `partido.cambiarLados`, `partido.finalizar` → `{registro,guardado}`, `partido.cancelar`, `partido.historial{max}`.

## Archivos que crea en el PC
- `%APPDATA%\Trybuchet\Smash Soda\phoenix-partidos.json` (historial, máx. 200), `phoenix-perfiles-sala.json` (máx. 20), `phoenix-eventos.json` (cola de eventos para la web, máx. 500).
- `%LOCALAPPDATA%\PhoenixLink\WebView2` (perfil del navegador interno).

## Reglas de la UI
Texto ≥ 12 px · sin `confirm/alert/prompt` (diálogo propio) · movimiento 100–220 ms solo transform/opacity · `prefers-reduced-motion` respetado · hover con movimiento solo con ratón · el campo que se está escribiendo no se pisa con el estado del motor (guarda al salir o con Enter).
