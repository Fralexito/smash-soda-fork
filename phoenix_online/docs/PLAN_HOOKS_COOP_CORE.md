# Plan: hooks de Cooperativo para el Core (11-nov-2026)

Estado (actualizado 11-nov noche): IMPLEMENTADO Y COMPILADO en el Core 0.4.97 (rama `claude/online-phoenix-core`, commit edb0a33, funcion `coop_online`, apagada por defecto, se aplica al reiniciar PES). NO instalado en ningun PES: se probara con el 2.o PES. Antes era solo plan. Fuente de la idea: `server/mod/pes2021online.c` de AzrakHQ (MIT), seccion «Online Co-op (1 vs 1)».

## Que exe se uso
- Verificacion en vivo (lectura de memoria, sin escribir): PES base `C:\PES21\eFootball PES 2021\PES2021.exe`, **458 739 200 bytes**, proceso abierto pid 2772, con `pes_memoria_leer`.
- Comprobacion previa (solo lectura, REA): copia recortada `D:\Frank\Games_\PRUEBA_CANCHA\copias\exe_pes2021\PES2021_recortado.exe` (SHA-256 `ebd6a786...c9dc9284`). Para el analisis completo con REA se usara SOLO el exe de C: (regla de Frank), nunca Conmegol ni D:\SP2026.

## El problema
En 1.01.00 el co-op nunca conecta: tras emparejar, `TaskCoopSession` (0x141134c00) arma la lista de pares pero no se la da a la capa P2P (0x141092720; Quick Match si lo hace, desde 0x14111a8fe), y el manejador de `WATCH_COOP_MATCHING` (0x14110fe00) tira las direcciones que el parser decodifico. El P2P busca «pid 0» y se rinde (MXNV_0013).

## Firmas AOB (las 6 del mod) — verificadas hoy en el PES base de C: en vivo
| VA | Bytes | Que es |
|---|---|---|
| 0x14110fe00 | 40554154415541564157488dac24b0ed | manejador de WATCH_COOP_MATCHING |
| 0x141134c00 | 488bc4554154415541564157488d68b8 | constructor de la lista de pares del co-op |
| 0x1411346c3 | e838050000b001 | su unico llamador (call 0x141134c00; mov al,1) |
| 0x141092720 | 48895c240848896c2410488974241857 | entrega de pares a la capa P2P |
| 0x14111a8fe | 498bcee81a7ef7ff | Quick Match: mov rcx,r14; call 0x141092720 |
| 0x140ff0a20 | 488b05b1c47002c3 | getter de globales: mov rax,[0x1436fced8]; ret |
Resultado: las 6 coinciden byte a byte (tambien en la copia recortada). **No hace falta relocalizar.**

## Diseno para el Core (hook nativo; desde Lua solo no se puede hacer un detour)
1. Arranque: comprobar las 6 firmas; si alguna no coincide, no se instala ningun hook (igual que el mod).
2. `hk_coop_peers` (sobre 0x141134c00): tras el original, leer los globales `[0x1436fced8]` (+0x30 estado coop, ranuras +0x58/+0x90) y llamar a 0x141092720 con la lista, como hace Quick Match.
3. `hk_coop_watch` (sobre 0x14110fe00): antes del original, guardar las direcciones de los pares que el parser dejo en la respuesta (cmd+0xd0) y, tras construir los registros, copiarlas a las ranuras del co-op.
4. El servidor debe emparejar co-op solo con quien declare soporte (linea `coop1` del PESID del mod; aqui, una marca que mande el Core).
## Verificacion
- Primero en vivo con x64dbg (MCP aun no cargado en esta sesion; hace falta reiniciarla) y `phoenix-pes`, solo con el PES base de C:, con punto de ruptura en 0x141092720 durante «Empezar partido» del co-op, y comparando con Quick Match.
- Despues, prueba con 2 PES reales.
## Riesgos
- Denuvo: no tocar el exe en disco; solo hooks en memoria y solo desde el Core ya aprobado.
- Si el hook falla, el PES puede cerrarse: probar primero con el PES base y con copia del Core anterior.
- Co-op de 2v2/3v3/11v11: el mod solo cubre 1 contra 1; los demas tamaños necesitan el relé (nuestro TURN) y mas analisis.
## Herramientas REA usadas hasta ahora
`open_binary`, `close_binary`, `read_bytes`, `analyze_function`, `inspect_artifact`, `extract_artifact`. Pendiente para el analisis completo: strings, xrefs, grafo de llamadas, RTTI/vtables, busqueda AOB, renombrar y anotar en el proyecto REA.

## Implementacion en el Core 0.4.97 (11-nov)
- Funcion `coop_online` en `paquetes/phoenix_core/src/phoenix_core.cpp` (apagada por defecto; activar con `\"coop_online\": true` en phoenix_core.json y reiniciar PES). Deshacer: `paquetes/phoenix_core/DESHACER_COOP.ps1 -Sider <SiderAddons>` (copia del json y deja la funcion en false; `-RestaurarDll` repone `respaldo_coop/phoenix_core.antes_coop.dll`, la dll compilada antes del cambio). Los ganchos son solo de memoria: no se toca el exe en disco.
- Comprueba las 6 firmas del plan byte a byte; si una difiere no instala nada. Ademas: firma larga y UNICA de WATCH_COOP_MATCHING (exe+0x110FE00) y de TaskCoopSession (exe+0x1134C00); la firma corta de TaskCoopSession del plan aparece 2 veces (tambien en exe+0xC4F840), por eso se alargo con `48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 10 4C 8B E1 E8` (verificado unica en el PES base vivo). La entrega al P2P se resuelve por la llamada de Quick Match (0x14111A901, rel en +2) y las globales por el getter 0x140FF0A20 (deben caer en 0x141092720 y 0x1436FCED8).
- Ganchos con `crearDesvio` del Core: 16 B en TaskCoopSession y 18 B en WATCH_COOP_MATCHING (prologos sin RIP-relativos). Lecturas/escrituras protegidas (SEH). `hk_coop_peers` entrega los pares solo si hay exactamente 1 (1v1); `hk_coop_watch` copia host/reflejada/NAT a los registros por pid. Todo o nada si el segundo gancho falla.
- Compilado con MSVC (cmake --build build_online --config Release): sin errores. SIN probar dentro del juego.

## Verificacion en vivo 11-nov (resultado y por que se colgo)
- OK (solo lectura, PES base C: pid 2772): las 6 firmas coinciden; las dos firmas largas son unicas.
- NO se pudo seguir el flujo con breakpoints: al adjuntar x64dbg (`debug_attach_pid`) el PES quedo PAUSADO por el depurador (estado `paused`, `pes_salud`: CONGELADO). Dos `debug_run` volvieron a pausar con `breakpoint_or_exception` (excepciones del juego, probablemente anti-depuracion). No se uso `debug_stop` (cerraria el PES).
- El plugin x64dbg_mcp (configuracion segura) devuelve `Method not allowed` para: disassembly.at, breakpoint.set, script.execute (y module.get_main); no hay `detach`. Para la proxima vez haria falta, en la configuracion del plugin (la cambia Frank, no Claude): habilitar los metodos breakpoint.* (mejor solo breakpoints por hardware), disassembly.* y un metodo `debug.detach`, y adjuntar con el PES en una pantalla estable; aun asi el anti-depurador puede congelarlo. Alternativa que no cuelga: lectura con phoenix-pes y logs `[COOP]` del propio Core con 2 PES reales.

## Herramientas REA usadas: `open_binary`, `close_binary`, `read_bytes`, `analyze_function`, `inspect_artifact`, `extract_artifact` (copia recortada), mas phoenix-pes (`pes_memoria_leer`, `pes_memoria_firma`, `pes_estado`, `pes_salud`) y x64dbg MCP (`memory_read`, `debug_attach_pid`, `debug_get_state`). Pendiente: strings, xrefs y grafo de llamadas con REA sobre el exe de C:.

## Resultado del analisis REA (11-nov, volcado en vivo del PES base de C:)
**Metodo (replicable).** El exe de C: (458 739 200 bytes, SHA-256 `641720e2...a9160e5a`) se abre en REA pero `read_bytes` no termina en 3600 s: es demasiado grande para importarlo a Ghidra. Solucion: 1) con `volcar.exe leer <RVA> <n>` (n = palabras de 4 bytes; solo lee la memoria del proceso, `D:\Frank\Games_\PRUEBA_CANCHA\scripts`) se volcaron 5 trozos de codigo del PES base abierto (0xff0000, 0x1092000, 0x110f000, 0x111a000, 0x1134000); 2) un script (`mkpe.py`, cabecera de 0x400 bytes: con 0x200 los datos salen desplazados 16 bytes) los envuelve en un PE minimo con cada trozo en su RVA y el punto de entrada en la funcion a analizar; 3) REA/Ghidra lo analiza en ~35 s y `batch_decompile` da el pseudocodigo. Archivos locales (NO se suben: son codigo del juego): `D:\Frank\IA\CLAUDE\pes-online\analisis_coop_rea\`.
**Hallazgos de 0x141092720 («entregar pares a la capa P2P»):**
- Recibe el objeto del modo de red. Lee un vector de pares en `obj+0x60..+0x68` con elementos de **0x30 bytes** (pid de 4 bytes al inicio, luego dos bloques de direccion y un valor `nat`).
- **Solo copia si hay MENOS de 2 pares** en el vector; arma una tabla local de hasta 2 entradas de 0x20c bytes (pid, direccion local, direccion reflexiva, nat) y llama a la capa P2P (0x14114c770) con la tabla y la cantidad. Coincide con la nota de AzrakHQ: **la capa NAT sostiene como mucho 2 pares → 2v2/3v3 necesitan el relé** (nuestro TURN), y el hook de co-op del mod solo vale para 1 contra 1.
- Antes de eso fija el «id de partida local» en `[sesion+0x10]+0x15c/0x160` (se llama a 0x14114a910 / 0x14114b630 / 0x14114c320).
**0x141134c00 (constructor de la lista de pares del co-op):** decompila bien (~280 lineas); usa el getter de globales 0x140ff0a20, helpers 0x140ff7e40/0x140ff7f40/0x140ff7fb0 y 0x14111e7d0. Falta leerlo entero para fijar donde engancha el hook.
**Falta:** decompilar 0x14110fe00 (WATCH_COOP_MATCHING; hay que armar otro PE con el punto de entrada alli), leer 0x141134c00 entero, xrefs/grafo de llamadas, renombrar y anotar. Verificacion en vivo con x64dbg: MCP no conectado en esta sesion.
**Herramientas REA usadas:** `open_binary`, `close_binary`, `read_bytes`, `analyze_function`, `batch_decompile`, `inspect_artifact`, `extract_artifact`; apoyo: `volcar.exe` y `phoenix-pes` (`pes_memoria_leer`).
