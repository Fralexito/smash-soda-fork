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
