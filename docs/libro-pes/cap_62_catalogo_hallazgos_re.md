# Capítulo 62 — Catálogo de hallazgos de ingeniería inversa (REA / Ghidra)

> Regla de Fralex (19-oct-2026): **cada vez que se use REA o Ghidra, guardar todo lo que pueda servir en el futuro, aunque no sea del tema del momento.** Este catálogo es ese depósito. Una fila por hallazgo: dirección (VA del exe de PES 2021, igual en el recortado), qué es, certeza y de dónde salió (archivo en `informes/datos/rea/`).
> Certeza: **C** = confirmado en código descompilado · **P** = probable (deducido) · **H** = hipótesis.

## 1. Configuración del PC (`settings.dat`) y objeto «app»
| Dirección | Qué es | Certeza | Fuente |
|---|---|---|---|
| `FUN_14149A470` | Devuelve el objeto global de configuración de la app (1.884 llamadas en todo el exe). | C | mandos_24 |
| `app+0xB0` | Puntero al contenido de `settings.dat` en memoria. | C | mandos_16_settings |
| `app+0xF4` | Tipo de entrada elegido (1/2; con 1 se pone modo mando). | P | mandos_34 |
| `app+0x94/0x98` | Resolución de pantalla usada (ancho/alto); mínimo forzado 0x200 (512). | C | mandos_16_settings |
| `app+0xB9` | Pantalla completa (bit 0 de las opciones). | C | mandos_16_settings |
| `0x1436F0570` | Copia en memoria de `settings.dat` (0x264 = 612 bytes). Detrás (`0x1436F07D8`, `0x1436F07F8`) hay textos de registro: la tabla no puede crecer en su sitio. | C | mandos_16_settings |
| `FUN_140408AF0` | Lector de `settings.dat`: firma `WECF` (0x46434557), versión `0x6B`; si falla, valores por defecto. | C | mandos_14 |
| `FUN_140408B50` | Valores por defecto: opciones 0x456 \| 0x60, 1920×1080, audio 2, puertos UDP 0x166B/0x166C (5739/5740), 9 casillas (teclado + 8 mandos). | C | mandos_14 |
| `FUN_140408D20` | Escribe un informe de texto con la configuración (secciones Display / Audio / Online). | C | mandos_15 |
| Bits de opciones (`settings+8`) | bit0 pantalla completa · bit2 60 fps en juego · bit3 60 fps en repeticiones · bit4 modo de mando alternativo · bit5 P2P · bit6 puerto UDP automático · bit7 chat de voz · bits 9/10/11 VSync Off/On1/On2. | C | mandos_15 |
| Formato de casilla | 0x40 bytes: GUID DirectInput (16) + 24 botones de 2 bytes. Casilla 0 = teclado (desde `+0x24`), casillas 1–8 = mandos (GUID en `+0x64 + i×0x40`). | C | mandos_14, mandos_32 |
| `Settings.exe` (.NET) | Tipos `HID_KIND` (teclado, PAD_1…PAD_8), `SettingsData` (magic, version, option, dispW, dispH, drawRatio, quality, m_hid[]), `hid` (GUID + rule[]), `HidRule` (type, index). Usa también `\KONAMI\temp`, `pes_info.zip`, `\Settings.txt`. | C (REA) | rea_settings_bundle.json |

## 2. Mandos (capa de entrada)
| Dirección | Qué es | Certeza | Fuente |
|---|---|---|---|
| `FUN_140402660` | **Conector settings → mandos**: arma los mapas de botones y recorre **8 casillas** (`0x140402A82: mov esi, 8`; rama alternativa `0x140402A40: cmp edi, 9`). Combinaciones fijas del teclado: `B16K1 B35K1`, `B17K28 B36K28`. | C | mandos_32 |
| Formato de mapa de botones | Texto de pares `B<n><letra><n>`; letras `BAXxPpMmVvHhUDLRK` (B = botón, A = eje, U/D/L/R = cruceta, K = tecla…). | C | mandos_32, mandos_1 |
| `0x1437F0A58` | Gestor de juego `cobra::game::impl::HidMan` (vftable `0x142AC5B68`). | C | mandos_3, mandos_4 |
| `HidMan` vftable | +0x30/+0x38 teclado N · +0x40/+0x48 ratón N · +0x50/+0x58 dispositivo principal · +0x60 fija modo de entrada · +0x68 máscara de mandos activos (32 bits) · +0xD8 lee config `pad_assign` · +0xE8 mapa de botones del mando N · +0xF8 parámetro +0x5C del mando N · +0x110 **asigna GUID al mando N** · +0x118 activa modo por GUID · +0x120 activa el mando N (bit). | C | mandos_4, mandos_29–31 |
| `FUN_141B54F60` | Cada fotograma: engancha cada mando físico con la entrada cuyo GUID coincide (tabla `HidMan+0xD0`, 32 × 0x20, GUID en +0x10). | C | mandos_28 |
| `0x14351A840` | Capacidades: 32 mandos, 1 teclado, 1 ratón. | C | mandos_8–10 |
| `0x1437F0AE0` | `cobra::hid::impl::Man` (ctor `FUN_141B58A80`); fábrica `_anon_9B2E8B15::Factory` (vftable `0x142AC7198`). | C | mandos_6, mandos_9 |
| `0x1437F0AF8` | Objeto DirectInput (`cobra::me::hid`); 32 ranuras de mando en `+0xE0`; si `+0x298` ≠ 0, las 4 primeras van por XInput. | C | mandos_17 |
| `FUN_141B5D410` / `FUN_141B5E1F0` | Llegada de un dispositivo / alta por GUID (árbol por GUID; tipos 1 mando, 2 ratón, 3 teclado). | C | mandos_22, mandos_23 |
| `FUN_141B54000` | Elige el mapa de botones por modelo: `0x0268054C` = DualShock 3 (Sony), `0x028E045E` = Xbox 360; nombres `@auto`, `XInput`. | C | mandos_1 |
| `FUN_140A9A6C0` | Pantalla de título: con ciertas teclas (0x13–0x24) pasa a modo teclado. | C | mandos_34 |

## 3. Menús
| Dirección | Qué es | Certeza | Fuente |
|---|---|---|---|
| `menu::SideSelect` (`FUN_140ABF720` en vftable +0xD8) | Pantalla de elegir lado: **8 fichas fijas** (punteros en `+0x90…+0xC8`, bucle `< 8`). | C | mandos_35 |
| `FUN_140E43620`, `FUN_140E431F0`, `FUN_140E42F80` | Funciones de menú que buscan «el primer mando activo» entre **8** (bucles `< 8` sobre la máscara de mandos). | C | mandos_35 |
| `FUN_140C678C0` / `FUN_140C67450` | Fábrica y constructor de `menu::SideSelectEach` (objeto de 0xC0 bytes). | C | mandos_33, mandos_1 |
| `FUN_140AD9E90` | Aviso `numControllers_4` del menú. | C (texto exacto: H) | mandos_1 |

## 4. Partido
| Dirección | Qué es | Certeza | Fuente |
|---|---|---|---|
| `[0x1436F3DC8]+0x180` | **24 ranuras de control** (0x74 bytes): `+0x10` = índice de entrada (PadInput, < 24), `+0x20` = jugador (0–21; 0–10 local, 11–21 visitante). | C | mandos_38, mandos_12 |
| `match::registry::PadInput` | Bloque de 0x7D4 bytes por mando; índice < 24. Nombre de registro `PADINPUT` (`FUN_14042A3A0`). | C | mandos_38 |
| `FUN_14043B870` | Jugador → ranura (35 llamadores). | C | mandos_12 |
| `match::CursorManager` | vftable `0x14259BD90`, global `0x1436F3FA0`, etiqueta `CURSOR_INFO`. | C | mandos_11 |
| `[0x1436F3DC8]+0x270` → `+0x116C`, `+0x173C`, `+0x1740`, `+0x1554` | Datos del evento/registro del partido (tipo de evento, jugadores implicados). | P | mandos_11, mandos_36 |

### 4.1 Los 44 «registros» de datos del partido (`match::registry::*`) — mapa para todo lo futuro
Cada registro es un bloque de datos compartido por los trabajos del partido (se lee/escribe con `…Ref`). Lista completa (RTTI): `AnalyzeInfo`, `AnimeInfo`, `BallAttach`, `BallExternalForce`, `BallInfo`, `BlockDebugInfo`, `CameraInfo`, `CameraSettings`, `CameraTargetInfo`, `ClothJudgeInfo`, `CommandInfo`, `CommandOutput`, `ConceptArrangeReleaseInfo`, `CornerFlagInfo`, `CursorInfo`, `DemoControl`, `DemoInfo`, `DemoPlayerInfo`, `ESports`, **`FieldInfo`**, `FixDemoInfo`, `GPAchievementInfo`, `GeneralSettings`, **`GoalPostInfo`**, `MatchEnv`, `MatchInfo`, `ModeInfo`, `NoOperationLeaveInfo`, `OrderInfo`, **`PadInput`**, `PkMatch`, `PlayData`, `PlayerInfo`, `PlayerMove`, `RagdollInfo`, `RandomInfo`, `RecordInfo`, `ReplayInfo`, `Screen2dInfo`, `SituationInfo`, `StepupTutorial`, `SystemSettings`, `TeamAIInfo`, `TrainingWork`, `VanishingSprayInfo`. [C]
- `FUN_140441490` (constructor de un trabajo grande del partido) guarda sus referencias en offsets fijos: `BallInfo +0x17890`, `Screen2dInfo +0x17C30`, `FixDemoInfo +0x17C58`, `CornerFlagInfo +0x17C80`, `VanishingSprayInfo +0x17CA8`, `TrainingWork +0x17D50`, `RecordInfo +0x17D78`, `PkMatch +0x17DA0`, `MatchInfo +0x17DC8`, `MatchEnv +0x17DF0`, `ESports +0x17E18`, `CursorInfo +0x17E40`, `PadInput +0x17E68`, `GeneralSettings +0x17E90`, `StepupTutorial +0x17FA8`. [C] (mandos_38)
- **Para el proyecto de cancha pequeña / N vs N:** `FieldInfo` y `GoalPostInfo` son candidatos directos para medidas del campo y porterías; `PlayerInfo`/`TeamAIInfo`/`OrderInfo` para la cantidad y colocación de jugadores. [H: por nombre, falta abrirlos]

## 5. Online
| Dato | Qué es | Certeza |
|---|---|---|
| `pes21-x64-gate.cs.konami.net`, `pes21-x64-stun.cs.konami.net`, `http://info.service.konami.net/XME994-E1/info/` | Servidores de PES 2021 (cerrados). | C (cadenas) |
| `MOMLobby*`, `match_teamLobby`, `c_teamLobby`, `multilobby`, `freelobby` | Restos del lobby Team Play 11 vs 11. | C (cadenas) |

## Cómo ampliar
Añadir filas con fecha cuando aparezca algo nuevo (aunque no sea del tema del día). Scripts: `scripts/ghidra/CadenasYUsos.java` (modos cadena, `fn:`, `callers:`, `sym:`, `vt:`), `_lanzar_mandos.bat`, `_lanzar_mandos_ref.bat`, `_lanzar_mandos_fn.bat`.

## 6. Añadidos 19-oct-2026 ~19:00
| Dirección / dato | Qué es | Certeza |
|---|---|---|
| `FUN_140408C50` | Abre `\KONAMI\eFootball PES 2021 SEASON UPDATE\settings.dat` (texto ancho UTF-16 en el exe) y lee 0x264 bytes con `fread`; acepta archivos más largos. | C |
| `FUN_140A646A0` | Constructor del registro `PadInput`: 24 bloques × (100 muestras de 0x14 + contador en +2000) = 0xBBE0; luego 8 banderas (+0xBBE0, por defecto 1) y 8 configuraciones de 0x12 bytes (+0xBBE8). | C |
| `FUN_140A64810` / `FUN_140A64850` / `FUN_140A647E0` | Llenado por fotograma de cada bloque desde `HidMan` (solo índices < 8) / escritura de muestra / bandera de presencia. Tabla de teclas por defecto 0x13–0x24. | C |
| `FUN_1408AA5C0` | Bucle del partido que actualiza las 24 entradas. | C |
| `menu::SideSelectEach` | Ventana por mando: `p_pad`, `pad_connected`, `arrow_0/1`, `padColor_<n%10>`, lado en `+0xAC` (< 3), número de mando en `+0x98`; `FUN_140C67BE0` la arma. | C |
| `FUN_140C67AF0` | Clave `sideselect_<n%10>` con la que se recuerda el lado de cada mando (`FUN_1414BEA50` / `FUN_1414BECA0` = almacén de lados). | C |
| `sideSelect.bin` | En `dt11_x64.cpk` (pos. 94208, 48972 bytes; cabecera `0xFF 0x10 'QWESYS'` + tamaños + zlib → 86080 bytes; Flash compilado `afp_sideSelect.apk`, plantilla `sideSelectPad`, `sideSelectManager`, `sideSelectWindow`). | C |
| Sider FFI | `luajit.ext.enabled = 1` en `sider.ini` habilita `ffi`/`bit` de LuaJIT (llamar funciones del juego desde Lua). | C (doc) |
| Tablas de menús de depuración | Cadenas `USER_SIDE`, `CHECK_COM_TEAM_PLAYERS`, `COM_V_COM` referenciadas desde tablas de datos (`0x14344AB58`, `0x14344AF58`, `0x14344D1E8`). | P |
| Ranura de cursor (0x74 bytes, 24 en `[0x1436F3DC8]+0x180`) | `+0x0D` bandera · `+0x10` mando (bloque PadInput, < 24) · `+0x20` jugador controlado (< 22, cambia al cambiar de cursor) · `+0x2C` otro jugador (destino/anterior) · `+0x68` temporizador (`FUN_141F9C3B0`). Lógica de cursores en `FUN_1404362D0`, `FUN_140436820` (5705 B), `FUN_1404386D0`, `FUN_140438F10`, `FUN_140439820`, `FUN_14043C880`, `FUN_14043D1B0`, `FUN_14043EC70` (3824 B), `FUN_14043FD10`. | C (campos) / P (significado de +0x2C, +0x68) |

## 7. Menús y motor gráfico (19-oct-2026 ~19:45)
| Dato | Qué es | Certeza |
|---|---|---|
| Flujos de menú JSON | `common/script/flow/**.json` en `dt16_all.cpk`, texto plano: eventos `proceed`/`return`/`0_pdsetting`/`1_controll`/`2_demoCheck` con `to` (pantalla destino), `go`, `clear`, `fade*`, `bgType`, `data.type`, `bgId`. Se pueden cambiar con livecpk (orden de pantallas). | C |
| `menu::MOMSideSelect` | Pantalla de lados del 11 vs 11 online; `sideMatchMembers.bin`; lista dinámica de miembros; `TaskSyncSelectSideInMulti`; avatares `avater_tex_`. | C |
| `flash::*` | Reproductor Flash de Konami: `MovieClip` (vftable `0x142BB2A70`), `Window`, `Page`, `Root`, `Choice`, `Duplicate`, `Rect`, `Package::AfpBin::Manager`, `SystemModule::AfpModule/AvsModule`. Nombres especiales al cargar (`FUN_141F9A070`): `_duplicate` → plantilla clonable (`FUN_141F9A3A0`). | C |
| Menús `.bin` | Cabecera `0xFF 0x10 'QWESYS'` + tamaño comprimido + tamaño real + zlib; dentro `afp_*.apk` (Flash compilado; nombres de nodos codificados salvo los principales). | C |
| `cmnIconMOM.bin` | Iconos del modo MOM (11 vs 11) en `common/menu/parts/`. | C (listado) |
| Imágenes de menús (`*.bin` → `afp_*.apk`) | Tras la cabecera `afp` hay un contenedor **`TXP2`** (big-endian; en `sideSelect.bin` en 0x30) con tabla de texturas (`tex000`) y un bloque **`TXDT`** comprimido con un método propio (no zlib; en `sideSelect.bin` en 0x6BA8, 58.460 bytes hasta el final). Extraerlas pide encontrar el descompresor en el exe (pendiente). | C (estructura) / H (método) |
| Ranura de cursor — campos confirmados en vivo | `+0x00` lado (0 Local / 1 centro / 2 Visitante) · `+0x01` nº ranura · `+0x10` mando PadInput · `+0x18` mando lógico HidMan (32 = ninguno) · `+0x20` jugador · `+0x28` primer jugador del equipo (0/11) · `+0x2C` jugador anterior · `+0x68…+0x72` banderas de cursor activo. | C (en vivo) |
| HidMan tabla GUID en vivo | Entradas 0–7 con los GUID de `settings.dat`; `+0x8` = índice de dispositivo físico, `+0xC` = entrada inversa; entradas 8–31 vacías. | C (en vivo) |
| Objeto «ranuras de cursor» (registro copiado) | ctor `FUN_140A54C50` → reset `FUN_140A55980`; 24 ranuras de 0x74 (lado +0x00 = 1, nº +0x01, mando +0x10 = −1, lógico +0x18 = −1, jugador +0x20 = 0xFF, +0x24 = 0xFF, +0x2C = 0xFF, +0x0D = 0x100…); `+0xAE0` 24×0x3C; `+0x1084` 22×u32 jugador→ranura (0x18 = ninguna); `+0x10DC` (0xFFFF), `+0x10E0`, `+0x10E4` (0x1818), `+0x10E8` 24×u32 (4), `+0x1148` (0x18). | C |
| Árbol DirectInput de dispositivos | `[0x1437F0AF8]+0x90` = cabecera de std::map (izq +0, padre +8, der +0x10, nil +0x19, GUID +0x20, dispositivo +0x30; tipo del dispositivo +0x29: 1 mando, 2 ratón, 3 teclado). En vivo: 9 mandos + teclado + ratón. | C (en vivo) |

#### 20-oct-2026 — Colores y flechas del partido (mandos_70–74)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Color de jugador/cursor (partido) | `FUN_1420BD9C0(out, idx, variante)`: idx < 6 → tabla `0x1437F9668` (cian, rosa, amarillo, verde, naranja, violeta); resto gris 0.83/0.098. RGBA = `R | G<<8 | B<<16 | A<<24` (`FUN_140490590`). | C |
| Escala de colores reutilizada | 2.ª tabla de cadenas de cuadros Flash `playerColor_0..7/none/on/off` (`0x1434AC540`, `0x1434AC750`) en `FUN_140ACFF60`/`FUN_140ADA3B0` (menús de formación: 11 casillas `p_grand`). | C |
| `match2D::Screen::ModelCursor` | vftable `0x142BDE210`; init `FUN_1420DBF00` (22 modelos, color inicial por futbolista, `FUN_141F9F870` tiñe +0x44..47); `+0x28` → 0x16; `+0x38` visible; `+0x50` posición; `+0x70` color; `+0x78` escala/parpadeo. | C |
| Capa 2D de cursores | `FUN_1420C46F0` (24 ranuras → `pantalla+0x19C8`, 0x20 c/u), llamada desde `FUN_1420C5330`; opción `Def_Game_match2d_cursor_on`. | C |
| Tabla de modelos game2d | `FUN_141EA0220(i)` → ruta `/Assets/pes16/model/game2d/...` desde `0x14351E380` (d2_cursor_a, d2_cursor_b, d2_manual_cursor…); usada por `FUN_1420E3C50` (carga de modelo 2D), `FUN_141F9E7F0`, `FUN_140403D20`. | C |
| Textura de avatar del cursor | `FUN_1420D7B00`: nombre `AvatarTex_cursor%d_0x%x`. | C |
| Constantes de cursor | `DevelopData/common/match/constant/match/cursor.json` (clase `constant::Cursor`). | C (existe) |

#### 20-oct-2026 — Pantalla de lados, productor y Sider (mandos_75–77, sider.dll con REA)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| `menu::SideSelect` | vftable `0x142673558` / `0x1426737B0`; ctor `FUN_140ABEAD0`; dtor `FUN_140ABF000` (lee S = `[[0x1414B6A60()]+0x50]+0x3F308`, `+0x3FCC8` ≤ 1, `FUN_1414BDC30(S, lado)` = nº de usuarios en ese lado, `FUN_1414C5950`). | C |
| Productor de ranuras | `FUN_141FB9040` (prólogo 18 bytes sin RIP-relativo); llamado por `FUN_141FCF130`, `FUN_141FD6190`, `FUN_141FE3590`. | C |
| Liberación de memoria de PES | `FUN_1403FD360` = `free` tipo dlmalloc (cabeceras de trozo, bins). Un cierre aquí = memoria corrompida antes. | C |
| Sider 7.3.4: imagen del overlay | `sider.dll` `FUN_1800147C0` (gancho de Present): recarga la textura cuando cambia la RUTA (`strcmp`); `FUN_18001D5E0` lee de `overlay_on` texto, ruta y opciones; `image_width/height` < 1 = fracción, si no = píxeles (1.0 = 1 px). Registro «Loaded 2D texture» con `debug = 128`. | C |
| Sider 7.3.4: entorno de módulos | Solo: assert, ipairs, pairs, tostring, tonumber, table, string, math, unpack, type, error, io, os (parcial: sin `os.remove`), _G + log, memory, fs, zlib, audio, match, input, _FILE (+ ffi/bit con `luajit.ext.enabled`). Sin `pcall`, `loadstring`. | C (en vivo) |
| Pantalla de lados: filas | `menu::SideSelect` `+0x90…+0xC8` = 8 filas (vftable `0x142715608`): `+0x98` nº mando, `+0x9C` nº perfil, `+0xA4` lado (0 Local/1 centro/2 Visitante). | C (en vivo) |
| «Datos personales» (perfiles) | Bloque propio (cabecera `+8` = 0x4FFE0); 32 × 0x980 desde +0x10: `+0` 0/1, `+4` nombre UTF-8 (0x2C). Perfil i ↔ mando i (también 8+). Dirección distinta en cada arranque. | C (en vivo) |
| Estado DirectInput por mando | Plaza lógica `+0xC` → fila de dispositivo → objeto → `[obj+0x20]` (GUID) `+0x238` = DIJOYSTATE (X/Y 0–0xFFFF, centro 0x7FFF; cruceta +0x20; botones +0x30, 0x80). 3 copias cada 0x50. | C (en vivo, mando ViGEm de prueba) |
| Vtable mando lógico HidMan | `0x142AC7660` (8 entradas); `+0x38` `FUN_141B5A160` reenvía a `[[this+0x80]]+0x38`. | C |
| Sider: opacidad de imágenes | `overlay.image-alpha-max` en sider.ini limita la opacidad (0.8 en ConmeGOL → se transparenta lo de debajo). | C (en vivo) |

#### 25-oct-2026 — Menos jugadores / modo street: lectura estática (hilo «modo street»)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| `[gestor+0x270]+0x1775` (byte) | `FUN_140447290` (secuencia de fin de partido/estado, ~0x8F2 = subestado 1..5) sale de inmediato si es ≠ 0. Bandera que corta la secuencia de cierre; posible interruptor útil. | H (lectura estática) |
| Subobjeto de «fase» en `S+0x1554` | `FUN_141B8B250` solo guarda un código 0..8 (0,1,6,7,8 vistos en MatchControl/FUN_1404a56c0/FUN_1404a6d20). El código en `+0x1574` es una fase del partido, **no** el motivo de abandono; el «2 = abandono» de ObserverForfeitedGame sigue sin confirmar. | H |
| Modos con menos jugadores ya existentes | Clases `match::registry::TrainingWork(Ref)`, `match::player::ActionFreeTrainingBreak`, `ActionTrainingMoveRoute`, `menu::MatchTeamSelectTraining`, `MatchPauseTrainingMenu`: el motor ya corre partidos con pocos actores en Entrenamiento libre. **Pista fuerte para «quitar jugadores»**: ver qué crea los actores en ese modo (TrainingWork) y reutilizarlo. | H (por nombre; sin abrir) |
| Menús de formación | Cuadros Flash `p_grand` con 11 casillas; `matchPlan::TacticsFormation`, `EasyMemberSelect`, `match::ai::TeamMemberChange`, `match::pad::ThinkUnitQucikMemberChange` (cambio rápido de jugador con el mando). Relevante para N jugadores y para el cambio de jugador. | H |
| Barrido «< 7» | Ninguna comparación `< 7` en las funciones del bloque 0x1404xxxx–0x1408xxxx parece contar jugadores en campo; las candidatas son de IA/posición. La regla de «menos de 7» sigue sin localizar. | negativo parcial |

#### 25-oct-2026 — La «puerta» del Entrenamiento libre (flujo de menús en JSON)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| **Flujo de menús = JSON editables** | `dt16_all.cpk` → `common/script/flow/**.json` (2158 archivos, SIN comprimir; se extraen leyendo `posicion`/`tamano_real` del catálogo `informes/datos/catalogo/cpk/dt16_all.cpk.tsv`). Cada pantalla es un JSON con `events{nombre:{to, go, clear, fade…}}` y `data{type: menu/process, bgId}`. **Todo el árbol de menús de PES se puede re-cablear con archivos** (Sider livecpk) sin tocar el exe. Abre la puerta a menús propios, atajos, saltarse pantallas, modos ocultos. | C (leído) |
| **Entrenamiento libre existe y está cableado** | `TopMenu/TopMenuNew.json` tiene el evento `05_freeTraining` → `Match/TrainingUgcCheckJoinMode` → `Match/MatchTrainingTeamSel` (bg EXHIBITION; return → TopMenu) → `TrainingPrePlayerSelect` → `MatchTrainingMatchMenu` → `MatchTrainingSetting` → `TrainingMatchInit` → `Match/MatchSetup`. Solo falta el **botón** en la pantalla principal (el exe tiene las cadenas `freeTraining`, `freeTrainingMenu`). | C (flujo) / H (que funcione) |
| Puerta A (fácil, reversible) | Copiar `TopMenuNew.json` a la carpeta de contenido de Sider y cambiar `04_tutorial.to` de `Tutorial/TutorialUgcCheckJoinMode` a `Match/TrainingUgcCheckJoinMode`: el botón Tutorial abriría el entrenamiento. Riesgo: que el código del botón prepare estado de «tutorial» y el entrenamiento no arranque. | H, sin probar |
| Puerta B (difícil) | Hacer visible el botón oculto `freeTraining` en `common/menu/general/topModeSelectDMM*.bin` (Flash) o en la lista de botones del exe (`menu::TopModeSelect`, vftable 0x14266DDD8). | H |
| Datos de entrenamiento | Menús `common/menu/general/trainingSelect.bin`, `trainingSkillSettings(Team).bin`, `playerTrainingSetting.bin`, `common/menu/mode/trainingMainMenu.bin`, `trainingResults.bin`, `trainingFloatingScore_1..3.bin`; objetos `Asset/model/training_prop/` (conos, maniquíes, barreras x3–x5, paneles de portería). Estadísticas `PLAY_DATA_TRAINING_FREE`, `_FREE_12`, `_FREE_22` (¿entrenamiento con 12 y con 22 jugadores? → el motor admite cantidades distintas). | C (existen) / H (significado) |
| Ideas que abre | Modo street dentro del entrenamiento; minijuegos de habilidad con conos/maniquíes; retos de tiros libres/penales/córners (`PLAY_DATA_TRAINING_FREE_KICK/PK/CONER_KICK/GK`); menú Phoenix propio en la pantalla principal; acceso directo a un partido (`Match/MatchDirectSetup`). | idea |

#### 25-oct-2026 (noche) — Prueba en vivo de la puerta del Entrenamiento libre
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Desvío por livecpk FUNCIONA | `SiderAddons\livecpk\Phoenix-Entrenamiento\common\script\flow\TopMenu\TopMenuNew.json` (copia de `livecpk\Menu`) con `04_tutorial` y `06_myclub` → `Match/TrainingUgcCheckJoinMode`. En Conmegol el botón «ENTRENAMIENTO» usa el evento **06_myclub** (el parche remapea eventos del TopMenu: 07→comentario, 08→sonido…). El JSON no se recarga en caliente: hay que reiniciar PES. | C (en vivo) |
| Pantallas del Entrenamiento libre | «Empezar» → «Local/Visitante» (elegir 2 equipos a la vez) → «Estrategia» de ambos equipos (11 por lado) con reloj ~114→0 → menú previo (Estadio, Empezar partido, Estrategia, Opciones generales, Cámara, Lista de usuarios). «Empezar partido/Estadio/Opciones generales» salen ATENUADOS: falta algo que el menú original prepara (parece pre-partido de sala). | C (en vivo) |
| Teclado | PES solo acepta teclado por scancode; Espacio abre el overlay de Sider. | C |
| Pendiente | Leer `MatchTrainingSetting`, `MatchTrainingMatchMenu`, `TrainingMatchInit` (en `livecpk\Menu\common\script\flow\Match\`) para saltar el lobby; probar Start con mando real. Informe largo: `PRUEBA_CANCHA\informes\ENTRENAMIENTO_LIBRE_PRUEBA.md`; capturas `informes\datos\entrenamiento\` 22–37. | — |
| Idea | «Entrenamiento libre» propio con 2 equipos editables a la vez. | idea |

#### 25-oct-2026 — Búsqueda web: parches de gameplay «street» para PES 2021
| Qué | Dónde / formato | Confianza |
|---|---|---|
| No existe un mod «street» ni futsal/5v5 conocido | Búsquedas en pesmodding.com / evo-web: solo mods de gameplay 11v11 con regate mejorado (HOS 2023 de Hosamalfars, EVO Soccer V7 / Overhaul de elijio876, Mystery/Scream/Modern Soccer de Alex, Real PES, Direct and Fast). Nadie ha hecho cancha pequeña ni menos jugadores → lo nuestro sería inédito. | C (búsqueda web) |
| Dónde vive el gameplay | Casi todos cambian `dt18_all.cpk` (o el exe entero); «Direct and Fast» va por módulos Lua de Sider. → los parámetros de regate son DATOS en `dt18_all.cpk`, se pueden estudiar y cargar como opción on/off por livecpk (sin exe modificado, válido en PES limpio). | H |

#### 25-oct-2026 (noche, 2) — Lectura estática del flujo del entrenamiento
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Cadena completa | `MatchTrainingTeamSel → TrainingPrePlayerSelect → MatchTrainingMatchMenu → (00_kickoff) MatchMenuEnd → MatchTrainingSetting (menú de ajustes) → TrainingMatchInit → Match/MatchSetup`. Frente a la exhibición se salta `ExhibiFlowInit` / `ExhibiChangeCompeDesignKind` (modo y estadio); el TopMenu de Conmegol no pasa por `TopMenuModeInit`. | C (flujo) |
| «Empezar partido» atenuado: candidatos | (a) en la prueba se eligió el MISMO equipo en ambos lados; (b) faltan los pasos de init de la exhibición. Bandera exacta no hallada en Ghidra. | H |
| `TrainingDirectInit` | Registrado en el exe junto a `MatchDirectInit`, pero sin JSON: entrada directa posible creando el JSON. | H |
| `freeTrainingMenu` | Evento de la pausa en el modo 11 (= entrenamiento libre). | C |
| Menús .bin | `trainingMainMenu.bin` = retos del Tutorial (Básico…Experto); `trainingSelect.bin` / `playerTrainingSetting.bin` = entrenamiento de jugadores de ML/BAL. No hay menú «Libre/Tiro libre/Penal» antes de elegir equipos. | C |
| Mini-campo | `loaderSwf_trainingPlayArea` (área de juego delimitada del entrenamiento) + modelos de conos, maniquíes, paneles y líneas de 6–42 m en el exe → base para modo de retos o mini-cancha. | H |
| Aviso | En el TopMenu de Conmegol `12_freelobby` y `18_info` → `Match/MatchDirectReplay` (cuidado con las repeticiones reales). | C |
| Pruebas preparadas (desactivadas) | P0: repetir con 2 equipos distintos. P1: `SiderAddons\livecpk\Phoenix-Entrenamiento-B1` (TrainingPrePlayerSelect → MatchTrainingSetting). P2: `-B2` (→ TrainingMatchInit directo). P3: crear `TrainingDirectInit.json`. Activar = línea `cpk.root` en sider.ini con respaldo. Informe: `PRUEBA_CANCHA\informes\ENTRENAMIENTO_LIBRE_PRUEBA.md` («AMPLIACIÓN»). | — |

#### 26-oct-2026 — Online de PES 2021 (lectura del código de Nikow5/Pes2021PrivateServer, MIT, feb-2026; hilo «Planear Phoenix Core al límite»)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Transporte TCP «NclMio» | Puerto 10000 (ConnectGate). Cabecera big-endian `[cmd 2][largo 2][secuencia 4]` + datos; todo el paquete con XOR cíclico de clave `5B 9F 2E 64`. Algunas respuestas van con zlib (0x2EE4). | P (código de terceros, parte heurística) |
| Comandos | `0x2EE0` CmdLobbyInit (ticket de Steam, 124 B) · `0x2E04` CmdMatchmaking/Auth (respuesta de 96 B) · `0x2EE4` CmdGetSvrList (128 B, entradas de 32 B que empiezan con la vftable `0x14285B5B0`) · `0x2EF4` CmdGetEulaAreaList. El cliente parece deducir el comando por `largo XOR 0x2E64`. | P |
| Direcciones del exe 1.07.02 | `0x1411AEEC0` validador de paquetes (origen del error C_IDGG_0002) · `0x140FE0C50` estado 5 · `0x140FE1FE0` estado 6 · `0x140FF3EC0` estado 10 de la conexión online. Trazador Frida `trace.js`. | P (comprobar en nuestro exe) |
| STUN | RFC 5389 estándar (cookie `0x2112A442`), puertos 5730–5739 / 3478. Partido P2P por UDP 5739/5740. | P |
| Estado del proyecto | Solo login, lista de servidores, EULA y matchmaking inicial; sin lobby ni partido. PESBUL (Docker `noobsaibot301/pes2021-emulator`) va más lejos (amistoso 1v1), pero su código no apareció público. `NoobSaibot888/pes_server` (sin licencia, 2025) no conectaba. | C (repos leídos) |
| PESBUL verificado (metadatos públicos de Docker Hub, 26-oct 19:10 UTC; sin descargar capas) | Imagen `noobsaibot301/pes2021-emulator`, pública, 38.018 descargas, creada 23-jun-2026, **última versión hoy 26-oct 17:46** (`0.13.10-phase13-r7.3.743.84-CLUBS-UNIQUE-MENU-EVENTS`), ~84 MB comprimida. Python 3.12, `/app/entrypoint.sh`, panel en :5000 (`/health`). Sin licencia ni enlace a código fuente en la imagen. | C |
| PESBUL: qué tiene | Variables y etiquetas muestran: **co-op** (grupos, salas, cuenta atrás vs COM), **Team Play** 11v11 («avanzado por RE»), **Clubs** (menús, bandeja, calendario de liga, perfiles), **relé de partido propio** (`PES_GAMERELAY_*`, plano de datos en 5100 TCP/UDP, intercambio de claves «KX» con códec nativo) → el partido NO va solo P2P: puede pasar por su relé. Sirve **Live Update** (datos en vivo de 2021-08-26, hash sha256). Puertos: 10000/10001 TCP, 3478–3479, 5730/5739/5740 UDP, 55000–55100 UDP, 53 UDP (DNS propio), 80/443. | C (metadatos) / P (significado) |
| Dato curioso | Según su etiqueta, el cliente de PES añade 1002 bytes en cero al final de cada archivo de Live Update que descarga (constante del cliente, papel desconocido). | P |

#### 26-oct-2026 (b) — Online: formato NclMio ampliado (hilo «Crear servidor Phoenix Online»; lectura de Nikow5, MIT)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Respuesta de la secuencia | La respuesta debe repetir la `secuencia` (4 B) del paquete contestado; 0x2E04 y 0x2EF4 además copian los primeros 8 B del token de la petición. | P |
| Tamaño = identidad | Un 0x2E04 de 64 B se lee como 0x2E24 y da `C_IDGG_0002`: el largo exacto es obligatorio (96 B). | P (de comentarios del autor) |
| Entrada de servidor 32 B | vtable `B0 5B 85 42 01 00 00 00` · id (BE) · tipo (BE) · IP · puerto **little-endian** · relleno; 4 entradas, zlib. | P |
| Debilidad a resolver con REA | Nikow5 deduce el comando por tamaño de paquete y no hace framing TCP; confirmar en el exe de dónde sale el cmd. | C |
| Guía completa | `/mnt/project-files/pes-online/PHOENIX_ONLINE_REA_GUIA.md` (plan R0–R8). | — |
| Online: máquina de estados del login (exe 1.07.02, según documentos de PESBUL) | switch `0x140fde489`; estado 5 AuthSteam `FUN_140fde810` · 6 `FUN_140fdebe0` · 7 CmdLoginAccount `FUN_140fdf300` · 8 CmdSendPsid `FUN_140fdf860` · 9 CmdGetPlayerData `FUN_140fdee40`; `FUN_140fe12e0` versión/URLs/hora · `FUN_140fe1fe0` lista de servidores · `FUN_140fe3f50` LoginMenu · `FUN_140fe4470` SetCurrentPlayer. Mensajes MessagePack con `msgid`/`rqid`/`result` (`NOERR`). 339 comandos `CMD_*`. Amistoso = P2P con NAT de Konami (`NtlManager`, `P2pFullMesh`) + respaldo TURN (`P2pFullMeshWithTurn`); Team Play = relé (`CsClientSession`, `ClltcpController`). Detalle: cap_72. | P (verificar en nuestro exe) |

#### 26-oct-2026 (c) — Phoenix Core: gol, eventos y portabilidad Conmegol → PES 2021 limpio (hilo «modo street»)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Código del exe | En el exe de Conmegol el código está en la sección **«.trace»** (no «.text»); hay además .xtext/.xcode/.text1/.sxdata. Core busca firmas en TODAS las secciones ejecutables. **Comprobar en PES limpio** el nombre de sección. | C (Conmegol) |
| SetState(actor,estado,modo) | firma `48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 8D`, RVA 0x6EEBB0 en Conmegol; prólogo 15 B sin RIP-relativos | C |
| ProcesarGol | firma `48 8B C4 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 D8`, RVA 0x478190; prólogo 14 B | C |
| PonerEvento(job,tipo) → job+0x874 | firma `48 89 5C 24 08 57 48 83 EC 20 48 8B D9 89 91 74 08 00 00 48 8D B9`, RVA 0x451FD0; prólogo 19 B | C |
| Tipos de evento | amistoso: gol = 0xA (montador FUN_14044F8A0, ret 0x44F8E3) → ProcesarGol; otros 2,4,5,6,7,9. Entrenamiento: gol = **5 (saque de puerta, k==7 sub==3, ret 0x44F444)**, banda = 6 (sub==2, ret 0x44F7A4), reinicio 0xE (0x4535F1 / 0x449BA9) | C (sonda en vivo) |
| Selector de parada | FUN_1404526A0: k=[obj+0x1574], sub=[obj+0x1554], obj=[[0x1436F3DC8]+0x270]; k==3 → gol | C (estático) |
| **Portabilidad (pendiente)** | Core 0.3.x usa aún RVAs fijas de Conmegol para: global 0x36F3DC8, retornos 0x449BA9/0x44F8E3, FUN_14044F8A0. Antes de usarlo en PES limpio hay que sacarlas por firma (instrucción RIP-relativa que lee el global, firma del montador). Las firmas de los 3 ganchos sí son AOB. Sospecha: si el exe limpio es otra versión (Conmegol = PATCH10100), las RVAs cambian; las firmas probablemente sirven. | P |
| Archivos | La prueba de Core no depende de cpk/livecpk de Conmegol; sí usa Sider (cualquier Sider con luajit.ext.enabled=1). Las pruebas de cancha (pitch_st002 del Camp Nou, goal_post, .geom en livecpk) SÍ eran de Conmegol: en PES limpio las rutas de estadios/AddOn serán distintas. | C |

#### 27-oct-2026 — Gol nativo completo en el Entrenamiento libre (hilo «modo street»)
| Qué | Dónde / formato | Confianza |
|---|---|---|
| Culpable del «saque de puerta» | FUN_1404A56C0 (gestor de paradas del entrenamiento, TrainingWork `[[0x1436F3DC8]+0x400]`) fuerza k=7 con 2× `mov edx,7; call FUN_141B8B250`. Firma `BA 07 00 00 00 49 8B CE E8 ?? ?? ?? ?? BA 07 00 00 00 48 8B CB E8` | C |
| Índice 0xFF sin comprobar | FUN_140A62690(lado,p) → 0xFF si no hay jugador; FUN_1405B1740 lo usa como ranura (2 sitios, firmas en cap_73 §4). Afecta a todo modo con <11 por lado | C |
| Monitor de cola de carga | FUN_1403E0760: `while(!listo[+0x9C] && cola[[p+0x88]+0x28]==0) esperar`; hilo de render FUN_14170FA10; spin-yield FUN_1403DB2C0/DB590; zlib inflate_fast FUN_1403B6AA0 | C (estático) |
| Demo de entrada al marcar | En entreno, al marcar se pide `…\demo\fixdemo\ent\table_ent.bin` + Entrance.lua ×4 (ruta trofeo/entrada); en amistoso no. Tablas fixdemo se inicializan en FUN_141EBCB10/141EC68A0/141ECA210/141F29420 (DAT_1437F4E60) | C |
| Error Lua de Conmegol | TournamentCornerFlags.lua l.98: `CornerFlag` nil para torneo 29 (entreno); DESCARTADO como causa del congelón (con el original no se congeló). Regla para el parche propio: módulos Lua deben tolerar torneo nil | C |
| Señal de cuelgue | CPU alta NO distingue cuelgue (~5 núcleos normal, 125–130 hilos). Señal buena: sonda sin escribir ≥8 s y sin «Commentary file detected» | C |
| sider.ini Conmegol | `Stadium_CornerFlag.lua` (l.164) y `CornerFlag.lua` (l.188) no existen (inocuo) | C |
| Entreno: estadio | stadium=2, stadium_choice=65533 (sin elección) | P |
| Reinicio tras gol | registro de parada sub: 1 = saque de centro (amistoso), 3 = saque de puerta (entreno). Rama k==3 de FUN_1404A56C0 en 0x1404A5B78 lo fuerza a 3; jne en firma+10 → jmp lo evita | C (sonda) |
| Selector PES2021 Start.exe | CONFIRMADO (27-oct): restaura módulos Lua del Conmegol al arrancar (un arreglo en TournamentCornerFlags.lua desapareció) | P |
| Secuencia del gol | FUN_14044F8A0 tras 0xA: LARGA (0x03,0x11,0x08,0x23,0x16,0x12,0x13,[0x17],0x14,0x15,0x0E,0x34,0x19,0x1A: celebración+repetición) o CORTA (0x43,0x07,0x44) si byte `[[root+0x2C0]+0x84]` != 0 (FUN_140A3F580). Salto en 0x14044F941 `74 47` | C (estático) |
| Deadlock en la repetición (entreno) | juego FUN_141717150: `while([[0x143747E98]+0xC4]==0) esperar`; render FUN_14170FA10: `while([obj+0xC0]==0 && ![obj+0xD0]) esperar`. Esperas FUN_1403F7BC0/1403F0330/1403E0760, spin 1403DB2C0. Bucle principal FUN_1404002D0 | C (perfiles vivos) |
| TrainingWork activo | `[root+0x400] != 0 && *(byte*)[root+0x400] != 0` (criterio de FUN_14044D110) | C |
| ⚠ Protección anti-manipulación | Sección «.impdata» (rva 0x3D14000, ~0x17C3D94F, ejecutable) = VM de protección del exe. Parchear FUN_14047C2D0 (ejecutor de pasos del gol, hook de 17 B) → crash 0xC0000005 DENTRO de .impdata segundos después (2 veces: +0x118074F5 en menú, +0xD1D4317 al entrar al entreno; llamador .trace +0x44521D). SÍ se pueden parchear sin problema: SetState, ProcesarGol, PonerEvento, montador FUN_14044F8A0, cuevas en FUN_1404A56C0 y FUN_1405B1740. Regla: si un parche nuevo provoca crash en .impdata, esa función está vigilada → usar sondeo desde un hilo propio (leer memoria) en vez de parchear | C |
| Cola real del gol (entreno) | job+0x878[]: `03 04 45 1D 03 11 08 23 16 12 13 14 15 0E 34 19 1A` (n en [+0x8F1], índice en [+0x8F0]). 0x04 = celebración (~12,7 s). **0x1D = paso donde se congela el entreno** (lo encola el montador si !FUN_140A3C020, también delante de la secuencia corta) | C (sondeo en vivo) |
| FUN_140A3C020(partido) | = byte [partido+0x177F]; si 0 el montador encola 0x1D. Ponerlo a 1 durante el montador quita 0x1D | C |
| Cola tras celebración (entreno) | se amplía a `… 10 26 45`; paso 0x07 = siguiente cambio de cámara, también se cuelga | C (sondeo) |
| Pasos de la cola del gol (inferido) | 03 = procesar gol; 04 = celebración (goal_celebrate_*); 45/43/44 = envoltorio HUD/cámara; 1D = cambio de escena; **07 (+10 26 45) = demo de fin de tiempo** (fixdemo\timeup\tu_half_cmn_01 + dml_ent_entering06); 16 12 13 14 15 0E/0C 34 19 1A = reinicio (igual que tras saque de puerta). Ejecutor: switch en FUN_1404D21F0 | P (por archivos pedidos y orden) |
| Cola segura en entreno | `03 16 12 13 14 15 0E 34 19 1A` → gol + saque de centro sin congelarse (0.4.14) | C (probado por Frank) |
