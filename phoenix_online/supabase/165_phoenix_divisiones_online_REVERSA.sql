-- Vuelta atrás de la migración 165 (borra la tabla de Divisiones del online y sus funciones).
drop function if exists public.phoenix_ranking_online(integer);
drop function if exists public.phoenix_srv_subir_divisiones(text, jsonb);
drop table if exists public.phoenix_divisiones;
