# Bitácora del frente LINK (solo se AÑADE; nunca editar ni borrar)
Formato: `AAAA-MM-DD HH:MM (Lima) | Cuenta A/B | LINK | Qué cambió | Archivos/commit | HECHO / A MEDIAS / PENDIENTE | Siguiente paso`
Protocolo: `COORDINACION.md` → «Dos cuentas de Claude».

---
## 2026-10-08 08:45 (Lima) · Cuenta A (chat WEB) · VOLCADO INICIAL reconstruido desde git y las notas del proyecto
⚠️ Escrito por el chat WEB, que NO trabajó este frente: se basa en el historial de git y en la memoria del proyecto. **El chat LINK debe revisarlo y añadir debajo lo que falte** (lo que está a medias, lo que no compila, lo que Fralex pidió en ese chat).

**Ramas:** `master` (base del autor + correcciones de compilación del 7 oct) · `experimental-html` (pantallas Sala/Opciones/Actividad estilo prototipo HTML, «Phoenix Portal») · **`rediseño-phoenix-portal` = rama de trabajo de LINK** · `mercado-fase0` = frente MERCADO. Tag estable mencionado por Fralex: `v1.0-base-estable` (no está en el remoto; puede ser solo local).
**Últimos commits de LINK:** 796d5da build en worktree `_phoenix-link` · 55abab2 pantalla Gente (lista + ficha: VIP, mod, teclado, ratón, quitar mando, expulsar, banear) · d712bc8 COORDINACION.md · 5999b0a / f68e517 renombre total a **Phoenix Link** (`PhoenixLink.exe`) · c1b928a Sala con degradado y acentos.
**Decisiones de Fralex:** la app se llama «Phoenix Link» en todos lados (logo nuevo pendiente de él); identidad Galaxy cian+púrpura, nada de Soda Arcade; estructura Sala · Partido · Mandos · Comunidad · Ajustes; español por defecto + idiomas principales; máx. 8 mandos; acercar la interfaz ImGui al prototipo HTML pantalla por pantalla (no incrustar HTML); siempre reversible; Windows 10 y 11; solo funciona con su web; instalador solo para hosts verificados; host neutral y retraso artificial solo editable por staff en oficiales; Phoenix Mercado se integrará dentro de Phoenix Link (opción de segundo código solo para manager).
**Contrato con la web:** API `/phoenix` (`claude/contrato-v1.md`). La web ya devuelve `version_liga` en `/presencia` y `/sala/latido` (para que Link avise a Mercado de cambios de Liga Máster).
**No verificado:** estado de compilación actual de la rama; pantallas pendientes del rediseño; si existe el tag `v1.0-base-estable`.
---
2026-10-08 08:45 (Lima) | Cuenta A | LINK | Protocolo de dos cuentas añadido (COORDINACION.md, CLAUDE.md, esta bitácora) | COORDINACION.md, CLAUDE.md, REGISTRO-LINK.md | HECHO | Chat LINK: completar el volcado con lo que solo él sabe

---
## 2026-10-08 08:50 (Lima) · Cuenta ? (chat LINK, el que hizo el trabajo) · COMPLEMENTO al volcado inicial
Lo que solo sabe este chat. Corrige/completa el bloque anterior.

**Compilación (estado real)**
- NUNCA se ha visto compilada y funcionando la rama `rediseño-phoenix-portal` con Sala v2 + Gente nueva en el PC de Fralex. Existe `PhoenixLink.exe` (02:42 del 8 oct) en `C:\dev\smash-soda-fork\x64\Release\`, pero es de un build previo al commit de Gente.
- Causa raíz de los «FALLO»/«sigue todo igual»: la carpeta `C:\dev\smash-soda-fork` quedó en la rama `mercado-fase0` (la movió el chat MERCADO) y el `.bat` hacía `git pull` ahí: compilaba el Shell viejo (error C2664 en `Shell.cpp(475)`: `TecladoVisual::render` con `Config::Keyboard`), no mi rama. Mi `Shell.cpp` usa `(void*)&Config::cfg.keyboard` y no tiene ese error.
- Arreglo: `COMPILAR_PHOENIX.bat` (NO versionado, vive solo en la carpeta principal del PC) reescrito el 8 oct: `git fetch`, toma el SHA de `refs/remotes/origin/redise*` (evita la ñ), crea/actualiza un worktree desacoplado en `_phoenix-link\` y compila ahí (`cmake -G "Visual Studio 18 2026"`, `--build ... --config Release --parallel`), log en `log_phoenix.txt`, salida `_phoenix-link\x64\Release\PhoenixLink.exe`. **El .bat nuevo todavía no se ha ejecutado** (riesgos: `git worktree add` primera vez, `for /f` con `^(`, configurar CMake desde cero). Si falla, pedir `log_phoenix.txt`.
- Verificación local previa a cada commit: la nube es Linux y no puede compilar. Solo `g++ -fsyntax-only` con cabeceras falsas (`scratchpad/stub`): `PantallaGente.cpp`, `Shell.cpp`, `ProveedorSalaSoda.cpp`, `PantallaAjustes.cpp`, `PantallaBiblioteca.cpp`, `TableroMandos.cpp` salen limpios. `I18n.cpp` (MTY_ReadFile), `PantallaSala.cpp` (strncpy_s/_TRUNCATE) y `TecladoVisual.cpp` (KEY_A) solo fallan por no existir Windows/matoya aquí, no por el código nuevo.

**Hecho en este chat (orden):** módulo 1 Mandos/Ajustes rápidos (cea8260) → Sala con pestañas Resumen/Opciones/Actividad + enlace host↔invitado + paneles latencia/transmisión (ce84f5c, 9201f7b, 42a00af, c1b928a) → renombre total a Phoenix Link (f68e517, 5999b0a) → COORDINACION.md → Gente lista+ficha con VIP/mod/teclado/ratón/quitar mando/expulsar/banear y 9 métodos nuevos en `ProveedorSala` (55abab2) → build en worktree (796d5da).

**A medias / pendiente (en este orden sugerido)**
1. Que Fralex ejecute el `.bat` nuevo y diga «EXITO»/«FALLO»; pedirle captura de Sala→Resumen y de Gente con alguien en sala. Sin eso no apilar más pantallas.
2. Ajustes: solo «Rápido» y «Biblioteca» son Phoenix; General, Video, Audio, Overlay y Avanzado siguen acoplando los paneles originales de Soda (`p.general`, `p.video`…). Prototipo HTML v3 propone: General, Chat y bot, Permisos, Video, Audio, Overlay, SFX, Avanzado, Cuenta. Mantener 100% de las opciones de Soda.
3. Gente→Moderación sigue acoplando `p.invitados` (GuestListWidget original); la ficha nueva no cubre listas de baneados/mods/VIP/historial.
4. «Partido» no existe como sección en `Shell.cpp` (`enum Seccion { SALA, MANDOS, GENTE, AJUSTES }`).
5. Logo: Fralex lo pasará después; pedir PNG cuadrado ≥512×512 → `SmashSoda/icon.ico` (+ `icon1.ico`, `icon2.ico`; referenciados desde `appicon.rc` y `Resource.rc`).
6. Prototipo HTML (en la sesión, `phoenix-portal-v3.html`): «Puestos» aún muestra 4 mandos con contadores hasta 32; debe pasar al límite de 8.
7. Posibles bugs del código original a avisar (no tocados): «Disable !bonk» invertido, toggle del puerto WebSocket, duplicados al editar en Biblioteca.
8. Limpieza opcional: `COMPILAR.bat`, `RUN_BUILD.bat`, `RUN_COMPILE.vbs` (rutas viejas), `SmashSoda.sln` obsoleto (no usar; la build real es CMake).

**Dejado igual a propósito (no «corregir»)**: User-Agent `PhoenixSoda/1.0` en `link/Http.cpp`, carpeta de ajustes `%APPDATA%\Trybuchet\Smash Soda`, `updater.bat`, cadenas originales en `I18nOriginal.cpp`. Etiqueta DPAPI del token cambió a «PhoenixLink» (no afecta a lectura).

**Pedido por Fralex en este chat:** que se vea idéntico al HTML aprobado (v3: https://claude.ai/artifact/Co2ryf61vzSc5yYyLS3ZiL), elegante y tecnológico; seguir con ImGui y probar otras tecnologías (RmlUi/WebView2) más adelante en `experimental-html` y decidir por convicción; que corra perfecto en Windows 10 y 11; «necesito que ya no hayan tantos errores»; tag/rama de respaldo `v1.0-base-estable` (no está en el remoto); informar al otro chat cada cambio que le afecte.

**Trucos de entorno de este chat:** archivos con CRLF + BOM (editar con Python respetando `\r\n`, no con sed); `.git/index.lock` en el PC no se puede borrar desde el puente (moverlo a `stale-index-lock`); tras cada push, `git fetch origin "refs/heads/rediseño-phoenix-portal:refs/remotes/origin/rediseño-phoenix-portal"` para que el hook de la nube vea el commit empujado; la nube no compila.
---
2026-10-08 08:50 (Lima) | Cuenta ? (chat LINK) | LINK | Volcado completado con estado real de compilación, causa raíz de «FALLO» (carpeta principal en mercado-fase0) y pendientes | REGISTRO-LINK.md | HECHO | Fralex ejecuta COMPILAR_PHOENIX.bat nuevo; si EXITO, Ajustes (General/Video/Audio/Overlay/Avanzado) en Phoenix
2026-10-08 09:00 (Lima) | Cuenta ? (chat LINK) | LINK | Traspaso sin pérdida: prototipo HTML v3 y capturas copiados al repo; copia versionada del .bat; sección «Retomar» en CLAUDE.md | docs/prototipo/*, tools/COMPILAR_PHOENIX.bat, CLAUDE.md | HECHO | Fralex ejecuta el .bat; luego Ajustes
