# Perfiles de amigos: más de 32 «Datos personales» en PES 2021

12 de octubre de 2026 · Frente LINK · Estado: **FASES 1, 2 y 3 hechas (motor, base de perfiles e interfaz); compila y enlaza (0.2.51); pruebas con COPIAS; NADA probado todavía con el PES real.**

## 1. El problema

PES 2021 guarda hasta **32 «Datos personales»** (Usuario 1…32). Cada uno lleva el nombre y la configuración de un amigo (botones, cursor, ayudas…). Fralex tiene más de 32 amigos, y el nombre le sirve para saber quién controla a cada jugador.

## 2. Qué se descubrió (con la prueba ZZTEST, 12 oct)

- ✅ Viven en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<SteamID>\save\SYSTEM00000000` (en la PC de Fralex: `D:\Users\Alexander\Documents\...\239200\save`). No en `settings.dat`.
- ✅ **PES no escribe ese archivo al editar el Usuario.** Lo escribe al **terminar un partido** (la primera prueba, sin partido, no cambió nada; la segunda, con partido, sí).
- ✅ El archivo está cifrado igual que EDIT/ML. Link ya sabe abrirlo (mismo cifrado). Descifrado: 219.568 bytes.
- ✅ Tabla **fija de 32 registros de 2.432 bytes desde el byte 0x50**; justo detrás (0x13050) siguen otros datos. Un registro: marca (0/1), **nombre de 48 bytes**, y desde +0x34 la configuración (mapa de botones, cursor, ayudas, mapas «Personalizado»…).
- ✅ El nombre «ZZTEST» apareció en el hueco 22 y cambiaron 4 bytes de la configuración (el botón que se cambió).
- ✅ **Ampliar el 32 de verdad exigiría cambiar el formato y el ejecutable** (la tabla está pegada a los demás datos). Descartado.
- ❓ Sin probar: que PES acepte un `SYSTEM00000000` escrito por Link (para EDIT y ML el juego no valida la cabecera; para SYSTEM falta probarlo). También si lee el archivo en vivo o solo al arrancar.

## 3. Plan (opción B, aprobada por Fralex)

Link guarda un **perfil por amigo** (nombre + configuración) en su propia base y lo vuelca a uno de los 32 huecos **antes de abrir PES**; al terminar, recoge los cambios que el amigo hizo.

| Fase | Qué | Estado |
|---|---|---|
| 0 | Localizar el archivo (ZZTEST) | ✅ |
| 1 | Módulo de Link `core/PerfilesPes`: leer los 32, exportar/importar un Usuario, escritura segura | ✅ con COPIAS (`pruebas-link/correr_perfiles.bat`) |
| 2 | Base de perfiles (por nombre), huecos protegidos, traer (PES a Link) y preparar sala (Link a PES): `core/PerfilesAmigos` | ✅ con COPIAS (`pruebas-link/perfiles_amigos_test.cpp`) |
| 3 | Interfaz GENTE › PERFILES PES (`web/AccionesPerfilesWeb.cpp`, `ui/js/vistas/perfiles.js`, simulador) | ✅ vista probada en el simulador |
| 4 | Pruebas en el juego (con permiso de Fralex) y guía | pendiente |

## 4. Fase 1: qué hace el módulo (`SmashSoda/phoenix/core/PerfilesPes.*`)

- `Guardado::abrir` lee y valida el `SYSTEM00000000` (cabecera del sobre, tipo SYSTEM, PES 2021, 219.568 B, los 32 registros con forma de Usuario). Si algo no cuadra, **no hace nada**.
- `usuario(i)` / `usuarios()` leen nombre y registro; `aPerfil` / `dePerfil` exportan e importan un perfil en **JSON propio de Phoenix** (`{formato:1, juego:"PES2021", nombre, tam, registro:"<hex>"}`).
- `ponerUsuario` / `rotular` cambian un hueco en memoria. Solo cambia ese registro; el resto del archivo queda idéntico (probado byte a byte).
- `guardarComo` escribe un archivo nuevo (nunca sobrescribe) y lo relee. `reemplazar` hace **copia `SYSTEM00000000.phoenix-<sello>`** al lado, escribe un temporal, lo relee y valida, y solo entonces reemplaza (todo o nada).
- Pruebas (con una COPIA real): abrir y listar 32; cifrar sin cambios da **exactamente el mismo archivo**; exportar → importar → exportar es idempotente; los otros 31 Usuarios y los datos de después de la tabla no cambian; rechaza hueco 33, registro corto o corrupto, archivo truncado, basura, archivo inexistente, perfil de otro formato; no deja temporales; el original no se toca.

## 5. Reglas para las fases siguientes

- **Solo con PES cerrado** (salvo que se pruebe que PES relee el archivo en vivo).
- Nunca escribir en los guardados reales de Fralex sin su permiso explícito y copia previa.
- Huecos **protegidos** (los que Fralex quiera conservar tal cual) y huecos **rotativos** (p. ej. los últimos 8–16).
- Si dos amigos piden el mismo hueco, gana el primero y el otro va a otro hueco libre; si no hay, se avisa (máximo 32 jugando a la vez).
- **Nunca se pierde una configuración**: antes de pisar un hueco rotativo, su contenido se recoge a su perfil o se guarda copia.
- El nombre de los amigos **no** se sube a ningún sitio ni se escribe en esta documentación.

## 6. Cómo reproducir las pruebas

```
pruebas-link\correr_perfiles.bat "<copia de SYSTEM00000000>"
```
Trabaja solo sobre una copia en una carpeta temporal. Resultado esperado: 32 líneas «Usuario N» y `TODO OK`.

## 7. Cómo se usa (GENTE › PERFILES PES)

1. **TRAER DE PES** (solo lee PES): guarda en Link cada Usuario de PES. Hazlo al principio y cuando cambies algo en PES. Link también lo hace solo cuando PES se cierra (mientras esa pestaña esté abierta).
2. Marca el candado 🔒 en tu(s) hueco(s): Link no los toca.
3. **¿QUIÉNES JUEGAN HOY?**: marca a los amigos (Link recuerda la última elección). Caben tantos como huecos sin proteger.
4. Con **PES cerrado**, **PREPARAR LA SALA**: Link trae primero (nada se pierde), hace copia con sello del guardado (`SYSTEM00000000.phoenix-<sello>`) y pone a los elegidos en los huecos libres de atrás hacia delante. Si ya estaban en un hueco con el mismo contenido, no escribe nada.
5. Abre PES y, en la pantalla de elegir mandos, elige tu nombre con L2/R2.

Qué se guarda: `perfiles-amigos.json` y `ajustes.json` en `%APPDATA%/Trybuchet/Smash Soda/phoenix-perfiles-amigos/` (+ carpeta `respaldos` con las 20 últimas copias). Un perfil se identifica por su NOMBRE; un Usuario sin nombre se guarda como «Usuario N».

## 8. Lo que falta

- **Probarlo en el PES real** (Fralex lo autoriza y se hace con copia previa): que PES acepte un `SYSTEM00000000` escrito por Link y que el Usuario con ese nombre aparezca en la lista.
- Vincular un perfil a un Parsec ID (hoy se identifica por nombre).
- Que PES lo recargue en vivo (hoy solo con PES cerrado).

## 9. Siguiente paso: que el Usuario se ponga solo al sentarse el amigo (investigación)

Lo que quiere Fralex: si el amigo «Erfuzz» se sienta en el mando 7, el mando 7 usa su Usuario de PES (nombre, tipo de pase, de disparo, guías...) sin hacer nada.
Camino seguro (sin escribir en la memoria de PES): (1) encontrar en la memoria, **solo leyendo**, el número de Usuario que tiene cada mando en la pantalla de elegir lados; (2) Link, que controla el mando virtual de cada puesto, pulsa **L2/R2** en ese mando hasta que el número coincida con el hueco del amigo (lo que dejó «Preparar la sala»), y se detiene.
Paso 1: `tools/radar/usuariopad.cpp` (compilar con `tools/radar/compilar_usuariopad.bat`): foto, R2, foto, R2... y deja los bytes que suben +1 cada vez. Sin resultados todavía.

### 9.1 Resultado de la investigación del número de Usuario (12 oct, 14:50, PC de Fralex)
- ✅ `PES2021.exe+0x37F18E4` (dirección fija 0x1437F18E4 con el exe en 0x140000000; igual en dos arranques) vale el **número de Usuario de 0 a 31** de la pantalla de elegir lados: R2 suma 1 y de 31 pasa a 0. Empezó en 21 = Usuario 22 y acabó en el Usuario 32 (DEGOX), como se anotó en pantalla.
- Estructura vecina (u16): `+0 = 7`, `+2 = 7`, `+4 = índice actual`, `+6 = índice anterior`, `+8 = 32 (nº de Usuarios)`. Parece el estado de la lista de Usuarios de la interfaz.
- ❓ Falta probar con DOS mandos si ese valor es el Usuario del mando que acaba de pulsar (lo necesario para el bucle «pulsar R2 hasta llegar al hueco») o solo de uno.
- Las otras dos posiciones (0x1437F1844 y 0x143AAC364) son contadores que no dan la vuelta; no sirven.
- Plan seguro: Link solo LEE esa posición y pulsa L2/R2 con el mando virtual del puesto hasta que coincida con el hueco del amigo (el que dejó «Preparar la sala»). Solo en la pantalla de elegir lados (si no, R2/L2 hacen cosas en el partido): hace falta una señal fiable de esa pantalla.

### 9.2 Prueba con DOS mandos (12 oct, ~15:00): 0x1437F18E4 NO es el Usuario de cada mando
- R2 alternando mando 1 y mando 2 hizo subir `0x1437F18E4` siempre +1 (16, 17 … 25), sin saltar entre dos valores. Los Usuarios finales fueron AXEL y JACK (casillas 15 y 16, valores 14 y 15), no el 25.
- Conclusión: es el cursor de la lista de la pantalla, COMPARTIDO entre mandos; no sirve para saber el Usuario de cada uno. Además PES salta los Usuarios que ya usa otro mando.
- Para el modo automático haría falta la posición propia de cada mando (dos búsquedas más, moviendo solo un mando cada vez: lo que cambia en ambas es el cursor compartido; lo que cambia solo en una es de ese mando).
- Alternativa sin riesgo, ya disponible: «Preparar la sala» deja el Usuario de cada amigo listo en PES y cada uno pulsa R2 hasta ver su nombre.
