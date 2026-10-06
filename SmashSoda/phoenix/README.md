# phoenix/ · Código propio de Phoenix Soda

Todo lo de Phoenix vive aquí. El código del autor solo recibe ganchos de 1–5 líneas, marcados con `phoenix::`, para que sincronizar versiones nuevas de Smash Soda sea fácil.

| Archivo | Qué hace |
|---|---|
| `PhoenixBuild.h` | Interruptores de la build (Soda Arcade y actualizaciones del autor apagados) |
| `PhoenixTheme.h` | Tema «Phoenix» (cian/púrpura), predeterminado |
| `PhoenixRoles.*` | Roles de sala + puerta de mandos + expulsión de no listados |

## Ganchos en código del autor
| Archivo | Gancho |
|---|---|
| `services/Arcade.cpp` | Funciones de red de Soda Arcade devuelven `false` |
| `Main.cpp` | No muestra login de Soda Arcade ni avisos de actualización del autor |
| `services/ThemeController.*`, `core/Config.h` | Registra el tema Phoenix y lo hace predeterminado |
| `core/GamepadClient.cpp` | `tryAssignGamepad` (autoíndice) y `pick` (!swap) consultan la puerta de mandos |
| `Hosting.cpp` | `processNewGuestConnection` expulsa a no listados si la sala lo pide |

## Probar la puerta de mandos (fase A)
1. Crear `%APPDATA%\Trybuchet\Smash Soda\phoenix-sala.json`:
```json
{
  "activo": true,
  "expulsarNoListados": false,
  "jugadores": [
    { "parsecId": 111111, "nombre": "Retador", "asiento": 1 },
    { "parsecId": 222222, "nombre": "Retado",  "asiento": 2 }
  ],
  "espectadores": [
    { "parsecId": 333333, "nombre": "Amigo" }
  ]
}
```
2. Los `parsecId` salen de la ventana de invitados de Smash Soda (el `#número` junto al nombre).
3. La app relee el archivo cada 2 s: no hace falta reiniciar. El log muestra `[Phoenix] Roles activos: …`.
4. Sin archivo, o con `"activo": false`, todo funciona como el Smash Soda original.

Límite conocido: el host todavía puede asignar mandos a mano desde su propia interfaz (se bloqueará en una fase siguiente).
