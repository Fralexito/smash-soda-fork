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

---

## 6. Actualización (tras la captura de «Usuario 27» y la aclaración de Fralex)

**Objetivo real:** Fralex tiene más de 32 amigos, cada uno con su nombre y su configuración; quiere más de 32 «Datos personales» (Usuario N).

### Qué sabemos (✅ = visto en `PES2021.exe`, solo lectura)
- ✅ «Datos personales» = **PD** en el código del juego. Hay menús propios `…PDSelect` (elegir usuario, el L2/R2 de la captura), `…PDSettingsMenu` (ajustes del usuario) y `…PDKeyConfig` / `…PDKeyConfigTypeSelect` (**mapeo de botones del usuario**) en Exhibición, Competición, Liga, Partido, Online y Coop. Es decir: **cada Usuario guarda al menos mapeo de botones y ajustes**; el nombre se asume (se ve en la captura) pero no está probado dónde se guarda. Cámara, radar y estadísticas: ❓ sin confirmar.
- ✅ En línea el juego envía el `personal_data` de cada jugador a los demás (`CMD_USER_DATA_SYNC`, `guest_list`, `pad_no`): el dato viaja con la partida, **no hace falta que el invitado lo tenga guardado en tu PC**.
- ✅ El ejecutable solo nombra dos rutas de configuración bajo `Documents\KONAMI\eFootball PES 2021 SEASON UPDATE\`: `settings` y `settings.dat`. Más el sistema de guardado (`savesystem`, carpeta `save`).
- ✅ `settings.dat` mide 612 B = cabecera 0x64 + **8 casillas de 64 B** (GUID del mando + mapa de botones) (`ORDEN-MANDOS-PES.md`). **No cabe una tabla de 32 usuarios ahí.** Esa casilla es el mapa del *mando físico* de Settings.exe, otra cosa que los Usuarios.
- ✅ `EDIT00000000` (option file) no documenta tabla de usuarios; ❓ no se ha descifrado buscando «Usuario».

### Qué NO se pudo averiguar (y por qué)
- ❓ **En qué archivo viven los Usuarios 1…32**: en esta PC solo está una copia de `…\2026\save\EDIT00000000`; no están ni `settings.dat` ni los datos reales de Fralex. Candidatos: el archivo `settings` (sin extensión), un archivo del `save\` real de Fralex, Steam/CPY (`%APPDATA%\CPY_SAVES\CPY\1259970` está vacío aquí), o dentro del option file.
- ❓ **De dónde sale el 32** (constante del exe, tabla fija del archivo o interfaz): sin el archivo no se puede decir. Con 8 mandos y 32 usuarios la tabla casi seguro es un array fijo de 32 entradas.
- ❓ Si PES relee los Usuarios sin reiniciar. El `settings.dat` solo se lee al arrancar (probado); para los Usuarios no hay prueba.

### (A) ¿Subir el 32?
Sin localizar el archivo no hay base para afirmarlo. Si es array fijo en el formato **y** en el exe, ampliar = parchear el ejecutable de 458 MB y cambiar el formato, y como el `personal_data` viaja por red, todos los jugadores online tendrían que llevar el mismo parche. **No recomendado: riesgo alto de romper el guardado.**

### (B) Perfiles ilimitados gestionados por Phoenix (rotación de 32 huecos)
Idea sólida y sin tocar el exe: Link guarda en su base (por Parsec ID) el contenido de cada Usuario y lo vuelca a uno de los 32 huecos antes de que esa persona juegue; al terminar, lee de vuelta los cambios. **Condiciones:** (1) que el Usuario esté en un archivo legible/escribible (no memoria); (2) que se pueda escribir con PES cerrado (siempre debería poder, como ya hace Link con `settings.dat`, con copia previa); (3) en vivo solo si PES relee el archivo, ❓ sin probar. Lo que Link sí puede hacer **sin tocar PES**: su propia base de perfiles (nombre, mapeo propio por persona, orden de mandos) y rotar a nivel archivo con PES cerrado.
Sin (1) se queda en (c): guardar solo lo de Link.
Para elegir el Usuario correcto al sentarse alguien en vivo haría falta tocar la selección en memoria: **no recomendado sin prueba segura**.

### Dónde está en memoria/archivo «mando → columna» y «Usuario N de cada mando» (para la sesión que lee el lado)
❓ No localizado. Pistas verificadas: clases/menús `PDSelect`, `OnlineGuestUserPDSelect`, `MatchPDSettingSelectUser`, `ExhibitionUserDataDoalog`; el `pad_no` y `side` viajan juntos con `personal_data` en el mensaje `CMD_USER_DATA_SYNC`. Buen punto de partida para buscar estructuras con `pad_no`/`side`/índice de usuario (0–31) durante la pantalla de asignación.

### Prueba segura que se pide a Fralex (nada se escribe en PES)
1. Con PES cerrado: ejecutar en cmd `dir /s /a /tn "%USERPROFILE%\Documents\KONAMI\eFootball PES 2021 SEASON UPDATE" > antes.txt` (y lo mismo en `OneDrive\Documents` si existe), y copiar toda esa carpeta a otro sitio como respaldo.
2. Abrir PES, ir a Datos personales, editar un Usuario libre (p. ej. el 32): ponerle nombre **ZZTEST** y cambiar un botón reconocible (p. ej. pase ↔ disparo). Salir de PES por completo.
3. Repetir el `dir` (`despues.txt`) y pasar ambos: el archivo cuya hora/tamaño cambió es el de los Usuarios. Con una copia de él se busca «ZZTEST» y se determina formato, tamaño de entrada y si el 32 es un tamaño de tabla.

---

## 7. Contenido de un «Usuario» (capturas de Fralex, ✅ visto en pantalla)

Pantalla «Editar Datos personales» → «Elige un nombre para tus Datos personales»: cada Usuario guarda
- **Nombre** («Usuario 27», editable; es lo que Fralex usa para saber quién controla a cada jugador).
- **Configuración de comandos**: Normal / Alternativa / **Personalizado** (mapa completo de botones por contexto: Ataque 1/2, Defensa…, más «Desplazamiento de jugadores» L/R).
- Cambiar cursor, cursor de siguiente jugador, tipo de cursor, nv. de ayuda en pases, tipo de pase en profundidad, tipo de disparo, guía de dirección / objetivo / posicionamiento, amago automático, entrada automática, controles de compañero (y posiblemente más filas por debajo de la captura).
- ❓ Estadísticas por usuario: no aparecen en estas pantallas (no confirmado que existan).

Esto confirma que el Usuario = **perfil de ajustes + nombre**, no datos de partida. Sigue sin saberse en qué archivo está (ver §6): pendiente la prueba ZZTEST.

### Consecuencia práctica
- **El nombre** (lo más importante para Fralex) se puede resolver **fuera de PES**: Link ya sabe quién ocupa cada puesto; basta mostrar «puesto N = nombre del amigo» en la UI de Link/overlay/web. Eso no depende de los 32 ni toca el juego.
- **La configuración de botones/ayudas** de cada amigo es lo que sí vive en PES. Con >32 amigos, la vía segura es (B) rotación con PES cerrado (si la prueba ZZTEST confirma que está en un archivo), guardando los perfiles en la base de Link por Parsec ID.
