# Guía — Que todos vean los mismos fichajes con «Activar»

9 de octubre de 2026, 20:30 (Lima). Para FRALEX y los jugadores del grupo. Frente SYNC.
(Copia en Google Drive: «📗 Guía — Que todos vean los mismos fichajes con «Activar» (9 oct 2026, 20:30)».)

---

## 1. La idea en una frase
Cuando alguien del grupo ficha a un jugador, **todos deben verlo en su juego al pulsar «Activar»**, sin Editar y sin reiniciar.

## 2. Palabras que se usan en esta guía
- **Option file** (`EDIT00000000`): archivo en `Documentos\KONAMI\…\save\`. Guarda los equipos cuando la opción en vivo está **desactivada**.
- **Base** (`PlayerAssignment.bin`): archivo en la carpeta **Phoenix-DB** del parche. Guarda los equipos cuando la opción en vivo está **activada**.
- **Player.bin**: archivo de Phoenix-DB con las **stats** (velocidad, regate…). Se usa siempre.
- **«Activar»**: botón del juego **Partido → Datos Actual. en vivo → Activar**. Phoenix lo revivió (phoenix.lua v0.17).
- **Phoenix Sync**: programa que detecta, sube y recibe los fichajes del grupo.
- **Phoenix Link**: programa que coloca los archivos en el juego y avisa en pantalla.
- **Entrega**: paquete de archivos que Sync deja listo para que Link lo coloque.

## 3. Lo que descubrimos (probado en la PC de FRALEX, 2026-10-09)

| «Datos Actual. en vivo» | El juego toma los equipos de… | Las stats salen de… | Cómo se recarga |
|---|---|---|---|
| **Activado** | la **base** (`PlayerAssignment.bin`) | `Player.bin` | **Activar** |
| Desactivado | el **option file** | `Player.bin` | Editar → Cargar |

Pruebas:
1. Lamine al Real Madrid **solo en el option file** + Activar → **no apareció**.
2. Vinícius al Barça **solo en la base** + Activar → **apareció**. ✅
3. Mbappé al Barça en el option file + Editar → Cargar (opción desactivada) → **apareció**. ✅

**Conclusión:** para fichar con «Activar», cada fichaje debe estar en la **base** de cada PC.

## 4. Cómo viaja un fichaje (cuando todo esté listo)
1. **El jugador A ficha** desde Editar → su option file cambia.
2. **Sync de A** lo detecta y sube a la web un mensaje corto: «jugador X del equipo 1 al 2».
3. **Sync de A** lo copia también a **su propia base** → A lo ve con Activar. *(⏳ pendiente de aprobación)*
4. **Sync de B** baja el mensaje, hace **respaldo**, lo escribe en su option file **y en su base**. *(✅ hecho)*
5. **Link de B** coloca los archivos y avisa: «⚡ Datos nuevos de Phoenix… Activar». *(⏳ falta que Link acepte la base)*
6. **B pulsa Activar** → ve el fichaje. **A y B ven lo mismo.**

## 5. Estado de cada pieza

*(Actualizado el 10 oct 2026.)*

| Pieza | Estado |
|---|---|
| Sync: detectar, subir y recibir fichajes | ✅ Hecho; exe recompilado el 10 oct con los comandos sync-* (381/381 pruebas) |
| Sync: escribir fichajes recibidos en la base | ✅ Hecho; **encendido en la PC de FRALEX** (`sync-base si`, aprobado 10 oct: «quiero que sea automático cada fichaje») |
| Sync: escribir **mis propios** fichajes en **mi** base | ✅ Hecho (10 oct, aprobado por FRALEX); va con `sync-base si`, haya grupo o no |
| Sync: aviso `recargar.txt` al colocar Link una entrega con base | ✅ Hecho (10 oct): en cada `<SiderAddons>\content\phoenix` que exista. Con el modo AUTO-FICHAJES de Link el juego recarga solo |
| Link: aceptar `PlayerAssignment.bin` | ✅ Hecho en Link (10 oct) |
| Web: rutas /v1/sync (config, operaciones, aplicada) | ✅ Publicadas (responden 401 sin token) |
| Web: `GET /v1/sync/grupos` y grupo FRALEX + amigo en `automatico` | ⏳ PENDIENTE-WEB: `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` |
| phoenix.lua v0.18 (botón Activar, modo ACTIVAR) | ✅ Instalada y cargada en el juego de FRALEX (sider.log 11:59) |
| Prueba real entre dos PCs | ⏳ Pendiente |

## 6. Guía paso a paso

### Paso 1 · Empezar todos iguales (muy importante)
1. FRALEX copia de su PC:
   - `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<steamid>\save\EDIT00000000`
   - `…\Phoenix-DB\common\etc\pesdb\Player.bin`
   - `…\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin`
2. El amigo **guarda una copia de los suyos** y pone los de FRALEX en las mismas rutas (si hay dos carpetas Phoenix-DB, en las dos).
3. Los dos deben usar **el mismo parche** (hoy ConmeGOL Patch 26). Si es distinto, Sync marca los fichajes como «incompatible».

### Paso 2 · Instalar las piezas en cada PC
1. **Phoenix Link** actualizado (el que recoge entregas), vinculado a la cuenta Phoenix.
2. **PhoenixSync.exe**.
3. **phoenix.lua v0.17** en la carpeta del modo del parche, con su línea en `sider.ini`.

### Paso 3 · Configurar el juego
- Los dos con **«Datos Actual. en vivo» activado**.
- En el overlay (Espacio), Phoenix debe decir **«botón nativo EN EL SITIO ✓»**.

### Paso 4 · Configurar Sync (una vez por PC)
```
PhoenixSync sync-juego "C:\ruta\de\la\carpeta\del\juego"
PhoenixSync sync-grupo <id del grupo que da la web>
```
Cuando Link ya acepte la base (Paso 6):
```
PhoenixSync sync-base si
```

### Paso 5 · Jugar con Sync encendido
```
PhoenixSync sync-vigilar 180
```
(180 = minutos vigilando). Si se cierra la ventana, Sync deja de vigilar.

### Paso 6 · Lo que falta construir (en este orden)
1. **Chat LINK:** pegar `prompts/PROMPT-LINK-playerassignment.md` para que Link acepte `PlayerAssignment.bin`.
2. **Chat SYNC (con aprobación de FRALEX):** que mis propios fichajes también vayan a mi base.
3. **Chat WEB:** confirmar rutas publicadas, correr el SQL y crear el grupo con FRALEX y su amigo, en modo **automático**.

### Paso 7 · La prueba real
1. El amigo ficha a un jugador desde Editar y guarda.
2. En unos segundos a FRALEX le llega el aviso en el overlay.
3. FRALEX pulsa **Activar** → el jugador aparece en su equipo nuevo.
4. Al revés: FRALEX ficha y el amigo lo ve con Activar.
5. Choque: los dos fichan al mismo jugador a la vez → «conflicto», no se pisa nada.

## 7. Qué puede salir mal y qué hacer

| Problema | Causa probable | Qué hacer |
|---|---|---|
| El fichaje no aparece con Activar | Falta en la base (Link sin actualizar o `sync-base` apagado) | Revisar Paso 6.1 y `sync-base si` |
| Yo no veo mi propio fichaje | Falta el arreglo del Paso 6.2 | Mientras tanto, Editar → Cargar |
| Sale «incompatible» | Parches distintos | Usar el mismo parche (Paso 1) |
| Sale «conflicto» | Los dos movieron al mismo jugador | Gana el que llegó primero; se revisa a mano |
| «Link rechazó la entrega» | Archivo no permitido o tamaño distinto | Ver el historial de Link |
| Algo se rompió | — | `PhoenixSync sync-deshacer` o `sync-restaurar <respaldo>` |

## 8. Otra opción que funciona hoy, sin cambios
Los dos con la opción **desactivada** y cargando con **Editar → Cargar**. Manda el option file, que Sync ya sincroniza. Funciona, pero son más pasos.

## 9. Reglas
- Ningún cambio en Sync sin aprobación de FRALEX.
- Siempre hay respaldo antes de aplicar algo (`%APPDATA%\Phoenix Mercado\respaldos\`).
- Los archivos de la base se fabrican siempre a partir de los de olmos. Cuando olmos se actualiza, hay que volver a fabricarlos.
