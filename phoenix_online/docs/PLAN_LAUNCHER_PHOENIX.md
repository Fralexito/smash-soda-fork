# Plan: Phoenix Launcher obligatorio (cuenta de la web + Online/Offline) — 10-nov-2026

Pedido de Frank (10-nov 22:00): «un launcher donde pueda iniciar sesión con la cuenta de la página y se vincule
con la web; que se abra sí o sí para iniciar el PES, y que ahí se elija entrar en modo online o iniciar el PES
en modo offline (nada que ver con lo online: el que quiera juega offline y local)».

## Cómo lo vería la persona
1. Abre **Phoenix Launcher** (acceso directo «PES 2021 Phoenix»).
2. La primera vez inicia sesión con su cuenta de la web; queda recordada.
3. Elige entre dos botones grandes:
   - **Jugar Online**: el launcher prepara la conexión al servidor Phoenix, avisa al servidor «este PES es de
     esta cuenta» y abre el PES. Al entrar al online ya tiene su club, sus GP y todo.
   - **Jugar Offline**: quita cualquier conexión al servidor y abre el PES normal. Sin cuenta, sin internet, sin
     nada del online.
4. Además: perfil del parche (V1/V2), caras y lo que ya tiene el `Phoenix Launcher.bat` actual.

## Por qué «obligatorio» sin romper nada
- No se toca el `.exe` del PES (Denuvo; regla de Frank: nunca saltar protecciones).
- Lo obligatorio es el **online**: el servidor solo deja entrar a un PES que el launcher haya presentado hace
  poco (`identidad.exigir_launcher`). Si alguien abre el PES directo, el **offline funciona igual que siempre**.
  Si intenta el online sin el launcher, ve el aviso «Abre Phoenix Launcher para jugar online».
- La PC del servidor (Frank) queda exenta: su club no cambia.

## Piezas técnicas
| Pieza | Qué hace | Estado |
|---|---|---|
| Inicio de sesión | Supabase Auth con la clave pública (la misma que usa la web) y la sesión guardada cifrada en la PC (DPAPI de Windows) | por hacer |
| Presentar el PES | El launcher manda al servidor `POST /entrar {token}`. El servidor comprueba el token con Supabase (`/auth/v1/user`) y liga esa PC a la cuenta. No hace falta pase ni código. | por hacer (el `POST /pase` de hoy queda de respaldo) |
| Online | Redirige los dominios de Konami al servidor. Hoy se hace con `hosts`, lo que pide administrador una vez; más adelante lo hará Phoenix Core sin tocar `hosts`. | `CONECTAR_PHOENIX.bat` ya existe |
| Offline | Quita la redirección (marcas `#PHOENIX`) y abre el PES. | `DESCONECTAR_PHOENIX.bat` ya existe |
| Mismo parche | Antes del online, compara la huella de los archivos con la del servidor y avisa si no coincide. | el servidor ya calcula la huella |
| Estado | Lee `GET /estado`: servidor encendido, mantenimiento, jugadores en línea. | ya existe |

## Con qué construirlo (lo mejor que ya existe)
- **Base: el `Phoenix Launcher.bat` actual** (perfiles V1/V2 y pestaña Caras, hecho en el hilo del parche).
  Se reutiliza su lógica; no se crea otro programa paralelo.
- Ventana: hay que decidir entre integrarlo en **Phoenix Link**, que ya es un programa de Frank para Windows y
  ya está pensado para iniciar sesión con la web, o hacer un launcher ligero aparte. Recomendación: **dentro de
  Link**, con una pestaña «Jugar» (Online/Offline). Así es un solo programa y la sesión de la web ya está ahí.
  Hasta que Link lo tenga, una versión puente con el `.bat` actual más un paso «Iniciar sesión».

## Orden
1. ✅ Servidor (10-nov): `POST /entrar {token}` (Supabase confirma la sesión; la PC queda ligada a la cuenta) e
   `identidad.exigir_launcher` + `launcher_horas` (apagado por defecto; la PC del servidor exenta). 97 pruebas OK.
2. Launcher puente (10-nov, programado): `tools/launcher/PhoenixJugar.ps1` (+ `PhoenixJugar.json` en la PC).
   «Jugar Online»: comprueba `/estado`, inicia sesión con la web (correo y contraseña; queda guardada cifrada
   con DPAPI en %APPDATA%\PhoenixLauncher), manda `POST /entrar`, pone las líneas `#PHOENIX` en hosts
   (permiso de administrador) y abre el PES. «Jugar Offline»: quita las líneas `#PHOENIX` y abre el PES.
   Al `Phoenix Launcher.bat` del hilo del parche solo se le añaden 2 opciones de menú que llaman al `.ps1`
   (cambio mínimo, con respaldo `.antes_jugar`). No se reescribe nada de perfiles V1/V2 ni de Caras.
   Límite de la versión puente: quien entra a la web solo con Discord/Google necesita poner una contraseña en
   su perfil. Link hará el inicio con Discord/Google.
3. Link: pestaña «Jugar» con inicio de sesión de la web. Hilo de Link, cuando Frank lo diga.
4. Encender `exigir_launcher` cuando todos los amigos lo tengan.
