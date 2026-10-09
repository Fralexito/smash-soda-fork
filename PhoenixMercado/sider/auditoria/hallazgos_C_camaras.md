# Hallazgos C: módulos de cámara y de partido (Sider 7.3.3, PES 2021)

Auditoría de solo lectura. No se modificó ningún archivo.
Fuentes: `sider.ini` activo (ConmeGOL Patch 26), `SiderAddons/modules/*.lua` y `.ini`, `lib/nesalib.lua`, `SiderAddons/sider.log` (10 851 líneas; en esa sesión **no se jugó ningún partido**: no hay líneas de `set_teams` ni de `after_set_conditions` ejecutándose, solo la carga inicial y el uso del overlay).

Las copias del parche (`ConmeGol Extras/.../modules/`) de Anti-Cheat, FoulScenes, goalscreams, sifflet, attack_mentality, RLFWM y RTMM son **idénticas byte a byte** a las instaladas.

---

## 0. Resumen en 6 líneas

1. **Conflicto real y grave: `camera.lua` + `DynamicWideCam.lua` parchean el MISMO desplazamiento de instrucción** (0x1408a1fbe). Cada uno le suma +0x2c, así que el juego termina leyendo el ángulo de Dynamic Wide desde **original + 0x58**, una dirección que contenía los bytes `6E 64 00 00` (`"nd\0\0"`, parece el final de un texto) y que DynamicWideCam sobrescribe con 0.1f. Log: líneas 4468-4470 y 8020-8025.
2. **`BroadCastCam.lua` + `StadiumCam.lua` usan el MISMO patrón AOB** y escriben el **mismo float** 0x1434a00cc ("pitch"). Gana StadiumCam (carga después). Además los dos parchean la misma función de código (0x1408a5859–0x1408a5b19), sin bytes compartidos, pero el NOP de BroadCastCam cambia el comportamiento de ese mismo bloque.
3. **`camera.lua` + `FanViewCam.lua` escriben los mismos 3 floats** (0x14260b7d8/7dc/7f4). Gana FanViewCam al cargar el partido; camera.lua los revierte si se toca su overlay.
4. Los demás módulos de cámara (CommonCam, PenaltyCam, ReplayCam, VerticalCam) no se pisan con nadie.
5. `goalscreams.lua` **no hace nada** en 7.3.3 (evento `goal_scored` desconocido, log línea 340). `Anti-Cheat.lua` es **placebo puro**.
6. De los inactivos, el peligroso si se reactiva es **`attack_mentality.lua`** (hook de código con "cueva" improvisada y reescritura en caliente). RTMM y FoulScenes están bien protegidos pero dependen de APIs/versión de Sider más nuevas.

**Recomendación principal:** desactivar `camera.lua` (es la versión 2019 que FanViewCam + DynamicWideCam ya reemplazan; su única función propia es "replays on/off"), y quedarse con **uno** de BroadCastCam / StadiumCam, o al menos igualar `broad_pitch` y `stad_pitch`. Después borrar los `.cache` de cámara para que se regeneren.

---

## 1. Cómo funcionan las teclas y el mando en Sider 7 (supuesto verificado indirectamente)

En Sider 6/7, `key_down`, `gamepad_input` y `overlay_on` solo se entregan al **módulo que está seleccionado en el overlay** (y con el overlay abierto). El código no lo comenta explícitamente, pero lo confirma indirectamente:
- Los 9 módulos de cámara usan exactamente las mismas teclas (7, 8, 9, 0, -, +) y el stick derecho; si todos las recibieran a la vez, la interfaz sería inutilizable, y ningún módulo comprueba "¿estoy activo?".
- El log muestra el cambio de módulo activo (`now active module on overlay: ...`, líneas 10790-10798 y 10824-10832).

Con ese supuesto, **no hay conflicto de teclas entre módulos de cámara**. Consecuencia lateral: la lógica que vive en `overlay_on` (repetición con stick en todos los de cámara; TODA la lógica de attack_mentality) **solo corre mientras el overlay está abierto en ese módulo**.

Teclas del overlay en sider.ini: abrir/cerrar = 0x20 (**barra espaciadora**), siguiente módulo = `1`, anterior = `` ` ``. Recargar = Shift+R.

---

## 2. Módulos de cámara (los 9 activos)

Todos, salvo `camera.lua`, usan `lib/nesalib.lua` (helper + caché). Comportamiento común de nesalib:
- Eventos: `after_set_conditions` y `set_teams` (registrados en `load_ini`), más `overlay_on`, `key_down`, `gamepad_input`. **Ninguno usa `display_frame`** (no hay trabajo por frame fuera del overlay).
- `set_teams` → recarga el .ini y aplica los valores "default". `after_set_conditions` → si hay sección para el estadio (`[NNN::Carpeta\Estadio]`, vía `ctx.stadium_server` de StadiumServer) cambia a modo estadio y aplica.
- Escrituras: `memory.write` de floats en las direcciones calculadas. No valida el valor viejo, no restaura nunca.
- Si no encuentra el patrón: `error(...)` → Sider marca el módulo como fallido y sigue. Pero los parches ya escritos antes del error se quedan.
- Caché: `ctx.sider_dir .. "modules\\<Módulo>.cache"`, 8 bytes (u64) por patrón. Al iniciar compara los bytes del patrón en la dirección guardada; si coinciden la usa ("matched cache hint #N"), si no, busca en todo el proceso. Se **reescribe en cada arranque** (`cache.save()`).
- Archivos escritos: su `.ini` (al soltar el stick o con teclas + / - / 8) y su `.cache`.
- Teclas: `9`/`0` elegir parámetro, `-`/`+` cambiar valor, `8` valores por defecto, `7` modo común/estadio. Mando: RS arriba/abajo elige, RS izquierda/derecha cambia (tras 30 frames sostenido, cambia cada frame).
- Errores menores: en `set_teams` el log usa una variable global `key` inexistente (solo afecta el texto del log); variables `sname`/`addr` sin `local`.

### 2.1 camera.lua (v3.1, 2019, sin nesalib)
- **Propósito:** ajustar ángulo de Dynamic Wide, zoom/altura/ángulo de Fan View y activar/desactivar repeticiones.
- **Eventos:** `set_teams` (aplica todo), `overlay_on`, `key_down`, `gamepad_input`. Sin trabajo por frame.
- **Escrituras de memoria:**
  | Qué | Patrón AOB | Dirección (log) | Escritura | Tipo |
  |---|---|---|---|---|
  | Bloque Fan View | `0F 11 44 24 20 89 45 87 0F 11 4C 24 30`; base = loc + rel32 leído en loc-4 | base 0x14260b7d0 (l.4464) | floats en +0x08 (zoom), +0x0C (altura), +0x24 (ángulo) | datos |
  | Repeticiones | `41 C6 45 08 04` (`mov byte [r13+8],4`) | 0x1420b2ff1 (l.4466) | 1 byte en loc+4: `04`=on, `07`=off | **código** (inmediato) |
  | Dynamic Wide ángulo | `F3 41 0F 59 FA F3 45 0F 5E C2 F3 41 0F 5E F2 F3 41 0F 5C C0` | instrucción en 0x1408a1fb9 (l.4468) | **rel32 en loc-4 (0x1408a1fbe) += 0x2C** en `init`; luego float en la nueva dirección | **código** (desplazamiento) + datos |
- **Validación:** solo la coincidencia del patrón. No comprueba el valor del desplazamiento antes de sumarle 0x2C (aquí está el choque con DynamicWideCam). No restaura.
- **Detalle peligroso del patrón de repeticiones:** solo 5 bytes, muy genérico, y **incluye el byte que el propio módulo cambia** (04). Si alguna vez se pone `replays = off` y luego se recarga el módulo, el patrón ya no coincide en la caché y la búsqueda puede encontrar **otra** instrucción `mov byte [r13+8],4` y cambiarla a 7. Hoy `replays = on` (no-op), riesgo dormido.
- **Archivos:** `SiderAddons\camera.ini` (existe, log l.4458-4462) y `SiderAddons\camera.cache` (3 × i64). `write_cache` no comprueba si el archivo se abrió.
- **Teclas:** 8, 9, 0, -, +; RS.
- **Log:** 4457-4475.
- **Riesgo: ALTO** (por el choque con DynamicWideCam y FanViewCam; ver sección 4). Es redundante con módulos más nuevos.

### 2.2 BroadCastCam.lua (v2.2)
- **Propósito:** cámara "Broadcast": altura, zoom, ángulo (FOV), pitch y 3 parámetros extra. Tiene secciones por estadio en el .ini (archivo de 55 KB).
- **Escrituras:**
  | Qué | Patrón | Dirección | Escritura | Tipo |
  |---|---|---|---|---|
  | Tabla zoom/pitch | `48 89 45 27 48 63 41 78 4C 8D 05` (+ rel32 en loc+11; base = loc+15+rel) | base 0x1434a00c0 | float +0x04 zoom (0x1434a00c4), **+0x0C pitch (0x1434a00cc)** | datos |
  | Altura/ángulo/params | base fija = loc + 0x44 (sin buscar) | 0x1408a5859 | floats en +3, +10, +17, +24, +31 (0x1408a585c/863/86a/871/878) = inmediatos de 5 instrucciones `mov dword [rbp+x], imm32` | **código** (inmediatos) |
  | "Quitar bloqueo de ángulo" | `F3 0F 11 45 D7 4C 8D 45 C7` buscando desde loc | 0x1408a5b15 | 5 × `90` (NOP sobre `movss [rbp-29],xmm0`, instrucción exacta de 5 bytes) | **código** |
- **Validación:** solo el patrón. El offset fijo +0x44 depende de la versión del exe; aquí los valores viejos del log son coherentes (0.3, 70, -3, 25.98, 46.72 → l.6988-6994), así que la alineación es correcta en este exe.
- **Nota:** `helper` es variable global (sin `local`); FanViewCam también. Si Sider comparte el entorno global entre módulos, FanViewCam la sobrescribe; no rompe nada porque los callbacks ya quedaron registrados.
- **Valores llamativos en el .ini:** `broad_param1 = -200`, `broad_param3 = 200/85` (originales -3 y 46.72). Son valores muy fuera del rango original; no es un crash, pero si la cámara se ve rara, empezar por aquí.
- **Log:** 6983-6998. **Riesgo: MEDIO** (choque de pitch con StadiumCam; parches de código en la misma función).

### 2.3 CommonCam.lua (v2.1)
- **Propósito:** cámara común usada por los presets Media, Larga, Amplia, Lateral y Personalizada. Secciones por estadio.
- **Escrituras:** patrón `84 C0 B9 06 00 00 00 0F 45 D9 F3 0F 10 5F 0C 48 8D 0D` (rel32 en loc+0x12; base = loc+0x16+rel) → base 0x14260b720; floats +0x28 zoom (…748), +0x2C altura (…74c), +0x30 pitch (…750), +0x44 ángulo (…764). Solo **datos**.
- **Log:** 8002-8011 (ángulo 1.3 → 3.5, pitch 0.5 → 2.5). **Riesgo: BAJO.** No se cruza con nadie.

### 2.4 DynamicWideCam.lua (v2.0)
- **Propósito:** zoom, altura y ángulo de Dynamic Wide.
- **Escrituras:**
  | Qué | Cómo | Dirección (log) | Tipo |
  |---|---|---|---|
  | Ángulo | mismo patrón de 20 bytes que camera.lua; **rel32 en loc-4 += 0x2C**; float en la nueva dirección | instrucción 0x1408a1fb9; "org" 0x1425985a4; nueva **0x1425985d0** (l.8020-8022) | **código** (desplazamiento) + datos |
  | Zoom | rel32 en loc-13, addr = loc-9+rel | 0x14260b8fc | datos |
  | Altura | rel32 en loc+24, addr = loc+28+rel | 0x14260b8e0 | datos |
- **Prueba del doble parche:** camera.lua dice "org 0x142598578 → nueva 0x1425985a4". DynamicWideCam, que carga después, dice "org **0x1425985a4**" (ya no es el original) → "nueva 0x1425985d0". El valor que había allí: `3.6027383517791e-41` (l.8025) = bytes `6E 64 00 00` = texto `"nd"` + terminador. Se sobrescribe con 0.1f (`CD CC CC 3D`). Es decir: **el juego ya no lee del hueco libre que eligió el autor, y además se pisa un dato ajeno (probablemente el final de una cadena de texto, perdiendo su terminador)**.
- **Recarga (Shift+R):** si el módulo se vuelve a inicializar en el mismo proceso, vuelve a sumar +0x2C (el patrón no incluye el desplazamiento, así que siempre coincide). Cada recarga empuja la lectura más lejos. Mismo problema en camera.lua y PenaltyCam.
- **Log:** 8013-8031. **Riesgo: ALTO mientras camera.lua siga activo; BAJO si se quita camera.lua.**

### 2.5 FanViewCam.lua (v2.2)
- **Propósito:** zoom/altura/ángulo de Fan View (mismos datos que camera.lua).
- **Escrituras:** mismo patrón que camera.lua (`0F 11 44 24 20 89 45 87 0F 11 4C 24 30`), base 0x14260b7d0; floats +0x08, +0x0C, +0x24. Solo **datos**.
- **Eventos:** registra `set_teams` **dos veces** (el de nesalib y uno propio) → aplica dos veces, inofensivo.
- **Log:** 8033-8048 (25.6 → 25, 0.43 → 0.37, 1 → 0.9). **Riesgo: BAJO por sí mismo; MEDIO por la pelea de valores con camera.lua.**

### 2.6 PenaltyCam.lua (v3.1)
- **Propósito:** zoom, altura y ángulo de la cámara de penales.
- **Escrituras:**
  | Qué | Patrón | Escritura | Dirección | Tipo |
  |---|---|---|---|---|
  | Zoom | `F3 0F 10 8B 88 08 00 00 F3 0F 59 35` | float en la constante leída por `mulss [rip+x]` (loc+16+rel) | 0x1425ee920 | datos (constante en .rdata; **podría compartirla otro código**; hoy 17.6 → 17.6, sin cambio) |
  | Altura | `48 8D 4E 08 48 89 9C 24 00 01 00 00 F3 44 0F 58 35` | **rel32 en loc+17 += 0x18** + float en el hueco | 0x14258c27c (valía 0 → 3) | **código** + datos |
  | Ángulo | `8B 86 84 08 00 00 41 89 44 24 0C 8B 86 88 08 00 00 41 89 44 24 10` | 11 bytes en loc+22: `41 C7 44 24 14 <imm32> 90 90` (+ imm en loc+27) | parche 0x14087d80b–0x14087d815; imm 0x14087d810 | **código** |
- **Validación:** no comprueba qué bytes había en loc+22 antes de escribir 11 bytes (asume `mov eax,[rsi+88C]` + `mov [r12+14],eax`). Si el exe fuera distinto, rompería una instrucción → crash. Aquí el patrón coincide, así que probablemente está bien.
- **Orden de errores:** el desplazamiento de altura se escribe **antes** de buscar el ángulo; si el ángulo no se encontrara, el módulo falla pero deja la altura apuntando a un hueco con 0.
- **Recarga:** acumula +0x18 en cada reinicialización.
- **Log:** 8050-8067. **Riesgo: BAJO-MEDIO** (parches de código sin verificación previa; nadie más toca esas direcciones).

### 2.7 ReplayCam.lua (v1.0)
- **Propósito:** altura (cam 0 y cam 3) y zoom de las repeticiones.
- **Escrituras:** tres inmediatos dentro de instrucciones `mov dword [...], imm32`, **código**: 0x1421790f8 (patrón `C7 45 AB CD CC CC 3E F3 41 0F 58 C0 F3 0F 11 45 A7 0F 28 C6`, +3), 0x14217a4b3 (patrón `F3 0F 58 E0 F3 0F 11 46 14 C7 46 10 66 66 A6 3F`, +12), 0x142178fad (patrón `C7 46 30 C3 F5 28 3F F3 44 0F 10 06 F3 44 0F 10 4E 08 F3 44 0F 11 45 C7`, +3).
- **Ojo:** dos de los patrones **incluyen el valor que se modifica** (`CD CC CC 3E`, `C3 F5 28 3F`). Si se cambia el valor y se recarga en el mismo proceso, el patrón ya no se encuentra → `error`. Hoy los valores son los originales (l.8081-8083: sin cambios), así que el módulo **no está haciendo nada útil**.
- **Log:** 8069-8087. **Riesgo: BAJO.**

### 2.8 StadiumCam.lua (v2.0)
- **Propósito:** cámara "Estadio": altura, zoom (FOV), ángulo, pitch. Secciones por estadio (33 KB).
- **Escrituras:**
  | Qué | Patrón | Dirección | Tipo |
  |---|---|---|---|
  | Pitch | **el mismo que BroadCastCam**: `48 89 45 27 48 63 41 78 4C 8D 05`; tabla + 0x0C | **0x1434a00cc** (= broad_pitch) | datos |
  | Altura | `F3 0F 11 45 CF EB 07 C7 45 CF` desde loc, +10 | 0x1408a5966 | código (inmediato) |
  | Zoom | `F3 0F 11 45 D7 EB 07 C7 45 D7` desde loc, +10 | 0x1408a5a61 | código (inmediato) |
  | Ángulo | `F3 0F 11 65 E7 EB 07 C7 45 E7` desde loc, +10 | 0x1408a5b04 | código (inmediato) |
- **Log:** 9656-9671. La línea 9666 lo demuestra: `stad_pitch: changed at 0x1434a00cc: 0 --> 0.9` (el "0" es lo que acababa de escribir BroadCastCam). **Riesgo: MEDIO** (pelea con BroadCastCam).

### 2.9 VerticalCam.lua (v2.2)
- **Propósito:** cámara vertical: altura, zoom, pitch, ángulo.
- **Escrituras:** base 0x142609dd0 (patrón `48 8B 7C 24 60 48 8B 74 24 58 48 8B 5C 24 50 83 F8 07 75 19 4C 8D 05`, rel32 en loc+0x17) floats +0x08/+0x0C/+0x10; ángulo 0x142609f74 (patrón nuevo 1.04 `F3 0F 11 85 CC 00 00 00 F3 0F 11 8D D0 00 00 00 F3 0F 11 95 D4 00 00 00 E9`, rel32 en loc+0x24) = constante de un `mulss` (posible constante compartida). Solo **datos**.
- **Log:** 9673-9691 (todos los valores sin cambio). **Riesgo: BAJO.**

### 2.10 Archivos .cache
- camera.lua: `SiderAddons\camera.cache`. Los de nesalib: `SiderAddons\modules\<Módulo>.cache`. No venían en la copia auditada.
- Se reescriben **en cada arranque**. La validación (comparar el patrón en la dirección guardada) es buena, pero hace `memory.read` en una dirección vieja: si se cambia el exe (actualización, otro ejecutable tipo FL), esa lectura podría caer fuera de memoria válida. **Recomendación:** borrar todos los `.cache` cada vez que cambie el `.exe` o después de quitar camera.lua.

---

## 3. Módulos de partido / juego

### 3.1 sifflet.lua (ACTIVO) — "WhistleHome"
- **Propósito:** reproducir `content/WhistleHome/sifflet.mp3` cuando el marcador pasa de 0-2 a 0-3 o de 0-3 a 1-3, y `applaudi.mp3` de 0-3 a 0-4 (siempre visto desde el local perdiendo).
- **Eventos:** `set_teams`, **`display_frame` (cada frame)**.
- **Trabajo por frame:** una llamada a `match.stats()` y dos comparaciones. Solo escribe en el log cuando cambia el marcador. Costo bajo (crea una tabla por frame → algo de basura para el GC).
- **Memoria:** ninguna escritura. **ffi/hooks:** no. **Archivos:** solo lee los mp3.
- **Errores:** `finish()` protegido con `pcall`; si `audio.new` devuelve nil lo registra. Si `audio.new` lanzara error (archivo inexistente), el error ocurriría dentro de `display_frame`, pero solo en esos 3 cambios de marcador concretos.
- **Teclas:** ninguna. **Log:** 328-337 (carga correcta).
- **Riesgo: BAJO.**

### 3.2 goalscreams.lua (ACTIVO) — inerte
- **Propósito:** reproducir un grito de gol por jugador (`content\audio-demo\goal\goalscreams\<id>_*.mp3`), solo goles locales.
- **Eventos:** `goal_scored`, `set_teams`, `context_reset`, `overlay_on`.
- **Hecho clave:** el propio archivo dice "Requires Sider 7.3.4+" y el usuario tiene **7.3.3**. Log línea 340: `WARN: trying to register for unknown event: "goal_scored"`. **El módulo nunca reproduce nada**; solo muestra su texto en el overlay.
- **Memoria:** ninguna. **Riesgo: NINGUNO** (placebo). Para que funcione hay que subir a Sider 7.3.4+.

### 3.3 Anti-Cheat.lua (inactivo) — placebo confirmado
- Pone `ctx.home_dynamic_difficulty = 0` y `ctx.away_dynamic_difficulty = 0`. **Esos campos no existen en Sider**: `ctx` es una tabla de Lua; escribir un campo inventado no llega al juego.
- `m.set_match_settings` (pone `settings.foul_level = 1`, campo que tampoco existe) y `m.after_set_conditions` **nunca se registran con `ctx.register`**, así que Sider jamás los llama.
- Lo único que hace es escribir una línea en el log. Sin memoria, sin hooks, sin archivos.
- **Si se reactiva: riesgo NINGUNO, efecto NINGUNO.** Las promesas ("quita la dificultad dinámica", "árbitro estándar") son falsas.

### 3.4 attack_mentality.lua (inactivo) — el más peligroso si se reactiva
- **Propósito:** forzar la mentalidad de la IA (0 Bus … 4 Ataque total) a partir de un minuto, según el marcador; modo manual y modo ida/vuelta.
- **Hook de código:**
  - Patrón `8B C3 41 0F 4F C6 89 81 A4 B8 00 00 3B DD`; hook en patrón+6 (8 bytes: `mov [rcx+B8A4],eax` + `cmp ebx,ebp`) → `E9 rel32 90 90 90`.
  - "Cueva": **la primera secuencia de 32 bytes `CC` que encuentre en el proceso** (relleno entre funciones), o si no, **32 bytes `00`**. No reserva memoria propia (no usa `memory.allocate_codecave`).
  - Cueva normal: `mov [rcx+B8A4],eax; cmp ebx,ebp; jmp vuelta` (13 bytes). Cueva forzada: `mov eax,valor; …` (18 bytes).
- **Problemas:**
  1. La cueva se **reescribe en caliente** mientras el juego puede estar ejecutándola (cambia de 13 a 18 bytes y viceversa) → posible crash por instrucción a medio escribir.
  2. Si no hay 32×CC y cae en 32×00 de una zona de datos no ejecutable → **crash** al saltar allí. Además esos ceros podrían ser datos usados.
  3. Cualquier otro mod que use el mismo truco del "primer bloque CC" chocaría con este.
  4. Nunca restaura el código original.
  5. `ptr_to_num` depende de que `tostring(dirección)` contenga "0x…"; si Sider devolviera un número, el módulo daría error al iniciar (después no instala nada).
  6. Toda la lógica corre en `overlay_on`: **solo funciona con el overlay abierto en este módulo**.
- **Archivos:** `ai_mentality_settings.txt` con ruta relativa → se escribe en la carpeta de trabajo del juego (no en SiderAddons).
- **Teclas:** P, O, M, L, -, +, [, ], 6, 7, 8, 9 (solo cuando está activo en el overlay).
- **Riesgo si se reactiva: ALTO.**

### 3.5 FoulScenes.lua (inactivo) — "PK VAR"
- **Propósito:** en jugadas de penal, con 65 % de probabilidad redirige la escena a una secuencia de VAR (archivos .fdc propios); desde el minuto 60 cambia algunas escenas de falta por otras ("escalada").
- **Hook:** patrón de 21 bytes `41 8B 0C 07 8B C1 C1 E8 10 0F B6 D8 89 5C 24 30 44 0F B7 E9 41 FF CD`; reemplaza 16 bytes exactos (4 instrucciones completas) por `FF 25 00000000 <u64>` + `90 90`. Cueva propia con `memory.allocate_codecave(128)` que llama a un **evento personalizado de Sider** (`ctx.custom_evt_rbx`, `ctx.get_event_id`).
- **Validación buena:** comprueba que los 16 bytes sigan originales ("hook ocupado por otro modulo") y que existan los eventos personalizados.
- **Escritura de datos:** 26 de 28 bytes de un registro de escena (rax+r15), con copia y **restauración** al terminar o tras 900 frames.
- **Eventos:** `display_frame` (contador y lectura del minuto cada 60 frames), `livecpk_make_key`, `livecpk_get_filepath`, `livecpk_rewrite`, `set_teams`, `overlay_on`.
- **Riesgos:** `livecpk_rewrite` redirige a nombres como `foul_excitee_card_n01_2015.fdc` y `foul_injuryy_card_w01.fdc` (¿errata o archivos propios?). Si esos archivos no existen en ningún cpk/livecpk, el juego podría fallar al cargar la escena. La carpeta de contenido es `SiderAddons\Scenes\` (no `livecpk\Scenes`, que es lo que está en `cpk.root`). Depende de APIs (`allocate_codecave`, eventos personalizados) que quizá no existan en 7.3.3; en ese caso se desactiva solo, sin parchear.
- **Riesgo si se reactiva: MEDIO.**

### 3.6 Real-Time-Match-Manager.lua (RTMM, inactivo, 176 KB)
- **Propósito:** hora del día que avanza durante el partido (sombras, luz), lluvia "brutal" y transiciones de clima.
- **Escrito para:** "Sider 7.4.x" y `FL_2026.exe`. El usuario tiene 7.3.3 + PES2021.exe.
- **Hooks (3 grupos, todos con cuevas propias de `memory.allocate_codecave`):**
  - Bomba de actualización de la hora: marcador `80 BB 00 04 00 00 00 74 07 C6 83 80 00 00 00 01 80 BB 80 00 00 00 00 74 1E`, hook 0x22 bytes antes; reemplaza 15 bytes `40 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF` por salto absoluto + NOP. Escribe hora/minuto/segundo en la zona de datos de su cueva.
  - RainDemo (lluvia): 3 firmas A/B/C (17/14/17 bytes reemplazados), más comprobación de distancias exactas entre ellas (0x1FB y 0x2E8) y de dos funciones auxiliares.
  - Lluvia en pantalla: constructor y update (22 y 14 bytes).
- **Validación excelente:** exige firma **única** (`memory.safe_search`), comprueba bytes originales, relee lo escrito y **restaura si falla**. Ante cualquier duda hace "SAFE ABORT" y no toca nada.
- **Riesgos:** `memory.safe_search` y `memory.allocate_codecave` podrían no existir en 7.3.3 → error en `init` antes de escribir nada (falla seguro). Trabajo en `display_frame` cada frame. Redirige archivos de luz/efectos (`livecpk_get_filepath`) que **también redirige RLFWM**; Sider usa la primera respuesta no vacía, así que RLFWM (está antes en sider.ini) taparía las de RTMM para los mismos archivos.
- **Archivos:** lee `modules\RTMM-settings.ini`, `modules\RTMM-night-lighting.ini`, `content\Real-Time-Match-Manager\...`.
- **Teclas:** F5, F6, F7, Re Pág, Av Pág, Insert, Supr.
- **Riesgo si se reactiva: MEDIO** (código serio, pero hooks profundos en un exe/versión distintos a los de su objetivo).

### 3.7 Real-Life-Feel-Weather-Manager.lua (RLFWM, inactivo, 89 KB)
- **Propósito:** elegir clima según competición, país, mes y hemisferio; niebla, nubes, truenos; historial para continuidad.
- **Memoria:** **ninguna escritura**, sin ffi (lo dice su cabecera y el grep lo confirma).
- **Eventos:** `set_teams`, `set_conditions`, `livecpk_make_key`, `livecpk_get_filepath`, `livecpk_data_ready`, `key_down`, `overlay_on`, `gamepad_input`, `display_frame` (cada frame: solo revisa el temporizador de truenos).
- **Se hace pasar por WeatherConditions:** pone `ctx.weather_conditions = m`. StadiumServer (activo) llama a `ctx.weather_conditions.set_conditions(...)` si existe el archivo `modules\WeatherConditions.lua` (existe), así que con RLFWM activo el clima pasaría dos veces por RLFWM (Sider + StadiumServer). Lo guarda en caché por partido, así que no hay doble sorteo.
- **Archivos escritos:** `modules\WeatherManager-history.ini` (mes) y `modules\WeatherManager-history.txt`.
- **Teclas:** F9-F12.
- **Riesgo si se reactiva: BAJO.**

### 3.8 WeatherConditions.lua (no figura en sider.ini, ni comentado)
- **Propósito:** sortear clima por competición desde `content\weather-conditions\map_competitions.csv`.
- Sin memoria ni hooks. Cambia `options.weather` / `weather_effects` en `set_conditions` y publica `ctx.weather_conditions`.
- **Problemas si se carga:** define funciones **globales** (`ternary`, `readCsv`, `tprint`, `INDEX_*`) y **modifica `string:split` para todo Lua** (MasterLeague.lua también define el suyo; el último cargado gana). `assert(io.open(...))`: si falta el csv, el módulo no carga. Si todas las probabilidades de una liga son 0, `math.random(0)` da error. Re-siembra el generador aleatorio global. Choca con RLFWM por `ctx.weather_conditions` (gana el último cargado).
- **Riesgo si se activa: BAJO.**

---

## 4. Tabla cruzada: patrón + offset → módulo

| Patrón AOB (inicio) | Offset / destino | Dirección real (log) | Módulos | ¿Conflicto? |
|---|---|---|---|---|
| `F3 41 0F 59 FA F3 45 0F 5E C2 …` (20 B) | rel32 en loc-4 (**código**), += 0x2C | 0x1408a1fbe → lee 0x142598578 / 0x1425985a4 / **0x1425985d0** | **camera.lua** + **DynamicWideCam** | **SÍ, GRAVE.** Suma doble (+0x58). camera.lua escribe en un sitio que el juego ya no lee; DynamicWideCam pisa `6E 64 00 00` ("nd\0"). |
| `0F 11 44 24 20 89 45 87 0F 11 4C 24 30` | base+0x08 / +0x0C / +0x24 (datos) | 0x14260b7d8 / 7dc / 7f4 | **camera.lua** + **FanViewCam** | **SÍ.** Mismos floats. Gana FanViewCam en set_teams/after_set_conditions; camera.lua los revierte al tocar su overlay. Sin crash. |
| `48 89 45 27 48 63 41 78 4C 8D 05` | tabla+0x0C (datos) | **0x1434a00cc** | **BroadCastCam** (broad_pitch) + **StadiumCam** (stad_pitch) | **SÍ.** Mismo float. Gana StadiumCam (log l.9666: 0 → 0.9). Sin crash. |
| mismo patrón | tabla+0x04 (datos) | 0x1434a00c4 | BroadCastCam | no |
| mismo patrón, loc+0x44 | inmediatos +3/+10/+17/+24/+31 (código) | 0x1408a585c…0x1408a587b | BroadCastCam | misma función que StadiumCam, sin bytes en común |
| `F3 0F 11 45 CF EB 07 C7 45 CF` | +10 (código) | 0x1408a5966 | StadiumCam | no |
| `F3 0F 11 45 D7 EB 07 C7 45 D7` | +10 (código) | 0x1408a5a61 | StadiumCam | no (FOV de la misma rutina que el NOP) |
| `F3 0F 11 65 E7 EB 07 C7 45 E7` | +10 (código) | 0x1408a5b04 | StadiumCam | no (11 bytes antes del NOP) |
| `F3 0F 11 45 D7 4C 8D 45 C7` | NOP ×5 (código) | 0x1408a5b15 | BroadCastCam | **indirecto**: quita una escritura final de `[rbp-29]` en la misma rutina que usa StadiumCam |
| `41 C6 45 08 04` | +4, 1 byte (código) | 0x1420b2ff5 | camera.lua | no (patrón genérico, riesgo dormido) |
| `84 C0 B9 06 00 00 00 0F 45 D9 F3 0F 10 5F 0C 48 8D 0D` | +0x28/2C/30/44 (datos) | 0x14260b748…767 | CommonCam | no |
| (derivadas de Dynamic Wide) | zoom / altura (datos) | 0x14260b8fc / 0x14260b8e0 | DynamicWideCam | no |
| `F3 0F 10 8B 88 08 00 00 F3 0F 59 35` | constante (datos) | 0x1425ee920 | PenaltyCam | no |
| `48 8D 4E 08 48 89 9C 24 00 01 00 00 F3 44 0F 58 35` | rel32 += 0x18 (código) | → 0x14258c27c | PenaltyCam | no |
| `8B 86 84 08 00 00 41 89 44 24 0C 8B 86 88 08 00 00 41 89 44 24 10` | 11 B en loc+22 (código) | 0x14087d80b | PenaltyCam | no |
| 3 patrones ReplayCam | inmediatos (código) | 0x1421790f8 / 0x14217a4b3 / 0x142178fad | ReplayCam | no |
| 2 patrones VerticalCam | datos | 0x142609dd8…de3 / 0x142609f74 | VerticalCam | no |

Ningún otro módulo activo registra en el log direcciones dentro de estos rangos (búsqueda en todo sider.log).

### Qué pasa exactamente en cada choque

**camera.lua + DynamicWideCam (orden de carga: camera.lua pos. 44, DynamicWideCam pos. 47)**
1. camera.lua cambia la instrucción para que lea en original+0x2C.
2. DynamicWideCam lee esa instrucción ya cambiada, cree que es la original y le suma otra vez 0x2C.
3. Resultado: el juego lee el ángulo en original+0x58. DynamicWideCam escribe 0.1 allí (funciona "por casualidad"), pero ese sitio no estaba libre: tenía `"nd\0\0"`.
4. camera.lua, en cada `set_teams`, escribe 0.2 en original+0x2C, que ya nadie lee: su control de "dynamic wide camera angle" no sirve para nada.
5. ¿Puede crashear? No de forma segura, pero sí es posible: si ese "nd\0" es el final de un texto que el juego usa, ese texto queda sin terminador y el juego leería basura después. Con Shift+R (recarga) el desplazamiento seguiría creciendo.
6. Solución: quitar `camera.lua`. DynamicWideCam solo suma +0x2C como diseñó su autor.

**camera.lua + FanViewCam**: escrituras de datos, gana el último que escribe. Al cargar un partido gana FanViewCam (registra después y también en `after_set_conditions`). Sin crash.

**BroadCastCam + StadiumCam**: comparten el float de pitch; el último en escribir gana (StadiumCam, en carga y en cada evento). Si el usuario ajusta `broad_pitch` en el overlay se aplica hasta el próximo `set_teams`/`after_set_conditions`, donde StadiumCam lo pisa. Los parches de código están en la misma función pero en bytes distintos, todos en límites de instrucción correctos → sin crash. Solución: usar solo uno o poner el mismo pitch en los dos .ini.

### Teclas y mando entre módulos de cámara
- Los 8 de nesalib: 7, 8, 9, 0, -, + y RS. camera.lua: 8, 9, 0, -, + y RS. **Todos idénticos.**
- Con el comportamiento de Sider 7 (solo el módulo activo en el overlay recibe teclas/mando), **no hay conflicto**.
- Fuera de cámaras: attack_mentality usaría 6, 7, 8, 9, -, + (mismo caso); RTMM F5-F7, Re Pág/Av Pág, Insert, Supr; RLFWM F9-F12.
- El atajo de recarga Shift+R es el que puede reaplicar los desplazamientos acumulativos (camera.lua, DynamicWideCam, PenaltyCam).

---

## 5. Lista de acciones sugeridas (en orden)

1. Comentar `lua.module = "camera.lua"` en sider.ini (si se quiere apagar repeticiones, hacerlo desde el juego).
2. Elegir BroadCastCam **o** StadiumCam; si se quedan los dos, igualar `broad_pitch` y `stad_pitch`.
3. Borrar `SiderAddons\camera.cache` y `SiderAddons\modules\*.cache` (se regeneran solos).
4. No usar Shift+R durante el juego con módulos de cámara cargados.
5. goalscreams.lua: o actualizar Sider a 7.3.4+, o quitarlo (hoy no hace nada).
6. Anti-Cheat.lua: borrarlo; no hace nada.
7. No reactivar attack_mentality.lua.
