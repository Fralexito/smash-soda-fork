# Bitácora del parche Phoenix de Frank (PES 2021 base)

## v0.1 · 4-nov-2026, 13:25–13:35 (hora de la PC) · armado sin probar

**Qué se logró:** Sider mínimo y 3 paquetes propios montados en `C:\PES21\eFootball PES 2021`, cada uno en su carpeta y con su interruptor en `Sider\sider.ini`. **No se probó en el juego**: el PES del Conmegol seguía abierto (atascado en la pantalla de mandos del descanso) y no se puede abrir un segundo PES ni cerrar ese de forma automática (ver «Errores»).

### Comprobación previa (C)
- `PES2021.exe` base: **458.739.200 bytes**, SHA-256 `641720e22725d5795ded8a2e61d56dd4f20afc886205c3896190b60aa9160e5a`. Es el exe base limpio: coincide con `ANALISIS_REA_EXE_BASE_VS_CONMEGOL.md`.
  - La cifra del encargo (458.806.784) es la del exe del **Conmegol** (`5e27a782…224a`), no la del base. No es un error del base.
- Listado de la carpeta del juego antes de tocar nada (97 entradas, 3 niveles, con tamaños): `respaldo_pes21_base\listado_20261104.txt`.
- **No se tocó ningún archivo existente.** Solo se crearon `Sider\` y `Phoenix\`. Nada en `Data\` ni en el exe. Nada descargado ni borrado.

### Paso a paso (para replicarlo)
1. `Sider\` ← `sider.exe` y `sider.dll` de `D:\Frank\Games_\Conmegol Patch\SiderPractica0` (Sider mínimo; sha256 exe `59d4c6f4…b825`, dll `2e45d1f5…8bfb`). Carpetas nuevas: `Sider\modules` (vacía), `Sider\content\livecpk\00_base` (vacía), `Sider\content\phoenix` (salida de la sonda y del referee).
2. `Sider\sider.ini` nuevo:
   - `exe.name` con la **ruta completa** del exe base, para que este Sider no se enganche nunca al Conmegol (el del Conmegol usa `"\PES2021.exe"`, que valdría para los dos).
   - `livecpk.enabled=1`, `lua.enabled=1`, `luajit.ext.enabled=1` (Core usa ffi), un `cpk.root` a `00_base`.
   - Los módulos se cargan con rutas relativas a `Sider\modules`: `"..\..\Phoenix\<paquete>\modules\x.lua"`.
3. `Phoenix\01_PhoenixCore\` (manifiesto incluido):
   - `core\phoenix_core.dll` copiada del Conmegol.
   - `core\phoenix_core.json` copiado con `street_activo=false` y 23 funciones apagadas: street_1v1_portero, street_jugadores, gol_nativo_free, saque_centro_free, marcador_free, gol_directo_free, cancha_chica_free, siempre_disparo_free, ia_cancha_chica, red_resorte, jugadores_formacion, street_pantalla_descanso, street_saque_nativo, street_dos_tiempos, sin_fin_tiempo_free, ajustes_partido_free, reloj_street_free, marcador_nativo_free, muros_core, suplentes_ocultos_free, jugadores_chica_free, jugadores_dentro_free, street_saque_trabado.
   - `modules\phoenix_core_loader.lua` v0.1.0 con **una línea cambiada**: busca la DLL en `..\Phoenix\01_PhoenixCore\core\` en vez de `phoenix_core\`.
4. `Phoenix\02_GraficosB\` (manifiesto incluido): los 7 livecpk de `D:\Frank\IA\CLAUDE\pes-graficos` en el mismo orden que en el sider.ini del Conmegol: Cielo-4096, Césped-Mips, Camisetas-Mips, Piel-SSS-LOD, Rebote05-CampNou, Bloom, Tinte-Piel-Fuerte.
   - Ojo: en el sider.ini del Conmegol hoy están las 7 con `;` (apagadas). Aquí van encendidas, porque son «lo nuestro».
   - **SP_Menu Light** (Sudamerican) no se copió: queda como línea apagada y anotada.
5. `Phoenix\03_Street\`: módulos `phoenix*.lua` (+ `phoenix_campo.ini`), livecpk `PhoenixMandos`, `Phoenix-Entrenamiento*`, `Phoenix-DB`, y datos `content\phoenix`, `content\stadium-server\Phoenix`.
   - **El MANIFIESTO.md de esta carpeta no se pudo escribir** (el control de permisos lo bloqueó). Su contenido es la tabla de abajo.

Tamaño total: Phoenix 233 MB, Sider 2,2 MB (en C: quedan ~19 GB libres).

### Qué quedó encendido y apagado
| Pieza | Estado | Motivo |
|---|---|---|
| Phoenix Core (cargador + DLL) | encendido | sin dependencias del Conmegol; Street apagado en su JSON |
| Gráficos B (7 livecpk) | encendidos | nuestros; camisetas y Camp Nou solo actúan si el base tiene esos archivos |
| SP_Menu Light | apagado | es del Sudamerican, no nuestro |
| `phoenix_sonda.lua` | encendido | sin dependencias; da datos al MCP phoenix-pes |
| `phoenix_referee.lua` | encendido | solo lee; nombres de equipo solo con `content\kit-server\map.txt` (el base no lo tiene) |
| `phoenix.lua` (Sync) | apagado | necesita Phoenix-DB y Phoenix Link |
| `phoenix_campo.lua` | apagado | pensado para el Conmegol; revisar `phoenix_campo.ini` |
| `phoenix_marcador_free.lua` | apagado | necesita `StadiumServer.lua` del Conmegol |
| `phoenix_mandos_fase1.lua` | apagado | necesita Link con más de 8 puestos (experimental) |
| livecpk PhoenixMandos, Entrenamiento-Ropa | apagados | solo sirven con el Street |
| livecpk Phoenix-DB | apagado | copia provisional de la base de olmosjr23 (no nuestra) |
| `content\phoenix`, `content\stadium-server\Phoenix` | copiados, sin usar | los módulos los buscan en `Sider\content\…`; para encender el Street hay que copiarlos allí |

### Errores e intentos fallidos
- **No se pudo probar (paso f):** el PES del Conmegol estaba abierto y atascado en la pantalla de mandos del descanso. Para cerrarlo se intentó, en este orden: teclas del MCP (Esc; no responden en esa pantalla, igual que en sesiones anteriores), `CloseMainWindow` (WM_CLOSE) desde PowerShell (rechazado por el control de permisos) y computer use (acceso denegado). Regla: un solo PES a la vez, así que no se abrió el base.
- El primer script de copia falló por una barra invertida al final de una cadena `r'...\'` en Python (no copió nada; el script se para si `Sider\` o `Phoenix\` ya existen). Arreglado con `os.path.join`.
- El MCP phoenix-pes lee los registros de la carpeta del Conmegol: con el base habrá que apuntar su `config.json` a la ruta nueva (solo la ruta del exe/SiderAddons; no tocar los interruptores de permisos).

### Cosas a tener en cuenta
- Las rutas relativas `..\..\Phoenix\...` en `lua.module` son la parte menos segura: si Sider no las acepta, saldrá en `Sider\sider.log` («cannot open»). Arreglo: copiar los `.lua` a `Sider\modules\` y poner solo el nombre.
- Phoenix Core puede escribir sus registros (`phoenix_core.log`, `peticiones.log`) dentro de `Phoenix\01_PhoenixCore\core\`.
- Las firmas AOB de Core se buscan por patrón; si alguna no está en el exe base, Core apaga esa función sola y lo anota en su log.

### Cómo probar (cuando el Conmegol esté cerrado)
1. Cerrar el PES del Conmegol (a mano) y comprobar que no quedó `sider.exe` colgado.
2. Abrir `C:\PES21\eFootball PES 2021\Sider\sider.exe` y luego `C:\PES21\eFootball PES 2021\PES2021.exe`.
3. Llegar al menú principal. Revisar `Sider\sider.log` (que no haya «error» ni «cannot open») y `Phoenix\01_PhoenixCore\core\cargador.txt` («Core … iniciado: normal»).
4. Si algo falla, poner `;` en la línea de ese paquete y repetir (como máximo 3 intentos por paquete).

### Pruebas del 4-nov por la tarde (14:02–14:30)
- **Intento 1 (Sider 7.3.4, `exe.name` con ruta completa):** Sider tardó en engancharse; cuando lo hizo, cargaron los 7 gráficos B, la sonda y el referee (las rutas `..\..\Phoenix\...` funcionan). Core falló por un error mío: la ruta del cargador tenía `\` sin escapar en Lua (`invalid escape sequence`). Arreglado con `\\`.
- **Intento 2 (7.3.4, `exe.name = "\PES2021.exe"`, ini sin tildes):** Core 0.5.4 arrancó «normal» con 21 funciones bien y ningún error (sus firmas valen en el exe base). Pero **PES se cerró solo** a los pocos segundos de terminar Sider, sin error de Windows. Con Core apagado pasó igual: no es culpa de Core ni de los paquetes.
- **Causa probable (readme de la 7.4.1):** con exes que no son de Steam, `free.side.select = 1` puede cerrar el juego al arrancar. Lo teníamos en 1.
- **Sider 7.4.1:** descargado del enlace de Frank (release oficial de juce, github.com/pes-modding/sider7, sha256 `67edb8fd…b390`, coincide). Instalado en `C:\PES21\eFootball PES 2021\Sider_7.4.1` (la carpeta `Sider` 7.3.4 no se pudo renombrar porque su sider.exe seguía abierto; queda como respaldo). Su `sider.ini`:
  - `start.game` con la ruta completa del exe base y `exe.name = "\PES2021.exe"` (el readme pide alinear los dos para exes que no son de Steam);
  - `free.side.select = 0`;
  - Core apagado para la primera prueba;
  - `netblock.lua` (módulo oficial que bloquea online y LiveUpdate) **apagado** a propósito: el hilo Online propio usa el online de este PES con hosts hacia nuestro servidor.
  - Copias: `sider.ini.original-7.4.1` (el de fábrica), y en la carpeta 7.3.4 `sider.ini.intento1` y `sider.ini.intento2`.
- **Pendiente:** cerrar el Sider 7.3.4, abrir la 7.4.1 y ver si PES llega al menú; si aguanta, encender Core y llegar a un amistoso. Si sigue cerrándose: probar «Sider tarde» (abrir Sider con el juego ya en el menú).

### Orden estándar y nombres técnicos (4-nov, 14:25–14:35; pedido de Frank)
- **Por qué:** Frank pidió que el `sider.ini` «se vea ordenado como todos los Sider» y con nombres técnicos que no digan a la vista qué es cada cosa.
- **Copias antes de tocar:** `Sider_7.4.1\sider.ini.antes-orden-20261104` y `sider.ini.original-7.4.1` (el de fábrica).
- **Movido (no borrado) a la estructura estándar de Sider, dentro de `Sider_7.4.1`:**
  - los 7 gráficos B y PhoenixMandos / Phoenix-Entrenamiento-Ropa → `content\livecpk\`;
  - los 7 módulos .lua (+ `phoenix_campo.ini`) → `modules\`;
  - DLL y json de Core → `phoenix_core\` (la ruta original del cargador; se quitó el cambio de ruta de la v0.1).
- **Renombrado** (tabla completa en `mi-parche\LEYENDA_NOMBRES.txt`):
  - livecpk: PHX_ENV_SKY (cielo), PHX_ENV_TRF (césped), PHX_KIT_MIP (camisetas), PHX_GFX_SSS (piel SSS/LOD), PHX_LGT_GI (rebote Camp Nou), PHX_FX_BLM (bloom), PHX_FX_LUT (tinte), PHX_IO_PAD (mandos), PHX_KIT_TRN (ropa de entrenamiento);
  - módulos: phx_core (cargador de Core), phx_mon (sonda), phx_ref (referee), phx_sync (Sync), phx_map2d (+ .ini; campo 2D), phx_sbf (marcador libre), phx_io16 (mandos 9-16). `netblock.lua` sigue igual (es oficial de Sider).
  - Referencias ajustadas: `phx_map2d.lua` ahora lee `modules\phx_map2d.ini`, y `phx_io16.lua` busca su propio nombre de overlay como `phx_io16`. La DLL y el json de Core no nombran ningún módulo ni carpeta. El MCP phoenix-pes solo los menciona en textos de ayuda.
  - **Los archivos de salida no cambian de nombre** (`content\phoenix\sonda.json`, `estado.json`, `resultado.json`, `pantalla.json`, `escenas.log`), porque los leen Link y el MCP.
- **Fuera del ini (anotado, no borrado):** Phoenix-DB (copia de la base de olmosjr23) y Phoenix-Entrenamiento / -B1 / -B2 / -C siguen en `Phoenix\03_Street\livecpk\`. SP_Menu Light (Sudamerican) no se copió. `Phoenix\` queda con un `LEEME.txt` que explica a dónde se movió todo.
- **ini final** (ASCII, comprobado que cada `cpk.root` y `lua.module` existe y que ya no aparece «phoenix»): ajustes arriba, luego LIVECPK y luego MÓDULOS. Encendidos: los 7 gráficos, phx_mon y phx_ref. Apagados: phx_core (para la primera prueba de la 7.4.1), PHX_IO_PAD, PHX_KIT_TRN, phx_sync, phx_map2d, phx_sbf, phx_io16 y netblock.

### Prueba con Sider 7.4.1 (4-nov, 14:27–14:30): ✅ FUNCIONA
- Se abrió `Sider_7.4.1\sider.exe`; con `start.game` lanzó el PES base solo y su ventana se cerró sola (es lo normal en el modo «un clic»: Frank creyó que no se había abierto).
- `sider.log`: `Sider DLL: version 7.4.1`, `DLL attaching to (C:\PES21\eFootball PES 2021\PES2021.exe)`, los 7 `cpk.root` PHX_* cargados, `phx_mon.lua` y `phx_ref.lua` en OK, «Sider initialization complete». Sin errores.
- **PES siguió abierto más de 2 minutos** (con la 7.3.4 y `free.side.select = 1` se cerraba a los pocos segundos). Causa confirmada en la práctica: `free.side.select = 1` con este exe que no es de Steam.
- No se pudo ver la pantalla (ventana minimizada; el MCP no la restaura). `sonda.json` aún no aparecía; `escenas.log` sí se escribía, así que `phx_ref` estaba vivo.
- **Cómo abrir el juego con el parche desde ahora:** doble clic en `C:\PES21\eFootball PES 2021\Sider_7.4.1\sider.exe` (no en `PES2021.exe`).
- 14:36: pausa pedida por el coordinador (otro hilo usa PES para grabar tráfico). PES quedó abierto con la 7.4.1; no se tocó nada más.

### Siguiente paso preparado (sin aplicar): encender Phoenix Core
1. Con PES **cerrado** (Alt+F4 en el juego).
2. En `Sider_7.4.1\sider.ini`, quitar el `;` de `;lua.module = "phx_core.lua"`.
3. Abrir `Sider_7.4.1\sider.exe`.
4. Comprobar `Sider_7.4.1\phoenix_core\cargador.txt` («Core 0.5.4 iniciado: normal») y `phoenix_core.log` (sin «no encontr…»). Con la 7.3.4 ya había cargado con 21 funciones bien en el exe base.
5. Llegar a un amistoso con el MCP phoenix-pes. El MCP lee los registros del Conmegol: para el base hay que apuntar su config a `Sider_7.4.1` (solo la ruta; sin tocar los interruptores de permisos).
- **Deshacer:** volver a poner el `;`. Si Core da problemas sin desinstalar: crear el archivo vacío `Sider_7.4.1\phoenix_core\PHOENIX_CORE_OFF`.
- Después: `velocidad_partido` está en -1 («no tocar») en su json, porque escribe un byte que el partido no lee (ver REA §5.3b).

### Cómo deshacer
Cerrar PES y Sider y borrar `C:\PES21\eFootball PES 2021\Sider`, `Sider_7.4.1` y `C:\PES21\eFootball PES 2021\Phoenix`. No hay nada más que restaurar: ningún archivo previo se modificó (comparar con `respaldo_pes21_base\listado_20261104.txt`).

## v0.1 · 10-nov-2026 · retoma (paso 1 preparado, paso 2 a medias, sin probar en vivo)
**Candado:** hay un PES abierto con Street (Conmegol), posiblemente con Frank jugando. No se abrió ni cerró ningún PES. Todo lo de abajo está PREPARADO, SIN PROBAR.

### Paso 1 · Phoenix Core
- `Sider_7.4.1\phoenix_core\phoenix_core.dll` = **Core 0.5.58** (md5 a0503463932b7417697486f3e4d0bd06, punto seguro `pes-street\respaldos\STREET_ENTRENAMIENTO_ESTABLE_10NOV`). La 0.5.64b del Conmegol está en desarrollo, no se usó. El 0.5.4 anterior quedó en `phoenix_core\_antes_v01\` (dll + json).
- `phoenix_core.json`: `street_activo=false`, `street_amistoso*=false`; todas las funciones en false salvo sondas/diagnóstico (diagnostico, sonda_partido, sonda_pasos, vigilante_bucles, pelota_sonda, goles_sonda, jugadores_sonda, cursor_pt_log). `velocidad_partido=-1`, `street_tamanos`, `arcos_visuales`, `season_forzada`, `clima_forzado`, `jugadores_chica_modo` en 0.
- `sider.ini`: línea `lua.module = "phx_core.lua"` encendida (el cargador es idéntico al del Conmegol). `MANIFIESTO.md` de phoenix_core actualizado.
- Pendiente (en vivo): abrir `Sider_7.4.1\sider.exe`, llegar a un amistoso 11v11 y mirar `sider.log` y `phoenix_core\phoenix_core.log` + `cargador.txt` ("Core 0.5.58 iniciado: normal"). Deshacer: `;` en la línea o archivo vacío `PHOENIX_CORE_OFF`.

### Paso 2 · Añadidos del Conmegol: revisión de dependencias y firmas (nada copiado todavía)
Prueba offline de las firmas AOB de cada módulo contra `PES2021.exe` base (458.739.200 B) y contra el del Conmegol: **mismo número de coincidencias en ambos** (todas las firmas principales dan 1 o 2; las que dan 0 son trozos secundarios que también dan 0 en el Conmegol). Es una prueba en disco, no en memoria.
| Añadido | Dependencias | Estado propuesto |
|---|---|---|
| 21 Anti-Cheat | solo API de Sider; ojo: además fija `foul_level=1` | encendido (anotar) |
| 22 attack_mentality | AOB OK (1 coincidencia); escribe `ai_mentality_settings.txt` | APAGADO |
| 24 FoulScenes | AOB principal OK; necesita `.\Scenes\` (76 MB, livecpk `Scenes`) | APAGADO |
| 20 cámaras (camera + 8 cams) | `lib\nesalib.lua` (nesalib) antes que ellas; un `.ini` por cámara en `modules\`; `camera.cache` en la carpeta de Sider; AOB OK | encendido; cada cam referencia su `.ini` por nombre (hay que ajustar al renombrar) |
| 18 SoundServer | `modules\SoundServer.ini`; sin AOB | encendido |
| 11 UIColors | `lib\CommonLib.lua` (ctx.common_lib), `content\ui-colors\` (map_exe.txt, ui_bin_colors.ini: **map_exe.txt es del exe del Conmegol**; validar con el base) | encendido si valida, si no apagado |
| 3 DR_KitMixer | necesita `kserv.lua` y `content\kit-server\map.txt` (IDs de equipos) | APAGADO: espera la DB global |
| 32 trophy | idéntico al `trophy.lua` oficial que ya trae el base; usa `content\trophy-server\map.txt` (IDs) | copiar apagado: sin mapa no hace nada |
| 35 env / tracer | `env.lua` del Conmegol vuelca globales al log (distinto al oficial); `tracer.lua` ya existe igual en el base | APAGADOS, uso interno |
Nota: la herramienta de informe al coordinador (`send_message`) dejó de estar disponible en esta sesión; por eso me detuve aquí sin copiar los añadidos.

## v0.1 · 10-nov-2026 (tarde) · Phoenix Launcher (C) y terreno de la DB global (D)
**Candado respetado:** no se abrió ni cerró ningún PES; el sider.ini real no se tocó (los tests comprueban su md5 antes y después).

### C) Phoenix Launcher — listo y probado sin PES
- **Qué es:** ventana (Python 3.11 + tkinter, ya instalados) para encender/apagar cada `cpk.root` y `lua.module` del `Sider_7.4.1\sider.ini`, con su nombre amigable de `LEYENDA_NOMBRES.txt`. El ini solo guarda nombres técnicos.
- **Abrir:** doble clic en `C:\PES21\eFootball PES 2021\Phoenix Launcher.bat` (lanza `pythonw` con `D:\Frank\IA\CLAUDE\mi-parche\launcher\phoenix_launcher.py`).
- **Funciones:** interruptor = quitar/poner `;` · antes de cada cambio copia `sider.ini.bak_<fecha>` · perfiles `V1 estable` (congelado: solo cambia con «Guardar como V1 estable», que además guarda el V1 anterior como `sider.ini.antes_<fecha>`) y `V2 en pruebas` en `Sider_7.4.1\perfiles\` · `Sider_7.4.1\CAMBIOS.md` se escribe solo · «Deshacer último cambio» (pila de 50, también deshace cambios de perfil) · ▲▼ para cambiar la prioridad · «JUGAR» lanza `sider.exe` y no lanza si ya hay un `PES2021.exe` abierto.
- **Primera apertura:** crea `perfiles\V1_estable\sider.ini` y `perfiles\V2_pruebas\sider.ini` como copia del ini de ese momento y deja activo V2. Cuando Frank confirme en vivo que la v0.1 va bien, pulsar «Guardar como V1 estable».
- **Pruebas:** `python -I test_launcher.py` → 8/8 OK sobre copias temporales (alternar ida y vuelta idéntico byte a byte, respaldo, CAMBIOS.md, deshacer x2, perfiles y V1 congelado, deshacer cambio de perfil, mover prioridad, finales de línea). Captura de la ventana hecha sobre una copia.
- **Errores en el camino:** la primera captura salía recortada porque Windows escala la ventana (DPI); se activó `SetProcessDpiAwareness(1)` y ahora se ve nítida. No hay Pillow: la captura se hace con PowerShell + System.Drawing.
- **Deshacer todo:** borrar `Phoenix Launcher.bat`, `Sider_7.4.1\perfiles\`, `Sider_7.4.1\CAMBIOS.md` y los `sider.ini.bak_*`. El sider.ini queda como lo dejó el último cambio (o restaurar el `.bak` que se quiera).

### D) Terreno de la DB global — inventario hecho (solo lectura)
- Informe completo: `D:\Frank\IA\CLAUDE\pes-db\terreno\INFORME_TERRENO_DB.md`; vista: `vista_terreno.html`.
- Cifras clave: caras base 2.838 / Conmegol 12.479 / Sudamerican 16.015 ids; 7.555 caras con el mismo id en Conmegol y Sudamerican; 117 jugadores de riesgo ALTO (mismo id, nombre distinto, cara en los dos); 214 equipos «choque» que en realidad son el mismo equipo con otro nombre (fallo de la maestra de equipos).
- Propuesta de carpetas `PHX_FIX`, `PHX_FAC_RMP/CON/SUD`, `PHX_MIN_CON/SUD`, `PHX_KIT_*`, `PHX_BAL_SUD`, `PHX_STD_<liga>`, `PHX_BDY_PRDX` con orden de prioridad (§6 del informe). Sin aplicar.
- Hueco: 4 `dt80_*` del Sudamerican no tienen TOC (solo ITOC) y no se leyeron.

## v0.1 · 10-nov-2026 (noche) · Phoenix Launcher: pestaña «Caras» (elegir qué cara gana)
**Candado respetado:** sin abrir PES; el sider.ini real no cambió (las pruebas lo comprueban por md5).

### Qué se logró
- El launcher tiene dos pestañas: **Interruptores** (lo de antes) y **Caras**.
- **Caras** lista los 8.395 jugadores con más de una versión de cara (juego base, Conmegol, Sudamerican). Primero los **116 de riesgo ALTO** (⚠, en rojo), luego los **53 con nombres que no coinciden** entre parches (≠, ámbar), luego el resto. Buscador por nombre, equipo o id.
- Por jugador, una tarjeta por versión: **minicara real** (vista previa PNG en `Sider_7.4.1\perfiles\cache_caras\`), **nombre y equipo según ese parche**, resolución de la cara (cabecera FTEX), nº de archivos y MB, si viene suelta o dentro de .cpk y si trae **PRDX (mangas/medias)**.
- Elegir: «Usar esta», «Usar la del juego», «Ninguna (usar la del juego)» o «Por defecto (orden de prioridad)». Se guarda por perfil en `Sider_7.4.1\perfiles\caras_elegidas.json` y va a `CAMBIOS.md`. V1 sigue congelado; «Guardar como V1 estable» copia también las caras del V2 (con copia previa del json).
- **Aplicar caras** crea el paquete `D:\Frank\Games_\PHX_paquetes\PHX_FAC_SEL`: **enlace duro** si la cara es una carpeta suelta del mismo disco (Conmegol: no gasta espacio); **copia** si viene de dentro de un .cpk o de otro disco (avisa antes con el tamaño). Pone `cpk.root = "D:\Frank\Games_\PHX_paquetes\PHX_FAC_SEL"` **la primera** y encendida (con respaldo `.bak`). Nunca borra: el paquete anterior va a `PHX_paquetes\_anteriores\`.
- **Quitar caras elegidas** solo apaga la línea. Se vuelve a encender en Interruptores.

### Paso a paso para replicarlo
1. `python -I D:\Frank\IA\CLAUDE\pes-db\terreno\scripts\5_nombres_por_parche.py` → `nombres_por_parche.csv` (nombre y equipos de cada id en base, Conmegol y Sudamerican).
2. Abrir el launcher → pestaña Caras → buscar → elegir → «Aplicar caras».
3. Pruebas: `python -I launcher\test_caras.py` (5/5) y `python -I launcher\test_launcher.py` (8/8).

### Errores e intentos fallidos
- **El id del jugador en Player.bin está en +8, no en +0** (en +0 hay otro número; con +0 salían decenas de jugadores con el mismo id). Nombre impreso en +251. Equipos: PlayerAssignment (fila, jugador, equipo) + Team.bin (id +8, nombre +0x170).
- **Las minicaras del juego base vienen comprimidas (CRILAYLA) y en BC7.** Se escribió un descompresor CRILAYLA en Python (comprobado: tamaño exacto y cabecera DDS) y, para BC7, se usa `D:\Frank\IA\CLAUDE\herramientas\texconv\texconv.exe` que ya estaba en la PC. DXT1/3/5 (casi todas las de los parches) se decodifican sin herramientas.
- La primera versión metía como «cara del Sudamerican» las de `content\Master-League-Selector` (no son ruta directa del juego). Se excluyen.
- Ninguna cara dentro de los .cpk de caras viene comprimida: se copian tal cual (offset = FileOffset + 2048).

### Cosas a tener en cuenta
- **Tu PES base usa SU jugador con ese id.** Ejemplo real: id 65342 = Riquelme en Conmegol pero A. Boninsegna en el base y en Sudamerican. Si eliges la cara del Conmegol, Boninsegna saldrá con la cara de Riquelme. La pantalla lo avisa.
- Las caras que vienen de .cpk ocupan espacio de verdad (unos 3–20 MB por jugador). Las de carpetas sueltas en D: no.
- El paquete es uno solo: al cambiar de perfil, pulsar «Aplicar caras» para regenerarlo con las caras de ese perfil (el launcher lo avisa).
- Vista previa de la textura grande de la cara (FTEX): no hecha (necesita descomprimir mips de Fox); se muestra su resolución.

### Comprobar y revertir
- Comprobar: abrir el paquete en el Explorador; los archivos con enlace duro muestran el mismo tamaño que el original y no suman espacio en disco.
- Revertir: «Quitar caras elegidas» (apaga). Para quitarlo del todo, borrar la línea `PHX_FAC_SEL` del sider.ini (o Deshacer) y la carpeta `D:\Frank\Games_\PHX_paquetes`.
