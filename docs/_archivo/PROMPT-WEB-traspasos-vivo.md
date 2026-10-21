# PROMPT para el chat WEB · Traspasos en vivo — **OBSOLETO: implementado directamente**

> **No hace falta pegar nada.** Fralex pidió que la parte web se hiciera desde el chat principal de Link: la función **`traspasos`** (migración 105, rama `borrador` de `pes-liga-automatica`, commit `bddb627`) ya está **desplegada** y probada contra la web real (6 rutas, 29 comprobaciones). El contrato vigente está en `C:\dev\pes-liga-automatica\docs\TRASPASOS-EN-VIVO.md` y resumido en `docs/TRASPASOS-EN-VIVO.md` (§3) de este repo.
>
> La versión anterior de este prompt (canal por anfitrión, aprobación de seguidores, rutas `/v1/traspasos/*` dentro de «liga») **ya no vale**: el modelo real es **código de 8 caracteres por anfitrión aprobado**, seguir con el código (`prueba:true` para seguirse a uno mismo), `seq` global y la función aparte `traspasos`.

## Contrato vigente (resumen)
Base: `https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/traspasos/v1` · `Authorization: Bearer phx_…` + `X-Phoenix-Version` + `X-Phoenix-Build` · respuestas `{ok:true,…}` o `{ok:false,codigo,mensaje,reintentable,reintentar_en?}`.

| Ruta | Quién | Cuerpo | Respuesta |
|---|---|---|---|
| `POST /traspasos` | anfitrión (host aprobado) | `{id 24hex, huella 64hex, creado ISO, autor≤40, parche≤60, base_huella≤64, cambios[1–60]{jugador_id>0, jugador≤48, desde_id≥0, desde≤48, hasta_id>0, hasta≤48}}` | `{seq, repetido}` (idempotente por `(anfitrión,id)`); 12/min; ≤32 KB |
| `GET /traspasos?desde=<seq>&limite=<1-50>` | amigo | — | `{lotes:[{seq,id,anfitrion,anfitrion_id,huella,creado,autor,parche,base_huella,cambios}],ultimo_seq}` (los de quien sigue y los propios); 40/min |
| `POST /traspasos/codigo` | anfitrión | `{rotar?}` | `{codigo,seguidores:[{nombre}]}` |
| `POST /traspasos/seguir` | amigo | `{codigo,prueba?}` | `{anfitrion,anfitrion_id}` |
| `POST /traspasos/dejar` | amigo | `{anfitrion_id}` | `{}` |
| `GET /traspasos/siguiendo` | amigo | — | `{sigo:[{anfitrion_id,nombre}],me_siguen:[{nombre}]}` |

Errores propios: `CODIGO_INVALIDO` 404 · `SEGUIR_A_UNO_MISMO` 409 · `HOST_NO_AUTORIZADO` 403. Los de siempre: `TOKEN_*` 401, `DEMASIADOS_INTENTOS` 429, `LIGA_PAUSADA` 503, `RUTA_NO_EXISTE` 404.
