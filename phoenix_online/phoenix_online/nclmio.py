"""Trama TCP «NclMio» (cap_72 §11.2).

Trama en el cable (todo con XOR cíclico 5B 9F 2E 64):
    tipo u16 BE | largo_cifrado u16 BE | secuencia u32 BE | MD5(16) | cifrado
MD5 = md5(8 bytes de cabecera + cifrado).
Cifrado = Blowfish ECB (clave de 56 bytes sacada del exe) de:
    tamaño_codificado u32 BE | tamaño_original u32 BE | zlib u8 | datos | ceros hasta múltiplo de 8
Datos = MessagePack (un mapa con msgid, rqid, ...).

Como cada trama mide 24 + 8·k bytes (múltiplo de 4), da igual si el XOR corre
continuo por la conexión o reinicia en cada trama. La endianness de Blowfish
(P) queda como opción de `Codec` hasta confirmarla con REA.
"""
import hashlib
import struct
import zlib

from . import msgpack_min

XOR_CLAVE = bytes((0x5B, 0x9F, 0x2E, 0x64))
TIPO_USUAL = 0x0060
CABECERA = 24
MAX_CIFRADO_CLIENTE = 0x410  # el cliente descarta tramas más grandes
SHA256_CLAVE = "e8b7fa83e4f9b37efaaa3ae9dab744ca903dc08c565304ac6dd658f211f60519"


class ErrorTrama(Exception):
    pass


def xor(data, inicio=0):
    k = XOR_CLAVE
    return bytes(b ^ k[(i + inicio) & 3] for i, b in enumerate(data))


def _blowfish(clave):
    from cryptography.hazmat.decrepit.ciphers.algorithms import Blowfish
    from cryptography.hazmat.primitives.ciphers import Cipher, modes
    return Cipher(Blowfish(clave), modes.ECB())


def _swap32(b):
    # Blowfish «little-endian»: cada palabra de 4 bytes invertida antes y después
    return b"".join(b[i:i + 4][::-1] for i in range(0, len(b), 4))


def verificar_clave(clave):
    return len(clave) == 56 and hashlib.sha256(clave).hexdigest() == SHA256_CLAVE


class Codec:
    def __init__(self, clave, blowfish_le=False, comprobar_clave=True):
        if comprobar_clave and not verificar_clave(clave):
            raise ValueError("la clave NclMio no coincide con el SHA-256 esperado")
        self._cifra = _blowfish(clave)
        self.blowfish_le = blowfish_le

    def _enc(self, b):
        if self.blowfish_le:
            b = _swap32(b)
        e = self._cifra.encryptor()
        out = e.update(b) + e.finalize()
        return _swap32(out) if self.blowfish_le else out

    def _dec(self, b):
        if self.blowfish_le:
            b = _swap32(b)
        d = self._cifra.decryptor()
        out = d.update(b) + d.finalize()
        return _swap32(out) if self.blowfish_le else out

    def codificar(self, secuencia, mensaje, tipo=TIPO_USUAL):
        """Devuelve la trama lista para el cable. Comprime si no cabe en 0x410."""
        datos = msgpack_min.packb(mensaje)
        cifrado = self._sellar(datos, comprimir=False)
        if len(cifrado) > MAX_CIFRADO_CLIENTE:
            cifrado = self._sellar(datos, comprimir=True)
        if len(cifrado) > MAX_CIFRADO_CLIENTE:
            # Falta el envío segmentado (investigar en el exe cuando haga falta)
            raise ErrorTrama(f"mensaje demasiado grande ({len(cifrado)} B) para una trama")
        cab = struct.pack(">HHI", tipo, len(cifrado), secuencia)
        return xor(cab + hashlib.md5(cab + cifrado).digest() + cifrado)

    def _sellar(self, datos, comprimir):
        cuerpo = zlib.compress(datos) if comprimir else datos
        interno = struct.pack(">IIB", len(cuerpo), len(datos), 1 if comprimir else 0) + cuerpo
        interno += b"\0" * (-len(interno) % 8)
        return self._enc(interno)

    def descifrar(self, tipo, secuencia, md5, cifrado):
        cab = struct.pack(">HHI", tipo, len(cifrado), secuencia)
        if hashlib.md5(cab + cifrado).digest() != md5:
            raise ErrorTrama("MD5 incorrecto")
        if len(cifrado) % 8:
            raise ErrorTrama("cifrado no es múltiplo de 8")
        interno = self._dec(cifrado)
        cod, orig, flag = struct.unpack(">IIB", interno[:9])
        cuerpo = interno[9:9 + cod]
        datos = zlib.decompress(cuerpo) if flag else cuerpo
        if len(datos) != orig:
            raise ErrorTrama(f"tamaño original {orig} != {len(datos)}")
        return msgpack_min.unpackb(datos)


class LectorTramas:
    """Arma tramas completas aunque TCP las parta o las pegue (fallo de Nikow5)."""

    def __init__(self, codec):
        self.codec = codec
        self._buf = bytearray()

    def alimentar(self, data):
        self._buf += data
        salida = []
        while len(self._buf) >= CABECERA:
            cab = xor(bytes(self._buf[:CABECERA]))
            tipo, largo, secuencia = struct.unpack(">HHI", cab[:8])
            total = CABECERA + largo
            if len(self._buf) < total:
                break
            trama = xor(bytes(self._buf[:total]))
            del self._buf[:total]
            msg = self.codec.descifrar(tipo, secuencia, trama[8:24], trama[24:])
            salida.append((tipo, secuencia, msg))
        return salida
