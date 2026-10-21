# Capítulo 60-j — ¿Más de 8 mandos en PES 2021? ¿Existió el 11 vs 11? (19-oct-2026)

> Pedido de Fralex: «haz lo imposible para ver si hay una remota posibilidad de que se pueda agregar más de 8 mandos», y verificar si existió una opción de 11 vs 11. Trabajo hecho **solo** con Ghidra (proyecto completo `copias/ghidra_proyecto_completo`, en modo solo lectura), cadenas del exe, `Settings.exe` y búsqueda web. **No se tocó el juego instalado ni se pidió ninguna prueba en vivo.**

## 0. Respuesta corta (SÍ/NO primero)

| Pregunta | Respuesta | Certeza |
|---|---|---|
| ¿Existió el 11 vs 11 con humanos? | **SÍ, pero solo ONLINE**: el «Team Play Lobby» (cada persona maneja **un** futbolista, hasta 22 usuarios). Llegó con el data pack de PES 2014 y su código **sigue dentro del exe de PES 2021**. Con los servidores de Konami cerrados (agosto 2022) **hoy no funciona**. Nunca fue local (en un mismo PC). | Confirmado (fuentes web + cadenas del exe) |
| ¿Hoy se puede jugar en PES 2021 con más de 8 mandos en un mismo partido local? | **NO**, tal como viene. El tope de 8 está en `settings.dat` (8 casillas fijas) y en la herramienta `Settings.exe` (`HID_KIND_PAD_1…8`). | Confirmado |
| ¿Hay una posibilidad remota de subirlo? | **SÍ, remota pero real**: el motor de entradas del juego está preparado para **32 mandos**, no para 8, y el motor del partido maneja a 22 jugadores con control humano (lo usa el 11 vs 11 online). El 8 vive en capas de arriba (configuración del PC, menú de lados y quizás la lista de usuarios locales del partido), que no he localizado del todo. | Hallazgo confirmado en Ghidra; la viabilidad completa es hipótesis |
| ¿Es fácil? | **NO.** Sería el trabajo de ingeniería inversa más grande del proyecto (semanas), con partes que hoy no sabemos dónde están. | Estimación |

## 1. Dónde vive el límite de 8 (evidencia)

### 1.1 Capa de configuración del PC: `settings.dat` y `Settings.exe` — AQUÍ está el 8
- `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\settings.dat` mide **612 bytes**: desde `0x64` hay **8 casillas de 64 bytes** (GUID DirectInput + mapa de botones). Un mando que no está en una casilla, PES lo ignora (ya descubierto por Link: `docs/ORDEN-MANDOS-PES.md` §2). `0x64 + 8×0x40 = 0x264 = 612`: el archivo no tiene sitio para una 9.ª casilla.
- `Settings.exe` (.NET, `D:\Frank\Games_\Conmegol Patch\Settings.exe`, solo leído) define el enumerado `HID_KIND_PAD_1` … `HID_KIND_PAD_8`, `HID_KIND_PAD_NUM`, `HID_KIND_MAX`, y funciones `GetJoystickDeviceList`, `controller_player_index`, `IsXinputUse/SetXinputUse` (casilla DirectInput/XInput).
- **No localizado:** el código del exe que lee `settings.dat` (el nombre no aparece en claro ni en el exe ni en `copias/volcado_memoria_PES2021.bin`; probablemente se arma en tiempo de ejecución o está en la parte protegida `.impdata`). Es la pieza clave que falta para saber si el 8 está también en el juego o solo en el archivo.

### 1.2 Motor de entradas del juego (`cobra::hid`) — NO tiene el 8: admite 32
| Qué | Dirección | Lo que muestra |
|---|---|---|
| Tabla de capacidades de mandos | `.data 0x14351A840` = `01 00 20 00 01 00 01 00 00…` | `+2 = 0x20` → **32 mandos**, `+4 = 1` teclado, `+6 = 1` ratón. La entrega `FUN_141B5C770` y la copia la fábrica `cobra::hid::impl::_anon_9B2E8B15::Factory` (vftable `0x142AC7198`, método +8 `FUN_141B58030`). |
| Gestor `cobra::hid::impl::_anon_FF75708B::Man` | ctor `FUN_141B58A80`, global `0x1437F0AE0`; arranque `FUN_141B593A0` | Crea las listas de mandos/teclados/ratones con el número que da la tabla (no con un 8 fijo). |
| Gestor `cobra::game::impl::_anon_4FDCE928::HidMan` | global `0x1437F0A58`; ctor `FUN_141B51400`; arranque `FUN_141B54560` | Las listas son **vectores dinámicos** (`FUN_141B54950` las redimensiona al número de la tabla; entradas de 0x68 bytes). |
| Máscara de mandos conectados | `FUN_141B52380` → `HidMan` vftable +0x68 = `FUN_141B52340` (lee `HidMan+0xC8`) | Es un entero de **32 bits** (un bit por mando). `FUN_140AD9E90` (menú) la recorre hasta `0x20` bits. |
| Ranuras físicas de la fábrica | `FUN_141B585C0` | Bitmap en `+0x1860` y contador en `+0x1884`: reparte mandos en ranuras libres, sin 8 fijo. |
| Traductores | `cobra::me::hid::impl::di8::*` (DirectInput 8) y `xb2::*` (XInput) | DirectInput no tiene tope práctico; **XInput solo maneja 4 mandos por diseño de Microsoft** (XUSER_MAX_COUNT = 4). Los mandos virtuales de Link entran por DirectInput (GUID en `settings.dat`), así que el 4 de XInput no nos limita. |
| Mapas de botones por tipo de mando | `FUN_141B54000` (cadena `XInput`, perfiles `@auto`, IDs `0x268054C` DualShock 3 (corregido: no es DS4) y `0x28E045E` Xbox 360) | Traduce botones; no limita la cantidad. |

### 1.3 Menús (pantalla de elegir lado)
- Clases `menu::SideSelect`, `menu::SideSelectEach` (vftables `0x142715608` y `0x142715860`), textos `Common/CmnCoopSideSelect`, `SideSelectChangeSidePd` (`FUN_140ABF490`), diseño `cpk_dat/common/menu/general/sideSelect.bin`.
- `FUN_140AD9E90` muestra el aviso **`numControllers_4`** cuando hay elementos en una lista; por el nombre, es un aviso de «número de mandos» (no sé aún qué texto exacto muestra).
- Para más de 8 habría que rehacer el diseño de esa pantalla (archivo de menú dentro de los CPK) y las cuentas que la llenan. Sin localizar dónde se fija «8 iconos».

### 1.4 Partido: el motor maneja 22 jugadores con control humano
- El 11 vs 11 online (Team Play) existió y su código está en el exe (ver §2): el partido sabe dar un humano a cada uno de los 22.
- Tabla de **24 ranuras** de 0x74 bytes en `[0x1436F3DC8]+0x180` (el gestor de actores ya conocido); cada ranura guarda en `+0x20` el índice de un jugador `0…21` (0–10 local, 11–21 visitante). La busca `FUN_14043B870` (35 llamadores) y la usa `match::CursorManager` (vftable `0x14259BD90`, ctor `FUN_1404360B0`, global `0x1436F3FA0`, etiqueta `CURSOR_INFO`). **Hipótesis:** son las ranuras de control/cursor por jugador; 24 ≥ 22 encaja con «un humano por futbolista». Falta confirmar si son humanos o solo actores.
- `match::registry::PadInput` y las 60 clases `match::pad::ThinkUnit*` (incluida `ThinkUnitPadId`) son la lógica que convierte botones en acciones; no se vio un 8 fijo en ellas (no se revisaron todas).

## 2. ¿Existió el 11 vs 11? — SÍ (online, «Team Play Lobby»)
- **PES 2014** lo añadió en un data pack: modo 11 vs 11 online y lobby «Team Play» ([GameSided](https://gamesided.com/2013/11/18/pes-2014-adds-11-11-online-play-free-companion-app/)).
- **PES 2019**: el manual oficial habla del Team Play Lobby con hasta 22 usuarios y la ficha oficial dice «1–22 jugadores online» ([manual PES 2019](https://dds.konami.com/games/manual/pes2019/PS4/en-us/menu.html), [ficha PES 2019](https://www.konami.com/wepes/2019/us/en-us/page/spec)). Cada usuario maneja un futbolista (importado de «Ser una Leyenda»).
- **PES 2021**: el exe tiene el lobby `MOMLobby` con `MOMLobbyMenuSideSelect`, `MOMLobbyMenuChangePositionRating`, `MOMLobbyBL…` (BL = Ser una Leyenda), `multilobby`, `freelobby`, `match_teamLobby`, `c_teamLobby`, `numUsers_user/numUsers_com` (`FUN_140DA0BA0`) y `ERR_LOBBYUSERMAX`. Los servidores se cerraron en agosto de 2022.
- **Intento de revivirlo:** el servidor privado **PESBUL** ya juega el amistoso 1 vs 1; en Team Play reconoce salas, el «Listo» y la cuenta atrás y conecta al «GameRelay» cifrado, pero **todavía no llega a elegir lados ni arranca el 11 vs 11** ([hilo PESBUL en Evo-Web](https://evoweb.uk/threads/pes-2021-private-server-online-emulator-open-beta-test-server.106068/); otro intento abandonado: [Evo-Web](https://evoweb.uk/threads/building-pes-2021-team-play-lobby-online-server.100632/)).
- **Ojo:** ese 11 vs 11 es **un PC/consola por persona**, cada uno con su copia de PES. No es «22 mandos en un PC».

## 3. Qué se rompería al subir el tope local por encima de 8
1. `settings.dat` (8 casillas fijas) y quien lo lee (no localizado) → habría que ampliar el formato o inyectar casillas extra en memoria.
2. Pantalla de lados (`SideSelect`) → diseño y lógica para más iconos.
3. Lista de usuarios locales del partido (dueño de cada mando, «datos personales» de cada uno, el límite de 32 datos personales ya investigado aparte) → probablemente tamaños fijos; no medido.
4. Indicadores sobre los jugadores (cursores de colores) → el juego trae pocos colores; con >8 habría repetidos.
5. Lo que sí parece aguantar: el motor de entradas (32) y el partido (22 controlados).

## 4. Caminos posibles (de más realista a más difícil)
| Camino | Qué da | Dificultad | Comentario |
|---|---|---|---|
| A. 8 humanos, cada uno **fijo en un futbolista** (estilo Ser una Leyenda) con Sider/Lua o con el modo de cursor fijo de PES | Partidos 4 vs 4 humanos + CPU, o 8 humanos en el campo | Media | Encaja con el N vs N y con Link (8 puestos). Recomendado primero. |
| B. **Varios mandos en una misma casilla** (Link ya mezcla mandos: «marionetas»/mapeo) | Más de 8 personas conectadas, pero compartiendo cursores | Baja | No es «más jugadores» de verdad; útil para turnos o relevos. |
| C. **Team Play online real**: un PC con PES por persona y un servidor privado (seguir/ayudar a PESBUL) | 11 vs 11 auténtico, el que existió | Muy alta (protocolo cifrado de Konami) | Es el único camino donde el juego ya sabe hacerlo todo. Phoenix podría alojar el servidor si PESBUL lo logra. |
| D. **Parchear el juego para >8 mandos locales** | 9–22 humanos en un PC/una sala de Link | Muy alta | Requiere encontrar el lector de `settings.dat`, el menú de lados y la lista de usuarios locales; con Sider (`memory`, `allocate_codecave`) se podrían ampliar en memoria. Hoy: posibilidad remota, sin garantía. |

**Recomendación:** hacer A ahora (sirve al objetivo N vs N) y, en paralelo y sin prisa, el siguiente paso de investigación de D: localizar en tiempo de ejecución quién lee `settings.dat` (por ejemplo, con un punto de ruptura en `CreateFileW` usando x64dbg o un gancho de Sider) — cuando Fralex vuelva a autorizar pruebas. Seguir de cerca a PESBUL para C.

## 5. Archivos de esta investigación
- Scripts nuevos: `scripts/ghidra/CadenasYUsos.java` (cadenas → usos; modos `fn:`, `callers:`, `sym:`, `vt:`), `scripts/ghidra/_lanzar_mandos.bat`, `scripts/ghidra/_lanzar_mandos_ref.bat`, listas `scripts/ghidra/mandos_cadenas_*.txt`.
- Salidas: `informes/datos/rea/mandos_1.txt … mandos_12.txt`, `mandos_3_gestor_hid.txt`, `mandos_6_driver.txt`.
- Revertir: borrar esos archivos; el proyecto de Ghidra se abrió en solo lectura y el juego no se tocó.

## Ampliaciones

### 19-oct-2026 ~18:10–18:30 — Búsqueda de 30 min: ENCONTRADO el lector de `settings.dat` (lo que pone el 8)
**SÍ/NO primero: SÍ encontré dónde el juego pone el 8; NO encontré todavía el último eslabón (quién reparte las casillas a los mandos).**
- **Truco que funcionó:** `settings.dat` empieza con la firma `WECF` (bytes `57 45 43 46`) y versión `0x6B`. Buscando esa firma en el exe aparecieron solo dos instrucciones.
- **`FUN_140408AF0` = lector de `settings.dat`** [CONFIRMADO]: lee **exactamente 0x264 bytes (612)** en la variable global `0x1436F0570`; si la firma no es `WECF` o la versión no es `0x6B`, pone valores por defecto. La llama el arranque de la app `FUN_140404D50`, que guarda el puntero en `app+0xB0` (`app = FUN_14149A470()`).
- **`FUN_140408B50` = valores por defecto** [CONFIRMADO]: rellena **9 casillas fijas** (bucle `< 9`): teclado + 8 mandos, de 0x40 bytes cada una (GUID de 16 bytes + 24 botones de 2 bytes), desde `+0x24`. Coincide con `Settings.exe` (REA: tipos `HID_KIND` = teclado + `PAD_1…PAD_8`, `SettingsData.m_hid`, `hid` = GUID + `rule[]`).
- **Justo detrás de la tabla hay otra variable** (`0x1436F07D8`, un texto de registro): la tabla **no puede crecer en su sitio**. Para más de 8 habría que reubicarla (memoria nueva con Sider) y corregir todo lo que lee `0x264`/`< 9`.
- **Capa DirectInput: 32 ranuras, sin 8** [CONFIRMADO]: `FUN_141B5D180` entrega «el mando N» desde el objeto DirectInput (`0x1437F0AF8`): acepta N `< 0x20` (32) en `objeto+0xE0`; si XInput está activo, las 4 primeras van por XInput. Los dispositivos se guardan por GUID (`FUN_141B5E1F0`) sin tope de 8.
- **ENCONTRADO el último eslabón: `FUN_140402660` (el «conector»)** [CONFIRMADO en Ghidra]: toma `settings.dat` desde `app+0xB0`, arma 9 mapas de botones (texto tipo `B0B14B1B12…` con las letras `BAXxPpMmVvHhUDLRK`) y luego **recorre exactamente 8 casillas** (`0x140402A82: mov esi, 8`; la rama alternativa usa `0x140402A40: cmp edi, 9` = 1…8). Para cada casilla `i`: `HidMan`+0xE8 (mapa de botones del mando lógico `i`), `HidMan`+0x120 (activa el mando `i`) y, si el GUID de la casilla (`settings+0x64+i×0x40`) no está vacío, `HidMan`+0x118 (activa el modo «por GUID») y **`HidMan`+0x110 = `FUN_141B52FC0` (asigna ese GUID a la entrada `i`)**.
- **Cómo engancha el mando físico:** `FUN_141B54F60` (cada fotograma) compara el GUID de cada mando que aparece con los GUID guardados en la tabla `HidMan+0xD0` (32 entradas de 0x20 bytes, GUID en `+0x10`) y los une. Esa tabla y el método `+0x110` aceptan índices hasta el tamaño de la tabla: **32**.
- **Conclusión actualizada:** el 8 está en **dos sitios concretos**: el formato del archivo (9 casillas, 612 bytes, `FUN_140408AF0`/`FUN_140408B50`) y el **bucle de 8 de `FUN_140402660`**. El gestor de mandos de abajo admite 32. **Posibilidad técnica para la capa de mandos: SÍ** — con Sider (gancho después de `FUN_140402660`) se podría llamar a `HidMan`+0xE8/+0x120/+0x118/+0x110 para las entradas 8…N con los GUID de los mandos extra (que vendrían de un archivo propio, no de `settings.dat`). **Sigue sin saberse** si la pantalla de lados y el partido aceptan un 9.º usuario local: eso es lo siguiente a investigar, y probablemente requiera prueba en vivo.
- **REA:** se analizó `Settings.exe` (instalado y copia; mismo SHA-256 `ea874e3e…7304`) con `inspect_managed_members` (evidencias `ev_81c9db60…`, `ev_1f8568ed…`, `ev_f69cabfe…`). `verify_reconstruction` **rechazó** las evidencias de inspección: solo acepta evidencias de **comparación** (`compare_managed_members`), que exigen pasar los dos registros completos (~317 KB cada uno). No se hizo por tamaño; queda pendiente si se quiere el sello de REA.
- Archivos: `informes/datos/rea/mandos_13…mandos_27.txt`, `mandos_16_settings.txt`; lanzador nuevo `scripts/ghidra/_lanzar_mandos_fn.bat`. Juego sin tocar; `settings.dat` solo leído.

### 19-oct-2026 ~18:30–18:45 — Menú de lados y partido
**SÍ/NO primero: el partido SÍ está hecho para más de 8 (hasta 24 mandos); el menú de lados NO (8 fijos).**
- **Partido — 24** [CONFIRMADO]: el registro de entradas `match::registry::PadInput` tiene un bloque de 0x7D4 bytes por mando y acepta índices `< 24`; las 24 ranuras de control (`[0x1436F3DC8]+0x180`) unen cada jugador (`+0x20`, 0–21) con un bloque de entrada (`+0x10`, < 24) (`FUN_1406A89F0`). Encaja con el 11 vs 11 online (22 humanos + 2).
- **Menú de lados — 8 fijos** [CONFIRMADO]: `menu::SideSelect` guarda 8 fichas en campos fijos (`+0x90…+0xC8`, `FUN_140ABF720` con `< 8`), y varias funciones del menú buscan «el primer mando» solo entre 8 (`FUN_140E43620`, `FUN_140E431F0`, `FUN_140E42F80`).
- **Mapa de dónde está el 8 (hoy):** (1) `settings.dat` 9 casillas; (2) conector `FUN_140402660` (8); (3) menú de lados (8 fichas). Debajo (gestor de mandos 32, DirectInput 32) y en el partido (24) hay sitio.
- **Lo que falta saber:** quién llena las 24 ranuras del partido a partir del resultado del menú de lados (si copia solo 8 o «todos los mandos activos»). Si copia la máscara de activos, un camino sería **saltarse el menú** (lados elegidos desde Link/Sider) y escribir las ranuras directamente.
- **Camino más realista para >8 (hipótesis de trabajo):** gancho Sider tras `FUN_140402660` para dar de alta mandos 9…N en `HidMan` (+0xE8/+0x120/+0x118/+0x110) + asignar sus lados sin el menú + verificar que el partido los usa. Requerirá pruebas en vivo cuando se autoricen.
- Hallazgos colaterales guardados en el nuevo **capítulo 62 (catálogo)**.

### 19-oct-2026 ~19:00 — ¿Clonar `settings.dat`? ¿Clonar las fichas de la pantalla de lados? ¿Cuántos se podría?
**SÍ/NO primero: SÍ es posible en teoría las dos cosas; con la interfaz del juego el techo realista es 10 mandos; sin usar esa pantalla (lados elegidos desde Link) el techo teórico es 22–24. Nada está probado en vivo.**

**1) «Clonar» `settings.dat` para mandos extra — SÍ (vía Sider)** [CONFIRMADO en código]
- `FUN_140408C50` abre `\KONAMI\eFootball PES 2021 SEASON UPDATE\settings.dat` (texto ancho en el exe) y hace `fread(buf, 1, 0x264)`: **solo exige que haya 612 bytes**; si el archivo es más largo, lo demás se ignora. Se pueden poner casillas extra (mismo formato de 0x40 bytes) al final del archivo o, más seguro (Settings.exe puede reescribirlo), en un archivo propio de Phoenix.
- Sider permite **FFI de LuaJIT** (`luajit.ext.enabled = 1` en `sider.ini`): desde Lua se pueden llamar funciones del juego por dirección. Un módulo leería las casillas extra y llamaría a `HidMan` (global `0x1437F0A58`): +0xE8 (mapa de botones del mando N), +0x120 (activar N), +0x118 y +0x110 (asignar el GUID de la casilla). `HidMan` acepta N hasta 31.
- **Pero eso solo «conecta» el mando.** El partido copia los mandos a sus entradas solo para los 8 primeros: `FUN_140A64810` (`0x140A64816: cmp edx, 8`), `FUN_140A64850` (`0x140A648AF: cmp edx, 8`; para N ≥ 8 deja la muestra a cero) y `FUN_140A647E0` (banderas, `0x140A647E0`). El registro de entradas (`FUN_140A646A0`) tiene **24 bloques** de 0x7D4 bytes (búfer de 100 fotogramas cada uno) pero solo **8 banderas** (`+0xBBE0`) y **8 configuraciones** de 0x12 bytes (`+0xBBE8`). El bucle del partido (`FUN_1408AA5C0`) ya recorre los 24. Hay que parchear esos `cmp 8` y dar banderas/configuración a los bloques 8+ (por ejemplo, pasar «sin configuración» = 0).

**2) Clonar las fichas de la pantalla de lados — SÍ en teoría, techo ~10** [CONFIRMADO en código + archivo]
- Cada mando tiene **su propia ventana** `menu::SideSelectEach` (objeto de 0xC0 bytes, carga `sideSelectWindow` de `common/menu/general/sideSelect.bin`, que está en `dt11_x64.cpk` comprimido con la cabecera `WESYS` + zlib; dentro es un Flash compilado `afp_sideSelect.apk`). **No hay 8 dibujos fijos**: hay una plantilla `sideSelectPad` y el código coloca cada ventana según su número (fotograma = número + 1) y le pone el color `padColor_<cifra>` (`FUN_140C67BE0`).
- El lado elegido (izquierda/centro/derecha, < 3) se guarda con la clave `sideselect_<cifra>` (`FUN_140C67AF0`, usa `número % 10`): **solo 10 claves distintas (0–9)**. Con 11+ se mezclarían.
- El menú padre `menu::SideSelect` guarda 8 punteros fijos (`+0x90…+0xC8`) y varias funciones recorren solo 8 (`0x140ABF758: cmp edx, 8` y los tres de §«Menú de lados»). Habría que reservar más sitio (memoria nueva con Sider) y cambiar esos 8.
- No se sabe aún si la animación Flash tiene posiciones/colores para los fotogramas 9 y 10: si no los tiene, las fichas 9–10 se dibujarían encima de otra (funcionarían, pero se verían mal). Editar ese Flash no tiene herramienta pública conocida.

**3) ¿Cuántos se podría?**
| Camino | Techo | Qué limita |
|---|---|---|
| Con la pantalla de lados del juego clonada | **9–10** | Claves `sideselect_<0–9>` y colores `padColor_<0–9>`; dibujo Flash de las posiciones 9–10 sin verificar. |
| Sin esa pantalla (Link/Sider decide el lado de cada mando extra) | **hasta 22** (24 entradas) | Registro de entradas del partido (24); 22 jugadores en el campo. Falta encontrar quién reparte las ranuras del partido a partir de los lados. |
| Capa de mandos | 32 | `HidMan` / DirectInput. |

**Piezas a construir (en orden):** (a) archivo de casillas extra + módulo Sider con FFI que registre los mandos 9…N en `HidMan`; (b) parches `cmp 8 → cmp N` en el llenado del partido + banderas para los bloques 8+; (c) asignar lado/jugador a los mandos extra (falta localizar quién llena las 24 ranuras del partido); (d) opcional: ampliar `SideSelect` a 9–10 fichas. Cada paso necesita prueba en vivo cuando Fralex la autorice. Riesgo: fallos o cierres del juego mientras se ajusta; todo reversible (Sider se desactiva quitando el módulo).
