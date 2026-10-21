# PROMPT PARA EL CHAT SYNC — `phoenix.lua` cuenta el partido a Phoenix Link (estado.json + resultado.json)

> **ACTUALIZACIÓN 2026-10-10 (chat LINK, a pedido de Fralex): el módulo ya existe y es aparte.**
> Fralex pidió el sistema completo y Link lo hizo **sin tocar `phoenix.lua`**: es un módulo nuevo e independiente, `phoenix_estadio.lua`. Está en la rama `rediseño-phoenix-portal`, en `SmashSoda/phoenix/sider/` (con `LEEME.md`).
> - Escribe `estado.json` **v2** (añade `segundo`, `anadido`, `reloj_corre`, nombres de equipo de kit-server, penales, `goles[]` con minuto y `completo`) y `resultado.json`. Lee `sala.txt`, que escribe Link, para un HUD en el overlay.
> - **Dos correcciones a lo de abajo:** el Lua de Sider 7.3.3 **no trae `pcall` ni `os.rename`** (lo dice el volcado de `env.lua` en `sider.log`). Por eso el módulo usa la «bandera» de errores y escribe `estado.json` directo; Link descarta una lectura a medias.
> - **Lo que te pido ahora** (en vez de reprogramarlo en `phoenix.lua`):
>   1. revisa el módulo con tus reglas de RIESGOS-SIDER;
>   2. si te parece bien, instálalo con una línea más en `sider.ini` (carpeta del modo de ConmeGOL), con respaldo;
>   3. haz las pruebas del `LEEME.md` y anótalas en `PRUEBAS.md`.
>
>   **Mejor NO fusionarlo con `phoenix.lua`** (v0.17e, que solo está en el juego: súbela a git). La v0.17e parchea código del juego (botón nativo). Este módulo es solo lectura y trabaja solo en el partido: separados, si uno se apaga el otro sigue. Comparten la carpeta `content\phoenix\`, pero ningún archivo.
> - La fase 2 (`partido.json`) sigue igual: pendiente y a acordar entre los dos chats.
>
> El texto original sigue debajo, como referencia del formato v1 (Link entiende v1 y v2).

Hola. Soy el chat LINK (rama `rediseño-phoenix-portal`, worktree `_phoenix-link`). Phoenix Link va a hacer de **árbitro del partido desde fuera del juego**: bloquea Start/Back/Guía a los invitados mientras se juega, pausa si alguien se cae y actualiza el marcador solo. Para eso necesita que `phoenix.lua` (tuyo) le cuente qué pasa en el partido. Sigue tu protocolo:
- rama `mercado-fase0`, carpeta `PhoenixSync/`; una línea por cambio en `PhoenixSync/REGISTRO.md`;
- **cada prueba en el juego se anota en `PhoenixSync/PRUEBAS.md`** (✅ / ❌ / ⏳, qué se vio);
- «si algo se rompe, mejor no lo hagas»; nada a `master` sin permiso de Fralex.

## Lo que Link ya hace (y no cambia)
- Escribe `avisos.txt` en `<juego>\SiderAddons\content\phoenix\` (atómico con `.tmp`).
- Lee lo que tú escribas en **esa misma carpeta**. Nunca crea carpetas ni toca `sider.ini`.
- Si `estado.json` no existe o tiene **más de 5 s** (fecha de modificación del archivo), Link muestra «sin datos del juego» y todo sigue en manual. Es decir: si el módulo se apaga, nada se rompe.

## FASE 1 (lo que pido ahora) — solo lectura

### 1. `estado.json`, cada ~1 s y SOLO durante el partido
Ruta: `<ctx.sider_dir>\content\phoenix\estado.json`. Contenido (UTF-8 sin BOM, una línea, sin comentarios):
```json
{"v":1,"fase":"en_juego","minuto":34,"periodo":1,"local":101,"visita":102,"goles_local":1,"goles_visita":0,"seq":57}
```
| Campo | Tipo | De dónde |
|---|---|---|
| `v` | int | versión del formato, hoy `1` |
| `fase` | `"menu"` \| `"en_juego"` \| `"pausado"` \| `"descanso"` \| `"final"` | ver abajo |
| `minuto` | int | `match.stats().clock_minutes` |
| `periodo` | int 0–5 | `match.stats().period` (1 y 2 tiempos, 3–4 prórroga, 5 penales) |
| `local`, `visita` | int | `ctx.home_team`, `ctx.away_team` (IDs; 0 si no se saben) |
| `goles_local`, `goles_visita` | int | `home_score`, `away_score` |
| `seq` | int | contador que sube en cada escritura (opcional; Link lo usa solo para depurar) |

Cómo deducir `fase` (propuesta; ajústala con lo que veas en el juego y anótalo en PRUEBAS.md):
- `match.stats()` da `nil` → `"menu"` (escribe UNA vez `menu` al salir del partido y deja de escribir).
- el reloj avanza → `"en_juego"`.
- el reloj no avanza ≥ 2 s en el mismo periodo → `"pausado"` (Link solo lo muestra; no decide nada con esto).
- entre periodo 1 y 2 (minuto ≥ 45 y el reloj quieto) → `"descanso"`.
- fin del partido (periodo 2/4/5 terminado, o `match.stats()` pasa a `nil` tras haber jugado) → `"final"`.
- **Lo importante para Link es que `en_juego` sea fiable**: con `en_juego` Link bloquea Start a los invitados, y solo con `en_juego` pulsa Start para pausar. Si dudas, NO pongas `en_juego`.

Requisito previo: `match.stats()` necesita `match-stats.enabled = 1` en `sider.ini` (función experimental de Sider). Decide tú si se activa y cómo (respaldo `sider.ini.respaldo-phoenix`, una línea, en la carpeta del modo de ConmeGOL, ver RIESGOS-SIDER.md #6 y #6b). Si está apagado, `match.stats()` da `nil` y el módulo no debe escribir nada.

### 2. `resultado.json`, una vez al terminar
Misma carpeta. Mismos campos que `estado.json` con `fase:"final"` + `pk_local`, `pk_visita` (penales, 0 si no hubo) + `fin` (`os.time()`). Link lo leerá más adelante para subir el resultado a la web (eso es otra tarea; hoy basta con que exista y sea correcto).

### Condiciones (obligatorias)
1. **Escritura atómica**: escribir `estado.tmp` / `resultado.tmp` y luego reemplazar (`os.rename` tras `os.remove` del destino, o el método que ya usas). Si el reemplazo falla porque Link está leyendo, se deja para el siguiente segundo (no reintentar en bucle dentro del frame).
2. **Solo lectura de la API de Sider** (`match.stats()`, `ctx`). **Nada de escribir memoria** en esta fase; nada de `ffi` nuevo.
3. Todo dentro de `pcall` y con el **contador de errores** que ya tienes (`MAX_FALLOS`): al llegar al tope el módulo deja de escribir estos archivos (y lo anota en `sider.log`).
4. **Coste cero fuera del partido**: si `match.stats()` es `nil`, no se escribe nada (salvo el `menu` único al salir). Dentro del partido, como mucho 1 escritura por segundo (comprobar con `os.clock()` antes de hacer nada más en el `display_frame`).
5. No usar teclas nuevas; no cambiar lo que ya hace v0.17 (avisos, botón nativo).
6. Probar offline en la PC de Fralex antes de nada con amigos, y **anotar cada prueba en `PRUEBAS.md`**.

### Cómo probarlo con Link
Link ya está listo para leerlo (Partido › En vivo muestra «Juego: en juego · 34'»). Para probar sin juego, Link usa un `estado.json` escrito a mano.

## FASE 2 (después, NO ahora) — `partido.json` (reglas del partido de liga)
Link (o Sync) dejará en la misma carpeta `partido.json` con las reglas del partido de liga:
```json
{"v":1,"id":"…","local":101,"visita":102,"duracion_min":10,"estadio":12,"clima":"despejado","hora":"noche"}
```
`phoenix.lua` lo leería en `set_teams` / `set_match_settings` / `set_conditions` (eventos que ya usan los módulos de ConmeGOL) para fijar equipos, duración, estadio y condiciones. **Marcado como fase 2**: solo cuando la fase 1 esté probada; es la primera vez que el módulo cambiaría algo del partido, así que va con la misma regla de RIESGOS-SIDER #8 (apagado en online/PESBUL) y su propia prueba en PRUEBAS.md. El formato exacto lo cerramos entre los dos chats antes de tocar nada.

## Respuesta que espero
Línea en tu `REGISTRO.md` con la versión de `phoenix.lua` que escribe `estado.json`, y la prueba en `PRUEBAS.md` (sobre todo: ¿la `fase` sale bien al pausar con Start, en el descanso y al final?).
