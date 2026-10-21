# Modo cedido: prestar tu PC a un amigo (solo Link y PES)

**Para qué sirve:** cuando no estás, o no puedes manejar la PC, un amigo conectado por Parsec maneja **Phoenix Link y PES 2021 con teclado y ratón**. No puede usar nada más de Windows. Tú puedes seguir usando tu teclado y tu ratón a la vez.

**Estado (11 oct 2026):** compilado y probado en el simulador y con pruebas automáticas. **Falta probarlo con un amigo real** (paso 6).

---

## 1. Antes de empezar (una sola vez)

1. En **Sala › Juegos**, elige el juego de la biblioteca que abre PES (el mismo que usarías en modo quiosco). Ese es el que abre el botón **ABRIR PES**.
   - Si usas Sider, pon en la biblioteca el lanzador que usas siempre.
   - Sin juego elegido, ABRIR PES solo puede traer PES al frente si ya está abierto.
2. Abre la sala como siempre.

## 2. Ceder la PC

1. Entra en **Gente › En sala** y toca a tu amigo. También puedes tocarlo en la lista de Sala. Puede estar **jugando o mirando**: da igual.
2. Pulsa **CEDER MI PC** y confirma. En el chat de la sala sale: «Fralex cedió su PC a …». Link le da teclado y ratón a tu amigo también en Parsec (desde 0.2.96; antes su Parsec no los enviaba).

No hay PIN. Devolver la PC siempre le quita el control a tu amigo, así que no hace falta protegerlo.

## 3. Qué ve y qué puede hacer tu amigo

- Arriba sale la banda **PC CEDIDA A …**, con un botón grande **ABRIR PES** (o **IR A PES** si ya está abierto) y **DEVOLVER LA PC**.
- Menú: solo **Sala** (Resumen, Conexión, Actividad), **Partido** y **Mandos** (Puestos y Bloqueo).
- Puede repartir mandos, aceptar a quien quiere jugar, llevar el partido y escribir en el chat de la sala.
- **No puede:** Ajustes, Sync, tu cuenta de la web, el chat General de la web, banear, expulsar, cambiar permisos, cerrar la sala, cerrar Link (ni con la X ni con Alt+F4), ni abrir el navegador.
- Los demás invitados **pierden teclado y ratón** mientras dure (siguen con su mando).

**En Windows, el guardián de Link:**
- Solo deja pasar los clics que caen sobre PES o Link. La barra de tareas, el escritorio y otras ventanas no reciben clics.
- La tecla Windows **sola** (pulsar y soltar) minimiza PES y muestra Link; nunca abre el menú Inicio (desde 0.2.96). Sus atajos (Win+D, Win+R…) siguen bloqueados. También bloquea Alt+Tab, Alt+Esc, Alt+Espacio, Ctrl+Esc, Ctrl+Shift+Esc, Impr Pant, Shift+Tab (overlay de Steam), las teclas que abren programas y los atajos del navegador interno de Link (Ctrl+P, Ctrl+N, F12…).
- Alt+F4 sí cierra PES (se vuelve a abrir con ABRIR PES), pero nunca Link.
- **Link no se puede minimizar, cerrar, mover ni cambiar de tamaño** desde la entrada de tu amigo (botones de la ventana, Alt+F4, Alt+Espacio). Si Link se minimiza por otra vía, vuelve a mostrarse solo. Tú sí puedes hacerlo con tu ratón o teclado físico (desde 0.2.93).
- Si otra ventana se pone delante, la devuelve a PES (o a Link) en menos de un segundo. Esto no pasa si tú estás usando tu teclado o ratón (espera 15 s sin que los toques).

**Tú sigues mandando:** tu teclado y tu ratón físicos nunca se bloquean, así que puedes usar la PC a la vez que tu amigo. Cuando tocas los tuyos, los de él se pausan un instante para que no os peleéis por el cursor. Link se ve limitado también para ti; para tenerlo completo, devuelve la PC (un clic).

## 4. Recuperar la PC

Cualquiera de estas cuatro:

1. **DEVOLVER LA PC** en la banda de arriba, y confirmar. Lo puedes pulsar tú o tu amigo; si lo pulsa él, solo se queda sin control.
2. **Ctrl+Alt+F12** en el teclado de tu PC. Solo funciona con el teclado físico: si tu amigo lo pulsa por Parsec, no pasa nada.
3. **Sola:** si tu amigo sale de la sala, si se cierra la sala, si la interfaz de Link deja de verse más de 10 s o si el guardián falla.
4. Si Link se cierra o se cuelga, tu amigo pierde el control al momento (Link es quien mete su teclado y su ratón).

Al recuperarla, tu amigo **pierde teclado y ratón** y en el chat sale «La PC vuelve al anfitrión (motivo)». Para dárselos otra vez, usa los permisos de siempre o cede de nuevo.

## 5. Cómo funciona por dentro (resumen)

- Parsec no mete la entrada en Windows por su cuenta. La recibe **Link** (`Hosting::pollInputs`) y la inyecta con `SendInput` (`InputControlService`). El filtro trabaja **antes de inyectar**, así que lo bloqueado nunca llega a Windows.
- Por eso no hace falta un gancho que mire `LLKHF_INJECTED` para bloquear. Ese indicador sirve para lo contrario: todo lo que inyecta Link llega marcado como inyectado, así que el atajo Ctrl+Alt+F12 solo se acepta si llega sin la marca, es decir, del teclado físico.
- Las piezas:
  - `phoenix/core/ModoCedido.*`: lógica pura (teclas, clics, lista blanca de acciones), probada en `pruebas-link/cedido_test.cpp`.
  - `phoenix/host/Cedido.*`: la parte de Windows (qué ventana es PES o Link, el guardián del foco, ABRIR PES, el atajo).
  - `phoenix/web/AccionesCedidoWeb.cpp`: las acciones `cedido.*` y el portero del puente, que devuelve `PC_CEDIDA` a todo lo que no sea de jugar.
- Cuenta como **PES**: `PES2021.exe`, el ejecutable del juego de la biblioteca y todo lo que esté dentro de la carpeta del juego (Sider, Settings…).
- Cuenta como **Link**: Link, sus procesos de WebView2 y el overlay PhoenixGlass.
- Para permitir otro programa, añádelo a mano en `phoenix-ui.json` → `"cedidoExtras": "ds4windows.exe"` (separados por comas, con Link cerrado).

## 6. Prueba con un amigo (pendiente)

1. Abre la sala. Tu amigo entra por Parsec.
2. Cédele la PC (paso 2) y quédate mirando sin tocar nada.
3. Pídele que pruebe:
   - ABRIR PES abre el juego.
   - Puede jugar y moverse por los menús de PES con teclado y ratón.
   - Puede volver a Link, repartir mandos y llevar el marcador.
   - La tecla Windows, Alt+Tab, clic en la barra de tareas y Ctrl+Shift+Esc no hacen nada.
   - Ajustes, Sync y Gente no aparecen.
4. Mueve tu ratón y escribe algo: tienes que poder usar la PC a la vez.
5. Pulsa Ctrl+Alt+F12: la PC vuelve a ti y él pierde teclado y ratón.
6. Repite con DEVOLVER LA PC, y luego haz que tu amigo salga de la sala: tiene que volver solo.
7. Prueba también a cederla a alguien que esté mirando (espectador).

Si algo falla, el modo se apaga solo y la PC vuelve a ti. Anota qué pasó y avisa en el chat LINK.

## 7. Límites honestos (lo que no cubre)

- **Dentro de PES y de Link** todo vale. Si un juego abriera un diálogo de Windows (abrir o guardar archivo), ese diálogo es de PES y se podría usar para ver carpetas. PES 2021 no tiene diálogos así, pero un lanzador o Sider con menús propios sí podrían.
- **Overlays dentro del juego:** el de Steam se abre con Shift+Tab, que está bloqueado. Si lo cambiaste a otra tecla, apágalo en Steam. El de NVIDIA (Alt+Z) es otra ventana: sus clics se bloquean, pero puede llegar a verse.
- **Atajos globales de otros programas** (Discord, OBS, NVIDIA…) pueden saltar con teclas normales que sí dejamos pasar, como Alt+Z o Ctrl+Shift+M. Si usas alguno, ciérralo antes de ceder o cambia su atajo.
- **Ctrl+Alt+Supr** no se puede mandar por Parsec (Windows no deja), así que tampoco hay que bloquearlo.
- **Ventanas de administrador** (por ejemplo, UAC): Link no puede escribir en ellas y el escritorio seguro tampoco recibe nada. Si aparece un aviso de UAC, la PC queda en pausa hasta que tú lo cierres.
- **Si no estás en la PC:** vuelve cuando tu amigo pulsa DEVOLVER LA PC o sale de la sala.
- **Jugar con teclado como mando:** mientras tu amigo tiene la PC, su teclado va a Windows (PES y Link) y deja de hacer de mando. Con su mando de verdad juega igual.
- **El chat de la sala:** lo que tu amigo escribe sale con tu nombre de anfitrión.
- **El modo quiosco original no abre el juego al hostear** (esa línea viene comentada en Smash Soda). Por eso ABRIR PES abre el juego de la biblioteca por su cuenta.
- **Pegar (Ctrl+V) en Link** está permitido: pega lo que haya en tu portapapeles.
