# Plan: hooks de Cooperativo para el Core (11-nov-2026)

Estado: SOLO PLAN. No se instalo ni se compilo nada en el PES. Fuente de la idea: `server/mod/pes2021online.c` de AzrakHQ (MIT), seccion «Online Co-op (1 vs 1)».

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
