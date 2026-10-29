"""Arranque: ConnectGate (TCP 10000), MENU (TCP 10001) y HTTP (URLs, TURN).

Uso:  python -m phoenix_online.servidor config.json
Cada sesión deja un registro JSONL en `capturas/` (comandos recibidos y
enviados con hora), para diagnosticar con el método de cap_72 §11.9.
"""
import asyncio
import json
import logging
import os
import sys
import time
import urllib.parse

from . import msgpack_min
from .datos import Datos
from .logica import Servidor, Sesion
from .nclmio import Codec, LectorTramas
from .stun import ProtocoloStun

log = logging.getLogger("phoenix_online")


def _json(o):
    if isinstance(o, bytes):
        return "hex:" + o.hex()
    if isinstance(o, msgpack_min.Empaquetado):
        return {"bin": o.valor}
    return repr(o)


class Captura:
    def __init__(self, carpeta, direccion, svrtype):
        os.makedirs(carpeta, exist_ok=True)
        nombre = f"{time.strftime('%Y%m%d-%H%M%S')}_{svrtype}_{direccion[0]}_{direccion[1]}.jsonl"
        self.f = open(os.path.join(carpeta, nombre), "a", encoding="utf-8")

    def anotar(self, sentido, secuencia, msg):
        self.f.write(json.dumps({"t": time.time(), "dir": sentido, "seq": secuencia, "msg": msg},
                                default=_json, ensure_ascii=False) + "\n")
        self.f.flush()

    def cerrar(self):
        self.f.close()


async def atender(srv, codec, svrtype, carpeta, reader, writer):
    direccion = writer.get_extra_info("peername") or ("?", 0)
    cap = Captura(carpeta, direccion, svrtype)

    def enviar_trama(secuencia, mensaje):
        cap.anotar("tx", secuencia, mensaje)
        writer.write(codec.codificar(secuencia, mensaje))

    sesion = Sesion(enviar_trama, direccion, svrtype)
    lector = LectorTramas(codec)
    srv.abrir(sesion)
    log.info("[%s] conecta %s", svrtype, direccion)
    try:
        while data := await reader.read(65536):
            for _tipo, secuencia, msg in lector.alimentar(data):
                cap.anotar("rx", secuencia, msg)
                await srv.recibir(sesion, secuencia, msg)
            await writer.drain()
            if sesion.cerrar_tras_responder:
                break
    except (ConnectionError, asyncio.IncompleteReadError):
        pass
    except Exception:
        log.exception("[%s] error con %s", svrtype, direccion)
    finally:
        await srv.cerrar(sesion)
        cap.cerrar()
        writer.close()
        log.info("[%s] desconecta %s", svrtype, direccion)


async def http(srv, reader, writer):
    """HTTP mínimo: imágenes de anuncios y comandos HTTP (TURN, AUTH_STEAM...)."""
    try:
        cab = await reader.readuntil(b"\r\n\r\n")
        linea, *resto = cab.decode("latin-1").split("\r\n")
        metodo, ruta, _ = linea.split(" ", 2)
        cabeceras = {k.lower(): v.strip() for k, _, v in (h.partition(":") for h in resto if h)}
        cuerpo = await reader.readexactly(int(cabeceras.get("content-length", 0) or 0))
        ruta_sin_query, _, query = ruta.partition("?")
        if "json" in cabeceras.get("content-type", ""):
            try:
                form = {str(k): v for k, v in json.loads(cuerpo or b"{}").items()}
            except (ValueError, AttributeError):
                form = {}
        else:
            form = {k: v[0] for k, v in
                    urllib.parse.parse_qs(query + "&" + cuerpo.decode("latin-1")).items()}
        log.info("HTTP %s %s %s", metodo, ruta_sin_query, sorted(form))
        # Espía: todo pedido HTTP queda completo en capturas/http.jsonl (formato aún desconocido)
        carpeta = srv.cfg.get("capturas", "capturas")
        os.makedirs(carpeta, exist_ok=True)
        with open(os.path.join(carpeta, "http.jsonl"), "a", encoding="utf-8") as f:
            f.write(json.dumps({"t": time.time(), "linea": linea, "cabeceras": cabeceras,
                                "cuerpo": cuerpo.decode("latin-1")}, ensure_ascii=False) + "\n")
        respuesta = srv.respuesta_http(form)
        if respuesta is not None:
            # P: formato de respuesta HTTP = MessagePack (como TURN); confirmar en la captura
            datos, tipo = msgpack_min.packb(respuesta), "application/x-msgpack"
        else:
            archivo = srv.cfg.get("archivos_http", {}).get(ruta_sin_query)
            if archivo and os.path.isfile(archivo):
                with open(archivo, "rb") as f:
                    datos, tipo = f.read(), "application/octet-stream"
            else:
                writer.write(b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
                return
        writer.write(f"HTTP/1.1 200 OK\r\nContent-Type: {tipo}\r\nContent-Length: {len(datos)}\r\n"
                     f"Connection: close\r\n\r\n".encode() + datos)
    except Exception:
        log.exception("HTTP error")
    finally:
        await writer.drain()
        writer.close()


async def principal(ruta_config):
    with open(ruta_config, encoding="utf-8") as f:
        cfg = json.load(f)
    with open(cfg["clave_nclmio"], "rb") as f:
        codec = Codec(f.read(), blowfish_le=cfg.get("blowfish_le", False))
    if cfg.get("turn"):
        # el secreto TURN nunca va en el archivo: variable de entorno
        cfg["turn"]["secreto"] = os.environ["PHOENIX_TURN_SECRETO"]
    # Solo cuentas de la web Phoenix (decisión de Frank, 28-oct): sin archivo de vínculos no entra nadie
    vinculos = None if cfg.get("abierto_a_todos") else {}
    if cfg.get("vinculos"):
        with open(cfg["vinculos"], encoding="utf-8") as f:
            vinculos = {k.lower(): v for k, v in json.load(f).items()}
    srv = Servidor(cfg, Datos(cfg.get("base_datos", "phoenix_online.db"), vinculos))
    carpeta = cfg.get("capturas", "capturas")
    servidores = [
        await asyncio.start_server(lambda r, w: atender(srv, codec, "CONNECTGATE", carpeta, r, w),
                                   "0.0.0.0", cfg.get("puerto_gate", 10000)),
        await asyncio.start_server(lambda r, w: atender(srv, codec, "MENU", carpeta, r, w),
                                   "0.0.0.0", cfg["puerto_menu"]),
    ]
    # HTTP en uno o varios puertos (5000 propio; 80 para el respaldo http:// de GateInfo.php)
    for puerto in cfg.get("puertos_http", [cfg.get("puerto_http", 5000)]):
        servidores.append(await asyncio.start_server(lambda r, w: http(srv, r, w), "0.0.0.0", puerto))
    bucle = asyncio.get_running_loop()
    for puerto in cfg.get("puertos_stun", [5730, 3478]):
        await bucle.create_datagram_endpoint(ProtocoloStun, local_addr=("0.0.0.0", puerto))
    log.info("Phoenix Online escuchando en %s", [s.sockets[0].getsockname() for s in servidores])
    await asyncio.gather(*(s.serve_forever() for s in servidores))


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    asyncio.run(principal(sys.argv[1] if len(sys.argv) > 1 else "config.json"))
