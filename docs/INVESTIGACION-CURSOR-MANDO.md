# Investigación · ¿qué mando controla a qué futbolista? (goles por persona para el ranking)

Frente LINK · 11 oct 2026 · estado: **método listo y simulado; falta la prueba en la PC de Fralex**.

## 1. El problema

- En 1 contra 1 el goleador se sabe por el lado (`phoenix_estadio.lua` ya da cada gol con lado y minuto).
- En 2 contra 2 o 3 contra 3 hay varios humanos por lado. Para saber **quién** marcó hay que saber qué mando
  (jugador 1-8 de PES = casilla de `settings.dat` = puesto de Phoenix Link, comprobado el 11 oct) controlaba
  al futbolista que marcó. Y, si se puede, quién dio la asistencia.

## 2. Qué se sabe (11 oct)

| Fuente | Qué da | ¿Sirve? |
|---|---|---|
| Sider 7.3.3/7.4 `match.stats()` | marcador, penales, periodo, reloj | Solo el lado del gol. **No da goleador ni mando.** |
| Eventos de Sider (doc `scripting.html` de sider7) | `set_teams`, `set_match_settings`, `display_frame`, `key_down`, `gamepad_input`… | No hay evento de gol ni de «jugador controlado». `gamepad_input` solo llega **con el overlay abierto** y **de un solo mando** («only one will generate these input events»). |
| Archivos que carga el juego (`livecpk_data_ready`) | `goal\cut_data\goal_celebrate_NNNN…` al celebrar (lo usa el ejemplo `cr7_audio.lua`) | Dice **qué celebración** se carga, no qué jugador ni qué mando. Como mucho, una pista de cuándo se celebra. |
| Tabla de Cheat Engine de xAranaktu (MIT, archivada en 2023) | presupuestos de LM, ficha del jugador al pasar el cursor del menú, tiempo de partido, estamina | **No** trae cursor del partido, mandos, goleadores ni eventos. |
| Foros (evo-web, pesmodding, CE) | — | No se encontró ninguna dirección pública del «controlled player» ni del registro de goles de PES 2021. Hay que encontrarla nosotros. |
| Base de SYNC (`05-PUERTA-EN-VIVO.md`, `RIESGOS-SIDER.md`) | Técnica de búsqueda segura (`VirtualQuery` + `ReadProcessMemory`) | **Clave.** Leer la memoria del juego directamente ya cerró PES (0xC0000005 el 9 oct, por ReShade/DLSS). Siempre se copia con `ReadProcessMemory`. |
| Base de SYNC (guías 16 y 19) | Goleadores y asistencias de temporada en el **guardado de Liga Máster** | Solo LM y por jugador, no por mando. No vale para amistosos por Parsec. |

**Conclusión:** nadie lo tiene hecho. Es viable con ingeniería inversa **solo de lectura** (no puede desincronizar
ni cambiar el partido), pero hay que localizar el dato con pruebas en el juego.

## 3. Hipótesis

1. **Cursor:** cada futbolista en el campo tiene una estructura en memoria creada al empezar el partido. Para dibujar
   el indicador «1P/2P» sobre su cabeza, el juego necesita saber qué mando lo controla: lo más probable es un byte
   (casilla 0-7, o 0xFF = nadie) dentro de esa estructura, o una tabla por mando que apunta a ella.
2. **Gol:** el juego muestra el nombre del goleador en el marcador y en las estadísticas del partido, así que guarda
   un registro de cada gol con el ID del jugador (y quizá el del asistente y el minuto).
3. En PES el mando que tiene el balón controla al que lo lleva, así que en el momento del gol el mando que
   controlaba al goleador es el que marcó: «gol de la casilla N» = casilla cuyo cursor estaba en el goleador
   cuando subió el marcador (o justo antes del tiro).

## 4. Método (prueba `pruebas-link/sider/phoenix_cursor_prueba.lua`)

Se usa un futbolista conocido: **Lamine Yamal, ID 162114** (Barça), el mismo que usó SYNC. El ID se puede cambiar
con `content\phoenix\cursor\objetivo.txt`.

1. **Búsqueda «menu»** (menú principal): todas las direcciones donde está el ID (copias de la base, menús).
2. **Búsqueda «partido»** (ya jugando, en pausa): lo que aparece **solo** en el partido es el futbolista en el campo.
   Se quedan hasta 96 direcciones «candidatas».
3. **Fotos** de 4 KB alrededor de cada candidata en momentos marcados con la tecla J:
   A = Lamine con el teclado, N = sin nadie, B = con el mando físico (dos veces cada una).
   Además, cada 0,25 s se anotan los bytes que cambian poco (`cambios.txt`) y en cada gol se hacen dos fotos solas.
4. **Búsqueda «gol»** tras marcar con Lamine: las direcciones nuevas con su ID son el candidato a registro del gol
   (`tras_gol.txt`; puede traer también minuto y asistente).
5. En el PC de desarrollo: `py pruebas-link/sider/analizar_cursor.py <carpeta>`. Un byte es **candidato fuerte** si vale
   igual en A1 y A2, igual en B1 y B2, igual en N1 y N2, y los tres valores son distintos.

**Seguridad:** solo lectura; nunca lee la memoria directamente (copia con `ReadProcessMemory`, que falla sin cerrar
el juego); sin `VirtualProtect` ni llamadas al juego; errores con «bandera» (5 seguidos → se apaga);
teclas J e I, que ningún módulo de ConmeGOL usa; escribe solo en `content\phoenix\cursor\`.
Mientras busca copia ~4 MB por cuadro: puede haber tirones unos segundos (por eso se hace en pausa).

**Simulación** (`pruebas-link/sider/simular_cursor.py`, LuaJIT de lupa + memoria real del proceso, imitando Sider):
recorre los 10 pasos, encuentra el byte plantado (+0x40: teclado 0x00 / mando 0x01 / nadie 0xFF), detecta el gol,
hace las fotos solas y encuentra el registro nuevo del gol. Las copias que la propia prueba deja en memoria
salen marcadas como «posible copia propia».

## 5. Qué sigue según el resultado

| Resultado | Siguiente paso |
|---|---|
| Hay candidato fuerte (byte con la casilla) | v0.2: leer ese byte en los 22 futbolistas (las estructuras deberían estar seguidas o en una tabla). Confirmar con 3 mandos. |
| Solo «con cursor sí/no» | Buscar al lado una tabla por mando: v0.2 busca punteros (u64) a la estructura del futbolista y vigila cuáles cambian al cambiar de jugador. |
| Nada en las candidatas | El cursor vive en otra estructura: v0.2 busca punteros a las candidatas, o sesión guiada con Cheat Engine («qué accede a esta dirección»), solo con permiso de Fralex. |
| Aparece registro del gol | Leer de ahí goleador (y asistente si está). Para encontrarlo en cada partido sin escanear toda la memoria, buscar una firma estable (AOB) o la estructura padre. |

**Integración final (cuando todo esté probado):** `phoenix_estadio.lua` añadiría a cada gol de `estado.json` /
`resultado.json` la casilla (`"c":N`, y `"a":N` del asistente). Link traduce casilla → persona (ya conoce los puestos)
y lo sube con el resultado. Cambia el contrato con la web: se pedirá por prompt al chat WEB. Mientras tanto,
alternativa sin memoria: que el árbitro o los capitanes asignen los goleadores en la web.

## 6. Guía para Fralex (solo cuando dé permiso)

**Antes:** PES cerrado. Partido offline contra la CPU, nunca con amigos.

1. **Copia de seguridad:** en `D:\Frank\Games_\Conmegol Patch\SiderAddons\` copia `sider.ini` como
   `sider.ini.antes-cursor`. Haz lo mismo en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\` (la que copia el switcher).
2. **Instalar:** copia `phoenix_cursor_prueba.lua` a las dos carpetas `SiderAddons\modules\` y, en los dos `sider.ini`,
   añade al final de las líneas `lua.module`: `lua.module = "phoenix_cursor_prueba.lua"`
3. **Jugar:** abre PES. En el menú principal: Espacio (overlay) → tecla 1 hasta «PRUEBA CURSOR» → J.
   Espera a que diga «lista». Cierra el overlay.
4. Amistoso **Barça contra la CPU**, con el **teclado y el mando físico los dos en el Barça**. Que juegue Lamine.
5. Ya jugando: pausa → overlay → J (búsqueda «partido»). Espera a «lista».
6. Sigue lo que dice el overlay (paso 3 a 8): pon la jugada que pide, pausa, overlay, J. Si te equivocas, I vuelve un paso.
7. Mete un gol **con Lamine**; cuando vuelva el saque, pausa → overlay → J (búsqueda «gol»).
8. Sal y cierra PES. Pásale a Claude la carpeta `SiderAddons\content\phoenix\cursor` y el `sider.log`.

**Volver atrás:** con PES cerrado, restaura los dos `sider.ini.antes-cursor` (o quita la línea) y borra
`phoenix_cursor_prueba.lua` de las dos carpetas `modules`. La carpeta `content\phoenix\cursor` se puede borrar.

## 7. Resultados

_(pendiente de la prueba con Fralex)_

## Fuentes

- Sider 7: <https://github.com/pes-modding/sider7> (`doc/scripting.html`). Se lee para aprender; no se copia código.
- Tabla de xAranaktu (MIT): <https://github.com/xAranaktu/PES-2021-Cheat-Table>
- Base de SYNC (rama `mercado-fase0`): `05-PUERTA-EN-VIVO.md`, `06-RUTA-DEL-DESCUBRIMIENTO.md`, `PhoenixSync/sider/RIESGOS-SIDER.md`,
  `PhoenixSync/sider/phoenix.lua` (técnica de copia segura) y `investigacion/herramientas-y-formatos.md` §9.
