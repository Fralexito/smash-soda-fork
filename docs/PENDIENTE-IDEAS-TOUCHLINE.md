# PENDIENTE · Ideas de Touchline para Phoenix (decidido 30-oct-2026)

**Decisión de Fralex:** copiar lo mejor de Touchline y de la Liga Máster de PES y aplicarlo a Phoenix, en versión **multijugador** (liga de personas reales). Hacerlo **cuando lo actual esté terminado**. Nada se construye sin su aprobación.

**Otras decisiones del mismo día:**
- Se abandona el soporte de varios parches. A futuro habrá un parche propio (Phoenix Evolution Patch). El Phoenix ID entre parches deja de ser urgente.
- Phoenix Sync ya recarga fichajes sin cerrar el juego (phoenix.lua, ver `PhoenixSync/MANUAL-SYNC.md` en `mercado-fase0`), pero falla a veces: **pulirlo es prioridad**.

**Documentos de apoyo (en esta misma carpeta):**
- `mapeo-touchline.md` — todas las funciones de Touchline y cómo están hechas.
- `comparacion-phoenix-vs-competencia.md` — Phoenix vs. My Football Legacy y Touchline.
- `herramientas-parche-pes2021.md` — caja de herramientas de la comunidad.

## Qué copiar (W = web · L = Phoenix Link · S = Phoenix Sync)

### Primero
1. **Modo seguro por versión** (S/L): revisar solo las partes del exe que usa Sync; si no coincide, aplicar por archivo en vez de fallar a medias. Puede ayudar con el "a veces falla".
2. **Instalador que muestra todo antes** + respaldo de `sider.ini` (L).
3. **Dorsales en vivo**: el DT los elige en la web y aparecen en el juego del host (W+S).
4. **Público según el partido**: clásico = estadio lleno (S; base: módulo Crowd Attendance).
5. **Tácticas del rival y sugerencia de formación** antes de un reto (W).
6. **Negociación de fichajes entre DTs**: oferta → contraoferta → sueldo/años/rol (W).
7. **Resultado leído del save** como respaldo de Link (L/S).

### Después
- Bandeja con mensajes de jugadores y directiva (W).
- Metas de temporada y patrocinadores con objetivos; dinero al cumplir (W).
- Despido del DT por malos resultados, opcional (W).
- Prensa con preguntas tras partidos destacados → Noticias (W).
- Forma según racha en el juego del host (S).
- Inicio en 3 columnas: próximo partido, noticias, tabla (W).
- Navegación atrás/adelante y uso con mando en Link (L).

### Al final
- Juveniles con nombres nuevos, traspasos forzados, retiro → cuerpo técnico, eventos aleatorios, repatriar cedidos.

### No copiar
- Depender de una sola versión del exe.
- Pedir administrador para todo.

## La idea guía
PES hace la Liga Máster para **uno solo** contra la máquina. Phoenix la hace para **toda una liga de amigos**: negocias con personas, la prensa habla de tus amigos, la directiva te compara con otros DTs reales.

---

## PLAN A FUTURO · Fichajes en memoria (agregado 30-oct-2026)
**Decisión de Fralex:** primero estabilizar la recarga por archivo (R3 / Activar) para que funcione el 100% de las veces. Después, como experimento aparte, intentar los fichajes en memoria. Detalle y motivos en `INFORME-VIABILIDAD-FICHAJES-EN-MEMORIA.md`.

**Orden:**
1. **Estabilizar R3 (ahora):** escribir cada fichaje en option file y `PlayerAssignment.bin` a la vez; detectar de cuál lee el juego (arreglar el caso "después de Editar"); avisar si falla; confirmar tras recargar; prueba guiada de 10 fichajes con un amigo.
2. **Experimento en memoria (futuro), por fases:**
   - **Fase 1 · Solo mirar:** foto de la memoria antes y después de R3 con un fichaje (lectura segura con `ReadProcessMemory`, sin escribir). Contar cuántas tablas cambia el juego.
   - **Fase 2 · Decidir:** si son pocas y fáciles de ubicar, seguir; si no, abandonar sin haber roto nada.
   - **Fase 3 · Escribir:** solo en menús, nunca en partido, revisando bytes antes (como `phoenix.lua`), en una copia de prueba.
   - **Guardado:** seguir escribiendo también el archivo, como hoy. La memoria solo quitaría el paso de presionar R3.

**Avisos:** el jugador vive en varias tablas (la Liga Máster usa ~11); escribir stats en memoria ya falló (el juego usa otras copias); leer memoria a lo bruto cerró el juego (ReShade). Solo la PC del host necesita los cambios (Parsec).
