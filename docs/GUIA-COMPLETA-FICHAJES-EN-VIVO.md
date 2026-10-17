# Guía completa de Phoenix Link: instalar y pasar un fichaje de un PES a otro (versión 0.3.12)

Probado de verdad el 13 de octubre de 2026: un anfitrión hizo fichajes en Editar de PES 2021, los guardó, Link los publicó y el PES de su amigo los recibió. Esta guía junta todo el recorrido, paso a paso, y los errores que pueden salir con la forma de evitarlos.

## 1. Qué vas a conseguir
- Tú (el **anfitrión**) haces un fichaje en **Editar** de PES y lo guardas.
- Tu Phoenix Link lo ve, lo convierte en un aviso diminuto («jugador X pasa del equipo A al B») y lo sube a la web de Phoenix.
- El Phoenix Link de tu **amigo** lo recoge solo y lo aplica en **su** base de datos. Cuando él cierra y abre PES, el fichaje está.
- Solo viajan nombres e identificadores de jugador y de equipo. No se copia ninguna base de datos ni ningún dato personal.

## 2. Qué necesitas antes de empezar
Los dos:
- Windows de 64 bits e internet.
- PES 2021 con Sider y un parche instalado (ConmeGOL u otro).
- Una cuenta de la liga en la web de Phoenix.
- El mismo número de versión de Phoenix Link (esta guía es para la 0.2.82).

Solo el anfitrión:
- Su cuenta debe estar **aprobada como host** en la web. Si no lo está, no obtiene código de traspasos.

Solo el amigo:
- El código de 8 letras y números del anfitrión.

No hace falta instalar nada a mano: el instalador pone lo que falte (Visual C++, ViGEmBus para los mandos virtuales y WebView2 para la interfaz).

## 3. Instalar Phoenix Link (los dos)
1. Pídele a quien te lo pasó el archivo `PhoenixLink-Setup-0.2.82.exe` (unos 13 MB). Guárdalo donde quieras.
2. Ábrelo con doble clic. Windows puede mostrar «Windows protegió su PC» (el instalador no está firmado): pulsa **Más información** y luego **Ejecutar de todas formas**.
3. Acepta el permiso de administrador (ventana de Windows con «Sí»).
4. Se abre el asistente:
   - **Bienvenida:** pulsa **Siguiente**.
   - **¿Dónde instalar?:** déjalo en `C:\Program Files\Phoenix Link` salvo que quieras otra carpeta. Deja marcado el acceso directo del escritorio. Pulsa **Instalar**.
   - **Instalando:** una barra de progreso. Si falta algo de Microsoft o ViGEmBus, lo descarga (hace falta internet) y lo instala solo. Tarda de 1 a 5 minutos.
   - **Final:** pulsa **Finalizar** (puedes dejar marcado «Abrir Phoenix Link ahora»).
5. Si ya tenías Phoenix Link, el instalador lo **actualiza** y no toca tus datos ni tu configuración.

## 4. Primer arranque: dejar todo listo (los dos)
Sync tiene **3 pestañas**, siempre en este orden: **MI PC** (¿está listo mi PC?), **TRASPASOS** (pasar un fichaje a un amigo) y **AVANZADO** (opciones técnicas, solo si algo falla).

Hazlo con **PES 2021 cerrado**.
1. Abre Phoenix Link y entra en **SYNC › MI PC**.
2. Arriba verás un mensaje: **«Todo listo»** (verde) o **«Falta algo: N cosas»** (ámbar). Debajo, una lista con ✓ (bien) y ✗ (falta algo). Cada ✗ trae una frase corta y un botón.
3. Si **Tu cuenta de la liga** está en ✗: en la web de la liga entra a *Mi perfil › vincular PC*, copia los 6 dígitos, escríbelos en esa fila y pulsa **UNIR**. Es lo único que haces tú.
4. Pulsa **ARREGLAR TODO**. Hace en orden, solo lo que es seguro:
   - busca tu juego (solo lee nombres de carpetas; si hay varios, te pregunta cuál);
   - instala los archivos de Phoenix en el juego (con copia previa de lo que cambia);
   - deja que el juego lea los fichajes (repone una línea de configuración, con copia);
   - deja la recarga automática activada.
5. Cuando arriba diga **«Todo listo»**, sigue. Si algo se queda en ✗, mira la sección 9.

## 5. Tu código (para enviar tus fichajes)
Desde la 0.3.12 **no hay que elegir papel**: en **SYNC › TRASPASOS** enviar y recibir van juntos y se encienden solos.
1. En el bloque **ENVIAR MIS FICHAJES** sale **tu código en grande** (8 letras y números). Pulsa **COPIAR CÓDIGO** y mándaselo a tu amigo. **CAMBIAR CÓDIGO** pone uno nuevo (el viejo deja de servir para unirse; quien ya lo recibe sigue).
2. Si no sale el código, la pantalla dice por qué: cuenta sin unir (botón **UNIR MI CUENTA**), cuenta no aprobada como anfitrión (puedes recibir igual; para enviar, pídeselo al staff) o la web sin conexión (**VOLVER A COMPROBAR**).
3. Debajo hay **«¿Algo no funciona?»** (se abre sola si hay un ✗) y **«Más opciones»** (modo prueba, carpeta compartida, apagar).

## 6. Recibir los fichajes de un amigo
1. En el bloque **RECIBIR FICHAJES DE UN AMIGO**, pega el código de tu amigo y pulsa **RECIBIR**. Verás su nombre con **RECIBIENDO** y el botón **DEJAR DE RECIBIR**.
2. Puedes recibir de varios amigos («Recibir también de otro amigo») y a la vez enviar los tuyos.
3. Link consulta la web cada unos 8 segundos. Si tu amigo ya envió fichajes antes, **los recibirás al pegar su código** (la web los guarda 30 días, hasta 200 por persona).

## 7. Hacer el fichaje (anfitrión)
1. Abre PES 2021 y entra a **Editar**.
2. Haz **un solo** traspaso para empezar (un jugador pasa de un equipo a otro).
3. **Guarda y sal de Editar.** PES solo escribe el archivo al guardar; antes de eso Link no puede ver nada.
4. Espera unos segundos. Link lee el archivo cada ~3 s y confirma cuando PES terminó de escribir. En **Traspasos en vivo** aparece el traspaso detectado.
5. Pulsa **PUBLICAR** (por defecto Link pregunta antes de enviar).
6. En el Link del amigo debe salir **«Fichaje aplicado: X pasa de A a B»** con lo que hacer.

## 8. Ver el fichaje en el PES del amigo
1. Con la recarga automática, el juego recarga solo al volver al menú y entrar a un modo (nunca en medio de un partido ni en Editar).
2. Si no quieres esperar: que el amigo **cierre PES y lo vuelva a abrir**, o use el botón de activar/recargar que indica el aviso.
3. Que busque al jugador en su equipo nuevo.
4. Si algo salió mal, el amigo puede pulsar **DESHACER el último lote** y la base vuelve a como estaba.

Reglas del traspaso (para entender qué detecta Link):
- Cuenta como traspaso un jugador que **sale de un equipo y entra en otro**.
- Altas, bajas, convocatorias y dorsales se ignoran.
- Más de 60 cambios de golpe se consideran «cambio masivo» (por ejemplo, cambiar de modo del parche) y no se publican solos.
- La primera vez Link solo guarda la «foto de partida» y no publica nada.

## 9. Errores posibles y cómo evitarlos

### Al instalar
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «Windows protegió su PC» | El instalador no está firmado digitalmente | Más información › Ejecutar de todas formas |
| «Phoenix Link está abierto» | El instalador no puede reemplazar archivos en uso | Cierra Phoenix Link y el overlay (también en la bandeja) y pulsa Reintentar |
| «No se pudo bajar…» (Visual C++, ViGEmBus, WebView2) | Sin internet, o un antivirus o cortafuegos lo bloquea | Revisa el internet y vuelve a ejecutar; el instalador retoma lo que falte |
| Error de permisos en la carpeta | Instalaste en una carpeta protegida sin ser administrador | Ejecuta el instalador con administrador o elige otra carpeta |
| El antivirus lo bloquea | Programa nuevo sin firma | Permite el instalador y la carpeta de Phoenix Link en tu antivirus |

### Al abrir Link
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «No se pudo crear la vista web (0x80080005)» | Quedó un proceso del motor web de una sesión anterior que se cerró mal | Desde la 0.2.74 Link lo limpia solo al reabrir; usa **Reintentar**. Si sigue, reinicia el PC |
| Se abre la interfaz de siempre y no la nueva | WebView2 no está instalado o falló | Reinstala con el instalador (instala WebView2) |
| Link se cierra solo | Fallo interno (hubo varios antes de la 0.2.78) | Manda el archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-fallos.log`; Link anota ahí la causa y las funciones implicadas |
| El overlay no aparece | La sala está cerrada, o PES va en pantalla completa «de verdad» | Abre la sala, pon PES en ventana o sin bordes. Panel de amigos: **Ctrl+Alt+A** |

### En Sync › MI PC
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| ✗ carpeta del juego | Link no encontró PES o hay varios | Elige la carpeta en la lista (por ejemplo la de tu parche) |
| ARREGLAR TODO se detiene: «cierra PES» | Escribir en el juego con PES abierto es peligroso | Cierra PES y pulsa de nuevo |
| ✗ archivos de Phoenix o «perdido» | Actualizaste el parche pegando carpetas encima y se borraron las líneas de Phoenix en `sider.ini` | Pulsa **ARREGLAR TODO**; repone las líneas |
| ✗ «Que el juego lea los traspasos» | El `sider.ini` no tiene la línea que lee la carpeta de fichajes (pasa si pegaste una actualización del parche encima) | Con PES cerrado, pulsa **ARREGLAR** o **ARREGLAR TODO**; Link hace copia antes |
| Faltan archivos del módulo | Copia incompleta | **ARREGLAR TODO** de nuevo; Link guarda copia de `sider.ini` antes de tocarlo |

### En Sync › Traspasos (anfitrión)
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| ✗ «sin permiso» / «aprobada como anfitrión» | Tu cuenta no está aprobada como host | Pídele al staff que apruebe tu cuenta como host en la web |
| «La web no reconoce este Link» | Cuenta no vinculada o token vencido | **Sync › Mi PC › Tu cuenta de la liga**: vuelve a unirla con un código nuevo (o mira el detalle en Avanzado › Cuenta web detallada) |
| «Sin conexión con la web [motivo]» | Internet, cortafuegos, antivirus o la web caída. El motivo técnico va entre corchetes | Link reintenta solo (10, 20, 40, 80 y 120 s). Si persiste, manda una captura del Paso 2 de Traspasos y de Avanzado › Detalles técnicos |
| «La web aún no tiene los traspasos en vivo» | La función de la web no estaba disponible | Link prueba de nuevo cada 5 minutos; botón **VOLVER A COMPROBAR** |
| La liga está en pausa | El staff apagó los resultados/traspasos | Espera a que la reactiven |
| Guardas en Editar y no sale nada | No saliste de Editar o no guardaste; la carpeta vigilada no es la tuya (hay varias carpetas de guardado) | Guarda y sal de Editar; mira la ruta del archivo en la lista y corrígela si no es la tuya |
| «Cambio masivo, pendiente» | Más de 60 traspasos de golpe | Descártalo; no se publica solo |
| No detecta nada con Liga Máster | Link solo mira el archivo de edición (`EDIT…`), no el de Liga Máster (`ML…`) | De momento no se soporta |

### En Traspasos en vivo (amigo)
| Qué ves | Por qué pasa | Qué hacer |
|---|---|---|
| «Código incorrecto» (CODIGO_INVALIDO) | Se escribió mal, o el anfitrión lo cambió | Pide el código actual; cópialo en vez de escribirlo |
| No puedes seguirte a ti mismo | Usaste tu propio código | Solo en modo **Prueba** |
| «Demasiados intentos» | Más de 10 intentos de seguir en un minuto | Espera un minuto |
| El lote se «salta» con un motivo | Su base es distinta a la del anfitrión y no hay coincidencia exacta de nombres | Lee el motivo; con parches distintos Link aplica por nombres de jugador y equipo, y si hay duda no toca nada |
| «Esperando a cerrar PES» | Con PES abierto no se toca nada salvo la base de Phoenix; si falta la línea de Phoenix-DB o hay un `Player.bin` ajeno, espera | Cierra PES; se aplica solo |
| Aplicado pero no se ve en el juego | El juego aún no recargó | Cierra y abre PES, o espera la recarga automática. Link no sabe si el juego ya recargó |
| Amigo y anfitrión con versiones distintas | Cambios entre versiones | Los dos con la misma versión |
| El amigo también edita | Hay conflicto entre su edición y la del anfitrión | Con «datos en vivo» activado manda la base de Phoenix para los jugadores del lote; el resto de su Editar no se toca |

### Desinstalar
- **Configuración › Aplicaciones › Phoenix Link › Desinstalar**, o `Desinstalar.exe` dentro de la carpeta.
- Casilla opcional: **borrar también mis datos y ajustes** (cuenta vinculada, perfiles de amigos, configuración).
- No toca la carpeta de PES. Si instalaste el módulo del juego y quieres quitarlo, hazlo **antes** desde Link: SYNC › Avanzado › Archivos de Phoenix en el juego.

## 10. Qué está probado y qué no
**Probado de verdad (13 oct 2026):**
- Instalar con el `.exe` en un PC distinto.
- Fichaje en Editar, guardado, detección, publicación, llegada al amigo y aplicación en su PES.
- Conexión con la web: 166 peticiones, 0 fallos, máximo 2,7 s.
- Detección de requisitos y copia real de archivos del instalador, y pantallas del asistente.

**Sin probar todavía en el juego real:**
- Desinstalar de verdad en un PC instalado.
- Overlay encima de PES y panel de amigos con una invitación real.
- Cambios del HUD de Sider: lado según el puesto, ping en vivo y goles en columnas.
- Lados automáticos A/B y campo 2D en vivo.
- ARREGLAR TODO con varios candidatos de juego.

## 11. Si algo falla, manda esto
1. Una captura de **SYNC › MI PC** y de **SYNC › TRASPASOS** (los 3 pasos y «¿Algo no funciona?» abierto). Si hay un error de la web, también **Avanzado › Detalles técnicos**.
2. El archivo `%TEMP%\PhoenixLink-instalador.log` (si falló al instalar).
3. El archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-fallos.log` (si Link se cerró).
4. El archivo `%APPDATA%\Trybuchet\Smash Soda\phoenix-traspasos\estado-actual.json` (estado de los traspasos; no lleva contraseñas).

## 12. Glosario
- **Anfitrión:** quien hace los fichajes y los publica.
- **Amigo:** quien sigue al anfitrión y los recibe.
- **Lote:** el aviso de uno o varios traspasos.
- **Foto de partida:** copia de cómo estaban las plantillas, para saber qué cambió.
- **Phoenix-DB:** la base con los fichajes que Link deja en el juego del amigo.
- **Sider:** el programa que carga los módulos de Phoenix dentro de PES.
