# Investigación profunda de ideas para el parche y el online (12 nov 2026, tercera ronda)

Para Frank. Español sencillo. **Nada de esto está instalado, descargado ni probado en tu PC.** Es solo investigación.

No repite las 71 ideas de `IDEAS_FUTURO_12NOV.md`, las 19 de `IDEAS_NUEVAS_13_12NOV.md` ni las 12 de `BUSQUEDA_PROFUNDA_12NOV.md`. Antes de escribir cada idea comprobé con búsqueda de texto que no estuviera ya en esos documentos ni en los capítulos de `para-proyecto/pes/` (cap_62, cap_70, cap_71, cap_76).

## 1. Cómo se hizo y qué límites tiene

- Unas 45 búsquedas y unas 60 páginas leídas enteras. Fuentes: la documentación oficial de Sider 7.4 (GitHub de juce), pesmodding.com (etiquetas de módulos, herramientas y jugabilidad, 2 páginas de cada una), pes-files.ru y pes-files.com (comunidad rusa), evoweb.uk, GitHub (4chan Cup, pes-modding, repos sueltos), la web de PESBUL, el repositorio de AzrakHQ (cambios hasta hoy) y sus notas que ya tenemos copiadas.
- **Novedad buena:** hoy pesmodding.com y pes-files sí se abrieron desde la nube. evoweb.uk se abrió al principio (leí 4 páginas) y luego empezó a rechazar las visitas (error 403). No intenté saltarme ese bloqueo. Lo que quedó sin leer está en la sección 8, para leerlo desde tu PC.
- **Otros idiomas:** en ruso salió material útil (pes-files.ru). En chino, japonés, portugués e indonesio el buscador devolvió casi solo guías viejas de Sider o copias de pesmodding. No encontré comunidades escondidas con herramientas propias en esos idiomas. Eso es un resultado honesto, no una falta de búsqueda.
- Marcas de cada idea:
  - **[leído]** = abrí la página y lo que digo sale de ahí.
  - **[buscador]** = solo vi el resumen del buscador.
  - **[propia]** = propuesta mía apoyada en lo leído.
- Esfuerzo: **bajo** = una tarde, **medio** = unos días, **alto** = semanas.
- Todo lo que es un mod de otra persona se **estudia** para aprender cómo lo hace y luego se escribe uno propio. No se copia ni se redistribuye. Las descargas necesitan tu SÍ.

---

## 2. Las 12 que haría primero

| # | Idea | Para qué | Impacto | Esfuerzo | Sección |
|---|---|---|---|---|---|
| 1 | Estudiar **eFutsal v9.4**: fútbol 5 contra 5 hecho solo con Sider | Es la prueba de que tu modo Street se puede hacer sin tocar el exe | Muy alto | Bajo (leerlo) | 4-A |
| 2 | Subir los tiempos de conexión con **`proto_opt`** | Menos «No se pudo conectar» el día de la prueba con tu amigo | Alto | Bajo | 3-A |
| 3 | **Grabar los partidos en el servidor** (`upload: "YES"`) | Ver partidos en vivo, repeticiones y prueba en disputas | Alto | Medio | 3-B |
| 4 | **Un identificador por amigo** dentro del lanzador | Quita el riesgo de «dos PCs parecen un solo jugador» | Alto | Medio | 3-C |
| 5 | Buscar el aviso de «máximo» **desde el texto** con STR Editor | Llegar directo a la función que lo muestra, sin más pruebas a ciegas | Alto | Bajo | 7 |
| 6 | **Final con ceremonia** en un partido suelto (Custom Exhibition Match) | La final de la Galaxy League con trofeo y narración de final | Alto | Bajo | 4-B |
| 7 | **Textos del juego con tu marca** (STR Editor + LiveCPK) | Que el menú diga «Phoenix Online» y «Galaxy League» | Alto | Bajo | 4-C |
| 8 | **Perfiles de jugabilidad** encender/apagar (dt18 con 480 valores explicados) | Arcade, Simulación y Street como opciones, sin tocar el exe | Alto | Medio | 4-D |
| 9 | **Compilar el parche con un comando** (Unlimited Tool Works) | Tu parche gigante se arma solo, sin pasos a mano | Alto | Medio | 5-A |
| 10 | **«Salas en vivo»** en la web | Ver quién busca rival ahora y entrar a jugar | Alto | Bajo | 6-A |
| 11 | **Jornada offline con resultado automático** (idea de MatchUP) | Liga contra la CPU o Carrera con el resultado leído del guardado | Alto | Alto | 4-E |
| 12 | **Touchline** como modelo para Phoenix Carrera | Es el compañero de Liga Máster más completo que existe, y es gratis | Alto | Bajo (leerlo) | 6-B |

---

## 3. Online

### 3-A. Subir los tiempos de conexión con `proto_opt` [leído]
- **Qué es:** el juego tiene una lista de ajustes de red (tiempos de espera, cuándo pasa de conexión directa a TURN) que **no vive en ningún archivo**: la mandaba el servidor de Konami dentro de `CMD_GET_SERVER_ENV`, en un campo de texto llamado `proto_opt`. Si el servidor no lo manda, el juego usa sus valores de fábrica.
- **Dato clave:** tres tiempos de espera valen 15 segundos de fábrica. El juego reintenta una vez, así que a los 30 segundos se rinde. Con dos PCs lejos y por Tailscale, 15 segundos puede ser poco.
- **Fuente:** `referencia_azrak_mit/notes/re/functions/nettune.md` (ya lo tenemos copiado; licencia MIT). Ahí están los nombres exactos de cada ajuste.
- **Impacto:** alto para la prueba con tu amigo. **Esfuerzo:** bajo.
- **Cómo probar:** mandar `proto_opt` con esos tres tiempos en 30 segundos y mirar en el registro del servidor cuánto tarda ahora en rendirse el emparejamiento contra el rival falso. Si pasa de 30 a 60 segundos, el juego lo aceptó.
- **Ojo:** en `TABLERO_ONLINE.md` no vi que el hilo Online lo haya aplicado. Si ya lo hizo, esta idea se tacha.

### 3-B. Grabar todos los partidos en el servidor [leído]
- **Qué es:** el juego sabe grabarse solo. Si el servidor contesta `upload: "YES"` en `CMD_GET_MATCHID`, cada jugador sube el partido en trozos cada 5 segundos. Con eso se consiguen tres cosas: ver un partido en vivo desde otro PES, guardar la repetición y tener una prueba cuando hay una disputa.
- **Fuente:** `referencia_azrak_mit/notes/re/functions/spectate.md`. La nota dice que funcionó en vivo el 6-nov-2026. El README de AzrakHQ dice que guarda las 20 repeticiones más nuevas y se ven en «Torneo en línea → Repeticiones de partidos»: https://github.com/AzrakHQ/pes2021-online-server
- **Impacto:** alto. **Esfuerzo:** medio (el código de referencia es MIT).
- **Cómo probar:** activar `upload: "YES"` solo para un partido de prueba y mirar si llega `CMD_SEND_GAME_SEQUENCE_DATA`. Medir cuánto disco ocupa un partido entero antes de dejarlo siempre encendido. Necesita dos jugadores reales para la parte de «ver».
- **Interruptor:** debe ir apagado por defecto y encenderse desde la configuración.

### 3-C. Un identificador por amigo [leído]
- **Qué es:** AzrakHQ cambió su sistema el 8-nov: cada amigo recibe su **propia copia** del mod con un nombre único dentro (`@nombre`). Solo entra quien tiene invitación, y quitar la invitación equivale a expulsar. Así dos copias del juego con el mismo Steam ID dejan de parecer la misma persona.
- **Fuente:** lista de cambios de AzrakHQ, 8-nov-2026 («Invites: one mod per friend with a handle; invites only, revoke = ban»): https://github.com/AzrakHQ/pes2021-online-server/commits/main
- **Propuesta para Phoenix:** que el lanzador escriba un identificador ligado a la cuenta de la web y que el servidor lo use como identidad, en lugar de la IP de Tailscale. Sirve también cuando dejes Tailscale.
- **Impacto:** alto (era un riesgo ya anotado para la prueba de 2 jugadores). **Esfuerzo:** medio.
- **Cómo probar:** dos sesiones del rival falso con el mismo Steam ID y distinto identificador; el servidor debe ver dos clubes.

### 3-D. Medir cuánta máquina gasta cada partido antes de invitar gente [leído]
- **Qué es:** PESBUL contó el 27-oct-2026 que su relay de 11 contra 11 se come el procesador: 8 jugadores en dos salas llenaron un servidor. También dieron un número útil: el juego avanza a unos 54 pasos por segundo cuando va bien; ellos empezaron en 5.
- **Fuente:** hilo de PESBUL en evoweb, página 11, mensajes 306 y 312: https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/latest
- **Propuesta:** apuntar en el tablero de salud «procesador por partido» y «pasos por segundo» de nuestro relay, y poner un tope de partidos a la vez. **Impacto:** medio. **Esfuerzo:** bajo.
- **Cómo probar:** dos rivales falsos por el relay y mirar el uso del procesador del servidor.

### 3-E. Copa mensual automática dentro del juego [leído]
- **Qué es:** AzrakHQ crea solo un torneo nuevo cada mes («Friends Cup») con su propia tabla en Clasificaciones, dentro del PES.
- **Fuente:** README de AzrakHQ (enlace de arriba).
- **Propuesta:** «Copa Phoenix del mes», creada por el servidor el día 1, usando la parte de Torneo que ya funciona. **Impacto:** medio. **Esfuerzo:** bajo.
- **Cómo probar:** crear a mano un torneo con fecha de fin a 30 días y ver que aparece en la lista.

### 3-F. Street online 1 contra 1 con eFutsal [propia]
- **Qué es:** el online de PES funciona porque los dos juegos calculan lo mismo. Si los dos jugadores cargan el mismo módulo de fútbol 5 y el mismo estadio, puede que el partido online también sea 5 contra 5.
- **Fuente:** eFutsal (sección 4-A). La página no dice nada de online.
- **Riesgo real:** que el juego detecte que los dos no calculan igual y corte el partido (error de sincronía). Nadie lo ha probado en público.
- **Impacto:** muy alto si funciona (es tu deseo de «Street online con un amigo»). **Esfuerzo:** medio.
- **Cómo probar:** primero offline. Después, el día de la prueba con tu amigo, un amistoso con el módulo en los dos PCs.

### 3-G. Comprobar que los dos tienen el mismo parche [propia]
- **Qué es:** el módulo o el Core calcula una huella de los archivos que cambian el juego (jugabilidad y lista de módulos) y la manda al servidor. Si no coinciden, el servidor avisa antes de empezar en vez de cortar a mitad del partido.
- **Apoyo:** el instalador del mod de Champions (sección 4-F) ya hace algo parecido con el exe.
- **Impacto:** medio. **Esfuerzo:** medio.
- **Cómo probar:** cambiar un archivo en una copia y ver que el servidor lo nota.

### 3-H. Lo que NO recomiendo en online
- **playit.gg** para no abrir puertos: las fuentes de 2026 no se ponen de acuerdo sobre sus límites gratis y la dirección cambia en cada sesión [buscador]. Tailscale o NetBird siguen siendo mejor.
- **Sider en Linux** (`FabianZettl/sider7-linux`, MIT) para tener un segundo jugador de pruebas: funciona con Proton, pero necesita pantalla y tarjeta gráfica. No sirve en un servidor sin pantalla [leído]: https://github.com/FabianZettl/sider7-linux

---

## 4. Parche y juego

### 4-A. eFutsal v9.4: fútbol 5 contra 5 solo con Sider [leído]
- **Qué es:** un mod de ARL Board publicado el 11-nov-2026. Cancha de 40 × 20 m con vallas donde rebota el balón, porterías de 3 × 2 m, 1 portero y 4 jugadores en formación 1-2-1 para ti y para la CPU, sin fuera de juego, el botón de tiro siempre tira, barrera de 1 o 2 jugadores a 5 m y saques de esquina adaptados. Trae «protección contra cierres en partidos con menos de 11 jugadores».
- **Por qué importa tanto:** todo lo hace un módulo Lua (`efutsal.lua`) más una carpeta LiveCPK. **No toca el exe.** Es justo lo que tu regla pide y resuelve el problema que tenías apuntado de «con menos de 7 jugadores se acaba el partido». Y un archivo Lua se puede leer.
- **Fuentes:** https://www.pesmodding.com/2026/11/pes-2021-futsal-mode.html y https://caocacao.net/pes-2021-efutsal-mods-football-life/ (esta trae las dos líneas del `sider.ini`, las teclas y el archivo de registro).
- **Detalles:** se activa eligiendo el estadio 42 (Rose Park). Tecla Inicio lo enciende o apaga. Pide Sider 7.3.0 o más y `luajit.ext.enabled = 1`. Pesa 74,5 MB.
- **Impacto:** muy alto para Street. **Esfuerzo:** bajo para leerlo; medio para hacer el tuyo.
- **Cómo probar:** con tu SÍ para descargar, instalarlo en Conmegol, jugar un partido y leer `efutsal.lua` para apuntar en el catálogo de hallazgos cómo saca a los 6 jugadores y cómo evita el cierre.

### 4-B. Final con ceremonia en un partido suelto [leído]
- **Qué es:** «Custom Exhibition Match» v1.3 (Mochnaufals) hace que un partido de exhibición se vea como una final: trofeo, celebración y narración de final. Lo consigue cambiando valores en memoria con Sider.
- **Fuente:** https://ultimatemasterpes.blogspot.com/2026/05/custom-exhibition-match-mod-pes-2021.html
- **Propuesta:** «Final de la Galaxy League»: cuando la web marca un partido como final, el módulo propio activa la ceremonia. **Impacto:** alto (es lo más «dopamínico» que encontré). **Esfuerzo:** bajo para probar el mod, medio para el propio.
- **Aviso del autor:** hay que apagarlo antes de entrar a Liga Máster o puede dañar la partida guardada.
- **Cómo probar:** en Conmegol, un amistoso con la opción «KONAMI Cup Final» y ver si sale la entrega del trofeo.

### 4-C. Textos del juego con tu marca [leído]
- **Qué es:** «STR Editor» (Devil Cold52, actualizado en 2025) abre los archivos de texto del juego (`common.str` y otros, dentro de `dt20_???_win.cpk`), deja buscar, editar, exportar e importar.
- **Fuente:** https://www.pesmodding.com/2025/06/pes2021-str-editor-released-update-2025.html
- **Propuesta:** cambiar solo los textos que quieras («eFootball» por «Phoenix Online», nombres de torneos) y cargarlos por LiveCPK. No toca el exe y se quita borrando la carpeta.
- **Impacto:** alto para la identidad. **Esfuerzo:** bajo.
- **Cómo probar:** cambiar una sola frase visible del menú principal en Conmegol.
- **Segunda utilidad:** sirve para encontrar cualquier aviso del juego por su texto (ver sección 7).

### 4-D. Perfiles de jugabilidad encender/apagar [leído]
- **Qué es:** la jugabilidad vive en `dt18_all.cpk`. Hay dos editores: el de la 4chan Cup (ya en tu catálogo) y uno nuevo, «TBG Gameplay Parametric Editor», que trae **más de 480 valores con nombre y explicación** en dos archivos (`param_map.json` y `tooltips.json`) y exporta a JSON.
- **Fuentes:** https://www.pesmodding.com/2025/07/tbg-beautiful-game-gameplay-parametric.html y la página de la 4chan Cup sobre dt18 de PES 2021: https://implyingrigged.info/wiki/User:Aoba/PES2021_dt18 (no se abre desde la nube; leer desde la PC).
- **Propuesta:** tres perfiles propios («Arcade», «Simulación», «Street») guardados como JSON y cargados por LiveCPK. Cumple tu regla de opción encender/apagar en un PES limpio.
- **Impacto:** alto. **Esfuerzo:** medio.
- **Cómo probar:** cambiar un solo valor fácil de notar (peso del balón) y jugar 5 minutos.
- **Ojo en online:** los dos jugadores deben tener el mismo perfil (ver 3-G).

### 4-E. Jornada offline con resultado automático [leído]
- **Qué es:** «MatchUP» (2021) genera un guardado de copa con el partido que tú eliges, lo juegas en el PES, y después el programa mira el guardado y lee el resultado solo: marcador, notas, lesiones y tarjetas.
- **Fuente:** https://www.pesmodding.com/2021/08/pes-2021-matchup-new-game-mode-option.html
- **Pieza que lo hace posible hoy:** `leo-grayrat/pes-file-reader` descifra y vuelve a cifrar los guardados de PES 2021 (EDIT, Liga Máster, Leyenda y repeticiones), con prueba de ida y vuelta byte a byte, y explica la comprobación SHA-512 que el juego exige. Está en Python. **No tiene licencia escrita**, así que solo se lee: https://github.com/leo-grayrat/pes-file-reader
- **Propuesta:** «Jornada Phoenix offline»: la web da el partido, el lanzador prepara el guardado y al terminar sube el resultado. Sirve para liga contra la CPU, para Phoenix Carrera y para quien no tiene buen internet.
- **Impacto:** alto. **Esfuerzo:** alto.
- **Cómo probar:** descifrar y volver a cifrar una **copia** de un guardado tuyo y comprobar que el juego la carga.

### 4-F. Tabla de la liga dentro del juego e instalador que comprueba el exe [leído]
- **Qué es:** el mod «New UEFA Champions League Format» (Ruls, sobre trabajo de Adriel_Venezuela) calcula una tabla de 32 equipos y la muestra en el overlay de Sider, con el calendario. Su instalador revisa el exe firma por firma y, si no coincide, **cancela sin escribir nada**.
- **Fuente:** https://www.pesmodding.com/2026/09/new-uefa-champions-league-format-for.html
- **Propuestas:** (a) tabla de la Galaxy League en el overlay, traída de la web; (b) copiar la idea del instalador para el Core: comprobar antes de tocar y no hacer nada si algo no cuadra.
- **Impacto:** medio-alto. **Esfuerzo:** medio.
- **Cómo probar:** un módulo mínimo que muestre un texto fijo de tabla con `overlay_on`.

### 4-G. Notas del partido propias dentro del juego [leído]
- **Qué es:** «Fair Player Ratings» (Yopen71, oct-2026) cambia las notas del final del partido por una fórmula propia: todos empiezan en 5,0, se suman pases, robos, paradas y goles, se restan pérdidas y faltas, con decimales. Usa un Lua y una DLL. Si el exe no es el esperado, **se queda quieto y lo apunta en `sider.log`**.
- **Fuente:** https://pes-files.ru/pes_2021_new_modules_fair_player_ratings_2026
- **Propuesta:** demuestra que la «nota Phoenix» (idea D1 anterior) se puede mostrar dentro del juego y no solo en la web. Copiar también su forma segura de fallar.
- **Impacto:** medio. **Esfuerzo:** alto.
- **Cómo probar:** leer cómo engancha la pantalla de notas antes de decidir.

### 4-H. Hándicap entre amigos y partido «justo» [leído]
- **Qué es:** el módulo «EC26 Momentum» (Emre Çakir, oct-2026) da a cada equipo de 0 a 5 estrellas de fuerza y permite cambiarlas con el teclado. El «Anti-Cheat» de Holland pone a cero dos valores internos (`home_dynamic_difficulty` y `away_dynamic_difficulty`) que el juego sortea de 0 a 5 al empezar.
- **Fuentes:** https://pes-files.ru/pes_2021_ec26_momentum_booster_module_v2_0 y https://pes-files.ru/pes_2021_anti_cheat_2026
- **Propuesta:** dos opciones encender/apagar: «Partido justo» (los dos valores a cero) y «Hándicap» (el que más sabe da ventaja al otro). **Impacto:** medio. **Esfuerzo:** medio.
- **Cómo probar:** offline, mismo equipo contra sí mismo con estrellas 0 y 5.
- **Sin verificar:** que esos valores existan también en partidos online.

### 4-I. Espíritu de equipo igual para los dos [leído]
- **Qué es:** «Bromi Gameplay Core» fija el Espíritu de equipo de los dos lados en el mismo valor y mantiene estable la barra táctica de la CPU.
- **Fuente:** https://www.pesmodding.com/2026/09/pes-2021-bromi-gameplay-core-v10.html
- **Propuesta:** opción «Espíritu de equipo igualado» para partidos de liga, para que no gane quien tiene el equipo más «rodado». **Impacto:** medio. **Esfuerzo:** medio.
- **Cómo probar:** mirar en Plan de juego que los dos equipos marcan el mismo valor.

### 4-J. Nombres narrados para jugadores nuevos, sin grabar nada [leído]
- **Qué es:** la idea anterior P14 (voz artificial) quedó como «no investigado». Hay un camino más corto: «CallnameServer 2026» une el número de un jugador con un nombre que el narrador **ya tiene grabado**. Los paquetes de Predator002 traen más de 10.000 nombres.
- **Fuentes:** https://pes-files.com/pes-2021-callname-server-2026-update-v1/ y https://pesmod-pc.blogspot.com/2020/09/pes-2021-english-commentary-callname-v1.html
- **Propuesta:** un guion que busque, para cada jugador inventado de la liga, un apellido igual o parecido ya grabado y lo enlace. **Impacto:** medio. **Esfuerzo:** bajo-medio.
- **Cómo probar:** enlazar a mano un jugador sin nombre narrado con un apellido que exista y marcar un gol.

### 4-K. Idioma del narrador según el torneo [leído]
- **Qué es:** «Dynamic Commentary» (pedrozo92) cambia el idioma del narrador según la competición y los equipos, leyendo la memoria. La versión 2.0 quita un retraso de un partido que tenía la primera.
- **Fuente:** https://pes-files.com/pes-2021-dynamic-commentary-v1-fl-2026/ (la 2.0 está en evoweb, sin leer).
- **Propuesta:** Galaxy League con narrador en español latino y copas con otro. **Impacto:** bajo-medio. **Esfuerzo:** bajo.

### 4-L. Cámara libre para clips y tráileres [leído]
- **Qué es:** la idea anterior P7 solo tenía el título. Ya la leí: es un paquete de 4 módulos de FletchAJX. Lo nuevo es **FreeCam 3.4.7**: cámara que vuela libre durante el partido, con órbita lenta, seguimiento del balón y posiciones guardadas por estadio. También trae 10 tomas de «cámara araña» antes del saque.
- **Fuente:** https://www.pesmodding.com/2026/11/dynamic-broadcast-cameras-for-pes-2021.html
- **Propuesta:** usarla para grabar los vídeos de la liga y los clips de goles. **Impacto:** medio. **Esfuerzo:** bajo.

### 4-M. Ropa que se mueve [leído]
- **Qué es:** «Cloth FX» (Kris1982 y Alex10, 26-oct-2026) añade movimiento de tela a camisetas y pantalones, con ajustes en el overlay y una opción para gastar menos procesador en jugadores lejanos.
- **Fuente:** https://www.pesmodding.com/2026/10/pes-2021fl-2026-cloth-fx-real-moving.html
- **Aviso:** un usuario cuenta que no le funcionó con otro parche y que al paquete le faltaban archivos. **Impacto:** medio (se ve mucho en el stream). **Esfuerzo:** bajo para probar.

### 4-N. Perfiles de guardado con un clic [leído]
- **Qué es:** «Profile Switcher» v1.1 (efootball_multiverse) cambia entre varios perfiles de guardado y arranca distintas configuraciones de Sider.
- **Fuente:** https://www.pesmodding.com/2025/10/pes-2021-profile-switcher-version-11-by.html
- **Propuesta:** en el Launcher Phoenix, botones «Online limpio» y «Parche completo» que cambian guardado y `sider.ini` a la vez. Encaja con tu plan Online/Offline. **Impacto:** medio. **Esfuerzo:** bajo.

### 4-Ñ. Partidos jugados por la CPU cuando alguien no aparece [propia]
- **Qué es:** la 4chan Cup lleva años haciendo ligas donde nadie juega: cada dueño entrega su equipo y sus tácticas, y la CPU juega los partidos en directo. Tienen herramientas para comprobar los equipos (`autoATF`, `4ccEditor`).
- **Fuentes:** https://github.com/orgs/the4chancup/repositories · módulo que deja a la CPU alinear sola: https://www.pesmodding.com/2026/10/fl26-pes-2021-chatty-auto-lineup-v102.html · hilo de evoweb con ajustes para CPU contra CPU (sin leer, sección 8).
- **Propuesta:** si un jugador no se presenta, en lugar de un 3-0 seco, la CPU juega su equipo con su plan de juego guardado y se emite en el stream. También sirve para una «Liga de IA» que funcione sola de noche.
- **Impacto:** alto como contenido. **Esfuerzo:** alto.
- **Cómo probar:** un partido CPU contra CPU en Conmegol con el módulo de alineación automática.

---

## 5. Herramientas

### 5-A. Compilar el parche con un comando [leído]
- **Qué es:** «Unlimited Tool Works» de la 4chan Cup. Un solo programa con ventana y con línea de comandos (`UTW <herramienta> <orden>`). Maneja CPK, FPK, FTEX, DDS, WESYS y modelos FMDL de PES 18 a 21, y compila equipos enteros (caras, botas, guantes, camisetas, árbitros) en archivos CPK. Tuvo cambios hoy mismo (12-nov-2026).
- **Licencia:** Apache-2.0 o MIT, a elegir. Deja fuera los datos que son de Konami.
- **Fuente:** https://github.com/the4chancup/unlimited-tool-works
- **Propuesta:** es la pieza que pedían las ideas anteriores H2 y H3: un guion que convierta una carpeta de trabajo en los CPK del parche.
- **Estado:** en desarrollo (6 de 20 fases); la versión 0.0.1 solo trae el editor de guardados. **Impacto:** alto. **Esfuerzo:** medio.
- **Cómo probar:** con tu SÍ, empaquetar una carpeta pequeña en un CPK y cargarla en Conmegol.

### 5-B. Editor de camisetas en la web [leído]
- **Qué es:** «Okarin Kit Editor»: dibujas la camiseta en 2D, la ves en 3D y exporta PNG, DDS o **FTEX directo**, probado en PES 2021. Funciona en el navegador o como programa de Windows.
- **Licencia:** GPL-3.0. Si metes su código dentro de tu web, tu web tendría que ser GPL también. Enlazarlo o usarlo aparte no obliga a nada.
- **Fuente:** https://github.com/okarin-hub/okarin-kit-editor
- **Propuesta:** «Crea la camiseta de tu club»: el jugador la hace ahí, sube el archivo a la web y entra al parche por Live Update. Completa la idea anterior W13 (creador de escudos). **Impacto:** alto para la comunidad. **Esfuerzo:** bajo si se usa aparte.

### 5-C. Creador de ligas [leído]
- **Qué es:** el proyecto DynamicRealism publica «DR Ultimate League Creator» (0.5.64, en pruebas): crea ligas, clubes, plantillas, escudos y camisetas reales en Football Life, y también las quita y devuelve todo al estado original.
- **Fuente:** https://dynamicrealism.pages.dev/
- **Propuesta:** mirar cómo organiza los datos para tu base de datos propia. **Ojo:** su parche de jugabilidad cambia el exe; eso no nos sirve. Solo interesa la herramienta de ligas. **Impacto:** medio-alto. **Esfuerzo:** bajo (mirar).

### 5-D. Cómo crece un jugador en Liga Máster [leído]
- **Qué es:** el mismo `pes-file-reader` documenta un mecanismo que dice que nadie había explicado: sorteos semanales, curvas de crecimiento que bajan después del mejor momento y temporada de 82 semanas.
- **Fuente:** https://github.com/leo-grayrat/pes-file-reader (documentación en chino, sin licencia: solo leer).
- **Propuesta:** usar esas curvas como base de la progresión en Phoenix Carrera. **Impacto:** medio. **Esfuerzo:** bajo (leer y traducir).

### 5-E. Guía de problemas de AzrakHQ [leído]
- **Qué es:** hoy movieron su guía de solución de problemas a `server/README.md` y escribieron «la historia de cómo se reconstruyó el servidor y cómo funciona».
- **Fuente:** cambios del 12-nov: https://github.com/AzrakHQ/pes2021-online-server/commits/main
- **Propuesta:** leerla antes de la prueba con tu amigo; sus fallos típicos serán los nuestros. **Esfuerzo:** bajo.

### 5-F. Datos para ligas nuevas, con cuidado [leído]
- «PES Transfermarkt Scrapper Tool V2.1» saca jugadores, equipos y técnicos a CSV: https://www.pesmodding.com/2025/09/pes-transfermarkt-scrapper-tool-v21.html
- «PESDatabase v0.2.1» convierte datos de eFootball a PES 2021 y exporta CSV; tiene parte gratis y parte de pago: https://www.pesmodding.com/2026/07/pesdatabase-v021-efootball-and-pes.html
- **Aviso:** sacar datos de una web puede ir contra sus condiciones de uso. Es decisión tuya. Para tu base de datos propia, mejor seguir esperando la de FL26 como ya decidiste.

---

## 6. Web, comunidad y app

### 6-A. «Salas en vivo» en la web [leído]
- **Qué es:** PESBUL tiene una página «Live lobbies» y muestra cuánta gente está conectada y cuánta está buscando rival ahora mismo. También un «Match center» y un botón de estado del servidor.
- **Fuente:** https://pes2021.pesbul.eu/elo/ y https://pes2021.pesbul.eu/team-matches/
- **Propuesta:** en la página `/online/` de tu web: «3 conectados, 1 buscando rival» y la lista de salas abiertas. El servidor ya sabe todo eso. **Impacto:** alto (da ganas de entrar). **Esfuerzo:** bajo.
- **Cómo probar:** con el rival falso buscando partido, ver que la web lo cuenta.

### 6-B. Touchline como modelo para Phoenix Carrera [leído]
- **Qué es:** «Touchline» (falasteeny, versión 1.0.9 del 12-nov-2026, gratis) es un compañero de Liga Máster: historias, mensajes de jugadores, preguntas de prensa, directiva que recuerda, personal contratado, patrocinios, negociación de fichajes con barra de oferta del 60 % al 140 % y medidor de paciencia, cesiones, sugerencias de cambios en vivo y noticias en formato periódico. Es una carpeta compañera más 3 módulos pequeños de Sider, y dice que lee el guardado sin modificarlo.
- **Fuentes:** https://pes-files.com/pes-2021-ml-touchline-update-v1-0-0/ y https://pes-files.com/pes-2021-master-league-touchline-update-v1-0-5/
- **Propuesta:** añadirlo al estudio que ya hace el hilo de la app sobre The Gaffer. Lo más copiable como idea: la barra de negociación con paciencia y la directiva con memoria. **Impacto:** alto para la app. **Esfuerzo:** bajo (leer).
- **Otros del mismo tipo, sin leer:** «ML Manager Pro» y «Master League Transfer Assistant» (sección 8).

### 6-C. Himno de gol por club [propia]
- **Qué es:** la 4chan Cup usa «rigdio», un reproductor que pone solo el himno y la canción de gol de cada equipo. Sider puede reproducir MP3 y saber cuándo cambia el marcador (`match.stats()`).
- **Fuente:** https://github.com/the4chancup/rigdio y la documentación de Sider 7.4: https://github.com/pes-modding/sider7/blob/master/doc/scripting.html
- **Propuesta:** cada club de la liga elige su canción de gol en la web (música propia o libre) y suena en el juego. **Impacto:** medio-alto, muy «dopamínico». **Esfuerzo:** bajo-medio.
- **Cómo probar:** un módulo de 30 líneas que suene un MP3 cuando cambia `home_score`.

---

## 7. Pistas sobre el aviso falso de «máximo» en los regalos

No toqué el trabajo del hilo Online. Esto son **pistas e hipótesis, no hechos**, para que las use o las descarte.

**Lo que sí comprobé en los datos de AzrakHQ que ya tenemos:**
1. En la tabla de errores (`server_errors.tsv`), `CMD_RECV_MYCLUB_PRESENT` solo tiene dos errores de servidor: `ERR_DATABASE` («Unable to claim present») y mantenimiento de KONAMI ID. **Ninguno habla de «máximo».** Los errores de tipo «máximo» de myClub pertenecen a otros comandos (entrenadores llenos, lista de deseos llena, nivel máximo). Esto apoya lo que el hilo ya concluyó: el aviso lo decide la pantalla del juego, no el código de error.
2. En `schemas.json`, la respuesta de recibir regalo lleva `body` como **número**, mientras que en la lista de regalos `body` es texto. Vale la pena mirar si mandamos `body` con el tipo correcto en cada una.

**Hipótesis principal [propia]: el juego cree que el tope es cero.**
- El aviso sale con cualquier cantidad (5 monedas, 50 GP, 5000 GP). Un tope real no se comporta así. Eso encaja con que el juego compare «lo que tengo + lo que recibo» contra un máximo que **viene de otra respuesta** que nosotros mandamos vacía o en cero.
- Además la bandeja pinta todo regalo como «Monedas de myClub». Eso indica que el juego no reconoce nuestro texto de `type` y cae en el tipo por defecto, que sería la moneda. Las dos cosas pueden tener la misma raíz: tipo no reconocido → se trata como moneda → se compara con un tope de monedas que nunca enviamos.
- **Qué mirar:** qué campo de tope de monedas o GP lee el juego al entrar a myClub, y la tabla de textos válidos para `type` del regalo (igual que encontraron la tabla de «uso de objeto» en 0x1437F33A0).

**Camino más directo [propia, usando una herramienta nueva]:**
1. Con **STR Editor** (sección 4-C) buscar la frase «ya tienes el máximo permitido» en los textos del juego y apuntar su identificador.
2. Buscar ese identificador en Ghidra. Eso debería llevar a la función que **muestra** el aviso. (No he comprobado cómo guarda el juego esos identificadores; si no aparece tal cual, buscar su código abreviado.)
3. Subir desde ahí hasta la comparación. Se ve de golpe qué dos números compara y de qué respuesta sale cada uno.
Es ir del síntoma a la causa, en vez de seguir cambiando la respuesta y mirando.

**Segunda vía:** el banco de pruebas `parsebench` de AzrakHQ (ya apuntado en la idea 1 de `IDEAS_NUEVAS_13`).

**No encontré** en internet a nadie que documente este aviso. Busqué los nombres `ERR_RECV_PRESENT_FULL`, `receive_result_list` y el texto del aviso en inglés: cero resultados útiles.

---

## 8. Para leer desde la PC (evoweb rechazó la nube a mitad de la investigación)

Solo vi el título. Están ordenados por interés.

1. **eFutsal PES2021** (hilo del autor, con respuestas): https://evoweb.uk/threads/efutsal-pes2021.107800/
2. **Lista de herramientas y tutoriales importantes** (hilo fijo): https://evoweb.uk/threads/list-of-important-tools-tutorials-updated-july-3rd-25.102301/
3. **Ajustes para CPU contra CPU**: https://evoweb.uk/threads/ai-vs-ai-maybe-hum-vs-ai-fundamentals-parameters-for-a-sim-experience.103466/
4. **The Gaffer - Master League App** (el hilo de la app lo necesita): https://evoweb.uk/threads/the-gaffer-master-league-app-public-test-release.107579/
5. **Touchline v1.0.9**: https://evoweb.uk/threads/new-master-league-immersion-football-manager-touchline-v1-0-9-new-update-12-november-2026.107018/
6. **MatchState: Dynamic Attack Mentality v4.5**: https://evoweb.uk/threads/matchstate-dynamic-attack-mentality-v4-5-update-11-10-26.107691/
7. **Dynamic Commentary v2.0**: https://evoweb.uk/threads/v2-0-dynamic-commentary-no-more-one-match-delay.107128/
8. **Banderas que ondean y público dinámico en finales**: https://evoweb.uk/threads/module-fl27-waving-flag-home-away%E2%80%A6%F0%9F%98%8Dnew-dynamic-crowd%F0%9F%98%8D-special-matches-finals-by-seratonin-and-balboaste.107746/
9. **Real Auto Feint**: https://evoweb.uk/threads/real-auto-feint-a-small-upgrade-to-the-games-auto-feint.107709/
10. **Master League Transfer Assistant** y **ML Manager Pro**: https://evoweb.uk/threads/tool-ml-manager-pro-master-league-career-manager-advanced-player-analytics-for-pes-2021.104023/
11. **Tifo Replay Mod**: https://evoweb.uk/threads/tifo-replay-mod.107244/
12. **Página dt18 de PES 2021 en la wiki de la 4chan Cup**: https://implyingrigged.info/wiki/User:Aoba/PES2021_dt18
13. **Hilo de PESBUL, páginas 8 a 10** (el buscador las señala al preguntar por el relay de 11 contra 11): https://evoweb.uk/threads/pes-2021-online-server-emulator-open-beta-test-server.106068/page-10

---

## 9. Lo que evitaría

- **Parches de jugabilidad que cambian el exe** (G.O.A.T. Rush, Winning Eleven 2026 Unofficial, Soccer Revolution y otros). Chocan con tu límite de no modificar `PES2021.exe`. Solo sirven las partes que van por Sider o LiveCPK.
- **PES 2021 Live Editor** (Ruls): edita la memoria usando la tabla de trucos. Su propio autor dice que nunca se use en modos online. No entra en una liga.
- **Descargas con contraseña o en servidores de anuncios** (varias de pesmodding piden quitar el bloqueador). Bajar solo del autor y con tu SÍ.
- **Mods hechos deprisa con IA sin revisar:** varios de 2026 lo reconocen y traen fallos (archivos que faltan, documentación que no coincide). Leer el código antes de instalar.
- **Código GPL o sin licencia dentro del tuyo:** Okarin Kit Editor y el editor de dt18 de la 4chan Cup son GPL-3; `pes-file-reader` no tiene licencia. Leer y aprender sí; pegar su código no.
- **Sacar datos de Transfermarkt o sortitoutsi sin mirar sus condiciones.**
- **Páginas basura** que siguen saliendo: `edx.ipeindia.org`, `grandavehousing.calpoly.edu` y parecidas.

---

## 10. Resumen de fuentes principales

- Sider 7.4, guía de programación: https://github.com/pes-modding/sider7/blob/master/doc/scripting.html y eventos propios: https://github.com/pes-modding/sider7/blob/master/doc/custom-events.html
- pesmodding.com, módulos: https://www.pesmodding.com/search/label/PES%202021%20Sider%20Modules · herramientas: https://www.pesmodding.com/search/label/PES%202021%20Tools · jugabilidad: https://www.pesmodding.com/search/label/PES%202021%20Gameplay
- pes-files (ruso e inglés): https://pes-files.ru/ y https://pes-files.com/
- 4chan Cup en GitHub: https://github.com/orgs/the4chancup/repositories
- Organización de juce: https://github.com/orgs/pes-modding/repositories
- PESBUL: https://pes2021.pesbul.eu/the-next-era-of-pesbul-2021-starts-now/ y https://pes2021.pesbul.eu/pes-2021-is-online-again/
- AzrakHQ (MIT): https://github.com/AzrakHQ/pes2021-online-server y la copia en `referencia_azrak_mit/`
- Foro PES 2021 de evoweb: https://evoweb.uk/forums/pes-2021.337/

## 11. Conteo

32 ideas nuevas: 7 de online (3-A a 3-G), 15 de parche y juego (4-A a 4-Ñ), 6 de herramientas (5-A a 5-F), 3 de web y app (6-A a 6-C) y 1 camino nuevo para el aviso de regalos (sección 7). De ellas, 27 salen de páginas leídas enteras y 5 son propuestas mías apoyadas en lo leído (3-F, 3-G, 4-Ñ, 6-C y la sección 7).
