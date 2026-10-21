# Parche ConmeGOL 27: cómo funciona y cómo se instala Phoenix (2026-10-12, Link 0.2.26)

Fralex instaló la actualización del parche (modos **ConmeGOL Patch 27**, **UEFAGOL 27** y **ConmeGOL Clásicos**). Esta guía dice qué cambió, con la evidencia, y qué hace Phoenix Link ahora.

## 1. Cómo funciona el lanzador nuevo (PES2021 Start.exe)

**Evidencia:** se leyó el código del lanzador (es un programa de Python empaquetado con PyInstaller; solo se leyó, no se modificó nada).

Al pulsar «Iniciar PES 2021» el lanzador hace, en este orden:

1. Comprueba que esté puesta la ruta de guardados de Konami.
2. Ejecuta `robocopy "ConmeGol Extras\<modo>" "<carpeta del juego>" /E /IS /IT`.
   Copia **todo** el contenido de la carpeta del modo encima de la raíz: `CPY.ini`, `download\CGP_database.cpk` (la base de datos del modo) y `SiderAddons\` (content, livecpk, modules y, si el modo lo trae, `sider.ini`).
   No lleva `/MIR` ni `/PURGE`: **no borra nada** de la raíz. Lo que no está en el modo se queda como estaba (por ejemplo, nuestras luas en `SiderAddons\modules`).
3. Escribe el nombre del modo en `version_actual.txt`.
4. Arranca `SiderAddons\sider.exe` (con esa carpeta como directorio de trabajo) y luego `PES2021.exe`.

Solo ofrece 3 tarjetas: Patch 27, UEFAGOL 27 y Clásicos. Los modos 26 siguen en `ConmeGol Extras`, pero este lanzador ya no los muestra.

## 2. Qué `sider.ini` usa cada modo

- Los modos **27 no traen `sider.ini`** en su `SiderAddons`. Por el punto 2 de arriba, al iniciarlos queda el `sider.ini` que ya hay en la raíz. El de la raíz es el que dejó la actualización (3455 bytes, fecha 09-oct 22:17: el del paquete del parche), sin líneas de Phoenix.
- Los modos **26 traen su `sider.ini`** y, al iniciarlos con el lanzador viejo, se copia sobre la raíz (por eso antes la raíz tenía las líneas de Phoenix: venían del modo).
- Sider siempre lee `SiderAddons\sider.ini` y carga los módulos de `SiderAddons\modules\`. Eso se ve en el `sider.log`: `sider dir: …\SiderAddons\`.

**Conclusión:** para los modos 27 la raíz es el único sitio que cuenta. Ahí tienen que estar `phoenix.lua` y `phoenix_referee.lua` y sus dos líneas `lua.module`.

**Suposición, no verificada:** si alguien arranca un modo 26 y después uno 27, la raíz se queda con el `sider.ini` del 26 (los 27 no lo reemplazan). Se deduce del `robocopy`; no se probó arrancando el juego. Link lo avisa (ver §4).

## 3. La base de datos del parche 27

- No hay ningún `Player.bin` suelto del 27. Va **dentro** de `ConmeGol Extras\<modo>\download\CGP_database.cpk`, que el lanzador copia a `download\` de la raíz al iniciar. (La raíz todavía tiene la del 26 hasta que se inicie un modo.)
- Se leyó la tabla de cada `.cpk` (solo lectura). El `Player.bin` de cada uno:

| Base | Tamaño de Player.bin |
|---|---|
| Patch 26 | 1.769.541 |
| **Patch 27** | **1.803.705** |
| **UEFAGOL 27** | **1.828.345** |
| Clásicos 27 | 936.160 (igual que «Clasicos (No LM)») |
| **Phoenix-DB** (los fichajes de Fralex, hechos sobre la base 26) | 1.769.252 |

- Cada modo tiene su propia base. Phoenix-DB sale de la 26: si se pusiera en el 27, **taparía a los jugadores del parche nuevo** con los del viejo.
- **Actualizado (Link 0.2.46/0.2.47):** ya no se copia ninguna base. Los fichajes se generan sobre la base real de cada PC, que Link detecta sola (ver `docs/FICHAJES-COMO-CAMBIOS.md`); Phoenix-DB solo lleva un `PlayerAssignment.bin` generado, nunca un `Player.bin`. Lo que sigue es el estado anterior (hasta la 0.2.45): la regla era que **sin una base reconocida en ese `sider.ini`, Phoenix-DB NO se pone.** La raíz del 27 no tiene ninguna `cpk.root` con `Player.bin` ni la de `olmosjr23\Database`, así que no se puso. No se copió ningún `Player.bin` de Fralex a otros sitios.
- ~~Para tener fichajes sobre el parche 27 hace falta una base propia por parche~~: ya no hace falta; los fichajes son cambios sobre la base del parche activo (0.2.46).

## 4. Qué cambió en Phoenix Link (0.2.26)

- **Detecta la pérdida.** Si el `.lua` está en `modules\` pero su línea ya no está en el `sider.ini` (ni comentada) y hay pruebas de que Phoenix ya estuvo ahí (otro destino sí la tiene, o hay copias `sider.ini.phoenix-…`), «Módulos del juego» muestra **«QUEDÓ SIN CARGAR TRAS UNA ACTUALIZACIÓN: REINSTALAR»** y el botón **REINSTALAR**.
- **Phoenix-DB solo con base.** `phoenix.lua` ya no exige la raíz Phoenix-DB en un `sider.ini` sin base reconocida (antes quedaba «a medias» para siempre) ni crea su carpeta vacía ahí. El mensaje dice dónde no se puso.
- **Muestra la estructura del parche:** último modo lanzado, modos que usan el `sider.ini` de la raíz y modos con el suyo.
- **Avisa** si se juega un modo 27 y la raíz tiene Phoenix-DB o la base `olmosjr23\Database` de un modo 26.
- **Cierre más estricto:** además de PES2021.exe, también exige cerrar `sider.exe`.
- **Copias de seguridad sin pisarse:** si dos cambios caen en el mismo segundo, la segunda copia lleva `-2` (antes la pisaba y se perdía el `sider.ini` original). Lo encontró la prueba nueva.
- Las reglas de siempre siguen: solo con el juego cerrado, copia con sello antes de cada cambio, una línea por módulo, nada más del `sider.ini` cambia, y «Desinstalar» disponible.

Los modos 27 no se tratan como destinos propios (no tienen `sider.ini`); los cubre la raíz. Los modos 26 siguen siendo destinos mientras existan.

## 5. Lo hecho en el juego de Fralex (2026-10-12 01:23)

Con PES2021.exe, sider.exe, Settings.exe y «PES2021 Start.exe» cerrados (comprobado con `tasklist`), usando el código del instalador:

- `SiderAddons\sider.ini` de la raíz: se añadieron **2 líneas** (`lua.module = "phoenix_referee.lua"` y `"phoenix.lua"`, debajo del último módulo activo). Un `diff` contra la copia confirma que no cambió nada más.
- Copias: `sider.ini.phoenix-20261012-012310` (idéntica al `sider.ini` del parche 27, byte a byte) y `…-2` (con la línea de referee).
- `phoenix.lua` (v0.18b, sha1 `0e6aebecc923`) y `phoenix_referee.lua` (v1.2, `b3d7b1312723`) ya estaban en `modules\` y eran idénticos: no se copiaron.
- **Phoenix-DB: no se puso** (sin base reconocida), como corresponde.
- Los 3 modos 26 no se tocaron (ya tenían una línea de cada).
- Falta ver en `sider.log`, tras el próximo arranque, `Loading module: phoenix.lua` / `phoenix_referee.lua`.

## 6. Pruebas

- `pruebas-link/parche27_test.cpp` imita la estructura 27 en una copia y también corre con copias de los `sider.ini` reales (el 27 y el de un modo 26): TODO OK.
- `modulos_test` y `quitar_modulo_test` siguen en TODO OK.
- `pruebas-link/phoenix_en_juego_real.cpp`: herramienta para ver o aplicar el paquete en el juego real con el mismo código del instalador.

## 6. Actualizar el parche pegando una carpeta nueva (observado el 2026-10-13)

Fralex pegó una carpeta **ConmeGol Extras** nueva sin borrar la anterior y reemplazó archivos de la raíz. Lo que se vio en su PC (solo lectura):

- **Extras** quedó con **3 modos** (ConmeGOL Clasicos, ConmeGOL Patch 27, UEFAGOL 27); los modos 26 desaparecieron. Ya no hay destinos de modos 26: solo la raíz.
- El **sider.ini de la raíz volvió al del paquete** (3455 bytes, fecha 09-oct 22:17): **sin las líneas de Phoenix**. Los .lua seguían en `modules`, así que Link los marcó «perdido». Es decir: **pegar una actualización encima borra las líneas de Phoenix del sider.ini**, aunque el lanzador por sí solo no lo haga. Se arregla con Sync › PREPARAR TODO › ARREGLAR TODO (PES y Sider cerrados, con copia de sider.ini).
- La raíz y el modo Patch 27 tienen el mismo CGP_database.cpk (md5 igual); UEFAGOL 27 y Clásicos tienen otro cada uno.
- El Phoenix-DB de la raíz (Player.bin 9-oct y PlayerAssignment.bin 10-oct) **no lo generó el motor actual** (no hay fichajes_aplicados.json): no se activa sin más.
- Recomendación para quien actualiza: borrar `ConmeGol Extras` antes de pegar la nueva, y después pulsar ARREGLAR TODO.
