# Capítulo 60-o — Modo street con menos jugadores: la puerta del Entrenamiento libre y los menús en JSON (23–26 oct 2026)

> Pregunta de Fralex (23-oct): «si ya logré agregar jugadores y mandos que no existen, ¿será posible lo contrario, quitar jugadores para mi modo street?». Este capítulo recoge todo lo hecho hasta el 26-oct: qué se consiguió, cómo replicarlo, los intentos fallidos y lo que queda. Base anterior: M09 (`PRUEBA_CANCHA\informes\manual\M09_MENOS_JUGADORES_4V4_Y_ROJA.md`), memorias `pes-cancha-modo-street-prototipo-a`, `pes-minimo-7-jugadores-termina-partido` y `pes-objetivo-cantidad-jugadores-elegible`.

## 1. Resumen
- **Respuesta a Fralex:** SÍ es posible, pero cuesta más que lo de los mandos. Los mandos tenían un tope único (8→16). El número de jugadores (11/22) está repartido en unas 27 000 comparaciones, y no hay un contador entero por equipo: se refutó con 5 partidos de rojas el 15-oct.
- **Cuatro piezas para el objetivo:** (1) quitar jugadores al empezar; (2) apagar la regla de «menos de 7, se acaba el partido»; (3) formaciones, IA y portería dibujada para pocos jugadores; (4) un menú para elegir N dentro de Phoenix Link.
- **Hallazgo principal (25-oct):** el flujo de menús de PES son **2158 archivos JSON** dentro de `Data\dt16_all.cpk` (`common/script/flow/**`). En `TopMenu/TopMenuNew.json` **sigue cableado el Entrenamiento libre** (evento `05_freeTraining`); Konami solo quitó el botón. Como el motor ya corre partidos de entrenamiento, es la vía elegida por Fralex para «menos jugadores».
- **Prueba en vivo (25-oct ~22:10):** con un override por livecpk, el botón «ENTRENAMIENTO» de Conmegol abre las pantallas escondidas (Local/Visitante, Estrategia de los dos equipos y menú previo). **«Empezar partido» sale atenuado**, así que el partido aún no arranca.
- **Gameplay street:** no existe ningún mod street, futsal o 5v5 para PES 2021; solo mods de regate 11v11. Queda registrado para el futuro (cap. 90), en orden y sin prisa.

## 2. La regla «menos de 7» y la ruta del abandono (lectura estática, 25-oct)
- Barrido de todas las funciones del volcado (`PRUEBA_CANCHA\informes\datos\rea\todo\todo_*.txt`, cabeceras `==== 0xDIRECCIÓN  FUN_x`) buscando `< 7` / `<= 6` junto a `_DAT_1436f3dc8+0x270` y bucles de 11/22: 160 candidatas pequeñas. En el bloque 0x1404–0x1408 ninguna cuenta jugadores en campo (IA, menús de 6 casillas, opciones). **Sin localizar.**
- `FUN_141B8B250` (setter de `S+0x1554`, código en `+0x1574`) recibe 0, 1, 6, 7 y 8 desde MatchControl y desde `FUN_1404A56C0`/`FUN_1404A6D20`: es una **fase del partido**, no el motivo de abandono. El «2 = abandono» de `ObserverForfeitedGame` sigue sin confirmar.
- `FUN_140447290` (secuencia de fin de partido, subestado en `+0x8F2` 1..5) termina enseguida si el byte `[gestor+0x270]+0x1775` ≠ 0. Es un posible interruptor.
- Script usado (reutilizable): `ext.py` extrae funciones por dirección de los `todo_*.txt`, y `grepfun.py` lista las funciones que contienen un patrón (por ejemplo una vftable `UNK_142bbb768`).

## 3. Los menús de PES son JSON (cómo extraerlos)
1. El catálogo `PRUEBA_CANCHA\informes\datos\catalogo\cpk\dt16_all.cpk.tsv` da `ruta, tamano_en_cpk, tamano_real, posicion`. Las entradas de `common/script/flow/` NO están comprimidas (los dos tamaños son iguales).
2. Para cada fila: `dd if=dt16_all.cpk of=<ruta> bs=1 skip=<posicion> count=<tamano_real>`. Salen 2158 JSON; hay copia comprimida en `PRUEBA_CANCHA\informes\datos\flujo_menus_dt16.tgz`.
3. Formato de cada pantalla: `events{ nombre:{ to:"Carpeta/Pantalla", go:forward|backward, clear, fadeStart, fadeEnd, bgType } }` + `data{ type: menu|process, bgId, dialog, mark }` (+ `string`, `binFile`).
4. **Consecuencia:** todo el árbol de menús se puede re-cablear con archivos (Sider livecpk) sin tocar el exe: botones propios («Modo Street»), atajos, saltarse pantallas, abrir modos ocultos. Conmegol ya trae su propio `livecpk\Menu` que remapea eventos del TopMenu (07→comentario, 08→sonido…).

## 4. Cadena del Entrenamiento libre
`TopMenuNew.05_freeTraining → Match/TrainingUgcCheckJoinMode → Match/MatchTrainingTeamSel (bg EXHIBITION) → TrainingPrePlayerSelect → MatchTrainingMatchMenu → (00_kickoff) MatchMenuSub/MatchMenuEnd → Match/MatchTrainingSetting (menú de ajustes) → TrainingMatchInit → Match/MatchSetup`.
- El Tutorial entra de la misma forma (`Tutorial/TutorialUgcCheckJoinMode`).
- Frente a la exhibición, el entrenamiento **no pasa** por `ExhibiFlowInit` ni por `ExhibiChangeCompeDesignKind` (modo y estadio). El TopMenu de Conmegol salta además `TopMenuModeInit`.
- En el exe: `freeTraining`, `freeTrainingMenu` (evento de pausa en el modo 11 = entrenamiento libre), `Match/TrainingDirectInit` (registrado junto a `MatchDirectInit`, **sin JSON**), `CreateTrainingSetupTeamSelect`, `PLAY_DATA_TRAINING_FREE / _FREE_12 / _FREE_22 / _FREE_KICK / _PK / _CONER_KICK / _GK`. Clases: `match::registry::TrainingWork(Ref)`, `match::player::ActionFreeTrainingBreak`, `ActionTrainingMoveRoute`, `menu::MatchTeamSelectTraining`, `MatchPauseTrainingMenu`.
- Recursos: `Asset/model/training_prop/` (conos, maniquíes, barreras x3–x5, paneles de portería, líneas de 6–42 m) y `loaderSwf_trainingPlayArea` (área de juego delimitada → posible mini-campo).
- `.bin` de interfaz: `trainingMainMenu.bin` = retos del Tutorial (Básico…Experto); `trainingSelect.bin` / `playerTrainingSetting.bin` = entrenamiento de jugadores de ML/BAL. No hay menú «Libre / Tiro libre / Penal» antes de elegir equipos.

## 5. Prueba en vivo del 25-oct (paso a paso para replicarla)
1. Respaldar `sider.ini` (se hizo `sider.ini.entrenamiento-20261025`).
2. Crear `SiderAddons\livecpk\Phoenix-Entrenamiento\common\script\flow\TopMenu\TopMenuNew.json` como **copia del de `livecpk\Menu` de Conmegol** (no del original) y cambiar `04_tutorial.to` **y `06_myclub.to`** a `Match/TrainingUgcCheckJoinMode`.
3. Añadir `cpk.root` de esa carpeta **al principio** de la lista de `sider.ini`.
4. Abrir PES **con el selector**. El JSON NO se recarga en caliente: cada cambio exige reiniciar PES (1–2 min por arranque).
5. Pantalla principal → «ENTRENAMIENTO» → «Empezar» → Local/Visitante (los dos equipos a la vez) → «Estrategia» de ambos equipos (4-2-1-3, 11 por lado, reloj ~114→0 que no avanza) → menú previo (Estadio, Empezar partido, Estrategia, Opciones generales, Cámara, Lista de usuarios).
6. Al terminar: cerrar PES, restaurar `sider.ini` desde el respaldo (verificado con `cmp`) y dejar la carpeta desactivada.

**Errores e intentos fallidos:**
- Con solo `04_tutorial` cambiado, el botón seguía yendo al Tutorial: en Conmegol ese botón dispara **`06_myclub`**.
- «Empezar partido», «Estadio» y «Opciones generales» salen **atenuados**; Enter no hizo nada en 45 s y unas 30 teclas en «Estrategia» tampoco avanzaron. No se probó con mando real.
- PES solo acepta teclado por **scancode**; Espacio abre el overlay de Sider.
- `sider.exe` se quedó colgado en el segundo cierre y se terminó con `taskkill` de su propio PID.
- En la prueba se eligió **el mismo equipo** en ambos lados (Academia Puerto Cabello); puede ser la causa del botón apagado.
- Capturas: `PRUEBA_CANCHA\informes\datos\entrenamiento\` (22–37 = flujo nuevo). Informe largo en la PC: `PRUEBA_CANCHA\informes\ENTRENAMIENTO_LIBRE_PRUEBA.md` (con «AMPLIACIÓN»).

## 6. Pruebas preparadas (desactivadas; hacerlas con el MCP phoenix-pes cuando el juego esté libre)
- **P0:** repetir con DOS equipos distintos, sin cambiar archivos.
- **P1:** `SiderAddons\livecpk\Phoenix-Entrenamiento-B1` — `TrainingPrePlayerSelect.proceed → Match/MatchTrainingSetting`.
- **P2:** `…-B2` — directo a `Match/TrainingMatchInit`.
- **P3:** crear `Match/TrainingDirectInit.json` (entrada directa).
- Activar B1 o B2 = añadir su `cpk.root` antes del de `Phoenix-Entrenamiento`, con respaldo de `sider.ini` antes. Coordinar primero con los demás hilos: Link y mandos también usan PES y `sider.ini`.
- **Aviso:** en el TopMenu de Conmegol, `12_freelobby` y `18_info` van a `Match/MatchDirectReplay` (repeticiones reales de Fralex: no tocar).

## 7. Cosas a tener en cuenta
- Regla del 25-oct: Conmegol es solo banco de pruebas. Toda función debe ser **opción encender/apagar** que funcione en PES 2021 limpio; los overrides livecpk cumplen eso.
- La «minicancha» que Fralex vio dibujada viene, casi seguro, del prototipo de cancha 30×20 del Camp Nou (`escalar_capa_campo.js`, `RESTAURAR_CAMP_NOU.ps1`). Es independiente de los mandos.
- Si el entrenamiento arranca, el siguiente paso es contar los actores creados (objetos de jugador de 640 B, `pelota.exe … jugador`) y compararlos con una exhibición, para ver dónde se decide N.
- Ideas que abre: modo street dentro del entrenamiento; retos de habilidad con conos y maniquíes; retos de tiros libres, penales y córners; un menú Phoenix propio en la pantalla principal; acceso directo a partido (`Match/MatchDirectSetup`).

## 8. ¡LOGRO! 26-oct ~01:17 — Se abre el «Menú de entrenamiento» escondido (prueba P1)
**Cómo se llegó (paso a paso, lo hizo Fralex a mano):**
1. Link 0.3.54 → SYNC > AVANZADO → «RAÍZ DE PRUEBA (SIDER)»: texto `.\livecpk\Phoenix-Entrenamiento`, PONER (Link pone la carpeta la primera en sider.ini y guarda copia). La regla del proyecto prohíbe a Claude editar sider.ini directamente: siempre por este botón.
2. **P0 (FALLÓ):** ENTRENAMIENTO → Empezar → FC Barcelona vs Real Madrid (equipos distintos) → menú previo: «Empezar partido», «Estadio» y «Opciones generales» siguen en GRIS; reloj en cuenta atrás (109 s); □ «Lista de usuarios» dice «No hay datos». Conclusión: ese menú (`MatchTrainingMatchMenu`) es el pre-partido de una SALA ONLINE; con la sala vacía, lo del anfitrión sale apagado. El equipo repetido NO era la causa.
3. **P1 (FUNCIONÓ):** cerrar PES (y sider.exe), en Link cambiar el texto a `.\livecpk\Phoenix-Entrenamiento-B1` y PONER (sin QUITAR la anterior), abrir PES con el selector → ENTRENAMIENTO → Empezar → equipos → aparece **«Menú de entrenamiento — Elige el tipo de entrenamiento»** (`Match/MatchTrainingSetting`).

**Opciones del menú (captura de Fralex):**
| Opción | Valor visto |
|---|---|
| Atacante/Defensor | Ataque |
| Tipo de entrenamiento | Entrenamiento libre (◀ ▶: hay más tipos) |
| Cursor de PT fijo | No |
| **Jugadores usuario** | **11** |
| **Jugadores COM** | **1** |
| Nivel de partido | Juvenil |
| Faltas | No |
| S. pta./córner/banda | Ambos equipos |

**Importancia:** el juego tiene de serie la opción de **elegir cuántos jugadores hay** por equipo (usuario y COM). Es la vía nativa para el modo street (N contra N) sin parchear las 27 000 comparaciones con 11/22. Pendiente: mínimos y máximos de cada valor, lista de tipos de entrenamiento, ver la cancha con por ejemplo 4 contra 4, y si las faltas, saques y portería funcionan.

### 8.1 Mínimos de jugadores según el tipo (pruebas de Fralex, 26-oct ~01:20)
El texto de ayuda del juego dice: «Elige el número de jugadores que participarán en el entrenamiento… El número mínimo necesario varía según el tipo de entrenamiento.» Al cambiar de tipo, el juego sube solo los valores que quedan por debajo del mínimo.
| Tipo de entrenamiento | Mínimo jugadores usuario | Mínimo jugadores COM | Notas |
|---|---|---|---|
| Entrenamiento libre | 3 (no dejó bajar de 3) | ¿? (con 4 vs 4 y luego Tiro libre, COM subió a 5) | Antes estaba en 11 usuario / 1 COM |
| Tiro libre | ¿? | 5 (sube solo) | Visto 4 usuario / 5 COM |
| Penalti | 1 | 1 | 1 contra 1 |
| Córner por la derecha | 4 | 1 | Probable también córner por la izquierda |
Pendiente: confirmar el mínimo de COM en Entrenamiento libre, la lista completa de tipos, y entrar a la cancha.

### 8.2 Primer intento de entrar a la cancha (26-oct ~01:24): carga atascada
- Configuración: Entrenamiento libre, 3 jugadores usuario vs 1 COM, Faltas Sí → OK. Mínimos confirmados en Entrenamiento libre: usuario 3, COM 1.
- Resultado: pantalla fija (fondo de River Plate). PES VIVO (sin WerFault; CPU sostenida, ~3–4 núcleos; memoria plana ~1970–2020 MB) pero sin avanzar durante más de 9 min.
- Pista: el log de Sider, justo después de `set_teams` (108 vs 109), da `lua ERROR from module_stadium_name: StadiumServer.lua:705: attempt to index a nil value` (línea: `team_assignment_map[ctx.home_team][random_num_team]`). La sonda marca `stadium_choice = 65535` (sin estadio). El camino B1 se salta la elección de estadio («Estadio» salía apagado en el pre-partido), así que el módulo de estadios de Conmegol falla. Causa más probable, no demostrada.
- La sonda ve el entrenamiento por este desvío en la zona MYCLUB (id 30), porque entra por el evento `06_myclub`.
- Siguiente prueba: (a) entrenamiento con el módulo `StadiumServer` apagado (necesita botón en Link, porque sider.ini solo lo edita el instalador), o (b) fijar un estadio antes de `TrainingMatchInit` (por ejemplo pasando por la elección de estadio). Recordatorio: la meta final es que funcione en PES limpio, sin los módulos de Conmegol.
