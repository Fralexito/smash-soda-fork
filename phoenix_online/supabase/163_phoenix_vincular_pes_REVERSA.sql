-- Vuelta atrás de la 163 (solo si Frank lo pide).
drop function if exists public.phoenix_vincular(text);
drop function if exists public.phoenix_clubes_sugeridos();
drop table if exists public.phoenix_duenos;
drop table if exists public.phoenix_vinculo_intentos;
drop table if exists public.phoenix_vinculos;
alter table public.myclub_clubes drop column if exists nombre_jugador;
