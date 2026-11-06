"""Descifra una captura de red (.pcap/.pcapng) de PES <-> servidor y la vuelve JSONL legible.

Uso:
    python tools/descifrar_captura.py captura.pcapng salida.jsonl [--config config.json] [--puertos 10000,10001]
    python tools/descifrar_captura.py --autotest        (prueba interna sin red; usa la clave local)

- La clave NclMio se lee de `clave_nclmio` en config.json (ruta local). NUNCA se imprime ni se guarda.
- Solo IPv4/TCP sobre Ethernet. Reensambla cada flujo por número de secuencia TCP y arma tramas NclMio con LectorTramas.
- Los puertos HTTP (5000/80) se vuelcan como texto (línea de petición + largo del cuerpo), sin descifrar.
Cada línea JSONL: {"t", "flujo", "dir" (cliente->servidor = "c2s", al revés "s2c"), "tipo", "seq", "msgid", "campos"}.
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from phoenix_online import nclmio  # noqa: E402


# ----------------------------------------------------------------------------- lectura de pcap / pcapng
def leer_paquetes(ruta):
    """Devuelve [(timestamp_segundos, link_type, bytes)] de un pcap clásico o pcapng."""
    datos = open(ruta, "rb").read()
    if datos[:4] == b"\x0a\x0d\x0d\x0a":
        return _pcapng(datos)
    if datos[:4] in (b"\xd4\xc3\xb2\xa1", b"\xa1\xb2\xc3\xd4", b"\x4d\x3c\xb2\xa1", b"\xa1\xb2\x3c\x4d"):
        return _pcap(datos)
    raise ValueError("formato de captura no reconocido (se espera .pcap o .pcapng)")


def _pcap(d):
    little = d[:4] in (b"\xd4\xc3\xb2\xa1", b"\x4d\x3c\xb2\xa1")
    nano = d[:4] in (b"\x4d\x3c\xb2\xa1", b"\xa1\xb2\x3c\x4d")
    e = "<" if little else ">"
    link = struct.unpack(e + "I", d[20:24])[0]
    pos, out = 24, []
    while pos + 16 <= len(d):
        s, f, incl, _orig = struct.unpack(e + "IIII", d[pos:pos + 16])
        pos += 16
        out.append((s + f / (1e9 if nano else 1e6), link, d[pos:pos + incl]))
        pos += incl
    return out


def _pcapng(d):
    pos, e, ifaces, out = 0, "<", [], []
    while pos + 12 <= len(d):
        tipo = struct.unpack(e + "I", d[pos:pos + 4])[0]
        if tipo == 0x0A0D0D0A:                       # SHB: detecta endianness
            e = "<" if d[pos + 8:pos + 12] == b"\x4d\x3c\x2b\x1a" else ">"
        largo = struct.unpack(e + "I", d[pos + 4:pos + 8])[0]
        cuerpo = d[pos + 8:pos + largo - 4]
        if tipo == 1:                                # IDB
            link = struct.unpack(e + "H", cuerpo[0:2])[0]
            res = 1e-6                               # if_tsresol por defecto
            op = 8
            while op + 4 <= len(cuerpo):
                cod, ln = struct.unpack(e + "HH", cuerpo[op:op + 4])
                if cod == 0:
                    break
                if cod == 9:
                    v = cuerpo[op + 4]
                    res = 2.0 ** -(v & 0x7F) if v & 0x80 else 10.0 ** -v
                op += 4 + ((ln + 3) & ~3)
            ifaces.append((link, res))
        elif tipo == 6:                              # EPB
            idx, th, tl, cap, _orig = struct.unpack(e + "IIIII", cuerpo[0:20])
            link, res = ifaces[idx] if idx < len(ifaces) else (1, 1e-6)
            out.append((((th << 32) | tl) * res, link, cuerpo[20:20 + cap]))
        elif tipo == 3:                              # SPB
            link, _ = ifaces[0] if ifaces else (1, 1e-6)
            out.append((0.0, link, cuerpo[4:]))
        pos += largo
    return out


# ----------------------------------------------------------------------------- IPv4 / TCP
def tcp_de(link, b):
    """(src, sport, dst, dport, seq, flags, payload) o None."""
    if link == 1:                                    # Ethernet
        if len(b) < 14:
            return None
        et, off = struct.unpack(">H", b[12:14])[0], 14
        while et in (0x8100, 0x88A8) and len(b) >= off + 4:   # VLAN
            et, off = struct.unpack(">H", b[off + 2:off + 4])[0], off + 4
        if et != 0x0800:
            return None
        b = b[off:]
    elif link == 101:                                # RAW IP
        pass
    else:
        return None
    if len(b) < 20 or b[0] >> 4 != 4 or b[9] != 6:
        return None
    ihl = (b[0] & 15) * 4
    total = struct.unpack(">H", b[2:4])[0]
    ip = b[:total] if 0 < total <= len(b) else b
    t = ip[ihl:]
    if len(t) < 20:
        return None
    sport, dport, seq = struct.unpack(">HHI", t[0:8])
    off = (t[12] >> 4) * 4
    flags = t[13]
    src = ".".join(str(x) for x in ip[12:16])
    dst = ".".join(str(x) for x in ip[16:20])
    return src, sport, dst, dport, seq, flags, t[off:]


class Flujo:
    """Reensambla un sentido de una conexión TCP por número de secuencia."""

    def __init__(self):
        self.sig = None
        self.pend = {}

    def poner(self, seq, flags, payload):
        if flags & 0x02:                             # SYN
            self.sig = (seq + 1) & 0xFFFFFFFF
            return b""
        if not payload:
            return b""
        if self.sig is None:
            self.sig = seq
        salida = bytearray()
        self.pend[seq] = payload
        while True:
            hallado = False
            for s in list(self.pend):
                p = self.pend[s]
                dif = (self.sig - s) & 0xFFFFFFFF
                if dif > 0x7FFFFFFF:                 # segmento futuro: esperar
                    continue
                if dif >= len(p):                    # retransmisión ya vista
                    del self.pend[s]
                    hallado = True
                    continue
                salida += p[dif:]
                self.sig = (s + len(p)) & 0xFFFFFFFF
                del self.pend[s]
                hallado = True
            if not hallado:
                break
        return bytes(salida)


# ----------------------------------------------------------------------------- proceso
def procesar(paquetes, codec, puertos_nclmio, puertos_http, salida):
    flujos = {}      # (src,sport,dst,dport) -> (Flujo, LectorTramas|None)
    cuenta = 0
    for t, link, b in paquetes:
        r = tcp_de(link, b)
        if not r:
            continue
        src, sport, dst, dport, seq, flags, payload = r
        if sport in puertos_nclmio:
            servidor, cliente, sentido = (src, sport), (dst, dport), "s2c"
        elif dport in puertos_nclmio:
            servidor, cliente, sentido = (dst, dport), (src, sport), "c2s"
        elif sport in puertos_http or dport in puertos_http:
            sentido = "s2c" if sport in puertos_http else "c2s"
            servidor = (src, sport) if sentido == "s2c" else (dst, dport)
            cliente = (dst, dport) if sentido == "s2c" else (src, sport)
        else:
            continue
        clave = (src, sport, dst, dport)
        if clave not in flujos:
            esn = servidor[1] in puertos_nclmio
            flujos[clave] = (Flujo(), nclmio.LectorTramas(codec) if esn else None)
        fl, lector = flujos[clave]
        datos = fl.poner(seq, flags, payload)
        if not datos:
            continue
        nombre = f"{cliente[0]}:{cliente[1]}<->puerto {servidor[1]}"
        if lector is not None:
            try:
                for tipo, sq, msg in lector.alimentar(datos):
                    salida.write(json.dumps({"t": round(t, 3), "flujo": nombre, "dir": sentido, "tipo": tipo,
                                             "seq": sq, "msgid": msg.get("msgid") if isinstance(msg, dict) else None,
                                             "campos": msg}, ensure_ascii=False, default=str) + "\n")
                    cuenta += 1
            except Exception as ex:                  # MD5/zlib mal: tramas ajenas o clave distinta
                salida.write(json.dumps({"t": round(t, 3), "flujo": nombre, "dir": sentido,
                                         "error": f"{type(ex).__name__}: {ex}", "bytes": len(datos)}) + "\n")
        else:
            linea = datos.split(b"\r\n", 1)[0][:200].decode("latin-1", "replace")
            salida.write(json.dumps({"t": round(t, 3), "flujo": nombre, "dir": sentido, "http": linea,
                                     "bytes": len(datos), "hex_inicio": datos[:48].hex()}) + "\n")
            cuenta += 1
    return cuenta


def cargar_codec(ruta_config):
    cfg = json.load(open(ruta_config, encoding="utf-8"))
    ruta = cfg["clave_nclmio"]
    clave = open(ruta, "rb").read()
    return nclmio.Codec(clave, blowfish_le=bool(cfg.get("blowfish_le", False)))


# ----------------------------------------------------------------------------- autoprueba
def _pcap_sintetico(codec, mensajes):
    """Arma un pcap clásico con TCP al puerto 10001 que lleva `mensajes` (ida y vuelta), partido en trozos."""
    def eth_ip_tcp(src, sport, dst, dport, seq, flags, payload):
        tcp = struct.pack(">HHIIBBHHH", sport, dport, seq, 0, 5 << 4, flags, 65535, 0, 0) + payload
        ip = struct.pack(">BBHHHBBH4s4s", 0x45, 0, 20 + len(tcp), 0, 0, 64, 6, 0, bytes(src), bytes(dst)) + tcp
        return b"\x00" * 12 + b"\x08\x00" + ip

    cli, srv = (10, 0, 0, 2), (10, 0, 0, 1)
    pk, t, sc, ss = [], 0.0, 1000, 5000
    pk.append((t, eth_ip_tcp(cli, 50000, srv, 10001, sc - 1, 0x02, b"")))
    for i, (de_cliente, m) in enumerate(mensajes):
        trama = codec.codificar(i, m)
        for trozo in (trama[:10], trama[10:]):       # fuerza el reensamblado
            t += 0.01
            if de_cliente:
                pk.append((t, eth_ip_tcp(cli, 50000, srv, 10001, sc, 0x18, trozo)))
                sc += len(trozo)
            else:
                pk.append((t, eth_ip_tcp(srv, 10001, cli, 50000, ss, 0x18, trozo)))
                ss += len(trozo)
    cab = struct.pack("<IHHiIII", 0xA1B2C3D4, 2, 4, 0, 0, 65535, 1)
    cuerpo = b"".join(struct.pack("<IIII", int(x), int((x % 1) * 1e6), len(p), len(p)) + p for x, p in pk)
    return cab + cuerpo


def autoprueba(ruta_config):
    import io
    import tempfile
    codec = cargar_codec(ruta_config)
    msgs = [(True, {"msgid": "CMD_GET_CHALLENGECODE", "rqid": 1}),
            (False, {"msgid": "CMD_GET_CHALLENGECODE", "rqid": 1, "result": "NOERR", "challenge_code": "ABC"}),
            (True, {"msgid": "CMD_LOGIN", "rqid": 2, "username": "3a660"})]
    tmp = os.path.join(tempfile.mkdtemp(), "t.pcap")
    open(tmp, "wb").write(_pcap_sintetico(codec, msgs))
    sal = io.StringIO()
    n = procesar(leer_paquetes(tmp), codec, {10000, 10001}, {5000, 80}, sal)
    filas = [json.loads(x) for x in sal.getvalue().splitlines()]
    ok = n == 3 and [f["msgid"] for f in filas] == ["CMD_GET_CHALLENGECODE", "CMD_GET_CHALLENGECODE", "CMD_LOGIN"] \
        and [f["dir"] for f in filas] == ["c2s", "s2c", "c2s"]
    print("AUTOPRUEBA", "OK" if ok else "FALLO", n, "mensajes")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("captura", nargs="?")
    ap.add_argument("salida", nargs="?")
    ap.add_argument("--config", default="config.json")
    ap.add_argument("--puertos", default="10000,10001", help="puertos NclMio (TCP cifrado)")
    ap.add_argument("--http", default="5000,80", help="puertos HTTP en claro")
    ap.add_argument("--autotest", action="store_true")
    a = ap.parse_args()
    if a.autotest:
        return autoprueba(a.config)
    if not (a.captura and a.salida):
        ap.error("falta captura y salida")
    codec = cargar_codec(a.config)
    puertos = {int(x) for x in a.puertos.split(",") if x}
    http = {int(x) for x in a.http.split(",") if x}
    with open(a.salida, "w", encoding="utf-8") as f:
        n = procesar(leer_paquetes(a.captura), codec, puertos, http, f)
    print(f"{n} mensajes escritos en {a.salida}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
