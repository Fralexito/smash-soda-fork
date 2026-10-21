# Prompt de traspaso — Parche Phoenix Evolution (PES 2021)

Copia todo lo que está debajo de la línea y pégalo como primer mensaje en el chat nuevo.

---

> **Actualizado el 2026-10-09:** lo más nuevo y completo está en `PhoenixSync/MANUAL-PARCHE-PHOENIX.md`. Si algo de este prompt choca con el manual, **manda el manual**.

Hola. Soy **FRALEX** (llámame siempre así). Vengo de otro chat donde trabajamos muchas horas en mi proyecto de **parche propio de PES 2021**. Te paso **todo el contexto** para que sigas exactamente donde quedamos. Léelo entero antes de hacer nada.

## 0. Cómo quiero que trabajes conmigo

- Háblame **siempre en español**, muy **simple, paso a paso, ordenado y espaciado**. Me mareo con mucha información junta o con muchos números y símbolos seguidos. Define cada término nuevo desde cero, usa ejemplos sencillos (incluso infantiles) y **repite** las cosas importantes si hace falta.
- **No me dejes mucho tiempo sin saber qué haces**: mándame avisos cortos de avance mientras trabajas.
- **Regla de oro: 0 errores.** El sistema tiene que quedar totalmente funcional. Si aparece un error, **no se avanza** hasta resolverlo o hasta que yo lo diga. Si algo puede romperse, mejor no lo hagas.
- Tienes **acceso total a mi PC** (a través de la app de escritorio de Claude, carpetas conectadas). Úsalo, pero **nunca cambies algo de mis juegos sin copia de seguridad** y siempre en archivos **nuevos**.
- **Nunca me pidas contraseñas ni secretos.**
- Quiero un **diario de prueba y error** de todo (qué se probó, qué falló, cómo se arregló), y al final un **PDF** que documente todo.
- La **base de conocimiento** del parche debe actualizarse **siempre también en Google Drive** (carpeta del proyecto, id `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`), no solo en el repo o la PC.
- Si descubres algo interesante durante cualquier análisis, **avísame** y dime qué podríamos hacer con eso.
- Haz toda la **ingeniería inversa** que haga falta.
- Las funciones deben ser **adaptables a cualquier parche** de PES 2021, siempre que se tenga la información de su base de datos.
- **No uses ni publiques contenido de otros moders** (caras, uniformes, estadios…) sin su **permiso** y sin darles **crédito**. Lo que fabriquemos nosotros (base de datos, programas, conexión con la web) sí es mío.
- Nunca se distribuye el juego: solo nuestros archivos.

## 1. El gran objetivo

Crear **el parche de PES 2021 más ambicioso, completo y sin errores que exista**: el **Phoenix Evolution Patch**. Tiene que ser **uno solo con todo mi ecosistema**:

1. **Mi web Phoenix Evolution**: liga, mercado de fichajes, perfiles. Repo `Fralexito/phoenixevolution`, con Supabase («Galaxy League»).
2. **Phoenix Link**: mi programa de **salas para jugar a distancia**. Es un fork de **Smash Soda 7.0.3** (C++, Parsec SDK, mandos virtuales ViGEm). El juego corre en la PC del anfitrión y los amigos lo reciben por streaming.
3. **Phoenix Sync**: el programa de PC que **mete en el juego lo que dice la web** (fichajes, dinero, plantillas) editando el **option file** y la **Liga Máster**.

Además quiero un **"PES online" propio**, conectado a mi web, y **funciones nuevas nunca vistas**.

## 2. Dónde está todo

- **Repositorio:** `Fralexito/smash-soda-fork`. En mi PC: `C:\dev\smash-soda-fork`.
  - Frente **MERCADO**: rama `mercado-fase0`, carpeta `PhoenixSync/`. Bitácora: `PhoenixSync/REGISTRO.md` (una línea por cambio). Diario de pruebas en el juego: `PhoenixSync/PRUEBAS.md`.
  - Frente **LINK**: rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`. Bitácora `REGISTRO-LINK.md`. Link compila en el worktree `_phoenix-link`.
  - Antes de tocar nada: leer `CLAUDE.md` y `COORDINACION.md`. Trabajo con **dos cuentas de Claude** sobre el mismo repo: siempre `git fetch` + `git pull --rebase` al empezar.
  - Reglas: no tocar la carpeta del otro frente; **nunca** `git checkout` de otra rama en `C:\dev\smash-soda-fork`; compilar sin errores antes de cada commit; una línea en la bitácora por cambio; **la web y Supabase se piden por prompt al chat de la WEB** (no se tocan desde aquí); nada va a `master` sin mi permiso.
- **Base de conocimiento** (repo y PDF): `PhoenixSync/base-conocimiento/`
  - `00-INDICE.md`
  - `01-MOTOR-PES2021.md`: formatos del juego; la sección J tiene el mapa ampliado de `Player.bin`.
  - `02-PARCHE-PHOENIX.md`: anatomía de un parche y plan.
  - `03-INVESTIGACION-PARCHES.md`: otros parches, causas de crash, modo online y herramientas.
  - `04-ESTRATEGIA-PHOENIX.md`: ConmeGOL vs Sudamerican, cómo superarlos, funciones nuevas, ruta por fases y proyección a futuro.
  - `parches/conmegol-26.md` y `parches/sudamerican-2026.md`.
  - Informes completos en `investigacion/`; datos en `datos/`.
  - PDF: `base-conocimiento-PES21.pdf` (33 páginas), que se genera con `generar_pdf.sh`.
- **Sider:**
  - `PhoenixSync/sider/`: módulo `phoenix.lua`, `PhoenixAviso.bat` / `.ps1` y `RIESGOS-SIDER.md`.
  - `AUDITORIA-SIDER.md` y la carpeta `auditoria/`: auditoría de los 49 módulos.
- **Juegos en mi PC:**
  - **ConmeGOL Patch 26:** `D:\Frank\Games_\Conmegol Patch\`. Guardados en `D:\Users\Alexander\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save`.
  - **Sudamerican Project 2026:** `D:\SP2026\`. Guardados en `…\292733975847239680\save`.
  - Carpeta de pruebas: `D:\Frank\Games_\Conmegol Patch\_PhoenixMercado_prueba` (exe del Mercado y `.bat` de prueba).
  - **Foto de versiones del 8 oct** (antes de las actualizaciones de los parches): `_PhoenixMercado_prueba\_versiones\2026-10-08\`.
- **Google Drive** (carpeta `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`), con estos documentos:
  - «Base de conocimiento PES 2021 — Phoenix (vivo)»
  - «Auditoría de Sider»
  - «Investigación de parches PES 2021 y modo online»
  - «Informe estratégico Phoenix»
  - «Proyección a futuro»
- **Página del experimento Barça:** https://claude.ai/artifact/WskFnXgRmMhAs12Mv6cwTh (página con base de datos: colecciones `jugadores/<pid>` y `club/barca`).

## 3. Lo que YA funciona (probado dentro del juego)

**Phoenix Sync** (C++; compila para Windows con MinGW, ejecutables estáticos):

- **Option file** (`EDIT00000000`, cifrado con libpesXcrypter):
  - Mover jugadores entre equipos con la **alineación bien hecha** (orden de formación y roles; sustituto por posición). Probado en un amistoso Boca.
  - Dorsales: el pedido, si no el de su club, si no el de su selección, si no el más alto libre (igual que el juego).
- **Liga Máster** (`ML0000000N`; ranura N = archivo N−1 en hexadecimal):
  - Vender de mi equipo a la computadora.
  - Traspasos entre equipos de la computadora.
  - **Fichar para mi equipo** (probado con Sommer: jugó, un solo contrato, dinero correcto). El contrato **no** se escribe en la tabla de contratos: el juego lo crea solo desde la ficha del bloque comprimido.
  - Cambiar el **presupuesto** y el **tope salarial**.
  - Leer y reescribir el **bloque comprimido (zlib)**: valor de mercado, sueldo, competiciones inscritas.
- **Conexión con la web:**
  - `/liga/cambios` viene **firmado** (Ed25519; clave `k67bd032d50`). Se aplica al option file y/o a la Liga Máster, **todo o nada**.
  - El fichaje para mi equipo ya va conectado.
  - Acepta campos opcionales `monto`, `sueldo`, `clausula`, `fin_contrato` (AAAA-MM-DD) y `dorsal` (1–99). **Ya le pasé el prompt a la web** para que los mande.
- **295/295 pruebas automáticas OK.**
- **Pendiente:** jugadores **libres** en la Liga Máster. Necesito hacer una prueba de referencia en el juego: fichar a un libre y rescindir a uno.

## 4. Sider (puente en vivo con el juego)

- Sider **7.3.3** en ConmeGOL y 7.3.4 en Sudamerican. Su Lua trae acceso a archivos, a la memoria del juego, `ffi`, `zlib`, `match.stats()` y una **pantallita (overlay)** que se abre con Espacio y cambia de módulo con la tecla 1. **No trae internet.**
- **El switcher de ConmeGOL copia `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons` encima de `SiderAddons` del juego.** Por eso cualquier cambio hay que hacerlo **en la carpeta del parche**. Así se perdió la línea de `phoenix.lua`, que **nunca llegó a cargarse**.
- `phoenix.lua` **está activo** (v0.10, instalado en la carpeta del modo): muestra `content\phoenix\avisos.txt` en el overlay (los avisos de la web llegan en vivo), tiene la tecla B (buscar una ficha en memoria con copia segura) y un **espía** de lecturas de la base. El Lua de Sider **no trae `pcall`**: se usa una bandera.
- **Auditoría** (antes de instalar nada; yo pedí un análisis profundo de **cada línea** para tener la máxima certeza de que nada choque):
  - 🔴 `camera.lua` + `DynamicWideCam.lua` parchean **la misma instrucción** y corrompen datos (comprobado en el log, en los **dos** parches).
  - 🔴 `GFX_lod.lua` escribe a ciegas.
  - 🟠 Riesgo medio: SoundServer, CGP_SB_Addons_2, shirtless_celebration (que añadí yo), real-coach-team, SleeveBadge, Commentary, Competition-Server.
  - 🟡 Módulos que no hacen nada: Chatty_AutoLineup (es de FL26), scoreboard-hexx, goalscreams, Anti-Cheat (placebo), env.lua.
  - 11 módulos del `sider.ini` no existen; «Menu Light» aparece dos veces.
  - No activar `attack_mentality.lua`.
  - Ojo: algunos módulos y archivos **los añadí yo a mano** (bajados de YouTube); no todos venían en el parche.
- **Plan acordado** (nada sin mi permiso):
  1. Yo juego **un partido de referencia** con el Sider actual.
  2. Limpieza con copia de seguridad: quitar camera.lua, GFX_lod, los inútiles y las líneas rotas. Otro partido y comparar los logs.
  3. Instalar `phoenix.lua` **en la carpeta del parche** y probar el aviso en pantalla.
- **Con amigos:** usar **Phoenix Link (Parsec)**. El juego corre solo en mi PC y ellos no instalan nada.

## 5. Modo online (lo realista)

- Los servidores de Konami cerraron en 2022, y PES 2021 de PC **no tiene LAN**.
- **PESBUL** (servidor de la comunidad, solo amistoso 1 contra 1) exige el juego de **Steam sin modificar y sin Sider**. Por eso no me funcionó: mis carpetas tienen Sider y archivos que no son de la instalación original. Su base es el servidor de código abierto de Nikow5 (MIT). En PES 6 existió algo parecido: `kinj1987/evo-league`.
- **Arquitectura elegida:**
  1. La **web** crea el partido.
  2. **Phoenix Link** (Parsec) en el PC anfitrión deja entrar solo a esos jugadores y carga el option file oficial.
  3. Un módulo **Sider** lee el marcador con `match.stats()` (no da goleadores ni tarjetas) y lo deja en `resultado.json` (el «buzón»).
  4. Phoenix Link lo sube a la web con una captura, y los dos jugadores confirman.
  5. El verificador revisa solo al anfitrión.
- Parsec desde Perú: subida ≥ 10 Mbps (30 recomendados), 60 fps, 720p/900p, cable, sin VPN. No funciona con CGNAT.
- Un servidor propio estilo PESBUL sería un proyecto de meses o años (frontera).

## 6. Lo aprendido de mis dos parches (los dos grandes referentes de qué hacer y qué no)

- **ConmeGOL 26:**
  - Casi todo por Sider (livecpk).
  - 90 competiciones, 749 equipos (el techo es 750), 29.997 jugadores (el techo es 30.000).
  - ⚠️ **La base que usa el juego es la que sirve Sider desde `SiderAddons\olmosjr23\Database`** (5/9/2026), no `CGP_database.cpk` (19/8). Tiene 5.894 jugadores distintos. **El catálogo subido a la web salió de la vieja: hay que regenerarlo desde la de Sider.**
  - La versión instalada es la 26; ya existe la 27. **Se viene una actualización.**
- **Sudamerican 2026:**
  - Parche «clásico» de CPK: 34 CPK, 13 de caras de 5–11 GB.
  - 80 competiciones y 726 equipos. Option file en portugués.
  - Trae **Liga 2 de Perú (14 equipos)**, que ConmeGOL tiene vacía.
  - Sus dos Sider tienen el mismo choque de cámaras, módulos que fallan al arrancar (randomMenu, TurfLoader, LogoHD), duplicados y `netblock.lua`, que bloquea el online.
  - **Se actualiza en una semana.**
- **Liga 1 2026** (18 equipos): igual en los dos.
- **Hueco libre:** no existe un parche de PC dedicado a la **Liga 1 de Perú** y las ligas andinas.
- **Protocolo cuando un parche se actualice:**
  1. Copia de la carpeta `save`.
  2. Comparar con la foto del 8 oct: base de datos, Sider y option file.
  3. Auditar solo lo nuevo.
  4. Regenerar el catálogo y subirlo.
  5. Probar.
  6. Anotar el «changelog técnico».
  - A futuro: un comando **«Auditor de parches»**.

## 7. Receta y ruta del Phoenix Evolution Patch

- **Base fija:**
  - PES 2021 de **Steam**, exe 1.07.02 + Data Pack 7.0.
  - Sider fijado (7.4.1, con `save.folder` para un EDIT propio).
  - Base de datos pequeña en un `.cpk` + contenido por Sider/livecpk en **addons por liga**.
  - Un `sider.ini` **auditado**.
  - **Lanzador propio** (Phoenix Link) sin `.bat`, que comprueba versiones, archivo de paginación y antivirus.
  - IDs reales de Konami.
  - **No cambiar el tamaño de una liga** sin parchear el exe (cuelga la Liga Máster).
  - Límites: 750 equipos, 40 por equipo, 30.000 jugadores.
- **Fases:**
  - **0.** Instalación limpia de referencia + partido de referencia. Pregunta abierta: **¿tengo PES 2021 en Steam?**
  - **1.** Versión 0.1: Liga 1 + Liga 2 + Copa, option file oficial, Sider limpio.
  - **2.** Fabricador de CPK y DpFileList (portar `the4chancup/pes-file-tools`), **Phoenix Doctor** (auditoría automática de Sider) y lanzador.
  - **3.** Conexión: avisos → resultado automático → verificador → option file por sala.
  - **4.** Addons de contenido con permisos y créditos.
  - **5.** Más allá: competiciones propias, estadísticas completas, Liga Máster viva y **Liga Máster online asíncrona** (cada amigo con su club y su carrera, compartiendo mercado y tabla en la web). Es la idea más original.
- **Funciones nuevas por viabilidad:**
  - 🟢 **Ya se pueden:** mercado conectado, Liga Máster controlada por la web, option file por sala, avisos, resultado automático, Phoenix Doctor, verificador, y «amistoso que se ve como partido oficial de mi liga» (como `CustomExhibitionMatch.lua`).
  - 🟡 **Con investigación:** goleadores y tarjetas desde la memoria (tabla de Cheat Engine de xAranaktu), cambios en vivo en la Liga Máster, CPK propios, competiciones propias (descifrar `Competition*` y `CompetitionRegulation`), calendario de la Liga Máster desde la web, actualizaciones por partes, ligas de más de 20 equipos, informe de errores.
  - 🔴 **Frontera:** Liga Máster online asíncrona, mundo real en el juego, servidor propio, espectadores en vivo, reglas propias.

## 8. Experimento en curso: «Vestuario Barça» (lo último que hicimos)

- Pedí que **todo mi equipo y mi Liga Máster sean totalmente editables, como en el juego e incluso más**: ficha de cada jugador con cara, escudo, camisetas, estadísticas, posición, dorsal, altura, habilidades…
- **Mi equipo de prueba es el Barcelona**: ID 108, carrera nueva en la **ranura 1** (`ML00000000`), índice 122 en la Liga Máster.
- **Hecho:** página editable (enlace arriba) con los 25 jugadores, minifaces y escudo (los sacamos del parche y los metimos en la página), datos de `Player.bin` de la base viva (olmosjr23), habilidades, posiciones jugables en un campo, valor y sueldo de la Liga Máster y finanzas. Los cambios se guardan como **pendientes** en la base de datos de la página (`cambios: [{campo, valor}]`).
- **Actualización 2026-10-09:** la técnica para aplicarlos ya está **probada** (Player.bin propio en `livecpk\Phoenix-DB` + Editar → Cargar). Falta automatizar. Plan original: editar `Player.bin` **solo en los campos confirmados**, servido por una raíz de Sider propia (`cpk.root` antes de olmosjr23), y la Liga Máster con lo que ya sabemos. Después marcar el jugador como aplicado.
- **Mapa de `Player.bin` (registro de 312 B, id en +8):**
  - Las 19 habilidades de campo ya estaban confirmadas.
  - Potencia de tiro: bit 358.
  - 5 de portero: bits 269, 300, 320, 326 y 364. **Falta saber cuál es cuál.**
  - Posiciones jugables: PT 350, DFC 468, LI 318, LD 474, MCD 414, MC 456, MI 466, MD 460, MO 464, EI 472, ED 476, SD 478 (probable), DC 470.
  - Pie malo, forma y lesiones: 454 / 462, 438, 458 (sin confirmar).
  - Habilidades especiales y estilos COM: bits 480–531, en calibración.
  - País: bits 10–18 de `Country.bin`.
  - Minifaces: `livecpk\Logos\common\render\symbol\player\<pid>.dds`. Escudos: `…\flag\e_000108_r_l.png`.
  - Faltan las fotos de Josué Caicedo y Gabriel Jesus. Las camisetas todavía no están. La cara 3D no se puede editar como foto.
- **La carrera se guardó recién creada:** todavía no tiene fecha (1/1), ni alineación, ni contratos definitivos.
- **Me toca a mí:**
  1. Avanzar al primer día de la temporada y guardar en la ranura 1.
  2. Mandar **fotos de la ficha en el juego** de **Joan García** (portero) y **Lamine Yamal**: todas las pantallas, para calibrar.
  3. Probar la página.

## 9. Pendientes generales (en orden)

1. Experimento Barça: calibrar con las fotos, aplicar los cambios al juego (`Player.bin` por Sider + Liga Máster) y probarlo dentro del juego.
2. Regenerar el catálogo con la base viva de Sider y subirlo a la web.
3. Jugadores libres en la Liga Máster (prueba de referencia en el juego).
4. Sider: partido de referencia → limpieza con permiso → `phoenix.lua` en la carpeta del parche → probar el aviso.
5. Cuando actualice ConmeGOL (mañana) y Sudamerican (en una semana): protocolo de actualización con la foto del 8 oct.
6. Fase 0 del parche (pregunta de Steam).
7. Subir a Drive todo lo nuevo (el experimento Barça todavía no está en Drive).

Empieza leyendo `COORDINACION.md`, el final de `PhoenixSync/REGISTRO.md` y `PhoenixSync/base-conocimiento/04-ESTRATEGIA-PHOENIX.md`. Después dime en simple qué entendiste y seguimos con el pendiente 1.

## 10. 🏆 Lo último: la «puerta en vivo» (2026-10-09)
- **Método probado:** Player.bin propio en `SiderAddons\livecpk\Phoenix-DB` (raíz antes de `olmosjr23\Database`) + **Editar → Cargar** en el juego ⇒ las stats cambian sin reiniciar (Lamine Velocidad 99 → 95 → 90, confirmado por huella).
- **Puente web → juego:** Phoenix Link escribe `content\phoenix\avisos.txt` y el módulo `phoenix.lua` lo muestra en el overlay de Sider (el «hola» de la web llegó al juego).
- **Detalle completo, fallos y universo de posibilidades:** `base-conocimiento/05-PUERTA-EN-VIVO.md`. Diario: `PRUEBAS.md`.
- **Futuro (no prioritario):** crear jugadores nuevos; avisos con aspecto nativo (imágenes de menú).
- **Phoenix Mercado ahora se llama Phoenix Sync** (carpeta `PhoenixSync/`; la rama sigue siendo `mercado-fase0`).
