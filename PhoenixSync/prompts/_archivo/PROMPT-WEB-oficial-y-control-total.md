# Prompt para el chat WEB — Option file oficial + control total del admin (fichajes y stats)

10 de octubre de 2026. De: chat SYNC (rama `mercado-fase0`). Estado: **PENDIENTE-WEB**.
Pedido de FRALEX: «un option file oficial en mi web; cuando alguien modifique algo, se modifica ese y se descarga automáticamente en todos» y «acceso total a modificar los fichajes y las estadísticas de los jugadores, ya sea en Liga Máster o en lo normal».

Lo de Phoenix Sync **ya está hecho y probado** (222/222 + 211/211 pruebas con una web simulada). Falta solo la web. Va sobre lo ya publicado en `/functions/v1/phoenix` (contrato v1.8.0, §26) y sobre el prompt anterior `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` (grupo de FRALEX y su amigo, `GET /v1/sync/grupos`). Mismo sobre de respuesta `{ ok, datos | error:{ codigo, mensaje, reintentar_en } }`. Mismo token `phx_`.

---

## 1. Option file oficial («foto oficial» del grupo)

Idea: la web guarda la foto oficial (option file + base). **Solo el admin del grupo la publica**, siempre desde su PC con Phoenix Sync. Las demás PCs la bajan solas: la primera vez siempre y después en modo `automatico`; en `autorizacion` solo se avisa. Los fichajes del día a día siguen viajando como operaciones cortas (`/v1/sync/operaciones`). La foto es el punto de partida común y sirve para «resincronizar».

**Almacenamiento:** un bucket privado de Supabase Storage, por ejemplo `sync-oficial`, con la ruta `<grupo_id>/<version>/<nombre>`. Nada público: se baja y se sube solo con URL firmadas de vida corta.

**Tabla:** ya existe `public.sync_option_versiones` (vacía). Úsala o ajústala para guardar, por cada versión: `grupo_id`, `version` (1, 2, 3… por grupo), `seq` (hasta qué operación del grupo incluye), `parche`, `huella_bd`, `base_datos`, `publicado_por`, `publicado_en` y `archivos jsonb` (`[{ nombre, sha256, tamano, ruta }]`). RLS activado; solo la función del servidor la toca.

**Rutas:**
1. `GET /v1/sync/oficial?grupo_id=…` (miembro del grupo) → `datos: { version, seq, parche, huella_bd, base_datos, publicado_por, publicado_en, archivos:[{ nombre, sha256, tamano, url }] }`. `url` = URL firmada de descarga de unos 10 minutos (https). Si no hay ninguna: `404 { ok:false, error:{ codigo:"SIN_OFICIAL" } }`.
2. `POST /v1/sync/oficial/subida` `{ grupo_id, nombre, sha256, tamano }` (**solo admin**, si no `403 SOLO_ADMIN`) → `datos: { url, ruta }`. `url` = URL firmada de **subida** (Sync hace `PUT` con el archivo en bruto, `Content-Type: application/octet-stream`). `nombre` ∈ {`EDIT00000000`, `PlayerAssignment.bin`, `Player.bin`}. Tope de tamaño: 64 MB (el option file mide unos 11 MB).
3. `POST /v1/sync/oficial` `{ grupo_id, parche, huella_bd, base_datos, seq, archivos:[{ nombre, sha256, tamano, ruta }] }` (**solo admin**) → comprueba que cada `ruta` existe en el bucket, que su tamaño cuadra y, si puedes, su sha256 (`crypto.subtle.digest`) → crea `version = última + 1` → `datos: { version }`. Sin `EDIT00000000` → `400 FALTA_ARCHIVO`.

Sync valida todo: https, sha de 64 caracteres, tamaño ≤ 64 MB y nombre permitido. Al bajar comprueba tamaño + sha256 y, si no cuadra, lo borra (`HUELLA_DISTINTA`). Si el parche es otro, lo rechaza (`INCOMPATIBLE`).

### 1.1 · La base de datos es PROVISIONAL (importante)
La base que usan hoy las PCs (Phoenix-DB) es **provisional**: una copia de la de olmosjr23 del ConmeGOL 26. Más adelante será la **oficial** del Phoenix Evolution Patch, con IDs propios. Para ese cambio:
- Cada foto y cada operación llevan `base_datos`: hoy `"provisional-olmosjr23"`. La web lo guarda y lo devuelve tal cual.
- Cuando FRALEX publique la foto con otra etiqueta (`sync-publicar-oficial --base phoenix-patch-26-v0.1`), las PCs entienden que es un **cambio de base**: exigen Player.bin y PlayerAssignment.bin en la foto y descartan lo pendiente de la base anterior. Una operación con otra `base_datos` se marca `incompatible`.
- En el panel, muestra la base actual del grupo (la de la última foto) y ponla en cada operación que crees. **No guardes IDs de jugador/equipo «para siempre»** sin su `base_datos`: con la base oficial cambiarán. El catálogo (`mercado_*`) debe poder tener una versión por base.

## 2. Control total del admin: fichajes y stats («lo normal»)

Las PCs ya aplican tres tipos de operación de `/v1/sync/operaciones`:
- `mover` `{ jugador_id, equipo_origen, equipo_destino, dorsal }`: ya existía.
- `quitar` `{ jugador_id, equipo_origen }`: ya existía.
- **NUEVO** `stats` `{ jugador_id, campo, valor }`: cambia una stat en `Player.bin` de Phoenix-DB. Hay respaldo antes, se entrega a Link y el juego recarga solo.

**Lo que pido:**
1. Aceptar `tipo:"stats"` en `POST /v1/sync/operaciones` **solo si quien publica es admin del grupo**. Si no: `403 SOLO_ADMIN`. Guardar `campo` y `valor` dentro de `datos` (ya es jsonb) y devolverlos tal cual en `GET /v1/sync/operaciones`.
2. Validar `campo` y `valor` con esta tabla (la misma de `PhoenixSync/core/FichasBase.cpp`; valor = el que se ve en el juego):
   - 40–99: `ataque, control, regate, regate_ajustado, pase_raso, pase_bombeado, finalizacion, cabezazo, balon_parado, efecto, velocidad, aceleracion, equilibrio, contacto_fisico, salto, resistencia, defensa, recuperacion, agresividad, potencia_tiro, pt_despejar, pt_cobertura, pt_reflejos, pt_actitud, pt_atajar`.
   - 1–4: `pie_malo_uso, pie_malo_precision` · 1–8: `forma` · 1–3: `resistencia_lesiones`.
   - 0–2 (0 = C, 1 = B, 2 = A): `pos_pt, pos_dfc, pos_li, pos_ld, pos_mcd, pos_mc, pos_mi, pos_md, pos_mo, pos_ei, pos_ed, pos_sd, pos_dc`.
3. **Panel «Control total» (solo FRALEX / admin):**
   - **Fichajes:** buscar jugador → elegir equipo destino → crea una operación `mover` en el grupo, con `autor` = admin y `autor_pc` = "web". Lo mismo para `quitar`.
   - **Stats:** buscar jugador → ver la tabla de arriba con controles (deslizador 40–99, A/B/C para posiciones) → cada cambio crea una operación `stats`. Pon un tope de 50 operaciones por envío.
   - Para buscar jugadores y equipos usa el catálogo que ya existe (`mercado_*`, `/catalogo`), con los IDs del parche.
   - **Historial:** quién hizo qué y el estado por PC (de `sync_confirmaciones`).
   - **Foto oficial:** versión actual, quién la publicó y cuándo. Botón «Pedir a mi PC que publique» = solo texto de ayuda: «en tu PC: `PhoenixSync sync-publicar-oficial`».
4. Las operaciones que crea la web llevan `parche`, `huella_bd` y `base_datos` **de la última foto oficial**, y `formato: 1`. Sin foto oficial, el panel no deja crear operaciones y avisa: «Publica antes la foto oficial desde tu PC». Sync rechaza las operaciones sin `huella_bd`, porque protege de mezclar parches.

## 3. Liga Máster (carrera guardada)

- **Fichajes en la carrera:** ya hay un camino firmado y probado. `GET /mercado/v1/liga/cambios?desde=N` (Ed25519) → `PhoenixSync sincronizar … --ml <ML> <ML nuevo>` los aplica a la carrera (vender a la IA, fichar para el usuario, IA ↔ IA) en un archivo nuevo. Pido que el panel «Control total» tenga una pestaña **Liga Máster** que cree esos cambios en `lm_*` / `lm_movimientos` (lo que ya usa `/liga/cambios`), solo para el admin.
- **Stats dentro de una carrera empezada:** no se ponen a un valor fijo. Funciona con las «barras de crecimiento»: barra al máximo = +1 al avanzar el calendario; barra a 0 = −1 (guías 24–29). Sync todavía no lo trae (falta programarlo y probarlo en el juego). Deja preparado en el panel, desactivado: «Subir / bajar 1 punto en la Liga Máster» con `{ jugador_id, campo, direccion: "sube"|"baja" }`, y una nota: «Próximamente».

## 4. Al terminar

Anota en `contrato-v1.md` (§27, v1.9.0) y en tu `REGISTRO.md`. Pásale a FRALEX el texto final para el chat SYNC: rutas publicadas, nombre del bucket y cualquier diferencia con este pedido. No pegues tokens ni secretos.
