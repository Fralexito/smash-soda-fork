# Phoenix Link beta 1 (0.2.44) — notas de versión y estado

12 oct 2026. Primera base pensada para que **otra persona** instale y use Link sin ayuda y para que Sync funcione sin romper nada.
Guía para el amigo: `docs/BETA-1-GUIA-AMIGO.md`. Regla de Fralex: «si algo se rompe, mejor no lo hagas».

## Qué trae
- **Instalador de Link** (`PhoenixLink-instalar.bat`) en español: comprueba Windows 64 bits, Link cerrado, 12 GB libres e internet **antes** de instalar nada; no reinstala ViGEmBus ni WebView2 si ya están; baja el overlay **PhoenixGlass** con sha256 (ya no el Smash Glass viejo); comprueba al final que están todos los archivos; deja un registro en `%TEMP%\PhoenixLink-instalador.log`. Se puede repetir (actualiza sin tocar ajustes). Si falla, un mensaje claro y nada del juego tocado.
- **Instalador de módulos sin Link** (`PhoenixJuego-instalar.bat`): instala `phoenix.lua` v0.18b **y** `phoenix_referee.lua` v1.3; **desinstala** con `desinstalar`; aplica la regla «sin base reconocida no se pone Phoenix-DB» (igual que Link); copia de `sider.ini` con sello, vuelta atrás si algo falla a medias, versión fija por commit.
- **Sync › Puente**: tarjeta «Qué funciona en tu juego» (juego, módulos, avisos, fichajes) con el siguiente paso en cada caso; detección de módulos **perdidos tras actualizar el parche** («REINSTALAR»), **desactualizados** («ACTUALIZAR») o que Sider no pudo cargar; fichajes marcados «desactivados en este parche (próximamente)» cuando el `sider.ini` no tiene base reconocida (hoy, el parche 27).
- **Carpeta del juego a mano** (Módulos del juego): si Link no ha visto el juego, se pega la ruta y se valida; además Link recuerda la carpeta en cuanto ve PES abierto.
- **COPIAR INFORME** (Sala › Conexión › Diagnóstico del PC): versiones, parche, módulos, sha256 cortos y final de `sider.log`, para depurar sin acceso al PC.
- Preferencias de Link (`phoenix-ui.json`) con guardado atómico y copia `.bak`; si el archivo se daña, se lee la copia.
- Etiqueta **BETA 1** en la interfaz y en el informe.

## Qué se probó de verdad (y qué no)
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

## Qué queda fuera de la beta (desactivado o «próximamente»)
- **Fichajes de Phoenix en el parche 27.** Solo 2 de 175 fichajes de Fralex encajan en la base nueva y Phoenix-DB viene del parche 26: no se pone sin base reconocida. Plan: guardar fichajes como **cambios** sobre la base del parche activo (`docs/FICHAJES-COMO-CAMBIOS.md`).
- Orden de mandos en `settings.dat` (experimental, apagado por defecto).

## Pasos para Fralex
1. Cierra Link si está abierto y abre la versión 0.2.44 (ya compilada en `x64\Release`).
2. **Overlay para el amigo:** en GitHub → `Fralexito/phoenix-overlay` → Releases → *Draft a new release*: etiqueta `v1.0.0-beta1`, adjunta `C:\dev\phoenix-overlay\bin\PhoenixGlass.exe` (sha256 `DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94`) y publica. Sin esto el instalador sigue bien, pero sin overlay.
3. Si apruebas, ponle la etiqueta **`beta-1`** al commit de la beta (`git tag beta-1 <commit>` y `git push origin beta-1`). No se ha creado ninguna.
4. Pásale al amigo `PhoenixLink-instalar.bat`, `PhoenixJuego-instalar.bat` y la guía.

## Riesgos conocidos
- El instalador **compila** en la PC del amigo (30–90 min, ~12 GB): la licencia del SDK de Parsec no permite repartir el programa compilado. Una versión más rápida sin compilar exigiría permiso de Parsec.
- El repositorio público ya incluye `Dependencies\parsecsdk` (con `parsec.dll`): es decisión de Fralex si lo deja así.
- Si Visual Studio cambia el enlace `aka.ms/vs/18/...` o winget falla, el instalador puede fallar en esa parte; el registro dice dónde.
- El instalador fija un commit. Cada cambio de Link que el amigo deba recibir exige subir ese commit en el `.bat`.
- Los `.bat` fijan el sha256 de `phoenix.lua` y `phoenix_referee.lua`: si se editan, hay que actualizar el `.bat` o dirá «no es la versión esperada» (y no tocará nada).
- Otro parche de ConmeGOL puede mover sus carpetas o reescribir `sider.ini`: Link lo muestra como «perdido» y se arregla con REINSTALAR.
