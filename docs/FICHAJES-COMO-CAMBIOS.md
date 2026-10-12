# Fichajes como CAMBIOS (generados en cada PC sobre SU base)

Estado: **implementado en Phoenix Link 0.2.46** (2026-10-12). Probado con bases falsas y con COPIAS de las bases reales de Fralex; **sin probar todavía dentro de PES** (ver §7). Este documento sustituye al informe de investigación de la mañana del 12-oct, cuya conclusión principal estaba equivocada (§2).

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
- **Base de cada sitio:** la primera `cpk.root` activa del `sider.ini` que manda ahí (sin Phoenix-DB) que trae `PlayerAssignment.bin` (olmosjr23 en los 26); si no hay, el `download\CGP_database.cpk` del modo. Si no se lee o no tiene el formato esperado: **no se crea nada** (regla «sin base reconocida, no se pone Phoenix-DB», que ahora se aplica a cada modo).
- **Qué escribe:** solo `PlayerAssignment.bin` (cabecera WESYS y zlib «almacenado», válido para cualquier lector zlib) y `fichajes_aplicados.json` (estado: fecha, huella de la base, de la lista y de lo escrito). Si Phoenix-DB tiene un `Player.bin` que **no** es el de la base (p. ej. la copia vieja del 26 en un modo 27), se **aparta** a `Phoenix-DB\_antes-de-fichajes-<sello>\` (no se borra) para no tapar a los jugadores del parche.
- **sider.ini:** si falta, añade una línea `cpk.root = ".\livecpk\Phoenix-DB"` (antes de la base si la hay; si no, antes de la primera `cpk.root`), con copia `sider.ini.phoenix-<sello>` y vuelta atrás.

## 4. Lista de cambios (`fichajes.json`, versión 1)
```json
{ "version": 1, "regla": "relajada",
  "cambios": [ { "player_id": 114506, "team_id": 101, "origen": 0, "nota": "texto opcional" } ] }
```
- Orden de búsqueda: `%APPDATA%\Phoenix Mercado\fichajes\fichajes.json` (punto de entrada para Sync/Mercado) y, si no existe, `<exe>\fichajes\fichajes.json` (el que lleva Link).
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
- `correr_escenarios.bat`: 55/55 (se actualizó E13: la advertencia de «Phoenix-DB sobre una base 27» ahora exige un Player.bin dentro de Phoenix-DB).
- `modulos_test`, `parche27_test` y `quitar_modulo_test` ya fallaban igual antes de este cambio (12, 21 y 8 fallos con el código del último commit y con el nuevo): pendientes de ponerlos al día.
- **Sin probar en PES (pendiente de Fralex):** que el juego acepte el `.bin` generado (zlib sin compresión), que Sider cargue la raíz Phoenix-DB que se añade al `sider.ini` del 27, y el significado de puesto/número bajo (un fichaje de prueba: ver `docs/BETA-1-GUIA-AMIGO.md`, «Probar un fichaje»).
