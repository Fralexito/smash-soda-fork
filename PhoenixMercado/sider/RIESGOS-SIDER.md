# Puente en vivo con Sider — riesgos y cómo se evitan

Este documento acompaña al módulo `phoenix.lua` (prueba 1, solo lectura) y a todo lo que venga después con Sider.
Regla de oro: **cada paso se prueba primero en la PC de Fralex, en partidos offline, y solo después con amigos.**

## Qué se sabe del entorno (revisado el 2026-10-08 en la PC de Fralex)

- 🔎 Sider **7.3.3**; `sider.ini` pide 60 módulos Lua: 49 cargan y 11 no existen. Auditoría completa en `AUDITORIA-SIDER.md`.
- 🔎 El Lua de Sider (LuaJIT 5.1) trae: `io` (leer y escribir archivos), `fs` (`find_files`, `make_dirs`), `memory` (leer/escribir/buscar en la memoria del juego), `ffi`, `zlib`, `match`, `audio`, `os` (solo `date`, `time`, `clock`). **No trae red**: el juego no habla con Internet desde Lua (salvo por `ffi`, que no usaremos para eso).
- 🔎 El overlay de Sider se abre con **Espacio** (`overlay.vkey.toggle = 0x20`), se cambia de módulo con **1** y **º/`**. Solo se dibuja (y solo llama a los módulos) mientras está abierto.
- 🔎 Si un módulo de la lista no existe, Sider lo anota (`PROBLEM: Unable to open file`) y sigue: hoy hay 7 así en `sider.ini` y el juego funciona.
- 🔎 Hay **PESBUL** (`PESBUL_2021_Server_Connector.bat`): servidor comunitario que imita el online de Konami cambiando el archivo `hosts` de Windows. Es decir, el online "de verdad" ya lo da la comunidad; el puente Phoenix tiene que convivir con él.
- 🔎 Otros enganches en el mismo juego: ReShade (`dxgi.dll`), DLSS5 (addons de ReShade), `PES21_Hook`, CreamAPI. Cualquiera puede chocar con algo nuevo; por eso lo nuevo entra de a poco.

## Cómo es el puente

```
Web Phoenix -> Phoenix Link / Phoenix Mercado (PC) -> archivo avisos.txt -> phoenix.lua (dentro del juego) -> overlay
```
El juego no se conecta a nada: solo lee un archivo pequeño que la app de PC deja en `SiderAddons\content\phoenix\`.

## Riesgos y medidas

| # | Riesgo | Qué podría pasar | Medida tomada |
|---|---|---|---|
| 1 | Error dentro del módulo | Mensaje de error, o en el peor caso que el juego se cierre | Todo el trabajo va dentro de `pcall`; tras 5 errores seguidos el módulo se apaga solo y lo anota en `sider.log`. Prueba 1 no usa `memory` ni hooks del partido. |
| 2 | Archivo leído a medio escribir | Texto cortado o raro | La app escribe en `avisos.tmp` y luego **reemplaza** `avisos.txt` de una vez (con reintentos si el juego lo está leyendo). |
| 3 | Archivo gigante o con basura | Lentitud o caracteres extraños | Se leen como máximo 4 KB, 14 líneas, 110 bytes por línea; si no es UTF-8 válido no se muestra; se quitan caracteres de control. |
| 4 | Bajón de rendimiento (FPS) | Tirones | Solo trabaja con el overlay abierto y mira el archivo cada 2 s como mucho. Con el overlay cerrado su coste es cero. |
| 5 | Choque de teclas con otros módulos | Una tecla hace dos cosas | La prueba 1 no usa ninguna tecla propia (MasterLeague usa 0, 6–9, RePág/AvPág, Supr; BallServer, F3…). |
| 6b | Instalar en la carpeta equivocada | El switcher pisa el cambio (pasó el 8 oct: la línea se perdió y el módulo nunca cargó) | Instalar en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`, que es la que copia el switcher. |
| 6 | Romper `sider.ini` | Sider no carga ningún mod | Antes de tocarlo se guarda `sider.ini.respaldo-phoenix`; el cambio es **una sola línea al final** de la lista de módulos. Desinstalar = quitar esa línea o restaurar el respaldo. |
| 7 | Una actualización de ConmeGOL pisa `sider.ini` | El módulo deja de cargarse (no rompe nada) | Documentado: hay que volver a añadir la línea. El instalador de Phoenix lo comprobará en el futuro. |
| 8 | Online con PESBUL / partidas entre dos PCs | Desincronización si los dos juegos no tienen lo mismo | La prueba 1 **solo dibuja texto**: no cambia nada del partido, así que no puede desincronizar. Para módulos que escriban memoria: **apagados en online** y probados solo offline hasta demostrar que son seguros. |
| 9 | Jugar con amigos | Que algo se rompa en sus PCs | Con **Phoenix Link (Parsec)** el juego corre **solo en la PC del anfitrión**: los amigos no instalan nada. Es el modo recomendado para las pruebas. |
| 10 | Escribir memoria en sitios equivocados (fases futuras) | El juego se cierra o guarda datos malos | Antes de escribir: localizar por patrón (AOB) **y** comprobar que lo leído tiene sentido (como hacen StartingYearChanger y Chatty_AutoLineup); nunca escribir con el juego guardando; probar cada valor en una partida desechable; guardados de respaldo. |
| 11 | Juego trampa / ventaja injusta | Desconfianza en la liga | Lo que cambie el partido (dinero, plantillas, reglas) lo decide la **web** (firmado con Ed25519, como `/liga/cambios`), nunca el jugador. |
| 12 | Archivos con nombre raro / rutas | No encuentra el archivo | La ruta se arma desde `ctx.sider_dir` (la que Sider da); la app comprueba que exista `sider.ini` antes de escribir. |

## Fases

1. **Prueba 1 (ahora): leer archivo → overlay.** No toca memoria. Objetivo: comprobar que el puente funciona y que el juego sigue estable.
2. **Prueba 2:** Phoenix Link escribe los avisos solo (resultado de la sala, fichajes confirmados por la web).
3. **Prueba 3:** leer (no escribir) datos del partido con `match` / `ctx` (marcador, equipos) y dejarlos en un archivo para que la app los suba a la web.
4. **Prueba 4:** escribir **un** valor conocido en memoria (p. ej. dinero de la Liga Máster), solo offline, con verificación.

Cada prueba se anota en `PhoenixMercado/PRUEBAS.md`.
