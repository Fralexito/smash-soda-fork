# Overlay premium: marcador de fútbol y panel de amigos (Link 0.2.73)

## Qué trae
- **Marcador de partido** (abajo al centro, pequeño): equipos, goles, minuto y estado (en vivo, pausa, descanso, final). No estorba: asoma 8 s al empezar, 10 s con cada gol, se queda en el descanso y el final con el resumen de goles (con goleador si el juego lo dice) y en juego solo reaparece 6 s cada 90 s.
- **Cartel «¡GOOOL!»** con equipo, goleador, minuto y marcador.
- **Panel de amigos** (como el de Discord): `Shift + |` o `Ctrl + Alt + A`. Se abre encima del juego, lista a tus amigos de la web por estado (jugando, en sala, activos, ausentes, desconectados), con buscador y botón **Invitar**. `Esc`, el mismo atajo o un clic fuera lo cierran. Sin sala abierta avisa y no deja invitar.

## Cómo funciona
- Link manda al overlay (WebSocket local): `phoenix:partido` (lo que lee del juego, ver `Arbitro.cpp › marcadorOverlay`) y `phoenix:amigos` (solo con el panel abierto), y recibe `phoenix:panel {abierto}` (latido cada 4 s mientras está abierto: así Link sondea amigos aunque su ventana no se vea) y `phoenix:invitar {usuarioId}`. El resultado vuelve en `phoenix:invitacion`.
- La invitación usa la misma petición que Gente › Amigos (`/v1/invitar`).
- Overlay: `frontend/src/components/overlay/widgets/PhoenixMarcadorView.vue` y `PhoenixAmigosPanelView.vue`, estilos en `phoenix-futbol.css`.

## Probar sin PES
En `phoenix-overlay/frontend`: `npx vite --config vite.preview.config.ts` y abrir `http://127.0.0.1:8767/preview/index.html?esc=futbol&panel=1&gol=1` (también `fase=descanso|final`, `sala=0`).

## Pendiente de probar en el juego real
Marcador con un partido de PES de verdad, atajo `Shift+|` dentro de PES y la invitación a un amigo conectado.

## Nota
Antes de esto el overlay **no arrancaba nunca**: su manifiesto tenía `version="0.1.0"` (Windows exige cuatro números). Corregido a `1.0.0.0`.
