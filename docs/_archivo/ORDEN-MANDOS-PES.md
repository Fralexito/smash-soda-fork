# Orden de mandos en PES 2021: que el puesto N de Link sea el jugador N de PES

10–11 de octubre de 2026. Frente LINK. Estado: **EXPERIMENTAL** (funciona a mano en la PC de Fralex; falta probar el modo automático).

---

## 1. El problema (de toda la comunidad PES + Parsec)

Con varios mandos (Parsec, ViGEmBus, DS4Windows, HidHide…) PES 2021 nunca asigna los mandos en el mismo orden:
- un amigo que en Link es el **puesto 2** acaba siendo el **jugador 5** en PES;
- a veces el mando del anfitrión deja de reconocerse o «se pulsa solo».

## 2. Qué descubrimos (causa exacta)

1. **PES 2021 elige los mandos por su `settings.dat`.** Está en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\settings.dat` y ocupa 612 bytes. Lo escribe `Settings.exe`.
   - Desde el byte `0x64` hay **8 casillas de 64 bytes**: la casilla 1 es el jugador 1, la 2 el jugador 2… hasta la 8.
   - Cada casilla tiene el **GUID de instancia DirectInput** del mando (16 bytes, siempre terminan en `444553540000`) y su **mapa de botones** (48 bytes).
   - **Un mando que no está en ninguna casilla, PES lo ignora.**
2. **Windows reparte esos GUID según el orden en que se conectan los mandos**, y los vuelve a repartir cada vez que los mandos virtuales (ViGEm) se desconectan y se conectan:
   - En la PC de Fralex había **más de 100 entradas fantasma** de mandos Xbox 360 y **15 GUID** reciclados.
   - Cerrar DS4Windows o reabrir Link **cambió el GUID** de varios puestos.
   - Antes, Link desconectaba sus 8 mandos al cerrar la sala y los conectaba al abrirla, así que barajaba en cada sesión.
3. **Cada mando virtual de Link tiene un número de serie en el bus ViGEm** (`USB\VID_045E&PID_028E\01…08`), y Link puede leerlo (`vigem_target_get_index`). La cadena completa es: GUID de PES → mando HID → serie ViGEm → puesto de Link.
4. **DS4Windows** crea su propio mando Xbox virtual en el mismo bus, así que aparece un 9.º mando que desplaza a los demás.
5. **Marionetas** (tu mando físico maneja un puesto): fallaban porque el maestro se elegía por posición en una lista que incluía los propios mandos de Link y se reordenaba sola. Así se colaba otro mando y el puesto 2 «se pulsaba solo».

## 3. Prueba que confirmó la solución (PC de Fralex, 10 oct, ~23:50)

1. Con la sala de Link abierta, se leyó el GUID actual de cada puesto con una sonda de solo lectura.
2. Se escribió `settings.dat` con **casilla N = puesto N**, con el mapa de botones Xbox y copia de seguridad antes.
3. Se abrió PES y en la pantalla de elegir lados se probó Ctrl+Alt+1, 2, 3, 4… (el anfitrión maneja ese puesto con el teclado).
4. Resultado de Fralex: **«todo estaba en la posición correcta perfectamente»**.

## 4. La solución dentro de Phoenix Link (EXPERIMENTAL)

Está en **Mandos › Puestos › ORDEN DE MANDOS EN PES** (interruptor). Encendido:
- Link lee el GUID de cada puesto. El código está en `SmashSoda/phoenix/core/OrdenPes.*`.
- Con **PES y Settings cerrados**, escribe `settings.dat` con casilla N = puesto N:
  - antes guarda una copia `settings.dat.phoenix-<fecha>`;
  - escribe de forma atómica;
  - las casillas sin puesto quedan vacías.
- **Ya no desconecta sus mandos al cerrar la sala**, así Windows no los vuelve a barajar mientras Link esté abierto.
- Muestra las 8 casillas con ✓ y te avisa si hay que cerrar PES («El orden cambió y PES está abierto»).
- Marionetas arregladas: el maestro **nunca** es un mando de Link y se reconoce por su ID, no por su posición.

## 5. Guía paso a paso (cualquier PC)

1. Cierra **DS4Windows** (y Steam Input si lo usas para el mando).
2. Abre Phoenix Link, ve a **Mandos › Puestos**, baja hasta **ORDEN DE MANDOS EN PES** y **enciéndelo**.
3. Cierra PES y su Settings. En unos segundos la tarjeta debe decir **«Ordenado»**, con las 8 casillas en ✓.
4. Abre la sala y luego PES. El puesto 1 de Link es el jugador 1, el 2 el jugador 2…
5. **Para jugar con tu propio mando (anfitrión):** ve a **Mandos › TU MANDO**, elige tu mando y arrástrate a un puesto en **Mandos › Puestos**. PES no ve tu mando directamente, porque no está en ninguna casilla: juegas a través de Link. No hace falta HidHide. Ver `TU-MANDO.md`.
6. Si cambias algo con PES abierto (reabrir Link, enchufar o desenchufar mandos), la tarjeta avisa: cierra PES y Link vuelve a ordenar solo.

**Volver atrás:** apaga el interruptor y restaura la copia `settings.dat.phoenix-<fecha>` (o abre Settings.exe y configúralo a mano).

## 6. Límites conocidos

- PES lee `settings.dat` **solo al arrancar**: cualquier cambio de orden necesita reiniciar PES.
- Probado con mandos virtuales Xbox de Link y un DualShock 4 físico. Con un mando **Xbox físico** puede que PES lo lea por XInput aunque no esté en ninguna casilla; está sin probar.
- El mapa de botones de los puestos de Link es el mapa Xbox que ya tenía el archivo (o el de fábrica de Settings).

## 7. Herramientas de diagnóstico usadas

- `sonda.cpp`: lista los mandos DirectInput con su GUID y su origen (serie ViGEm o físico).
- `monitor.cpp`: muestra en vivo qué mando ocupa cada casilla de PES y qué está pulsando.
- Las dos están en el scratchpad de la sesión del 10 de octubre. La lógica de la sonda ya está dentro de `OrdenPes.cpp`.
- Prueba: `pruebas-link/ordenpes_test.cpp`. Con el `settings.dat` real de Fralex da el mismo archivo que funcionó en PES.
