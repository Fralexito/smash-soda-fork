# Sistema de jugadores: estadísticas, ranking, rangos, prestigio y logros

Estado: **DISEÑADO, SIN APLICAR** (decisión de Fralex, 2026-10-11).
- Link: el recolector de fichas está hecho y compila, pero **apagado** (`kFichaJugadorActiva = false` en `SmashSoda/phoenix/web/EstadisticasPartido.h`). Mientras esté apagado, `partida_fin` sale igual que con el ranking 1v1.
- Web: `docs/PROMPT-WEB-sistema-jugadores.md` está escrito, **sin enviar**. Va después de `docs/PROMPT-WEB-ranking-1v1.md`.
- Base: `docs/RANKING-1V1.md` (flujo, `partida_fin` v2, reglas de validez y Elo).

## 1. Hasta dónde se puede llegar (el techo)

| Nivel | Fuente | Qué da, por persona | Fiabilidad | Estado |
|---|---|---|---|---|
| 1a | Partido de Link + `phoenix_estadio.lua` (`match.stats()`) | Resultado, goles del lado (1v1 = suyos), minuto de cada gol, penales, equipo del juego, rival, cómo terminó, duración | Alta | **Se envía ya** (`partida_fin` v2) |
| 1b | Anfitrión de Parsec (Link) | Su mando, pulsaciones, pulsaciones por minuto, Start pulsados, inactividad, ping medio y máximo, segundos con lag, caídas, tiempo fuera | Alta (Link lo ve todo) | **Hecho, apagado** (ficha) |
| 2 | Sider sin tocar memoria | Nada más: `match.stats()` no da goleador, tarjetas, tiros ni posesión | — | Techo del modo seguro |
| 3 | Leer la memoria de PES (Sider `memory` / ReadProcessMemory, como `phoenix.lua`) | Lo que PES muestra al final del partido: goleador y asistente, tiros, posesión, pases, faltas, tarjetas y qué futbolista lleva cada mando | Media: depende de direcciones de memoria que cambian con cada versión | **Solo investigación** (§7) |

Límites que ningún nivel supera:
1. Solo existen los partidos hospedados con Phoenix Link y con `phoenix_estadio.lua` instalado en la PC del anfitrión.
2. Solo cuenta quien tenga su Parsec vinculado a su cuenta de la web.
3. Los datos salen de la PC del anfitrión. Se puede dificultar y vigilar la trampa, pero no eliminarla del todo.
4. Cada evento admite 4 KB. Hoy cabe, porque Link quita primero la lista de goles y luego las fichas. El nivel 3 necesitaría una ruta nueva en la web.
5. En partidos con varios por lado, los goles no se atribuyen a personas hasta tener el nivel 3 (otra sesión investiga qué mando controla a cada futbolista).

## 2. Ficha por jugador y partido (nivel 1b)

`EstadisticasPartido.cpp` mira la sala 20 veces por segundo, y solo con el partido **en juego** (la pausa no cuenta). Se guarda por `parsecId` y se añade a cada jugador de `partida_fin` como `ficha`:

```json
"ficha": {
  "mando": 1, "cambios_mando": 0,
  "seg_mando": 640,            // segundos en juego con mando
  "seg_fuera": 0, "caidas": 0, // fuera de la sala
  "pulsaciones": 2890, "pulsaciones_min": 271,
  "start": 1,                  // veces que pulsó Start
  "seg_inactivo": 35, "inactivo_max_seg": 9,   // nada pulsado y sticks en reposo
  "ping_medio": 48, "ping_max": 130, "seg_lag": 0   // solo invitados (> 150 ms)
}
```
- Solo lee lo que recibe el mando virtual. No toca mandos, sala ni juego.
- Una pulsación de menos de 50 ms se puede perder, así que las pulsaciones son aproximadas. Sirven para comparar, no como cifra exacta.
- No se guarda nada en bruto: solo el resumen. Hay que avisar a los jugadores de que se registra su actividad (página de privacidad de la web).

## 3. Identidad: ¿obligar a entrar a la sala por la web?

**Recomendación: no obligar a entrar siempre por la web. Sí obligar a vincular el Parsec una vez para que cuenten las estadísticas.**

- El ID de Parsec no cambia. Una vez vinculado en la web (`cuentas_parsec`), Link y la web saben quién es en cualquier sala, entre por donde entre. Pasar cada vez por la página no añade seguridad y sí molestias: amigos que se pasan el enlace, reconexiones a mitad de partido, gente que solo quiere mirar.
- Los partidos de quien no esté vinculado se guardan igual, pero no puntúan (motivo `sin_cuenta`). Si se vincula después, el staff puede reasignarlos.
- **Más adelante (opcional): «Sala clasificatoria».** Un interruptor del anfitrión con dos efectos:
  1. Solo quien tenga cuenta vinculada puede tomar un mando. Los demás entran como espectadores y reciben un aviso con el enlace para vincularse.
  2. Al empezar, Link avisa en el chat si alguien no puntuará.

  Esto se haría en los puestos de mando (`PhoenixRoles`), en una sesión aparte, porque esos archivos los lleva otra. Con eso se consigue lo que buscas (que cuente todo de cada uno) sin obligar a nadie a pasar por la web cada vez.
- La entrada por la web (radar, «Ver», rival aceptado) sigue existiendo y es la recomendada para retos y torneos.

## 4. Qué se construye en la web con estos datos

### 4.1 Ranking (ya diseñado)
Elo, PJ, PG, PE, PP, GF, GC, DG, puntos y racha. Reglas en `RANKING-1V1.md` §4.

### 4.2 Rangos (según el Elo de la temporada)
| Rango | Elo |
|---|---|
| Bronce | < 900 |
| Plata | 900 – 1049 |
| Oro | 1050 – 1199 |
| Platino | 1200 – 1349 |
| Diamante | 1350 – 1499 |
| Leyenda | ≥ 1500 |
| Élite (insignia) | top 10 de la temporada, con 20 o más partidos |
Con menos de 5 partidos válidos: «Sin clasificar».

### 4.3 Prestigio (XP que nunca baja)
- Por partido válido: +10 jugar, +5 ganar, +2 empatar. Racha de victorias: +3 por cada victoria desde la 3.ª (máximo +15). Partido limpio (ver fair play): +3.
- Nivel = floor(√(XP / 25)) + 1. Ejemplo: 100 XP → nivel 3; 2500 XP → nivel 11.
- **Estrellas de prestigio**: una por temporada jugada con 20 o más partidos, y otra extra por terminar en Diamante o mejor.
- Por qué existe aparte del Elo: el Elo mide lo bueno que eres y puede bajar; el prestigio premia jugar y ser constante, y nunca se pierde.

### 4.4 Fair play (0–100, empieza en 100)
| Hecho (de `partida_fin` y la ficha) | Efecto |
|---|---|
| Partido abandonado (`abandono_juego`, o `presente: false` al final) | −15 |
| Caída durante el partido (`caidas`) | −3 cada una (máximo −9 por partido) |
| Inactivo 60 s o más seguidos (`inactivo_max_seg`) | −5 |
| 4 o más Start en un partido 1v1 (`start`) | −2 |
| Partido limpio (completo, sin caídas, sin inactividad larga) | +2, hasta 100 |
Por debajo de 60: marca visible y el staff revisa sus partidos. Por debajo de 40: sus partidos van a «revisar» automáticamente.

### 4.5 Logros (insignias)
Primer partido · Primera victoria · Triplete (3 goles o más en un 1v1) · Goleada (ganar por 4 o más) · Portería a cero · Remontada (ir perdiendo por 2 y ganar, según `juego.goles`) · Gol en el descuento (minuto ≥ 90) · Racha 5 / Racha 10 · Invicto 10 · Matagigantes (ganar a alguien con 200 o más de Elo por encima) · Rey de los penales (5 tandas ganadas) · Fiel a su club (50 partidos con el mismo equipo) · Trotamundos (10 equipos distintos) · Centenario (100 partidos) · Juego limpio (20 partidos limpios seguidos) · Incansable (300 o más pulsaciones por minuto de media en 10 partidos).

### 4.6 Temporadas
Una temporada dura lo que decida el staff (por ejemplo, un mes o «Temporada 1»). Al cerrarla se guarda una foto del ranking (posición, rango y Elo) y se hace un reinicio suave: Elo = 1000 + (Elo − 1000) / 2. El prestigio y los logros no se reinician.

### 4.7 Ficha pública del jugador y cara a cara
- **Ficha:** rango, Elo, nivel y estrellas, fair play, logros, últimos 10 partidos, equipo favorito, goles por partido, % de victorias, ping medio y pulsaciones por minuto de media.
- **Cara a cara (A vs B):** partidos, victorias de cada uno, empates, goles y último resultado.

## 5. Contrato de Link (cuando se encienda)
- Sin cambios de endpoint: todo va en `partida_fin` v2. Cada `a/b.jugadores[]` gana `ficha` (opcional).
- Orden de recorte si `datos` pasa de 3,8 KB: primero `juego.goles`, después las `ficha`. El resultado siempre llega.

## 6. Cómo se activa (en este orden, cuando Fralex lo diga)
1. Chat WEB: aplicar `PROMPT-WEB-ranking-1v1.md`, y después `PROMPT-WEB-sistema-jugadores.md`.
2. Link: poner `kFichaJugadorActiva = true` en `EstadisticasPartido.h`, compilar y probar un 1v1 offline (comprobar que la ficha llega en el evento).
3. Instalar `phoenix_estadio.lua` (Módulos del juego) y probar con un amigo.
4. Quitar los `PENDIENTE-WEB` de `COORDINACION.md`.

## 7. Nivel 3 (memoria del juego): plan de investigación, sin implementar
Va contra «si algo se rompe, mejor no lo hagas» mientras no esté probado, así que solo se investiga:
1. Solo lectura y offline, nunca en partidos online. Con el mismo método de seguridad que `phoenix.lua`: comprobar los bytes antes de leer y apagarse solo tras varios errores.
2. Buscar en PES 2021 (la misma versión del ejecutable, con su sha256) las estructuras de estadísticas del partido: goles con autor, asistencias, tiros, posesión y tarjetas. Contrastarlas con la pantalla de estadísticas de 5 partidos reales.
3. Si sale bien, `phoenix_estadio.lua` escribiría `estadisticas.json` al final del partido y Link lo enviaría por una ruta nueva (pedírsela al chat WEB), porque no cabe en 4 KB.
4. La atribución por persona en partidos con varios por lado depende de saber qué mando controla a cada futbolista (lo investiga otra sesión).

## 8. Archivos
- `SmashSoda/phoenix/web/EstadisticasPartido.h/.cpp` (nuevos): recolector y `fichaJugador()`.
- `SmashSoda/phoenix/web/AccionesPartidoWeb.cpp`: `ladoWeb()` añade la ficha y recorta si no cabe.
- `SmashSoda/phoenix/web/InterfazWeb.cpp`: una línea, `tickEstadisticas(in)` después de `tickDatos(in)`.
