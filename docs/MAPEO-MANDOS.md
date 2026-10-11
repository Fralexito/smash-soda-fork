# Mapeo de mandos por persona: arreglar el mando de un invitado

11 de octubre de 2026. Frente LINK. Estado: hecho, compila y probado en el simulador; **falta probarlo con un invitado real**.

## Para qué sirve
Muchos invitados juegan con mandos genéricos o mal mapeados. Desde **Mandos › MAPEO** se corrige el mando de cada persona sin que ella toque nada en su PC:
- **Botones cambiados** (A sale como B, X como Y…): se reasignan.
- **Stick que camina solo** (el jugador se mueve sin tocar nada): zona muerta.
- **Stick al revés** (arriba va abajo): invertir ese eje.
- **Sticks cambiados** (el izquierdo mueve la cámara): cambiar izquierdo por derecho.
- **Gatillos raros**: un gatillo puede ser un botón y un botón puede ser un gatillo.

Cada persona tiene **su propio mapeo**. Se guarda y se aplica **solo** cada vez que esa persona entra y toma un mando, también otro día.

## Paso a paso
1. Abre la sala y dale un **puesto** al invitado (Mandos › Puestos). Sin puesto no llega su mando.
2. Ve a **Mandos › MAPEO** y elige a la persona en la lista de la izquierda.
3. Mira **MANDO EN VIVO**:
   - **LLEGA (SU MANDO)**: lo que pulsa en su mando.
   - **SALE AL JUEGO**: lo que recibe PES después del mapeo.
4. Pulsa **EMPEZAR** en el **ASISTENTE PASO A PASO**. Te dice qué pedirle, por ejemplo: «Pide a Mirko: pulsa el botón que quieres que sea A».
   - Díselo por voz, o pulsa **AVISAR POR CHAT** y le llega por el chat.
   - Lo que pulse pasa a ser ese botón y el asistente avanza solo.
   - Si ese botón ya está bien, pulsa **SALTAR (ESTÁ BIEN)**.
   - Al final de los botones vienen los sticks: «empuja el stick IZQUIERDO hacia la DERECHA», luego hacia ARRIBA, y lo mismo con el DERECHO. Con eso detecta los sticks cambiados o al revés.
5. Revisa el **RESUMEN** (por ejemplo `A → B`, `B → A`, `LY invertido`) y pulsa **GUARDAR MAPEO**.
6. Si el jugador camina solo, en **STICKS › ZONA MUERTA** pídele que suelte el stick y pulsa **AJUSTAR SOLO**. También puedes mover la barra:
   - El punto es lo que llega de su stick.
   - Dentro del círculo, el juego no recibe nada.
7. Para arreglar un solo botón sin el asistente, usa **BOTONES UNO A UNO**: a la izquierda, lo que pulsa; a la derecha, lo que recibe el juego. «— nada» lo desactiva.
8. Para dejarlo como estaba: **RESTABLECER** (arriba, en MANDO EN VIVO).

## Qué no cambia
- El **modo competitivo** y el **bloqueo de botones** se aplican **después** del mapeo. Reasignar un botón a Start no se salta el bloqueo de Start durante el partido.
- Quien no tiene mapeo juega exactamente igual que antes: el motor no toca su mando.
- El teclado de los invitados sigue en Mandos › Teclado; el mapeo es solo para mandos.

## Cómo funciona por dentro
- **Motor:** `SmashSoda/phoenix/core/MapeoMandos.h/.cpp`, con la lógica pura (sin Windows). Cada botón y cada gatillo es una «fuente» con sus «salidas» (botones, LT o RT). Los ejes se procesan en este orden: 1) cambiar sticks, 2) invertir, 3) zona muerta radial con reescalado (sin saltos al salir de la zona).
- **Perfiles:** `SmashSoda/phoenix/core/PerfilesMapeo.h/.cpp`. Se guardan en `%APPDATA%\Trybuchet\Smash Soda\phoenix-mapeos.json` con escritura atómica. Si el archivo está dañado, se ignora y nadie tiene mapeo.
- **GamepadClient.cpp:** en cada mensaje de botón, eje o estado de un invitado:
  - guarda lo que llegó (`AGamepad::phoenixCrudo`);
  - si esa persona tiene perfil, lo aplica y luego los bloqueos;
  - si no tiene perfil, sigue el camino original.
  Cuesta unos 50 ns por mensaje.
- **Puente:** acciones `mapeo.ver`, `mapeo.guardar` y `mapeo.restablecer`; estado `mapeo{perfiles, vista}`. Ver `SmashSoda/phoenix/web/PROTOCOLO.md`.
- **Interfaz:**
  - `ui/js/vistas/mapeo.js`: la pestaña, que reutiliza el dibujo de `mandovivo.js`.
  - `ui/js/mapeo.js`: la misma lógica que el motor, en JS, para el asistente y el simulador.
- **Asistente:** el motor anota la última pulsación nueva de la persona observada (`ultima` y un contador `n`). El asistente avanza cuando cambia `n`, así no pierde pulsaciones aunque el estado llegue 5 veces por segundo.

## Probarlo sin el juego
- **Simulador** (`ui/` con `python -m http.server` y abrir en el navegador): Mandos › MAPEO.
  - Mirko tiene A y B cambiados, el stick izquierdo al revés en vertical y además camina solo.
  - En el asistente, **AVISAR POR CHAT** hace que Mirko «pulse» lo que se le pide con su mando defectuoso.
  - También puedes escribir en la consola: `simPulsar("a")`, `simPulsar("lt")`, `simPulsar("lx+")`.
- **Prueba de la lógica pura:** `pruebas-link/mapeo_test.cpp` (identidad, A↔B, botón→gatillo, gatillo→botón, invertir, cambiar sticks, zona muerta, JSON y velocidad).
  - Con MSVC: `cl /std:c++17 /EHsc /O2 /I SmashSoda /I external pruebas-link\mapeo_test.cpp SmashSoda\phoenix\core\MapeoMandos.cpp`.
  - En Linux: `bash pruebas-link/correr.sh`.

## Límites conocidos
- Si un stick manda el eje horizontal por el vertical, el asistente lo avisa pero no lo corrige.
- Solo llega el mando de quien tiene un puesto. El asistente necesita que la persona esté sentada.
- Falta probarlo con un mando genérico real.
