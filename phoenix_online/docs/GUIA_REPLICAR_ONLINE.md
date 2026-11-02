# Guía: poner Phoenix Online en cualquier PES 2021 (o en el parche propio)

Estado al 02-nov-2026: el recorrido completo de un jugador funciona, de la elección de país hasta «Buscando rival» y Cancelar. La pareja de dos jugadores y el partido todavía NO están probados (ver ESTADO_TRASPASO.md).

## 1. Qué necesita el PES del jugador
1. **Versión:** PES 2021 1.07.02, DLC 7.00 (Data Pack 7). El servidor devuelve la misma versión que manda el cliente, así que no hay que tocar nada.
2. **Botones online visibles.** Conmegol los esconde con `livecpk\Menu` cuando se abre con Sider. Por eso hoy se abre `PES2021.exe` **sin Sider** y se entra por el icono «EF» → Partida rápida.
   - En el parche propio: no esconder los botones online, o añadir una casilla propia en el menú (es tarea del Core).
3. **Mismos datos para los dos jugadores:** el mismo parche y los mismos cpk. Si son distintos, el partido se desincroniza. El servidor recibe `dp_file_list` y `file_hash_list` y podrá exigir que coincidan (`exigir_misma_huella`).
4. **Identidad distinta por jugador:** las copias CPY traen el mismo SteamID (239200). Hasta tener el «pase» de la web, cada jugador necesita un SteamID diferente en el CPY.ini de su shim, y ese ID debe estar dado de alta en `vinculos.json`.

## 2. Redirigir PES a nuestro servidor
Hay dos opciones:
- **Hosts (lo que se usa hoy):** apuntar estos 8 dominios a la IP del servidor:
  `pes21-x64-gate.cs.konami.net`, `pes21-x64-stun.cs.konami.net`, `pesam.stun.service.konami.net`, `info.service.konami.net`, `cs.konami.net`, `ntl.service.konami.net`, `ntleu.service.konami.net`, `ntljp.service.konami.net`.
  - En la PC de Frank lo hacen `ACTIVAR_PRUEBA_ONLINE.ps1` y `RESTAURAR_HOSTS.ps1` (como administrador). Copia de seguridad: `hosts_copia_antes_ntl.txt`.
- **Core (futuro, sin admin):** módulo `redireccion_online` (gancho en la IAT de la resolución de nombres, más las 4 IP fijas de AWS de Konami). Está en la rama `claude/online-phoenix-core`, sin instalar.

## 3. Arrancar el servidor
1. Python 3.11 o superior. No hace falta instalar paquetes: msgpack está hecho a mano en `msgpack_min.py`.
2. Copiar `config.ejemplo.json` a `config.json` y cambiar `ip_publica` y las URLs por la IP real (de la LAN, de Tailscale o del VPS).
   - Si falta una clave de URLLIST, la URL queda en NULL y el juego falla en silencio o se cae. El servidor completa solo las claves obligatorias (`URLS_EULA` en logica.py).
3. Poner la clave NclMio en la ruta de `clave_nclmio`. **Esa clave nunca va en repos, chat ni documentos.**
4. Crear `vinculos.json` a partir de `vinculos.ejemplo.json`, o poner `abierto_a_todos: true` solo para pruebas.
5. Ejecutar `python -m phoenix_online.servidor config.json`.
6. Puertos:
   - TCP 10000 (gate), 10001 (menú), 5000 y 80 (HTTP);
   - UDP 3478 y 5730 (STUN).
   - PESBUL usa además UDP 5739/5740.
7. Pruebas: `python -m unittest tests.test_phoenix_online` (deben pasar todas).

## 4. Primera vez de cada jugador
País → aceptar los 3 documentos (pulsar Enter en cada uno, aunque no se abran) → nombre de usuario → «no hay actualizaciones» → menú online. Las siguientes veces PES lo recuerda todo.

## 5. Reglas de prueba
- No reiniciar el servidor mientras alguien está dentro (da C_MHJX_0002).
- No hacer detach de Frida con PES abierto (lo cierra). Frida Stalker también lo tumba.
- Al terminar, restaurar el hosts.

## 6. Dónde está cada detalle
- Protocolo y cada respuesta: comentarios en `phoenix_online/logica.py`. «C» = comprobado; «P» = provisional.
- Historia y hallazgos: `docs/ESTADO_TRASPASO.md`, `docs/cap_62_hallazgos_online_29oct.md`, `docs/PHOENIX_ONLINE_REA_GUIA.md`.
- Lo que falta: la pareja de dos, STUN/NAT real (hoy PES publica 127.0.0.1), el partido y su resultado, la web (pase + resultados), el VPS y el envío segmentado de respuestas grandes.
