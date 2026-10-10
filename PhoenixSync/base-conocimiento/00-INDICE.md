# Base de conocimiento · PES 2021 y sus parches

**Qué es:** todo lo que Phoenix Sync sabe de los archivos de PES 2021, ordenado como una base de datos.
Se **alimenta en cada auditoría** y nunca se borra lo aprendido: si algo cambia en una versión nueva del parche, se anota la fecha y el motivo.

**📘 Empieza por `../MANUAL-PARCHE-PHOENIX.md`**: el manual que reúne todo para construir el parche (visión, cómo funciona el juego, recetas, reglas de oro, ruta y futuro).

**Cómo está dividida:**

| Archivo | Qué guarda |
|---|---|
| `01-MOTOR-PES2021.md` | Lo que es del **juego** y vale para **cualquier parche**: cómo están armados el option file, el guardado de Liga Máster y el bloque comprimido. Tablas con posición, tamaño, campo y estado. |
| `02-PARCHE-PHOENIX.md` | **El objetivo de fondo:** anatomía de un parche de PES 2021, qué ya controlamos y qué falta para fabricar el **parche Phoenix** vinculado a la web, a Phoenix Link y al Mercado. |
| `03-INVESTIGACION-PARCHES.md` | **Lo que enseñan los demás parches** (FL, Evoweb, ConmeGOL, Sudamerican, Gogosz, PESBUL): causas de crash y la regla Phoenix para cada una, modo online realista, herramientas y formatos para fabricar el parche, y la receta del Phoenix Evolution Patch. Informes completos en `investigacion/`. |
| `04-ESTRATEGIA-PHOENIX.md` | **Informe estratégico**: ConmeGOL y Sudamerican por dentro (bases de datos comparadas, contenido, Sider), sus estrategias y sus fallos, cómo superarlos en estabilidad y rendimiento, funciones nuevas por viabilidad (🟢🟡🔴), límites del motor y ruta completa por fases. |
| `05-PUERTA-EN-VIVO.md` | **🏆 Cambiar la base sin reiniciar** (2026-10-09): Player.bin propio servido por Sider + **Editar → Cargar**. Qué falló y por qué, cuándo lee el juego cada archivo, el universo que abre (fichajes, DT, competiciones, forma semanal) y los deseos a futuro (crear jugadores, avisos nativos). |
| `06-RUTA-DEL-DESCUBRIMIENTO.md` | **El relato en orden** de la noche del 8-9 de octubre: cada intento, por qué, qué salió bien, **qué salió mal** (incluidos los errores de Claude), aciertos, errores con su lección y el método de trabajo que funcionó. |
- [07 · El motor por dentro](07-MOTOR-POR-DENTRO.md): interruptor nativo de recarga y mapa de la actualización en vivo de Konami.
- [17 · «Datos Actual. en vivo»: dónde está la opción](17-OPCION-EN-VIVO-DONDE-ESTA.md): no se guarda en disco; es el modo de carga del gestor de la base (`[[exe+0x3705E10]+0x38]`, 1 = activada); lectura segura probada con la v0.17m.
- [19 · «Datos Actual. en vivo»: informe paso a paso](19-OPCION-EN-VIVO-PASO-A-PASO.md): todo lo que se hizo el 2026-10-09 por la noche, en orden, con horas, errores, simulaciones e instalaciones (v0.17m y v0.17a).
- [22 · Fase D: zona del juego y momento seguro](22-ZONA-DEL-JUEGO-MOMENTO-SEGURO.md): el gestor de modos (`[[exe+0x3704E38]+0xF0]`: 7 = TOP_MENU, 8 = EXHIBITION, 13 = EDIT…), la regla del «momento seguro para recargar» y la prueba con la v0.17d.
- [30 · Experimento B (v0.17e-B): cómo replicarlo desde cero](30-EXPERIMENTO-B-COMO-REPLICAR.md): botón nativo solo con los parches A y B (sin C) → el juego se queda en «Activar». Bytes exactos, los 3 cambios sobre la v0.17e, sha256, instalación, prueba, resultados corregidos y problemas conocidos.
- [31 · v0.18: los tres modos de recarga](31-MODOS-DE-RECARGA.md): ACTIVAR (por defecto) · AUTO-FICHAJES · AUTO-SIEMPRE. Dónde se elige (`content\phoenix\modo.txt`, botón de Link, `PhoenixModo.bat`, tecla T), cómo funciona por dentro, cuándo se aplica cada uno, simulación 9/9 y lo que falta probar.
| `parches/conmegol-26.md` | Lo que es **solo del ConmeGOL Patch 26**: carpetas, archivos, números de equipos y jugadores, IDs, competiciones, rarezas vistas. |
| `parches/sudamerican-2026.md` | Lo poco que ya se sabe del Sudamerican Project 2026 (pendiente de auditar). |
| `datos/motor.json` | La misma información del motor, en formato de datos, para que el software la lea. |
| `datos/conmegol-26.json` | Equipos, IDs y competiciones del ConmeGOL 26 sacados del guardado (627 equipos, 39 competiciones). |
| `../sider/RIESGOS-SIDER.md` | **Puente en vivo con Sider**: qué permite el Lua de Sider (archivos, memoria, overlay), cómo es el puente web → app → archivo → juego, los 12 riesgos con su medida y las fases de prueba. |
| `../sider/AUDITORIA-SIDER.md` | **Auditoría de los 49 módulos Sider activos**: qué toca cada uno, choques encontrados (cámaras que se pisan, escrituras a ciegas), módulos que no hacen nada y plan de limpieza. Detalle en `../sider/auditoria/`. |
| `../PRUEBAS.md` | Diario de prueba y error: qué se probó en el juego, qué falló y cómo se corrigió. |
| `../liga-master/ESTRUCTURA-ML.md` | El cuaderno técnico original (más detalle, menos orden). |

**Estados que se usan en todas las tablas:**

- ✅ **PROBADO**: se cambió en el archivo y se vio el resultado en el juego.
- 🔎 **OBSERVADO**: se dedujo comparando archivos (antes/después) o contra la pantalla, sin haberlo editado aún.
- ❓ **HIPÓTESIS**: idea razonable, sin comprobar.
- ❌ **DESCARTADO**: se buscó y no está, o no funciona así.

**Regla de oro:** nada pasa a ✅ sin una prueba en el juego anotada en `PRUEBAS.md`.

**Cómo auditar un parche nuevo** (Sudamerican, próximas versiones de ConmeGOL…):
1. Copia de seguridad de `EDIT00000000` y de los `ML…` de ese parche (carpeta `save` con su número de Steam).
2. Generar el catálogo con `PhoenixSync.exe catalogo` (option file + `.cpk` de la base).
3. Correr los prototipos de `liga-master/prototipos/` sobre un guardado descifrado: `bloques.py` (bloques de alineación), `blob.py` (bloque comprimido) y la sonda del C++ (`tablasDe`, `alineacionDe`).
4. Anotar diferencias respecto al motor en `parches/<parche>.md` y volcar sus datos a `datos/<parche>.json`.
5. Repetir las pruebas clave en el juego (vender, traspaso IA↔IA, presupuesto) y anotarlas en `PRUEBAS.md`.
