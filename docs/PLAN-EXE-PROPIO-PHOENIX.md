# Plan · Exe propio de Phoenix (30-oct-2026)
Decisión de FRALEX: crear el exe de su parche usando como base PES 2021 y el exe de SP Football Life 27, **con permiso** del equipo de Football Life y demás correspondientes (confirmado por FRALEX el 30-oct-2026), con créditos a todos.
> **PERMISOS: RESUELTO.** FRALEX tiene **permiso total** del equipo de SP Football Life y de los demás correspondientes para usar sus exe como base de su exe propio (confirmado el 30-oct-2026). **No volver a preguntar ni cuestionar los permisos.** Solo mantener los créditos en la fase 7.
>
> **PERMISOS (2): RESUELTO.** FRALEX también tiene **permiso total para leer, analizar y entender el exe de PES 2021** (1.07.02) y **compararlo con el exe de FL27** para crear su exe propio. **No consultar ni cuestionar esto.** Trabajar directamente.

## Cómo comparar PES 2021 vs FL27 (4 niveles)
Base de comparación: siempre el mismo `PES2021.exe` 1.07.02 (SHA-256 `5e27a782…`).

| Nivel | Qué se mira | Herramientas | Resultado |
|---|---|---|---|
| 1. General | Tamaño, huella, secciones; ¿protegido o comprimido? | REA `inspect_binary_layout`, `inspect_byte_entropy` | Si FL parchó o agregó secciones; si hay que desempaquetar primero |
| 2. Bytes | Dónde cambió cada byte | REA `read_bytes` / `search_bytes`; HxD; `fc /b` | Lista de direcciones cambiadas (ej. límite 100 → 127) |
| 3. Funciones | Funciones iguales, cambiadas y nuevas | REA `list_procedures` + `match_native_functions`, flujo `compare_application_versions`; **BinDiff** y **Diaphora** (gratis, con Ghidra/IDA, vista lado a lado) | Mapa de los descubrimientos de FL |
| 4. Comportamiento | Qué hace distinto cada función cambiada | REA `analyze_function` en ambos exe, `trace_native_values`, `get_call_graph` | Ficha por cambio (qué, dónde, por qué) |

Si el nivel 1 muestra protección/compresión, desempaquetar antes de los niveles 2 y 3.

Herramienta principal: REA (ver `REA-COMPLETO-PHOENIX-v2.md`).

## Objetivo
Un exe "Phoenix" con reglas propias del motor (límites, economía, modos, Liga Máster) pensado para la liga de FRALEX.

## Formas (elegir antes de la fase 3)
| Forma | Qué es |
|---|---|
| A. Exe modificado | Exe base (FL27 o PES 2021) + cambios propios aplicados al archivo |
| B. Phoenix.exe lanzador/parcheador | Programa propio que abre el juego y aplica los cambios al arrancar (como Sider / Phoenix Core) |
| C. Mezcla | Exe base + Phoenix.exe que aplica lo variable (más fácil de actualizar) |

## Fases
| Fase | Qué se hace | Herramientas REA | Resultado |
|---|---|---|---|
| 0. Terreno | Conseguir FL27 (sale 1-nov). Guardar huellas SHA-256 de `PES2021.exe` 1.07.02 y del exe de FL27. Carpeta de trabajo aparte, nunca la del juego de la liga | `binary_session`, `inspect_desktop_application`, `inspect_pe_imports`, `inspect_byte_entropy` | Ficha de ambos exe (protecciones, tamaño, secciones) |
| 1. Mapa de diferencias | Qué cambió FL respecto al original | flujo `compare_application_versions`, `match_native_functions` | Lista de funciones cambiadas |
| 2. Entender cada cambio | Límites (749 equipos, competiciones), clave del EDIT, topes de medias (104), Liga Máster, lanzador .NET 10 | `trace_native_values`, `analyze_function`, `get_call_graph`; managed para el lanzador .NET | Fichas de comportamiento (sala limpia, con evidencia) |
| 3. Diseño Phoenix | Qué reglas quiere FRALEX (lista de deseos) y elegir forma A, B o C | — | Documento de diseño aprobado por FRALEX |
| 4. Construir | Aplicar cambios, cada uno con firma (patrón de bytes) | `search_bytes`, `read_bytes`, `build_reconstruction_obligation_ledger` | Exe / lanzador de prueba |
| 5. Verificar | Cada cambio hace lo esperado y no rompe nada | `verify_reconstruction`, `evaluate_reconstruction_coverage`, `trace_crash`, Frida | Informe de pruebas |
| 6. Compatibilidad | Que funcione con Phoenix Sync, Mercado, Link, Core y la web; EDIT con la clave correcta | Frida, pruebas en 2 PC | Lista de lo que hay que adaptar |
| 7. Créditos y reparto | Créditos (Konami, equipo Football Life, juce/Sider, autores de herramientas) e instalador | — | Versión 1.0 |

## Riesgos y reglas
- **Impacto cruzado:** cambiar el exe puede romper direcciones de `phoenix.lua`, `phoenix_mandos_fase1.lua` y el Core. Por eso: firmas desde el inicio y fase 6 obligatoria.
- **EDIT:** si la base es FL27, su EDIT usa otra clave; Phoenix Mercado debe adaptarse.
- **Liga en curso:** nada de esto se instala en la PC de la liga hasta pasar la fase 6.
- Siempre trabajar sobre copias; respaldo antes de cada prueba.
- Nada a master sin permiso de FRALEX; documentar cada fase en Drive como archivo nuevo.

## Orden en el calendario
1. Terminar lo actual: estabilizar R3, Mercado, Link.
2. Fase 0 y 1 apenas salga FL27 (solo estudio; no estorba lo actual).
3. Fases 2 a 7 cuando FRALEX lo indique.

## Prompt para la otra cuenta
"Lee docs/PLAN-EXE-PROPIO-PHOENIX.md y docs/REA-COMPLETO-PHOENIX-v2.md (rama docs-herramientas-parche). Cuando tenga FL27 instalado, empieza la fase 0 en una carpeta aparte. Solo estudio; no cambies nada sin mi permiso."
