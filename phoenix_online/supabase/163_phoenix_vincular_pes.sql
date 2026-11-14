-- Migración 163 (10-nov-2026): «Vincular mi PES» desde la web (pedido de Frank: lo más simple).
-- Solo AÑADE: una columna nueva en myclub_clubes, 3 tablas y 2 funciones. No toca nada de la web.
-- Cómo se une el club del online a la cuenta de la web (Frank: «nombre, pero con un código de
-- comprobación»):
--   1) Si el nombre del jugador en el PES coincide con el username de la web, la web le enseña su
--      club («Encontramos tu club…», phoenix_clubes_sugeridos).
--   2) Lo confirma con el código de 6 cifras que el servidor le muestra en los Avisos del juego
--      (phoenix_vincular). Sin el código nadie se queda con un club ajeno. Si el nombre no coincide,
--      el código solo también sirve.
-- El servidor lee phoenix_duenos y sube el club con su usuario (así la persona lo ve en su perfil).
-- APLICADA el 10-nov ~22:00 UTC con execute_sql (no figura en la lista de migraciones de Supabase).
-- Vuelta atrás: 163_phoenix_vincular_pes_REVERSA.sql.

alter table public.myclub_clubes add column if not exists nombre_jugador text not null default '';

create table if not exists public.phoenix_vinculos (
  codigo text primary key check (codigo ~ '^[0-9]{6}$'),
  clave_servidor text not null unique,
  caduca timestamptz not null,
  creado timestamptz not null default now()
);
create table if not exists public.phoenix_vinculo_intentos (
  id bigint generated always as identity primary key,
  usuario uuid not null references auth.users(id) on delete cascade,
  hora timestamptz not null default now()
);
create index if not exists phoenix_vinculo_intentos_idx on public.phoenix_vinculo_intentos (usuario, hora);
alter table public.phoenix_vinculos enable row level security;
alter table public.phoenix_vinculo_intentos enable row level security;
-- sin políticas: solo el servidor (clave de servicio) y las funciones de abajo las tocan

-- Clubes del online sin dueño cuyo jugador se llama igual que mi username de la web.
create or replace function public.phoenix_clubes_sugeridos()
returns table (clave_servidor text, nombre text, nombre_jugador text, gp integer, partidos integer)
language sql
stable
security definer
set search_path = ''
as $$
  select c.clave_servidor, c.nombre, c.nombre_jugador, c.gp, c.partidos
  from public.myclub_clubes c
  join public.perfiles p on p.id = (select auth.uid())
  where c.usuario is null and c.nombre_jugador <> ''
    and lower(c.nombre_jugador) = lower(p.username)
    and not exists (select 1 from public.phoenix_duenos d where d.clave_servidor = c.clave_servidor)
  limit 5;
$$;

-- Dueño de cada club (lo escribe phoenix_vincular; el servidor lo lee y sube el club con usuario).
-- Va en su propia tabla para no tener que modificar filas de myclub_clubes desde la web.
create table if not exists public.phoenix_duenos (
  clave_servidor text primary key,
  usuario uuid not null references auth.users(id) on delete cascade,
  creado timestamptz not null default now()
);
create index if not exists phoenix_duenos_usuario_idx on public.phoenix_duenos (usuario);
alter table public.phoenix_duenos enable row level security;
create policy phoenix_duenos_propio on public.phoenix_duenos for select to authenticated
  using (usuario = (select auth.uid()) or (select private.admin_seguro()));

-- Código de los Avisos del juego. Máximo 10 intentos por hora y persona. Un club = un dueño.
create or replace function public.phoenix_vincular(p_codigo text)
returns text
language plpgsql
security definer
set search_path = ''
as $$
declare
  v_uid uuid := auth.uid();
  v_clave text;
  v_hecho text;
  v_nombre text;
begin
  if v_uid is null then
    raise exception 'Hace falta iniciar sesión';
  end if;
  if (select count(*) from public.phoenix_vinculo_intentos
      where usuario = v_uid and hora > now() - interval '1 hour') >= 10 then
    raise exception 'Demasiados intentos; espera una hora';
  end if;
  insert into public.phoenix_vinculo_intentos (usuario) values (v_uid);
  select v.clave_servidor into v_clave from public.phoenix_vinculos v
  where v.codigo = trim(p_codigo) and v.caduca > now();
  if v_clave is null then
    raise exception 'Código no válido o caducado';
  end if;
  insert into public.phoenix_duenos (clave_servidor, usuario) values (v_clave, v_uid)
  on conflict (clave_servidor) do nothing
  returning clave_servidor into v_hecho;
  if v_hecho is null then
    raise exception 'Ese club ya tiene dueño';
  end if;
  select coalesce(nullif(c.nombre, ''), 'tu club') into v_nombre
  from public.myclub_clubes c where c.clave_servidor = v_clave;
  return coalesce(v_nombre, 'tu club');
end;
$$;

revoke all on function public.phoenix_clubes_sugeridos() from public, anon;
revoke all on function public.phoenix_vincular(text) from public, anon;
grant execute on function public.phoenix_clubes_sugeridos() to authenticated;
grant execute on function public.phoenix_vincular(text) to authenticated;
