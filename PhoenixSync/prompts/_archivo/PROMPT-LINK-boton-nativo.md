# PROMPT PARA EL CHAT LINK — Phoenix Link entrega datos nuevos y avisa «pulsa Datos Actual. en vivo»

Hola. Soy el chat SYNC (cuenta B, rama `mercado-fase0`, carpeta `PhoenixSync/`). Te pido una función nueva en **Phoenix Link**. Sigue tu protocolo:
- rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`;
- compilar en el worktree `_phoenix-link` **sin errores** antes de cada commit;
- una línea por cambio en `REGISTRO-LINK.md`;
- nada a `master` sin permiso de Fralex;
- «si algo se rompe, mejor no lo hagas».

## Qué ya funciona (probado en el juego de Fralex, 2026-10-09)
- **Tu cartero** (avisos web → `SiderAddons\content\phoenix\avisos.txt`) funciona.
- **`phoenix.lua` v0.17** (Sider) **revive el botón nativo** «Partido → Datos Actual. en vivo → Activar». Al pulsarlo:
  - sale el **mensaje nativo de Konami**;
  - el juego **relee y aplica ahí mismo** la base (`Player.bin`, etc.) y el option file (`EDIT00000000`);
  - no hace falta Editar, reiniciar ni volver al menú.
  - Los parches se aplican solos al arrancar el juego, en memoria y con comprobación de bytes.
- **Dónde lee el juego los datos de Phoenix:**
  - **Stats (base):** `<juego>\SiderAddons\livecpk\Phoenix-DB\common\etc\pesdb\Player.bin`. Esa raíz va en `sider.ini` **antes** de la del parche. Formato WESYS: cabecera 16 B `ff 10 81 57 'WESYS'` + tamaño comprimido u32 + tamaño real u32, y después zlib.
  - **Plantillas / fichajes:** el option file `%USERPROFILE%\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\<steamid>\save\EDIT00000000` (en el PC de Fralex, la carpeta activa es `239200`).
  - El ConmeGOL usa un **switcher** que copia `ConmeGol Extras\<modo>\SiderAddons` encima del juego. Por eso **Phoenix-DB tiene que existir en las dos rutas**: `<juego>\SiderAddons\livecpk\Phoenix-DB\…` y `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\livecpk\Phoenix-DB\…`.
- Detalle: `PhoenixSync/PRUEBAS.md` y `PhoenixSync/base-conocimiento/07-MOTOR-POR-DENTRO.md` (rama `mercado-fase0`).

## Lo que tienes que construir: el «repartidor de datos»
**Quién fabrica los archivos:** los fabrica **Phoenix Sync** (`PhoenixSyncCore`: `OptionFile::mover` para fichajes; el generador de `Player.bin` lo hago yo en mi frente). **No los fabriques tú.** Tú solo los **colocas** y **avisas**.

1. **Entrada.** Sync deja los archivos listos en `%APPDATA%\Phoenix Mercado\entrega\` (la carpeta se mantiene con el nombre viejo a propósito):
   - `Player.bin` (opcional);
   - `EDIT00000000` (opcional);
   - `entrega.json` = `{ "version": 1, "id": "<uuid>", "creado_en": "<ISO>", "resumen": "texto corto", "archivos": [ {"nombre": "Player.bin", "sha256": "…"}, … ] }`.
2. **Comprobar antes de tocar nada:**
   - el sha256 de cada archivo coincide con `entrega.json`;
   - `Player.bin` empieza por la cabecera WESYS;
   - `EDIT00000000` mide lo mismo que el actual;
   - si algo falla → **no colocar nada** y avisar «⚠ entrega rechazada: …» en `avisos.txt`.
3. **Colocar de forma atómica** (cada archivo: `.tmp` en la misma carpeta → `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`; si falla porque está en uso, reintentar hasta 10 veces cada 200 ms):
   - `Player.bin` → las **dos** rutas Phoenix-DB. **No crees carpetas**: si Phoenix-DB no existe, rechaza con «Phoenix-DB no instalado».
   - `EDIT00000000` → antes, **respaldo** `EDIT00000000.phoenix-<fecha>` en la misma carpeta (guarda solo los 5 últimos), y después colocar.
4. **Avisar** en el buzón (mismo formato que ya usas): `[hh:mm] ⚡ Datos nuevos de Phoenix: <resumen>. Partido → Datos Actual. en vivo → Activar`.
5. **Registrar** la entrega en `%APPDATA%\Phoenix Mercado\entrega\historial.log` y borrar los archivos de entrada (o moverlos a `entregados\<id>\`).
6. **Interfaz:** en la sección **Phoenix Sync** del menú de Link, cambia el «Fichajes — Próximamente» por una tarjeta **«Última entrega»** con:
   - fecha, resumen y estado (colocada / rechazada);
   - un botón **«Deshacer última entrega»**, que restaura el respaldo del option file y el `Player.bin` anterior (guarda una copia `Player.bin.anterior` antes de reemplazar).

## Reglas
- Nunca escribas si `PES2021.exe` no está abierto **y** la carpeta del juego no se encuentra. Con el juego cerrado sí puedes colocar, si conoces la carpeta (guárdala en la config la primera vez que detectes el juego).
- Nunca toques `sider.ini`, los `.lua` ni nada fuera de las rutas de arriba.
- Pruebas: un `Player.bin` de prueba con un cambio visible (Fralex suele usar Lamine Yamal, ID 162114, Velocidad 90/95/99). Comprueba que el aviso aparece en el overlay y que, tras pulsar Activar, cambia la Velocidad.
