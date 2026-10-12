# Traspasos EN VIVO entre PCs con Phoenix

Estado: **0.2.49 (beta, apagado por defecto)**. Pedido de Fralex: «que yo haga un movimiento en Editar de PES (base normal; Liga Máster más adelante) y que automáticamente en el PC de mi amigo se haga el traspaso también, en tiempo real, en un momento seguro».

## 0. En una frase
Fralex hace un traspaso en **Editar** y **guarda**; Link (su PC) ve que el option file cambió, saca *qué jugador cambió de equipo* y lo publica como un **lote** diminuto; el Link del amigo lo recoge, lo aplica sobre **su propia base** (`Phoenix-DB\PlayerAssignment.bin`, con el sistema de Fichajes) y avisa a `phoenix.lua`, que recarga **solo en un momento seguro** según el modo del amigo. Solo viajan cambios «jugador → equipo», **nunca bases**.

## 1. Qué está probado y qué no (honesto)
| | Estado | Evidencia |
|---|---|---|
| Leer el option file real y sacar quién está en cada plantilla | ✅ probado con **copias de los option files reales de Fralex** | 749 equipos, 19.364 jugadores; comparando sus respaldos detecta exactamente los traspasos que Sync hizo (p. ej. 162114 Barça→Arsenal) y 0 cuando no hay cambios |
| Comparar dos fotos → traspasos (ignora altas/bajas/convocatorias/dorsales) | ✅ | pruebas `traspasos_test` (93/93) |
| Lote, huella, idempotencia, transporte en carpeta, transporte web (hub **falso**) | ✅ lógica | 93/93 |
| Aplicar en el PC del amigo (por ids, por nombres, otro parche, PES abierto/cerrado, deshacer) | ✅ en **juegos falsos** y con **copias de las bases reales** (modos 26, 27 y raíz) | Yamal Barça→Arsenal: publicado desde un option file real, nombres reales «Lamine Yamal / FC Barcelona / Arsenal FC», aplicado en los 3 sitios del amigo y deshecho |
| **PES guarda el option file al guardar/salir de Editar** | ❌ **sin probar** | Sospecha fuerte: `EDIT00000000` de Documents no cambia solo (la última vez fue Sync, 11-oct 16:53, y el juego se usó después). Falta que Fralex haga un traspaso real y guarde |
| El juego del amigo **recarga** el `PlayerAssignment.bin` nuevo | 🟡 ya probado antes para fichajes (Phoenix-DB + recarga) pero **no con este flujo** | ver `PLAN-PAQUETE-LUAS-PHOENIX.md` («Lo que ya lograste») |
| La web como centro | ❌ no existe aún | falta pegar `docs/PROMPT-WEB-traspasos-vivo.md` en el chat WEB |

## 2. Fase 1 · CAPTURA (anfitrión, solo lectura)
**Elegida: vigilar el option file** (`Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<id>\save\EDIT00000000`).
- Link lo lee cada ~3 s **solo si cambió** (tamaño + fecha) y **solo cuando lleva 2 pasos sin cambiar** (PES ya terminó de escribir). Lo descifra (libpesXcrypter, dominio público; copiada a `SmashSoda\phoenix\terceros\pesxcrypter`) y saca, por equipo, los 40 jugadores de la plantilla (posiciones verificadas por Phoenix Sync).
- **Traspaso** = un jugador que **sale de un equipo y entra en otro** respecto a la foto anterior. Todo lo demás se cuenta como «ignorado» (altas/convocatorias, bajas, varios a la vez). Dorsales, tácticas, nombres, Liga Máster (`ML…`): no se miran.
- La primera vez solo guarda la **foto de partida** (no publica nada). Si Fralex cambia de modo de ConmeGOL y el option file se reemplaza entero (más de 60 traspasos a la vez), **no se publica solo**: queda «pendiente masivo» para descartarlo.
- «Al toque» es **al guardar / salir de Editar**: PES no escribe el archivo hasta entonces, así que antes no hay nada que ver (límite de PES, no de Link).
- **Seguridad:** un archivo cortado o a medio guardar **cerraba Link** (el descifrador original se fía de los tamaños de la cabecera). Se añadió `pesxvalidar.c`: comprueba la cabecera contra el tamaño real antes de descifrar. Probado con basura y con el archivo cortado a la mitad: no se cierra, no se publica nada y reintenta.
- **Alternativas descartadas:** leer la tabla de asignación de la **memoria** (no hay dirección verificada; es lo que cerró PES el 9-oct; y cambiaría al instante pero sin guardar, lo que daría traspasos «fantasma» que el usuario aún puede deshacer en Editar). Vigilar `PES21_Hook\EDIT00000000`: es el option file que **trae** el parche, no el que PES guarda.

## 3. Fase 2 · TRANSPORTE
**Recomendado: la web (Supabase) como centro.** Es el único sitio que funciona aunque el amigo tenga Link apagado (se pone al día al abrir) y que sabe quién es quién (token de Link, roles). Contrato mínimo en `docs/PROMPT-WEB-traspasos-vivo.md` (2 endpoints + 1 tabla, idempotente por `id`).
- **Hoy (prueba, sin web):** una **carpeta compartida** (Google Drive, OneDrive, red local) que ven los dos Link. Cada lote es `00000001-<id>.json`. En una sola PC se prueba con dos carpetas.
- Descartado: Link↔Link directo por la sala/Parsec (solo funciona con los dos conectados a la vez y hay que abrir un canal nuevo).
- **Lote** (`formato 1`): `{formato, id (24 hex), huella (sha256 de los cambios), creado (UTC), autor, anfitrion, parche, base_huella, seq (lo pone el transporte), cambios:[{jugador_id, jugador, desde_id, desde, hasta_id, hasta}]}`. El mismo contenido enviado dos veces = mismo `id`: no se duplica. Un lote alterado (huella distinta), con ids ≤ 0 o con origen = destino **se rechaza**. Los nombres salen de la **base real del anfitrión** (`leerNombresDeBase`), no del option file.
- El código del transporte web ya está (`TransporteWeb`, probado contra un hub falso que cumple el contrato); falta **enchufarlo** a la petición autenticada de Link cuando la web tenga los endpoints.

## 4. Fase 3 · APLICACIÓN (amigo)
1. Trae lotes con `seq` mayor al último visto (desordenados o repetidos no importan: se ordenan por `seq` y se descartan los ya vistos). Los lotes **propios** no se aplican.
2. **Cómo se aplica en su PC:** si su base es la misma que la del anfitrión (huella igual) → por ids; si es otro parche → **por nombres** (jugador y equipo, coincidencia exacta; si hay duda se salta con motivo); si es otro parche **y el lote no trae nombres** → se salta, **no se adivinan ids**.
3. Construye la lista efectiva = su `fichajes.json` manual + todos los lotes no deshechos (varios traspasos del mismo jugador se colapsan: primer origen → último destino) y se la pasa a **Fichajes** (`aplicarAlJuego`: todo o nada, con copia). Si queda vacía, quita lo generado.
4. **Con PES abierto** solo se reemplaza el `PlayerAssignment.bin` de Phoenix-DB (escritura atómica). **No** se toca `sider.ini` ni se aparta ningún `Player.bin`: si falta la línea Phoenix-DB o hay un `Player.bin` ajeno, **espera a que cierre PES** (el lote queda «pendiente» con el motivo). Con PES cerrado, queda para el próximo arranque.
5. Avisa a `phoenix.lua` escribiendo `content\phoenix\recargar.txt` (cambia en cada lote). Lo que pasa después lo decide **su modo de recarga**: *AUTO-SIEMPRE / AUTO-FICHAJES* → recarga sola al volver al menú y entrar a un modo (nunca en un partido ni en Editar, momento seguro de `phoenix.lua`); *ACTIVAR* → queda listo hasta que pulse «Datos Actual. en vivo → Activar».
6. **Por jugador** se guarda el resultado: `aplicado | ya_estaba | saltado (motivo) | reemplazado por uno posterior`; por lote: `aplicado | parcial | saltado | deshecho`. **DESHACER el último lote** regenera Phoenix-DB sin él (o lo quita si no queda nada) y vuelve a avisar.
- **Link del amigo apagado:** al abrirlo trae todos los lotes que se perdió, en orden.
- **Conflicto (el amigo también edita):** con «datos en vivo» activado manda la **base** (Phoenix-DB) sobre su option file para los jugadores del lote; el resto de su Editar no se toca. Si el amigo también **publica**, un traspaso que él recibió **no se vuelve a publicar** (anti-bucle: se compara con lo recibido).
- Límite honesto: Link **no sabe** si el juego ya recargó (phoenix.lua no se lo dice a Link); por eso en modo ACTIVAR el lote figura «aplicado» aunque el juego aún no lo haya cargado. Sí lo sabe el amigo en el overlay.

## 5. Interfaz (SYNC › TRASPASOS EN VIVO)
Interruptor del **anfitrión** (publicar; pedir confirmación antes, por defecto sí; ver los detectados y **PUBLICAR / DESCARTAR**), interruptor del **amigo** (recibir), estado, historial de lotes con cada jugador y su resultado, **DESHACER el último lote**, carpeta de envío, option file a mano y «olvidar la foto de partida». Estado en `%APPDATA%\Trybuchet\Smash Soda\phoenix-traspasos\`. Acciones: `traspasos.estado|config|accion` (ver `SmashSoda\phoenix\web\PROTOCOLO.md`). Todo apagado por defecto.

## 6. Pruebas
`pruebas-link\correr_traspasos.bat ["…\save\EDIT00000000" ["D:\…\Conmegol Patch"]]` (MSVC; el .exe se llama `traspasos_test`). **93/93** con una copia del option file real y las copias de las bases reales; **80/80** con solo el option file; **23/23** sin nada (lógica pura). Cubre: lote/huella/validación, transporte carpeta y web (hub falso), colapso de lotes, anfitrión (referencia, espera a que termine de guardar, publicar, no republicar, otros cambios ignorados, masivo, confirmar/descartar, sobrevive a cerrar Link, archivo cortado/basura/ausente), amigo (aplicar, idempotente, repetido, deshacer, PES abierto sin/con raíz, jugador inexistente, otro parche por nombres, otro parche sin nombres, se pone al día, anti-bucle) y la cadena con bases reales.

## 7. Prueba real con Fralex (cuando quiera; con copia antes)
1. Copia de seguridad de `…\save\EDIT00000000` (y de `SiderAddons\livecpk\Phoenix-DB` si existe).
2. Abre Link → SYNC › TRASPASOS EN VIVO. Elige una **carpeta de prueba** (p. ej. `D:\PruebaTraspasos`). Enciende «Publicar» (con confirmación) **y** «Recibir» en esa misma PC solo para ver el flujo, o usa la carpeta compartida con tu amigo.
3. En PES: Editar → haz **un** traspaso → **guarda** (Guardar/Cargar › Guardar) y sal. Mira si Link muestra «Detecté traspasos» (si no: dime la hora y miramos si PES guardó el archivo).
4. Pulsa PUBLICAR. En el PC receptor: debe aparecer el lote «aplicado». Con PES abierto en AUTO: vuelve al menú, entra a un modo y mira el equipo.
5. Prueba **DESHACER el último lote**.

## 8. Archivos
`link\Captura.h/.cpp` (fase 1), `link\Traspasos.h/.cpp` (lote, transportes, motor), `link\ServicioTraspasos.h/.cpp` (hilo + ajustes), `web\AccionesTraspasosWeb.cpp`, `web\ui\js\vistas\traspasos.js`, `terceros\pesxcrypter\` (+ `pesxvalidar.c`), `pruebas-link\traspasos_test.cpp` y `correr_traspasos.bat`. CMake: lenguaje C activado para los 4 `.c` (fuera de unity y sin `pch.h`). No se tocó `PhoenixSync/` ni `Fichajes.cpp/.h` (solo se usa su API pública: `analizar`, `aplicarAlJuego`, `quitarDelJuego`, `leerNombresDeBase`).

## 9. Pendiente
- Prueba real en PES (§7) — **bloquea dar esto por bueno**.
- Web: pegar `PROMPT-WEB-traspasos-vivo.md` y enchufar `TransporteWeb`.
- Liga Máster (más adelante: otro archivo `ML…` y otras reglas).
- Que `phoenix.lua` diga a Link «ya recargué» (para marcar «cargado en el juego»).
