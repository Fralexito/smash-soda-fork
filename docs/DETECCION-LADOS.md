# Detección automática del lado (local/visita) de cada mando

Frente LINK · 11 oct 2026 · estado: **integrado en Link 0.2.25 (interruptor «LADOS AUTO» en Partido, encendido por defecto);
probado en simulación; falta la prueba con PES y mandos reales** (§4). Ante la duda no toca nada y queda el botón CORREGIR LADOS.

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

## 5. Cómo está integrado en Link (0.2.25)

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

## 6. Partido › Armar sin clics (0.2.42)

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
