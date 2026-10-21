# 02 · Hacia el parche Phoenix (parche propio, vinculado a la web, a Phoenix Link y al Mercado)

**Objetivo declarado (8 oct 2026):** sacar un parche propio, el más ambicioso posible, que sea **uno solo con el ecosistema**:
la web (liga, mercado, perfiles), Phoenix Link (salas) y Phoenix Sync (lo que entra al juego). Todo lo que se descubre
auditando ConmeGOL y otros parches alimenta este plan. Esta página es el **mapa de lo que hay que dominar** y el estado real
de cada pieza. Lo marcado ❓ está por verificar: no se da por sabido.

## A. De qué está hecho un parche de PES 2021 (anatomía)

| Pieza | Dónde vive | Qué contiene | Cómo se fabrica / edita | Estado nuestro |
|---|---|---|---|---|
| **Archivos CPK** | `<juego>\download\*.cpk` | paquetes CRI con todo el contenido: base de datos, caras, uniformes, estadios, balones, botines, logos, menús | se arman con herramientas CPK (CriPakTools / CPK File Builder); el juego los carga por `DpFileList.bin` | leemos la base (`.cpk` → `Player.bin`) ✅; **no armamos CPK todavía** ⏳ |
| **`DpFileList.bin`** | `<juego>\download\` | lista de CPK que el juego carga y en qué orden (el último pisa a los anteriores) | «DpFileList Generator» o generador propio | 🔎 formato leído del ConmeGOL (ver `parches/conmegol-26.md`): 16 B + 48 B por CPK |
| **Base de datos** (`common\etc\pesdb\`) | dentro del CPK de base (ConmeGOL: `CGP_database.cpk`) | `Player.bin` (jugadores y habilidades, mapa de bits), `Team.bin`, `PlayerAssignment.bin` (plantillas), `Competition*.bin`, `Coach.bin`, `Stadium.bin`, `Ball.bin`, `Boots.bin`… | editores de base (PES editor / 4ccEditor) o escritura directa con el mapa de bits | `Player.bin` ✅ (mapa de bits en 01 §B); el resto ❓ |
| **Option file** (`EDIT00000000`) | carpeta `save` de Steam | jugadores/equipos editados, plantillas, dorsales, tácticas, uniformes editados, competiciones editadas | el juego (modo Editar) o Phoenix Sync | plantillas, dorsales y tácticas ✅; uniformes/competiciones ❓ |
| **Caras y cuerpos** | CPK (`common\character0\model\character\face\real\<id>\`) | modelos `.fmdl`, texturas `.ftex`, `face.fpk` | Facemaker / Blender + herramientas de la comunidad | solo se reutilizan (viajan con el ID del jugador) |
| **Uniformes** | CPK (`…\uniform\team\<id>\`) + `kit_config` | texturas y configuración por equipo; también vía Sider (kserv) | Kit Studio / plantillas PSD + `kit_config` | ❓ |
| **Estadios, balones, botines, guantes, adboards, marcadores** | CPK | modelos y texturas; muchos se sirven mejor por **Sider** (stadium server, ball server…) | herramientas de la comunidad | ❓ |
| **Logos, banderas, menús** | CPK (`Asset\...`, `common\menu\...`) | emblemas de equipo y competición, fondos, menú | editores de imágenes + empaquetado CPK | ❓ rutas exactas por confirmar |
| **Sider** (juce66) | carpeta del juego: `sider.exe`, `sider.ini`, `modules\*.lua` | carga contenido suelto sin reempaquetar (**LiveCPK**: `content\live-cpk\`), módulos Lua (uniformes, estadios, marcadores, cámara, **jugabilidad**), opción `save.folder` (7.4.0) | se escribe en Lua con la API de Sider | se usará como vehículo ✅ (decidido); módulos propios ⏳ |
| **Guardado de Liga Máster** (`ML0000000N`) | carpeta `save` | la carrera: 700 equipos, tablas del usuario, alineaciones, contratos, dinero, blob (calendario/resultados) | Phoenix Sync | ampliamente documentado en 01 §C–§I ✅ |

## B. Lo que el ecosistema ya tiene y encaja

| Pieza | Estado |
|---|---|
| La web guarda la verdad de la liga (clubes, plantillas, traspasos) y la **firma** (Ed25519) | ✅ (docs/mercado-api.md) |
| Option file **oficial** publicado por el staff, con huella SHA-256 (`/option/actual`) y descarga verificada | ✅ (`PhoenixSync bajar`) |
| Lista de cambios de liga (`/liga/cambios`), firmada, con versión incremental | ✅ web · ✅ Mercado (`sincronizar`, 8 oct) |
| Aplicar los cambios al **option file** (amistosos/salas) con alineación correcta | ✅ (probado en archivo; prueba en juego pendiente) |
| Aplicar los cambios a la **Liga Máster** del usuario: usuario→IA e IA→IA | ✅ (probado en juego: ranuras 9, 10, 13) |
| Aplicar a la Liga Máster: IA→usuario y agentes libres | ⏳ (quedan como «pendientes» en el informe hasta tener la referencia del juego) |
| Interruptores: option file / Liga Máster / ambos | ✅ (`Alcance`) |
| Presupuesto del club del usuario desde fuera (p. ej. el que fija la web) | ✅ (`fijarFinanzas`) |
| Catálogo del parche (jugadores, posiciones, habilidades) subido a la web | ✅ |
| Equivalencias entre parches (jugar con otro parche sin romper la liga) | ✅ (emparejamiento + huella) |

## C. Lo que falta dominar para un parche propio (por orden sugerido)

1. **Empaquetar CPK y generar `DpFileList.bin`** desde Phoenix Sync (o un script). `DpFileList.bin` ya está entendido 🔎; el CPK se puede **evitar casi del todo con LiveCPK de Sider** (así hace ConmeGOL: solo su base va en un `.cpk` de 3,7 MB; el resto, carpetas sueltas). ❓ armar el `.cpk` de la base
2. **Escribir la base de datos** (`Team.bin`, `PlayerAssignment.bin`, `Competition*.bin`) y no solo leer `Player.bin`:
   crear/renombrar equipos, cambiar plantillas de fábrica, competiciones y calendarios base. ❓ (formatos por documentar;
   el mapa de bits de `Player.bin` ya está).
3. **Uniformes y logos** con `kit_config` y emblemas: el mínimo visual para que un club de la liga «exista». ❓
4. **Sider**: módulos propios (p. ej. cargar el option/plantillas de la sala en caliente, marcador con el nombre de la liga,
   `save.folder` por usuario para el «switcher»). ⏳
5. **Distribución y actualización**: el parche se baja/actualiza desde la web igual que el option oficial (huella SHA-256,
   descarga verificada, nunca se sobrescribe sin copia). ⏳ (la mecánica ya existe para el option).
6. **Ligas propias**: fuera de la Liga Máster primero (competiciones del option file / base), dentro después (01 §C/§I). ❓

## D. Qué auditar en cada parche que se estudie (para copiar lo bueno)

- Qué CPK trae y en qué orden los carga (`DpFileList.bin`).
- Cómo organiza la base (`pesdb`), cuántos jugadores/equipos/competiciones, qué IDs usa (rangos libres para crear).
- Si usa Sider y qué módulos (para no reinventarlos).
- Rarezas del option file (jugadores «Player Identidad», equipos vacíos, formatos de tácticas).

**Regla:** cada hallazgo se anota con su estado (✅ 🔎 ❓ ❌) y, si se probó en el juego, con su entrada en `PRUEBAS.md`.
