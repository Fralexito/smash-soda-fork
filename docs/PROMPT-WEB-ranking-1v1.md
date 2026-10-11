# PROMPT para el chat WEB · Ranking de jugadores 1 contra 1

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`, rama `borrador`). Origen: chat LINK, 2026-10-11. Diseño completo: `docs/RANKING-1V1.md` en `smash-soda-fork`, rama `rediseño-phoenix-portal`.

## Qué quiere Fralex
Un ranking de jugadores en la web, empezando por los partidos 1 contra 1 que se juegan en salas de Phoenix Link.

## Qué ya manda Phoenix Link (no hay endpoint nuevo)
Al terminar cada partido, Link manda el evento `partida_fin` por el `POST /v1/eventos` de siempre (contrato §6). Ahora sus `datos` traen `"v": 2` y el resultado completo. Un Link viejo solo manda `{"marcador":"2-1"}`: eso se sigue aceptando y se ignora para el ranking.

```json
{
  "v": 2, "marcador": "2-1", "partido_id": "1760140000000",
  "inicio_ms": 1760140000000, "fin_ms": 1760140720000, "duracion_seg": 690,
  "cierre": "completo",            // completo | manual | cancelado
  "modo": "1v1",                   // 1v1 | equipos
  "ganador": "a",                  // a | b | empate | ""
  "a": { "nombre": "Mirko", "goles": 2, "local": true,
         "jugadores": [ { "parsec_id": "111", "nombre": "Mirko", "presente": true, "es_host": true } ] },
  "b": { "nombre": "Kaiser", "goles": 1, "local": false,
         "jugadores": [ { "parsec_id": "222", "nombre": "Kaiser", "presente": true, "es_host": false } ] },
  "correcciones": 0, "pausas": 1, "host_parsec": "111", "abandono_juego": false,
  "juego": { "fase": "final", "minuto": 90, "completo": true,
             "equipo_a": 101, "equipo_b": 102, "nombre_equipo_a": "Real Madrid", "nombre_equipo_b": "Barcelona",
             "goles_a": 2, "goles_b": 1, "coincide": true, "penales_a": 4, "penales_b": 3,
             "goles": [ { "m": 12, "lado": "a" }, { "m": 34, "lado": "b" } ] }
}
```
- `juego` solo viene si había datos del juego al cerrar. `penales_*` solo si hubo tanda. Si `datos` pasa de ~3,8 KB, Link quita `juego.goles`.
- Los partidos con varios por lado (`modo: "equipos"`) también llegan. **Se guardan, pero sin atribuir goles a personas.**
- `parsec_id` es texto de dígitos. El `usuario_id` lo resuelve la web con `cuentas_parsec`, igual que en `/v1/perfiles`. Lo que Link crea saber no vale.

## Qué hay que hacer

### 1) Migración (adapta los nombres a los reales: `cuentas_parsec`, `perfiles`, `salas`, etc.)
```sql
create table public.partidos_link (
  id              bigserial primary key,
  sala_id         uuid not null,
  partido_id      text not null,               -- datos.partido_id
  evento_clave    text not null unique,        -- clave del evento
  host_usuario_id uuid,                        -- dueño del token que lo envió
  inicio          timestamptz,
  fin             timestamptz not null,
  duracion_seg    int not null,
  cierre          text not null check (cierre in ('completo','manual','cancelado')),
  modo            text not null check (modo in ('1v1','equipos')),
  goles_a         int not null, goles_b int not null,
  penales_a       int, penales_b int,
  ganador         text not null default '',    -- a | b | empate | ''
  correcciones    int not null default 0,
  pausas          int not null default 0,
  abandono_juego  boolean not null default false,
  datos           jsonb not null,              -- tal cual llegó
  estado_ranking  text not null default 'excluido' check (estado_ranking in ('valido','revisar','excluido')),
  motivos         text[] not null default '{}',
  revisado_por    uuid, revisado_en timestamptz,
  creado_en       timestamptz not null default now(),
  unique (sala_id, partido_id)
);
create table public.partidos_link_jugadores (
  partido    bigint not null references public.partidos_link(id) on delete cascade,
  lado       text not null check (lado in ('a','b')),
  parsec_id  text not null,
  usuario_id uuid,                              -- foto fija al recibirlo
  nombre     text not null default '',
  presente   boolean not null default true,
  es_host    boolean not null default false,
  goles      int,                               -- solo 1v1; null en equipos
  primary key (partido, lado, parsec_id)
);
create table public.partidos_link_goles (
  partido    bigint not null references public.partidos_link(id) on delete cascade,
  orden      int not null,
  minuto     int not null,
  lado       text not null check (lado in ('a','b')),
  usuario_id uuid,                              -- solo 1v1
  primary key (partido, orden)
);
create table public.ranking_1v1 (
  usuario_id uuid primary key,
  elo int not null default 1000,
  pj int not null default 0, pg int not null default 0, pe int not null default 0, pp int not null default 0,
  gf int not null default 0, gc int not null default 0, pts int not null default 0,
  racha text not null default '',               -- 'G3', 'E1', 'P2'
  ultimos text not null default '',             -- últimos 5, el más reciente a la izquierda: 'GGEPG'
  ultimo_partido timestamptz,
  actualizado timestamptz not null default now()
);
```
RLS: activado en las cuatro tablas. Solo `service_role` escribe (la función edge y las funciones `security definer`). El público lee **solo por vistas**, sin `datos` ni `parsec_id`: `ranking_1v1_publico` y `partidos_1v1_publico` (fecha, jugadores con nombre y avatar, marcador, penales y estado). El staff lee las tablas.

### 2) Función `registrar_partido_link(p_sala uuid, p_clave text, p_ocurrido timestamptz, p_host uuid, p_datos jsonb)` (security definer)
- Sin `p_datos->>'v' = '2'` → no hace nada.
- Inserta en `partidos_link` con `on conflict (sala_id, partido_id) do nothing`. Si ya existía, no hace nada más (idempotente).
- Por cada jugador de `a` y `b`: resuelve `usuario_id` con `cuentas_parsec.parsec_id` e inserta en `partidos_link_jugadores`. En 1v1, `goles` = goles de su lado.
- `juego.goles[]` → `partidos_link_goles` (en 1v1 con el `usuario_id` del lado).
- Calcula `estado_ranking` y `motivos`:
  - **excluido**: `modo` ≠ `1v1` (`equipos`), `cierre = 'cancelado'` (`cancelado`), un jugador sin `usuario_id` (`sin_cuenta`), el mismo `usuario_id` en los dos lados (`misma_persona`), `duracion_seg < 180` (`muy_corto`).
  - **revisar** (si no está excluido): `cierre = 'manual'` (`cierre_manual`), `correcciones > 0` (`marcador_tocado`), `juego.coincide = false` (`no_coincide_juego`), `abandono_juego` (`abandono`), algún `presente = false` (`jugador_ausente`), ya hay 3 o más partidos `valido` de la misma pareja en las 24 h anteriores (`farmeo`).
  - **valido**: el resto.
- Si es 1v1 y no `excluido` → `perform recalcular_ranking_1v1()`.

### 3) Función `recalcular_ranking_1v1()` (security definer): desde cero, en orden de `fin`, solo `estado_ranking = 'valido'`
- Elo inicial 1000. Esperado E = 1 / (1 + 10^((R_rival − R)/400)). S = 1 / 0,5 / 0. K = 40 si el jugador lleva menos de 10 partidos válidos; si no, 24.
- Multiplicador por diferencia de goles N: 1 si N ≤ 1, 1,5 si N = 2, (11+N)/8 si N ≥ 3. Delta = round(K · G · (S − E)).
- **Los penales cuentan como empate** (Elo, PE y Pts). Se muestran aparte.
- Pts 3/1/0. GF/GC son los goles del lado. Racha = letra del último resultado + cuántos seguidos. `ultimos` = 5 últimos.
- Borra y reescribe `ranking_1v1` en una transacción. Usa un bloqueo consultivo para que dos recálculos no se pisen.
- Vista pública `ranking_1v1_publico`: une con `perfiles` (nombre, avatar, país) y añade `dg = gf - gc` y `provisional = pj < 5`. Ordena por elo desc, luego pj desc.

### 4) Función edge `phoenix` · `POST /v1/eventos`
- Por cada evento aceptado con `tipo = 'partida_fin'` y `datos.v = 2`, llama a `registrar_partido_link(sala_id, clave, ocurrido, <usuario del token>, datos)`.
- **Si falla el registro, el evento se acepta igual**: se loguea y no va a `rechazados`. Si no, Link lo descartaría o lo reintentaría para siempre.
- Duplicados (misma `clave`): no se vuelve a registrar.
- No cambia nada publicado. Sube el contrato a **1.11.0** (cambio compatible) y documenta los `datos` v2 de `partida_fin` en §6.

### 5) Página de la web
- «Ranking 1v1»: posición, avatar, nombre, Elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5. Los provisionales van marcados o en una pestaña aparte.
- En el perfil de un jugador: sus últimos partidos 1v1 (rival, marcador, penales, fecha, válido o por revisar).
- Staff: lista de partidos «por revisar» con sus `motivos`, y botones Aprobar → `valido` o Excluir → `excluido`, que guardan `revisado_por` y `revisado_en` y recalculan.

## Criterios de aceptación
1. Un `partida_fin` v2 1v1 completo, con los dos jugadores vinculados y presentes, deja una fila `valido` en `partidos_link`, 2 jugadores y N goles con `usuario_id`, y actualiza `ranking_1v1` (cada uno con PJ 1).
2. Reenviar el mismo evento (misma `clave`) u otro con el mismo `(sala_id, partido_id)` no duplica nada ni cambia el Elo.
3. Un `partida_fin` sin `v` (Link viejo) se acepta como evento y no crea partido.
4. `modo: "equipos"` se guarda con sus jugadores y goles por lado, pero sin `usuario_id` en los goles ni `goles` por jugador, y no toca el ranking.
5. `cierre: "manual"` o `correcciones > 0` → `revisar`, y no cuenta hasta que el staff lo apruebe. Al aprobarlo, el ranking se recalcula.
6. El mismo usuario en los dos lados, un jugador sin cuenta o `duracion_seg < 180` → `excluido` con su motivo.
7. El 4.º partido válido de la misma pareja en 24 h queda en `revisar` con `farmeo`.
8. Un empate a goles con penales 4-3 suma un empate a los dos (PE +1, Pts +1).
9. Un error dentro de `registrar_partido_link` no hace fallar `POST /v1/eventos` ni manda el evento a `rechazados`.
10. La vista pública no expone `datos` en bruto ni `parsec_id`.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK con la versión del contrato (1.11.0) para quitar el `PENDIENTE-WEB` de `COORDINACION.md`.
