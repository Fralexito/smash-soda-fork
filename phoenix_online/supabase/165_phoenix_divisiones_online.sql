-- Migración 165 (10-nov-2026): Divisiones del online Phoenix en la web.
-- El servidor sube (con su secreto, función phoenix_srv_subir_divisiones) la división, rating y récord
-- de cada jugador. La web lee el ranking con phoenix_ranking_online(), que NUNCA devuelve la clave del
-- servidor (para amigos sale de su IP). Cada persona ve su propia fila completa; el admin, todas.
-- Vuelta atrás: 165_phoenix_divisiones_online_REVERSA.sql.

create table if not exists public.phoenix_divisiones (
  clave_servidor text primary key,
  usuario uuid references auth.users(id) on delete set null,
  nombre_jugador text not null default '',
  division text not null default 'D12',
  rating integer not null default 1000,
  sp integer not null default 0,
  temporada integer not null default 1,
  jugados integer not null default 0,
  victorias integer not null default 0,
  empates integer not null default 0,
  derrotas integer not null default 0,
  racha_max integer not null default 0,
  partidos_total integer not null default 0,
  actualizado timestamptz not null default now()
);
create index if not exists phoenix_divisiones_rating_idx on public.phoenix_divisiones (rating desc);
alter table public.phoenix_divisiones enable row level security;
create policy phoenix_divisiones_leer on public.phoenix_divisiones for select to authenticated
  using (usuario = (select auth.uid()) or (select private.admin_seguro()));

create or replace function public.phoenix_srv_subir_divisiones(p_secreto text, p_filas jsonb)
returns integer
language plpgsql
security definer
set search_path = ''
as $$
declare v_n integer;
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  if jsonb_array_length(p_filas) > 500 then raise exception 'demasiadas filas'; end if;
  insert into public.phoenix_divisiones as d (clave_servidor, usuario, nombre_jugador, division, rating, sp,
       temporada, jugados, victorias, empates, derrotas, racha_max, partidos_total, actualizado)
  select x.clave_servidor, x.usuario, coalesce(left(x.nombre_jugador, 40), ''), coalesce(x.division, 'D12'),
         coalesce(x.rating, 1000), coalesce(x.sp, 0), coalesce(x.temporada, 1), coalesce(x.jugados, 0),
         coalesce(x.victorias, 0), coalesce(x.empates, 0), coalesce(x.derrotas, 0), coalesce(x.racha_max, 0),
         coalesce(x.partidos_total, 0), now()
  from jsonb_to_recordset(p_filas) as x(clave_servidor text, usuario uuid, nombre_jugador text, division text,
       rating integer, sp integer, temporada integer, jugados integer, victorias integer, empates integer,
       derrotas integer, racha_max integer, partidos_total integer)
  where x.clave_servidor is not null
  on conflict (clave_servidor) do update set
    usuario = coalesce(excluded.usuario, d.usuario), nombre_jugador = excluded.nombre_jugador,
    division = excluded.division, rating = excluded.rating, sp = excluded.sp, temporada = excluded.temporada,
    jugados = excluded.jugados, victorias = excluded.victorias, empates = excluded.empates,
    derrotas = excluded.derrotas, racha_max = excluded.racha_max, partidos_total = excluded.partidos_total,
    actualizado = excluded.actualizado;
  get diagnostics v_n = row_count;
  return v_n;
end;
$$;

-- Ranking público del online: sin clave del servidor ni usuario (solo lo que se ve en el juego).
create or replace function public.phoenix_ranking_online(p_limite integer default 100)
returns table (puesto bigint, nombre_jugador text, division text, rating integer, sp integer,
               victorias integer, empates integer, derrotas integer, partidos_total integer, actualizado timestamptz)
language sql
stable
security definer
set search_path = ''
as $$
  select row_number() over (order by d.rating desc, d.sp desc, d.nombre_jugador),
         d.nombre_jugador, d.division, d.rating, d.sp, d.victorias, d.empates, d.derrotas, d.partidos_total,
         d.actualizado
  from public.phoenix_divisiones d
  where d.partidos_total > 0
  order by d.rating desc, d.sp desc, d.nombre_jugador
  limit least(greatest(coalesce(p_limite, 100), 1), 500);
$$;

revoke all on function public.phoenix_srv_subir_divisiones(text, jsonb) from public;
grant execute on function public.phoenix_srv_subir_divisiones(text, jsonb) to anon, authenticated;
revoke all on function public.phoenix_ranking_online(integer) from public;
grant execute on function public.phoenix_ranking_online(integer) to anon, authenticated;
