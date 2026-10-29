import asyncio
import os
import unittest

from phoenix_online import msgpack_min
from phoenix_online.datos import Datos
from phoenix_online.logica import Servidor, Sesion
from phoenix_online.msgpack_min import Empaquetado
from phoenix_online.nclmio import Codec, ErrorTrama, LectorTramas, xor

CLAVE_FALSA = bytes(range(56))  # la real vive solo en la PC de Frank


class TestMsgpack(unittest.TestCase):
    def test_ida_y_vuelta(self):
        o = {"msgid": "CMD_LOGIN", "rqid": 7, "n": -3, "g": 1 << 40, "b": b"\x00\xff",
             "l": list(range(20)), "t": True, "f": 1.5, "x": None, "s": "ñ" * 40}
        self.assertEqual(msgpack_min.unpackb(msgpack_min.packb(o)), o)

    def test_empaquetado_es_bin(self):
        b = msgpack_min.packb({"ulist": [Empaquetado({"type": "TOPINFO", "url": "u"})]})
        lista = msgpack_min.unpackb(b)["ulist"]
        self.assertIsInstance(lista[0], bytes)
        self.assertEqual(msgpack_min.unpackb(lista[0]), {"type": "TOPINFO", "url": "u"})

    def test_texto_no_utf8_no_rompe(self):
        self.assertEqual(msgpack_min.unpackb(b"\xa2\xff\xfe"), b"\xff\xfe")


class TestTrama(unittest.TestCase):
    def setUp(self):
        self.codec = Codec(CLAVE_FALSA, comprobar_clave=False)

    def test_ida_y_vuelta_partida_y_pegada(self):
        a = self.codec.codificar(5, {"msgid": "A", "rqid": 1})
        b = self.codec.codificar(6, {"msgid": "B", "rqid": 2, "d": "x" * 3000})  # comprime
        self.assertEqual(len(a) % 8, 0)
        lector = LectorTramas(self.codec)
        flujo = a + b
        recibidos = []
        for i in range(0, len(flujo), 7):
            recibidos += lector.alimentar(flujo[i:i + 7])
        self.assertEqual([(s, m["msgid"]) for _, s, m in recibidos], [(5, "A"), (6, "B")])

    def test_md5_malo(self):
        t = bytearray(xor(self.codec.codificar(1, {"msgid": "A"})))
        t[-1] ^= 1
        with self.assertRaises(ErrorTrama):
            LectorTramas(self.codec).alimentar(xor(bytes(t)))

    def test_blowfish_le(self):
        c = Codec(CLAVE_FALSA, blowfish_le=True, comprobar_clave=False)
        self.assertEqual(LectorTramas(c).alimentar(c.codificar(3, {"msgid": "Z"}))[0][2]["msgid"], "Z")

    def test_clave_real_exigida(self):
        with self.assertRaises(ValueError):
            Codec(CLAVE_FALSA)

    def test_cabe_en_0x410(self):
        t = self.codec.codificar(1, {"msgid": "A", "d": "abc" * 2000})
        self.assertLessEqual(len(t) - 24, 0x410)


CFG = {
    "urls": {"TOPINFO": "t", "BANNER": "b", "GET_TURN_SERVER_LIST": "u"},
    "versiones": {"version": 1}, "naciones": [], "nacion": 0, "ip_publica": "1.2.3.4",
    "puerto_menu": 10001,
    "lobbies": [{"lobby_id": 1, "name": "L", "player_num": 0, "player_num_max": 9,
                 "gamerelay_index": 0, "lobby_type": "FREE"}],
    "turn": {"secreto": "s", "servidores": [{"address": "1.2.3.4"}]},
}


def cliente(srv):
    enviados = []
    s = Sesion(lambda seq, m: enviados.append((seq, m)), ("x", 1), "MENU")
    srv.abrir(s)
    return s, enviados


async def _sin_pausa(_):
    return None


class TestFlujo(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.srv = Servidor(CFG, Datos())
        self.srv.dormir = _sin_pausa

    async def entrar(self, steam):
        s, env = cliente(self.srv)
        r = self.srv.recibir
        await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": steam})
        await r(s, 2, {"msgid": "CMD_GET_PLAYER_DATA", "rqid": 2})
        self.assertFalse(env[-1][1]["has_data"])
        await r(s, 3, {"msgid": "CMD_CREATE_PLAYER", "rqid": 3})
        for w in ("CMD_REQWATCHL_ROOMLIST", "CMD_REQWATCHL_ROOMPLAYERLIST_NTL",
                  "CMD_WATCH_ROOMSTATE", "CMD_WATCH_ENTRY_GAME", "CMD_WATCH_DECIDE_GAMEENV_FREE",
                  "CMD_WATCH_TURN_ADDRESS_DATA"):
            await r(s, 4, {"msgid": w, "rqid": 100 + len(w)})
        return s, env

    async def test_urllist_y_secuencia(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 42, {"msgid": "CMD_GET_URLLIST", "rqid": 9})
        seq, m = env[-1]
        self.assertEqual((seq, m["rqid"], m["result"], m["url_num"]), (42, 9, "NOERR", 3))
        self.assertTrue(all(isinstance(x, Empaquetado) for x in m["ulist"]))

    async def test_confirmacion_offline(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_JOIN_OFFLINE_MODE", "rqid": 5})
        self.assertEqual(env[-1][1], {"msgid": "CMD_JOIN_OFFLINE_MODE", "rqid": 5, "result": "NOERR"})

    async def test_vinculo_exigido(self):
        self.srv.datos.vinculos = {"aa": {}}
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "BB"})
        self.assertNotEqual(env[-1][1]["result"], "NOERR")

    async def test_amistoso_completo(self):
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        await self.srv.recibir(a, 5, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 6, "name": "X"})
        room = ea[-1][1]["room_id"]
        await self.srv.recibir(a, 6, {"msgid": "CMD_SET_GAMEENV", "rqid": 7, "gametime": 10})
        self.assertTrue(any(m.get("list_type") == "ADD" for _, m in eb))  # B vio la sala nueva
        eb.clear()
        await self.srv.recibir(b, 9, {"msgid": "CMD_JOIN_ROOM", "rqid": 8, "room_id": room})
        listas = [m for _, m in eb if m["msgid"] == "CMD_REQWATCHL_ROOMPLAYERLIST_NTL"]
        self.assertEqual([m["list_type"] for m in listas], ["UPDATE", "ADD"])
        self.assertEqual(listas[-1]["list_num"], 2)  # sin esto: UQOW_0006
        self.assertTrue(all(seq == 9 for seq, _ in eb))  # secuencia actual de la sesión
        self.assertEqual(eb[-1][1]["msgid"], "CMD_WATCH_ENTRY_GAME")
        await self.srv.recibir(a, 7, {"msgid": "CMD_ENTRY_GAME", "rqid": 10, "entry": True})
        self.assertFalse(any(m.get("state") == "SETENV" for _, m in ea))
        await self.srv.recibir(b, 10, {"msgid": "CMD_ENTRY_GAME", "rqid": 11, "entry": True})
        for e in (ea, eb):
            ids = [m["msgid"] for _, m in e]
            self.assertIn("CMD_WATCH_ROOMSTATE", ids)
            self.assertEqual(e[-2][1]["msgid"], "CMD_WATCH_DECIDE_GAMEENV_FREE")
            self.assertEqual(e[-2][1]["gametime"], 10)
        sala = self.srv.salas[room]
        self.assertEqual(sala.estado, "GAME")
        for s, q in ((a, 8), (b, 11)):
            await self.srv.recibir(s, q, {"msgid": "CMD_SEND_GAME_RESULT", "rqid": 1, "result_a": 2})
        self.assertEqual(len(self.srv.datos.resultados(sala.partido)), 2)
        await self.srv.recibir(b, 12, {"msgid": "CMD_LEAVE_ROOM", "rqid": 1})
        await self.srv.cerrar(a)
        self.assertEqual(self.srv.salas, {})

    async def test_reconexion_no_la_borra_la_sesion_vieja(self):
        a, _ = await self.entrar("A1")
        await self.srv.recibir(a, 5, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 6})
        room = a.room_id
        a2, _ = cliente(self.srv)
        await self.srv.recibir(a2, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "A1"})
        await self.srv.recibir(a2, 2, {"msgid": "CMD_GET_PLAYER_DATA", "rqid": 2})
        await self.srv.cerrar(a)  # la vieja se cierra DESPUÉS
        self.assertIn(room, self.srv.salas)
        self.assertEqual(self.srv.salas[room].miembros, [a2])
        self.assertIs(self.srv.por_pid[a2.pid], a2)

    async def test_turn_pendiente_y_entrega(self):
        a, ea = await self.entrar("A1")
        b, eb = cliente(self.srv)
        await self.srv.recibir(b, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "B2"})
        await self.srv.recibir(b, 2, {"msgid": "CMD_CREATE_PLAYER", "rqid": 2})
        await self.srv.recibir(a, 3, {"msgid": "CMD_SEND_TURN_ADDRESS_DATA", "rqid": 3,
                                      "pid": b.pid, "address": b"\x01\x02", "address_data_size": 2})
        await self.srv.recibir(b, 4, {"msgid": "CMD_WATCH_TURN_ADDRESS_DATA", "rqid": 77})
        self.assertEqual(eb[-1][1]["address"], b"\x01\x02")
        self.assertEqual(eb[-1][1]["rqid"], 77)
        r = self.srv.respuesta_turn({"pid": "1", "rqid": "4"})
        self.assertEqual(r["turn_server_list"][0]["address"], "1.2.3.4")
        self.assertIn(":1", r["id"])


if __name__ == "__main__":
    unittest.main()
