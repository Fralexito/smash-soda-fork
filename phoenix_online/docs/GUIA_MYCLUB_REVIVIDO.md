# Guía: cómo se revivió myClub de PES 2021 con un servidor propio (bitácora del 4-5 nov 2026)

Autor del trabajo en la PC: el agente local («Claude en la PC de Frank»). El protocolo (comandos, claves, tipos) vive en `phoenix_online/docs/ESPEC_MYCLUB.md` del repo; esta guía cuenta **cómo se llegó**, con los pasos para repetirlo, los errores y lo que hay que tener en cuenta.

## 1. Qué se logró (estado al 5-nov, ~04:00 hora Lima)
Con el **PES 2021 base** (sin parche, sin Sider) apuntando al servidor propio (Python, carpeta `pes-online\phoenix_online`):
- myClub **abre**: cabecera «Phoenix FC» con escudo y saldo GP; pestañas Partido, Sede de club, Contrato, Estadísticas, Configuración.
- Sede de club: Dirección plantilla (XI + banquillo colocados, DT con nombre), Miembros (23 jugadores con foto, radar y stats), Entrenamiento.
- Contrato: Representantes, Cazatalentos (8 botones), DT (lista, ficha, pago con GP), Acuerdo de préstamo, Negociar.
- Partido **vs. COM jugado de verdad**: selección de mandos → alineaciones (Barcelona vs Real Madrid con 11 jugadores distintos) → saque inicial → partido ~10 min → `SET_MYCLUB_VSCOM_GAMERESULT` guardado (+GP).
- Compra de un DT con GP (saldo sube/baja y se guarda en la base del servidor).
Pendiente: media/posición reales de jugadores (bits de `Player.bin`), mercado de jugadores (Representantes/Cazatalentos/subastas con datos), experiencia tras partido, partidos entre dos personas (relé UDP).

## 2. Cómo arrancar la prueba (paso a paso)
1. PES **cerrado**. Copiar a `pes-online\phoenix_online\phoenix_online\` los `.py` de la rama `claude/project-thread-9ciw66` (worktree `C:\dev\phoenix-online-push-wt`). **Nunca reiniciar el servidor con PES abierto** (el cliente lo vive como desconexión).
2. `config.json` (NO se sube nunca): bloque `myclub` con `jugadores`, `entrenadores` (coach_id = **primera columna de Coach.bin**), `mercado_dt`, `club_de_prueba`, `rival`, `gp_por_partido`, `catalogo`.
3. Lanzar el servidor: desde `pes-online\phoenix_online`: `python ..\diag_gamerelay.py config.json > servidor_gamerelay_05nov_X.log` (añade el log `RX_MSGID` de cada comando TCP; los manejadores ya implementados no se registraban si no).
4. Lanzar PES: `C:\PES21\eFootball PES 2021\PES2021.exe`. Teclas por el MCP `phoenix-pes` (`pes_tecla`/`pes_navegar`): Enter (título) → Enter (aviso) → Abajo → Enter (myClub) → Enter (Empezar) → Enter ×3 (avisos) → menú. Pestañas: Izquierda/Derecha repetidas (E/Q no cambian).
5. Mirar el log del servidor: `grep -E "RX_MSGID|WARNING|Traceback"`; cada petición **sin manejador** aparece como «comando sin manejador» con sus campos.

## 3. Método de ingeniería inversa (el que funcionó)
- **Ghidra headless, solo lectura**, sobre la copia del exe recortado (proyecto `C:\Temp\claude\gh_copia`, script `corre.ps1`). Para cada comando `CMD_…`: la cadena del msgid la referencian dos constructores (petición y respuesta); la vtable `Result` entrada `+0x10` es el **lector** y la del Request `+0x08` el serializador. Herramienta propia `VtFromCtor.java` + `KeyKinds.java` (tipos por patrón: `cvttsd2si`=int, copia a cadena=texto, `14023d170`=lista, `141c190a0`=anidado). **Ojo:** `KeyKinds` ignoraba claves de <3 letras (se perdió `gp`; ya corregido a ≥2).
- **Reglas de formato** que el cliente exige: anidados = `bin(msgpack(mapa))`; elementos de lista de objetos = bin; enumerados/indicadores = TEXTO (int/bool donde va texto → crash); enums con PREFIJO (`PLAYER_RARITY_*`, `MATCH_LEVEL_*`, `TEAM_POWER_LV*`); **datos binarios (gameplan) = TEXTO base64** (el cliente los lee como cadena terminada en NUL).
- **Volcados de error de Windows** (`C:\Users\WinterOS\AppData\Local\CrashDumps\PES2021.exe.<pid>.dmp`): se leen con Python (`MINIDUMP_EXCEPTION_STREAM`): código, dirección, registros y direcciones de retorno en el módulo (base 0x140000000, sin ASLR). Así se localizó el lector que fallaba (p. ej. `141c4e427` dentro del lector de COMMENTARY_INFO → `get_player_result` debía ser lista).
- **Depurador propio de solo lectura** (`scratchpad\dbg_oponente.py`, ctypes): `DebugActiveProcess` + `DebugSetProcessKillOnExit(False)` + **puntos de ruptura por hardware** (Dr0-Dr3, sin escribir memoria) + `DebugActiveProcessStop` al final. PES NO se cerró al adjuntar. Se usó para ver que los 18 miembros del rival se guardaban bien (llamadas en `141da8d25`). x64dbg (en `herramientas\x64dbg\programa\release\x64`) ya estaba instalado pero no se manejó (interfaz gráfica).
- **Datos del juego**: `Data\dt10_x64.cpk` → `common/etc/pesdb/` (`PlayerAssignment.bin`, `Player.bin`, `Team.bin`, `Coach.bin`, `MyclubCoach.bin`) con `CriPakTools_fix.exe`; cabecera `FF 10 81 'WESYS'` + zlib. Las pantallas del juego están descritas por JSON de flujo en `dt16_all.cpk` (`common/script/flow/Online/MyClub/*.json`).

## 4. Errores e intentos fallidos (y la causa real de cada uno)
| Síntoma | Causa real |
|---|---|
| myClub volvía al menú ~1 s tras SQUADLIST | El estado 0x15 «salir» (código 1413d5000..): lista de jugadores vacía → hay que mandar jugadores/coach/plantilla reales |
| Tarjetas con «!» rojo | `contract_term` = 10 (muy bajo); con 100 desaparece |
| «Total de gastos 90/70» | `gameplayer_cost` 5 × 18 > organización del DT (70); con coste 3 → 54/70 |
| DT sin nombre | `coach_id` = primera columna de `Coach.bin` (8, 24, 37…), no los 121xxx de `MyclubCoach.bin` |
| «No tienes jugadores suficientes» | el aviso mezcla contrato/coste/DT; se arregló con las tres cosas anteriores + 18 inscritos |
| Rareza «NotImplement» | faltaba el prefijo `PLAYER_RARITY_` |
| PES se cierra al pedir el rival | `coach_adaptability_rate` debe ser LISTA, `concept_data` LISTA; y la respuesta (>1024 B cifrados) no cabía en una trama (`ErrorTrama`) → vaciar concept_data / trocear |
| PES se cierra en la carga | `get_player_result` en COMMENTARY_INFO debía ser lista |
| Rival = 11 × «T. Courtois (PT)(C)» y todos los nuestros «(PT)»; partido que acaba solo (INJURY) | `gameplan_data` devuelto como bytes crudos (empieza por 0x00 → cadena vacía); debe ir en **base64 como texto**, con `gameplan_data_size` 536 |
| Saldo «G 0» | `GET_GP_ACHIEVEMENT.all_gp` fijo en 0 |
| GAMERESULT espurio DRAW +120 al entrar | el cliente manda un GAMERESULT vacío al login (pes_srv_rtt_nums 0 y past_game_played_pid 0): ignorar |

## 5. Cosas a tener en cuenta
- El cliente dibuja foto/nombre/valoración desde SU base de datos por `gameplayer_id`: el servidor solo reparte ids. Con parches de plantillas distintos hay que leer el `PlayerAssignment.bin` de ese parche.
- Real Madrid sin licencia = «Madrid Chamartín», escudo «MD» (oficial del juego).
- La ventana de PES queda «minimizada» durante la carga; `pes_captura` falla un rato.
- No subir nunca: `config.json`, capturas, claves, `catalogo.json`, `vinculos.json`, código de PESBUL.
- Reglas de seguridad vigentes: nada de apuestas con dinero real (monedas myClub = siempre apagadas), descargas solo con permiso directo de Frank (el sistema de permisos de Claude Code lo aplica), sin Frida, sin tocar `sider.ini`, sin Conmegol.

## 6. Cómo comprobar y revertir
- Comprobar: `servidor_gamerelay_*.log` sin `Traceback` ni «sin manejador»; en PES: menú de myClub, Dirección plantilla sin «!», vs. COM hasta el saque.
- Revertir: `phoenix_online.db` tiene copias (`*.antes_*.db`); `config.antes_*.json` guarda configuraciones previas.
