# Resultado Escalón 1 — botón «Phoenix» en el menú principal (10-nov-2026)

## Estado: MECANISMO CONFIRMADO. Un evento de TopMenuNew.json remapeado desde livecpk lleva desde el menú principal de Conmegol a la selección de equipos de Entrenamiento libre. El hueco 14 NO es un botón visible; los botones visibles están en la pestaña «Ajustes».

## Resultado de la prueba final (10-nov, 21:5x)
- Pestaña **Ajustes** (arriba, tercera) abre una lista cuyos elementos son exactamente eventos de TopMenuNew: Editar (`17_edit`), Aplicar nueva Actualización (`23_liveUpdate`), **Opciones en línea (`20_onlinesettings`)**, Opciones del sistema (`21_systemsettings`), Opciones (`22_option`)…
- Con `20_onlinesettings` → `Match/TrainingDirectInit` y `TrainingDirectInit` → **`Match/MatchTrainingTeamSel`**, pulsar «Opciones en línea» abre la pantalla Local/Visitante (Entrenamiento libre). PES respondió, sin cierre. (La sonda sigue diciendo zona ONLINE_SETTINGS: la zona se decide por el id del evento, no por la pantalla destino.)
- Archivo de esa variante: `Phoenix-Menu\...\TopMenu\TopMenuNew.json.e1_variante_prueba_20_21` (también remapea 21).

## Errores / intentos fallidos
1. Primer arranque en el base `C:\PES21` y abierto con `PES2021.exe` directo → sin Sider (no cargó nada). Frank corrigió: el botón va en Conmegol.
2. Remapear 08/11 (la lista «Partida rápida / Divisiones en línea / Cooperativo…» del mosaico Online) y pulsar «Partida rápida»: PES se fue al flujo ONLINE real (zona ONLINE_QUICK) y quedó colgado en pantalla azul sin conexión; sin recarga en caliente los cambios del paquete NO se aplican (hay que reiniciar PES). Hubo que cerrar y reabrir. Cuidado: no probar con ítems de la lista online.
3. `TrainingDirectInit` → `Match/TrainingMatchInit` (como pedía el plan): PES se cerró al entrar. Con `Match/MatchTrainingTeamSel` (la ruta oficial de `TrainingUgcCheckJoinMode`) funciona. **Usar MatchTrainingTeamSel.**
4. El MCP rechaza «Escape»: la tecla se llama `Esc`.

## Estado final dejado en Conmegol
- `Phoenix-Menu` activo en `sider.ini`, con SOLO el cambio del hueco 14 (invisible, inofensivo) y `TrainingDirectInit` → `MatchTrainingTeamSel`.
- Para tener un botón real hay que elegir un evento visible. Recomendación: usar **`20_onlinesettings`** («Opciones en línea», inútil en offline) y cambiarle el texto a «Phoenix» (el texto viene del archivo de idioma/UI, no del flow: pendiente de localizar). Alternativa: `13_multilobby`/`14_onlinewatch` solo si se localiza dónde crear el mosaico.
- Siguiente escalón: localizar el texto del ítem y, de ahí, mostrar un submenú propio.

## Estado anterior (referencia)

## Dónde
Frank corrigió: va en **Conmegol** (`D:\Frank\Games_\Conmegol Patch`), no en el base `C:\PES21`. Un primer montaje en el base (`Phoenix\04_MenuPhoenix`) quedó APAGADO (línea comentada) y sin uso.

## Paso a paso (replicable)
1. Backups: `D:\Frank\IA\CLAUDE\pes-menus\respaldo_E1_conmegol_20261110` (`sider.ini.antes`, `Menu_TopMenu`, `TrainingMatchInit.json`). Del livecpk solo TopMenu (entero es enorme).
2. Conmegol ya sobrescribe `TopMenuNew.json` en `SiderAddons\livecpk\Menu` (única línea activa del ini con ese archivo). Ahí `13_multilobby` y `14_onlinewatch` apuntan a `TopMenu/TopMenu` (sin uso).
3. Paquete `SiderAddons\livecpk\Phoenix-Menu\common\script\flow\`: `TopMenu\TopMenuNew.json` (copia del de Conmegol, UN cambio: `14_onlinewatch` → `Match/TrainingDirectInit`) y `Match\TrainingDirectInit.json` (proceed → `Match/TrainingMatchInit`). `LEEME.md` dentro.
4. `SiderAddons\sider.ini`: `cpk.root = ".\livecpk\Phoenix-Menu"` antes de `PhoenixMandos`. Apagar = `;` delante.
5. Abrir Conmegol: `SiderAddons\sider.exe`, 4 s, luego `PES2021.exe` de la raíz. Verificar ruta del proceso (no `C:\PES21`).

## Comprobado
- `sider.log` de hoy 21:36: «Using cpk.root: ...\Phoenix-Menu\» → cargó.
- PES llegó al menú principal sin cerrarse (zona TOP_MENU). Teclas del MCP: `Esc` (no «Escape»), `Espacio`, `Enter`, flechas.
- Mosaicos: Día de Partido, Liga Máster, Become a Legend, Entrenamiento; pestañas Música y Sonido, Estadísticas, Ajustes. Ninguno identificado aún como el hueco 14.

## Hallazgos
- El mapeo mosaico→evento está en el layout de UI, no en el flow JSON. Las capturas del MCP van atrasadas y recortadas.
- Core de Conmegol arrancó en «modo seguro» (no es de este trabajo).

## Revertir
`;` delante de la línea `Phoenix-Menu` en `SiderAddons\sider.ini` (o restaurar `sider.ini.antes`) y reiniciar PES.

## PRUEBA FINAL (10-nov, ~22:05) — BOTÓN COMPLETO OK
- Config activa en Conmegol: `Phoenix-Menu` (remapea `14` y `20_onlinesettings` → `Match/TrainingDirectInit` → `Match/MatchTrainingTeamSel`) + `Phoenix-Menu-Texto` (`all.str` con «Opciones en línea» → «Phoenix», sustitución en sitio). Ambas líneas `cpk.root` activas arriba del `sider.ini`.
- Resultado: en Ajustes el ítem se ve como **«Phoenix»**; al pulsarlo se abre Entrenamiento libre (Local/Visitante). PES respondió, sin cierres. `sider.log` muestra los dos paquetes cargados. Ítems online NO pulsados.
- Se deja ACTIVO. Revertir: `;` a las dos líneas (o restaurar `respaldo_E1_conmegol_20261110\sider.ini.antes_prueba_boton`).
- Rama de respaldo: GitHub `Fralexito/smash-soda-fork` → `claude/pantallas-propias-menus` (carpeta `parche-v01/menu-phoenix-e1`, sin `all.str`). Drive: carpeta «Menu Phoenix E1 — 10-nov-2026 (respaldo)».

## PRUEBA 1 del diseño de pantalla propia — ida y vuelta (10-nov, ~22:20) — OK
- Cambio solo de JSON en `Phoenix-Menu\...\TopMenu\TopMenuNew.json`: `20_onlinesettings` → `TopMenu/TopMenu` (antes → `Match/TrainingDirectInit`). Backup: `TopMenuNew.json.e1_antes_prueba1_entrenamiento`. Sin Core, sin descargas, sin ítems online.
- Resultado: Frank pulsó «Phoenix» en Conmegol y volvió al menú principal sin cerrarse. La sonda registró la zona **7 → 49 → 7** (TOP_MENU → ONLINE_SETTINGS → TOP_MENU) y PES siguió estable. Confirma el flanco 7→49 como disparador para la pantalla propia (ver DISENO_PANTALLA_PROPIA_CORE.md §2–3, variante A).
- Estado actual: «Phoenix» hace ida y vuelta y NO abre el Entrenamiento libre. Para volver al Entrenamiento libre: restaurar el JSON del backup.
- Incidente: mis teclas de arranque coincidieron con Frank usando la misma ventana (él tenía Conmegol abierto); no mandé más teclas al darme cuenta.
