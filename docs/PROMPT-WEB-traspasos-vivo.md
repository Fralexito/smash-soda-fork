# PROMPT para el chat WEB · Traspasos en vivo (hub de lotes)

> Pégalo en el chat WEB. Origen: chat LINK, 2026-10-12. Diseño completo: `docs/TRASPASOS-EN-VIVO.md` en `smash-soda-fork` (rama `rediseño-phoenix-portal`). **Link ya está hecho (0.2.49, apagado por defecto); lo que falta es solo esto de la web.**

## Qué quiere Fralex
Que un traspaso que él haga en **Editar** de PES (y guarde) se refleje solo en el PES de sus amigos. Cada Link publica los traspasos como un **lote** (JSON corto) y los demás Link lo recogen y lo aplican sobre su propia base. La web es el **centro**: guarda los lotes en orden y se los da a quien los pida, aunque ese amigo tuviera Link apagado.

## Lo que hay que crear (dentro de la función `phoenix`, mismo token de Link: `Authorization: Bearer <token>`)

### Tabla `traspasos_lotes`
| columna | tipo | nota |
|---|---|---|
| `mundo` | text | grupo que comparte traspasos; por ahora siempre `"phoenix"` (queda abierto a una por clan/liga) |
| `seq` | bigint | consecutivo **por mundo** (1, 2, 3…), lo asigna la web al insertar |
| `id` | text | 24 hex; el que manda Link. **Único por (mundo, id)** → idempotencia |
| `autor_usuario` | uuid | quién publicó (sale del token, no del cuerpo) |
| `autor` | text | nombre que muestra Link (≤ 60) |
| `anfitrion` | text | id estable del Link que lo publicó (≤ 64) |
| `creado` | timestamptz | el del lote |
| `recibido_en` | timestamptz | `now()` |
| `parche`, `base_huella` | text | informativos |
| `huella` | text | sha256 hex de los cambios (ver abajo) |
| `cambios` | jsonb | lista (≤ 200 por lote) |
| `retirado` | bool | `false`; solo el staff lo pone a `true` (el lote deja de entregarse) |

### `POST /v1/traspasos/lotes`
Cuerpo: `{"mundo":"phoenix","lote":{ ...lote... }}`. Respuestas:
- `201 {"seq":N}` lote nuevo guardado.
- `409 {"seq":N,"ya":true}` ese `id` ya estaba (idempotente: **no** se duplica y devuelve su `seq`). Link lo trata como éxito.
- `400` formato malo · `401/403` sin permiso · `413` > 64 KB · `429` demasiados (máx. **30 lotes por hora por usuario**).
**Quién puede publicar:** solo usuarios con permiso de «anfitrión de mundo» (que lo decida Fralex/staff; propuesta: rol de staff + lista de usuarios autorizados). Todo el que esté en el mundo puede **leer**.

### `GET /v1/traspasos/lotes?mundo=phoenix&desde=<seq>&limite=100`
Devuelve `{"lotes":[ {…lote…, "seq":N}, … ], "hay_mas":false}` con `seq > desde`, en orden ascendente, sin los `retirado`. Link lo consulta cada ~10–15 s (sondeo; si la web quiere dar Realtime a futuro, es opcional y no cambia nada).

## Forma de un lote (formato 1)
```json
{
  "formato": 1,
  "id": "9f8e7d6c5b4a39281716f5e4",
  "huella": "<sha256 hex>",
  "creado": "2026-10-12T17:30:00Z",
  "autor": "Fralex",
  "anfitrion": "3fa9c1…",
  "parche": "ConmeGOL 26 (base olmosjr23)",
  "base_huella": "358c51b8ef11",
  "cambios": [
    { "jugador_id": 162114, "jugador": "Lamine Yamal",
      "desde_id": 108, "desde": "FC Barcelona",
      "hasta_id": 101, "hasta": "Arsenal FC" }
  ]
}
```
Validación mínima en la web (Link ya valida lo mismo al recibir y **rechaza** el lote si algo no cuadra): `formato == 1`; `id` = 24 hex; `cambios` es un array de 1 a 200 con `jugador_id`, `desde_id`, `hasta_id` enteros > 0 y `desde_id != hasta_id`; textos recortados a 80.
**Huella** (opcional que la web la recalcule; Link sí): sha256 en hex sobre el texto UTF-8 formado por **una línea por cambio**, ordenados por `jugador_id` y luego `hasta_id`: `jugador_id|desde_id|hasta_id|jugador|desde|hasta\n`. **La web NO debe reescribir ni reordenar los campos**: se guardan y se devuelven tal cual.

## Reglas
- **Idempotente por `id`.** Reintentos de Link (red caída) no duplican.
- **`seq` solo crece**; nunca se reutiliza ni se reordena. Link guarda hasta qué `seq` vio.
- **No se guardan bases** ni nada más que esa lista corta. Los lotes `retirado` no se entregan, pero Link ya aplicó los que recibió (el amigo los deshace con su botón).
- Nada de secretos en el lote. El `anfitrion` no es un secreto (solo evita que un Link se aplique sus propios lotes).
- Sin Realtime ni push por ahora; el sondeo basta (un traspaso a mano cada varios minutos).

## Cuando esté
Avisa a Fralex: en Link solo hay que enchufar la petición autenticada al `TransporteWeb` (ya escrito y probado con un hub falso que cumple este contrato; `pruebas-link/traspasos_test.cpp`, sección «transporte por la web»).
