# Capítulo 60-l — ¿Sirve el código online «Team Play» 11 vs 11 para tener 22 humanos en local? (20-oct-2026)

Solo análisis estático (Ghidra, proyecto `ghidra_proyecto_completo`, mismas direcciones que el exe en vivo). No se tocó el juego, el módulo ni Link.
Salidas crudas: `C:\Temp\claude\D--Frank-IA-CLAUDE\6f3f30c3-8566-4cb2-b4c3-680fb60ba325\scratchpad\re\teamplay\` (cad.txt, clases.txt, vt_online.txt, vt_monline.txt, dec1.txt, dec2.txt, slots.txt).
Script nuevo: `scripts\ghidra\SlotsDec.java` (`<salida> <0xvtable> <0xdesp>...` → descompila las funciones de esas casillas de una vtable).

## 0. Respuesta corta

**NO conviene** reutilizar el camino online. El juego no tiene una «entrada de mando remoto» aparte que podamos rellenar: en línea cada ranura de cursor lleva **qué máquina la controla** (byte +0x02) y **qué mando de esa máquina** (byte +0x10). La máquina que es dueña lee su mando local y el resultado viaja por la sesión P2P de Konami. Para usarlo en local habría que **falsificar una sesión de red completa** (objeto de sesión, número de estación, colas de juego `onlinesystem::gamequeue`, comandos `MultiplayCmd*Sync`). Esfuerzo: semanas, alto riesgo de cierres. Seguir buscando el **tope local de 8** es más barato, y este análisis deja una pista concreta (sección 4).

## 1. ¿Cómo entra la entrada de los jugadores remotos?

- Interfaz global de «partido online»: puntero `PTR_1434a2d40`. Por defecto apunta a un objeto vacío (`0x1434a2d48`, clase base `match::registry::online::Interface`, vftable `0x142644638`, todas las casillas devuelven 0/1). En partido online se sustituye por **`match::Online`** (vftable `0x142bf1a28`, ctor `FUN_14213c120`, init `FUN_14213d460`; se restaura con `FUN_14213eb00`).
- `match::Online` guarda la sesión de red en `+0x30` (objeto con sesión en `+0x1c0` y caché en `+0x5500…`) y datos de partido en `+0x48`.
- Casillas clave de `match::Online`:
  | casilla | función | qué hace |
  |---|---|---|
  | +0x08 | `FUN_14213e2d0` | ¿hay sesión? (`+0x30 != 0` y `FUN_142143d50`) |
  | +0x98 | `FUN_14213def0` | **número de estación (máquina) local** vía `FUN_142143760` → `FUN_141254ce0(sesión)`; 0xFF si no hay sesión |
  | +0x1d8 | `FUN_14213d420` | lee el **mando físico local** `slot+0x10` (`FUN_141b524c0` → gestor de mandos `DAT_1437f0a58` vt+0x18) y devuelve dirección 0–3 del stick |
  | +0x28 | `FUN_14213cfc0` | guarda un valor por ranura en una tabla de **24** (`[+0x48]+0x10 +0x140 + i*4`, `i < 0x18`) |
  | +0x2e0 | `FUN_14213c4f0` | reenvía a otro objeto (`FUN_142162020` vt+0x60) |

- Bucle que lo demuestra: **`FUN_1420b8e40`** recorre las **24 ranuras de cursor** (paso 0x74) y solo procesa las que cumplen:
  ```c
  if (slot[3] != 0 && slot[0xd] == 0 && slot[2] == Online->vt[0x98]()) // mi estación
      Online->vt[0x1d8](Online, slot[0x10], &dir);                     // mi mando local
  ```
  Es decir: **cada PC solo lee sus propios mandos**; las ranuras de otras estaciones se rellenan con lo que llega por la red.
- La misma función copia cada fotograma desde un búfer circular de fotogramas de **0x1FB0 bytes** (`[[DAT_1436f5118+0x6a8]+0x40] + frame*0x1FB0`) con entradas de **300 bytes por jugador** (`(n-1)*300 + 0x36b`). 0x1FB0 deja sitio para ~24 entradas de 300. *Hipótesis:* es el registro de fotogramas para el modo bloqueado (lockstep)/repetición; **sin confirmar**.
- Las tablas online van hasta **24**, no 8 (la tabla de +0x140 y el bucle de 0x18). El límite de 8 **no está** en el camino online.

## 2. ¿Qué decide si una ranura es local o remota?

- **`slot+0x02` = estación dueña**, comparada con `Online vt+0x98`. **`slot+0x10` = mando físico en esa estación**. `slot+0x03` = activa, `slot+0x0d` debe ser 0.
- Sin sesión online la interfaz es la vacía: vt+0x98 de la base no existe como «estación», y el juego usa el camino local (el de `S+0x180`, índice de usuario local < 8, ya documentado en cap_60_k).
- ¿Marcar los mandos 9–24 como «humanos remotos» y meterles la entrada nosotros? **No es directo**: «remoto» significa «sus datos llegan de la sesión de red». Para eso hay que crear `match::Online` con una sesión válida en `+0x30`, que `FUN_141254ce0` devuelva una estación, y alimentar la cola de fotogramas que normalmente escribe el código de red. Ese escritor no se ha localizado (está detrás de `onlinesystem::gamequeue::*`, `onlinemode::MultiplayCommand<…>`, `fox::nio::impl::NetworkConnectionImpl`).

## 3. ¿Se puede arrancar Team Play sin servidores de Konami?

- `FUN_14103b3a0` = `[DAT_1436fe468]+0x10 → byte +0x15B` (devuelve 1 si no hay objeto). Se llama muchas veces (≥9 solo en una función) → es una bandera de estado de conexión muy repartida.
- Modo actual: `FUN_14149a470` → `DAT_143704e38`; `FUN_14149f650` dice «modo online» si `[+0xF0]-2` ∈ {0, 0x16–0x1d, 0x21–0x2a, 0x34–0x3a}.
- El flujo pasa por login (`menu::OnlineFlowLogin*`), sala (`menu::OnlineLobbyRoom*`, `onlinemode::GetCoopRoomMember`), sincronización de datos (`MultiplayCmdUserDataSync`, `MultiplayCmdTeamDataSync`, `MultiplayCmdMatchMenuUpdate`) y `CoopMatch` → `CoopMatchTeamSelect` (`FUN_140bf7720`). Cada paso espera respuestas del servidor o de los pares.
- Conclusión: decenas de comprobaciones y, sobre todo, **estado de red que hay que fabricar** (no son solo `if` que se puedan saltar). Difícil.

## 4. Veredicto y pista del tope local

- **Online: NO viable a corto plazo** (estimado: 3–6 semanas de RE + emulador de sesión, con muchos cierres posibles).
- **Tope local: más viable** (estimado: días). Pista nueva: la lectura del mando físico pasa por **`FUN_141b524c0(índice)` → `DAT_1437f0a58` vt+0x18** (gestor de mandos). Si para índices ≥ 8 devuelve NULL o un mando «no conectado», el jugador tendría PadInput lleno pero el motor no lo tomaría. **Sin confirmar.**

## 5. Siguientes pasos concretos

1. Descompilar la vtable del objeto en `DAT_1437f0a58` (casilla +0x18) y ver si limita el índice a 8 (`cmp ecx,8` o tabla de 8).
2. Buscar en el consumidor del cursor el uso de `slot+0x10` y `S+0x180` (con `BuscarDesplazamiento.java 0x180`, `0x10` cerca de 0x74) para hallar el `if (idx >= 8)` o `!= 0xFF`.
3. Solo si 1–2 fallan, volver a lo online: localizar quién escribe el búfer de 0x1FB0 (referencias a `DAT_1436f5118+0x6a8`).
