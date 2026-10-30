"""STUN mínimo (RFC 5389): responde Binding con la dirección pública que ve el servidor.

El juego lo usa para descubrir su IP y puerto antes del P2P (UDP 5730; también 3478).
"""
import asyncio
import logging
import socket
import struct

log = logging.getLogger("phoenix_online")

COOKIE = 0x2112A442
BINDING_REQUEST = 0x0001
BINDING_RESPONSE = 0x0101
MAPPED_ADDRESS = 0x0001
XOR_MAPPED_ADDRESS = 0x0020


def respuesta_binding(peticion, ip, puerto):
    """Devuelve la respuesta a un Binding Request, o None si no lo es."""
    if len(peticion) < 20:
        return None
    tipo, _largo, cookie = struct.unpack(">HHI", peticion[:8])
    if tipo != BINDING_REQUEST or cookie != COOKIE:
        return None
    txid = peticion[8:20]
    ip_b = socket.inet_aton(ip)
    mapped = struct.pack(">HHBBH", MAPPED_ADDRESS, 8, 0, 1, puerto) + ip_b
    xip = bytes(a ^ b for a, b in zip(ip_b, struct.pack(">I", COOKIE)))
    xmapped = struct.pack(">HHBBH", XOR_MAPPED_ADDRESS, 8, 0, 1, puerto ^ (COOKIE >> 16)) + xip
    cuerpo = mapped + xmapped
    return struct.pack(">HHI", BINDING_RESPONSE, len(cuerpo), COOKIE) + txid + cuerpo


class ProtocoloStun(asyncio.DatagramProtocol):
    def connection_made(self, transporte):
        self.t = transporte

    def datagram_received(self, data, addr):
        r = respuesta_binding(data, addr[0], addr[1])
        if r is None:
            log.info("STUN: paquete no reconocido de %s (%d B) %s", addr, len(data), data[:16].hex())
            return
        self.t.sendto(r, addr)
