# PROMPT para el chat WEB · Marcador «En vivo ahora» que se queda colgado

> Repo `Fralexito/phoenixevolution` (rama `borrador`). Origen: chat LINK, 2026-10-11. Fralex: «se quedó así, hace horas que se dejó de jugar» (Arensz 2-1 Erfuzz, 90' · 2.º tiempo).

## Por qué pasa
`liga_vivo` solo se borra cuando Link manda `POST /v1/liga/vivo {terminado:true}` (o con `/v1/liga/resultado`). Link solo lo mandaba si cerraba el partido en su marcador con la sala aún publicada. Si el host cerraba la sala o Phoenix Link, cerraba PES o el partido se abandonaba, la fila se quedaba para siempre.
- `sala/cerrar` y la marca de sala `caida` (3 min sin latido) **no** borran `liga_vivo`.
- Las salas no se borran: el `on delete cascade` nunca llega a actuar.
- `vivo.js` muestra todas las filas, sin mirar `actualizado`.

## Ya hecho en Phoenix Link (rama `rediseño-phoenix-portal`)
- Al cerrar la sala o la app, Link manda `terminado:true` **antes** de `sala/cerrar`.
- Mientras hay partido y el juego da datos, Link reenvía el marcador **al menos cada 2 minutos** (aunque no cambie nada). Así `actualizado` sirve de latido.

## Qué hay que hacer en la web
1. **Limpiar ya** (una vez). Usa los estados reales de `salas`:
   ```sql
   delete from public.liga_vivo v
   using public.salas s
   where s.id = v.sala_id and (s.estado <> 'abierta' or v.actualizado < now() - interval '10 minutes');
   ```
2. **Función `phoenix`**: en `POST /v1/sala/cerrar`, y donde se marca una sala como `caida`, borrar también su fila de `liga_vivo` (`delete … where sala_id = $1`).
3. **Limpieza periódica** (pg_cron cada 5 min, o dentro del proceso que ya marca las salas caídas): borrar las filas de `liga_vivo` con `actualizado` de más de 10 minutos, o cuya sala no esté `abierta`.
4. **`src/js/features/liga/vivo.js`**: no mostrar filas con `actualizado` de más de 10 minutos, y volver a comprobarlo cada minuto (sin esperar a Realtime), así una tarjeta vieja desaparece aunque nadie la borre. Al recibir un DELETE por Realtime, quitar la tarjeta (comprobar que ya lo hace).

## Criterios de aceptación
1. Tras el paso 1, la tarjeta «Arensz 2-1 Erfuzz» ya no sale.
2. Abrir sala, jugar y **cerrar la sala sin terminar el partido** → la tarjeta desaparece en segundos.
3. Cerrar Phoenix Link a la fuerza (sin avisar) → la tarjeta desaparece como mucho en 10 minutos.
4. Un partido con una pausa larga en el juego **no** desaparece: Link manda el latido cada 2 minutos.
5. Nada de lo publicado en el contrato cambia.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK para quitar el `PENDIENTE-WEB`.
