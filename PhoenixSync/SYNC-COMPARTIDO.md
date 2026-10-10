# Option file compartido entre PCs — informe (etapa 1, 2026-10-09)

## Qué hace (en palabras simples)
- **Fichaje = «operación»**: un mensaje pequeño («jugador 126624 del equipo 172 al 108»). Se comparte eso, no el archivo entero. Así, si FRALEX ficha a X y el amigo ficha a Y a la vez, **se suman los dos**.
- **Va en los dos sentidos**: cualquier PC del grupo publica y cualquier otra recibe.
- **Origen** (quien ficha): Sync mira el `EDIT00000000`; cuando deja de cambiar 10 s, compara las plantillas con la última «foto» y sube cada diferencia.
- **Destino** (quien recibe): baja las operaciones nuevas, hace **respaldo**, las aplica en una copia de SU option file y la deja en `entrega\` para que **Phoenix Link** la coloque y avise en el juego.
- **Modo** (automático / con autorización): lo decide **solo el admin desde la web**. Si la web no responde → se actúa **con autorización** (lo seguro).

## Protecciones
| Riesgo | Qué hace Sync |
|---|---|
| Bucle (ping-pong) | Lo recibido nunca se vuelve a subir: ids propios/aplicados y huellas (sha256) de los archivos que llegaron de fuera. Probado con dos PCs simuladas. |
| Dos fichajes del mismo jugador a la vez | «conflicto»: no se pisa nada, gana el que ya estaba; queda en el historial y avisa «1 cambio omitido: conflicto». |
| Aplicar dos veces | Idempotente: si el jugador ya está en el destino, no hace nada. |
| Parche distinto | Cada operación lleva el parche y la **huella de la tabla de equipos**. Si no coincide: «incompatible», no se toca nada. |
| Archivo a medias | Espera a que quede quieto y que se pueda leer entero. |
| Web caída / rutas que aún no existen (404) | Espera (2, 4, 8… hasta 300 s; 404 → 5 min), no se rompe, modo autorización. |
| Respaldo imposible (sin espacio…) | **No aplica** y avisa «no se pudo respaldar: …». |

## Respaldos (en cada PC)
`%APPDATA%\Phoenix Mercado\respaldos\<AAAA-MM-DD_HHMMSS>\` con `save\` (la carpeta save completa), `phoenix-db\<modo>\Player.bin` y `manifiesto.json` (ruta original, tamaño, sha256, motivo). Antes de cada aplicación, uno diario y el manual. Retención: 10 últimos + el primero de cada uno de los últimos 7 días. Nunca toca los respaldos de Link.

## Archivos
- `core/RutasJuego` (multiparche: Documentos normal y OneDrive, cualquier edición/steamid, cualquier modo del switcher con Phoenix-DB)
- `core/Respaldos`, `core/Entrega` (formato exacto de Link), `core/Grupo` (operaciones, choques, estado), `core/ClienteSync` (web), `core/SyncCompartido` (motor)
- `app/ComandosSync.cpp` (comandos `sync-…` de consola), `windows/Plataforma` (Documentos y PES2021.exe abierto)
- Pruebas: `pruebas/compartido.cpp` → `PhoenixSyncPruebasCompartido.exe`

## Pendiente
1. **WEB**: publicar §26 (`prompts/PROMPT-WEB-sync-compartido.md`, con SQL).
2. Prueba real con dos PCs (sección 7 del encargo).
3. Interfaz: hoy son comandos de consola; luego, tarjeta en la sección «Phoenix Sync» de Link (prompt al chat LINK).
4. **Qué archivo manda en las plantillas (prueba de FRALEX, mañana del 2026-10-09):** cambiar `PlayerAssignment.bin` (Lamine → Real Madrid) NO movió a Lamine; cambiar el **option file** SÍ. ⇒ Sync aplica los fichajes al option file (correcto). Queda una duda abierta: la nota de las 06:54 (PRUEBAS.md) vio plantillas de la base al usar «Datos Actual. en vivo». Hay que repetir la prueba de fichaje usando ese botón para confirmar que también ahí manda el option file.
