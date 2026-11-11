"""Arranque: ConnectGate (TCP 10000), MENU (TCP 10001) y HTTP (URLs, TURN).

Uso:  python -m phoenix_online.servidor config.json
Cada sesión deja un registro JSONL en `capturas/` (comandos recibidos y
enviados con hora), para diagnosticar con el método de cap_72 §11.9.
"""
import asyncio
import gzip
import json
import logging
import logging.handlers
import os
import shutil
import socket
import sys
import time
import urllib.parse

from . import msgpack_min
from .datos import Datos
from .logica import Servidor, Sesion
from .nclmio import Codec, LectorTramas
from .stun import ProtocoloStun
from .gamerelay import Rele

log = logging.getLogger("phoenix_online")


def _json(o):
    if isinstance(o, bytes):
        return "hex:" + o.hex()
    if isinstance(o, msgpack_min.Empaquetado):
        return {"bin": o.valor}
    return repr(o)


class Presupuesto:
    """Tope de disco para las capturas (protección: antes podían llenar el disco).

    Se cuenta lo que ya hay en la carpeta al arrancar; al pasar el tope se dejan de escribir
    capturas (el servidor sigue funcionando) y se avisa una vez en el registro."""

    def __init__(self, carpeta, total_mb=2000, archivo_mb=50, activo=True):
        self.carpeta = carpeta
        self.tope = int(total_mb) * 1024 * 1024
        self.tope_archivo = int(archivo_mb) * 1024 * 1024
        self.activo = bool(activo)
        self.usado = 0
        self.avisado = False
        try:
            for raiz, _d, archivos in os.walk(carpeta):
                self.usado += sum(os.path.getsize(os.path.join(raiz, a)) for a in archivos)
        except OSError:
            pass

    def permitir(self, n):
        if not self.activo:
            return False
        if self.usado + n > self.tope:
            if not self.avisado:
                log.warning("capturas: tope de %d MB alcanzado; dejo de capturar (el servidor "
                            "sigue). Mueve o comprime la carpeta %s", self.tope >> 20, self.carpeta)
                self.avisado = True
            return False
        self.usado += n
        return True


PRESUPUESTO = None  # se crea en principal(); en pruebas, sin tope


class Captura:
    def __init__(self, carpeta, direccion, svrtype):
        self.f = None
        self.escrito = 0
        if PRESUPUESTO is not None and not PRESUPUESTO.activo:
            return
        os.makedirs(carpeta, exist_ok=True)
        ip = str(direccion[0]).replace(":", "_")
        nombre = f"{time.strftime('%Y%m%d-%H%M%S')}_{svrtype}_{ip}_{direccion[1]}.jsonl"
        self.ruta = os.path.join(carpeta, nombre)
        self.f = open(self.ruta, "a", encoding="utf-8")

    def anotar(self, sentido, secuencia, msg):
        if self.f is None:
            return
        try:
            linea = json.dumps({"t": time.time(), "dir": sentido, "seq": secuencia, "msg": msg},
                               default=_json, ensure_ascii=False) + "\n"
            if PRESUPUESTO is not None and (self.escrito + len(linea) > PRESUPUESTO.tope_archivo
                                            or not PRESUPUESTO.permitir(len(linea))):
                return
            self.escrito += len(linea)
            self.f.write(linea)
            self.f.flush()
        except (OSError, ValueError) as e:
            # disco lleno o archivo cerrado: la captura nunca debe tumbar la conexión del jugador
            log.warning("captura desactivada para esta conexión: %s", e)
            self._cerrar_archivo()

    def _cerrar_archivo(self):
        try:
            if self.f:
                self.f.close()
        except OSError:
            pass
        self.f = None

    def cerrar(self):
        self._cerrar_archivo()
        # las capturas grandes se guardan comprimidas (≈10 veces menos disco, mismo contenido)
        try:
            if self.escrito > 1024 * 1024:
                with open(self.ruta, "rb") as fi, gzip.open(self.ruta + ".gz", "wb") as fo:
                    shutil.copyfileobj(fi, fo)
                os.remove(self.ruta)
        except OSError as e:
            log.warning("no pude comprimir %s: %s", getattr(self, "ruta", "?"), e)


def _vigilar_conexion(writer):
    """TCP keepalive: si la PC del jugador se apaga o pierde internet, Windows/Linux lo
    detecta en ~2 min y la conexión se libera (antes quedaba «colgada» para siempre)."""
    sock = writer.get_extra_info("socket")
    if sock is None:
        return
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        if hasattr(socket, "TCP_KEEPIDLE"):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 60)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 15)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 4)
        elif hasattr(socket, "SIO_KEEPALIVE_VALS"):  # Windows
            sock.ioctl(socket.SIO_KEEPALIVE_VALS, (1, 60000, 15000))
    except OSError:
        pass


CONEXIONES_POR_IP = {}


def _ip_local(writer):
    loc = writer.get_extra_info("sockname")
    return loc[0] if isinstance(loc, tuple) else None


async def atender(srv, codec, svrtype, carpeta, reader, writer):
    direccion = writer.get_extra_info("peername") or ("?", 0)
    ip = direccion[0]
    # protección: un programa que abra cientos de conexiones no tumba el servidor
    tope_ip = int(srv.cfg.get("max_conexiones_por_ip", 16))
    if CONEXIONES_POR_IP.get(ip, 0) >= tope_ip:
        log.warning("[%s] rechazo %s: ya tiene %d conexiones", svrtype, ip, tope_ip)
        writer.close()
        return
    CONEXIONES_POR_IP[ip] = CONEXIONES_POR_IP.get(ip, 0) + 1
    _vigilar_conexion(writer)
    try:
        cap = Captura(carpeta, direccion, svrtype)
    except OSError as e:
        log.warning("sin captura para %s: %s", direccion, e)
        cap = Captura.__new__(Captura)
        cap.f, cap.escrito = None, 0

    def enviar_trama(secuencia, mensaje):
        cap.anotar("tx", secuencia, mensaje)
        writer.write(codec.codificar(secuencia, mensaje))

    sesion = Sesion(enviar_trama, direccion, svrtype)
    sesion.ip_local = _ip_local(writer)
    lector = LectorTramas(codec)
    srv.abrir(sesion)
    log.info("[%s] conecta %s", svrtype, direccion)
    inactivo = int(srv.cfg.get("inactividad_max_s", 0)) or None  # 0 = sin límite (PES calla en menús)
    try:
        while data := await asyncio.wait_for(reader.read(65536), inactivo):
            for _tipo, secuencia, msg in lector.alimentar(data):
                cap.anotar("rx", secuencia, msg)
                await srv.recibir(sesion, secuencia, msg)
            await writer.drain()
            if sesion.cerrar_tras_responder:
                break
    except (ConnectionError, asyncio.IncompleteReadError):
        pass
    except asyncio.TimeoutError:
        log.info("[%s] %s sin datos %s s: se cierra", svrtype, direccion, inactivo)
    except Exception:
        log.exception("[%s] error con %s", svrtype, direccion)
    finally:
        CONEXIONES_POR_IP[ip] = max(0, CONEXIONES_POR_IP.get(ip, 1) - 1)
        if not CONEXIONES_POR_IP[ip]:
            del CONEXIONES_POR_IP[ip]
        try:
            await srv.cerrar(sesion)
        except Exception:
            log.exception("[%s] fallo al cerrar la sesión de %s", svrtype, direccion)
        cap.cerrar()
        writer.close()
        log.info("[%s] desconecta %s", svrtype, direccion)


EULA_HTML = ("<!doctype html><html lang=\"es\"><head><meta charset=\"utf-8\"><title>Phoenix Online</title>"
             "</head><body><h1>Phoenix Online</h1><p>Servidor comunitario de PES 2021, sin relación con "
             "KONAMI. Documento: %s</p></body></html>")


HTTP_ESPERA_S = 20             # un cliente que no termina de mandar no retiene la conexión
HTTP_MAX_CUERPO = 4 * 1024 * 1024  # 4 MB (el avatar, lo más grande, ronda 66 KB)


def _http_simple(writer, codigo, texto, datos=b"", tipo="text/plain; charset=utf-8"):
    writer.write(f"HTTP/1.1 {codigo} {texto}\r\nContent-Type: {tipo}\r\n"
                 f"Content-Length: {len(datos)}\r\nConnection: close\r\n\r\n".encode() + datos)


async def http(srv, reader, writer):
    """HTTP mínimo: imágenes de anuncios y comandos HTTP (TURN, AUTH_STEAM...)."""
    try:
        try:
            cab = await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), HTTP_ESPERA_S)
        except (asyncio.TimeoutError, asyncio.LimitOverrunError, asyncio.IncompleteReadError):
            return
        linea, *resto = cab.decode("latin-1").split("\r\n")
        metodo, ruta, _ = linea.split(" ", 2)
        cabeceras = {k.lower(): v.strip() for k, _, v in (h.partition(":") for h in resto if h)}
        try:
            largo = int(cabeceras.get("content-length", 0) or 0)
        except ValueError:
            largo = -1
        if largo < 0 or largo > HTTP_MAX_CUERPO:
            _http_simple(writer, 413, "Payload Too Large")
            return
        cuerpo = await asyncio.wait_for(reader.readexactly(largo), HTTP_ESPERA_S)
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
        # Espía: cada pedido HTTP queda en capturas/http.jsonl (cuerpo recortado y con tope de disco)
        try:
            linea_cap = json.dumps({"t": time.time(), "linea": linea, "cabeceras": cabeceras,
                                    # el pase de identidad nunca se guarda en claro
                                    "cuerpo": "(oculto)" if ruta_sin_query.rstrip("/") in ("/pase", "/entrar")
                                    else cuerpo[:8192].decode("latin-1")},
                                   ensure_ascii=False) + "\n"
            if PRESUPUESTO is None or PRESUPUESTO.permitir(len(linea_cap)):
                carpeta = srv.cfg.get("capturas", "capturas")
                os.makedirs(carpeta, exist_ok=True)
                with open(os.path.join(carpeta, "http.jsonl"), "a", encoding="utf-8") as f:
                    f.write(linea_cap)
        except OSError as e:
            log.warning("http.jsonl sin escribir: %s", e)
        if metodo == "GET" and ruta_sin_query.rstrip("/") == "/estado":
            # Estado del servidor (para Link, la web o mirarlo desde el celular)
            _http_simple(writer, 200, "OK", json.dumps(srv.estado(), ensure_ascii=False).encode(),
                         "application/json; charset=utf-8")
            return
        if metodo == "POST" and ruta_sin_query.rstrip("/") == "/entrar":
            # Phoenix Launcher: «Jugar Online» con la sesión de la web (respuesta JSON)
            ip = (writer.get_extra_info("peername") or (None,))[0]
            codigo, r = await srv.entrar_launcher(form.get("token"), ip, _ip_local(writer))
            _http_simple(writer, codigo, "OK" if codigo == 200 else "Error",
                         json.dumps(r, ensure_ascii=False).encode(), "application/json; charset=utf-8")
            return
        if metodo == "POST" and ruta_sin_query.rstrip("/") == "/pase":
            # Pase de identidad de la web (Link lo manda; la respuesta es JSON)
            ip = (writer.get_extra_info("peername") or (None,))[0]
            codigo, r = await srv.canjear_pase(form.get("pase"), ip, _ip_local(writer))
            _http_simple(writer, codigo, "OK" if codigo == 200 else "Error",
                         json.dumps(r, ensure_ascii=False).encode(), "application/json; charset=utf-8")
            return
        if metodo == "GET" and ruta_sin_query.startswith("/eula"):
            # Textos legales que el cliente muestra tras elegir país (EULA_PES/MYCLUB/PRIVACY_POLICY)
            datos = (EULA_HTML % ruta_sin_query).encode("utf-8")
            writer.write(f"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                         f"Content-Length: {len(datos)}\r\nConnection: close\r\n\r\n".encode() + datos)
            return
        if metodo in ("GET", "HEAD") and ruta_sin_query.startswith("/actualizacion/"):
            # actualización en vivo propia (actualizacion.py): solo el .cpk que se ofrece ahora
            from . import actualizacion
            archivo = actualizacion.archivo_para(srv.cfg.get("carpeta_actualizacion", "actualizacion"),
                                                 urllib.parse.unquote(ruta_sin_query.rsplit("/", 1)[-1]))
            if archivo is None:
                writer.write(b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
                return
            tam = os.path.getsize(archivo)
            writer.write(f"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: {tam}\r\n"
                         f"Connection: close\r\n\r\n".encode())
            if metodo == "GET":
                with open(archivo, "rb") as f:
                    for trozo in iter(lambda: f.read(1 << 20), b""):
                        writer.write(trozo)
                        await writer.drain()
                log.info("actualización %s enviada a %s", os.path.basename(archivo),
                         (writer.get_extra_info("peername") or ("",))[0])
            return
        if ruta_sin_query.lstrip("/").startswith("livedata/"):
            respuesta = srv.respuesta_livedata(ruta_sin_query.rstrip("/").rsplit("/", 1)[-1], form)
        else:
            ip = (writer.get_extra_info("peername") or (None,))[0]
            respuesta = srv.respuesta_http(form, ip, _ip_local(writer))
        if respuesta is not None:
            # P: formato de respuesta HTTP = MessagePack (como TURN); confirmar en la captura
            datos, tipo = msgpack_min.packb(respuesta), "application/x-msgpack"
            if "QUALITYCHECKLIST" in str(form.get("msgid", "")):
                tipo = "application/octet-stream"  # L (PESBUL): así responde el checklist
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
        try:
            await writer.drain()
        except (ConnectionError, OSError):
            pass
        writer.close()


async def principal(ruta_config):
    with open(ruta_config, encoding="utf-8") as f:
        cfg = json.load(f)
    with open(cfg["clave_nclmio"], "rb") as f:
        codec = Codec(f.read(), blowfish_le=cfg.get("blowfish_le", False),
                      tipos_troceado=[int(str(t), 0) for t in cfg.get("tipos_troceado") or []] or None)
    if cfg.get("turn"):
        # el secreto TURN nunca va en el archivo: variable de entorno
        cfg["turn"]["secreto"] = os.environ["PHOENIX_TURN_SECRETO"]
    # Solo cuentas de la web Phoenix (decisión de Frank, 28-oct): sin archivo de vínculos no entra nadie
    vinculos = None if cfg.get("abierto_a_todos") else {}
    if cfg.get("vinculos"):
        with open(cfg["vinculos"], encoding="utf-8") as f:
            vinculos = {k.lower(): v for k, v in json.load(f).items()}
    datos = Datos(cfg.get("base_datos", "phoenix_online.db"), vinculos)
    srv = Servidor(cfg, datos)
    srv.ruta_config = ruta_config  # recarga en caliente de cfg['myclub'] (ver Servidor._recargar_config)
    carpeta = cfg.get("capturas", "capturas")
    global PRESUPUESTO
    PRESUPUESTO = Presupuesto(carpeta, cfg.get("capturas_total_mb", 2000),
                              cfg.get("capturas_archivo_mb", 50), cfg.get("capturas_activas", True))
    escucha = cfg.get("escuchar_en", "0.0.0.0")
    try:
        servidores = [
            await asyncio.start_server(lambda r, w: atender(srv, codec, "CONNECTGATE", carpeta, r, w),
                                       escucha, cfg.get("puerto_gate", 10000)),
            await asyncio.start_server(lambda r, w: atender(srv, codec, "MENU", carpeta, r, w),
                                       escucha, cfg["puerto_menu"]),
        ]
        # HTTP en uno o varios puertos (5000 propio; 80 para el respaldo http:// de GateInfo.php)
        for puerto in cfg.get("puertos_http", [cfg.get("puerto_http", 5000)]):
            servidores.append(await asyncio.start_server(lambda r, w: http(srv, r, w), escucha, puerto))
    except OSError as e:
        # casi siempre: el servidor ya está abierto en otra ventana (o PESBUL local sigue activo)
        log.error("no puedo abrir los puertos (%s). ¿Ya hay otro servidor abierto? Ciérralo y "
                  "vuelve a arrancar.", e)
        datos.cerrar()
        raise SystemExit(SALIDA_PUERTO_OCUPADO)
    bucle = asyncio.get_running_loop()
    for puerto in cfg.get("puertos_stun", [5730, 3478]):
        await bucle.create_datagram_endpoint(ProtocoloStun, local_addr=("0.0.0.0", puerto))
    # Relé UDP (comprobación de red). Claves solo en un archivo local (ESPEC_GAMERELAY §2b-C)
    g = cfg.get("gamerelay") or {}
    if cfg.get("clave_gamerelay") and g.get("activo", True):
        rele = Rele.desde_archivo(srv, cfg["clave_gamerelay"], int(g.get("indice", 1)))
        await bucle.create_datagram_endpoint(lambda: rele, local_addr=("0.0.0.0", int(g.get("puerto", 5100))))
        srv.rele = rele
        log.info("gamerelay UDP en %s", g.get("puerto", 5100))
    else:
        log.warning("sin clave_gamerelay: no hay relé UDP; la comprobación de red fallará")
    # Relé TURN propio (R: AzrakHQ, partido entero por relé probado con 2 PES). Opcional y apagable.
    tp = cfg.get("turn_propio") or {}
    if tp.get("activo"):
        from .turn_propio import TurnPropio
        turn = TurnPropio(tp)
        ip_fija = str(cfg.get("ip_publica") or "auto")
        ips = [ip_fija] if ip_fija.lower() != "auto" else []
        if await turn.iniciar(lambda ip_local, ip_cliente: srv.ip_anuncio(ip_local=ip_local), ips):
            srv.turn_propio = turn
    log.info("Phoenix Online escuchando en %s", [s.sockets[0].getsockname() for s in servidores])
    tareas = [s.serve_forever() for s in servidores]
    if cfg.get("reglas_myclub"):
        tareas.append(vigilar_reglas(srv, cfg["reglas_myclub"], int(cfg.get("reglas_cada_s", 30))))
    from .nube import desde_config
    nube = desde_config(srv, cfg)
    srv.nube = nube
    if nube is not None:
        tareas.append(nube.correr())
        log.info("espejo en Supabase activo (cada %d s)", nube.cada_s)
    r = cfg.get("respaldos", {})
    if r.get("activo", True):
        tareas.append(respaldar_cada(datos, r.get("carpeta", "respaldos"),
                                     float(r.get("cada_horas", 6)), int(r.get("guardar", 28))))
    try:
        await asyncio.gather(*tareas)
    finally:
        datos.cerrar()


SALIDA_PUERTO_OCUPADO = 3  # el arrancador no reintenta con este código


async def respaldar_cada(datos, carpeta, horas, guardar):
    """Copia de la base al arrancar y cada `horas`. Un fallo no detiene el servidor."""
    while True:
        try:
            ruta = await asyncio.to_thread(datos.respaldar, carpeta, guardar)
            if ruta:
                log.info("respaldo de la base: %s", ruta)
        except Exception as e:
            log.warning("respaldo de la base falló (se reintenta luego): %s", e)
        await asyncio.sleep(max(0.1, horas) * 3600)


async def vigilar_reglas(srv, ruta, cada):
    """Relee el archivo de reglas de myClub cuando cambia (sin reiniciar el servidor).
    Más adelante lo escribirá el espejo de Supabase (docs/PLAN_MULTIUSUARIO.md §3)."""
    visto = None
    while True:
        try:
            marca = os.path.getmtime(ruta)
            if marca != visto:
                with open(ruta, encoding="utf-8") as f:
                    srv.aplicar_reglas_myclub(json.load(f))
                visto = marca
        except FileNotFoundError:
            pass
        except Exception as e:  # archivo a medio escribir o JSON roto: se reintenta luego
            log.warning("reglas_myclub sin aplicar: %s", e)
        await asyncio.sleep(cada)


def configurar_registro(cfg_ruta):
    """Consola + archivo con rotación (registro/servidor.log, 10 MB × 5): el historial no se
    pierde al cerrar la ventana y tampoco crece sin límite."""
    formato = logging.Formatter("%(asctime)s %(levelname)s %(message)s")
    raiz = logging.getLogger()
    raiz.setLevel(logging.INFO)
    consola = logging.StreamHandler()
    consola.setFormatter(formato)
    raiz.addHandler(consola)
    try:
        with open(cfg_ruta, encoding="utf-8") as f:
            carpeta = json.load(f).get("registro", "registro")
    except (OSError, ValueError):
        carpeta = "registro"
    if carpeta:
        try:
            os.makedirs(carpeta, exist_ok=True)
            archivo = logging.handlers.RotatingFileHandler(
                os.path.join(carpeta, "servidor.log"), maxBytes=10 * 1024 * 1024, backupCount=5,
                encoding="utf-8")
            archivo.setFormatter(formato)
            raiz.addHandler(archivo)
        except OSError as e:
            log.warning("sin registro en archivo: %s", e)


if __name__ == "__main__":
    ruta = sys.argv[1] if len(sys.argv) > 1 else "config.json"
    configurar_registro(ruta)
    try:
        asyncio.run(principal(ruta))
    except KeyboardInterrupt:
        log.info("servidor detenido con Ctrl+C")
