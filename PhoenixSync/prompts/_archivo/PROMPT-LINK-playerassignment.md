# Prompt para el chat LINK — aceptar `PlayerAssignment.bin` en las entregas de Sync

Pégalo en el chat LINK (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`). Es un cambio pequeño en `phoenix/link/Entrega.h/.cpp`.

---

Hola. Soy el frente SYNC. FRALEX comprobó hoy (2026-10-09, 20:08) que con **Partido → Datos Actual. en vivo → Activar** el juego toma los equipos de la **base** (`PlayerAssignment.bin` de Phoenix-DB) y no del option file. Para que un fichaje del grupo aparezca con «Activar», Sync va a mandar en `entrega\` también ese archivo. Necesito que Link lo acepte, **con las mismas reglas que ya usa para `Player.bin`**:

1. **Archivo permitido nuevo:** `PlayerAssignment.bin` (además de `Player.bin` y `EDIT00000000`). Sigue siendo una lista cerrada: cualquier otro nombre se rechaza.
2. **Comprobaciones:** sha256 igual al de `entrega.json` y «WESYS» en los primeros 16 bytes (igual que `Player.bin`).
3. **Destino:** `…\livecpk\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin`, en **las mismas carpetas Phoenix-DB a las que hoy copias `Player.bin`** (raíz del juego y modos del switcher). Solo si la carpeta `pesdb` ya existe: **nunca crear carpetas del juego**.
4. **Seguridad igual que `Player.bin`:** colocación atómica (`.tmp` → reemplazo, reintentos), guardar `PlayerAssignment.bin.anterior` antes de reemplazar, y que **«Deshacer última entrega»** también lo devuelva.
5. **Todo o nada:** si cualquier archivo de la entrega falla, no se coloca ninguno (como hoy).
6. **Aviso en el juego:** el mismo de siempre (`⚡ Datos nuevos de Phoenix: <resumen>. Partido → Datos Actual. en vivo → Activar`).
7. Una entrega puede traer solo `PlayerAssignment.bin`, o junto con `EDIT00000000`, o con `Player.bin`.

**Compatibilidad:** Sync NO mandará `PlayerAssignment.bin` hasta que FRALEX lo active en cada PC con `PhoenixSync sync-base si`. Así, un Link viejo nunca recibe este archivo y no rechaza nada.

Pruebas que pido: entrega con los dos archivos → los dos colocados y `.anterior` guardado; sha256 mal en `PlayerAssignment.bin` → no se coloca nada; sin carpeta `pesdb` → rechazo claro; «Deshacer» devuelve también `PlayerAssignment.bin`. Compila sin errores antes de commitear y anota en `REGISTRO-LINK.md` y en `COORDINACION.md`. Cuando esté listo, avisa a FRALEX para que active `sync-base si`.
