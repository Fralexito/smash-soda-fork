# Capítulo 90 — Estado actual, decisiones que solo tú puedes tomar, contradicciones por resolver y hoja de ruta

Fecha de corte: 15-oct-2026. Resume lo que informaron los 11 agentes de lectura (capítulos 10–61) y el trabajo propio del día. Las horas de las bitácoras de las sesiones están en UTC; tu hora local (Lima) es UTC−5.

## 90.1 Semáforo: qué está probado de verdad
| Área | Estado | Evidencia |
|---|---|---|
| Orden de mandos: puesto N de Link = jugador N de PES (`settings.dat`, 8 casillas de 64 bytes desde 0x64) | PROBADO en tu PC (11-oct) | cap. 10 y 30 |
| Fichaje de tu «Editar» de PES al PES de un amigo, por la web, con el instalador .exe (Link 0.2.82) | PROBADO el 13-oct (166 peticiones, 0 fallos, máx. 2,7 s) | cap. 11, 30, 40 |
| Datos en vivo: botón nativo «Datos Actual. en vivo» revivido sin servidores de Konami (v0.16; Lamine Yamal Velocidad 90→99 en pantalla) | PROBADO el 9-oct | cap. 20 |
| Guardado de Liga Máster abierto y editado; barras de crecimiento (28 de 30 identificadas) | PROBADO | cap. 21 |
| Posiciones de los 22 jugadores leídas de memoria (Campo 2D) | PROBADO (11–12-oct) | cap. 10, 30 |
| Cancha 30×20 coherente (lógica + dibujo en Camp Nou) | PROBADO por ti el 14-oct (22:31 y «perfecto» 22:59 hora local) | cap. 12, 60 |
| Análisis completo de Ghidra sobre el exe recortado | HECHO (34 min) | cap. 60 |
| Mandos y partido con invitados reales (beta); instalador en PC limpia; traspasos con token real de punta a punta; Editar de PES → option file en la PC del amigo | SIN PROBAR (o solo en simulador) | cap. 40, 41 |
| Red/portería dibujada para la cancha pequeña (FoxAnim `.geom`) | PENDIENTE de la prueba del gol | cap. 60 |
| 4 contra 4 | SIN RESOLVER (viabilidad estimada 30–40 %, sin evidencia sólida) | cap. 60 |

## 90.2 Decisiones que solo puedes tomar tú
1. **Repos públicos con commit de la beta privada.** El 13-oct un `git push` subió a GitHub público el commit `606867f` (PhoenixBeta.h, BETA-PRIVADA.md, licencia, nombre del archivo de exención). Se cambió el archivo de exención (0.2.85), pero el historial sigue visible. Los repos `smash-soda-fork` y `phoenix-overlay` siguen públicos. Acción: ponerlos en privado (Settings › Danger Zone › Change visibility). Hasta entonces, cero `git push`.
2. **`main` de la web está desactualizado**: `origin/main` sigue en 42bccd6 (5-oct). Las rondas 171–274 y las migraciones 078–105 solo existen en la vista previa `/fase-beta/`. El respaldo semanal y el keep-alive solo corren desde `main` y no hay constancia de que se hayan probado. Decidir cuándo pasar a `main` («Súbelo»).
3. **Cuatro copias del repo de la web** (C:\Users\WinterOS\phoenixevolution = ronda 210; D:\Frank\IA\CLAUDE\pes-borrador = ronda 272; C:\dev\pes-liga-automatica = ronda 274, la más reciente; D:\Frank\IA\CLAUDE\phoenixevolution = borrador). Elegir una como oficial y archivar las demás.
4. **Permiso de descarga** de GzsTool y FoxTool (GitHub, MIT) para probarlos sobre el `.fox2` del Camp Nou y mover la red/portería.
5. **Prueba del gol**: abrir PES con el selector y marcar un gol para ver si el balón se detiene en la red con los `.geom` de FoxAnim a ±15.
6. **Divergencia entre chats de Sync** (cap. 21): el chat «Player.bin» y la cuenta B de SYNC excluyen los módulos que modifican el código del juego en memoria (v0.3–v0.17e) y el informe 27 recomienda instalar solo la v0.2; el chat principal de SYNC/LINK adopta el botón nativo parcheado en memoria y avanza a la v0.18. Falta tu decisión sobre cuál es la línea oficial.
7. **Actualizar Sider a 7.4.1** (hoy 7.3.4; hay vuelta atrás a 7.3.3 con un .bat): decisión tuya; antes auditar que ningún módulo use el hack de `colorsdemo.lua`.
8. **Base de datos oficial** del Phoenix Evolution Patch (hecha desde la base original de Konami con IDs propios): cuándo llega y cómo se publica con `sync-publicar-oficial --base`.
9. **Windows con animaciones apagadas** en tu PC: la web usa «Ajustes → Animaciones» por encima del sistema; confirmar que es lo que quieres.
10. **Gemini/Deep Research**: si quieres repetir la investigación con el prompt maestro corregido (I02 lista las afirmaciones dudosas).

## 90.3 Contradicciones entre capítulos (por resolver o ya resueltas)
| Tema | Versión A | Versión B | Estado |
|---|---|---|---|
| Hora del hallazgo del bloque de floats del campo | M10/M06: ~22:00 hora local | Transcripciones: entre 20:04 y 20:21 locales del 14-oct | Se toma la de las transcripciones (cap. 12). |
| `0x1436F3DC8` | M09: «registro de recursos» | cap. 60: es el gestor de actores (arrays de 22/26 entradas de 0x28 bytes en +0x1DD8, +0x25F8, +0x19C8, +0x21E8; equipos en +0x1928 y +0x1978; se usan con actor+0x5D0 y +0x55C) | Probable el gestor de actores; verificar en vivo (solo lectura). Es la vía más concreta para el 4v4. |
| Función que lee los 6 volúmenes de `.data 0x1437CF4C0` | Mi lectura del 15-oct (Referencias.java): prueba de cajas, filtro de visibilidad/recorte | cap. 60: pertenece al trabajo de física `cobra::And::PhPreProcessJob` («AndPhPreProcess»): filtro de candidatos de colisión | Lo más probable es el pre-proceso de física. En ambos casos NO es la red de la portería. |
| `0x1437CF518` | Gemini: dirección «libre» de 8 bytes | Es el componente z de la 6.ª caja y esa función la lee | Gemini equivocado: no usarla. |
| `0x140A6C54A` (ejemplo oficial de Sider) | Cita la doc de Sider | En nuestro exe cae en relleno 0xCC; la misma función está en 0x140A6BF1A | El ejemplo es de otra versión del exe. Para el hook usar 0x140A72E70 (primeros 16 bytes desplazables). |
| Estado `0x10` | Observado en la cadena de la roja (0x7→0x4→0x10→0x2) | Su manejador FUN_1407B2A10 decide pases/tiros; aparece también en t≈178,7 s y 311,6 s del log (este último tras 0xA→0x48→0xA) | No es solo roja; falta ver dónde se descuenta al expulsado. |
| Qué archivo manda con «Activar» | Con el botón activado mandan PlayerAssignment.bin y Player.bin | Con Editar→Cargar y la tecla L manda el option file | Ambas son ciertas según el camino (cap. 20/21). |
| Funciones descompiladas | «158.633 funciones» | Faltan hojas fuera de .pdata: 13.847 destinos `func_0x…`, uno usado 10.874 veces | La cifra completa es mayor; ver cap. 60. |
| `escalar_capa_campo.js` | Manuales M07/M12: reproduce `runs 21 vertices 8317` | El script de disco era la v2 fallida (`runs 1 vertices 8524`) | RESUELTO el 15-oct: filtro restaurado, MD5 idéntico al archivo bueno (0f544cc8…). |
| «Fichajes: 175, solo 2 se aplican» (doc 10 de Drive) | Hecho como cierto | Falso: se comparó con la base equivocada | Corregido (docs 11 y 12; Link 0.2.46/0.2.47). |

## 90.4 Pendientes por frente
**LINK.** Probar con invitados reales mandos y partido; probar el instalador en una PC limpia; «ARREGLAR TODO» no repone la línea `cpk.root ".\livecpk\Phoenix-DB"` si no hay ningún fichaje escrito (reportado el 13-oct 21:31 UTC); la línea de `phoenix_campo` en `sider.ini` la puso a mano la sesión de ingeniería inversa y Link no la repone; numeración de versiones con saltos (0.2.5, 0.2.56, 0.2.64 y 0.2.88 sin commit propio; dos commits dicen «0.2.40»); Link tiene 5 commits locales sin subir (último subido 0.2.84); la beta 2 pedía publicar releases en GitHub, lo cual choca con la regla de no hacer push mientras los repos sean públicos.
**SYNC.** `mercado-fase0` local va 6 commits detrás de origin (Yamal/Havertz y mover_cli); traspasos con token real de punta a punta; Editar de PES → option file y recarga en el PC del amigo; la v0.18 parchea memoria (recomendar el modo ACTIVAR); las reglas 5b (subir kVersion) y 5c (cualquier PC y parche) solo están en `COORDINACION.md` de la rama Link: copiarlas a la de Sync.
**WEB.** Elegir copia oficial (ver 90.2); pasar a `main` cuando digas «Súbelo»; probar el respaldo semanal y el keep-alive (solo corren desde `main`); Edge Function `directos` (pago-webhook) sin desplegar; plan FREE de Supabase (500 mil invocaciones al mes) condiciona el diseño; «App-galaxy-» está vacío en GitHub y sin copia local; capa 3D en toda la web (ronda 242) y Premios (cinco versiones; volviste a la primera).
**PC / PARCHE.** El parche activo es ConmeGOL 27 (`version_actual.txt`); `conmegol_info.txt` sigue diciendo 26: no fiarse. Phoenix-DB y olmosjr23 están apartadas desde el 13-oct y el `sider.ini` actual (3.553 B) solo lleva `phoenix.lua` v0.18b, `phoenix_referee.lua` v1.4 y `phoenix_campo.lua` v0.2d inerte. Cada cambio de modo o update devuelve el `sider.ini` a la base de 3.455 B. El `PES2021.exe` actual es byte a byte el de Sudamerican_Backup (SHA-256 5E27…); los modos 26 de ConmeGOL Extras ya no existen: solo se vuelve al 26 desde `_PhoenixMercado_prueba\_versiones\2026-10-08\conmegol`.
**INGENIERÍA INVERSA DE PES.** Verificar en vivo (solo lectura) el gestor de actores `0x1436F3DC8`; encontrar dónde se descuenta o desactiva a un expulsado; hook de Sider en `0x140A72E70` para reemplazar al vigilante; probar los 5 `.geom` de portería a ±15 con un gol; probar GzsTool/FoxTool sobre el `.fox2` del estadio; comparar `.impdata` en memoria vs archivo; instantáneas antes/después de una roja; raya central y líneas finas duplicadas (¿`pitch_alp.ftex` o `line_alp.ftex`?); leer `patterns.h` de Sider para nombrar los 35 puntos de `startup.cache`; clasificador de zonas trata el «círculo central» como cuadrado de 9,15.

## 90.5 Hoja de ruta propuesta (en orden)
1. **Hoy, sin riesgo (solo lectura):** verificar el gestor de actores en vivo; nombrar los puntos de `startup.cache`; decompilar `FUN_1407B05F0` y los manejadores vecinos de los estados 4, 7 y 0xA; escribir las respuestas en la bitácora.
2. **Con tu mano:** poner los repos en privado; prueba del gol con los `.geom` a ±15; decisión sobre descargar GzsTool/FoxTool.
3. **Siguiente prototipo:** módulo de Sider con evento personalizado en `0x140A72E70` (captura el puntero con RCX, escribe las medidas y se desengancha) para sustituir al vigilante externo.
4. **4v4:** una vez comprendido el gestor de actores y el descuento del expulsado, probar desactivar 7 jugadores de campo por equipo con el mismo mecanismo de la roja, en un partido de prueba, con instantáneas y vuelta atrás.
5. **Trucos y animaciones:** MtarTool sobre los 44 `.mtar` y 9.514 `.gani`; usar `FoulScenes.lua` y `shirtless_celebration.lua` como modelos.
6. **Parche propio:** base oficial con IDs propios; perfiles de parche (Phoenix Doctor como punto de partida); decidir Sider 7.4.1.
7. **Mantenimiento del libro:** cada descubrimiento a la bitácora; reconstruir con `node construir_libro.js`; subir a Drive.

## Objetivo aclarado (15-oct, 13:50): N contra N a elección
La hoja de ruta anterior habla de «4v4» por ser el primer caso de prueba. Frank pidió la **libertad de escoger la cantidad** (2v2, 3v3, 5v5…). Cada paso de 90.5 debe leerse como «para un N configurable». El paso 4 pasa a ser: «mecanismo para dejar N jugadores por equipo (con portero), reutilizando la desactivación de actores, más formaciones y reglas para cualquier N».

## 90.6 Preguntas abiertas de ingeniería inversa (resumen; las 20 completas están en cap. 60 parte f)
¿Qué contiene `.impdata` (380 MB)? ¿Dónde se descuenta al expulsado y cómo reacciona el árbitro con menos jugadores? ¿Qué lee exactamente la red de la portería para frenar el balón (física, no los volúmenes de `.data`)? ¿Qué hace el estado 0x10 además de la roja? ¿Hay una constante global de velocidad de juego en PES 2021 (en PES 2019 estaba en 0x1B93C08)? ¿Cómo guarda el juego la opción «Datos Actual. en vivo»? ¿Cuál es el mapa completo de las 30 barras (faltan 2: Cobertura por descarte; sin barra para «Uso de pie malo»)?

## Ampliaciones
(Vacío: añadir con fecha, y reflejar en la bitácora.)
