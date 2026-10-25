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
