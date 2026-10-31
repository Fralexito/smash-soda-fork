# Guía REA para Phoenix v2 · cómo usarlo completo (no solo Ghidra)
> **v2:** base = Conmegol Patch sobre PES 2021 exe 1.07.02 (FL27 no aplica). Se conserva la v1.
Para la cuenta de Claude Code en la PC de FRALEX. Fuente: repo `morluto/rea` (MIT), docs y skill `reverse-engineer-anything` v55.

## El problema
REA **no es Ghidra**. Ghidra es solo uno de sus "motores" para `.exe` nativos. REA trae **más de 30 tipos de análisis**. Si solo se usa Ghidra, se pierde la mayoría.

## Regla 1: elegir la herramienta según QUÉ es el programa
Antes de abrir Ghidra, siempre primero:

| Paso | Herramienta REA | Para qué |
|---|---|---|
| 1 | `inspect_desktop_application` | Darle la **carpeta** del programa. Dice en qué está hecho (.NET, Python, Electron, nativo C++) y qué analizar después |
| 2 | `inspect_pe_imports` | Qué DLLs y funciones de Windows usa (ej. `ReadProcessMemory` = edita memoria; `CryptDecrypt` = cifra) |
| 3 | `inspect_pe_load_config` / `inspect_pe_certificates` | Datos extra del .exe y quién lo firmó |

Según el resultado del paso 1:

| Si es… | Usar | NO usar |
|---|---|---|
| **.NET / C#** (muchas herramientas de PES: Devil Cold52, varios editores) | `inspect_managed_artifact`, `inspect_managed_members`, `inspect_managed_native_boundaries`, `compare_managed_members` | Ghidra (da basura con .NET) |
| **Python empaquetado (PyInstaller)** (ej. Touchline y otras apps Python) | `inspect_artifact` + `extract_artifact` (saca los scripts), luego guía Python `.pyc` | Ghidra |
| **Electron / JavaScript** (apps de escritorio web) | flujo JavaScript/Electron (módulos, IPC, source maps) | Ghidra |
| **Nativo C/C++** (PES2021.exe, Sider, Live Editor) | Ghidra / IDA / Hopper: pseudocódigo, strings, referencias | — |
| **Página web** (My Football Legacy, ML Manager Pro, TEditor) | `browser-observation`, `analyze_web_bundle`, `export_web_scripts` | Ghidra |
| **Base de datos SQLite** (apps que guardan datos locales) | análisis SQLite (tablas, columnas, filas) | — |
| **Captura de red** (.har del navegador) | `inspect_web_network_capture` (ver la API que usa una web) | — |

## Regla 2: herramientas que casi nadie usa y le sirven mucho a Phoenix
| Herramienta | Uso en Phoenix |
|---|---|
| **`match_native_functions`** (comparar funciones entre 2 versiones de un .exe) | Trasladar direcciones si el exe cambia (copia distinta de un amigo, actualización de Conmegol, tu parche futuro) |
| **Frida** (`start_frida_session`, `load_frida_script`) | Observar el juego **mientras corre**: ver qué función se llama al presionar R3 o "Activar datos en vivo". Así sabremos por qué la recarga a veces falla. Mejor que leer memoria a ciegas |
| **`trace_native_ui_action` / `trace_native_values`** | Qué código corre al presionar un botón del menú y de dónde sale cada valor (mandos) |
| **`inspect_packet_capture`** | Entender el protocolo online de Phoenix Online desde el tráfico grabado |
| **`compare_managed_members`** | Comparar 2 versiones de una herramienta .NET y ver qué cambió |
| **`evaluate_reconstruction_coverage`** + ledgers | Al reescribir algo (sala limpia), comprobar que **cubrimos todo** lo que hacía el original |
| **`get_evidence_bundle`** | Guardar las pruebas de cada hallazgo para documentarlas en Drive |

Ojo: la captura de procesos (`process capture`) **no funciona en Windows nativo**; Frida sí.

## Regla 3: método "sala limpia" en cada análisis
1. Analizar con la herramienta correcta (tabla de arriba).
2. Escribir en un `.md` **el comportamiento** (formato, pasos, offsets) con la evidencia de REA.
3. Reescribir desde cero en el código de Phoenix. Nunca pegar código descompilado.
4. Verificar cobertura con `evaluate_reconstruction_coverage`.
5. Subir el `.md` a Drive como archivo nuevo.

## Regla 4: código abierto primero
Si la herramienta tiene repo (FL Daily Edit, pesXdecrypter, CareerLab, Pes Editor Rebuilt…), **leer el código directo**, sin REA. REA es para lo cerrado.

## Plan de uso concreto (en orden)
| # | Objetivo | Herramienta REA |
|---|---|---|
| 1 | Por qué la recarga R3 a veces falla | **Frida** sobre PES2021.exe en el menú: rastrear qué lee al recargar (EDIT vs PlayerAssignment.bin) |
| 2 | Firmas para las direcciones de phoenix.lua y mandos | `read_bytes` + `search_bytes` (patrón único) |
| 3 | Cómo editan el EDIT las herramientas de Devil Cold52 | `inspect_desktop_application` → si es .NET, `inspect_managed_members` |
| 4 | Cómo funciona Touchline por dentro | si viene empaquetado: `extract_artifact`; sus Lua se leen directo |
| 5 | Cómo funciona My Football Legacy (resultados con IA) | `browser-observation` + captura .har → `inspect_web_network_capture` |
| 6 | Cómo el Live Editor mueve datos en memoria | Ghidra (nativo) + `inspect_pe_imports` |
| 7 | Mandos y menús | `trace_native_ui_action`, `trace_native_values`, Frida, firmas |
| 8 | Phoenix Online: error tras elegir país | `trace_crash`, Frida en 140fe2920 / 140fe2fb0, `inspect_packet_capture` |

## Si REA falla
- Herramientas que no aparecen → reiniciar Claude Code; si sigue, `rea doctor`.
- No repetir `rea setup` por cualquier error (la skill lo prohíbe).
- Archivos grandes en Ghidra → subir `REA_GHIDRA_STARTUP_TIMEOUT_MS`.
- Leer `references/connection-and-recovery.md` de la skill antes de un segundo intento.

**Regla de oro:** nada se cambia en Phoenix, GitHub o Drive sin aprobación de FRALEX. REA es solo para **estudiar**.
