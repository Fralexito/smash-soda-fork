# Phoenix Link · beta 2 — Guía para instalar y probar (una página)

> Actualizada al menú del plan UX (Link 0.3.34): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES. Mapa completo en `docs/GUIA-MENU-NUEVO.md`.

Para quien va a **hostear** con su propia PC (PES 2021 + parche + Parsec). Si solo entras a la sala de otro por Parsec, no necesitas nada de esto.

## Antes de empezar
- Windows 10 u 11 de 64 bits e internet. Hacen falta **menos de 1 GB** libres.
- PES 2021 con su parche **funcionando** (ábrelo una vez con su lanzador, como siempre) y una cuenta de Parsec.
- Los archivos que te pasa Fralex: `PhoenixLink-instalar-rapido.bat` y `PhoenixJuego-instalar.bat`.
- Tiempo: unos **3 minutos**.

## Paso 1 · Instalar Link
1. Doble clic en `PhoenixLink-instalar-rapido.bat`. Si Windows pide permiso, pulsa **Sí**.
2. Pulsa una tecla y elige dónde instalar (o cierra la ventanita para usar `C:\Program Files`).
3. **Espera.** Baja Link (17 MB) y solo lo que le falte a tu PC: runtime de Visual C++, ViGEmBus y WebView2. Si Windows pide reiniciar, reinicia y repite.
4. Termina con **INSTALACION TERMINADA**. Acepta el acceso directo del escritorio.

Si sale **NO SE PUDO TERMINAR**: tu juego no se tocó. Manda a Fralex una foto de la ventana o el archivo `PhoenixLink-instalador.log` (en `%TEMP%`: pega eso en la barra del Explorador).

## Paso 2 · Abrir Link y poner Phoenix en tu juego
1. **Cierra PES 2021 por completo** (y Sider si queda abierto).
2. Abre **Phoenix Link** e inicia sesión en Parsec cuando lo pida.
3. Ve a **SYNC → Módulos del juego**. Si no conoce tu juego, pega la carpeta donde está `PES2021.exe` y pulsa **GUARDAR**.
4. Pulsa **INSTALAR** en `phoenix_referee.lua` y en `phoenix.lua`. Deben quedar en **INSTALADO** (Link guarda copia de `sider.ini` con fecha).
5. Abre PES con su lanzador. En **SYNC**, la tarjeta «Qué funciona en tu juego» debe quedar en verde.

Sin Link (solo los módulos): doble clic en `PhoenixJuego-instalar.bat`, elige la carpeta del juego y espera **LISTO**.

## Qué trae esta beta
| Función | Dónde | Estado |
|---|---|---|
| Sala, mandos, puestos, equipos A/B | SALA, MANDOS | Lo usa Fralex; con otra persona, esta sesión lo comprueba |
| **JUGAR**: salas abiertas, amigos, tu perfil de PES | JUGAR | Nuevo. Perfil probado en el PES de Fralex; en tu PC, a comprobar |
| Lados automáticos (Link ve de qué lado juega cada mando) | PARTIDO | Nuevo, activado |
| Aviso cuando alguien entra a la sala | Ajustes › Sonidos | Nuevo, activado |
| Campo 2D en vivo | PARTIDO / web | Nuevo, **apagado**: se enciende en Partido |
| Marcador, goles, HUD y árbitro | PARTIDO | Listo, a probar contigo |
| Avisos de la web dentro del juego | SYNC | Listo |
| Fichajes de Phoenix | SYNC | Se aplican sobre **tu** base; la lista va vacía: no cambia nada |
| **Traspasos en vivo** | SYNC | **Apagado por defecto** («¿Qué eres tú?» = «Apagado»). Ya funciona con la web; enciéndelo solo si Fralex te lo pide |

## Quitar Phoenix
- Del juego: Link → SYNC → Módulos del juego → **DESINSTALAR**. Sin Link: ejecuta `PhoenixJuego-instalar.bat desinstalar` desde una consola. No borra nada: aparta los `.lua` y guarda copias de `sider.ini`.
- De Link: borra la carpeta `Phoenix Link` y el acceso directo. Tus ajustes quedan en `%APPDATA%\Trybuchet\Smash Soda`.

---

# Lista para la PRIMERA sesión con el amigo

**Antes de abrir sala (5 min)**
- [ ] Link abre y dice **BETA 2** abajo a la izquierda.
- [ ] INICIO (o Sala › Conexión) › Semáforo Phoenix: «Todo listo para jugar». Lo que falte sale con su botón para arreglarlo.
- [ ] SYNC › «Qué funciona en tu juego»: módulos al día, sin ámbar ni rojo.
- [ ] Abre PES: en `SiderAddons\sider.log` aparece `[phoenix.lua]` sin «is NOT activated».

**JUGAR y perfiles**
- [ ] JUGAR › Salas abiertas: carga (aunque no haya salas) y no da error.
- [ ] JUGAR › Perfil PES: pones tu nombre y opciones; al sentarte en la sala, PES los muestra en tu mando y los devuelve al levantarte.

**Sala y partido**
- [ ] Entra el amigo: suena y sale el aviso de entrada (sonido + notificación).
- [ ] Equipos A/B: cada uno en su equipo y su puesto; probad a cambiar de equipo arrastrando.
- [ ] Lados automáticos: tras el pitazo, Link pone a cada uno en su lado; si está al revés, se corrige solo.
- [ ] El marcador y los goles aparecen en Link y en el HUD del juego.
- [ ] Campo 2D: enciéndelo en Partido y mira que las fichas se mueven (puede tardar ~20 s en «BUSCANDO»).
- [ ] Un aviso de la web sale dentro del juego (barra espaciadora → panel de Sider).
- [ ] Mira si hay retraso en los mandos (medidor de Link).

**Si algo falla, anota**
1. Qué hacías y qué pasó (una frase) y la hora.
2. Una foto de la pantalla de Link.
3. El informe: **Sala › Conexión › Detalles técnicos del PC › COPIAR INFORME**, y pégalo en el chat. Trae versiones, parche, módulos y el final de `sider.log` con una sección «Qué pasa y qué hacer». Tapa tu nombre de usuario de Windows y no lleva contraseñas ni claves.

**Reglas de oro:** no abras PES mientras instalas módulos; no enciendas «Traspasos en vivo» salvo que Fralex te lo pida; si dudas, no cambies nada y manda el informe.
