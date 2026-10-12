# Fichajes como CAMBIOS (en vez de una copia de la base 26)

Investigación de solo lectura, 12 oct 2026. No se tocó el juego ni `PhoenixSync/`. Los scripts de análisis (Python) quedaron en el scratchpad de la sesión; no se commitean.

## 1. Formato de las bases (verificado)

Cada `.bin` de `CGP_database.cpk` (CPK sin comprimir, tabla `@UTF` enmascarada) y de `Phoenix-DB` tiene una cabecera de 16 bytes `FF 10 81 'WESYS'` + `u32 tamaño comprimido` + `u32 tamaño real`, y detrás un flujo **zlib** normal (`78 DA`): `zlib.decompress(d[16:])`. (`Country.bin` va sin esa cabecera.)

**PlayerAssignment.bin** (lo único que dice «jugador → equipo»): sin cabecera, registros de **16 bytes** = `u32 ×4`:

| campo | significado |
|---|---|
| 0 | id de fila (1…21942; único; no es estable entre bases) |
| 1 | **player_id** (1697…235644; un jugador puede salir en 2 filas: club + selección) |
| 2 | **team_id** |
| 3 | empaquetado: `>>10` = puesto en la plantilla (0…39, único por equipo), `&1023` = otro número único por equipo (probablemente dorsal). Interpretación probable, sin confirmar |

Filas: 26 = 20.398, 27 = 19.612, UEFAGOL 27 = 19.489, Clásicos = 15.562, Phoenix-DB = 19.747. Máx. 40 jugadores por equipo.

**Player.bin**: cabecera de 8 bytes (`u32` 0 en la 26 / 1250 en 27, Phoenix y UEFA; `u32` 0) + **29.997 huecos de 312 bytes**. Los 4 primeros bytes de cada registro = player_id (0 = hueco vacío). No lleva el equipo, así que un fichaje es solo un cambio en PlayerAssignment.

**Team.bin**: tiene secciones, no es de registro fijo; no se decodificó. Para «equipo válido» sirve que el `team_id` exista en la PlayerAssignment de la base destino.

## 2. Qué diferencia hay entre la 26 y Phoenix-DB

Hallazgo importante: **Phoenix-DB NO es «la 26 + fichajes»**. Es un híbrido:

- `Player.bin` de Phoenix: 24.038 de 29.932 registros idénticos a la 26; en otros hay campos con valor del 27 (bytes 12/13/19/43) y la cabecera es la del 27. 2.725 jugadores no coinciden ni con la 26 ni con la 27.
- `PlayerAssignment`: 3.677 pares (jugador, equipo) distintos de la 26 y 6.119 distintos de la 27. 2.531 jugadores cambian de equipo respecto a la 26 y **2.356 de ellos ya están igual que en la 27**. Casi todo lo «distinto» es deriva de base (una 26 parcialmente actualizada), no fichajes de Fralex.

Lo que queda **fuera de la 27** (lo que parece de Fralex): **175 jugadores**.

| grupo | n | qué es |
|---|---|---|
| Altas en equipo real | 12 | p. ej. 200050 → 1258, 201930 → 2725, 208123 → 2661, 208750 y 208799 → 139, 208912 → 2195, 208914 → 2651, 150927 → 5373, 160723 → 5370, 167093 → 5001, 164757 → 5503, 129375 → 2208 |
| «Player Identidad» aparcados en los equipos 9107/9108 | 37 | marcadores de posición; esos equipos **no existen en la 27** (0 jugadores) |
| Bajas (jugador sin equipo en Phoenix) | 126 | quedan libres; sin pruebas de que sean intencionadas |

Los 37 de Identidad y la mayoría de las bajas tienen `Player.bin` igual al de la 26: el fichaje no toca Player.bin.

## 3. Cuántos se reaplican limpios sobre la base 27

Regla estricta (jugador existe + origen igual que en la 26 + destino válido): **solo 2 de 175**. En la 27 el jugador ya cambió de equipo (134 casos) o el destino no existe (37).

Regla relajada (jugador existe, destino válido, **gana el cambio de Fralex** aunque el origen sea otro):

| base | altas reales aplicables (de 12) | falta jugador | falta equipo |
|---|---|---|---|
| ConmeGOL 27 | **10** (8 con origen cambiado por el parche) | 2 | 0 |
| UEFAGOL 27 | 7 (6 con origen cambiado) | 4 | 1 |

Los 37 «Identidad» no tienen sitio en la 27: no se aplican. Las 126 bajas parecen deriva y no conviene guardarlas como cambios sin confirmarlo con Fralex.

**Conclusión:** los fichajes reales son ~12 filas, no una base de 19.747. La copia completa arrastra miles de cambios ajenos y tapa a los jugadores nuevos del 27.

## 4. Diseño propuesto

**Formato** (`fichajes.json`, versión 1; operaciones, no filas):

```json
{ "version": 1, "creado": "2026-10-12T…", "autor": "Fralex",
  "baseOrigen": { "modo": "ConmeGOL 26", "sha1PlayerAssignment": "…" },
  "cambios": [
    { "op": "mover", "jugador": 208750, "nombre": "…", "de": 9103, "a": 139 },
    { "op": "alta",  "jugador": 200050, "a": 1258 },
    { "op": "baja",  "jugador": 150927, "de": 2504 }
  ] }
```

- `mover` = quitar la fila (jugador, `de`) y crear (jugador, `a`). `alta` = sin equipo previo. `baja` = solo si Fralex la pide a propósito.
- La clave es siempre `player_id` (estable entre bases); `nombre` es solo para mostrar.

**Quién guarda y quién aplica**
- **Sync** (guarda): compara la base reconocida del modo con la edición de Fralex y publica **solo** estas operaciones (unos KB) en vez de los `.bin` completos de `sync-publicar-oficial --base`. Hay que comparar contra la base real de cada modo, no una 26 asumida; si no, la deriva se cuela como fichaje.
- **Link** (aplica): al iniciar un modo, lee la `PlayerAssignment.bin` del CPK ACTIVO, aplica la lista y escribe el resultado en `Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin` (cabecera WESYS + zlib). Phoenix-DB ya no lleva `Player.bin`: se usa el del parche. Así Phoenix-DB sí se puede poner en el 27 (sustituye la regla «sin base reconocida no se pone»).

**Comprobaciones previas (cada cambio)**
1. El jugador existe en `Player.bin` del parche activo.
2. El equipo destino existe en esa base.
3. El destino tiene sitio (< 40). Fila nueva: id = máx+1; empaquetado = siguiente puesto libre `(n<<10)|número libre`.
4. Si hay `de`: ¿sigue ahí el jugador? Si no → conflicto.

**Conflictos** (el parche ya movió al jugador, o el equipo no existe): por defecto **se salta y se avisa** con la lista («208750: en el 27 ya está en 5050»). Opción por jugador «forzar el de Fralex». Nunca se inventan equipos ni se deja una fila inválida.

**Copia de seguridad y vuelta atrás**
- Antes de aplicar: guardar la `PlayerAssignment.bin` anterior (patrón `.anterior` ya existente) y un `aplicado.json` (qué entró, qué se saltó, hash de la base).
- Idempotente: se reaplica solo si cambia el hash del CPK activo o el `fichajes.json`.
- Vuelta atrás: restaurar `.anterior` o quitar la raíz Phoenix-DB; el juego vuelve a la base pura del parche. Nunca se toca el CPK del parche.
- Escribir con PES cerrado, igual que con `sider.ini`.

**Por verificar / límites**
- Significado exacto de los 10 bits bajos del campo 3: probar un fichaje en el juego.
- Team.bin sin decodificar; el equipo se valida por PlayerAssignment.
- Clásicos usa ids distintos (240001…); la lista debe llevar el modo de origen y no aplicarse allí.
- Jugadores nuevos que no existen en la base requieren `Player.bin`: fuera de este diseño.
