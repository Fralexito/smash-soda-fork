# Tu mando: jugar con tu propio mando como un puesto más (anfitrión)

11 de octubre de 2026. Frente LINK. Estado: hecho y probado en el simulador; **falta probarlo con un mando real**.

## Qué fallaba
- «Tomar mando» (Ctrl+Alt+número) **bloqueaba** el puesto y le mandaba el **teclado** unas 60 veces por segundo.
- Las Marionetas (maestro y títeres) copiaban tu mando físico **a ese mismo puesto** a la vez.
- Los dos se pisaban, y por eso:
  - el puesto salía «bloqueado»;
  - tu mando no movía nada;
  - solo podías cambiarte de puesto con Ctrl+Alt.
- La pantalla de maestro y títeres era confusa.
- Tú no aparecías en Puestos, así que no podías arrastrarte ni pasarte a «Mirando».

## Cómo queda
- **Mandos › TU MANDO**: eliges una vez el mando con el que juegas (o «Sin mando: teclado»).
- **Mandos › Puestos**:
  - Apareces como **TÚ**: en «Mirando» si no juegas, o en tu puesto si juegas.
  - Te **arrastras** a cualquier puesto libre, o eliges **PONERME EN…**. Tu mando mueve ese puesto, **sin bloqueo**.
  - Para mirar, te arrastras a **Mirando** o pulsas **QUITAR**.
  - **TODOS A ESPECTADOR** (arriba) pasa a todos a mirar. Sustituye al viejo «QUITAR TODOS», que desenchufaba los mandos.
- Por dentro: es una marioneta de **un solo puesto** (el tuyo), que cambia sola cuando te mueves.

## Paso a paso
1. Cierra DS4Windows.
2. Conecta tu mando y ve a **Mandos › TU MANDO**. Pulsa un botón: se enciende su luz. Pulsa **USAR ESTE**.
3. Si los botones salen cambiados, pulsa **¿BOTONES CAMBIADOS?** (cambia la lectura entre Xbox y PlayStation).
4. Abre la sala. En **Mandos › Puestos**, arrástrate (TÚ) al puesto que quieras.
5. Para que PES no lea también tu mando por su cuenta (y no manejes dos jugadores), enciende **ORDEN DE MANDOS EN PES** en esa misma pestaña. Ver `ORDEN-MANDOS-PES.md`.

## No abras Settings de PES para tu mando
Con el orden de mandos en PES, **tu mando no va en Settings**: PES solo ve los 8 mandos de Link y tu mando entra a través de tu puesto. Si Settings muestra mandos «XBOX», son los de Link (lo normal).

## Para sacar a alguien de un puesto
- Botón **QUITAR** (rojo) en su fila, o arrástralo a **Mirando**.
- Para sacarlo **de la sala**, ve a Gente.

## Sentarte solo (desde 0.2.17)
En **Mandos › TU MANDO › Mi puesto al jugar** hay dos interruptores (los dos encendidos por defecto):
- **Empezar sentado**: al abrir la sala ya estás en tu puesto (el que elegiste: fijo, 1.º de Local o 1.º de Visita). Si no quieres jugar, arrástrate a **Mirando**.
- **Entrar pulsando tu mando**: si no estás en un puesto, pulsa **A, B, X o Y** (✕ ○ □ △) y entras. Si tu puesto está ocupado, vas a otro libre de tu equipo; si no, a cualquiera libre. **Nunca saca a nadie**: si todo está lleno, no pasa nada.

Link recuerda tu mando: al volver a abrirlo, el que elegiste con «USAR ESTE» queda elegido solo cuando está conectado. Con teclado no hay entrada automática: usa el botón **JUGAR** en Mirando.

Los invitados funcionan igual: un jugador autorizado (aceptó el reto en la web, lo aceptaste tú o ya lo habías sentado) pulsa A/B/X/Y y cae en un puesto libre. A quien mandaste a Mirando no entra solo: te sale como «quiere jugar».

## Salir como jugador en la web (desde 0.2.23, automático desde 0.2.24)
Antes, si jugabas tú con tu mando de Link, el partido que Link mandaba a la web **no te incluía**: contra un invitado parecía «invitado contra la CPU» y no puntuaba. Ahora es **automático**, sin tocar nada:
- **Sentado en un puesto:** al pitazo inicial Link te apunta en el lado de tu puesto (1.º–«equipoLocal» = Local, el resto = Visita).
- **Sin puesto, pero jugando directo en PES:** si Link ve que tocaste tu mando en los 2 minutos antes del pitazo (cuando eliges equipos en los menús), te apunta en tu lado habitual, el de «Mi puesto al jugar» (puesto fijo, 1.º de Local o 1.º de Visita).
- **Si solo miras y no tocas tu mando:** no sales.
- Interruptor «Contarme como jugador en la web (automático)» en **TU MANDO › Mi puesto al jugar**, encendido por defecto. Apágalo si nunca quieres salir sin puesto.
- Funciona al armarse los lados solos al pitazo. Si ya los elegiste a mano en Partido › Armar, ahí mandas tú.
