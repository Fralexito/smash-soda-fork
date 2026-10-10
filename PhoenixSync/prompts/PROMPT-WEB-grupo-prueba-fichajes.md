# Prompt para el chat WEB — Grupo de prueba de fichajes con «Activar»

10 de octubre de 2026. De: chat SYNC. Estado: **PENDIENTE-WEB**.

Hola, chat WEB. FRALEX quiere probar hoy los fichajes en vivo entre su PC y la de un amigo.

**Comprobado desde Sync (sin token):**
- `GET/POST /v1/sync/config` y `/v1/sync/operaciones` → 401 (publicadas ✅).
- `POST /v1/sync/operaciones/aplicada` → 405 con GET (publicada ✅).
- `GET /v1/sync/grupos` → **404 (falta)**.
- En la base hay 2 filas en `sync_grupos`, 3 en `sync_miembros`, 2 en `sync_config`, 0 operaciones. No sé cuál es el de FRALEX y su amigo.

**Lo que hace falta (en este orden):**
1. Un grupo «Prueba fichajes FRALEX» con **FRALEX como admin** y **su amigo como miembro** (los dos con `puede_publicar` y `puede_aplicar` = sí). Si uno de los 2 grupos existentes ya es ese, reutilizarlo.
2. Su `sync_config` en modo **`automatico`** (lo pide FRALEX para la prueba).
3. Que FRALEX pueda **ver y copiar el id del grupo** (uuid) en el panel web, para escribir en cada PC: `PhoenixSync sync-grupo <id>`.
4. Publicar `GET /v1/sync/grupos` → `datos: [ { id, nombre, rol, modo } ]` (mis grupos, según el token). Contrato en `prompts/PROMPT-WEB-sync-compartido.md`, punto 6.

**Para avisar a SYNC al terminar:** el id del grupo, el modo y quiénes son miembros. No pegues tokens ni secretos.
