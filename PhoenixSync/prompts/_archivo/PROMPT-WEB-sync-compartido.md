# Prompt para el chat WEB — «Sync compartido» (contrato v1.8.0, §26)

Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). **Si ese chat no puede tocar Supabase, que te pase el SQL y lo corres tú a mano** (abajo va un SQL listo para revisar).

---

Hola. Soy el frente SYNC (Phoenix Sync, rama `mercado-fase0`). Ya está hecho y probado en el programa (Linux + compilación para Windows, 135/135 pruebas, incluida una simulación de dos PCs) un **motor de fichajes compartidos entre PCs**. Necesito que la web publique estas rutas en la API de Phoenix Link (`/functions/v1/phoenix`), con el **token `phx_` de siempre** (`Authorization: Bearer …`, cabecera `X-Phoenix-Version`), el sobre de siempre `{ ok, datos }` / `{ ok:false, error:{ codigo, mensaje, reintentar_en } }` y la política del §0 (429 con `reintentar_en`).

**Idea:** se comparte la OPERACIÓN (un fichaje), no el archivo. Cada PC la aplica en su propio option file. Así dos fichajes hechos a la vez no se pisan.

## Rutas (nombres finales los decides tú; si cambias algo, avísame y me ajusto)
1. `GET /v1/sync/config?grupo_id=…` → `datos: { modo: "automatico"|"autorizacion", actualizado_por, actualizado_en, etag }`. Con `If-None-Match` igual al etag → **304** sin cuerpo.
2. `PUT /v1/sync/config` `{ grupo_id, modo }` → **solo el admin del grupo** (FRALEX). Otro usuario → `403 SOLO_ADMIN`. Grupo nuevo = `autorizacion` por defecto. Botón/interruptor en el panel web para el admin.
3. `POST /v1/sync/operaciones` cuerpo:
   `{ grupo_id, id, tipo:"mover"|"quitar", jugador_id, equipo_origen, equipo_destino, dorsal, autor_pc, creado_en, base, sha_resultado, resumen, compat:{ parche, huella_base, formato:1 } }`
   → `datos: { seq }`. La web pone `seq` (creciente) y `autor` (nombre del dueño del token). Mismo `id` dos veces en el grupo → `409 OPERACION_DUPLICADA` (el programa lo toma como «ya subida»). Sin permiso de publicar → `403 SIN_PERMISO`.
4. `GET /v1/sync/operaciones?grupo_id=…&desde=<seq>` → `datos: { operaciones:[ {…lo de arriba…, seq, autor} ], hasta:<seq máx del grupo>, etag }`, ordenadas por `seq`, máx. 200 por llamada. `If-None-Match` → 304.
5. `POST /v1/sync/operaciones/aplicada` `{ grupo_id, id, estado:"aplicada"|"conflicto"|"omitida"|"pendiente"|"rechazada", motivo }` → `datos:{ ok:true }`. Se guarda por (operación, usuario); repetir solo actualiza. Op inexistente → `404 NO_EXISTE`.
6. (Para el panel) `GET /v1/sync/grupos` → mis grupos `{ id, nombre, rol, modo }`, y gestión de miembros para el admin (quién puede publicar / aplicar).

**Reglas:** nadie fuera del grupo ve ni sube nada (`403 NO_ES_MIEMBRO`). Un miembro con `puede_publicar=false` no publica. El modo **no** se puede cambiar desde la PC: solo el admin, desde la web. Mientras estas rutas no existan, el programa recibe 404 y espera 5 min sin romperse (y actúa en «autorización»).

Anota en `contrato-v1.md` como §26 (v1.8.0) y en tu `REGISTRO.md`. Cuando esté publicado, pásale a FRALEX el texto final para el chat SYNC.

## SQL propuesto (revísalo; ajusta la referencia a tu tabla de usuarios)
```sql
-- Grupos de Sync compartido
create table if not exists public.sync_grupos (
  id uuid primary key default gen_random_uuid(),
  nombre text not null,
  admin_id uuid not null,                       -- usuario admin (FRALEX); FK a tu tabla de usuarios
  modo text not null default 'autorizacion' check (modo in ('automatico','autorizacion')),
  modo_actualizado_por text,
  modo_actualizado_en timestamptz,
  creado_en timestamptz not null default now()
);

create table if not exists public.sync_miembros (
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  usuario_id uuid not null,                     -- FK a tu tabla de usuarios
  rol text not null default 'miembro' check (rol in ('admin','miembro')),
  puede_publicar boolean not null default true,
  puede_aplicar boolean not null default true,
  agregado_en timestamptz not null default now(),
  primary key (grupo_id, usuario_id)
);

create table if not exists public.sync_operaciones (
  seq bigint generated always as identity primary key,
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  op_id text not null check (length(op_id) between 1 and 64),
  autor_id uuid not null,
  autor_nombre text not null,
  datos jsonb not null,                         -- la operación tal como la mandó la PC
  creado_en timestamptz not null default now(),
  unique (grupo_id, op_id)
);
create index if not exists sync_operaciones_grupo_seq on public.sync_operaciones (grupo_id, seq);

create table if not exists public.sync_confirmaciones (
  grupo_id uuid not null references public.sync_grupos(id) on delete cascade,
  op_id text not null,
  usuario_id uuid not null,
  estado text not null check (estado in ('aplicada','conflicto','omitida','pendiente','rechazada')),
  motivo text,
  actualizado_en timestamptz not null default now(),
  primary key (grupo_id, op_id, usuario_id)
);

-- Solo la función del servidor (service role) toca estas tablas.
alter table public.sync_grupos enable row level security;
alter table public.sync_miembros enable row level security;
alter table public.sync_operaciones enable row level security;
alter table public.sync_confirmaciones enable row level security;
```
El etag puede ser `"o" || max(seq)` del grupo para operaciones y `md5(modo || modo_actualizado_en)` para la config.
