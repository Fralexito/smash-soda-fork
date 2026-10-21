# Pasada visual (Link 0.3.35 – 0.3.40, 19-oct-2026)

Pedido de Fralex sobre 0.3.34: «más visual, más dinámico, más intuitivo, más premium y profesional, quiero dinamismo».
Rama `plan-ux`. Volver a 0.3.34: etiqueta `antes-pasada-visual-0.3.34` o la copia `x64\Release\PhoenixLink-0.3.34.exe`
(carpeta completa en `C:\dev\PhoenixLink-respaldo\Link-0.3.34-completo\`). Volver a 0.3.20: `docs/VOLVER-A-0.3.20.md`.

## En toda la app (lo que vale para todos los apartados)

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

## Apartado por apartado

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

## Sin probar en la app real

- Que el movimiento vaya fluido en el WebView2 de Link con PES delante (sin foco se para; con foco, comprobar que no hay tirones).
- Destellos con datos reales (alguien que entra y se sienta, la web que se conecta, el juego que se conecta).
- Esqueletos con la carga real (en el simulador todo llega en milisegundos).
- Barra de Mi juego avanzando durante un ARREGLAR TODO real.
