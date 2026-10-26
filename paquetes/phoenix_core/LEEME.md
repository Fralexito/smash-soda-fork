# Phoenix Core · v0.2.0 (fase 0 + sonda)

DLL propia que se carga dentro de PES 2021 desde Sider. En esta fase **no cambia nada del juego**: solo arranca, se identifica y deja registro. Es la base donde irán las funciones on/off (la primera: el gol del modo street en Entrenamiento libre).

Plan completo: `para-proyecto/pes/cap_70_phoenix_core_arquitectura_y_plan.md`.

## Qué trae

| Archivo | Para qué |
|---|---|
| `src/phoenix_core.cpp`, `src/phoenix_api.h` | Código de la DLL y su API en C |
| `CMakeLists.txt` | Compilar con MSVC (x64, /MT) |
| `SiderAddons/modules/phoenix_core_loader.lua` | Módulo de Sider que carga la DLL (LoadLibraryA por ffi) y llama a `phx_core_tick` cada fotograma |
| `SiderAddons/phoenix_core/phoenix_core.json` | Funciones encendidas/apagadas |
| `INSTALAR.ps1` / `DESINSTALAR.ps1` | Poner y quitar sin Phoenix Link |

## Compilar (Windows, Visual Studio con C++)

```
cmake -S . -B build -A x64
cmake --build build --config Release
copy build\Release\phoenix_core.dll SiderAddons\phoenix_core\
```

## Seguridad

- **Interruptor general:** si existe `SiderAddons\phoenix_core\PHOENIX_CORE_OFF` (vacío), Core no hace nada.
- **Modo seguro automático:** al arrancar crea `arrancando.flag` y la borra a los 60 s estables. Si PES se cerró antes, el siguiente arranque va en modo seguro (todas las funciones apagadas).
- **Firmas verificadas:** cada parche busca su patrón de bytes y comprueba los bytes originales; si el exe es otro, la función queda apagada y lo dice en el registro.
- **Fallos:** cada función corre protegida; a los 3 fallos se apaga sola.
- Al cerrar PES se restauran los bytes originales. El exe en disco nunca se toca.

## Comprobar que cargó

- `SiderAddons\phoenix_core\cargador.txt`: lo que vio el módulo de Sider.
- `SiderAddons\phoenix_core\phoenix_core.log`: versión, identidad del exe y estado de cada función.
- `SiderAddons\phoenix_core\phoenix_core_estado.json`: lo mismo en JSON (lo leerá Phoenix Link).

## Funciones (fase 0)

| id | Qué hace | Por defecto |
|---|---|---|
| `diagnostico` | Busca `FREE_TRAINING_BREAK` en el exe, solo lectura | encendida |
| `gol_nativo_free` | Gol nativo en Entrenamiento libre | apagada (sin firma todavía) |
| `sonda_partido` | Solo mira: desvía SetState (0x1406EEBB0) y el procesador del gol (0x140478190) y anota en `sonda_partido.txt` cada cambio de estado, cada gol procesado (goles, escena job+0x874, fase) y el indicador «partido detenido», con quién llamó | encendida |

Los desvíos son propios (sin MinHook): copian el prólogo conocido (15 y 14 bytes, sin instrucciones relativas a RIP, leídos con Ghidra el 26-oct) a un trampolín y escriben un `jmp` absoluto de 14 bytes. Antes de escribir se comprueba la firma (única en `.text`) y los bytes originales; al cerrar PES se restauran.
