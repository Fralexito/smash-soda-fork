# Prompt para el chat LINK — botón «Modo de recarga» (phoenix.lua v0.18)

Pega esto en el chat del frente LINK (rama `rediseño-phoenix-portal`, carpeta `SmashSoda/`).

---

Hola. Desde el frente SYNC (rama `mercado-fase0`) necesitamos un **botón de tres opciones en Phoenix Link** para que cada usuario elija cómo recarga el juego los fichajes. No toques nada de `PhoenixSync/`; solo hace falta que Link escriba un archivo.

**Qué escribir:** `<juego>\SiderAddons\content\phoenix\modo.txt` (la misma carpeta donde Link ya escribe `avisos.txt`).
- Contenido: **una sola palabra**: `ACTIVAR`, `AUTO-FICHAJES` o `AUTO-SIEMPRE`.
- UTF-8 sin BOM, sin nada más (un salto de línea final se tolera), de 64 bytes como mucho.
- **Escritura atómica:** escribir `modo.tmp` y renombrarlo a `modo.txt` (igual que `avisos.txt`).
- Si es posible, escribirlo también en `ConmeGol Extras\<modo activo>\SiderAddons\content\phoenix\`, porque el switcher de ConmeGOL copia esa carpeta encima.
- El juego lo nota en unos 2 segundos, esté abierto o no.

**El botón (texto sugerido, en español):**
- 🟢 **Activar (recomendado, viene por defecto)**: «Los fichajes se cargan solo cuando pulsas Partido → Datos Actual. en vivo → Activar.»
- 🔵 **Automático: solo fichajes nuevos**: «Activar sigue funcionando. Además, cuando haya fichajes nuevos, el juego los carga solo al volver al menú principal y entrar a un modo.»
- 🟣 **Automático: siempre**: «Activar sigue funcionando. Además, el juego recarga cada vez que vuelves al menú principal y entras a un modo (tarda un poco más).»

**Por defecto:** ACTIVAR. Si el usuario nunca eligió, Link puede no escribir nada (sin archivo = ACTIVAR) o escribir `ACTIVAR`. Guardar la elección en los ajustes de Link y volver a escribir `modo.txt` al arrancar.

**No hace falta** que Link escriba `recargar.txt`: ese aviso («hay fichajes nuevos») lo escribe Phoenix Sync.

Contrato completo: `COORDINACION.md` → «Contratos compartidos» y `PhoenixSync/base-conocimiento/31-MODOS-DE-RECARGA.md` (rama `mercado-fase0`). Compila sin errores antes de commitear y anota una línea en `REGISTRO-LINK.md`.
