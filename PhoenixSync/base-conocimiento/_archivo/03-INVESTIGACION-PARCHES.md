# 03 · Lo que enseñan los demás parches (y cómo hacer el Phoenix el más completo y sin errores)

Investigación del 8 oct 2026: parches grandes de PES 2021, modo online, herramientas y formatos. Informes completos, con todas las
fuentes, en `investigacion/`:
- `parches-pes2021.md`: ficha de cada parche, las 14 causas de crash más reportadas con su solución y buenas prácticas.
- `modo-online.md`: servidores oficiales, PESBUL, alternativas, Sider para resultados, Parsec desde Perú y arquitectura recomendada.
- `herramientas-y-formatos.md`: cómo fabricar CPK, formatos de la base de datos, kits, caras, option file y memoria (Cheat Engine).

## A. El mapa de los parches en 2026

| Parche | Estilo | Lo que hay que copiar | Lo que hay que evitar |
|---|---|---|---|
| **Football Life** (SmokePatch; FL26 15/10/2025, FL27 a inicios de nov. 2026) | Juego «propio» sobre el motor: exe, lanzador, instalador y switcher propios | Lanzador que comprueba versiones; updates clasificadas según si rompen la carrera; ligas de 30 y Mundial de 48 **parcheando el exe** | Exe modificado sin online (PESBUL no funciona con FL) |
| **Evoweb Patch** (2024 v3.0) | Sider + base de datos | IDs **reales** de Konami para que cualquier facepack encaje | No convive con ninguna otra base de datos |
| **ConmeGOL** (27, sept. 2026) | Casi todo por Sider/livecpk; base en un `.cpk` de 3,7 MB | Base pequeña + contenido suelto (actualizar es copiar carpetas) | AIO de ~234 GB; `sider.ini` con módulos que faltan y pares que se pisan (ver `../sider/AUDITORIA-SIDER.md`) |
| **Sudamerican Project 2026** | «Clásico» de CPK: 34 CPK, 13 de caras de 5–11 GB | Créditos y comunidad (Discord, 30+ colaboradores) | CPK gigantes; módulos cargados dos veces; `netblock.lua` (bloquea el online) |
| **Gogosz** (World 12.5 / CONMEBOL 4.5, oct. 2026) | AIO enorme (~200 GB) | Cobertura de ligas | El tamaño |
| **PESBUL** (beta, jul. 2026) | Servidor de la comunidad que imita al de Konami | Que el online «de verdad» es posible | Solo amistoso 1 vs 1; exige el juego de Steam **sin modificar** |

**Hueco libre:** no existe un parche de PC dedicado a la **Liga 1 de Perú** y la zona andina; hoy solo aparece dentro de ConmeGOL. Ahí entra Phoenix.

## B. Las causas de crash que más se repiten (y la regla Phoenix para cada una)

| Causa | Regla Phoenix |
|---|---|
| Copias repack/modificadas, restos de otros parches, DpFileList para el DLC equivocado | Base única declarada: **PES 2021 de Steam, exe 1.07.02 + Data Pack 7.0**; el lanzador lo comprueba y no arranca si no coincide |
| Mezclar bases de datos (jugadores «sin nombre, media 40») | **Una sola** base; Phoenix Sync escribe solo en la de Phoenix y detecta otras |
| Option file de otra versión | El EDIT lleva su versión; `save.folder` de Sider 7.4.1 para que Phoenix tenga **su propio** EDIT |
| Falta memoria virtual (pantalla negra con muchas caras) | El lanzador revisa el archivo de paginación y avisa; caras en DDS comprimido |
| Módulos Sider viejos, duplicados o que se pisan | **Un solo `sider.ini` limpio**, auditado; Sider **fijado** (7.4.1) y distribuido con el parche |
| Intel 12.ª gen+ (DRM), drivers, antivirus, OneDrive | Lista de comprobación en el lanzador; hashes publicados; nada de `.bat` (lanzador `.exe`) |
| Cambiar el número de equipos de una liga (Evoweb: la Liga Máster se colgaba en la temporada 2) | **No cambiar** el número de equipos de una liga sin parchear el exe; el calendario está dentro del exe |
| Audio/mods de narración incompatibles | Addons con versión; se instalan de a uno y el lanzador comprueba la versión |

Límites documentados de la base de datos (fuente de foro): **750 equipos, 40 jugadores por equipo, 30.000 jugadores**.
(Cuadra con lo visto: 749 equipos en ConmeGOL y 29.997 jugadores en `Player.bin`.)

## C. Modo online: lo realista

- Los servidores oficiales de Konami están **cerrados** (2022). PES 2021 de PC **no tiene modo LAN**: Radmin/ZeroTier/Hamachi solos no sirven.
- **PESBUL**: rehace el lobby de Konami (amistoso 1 vs 1). El partido va directo entre los dos PC. **Exige el juego de Steam sin
  modificar, sin Sider ni mods de jugabilidad**, y abrir los puertos UDP 5739–5740. Las dos carpetas de juego de la PC de Fralex
  no cumplen esas condiciones: llevan Sider con muchos módulos y archivos que no son de la instalación original de Steam
  (`CPY.ini`, `steamclient64.dll`). Es la explicación más probable de que PESBUL no conectara. Su base de código abierto es
  el servidor de **Nikow5** (licencia MIT).
- **Antecedente bonito:** en PES 6 hubo un servidor propio unido a una web de ligas, de código abierto (`kinj1987/evo-league`). Es la
  idea de «Phoenix online» hecha hace años.

**Arquitectura recomendada para Phoenix online (se puede hacer ya):**
1. La **web** crea el partido (liga, rivales).
2. **Phoenix Link** (Parsec) en el PC anfitrión deja entrar solo a esos jugadores y abre el juego con el option file oficial.
3. Un módulo **Sider** lee el marcador con `match.stats()` (marcador, penales, periodo y reloj; **no** da goleadores ni tarjetas) y
   lo deja en un archivo `resultado.json` (el «buzón», al revés que `avisos.txt`).
4. Phoenix Link sube el resultado a la web con una captura automática; los dos jugadores lo confirman.
5. El verificador revisa solo al anfitrión (sus archivos y su option file).

Así no hay desincronización (solo hay un juego corriendo) y no se depende de Konami ni de PESBUL.
**Largo plazo (meses o años, investigación):** servidor propio a partir del de Nikow5; haría falta entender el protocolo y el login de Steam.

**Parsec desde Perú:** el anfitrión necesita ≥ 10 Mbps de subida (30 recomendados); ping bueno < 30 ms, aceptable < 60 ms; ajustes
para fútbol: 60 fps, 720p o 900p, H.265 si todos lo soportan, cable, V-Sync apagado, sin VPN. No funciona detrás de CGNAT (internet móvil).

## D. Herramientas y formatos que ya tenemos documentados para fabricar el parche

- **Crear CPK:** `the4chancup/pes-file-tools` (`cpk.py`, unas 250 líneas): archivos desde 0x800, cada uno alineado a 0x800, sin
  comprimir, ordenados por nombre en mayúsculas; tablas en big-endian cifradas con XOR (empieza en 0x5F, × 0x15 por byte).
  Fácil de pasar a C++ (sin licencia declarada: usar como referencia y escribir código propio).
- **DpFileList.bin:** filas de 48 B con el nombre del CPK en +0x10 (ya entendido en `parches/conmegol-26.md`).
- **WESYS** (envoltorio de los `.bin` de la base): cabecera de 16 B (3 bytes de versión, «WESYS», tamaño comprimido, tamaño original) + zlib.
- **Tamaños de registro** (`pes-db-generator`, GPL-3): Team 1532, Coach 100, PlayerAssignment 16, CompetitionEntry 12, PlayerAppearance 60.
  **Sin documentación pública:** Competition, CompetitionKind, CompetitionRegulation, Stadium, Derby, Tactics, Ball, Boots, Glove →
  hay que descifrarlos comparando antes/después (como hicimos con la Liga Máster).
- **Kits:** TeamColor.bin 16 B por equipo; UniColor.bin 85 B por equipo; UniformParameter documentado; kit config de 0x78 B (de `kitinfo.cpp` de Sider).
- **Caras:** `pes-file-tools` (.fpk/.ftex/.fsop), `pes-fmdl-blender` 0.7.2 (mayo 2026), GzsTool/FtexTool de Atvaark (MIT).
- **Option file:** el mapa de 4ccEditor (C++, licencia zlib) coincide **exactamente** con el nuestro (jugadores 0x7C × 0x138, equipos
  0x8ED2FC × 0x24C, plantillas 0x9D4648 × 0x11C, tácticas 0xA09880 × 0x274): confirma nuestro trabajo.
- **Liga Máster:** no existe ningún editor de guardados de código abierto ni el formato documentado en público: **lo nuestro
  (`01-MOTOR-PES2021.md`) es, hasta donde se encontró, lo más completo que hay.**
- **Memoria (para el puente en vivo):** tabla de Cheat Engine de xAranaktu (MIT): presupuesto de fichajes en `[rdi+0x016ECBF4]`,
  salarial en `+0x016ECC08`; jugador en RAM de ~0x17C B (sueldo +0x15C, valor +0x174, contrato +0x138).
- **Sider 7.4.1:** código en GitHub (`pes-modding/sider7`) — leer, no copiar (licencia). Novedad clave: `save.folder`.

## E. Receta del Phoenix Evolution Patch (propuesta)

1. **Base fija:** PES 2021 de Steam, exe 1.07.02 + DP 7.0, Sider 7.4.1 incluido, `save.folder` propio.
2. **Núcleo pequeño:** base de datos Phoenix en un `.cpk` chico (como ConmeGOL) + option file oficial publicado por la web (ya existe).
3. **Contenido por Sider/livecpk**, en addons con versión (caras, kits, estadios, narración), con **un `sider.ini` auditado** sin duplicados.
4. **Lanzador propio** (Phoenix Link): comprueba exe/DLC/Sider/addons, archivo de paginación, antivirus y OneDrive; muestra avisos de update.
5. **Mercado y Liga Máster** siempre sobre la base Phoenix, con copia de seguridad automática antes de cada escritura (ya lo hacemos).
6. **Online:** modelo Parsec + web + buzón Sider (sección C).
7. **Identidad:** Liga 1 y ligas andinas como punto fuerte; IDs reales de Konami para que los facepacks encajen.
8. **Pruebas antes de cada versión:** instalación limpia; arranque, amistoso, Liga Máster (temporada completa y mercado), copa,
   Ser una Leyenda, edit, option file y partido por Phoenix Link. Todo anotado en `PRUEBAS.md`.
