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
| 7 | Interruptores de emergencia: `interruptores.apagados` en config.json, en caliente (docs/INTERRUPTORES_EMERGENCIA.md) | Opus | ✅ |
| 8 | Copia de seguridad de la base y de `config.json` justo antes de la prueba | sesión PC | ⏳ |
| 9 | **Firewall de Windows (solo lectura):** el amigo debe poder entrar por Tailscale a python.exe (TCP 10000, 10001, 5000, 80; UDP 3478, 5730, 5100; TURN 5349/5350 + UDP 49160-49179) y los dos PES2021.exe deben recibir UDP del otro. `tools/amigo/REVISAR_ANTES_DE_LA_PRUEBA.ps1` lo comprueba. Cualquier cambio en el firewall lo decide y lo hace Frank (límite 3) | sesión PC + Frank | ⏳ |

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

## Revisión de código (12-nov, hallazgos)

- Direcciones: con `ip_publica` "auto" cada PES recibe la IP por la que llegó; `_direcciones_para` cambia el host_addr de Frank (127.0.0.1/192.168.x) por su IP de Tailscale cuando quien mira es el amigo (100.64/10 se trata como remota). El amigo publica su 100.y por nuestro STUN. Correcto en el código.
- Identidad: con Steam compartido, cada jugador se distingue por IP (Tailscale da una distinta). Hay prueba automática.
- Registro: cada conexión deja su JSONL en `capturas/` y el HTTP en `capturas/http.jsonl` (con tope de disco). Sirve para corregir sin repetir. Nunca se suben a GitHub.
- `tools/comparar_con_azrak.py`: los «faltan» en listas vacías son falsos positivos (la lista no trae elementos en la prueba); no hay huecos en los modos de 2 jugadores que cambien el flujo.
- Riesgo principal fuera del código: el **firewall** (punto 9).

## C. Reglas del día

- Nadie reinicia el servidor con un PES dentro del online.
- Cada fallo: hora exacta, pantalla, mensaje y código; el servidor guarda el resto.
- Al final: paquete de diagnóstico + copia de seguridad.
- Si algo grave bloquea, apagar solo ese módulo con su interruptor y seguir con los demás.
