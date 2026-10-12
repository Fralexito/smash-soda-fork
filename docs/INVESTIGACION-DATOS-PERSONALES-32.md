# Investigación: ¿se pueden ampliar los 32 «datos personales» de PES 2021?

2026-10-12 · solo lectura (no se tocó el juego ni ningún guardado; no se escribió en memoria de PES). Etiquetas: ✅ evidencia propia · 🌐 fuente externa (comunidad, no oficial) · ❓ suposición.

## Respuesta corta

**Parcialmente / no recomendable tocarlo.** No hay forma segura y comprobada de pasar de 32 slots. Pero **no hace falta**: lo que Link necesita guardar por amigo (nombre, mando, ajustes propios, estadísticas) se puede guardar **fuera de PES**, en Link/web, sin límite. Recomendación: opción (c).

## 1. ¿Qué son los «datos personales»?

- ✅ `PES2021.exe` (Conmegol Patch) contiene la etiqueta `personal_data` dentro de los mensajes de red por jugador (junto a `pad_no`, `side`, `guest_num`, `guest_list`, `guest_name`, `guest_pad_no`, `CMD_USER_DATA_SYNC`) y un valor de enum `PERSONAL_DATA`. Es decir: son datos **por usuario que el juego sincroniza en línea** (anfitrión y cada invitado), no el option file.
- 🌐 Guías de la comunidad (FIFPlay, PESNewUpdate): en Ajustes de mando hay un «preset personal» con una lista de usuarios («user 1», …) donde cada uno guarda su configuración de mandos/cursor/ayuda de pase. Encaja con lo que ve Fralex.
- ❓ Mi hipótesis: «datos personales» = esa lista de usuarios/presets (nombre + ajustes por usuario), con tope 32. **No pude confirmar el nombre exacto del menú ni el 32**: ninguna fuente lo menciona (wiki de Rigged bloqueada por Cloudflare, no se esquivó).

## 2. ¿Dónde se guardan?

- ✅ En la PC de Fralex, `Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\2026\save` solo tiene `EDIT00000000` (10,9 MB). No hay otro archivo de usuarios ahí.
- ✅ El option file tiene (por nuestra base de conocimiento, `PhoenixSync/base-conocimiento/01-MOTOR-PES2021.md` §B) jugadores editados, equipos, plantillas, tácticas: **ninguna tabla de usuarios documentada**. ❓ No lo descifré de nuevo para buscar una; no es necesario para la conclusión.
- ✅ No existe `settings.dat` en la carpeta del juego Conmegol (solo `Settings.exe` y `Settings_b.dll`); `CPY.ini` usa `SavePath=%DEFAULT_CPY_DIR%` (en esta PC `%APPDATA%\CPY_SAVES\CPY\1259970` está vacío).
- ❓ Lo más probable es que los presets vivan en un archivo propio del juego/Steam (userdata o nube) o se creen al primer uso. **Sin captura o prueba en el PC de Fralex no se sabe cuál.**
- ❓ Dónde está el 32: no localizado. Podría ser constante del ejecutable, tamaño fijo de tabla del archivo o la interfaz. Averiguarlo exige desensamblar `PES2021.exe` (458 MB), fuera de alcance y de riesgo.

## 3. ¿Se puede ampliar?

| Opción | Viabilidad | Riesgo |
|---|---|---|
| (a) Parchear exe/formato para >32 | ❓ posible en teoría, sin comprobar | **Muy alto**: tabla fija → corrupción de guardados, desincronía online (el `personal_data` viaja por red: todos los jugadores tendrían que tener el mismo parche), puede romper con cada actualización del exe |
| (b) Rotar los 32 slots con Sider/Lua | ❓ requeriría escribir memoria/archivo de PES | Alto: escribir en datos del juego en vivo; sin API de Sider conocida para esto |
| (c) Perfiles propios de Phoenix por Parsec ID, guardados por Link/web | ✅ viable, ya hay base (Link sabe quién ocupa cada mando/puesto; ranking en web) | **Ninguno para PES**: no toca el juego |

## 4. Recomendación

1. **No ampliar el límite en PES** (regla «si algo se rompe, mejor no lo hagas»).
2. Si lo que importa son los ajustes por amigo: guardarlos en Link por Parsec ID (mapeo de mando, nombre, preferencias) → ya existe parte (mapeo por persona, ranking).
3. Los 32 slots de PES se pueden dejar para los 8 mandos habituales; el resto de amigos usa el perfil Phoenix.

## 5. Qué necesito de Fralex para cerrar

- Captura de la pantalla donde ve «32» y qué muestra cada usuario (nombre, mando, estadísticas…).
- Cuántos amigos distintos quiere cubrir y qué datos de ellos quiere conservar.

Con eso se decide si basta (c) o hace falta algo más.
