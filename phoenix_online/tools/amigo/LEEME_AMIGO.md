# Prueba con Frank: pasos para tu PC

Vas a ayudar a Frank a probar su servidor de PES 2021. Son 6 pasos cortos.
Nada se instala en PES ni se descarga nada raro. Si algo te da miedo, pregúntale a Frank antes.

## Antes de empezar
- Necesitas **PES 2021 en tu PC** (la misma versión que Frank).
- Frank te dirá **una dirección** que empieza con `100.` (por ejemplo `100.64.1.5`). La necesitas en el paso 4.

## Pasos
1. **Instala Tailscale** (programa gratis, sirve para que tu PC y la de Frank se vean). Entra con la cuenta que Frank te indique.
2. **Comprueba tu PES**: haz doble clic en `COMPROBAR_MI_PES.bat`. Te muestra unos números en pantalla.
   Mándale una foto o copia de esos números a Frank. Este paso **no cambia nada**.
3. **Espera** a que Frank te diga «listo, mi servidor está encendido».
4. Haz **clic derecho** en `CONECTAR_PHOENIX.bat` y elige **Ejecutar como administrador**. Escribe la dirección `100.…` que te dio Frank y pulsa Enter.
5. **Abre PES 2021** y entra al modo online (el icono «EF»).
6. Haz lo que Frank te pida (por ejemplo, crear una sala o aceptar su reto).

## Al terminar
Haz **clic derecho** en `DESCONECTAR_PHOENIX.bat` → **Ejecutar como administrador**.
Así tu PES vuelve a funcionar como antes.

## Qué hace `CONECTAR_PHOENIX.bat` (por si quieres saberlo)
- Guarda una copia de seguridad del archivo `hosts` de Windows (`hosts.antes_phoenix`).
- Añade unas pocas líneas que dicen: «los servidores de Konami del PES ahora son el servidor de Frank».
- Cada línea lleva la marca `#PHOENIX`, y `DESCONECTAR_PHOENIX.bat` borra solo esas líneas.
- No toca PES, ni el firewall, ni el registro.

## Si algo sale mal
- Si PES dice «error de conexión»: pregúntale a Frank si su servidor sigue encendido.
- Si no puedes volver a lo normal: ejecuta `DESCONECTAR_PHOENIX.bat` otra vez como administrador.
- Si PES se cierra solo: ábrelo de nuevo y avisa a Frank.

---

### Para Frank (notas técnicas)
- Escrito por nosotros; no usa código de PESBUL.
- Los dos PES deben tener la misma base de jugadores; si no, myClub puede mostrar otros jugadores o el juego puede cerrarse.
- El servidor debe estar encendido y alcanzable por la IP de Tailscale. Puertos: TCP 80, 5000, 10000, 10001 y UDP 5100, 3478, 5730 (ver `GUIA_REPLICAR_ONLINE.md`). El día de la prueba, ver `DIA_DE_LA_PRUEBA.md`.
- Partido entre los dos: «Partida rápida» o «Divisiones en línea» van por el relé del servidor (lo más seguro). El amistoso de sala libre va directo entre las dos PCs (P2P).
- Con el pase de la web (cuando esté activo): `USAR_PASE.bat`, escribe la IP del servidor y pega el pase de 10 minutos. El pase nunca se guarda en ningún archivo ni registro; solo viaja su huella.
