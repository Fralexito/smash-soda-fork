# Phoenix Link · Mandos: orden, puestos, equipos, lados, retraso, modo cedido y más de 8 mandos

Todo lo que hace Link con los mandos, del 11 al 21-oct-2026, desde que el puesto N de Link es el jugador N de PES hasta los puestos 9–16 (experimentales, 0.3.41–0.3.44).

**Estado al 21-oct:**
- **Funciona:** orden de mandos, tu propio mando, mapeo por persona, Equipo A/B, medidor de retraso, modo cedido sin PIN y hasta 16 puestos con ViGEm (tope seguro 16).
- **Falta:** que un mando 9+ mueva a su futbolista dentro de PES. Ese trabajo es del módulo Sider `phoenix_mandos_fase1.lua`, documentado en el libro (`PRUEBA_CANCHA\informes\LIBRO_PERSONAL\cap_60_j/k/l`).
- **Investigaciones que no llevaron a nada usable,** guardadas para no repetirlas: el cursor del mando, mandos ocultos y color, y los 32 datos personales.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 12 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Orden de mandos en PES 2021: que el puesto N de Link sea el jugador N de PES](#parte-1-orden-de-mandos-en-pes-2021-que-el-puesto-n-de-link-sea-el-jugador-n-de-pes) — origen `docs/ORDEN-MANDOS-PES.md` (c91adc0 2026-10-11)
- [Parte 2 · Tu mando: jugar con tu propio mando como un puesto más (anfitrión)](#parte-2-tu-mando-jugar-con-tu-propio-mando-como-un-puesto-mas-anfitrion) — origen `docs/TU-MANDO.md` (33764ad 2026-10-12)
- [Parte 3 · Mapeo de mandos por persona: arreglar el mando de un invitado](#parte-3-mapeo-de-mandos-por-persona-arreglar-el-mando-de-un-invitado) — origen `docs/MAPEO-MANDOS.md` (21fb76c 2026-10-11)
- [Parte 4 · Medidor de retraso del mando (Link 0.2.16)](#parte-4-medidor-de-retraso-del-mando-link-0216) — origen `docs/MEDIDOR-RETRASO.md` (8a23f71 2026-10-11)
- [Parte 5 · Investigación · ¿qué mando controla a qué futbolista? (goles por persona para el ranking)](#parte-5-investigacion-que-mando-controla-a-que-futbolista-goles-por-persona-para-el-ranking) — origen `docs/INVESTIGACION-CURSOR-MANDO.md` (dcaa765 2026-10-11)
- [Parte 6 · Equipo A / Equipo B: elegir qué puestos son de cada equipo](#parte-6-equipo-a-equipo-b-elegir-que-puestos-son-de-cada-equipo) — origen `docs/EQUIPOS-A-B.md` (e5a55e7 2026-10-12)
- [Parte 7 · Detección automática del lado (local/visita) de cada mando](#parte-7-deteccion-automatica-del-lado-localvisita-de-cada-mando) — origen `docs/DETECCION-LADOS.md` (e0b4733 2026-10-12)
- [Parte 8 · Mandos ocultos hasta que alguien los ocupe + color de cada mando (PES 2021)](#parte-8-mandos-ocultos-hasta-que-alguien-los-ocupe-color-de-cada-mando-pes-2021) — origen `docs/INVESTIGACION-MANDOS-OCULTOS-Y-COLOR.md` (9b6d71f 2026-10-12)
- [Parte 9 · Investigación: ¿se pueden ampliar los 32 «datos personales» de PES 2021?](#parte-9-investigacion-se-pueden-ampliar-los-32-datos-personales-de-pes-2021) — origen `docs/INVESTIGACION-DATOS-PERSONALES-32.md` (e676669 2026-10-12)
- [Parte 10 · Perfiles de amigos: más de 32 «Datos personales» en PES 2021](#parte-10-perfiles-de-amigos-mas-de-32-datos-personales-en-pes-2021) — origen `docs/PERFILES-DE-AMIGOS.md` (d8f523f 2026-10-12)
- [Parte 11 · Modo cedido: prestar tu PC a un amigo (solo Link y PES)](#parte-11-modo-cedido-prestar-tu-pc-a-un-amigo-solo-link-y-pes) — origen `docs/MODO-CEDIDO.md` (4e5c6e4 2026-10-16)
- [Parte 12 · Más de 8 mandos en PES 2021 — lo que hace Phoenix Link (0.3.41–0.3.42, EXPERIMENTAL)](#parte-12-mas-de-8-mandos-en-pes-2021-lo-que-hace-phoenix-link-03410342-experimental) — origen `docs/MAS-DE-8-MANDOS.md` (06a673d 2026-10-21)

---

## Parte 1 · Orden de mandos en PES 2021: que el puesto N de Link sea el jugador N de PES

> Origen: `docs/ORDEN-MANDOS-PES.md` · último cambio c91adc0 2026-10-11 · 5397 bytes. Copia íntegra.

10–11 de octubre de 2026. Frente LINK. Estado: **EXPERIMENTAL** (funciona a mano en la PC de Fralex; falta probar el modo automático).

---

### 1. El problema (de toda la comunidad PES + Parsec)

Con varios mandos (Parsec, ViGEmBus, DS4Windows, HidHide…) PES 2021 nunca asigna los mandos en el mismo orden:
- un amigo que en Link es el **puesto 2** acaba siendo el **jugador 5** en PES;
- a veces el mando del anfitrión deja de reconocerse o «se pulsa solo».

### 2. Qué descubrimos (causa exacta)

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

### 3. Prueba que confirmó la solución (PC de Fralex, 10 oct, ~23:50)

1. Con la sala de Link abierta, se leyó el GUID actual de cada puesto con una sonda de solo lectura.
2. Se escribió `settings.dat` con **casilla N = puesto N**, con el mapa de botones Xbox y copia de seguridad antes.
3. Se abrió PES y en la pantalla de elegir lados se probó Ctrl+Alt+1, 2, 3, 4… (el anfitrión maneja ese puesto con el teclado).
4. Resultado de Fralex: **«todo estaba en la posición correcta perfectamente»**.

### 4. La solución dentro de Phoenix Link (EXPERIMENTAL)

Está en **Mandos › Puestos › ORDEN DE MANDOS EN PES** (interruptor). Encendido:
- Link lee el GUID de cada puesto. El código está en `SmashSoda/phoenix/core/OrdenPes.*`.
- Con **PES y Settings cerrados**, escribe `settings.dat` con casilla N = puesto N:
  - antes guarda una copia `settings.dat.phoenix-<fecha>`;
  - escribe de forma atómica;
  - las casillas sin puesto quedan vacías.
- **Ya no desconecta sus mandos al cerrar la sala**, así Windows no los vuelve a barajar mientras Link esté abierto.
- Muestra las 8 casillas con ✓ y te avisa si hay que cerrar PES («El orden cambió y PES está abierto»).
- Marionetas arregladas: el maestro **nunca** es un mando de Link y se reconoce por su ID, no por su posición.

### 5. Guía paso a paso (cualquier PC)

1. Cierra **DS4Windows** (y Steam Input si lo usas para el mando).
2. Abre Phoenix Link, ve a **Mandos › Puestos**, baja hasta **ORDEN DE MANDOS EN PES** y **enciéndelo**.
3. Cierra PES y su Settings. En unos segundos la tarjeta debe decir **«Ordenado»**, con las 8 casillas en ✓.
4. Abre la sala y luego PES. El puesto 1 de Link es el jugador 1, el 2 el jugador 2…
5. **Para jugar con tu propio mando (anfitrión):** ve a **Mandos › TU MANDO**, elige tu mando y arrástrate a un puesto en **Mandos › Puestos**. PES no ve tu mando directamente, porque no está en ninguna casilla: juegas a través de Link. No hace falta HidHide. Ver `TU-MANDO.md`.
6. Si cambias algo con PES abierto (reabrir Link, enchufar o desenchufar mandos), la tarjeta avisa: cierra PES y Link vuelve a ordenar solo.

**Volver atrás:** apaga el interruptor y restaura la copia `settings.dat.phoenix-<fecha>` (o abre Settings.exe y configúralo a mano).

### 6. Límites conocidos

- PES lee `settings.dat` **solo al arrancar**: cualquier cambio de orden necesita reiniciar PES.
- Probado con mandos virtuales Xbox de Link y un DualShock 4 físico. Con un mando **Xbox físico** puede que PES lo lea por XInput aunque no esté en ninguna casilla; está sin probar.
- El mapa de botones de los puestos de Link es el mapa Xbox que ya tenía el archivo (o el de fábrica de Settings).

### 7. Herramientas de diagnóstico usadas

- `sonda.cpp`: lista los mandos DirectInput con su GUID y su origen (serie ViGEm o físico).
- `monitor.cpp`: muestra en vivo qué mando ocupa cada casilla de PES y qué está pulsando.
- Las dos están en el scratchpad de la sesión del 10 de octubre. La lógica de la sonda ya está dentro de `OrdenPes.cpp`.
- Prueba: `pruebas-link/ordenpes_test.cpp`. Con el `settings.dat` real de Fralex da el mismo archivo que funcionó en PES.

---

## Parte 2 · Tu mando: jugar con tu propio mando como un puesto más (anfitrión)

> Origen: `docs/TU-MANDO.md` · último cambio 33764ad 2026-10-12 · 5318 bytes. Copia íntegra.

11 de octubre de 2026. Frente LINK. Estado: hecho y probado en el simulador; **falta probarlo con un mando real**.

### Qué fallaba
- «Tomar mando» (Ctrl+Alt+número) **bloqueaba** el puesto y le mandaba el **teclado** unas 60 veces por segundo.
- Las Marionetas (maestro y títeres) copiaban tu mando físico **a ese mismo puesto** a la vez.
- Los dos se pisaban, y por eso:
  - el puesto salía «bloqueado»;
  - tu mando no movía nada;
  - solo podías cambiarte de puesto con Ctrl+Alt.
- La pantalla de maestro y títeres era confusa.
- Tú no aparecías en Puestos, así que no podías arrastrarte ni pasarte a «Mirando».

### Cómo queda
- **Mandos › TU MANDO**: eliges una vez el mando con el que juegas (o «Sin mando: teclado»).
- **Mandos › Puestos**:
  - Apareces como **TÚ**: en «Mirando» si no juegas, o en tu puesto si juegas.
  - Te **arrastras** a cualquier puesto libre, o eliges **PONERME EN…**. Tu mando mueve ese puesto, **sin bloqueo**.
  - Para mirar, te arrastras a **Mirando** o pulsas **QUITAR**.
  - **TODOS A ESPECTADOR** (arriba) pasa a todos a mirar. Sustituye al viejo «QUITAR TODOS», que desenchufaba los mandos.
- Por dentro: es una marioneta de **un solo puesto** (el tuyo), que cambia sola cuando te mueves.

### Paso a paso
1. Cierra DS4Windows.
2. Conecta tu mando y ve a **Mandos › TU MANDO**. Pulsa un botón: se enciende su luz. Pulsa **USAR ESTE**.
3. Si los botones salen cambiados, pulsa **¿BOTONES CAMBIADOS?** (cambia la lectura entre Xbox y PlayStation).
4. Abre la sala. En **Mandos › Puestos**, arrástrate (TÚ) al puesto que quieras.
5. Para que PES no lea también tu mando por su cuenta (y no manejes dos jugadores), enciende **ORDEN DE MANDOS EN PES** en esa misma pestaña. Ver `ORDEN-MANDOS-PES.md`.

### No abras Settings de PES para tu mando
Con el orden de mandos en PES, **tu mando no va en Settings**: PES solo ve los 8 mandos de Link y tu mando entra a través de tu puesto. Si Settings muestra mandos «XBOX», son los de Link (lo normal).

### Para sacar a alguien de un puesto
- Botón **QUITAR** (rojo) en su fila, o arrástralo a **Mirando**.
- Para sacarlo **de la sala**, ve a Gente.

### Sentarte solo (desde 0.2.17)
En **Mandos › TU MANDO › Mi puesto al jugar** hay dos interruptores (los dos encendidos por defecto):
- **Empezar sentado**: al abrir la sala ya estás en tu puesto (el que elegiste: fijo, 1.º de A o 1.º de B). Si no quieres jugar, arrástrate a **Mirando**.
- **Entrar pulsando tu mando**: si no estás en un puesto, pulsa **A, B, X o Y** (✕ ○ □ △) y entras. Si tu puesto está ocupado, vas a otro libre de tu equipo; si no, a cualquiera libre. **Nunca saca a nadie**: si todo está lleno, no pasa nada.

Link recuerda tu mando: al volver a abrirlo, el que elegiste con «USAR ESTE» queda elegido solo cuando está conectado. Con teclado no hay entrada automática: usa el botón **JUGAR** en Mirando.

Los invitados funcionan igual: un jugador autorizado (aceptó el reto en la web, lo aceptaste tú o ya lo habías sentado) pulsa A/B/X/Y y cae en un puesto libre. A quien mandaste a Mirando no entra solo: te sale como «quiere jugar».

### Salir como jugador en la web (desde 0.2.23, automático desde 0.2.24)
Antes, si jugabas tú con tu mando de Link, el partido que Link mandaba a la web **no te incluía**: contra un invitado parecía «invitado contra la CPU» y no puntuaba. Ahora es **automático**, sin tocar nada:
- **Sentado en un puesto:** al pitazo inicial Link te apunta en el lado de tu puesto (el equipo de tu puesto: Equipo A o Equipo B, según el mapa de Mandos › Puestos).
- **Sin puesto, pero jugando directo en PES:** si Link ve que tocaste tu mando en los 2 minutos antes del pitazo (cuando eliges equipos en los menús), te apunta en tu lado habitual, el de «Mi puesto al jugar» (puesto fijo, 1.º de A o 1.º de B).
- **Si solo miras y no tocas tu mando:** no sales.
- Interruptor «Contarme como jugador en la web (automático)» en **TU MANDO › Mi puesto al jugar**, encendido por defecto. Apágalo si nunca quieres salir sin puesto.
- Funciona al armarse los lados solos al pitazo. Si ya los elegiste a mano en Partido › Armar, ahí mandas tú.

### Cruzarte con un amigo (desde 0.2.31)
Si estás sentado (por ejemplo en el puesto 02) y arrastras tu fila a un puesto que ya ocupa un amigo (el 01), **os cruzáis**: tú pasas al 01 y él al 02. Nadie se queda sin mando ni va a Mirando. Link muestra el aviso «Te sentaste en el 01 y Fulano pasó al 02».
Si aún no estás sentado, sigue valiendo el aviso de siempre (el puesto está ocupado): sentarte en un puesto libre, y luego cruzarte.

### Ser siempre el mando 01 aunque juegues del otro lado (desde 0.2.33)
Los puestos ya no se reparten «los primeros N son Local»: **cada puesto tiene su equipo (Equipo A o Equipo B)** y tú lo eliges. Para ser siempre el **mando 01** aunque tu equipo sea el B:
1. **Mandos › TU MANDO › Mi puesto al jugar** → **PUESTO FIJO** y puesto **1**.
2. **Mandos › Puestos** → en la fila del 01, botón **→ B** (PASAR A EQUIPO B). El número 01 no cambia; solo cambia su equipo.
Al pitazo, Link te apunta en el lado de tu puesto (Equipo B) y a tu rival en el A. Detalle en `docs/EQUIPOS-A-B.md`.

---

## Parte 3 · Mapeo de mandos por persona: arreglar el mando de un invitado

> Origen: `docs/MAPEO-MANDOS.md` · último cambio 21fb76c 2026-10-11 · 5186 bytes. Copia íntegra.

11 de octubre de 2026. Frente LINK. Estado: hecho, compila y probado en el simulador; **falta probarlo con un invitado real**.

### Para qué sirve
Muchos invitados juegan con mandos genéricos o mal mapeados. Desde **Mandos › MAPEO** se corrige el mando de cada persona sin que ella toque nada en su PC:
- **Botones cambiados** (A sale como B, X como Y…): se reasignan.
- **Stick que camina solo** (el jugador se mueve sin tocar nada): zona muerta.
- **Stick al revés** (arriba va abajo): invertir ese eje.
- **Sticks cambiados** (el izquierdo mueve la cámara): cambiar izquierdo por derecho.
- **Gatillos raros**: un gatillo puede ser un botón y un botón puede ser un gatillo.

Cada persona tiene **su propio mapeo**. Se guarda y se aplica **solo** cada vez que esa persona entra y toma un mando, también otro día.

### Paso a paso
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

### Qué no cambia
- El **modo competitivo** y el **bloqueo de botones** se aplican **después** del mapeo. Reasignar un botón a Start no se salta el bloqueo de Start durante el partido.
- Quien no tiene mapeo juega exactamente igual que antes: el motor no toca su mando.
- El teclado de los invitados sigue en Mandos › Teclado; el mapeo es solo para mandos.

### Cómo funciona por dentro
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

### Probarlo sin el juego
- **Simulador** (`ui/` con `python -m http.server` y abrir en el navegador): Mandos › MAPEO.
  - Mirko tiene A y B cambiados, el stick izquierdo al revés en vertical y además camina solo.
  - En el asistente, **AVISAR POR CHAT** hace que Mirko «pulse» lo que se le pide con su mando defectuoso.
  - También puedes escribir en la consola: `simPulsar("a")`, `simPulsar("lt")`, `simPulsar("lx+")`.
- **Prueba de la lógica pura:** `pruebas-link/mapeo_test.cpp` (identidad, A↔B, botón→gatillo, gatillo→botón, invertir, cambiar sticks, zona muerta, JSON y velocidad).
  - Con MSVC: `cl /std:c++17 /EHsc /O2 /I SmashSoda /I external pruebas-link\mapeo_test.cpp SmashSoda\phoenix\core\MapeoMandos.cpp`.
  - En Linux: `bash pruebas-link/correr.sh`.

### Límites conocidos
- Si un stick manda el eje horizontal por el vertical, el asistente lo avisa pero no lo corrige.
- Solo llega el mando de quien tiene un puesto. El asistente necesita que la persona esté sentada.
- Falta probarlo con un mando genérico real.

---

## Parte 4 · Medidor de retraso del mando (Link 0.2.16)

> Origen: `docs/MEDIDOR-RETRASO.md` · último cambio 8a23f71 2026-10-11 · 4812 bytes. Copia íntegra.

**Para qué sirve:** cuando alguien dice «tengo lag» o «el mando me va tarde», saber **quién** va con retraso y **por qué** (su ping, su conexión que corta, el vídeo…), sin adivinar.

**Estado (11 oct 2026):** compila, pruebas automáticas OK (`pruebas-link/retraso_test.cpp`) y probado en el simulador. **Falta verlo con un invitado real** (paso 5).

---

### 1. Dónde se ve

- **Sala › Conexión:** cada invitado tiene su tarjeta de siempre (ping, gráfica) y debajo un bloque nuevo **RETRASO DEL MANDO**:
  - el **retraso estimado** en ms con su semáforo (VA BIEN / SE NOTA / CON RETRASO) y la causa principal;
  - **VÍDEO**: lo que tarda tu PC en codificar + la suya en decodificar;
  - **PÉRDIDA**: % de paquetes de vídeo que Parsec tuvo que reenviar (últimos ~10 s);
  - **JITTER MANDO**: cuánto varía el tiempo entre mensajes seguidos de su mando;
  - **SALTOS/MIN**: veces que su mando se quedó retenido y llegó todo de golpe;
  - abajo, en pequeño: cada cuántos ms llega su mando (media y p95) y cuántos mensajes por segundo.
  - Arriba, **¿Cómo se calcula?** lo explica todo dentro de Link.
- **Mandos › Puestos:** junto al ping de cada jugador sentado, un punto de color y «~74». Pasa el ratón por encima para ver el detalle.
- **Chat de la sala (solo durante un partido):** si alguien sigue **en rojo 10 s seguidos**, el bot avisa: «Aviso de retraso: Lucho va con retraso (~180 ms, ping alto)…». Como mucho una vez cada 3 min por jugador y una cada 30 s en total.

### 2. Cómo se calcula

**Retraso estimado = ping (ida y vuelta) + vídeo (codificar + decodificar) + jitter de su mando.**

| Semáforo | Retraso estimado | o pérdida | o saltos |
|---|---|---|---|
| Verde (va bien) | ≤ 80 ms | < 2 % | < 6 por minuto |
| Ámbar (se nota) | ≤ 130 ms | ≥ 2 % | ≥ 6 por minuto |
| Rojo (con retraso) | más de 130 ms | ≥ 5 % | ≥ 20 por minuto |

Manda la peor de las tres reglas.

**De dónde salen los datos del mando:** Link apunta la hora exacta a la que llega a tu PC cada mensaje del mando de cada invitado (antes de bloqueos y mapeos). Mientras lo mueve, los mensajes llegan a ritmo fijo (por ejemplo, cada 8 ms). Si su conexión va mal, ese ritmo se rompe:
- **Jitter:** los intervalos varían mucho entre sí.
- **Salto:** un hueco (≥ 40 ms y ≥ 3 veces lo normal) seguido de **varios mensajes de golpe**. Es la huella de la red reteniendo sus pulsaciones y soltándolas juntas. Una pausa normal del jugador **no** cuenta (después de una pausa llega un mensaje, no una ráfaga).
- Si no toca el mando, no hay datos del mando (sale «se medirá cuando lo mueva»).

**Coste:** se mide en el hilo de entrada con arrays fijos (ventana de los últimos 128 intervalos y saltos del último minuto), sin bloqueos ni memoria nueva por mensaje. La interfaz lee un resumen una vez por segundo.

### 3. Lo que NO se puede medir

El retraso **dentro de la casa del invitado**: su mando (sobre todo por Bluetooth), su PC y su pantalla o tele (con el «modo juego» apagado, +50 ms o más). Si todo le sale verde y aún nota lag, el problema suele estar ahí: que pruebe con cable y con el modo juego de la tele.

### 4. Qué hacer según la causa

| Causa | Qué suele ser | Qué probar |
|---|---|---|
| ping alto | Está lejos o su internet es lento | Cable en vez de WiFi; cerrar descargas |
| mando a saltos | WiFi saturado o cortes cortos | Cable o WiFi de 5 GHz; acercarse al router |
| pierde paquetes | Conexión inestable | Cable; en Sala › Opciones bajar los Mbps |
| vídeo lento | Su PC (o la tuya) tarda en (de)codificar | Que cierre programas; en su Parsec, decodificador por hardware |
| mando irregular | Su mando o su conexión envían a trompicones | Cable para el mando y para internet |

### 5. Prueba con un invitado real

1. Abre la sala y que entre un amigo con mando.
2. **Sala › Conexión:** que mueva el stick sin parar 5 s. Debe salir «Llegada del mando: cada X ms… mensajes/s» y el semáforo.
3. Si puede, que se pase al WiFi lejos del router (o que alguien en su casa descargue algo): deben subir el jitter o los saltos.
4. **Mandos › Puestos:** comprueba el punto y «~N» junto a su ping.
5. Para el aviso del chat: con un partido empezado, alguien en rojo 10 s seguidos.

### 6. Archivos

- `SmashSoda/phoenix/core/MedidorRetraso.*` — lógica pura (ventana deslizante, evaluación, pérdida, antispam).
- `SmashSoda/phoenix/host/RetrasoMando.*` — enganche con el hilo de entrada (`Hosting::pollInputs`).
- `SmashSoda/phoenix/web/RetrasoWeb.cpp` — 1 vez por segundo junta todo, estado `retraso` y aviso del chat.
- `SmashSoda/phoenix/web/ui/js/vistas/retraso.js` — bloque de Conexión, leyenda e indicador de Puestos.
- `pruebas-link/retraso_test.cpp` — pruebas.

---

## Parte 5 · Investigación · ¿qué mando controla a qué futbolista? (goles por persona para el ranking)

> Origen: `docs/INVESTIGACION-CURSOR-MANDO.md` · último cambio dcaa765 2026-10-11 · 8555 bytes. Copia íntegra.

Frente LINK · 11 oct 2026 · estado: **método listo y simulado; falta la prueba en la PC de Fralex**.

### 1. El problema

- En 1 contra 1 el goleador se sabe por el lado (`phoenix_estadio.lua` ya da cada gol con lado y minuto).
- En 2 contra 2 o 3 contra 3 hay varios humanos por lado. Para saber **quién** marcó hay que saber qué mando
  (jugador 1-8 de PES = casilla de `settings.dat` = puesto de Phoenix Link, comprobado el 11 oct) controlaba
  al futbolista que marcó. Y, si se puede, quién dio la asistencia.

### 2. Qué se sabe (11 oct)

| Fuente | Qué da | ¿Sirve? |
|---|---|---|
| Sider 7.3.3/7.4 `match.stats()` | marcador, penales, periodo, reloj | Solo el lado del gol. **No da goleador ni mando.** |
| Eventos de Sider (doc `scripting.html` de sider7) | `set_teams`, `set_match_settings`, `display_frame`, `key_down`, `gamepad_input`… | No hay evento de gol ni de «jugador controlado». `gamepad_input` solo llega **con el overlay abierto** y **de un solo mando** («only one will generate these input events»). |
| Archivos que carga el juego (`livecpk_data_ready`) | `goal\cut_data\goal_celebrate_NNNN…` al celebrar (lo usa el ejemplo `cr7_audio.lua`) | Dice **qué celebración** se carga, no qué jugador ni qué mando. Como mucho, una pista de cuándo se celebra. |
| Tabla de Cheat Engine de xAranaktu (MIT, archivada en 2023) | presupuestos de LM, ficha del jugador al pasar el cursor del menú, tiempo de partido, estamina | **No** trae cursor del partido, mandos, goleadores ni eventos. |
| Foros (evo-web, pesmodding, CE) | — | No se encontró ninguna dirección pública del «controlled player» ni del registro de goles de PES 2021. Hay que encontrarla nosotros. |
| Base de SYNC (`05-PUERTA-EN-VIVO.md`, `RIESGOS-SIDER.md`) | Técnica de búsqueda segura (`VirtualQuery` + `ReadProcessMemory`) | **Clave.** Leer la memoria del juego directamente ya cerró PES (0xC0000005 el 9 oct, por ReShade/DLSS). Siempre se copia con `ReadProcessMemory`. |
| Base de SYNC (guías 16 y 19) | Goleadores y asistencias de temporada en el **guardado de Liga Máster** | Solo LM y por jugador, no por mando. No vale para amistosos por Parsec. |

**Conclusión:** nadie lo tiene hecho. Es viable con ingeniería inversa **solo de lectura** (no puede desincronizar
ni cambiar el partido), pero hay que localizar el dato con pruebas en el juego.

### 3. Hipótesis

1. **Cursor:** cada futbolista en el campo tiene una estructura en memoria creada al empezar el partido. Para dibujar
   el indicador «1P/2P» sobre su cabeza, el juego necesita saber qué mando lo controla: lo más probable es un byte
   (casilla 0-7, o 0xFF = nadie) dentro de esa estructura, o una tabla por mando que apunta a ella.
2. **Gol:** el juego muestra el nombre del goleador en el marcador y en las estadísticas del partido, así que guarda
   un registro de cada gol con el ID del jugador (y quizá el del asistente y el minuto).
3. En PES el mando que tiene el balón controla al que lo lleva, así que en el momento del gol el mando que
   controlaba al goleador es el que marcó: «gol de la casilla N» = casilla cuyo cursor estaba en el goleador
   cuando subió el marcador (o justo antes del tiro).

### 4. Método (prueba `pruebas-link/sider/phoenix_cursor_prueba.lua`)

Se usa un futbolista conocido: **Lamine Yamal, ID 162114** (Barça), el mismo que usó SYNC. El ID se puede cambiar
con `content\phoenix\cursor\objetivo.txt`.

1. **Búsqueda «menu»** (menú principal): todas las direcciones donde está el ID (copias de la base, menús).
2. **Búsqueda «partido»** (ya jugando, en pausa): lo que aparece **solo** en el partido es el futbolista en el campo.
   Se quedan hasta 96 direcciones «candidatas».
3. **Fotos** de 4 KB alrededor de cada candidata en momentos marcados con la tecla J:
   A = Lamine con el teclado, N = sin nadie, B = con el mando físico (dos veces cada una).
   Además, cada 0,25 s se anotan los bytes que cambian poco (`cambios.txt`) y en cada gol se hacen dos fotos solas.
4. **Búsqueda «gol»** tras marcar con Lamine: las direcciones nuevas con su ID son el candidato a registro del gol
   (`tras_gol.txt`; puede traer también minuto y asistente).
5. En el PC de desarrollo: `py pruebas-link/sider/analizar_cursor.py <carpeta>`. Un byte es **candidato fuerte** si vale
   igual en A1 y A2, igual en B1 y B2, igual en N1 y N2, y los tres valores son distintos.

**Seguridad:** solo lectura; nunca lee la memoria directamente (copia con `ReadProcessMemory`, que falla sin cerrar
el juego); sin `VirtualProtect` ni llamadas al juego; errores con «bandera» (5 seguidos → se apaga);
teclas J e I, que ningún módulo de ConmeGOL usa; escribe solo en `content\phoenix\cursor\`.
Mientras busca copia ~4 MB por cuadro: puede haber tirones unos segundos (por eso se hace en pausa).

**Simulación** (`pruebas-link/sider/simular_cursor.py`, LuaJIT de lupa + memoria real del proceso, imitando Sider):
recorre los 10 pasos, encuentra el byte plantado (+0x40: teclado 0x00 / mando 0x01 / nadie 0xFF), detecta el gol,
hace las fotos solas y encuentra el registro nuevo del gol. Las copias que la propia prueba deja en memoria
salen marcadas como «posible copia propia».

### 5. Qué sigue según el resultado

| Resultado | Siguiente paso |
|---|---|
| Hay candidato fuerte (byte con la casilla) | v0.2: leer ese byte en los 22 futbolistas (las estructuras deberían estar seguidas o en una tabla). Confirmar con 3 mandos. |
| Solo «con cursor sí/no» | Buscar al lado una tabla por mando: v0.2 busca punteros (u64) a la estructura del futbolista y vigila cuáles cambian al cambiar de jugador. |
| Nada en las candidatas | El cursor vive en otra estructura: v0.2 busca punteros a las candidatas, o sesión guiada con Cheat Engine («qué accede a esta dirección»), solo con permiso de Fralex. |
| Aparece registro del gol | Leer de ahí goleador (y asistente si está). Para encontrarlo en cada partido sin escanear toda la memoria, buscar una firma estable (AOB) o la estructura padre. |

**Integración final (cuando todo esté probado):** `phoenix_estadio.lua` añadiría a cada gol de `estado.json` /
`resultado.json` la casilla (`"c":N`, y `"a":N` del asistente). Link traduce casilla → persona (ya conoce los puestos)
y lo sube con el resultado. Cambia el contrato con la web: se pedirá por prompt al chat WEB. Mientras tanto,
alternativa sin memoria: que el árbitro o los capitanes asignen los goleadores en la web.

### 6. Guía para Fralex (solo cuando dé permiso)

**Antes:** PES cerrado. Partido offline contra la CPU, nunca con amigos.

1. **Copia de seguridad:** en `D:\Frank\Games_\Conmegol Patch\SiderAddons\` copia `sider.ini` como
   `sider.ini.antes-cursor`. Haz lo mismo en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\` (la que copia el switcher).
2. **Instalar:** copia `phoenix_cursor_prueba.lua` a las dos carpetas `SiderAddons\modules\` y, en los dos `sider.ini`,
   añade al final de las líneas `lua.module`: `lua.module = "phoenix_cursor_prueba.lua"`
3. **Jugar:** abre PES. En el menú principal: Espacio (overlay) → tecla 1 hasta «PRUEBA CURSOR» → J.
   Espera a que diga «lista». Cierra el overlay.
4. Amistoso **Barça contra la CPU**, con el **teclado y el mando físico los dos en el Barça**. Que juegue Lamine.
5. Ya jugando: pausa → overlay → J (búsqueda «partido»). Espera a «lista».
6. Sigue lo que dice el overlay (paso 3 a 8): pon la jugada que pide, pausa, overlay, J. Si te equivocas, I vuelve un paso.
7. Mete un gol **con Lamine**; cuando vuelva el saque, pausa → overlay → J (búsqueda «gol»).
8. Sal y cierra PES. Pásale a Claude la carpeta `SiderAddons\content\phoenix\cursor` y el `sider.log`.

**Volver atrás:** con PES cerrado, restaura los dos `sider.ini.antes-cursor` (o quita la línea) y borra
`phoenix_cursor_prueba.lua` de las dos carpetas `modules`. La carpeta `content\phoenix\cursor` se puede borrar.

### 7. Resultados

_(pendiente de la prueba con Fralex)_

### Fuentes

- Sider 7: <https://github.com/pes-modding/sider7> (`doc/scripting.html`). Se lee para aprender; no se copia código.
- Tabla de xAranaktu (MIT): <https://github.com/xAranaktu/PES-2021-Cheat-Table>
- Base de SYNC (rama `mercado-fase0`): `05-PUERTA-EN-VIVO.md`, `06-RUTA-DEL-DESCUBRIMIENTO.md`, `PhoenixSync/sider/RIESGOS-SIDER.md`,
  `PhoenixSync/sider/phoenix.lua` (técnica de copia segura) y `investigacion/herramientas-y-formatos.md` §9.

---

## Parte 6 · Equipo A / Equipo B: elegir qué puestos son de cada equipo

> Origen: `docs/EQUIPOS-A-B.md` · último cambio e5a55e7 2026-10-12 · 6378 bytes. Copia íntegra.

12 de octubre de 2026. Frente LINK, Link 0.2.33. Estado: hecho, compilado y probado en el simulador y con pruebas de lógica pura; **falta probarlo con una sala real**.

### Qué pidió Fralex
«Corrige y da la opción de escoger el espacio de los mandos, y ahí en vez de decir Local / Visita que sea Equipo A / Equipo B.» Él quiere ser **siempre el mando 01** aunque juegue del lado de visita.

### Qué fallaba
- Link suponía que **los primeros N puestos eran Local** y el resto Visita, por número de puesto. El puesto 01 salía siempre como Local, aunque en PES jugaras de visita.
- Los lados reales los elige el juego, no el número de puesto.

### Cómo queda
- **Textos:** Local → **EQUIPO A**, Visita / Visitante → **EQUIPO B** en Puestos, Partido, selectores de mando, tutorial y ayudas.
- **Qué son A y B:** equipos **de Link**. No tienen por qué coincidir con local y visita del juego. A es el que estaba a la izquierda en el saque (según el Campo 2D).
- **Mapa por puesto:** `equipoPuesto` en `phoenix-ui.json`, ocho letras, una por puesto: `A`, `B` o `-` (fuera). Ejemplos: `AABB----` (el de siempre, 2 contra 2) y `BA------` (el 01 es del B y el 02 del A).
- **El número del puesto no cambia.** Es el mando virtual de Link: con «Orden de mandos en PES», el puesto N es el jugador N. No se renumera ningún mando.
- **Migración:** un `phoenix-ui.json` viejo (sin `equipoPuesto`) se convierte solo: los primeros `equipoLocal` puestos son A, hasta `mandosActivos` son B, el resto queda fuera. «Mi puesto al jugar» con `local` / `visitante` se lee como `a` / `b`.

### Cómo se usa (Mandos › Puestos)
- Cada fila trae **→ B** o **→ A** (PASAR A EQUIPO B / A): pasa ese puesto al otro equipo, con su número.
- **+ AÑADIR PUESTO** mete el primer puesto sin usar en ese equipo. **− QUITAR UNO** saca el último puesto libre de ese equipo.
- **PUESTOS SIN USAR**: lista los puestos fuera con botones **A** y **B** para elegir cuál añadir y a qué equipo.
- Cada equipo conserva al menos un puesto (no deja a un equipo vacío).
- **Mandos › TU MANDO › Mi puesto al jugar:** PUESTO FIJO, **1.º DE A** o **1.º DE B**.

### Caso clave: ser el 01 jugando del Equipo B
1. TU MANDO › Mi puesto al jugar → PUESTO FIJO 1.
2. Puestos → en el 01, **→ B**; en el 02, **→ A** si hace falta.
3. Resultado: tú en el 01 siendo del Equipo B, tu rival en el 02 siendo del A. Al pitazo Link arma `a` con los puestos del A y `b` con los del B, y te apunta a ti en el lado de tu puesto.

### Quién usa el mapa
`PhoenixPrefs` (`equipoDe`, `sincronizarEquipos`) · `EstadoWeb` (`equipo` de cada puesto: `a` / `b` / `fuera`; `formacion` `{a, b, mapa}`) · `Arbitro` (armado de lados al pitazo y apunte del anfitrión; HUD de la sala) · `MandoHost` (puesto libre) · `AccionesSala` (!equipo, jugador en espera) · `AccionesMandosWeb` (`mandos.equipo`, `mandos.miPuesto`, `mandos.cantidad`) · `AccionesSalaWeb` / `AccionesPartidoWeb` (formación y reparto de mandos de `partido.preparar`) · `TableroMandos` (tablero clásico) · web: `puestos.js`, `jugar.js`, `partido.js`, `sala.js`, `gente.js`, `tutorial.js`, `simulador.js`.

### Contrato con la web: no cambia
`partida_fin` sigue con los lados `a` / `b` y el booleano `local`; `mandos.formacion{local,visitante}` y los perfiles de sala conservan `local` / `visitante` (ahora cuentan los puestos del A y del B). Solo cambian textos visibles. Lo único nuevo: la acción `mandos.equipo{indice, equipo: a|b|fuera}` y el campo `formacion.mapa`.

### Pruebas
- `pruebas-link/equipos_test.cpp` (lógica pura: migración, mover puestos, caso 01 en el B, reglas, recortes): **todo bien**.
- Simulador (`?escenario=cruzado`): el 01 es del Equipo B (tú) y el 02 del A; mover puestos, añadir y quitar sin errores en la consola. `?escenario=llena` sin errores.
- Compilado con `cmake --build` (Release), sin errores.

### Cambios de 0.2.34: más intuitivo y candado de puesto
- **Arrastrar el puesto entre equipos:** se toma la tarjeta por su **número** y se suelta en el otro equipo (se ilumina «Suelta aquí…»). El botón «→ A» / «→ B» sigue como alternativa.
- **Puestos sin usar:** una bandeja con solo los números; se arrastran al equipo que se quiera.
- **Ayuda plegable:** «¿QUÉ SON A Y B?» en la cabecera de JUGANDO.
- **Candado de puesto (solo a quien tú fijes):** el icono de chincheta en la fila de un jugador **fija su puesto**. Cuando vuelva a la sala y tú lo aceptes como jugador (JUEGA), o él pulse su mando, cae en ese puesto; **si está ocupado, en el siguiente libre** (dando la vuelta, solo puestos de un equipo). Quien no tiene candado entra como siempre. Se guarda en `phoenix-ui.json` (`puestosFijados`, id de Parsec → puesto) y sobrevive a cerrar Link. En «Mirando» se ve «PUESTO 0N» junto a quien lo tiene fijado.
- Detalle técnico: acción `mandos.fijarPuesto{parsecId, puesto 0–8}` (0 = soltar), estado `mandos.fijados`; el gancho al pulsar el mando está en `GamepadClient::tryAssignGamepad` y la aceptación del host en `AccionesSala::decidirEspera`. Con turnos (hotseat) manda su propia lógica.

### Ajuste de 0.2.35 (a pedido de Fralex)
Se vuelve al aspecto de siempre de Puestos: la fila con su barrita de arrastre, sin bandeja de «puestos sin usar» ni recuadros en el número. **Se arrastra la fila entera a la columna del otro equipo** (se ilumina «Suelta aquí: el puesto 0N pasa a EQUIPO X») y el puesto se va **con su mando y su número**. Soltar sobre otra fila sigue intercambiando personas, como antes. Se quitaron los botones «→ A / → B». «+ AÑADIR PUESTO» mete el primer puesto libre en ese equipo.

### Ajuste de 0.2.36
- **Cruzar dos puestos:** arrastra la fila de un puesto **encima de una fila del otro equipo** y los dos puestos intercambian de equipo (el 5 del A y el 6 del B pasan a B y A), cada uno con su mando y su número. Si uno de los dos es **tu** puesto, se mantiene lo de antes: os cruzáis de asiento tú y tu amigo.
- **Botoncito ⇄ (muy sutil)** en cada fila: pasa ese puesto al otro equipo con un clic (se aclara al pasar el ratón; apagado si es el único puesto de su equipo).
- Acción nueva `mandos.equipoIntercambiar{a,b}` (permitida en modo cedido). La chincheta (candado de puesto) sigue igual y ahora se explica en «¿QUÉ SON A Y B?».

---

## Parte 7 · Detección automática del lado (local/visita) de cada mando

> Origen: `docs/DETECCION-LADOS.md` · último cambio e0b4733 2026-10-12 · 12474 bytes. Copia íntegra.

Frente LINK · 11 oct 2026 · estado: **integrado en Link 0.2.25 (interruptor «LADOS AUTO» en Partido, encendido por defecto);
probado en simulación; falta la prueba con PES y mandos reales** (§4). Ante la duda no toca nada y queda el botón CORREGIR LADOS.

### 1. El problema

Al pitazo inicial Link arma los lados solo según la formación de mandos (`origenLados = "mandos"`: los primeros
`equipoLocal` mandos son el local). Pero en la pantalla de mandos del juego cada uno puede mover su icono al otro lado,
y entonces Link se equivoca hasta que alguien pulsa CORREGIR LADOS. El ranking 1v1 deja «por revisar» esos partidos.

Link no puede leer en qué lado puso cada uno su mando dentro del juego (no se conoce esa dirección de memoria),
pero sí ve dos cosas en vivo:

- el **stick izquierdo de cada mando** (`ProveedorSala::asientos()` → `AsientoVista.ejeLX/ejeLY`, lo que recibe el juego);
- la **posición de cada futbolista** (Campo 2D, solo lectura de memoria, ~20 lecturas/s; equipo A/B fijado en el saque de centro).

### 2. Método

#### 2.1 Mando → equipo A/B (stick contra velocidad)

El futbolista que maneja un mando corre hacia donde apunta su stick, con un pequeño retraso.
Por cada mando y cada fichita se calcula, cuadro a cuadro, el coseno entre la dirección del stick de hace 0,25 s y la
velocidad de la fichita (solo si el stick está apretado ≥ 70 % y la fichita corre ≥ 1,5 m/s), con memoria de ~1 s.
En cada cuadro la fichita con la correlación sostenida más alta (≥ 0,6) **vota** por su equipo. El controlado cambia
a menudo, pero siempre dentro del mismo equipo; un rival que persigue al controlado puede correlacionar a ratos, pero
llega tarde y pierde la votación.

- **Cámara:** no se supone. Se prueban las 4 orientaciones posibles (signo de x y de z) y se queda la que da más votos
  sumando todos los mandos; tiene que ganar por 1,5× a la segunda.
- **Decide** cuando un mando tiene ≥ 200 votos (≈ 10-20 s de juego), el ganador lleva ≥ 80 % y ha votado en ≥ 35 % de
  los cuadros con stick apretado (un mando que no maneja a nadie apenas vota).
- **Seguimiento:** las fichitas no traen identidad; se unen con la más cercana del cuadro anterior (mismo equipo).

Código: `SmashSoda/phoenix/core/DeteccionLados.{h,cpp}` (`lados::Detector`), lógica pura.

#### 2.2 Equipo A/B → local/visita (el ancla)

A/B del Campo = «izquierda/derecha en el primer saque de centro que vio Link», no local/visita. Dos anclas:

1. **Primer gol (segura, no supone nada del juego).** `lados::LadoDefendido` sabe qué portería defiende cada equipo
   (el que casi siempre tiene a alguien en su área chica = su portero; memoria de ~20 s, se adapta solo al descanso).
   Cuando el balón entra en una portería (`porteriaDelGol`), marcó el equipo que **no** la defiende (vale en propia
   puerta). `estado.json` dice si ese gol fue del local o de la visita → A/B queda atado a local/visita.
   Pega: hace falta un gol (un 0-0 no se resuelve).
2. **Saque inicial (hipótesis, sin confirmar).** En el único saque inicial capturado (Barça–City, `CAMPO-2D-INVESTIGACION.md` §7)
   el **local estaba a la izquierda (x < 0)**. Si PES siempre pone al local a la izquierda en el primer tiempo, basta con
   saber qué equipo defiende x < 0 con el reloj en el primer tiempo. Hay que comprobarlo en varios partidos (con el local
   elegido de ambos lados de la pantalla de mandos) antes de fiarse.

### 3. Resultados

#### 3.1 Simulación (`pruebas-link/lados_test.cpp`)

Partido simulado de 60 s: 10 + 10 jugadores de campo con IA aleatoria, dos porteros, un mando por equipo que maneja a
un futbolista (cambia cada 4 s) con 0,25 s de retraso, **un rival que copia la carrera del controlado 0,5 s después**
(el peor caso), un mando quieto y un mando que se mueve sin manejar a nadie. 20 semillas × 4 orientaciones de cámara:

| | Resultado |
|---|---|
| Mandos que manejan a alguien | **40/40 decididos, 40 aciertos, 0 errores** (votos típicos 600/20) |
| Orientación de cámara | acertada en 20/20 |
| Mando quieto | nunca decide |
| Mando que se mueve sin manejar a nadie | nunca decide (votos 50-200 repartidos) |
| Ancla «portería que defiende cada equipo» y quién marcó | correcta en 20/20 |

Se compila con MSVC o g++ (está en `pruebas-link/correr.sh`).

#### 3.2 Con PES y mandos reales

_(pendiente: hace falta que Fralex juegue; ver §4)_. Lo que la simulación NO puede asegurar y la prueba debe mirar:

- que la velocidad que se lee del Campo siga al stick (retraso real, aceleración, regates, carreras con R2);
- cuántos votos se consiguen en los primeros 20-60 s reales (saques, pausas, repeticiones);
- si la orientación de cámara sale clara con la cámara que usa la liga;
- la hipótesis del saque inicial (§2.2-2).

### 4. Prueba con el juego (cuando Fralex dé permiso)

Herramienta aparte, **no toca Link ni el juego**: `pruebas-link/lados_real.cpp` (compilada en
`C:\Temp\claude\lados\lados_real.exe`). Lee el Campo con el mismo lector de Link (solo lectura, solo con el
`PES2021.exe` conocido) y los mandos XInput 0-3 del PC (los físicos y los virtuales de Link salen igual). Cada 2 s
escribe el veredicto por mando y, si ve un gol, qué equipo marcó; graba todo en `lados_AAAAMMDD_HHMMSS.csv` para
afinar sin repetir el partido.

1. Abre PES y Link como siempre. En una consola: `C:\Temp\claude\lados\lados_real.exe 300` (5 minutos).
2. Amistoso con **dos mandos, uno en cada lado**. Apunta quién es local en la pantalla de mandos y qué mando es cuál.
   Mejor que el partido empiece con la herramienta ya abierta (necesita ver el saque de centro).
3. Juega normal 1-2 minutos y mete un gol si puedes. Mira en la consola: «mando XInput N: equipo A/B … DECIDIDO»,
   «A defiende izquierda/derecha» y «GOL … marcó el equipo A/B».
4. Repite cambiando los lados en la pantalla de mandos (el local con el otro mando) y, si se puede, un 2 contra 2.
5. Pásale a Claude la salida de la consola, el CSV y lo que apuntaste (quién era local con qué mando).

**Volver atrás:** no hay nada que deshacer (no instala nada; cerrar la consola la para).

### 5. Cómo está integrado en Link (0.2.25)

Por petición de Fralex («si juego de local y luego de visitante, que lo detecte y me ponga en la posición adecuada solo»)
se conectó ya, con guardas. Archivos: `web/LadosAuto.{h,cpp}` (nuevo), ganchos en `web/Arbitro.cpp` (2a2),
`web/InterfazWeb.cpp` (enciende el lector y alimenta el detector), `web/AccionesMandosWeb.cpp`, acción
`partido.ladosAuto` y estado `arbitro.ladosAuto/ladosEstado`, `PhoenixPrefs.ladosAuto` (encendido por defecto),
`ProveedorSala::ejesMandoPropio()` (stick del mando físico del anfitrión), interruptor «LADOS AUTO» en Partido.

1. **Cuándo lee el juego:** con la sala abierta, desde que `pantalla.json` dice «saliendo» (antes del pitazo, para
   ver el saque inicial y repartir equipos) y hasta 5 min después del pitazo o hasta resolver. Fuera de eso, nada.
2. **Mandos que mira:** los 8 puestos (`asientos(8)`: invitados, y el anfitrión sentado con su mando de Link) y el
   mando físico del anfitrión (`ejesMandoPropio`, cuando juega directo en PES sin puesto). Persona: dueño del puesto,
   el anfitrión si es el puesto que maneja (`MandoHost`) o su mando físico.
3. **Ancla:** el gol (seguro) o, si no hay, el 1.er tiempo: el equipo que defiende x < 0 es el local (hipótesis del
   saque, §2.2-2). Si las dos se contradicen, no hace nada.
4. **Decisión** (`lados::decidir`, probado): solo con veredictos firmes de jugadores que están en los lados de Link.
   Si TODOS están al revés → `Partido::corregirLados()` (no mueve mandos), `origenLados("juego")`, relee el marcador
   del juego y avisa una vez en el chat: «Link vio en el juego los lados: X es el local y Y la visita (corregido
   solo).». Si todos coinciden → `origenLados("juego")` sin aviso. Mezcla, sin ancla o sin veredicto → nada.
5. **No actúa** si alguien ya pulsó CORREGIR LADOS (`correccionesLados > 0`), con el interruptor apagado o pasados 5 min.
6. Para la web: `lados.origen = "juego"` ya está previsto en el ranking y vale como `host` (`PROMPT-WEB-ranking-1v1.md`).

Coste: el del lector del Campo (escaneo inicial de ~12 s a prioridad baja y ~20 lecturas/s de 16 bytes por jugador)
durante esos minutos. Si en la prueba se notan tirones, apagar «LADOS AUTO».

**Lo que falta probar con el juego real** (pedido a Fralex): un partido contra la CPU de LOCAL y otro de VISITANTE,
mirando que Link deja (o corrige a) los lados correctos y el aviso del chat. Eso valida a la vez el detector, la
hipótesis del saque y el aviso. Si falla la hipótesis del saque, se quita esa ancla y queda solo la del gol.

### 6. Partido › Armar sin clics (0.2.42)

Con «LADOS AUTO» y el marcador automático encendidos, **Armar ya no pide elegir A/B**: muestra «Los lados se arman
solos al empezar el partido» y, en vivo, cómo quedarán los dos equipos (`partido.previa` del estado, calculada con
`armarLadosSegunMandos`, la MISMA función que usa el árbitro al pitazo: equipo de cada puesto + anfitrión sentado o
que tocó su mando; lado vacío = CPU). Al pitazo se arman solos (`origenLados = "mandos"`) y en el primer minuto
LadosAuto los comprueba en el juego y los corrige si hace falta (§5). El armado manual sigue disponible plegado
(«ARMAR A MANO», con «VOLVER A AUTOMÁTICO»); el contrato con la web no cambia (lados «a»/«b»).

**Antes del pitazo todavía no se lee el lado del juego.** La sesión «Leer el lado de cada mando en la pantalla de PES»
(`tools/radar/ladomando.cpp`) encontró 3 posiciones de memoria que valen 0/1/2 con el mando a izquierda/centro/derecha
en la pantalla de mandos, pero falta confirmarlas (dos mandos, varios arranques, firma estable, casilla ↔ puesto).
Cuando estén confirmadas, la vista previa de Armar usará ese lado directamente (local = izquierda) y la corrección
del primer minuto quedará solo como comprobación.

### 7. Equipo dinámico según la pantalla de mandos de PES (0.2.43, preparado; falta el lector)

Pedido de Fralex: «si en el juego me pongo a la izquierda, en Link Equipo A; a la derecha, Equipo B; en el medio, Mirando».

**Ya está en Link (sin efecto hasta que haya lector confirmado):**
- `core/LadoPantalla.{h,cpp}` (lógica pura, `pruebas-link/ladopantalla_test.cpp`): `Lectura` = lado por casilla PES 1-8
  (−1 izquierda, 0 centro, +1 derecha, 2 sin mando). `Estabilizador`: un lado cuenta solo si se lee igual **1,5 s seguidos**
  (sin parpadeos); lecturas no válidas o con valores raros no cuentan; sin lecturas 3 s → todo «desconocido».
  Enganche: `ladopantalla::fijarProveedor(fn)`; **sin proveedor no cambia nada** (hoy no hay ninguno).
- `web/LadosAuto.cpp`: lee 2/s con el proveedor (solo con LADOS AUTO) y recuerda el último lado estable 5 min, porque
  al pitazo la pantalla de mandos ya no está. `ladoPantallaPuesto(i)` → 'A' / 'B' / '-' / '?'.
- `armarLadosSegunMandos` (Arbitro.cpp): el equipo de cada puesto ocupado (y del anfitrión sentado con su mando de Link)
  sale de la pantalla si se conoce; si no, del puesto (Mandos › Puestos). Si TODOS salen de la pantalla, el partido arranca
  con `origenLados = "juego"`. Lo usan Armar (vista previa) y el árbitro al pitazo.
- **Mirando** (interruptor «Mirando automático», encendido, solo visible cuando llegan lecturas): mando en el centro = no juega
  este partido (sale en una tarjeta MIRANDO de Armar), **pero conserva su puesto y su mando**. Decisión a propósito: si se le
  quitara el puesto, su mando virtual desaparecería de PES y no podría volver a un lado moviendo el icono. Nadie se mueve de
  puesto ni se expulsa; no se pisa lo que el anfitrión sentó a mano.
- No cubierto: el anfitrión que juega **directo en PES con su mando físico** (sin puesto): su casilla en PES no se sabe;
  sigue con «Mi puesto al jugar» y la comprobación por stick (§5).

**Falta (sesión «Leer el lado de cada mando en la pantalla de PES», `tools/radar/ladomando.cpp`):** confirmar las 3 posiciones
de memoria que valen 0/1/2 con el mando izquierda/centro/derecha (otro arranque, dos mandos, firma estable, qué casilla es cada
una) y escribir el proveedor (solo lectura con ReadProcessMemory, mismo cuidado que el Campo 2D, solo con el exe conocido).
Ojo al traducir: esa lectura da 0/1/2 → hay que pasarlo a −1/0/+1.

---

## Parte 8 · Mandos ocultos hasta que alguien los ocupe + color de cada mando (PES 2021)

> Origen: `docs/INVESTIGACION-MANDOS-OCULTOS-Y-COLOR.md` · último cambio 9b6d71f 2026-10-12 · 9077 bytes. Copia íntegra.

Frente LINK · 12 oct 2026 · **INVESTIGACIÓN. No se cambió código de Link ni el juego de Fralex.**
Etiquetas: **[EVIDENCIA]** = visto en archivos/código/pruebas previas o fuente citada. **[SUPOSICIÓN]** = razonado, sin probar.

### Resumen corto

| Pregunta | Respuesta |
|---|---|
| 1. ¿Mandos listos por dentro pero invisibles para PES hasta que alguien los ocupe? | **Parcialmente.** No hay forma segura de mostrarlos/ocultarlos *con PES abierto*. Sí hay una vía ya existente y segura: dejar vacías las casillas de `settings.dat` de los puestos que no se van a usar *antes de abrir PES*. |
| 2. ¿Color de cada mando (mando 1 rojo, mando 2 azul) sin importar el puesto? | **No hay vía segura.** Lo que sí se logra es elegir *qué número de jugador* es cada persona (mando N de Link = jugador N). El color en sí lo decide el motor de PES. |

Recomendación: **no implementar nada**. Primero dos pruebas de observación con Fralex (§4). Regla de Fralex: «si algo se rompe, mejor no lo hagas».

### 1. Pregunta 1 — mandos «listos pero ocultos»

#### 1a. Cómo muestra PES los mandos que nadie toca
- **[EVIDENCIA]** PES solo hace caso a los pads que están en alguna de las 8 casillas de `settings.dat`; el resto los ignora (`docs/ORDEN-MANDOS-PES.md` §2). Lo lee **solo al arrancar** (§6).
- **[EVIDENCIA]** Link ya deja la casilla vacía cuando el puesto está desenchufado (`core/OrdenPes.cpp:163`).
- **No encontré documentación pública** sobre si PES 2021 muestra todos los pads conectados o solo los que pulsan, ni sobre el límite, ni sobre hot-plug (búsqueda en Steam, pesmodding, foros: solo hilos genéricos de «qué mando funciona»). Hilo más cercano: <https://steamcommunity.com/app/1259970/discussions/0/2967267880874790922> (sin datos de multi-mando).
- **[SUPOSICIÓN]** El estilo clásico de la serie: cada mando se asigna a un lado pulsando un botón. No confirmado. **Hace falta observarlo (prueba A, §4).**

#### 1b. Opciones evaluadas

| Opción | Veredicto | Motivo |
|---|---|---|
| **Vaciar casillas de `settings.dat` de los puestos no usados** (PES cerrado) | **Viable, ya existe.** Pad enchufado, GUID estable, PES lo ignora. | Granularidad por *sesión*, no en vivo: cambiar K exige reiniciar PES. Los puestos que sí se usan conservan N = jugador N. Riesgo bajo (Link ya hace copia de seguridad y escribe atómico). |
| HidHide para ocultar/mostrar pads virtuales | **No merece la pena.** | Driver extra + admin. **[SUPOSICIÓN]** PES enumera al arrancar; mostrar un pad con PES abierto probablemente no lo recoja. Mal encaje con pads ViGEm propios. |
| Desactivar/activar el dispositivo (cfgmgr32) | **No recomendado.** | Requiere admin; si Link se cierra mal, quedan pads deshabilitados. **[SUPOSICIÓN]** deshabilitar no cambia el GUID, pero reenumerar con PES abierto sigue sin garantía. Sin evidencia a favor. |
| ViGEm plug/unplug con el mismo serial | **No.** | **[EVIDENCIA]** (10 oct) al reconectar Windows reparte GUID de nuevo y rompió el orden; había 100+ entradas fantasma. Es justo lo que Link evita. |
| Desde Sider/Lua | **No.** | **[EVIDENCIA]** Sider solo da `gamepad_input` con overlay abierto y de un solo mando; no controla qué dispositivos ve DirectInput (`INVESTIGACION-CURSOR-MANDO.md` §2). |
| Casillas reservadas para pads que llegan después | Ya es el diseño actual (casilla N = puesto N, 8 siempre). | Lo que falta es solo que PES los *muestre* solo al pulsar: depende de la prueba A. |

#### 1c. Riesgos
Admin (HidHide, cfgmgr32), estabilidad (pads deshabilitados al cerrarse mal Link), GUID cambiantes (cualquier plug/unplug). La vía de casillas vacías no añade ninguno.

### 2. Pregunta 2 — color por mando

#### 2a. De dónde sale el color
- **No se encontró ninguna fuente pública** que diga si es por número de mando, por lado (local/visita) o por orden de pulsación. **[SUPOSICIÓN]** lo decide el motor según el lado/asignación, no una tabla modificable por texturas.
- **[EVIDENCIA]** `UIColors.lua` (Zlac, 2024) solo cambia **colores de texto de la interfaz**: 80 índices en memoria (`map_exe.txt`) y offsets de colores en archivos `.bin` de menú (`ui_bin_colors.ini`). El único índice relacionado con mandos es el 80 «Text Outline Colour (Controller Button Info)», un contorno de texto de ayuda de botones. **No hay nada de cursores 1P/2P.** (Revisado solo en lectura en `D:\Frank\Games_\Conmegol Patch\SiderAddons`.)
- **[EVIDENCIA, confianza media]** Hay mods públicos de Sider que **sustituyen texturas** de cursores del campo y barra de potencia (<https://www.pesmodding.com/2022/12/new-pes-2021-ingame-cursors-power-gauge.html>, pack retro con 3 cursores de colores distintos <https://www.pesmodding.com/2023/01/new-pes-2021-ingame-cursors-retro-pack.html>) y otro que cambia el icono de la selección de equipos (<https://www.pesmodding.com/2023/08/pes-2021-new-plate-color-mod.html>, `cmnlcon.bin`). Es decir: el **aspecto** del cursor es cambiable; la **regla** de a quién le toca cada uno, no.

#### 2b. Qué se consigue y qué no
- **Sí (sin tocar el juego):** elegir qué jugador de PES es cada persona, porque mando N de Link = jugador N (probado el 10 oct). Si PES colorea por número de jugador, entonces «mando 1 rojo» se logra poniendo a esa persona en el puesto 1.
- **No (sin modificar la interfaz):** que un color siga a la persona independientemente de su puesto/lado. Exigiría reemplazar texturas por mando (si el color es por número) o parchear lógica de UI en memoria (**no seguro**: la lectura directa de memoria ya cerró PES el 9 oct).

### 3. Recomendación
1. **No implementar.** No hay vía probada y segura que cumpla la regla de Fralex.
2. Si la prueba A muestra que PES enseña todos los pads y eso molesta: usar la vía **segura ya existente**, documentándola como opción «Puestos que se usan hoy» (K casillas llenas, el resto vacías, antes de abrir PES). Es un cambio pequeño en `OrdenPes`, pero **hay que decidirlo con Fralex** y requiere reiniciar PES para cambiar K.
3. Color: si la prueba B muestra que el color depende del número de jugador, bastará con **asignar puestos**; si depende del lado, no hay nada que hacer salvo mods de textura propios (otra investigación, no urgente).

### 4. Pruebas pedidas a Fralex (solo observar, sin cambiar nada)
- **Prueba A (mandos visibles).** Con la sala abierta, los 8 puestos enchufados y el orden de mandos encendido, entra a PES con 2 personas que pulsan. En la pantalla de selección de equipos y en el partido: ¿se ven los 8 pads, solo los 2 que pulsaron, o solo los que están en el lado de cada uno? Una captura vale.
- **Prueba B (colores).** Con 2 o 3 personas en puestos conocidos, apunta el **color del cursor/indicador** de cada una (selección de equipos y en el campo). Luego cambia a esas personas de puesto (sin cambiar el lado) y mira si el color se queda o cambia; después pásalas al otro lado y repite.

### 5. Fuentes y límites
- Búsqueda web amplia: poca información útil para PES 2021 multi-mando; todo lo de comportamiento queda como suposición hasta las pruebas.
- Docs previos: `ORDEN-MANDOS-PES.md`, `INVESTIGACION-CURSOR-MANDO.md`, `DETECCION-LADOS.md`.

### 6. Ronda 2 de búsqueda (12 oct): ¿existe alguna manera?

- **[EVIDENCIA]** Existe una herramienta pública con la misma idea que Link: <https://github.com/KOUFU-DIY/PES2021-Setting-Pro> (8 mandos Xbox virtuales persistentes con ViGEmBus + escribe `settings.dat`, copia `.bak`, juego y Settings cerrados). Ofrece «desactivar un número concreto» y reordenar, pero **no** oculta/muestra pads en vivo ni habla de colores. Confirma que el enfoque de Link (pads fijos + casillas) es el camino que sigue la comunidad.
- **[EVIDENCIA]** Parsec crea el mando del invitado cuando este pulsa botones (<https://support.parsec.app/hc/en-us/articles/32381826078868-Controller-Isn-t-Working-in-the-Game>): es el comportamiento «aparece al ocuparse» de Parsec puro, pero es justo lo que reordena los GUID; Link lo evita a propósito.
- **[EVIDENCIA indirecta, baja]** Reportes de PES 2018/2020 dicen que hay que reenchufar el pad dentro del juego tras configurarlo (<https://steamcommunity.com/app/592580/discussions/0/2765630416809431049>): sugiere que PES no re-enumera bien en caliente. Nada específico de PES 2021.
- **Color:** no apareció ningún offset ni textura documentada del cursor 1P/2P. Solo guías de color de texto del `.exe` y el pack de cursores de moukos13 (3 cursores de colores numerados 1-3 «para varios jugadores»; la página no dice si el número es por mando o por lado). Ver <https://www.pesmodding.com/2023/01/new-pes-2021-ingame-cursors-retro-pack.html>.
- **Conclusión de la ronda 2:** no existe una vía documentada mejor que la de §1b. La única pista abierta para color es que el pack de cursores usa texturas numeradas: si la prueba B muestra que el color sigue al número de jugador, un pack de texturas propio daría «mando 1 rojo, mando 2 azul» sin tocar memoria.

---

## Parte 9 · Investigación: ¿se pueden ampliar los 32 «datos personales» de PES 2021?

> Origen: `docs/INVESTIGACION-DATOS-PERSONALES-32.md` · último cambio e676669 2026-10-12 · 12254 bytes. Copia íntegra.

2026-10-12 · solo lectura (no se tocó el juego ni ningún guardado; no se escribió en memoria de PES). Etiquetas: ✅ evidencia propia · 🌐 fuente externa (comunidad, no oficial) · ❓ suposición.

### Respuesta corta

**Parcialmente / no recomendable tocarlo.** No hay forma segura y comprobada de pasar de 32 slots. Pero **no hace falta**: lo que Link necesita guardar por amigo (nombre, mando, ajustes propios, estadísticas) se puede guardar **fuera de PES**, en Link/web, sin límite. Recomendación: opción (c).

### 1. ¿Qué son los «datos personales»?

- ✅ `PES2021.exe` (Conmegol Patch) contiene la etiqueta `personal_data` dentro de los mensajes de red por jugador (junto a `pad_no`, `side`, `guest_num`, `guest_list`, `guest_name`, `guest_pad_no`, `CMD_USER_DATA_SYNC`) y un valor de enum `PERSONAL_DATA`. Es decir: son datos **por usuario que el juego sincroniza en línea** (anfitrión y cada invitado), no el option file.
- 🌐 Guías de la comunidad (FIFPlay, PESNewUpdate): en Ajustes de mando hay un «preset personal» con una lista de usuarios («user 1», …) donde cada uno guarda su configuración de mandos/cursor/ayuda de pase. Encaja con lo que ve Fralex.
- ❓ Mi hipótesis: «datos personales» = esa lista de usuarios/presets (nombre + ajustes por usuario), con tope 32. **No pude confirmar el nombre exacto del menú ni el 32**: ninguna fuente lo menciona (wiki de Rigged bloqueada por Cloudflare, no se esquivó).

### 2. ¿Dónde se guardan?

- ✅ En la PC de Fralex, `Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\2026\save` solo tiene `EDIT00000000` (10,9 MB). No hay otro archivo de usuarios ahí.
- ✅ El option file tiene (por nuestra base de conocimiento, `PhoenixSync/base-conocimiento/01-MOTOR-PES2021.md` §B) jugadores editados, equipos, plantillas, tácticas: **ninguna tabla de usuarios documentada**. ❓ No lo descifré de nuevo para buscar una; no es necesario para la conclusión.
- ✅ No existe `settings.dat` en la carpeta del juego Conmegol (solo `Settings.exe` y `Settings_b.dll`); `CPY.ini` usa `SavePath=%DEFAULT_CPY_DIR%` (en esta PC `%APPDATA%\CPY_SAVES\CPY\1259970` está vacío).
- ❓ Lo más probable es que los presets vivan en un archivo propio del juego/Steam (userdata o nube) o se creen al primer uso. **Sin captura o prueba en el PC de Fralex no se sabe cuál.**
- ❓ Dónde está el 32: no localizado. Podría ser constante del ejecutable, tamaño fijo de tabla del archivo o la interfaz. Averiguarlo exige desensamblar `PES2021.exe` (458 MB), fuera de alcance y de riesgo.

### 3. ¿Se puede ampliar?

| Opción | Viabilidad | Riesgo |
|---|---|---|
| (a) Parchear exe/formato para >32 | ❓ posible en teoría, sin comprobar | **Muy alto**: tabla fija → corrupción de guardados, desincronía online (el `personal_data` viaja por red: todos los jugadores tendrían que tener el mismo parche), puede romper con cada actualización del exe |
| (b) Rotar los 32 slots con Sider/Lua | ❓ requeriría escribir memoria/archivo de PES | Alto: escribir en datos del juego en vivo; sin API de Sider conocida para esto |
| (c) Perfiles propios de Phoenix por Parsec ID, guardados por Link/web | ✅ viable, ya hay base (Link sabe quién ocupa cada mando/puesto; ranking en web) | **Ninguno para PES**: no toca el juego |

### 4. Recomendación

1. **No ampliar el límite en PES** (regla «si algo se rompe, mejor no lo hagas»).
2. Si lo que importa son los ajustes por amigo: guardarlos en Link por Parsec ID (mapeo de mando, nombre, preferencias) → ya existe parte (mapeo por persona, ranking).
3. Los 32 slots de PES se pueden dejar para los 8 mandos habituales; el resto de amigos usa el perfil Phoenix.

### 5. Qué necesito de Fralex para cerrar

- Captura de la pantalla donde ve «32» y qué muestra cada usuario (nombre, mando, estadísticas…).
- Cuántos amigos distintos quiere cubrir y qué datos de ellos quiere conservar.

Con eso se decide si basta (c) o hace falta algo más.

---

### 6. Actualización (tras la captura de «Usuario 27» y la aclaración de Fralex)

**Objetivo real:** Fralex tiene más de 32 amigos, cada uno con su nombre y su configuración; quiere más de 32 «Datos personales» (Usuario N).

#### Qué sabemos (✅ = visto en `PES2021.exe`, solo lectura)
- ✅ «Datos personales» = **PD** en el código del juego. Hay menús propios `…PDSelect` (elegir usuario, el L2/R2 de la captura), `…PDSettingsMenu` (ajustes del usuario) y `…PDKeyConfig` / `…PDKeyConfigTypeSelect` (**mapeo de botones del usuario**) en Exhibición, Competición, Liga, Partido, Online y Coop. Es decir: **cada Usuario guarda al menos mapeo de botones y ajustes**; el nombre se asume (se ve en la captura) pero no está probado dónde se guarda. Cámara, radar y estadísticas: ❓ sin confirmar.
- ✅ En línea el juego envía el `personal_data` de cada jugador a los demás (`CMD_USER_DATA_SYNC`, `guest_list`, `pad_no`): el dato viaja con la partida, **no hace falta que el invitado lo tenga guardado en tu PC**.
- ✅ El ejecutable solo nombra dos rutas de configuración bajo `Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\`: `settings` y `settings.dat`. Más el sistema de guardado (`savesystem`, carpeta `save`).
- ✅ `settings.dat` mide 612 B = cabecera 0x64 + **8 casillas de 64 B** (GUID del mando + mapa de botones) (`ORDEN-MANDOS-PES.md`). **No cabe una tabla de 32 usuarios ahí.** Esa casilla es el mapa del *mando físico* de Settings.exe, otra cosa que los Usuarios.
- ✅ `EDIT00000000` (option file) no documenta tabla de usuarios; ❓ no se ha descifrado buscando «Usuario».

#### Qué NO se pudo averiguar (y por qué)
- ❓ **En qué archivo viven los Usuarios 1…32**: en esta PC solo está una copia de `…\2026\save\EDIT00000000`; no están ni `settings.dat` ni los datos reales de Fralex. Candidatos: el archivo `settings` (sin extensión), un archivo del `save\` real de Fralex, Steam/CPY (`%APPDATA%\CPY_SAVES\CPY\1259970` está vacío aquí), o dentro del option file.
- ❓ **De dónde sale el 32** (constante del exe, tabla fija del archivo o interfaz): sin el archivo no se puede decir. Con 8 mandos y 32 usuarios la tabla casi seguro es un array fijo de 32 entradas.
- ❓ Si PES relee los Usuarios sin reiniciar. El `settings.dat` solo se lee al arrancar (probado); para los Usuarios no hay prueba.

#### (A) ¿Subir el 32?
Sin localizar el archivo no hay base para afirmarlo. Si es array fijo en el formato **y** en el exe, ampliar = parchear el ejecutable de 458 MB y cambiar el formato, y como el `personal_data` viaja por red, todos los jugadores online tendrían que llevar el mismo parche. **No recomendado: riesgo alto de romper el guardado.**

#### (B) Perfiles ilimitados gestionados por Phoenix (rotación de 32 huecos)
Idea sólida y sin tocar el exe: Link guarda en su base (por Parsec ID) el contenido de cada Usuario y lo vuelca a uno de los 32 huecos antes de que esa persona juegue; al terminar, lee de vuelta los cambios. **Condiciones:** (1) que el Usuario esté en un archivo legible/escribible (no memoria); (2) que se pueda escribir con PES cerrado (siempre debería poder, como ya hace Link con `settings.dat`, con copia previa); (3) en vivo solo si PES relee el archivo, ❓ sin probar. Lo que Link sí puede hacer **sin tocar PES**: su propia base de perfiles (nombre, mapeo propio por persona, orden de mandos) y rotar a nivel archivo con PES cerrado.
Sin (1) se queda en (c): guardar solo lo de Link.
Para elegir el Usuario correcto al sentarse alguien en vivo haría falta tocar la selección en memoria: **no recomendado sin prueba segura**.

#### Dónde está en memoria/archivo «mando → columna» y «Usuario N de cada mando» (para la sesión que lee el lado)
❓ No localizado. Pistas verificadas: clases/menús `PDSelect`, `OnlineGuestUserPDSelect`, `MatchPDSettingSelectUser`, `ExhibitionUserDataDoalog`; el `pad_no` y `side` viajan juntos con `personal_data` en el mensaje `CMD_USER_DATA_SYNC`. Buen punto de partida para buscar estructuras con `pad_no`/`side`/índice de usuario (0–31) durante la pantalla de asignación.

#### Prueba segura que se pide a Fralex (nada se escribe en PES)
1. Con PES cerrado: ejecutar `tools/prueba-zztest/ANTES.bat` (respaldo con sello + listado de todas las carpetas de guardado).
2. Abrir PES, ir a Datos personales, editar un Usuario libre (p. ej. el 32): ponerle nombre **ZZTEST** y cambiar un botón reconocible (p. ej. pase ↔ disparo). Salir de PES por completo.
3. Ejecutar `tools/prueba-zztest/DESPUES.bat`: escribe `INFORME.txt` en el Escritorio (carpeta ZZTEST) con los archivos cambiados. Con una copia del archivo cambiado se busca «ZZTEST» y se determina formato, tamaño de entrada y si el 32 es un tamaño de tabla.

---

### 7. Contenido de un «Usuario» (capturas de Fralex, ✅ visto en pantalla)

Pantalla «Editar Datos personales» → «Elige un nombre para tus Datos personales»: cada Usuario guarda
- **Nombre** («Usuario 27», editable; es lo que Fralex usa para saber quién controla a cada jugador).
- **Configuración de comandos**: Normal / Alternativa / **Personalizado** (mapa completo de botones por contexto: Ataque 1/2, Defensa…, más «Desplazamiento de jugadores» L/R).
- Cambiar cursor, cursor de siguiente jugador, tipo de cursor, nv. de ayuda en pases, tipo de pase en profundidad, tipo de disparo, guía de dirección / objetivo / posicionamiento, amago automático, entrada automática, controles de compañero (y posiblemente más filas por debajo de la captura).
- ❓ Estadísticas por usuario: no aparecen en estas pantallas (no confirmado que existan).

Esto confirma que el Usuario = **perfil de ajustes + nombre**, no datos de partida. Sigue sin saberse en qué archivo está (ver §6): pendiente la prueba ZZTEST.

#### Consecuencia práctica
- **El nombre** (lo más importante para Fralex) se puede resolver **fuera de PES**: Link ya sabe quién ocupa cada puesto; basta mostrar «puesto N = nombre del amigo» en la UI de Link/overlay/web. Eso no depende de los 32 ni toca el juego.
- **La configuración de botones/ayudas** de cada amigo es lo que sí vive en PES. Con >32 amigos, la vía segura es (B) rotación con PES cerrado (si la prueba ZZTEST confirma que está en un archivo), guardando los perfiles en la base de Link por Parsec ID.

---

### 8. Resultado de la primera prueba (12 oct, ~13:17) y hallazgos

- ✅ La carpeta real de guardado de Fralex **no** está en `C:\Users\...`: es `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\<SteamID>\save` (239200, 239201, 239202…) y `settings.dat` está un nivel arriba. **El primer ANTES/DESPUES miró solo `C:\Users` y por eso salió vacío** (INFORME sin cambios). Corregido en `ZZTEST.ps1` (usa la carpeta Documentos real de Windows y las carpetas Users de todos los discos).
- ✅ Ese mismo día PES estuvo abierto (13:17–13:19, según `sider.log`) y **ningún archivo de `D:\Users` cambió** (solo `settings.dat`, a las 12:34, que es de Link). Es decir: la edición del Usuario no llegó a escribirse en disco, o se guarda solo en otro momento. ❓ Pendiente preguntar a Fralex si pulsó Confirmar y si vio «Guardando».
- ✅ Archivos de `save\`: `EDIT`, `ML`, `CUP`, `BL`, `REPLAY`, `FILELIST`, `GRAPHICS000000` (19 B) y **`SYSTEM00000000`** (239.764 B, tamaño fijo en todos los perfiles = tabla fija).
- ✅ `SYSTEM00000000` se abre con el mismo cifrado que EDIT/ML (probado con `SobrePes`, en COPIAS en el scratchpad): tipo `SYSTEM`, 219.568 B de datos. Contiene el **nombre del perfil de Steam/jugador («FRALEX», offset 84)** y datos de equipo favorito («FC Barcelona»…). Cabecera `10, 0x50, 0x35960, 0x29CE, 700, 700` (misma familia que `ML`).
- ❌ La cadena «Usuario» y «ZZTEST» **no están** en el `SYSTEM00000000` de 239200/239201 (datos descifrados). Así que o los Datos personales se guardan en otro sitio o están en binario/UTF-16 distinto, o el SYSTEM de hoy no se escribió.
- ✅ Los `SYSTEM00000000` de IDs distintos (239200 y 239201) difieren en ~6.900 bytes: hay datos propios por perfil.

Pendiente: repetir la prueba con el script corregido y confirmando el guardado en PES.

---

## Parte 10 · Perfiles de amigos: más de 32 «Datos personales» en PES 2021

> Origen: `docs/PERFILES-DE-AMIGOS.md` · último cambio d8f523f 2026-10-12 · 28478 bytes. Copia íntegra.

12 de octubre de 2026 · Frente LINK · Estado: **FASES 1, 2 y 3 hechas (motor, base de perfiles e interfaz); compila y enlaza (0.2.51); pruebas con COPIAS; NADA probado todavía con el PES real.**

### 1. El problema

PES 2021 guarda hasta **32 «Datos personales»** (Usuario 1…32). Cada uno lleva el nombre y la configuración de un amigo (botones, cursor, ayudas…). Fralex tiene más de 32 amigos, y el nombre le sirve para saber quién controla a cada jugador.

### 2. Qué se descubrió (con la prueba ZZTEST, 12 oct)

- ✅ Viven en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<SteamID>\save\SYSTEM00000000` (en la PC de Fralex: `D:\Users\Alexander\Documents\...\239200\save`). No en `settings.dat`.
- ✅ **PES no escribe ese archivo al editar el Usuario.** Lo escribe al **terminar un partido** (la primera prueba, sin partido, no cambió nada; la segunda, con partido, sí).
- ✅ El archivo está cifrado igual que EDIT/ML. Link ya sabe abrirlo (mismo cifrado). Descifrado: 219.568 bytes.
- ✅ Tabla **fija de 32 registros de 2.432 bytes desde el byte 0x50**; justo detrás (0x13050) siguen otros datos. Un registro: marca (0/1), **nombre de 48 bytes**, y desde +0x34 la configuración (mapa de botones, cursor, ayudas, mapas «Personalizado»…).
- ✅ El nombre «ZZTEST» apareció en el hueco 22 y cambiaron 4 bytes de la configuración (el botón que se cambió).
- ✅ **Ampliar el 32 de verdad exigiría cambiar el formato y el ejecutable** (la tabla está pegada a los demás datos). Descartado.
- ❓ Sin probar: que PES acepte un `SYSTEM00000000` escrito por Link (para EDIT y ML el juego no valida la cabecera; para SYSTEM falta probarlo). También si lee el archivo en vivo o solo al arrancar.

### 3. Plan (opción B, aprobada por Fralex)

Link guarda un **perfil por amigo** (nombre + configuración) en su propia base y lo vuelca a uno de los 32 huecos **antes de abrir PES**; al terminar, recoge los cambios que el amigo hizo.

| Fase | Qué | Estado |
|---|---|---|
| 0 | Localizar el archivo (ZZTEST) | ✅ |
| 1 | Módulo de Link `core/PerfilesPes`: leer los 32, exportar/importar un Usuario, escritura segura | ✅ con COPIAS (`pruebas-link/correr_perfiles.bat`) |
| 2 | Base de perfiles (por nombre), huecos protegidos, traer (PES a Link) y preparar sala (Link a PES): `core/PerfilesAmigos` | ✅ con COPIAS (`pruebas-link/perfiles_amigos_test.cpp`) |
| 3 | Interfaz GENTE › PERFILES PES (`web/AccionesPerfilesWeb.cpp`, `ui/js/vistas/perfiles.js`, simulador) | ✅ vista probada en el simulador |
| 4 | Pruebas en el juego (con permiso de Fralex) y guía | pendiente |

### 4. Fase 1: qué hace el módulo (`SmashSoda/phoenix/core/PerfilesPes.*`)

- `Guardado::abrir` lee y valida el `SYSTEM00000000` (cabecera del sobre, tipo SYSTEM, PES 2021, 219.568 B, los 32 registros con forma de Usuario). Si algo no cuadra, **no hace nada**.
- `usuario(i)` / `usuarios()` leen nombre y registro; `aPerfil` / `dePerfil` exportan e importan un perfil en **JSON propio de Phoenix** (`{formato:1, juego:"PES2021", nombre, tam, registro:"<hex>"}`).
- `ponerUsuario` / `rotular` cambian un hueco en memoria. Solo cambia ese registro; el resto del archivo queda idéntico (probado byte a byte).
- `guardarComo` escribe un archivo nuevo (nunca sobrescribe) y lo relee. `reemplazar` hace **copia `SYSTEM00000000.phoenix-<sello>`** al lado, escribe un temporal, lo relee y valida, y solo entonces reemplaza (todo o nada).
- Pruebas (con una COPIA real): abrir y listar 32; cifrar sin cambios da **exactamente el mismo archivo**; exportar → importar → exportar es idempotente; los otros 31 Usuarios y los datos de después de la tabla no cambian; rechaza hueco 33, registro corto o corrupto, archivo truncado, basura, archivo inexistente, perfil de otro formato; no deja temporales; el original no se toca.

### 5. Reglas para las fases siguientes

- **Solo con PES cerrado** (salvo que se pruebe que PES relee el archivo en vivo).
- Nunca escribir en los guardados reales de Fralex sin su permiso explícito y copia previa.
- Huecos **protegidos** (los que Fralex quiera conservar tal cual) y huecos **rotativos** (p. ej. los últimos 8–16).
- Si dos amigos piden el mismo hueco, gana el primero y el otro va a otro hueco libre; si no hay, se avisa (máximo 32 jugando a la vez).
- **Nunca se pierde una configuración**: antes de pisar un hueco rotativo, su contenido se recoge a su perfil o se guarda copia.
- El nombre de los amigos **no** se sube a ningún sitio ni se escribe en esta documentación.

### 6. Cómo reproducir las pruebas

```
pruebas-link\correr_perfiles.bat "<copia de SYSTEM00000000>"
```
Trabaja solo sobre una copia en una carpeta temporal. Resultado esperado: 32 líneas «Usuario N» y `TODO OK`.

### 7. Cómo se usa (GENTE › PERFILES PES)

1. **TRAER DE PES** (solo lee PES): guarda en Link cada Usuario de PES. Hazlo al principio y cuando cambies algo en PES. Link también lo hace solo cuando PES se cierra (mientras esa pestaña esté abierta).
2. Marca el candado 🔒 en tu(s) hueco(s): Link no los toca.
3. **¿QUIÉNES JUEGAN HOY?**: marca a los amigos (Link recuerda la última elección). Caben tantos como huecos sin proteger.
4. Con **PES cerrado**, **PREPARAR LA SALA**: Link trae primero (nada se pierde), hace copia con sello del guardado (`SYSTEM00000000.phoenix-<sello>`) y pone a los elegidos en los huecos libres de atrás hacia delante. Si ya estaban en un hueco con el mismo contenido, no escribe nada.
5. Abre PES y, en la pantalla de elegir mandos, elige tu nombre con L2/R2.

Qué se guarda: `perfiles-amigos.json` y `ajustes.json` en `%APPDATA%/Trybuchet/Smash Soda/phoenix-perfiles-amigos/` (+ carpeta `respaldos` con las 20 últimas copias). Un perfil se identifica por su NOMBRE; un Usuario sin nombre se guarda como «Usuario N».

### 8. Lo que falta

- **Probarlo en el PES real** (Fralex lo autoriza y se hace con copia previa): que PES acepte un `SYSTEM00000000` escrito por Link y que el Usuario con ese nombre aparezca en la lista.
- Vincular un perfil a un Parsec ID (hoy se identifica por nombre).
- Que PES lo recargue en vivo (hoy solo con PES cerrado).

### 9. Siguiente paso: que el Usuario se ponga solo al sentarse el amigo (investigación)

Lo que quiere Fralex: si el amigo «Erfuzz» se sienta en el mando 7, el mando 7 usa su Usuario de PES (nombre, tipo de pase, de disparo, guías...) sin hacer nada.
Camino seguro (sin escribir en la memoria de PES): (1) encontrar en la memoria, **solo leyendo**, el número de Usuario que tiene cada mando en la pantalla de elegir lados; (2) Link, que controla el mando virtual de cada puesto, pulsa **L2/R2** en ese mando hasta que el número coincida con el hueco del amigo (lo que dejó «Preparar la sala»), y se detiene.
Paso 1: `tools/radar/usuariopad.cpp` (compilar con `tools/radar/compilar_usuariopad.bat`): foto, R2, foto, R2... y deja los bytes que suben +1 cada vez. Sin resultados todavía.

#### 9.1 Resultado de la investigación del número de Usuario (12 oct, 14:50, PC de Fralex)
- ✅ `PES2021.exe+0x37F18E4` (dirección fija 0x1437F18E4 con el exe en 0x140000000; igual en dos arranques) vale el **número de Usuario de 0 a 31** de la pantalla de elegir lados: R2 suma 1 y de 31 pasa a 0. Empezó en 21 = Usuario 22 y acabó en el Usuario 32 (DEGOX), como se anotó en pantalla.
- Estructura vecina (u16): `+0 = 7`, `+2 = 7`, `+4 = índice actual`, `+6 = índice anterior`, `+8 = 32 (nº de Usuarios)`. Parece el estado de la lista de Usuarios de la interfaz.
- ❓ Falta probar con DOS mandos si ese valor es el Usuario del mando que acaba de pulsar (lo necesario para el bucle «pulsar R2 hasta llegar al hueco») o solo de uno.
- Las otras dos posiciones (0x1437F1844 y 0x143AAC364) son contadores que no dan la vuelta; no sirven.
- Plan seguro: Link solo LEE esa posición y pulsa L2/R2 con el mando virtual del puesto hasta que coincida con el hueco del amigo (el que dejó «Preparar la sala»). Solo en la pantalla de elegir lados (si no, R2/L2 hacen cosas en el partido): hace falta una señal fiable de esa pantalla.

#### 9.2 Prueba con DOS mandos (12 oct, ~15:00): 0x1437F18E4 NO es el Usuario de cada mando
- R2 alternando mando 1 y mando 2 hizo subir `0x1437F18E4` siempre +1 (16, 17 … 25), sin saltar entre dos valores. Los Usuarios finales fueron AXEL y JACK (casillas 15 y 16, valores 14 y 15), no el 25.
- Conclusión: es el cursor de la lista de la pantalla, COMPARTIDO entre mandos; no sirve para saber el Usuario de cada uno. Además PES salta los Usuarios que ya usa otro mando.
- Para el modo automático haría falta la posición propia de cada mando (dos búsquedas más, moviendo solo un mando cada vez: lo que cambia en ambas es el cursor compartido; lo que cambia solo en una es de ese mando).
- Alternativa sin riesgo, ya disponible: «Preparar la sala» deja el Usuario de cada amigo listo en PES y cada uno pulsa R2 hasta ver su nombre.

#### 9.3 La copia de los Usuarios que PES tiene en memoria (12 oct, ~15:05)
- ✅ `usuariopad.exe buscar FRALEX` encontró UNA copia real: el registro del Usuario 1 con su configuración (`01 03 02 00 04 06 05 07 08 08 …`) en una región de memoria RW (dirección alta tipo 0x7FF4…, cambia en cada arranque). Es la tabla de 32 registros de 2.432 B tal como está en el archivo SYSTEM00000000 descifrado.
- Prueba de escritura REVERSIBLE (con permiso expreso de Fralex): `usuariopad.exe escribir <nombre> <casilla> <casilla vacía> [texto]` localiza la tabla por el nombre, valida los 32 registros, escribe un texto SOLO en el nombre (48 B) de una casilla vacía y lo restaura al pulsar Numpad 8, al cerrar la ventana o al cerrarse PES. No toca ningún archivo.
- ❓ Resultado pendiente: si PES muestra ese nombre en la lista de Usuarios sin reiniciar.

#### 9.4 RESULTADO: la escritura en vivo FUNCIONA (12 oct, ~15:20)
- ✅ `usuariopad.exe escribir FRALEX 1 31 ZZMEM`: se escribió «ZZMEM» en el nombre del Usuario 31 en la copia que PES tiene en memoria y **apareció al instante sobre el mando en la pantalla de elegir lados**; al restaurar (Numpad 8) desapareció. No se guardó nada en archivos. Tabla en 0x7FF4FBD60010 en ese arranque (cambia en cada arranque; se localiza buscando un nombre conocido y validando los 32 registros).
- Conclusión: PES lee los Usuarios en vivo de esa tabla. Un módulo (o Link) puede poner el perfil de un amigo (registro de 2.432 B) en un Usuario con PES abierto.
- Falta: saber qué Usuario tiene cada mando (para elegir el hueco). Siguiente paso: `usuariopad.exe punteros` (solo lee) busca las referencias a cada registro.

#### 9.5 RESULTADO: dónde está «el Usuario de cada mando» (12 oct, ~15:45, PES 2021 ConmeGOL, PC de Fralex)
- ✅ **Lista de 8 u32 = índice de Usuario (0..31) de cada uno de los 8 mandos.** Se encontró con la herramienta `pares` (dos estados, 4 Usuarios con nombre: una sola pareja candidata) y se confirmó con R2: el mando 1 cambia la entrada 0 (2→4→8) y el mando 2 la entrada 1 (31→0→2); las otras 6 no se mueven.
- ✅ PES **salta los Usuarios que ya tienen los otros mandos** (los de los mandos sin usar: 1,3,30,5,6,7…), por eso R2 hace 2→4, 4→8, 0→2.
- ✅ **Ancla fija**: `PES2021.exe + 0x36F5820` (0x1436F5820 con el exe en 0x140000000) contiene un puntero de 8 bytes a esa lista, y `+0x36F5828` un u32 = **8** (nº de mandos). Comprobado en vivo (dos lecturas). La lista está en el heap (cambia por arranque), la ancla no.
- ✅ **Configuración del Usuario: una sola copia** (la tabla de 32×2.432 B). 0 copias fuera de la tabla (`copias`). La tabla está en el heap; no hay ancla estática (4 referencias, todas en el heap); se localiza escaneando regiones RW y validando 32 registros.
- ✅ **Escribir en la tabla se ve al instante** (prueba ZZMEM, 15:07).
- ❓ Sin probar: que un REGISTRO COMPLETO escrito en la tabla cambie también las OPCIONES en el partido (cursor, tipo de pase…), no solo el nombre; que el orden de entradas = puesto N de Link (probable por «Orden de mandos en PES»: casilla N = puesto N); qué pasa con la tabla al guardar PES (se guardaría el Usuario sobrescrito en SYSTEM00000000: por eso hay que restaurar el original al terminar).
- Plan seguro de «Usuario automático»: (1) Link hace TRAER (copia de todo). (2) En la pantalla de elegir lados, para cada puesto con un amigo con perfil: leer el índice de su mando (ancla + entrada N-1) y escribir su perfil en ese registro de la tabla (todo o nada: leer, escribir, releer). (3) Al terminar o salir el amigo, restaurar el registro original. Solo en la pantalla de lados (señal de `LadoPantalla`).

### 10. Prueba en vivo en Link 0.2.53 (beta): «Usuario automático», primer paso
- `core/UsuariosEnVivo.*`: se engancha a PES2021.exe, lee el Usuario de cada mando (ancla `exe+0x36F5820`), localiza la tabla de 32 Usuarios y escribe el registro de UN Usuario con relectura (si no coincide, lo deja como estaba).
- Acciones: `perfiles.mandos` (solo lee), `perfiles.aplicar {puesto, id}` y `perfiles.restaurar`. Antes de pisar un Usuario guarda lo que había como perfil de Link y en memoria (para restaurar).
- Interfaz: GENTE › PERFILES PES › «PRUEBA EN VIVO (BETA)»: elegir puesto y perfil → PONER EN EL MANDO; VER USUARIO DE CADA MANDO; RESTAURAR LO ANTERIOR.
- Prueba manual de solo lectura: `pruebas-link/correr_usuarios_vivo.bat` (con PES abierto).
- ❓ Sin probar con el PES real: que el registro COMPLETO cambie las opciones del partido; que puesto N de Link = mando N de PES.
- Falta: aplicarlo solo al sentarse el amigo (señal de la pantalla de lados), el perfil del amigo nuevo que viene de su cuenta (web) y el editor de la configuración.

#### 10.1 Prueba REAL con PES abierto (12 oct, 16:40, Link 0.2.57, hecha por Claude sobre el PC de Fralex con su permiso)
- ✅ «VER USUARIO DE CADA MANDO» lee bien: P1 FRALEX, P3 ARENZ… coincide con lo que PES muestra sobre cada mando. Hubo dos fallos reales, ya corregidos: (1) Link elegía una **tabla falsa** (región con 0xFF en los nombres): `tablaValida` ahora exige nombres UTF-8 válidos y mapa de botones con valores ≤ 0x10; (2) el valor **32 en la lista de mandos = «ese mando todavía no tiene Usuario»** (aparece al reconectarse los mandos virtuales de Link): se trata como -1, no como error.
- ✅ **PONER EN EL MANDO (puesto 1, perfil AXEL): PES mostró «AXEL» sobre el mando 1 en la pantalla de elegir lados**, sin reiniciar. «RESTAURAR LO ANTERIOR» devolvió FRALEX (comprobado leyendo la memoria: un solo registro FRALEX, AXEL solo en su casilla).
- ❓ NO comprobado: que las OPCIONES (tipo de pase, disparo, guías, botones) se vean/apliquen — hace falta abrir «Editar Datos personales» (□) con ese mando; con el teclado no se encontró la tecla del cuadrado (la S cambió de Usuario). El registro escrito es el completo (2.432 B) y Link verifica por relectura que quedó idéntico al perfil.
- Observaciones: al reiniciar Link, PES reasigna los Usuarios de los mandos (el orden de la lista de mandos cambió: 0,5,1,2,32,3,6,4). Hay que releer la lista siempre antes de escribir (ya se hace). PES no deja dos mandos con el mismo Usuario.

#### 10.2 RESULTADO FINAL de la prueba real (12 oct, 16:45, Link 0.2.57): las OPCIONES también se aplican en vivo
- Tecla del botón cuadrado en el teclado de PES = **C** (la dijo Fralex); **Z** = Atrás; **S** cambia de Usuario (R2). Los códigos de tecla están en `settings.dat` (desde +0x34, formato `10 <scancode>`).
- ✅ Con el mando 1 en FRALEX, «Editar Datos personales» mostraba: ayuda de pases Nivel 2, guía de dirección No, guía de posicionamiento No, amago automático No. Tras **PONER EN EL MANDO (puesto 1, perfil AXEL)** mostró: nombre AXEL, ayuda de pases **Nivel 1**, guía de dirección **Sí**, guía de posicionamiento **Sí**, amago automático **Sí**. Es decir: **el registro completo escrito en la tabla cambia nombre Y opciones en vivo, sin reiniciar PES.**
- ✅ RESTAURAR LO ANTERIOR devolvió FRALEX («Usuarios restaurados como estaban»).
- Aprendido: la lista de 8 mandos solo existe en la pantalla de ELEGIR LADOS (en «Editar Datos personales» el puntero es 0). Link 0.2.58: si PES está abierto pero esa lista no existe, «Restaurar» NO olvida lo pendiente (antes lo habría olvidado) y pide volver a esa pantalla.
- ❓ Sin comprobar: el efecto de esas opciones DENTRO de un partido (el juego las lee de la tabla al empezar; es lo esperable). Tampoco el puesto N de Link = mando N de PES con mandos reales de amigos.
- Falta: aplicarlo solo al sentarse el amigo (señal de pantalla de lados) y de dónde sale el perfil del amigo nuevo (su cuenta/web) + editor de la configuración.

#### 10.3 Puesto 2 = mando 2 (12 oct, 16:57, Link 0.2.58, prueba hecha por Claude con el mando 2 de Fralex en el lado Visitante)
- ✅ «PONER EN EL MANDO» con Puesto 2 y perfil JACK: PES mostró «JACK» sobre el **segundo mando** (el rosa, columna Visitante); el mando 1 siguió en FRALEX. **Puesto N de Link = entrada N-1 de la lista de mandos = mando N de PES** (con el orden que deja «Orden de mandos en PES»).
- ✅ Restaurar OK. La lista de mandos cuenta con el mando fuera del lado Local también (los Usuarios siguen por número de mando, no por lado).
- Nota de uso de la interfaz: la primera pulsación en Link tras dar foco a su ventana a veces solo da foco; si «Poner en el mando» no abre el aviso, pulsar otra vez.
- Siguiente: aplicarlo SOLO al sentarse el amigo (ver §9.5, plan) y el vínculo persona → perfil.

### 11. MAPA DE OPCIONES de un Usuario de PES 2021 (registro de 2.432 B) — decodificado el 12 oct con «Editar Datos personales» + volcado de memoria
Método: en la pantalla de edición (tecla C = cuadrado, Z = Atrás, flechas = D-pad) se cambia una opción, se SALE de la pantalla (PES guarda la edición al salir; dentro no cambia la memoria) y se vuelca el registro con `usuariopad registro` comparando con el anterior. Verificado cambiando las 12 opciones de FRALEX y restaurándolo (registro idéntico al original).

| Offset | Opción en pantalla | Valores vistos |
|---|---|---|
| +0x00 | marca | 0 / 1 |
| +0x04 (48 B) | **Datos personales (nombre)** | texto UTF-8 terminado en 0 |
| +0x34…+0x47 | **Configuración de comandos** (mapa de botones, 2×10 B) | Normal = `01 03 02 00 04 06 05 07 08 08 / 01 03 02 00 07 05 04 06 07 07` |
| +0x48 | Cambiar cursor | 1 = Semiasistido, 2 = Sin ayuda (0 = ¿Asistido?) |
| +0x4B | Cursor de siguiente jugador | 0 = Sí, 1 = No |
| +0x4C | Tipo de cursor | 4 = «Datos personales», 0 = Nombre del jugador |
| +0x4D | Nv. de ayuda en pases | 1 = Nivel 1, 2 = Nivel 2 |
| +0x4E | Tipo pase en profundidad | 0 = Básico, 1 = Avanzado |
| +0x4F | Tipo de disparo | 0 = Básico, 1 = Avanzado |
| +0x50 | Guía de dirección | 0 = Sí, 1 = No |
| +0x51 | Guía de objetivo | 0 = Sí, 1 = No |
| +0x52 | Guía de posicionamiento | 0 = Sí, 1 = No |
| +0x53 | Amago automático | 0 = Sí, 1 = No |
| +0x54 | Entrada automática | 1 = Semiautomático, 2 = No (0 = ¿Automático?) |
| +0x55 | Controles de compañero | 0 = Asistido, 1 = Manual |
| +0x49, +0x4A, +0x56… | sin identificar (valores 1,1 / 0 / 1…) | no tocar: se conservan al copiar el registro |

- ❓ Pendiente de identificar: los valores de otros textos de «Tipo de cursor», el valor 0 de «Cambiar cursor» y de «Entrada automática», y los presets «Alternativa/Personalizado» de la configuración de comandos (bytes +0x34…+0x47).
- Consecuencia para el editor de Link: un perfil nuevo se construye **copiando un registro base real** (p. ej. el de cualquier Usuario sin nombre) y cambiando solo estos bytes y el nombre; así los campos desconocidos quedan como PES los espera.

#### 11.1 Configuración de comandos (subpantalla, 12 oct) — pedido de Fralex
- ✅ **+0x32 = Desplazamiento de jugadores** (0 = stick izquierdo, 1 = cruceta). **+0x33 = preajuste** (0 = Normal, 1 = Alternativa; «Personalizado» se espera = 2 y usaría el mapa de +0x34…+0x47; sin comprobar). Cambiar a «Alternativa» NO toca los bytes +0x34…+0x47: el preajuste solo cambia +0x33.
- Mapa de botones (+0x34…+0x47): códigos × = 0, ○ = 1, □ = 2, △ = 3, L1 = 4, R1 = 5, L2 = 6, R2 = 7, 8 = ninguno. Primer bloque (Ataque) de «Normal»: `01 03 02 00 04 06 05 07 08 08` = Pase bombeado ○, Pase en profundidad △, Disparo □, Pase raso ×, Cambiar cursor L1, Controles manuales L2, Esprintar R1, Controles especiales R2. Segundo bloque (Defensa): `01 03 02 00 07 05 04 06 07 07` (los 4 primeros = Entrada deslizante ○, Salir con el portero △, Presión compañeros COM □, Presión ×; el resto sin decodificar).
- Nota de uso del teclado en PES (mi método): en el cuadro del NOMBRE el teclado escribe letras (la Z se añadió al nombre; se borró con retroceso); la primera pulsación tras cambiar de pantalla a veces se pierde; hay que comprobar la fila resaltada antes de Enter.
- Nota de Fralex: los nombres de las opciones pueden variar de un PES o parche a otro (idioma/versión): el editor de Link debe trabajar con los BYTES y tener sus etiquetas por parche, no fiarse de los textos.


### 12. Usuario automático (Link 0.2.59)

Objetivo: que quien se siente en un puesto tenga SU nombre y SU configuración en el Usuario de su mando, sin hacer nada más.

- **Vínculo**: cada perfil de Link puede ligarse a una persona por su **identificador de Parsec** (no por el nombre de cuenta; Fralex: el nombre real lo decide la propia persona). Se guarda en `perfiles-amigos.json` (`parsecId`). Una persona tiene un solo perfil: ligar otro quita el anterior. La interfaz está en GENTE › PERFILES PES › USUARIO AUTOMÁTICO (select por cada sentado, o por ID manual).
- **Vigilante** (`tickPerfilesAuto`, 1 vez por segundo, solo con PES abierto y en la pantalla de elegir lados, que es cuando existe la lista de mandos): para cada sentado con perfil, escribe el perfil en el Usuario de su mando (puesto N = mando N). Antes guarda lo que había como perfil si no estaba. Al levantarse o cambiar de persona o de Usuario, devuelve el registro original (con relectura). Si la persona cambió sus opciones en PES, ese cambio se guarda en SU perfil en vez de pisarlo.
- **Interruptor** ACTIVAR/APAGAR AUTOMÁTICO (`perfiles.auto`, ajuste `autoUsuarios`, activo por defecto). Si hay Usuarios pisados al apagarlo, los devuelve.
- **Sentados de prueba** (`perfiles.simular {puesto, parsecId}`): simulan a alguien sentado sin gente de verdad; sirven para probar.
- **Prueba real (12 oct 2026, PC de Fralex)**: perfil «Usuario 2» ligado a un ID falso, sentado de prueba en el puesto 1 → PES mostró «Usuario 2» en el mando 1 a los ~2 s; al quitar el sentado volvió «FRALEX» idéntico.
- Acciones nuevas: `perfiles.vincular {id, parsecId}`, `perfiles.auto {si}`, `perfiles.simular {puesto?, parsecId?}`. `perfiles.estado` añade `auto`, `autoLog`, `sentados`, `autoAplicados` y `parsecId` por perfil.
- Pendiente: «Mi perfil de PES» (editor de nombre y opciones por bytes, etiquetas por parche) y que el perfil viaje con la persona a otros anfitriones.

#### 12.1 Mi perfil de PES: formulario con nombre y opciones (Link 0.2.61)

- En GENTE › PERFILES PES › MIS PERFILES: botones **+ NUEVO PERFIL** y **EDITAR**. El formulario es el «Editar Datos personales» de PES: nombre, **configuración de comandos** (preajuste, desplazamiento y, si es Personalizado, los 20 botones), **cambiar cursor**, cursor de siguiente jugador, **tipo de cursor** (siempre «Datos personales», fijo), nivel de ayuda en pases, pase en profundidad, tipo de disparo, guías, **amago automático**, **entrada automática**, **controles de compañero**, y el ID de Parsec opcional para ligarlo.
- Lógica en `core/OpcionesUsuario.{h,cpp}` (solo bytes; las etiquetas viven en `vistas/perfiles.js`, por parche). Acciones `perfiles.opciones{id?}` y `perfiles.editar{id?, nombre, parsecId?, opciones}`. Un perfil nuevo parte de un registro real de la base y cambia SOLO el nombre y los bytes conocidos (el resto se conserva).
- **Corrección importante**: el campo del nombre (48 B desde +0x04) comparte sus 2 últimos bytes con las opciones +0x32 (desplazamiento) y +0x33 (preajuste). Antes, poner un nombre los borraba. Ahora solo se borran los primeros 46 bytes (`ponerNombre`, `renombrar` y `Guardado::rotular`).
- Si se edita el perfil de alguien que ya está sentado, el vigilante le pone la versión nueva.
- **Prueba real (12 oct 2026)**: perfil «PRUEBA LINK» ligado a un ID falso y sentado de prueba en el puesto 1 → PES mostró ese nombre en el mando 1 y, en Editar Datos personales, las opciones elegidas; al quitar el sentado volvió FRALEX; el perfil de prueba se borró. Los valores de FRALEX en el formulario coincidieron con su pantalla de PES.
- Sin comprobar aún: «Cambiar cursor» = 0, «Entrada automática» = 0 y el preajuste Personalizado (=2) con su mapa de botones; las etiquetas de los botones de defensa a partir del 5.º.

#### 12.2 Salas abiertas dentro de Link (0.2.62)

- Pedido de Fralex: ver en Link las mismas salas que muestra la web, para entrar sin salir de la app. GENTE › **SALAS ABIERTAS**: tarjetas con anfitrión, juego/parche/región, plazas libres, calidad (semáforo y ping), etiquetas TE INVITARON / AMIGO / EN PARTIDA / INESTABLE y botón **UNIRME** (abre el enlace de Parsec; apagado si es tu sala, está llena o no tienes acceso).
- Datos: `GET /v1/salas` (contrato §17 de la web: mismas reglas de visibilidad que la página). `PhoenixLink::pasoSalas` sondea cada `sondeo_seg` (25 s) con ETag y SOLO con la ventana visible (§21, por el coste de llamadas). Solo se ofrece entrar con un enlace `https://`. Estado de la interfaz: `salasAbiertas{cargadas, lista[]}`.
- No se creó ninguna acción nueva: UNIRME usa `ui.abrir`.

#### 12.3 Nuevo orden de pestañas (Link 0.2.63)

Reagrupado a pedido de Fralex (GENTE era un cajón de sastre). Solo cambia dónde cuelga cada pestaña; los nombres internos y las acciones no cambian.

- **SALA:** Resumen · **Salas abiertas** · Opciones de sala · Juegos · Conexión · Actividad. El Resumen con la sala cerrada muestra el botón «SALAS ABIERTAS (N)» si hay salas donde entrar.
- **MANDOS:** Puestos · **Perfiles PES** · Tu mando · Mapeo · Teclado · Turnos · Bloqueo.
- **GENTE:** En sala · Moderación · Amigos · Permisos.
- PARTIDO, SYNC y AJUSTES igual.
- Donde en este documento dice «GENTE › PERFILES PES» o «GENTE › SALAS ABIERTAS», ahora es **MANDOS › PERFILES PES** y **SALA › SALAS ABIERTAS**. El estado `salasAbiertas` llega con la sección Sala visible.

#### 12.4 Sección nueva JUGAR (Link 0.2.65, reemplaza el reagrupamiento de 12.3)

A pedido de Fralex: salas abiertas no encajaba en SALA (es «tu sala») ni en GENTE (administración), así que hay una sección nueva, **JUGAR**, con lo que se busca rápido al ir a jugar. Va en el menú justo después de SALA.

- **JUGAR:** Salas abiertas · Amigos · Perfil PES · Mi puesto.
- **SALA:** Resumen · Opciones de sala · Juegos · Conexión · Actividad (el Resumen con la sala cerrada tiene un botón «SALAS ABIERTAS (N)» que lleva a JUGAR).
- **MANDOS:** Puestos · Tu mando · Mapeo · Teclado · Turnos · Bloqueo.
- **GENTE:** En sala · Moderación · Permisos.
- PARTIDO, SYNC y AJUSTES igual. La sección no existe en el modo «PC cedida» (solo se ven Sala, Partido y Mandos).
- Archivos: `vistas/seccionjugar.js` (solo compone vistas que ya existían), `app.js` (menú), `ui.js` (icono), `tienda.js` (pestaña por defecto), `paleta.js` (Ctrl+K), `tutorial.js` (los pasos que señalan el menú por posición se corrieron un lugar y hay un paso nuevo). El estado `amigos` y `salasAbiertas` llega también con JUGAR visible.
- Donde este documento dice «GENTE › PERFILES PES» o «MANDOS › PERFILES PES» ahora es **JUGAR › PERFIL PES**.

#### 12.5 Orden final del menú (Link 0.2.66)

Ajuste pedido por Fralex: primero las secciones del anfitrión, luego lo de jugar en salas de otros.

- Menú: **SALA · PARTIDO · MANDOS · GENTE · JUGAR · SYNC · AJUSTES**.
- **JUGAR** (salas de otros): Salas abiertas · Amigos · Perfil PES.
- **SALA:** Resumen · **Mi puesto** · Opciones de sala · Juegos · Conexión · Actividad («Mi puesto al jugar» vuelve a Sala).
- MANDOS y GENTE como en 12.4. El tutorial se reajustó a las posiciones nuevas del menú (JUGAR va después de GENTE).

---

## Parte 11 · Modo cedido: prestar tu PC a un amigo (solo Link y PES)

> Origen: `docs/MODO-CEDIDO.md` · último cambio 4e5c6e4 2026-10-16 · 7984 bytes. Copia íntegra.

**Para qué sirve:** cuando no estás, o no puedes manejar la PC, un amigo conectado por Parsec maneja **Phoenix Link y PES 2021 con teclado y ratón**. No puede usar nada más de Windows. Tú puedes seguir usando tu teclado y tu ratón a la vez.

**Estado (11 oct 2026):** compilado y probado en el simulador y con pruebas automáticas. **Falta probarlo con un amigo real** (paso 6).

---

### 1. Antes de empezar (una sola vez)

1. En **Sala › Juegos**, elige el juego de la biblioteca que abre PES (el mismo que usarías en modo quiosco). Ese es el que abre el botón **ABRIR PES**.
   - Si usas Sider, pon en la biblioteca el lanzador que usas siempre.
   - Sin juego elegido, ABRIR PES solo puede traer PES al frente si ya está abierto.
2. Abre la sala como siempre.

### 2. Ceder la PC

1. Entra en **Gente › En sala** y toca a tu amigo. También puedes tocarlo en la lista de Sala. Puede estar **jugando o mirando**: da igual.
2. Pulsa **CEDER MI PC** y confirma. En el chat de la sala sale: «Fralex cedió su PC a …». Link le da teclado y ratón a tu amigo también en Parsec (desde 0.2.96; antes su Parsec no los enviaba).

No hay PIN. Devolver la PC siempre le quita el control a tu amigo, así que no hace falta protegerlo.

### 3. Qué ve y qué puede hacer tu amigo

- Arriba sale la banda **PC CEDIDA A …**, con un botón grande **ABRIR PES** (o **IR A PES** si ya está abierto) y **DEVOLVER LA PC**.
- Menú: solo **Sala** (Resumen, Conexión, Actividad), **Partido** y **Mandos** (Puestos y Bloqueo).
- Puede repartir mandos, aceptar a quien quiere jugar, llevar el partido y escribir en el chat de la sala.
- **No puede:** Ajustes, Sync, tu cuenta de la web, el chat General de la web, banear, expulsar, cambiar permisos, cerrar la sala, cerrar Link (ni con la X ni con Alt+F4), ni abrir el navegador.
- Los demás invitados **pierden teclado y ratón** mientras dure (siguen con su mando).

**En Windows, el guardián de Link:**
- Solo deja pasar los clics que caen sobre PES o Link. La barra de tareas, el escritorio y otras ventanas no reciben clics.
- La tecla Windows **sola** (pulsar y soltar) minimiza PES y muestra Link; nunca abre el menú Inicio (desde 0.2.96). Sus atajos (Win+D, Win+R…) siguen bloqueados. También bloquea Alt+Tab, Alt+Esc, Alt+Espacio, Ctrl+Esc, Ctrl+Shift+Esc, Impr Pant, Shift+Tab (overlay de Steam), las teclas que abren programas y los atajos del navegador interno de Link (Ctrl+P, Ctrl+N, F12…).
- Alt+F4 sí cierra PES (se vuelve a abrir con ABRIR PES), pero nunca Link.
- **Link no se puede minimizar, cerrar, mover ni cambiar de tamaño** desde la entrada de tu amigo (botones de la ventana, Alt+F4, Alt+Espacio). Si Link se minimiza por otra vía, vuelve a mostrarse solo. Tú sí puedes hacerlo con tu ratón o teclado físico (desde 0.2.93).
- Si otra ventana se pone delante, la devuelve a PES (o a Link) en menos de un segundo. Esto no pasa si tú estás usando tu teclado o ratón (espera 15 s sin que los toques).

**Tú sigues mandando:** tu teclado y tu ratón físicos nunca se bloquean, así que puedes usar la PC a la vez que tu amigo. Cuando tocas los tuyos, los de él se pausan un instante para que no os peleéis por el cursor. Link se ve limitado también para ti; para tenerlo completo, devuelve la PC (un clic).

### 4. Recuperar la PC

Cualquiera de estas cuatro:

1. **DEVOLVER LA PC** en la banda de arriba, y confirmar. Lo puedes pulsar tú o tu amigo; si lo pulsa él, solo se queda sin control.
2. **Ctrl+Alt+F12** en el teclado de tu PC. Solo funciona con el teclado físico: si tu amigo lo pulsa por Parsec, no pasa nada.
3. **Sola:** si tu amigo sale de la sala, si se cierra la sala, si la interfaz de Link deja de verse más de 10 s o si el guardián falla.
4. Si Link se cierra o se cuelga, tu amigo pierde el control al momento (Link es quien mete su teclado y su ratón).

Al recuperarla, tu amigo **pierde teclado y ratón** y en el chat sale «La PC vuelve al anfitrión (motivo)». Para dárselos otra vez, usa los permisos de siempre o cede de nuevo.

### 5. Cómo funciona por dentro (resumen)

- Parsec no mete la entrada en Windows por su cuenta. La recibe **Link** (`Hosting::pollInputs`) y la inyecta con `SendInput` (`InputControlService`). El filtro trabaja **antes de inyectar**, así que lo bloqueado nunca llega a Windows.
- Por eso no hace falta un gancho que mire `LLKHF_INJECTED` para bloquear. Ese indicador sirve para lo contrario: todo lo que inyecta Link llega marcado como inyectado, así que el atajo Ctrl+Alt+F12 solo se acepta si llega sin la marca, es decir, del teclado físico.
- Las piezas:
  - `phoenix/core/ModoCedido.*`: lógica pura (teclas, clics, lista blanca de acciones), probada en `pruebas-link/cedido_test.cpp`.
  - `phoenix/host/Cedido.*`: la parte de Windows (qué ventana es PES o Link, el guardián del foco, ABRIR PES, el atajo).
  - `phoenix/web/AccionesCedidoWeb.cpp`: las acciones `cedido.*` y el portero del puente, que devuelve `PC_CEDIDA` a todo lo que no sea de jugar.
- Cuenta como **PES**: `PES2021.exe`, el ejecutable del juego de la biblioteca y todo lo que esté dentro de la carpeta del juego (Sider, Settings…).
- Cuenta como **Link**: Link, sus procesos de WebView2 y el overlay PhoenixGlass.
- Para permitir otro programa, añádelo a mano en `phoenix-ui.json` → `"cedidoExtras": "ds4windows.exe"` (separados por comas, con Link cerrado).

### 6. Prueba con un amigo (pendiente)

1. Abre la sala. Tu amigo entra por Parsec.
2. Cédele la PC (paso 2) y quédate mirando sin tocar nada.
3. Pídele que pruebe:
   - ABRIR PES abre el juego.
   - Puede jugar y moverse por los menús de PES con teclado y ratón.
   - Puede volver a Link, repartir mandos y llevar el marcador.
   - La tecla Windows, Alt+Tab, clic en la barra de tareas y Ctrl+Shift+Esc no hacen nada.
   - Ajustes, Sync y Gente no aparecen.
4. Mueve tu ratón y escribe algo: tienes que poder usar la PC a la vez.
5. Pulsa Ctrl+Alt+F12: la PC vuelve a ti y él pierde teclado y ratón.
6. Repite con DEVOLVER LA PC, y luego haz que tu amigo salga de la sala: tiene que volver solo.
7. Prueba también a cederla a alguien que esté mirando (espectador).

Si algo falla, el modo se apaga solo y la PC vuelve a ti. Anota qué pasó y avisa en el chat LINK.

### 7. Límites honestos (lo que no cubre)

- **Dentro de PES y de Link** todo vale. Si un juego abriera un diálogo de Windows (abrir o guardar archivo), ese diálogo es de PES y se podría usar para ver carpetas. PES 2021 no tiene diálogos así, pero un lanzador o Sider con menús propios sí podrían.
- **Overlays dentro del juego:** el de Steam se abre con Shift+Tab, que está bloqueado. Si lo cambiaste a otra tecla, apágalo en Steam. El de NVIDIA (Alt+Z) es otra ventana: sus clics se bloquean, pero puede llegar a verse.
- **Atajos globales de otros programas** (Discord, OBS, NVIDIA…) pueden saltar con teclas normales que sí dejamos pasar, como Alt+Z o Ctrl+Shift+M. Si usas alguno, ciérralo antes de ceder o cambia su atajo.
- **Ctrl+Alt+Supr** no se puede mandar por Parsec (Windows no deja), así que tampoco hay que bloquearlo.
- **Ventanas de administrador** (por ejemplo, UAC): Link no puede escribir en ellas y el escritorio seguro tampoco recibe nada. Si aparece un aviso de UAC, la PC queda en pausa hasta que tú lo cierres.
- **Si no estás en la PC:** vuelve cuando tu amigo pulsa DEVOLVER LA PC o sale de la sala.
- **Jugar con teclado como mando:** mientras tu amigo tiene la PC, su teclado va a Windows (PES y Link) y deja de hacer de mando. Con su mando de verdad juega igual.
- **El chat de la sala:** lo que tu amigo escribe sale con tu nombre de anfitrión.
- **El modo quiosco original no abre el juego al hostear** (esa línea viene comentada en Smash Soda). Por eso ABRIR PES abre el juego de la biblioteca por su cuenta.
- **Pegar (Ctrl+V) en Link** está permitido: pega lo que haya en tu portapapeles.

---

## Parte 12 · Más de 8 mandos en PES 2021 — lo que hace Phoenix Link (0.3.41–0.3.42, EXPERIMENTAL)

> Origen: `docs/MAS-DE-8-MANDOS.md` · último cambio 06a673d 2026-10-21 · 5846 bytes. Copia íntegra.

**¿Ya se puede jugar con más de 8 mandos? Todavía NO del todo.** Link ya crea los puestos 9 a 16 y escribe el archivo
que PES necesita para ellos, pero PES solo los usará cuando esté instalado el **módulo de Sider de mandos extra**
(se hace aparte, en `D:\Frank\Games_\PRUEBA_CANCHA\modulo_sider`). Sin ese módulo, los puestos 9+ existen en Link
y en Windows, pero PES los ignora.

Complementa a `docs/ORDEN-MANDOS-PES.md` (el orden de los puestos 1–8 en settings.dat).

### Por qué hace falta un archivo nuevo

- `settings.dat` de PES tiene **8 casillas** de jugador (GUID del mando + mapa de botones). No se toca: sigue igual
  que con «Orden de mandos en PES» (casilla N = puesto N, del 1 al 8).
- El motor del partido tiene 24 ranuras y el gestor de mandos admite 32 (ver libro, `cap_60_k_estrategia_mas_mandos.md`).
- Para los puestos 9+ Link escribe **`phoenix_mandos_extra.dat`** junto a `settings.dat`:
  `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\phoenix_mandos_extra.dat`

#### Formato del archivo (versión 1)

| Posición | Tamaño | Qué es |
|---|---|---|
| 0x00 | 4 | `PHXM` (texto) |
| 0x04 | u32 | versión = 1 |
| 0x08 | u32 | N = cantidad de casillas |
| 0x0C | N × 0x40 | casilla i = puesto 9+i, **mismo formato que settings.dat**: GUID de instancia DirectInput (16 bytes) + 24 botones de 2 bytes (48 bytes) |

- Todo en little endian. GUID a cero = ese puesto está desenchufado.
- El mapa de botones es el de la casilla 1 de `settings.dat` (el mapa Xbox que ya usa PES); si está vacío, el Xbox de fábrica.
- Si bajas a 8 puestos y había archivo, Link lo deja con N = 0 (así el módulo no lee mandos viejos).
- Código: `SmashSoda/phoenix/core/MandosExtra.{h,cpp}`; se escribe desde `OrdenPes.cpp`.

#### Reglas (las mismas que settings.dat)

1. Solo con **PES cerrado** (y Settings.exe cerrado). Si está abierto, Link avisa y espera.
2. **Copia de seguridad** antes del primer cambio de cada sesión: `phoenix_mandos_extra.dat.phoenix-AAAAMMDD-HHMMSS`
   (si antes no había archivo, no hay nada que copiar).
3. **Escritura atómica**: se escribe `…phoenix-tmp` y se cambia de golpe; si algo falla, el archivo de antes queda intacto.
4. Solo funciona con **«Orden de mandos en PES»** encendido (es el mismo módulo, en Mandos › Puestos).
5. Link **no toca sider.ini** ni instala módulos de Sider.

### Cuántos mandos virtuales aguanta ViGEmBus (medido)

19-oct-2026, PC de Fralex, ViGEmBus 1.21.442: se pidieron 40 mandos Xbox 360 seguidos, tres veces.
Resultado: **21, 19 y 16** mandos a la vez (contando los que ya hubiera); el siguiente falla con `0xE0000007`
(TARGET_NOT_PLUGGED_IN: Windows no llega a enchufarlo a tiempo). Lo único seguro es **16**, por eso el tope de Link
es 16 y no 22. El programa de la medición crea los mandos, espera 3 s y los quita todos.

### Paso a paso (anfitrión)

1. Abre Link → **Mandos › Puestos**. Abajo, en la tarjeta **ORDEN DE MANDOS EN PES**, enciende el interruptor.
2. En la misma tarjeta, **PUESTOS MÁXIMOS**: súbelo de 8 a los que quieras (hasta 16). Sale la marca EXPERIMENTAL.
3. Ve a **Mandos › Tu mando › Mandos virtuales** (despliega la tarjeta) y sube **Xbox** (por ejemplo a 12). Desde 0.3.43 sube directo hasta 16 y «Puestos máximos» se ajusta solo.
   Hazlo con la sala cerrada; Windows tarda unos segundos en registrar cada mando nuevo.
4. Vuelve a **Puestos**: los puestos 9+ salen con la etiqueta **EXPERIMENTAL · Necesita el módulo de mandos extra**.
   Se meten en el Equipo A o B igual que los demás (botones A / B o «+ AÑADIR PUESTO»).
5. Con **PES cerrado**, la tarjeta pasa a «Ordenado» y muestra las casillas **PES 1–8** y **EXTRA 9–16** con ✓.
   Debajo: la copia de `settings.dat` y la de mandos extra.
6. Instala el módulo de Sider de mandos extra (va aparte) y abre PES.

Para volver a lo de siempre: Puestos máximos = 8 (Link quita los mandos que sobran, primero los de PlayStation).

### Qué cambió en Link (0.3.41)

- Ajuste nuevo `puestosMax` (8–16, por defecto 8) en las preferencias; acción `mandos.puestosMax{n}`.
- `mandos.cantidad` acepta hasta `puestosMax` mandos (antes 8 fijo).
- Equipos A/B hasta 16 puestos (`core/EquiposPuestos.h`): el mapa sigue siendo de 8 letras y solo crece si un puesto 9+
  tiene equipo, así las preferencias viejas siguen valiendo.
- Todas las vistas de la sala (árbitro, lados, partido, sala clasificatoria…) miran hasta 16 puestos; candado de
  puesto, «!cambio N» y «tomar puesto» aceptan hasta 16.
- Estado nuevo para la interfaz: `mandos.puestosMax|puestosTope|puestosPes`, `mandos.lista[].extra`,
  `ordenPes.casillas[].extra`, `ordenPes.copiaExtra` (ver `SmashSoda/phoenix/web/PROTOCOLO.md`).
- Interfaz: se amplió la de siempre (Puestos y Tu mando), sin rehacer nada.

### Pruebas

- `pruebas-link\correr_mandosextra.bat` → formato (firma, versión, N, casillas, GUID a cero, archivos no válidos),
  mapa de la casilla 1, escritura atómica (sin temporal sobrante; archivo bloqueado = queda intacto) y copia (una por
  sesión, con el contenido anterior). **TODO OK**.
- `pruebas-link/equipos_test.cpp` (compila con g++ o cl) → casos nuevos de 9 a 16 puestos. **todo bien**.
- Compilado con `cmake --build build --config Release` (nunca `COMPILAR_PHOENIX.bat`).
- Vista previa: simulador con 12 puestos: fila del 10 con la marca EXPERIMENTAL y casillas EXTRA 9–12.

### Lo que falta (fuera de Link)

- El módulo de Sider que lee `phoenix_mandos_extra.dat` y hace que PES use los mandos 9+.
- Probarlo en un partido real cuando termine la pausa de pruebas.

> Si tras compilar no ves «Puestos máximos»: la carpeta `x64\Release\ui` quedó vieja (la copia falla con Link abierto). Cierra Link y vuelve a compilar con `cmake --build`.

---

---

## Parte 13 · Mandos 9–16 DENTRO de PES: el módulo de Sider v0.9.58 (25-oct-2026, confirmado por Fralex)

**Qué se logró.** El frente que la Parte 12 dejaba abierto («falta el módulo de Sider que haga que PES use los mandos 9+») quedó **resuelto**: hasta **16 mandos a la vez** en un mismo PC (8 contra 8), con los mandos 9–16 **indistinguibles de los 1–8**. Fralex lo probó y lo confirmó en vivo el 25-oct.
- Los mandos 9–16 tienen jugador, flecha de color y lo **mueven** (v0.9.16, 23-oct).
- Menús con foco para 16: elegir equipo, estrategia y pausa (Local/Visitante y Estrategia) — el mando 9+ toma «prestado» un número libre del 0 al 7 entre la pantalla de lados y el saque (v0.9.28).
- **L2/R2** eligen el usuario de «Datos personales» (salta los usados), **□** edita «Datos personales» de su usuario, **○** vuelve atrás (v0.9.29–v0.9.56; el ○ del mando 16 llega a PES como botón 1, no 2: v0.9.40).
- **Capitán** por equipo: L3 los jugadores, F5/F6 el anfitrión (v0.9.31).
- El mando 9+ juega con el **usuario que eligió** (v0.9.57).
- **Tipo de cursor** (Datos personales / Nombre de jugador / Mando n.º / No) y **barra de potencia** también en los mandos 13–16, ampliando de 12 a 16 el dibujo de PES; un usuario sin nombre sale como «Usuario N» (v0.9.58). Antes de cambiar nada, el módulo comprueba byte a byte que el `PES2021.exe` sea el esperado; si no coincide, no toca nada.
- **Paquete instalable sin Phoenix Link** (regla: todo mod de PES también va como paquete suelto): `paquetes/phoenix_mandos_9a16/` en este repo (LEEME, `INSTALAR.ps1`, módulo, flujo de pausa). El zip completo está en la PC: `D:\Frank\Games_\PRUEBA_CANCHA\paquetes\phoenix_mandos_9a16_v0.9.58.zip`. **Lo que NO está en git:** `content\phoenix\lados_replica\*.raw` (≈9 MB, dibujos generados a partir de pantallas de PES; se regeneran con `modulo_sider\herramientas\generar_replica_lados.ps1`). El resto de versiones antiguas (zip por versión) y los respaldos `RESPALDO_v0.9.*.lua` están solo en la PC.

**Paso a paso para replicarlo (en el PC de Fralex):**
1. Cerrar PES. 2. Descomprimir el zip (o copiar `paquetes/phoenix_mandos_9a16/` de este repo y traer los `.raw` de la PC). 3. Ejecutar `INSTALAR.ps1 -Sider "D:\Frank\Games_\Conmegol Patch\SiderAddons"`. 4. En `sider.ini`: `luajit.ext.enabled=1`, `overlay.enabled=1`, `cpk.root=".\livecpk\PhoenixMandos"` como primera carpeta y `lua.module="phoenix_mandos_fase1.lua"` como último módulo. 5. Abrir PES; comprobar que `SiderAddons\content\phoenix\mandos\fase1.txt` empieza con `# fase1 v0.9.58`.

**Integrar en Link (pendiente, solo cuando el coordinador avise):** copiar `modules\phoenix_mandos_fase1.lua`, `content\phoenix\lados_replica\` y `livecpk\PhoenixMandos\` y poner lo de `sider.ini` de arriba. Link solo toca `sider.ini` con su instalador y con PES cerrado.

**Errores e intentos fallidos (resumen; la historia con direcciones está en `docs/libro-pes/cap_60_k_estrategia_mas_mandos.md` y `cap_60_n_diseno_16_asientos.md`):**
- 20–21-oct: el jugador del 9+ existía pero no respondía; inyectar «usuario 7» y copiar ajustes de cursor no lo movió (deshecho; el frente se cerró a v0.9.14).
- 23-oct: causa hallada con Frida y Ghidra: `match::Command` (`FUN_1408A9180`) solo daba entrada humana a mandos < 8 → parches a 16 (v0.9.16, funciona).
- Ampliar de golpe los 8 asientos del foco de los menús (v0.9.22) impidió arrancar PES → descartado; se sustituyó por el «número prestado» (v0.9.28).
- 24-oct: el ○ del mando 16 no se veía en la pausa porque llega como botón 1 y se esperaba el 2 (v0.9.40).

**Cosas a tener en cuenta:** el módulo debe ser el **último** de `sider.ini`; una persona no debe ocupar dos puestos a la vez (mando 1 de PES y un puesto 9+ de Link); los botones 9+ se leen con la distribución tipo Xbox de DirectInput; los cambios son solo en memoria (se pierden al cerrar PES; no se tocan archivos del juego ni guardados). Si PES se cierra al empezar o terminar un partido tras la v0.9.58, desactivar el HUD de 16 (aviso de Fralex al hilo de mandos).

**Lo que falta:** la **pantalla de Local/Visitante nativa de 16 filas** dentro de PES (hoy es una réplica encima de la de PES). Es lo siguiente (mañana 26-oct); diseño en `cap_60_n_diseno_16_asientos.md`. Candado: lo lleva la Cuenta 1 con acceso a la PC (ver `TRASPASO.md` de la web).

**Cómo comprobar y revertir:** comprobar con la línea de `fase1.txt` y jugando un partido con el mando 16 (pausa › Local/Visitante › ○; barra al cargar un tiro). Revertir: devolver `sider.ini.antes-phoenix-mandos-FECHA` como `sider.ini` o quitar las dos líneas del módulo y de `cpk.root`.
