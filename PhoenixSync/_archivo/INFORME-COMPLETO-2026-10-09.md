# 📕 Informe completo — Cómo se logró: datos en vivo y fichajes compartidos en PES 2021

9 de octubre de 2026, 20:40 (Lima). Proyecto Phoenix (FRALEX). Frentes SYNC y LINK.
(Copia en Google Drive con el mismo título.)
Fuentes: rama `mercado-fase0` (PhoenixSync/REGISTRO.md, PRUEBAS.md, base-conocimiento/, SYNC-COMPARTIDO.md, GUIA-MISMOS-FICHAJES-ACTIVAR.md).

---

## 0. Resumen en 5 líneas
1. Se conectó la web y el juego a través de **Sider** (un «puente» de mods) y un módulo propio, **phoenix.lua**.
2. Se descubrió **qué archivo manda en cada cosa**: stats en `Player.bin`; equipos en el **option file** o en la **base** (`PlayerAssignment.bin`), según la opción en vivo.
3. Se **revivió el botón nativo de Konami** «Datos Actual. en vivo → Activar», apagado desde 2022, para cargar datos de Phoenix **sin reiniciar**.
4. Se construyó en **Phoenix Sync** el motor para **compartir fichajes entre PCs**, con respaldos, anti-bucle y protección contra choques.
5. Falta conectar las últimas piezas (Link, web) y la **prueba real con dos PCs**.

---

## 1. Glosario (palabras nuevas)
- **PES 2021 / parche ConmeGOL:** el juego y el parche que usa la comunidad.
- **Switcher:** programa de ConmeGOL que copia un «modo» (carpeta) encima del juego y lo abre.
- **Sider:** programa que se mete en el juego y permite cargar archivos y módulos **Lua** (pequeños programas) sin tocar el `.exe`.
- **Overlay:** ventana de Sider dentro del juego (tecla Espacio).
- **Phoenix-DB:** carpeta nuestra dentro de `SiderAddons\livecpk\`. Sider sirve al juego los archivos de aquí en lugar de los originales.
- **Base / pesdb:** archivos de datos del parche: `Player.bin` (stats), `PlayerAssignment.bin` (quién juega en qué equipo), `Team.bin`, etc.
- **Option file (`EDIT00000000`):** archivo en Documentos con las ediciones del jugador (equipos, dorsales).
- **Memoria:** lo que el juego tiene cargado mientras está abierto.
- **Parche en memoria:** cambiar unos pocos bytes del juego mientras corre (el `.exe` en disco no se toca; al cerrar se borra).
- **Phoenix Link:** programa de PC de Phoenix (sala, avisos, colocar archivos).
- **Phoenix Sync:** programa de PC de Phoenix (option file, fichajes, Liga Máster).
- **Entrega:** paquete de archivos que Sync deja en una carpeta para que Link lo coloque en el juego.
- **Operación:** un fichaje escrito como mensaje corto: «jugador X del equipo A al B».
- **Respaldo:** copia de seguridad antes de cambiar algo.

---

## 2. El punto de partida
- La comunidad (+50 jugadores) juega PES 2021 online por **Parsec**.
- Konami **apagó sus servidores** el 25/08/2022. La «actualización en vivo» del juego dejó de funcionar.
- Objetivo de FRALEX: que lo que pasa en la **web Phoenix** (fichajes, stats) llegue al **juego de todos**, idealmente **sin reiniciar** y con un solo botón.

---

## 3. Etapa 1 · Entender el parche y el switcher
**Qué se hizo:** se estudió el «PES2021 Start.exe» de ConmeGOL.
**Qué se descubrió:**
- Es un programa en Python que, al elegir un modo, **copia con robocopy** la carpeta del modo encima del juego y abre Sider y el juego.
- **No usa internet.**
- Lo que pongamos en la raíz del juego **se borra** al volver a usar el switcher → lo nuestro debe vivir **dentro de la carpeta del modo** (`ConmeGol Extras\<modo>\SiderAddons`).

**Documento:** `sider/VINCULO-TIEMPO-REAL.md`.

---

## 4. Etapa 2 · Primer puente: avisos de la web en el juego
**Qué se hizo:** módulo `phoenix.lua` que lee un archivo de texto (`content\phoenix\avisos.txt`) y lo muestra en el overlay.
**Problema encontrado:** el Lua de Sider **no trae `pcall`** (la función que atrapa errores) → la v0.1 no cargaba. Se cambió por una «bandera» que cuenta errores y apaga el módulo tras 5 seguidos.
**Resultado (00:45):** ✅ el aviso llega al overlay en ~1 segundo, sin reiniciar.
**Después:** con Phoenix Link escribiendo ese archivo, un aviso escrito en la web («hola») llegó al juego. ✅ Cadena web → Link → juego completa.

---

## 5. Etapa 3 · Intentar cambiar stats en la memoria (y aprender del fallo)
**Idea:** buscar la ficha de Lamine Yamal en la memoria y cambiar su Velocidad.
- **v0.3:** la búsqueda **cerró el juego** (leyó una zona de memoria que otro programa, ReShade, acababa de liberar). Sin daño en datos.
- **v0.4–v0.5:** se cambió a **copia segura** (ReadProcessMemory): si la zona ya no existe, falla sin cerrar el juego. Se encontró la ficha y se confirmó un dato (Potencia de tiro).
- **v0.6–v0.8:** se escribió Velocidad 99 en memoria, pero **la pantalla seguía en 90**: el juego usa otras copias.

**Lección:** cambiar la memoria es frágil. **Mejor camino: los archivos.**

---

## 6. Etapa 4 · Stats por archivo: `Player.bin` en Phoenix-DB
**Qué se hizo:**
- Un «espía» en phoenix.lua (v0.9) anotó **cuándo lee el juego cada archivo** de la base: al arrancar y al **Editar → Cargar**.
- Se creó la carpeta **Phoenix-DB** con un `Player.bin` propio (copia del de olmos con Lamine cambiado) y se puso **antes** que la carpeta de olmos en `sider.ini`.

**Resultados:**
- 05:10 → Lamine con Velocidad 99 al reiniciar. ✅
- 05:30 → **Sin reiniciar**: cambiar el archivo + **Editar → Cargar** → el amistoso mostró el valor nuevo. ✅

**Regla:** nuestro `Player.bin` es **una copia del de olmos con nuestros cambios encima**. Si olmos se actualiza, hay que volver a fabricarlo.
**Documentos:** `base-conocimiento/05-PUERTA-EN-VIVO.md`, `MANUAL-PARCHE-PHOENIX.md`.

---

## 7. Etapa 5 · Fichajes por el option file
**Qué se hizo:** `OptionFile::mover` de Phoenix Sync (mueve un jugador de plantilla y corrige la alineación) y se probó en el juego.
**Resultado (04:50):** Lamine al **Real Madrid** (dorsal 26) con **Editar → Cargar**, sin reiniciar. ✅
**Prueba de la mañana:** cambiar `PlayerAssignment.bin` (opción en vivo desactivada) **no** movió a Lamine; cambiar el **option file** sí.

---

## 8. Etapa 6 · Recargar sin entrar a Editar
**Qué se hizo:** se estudió el código de `PES2021.exe` (sin cifrar en esa parte).
**Descubrimientos:**
- Un **interruptor escondido** de Konami (byte `exe+0x37F5C39`): si vale 1, al volver al menú principal el juego recarga el option file.
- La recarga del menú principal **no** incluía la base; Editar → Cargar sí. Se cambió **1 byte en memoria** (`exe+0xAEF78E`: 00 → 01) para que incluyera la base.

**Resultados:**
- Tecla L (v0.11): el juego recarga el option file. Lamine volvió al Barça. ✅
- Tecla P (v0.14): recarga **completa** (option file + base). ✅

**Seguridad:** antes de escribir, phoenix.lua **comprueba los bytes exactos** del juego. Si no coinciden (otra versión del exe u otro juego), **no toca nada**.

---

## 9. Etapa 7 · Revivir el botón nativo «Datos Actual. en vivo»
**Problema:** el botón intentaba iniciar sesión en Konami (servidor muerto) → error «servicios finalizados».
**Qué se hizo:**
- v0.15: un parche en memoria de 19 bytes que **salta el inicio de sesión** y enciende el interruptor. ✅ (05:46)
- v0.16 «en el sitio», con tres parches:
  - **A:** saltar el inicio de sesión;
  - **B:** saltar la descarga y mostrar el **mensaje de éxito** de Konami;
  - **C:** que la recarga del paso 26 incluya la base.

  Resultado (05:53): **mensaje nativo de Konami** y Lamine 90 → 99 **sin salir de la pantalla**. ✅
- v0.17: los parches se aplican **solos al arrancar** (sin tecla U).

**Documentos en Drive:** «🏆🏆🏆 El botón nativo… revive» y «🏆🏆🏆🏆 Botón nativo EN EL SITIO».

---

## 10. Etapa 8 · El misterio de las plantillas (resuelto)
**Síntoma:** con «Activar» llegaban las stats, pero **no los fichajes** del option file (Julián, Lamine). Y la plantilla del Atlético cambiaba sola.
**Pruebas decisivas (noche):**
1. Lamine al Real Madrid **solo en el option file** + Activar → **no apareció**.
2. Mbappé al Barça + Editar → Cargar (opción desactivada) → **apareció**.
3. **Vinícius al Barça solo en la base** (`PlayerAssignment.bin`) + Activar → **apareció**. ✅

**Regla final:**

| «Datos Actual. en vivo» | Equipos salen de… | Stats salen de… | Se recarga con… |
|---|---|---|---|
| **Activado** | la **base** (`PlayerAssignment.bin`) | `Player.bin` | **Activar** |
| Desactivado | el **option file** | `Player.bin` | Editar → Cargar |

**Por qué:** así funcionaba la actualización en vivo de Konami: con la opción activada, el juego usa los equipos «oficiales» de la base.
**Explica también:** el cambio raro del Atlético venía de un `PlayerAssignment.bin` de prueba.

---

## 11. Etapa 9 · Phoenix Sync: fichajes compartidos entre PCs
**Encargo de FRALEX:** «si yo hago un cambio en mi option file, que también cambie en el de mi amigo, y al revés».
**Cómo se diseñó:**
1. **Se comparte la operación, no el archivo.** Si dos personas fichan a la vez, se suman los dos fichajes. Si se copiara el archivo entero, uno pisaría al otro.
2. **Detectar el fichaje (origen):** Sync vigila el option file. Cuando se queda quieto 10 s, compara los equipos con la última «foto» y cada diferencia se vuelve una operación.
3. **Subir a la web:** con el token de Phoenix y reintentos (2, 4, 8… hasta 300 s).
4. **Recibir (destino):** baja las operaciones nuevas, hace **respaldo**, aplica cada una en el **option file** y en la **base**, y deja una **entrega** para Link.
5. **Link** coloca los archivos y avisa en el juego.

**Protecciones:**
- **Anti-bucle:** lo recibido nunca se vuelve a subir.
- **Choques:** dos fichajes del mismo jugador → «conflicto», no se pisa nada.
- **Idempotencia:** aplicar dos veces lo mismo no duplica.
- **Parche distinto:** «incompatible», no se toca nada (se compara una huella de la tabla de equipos).
- **Modo automático / con autorización:** lo decide solo el admin desde la web. Si la web falla → con autorización.
- **Respaldos completos** (carpeta save + Phoenix-DB) antes de cada cambio, uno diario y manual. Se conservan 10 + uno por día de los últimos 7. Se puede restaurar y deshacer.
- **Multiparche:** busca Documentos normal y OneDrive, cualquier edición, y los modos del switcher.

**Fallo real encontrado en las pruebas:** dos guardados en el mismo segundo no se detectaban → arreglado (fecha precisa + revisión cada 60 s).

**Pruebas:** 158 nuevas + 211 antiguas, 0 errores, con **dos PCs simuladas** (ida, vuelta, choque, autorización, rechazo, web caída, deshacer, parche distinto, base). Compila para Windows.

**Web:** contrato v1.8.0 publicado por el chat WEB (`/v1/sync/config`, `/v1/sync/operaciones`, `…/aplicada`); Sync ajustado a sus nombres (`op_id`, `base_seq`, `parche`, `huella_bd`).

---

## 12. Cómo debe funcionar al final (el viaje de un fichaje)
1. El amigo ficha a un jugador en Editar y guarda.
2. Su Sync lo detecta y lo sube a la web.
3. El Sync de FRALEX lo baja, hace respaldo y lo escribe en su option file y su base.
4. El Link de FRALEX coloca los archivos y avisa: «⚡ Datos nuevos de Phoenix… Activar».
5. FRALEX pulsa **Partido → Datos Actual. en vivo → Activar**.
6. El jugador aparece en su equipo nuevo. **Sin Editar y sin reiniciar.** Al revés funciona igual.

---

## 13. Qué falta (en orden)
1. **Link:** aceptar `PlayerAssignment.bin` (prompt `PhoenixSync/prompts/PROMPT-LINK-playerassignment.md`).
2. **Sync (con aprobación de FRALEX):** que los fichajes propios también vayan a la base propia. Si no, el que ficha no ve su propio fichaje con Activar.
3. **Web:** confirmar rutas publicadas, correr el SQL y crear el grupo (FRALEX + amigo).
4. **Instalación** en la PC del amigo y **empezar con los mismos archivos** (option file, `Player.bin`, `PlayerAssignment.bin`).
5. **Activar** `sync-base si` en cada PC.
6. **Prueba real** con dos PCs.
7. Después: botones de Sync dentro de Link y una **phoenix.lua v0.18** limpia (quitar las teclas de prueba B/K/L/P y arreglar Shift+R).

---

## 14. Errores y lecciones del camino
- Leer la memoria directamente **cerró el juego** → siempre copia segura.
- Escribir stats en memoria **no se veía** → mejor por archivo.
- Una copia de phoenix.lua **no llegó a la PC** y se perdió tiempo → comprobar siempre el sha256 después de copiar.
- Se creyó que el option file mandaba siempre → **depende de la opción en vivo**. Se resolvió con pruebas aisladas (cambiar una sola cosa cada vez).
- Lección general: **probar de a un cambio, con respaldo, y anotar todo**.

---

## 15. Reglas del proyecto
- Si algo puede romperse, mejor no hacerlo.
- Nada a `master` sin permiso de FRALEX. **Ningún cambio en Sync sin su aprobación.**
- Todo compila sin errores antes de subirse.
- Siempre respaldo antes de aplicar algo.
- Todo se documenta en GitHub y en Drive, como actualizaciones nuevas, sin sobrescribir.
