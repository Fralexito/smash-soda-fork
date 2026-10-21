# Link 0.3.41 a 0.3.45: vincular la cuenta y Sync más sencillo (21-oct-2026)

Guía paso a paso para Fralex, testers y cualquier IA que retome el trabajo. Rama `plan-ux` del repo `Fralexito/smash-soda-fork` (worktree `C:\dev\smash-soda-fork\_phoenix-link`).

## 1. Qué cambió

| Versión | Qué | Commit |
|---|---|---|
| 0.3.41–0.3.43 | Hasta 16 puestos (experimental): «Puestos máximos», puestos 9+ en `phoenix_mandos_extra.dat`, contador de Xbox hasta 16. Guía: `docs/MAS-DE-8-MANDOS.md`. | `06a673d` |
| 0.3.44 | Arreglo: no se podía vincular la cuenta (la web lo rechazaba todo). | `06a673d` |
| 0.3.45 | SYNC › TRASPASOS más sencillo: arriba solo el código y recibir; los interruptores juntos en «Ajustes de traspasos». | `d14a128` |

## 2. Vincular la cuenta (arreglo 0.3.44)

**Qué pasaba.** Desde el 19-oct (~17:00 Lima) la web contestaba `CAMPO_INVALIDO «x-phoenix-build»` a todo lo que mandaba Link: vincular, chat, presencia, salas, buzón del juego.

**Por qué.** La cabecera `X-Phoenix-Build` lleva la huella SHA-256 del exe (64 caracteres). Se guardaba en un texto compartido sin candado. Al arrancar, varios hilos la pedían a la vez y cada uno le añadía sus 64 caracteres. Salía una huella de 128 o más, y la web la rechaza.

**Arreglo** (`SmashSoda/phoenix/link/PhoenixLink.cpp`, `huellaExe`). Se calcula una sola vez con un estático local de C++11, que es seguro entre hilos. Si no salen 64 caracteres, se manda vacía, porque la web también acepta eso.

**Cómo se comprobó (21-oct).**
- Petición a `/functions/v1/phoenix/v1/emparejar` con la huella del exe nuevo y un código falso. La web responde `404 CODIGO_NO_ENCONTRADO` («Código incorrecto»), que es lo normal: ya no rechaza la cabecera.
- La misma petición con la huella duplicada (128 caracteres) da `422 CAMPO_INVALIDO`. Esto confirma la causa.
- Registros de Supabase: los rechazos `CAMPO_INVALIDO` paran el 20-oct a las 18:14 UTC. El último intento real de Fralex fue a las 13:14 Lima, y el exe arreglado se compiló a las 13:21 Lima. Todavía no se había probado con la versión buena.

**Pasos para vincular.**
1. Cierra Link del todo y ábrelo con el acceso directo «PhoenixLink» del menú Inicio. Apunta a `_phoenix-link\x64\Release\PhoenixLink.exe`.
2. Comprueba que pone la versión 0.3.45 (abajo a la izquierda).
3. En la web de la liga: *Mi perfil › vincular PC*. Saca un código nuevo, porque caducan.
4. En Link: SYNC › MI JUEGO › «Tu cuenta de la liga». Pega el código y pulsa CONECTAR.
5. Debe salir «PC vinculada a <tu nombre>». La cuenta se guarda en `%APPDATA%\Trybuchet\Smash Soda\phoenix-token.dat` (cifrada con DPAPI).
6. Si sale otro mensaje, apúntalo tal cual: el texto viene de la web y dice la causa.

## 3. SYNC › TRASPASOS más sencillo (0.3.45)

**Pedido de Fralex:** «Confunde más las tantas opciones y ventanas que tiene el sync».

**Antes.** Había una tarjeta entera de «Cambios oficiales», dos interruptores dentro de ENVIAR, uno dentro de RECIBIR y otro «Más opciones» abajo. En total, 24 botones a la vista.

**Ahora.**
1. Arriba, «PASAR FICHAJES ENTRE AMIGOS» con una línea: «Los cambios oficiales de Phoenix llegan solos, sin código».
2. **ENVIAR MIS FICHAJES:** tu código en grande, COPIAR CÓDIGO y CAMBIAR CÓDIGO, el fichaje detectado con PUBLICAR o DESCARTAR, y «Mi archivo de guardado» (antes «Más opciones») para pegar la ruta si Link no la encuentra.
3. **RECIBIR FICHAJES DE UN AMIGO:** pega su código y pulsa RECIBIR. Debajo, el último fichaje aplicado y «Fichajes recibidos».
4. Abajo, «¿Algo no funciona?» y **«Ajustes de traspasos»** con todos los interruptores juntos:
   - recibir los cambios oficiales de Phoenix;
   - aprobar cada fichaje antes de enviarlo;
   - mantener mis fichajes en mi juego;
   - aprobar cada fichaje antes de ponerlo en mi juego;
   - modo prueba, carpeta compartida y APAGAR TRASPASOS.

Lo normal es no tocar los ajustes.

**Solo cambia la interfaz** (`SmashSoda/phoenix/web/ui/js/vistas/traspasos.js`). Las acciones `traspasos.estado`, `traspasos.config` y `traspasos.accion` son las mismas.

**Cómo se probó.** En el simulador (`python -m http.server 8791 --directory SmashSoda/phoenix/web/ui`, abrir `#sync` y la pestaña TRASPASOS) no hay errores de consola y los 7 ajustes están en «Ajustes de traspasos». Compilado con `cmake --build C:\dev\smash-soda-fork\_phoenix-link\build --config Release --parallel` sin errores; la copia de `ui\` junto al exe quedó al día.

## 4. Cómo replicarlo desde cero

1. `git fetch origin plan-ux` en `C:\dev\smash-soda-fork\_phoenix-link` (o en un clon con la rama `plan-ux`).
2. Compila con cmake, como en el punto 3. **Nunca** con `COMPILAR_PHOENIX.bat`: hace `git checkout --force` y borra lo que no tenga commit.
3. Con Link cerrado, para que la copia de `ui\` no falle.
4. Para probar solo la interfaz, usa el simulador del punto 3.

## 5. Pendiente

- Que Fralex vincule la cuenta con el exe 0.3.45 (punto 2).
- Seguir con el «modo sencillo» (Jugar, Ser anfitrión, Mi cuenta): la pestaña AVANZADO de Sync y el resto de menús.
- Los capítulos del libro personal de mandos 9+ (`cap_60_j/k/l`, `cap_62`, `00_BITACORA_VIVA`) no se copiaron al repo público. Falta que Fralex diga si quiere publicarlos.
