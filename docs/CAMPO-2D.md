# Campo 2D en vivo (Partido › Campo)

Muestra en Phoenix Link, en tiempo real, las fichitas de los jugadores de PES 2021 sobre un campo 2D, como el radar del modo entrenador.
Cómo se encontró todo (fuentes, estructura de memoria, pruebas): [CAMPO-2D-INVESTIGACION.md](CAMPO-2D-INVESTIGACION.md).
Para replicarlo paso a paso (humano o IA): [CAMPO-2D-REPLICAR.md](CAMPO-2D-REPLICAR.md).

## Cómo se usa

1. Abre **Partido › CAMPO** en Link.
2. Abre PES (el modo de ConmeGOL sirve) y empieza el partido.
3. Tarda unos 15 s en «encontrar» a los jugadores la primera vez (barre la memoria del juego una vez) y luego se mueven en vivo, ~20 veces por segundo.

Colores: cian = local, violeta = visitante, amarillo = asistentes de línea, gris = equipo aún desconocido. El balón es la bolita blanca.

## Qué hace Link (y qué NO)

- **Solo lee.** Abre PES2021.exe con permiso de lectura (`PROCESS_VM_READ`) y nada más: no escribe, no inyecta, no hookea, no pausa el juego, no puede desincronizar un partido.
- **Solo funciona con el ejecutable conocido.** Antes de leer comprueba tamaño y SHA-256 de `PES2021.exe` (hoy: 458 806 784 bytes, `5E27A782…BC224A`, el del parche ConmeGOL y SP2026). Con otro ejecutable la tarjeta dice «no compatible» y no toca nada.
- **Se apaga sola** si lo que lee no tiene sentido (jugadores fuera del campo, valores raros, más de 3 jugadores que dejan de ser válidos): vuelve a buscar con calma o se queda esperando.
- **Coste cero fuera de la tarjeta.** Con la tarjeta cerrada no hay hilo leyendo ni evento alguno. Con la tarjeta abierta: ~20 lecturas por segundo de 16 bytes por jugador.
- Si PES no está abierto, la tarjeta dice «Esperando a que empiece un partido en PES».

## Piezas

| Pieza | Dónde |
|---|---|
| Firma y validación (lógica pura, con pruebas) | `SmashSoda/phoenix/core/CampoVivo.{h,cpp}` · `pruebas-link/campo_test.cpp` |
| Lector del juego (hilo propio, Windows) | `SmashSoda/phoenix/host/CampoVivo.{h,cpp}` |
| Acción `campo.vivo`, evento `campo`, estado `campo` | `web/AccionesMandosWeb.cpp`, `web/InterfazWeb.cpp`, `web/EstadoWeb.cpp`, `web/PROTOCOLO.md` |
| Vista (SVG, interpolación suave) | `web/ui/js/vistas/campo.js` (pestaña CAMPO de Partido) |
| Simulador | `web/ui/js/simulador.js` (22 fichitas + balón + 2 asistentes en movimiento) |
| Prueba manual con el juego abierto | `pruebas-link/campo_vivo_real.cpp` |

## Pendiente

- **Equipo de cada fichita** (hoy salen en gris salvo los asistentes): se fija con el saque de centro.
- **Balón** (hoy la bolita no se dibuja con datos reales; en el simulador sí).
- ~~Mini-radar en el overlay~~ (descartado por Fralex: el campo va a la web).

## 0.2.21 — equipos, porteros, balón y tanda de penales

- **Colores por equipo**: A (cian) = el que ocupaba la izquierda en el primer saque de centro que vio Link; B (violeta); árbitros en amarillo. Como en el descanso se cambian los lados, A/B se mantienen por identidad (no por lado). Si abres Link a mitad de un tiempo, las fichitas van en gris hasta el siguiente saque (gol o descanso).
- **Balón**: la bolita blanca; se agranda un poco cuando va por el aire. Desaparece durante las repeticiones (el juego la aparca).
- **Porteros**: ya cuentan (24 fichitas: 22 + 2 árbitros… y el árbitro principal cuando el saque lo identifica).
- **Tanda de penales**: debajo del campo aparece la tanda en vivo: una fila por equipo con sus lanzamientos (✓ gol, ✗ fallo o parada), el marcador de la tanda y «A punto de patear: Equipo A/B». Los nombres de los lanzadores todavía no se leen del juego (pendiente).

## 0.2.22 — mini-radar en el overlay (RETIRADO: ver la sección siguiente; el campo va a la web)

- En **Partido › CAMPO** hay un interruptor «Mini-radar en el overlay». Encendido, Link lee el juego aunque esa tarjeta esté cerrada y manda el campo al overlay (WebSocket, evento `phoenix:campo`, ~10/s).
- En el **overlay › Widgets › Mini-radar** se enciende el widget y se elige la esquina (apagado por defecto). Se puede mover/redimensionar como los demás. Si dejan de llegar datos (juego cerrado, repetición larga) se esconde solo a los 4 s.
- Coste: lo mismo que la tarjeta (≈ 20 lecturas/s de 16 bytes por actor) y solo mientras el interruptor esté encendido.
- Código: overlay `frontend/src/components/overlay/widgets/OverlayRadarWidgetView.vue` + `models/widgets/RadarWidget.ts` (commit `2f740db` en `phoenix-overlay`, sin push); Link `web/InterfazWeb.cpp` (envío) y `PhoenixPrefs.radarOverlay`.

## 2026-10-11 · El campo va a la web, no al overlay

Fralex: «en el overlay no tiene que aparecer el campo 2D; debe aparecer en la web». El mini-radar del overlay (0.2.22, `campo.radarOverlay`) se quitó. Su interruptor ahora es **«Campo en vivo en la web»** (`campo.web`, preferencia `campoWeb`, apagada por defecto). Con un partido publicado en juego, `web/CampoWeb.cpp` manda el campo y la posesión a `POST /v1/liga/campo` como mucho 1/s, y la web lo enseña en el tablero que se abre al tocar «En vivo ahora» (`docs/PROMPT-WEB-tablero-partido.md`). El overlay ya no recibe `phoenix:campo`.

## 0.2.29 · Repeticiones, cinemáticas y pausas (fase del campo)

Problema (Fralex, repetición de un gol): las fichitas desaparecían. Causa: durante la repetición el juego aparca el balón (z = 100) y las posiciones que se leen son de la película, y si dejaban de leerse el lector se daba por perdido y volvía a buscar (~20 s «sin señal»).

- El lector clasifica cada lectura en una **fase** (`FotoCampo.fase`, evento `campo` campo `f`, solo si ≠ vivo; en `POST /v1/liga/campo` siempre como `fase`):
  - `vivo`: juego en directo.
  - `repeticion`: el balón lleva ≥ 0,5 s aparcado (|z| > 40 en todos los candidatos). Se **congelan las últimas posiciones buenas**, el balón se oculta, no se asignan equipos ni se cuenta posesión ni se alimenta la tanda. Vuelve a `vivo` en la primera lectura con balón en juego (< 0,1 s; si hay salto grande las fichitas se colocan sin deslizar).
  - `otro`: más de 3 fichitas dejaron de leerse; se congela igual y se reintenta hasta 30 s antes de volver a escanear (antes: escaneo inmediato).
  - `pausa`: todo quieto ≥ 4 s; solo informativo (posiciones reales).
- Tarjeta Campo: etiqueta sobre el campo «REPETICIÓN», «SIN LECTURA EN VIVO» o «PAUSA». El simulador muestra «REPETICIÓN» 4 s cada 32 s.
- Web: el tablero debe hacer lo mismo con `fase` (≠ `vivo`: congelar y mostrar la etiqueta). La posesión no se calcula fuera de `vivo`.
- Lados automáticos no reciben lecturas de `repeticion`/`otro`.
- La celebración de gol no tiene fase propia: si el balón sigue en juego (|z| ≤ 40) se ve como `vivo` con las posiciones reales; si el juego lo aparca, como `repeticion`. No verificado con partido real (pendiente probar con una repetición y celebración en vivo).

## 0.2.30 · Colores de camiseta por equipo

Fralex: «adapta el color de cada equipo según su color referente». Cada equipo se pinta con el color de su camiseta principal, sacado del **nombre del equipo que da el juego** (`phoenix_referee`: `nombreLocal`/`nombreVisita`).
- Tabla en `core/ColoresClub.{h,cpp}` (≈100 entradas: selecciones de CONMEBOL y grandes, clubes peruanos, sudamericanos y europeos; color principal + secundario; busca por trozo del nombre sin tildes, y por palabra entera si la clave es corta: «usa», «roma», «inter»). Prueba: `pruebas-link/colores_test.cpp`.
- Reglas: un color demasiado oscuro (negro, azul marino) se sustituye por el secundario o por gris azulado; si los dos equipos quedan parecidos (distancia RGB < 110) el segundo usa su secundario y, si sigue parecido, el violeta/cian/amarillo más lejano; nombre desconocido = cian (A) / violeta (B) de siempre. Árbitros siguen amarillos.
- **A = local del juego** (se asume; si Link detectara que A es en realidad el visitante, los colores irían cruzados: es el mismo asunto abierto que «lados» y «eq_local»).
- Protocolo: el evento `campo` y `POST /v1/liga/campo` traen `c:["#rrggbb","#rrggbb"]` (A, B) solo si al menos un nombre se reconoce. Tarjeta de Link: fichitas y leyenda con esos colores. Web: Edge `liga` 1.3.1 (v5) deja pasar `c` y el tablero (`features/liga/campoVivo.js`) los usa (repo `pes-liga-automatica`, commit `7f2b9c2`).
- Para añadir un club: una línea en `kTabla` (clave normalizada en minúsculas sin tildes, principal, secundario) y un caso en la prueba.

## 0.2.31 · «Viendo desde la web» en la sala

Fralex: al tocar un partido en la web se ve el tablero (y las estadísticas) sin entrar a la sala; quien quiera puede entrar; y en la sala debe verse quién mira desde la web.
- **Web** (repo `pes-liga-automatica`, ronda 237): el tablero de «En vivo» / «Ahora mismo» / Inicio tiene el botón **Entrar a la sala** (lee `salas_enlace`, que la base solo da a quien puede ver esa sala) y un panel de estadísticas. Con sesión, avisa cada ~30 s que alguien mira (tabla `liga_mirando`, migración 104; el nombre sale del perfil en el servidor).
- **Link:** el hilo de la liga pregunta cada ~10 s `GET /v1/liga/mirando?sala_id=` (Edge `liga` 1.4.0), solo con el marcador de la sala publicado. Resultado: (1) lista «Viendo desde la web» en **Gente**, (2) un mensaje del bot en el chat de la sala por persona («X está viendo el partido desde la web.»), que se vuelve a decir si se va y vuelve. Código: `link/PhoenixLink.cpp` (`mirandoWeb()`), `web/InterfazWeb.cpp`, `web/EstadoWeb.cpp` (`mirandoWeb`), `ui/js/vistas/gente.js`.
- Límites: los visitantes sin sesión no aparecen (no hay nombre que mostrar). Los espectadores de la web no entran al Parsec salvo que pulsen «Entrar a la sala».

## 0.2.37 · Poseedor del balón y retraso del campo (medición y arreglos)

Fralex: «a veces no aparece marcado el jugador que tiene el balón» y «a veces se tarda». Método: medir cada tramo, atacar el que más aporta, probar la lógica con pruebas puras y verificar con un partido real.

### Tramos del retraso y qué se encontró (por lectura del código; las cifras reales se rellenan con el diagnóstico)
| Tramo | Antes | Ahora |
|---|---|---|
| Lector de memoria | `dormir(50)` en rebanadas de 25 ms con el temporizador de Windows a 15,6 ms → cada pasada de ≈ 62 ms (≈ 16 lecturas/s) con tirones; `reloj` sumaba 0,05 fijo | `timeBeginPeriod(1)` + horario fijo de 33 ms (≈ 30 lecturas/s) y reloj real |
| Publicación a la interfaz de Link | evento ≤ 20/s, atado al fotograma de Link | evento ≤ 30/s |
| Suavizado en la interfaz | 70 ms | 45 ms |
| Envío a la web | 1 mensaje/s, y cada envío pasaba por 3 consultas en la Edge (≈ 100–300 ms) | 1/s sin espectadores con sesión, **4/s si alguien mira** (liga_mirando); la Edge guarda en caché la PC (60 s) y la sala (30 s) |
| Suavizado en el navegador | 0,3 s fijo | ≈ medio intervalo de llegada (60–500 ms) |

El tramo que más aportaba era el **envío a la web a 1/s** (el tablero siempre iba ≥ 1 s por detrás por diseño). Un envío a 4/s lo deja en ≈ 0,25–0,4 s + red.
Coste: cada mensaje es una invocación de la función (plan FREE ≈ 500 000 al mes). Por eso solo se sube a 4/s mientras alguien mira con sesión; un visitante sin sesión ve 1/s.

### Diagnóstico visible
- **Link › Partido › CAMPO** (bajo el campo): lecturas del juego por segundo y duración de la pasada (media y máxima = tirones del lector), retraso del campo hasta la interfaz (hora de la lectura en el evento `ts` frente al reloj de la interfaz; mismo PC, así que es exacto), eventos/s, envíos a la web por segundo, ms por envío, fallos y cuántos miran.
- **Web:** añadir `?diag` a la dirección del tablero enseña mensajes/s y retraso desde la lectura (`ts`; solo es exacto si el visor y el host tienen la hora bien puesta).

### Poseedor del balón (`core/CampoVivo` → clase `Poseedor`)
La heurística vieja (el más cercano a ≤ 2 m, calculada solo para la posesión de la web) no marcaba nada en el campo y fallaba con balón alto, pases y disputas. Reglas nuevas, con histéresis:
- Lo **adquiere** el más cercano a ≤ 1,4 m, con el balón a ≤ 2,4 m de altura y sin ir lanzado (> 13 m/s), sostenido ≥ 80 ms.
- Lo **conserva** mientras esté a ≤ 2,8 m; si se aleja, el balón sube o sale disparado, lo sostiene 350 ms y luego el balón queda **libre** (-1).
- **Disputa:** otro se lo quita solo si está a ≤ 1,0 m y el poseedor ya está a más de 1,8 m. Dos casi empatados no parpadean.
- Sin balón visible (repetición/pausa) conserva 350 ms y luego libre. Árbitros y asistentes no cuentan.
- Se dibuja un **halo blanco** sobre el poseedor en Link y en la web; el protocolo lleva `p` (índice en `j`; -1 = libre; ausente = Link viejo). La posesión % de la web usa ahora al poseedor.
- Casos cubiertos por `pruebas-link/campo_test.cpp` (sintéticos): regate sin parpadeo, pase y recepción, balón por el aire, balón libre, disputa con histéresis, repetición, árbitros, cambio de número de fichas.
- **No resuelto con datos reales todavía:** si PES guarda en memoria quién controla el balón (un puntero o bandera), sería mejor que la heurística; se busca con un partido real (ver abajo). Mientras tanto, el estado «libre» y el halo salen de la geometría.

### Pendiente de verificar con un partido real (3–5 min contra la CPU)
Comparar captura del juego frente al tablero (balón en el aire, saques, córners, repeticiones, pausa y vuelta al juego) y rellenar las cifras de retraso medidas.

## 0.2.38 · Porteros y estadísticas del partido

### Porteros
Fralex: «no me reconocía los porteros». Los volcados reales (`ko_entes.txt`, `entes_raw.txt`) enseñan que el juego guarda **dos copias de cada portero** (a 640 bytes una de otra, `+312 = 1`, `+492 = 0x80001`) y que, hasta ahora, Link cogía la de menor dirección y un solo portero por lado. Si esa copia es la que se queda quieta en el sitio del saque, el portero salía parado (o no salía). Ahora, por cada lado, Link lee **todas las copias** y se queda con la que **se mueve** (cambia solo si la elegida lleva > 1 s quieta y la otra se movió hace < 0,4 s: sin parpadeo). También escribe `phoenix-campo-escaneo.txt` en la carpeta temporal (jugadores, copias de porteros con su posición y dirección) para diagnosticar sin volver a abrir el juego, y la tarjeta CAMPO enseña «porteros: N/2 (X copias)».
Aún **no verificado con un partido real**: los volcados antiguos solo traen las copias del portero izquierdo; el derecho no aparece en ellos. Si con partido real sale «porteros 1/2», pásame `phoenix-campo-escaneo.txt`.

### Estadísticas derivadas (`core/CampoVivo` → `Estadisticas`)
El juego no deja leer sus estadísticas (la pantalla «Datos del jugador» solo existe al final del partido y no se encontró su almacén en memoria). Se calculan con las posiciones y el poseedor del balón; son **aproximadas** y se rotulan así:
| Estadística | Cómo se calcula |
|---|---|
| Posesión | segundos con el balón por equipo |
| Pases completados | el balón pasa a otro jugador del mismo equipo en ≤ 3 s |
| Pérdidas / recuperaciones | pasa al otro equipo en ≤ 3 s |
| Tiros / a puerta | balón lanzado > 14 m/s hacia la portería rival, a ≤ 32 m, que apunta a |z| < 7 (a puerta: < 4,2 al llegar a la línea); un pase largo muy tenso hacia el área puede contarse como tiro |
| Córners / saques de puerta | el balón sale por la línea de fondo (no es gol): si lo tocó el defensor, córner del rival; si lo tocó el atacante, saque de puerta |
| Saques de banda | sale por la banda: saque para el equipo que no lo tocó |
El lado que defiende A se saca de dónde está su portero (cambia en el descanso). Pruebas sintéticas en `pruebas-link/campo_test.cpp` (posesión por tiempo, pase, pérdida, tiro/desvío, córner, saque de puerta, banda, gol, lado desconocido).
**No se pueden leer todavía:** faltas, tarjetas amarillas y rojas, cambios (quién entra y quién sale), asistencias, fueras de juego. Opciones: (a) un módulo de Sider que observe los archivos de escena que carga el juego (`foul_pk_card_y01`…) — requiere tocar `sider.ini` con PES cerrado, decisión de Fralex; (b) buscar el almacén de estadísticas en memoria comparando antes/después de un evento con el juego congelado (necesita partido real).
Protocolo: el evento `campo` y `POST /v1/liga/campo` traen `est:{pos,pas,per,rec,tir,pue,cor,ban,met}` (pares [A,B]); la web lo pinta en una tabla bajo el campo (Edge `liga` 1.6.0; repo web, commit `456d58e`).

## 0.2.39 · Prueba de las dos vías para faltas, tarjetas y cambios

Fralex: «probemos ambas cosas para encontrar lo más eficiente». Hallazgos antes de probar (leyendo la instalación de ConmeGOL, solo lectura):
- Sider es la 7.3.4. El parche trae `modules\FoulScenes.lua`, que engancha el **selector de escenas** del juego y recibe en cada escena `RCX` = código de escena y `RBX` = lado (0 = visita, 1 = local). Códigos que usa: `0x0002000F` amarilla, `0x00020010` roja, `0x00020011`/`0x00020014` falta sin tarjeta, `0x00020012` aviso, `0x00020013` roja (alt.), `0x00050001` cambio real, `0x00050006` cambio (VAR). Además el juego pide por nombre los archivos de escena (`foul_pk_card_y01.fdc`, `foul_pk_card_r01.fdc`…) y `FoulScenes.lua` los intercepta con `livecpk_rewrite`.
- `FoulScenes.lua` solo está activo en el sider.ini del modo **ConmeGOL Patch 26**, no en el 27 (el modo actual de Fralex).

Dos vías, ambas **solo lectura** y metidas en `phoenix_referee.lua` v1.3 (el módulo que Link ya instala; no se toca sider.ini):
| Vía | Qué anota | Alcance |
|---|---|---|
| **A · nombres de escena** (`livecpk_rewrite`) | cada archivo «demo/foul/card/corner/throw/offside/change/subst/injur/penal/kick/var_» que el juego pide, con la hora | cualquier parche con Sider 7.3.x (también el modo 27) |
| **C · selector** (evento `custom:pk_var_supersub_redirect`) | código de escena + lado (RBX) | solo si `FoulScenes.lua` está activo (modo 26) |
Ambas escriben `<juego>\SiderAddons\content\phoenix\escenas.log` (`hora|archivo` o `hora|SEL|0xCÓDIGO|rsi=..|rbx=..`), se vacía en cada partido y se detiene a las 4000 líneas.
**Cómo se prueba:** (1) cerrar PES; (2) en Link, SYNC › Puente › Módulos del juego → actualizar (copia el nuevo `phoenix_referee.lua` en la raíz y en cada modo, con copia de seguridad); (3) jugar 5 min contra la CPU provocando faltas, una amarilla, una roja, un cambio, un córner, un saque de banda y un fuera de juego, anotando la hora de cada uno; (4) leer `escenas.log` y cruzar con las horas anotadas.
**Cómo se decide la más eficiente:** la que dé nombre de evento fiable para los 3 casos (amarilla/roja/cambio) con menos falsos positivos, sin leer memoria y en todos los modos. Si A solo da los archivos pero no el lado, el equipo se infiere con la posesión (el que cometió la falta suele ser el que no llevaba el balón); si C da el lado, se usa C donde exista.
Vía de memoria (buscar el almacén de estadísticas comparando antes/después) queda como tercera opción, solo si ninguna de las dos basta: necesita partido real y comparar volcados de varios GB.

## 0.2.40 · Equipos que juegan, color de club y dorsales

- **Quién contra quién:** la tarjeta CAMPO pone «FC BARCELONA VS BAYERN…» (nombres que da el juego en `estado.json`; A = local) y la leyenda usa esos nombres. El color de cada equipo ya salía de la tabla de camisetas (0.2.30).
- **Dorsal en cada ficha: NO se puede todavía.** Cada actor del campo (objeto de 640 bytes) no trae su dorsal ni su jugador: se comprobó con los volcados reales (`ko_entes.txt`): en los 22 actores de una lista, ningún campo es un entero 1..99 distinto por jugador; solo hay posiciones, punteros y banderas. Por eso las fichas no pueden llevar número ni nombre con lo que se lee hoy.
- **Herramienta para encontrarlo** (`tools/radar/cadena.cpp`, solo lectura): escanea como Link, toma los 22 actores y sigue los punteros de cada uno hasta 2 niveles buscando (a) enteros 1..99 distintos entre jugadores (dorsal) y (b) textos de ≥ 4 letras distintos (nombre). Imprime las rutas «+off1 → +off2» que mejor separan a los jugadores. Se ejecuta con el partido en juego (`cad\cadena.exe`) y no pausa ni escribe nada.
- **Plan B si la herramienta no encuentra nada:** inferirlo por formación. Al saque de centro cada jugador está en su casilla de la formación; la alineación (orden de titulares) y los dorsales salen de los datos del equipo (Phoenix-DB / Player.bin con los ids de equipo `local`/`visita` de `estado.json`). Hay que casar casillas con posiciones, y los cambios mueven los dorsales: solo servirá con la alineación inicial y los cambios que se lean con `escenas.log`.

## 0.2.76 · Cartel de repetición/pausa por el reloj del juego (y medición de movimiento)

Fralex: «que muestre cuando es repetición y cuando está en pausa» y «hay inconsistencias de movimiento y posición exacta».
- Antes la pausa solo se notaba si **todas** las fichas estaban casi quietas (< 2 cm en 2,7 s), pero en pausa el juego sigue moviendo animaciones y nunca se cumplía; la repetición solo se detectaba si el balón estaba aparcado.
- Ahora Link también usa el **reloj del juego** (`estado.json`: `relojCorre`, `fase`): si el reloj lleva > 0,8 s parado con partido en curso, el lector **congela las últimas posiciones en vivo** y publica la fase `detenido` («REPETICIÓN O PAUSA»), o `descanso` («DESCANSO»). Al volver a correr el reloj, vuelve solo a vivo. Sin datos frescos del juego (estado.json > 5 s) no se congela nada. Protocolo y web: Edge `liga` 1.6.1 y tablero (commit web `005b16d`).
- **Medición** (`tools/radar/medir.cpp`, solo lectura): con el partido en juego mide el ritmo real de cambio de posición de cada actor, saltos imposibles y cuánto se parecen/desfasan las dos listas de jugadores del juego (en los volcados viejos difieren ≈ 0,5 m). Con esas cifras se decide si Link debe leer la otra lista, promediar o suavizar. **Pendiente de un partido real.**
- Los porteros siguen pendientes de partido real (ver 0.2.38: «porteros N/2» en el diagnóstico).
