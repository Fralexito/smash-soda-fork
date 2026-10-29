# Phoenix Online — servidor propio (v0.1, 28-oct-2026)

Servidor para que PES 2021 (Steam 1.07.02) juegue online con cuentas Phoenix. Escrito desde la especificación de sala limpia `para-proyecto/pes/cap_72_pesbul_como_funciona.md` §11 y desde el código MIT de Nikow5. **No contiene código de PESBUL.**

## Qué hace hoy
- Trama NclMio completa: XOR, cabecera con MD5, Blowfish ECB, compresión zlib, MessagePack y armado de tramas aunque TCP las parta (`nclmio.py`, `msgpack_min.py`).
- ConnectGate: lista de URLs, versiones, hora, naciones EULA, lista de servidores y anuncios.
- Login: estados 5 a 9, alta de jugador e identidad por SteamID. Puede exigir que el SteamID esté vinculado a una cuenta Phoenix (`vinculos`).
- Lobby y salas del amistoso 1v1: crear, unirse con el orden de empujes que evita UQOW_0006, «Empezar» (SETENV a GAME), resultado de los dos lados y salida.
- STUN propio (UDP 5730 y 3478) para que el juego descubra su IP antes del P2P.
- TURN con credenciales REST de coturn. El secreto se lee de la variable `PHOENIX_TURN_SECRETO` y nunca va en archivos.
- Registro JSONL de cada sesión en `capturas/`.
- 16 pruebas: `python -m unittest -v tests.test_phoenix_online`.

## Qué falta (marcado «P» en el código)
1. ✓ Clave real extraída (28-oct, SHA-256 coincide) en `D:\Frank\IA\CLAUDE\pes-online\nclmio_key.bin`; Blowfish con palabras little-endian (`blowfish_le: true`), XOR desde el byte 0 y MD5 sobre cabecera 8 B + cifrado: los tres CONFIRMADOS y ya así en el código.
2. Los valores de `versiones` y la lista completa de tipos de URL.
3. Las claves internas de `pname`, el formato de `date`, los códigos de error propios y los campos de `CMD_WATCH_ENTRY_GAME`.
4. El primer paso de autenticación Steam (ticket y challenge) antes de `CMD_LOGIN`.
5. El envío segmentado de mensajes de más de 0x410 bytes.
6. Probar con dos PCs en redes distintas.

## Requisitos
Python 3.11 o más nuevo y el paquete `cryptography`. La configuración se toma de `config.ejemplo.json`, copiado como `config.json`.
