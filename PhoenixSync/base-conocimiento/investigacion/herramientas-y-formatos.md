# Herramientas y documentación para construir un parche de PES 2021 (C++)

Investigación hecha en octubre de 2026.

Cómo leer este informe:

- Cada sección tiene una **tabla corta** de herramientas y luego los **datos técnicos concretos** (offsets) que encontré.
- Marco con **[VERIFICADO EN CÓDIGO]** lo que leí yo mismo en el código fuente clonado de GitHub.
- Marco con **[SEGÚN PÁGINA]** lo que solo dice un blog o foro (no lo pude comprobar en código).
- Marco con **[NO ENCONTRADO]** lo que busqué y no existe en público (habrá que sacarlo comparando archivos a mano).

Palabras que se repiten:

- **Offset** = la posición de un dato dentro del archivo, contando bytes desde 0. Se escribe en hexadecimal (0x...).
- **Registro** = un "bloque" de tamaño fijo que se repite (por ejemplo, un equipo dentro de Team.bin).
- **Little-endian (LE)** = los números se guardan con el byte pequeño primero (así guarda PES casi todo).
- **Big-endian (BE)** = al revés (así guardan las tablas internas de los CPK).

---

## 0. Resumen de lo más útil (lo primero que deberías mirar)

| Qué necesitas | Mejor fuente | Por qué |
|---|---|---|
| **Escribir CPK** | `the4chancup/pes-file-tools` → `lib/pes_file_tools/cpk.py` (clase `CpkWriter`) | Unas 250 líneas, muy fácil de pasar a C++. Hace CPK que PES acepta. |
| **Comprimir/descomprimir WESYS** | mismo repo → `zlib.py` | Cabecera de 16 bytes + zlib normal. |
| **Generar Team.bin, Coach.bin, PlayerAssignment.bin, CompetitionEntry.bin** | `the4chancup/pes-db-generator` (Python, GPL-3) | Escribe esos archivos **desde cero para PES 21**. De ahí salen los tamaños de registro. |
| **Bloques del option file (EDIT)** | `the4chancup/4ccEditor` → `main.cpp`, `pes20.cpp` | Offsets exactos de jugadores, equipos, plantillas y tácticas en PES 21. |
| **Kits (UniformParameter, UniColor, TeamColor, kit config)** | `pes-file-tools/uniparam.py` + `4cc-aet-compiler-red/bins_update.py` + Sider `kitinfo.cpp` | Formatos de contenedor y de cada kit config. |
| **Memoria RAM (futuro puente Sider)** | `xAranaktu/PES-2021-Cheat-Table` (MIT) + Sider 7 (`pes-modding/sider7`) | Patrones AOB, offsets de jugador y presupuesto ML, y `match.stats()` para el marcador. |

---

## 1. Crear / reempaquetar CPK y DpFileList.bin

### 1.1 Herramientas

| Nombre | Autor | Enlace | Licencia | Lenguaje | Última actividad | Utilidad para C++ |
|---|---|---|---|---|---|---|
| **pes-file-tools** (cpk.py, CpkWriter) | the4chancup | https://github.com/the4chancup/pes-file-tools | Sin archivo de licencia (ojo: legalmente "todos los derechos reservados"; úsalo como referencia, reescribe tú el código) | Python | mayo 2024 | ★★★★★ La mejor referencia: escritor de CPK mínimo y pensado para PES |
| **4cc-aet-compiler-red** | the4chancup | https://github.com/the4chancup/4cc-aet-compiler-red | GPL-3 | Python | ago 2026 (activo) | ★★★★ Usa el CpkWriter de arriba en producción, lee DpFileList, actualiza bins |
| **CriCodecs / CriStudio** | Youjose | https://github.com/Youjose/PyCriCodecs | Sin licencia explícita | C++ + Python | sep 2026 (activo) | ★★★★ Tiene `cpk_builder.cpp`, `cpk_reader.cpp`, `crilayla.cpp` en C++ real. Comando: `cricodecs --build -f cpk carpeta -o archivo.cpk --alignment 2048` |
| **CpkPatcher** | MjTs140914 | https://github.com/MjTs140914/CpkPatcher | MIT | C# | jul 2023 | ★★★ Parchea un CPK sin reconstruirlo entero |
| **CriPakTools** (3 forks) | esperknight / uyjulian / wmltogether (base de Falo, Xentax) | github.com/esperknight/CriPakTools, /uyjulian/CriPakTools, /wmltogether/CriPakTools | Sin licencia | C# | 2015–2019 | ★★ Antiguo. El fork de wmltogether añade compresión CRILAYLA y arregla GTOC/ETOC |
| **cpktools** | kamikat | https://github.com/kamikat/cpktools | Sin licencia | Python | 2015 | ★ Solo histórico |
| **CPK File Builder / DpFileList Generator 1.0 (DLC 7.0)** | anónimo (post de G. Leroy en pesmodding) | https://www.pesmodding.com/2021/06/pes-2021-new-dpfilelist-generator-dlc-70.html | Cerrado | .NET 4.5 | 2021 | ★ Solo para comparar resultados. Admite **hasta 47 CPK** y actualiza `dlc.csv` [SEGÚN PÁGINA] |

### 1.2 Cómo es un CPK que PES 2021 acepta [VERIFICADO EN CÓDIGO: pes-file-tools/cpk.py]

Piensa en el CPK como **una caja con un índice**.

1. **Cabecera** en el byte 0: una tabla llamada `CPK ` (con espacio).
2. Los bytes `"(c)CRI"` terminan justo en **0x800** (es decir, empiezan en 0x7FA).
3. Los archivos empiezan en **0x800**. Cada archivo se rellena con ceros hasta un múltiplo de **0x800 (2048)** = el "Align".
4. Después de los archivos va el índice `TOC ` y, opcionalmente, `ETOC` (fechas).

Cada tabla tiene este envoltorio (little-endian):

```
4 bytes  nombre ("CPK ", "TOC ", "ETOC")
4 bytes  0
8 bytes  longitud del contenido (uint64)
...      contenido "@UTF" (cifrado con XOR)
```

El contenido `@UTF` va en **big-endian**:

```
0x00  "@UTF"
0x04  u32 bodyLength   (= longitud total - 8)
0x08  u32 rowsOffset   (relativo a +8)
0x0C  u32 stringsOffset(relativo a +8)
0x10  u32 dataOffset   (relativo a +8)
0x14  u32 tableName    (offset dentro de strings)
0x18  u16 columnCount
0x1A  u16 rowLength
0x1C  u32 rowCount
0x20  columnas: 1 byte flags + u32 offset del nombre
```

- `flags & 0x0F` = tipo: 0=int8, 2=int16, 4=int32, 6=int64, 8=float32, 10=string (4 bytes offset), 11=bytes (8 bytes: offset+longitud).
- `flags >> 4` = almacenamiento: 1=nulo, 3=constante, 5=por fila.
- Cifrado XOR del bloque: empieza con `m = 0x5F`, y en cada byte: `salida = byte ^ m; m = (m * 0x15) & 0xFF`.

Columnas del **TOC** que escribe el CpkWriter (orden importa poco, pero copia esto):
`DirName (string), FileName (string), FileSize (int32), ExtractSize (int32), FileOffset (int64, relativo a 0x800), ID (int32), UserString (string)`.

- Los archivos se ordenan por nombre **en mayúsculas**.
- `FileSize == ExtractSize` → archivo **sin comprimir**. PES 2021 no necesita compresión CRILAYLA; los .bin de pesdb ya vienen comprimidos con WESYS (ver sección 2).
- Nota del código: *"el offset real que usa libcpk parece fijo en 0x800 e ignora ContentOffset"*.

Campos de la **cabecera CPK** que pone (los demás en nulo):
`ContentOffset=0x800, ContentSize, TocOffset, TocSize, EtocOffset, EtocSize, EnabledPackedSize, EnabledDataSize, Files, Groups=0, Attrs=0, Version=7, Revision=14, Align=0x800, Sorted=1, EnableFileName=1, CpkMode=1, Tvers="pes-file-tools", Codec=0, DpkItoc=0, EnableTocCrc=0, EnableFileCrc=0, CrcMode=0`.

ETOC: columnas `UpdateDateTime (int64)` y `LocalDir (string)`; una fila por archivo **más una fila final vacía**. La fecha se codifica: año<<48 | mes<<40 | día<<32 | hora<<24 | min<<16 | seg<<8.

Si un archivo viene comprimido con CRILAYLA (empieza con `"CRILAYLA"`), `crilayla.py` lo descomprime (72 líneas, fácil de portar).

### 1.3 DpFileList.bin [VERIFICADO EN CÓDIGO: 4cc-aet-compiler-red/utils/dpfl_scan.py]

- Es la **lista de CPK que PES carga** desde la carpeta `download`.
- Está hecho de **filas de 48 bytes**.
- El **nombre del CPK** está en el **offset 0x10** de cada fila, terminado en byte 0.
- La lista **termina en la primera fila con el nombre vacío**.
- Los 16 primeros bytes de cada fila **[NO ENCONTRADO]**: ningún código público los documenta. Consejo práctico: lee el DpFileList original, conserva esos 16 bytes de una fila existente como plantilla y solo cambia el nombre.
- El generador público admite hasta 47 CPK [SEGÚN PÁGINA].
- El compilador 4cc busca archivos "de abajo hacia arriba" ordenando los nombres **en orden alfabético inverso**: el CPK con nombre "mayor" gana. Úsalo como regla para decidir prioridades.

---

## 2. Base de datos `common/etc/pesdb` y el envoltorio WESYS

### 2.1 WESYS (zlib de Konami) [VERIFICADO EN CÓDIGO: pes-file-tools/zlib.py y documentación de Sider]

Cabecera de **16 bytes**, luego datos zlib normales:

```
0x00  3 bytes  versión/magic  (pes-file-tools escribe 00 10 01; la doc de Sider dice 00 01 01)
0x03  5 bytes  "WESYS"
0x08  u32 LE   tamaño comprimido
0x0C  u32 LE   tamaño sin comprimir
0x10  ...      datos zlib (zlib.compress normal, con cabecera 78 xx)
```

- Para **detectar** WESYS, basta mirar si los bytes 4..7 son `"ESYS"` (así lo hace pes-file-tools).
- Si comprimir no ahorra espacio, pes-file-tools guarda el archivo **sin comprimir**; el juego acepta ambos.
- Sider tiene `zlib.pack()` / `zlib.unpack()` en Lua que hacen justo esto.

### 2.2 Herramientas de base de datos

| Nombre | Autor | Enlace | Licencia | Lenguaje | Actualidad | Qué documenta | Utilidad C++ |
|---|---|---|---|---|---|---|---|
| **pes-db-generator** | the4chancup | https://github.com/the4chancup/pes-db-generator | GPL-3 | Python | jun 2025 | Escribe Team, Coach, Player, PlayerAppearance, PlayerAssignment, CompetitionEntry para PES 15–21, y la lista de .bin vacíos que PES 21 necesita | ★★★★★ |
| **4ccEditor** | the4chancup | https://github.com/the4chancup/4ccEditor | zlib | C++ (Win32) | abr 2026 | Bloques del EDIT (ver sección 7). Los registros del EDIT son casi los mismos que los de pesdb | ★★★★★ |
| **PES 2021 Editor** | Ejogc327 | https://www.soccerfandom.com/2020/10/pes-2021-editor-by-ejogc327.html | Cerrado (versión de pago para EDIT) | .NET | 2020–2023 | Edita Team, Player, PlayerAppearance, PlayerAssignment, Competition, CompetitionRegulation, CompetitionEntry por CSV [SEGÚN PÁGINA] | ★★ (solo para comparar salidas) |
| **DinoTem Editor 21.2** | lagun-2 | https://www.welovepes.com/2020/03/dinotemeditor.html | Cerrado | ? | 2021 (tras Data Pack 2) | Player, Team, Stadium, Tactics, balones, botas, guantes, entrenadores [SEGÚN PÁGINA] | ★★ (para generar archivos de prueba y comparar) |
| **PESDatabase v0.2.1** | kisni07 | https://www.pesmodding.com/2026/07/pesdatabase-v021-efootball-and-pes.html | Cerrado | .NET 8 | jul 2026 | Player, PlayerAssignment, Team, Coach → CSV | ★★ |
| **Tutorial Ejogc** | BalkanPesBox | https://www.balkanpesbox.com/forum/topic/4066-pes-editor-by-ejogc-tutorials-for-database-edits/ | — | — | — | Límites y flujo de trabajo | ★★★ (reglas) |

### 2.3 Estructuras concretas para PES 2021 [VERIFICADO EN CÓDIGO: pes-db-generator]

Todo en little-endian. Los textos son UTF-8 rellenos con ceros.

**Team.bin → registro de 1532 bytes (0x5FC)**

(Ojo: el módulo de ejemplo `team_bin_reader.lua` de Sider usa 1468 bytes; ese tamaño es el de **PES 2019**. El generador da 1400 para PES 18, 1468 para PES 19 y **1532 para PES 20/21**.)

```
0x000  u32  ID del entrenador (Coach)
0x004  4    desconocido
0x008  u32  ID del equipo
0x00C  48   desconocido (en PES 19 eran solo 4)
0x03C  u16  ID del estadio
0x03E  u16  número de orden (0xFFFF)
0x040  u16  nacionalidad del equipo
0x042  4    desconocido
0x046  u16  "nacionalidad 2" (el generador pone 0x039C)
0x048  12   desconocido
0x054  u32  licencias de equipo y kit (el generador pone 0x0C)
0x058  70   nombre japonés
0x09E  70   nombre español
0x0E4  70   nombre sueco
0x12A  70   nombre griego
0x170  70   nombre inglés
0x1B6  70   nombre latino
0x1FC  70   nombre francés
0x242  70   nombre turco
0x288  70   vacío 1
0x2CE  70   nombre portugués
0x314  24   nombre interno de base de datos (abreviatura)
0x32C  70   nombre alemán
0x372  10   abreviatura con licencia
0x37C  70   nombre brasileño
0x3C2  70   nombre chino
0x408  70   vacío 2
0x44E  70   nombre italiano
0x494  70   vacío 3
0x4DA  70   nombre ruso
0x520  70   nombre neerlandés
0x566  10   abreviatura "falsa" (el generador escribe "None")
0x570  70   vacío 4
0x5B6  70   nombre inglés (EE. UU.)
0x5FC  fin
```

**Coach.bin → registro de 100 bytes (0x64)**

```
0x00  u32  ID del entrenador
0x04  u16  nacionalidad
0x06  2    relleno (PES 19–21; en PES 15–18 eran 10)
0x08  46   nombre japonés
0x36  46   nombre inglés
```

**PlayerAssignment.bin → registro de 16 bytes**

```
0x00  u32  índice de la asignación (1, 2, 3...)
0x04  u32  ID del jugador
0x08  u32  ID del equipo
0x0C  u32  orden en el equipo (empieza en 0 en PES 19–21; en 15–18 empezaba en 1)
```

(El generador escribe el orden como u32; es posible que en realidad sean campos más pequeños, como dorsal, dentro de esos 4 bytes. Compruébalo con un archivo real.)

**CompetitionEntry.bin → registro de 12 bytes**

```
0x00  u32  ID del equipo
0x04  2    desconocido
0x06  u16  ID de la entrada
0x08  u16  ID de la competición
0x0A  u16  orden dentro de la competición
```

**Player.bin**: tú ya lo tienes. El generador usa una plantilla de **312 bytes** para PES 20/21 (con 8 bytes de relleno antes del ID).

**PlayerAppearance.bin**: va en `common/character0/model/character/appearance/` (no en pesdb). Registro de **60 bytes**, que empieza con el u32 del ID del jugador.

**Archivos vacíos que PES 21 necesita** cuando generas una base de datos nueva (según el generador):
`MyclubCoach.bin, MyclubTactics.bin, MyclubTacticsFormation.bin, PlayerWeekly.bin, TeamWeekly.bin, CoachDeleteList.bin, Derby.bin, InstallVersionPlayer.bin, PlayerDeleteList.bin, SpecialPlayerAssignment.bin, SpecialPlayerAssignmentKind.bin, Tactics.bin, TacticsFormation.bin`.

### 2.4 Lo que NO está documentado en público [NO ENCONTRADO]

- `Competition.bin`, `CompetitionKind.bin`, `CompetitionRegulation.bin`, `Stadium.bin`, `Country.bin`, `Derby.bin`, `Tactics.bin`, `TacticsFormation.bin`, `Ball.bin`, `Boots.bin`, `Glove.bin`.
- Solo los editores cerrados (Ejogc, DinoTem) los entienden.
- **Método recomendado** para sacarlos tú:
  1. Descomprime el archivo WESYS.
  2. Divide el tamaño total entre el número de registros conocidos (por ejemplo, número de estadios) para obtener el tamaño de registro.
  3. Busca IDs conocidos (u16/u32) para fijar el inicio de cada registro.
  4. Cambia un valor con DinoTem o Ejogc y compara los bytes antes y después (un "diff").
  5. Anota cada prueba en tu `PRUEBAS.md`.

### 2.5 Avisos importantes

- **Archivo EDIT nuevo obligatorio**: si cambias IDs de equipos o jugadores, competiciones, entradas o reglamentos, hay que crear un `EDIT00000000` nuevo [SEGÚN PÁGINA: tutorial Ejogc].
- **Rareza de PES 19+** [VERIFICADO EN CÓDIGO: README de pes-db-generator]: cuando el juego genera un EDIT nuevo, **no incluye los jugadores**. Arreglo: poner el número de jugadores en el **offset 0x60** de `data.dat` (descifrado) y copiar los datos de jugador en el **offset 0x7C**. Sustituir bytes, nunca insertar.
- Añadir equipos a Team.bin puede hacer que el juego **no cree el EDIT** y tengas que hacerlo a mano [SEGÚN PÁGINA: Ejogc].
- En **PES 2015** poner Competition/CompetitionKind/CompetitionRegulation en un DLC hacía crashear el juego (dato del README del generador; no se sabe si pasa en PES 21; pruébalo).

---

## 3. Crear o ampliar competiciones

Lo que se sabe (poco, y casi todo de foros):

- **Límites de la base de datos** [SEGÚN PÁGINA: tutorial Ejogc]: Team.bin tiene **750 huecos de equipo**, **40 jugadores por equipo** como máximo; Player.bin tiene **30 000 huecos**.
- **El calendario de liga está fijo dentro del exe** [SEGÚN PÁGINA: EvoWeb Patch 2022 Update Vol. 2]. Si cambias el número de equipos de una liga (ellos tocaron la liga turca), **la Liga Máster crashea al pasar a la 2.ª temporada**. Su arreglo fue volver a **21 equipos**, el número original. Una partida ya empezada no se puede recuperar.
- Consecuencia práctica: **no cambies el número de equipos de una liga** que se use en ML. Sustituye equipos dentro de los huecos existentes.
- **No es posible crear una liga europea nueva desde cero**; lo normal es reutilizar una competición de Konami y cambiar nombre, logo y equipos [SEGÚN PÁGINA: hilo EvoWeb "Create a new league in PES 2021", sin confirmar].
- Para mover equipos entre ligas tras la 1.ª temporada, algunos usuarios lo hacen a mano porque el programa automático crashea [SEGÚN PÁGINA].
- Una "liga de 40 equipos" o "ampliación de ML" con parche de exe **[NO ENCONTRADO]**: no hay documentación pública.

Enlaces:
- https://www.pesmodding.com/2022/05/pes-2021-evoweb-patch-2022-update.html
- https://evoweb.uk/threads/create-a-new-league-in-pes-2021.86342/
- https://evoweb.uk/threads/master-league-crash-2-season-pes-2021.97263/

---

## 4. Kits (equipaciones)

### 4.1 Archivos y rutas [VERIFICADO EN CÓDIGO: 4cc-aet-compiler-red/FILE_INFO.py y bins_update.py]

- `common/etc/TeamColor.bin` → colores del equipo.
- `common/character0/model/character/uniform/team/UniColor.bin` → colores de cada kit (para el radar y los menús).
- `common/character0/model/character/uniform/team/UniformParameter.bin` → contenedor de todos los **kit config** (uno por kit).

### 4.2 TeamColor.bin

- **16 bytes por equipo**: 4 bytes iniciales (probablemente el ID) + hasta **4 colores RGB de 3 bytes**.
- El compilador 4cc calcula la posición como `(ID - 100) * 16 + 4`. Eso solo vale para su base de datos, donde los IDs son seguidos. En el juego original hay que **buscar el registro por ID**.

### 4.3 UniColor.bin

- **85 bytes por equipo**: 4 bytes iniciales (probablemente el ID) + 1 byte con el número de kits (máximo 10) + **10 entradas de 8 bytes**.
- Cada entrada de 8 bytes: `número de kit (jugador 0..; portero 16..)`, `icono (0–23; por defecto 3)`, `color1 RGB (3 bytes)`, `color2 RGB (3 bytes)`.
- Entrada vacía: `FF 00 00 00 00 00 00 00`.

### 4.4 UniformParameter.bin [VERIFICADO EN CÓDIGO: pes-file-tools/uniparam.py]

- Puede venir comprimido con WESYS.
- Cabecera: `u32 número de entradas`, `u32 offset de la tabla (= 8)`.
- Cada entrada (12 bytes): `u32 offset del contenido`, `u32 longitud`, `u32 offset del nombre`.
- Luego van los nombres (terminados en 0) y después los contenidos, cada uno **alineado a 16 bytes**.
- Las entradas se escriben **ordenadas por nombre**. Los nombres de kit config siguen el patrón `XXX_DEF_xxx_realUni.bin`.

### 4.5 Kit config (dentro de UniformParameter) → 0x78 bytes [VERIFICADO EN CÓDIGO: sider7/kitinfo.cpp]

```
0x00  ShortSleevesModel
0x01  ShirtModel
0x02  LongSleevesType (62 = normal + camiseta interior, 187 = solo camiseta interior)
0x03  ShortsModel (0–17)
0x04  ShirtColor1 (RGB)        0x07  ShirtColor2 (RGB)
0x0A  ShortsColor (RGB)        0x0D  SocksColor (RGB)
0x10  UndershirtColor (RGB)
0x13  desconocido (byte entero)
0x14  Collar                   0x15  WinterCollar (1–126)
0x16  palabra: ShortsNumberY bits 0-4, X bits 5-9, Size bits 10-14, Side bit 15
0x18  palabra: BackNumberY bits 0-4, Size 5-9, Spacing 10-13, ChestNumberY (bajo) 14-15
0x1A  palabra: ChestNumberY (alto) 0-2, ChestNumberX 3-7, ChestNumberSize 8-12, SleeveLimits 13-14, TightKit 15
0x1C  palabra: desconocido 0-1, BackNumberType 2, NameY 3-8, NameSize 9-13, NameShape 14-15
0x1E  palabra: Name bit 0, LeftShortX 1-5, LeftShortY 6-10, RightShortX 11-15
0x20  palabra: RightShortY 0-4, LeftLongX 5-9, LeftLongY 10-14, RightLongX (bajo) 15
0x22  palabra: RightLongX (alto) 0-3, RightLongY 4-8, desconocido 9-15
0x24  palabra: desconocido 0-4, ShirtPattern 5-7, NameStretch 8-9, desconocido 10-15
0x26  desconocido   0x27  desconocido
0x28  16 bytes  KitFile (texto)
0x38  16 bytes  BackNumbersFile
0x48  16 bytes  ChestNumbersFile
0x58  16 bytes  LegNumbersFile
0x68  16 bytes  NameFontFile
0x78  fin
```

("palabra" = 2 bytes leídos como u16 little-endian; los bits se cuentan desde el bit 0, el menos importante.)

### 4.6 Kit server (Sider)

- Sider trae `modules/kserv.lua` y la carpeta `content/kit-server/` con un `config.txt` de ejemplo.
- API Lua: `ctx.kits.get(team_id, kit_id)`, `ctx.kits.set(team_id, kit_id, kit_cfg, home_or_away)`, `ctx.kits.get_gk`, `ctx.kits.set_gk`, `ctx.kits.refresh`, `ctx.kits.get_current_team`, `ctx.kits.get_current_kit_id`. Evento `set_kits`.
- Los nombres de campos del `config.txt` son los mismos de la tabla 4.5.

---

## 5. Caras y pelo (.fpk / .fmdl / .ftex)

| Nombre | Autor | Enlace | Licencia | Lenguaje | Actualidad | Qué hace | Utilidad C++ |
|---|---|---|---|---|---|---|---|
| **pes-file-tools** (fpk.py, ftex.py, fsop.py) | the4chancup | (arriba) | sin licencia | Python | 2024 | Empaqueta/desempaqueta `.fpk`, convierte `.ftex` ↔ `.dds`, shaders `.fsop` | ★★★★★ (formatos cortos, fáciles de portar) |
| **pes-fmdl-blender** v0.7.2 | "foreground" (the4chancup) | https://github.com/the4chancup/pes-fmdl-blender | sin archivo de licencia | Python (plugin Blender) | mayo 2026 | Importa/exporta `.fmdl` de PES 2018+ | ★★★ (lectura de fmdl muy completa) |
| **GzsTool** | Atvaark | https://github.com/Atvaark/GzsTool | MIT | C# | 2018 | fpk/fpkd/dat/qar del Fox Engine | ★★★ |
| **FtexTool** | Atvaark | https://github.com/Atvaark/FtexTool | MIT | C# | 2017 | ftex ↔ dds | ★★★ |
| **FoxTool** | Atvaark | https://github.com/Atvaark/FoxTool | (ver repo) | C# | antiguo | archivos .fox2 del Fox Engine | ★ |
| Face ID Relinker / Player Real Face Manager / BAL Face Relink | Rul's, AlexDelPiero23 | pesmodding.com (2026) | cerrados | — | 2026 | Reasignar caras a IDs | ★ (solo uso) |

Datos de formato [VERIFICADO EN CÓDIGO: pes-file-tools]:

- **.fpk**: cabecera de 48 bytes `"foxfpk"` + 1 byte + `"win"` + u32 + 18 bytes + 4×u32. Cada entrada: 4×u64 + 16 bytes (hash).
- **.ftex**: empieza con `"FTEX"`.
- **.fsop**: cada shader va cifrado con XOR **0x9C**.
- Las caras de jugador van en carpetas cuyo nombre es el **ID del jugador** (convención de los facepacks; el compilador 4cc usa los nombres `face_high`, `hair_high`, `oral`, `fcl_hair`).
- Los jugadores creados en el EDIT reciben IDs **≥ 0x80000000** (2147483648) [SEGÚN PÁGINA: Ejogc]. Úsalo para separar tus jugadores nuevos de los de Konami.

---

## 6. Parches del exe

Lo que hay en público es muy poco:

- **No encontré** ninguna lista abierta de direcciones para quitar límites, cambiar ventanas de fichajes o el calendario de liga **[NO ENCONTRADO]**.
- **Starting Year Tool v5.1** (Devil Cold52): cambia el año inicial de ML/BAL; funciona con "todos los exe de Steam" de la Season Update. Cerrado y ofuscado (puede dar falso positivo de antivirus). https://www.pesmodding.com/2022/04/pes-2021-starting-year-tool-v51.html [SEGÚN PÁGINA]
- **Sider 7** (ver sección 9) es la forma moderna: en vez de parchear el exe en disco, se parchea **en memoria** desde Lua (`memory.search`, `memory.write`, `memory.allocate_codecave`). Su archivo `patterns.h` (969 líneas) contiene los **patrones de bytes** que Sider busca en PES2021.exe: es la mejor lista pública de "sitios interesantes" del exe.
- El presupuesto de ML se lee en el exe en `PES2021.exe+EA4C58` (instrucción `mov eax,[rdi+016ECBF4]`), según la tabla de Aranaktu (sección 9).
- Versiones del exe / Data Pack: la tabla de Aranaktu (v21.1.0, nov 2022) y las herramientas de 2026 dicen estar probadas en la **última Season Update de Steam**. No confirmé el número de versión exacto "1.01.01"; compruébalo en las propiedades de tu exe antes de usar offsets fijos. **Mejor buscar por patrón AOB que por dirección fija.**

---

## 7. Option file (EDIT00000000)

### 7.1 Herramientas

| Nombre | Autor | Enlace | Licencia | Lenguaje | Actualidad | Utilidad |
|---|---|---|---|---|---|---|
| **libpesXcrypter** (rama `libpesXcrypter` de pesXdecrypter) | the4chancup (mantenido por 4ccbent; actualización PES 19/20 por zlac) | https://github.com/the4chancup/pesXdecrypter/tree/libpesXcrypter | **Dominio público** | C | último commit **18-09-2020 "Updated for PES21"** | ★★★★★ (es la que ya usas) |
| **4ccEditor** "Spring 26 Edition" | the4chancup | https://github.com/the4chancup/4ccEditor | zlib | C++ | abr 2026 | ★★★★★ |
| **pesXdecrypter 2021** (versión de Zlac) | Zlac | https://www.pesmodding.com/2020/09/pes-2021-pesxdecrypter-tool.html | — | — | 2020 | ★★ |
| **PES 2021 Editor** | Ejogc327 | (sección 2) | cerrado | .NET | — | ★★ |
| **File Crypter** | Devil Cold52 | https://www.pesmodding.com/2020/09/pes-2021-file-crypter-by-devil-cold52.html | cerrado | — | 2020 | ★ |

No hay versión de libpesXcrypter más nueva que la de septiembre de 2020.

### 7.2 API de libpesXcrypter [VERIFICADO EN CÓDIGO]

- Clave exportada: `MasterKeyPes21` (64 bytes).
- Funciones: `createFileDescriptorNew()`, `decryptWithKeyNew(desc, input, masterKey)`, `encryptWithKeyNew(desc, &size, masterKey)`, `destroyFileDescriptorNew(desc)`.
- `FileHeaderNew`: `mysteryData[64]`, `u32 dataSize`, `u32 logoSize`, `u32 descSize`, `u32 serialLength`, `hash[64]`, `fileTypeString[32]`, `gameVersionString[32]`.
- Bloques del archivo: cabecera de cifrado, cabecera, logo, descripción, **datos**, serial. El hash/checksum **no lo comprueba el juego** (según el README).

### 7.3 Mapa del bloque de datos de PES 2021 [VERIFICADO EN CÓDIGO: 4ccEditor main.cpp / pes20.cpp]

```
0x60      u16  número de jugadores
0x64      u16  número de equipos
0x7C      jugadores: cada uno 0x138 bytes (312)
            = 0xF0 (240) datos del jugador + 0x48 (72) apariencia
0x8ED2FC  equipos: cada uno 0x24C bytes (588)        (PES 20 = 0x210)
0x9D4648  plantillas: cada una 0x11C bytes (284)      (PES 20 = 0x9CCC04)
0xA09880  tácticas: cada una 0x274 bytes (628)        (PES 20 = 0xA01E3C)
```

**Equipo (0x24C)**:
```
0x00  u32 ID del equipo
0x04  u32 ID del entrenador
0x08  2   emblema
0x0A  u16 ID del estadio
0x0C..0x15  desconocido
0x16..  colores del equipo (6 bits por componente, repartidos), flags "nombre editado",
        "emblema editado", colores de fondo, "estadio editado", ajustes de estadio
… +0x39 bytes, luego 3 × u32 ID de rival, 4 bytes flags de pancartas
     70 bytes nombre (UTF-8)
     4 bytes abreviatura
     181 bytes nombre del estadio
     4 × 16 bytes textos de pancartas
     165 bytes desconocido (PES 20: 105)
```

**Plantilla (0x11C)**: `u32 ID equipo` + `40 × u32 ID jugador` + `40 × u16 dorsal` + `40 bytes desconocidos`.

**Tácticas (0x274)**: `u32 ID equipo`, +0x1E0 bytes, **11 bytes** con los índices del once inicial, +0x26 bytes, **índice del capitán en 0x215**, +0x5F bytes.

**Jugador (0xF0)**: `u32 ID`, 4 bytes (comentario), `u16 nación`, `u8 altura`, `u8 peso`, luego todas las habilidades empaquetadas por bits (tu mapa de bits de Player.bin coincide con esto). Al final: nombre 61 bytes, nombre en camiseta 61 bytes, nombre de selección 64 bytes. Apariencia (0x48): `u32 ID`, flags de edición, `boot_id` (14 bits), `glove_id` (14 bits), `copy_id` (u32), medidas del cuerpo en 4 bits, etc.

Las funciones `read_data(start_bit, bits, current_byte)` y `write_data(...)` de `data_util.cpp` son las que empaquetan los bits; son cortas y reutilizables (licencia zlib).

---

## 8. Liga Máster (ML)

- **No existe ningún editor de partidas de ML de código abierto** para PES 2021 **[NO ENCONTRADO]**. Tampoco hay documentación pública del formato del guardado de ML.
- Lo que sí hay trabaja **en memoria RAM**, no en el archivo:

| Nombre | Autor | Enlace | Licencia | Cómo funciona |
|---|---|---|---|---|
| **PES 2021 Cheat Table v21.1.0** | xAranaktu | https://github.com/xAranaktu/PES-2021-Cheat-Table | **MIT** | Cheat Engine. Presupuestos, jugador, tiempo de partido, estamina. Incluye tabla para SP Football Life 2023 |
| **PES 2021 Live Editor** | Ruls (con la tabla de xAranaktu) | https://www.pesmodding.com/2026/06/pes-2021-live-editor-real-time-memory.html | cerrado | Lee la tabla de Aranaktu y escribe en memoria con `memory_engine.dll` |
| **BudgetSetter.lua** (ML Budget Manager v1.0) | JamesHoward333 (direcciones: xAranaktu, tuongquoccuong) | https://www.pesmodding.com/2026/03/pes-2021sp-football-life-2026-master.html | ? (es un .lua, se puede leer) | Módulo de Sider: cambia presupuesto de fichajes y de salarios con teclas. Hay que abrir el menú Negociaciones una vez para "inicializarlo" |
| **Master League Selector** | Geazi Gonçalves | https://www.pesmodding.com/2025/05/pes-2021-master-league-selector.html | ? | Módulo Sider para elegir club de ML |

Idea práctica: como ya lees el guardado de ML, puedes buscar en tu archivo los mismos valores que muestra la tabla de Aranaktu en memoria (presupuesto, salario, fecha de contrato, valor de mercado). Muchas veces la estructura en RAM y en el guardado es parecida.

---

## 9. Cheat Engine y memoria (para un futuro puente con Sider)

### 9.1 Tabla de xAranaktu v21.1.0 [VERIFICADO EN CÓDIGO]

Patrones AOB (se buscan dentro de `PES2021.exe`):

```
INJECT_ClubBudget      8B 87 F4 CB 6E 01 89 45 C4
INJECT_ptrPlayer       CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01   (se inyecta en +0x13)
INJECT_ptrPlayerTwo    48 8B 40 2C 48 89 02
INJECT_MatchTime       8B 44 24 40 89 44 24 40
INJECT_UnlimitedStamina F7 7D 18 48 83 C4 30
INJECT_TrainingOnChange 0F BE 41 0A 2B C2
INJECT_TrainingOnEnter 49 8B 86 90 00 00 00 0F BE
```

**Presupuesto de ML**: el código captura `rdi` (puntero al club) en esta instrucción:
`PES2021.exe+EA4C58: mov eax,[rdi+016ECBF4]`.
- Presupuesto de fichajes = `[rdi + 0x016ECBF4]` (u32)
- Presupuesto de salarios = `[rdi + 0x016ECC08]` (u32)

**Jugador en RAM** (puntero capturado al pasar el cursor sobre un jugador; la estructura mide unos 0x17C bytes):

```
0x000  u8  altura (cm)          0x001  u8 peso (kg)
0x003  bit7 pie fuerte; bits0-6 Offensive Awareness
0x004  bits0-6 Defensive Awareness; bit7.. GK Awareness
0x007  bits4-7 posición registrada
0x017  bits4-6 forma
0x01C  bits0-5 edad
0x01D  bits3-7 estilo de juego
0x01F..0x022  posiciones jugables (2 bits cada una)
0x024..0x029  habilidades (1 bit cada una)
0x030  u32 ID del jugador
0x034  u32 ID de comentario
0x038  nombre (texto)    0x075 nombre en camiseta (club)    0x0B2 nombre en camiseta (selección)
0x12C  u16 equipo (?)    0x12E u16 liga (?)
0x138  u16 año fin de contrato   0x13A u8 mes   0x13B u8 día
0x13E  u8 afecto
0x13F  bit0 afecto máximo (?), bit1 jugador "listado"
0x143  bits6-7 nivel
0x144  u16 nacionalidad
0x146  bits0-6 barra de estamina; bit7 flecha de forma parpadeando
0x147  bits0-2 flecha de forma
0x148  u8 días de baja
0x14A  bit1 en lista de transferibles; bit2 en lista de cesión
0x150  bits0-4 rol en el equipo
0x151..0x155  personalidad (Team Player, Passion, Technique, Insight, Impact)
0x15C  u32 salario (euros)
0x174  u32 valor de mercado (euros)
```

(Los bits de las habilidades individuales están en el archivo de la tabla; los extraje y coinciden con la idea de "1 bit por habilidad".)

### 9.2 Sider 7 [VERIFICADO EN CÓDIGO: github.com/pes-modding/sider7]

- Autor: **juce** (y nesa24). Versión **7.4.1** (10 de abril de 2026); último commit en julio de 2026.
- Lenguaje: **C++** + LuaJIT. El código está público, pero la licencia solo permite **redistribuir el binario sin modificar**. Puedes **leerlo y aprender**, pero no copiar código.
- Documentación: `doc/scripting.html` (para `sider.dll` 7.4).
- Cosas útiles para tu puente:
  - **`match.stats()`** (activar con `match-stats.enabled = 1` en `sider.ini`): devuelve `home_score`, `away_score`, `pk_home_score`, `pk_away_score`, `period` (0–5), `clock_minutes`, `clock_seconds`, `added_minutes`. **Esto te da el marcador en vivo sin buscar memoria tú mismo.**
  - Contexto: `ctx.home_team`, `ctx.away_team`, `ctx.tournament_id`, `ctx.match_id`, `ctx.match_leg`, `ctx.stadium`, `ctx.kits`, etc.
  - Eventos: `livecpk_make_key`, `livecpk_get_filepath`, `livecpk_rewrite`, `livecpk_data_ready`, `livecpk_read`, `set_teams`, `set_kits`, `set_match_time`, `set_stadium`, `set_conditions`, `after_set_conditions`, `trophy_check`, etc.
  - Librería `memory`: `read`, `write`, `guard`, `search`, `safe_search`, `pack`, `unpack`, `get_process_info`, `search_process`, `allocate_codecave`.
  - `livecpk_data_ready` / `livecpk_read` dejan **leer cualquier archivo cuando el juego lo carga** (por ejemplo, Team.bin). Así puedes leer o cambiar la base de datos "en caliente" sin rehacer CPK.
  - Opción nueva `save.folder` (7.4.0): permite que un parche use **su propio EDIT00000000**.
  - **Aviso 7.4.0**: el truco de memoria de `colorsdemo.lua` ya **no funciona** y corrompe memoria.

---

## 10. Recomendación de orden de trabajo (para tu herramienta en C++)

1. **WESYS**: comprimir/descomprimir (16 bytes + zlib). Es lo más fácil y lo necesitas para todo.
2. **Escritor de CPK**: porta `CpkWriter` de pes-file-tools (alineación 0x800, TOC sin compresión). Prueba con un CPK que solo contenga un Team.bin cambiado.
3. **DpFileList**: lee el original, copia una fila de 48 bytes como plantilla y cambia el nombre en 0x10.
4. **Team.bin / Coach.bin / PlayerAssignment.bin / CompetitionEntry.bin**: usa los tamaños 1532 / 100 / 16 / 12. Comprueba siempre que `tamaño_archivo % tamaño_registro == 0`.
5. **Kits**: UniColor (85), TeamColor (16), UniformParameter (contenedor) y kit config (0x78).
6. **Competiciones**: no cambies el número de equipos por liga. Saca CompetitionRegulation comparando archivos a mano.
7. **Puente en vivo**: módulo Lua de Sider con `match.stats()` + los AOB de Aranaktu.

---

## Fuentes

- https://github.com/the4chancup/pes-file-tools
- https://github.com/the4chancup/pes-db-generator
- https://github.com/the4chancup/4cc-aet-compiler-red
- https://github.com/the4chancup/4ccEditor
- https://github.com/the4chancup/pesXdecrypter (rama libpesXcrypter)
- https://github.com/the4chancup/pes-fmdl-blender
- https://github.com/the4chancup/pes-gameplay-editor (edita los .bin del CPK "dt18", IA del juego)
- https://github.com/orgs/the4chancup/repositories
- https://github.com/pes-modding/sider7
- https://github.com/xAranaktu/PES-2021-Cheat-Table
- https://github.com/Youjose/PyCriCodecs
- https://github.com/MjTs140914/CpkPatcher
- https://github.com/esperknight/CriPakTools · https://github.com/uyjulian/CriPakTools · https://github.com/wmltogether/CriPakTools
- https://github.com/kamikat/cpktools
- https://github.com/Atvaark/GzsTool · https://github.com/Atvaark/FtexTool · https://github.com/Atvaark/FoxTool
- https://www.pesmodding.com/2021/06/pes-2021-new-dpfilelist-generator-dlc-70.html
- https://www.pesmodding.com/2020/09/efootball-pes-2021-cheat-table.html
- https://www.pesmodding.com/2026/04/pes-2021-sider-v741.html
- https://www.pesmodding.com/2026/06/pes-2021-live-editor-real-time-memory.html
- https://www.pesmodding.com/2026/07/pesdatabase-v021-efootball-and-pes.html
- https://www.pesmodding.com/2026/03/pes-2021sp-football-life-2026-master.html
- https://www.pesmodding.com/2025/05/pes-2021-master-league-selector.html
- https://www.pesmodding.com/2022/04/pes-2021-starting-year-tool-v51.html
- https://www.pesmodding.com/2022/05/pes-2021-evoweb-patch-2022-update.html
- https://www.pesmodding.com/2020/09/pes-2021-pesxdecrypter-tool.html
- https://www.soccerfandom.com/2020/10/pes-2021-editor-by-ejogc327.html
- https://www.balkanpesbox.com/forum/topic/4066-pes-editor-by-ejogc-tutorials-for-database-edits/
- https://www.welovepes.com/2020/03/dinotemeditor.html
- https://evoweb.uk/threads/create-a-new-league-in-pes-2021.86342/
- https://evoweb.uk/threads/master-league-crash-2-season-pes-2021.97263/

No pude abrir (bloqueados para mí): evo-web.co.uk e implyingrigged.info (la wiki de 4chan Cup tiene una página "Pro_Evolution_Soccer_2021/Edit_file" que podría tener más detalles; ábrela tú desde el navegador).
