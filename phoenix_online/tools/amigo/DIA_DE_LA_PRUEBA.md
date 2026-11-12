# Día de la prueba con el amigo (para Frank)

Nada de esto se aplica antes del día. Cada cambio lleva su reversa. Haz copia de `config.json` antes de tocar nada.
No subas `config.json`, claves ni capturas a GitHub.

## 0. Antes (los dos)
1. Los dos instalan Tailscale e inician sesión en la misma red (tailnet). Tailscale ya está instalado en tu PC (v1.104.5) pero hoy **no tiene IP 100.x**: abre Tailscale e inicia sesión.
2. Mira tu IP de Tailscale: `tailscale ip -4` (empieza por 100.). Esa es `TU_IP_TS`.
3. Tu amigo ejecuta `COMPROBAR_MI_PES.bat` y te manda los números; compáralos con `VALORES_DE_FRANK.txt`.
   Si el SHA-256 de `PES2021.exe` o de `DpFileList.bin` no coincide, no sigas (el partido puede fallar).
4. En tu PC, deja abiertos los puertos que usa el servidor SOLO si Windows los pide (ventana del firewall). No cambies reglas a mano; si hace falta, hazlo tú. Puertos: TCP 80, 5000, 10000, 10001, 5349 y UDP 5100, 3478, 5730, 5350, 49160-49179.

## 1. Qué hay que cambiar en el servidor
Hoy `config.json` tiene `"ip_publica": "auto"`. Con `auto` el servidor da a cada PES **la IP por la que ese PES llegó**: tu PES (en la misma PC) sigue usando 127.0.0.1/192.168.x y tu amigo recibe tu IP de Tailscale. **No hace falta cambiar `ip_publica`.**
El servidor ya trata como «remoto» a cualquier IP 100.64.0.0/10 (Tailscale) para dar el `host_addr` correcto.

Lo ÚNICO que sí depende de la IP es el certificado del relé TURN propio:

| Qué | Hoy | El día de la prueba |
|---|---|---|
| `ip_publica` | `"auto"` | dejar `"auto"` |
| `gamerelay.ip` | `"auto"` | dejar `"auto"` |
| `host_addr` | lo calcula el servidor (no se escribe) | nada que cambiar |
| `turn_propio.ips` | `["192.168.100.236"]` | `["192.168.100.236", "TU_IP_TS"]` |

El certificado TURN (`turn/turn.crt`) se rehace solo al arrancar si cambian las IP de `turn_propio.ips`.

### Pasos
1. Con el servidor parado (el PES NO debe estar en el online; si lo reinicias con el PES dentro, sale VNBP_0001):
   ```
   copy config.json config.antes_dia_prueba.json
   ```
2. En `config.json`, en `turn_propio`, añade tu IP de Tailscale a `ips`:
   `"turn_propio": {"activo": true, "ips": ["192.168.100.236", "100.x.y.z"]}`
3. Arranca el servidor y comprueba en el log la línea «TURN propio en tls/5349 + dtls/5350».
4. Dale a tu amigo `TU_IP_TS` para `CONECTAR_PHOENIX.bat`.

### Reversa (todo)
```
copy /y config.antes_dia_prueba.json config.json
```
y reinicia el servidor (fuera del online). El certificado `turn/turn.crt` se rehace solo con la IP vieja.
Tu amigo ejecuta `DESCONECTAR_PHOENIX.bat` (como administrador).

## 2. Qué probar (con 2 PES)
1. Los dos entran al online; el amigo entra con su propio club (IP distinta = club propio).
2. Amistoso: tú creas sala, él la ve en la lista y entra (apartados 19 y 20 del tablero).
3. Partida rápida o Divisiones: partido entre los dos por el relé (apartados 9 y 10).
4. Cuando acabe, mira el log y el tablero, y anota resultados.

## 3. Si algo falla
- «Conexión al servidor perdida (VNBP_0001)»: pasa si reinicias el servidor con el PES dentro. OK y vuelve a entrar.
- «MXNV_0013» al conectar entre los dos: el enlace directo falló; va por el relé TURN (revisa que el relé esté en el log).
- «SGJC_0007»: el PES intentó la IP pública desde la misma casa; usa la IP de Tailscale.
- Para dejar todo como estaba: sección «Reversa».
