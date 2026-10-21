# Equipo A / Equipo B: elegir qué puestos son de cada equipo

12 de octubre de 2026. Frente LINK, Link 0.2.33. Estado: hecho, compilado y probado en el simulador y con pruebas de lógica pura; **falta probarlo con una sala real**.

## Qué pidió Fralex
«Corrige y da la opción de escoger el espacio de los mandos, y ahí en vez de decir Local / Visita que sea Equipo A / Equipo B.» Él quiere ser **siempre el mando 01** aunque juegue del lado de visita.

## Qué fallaba
- Link suponía que **los primeros N puestos eran Local** y el resto Visita, por número de puesto. El puesto 01 salía siempre como Local, aunque en PES jugaras de visita.
- Los lados reales los elige el juego, no el número de puesto.

## Cómo queda
- **Textos:** Local → **EQUIPO A**, Visita / Visitante → **EQUIPO B** en Puestos, Partido, selectores de mando, tutorial y ayudas.
- **Qué son A y B:** equipos **de Link**. No tienen por qué coincidir con local y visita del juego. A es el que estaba a la izquierda en el saque (según el Campo 2D).
- **Mapa por puesto:** `equipoPuesto` en `phoenix-ui.json`, ocho letras, una por puesto: `A`, `B` o `-` (fuera). Ejemplos: `AABB----` (el de siempre, 2 contra 2) y `BA------` (el 01 es del B y el 02 del A).
- **El número del puesto no cambia.** Es el mando virtual de Link: con «Orden de mandos en PES», el puesto N es el jugador N. No se renumera ningún mando.
- **Migración:** un `phoenix-ui.json` viejo (sin `equipoPuesto`) se convierte solo: los primeros `equipoLocal` puestos son A, hasta `mandosActivos` son B, el resto queda fuera. «Mi puesto al jugar» con `local` / `visitante` se lee como `a` / `b`.

## Cómo se usa (Mandos › Puestos)
- Cada fila trae **→ B** o **→ A** (PASAR A EQUIPO B / A): pasa ese puesto al otro equipo, con su número.
- **+ AÑADIR PUESTO** mete el primer puesto sin usar en ese equipo. **− QUITAR UNO** saca el último puesto libre de ese equipo.
- **PUESTOS SIN USAR**: lista los puestos fuera con botones **A** y **B** para elegir cuál añadir y a qué equipo.
- Cada equipo conserva al menos un puesto (no deja a un equipo vacío).
- **Mandos › TU MANDO › Mi puesto al jugar:** PUESTO FIJO, **1.º DE A** o **1.º DE B**.

## Caso clave: ser el 01 jugando del Equipo B
1. TU MANDO › Mi puesto al jugar → PUESTO FIJO 1.
2. Puestos → en el 01, **→ B**; en el 02, **→ A** si hace falta.
3. Resultado: tú en el 01 siendo del Equipo B, tu rival en el 02 siendo del A. Al pitazo Link arma `a` con los puestos del A y `b` con los del B, y te apunta a ti en el lado de tu puesto.

## Quién usa el mapa
`PhoenixPrefs` (`equipoDe`, `sincronizarEquipos`) · `EstadoWeb` (`equipo` de cada puesto: `a` / `b` / `fuera`; `formacion` `{a, b, mapa}`) · `Arbitro` (armado de lados al pitazo y apunte del anfitrión; HUD de la sala) · `MandoHost` (puesto libre) · `AccionesSala` (!equipo, jugador en espera) · `AccionesMandosWeb` (`mandos.equipo`, `mandos.miPuesto`, `mandos.cantidad`) · `AccionesSalaWeb` / `AccionesPartidoWeb` (formación y reparto de mandos de `partido.preparar`) · `TableroMandos` (tablero clásico) · web: `puestos.js`, `jugar.js`, `partido.js`, `sala.js`, `gente.js`, `tutorial.js`, `simulador.js`.

## Contrato con la web: no cambia
`partida_fin` sigue con los lados `a` / `b` y el booleano `local`; `mandos.formacion{local,visitante}` y los perfiles de sala conservan `local` / `visitante` (ahora cuentan los puestos del A y del B). Solo cambian textos visibles. Lo único nuevo: la acción `mandos.equipo{indice, equipo: a|b|fuera}` y el campo `formacion.mapa`.

## Pruebas
- `pruebas-link/equipos_test.cpp` (lógica pura: migración, mover puestos, caso 01 en el B, reglas, recortes): **todo bien**.
- Simulador (`?escenario=cruzado`): el 01 es del Equipo B (tú) y el 02 del A; mover puestos, añadir y quitar sin errores en la consola. `?escenario=llena` sin errores.
- Compilado con `cmake --build` (Release), sin errores.

## Cambios de 0.2.34: más intuitivo y candado de puesto
- **Arrastrar el puesto entre equipos:** se toma la tarjeta por su **número** y se suelta en el otro equipo (se ilumina «Suelta aquí…»). El botón «→ A» / «→ B» sigue como alternativa.
- **Puestos sin usar:** una bandeja con solo los números; se arrastran al equipo que se quiera.
- **Ayuda plegable:** «¿QUÉ SON A Y B?» en la cabecera de JUGANDO.
- **Candado de puesto (solo a quien tú fijes):** el icono de chincheta en la fila de un jugador **fija su puesto**. Cuando vuelva a la sala y tú lo aceptes como jugador (JUEGA), o él pulse su mando, cae en ese puesto; **si está ocupado, en el siguiente libre** (dando la vuelta, solo puestos de un equipo). Quien no tiene candado entra como siempre. Se guarda en `phoenix-ui.json` (`puestosFijados`, id de Parsec → puesto) y sobrevive a cerrar Link. En «Mirando» se ve «PUESTO 0N» junto a quien lo tiene fijado.
- Detalle técnico: acción `mandos.fijarPuesto{parsecId, puesto 0–8}` (0 = soltar), estado `mandos.fijados`; el gancho al pulsar el mando está en `GamepadClient::tryAssignGamepad` y la aceptación del host en `AccionesSala::decidirEspera`. Con turnos (hotseat) manda su propia lógica.

## Ajuste de 0.2.35 (a pedido de Fralex)
Se vuelve al aspecto de siempre de Puestos: la fila con su barrita de arrastre, sin bandeja de «puestos sin usar» ni recuadros en el número. **Se arrastra la fila entera a la columna del otro equipo** (se ilumina «Suelta aquí: el puesto 0N pasa a EQUIPO X») y el puesto se va **con su mando y su número**. Soltar sobre otra fila sigue intercambiando personas, como antes. Se quitaron los botones «→ A / → B». «+ AÑADIR PUESTO» mete el primer puesto libre en ese equipo.

## Ajuste de 0.2.36
- **Cruzar dos puestos:** arrastra la fila de un puesto **encima de una fila del otro equipo** y los dos puestos intercambian de equipo (el 5 del A y el 6 del B pasan a B y A), cada uno con su mando y su número. Si uno de los dos es **tu** puesto, se mantiene lo de antes: os cruzáis de asiento tú y tu amigo.
- **Botoncito ⇄ (muy sutil)** en cada fila: pasa ese puesto al otro equipo con un clic (se aclara al pasar el ratón; apagado si es el único puesto de su equipo).
- Acción nueva `mandos.equipoIntercambiar{a,b}` (permitida en modo cedido). La chincheta (candado de puesto) sigue igual y ahora se explica en «¿QUÉ SON A Y B?».
