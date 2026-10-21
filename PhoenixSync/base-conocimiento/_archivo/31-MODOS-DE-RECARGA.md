# 31 · phoenix.lua v0.18 — los tres modos de recarga: dónde, cómo y cuándo se aplica cada uno

Escrito el 2026-10-10 a las 10:25 (Lima). Pedido de FRALEX:
- 09:50: «quiero el que sea automático y el que haga los fichajes cuando le dé en Activar, y una tecla para elegir el modo».
- 10:10: «dependería de la elección del usuario, poniendo un botón en Link».
- 10:12: «deja más claro la diferencia de cada botón y cuál viene por defecto».

**Estado: escrita y simulada, SIN instalar.** En el PC sigue la v0.17e-B.

---

## 1. Los tres modos, en palabras simples

### 🟢 Modo 1 · ACTIVAR · ⭐ VIENE POR DEFECTO
- **Qué haces tú:** Partido → Datos Actual. en vivo → **Activar**.
- **Qué hace el juego:** recarga los fichajes **solo cuando tú lo pides**.
- **Ejemplo:** una lámpara con interruptor. Solo se enciende si tú la pulsas.
- Es **exactamente la v0.17e**, ya probada. Después de Activar la pantalla dice «Desactivar», pero eso no importa.
- **Por qué es el de por defecto:** es el probado. Además, si no hay elección (falta `modo.txt` o dice algo raro), o si lo automático falla, el módulo usa este.

### 🔵 Modo 2 · AUTO-FICHAJES (automático, solo con fichajes nuevos)
- **Qué haces tú:** nada (Activar sigue funcionando si lo quieres usar).
- **Qué hace el juego:** cuando Phoenix Sync avisa «hay fichajes nuevos», el juego recarga solo **la próxima vez que estés en el menú principal y entres a un modo**. Si no hay nada nuevo, no recarga.
- **Ejemplo:** un cartero. Solo toca el timbre cuando hay una carta.

### 🟣 Modo 3 · AUTO-SIEMPRE (automático, siempre)
- **Qué haces tú:** nada (Activar sigue funcionando).
- **Qué hace el juego:** **cada vez** que vuelves al menú principal desde un modo y entras a otro, recarga, haya algo nuevo o no.
- **Ejemplo:** mirar el buzón cada vez que pasas por la puerta.
- La **primera** entrada después de abrir el juego no recarga, porque la base se acaba de cargar.
- El juego tarda un poco más en entrar a cada modo.

| Modo | ¿Recarga solo? | ¿Cuándo? |
|---|---|---|
| 🟢 ACTIVAR ⭐ (por defecto) | No | Solo al pulsar Activar |
| 🔵 AUTO-FICHAJES | Sí | Solo si hay fichajes nuevos |
| 🟣 AUTO-SIEMPRE | Sí | Cada vez que vuelves al menú y entras a un modo |

**En los tres, el botón Activar del juego sigue funcionando.**

---

## 2. DÓNDE se elige el modo

Todo vive en la carpeta **buzón** de Phoenix, la misma del archivo de avisos:

```
<juego>\SiderAddons\content\phoenix\
    avisos.txt      ← ya existía (mensajes de la web en el overlay)
    modo.txt        ← NUEVO: el modo elegido
    recargar.txt    ← NUEVO: el aviso «hay fichajes nuevos»
```

En el PC de FRALEX, la carpeta que usa el juego es `D:\Frank\Games_\Conmegol Patch\SiderAddons\content\phoenix\`. Ojo: el switcher de ConmeGOL copia todo desde `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\`, así que conviene tener los archivos también ahí (ver guía VINCULO-TIEMPO-REAL).

### `modo.txt`: la notita del modo
- **Quién lo escribe:** Phoenix Link, cuando el usuario pulsa el botón de modo. Mientras el botón no exista, `PhoenixModo.bat`.
- **Qué contiene:** una sola palabra: `ACTIVAR`, `AUTO-FICHAJES` o `AUTO-SIEMPRE`. Da igual mayúsculas o minúsculas; se acepta `_` en vez de `-`, y un BOM o un salto de línea al final.
- **Cómo se escribe:** de forma atómica (primero a `modo.tmp`, luego se renombra), en UTF-8 sin BOM, de 64 bytes como mucho.
- Si **no existe** o dice otra cosa → **ACTIVAR**.

### `recargar.txt`: la notita de «hay fichajes nuevos»
- **Quién lo escribe:** Phoenix Sync, cada vez que mete fichajes nuevos en el option file. Mientras no lo haga, `PhoenixRecargar.bat`.
- **Qué contiene:** cualquier texto **distinto del anterior**, de 256 bytes como mucho (por ejemplo la fecha y hora). Lo que importa es que **cambie**.
- Escritura atómica igual que `modo.txt`.
- Lo que ya hubiera al abrir el juego **no cuenta** (la base se acaba de cargar). Con el modo ACTIVAR, los avisos se ignoran.

### La tecla T: para probar
- Con el overlay de Sider abierto (Espacio, pasar al módulo Phoenix), **T** pasa de un modo al siguiente: ACTIVAR → AUTO-FICHAJES → AUTO-SIEMPRE → ACTIVAR.
- Vale **solo durante esa sesión de juego**. Si `modo.txt` cambia después (Link eligió otra vez), manda `modo.txt`.
- No es una tecla de prueba peligrosa: solo cambia una elección; no escribe en el juego por sí misma.

---

## 3. CÓMO funciona por dentro (para otra IA o una persona técnica)

Archivo: `PhoenixSync/sider/pruebas/phoenix-v0.18-prueba.lua` (sha256 en §6). Es la v0.17e más un bloque «v0.18».

1. **Arranque:** como la v0.17e. En la primera lectura de la base (`livecpk_read`, `autoHecho`), aplica los parches A, B y C del botón nativo (guía 30). Antes de eso, lo automático no hace nada.
2. **Evento nuevo `display_frame`:** Sider lo llama en cada cuadro de imagen. El trabajo real se hace **como mucho cada 2 segundos** (`os.clock`):
   - Lee `modo.txt` y `recargar.txt` (dos archivos diminutos).
   - **En ACTIVAR, no hace nada más:** ni lee ni escribe la memoria del juego.
   - En un modo automático, mira la zona del juego (solo lectura, guía 22) y el interruptor `exe+0x37F5C39` (solo lectura).
3. **Cuándo decide recargar:**
   - AUTO-FICHAJES: si `recargar.txt` cambió.
   - AUTO-SIEMPRE: si desde la última vez se vio una zona de juego (id > 7: Partido 8, Editar 13, Liga Máster 19…), o si llegó un aviso.
4. **Solo en el MOMENTO SEGURO** (guía 22): zona = TOP_MENU (7), sin cambio de modo en curso, gestor de edición `[exe+0x37F5C28]` = 0, gestor de la base en estado 4 (quieto). **Nunca en un partido ni en Editar.**
5. **Qué escribe:**
   - Primero comprueba los bytes de código del interruptor (`0xAEF770`, `0x1EFB440`) **antes de escribir nada**.
   - Luego hace lo mismo que la vieja tecla **P** (`recargaCompleta`, probada el 2026-10-09 a las 05:30): 1 byte de código `exe+0xAEF78E` 00→01 (con su propia comprobación de los 8 bytes de `0xAEF78A`; se hace una sola vez por sesión) y el interruptor `exe+0x37F5C39` 0→1.
   - Si el interruptor ya estaba en 1 (lo encendió el juego, p. ej. Ser una Leyenda), no escribe nada.
6. **Quién recarga de verdad:** el propio juego. Al entrar a un modo desde el menú principal ve el interruptor en 1, crea la tarea `editLoadDataInTopMenu` (la misma de Editar → Cargar) y lo pone a 0. Con el byte de la P, relee **EDIT + toda la base**. phoenix.lua nota el 1→0 y anota «recarga automática hecha».
7. **Seguridad:**
   - Bandera propia de errores en `display_frame`.
   - 3 fallos (error de Lua o interruptor que no se puede encender) → lo automático se **apaga hasta reiniciar el juego** y queda ACTIVAR.
   - Todo vive en la memoria: al cerrar el juego, desaparece.
8. **Overlay:** línea nueva `[MODO PHOENIX] <modo> · <de dónde salió la elección> · T = cambiar`, la explicación del modo y, en los automáticos, `[AUTO] <estado>`: esperando, pendiente (esperando el momento seguro), interruptor encendido HH:MM:SS o ✓ el juego hizo la recarga.
9. **sider.log:** líneas `[phoenix] v0.18 …` (como mucho 300): modo leído, tecla T, aviso de fichajes, interruptor encendido, recarga hecha, fallos.

---

## 4. CUÁNDO se aplica cada cosa (la línea de tiempo)

**Cambiar de modo (botón de Link, `.bat` o T):** el juego lo nota en **unos 2 segundos**, abierto o no (si está cerrado, al abrirlo).

**AUTO-FICHAJES, ejemplo:**
1. 10:00 · Estás en el menú principal. Phoenix Sync mete fichajes y cambia `recargar.txt`.
2. ~10:00:02 · phoenix.lua lo nota. Si es momento seguro, enciende el interruptor.
3. 10:01 · Entras a Partido → **el juego recarga** → ya juegas con los fichajes.

Si el aviso llega **mientras juegas un partido**, espera: cuando vuelves al menú principal enciende el interruptor, y la recarga ocurre al entrar al **siguiente** modo.

**AUTO-SIEMPRE, ejemplo:**
1. Abres el juego → menú → Partido: **no recarga** (la base es recién cargada).
2. Terminas → vuelves al menú → phoenix.lua enciende el interruptor.
3. Entras a Partido, Liga Máster o lo que sea → **recarga**.
4. Y así cada vuelta.

**ACTIVAR:** solo cuando pulsas Activar (recarga en el sitio, con el mensaje de éxito).

---

## 5. Lo que hay que comprobar en el juego (todavía no probado)

1. Que `display_frame` llegue también en el menú principal (si no, lo automático solo avanza con el overlay abierto o al leer archivos).
2. ACTIVAR: igual que la v0.17e (Activar, mensaje, fichajes).
3. AUTO-SIEMPRE con Partido: menú → Partido (no recarga) → menú → Partido (recarga; el log debe decir «interruptor encendido» y «recarga automática hecha»).
4. AUTO-FICHAJES: doble clic en `PhoenixRecargar.bat` en el menú → Partido → recarga. Sin aviso → no recarga.
5. **Qué equipos salen tras la recarga automática.** Es la misma recarga de la tecla P, que en las pruebas del 2026-10-09 trajo los fichajes del option file y las stats de la base. Hay que verlo con la lista **completa** de jugadores.
6. **Liga Máster: NO probar al principio.** Lo automático nunca escribe dentro de Liga Máster, pero sí puede dejar pedida una recarga para cuando vuelvas a entrar. Primero solo con Partido y sin guardar.

---

## 6. Archivos y huellas

| Archivo | Para qué | sha256 |
|---|---|---|
| `PhoenixSync/sider/pruebas/phoenix-v0.18-prueba.lua` | el módulo | `a39b54de9f6a579d539ecdc432e14bbc2b32b982c14f82d16e1f304651e4f361` |
| `PhoenixSync/sider/pruebas/simular_v018.py` | simulación (9 casos) | `5efb8b6f23294e3b1d449dd40709cd83939bfe3f37832f04dafb5aa2064d8b06` |
| `PhoenixSync/sider/PhoenixModo.bat` | elegir modo (1/2/3) mientras no hay botón en Link | `8e7002b662c2b0c4f17012e4f6aef262c8e0d42a254b13699ebf9529ec0ecb7b` |
| `PhoenixSync/sider/PhoenixRecargar.bat` | simular «hay fichajes nuevos» | `da68c792bd8336ab3f55aa5dab597c1a4d9667e891dfd0ffecac9d3cde58f7de` |

Los dos `.bat` van dentro de `content\phoenix\` y escriben en su propia carpeta (`%~dp0`), de forma atómica.

### Simulación (sin juego, LuaJIT con memoria, archivos y reloj falsos): 9 de 9 bien
- S1 sin `modo.txt` → ACTIVAR: tras el arranque, 0 escrituras y **0 lecturas de memoria** en 18 pasos.
- S2 AUTO-SIEMPRE: 0 escrituras en el primer menú y en Partido; al volver al menú, `AEF78E` + `37F5C39`; no repite; en la segunda vuelta solo `37F5C39`.
- S3 AUTO-FICHAJES: el aviso que ya había no cuenta; aviso nuevo en Partido → espera; menú con la base cargando → espera; momento seguro → enciende; sin aviso nuevo → nada más.
- S4 `modo.txt` raro → ACTIVAR; T cambia de modo; cuando Link reescribe `modo.txt`, manda `modo.txt` (también con BOM).
- S5 bytes del interruptor distintos → **0 escrituras**; a los 3 fallos se apaga lo automático y queda ACTIVAR.
- S6 nunca escribe en Partido, Editar (con gestor de edición vivo), Liga Máster ni cambiando de modo.
- S7 interruptor ya encendido por el juego → no escribe.
- S8 antes del arranque → no toca la memoria.
- S9 300 cuadros en el mismo segundo = un solo paso; tras Shift+R funciona.
- Además pasan con la v0.18 las simulaciones viejas: v0.17m, v0.17a, v0.17d (7 de 7), v0.17e (teclas de prueba ignoradas) y la del experimento B (la v0.18 aplica A+B+C como la v0.17e).

---

## 7. Lo que falta para que el botón exista en Phoenix Link

Link es **el otro frente** (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`), así que desde aquí no se toca. Ya quedó:
- El **contrato** de `modo.txt` y `recargar.txt` en `COORDINACION.md` (sección «Contratos compartidos»).
- Un **prompt** para el chat de Link: `PhoenixSync/prompts/PROMPT-LINK-modo-recarga.md` (botón de tres opciones, con ACTIVAR por defecto y los textos de cada una).

Phoenix Sync escribirá `recargar.txt` cuando se conecte la entrega de fichajes (pendiente en este frente).
