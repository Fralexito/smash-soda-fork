# PROMPT para el chat WEB · Sistema de jugadores (rangos, prestigio, fair play, logros, temporadas)

> **NO ENVIAR TODAVÍA** (Fralex: «aún no lo apliques»). Va **después** de `docs/PROMPT-WEB-ranking-1v1.md`: usa sus tablas (`partidos_link`, `partidos_link_jugadores`, `partidos_link_goles`, `ranking_1v1`) y su función `recalcular_ranking_1v1()`. Diseño completo: `docs/SISTEMA-JUGADORES.md` (repo `smash-soda-fork`, rama `rediseño-phoenix-portal`).

## Qué llega de Phoenix Link
El mismo `partida_fin` v2. Cuando Fralex encienda el sistema, cada `a/b.jugadores[]` traerá además `ficha` (opcional; si el evento no cabe en 4 KB, Link la quita):
```json
"ficha": { "mando": 1, "cambios_mando": 0, "seg_mando": 640, "seg_fuera": 0, "caidas": 0,
           "pulsaciones": 2890, "pulsaciones_min": 271, "start": 1,
           "seg_inactivo": 35, "inactivo_max_seg": 9,
           "ping_medio": 48, "ping_max": 130, "seg_lag": 0 }
```
`ping_*` y `seg_lag` no vienen para el anfitrión. Sin `ficha`, todo funciona igual: solo dejan de sumar los cálculos que dependen de ella.

## 1) Migración
```sql
-- Ficha por jugador y partido (columnas tipadas para poder consultar)
alter table public.partidos_link_jugadores
  add column ficha jsonb,
  add column seg_mando int, add column caidas int, add column seg_fuera int,
  add column pulsaciones int, add column pulsaciones_min int, add column starts int,
  add column seg_inactivo int, add column inactivo_max_seg int,
  add column ping_medio int, add column ping_max int, add column seg_lag int,
  add column equipo_juego int, add column nombre_equipo text;   -- de datos.juego.equipo_a/b

create table public.temporadas (
  id serial primary key, nombre text not null,
  inicio timestamptz not null, fin timestamptz,          -- fin null = temporada en curso (solo una)
  creado_en timestamptz not null default now()
);
create unique index temporadas_una_abierta on public.temporadas ((fin is null)) where fin is null;
alter table public.partidos_link add column temporada int references public.temporadas(id);

-- El ranking pasa a ser por temporada (la clave pasa a ser temporada + usuario)
alter table public.ranking_1v1 add column temporada int references public.temporadas(id);
alter table public.ranking_1v1 drop constraint ranking_1v1_pkey, add primary key (temporada, usuario_id);
-- (antes: crear la «Temporada 1» y rellenar temporada en las filas que ya haya)
create table public.ranking_1v1_historico (                -- foto al cerrar cada temporada
  temporada int not null references public.temporadas(id), usuario_id uuid not null,
  posicion int not null, elo int not null, rango text not null, pj int not null, pg int not null,
  pe int not null, pp int not null, gf int not null, gc int not null, pts int not null,
  primary key (temporada, usuario_id)
);

-- Progreso que nunca se reinicia
create table public.jugador_progreso (
  usuario_id uuid primary key,
  xp int not null default 0, nivel int not null default 1, estrellas int not null default 0,
  fair_play int not null default 100,
  partidos_limpios_seguidos int not null default 0,
  actualizado timestamptz not null default now()
);

create table public.logros (
  clave text primary key, nombre text not null, descripcion text not null, icono text, orden int not null default 0
);
create table public.jugador_logros (
  usuario_id uuid not null, logro text not null references public.logros(clave),
  partido bigint references public.partidos_link(id), conseguido_en timestamptz not null default now(),
  primary key (usuario_id, logro)
);
```
RLS: solo `service_role` escribe. El público lee **solo por vistas**, sin `parsec_id`, `datos` ni `ficha` en bruto.
- `jugador_publico`: nombre, avatar, rango, Elo, nivel, estrellas, fair play, logros y medias.
- `cara_a_cara(a uuid, b uuid)`: función o vista.
- `partidos_1v1_publico`.

Las medias de ping y pulsaciones se muestran **solo en la ficha del propio jugador y al staff**, no en público.

## 2) Al registrar un partido (`registrar_partido_link`, ampliar)
- Copiar `ficha` a las columnas tipadas y el equipo de cada lado (`juego.equipo_a`/`nombre_equipo_a` para A, y lo mismo para B).
- Asignar `temporada` = la abierta según `fin`.
- Reglas de validez nuevas (además de las de `ranking-1v1`):
  - jugador con `jugador_progreso.fair_play` < 40 → `revisar` (`fair_play_bajo`);
  - `inactivo_max_seg` ≥ 120 de un jugador → `revisar` (`inactivo`).
- Después de `recalcular_ranking_1v1()`, llamar a `recalcular_progreso(usuario)` para cada jugador del partido.

## 3) `recalcular_ranking_1v1()` por temporada
Igual que el diseño base, pero solo con los partidos `valido` de la temporada abierta. Al cerrar una temporada, `cerrar_temporada(nombre_nueva)` (solo staff):
- copia `ranking_1v1` en `ranking_1v1_historico`;
- da estrellas: +1 a quien jugó 20 o más partidos, +1 extra si terminó en Diamante o Leyenda;
- cierra la temporada (`fin = now()`) y abre la nueva;
- Elo inicial de la nueva para quien ya jugaba: 1000 + (Elo final − 1000) / 2. Guárdalo como semilla en el recálculo.

## 4) Rango (función `rango_de(elo int, pj int)`)
`pj < 5` → «Sin clasificar». Si no: < 900 Bronce · 900–1049 Plata · 1050–1199 Oro · 1200–1349 Platino · 1350–1499 Diamante · ≥ 1500 Leyenda. «Élite» es una insignia aparte: top 10 de la temporada con 20 o más PJ.

## 5) `recalcular_progreso(usuario)` desde cero, en orden de `fin`, solo partidos `valido`
- **XP:** +10 por jugar, +5 por ganar, +2 por empatar. +3 por cada victoria seguida desde la 3.ª, con un máximo de +15 por partido. +3 si el partido fue limpio. Nivel = floor(sqrt(xp/25)) + 1.
- **Fair play** (sobre **todos** los partidos 1v1 del jugador, también `revisar`; no los `excluido` por `cancelado`). Empieza en 100 y se limita a 0–100:
  - abandono (`abandono_juego`, o él con `presente = false`): −15;
  - −3 por caída (máximo −9 por partido);
  - `inactivo_max_seg` ≥ 60: −5;
  - `starts` ≥ 4: −2;
  - partido limpio: +2.
  - Limpio = `cierre = 'completo'`, sin caídas y con `inactivo_max_seg` < 60.
- **Logros** (insertar si no existen, con el partido que lo dio):

| clave | regla |
|---|---|
| `primer_partido` | 1 PJ válido |
| `primera_victoria` | 1 PG |
| `triplete` | 3 goles o más propios en un 1v1 |
| `goleada` | ganar por 4 o más |
| `porteria_cero` | ganar sin recibir goles |
| `remontada` | según `partidos_link_goles` en orden, ir perdiendo por 2 o más y ganar |
| `descuento` | marcar un gol en el minuto 90 o después |
| `racha_5`, `racha_10` | victorias seguidas |
| `invicto_10` | 10 partidos seguidos sin perder |
| `matagigantes` | ganar a un rival con 200 o más de Elo por encima en ese momento (guarda el Elo previo de cada partido en el recálculo del ranking) |
| `rey_penales` | 5 tandas ganadas |
| `fiel_club` | 50 PJ con el mismo `equipo_juego` |
| `trotamundos` | 10 `equipo_juego` distintos |
| `centenario` | 100 PJ |
| `juego_limpio` | 20 partidos limpios seguidos |
| `incansable` | media de `pulsaciones_min` ≥ 300 en sus últimos 10 PJ con ficha |

Carga el catálogo `logros` con esos nombres en español, una descripción corta y un icono.

## 6) Páginas
- **Ranking:** selector de temporada (en curso e históricas), rango con su color, Elo, PJ, PG, PE, PP, GF, GC, DG, Pts, racha y últimos 5.
- **Ficha del jugador:**
  - pública: rango, Elo, nivel, barra de XP, estrellas, fair play, logros (conseguidos en color, los demás en gris), últimos 10 partidos, equipo favorito, % de victorias y goles por partido;
  - privada (el propio jugador y el staff): ping medio y pulsaciones por minuto medias.
- **Cara a cara:** elegir dos jugadores → PJ, victorias de cada uno, empates, goles y últimos 5 enfrentamientos.
- **Staff:**
  - partidos por revisar, con motivos y botones Aprobar o Excluir;
  - lista de jugadores con fair play < 60;
  - cerrar temporada;
  - reasignar partidos de un `parsec_id` a una cuenta que se vinculó después (re-resolver `usuario_id` y recalcular).
- **Privacidad:** una línea en la página de privacidad: «En las salas de Phoenix Link se registra, resumida por partido, tu actividad con el mando (pulsaciones, inactividad) y tu conexión (ping, caídas) para el ranking y el fair play».

## Criterios de aceptación
1. Un partido válido con `ficha` llena las columnas tipadas, suma XP (jugar + ganar) a los dos y recalcula nivel y rango.
2. Sin `ficha`, el partido cuenta igual para el ranking, la XP y los logros que no dependen de ella.
3. Un abandono baja 15 de fair play al que se fue. Con fair play < 40, sus siguientes partidos entran como `revisar`.
4. Cerrar la temporada guarda la foto, da las estrellas, hace el reinicio suave del Elo y empieza un ranking vacío de la nueva temporada. La XP, el nivel y los logros no cambian.
5. Los logros no se duplican, y recalcular desde cero da el mismo resultado (idempotente).
6. Ninguna vista pública expone `parsec_id`, `datos`, `ficha`, ping ni pulsaciones de otros.
7. Reasignar un `parsec_id` a una cuenta recalcula ranking y progreso, y los partidos de esa persona pasan a contar si cumplen las reglas.

Al terminar: una línea en `REGISTRO.md` del repo web y avisa al chat LINK para que encienda `kFichaJugadorActiva`.
