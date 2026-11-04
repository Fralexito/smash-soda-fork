"""Relé UDP de juego (gamerelay) de PES 2021, fase 1: la «sonda de calidad».

Todo según docs/ESPEC_GAMERELAY.md (§2, §2b, §3, §4), escrito por nosotros. Las claves
NO van aquí: se leen de un archivo local (cfg["clave_gamerelay"], 272 bytes = 256 de la
clave exterior + 16 de la interior), que vive solo en la PC del servidor.

Marcas como en logica.py: C = comprobado en vivo, L = leído en la referencia, P = suposición.
Fase 1 = lo que PES necesita para aprobar «Comprobando entorno de red» con un solo jugador:
KX, apretón del canal 0, abrir canales (canal 1), lista de participantes (canal 5: servidor
+ yo) y eco de calidad (canal 6). El reenvío del partido entre dos jugadores es la fase 3.
"""

import asyncio
import logging
import os
import struct
import time

log = logging.getLogger("phoenix.gamerelay")

IV0 = 0xEA
ID_SERVIDOR = 31
CANAL_CONTROL, CANAL_FLUJOS, CANAL_LISTA, CANAL_CALIDAD = 0, 1, 5, 6
CANALES_JUEGO = range(9, 16)

# canal 0 (L): SYN = 0x80, SYN+ACK = 0x90, ACK = 0x10; vida: sonda 0x08 -> respuesta 0x04
SYN, SYN_ACK, ACK, VIDA, VIDA_RESP = 0x80, 0x90, 0x10, 0x08, 0x04
# canal 1 (L): [op][K]
ABRIR, ABRIR_OK, CERRAR, CERRAR_OK, RESET = 0x80, 0x90, 0x40, 0x50, 0x20


# ---------------- cifrado ----------------
def _rc4_flujo(clave, n):
    s = list(range(256))
    j = 0
    for i in range(256):
        j = (j + s[i] + clave[i % len(clave)]) & 0xFF
        s[i], s[j] = s[j], s[i]
    i = j = 0
    out = bytearray(n)
    for k in range(n):
        i = (i + 1) & 0xFF
        j = (j + s[i]) & 0xFF
        s[i], s[j] = s[j], s[i]
        out[k] = s[(s[i] + s[j]) & 0xFF]
    return out


def rc4(clave, datos):
    """RC4 simple (KSA nuevo cada vez)."""
    ks = _rc4_flujo(clave, len(datos))
    return bytes(a ^ b for a, b in zip(datos, ks))


def cifrar_exterior(clave, plano):
    """L §2b-A: RC4 con KSA nuevo por datagrama + XOR con el byte cifrado anterior (IV 0xEA)."""
    ks = _rc4_flujo(clave, len(plano))
    iv, out = IV0, bytearray(len(plano))
    for k, p in enumerate(plano):
        c = ks[k] ^ p ^ iv
        out[k] = c
        iv = c
    return bytes(out)


def descifrar_exterior(clave, cifrado):
    ks = _rc4_flujo(clave, len(cifrado))
    iv, out = IV0, bytearray(len(cifrado))
    for k, c in enumerate(cifrado):
        out[k] = ks[k] ^ c ^ iv
        iv = c
    return bytes(out)


def suma_control(datos):
    """RFC 1071: palabras de 16 bits big-endian, acarreo plegado, complemento a uno."""
    if len(datos) % 2:
        datos += b"\x00"
    total = 0
    for i in range(0, len(datos), 2):
        total += (datos[i] << 8) | datos[i + 1]
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def armar_marco(flags, cuerpo):
    sin = bytes([flags]) + b"\x00\x00" + cuerpo
    return bytes([flags]) + struct.pack(">H", suma_control(sin)) + cuerpo


def leer_marco(plano):
    """-> (flags, cuerpo) o None si el checksum no cuadra.

    C (04-nov, 6110 datagramas reales): con el bit 6 (0x40) el marco es
    [flags][ zlib( checksum u16 BE || cuerpo ) ] y el checksum se calcula con flags sin 0x40.
    Sin el bit 6: [flags][checksum u16 BE][cuerpo]. El cliente manda así su primer SYN."""
    if len(plano) < 3:
        return None
    flags = plano[0]
    if flags & 0x40:
        import zlib
        d = zlib.decompressobj()
        try:
            out = d.decompress(plano[1:], 0x801)
        except zlib.error:
            return None
        if not d.eof or d.unused_data or d.unconsumed_tail or len(out) < 2 or len(out) >= 0x801:
            return None
        cs, cuerpo = struct.unpack(">H", out[:2])[0], out[2:]
        base = flags & ~0x40
    else:
        cs, cuerpo = struct.unpack(">H", plano[1:3])[0], plano[3:]
        base = flags
    if suma_control(bytes([base]) + b"\x00\x00" + cuerpo) != cs:
        return None
    return flags, cuerpo


# ---------------- KX (L §2b-B) ----------------
def _xorshift32(x):
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= x >> 7
    x ^= (x << 5) & 0xFFFFFFFF
    return x & 0xFFFFFFFF


def material_kx(semilla, nonce):
    x = _xorshift32(semilla & 0xFFFFFFFF)
    x = (x + x // 0xFFFFFFFF + nonce) & 0xFFFFFFFF
    m = bytearray(256)
    for k in range(256):
        x = _xorshift32(x)
        m[k] = ((x // 0xFF) + x) & 0xFF
    return bytes(m)


# ---------------- transporte (L §2) ----------------
def leer_transporte(cuerpo):
    """-> (seq, [(canal, ack, [(seq_elem, carga)], [carga])]); ValueError si sobra o falta."""
    if len(cuerpo) < 5:
        raise ValueError("cuerpo corto")
    seq = struct.unpack(">I", cuerpo[:4])[0]
    p, secciones = 5, []
    while p < len(cuerpo):
        canal = cuerpo[p] >> 4
        marca = cuerpo[p + 1]
        if marca >> 4 != canal:
            raise ValueError(f"sección rara {cuerpo[p]:02x} {marca:02x}")
        ack = struct.unpack(">I", cuerpo[p + 2:p + 6])[0]
        p += 6
        lista_a, lista_b = [], []
        if marca & 0x02:
            n = cuerpo[p] + 1
            p += 1
            for _ in range(n):
                s_elem, largo = struct.unpack(">IH", cuerpo[p:p + 6])
                largo += 1
                lista_a.append((s_elem, cuerpo[p + 6:p + 6 + largo]))
                p += 6 + largo
        if marca & 0x01:
            # C (04-nov, canal 6): lista B = [u16 BE 0x8000 | (cuenta-1)] y por elemento
            # [largo-1 u16 BE][seq u32 BE][carga]; la carga son bloques como en la lista A
            (cab,) = struct.unpack(">H", cuerpo[p:p + 2])
            n = (cab & 0x7FFF) + 1
            p += 2
            for _ in range(n):
                (largo,) = struct.unpack(">H", cuerpo[p:p + 2])
                largo += 1
                elem = cuerpo[p + 2:p + 2 + largo]
                if len(elem) != largo or largo < 4:
                    raise ValueError("elemento B corto")
                lista_b.append((struct.unpack(">I", elem[:4])[0], elem[4:]))
                p += 2 + largo
        secciones.append((canal, ack, lista_a, lista_b))
    if p != len(cuerpo):
        raise ValueError("bytes de más")
    return seq, secciones


def armar_transporte(seq, secciones):
    """secciones = [(canal, ack, [(seq_elem, carga)])] (solo lista A)."""
    out = bytearray(struct.pack(">I", seq) + b"\x00")
    for canal, ack, lista_a in secciones:
        out += bytes([canal << 4, (canal << 4) | (0x02 if lista_a else 0)]) + struct.pack(">I", ack)
        if lista_a:
            out.append(len(lista_a) - 1)
            for s_elem, carga in lista_a:
                out += struct.pack(">IH", s_elem, len(carga) - 1) + carga
    return bytes(out)


def bloques(carga):
    """Carga = ([largo u8][bytes])*."""
    p, out = 0, []
    while p < len(carga):
        n = carga[p]
        out.append(carga[p + 1:p + 1 + n])
        p += 1 + n
    return out


def carga_de(*partes):
    return b"".join(bytes([len(x)]) + x for x in partes)


class Canal:
    def __init__(self):
        self.esperado = 0      # siguiente elemento que espero (= ack que mando)
        self.siguiente = 0     # siguiente número para mis elementos
        self.pendientes = {}   # seq -> carga, hasta que el cliente confirme


class Cliente:
    def __init__(self, addr, clave):
        self.addr = addr
        self.clave = clave       # constante hasta el KX; luego M (P)
        self.clave_constante = clave
        self.seq = 0
        self.canales = {}
        self.establecido = False
        self.abiertos = set()
        self.lista_enviada = False
        self.visto = time.time()
        self.sonda = False

    def canal(self, n):
        return self.canales.setdefault(n, Canal())

    def poner(self, n, carga):
        c = self.canal(n)
        c.pendientes[c.siguiente] = carga
        c.siguiente += 1


class Rele(asyncio.DatagramProtocol):
    """Atiende el puerto UDP del nodo (5100 por defecto)."""

    VIDA_SONDA = 30  # s (C: la sonda caduca a los ~27 s)

    def __init__(self, srv, clave_exterior, clave_interior, indice=1):
        self.srv = srv
        self.k_ext, self.k_int, self.indice = clave_exterior, clave_interior, indice
        self.clientes = {}
        self.transporte = None

    @classmethod
    def desde_archivo(cls, srv, ruta, indice=1):
        with open(ruta, "rb") as f:
            datos = f.read()
        if len(datos) != 272:
            raise ValueError("clave_gamerelay debe tener 272 bytes (256 + 16)")
        return cls(srv, datos[:256], datos[256:], indice)

    def connection_made(self, transport):
        self.transporte = transport

    def enviar(self, cli, plano_marco):
        self.transporte.sendto(cifrar_exterior(cli.clave, plano_marco), cli.addr)

    # ---- entrada ----
    def datagram_received(self, data, addr):
        try:
            self._recibir(data, addr)
        except Exception:
            log.exception("gamerelay: error con %s (%d B)", addr, len(data))

    def _recibir(self, data, addr):
        cli = self.clientes.get(addr)
        marco = None
        if cli:
            marco = leer_marco(descifrar_exterior(cli.clave, data))
            if marco is None and cli.clave is not cli.clave_constante:
                marco = leer_marco(descifrar_exterior(cli.clave_constante, data))  # plan B (P)
                if marco is not None:
                    log.warning("gamerelay %s: llegó con la clave constante tras el KX", addr)
        if marco is None:
            marco = leer_marco(descifrar_exterior(self.k_ext, data))
        if marco is None:
            log.info("gamerelay: datagrama ilegible de %s (%d B) %s", addr, len(data), data[:12].hex())
            return
        flags, cuerpo = marco
        if len(cuerpo) == 4 and flags & 0x80 and not flags & 0x40:
            self._kx(addr, cuerpo)
            return
        if not cli:
            cli = self.clientes[addr] = Cliente(addr, self.k_ext)
        cli.visto = time.time()
        try:
            _, secciones = leer_transporte(cuerpo)
        except (ValueError, struct.error, IndexError) as e:
            log.info("gamerelay %s: transporte no entendido (%s) %s", addr, e, cuerpo[:32].hex())
            return
        for canal, ack, lista_a, lista_b in secciones:
            c = cli.canal(canal)
            for s_elem in [s for s in c.pendientes if s < ack]:  # ack = «siguiente que espero»
                del c.pendientes[s_elem]
            for s_elem, carga in sorted(lista_a):
                if s_elem != c.esperado:
                    continue  # repetido o desordenado: el cliente lo reenviará
                c.esperado += 1
                self._elemento(cli, canal, carga)
            for _s_b, carga in lista_b:
                self._elemento(cli, canal, carga)
        self._responder(cli)  # C: cada datagrama del cliente recibe respuesta (eco RTT)

    def _kx(self, addr, cuerpo):
        nonce = struct.unpack(">I", rc4(self.k_int, cuerpo))[0]
        semilla = struct.unpack(">I", os.urandom(4))[0]
        m = material_kx(semilla, nonce)
        respuesta = armar_marco(0x80, rc4(self.k_int, m))
        cli = self.clientes[addr] = Cliente(addr, self.k_ext)
        self.enviar(cli, respuesta)    # la respuesta va con la clave constante
        cli.clave = m                  # P: después, M como clave exterior
        log.info("gamerelay KX con %s (nonce %08x)", addr, nonce)

    def _elemento(self, cli, canal, carga):
        for b in bloques(carga):
            if canal == CANAL_CONTROL and b:
                op = b[0]
                if op & SYN and not op & ACK:
                    cli.poner(CANAL_CONTROL, carga_de(bytes([SYN_ACK])))
                elif op == ACK:
                    cli.establecido = True
                    log.info("gamerelay %s: canal 0 establecido", cli.addr)
                    self._quizas_lista(cli)
                elif op == VIDA:
                    cli.poner(CANAL_CONTROL, carga_de(bytes([VIDA_RESP])))
            elif canal == CANAL_FLUJOS and len(b) >= 2:
                op, k = b[0], b[1]
                if op == ABRIR:
                    cli.abiertos.add(k)
                    cli.poner(CANAL_FLUJOS, carga_de(bytes([ABRIR_OK, k])))
                    log.info("gamerelay %s: abre canal %d", cli.addr, k)
                    if k in CANALES_JUEGO:
                        log.info("gamerelay %s: canal de juego %d (fase 3, sin reenvío aún)", cli.addr, k)
                elif op == CERRAR:
                    cli.abiertos.discard(k)
                    cli.poner(CANAL_FLUJOS, carga_de(bytes([CERRAR_OK, k])))
            elif canal == CANAL_CALIDAD and len(b) == 5:
                cuenta = (b[0] >> 3) + 1
                cli.poner(CANAL_CALIDAD, carga_de(*([b] * cuenta)))  # L: idéntico, «cuenta» veces
            elif canal in CANALES_JUEGO:
                pass  # fase 3: reenviar a los demás de la sala

    def _quizas_lista(self, cli):
        """L §4: sin sala, solo si el HTTP/TCP autorizó la sonda: lista = servidor + yo."""
        if cli.lista_enviada:
            return
        if self.srv is not None and not self.srv.sonda_autorizada(cli.addr[0], self.indice):
            log.info("gamerelay %s: sin sonda autorizada ni sala; no se manda lista", cli.addr)
            return
        cli.sonda = cli.lista_enviada = True
        cli.poner(CANAL_LISTA, carga_de(bytes([ID_SERVIDOR, 0xC0 | 0])))
        log.info("gamerelay %s: lista de participantes (sonda)", cli.addr)

    def _responder(self, cli):
        secciones = []
        canales = {CANAL_CONTROL, CANAL_FLUJOS} | set(cli.canales)  # L: siempre 0 y 1
        for n in sorted(canales):
            c = cli.canal(n)
            secciones.append((n, c.esperado, sorted(c.pendientes.items())))
        cuerpo = armar_transporte(cli.seq, secciones)
        cli.seq += 1
        # C (04-nov): tras el KX el cliente descarta marcos con bit 7; flags = sal de 6 bits
        # (bits 6 y 7 limpios), como hace el propio cliente. Solo la respuesta KX lleva 0x80.
        self.enviar(cli, armar_marco(os.urandom(1)[0] & 0x3F, cuerpo))

    def limpiar(self):
        corte = time.time() - self.VIDA_SONDA
        for a in [a for a, c in self.clientes.items() if c.visto < corte]:
            del self.clientes[a]
