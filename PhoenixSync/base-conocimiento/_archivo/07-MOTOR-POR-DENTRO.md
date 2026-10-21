# 07 · El motor por dentro: recargar sin «Editar → Cargar» y el botón nativo de actualización

**Estudio del 2026-10-09 (05:00–05:15, Lima)** sobre `PES2021.exe` del PC de Fralex (437 MB, versión final `PATCH10100`).
Herramientas: Python + capstone en la VM del PC, directamente sobre el exe. **No se modificó el exe.**
Ayudantes guardados en la VM: `~/re/xref.py`, `~/re/desm.py`, `~/re/quien.py`, `~/re/llamas.py`, `~/re/refsa.py`.

---

## 1. Cómo está hecho el ejecutable

| Sección | Tamaño | Qué es |
|---|---|---|
| `.trace` | 37 MB | **El código del juego, sin cifrar** (entropía 6,5, normal en código x64). Se puede leer y desensamblar. |
| `.impdata` | 380 MB | Capa de protección (tipo Denuvo). El punto de entrada está en `.sbss`, dentro de la capa. |
| `.rdata` | 14 MB | Textos y tablas: nombres de clases (RTTI), rutas del código fuente de Konami y mensajes de depuración. |
| `.pdata` | 1,8 MB | Tabla de funciones: **156 556 funciones** en `.trace` con su inicio y su fin. |

- El motor es **Fox Engine** (`FoxCore`), con Lua propio para escenas. **Los menús son clases C++** (`Process…`, `…Flow`, `…Functor`).
- Base de carga 0x140000000. Las direcciones de este documento son **RVA** (exe + RVA).

## 2. La fábrica de «cargar EDIT» (la que usa Editar → Cargar)

- **`0x1EFB1F0` = crear tarea de carga del option file** `(nombre, parámetros)`. Se usa en 7 sitios, entre ellos:

| Sitio | Nombre de la tarea | Cuándo |
|---|---|---|
| `0x130F280` | `editLoadData` | Menú Editar → **Cargar** |
| `0xAEF7A3` | `editLoadDataInTopMenu` | Al **volver al menú principal**, solo si el interruptor vale 1 |
| `0x20AE682` | `editLoadDataInLiveDataSet` | Último paso de la **actualización en vivo de Konami** |
| `0xCDD8F1` | `EditLoadForBLPlayerSave` | «Ser una Leyenda» |

- La tarea se cuelga de su padre con `0x149FC70(padre, tarea)`.
- Hay textos de proceso que confirman la relectura de la base: `ProcessEditDataLoad::CreateReloadPesdb`, `Edit/Load/EditDataLoadProcess`.

## 3. ⭐ El interruptor nativo: recargar al volver al menú principal

- **Byte `exe+0x37F5C39`** (zona de datos, empieza en 0).
- Lo lee `menu::ModeFlowCmnInitFunctor` (vtable `0x2684BB0`, función `0xAEF2E0`). Esa función se ejecuta al **salir de un modo al menú principal** (también llama a `CmdLeaveOfflineMode`):
  ```
  0xAEF770  cmp byte [exe+0x37F5C39], 0      ; 80 3D C2 64 D0 02 00
            je  seguir
            crear tarea "editLoadDataInTopMenu" (parámetros 0x100, igual que Cargar)
  0xAEF7CE  llamar 0x1EFB440 con cl=0        ; vuelve a poner el byte a 0
  ```
- **`0x1EFB440`** = `mov [exe+0x37F5C39], cl ; ret` (88 0D F3 A7 8F 01 C3). Lo llaman el menú principal (para ponerlo a 0) y `0xCDD820` (tras guardar en «Ser una Leyenda», lo pone a 1).
- **Idea:** escribir **1** en ese byte. La próxima vez que el jugador vuelva al menú principal, el juego recarga EDIT + base **solo**. Sin Editar → Cargar.
- **Prueba:** `phoenix.lua` v0.11, tecla **L**. Antes de escribir, comprueba los 7 bytes de `0xAEF770` y de `0x1EFB440` (si el exe fuera otro, no toca nada). Muestra el valor en el overlay y avisa cuando el juego lo consume (1 → 0). Simulado OK en LuaJIT con el entorno de Sider. **Resultado en el juego: ver PRUEBAS.md.**

### 3.1 Resultado de la prueba (05:08) y la relectura de la base
- ✅ El juego **consume el interruptor** al entrar a un modo (1 → 0), sin crash. **Recarga solo el EDIT, no la base** (el espía no ve lecturas de pesdb).
- Editar → Cargar hace dos cosas: la tarea del EDIT + **`0x1EF2FA0(gestor, 0)` = iniciar la relectura de la base**.
  - El gestor se obtiene con `0x1EF2250()` (y se crea con `0x1EEBBA0(1)` si falta). Así lo hace el proceso `0x13E4580`.
  - El gestor avanza la relectura cuadro a cuadro (`0x1F08820` → `0x125F310`, «cpk_dat/common/etc/pesdb/%s»). Listas de archivos por tipo (0–2) en `0x3529900`.
- El constructor de la tarea del EDIT (`0x1EFAE20`) está **virtualizado** por la protección: se puede usar, no leer.
- 🏆 **05:13 · probado:** option file original + L + entrar a Partido ⇒ Lamine volvió al Barça. **Fichajes en vivo sin Editar → Cargar.**
- **Consecuencia:** fichajes (option file) → interruptor ✅; stats (base) → falta disparar `0x1EF2FA0(gestor, 0)`.

### 3.2 🏆 La recarga completa (05:31): stats + fichajes sin Editar
- Editar → Cargar = proceso «ProcessEditDataLoad::CreateReloadPesdb» (tabla `.data 0x34DA048` → fábrica `0x130F220`, bandera `[+0x8C] = 1`) → tarea «editLoadData» con parámetros **1,1,0,0,0**.
- El menú principal (`0xAEF78A`: `mov dword [rsp+0x20], 0x100`) pasa **0,1,0,0,0** → recarga el EDIT sin la base.
- **Parche en memoria de 1 byte:** `exe+0xAEF78E` 00 → 01 (queda 0x101, igual que Cargar). No se toca el exe en disco. La página ya era RWX (0x40).
- **Probado:** tecla P (parche + interruptor) → entrar a Partido → el juego relee TODA la base (huella v95) → **Lamine Velocidad 95**. ✅
- `0x1EF2FA0` (tecla K) solo **lee** 4 archivos (tipo 0: Boots, Glove, Player, Country) y **no aplica**. Sirve para diagnóstico, no para el producto.

## 4. La actualización en vivo de Konami por dentro (`LiveDataSetFlow`)

- **Se crea con** `0x20AC030(nombre, modo, 1, 2)`. Por ejemplo, desde Editar → Gestión de datos → Actualización en vivo (`0x13AB293`, «ProcessEditLiveUpdate_LiveDataSetFlow»).
- **Actualización:** `0x20AC3F0`. Máquina de **38 estados** en `[flujo+0x94]`, tabla de saltos en `0x20AECC0`:

| Estados | Qué hace |
|---|---|
| 0–4 | Preparar y comparar la versión (DLC VERSION) |
| **5** (`0x20AC6B9`) | **Pide la lista al servidor** (`CmdGetLivedataList`). Si falla → estado 30 (error) |
| 6–10 | Descargar archivos, comprobar el hash |
| 11–12 | Guardar |
| 13–18 | «Bind» (aplicar los datos) |
| 19–25 | Diálogos y resumen |
| **26 (0x1A)** (`0x20AE5FA`) | **`editLoadDataInLiveDataSet`: carga el EDIT** y termina (estado 27) |
| 30 (0x1E) | Diálogo de error |

- **Konami hacía exactamente lo que quiere Fralex:** descargar y luego **cargar el EDIT**.
- **Plan para el «botón nativo»:** al llegar al estado 5, saltar al 26 (sin red). Hay dos formas:
  - (a) un parche de 15 bytes al inicio del estado 5: `mov dword [rdi+0x94], 0x1A ; jmp 0x20AEC82`;
  - (b) hallar el objeto del flujo y cambiar su estado desde Lua.
- **Falta averiguar:** si antes del flujo hay una **puerta de inicio de sesión** (`ProcessEditLiveUpdateLogin`, `OnlineFlowLogin`) que corta con el mensaje «los servicios finalizaron el 25/08/2022». Habría que encontrar esa puerta y dejarla pasar.

### 4.1 🏆 La puerta y el botón nativo funcionando (05:46)
- **Ruta del botón** «Partido → Datos Actual. en vivo → Activar»: `Exhibition/LiveData/LiveDataSet` (`0x1308090`) → «ProcessCmnLiveDataSetFlow» (crea `0x1350E10`, actualización `0x20AF620`).
- **Estados del flujo común:**
  - 1 = crear «LiveDataLogin» (**la puerta**: inicio de sesión en Konami);
  - 2 = esperar. `0x20AF7D0` recibe el resultado; la decisión `0x1350E90` está **virtualizada**. Si falla, va al estado 5 = diálogo «servicios finalizados» + «No se han podido implementar…»;
  - 3 = crear LiveDataSetFlow;
  - 7 = terminar.
- **Parche de 19 B** en `exe+0x20AF73B` (inicio del estado 1): `mov byte [exe+0x37F5C39], 1 ; mov dword [rdi+0x94], 7 ; jmp 0x20AF7BA`. Original: `48 83 BF 98 00 00 00 00 75 6B 33 DB 89 5C 24 28 C7 44 24`.
- **Con el byte de la recarga completa** (`0xAEF78E` = 1): Activar → sin inicio de sesión → interruptor = 1 → al entrar a Partido el juego relee EDIT + toda la base. **Probado: Lamine 95 → 90 sin pulsar L ni P.**

### 4.2 🏆 En el sitio, con el mensaje nativo (v0.16, 05:53)
- **Parche A** (`0x20AF73B`): `mov [rdi+0x94], 3 ; jmp 0x20AF7BA` → sin inicio de sesión, crea LiveDataSetFlow.
- **Parche B** (`0x20AC6B9`, estado 5): `mov [rdi+0x94], 0x16 ; jmp 0x20AEC82` → sin internet, al diálogo nativo (0xF90042).
- **Parche C** (`0x20AE664`): `0x100 → 0x101` → la recarga del estado 26 incluye la base.
- **Probado:** Activar → diálogo nativo («No hay cambios… forma física», con flechas) → el juego relee toda la base y la aplica **sin salir de la pantalla** (Lamine 90 → 99).
- Los estados 0–4 pasaron sin problemas (la comparación de versión no dio error).
- **Pendiente:** en esta recarga, las plantillas del Atlético cambiaron. ¿En modo vivo manda la base sobre el option file? Hay que confirmarlo.

## 5. Viabilidad (honesta) tras el estudio

| Meta | Antes | Ahora | Por qué |
|---|---|---|---|
| Recargar sin entrar a Editar | ~35–50 % | **✅ PROBADO: fichajes (L) y stats + fichajes (P)** | El juego ya trae el interruptor; es 1 byte de datos |
| Que el botón «actualización en vivo» cargue nuestros datos | ~25–35 % | **✅ PROBADO (05:46)** | Ya tenemos el mapa completo del flujo; falta la puerta de inicio de sesión |
| Que la Liga Máster reciba stats en vivo | ? | Pendiente | Editar → Cargar no llega (prueba 04:53); falta la prueba de reinicio |

## 6. Reglas para seguir

1. Antes de escribir algo, **comprobar los bytes exactos del código** (versión del exe). Si no coinciden, no se toca nada.
2. Solo bytes de **datos** primero (como el interruptor). Parchear código va después y con el OK de Fralex.
3. Nunca durante un partido. Nunca pulsar «Guardar» en Editar durante las pruebas.
4. Todo con respaldo: `phoenix.lua.v010` en las dos carpetas `modules`.
