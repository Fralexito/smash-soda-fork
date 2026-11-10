# Terreno para la DB global de Phoenix (10-nov-2026)

Inventario **por ruta** de caras, minicaras, kits, estadios y balones en las tres instalaciones de PES 2021 de Frank, con los **choques** entre ellas y una **propuesta de carpetas PHX_** para meter todo como paquetes on/off del Phoenix Launcher. Todo se hizo **solo leyendo**: no se copió, movió ni cambió ningún archivo del juego. Las salidas son índices (CSV) y no contienen archivos de los parches.

> Hilo de BD: este trabajo **no repite** lo suyo. Usa su lector de TOC (`pes-db/panel-duelo/cpk_toc.py`) y su tabla maestra (`pes-db/tabla-maestra/`). Le devuelvo dos hallazgos que le sirven (ver §4 y §6).

---

## 1. Qué se consiguió

| Categoría | Base (PES limpio) | Conmegol | Sudamerican (+WC26) |
|---|---|---|---|
| Caras 3D (`face/real/<id>/`) | 2.838 ids · 15,7 GB | 12.479 ids · 52,0 GB | 16.015 ids · 98,7 GB |
| Minicaras (`render/symbol/player/<id>.dds`) | 22.978 ids · 0,5 GB | 4.427 ids · 0,2 GB | 40.332 ids · 1,5 GB |
| Kits (equipos con textura `uXXXX` o carpeta kit-server) | 450 · 2,2 GB | 3.106 · 5,6 GB | 4.296 · 17,8 GB |
| Estadios (`bg/stNNN`, stadium-server) | 69 · 0,1 GB | 720 · 142,7 GB | 1.001 · 257,3 GB |
| Balones (`model/ball/ballNNN`, ball-server) | 52 · 0,06 GB | 88 · 0,3 GB | 1.577 · 7,4 GB |

(«ids» = carpetas o archivos distintos; en kit-server y stadium-server el id es la carpeta, p. ej. `brasileirao betano/flamengo`. Los GB son tamaño descomprimido según el TOC o tamaño en disco en carpetas sueltas.)

**Choques encontrados**

| Tipo | Cuántos | Qué significa |
|---|---|---|
| Mismo id de cara en Conmegol y Sudamerican | 7.555 (835 también en el base) | La misma carpeta `face/real/<id>` existe en los dos. Si se encienden juntos, gana el de arriba. |
| Mismo id de minicara en ≥2 fuentes | 20.072 | Casi todo base+Sudamerican (17.643): Sudamerican rehízo minicaras del juego. |
| Mismo id de kit en ≥2 fuentes | 750 | 308 Conmegol+Sudamerican, 204 en las tres. |
| Estadios `stNNN` en ≥2 fuentes | 70 | Los st del juego que ambos parches retocan (césped, luces). |
| **Identidad de jugador** (mismo id, ¿otra persona?) | 711 a revisar → **117 de riesgo ALTO** (cara en los dos parches), 32 medio (minicara en los dos), 562 bajo | Salen de las filas `mismo_id_nombre_distinto` de la tabla maestra. En los ALTO, poner la cara de un parche sobre la base del otro puede dar la cara de otra persona. |
| **Identidad de equipo** (mismo id, otro equipo según la maestra) | 214 ids; 202 con kit en los dos | Casi todos son **el mismo equipo con otro nombre** («Republic of Ireland» / «Ireland», «Manchester United FC» / «Manchester United»). Es un fallo de la unión por nombre de la maestra de equipos, no un choque real. |
| Choques internos (misma ruta en varios contenedores de la **misma** fuente) | 1.000+ combinaciones | Lo normal (los `download\dt80_*` pisan a `Data\dt14/dt34`) y lo peligroso (en Sudamerican `SP_Faces_9.cpk` pisa caras de `SP_Faces_0…8`; en Conmegol `livecpk\Kits` choca con el paquete Phoenix-Camisetas-Mips). |

**Calidad (indicios):**
- Minicaras: Conmegol las trae a **180×180**, Sudamerican a **128×128** (cabecera DDS de 9.600 minicaras sueltas en choque). Las del base y las de dentro de .cpk no se midieron (habría que extraer).
- Caras compartidas Conmegol+Sudamerican: la de **Conmegol pesa más en 5.154** casos, la de Sudamerican en 2.161 (más bytes ≈ texturas más grandes; no es prueba absoluta).

## 2. Archivos (en `D:\Frank\IA\CLAUDE\pes-db\terreno\`)

| Archivo | Qué es |
|---|---|
| `vista_terreno.html` | Vista para mirar: tarjetas de cifras y tablas con filtro. Abrir con doble clic (no usa internet). |
| `inventario_caras.csv`, `inventario_minicaras.csv`, `inventario_kits.csv`, `inventario_estadios.csv`, `inventario_balones.csv` | Una fila por (id, fuente, contenedor): `categoria,id,nombre,fuente,via,tipo,contenedor,activo,prioridad,n_archivos,bytes,ejemplo`. `nombre` sale de la tabla maestra (jugador/equipo) o de `kit-server\map.txt` (ids de equipo). `activo/prioridad` = si esa carpeta está encendida en el sider.ini de su parche y su puesto (0 = gana). |
| `choques_misma_ruta.csv` | Ids presentes en ≥2 fuentes, con archivos y KB por fuente, la más pesada y medida DDS cuando se pudo leer. |
| `choques_identidad_jugadores.csv` | Los 711 jugadores «mismo id, nombre distinto», con dónde hay cara/minicara y el riesgo. |
| `choques_identidad.csv` | Equipos/kits con mismo id y distinto id Phoenix en la maestra. |
| `choques_internos.csv` | Dentro de una misma fuente, qué contenedores traen la misma ruta, cuántas, ejemplo y quién gana por orden. |
| `resumen.json` | Todas las cifras, contenedores y errores de lectura (lo usa la vista). |
| `crudo_rutas.tsv` (221 MB) y `crudo_contenedores.tsv` | Listado bruto de 1,5 millones de rutas de interés y el estado de cada contenedor. **No va a GitHub** (pesa); se regenera en 2 minutos. |
| `scripts\1_recolectar.py` … `4_vista.py` | Los pasos (abajo). |

## 3. Paso a paso para replicarlo

Requisitos: Python 3.11 (sin librerías extra), el lector `D:\Frank\IA\CLAUDE\pes-db\panel-duelo\cpk_toc.py` y la tabla maestra. Ningún paso abre PES.

```bat
cd /d D:\Frank\IA\CLAUDE\pes-db\terreno\scripts
python -I 1_recolectar.py          :: ~2 min. Lee TOC de .cpk y recorre livecpk/content. -> crudo_*.tsv
python -I 2_clasificar.py          :: ~3 min. Clasifica, agrupa, choques. -> inventario_*.csv, choques_*.csv, resumen.json
python -I 3_identidad_jugadores.py :: segundos. -> choques_identidad_jugadores.csv
python -I 4_vista.py               :: segundos. -> vista_terreno.html
```

Qué hace cada paso:
1. **Recolectar.** Para cada fuente (base `C:\PES21\eFootball PES 2021`, Conmegol `D:\Frank\Games_\Conmegol Patch`, Sudamerican `D:\SP2026` y su variante WC26):
   - Lee el **índice (TOC)** de cada `.cpk` de `Data\` y `download\`. Los `.cpk` de un parche con el **mismo nombre y tamaño** que el del base se saltan (son el juego sin tocar).
   - Lee el `sider.ini` de su Sider y recorre cada `cpk.root` (encendido o con `;`), guardando su orden. Recorre también las subcarpetas de `livecpk` que el ini no nombra («no_listado»).
   - Recorre `content\*` (kit-server, stadium-server, ball-server, miniface_server…).
   - Solo guarda rutas que contienen face/, symbol/player, uniform, kit, stadium, /bg/, ball o miniface.
2. **Clasificar.** Reglas por ruta (en minúsculas, con `/`):
   - cara: `face/real/<id>/` · minicara: `render/symbol/player/<id>.dds` (también dentro de `miniface_server`)
   - kit: `uniform/texture/…/u<4 cifras>[p|g]<n>` → id de equipo = esas 4 cifras sin ceros a la izquierda; en `kit-server` el id es la carpeta (`liga/equipo`) y su id de equipo sale de `map.txt`
   - estadio: `bg/stNNN` o `stadium/stNNN`; en `stadium-server` la carpeta
   - balón: `model/ball/ballNNN`; en `ball-server` la carpeta
3. **Identidad de jugadores.** Cruza las filas `mismo_id_nombre_distinto` de la maestra con dónde hay cara.
4. **Vista.** Mete el resumen y hasta 4.000 filas por tabla en un HTML autocontenido.

## 4. Errores e intentos fallidos

- **Lanzar el trabajo en un subagente falló**: un hook de la sesión cambiaba la llamada y el modo automático no la pudo revisar. Se hizo todo en el hilo principal.
- **11 `.cpk` del Sudamerican no se pudieron leer**: `SP_Adboards_1…7.cpk` (cabecera rara: «unpack requires a buffer of 8 bytes») y `dt80_300E…600E_x64.cpk` («sin TOC»: probablemente solo traen ITOC). Los Adboards no son de estas categorías; los dt80 del Sudamerican **sí podrían traer caras/kits**: queda como hueco conocido (para leerlos hay que añadir soporte ITOC al lector).
- **89 `cpk.root` del sider.ini principal del Sudamerican no existen en disco** (todo el `PRDX_Body Model` y `SP_cursorfc24`, `SP_Lightmod`). Ese ini parece copiado del WC26; el que de verdad trae esas carpetas es el WC26.
- **La tabla maestra no detecta choques de jugador por sí sola**: su regla 1 es «mismo id = mismo jugador», así que el cruce directo dio 0. Por eso se añadió el paso 3 con los 711 «nombre distinto».
- **La maestra de equipos separa equipos que son el mismo** (214 ids con nombre variante). El cruce de kits los marca como choque; en la práctica casi todos no lo son. **Para el hilo de BD:** unir equipos por «mismo id + nombre parecido» arreglaría casi todos.
- Medir la calidad de minicaras solo se pudo con las sueltas (en carpeta). Las de dentro de un `.cpk` necesitan extraer, y no se hizo para no tardar.

## 5. Cosas a tener en cuenta

- **`face/real/<id>/` no siempre es «una cara».** El mod PRDX Body Model mete por jugador piezas del cuerpo (mangas, medias, vendas) dentro de `face/real/<id>/`. En Conmegol hay ~60 carpetas PRDX así. Un paquete de caras que pise esa carpeta entera puede quitarle a un jugador sus medias o mangas especiales, y al revés.
- **Conmegol usa una carpeta del Sudamerican**: su sider.ini tiene encendido `D:\SP2026\Sudamerican Project Sider\livecpk\SP_Menu Light`. Si algún día se borra o mueve el Sudamerican, al Conmegol le falta eso.
- **En Sider gana el `cpk.root` de arriba**; un livecpk encendido pisa a cualquier `.cpk`. Entre `.cpk`, los de `download\` pisan a los de `Data\`. El orden importa tanto como el contenido.
- **Minicaras**: Sudamerican trae 38.964 sueltas en `SP_Minifaces` (activo, prioridad 101). Son 128×128; las del Conmegol, 180×180.
- **Estadios** son lo más pesado (Conmegol 143 GB, Sudamerican 257 GB descomprimidos). Meter estadios de los dos a la vez no es viable en disco sin elegir.
- Los kits y estadios por `kit-server` / `stadium-server` no van por ruta del juego: el módulo Lua elige qué carpeta servir por id de equipo (`map.txt`). No chocan con livecpk de la misma forma; chocan **en el map.txt** (mismo id de equipo → dos carpetas).

## 6. Propuesta: carpetas PHX_ para la DB global (sin aplicar)

Idea: cada bloque es **una carpeta livecpk** con un nombre corto, que el Phoenix Launcher enciende o apaga con su línea `cpk.root` y ordena con ▲▼. Todas viven en `C:\PES21\eFootball PES 2021\Sider_7.4.1\content\livecpk\` y se anotan en `LEYENDA_NOMBRES.txt`.

**Orden propuesto (arriba gana):**

| Puesto | Carpeta | Contenido | Por qué ahí |
|---|---|---|---|
| 1 | `PHX_FIX_<tema>` | Arreglos puntuales a mano (una cara corregida, un kit) | Siempre debe ganar lo corregido a mano. |
| 2 | `PHX_FAC_RMP` | Caras de los choques de identidad **remapeadas** a ids Phoenix nuevos (ver abajo) | Así no pisan a nadie. |
| 3 | `PHX_FAC_CON` | Caras del Conmegol (más pesadas en 2/3 de los choques) | Mejor calidad estimada en lo compartido. |
| 4 | `PHX_FAC_SUD` | Caras del Sudamerican que el Conmegol **no** tiene (≈8.500 ids) | Rellena sin pisar. |
| 5 | `PHX_MIN_CON` | Minicaras Conmegol (180×180) | Mejor resolución. |
| 6 | `PHX_MIN_SUD` | Minicaras Sudamerican (128×128) | Rellena el resto (~36.000). |
| 7 | `PHX_KIT_CON` / `PHX_KIT_SUD` | Kits por textura `uXXXX` | Igual criterio: uno gana, el otro rellena. |
| 8 | `PHX_BAL_SUD` | Balones (Sudamerican trae 1.577) | Casi sin choques. |
| 9 | `PHX_STD_<liga o país>` | Estadios por bloques pequeños (p. ej. `PHX_STD_BRA`, `PHX_STD_ENG`) | Son enormes: mejor por bloques para encender solo lo que se use. |
| 10 | `PHX_BDY_PRDX` | PRDX Body Model (cuerpos, mangas, medias) | Comparte `face/real/<id>` con las caras (ver §5): probar el orden en vivo. |

Los servidores Lua (kit-server, stadium-server, ball-server, miniface_server) no son livecpk: se propone **un solo kit-server Phoenix** con su `map.txt` construido desde la tabla maestra (id de equipo Phoenix → carpeta), en vez de dos mapas que se pisan.

**Cómo resolver los choques de identidad (propuesta, no aplicada):**
1. Para los **117 jugadores de riesgo ALTO**: mirar a mano (minicara de cada parche) si son la misma persona. Si no lo son, el de un parche se queda con su id y el otro recibe un **id libre del rango Phoenix** (por ejemplo, a partir de 200.000; el juego admite hasta 262.143). Su cara se copia a `PHX_FAC_RMP\…\face\real\<id nuevo>\` y su ficha en la DB usa ese id.
2. Para los **214 equipos**: primero corregir la maestra de equipos uniendo por **id igual + nombre parecido** (casi todos son el mismo equipo). Solo los que queden distintos se remapean como en el punto 1.
3. Guardar la equivalencia `id_original_parche → id_phoenix_juego` en la tabla maestra (columna nueva), para poder deshacerlo.

## 7. Cómo comprobar y revertir

- Comprobar: abrir `vista_terreno.html`; cruzar un caso a mano (p. ej. buscar `41180` en «Identidad jugadores»: Roberto Fernández, cara en los dos parches → ALTO).
- Revertir: no hay nada que revertir en el juego (todo fue lectura). Para quitar el inventario basta borrar la carpeta `terreno\`.

## 8. Ideas nuevas para el parche (para Frank)

- **«Elegir cara» por jugador en el launcher**: con este inventario se puede mostrar, para un jugador, las 2–3 caras que existen (base, Conmegol, Sudamerican) y que Frank elija cuál gana, generando un `PHX_FIX` automático. Nunca se hizo en PES.
- **Paquetes de estadio a la carta**: encender solo los estadios de las ligas que se van a jugar ahorra memoria y tiempo de carga.
- **Detector de choques en el launcher**: al encender un paquete, avisar «este paquete pisa 320 rutas de PHX_KIT_MIP» antes de jugar.
