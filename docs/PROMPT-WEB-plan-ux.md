# PROMPT para el chat WEB · Plan de experiencia de Phoenix Link (19-oct-2026)

> Copia todo este archivo en el chat WEB (repo `C:\dev\pes-liga-automatica`, rama `borrador`).
> Regla de siempre: nada se publica hasta que Fralex diga «Súbelo». Lo de Supabase, como migración nueva + SQL listo para pegar.

## Contexto

Link aplicó el plan de experiencia (`plan-phoenix-link.html`) en la rama `plan-ux` (0.3.21 – 0.3.34): pantalla INICIO con
JUGAR CON UN AMIGO / BUSCAR RIVAL, Semáforo Phoenix único, asistente de bienvenida, sentado automático, revancha, glosario
(«Conectar mi cuenta», «Mi juego», «Jugar»). Lo que sigue **no se puede hacer desde Link**: depende de la web. Link ya está
preparado para cada punto (se dice cuál). Orden sugerido: 1 → 2 → 4 → 6 → el resto.

## 1. Aprobación de anfitrión (bloqueo P0)

Hoy, sin aprobación manual del staff (`HOST_NO_AUTORIZADO`), un usuario nuevo no puede abrir sala publicada, enviar traspasos ni salir en el radar.
Fralex aún no eligió; propón y deja listas **las dos** opciones detrás de un interruptor del admin, por defecto la A:
- **A · Automática con límites:** al vincular la primera PC, la cuenta queda como anfitrión «básico» (salas de amigos y públicas con tope
  de plazas, sin oficiales) y el staff la revisa después (lista «por revisar» en el admin).
- **B · Prueba de 7 días:** anfitrión completo durante 7 días desde la primera vinculación; luego pasa a pedir aprobación.
- En ambos casos: `POST /v1/anfitrion/solicitar` (o el nombre que prefieras) para que Link pida la aprobación desde la app.

Link ya explica la pausa: lee `codigo` en el error y dice «Tu cuenta aún no está aprobada como anfitrión. Mientras tanto puedes jugar
con amigos y unirte a salas de otros». Si añades la ruta de solicitud, dímelo y Link pone el botón «Pedir aprobación».

## 2. Conectar la PC por el navegador (sin copiar códigos)

Hoy: código de instalación (24 h) + código de 6 cifras (10 min). Pedido: flujo de dispositivo.
1. Link `POST /v1/pc/emparejar/iniciar` → `{ id, url, caduca_seg }` (sin token).
2. Link abre `url` en el navegador (página de la web con la sesión iniciada): «¿Conectar Phoenix Link en este PC?» [Conectar].
3. Link sondea `GET /v1/pc/emparejar/estado?id=…` cada 2 s → `{ estado: "esperando" | "listo" | "caducado", token?, usuario? }`.
Mismo token que hoy da `web.vincular`. El código de 6 cifras sigue valiendo como respaldo.
En la web, renombra «Mi perfil › vincular PC» a **«Conectar Phoenix Link»** (Link ya dice «Conectar mi cuenta» en todas partes).

## 3. PC conectada sin ser anfitrión

Permitir conectar una PC aunque la cuenta no sea anfitrión, para lo de invitado: presencia, avisos, amigos, recibir traspasos y cambios
oficiales, salas abiertas. Las rutas de anfitrión (abrir sala publicada, enviar traspasos, resultados oficiales) siguen pidiendo aprobación.

## 4. Versiones de Link e instalador

- `link_versiones` está **vacía**: el botón ACTUALIZAR (0.3.20) nunca encuentra nada. Publica la primera versión firmada cuando Fralex
  apruebe esta rama (Link: `tools/EMPAQUETAR-RELEASE.ps1 -Subir -Notas "…"`; web: `/link-versiones/`).
- Un **instalador .exe** único (VC++, ViGEmBus, WebView2 y Parsec si faltan) en lugar del ZIP del código fuente para «Quiero ser anfitrión».
- Cabecera con la versión real: Link puede mandar `X-Phoenix-Link-Version: 0.3.34` además de `X-Phoenix-Version: 7.0.4` (que sigue igual
  para no romper `version_app_min`). Confirma que la web la acepta y la guarda por PC, y Link la añade.

## 5. Un solo interruptor de pausa

Hoy liga y traspasos se pausan con una clave y Phoenix con otra. Unifícalo (o documenta cuál manda) para que Link enseñe una sola causa.

## 6. Buscar rival: «Jugar ahora» y cuántos buscan

- Web: botón **«Jugar ahora»** que entra a la primera sala libre de la región con buen ping (sobre `radar_salas`).
- Para Link: en `GET /v1/salas` (o ruta aparte) añade `buscando: N` (cuántos están en la cola de la Sala de Duelos / `retos_matchmaking`)
  y, si es posible, entrar a la cola desde Link (`POST /v1/retos/buscar`, `DELETE` para salir). Link enseña «N personas buscan partido» en
  INICIO › BUSCAR RIVAL en cuanto exista el campo.
- Orden de salas: Link ya ordena igual que la web (invitado → amigo → con sitio → ping). Si la web cambia el criterio, avisa.

## 7. Amigos en los perfiles (sentado automático «solo mis amigos»)

`POST /v1/perfiles` devuelve la carta por `parsec_id`, pero **no el id de usuario**. Link hoy reconoce a un amigo comparando el nombre del
perfil con el de tu lista de amigos. Añade a cada perfil `usuario_id` (o mejor `es_amigo: true|false` respecto a la cuenta de la PC que pregunta).
Link ya lee `usuario_id`/`usuarioId`/`id` si llega.

## 8. Resultado en disputa → «En revisión»

Cuando dos propuestas no coinciden la web marca disputa. Devuelve el estado del partido oficial en `/v1/liga` (o donde Link ya lee los
partidos) con `estado: "disputa"` y un `url` de la ficha, y Link enseñará «En revisión» con el enlace en la tarjeta de resultado.

## 9. Fichajes: un solo camino

Decisión por defecto (Fralex no contestó): **el canal de Link** (traspasos en vivo + cambios oficiales, migraciones 125/126) es el camino
de los fichajes. Las rutas de Phoenix Sync para grupos y foto oficial quedan **dormidas** (no se borran). Publica la página
«Cambios oficiales» si sigue solo en `borrador` y haz el primer cambio oficial de prueba (hoy hay 0) para probarlo con Link en una PC real.

## 10. Lo que Link ya hace (para que la web no lo repita)

- Semáforo Phoenix, asistente de bienvenida y errores con causa + botón (`estado.web.codigo`).
- INICIO: JUGAR CON UN AMIGO (abre sala de amigos + copia enlace + `amigos.invitar` → `/v1/invitar`), BUSCAR RIVAL (sala pública).
- Sentado automático al pulsar el mando (opción «solo mis amigos»).
- Tarjeta de resultado con REVANCHA y CAMBIAR LADOS.

Cuando termines cada punto, deja en `docs/` de la web qué ruta/campo quedó y su forma exacta, para que el chat LINK lo conecte.
