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
# C (05-nov, en vivo): 984 B cifrados (trama 1008) entra; 1000 (trama 1024 = 0x400) el cliente
# la rechaza (C_JZZX_0002). Límite seguro probado = 0x3D8.
MAX_CIFRADO_CLIENTE = 0x3D8
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
    def __init__(self, clave, blowfish_le=False, comprobar_clave=True, tipos_troceado=None):
        if comprobar_clave and not verificar_clave(clave):
            raise ValueError("la clave NclMio no coincide con el SHA-256 esperado")
        self._cifra = _blowfish(clave)
        self.blowfish_le = blowfish_le
        # P: tipos del envío troceado hacia el cliente (inicio, sigue, último). 0x0070-72 no los
        # aceptó PES (05-nov); config «tipos_troceado» permite probar otros (p. ej. 0x1070-72)
        self.tipos_troceado = tuple(tipos_troceado or (0x0070, 0x0071, 0x0072))

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
            return self._troceado(secuencia, zlib.compress(datos))
        return self._trama(tipo, secuencia, cifrado)

    def _trama(self, tipo, secuencia, cifrado):
        cab = struct.pack(">HHI", tipo, len(cifrado), secuencia)
        return xor(cab + hashlib.md5(cab + cifrado).digest() + cifrado)

    TROZO = 0x3B0  # P: cabe en MAX_CIFRADO_CLIENTE con la cabecera interna y el relleno

    def _troceado(self, secuencia, comprimido):
        """P (05-nov): envío troceado hacia el cliente con el MISMO formato que el cliente usa
        hacia nosotros (L: PESBUL nclmio, C: capturas 04-nov). Sin probar aún en PES; solo se
        usa cuando el mensaje no cabe en una trama (antes se cortaba la conexión).
        INICIO 0x0070: [total u32][largo u32][zlib u8][4 bytes 0][trozo]
        SIGUE 0x0071 / ÚLTIMO 0x0072: [largo u32][trozo]."""
        trozos = [comprimido[i:i + self.TROZO] for i in range(0, len(comprimido), self.TROZO)]
        salida = b""
        for n, t in enumerate(trozos):
            if n == 0:
                interno = struct.pack(">IIB", len(comprimido), len(t), 1) + b"\0" * 4 + t
                tipo = self.tipos_troceado[0]
            else:
                interno = struct.pack(">I", len(t)) + t
                tipo = self.tipos_troceado[2] if n == len(trozos) - 1 else self.tipos_troceado[1]
            interno += b"\0" * (-len(interno) % 8)
            salida += self._trama(tipo, secuencia, self._enc(interno))
        return salida

    def _sellar(self, datos, comprimir):
        cuerpo = zlib.compress(datos) if comprimir else datos
        interno = struct.pack(">IIB", len(cuerpo), len(datos), 1 if comprimir else 0) + cuerpo
        interno += b"\0" * (-len(interno) % 8)
        return self._enc(interno)

    def abrir(self, tipo, secuencia, md5, cifrado):
        """Comprueba MD5 y descifra: devuelve el bloque interno en claro."""
        cab = struct.pack(">HHI", tipo, len(cifrado), secuencia)
        if hashlib.md5(cab + cifrado).digest() != md5:
            raise ErrorTrama("MD5 incorrecto")
        if len(cifrado) % 8:
            raise ErrorTrama("cifrado no es múltiplo de 8")
        return self._dec(cifrado)

    def descifrar(self, tipo, secuencia, md5, cifrado):
        interno = self.abrir(tipo, secuencia, md5, cifrado)
        cod, orig, flag = struct.unpack(">IIB", interno[:9])
        cuerpo = interno[9:9 + cod]
        datos = zlib.decompress(cuerpo) if flag else cuerpo
        if not flag and datos[:1] == b"\x78":
            # C (04-nov): llegó un «mensaje» que era el entero 120 = 0x78, cabecera zlib con el
            # indicador de compresión a 0: se intenta descomprimir
            try:
                datos = zlib.decompress(datos)
            except zlib.error:
                pass
        if len(datos) != orig and not (not flag and len(datos) > len(cuerpo)):
            raise ErrorTrama(f"tamaño original {orig} != {len(datos)}")
        return msgpack_min.unpackb(datos)


# L (PESBUL nclmio, 04-nov): mensajes grandes del cliente llegan en varias tramas.
# INICIO: [total u32][largo_trozo u32][zlib u8][4 bytes][trozo]; SIGUE: [largo_trozo u32][trozo].
# Las terminales cierran el mensaje. La respuesta va con el tipo normal 0x0060.
TIPOS_INICIO = {0x0070, 0x1070}
TIPOS_SIGUE = {0x0071, 0x0072, 0x1071, 0x1072}
TIPOS_FIN = {0x0072, 0x1072}


class LectorTramas:
    """Arma tramas completas aunque TCP las parta o las pegue (fallo de Nikow5)."""

    def __init__(self, codec):
        self.codec = codec
        self._buf = bytearray()
        self._seg = None  # (secuencia de inicio, total, zlib, bytearray)

    def _segmento(self, tipo, secuencia, interno):
        """Devuelve el mensaje cuando el troceado se completa; si no, None."""
        if tipo in TIPOS_INICIO:
            total, largo, flag = struct.unpack(">IIB", interno[:9])
            self._seg = (secuencia, total, flag, bytearray(interno[13:13 + largo]))
        else:
            if self._seg is None:
                # L (PESBUL «orphan»): continuación sin inicio = se ignora sin cortar la conexión
                import logging
                logging.getLogger("phoenix.nclmio").warning("continuación 0x%04x sin inicio", tipo)
                return None
            (largo,) = struct.unpack(">I", interno[:4])
            self._seg[3].extend(interno[4:4 + largo])
        inicio, total, flag, datos = self._seg
        if len(datos) >= total or tipo in TIPOS_FIN:
            self._seg = None
            crudo = bytes(datos[:total])
            if flag or crudo[:1] == b"\x78":
                try:
                    crudo = zlib.decompress(crudo)
                except zlib.error:
                    if flag:
                        raise
            return inicio, msgpack_min.unpackb(crudo)
        return None

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
            if tipo in TIPOS_INICIO or tipo in TIPOS_SIGUE:
                listo = self._segmento(tipo, secuencia,
                                       self.codec.abrir(tipo, secuencia, trama[8:24], trama[24:]))
                if listo:
                    salida.append((TIPO_USUAL, listo[0], listo[1]))  # responde con la secuencia del inicio
                continue
            msg = self.codec.descifrar(tipo, secuencia, trama[8:24], trama[24:])
            salida.append((tipo, secuencia, msg))
        return salida
