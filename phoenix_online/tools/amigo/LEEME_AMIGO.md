# Jugar en el servidor Phoenix (prueba con un amigo)

1. Tu amigo necesita **su propia copia de PES 2021**, la misma versión que Frank (PES base, sin parche, igual que el de pruebas de Frank).
2. Frank le pasa la **IP** de su servidor.
3. El amigo abre `CONECTAR_PHOENIX.bat` con clic derecho → **Ejecutar como administrador** y escribe la IP.
4. Abre PES 2021 y entra al modo online (icono «EF»).
5. Para dejarlo como antes: `DESCONECTAR_PHOENIX.bat` (como administrador).

Notas:
- Solo cambia líneas marcadas `#PHOENIX` en el archivo hosts; guarda una copia `hosts.antes_phoenix` la primera vez.
- El servidor de Frank debe estar encendido y alcanzable desde internet (puerto abierto o VPS). Puertos: TCP 80, 5000, 10000, 10001 y UDP 5100 (ver GUIA_REPLICAR_ONLINE.md).
- Si los dos PES no tienen la misma base de jugadores, myClub mostrará otros jugadores o el juego puede cerrarse.
- Escrito por nosotros; no usa código de PESBUL.
