# REA completo · manual para sacarle el máximo (Phoenix)
Para la cuenta de Claude Code en la PC de FRALEX. Complementa `GUIA-REA-PHOENIX.md`.
Fuente: repo `morluto/rea` (MIT), carpeta `docs/` y skill `reverse-engineer-anything` v55. Revisado el 30-oct-2026.

---

## 0. Primero: diagnóstico (por qué solo usas Ghidra)
Hay 3 causas típicas. Revisarlas en este orden:

1. **Catálogo recortado.** Si REA se instaló con `--tool-families`, solo aparecen algunas familias de herramientas (por ejemplo, solo las nativas = Ghidra).
   - Ver en `~/.claude.json` → `mcpServers.rea.env` si existe `REA_MCP_TOOL_FAMILIES`.
   - Si existe y es corto, volver a correr `rea setup --client claude_code` con las familias necesarias (o sin recorte) y reiniciar Claude Code.
2. **No preguntar qué hay disponible.** Llamar `binary_session` al inicio: es la fuente oficial de qué herramientas y motores están activos.
3. **No usar las guías.** REA trae flujos guiados (sección 2) y un buscador de su propia documentación (sección 3). Úsalos antes de improvisar.

Además: Ghidra en Windows es **experimental** en REA (ver `docs/windows-ghidra-p0.md`). Para muchos casos de Phoenix hay herramientas mejores que no necesitan Ghidra.

---

## 1. Regla de oro: identificar → elegir → investigar → verificar
| Paso | Herramienta |
|---|---|
| Identificar qué es el programa | `inspect_desktop_application` (carpeta), `inspect_pe_imports`, `inspect_pe_resources`, `inspect_pe_load_config`, `inspect_pe_certificates`, `inspect_byte_entropy` (¿está comprimido o cifrado?) |
| Elegir la herramienta según el tipo | tabla de la sección 4 |
| Investigar una función concreta | flujo `investigate_feature` / `trace_feature` |
| Verificar que lo reescrito hace lo mismo | `verify_reconstruction`, `evaluate_reconstruction_coverage` |

---

## 2. Los 6 flujos guiados (úsalos siempre)
Se piden con la herramienta `guided_workflows` (`action: "list"` o `"render"`) o desde la terminal con `rea guided-workflows`.

| Flujo | Para qué | Uso en Phoenix |
|---|---|---|
| `investigate_feature` | Llevar una función desde "la veo en pantalla" hasta el código exacto | "¿Qué hace el botón Activar datos en vivo?" |
| `compare_application_versions` | Comparar 2 versiones de un programa | PES2021 vs FL27; versiones de herramientas |
| `verify_reconstruction` | Comprobar que nuestra versión reescrita cumple todo | Después de reescribir algo en Phoenix |
| `trace_crash` | Relacionar un cierre del juego con el código | Crashes por ReShade o por leer memoria |
| `audit_residual_unknowns` | Listar lo que todavía no se sabe | Cerrar huecos antes de programar |
| `prepare_bounded_process_capture` | Planear un experimento controlado antes de correrlo | Antes de probar algo en el juego |

---

## 3. El buscador de documentación interna
- `search_guidance` → buscar en las guías de REA por pregunta ("frida windows", "managed decompile").
- `read_guidance_section` → leer la sección completa.
- Terminal: `rea guidance --query "..."`.

**Regla:** ante cualquier duda de cómo usar REA, buscar aquí primero.

---

## 4. Catálogo completo por tipo de programa
⭐ = muy útil para Phoenix.

### 4.1 Programas nativos C/C++ (PES2021.exe, Sider, Live Editor)
| Herramienta | Qué hace |
|---|---|
| ⭐ `binary_overview` | Resumen del binario |
| ⭐ `analyze_function` / `procedure_pseudo_code` | Pseudocódigo de una función |
| `decompile_all` / `batch_decompile` | Descompilar muchas funciones |
| ⭐ `get_call_graph` / `trace_call_path` | Quién llama a quién |
| ⭐ `trace_native_values` | Seguir de dónde sale un valor (ej. el byte del toggle) |
| `trace_native_ui_action` / `authored_ui_dispatch` | Qué código corre al tocar un botón |
| `native_seed_call_trace` | Rastrear llamadas desde una función semilla |
| ⭐ `search_bytes` / `read_bytes` | Buscar y leer bytes (patrones / firmas) |
| `address_to_file_offset` | Pasar de dirección en memoria a posición en el archivo |
| `inspect_native_exports` / `inspect_native_api` / `inspect_native_dispatch_metadata` | Funciones exportadas y APIs usadas |
| `inspect_binary_layout` | Secciones del .exe |
| ⭐ `match_native_functions` | Emparejar funciones entre 2 versiones (**offsets FL26 → FL27**) |
| `inspect_with_objdump` / `execute_rizin_command` / `start_rizin_debug_session` | Motores alternativos (objdump, Rizin) |
| `start_gdb_session` / `gdb_console` | Depurador GDB |
| `cutter_command` | Motor Cutter |
| Motores | Ghidra, IDA, Hopper, Kuna (elegir con `native_decompiler`) |

### 4.2 Programas .NET / C# (muchas herramientas de PES)
| Herramienta | Qué hace |
|---|---|
| ⭐ `inspect_managed_artifact` | Identifica el programa .NET |
| ⭐ `inspect_managed_members` | Clases, métodos, código CIL, llamadas, campos |
| ⭐ `inspect_managed_native_boundaries` | Qué funciones nativas llama (ej. leer memoria) |
| `compare_managed_members` | Comparar 2 versiones |
| `verify_managed_native_boundaries` | Comprobar esas llamadas nativas |
| `import_managed_reconstruction` | Comparar código reescrito con el original |
| `project_managed_application_graph` | Mapa de la aplicación |
| Guía extra | `docs/dotnet-bundles.md` (programas .NET empaquetados en un solo .exe), `docs/ghidra-nativeaot.md` |

### 4.3 Python empaquetado (PyInstaller) y scripts
| Herramienta | Qué hace |
|---|---|
| ⭐ `inspect_artifact` / `extract_artifact` | Inventario y extracción del .exe de Python |
| ⭐ `inspect_python_pyc` | Leer los .pyc extraídos |

### 4.4 Apps web, JavaScript y Electron
| Herramienta | Qué hace |
|---|---|
| ⭐ `analyze_javascript_application` / `trace_javascript_semantics` | Analizar una app JS |
| `recover_javascript_sources` / `compare_source_to_bundle` | Recuperar el código original desde el bundle |
| ⭐ `inspect_web_page` / `observe_web_session` / `observe_web_execution` | Ver una página mientras funciona |
| ⭐ `capture_browser_scenario` / `capture_web_screenshot` / `compare_web_screenshots` | Grabar un recorrido y capturas |
| `analyze_web_bundle` / `export_web_scripts` / `trace_web_module_imports` / `trace_web_source_location` | Scripts de la web |
| `inspect_web_event_listeners` | Qué hace cada botón |
| ⭐ `inspect_web_network_capture` / `compare_web_captures` | Ver la API que usa una web (archivo .har) |
| `discover_webmcp_tools` | Herramientas WebMCP de una página |
| Electron: `list_electron_targets`, `inspect_electron_page`, `capture_electron_scenario`, `inspect_electron_fuses` | Apps de escritorio hechas con web |
| `export_environment_bundle` | Crear un "entorno de agente" de una web (copia para pruebas) |

### 4.5 En vivo (programa corriendo)
| Herramienta | Qué hace |
|---|---|
| ⭐ `list_frida_processes` / `start_frida_session` / `load_frida_script` / `resume_frida_session` / `frida_session_status` / `unload_frida_script` / `close_frida_session` | **Frida**: meter un script dentro del juego para ver qué funciones se llaman, con qué valores, en tiempo real. Funciona en Windows |
| `list_javascript_runtime_targets` / `observe_javascript_runtime` / `reconcile_javascript_runtime` | Node/Electron en vivo |
| `prepare_bounded_process_capture` | Planear captura (la captura de procesos NO funciona en Windows nativo) |
| `inspect_recorded_crash` / `trace_crash` | Analizar cierres |

### 4.6 Datos y archivos
| Herramienta | Qué hace |
|---|---|
| ⭐ `inspect_sqlite_database` | Tablas y filas de bases SQLite locales |
| `inspect_byte_entropy` | Detectar zonas cifradas o comprimidas (útil con el EDIT y los .cpk) |
| `open_bundle_resources` / `get_evidence_bundle` | Abrir recursos y juntar pruebas |
| `inspect_plist`, `inspect_keyed_archive`, `inspect_asset_catalog` | Formatos de Apple |
| `inspect_wasm_artifact`, `compare_wasm_interfaces` | WebAssembly |
| `inspect_model_artifact` | Modelos de IA (GGUF/safetensors) |
| `inspect_packet_capture` / `inspect_pcap_capture` / `inspect_packet_fields` | Capturas de red .pcap (ej. tráfico de Parsec, con tshark) |

### 4.7 Reescritura limpia (verificar que copiamos bien la función)
| Herramienta | Qué hace |
|---|---|
| ⭐ `build_reconstruction_obligation_ledger` | Lista de obligaciones: todo lo que el original hace |
| ⭐ `evaluate_reconstruction_coverage` | Qué porcentaje cubrimos |
| ⭐ `verify_reconstruction` | Veredicto final con pruebas |
| `audit_residual_unknowns` | Lo que falta saber |

### 4.8 Sesión y ajustes
| Herramienta | Qué hace |
|---|---|
| ⭐ `binary_session` | Qué herramientas y motores hay activos (llamar primero) |
| `server_health` | Estado del servidor |
| `inspect_analysis_settings` / `update_analysis_settings` | Ajustes del análisis |
| `inspect_analysis_view` | Resumen de resultados guardados |
| `idb_close` / `close_binary` | Cerrar análisis |

### 4.9 Otros (existen, poco uso para Phoenix)
Android (`inspect_android_*`, ADB, JADX, JEB, Apktool, Flutter), Apple (IPS, Objective-C), firmware, Go, Rust, GPU, Erlang, EVM, kernel Linux, Godot, Unity IL2CPP, señales lógicas. Útiles si algún día Phoenix tiene app móvil.

---

## 5. Recetas listas para Phoenix (copiar y pedir)
1. **Recarga R3 que falla**
   "Con REA: `binary_session`, luego flujo `prepare_bounded_process_capture` y Frida sobre PES2021.exe en el menú principal. Engancha las lecturas de EDIT00000000 y PlayerAssignment.bin al presionar R3 / Activar. Solo observar."
2. **Offsets para FL27**
   "Flujo `compare_application_versions` entre el exe actual y FL_2027.exe; luego `match_native_functions` para las funciones de phoenix.lua (toggle `0x37F5C39`, parche `0xAEF78E`, botón Activar)."
3. **Cómo edita el EDIT una herramienta cerrada**
   "`inspect_desktop_application` sobre su carpeta; si es .NET → `inspect_managed_members` y `inspect_managed_native_boundaries`; si es nativo → `investigate_feature`."
4. **App Python (ej. Touchline)**
   "`inspect_artifact` → `extract_artifact` → `inspect_python_pyc`."
5. **Web competidora (My Football Legacy)**
   "`capture_browser_scenario` del flujo de subir resultado + `inspect_web_network_capture` del .har; explica la API que usa."
6. **Reescribir algo en Phoenix**
   "`build_reconstruction_obligation_ledger` del original → programar → `evaluate_reconstruction_coverage` → `verify_reconstruction`."

---

## 6. Terminal (CLI) y variables útiles
- Todo lo de MCP existe en terminal: mismo nombre con guiones (`rea inspect-managed-artifact`, `rea guided-workflows`, `rea guidance`).
- `rea doctor` → revisar la instalación (solo si REA no arranca).
- `REA_GHIDRA_STARTUP_TIMEOUT_MS` → subir el tiempo para binarios grandes como PES2021.exe.
- `REA_MCP_TOOL_FAMILIES` → qué familias están visibles (ver sección 0).
- `REA_MCP_MAX_RESPONSE_BYTES` → tamaño máximo de respuesta.
- `REA_MCP_OUTPUT_SCHEMA_PROFILE` = `compact` → si el cliente rechaza el catálogo por grande.

## 7. Errores típicos
- Herramientas que no aparecen → reiniciar Claude Code; si sigue, `rea doctor`.
- **No repetir `rea setup` por cualquier error** (la skill lo prohíbe).
- Si una llamada se corta por tiempo, no repetirla igual: leer la guía de recuperación de la skill.
- Si la conexión cae, reintentar una sola vez; si vuelve a fallar, avisar a FRALEX y reiniciar.

---
**Reglas de Phoenix:** REA es para **estudiar**. Copiar el comportamiento, nunca el código descompilado. Nada se cambia en Phoenix, GitHub o Drive sin aprobación de FRALEX. Documentar cada hallazgo en Drive como archivo nuevo.
