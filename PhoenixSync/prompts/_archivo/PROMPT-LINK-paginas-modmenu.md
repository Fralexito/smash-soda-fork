# Prompt para el chat LINK — páginas propias en «Sider Mod Menu» (F1) para phoenix_referee y phoenix_campo

18 de octubre de 2026. De: chat SYNC (rama `mercado-fase0`). Pedido de FRALEX: «hazlo más visual para que se vea mejor en ese programa, y adaptable a futuras versiones del programa».

---

Hola, chat LINK. FRALEX va a usar **Sider Mod Menu v1.2** (`SiderModMenu-v1.2.zip`, de FlyingDutchman50, licencia MIT, fuente incluida). Es un módulo de Sider (`modmenu.lua` + `modmenu.dll`, Dear ImGui) que abre con **F1** un menú con ratón: barra lateral, tarjetas, interruptores, deslizadores, listas, botones e imágenes. Los módulos que ya tienen overlay (Espacio) salen solos como página «classic overlay». Los que se adaptan tienen una página **nativa**.

**Sync ya lo hizo para `phoenix.lua` (v0.18c, instalada en el PC de FRALEX el 18 oct).** Copia su patrón. Está en `PhoenixSync/sider/pruebas/phoenix-v0.18c-prueba.lua`, bloque «v0.18c» al final; simulación en `simular_v018c.py`, 7/7, más una prueba con el `modmenu.lua` real.

## Reglas (para que aguante versiones futuras del menú)
1. **Todo lo del menú en un bloque**, con el adaptador `UIX` (cópialo tal cual): cada pieza se usa solo si existe (`type(ui.x) == "function"`). Si falta, se muestra como `ui:text`, y si tampoco existe, no se hace nada.
2. **Sin menú no cambia nada.** Si `ctx.menu`/`ctx.menu.add` no existen, el módulo registra como siempre.
3. **Registra `overlay_on`/`key_down` con `ctx.modmenu_register` si existe.** Es el registro original de Sider que guarda el menú. Así no sale además una página «classic overlay» repetida. Si no existe, usa `ctx.register`.
4. **Escribe literalmente `ctx.menu.add(`** en el código. El menú busca ese texto en cada módulo para no tomarlo como «clásico» y no desordenar las páginas de los demás.
5. **No hay `pcall`** (ni `setmetatable`, `select`, `next`, `require`, `print`). Cuenta los errores de `build` con una bandera, como hace `phoenix.lua`: a los 3, la página pasa a mostrar el texto del overlay.
6. **`build` se llama en cada cuadro.** Solo debe pintar lo que el módulo ya tiene en memoria. No leas archivos ni memoria del juego en `build`; si hace falta, que sea con temporizador (como mucho cada 1–2 s).
7. **`modmenu.lua` debe ser el PRIMER `lua.module` de `sider.ini`.** El instalador de «Módulos del juego» de Link no debe ponerse por encima. Si ve `modmenu.lua`, que deje los módulos de Phoenix debajo. Con ConmeGOL 27 basta la raíz: los modos ya no traen `sider.ini` ni Luas de Phoenix.

API v1.2 (DEVELOPERS.md del zip):
- Piezas: `section(title, icon, sub)`, `text(text, style)`, `kv(label, value, tone)`, `toggle(id, label, on, fn(on), hint)`, `choice(id, label, options, selected, fn(i))`, `slider(id, label, value, min, max, step, fmt, fn(value, commit))`, `stepper`, `list(id, label, items, sel, fn(i, activated), {rows, search})`, `button(id, label, fn, style, icon)`, `row/endrow`, `cols/nextcol/endcols`, `image(path, max_w, max_h, caption)`, `message(text, tone)`, `separator`, `space`.
- Tonos de `kv`: `ok`, `warn`, `accent`, `muted`. Tonos de `message`: `info`, `ok`, `warn`, `err`.
- Íconos: `settings home globe camera search close check refresh star add remove left right down up person palette tv grid sun moon cloud flag info warning delete save tag trophy lock unlock play list edit shuffle ball game puzzle speed brush layers filter music heart link open`.

## Página 1 · `phoenix_referee.lua` → «Árbitro y marcador» (icono `trophy`, order 16)
- **Tarjeta de marcador:** `section("Barcelona 2 – 1 Real Madrid", "ball", "Minuto 67 · segundo tiempo")`. Fuera del partido: «Sin partido en curso».
- **Goles:** una fila `kv("61' Lamine Yamal", "Barcelona")` por gol.
- **Jugadores:** `kv("Local · Fralex", "18 ms", tono)` y lo mismo para el visitante. Tono `ok` por debajo de 50 ms, `warn` hasta 120 ms, y por encima `warn` con el texto «alto».
- **Árbitro:** el modo (competitivo/amistoso), pausas y el último mensaje del chat de la sala (`sala.txt`), con datos que ya tenga en memoria.
- Solo lectura, como hoy. Sin botones que cambien el partido.

## Página 2 · `phoenix_campo.lua` → «Tamaño del campo» (icono `grid`, order 17)
- Hoy **no tiene página**, porque no registra `overlay_on`; con esto aparece por primera vez.
- `toggle("activo", "Activo", activo, fn)`.
- `slider("largo", "Largo (mitad, m)", largo, 30, 60, 0.01, "%.2f", fn)` y lo mismo para el ancho (20–40). Guarda en `phoenix_campo.ini` **solo cuando `commit` = true** (al soltar), no en cada cuadro.
- `row` + `button("aplicar", "Aplicar (F9)", …, "primary", "check")` + `button("restaurar", "Restaurar (F10)", …, "normal", "refresh")` + `endrow`: lo mismo que hacen hoy F9 y F10.
- `message` con el estado: «Campo encontrado y aplicado» (ok), «Buscando el campo…» (info) o «Fuera de partido» (muted).

## Instalador de Luas de Link
- Sustituye la `phoenix.lua` que reparte Link por la **v0.18c**: `PhoenixSync/sider/pruebas/phoenix-v0.18c-prueba.lua`, con LF, sha256 `5855215799b86317fad591f4152a40889a5e47bfe7333719f2800b73976e6b82`. Lleva AUTO-SIEMPRE por defecto y la página del menú. Esto sustituye el pedido de la v0.18b en `PROMPT-LINK-modo-por-defecto.md`.
- Opcional: que el instalador ofrezca también Sider Mod Menu, siempre como primer módulo y con copia de `sider.ini`.

Compila sin errores, sube la versión de Link (`kVersion`), simula las páginas con un `ui` falso (ver `simular_v018c.py`) y anota una línea en `REGISTRO-LINK.md`.
