-- BORRADOR (05-nov-2026). NO APLICADO. Requiere SÍ de Frank y revisión de Cuenta 2.
-- Migración 162: datos de myClub de Phoenix Online (espejo del servidor) y reglas editables.

create table if not exists public.myclub_clubes (
  usuario uuid primary key references auth.users(id) on delete cascade,
  datos jsonb not null,
  gp integer not null default 0,
  partidos integer not null default 0,
  actualizado timestamptz not null default now()
);

create table if not exists public.myclub_reglas (
  id smallint primary key default 1 check (id = 1),
  reglas jsonb not null default '{}'::jsonb,
  actualizado timestamptz not null default now()
);

create table if not exists public.myclub_movimientos (
  id bigint generated always as identity primary key,
  usuario uuid not null references auth.users(id) on delete cascade,
  tipo text not null check (tipo in ('gp', 'monedas')),
  cantidad integer not null,
  motivo text not null default '',
  aplicado boolean not null default false,
  creado timestamptz not null default now()
);

create table if not exists public.phoenix_pases (
  token text primary key,
  usuario uuid not null references auth.users(id) on delete cascade,
  caduca timestamptz not null,
  usado boolean not null default false
);

alter table public.myclub_clubes enable row level security;
alter table public.myclub_reglas enable row level security;
alter table public.myclub_movimientos enable row level security;
alter table public.phoenix_pases enable row level security;

-- Cada persona ve su club; el servidor escribe con la clave de servicio (salta RLS).
create policy myclub_clubes_propio on public.myclub_clubes
  for select using (auth.uid() = usuario);
-- Reglas: lectura pública; escritura solo servidor/admin (clave de servicio).
create policy myclub_reglas_leer on public.myclub_reglas for select using (true);
-- Movimientos y pases: cada uno ve los suyos.
create policy myclub_mov_propio on public.myclub_movimientos
  for select using (auth.uid() = usuario);
create policy phoenix_pases_propio on public.phoenix_pases
  for select using (auth.uid() = usuario);
