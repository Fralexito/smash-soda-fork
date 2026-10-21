# Mandos ocultos hasta que alguien los ocupe + color de cada mando (PES 2021)

Frente LINK · 12 oct 2026 · **INVESTIGACIÓN. No se cambió código de Link ni el juego de Fralex.**
Etiquetas: **[EVIDENCIA]** = visto en archivos/código/pruebas previas o fuente citada. **[SUPOSICIÓN]** = razonado, sin probar.

## Resumen corto

| Pregunta | Respuesta |
|---|---|
| 1. ¿Mandos listos por dentro pero invisibles para PES hasta que alguien los ocupe? | **Parcialmente.** No hay forma segura de mostrarlos/ocultarlos *con PES abierto*. Sí hay una vía ya existente y segura: dejar vacías las casillas de `settings.dat` de los puestos que no se van a usar *antes de abrir PES*. |
| 2. ¿Color de cada mando (mando 1 rojo, mando 2 azul) sin importar el puesto? | **No hay vía segura.** Lo que sí se logra es elegir *qué número de jugador* es cada persona (mando N de Link = jugador N). El color en sí lo decide el motor de PES. |

Recomendación: **no implementar nada**. Primero dos pruebas de observación con Fralex (§4). Regla de Fralex: «si algo se rompe, mejor no lo hagas».

## 1. Pregunta 1 — mandos «listos pero ocultos»

### 1a. Cómo muestra PES los mandos que nadie toca
- **[EVIDENCIA]** PES solo hace caso a los pads que están en alguna de las 8 casillas de `settings.dat`; el resto los ignora (`docs/ORDEN-MANDOS-PES.md` §2). Lo lee **solo al arrancar** (§6).
- **[EVIDENCIA]** Link ya deja la casilla vacía cuando el puesto está desenchufado (`core/OrdenPes.cpp:163`).
- **No encontré documentación pública** sobre si PES 2021 muestra todos los pads conectados o solo los que pulsan, ni sobre el límite, ni sobre hot-plug (búsqueda en Steam, pesmodding, foros: solo hilos genéricos de «qué mando funciona»). Hilo más cercano: <https://steamcommunity.com/app/1259970/discussions/0/2967267880874790922> (sin datos de multi-mando).
- **[SUPOSICIÓN]** El estilo clásico de la serie: cada mando se asigna a un lado pulsando un botón. No confirmado. **Hace falta observarlo (prueba A, §4).**

### 1b. Opciones evaluadas

| Opción | Veredicto | Motivo |
|---|---|---|
| **Vaciar casillas de `settings.dat` de los puestos no usados** (PES cerrado) | **Viable, ya existe.** Pad enchufado, GUID estable, PES lo ignora. | Granularidad por *sesión*, no en vivo: cambiar K exige reiniciar PES. Los puestos que sí se usan conservan N = jugador N. Riesgo bajo (Link ya hace copia de seguridad y escribe atómico). |
| HidHide para ocultar/mostrar pads virtuales | **No merece la pena.** | Driver extra + admin. **[SUPOSICIÓN]** PES enumera al arrancar; mostrar un pad con PES abierto probablemente no lo recoja. Mal encaje con pads ViGEm propios. |
| Desactivar/activar el dispositivo (cfgmgr32) | **No recomendado.** | Requiere admin; si Link se cierra mal, quedan pads deshabilitados. **[SUPOSICIÓN]** deshabilitar no cambia el GUID, pero reenumerar con PES abierto sigue sin garantía. Sin evidencia a favor. |
| ViGEm plug/unplug con el mismo serial | **No.** | **[EVIDENCIA]** (10 oct) al reconectar Windows reparte GUID de nuevo y rompió el orden; había 100+ entradas fantasma. Es justo lo que Link evita. |
| Desde Sider/Lua | **No.** | **[EVIDENCIA]** Sider solo da `gamepad_input` con overlay abierto y de un solo mando; no controla qué dispositivos ve DirectInput (`INVESTIGACION-CURSOR-MANDO.md` §2). |
| Casillas reservadas para pads que llegan después | Ya es el diseño actual (casilla N = puesto N, 8 siempre). | Lo que falta es solo que PES los *muestre* solo al pulsar: depende de la prueba A. |

### 1c. Riesgos
Admin (HidHide, cfgmgr32), estabilidad (pads deshabilitados al cerrarse mal Link), GUID cambiantes (cualquier plug/unplug). La vía de casillas vacías no añade ninguno.

## 2. Pregunta 2 — color por mando

### 2a. De dónde sale el color
- **No se encontró ninguna fuente pública** que diga si es por número de mando, por lado (local/visita) o por orden de pulsación. **[SUPOSICIÓN]** lo decide el motor según el lado/asignación, no una tabla modificable por texturas.
- **[EVIDENCIA]** `UIColors.lua` (Zlac, 2024) solo cambia **colores de texto de la interfaz**: 80 índices en memoria (`map_exe.txt`) y offsets de colores en archivos `.bin` de menú (`ui_bin_colors.ini`). El único índice relacionado con mandos es el 80 «Text Outline Colour (Controller Button Info)», un contorno de texto de ayuda de botones. **No hay nada de cursores 1P/2P.** (Revisado solo en lectura en `D:\Frank\Games_\Conmegol Patch\SiderAddons`.)
- **[EVIDENCIA, confianza media]** Hay mods públicos de Sider que **sustituyen texturas** de cursores del campo y barra de potencia (<https://www.pesmodding.com/2022/12/new-pes-2021-ingame-cursors-power-gauge.html>, pack retro con 3 cursores de colores distintos <https://www.pesmodding.com/2023/01/new-pes-2021-ingame-cursors-retro-pack.html>) y otro que cambia el icono de la selección de equipos (<https://www.pesmodding.com/2023/08/pes-2021-new-plate-color-mod.html>, `cmnlcon.bin`). Es decir: el **aspecto** del cursor es cambiable; la **regla** de a quién le toca cada uno, no.

### 2b. Qué se consigue y qué no
- **Sí (sin tocar el juego):** elegir qué jugador de PES es cada persona, porque mando N de Link = jugador N (probado el 10 oct). Si PES colorea por número de jugador, entonces «mando 1 rojo» se logra poniendo a esa persona en el puesto 1.
- **No (sin modificar la interfaz):** que un color siga a la persona independientemente de su puesto/lado. Exigiría reemplazar texturas por mando (si el color es por número) o parchear lógica de UI en memoria (**no seguro**: la lectura directa de memoria ya cerró PES el 9 oct).

## 3. Recomendación
1. **No implementar.** No hay vía probada y segura que cumpla la regla de Fralex.
2. Si la prueba A muestra que PES enseña todos los pads y eso molesta: usar la vía **segura ya existente**, documentándola como opción «Puestos que se usan hoy» (K casillas llenas, el resto vacías, antes de abrir PES). Es un cambio pequeño en `OrdenPes`, pero **hay que decidirlo con Fralex** y requiere reiniciar PES para cambiar K.
3. Color: si la prueba B muestra que el color depende del número de jugador, bastará con **asignar puestos**; si depende del lado, no hay nada que hacer salvo mods de textura propios (otra investigación, no urgente).

## 4. Pruebas pedidas a Fralex (solo observar, sin cambiar nada)
- **Prueba A (mandos visibles).** Con la sala abierta, los 8 puestos enchufados y el orden de mandos encendido, entra a PES con 2 personas que pulsan. En la pantalla de selección de equipos y en el partido: ¿se ven los 8 pads, solo los 2 que pulsaron, o solo los que están en el lado de cada uno? Una captura vale.
- **Prueba B (colores).** Con 2 o 3 personas en puestos conocidos, apunta el **color del cursor/indicador** de cada una (selección de equipos y en el campo). Luego cambia a esas personas de puesto (sin cambiar el lado) y mira si el color se queda o cambia; después pásalas al otro lado y repite.

## 5. Fuentes y límites
- Búsqueda web amplia: poca información útil para PES 2021 multi-mando; todo lo de comportamiento queda como suposición hasta las pruebas.
- Docs previos: `ORDEN-MANDOS-PES.md`, `INVESTIGACION-CURSOR-MANDO.md`, `DETECCION-LADOS.md`.

## 6. Ronda 2 de búsqueda (12 oct): ¿existe alguna manera?

- **[EVIDENCIA]** Existe una herramienta pública con la misma idea que Link: <https://github.com/KOUFU-DIY/PES2021-Setting-Pro> (8 mandos Xbox virtuales persistentes con ViGEmBus + escribe `settings.dat`, copia `.bak`, juego y Settings cerrados). Ofrece «desactivar un número concreto» y reordenar, pero **no** oculta/muestra pads en vivo ni habla de colores. Confirma que el enfoque de Link (pads fijos + casillas) es el camino que sigue la comunidad.
- **[EVIDENCIA]** Parsec crea el mando del invitado cuando este pulsa botones (<https://support.parsec.app/hc/en-us/articles/32381826078868-Controller-Isn-t-Working-in-the-Game>): es el comportamiento «aparece al ocuparse» de Parsec puro, pero es justo lo que reordena los GUID; Link lo evita a propósito.
- **[EVIDENCIA indirecta, baja]** Reportes de PES 2018/2020 dicen que hay que reenchufar el pad dentro del juego tras configurarlo (<https://steamcommunity.com/app/592580/discussions/0/2765630416809431049>): sugiere que PES no re-enumera bien en caliente. Nada específico de PES 2021.
- **Color:** no apareció ningún offset ni textura documentada del cursor 1P/2P. Solo guías de color de texto del `.exe` y el pack de cursores de moukos13 (3 cursores de colores numerados 1-3 «para varios jugadores»; la página no dice si el número es por mando o por lado). Ver <https://www.pesmodding.com/2023/01/new-pes-2021-ingame-cursors-retro-pack.html>.
- **Conclusión de la ronda 2:** no existe una vía documentada mejor que la de §1b. La única pista abierta para color es que el pack de cursores usa texturas numeradas: si la prueba B muestra que el color sigue al número de jugador, un pack de texturas propio daría «mando 1 rojo, mando 2 azul» sin tocar memoria.
