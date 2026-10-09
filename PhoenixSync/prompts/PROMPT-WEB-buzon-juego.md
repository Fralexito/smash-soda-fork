# PROMPT PARA EL CHAT WEB — «Buzón del juego» (avisos web → PES 2021 en vivo)

Hola. Soy el chat MERCADO (cuenta B, rama `mercado-fase0` de `Fralexito/smash-soda-fork`). Te pido un cambio en la web y en Supabase. Sigue tu protocolo de siempre:
- `git pull`;
- reservar la migración en `REGISTRO.md` (la siguiente libre; hoy creo que es la **096**);
- una línea por cambio en `REGISTRO.md`;
- documento en Drive;
- nada a `master` sin permiso de Fralex.

## Contexto (ya probado en el juego de Fralex el 2026-10-09)
- Un módulo de Sider, `phoenix.lua` (v0.2), lee el archivo `<juego>\SiderAddons\content\phoenix\avisos.txt` cada 2 segundos y lo muestra en el overlay de Sider (barra espaciadora).
- Con el juego abierto, reemplazamos el archivo y el texto nuevo apareció en **1 a 2 segundos**. Las tildes, la ñ, ⚡ y → se ven bien.
- Falta el tramo **web → PC**: **Phoenix Link** (el programa de la PC, que ya habla con `/functions/v1/phoenix` usando su token `phx_…`) va a consultar la web y escribir ese archivo.
- Tú construyes la parte web. Al chat LINK le paso otro prompt con el mismo contrato.

## 1. Base de datos (migración nueva)
Tabla `juego_avisos`:

| columna | tipo | regla |
|---|---|---|
| `id` | bigint identity PK | |
| `usuario_id` | uuid → `auth.users` / perfiles | **destinatario** (dueño de la PC) |
| `dispositivo_id` | uuid null | null = todas las PCs vinculadas de ese usuario; si no, solo esa PC (`phoenix` dispositivos) |
| `texto` | text not null | 1–600 caracteres; máx. 8 líneas; sin caracteres de control salvo `\n` |
| `tipo` | text not null default `'aviso'` | `aviso` \| `partido` \| `liga` \| `sistema` (por ahora solo cambia el icono en la web) |
| `creado_por` | uuid | quien lo escribió (el mismo usuario o staff) |
| `creado_en` | timestamptz default now() | |
| `expira_en` | timestamptz default now() + interval '24 hours' | pasado esto, no se entrega |
| `entregado_en` | timestamptz null | lo marca Phoenix Link cuando ya lo escribió en el juego |

Además:
- **Avisos globales del staff:** una tabla o un flag (`global boolean`) para que el staff mande un aviso a **todos** los dispositivos (p. ej. «Mantenimiento a las 22:00»). Decide tú la forma más limpia.
- **RLS:**
  - el usuario lee y crea solo sus avisos (`usuario_id = auth.uid()`, `creado_por = auth.uid()`);
  - el staff puede crear para cualquiera y globales;
  - nadie edita el texto después de creado;
  - el usuario puede borrar los suyos.
- **Índices:** por `(usuario_id, creado_en desc)` y parcial donde `entregado_en is null`.
- **Límite anti-spam:** máx. 20 avisos por usuario por hora (RPC o trigger).
- **Configuración en `phoenix_config`:**
  - `sondeo_buzon_seg`: por defecto 15, mínimo 5;
  - interruptor `buzon_juego`: por defecto true. Si es false, la API responde vacío y Link deja de preguntar hasta el próximo `config`.

## 2. API para Phoenix Link (función `phoenix`, contrato `docs/contrato-v1.md`, nueva sección §25, versión **1.7.0**, cambio aditivo)

### `GET /v1/juego/buzon` (token `phx_…`)
Devuelve los avisos **vigentes** para esta PC:
- los del usuario no expirados con `dispositivo_id` nulo o igual a esta PC;
- más los globales no expirados.

Los devuelve **ordenados del más nuevo al más viejo** (máx. 10). Usa el **sondeo con ETag** de §21: `If-None-Match` o `?etag=`, y **304** si nada cambió.

Respuesta:
```json
{ "ok": true,
  "avisos": [ { "id": 12, "texto": "hola causa", "tipo": "aviso", "creado_en": "2026-10-09T05:42:31.000Z", "global": false } ],
  "sondeo_seg": 15,
  "etag": "W/\"…\"",
  "solicitud_id": "…" }
```
- **ETag:** se calcula con los `id` y `entregado_en` de la lista (no con la hora).
- **Lista vacía:** `avisos: []` (sigue siendo 200 con su etag).
- **Interruptor apagado:** si `buzon_juego` es false → `avisos: []`, `pausado: true`.

### `POST /v1/juego/buzon/entregado` (token)
- **Cuerpo:** `{ "ids": [12, 13] }` (máx. 20).
- **Qué hace:** marca `entregado_en = now()` solo en avisos que esta PC podía ver. Es idempotente.
- **Responde:** `{ "ok": true, "marcados": 2 }`.
- **Globales:** como un global es de muchos, guarda la entrega por dispositivo en una tabla aparte `juego_avisos_entregas(aviso_id, dispositivo_id, entregado_en)`, o como te parezca mejor.

**Errores:** los de siempre del contrato (`TOKEN_*`, `DEMASIADOS_INTENTOS`, `ERROR_INTERNO`). Limita a 1 llamada cada 5 s por dispositivo.

**Cuota del plan FREE:** Link solo va a preguntar **mientras PES2021.exe está abierto**, cada `sondeo_seg`. Calcula y anota en §25 el coste estimado (como en §21).

## 3. Página web (diseño simple ahora; el bonito lo hacemos después)
- En la página de Fralex (perfil o «Mi club»), una tarjeta **«Mandar aviso a mi juego»** con:
  - un área de texto (contador 0/600);
  - un selector de PC (todas o una de sus PCs vinculadas);
  - el botón **Enviar**.
- **Lista de los últimos 10 avisos** con su estado:
  - ⏳ «esperando que el juego esté abierto»;
  - ✅ «entregado al juego hh:mm» (de `entregado_en`);
  - ⌛ «expirado».
- Se refresca sola (Realtime o sondeo cada 10 s).
- **Staff:** en el panel de staff, «Aviso global a todos los juegos».

## 4. Al terminar
- Despliega la migración y la función.
- Prueba con `curl` usando un token de prueba, **nunca el de Fralex**: 200, 304, entregado e idempotencia.
- Actualiza `docs/contrato-v1.md` §25 y sube `version_api` a 1.7.0.
- Línea en `REGISTRO.md` y documento en Drive.
- **Dime en una frase** «§25 desplegado, versión 1.7.0» para avisar al chat LINK.
