# Sync y PC del amigo: análisis de escenarios (beta 1 · 0.2.45)

12 oct 2026. Pregunta de Fralex: «¿cómo me aseguro de que a mi amigo le funcionará bien el Sync?». Aquí van los escenarios, qué pasa hoy, qué se arregló y **cómo se probó**.

**Estados:** ✅ cubierto y probado (prueba automática en carpetas falsas) · 🟡 cubierto, sin probar (por código o por diseño) · 🔴 pendiente en la PC real del amigo.
**Pruebas:** `pruebas-link\correr_escenarios.bat` (54 comprobaciones, 0 fallos, usa el mismo código de Link y juegos FALSOS) y las pruebas de los `.bat` hechas a mano en carpetas temporales. El juego real de Fralex solo se **leyó**.
Regla general: **o todo o nada** (se comprueba todo antes de escribir; si algo falla a medias, los `sider.ini` ya escritos vuelven a su copia) y **Phoenix-DB solo con base de parche reconocida**.

## A. Juego del amigo

| # | Escenario | Qué pasa | Arreglo / cómo queda | Estado y prueba |
|---|---|---|---|---|
| A1 | Ruta con espacios, acentos, ñ, OneDrive, otro disco | Funciona | Prueba E01; el informe avisa si está en OneDrive o en Archivos de programa | ✅ E01 (ruta `OneDrive - … ñandú\Juegos é\…`) y `.bat` con acentos |
| A2 | Carpeta protegida (Archivos de programa) o de solo lectura | Escribir falla | Mensaje: «¿PES abierto, antivirus, disco lleno o solo lectura? Prueba como administrador»; el `.bat` rechaza un `sider.ini` de solo lectura sin pisarlo; todo vuelve atrás | ✅ E09 (solo lectura, vuelta atrás) y `.bat` solo lectura · 🔴 Archivos de programa real |
| A3 | PES sin Sider (no hay `SiderAddons\sider.ini`) | Se niega, no crea nada | Mensaje claro; el informe lo explica («ese juego no usa Sider») | ✅ E02 |
| A4 | Sider 7.3.3 (sin `goal_scored`) | Marcador sí; goleador no | El informe lee la versión de `sider.log` y lo explica | 🟡 (lectura de versión sin probar con un log antiguo) · 🔴 en su PC |
| A5 | PES vanilla / otro parche sin cambiador, sin base | Instala módulos; no pone Phoenix-DB | Mensaje «Phoenix-DB no se puso… es normal» | ✅ E07 |
| A6 | Parche 26, 27 o ambos, con cambiador, varios modos, modo activo ≠ modo instalado | Se instala en la raíz y en cada modo; Phoenix-DB solo donde hay base | Estado «instalado» aunque falte base; reconoce modos 26 (propios) y 27 (raíz) | ✅ E06, E13 |
| A7 | `sider.ini` vacío o sin lista de módulos | Se niega, sin tocar | Mensaje «no tiene lista de módulos» | ✅ E03 |
| A8 | `sider.ini` con BOM / LF / CRLF | Se respeta el formato | **Bug corregido:** con BOM, una línea insertada arriba dejaba el BOM a mitad del archivo | ✅ E04 |
| A9 | Líneas de Phoenix duplicadas, comentadas, en otra posición, editadas a mano | Queda **una** línea; la comentada se enciende | **Arreglado:** antes se dejaban duplicadas | ✅ E05 |
| A10 | Falta la carpeta `modules` en un modo | Se niega, nada cambia | Mensaje con el modo | ✅ E10 |
| A11 | Nombre de módulo raro (`..\x.lua`) | Se niega | — | ✅ E14 |

## B. Estado del juego y transacciones

| # | Escenario | Qué pasa | Arreglo | Estado y prueba |
|---|---|---|---|---|
| B1 | PES/Sider abierto al instalar | Se niega con mensaje | Link y los dos `.bat` | ✅ E08 (proceso falso) y `.bat` |
| B2 | PES se abre justo después | Sider lee los módulos al arrancar | Nada que hacer: arranca ya con ellos | 🟡 |
| B3 | Antivirus/Windows bloquea o archivo en uso | Falla sin romper | `.bat`: mensaje claro y nada cambia; Link: mensaje + vuelta atrás | ✅ E09 (en uso) y `.bat` con archivo bloqueado |
| B4 | Disco lleno | La copia o la escritura falla | Mensaje «¿disco lleno?» y vuelta atrás | 🔴 (no se puede simular con seguridad) |
| B5 | Actualización del parche borra las líneas | Estado «perdido» → REINSTALAR | Tarjeta «Qué funciona en tu juego» lo dice con el paso | ✅ E12 |
| B6 | Instalar dos veces | Sin cambios, sin copias nuevas | — | ✅ E05, E11 y `.bat` |
| B7 | Desinstalar y reinstalar | `sider.ini` vuelve idéntico al original | Los `.lua` se apartan, no se borran | ✅ E11 y `.bat` |
| B8 | Volver a una versión anterior | Copias con sello `sider.ini.phoenix-…` y `.lua.antes-v…` | Se comprueba que existe una copia idéntica al original | ✅ E11 |
| B9 | Fallo a medias en varios modos | Vuelta atrás de los ya escritos | — | ✅ E09 · `.bat` 🟡 (probado el aviso; la restauración por código, no forzada) |

## C. Datos (fichajes / Phoenix-DB)

| # | Escenario | Qué pasa | Arreglo | Estado y prueba |
|---|---|---|---|---|
| C1 | Base no reconocida (parche 27, Clásicos…) | Phoenix-DB **no** se pone ni se crea su carpeta | Mensaje explicativo («no es un error»); Sync › «Fichajes: desactivados en este parche (próximamente)» | ✅ E06, E07 |
| C2 | Phoenix-DB ya puesta sobre un 27 (instalación vieja) | Taparía jugadores | Advertencia «desinstala y reinstala» | ✅ E13 |
| C3 | Player.bin / PlayerAssignment.bin de otra versión | Solo llegan si hay Phoenix-DB; sin base no hay dónde | El repartidor de datos nunca crea carpetas del juego | 🟡 (pruebas previas de `entrega_test`) |
| C4 | Orden de las raíces `cpk.root` | Phoenix-DB va justo antes de la base | El Doctor avisa si queda detrás | ✅ E04/E05 (posición) |
| C5 | Conflicto con otro módulo (DR_KitMixer, kserv, Chatty…) | Phoenix no toca esos módulos; solo añade sus líneas debajo del último | Los demás módulos quedan intactos | ✅ E05 (los demás intactos) · 🔴 conflicto real en partido |
| C6 | Límite de módulos de Sider | Se añaden solo 2 líneas | — | 🔴 |

## D. Red y cuenta (Link ↔ web)

| # | Escenario | Qué pasa | Estado |
|---|---|---|---|
| D1 | Sin internet / web caída / proxy o VPN | Link muestra «SIN CONEXIÓN» y reintenta con calma; el juego y la sala local siguen | 🟡 por diseño (sin probar en esta sesión) |
| D2 | Ruta nueva aún sin desplegar (404) | Link espera 5 min antes de reintentar | 🟡 (según REGISTRO-LINK) |
| D3 | Token caducado / Parsec no vinculado / reloj mal / sin cuenta web | Se ve en CUENTA WEB y en Diagnóstico; la sala funciona sin cuenta web | 🔴 |
| D4 | Sala privada vs pública | Ajuste de visibilidad existente | 🔴 |

## E. Link del amigo

| # | Escenario | Qué pasa | Arreglo | Estado |
|---|---|---|---|---|
| E1 | Sin ViGEmBus | Nadie puede jugar | El instalador lo instala si falta; Diagnóstico lo marca en rojo | ✅ (rama «ya instalado» del instalador, probada) · 🔴 instalación real |
| E2 | Sin WebView2 | Interfaz ImGui de respaldo | El instalador lo instala (instalador oficial de Microsoft) | 🟡 · 🔴 |
| E3 | Sin VC++ runtime | — | Se compila en su PC con Visual Studio, que ya lo trae | 🟡 |
| E4 | Windows 10 vs 11, antivirus, SmartScreen | — | Se compila en su PC: sin marca de «descargado»; si un antivirus borra el `.exe`, el instalador no podrá corregirlo | 🔴 |
| E5 | Dos copias de Link abiertas | Se pelean por mandos y sala | El informe lo detecta y lo explica. **No hay bloqueo automático** (no lo añadí: tocar el arranque es arriesgado) | 🟡 detección · bloqueo pendiente |
| E6 | Configuración de una versión vieja | Faltan claves → valores por defecto; archivo dañado → copia `.bak` | Guardado atómico + `.bak` | 🟡 (código; sin prueba automática) |
| E7 | Ruta de configuración con acentos/espacios, usuario sin permisos en AppData | Diagnóstico «Carpeta de configuración» en rojo | — | 🟡 |
| E8 | Resolución/escala pequeña, idioma | Interfaz responsive; ES/EN | — | 🟡 (se vio en el simulador) |
| E9 | Instalar sobre una instalación existente | Se actualiza, no toca ajustes | — | ✅ (mensaje de actualización y copia sobre carpeta existente) |

## F. Overlay

| # | Escenario | Qué pasa | Estado |
|---|---|---|---|
| F1 | PhoenixGlass vs Smash Glass viejo | El instalador baja **PhoenixGlass.exe** (sha256) y no toca un `SmashGlass.exe` existente; Link prefiere PhoenixGlass | ✅ descarga con sha bueno/malo/404 · 🔴 hasta publicar el archivo en GitHub |
| F2 | Sin WebView2 | El overlay no abre; Link sigue | 🟡 |
| F3 | PES a pantalla completa | Solo se ve el aviso dentro del juego (panel de Sider) | 🔴 |
| F4 | Puerto del WebSocket ocupado | Diagnóstico › Overlay lo marca | 🔴 |

## G. Diagnóstico («qué pasa» + «qué hacer»)

**COPIAR INFORME** (Sala › Conexión › Diagnóstico del PC) ahora incluye una sección «Qué pasa y qué hacer (en claro)» con un aviso por cada caso detectable: juego no encontrado, sin Sider, PES abierto, Archivos de programa u OneDrive, `sider.ini`/`modules` de solo lectura, versión de Sider, módulos perdidos/viejos/sin instalar/no cargados, Phoenix-DB no puesto (explicado como normal), advertencias de parche y varias copias de Link. Tapa tu nombre de usuario de Windows en las rutas y no lleva contraseñas ni claves. Estado: 🟡 (compila; se vio el texto en el simulador; falta verlo en Link real).

## Lista corta para la primera sesión con el amigo
1. Antes de nada: Sala › Conexión › Diagnóstico del PC en verde (WebView2, ViGEmBus, Parsec).
2. SYNC › «Qué funciona en tu juego»: juego conocido, módulos al día. Si algo está en ámbar/rojo, **COPIAR INFORME**.
3. Con PES **cerrado**, INSTALAR ambos módulos → «INSTALADO». Abrir PES y mirar `sider.log`: `[phoenix.lua]` sin «is NOT activated», y la versión de Sider (7.3.4).
4. Fichajes: en parche 27 deben salir «desactivados (próximamente)»; en parche 26, activos.
5. Probar a mano en su PC: carpeta con acentos/espacios, OneDrive, cerrar Link y abrirlo dos veces, desinstalar y reinstalar.
6. Anotar y mandar el informe si algo falla.

## Qué NO hice (y por qué)
- No bloqueé la segunda copia de Link (riesgo en el arranque); queda detectada y explicada.
- No simulé disco lleno ni antivirus: peligroso en una PC con datos.
- No toqué el juego real de Fralex.
