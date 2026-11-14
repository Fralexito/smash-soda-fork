"""MessagePack mínimo, sin dependencias.

Hecho a mano para no descargar paquetes y para tolerar textos que no son
UTF-8 (cap_72 §11.2 punto 8): esos se devuelven como bytes en vez de fallar.
"""
import struct


class Empaquetado:
    """Un mapa que viaja como `bin` con otro MessagePack dentro (cap_72 §11.2 punto 9)."""

    __slots__ = ("valor",)

    def __init__(self, valor):
        self.valor = valor

    def __repr__(self):
        return f"Empaquetado({self.valor!r})"


def packb(obj):
    out = bytearray()
    _pack(obj, out)
    return bytes(out)


def _pack(o, out):
    if o is None:
        out.append(0xC0)
    elif o is True:
        out.append(0xC3)
    elif o is False:
        out.append(0xC2)
    elif isinstance(o, int):
        _pack_int(o, out)
    elif isinstance(o, float):
        out.append(0xCB)
        out += struct.pack(">d", o)
    elif isinstance(o, str):
        b = o.encode("utf-8")
        n = len(b)
        if n < 32:
            out.append(0xA0 | n)
        elif n < 0x100:
            out += bytes((0xD9, n))
        elif n < 0x10000:
            out.append(0xDA)
            out += struct.pack(">H", n)
        else:
            out.append(0xDB)
            out += struct.pack(">I", n)
        out += b
    elif isinstance(o, Empaquetado):
        _pack_bin(packb(o.valor), out)
    elif isinstance(o, (bytes, bytearray)):
        _pack_bin(bytes(o), out)
    elif isinstance(o, (list, tuple)):
        n = len(o)
        if n < 16:
            out.append(0x90 | n)
        elif n < 0x10000:
            out.append(0xDC)
            out += struct.pack(">H", n)
        else:
            out.append(0xDD)
            out += struct.pack(">I", n)
        for x in o:
            _pack(x, out)
    elif isinstance(o, dict):
        n = len(o)
        if n < 16:
            out.append(0x80 | n)
        elif n < 0x10000:
            out.append(0xDE)
            out += struct.pack(">H", n)
        else:
            out.append(0xDF)
            out += struct.pack(">I", n)
        for k, v in o.items():
            _pack(k, out)
            _pack(v, out)
    else:
        raise TypeError(f"tipo no soportado: {type(o).__name__}")


def _pack_int(i, out):
    if 0 <= i < 0x80:
        out.append(i)
    elif -32 <= i < 0:
        out.append(i & 0xFF)
    elif i >= 0:
        for tag, fmt, lim in ((0xCC, ">B", 0x100), (0xCD, ">H", 0x10000),
                              (0xCE, ">I", 1 << 32), (0xCF, ">Q", 1 << 64)):
            if i < lim:
                out.append(tag)
                out += struct.pack(fmt, i)
                return
        raise OverflowError(i)
    else:
        for tag, fmt, lim in ((0xD0, ">b", 1 << 7), (0xD1, ">h", 1 << 15),
                              (0xD2, ">i", 1 << 31), (0xD3, ">q", 1 << 63)):
            if i >= -lim:
                out.append(tag)
                out += struct.pack(fmt, i)
                return
        raise OverflowError(i)


def _pack_bin(b, out):
    n = len(b)
    if n < 0x100:
        out += bytes((0xC4, n))
    elif n < 0x10000:
        out.append(0xC5)
        out += struct.pack(">H", n)
    else:
        out.append(0xC6)
        out += struct.pack(">I", n)
    out += b


def unpackb(data):
    obj, pos = _unpack(memoryview(bytes(data)), 0)
    return obj


def _texto(b):
    try:
        return bytes(b).decode("utf-8")
    except UnicodeDecodeError:
        return bytes(b)  # tolerante: no cerrar la sesión por un nombre raro


def _unpack(m, p):
    t = m[p]
    p += 1
    if t < 0x80:
        return t, p
    if t >= 0xE0:
        return t - 0x100, p
    if 0x80 <= t <= 0x8F:
        return _map(m, p, t & 0x0F)
    if 0x90 <= t <= 0x9F:
        return _arr(m, p, t & 0x0F)
    if 0xA0 <= t <= 0xBF:
        n = t & 0x1F
        return _texto(m[p:p + n]), p + n
    if t == 0xC0:
        return None, p
    if t == 0xC2:
        return False, p
    if t == 0xC3:
        return True, p
    if t in (0xC4, 0xC5, 0xC6):
        w = {0xC4: 1, 0xC5: 2, 0xC6: 4}[t]
        n = int.from_bytes(m[p:p + w], "big")
        p += w
        return bytes(m[p:p + n]), p + n
    if t == 0xCA:
        return struct.unpack(">f", m[p:p + 4])[0], p + 4
    if t == 0xCB:
        return struct.unpack(">d", m[p:p + 8])[0], p + 8
    ints = {0xCC: ">B", 0xCD: ">H", 0xCE: ">I", 0xCF: ">Q",
            0xD0: ">b", 0xD1: ">h", 0xD2: ">i", 0xD3: ">q"}
    if t in ints:
        f = ints[t]
        w = struct.calcsize(f)
        return struct.unpack(f, m[p:p + w])[0], p + w
    if t in (0xD9, 0xDA, 0xDB):
        w = {0xD9: 1, 0xDA: 2, 0xDB: 4}[t]
        n = int.from_bytes(m[p:p + w], "big")
        p += w
        return _texto(m[p:p + n]), p + n
    if t in (0xDC, 0xDD):
        w = 2 if t == 0xDC else 4
        return _arr(m, p + w, int.from_bytes(m[p:p + w], "big"))
    if t in (0xDE, 0xDF):
        w = 2 if t == 0xDE else 4
        return _map(m, p + w, int.from_bytes(m[p:p + w], "big"))
    raise ValueError(f"tipo MessagePack no soportado: 0x{t:02X}")


def _arr(m, p, n):
    out = []
    for _ in range(n):
        v, p = _unpack(m, p)
        out.append(v)
    return out, p


def _map(m, p, n):
    out = {}
    for _ in range(n):
        k, p = _unpack(m, p)
        v, p = _unpack(m, p)
        if isinstance(k, bytes):
            k = k.decode("latin-1")
        out[k] = v
    return out, p
