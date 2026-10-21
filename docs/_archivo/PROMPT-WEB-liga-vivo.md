# PROMPT para el chat WEB · Marcador en vivo y validación de la liga

> Pégalo en el chat WEB (repo `Fralexito/phoenixevolution`). Origen: chat LINK (árbitro), 2026-10-11. La base ya está hecha y aplicada en Supabase. Está en la rama **`liga-automatica`** (commit `5b69fad`, ya subida). Diseño completo: `docs/LIGA-AUTOMATICA.md` de esa rama. Protocolo de Link: `SmashSoda/phoenix/web/PROTOCOLO.md` (repo `smash-soda-fork`, rama `rediseño-phoenix-portal`).

## Qué quiere Fralex
1. Ver **en tiempo real** en la web el marcador de los partidos que se juegan en salas de Phoenix Link.
2. Que el resultado de un partido oficial lo **valide el staff o un árbitro designado, nunca el host**. Los partidos pueden ser en host neutral.
3. Que los partidos **contra la máquina (CPU)** salgan en el historial, pero que no cuenten para el ranking.

## Lo que ya existe (no lo rehagas)
- **Rama `liga-automatica`**:
  - migraciones `102_liga_automatica.sql` y `102b_liga_automatica_datos.sql`, **ya aplicadas en producción**;
  - Edge Function `supabase/functions/liga` (v1 desplegada);
  - `docs/LIGA-AUTOMATICA.md`.
  - **Antes de nada, júntala con la rama de trabajo actual** (`borrador`/fase-beta), sin pisar nada. La próxima migración tuya es la **103**.
- **Tablas** (lectura pública por RLS; nadie escribe desde el navegador):
  - `liga_ediciones`, `liga_participantes` (`usuario` = cuenta de la web del DT).
  - `liga_partidos`:
    - `estado`: `pendiente | propuesto | confirmado | disputa | anulado`;
    - `origen`: `copafacil | link | staff`;
    - otras columnas: `arbitro`, `detalle` (jsonb con goles y minuto, penales, `doctor`, `reclamo`), `resuelto_por`, `resuelto_en`.
  - `liga_vivo`: una fila por sala mientras dura el partido.
    - Columnas: `partido_id` (null = amistoso), `local`, `visitante`, `goles_local`, `goles_visita`, `minuto`, `periodo`, `fase` (`previa | en_juego | descanso | pausado | final`), `goles` `[{m, l:"l"|"v", j?, pp?}]` y `actualizado`. `j` = goleador (nombre de la camiseta) y `pp:true` = en propia puerta; solo llegan con Sider 7.3.4+, si no, el gol va sin nombre. Muéstralo como «34′ MESSI» o «80′ ARAUJO (p.p.)». Lo mismo en `liga_partidos.detalle.juego.goles` y en `partida_fin` (`juego.goles[{m, lado, jugador?, propia?}]`).
    - La función la borra al terminar.
- **Realtime** está activado en `liga_vivo` y `liga_partidos`. Se suscribe con la anon key; no hace falta consultar cada X segundos.
- **Vista `liga_tabla`**: `edicion_id, nombre, club, pj, g, e, p, gf, gc, dg, pts`. Solo cuenta los resultados `confirmado`.
- **Funciones RPC** (security definer; llámalas con `supabase.rpc`):
  - `liga_resolver_resultado(p_partido, p_aceptar, p_goles_local?, p_goles_visita?)` → texto. Solo la usan el staff o el árbitro designado de ese partido (si no, error 42501).
    - `true` = confirmado; puede corregir el marcador.
    - `false` = disputa.
  - `liga_reclamar_resultado(p_partido, p_motivo)`: solo los DT de ese partido. Pasa a `disputa` y guarda `detalle.reclamo`.
  - `liga_designar_arbitro(p_partido, p_arbitro uuid)`: solo el staff.
- **Interruptor** `phoenix_config.liga_resultados` (en `integracion`): apaga la recepción de resultados desde Link.
- **Datos ya cargados**: Galaxy edición 1, 22 fechas, 132 partidos, 52 confirmados. La tabla coincide con la de la web (Victor 27, Beto 24, Degox 21…).

## Qué hay que hacer

### 1) Marcador en vivo (Inicio y la página «En vivo» si existe)
- Al cargar: `select * from liga_vivo`. Después, suscríbete a `postgres_changes` de `liga_vivo` (insert, update y delete).
- Tarjeta por partido, dentro del diseño actual de la web (no lo rehagas):
  - local – visitante, marcador grande y minuto;
  - fase (EN JUEGO / DESCANSO / PAUSA / FINAL);
  - los goles con su minuto;
  - un chip **OFICIAL** si `partido_id` no es null (con la fecha y la edición sacadas de `liga_partidos`), o **AMISTOSO** si es null.
- Sin filas: no muestres nada, o una línea discreta «No hay partidos en juego».
- Al llegar un `delete` (fin del partido): deja la tarjeta unos 30 s con «FINAL» y luego quítala.
- Link solo manda los amistosos si la sala no es privada. No hay que filtrar nada más.

### 2) Panel de validación del staff (zona de staff/admin que ya exista)
- Lista de partidos `propuesto` y `disputa` (los más nuevos primero). Cada uno muestra:
  - marcador, quién lo propuso (`origen`, sala) y la hora;
  - los goles con su minuto (`detalle`);
  - el **reclamo** si hay uno (`detalle.reclamo.motivo`);
  - el resumen del **Phoenix Doctor** (`detalle.doctor`: huella, parche, avisos). Es **solo informativo**: aún no hay parche oficial, así que muéstralo como dato, sin aprobar ni rechazar automáticamente nada.
- Botones:
  - **CONFIRMAR** → `liga_resolver_resultado(id, true)`;
  - **CORREGIR Y CONFIRMAR** (dos números) → `(id, true, gl, gv)`;
  - **EN DISPUTA** → `(id, false)`.
- **Designar árbitro**: buscador de perfiles → `liga_designar_arbitro(id, uuid)`. Disponible también en los `pendiente`, para designarlo antes de que se juegue.
- **Vista del árbitro designado** (no es staff): «Mis partidos para validar» = `liga_partidos where arbitro = auth.uid()`, con los mismos botones.
- Muestra el error de la RPC tal cual: el texto ya viene en español.

### 3) Botón «Reclamar» para los DT
- En el detalle de un partido `propuesto` o `confirmado`, si el usuario es uno de sus DT (`liga_participantes.usuario = auth.uid()` para `local` o `visitante`): botón **RECLAMAR** con un motivo de 300 caracteres como máximo → `liga_reclamar_resultado`.

### 4) Página de la liga
- Tabla de posiciones desde **`liga_tabla`**, en vez del cálculo local, para las ediciones que ya estén en la base (hoy Galaxy edición 1). El resto sigue igual.
- Resultados por fecha desde `liga_partidos`:
  - `propuesto` con la marca «pendiente de validar»;
  - `disputa` con la marca «en disputa»;
  - `anulado` tachado o sin mostrar.
- Suscríbete a `liga_partidos` para que una confirmación del staff se vea al momento.

### 5) Partidos contra la máquina (CPU) · evento `partida_fin` de Link
- Link manda ahora `datos.contra_ia: true` y `datos.modo: "contra_ia"`. El jugador de la máquina va como `{ "parsec_id": "0", "nombre": "Máquina", "es_ia": true }`.
- Guárdalos en el historial de partidos (si existe `partidos_link` del ranking 1v1) con una marca 🤖 «vs CPU».
- **No cuentan para el ranking 1v1** ni para ninguna estadística competitiva.
- `parsec_id "0"` nunca se resuelve contra `cuentas_parsec`.
- Si el ranking ya filtra por `modo = '1v1'`, basta con eso. Compruébalo y añade una prueba.

## Reglas
- No toques `main`. Trabaja sobre la rama de trabajo y pide permiso a Fralex antes de hacer commit o merge.
- Las migraciones nuevas van desde la **103**.
- Nada de escribir en estas tablas desde el navegador: todo por RPC (ya están los `revoke`).
- Mantén el diseño actual de la web; Fralex no quiere rediseños sin pedirlos.
- Al terminar: una línea en la bitácora de la web y aviso al chat LINK si cambia algún contrato (`/v1/liga/*`, `partida_fin`).
