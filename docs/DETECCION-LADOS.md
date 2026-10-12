# Detección automática del lado (local/visita) de cada mando

Frente LINK · 11 oct 2026 · estado: **método listo y probado en simulación; falta la prueba con PES y mandos reales.
Link todavía NO lo usa** (el árbitro sigue armando los lados por la formación de mandos y CORREGIR LADOS es a mano).

## 1. El problema

Al pitazo inicial Link arma los lados solo según la formación de mandos (`origenLados = "mandos"`: los primeros
`equipoLocal` mandos son el local). Pero en la pantalla de mandos del juego cada uno puede mover su icono al otro lado,
y entonces Link se equivoca hasta que alguien pulsa CORREGIR LADOS. El ranking 1v1 deja «por revisar» esos partidos.

Link no puede leer en qué lado puso cada uno su mando dentro del juego (no se conoce esa dirección de memoria),
pero sí ve dos cosas en vivo:

- el **stick izquierdo de cada mando** (`ProveedorSala::asientos()` → `AsientoVista.ejeLX/ejeLY`, lo que recibe el juego);
- la **posición de cada futbolista** (Campo 2D, solo lectura de memoria, ~20 lecturas/s; equipo A/B fijado en el saque de centro).

## 2. Método

### 2.1 Mando → equipo A/B (stick contra velocidad)

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

### 2.2 Equipo A/B → local/visita (el ancla)

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

## 3. Resultados

### 3.1 Simulación (`pruebas-link/lados_test.cpp`)

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

### 3.2 Con PES y mandos reales

_(pendiente: hace falta que Fralex juegue; ver §4)_. Lo que la simulación NO puede asegurar y la prueba debe mirar:

- que la velocidad que se lee del Campo siga al stick (retraso real, aceleración, regates, carreras con R2);
- cuántos votos se consiguen en los primeros 20-60 s reales (saques, pausas, repeticiones);
- si la orientación de cámara sale clara con la cámara que usa la liga;
- la hipótesis del saque inicial (§2.2-2).

## 4. Prueba con el juego (cuando Fralex dé permiso)

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

## 5. Integración en Link (solo cuando §3.2 salga bien en varios partidos)

En `web/Arbitro.cpp`, con el partido arrancado por los mandos (`origenLados == "mandos"`) y sin CORREGIR LADOS:

1. Durante los primeros 60 s de juego, encender el lector del Campo (`campo::pedirVivo`, como hace el mini-radar) y
   alimentar `lados::Detector` con la foto del Campo y los sticks de `sala.asientos(8)` (asiento = puesto).
2. En cada gol que dé `estado.json`, mirar la portería con `porteriaDelGol` + `LadoDefendido` → equipo A/B del local.
   (Si la hipótesis del saque se confirma, también al arrancar: equipo que defiende x < 0 en el primer tiempo = local.)
3. Con mandos decididos y ancla: si los mandos del lado A de Link juegan en realidad con el visitante (y todos los
   decididos están de acuerdo), `in.partido.corregirLados()` + `origenLados("juego")` y aviso en el chat
   («Link vio en el juego que X juega de visita: lados corregidos»). Si hay dudas, no tocar nada.
4. `origen = "juego"` ya cuenta como «lados confirmados» para la web (no deja el partido «por revisar»); confirmar
   con el chat WEB (`docs/PROMPT-WEB-ranking-1v1.md`) que lo acepta igual que una corrección a mano.

Coste: el escaneo inicial del Campo (~12 s de lectura de memoria a prioridad baja, como al abrir la tarjeta CAMPO) y
~20 lecturas/s de 16 bytes por jugador. Regla de Fralex: si en la prueba se notan tirones, no se integra.
