# Caja de herramientas · Phoenix Evolution Patch (PES 2021 / Football Life)
Investigado el 30-oct-2026. Fuentes: EvoWeb, PES Modding, PeSoccerWorld, pes-files, Nexus, GitHub, YouTube, X, wiki Rigged.
Regla: probar todo en una COPIA del juego. Nada de esto se instala sin aprobar.

---

## ⚠️ 0. Lo que toca directo a Phoenix (leer primero)

| Herramienta | Qué hace | Por qué importa | Dónde |
|---|---|---|---|
| **My Football Legacy** | Web app: Liga Máster online con amigos (liga, mercado, finanzas, prensa). Juegas los partidos en tu juego, por Parsec o sofá. Solo paga quien crea la partida | **Es casi la idea de tu Liga Máster web.** Competencia directa: estudiar qué hace bien | myfootballlegacy.com · hilo EvoWeb "Online Master League & UCL 36 teams with Friends" |
| **Touchline (ahmadsk-cell)** | Compañero de Liga Máster para FL26: lee el save, crea prensa, junta directiva, despidos, aforo dinámico, dorsales; escribe al juego en vivo con módulos Lua | Mismo patrón que Phoenix Mercado: leer save + escribir con Sider | github.com/ahmadsk-cell/Touchline (releases, v0.9.9.4 del 25-oct-2026) |
| **Prologue (mismo autor)** | Igual que Touchline, pero para Ser una Leyenda (BAL) | Ideas para modo jugador | askandalus.com |
| **PESM (HollywoodKills, en desarrollo)** | Football Manager completo corriendo en vivo sobre la Liga Máster | Seguir de cerca | Hilo EvoWeb "[WIP] PESM" (8-oct-2026) |
| **pes2021-transfer-tool (kickoffsage)** | Aplica traspasos al EDIT y descarga fichajes de Transfermarkt | Igual que Mercado: comparar código | github.com/kickoffsage/pes2021-transfer-tool |
| **boggylp/pes** | Kit en Go: leer/extraer .cpk, descifrar/editar/cifrar EDIT, sacar plantillas de Player.bin, detectar caras con ID mal, añadir espacios de camisetas | Piezas que Mercado ya hace; útil para validar | github.com/boggylp/pes |
| **Football Life Mod Studio (mataodzanata)** | Agrega ligas nuevas a la Liga Máster de FL26; v0.2.5 trae CONMEBOL moderno | **Justo lo que pediste: más ligas** | Hilo EvoWeb "[FL26] Football Life Mod Studio" |

---

## 1. Datos del mundo real (ligas, equipos, jugadores)

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **openfootball** | Clubes, estadios, ligas y copas del mundo. Dominio público | github.com/openfootball (clubs, leagues, world, football.json) |
| **transfermarkt-datasets** | 37.000+ jugadores, traspasos, valores; actualización semanal | github.com/dcaribou/transfermarkt-datasets |
| **soccerdata (Python)** | Un solo lugar para FBref, Sofascore, SoFIFA, Transfermarkt… | github.com/probberechts/soccerdata |
| **PESBuilder / PESDatabase (kisni07)** | Base de eFootball actualizada cada jueves; convierte stats eFootball → PES 2021; exporta CSV para el editor de ejogc | Hilo EvoWeb "[RELEASE] PESBuilder v0.3.1" (25-oct-2026) |
| **CSVs convertidos de eFootball** | Bases ya convertidas | Foro EvoWeb, sección Database (31-mar-2026) |
| **Player ID Collection v5.4.0** | IDs de jugadores de PES 4 a eFootball 2026 | Foro EvoWeb, Database |
| **PES Stats Converter Tool** | Convierte stats entre versiones | Foro EvoWeb, Tools |
| **API-Football / Sportmonks** | APIs con 1.200+ / 2.200+ ligas (gratis limitado / pago) | api-football.com · sportmonks.com |
| **Reglamentos oficiales** | Formato, ascensos, cupos (CBF, AFA, FPF, Conmebol, federaciones estatales) | Web de cada federación |

## 2. Base de datos del juego (equipos, ligas, competiciones)

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **PES Editor (ejogc327)** | Editor principal de Player/Team/Competition .bin; importa CSV | ejogc327.blogspot.com · tutorial en hilo EvoWeb "EJOGC Pes Editor - Database tutorials" |
| **DinoTem Editor (lagun-2)** | Edita Competition, CompetitionEntry, CompetitionRegulation, Derby, Stadium, Tactics… | welovepes.com |
| **Competition-Server (Gerlamp)** | Segundas divisiones con ascenso/descenso reales (Lua) | PES Modding |
| **UCL36 / UCL 36 Teams** | Formato nuevo de Champions y Europa League (36 equipos) | EvoWeb · pes-files |
| **Database Module (GRAND)** | Cambia nombres, licencias y patrocinadores de clubes con CSV simples | PES Modding |
| **Team / Competition Licencer (zlac)** | Licenciar/deslicenciar equipos y torneos | pesnewupdate.com |
| **4ccEditor** | Editor del EDIT, código abierto, pasa equipos entre versiones | github.com/the4chancup/4ccEditor |
| **TEditor web** | Edita .TED en el navegador; cambia ID de equipo para moverlo de liga/continente | teditor.app |
| **PES 2021 Player Data Editor (Devil Cold52)** | Edita jugadores, equipos, estadios, ligas, entrenadores en el EDIT | PES Modding |
| **Wiki "Edit file" (Rigged)** | Mapa byte a byte del EDIT de PES 2021 | implyingrigged.info/wiki/Pro_Evolution_Soccer_2021/Edit_file |
| **4chan cup – scripts de base de datos** | Generan bases de datos en Python | github.com/the4chancup |
| **CallnameServer v2.0 + Manager** | Nombres que dice el narrador | Foro EvoWeb (PES ULTRA) |
| **STR Editor (Devil Cold52)** | Cambia textos del juego (torneos, menús) | PES Modding |

## 3. Camisetas, logos y texturas

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **PES Master Kit Creator** | Camisetas en minutos con plantillas; 2048×2048 | pesmaster.com/kit-creator |
| **PES Texture Converter v2.0.1 (Maker Dreamer)** | PNG/DDS/FTEX en lote con arrastrar y soltar | patreon.com/MakerDreamer (anuncio en X, ago-2026) |
| **Ftex Converter 2.0 (Devil Cold52)** | FTEX ↔ DDS; PNG/JPG → DDS | balkanpesbox.com |
| **Kit Manager PES2021 edition** | Configura camisetas (colores, cuello, números) | Wiki Rigged → Tools |
| **Kit Studio (zlac)** | Configuración de kits y Kitserver | EvoWeb |
| **Kitserver: mayúsculas/minúsculas y mover nombres/números** | Ajustes finos de dorsales y nombres | Foro EvoWeb (pedritos, oct-2026) |
| **Tattoo UV Builder Tool (stoo)** | Tatuajes | Foro EvoWeb (oct-2026) |
| **CGPE (shawminator)** | Abre/edita .cpk | EvoWeb |
| **PES CPK TOOL (StpN_17)** | Arma/extrae .cpk arrastrando carpetas | PES Modding |

## 4. Caras y minicaras

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **Player Real Face Manager (Ruls)** | Revisa 1000+ jugadores, busca caras en Google, exporta packs | PeSoccerWorld |
| **Face ID Relinker (Ruls)** | Cambia el ID de una cara sin perder texturas, sin Blender | PES Modding · kazemario.com |
| **Face Relinked con vista 3D (About PES)** | Relink con vista previa | PES Modding |
| **PES 21 Face Switcher (AlexDelPiero10)** | Cambiar caras rápido | Foro EvoWeb (sep-2026) |
| **Miniface Scrapper (Ruls)** | Descarga minicaras automáticas | PES Modding |
| **MiniFace Maker v1.1 (Nobody)** | Crea minicaras en segundos | PeSoccerWorld |
| **Flujo IA para caras** | ChatGPT (foto) → AvatarSDK (3D) → Wrap → Blender | Hilos EvoWeb / SoccerGaming "AI-Powered Face Making" |
| **Head Rescale Tool (Football4Life)** | Ajusta tamaño de cabezas | Foro EvoWeb |

## 5. Estadios, marcadores, menús, presentación

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **Stadium Server v1.80 (zlac)** | AddOn, .geom, multi-ID (publicidad/TV), vallas con rebote | PES Modding |
| **PES Stadium Exporter (Blender)** | Exportar estadios desde Blender 2.90–5.0 | github.com/MjTs140914/PES_Stadium_Exporter |
| **Stadium Manager / Stadium Checker** | Gestiona y revisa map_teams.txt | PES Modding |
| **Stadium ID Changer (MJTS)** | Cambia ID de estadio | pesnewupdate.com |
| **A Tool for making scoreboard** | Crear marcadores | Foro EvoWeb (jul-2026) |
| **ScoreboardServer / P4ScoreboardServer** | Un marcador por competición | PES Modding |
| **MenuServer, Entrance, Tunnel, Intro Server** | Menús, entradas, túneles e intros por competición | PES Modding / EvoWeb |
| **CustomExhibitionMatch.lua** | Hace que un amistoso "sea" una competición (clave para Parsec) | PES Modding |
| **Font Server (Aoba)** | Fuentes por competición | Foro EvoWeb (may-2026) |
| **Anthem Animation Style Editor** | Cómo forman en el himno | PES Modding (oct-2026) |
| **Anthem Jacket Mod (Ruls)** | Chaquetas en el himno | Foro EvoWeb (oct-2026) |
| **Crowd Attendance 2.0** | Aforo de público en cualquier modo, incluido amistoso | pes-files |
| **Goalnets Module v0.6.1** | Redes por equipo/competición/fase | pes-files |
| **Turf-loader v4.1 / light-loader** | Césped e iluminación | Foro EvoWeb |
| **Sider Mod Menu v1.2** | Menú de Sider con mouse | Foro EvoWeb (oct-2026) |
| **Camera Switcher / Penalty Camera** | Cámaras | Foro EvoWeb |

## 6. Gameplay, narración y Liga Máster

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **TBG Gameplay Parametric Editor** | Edita parámetros del dt18 y exporta | PES Modding |
| **dt18 editor (4chan cup)** | Edita binarios de gameplay | github.com/the4chancup |
| **Anti-Cheat (Holland)** | Quita el "scripting" (dificultad dinámica = 0) | PES Modding |
| **Dynamic Difficulty Levels** | Niveles de dificultad dinámicos | Foro EvoWeb (oct-2026) |
| **Fair Player Ratings** | Calificación justa al final del partido | pes-files |
| **Gameplay Lua (xxdangixx)** | Física del balón, reacción, cansancio, arqueros | pes-files |
| **Fakecid AI Overhaul** | IA con aprendizaje automático | Foro EvoWeb (sep-2026) |
| **Commentary Server v2.0** | Narración por servidor | Foro EvoWeb |
| **Dynamic Commentary v2.0** | Cambia idioma del narrador según competición | pes-files / EvoWeb |
| **Comentarios estilo EA FC 27** | Narración nueva | Foro EvoWeb (oct-2026) |
| **Chatty Tactics Server / Auto Lineup** | Tácticas y alineaciones automáticas | Foro EvoWeb |
| **Live Editor (Ruls)** | Edita jugadores y presupuestos en vivo | PES Modding · github.com/xAranaktu/PES-2021-Cheat-Table |
| **Stat Editor v35 (JamesHoward333)** | Panel de stats dentro del juego | pes-files |
| **UltimateYearChanger 2.1 / StartingYearChanger** | Empezar Liga Máster en cualquier año | Foro EvoWeb · pes-files |
| **ML Fast Forward v1.2** | Saltar fechas en Liga Máster | Foro EvoWeb |
| **ML Transfer Assistant (DcV253)** | Ayuda de fichajes en Liga Máster | Foro EvoWeb |
| **Age Lock / Better Regens / Real Injuries** | Edad, juveniles generados, lesiones | Foro EvoWeb |
| **PES2021 ML Editor** | Presupuesto y seguidores de la Liga Máster (código abierto) | github.com/felipemikeias/pes21-ml-editor |
| **PES2021 Mod Studio (blue41)** | Cánticos propios y editor de BAL | github.com/LimaDiogenes/blue41_PES2021_MOD_STUDIO |
| **BAL Editor & Face Relink (AlexDelPiero23)** | Editor completo de Ser una Leyenda | PeSoccerWorld |

## 7. Base del sistema

| Herramienta | Qué hace | Dónde |
|---|---|---|
| **Sider 7.4.1 (juce)** | Base de todo; carpeta de save propia por parche | PES Modding / welovepes.com |
| **DpFileList Generator v1.3 (Baris)** | Registra .cpk en el juego | welovepes.com |
| **pes-fmdl (Blender)** | Modelos 3D | github.com/the4chancup |
| **PES File Crypter (Devil Cold52)** | Descifrar/cifrar EDIT (pronto con llave de FL27) | pespatchs.com |

---

## Dónde seguir buscando (lo "under")
- **EvoWeb → foro PES 2021**, prefijos [TOOL], Tools, Sider Module, Database: evoweb.uk/forums/pes-2021.337/
- **pes-files.com → Editing Tools** (casi diario) · pes-files.ru (versión rusa, a veces antes)
- **PeSoccerWorld → Herramientas** (41 herramientas)
- **PES Modding → etiqueta "PES 2021 Tools"**
- **GitHub → tema `pes`** y búsqueda "pes2021"
- **Nexus Mods → eFootball PES 2021**
- **Wiki Rigged → Tools**
- **Balkan PES Box** (foro)
- **X/Twitter**: modders como @kitmakerdreamer, @UMLmod
- **Discord**: "PES 21 Patchs (PC)" (21k), Discords de UML, ConmeGOL, Sudamerican Project

## Correcciones a documentos anteriores
- VirtuaRED NO está parado: salió **v10.3 el 26-jul-2026** (hilo EvoWeb).

---

## 8. Ronda 2 de búsqueda (30-oct-2026, noche)

### ⭐ Las más importantes de esta ronda
| Herramienta | Qué hace | Por qué importa | Dónde |
|---|---|---|---|
| **FootballLife-new-leagues + FL26 Mod Studio (Matozanato)** | Ligas, clubes y **países nuevos** en la Liga Máster, sin parchear el exe ni rehacer .cpk (todo por módulos de Sider). Sube la tabla de competiciones de 100 a 127 (experimental 192). **Plantillas desde una tabla** (escrita a mano, copiada de una web, o exportada de Football Manager / EA FC). Gestor de Sider con perfiles, puntos de restauración y diagnóstico. Licencia **MIT** (se puede usar su código) | Es justo lo que pediste para tu parche: más ligas y divisiones, y armar plantillas desde una tabla | github.com/Matozanato/FootballLife-new-leagues |
| **Pes Editor Rebuilt (Agustinioquino)** | Editor del EDIT00000000 en el navegador; instala su propio módulo de Sider (`edit21_db.lua`) y lee la base directo de los .cpk del juego | Idea para Phoenix: editor web + módulo Sider | github.com/Agustinioquino/Pes-Editor-Rebuilt |
| **sider-lua-localize (boggylp)** | Encuentra módulos Lua que chocan por usar la misma variable global y los arregla | Evita choques entre phoenix.lua y otros módulos | github.com/boggylp/pes (tools/) |

### Más herramientas nuevas
| Herramienta | Qué hace | Dónde |
|---|---|---|
| **Economy Scaler v5** | Cambia los sistemas económicos del juego | pes-files (30-oct-2026) |
| **ML Budget Manager v1.0 (JamesHoward333)** | Ajusta presupuestos de la Liga Máster con teclas | PES Modding |
| **CamSwitcher v1.0** | Cambia de cámara en tiempo real (Lua) | pes-files (26-oct-2026) |
| **Soundtrack Server v11 / Radio Server v1 (MarcoIacovelli02)** | Música del juego por servidor | pes-files / EvoWeb |
| **Menú de selección estilo PES 2019 (Spursfan07)** | Pantalla de elegir equipo en cuadrícula | PeSoccerWorld |
| **PES21FX (Pes Editor Rebuilt)** | Mejora césped, luces, piel y cuerpos; menú dentro del juego | PeSoccerWorld (27-oct-2026) |
| **Boots & Gloves Relinker (RFootball)** | Cambia IDs de botines con un script | PES Modding |
| **PES2021-scripts (Gustavohps10)** | Módulos Lua de ejemplo: Soundtrack, Ringtone, Playername | github.com/Gustavohps10/PES2021-scripts |
| **spfl26-pes2021-linux** | Jugar FL26 en Linux / Steam Deck | github.com/agentkyo/spfl26-pes2021-linux |
| **eFootball Minifaces Downloader** | Descarga minicaras con IDs originales | github.com/mohamedamrnady/eFootball-Minifaces-Downloalder |
| **ML Manager Pro (Ruls)** | Web gratis de objetivos de carrera y análisis de jugadores | ml-manager-pro.vercel.app |

---

## 9. Ronda 3: la biblioteca grande (30-oct-2026, noche)
Organizada por **para qué sirve**. ⭐ = clave para Phoenix o tu parche.

### A. Archivos base del juego (abrir, empaquetar, convertir)
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **Unlimited Tool Works (4chan Cup)** | Suite "todo en uno" en desarrollo: lee/escribe CPK, FPK, FTEX, DDS, uniparam, modelos y el save (descifra y reescribe byte a byte). GUI + línea de comandos | github.com/the4chancup/unlimited-tool-works |
| ⭐ **pes-file-tools (4chan Cup)** | Librerías Python para manipular archivos de PES (alternativa DDS→FTEX) | github.com/the4chancup/pes-file-tools |
| ⭐ **boggylp/pes** | Herramientas en Go: leer/extraer CPK, **descifrar-editar-cifrar EDIT**, sacar plantillas de Player.bin, detectar caras con ID mal, añadir slots de uniformes, descargar adjuntos de EvoWeb | github.com/boggylp/pes |
| **CpkPatcher (mosamadeeb / MjTs)** | Parchea un CPK cambiando archivos **sin rehacerlo entero** | github.com/MjTs140914/CpkPatcher |
| **PES CPK Tool v1.0** | Arma y extrae varios CPK arrastrando carpetas | pespatch.eu.org |
| **CRI Packed File Maker** | El empaquetador oficial de CPK | sitios de mods PES |
| **GzsTool** | Abre .fpk / .fpkd (paquetes del motor Fox) | GitHub (Atvaark) |
| **FtexTool / PES 2021 Ftex Converter** | Convierte texturas .ftex ↔ .dds | GitHub / implyingrigged |
| **Universal DpFileList Generator** | Genera el DpFileList.bin respetando el formato real de Konami | github.com/CPKProtectionToolkit/udg |
| **PES Protection Toolkit** | Cifra y licencia tu parche (de pago) | cpkprotectiontoolkit.github.io |
| **pes16decrypter (4chan Cup)** | Descifrador de saves PES 2016-2020 | github.com/the4chancup |
| **PES CPK Tool Android** | Manejo de CPK desde Android | github.com/yi8438222-rgb/pes2021-CPK- |

### B. Base de datos y option file
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **4ccEditor** | Editor de save: estéticas, **portar equipos entre versiones de PES** | github.com/the4chancup/4ccEditor |
| ⭐ **pes-db-generator** | Scripts para **generar una base de datos** de PES | github.com/the4chancup/pes-db-generator |
| ⭐ **PESDatabase v0.2.1** | Convierte datos de eFootball a PES; Magic Ability Generator (stats viejos → nuevos) | PES Modding |
| **PES 2021 Live Editor (Ruls/xAranaktu)** | Editor de memoria en tiempo real | PES Modding / GitHub xAranaktu |
| **PES-2021-Cheat-Table (xAranaktu)** | Tabla de Cheat Engine de la comunidad | github.com/xAranaktu/PES-2021-Cheat-Table |
| **Player Real Face Manager (Ruls)** | Gestiona caras reales | PES Modding |
| **Face ID Relinker (Ruls)** | Cambia el ID de una carpeta de cara (edita el .fpk) | PES Modding |
| **BAL Face Relink & BAL Editor (AlexDelPiero23)** | Editor de "Ser una Leyenda" | PES Modding / PeSoccerWorld |
| **UltimateYearChanger 2.1 (loucopouco)** | Jugar en cualquier año (ML, BAL, Liga, Copa) | EvoWeb |
| **ML Fast Forward v1.2** | Saltar a cualquier fecha en la Liga Máster (módulo) | EvoWeb |
| **The Gaffer** | App de Liga Máster para FL | EvoWeb |
| **Online Master League & UCL 36 (web)** | Liga Máster online con amigos, sin instalar | EvoWeb (competidor, ya analizado) |

### C. Modelos 3D, estadios, caras (Blender)
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **pes-fmdl-blender** | Importa/exporta modelos .fmdl (PES 2018+) en Blender | github.com/the4chancup/pes-fmdl-blender |
| **pes-fmdl-blender-5.2** | Versión para Blender 4.2–5.2 | github.com/EssexDotDev/pes-fmdl-blender-5.2 |
| ⭐ **PES Stadium Exporter (MjTs)** | Hace estadios completos en Blender: luces, público, banderas, pantallas, césped, personal | github.com/MjTs140914/PES_Stadium_Exporter |
| **Stadium ID Changer (MjTs)** | Cambia el ID de un estadio | github.com/MjTs140914 |
| **PES Face Hair Modifier (MjTs)** | Modifica caras y pelo | github.com/MjTs140914 |
| **pes-model-blender** | Modelos PES 2014-2017 | github.com/the4chancup |
| **Head Rescale Tool (Football4Life)** | Reescala cabezas | EvoWeb |
| **Tattoo Converter v0.1** | Pasa tatuajes de brazo a torso | EvoWeb |
| **xNormal** | Mapas de normales para texturas | xnormal.net |

### D. Camisetas, fuentes, dorsales
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **PES Kit Creator (PES Master)** | Diseñar camisetas en la web, plantillas de marcas | pesmaster.com |
| **Plantillas Puma / Adidas 2026 (A.ZK)** | Plantillas para Photoshop o Kit Creator | PES Modding |
| **KitPos (módulo)** | Mueve libremente nombres y números en la camiseta | PES Modding |
| **Anthem Jacket Mod** | Chaquetas de himno estilo eFootball | EvoWeb |
| **Font Server para Sider 7 (Aoba)** | Fuentes por servidor | EvoWeb |

### E. Sonido y narración
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **PES Sound Editor v3.1 (Devil Cold52)** | Edita sonidos del juego | EvoWeb |
| **CRI Atom Craft** | Arma paquetes de audio (lo usan los de callnames) | CRI Middleware |
| **CallnamesMaker + guía con IA (Dcv)** | Crea callnames de jugadores y equipos con IA | EvoWeb |
| **English Callname Pack V12 (predator002)** | 1301 callnames en inglés | PES Modding |
| **Narración EA FC 27 (módulo)** | Narración completa nueva | EvoWeb |
| **Home Crowd Bronca / Random Whistle** | Silbidos y reacciones del público | PES Modding |
| **Audacity / GoldWave / Wavosaur** | Editores de audio | gratis |

### F. Jugabilidad y textos
| Herramienta | Qué hace | Dónde |
|---|---|---|
| ⭐ **pes-gameplay-editor (4chan Cup)** | Edita los valores de jugabilidad en dt18 | github.com/the4chancup/pes-gameplay-editor |
| **TBG Gameplay Parametric Editor** | Edita archivos .o de jugabilidad | PES Modding |
| **Gameplay Editor (Football4Life)** | Editor de jugabilidad | EvoWeb |
| ⭐ **STR Editor 2025 (Devil Cold52)** | Cambia **los textos del juego** | PES Modding |
| **Anthem Animation Style Editor** | Estilo de animación del himno | PES Modding |
| **Tutorial ChatGPT + exe de jugabilidad** | Analizar el exe con IA | EvoWeb |

### G. Módulos Sider extra
| Módulo | Qué hace | Dónde |
|---|---|---|
| ⭐ **Sider Mod Menu v1.2** | Menú moderno con mouse dentro del juego | PES Modding |
| ⭐ **Chatty Tactics Server / Chatty Auto Lineup** | Captura el plan de juego, perfiles, alineación automática | EvoWeb / PES Modding |
| **Nuevo formato Champions (Ruls)** | UCL formato nuevo | PES Modding |
| **Mundial 48 equipos (papijonnnn)** | Formato Mundial 2026 | EvoWeb |
| **Custom Celebrations** | Nuevos slots de celebración | PES Modding |
| **crowd_attendance.lua** | Asistencia de público | PES Modding |
| **PlayerCam / VerticalCam** | Cámara en tercera persona | EvoWeb |
| **GOALNETS (FicaBre)** | Redes de arco + tutorial | EvoWeb |
| **Post-Processing Editor + ReShade (Godmode_ON)** | Gráficos | EvoWeb |
| **Free Cloth Physics** | Física de ropa | EvoWeb |
| **MatchUP** | Modo de juego nuevo | EvoWeb |
| **Scoreboard modding en game2dPes.bin** | Fuentes y tamaño del marcador en el juego | EvoWeb |
| **UI Modding: UIColors.lua y offsets (SoulBallZ)** | Tutorial para cambiar menús | EvoWeb |

### H. Listas maestras para no perder nada
- EvoWeb: hilo **"List of important tools & tutorials"** (fijado en el foro PES 2021).
- **Rigged Wiki** (implyingrigged.info): tutoriales de Blender, estadios y herramientas.
- PES Modding: etiquetas "PES 2021 Tools" y "PES 2021 Sider Modules".
- Documentación oficial de Sider: mapote.com/doc/sider.
