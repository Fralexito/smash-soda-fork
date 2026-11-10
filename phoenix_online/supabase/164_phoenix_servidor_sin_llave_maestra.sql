-- Migración 164 (10-nov-2026): el servidor Phoenix Online habla con Supabase SIN la clave service_role.
-- Usa la clave pública de la web y un SECRETO PROPIO generado en la PC del servidor; aquí solo se guarda
-- su huella (sha256). Cada función comprueba el secreto y hace SOLO su tarea (permisos mínimos): si el
-- secreto se filtrara, nadie podría tocar el resto de la base de la web.
-- Vuelta atrás: 164_phoenix_servidor_sin_llave_maestra_REVERSA.sql.

create table if not exists public.phoenix_servidores (
  hash text primary key,
  nombre text not null default '',
  creado timestamptz not null default now()
);
alter table public.phoenix_servidores enable row level security;
-- sin políticas: nadie la lee ni la escribe desde la web

create or replace function private.phoenix_servidor_ok(p_secreto text)
returns boolean
language sql
stable
security definer
set search_path = ''
as $$
  select p_secreto is not null and length(p_secreto) >= 32 and exists (
    select 1 from public.phoenix_servidores s
    where s.hash = encode(extensions.digest(p_secreto, 'sha256'), 'hex'));
$$;

-- Clubes cambiados (upsert por clave_servidor). El dueño solo se pone, nunca se borra.
create or replace function public.phoenix_srv_subir_clubes(p_secreto text, p_filas jsonb)
returns integer
language plpgsql
security definer
set search_path = ''
as $$
declare v_n integer;
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  if jsonb_array_length(p_filas) > 500 then raise exception 'demasiadas filas'; end if;
  insert into public.myclub_clubes as c (clave_servidor, nombre, nombre_jugador, gp, monedas, partidos, racha,
                                         nivel_com, datos, actualizado, usuario)
  select x.clave_servidor, coalesce(x.nombre, ''), coalesce(x.nombre_jugador, ''), coalesce(x.gp, 0),
         coalesce(x.monedas, 0), coalesce(x.partidos, 0), coalesce(x.racha, 0), coalesce(x.nivel_com, 0),
         coalesce(x.datos, '{}'::jsonb), now(), x.usuario
  from jsonb_to_recordset(p_filas) as x(clave_servidor text, nombre text, nombre_jugador text, gp integer,
       monedas integer, partidos integer, racha integer, nivel_com integer, datos jsonb, usuario uuid)
  where x.clave_servidor is not null
  on conflict (clave_servidor) do update set
    nombre = excluded.nombre, nombre_jugador = excluded.nombre_jugador, gp = excluded.gp,
    monedas = excluded.monedas, partidos = excluded.partidos, racha = excluded.racha,
    nivel_com = excluded.nivel_com, datos = excluded.datos, actualizado = excluded.actualizado,
    usuario = coalesce(excluded.usuario, c.usuario);
  get diagnostics v_n = row_count;
  return v_n;
end;
$$;

create or replace function public.phoenix_srv_reglas(p_secreto text)
returns table (reglas jsonb, actualizado timestamptz)
language plpgsql
stable
security definer
set search_path = ''
as $$
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  return query select r.reglas, r.actualizado from public.myclub_reglas r where r.id = 1;
end;
$$;

create or replace function public.phoenix_srv_movimientos(p_secreto text)
returns table (id bigint, clave_servidor text, tipo text, cantidad integer, motivo text)
language plpgsql
stable
security definer
set search_path = ''
as $$
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  return query select m.id, m.clave_servidor, m.tipo, m.cantidad, m.motivo
    from public.myclub_movimientos m where m.aplicado is null order by m.id limit 200;
end;
$$;

-- Marca un movimiento como aplicado; true solo la primera vez (así nunca se da dos veces).
create or replace function public.phoenix_srv_marcar_movimiento(p_secreto text, p_id bigint)
returns boolean
language plpgsql
security definer
set search_path = ''
as $$
declare v_id bigint;
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  update public.myclub_movimientos set aplicado = now() where id = p_id and aplicado is null
  returning id into v_id;
  return v_id is not null;
end;
$$;

create or replace function public.phoenix_srv_subir_codigos(p_secreto text, p_filas jsonb)
returns integer
language plpgsql
security definer
set search_path = ''
as $$
declare v_n integer;
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  if jsonb_array_length(p_filas) > 500 then raise exception 'demasiadas filas'; end if;
  insert into public.phoenix_vinculos as v (codigo, clave_servidor, caduca)
  select x.codigo, x.clave_servidor, x.caduca
  from jsonb_to_recordset(p_filas) as x(codigo text, clave_servidor text, caduca timestamptz)
  on conflict (clave_servidor) do update set codigo = excluded.codigo, caduca = excluded.caduca;
  get diagnostics v_n = row_count;
  return v_n;
end;
$$;

create or replace function public.phoenix_srv_duenos(p_secreto text, p_claves text[])
returns table (clave_servidor text, usuario uuid)
language plpgsql
stable
security definer
set search_path = ''
as $$
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  return query select d.clave_servidor, d.usuario from public.phoenix_duenos d
    where d.clave_servidor = any (p_claves[1:500]);
end;
$$;

create or replace function public.phoenix_srv_canjear_pase(p_secreto text, p_hash text)
returns uuid
language plpgsql
security definer
set search_path = ''
as $$
declare v_usuario uuid;
begin
  if not private.phoenix_servidor_ok(p_secreto) then raise exception 'servidor no autorizado'; end if;
  update public.phoenix_pases set usado = now()
  where hash = p_hash and usado is null and caduca > now()
  returning usuario into v_usuario;
  return v_usuario;
end;
$$;

revoke all on function private.phoenix_servidor_ok(text) from public, anon, authenticated;
revoke all on function public.phoenix_srv_subir_clubes(text, jsonb) from public;
revoke all on function public.phoenix_srv_reglas(text) from public;
revoke all on function public.phoenix_srv_movimientos(text) from public;
revoke all on function public.phoenix_srv_marcar_movimiento(text, bigint) from public;
revoke all on function public.phoenix_srv_subir_codigos(text, jsonb) from public;
revoke all on function public.phoenix_srv_duenos(text, text[]) from public;
revoke all on function public.phoenix_srv_canjear_pase(text, text) from public;
grant execute on function public.phoenix_srv_subir_clubes(text, jsonb) to anon, authenticated;
grant execute on function public.phoenix_srv_reglas(text) to anon, authenticated;
grant execute on function public.phoenix_srv_movimientos(text) to anon, authenticated;
grant execute on function public.phoenix_srv_marcar_movimiento(text, bigint) to anon, authenticated;
grant execute on function public.phoenix_srv_subir_codigos(text, jsonb) to anon, authenticated;
grant execute on function public.phoenix_srv_duenos(text, text[]) to anon, authenticated;
grant execute on function public.phoenix_srv_canjear_pase(text, text) to anon, authenticated;
