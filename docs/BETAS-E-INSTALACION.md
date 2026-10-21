# Phoenix Link · Betas, instalación y pruebas con amigos

Cómo se arma, se reparte y se prueba la beta de Link: beta 1 (0.2.44), beta 2 (0.2.71), beta privada con caducidad por tester (0.2.83+) y la prueba online con un amigo por Parsec.

**Al 21-oct:**
- El amigo tiene la 0.2.71 y Link va por la 0.3.45.
- La licencia de Soda Arcade pide no subir los compilados con el SDK de Parsec al repo.
- El sello del dueño vive solo en `PhoenixBetaSello.h`, que está fuera de git.

> **Cómo está hecho este documento:** junta, **completos y sin recortar**, 6 documentos que antes estaban sueltos (reorganización del 21-oct-2026, por pedido de Fralex: «cada documento por tema y que tenga absolutamente completo»). Cada parte dice de qué archivo viene. Los originales siguen intactos en la carpeta `_archivo/` y en el historial de git. **Desde ahora, lo nuevo de este tema se AÑADE aquí** como una parte más, con: qué se logró, paso a paso para replicarlo, errores e intentos fallidos, cosas a tener en cuenta, y cómo comprobar y revertir.

## Índice

- [Parte 1 · Phoenix Link beta 1 (0.2.44) — notas de versión y estado](#parte-1-phoenix-link-beta-1-0244-notas-de-version-y-estado) — origen `docs/BETA-1-NOTAS.md` (a59c4b1 2026-10-12)
- [Parte 2 · Phoenix Link · beta 1 — Guía para instalar (una página)](#parte-2-phoenix-link-beta-1-guia-para-instalar-una-pagina) — origen `docs/BETA-1-GUIA-AMIGO.md` (e1333b3 2026-10-12)
- [Parte 3 · Phoenix Link beta 2 (0.2.71) — notas de versión, publicación y estado](#parte-3-phoenix-link-beta-2-0271-notas-de-version-publicacion-y-estado) — origen `docs/BETA-2-NOTAS.md` (1cad9e7 2026-10-12)
- [Parte 4 · Beta privada de Phoenix Link: cómo repartirla sin que se la queden (0.2.83+)](#parte-4-beta-privada-de-phoenix-link-como-repartirla-sin-que-se-la-queden-0283) — origen `docs/BETA-PRIVADA.md` (a5349cc 2026-10-13)
- [Parte 5 · Phoenix Link · beta 2 — Guía para instalar y probar (una página)](#parte-5-phoenix-link-beta-2-guia-para-instalar-y-probar-una-pagina) — origen `docs/BETA-2-GUIA-AMIGO.md` (053c502 2026-10-19)
- [Parte 6 · Prueba del modo online con un amigo (Phoenix Link + PES 2021 por Parsec)](#parte-6-prueba-del-modo-online-con-un-amigo-phoenix-link-pes-2021-por-parsec) — origen `docs/PRUEBA-ONLINE-CON-AMIGO.md` (053c502 2026-10-19)

---

## Parte 1 · Phoenix Link beta 1 (0.2.44) — notas de versión y estado

> Origen: `docs/BETA-1-NOTAS.md` · último cambio a59c4b1 2026-10-12 · 5400 bytes. Copia íntegra.

12 oct 2026. Primera base pensada para que **otra persona** instale y use Link sin ayuda y para que Sync funcione sin romper nada.
Guía para el amigo: `docs/BETA-1-GUIA-AMIGO.md`. Regla de Fralex: «si algo se rompe, mejor no lo hagas».

### Qué trae
- **Instalador de Link** (`PhoenixLink-instalar.bat`) en español: comprueba Windows 64 bits, Link cerrado, 12 GB libres e internet **antes** de instalar nada; no reinstala ViGEmBus ni WebView2 si ya están; baja el overlay **PhoenixGlass** con sha256 (ya no el Smash Glass viejo); comprueba al final que están todos los archivos; deja un registro en `%TEMP%\PhoenixLink-instalador.log`. Se puede repetir (actualiza sin tocar ajustes). Si falla, un mensaje claro y nada del juego tocado.
- **Instalador de módulos sin Link** (`PhoenixJuego-instalar.bat`): instala `phoenix.lua` v0.18b **y** `phoenix_referee.lua` v1.3; **desinstala** con `desinstalar`; aplica la regla «sin base reconocida no se pone Phoenix-DB» (igual que Link); copia de `sider.ini` con sello, vuelta atrás si algo falla a medias, versión fija por commit.
- **Sync › Puente**: tarjeta «Qué funciona en tu juego» (juego, módulos, avisos, fichajes) con el siguiente paso en cada caso; detección de módulos **perdidos tras actualizar el parche** («REINSTALAR»), **desactualizados** («ACTUALIZAR») o que Sider no pudo cargar; fichajes marcados «desactivados en este parche (próximamente)» cuando el `sider.ini` no tiene base reconocida (hoy, el parche 27).
- **Carpeta del juego a mano** (Módulos del juego): si Link no ha visto el juego, se pega la ruta y se valida; además Link recuerda la carpeta en cuanto ve PES abierto.
- **COPIAR INFORME** (Sala › Conexión › Diagnóstico del PC): versiones, parche, módulos, sha256 cortos y final de `sider.log`, para depurar sin acceso al PC.
- Preferencias de Link (`phoenix-ui.json`) con guardado atómico y copia `.bak`; si el archivo se daña, se lee la copia.
- Etiqueta **BETA 1** en la interfaz y en el informe.

### Qué se probó de verdad (y qué no)
**Probado** (en carpetas de prueba; el juego real de Fralex solo se **leyó**):
- Módulos: instalar, repetir (no cambia nada), actualizar (referee viejo → nuevo), detectar «perdido» tras un `sider.ini` sin líneas Phoenix, con el mismo código que usa Link.
- `PhoenixJuego-instalar.bat`: parche 26 simulado (pone Phoenix-DB), parche 27 simulado (no la pone), segunda ejecución (sin cambios), desinstalar y reinstalar, carpeta equivocada.
- `PhoenixLink-instalar.bat`: comprobaciones previas, fallo limpio con commit inexistente, descarga del overlay con sha correcto / incorrecto / 404 (servidor local). El juego real de Fralex: Phoenix y referee v1.3 ya instalados y cargados.
- Compila (0.2.44) y la interfaz nueva se vio en el simulador sin errores.
**Sin probar** (hace falta otra PC o tiempo):
- Una instalación completa desde cero en una PC limpia (Git, Visual Studio Build Tools, CMake, compilar 30–90 min). Se probó el tramo de compilación en limpio de la versión fijada; ver REGISTRO-LINK.
- El botón COPIAR INFORME y «Carpeta del juego a mano» dentro de Link real (solo el simulador).
- Sala con un amigo de verdad: mandos, retraso, campo 2D, árbitro con dos PCs.
- El overlay nuevo para el amigo hasta que Fralex publique el archivo (ver pasos).

### Qué queda fuera de la beta (desactivado o «próximamente»)
- **Fichajes de Phoenix en el parche 27.** Solo 2 de 175 fichajes de Fralex encajan en la base nueva y Phoenix-DB viene del parche 26: no se pone sin base reconocida. Plan: guardar fichajes como **cambios** sobre la base del parche activo (`docs/FICHAJES-COMO-CAMBIOS.md`).
- Orden de mandos en `settings.dat` (experimental, apagado por defecto).

### Pasos para Fralex
1. Cierra Link si está abierto y abre la versión 0.2.44 (ya compilada en `x64\Release`).
2. **Overlay para el amigo:** en GitHub → `Fralexito/phoenix-overlay` → Releases → *Draft a new release*: etiqueta `v1.0.0-beta1`, adjunta `C:\dev\phoenix-overlay\bin\PhoenixGlass.exe` (sha256 `DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94`) y publica. Sin esto el instalador sigue bien, pero sin overlay.
3. Si apruebas, ponle la etiqueta **`beta-1`** al commit de la beta (`git tag beta-1 <commit>` y `git push origin beta-1`). No se ha creado ninguna.
4. Pásale al amigo `PhoenixLink-instalar.bat`, `PhoenixJuego-instalar.bat` y la guía.

### Riesgos conocidos
- El instalador **compila** en la PC del amigo (30–90 min, ~12 GB): la licencia del SDK de Parsec no permite repartir el programa compilado. Una versión más rápida sin compilar exigiría permiso de Parsec.
- El repositorio público ya incluye `Dependencies\parsecsdk` (con `parsec.dll`): es decisión de Fralex si lo deja así.
- Si Visual Studio cambia el enlace `aka.ms/vs/18/...` o winget falla, el instalador puede fallar en esa parte; el registro dice dónde.
- El instalador fija un commit. Cada cambio de Link que el amigo deba recibir exige subir ese commit en el `.bat`.
- Los `.bat` fijan el sha256 de `phoenix.lua` y `phoenix_referee.lua`: si se editan, hay que actualizar el `.bat` o dirá «no es la versión esperada» (y no tocará nada).
- Otro parche de ConmeGOL puede mover sus carpetas o reescribir `sider.ini`: Link lo muestra como «perdido» y se arregla con REINSTALAR.

---

## Parte 2 · Phoenix Link · beta 1 — Guía para instalar (una página)

> Origen: `docs/BETA-1-GUIA-AMIGO.md` · último cambio e1333b3 2026-10-12 · 5729 bytes. Copia íntegra.

> **Esta guía es de la beta 1. La vigente es `docs/BETA-2-GUIA-AMIGO.md`.**


Para quien va a **hostear** con su propia PC (PES 2021 + ConmeGOL + Parsec). Si solo entras a la sala de otro por Parsec, no necesitas nada de esto.

### Antes de empezar
- Windows 10 u 11 de 64 bits e internet. Con el instalador rápido hacen falta **menos de 1 GB**; con el instalador que compila, **12 GB**.
- PES 2021 con ConmeGOL **funcionando** (ábrelo una vez con su lanzador, como siempre) y una cuenta de Parsec.
- Los archivos que te pasa Fralex: `PhoenixLink-instalar-rapido.bat` (recomendado) y `PhoenixJuego-instalar.bat`. El `PhoenixLink-instalar.bat` compila Link en tu PC y solo sirve si el rápido no está disponible.
- Tiempo: **unos 3 minutos** con el instalador rápido (de 30 a 90 si compilas).

### Paso 1 · Instalar Link
1. Doble clic en `PhoenixLink-instalar-rapido.bat`. Si Windows pide permiso, pulsa **Sí**.
2. Lee el aviso, pulsa una tecla y elige dónde instalar (o cierra la ventanita para usar `C:\Program Files`).
3. **Espera.** Baja Link (17 MB) y solo los componentes que le falten a tu PC (runtime de Visual C++, ViGEmBus, WebView2). Si Windows pide reiniciar, reinicia y repite.
4. Termina con **INSTALACION TERMINADA**. Acepta el acceso directo del escritorio.

Si sale **NO SE PUDO TERMINAR**: tu juego no se tocó. Manda a Fralex una foto de la ventana o el archivo `PhoenixLink-instalador.log` (está en `%TEMP%`: pega eso en la barra del Explorador).

### Paso 2 · Poner Phoenix en tu juego
1. **Cierra PES 2021 por completo** (y Sider si queda abierto).
2. Abre **Phoenix Link** (el acceso directo) e inicia sesión en Parsec cuando lo pida.
3. Ve a **SYNC → Módulos del juego**.
   - Si dice que no conoce tu juego: pega la carpeta donde está `PES2021.exe` y pulsa **GUARDAR**.
4. Pulsa **INSTALAR** en `phoenix_referee.lua` y en `phoenix.lua`. Deben quedar en **INSTALADO**.
   Link guarda una copia de `sider.ini` con fecha antes de tocar nada.
5. Abre PES con su lanzador. Vuelve a **SYNC**: la tarjeta «Qué funciona en tu juego» debe mostrar todo en verde.

Sin Link (solo los módulos): doble clic en `PhoenixJuego-instalar.bat`, elige la carpeta del juego y espera **LISTO**.

### Qué esperar en esta beta
| Función | Estado |
|---|---|
| Abrir sala, mandos, puestos, equipos A/B | Listo (lo usa Fralex; con otra persona, esta sesión lo comprueba) |
| Avisos de la web dentro del juego | Listo |
| Marcador, goles, HUD y árbitro | Listo, a probar contigo |
| **Fichajes de Phoenix** | **Se generan sobre TU base** (Link 0.2.46): SYNC › Puente › «Fichajes de Phoenix». No copia bases de nadie ni toca Player.bin; se quitan en un clic. Hoy la lista va vacía: no cambia nada hasta que haya fichajes. Sin probar aún dentro de PES |

### Fichajes de Phoenix: cómo funcionan y cómo probar uno
Link lee la base de datos de **tu** juego (la del modo que juegas), le aplica una lista corta de cambios «jugador → equipo» y deja solo `PlayerAssignment.bin` en `SiderAddons\livecpk\Phoenix-DB`. Funciona igual con ConmeGOL 26, 27, UEFAGOL 27 y Clásicos: si no entiende tu base, no crea nada. Detalle: `docs/FICHAJES-COMO-CAMBIOS.md`.

**Probar un fichaje (con PES cerrado y tu juego con copia hecha):**
1. Crea la carpeta `%APPDATA%\Phoenix Mercadoichajes` y dentro `fichajes.json` con un solo cambio de prueba (un jugador que conozcas y un equipo que existe en tu modo):
   `{ "version": 1, "cambios": [ { "player_id": 208750, "team_id": 139, "nota": "prueba" } ] }`
2. Link → SYNC → Puente → **Fichajes de Phoenix** → REVISAR: cada modo dice «Listo para aplicar» (o por qué salta el cambio).
3. **APLICAR FICHAJES DE PHOENIX**. Abre PES con el lanzador, ve a Editar equipos y mira si el jugador está en su equipo nuevo.
4. Si algo se ve raro: Link → **QUITAR** (vuelve a la base original) y manda el informe (Sala › Conexión › Diagnóstico del PC › COPIAR INFORME).

### Quitar Phoenix
- Del juego: Link → SYNC → Módulos del juego → **DESINSTALAR**. Sin Link: abre una consola en la carpeta del .bat y ejecuta `PhoenixJuego-instalar.bat desinstalar`. No borra nada: aparta los `.lua` y guarda copias de `sider.ini`.
- De Link: borra la carpeta `Phoenix Link` y el acceso directo. Tus ajustes quedan en `%APPDATA%\Trybuchet\Smash Soda`.

---

## Lista para la PRIMERA sesión de prueba (Fralex + amigo)

**Antes de abrir sala (5 min)**
- [ ] Link abre sin errores y dice **BETA 1** abajo a la izquierda.
- [ ] Sala › Conexión › Diagnóstico del PC: todo verde (WebView2, ViGEmBus, Parsec).
- [ ] SYNC › «Qué funciona en tu juego»: módulos al día; fichajes «desactivados (próximamente)» en el parche 27.
- [ ] Abre PES: en `SiderAddons\sider.log` aparece `[phoenix.lua]` sin «is NOT activated».

**Durante el partido**
- [ ] Entran los dos a la sala; suena/sale el aviso de entrada.
- [ ] Cada uno tiene su puesto y su mando funciona en el juego.
- [ ] El marcador y los goles aparecen en Link.
- [ ] Un aviso de la web sale dentro del juego (barra espaciadora → panel de Sider).
- [ ] Mira si hay retraso en los mandos (medidor de Link).

**Si algo falla, anota**
1. Qué hacías y qué pasó (una frase).
2. La hora aproximada.
3. Una foto de la pantalla de Link.
4. El informe: **Sala › Conexión › Diagnóstico del PC › COPIAR INFORME** y pégalo en el chat.
   (Lleva versiones, parche, módulos y el final de `sider.log`. Tapa tu nombre de usuario de Windows y no lleva contraseñas ni claves; trae una sección «Qué pasa y qué hacer» en lenguaje claro.)

**Reglas de oro:** no abras PES mientras instalas módulos; si dudas, no cambies nada y manda el informe.

---

## Parte 3 · Phoenix Link beta 2 (0.2.71) — notas de versión, publicación y estado

> Origen: `docs/BETA-2-NOTAS.md` · último cambio 1cad9e7 2026-10-12 · 5356 bytes. Copia íntegra.

12 oct 2026. Misma idea que la beta 1 (que otra persona instale y use Link sin ayuda), con todo lo que se hizo después: JUGAR, perfiles de amigos, equipos A/B, lados automáticos, aviso de entrada, campo 2D, fichajes sobre la base de cada PC y traspasos en vivo (apagado). Guía para el amigo: `docs/BETA-2-GUIA-AMIGO.md`. Escenarios de Sync: `docs/SYNC-ESCENARIOS.md`. Regla de Fralex: «si algo se rompe, mejor no lo hagas».

### El paquete
- **Zip de release:** `PhoenixLink-0.2.71-beta2.zip` (6,3 MB), construido desde una **copia limpia** del commit `cdd4b5f` (git worktree aparte, compilación propia; no se tocó el árbol de trabajo).
- **SHA256:** `93CEE03BF1A34BA6256F7239E464CFF4E3AE152B4BEAC140EA6AAF02C01FD909`.
- **Lleva:** `PhoenixLink.exe`, `parsec.dll`, `SDL2.dll`, `WebView2Loader.dll`, `nvEncodeAPI64.dll`, `ui\`, `sider\` (phoenix.lua v0.18b y phoenix_referee.lua v1.3), `fichajes\`, `fonts`, `icons`, `sfx`, `LICENCIAS.txt`. No lleva `.pdb`, `imgui.ini`, el overlay (se baja aparte) ni `NotoSansSC-VariableFont_wght.ttf` (17 MB; ningún código de Link lo carga: solo se usan Afacad Flux y Montserrat). Así el zip pesa 6,3 MB y entra en el límite de 10 MB de la herramienta de subida.
- **`parsec.dll`:** va dentro del zip porque Fralex confirmó (12 oct) que pidió permiso a Parsec y pagó las licencias para repartirlo. Si el permiso no cubre un release público, repartir el zip en privado y cambiar `PHX_ZIP_URL` en `PhoenixLink-instalar-rapido.bat`.
- **Para rehacerlo:** compilar en un worktree limpio y ejecutar `tools\EMPAQUETAR-RELEASE.ps1 -Release <x64\Release> -Salida <carpeta>`; luego poner el SHA256 y la URL en el `.bat` rápido.
- El zip está en `C:\dev\phoenix-beta2-dist\` (fuera del repositorio).

### Qué se probó de verdad (y qué no)
**Probado**
- Compilación **limpia** del commit `cdd4b5f` (configurar + compilar desde cero en carpeta aparte): sin errores; el `.exe` del zip es la 0.2.71.
- `PhoenixLink-instalar-rapido.bat` contra el zip local (servidor de prueba): instala en carpeta temporal con todos los archivos, rechaza un zip con SHA malo sin instalar nada, y actualiza encima de una instalación existente. Overlay con SHA correcto.
- Escenarios de Sync (módulos, `sider.ini`, parches 26/27, vueltas atrás): `pruebas-link\correr_escenarios.bat` → **55 comprobaciones, 0 fallos**.
- Las pruebas de las otras sesiones constan en `REGISTRO-LINK.md` (perfiles probados con el PES real de Fralex, fichajes con copias de bases reales).
**Sin probar**
- Abrir el Link instalado desde el zip en una PC limpia (no hay VM ni Windows Sandbox en esta PC), y la instalación real de Visual C++, ViGEmBus y WebView2.
- Una sala con un amigo de verdad (mandos, lados automáticos, equipos A/B, campo 2D, aviso de entrada).
- Fichajes y traspasos dentro del PES real; perfiles en el PC del amigo.

### Pasos para Fralex (publicar; son dos releases, sin `gh`, todo en la web)
1. **Zip de Link.** Abre https://github.com/Fralexito/smash-soda-fork/releases/new
   - *Choose a tag*: escribe `beta-2` y pulsa «Create new tag: beta-2 on publish».
   - *Target*: elige la rama `rediseño-phoenix-portal` (o el commit `cdd4b5f`).
   - *Title*: `Phoenix Link beta 2 (0.2.71)`.
   - Arrastra `C:\dev\phoenix-beta2-dist\PhoenixLink-0.2.71-beta2.zip` a «Attach binaries».
   - Marca **Set as a pre-release** y pulsa **Publish release**.
2. **Overlay (en zip, 6,6 MB, para caber en el límite de subida).** Abre https://github.com/Fralexito/phoenix-overlay/releases/new
   - Tag nuevo `v1.0.0-beta1`, título `PhoenixGlass 1.0.0 beta 1`.
   - Arrastra `C:\dev\phoenix-beta2-dist\PhoenixGlass-1.0.0-beta1.zip` (SHA256 `A83455807EA145E8ACCF77F81A7A5ABA5A47501AE981E988F68131A3C4A632E5`; dentro va el `PhoenixGlass.exe` con SHA256 `DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94`), pre-release, **Publish release**.
3. Avísame (o a la sesión que te lo pidió): comprobaré con `curl` que ambos enlaces responden 200 y que los SHA256 coinciden. Los enlaces que espera el instalador son:
   - `https://github.com/Fralexito/smash-soda-fork/releases/download/beta-2/PhoenixLink-0.2.71-beta2.zip`
   - `https://github.com/Fralexito/phoenix-overlay/releases/download/v1.0.0-beta1/PhoenixGlass-1.0.0-beta1.zip`
4. Pásale al amigo `PhoenixLink-instalar-rapido.bat` y `PhoenixJuego-instalar.bat` (como archivos, no copiados de la web) y `docs/BETA-2-GUIA-AMIGO.md`.

### Riesgos conocidos
- **Hasta publicar los dos releases, el instalador rápido da 404** y se detiene con un mensaje claro (sin instalar nada).
- El zip es una versión fija: cada cambio de Link que el amigo deba recibir exige regenerar el zip y su SHA256 en el `.bat`.
- `parsec.dll` en un release público: confirmar que el permiso de Parsec lo cubre.
- Interfaz y funciones nuevas (JUGAR, perfiles, campo 2D, lados automáticos) se probaron sobre todo en el PC de Fralex; en otro PC o parche pueden fallar. El informe (Sala › Conexión › Diagnóstico del PC › COPIAR INFORME) sirve para depurar.
- **Traspasos en vivo** viene apagado: ya habla con la web real (0.2.71) pero no se ha probado con el amigo; encenderlo solo de común acuerdo.
- Los `.bat` fijan el SHA256 de `phoenix.lua` y `phoenix_referee.lua`: si se editan, hay que actualizar el `.bat`.

---

## Parte 4 · Beta privada de Phoenix Link: cómo repartirla sin que se la queden (0.2.83+)

> Origen: `docs/BETA-PRIVADA.md` · último cambio a5349cc 2026-10-13 · 3840 bytes. Copia íntegra.

Qué protege y qué no: un programa que corre en el PC de otra persona no se puede blindar al 100 %. Lo que sí se consigue es **disuadir, rastrear de quién es cada copia y poder apagarla**. La parte de más valor (liga, radar, traspasos) vive en la web y esa sí la controlas.

### 1. Antes de repartir (una sola vez)
1. **Pon los repos en privado** (GitHub › cada repo › Settings › Danger Zone › Change visibility): `Fralexito/smash-soda-fork` y `Fralexito/phoenix-overlay`. Hoy son públicos. Mientras sigan así, **no se sube nada nuevo** (los cambios se guardan solo en tu PC con `git commit`).
2. Decide la **fecha de caducidad** de la beta: constante `kCaduca` en `SmashSoda/phoenix/PhoenixBuild.h` (formato `AAAA-MM-DD`). Pasada esa fecha Link no abre y pide la versión nueva; 7 días antes avisa al arrancar. Para renovar: cambia la fecha, compila y reparte los instaladores nuevos.
3. Tu PC está **exento** de la caducidad mientras exista el archivo marca que define `esDueno()` en `PhoenixBeta.h` (en la carpeta de configuración de Link). No lo nombres en ningún sitio público. Los testers no lo tienen.

### 2. Un instalador distinto por persona
```
powershell -ExecutionPolicy Bypass -File tools\CREAR-INSTALADOR-EXE.ps1 -Tester Mirko
```
- Crea `PhoenixLink-Setup-<versión>-Mirko.exe` en `C:\dev\phoenix-beta2-dist`.
- Cada instalador tiene un **SHA256 distinto**. Se anota en `testers.csv` (tester, fecha, versión, caducidad, sha256, archivo). Ese archivo es **privado**: no lo compartas.
- Lo que instala: `tester.id` (el nombre del tester) junto a `PhoenixLink.exe` y la `LICENCIA-BETA.txt`. Link manda el nombre en la cabecera `X-Phoenix-Tester` a la web.
- El asistente muestra la **licencia de la versión de prueba** y obliga a aceptarla antes de instalar.
- Si una copia aparece donde no debe: calcula su SHA256 (si es el instalador) o mira `tester.id` (si es la carpeta instalada) y búscalo en `testers.csv`.

### 3. Cómo mandarlo
- Un enlace **privado por persona**: Drive restringido a su correo, no «cualquiera con el enlace».
- Solo gente de confianza y pocas personas al principio.
- Un mensaje claro por escrito: «es una prueba privada, no lo compartas; caduca el AAAA-MM-DD».

### 4. Apagar a una persona (revocar)
En la web (Supabase), cada PC vinculada es una fila de `dispositivos_host`:
- **Suspender** (reversible): `select public.staff_suspender_dispositivo('<id del dispositivo>', true, 'motivo');` (con `false` lo reactivas).
- **Revocar** (definitivo): `select public.revocar_dispositivo('<id del dispositivo>');`
- Desde ese momento ese Link deja de funcionar en la liga, los traspasos y las salas («La web no reconoce este Link»). Lo local (abrir Link, mandos) sigue hasta que caduque la build.
- Para saber el id: `select id, usuario, nombre, creado from public.dispositivos_host order by creado desc;`

### 5. Límites honestos
- El reloj del PC se puede cambiar para saltarse la caducidad.
- Quien borre `tester.id` pierde el rastro por carpeta, pero el SHA256 del instalador sigue señalándolo.
- Los archivos de la interfaz (`ui`) y los módulos de PES (`.lua`) van legibles dentro de la instalación.
- La base del proyecto (Smash Soda) es de licencia MIT: esa parte la puede copiar cualquiera. Lo propio de Phoenix es lo que va cubierto por la licencia de la beta.
- `parsec.dll` (SDK de Parsec) va dentro del instalador. La licencia de la API de Soda Arcade dice que las versiones compiladas que se redistribuyen no deberían llevar el SDK de Parsec. Con pocas personas conocidas el riesgo es bajo; antes de repartir a más gente, decide si cada tester lo descarga de Parsec o si se pide permiso.
- Esta licencia está escrita en lenguaje sencillo y **no ha sido revisada por un abogado**.

---

## Parte 5 · Phoenix Link · beta 2 — Guía para instalar y probar (una página)

> Origen: `docs/BETA-2-GUIA-AMIGO.md` · último cambio 053c502 2026-10-19 · 5241 bytes. Copia íntegra.

> Actualizada al menú del plan UX (Link 0.3.34): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES. Mapa completo en `docs/GUIA-MENU-NUEVO.md`.

Para quien va a **hostear** con su propia PC (PES 2021 + parche + Parsec). Si solo entras a la sala de otro por Parsec, no necesitas nada de esto.

### Antes de empezar
- Windows 10 u 11 de 64 bits e internet. Hacen falta **menos de 1 GB** libres.
- PES 2021 con su parche **funcionando** (ábrelo una vez con su lanzador, como siempre) y una cuenta de Parsec.
- Los archivos que te pasa Fralex: `PhoenixLink-instalar-rapido.bat` y `PhoenixJuego-instalar.bat`.
- Tiempo: unos **3 minutos**.

### Paso 1 · Instalar Link
1. Doble clic en `PhoenixLink-instalar-rapido.bat`. Si Windows pide permiso, pulsa **Sí**.
2. Pulsa una tecla y elige dónde instalar (o cierra la ventanita para usar `C:\Program Files`).
3. **Espera.** Baja Link (17 MB) y solo lo que le falte a tu PC: runtime de Visual C++, ViGEmBus y WebView2. Si Windows pide reiniciar, reinicia y repite.
4. Termina con **INSTALACION TERMINADA**. Acepta el acceso directo del escritorio.

Si sale **NO SE PUDO TERMINAR**: tu juego no se tocó. Manda a Fralex una foto de la ventana o el archivo `PhoenixLink-instalador.log` (en `%TEMP%`: pega eso en la barra del Explorador).

### Paso 2 · Abrir Link y poner Phoenix en tu juego
1. **Cierra PES 2021 por completo** (y Sider si queda abierto).
2. Abre **Phoenix Link** e inicia sesión en Parsec cuando lo pida.
3. Ve a **SYNC → Módulos del juego**. Si no conoce tu juego, pega la carpeta donde está `PES2021.exe` y pulsa **GUARDAR**.
4. Pulsa **INSTALAR** en `phoenix_referee.lua` y en `phoenix.lua`. Deben quedar en **INSTALADO** (Link guarda copia de `sider.ini` con fecha).
5. Abre PES con su lanzador. En **SYNC**, la tarjeta «Qué funciona en tu juego» debe quedar en verde.

Sin Link (solo los módulos): doble clic en `PhoenixJuego-instalar.bat`, elige la carpeta del juego y espera **LISTO**.

### Qué trae esta beta
| Función | Dónde | Estado |
|---|---|---|
| Sala, mandos, puestos, equipos A/B | SALA, MANDOS | Lo usa Fralex; con otra persona, esta sesión lo comprueba |
| **JUGAR**: salas abiertas, amigos, tu perfil de PES | JUGAR | Nuevo. Perfil probado en el PES de Fralex; en tu PC, a comprobar |
| Lados automáticos (Link ve de qué lado juega cada mando) | PARTIDO | Nuevo, activado |
| Aviso cuando alguien entra a la sala | Ajustes › Sonidos | Nuevo, activado |
| Campo 2D en vivo | PARTIDO / web | Nuevo, **apagado**: se enciende en Partido |
| Marcador, goles, HUD y árbitro | PARTIDO | Listo, a probar contigo |
| Avisos de la web dentro del juego | SYNC | Listo |
| Fichajes de Phoenix | SYNC | Se aplican sobre **tu** base; la lista va vacía: no cambia nada |
| **Traspasos en vivo** | SYNC | **Apagado por defecto** («¿Qué eres tú?» = «Apagado»). Ya funciona con la web; enciéndelo solo si Fralex te lo pide |

### Quitar Phoenix
- Del juego: Link → SYNC → Módulos del juego → **DESINSTALAR**. Sin Link: ejecuta `PhoenixJuego-instalar.bat desinstalar` desde una consola. No borra nada: aparta los `.lua` y guarda copias de `sider.ini`.
- De Link: borra la carpeta `Phoenix Link` y el acceso directo. Tus ajustes quedan en `%APPDATA%\Trybuchet\Smash Soda`.

---

## Lista para la PRIMERA sesión con el amigo

**Antes de abrir sala (5 min)**
- [ ] Link abre y dice **BETA 2** abajo a la izquierda.
- [ ] INICIO (o Sala › Conexión) › Semáforo Phoenix: «Todo listo para jugar». Lo que falte sale con su botón para arreglarlo.
- [ ] SYNC › «Qué funciona en tu juego»: módulos al día, sin ámbar ni rojo.
- [ ] Abre PES: en `SiderAddons\sider.log` aparece `[phoenix.lua]` sin «is NOT activated».

**JUGAR y perfiles**
- [ ] JUGAR › Salas abiertas: carga (aunque no haya salas) y no da error.
- [ ] JUGAR › Perfil PES: pones tu nombre y opciones; al sentarte en la sala, PES los muestra en tu mando y los devuelve al levantarte.

**Sala y partido**
- [ ] Entra el amigo: suena y sale el aviso de entrada (sonido + notificación).
- [ ] Equipos A/B: cada uno en su equipo y su puesto; probad a cambiar de equipo arrastrando.
- [ ] Lados automáticos: tras el pitazo, Link pone a cada uno en su lado; si está al revés, se corrige solo.
- [ ] El marcador y los goles aparecen en Link y en el HUD del juego.
- [ ] Campo 2D: enciéndelo en Partido y mira que las fichas se mueven (puede tardar ~20 s en «BUSCANDO»).
- [ ] Un aviso de la web sale dentro del juego (barra espaciadora → panel de Sider).
- [ ] Mira si hay retraso en los mandos (medidor de Link).

**Si algo falla, anota**
1. Qué hacías y qué pasó (una frase) y la hora.
2. Una foto de la pantalla de Link.
3. El informe: **Sala › Conexión › Detalles técnicos del PC › COPIAR INFORME**, y pégalo en el chat. Trae versiones, parche, módulos y el final de `sider.log` con una sección «Qué pasa y qué hacer». Tapa tu nombre de usuario de Windows y no lleva contraseñas ni claves.

**Reglas de oro:** no abras PES mientras instalas módulos; no enciendas «Traspasos en vivo» salvo que Fralex te lo pida; si dudas, no cambies nada y manda el informe.

---

## Parte 6 · Prueba del modo online con un amigo (Phoenix Link + PES 2021 por Parsec)

> Origen: `docs/PRUEBA-ONLINE-CON-AMIGO.md` · último cambio 053c502 2026-10-19 · 4245 bytes. Copia íntegra.

> Actualizada al menú del plan UX (Link 0.3.34): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES. Mapa completo en `docs/GUIA-MENU-NUEVO.md`.

**Cómo funciona:** PES corre **solo en tu PC** (el anfitrión). Tu amigo **no instala PES ni nada de Phoenix**: solo Parsec. Ve tu pantalla y juega con su mando como si estuviera a tu lado.

---

### A) En tu PC (anfitrión) — antes de que se conecte

1. **Compilar Link** (si cambiaste algo o es la primera vez):
   - doble clic en `C:\dev\smash-soda-fork\_phoenix-link\COMPILAR_LINK.bat`;
   - tarda de 1 a 10 minutos y **no borra nada**;
   - al final dice `LISTO`.
   - **Nunca** uses `COMPILAR_PHOENIX.bat`: borra lo que no está guardado en git.
2. **Abrir** `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\PhoenixLink.exe`. Si Windows pregunta por el firewall, pulsa **Permitir**.
3. **Cuenta web:** si Link te enseña la bienvenida, sigue sus 3 pasos. Si no: SYNC › MI JUEGO › «Tu cuenta de la liga» › escribe el código de 6 cifras de tu perfil y pulsa **CONECTAR** (si no lo hiciste ya).
4. **(Opcional) Archivos de Phoenix en el juego:** SYNC › MI JUEGO › **ARREGLAR TODO**. Si PES está abierto, Link espera a que lo cierres y sigue solo.
5. **Abrir PES 2021** como siempre (con el switcher de ConmeGOL).
6. **Abrir la sala:** en **INICIO** pulsa **JUGAR CON UN AMIGO**: abre la sala, copia el enlace y te deja invitar a tus amigos de la web. Pega el enlace en WhatsApp o Discord. Si algo falta (por ejemplo ViGEmBus), te lo dice con el botón que lo arregla.

### B) En la PC de tu amigo (5 minutos)

1. Descargar **Parsec** desde https://parsec.app y crear una cuenta gratis.
2. Conectar su mando (Xbox, PlayStation o genérico) **antes** de entrar.
3. Abrir el enlace que le mandaste y esperar.
4. Nada más: no necesita PES, ni Phoenix Link, ni el parche.

### C) Cuando entre

1. Tu amigo pulsa un botón de su mando y **se sienta solo** en el primer puesto libre (sentado automático; se cambia en Mandos › Puestos o Ajustes › General). Si lo apagaste, verás su solicitud: ACEPTAR, o arrástralo a un puesto en Mandos › Puestos.
2. Con los lados automáticos no hay que tocar nada: el partido arranca solo al pitazo. A mano: Partido › Marcador › elige los dos lados › **¡EMPEZAR!**. Al terminar sale el resultado con **REVANCHA**.
3. En PES entra a Partido › Amistoso. Cada uno elige equipo con su mando.

### D) Qué probar (marca cada uno)

- [ ] **Conexión:** su ping en la portada de la sala (verde < 60 ms, naranja < 100, rojo más).
- [ ] **Su mando mueve su jugador** y la imagen va fluida.
- [ ] **Modo competitivo:** durante el partido tu amigo pulsa **Start**. **No** debe pausar. En Partido › Marcador debe salir «Start/Back/Guía bloqueados».
- [ ] **Marcador:**
  - con el módulo instalado, el marcador de Link sigue al del juego y cada gol sale en el chat con su minuto;
  - sin el módulo, lo llevas a mano con +GOL.
- [ ] **Chat:** escríbele desde Link y que te responda desde Parsec.
- [ ] **Final:** al terminar, el partido aparece en Partido › Historial.
- [ ] **(Opcional) Pausa automática:** actívala en Mandos › Bloqueo › Árbitro y pídele que se desconecte a propósito. Debe salir el aviso en el chat y pausarse el marcador (y el juego si el módulo está instalado).
- [ ] **(Opcional) Overlay PhoenixGlass:** Ajustes › Overlay › abrir. Muestra el ping, el chat y los mandos encima del juego (para transmitir).

### E) Si algo falla, mándame esto

- Qué estabas haciendo y qué viste (una captura ayuda mucho).
- Si falló el juego o un módulo: `D:\Frank\Games_\Conmegol Patch\SiderAddons\sider.log`.
- Si falló la conexión: el ping que marcaba y si tu internet es de fibra o de antena.

### Requisitos de red (importantes)

- **Tu subida:** ≥ 10 Mbps, mejor por **cable**. Link ajusta la calidad solo (ancho de banda automático).
- **CGNAT:** algunas compañías en Perú comparten tu IP con otros clientes y Parsec no logra conectar directo. Si tu amigo no puede entrar nunca, ese es el sospechoso: hay que pedir a tu compañía una IP pública.
- Cierra descargas y Steam en segundo plano durante la prueba.

---
