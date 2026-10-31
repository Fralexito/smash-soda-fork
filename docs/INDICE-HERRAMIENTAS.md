# Índice único de herramientas · Phoenix (30-oct-2026)
Todas las herramientas encontradas (rondas 1 a 5), **sin descartar ninguna** y sin repetirlas. Los detalles y enlaces están en `herramientas-parche-pes2021.md`.

## Cómo leer este índice
| Marca | Significa |
|---|---|
| 🟢 **COPIAR** | Copiar su código o su función a Phoenix. Si no tiene licencia abierta (MIT, zlib, GPL), copiamos la idea y la reescribimos nosotros |
| 🔵 **USAR** | Usarla tal cual, como apoyo para trabajar (crear contenido, probar, convertir) |
| 🟣 **REFERENCIA** | Estudiarla para inspirarnos o compararnos (competidores, ideas, documentación) |

Prioridad: **P1** = primero (toca Mercado, Link o Sync), **P2** = cuando hagamos tu parche, **P3** = extra.

---

## 1. Mercado y Sync (fichajes, EDIT, recarga)
| Herramienta | Marca | Prioridad | Qué tomamos |
|---|---|---|---|
| FL Daily Edit (gvoze32, MIT) | 🟢 | P1 | Cómo escribe fichajes en el EDIT con respaldo y reemplazo seguro |
| pesXdecrypter (4chan Cup) | 🟢 | P1 | Abrir y cerrar el EDIT |
| boggylp/pes (editsave, pesdb) | 🟢 | P1 | Descifrar/editar/cifrar EDIT; plantillas desde Player.bin |
| PES 2021 Utilities (srafols, MIT) | 🟢 | P1 | Leer archivos TED en Python |
| GV PES TED Editor | 🟢 | P1 | Editar equipos por línea de comandos |
| Crypter (Tomato4cc) | 🟢 | P2 | Proceso por lotes |
| CareerLab (nsfxu) + DBDump.lua | 🟢 | P1 | Módulo Sider **solo lectura** para leer la base |
| Pes Editor Rebuilt + edit21_db.lua | 🟢 | P1 | Editor web + módulo Sider; leer la base de los .cpk |
| TEditor Web-Tool | 🟢 | P1 | Arreglar el flag de datos en vivo con un clic |
| FL ML Desk (gvoze32) | 🟢 | P2 | Editar Liga Máster en curso |
| Master League FL26 (Vanic17) | 🟢 | P1 | Actualizar en **otra PC** y restaurar el EDIT |
| pes2021-transfer-tool (kickoffsage) | 🟢 | P1 | Herramienta de fichajes |
| EDIT Version Changer | 🔵 | P2 | Cambiar versión del option file |
| PES File Crypter v3 (Devil Cold52) | 🔵 | P1 | Probar a mano el EDIT |
| Player Data Editor (Devil Cold52) | 🔵 | P1 | Revisar a mano lo que escribe Phoenix |
| 4ccEditor | 🔵 | P2 | Portar equipos entre versiones |
| PES Editor (ejogc327) / DinoTem Editor | 🔵 | P2 | Editores clásicos del EDIT |
| Live Editor (Ruls/xAranaktu) + Cheat Table | 🟣 | P2 | Memoria (plan a futuro) |
| FC 26 Live Editor (xAranaktu) | 🟣 | P3 | Hasta dónde llega un editor de memoria completo |
| Wiki "Edit file" (Rigged) | 🟣 | P1 | Documentación del formato |

## 2. Phoenix Link (multijugador, Parsec, resultados)
| Herramienta | Marca | Prioridad | Qué tomamos |
|---|---|---|---|
| PES 2011 Online Matchmaker (idea EvoWeb) | 🟣 | P1 | Juntar jugadores, elegir host, conectar solo, leer marcador |
| Touchline (ahmadsk-cell) + Prologue | 🟢 (ideas) | P1 | Presupuestos, forma, público, despidos en vivo (ver `mapeo-touchline.md`) |
| My Football Legacy | 🟣 | P1 | Resultados con IA por captura, liga online (competidor) |
| Online ML & UCL 36 web | 🟣 | P1 | Liga Máster online con amigos (competidor) |
| PESM (HollywoodKills) | 🟣 | P2 | En desarrollo, vigilar |
| Anti-Cheat (Holland) | 🟢 | P1 | Juego limpio en partidas de liga |
| Fair Player Ratings / Dynamic Difficulty | 🟣 | P2 | Equilibrio |
| sider7-linux | 🔵 | P3 | Jugadores en Linux |
| spfl26-pes2021-linux / PES 2021 NX | 🔵 | P3 | Linux / Android |

## 3. Web Phoenix Evolution (liga, torneos, datos)
| Herramienta | Marca | Prioridad | Qué tomamos |
|---|---|---|---|
| openfootball / transfermarkt-datasets / soccerdata | 🔵 | P1 | Datos reales de ligas y jugadores |
| API-Football / Sportmonks | 🔵 | P2 | Datos en vivo (de pago) |
| PESDatabase / PESBuilder (kisni07) | 🔵 | P1 | eFootball → PES; Magic Ability Generator |
| CSVs de eFootball / Player ID Collection / PES Stats Converter | 🔵 | P1 | IDs y stats |
| pes-db-generator (4chan Cup) | 🟢 | P2 | Generar base de datos |
| ML Manager Pro (Ruls) | 🟢 (ideas) | P1 | Objetivos de carrera, análisis de jugadores |
| ML Transfer Assistant (DcV253) | 🟢 (ideas) | P1 | Asistente de fichajes |
| The Gaffer | 🟣 | P2 | App de Liga Máster |
| pes-ml-sim | 🟢 | P2 | Simular partidos sin jugar |
| Torneo Sofascore / Competize / Cupside / Easy Tournament | 🟣 | P1 | Formatos, reporte de partidos, chat por torneo |
| Extensión Chrome Manager League Players | 🟣 | P3 | Buscar jugadores libres |
| hkpl-pes2021-edit-data | 🟣 | P3 | Liga guardada en GitHub |
| Reglamentos oficiales | 🔵 | P1 | Reglas de campeonatos |

## 4. Sider y módulos Lua (motor de todo)
| Herramienta | Marca | Prioridad | Qué tomamos |
|---|---|---|---|
| Sider 7.4.1 + código fuente (juce) | 🔵 | P1 | Base de todo |
| sider-lua-localize (boggylp) | 🔵 | P1 | Evitar choques de variables con phoenix.lua |
| PES2021-scripts (Gustavohps10) | 🟢 | P2 | Ejemplos de módulos |
| CommonLib.lua | 🔵 | P2 | Librería común |
| Sider Mod Menu v1.2 | 🟢 (ideas) | P1 | Menú con mouse dentro del juego (para Phoenix en el juego) |
| Competition-Server (Gerlamp) / Database Module (GRAND) | 🟢 | P2 | Competiciones por módulo |
| Team / Competition Licencer (zlac) | 🔵 | P2 | Licencias de equipos |
| CustomExhibitionMatch.lua | 🔵 | P1 | Amistoso que cuenta como competición |
| ScoreboardServer / P4 / Scoreboard Server 1.4 | 🔵 | P2 | Marcadores |
| MenuServer AIO, Entrance, Tunnel, Intro Server | 🔵 | P2 | Presentación |
| Kitserver 1.12 / KitPos | 🔵 | P2 | Camisetas |
| BallServer / RefKitServer / MiniFaceServer | 🔵 | P2 | Balones, árbitros, minicaras |
| Stadium Server (zlac) | 🔵 | P2 | Estadios |
| Tournament StaffServer v3.2 | 🔵 | P2 | Personal de torneo |
| Derbys.lua / LogoHalftime.lua | 🔵 | P3 | Derbis, logo del descanso |
| UnlockCam / Camera Switcher / CamSwitcher / PlayerCam / Penalty Camera | 🔵 | P3 | Cámaras |
| Chatty Tactics Server / Auto Lineup | 🟢 (ideas) | P2 | Planes de juego y alineación |
| Stat Editor v35 (JamesHoward333) | 🟢 (ideas) | P2 | Editar dentro del juego |
| ML Budget Manager / Economy Scaler | 🟢 (ideas) | P2 | Economía |
| ML Fast Forward / UltimateYearChanger / Age Lock / Better Regens / Real Injuries | 🔵 | P3 | Liga Máster |
| Crowd Attendance / crowd_attendance.lua | 🔵 | P3 | Público |
| Custom Celebrations | 🔵 | P3 | Celebraciones |
| UCL 36 / Champions nuevo (Ruls) / UCL 32 / Mundial 48 / Carabao Cup | 🔵 | P2 | Formatos de torneo |
| Soundtrack Server / Radio Server | 🔵 | P3 | Música |
| UIColors.lua + tutorial (SoulBallZ) | 🔵 | P2 | Colores de menú |

## 5. Tu parche (ligas, equipos, gráficos)
| Herramienta | Marca | Prioridad | Qué tomamos |
|---|---|---|---|
| FL26 Mod Studio / FootballLife-new-leagues (MIT) | 🟢 | P2 | Ligas y países nuevos, 127 competiciones, plantillas desde tabla |
| Unlimited Tool Works (4chan Cup) | 🔵 | P2 | Suite todo en uno |
| pes-file-tools / CpkPatcher / PES CPK Tool / CRI tools | 🔵 | P2 | CPK |
| GzsTool / FtexTool / Ftex Converter / PES Texture Converter | 🔵 | P2 | Texturas y paquetes |
| DpFileList Generator v1.3 / Universal DpFileList Generator | 🔵 | P2 | Lista de CPK |
| PES Protection Toolkit | 🔵 | P3 | Proteger tu parche (de pago) |
| Slot Creator (Devil Cold52) | 🔵 | P3 | Slots de torneo |
| CGPE (shawminator) | 🔵 | P2 | Editor de parámetros |
| STR Editor 2025 | 🔵 | P2 | Textos del juego |
| PES Kit Creator / Kit Studio / Kit Manager 2021 / plantillas A.ZK | 🔵 | P2 | Camisetas |
| Boots & Gloves Relinker | 🔵 | P3 | Botines |
| Anthem Animation Style Editor / Anthem Jacket | 🔵 | P3 | Himnos |
| Font Server (Aoba) | 🔵 | P2 | Fuentes |
| A Tool for making scoreboard / marcador en game2dPes.bin | 🔵 | P2 | Marcadores |
| Menú estilo PES 2019 / Graphic Menus | 🔵 | P3 | Menús |
| pes-fmdl-blender (+5.2) / pes-model-blender / xNormal | 🔵 | P2 | Modelos 3D |
| PES Stadium Exporter / Stadium ID Changer / Stadium Manager / Checker | 🔵 | P2 | Estadios |
| Turf-Loader / light-loader / Goalnets (FicaBre) | 🔵 | P3 | Césped, luces, redes |
| PES21FX / Post-Processing Editor / ReShade / Cloth Physics | 🔵 | P3 | Gráficos |
| Player Real Face Manager / Face ID Relinker / Face Relinked 3D / Face Switcher | 🔵 | P2 | Caras |
| Face Hair Modifier / Head Rescale / Tattoo Converter / Tattoo UV Builder | 🔵 | P3 | Caras y tatuajes |
| Miniface Scrapper / MiniFace Maker / Minifaces Downloader / flujo IA de caras | 🔵 | P2 | Minicaras |
| Callname Server + Manager / CallnamesMaker IA / Callname Pack V12 | 🔵 | P2 | Narración con nombres |
| Commentary Server / Dynamic Commentary / narración EA FC 27 | 🔵 | P3 | Narración |
| PES Sound Editor / CRI Atom Craft / Audacity | 🔵 | P3 | Sonido |
| Bronca / Random Whistle | 🔵 | P3 | Público |
| TBG Gameplay Editor / pes-gameplay-editor / Gameplay Editor F4L / Gameplay Lua / Fakecid AI | 🔵 | P2 | Jugabilidad |
| Tutorial ChatGPT + exe de jugabilidad | 🟣 | P3 | Analizar el exe con IA |
| BAL Editor / blue41 Mod Studio / BAL Career Editor | 🔵 | P3 | Ser una Leyenda |
| Substitution Board (Balkan PES Box) | 🔵 | P3 | Tablero de cambios |
| MatchUP | 🟣 | P3 | Modo de juego nuevo |

## 6. Listas maestras (para seguir al día)
| Fuente | Marca |
|---|---|
| EvoWeb "List of important tools & tutorials" | 🟣 |
| Rigged Wiki (implyingrigged.info/wiki/Tools) | 🟣 |
| PES Modding (etiquetas Tools y Sider Modules) | 🟣 |
| pes-files / PeSoccerWorld / welovepes / kazemario | 🟣 |
| Documentación de Sider (mapote.com) | 🟣 |

---

## Orden sugerido para la otra cuenta
1. **Mercado y Sync, P1:** estudiar FL Daily Edit, pesXdecrypter, CareerLab y Vanic17 para estabilizar la recarga R3 (ver `PENDIENTE-IDEAS-TOUCHLINE.md`).
2. **Link, P1:** leer el hilo del Matchmaker y aplicar las ideas de Touchline.
3. **Web, P1:** datos reales (openfootball, PESDatabase) e ideas de ML Manager Pro y Cupside.
4. **Parche, P2:** FL26 Mod Studio como base para ligas nuevas.

Regla: nada se copia ni se cambia sin aprobación de FRALEX.
