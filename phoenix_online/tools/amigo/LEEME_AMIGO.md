# Jugar en el servidor Phoenix (prueba con un amigo)

1. Tu amigo necesita **su propia copia de PES 2021**, la misma versión que Frank (PES base, sin parche, igual que el de pruebas de Frank).
2. Frank le pasa la **IP** de su servidor. Lo más fácil y seguro: los dos instalan **Tailscale** (gratis) y
   entran a la misma red; Frank le pasa su IP de Tailscale (empieza por 100.). Así no hay que abrir puertos del
   router, y cada amigo tiene una IP fija, que es la que el servidor usa para darle **su propio club**.
3. El amigo abre `CONECTAR_PHOENIX.bat` con clic derecho → **Ejecutar como administrador** y escribe la IP.
4. Abre PES 2021 y entra al modo online (icono «EF»).
5. Para dejarlo como antes: `DESCONECTAR_PHOENIX.bat` (como administrador).

Notas:
- Solo cambia líneas marcadas `#PHOENIX` en el archivo hosts; guarda una copia `hosts.antes_phoenix` la primera vez.
- El servidor de Frank debe estar encendido y alcanzable desde internet (puerto abierto o VPS). Puertos: TCP 80, 5000, 10000, 10001 y UDP 5100, 3478, 5730 (ver GUIA_REPLICAR_ONLINE.md). El servidor debe tener `ip_publica: "auto"` (o la IP de Tailscale de Frank).
- Partido entre los dos: «Partida rápida» o «Divisiones en línea» van por el relé del servidor (lo más seguro). El amistoso de sala libre va directo entre las dos PCs (P2P).
- Si los dos PES no tienen la misma base de jugadores, myClub mostrará otros jugadores o el juego puede cerrarse.
- Escrito por nosotros; no usa código de PESBUL.

## Tu propio club con el pase de la web (cuando esté activo)
1. En la web Phoenix, con tu sesión iniciada, pide un **pase para el online** (dura 10 minutos y sirve una vez).
2. Abre `USAR_PASE.bat`, escribe la IP del servidor y pega el pase.
3. Si dice «listo», entra al online: ese club es tuyo y queda ligado a tu cuenta.
El pase nunca se guarda en ningún archivo ni registro del servidor; solo viaja su huella (hash).
