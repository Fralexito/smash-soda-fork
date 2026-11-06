"""Prueba variantes de descifrado NclMio sobre una trama cruda capturada.

Uso (en la PC, la clave nunca sale de ahí):
    python -I -m phoenix_online.probar_variantes trama.bin nclmio_key.bin

trama.bin = bytes tal cual llegaron por TCP (primera trama, con XOR).
Imprime solo las variantes cuyo texto descifrado contiene «msgid» o «CMD_»,
con los primeros bytes en hex. No imprime la clave.
"""
import hashlib
import itertools
import struct
import sys
import zlib

from cryptography.hazmat.decrepit.ciphers.algorithms import Blowfish
from cryptography.hazmat.primitives.ciphers import Cipher, modes

from .nclmio import XOR_CLAVE, verificar_clave


def xor(data, clave):
    return bytes(b ^ clave[i % len(clave)] for i, b in enumerate(data))


def swap32(b):
    return b"".join(b[i:i + 4][::-1] for i in range(0, len(b) - len(b) % 4, 4)) + b[len(b) - len(b) % 4:]


def descifrar(clave, datos, le, modo):
    datos = datos[: len(datos) - len(datos) % 8]
    if le:
        datos = swap32(datos)
    m = modes.ECB() if modo == "ECB" else modes.CBC(b"\0" * 8)
    d = Cipher(Blowfish(clave), m).decryptor()
    out = d.update(datos) + d.finalize()
    return swap32(out) if le else out


def main(ruta_trama, ruta_clave):
    trama = open(ruta_trama, "rb").read()
    clave_ok = open(ruta_clave, "rb").read()
    print(f"trama {len(trama)} B; clave válida: {verificar_clave(clave_ok)}")
    claves = {"clave": clave_ok, "clave[:16]": clave_ok[:16], "clave[::-1]": clave_ok[::-1],
              "clave swap32": swap32(clave_ok)}
    aciertos = 0
    for usar_xor in (True, False):
        t = xor(trama, XOR_CLAVE) if usar_xor else trama
        tipo, largo, seq = struct.unpack(">HHI", t[:8])
        md5_ok = hashlib.md5(t[:8] + t[24:24 + largo]).digest() == t[8:24]
        print(f"xor={usar_xor}: tipo=0x{tipo:04x} largo={largo} seq={seq} md5_ok={md5_ok}")
        for inicio in (24, 8, 0):
            cuerpo = t[inicio:]
            for (nombre, k), le, modo in itertools.product(claves.items(), (False, True), ("ECB", "CBC")):
                try:
                    p = descifrar(k, cuerpo, le, modo)
                except ValueError:
                    continue
                if b"msgid" in p or b"CMD_" in p:
                    aciertos += 1
                    i = p.find(b"msgid") if b"msgid" in p else p.find(b"CMD_")
                    print(f"  ACIERTO xor={usar_xor} inicio={inicio} {nombre} le={le} {modo} "
                          f"pos={i} cabeza={p[:24].hex()}")
                    print(f"    BE: {struct.unpack('>IIB', p[:9])}  LE: {struct.unpack('<IIB', p[:9])}")
                for off in (9, 8, 10, 12, 0):
                    for wb in (15, -15, 47):
                        try:
                            z = zlib.decompressobj(wb).decompress(p[off:])
                        except zlib.error:
                            continue
                        if b"msgid" in z or b"CMD_" in z:
                            aciertos += 1
                            print(f"  ACIERTO-ZLIB xor={usar_xor} inicio={inicio} {nombre} le={le} "
                                  f"{modo} off={off} wbits={wb} cabeza={p[:16].hex()} z={z[:24].hex()}")
    if not aciertos:
        print("ninguna variante muestra msgid/CMD_: probablemente otro esquema (Enhanced) "
              "o zlib sin cabecera; revisar DivBufFor*EncryptionVer")


if __name__ == "__main__":
    main(*sys.argv[1:3])
