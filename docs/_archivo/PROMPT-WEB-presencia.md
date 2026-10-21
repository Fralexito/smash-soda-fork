# PROMPT para el chat WEB · Presencia: «Activo» solo en la web, «En sala» con Phoenix Link

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). Origen: chat LINK, 2026-10-11.

## Qué quiere Fralex
«Que salga **activo** en Phoenix si está dentro de la página web, y que salga **en sala** si tiene abierto Phoenix Link.»

Hoy un jugador sale **ACTIVO** (`disponible`) solo por tener Phoenix Link abierto, aunque no esté en la web. Eso cambia.

## Estados (los mismos nombres de siempre)
| Estado | Cuándo | Quién lo marca |
|---|---|---|
| `disponible` («ACTIVO») | Tiene la **página web** abierta y visible | **La web** (nuevo) |
| `en_sala` («EN SALA») | Tiene **Phoenix Link** abierto | Link, por `POST /v1/presencia` |
| `en_partida` («JUGANDO») | Partido en curso o PES abierto | Link, por `POST /v1/presencia` |
| `desconectado` | Nada de lo anterior en los últimos ~2 min | Caduca solo |

Si hay varios a la vez, gana el más fuerte: `en_partida` > `en_sala` > `disponible`.

## Qué cambia en Phoenix Link (ya hecho, rama `rediseño-phoenix-portal`)
`POST /v1/presencia` (mismo endpoint, contrato §presencia):
```json
{ "estado": "en_sala", "origen": "link", "sala_id": "…" }      // Link abierto
{ "estado": "en_partida", "origen": "link", "sala_id": "…" }   // jugando
```
- `en_sala` reemplaza al `disponible` que mandaba Link. `origen` es un campo nuevo y opcional.
- Si la web responde **400** a eso, Link vuelve solo a mandar `{"estado":"disponible"}` sin `origen`, como antes (no se pierde la presencia). Por eso conviene **aceptar `en_sala` y `origen`** antes de que Fralex reparta el nuevo Link.

## Qué tiene que hacer la web
1. **Aceptar** en `POST /v1/presencia` el estado `en_sala` y el campo opcional `origen` (`"link"`). No romper con valores desconocidos: ignorarlos.
2. **Latido de la página web:** mientras la pestaña está abierta y visible (`document.visibilityState === "visible"`), marcar `disponible` cada ~60 s (la misma tabla de presencia, `origen: "web"`). Al ocultarse o cerrarse, dejar de marcarlo (caduca a los ~2 min, o `navigator.sendBeacon` al cerrar).
3. **Guardar por origen:** la presencia de Link y la de la web no se pisan. Lo que se muestra es la más fuerte de las que no han caducado.
4. **Mostrar:** en la lista de jugadores, perfiles y amigos:
   - «ACTIVO» (verde) solo con `disponible` de la web;
   - «EN SALA» con `en_sala` de Link;
   - «JUGANDO» con `en_partida`.
   `GET /v1/presencia/amigos` sigue devolviendo `estado` con esos mismos nombres (Link ya los pinta: `disponible` = ACTIVO, `en_sala` = EN SALA, `en_partida` = JUGANDO).
5. **Compatibilidad:** un Link viejo sigue mandando `disponible` sin `origen`. Tratarlo como `en_sala` (viene de Link, no de la web).

## Hecho cuando
- Con solo Phoenix Link abierto, el jugador sale «EN SALA», no «ACTIVO».
- Con solo la web abierta, sale «ACTIVO».
- Con las dos abiertas, sale «EN SALA». Jugando, sale «JUGANDO».
- Al cerrar todo, pasa a desconectado en ~2 min.
