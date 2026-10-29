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
| Entreno libre: TrainingWork | Raíz `[0x1436F3DC8]`; `+0x3E8` vftable `0x142598078` (TrainingWorkRef); `+0x3F8` → objeto (vftable `0x142597FD8`) → `+0x10` = datos: `+0` activo, `+4` TIPO (FUN_1408E1C50; siempre 1 aunque el menú diga Ataque o Defensa), `+0x14` (FUN_141030AB0, =2), `+0x1C` (FUN_140DD1510), `+0x20` puntero a 12 B (posición inicial x,y,z; FUN_14217D310), `+0x40`=jugadores usuario. En el menú de entreno el valor Atacante/Defensor NO está aquí; el defensor lleva el portero dentro de su cifra. Raíz+0x400 vale 0 (no es el puntero real). | C (en vivo, 27-oct) |
| Cola de pasos del gol: pasos | 0x03 procesar gol; 0x04 celebración (~21 s); 0x07 «¿fin de tiempo?» → demo timeup/tu_half (congela en el entreno); 0x1D repetición/cámara (congela: el montador solo lo encola si `[partido+0x177F]`==0 = FUN_140A3C020); 0x16 0x12 0x13 0x14 0x15 0x0E 0x34 0x19 0x1A reinicio nativo; escena 0xE (0x39/0x3A, 0x0F, 0x37) y escena 3 = saque de centro. | M (por orden y archivos pedidos) |
| Timer de celebración | `job+0x27D0` (dword, ≈56/s desde el gol); `job+0x27C0/27C8/27CC/27D4` contadores de fotogramas; `+0x27D8` fase de escena; `+0x27DC` flags; `+0x900` nº de reinicios. Saltar el índice `[job+0x8F0]` en mitad del 0x04 provoca deadlock render en el 2.º gol. | C (en vivo, 27-oct) |
| Deadlock juego↔render | Juego: exe+0x3F3AE1→0x3DC7DE→0x3FF44A→0x171717A (FUN_141717150)→0x3FF4AA→0x1957C8D; render: 0x3F7C15→0x3F0346→0x3F030E→0x3E07DB (FUN_1403E0760, cola de carga). Aparece en transiciones de escena con demos (replay 0x1D, timeup 0x07, salto del 0x04). | C |
| Duración de la celebración del gol | El paso 0x04 dura lo que la animación de celebración elegida: medido 874 y 1659 ticks de `job+0x27D0` (~54/s = 16 s y 31 s). No hay valor final fijo. | C (en vivo, 27-oct) |
| Recorte de la celebración (timer) | Escribir `job+0x27D0=4000` a los 8 s: el paso 0x04 termina 4–7 s después (celebración total 12–15 s). Saltar `[job+0x8F0]` provoca deadlock render en el 2.º gol. | C (en vivo, 27-oct) |
| Modo seguro del Core | `arrancando.flag` presente al arrancar ⇒ todas las funciones apagadas (síntoma: el gol vuelve a saque de puerta). Se crea al abrir y se borra a los 60 s estables. | C |
| Entreno libre: portero del grupo atacante | Con «Atacante» y N jugadores usuario, solo `tw+0x40`=N cambia (N=4→0x0B no mueve ninguna otra cifra de TrainingWork); el portero propio aparece solo con N=11 (plantilla completa) o con tipo Defensa. No hay flag de portero. Los jugadores son `match::AnimePlayer` (vftable `0x1425E34D8`), solo los N+COM creados. Registros del partido: lista de 5 qwords [vtable, flag, objeto, 0, 0] en `[0x1436F3DC8]+0x000…0x500` (TrainingWork en +0x3E8); `[root+0x270]` (partido) vale 0 salvo en instantes puntuales. Hipótesis: el grupo atacante toma los N últimos del once. | C (lectura) / M (hipótesis) |
| Bloque de dimensiones del campo | 36 B / 9 floats en el montón por partido (2 copias vivas): [0] 52.56 semilargo, [1] 34.06 semiancho, [2] 3.66, [3] 2.44, [4] 47.06, [5] 9.16, [6] 36.14, [7] 20.12, [8] 41.56. Firma `70 3D 52 42 70 3D 08 42 71 3D 6A 40 F6 28 1C 40 70 3D 3C 42 5C 8F 12 41 5C 8F 10 42 C2 F5 A0 41 70 3D 26 42`. Reescribir [0],[1],[4..8] = cancha lógica menor (verificado 30×20 con Core 0.4.20 `cancha_chica_free`). | C (en vivo, 27-oct) |
| Modelos de portería (.fmdl Fox v2.03) | Vértices float32 xyz con paso 12 B desde ~0x350 y bbox en 0x240/0x250, con x≈±52,5 (goal.fmdl: 410 floats; redes ~4 800). Mover los floats 44<|v|<60 → arcos a ±15 (script `paquetes/phoenix_core/herramientas/mover_porteria_fmdl.py`). La traslación del fox2 `StadiumModel` NO se aplica. Los goalnet no están en los CPK de Data: salen de AddOn «Goal Post Colour» de otros estadios. | C (captura, 27-oct) |
| CPK anidado de estadio | `Data\dt30_g4.cpk` contiene `Asset/model/bg/st002.cpk` (190 MB: pitch_st002.fpk/fpkd, pitch_alp/pitch_detail/cm000_line*/st002_turf* .ftex). CriPakTools (esperknight) compara mal rutas con mayúsculas: pasar la ruta en minúsculas con el fix local `CriPakTools_fix.exe`. | C |
| Cajas de portería (.data) | `exe+0x37CF4C0` (96 B, 6 vec4); copiadas de la tabla `.rdata` `0x142ABDB00` por `FUN_1402142C0` («AndPhPreProcess»); solo las lee `FUN_141B0D290` (filtro del job `cobra::And::PhPreProcessJob`, ctor `FUN_141B0D4E0`): devuelve 0 (no probar) si un cuerpo con bit 0x22 de `[body+0x1B8]` queda fuera de ambas cajas de portería; `[job+0x6CC]` = 1 si el cuerpo sale del rango z (FUN_141B0D3F0). Valores reales: 52,44 / 55,5047 / 2,903 / 4,354449 / 4,35472 / 35,5. | C |
| Colisiones de portería (.geom) | `common\render\model\bg\hit\goal\goal_l|r.geom` (dt15_x64.cpk, 4 957 B) y `common\anime\FoxAnim\goal_hit|goal_l|goal_r.geom` (dt13_all.cpk): cajas/cápsulas con centros float32 en x≈±52,5/±53,97/±55,47. Se pueden servir desde Sider (livecpk_get_filepath) y quedan en memoria desplazadas (0x7AEE0DC0). Desplazarlas NO hace que la red frene la pelota. | C |
| CPK anidado y CriPakTools | `Data\dt30_g4.cpk` → `Asset/model/bg/st002.cpk`. CriPakTools (esperknight): la comparación de ruta falla con mayúsculas en el directorio; usar la compilación `CriPakTools_fix.exe` con la ruta en minúsculas. FtexTool v0.4.0 NO lee los .ftex de PES 2021 (FTEX v2.04). | C |
| Césped pitch_st002.fmdl | Materiales `st081_pitch_0/1` (shader `pes3DDF_Pitch2020`, texturas pitch_alp/pitch_detail/line_*/turf_*); vértices float32 xyz (bloques 126/1088/60/7364 vértices); escalar x,z encoge césped+líneas pero rompe la luz (Enlighten) y deja el fondo blanco. | C |
| Frida MemoryAccessMonitor re-armado (2 ms) sobre posición del balón | PES se cierra solo; una pasada sencilla no. Ver cap_02 (27-oct). Alternativa: estática, xrefs de la transformada del cuerpo AndPh |
| ThinkUnitShoot / ThinkUnitClear (vftable exe+0x260FB58 / 0x260FEE8, fábrica FUN_1408AD0B0; dueño: match::CommandPlayer, ctor FUN_1408B3BC0, unidades en +0x30) | Ambos usan el botón 0xF. Despachador común FUN_1408AC500 (ranura +0x40): estado +0x1C (1 pulsar, 2 mantener, 3 soltar), llama ranuras +0x60/+0x68/+0x70/+0x78. Clear: +0x60 devuelve 0x58 al pulsar; Shoot devuelve 0x55 a los 0,148 s y al soltar. Predicado «tiro permitido» FUN_1406B45D0 (único llamador FUN_1408AC500): falso en campo/área propia → gana Clear. Forzarlo a 1 hace cargar el tiro pero se cancela antes del contacto: hay una validación posterior no localizada (27-oct) |
| Acciones 0x55/0x58 | Tabla de 5 dwords por id en exe+0x26468F0: 0x55=(25,3,0,100,257), 0x58=(21,3,0,100,257); ambas categoría 3 (FUN_140A65DB0). 0x54 y 0x56 son categoría 4 |
| Season (clima) | ctx.season 0 = verano, 1 = invierno; lo elige Real-Life-Feel-Weather-Manager por el mes real: por eso cambia entre arranques. Con season 0 la cancha chica muestra vacío blanco y césped oscuro; con season 1, vacío azul y césped claro. Opción: «season_forzada» en phoenix_core.json (módulo phoenix_marcador_free.lua v0.5, evento set_conditions) |
| Frida MemoryAccessMonitor | Ver cap_02: re-armado en bucle tumba PES |
| Medidor de potencia (barra bajo el jugador) y orden de mando (27-oct) | Se calcula en FUN_1408AAA30: lee el CommandPlayer de cada ranura (objeto en [param_1+0x8620+slot*8]); el valor del medidor = CommandPlayer+0x10 (carga) y FUN_1408B4330 lee +0x3C/+0x38. FUN_1408AB000: si el tipo de orden (CommandPlayer+0x1768) es 4 → medidor = 1.0 (lleno); tipos 2/0x35 solo si carga ≥ umbral. El tipo sale de la unidad ganadora: ThinkUnitShoot tiene +0x14 = 1 y ThinkUnitClear +0x14 = 4 |
| Carga de la unidad (+0x20) | FUN_1408AC9D0 (ranura +0x68 de Shoot, vía FUN_1408B9AF0) suma FUN_1408AB9E0(unidad, pad, id de acción) por fotograma (~0,074) con tope 1.0, y sale de inmediato si +0x14 == 4 (Clear no acumula). Por eso: cuando gana Clear (en campo/área propia, Shoot desactivado o con predicado falso) el tiro sale con medidor lleno aunque se dé un toque |
| Despachador FUN_1408AC500 | Empieza con «si el byte +0x18 de la unidad es 0 → devuelve 0» (unidad desactivada); el estado +0x1C cambia en FUN_1408AC860 (+0x80 de la vtable de Shoot). Quedó sin confirmar quién pone +0x18 a 0 en campo propio y cuál es el filtro posterior que cancela una orden de tipo 1 antes del contacto |
| Pruebas del 27-oct (Core 0.4.27–0.4.34) | forzar predicado a 1: carga pero cancela; Clear devuelve 0: no patea; Clear devuelve 0x55 en pulsar+mantener+soltar (modo 4): patea, barra llena; modo 5/6: barra llena igual (tipo 4). Modo 7 (0.4.35): Clear con tipo 1 y carga acumulada |
| Registro de movimiento del jugador (27-oct) | *(raíz + (i+0xBF)*0x28) con i = 0..25 (accesor FUN_140A48A10; FUN_140A48910 usa +0xA5; FUN_140A48A30 equipos +0xA1). Posición lógica (x, y, z) en rec+0x55C/+0x560/+0x564 (la lee FUN_1406B45D0). Core 0.4.38 jugadores_sonda las anota cada 3 s |
| CommandPlayer en vivo | cmd=0x12A7B5800 (sesión del 27-oct); al pulsar cuadrado el tipo cmd+0x1768 pasa 0x4E → 4 y el medidor 0 → 1,0 aunque Clear+0x14 valga 1; cmd+0x10 (carga) sigue 0 mientras la unidad Clear acumula 0,074…0,296 en su +0x20. Vigía de escritura (Core 0.4.37, DR0/DR1 + VEH) para ver quién escribe |
| x64dbg adjunto a PES | Congela el juego (responde=false) y, al cerrar x64dbg, PES muere con él; sin control de la ventana (computer-use denegado) no sirve. Alternativa: vigía de escritura dentro del Core con registros de depuración |
| Sider matchset / suplentes | set_match_settings(ctx, options): options.substitutions; matchset.lua lo pone a nil fuera de tournament 65535. phoenix_marcador_free.lua 0.6: «suplentes_cero» |
| Pelota del partido (frente B, 27-oct) | NO es cuerpo rígido: la mueve un modelo analítico propio FUN_140876360 (RVA 0x876360, firma AOB 22 B `48 8B C4 F3 0F 11 40 08 55 53 56 48 8D 68 88 48 81 EC 60 01 00 00`) sobre estado S de 272 B: S[0..2] posición (x largo, y, z ancho), S[3..5] velocidad, S[6..8] giro, radio 0,10869. Se predice 250 pasos por fotograma (FUN_140876200, FUN_140876020, bucle FUN_140873EE0). BallInfo en [[raíz]+0x40] (entradas de 40 B desde +0x88). BallControl = [exe+0x36F4090]; actor BD = [BallControl+0x20]. No hay colisión con .geom: por eso la pelota atraviesa la red. Core 0.4.41 red_frena_free amortigua S[3..5] en la red |
| Quién gana entre ThinkUnit (frente D, 27-oct) | cmd+0x1768 = id de la unidad ganadora (3 Shoot, 4 Clear, 1 LongPass); el objeto cmd tiene el gestor de unidades en cmd+0x40, así que el campo es [reg+0x1728]. Escritor mov [rsi+1728h],edi en FUN_1408B1F90 (RVA 0x8B204D). FUN_1408B08A0 empuja Clear antes que Shoot; habilitación por ranura +0x88: Shoot FUN_1408B9C80 (no en mitad propia con mode+0x116C==1), Clear FUN_1408BB0D0 (solo en mitad propia). FUN_1408B1490 case 3: si id==4 la potencia = 1.0. Parches de 1 byte A 0x1408B9CE8 75 22→EB 22 y B 0x1408BB150 76 34→EB 34 (modo 8 del Core 0.4.40) |
| Posición de reinicio de los jugadores (frente C, 27-oct) | Tabla de Vec3 en metros absolutos de la cancha grande: team+0x4B4C+(idx%11)*12, team = *(raíz+(side+0xA1)*0x28), side=(idx>10&&idx<22). La lee FUN_1409F5530(out, uint* pIdx, ctx, teamAI) (RVA 0x9F5530, firma `48 8B C4 53 41 57 48 83 EC 78 48 89 70 E8`). Cadena: FUN_140925470 → FUN_140926B00 (switch [raíz+0x270]+0x11B0) → FUN_14092E7E0 (Player+0xACA0) → FUN_140929610 (recorta a dims, salvo pateadores) → FUN_1409232C0 → FUN_141EB5B30 (poner posición del actor). El primer teletransporte no se recorta: por eso nacen fuera. Fila de banda de suplentes: FUN_1409266E0 → FUN_140926940 → FUN_140A597D0 (x=(L/20)*idx-L/2, z=W+2,5; RVA 0xA597D0). Aparcamiento del juego: (0,y,100). Core 0.4.43: jugadores_chica_free y suplentes_ocultos_free |
| Idea: evitar la «cámara lenta» online en WAN (29-oct, informes Gemini/PESBUL, P) | En internet real el partido arranca lento con el icono de carga hasta que el balón sale por primera vez (resincroniza el reloj de Fox Engine). Idea para Phoenix Core: leer el contador de fotogramas de simulación de ambos clientes y microajustar el reloj del bucle ±1 % durante ~0,5 s para igualar sin esperar al balón. Sin probar; GGPO/rollback completo NO sirve (22 jugadores). Función para encender/apagar en PES limpio |
| Cliente HTTP de PES = WinHTTP (29-oct, C) | Agente «WinningEleven/16.0» (fn 14116f910 abre, 141170e90 arma, 141171c50 callback). Bandera SECURE si el esquema es https; sin WinHttpSetOption en esas 3 fns (validación normal de Windows, sin pinning visible). Cuerpo JSON o formulario según byte +0x15c del objeto |
| API de red del exe resuelta en ejecución (29-oct, C) | getaddrinfo/gethostbyname/connect/WinHttp* se piden por nombre con GetProcAddress (getaddrinfo en fn 1411987b0, puntero 1434d6398): no hay IAT estática. Engancharlas con MinHook desde Core/Sider (redirigir *.konami.net sin hosts ni administrador) es la vía limpia |
| Tabla de URLs de CMD_GET_URLLIST (29-oct, C) | 53 tipos en 0x1437F3A20, lectura solo por FUN_141b9a250(tipo); consumidores directos: BANNER 5, P2PMATCH 7, MYCLUB_AGENT_BANNER 17, DMM_TOP 18, LIVEEVENT_BANNER 39, WEEKLY_INFO_BANNER 45, WATCHING_GAME 46. ConnectGate (fn 140fddef0) no valida ningún tipo |
| Dominios del conector que funciona en PESBUL (29-oct, informe, P) | gate, stun, pesam.stun.service.konami.net, info.service.konami.net, cs.konami.net. En el exe recortado NO aparecen pesam.stun.* ni pes21-pc.konamionline.com (0 coincidencias en ASCII y UTF-16); sí ntl/ntleu/ntljp/ntlus (2 c/u), info (1), gate (1), stun (1) |

## Hallazgos 29-oct-2026 (GhidraMCP sobre PES_MCP_RECORTADO, 12.0.4)
- **ThinkUnitGKClear = FUN_142132B40** (vtable, sin callers directos): despeja si rival más cercano al balón a < R, con R = 12 m (balón cerca del arco) o 15 m (lejos) (.rdata: DAT_14259be04=12,0; DAT_14259be0c=15,0; DAT_1425a2670=400,0). Ver frente_W §8. Idea de función nueva (apagada): `arquero_controla` (despeja solo con balón < 4,5 m del arco y rival < 2,5 m; si no, controla y pasa).
- **Marcador (FUN_1408C9E90 y callers):** todos los EQ salen de `FUN_1408C7290(M+0x750D0, lado)`; los callers FUN_140A202D0/FUN_140A20FC0/FUN_140A23940 usan slot≠0 (stats por jugador, sin hash); la suma de equipo es `FUN_1408C8FC0(EQ,0,5)` = CAT_0..7. Única copia distinta posible: singleton `MatchOutputStats` (DAT_1437F9C98, 0x10B600 B).
- **Denuvo 17.0 [Steam]** en PES2021.exe (DiE 3.21): `.impdata` 380 MB RWX; código real `.trace` 37 MB, sha256 `cff7b2d9…9def`. Proyectos: PEStrim (12.1.4, 192.244 funciones, 91,4 % de .trace) y PES_MCP_RECORTADO (12.0.4 + GhidraMCP, puerto 8765).
- **Cierres al salir de PES:** `+0x3FD68B` (28-oct 16:45, escritura en 0x31) y `+0x3FE920` (29-oct 01:35); volcados en CrashDumps; sin analizar.
| Protector copia la IAT a la tabla del juego (29-oct, C) | Stub en .text1: lee IAT real (.tls$, ej. WinHttpConnect 0x1b9618b8) y escribe la tabla en .rdata (0x14252fe10) una sola vez. Un hook por valor que cambie ambas casillas cubre cualquier orden de carga |
| Override nativo del servidor gate/stun (29-oct, C/P) | FUN_1411725c0/141172690 → FUN_1411726c0 (tabla 6 entradas 0x1434d62e0) → FUN_1411727f0 «¿existe archivo X en la carpeta del juego?». Hosts dev: ligarloose/pele/resic06/zico.konami. Posible vía OFICIAL sin hooks; sin leer valores en memoria |
| 4 IP fijas de Konami en el exe (29-oct, C) | 52.199.19.186, 18.176.195.37, 54.178.167.19, 18.182.217.155 (0x1428af308‥) con etiquetas Msp/Dlcp/Npmp/Lltcp. No pasan por DNS |
| Máquina de login ConnectAccount (29-oct, C) | fn 140fdb320: AuthSteam→Challengecode→LoginAccount→SendPsid→GetPlayerData→(crear perfil | SetCurrentPlayer→LoginMenu). `result`≠NOERR en cualquier paso = error con código propio |
| Steam en PES2021 (29-oct, C/P) | SteamUser021 + CCallResult<EncryptedAppTicketResponse_t> (P: RequestEncryptedAppTicket); np_ticket/np_ticket_size son de compras. El ticket no es verificable sin la clave de Konami |
| CMD_AUTH_STEAM es HTTP (29-oct, C) | Base onlinesystem::CommandObjectHttp (fn 141172a40), vftable 0x1428ae1c0. Manda ticket = ISteamUser::GetAuthSessionTicket (1024 B, hex) + SteamID64 en claro (GetSteamID) + una cadena de tabla. Verificable fuera de Konami (Web API/firma Steam) |
| CmdLoginAccount: respuestas (29-oct, C) | LOBBYUSERMAX = probar siguiente servidor de la lista; LOGIN_FAILED/DIFFERENT_PATCHVERSION/SERVERNOTREADY = fatales con pantalla propia |
| Crear perfil: bucle (29-oct, C) | ConnectAccount2: CreatePlayer→…→GetPlayerData; si has_data=0 vuelve a crear (causa de «latidos hasta caer» en PESBUL) |
| Archivos marcador de QA para cambiar el gate (29-oct, C) | En la carpeta del juego: QACheckGate.bin, QCGate.bin, RatingCheckGate.bin, EnzymeCheckGate.bin, ReviewCodeGate.bin, OnlineModeCheck.bin → gate/stun pasan a IP fijas de Konami QA (AWS) o «pele.konami». Destino fijo, no editable por contenido |
| Comandos TURN del cliente (29-oct, C) | SendIpandportNtl (IP/puerto local+global + tipo NAT), SendTurnAddressData (u32 + hasta 20 B de dirección TURN), WatchTurnAddressData (la del rival); módulo «NETCLIENT» |
| Respuesta CHECK_LOCAL_GAMERELAY_PLAYER (29-oct, C) | claves is_local_gamerelay_player, gamerelay_register_url_data [{type,url}], build_gamerelay_num_for_1vs1/3vs3/11vs11 |
