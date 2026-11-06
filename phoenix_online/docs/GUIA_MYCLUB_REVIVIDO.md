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

## 7. Planes e ideas de Frank (anotado el 5-nov-2026, tarde-noche Lima)
Orden de trabajo acordado para dejar myClub completo y escalable (los pasos 1-3 son los inmediatos):
1. **Cerrar el vs. COM de punta a punta:** comprobar MAIN_SQUAD completa tras «Recompensas del partido» (hoy solo se probó sin fichas), experiencia de jugadores, abandonar/cerrar PES a mitad sin corromper el club. Estado: el troceado servidor→cliente quedó corregido (formato [total][largo][tamaño original][flag zlib]+trozo, tipos 0x70/0x71/0x72) y probado con GAMEPLAYERLIST de 27 jugadores; el partido vs COM contra Madrid terminó 3–2 con +240 GP.
2. **Fichar jugadores:** los 13 comandos de Agentes/Cazatalentos están leídos (esquemas en `ESQUEMAS_FICHAJE_MYCLUB_05nov.txt`); el coordinador los implementa y se prueban uno a uno en PES.
3. **Datos reales de cartas:** posición registrada (bits 370-373 de Player.bin) y nación (bits 169-175) ya decodificadas; falta rareza real y media aproximada (la media no se guarda en Player.bin).
4. **Cartas especiales y leyendas:** buscar leyendas en Player.bin; rarezas por `default_rarity` (PLAYER_RARITY_BLACK/GOLD/SILVER/BRONZE/WHITE). Diseño del marco de las cartas = texturas del cliente (`cmnMyClubRarity.bin` en dt11_x64.cpk y variantes 4K/f2p); cambiarlo exige parche/livecpk en cada PC (copia previa, nada sin permiso).
5. **Escalable para mucha gente:** base por cuenta (ya), respaldo, límites de tamaño (troceado), registro de errores por cuenta, herramienta de admin (`tools/admin_myclub.py`: dar o fijar GP/monedas; `lista`), prueba con 2-3 cuentas en turnos. Monedas = solo premio gratis (`monedas_gratis`), nunca a la venta; GP por partido y monedas por partido editables en config (`gp_por_partido`, `monedas_por_partido`).
6. **Partidos entre dos personas:** relé UDP, requiere dos PCs reales; es lo último.
7. **Catálogo automático** (`catalogo_pesdb.py` desde PlayerAssignment.bin/Player.bin): regenerarlo si cambia la base del juego/parche; los ids tienen que existir en el PES de quien juegue.
8. **App tipo «The Gaffer» (idea de Frank):** app propia que lea club/jugadores/GP/fichajes; meta «hacer todo lo que hace The Gaffer y mejor». Necesita: API de solo lectura en el servidor con identificación de cuenta (decidir si cuenta web/Supabase o Steam). Pendiente: lista/capturas de funciones de The Gaffer (el coordinador abre hilo en «planes»).
9. **Conexión con la web de Frank:** misma API de solo lectura; decisión pendiente de cuentas.
10. **Leyendas y jugadores propios:** existentes en el juego = dar su id; propios = deben existir en la base del PES de cada PC (parche/option file). No se puede fijar una media desde el servidor.
Incidente anotado: aviso «Conexión perdida VNBP_0002» (21:03) = respuesta de 1080 B sin trocear; aviso C_WNJJ_0002/C_JZZX_0002 = respuestas al límite de la trama (≈1000 B cifrados) o partes aún sin troceado.

## 8. Fichajes (Cazatalentos y Representantes) — logrado el 5-nov-2026 (noche, Lima)
### 8.1 Qué se logró (probado en PES 2021 base, con capturas y registros)
- **Cazatalentos** (Contrato → Cazatalentos → Fichar jugador → Espacio 1 → reclutador → «Pedido de negociación»): sale la carta nueva (L. Millar), «Añadido a Miembros de myClub», plantilla 27→28. **Vender cazatalentos**: marcar casilla con Enter y **confirmar con la tecla C** → «¿Vender por 500 GP?» → Sí → +500 GP (CMD_SELL_MYCLUB_SCOUT).
- **Representantes** (Contrato → Representantes → «Caja de selección» → usar → Sí): sale la carta nueva (L. Goretzka, Bayern MC 85), plantilla 28→29, GP −10 000, caja 154→153 (CMD_USE_MYCLUB_AGENT). Cobro automático de logros de GP (CMD_SET_GP_ACHIEVEMENT: +300 por logro nuevo).
- **Partido vs COM completo** con premio, experiencia aplicada (Nv.1→Nv.2), plantilla completa tras el partido sin pantalla en blanco.
### 8.2 Cómo se llegó (para replicar): los 8 arreglos de Representantes, en orden
1. El botón rebotaba al menú: `ProcessPreMarketAgentList` (clase `process::OnlineMyClubAgentListPre`) envía él mismo CmdGetMyclubMarketAgentlist y, si el objeto de comando trae la bandera de error, emite «return». Causa: los campos del agente iban sueltos; el lector espera cada elemento como `bin({agent: bin({...}), banner_file_info: bin, skill: bin})` (banner y skill HERMANOS de «agent»).
2. AGENT_BOX_INFO: `agent_info: bin({agent_info: bin({agent, banner_file_info, skill}), box_size, box_pack_num, reset_remain_count, main_player_rarity_info, sub1_player_rarity_info, sub2_player_rarity_info, is_use_reset})` (DOS niveles agent_info).
3. Crash al entrar a la caja: `main_player_rarity_info` = LISTA de bins {rarity:int,count:int}; `sub1_` y `sub2_player_rarity_info` = UN bin cada una (no lista). Se encontró adjuntando un depurador de solo lectura (dbg_crash.py: DebugActiveProcess, sin puntos de ruptura, registra excepción + registros + direcciones de retorno) → la dirección de retorno cayó dentro del lector 141c2e880 y las cadenas en los registros («sub1», «yer_rarity») dijeron qué clave fallaba.
4. Aviso «No puedes usar este Representante porque ya caducó» (mensaje 0x22602be de la tabla de textos `dt20_*_x64.cpk → string/all.str`, zlib): lo lanza el constructor de la pantalla de la caja (`menu::OnlineMyClubMarketAgentBox`, FUN_140d46f90) cuando el vector de 8 B @+0x50 (es `token_list` {level,count}) está vacío y (ints @+0x78/+0x7c == 0 o byte @+0x75 == 0). Arreglo: `token_list=[{level:1,count:1}]`, `token_list_num=1`, `is_draw_checked="YES"`. No dependía de horas ni de GET_SVRTIME.
5. Crash por URL nula: la pantalla descarga el banner del agente con la URL del tipo 17 de GET_URLLIST (`MYCLUB_AGENT_BANNER`); sin esa clave la URL es NULL → c0000005. Se añadió `base + /asset/myclub-agent-banner` (el HTTP responde 404 limpio).
### 8.3 Teclas útiles en PES con el MCP (descubiertas)
Enter = aceptar/marcar; Esc = atrás; **C = acción contextual** (Clasificar en lista de miembros, Vender en cazatalentos); V = alterna lista/rejilla; Z = desmarcar. Cuidado: tras Esc ×2 desde Miembros se cae al menú principal y las flechas pueden entrar a otro modo (lobby online → «No se ha podido enviar ni recibir datos»; no es fallo del servidor).
### 8.4 Cosas a tener en cuenta
- Una respuesta mal formada NO siempre da aviso: puede cerrar PES sin volcado ni excepción en el servidor; entonces adjuntar el depurador de solo lectura y mirar la dirección de retorno.
- El flujo de pantallas y qué comando manda cada botón están en `common/script/flow/**.json` de `dt16_all.cpk`; los nombres de clases (`menu::…`, `process::…`) están como símbolos en Ghidra (SymSearch.java).
- Pendiente: «1/3» y «Finaliza en 0 min» en la caja/nuevo fichaje, ratio de aparición (AGENT_DRAW_RATE), renovar contrato, segunda caja, pausa/abandono en vs COM (Esc no abre la pausa en partido; el flujo existe: MatchPause `02_exit` → MatchDiscontinue `4_toMyClubMain`), valores de `type` del agente.

### 8.5 Menú del jugador y renovar contratos (descubierto con Frank, 5-nov noche)
- En Miembros de myClub → Lista de miembros: **D (la Y del mando) abre el «Menú del jugador»** del jugador bajo el cursor (en el teclado de Frank X = A de Xbox, que abre/entra; D = Y). Opciones: Añadir a Plantilla, **Renovación del contrato**, Bloquear, Desbloquear, Convertir en Entrenador de experiencia, Liberar, y «Usar objeto: Boleto de contrato ×N» (ahora ×0).
- La opción de renovar NO aparece si el contrato está al máximo (todos nuestros jugadores tenían contract_term=100). Cada renovación suma 10 partidos. Para probar: `python -I tools\admin_myclub.py config.json poner-contrato <persona> <partidos> <unique_id…>` (el servidor lee el club al entrar a myClub; no hace falta reiniciarlo).
- El precio sale de la FICHA del jugador (no hay comando de precio): `contract_gp` (GP) y `contract_pes_point` (monedas); `sell_gp` es lo que se cobra al vender. Con 0/0 la pantalla «Comprobar estado» deja solo «Pagar Monedas». Config: `renovacion_gp`, `renovacion_monedas`, `venta_gp`, `renovacion_partidos`. Comando de la renovación: CMD_EXTEND_MYCLUB_GAMEPLAYER {gameplayer_unique_id_list, use_pes_point} → {contract_term_list:[{gameplayer_unique_id, gameplayer_id, contract_term}], gp, pes_point}.
- Pantalla «Comprobar estado» verificada: «Estás a punto de renovar tu contrato con L. MESSI por 10 partidos más… GP 200 · Monedas de myClub 2 · Pagar con GP / Pagar Monedas / Comprar Monedas».
