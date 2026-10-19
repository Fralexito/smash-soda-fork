# Prueba del modo online con un amigo (Phoenix Link + PES 2021 por Parsec)

> Actualizada al menú del plan UX (Link 0.3.34): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES. Mapa completo en `docs/GUIA-MENU-NUEVO.md`.

**Cómo funciona:** PES corre **solo en tu PC** (el anfitrión). Tu amigo **no instala PES ni nada de Phoenix**: solo Parsec. Ve tu pantalla y juega con su mando como si estuviera a tu lado.

---

## A) En tu PC (anfitrión) — antes de que se conecte

1. **Compilar Link** (si cambiaste algo o es la primera vez):
   - doble clic en `C:\dev\smash-soda-fork\_phoenix-link\COMPILAR_LINK.bat`;
   - tarda de 1 a 10 minutos y **no borra nada**;
   - al final dice `LISTO`.
   - **Nunca** uses `COMPILAR_PHOENIX.bat`: borra lo que no está guardado en git.
2. **Abrir** `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\PhoenixLink.exe`. Si Windows pregunta por el firewall, pulsa **Permitir**.
3. **Cuenta web:** si Link te enseña la bienvenida, sigue sus 3 pasos. Si no: SYNC › MI JUEGO › «Tu cuenta de la liga» › escribe el código de 6 cifras de tu perfil y pulsa **CONECTAR** (si no lo hiciste ya).
4. **(Opcional) Archivos de Phoenix en el juego:** SYNC › MI JUEGO › **ARREGLAR TODO**. Si PES está abierto, Link espera a que lo cierres y sigue solo.
5. **Abrir PES 2021** como siempre (con el switcher de ConmeGOL).
6. **Abrir la sala:** en **INICIO** pulsa **JUGAR CON UN AMIGO**: abre la sala, copia el enlace y te deja invitar a tus amigos de la web. Pega el enlace en WhatsApp o Discord. Si algo falta (por ejemplo ViGEmBus), te lo dice con el botón que lo arregla.

## B) En la PC de tu amigo (5 minutos)

1. Descargar **Parsec** desde https://parsec.app y crear una cuenta gratis.
2. Conectar su mando (Xbox, PlayStation o genérico) **antes** de entrar.
3. Abrir el enlace que le mandaste y esperar.
4. Nada más: no necesita PES, ni Phoenix Link, ni el parche.

## C) Cuando entre

1. Tu amigo pulsa un botón de su mando y **se sienta solo** en el primer puesto libre (sentado automático; se cambia en Mandos › Puestos o Ajustes › General). Si lo apagaste, verás su solicitud: ACEPTAR, o arrástralo a un puesto en Mandos › Puestos.
2. Con los lados automáticos no hay que tocar nada: el partido arranca solo al pitazo. A mano: Partido › Marcador › elige los dos lados › **¡EMPEZAR!**. Al terminar sale el resultado con **REVANCHA**.
3. En PES entra a Partido › Amistoso. Cada uno elige equipo con su mando.

## D) Qué probar (marca cada uno)

- [ ] **Conexión:** su ping en la portada de la sala (verde < 60 ms, naranja < 100, rojo más).
- [ ] **Su mando mueve su jugador** y la imagen va fluida.
- [ ] **Modo competitivo:** durante el partido tu amigo pulsa **Start**. **No** debe pausar. En Partido › Marcador debe salir «Start/Back/Guía bloqueados».
- [ ] **Marcador:**
  - con el módulo instalado, el marcador de Link sigue al del juego y cada gol sale en el chat con su minuto;
  - sin el módulo, lo llevas a mano con +GOL.
- [ ] **Chat:** escríbele desde Link y que te responda desde Parsec.
- [ ] **Final:** al terminar, el partido aparece en Partido › Historial.
- [ ] **(Opcional) Pausa automática:** actívala en Mandos › Bloqueo › Árbitro y pídele que se desconecte a propósito. Debe salir el aviso en el chat y pausarse el marcador (y el juego si el módulo está instalado).
- [ ] **(Opcional) Overlay PhoenixGlass:** Ajustes › Overlay › abrir. Muestra el ping, el chat y los mandos encima del juego (para transmitir).

## E) Si algo falla, mándame esto

- Qué estabas haciendo y qué viste (una captura ayuda mucho).
- Si falló el juego o un módulo: `D:\Frank\Games_\Conmegol Patch\SiderAddons\sider.log`.
- Si falló la conexión: el ping que marcaba y si tu internet es de fibra o de antena.

## Requisitos de red (importantes)

- **Tu subida:** ≥ 10 Mbps, mejor por **cable**. Link ajusta la calidad solo (ancho de banda automático).
- **CGNAT:** algunas compañías en Perú comparten tu IP con otros clientes y Parsec no logra conectar directo. Si tu amigo no puede entrar nunca, ese es el sospechoso: hay que pedir a tu compañía una IP pública.
- Cierra descargas y Steam en segundo plano durante la prueba.
