# Fichajes como CAMBIOS (generados en cada PC sobre SU base)

Estado: **implementado en Phoenix Link 0.2.47** (2026-10-12; la 0.2.46 trajo los fichajes y la 0.2.47 el detector de parche/base por evidencia y la lista por nombres). Probado con bases falsas, estructuras falsas de cada caso y COPIAS de las bases reales de Fralex; **sin probar todavía dentro de PES** (ver §7). Este documento sustituye al informe de investigación de la mañana del 12-oct, cuya conclusión principal estaba equivocada (§2).

## 0. En una frase
Link ya no lleva ni copia ninguna base de datos. Lee la base REAL del PC (la del modo/parche activo), le aplica una lista corta de cambios «jugador → equipo» (`fichajes.json`) y deja solo `PlayerAssignment.bin` en `SiderAddons\livecpk\Phoenix-DB\…\pesdb\`. Player.bin queda el de la base. Se quita en un clic.

## 1. Formato de las bases (verificado con las bases reales)
Cada `.bin` de `CGP_database.cpk` (CPK sin comprimir, tabla `@UTF` enmascarada; el tamaño del bloque va en **little endian**) y de las carpetas `olmosjr23\Database` / `Phoenix-DB` tiene cabecera de 16 bytes `FF 10 81 'WESYS'` + `u32 tamaño comprimido` + `u32 tamaño real` + flujo **zlib** (`78 DA`). `Country.bin` va sin esa cabecera.

**PlayerAssignment.bin**: sin cabecera, filas de **16 bytes** = 4 × `u32` little endian:

| campo | significado |
|---|---|
| 0 | id de fila (único; no es estable entre bases) |
| 1 | **player_id** (un jugador puede salir en 2 filas: club + selección) |
| 2 | **team_id** |
| 3 | empaquetado: `>>10` = puesto en la plantilla (único por equipo), `&1023` = número bajo (no siempre único; probable dorsal, sin confirmar) |

Filas: 26 (olmosjr23) = 19.747, CPK 26 = 20.398, 27 = 19.612, UEFAGOL 27 = 19.489, Clásicos = 15.562. En ninguna base hay un equipo con más de 40 filas.

**Player.bin**: cabecera de 8 bytes + registros de 312 bytes (id = primeros 4 bytes; 0 = hueco); el archivo termina con el último registro recortado. **Team.bin** no se decodifica: un equipo es válido si la base le da al menos una fila.

## 2. Corrección: Phoenix-DB no tenía fichajes
La investigación inicial comparó Phoenix-DB con la CPK del 26 y encontró 175 «cambios de Fralex» (12 altas reales, 37 «Player Identidad», 126 bajas). **Era un error de base:** los modos 26 no usan la CPK, usan `olmosjr23\Database` (esa raíz va en su `sider.ini`). Comparado con esa base, el `PlayerAssignment.bin` de Phoenix-DB es **idéntico** (mismo md5 `7958bbaf…`) y su `Player.bin` también (idéntico al descomprimir; solo cambia la compresión: 1.769.252 vs 1.777.004 bytes). Es decir: Phoenix-DB es una **copia de la base de olmosjr23**, sin ningún fichaje. El único fichaje que pasó por Sync fue una prueba («Jugador 114506: equipo 101», 10-oct) que se deshizo.

Consecuencias:
- La lista de fichajes que lleva Link va **vacía** (`SmashSoda\phoenix\fichajes\fichajes.json`). Los fichajes reales los pondrá Fralex (o vendrán de la web/Mercado, §5).
- Las 175 diferencias eran solo la distancia entre la CPK 26 y olmosjr23: no hay que guardarlas.
- La base «real» que ve el juego depende del modo: **26 → olmosjr23\Database**; **27, UEFAGOL 27, Clásicos → el CGP_database.cpk del modo**.

## 3. Diseño implementado
- **Código:** `SmashSoda\phoenix\link\Fichajes.h/.cpp` (inflate propio, lector de CPK, formato `.bin`, reglas, aplicar/quitar), `web\AccionesFichajesWeb.cpp` (acciones `fichajes.estado` y `fichajes.accion`), `ModulosJuego` (`raicesActivas`, `carpetasModos`, `asegurarRaizPhoenixDB`), UI en SYNC › Puente › «Fichajes de Phoenix».
- **Sitios:** la raíz del juego y cada modo del cambiador (`ConmeGol Extras\<modo>`). El lanzador copia `SiderAddons` del modo sobre la raíz al iniciar, así que el resultado de cada modo viaja con él.
- **Base de cada sitio (detector por evidencia, no por nombres de ConmeGOL).** Para cada archivo (`PlayerAssignment.bin`, `Player.bin`, `Team.bin`) Link hace lo mismo que el juego, en este orden:
  1. recorre las `cpk.root` activas del `sider.ini` que manda ahí (sin Phoenix-DB), en orden de prioridad, y toma la primera cuya carpeta `common/etc/pesdb` trae el archivo (la carpeta puede llamarse como sea: `olmosjr23/Database`, `livecpk/Mi Base`…);
  2. si ninguna lo trae, mira los CPK: los `.cpk` de la carpeta `download` del modo (o de la raíz) y de `Data`, leyendo solo la cabecera y la tabla del CPK (los `dt80_*.cpk` pesan GB), y se queda con el de **mejor prioridad** según `DpFileList.bin` (1 = la más alta; el CPK propio de un modo manda sobre el resto). Si dos CPK empatan en prioridad con bases distintas, **no se adivina**.
  Lo que se encuentra se **valida por estructura** (cabecera `WESYS`, zlib, filas de 16 bytes, jugadores y equipos con sentido, máx. 40 por equipo). Si no se lee o el formato es otro: **no se crea nada** y el mensaje dice «qué detecté» y «qué hacer».
- **Parche detectado (nombre legible):** por la huella del `PlayerAssignment.bin` (ConmeGOL 26 olmosjr23, Patch 26, Patch 27, UEFAGOL 27, Clásicos), si no por el nombre de la carpeta del modo, por `version_actual.txt` (raíz) o por el CPK oficial («PES 2021 original»). Si no lo reconoce por nombre pero la base se lee y valida: «parche desconocido (…)» y se usa como **base genérica**, y la interfaz lo dice. Cada análisis guarda la evidencia («cómo lo supe»).

| Estructura | ¿Detectada? |
|---|---|
| Base en `cpk.root` con cualquier nombre de carpeta; varias bases (gana la primera) | Sí |
| Parche con su CPK en `download` (cualquier nombre de `.cpk`, con o sin `DpFileList.bin`) | Sí (empate de prioridad con bases distintas: no se adivina) |
| Cambiador de modos (varios `ConmeGol Extras/<modo>`), cada uno con su base | Sí, cada modo por separado |
| Parche desconocido con base válida (Sudamerican, PESEdit…; no probados con copias reales) | Sí, como «base genérica», si la base está en formato WESYS |
| PES original sin parche (`dt80_*.cpk`, `Data/dt*.cpk`) | **No:** esos `.bin` van cifrados con un formato propio del juego. Se reconoce «PES 2021 original» y no se crea nada |
| CPK con los `.bin` comprimidos (CRILAYLA), bases con otro formato, sin Sider | No (no se crea nada y se explica por qué) |
- **Qué escribe:** solo `PlayerAssignment.bin` (cabecera WESYS y zlib «almacenado», válido para cualquier lector zlib) y `fichajes_aplicados.json` (estado: fecha, huella de la base, de la lista y de lo escrito). Si Phoenix-DB tiene un `Player.bin` que **no** es el de la base (p. ej. la copia vieja del 26 en un modo 27), se **aparta** a `Phoenix-DB\_antes-de-fichajes-<sello>\` (no se borra) para no tapar a los jugadores del parche.
- **sider.ini:** si falta, añade una línea `cpk.root = ".\livecpk\Phoenix-DB"` (antes de la base si la hay; si no, antes de la primera `cpk.root`), con copia `sider.ini.phoenix-<sello>` y vuelta atrás.

## 4. Lista de cambios (`fichajes.json`, versión 1)
```json
{ "version": 1, "regla": "relajada",
  "cambios": [ { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto opcional" } ] }
```
- **Versión 2 (compatible con la 1): referirse por NOMBRE.** Como los `player_id` y `team_id` cambian entre parches, cada cambio puede ir por nombres, resueltos contra la base de ESE PC:
  ```json
  { "version": 2, "cambios": [
    { "jugador": "Cristiano Ronaldo", "equipo_origen": "Al-Nassr FC", "equipo_destino": "Real Madrid CF", "nota": "texto" },
    { "player_id": 114506, "equipo_destino": "FC Barcelona" } ] }
  ```
  El jugador (`jugador` o `player_id`), el destino (`equipo_destino`/`equipo` o `team_id`) y el origen opcional (`equipo_origen` u `origen`) pueden mezclarse, pero cada campo va por id **o** por nombre. La comparación es **exacta** (sin mayúsculas, acentos, espacios ni signos: «Kylian Mbappé» = «kylian mbappe») contra el nombre completo del jugador (`Player.bin`) y el nombre del equipo (`Team.bin`). Si el nombre de un jugador sale varias veces, el equipo de origen lo desambigua; si sigue sin ser único, o un equipo tiene el nombre repetido, **se salta** con el motivo (`jugador_ambiguo`, `equipo_ambiguo`, `jugador_no_encontrado`, `equipo_no_encontrado`, `nombres_no_disponibles`). La versión 1 solo admite ids.
- Orden de búsqueda: `%APPDATA%' + B + 'Phoenix Mercado' + B + 'fichajes' + B + 'fichajes.json` (punto de entrada para Sync/Mercado) y, si no existe, `<exe>\fichajes\fichajes.json` (el que lleva Link).
- **Reglas** por cada cambio: el jugador debe existir en `Player.bin` de la base y el equipo en ella; máximo 40 por equipo; si ya está en el equipo destino no hace nada; sin `origen` mueve su única fila (si tiene dos, se salta por ambiguo); con `origen` mueve esa fila. **Relajada** (por defecto): si el jugador ya no está en `origen`, gana el cambio de la lista; **estricta**: se salta. No hay bajas (no se puede sacar a un jugador de un equipo) ni se usan equipos que no existan (p. ej. 9107/9108 del 26).
- Fila movida: conserva su id; puesto = primer hueco libre del equipo destino; número bajo = el suyo si está libre, si no el primero libre. Altas: id = máximo+1.
- **Validación antes de escribir (todo o nada):** ids de fila únicos, sin pares (jugador, equipo) repetidos, puesto único por equipo, ≤ 40 por equipo; después de escribir se vuelve a leer y se comprueba el sha256.

## 5. Quién guarda y quién aplica
- **Link aplica** (esto). **Quién guarda la lista:** Sync/Mercado deberá dejar `fichajes.json` en `%APPDATA%\Phoenix Mercado\fichajes\`; el formato y las reglas están en `docs/PROMPT-WEB-fichajes-lista.md` (para pegar en el chat WEB/Sync). Mientras tanto la lista se edita a mano. La entrega antigua de Sync (`PlayerAssignment.bin` ya hecho dentro de Phoenix-DB) sigue funcionando; si pisa un archivo generado por Link, el estado pasa a «lo cambió otra herramienta» y Link no lo borra al quitar.

## 6. Seguridad, copia y vuelta atrás
- Solo con PES y `sider.exe` cerrados. Copia de lo que había antes (`_antes-de-fichajes-<sello>`) y de cada `sider.ini`. Escritura atómica (`.tmp` + reemplazo).
- **Quitar** (un clic): borra solo lo que generó Link (si su sha coincide con el estado guardado), deja la copia y el juego vuelve a la base original del parche. El `sider.ini` conserva su línea Phoenix-DB (inofensiva sin archivos).
- **Doctor / diagnóstico:** el informe para depurar y el diagnóstico en claro llevan «Fichajes: N aplicados, M saltados en K sitios» y una línea por modo con los motivos de lo saltado.

## 7. Pruebas
- `pruebas-link\correr_fichajes.bat` (MSVC; el .exe se llama `fichajes_test`, sin «instal»): **88/88 bien** con las copias reales de las bases de Fralex (CPK 26/27/UEFAGOL 27/Clásicos y olmosjr23): zlib real con códigos dinámicos, bin, CPK, reglas, 40 por equipo, idempotente (aplicar dos veces = mismos bytes), quitar, base dañada o ilegible → no se crea nada. El juego real solo se lee; todo se copia a una carpeta temporal.
- 0.2.47: `correr_fichajes.bat` **148/148** (con y sin las bases reales): estructuras falsas de cada caso (vanilla con `dt*.cpk`, parche con base en carpeta, parche con CPK propio y prioridad por `DpFileList.bin`, cambiador con varios modos, varias bases con prioridades, empate de prioridad, base dañada, nombre desconocido con formato válido, sin ninguna base), un CPK falso generado por el propio test (tabla `@UTF` enmascarada), la lista por nombres, y las copias reales (26 olmosjr23, 27, UEFAGOL 27, Clásicos): parche detectado por huella, base por `cpk.root` en el 26 y por CPK en el resto, idempotente. La data original de PES (`Data/dt*.cpk`, abierta en solo lectura) se reconoce y no se crea nada.
- `correr_escenarios.bat`: 55/55 (se actualizó E13: la advertencia de «Phoenix-DB sobre una base 27» ahora exige un Player.bin dentro de Phoenix-DB).
- `modulos_test`, `parche27_test` y `quitar_modulo_test`: **TODO OK** con `pruebas-link/correr_modulos.bat`. (Antes dije que fallaban: era mi forma de lanzarlos, con argumentos equivocados. Con el `sider.ini` real de la raíz del juego de Fralex, `parche27_test` falla porque ese `sider.ini` ya lleva las líneas de Phoenix; la prueba espera el del parche sin Phoenix.)
- **Sin probar en PES (pendiente de Fralex):** que el juego acepte el `.bin` generado (zlib sin compresión), que Sider cargue la raíz Phoenix-DB que se añade al `sider.ini` del 27, y el significado de puesto/número bajo (un fichaje de prueba: ver `docs/BETA-1-GUIA-AMIGO.md`, «Probar un fichaje»).
