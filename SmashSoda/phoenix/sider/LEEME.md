# Phoenix Estadio — módulo de Sider + árbitro de Phoenix Link

Une el partido de PES 2021 con Phoenix Link (Parsec: PES corre solo en la PC del anfitrión).
Documento completo (qué se hizo, cómo, por qué, cómo conviven con `phoenix.lua` en el mismo paquete de Luas y cómo replicarlo): `docs/PHOENIX-ESTADIO-ARBITRO.md`.

> Esta carpeta también lleva **`phoenix.lua` v0.18** (el de Phoenix Sync), para instalarlo en la PC de un amigo desde «Módulos del juego» o con `PhoenixJuego-instalar.bat`. Guía: `docs/PREPARAR-JUEGO-AMIGO.md`. Es copia exacta del de Sync (sha256 `a39b54de…`): no editarlo aquí.

## Qué se vive
- **En el juego** (overlay de Sider: Espacio, y con 1 / º hasta «PHOENIX ESTADIO»): marcador con los nombres reales de los equipos, minuto y tiempo, goles con su minuto, quién juega en cada lado con su mando y su ping (bien / justo / ALTO), el estado del árbitro y las últimas líneas del chat de la sala.
- **En Link y en el chat de la sala**:
  - el marcador arranca solo cuando el balón echa a rodar (si los lados ya están elegidos);
  - cada gol se anuncia con su minuto («GOL 34' de Mirko. Mirko 2-1 Kaiser»);
  - se anuncia el descanso;
  - al final, el partido se guarda en el historial y se avisa a la web, con los goles.
- **Juego limpio**: mientras se juega, los invitados no pueden usar Start, Back ni Guía (modo competitivo). Si alguien se cae o tiene más de 150 ms de ping durante 5 s, el juego se pausa (pausa automática; viene apagada hasta probarla). La pausa nunca se pulsa durante una repetición o una celebración: espera a que el reloj vuelva a correr.

## Archivos (todos en `<juego>\SiderAddons\content\phoenix\`)
| Archivo | Lo escribe | Cuándo | Contenido |
|---|---|---|---|
| `estado.json` | `phoenix_estadio.lua` | cada ~1 s **solo durante el partido**, y al momento en un gol o un cambio de fase | `{v:2, fase, minuto, segundo, periodo, anadido, reloj_corre, local, visita, nombre_local, nombre_visita, goles_local, goles_visita, pk_local, pk_visita, goles:[{m,l:"l"|"v"}], completo, torneo, seq}` |
| `resultado.json` | `phoenix_estadio.lua` | una vez, al salir de un partido **completo** | igual que `estado.json` con `fase:"final"` + `fin` (hora Unix) |
| `sala.txt` | Phoenix Link (atómico) | al cambiar, y cada 4 s con la hora `t=` | líneas `clave=valor`: `abierta`, `sala`, `local`, `visita`, `marcador`, `goles`, `ja=`/`jb=` `nombre\|ping\|mando` (local/visita), `arbitro`, `chat` (×4), `t` |
| `avisos.txt` | Phoenix Link | ya existía (avisos de la web; lo muestra `phoenix.lua`) | — |

**Fases** (`match.stats()` no dice si el juego está en pausa, así que se deduce del reloj):
- `menu`: previa del partido (periodo 0), o se salió a medias;
- `en_juego`: lo normal;
- `descanso`: reloj quieto 3 s en el 45' (o en el 105');
- `pausado`: reloj quieto 45 s;
- `final`: se salió del partido después del 89' o en la prórroga/penales (`completo:true`).

Link solo bloquea con `en_juego` y solo cierra el marcador con `final` + `completo`. Un partido abandonado no cierra nada: solo avisa en el chat.

## Seguridad
- Solo lectura del juego (`match.stats()` y `ctx`). No escribe memoria, no usa `ffi` ni teclas, y no cambia nada del partido, así que no puede desincronizar a nadie.
- Fuera del partido solo mira `match.stats()` dos veces por segundo.
- El Lua de Sider 7.3.3 **no trae `pcall` ni `os.rename`**:
  - los errores se cuentan con una «bandera» y, tras 5 seguidos, el módulo se apaga solo (queda en `sider.log`);
  - `estado.json` se escribe directo (no se puede renombrar), así que Link descarta una lectura a medias y se queda con la anterior (máx. 3 s).
- Necesita `match-stats.enabled = 1` en `sider.ini` (el ConmeGOL de Fralex ya lo tiene, línea 7).
- Los nombres de los equipos salen de `content\kit-server\map.txt`. Si no existe, se ve «Local» / «Visita».

## Instalar (en la PC del anfitrión, con el juego cerrado)
**Con un botón (recomendado):** Phoenix Link › SYNC › Puente › «MÓDULOS DEL JUEGO» › INSTALAR. Hace los pasos de abajo en las dos carpetas, guarda una copia de `sider.ini` y comprueba que el archivo copiado es idéntico. APAGAR comenta la línea.

**A mano:**
1. Copiar `phoenix_estadio.lua` a `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\modules\` (la carpeta que copia el switcher) **y** a `<juego>\SiderAddons\modules\`.
2. Guardar una copia de `sider.ini` y añadir al final de la lista de módulos, debajo de `lua.module = "phoenix.lua"`:
   `lua.module = "phoenix_estadio.lua"`
3. Abrir el juego. En `sider.log` debe salir `[estadio] v1.0 listo`.

**Desinstalar:** quitar esa línea (o poner `;` delante).

## Probar (orden recomendado, offline primero)
1. Amistoso contra la CPU con el overlay en PHOENIX ESTADIO: comprobar marcador, minuto, nombres y goles con minuto.
2. Mirar `content\phoenix\estado.json` mientras se juega: debe cambiar cada segundo y dejar de cambiar en el menú.
3. Con Link abierto y la sala abierta: Partido › En vivo muestra «Juego: en juego · 34'» y los goles. Mandos › Bloqueo muestra «START BLOQUEADO».
4. Un gol, el descanso y el final: mirar el chat y el historial.
5. Solo entonces, encender la pausa automática y probarla con un amigo (desconectarse a propósito).

Anotar cada prueba en `PhoenixSync/PRUEBAS.md`.
