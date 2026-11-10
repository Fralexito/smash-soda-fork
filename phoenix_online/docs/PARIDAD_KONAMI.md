# Online igual (o mejor) que el de Konami: qué hay y en qué orden sigue (10-nov-2026)

Pedido de Frank (10-nov): «que el online funcione tal cual funcionaba con Konami original y mucho mejor
incluso, pero con todo lo que necesito para hacer mi parche». PES: solo el base `C:\PES21\eFootball PES 2021`.

## Cobertura de comandos (cuenta automática sobre `docs/cmd_lista.txt`)
- El juego conoce **439** comandos. Hoy tienen manejador propio **153**, y unos 30 más se contestan con un
  simple «NOERR» (confirmaciones). Los demás reciben NOERR genérico (no cierra el juego, pero el menú puede salir vacío).
- `GET /estado` cuenta en vivo qué comandos llegan sin manejador: así se sabe qué falta de verdad, sin adivinar.

| Modo | Estado | Comandos que faltan |
|---|---|---|
| Entrar al online, perfil, avisos, vestíbulo, salas | ✅ funciona | — |
| Partida rápida y Divisiones 1v1 | ✅ emparejar + relé de 2 programado; ⏳ prueba con 2 PES | pocos (READY_FLAG, SELECT_SIDE van por el relé) |
| Amistoso con amigo (sala libre) | 🟡 salas OK; el partido va directo PC a PC (P2P). Con Tailscale no hace falta TURN | — |
| myClub vs COM | ✅ funciona (fichajes, contratos, subastas, entrenadores, GP) | 48 extra (nombres de club/plantilla, regalos, logros, historial, rankings, alquiler, objetos de habilidad…) |
| Cooperativo (2v2, 3v3) | ❌ no hecho | 46 |
| Matchday (eventos de clubes reales) | ❌ no hecho | 20 |
| Eventos/competiciones online (COMPEV) | ❌ no hecho | 11 |
| PES League | ❌ no hecho | 15 |
| Clanes | ❌ no hecho | 21 |
| Tienda / monedas de pago | 🚫 a propósito: nada de dinero real (las monedas solo se regalan) | 9 |

## Lo que lo hace «mejor que Konami» para el parche de Frank
- **Mismo parche obligatorio:** el servidor recibe la huella de los archivos del juego (`file_hash_list`) y puede
  emparejar solo a quien tiene los mismos archivos (`emparejar_misma_huella`, se enciende tras la prueba con 2 PES)
  y rechazar versiones distintas al entrar (`exigir_versiones`).
- **Reglas sin reiniciar** (`reglas_myclub.json`), **mantenimiento** sin cortar partidos, **respaldos** automáticos,
  **arrancador** que reabre el servidor, **estado** en vivo (`/estado`) para Link, la web o el celular.
- **Base de jugadores propia:** myClub lee el catálogo del parche (Player.bin y equipos), así que los fichajes son
  los jugadores del parche de Frank, no los de Konami.
- **Identidad por persona:** provisional por IP (Tailscale), luego el pase de la web.

## Orden propuesto (de más útil a menos)
1. Prueba real del partido de 2 (rival de prueba → amigo por Tailscale) y arreglar lo que salga.
2. myClub extra rápido: nombre del club y de la plantilla, historial, regalos y logros (recompensas gratis),
   rankings semanales. Luego, partido igualado contra humanos en myClub.
3. Cooperativo (2v2/3v3): salas coop + relé ya preparado para más de 2 (la lista del canal 5 admite hasta 31).
4. Eventos propios (COMPEV/Matchday) armados desde la web: torneos y desafíos de la liga Phoenix.
5. PES League y clanes (si Frank los quiere).
Cada modo: estudio de solo lectura de la referencia (con nuestras palabras) → código + pruebas → prueba en vivo.
