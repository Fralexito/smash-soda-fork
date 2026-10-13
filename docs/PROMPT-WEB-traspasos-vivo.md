# PROMPT para el chat WEB · Traspasos en vivo (canal por anfitrión en la web)

> Pégalo **tal cual** en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). Origen: chat LINK, 2026-10-12. Diseño completo en `docs/TRASPASOS-EN-VIVO.md` de `smash-soda-fork` (rama `rediseño-phoenix-portal`). **Link ya está hecho (0.2.70, apagado por defecto) y ya habla este contrato; lo que falta es solo esto de la web.** Mientras la web no lo tenga, Link recibe `404` y no rompe nada (avisa «La web aún no tiene los traspasos en vivo» y reintenta cada 5 min).

## Qué quiere Fralex
Que un traspaso que él haga en **Editar** de PES (y guarde) aparezca solo en el PES de sus amigos. Link del anfitrión detecta el traspaso y lo manda a la web como un **lote** (JSON corto de «jugador → equipo»); los Link de los amigos que **siguen** a ese anfitrión lo recogen y lo aplican sobre su propia base. **La web es el centro** (no carpetas compartidas): guarda los lotes en orden y se los da solo a quien el anfitrión autoriza, aunque ese amigo tuviera Link apagado.

## Cómo se enlazan anfitrión y amigo (decidido)
- **Cada anfitrión tiene UN canal** (su cuenta de la web). Un canal tiene un **código corto** (`PHX-XXXXXX`) que el anfitrión comparte y que puede **rotar** (el viejo deja de servir; quienes ya seguían siguen igual).
- **Seguir con el código** → entra **al momento** (`activo`). **Seguir por ser amigo** (sin código) → queda `pendiente` hasta que el anfitrión **aprueba**. El anfitrión puede **quitar** a cualquiera o **pausar** el canal.
- **«Amigo»** = la misma relación de amistad que la web ya usa para `GET /v1/presencia/amigos` (no inventes otra).
- **Solo reciben quienes el anfitrión autoriza.** El contenido son **únicamente** ids y nombres de jugador/equipo: sin datos personales, sin bases, sin archivos.
- Un amigo puede seguir a varios anfitriones (Link usa el primero `activo`). Un canal admite hasta **25 seguidores**.

## Convenciones (las de las rutas `/v1/liga/*` que ya existen)
- **Edge Function `liga`**, rutas nuevas bajo `/v1/traspasos/*`. URL completa: `{SUPABASE_URL}/functions/v1/liga/v1/traspasos/...` (igual que `/functions/v1/liga/v1/liga/vivo`).
- **Auth:** `Authorization: Bearer <token de Link>` + `X-Phoenix-Version` + `X-Phoenix-Build`, con el mismo helper `autenticar` que `/v1/liga/vivo` (de ahí sale el `usuario` = uuid de la cuenta de la web). Sin token válido: `401 {"ok":false,"codigo":"TOKEN_INVALIDO"}`.
- Respuestas JSON `{ "ok": true, … }` o `{ "ok": false, "codigo": "…", "mensaje": "texto en español" }`. Códigos de error estables (abajo). **Importante:** un `404` **sin** `codigo` (o con `RUTA_NO_EXISTE`) significa «la ruta no existe» para Link (espera 5 min). Si lo que no existe es un canal o un usuario, responde `404` **con** `codigo: "CANAL_NO_EXISTE"`.
- Contrato **versionado**: añade una línea a `docs/LIGA-AUTOMATICA.md` («traspasos 1.0.0») y sube la versión de la función.
- Interruptor en `phoenix_config` (clave `integracion`): `traspasos_activo` (boolean, por defecto `true`). Apagado: toda ruta responde `403 {"codigo":"TRASPASOS_APAGADOS","mensaje":"Los traspasos en vivo están apagados."}`. Opcional `traspasos_anfitriones` (array de uuid): si no está vacío, solo ellos pueden crear canal y publicar (`403 SIN_PERMISO`).
- Nada se escribe desde el navegador: todo por la Edge Function (con service role) y RPC `security definer`.

## 1) Migración SQL (usa el siguiente número libre, ≥ 103)
```sql
-- Traspasos en vivo: un canal por anfitrión, seguidores (activo | pendiente) y lotes con seq por canal.
create table if not exists public.traspasos_canales (
  id          uuid primary key default gen_random_uuid(),
  anfitrion   uuid not null unique,                       -- cuenta de la web del anfitrión (misma columna de usuario que usa liga_participantes.usuario)
  codigo      text not null unique check (codigo ~ '^PHX-[A-Z2-9]{6}$'),
  activo      boolean not null default true,
  ultimo_seq  bigint  not null default 0,
  seq_min     bigint  not null default 1,                 -- el lote más antiguo que aún se conserva (retención)
  creado      timestamptz not null default now(),
  codigo_rotado_en timestamptz
);
create table if not exists public.traspasos_seguidores (
  canal_id uuid not null references public.traspasos_canales(id) on delete cascade,
  usuario  uuid not null,
  estado   text not null check (estado in ('activo','pendiente')),
  creado   timestamptz not null default now(),
  primary key (canal_id, usuario)
);
create index if not exists traspasos_seguidores_usuario on public.traspasos_seguidores(usuario);
create table if not exists public.traspasos_lotes (
  canal_id    uuid   not null references public.traspasos_canales(id) on delete cascade,
  seq         bigint not null,
  id          text   not null check (id ~ '^[0-9a-f]{24}$'),
  creado      timestamptz not null,                       -- el del lote (UTC)
  recibido_en timestamptz not null default now(),
  autor       text   not null,                            -- lo pone la web (nombre del perfil), no el cuerpo
  anfitrion   text   not null,                            -- id de Link tal cual lo mandó (Link lo usa para reconocer sus propios lotes: NO se reescribe)
  parche      text,
  base_huella text   check (base_huella is null or base_huella ~ '^[0-9a-f]{1,16}$'),
  huella      text   not null,
  cambios     jsonb  not null check (jsonb_typeof(cambios) = 'array' and jsonb_array_length(cambios) between 1 and 200),
  retirado    boolean not null default false,             -- solo el staff; deja de entregarse
  primary key (canal_id, seq),
  unique (canal_id, id)
);
-- Límites por usuario (ventana fija) para publicar, leer, probar códigos…
create table if not exists public.traspasos_limites (
  usuario uuid not null, clave text not null, ventana timestamptz not null, n int not null default 0,
  primary key (usuario, clave, ventana)
);
alter table public.traspasos_canales   enable row level security;
alter table public.traspasos_seguidores enable row level security;
alter table public.traspasos_lotes     enable row level security;
alter table public.traspasos_limites   enable row level security;
revoke all on public.traspasos_canales, public.traspasos_seguidores, public.traspasos_lotes, public.traspasos_limites from anon, authenticated;
-- (sin políticas = solo la service role; el navegador no lee ni escribe estas tablas)

-- Publicar un lote de forma ATÓMICA: idempotente por id y con seq consecutivo por canal.
create or replace function public.traspasos_publicar(p_canal uuid, p_lote jsonb, p_autor text)
returns jsonb language plpgsql security definer set search_path = public as $$
declare v_seq bigint; v_ya bigint;
begin
  perform 1 from traspasos_canales where id = p_canal for update;      -- serializa los publicadores del canal
  select seq into v_ya from traspasos_lotes where canal_id = p_canal and id = p_lote->>'id';
  if found then return jsonb_build_object('ya', true, 'seq', v_ya); end if;
  update traspasos_canales set ultimo_seq = ultimo_seq + 1 where id = p_canal returning ultimo_seq into v_seq;
  insert into traspasos_lotes(canal_id, seq, id, creado, autor, anfitrion, parche, base_huella, huella, cambios)
  values (p_canal, v_seq, p_lote->>'id', (p_lote->>'creado')::timestamptz, left(p_autor, 60), left(p_lote->>'anfitrion', 64),
          left(p_lote->>'parche', 80), nullif(p_lote->>'base_huella', ''), p_lote->>'huella', p_lote->'cambios');
  return jsonb_build_object('ya', false, 'seq', v_seq);
end $$;
revoke all on function public.traspasos_publicar(uuid, jsonb, text) from public, anon, authenticated;

-- Límite por ventana fija: devuelve 0 si pasa, o los segundos que faltan si se pasó.
create or replace function public.traspasos_limitar(p_usuario uuid, p_clave text, p_max int, p_ventana_seg int)
returns int language plpgsql security definer set search_path = public as $$
declare v_v timestamptz := to_timestamp(floor(extract(epoch from now()) / p_ventana_seg) * p_ventana_seg); v_n int;
begin
  insert into traspasos_limites(usuario, clave, ventana, n) values (p_usuario, p_clave, v_v, 1)
  on conflict (usuario, clave, ventana) do update set n = traspasos_limites.n + 1 returning n into v_n;
  if v_n > p_max then return greatest(1, ceil(extract(epoch from (v_v + make_interval(secs => p_ventana_seg) - now())))::int); end if;
  return 0;
end $$;
revoke all on function public.traspasos_limitar(uuid, text, int, int) from public, anon, authenticated;

-- Retención: 180 días y como mucho 5.000 lotes por canal. Programa esto cada día (pg_cron o la tarea que ya uses).
create or replace function public.traspasos_purgar() returns void language plpgsql security definer set search_path = public as $$
begin
  delete from traspasos_lotes l using traspasos_canales c
   where l.canal_id = c.id and (l.recibido_en < now() - interval '180 days' or l.seq <= c.ultimo_seq - 5000);
  update traspasos_canales c set seq_min = coalesce((select min(seq) from traspasos_lotes l where l.canal_id = c.id), c.ultimo_seq + 1);
  delete from traspasos_limites where ventana < now() - interval '2 days';
end $$;
revoke all on function public.traspasos_purgar() from public, anon, authenticated;
```

## 2) Rutas de la función `liga` (todas con auth de Link)

### `POST /v1/traspasos/canal` · el anfitrión crea u obtiene su canal y lo administra
Cuerpo: `{ "accion": "obtener" | "rotar_codigo" | "pausar" | "reanudar" }` (por defecto `obtener`).
- `obtener` **crea el canal si no existe** (idempotente) con un código `PHX-` + 6 caracteres de `ABCDEFGHJKLMNPQRSTUVWXYZ23456789` (único). `rotar_codigo` genera otro y guarda `codigo_rotado_en`. `pausar`/`reanudar` cambian `activo`.
- `200`:
```json
{ "ok": true,
  "canal": { "id": "6b1f0c9e-…", "codigo": "PHX-K7M2QX", "activo": true, "seguidores_max": 25 },
  "seguidores": [ { "usuario_id": "a2000000-…", "nombre": "Beto", "estado": "activo", "desde": "2026-10-12T10:00:00Z" },
                  { "usuario_id": "a3000000-…", "nombre": "Degox", "estado": "pendiente", "desde": "2026-10-12T11:00:00Z" } ] }
```
- Errores: `400 DATOS_INVALIDOS` (acción desconocida) · `403 SIN_PERMISO` (si `traspasos_anfitriones` no lo incluye) · `429`.

### `POST /v1/traspasos/seguidores` · el anfitrión aprueba o quita
Cuerpo: `{ "usuario_id": "<uuid>", "accion": "aprobar" | "quitar" }`. Solo sobre **su** canal. `aprobar` pasa `pendiente`→`activo` (si ya estaba `activo`, no hace nada); `quitar` borra la fila (sea `activo` o `pendiente`).
- `200`: la misma forma que `canal` (canal + lista de seguidores actualizada).
- Errores: `404 CANAL_NO_EXISTE` (ese usuario no te sigue / no tienes canal) · `400 DATOS_INVALIDOS`.

### `POST /v1/traspasos/seguir` · el amigo sigue a un anfitrión
Cuerpo: `{ "codigo": "PHX-K7M2QX" }` **o** `{ "anfitrion_usuario_id": "<uuid>" }`.
- Con `codigo` (canal `activo`): entra como `activo`. Con `anfitrion_usuario_id`: **solo si son amigos** y entra como `pendiente`. Si ya seguía, devuelve su estado actual (idempotente). No puede seguirse a sí mismo.
- `200`:
```json
{ "ok": true, "siguiendo": { "canal_id": "6b1f0c9e-…", "anfitrion": { "usuario_id": "a1000000-…", "nombre": "Fralex" }, "estado": "pendiente", "desde": "2026-10-12T12:00:00Z" } }
```
- Errores: `404 CANAL_NO_EXISTE` (código incorrecto, rotado o canal pausado: **mismo mensaje** para no revelar cuál) · `403 NO_ES_AMIGO` · `403 CANAL_LLENO` · `400 DATOS_INVALIDOS` · `429` (**10 intentos con código por hora y usuario**, anti fuerza bruta: `traspasos_limitar(usuario,'codigo',10,3600)`).

### `GET /v1/traspasos/siguiendo` · a quién sigo
`200 { "ok": true, "siguiendo": [ { "canal_id": "…", "anfitrion": { "usuario_id": "…", "nombre": "Fralex" }, "estado": "activo" | "pendiente", "desde": "…" } ] }` (máximo 20).

### `POST /v1/traspasos/dejar` · dejar de seguir
Cuerpo `{ "canal_id": "<uuid>" }` → `200 { "ok": true }` · `404 CANAL_NO_EXISTE` si no lo seguía.

### `POST /v1/traspasos/lotes` · el anfitrión publica un lote (en SU canal)
Cuerpo (máx. **64 KB**): `{ "lote": { … } }` con la forma de abajo. El canal es el del usuario del token (si no tiene, se crea). Valida y llama a `traspasos_publicar` (el `autor` lo pone la **web** con el nombre del perfil; ignora el del cuerpo).
- `201 { "ok": true, "seq": 12, "canal_id": "…" }` lote nuevo.
- `409 { "ok": true, "ya": true, "seq": 7, "canal_id": "…" }` ese `id` **ya estaba** → idempotente, **no** se duplica (Link lo trata como éxito).
- Errores: `400 LOTE_INVALIDO` (con el motivo en `mensaje`) · `413 DEMASIADO_GRANDE` · `403 SIN_PERMISO` · `403 CANAL_PAUSADO` (el anfitrión pausó su canal) · `429` (**30 lotes por hora y 200 por día por usuario**).

Forma del lote (formato 1):
```json
{ "formato": 1, "id": "9f8e7d6c5b4a39281716f5e4", "huella": "<sha256 hex>", "creado": "2026-10-12T17:30:00Z",
  "autor": "Fralex", "anfitrion": "3fa9c1…", "parche": "ConmeGOL 26 (base olmosjr23)", "base_huella": "358c51b8ef11",
  "cambios": [ { "jugador_id": 162114, "jugador": "Lamine Yamal", "desde_id": 108, "desde": "FC Barcelona", "hasta_id": 101, "hasta": "Arsenal FC" } ] }
```
Validación: `formato == 1`; `id` = 24 hex minúsculas; `cambios` array de **1 a 200** con `jugador_id`, `desde_id`, `hasta_id` **enteros > 0** y `desde_id != hasta_id`; textos recortados a 80; `creado` ISO-UTC válido y no más de 1 día en el futuro; `base_huella` vacía o hex ≤ 16.
**Huella** (opcional que la web la recalcule; Link **siempre** la comprueba al recibir y rechaza el lote si no cuadra): sha256 hex del texto UTF-8 con **una línea por cambio**, ordenados por `jugador_id` y luego `hasta_id`: `jugador_id|desde_id|hasta_id|jugador|desde|hasta\n`. **La web no reescribe ni reordena ningún campo del lote**: se guarda y se devuelve tal cual (salvo `autor`, `seq` y `canal_id`).

### `GET /v1/traspasos/lotes?canal_id=<uuid>&desde=<seq>&limite=100` · leer lotes
Pueden leer el **dueño** del canal (para el modo «Prueba») y los seguidores **`activo`**. Devuelve los lotes con `seq > desde`, **en orden ascendente**, sin los `retirado`, hasta `limite` (máx. 100):
```json
{ "ok": true, "canal_id": "6b1f0c9e-…", "lotes": [ { …lote… , "seq": 12 } ], "hay_mas": false,
  "seq_max": 12, "seq_min": 1, "purgado": false, "sondeo_seg": 10 }
```
- `hay_mas: true` si quedaron lotes por traer (Link pide la siguiente página enseguida: así se «pone al día»). `purgado: true` si `desde + 1 < seq_min` (la retención borró lotes que ese Link no llegó a ver; Link lo avisa). `sondeo_seg` = cada cuánto quieres que pregunte (Link lo acota a 5–120 s).
- Canal **pausado** → `200` con `lotes: []` (no entrega nada). Mismo resultado sin lotes nuevos (barato: un `select` por índice).
- Errores: `404 CANAL_NO_EXISTE` · `403 PENDIENTE_APROBACION` (sigue pero el anfitrión aún no aprobó: Link le dice «tu anfitrión aún no te ha aprobado») · `403 SIN_PERMISO` (no sigue ese canal o lo quitaron) · `429` (**12 lecturas por minuto y usuario**; Link con la ventana abierta pregunta cada ~10 s).

## 3) Códigos de error (estables) y qué hace Link con cada uno
| HTTP | `codigo` | Link muestra / hace |
|---|---|---|
| 401 | `TOKEN_INVALIDO` | «La web no reconoce este Link: vuelve a vincular tu cuenta» · espera 60 s |
| 404 | *(sin `codigo`)* / `RUTA_NO_EXISTE` | «La web aún no tiene los traspasos en vivo» · **espera 5 min**, no falla nada |
| 404 | `CANAL_NO_EXISTE` | «Elige a qué anfitrión sigues» / «ese canal ya no existe» · espera 60 s |
| 403 | `PENDIENTE_APROBACION` | «Tu anfitrión aún no te ha aprobado» · espera 30 s |
| 403 | `SIN_PERMISO`, `NO_ES_AMIGO`, `CANAL_LLENO`, `CANAL_PAUSADO`, `TRASPASOS_APAGADOS` | muestra tu `mensaje` · espera 60 s |
| 429 | `LIMITE` + `"reintentar_en": <seg>` | espera esos segundos (5–600) |
| 400/413 | `LOTE_INVALIDO`, `DATOS_INVALIDOS`, `DEMASIADO_GRANDE` | muestra tu `mensaje` · espera 30 s |
| 5xx / sin respuesta | — | «Sin conexión con la web» · espera 10, 20, 40, 80, 120 s |

## 4) Reglas
- **Idempotente por `(canal, id)`**; `seq` consecutivo por canal, nunca se reutiliza ni se reordena.
- **Privacidad:** el lote solo lleva ids y nombres de jugador/equipo. No guardes IP, rutas ni nada del PC. El nombre que ven los amigos es el del perfil del anfitrión.
- Un seguidor solo ve **lotes del canal que sigue**; el código no revela quién es el anfitrión hasta que `seguir` tiene éxito.
- Sin Realtime obligatorio. **Opcional** (la web, no Link): Broadcast **privado** `traspasos:<canal_id>`, evento `lote`, payload `{ "seq": N }`, solo para el dueño y los seguidores `activo`. Link hoy hace sondeo suave y no lo usa.
- Nada de secretos en ningún cuerpo ni en logs.

## 5) UI mínima en la web (opcional, después de las rutas)
Una sección «Traspasos en vivo» en el perfil del anfitrión: su código con botón copiar y «Código nuevo», la lista de seguidores con **Aprobar / Quitar**, interruptor de **pausa**, y los últimos 20 lotes. Sin rediseñar nada de la web.

## Hecho cuando
1. Con las rutas desplegadas, `POST /v1/traspasos/canal` crea el canal y `GET /v1/traspasos/lotes?canal_id=…&desde=0` (del dueño) devuelve `lotes: []`.
2. Publicar el mismo lote dos veces devuelve `201` y luego `409` con el mismo `seq`, y la tabla tiene **una** fila.
3. Un amigo que sigue **con código** recibe lotes; uno que sigue **por amistad** recibe `403 PENDIENTE_APROBACION` hasta que el anfitrión aprueba; uno que no sigue recibe `403 SIN_PERMISO`.
4. Con el código rotado, el viejo da `404 CANAL_NO_EXISTE`; con el canal pausado, `lotes: []`.
5. 31 publicaciones en una hora → `429` con `reintentar_en`; 11 códigos malos en una hora → `429`.
6. Un `404` de canal lleva `codigo: "CANAL_NO_EXISTE"` y una ruta inexistente **no** lo lleva.
7. `traspasos_activo = false` → `403 TRASPASOS_APAGADOS` en todas.

Al terminar: una línea en `REGISTRO.md` del repo web y **avisa al chat LINK** (para quitar el `PENDIENTE-WEB`). No toques `main`; pide permiso a Fralex antes de hacer commit o merge.
