# Campo 2D en vivo (Partido › Campo)

Muestra en Phoenix Link, en tiempo real, las fichitas de los jugadores de PES 2021 sobre un campo 2D, como el radar del modo entrenador.
Cómo se encontró todo (fuentes, estructura de memoria, pruebas): [CAMPO-2D-INVESTIGACION.md](CAMPO-2D-INVESTIGACION.md).
Para replicarlo paso a paso (humano o IA): [CAMPO-2D-REPLICAR.md](CAMPO-2D-REPLICAR.md).

## Cómo se usa

1. Abre **Partido › CAMPO** en Link.
2. Abre PES (el modo de ConmeGOL sirve) y empieza el partido.
3. Tarda unos 15 s en «encontrar» a los jugadores la primera vez (barre la memoria del juego una vez) y luego se mueven en vivo, ~20 veces por segundo.

Colores: cian = local, violeta = visitante, amarillo = asistentes de línea, gris = equipo aún desconocido. El balón es la bolita blanca.

## Qué hace Link (y qué NO)

- **Solo lee.** Abre PES2021.exe con permiso de lectura (`PROCESS_VM_READ`) y nada más: no escribe, no inyecta, no hookea, no pausa el juego, no puede desincronizar un partido.
- **Solo funciona con el ejecutable conocido.** Antes de leer comprueba tamaño y SHA-256 de `PES2021.exe` (hoy: 458 806 784 bytes, `5E27A782…BC224A`, el del parche ConmeGOL y SP2026). Con otro ejecutable la tarjeta dice «no compatible» y no toca nada.
- **Se apaga sola** si lo que lee no tiene sentido (jugadores fuera del campo, valores raros, más de 3 jugadores que dejan de ser válidos): vuelve a buscar con calma o se queda esperando.
- **Coste cero fuera de la tarjeta.** Con la tarjeta cerrada no hay hilo leyendo ni evento alguno. Con la tarjeta abierta: ~20 lecturas por segundo de 16 bytes por jugador.
- Si PES no está abierto, la tarjeta dice «Esperando a que empiece un partido en PES».

## Piezas

| Pieza | Dónde |
|---|---|
| Firma y validación (lógica pura, con pruebas) | `SmashSoda/phoenix/core/CampoVivo.{h,cpp}` · `pruebas-link/campo_test.cpp` |
| Lector del juego (hilo propio, Windows) | `SmashSoda/phoenix/host/CampoVivo.{h,cpp}` |
| Acción `campo.vivo`, evento `campo`, estado `campo` | `web/AccionesMandosWeb.cpp`, `web/InterfazWeb.cpp`, `web/EstadoWeb.cpp`, `web/PROTOCOLO.md` |
| Vista (SVG, interpolación suave) | `web/ui/js/vistas/campo.js` (pestaña CAMPO de Partido) |
| Simulador | `web/ui/js/simulador.js` (22 fichitas + balón + 2 asistentes en movimiento) |
| Prueba manual con el juego abierto | `pruebas-link/campo_vivo_real.cpp` |

## Pendiente

- **Equipo de cada fichita** (hoy salen en gris salvo los asistentes): se fija con el saque de centro.
- **Balón** (hoy la bolita no se dibuja con datos reales; en el simulador sí).
- Mini-radar en el overlay PhoenixGlass (opcional).

## 0.2.21 — equipos, porteros, balón y tanda de penales

- **Colores por equipo**: A (cian) = el que ocupaba la izquierda en el primer saque de centro que vio Link; B (violeta); árbitros en amarillo. Como en el descanso se cambian los lados, A/B se mantienen por identidad (no por lado). Si abres Link a mitad de un tiempo, las fichitas van en gris hasta el siguiente saque (gol o descanso).
- **Balón**: la bolita blanca; se agranda un poco cuando va por el aire. Desaparece durante las repeticiones (el juego la aparca).
- **Porteros**: ya cuentan (24 fichitas: 22 + 2 árbitros… y el árbitro principal cuando el saque lo identifica).
- **Tanda de penales**: debajo del campo aparece la tanda en vivo: una fila por equipo con sus lanzamientos (✓ gol, ✗ fallo o parada), el marcador de la tanda y «A punto de patear: Equipo A/B». Los nombres de los lanzadores todavía no se leen del juego (pendiente).

## 0.2.22 — mini-radar en el overlay

- En **Partido › CAMPO** hay un interruptor «Mini-radar en el overlay». Encendido, Link lee el juego aunque esa tarjeta esté cerrada y manda el campo al overlay (WebSocket, evento `phoenix:campo`, ~10/s).
- En el **overlay › Widgets › Mini-radar** se enciende el widget y se elige la esquina (apagado por defecto). Se puede mover/redimensionar como los demás. Si dejan de llegar datos (juego cerrado, repetición larga) se esconde solo a los 4 s.
- Coste: lo mismo que la tarjeta (≈ 20 lecturas/s de 16 bytes por actor) y solo mientras el interruptor esté encendido.
- Código: overlay `frontend/src/components/overlay/widgets/OverlayRadarWidgetView.vue` + `models/widgets/RadarWidget.ts` (commit `2f740db` en `phoenix-overlay`, sin push); Link `web/InterfazWeb.cpp` (envío) y `PhoenixPrefs.radarOverlay`.
