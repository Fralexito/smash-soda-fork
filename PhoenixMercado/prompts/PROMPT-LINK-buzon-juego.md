# PROMPT PARA EL CHAT LINK — Phoenix Link escribe el «buzón» del juego (avisos web → PES 2021 en vivo)

Hola. Soy el chat MERCADO (cuenta B, rama `mercado-fase0`). Te pido una función nueva en **Phoenix Link**. Sigue tu protocolo:
- rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`;
- compilar en el worktree `_phoenix-link` (`COMPILAR_PHOENIX.bat`), **sin errores** antes de cada commit;
- una línea por cambio en `REGISTRO-LINK.md`;
- nada a `master` sin permiso de Fralex;
- «si algo se rompe, mejor no lo hagas».

## Qué ya funciona (probado en el juego de Fralex, 2026-10-09)
- El módulo de Sider `phoenix.lua` v0.2 está instalado en el parche ConmeGOL (`ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`, que el switcher copia encima del juego con robocopy).
- Cada 2 s lee `<carpeta del juego>\SiderAddons\content\phoenix\avisos.txt` y lo muestra en el overlay de Sider (barra espaciadora, luego tecla 1 hasta «PHOENIX EVOLUTION»).
- Con el juego abierto se reemplazó el archivo y el texto nuevo («hola causa») apareció en 1–2 s. Tildes, ñ, ⚡ y → se ven bien.
- Código y pruebas: rama `mercado-fase0`:
  - `PhoenixMercado/sider/phoenix.lua`;
  - `PhoenixMercado/sider/VINCULO-TIEMPO-REAL.md`;
  - `PhoenixMercado/PRUEBAS.md`.

## Lo que tienes que construir: el «cartero»
Un paso nuevo en el hilo de `PhoenixLink` (`SmashSoda/phoenix/link/PhoenixLink.cpp`), al estilo de `pasoAmigos`:

1. **Solo cuando el juego está abierto.** Detecta `PES2021.exe` con `CreateToolhelp32Snapshot` cada ~5 s. Con `QueryFullProcessImageNameW` (`PROCESS_QUERY_LIMITED_INFORMATION`) saca la **carpeta del juego**. Esto vale para cualquier parche (ConmeGOL, SP, etc.), sin que Fralex configure nada.
   - Si no hay juego abierto → no se pregunta a la web (ahorra cuota del plan FREE).
   - Si el juego está abierto pero no existe `<juego>\SiderAddons\content\phoenix\` → no escribir nada. Solo marca el estado «Puente Sider no instalado». **Nunca crees carpetas ni toques `sider.ini`.**
2. **Consulta a la web:** `GET /v1/juego/buzon` (contrato `docs/contrato-v1.md` §25, versión 1.7.0, que está haciendo el chat WEB).
   - Mismos encabezados y token `phx_…` de siempre.
   - Usa ETag igual que en `pasoAmigos` (`If-None-Match`; 304 = sin cambios).
   - Cada `sondeo_seg` de la respuesta (por defecto 15 s, nunca menos de 5).
   - Política de reintentos del contrato §0.
   - Si `pausado: true` o `interruptores` lo apagan → parar hasta el próximo `config`.
3. **Escribe el buzón** cuando la lista cambie. Formato del archivo (UTF-8 **sin BOM**, `\n`):
   - **Contenido:** máx. 14 líneas y 1500 bytes. Avisos del más nuevo al más viejo, cada uno como `[hh:mm] texto` (hora local de Lima, de `creado_en`). Entre avisos va una línea vacía. Si no caben, se corta sin partir una letra UTF-8. `phoenix.lua` igual recorta a 14 líneas y 110 bytes por línea.
   - **Sin avisos:** el texto `Sin avisos nuevos de Phoenix Evolution.`
   - **Escritura atómica (obligatorio):**
     - escribe primero `avisos.tmp` en la misma carpeta;
     - luego `ReplaceFileW(avisos.txt, avisos.tmp, …)`, o `MoveFileExW(tmp, txt, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` si `avisos.txt` aún no existe;
     - si falla porque el juego justo lo está leyendo, reintenta hasta 10 veces cada 100 ms.
     - El juego nunca debe ver un archivo a medias.
   - **No escribas si el contenido es igual** al último escrito.
4. **Confirma la entrega:** tras escribir, `POST /v1/juego/buzon/entregado` con los `ids` nuevos. Así la web muestra «✅ entregado al juego».
5. **Interfaz (mínima por ahora; el diseño lo vemos después):** en Ajustes o en el panel de estado, una fila «Puente con el juego» con:
   - **estado:** Juego cerrado / Puente Sider no instalado / Conectado (último aviso hh:mm) / Sin conexión;
   - **casilla:** «Mostrar avisos de la web en el juego», por defecto activada y guardada en `PhoenixPrefs`. Si se apaga, no se consulta ni se escribe.
6. **Seguridad y rendimiento:**
   - todo en el hilo de `PhoenixLink`, nunca en el hilo de la interfaz ni bloqueando al juego;
   - timeout de 5 s;
   - sin lecturas de memoria del juego ni inyección: **solo escribir ese archivo de texto**;
   - registra en el log lo justo (sin el token).

## Pruebas antes del commit
- **Sin el endpoint todavía:** si `/v1/juego/buzon` devuelve 404 o `RUTA_NO_ENCONTRADA`, Link no debe romperse. Que se quede en «Sin conexión», reintente con espera larga y siga con todo lo demás normal.
- **Prueba unitaria del formateador:** límites de 14 líneas y 1500 bytes, UTF-8 cortado bien, línea vacía entre avisos, hora de Lima.
- **Prueba de escritura atómica** sobre una carpeta temporal: escribir mientras otro hilo lee en bucle; nunca debe leerse un archivo vacío o a medias.
- **Prueba real (con Fralex):** juego abierto con ConmeGOL → mandar un aviso desde la web → debe salir en el overlay en ≤ 20 s y la web marcar ✅.

## Coordinación
- **Archivos del Mercado:** no toques `PhoenixMercado/`. El módulo Lua es del Mercado. Si necesitas que cambie su formato, pídelo por prompt.
- **`COORDINACION.md`:** anota en «Registro de cambios que afectan al otro chat»: «Phoenix Link escribe `<juego>\SiderAddons\content\phoenix\avisos.txt` (atómico, UTF-8 sin BOM, ≤ 14 líneas / 1500 bytes)».
- **Siguiente fase (aún no):**
  - el juego escribe `content\phoenix\resultado.json` al terminar cada partido y Link lo sube a la web;
  - la web manda «ajustes del próximo partido» por el mismo buzón.
  - Te lo pediré con otro prompt.
