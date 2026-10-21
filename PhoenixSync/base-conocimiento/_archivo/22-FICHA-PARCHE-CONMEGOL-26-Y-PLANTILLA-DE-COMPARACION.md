# 22 · Ficha del parche ConmeGOL 26 y plantilla para comparar parches

**Para FRALEX · 10 de octubre de 2026.** Objetivo: tener TODO lo de ConmeGOL clasificado en un solo lugar (qué trae, qué hace bien, qué hace mal, qué bugs tiene) y una plantilla para compararlo con otros parches y decidir qué copiar al parche Phoenix.
Etiquetas: [OBSERVADO] visto en tus archivos · [PROBADO] visto funcionando · [AUDITORÍA] viene de la auditoría del 8 de oct (`sider/AUDITORIA-SIDER.md`) · [FALTA] sin comprobar · [ESTIMACIÓN] juicio mío.

---

## 1. Identidad

| Dato | Valor |
|---|---|
| Nombre | CONMEGOL PATCH 2026 («ConmeGOL Patch 26») |
| Tipo de parche | Estilo moderno: casi todo suelto por Sider (`livecpk`), base de datos pequeña en un `.cpk` |
| Juego base | eFootball PES 2021 SEASON UPDATE, `PES2021.exe` de 458.806.784 B (16/4/2023) |
| Steam ID usado | 239200 (variante «Clásicos» 239203) |
| Carpeta de guardado | `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\saveConmeGOL Patch 26` |
| Option file | `PES21_Hook\EDIT00000000` (11.026.776 B) |
| Base de datos | `download\CGP_database.cpk` (3,7 MB) + `SiderAddons\olmosjr23\Database` (copia suelta de la base, autor de la base: Olmos Jr 23) |
| Sider | 7.3.3, 61 líneas de módulo, 37 carpetas en `livecpk`, 34 carpetas en `content` |
| Variantes en `ConmeGol Extras` | (1) **ConmeGOL Patch 26** (la normal, con Liga Máster) · (2) **Clásicos (No LM)** (incluye el Mundial de 48 y equipos clásicos; sin Liga Máster) · (3) **ConmeGOL Patch 26 (Perú, Chile, Uruguay B)** (otra mezcla de ligas) |
| Cambio entre variantes | El «switcher» de FRALEX copia la carpeta `SiderAddons` de la variante elegida encima de la del juego |

## 2. Contenido de la base de datos [OBSERVADO]

| Concepto | Original Konami | ConmeGOL 26 | Sudamerican 2026 |
|---|---|---|---|
| Competiciones (`Competition.bin`) | 91 | 90 | 80 (edición Mundial: 81) |
| Filas de equipos por competición | 1.291 | 1.359 | 1.166 (Mundial: 682) |
| Equipos (`Team.bin`) | 663 | 749 | 726 (Mundial: 103) |
| Jugadores (`Player.bin`) | 17.782 | 29.997 | 29.323 (Mundial: 29.471) |
| Liga de 1.ª con más equipos | 24 | 30 (Argentina, MLS) | 30 (Argentina, EE. UU.) |
| Copa más grande | 44 | 66 (Copa Argentina) | 60 (Copa Argentina) |
| Segundas divisiones | 5 (Ing., Esp., Fra., Ita., Bra.) | 7 (Arg. 36, Bra., Ecu., Col., Chi., Per., Uru.) | 5 (Arg. 32, Per., Bra., Chi., Ing.) |
| Torneos continentales | Champions, Europa, Libertadores, AFC, Concacaf | Libertadores 47, Sudamericana 44, Recopa, Champions, Concacaf | Champions, Europa, Libertadores 33, Concacaf |
| Ligas europeas | Ing., Ita., Esp., Fra., Hol., Por., Bél., Rus., Sui., Tur., Din., Esc. | Ing., Ita., Esp., Fra. (solo las 4 grandes + Arabia) | Ing., Esp., Ita., Ale., Fra., Hol. |
| Ligas de otros continentes | China, Tailandia | MLS, México, Arabia Saudita, Costa Rica, Honduras | México, EE. UU. |

Cómo lo hace: **reutiliza los mismos espacios (IDs) del original con otro contenido** (ver informe 21). El ID 9 era Inglaterra y ahora es Argentina, etc.

## 3. Virtudes (lo bueno)

| # | Virtud | Evidencia |
|---|---|---|
| V1 | **Mucho contenido sudamericano licenciado/ordenado:** 7 segundas divisiones, Copa Argentina de 66, Libertadores de 47, Sudamericana de 44 | `Competition.bin` y `CompetitionEntry.bin` |
| V2 | **Ligas de 30 equipos** funcionando con el .exe estándar | Base de datos; el motor lee el número desde un dato con tope 127 |
| V3 | **Parche liviano:** base de datos de 3,7 MB; el resto va suelto por Sider, fácil de cambiar sin reempaquetar | `sider.ini` con `cpk.root` |
| V4 | **Competition-Server** activo: ascensos/descensos y clasificación a copas | `modules/Competition-Server.lua`, `content/Competition-Server/data.csv` |
| V5 | **Mundial de 48 equipos** (variante «Clásicos») con módulos de grupos, llaves, calendario | `FWC_*.lua`; `FWC_Expansion.lua` contiene los parches de Gogosz y los otros tres siguen el mismo esquema [OBSERVADO en `FWC_Expansion`, supuesto en los demás] |
| V6 | **Muchos módulos de presentación:** estadios, kits, balones, cánticos, himnos, comentaristas, cámaras, VAR, banners, marcadores, intros, entradas al campo | Carpeta `modules`, 60 módulos pedidos en `sider.ini` |
| V7 | **Selector de club de Liga Máster** y sala de prensa por club | `MasterLeague.lua`, `content/Master-League-Selector` |
| V8 | **Tres variantes** para distintos usos (Liga Máster, Clásicos/Mundial, otra mezcla de ligas) | `ConmeGol Extras` |
| V9 | Compatible con Liga Máster guardable y descifrable (probado en esta sesión) | Informes 9 a 11 |
| V10 | **Módulo Phoenix** ya integrado en `modules` (versiones 010 a 017m) | `phoenix.lua*` |

## 4. Falencias, bugs y riesgos

| # | Gravedad | Falencia | Evidencia |
|---|---|---|---|
| F1 | Alta | `camera.lua` y `DynamicWideCam.lua` parchean la **misma** instrucción del juego y se pisan | [AUDITORÍA] problema 1 |
| F2 | Alta | `GFX_lod.lua` escribe en una dirección fija pensada para otra versión del .exe, sin comprobar | [AUDITORÍA] problema 2 |
| F3 | Media-alta | `SoundServer.lua` sigue punteros desde una dirección fija sin comprobar y escribe cada fotograma | [AUDITORÍA] problema 3 |
| F4 | Media | Módulos que escriben en memoria cada fotograma sin protección (`CGP_SB_Addons_2`), parches de código grandes (`shirtless_celebration`) | [AUDITORÍA] problemas 4 y 5 |
| F5 | Media | **11 módulos pedidos en `sider.ini` no existen** (tunnel, Derbys, cr7_audio, CornerFlag…) y hay entradas repetidas («Menu Light») | [AUDITORÍA] problemas 8 y 10 |
| F6 | Media | **Estadios asignados dos veces** a la misma competición (grupos del Mundial 2026, Euro, MLS): el módulo avisa «Skipping duplicated assignment» más de 25 veces | `sider.log` (10 oct) |
| F7 | Baja | Falta la configuración de kits de **Real Betis** (g4 y g5) | `sider.log`: «unable to load kit config» |
| F8 | Baja | `UIColors` no encuentra `topModeSelectDMM.bin`; `env.lua` da error | `sider.log` |
| F9 | Media | **Perdió ligas europeas del original** (Países Bajos, Portugal, Bélgica, Rusia, Suiza, Turquía, Dinamarca, Escocia, China, Tailandia) para dar espacio a las sudamericanas | Comparación con `dt10_x64.cpk` |
| F10 | Media | **El Mundial de 48 no se puede usar con Liga Máster** (variante «No LM») | `ConmeGol Extras\Clasicos (No LM)` |
| F11 | Baja | Una misma liga con 2 versiones de nombre entre parches (por ejemplo IDs reutilizados), lo que complica mezclar option files | Informe 21 |
| F12 | Info | **Anomalía abierta:** el calendario de la Liga Máster de FRALEX marca la Premier como competición 9, pero en la base actual el 9 es Argentina | Informe 21 |
| F13 | Info | El `sider.log` revisado antes era de una sesión sin partido; falta log con partido jugado | [AUDITORÍA] |
| F14 | Info | Los módulos de Sudamerican (netblock, jittest, zlibtest…) no aparecen aquí; en ConmeGOL sí están `Anti-Cheat` (placebo, apagado) y `attack_mentality` (apagado, alto riesgo) | [AUDITORÍA] |

## 5. Cosas únicas o curiosas
- El módulo `Competition-Server` y los `FWC_*` son de la comunidad (Gerlamp, Gogosz). ConmeGOL no los escribió; los integró.
- La mayor parte de los datos de estadios, kits y comentaristas viven en `content/` en forma de archivos de texto (`map_competitions.txt`), no dentro del juego.
- Los IDs que ven los módulos de Sider (`tournament_id`) son distintos de los de la base de datos (informe 23, sección 2.4).
- ConmeGOL incluye 4 competiciones con IDs nuevos (145, 197, 198, 199) de tipo «especial».

## 6. Comparación rápida con Sudamerican 2026 [OBSERVADO]

| Tema | ConmeGOL 26 | Sudamerican 2026 |
|---|---|---|
| Estilo | Sider (suelto) | `.cpk` clásicos: 34 archivos; las caras son 13 archivos de 0,7 a 11 GB |
| Tamaño de `download` | ~9,3 GB (7 archivos `dt80` + 3,7 MB de base) | ~135 GB |
| Sider | 1, 60 módulos pedidos | 2 («normal» 56 módulos y «WC26» 66 módulos) |
| Módulo de Liga Máster | `MasterLeague.lua` (selector de club) | `MasterLeague.lua` y `ML.lua` (v1.6, mezcla menús) |
| Mundial de 48 | Variante «Clásicos (No LM)» con `FWC_*` | Edición aparte `Sider WC26` + `SP_Subs_WC.cpk` con `GogoszPatch*` |
| Online | No bloquea | `netblock.lua` **bloquea la red** |
| Módulos de prueba activos | No | `jittest`, `zlibtest`, `etrace` |
| Módulos cargados dos veces | Algunos pares que se pisan | `MenuServer`, `env`, `etrace`, `camera`, `netblock` |
| Errores vistos en el log | Avisos de estadios duplicados, kits Betis, UIColors | `SoundtrackServer` (valor nulo, 5 veces), `randomMenu`, `TurfLoader`, `LogoHD` fallan al iniciar |
| Mismo .exe | Sí | Sí |
| Documentación | Poca | `LEER.txt` con créditos de 30+ personas, Discord oficial |

## 7. Qué conviene copiar o evitar en Phoenix

**Copiar:**
1. Base de datos en un `.cpk` pequeño y todo lo demás por Sider (de ConmeGOL).
2. Competition-Server para ascensos y descensos (ya está en ambos).
3. Técnica de Gogosz (memoria por anclas) para estructuras de torneo propias, **solo si se prueba aparte de Liga Máster**.
4. Variantes separadas por uso (Liga Máster / Mundial / Clásicos).
5. Fichero de créditos y Discord (de Sudamerican).

**Evitar:**
1. Duplicar módulos o dejar módulos de prueba activos (Sudamerican).
2. `netblock.lua` si habrá juego en línea.
3. Pares de módulos que parchean la misma instrucción (cámaras).
4. Escribir a direcciones fijas sin comprobar (GFX_lod).
5. Reutilizar IDs de competición sin anotar la tabla de equivalencias (riesgo para guardados de Liga Máster).

## 8. Plantilla para fichar CUALQUIER otro parche (copia y rellena)

```
PARCHE: ____________  VERSIÓN: ______  FECHA: ______  AUTOR/ES: ______
CARPETA: ______  TAMAÑO TOTAL: ____  CPK PROPIOS: ____  USA SIDER: sí/no (versión __)
.EXE: tamaño ____  sha256 ____  ¿igual al de ConmeGOL? sí/no

BASE DE DATOS (abrir los .bin con Python/zlib):
 competiciones __ | filas de equipos __ | equipos __ | jugadores __
 liga más grande __ | copa más grande __ | segundas divisiones __
 IDs nuevos respecto al original: __ | IDs reutilizados: __

LIGA MÁSTER: ¿funciona? ¿2.ª temporada? ¿guardado descifrable? ¿ligas jugables? __
MÓDULOS (sider.ini): pedidos __ | existen __ | no existen __ | duplicados __
 de competición/torneo: __  de cámara: __  de estadios: __  de kits: __  de sonido: __
LOG (sider.log con partido jugado): errores __ | avisos __
VIRTUDES (con evidencia): __
FALENCIAS Y BUGS (con evidencia y gravedad): __
COSAS ÚNICAS: __
QUÉ COPIAR / QUÉ EVITAR: __
PENDIENTE: __
```

**Cómo sacar los datos (comandos mínimos):**
- Abrir un `.bin` WESYS: leer el archivo, saltar 16 bytes, `zlib.decompress`.
- Competition.bin = filas de 36 B (byte 5 = ID, byte 3 = región, nombre ASCII desde el byte 8). CompetitionEntry.bin = filas de 12 B (el tercer número: `(puesto<<8)|ID`). Team.bin = 1.532 B. Player.bin = 312 B.
- Si la base está dentro de un `.cpk`, buscar la marca `ff 10 81 'WESYS'` y descomprimir cada bloque.
- Módulos de Sider: leer `sider.ini` (líneas `lua.module` y `cpk.root`), comparar con la carpeta `modules`.
- Errores: filtrar `sider.log` por `error`, `fail`, `cannot`, `PROBLEM`.

## 9. Pendiente para completar la comparación
- Evoweb 2024, Football Life 27 y UML 2024 (no están en tu PC: hay que bajarlos o describirlos por la web).
- Sudamerican: terminar la comparación de equipos y jugadores entre ambos parches, y revisar su `sider.log` con partido.
- Repetir la plantilla en cada parche nuevo.
