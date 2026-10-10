# Prueba del modo online con un amigo (Phoenix Link + PES 2021 por Parsec)

**Cómo funciona:** PES corre **solo en tu PC** (el anfitrión). Tu amigo **no instala PES ni nada de Phoenix**: solo Parsec. Ve tu pantalla y juega con su mando como si estuviera a tu lado.

---

## A) En tu PC (anfitrión) — antes de que se conecte

1. **Compilar Link** (si cambiaste algo o es la primera vez):
   - doble clic en `C:\dev\smash-soda-fork\_phoenix-link\COMPILAR_LINK.bat`;
   - tarda de 1 a 10 minutos y **no borra nada**;
   - al final dice `LISTO`.
   - **Nunca** uses `COMPILAR_PHOENIX.bat`: borra lo que no está guardado en git.
2. **Abrir** `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\PhoenixLink.exe`. Si Windows pregunta por el firewall, pulsa **Permitir**.
3. **Cuenta web:** en SYNC › Cuenta web, vincula la PC con el código de 6 cifras de tu perfil (si no lo hiciste ya).
4. **(Opcional) Módulo del partido:** con PES **cerrado**, ve a SYNC › Puente › «Módulos del juego» › **INSTALAR** `phoenix_estadio.lua`.
5. **Abrir PES 2021** como siempre (con el switcher de ConmeGOL).
6. **Abrir la sala:** Sala › **Abrir sala** › **Copiar enlace** › mándaselo a tu amigo por WhatsApp o Discord.

## B) En la PC de tu amigo (5 minutos)

1. Descargar **Parsec** desde https://parsec.app y crear una cuenta gratis.
2. Conectar su mando (Xbox, PlayStation o genérico) **antes** de entrar.
3. Abrir el enlace que le mandaste y esperar.
4. Nada más: no necesita PES, ni Phoenix Link, ni el parche.

## C) Cuando entre

1. En Link verás su **solicitud** o lo verás en «Mirando»: dale **un puesto** (Mandos › Puestos, o arrástralo al lado Visita).
2. Partido › En vivo: elige los dos lados (tú Local, él Visita) › **¡EMPEZAR!**
3. En PES entra a Partido › Amistoso. Cada uno elige equipo con su mando.

## D) Qué probar (marca cada uno)

- [ ] **Conexión:** su ping en la portada de la sala (verde < 60 ms, naranja < 100, rojo más).
- [ ] **Su mando mueve su jugador** y la imagen va fluida.
- [ ] **Modo competitivo:** durante el partido tu amigo pulsa **Start**. **No** debe pausar. En Partido › En vivo debe salir «Start/Back/Guía bloqueados».
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
