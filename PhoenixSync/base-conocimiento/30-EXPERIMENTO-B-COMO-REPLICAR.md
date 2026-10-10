# 30 · Experimento B (phoenix.lua v0.17e-B) — cómo replicarlo desde cero

Escrito el 2026-10-10 a las 09:55 (Lima), a pedido de FRALEX: «guarda perfectamente cómo se llega al resultado de la v0.17e-B para que otra IA lo replique si es necesario».

Esta guía está pensada para que **otra persona u otra IA** pueda reconstruir la v0.17e-B y obtener el mismo resultado sin haber visto la conversación.

---

## 1. Qué es, en una frase

La v0.17e-B es la v0.17e de phoenix.lua con **un solo cambio**: al arrancar el juego aplica dos de los tres parches del botón nativo «Datos Actual. en vivo» (A y B). El tercero (C) **no lo aplica**.

Resultado: al pulsar **Activar**, el juego carga los equipos de la base de Phoenix-DB **y se queda de verdad en «Activar»**. La pantalla muestra «Selección actual: Activar Actualización en vivo». Con la v0.17e (A+B+C), en cambio, siempre volvía a «Desactivar».

---

## 2. Requisitos (si algo no coincide, el resultado puede ser otro)

| Cosa | Valor exacto |
|---|---|
| Juego | PES 2021 (eFootball PES 2021 SEASON UPDATE), `PES2021.exe` sha256 `5e27a782…bc224a` |
| Dirección base del exe | `0x140000000` (las direcciones de abajo son relativas: `exe+…`) |
| Sider | 7.3.3, con módulos Lua (LuaJIT, **sin `pcall`**) |
| Archivo de partida | `sider/pruebas/phoenix-v0.17e-prueba.lua`, sha256 `114b6f2ee9d8ac64465a62549f23879179e125fc4178096295a64df8dc17c069` |
| Archivo final | `sider/pruebas/phoenix-v0.17e-B-prueba.lua`, sha256 `e276c72e7cb133c177db9c439501dc772591b32bfb689bb75c78cf870279a811` |
| Simulación | `sider/pruebas/simular_expB.py`, sha256 `dce0e5252769504dbe6e19696a0ce5e7b95ccbcb7182732eecf75878502f2952` |

Todos los parches se hacen **solo en la memoria** del juego. El `.exe` del disco nunca se toca. Al cerrar el juego, todo desaparece.

---

## 3. Los tres parches del botón nativo (para entender qué se quita)

Están en la tabla `PARCHES16` de phoenix.lua (función `botonEnElSitio`). Se aplican solos la primera vez que el juego lee un archivo de la base (`livecpk_read`, variable `autoHecho`).

Antes de escribir nada, phoenix.lua comprueba que **los tres** sitios tienen los bytes esperados. Si uno no coincide, no escribe nada.

### Parche A — saltar el inicio de sesión en Konami
- Sitio: `exe+0x20AF73B` (flujo común `ProcessCmnLiveDataSetFlow`, estado 1).
- Bytes originales aceptados:
  - `48 83 bf 98 00 00 00 00 75 6b 33 db 89 5c 24 28 c7 44 24` (juego limpio), o
  - `c6 05 f7 64 74 01 01 c7 87 94 00 00 00 07 00 00 00 eb 6c` (parche viejo de la v0.15).
- Bytes nuevos: `c7 87 94 00 00 00 03 00 00 00 eb 73` → `mov dword [rdi+0x94], 3 ; jmp fin`.
- Efecto: va directo a crear `LiveDataSetFlow` sin pedir inicio de sesión (los servidores de Konami están cerrados).

### Parche B — saltar la petición al servidor y mostrar «éxito»
- Sitio: `exe+0x20AC6B9` (`LiveDataSetFlow`, estado 5).
- Original: `e8 52 e2 09 ff 48 85 c0 74 10 48 8d 15 16 53`.
- Nuevo: `c7 87 94 00 00 00 16 00 00 00 e9 ba 25 00 00` → `mov dword [rdi+0x94], 0x16 ; jmp 0x20AEC82`.
- Efecto: sin internet, salta al diálogo de éxito (estado 22, mensaje `0xF90042`). En ese paso el juego recarga la base en **modo 1** (activado).

### Parche C — el que el experimento B NO aplica
- Sitio: `exe+0x20AE664` (`LiveDataSetFlow`, estado 26, «editLoadDataInLiveDataSet»).
- Original: `66 c7 44 24 30 00 01` → `mov word [rsp+0x30], 0x100`.
- Nuevo (solo en la v0.17e): `66 c7 44 24 30 01 01` → `0x101`.
- Efecto con C: la carga del estado 26 también relee la **base**, pero en **modo 0**. Por eso, con A+B+C, Activar hace **dos** recargas (modo 1 y luego modo 0) y el juego termina diciendo «Desactivar».
- Sin C: el estado 26 carga solo el option file (`0x100`), sin base. **Una sola** recarga, y el modo se queda en 1.

---

## 4. Paso a paso para construir la v0.17e-B desde la v0.17e

Son exactamente 3 cambios (el `diff` completo tiene 25 líneas):

**Paso 1.** Cambiar la versión (línea 21):
```lua
local m = { version = "0.17e-B" }
```

**Paso 2.** Justo **antes** de `local function botonEnElSitio()`, añadir el interruptor del experimento:
```lua
-- EXPERIMENTO B: aplicar SOLO los parches A y B. El C no se escribe (sus bytes sí se comprueban).
-- Para volver al comportamiento de la v0.17e: poner false.
local EXPERIMENTO_B = true
```

**Paso 3.** Dentro de `botonEnElSitio()`, en el bucle «2) escribir y verificar», sustituir la línea
```lua
        if leerSeguro(base + pt.rva, #pt.nuevo) ~= pt.nuevo then
```
por
```lua
        if EXPERIMENTO_B and pt.nombre == "C" then
            if leerSeguro(base + pt.rva, #pt.nuevo) == pt.nuevo then
                log("[phoenix] experimento B: el parche C YA estaba puesto en esta sesión; no se quita (cerrar y abrir el juego)")
            else
                log("[phoenix] experimento B: parche C NO aplicado (a propósito)")
            end
        elseif leerSeguro(base + pt.rva, #pt.nuevo) ~= pt.nuevo then
```
Al final de la función, añadir al texto de `estadoBoton`:
```lua
        .. (EXPERIMENTO_B and "  ·  EXPERIMENTO B: sin parche C" or "")
```

**Importante:** el bucle «1) comprobar TODO» **no se cambia**. Los bytes de C se siguen comprobando, porque sirven para confirmar que el exe es el esperado.

**Paso 4.** Comprobar que el sha256 del archivo resultante es `e276c72e…a811` (sección 2). Si no coincide, compara con `diff` contra el archivo del repo.

---

## 5. Comprobación sin juego (simulación)

`python3 sider/pruebas/simular_expB.py` (necesita `lupa` con LuaJIT). Ejecuta phoenix.lua con una memoria falsa y cuenta las lecturas y escrituras.

Debe dar:
- Con bytes originales: escribe **solo** en `0x20AF73B` (A) y `0x20AC6B9` (B). C queda igual.
- La v0.17e, en la misma situación, escribe los tres.
- Si C ya estaba puesto: no lo toca (no lo quita).
- Los 18 casos anteriores y la prueba de teclas siguen bien.

---

## 6. Instalación (como se hizo el 2026-10-10 a las 09:25)

1. **Cerrar el juego** (en `sider.log`, Sider escribe «All done»).
2. Hacer una copia de respaldo del `phoenix.lua` instalado en **las dos** carpetas `SiderAddons\modules\` del parche ConmeGol, con nombre `phoenix.lua.v017e`.
3. Copiar la v0.17e-B con el nombre `phoenix.lua` en las dos carpetas.
4. Comprobar el sha256 en el PC: debe ser `e276c72e…a811` en las dos.
5. Comprobar que el respaldo `.v017` (la versión estable) sigue intacto.

---

## 7. Prueba en el juego y resultado esperado

1. Abrir el juego. En `sider.log` deben salir: «parche A aplicado», «parche B aplicado» y «experimento B: parche C NO aplicado (a propósito)».
2. Partido → Datos Actual. en vivo → **Activar**. Debe salir el mensaje de éxito.
3. Volver a entrar a «Datos Actual. en vivo»: debe decir **«Selección actual: Activar Actualización en vivo»**.
4. Overlay de Sider (módulo Phoenix): `[MODO] opción en vivo: ACTIVADA (1) · gestor estado 4`, `[ÚLTIMA RECARGA] ACTIVAR`, `[BOTÓN] … EXPERIMENTO B: sin parche C`.
5. En `sider.log`: **una sola** relectura de la base (28 archivos) en modo 1.

### Resultado comprobado por FRALEX (2026-10-10 09:28–09:46), ya corregido

| Lo que pulsas | Equipos que salen | Qué dice la pantalla | Modo en memoria `[[exe+0x3705E10]+0x38]` |
|---|---|---|---|
| **Activar** | Base de Phoenix-DB (Barça con Vinícius y Mbappé; flechas de forma física) | «Activar» | 1 |
| **Editar** (solo entrar) | Option file `EDIT00000000` (Mbappé en el Barça, Vinícius en el Madrid). Relee 11 archivos | sigue diciendo «Activar» ⚠️ | sigue en 1 ⚠️ |
| **Desactivar** | Option file (Vinícius al Madrid; Mbappé en el Barça, de suplente). No relee archivos | «Desactivar» | 0 |

**Corrección registrada:** a las 09:42 se dijo que había una «tercera versión» del Barça. Era falso: Mbappé estaba más abajo en la lista. **Regla:** para saber si un jugador está en un equipo, mirar siempre la lista **completa**.

---

## 8. Problemas conocidos (por qué no es todavía la versión principal)

1. **Después de Editar, el modo dice 1 pero los equipos son del option file.** La «última recarga» de phoenix.lua (opción A, guía 19) se equivoca en ese caso. Posible arreglo, sin hacer todavía: detectar la relectura de 11 archivos de Editar, o la zona EDIT (13), y marcar NORMAL.
2. **Las stats nuevas sin C no están probadas.** Haría falta cambiar `Player.bin` de Phoenix-DB con el juego abierto y pulsar Activar.
3. **El Barça visitante** en partidos Barça–Barça: ❓ sin confirmar si es el del option file.

---

## 9. Cómo volver atrás

- **Rápido, sin tocar archivos:** cerrar el juego (los parches viven solo en la memoria). Si C ya se puso en una sesión, solo se quita cerrando el juego.
- **Al comportamiento de la v0.17e:** poner `EXPERIMENTO_B = false`, o instalar el respaldo `phoenix.lua.v017e` (sha256 `114b6f2e…`).
- **A la versión estable:** respaldo `phoenix.lua.v017` (sha256 `d85e1071…`), que es también `sider/phoenix.lua` del repo.

---

## 10. Dónde está cada prueba

- `PRUEBAS.md`: entradas del 2026-10-10 09:25, 09:26, 09:28–09:32, 09:32–09:42 y la corrección de 09:46.
- `REGISTRO.md`: líneas de las mismas horas.
- Guías relacionadas: 17 (dónde vive la opción), 19 (paso a paso y opción A), 22 (zona y momento seguro).
