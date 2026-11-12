# Plan: prueba de 2 jugadores (Frank + amigo) en una sola sesión

Objetivo (Frank, 12-nov): dejar todo el apartado online listo para que, en la prueba con su amigo, se encuentre y se corrija la mayor cantidad de fallos posible **sin repetir sesiones**.

## A. Antes del día (sin amigo)

| # | Qué | Quién | Estado |
|---|-----|-------|--------|
| 1 | Kit del amigo: scripts, instructivo simple, comprobador de PES | sesión PC | ✅ 782c39b |
| 2 | Tailscale: iniciar sesión en la PC de Frank y anotar la IP 100.x | Frank o PC | ⏳ |
| 3 | Ensayo con `tools/rival_falso.py` en cada modo (OC_RANK, OC_QUICK, `--revision`, Matchday, Torneo) con el PES de Frank; anotar hasta dónde llega y qué falla | sesión PC | ⏳ |
| 4 | Revisión de escritorio del camino de 2 jugadores: unirse a sala, relé, TURN, resultado, abandono, reconexión, comprobación de datos (DATAPACK), anti-trampa (sumas) | Opus | ⏳ |
| 5 | `tools/comparar_con_azrak.py` sobre TODAS las respuestas de los modos de 2 jugadores | cualquiera | ⏳ |
| 6 | Registro del día: log del servidor con hora + peticiones/respuestas de cada jugador (claves y secretos tachados) + eventos del relé y TURN; un comando que empaquete todo en una carpeta de diagnóstico | cualquiera | ⏳ |
| 7 | Interruptores de emergencia: lista por módulo (cómo apagarlo con una clave de config) | cualquiera | ⏳ |
| 8 | Copia de seguridad de la base y de `config.json` justo antes de la prueba | sesión PC | ⏳ |

## B. Guion del día (orden propuesto)

1. Comprobar que el PES del amigo es igual al de Frank (`COMPROBAR_MI_PES.bat`).
2. Los dos con Tailscale; el amigo ejecuta `CONECTAR_PHOENIX.bat`.
3. Servidor arrancado **y ya no se reinicia durante la prueba**.
4. Amistoso: crear sala (Frank) → el amigo la ve → se une → partido corto → resultado.
5. Divisiones en línea (OC_RANK): emparejamiento, comprobación de red, partido, puntos y rating.
6. Partida rápida (OC_QUICK).
7. Matchday y Torneo (partido entre los dos).
8. Abandono a mitad de partido (3-0), reconexión, corte de red.
9. Mismo rival varias veces (tope diario de 3).
10. TURN: forzar relé aunque la conexión directa sea posible.
11. Cooperativo 2 contra 2 solo si el Core 0.4.97 está instalado en los dos PES (opcional).

## C. Reglas del día

- Nadie reinicia el servidor con un PES dentro del online.
- Cada fallo: hora exacta, pantalla, mensaje y código; el servidor guarda el resto.
- Al final: paquete de diagnóstico + copia de seguridad.
- Si algo grave bloquea, apagar solo ese módulo con su interruptor y seguir con los demás.
