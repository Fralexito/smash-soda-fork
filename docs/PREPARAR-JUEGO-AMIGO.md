# Preparar PES 2021 de un amigo para Phoenix (phoenix.lua v0.18)

Esta guía es para cuando **tu amigo también hostea**, o sea, corre PES y Phoenix Link en su PC. Si solo entra a tu sala por Parsec, no necesita nada de esto: mira `PRUEBA-ONLINE-CON-AMIGO.md`.

**Lo que tiene que tener antes:**
- PES 2021 con **ConmeGOL Patch 26** y su switcher, funcionando.
- Phoenix Link instalado con `PhoenixLink-instalar.bat`. Solo hace falta si va a usar Link.

**Lo que se instala (en cada carpeta de Sider del juego):**

| Qué | Dónde |
|---|---|
| `phoenix.lua` v0.18 | `SiderAddons\modules\` |
| `lua.module = "phoenix.lua"` | `sider.ini`, debajo del último `lua.module` activo |
| `cpk.root = ".\livecpk\Phoenix-DB"` | `sider.ini`, **justo antes** de `cpk.root = ".\olmosjr23\Database"` (la base del parche) |
| carpeta `content\phoenix\` (vacía) | buzón de avisos de la web |
| carpeta `livecpk\Phoenix-DB\common\etc\pesdb\` (vacía) | aquí llegan los fichajes desde Phoenix Sync |

Las carpetas de Sider son **dos**, y hay que tocar las dos:
1. `<juego>\SiderAddons`
2. `<juego>\ConmeGol Extras\ConmeGOL Patch 26\SiderAddons`

Cada vez que cambias de modo, el switcher copia la segunda encima de la primera. Si solo tocas la primera, Phoenix desaparece en el próximo cambio.

> **No copies tu `Player.bin` ni tu Phoenix-DB** a la PC del amigo. Esos datos salen de la base de olmosjr23 y no son nuestros para repartir. La carpeta Phoenix-DB vacía funciona bien: los datos le llegan por Phoenix Sync.

---

## Opción 1 — Con Phoenix Link (lo más fácil)

1. **Cierra PES.**
2. Si es la primera vez, abre PES **una vez** con Link abierto, para que Link aprenda dónde está el juego. Después ciérralo.
3. En Link ve a **SYNC › Puente › Módulos del juego**.
4. En `phoenix.lua v0.18` pulsa **INSTALAR**. Si ya tenía otra versión, el botón dice **ACTUALIZAR**.
5. Debe decir **INSTALADO**, con ● en las dos carpetas.

Link también:
- hace una copia de `sider.ini` antes de tocarlo (`sider.ini.phoenix-<fecha>`);
- guarda la versión vieja como `phoenix.lua.antes-v<versión>`.

## Opción 2 — Sin Link: `PhoenixJuego-instalar.bat`

1. Pásale `PhoenixJuego-instalar.bat`. Mejor junto con `phoenix.lua` en la misma carpeta; si no lo lleva, el .bat lo baja de GitHub.
2. **Cierra PES.**
3. Doble clic en el .bat y elige la carpeta del juego (la que tiene `PES2021.exe` y `SiderAddons`).
4. Debe terminar con `LISTO: phoenix.lua v0.18 en 2 carpeta(s).`

El .bat:
- hace exactamente lo mismo que el botón de Link (mismas copias de seguridad);
- comprueba el sha256 de `phoenix.lua`; si no es la v0.18 exacta, **no toca nada**;
- se puede ejecutar dos veces sin problema: la segunda no cambia nada.

## Opción 3 — A mano

En **cada una** de las dos carpetas de Sider:

1. Copia `sider.ini` a `sider.ini.respaldo` (por si acaso).
2. Copia `phoenix.lua` (v0.18) en `modules\`. Si ya había uno, renómbralo antes a `phoenix.lua.viejo`.
3. Abre `sider.ini` con el Bloc de notas:
   - busca la línea `cpk.root = ".\olmosjr23\Database"` y **justo encima** escribe:
     ```
     cpk.root = ".\livecpk\Phoenix-DB"
     ```
   - baja hasta la lista de `lua.module = ...` y **al final** añade:
     ```
     lua.module = "phoenix.lua"
     ```
   - guarda.
4. Crea las carpetas vacías `content\phoenix` y `livecpk\Phoenix-DB\common\etc\pesdb`.

Nunca dejes **dos** líneas de Phoenix-DB ni dos de `phoenix.lua`.

---

## Comprobar que funciona

1. Abre PES con el switcher, como siempre.
2. Abre `SiderAddons\sider.log` con el Bloc de notas:
   - debe aparecer `[phoenix.lua]`;
   - **no** debe decir `Module (phoenix.lua) is NOT activated`.
3. En Link, **SYNC › Puente › Módulos del juego** debe decir «✅ Cargó en el último arranque del juego».
4. En el juego, el panel de Sider (barra espaciadora) muestra los avisos de la web, y en Partido aparece el botón «Datos Actual. en vivo».
5. **Modo de recarga:** queda en **ACTIVAR**, el probado. Se cambia en SYNC › Fichajes, que escribe `content\phoenix\modo.txt`.

## Extra opcional: el módulo del partido

`phoenix_estadio.lua` lleva el marcador y los goles del juego a Link. Solo se instala desde Link: **SYNC › Puente › Módulos del juego › INSTALAR**, con PES cerrado.

## Volver atrás

- Restaura `sider.ini.phoenix-<fecha>` (o tu `sider.ini.respaldo`) como `sider.ini` en las dos carpetas.
- O en Link pulsa **APAGAR**: comenta la línea `lua.module` y deja el archivo por si lo vuelves a encender.
