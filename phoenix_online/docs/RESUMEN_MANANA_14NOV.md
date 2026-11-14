# Resumen para la mañana — 14-nov-2026 (hilo online, solo PES, un jugador)

## Hecho hoy (todo subido a GitHub rama claude/project-thread-9ciw66, TABLERO y Drive)
1. **Ranking semanal con datos reales: FUNCIONA en PES.** myClub → Sede → Estadíst. (tecla A ×3) → «eFootball League semanal» → Participar → «Clasif. general» con FRALEX 7 puntos. Interruptor `myclub.ranking_semanal_real` (ahora activado por defecto) y `is_beginner` «NO» por defecto (commit cb05d97; el servidor ya corre ese código, reiniciado esta noche).
2. **Entrenador de experiencia y ENTRENADOR ESPECIAL probados en vivo**: aparecen en la lista («Puntos de experiencia 100»), vista previa correcta, confirmación «¿Seguro?», `CMD_USE_MYCLUB_STAFF` NOERR, Ter Stegen sube de nivel.
3. **Diálogo «usar objeto»** localizado con Ghidra (FUN_140d4fc20; padres FUN_1413cc180/1413cea00).
4. **Causa de la fuga de memoria de Windows (AppXSvc)**: el plugin `ecc` (continuous-learning-v2) lanza `python3` (alias de la Store) en cada llamada a herramienta. Arreglo (tuyo, 1 minuto): Configuración → Aplicaciones → Configuración avanzada → Alias de ejecución de aplicaciones → apagar python.exe y python3.exe. Revertir = volver a encenderlos.
5. **Método seguro probado**: depurador propio con 1 solo registro (script `dbg_vigila.py`): PES no cayó y DR7 quedó limpio en 22 hilos.

## Pendiente (nada crítico)
- Cabecera «Puntos de experiencia» de «Resultados del entrenamiento» sale 0 (cosmético). Resultado del depurador: `exp_added` llega bien a `+0xc0` del objeto respuesta, que se inserta en un `std::map` (FUN_141f8bad0) y NO vuelve a leerse en los 43 s siguientes → la cabecera sale de otro dato. A probar: rellenar `traning_result`/diferencias de `parameters` o comparar con la respuesta del entrenador normal.
- 19 índices de `result_parameters` sin resolver (banderas de 1 bit de habilidades): solo afectan a «Habilidades de jugador». Lista en el tablero (tanda 16).
- Dorsales y entrenador por plantilla: sin probar.
- Arranque lento de PES: causa sin confirmar; falta tu SÍ para probar sin servidor Phoenix y sin la regla de afinidad de Process Lasso. Hubo 1 caída (02:20, offset 0x18c610fe) en reposo.
- x64dbg MCP sigue sin conectar (ECONNREFUSED): abrir x64dbg con PES y /mcp → Reconnect.

## Datos de prueba que dejé en tu club (hay copias de la base)
- `objetos_especial` = 3 (ahora 2 tras la prueba) y 2 entrenadores de prueba (FABINHO, K. LAIMER). Copias: `phoenix_online.antes_especial2_14nov.db`.
