# Phoenix Sync · Pedidos a Link y a la web (prompts)

Todos los prompts que Sync escribió para que Link o la web hicieran su parte (buzón del juego, modo de recarga, PlayerAssignment, páginas del Mod Menu, oficial y control total, Sync compartido). Cada parte dice si quedó hecha.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 11 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · PROMPT PARA EL CHAT LINK — Phoenix Link entrega datos nuevos y avisa «pulsa Datos Actual. en vivo»](#parte-1-prompt-para-el-chat-link-phoenix-link-entrega-datos-nuevos-y-avisa-pulsa-datos-actual-en-vivo) — origen `PhoenixSync/prompts/PROMPT-LINK-boton-nativo.md` (540fa1a 2026-10-09)
- [Parte 2 · PROMPT PARA EL CHAT LINK — Phoenix Link escribe el «buzón» del juego (avisos web → PES 2021 en vivo)](#parte-2-prompt-para-el-chat-link-phoenix-link-escribe-el-buzon-del-juego-avisos-web-pes-2021-en-vivo) — origen `PhoenixSync/prompts/PROMPT-LINK-buzon-juego.md` (e30cbd9 2026-10-09)
- [Parte 3 · Prompt para el chat LINK — «Automático: siempre» primero y por defecto (phoenix.lua v0.18b)](#parte-3-prompt-para-el-chat-link-automatico-siempre-primero-y-por-defecto-phoenixlua-v018b) — origen `PhoenixSync/prompts/PROMPT-LINK-modo-por-defecto.md` (b8d7097 2026-10-10)
- [Parte 4 · Prompt para el chat LINK — botón «Modo de recarga» (phoenix.lua v0.18)](#parte-4-prompt-para-el-chat-link-boton-modo-de-recarga-phoenixlua-v018) — origen `PhoenixSync/prompts/PROMPT-LINK-modo-recarga.md` (0387464 2026-10-10)
- [Parte 5 · Prompt para el chat LINK — páginas propias en «Sider Mod Menu» (F1) para phoenix_referee y phoenix_campo](#parte-5-prompt-para-el-chat-link-paginas-propias-en-sider-mod-menu-f1-para-phoenixreferee-y-phoenixcampo) — origen `PhoenixSync/prompts/PROMPT-LINK-paginas-modmenu.md` (3305aee 2026-10-18)
- [Parte 6 · Prompt para el chat LINK — aceptar `PlayerAssignment.bin` en las entregas de Sync](#parte-6-prompt-para-el-chat-link-aceptar-playerassignmentbin-en-las-entregas-de-sync) — origen `PhoenixSync/prompts/PROMPT-LINK-playerassignment.md` (fa26396 2026-10-09)
- [Parte 7 · PROMPT PARA EL CHAT SYNC — `phoenix.lua` cuenta el partido a Phoenix Link (estado.json + resultado.json)](#parte-7-prompt-para-el-chat-sync-phoenixlua-cuenta-el-partido-a-phoenix-link-estadojson-resultadojson) — origen `PhoenixSync/prompts/PROMPT-SYNC-estado-partido.md` (8f50132 2026-10-10)
- [Parte 8 · PROMPT PARA EL CHAT WEB — «Buzón del juego» (avisos web → PES 2021 en vivo)](#parte-8-prompt-para-el-chat-web-buzon-del-juego-avisos-web-pes-2021-en-vivo) — origen `PhoenixSync/prompts/PROMPT-WEB-buzon-juego.md` (e30cbd9 2026-10-09)
- [Parte 9 · Prompt para el chat WEB — Grupo de prueba de fichajes con «Activar»](#parte-9-prompt-para-el-chat-web-grupo-de-prueba-de-fichajes-con-activar) — origen `PhoenixSync/prompts/PROMPT-WEB-grupo-prueba-fichajes.md` (f6a5423 2026-10-10)
- [Parte 10 · Prompt para el chat WEB — Option file oficial + control total del admin (fichajes y stats)](#parte-10-prompt-para-el-chat-web-option-file-oficial-control-total-del-admin-fichajes-y-stats) — origen `PhoenixSync/prompts/PROMPT-WEB-oficial-y-control-total.md` (e626317 2026-10-10)
- [Parte 11 · Prompt para el chat WEB — «Sync compartido» (contrato v1.8.0, §26)](#parte-11-prompt-para-el-chat-web-sync-compartido-contrato-v180-26) — origen `PhoenixSync/prompts/PROMPT-WEB-sync-compartido.md` (f2a0296 2026-10-09)

---

## Parte 1 · PROMPT PARA EL CHAT LINK — Phoenix Link entrega datos nuevos y avisa «pulsa Datos Actual. en vivo»

> Origen: `PhoenixSync/prompts/PROMPT-LINK-boton-nativo.md` · último cambio 540fa1a 2026-10-09 · 4694 bytes. Copia íntegra.

Hola. Soy el chat SYNC (cuenta B, rama `mercado-fase0`, carpeta `PhoenixSync/`). Te pido una función nueva en **Phoenix Link**. Sigue tu protocolo:
- rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`;
- compilar en el worktree `_phoenix-link` **sin errores** antes de cada commit;
- una línea por cambio en `REGISTRO-LINK.md`;
- nada a `master` sin permiso de Fralex;
- «si algo se rompe, mejor no lo hagas».

### Qué ya funciona (probado en el juego de Fralex, 2026-10-09)
- **Tu cartero** (avisos web → `SiderAddons\content\phoenix\avisos.txt`) funciona.
- **`phoenix.lua` v0.17** (Sider) **revive el botón nativo** «Partido → Datos Actual. en vivo → Activar». Al pulsarlo:
  - sale el **mensaje nativo de Konami**;
  - el juego **relee y aplica ahí mismo** la base (`Player.bin`, etc.) y el option file (`EDIT00000000`);
  - no hace falta Editar, reiniciar ni volver al menú.
  - Los parches se aplican solos al arrancar el juego, en memoria y con comprobación de bytes.
- **Dónde lee el juego los datos de Phoenix:**
  - **Stats (base):** `<juego>\SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\Player.bin`. Esa raíz va en `sider.ini` **antes** de la del parche. Formato WESYS: cabecera 16 B `ff 10 81 57 'WESYS'` + tamaño comprimido u32 + tamaño real u32, y después zlib.
  - **Plantillas / fichajes:** el option file `%USERPROFILE%\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\<steamid>\save\EDIT00000000` (en el PC de Fralex, la carpeta activa es `239200`).
  - El ConmeGOL usa un **switcher** que copia `ConmeGol Extras\<modo>\SiderAddons` encima del juego. Por eso **Phoenix-DB tiene que existir en las dos rutas**: `<juego>\SiderAddons\livecpk\Phoenix-DB\…` y `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\livecpk\Phoenix-DB\…`.
- Detalle: `PhoenixSync/PRUEBAS.md` y `PhoenixSync/base-conocimiento/07-MOTOR-POR-DENTRO.md` (rama `mercado-fase0`).

### Lo que tienes que construir: el «repartidor de datos»
**Quién fabrica los archivos:** los fabrica **Phoenix Sync** (`PhoenixSyncCore`: `OptionFile::mover` para fichajes; el generador de `Player.bin` lo hago yo en mi frente). **No los fabriques tú.** Tú solo los **colocas** y **avisas**.

1. **Entrada.** Sync deja los archivos listos en `%APPDATA%\Phoenix Mercado\entrega\` (la carpeta se mantiene con el nombre viejo a propósito):
   - `Player.bin` (opcional);
   - `EDIT00000000` (opcional);
   - `entrega.json` = `{ "version": 1, "id": "<uuid>", "creado_en": "<ISO>", "resumen": "texto corto", "archivos": [ {"nombre": "Player.bin", "sha256": "…"}, … ] }`.
2. **Comprobar antes de tocar nada:**
   - el sha256 de cada archivo coincide con `entrega.json`;
   - `Player.bin` empieza por la cabecera WESYS;
   - `EDIT00000000` mide lo mismo que el actual;
   - si algo falla → **no colocar nada** y avisar «⚠ entrega rechazada: …» en `avisos.txt`.
3. **Colocar de forma atómica** (cada archivo: `.tmp` en la misma carpeta → `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`; si falla porque está en uso, reintentar hasta 10 veces cada 200 ms):
   - `Player.bin` → las **dos** rutas Phoenix-DB. **No crees carpetas**: si Phoenix-DB no existe, rechaza con «Phoenix-DB no instalado».
   - `EDIT00000000` → antes, **respaldo** `EDIT00000000.phoenix-<fecha>` en la misma carpeta (guarda solo los 5 últimos), y después colocar.
4. **Avisar** en el buzón (mismo formato que ya usas): `[hh:mm] ⚡ Datos nuevos de Phoenix: <resumen>. Partido → Datos Actual. en vivo → Activar`.
5. **Registrar** la entrega en `%APPDATA%\Phoenix Mercado\entrega\historial.log` y borrar los archivos de entrada (o moverlos a `entregados\<id>\`).
6. **Interfaz:** en la sección **Phoenix Sync** del menú de Link, cambia el «Fichajes — Próximamente» por una tarjeta **«Última entrega»** con:
   - fecha, resumen y estado (colocada / rechazada);
   - un botón **«Deshacer última entrega»**, que restaura el respaldo del option file y el `Player.bin` anterior (guarda una copia `Player.bin.anterior` antes de reemplazar).

### Reglas
- Nunca escribas si `PES2021.exe` no está abierto **y** la carpeta del juego no se encuentra. Con el juego cerrado sí puedes colocar, si conoces la carpeta (guárdala en la config la primera vez que detectes el juego).
- Nunca toques `sider.ini`, los `.lua` ni nada fuera de las rutas de arriba.
- Pruebas: un `Player.bin` de prueba con un cambio visible (Fralex suele usar Lamine Yamal, ID 162114, Velocidad 90/95/99). Comprueba que el aviso aparece en el overlay y que, tras pulsar Activar, cambia la Velocidad.

---

## Parte 2 · PROMPT PARA EL CHAT LINK — Phoenix Link escribe el «buzón» del juego (avisos web → PES 2021 en vivo)

> Origen: `PhoenixSync/prompts/PROMPT-LINK-buzon-juego.md` · último cambio e30cbd9 2026-10-09 · 6300 bytes. Copia íntegra.

Hola. Soy el chat SYNC (antes «MERCADO»; Phoenix Mercado ahora se llama **Phoenix Sync**; cuenta B, rama `mercado-fase0`, carpeta `PhoenixSync/`). Te pido una función nueva en **Phoenix Link**. Sigue tu protocolo:
- rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`;
- compilar en el worktree `_phoenix-link` (`COMPILAR_PHOENIX.bat`), **sin errores** antes de cada commit;
- una línea por cambio en `REGISTRO-LINK.md`;
- nada a `master` sin permiso de Fralex;
- «si algo se rompe, mejor no lo hagas».

### Qué ya funciona (probado en el juego de Fralex, 2026-10-09)
- El módulo de Sider `phoenix.lua` v0.2 está instalado en el parche ConmeGOL (`ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`, que el switcher copia encima del juego con robocopy).
- Cada 2 s lee `<carpeta del juego>\SiderAddons\content\phoenix\avisos.txt` y lo muestra en el overlay de Sider (barra espaciadora, luego tecla 1 hasta «PHOENIX EVOLUTION»).
- Con el juego abierto se reemplazó el archivo y el texto nuevo («hola causa») apareció en 1–2 s. Tildes, ñ, ⚡ y → se ven bien.
- Código y pruebas: rama `mercado-fase0`:
  - `PhoenixSync/sider/phoenix.lua`;
  - `PhoenixSync/sider/VINCULO-TIEMPO-REAL.md`;
  - `PhoenixSync/PRUEBAS.md`.

### Lo que tienes que construir: el «cartero»
Un paso nuevo en el hilo de `PhoenixLink` (`SmashSoda/phoenix/link/PhoenixLink.cpp`), al estilo de `pasoAmigos`:

1. **Solo cuando el juego está abierto.** Detecta `PES2021.exe` con `CreateToolhelp32Snapshot` cada ~5 s. Con `QueryFullProcessImageNameW` (`PROCESS_QUERY_LIMITED_INFORMATION`) saca la **carpeta del juego**. Esto vale para cualquier parche (ConmeGOL, SP, etc.), sin que Fralex configure nada.
   - Si no hay juego abierto → no se pregunta a la web (ahorra cuota del plan FREE).
   - Si el juego está abierto pero no existe `<juego>\SiderAddons\content\phoenix\` → no escribir nada. Solo marca el estado «Puente Sider no instalado». **Nunca crees carpetas ni toques `sider.ini`.**
2. **Consulta a la web:** `GET /v1/juego/buzon` (contrato `docs/contrato-v1.md` §25, versión 1.7.0, que está haciendo el chat WEB).
   - Mismos encabezados y token `phx_…` de siempre.
   - Usa ETag igual que en `pasoAmigos` (`If-None-Match`; 304 = sin cambios).
   - Cada `sondeo_seg` de la respuesta (por defecto 15 s, nunca menos de 5).
   - Política de reintentos del contrato §0.
   - Si `pausado: true` o `interruptores` lo apagan → parar hasta el próximo `config`.
3. **Escribe el buzón** cuando la lista cambie. Formato del archivo (UTF-8 **sin BOM**, `\n`):
   - **Contenido:** máx. 14 líneas y 1500 bytes. Avisos del más nuevo al más viejo, cada uno como `[hh:mm] texto` (hora local de Lima, de `creado_en`). Entre avisos va una línea vacía. Si no caben, se corta sin partir una letra UTF-8. `phoenix.lua` igual recorta a 14 líneas y 110 bytes por línea.
   - **Sin avisos:** el texto `Sin avisos nuevos de Phoenix Evolution.`
   - **Escritura atómica (obligatorio):**
     - escribe primero `avisos.tmp` en la misma carpeta;
     - luego `ReplaceFileW(avisos.txt, avisos.tmp, …)`, o `MoveFileExW(tmp, txt, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` si `avisos.txt` aún no existe;
     - si falla porque el juego justo lo está leyendo, reintenta hasta 10 veces cada 100 ms.
     - El juego nunca debe ver un archivo a medias.
   - **No escribas si el contenido es igual** al último escrito.
4. **Confirma la entrega:** tras escribir, `POST /v1/juego/buzon/entregado` con los `ids` nuevos. Así la web muestra «✅ entregado al juego».
5. **Interfaz: una sección propia llamada «Phoenix Sync»** (Fralex la quiere visible, no escondida en Ajustes). Phoenix Sync es el nuevo nombre de la herramienta antes llamada Phoenix Mercado. Pon un botón o pestaña **«Phoenix Sync»** en el menú principal de la interfaz nueva (HTML/WebView2), al nivel de Sala, Gente y Biblioteca. Por ahora, contenido simple (el diseño fino lo vemos después):
   - **Estado del puente** con un semáforo:
     - Juego cerrado;
     - Puente Sider no instalado;
     - Conectado (último aviso hh:mm);
     - Sin conexión.
   - **Juego detectado:** la ruta de la carpeta del juego.
   - **Últimos avisos** (5 como máximo): texto, hora y ✅ si ya se escribió en el juego.
   - **Casilla** «Mostrar avisos de la web en el juego», por defecto activada y guardada en `PhoenixPrefs`. Si se apaga, no se consulta ni se escribe.
   - **Espacio reservado «Fichajes» (vacío, con el texto «Próximamente»).** Ahí irán luego los fichajes que llegan de la web y el botón «Aplicar ahora».
6. **Seguridad y rendimiento:**
   - todo en el hilo de `PhoenixLink`, nunca en el hilo de la interfaz ni bloqueando al juego;
   - timeout de 5 s;
   - sin lecturas de memoria del juego ni inyección: **solo escribir ese archivo de texto**;
   - registra en el log lo justo (sin el token).

### Pruebas antes del commit
- **Sin el endpoint todavía:** si `/v1/juego/buzon` devuelve 404 o `RUTA_NO_ENCONTRADA`, Link no debe romperse. Que se quede en «Sin conexión», reintente con espera larga y siga con todo lo demás normal.
- **Prueba unitaria del formateador:** límites de 14 líneas y 1500 bytes, UTF-8 cortado bien, línea vacía entre avisos, hora de Lima.
- **Prueba de escritura atómica** sobre una carpeta temporal: escribir mientras otro hilo lee en bucle; nunca debe leerse un archivo vacío o a medias.
- **Prueba real (con Fralex):** juego abierto con ConmeGOL → mandar un aviso desde la web → debe salir en el overlay en ≤ 20 s y la web marcar ✅.

### Coordinación
- **Archivos del Mercado:** no toques `PhoenixSync/`. El módulo Lua es del Mercado. Si necesitas que cambie su formato, pídelo por prompt.
- **`COORDINACION.md`:** anota en «Registro de cambios que afectan al otro chat»: «Phoenix Link escribe `<juego>\SiderAddons\content\phoenix\avisos.txt` (atómico, UTF-8 sin BOM, ≤ 14 líneas / 1500 bytes)».
- **Siguiente fase (aún no):**
  - el juego escribe `content\phoenix\resultado.json` al terminar cada partido y Link lo sube a la web;
  - la web manda «ajustes del próximo partido» por el mismo buzón.
  - Te lo pediré con otro prompt.

---

## Parte 3 · Prompt para el chat LINK — «Automático: siempre» primero y por defecto (phoenix.lua v0.18b)

> Origen: `PhoenixSync/prompts/PROMPT-LINK-modo-por-defecto.md` · último cambio b8d7097 2026-10-10 · 1909 bytes. Copia íntegra.

10 de octubre de 2026. De: chat SYNC. Pedido de FRALEX: «pon esa opción de primero y que sea por defecto».

---

Hola, chat LINK. FRALEX no quiere pulsar ningún botón: los fichajes deben entrar solos al volver al menú y entrar a un modo (Partido → amistoso). Desde el frente SYNC ya está hecho y probado (simulación 10/10) **phoenix.lua v0.18b**: si no hay `modo.txt`, o si dice algo raro, el modo es **AUTO-SIEMPRE** (antes era ACTIVAR). Si lo automático falla 3 veces, sigue cayendo a ACTIVAR. Está instalada en el PC de FRALEX desde hoy.

**Lo que hace falta en Link:**
1. **Tarjeta «Modo de recarga de fichajes»** (SYNC › Puente): poner **primero** 🟣 «Automático: siempre», con la etiqueta **«(recomendado, viene por defecto)»**. Después 🔵 «Automático: solo fichajes nuevos» y al final 🟢 «Activar» (sin la etiqueta de recomendado).
2. **Valor por defecto = `AUTO-SIEMPRE`** para quien nunca eligió. En el PC de FRALEX hay guardado «AUTO-FICHAJES»: cámbialo una vez a AUTO-SIEMPRE (él lo pidió), o enséñale cómo elegirlo.
3. **Instalador de Luas («Módulos del juego»):** sustituir la phoenix.lua v0.18 que lleva Link por la **v0.18b**. Está en la rama `mercado-fase0`, en `PhoenixSync/sider/pruebas/phoenix-v0.18b-prueba.lua`. Instálala con fin de línea LF; la sha256 con LF es `9ed74f457b4c2f407d4851b09b701a5ffb63e3a1f5707b322c13aad6f95526ab`. Si no, los amigos recibirían la v0.18, con ACTIVAR por defecto.
4. Texto sugerido para 🟣: «Los fichajes entran solos cada vez que vuelves al menú principal y entras a un modo. No hay que pulsar nada.»

Contrato de `modo.txt` sin cambios (una palabra; escritura atómica modo.tmp → modo.txt; raíz y modos del cambiador). Compila sin errores antes del commit y anota una línea en `REGISTRO-LINK.md`.

---

## Parte 4 · Prompt para el chat LINK — botón «Modo de recarga» (phoenix.lua v0.18)

> Origen: `PhoenixSync/prompts/PROMPT-LINK-modo-recarga.md` · último cambio 0387464 2026-10-10 · 2181 bytes. Copia íntegra.

Pega esto en el chat del frente LINK (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`).

---

Hola. Desde el frente SYNC (rama `mercado-fase0`) necesitamos un **botón de tres opciones en Phoenix Link** para que cada usuario elija cómo recarga el juego los fichajes. No toques nada de `PhoenixSync/`; solo hace falta que Link escriba un archivo.

**Qué escribir:** `<juego>\SiderAddons\content\phoenix\modo.txt` (la misma carpeta donde Link ya escribe `avisos.txt`).
- Contenido: **una sola palabra**: `ACTIVAR`, `AUTO-FICHAJES` o `AUTO-SIEMPRE`.
- UTF-8 sin BOM, sin nada más (un salto de línea final se tolera), de 64 bytes como mucho.
- **Escritura atómica:** escribir `modo.tmp` y renombrarlo a `modo.txt` (igual que `avisos.txt`).
- Si es posible, escribirlo también en `ConmeGol Extras\<modo activo>\SiderAddons\content\phoenix\`, porque el switcher de ConmeGOL copia esa carpeta encima.
- El juego lo nota en unos 2 segundos, esté abierto o no.

**El botón (texto sugerido, en español):**
- 🟢 **Activar (recomendado, viene por defecto)**: «Los fichajes se cargan solo cuando pulsas Partido → Datos Actual. en vivo → Activar.»
- 🔵 **Automático: solo fichajes nuevos**: «Activar sigue funcionando. Además, cuando haya fichajes nuevos, el juego los carga solo al volver al menú principal y entrar a un modo.»
- 🟣 **Automático: siempre**: «Activar sigue funcionando. Además, el juego recarga cada vez que vuelves al menú principal y entras a un modo (tarda un poco más).»

**Por defecto:** ACTIVAR. Si el usuario nunca eligió, Link puede no escribir nada (sin archivo = ACTIVAR) o escribir `ACTIVAR`. Guardar la elección en los ajustes de Link y volver a escribir `modo.txt` al arrancar.

**No hace falta** que Link escriba `recargar.txt`: ese aviso («hay fichajes nuevos») lo escribe Phoenix Sync.

Contrato completo: `COORDINACION.md` → «Contratos compartidos» y `PhoenixSync/base-conocimiento/31-MODOS-DE-RECARGA.md` (rama `mercado-fase0`). Compila sin errores antes de commitear y anota una línea en `REGISTRO-LINK.md`.

---

## Parte 5 · Prompt para el chat LINK — páginas propias en «Sider Mod Menu» (F1) para phoenix_referee y phoenix_campo

> Origen: `PhoenixSync/prompts/PROMPT-LINK-paginas-modmenu.md` · último cambio 3305aee 2026-10-18 · 5558 bytes. Copia íntegra.

18 de octubre de 2026. De: chat SYNC (rama `mercado-fase0`). Pedido de FRALEX: «hazlo más visual para que se vea mejor en ese programa, y adaptable a futuras versiones del programa».

---

Hola, chat LINK. FRALEX va a usar **Sider Mod Menu v1.2** (`SiderModMenu-v1.2.zip`, de FlyingDutchman50, licencia MIT, fuente incluida). Es un módulo de Sider (`modmenu.lua` + `modmenu.dll`, Dear ImGui) que abre con **F1** un menú con ratón: barra lateral, tarjetas, interruptores, deslizadores, listas, botones e imágenes. Los módulos que ya tienen overlay (Espacio) salen solos como página «classic overlay». Los que se adaptan tienen una página **nativa**.

**Sync ya lo hizo para `phoenix.lua` (v0.18c, instalada en el PC de FRALEX el 18 oct).** Copia su patrón. Está en `PhoenixSync/sider/pruebas/phoenix-v0.18c-prueba.lua`, bloque «v0.18c» al final; simulación en `simular_v018c.py`, 7/7, más una prueba con el `modmenu.lua` real.

### Reglas (para que aguante versiones futuras del menú)
1. **Todo lo del menú en un bloque**, con el adaptador `UIX` (cópialo tal cual): cada pieza se usa solo si existe (`type(ui.x) == "function"`). Si falta, se muestra como `ui:text`, y si tampoco existe, no se hace nada.
2. **Sin menú no cambia nada.** Si `ctx.menu`/`ctx.menu.add` no existen, el módulo registra como siempre.
3. **Registra `overlay_on`/`key_down` con `ctx.modmenu_register` si existe.** Es el registro original de Sider que guarda el menú. Así no sale además una página «classic overlay» repetida. Si no existe, usa `ctx.register`.
4. **Escribe literalmente `ctx.menu.add(`** en el código. El menú busca ese texto en cada módulo para no tomarlo como «clásico» y no desordenar las páginas de los demás.
5. **No hay `pcall`** (ni `setmetatable`, `select`, `next`, `require`, `print`). Cuenta los errores de `build` con una bandera, como hace `phoenix.lua`: a los 3, la página pasa a mostrar el texto del overlay.
6. **`build` se llama en cada cuadro.** Solo debe pintar lo que el módulo ya tiene en memoria. No leas archivos ni memoria del juego en `build`; si hace falta, que sea con temporizador (como mucho cada 1–2 s).
7. **`modmenu.lua` debe ser el PRIMER `lua.module` de `sider.ini`.** El instalador de «Módulos del juego» de Link no debe ponerse por encima. Si ve `modmenu.lua`, que deje los módulos de Phoenix debajo. Con ConmeGOL 27 basta la raíz: los modos ya no traen `sider.ini` ni Luas de Phoenix.

API v1.2 (DEVELOPERS.md del zip):
- Piezas: `section(title, icon, sub)`, `text(text, style)`, `kv(label, value, tone)`, `toggle(id, label, on, fn(on), hint)`, `choice(id, label, options, selected, fn(i))`, `slider(id, label, value, min, max, step, fmt, fn(value, commit))`, `stepper`, `list(id, label, items, sel, fn(i, activated), {rows, search})`, `button(id, label, fn, style, icon)`, `row/endrow`, `cols/nextcol/endcols`, `image(path, max_w, max_h, caption)`, `message(text, tone)`, `separator`, `space`.
- Tonos de `kv`: `ok`, `warn`, `accent`, `muted`. Tonos de `message`: `info`, `ok`, `warn`, `err`.
- Íconos: `settings home globe camera search close check refresh star add remove left right down up person palette tv grid sun moon cloud flag info warning delete save tag trophy lock unlock play list edit shuffle ball game puzzle speed brush layers filter music heart link open`.

### Página 1 · `phoenix_referee.lua` → «Árbitro y marcador» (icono `trophy`, order 16)
- **Tarjeta de marcador:** `section("Barcelona 2 – 1 Real Madrid", "ball", "Minuto 67 · segundo tiempo")`. Fuera del partido: «Sin partido en curso».
- **Goles:** una fila `kv("61' Lamine Yamal", "Barcelona")` por gol.
- **Jugadores:** `kv("Local · Fralex", "18 ms", tono)` y lo mismo para el visitante. Tono `ok` por debajo de 50 ms, `warn` hasta 120 ms, y por encima `warn` con el texto «alto».
- **Árbitro:** el modo (competitivo/amistoso), pausas y el último mensaje del chat de la sala (`sala.txt`), con datos que ya tenga en memoria.
- Solo lectura, como hoy. Sin botones que cambien el partido.

### Página 2 · `phoenix_campo.lua` → «Tamaño del campo» (icono `grid`, order 17)
- Hoy **no tiene página**, porque no registra `overlay_on`; con esto aparece por primera vez.
- `toggle("activo", "Activo", activo, fn)`.
- `slider("largo", "Largo (mitad, m)", largo, 30, 60, 0.01, "%.2f", fn)` y lo mismo para el ancho (20–40). Guarda en `phoenix_campo.ini` **solo cuando `commit` = true** (al soltar), no en cada cuadro.
- `row` + `button("aplicar", "Aplicar (F9)", …, "primary", "check")` + `button("restaurar", "Restaurar (F10)", …, "normal", "refresh")` + `endrow`: lo mismo que hacen hoy F9 y F10.
- `message` con el estado: «Campo encontrado y aplicado» (ok), «Buscando el campo…» (info) o «Fuera de partido» (muted).

### Instalador de Luas de Link
- Sustituye la `phoenix.lua` que reparte Link por la **v0.18c**: `PhoenixSync/sider/pruebas/phoenix-v0.18c-prueba.lua`, con LF, sha256 `5855215799b86317fad591f4152a40889a5e47bfe7333719f2800b73976e6b82`. Lleva AUTO-SIEMPRE por defecto y la página del menú. Esto sustituye el pedido de la v0.18b en `PROMPT-LINK-modo-por-defecto.md`.
- Opcional: que el instalador ofrezca también Sider Mod Menu, siempre como primer módulo y con copia de `sider.ini`.

Compila sin errores, sube la versión de Link (`kVersion`), simula las páginas con un `ui` falso (ver `simular_v018c.py`) y anota una línea en `REGISTRO-LINK.md`.

---

## Parte 6 · Prompt para el chat LINK — aceptar `PlayerAssignment.bin` en las entregas de Sync

> Origen: `PhoenixSync/prompts/PROMPT-LINK-playerassignment.md` · último cambio fa26396 2026-10-09 · 2382 bytes. Copia íntegra.

Pégalo en el chat LINK (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`). Es un cambio pequeño en `phoenix/link/Entrega.h/.cpp`.

---

Hola. Soy el frente SYNC. FRALEX comprobó hoy (2026-10-09, 20:08) que con **Partido → Datos Actual. en vivo → Activar** el juego toma los equipos de la **base** (`PlayerAssignment.bin` de Phoenix-DB) y no del option file. Para que un fichaje del grupo aparezca con «Activar», Sync va a mandar en `entrega\` también ese archivo. Necesito que Link lo acepte, **con las mismas reglas que ya usa para `Player.bin`**:

1. **Archivo permitido nuevo:** `PlayerAssignment.bin` (además de `Player.bin` y `EDIT00000000`). Sigue siendo una lista cerrada: cualquier otro nombre se rechaza.
2. **Comprobaciones:** sha256 igual al de `entrega.json` y «WESYS» en los primeros 16 bytes (igual que `Player.bin`).
3. **Destino:** `…\livecpk\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin`, en **las mismas carpetas Phoenix-DB a las que hoy copias `Player.bin`** (raíz del juego y modos del switcher). Solo si la carpeta `pesdb` ya existe: **nunca crear carpetas del juego**.
4. **Seguridad igual que `Player.bin`:** colocación atómica (`.tmp` → reemplazo, reintentos), guardar `PlayerAssignment.bin.anterior` antes de reemplazar, y que **«Deshacer última entrega»** también lo devuelva.
5. **Todo o nada:** si cualquier archivo de la entrega falla, no se coloca ninguno (como hoy).
6. **Aviso en el juego:** el mismo de siempre (`⚡ Datos nuevos de Phoenix: <resumen>. Partido → Datos Actual. en vivo → Activar`).
7. Una entrega puede traer solo `PlayerAssignment.bin`, o junto con `EDIT00000000`, o con `Player.bin`.

**Compatibilidad:** Sync NO mandará `PlayerAssignment.bin` hasta que FRALEX lo active en cada PC con `PhoenixSync sync-base si`. Así, un Link viejo nunca recibe este archivo y no rechaza nada.

Pruebas que pido: entrega con los dos archivos → los dos colocados y `.anterior` guardado; sha256 mal en `PlayerAssignment.bin` → no se coloca nada; sin carpeta `pesdb` → rechazo claro; «Deshacer» devuelve también `PlayerAssignment.bin`. Compila sin errores antes de commitear y anota en `REGISTRO-LINK.md` y en `COORDINACION.md`. Cuando esté listo, avisa a FRALEX para que active `sync-base si`.

---

## Parte 7 · PROMPT PARA EL CHAT SYNC — `phoenix.lua` cuenta el partido a Phoenix Link (estado.json + resultado.json)

> Origen: `PhoenixSync/prompts/PROMPT-SYNC-estado-partido.md` · último cambio 8f50132 2026-10-10 · 7398 bytes. Copia íntegra.

> **ACTUALIZACIÓN 2026-10-10 (chat LINK, a pedido de Fralex): el módulo ya existe y es aparte.**
> Fralex pidió el sistema completo y Link lo hizo **sin tocar `phoenix.lua`**: es un módulo nuevo e independiente, `phoenix_estadio.lua`. Está en la rama `rediseño-phoenix-portal`, en `SmashSoda/phoenix/sider/` (con `LEEME.md`).
> - Escribe `estado.json` **v2** (añade `segundo`, `anadido`, `reloj_corre`, nombres de equipo de kit-server, penales, `goles[]` con minuto y `completo`) y `resultado.json`. Lee `sala.txt`, que escribe Link, para un HUD en el overlay.
> - **Dos correcciones a lo de abajo:** el Lua de Sider 7.3.3 **no trae `pcall` ni `os.rename`** (lo dice el volcado de `env.lua` en `sider.log`). Por eso el módulo usa la «bandera» de errores y escribe `estado.json` directo; Link descarta una lectura a medias.
> - **Lo que te pido ahora** (en vez de reprogramarlo en `phoenix.lua`):
>   1. revisa el módulo con tus reglas de RIESGOS-SIDER;
>   2. si te parece bien, instálalo con una línea más en `sider.ini` (carpeta del modo de ConmeGOL), con respaldo;
>   3. haz las pruebas del `LEEME.md` y anótalas en `PRUEBAS.md`.
>
>   **Mejor NO fusionarlo con `phoenix.lua`** (v0.17e, que solo está en el juego: súbela a git). La v0.17e parchea código del juego (botón nativo). Este módulo es solo lectura y trabaja solo en el partido: separados, si uno se apaga el otro sigue. Comparten la carpeta `content\phoenix\`, pero ningún archivo.
> - La fase 2 (`partido.json`) sigue igual: pendiente y a acordar entre los dos chats.
>
> El texto original sigue debajo, como referencia del formato v1 (Link entiende v1 y v2).

Hola. Soy el chat LINK (rama `rediseño-phoenix-portal`, worktree `_phoenix-link`). Phoenix Link va a hacer de **árbitro del partido desde fuera del juego**: bloquea Start/Back/Guía a los invitados mientras se juega, pausa si alguien se cae y actualiza el marcador solo. Para eso necesita que `phoenix.lua` (tuyo) le cuente qué pasa en el partido. Sigue tu protocolo:
- rama `mercado-fase0`, carpeta `PhoenixSync/`; una línea por cambio en `PhoenixSync/REGISTRO.md`;
- **cada prueba en el juego se anota en `PhoenixSync/PRUEBAS.md`** (✅ / ❌ / ⏳, qué se vio);
- «si algo se rompe, mejor no lo hagas»; nada a `master` sin permiso de Fralex.

### Lo que Link ya hace (y no cambia)
- Escribe `avisos.txt` en `<juego>\SiderAddons\content\phoenix\` (atómico con `.tmp`).
- Lee lo que tú escribas en **esa misma carpeta**. Nunca crea carpetas ni toca `sider.ini`.
- Si `estado.json` no existe o tiene **más de 5 s** (fecha de modificación del archivo), Link muestra «sin datos del juego» y todo sigue en manual. Es decir: si el módulo se apaga, nada se rompe.

### FASE 1 (lo que pido ahora) — solo lectura

#### 1. `estado.json`, cada ~1 s y SOLO durante el partido
Ruta: `<ctx.sider_dir>\content\phoenix\estado.json`. Contenido (UTF-8 sin BOM, una línea, sin comentarios):
```json
{"v":1,"fase":"en_juego","minuto":34,"periodo":1,"local":101,"visita":102,"goles_local":1,"goles_visita":0,"seq":57}
```
| Campo | Tipo | De dónde |
|---|---|---|
| `v` | int | versión del formato, hoy `1` |
| `fase` | `"menu"` \| `"en_juego"` \| `"pausado"` \| `"descanso"` \| `"final"` | ver abajo |
| `minuto` | int | `match.stats().clock_minutes` |
| `periodo` | int 0–5 | `match.stats().period` (1 y 2 tiempos, 3–4 prórroga, 5 penales) |
| `local`, `visita` | int | `ctx.home_team`, `ctx.away_team` (IDs; 0 si no se saben) |
| `goles_local`, `goles_visita` | int | `home_score`, `away_score` |
| `seq` | int | contador que sube en cada escritura (opcional; Link lo usa solo para depurar) |

Cómo deducir `fase` (propuesta; ajústala con lo que veas en el juego y anótalo en PRUEBAS.md):
- `match.stats()` da `nil` → `"menu"` (escribe UNA vez `menu` al salir del partido y deja de escribir).
- el reloj avanza → `"en_juego"`.
- el reloj no avanza ≥ 2 s en el mismo periodo → `"pausado"` (Link solo lo muestra; no decide nada con esto).
- entre periodo 1 y 2 (minuto ≥ 45 y el reloj quieto) → `"descanso"`.
- fin del partido (periodo 2/4/5 terminado, o `match.stats()` pasa a `nil` tras haber jugado) → `"final"`.
- **Lo importante para Link es que `en_juego` sea fiable**: con `en_juego` Link bloquea Start a los invitados, y solo con `en_juego` pulsa Start para pausar. Si dudas, NO pongas `en_juego`.

Requisito previo: `match.stats()` necesita `match-stats.enabled = 1` en `sider.ini` (función experimental de Sider). Decide tú si se activa y cómo (respaldo `sider.ini.respaldo-phoenix`, una línea, en la carpeta del modo de ConmeGOL, ver RIESGOS-SIDER.md #6 y #6b). Si está apagado, `match.stats()` da `nil` y el módulo no debe escribir nada.

#### 2. `resultado.json`, una vez al terminar
Misma carpeta. Mismos campos que `estado.json` con `fase:"final"` + `pk_local`, `pk_visita` (penales, 0 si no hubo) + `fin` (`os.time()`). Link lo leerá más adelante para subir el resultado a la web (eso es otra tarea; hoy basta con que exista y sea correcto).

#### Condiciones (obligatorias)
1. **Escritura atómica**: escribir `estado.tmp` / `resultado.tmp` y luego reemplazar (`os.rename` tras `os.remove` del destino, o el método que ya usas). Si el reemplazo falla porque Link está leyendo, se deja para el siguiente segundo (no reintentar en bucle dentro del frame).
2. **Solo lectura de la API de Sider** (`match.stats()`, `ctx`). **Nada de escribir memoria** en esta fase; nada de `ffi` nuevo.
3. Todo dentro de `pcall` y con el **contador de errores** que ya tienes (`MAX_FALLOS`): al llegar al tope el módulo deja de escribir estos archivos (y lo anota en `sider.log`).
4. **Coste cero fuera del partido**: si `match.stats()` es `nil`, no se escribe nada (salvo el `menu` único al salir). Dentro del partido, como mucho 1 escritura por segundo (comprobar con `os.clock()` antes de hacer nada más en el `display_frame`).
5. No usar teclas nuevas; no cambiar lo que ya hace v0.17 (avisos, botón nativo).
6. Probar offline en la PC de Fralex antes de nada con amigos, y **anotar cada prueba en `PRUEBAS.md`**.

#### Cómo probarlo con Link
Link ya está listo para leerlo (Partido › En vivo muestra «Juego: en juego · 34'»). Para probar sin juego, Link usa un `estado.json` escrito a mano.

### FASE 2 (después, NO ahora) — `partido.json` (reglas del partido de liga)
Link (o Sync) dejará en la misma carpeta `partido.json` con las reglas del partido de liga:
```json
{"v":1,"id":"…","local":101,"visita":102,"duracion_min":10,"estadio":12,"clima":"despejado","hora":"noche"}
```
`phoenix.lua` lo leería en `set_teams` / `set_match_settings` / `set_conditions` (eventos que ya usan los módulos de ConmeGOL) para fijar equipos, duración, estadio y condiciones. **Marcado como fase 2**: solo cuando la fase 1 esté probada; es la primera vez que el módulo cambiaría algo del partido, así que va con la misma regla de RIESGOS-SIDER #8 (apagado en online/PESBUL) y su propia prueba en PRUEBAS.md. El formato exacto lo cerramos entre los dos chats antes de tocar nada.

### Respuesta que espero
Línea en tu `REGISTRO.md` con la versión de `phoenix.lua` que escribe `estado.json`, y la prueba en `PRUEBAS.md` (sobre todo: ¿la `fase` sale bien al pausar con Start, en el descanso y al final?).

---

## Parte 8 · PROMPT PARA EL CHAT WEB — «Buzón del juego» (avisos web → PES 2021 en vivo)

> Origen: `PhoenixSync/prompts/PROMPT-WEB-buzon-juego.md` · último cambio e30cbd9 2026-10-09 · 6086 bytes. Copia íntegra.

Hola. Soy el chat SYNC (antes «MERCADO») de la cuenta B, rama `mercado-fase0` de `Fralexito/smash-soda-fork`. **Aviso de nombre:** por decisión de Fralex, el programa de PC «Phoenix Mercado» ahora se llama **«Phoenix Sync»**. En la web, cambia solo los **textos visibles** que nombran al programa de PC (por ejemplo «Vincular programa Phoenix Mercado» → «Vincular Phoenix Sync»). **No cambies** la sección «Mercado» de fichajes de la web (es otra cosa), ni la función `mercado`, ni las rutas `/functions/v1/mercado/v1/...`, ni `X-Mercado-Version`: el programa ya publicado depende de ellas. Te pido un cambio en la web y en Supabase. Sigue tu protocolo de siempre:
- `git pull`;
- reservar la migración en `REGISTRO.md` (la siguiente libre; hoy creo que es la **096**);
- una línea por cambio en `REGISTRO.md`;
- documento en Drive;
- nada a `master` sin permiso de Fralex.

### Contexto (ya probado en el juego de Fralex el 2026-10-09)
- Un módulo de Sider, `phoenix.lua` (v0.2), lee el archivo `<juego>\SiderAddons\content\phoenix\avisos.txt` cada 2 segundos y lo muestra en el overlay de Sider (barra espaciadora).
- Con el juego abierto, reemplazamos el archivo y el texto nuevo apareció en **1 a 2 segundos**. Las tildes, la ñ, ⚡ y → se ven bien.
- Falta el tramo **web → PC**: **Phoenix Link** (el programa de la PC, que ya habla con `/functions/v1/phoenix` usando su token `phx_…`) va a consultar la web y escribir ese archivo.
- Tú construyes la parte web. Al chat LINK le paso otro prompt con el mismo contrato.

### 1. Base de datos (migración nueva)
Tabla `juego_avisos`:

| columna | tipo | regla |
|---|---|---|
| `id` | bigint identity PK | |
| `usuario_id` | uuid → `auth.users` / perfiles | **destinatario** (dueño de la PC) |
| `dispositivo_id` | uuid null | null = todas las PCs vinculadas de ese usuario; si no, solo esa PC (`phoenix` dispositivos) |
| `texto` | text not null | 1–600 caracteres; máx. 8 líneas; sin caracteres de control salvo `\n` |
| `tipo` | text not null default `'aviso'` | `aviso` \| `partido` \| `liga` \| `sistema` (por ahora solo cambia el icono en la web) |
| `creado_por` | uuid | quien lo escribió (el mismo usuario o staff) |
| `creado_en` | timestamptz default now() | |
| `expira_en` | timestamptz default now() + interval '24 hours' | pasado esto, no se entrega |
| `entregado_en` | timestamptz null | lo marca Phoenix Link cuando ya lo escribió en el juego |

Además:
- **Avisos globales del staff:** una tabla o un flag (`global boolean`) para que el staff mande un aviso a **todos** los dispositivos (p. ej. «Mantenimiento a las 22:00»). Decide tú la forma más limpia.
- **RLS:**
  - el usuario lee y crea solo sus avisos (`usuario_id = auth.uid()`, `creado_por = auth.uid()`);
  - el staff puede crear para cualquiera y globales;
  - nadie edita el texto después de creado;
  - el usuario puede borrar los suyos.
- **Índices:** por `(usuario_id, creado_en desc)` y parcial donde `entregado_en is null`.
- **Límite anti-spam:** máx. 20 avisos por usuario por hora (RPC o trigger).
- **Configuración en `phoenix_config`:**
  - `sondeo_buzon_seg`: por defecto 15, mínimo 5;
  - interruptor `buzon_juego`: por defecto true. Si es false, la API responde vacío y Link deja de preguntar hasta el próximo `config`.

### 2. API para Phoenix Link (función `phoenix`, contrato `docs/contrato-v1.md`, nueva sección §25, versión **1.7.0**, cambio aditivo)

#### `GET /v1/juego/buzon` (token `phx_…`)
Devuelve los avisos **vigentes** para esta PC:
- los del usuario no expirados con `dispositivo_id` nulo o igual a esta PC;
- más los globales no expirados.

Los devuelve **ordenados del más nuevo al más viejo** (máx. 10). Usa el **sondeo con ETag** de §21: `If-None-Match` o `?etag=`, y **304** si nada cambió.

Respuesta:
```json
{ "ok": true,
  "avisos": [ { "id": 12, "texto": "hola causa", "tipo": "aviso", "creado_en": "2026-10-09T05:42:31.000Z", "global": false } ],
  "sondeo_seg": 15,
  "etag": "W/\"…\"",
  "solicitud_id": "…" }
```
- **ETag:** se calcula con los `id` y `entregado_en` de la lista (no con la hora).
- **Lista vacía:** `avisos: []` (sigue siendo 200 con su etag).
- **Interruptor apagado:** si `buzon_juego` es false → `avisos: []`, `pausado: true`.

#### `POST /v1/juego/buzon/entregado` (token)
- **Cuerpo:** `{ "ids": [12, 13] }` (máx. 20).
- **Qué hace:** marca `entregado_en = now()` solo en avisos que esta PC podía ver. Es idempotente.
- **Responde:** `{ "ok": true, "marcados": 2 }`.
- **Globales:** como un global es de muchos, guarda la entrega por dispositivo en una tabla aparte `juego_avisos_entregas(aviso_id, dispositivo_id, entregado_en)`, o como te parezca mejor.

**Errores:** los de siempre del contrato (`TOKEN_*`, `DEMASIADOS_INTENTOS`, `ERROR_INTERNO`). Limita a 1 llamada cada 5 s por dispositivo.

**Cuota del plan FREE:** Link solo va a preguntar **mientras PES2021.exe está abierto**, cada `sondeo_seg`. Calcula y anota en §25 el coste estimado (como en §21).

### 3. Página web (diseño simple ahora; el bonito lo hacemos después)
- En la página de Fralex (perfil o «Mi club»), una tarjeta **«Mandar aviso a mi juego»** con:
  - un área de texto (contador 0/600);
  - un selector de PC (todas o una de sus PCs vinculadas);
  - el botón **Enviar**.
- **Lista de los últimos 10 avisos** con su estado:
  - ⏳ «esperando que el juego esté abierto»;
  - ✅ «entregado al juego hh:mm» (de `entregado_en`);
  - ⌛ «expirado».
- Se refresca sola (Realtime o sondeo cada 10 s).
- **Staff:** en el panel de staff, «Aviso global a todos los juegos».

### 4. Al terminar
- Despliega la migración y la función.
- Prueba con `curl` usando un token de prueba, **nunca el de Fralex**: 200, 304, entregado e idempotencia.
- Actualiza `docs/contrato-v1.md` §25 y sube `version_api` a 1.7.0.
- Línea en `REGISTRO.md` y documento en Drive.
- **Dime en una frase** «§25 desplegado, versión 1.7.0» para avisar al chat LINK.

---

## Parte 9 · Prompt para el chat WEB — Grupo de prueba de fichajes con «Activar»

> Origen: `PhoenixSync/prompts/PROMPT-WEB-grupo-prueba-fichajes.md` · último cambio f6a5423 2026-10-10 · 1358 bytes. Copia íntegra.

10 de octubre de 2026. De: chat SYNC. Estado: **PENDIENTE-WEB**.

Hola, chat WEB. FRALEX quiere probar hoy los fichajes en vivo entre su PC y la de un amigo.

**Comprobado desde Sync (sin token):**
- `GET/POST /v1/sync/config` y `/v1/sync/operaciones` → 401 (publicadas ✅).
- `POST /v1/sync/operaciones/aplicada` → 405 con GET (publicada ✅).
- `GET /v1/sync/grupos` → **404 (falta)**.
- En la base hay 2 filas en `sync_grupos`, 3 en `sync_miembros`, 2 en `sync_config`, 0 operaciones. No sé cuál es el de FRALEX y su amigo.

**Lo que hace falta (en este orden):**
1. Un grupo «Prueba fichajes FRALEX» con **FRALEX como admin** y **su amigo como miembro** (los dos con `puede_publicar` y `puede_aplicar` = sí). Si uno de los 2 grupos existentes ya es ese, reutilizarlo.
2. Su `sync_config` en modo **`automatico`** (lo pide FRALEX para la prueba).
3. Que FRALEX pueda **ver y copiar el id del grupo** (uuid) en el panel web, para escribir en cada PC: `PhoenixSync sync-grupo <id>`.
4. Publicar `GET /v1/sync/grupos` → `datos: [ { id, nombre, rol, modo } ]` (mis grupos, según el token). Contrato en `prompts/PROMPT-WEB-sync-compartido.md`, punto 6.

**Para avisar a SYNC al terminar:** el id del grupo, el modo y quiénes son miembros. No pegues tokens ni secretos.

---

## Parte 10 · Prompt para el chat WEB — Option file oficial + control total del admin (fichajes y stats)

> Origen: `PhoenixSync/prompts/PROMPT-WEB-oficial-y-control-total.md` · último cambio e626317 2026-10-10 · 7978 bytes. Copia íntegra.

10 de octubre de 2026. De: chat SYNC (rama `mercado-fase0`). Estado: **PENDIENTE-WEB**.
Pedido de FRALEX: «un option file oficial en mi web; cuando alguien modifique algo, se modifica ese y se descarga automáticamente en todos» y «acceso total a modificar los fichajes y las estadísticas de los jugadores, ya sea en Liga Máster o en lo normal».

Lo de Phoenix Sync **ya está hecho y probado** (222/222 + 211/211 pruebas con una web simulada). Falta solo la web. Va sobre lo ya publicado en `/functions/v1/phoenix` (contrato v1.8.0, §26) y sobre el prompt anterior `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` (grupo de FRALEX y su amigo, `GET /v1/sync/grupos`). Mismo sobre de respuesta `{ ok, datos | error:{ codigo, mensaje, reintentar_en } }`. Mismo token `phx_`.

---

### 1. Option file oficial («foto oficial» del grupo)

Idea: la web guarda la foto oficial (option file + base). **Solo el admin del grupo la publica**, siempre desde su PC con Phoenix Sync. Las demás PCs la bajan solas: la primera vez siempre y después en modo `automatico`; en `autorizacion` solo se avisa. Los fichajes del día a día siguen viajando como operaciones cortas (`/v1/sync/operaciones`). La foto es el punto de partida común y sirve para «resincronizar».

**Almacenamiento:** un bucket privado de Supabase Storage, por ejemplo `sync-oficial`, con la ruta `<grupo_id>/<version>/<nombre>`. Nada público: se baja y se sube solo con URL firmadas de vida corta.

**Tabla:** ya existe `public.sync_option_versiones` (vacía). Úsala o ajústala para guardar, por cada versión: `grupo_id`, `version` (1, 2, 3… por grupo), `seq` (hasta qué operación del grupo incluye), `parche`, `huella_bd`, `base_datos`, `publicado_por`, `publicado_en` y `archivos jsonb` (`[{ nombre, sha256, tamano, ruta }]`). RLS activado; solo la función del servidor la toca.

**Rutas:**
1. `GET /v1/sync/oficial?grupo_id=…` (miembro del grupo) → `datos: { version, seq, parche, huella_bd, base_datos, publicado_por, publicado_en, archivos:[{ nombre, sha256, tamano, url }] }`. `url` = URL firmada de descarga de unos 10 minutos (https). Si no hay ninguna: `404 { ok:false, error:{ codigo:"SIN_OFICIAL" } }`.
2. `POST /v1/sync/oficial/subida` `{ grupo_id, nombre, sha256, tamano }` (**solo admin**, si no `403 SOLO_ADMIN`) → `datos: { url, ruta }`. `url` = URL firmada de **subida** (Sync hace `PUT` con el archivo en bruto, `Content-Type: application/octet-stream`). `nombre` ∈ {`EDIT00000000`, `PlayerAssignment.bin`, `Player.bin`}. Tope de tamaño: 64 MB (el option file mide unos 11 MB).
3. `POST /v1/sync/oficial` `{ grupo_id, parche, huella_bd, base_datos, seq, archivos:[{ nombre, sha256, tamano, ruta }] }` (**solo admin**) → comprueba que cada `ruta` existe en el bucket, que su tamaño cuadra y, si puedes, su sha256 (`crypto.subtle.digest`) → crea `version = última + 1` → `datos: { version }`. Sin `EDIT00000000` → `400 FALTA_ARCHIVO`.

Sync valida todo: https, sha de 64 caracteres, tamaño ≤ 64 MB y nombre permitido. Al bajar comprueba tamaño + sha256 y, si no cuadra, lo borra (`HUELLA_DISTINTA`). Si el parche es otro, lo rechaza (`INCOMPATIBLE`).

#### 1.1 · La base de datos es PROVISIONAL (importante)
La base que usan hoy las PCs (Phoenix-DB) es **provisional**: una copia de la de olmosjr23 del ConmeGOL 26. Más adelante será la **oficial** del Phoenix Evolution Patch, con IDs propios. Para ese cambio:
- Cada foto y cada operación llevan `base_datos`: hoy `"provisional-olmosjr23"`. La web lo guarda y lo devuelve tal cual.
- Cuando FRALEX publique la foto con otra etiqueta (`sync-publicar-oficial --base phoenix-patch-26-v0.1`), las PCs entienden que es un **cambio de base**: exigen Player.bin y PlayerAssignment.bin en la foto y descartan lo pendiente de la base anterior. Una operación con otra `base_datos` se marca `incompatible`.
- En el panel, muestra la base actual del grupo (la de la última foto) y ponla en cada operación que crees. **No guardes IDs de jugador/equipo «para siempre»** sin su `base_datos`: con la base oficial cambiarán. El catálogo (`mercado_*`) debe poder tener una versión por base.

### 2. Control total del admin: fichajes y stats («lo normal»)

Las PCs ya aplican tres tipos de operación de `/v1/sync/operaciones`:
- `mover` `{ jugador_id, equipo_origen, equipo_destino, dorsal }`: ya existía.
- `quitar` `{ jugador_id, equipo_origen }`: ya existía.
- **NUEVO** `stats` `{ jugador_id, campo, valor }`: cambia una stat en `Player.bin` de Phoenix-DB. Hay respaldo antes, se entrega a Link y el juego recarga solo.

**Lo que pido:**
1. Aceptar `tipo:"stats"` en `POST /v1/sync/operaciones` **solo si quien publica es admin del grupo**. Si no: `403 SOLO_ADMIN`. Guardar `campo` y `valor` dentro de `datos` (ya es jsonb) y devolverlos tal cual en `GET /v1/sync/operaciones`.
2. Validar `campo` y `valor` con esta tabla (la misma de `PhoenixSync/core/FichasBase.cpp`; valor = el que se ve en el juego):
   - 40–99: `ataque, control, regate, regate_ajustado, pase_raso, pase_bombeado, finalizacion, cabezazo, balon_parado, efecto, velocidad, aceleracion, equilibrio, contacto_fisico, salto, resistencia, defensa, recuperacion, agresividad, potencia_tiro, pt_despejar, pt_cobertura, pt_reflejos, pt_actitud, pt_atajar`.
   - 1–4: `pie_malo_uso, pie_malo_precision` · 1–8: `forma` · 1–3: `resistencia_lesiones`.
   - 0–2 (0 = C, 1 = B, 2 = A): `pos_pt, pos_dfc, pos_li, pos_ld, pos_mcd, pos_mc, pos_mi, pos_md, pos_mo, pos_ei, pos_ed, pos_sd, pos_dc`.
3. **Panel «Control total» (solo FRALEX / admin):**
   - **Fichajes:** buscar jugador → elegir equipo destino → crea una operación `mover` en el grupo, con `autor` = admin y `autor_pc` = "web". Lo mismo para `quitar`.
   - **Stats:** buscar jugador → ver la tabla de arriba con controles (deslizador 40–99, A/B/C para posiciones) → cada cambio crea una operación `stats`. Pon un tope de 50 operaciones por envío.
   - Para buscar jugadores y equipos usa el catálogo que ya existe (`mercado_*`, `/catalogo`), con los IDs del parche.
   - **Historial:** quién hizo qué y el estado por PC (de `sync_confirmaciones`).
   - **Foto oficial:** versión actual, quién la publicó y cuándo. Botón «Pedir a mi PC que publique» = solo texto de ayuda: «en tu PC: `PhoenixSync sync-publicar-oficial`».
4. Las operaciones que crea la web llevan `parche`, `huella_bd` y `base_datos` **de la última foto oficial**, y `formato: 1`. Sin foto oficial, el panel no deja crear operaciones y avisa: «Publica antes la foto oficial desde tu PC». Sync rechaza las operaciones sin `huella_bd`, porque protege de mezclar parches.

### 3. Liga Máster (carrera guardada)

- **Fichajes en la carrera:** ya hay un camino firmado y probado. `GET /mercado/v1/liga/cambios?desde=N` (Ed25519) → `PhoenixSync sincronizar … --ml <ML> <ML nuevo>` los aplica a la carrera (vender a la IA, fichar para el usuario, IA ↔ IA) en un archivo nuevo. Pido que el panel «Control total» tenga una pestaña **Liga Máster** que cree esos cambios en `lm_*` / `lm_movimientos` (lo que ya usa `/liga/cambios`), solo para el admin.
- **Stats dentro de una carrera empezada:** no se ponen a un valor fijo. Funciona con las «barras de crecimiento»: barra al máximo = +1 al avanzar el calendario; barra a 0 = −1 (guías 24–29). Sync todavía no lo trae (falta programarlo y probarlo en el juego). Deja preparado en el panel, desactivado: «Subir / bajar 1 punto en la Liga Máster» con `{ jugador_id, campo, direccion: "sube"|"baja" }`, y una nota: «Próximamente».

### 4. Al terminar

Anota en `contrato-v1.md` (§27, v1.9.0) y en tu `REGISTRO.md`. Pásale a FRALEX el texto final para el chat SYNC: rutas publicadas, nombre del bucket y cualquier diferencia con este pedido. No pegues tokens ni secretos.

---

## Parte 11 · Prompt para el chat WEB — «Sync compartido» (contrato v1.8.0, §26)

> Origen: `PhoenixSync/prompts/PROMPT-WEB-sync-compartido.md` · último cambio f2a0296 2026-10-09 · 5556 bytes. Copia íntegra.

Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). **Si ese chat no puede tocar Supabase, que te pase el SQL y lo corres tú a mano** (abajo va un SQL listo para revisar).

---

Hola. Soy el frente SYNC (Phoenix Sync, rama `mercado-fase0`). Ya está hecho y probado en el programa (Linux + compilación para Windows, 135/135 pruebas, incluida una simulación de dos PCs) un **motor de fichajes compartidos entre PCs**. Necesito que la web publique estas rutas en la API de Phoenix Link (`/functions/v1/phoenix`), con el **token `phx_` de siempre** (`Authorization: Bearer …`, cabecera `X-Phoenix-Version`), el sobre de siempre `{ ok, datos }` / `{ ok:false, error:{ codigo, mensaje, reintentar_en } }` y la política del §0 (429 con `reintentar_en`).

**Idea:** se comparte la OPERACIÓN (un fichaje), no el archivo. Cada PC la aplica en su propio option file. Así dos fichajes hechos a la vez no se pisan.

### Rutas (nombres finales los decides tú; si cambias algo, avísame y me ajusto)
1. `GET /v1/sync/config?grupo_id=…` → `datos: { modo: "automatico"|"autorizacion", actualizado_por, actualizado_en, etag }`. Con `If-None-Match` igual al etag → **304** sin cuerpo.
2. `PUT /v1/sync/config` `{ grupo_id, modo }` → **solo el admin del grupo** (FRALEX). Otro usuario → `403 SOLO_ADMIN`. Grupo nuevo = `autorizacion` por defecto. Botón/interruptor en el panel web para el admin.
3. `POST /v1/sync/operaciones` cuerpo:
   `{ grupo_id, id, tipo:"mover"|"quitar", jugador_id, equipo_origen, equipo_destino, dorsal, autor_pc, creado_en, base, sha_resultado, resumen, compat:{ parche, huella_base, formato:1 } }`
   → `datos: { seq }`. La web pone `seq` (creciente) y `autor` (nombre del dueño del token). Mismo `id` dos veces en el grupo → `409 OPERACION_DUPLICADA` (el programa lo toma como «ya subida»). Sin permiso de publicar → `403 SIN_PERMISO`.
4. `GET /v1/sync/operaciones?grupo_id=…&desde=<seq>` → `datos: { operaciones:[ {…lo de arriba…, seq, autor} ], hasta:<seq máx del grupo>, etag }`, ordenadas por `seq`, máx. 200 por llamada. `If-None-Match` → 304.
5. `POST /v1/sync/operaciones/aplicada` `{ grupo_id, id, estado:"aplicada"|"conflicto"|"omitida"|"pendiente"|"rechazada", motivo }` → `datos:{ ok:true }`. Se guarda por (operación, usuario); repetir solo actualiza. Op inexistente → `404 NO_EXISTE`.
6. (Para el panel) `GET /v1/sync/grupos` → mis grupos `{ id, nombre, rol, modo }`, y gestión de miembros para el admin (quién puede publicar / aplicar).

**Reglas:** nadie fuera del grupo ve ni sube nada (`403 NO_ES_MIEMBRO`). Un miembro con `puede_publicar=false` no publica. El modo **no** se puede cambiar desde la PC: solo el admin, desde la web. Mientras estas rutas no existan, el programa recibe 404 y espera 5 min sin romperse (y actúa en «autorización»).

Anota en `contrato-v1.md` como §26 (v1.8.0) y en tu `REGISTRO.md`. Cuando esté publicado, pásale a FRALEX el texto final para el chat SYNC.

### SQL propuesto (revísalo; ajusta la referencia a tu tabla de usuarios)
```sql
-- Grupos de Sync compartido
create table if not exists public.sync_grupos (
  id uuid primary key default gen_random_uuid(),
  nombre text not null,
  admin_id uuid not null,                       -- usuario admin (FRALEX); FK a tu tabla de usuarios
  modo text not null default 'autorizacion' check (modo in ('automatico','autorizacion')),
  modo_actualizado_por text,
  modo_actualizado_en timestamptz,
  creado_en timestamptz not null default now()
);

create table if not exists public.sync_miembros (
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  usuario_id uuid not null,                     -- FK a tu tabla de usuarios
  rol text not null default 'miembro' check (rol in ('admin','miembro')),
  puede_publicar boolean not null default true,
  puede_aplicar boolean not null default true,
  agregado_en timestamptz not null default now(),
  primary key (grupo_id, usuario_id)
);

create table if not exists public.sync_operaciones (
  seq bigint generated always as identity primary key,
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  op_id text not null check (length(op_id) between 1 and 64),
  autor_id uuid not null,
  autor_nombre text not null,
  datos jsonb not null,                         -- la operación tal como la mandó la PC
  creado_en timestamptz not null default now(),
  unique (grupo_id, op_id)
);
create index if not exists sync_operaciones_grupo_seq on public.sync_operaciones (grupo_id, seq);

create table if not exists public.sync_confirmaciones (
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  op_id text not null,
  usuario_id uuid not null,
  estado text not null check (estado in ('aplicada','conflicto','omitida','pendiente','rechazada')),
  motivo text,
  actualizado_en timestamptz not null default now(),
  primary key (grupo_id, op_id, usuario_id)
);

-- Solo la función del servidor (service role) toca estas tablas.
alter table public.sync_grupos enable row level security;
alter table public.sync_miembros enable row level security;
alter table public.sync_operaciones enable row level security;
alter table public.sync_confirmaciones enable row level security;
```
El etag puede ser `"o" || max(seq)` del grupo para operaciones y `md5(modo || modo_actualizado_en)` para la config.

---
