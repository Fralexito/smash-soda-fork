# PROMPT para el chat WEB · Tablero del partido al tocar «En vivo ahora» (campo 2D + estadísticas)

> Repo `Fralexito/phoenixevolution`, rama `borrador`. Origen: chat LINK, 2026-10-11.
> Fralex: «en el overlay no tiene que aparecer el campo 2D, debe aparecer en la web: por encima sale la tarjeta como ahora y, al darle clic, se abre el tablero con las demás estadísticas del partido».

## Qué ya hace Phoenix Link (rama `rediseño-phoenix-portal`)
- La tarjeta sigue igual: `liga_vivo` vía `POST /v1/liga/vivo` (marcador, minuto, fase, pantalla, goles con goleador).
- **Nuevo:** con «Campo en vivo en la web» encendido en Link (Partido › CAMPO, apagado por defecto) y el partido en juego en una sala que ya sale en «En vivo ahora», Link manda **como mucho 1 vez por segundo**:
  ```
  POST {base}/functions/v1/liga/v1/liga/campo     (mismo token y cabeceras que /v1/liga/vivo)
  { "sala_id": "uuid", "v": 1,
    "j": [[x, z, eq], …],          // 20–25 fichitas: metros, centro = (0,0); x ∈ [-52.5, 52.5] (largo), z ∈ [-34, 34] (ancho)
                                    // eq: 0 = equipo A, 1 = equipo B, 2 = árbitro/asistente, -1 = aún sin equipo
    "b": [x, z, y] | null,         // balón (y = altura en m); null si no se ve
    "t": { "l": [[eq, gol], …], "e": eq, "listo": bool, "vuelo": bool },   // solo durante una tanda de penales
    "equipos": true,               // false = los colores aún no se saben (llegan en el siguiente saque de centro)
    "posesion": [55.3, 44.7] }     // % equipo A / B (aproximada: quién está más cerca del balón); falta los primeros segundos
  ```
  ~0,5–1 KB por mensaje. Si la ruta no existe (404), Link espera 5 min antes de volver a probar.
- **Equipo A/B frente a local/visita:** A = el equipo que estaba a la **izquierda** en el primer saque de centro que vio Link. No se ha comprobado que eso sea siempre el local (otra sesión lo está investigando). En el tablero, pon a A como local y a B como visita, con una nota pequeña «colores aproximados», hasta que Link mande `"eq_local"` (vendrá en una versión posterior: si llega, úsalo).

## Qué hay que hacer

### 1) Función `liga`: ruta `POST /v1/liga/campo`
- Autenticar igual que `/v1/liga/vivo` (`autenticar`, `salaDe`). Validar: `j` array ≤ 30 de `[num, num, int]`, `b` null o 3 números, cuerpo ≤ 4 KB, `posesion` 2 números entre 0 y 100.
- Límite: 2 por segundo por sala (más → `429` con `reintentar_en`).
- **No escribir en la base.** Reenviar por **Realtime Broadcast** al canal `campo:<sala_id>`, evento `campo`, con el cuerpo sin `sala_id`. Desde la Edge Function: `sb.channel('campo:' + sala).send({ type: 'broadcast', event: 'campo', payload })` (supabase-js v2; usa el envío HTTP sin suscribirse) o la REST `POST {SUPABASE_URL}/realtime/v1/api/broadcast` con la service key.
- Respuesta `{ ok: true }`. Contrato: documentarla en `docs/LIGA-AUTOMATICA.md`; sube la versión de la función.
- Canal **público de solo lectura** (cualquiera que vea la tarjeta puede mirar el tablero; son datos del partido, sin nombres de cuentas ni Parsec).

### 2) Tarjeta → tablero
- `src/components/MarcadorVivo.astro` y `src/js/features/liga/vivo.js`: toda la tarjeta se puede pulsar (y con teclado: `role="button"`, Enter/Espacio). Abre un **tablero** (modal en escritorio, hoja a pantalla completa en el móvil; cerrar con ✕, Esc o deslizando) con:
  1. **Cabecera** igual que la tarjeta: tipo (amistoso u oficial), equipos, marcador, minuto y fase o pantalla, en vivo con la misma suscripción a `liga_vivo`.
  2. **Campo 2D en vivo:** SVG del campo (105 × 68 con líneas, áreas y círculo central). Las fichitas con los colores del equipo A, el B y los árbitros, y el balón un poco más grande (si `y > 2`, con sombra). Interpola entre mensajes (~1 s) para que se mueva suave, sin saltos.
  3. **Posesión:** barra A/B con los porcentajes (o «calculando…» si aún no llega).
  4. **Goles:** línea de tiempo con el minuto, el goleador y el lado (de `liga_vivo.goles`).
  5. **Tanda de penales** si llega `t`: fila de círculos por equipo (gol / fallo / pendiente).
  6. Si en 5 s no llega ningún `campo`: «El campo en vivo no está activado en esta sala» (el resto del tablero sigue funcionando).
- Suscribirse a `campo:<sala_id>` **solo con el tablero abierto**, y desuscribirse al cerrarlo. Si la fila de `liga_vivo` se borra (el partido terminó), mostrar «Partido terminado» con el marcador final y no cerrar de golpe.
- Rendimiento: un solo `requestAnimationFrame` para la interpolación; nada de re-render por cada mensaje si el tablero está oculto.

## Criterios de aceptación
1. La tarjeta se ve igual que hoy y, al pulsarla, abre el tablero (escritorio y móvil).
2. Con Link enviando, las fichitas y el balón se mueven suaves (1 mensaje/s interpolado) y la posesión se actualiza.
3. `POST /v1/liga/campo` no escribe en la base (comprobar que no crece ninguna tabla) y rechaza a quien no es dueño de la sala.
4. Sin campo (Link viejo o interruptor apagado), el tablero muestra la cabecera, los goles y el aviso.
5. Cerrar el tablero cierra la suscripción al canal.
6. Accesible: foco atrapado dentro del modal, Esc cierra, contraste AA.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK (para quitar el `PENDIENTE-WEB` y, si hace falta, ajustar el formato).
