# Phoenix Link beta 2 (0.2.68) — notas de versión, publicación y estado

12 oct 2026. Misma idea que la beta 1 (que otra persona instale y use Link sin ayuda), con todo lo que se hizo después: JUGAR, perfiles de amigos, equipos A/B, lados automáticos, aviso de entrada, campo 2D, fichajes sobre la base de cada PC y traspasos en vivo (apagado). Guía para el amigo: `docs/BETA-2-GUIA-AMIGO.md`. Escenarios de Sync: `docs/SYNC-ESCENARIOS.md`. Regla de Fralex: «si algo se rompe, mejor no lo hagas».

## El paquete
- **Zip de release:** `PhoenixLink-0.2.68-beta2.zip` (6,2 MB), construido desde una **copia limpia** del commit `32f5018` (git worktree aparte, compilación propia; no se tocó el árbol de trabajo).
- **SHA256:** `DA03A6245896BFE50E08B27804547BE9D174F2ABA95994706BDF321B9482EA6C`.
- **Lleva:** `PhoenixLink.exe`, `parsec.dll`, `SDL2.dll`, `WebView2Loader.dll`, `nvEncodeAPI64.dll`, `ui\`, `sider\` (phoenix.lua v0.18b y phoenix_referee.lua v1.3), `fichajes\`, `fonts`, `icons`, `sfx`, `LICENCIAS.txt`. No lleva `.pdb`, `imgui.ini`, el overlay (se baja aparte) ni `NotoSansSC-VariableFont_wght.ttf` (17 MB; ningún código de Link lo carga: solo se usan Afacad Flux y Montserrat). Así el zip pesa 6,2 MB y entra en el límite de 10 MB de la herramienta de subida.
- **`parsec.dll`:** va dentro del zip porque Fralex confirmó (12 oct) que pidió permiso a Parsec y pagó las licencias para repartirlo. Si el permiso no cubre un release público, repartir el zip en privado y cambiar `PHX_ZIP_URL` en `PhoenixLink-instalar-rapido.bat`.
- **Para rehacerlo:** compilar en un worktree limpio y ejecutar `tools\EMPAQUETAR-RELEASE.ps1 -Release <x64\Release> -Salida <carpeta>`; luego poner el SHA256 y la URL en el `.bat` rápido.
- El zip está en `C:\dev\phoenix-beta2-dist\` (fuera del repositorio).

## Qué se probó de verdad (y qué no)
**Probado**
- Compilación **limpia** del commit `32f5018` (configurar + compilar desde cero en carpeta aparte): sin errores; el `.exe` del zip es la 0.2.68.
- `PhoenixLink-instalar-rapido.bat` contra el zip local (servidor de prueba): instala en carpeta temporal con todos los archivos, rechaza un zip con SHA malo sin instalar nada, y actualiza encima de una instalación existente. Overlay con SHA correcto.
- Escenarios de Sync (módulos, `sider.ini`, parches 26/27, vueltas atrás): `pruebas-link\correr_escenarios.bat` → **55 comprobaciones, 0 fallos**.
- Las pruebas de las otras sesiones constan en `REGISTRO-LINK.md` (perfiles probados con el PES real de Fralex, fichajes con copias de bases reales).
**Sin probar**
- Abrir el Link instalado desde el zip en una PC limpia (no hay VM ni Windows Sandbox en esta PC), y la instalación real de Visual C++, ViGEmBus y WebView2.
- Una sala con un amigo de verdad (mandos, lados automáticos, equipos A/B, campo 2D, aviso de entrada).
- Fichajes y traspasos dentro del PES real; perfiles en el PC del amigo.

## Pasos para Fralex (publicar; son dos releases, sin `gh`, todo en la web)
1. **Zip de Link.** Abre https://github.com/Fralexito/smash-soda-fork/releases/new
   - *Choose a tag*: escribe `beta-2` y pulsa «Create new tag: beta-2 on publish».
   - *Target*: elige la rama `rediseño-phoenix-portal` (o el commit `32f5018`).
   - *Title*: `Phoenix Link beta 2 (0.2.68)`.
   - Arrastra `C:\dev\phoenix-beta2-dist\PhoenixLink-0.2.68-beta2.zip` a «Attach binaries».
   - Marca **Set as a pre-release** y pulsa **Publish release**.
2. **Overlay (en zip, 6,6 MB, para caber en el límite de subida).** Abre https://github.com/Fralexito/phoenix-overlay/releases/new
   - Tag nuevo `v1.0.0-beta1`, título `PhoenixGlass 1.0.0 beta 1`.
   - Arrastra `C:\dev\phoenix-beta2-dist\PhoenixGlass-1.0.0-beta1.zip` (SHA256 `A83455807EA145E8ACCF77F81A7A5ABA5A47501AE981E988F68131A3C4A632E5`; dentro va el `PhoenixGlass.exe` con SHA256 `DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94`), pre-release, **Publish release**.
3. Avísame (o a la sesión que te lo pidió): comprobaré con `curl` que ambos enlaces responden 200 y que los SHA256 coinciden. Los enlaces que espera el instalador son:
   - `https://github.com/Fralexito/smash-soda-fork/releases/download/beta-2/PhoenixLink-0.2.68-beta2.zip`
   - `https://github.com/Fralexito/phoenix-overlay/releases/download/v1.0.0-beta1/PhoenixGlass-1.0.0-beta1.zip`
4. Pásale al amigo `PhoenixLink-instalar-rapido.bat` y `PhoenixJuego-instalar.bat` (como archivos, no copiados de la web) y `docs/BETA-2-GUIA-AMIGO.md`.

## Riesgos conocidos
- **Hasta publicar los dos releases, el instalador rápido da 404** y se detiene con un mensaje claro (sin instalar nada).
- El zip es una versión fija: cada cambio de Link que el amigo deba recibir exige regenerar el zip y su SHA256 en el `.bat`.
- `parsec.dll` en un release público: confirmar que el permiso de Parsec lo cubre.
- Interfaz y funciones nuevas (JUGAR, perfiles, campo 2D, lados automáticos) se probaron sobre todo en el PC de Fralex; en otro PC o parche pueden fallar. El informe (Sala › Conexión › Diagnóstico del PC › COPIAR INFORME) sirve para depurar.
- **Traspasos en vivo** está apagado a propósito: no se prueba con el amigo todavía.
- Los `.bat` fijan el SHA256 de `phoenix.lua` y `phoenix_referee.lua`: si se editan, hay que actualizar el `.bat`.
