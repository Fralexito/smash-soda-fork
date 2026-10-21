# Ranking de jugadores 1 contra 1 (Phoenix Link → web)

Estado: **Link HECHO** (envía `partida_fin` v2). **Web PENDIENTE-WEB** (`docs/PROMPT-WEB-ranking-1v1.md`).

## 1. Flujo de hoy (gol → Link → historial → web)

```
PES 2021 + phoenix_estadio.lua (Sider)
   │  estado.json cada ~1 s: fase, minuto, goles_local/visita, goles[{m,l}], pk_*, completo, equipos
   ▼
PhoenixLink (hilo propio)  link/EstadoJuego.cpp · solo lectura; >5 s = «sin datos»
   │  PhoenixLink::estadoJuego()
   ▼
Árbitro (web/Arbitro.cpp, 2 veces/s)
   ├─ arranque automático: lados «listo» + reloj corriendo → Partido::iniciar + evento partida_inicio
   ├─ goles del juego → Partido::fijarGoles (lado A = local salvo «Cambiar lados»)
   ├─ chat: «GOL 34' de Mirko. Mirko 2-1 Kaiser», descanso
   └─ fase final + completo → finalizarPartido()
                                   │ (también el botón FINALIZAR: partido.finalizar)
                                   ▼
web/AccionesPartidoWeb.cpp · finalizarPartido()
   ├─ Partido::finalizar → RegistroPartido
   ├─ historial local %APPDATA%\Trybuchet\Smash Soda\phoenix-partidos.json (últimos 200)
   └─ PhoenixLink::evento("partida_fin", datos)  → cola phoenix-eventos.json (máx. 500, persistente)
                                   │ cada 15 s, lotes de 50
                                   ▼
POST /v1/eventos {sala_id, eventos[{clave uuid, tipo, ocurrido, datos}]}  (contrato §6, token del PC)
```

**Antes de este cambio** `partida_fin` solo llevaba `{"marcador":"2-1"}` (o `"cancelado"`). La web no sabía quién jugó, en qué lado, qué goles ni si el partido terminó de verdad.

Quién juega dónde: `partido.preparar{a[],b[]}` guarda en cada lado los jugadores (parsecId + nombre). Esos son los puestos de mando que el host elige en Partido. La cuenta web **no** la decide Link: la web la resuelve con `cuentas_parsec` (la misma que usa `/v1/perfiles`). Link solo tiene una copia de los perfiles para mostrarlos, y la web no debe fiarse de ella.

## 2. Qué faltaba para un ranking fiable → qué manda ahora Link

| Necesidad | Antes | Ahora (`partida_fin` v2) |
|---|---|---|
| Identidad de cada jugador | — | `a/b.jugadores[].parsec_id` (texto de dígitos). La web lo pasa a `usuario_id` |
| Resultado | `marcador` | `a.goles`, `b.goles`, `ganador`, `juego.penales_a/b` |
| Goles por persona | — | 1v1: los goles del lado son de su único jugador. `juego.goles[{m,lado}]` con minuto |
| Válido o abandonado | — | `cierre`: `completo` (el juego confirmó el final) · `manual` (lo cerró el host) · `cancelado`. Y `abandono_juego` |
| Partido corto | — | `duracion_seg` (reloj real sin pausas) y `juego.minuto` |
| Misma persona en los dos lados | Link ya lo impide (`Partido::preparar`) | La web lo vuelve a comprobar con `usuario_id` |
| Marcador tocado a mano | — | `correcciones` (veces que se usó +/−) y `juego.coincide` (marcador de Link = marcador del juego) |
| Jugador que se fue | — | `jugadores[].presente` al terminar |
| Host que juega y cierra él | — | `jugadores[].es_host`, `host_parsec` |
| Duplicados | — | `partido_id` (= `inicio_ms`, el mismo id que el historial local) + `clave` del evento |

Partidos con varios por lado: se mandan igual con `modo:"equipos"`. Se guardan, pero **no se atribuyen goles a personas** (eso lo investiga otra sesión).

## 3. Contrato: `datos` de `partida_fin` v2

Mismo endpoint y mismo tipo (`POST /v1/eventos`, tipo `partida_fin`). Solo crecen los `datos`, que el contrato deja libres (≤ 4 KB). Si no caben, Link quita `juego.goles`. `marcador` se mantiene, así que la web vieja no se rompe.

```json
{
  "v": 2,
  "marcador": "2-1",                 // "cancelado" si se canceló
  "partido_id": "1760140000000",     // epoch ms del inicio; único por sala
  "inicio_ms": 1760140000000, "fin_ms": 1760140720000,
  "duracion_seg": 690,               // reloj real de Link sin pausas
  "cierre": "completo",              // completo | manual | cancelado
  "modo": "1v1",                     // 1v1 (1 persona por lado) | equipos
  "ganador": "a",                    // a | b | empate | "" (cancelado)
  "a": { "nombre": "Mirko", "goles": 2, "local": true,
         "jugadores": [ { "parsec_id": "111", "nombre": "Mirko", "presente": true, "es_host": true } ] },
  "b": { "nombre": "Kaiser", "goles": 1, "local": false,
         "jugadores": [ { "parsec_id": "222", "nombre": "Kaiser", "presente": true, "es_host": false } ] },
  "correcciones": 0, "pausas": 1,
  "lados": { "origen": "host", "correcciones": 0 },   // host = los eligió el anfitrión; mandos = Link los armó solo al pitazo
  "host_parsec": "111",
  "abandono_juego": false,
  "juego": {                         // solo si había estado.json fresco al cerrar
    "fase": "final", "minuto": 90, "completo": true,
    "equipo_a": 101, "equipo_b": 102, "nombre_equipo_a": "Real Madrid", "nombre_equipo_b": "Barcelona",
    "goles_a": 2, "goles_b": 1, "coincide": true,
    "penales_a": 4, "penales_b": 3,  // solo si hubo tanda
    "goles": [ { "m": 12, "lado": "a" }, { "m": 34, "lado": "b" }, { "m": 80, "lado": "a" } ]
  }
}
```

- `local`: el lado A juega con el equipo local, salvo con los mandos cruzados (tras «Cambiar lados»). `equipo_*`, `goles_*`, `penales_*` y `goles[].lado` ya vienen pasados a lados A/B.
- `presente`: el jugador seguía en la sala al cerrar (el host siempre).
- Sin `v` (Link viejo) = solo marcador: la web lo guarda como evento y no cuenta para el ranking.
- `partida_inicio` y `pausa` no cambian.

Local: `phoenix-partidos.json` y `partido.historial` ganan `correcciones`, `pausas` y `cierre` (los registros viejos se leen igual).

## 4. Cómo lo guarda y calcula la web (propuesta)

Detalle (SQL, RLS, criterios) en `docs/PROMPT-WEB-ranking-1v1.md`. Resumen:

- **`partidos_link`**: una fila por `partida_fin` v2 (todos los modos, también cancelados). Única por `(sala_id, partido_id)`. Guarda `datos` en bruto, el `estado_ranking` (`valido` | `revisar` | `excluido`) y sus `motivos`.
- **`partidos_link_jugadores`**: lado, `parsec_id`, `usuario_id` resuelto **al recibirlo** (foto fija), nombre, presente, es_host y `goles` (solo 1v1).
- **`partidos_link_goles`**: minuto, lado y `usuario_id` (solo 1v1; en `equipos`, nulo).
- **`ranking_1v1`**: por usuario: elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5. Se recalcula desde cero, en orden de fecha, con `recalcular_ranking_1v1()` cada vez que entra o se revisa un partido 1v1 (a esta escala es barato y deja corregir la historia).

### Reglas de validez (las aplica la web, no Link)

Excluido (se guarda, no cuenta):
- `modo` ≠ `1v1`, `cierre` = `cancelado`, o no es `v` 2.
- Algún jugador sin cuenta web vinculada, o el mismo `usuario_id` en los dos lados.
- `duracion_seg` < 180.

Por revisar (no cuenta hasta que lo apruebe el staff):
- `cierre` = `manual` (el juego no confirmó el final).
- `lados.origen` = `mandos` y `lados.correcciones` = 0: Link armó los lados según la formación y nadie los confirmó (pueden estar al revés). Esta regla se quita cuando Link detecte el lado de cada mando en el juego (llegará `origen: "juego"`).
- `correcciones` > 0, `juego.coincide` = false o `abandono_juego`.
- Algún jugador con `presente` = false.
- Más de 3 partidos válidos de la misma pareja en 24 h (anti-farmeo): del 4.º en adelante.

Válido: todo lo demás (lo normal: `completo`, sin tocar, los dos presentes).

### Puntuación: Elo + tabla de puntos

- **Orden del ranking: Elo.** Empieza en 1000. E = 1 / (1 + 10^((Rrival − R)/400)). S = 1 / 0,5 / 0. K = 40 en los 10 primeros partidos válidos y 24 después. Por diferencia de goles, ×1 (1 gol), ×1,5 (2 goles), ×(11+N)/8 (3 o más).
  - Por qué Elo: en una liga pequeña, ganarle al mejor vale más que ganarle a un novato. Además, farmear contra cuentas débiles casi no da puntos. Con puntos puros, gana quien más juega.
- **Columnas de la tabla (fáciles de leer):** PJ, PG, PE, PP, GF, GC, DG, Pts (3/1/0) y racha (`G3`, `E1`, `P2`).
- Penales: cuentan como empate para Elo, PG, PE y Pts. La tanda se guarda y se muestra aparte.
- Con menos de 5 PJ el jugador sale como «provisional», debajo o con marca.

## 5. Archivos de Link tocados

- `SmashSoda/phoenix/web/AccionesPartidoWeb.cpp`: `ladoWeb()` y `datosResultado()`. `finalizarPartido()` y `partido.cancelar` mandan `partida_fin` v2. El historial guarda `cierre`.
- `SmashSoda/phoenix/web/Partido.h/.cpp`: cuenta `correcciones` (+/− que cambian el marcador) y `pausas`. Expone `inicioMs()`, `correcciones()` y `pausas()`. `RegistroPartido` gana `correcciones`, `pausas` y `cierre`.
- No se tocó el árbitro, los mandos ni la UI. `completo` se lee de `estadoJuego()` dentro de `finalizarPartido`, que el árbitro llama justo en `final` + `completo`.

## 6. Riesgos y pendientes

- Sin probar con PES real (igual que phoenix_estadio.lua). Sin `estado.json`, todo sale como `manual` y queda «por revisar».
- Si se cruzan los mandos a mitad de partido, `goles[].lado` usa el cruce del final. En 1v1 casi no pasa.
- El `usuario_id` lo decide la web en el momento de recibir el partido. Si alguien vincula su Parsec después, sus partidos viejos no se le asignan (se podría rehacer con una función de staff).
- Goles por persona en partidos con varios por lado: lo investiga otra sesión.
