"""Rival de prueba: un «segundo jugador» falso para probar el emparejamiento con UN solo PES.

Entra al servidor como otro jugador (SteamID inventado), declara su dirección UDP (NTL), busca
partida del mismo tipo que tú y, cuando el servidor os empareja, ENTRA AL RELÉ POR UDP como haría
un PES (KX, canal 0, claves 5/14/15, lista de participantes) e informa «chequeo correcto».
Todo lo que tu PES le manda por el relé queda anotado en registro/rival_udp.txt: así se ve qué
espera el PES del otro jugador durante «Revisando estado de conexión».
NO juega: no manda mandos (salvo con --espejo, que devuelve al PES lo que él manda, solo como
experimento). Es una herramienta de diagnóstico.

Uso (en la carpeta del servidor, con el servidor abierto):
    python tools/rival_falso.py                     -> Divisiones en línea (OC_RANK)
    python tools/rival_falso.py OC_QUICK            -> Partida rápida
    python tools/rival_falso.py OC_RANK --espejo    -> además devuelve al PES lo que manda
    python tools/rival_falso.py OC_RANK --sin-udp   -> como antes (solo TCP)
Primero arranca este rival y DESPUÉS busca partida en el PES (o al revés: da igual).
"""
import asyncio
import json
import time
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from phoenix_online.gamerelay_cliente import ClienteRele, claves_de_archivo, endpoint_ntl  # noqa: E402
from phoenix_online.gamerelay import carga_de  # noqa: E402
from phoenix_online.msgpack_min import Empaquetado  # noqa: E402
from phoenix_online.nclmio import Codec, LectorTramas  # noqa: E402

STEAM_FALSO = "0110000100001234"
HOST = "127.0.0.1"


class ClienteRevision(ClienteRele):
    """Con --revision: reconstruye el CMD_REVISION_CHECK que manda el PES por el canal 15
    (framing PESBUL: [0x40][largo zlib][datos] + trozos [0x80|n][n bytes], zlib de un mapa msgpack)
    y manda el suyo, con los mismos checksums, su pid y su nombre, cada 5 s."""

    def __init__(self, *a, **k):
        super().__init__(*a, **k)
        self._trozos = {}          # (id emisor) -> bytearray del mensaje en curso
        self._falta = {}
        self._enviado = 0.0
        # el pid nativo del PES = 32 bits bajos del pid de 64 bits del roster (292733975847239680 -> 68157440)
        self.pid_rival = int(STEAM_FALSO, 16) & 0xFFFFFFFF

    def _juego(self, canal, b):
        super()._juego(canal, b)
        if canal != 15 or len(b) < 3:
            return
        cab, resto = b[:2], b[2:]
        emisor = ((cab[0] << 8 | cab[1]) >> 4) & 0x1F
        if emisor == self.mi_id:
            return
        if resto[0] == 0x40 and len(resto) >= 2:
            self._trozos[emisor] = bytearray(resto[2:])
            self._falta[emisor] = resto[1] - len(resto[2:])
        elif resto[0] & 0xC0 == 0x80 and emisor in self._trozos:
            self._trozos[emisor] += resto[1:]
            self._falta[emisor] -= len(resto) - 1
        else:
            return
        if self._falta.get(emisor, 1) == 0:
            self._responder_revision(bytes(self._trozos.pop(emisor)))

    def _responder_revision(self, comprimido):
        import zlib
        from phoenix_online import msgpack_min
        try:
            m = msgpack_min.unpackb(zlib.decompress(comprimido))
        except Exception as e:
            self._anotar(f"REVISION_CHECK del PES no se pudo leer: {e}")
            return
        if not isinstance(m, dict) or m.get("msgid") != "CMD_REVISION_CHECK":
            self._anotar(f"mensaje del 15 que no es REVISION_CHECK: {str(m)[:120]}")
            return
        self._anotar(f"REVISION_CHECK del PES: {m}")
        if int(m.get("is_request", 1)) == 0:
            self._anotar("  -> es la RESPUESTA del PES a mi petición (el PES aceptó mi revision check)")
            return
        # petición del PES: se responde SIEMPRE (is_request 0) y además se manda la propia petición
        self._enviar_revision(m, 0)
        if time.monotonic() - self._enviado < 4:
            return
        self._enviado = time.monotonic()
        self._enviar_revision(m, 1)

    def _enviar_revision(self, m, es_peticion):
        import zlib
        from phoenix_online import msgpack_min
        mio = dict(m)
        mio["pid"] = self.pid_rival
        mio["name"] = "RIVAL_PRUEBA"
        mio["is_request"] = es_peticion
        crudo = zlib.compress(msgpack_min.packb(mio))
        pieza = bytes([0x40, len(crudo)]) + crudo[:14]
        self.poner(15, carga_de(pieza))
        resto = crudo[14:]
        while resto:
            n = resto[:15]
            self.poner(15, carga_de(bytes([0x80 | len(n)]) + n))
            resto = resto[15:]
        self._anotar(f"REVISION_CHECK del rival enviado (is_request {es_peticion}, {len(crudo)} B comprimidos)")


async def main(tipo, espejo=False, con_udp=True, revision=False):
    with open("config.json", encoding="utf-8") as f:
        cfg = json.load(f)
    with open(cfg["clave_nclmio"], "rb") as f:
        codec = Codec(f.read(), blowfish_le=cfg.get("blowfish_le", False))
    lector, (r, w) = LectorTramas(codec), await asyncio.open_connection(HOST, cfg["puerto_menu"])
    seq = 0

    async def mandar(msg):
        nonlocal seq
        seq += 1
        w.write(codec.codificar(seq, {"rqid": seq, **msg}))
        await w.drain()

    async def esperar(msgid, segundos=600):
        fin = asyncio.get_running_loop().time() + segundos
        while asyncio.get_running_loop().time() < fin:
            datos = await asyncio.wait_for(r.read(65536), fin - asyncio.get_running_loop().time())
            if not datos:
                raise ConnectionError("el servidor cerró la conexión")
            for _t, _s, m in lector.alimentar(datos):
                print("  <-", m.get("msgid"), m.get("result"), m.get("matching_state", ""))
                if m.get("msgid") == msgid:
                    return m
        raise TimeoutError(msgid)

    # relé UDP: el socket se abre antes para poder declarar su puerto por NTL
    udp = None
    registro = None
    if con_udp:
        k_ext, k_int = claves_de_archivo(cfg["clave_gamerelay"])
        os.makedirs("registro", exist_ok=True)
        registro = open(os.path.join("registro", "rival_udp.txt"), "a", encoding="utf-8")
        puerto_rele = int((cfg.get("gamerelay") or {}).get("puerto", 5100))
        _t, udp = await asyncio.get_running_loop().create_datagram_endpoint(
            lambda: (ClienteRevision if revision else ClienteRele)(k_ext, k_int, espejo=espejo, registro=registro),
            remote_addr=(HOST, puerto_rele))
        ip_udp, puerto_udp = _t.get_extra_info("sockname")[:2]
        print(f"Socket UDP del rival: {ip_udp}:{puerto_udp} -> relé {HOST}:{puerto_rele}")

    await mandar({"msgid": "CMD_LOGIN", "username": STEAM_FALSO.lstrip("0")})
    await esperar("CMD_LOGIN")
    await mandar({"msgid": "CMD_GET_PLAYER_DATA"})
    if (await esperar("CMD_GET_PLAYER_DATA")).get("has_data") != "YES":
        await mandar({"msgid": "CMD_CREATE_PLAYER", "pname": "RIVAL_PRUEBA"})
        await esperar("CMD_CREATE_PLAYER")
    if udp is not None:
        ntl = Empaquetado(endpoint_ntl(ip_udp, puerto_udp))
        await mandar({"msgid": "CMD_SEND_IPANDPORT_NTL", "ntl_nat_type": 1,
                      "host_addr": ntl, "reflexive_addr": ntl})
        await esperar("CMD_SEND_IPANDPORT_NTL")
    print(f"Rival de prueba dentro. Buscando {tipo}… (ahora busca partida en tu PES)")
    await mandar({"msgid": "CMD_START_MATCHING", "matching_type": tipo})
    while True:
        m = await esperar("CMD_WATCH_MATCHING")
        if m.get("matching_state") == "FINISH":
            break
    print("¡Emparejados!")
    vida = None
    try:
        if udp is not None:
            # mismas claves que abre el PES en el chequeo; con --espejo también las del partido
            claves = (5, 9, 10, 12, 14, 15) if espejo else (5, 14, 15)
            if not await udp.entrar(abrir=claves):
                print("El rival NO pudo entrar al relé UDP (mira registro/rival_udp.txt).")
            else:
                vida = asyncio.ensure_future(udp.mantener(240))
                try:
                    await asyncio.wait_for(udp.lista_llego.wait(), 10)
                    print(f"Rival ligado a la partida por UDP (lista {udp.lista.hex()}, id {udp.mi_id}).")
                except asyncio.TimeoutError:
                    print("El relé no mandó la lista de participantes al rival (no se ligó).")
        await asyncio.sleep(2)
        print("Informo chequeo de conexión correcto.")
        await mandar({"msgid": "CMD_SET_GAMERELAY_CHECK_RES", "error_code": "NOERR", "failed_reason": 0})
        try:
            await esperar("CMD_WATCH_GAMERELAY_CHECK_RES", 120)
            print("El servidor liberó la barrera del chequeo: tu PES también informó NOERR.")
            await mandar({"msgid": "CMD_SEND_MATCHING_RESULT", "error_code": "NOERR",
                          "quality_select_kind": "SELECTED"})
        except TimeoutError:
            print("Tu PES no informó chequeo correcto en 120 s.")
        # mantiene la conexión mientras el PES intenta cargar
        for _ in range(18):
            await asyncio.sleep(5)
            if udp is not None:
                print(f"  udp: {udp.datagramas} datagramas recibidos, {len(udp.recibidos)} mensajes de juego")
    finally:
        if vida:
            vida.cancel()
        await mandar({"msgid": "CMD_LEAVE_GAME"})
        w.close()
        if registro:
            registro.close()
        print("Rival de prueba fuera. Mira registro/servidor.log y registro/rival_udp.txt.")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    asyncio.run(main(args[0] if args else "OC_RANK", espejo="--espejo" in sys.argv,
                     con_udp="--sin-udp" not in sys.argv, revision="--revision" in sys.argv))
