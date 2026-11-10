"""Cliente mínimo del relé UDP (solo para pruebas: tools/rival_falso.py y los tests).

Hace lo mismo que un PES al entrar a la partida por el relé: KX, apretón del canal 0 (SYN/ACK),
abre las claves 5, 14 y 15 por el canal 1, recibe la lista de participantes del canal 5 y
mantiene viva la conexión. Todo lo que llega por los canales de juego (9..15) se guarda y se
anota, para saber QUÉ manda el PES al otro durante el «chequeo de conexión».

Modo espejo (opcional): lo que el PES manda por 9/10/12/15 se le devuelve como si lo mandara el
rival (sin la etiqueta del relé). Es un experimento de diagnóstico, no un partido de verdad.
Escrito por nosotros con el formato de docs/ESPEC_GAMERELAY*.md (mismo transporte que gamerelay.py).
"""
import asyncio
import logging
import os
import struct
import time

from .gamerelay import (ABRIR, ABRIR_OK, ACK, CANAL_CONTROL, CANAL_FLUJOS, CANAL_LISTA, CANAL_ORDENES,
                        CANALES_FLUJO, SYN, SYN_ACK, VIDA, armar_marco, armar_transporte, bloques, carga_de,
                        cifrar_exterior, descifrar_exterior, leer_datagrama, leer_marco, rc4, trozos)

log = logging.getLogger("phoenix.gamerelay_cliente")


class _Canal:
    def __init__(self):
        self.siguiente = 0     # siguiente clave para MIS bloques
        self.esperado = 0      # siguiente clave que espero del servidor (= mi ack)
        self.pendientes = {}   # clave -> carga, hasta que el servidor la confirme
        self.futuros = {}


class ClienteRele(asyncio.DatagramProtocol):
    def __init__(self, clave_exterior, clave_interior, espejo=False, registro=None):
        self.k_ext, self.k_int = clave_exterior, clave_interior
        self.clave = clave_exterior
        self.kx_hecho = asyncio.Event()
        self.establecido = asyncio.Event()
        self.lista = None                 # bytes de la lista del canal 5
        self.lista_llego = asyncio.Event()
        self.canales = {}
        self.abiertos = set()
        self.seq = 0
        self.transporte = None
        self.espejo = espejo
        self.recibidos = []               # [(canal, bytes)] de los canales de juego
        self.registro = registro          # archivo de texto opcional
        self.datagramas = 0
        self.mi_id = None

    # ---- utilidades ----
    def canal(self, n):
        return self.canales.setdefault(n, _Canal())

    def poner(self, n, carga):
        c = self.canal(n)
        c.pendientes[c.siguiente] = carga
        c.siguiente += max(1, len(bloques(carga)))

    def _anotar(self, texto):
        log.info("rival udp: %s", texto)
        if self.registro:
            self.registro.write(f"{time.strftime('%H:%M:%S')} {texto}\n")
            self.registro.flush()

    def connection_made(self, transport):
        self.transporte = transport

    def _mandar_marco(self, marco):
        if self.transporte is not None:
            self.transporte.sendto(cifrar_exterior(self.clave, marco))

    # ---- KX ----
    def mandar_kx(self):
        self.nonce = struct.unpack(">I", os.urandom(4))[0]
        self._mandar_marco(armar_marco(0x80, rc4(self.k_int, struct.pack(">I", self.nonce))))

    # ---- envío ----
    def enviar(self):
        canales = [0, 1] + sorted(n for n in self.canales if n not in (0, 1))
        secciones = []
        for n in canales:
            c = self.canal(n)
            lista = sorted(c.pendientes.items())[:64]
            secciones.append((n, c.esperado, lista))
        cuerpo = armar_transporte(self.seq, secciones)
        self.seq += 1
        self._mandar_marco(armar_marco(os.urandom(1)[0] & 0x3F, cuerpo))

    # ---- recepción ----
    def datagram_received(self, data, addr):
        try:
            self._recibir(data)
        except Exception as e:  # nunca tumbar el bucle por un datagrama raro
            self._anotar(f"datagrama no entendido ({e})")

    def _recibir(self, data):
        self.datagramas += 1
        if not self.kx_hecho.is_set():
            marco = leer_marco(descifrar_exterior(self.k_ext, data))
            if marco and marco[0] & 0x80 and len(marco[1]) == 256:
                self.clave = rc4(self.k_int, marco[1])
                self.kx_hecho.set()
                self._anotar("KX hecho")
            return
        marco = leer_marco(descifrar_exterior(self.clave, data))
        if marco is None:
            self._anotar(f"marco ilegible ({len(data)} B)")
            return
        _seq, _flags, _eco, secciones = leer_datagrama(marco[1])
        for canal, ack, lista_a, lista_b in secciones:
            c = self.canal(canal)
            for k in [k for k, carga in c.pendientes.items() if k + max(1, len(bloques(carga))) <= ack]:
                del c.pendientes[k]
            for clave, carga in sorted(lista_a):
                self._aceptar(canal, c, clave, carga)
            for _s, carga in lista_b:
                self._elemento(canal, bloques(carga))

    def _aceptar(self, canal, c, clave, carga):
        blq = bloques(carga)
        n = max(1, len(blq))
        if clave + n <= c.esperado:
            return
        if clave > c.esperado:
            c.futuros[clave] = carga
            return
        nuevos = blq[c.esperado - clave:]
        c.esperado = clave + n
        self._elemento(canal, nuevos)
        while c.esperado in c.futuros:
            sig = bloques(c.futuros.pop(c.esperado))
            c.esperado += max(1, len(sig))
            self._elemento(canal, sig)

    def _elemento(self, canal, blq):
        if canal in CANALES_FLUJO and blq:
            # 10/12/15: un elemento = un mensaje partido en bloques de ≤255; se junta entero
            self._juego(canal, b"".join(blq))
            return
        for b in blq:
            if canal == CANAL_CONTROL and b:
                if b[0] == SYN_ACK:
                    self.poner(CANAL_CONTROL, carga_de(bytes([ACK])))
                    self.establecido.set()
                    self._anotar("canal 0 establecido")
            elif canal == CANAL_FLUJOS and len(b) >= 2 and b[0] == ABRIR_OK:
                self.abiertos.add(b[1])
                self._anotar(f"clave {b[1]} abierta")
            elif canal == CANAL_LISTA:
                self.lista = bytes(b)
                yo = [x & 0x1F for x in b if x & 0xC0 == 0xC0]
                self.mi_id = yo[0] if yo else None
                self.lista_llego.set()
                self._anotar(f"lista de participantes {b.hex()} (mi id {self.mi_id})")
            elif canal >= CANAL_ORDENES:
                self._juego(canal, bytes(b))

    def _juego(self, canal, b):
        self.recibidos.append((canal, b))
        if len(self.recibidos) <= 400:
            self._anotar(f"canal {canal} <- {len(b)} B {b[:64].hex()}")
        if not self.espejo:
            return
        if canal == CANAL_ORDENES and len(b) >= 2:
            emisor = b[0] >> 3
            if emisor != self.mi_id:
                self.poner(CANAL_ORDENES, carga_de(b[1:]))
        elif canal in CANALES_FLUJO and len(b) >= 3:
            (cab,) = struct.unpack(">H", b[:2])
            if (cab >> 4) & 0x1F != self.mi_id:
                self.poner(canal, carga_de(*trozos(b[2:])))

    # ---- guion completo ----
    async def entrar(self, abrir=(CANAL_LISTA, 14, 15), espera=10.0):
        """KX + canal 0 + abrir claves. Devuelve True si quedó establecido."""
        for _ in range(5):
            self.mandar_kx()
            try:
                await asyncio.wait_for(self.kx_hecho.wait(), 2)
                break
            except asyncio.TimeoutError:
                continue
        else:
            self._anotar("el relé no respondió al KX")
            return False
        self.poner(CANAL_CONTROL, carga_de(bytes([SYN])))
        fin = time.monotonic() + espera
        while not self.establecido.is_set() and time.monotonic() < fin:
            self.enviar()
            await asyncio.sleep(0.05)
        if not self.establecido.is_set():
            self._anotar("el canal 0 no se estableció")
            return False
        for k in abrir:
            self.poner(CANAL_FLUJOS, carga_de(bytes([ABRIR, k])))
        return True

    async def mantener(self, segundos, cada=0.05, vida_cada=5.0):
        """Manda un datagrama cada `cada` s (lleva acks y lo pendiente) y la vida del canal 0."""
        fin = time.monotonic() + segundos
        ultima_vida = time.monotonic()
        while time.monotonic() < fin:
            if time.monotonic() - ultima_vida >= vida_cada:
                self.poner(CANAL_CONTROL, carga_de(bytes([VIDA])))
                ultima_vida = time.monotonic()
            self.enviar()
            await asyncio.sleep(cada)


def endpoint_ntl(ip, puerto):
    """Declaración NTL (mapa con sa_data) para CMD_SEND_IPANDPORT_NTL: puerto, relleno, IPv4, ceros."""
    import base64
    sa = struct.pack(">H", puerto) + b"\x00\x00" + bytes(int(x) for x in ip.split(".")) + b"\x00" * 6
    return {"address_family": 2, "sa_data": base64.b64encode(sa).decode()}


def claves_de_archivo(ruta):
    with open(ruta, "rb") as f:
        datos = f.read()
    if len(datos) != 272:
        raise ValueError("clave_gamerelay debe tener 272 bytes (256 + 16)")
    return datos[:256], datos[256:]
