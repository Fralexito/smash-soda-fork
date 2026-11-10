-- Migración 162 (10-nov-2026): Phoenix Online en Supabase. APLICADA con permiso de Frank
-- («Si puedes usar supabase y github o lo que necesites, ni me pidas permiso solo informarme»).
-- Solo AÑADE tablas y funciones nuevas: no toca nada de la web.
-- Vuelta atrás (si hiciera falta): ver 162_phoenix_online_myclub_REVERSA.sql.
--
-- Idea (docs/PLAN_MULTIUSUARIO.md):
--   * El servidor sigue con su SQLite local; un «espejo» sube aquí cada club cambiado.
--   * Las reglas de myClub se editan desde la web (solo admin) y el servidor las lee.
--   * Los movimientos (dar GP/monedas) los crea un admin y el servidor los aplica y marca.
--   * Pases de identidad: la persona pide un pase en la web; Link lo entrega al juego;
--     el servidor lo valida. Se guarda solo el hash del pase, nunca el pase en claro.

create table if not exists public.myclub_clubes (
  id bigint generated always as identity primary key,
  clave_servidor text not null unique,           -- clave de la persona en el servidor (hoy SteamID/IP; luego el pase)
  usuario uuid references auth.users(id) on delete set null,
  nombre text not null default '',
  gp integer not null default 0,
  monedas integer not null default 0,
  partidos integer not null default 0,
  racha integer not null default 0,
  nivel_com integer not null default 0,
  datos jsonb not null default '{}'::jsonb,      -- club completo tal como lo guarda el servidor
  actualizado timestamptz not null default now()
);
create index if not exists myclub_clubes_usuario_idx on public.myclub_clubes (usuario);

create table if not exists public.myclub_reglas (
  id smallint primary key default 1 check (id = 1),
  reglas jsonb not null default '{}'::jsonb,
  actualizado timestamptz not null default now(),
  actualizado_por uuid references auth.users(id) on delete set null
);
insert into public.myclub_reglas (id) values (1) on conflict (id) do nothing;

create table if not exists public.myclub_movimientos (
  id bigint generated always as identity primary key,
  clave_servidor text not null,
  tipo text not null check (tipo in ('gp', 'monedas', 'boletos', 'recuperar')),
  cantidad integer not null check (cantidad between -10000000 and 10000000),
  motivo text not null default '' check (char_length(motivo) <= 200),
  creado_por uuid references auth.users(id) on delete set null,
  creado timestamptz not null default now(),
  aplicado timestamptz
);
create index if not exists myclub_movimientos_pendientes_idx
  on public.myclub_movimientos (clave_servidor) where aplicado is null;

create table if not exists public.phoenix_pases (
  hash text primary key,                          -- sha256 del pase (el pase en claro solo lo ve su dueño una vez)
  usuario uuid not null references auth.users(id) on delete cascade,
  caduca timestamptz not null,
  usado timestamptz,
  creado timestamptz not null default now()
);
create index if not exists phoenix_pases_usuario_idx on public.phoenix_pases (usuario);

alter table public.myclub_clubes enable row level security;
alter table public.myclub_reglas enable row level security;
alter table public.myclub_movimientos enable row level security;
alter table public.phoenix_pases enable row level security;

-- Clubes: cada persona ve el suyo; los admin ven todos. Escribe solo el servidor (clave de servicio).
create policy myclub_clubes_leer on public.myclub_clubes for select to authenticated
  using (usuario = (select auth.uid()) or (select private.admin_seguro()));

-- Reglas: cualquiera con sesión las lee; solo admin las cambia.
create policy myclub_reglas_leer on public.myclub_reglas for select to authenticated using (true);
create policy myclub_reglas_admin on public.myclub_reglas for update to authenticated
  using ((select private.admin_seguro())) with check ((select private.admin_seguro()));

-- Movimientos: solo admin los crea y los ve (el servidor los aplica con la clave de servicio).
create policy myclub_mov_admin_leer on public.myclub_movimientos for select to authenticated
  using ((select private.admin_seguro()));
create policy myclub_mov_admin_crear on public.myclub_movimientos for insert to authenticated
  with check ((select private.admin_seguro()) and aplicado is null);

-- Pases: cada uno ve los suyos (sin el pase en claro). Se crean con la función de abajo.
create policy phoenix_pases_propio on public.phoenix_pases for select to authenticated
  using (usuario = (select auth.uid()));

-- Pide un pase de 10 minutos para entrar al online. Devuelve el pase en claro UNA vez.
create or replace function public.phoenix_pedir_pase()
returns text
language plpgsql
security definer
set search_path = ''
as $$
declare
  v_uid uuid := auth.uid();
  v_pase text;
begin
  if v_uid is null then
    raise exception 'Hace falta iniciar sesión';
  end if;
  -- máximo 5 pases vivos por persona (evita abusos)
  delete from public.phoenix_pases where usuario = v_uid and (caduca < now() or usado is not null);
  if (select count(*) from public.phoenix_pases where usuario = v_uid) >= 5 then
    raise exception 'Demasiados pases pedidos; espera unos minutos';
  end if;
  v_pase := encode(extensions.gen_random_bytes(18), 'hex');
  insert into public.phoenix_pases (hash, usuario, caduca)
  values (encode(extensions.digest(v_pase, 'sha256'), 'hex'), v_uid, now() + interval '10 minutes');
  return v_pase;
end;
$$;
revoke all on function public.phoenix_pedir_pase() from public, anon;
grant execute on function public.phoenix_pedir_pase() to authenticated;
