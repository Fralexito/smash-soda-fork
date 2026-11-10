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
    seq, _flags, _eco, secciones = leer_datagrama(cuerpo)
    return seq, secciones


def leer_datagrama(cuerpo):
    """-> (seq, flags, eco, secciones). L (ESPEC_GAMERELAY_FASE23 §3.2): tras [seq u32][flags u8],
    si flags & 0x40 vienen [echo_seq u32][hold_ms u8]; flags & 0x80 = «quiero eco RTT»."""
    if len(cuerpo) < 5:
        raise ValueError("cuerpo corto")
    seq = struct.unpack(">I", cuerpo[:4])[0]
    flags, p, eco = cuerpo[4], 5, None
    if flags & 0x40:
        if len(cuerpo) < 10:
            raise ValueError("eco corto")
        eco = struct.unpack(">IB", cuerpo[5:10])
        p = 10
    secciones = []
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
            # C (04-nov, canal 6): lista B = [grupos u8][cuenta-1 u8] y por elemento
            # [largo-1 u16 BE][seq u32 BE][carga]. L (FASE23 §3.2): el byte de grupos puede traer
            # 0x80 (grupo 0) y/o 0x40 (grupo 1), cada uno con su cuenta; los 6 bits bajos a 0
            grupos = cuerpo[p]
            if grupos & 0x3F or not grupos & 0xC0:
                raise ValueError(f"grupos B raros {grupos:02x}")
            p += 1
            for g in (0x80, 0x40):
                if not grupos & g:
                    continue
                n = cuerpo[p] + 1
                p += 1
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
    return seq, flags, eco, secciones


def armar_transporte(seq, secciones, eco=None):
    """secciones = [(canal, ack, [(seq_elem, carga)])] (solo lista A).
    eco = (echo_seq, hold_ms): flags 0x40 + esos 5 bytes (respuesta al eco RTT del cliente)."""
    out = bytearray(struct.pack(">I", seq))
    if eco is not None:
        out += bytes([0x40]) + struct.pack(">IB", eco[0] & 0xFFFFFFFF, max(0, min(255, int(eco[1]))))
    else:
        out.append(0)
    for canal, ack, lista_a in secciones:
        out += bytes([canal << 4, (canal << 4) | (0x02 if lista_a else 0)]) + struct.pack(">I", ack)
        if lista_a:
            out.append(len(lista_a) - 1)
            for s_elem, carga in lista_a:
                out += struct.pack(">IH", s_elem, len(carga) - 1) + carga
    return bytes(out)


def tamano_seccion(lista_a):
    return 6 + (1 + sum(6 + len(c) for _, c in lista_a) if lista_a else 0)


def trozos(datos, maximo=255):
    """Parte un mensaje en bloques de ≤255 bytes (la anchura del largo de cada bloque)."""
    return [datos[i:i + maximo] for i in range(0, len(datos), maximo)] or [b""]


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


CANAL_ORDENES = 9                 # entrada de mandos (lockstep): se reenvía a todos, también al emisor
CANALES_FLUJO = (10, 12, 15)      # carga, partido y GameSeq: se reenvían al otro con cabecera 0x4000|id<<4
MAX_CUERPO = 0x800                # L (FASE23 §3.2): cuerpo máximo por datagrama
MAX_ELEMENTOS = 256               # por lista A
MAX_FUTUROS = 4096                # claves guardadas por delante de la esperada (protección de memoria)
ID_RESERVADO = ID_SERVIDOR
VIDA_PARTIDA = 90                 # s sin datos antes de olvidar a un jugador ligado (L: PEER_TTL_SEC)


class Canal:
    def __init__(self):
        self.esperado = 0      # siguiente clave que espero (= ack que mando)
        self.siguiente = 0     # siguiente clave para mis bloques
        self.pendientes = {}   # clave -> carga, hasta que el cliente confirme
        self.futuros = {}      # clave -> carga recibida por delante (reordenación)


class Cliente:
    def __init__(self, addr, clave):
        self.addr = addr
        self.clave = clave       # constante hasta el KX; luego M
        self.clave_constante = clave
        self.seq = 0
        self.canales = {}
        self.establecido = False
        self.abiertos = set()
        self.lista_enviada = False
        self.visto = time.time()
        self.sonda = False
        self.intento = None      # Intento al que quedó ligado (partida real)
        self.pid = None          # clave de la persona (la misma que usa el servidor TCP)
        self.id_rele = None      # id de 5 bits en la lista del canal 5
        self.ecos = []           # [(dgseq, instante)] pedidos de eco RTT pendientes (máx. 16)
        self.vistos_juego = 0    # mensajes de juego anotados en el registro (diagnóstico)

    def canal(self, n):
        return self.canales.setdefault(n, Canal())

    def poner(self, n, carga):
        """Encola un elemento; L (FASE23 §3.2): cada BLOQUE consume una clave ARQ."""
        c = self.canal(n)
        c.pendientes[c.siguiente] = carga
        c.siguiente += max(1, len(bloques(carga)))


class Intento:
    """Una partida de 2 (o más) jugadores ligada al relé (L FASE23 §1.5): lista congelada."""

    def __init__(self, room_id, miembros, generacion=1):
        self.room_id = room_id
        self.generacion = generacion
        # miembros: [{"pid", "enter_no", "endpoints": callable -> [(ip, puerto)], "ip_tcp"}]
        self.miembros = sorted(miembros, key=lambda x: (int(x["enter_no"]), str(x["pid"])))
        self.ids = {str(x["pid"]): i for i, x in enumerate(self.miembros)}
        self.pares = {}          # pid -> Cliente ligado
        self.salidos = set()
        self.contados = {}       # (pid, canal, bytes) -> [nativo, tcp] para no entregar dos veces
        self.reenviados = 0
        self.creado = time.time()

    def lista_para(self, pid):
        """L §2.1: [servidor 1F][yo C0|id][otros 80|id …] en orden de id."""
        yo = self.ids[str(pid)]
        return bytes([ID_RESERVADO, 0xC0 | yo] + [0x80 | i for p, i in sorted(self.ids.items(), key=lambda kv: kv[1])
                                                  if i != yo])

    def contar(self, pid, canal, datos, via):
        """Devuelve True si hay que entregar (primera copia de esa ocurrencia por cualquier vía)."""
        k = (str(pid), canal, bytes(datos))
        n = self.contados.get(k)
        if n is None:
            if len(self.contados) >= 4096:
                self.contados.pop(next(iter(self.contados)))
            n = self.contados[k] = [0, 0]
        i = 0 if via == "nativo" else 1
        n[i] += 1
        return n[i] > n[1 - i]


def endpoint_de_ntl(v):
    """sa_data de CMD_SEND_IPANDPORT_NTL -> (ip, puerto) o None.

    L (FASE23 §1.1): mapa con sa_data = 14 bytes en base64: puerto u16 BE, 2 de relleno, IPv4,
    6 ceros. P: si con ese orden sale 0.0.0.0 se prueba el orden clásico de sockaddr_in."""
    import base64
    try:
        if isinstance(v, (bytes, bytearray)):
            from . import msgpack_min
            v = msgpack_min.unpackb(bytes(v))
        if hasattr(v, "valor"):
            v = v.valor
        if not isinstance(v, dict):
            return None
        sa = v.get("sa_data")
        if isinstance(sa, str):
            sa = base64.b64decode(sa + "=" * (-len(sa) % 4))
        if not isinstance(sa, (bytes, bytearray)) or len(sa) < 8:
            return None
        puerto = struct.unpack(">H", sa[:2])[0]
        ip = sa[4:8]
        if ip == b"\x00\x00\x00\x00" and sa[2:6] != b"\x00\x00\x00\x00":
            ip = sa[2:6]
        if not puerto or ip == b"\x00\x00\x00\x00":
            return None
        return ".".join(str(b) for b in ip), puerto
    except Exception:
        return None


class Rele(asyncio.DatagramProtocol):
    """Atiende el puerto UDP del nodo (5100 por defecto)."""

    VIDA_SONDA = 10  # s (L: PESBUL borra la sonda tras 10 s sin datos; el cliente vuelve a medir)

    def __init__(self, srv, clave_exterior, clave_interior, indice=1):
        self.srv = srv
        self.k_ext, self.k_int, self.indice = clave_exterior, clave_interior, indice
        self.clientes = {}
        self.intentos = {}       # room_id -> Intento
        self.transporte = None
        self.empuje_inmediato = True

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
        if self.transporte is not None:
            self.transporte.sendto(cifrar_exterior(cli.clave, plano_marco), cli.addr)

    # ---- intentos de partida (los abre y cierra el servidor TCP) ----
    def abrir_intento(self, room_id, miembros):
        viejo = self.intentos.get(room_id)
        gen = viejo.generacion + 1 if viejo else 1
        if viejo:
            self.cerrar_intento(room_id)
        it = self.intentos[room_id] = Intento(room_id, miembros, gen)
        log.info("gamerelay: partida %s abierta (gen %d) con %s", room_id, gen,
                 [(m["pid"], m["enter_no"]) for m in it.miembros])
        return it

    def cerrar_intento(self, room_id):
        it = self.intentos.pop(room_id, None)
        if not it:
            return
        for cli in it.pares.values():
            if cli.intento is it:
                cli.intento = cli.pid = cli.id_rele = None
        log.info("gamerelay: partida %s cerrada (%d mensajes reenviados)", room_id, it.reenviados)

    def miembro_salio(self, room_id, pid):
        """L §4.3: se olvida su estado de par; sin aviso UDP al otro. Si ya no queda nadie, se cierra."""
        it = self.intentos.get(room_id)
        if not it:
            return
        it.salidos.add(str(pid))
        cli = it.pares.pop(str(pid), None)
        if cli is not None:
            cli.intento = cli.pid = cli.id_rele = None
        if it.salidos >= set(it.ids):
            self.cerrar_intento(room_id)

    def _ligar(self, cli):
        """L §1.2: endpoint exacto (reflexivo, luego host, alias LAN), luego «IP única»; ambiguo = nada."""
        if cli.intento is not None:
            return True
        ip, puerto = cli.addr[0], cli.addr[1]
        exactos, por_ip = [], []
        for it in self.intentos.values():
            for m in it.miembros:
                pid = str(m["pid"])
                if pid in it.salidos:
                    continue
                eps = [e for e in (m["endpoints"]() if callable(m["endpoints"]) else m["endpoints"]) if e]
                tcp = m.get("ip_tcp")
                if (ip, puerto) in eps or any(e[1] == puerto and tcp == ip for e in eps):
                    exactos.append((it, pid))
                elif tcp == ip or any(e[0] == ip for e in eps):
                    por_ip.append((it, pid))
        candidatos = exactos or por_ip
        libres = [(it, pid) for it, pid in candidatos
                  if not (pid in it.pares and it.pares[pid] is not cli and it.pares[pid].addr != cli.addr
                          and time.time() - it.pares[pid].visto < 3)]
        if len(libres) != 1:
            if candidatos:
                log.warning("gamerelay %s: no se liga (%d candidatos, %d libres)", cli.addr,
                            len(candidatos), len(libres))
            return False
        it, pid = libres[0]
        cli.intento, cli.pid, cli.id_rele = it, pid, it.ids[pid]
        it.pares[pid] = cli
        log.info("gamerelay %s: ligado a la partida %s como %s (id %d)", cli.addr, it.room_id, pid, cli.id_rele)
        return True

    # ---- entrada ----
    def datagram_received(self, data, addr):
        try:
            self._recibir(data, addr)
        except Exception:
            log.exception("gamerelay: error con %s (%d B)", addr, len(data))

    def _recibir(self, data, addr):
        self._n = getattr(self, "_n", 0) + 1
        if self._n % 500 == 0:
            self.limpiar()
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
            dgseq, tflags, _eco, secciones = leer_datagrama(cuerpo)
        except (ValueError, struct.error, IndexError) as e:
            log.info("gamerelay %s: transporte no entendido (%s) %s", addr, e, cuerpo[:32].hex())
            return
        if tflags & 0x80:  # L §4.1: el cliente pide eco RTT
            cli.ecos = (cli.ecos + [(dgseq, time.monotonic())])[-16:]
        tocados = set()
        for canal, ack, lista_a, lista_b in secciones:
            c = cli.canal(canal)
            for k in [k for k, carga in c.pendientes.items() if k + max(1, len(bloques(carga))) <= ack]:
                del c.pendientes[k]  # ack = «siguiente que espero»
            for clave, carga in sorted(lista_a):
                self._aceptar(cli, canal, c, clave, carga, tocados)
            for _s_b, carga in lista_b:
                self._elemento(cli, canal, bloques(carga), tocados)
        # empuje inmediato a los otros jugadores que recibieron algo (L §3.2)
        for otro in tocados:
            if otro is not cli and self.empuje_inmediato:
                self._responder(otro)
        self._responder(cli)  # C: cada datagrama del cliente recibe respuesta (eco RTT)

    def _aceptar(self, cli, canal, c, clave, carga, tocados):
        """Recepción ordenada por bloque (L §3.2): lo nuevo se entrega una sola vez; lo futuro se guarda."""
        blq = bloques(carga)
        n = max(1, len(blq))
        if clave + n <= c.esperado:
            return  # repetido
        if clave > c.esperado:
            if clave - c.esperado < MAX_FUTUROS and len(c.futuros) < MAX_FUTUROS:
                c.futuros[clave] = carga
            return
        nuevos = blq[c.esperado - clave:]
        c.esperado = clave + n
        self._elemento(cli, canal, nuevos, tocados)
        while c.esperado in c.futuros:
            sig = c.futuros.pop(c.esperado)
            b2 = bloques(sig)
            c.esperado += max(1, len(b2))
            self._elemento(cli, canal, b2, tocados)
        for k in [k for k in c.futuros if k < c.esperado]:
            del c.futuros[k]

    def _kx(self, addr, cuerpo):
        nonce = struct.unpack(">I", rc4(self.k_int, cuerpo))[0]
        semilla = struct.unpack(">I", os.urandom(4))[0]
        m = material_kx(semilla, nonce)
        respuesta = armar_marco(0x80, rc4(self.k_int, m))
        cli = self.clientes[addr] = Cliente(addr, self.k_ext)
        self.enviar(cli, respuesta)    # la respuesta va con la clave constante
        cli.clave = m                  # P: después, M como clave exterior
        log.info("gamerelay KX con %s (nonce %08x)", addr, nonce)

    def _elemento(self, cli, canal, blq, tocados=None):
        tocados = tocados if tocados is not None else set()
        if canal in CANALES_JUEGO:
            self._juego(cli, canal, blq, tocados)
            return
        for b in blq:
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
                    if k == CANAL_LISTA:
                        self._quizas_lista(cli)
                elif op == CERRAR:
                    cli.abiertos.discard(k)
                    cli.poner(CANAL_FLUJOS, carga_de(bytes([CERRAR_OK, k])))
            elif canal == CANAL_CALIDAD and len(b) == 5:
                cuenta = (b[0] >> 3) + 1
                # C (04-nov): una copia por ELEMENTO (cada una con su seq); fundidas en una carga
                # el cliente mide ~1240 ms en LAN
                for _ in range(cuenta):
                    cli.poner(CANAL_CALIDAD, carga_de(b))

    def _juego(self, cli, canal, blq, tocados):
        """Fase 3: reenvío entre los jugadores de la misma partida (L FASE23 §3.3-3.4)."""
        it = cli.intento
        # diagnóstico (10-nov): los primeros mensajes de juego de cada jugador quedan en el registro,
        # para saber qué manda el PES durante el chequeo de conexión (sobre todo por 14 y 15)
        cli.vistos_juego += 1
        if cli.vistos_juego <= 60:
            log.info("gamerelay %s: canal %d -> %s", cli.addr, canal,
                     " | ".join(b[:48].hex() + ("…" if len(b) > 48 else "") for b in blq[:4]) or "(vacío)")
        if it is None:
            if blq:
                log.info("gamerelay %s: canal de juego %d sin partida ligada (se ignora)", cli.addr, canal)
            return
        if canal == CANAL_ORDENES:
            for b in blq:
                if not b or len(b) > 254:
                    continue
                trama = bytes([(cli.id_rele & 0x1F) << 3]) + b
                for pid, otro in self._receptores(it, cli, incluir_emisor=True):
                    otro.poner(CANAL_ORDENES, carga_de(trama))
                    tocados.add(otro)
                    it.reenviados += 1
        elif canal in CANALES_FLUJO:
            # C (10-nov, PES base): cada BLOQUE es un mensaje propio (fragmentos de ~16 B con su
            # cabecera de aplicación, p. ej. 40 dd…, 8f…, 8c…), igual que cada registro de la copia
            # TCP. Juntarlos en uno dejaba al otro PES con un mensaje que no sabe leer.
            for b in blq:
                if b and it.contar(cli.pid, canal, bytes(b), "nativo"):
                    self.entregar_flujo(it, cli.pid, canal, bytes(b), tocados)
        # 11, 13 y 14: solo se observan (L §3.5)

    def _receptores(self, it, emisor, incluir_emisor=False):
        lista = [(emisor.pid, emisor)] if incluir_emisor else []
        lista += [(p, c) for p, c in sorted(it.pares.items(), key=lambda kv: it.ids.get(kv[0], 99))
                  if c is not emisor and p not in it.salidos]
        return lista

    def entregar_flujo(self, it, pid_emisor, canal, mensaje, tocados=None):
        """Mensaje de 10/12/15 al resto: cabecera u16 0x4000|(id<<4) + mensaje, en bloques ≤255."""
        idx = it.ids.get(str(pid_emisor))
        if idx is None:
            return 0
        trama = struct.pack(">H", 0x4000 | ((idx & 0x1F) << 4)) + mensaje
        carga = carga_de(*trozos(trama))
        n = 0
        for pid, otro in sorted(it.pares.items()):
            if pid == str(pid_emisor) or pid in it.salidos:
                continue
            otro.poner(canal, carga)
            n += 1
            it.reenviados += 1
            if tocados is not None:
                tocados.add(otro)
            elif self.empuje_inmediato:
                self._responder(otro)
        return n

    def reenviar_tcp(self, pid_emisor, canal, mensaje, from_id=None):
        """Copia TCP (CMD_SEND_GAME_SEQUENCE_DATA) de un mensaje 10/12/15 (L §3.4)."""
        for it in self.intentos.values():
            if str(pid_emisor) in it.ids and str(pid_emisor) not in it.salidos:
                if from_id is not None and int(from_id) != it.ids[str(pid_emisor)]:
                    log.warning("gamerelay: GameSeq con fromId %s distinto del id %s de %s",
                                from_id, it.ids[str(pid_emisor)], pid_emisor)
                    return 0
                if it.contar(pid_emisor, canal, mensaje, "tcp"):
                    return self.entregar_flujo(it, pid_emisor, canal, mensaje)
                return 0
        return 0

    def _quizas_lista(self, cli):
        """Partida: lista de 3 bytes de la partida congelada. Sin partida, solo si el HTTP/TCP
        autorizó la sonda: lista = servidor + yo (L §1.4)."""
        if cli.lista_enviada:
            return
        if self._ligar(cli):
            cli.lista_enviada = True
            cli.poner(CANAL_LISTA, carga_de(cli.intento.lista_para(cli.pid)))
            log.info("gamerelay %s: lista de participantes de la partida %s", cli.addr, cli.intento.room_id)
            return
        if self.srv is not None and not self.srv.sonda_autorizada(cli.addr[0], self.indice):
            log.info("gamerelay %s: sin sonda autorizada ni sala; no se manda lista", cli.addr)
            return
        cli.sonda = cli.lista_enviada = True
        cli.poner(CANAL_LISTA, carga_de(bytes([ID_SERVIDOR, 0xC0 | 0])))
        log.info("gamerelay %s: lista de participantes (sonda)", cli.addr)

    def _responder(self, cli):
        """Un datagrama con lo pendiente de cada canal, en orden de clave y sin pasar de 2048 B
        (control primero: 0, 1 y 5 nunca quedan detrás del juego). L §3.2."""
        eco = None
        ahora = time.monotonic()
        while cli.ecos:
            dg, t = cli.ecos.pop(0)
            espera = int((ahora - t) * 1000)
            if espera <= 255:
                eco = (dg, espera)
                break
        resto = MAX_CUERPO - (10 if eco else 5)
        canales = [0, 1] + sorted(n for n in cli.canales if n not in (0, 1) and (
            n in (CANAL_LISTA, CANAL_CALIDAD) or n in cli.abiertos or cli.canales[n].esperado > 0))
        secciones = []
        for n in canales:
            c = cli.canal(n)
            lista = []
            usado = 6 + 1
            for k in sorted(c.pendientes):
                carga = c.pendientes[k]
                if len(lista) >= MAX_ELEMENTOS or usado + 6 + len(carga) > resto:
                    if not lista and 6 + 1 + 6 + len(carga) > MAX_CUERPO - 10:
                        log.warning("gamerelay %s: elemento de %d B no cabe en un datagrama (canal %d)",
                                    cli.addr, len(carga), n)
                    break
                lista.append((k, carga))
                usado += 6 + len(carga)
            resto -= tamano_seccion(lista)
            secciones.append((n, c.esperado, lista))
        cuerpo = armar_transporte(cli.seq, secciones, eco)
        cli.seq += 1
        # C (04-nov): tras el KX el cliente descarta marcos con bit 7; flags = sal de 6 bits
        # (bits 6 y 7 limpios), como hace el propio cliente. Solo la respuesta KX lleva 0x80.
        self.enviar(cli, armar_marco(os.urandom(1)[0] & 0x3F, cuerpo))

    VIDA_INTENTO = 3 * 3600  # protección: una partida «olvidada» se cierra sola a las 3 h

    def limpiar(self):
        ahora = time.time()
        for room in [r for r, it in self.intentos.items() if ahora - it.creado > self.VIDA_INTENTO]:
            log.warning("gamerelay: partida %s abierta más de 3 h; se cierra", room)
            self.cerrar_intento(room)
        for a in [a for a, c in self.clientes.items()
                  if c.visto < ahora - (VIDA_PARTIDA if c.intento is not None else self.VIDA_SONDA)]:
            del self.clientes[a]

    def estado(self):
        return {"clientes_udp": len(self.clientes),
                "partidas": {str(r): {"gen": it.generacion, "ligados": sorted(it.pares),
                                      "reenviados": it.reenviados} for r, it in self.intentos.items()}}
