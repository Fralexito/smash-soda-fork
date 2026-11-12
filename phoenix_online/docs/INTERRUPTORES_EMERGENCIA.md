# Interruptores de emergencia (prueba de 2 jugadores)

Sirven para cuando una parte del online falla durante la prueba. Se apaga solo esa parte y se sigue con las demás, **sin reiniciar el servidor**.

## Cómo se usa

1. Abrir `config.json` del servidor (está en la PC de Frank y nunca se sube a GitHub).
2. En `"interruptores"` → `"apagados"`, escribir el nombre del módulo. Ejemplo:

   ```json
   "interruptores": { "apagados": ["matchday", "chat"], "error": "ERR_SERVERNOTREADY" }
   ```

3. Guardar el archivo. El servidor lo lee solo en unos 5 segundos, y en su log aparece «recargado en caliente … apagados=[…]».
4. Para volver a encenderlo, quitar el nombre de la lista y guardar.

Cuando un módulo está apagado, el PES que intenta entrar recibe un error y vuelve al menú. Lo demás sigue igual. Cada rechazo queda en el log como «interruptor: módulo X apagado».

> Sin probar todavía en el PES: el mensaje exacto que muestra el PES con ese error. Si se queda colgado en vez de volver al menú, cambiar `"error"` por `"ERR_FORBIDDEN"`.

## Módulos (nombre → qué corta)

| Nombre | Qué apaga | Comando que corta |
|---|---|---|
| `divisiones` | Divisiones en línea (OC_RANK) | CMD_START_MATCHING con OC_RANK |
| `partida_rapida` | Partida rápida (OC_QUICK) | CMD_START_MATCHING con OC_QUICK |
| `salas` | Salas de amistoso (crear y unirse) | CMD_CREATEJOIN_ROOM, CMD_JOIN_ROOM |
| `cooperativo` | Cooperativo 2 contra 2 | START_COOP_MATCHING, CREATEJOIN/JOIN_COOP_ROOM |
| `matchday` | Entrar a jugar Matchday | CMD_JOIN_MATCHDAY |
| `pesleague` | Inscribirse en PES League / Torneo | CMD_JOIN_PESLEAGUE_COMPE |
| `eventos` | Eventos de competición | CMD_JOIN_COMPEV, CMD_REENTRY_COMPEV |
| `chat` | Chat de texto | CMD_SEND_TEXTCHAT |
| `clanes` | Crear, entrar e invitar a clanes | CREATE_CLAN, ENTRY_CLAN, INVITE_CLAN |
| `subastas` | Pujar en subastas | CMD_TENDER_MYCLUB_AUCTION_SCOUT |
| `intercambio` | Intercambio de jugadores | CMD_TRADE_MYCLUB_GAMEPLAYER |
| `turn` | Relé TURN: el PES deja de recibirlo y solo intenta conexión directa | lista de CMD_GET_TURN_SERVER_LIST vacía |

## Otras claves que también se cambian sin reiniciar

| Clave | Para qué |
|---|---|
| `max_partidos_rival_dia` | Tope de partidos contra el mismo rival al día (0 = sin tope). Útil para la prueba: subirlo si molesta |
| `sonda_vida` | Comprobación de que el rival sigue vivo (anula el partido si no responde) |
| `revision_check` | Comprobación de datos (DATAPACK) antes del partido |
| `veredicto_al_final` | Decidir el resultado al final con los dos informes |
| `emparejar_mismos_datos` | Solo emparejar PES con los mismos datos |
| `exigir_versiones` | Versiones de PES exigidas |
| `direccion_para_remoto` | Cambiar la IP del anfitrión por la que ve el rival (Tailscale) |
| `palabras_bloqueadas` | Palabras prohibidas en nombres y chat |
| `myclub.*` | Todo myClub: `abandono_es_derrota`, `logros.activo`, `subastas.activa`, `dificultad.activa`, `intercambio.activo`, etc. |

## Lo que NO se cambia sin reiniciar

`gamerelay`, `turn_propio`, `turn`, `ip_publica`, los puertos, las claves y `supabase`. Esas solo se leen al arrancar. **Durante la prueba no se reinicia el servidor**, así que si hace falta tocarlas, se hace al final o con los dos PES fuera del online.
