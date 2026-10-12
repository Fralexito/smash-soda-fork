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

### 9.3 La copia de los Usuarios que PES tiene en memoria (12 oct, ~15:05)
- ✅ `usuariopad.exe buscar FRALEX` encontró UNA copia real: el registro del Usuario 1 con su configuración (`01 03 02 00 04 06 05 07 08 08 …`) en una región de memoria RW (dirección alta tipo 0x7FF4…, cambia en cada arranque). Es la tabla de 32 registros de 2.432 B tal como está en el archivo SYSTEM00000000 descifrado.
- Prueba de escritura REVERSIBLE (con permiso expreso de Fralex): `usuariopad.exe escribir <nombre> <casilla> <casilla vacía> [texto]` localiza la tabla por el nombre, valida los 32 registros, escribe un texto SOLO en el nombre (48 B) de una casilla vacía y lo restaura al pulsar Numpad 8, al cerrar la ventana o al cerrarse PES. No toca ningún archivo.
- ❓ Resultado pendiente: si PES muestra ese nombre en la lista de Usuarios sin reiniciar.

### 9.4 RESULTADO: la escritura en vivo FUNCIONA (12 oct, ~15:20)
- ✅ `usuariopad.exe escribir FRALEX 1 31 ZZMEM`: se escribió «ZZMEM» en el nombre del Usuario 31 en la copia que PES tiene en memoria y **apareció al instante sobre el mando en la pantalla de elegir lados**; al restaurar (Numpad 8) desapareció. No se guardó nada en archivos. Tabla en 0x7FF4FBD60010 en ese arranque (cambia en cada arranque; se localiza buscando un nombre conocido y validando los 32 registros).
- Conclusión: PES lee los Usuarios en vivo de esa tabla. Un módulo (o Link) puede poner el perfil de un amigo (registro de 2.432 B) en un Usuario con PES abierto.
- Falta: saber qué Usuario tiene cada mando (para elegir el hueco). Siguiente paso: `usuariopad.exe punteros` (solo lee) busca las referencias a cada registro.

### 9.5 RESULTADO: dónde está «el Usuario de cada mando» (12 oct, ~15:45, PES 2021 ConmeGOL, PC de Fralex)
- ✅ **Lista de 8 u32 = índice de Usuario (0..31) de cada uno de los 8 mandos.** Se encontró con la herramienta `pares` (dos estados, 4 Usuarios con nombre: una sola pareja candidata) y se confirmó con R2: el mando 1 cambia la entrada 0 (2→4→8) y el mando 2 la entrada 1 (31→0→2); las otras 6 no se mueven.
- ✅ PES **salta los Usuarios que ya tienen los otros mandos** (los de los mandos sin usar: 1,3,30,5,6,7…), por eso R2 hace 2→4, 4→8, 0→2.
- ✅ **Ancla fija**: `PES2021.exe + 0x36F5820` (0x1436F5820 con el exe en 0x140000000) contiene un puntero de 8 bytes a esa lista, y `+0x36F5828` un u32 = **8** (nº de mandos). Comprobado en vivo (dos lecturas). La lista está en el heap (cambia por arranque), la ancla no.
- ✅ **Configuración del Usuario: una sola copia** (la tabla de 32×2.432 B). 0 copias fuera de la tabla (`copias`). La tabla está en el heap; no hay ancla estática (4 referencias, todas en el heap); se localiza escaneando regiones RW y validando 32 registros.
- ✅ **Escribir en la tabla se ve al instante** (prueba ZZMEM, 15:07).
- ❓ Sin probar: que un REGISTRO COMPLETO escrito en la tabla cambie también las OPCIONES en el partido (cursor, tipo de pase…), no solo el nombre; que el orden de entradas = puesto N de Link (probable por «Orden de mandos en PES»: casilla N = puesto N); qué pasa con la tabla al guardar PES (se guardaría el Usuario sobrescrito en SYSTEM00000000: por eso hay que restaurar el original al terminar).
- Plan seguro de «Usuario automático»: (1) Link hace TRAER (copia de todo). (2) En la pantalla de elegir lados, para cada puesto con un amigo con perfil: leer el índice de su mando (ancla + entrada N-1) y escribir su perfil en ese registro de la tabla (todo o nada: leer, escribir, releer). (3) Al terminar o salir el amigo, restaurar el registro original. Solo en la pantalla de lados (señal de `LadoPantalla`).

## 10. Prueba en vivo en Link 0.2.53 (beta): «Usuario automático», primer paso
- `core/UsuariosEnVivo.*`: se engancha a PES2021.exe, lee el Usuario de cada mando (ancla `exe+0x36F5820`), localiza la tabla de 32 Usuarios y escribe el registro de UN Usuario con relectura (si no coincide, lo deja como estaba).
- Acciones: `perfiles.mandos` (solo lee), `perfiles.aplicar {puesto, id}` y `perfiles.restaurar`. Antes de pisar un Usuario guarda lo que había como perfil de Link y en memoria (para restaurar).
- Interfaz: GENTE › PERFILES PES › «PRUEBA EN VIVO (BETA)»: elegir puesto y perfil → PONER EN EL MANDO; VER USUARIO DE CADA MANDO; RESTAURAR LO ANTERIOR.
- Prueba manual de solo lectura: `pruebas-link/correr_usuarios_vivo.bat` (con PES abierto).
- ❓ Sin probar con el PES real: que el registro COMPLETO cambie las opciones del partido; que puesto N de Link = mando N de PES.
- Falta: aplicarlo solo al sentarse el amigo (señal de la pantalla de lados), el perfil del amigo nuevo que viene de su cuenta (web) y el editor de la configuración.

### 10.1 Prueba REAL con PES abierto (12 oct, 16:40, Link 0.2.57, hecha por Claude sobre el PC de Fralex con su permiso)
- ✅ «VER USUARIO DE CADA MANDO» lee bien: P1 FRALEX, P3 ARENZ… coincide con lo que PES muestra sobre cada mando. Hubo dos fallos reales, ya corregidos: (1) Link elegía una **tabla falsa** (región con 0xFF en los nombres): `tablaValida` ahora exige nombres UTF-8 válidos y mapa de botones con valores ≤ 0x10; (2) el valor **32 en la lista de mandos = «ese mando todavía no tiene Usuario»** (aparece al reconectarse los mandos virtuales de Link): se trata como -1, no como error.
- ✅ **PONER EN EL MANDO (puesto 1, perfil AXEL): PES mostró «AXEL» sobre el mando 1 en la pantalla de elegir lados**, sin reiniciar. «RESTAURAR LO ANTERIOR» devolvió FRALEX (comprobado leyendo la memoria: un solo registro FRALEX, AXEL solo en su casilla).
- ❓ NO comprobado: que las OPCIONES (tipo de pase, disparo, guías, botones) se vean/apliquen — hace falta abrir «Editar Datos personales» (□) con ese mando; con el teclado no se encontró la tecla del cuadrado (la S cambió de Usuario). El registro escrito es el completo (2.432 B) y Link verifica por relectura que quedó idéntico al perfil.
- Observaciones: al reiniciar Link, PES reasigna los Usuarios de los mandos (el orden de la lista de mandos cambió: 0,5,1,2,32,3,6,4). Hay que releer la lista siempre antes de escribir (ya se hace). PES no deja dos mandos con el mismo Usuario.

### 10.2 RESULTADO FINAL de la prueba real (12 oct, 16:45, Link 0.2.57): las OPCIONES también se aplican en vivo
- Tecla del botón cuadrado en el teclado de PES = **C** (la dijo Fralex); **Z** = Atrás; **S** cambia de Usuario (R2). Los códigos de tecla están en `settings.dat` (desde +0x34, formato `10 <scancode>`).
- ✅ Con el mando 1 en FRALEX, «Editar Datos personales» mostraba: ayuda de pases Nivel 2, guía de dirección No, guía de posicionamiento No, amago automático No. Tras **PONER EN EL MANDO (puesto 1, perfil AXEL)** mostró: nombre AXEL, ayuda de pases **Nivel 1**, guía de dirección **Sí**, guía de posicionamiento **Sí**, amago automático **Sí**. Es decir: **el registro completo escrito en la tabla cambia nombre Y opciones en vivo, sin reiniciar PES.**
- ✅ RESTAURAR LO ANTERIOR devolvió FRALEX («Usuarios restaurados como estaban»).
- Aprendido: la lista de 8 mandos solo existe en la pantalla de ELEGIR LADOS (en «Editar Datos personales» el puntero es 0). Link 0.2.58: si PES está abierto pero esa lista no existe, «Restaurar» NO olvida lo pendiente (antes lo habría olvidado) y pide volver a esa pantalla.
- ❓ Sin comprobar: el efecto de esas opciones DENTRO de un partido (el juego las lee de la tabla al empezar; es lo esperable). Tampoco el puesto N de Link = mando N de PES con mandos reales de amigos.
- Falta: aplicarlo solo al sentarse el amigo (señal de pantalla de lados) y de dónde sale el perfil del amigo nuevo (su cuenta/web) + editor de la configuración.

### 10.3 Puesto 2 = mando 2 (12 oct, 16:57, Link 0.2.58, prueba hecha por Claude con el mando 2 de Fralex en el lado Visitante)
- ✅ «PONER EN EL MANDO» con Puesto 2 y perfil JACK: PES mostró «JACK» sobre el **segundo mando** (el rosa, columna Visitante); el mando 1 siguió en FRALEX. **Puesto N de Link = entrada N-1 de la lista de mandos = mando N de PES** (con el orden que deja «Orden de mandos en PES»).
- ✅ Restaurar OK. La lista de mandos cuenta con el mando fuera del lado Local también (los Usuarios siguen por número de mando, no por lado).
- Nota de uso de la interfaz: la primera pulsación en Link tras dar foco a su ventana a veces solo da foco; si «Poner en el mando» no abre el aviso, pulsar otra vez.
- Siguiente: aplicarlo SOLO al sentarse el amigo (ver §9.5, plan) y el vínculo persona → perfil.

## 11. MAPA DE OPCIONES de un Usuario de PES 2021 (registro de 2.432 B) — decodificado el 12 oct con «Editar Datos personales» + volcado de memoria
Método: en la pantalla de edición (tecla C = cuadrado, Z = Atrás, flechas = D-pad) se cambia una opción, se SALE de la pantalla (PES guarda la edición al salir; dentro no cambia la memoria) y se vuelca el registro con `usuariopad registro` comparando con el anterior. Verificado cambiando las 12 opciones de FRALEX y restaurándolo (registro idéntico al original).

| Offset | Opción en pantalla | Valores vistos |
|---|---|---|
| +0x00 | marca | 0 / 1 |
| +0x04 (48 B) | **Datos personales (nombre)** | texto UTF-8 terminado en 0 |
| +0x34…+0x47 | **Configuración de comandos** (mapa de botones, 2×10 B) | Normal = `01 03 02 00 04 06 05 07 08 08 / 01 03 02 00 07 05 04 06 07 07` |
| +0x48 | Cambiar cursor | 1 = Semiasistido, 2 = Sin ayuda (0 = ¿Asistido?) |
| +0x4B | Cursor de siguiente jugador | 0 = Sí, 1 = No |
| +0x4C | Tipo de cursor | 4 = «Datos personales», 0 = Nombre del jugador |
| +0x4D | Nv. de ayuda en pases | 1 = Nivel 1, 2 = Nivel 2 |
| +0x4E | Tipo pase en profundidad | 0 = Básico, 1 = Avanzado |
| +0x4F | Tipo de disparo | 0 = Básico, 1 = Avanzado |
| +0x50 | Guía de dirección | 0 = Sí, 1 = No |
| +0x51 | Guía de objetivo | 0 = Sí, 1 = No |
| +0x52 | Guía de posicionamiento | 0 = Sí, 1 = No |
| +0x53 | Amago automático | 0 = Sí, 1 = No |
| +0x54 | Entrada automática | 1 = Semiautomático, 2 = No (0 = ¿Automático?) |
| +0x55 | Controles de compañero | 0 = Asistido, 1 = Manual |
| +0x49, +0x4A, +0x56… | sin identificar (valores 1,1 / 0 / 1…) | no tocar: se conservan al copiar el registro |

- ❓ Pendiente de identificar: los valores de otros textos de «Tipo de cursor», el valor 0 de «Cambiar cursor» y de «Entrada automática», y los presets «Alternativa/Personalizado» de la configuración de comandos (bytes +0x34…+0x47).
- Consecuencia para el editor de Link: un perfil nuevo se construye **copiando un registro base real** (p. ej. el de cualquier Usuario sin nombre) y cambiando solo estos bytes y el nombre; así los campos desconocidos quedan como PES los espera.

### 11.1 Configuración de comandos (subpantalla, 12 oct) — pedido de Fralex
- ✅ **+0x32 = Desplazamiento de jugadores** (0 = stick izquierdo, 1 = cruceta). **+0x33 = preajuste** (0 = Normal, 1 = Alternativa; «Personalizado» se espera = 2 y usaría el mapa de +0x34…+0x47; sin comprobar). Cambiar a «Alternativa» NO toca los bytes +0x34…+0x47: el preajuste solo cambia +0x33.
- Mapa de botones (+0x34…+0x47): códigos × = 0, ○ = 1, □ = 2, △ = 3, L1 = 4, R1 = 5, L2 = 6, R2 = 7, 8 = ninguno. Primer bloque (Ataque) de «Normal»: `01 03 02 00 04 06 05 07 08 08` = Pase bombeado ○, Pase en profundidad △, Disparo □, Pase raso ×, Cambiar cursor L1, Controles manuales L2, Esprintar R1, Controles especiales R2. Segundo bloque (Defensa): `01 03 02 00 07 05 04 06 07 07` (los 4 primeros = Entrada deslizante ○, Salir con el portero △, Presión compañeros COM □, Presión ×; el resto sin decodificar).
- Nota de uso del teclado en PES (mi método): en el cuadro del NOMBRE el teclado escribe letras (la Z se añadió al nombre; se borró con retroceso); la primera pulsación tras cambiar de pantalla a veces se pierde; hay que comprobar la fila resaltada antes de Enter.
- Nota de Fralex: los nombres de las opciones pueden variar de un PES o parche a otro (idioma/versión): el editor de Link debe trabajar con los BYTES y tener sus etiquetas por parche, no fiarse de los textos.


## 12. Usuario automático (Link 0.2.59)

Objetivo: que quien se siente en un puesto tenga SU nombre y SU configuración en el Usuario de su mando, sin hacer nada más.

- **Vínculo**: cada perfil de Link puede ligarse a una persona por su **identificador de Parsec** (no por el nombre de cuenta; Fralex: el nombre real lo decide la propia persona). Se guarda en `perfiles-amigos.json` (`parsecId`). Una persona tiene un solo perfil: ligar otro quita el anterior. La interfaz está en GENTE › PERFILES PES › USUARIO AUTOMÁTICO (select por cada sentado, o por ID manual).
- **Vigilante** (`tickPerfilesAuto`, 1 vez por segundo, solo con PES abierto y en la pantalla de elegir lados, que es cuando existe la lista de mandos): para cada sentado con perfil, escribe el perfil en el Usuario de su mando (puesto N = mando N). Antes guarda lo que había como perfil si no estaba. Al levantarse o cambiar de persona o de Usuario, devuelve el registro original (con relectura). Si la persona cambió sus opciones en PES, ese cambio se guarda en SU perfil en vez de pisarlo.
- **Interruptor** ACTIVAR/APAGAR AUTOMÁTICO (`perfiles.auto`, ajuste `autoUsuarios`, activo por defecto). Si hay Usuarios pisados al apagarlo, los devuelve.
- **Sentados de prueba** (`perfiles.simular {puesto, parsecId}`): simulan a alguien sentado sin gente de verdad; sirven para probar.
- **Prueba real (12 oct 2026, PC de Fralex)**: perfil «Usuario 2» ligado a un ID falso, sentado de prueba en el puesto 1 → PES mostró «Usuario 2» en el mando 1 a los ~2 s; al quitar el sentado volvió «FRALEX» idéntico.
- Acciones nuevas: `perfiles.vincular {id, parsecId}`, `perfiles.auto {si}`, `perfiles.simular {puesto?, parsecId?}`. `perfiles.estado` añade `auto`, `autoLog`, `sentados`, `autoAplicados` y `parsecId` por perfil.
- Pendiente: «Mi perfil de PES» (editor de nombre y opciones por bytes, etiquetas por parche) y que el perfil viaje con la persona a otros anfitriones.

### 12.1 Mi perfil de PES: formulario con nombre y opciones (Link 0.2.61)

- En GENTE › PERFILES PES › MIS PERFILES: botones **+ NUEVO PERFIL** y **EDITAR**. El formulario es el «Editar Datos personales» de PES: nombre, **configuración de comandos** (preajuste, desplazamiento y, si es Personalizado, los 20 botones), **cambiar cursor**, cursor de siguiente jugador, **tipo de cursor** (siempre «Datos personales», fijo), nivel de ayuda en pases, pase en profundidad, tipo de disparo, guías, **amago automático**, **entrada automática**, **controles de compañero**, y el ID de Parsec opcional para ligarlo.
- Lógica en `core/OpcionesUsuario.{h,cpp}` (solo bytes; las etiquetas viven en `vistas/perfiles.js`, por parche). Acciones `perfiles.opciones{id?}` y `perfiles.editar{id?, nombre, parsecId?, opciones}`. Un perfil nuevo parte de un registro real de la base y cambia SOLO el nombre y los bytes conocidos (el resto se conserva).
- **Corrección importante**: el campo del nombre (48 B desde +0x04) comparte sus 2 últimos bytes con las opciones +0x32 (desplazamiento) y +0x33 (preajuste). Antes, poner un nombre los borraba. Ahora solo se borran los primeros 46 bytes (`ponerNombre`, `renombrar` y `Guardado::rotular`).
- Si se edita el perfil de alguien que ya está sentado, el vigilante le pone la versión nueva.
- **Prueba real (12 oct 2026)**: perfil «PRUEBA LINK» ligado a un ID falso y sentado de prueba en el puesto 1 → PES mostró ese nombre en el mando 1 y, en Editar Datos personales, las opciones elegidas; al quitar el sentado volvió FRALEX; el perfil de prueba se borró. Los valores de FRALEX en el formulario coincidieron con su pantalla de PES.
- Sin comprobar aún: «Cambiar cursor» = 0, «Entrada automática» = 0 y el preajuste Personalizado (=2) con su mapa de botones; las etiquetas de los botones de defensa a partir del 5.º.

### 12.2 Salas abiertas dentro de Link (0.2.62)

- Pedido de Fralex: ver en Link las mismas salas que muestra la web, para entrar sin salir de la app. GENTE › **SALAS ABIERTAS**: tarjetas con anfitrión, juego/parche/región, plazas libres, calidad (semáforo y ping), etiquetas TE INVITARON / AMIGO / EN PARTIDA / INESTABLE y botón **UNIRME** (abre el enlace de Parsec; apagado si es tu sala, está llena o no tienes acceso).
- Datos: `GET /v1/salas` (contrato §17 de la web: mismas reglas de visibilidad que la página). `PhoenixLink::pasoSalas` sondea cada `sondeo_seg` (25 s) con ETag y SOLO con la ventana visible (§21, por el coste de llamadas). Solo se ofrece entrar con un enlace `https://`. Estado de la interfaz: `salasAbiertas{cargadas, lista[]}`.
- No se creó ninguna acción nueva: UNIRME usa `ui.abrir`.

### 12.3 Nuevo orden de pestañas (Link 0.2.63)

Reagrupado a pedido de Fralex (GENTE era un cajón de sastre). Solo cambia dónde cuelga cada pestaña; los nombres internos y las acciones no cambian.

- **SALA:** Resumen · **Salas abiertas** · Opciones de sala · Juegos · Conexión · Actividad. El Resumen con la sala cerrada muestra el botón «SALAS ABIERTAS (N)» si hay salas donde entrar.
- **MANDOS:** Puestos · **Perfiles PES** · Tu mando · Mapeo · Teclado · Turnos · Bloqueo.
- **GENTE:** En sala · Moderación · Amigos · Permisos.
- PARTIDO, SYNC y AJUSTES igual.
- Donde en este documento dice «GENTE › PERFILES PES» o «GENTE › SALAS ABIERTAS», ahora es **MANDOS › PERFILES PES** y **SALA › SALAS ABIERTAS**. El estado `salasAbiertas` llega con la sección Sala visible.
