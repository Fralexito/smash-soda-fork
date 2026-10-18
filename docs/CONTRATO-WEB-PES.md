# Contrato WEB ↔ PES · «Cambios oficiales» (v1)

Estado: **v1 publicado el 17-oct-2026** (sesión BASE, migración 125). Copia idéntica en los dos repos:
`smash-soda-fork/docs/CONTRATO-WEB-PES.md` (rama `rediseño-phoenix-portal`) y `phoenixevolution/docs/CONTRATO-WEB-PES.md` (rama `borrador`).
Si cambias algo, cambia **las dos copias** y sube `version` si un PC viejo dejaría de entenderlo.

Pedido de Fralex: «que cualquier cambio que haga en mi web se refleje en el PES y que tenga todo el registro e historial de todos los
usuarios que usan mi sistema en sus PES», con **un solo formulario** donde cada cambio se aplica a ☐ Base normal (amistosos) ☐ Liga Máster ☐ Las dos.

Quién hace qué:
| Parte | Sesión | Migración |
|---|---|---|
| Formulario único, cambio oficial, base normal (amistosos) por el canal oficial de traspasos, registro de PCs | BASE (`ws-amistosos-*`) | **125** |
| Tipos `stats` e `info` (Player.bin) | web-pes-estadisticas | 126 |
| Destino `lm` (fichajes, dinero, valor y stats de Liga Máster) | web-pes-liga-master | 127 |

---

## 1. Qué es un «cambio»

Una fila de `public.pes_cambios`. **Un cambio = un solo registro en el historial**, aunque vaya a los dos destinos: si es `ambos`
tiene **dos resultados** (columna `estado_base` y columna `estado_lm`, más los resultados por PC en `pes_eventos` con `destino` = `base` o `lm`).

| Campo | Tipo | Qué es |
|---|---|---|
| `id` | uuid | Identidad del cambio. Nunca cambia. |
| `seq` | bigint | Orden global (1, 2, 3…). Los PCs leen «lo que venga después de mi último `seq`». |
| `version` | int | Versión de **este contrato** con la que se escribió (hoy `1`). |
| `tipo` | text | `fichaje` · `stats` · `info` · `dinero` · `valor` · … (lista viva en `public.pes_tipos_cambio`, ver §2). |
| `destino` | text | `base` · `lm` · `ambos`. |
| `datos` | jsonb | Lo que cambia; la forma depende del `tipo` (§2). |
| `autor` / `autor_nombre` | uuid / text | Quién lo hizo en la web (staff). |
| `creado` | timestamptz | Cuándo. |
| `firma` | text (64 hex) | `sha256("pes-cambio|v<version>|<id>|<tipo>|<destino>|<datos::text>")`. Integridad: si alguien toca `datos` a mano la firma deja de cuadrar. **No** es una firma criptográfica de clave privada: el canal Liga Máster puede añadir su firma Ed25519 propia (la de `mercado/v1`). |
| `estado` | text | `activo` · `anulado` (anulado = no se vuelve a mandar; lo ya aplicado se deshace con un cambio nuevo en sentido contrario). |
| `estado_base` | text | `no_aplica` · `pendiente` · `publicado` · `error`. |
| `base_lote` / `base_seq` | text / bigint | El lote del canal oficial de traspasos que lleva la parte base (§3). |
| `estado_lm` | text | `no_aplica` · `pendiente` · `tomado` · `publicado` · `error`. |
| `lm_ref` | text | Referencia que deja el motor de Liga Máster (su id o versión de cambios). |
| `nota` | text | Comentario libre del staff (≤ 300). |

Reglas:
- Se crea **solo** con la función `public.pes_cambio_crear(p_tipo, p_destino, p_datos, p_nota)` (staff `admin`; la web la llama con la sesión del usuario). Valida el tipo y que el destino esté permitido para ese tipo, calcula la firma y, si toca, publica la parte base en el mismo momento.
- `destino = base` → `estado_lm = no_aplica`. `destino = lm` → `estado_base = no_aplica`. `ambos` → los dos empiezan en `pendiente`.
- Nadie borra cambios: se anulan (`public.pes_cambio_anular(p_id, p_motivo)`).

## 2. Tipos y la forma de `datos`

Catálogo: `public.pes_tipos_cambio (tipo pk, etiqueta, destinos text[], motor text, activo bool)`. Cada sesión **añade o enciende su fila** en su migración
(`insert … on conflict (tipo) do update`). La 125 deja `fichaje` encendido (destinos `base`, `lm`, `ambos`) y reserva, **apagados**
(`activo = false`) hasta que su migración los encienda: `stats`, `info` (126; `base`, `lm`, `ambos`) y `dinero`, `valor` (127; solo `lm`).
Si el tipo **no** es `fichaje`, `pes_cambio_crear` solo guarda el cambio (y su parte base queda `pendiente` para el motor de ese tipo,
igual que la parte `lm`); cada motor marca su resultado con `public.pes_cambio_marcar(p_id, p_lado, p_estado, p_ref)` (§3b).

### `fichaje` (v1)
```json
{ "jugador_id": 162114, "jugador": "Lamine Yamal",
  "desde_id": 101,  "desde": "FC Barcelona",
  "hasta_id": 102,  "hasta": "Arsenal FC",
  "lm": { "jugador_id": 57, "club_id": 3 } }
```
- Nombres: obligatorios (1–48 **bytes** UTF-8: Link los corta por bytes al leer y, si no, la huella no cuadra; una tilde cuenta doble). Es lo que de verdad usa cada PC, porque **la base de cada PC puede ser otra** (otro parche, otro PC).
- `jugador_id` / `desde_id` / `hasta_id`: el id de PES si se sabe (la web lo saca de `lm_jugadores.pes_id`). **Si no se sabe**, se pone un
  **id sintético** `90000000 + (número sacado del sha256 del nombre en minúsculas, entre 0 y 9.999.998)`; ids ≥ 90.000.000 significan «no es un id real, aplícalo por nombre».
  Lo calcula la función SQL `public.pes_id_sintetico(nombre)`: la web no tiene que hacerlo.
  `desde_id` puede ser `0` (sin equipo / agente libre).
- `desde` es **obligatorio** salvo agente libre: un jugador está a la vez en su club y en su selección y hay que decir de cuál sale (bug real de Sync, ver `PhoenixSync/REGISTRO.md`, mover_cli).
- `lm` (opcional): ids de la web de Liga Máster (`lm_jugadores.id`, `lm_clubes.id`) para el motor 127. La parte base lo ignora.

### `stats`, `info`, `dinero`, `valor`
Los define cada sesión en este mismo archivo (añadir una sección `### <tipo> (v1)`), sin tocar lo de arriba.

### `stats` y `info` (v1) — sesión 126 (estadísticas e información, Player.bin)
```json
{ "jugador_id": 162114, "jugador": "Lamine Yamal",
  "valores": { "velocidad": 95, "finalizacion": 90 },
  "antes":   { "velocidad": 90 },
  "deshace": "<uuid de otro cambio>",
  "lm": { "jugador_id": 57 } }
```
- `jugador` obligatorio (1–48 bytes). `jugador_id` opcional (id de PES, entero 1–99.999.999; ≥ 90.000.000 = sintético → por nombre).
  Cada PC usa el id **solo si en su base ese id tiene ese nombre**; si no, busca el nombre (único). Nunca toca a un jugador distinto.
- `valores`: 1–46 campos; claves y rangos en `public.pes_fichas_campos` (copia en Link `link/Fichas.cpp` y en la web `core/fichasCampos.js`).
  `stats` = 20 de campo + 5 de portero (40–99). `info` = pie malo uso/precisión (1–4), forma (1–8), resistencia a lesiones (1–3),
  13 posiciones `pos_*` (0 = C, 1 = B, 2 = A), `altura` (150–210), `peso` (45–110), `edad` (15–45), `posicion_registrada` (0–12: PT DFC LI LD MCD MC MI MD MO EI ED SD DC).
  Valor `null` = «vuelve al valor de la base del parche» (lo usa el deshacer).
- `antes` (opcional): lo que la web tenía antes, solo para el historial. `deshace` (opcional): este cambio deshace a otro (que queda `anulado`).
- La web valida los datos al crear (trigger de la 126) y deja `estado_base = 'publicado'`, `base_ref = 'v1/cambios'` al instante:
  la parte base **no** viaja por `liga_traspasos`, cada Link la baja de **§3c** (`GET /v1/cambios?destino=base`).
- **Cómo lo aplica cada Link (≥ 0.3.18):** junta todos los `stats`/`info` activos en orden de `seq` (el último valor de cada jugador+campo gana; `null` lo quita)
  y genera `Phoenix-DB\common\etc\pesdb\Player.bin` = Player.bin de **su** base + esa lista (solo cambian los bits de esos campos; copia antes; todo o nada).
  Quitar o deshacer = el campo vuelve al de la base. Informa por §4 con `destino: "base"` y `motivo` = «campo antes→después, …» (o el código del salto).
- **Destino `lm`:** el motor 127 lee `estado_lm = 'pendiente'` como para cualquier tipo. En Liga Máster las stats no salen de Player.bin: se suben/bajan
  con las barras de crecimiento de la partida (PhoenixSync, guías 24–29). Un `null` en `lm` = devolver el valor que 127 guardó como «antes» al aplicar el cambio de `deshace`.

## 3. Cómo lo baja cada PC

### 3a. Destino `base` de un `fichaje` (amistosos) → canal OFICIAL de traspasos
Al crear el cambio, la web mete **un lote** en `public.liga_traspasos` con `oficial = true` y `cambio = <id del cambio>`. El lote tiene
el **formato de Link** (ver `docs/TRASPASOS-EN-VIVO.md` §3):
```json
{ "seq": 812, "id": "<24 hex>", "huella": "<sha256>", "creado": "2026-10-17T20:00:00Z", "autor": "Phoenix (oficial)",
  "parche": "", "base_huella": "",
  "anfitrion": "Phoenix (oficial)", "anfitrion_id": "00000000-0000-4000-8000-0000000f0e11",
  "oficial": true, "cambio_id": "<uuid del cambio>",
  "cambios": [ { "jugador_id": 162114, "jugador": "Lamine Yamal", "desde_id": 101, "desde": "FC Barcelona", "hasta_id": 102, "hasta": "Arsenal FC" } ] }
```
- `huella` = `sha256` de una línea por cambio `jugador_id|desde_id|hasta_id|jugador|desde|hasta\n` (la misma de Link, `huellaDeCambios`), `id` = primeros 24 hex de `sha256("oficial|<cambio_id>")`.
- `base_huella` **vacía a propósito**: así cada Link lo aplica **por nombres** sobre su propia base (vale en cualquier PC y parche). Nunca por ids.
- `anfitrion_id` es **fijo** (`00000000-0000-4000-8000-0000000f0e11`), no el del admin: así el PC del propio admin también lo aplica (no lo toma por «propio»).
- **Ruta:** la misma de siempre, `GET /functions/v1/traspasos/v1/traspasos?desde=<seq>&limite=<1-50>`, devuelve los lotes de quien sigues **más** todos los oficiales.
  Con `&solo_oficial=1` devuelve **solo** los oficiales (lo usa un Link con «Recibir de amigos» apagado). No hace falta código ni seguir a nadie:
  **todo PC vinculado sigue el canal oficial**. Un Link viejo que no conoce `oficial` lo aplica igual si ya recibe de amigos (es un lote normal).
- Link (≥ 0.3.17): interruptor «Recibir los cambios oficiales de Phoenix», **encendido por defecto**. Los oficiales no piden aprobación, no se
  pueden «dejar de seguir» y van a la Phoenix-DB (se quedan al cerrar PES), igual que un traspaso de amigo.

### 3b. Destino `lm` (Liga Máster) → motor de la sesión 127 (y cualquier otro motor)
La 125 **no** manda nada al PC por este lado: deja `estado_lm = 'pendiente'`. El motor LM:
1. Lee los pendientes: `select * from public.pes_cambios where estado = 'activo' and estado_lm = 'pendiente' order by seq` (service role) o la ruta genérica de §3c.
2. Los traduce a su formato `lm_*` firmado y los publica por su canal (`/functions/v1/mercado/v1/liga/cambios`).
3. Marca el resultado con `public.pes_cambio_marcar(p_id, 'lm', p_estado, p_ref)` (`tomado` · `publicado` · `error`), solo service role o admin.
   El motor de stats/info (126) hace lo mismo con `p_lado = 'base'` para sus tipos.

### 3c. Ruta genérica (para cualquier motor o un Link futuro)
`GET /functions/v1/traspasos/v1/cambios?desde=<seq>&destino=base|lm&limite=<1-100>` (token de PC `phx_…`, cualquier PC vinculado) →
`{ ok, cambios:[{seq,id,version,tipo,destino,datos,creado,firma,estado}], ultimo_seq }`. Solo cambios `activo` cuyo destino incluye el pedido.

## 4. Cómo informa cada PC (registro de usuarios)

`POST /functions/v1/traspasos/v1/pc/informe` (token `phx_…` + cabeceras `X-Phoenix-Version` / `X-Phoenix-Build`). Cuerpo (≤ 32 KB):
```json
{ "link_version": "0.3.17", "link_build": 317, "parche": "ConmeGOL 26", "base_huella": "a1b2c3d4e5f6",
  "juego": true, "pes_abierto": false, "oficial": true, "recibir": true, "publicar": false,
  "resultados": [ { "cambio_id": "<uuid>", "destino": "base", "resultado": "aplicado", "motivo": "", "jugadores": 1, "cuando": "2026-10-17T20:01:00Z" } ] }
```
- `resultado`: `aplicado` · `ya_estaba` · `parcial` · `saltado` · `error` · `deshecho` · `pendiente` · `por_aprobar` · `reemplazado`.
- Se manda: al arrancar, cada ~10 min (latido) y en cuanto cambia el resultado de un lote oficial. Máx. 50 resultados por informe.
- **Ningún dato personal ni ruta de disco**: versión, parche, huella corta de la base (12 hex) y resultados. El usuario ya es el dueño del token.
- La web (función SQL `public.pes_registrar_informe(p_dispositivo, p_usuario, p_informe)`, solo `service_role`):
  - `public.pes_pcs` — una fila por PC (`dispositivo` = `dispositivos_host.id`): usuario, versión, build, parche, huella de base, último contacto, primer contacto, contadores aplicados/saltados/errores.
  - `public.pes_eventos` — historial **solo-añadir**: una fila por cada `(PC, cambio, destino)` cuyo resultado **cambió** (repetir el mismo resultado no añade fila), más eventos `contacto` cuando cambia la versión, el parche o la base del PC.
  - Contesta `{ ok, guardados, ignorados }`. Resultados con `cambio_id` que no existe se ignoran (cuentan en `ignorados`).
- El motor LM (127) informa sus resultados por la **misma ruta** con `destino: "lm"` (su PC tiene el mismo token de dispositivo) o, desde el servidor, con `public.pes_registrar_informe` (solo service_role).

## 5. Quién ve qué (RLS)
| Tabla | admin / moderador | el dueño | resto |
|---|---|---|---|
| `pes_cambios`, `pes_tipos_cambio` | leer | — | — |
| `pes_pcs`, `pes_eventos` | leer todo | leer los suyos | nada |

Escribir: solo por las funciones (`pes_cambio_crear`, `pes_cambio_anular`, `pes_cambio_marcar`, `public.pes_registrar_informe` (solo service_role)).
Páginas web: **/cambios-oficiales/** (formulario único + historial) y **/usuarios-pes/** (PCs, versión, último contacto, historial por usuario y PC).

## 6. Enchufes del formulario (para 126 y 127)
`src/js/core/cambiosOficiales.js` exporta `registrarTipo({ tipo, etiqueta, destinos, pintar(contenedor, ctx), leer() → datos | lanza Error })`.
Cada sesión añade su archivo `src/js/features/cambios/<tipo>.js` que llama a `registrarTipo(...)` y lo importa en `src/js/pages/cambiosOficiales.js`.
Las casillas Base normal / Liga Máster / Las dos son comunes; `ctx.destino` dice cuál está marcada para pintar los campos extra (p. ej. club de LM).

## 7. Límites
- `liga_traspasos` sigue con su retención de 30 días: un PC que pase más de 30 días sin abrir Link no recibe los oficiales viejos por el canal
  de traspasos (sí los ve en `GET /v1/cambios`). Los oficiales **no** cuentan para el tope de 200 lotes por anfitrión.
- 1 fichaje por cambio (el formulario puede crear varios seguidos).
- Lo que el PC hace en el juego (recargar Phoenix-DB) es igual que en traspasos en vivo: ver `docs/TRASPASOS-EN-VIVO.md` §4.

## 8. Historial del contrato
- v1 (17-oct-2026, sesión BASE): primera versión.
