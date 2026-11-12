"""Sockets UDP que no se mueren en Windows.

En Windows, cuando el servidor manda un datagrama a un puerto que ya no escucha (un PES que se cerró,
el rival falso de una prueba anterior…), el sistema devuelve un «ICMP port unreachable» y la SIGUIENTE
lectura del socket falla con WinError 10054 (ConnectionResetError). En Python 3.11 el bucle de lectura
del transporte UDP de asyncio (ProactorEventLoop) llama a error_received() y NO vuelve a leer: el
puerto queda abierto pero sordo para siempre, sin ningún error en el log (CPython gh-91227, corregido
en 3.13). Prueba 12-nov: el relé 5100 dejó de contestar al KX del rival y el chequeo de conexión
falló con ER_MATCHING_RETRY.

Arreglo estándar: apagar SIO_UDP_CONNRESET en el socket (WSAIoctl), así Windows no avisa de esos
ICMP. En Linux/macOS no hace falta y no se toca nada.
"""
import logging
import socket
import sys

log = logging.getLogger("phoenix.udp")

SIO_UDP_CONNRESET = 0x9800000C


def sin_connreset(sock):
    """Apaga los avisos de «puerto inalcanzable» en un socket UDP de Windows. True si lo hizo."""
    if sys.platform != "win32":
        return False
    try:
        import ctypes
        from ctypes import wintypes
        ws2 = ctypes.WinDLL("ws2_32")
        wsaioctl = ws2.WSAIoctl
        wsaioctl.argtypes = [ctypes.c_size_t, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD,
                             ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD),
                             ctypes.c_void_p, ctypes.c_void_p]
        wsaioctl.restype = ctypes.c_int
        apagado = wintypes.BOOL(False)
        devueltos = wintypes.DWORD(0)
        r = wsaioctl(sock.fileno(), SIO_UDP_CONNRESET, ctypes.byref(apagado), ctypes.sizeof(apagado),
                     None, 0, ctypes.byref(devueltos), None, None)
        if r != 0:
            log.warning("SIO_UDP_CONNRESET no se pudo apagar (WSAIoctl=%s, error %s)", r,
                        ws2.WSAGetLastError())
            return False
        return True
    except Exception as e:  # nunca impedir que arranque el servidor por esto
        log.warning("SIO_UDP_CONNRESET no se pudo apagar: %s", e)
        return False


def socket_udp(host, puerto):
    """Socket UDP ya enlazado y protegido; para create_datagram_endpoint(sock=...)."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((host, puerto))
    except OSError:
        sock.close()
        raise
    sin_connreset(sock)
    sock.setblocking(False)
    return sock


async def abrir(bucle, fabrica, host, puerto):
    """create_datagram_endpoint con el socket protegido (mismo resultado que local_addr=(host, puerto))."""
    return await bucle.create_datagram_endpoint(fabrica, sock=socket_udp(host, puerto))


class AvisoError:
    """Mezcla para protocolos UDP: deja en el log los errores que antes se tragaba asyncio."""

    nombre_udp = "udp"

    def error_received(self, exc):
        log.warning("%s: error UDP %r (si se repite y el puerto deja de contestar, revisar "
                    "SIO_UDP_CONNRESET)", self.nombre_udp, exc)
