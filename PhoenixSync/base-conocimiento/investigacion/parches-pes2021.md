# Parches de PES 2021 (PC/Steam) en 2025–2026: qué existe, cómo están hechos y por qué crashean

Investigación web hecha el 8 de octubre de 2026 para el **Phoenix Evolution Patch**.

**Cómo leer este documento**
- Cada dato importante lleva su fuente (enlace).
- **[INCIERTO]** = no lo pude confirmar con una fuente fiable, o las fuentes se contradicen.
- **[INFERENCIA]** = conclusión mía a partir de varias fuentes; no está escrita así en ninguna.
- Aviso: muchos resultados de búsqueda sobre "PES 2021 patch 2025/2026" son páginas basura/SEO (dominios universitarios o de empresas hackeados que repiten texto). Las descarté. Las fuentes útiles fueron sobre todo **pessmokepatch.com** (oficial de Football Life), **pesmodding.com** (blog agregador, autor "G. Leroy"), **evoweb.uk** (foro; empezó a bloquear mis lecturas a mitad de la investigación), **Steam Community** y el código de **4ccEditor** en GitHub.

---

## 0. Contexto base (lo que todo parche tiene que respetar)

### 0.1 El juego ya no se vende
- Konami retiró PES 2021 de las tiendas digitales el **9 de diciembre de 2021**. Quien lo compró antes puede seguir instalándolo. Fuente: [La República, 09/11/2021](https://larepublica.pe/videojuegos/2021/11/09/pes-2021-sera-retirado-de-las-tiendas-digitales-y-cerrara-sus-servidores).
- Los servidores oficiales cerraron en **agosto de 2022**. Fuente: [hilo de Steam "Servers are now officially down forever", 24/08/2022](https://steamcommunity.com/app/1259970/discussions/0/3323114398565714368).
- **Consecuencia:** muchos jugadores nuevos usan copias crackeadas o "repacks". Los autores de parches dicen que muchos bugs vienen justo de ahí. Ejemplos:
  - El autor del Evoweb Patch atribuye a copias piratas las pantallas de carga infinitas en las copas de la Liga Máster, y confirma que el parche **no** funciona con el repack DODI ([pesmodding, Evoweb 2024 v3.0](https://www.pesmodding.com/2024/06/pes-2021-evoweb-patch-2024-version-30.html)).
  - Gogosz responde lo mismo ante cargas infinitas ([caocacao.net](https://caocacao.net/pes-2021-gogosz-patch/)).

### 0.2 La versión final del juego
- El último Data Pack oficial es el **7.0**, del 24 de junio de 2021 ([DSOGaming](https://www.dsogaming.com/tag/efootball-pes-2021-season-update/)). Corresponde a la actualización 1.08 en consolas y móvil ([AttackOfTheFanboy](https://attackofthefanboy.com/guides/efootball-pes-2021-update-1-09-patch-notes/)).
- El ejecutable que hoy exigen los mods para PC es **PES2021.exe 1.07.02** (versión interna 1.7.2.0). Lo dice textualmente el mod Custom Celebrations de octubre de 2026 ([pesmodding](https://www.pesmodding.com/2026/10/pes-2021fl-2026-custom-celebrations-add.html)).
- **"Data Pack 8": [INCIERTO / probablemente no existe].** No encontré ninguna fuente oficial de un DP8 para PES 2021. Si alguien habla de "DP8", seguramente es un paquete hecho por la comunidad, no de Konami.
- **Versiones antiguas del exe:** 1.00 (lanzamiento), 1.01.00 y 1.01.01 (Sider 7.0.2 se adaptó a estas dos; [pesmodding 2020](https://www.pesmodding.com/2020/09/sider-for-pes-2021-season-update.html)), y luego varias más hasta llegar a 1.07.02. Hoy **todo el ecosistema apunta a 1.07.02 + DLC/DP 7.0**. Por ejemplo, el Evoweb Patch exige "DLC 7 instalado por Steam y arrancar el juego una vez" antes de instalarlo ([pesmodding](https://www.pesmodding.com/2024/05/pes-2021-evoweb-patch-2024-version-20.html)).

### 0.3 Sider: el "cargador" que usa todo el mundo
**Qué es, explicado simple:** Sider (hecho por *juce*) es un programa que se mete dentro del juego cuando este arranca. Con eso:
- Sirve archivos sueltos desde carpetas (**livecpk**, se activan con líneas `cpk.root` en `sider.ini`), en vez de tener que empaquetarlos en archivos CPK.
- Ejecuta **módulos Lua** (pequeños scripts que pueden leer y cambiar el juego mientras corre).

**Historia de versiones:**
- 7.0.x salió en 2020 ([pesmodding](https://www.pesmodding.com/2020/09/sider-for-pes-2021-season-update.html)).
- 7.3.3 es la versión que en 2025–2026 todavía piden muchos packs, como los facepacks sudamericanos ([pes-files](https://pes-files.com/pes-2021-sudamericana-facepack-2026-update-v3/)).
- **7.4.0 (6 de abril de 2026):**
  - Añade la opción `save.folder`, para que cada parche tenga su propio `EDIT00000000`.
  - **Rompe** los módulos que cambiaban los colores del overlay con el viejo "hack de memoria" (por ejemplo `colorsdemo.lua` y los módulos de logos de TV). Según el aviso, "causan crashes y corrupción de memoria" ([pesmodding](https://www.pesmodding.com/2026/04/pes-2021-sider-v740.html)).
- **7.4.1 (22 de abril de 2026):**
  - Corrige la lógica de `save.folder`, que corrompía partidas y sobrescribía `SYSTEM00000000` sin necesidad.
  - Mejora `netblock.lua` y acelera las comprobaciones de livecpk ([pesmodding](https://www.pesmodding.com/2026/04/pes-2021-sider-v741.html)).
- **Instalación:** la carpeta de Sider debe estar donde tu usuario de Windows tenga permisos completos (Escritorio, Documentos o tu carpeta personal). Lo repiten todas las versiones.

### 0.4 Dos maneras de instalar contenido
1. **CPK + DpFileList.bin** (el método clásico):
   - Los archivos se empaquetan en `.cpk` dentro de `download/`.
   - `DpFileList.bin` lista qué CPK carga el juego y en qué orden.
   - El generador más usado admite **un máximo de 47 CPK** ([pesmodding, generador DLC 7.0](https://www.pesmodding.com/2021/06/pes-2021-new-dpfilelist-generator-dlc-70.html)).
   - Si al generarlo no eliges el DLC más reciente, aparecen **cargas infinitas** ([pesmodding, generador DLC 4.0](https://www.pesmodding.com/2021/02/pes-2021-dpfilelist-generator-dlc-40.html)).
2. **Sider livecpk** (el método moderno):
   - Carpetas sueltas activadas con `cpk.root = ".\livecpk\X"` en `sider.ini`.
   - Hoy lo usan casi todos: Evoweb, ConmeGOL, packs de FL y facepacks.

---

## 1. Ficha por parche

### 1.1 SP Football Life (FL24 / FL25 / FL26 / FL27), del SmokePatch Team

**Quién lo hace:** el SmokePatch Team, liderado por "dido", en [pessmokepatch.com](https://www.pessmokepatch.com/). En el foro lo describen como "un juego que se actualiza con cada temporada" ([hilo FL27 en evoweb](https://evoweb.uk/threads/sp-football-life-2027-discussion-thread-pc.106347/)).

**Qué es:** un **juego independiente** construido sobre el motor de PES 2021.
- Trae su **propio ejecutable** (`FL_2026.exe`), un **lanzador** (`FL_2026 start.exe`) y un **instalador** (`SPFL26_setup.exe`).
- Se instala en una carpeta nueva y **nunca encima de un PES existente** ([página FL26](https://www.pessmokepatch.com/2025/10/spfl26.html)).

**Versiones:**

| Versión | Fecha | Fuente |
|---|---|---|
| FL26 1.0 | 15 de octubre de 2025 | [página FL26](https://www.pessmokepatch.com/2025/10/spfl26.html) |
| FL26 update 2.0 | 7 de marzo de 2026 | [notas de versión](https://www.pessmokepatch.com/2026/03/FL26updatenotes.html) |
| FL26 update 2.1 | sin fecha | mismas notas |
| FL26 update 2.2 | 12 de mayo de 2026 | mismas notas |
| Selecciones para el Mundial 2026 | junio de 2026 | [FL26WC](https://www.pessmokepatch.com/2026/06/FL26WC.html) |
| **FL27** | **en desarrollo; objetivo "principios de noviembre de 2026"** | [FL27comps](https://www.pessmokepatch.com/2026/08/FL27comps.html), [FL27](https://www.pessmokepatch.com/2026/07/FL27.html) |

**Qué incluye:**
- **Base de datos:**
  - Plantillas 2025/26 con ascensos y descensos.
  - 15 equipos nuevos de clubes ascendidos (update 2.0).
  - Cientos de jugadores importados de la base "Live" de Konami.
  - El Edit mode funciona, y hay 3 equipos libres para que la comunidad los edite: Ceres Negros, Zalgiris Vilnius y Zamora FC.
- **Gameplay "simulación"** (update 2.0):
  - Más lento y táctico, con menos automatismos.
  - El clima y el césped afectan más al balón y a los jugadores.
  - La barra de los tiros libres carga a velocidad variable según la distancia.
  - **`SP switcher.exe`** permite volver al gameplay clásico.
- **Kits:** la update 2.2 trae 49 juegos de kits de clubes y 39 de selecciones.
- **Estadios:** 36 de serie, y más con el addon "SP Stadiums", que funciona con Sider ([siderstadiums](https://www.pessmokepatch.com/2024/11/siderstadiums.html)).
- **Caras:** van en un addon aparte, "Real Faces FL26/27". La Update 4 (12 de mayo de 2026) añadió 600 caras ([faces2627](https://www.pessmokepatch.com/2025/10/faces2627.html)).
- **Comentarios:** en inglés, v8 al lanzar y v9 desde marzo de 2026 ([flcoms](https://www.pessmokepatch.com/2025/10/flcoms.html)).
- **Modo carrera:** ofertas de selecciones nacionales tras 1 o 2 temporadas ([FAQ](https://www.pessmokepatch.com/2022/12/FAQ.html)). Las partidas de carrera de cualquier versión de FL26 se conservan al actualizar ([notas](https://www.pessmokepatch.com/2026/03/FL26updatenotes.html)).
- **FL27, competiciones** ([FL27comps](https://www.pessmokepatch.com/2026/08/FL27comps.html)):
  - Mundial de **48 equipos** (12 grupos de 4; pasan los 8 mejores terceros a una fase final de 32).
  - Copa América de 16 equipos.
  - Argentina con **30 equipos**, quitando los topes anteriores de 26 en liga y 24 en copa.
  - MLS con 30 equipos y Ligue 2 con 18.
  - Bélgica con 18 equipos y sin playoffs.
  - Coupe de France con 36 equipos.
  - Los nuevos formatos UEFA (UCL/UEL) **todavía no son estables** y se dejan para después.
  - Todo va "integrado en el juego, sin scripts externos".

**Política de actualización (muy buena idea para copiar)** ([FL27comps](https://www.pessmokepatch.com/2026/08/FL27comps.html)):
- Los cambios que **no** afectan a una carrera empezada salen en una update normal.
- Los que necesitan partida nueva salen con notas que lo explican.
- Los que podrían **dañar** una carrera empezada se guardan para la versión de la temporada siguiente.

**Innovación técnica clave, el exe y el lanzador por tipo de CPU** ([compatibilidad](https://www.pessmokepatch.com/p/compatibility.html)):
- **FL23–FL25** traían varios exe según cuántos "núcleos de rendimiento" tiene tu CPU:
  - `start.exe` (CPUs anteriores a 2022, todos los AMD e i7/i9 K/KF/HX).
  - `12G.exe` (6 núcleos P).
  - `M.exe` (4 núcleos P).
  - `S.exe` (2 núcleos P, baja la calidad de render).
  - `U.exe` ("sin restricciones").
  - Iban acompañados de `.bat` que **los antivirus marcan a veces como peligrosos**.
- **FL26:** trae "un ejecutable rediseñado y un lanzador que maneja el hardware moderno automáticamente", así que ya no hacen falta variantes.
- **Por qué existía esto:** los Intel de 12.ª generación en adelante, con núcleos P y E mezclados, chocaban con el DRM (Denuvo) del juego. Intel lo reconoció y lo arregló para la mayoría de juegos ([Tom's Hardware](https://www.tomshardware.com/uk/news/intel-fixes-most-drm-issues-with-alder-lake)). En evoweb, un usuario con Intel de 12.ª gen reportó un bucle infinito antes del saque inicial que se arreglaba con la copia legal de Steam ([evoweb](https://evoweb.uk/threads/efootball-pes-2021-discussion-thread-pc.84053/post-4154907)).

**Ecosistema alrededor de FL** (casi todo funciona con Sider, en la carpeta `SiderAddons`):
- **FL26 1.2 Essentials Mod Pack:** reúne scoreboards, entradas al campo, cánticos, estadios, etc.
  - Funciona **solo** con la versión 1.2.
  - **No** sirve para 1.3 ni para las updates 2.0/2.2 ([pesmodding](https://www.pesmodding.com/2025/11/sp-football-life-2026-v12-essentials.html)).
- **Football Life Club Creator (FLCC):**
  - Edita nombre, abreviatura, estadio y escudo de los clubes.
  - Configura livecpk automáticamente y hace copias de seguridad.
  - Todavía no edita jugadores, ligas ni torneos ([pesmodding](https://www.pesmodding.com/2026/08/football-life-club-creator-flcc-v011.html)).
- **Módulos Lua que funcionan en PES 2021 y en FL26:**
  - Crowd Attendance ([pesmodding](https://www.pesmodding.com/2026/10/pes-2021-crowd-attendance-module.html)).
  - Chatty Auto Lineup.
  - Custom Celebrations, que tiene versiones separadas para PES y para FL26 ([pesmodding](https://www.pesmodding.com/2026/10/pes-2021fl-2026-custom-celebrations-add.html)).
  - Touchline, de "inmersión" para la Liga Máster ([índice de evoweb](https://evoweb.uk/forums/pes-2021.337/)).
- **Option files hechos por la comunidad** (por ejemplo el de Ribeiro50 para FL25): sirven **solo** para esa versión de FL, "no para otros parches ni para PES 2021/2020" ([pesmodding](https://www.pesmodding.com/2025/01/sp-football-life-2025-option-file-fl25.html)).

**Problemas conocidos y soluciones oficiales** ([FAQ](https://www.pessmokepatch.com/2022/12/FAQ.html)):

| Problema | Solución |
|---|---|
| Pantalla negra o blanca, o crash **después de instalar un facepack** | Subir el archivo de paginación de Windows, mejor en "administrado por el sistema" |
| Lag o crash en portátiles con dos GPU | Forzar la GPU dedicada para `FL_20XX.exe` **y también** `settings.exe` |
| Error de "archivos de Windows que faltan" | Instalar DirectX Runtime, Visual C++ Redistributable y .NET 4.5 |
| Alineaciones raras | Falta el archivo EDIT: reinstalar como administrador, dejar que el instalador acceda a Documentos y quitar los mods de base de datos |
| Error al instalar | Documentos está en OneDrive "solo en la nube": marcar "Mantener siempre en este dispositivo" |
| Windows no deja guardar `map_teams.txt` o `sider.ini` | Darse permisos o editar el archivo en otro disco |
| ¿Los mods externos rompen el juego? | En general no, pero los **mods de base de datos** "pueden romper el juego o causar bugs fácilmente" |

Avisos oficiales en las notas de versión ([notas](https://www.pessmokepatch.com/2026/03/FL26updatenotes.html)):
- Editar los bins de la base de datos o el exe "puede causar bugs en el modo carrera".
- Solo prueban con el juego por defecto más los addons de SP.

Problemas que reportan los usuarios ([pesmodding Essentials](https://www.pesmodding.com/2025/11/sp-football-life-2026-v12-essentials.html), [r/SPFootballLife](https://lr.eu.psf.lt/r/SPFootballLife)):
- **El juego deja de arrancar** después de instalar los comentaristas franceses (sin solución publicada).
- **Scoreboards de la competición equivocada** (AFC/CONCACAF mezclados, o el del Mundial en todas las ligas). La causa es `map-competition.txt`.
- **Estadios que desaparecen**: hay que moverlos a stadium-server y volver a mapearlos.
- **Césped negro**: el ID o la ruta del estadio están mal.
- **Textos del menú gigantes**: se arregla comentando una línea `cpk.root`.
- **Configuración que se reinicia en cada arranque.**
- **Transferencias bloqueadas por el mod NoRegen** al cambiar de temporada (en FL24): desactivarlo temporalmente.
- **Steam Deck/Lutris:** Sider deja de cargar después de actualizar SteamOS o Lutris.

**Problema de distribución:** en abril de 2026 un comentario dice que la web de Smoke mostraba "blog removed" ([pesmodding](https://www.pesmodding.com/2025/10/sp-football-life-2026.html)). Hoy la web está activa (la consulté). **[INCIERTO]** si fue una caída temporal.

### 1.2 Evoweb Patch

**Quién lo hace:** Adrian2780 y Cesc Fabregas, con ayuda de Hawke, ninet y scottish_carson ([pesmodding](https://www.pesmodding.com/2024/06/pes-2021-evoweb-patch-2024-version-30.html)).

**Última versión encontrada:** **Evoweb Patch 2024 v3.0 (16 de junio de 2024)**, más el "Kit Update Vol. 1". **[INCIERTO]** si salió alguna versión 2025 o 2026: no la encontré. Parece que el proyecto está parado desde mediados de 2024.

**Qué incluye:**
- Euro 2024 y Copa América 2024: plantillas, kits y minicaras.
- Brasileirão A y B reconstruidos.
- Contratos y valores de mercado de las ligas grandes.
- Carpeta de caras.
- "Jugadores con IDs reales" (corrigieron IDs de jugadores para que coincidan con los oficiales).

**Estructura (todo con Sider):**
- 6 `cpk.root`: Boots-Gloves-AddOn, Boots-Gloves, Database, Faces, Graphics y Kits.
- Un `EDIT00000000` que va en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<id>\save`.
- Un módulo `MiniFaceServer.lua`.
- Exige DLC 7 y vaciar las carpetas WEPES/mount de cualquier option file anterior.

**Problemas:**
- La base de datos va en **bins**, así que hay que desactivar cualquier otro mod que toque bins de base de datos. Por ejemplo, el **ML Manager Mod de SoulBallZ** no funciona hasta que su autor lo actualice ([v2.0](https://www.pesmodding.com/2024/05/pes-2021-evoweb-patch-2024-version-20.html)).
- Cargas infinitas en las copas de la Liga Máster (el autor dice: copias piratas o restos de otros parches).
- No es compatible con FL2024.
- Partes del RAR faltantes y descargas lentas.

### 1.3 SmokePatch 21 (el SmokePatch "clásico" para PES 2021)
- Fue el producto de Smoke **antes** de que todo pasara a Football Life. Ejemplos: "Inferno Smoke Patch", 2022 ([pesmodding](https://www.pesmodding.com/2022/08/efootball-pes-2021-inferno-smoke-patch.html)); option files Smoke para PES 2021 en 2023–2024 ([pesmodding](https://www.pesmodding.com/2024/02/pes-2021-smoke-patch-option-file-winter.html)).
- **Hoy SP21 está, en la práctica, sustituido por Football Life. [INFERENCIA]**
- Un problema conocido de 2022: después de instalar Smoke ya no se podía aplicar la update online ("problemas con el servidor") ([Steam](https://steamcommunity.com/app/1259970/discussions/0/3323114398565714368)).

### 1.4 ConmeGOL Patch (latinoamericano)

**Quién lo hace:** "CONMEGOL PATCH" y "TAVO RTX".

**Última versión:** **ConmeGOL Patch 27** (publicado el 23 de septiembre de 2026), temporada 2027 ([pesfreedownloads](https://www.pesfreedownloads.com/2026/09/conmegol-patch-27-el-mejor-parche.html), [ketubanjiwa](https://www.ketubanjiwa.com/2026/09/pes-2021-conmegol-patch-27-aio-latinoamericano-season-2027.html)).

**Qué incluye:**
- Ligas de Argentina, Brasil, Chile, Colombia, Ecuador, Paraguay, **Perú**, Uruguay, Bolivia, Venezuela y Costa Rica, más segundas divisiones.
- Las ligas grandes de Europa.
- Competiciones CONMEBOL.
- Kits, escudos, caras y estadios nuevos.
- Scoreboard de la LPF argentina mediante `SiderAddons\content\scoreboard-server\...`.

**Estructura:** es un **AIO** ("todo en uno") con el **juego ya modificado incluido**. Ocupa 109 GB comprimido y **234 GB instalado**. Se instala en orden: base, parche, base de datos, caras (7 partes) y estadios (7 partes).

**Problemas:**
- No se debe mezclar con otros mods.
- Las minicaras que faltan se piden por Discord.
- El enorme tamaño complica descargarlo e instalarlo.
- Como el juego viene incluido, **[INCIERTO]** su situación legal y si su exe es original. Ojo con esto.

### 1.5 Gogosz Patch (brasileño/sudamericano)

**Versiones (octubre de 2026):**
- World v12.0 (base) más la update 12.5.
- CONMEBOL v4.0 más la update 4.5.
- Legends v4.0 más la update 4.3.
- World Cup 2026 v1.x.

Fuente: [caocacao.net](https://caocacao.net/pes-2021-gogosz-patch/).

**Qué incluye:**
- Narración en portugués, estadios 4K, caras 3D y scoreboards.
- "Staff y árbitros reales en Liga Máster PRO" y un modo "Konami Cup".
- Legends es una edición nostálgica (Mundial 2002) sin Liga Máster ni Ser una Leyenda.

**Tamaños:** World unos 228 GB y CONMEBOL unos 201 GB.

**Problemas:**
- Partes de la descarga rotas.
- **Carga infinita** si no usas la versión de Steam: la respuesta oficial es que hace falta la copia de Steam, no un crack.

### 1.6 Packs sudamericanos sueltos ("Sudamericana", "South America Football Patch")
- "PES 2021 Sudamericana Facepack 2026 Update v3", de PES_Triste (26 de enero de 2026):
  - Requiere **Sider 7.3.3**.
  - Cada cara va en `livecpk\Faces\Asset\model\character\face\real\<ID>`.
  - También funciona en PES 2020 con Sider 6.4.3 ([pes-files](https://pes-files.com/pes-2021-sudamericana-facepack-2026-update-v3/)).
- Existe un "South America Football Patch" en Patreon ([Patreon](https://www.patreon.com/SouthAmericaFootballPatch)). **No pude leer su contenido. [INCIERTO]**
- **"Sudamerican Project 2026": [NO ENCONTRADO].** No hallé ninguna fuente con ese nombre exacto. Puede ser un nombre de YouTube o de Facebook que no está indexado.

### 1.7 VirtuaRED y UML (Ultimate Master League)

**VirtuaRED 2024 v8.5 (19 de junio de 2024)** ([pesmodding](https://www.pesmodding.com/2024/06/pes-2021-virtuared-2024-patch-v85.html)):
- **10.177 caras** y **29.718 minicaras**.
- Más de 100 jugadores creados.
- 467 banderines de córner y estadios actualizados.
- Scoreboards, menús y entradas al campo.
- Las instrucciones están en su foro (virtuared.com).

**UML** (PogChampion, jovic1901 y cYNIC): una base de datos "ultra realista" que se pone **encima** de VirtuaRED ([pesmodding](https://www.pesmodding.com/2023/06/pes-2021-ultimate-master-league-uml.html)).
- Un usuario de Steam dice que con UML más el gameplay de Holland lleva más de 3 temporadas estables, y que los crashes venían de la base de datos de VRED ([Steam](https://steamcommunity.com/app/1259970/discussions/0/3829791007681480550)).

**Fecha de la última versión: [INCIERTO]** para 2025–2026.

### 1.8 Otros parches o mods activos en 2026
- **Soccer Revolution 2026 V12**, de elijio876 (25 de marzo de 2026) ([pesmodding](https://www.pesmodding.com/2026/03/pes-2021-soccer-revolution-2026-v12.html)):
  - Menú y UIColors con Sider (`cpk.root` más el módulo `UIColors.lua`).
  - Presets de **ReShade 6.7.1** (d3d10/11, "sin shaders extra").
  - **Reemplaza `PES2021.exe`, `dt18_all` y `dt13_all`** para cambiar el gameplay.
- **Gameplay "Winning Eleven 2026"**, de Holland (marzo de 2026): también **reemplaza el exe**. Pide verificar los archivos en Steam antes y hacer copia de seguridad ([pesmodding](https://www.pesmodding.com/2026/03/pes-2021-new-gameplay-mod-winning.html)).
- **SimSnob Experience 5.2** y **Movement Project Update 12**: mods de gameplay ([índice de evoweb](https://evoweb.uk/forums/pes-2021.337/)).
- **ML Team Selector** (Sider Lua, de Geazi Gonçalves): permite elegir uno de unos 39 clubes en la Liga Máster ([pesmodding](https://www.pesmodding.com/2025/05/pes-2021-master-league-selector.html)).
- **Age Lock v1** para la Liga Máster.
- **Sleeve/Armband Server 4.0** (junio de 2026): parches de la manga del Mundial 2026 ([pesmodding](https://www.pesmodding.com/2026/06/pes-2021fl26-sleeve-patch-server-40.html)).
- **Parche asiático:** el ICMP Patch (2021) ([pesmodding](https://www.pesmodding.com/2021/06/pes-2021-icmp-patch-v20.html)). **[INCIERTO]** si hay un parche indonesio activo en 2026. Solo encontré un pack de "Save, sider, facepack Season 2026" en Gumroad ([gumroad](https://elraska.gumroad.com/l/hwlysu)).
- **Parche peruano:** no encontré un parche de PC dedicado a la Liga 1 de Perú en 2025–2026. Hay uno de 2023 para PS4/PS5 ([Líbero](https://libero.pe/esports/2023/05/19/liga-1-en-tus-manos-pes-realizo-parche-temporada-2023-futbol-peruano-totalmente-gratis-zona-gamer-play-station-4-play-station-5-469566)). En PC, la Liga 1 de Perú viene dentro de ConmeGOL 27 y del kitpack DN7 (Perú). **Esto es un hueco que Phoenix podría ocupar.**
- **"Next Season Patch", "eFootball World", "Super Patch" y "Mod Zone": [NO VERIFICADOS].**
  - No encontré fuentes fiables con esos nombres para PES 2021 en PC.
  - Hay páginas de "pre-order" pagado (carrd y gumroad con "2025/26 Full Season Update - PES 2021") que **parecen poco confiables**: [carrd](https://pespatchpreorder.carrd.co/). No recomiendo usarlas como referencia.
- **eFootball 2027 "ePatch" y "Master League for eFootball 2027"** ([pesmodding](https://www.pesmodding.com/)): son para **eFootball**, no para PES 2021. No hay que confundirlos.

### 1.9 PESBUL: online restaurado
- PESBUL ([pes2021.pesbul.eu](https://pes2021.pesbul.eu)) reactiva el **lobby de partidos amistosos** de PES 2021. Se instala con un `.bat` que se ejecuta como administrador: la opción 1 instala y la opción 2 deshace los cambios.
- Los partidos son **P2P**: el tráfico del partido va directo entre los dos jugadores.
- Está en beta pública.
- **Problemas conocidos:**
  - Avatares rotos.
  - Si alguien se desconecta, el rival queda en una sala que ya no existe.
  - El estado de las salas no se actualiza bien.
  - Team Play Lobby y Clubs CO-OP sin terminar.
  - Verificación opcional del perfil mediante un código que se escribe en el campo "Equipo favorito".
- **No dice nada** sobre si es compatible con FL o con otros parches.
- Fuente: [pesmodding, 30/07/2026](https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html).
- **Relevante para Phoenix Link:** la alternativa que ya usa la comunidad para jugar a distancia es Parsec. Un usuario se queja de que es lento ([Steam](https://steamcommunity.com/app/1259970/discussions/0/3323114398565714368)).

---

## 2. Causas de crash más comunes y sus soluciones (resumen de todas las fuentes)

| # | Síntoma | Causa | Solución | Fuente |
|---|---|---|---|---|
| 1 | Pantalla negra o blanca, o crash al cargar caras | Falta memoria virtual: muchas caras o texturas de golpe | Subir el archivo de paginación de Windows (administrado por el sistema) | [FAQ de FL](https://www.pessmokepatch.com/2022/12/FAQ.html) |
| 2 | Carga infinita antes del partido o en copas de la Liga Máster | Copia pirata o repack, restos de otros parches, o DpFileList generado para un DLC equivocado | Steam legal + DLC 7, instalación limpia, regenerar DpFileList eligiendo el DLC más reciente | [Evoweb](https://www.pesmodding.com/2024/06/pes-2021-evoweb-patch-2024-version-30.html), [Gogosz](https://caocacao.net/pes-2021-gogosz-patch/), [DpFileList](https://www.pesmodding.com/2021/02/pes-2021-dpfilelist-generator-dlc-40.html) |
| 3 | Bucle infinito o crash en Intel de 12.ª gen o posterior | DRM con los núcleos P/E | Copia legal actualizada o parche de Intel; en FL: el lanzador adecuado (FL23–25) o FL26 | [Tom's HW](https://www.tomshardware.com/uk/news/intel-fixes-most-drm-issues-with-alder-lake), [evoweb](https://evoweb.uk/threads/efootball-pes-2021-discussion-thread-pc.84053/post-4154907), [SP](https://www.pessmokepatch.com/p/compatibility.html) |
| 4 | Pantalla blanca y cierre a los 5 s, o error "guardando datos del sistema" | Drivers de AMD o NVIDIA; Windows Defender bloquea el guardado | Volver a otro driver o actualizarlo; añadir `PES2021.exe` a las exclusiones del antivirus | [Steam](https://steamcommunity.com/app/1259970/discussions/0/4436564907318510604) |
| 5 | Sale la pantalla de ajustes en cada arranque | `SYSTEM00000000` corrupto o Sider dentro de la carpeta del parche | Borrar o respaldar ese archivo, verificar en Steam y poner Sider en el Escritorio | [evoweb](https://evoweb.uk/threads/efootball-pes-2021-discussion-thread-pc.84053/post-4154907) |
| 6 | Partida corrupta o `SYSTEM` sobrescrito | Bug de `save.folder` en Sider 7.4.0 | Actualizar a **Sider 7.4.1** | [pesmodding](https://www.pesmodding.com/2026/04/pes-2021-sider-v741.html) |
| 7 | Crash o corrupción de memoria con módulos de overlay | Módulos con el viejo hack de colores en Sider 7.4 o superior | Quitarlos o actualizarlos (`colorsdemo2.lua`) | [pesmodding](https://www.pesmodding.com/2026/04/pes-2021-sider-v740.html) |
| 8 | Bugs en la carrera o la Liga Máster, jugadores "sin nombre, media 40" | Mezclar mods de base de datos (bins) o un option file de otra versión | Una sola base de datos; option file de la misma versión exacta | [Evoweb](https://www.pesmodding.com/2024/05/pes-2021-evoweb-patch-2024-version-20.html), [FL25 OF](https://www.pesmodding.com/2025/01/sp-football-life-2025-option-file-fl25.html) |
| 9 | Pantalla de carga atascada | Módulo de público con asistencia local muy baja (cerca del 10 %) | Usar 50 % o más | [Crowd Attendance](https://www.pesmodding.com/2026/10/pes-2021-crowd-attendance-module.html) |
| 10 | Antivirus borra o bloquea archivos | `.bat`, inyección de Sider, exe modificados | Exclusiones; distribuir con hashes | [SP compat](https://www.pessmokepatch.com/p/compatibility.html), [PESBUL](https://www.pesmodding.com/2026/07/how-to-enable-pes-2021-online-mode.html) |
| 11 | Error de instalación o EDIT que falta | Documentos en OneDrive "solo nube"; sin permisos de administrador | "Mantener en este dispositivo"; ejecutar como administrador | [FAQ de FL](https://www.pessmokepatch.com/2022/12/FAQ.html) |
| 12 | Error de "archivos que faltan" al arrancar | Faltan las librerías de DirectX, VC++ o .NET 4.5 | Instalarlas | [FAQ de FL](https://www.pessmokepatch.com/2022/12/FAQ.html) |
| 13 | No arranca después de instalar comentarios o mods de audio | Paquete de audio incompatible | Instalar de a un mod por vez y probar | [Essentials](https://www.pesmodding.com/2025/11/sp-football-life-2026-v12-essentials.html) |
| 14 | Option file "incompatible" después de una update | La versión del EDIT no coincide con el exe/DLC | Herramienta "EDIT Version Changer" (eliges exe y DLC) | [pesmodding](https://www.pesmodding.com/2021/05/pes-2021-edit-version-changer.html) |

---

## 3. Buenas prácticas para un parche de PES 2021 sin crashes

### 3.1 Una sola base técnica, fija y declarada
- **Objetivo:** Steam legal, **exe 1.07.02 + DLC/Data Pack 7.0**, nada más. Comprobar la versión del exe al arrancar (con el lanzador o un módulo) y negarse a seguir si no coincide.
- **No dar soporte a repacks.** Es la primera causa de cargas infinitas según varios autores.
- Si modificas el exe (gameplay), hazlo **como un parche de bytes sobre 1.07.02 con verificación de hash**, no distribuyendo un exe entero. Y deja un "switcher" para volver al exe original, como el `SP switcher.exe` de FL.

### 3.2 Sider: una versión fija y en un lugar seguro
- Fijar **Sider 7.4.1** (o la que elijas) e incluirla dentro del parche. Nunca "usa la última".
- Revisar todos los módulos Lua contra 7.4 o superior: **nada de hacks viejos de color del overlay**.
- Usar **`save.folder`** (7.4.1 o superior) para que Phoenix tenga **su propio `EDIT00000000`** y no pise el del usuario ni el de otros parches.
- Sider siempre fuera de las carpetas del sistema y de las del parche: Escritorio, Documentos o la carpeta personal.
- **Orden de los módulos:** algunos dependen del orden de carga (Custom Celebrations debe ir **antes** de GoalScenes). Documenta el orden en `sider.ini` y no lo cambies.
- **Prioridad de `cpk.root`:** Sider busca en el orden en que aparecen las raíces (la primera que tiene el archivo gana). **[INFERENCIA, verificar en el README de Sider]** Pon arriba los parches pequeños y concretos (fixes) y abajo los packs grandes.

### 3.3 CPK y DpFileList
- **Máximo 47 CPK** en DpFileList, según el generador clásico ([pesmodding](https://www.pesmodding.com/2021/06/pes-2021-new-dpfilelist-generator-dlc-70.html)). **[INCIERTO]** si es un límite del juego o solo de esa herramienta. Aun así, quedarse muy por debajo (20–30) es lo prudente.
- Generar DpFileList **siempre para el DLC más reciente**: si no, hay cargas infinitas.
- Preferir **livecpk de Sider** para todo lo que cambia a menudo (kits, caras, scoreboards). Reservar los CPK para lo estable.
- **Tamaño máximo de un CPK: [INCIERTO].** No encontré una cifra documentada. Por prudencia, mantener cada CPK por debajo de unos 4 GB y separarlo por tipo (faces, kits, stadiums, ui).

### 3.4 Memoria
- PES 2021 es un juego de **64 bits**, así que el famoso límite de "4 GB / Large Address Aware" de los PES antiguos de 32 bits **no aplica**. **[INFERENCIA]** Los "out of memory" de PES 2021 son de **memoria virtual (archivo de paginación) y de VRAM**.
- FL pide **10 GB de RAM**, 2 GB de VRAM como mínimo y 6 GB recomendados (10 GB para 4K) ([FL26](https://www.pessmokepatch.com/2025/10/spfl26.html)). Usa cifras parecidas si Phoenix trae muchas caras o estadios.
- Caras: VirtuaRED llega a ~10.000 caras y ~30.000 minicaras sin que se reporte un límite "duro" ([VRED 8.5](https://www.pesmodding.com/2024/06/pes-2021-virtuared-2024-patch-v85.html)). El problema práctico es **el pagefile**. Recomienda paginación administrada por el sistema y texturas DDS comprimidas, no en 4K a lo loco.

### 3.5 Base de datos, equipos e IDs
- **Una sola fuente de verdad para la base de datos.** Tanto FL como Evoweb avisan de que mezclar mods de bins de base de datos rompe la carrera. Si el **Phoenix Sync** edita la base de datos o la Liga Máster, que lo haga **solo** sobre la base de Phoenix, y que **detecte** si hay otras bases de datos instaladas.
- **Estructura del option file (`EDIT00000000`), según el código de 4ccEditor** ([GitHub](https://github.com/the4chancup/4ccEditor)):
  - Está **cifrado** (usa `libpesXcrypter` con una clave específica de PES21).
  - El número de equipos y de jugadores va en la **cabecera**.
  - Cada equipo tiene **40 huecos de jugador** en el roster (con 40 dorsales).
  - Los IDs de equipo y de jugador son de 32 bits.
  - El nombre del jugador ocupa un campo de 61 caracteres anchos y el nombre de la camiseta 21 bytes. **[Verificar el máximo real dentro del juego.]**
- **Límites de IDs de equipos y de competiciones: [INCIERTO].** No encontré una fuente fiable con el rango exacto de IDs ni el número máximo de equipos o competiciones de PES 2021. Lo que sí está documentado:
  - FL quita los "topes" de Argentina (26/24 → 30) y monta un Mundial de 48 equipos, **pero modificando el exe** ("integrado en el juego").
  - **Conclusión:** los límites existen en el exe original y FL los parchea. Si Phoenix quiere ligas de más de 20 equipos o formatos nuevos, necesitará parches del exe (o módulos Lua que localicen el código por patrones, como hace Crowd Attendance: "no usa direcciones fijas, localiza el código del juego").
- **IDs reales de jugadores:** Evoweb migró a "IDs reales" (las de Konami) para que caras y minicaras coincidan con las de otros packs. Hazlo también: así cualquier facepack de la comunidad (con carpetas `face\real\<ID>`) encaja sin conflictos.
- **Guarda la versión del option file** junto con la del exe y el DLC, porque si no coinciden el juego no lo carga. Existe el "EDIT Version Changer" ([pesmodding](https://www.pesmodding.com/2021/05/pes-2021-edit-version-changer.html)).

### 3.6 Instalación y actualizaciones (copiar a FL)
- Usar un **instalador propio** que:
  - instale en una carpeta nueva,
  - compruebe Documentos y OneDrive,
  - pida permisos de administrador,
  - y respalde `EDIT` y `SYSTEM`.
- **Clasificar cada update** según cómo afecta a la carrera, igual que FL: "no afecta a la carrera", "necesita partida nueva", o "se guarda para la próxima temporada".
- **Packs de addons atados a una versión exacta.** Ejemplo: el pack Essentials de FL solo sirve para la 1.2 y se rompió con la 2.0. Phoenix debería comprobar la versión de cada addon al arrancar.
- Un aviso en el lanzador cuando hay una update, como hace FL.

### 3.7 DirectX, ReShade y gráficos
- El juego usa **DirectX 11**. ReShade debe instalarse en modo **d3d10/11**, "sin shaders extra" ([Soccer Revolution](https://www.pesmodding.com/2026/03/pes-2021-soccer-revolution-2026-v12.html)). Y ReShade siempre opcional.
- En portátiles: forzar la **GPU dedicada** también para `settings.exe`.
- **Interacción con Phoenix Link (Parsec): [INFERENCIA].** ReShade y los overlays (Sider, Steam, Parsec) se meten todos en el mismo proceso de dibujo de DirectX 11. Prueba siempre la combinación Phoenix Link + Sider + ReShade, y deja ReShade apagado por defecto en las partidas en línea.

### 3.8 Windows, Steam y antivirus
- Pedir **Windows actualizado**, los drivers de GPU al día y las librerías de DirectX, VC++ y .NET instaladas.
- **Antivirus:**
  - Sider (porque se inyecta en el juego), los `.bat` y los exe parcheados dan **falsos positivos**.
  - Firmar los binarios propios si se puede, publicar los hashes SHA-256 y evitar los `.bat` (usar un lanzador `.exe` propio).
  - Explicar cómo añadir la carpeta a las exclusiones del antivirus.
- **No modificar** los archivos que Steam verifica. Si se modifican, avisar de que "Verificar integridad" los devolverá a su estado original.

### 3.9 Proceso de pruebas
- Probar siempre desde una **instalación limpia de la versión por defecto**, que es lo primero que pide la FAQ de FL. Después añadir los mods **de uno en uno**.
- Escenarios mínimos de prueba:
  1. Arranque en frío.
  2. Amistoso.
  3. Liga Máster: crear, pasar una temporada y el mercado de fichajes.
  4. Copa de la Liga Máster.
  5. Ser una Leyenda.
  6. Edit mode: importar un PNG.
  7. Carga y guardado del option file.
  8. Partido en línea (Phoenix Link y PESBUL).

---

## 4. Ideas concretas para el Phoenix Evolution Patch
1. **Ocupar el hueco peruano y andino.** No hay un parche de PC dedicado a la Liga 1 de Perú en 2025–2026. ConmeGOL lo trae, pero dentro de un AIO de 234 GB.
2. **Ser ligero y modular.** Frente a los AIO de más de 200 GB (ConmeGOL, Gogosz): un núcleo pequeño (base de datos + EDIT + Sider fijado) y addons opcionales con versión (caras, estadios).
3. **Lanzador propio** (como FL):
   - comprueba exe 1.07.02, Sider y versiones de addons,
   - configura el pagefile o avisa,
   - activa `save.folder`,
   - y conecta con la web y el Mercado.
4. **Mercado y Liga Máster** siempre sobre la base de datos de Phoenix, con copia de seguridad automática del EDIT y del save antes de cada escritura (como hace FLCC).
5. **Online:** documentar si convive con PESBUL (el `.bat` cambia la configuración de red del juego) y con `netblock.lua` de Sider (que **bloquea** comprobaciones online). **Posible conflicto [INFERENCIA]:** si usas netblock, PESBUL no funcionará.
