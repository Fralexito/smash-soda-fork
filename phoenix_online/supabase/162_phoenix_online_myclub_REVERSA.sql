-- Vuelta atrás de la migración 162 (solo si hiciera falta). Borra SOLO lo que creó la 162.
-- Antes de usarla: exportar los datos de estas tablas si tienen algo que conservar.
drop function if exists public.phoenix_pedir_pase();
drop table if exists public.phoenix_pases;
drop table if exists public.myclub_movimientos;
drop table if exists public.myclub_reglas;
drop table if exists public.myclub_clubes;
