# Investigación: que Street parezca un modo normal (6-nov-2026, solo lectura, nada instalado)

Fuentes: `flujo_menus_dt16.tgz` (JSON de menús), cap. 60-p (`cap_60_p_menus_botones_y_modos_nuevos.md`), estado actual de Street (Entrenamiento libre + Sider). Certeza: **C** = leído en archivos; **H** = hipótesis sin probar en juego.

## Respuesta corta
**SÍ**, se puede. Recomendación: **Camino A** (botón «Phoenix Street» en un hueco muerto del menú principal que entra directo al entrenamiento) + **panel propio «Táctica Street»** en vez de la zona de estrategia nativa. El Camino B (amistoso real con 5 por lado) es más fiel pero depende de Core/exe y es de riesgo alto.

## Flujo real del Entrenamiento libre (C)
`TopMenuNew (05_freeTraining)` → `Match/TrainingUgcCheckJoinMode` (proceso) → `Match/MatchTrainingTeamSel` (equipos; fondo BG_ID_EXHIBITION; `return` → TopMenu) → `Match/TrainingPrePlayerSelect` (proceso) → `Match/MatchTrainingMatchMenu` → `Match/MatchTrainingSetting` (menú de entrenamiento) → `Match/TrainingMatchInit` (proceso) → `Match/MatchSetup` → partido. Pausa: `Match/MatchPauseTraining` (salir / pausa / replay), sin revancha.
El amistoso usa `Exhibition/*` (ExhibiSideSetting → ExhibiUgcCheck → … → ExhibiMatchInit → `Match/MatchSetup`). Ambos terminan en el mismo `MatchSetup`.

## Camino A — botón «Phoenix Street» → entrenamiento directo
Re-cablear solo JSON (nivel 1 del cap. 60-p; ya funcionó el 25-oct con otro desvío).
1. Un hueco muerto de TopMenuNew (candidatos: 13_multilobby o 14_onlinewatch; Conmegol ya usa 07, 08, 11, 20, 21) → `Match/MatchTrainingTeamSel`; un JSON propio `Match/TrainingPrePlayerSelect.proceed → Match/TrainingMatchInit` salta el menú de entrenamiento. Alternativa: crear JSON para `Match/TrainingDirectInit` (registrada en el exe sin JSON, C).
2. El modo se elige solo: `sonda.json` zona FREE_TRAINING + `street_auto_estadio` pone la Calle. **H**: si el tipo de entrenamiento (FREE) se fija en `MatchTrainingSetting`, hay que forzarlo por Sider/Core (a probar).
3. Texto del botón: `all.str`; dibujo: textura del `.bin` (nivel 2). Primera versión: reutilizar texto existente.
4. Pantalla de carga propia y revancha: overlay Sider (`phoenix_marcador_free.lua`) + orden al Core. **H**.
Esfuerzo: JSON 2-3 h con pruebas (1-2 min de arranque cada una); texto +1 h; carga/revancha 3-4 h. Riesgo bajo (peor caso pantalla negra; se restaura el JSON). Todo en carpeta livecpk suelta con on/off; funciona en PES limpio.

## Camino B — amistoso real con 5 por lado
- Ventaja: zona de estrategia nativa y flujo idéntico a un amistoso.
- Problema: el amistoso valida la plantilla (11 por lado; regla de <7 jugadores). **H**: la validación está en el exe; evitarla exige parche (rompe «PES limpio») o que el Core retire/oculte 6 jugadores tras `MatchSetup` (hoy ya hay `jugadores_chica_free` y `suplentes_ocultos_free` en entrenamiento; falta comprobar si en amistoso el partido se corta al faltar jugadores y cómo reacciona la IA/árbitro).
- Riesgo medio-alto, toca el Core (hilo IA). 8-15 h solo para saber si es viable. No recomendado esta noche.

## Estrategia
- La zona nativa trabaja con 11 titulares y no define formaciones de 5. **H**: no se adapta sin código.
- Mejor: **panel «Táctica Street»** en F9 (`phoenix_marcador_free.lua` → órdenes a Core): formación 2-2 / 1-2-1 / 2-1-1, arquero, presión (`ia_presion_k`, `ia_ataque_k`, del hilo IA). Panel 2 h (mío); lectura de la formación en el Core 3-5 h (hilo IA).
- Heredar la alineación del juego: localizar la tabla de plantilla con `pes_memoria_firma`/x64dbg/REA (**H**).

## Orden recomendado
1. P1: salto de menús con respaldo de `sider.ini`, cuando PES esté libre.
2. Botón «Phoenix Street» en hueco muerto + texto.
3. Panel «Táctica Street» (Lua) con órdenes `formacion_street` / `arquero_street`; el Core las implementa.
4. Carga y revancha por overlay.
5. Camino B solo como investigación posterior.

## Ideas para el catálogo
- `Match/TrainingDirectInit` (C): entrada directa al entrenamiento sin menús, ideal para «Phoenix Street».
- `ExhibiMatchInit` y `TrainingMatchInit` van a `Match/MatchSetup`: se puede reutilizar el cierre del amistoso en Street.
- Mismo mecanismo para otros modos Phoenix (retos, penales): un botón por hueco muerto.

## Qué no se hizo
No se abrió PES ni se instaló nada. Todo lo H necesita prueba en juego.
