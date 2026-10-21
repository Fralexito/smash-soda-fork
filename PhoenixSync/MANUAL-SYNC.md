# Phoenix Sync · Manual completo: datos en vivo, fichajes compartidos, control total y parche Phoenix

Cómo funciona Phoenix Sync (antes «Mercado»), de punta a punta:
- el option file y los fichajes compartidos entre PCs;
- el botón nativo «Activar»;
- el control total desde la web;
- el manual y el prompt de traspaso del futuro Parche Phoenix Evolution.

**Estado al 21-oct:** rama `mercado-fase0`, `phoenix.lua` v0.18c. Falta probar los fichajes en un PES real con un amigo. **Base de datos provisional:** la Phoenix-DB es una copia de olmosjr23. El conocimiento del motor y de la Liga Máster está en `base-conocimiento/`.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 6 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · 📕 Informe completo — Cómo se logró: datos en vivo y fichajes compartidos en PES 2021](#parte-1-informe-completo-como-se-logro-datos-en-vivo-y-fichajes-compartidos-en-pes-2021) — origen `PhoenixSync/INFORME-COMPLETO-2026-10-09.md` (d8e9402 2026-10-09)
- [Parte 2 · Option file compartido entre PCs — informe (etapa 1, 2026-10-09)](#parte-2-option-file-compartido-entre-pcs-informe-etapa-1-2026-10-09) — origen `PhoenixSync/SYNC-COMPARTIDO.md` (fa26396 2026-10-09)
- [Parte 3 · Guía — Que todos vean los mismos fichajes con «Activar»](#parte-3-guia-que-todos-vean-los-mismos-fichajes-con-activar) — origen `PhoenixSync/GUIA-MISMOS-FICHAJES-ACTIVAR.md` (8f50132 2026-10-10)
- [Parte 4 · Control total de Phoenix: option file oficial, fichajes y stats](#parte-4-control-total-de-phoenix-option-file-oficial-fichajes-y-stats) — origen `PhoenixSync/PLAN-CONTROL-TOTAL.md` (e626317 2026-10-10)
- [Parte 5 · 📘 Manual del Parche Phoenix Evolution (PES 2021)](#parte-5-manual-del-parche-phoenix-evolution-pes-2021) — origen `PhoenixSync/MANUAL-PARCHE-PHOENIX.md` (9444d91 2026-10-09)
- [Parte 6 · Prompt de traspaso — Parche Phoenix Evolution (PES 2021)](#parte-6-prompt-de-traspaso-parche-phoenix-evolution-pes-2021) — origen `PhoenixSync/PROMPT-PARCHE-PHOENIX.md` (5bf26d4 2026-10-09)

---

## Parte 1 · 📕 Informe completo — Cómo se logró: datos en vivo y fichajes compartidos en PES 2021

> Origen: `PhoenixSync/INFORME-COMPLETO-2026-10-09.md` · último cambio d8e9402 2026-10-09 · 13058 bytes. Copia íntegra.

9 de octubre de 2026, 20:40 (Lima). Proyecto Phoenix (FRALEX). Frentes SYNC y LINK.
(Copia en Google Drive con el mismo título.)
Fuentes: rama `mercado-fase0` (PhoenixSync/REGISTRO.md, PRUEBAS.md, base-conocimiento/, SYNC-COMPARTIDO.md, GUIA-MISMOS-FICHAJES-ACTIVAR.md).

---

### 0. Resumen en 5 líneas
1. Se conectó la web y el juego a través de **Sider** (un «puente» de mods) y un módulo propio, **phoenix.lua**.
2. Se descubrió **qué archivo manda en cada cosa**: stats en `Player.bin`; equipos en el **option file** o en la **base** (`PlayerAssignment.bin`), según la opción en vivo.
3. Se **revivió el botón nativo de Konami** «Datos Actual. en vivo → Activar», apagado desde 2022, para cargar datos de Phoenix **sin reiniciar**.
4. Se construyó en **Phoenix Sync** el motor para **compartir fichajes entre PCs**, con respaldos, anti-bucle y protección contra choques.
5. Falta conectar las últimas piezas (Link, web) y la **prueba real con dos PCs**.

---

### 1. Glosario (palabras nuevas)
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

### 2. El punto de partida
- La comunidad (+50 jugadores) juega PES 2021 online por **Parsec**.
- Konami **apagó sus servidores** el 25/08/2022. La «actualización en vivo» del juego dejó de funcionar.
- Objetivo de FRALEX: que lo que pasa en la **web Phoenix** (fichajes, stats) llegue al **juego de todos**, idealmente **sin reiniciar** y con un solo botón.

---

### 3. Etapa 1 · Entender el parche y el switcher
**Qué se hizo:** se estudió el «PES2021 Start.exe» de ConmeGOL.
**Qué se descubrió:**
- Es un programa en Python que, al elegir un modo, **copia con robocopy** la carpeta del modo encima del juego y abre Sider y el juego.
- **No usa internet.**
- Lo que pongamos en la raíz del juego **se borra** al volver a usar el switcher → lo nuestro debe vivir **dentro de la carpeta del modo** (`ConmeGol Extras\<modo>\SiderAddons`).

**Documento:** `sider/VINCULO-TIEMPO-REAL.md`.

---

### 4. Etapa 2 · Primer puente: avisos de la web en el juego
**Qué se hizo:** módulo `phoenix.lua` que lee un archivo de texto (`content\phoenix\avisos.txt`) y lo muestra en el overlay.
**Problema encontrado:** el Lua de Sider **no trae `pcall`** (la función que atrapa errores) → la v0.1 no cargaba. Se cambió por una «bandera» que cuenta errores y apaga el módulo tras 5 seguidos.
**Resultado (00:45):** ✅ el aviso llega al overlay en ~1 segundo, sin reiniciar.
**Después:** con Phoenix Link escribiendo ese archivo, un aviso escrito en la web («hola») llegó al juego. ✅ Cadena web → Link → juego completa.

---

### 5. Etapa 3 · Intentar cambiar stats en la memoria (y aprender del fallo)
**Idea:** buscar la ficha de Lamine Yamal en la memoria y cambiar su Velocidad.
- **v0.3:** la búsqueda **cerró el juego** (leyó una zona de memoria que otro programa, ReShade, acababa de liberar). Sin daño en datos.
- **v0.4–v0.5:** se cambió a **copia segura** (ReadProcessMemory): si la zona ya no existe, falla sin cerrar el juego. Se encontró la ficha y se confirmó un dato (Potencia de tiro).
- **v0.6–v0.8:** se escribió Velocidad 99 en memoria, pero **la pantalla seguía en 90**: el juego usa otras copias.

**Lección:** cambiar la memoria es frágil. **Mejor camino: los archivos.**

---

### 6. Etapa 4 · Stats por archivo: `Player.bin` en Phoenix-DB
**Qué se hizo:**
- Un «espía» en phoenix.lua (v0.9) anotó **cuándo lee el juego cada archivo** de la base: al arrancar y al **Editar → Cargar**.
- Se creó la carpeta **Phoenix-DB** con un `Player.bin` propio (copia del de olmos con Lamine cambiado) y se puso **antes** que la carpeta de olmos en `sider.ini`.

**Resultados:**
- 05:10 → Lamine con Velocidad 99 al reiniciar. ✅
- 05:30 → **Sin reiniciar**: cambiar el archivo + **Editar → Cargar** → el amistoso mostró el valor nuevo. ✅

**Regla:** nuestro `Player.bin` es **una copia del de olmos con nuestros cambios encima**. Si olmos se actualiza, hay que volver a fabricarlo.
**Documentos:** `base-conocimiento/05-PUERTA-EN-VIVO.md`, `MANUAL-PARCHE-PHOENIX.md`.

---

### 7. Etapa 5 · Fichajes por el option file
**Qué se hizo:** `OptionFile::mover` de Phoenix Sync (mueve un jugador de plantilla y corrige la alineación) y se probó en el juego.
**Resultado (04:50):** Lamine al **Real Madrid** (dorsal 26) con **Editar → Cargar**, sin reiniciar. ✅
**Prueba de la mañana:** cambiar `PlayerAssignment.bin` (opción en vivo desactivada) **no** movió a Lamine; cambiar el **option file** sí.

---

### 8. Etapa 6 · Recargar sin entrar a Editar
**Qué se hizo:** se estudió el código de `PES2021.exe` (sin cifrar en esa parte).
**Descubrimientos:**
- Un **interruptor escondido** de Konami (byte `exe+0x37F5C39`): si vale 1, al volver al menú principal el juego recarga el option file.
- La recarga del menú principal **no** incluía la base; Editar → Cargar sí. Se cambió **1 byte en memoria** (`exe+0xAEF78E`: 00 → 01) para que incluyera la base.

**Resultados:**
- Tecla L (v0.11): el juego recarga el option file. Lamine volvió al Barça. ✅
- Tecla P (v0.14): recarga **completa** (option file + base). ✅

**Seguridad:** antes de escribir, phoenix.lua **comprueba los bytes exactos** del juego. Si no coinciden (otra versión del exe u otro juego), **no toca nada**.

---

### 9. Etapa 7 · Revivir el botón nativo «Datos Actual. en vivo»
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

### 10. Etapa 8 · El misterio de las plantillas (resuelto)
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

### 11. Etapa 9 · Phoenix Sync: fichajes compartidos entre PCs
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

### 12. Cómo debe funcionar al final (el viaje de un fichaje)
1. El amigo ficha a un jugador en Editar y guarda.
2. Su Sync lo detecta y lo sube a la web.
3. El Sync de FRALEX lo baja, hace respaldo y lo escribe en su option file y su base.
4. El Link de FRALEX coloca los archivos y avisa: «⚡ Datos nuevos de Phoenix… Activar».
5. FRALEX pulsa **Partido → Datos Actual. en vivo → Activar**.
6. El jugador aparece en su equipo nuevo. **Sin Editar y sin reiniciar.** Al revés funciona igual.

---

### 13. Qué falta (en orden)
1. **Link:** aceptar `PlayerAssignment.bin` (prompt `PhoenixSync/prompts/PROMPT-LINK-playerassignment.md`).
2. **Sync (con aprobación de FRALEX):** que los fichajes propios también vayan a la base propia. Si no, el que ficha no ve su propio fichaje con Activar.
3. **Web:** confirmar rutas publicadas, correr el SQL y crear el grupo (FRALEX + amigo).
4. **Instalación** en la PC del amigo y **empezar con los mismos archivos** (option file, `Player.bin`, `PlayerAssignment.bin`).
5. **Activar** `sync-base si` en cada PC.
6. **Prueba real** con dos PCs.
7. Después: botones de Sync dentro de Link y una **phoenix.lua v0.18** limpia (quitar las teclas de prueba B/K/L/P y arreglar Shift+R).

---

### 14. Errores y lecciones del camino
- Leer la memoria directamente **cerró el juego** → siempre copia segura.
- Escribir stats en memoria **no se veía** → mejor por archivo.
- Una copia de phoenix.lua **no llegó a la PC** y se perdió tiempo → comprobar siempre el sha256 después de copiar.
- Se creyó que el option file mandaba siempre → **depende de la opción en vivo**. Se resolvió con pruebas aisladas (cambiar una sola cosa cada vez).
- Lección general: **probar de a un cambio, con respaldo, y anotar todo**.

---

### 15. Reglas del proyecto
- Si algo puede romperse, mejor no hacerlo.
- Nada a `master` sin permiso de FRALEX. **Ningún cambio en Sync sin su aprobación.**
- Todo compila sin errores antes de subirse.
- Siempre respaldo antes de aplicar algo.
- Todo se documenta en GitHub y en Drive, como actualizaciones nuevas, sin sobrescribir.

---

## Parte 2 · Option file compartido entre PCs — informe (etapa 1, 2026-10-09)

> Origen: `PhoenixSync/SYNC-COMPARTIDO.md` · último cambio fa26396 2026-10-09 · 4397 bytes. Copia íntegra.

### Qué hace (en palabras simples)
- **Fichaje = «operación»**: un mensaje pequeño («jugador 126624 del equipo 172 al 108»). Se comparte eso, no el archivo entero. Así, si FRALEX ficha a X y el amigo ficha a Y a la vez, **se suman los dos**.
- **Va en los dos sentidos**: cualquier PC del grupo publica y cualquier otra recibe.
- **Origen** (quien ficha): Sync mira el `EDIT00000000`; cuando deja de cambiar 10 s, compara las plantillas con la última «foto» y sube cada diferencia.
- **Destino** (quien recibe): baja las operaciones nuevas, hace **respaldo**, las aplica en una copia de SU option file y la deja en `entrega\` para que **Phoenix Link** la coloque y avise en el juego.
- **Modo** (automático / con autorización): lo decide **solo el admin desde la web**. Si la web no responde → se actúa **con autorización** (lo seguro).

### Protecciones
| Riesgo | Qué hace Sync |
|---|---|
| Bucle (ping-pong) | Lo recibido nunca se vuelve a subir: ids propios/aplicados y huellas (sha256) de los archivos que llegaron de fuera. Probado con dos PCs simuladas. |
| Dos fichajes del mismo jugador a la vez | «conflicto»: no se pisa nada, gana el que ya estaba; queda en el historial y avisa «1 cambio omitido: conflicto». |
| Aplicar dos veces | Idempotente: si el jugador ya está en el destino, no hace nada. |
| Parche distinto | Cada operación lleva el parche y la **huella de la tabla de equipos**. Si no coincide: «incompatible», no se toca nada. |
| Archivo a medias | Espera a que quede quieto y que se pueda leer entero. |
| Web caída / rutas que aún no existen (404) | Espera (2, 4, 8… hasta 300 s; 404 → 5 min), no se rompe, modo autorización. |
| Respaldo imposible (sin espacio…) | **No aplica** y avisa «no se pudo respaldar: …». |

### Respaldos (en cada PC)
`%APPDATA%\Phoenix Mercado\respaldos\<AAAA-MM-DD_HHMMSS>\` con `save\` (la carpeta save completa), `phoenix-db\<modo>\Player.bin` y `manifiesto.json` (ruta original, tamaño, sha256, motivo). Antes de cada aplicación, uno diario y el manual. Retención: 10 últimos + el primero de cada uno de los últimos 7 días. Nunca toca los respaldos de Link.

### Archivos
- `core/RutasJuego` (multiparche: Documentos normal y OneDrive, cualquier edición/steamid, cualquier modo del switcher con Phoenix-DB)
- `core/Respaldos`, `core/Entrega` (formato exacto de Link), `core/Grupo` (operaciones, choques, estado), `core/ClienteSync` (web), `core/SyncCompartido` (motor)
- `app/ComandosSync.cpp` (comandos `sync-…` de consola), `windows/Plataforma` (Documentos y PES2021.exe abierto)
- Pruebas: `pruebas/compartido.cpp` → `PhoenixSyncPruebasCompartido.exe`

### Pendiente
1. **WEB**: publicar §26 (`prompts/PROMPT-WEB-sync-compartido.md`, con SQL).
2. Prueba real con dos PCs (sección 7 del encargo).
3. Interfaz: hoy son comandos de consola; luego, tarjeta en la sección «Phoenix Sync» de Link (prompt al chat LINK).
4. **Qué archivo manda en las plantillas (prueba de FRALEX, mañana del 2026-10-09):** cambiar `PlayerAssignment.bin` (Lamine → Real Madrid) NO movió a Lamine; cambiar el **option file** SÍ. ⇒ Sync aplica los fichajes al option file (correcto). Queda una duda abierta: la nota de las 06:54 (PRUEBAS.md) vio plantillas de la base al usar «Datos Actual. en vivo». Hay que repetir la prueba de fichaje usando ese botón para confirmar que también ahí manda el option file.

### Actualización 2026-10-09 20:30 · Fichajes con «Activar» (PlayerAssignment.bin)
- Confirmado por FRALEX: con «Activar» los equipos salen de `PlayerAssignment.bin` de Phoenix-DB.
- Sync ya escribe cada fichaje recibido **también en `PlayerAssignment.bin`** (`core/AsignacionBase`): mueve el registro en su sitio (mismo índice), el jugador va al final del destino, dorsal pedido o el más alto libre, los de detrás en el origen suben un puesto, sin capitanía. «Quitar» (dejar libre) aún no se aplica en la base.
- Se activa en cada PC con `PhoenixSync sync-base si` **solo cuando Link acepte ese archivo** (prompt `prompts/PROMPT-LINK-playerassignment.md`). Sin activar, todo sigue como antes.
- Respaldos guardan también `PlayerAssignment.bin`; «deshacer» lo devuelve.
- Pruebas: 158/158 (incluye el flujo completo con la base) + 211/211. Compila para Windows.

---

## Parte 3 · Guía — Que todos vean los mismos fichajes con «Activar»

> Origen: `PhoenixSync/GUIA-MISMOS-FICHAJES-ACTIVAR.md` · último cambio 8f50132 2026-10-10 · 7245 bytes. Copia íntegra.

9 de octubre de 2026, 20:30 (Lima). Para FRALEX y los jugadores del grupo. Frente SYNC.
(Copia en Google Drive: «📗 Guía — Que todos vean los mismos fichajes con «Activar» (9 oct 2026, 20:30)».)

---

### 1. La idea en una frase
Cuando alguien del grupo ficha a un jugador, **todos deben verlo en su juego al pulsar «Activar»**, sin Editar y sin reiniciar.

### 2. Palabras que se usan en esta guía
- **Option file** (`EDIT00000000`): archivo en `Documentos\KONAMI\…\save\`. Guarda los equipos cuando la opción en vivo está **desactivada**.
- **Base** (`PlayerAssignment.bin`): archivo en la carpeta **Phoenix-DB** del parche. Guarda los equipos cuando la opción en vivo está **activada**.
- **Player.bin**: archivo de Phoenix-DB con las **stats** (velocidad, regate…). Se usa siempre.
- **«Activar»**: botón del juego **Partido → Datos Actual. en vivo → Activar**. Phoenix lo revivió (phoenix.lua v0.17).
- **Phoenix Sync**: programa que detecta, sube y recibe los fichajes del grupo.
- **Phoenix Link**: programa que coloca los archivos en el juego y avisa en pantalla.
- **Entrega**: paquete de archivos que Sync deja listo para que Link lo coloque.

### 3. Lo que descubrimos (probado en la PC de FRALEX, 2026-10-09)

| «Datos Actual. en vivo» | El juego toma los equipos de… | Las stats salen de… | Cómo se recarga |
|---|---|---|---|
| **Activado** | la **base** (`PlayerAssignment.bin`) | `Player.bin` | **Activar** |
| Desactivado | el **option file** | `Player.bin` | Editar → Cargar |

Pruebas:
1. Lamine al Real Madrid **solo en el option file** + Activar → **no apareció**.
2. Vinícius al Barça **solo en la base** + Activar → **apareció**. ✅
3. Mbappé al Barça en el option file + Editar → Cargar (opción desactivada) → **apareció**. ✅

**Conclusión:** para fichar con «Activar», cada fichaje debe estar en la **base** de cada PC.

### 4. Cómo viaja un fichaje (cuando todo esté listo)
1. **El jugador A ficha** desde Editar → su option file cambia.
2. **Sync de A** lo detecta y sube a la web un mensaje corto: «jugador X del equipo 1 al 2».
3. **Sync de A** lo copia también a **su propia base** → A lo ve con Activar. *(⏳ pendiente de aprobación)*
4. **Sync de B** baja el mensaje, hace **respaldo**, lo escribe en su option file **y en su base**. *(✅ hecho)*
5. **Link de B** coloca los archivos y avisa: «⚡ Datos nuevos de Phoenix… Activar». *(⏳ falta que Link acepte la base)*
6. **B pulsa Activar** → ve el fichaje. **A y B ven lo mismo.**

### 5. Estado de cada pieza

*(Actualizado el 10 oct 2026.)*

| Pieza | Estado |
|---|---|
| Sync: detectar, subir y recibir fichajes | ✅ Hecho; exe recompilado el 10 oct con los comandos sync-* (381/381 pruebas) |
| Sync: escribir fichajes recibidos en la base | ✅ Hecho; **encendido en la PC de FRALEX** (`sync-base si`, aprobado 10 oct: «quiero que sea automático cada fichaje») |
| Sync: escribir **mis propios** fichajes en **mi** base | ✅ Hecho (10 oct, aprobado por FRALEX); va con `sync-base si`, haya grupo o no |
| Sync: aviso `recargar.txt` al colocar Link una entrega con base | ✅ Hecho (10 oct): en cada `<SiderAddons>\content\phoenix` que exista. Con el modo AUTO-FICHAJES de Link el juego recarga solo |
| Link: aceptar `PlayerAssignment.bin` | ✅ Hecho en Link (10 oct) |
| Web: rutas /v1/sync (config, operaciones, aplicada) | ✅ Publicadas (responden 401 sin token) |
| Web: `GET /v1/sync/grupos` y grupo FRALEX + amigo en `automatico` | ⏳ PENDIENTE-WEB: `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` |
| phoenix.lua v0.18 (botón Activar, modo ACTIVAR) | ✅ Instalada y cargada en el juego de FRALEX (sider.log 11:59) |
| Prueba real entre dos PCs | ⏳ Pendiente |

### 6. Guía paso a paso

#### Paso 1 · Empezar todos iguales (muy importante)
1. FRALEX copia de su PC:
   - `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<steamid>\save\EDIT00000000`
   - `…\Phoenix-DB\common\etc\pesdb\Player.bin`
   - `…\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin`
2. El amigo **guarda una copia de los suyos** y pone los de FRALEX en las mismas rutas (si hay dos carpetas Phoenix-DB, en las dos).
3. Los dos deben usar **el mismo parche** (hoy ConmeGOL Patch 26). Si es distinto, Sync marca los fichajes como «incompatible».

#### Paso 2 · Instalar las piezas en cada PC
1. **Phoenix Link** actualizado (el que recoge entregas), vinculado a la cuenta Phoenix.
2. **PhoenixSync.exe**.
3. **phoenix.lua v0.17** en la carpeta del modo del parche, con su línea en `sider.ini`.

#### Paso 3 · Configurar el juego
- Los dos con **«Datos Actual. en vivo» activado**.
- En el overlay (Espacio), Phoenix debe decir **«botón nativo EN EL SITIO ✓»**.

#### Paso 4 · Configurar Sync (una vez por PC)
```
PhoenixSync sync-juego "C:\ruta\de\la\carpeta\del\juego"
PhoenixSync sync-grupo <id del grupo que da la web>
```
Cuando Link ya acepte la base (Paso 6):
```
PhoenixSync sync-base si
```

#### Paso 5 · Jugar con Sync encendido
```
PhoenixSync sync-vigilar 180
```
(180 = minutos vigilando). Si se cierra la ventana, Sync deja de vigilar.

#### Paso 6 · Lo que falta construir (en este orden)
1. **Chat LINK:** pegar `prompts/PROMPT-LINK-playerassignment.md` para que Link acepte `PlayerAssignment.bin`.
2. **Chat SYNC (con aprobación de FRALEX):** que mis propios fichajes también vayan a mi base.
3. **Chat WEB:** confirmar rutas publicadas, correr el SQL y crear el grupo con FRALEX y su amigo, en modo **automático**.

#### Paso 7 · La prueba real
1. El amigo ficha a un jugador desde Editar y guarda.
2. En unos segundos a FRALEX le llega el aviso en el overlay.
3. FRALEX pulsa **Activar** → el jugador aparece en su equipo nuevo.
4. Al revés: FRALEX ficha y el amigo lo ve con Activar.
5. Choque: los dos fichan al mismo jugador a la vez → «conflicto», no se pisa nada.

### 7. Qué puede salir mal y qué hacer

| Problema | Causa probable | Qué hacer |
|---|---|---|
| El fichaje no aparece con Activar | Falta en la base (Link sin actualizar o `sync-base` apagado) | Revisar Paso 6.1 y `sync-base si` |
| Yo no veo mi propio fichaje | Falta el arreglo del Paso 6.2 | Mientras tanto, Editar → Cargar |
| Sale «incompatible» | Parches distintos | Usar el mismo parche (Paso 1) |
| Sale «conflicto» | Los dos movieron al mismo jugador | Gana el que llegó primero; se revisa a mano |
| «Link rechazó la entrega» | Archivo no permitido o tamaño distinto | Ver el historial de Link |
| Algo se rompió | — | `PhoenixSync sync-deshacer` o `sync-restaurar <respaldo>` |

### 8. Otra opción que funciona hoy, sin cambios
Los dos con la opción **desactivada** y cargando con **Editar → Cargar**. Manda el option file, que Sync ya sincroniza. Funciona, pero son más pasos.

### 9. Reglas
- Ningún cambio en Sync sin aprobación de FRALEX.
- Siempre hay respaldo antes de aplicar algo (`%APPDATA%\Phoenix Mercado\respaldos\`).
- Los archivos de la base se fabrican siempre a partir de los de olmos. Cuando olmos se actualiza, hay que volver a fabricarlos.

---

## Parte 4 · Control total de Phoenix: option file oficial, fichajes y stats

> Origen: `PhoenixSync/PLAN-CONTROL-TOTAL.md` · último cambio e626317 2026-10-10 · 6787 bytes. Copia íntegra.

10 de octubre de 2026 · frente SYNC · para FRALEX y para cualquier IA que siga el trabajo.
Pedido de FRALEX: «un option file oficial en mi web; cuando alguien modifique algo, se modifica ese y se descarga automáticamente en todos» y «acceso total a modificar los fichajes y las estadísticas de los jugadores, ya sea en Liga Máster o en lo normal».

### 1. Cómo funciona (en simple)

```
                ┌──────────────── WEB (Supabase) ────────────────┐
                │ Foto oficial vN (option file + base)           │
                │ Cambios del grupo: mover · quitar · stats      │
                │ Panel «Control total» (solo admin)             │
                └───────▲────────────────────────────┬───────────┘
   admin publica la foto│                            │ cada PC baja lo nuevo
   (sync-publicar-oficial)                         ▼
         PC de FRALEX ──► Phoenix Sync ──► respaldo ──► Phoenix Link coloca ──► phoenix.lua recarga
                                                                        (al volver al menú y entrar a un modo)
```

- **Foto oficial:** es el punto de partida común. La publica el admin y las demás PCs la bajan solas (la primera vez siempre; después, en modo automático).
- **Cambios del día a día:** viajan como mensajes cortos (unos cientos de bytes, no 11 MB).
- **Admin = acceso total:** desde el panel web (cuando exista) o desde su PC con Sync.

### 1.1 La base de datos es PROVISIONAL

- **Hoy:** Phoenix-DB es una copia **provisional** de la base de olmosjr23 (ConmeGOL 26), con etiqueta `provisional-olmosjr23` (`rutas::kBaseProvisional`).
- **Mañana:** será la base **oficial** del Phoenix Evolution Patch, hecha desde la de Konami, con IDs propios (ver «Ruta de planificación v1»).
- **Camino ya preparado (10 oct):**
  1. Cada foto oficial y cada cambio llevan `base_datos`.
  2. Para cambiar: deja la base oficial en tu Phoenix-DB y ejecuta `.\PhoenixSync sync-publicar-oficial --base phoenix-patch-26-v0.1`.
  3. Cada PC baja la foto, comprueba que trae Player.bin y PlayerAssignment.bin, hace respaldo y descarta lo pendiente de la base vieja.
  4. Los cambios con otra base se marcan `incompatible`, así nunca se aplica un ID viejo sobre la base nueva.
- Si la base oficial vive en otra carpeta, se cambia solo `rutas::kCarpetaBase` (hoy `Phoenix-DB`).
- El editor de stats (`FichasBase`) vale para cualquier base: los bits de Player.bin son del motor del juego, no del parche.
- **No hay que reescribir nada al cambiar de base.** Solo hay que regenerar los catálogos de jugadores y equipos de la web con la base nueva.

### 2. Estado de cada pieza

| Pieza | Estado | Dónde |
|---|---|---|
| Fichajes `mover` / `quitar` entre PCs | ✅ Hecho y probado (simulación de 2 PCs) | `core/SyncCompartido.cpp` |
| Mis fichajes también en mi base (con «Activar») | ✅ Hecho (10 oct) | `entregarMiBase` |
| Aviso de recarga `recargar.txt` + phoenix.lua v0.18b automática | ✅ Hecho; v0.18b instalada | `avisarRecarga`, `sider/pruebas/phoenix-v0.18b-prueba.lua` |
| **Stats en Player.bin** (20 de campo, 5 de portero, pie malo, forma, lesiones, 13 posiciones) | ✅ Hecho (10 oct). Lectura real de Lamine = pantalla. Falta verlo en el juego tras un cambio | `core/FichasBase.*`, `sync-ficha`, `sync-stats` |
| **Operación de grupo `stats`** (respaldo → entrega → recarga; deshacer) | ✅ Hecho y probado (simulación) | `aplicarStats` |
| **Foto oficial**: publicar (admin) y bajar sola | ✅ Hecho y probado con una web simulada | `publicarOficial`, `ponerOficial`, `sync-oficial` |
| Cambios creados en el panel web | ✅ Sync los acepta si traen la huella de la foto oficial | `motivoIncompatible` |
| Web: rutas `/v1/sync/oficial`, bucket, tipo `stats` solo para el admin, panel «Control total» | ⏳ **PENDIENTE-WEB** | `prompts/PROMPT-WEB-oficial-y-control-total.md` |
| Web: grupo FRALEX + amigo en automático y `GET /v1/sync/grupos` | ⏳ **PENDIENTE-WEB** | `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` |
| Link: «Automático: siempre» por defecto e instalador con v0.18b | ⏳ PENDIENTE-LINK | `prompts/PROMPT-LINK-modo-por-defecto.md` |
| **Liga Máster · fichajes** (vender, fichar para ti, IA ↔ IA, dinero, valor) | ✅ Probado en el juego (guías 08–09); se aplica desde la web firmada con `sincronizar --ml` | `core/LigaMaster.*`, `core/Sincronizacion.*` |
| **Liga Máster · stats** (±1 con las barras de crecimiento) | 🟡 Método probado en el juego (guías 24–29, herramienta `palanca`); **falta programarlo dentro de Sync** | §4 |

### 3. Comandos nuevos (PowerShell, en la carpeta de PhoenixSync.exe)

```
.\PhoenixSync sync-campos                       # lista de stats y rangos
.\PhoenixSync sync-ficha 162114                 # stats actuales de un jugador (162114 = Lamine)
.\PhoenixSync sync-stats 162114 velocidad 95    # cambia una stat aquí y, si hay grupo, la publica (solo admin)
.\PhoenixSync sync-publicar-oficial             # (admin) tu option file + base = foto oficial
.\PhoenixSync sync-oficial                      # baja la foto oficial si hay una nueva (--forzar = aunque sea la misma)
.\PhoenixSync sync-deshacer                     # deshace lo último (fichajes, stats o foto oficial)
```

Reglas que no cambian: respaldo siempre antes, archivos nuevos (nunca encima), comprobación de huella, todo o nada, y los cambios solo los coloca Link.

### 4. Lo que falta (en orden)

1. **WEB:** pegar `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` y después `prompts/PROMPT-WEB-oficial-y-control-total.md` en el chat WEB.
2. **Prueba en el juego (FRALEX, 5 minutos):** `sync-stats 162114 velocidad 95` → vuelve al menú → Partido → ficha de Lamine: ¿95? Después `sync-deshacer`.
3. **LINK:** pegar `prompts/PROMPT-LINK-modo-por-defecto.md`.
4. **Liga Máster · stats en Sync** (cuando FRALEX lo apruebe):
   - operación `lm-stats` `{ jugador_id, campo, direccion }`;
   - con el juego cerrado o en una ranura que no esté cargada, buscar la tabla D por anclas (guía 29 §4.3);
   - barra del campo = 9.999 (sube) o 0 (baja);
   - guardar en un archivo nuevo y comprobar que solo cambian 2 bytes por barra;
   - prueba en una ranura de prueba antes de la carrera real.

   Es el mismo método de `herramientas/palanca`, ya probado en el juego.
5. **Liga Máster · panel web:** pestaña que crea cambios en `lm_*` para `/mercado/v1/liga/cambios` (ya firmados y probados).

---

## Parte 5 · 📘 Manual del Parche Phoenix Evolution (PES 2021)

> Origen: `PhoenixSync/MANUAL-PARCHE-PHOENIX.md` · último cambio 9444d91 2026-10-09 · 18819 bytes. Copia íntegra.

**Versión del manual:** 2026-10-09 (madrugada, Lima) · **Autor del proyecto:** FRALEX · Redactado con Claude.

**Para qué sirve:** reúne **todo lo aprendido y probado** para construir el **Phoenix Evolution Patch**: qué es cada pieza, cómo funciona el juego por dentro, las recetas paso a paso, los errores que no hay que repetir y la ruta para fabricarlo.

**Regla de lectura:** si algo de aquí choca con un documento más viejo, **manda este manual**. Lo más nuevo y detallado está en `base-conocimiento/05-PUERTA-EN-VIVO.md` y en el diario `PRUEBAS.md`.

**Leyenda:**
- ✅ **probado en el juego**;
- 🟢 **se puede**: mismo método ya probado, falta hacerlo;
- 🟡 **con investigación**: el camino es conocido;
- 🔴 **frontera**: nadie lo ha hecho;
- ❌ **no funciona**.

---

### 1. La visión

El **Phoenix Evolution Patch** quiere ser el parche de PES 2021 **más completo, estable y conectado** que exista: un solo sistema con todo el ecosistema de FRALEX.

| Pieza | Qué es | Dónde vive |
|---|---|---|
| **Web Phoenix Evolution** | Liga (Galaxy League), mercado de fichajes, perfiles, buzón de avisos | repo `Fralexito/phoenixevolution` (rama `borrador`) + Supabase |
| **Phoenix Link** | App de PC: salas para jugar a distancia por Parsec (fork de Smash Soda 7.0.3) y **cartero** web ⇄ juego | repo `Fralexito/smash-soda-fork`, rama `rediseño-phoenix-portal`, carpeta `SmashSoda/` |
| **Phoenix Sync** (antes «Phoenix Mercado») | Programa que **mete en el juego lo que dice la web**: option file, Liga Máster y **base de datos** | mismo repo, rama `mercado-fase0`, carpeta `PhoenixSync/` |
| **Módulo Sider `phoenix.lua`** | Código Phoenix **dentro del juego**: muestra avisos y espía lecturas | `PhoenixSync/sider/phoenix.lua` |
| **El parche** | Base de datos + contenido + `sider.ini` limpio + lanzador | por construir (ver §7) |

**El camino de los datos:**

```
WEB (Supabase) ⇄ PHOENIX LINK (cartero, internet) ⇄ archivos en la PC ⇄ SIDER (dentro del juego) ⇄ PES 2021
                                    ↑
                         PHOENIX SYNC (fabrica los archivos: option file, Liga Máster, base de datos)
```

**El juego nunca toca internet:** todo lo online pasa por Phoenix Link. Sider no tiene red, y eso lo hace más seguro.

---

### 2. Cómo funciona PES 2021 por dentro (lo esencial)

#### 2.1 Capas del juego
1. **El ejecutable** `PES2021.exe` (1.07.02 + Data Pack 7.0). Las reglas «cableadas» (tamaños de liga, IA, físicas) solo se cambian parcheando código. 🔴
2. **Paquetes CPK** listados en `DpFileList.bin` (carpeta `download`). Es el modelo clásico; lo usa Sudamerican.
3. **Sider** (`SiderAddons\sider.exe`, `sider.dll`, `sider.ini`). Entrega archivos sueltos al juego (**livecpk**) y corre módulos Lua. Lo usa ConmeGOL para casi todo.
   - **Gana la primera raíz** `cpk.root` que tenga el archivo pedido.
4. **Carpeta `save`**: `EDIT00000000` (option file, cifrado), `ML0000000N` (Liga Máster, ranura N = archivo N−1 en hexadecimal) y `SYSTEM00000000`.

#### 2.2 La base de datos (`common\etc\pesdb\*.bin`)
- Cada archivo va comprimido con cabecera **WESYS**: 16 bytes = `ff 10 81 57` + `"WESYS"` + tamaño comprimido u32 + tamaño real u32, seguidos del contenido en **zlib**.
  - **El juego acepta una recompresión distinta** (zlib nivel 9). ✅
- **Archivos principales:** `Player.bin` (fichas de 312 B, ID u32 en +8), `PlayerAssignment.bin` (16 B: jugador en +4, equipo en +8), `Team.bin` (1532 B, ID en +8), `Competition.bin` (36 B), `CompetitionEntry.bin` (12 B), `Coach.bin`, `Stadium.bin`, `Country.bin`, `Tactics*.bin`, `PlayerWeekly.bin` y `TeamWeekly.bin` (forma semanal), `Boots.bin`, `Glove.bin`, `Derby.bin`…
- **Límites del juego:** 750 equipos, 30.000 jugadores, 40 por plantilla. No cambiar el tamaño de una liga sin parchear el exe, porque cuelga la Liga Máster.

#### 2.3 Cuándo lee el juego la base (✅ medido con el espía `livecpk_read`, 2026-10-09)
| Momento | ¿Relee `pesdb`? |
|---|---|
| Al arrancar | Sí, todo, dos veces (arranque y «Datos del sistema: Cargando») |
| **Al entrar a Editar** | Sí (Player, PlayerAssignment, Team, Coach, Competition…), pero **en un borrador** que se descarta al salir |
| **Editar → Cargar** | Sí, **y lo aplica al juego** ✅ |
| Al entrar a la Liga Máster | Solo `InstallVersionPlayer.bin` |
| Al abrir pantallas de jugadores, armar un amistoso o cambiar «Datos Actual. en vivo» | No |

#### 2.4 Mapa de `Player.bin` (registro de 312 B; bits desde el inicio; 6 bits = valor − 40)
- **19 habilidades de campo** ✅:
  - ataque 370, control 281, regate 352, posesión estrecha 416;
  - pase raso 263, pase bombeado 402, finalización 396, cabezazo 288;
  - balón parado 250, efecto 332;
  - velocidad 306, aceleración 344, equilibrio 376, contacto físico 390, salto 294, resistencia 338;
  - defensa 275, recuperación 312, agresividad 384.
- **Potencia de tiro** 358 ✅ (calibrada con la pantalla del juego).
- **Físico y datos** ✅:
  - altura 216 (8 bits, +100);
  - peso 256 (7 bits, +30);
  - edad 408 (6 bits, +15);
  - posición principal 434 (4 bits);
  - nacionalidad 233 (9 bits → `Country.bin`).
- **Portero** ✅: 269 = Despejar, 300 = Cobertura, **320 = Reflejos**, **326 = Actitud de portero**, 364 = Atajar (confirmado con Joan García y Livakovic).
- **Posiciones jugables** (2 bits: 0 no, 1 B, 2 A) 🔎:
  - PT 350, DFC 468, LI 318, LD 474;
  - MCD 414, MC 456, MI 466, MD 460, MO 464;
  - EI 472, ED 476, SD 478, DC 470.
- **Pie malo** uso 454 / precisión 462, **forma (Regularidad)** 438, **lesiones** 458 ✅: lo que muestra el juego = valor guardado + 1. El juego **varía** estos bytes (54-57) al cargar.
- **Habilidades especiales y estilos COM**: bits 480–531, uno por habilidad. **496 = Patadón en largo (PT)** ✅. 502/522 = Pase largo portero / Parapenaltis (falta el orden). Resto ❓.
- **Nombres** dentro de la ficha (en mayúsculas en el byte 129 y otra vez en el 190; en formato normal en el 251).

**Imágenes:**
- minicaras: `livecpk\Logos\common\render\symbol\player\<ID>.dds`;
- escudos: `…\symbol\flag\e_<6 dígitos>_r_l.png`;
- caras 3D: carpeta `face\real\<ID>\`.

#### 2.5 La base viva de ConmeGOL
El juego **no** usa `CGP_database.cpk`. Usa la base que sirve Sider desde `SiderAddons\olmosjr23\Database` (5/9/2026). El catálogo de la web hay que regenerarlo desde ahí.

---

### 3. Las piezas que YA funcionan

| Capacidad | Estado | Cómo |
|---|---|---|
| Mover jugadores en el option file (alineación y dorsales correctos) | ✅ | `PhoenixSync.exe mover` / `sincronizar` |
| Liga Máster: vender, traspasos de la IA, **fichar para el usuario**, presupuesto y tope salarial | ✅ | Phoenix Sync, con el juego en el menú (el juego relee el guardado al cargar la ranura) |
| Cambios firmados desde la web (Ed25519), aplicados todo o nada | ✅ | `/liga/cambios` |
| **Avisos web → juego en vivo** (1-2 s) | ✅ | Web → Phoenix Link → `content\phoenix\avisos.txt` → `phoenix.lua` → overlay de Sider |
| **Base de datos propia servida por Sider** (stats cambiadas al arrancar) | ✅ | Raíz `livecpk\Phoenix-DB` antes de `olmosjr23\Database` |
| **Cambiar stats sin reiniciar** | ✅ | Archivo nuevo + **Editar → Cargar** |
| Leer la memoria del juego sin riesgo | ✅ | `ReadProcessMemory` sobre el propio proceso (copia segura) |
| Página «Vestuario» con la plantilla editable | ✅ prototipo | artifact con base de datos; los cambios quedan «pendientes» |
| 295/295 pruebas automáticas de Phoenix Sync | ✅ | `PhoenixSyncPruebas.exe` |

---

### 4. Recetas paso a paso

#### R1 · Cambiar stats de un jugador en vivo ✅
1. **Phoenix Sync** toma la base que sirve el parche (`olmosjr23\...\Player.bin`) y la descomprime.
2. Busca la ficha por ID (u32 en +8) y cambia **solo** los bits del campo, conservando los demás bits del byte.
3. Recomprime con WESYS + zlib. Verifica de ida y vuelta (descomprimir y comparar).
4. Escribe en `SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\Player.bin` de forma **atómica** (`.tmp` y luego renombrar), en la raíz del juego **y** en la carpeta del modo.
5. En el juego: **Editar → Cargar** (sin guardar).
6. El siguiente partido o pantalla nueva muestra el cambio.

**Comprobación:** el espía de `phoenix.lua` anota cada lectura de `player.bin` con una **huella** (suma cada 64 bytes del trozo leído). Compárala con la del archivo.

#### R2 · Mandar un aviso al juego ✅
- **Desde la web:** tarjeta «Mandar aviso a mi juego». Phoenix Link revisa cada 15 s con el juego abierto y escribe `avisos.txt`.
- **A mano:** escribir `SiderAddons\content\phoenix\avisos.txt` (UTF-8 sin BOM, máx. 14 líneas, de forma atómica).
- **En el juego:** barra espaciadora → tecla 1 hasta «PHOENIX EVOLUTION».

#### R3 · Instalar algo en un parche con «switcher» ✅
- El switcher de ConmeGOL (`PES2021 Start.exe`, Python con PyInstaller) hace `robocopy "ConmeGol Extras\<modo>" "<juego>" /E /IS /IT`, escribe `version_actual.txt`, abre `sider.exe` y, 2 s después, `PES2021.exe`.
- **Por eso todo va en `ConmeGol Extras\<modo>\SiderAddons`**; si no, se pierde. Lo que se cambia **en vivo** se escribe en la raíz del juego.

#### R4 · Probar un módulo Sider sin romper nada ✅
1. **Sintaxis:** `luajit -bl modulo.lua`.
2. **Simulación con los globales exactos de Sider 7.3.3:** assert, pairs, ipairs, tostring, tonumber, type, error, unpack, collectgarbage, table, string, math, os, io, fs, ffi, jit, bit, zlib, memory, match, audio, input, log. **No hay `pcall`.**
3. Memoria y ffi falsos para probar búsquedas y escrituras, incluida una zona que «desaparece» a mitad de la prueba.
4. **Instalar con respaldo** de `sider.ini` y **una sola línea** nueva. Comprobar con `diff`.
5. **Ver `sider.log`:** «Module (x) is NOT activated» explica cualquier fallo de arranque.

#### R5 · Día de actualización de un parche (ConmeGOL / Sudamerican)
1. **Antes:** copia de la carpeta `save`.
2. **Comparar con la foto del 8/10** (`_PhoenixMercado_prueba\_versiones\2026-10-08\`): base, DpFileList, Sider y option file.
3. **Regenerar `livecpk\Phoenix-DB`** desde la base nueva. **Si no, tapa los datos nuevos del parche.**
4. **Auditar** solo los módulos Sider nuevos o cambiados.
5. **Regenerar el catálogo** y subirlo a la web.
6. **Probar** y anotar en `PRUEBAS.md`, `parches/<parche>.md` y Drive.

#### R6 · Operaciones de Liga Máster ✅
- Siempre con el juego **en el menú** (la ranura sin cargar) y con copia de seguridad.
- El juego relee el guardado al entrar a la ranura.
- Detalle técnico: `liga-master/ESTRUCTURA-ML.md` y `base-conocimiento/01-MOTOR-PES2021.md`.

---

### 5. Reglas de oro técnicas (aprendidas a golpes)

1. **Nunca leer ni escribir memoria del juego directo.** Otros hilos (ReShade, DLSS) la liberan en cualquier momento: crash 0xC0000005 comprobado. Hay que usar `ReadProcessMemory` / `WriteProcessMemory`, que fallan sin cerrar el juego.
2. **Mejor archivo que memoria.** El juego rehace sus copias desde la base, así que escribir en memoria no sirve para las stats. El camino es **archivo + Editar → Cargar**.
3. **El Lua de Sider no trae `pcall`.** Un error en `init` deja el módulo apagado. Para contar errores se usa una «bandera».
4. **`ffi.cdef` no deja redeclarar.** Con Shift+R el Lua es el mismo, así que se usan nombres con versión (`phx06_*` + `__asm__("FunciónReal")`).
5. **Shift+R solo recarga módulos que ya estaban activos.**
6. **Buscar fichas por ID y verificar sus cualidades.** El juego varía bits (forma, lesión), así que una ficha nunca coincide byte a byte.
7. **El switcher pisa la raíz.** Lo permanente va en la carpeta del modo.
8. **La actualización en vivo de Konami ya no existe:** sus servidores cerraron el 25/08/2022.
9. **Respaldos, siempre.** Un cambio = una línea en `REGISTRO.md`. Cada prueba en el juego se anota en `PRUEBAS.md`, salga bien o mal.
10. **0 errores:** si algo falla, no se avanza hasta entenderlo y arreglarlo. «Si algo se rompe, mejor no lo hagas.»
11. **Contenido de terceros** (caras, kits, estadios, módulos) solo con **permiso y crédito**. Lo propio (base, programas, conexión) es de FRALEX. **Nunca se distribuye el juego.**

---

### 6. El universo que abrió la «puerta en vivo»

| Qué | Archivo | Estado |
|---|---|---|
| Stats y habilidades | `Player.bin` | ✅ |
| **Fichajes y traspasos en vivo** | **option file `EDIT00000000`** (`OptionFile::mover`) + Editar → Cargar. `PlayerAssignment.bin` NO sirve: el option file manda en plantillas | ✅ Probado (Lamine → Real Madrid) |
| Entrenadores | `Coach.bin` | 🟢 |
| Equipos | `Team.bin` | 🟢 |
| Ascensos, descensos, participantes de cada liga | `CompetitionEntry.bin` | 🟢 (ojo: los módulos CGP comprueban equipos por competición) |
| Estadios, botines y guantes | `Stadium.bin`, `Boots.bin`, `Glove.bin` | 🟢 |
| Forma semanal propia | `PlayerWeekly.bin` / `TeamWeekly.bin` | 🟡 |
| Habilidades en la Liga Máster | `Player.bin` (la LM no las guarda aparte) | 🟢 por confirmar en una carrera |

**Productos posibles:**
1. **Mercado real conectado:** un fichaje en la web o en la vida real aparece en el juego.
2. **Phoenix Weekly:** stats según el rendimiento real, con reglas fijas y aprobación de FRALEX. **Sin copiar notas de otros juegos.**
3. **Liga online justa:** todos con el mismo archivo, verificado por huella SHA-256.
4. **Eventos:** jugador de la semana, lesiones o suspensiones, homenajes.
5. **Liga Máster conectada** a la web.

---

### 7. Ruta para construir el parche (actualizada)

| Fase | Qué | Estado |
|---|---|---|
| **0. Terreno** | Instalación limpia de referencia (¿PES 2021 de Steam?), exe 1.07.02 + DP 7.0, Sider fijado, partido de referencia | ⏳ |
| **1. Núcleo** | Base de datos Phoenix (Liga 1 + Liga 2 + Copa; lo mejor de ConmeGOL y Sudamerican **con permiso**), option file oficial de la web, `sider.ini` limpio y auditado | ⏳ (la técnica de base propia ya está ✅) |
| **2. Herramientas** | Phoenix Sync fabrica Player/PlayerAssignment/… (WESYS), CPK y DpFileList; **Phoenix Doctor** (auditoría automática de Sider); **Auditor de parches**; lanzador sin `.bat` | 🟢 en parte |
| **3. Conexión** | Avisos ✅ → cambios de stats en vivo ✅ → **fichajes en vivo** 🟢 → resultado automático (`match.stats()` → `resultado.json` → Link → web) 🟢 → verificador del anfitrión 🟢 | En marcha |
| **4. Contenido** | Addons por liga (caras, kits, estadios) con permisos y créditos; menos archivos sueltos; texturas bien comprimidas | ⏳ |
| **5. Más allá** | Competiciones propias 🟡, estadísticas completas 🟡, **Liga Máster online asíncrona** 🔴 (la idea más original), crear jugadores 🟡, avisos con aspecto nativo 🟡 | Futuro |

**Inmediato (para la próxima sesión):**
1. ~~Fichaje en vivo~~ ✅ hecho (option file + Editar → Cargar). Siguiente: confirmar que stats y fichajes llegan a la Liga Máster.
2. Confirmar si los cambios de stats llegan a la Liga Máster.
3. Automatizar: Vestuario web → Phoenix Sync arma el archivo → Phoenix Link lo coloca → aviso «Editar → Cargar». Los prompts para LINK y WEB están por escribir.
4. Calibración: portero completo, pie malo, forma y lesiones ✅. Siguen las habilidades especiales (bits 480–531), una foto de «Habilid. 4/4» por jugador.
5. Actualización de ConmeGOL: aplicar la receta R5 y regenerar `Phoenix-DB`.

---

### 8. Futuro (deseos de FRALEX, guardados para no olvidarlos)

- **Crear jugadores que no existen** (juveniles, jugadores reales que faltan, jugadores de la liga).
  - **Hipótesis:** fichas nuevas en `Player.bin` (ID libre) + su fila en `PlayerAssignment.bin`.
  - **Falta investigar:** rango de IDs seguros, `PlayerDeleteList.bin`, `InstallVersionPlayer.bin` y el efecto en la Liga Máster.
- **Hacer faces:** FRALEX quiere aprender. Claude le dará el **ID exacto** de cada jugador y **la carpeta** (`face\real\<ID>\`, minicara `symbol\player\<ID>.dds`), revisará los nombres de archivo y lo instalará en una raíz Phoenix sin pisar las del parche.
- **Avisos con aspecto nativo:** imágenes de los menús generadas por Phoenix Link y servidas por Sider, en lugar del overlay.
- **Botones nativos en vez de Sider:** el overlay es solo de laboratorio. En la versión final todo se aplica solo, y el jugador usa la web, Phoenix Link y un único botón del juego (Editar → Cargar).
- **PES online propio:** Phoenix Link (Parsec) + verificador + resultado automático. Un servidor propio estilo PESBUL es 🔴 (meses o años).
- **Liga Máster online asíncrona** 🔴: cada amigo con su club y su carrera, compartiendo mercado y tabla en la web.

---

### 9. Dónde está todo

| Qué | Dónde |
|---|---|
| Este manual | `PhoenixSync/MANUAL-PARCHE-PHOENIX.md` (y en Drive) |
| Relato de cómo se llegó aquí (aciertos y errores) | `PhoenixSync/base-conocimiento/06-RUTA-DEL-DESCUBRIMIENTO.md` |
| Base de conocimiento | `PhoenixSync/base-conocimiento/` (00-06, `parches/`, `investigacion/`, `datos/`) + PDF `base-conocimiento-PES21.pdf` |
| Diario de pruebas en el juego | `PhoenixSync/PRUEBAS.md` |
| Bitácora de cambios | `PhoenixSync/REGISTRO.md` (Sync) · `REGISTRO-LINK.md` (Link, en su rama) · `REGISTRO.md` del repo web |
| Sider | `PhoenixSync/sider/` (`phoenix.lua`, `VINCULO-TIEMPO-REAL.md`, `RIESGOS-SIDER.md`, `AUDITORIA-SIDER.md`) |
| Prompts para los otros chats | `PhoenixSync/prompts/` · traspaso completo: `PhoenixSync/PROMPT-PARCHE-PHOENIX.md` |
| Coordinación entre chats y cuentas | `COORDINACION.md`, `CLAUDE.md` |
| Juego ConmeGOL | `D:\Frank\Games_\Conmegol Patch\` (Sider en `SiderAddons\`, modos en `ConmeGol Extras\`) |
| Juego Sudamerican | `D:\SP2026\` |
| Pruebas en la PC | `…\Conmegol Patch\_PhoenixMercado_prueba\` (exe de Sync, `.bat`, `db\` con las variantes v90/v95/v99, `_versiones\2026-10-08\`) |
| Respaldos de Sider | `sider.ini.respaldo-phoenix-20261009` y `sider.ini.respaldo-phoenixdb-20261009` (en la raíz y en la carpeta del modo) |
| Google Drive | carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI` |
| Página Vestuario Barça | https://claude.ai/artifact/WskFnXgRmMhAs12Mv6cwTh |

---

### 10. Créditos y respeto

- **ConmeGOL Patch** y **Sudamerican Project:** referencias de estudio. Su contenido (caras, kits, estadios, módulos) es de sus autores (Olmos Jr 23, facemakers, kitmakers…). Para usarlo en Phoenix: **permiso + crédito**.
- **Herramientas de la comunidad** (Sider de juce, libpesXcrypter, 4ccEditor, pes-file-tools, módulos Lua como CommonLib de zlac): se respetan sus licencias y se citan.
- **Lo propio del Phoenix Evolution Patch** (base de datos, Phoenix Sync, Phoenix Link, el módulo Phoenix y la conexión con la web): **de FRALEX**.

---

## Parte 6 · Prompt de traspaso — Parche Phoenix Evolution (PES 2021)

> Origen: `PhoenixSync/PROMPT-PARCHE-PHOENIX.md` · último cambio 5bf26d4 2026-10-09 · 18611 bytes. Copia íntegra.

Copia todo lo que está debajo de la línea y pégalo como primer mensaje en el chat nuevo.

---

> **Actualizado el 2026-10-09:** lo más nuevo y completo está en `PhoenixSync/MANUAL-PARCHE-PHOENIX.md`. Si algo de este prompt choca con el manual, **manda el manual**.

Hola. Soy **FRALEX** (llámame siempre así). Vengo de otro chat donde trabajamos muchas horas en mi proyecto de **parche propio de PES 2021**. Te paso **todo el contexto** para que sigas exactamente donde quedamos. Léelo entero antes de hacer nada.

### 0. Cómo quiero que trabajes conmigo

- Háblame **siempre en español**, muy **simple, paso a paso, ordenado y espaciado**. Me mareo con mucha información junta o con muchos números y símbolos seguidos. Define cada término nuevo desde cero, usa ejemplos sencillos (incluso infantiles) y **repite** las cosas importantes si hace falta.
- **No me dejes mucho tiempo sin saber qué haces**: mándame avisos cortos de avance mientras trabajas.
- **Regla de oro: 0 errores.** El sistema tiene que quedar totalmente funcional. Si aparece un error, **no se avanza** hasta resolverlo o hasta que yo lo diga. Si algo puede romperse, mejor no lo hagas.
- Tienes **acceso total a mi PC** (a través de la app de escritorio de Claude, carpetas conectadas). Úsalo, pero **nunca cambies algo de mis juegos sin copia de seguridad** y siempre en archivos **nuevos**.
- **Nunca me pidas contraseñas ni secretos.**
- Quiero un **diario de prueba y error** de todo (qué se probó, qué falló, cómo se arregló), y al final un **PDF** que documente todo.
- La **base de conocimiento** del parche debe actualizarse **siempre también en Google Drive** (carpeta del proyecto, id `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`), no solo en el repo o la PC.
- Si descubres algo interesante durante cualquier análisis, **avísame** y dime qué podríamos hacer con eso.
- Haz toda la **ingeniería inversa** que haga falta.
- Las funciones deben ser **adaptables a cualquier parche** de PES 2021, siempre que se tenga la información de su base de datos.
- **No uses ni publiques contenido de otros moders** (caras, uniformes, estadios…) sin su **permiso** y sin darles **crédito**. Lo que fabriquemos nosotros (base de datos, programas, conexión con la web) sí es mío.
- Nunca se distribuye el juego: solo nuestros archivos.

### 1. El gran objetivo

Crear **el parche de PES 2021 más ambicioso, completo y sin errores que exista**: el **Phoenix Evolution Patch**. Tiene que ser **uno solo con todo mi ecosistema**:

1. **Mi web Phoenix Evolution**: liga, mercado de fichajes, perfiles. Repo `Fralexito/phoenixevolution`, con Supabase («Galaxy League»).
2. **Phoenix Link**: mi programa de **salas para jugar a distancia**. Es un fork de **Smash Soda 7.0.3** (C++, Parsec SDK, mandos virtuales ViGEm). El juego corre en la PC del anfitrión y los amigos lo reciben por streaming.
3. **Phoenix Sync**: el programa de PC que **mete en el juego lo que dice la web** (fichajes, dinero, plantillas) editando el **option file** y la **Liga Máster**.

Además quiero un **"PES online" propio**, conectado a mi web, y **funciones nuevas nunca vistas**.

### 2. Dónde está todo

- **Repositorio:** `Fralexito/smash-soda-fork`. En mi PC: `C:\dev\smash-soda-fork`.
  - Frente **MERCADO**: rama `mercado-fase0`, carpeta `PhoenixSync/`. Bitácora: `PhoenixSync/REGISTRO.md` (una línea por cambio). Diario de pruebas en el juego: `PhoenixSync/PRUEBAS.md`.
  - Frente **LINK**: rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`. Bitácora `REGISTRO-LINK.md`. Link compila en el worktree `_phoenix-link`.
  - Antes de tocar nada: leer `CLAUDE.md` y `COORDINACION.md`. Trabajo con **dos cuentas de Claude** sobre el mismo repo: siempre `git fetch` + `git pull --rebase` al empezar.
  - Reglas: no tocar la carpeta del otro frente; **nunca** `git checkout` de otra rama en `C:\dev\smash-soda-fork`; compilar sin errores antes de cada commit; una línea en la bitácora por cambio; **la web y Supabase se piden por prompt al chat de la WEB** (no se tocan desde aquí); nada va a `master` sin mi permiso.
- **Base de conocimiento** (repo y PDF): `PhoenixSync/base-conocimiento/`
  - `00-INDICE.md`
  - `01-MOTOR-PES2021.md`: formatos del juego; la sección J tiene el mapa ampliado de `Player.bin`.
  - `02-PARCHE-PHOENIX.md`: anatomía de un parche y plan.
  - `03-INVESTIGACION-PARCHES.md`: otros parches, causas de crash, modo online y herramientas.
  - `04-ESTRATEGIA-PHOENIX.md`: ConmeGOL vs Sudamerican, cómo superarlos, funciones nuevas, ruta por fases y proyección a futuro.
  - `parches/conmegol-26.md` y `parches/sudamerican-2026.md`.
  - Informes completos en `investigacion/`; datos en `datos/`.
  - PDF: `base-conocimiento-PES21.pdf` (33 páginas), que se genera con `generar_pdf.sh`.
- **Sider:**
  - `PhoenixSync/sider/`: módulo `phoenix.lua`, `PhoenixAviso.bat` / `.ps1` y `RIESGOS-SIDER.md`.
  - `AUDITORIA-SIDER.md` y la carpeta `auditoria/`: auditoría de los 49 módulos.
- **Juegos en mi PC:**
  - **ConmeGOL Patch 26:** `D:\Frank\Games_\Conmegol Patch\`. Guardados en `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
  - **Sudamerican Project 2026:** `D:\SP2026\`. Guardados en `…\292733975847239680\save`.
  - Carpeta de pruebas: `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba` (exe del Mercado y `.bat` de prueba).
  - **Foto de versiones del 8 oct** (antes de las actualizaciones de los parches): `_PhoenixMercado_prueba\_versiones\2026-10-08\`.
- **Google Drive** (carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`), con estos documentos:
  - «Base de conocimiento PES 2021 — Phoenix (vivo)»
  - «Auditoría de Sider»
  - «Investigación de parches PES 2021 y modo online»
  - «Informe estratégico Phoenix»
  - «Proyección a futuro»
- **Página del experimento Barça:** https://claude.ai/artifact/WskFnXgRmMhAs12Mv6cwTh (página con base de datos: colecciones `jugadores/<pid>` y `club/barca`).

### 3. Lo que YA funciona (probado dentro del juego)

**Phoenix Sync** (C++; compila para Windows con MinGW, ejecutables estáticos):

- **Option file** (`EDIT00000000`, cifrado con libpesXcrypter):
  - Mover jugadores entre equipos con la **alineación bien hecha** (orden de formación y roles; sustituto por posición). Probado en un amistoso Boca.
  - Dorsales: el pedido, si no el de su club, si no el de su selección, si no el más alto libre (igual que el juego).
- **Liga Máster** (`ML0000000N`; ranura N = archivo N−1 en hexadecimal):
  - Vender de mi equipo a la computadora.
  - Traspasos entre equipos de la computadora.
  - **Fichar para mi equipo** (probado con Sommer: jugó, un solo contrato, dinero correcto). El contrato **no** se escribe en la tabla de contratos: el juego lo crea solo desde la ficha del bloque comprimido.
  - Cambiar el **presupuesto** y el **tope salarial**.
  - Leer y reescribir el **bloque comprimido (zlib)**: valor de mercado, sueldo, competiciones inscritas.
- **Conexión con la web:**
  - `/liga/cambios` viene **firmado** (Ed25519; clave `k67bd032d50`). Se aplica al option file y/o a la Liga Máster, **todo o nada**.
  - El fichaje para mi equipo ya va conectado.
  - Acepta campos opcionales `monto`, `sueldo`, `clausula`, `fin_contrato` (AAAA-MM-DD) y `dorsal` (1–99). **Ya le pasé el prompt a la web** para que los mande.
- **295/295 pruebas automáticas OK.**
- **Pendiente:** jugadores **libres** en la Liga Máster. Necesito hacer una prueba de referencia en el juego: fichar a un libre y rescindir a uno.

### 4. Sider (puente en vivo con el juego)

- Sider **7.3.3** en ConmeGOL y 7.3.4 en Sudamerican. Su Lua trae acceso a archivos, a la memoria del juego, `ffi`, `zlib`, `match.stats()` y una **pantallita (overlay)** que se abre con Espacio y cambia de módulo con la tecla 1. **No trae internet.**
- **El switcher de ConmeGOL copia `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons` encima de `SiderAddons` del juego.** Por eso cualquier cambio hay que hacerlo **en la carpeta del parche**. Así se perdió la línea de `phoenix.lua`, que **nunca llegó a cargarse**.
- `phoenix.lua` **está activo** (v0.10, instalado en la carpeta del modo): muestra `content\phoenix\avisos.txt` en el overlay (los avisos de la web llegan en vivo), tiene la tecla B (buscar una ficha en memoria con copia segura) y un **espía** de lecturas de la base. El Lua de Sider **no trae `pcall`**: se usa una bandera.
- **Auditoría** (antes de instalar nada; yo pedí un análisis profundo de **cada línea** para tener la máxima certeza de que nada choque):
  - 🔴 `camera.lua` + `DynamicWideCam.lua` parchean **la misma instrucción** y corrompen datos (comprobado en el log, en los **dos** parches).
  - 🔴 `GFX_lod.lua` escribe a ciegas.
  - 🟠 Riesgo medio: SoundServer, CGP_SB_Addons_2, shirtless_celebration (que añadí yo), real-coach-team, SleeveBadge, Commentary, Competition-Server.
  - 🟡 Módulos que no hacen nada: Chatty_AutoLineup (es de FL26), scoreboard-hexx, goalscreams, Anti-Cheat (placebo), env.lua.
  - 11 módulos del `sider.ini` no existen; «Menu Light» aparece dos veces.
  - No activar `attack_mentality.lua`.
  - Ojo: algunos módulos y archivos **los añadí yo a mano** (bajados de YouTube); no todos venían en el parche.
- **Plan acordado** (nada sin mi permiso):
  1. Yo juego **un partido de referencia** con el Sider actual.
  2. Limpieza con copia de seguridad: quitar camera.lua, GFX_lod, los inútiles y las líneas rotas. Otro partido y comparar los logs.
  3. Instalar `phoenix.lua` **en la carpeta del parche** y probar el aviso en pantalla.
- **Con amigos:** usar **Phoenix Link (Parsec)**. El juego corre solo en mi PC y ellos no instalan nada.

### 5. Modo online (lo realista)

- Los servidores de Konami cerraron en 2022, y PES 2021 de PC **no tiene LAN**.
- **PESBUL** (servidor de la comunidad, solo amistoso 1 contra 1) exige el juego de **Steam sin modificar y sin Sider**. Por eso no me funcionó: mis carpetas tienen Sider y archivos que no son de la instalación original. Su base es el servidor de código abierto de Nikow5 (MIT). En PES 6 existió algo parecido: `kinj1987/evo-league`.
- **Arquitectura elegida:**
  1. La **web** crea el partido.
  2. **Phoenix Link** (Parsec) en el PC anfitrión deja entrar solo a esos jugadores y carga el option file oficial.
  3. Un módulo **Sider** lee el marcador con `match.stats()` (no da goleadores ni tarjetas) y lo deja en `resultado.json` (el «buzón»).
  4. Phoenix Link lo sube a la web con una captura, y los dos jugadores confirman.
  5. El verificador revisa solo al anfitrión.
- Parsec desde Perú: subida ≥ 10 Mbps (30 recomendados), 60 fps, 720p/900p, cable, sin VPN. No funciona con CGNAT.
- Un servidor propio estilo PESBUL sería un proyecto de meses o años (frontera).

### 6. Lo aprendido de mis dos parches (los dos grandes referentes de qué hacer y qué no)

- **ConmeGOL 26:**
  - Casi todo por Sider (livecpk).
  - 90 competiciones, 749 equipos (el techo es 750), 29.997 jugadores (el techo es 30.000).
  - ⚠️ **La base que usa el juego es la que sirve Sider desde `SiderAddons\olmosjr23\Database`** (5/9/2026), no `CGP_database.cpk` (19/8). Tiene 5.894 jugadores distintos. **El catálogo subido a la web salió de la vieja: hay que regenerarlo desde la de Sider.**
  - La versión instalada es la 26; ya existe la 27. **Se viene una actualización.**
- **Sudamerican 2026:**
  - Parche «clásico» de CPK: 34 CPK, 13 de caras de 5–11 GB.
  - 80 competiciones y 726 equipos. Option file en portugués.
  - Trae **Liga 2 de Perú (14 equipos)**, que ConmeGOL tiene vacía.
  - Sus dos Sider tienen el mismo choque de cámaras, módulos que fallan al arrancar (randomMenu, TurfLoader, LogoHD), duplicados y `netblock.lua`, que bloquea el online.
  - **Se actualiza en una semana.**
- **Liga 1 2026** (18 equipos): igual en los dos.
- **Hueco libre:** no existe un parche de PC dedicado a la **Liga 1 de Perú** y las ligas andinas.
- **Protocolo cuando un parche se actualice:**
  1. Copia de la carpeta `save`.
  2. Comparar con la foto del 8 oct: base de datos, Sider y option file.
  3. Auditar solo lo nuevo.
  4. Regenerar el catálogo y subirlo.
  5. Probar.
  6. Anotar el «changelog técnico».
  - A futuro: un comando **«Auditor de parches»**.

### 7. Receta y ruta del Phoenix Evolution Patch

- **Base fija:**
  - PES 2021 de **Steam**, exe 1.07.02 + Data Pack 7.0.
  - Sider fijado (7.4.1, con `save.folder` para un EDIT propio).
  - Base de datos pequeña en un `.cpk` + contenido por Sider/livecpk en **addons por liga**.
  - Un `sider.ini` **auditado**.
  - **Lanzador propio** (Phoenix Link) sin `.bat`, que comprueba versiones, archivo de paginación y antivirus.
  - IDs reales de Konami.
  - **No cambiar el tamaño de una liga** sin parchear el exe (cuelga la Liga Máster).
  - Límites: 750 equipos, 40 por equipo, 30.000 jugadores.
- **Fases:**
  - **0.** Instalación limpia de referencia + partido de referencia. Pregunta abierta: **¿tengo PES 2021 en Steam?**
  - **1.** Versión 0.1: Liga 1 + Liga 2 + Copa, option file oficial, Sider limpio.
  - **2.** Fabricador de CPK y DpFileList (portar `the4chancup/pes-file-tools`), **Phoenix Doctor** (auditoría automática de Sider) y lanzador.
  - **3.** Conexión: avisos → resultado automático → verificador → option file por sala.
  - **4.** Addons de contenido con permisos y créditos.
  - **5.** Más allá: competiciones propias, estadísticas completas, Liga Máster viva y **Liga Máster online asíncrona** (cada amigo con su club y su carrera, compartiendo mercado y tabla en la web). Es la idea más original.
- **Funciones nuevas por viabilidad:**
  - 🟢 **Ya se pueden:** mercado conectado, Liga Máster controlada por la web, option file por sala, avisos, resultado automático, Phoenix Doctor, verificador, y «amistoso que se ve como partido oficial de mi liga» (como `CustomExhibitionMatch.lua`).
  - 🟡 **Con investigación:** goleadores y tarjetas desde la memoria (tabla de Cheat Engine de xAranaktu), cambios en vivo en la Liga Máster, CPK propios, competiciones propias (descifrar `Competition*` y `CompetitionRegulation`), calendario de la Liga Máster desde la web, actualizaciones por partes, ligas de más de 20 equipos, informe de errores.
  - 🔴 **Frontera:** Liga Máster online asíncrona, mundo real en el juego, servidor propio, espectadores en vivo, reglas propias.

### 8. Experimento en curso: «Vestuario Barça» (lo último que hicimos)

- Pedí que **todo mi equipo y mi Liga Máster sean totalmente editables, como en el juego e incluso más**: ficha de cada jugador con cara, escudo, camisetas, estadísticas, posición, dorsal, altura, habilidades…
- **Mi equipo de prueba es el Barcelona**: ID 108, carrera nueva en la **ranura 1** (`ML00000000`), índice 122 en la Liga Máster.
- **Hecho:** página editable (enlace arriba) con los 25 jugadores, minifaces y escudo (los sacamos del parche y los metimos en la página), datos de `Player.bin` de la base viva (olmosjr23), habilidades, posiciones jugables en un campo, valor y sueldo de la Liga Máster y finanzas. Los cambios se guardan como **pendientes** en la base de datos de la página (`cambios: [{campo, valor}]`).
- **Actualización 2026-10-09:** la técnica para aplicarlos ya está **probada** (Player.bin propio en `livecpk\Phoenix-DB` + Editar → Cargar). Falta automatizar. Plan original: editar `Player.bin` **solo en los campos confirmados**, servido por una raíz de Sider propia (`cpk.root` antes de olmosjr23), y la Liga Máster con lo que ya sabemos. Después marcar el jugador como aplicado.
- **Mapa de `Player.bin` (registro de 312 B, id en +8):**
  - Las 19 habilidades de campo ya estaban confirmadas.
  - Potencia de tiro: bit 358.
  - 5 de portero: bits 269, 300, 320, 326 y 364. **Falta saber cuál es cuál.**
  - Posiciones jugables: PT 350, DFC 468, LI 318, LD 474, MCD 414, MC 456, MI 466, MD 460, MO 464, EI 472, ED 476, SD 478 (probable), DC 470.
  - Pie malo, forma y lesiones: 454 / 462, 438, 458 (sin confirmar).
  - Habilidades especiales y estilos COM: bits 480–531, en calibración.
  - País: bits 10–18 de `Country.bin`.
  - Minifaces: `livecpk\Logos\common\render\symbol\player\<pid>.dds`. Escudos: `…\flag\e_000108_r_l.png`.
  - Faltan las fotos de Josué Caicedo y Gabriel Jesus. Las camisetas todavía no están. La cara 3D no se puede editar como foto.
- **La carrera se guardó recién creada:** todavía no tiene fecha (1/1), ni alineación, ni contratos definitivos.
- **Me toca a mí:**
  1. Avanzar al primer día de la temporada y guardar en la ranura 1.
  2. Mandar **fotos de la ficha en el juego** de **Joan García** (portero) y **Lamine Yamal**: todas las pantallas, para calibrar.
  3. Probar la página.

### 9. Pendientes generales (en orden)

1. Experimento Barça: calibrar con las fotos, aplicar los cambios al juego (`Player.bin` por Sider + Liga Máster) y probarlo dentro del juego.
2. Regenerar el catálogo con la base viva de Sider y subirlo a la web.
3. Jugadores libres en la Liga Máster (prueba de referencia en el juego).
4. Sider: partido de referencia → limpieza con permiso → `phoenix.lua` en la carpeta del parche → probar el aviso.
5. Cuando actualice ConmeGOL (mañana) y Sudamerican (en una semana): protocolo de actualización con la foto del 8 oct.
6. Fase 0 del parche (pregunta de Steam).
7. Subir a Drive todo lo nuevo (el experimento Barça todavía no está en Drive).

Empieza leyendo `COORDINACION.md`, el final de `PhoenixSync/REGISTRO.md` y `PhoenixSync/base-conocimiento/04-ESTRATEGIA-PHOENIX.md`. Después dime en simple qué entendiste y seguimos con el pendiente 1.

### 10. 🏆 Lo último: la «puerta en vivo» (2026-10-09)
- **Método probado:** Player.bin propio en `SiderAddons\livecpk\Phoenix-DB` (raíz antes de `olmosjr23\Database`) + **Editar → Cargar** en el juego ⇒ las stats cambian sin reiniciar (Lamine Velocidad 99 → 95 → 90, confirmado por huella).
- **Puente web → juego:** Phoenix Link escribe `content\phoenix\avisos.txt` y el módulo `phoenix.lua` lo muestra en el overlay de Sider (el «hola» de la web llegó al juego).
- **Detalle completo, fallos y universo de posibilidades:** `base-conocimiento/05-PUERTA-EN-VIVO.md`. Diario: `PRUEBAS.md`.
- **Futuro (no prioritario):** crear jugadores nuevos; avisos con aspecto nativo (imágenes de menú).
- **Phoenix Mercado ahora se llama Phoenix Sync** (carpeta `PhoenixSync/`; la rama sigue siendo `mercado-fase0`).

---
