# Propuesta: más datos del partido para la web (25-oct-2026)

Hoy Phoenix Link ya manda a la web el resultado, los goles con su minuto y su goleador, los penales y la pantalla en la que está el juego (`estado.json`, `resultado.json` y `pantalla.json` de `phoenix_referee.lua`). Esto es lo que se puede sumar, ordenado de más fácil a más difícil.

Reparto: la parte de Link la hace el hilo 1 y la de la web, la Cuenta 2. Este hilo no toca ninguno de los dos.

## A. Ya disponible, sin investigar (solo leer y mandar)

| Dato | De dónde sale | Idea para la web |
|---|---|---|
| Estadio, clima, día o noche, estación | `ctx.stadium`, `ctx.weather`, `ctx.timeofday`, `ctx.season` (sonda) | Ficha del partido: «Anfield · lluvia · noche» |
| Duración, dificultad, prórroga, penales, número de cambios | `ctx.match_time`, `difficulty`, `extra_time`, `penalties`, `substitutions` | Validar que un partido de liga se jugó con las reglas de la liga (anti-trampas) |
| Tanda de penales en vivo | `pk_local` y `pk_visita` (ya están en `estado.json`) | Marcador de penales en la tarjeta en vivo |
| Minutos añadidos | `anadido` (ya está en `estado.json`) | «45+3'» en vivo |
| Modo de juego | zona de la sonda (EXHIBITION, UEFA_ML, EDIT…) | Estado de la sala: «en el menú», «eligiendo equipos», «en Liga Máster» |
| Quién juega en cada lado, su mando y su ping | `sala.txt` (`ja=` / `jb=`), de Link | Alineación de personas en vivo y «ping del partido» |

## B. Con poco trabajo (los datos ya llegan, falta traducirlos)

| Dato | De dónde sale | Trabajo |
|---|---|---|
| Tarjetas amarillas y rojas, cambios, faltas, córners, fueras de juego, lesiones (con su minuto) | `escenas.log` (referee v1.3): nombres de las escenas que pide el juego | Tabla «escena → suceso», con unos cuantos partidos de prueba. Da un **minuto a minuto** completo. |
| Más datos del gol (asistente, tipo de gol) | evento `goal_scored` de Sider 7.3.4+ | La sonda guarda la tabla entera del evento: se mira qué campos trae. |

## C. Necesita ingeniería inversa (fase 3 del MCP)

| Dato | Pista que ya tenemos |
|---|---|
| Posesión, tiros, tiros a puerta, faltas totales | El juego guarda estadísticas en `record::Observer` (se vio `ObserverFoul`, de 512 bytes, al estudiar las rojas) |
| Alineación de los 22 jugadores y quién sale o entra | Gestión de equipo en `gestor+0x1928` / `+0x1978` (sin confirmar) |
| Pelota y jugadores en un campo 2D en vivo | Ya conocemos el objeto de la pelota (640 bytes, posición en +208/+212/+216). Link ya tiene `CampoVivo`. |
| Nota del jugador y MVP | Sin investigar |

## Recomendación
1. **Ahora:** lo del bloque A. Link solo tiene que leer `sonda.json` y añadir los campos a `partida_fin` y al estado en vivo; la web los muestra.
2. **Después:** el minuto a minuto del bloque B, que es lo que más se ve en la web y no toca la memoria.
3. **Más adelante:** el bloque C, con el MCP para investigar más rápido.
