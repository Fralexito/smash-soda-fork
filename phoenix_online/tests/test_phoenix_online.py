import asyncio
import os
import unittest

from phoenix_online import msgpack_min
from phoenix_online.datos import Datos
from phoenix_online.logica import Servidor, Sesion
from phoenix_online.msgpack_min import Empaquetado
from phoenix_online.stun import respuesta_binding
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

    def test_trama_troceada(self):
        import hashlib, struct, zlib
        from phoenix_online import nclmio
        crudo = msgpack_min.packb({"msgid": "CMD_SET_AVATAR", "rqid": 3, "x": "a" * 3000})
        datos = zlib.compress(crudo)
        mitad = len(datos) // 2

        def trama(tipo, seq, interno):
            interno += b"\0" * (-len(interno) % 8)
            cif = self.codec._enc(interno)
            cab = struct.pack(">HHI", tipo, len(cif), seq)
            return nclmio.xor(cab + hashlib.md5(cab + cif).digest() + cif)

        a = trama(0x0070, 7, struct.pack(">IIIB", len(datos), mitad, len(crudo), 1) + datos[:mitad])
        b = trama(0x0072, 8, struct.pack(">I", len(datos) - mitad) + datos[mitad:])
        lector = LectorTramas(self.codec)
        self.assertEqual(lector.alimentar(a), [])
        [(tipo, seq, msg)] = lector.alimentar(b)
        self.assertEqual((tipo, seq, msg["msgid"], len(msg["x"])), (0x0060, 7, "CMD_SET_AVATAR", 3000))

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

    def test_respuesta_grande_va_troceada(self):
        import os
        grande = {"msgid": "G", "x": os.urandom(3000)}
        trama = self.codec.codificar(9, grande)
        tipo = __import__("struct").unpack(">H", xor(trama[:2]))[0]
        self.assertEqual(tipo, 0x0070)
        recibidos = LectorTramas(self.codec).alimentar(trama)
        self.assertEqual(recibidos[0][1], 9)
        self.assertEqual(recibidos[0][2]["x"], grande["x"])

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


class TestStun(unittest.TestCase):
    def test_binding(self):
        import struct
        txid = bytes(range(12))
        pet = struct.pack(">HHI", 1, 0, 0x2112A442) + txid
        r = respuesta_binding(pet, "1.2.3.4", 5739)
        self.assertEqual(r[:2], b"\x01\x01")
        self.assertEqual(r[8:20], txid)
        x = r[20 + 12:]  # XOR-MAPPED-ADDRESS
        puerto = struct.unpack(">H", x[6:8])[0] ^ 0x2112
        ip = bytes(a ^ b for a, b in zip(x[8:12], bytes.fromhex("2112A442")))
        self.assertEqual((puerto, ip), (5739, bytes([1, 2, 3, 4])))
        self.assertIsNone(respuesta_binding(b"basura", "1.2.3.4", 1))


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
        self.assertEqual(env[-1][1]["has_data"], "NO")
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
        self.assertEqual((seq, m["rqid"], m["result"], m["url_num"]), (42, 9, "NOERR", 26))  # +6 URLs HTTP de rankings y regalos (ESPEC B.0)
        self.assertTrue(all(isinstance(x, Empaquetado) for x in m["ulist"]))

    async def test_login_real_steam_challenge_account(self):
        s, env = cliente(self.srv)
        r = self.srv.recibir
        await r(s, 1, {"msgid": "CMD_AUTH_STEAM", "rqid": 1, "ticket": b"x" * 16})
        self.assertEqual(env[-1][1]["result"], "NOERR")
        await r(s, 2, {"msgid": "CMD_GET_CHALLENGECODE", "rqid": 2})
        self.assertTrue(env[-1][1]["challenge_code"])
        await r(s, 3, {"msgid": "CMD_LOGIN_ACCOUNT", "rqid": 3, "np_account_id": 0x0110000100000000})
        self.assertEqual(env[-1][1]["result"], "NOERR")
        self.assertEqual(s.steam_hex, "0110000100000000")
        self.assertEqual(Servidor._steam_de({"np_account_id": "239200"}), "000000000003a660")
        await r(s, 4, {"msgid": "CMD_SEND_PSID", "rqid": 4})
        self.assertEqual(env[-1][1]["result"], "NOERR")

    async def test_auth_steam_http_y_perfil_creado(self):
        r = self.srv.respuesta_http({"msgid": "CMD_AUTH_STEAM", "rqid": "4", "ticket": "ab" * 300})
        self.assertEqual((r["msgid"], r["rqid"], r["result"]), ("CMD_AUTH_STEAM", 4, "NOERR"))
        self.assertIsNone(self.srv.respuesta_http({}))
        # Tras CREATE_PLAYER, GET_PLAYER_DATA debe decir has_data (si no, el cliente vuelve a crear)
        s, env = await self.entrar("0110000100000077")
        await self.srv.recibir(s, 9, {"msgid": "CMD_GET_PLAYER_DATA", "rqid": 9})
        self.assertEqual(env[-1][1]["has_data"], "YES")
        pn = env[-1][1]["pname"].valor
        self.assertIsInstance(pn["psn_user_id"], Empaquetado)
        self.assertEqual(pn["psn_user_id"].valor["is_valid_cache"], "1")

    async def test_check_local_gamerelay(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_CHECK_LOCAL_GAMERELAY_PLAYER", "rqid": 2})
        m = env[-1][1]
        self.assertEqual((m["result"], m["is_local_gamerelay_player"]), ("NOERR", False))
        self.assertEqual(m["build_gamerelay_num_for_1vs1"], 0)

    async def test_login_account_rechaza_sin_vinculo(self):
        srv = Servidor(CFG, Datos(vinculos={}))
        s, env = cliente(srv)
        await srv.recibir(s, 1, {"msgid": "CMD_LOGIN_ACCOUNT", "rqid": 1, "np_account_id": 5})
        self.assertEqual(env[-1][1]["result"], "ERR_LOGIN_FAILED")

    async def test_svrlist_trae_account_y_menu(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_SVRLIST", "rqid": 10, "svrtype": "GATE"})
        m = env[-1][1]
        self.assertEqual(m["server_num"], 2)
        self.assertEqual([x.valor["svrtype"] for x in m["svrlist"]], ["ACCOUNT", "MENU"])

    async def test_svrversion_como_cliente_y_eula_no_cero(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_SVRVERSION", "rqid": 3,
                                      "patch_version": "1.07.02", "dlc_version": "7.00"})
        m = env[-1][1]
        self.assertEqual((m["version"], m["patch_version"], m["dlc_version"]), ("1", "1.07.02", "7.00"))
        self.assertEqual((m["eula_version"], m["eula_myclub_version"], m["eula_privacy_policy_version"]),
                         ("0", "0", "0"))
        self.assertEqual(m["eula_privacy_policy_japan_version"], "0")
        self.assertTrue(all(isinstance(v, str) for k, v in m.items() if k.endswith("version")))

    async def test_urllist_trae_las_tres_eula(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_URLLIST", "rqid": 1})
        tipos = [x.valor["type"] for x in env[-1][1]["ulist"]]
        for t in ("EULA_PES", "EULA_MYCLUB", "EULA_PRIVACY_POLICY", "FLOW", "DMM_TOP", "WEEKLY_INFO_BANNER"):
            self.assertIn(t, tipos)
        self.assertEqual(env[-1][1]["url_num"], len(tipos))

    def test_username_solo_digitos_es_hex(self):
        self.assertEqual(Servidor._steam_de({"username": "410000004100000"}), "0410000004100000")
        self.assertEqual(Servidor._steam_de({"steamid": "292733975847239680"}), "0410000004100000")

    def test_steam_hex_sin_ceros(self):
        self.assertEqual(Servidor._steam_de({"np_account_id": "0", "username": "3a660"}),
                         "000000000003a660")

    async def test_crear_jugador_dos_veces(self):
        d = self.srv.datos
        a = d.crear_jugador("000000000003a660", "FRALEX")
        b = d.crear_jugador("000000000003a660", "FRALEX")
        self.assertEqual(a["pid"], b["pid"])

    async def test_nacion_siempre_cadena(self):
        self.srv.cfg = dict(CFG, naciones=[{"nation_code": 0, "has_area": False, "name": "Peru"}],
                            nacion=0)
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_EULA_NATION_LIST", "rqid": 1})
        self.assertEqual(env[-1][1]["nation_list"][0].valor,
                         {"nation_code": "0", "has_area": "NO", "name": "Peru",
                          "privacy_kind": "PRIVACY_POLICY"})
        await self.srv.recibir(s, 2, {"msgid": "CMD_GET_EULA_NATION_CODE", "rqid": 2})
        self.assertEqual((env[-1][1]["nation_code"], env[-1][1]["area_code"]), ("0", ""))

    async def test_area_list_cadenas(self):
        self.srv.cfg = dict(CFG, areas_por_defecto=[{"area_code": 7, "name": "Lima"}])
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_EULA_AREA_LIST", "rqid": 1})
        m = env[-1][1]
        self.assertEqual((m["area_list_num"], m["area_list"][0].valor), (1, {"area_code": "7", "name": "Lima"}))

    async def test_avisos_nunca_vacios(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_GLOBALINFOTITLELIST", "rqid": 1})
        m = env[-1][1]
        self.assertEqual(m["annnounce_info_list_num"], 1)
        self.assertEqual(m["announce_info_list"][0].valor["announce_type"], "NO_BANNER_INFO")
        await self.srv.recibir(s, 2, {"msgid": "CMD_GET_INFOBODY", "rqid": 2, "announce_id": 1})
        self.assertIn("body", env[-1][1]["announce_info"].valor)

    async def test_confirmacion_offline(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_JOIN_OFFLINE_MODE", "rqid": 5})
        self.assertEqual(env[-1][1], {"msgid": "CMD_JOIN_OFFLINE_MODE", "rqid": 5, "result": "NOERR"})

    async def test_disconnect_mov(self):
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 3, {"msgid": "CMD_DISCONNECT", "rqid": 4, "logoutstat": "MOV"})
        self.assertEqual(env[-1][1]["result"], "NOERR")
        self.assertTrue(s.cerrar_tras_responder)

    async def test_vinculo_exigido(self):
        self.srv.datos.vinculos = {"aa": {}}
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "BB"})
        self.assertNotEqual(env[-1][1]["result"], "NOERR")

    async def test_amistoso_completo(self):
        """R (rooms.md/livetests.md AzrakHQ, flujo probado con 2 PES el 06-nov): ADD de la sala al
        crearla, nunca RELOAD empujado, lista de jugadores solo a quien ya mandó ENTRY_GAME, inicio con
        SET_GAMEMEMBER del dueño, DECIDE_GAMEENV + SETENV, vuelta a la sala tras LEAVE_GAME."""
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        r = self.srv.recibir
        await r(a, 5, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 6, "name": "X"})
        self.assertEqual([m["msgid"] for _, m in ea[-2:]], ["CMD_CREATEJOIN_ROOM", "CMD_REQWATCHL_ROOMLIST"])
        self.assertEqual(ea[-1][1]["list_type"], "ADD")
        room = ea[-2][1]["room_id"]
        env = msgpack_min.packb({"gametime": 15, "limitTime": "LONG", "injury": "NO"})
        await r(a, 6, {"msgid": "CMD_SET_GAMEENV", "rqid": 7, "game_env": env})
        vistas = [m for _, m in eb if m.get("msgid") == "CMD_REQWATCHL_ROOMLIST" and m["list_num"]]
        self.assertTrue(vistas)
        self.assertNotIn("RELOAD", [m["list_type"] for m in vistas[1:]])  # RELOAD solo como respuesta
        fila = msgpack_min.unpackb(msgpack_min.packb(vistas[-1]["list_data"][0].valor))
        self.assertEqual((fila["vs_com"], fila["is_passwd"], fila["is_room_owner"]), ("PL_VS_PL", "NO", "NO"))
        self.assertEqual(msgpack_min.unpackb(fila["gameenv"])["gametime"], 15)
        eb.clear()
        await r(b, 9, {"msgid": "CMD_JOIN_ROOM_NTL", "rqid": 8, "room_id": room, "password": ""})
        self.assertEqual(eb[0][1]["msgid"], "CMD_JOIN_ROOM_NTL")
        self.assertEqual(eb[0][1]["room_stat"], "WAITING")
        # el dueño aún no mandó ENTRY_GAME: no se le empuja la lista de jugadores
        self.assertNotIn("CMD_REQWATCHL_ROOMPLAYERLIST_NTL", [m["msgid"] for _, m in ea[-3:]])
        await r(b, 10, {"msgid": "CMD_REQWATCHL_ROOMPLAYERLIST_NTL", "rqid": 9, "room_id": room})
        rel = [m for _, m in eb if m["msgid"] == "CMD_REQWATCHL_ROOMPLAYERLIST_NTL"][-1]
        self.assertEqual((rel["list_type"], rel["list_num"]), ("RELOAD", 2))
        await r(a, 7, {"msgid": "CMD_ENTRY_GAME", "rqid": 10, "entry": "TRUE"})
        self.assertEqual([m for _, m in ea if m["msgid"] == "CMD_ENTRY_GAME"][-1]["entryflag"], "TRUE")
        await r(a, 8, {"msgid": "CMD_SET_GAMEMEMBER", "rqid": 11})
        self.assertFalse(any(m.get("state") == "SETENV" for _, m in ea))  # falta el invitado
        await r(b, 11, {"msgid": "CMD_ENTRY_GAME", "rqid": 12, "entry": "TRUE"})
        eg = [m for _, m in ea if m["msgid"] == "CMD_WATCH_ENTRY_GAME"][-1]
        self.assertEqual(eg["entry_count"], 2)
        self.assertIsInstance(msgpack_min.packb(eg), bytes)
        await r(a, 9, {"msgid": "CMD_SET_GAMEMEMBER", "rqid": 13})
        for e in (ea, eb):
            ids = [m["msgid"] for _, m in e]
            self.assertIn("CMD_WATCH_DECIDE_GAMEENV", ids)
            st = [m for _, m in e if m["msgid"] == "CMD_WATCH_ROOMSTATE"][-1]
            self.assertEqual((st["event"], st["state"]), ("StateChanged", "SETENV"))
        sala = self.srv.salas[room]
        pareja = self.srv._pareja_por_clave(a)
        self.assertTrue(pareja and pareja["room_id"])  # partida abierta: id de partido, relé, resultados
        mid = await self.srv.c_cmd_get_matchid(b, {})
        self.assertNotEqual(mid["match_id"], "PHOENIX-0000000000")
        for s, q in ((a, 12), (b, 13)):
            await r(s, q, {"msgid": "CMD_SEND_GAME_RESULT", "rqid": 1, "home_score": 1, "away_score": 0})
        self.assertEqual(len(self.srv.datos.resultados(f"pareja{pareja['room_id']}")), 2)
        await r(a, 13, {"msgid": "CMD_LEAVE_GAME", "rqid": 1})
        self.assertEqual(sala.estado, "SETENV")
        await r(b, 14, {"msgid": "CMD_LEAVE_GAME", "rqid": 1})
        self.assertEqual((sala.estado, sala.game_no), ("WAITING", 1))
        # el dueño se va: el invitado pasa a ser dueño (AppointOwner)
        await r(a, 14, {"msgid": "CMD_LEAVE_ROOM", "rqid": 1})
        st = [m for _, m in eb if m["msgid"] == "CMD_WATCH_ROOMSTATE"][-1]
        self.assertEqual(st["event"], "AppointOwner")
        self.assertIs(sala.dueno, b)
        await self.srv.cerrar(b)
        self.assertEqual(self.srv.salas, {})

    async def test_partida_rapida_empareja_dos(self):
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        r = self.srv.recibir
        await r(a, 20, {"msgid": "CMD_GET_QUICKMATCHOPTION", "rqid": 1})
        self.assertEqual(ea[-1][1]["ping_level"], "PING_LEVEL_3")
        await r(a, 21, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_QUICK"})
        self.assertEqual(sorted(ea[-2][1]), ["msgid", "result", "rqid"])  # START solo acusa recibo
        self.assertEqual(ea[-1][1]["matching_state"], "SEARCHING")
        await r(b, 30, {"msgid": "CMD_START_MATCHING", "rqid": 3, "matching_type": "OC_QUICK"})
        for e in (ea, eb):
            m = e[-1][1]
            self.assertEqual((m["msgid"], m["rqid"], m["matching_state"], m["player_num"]),
                             ("CMD_WATCH_MATCHING", 0, "FINISH", 2))
            lados = [msgpack_min.unpackb(msgpack_min.packb(x.valor))["side"] for x in m["member"]]
            self.assertEqual(lados, ["HOME", "AWAY"])
        await r(a, 22, {"msgid": "CMD_SET_GAMERELAY_CHECK_RES", "rqid": 4})
        self.assertNotEqual(ea[-1][1]["msgid"], "CMD_WATCH_GAMERELAY_CHECK_RES")
        await r(b, 31, {"msgid": "CMD_SET_GAMERELAY_CHECK_RES", "rqid": 5})
        for e in (ea, eb):
            self.assertEqual(e[-1][1]["error_code"], "NOERR")

    async def test_pareja_abre_partida_en_el_rele(self):
        import base64, struct
        from phoenix_online import gamerelay as g
        rele = g.Rele(None, bytes(256), bytes(16))
        self.srv.rele = rele
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        r = self.srv.recibir
        sa = base64.b64encode(struct.pack(">H", 50001) + b"\0\0" + bytes([100, 64, 0, 1]) + b"\0" * 6).decode()
        await r(a, 19, {"msgid": "CMD_SEND_IPANDPORT_NTL", "rqid": 9, "reflexive_addr": {"sa_data": sa}})
        await r(a, 21, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_RANK"})
        await r(b, 30, {"msgid": "CMD_START_MATCHING", "rqid": 3, "matching_type": "OC_RANK"})
        [(room, it)] = rele.intentos.items()
        self.assertGreater(room, 700000000)
        self.assertEqual(it.ids, {"00000000000000a1": 0, "00000000000000b2": 1})
        self.assertEqual(it.miembros[0]["endpoints"](), [("100.64.0.1", 50001)])
        datos = base64.b64encode(bytes([2, 0, 15]) + (0).to_bytes(4, "big") + (3).to_bytes(2, "big")
                                 + b"abc" + b"\0" * 20).decode()
        await r(b, 32, {"msgid": "CMD_SEND_GAME_SEQUENCE_DATA", "rqid": 6, "game_seq_data": [
            {"type": 2, "fromId": 1, "channelId": 15, "sequenceNumber": 0, "data": datos, "data_len": 32}]})
        self.assertEqual(eb[-1][1]["result"], "NOERR")
        self.assertEqual(it.contados[("00000000000000b2", 15, b"abc")], [0, 1])
        # un fallo en el chequeo no libera la barrera; la salida de los dos cierra la partida
        await r(a, 22, {"msgid": "CMD_SET_GAMERELAY_CHECK_RES", "rqid": 4, "error_code": "ER_CONNECT"})
        self.assertNotEqual(ea[-1][1]["msgid"], "CMD_WATCH_GAMERELAY_CHECK_RES")
        await r(a, 23, {"msgid": "CMD_LEAVE_GAME", "rqid": 5})
        await self.srv.cerrar(b)
        self.assertEqual(rele.intentos, {})
        self.assertIn("rele", self.srv.estado())

    def test_lista_calidad_http_autoriza_sonda(self):
        r = self.srv.respuesta_http({"msgid": "CMD_GET_GAMERELAY_QUALITYCHECKLIST", "rqid": "5",
                                     "pid": "7"}, "10.0.0.2")
        self.assertEqual((r["result"], r["quality_check_data_num"], r["quality_list_num"]), ("NOERR", 1, 0))
        nodo = r["quality_check_data_list"][0]
        self.assertIsInstance(nodo, dict)  # C: como bin el cliente no lo lee
        self.assertEqual((r["quality_threshold_1"], r["quality_threshold_2"]), (150, 200))
        self.assertEqual((nodo["gamerelay_index"], nodo["port"], nodo["grid"]), (1, 5100, 0))
        self.assertTrue(self.srv.sonda_autorizada("10.0.0.2", 1))
        self.assertFalse(self.srv.sonda_autorizada("10.0.0.3", 1))

    async def test_set_gamerelay_quality_y_revocar(self):
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 30, {"msgid": "CMD_GET_GAMERELAY_QUALITYCHECKLIST", "rqid": 7})
        self.assertEqual(ea[-1][1]["quality_check_data_num"], 1)
        item = msgpack_min.packb({"gamerelay_index": 1, "quality": 18, "last_update": 1})
        await self.srv.recibir(a, 31, {"msgid": "CMD_SET_GAMERELAY_QUALITY", "rqid": 8,
                                       "quality_list": [item]})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        self.assertEqual(self.srv.calidades[str(self.srv._pid_visible(a))][1]["quality"], 18)
        self.assertTrue(self.srv.sondas)
        await self.srv.cerrar(a)
        self.assertFalse(self.srv.sondas)

    async def test_vestibulo_blocklist_y_url_relay(self):
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 40, {"msgid": "CMD_GET_LOBBYLIST", "rqid": 1, "lobby_type": "FREE"})
        for x in ea[-1][1]["lobby_list"]:
            self.assertGreater(msgpack_min.unpackb(msgpack_min.packb(x.valor))["gamerelay_index"], 0)
        await self.srv.recibir(a, 41, {"msgid": "CMD_JOIN_LOBBY", "rqid": 2, "lobby_id": 1})
        self.assertTrue(ea[-1][1]["gamerelay_web_url"].endswith("/gamerelay"))
        await self.srv.recibir(a, 42, {"msgid": "CMD_GET_BLOCKLIST", "rqid": 3})
        r = ea[-1][1]
        self.assertEqual((r["count"], msgpack_min.unpackb(msgpack_min.packb(r["bklist"][0].valor))["block_id"]),
                         (1, 1))
        await self.srv.recibir(a, 43, {"msgid": "CMD_GET_SERVER_ENV", "rqid": 4})
        self.assertIn("MatchCommandDropMode", ea[-1][1]["proto_opt"])
        self.assertNotIn("P2P", ea[-1][1]["proto_opt"])
        await self.srv.recibir(a, 44, {"msgid": "CMD_JOIN_ONLINE_MODE", "rqid": 5, "online_mode": "RANKMATCH"})
        await self.srv.recibir(a, 45, {"msgid": "CMD_GET_SERVER_ENV", "rqid": 6})
        self.assertIn('"P2P_LOW_LEVEL":{"Match":{"ActualMatchHzFatalConditionDeterminationTimeUs":60000000}}',
                      ea[-1][1]["proto_opt"])

    async def test_salon_propio_lista_de_jugadores(self):
        s, env = cliente(self.srv)
        r = self.srv.recibir
        await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "A1"})
        await r(s, 2, {"msgid": "CMD_CREATE_PLAYER", "rqid": 2})
        await r(s, 3, {"msgid": "CMD_REQWATCHL_ROOMLIST", "rqid": 3})
        await r(s, 4, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 4, "name": "Salón", "vs_com": "PL_VS_PL"})
        await r(s, 5, {"msgid": "CMD_REQWATCHL_ROOMPLAYERLIST_NTL", "rqid": 5, "room_id": 1})
        m = env[-1][1]
        self.assertEqual((m["msgid"], m["rqid"], m["list_type"], m["list_num"]),
                         ("CMD_REQWATCHL_ROOMPLAYERLIST_NTL", 5, "RELOAD", 1))
        fila = msgpack_min.unpackb(msgpack_min.packb(m["list_data"][0].valor))
        self.assertEqual((fila["is_owner"], fila["room_entry_no"], fila["side"]), ("YES", 1, "HOME"))
        self.assertIsInstance(fila["pname"], bytes)

    async def test_division_info_cuenta_nueva(self):
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 50, {"msgid": "CMD_GET_DIVISION_INFO", "rqid": 1})
        info = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["division_info"].valor))
        self.assertEqual((info["division"], info["other_match_num"]), ("D12", 1))

    async def test_bloque_salir_y_revisar_texto(self):
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 60, {"msgid": "CMD_JOIN_LOBBY", "rqid": 1, "lobby_id": 1})
        await r(a, 61, {"msgid": "CMD_JOIN_BLOCK", "rqid": 2, "block_id": 1})
        self.assertEqual((ea[-1][1]["result"], a.block_id), ("NOERR", 1))
        await r(a, 62, {"msgid": "CMD_CHECK_STRING", "rqid": 3, "str": "Sala de Frank"})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 63, {"msgid": "CMD_CHECK_STRING", "rqid": 4, "str": "una mierda"})
        self.assertEqual(ea[-1][1]["result"], "ERR_NGWORD")
        await r(a, 64, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 5, "name": "S"})
        self.assertEqual(len(self.srv.salas), 1)
        await r(a, 65, {"msgid": "CMD_LEAVE_LOBBY", "rqid": 6})
        self.assertEqual((ea[-1][1]["msgid"], ea[-1][1]["result"]), ("CMD_LEAVE_LOBBY", "NOERR"))
        self.assertEqual((len(self.srv.salas), a.lobby_id, a.block_id), (0, None, None))

    async def test_perfil_guardar_y_leer(self):
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        perfil = msgpack_min.packb({"favorite_team": "Alianza", "favorite_player": "Cueto",
                                    "intro": "Hola", "date": 0})
        await r(a, 70, {"msgid": "CMD_SET_PLAYERPROFILE", "rqid": 1, "profile": perfil})
        self.assertEqual(ea[-1][1]["error_intro"], "")
        await r(a, 71, {"msgid": "CMD_GET_PLAYERINFO", "rqid": 2, "pid": self.srv._pid_visible(a)})
        m = ea[-1][1]
        self.assertEqual((m["favoriteTeam"], m["favoritePlayer"], m["intro"]),
                         ("Alianza", "Cueto", "Hola"))
        est = msgpack_min.unpackb(msgpack_min.packb(m["rankmatch_stats"].valor))
        self.assertEqual((est["division"], est["rating"]), ("D12", 1000))
        pn = msgpack_min.unpackb(msgpack_min.packb(m["pname"].valor))
        self.assertIsInstance(pn["psn_user_id"], bytes)
        self.assertEqual(msgpack_min.unpackb(msgpack_min.packb(m["manner"].valor))["manner"], "A")  # R: grado real de cortesía (pesmanner AzrakHQ)

    async def test_registros_y_ranking(self):
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        r = self.srv.recibir
        await r(a, 80, {"msgid": "CMD_GET_DIVISION_POINT_INFO", "rqid": 1, "pid": 1})
        lista = ea[-1][1]["division_list"]
        self.assertEqual(len(lista), 12)
        self.assertEqual(msgpack_min.unpackb(msgpack_min.packb(lista[0].valor))["division_type"], "D12")
        await r(a, 81, {"msgid": "CMD_GET_GAMERESULTS", "rqid": 2})
        self.assertEqual(ea[-1][1]["no_match"], "YES")
        await r(a, 82, {"msgid": "CMD_GET_OFTEN_USED_TEAM_LIST", "rqid": 3})
        self.assertEqual(ea[-1][1]["used_team_id_list_num"], 0)
        for x in (a, b):  # solo salen quienes jugaron Divisiones (o quien pregunta)
            self.srv.datos.guardar_division(x.steam_hex, {"jugados": 1})
        await r(a, 83, {"msgid": "CMD_GET_DIVISION_RANKING", "rqid": 4, "division_type": "D12"})
        m = ea[-1][1]
        self.assertEqual((m["total_rank_num"], m["rank_list"][1]["rank"]), (2, 2))
        self.assertIsInstance(m["rank_list"][0]["user"], dict)
        await r(a, 84, {"msgid": "CMD_GET_DIVISION_RANKING", "rqid": 5, "division_type": "D1"})
        self.assertEqual(ea[-1][1]["total_rank_num"], 0)

    async def test_avisos_y_chat_quedan_abiertos(self):
        a, ea = await self.entrar("A1")
        antes = len(ea)
        for w in ("CMD_WATCH_SYSTEM_NOTICE", "CMD_WATCH_SYNC_NOTICE", "CMD_WATCH_TEXTCHAT"):
            await self.srv.recibir(a, 90, {"msgid": w, "rqid": 7})
        self.assertEqual(len(ea), antes)

    async def test_myclub_vacio_con_tipos(self):
        a, ea = await self.entrar("A1")
        casos = {"CMD_GET_MYCLUB_SERVER_ENV": "is_all_club_edition_set",
                 "CMD_GET_MYCLUB_USER_INFO": "progress_status",
                 "CMD_GET_MYCLUB_FEATURE_PLAYER_LIST": "hold_gameplayer_list_num",
                 "CMD_GET_MYCLUB_GAMEPLAYERLIST": "gameplayer_list_max",
                 "CMD_GET_MYCLUB_COACHLIST": "coach_list_num",
                 "CMD_GET_MYCLUB_SQUADLIST": "main_squad_id",
                 "CMD_GET_MYCLUB_PROCURABLE_GAMEPLAYERLIST": "gameplayer_list_num",
                 "CMD_REQWATCH_MYCLUB_AUCTION_REFUNDED_GP": "total_gp",
                 "CMD_SET_MYCLUB_VSCOM_GAMERESULT": "player_data",
                 "CMD_GET_MYCLUB_MAINMENU_INFO": "matchday_emblem_info",
                 "CMD_GET_MYCLUB_LOGIN_BONUS": "is_bonus",
                 "CMD_GET_MYCLUB_ITEMLIST": "item_list_num",
                 "CMD_GET_MYCLUB_SQUAD_GAMEPLAN": "gameplan_data",
                 "CMD_SET_MYCLUB_MAIN_SQUAD": "result",
                 "CMD_SET_MYCLUB_CLOUD_SQUAD": "result",
                 "CMD_GET_MYCLUB_PLAYER_NUMBER_FIXATION": "player_number_list_num",
                 "CMD_GET_MYCLUB_SCOUTLIST": "scout_list_num",
                 "CMD_GET_MYCLUB_WEEKLY_RANKING_BASIC_INFO": "basic_info",
                 "CMD_GET_MYCLUB_CLOUDMATCH_BONUS": "match_list_num",
                 "CMD_GET_MYCLUB_MARKET_AGENTLIST": "market_subscription",
                 "CMD_GET_MYCLUB_MARKET_COACHLIST": "end_time",
                 "CMD_GET_MYCLUB_REFINE_SEARCH_AUCTION_LIST": "total_auction_info_list_num",
                 "CMD_GET_MYCLUB_AUCTION_WATCH_LIST": "auction_info_list",
                 "CMD_GET_MYCLUB_AUCTION_WISH_LIST": "total_wish_info_list_num",
                 "CMD_GET_MYCLUB_AUCTION_PRICE_RANKING": "auction_price_ranking_num",
                 "CMD_GET_MYCLUB_RENTAL_GAMEPLAYERLIST": "rental_gameplayer_list_num",
                 "CMD_GET_MATCHREJECTLIST": "reject_list_num",
                 "CMD_GET_EVENT_CATEGORY_LIST": "event_category_list",
                 "CMD_GET_PESLEAGUE_COMPE_RESULT_LIST": "result_list_num",
                 "CMD_GET_MYCLUB_VSCOM_OPPONENT": "team_data"}
        for i, (cmd, clave) in enumerate(casos.items()):
            await self.srv.recibir(a, 100 + i, {"msgid": cmd, "rqid": i, "pid": 1})
            self.assertEqual((ea[-1][1]["msgid"], ea[-1][1]["result"]), (cmd, "NOERR"))
            self.assertIn(clave, ea[-1][1])
        msgpack_min.packb(ea[-1][1])  # todo se puede empaquetar

    async def test_myclub_crear_club(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {"progreso_sin_club": "X_SIN", "progreso_con_club": "X_CON"}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1, "pid": 1})
        self.assertEqual(ea[-1][1]["progress_status"], "X_SIN")
        casa = msgpack_min.packb({"nation_id": 5, "label": "L", "name": "Lima"})
        await r(a, 2, {"msgid": "CMD_CREATE_MYCLUB_TEAM", "rqid": 2, "pid": 1, "team_name": "Phoenix FC",
                       "team_name_short": "PHX", "base_team_id": 7, "home_town": casa})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 3, {"msgid": "CMD_CREATE_MYCLUB_TEAM", "rqid": 3, "team_name": "Otra"})
        self.assertEqual(ea[-1][1]["result"], "ERR_ALREADYEXISTS")
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 4, "pid": 1})
        m = ea[-1][1]
        self.assertEqual((m["progress_status"], m["team_name"], m["base_team_id"]), ("X_CON", "Phoenix FC", 7))
        self.assertEqual(msgpack_min.unpackb(msgpack_min.packb(m["home_town"].valor))["name"], "Lima")
        await r(a, 5, {"msgid": "CMD_CREATE_MYCLUB_SQUAD", "rqid": 5, "name": "A"})
        self.assertEqual(ea[-1][1]["squad_id"], 1)
        jug = msgpack_min.packb({"gameplayer_id": 1234, "overall": 70})
        await r(a, 6, {"msgid": "CMD_CREATE_MYCLUB_STAFF", "rqid": 6, "player_info_list": [jug]})
        el = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["staff_info_list"][0].valor))
        self.assertIsInstance(el["base_player_info"], bytes)
        self.assertEqual(el["id"], 1)

    async def test_myclub_plantel_desde_config(self):
        jug = [{"gameplayer_id": 100 + i, "rating": 90} for i in range(11)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {"jugadores": jug,
                                                    "entrenadores": [{"coach_id": 5}]}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYERLIST", "rqid": 1})
        m = ea[-1][1]
        self.assertEqual(m["gameplayer_list_num"], 11)
        f = msgpack_min.unpackb(msgpack_min.packb(m["gameplayer_list"][0].valor))
        self.assertIsInstance(f["concept"], bytes)
        self.assertEqual((f["gameplayer_id"], f["rating"]), (100, 90))
        self.assertEqual(f["default_rarity"], "PLAYER_RARITY_BLACK")
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_COACHLIST", "rqid": 2})
        self.assertEqual(ea[-1][1]["coach_list_num"], 1)
        await r(a, 3, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 3})
        m = ea[-1][1]
        self.assertEqual((m["squad_list_num"], m["main_squad_id"]), (1, 1))
        sq = msgpack_min.unpackb(msgpack_min.packb(m["squad_list"][0].valor))
        self.assertEqual(sq["member_num"], 11)
        self.assertEqual(sq["gameplan_data_size"], 536)
        msgpack_min.packb(m)

    async def test_myclub_gameplan_se_guarda(self):
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_SET_MYCLUB_SQUAD_GAMEPLAN", "rqid": 1, "squad_id": 1,
                       "gameplan_data": "AQJwbGFu", "gameplan_data_size": 6})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_SQUAD_GAMEPLAN", "rqid": 2, "squad_id": 1})
        # C (05-nov): texto base64 tal cual lo mandó el cliente; size = bytes sin base64
        self.assertEqual((ea[-1][1]["gameplan_data"], ea[-1][1]["gameplan_data_size"]),
                         ("AQJwbGFu", 6))
        await r(a, 3, {"msgid": "CMD_GET_MYCLUB_SQUAD_GAMEPLAN", "rqid": 3, "squad_id": 9})
        self.assertIsInstance(ea[-1][1]["gameplan_data"], str)
        self.assertEqual(ea[-1][1]["gameplan_data_size"], 536)

    async def test_myclub_inscritos_orden(self):
        jug = [{"gameplayer_id": 100 + i} for i in range(23)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {"jugadores": jug}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 1})
        sq = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["squad_list"][0].valor))
        self.assertEqual(sq["member_num"], 18)
        lista = [msgpack_min.packb({"gameplayer_unique_id": u}) for u in [2, 1] + list(range(3, 19))]
        await r(a, 2, {"msgid": "CMD_SET_MYCLUB_SQUAD_GAMEPLAYER", "rqid": 2, "squad_id": 1,
                       "member_list": lista, "member_list_num": 18})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 3, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 3})
        sq = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["squad_list"][0].valor))
        primero = msgpack_min.unpackb(sq["member_list"][0])
        self.assertEqual(primero["gameplayer_id"], 101)

    async def test_myclub_parametros_por_jugador(self):
        a, ea = await self.entrar("A1")
        pedidos = [msgpack_min.packb({"gameplayer_unique_id": i, "gameplayer_id": 100 + i}) for i in range(3)]
        await self.srv.recibir(a, 1, {"msgid": "CMD_GET_PLAYER_PARAMETERLIST", "rqid": 1,
                                      "player_list": pedidos, "is_equalization": "YES"})
        lista = ea[-1][1]["param_playerlist"]
        self.assertEqual(len(lista), 3)
        el = msgpack_min.unpackb(msgpack_min.packb(lista[2].valor))
        self.assertEqual(msgpack_min.unpackb(el["gameplayer_id_info"])["gameplayer_id"], 102)

    async def test_myclub_rival_com_tipos(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {"rival": {
            "jugadores": [{"gameplayer_id": 500 + i} for i in range(18)], "coach_id": 24}}}
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 1, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 1})
        r = ea[-1][1]
        self.assertEqual(r["coach_adaptability_rate"], [100])
        self.assertEqual(r["cpu_level"], "MATCH_LEVEL_REGULAR")
        self.assertEqual(r["concept_data"], [])
        self.srv.cfg["myclub"]["rival"]["con_conceptos"] = True
        await self.srv.recibir(a, 2, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 2})
        r = ea[-1][1]
        self.assertEqual(len(r["concept_data"]), 18)
        el = msgpack_min.unpackb(msgpack_min.packb(r["concept_data"][3].valor))
        self.assertEqual(el["gameplayer_id"], 503)
        self.assertEqual((r["gameplan_data_size"], type(r["gameplan_data"])), (536, str))

    async def test_myclub_club_propio_por_cuenta_y_gp(self):
        jug = [{"gameplayer_id": 100 + i} for i in range(18)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": jug, "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "Phoenix FC", "gp": 1000},
            "gp_por_partido": {"WIN": 300, "DRAW": 100, "LOSE": 50}}}
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        await self.srv.recibir(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        await self.srv.recibir(b, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        self.assertEqual((ea[-1][1]["gp"], eb[-1][1]["gp"]), (1000, 1000))
        await self.srv.recibir(a, 2, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": 2,
                                      "winner_side": "WINNER_HOME",
                                      "gameend_reason": "NORMAL_END", "cpu_level": "NORMAL",
                                      "past_game_played_pid": 7, "pes_srv_rtt_nums": 3})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await self.srv.recibir(a, 21, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": 21,
                                       "winner_side": "WINNER_NONE", "gameend_reason": "NORMAL_END",
                                       "past_game_played_pid": 0, "pes_srv_rtt_nums": 0})
        await self.srv.recibir(a, 3, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 3})
        await self.srv.recibir(b, 3, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 3})
        self.assertEqual((ea[-1][1]["gp"], eb[-1][1]["gp"]), (1300, 1000))
        await self.srv.recibir(a, 22, {"msgid": "CMD_GET_MYCLUB_GAME_BONUS", "rqid": 22})
        self.assertEqual((ea[-1][1]["gain_gp"], ea[-1][1]["game_res"], ea[-1][1]["is_first_win"]),
                         (300, "WIN", "YES"))
        await self.srv.recibir(a, 23, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYER_EXPERIENCE", "rqid": 23})
        f0 = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["gameplayer_list"][0].valor))
        self.assertEqual(msgpack_min.unpackb(f0["level_info"])["exp_total"], 60)
        await self.srv.recibir(a, 24, {"msgid": "CMD_GET_MYCLUB_MAIN_SQUAD", "rqid": 24, "target_pid": 1})
        self.assertEqual(ea[-1][1]["gameplayer_list_num"], 0)  # cabe en una trama
        await self.srv.recibir(a, 4, {"msgid": "CMD_GET_MYCLUB_MAINMENU_INFO", "rqid": 4})
        self.assertEqual((ea[-1][1]["com_match_num"], ea[-1][1]["com_countWin_num"]), (1, 1))
        await self.srv.recibir(a, 5, {"msgid": "CMD_GET_GP_ACHIEVEMENT", "rqid": 5})
        self.assertEqual(ea[-1][1]["all_gp"], 1300)
        await self.srv.recibir(a, 6, {"msgid": "CMD_GET_MYCLUB_COMMENTARY_INFO", "rqid": 6,
                                      "match_category": "VSCOM_MATCH", "team_spirit": 1})
        self.assertEqual((ea[-1][1]["total_match_num"], ea[-1][1]["cont_win_num"]), (1, 1))
        self.assertEqual(self.srv._sumar_gp(b, -2000), None)
        self.assertEqual(self.srv._sumar_gp(b, -200), 800)

    async def test_myclub_dificultad_sube_y_baja(self):
        # C (05-nov noche): nivel de la COM según rachas (config myclub.dificultad)
        niveles = ["N1", "N2", "N3"]
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(18)],
            "entrenadores": [{"coach_id": 8}], "club_de_prueba": {"team_name": "P", "gp": 0},
            "rival": {"jugadores": [{"gameplayer_id": 500 + i} for i in range(18)]},
            "dificultad": {"activa": True, "niveles": niveles, "victorias_para_subir": 2,
                           "derrotas_para_bajar": 2}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})

        async def partido(ganador, n):
            await r(a, n, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": n,
                           "winner_side": ganador, "gameend_reason": "NORMAL_END",
                           "past_game_played_pid": 7, "pes_srv_rtt_nums": 3})
            await r(a, n + 1, {"msgid": "CMD_GET_MYCLUB_GAME_BONUS", "rqid": n + 1})
            bono = ea[-1][1]
            await r(a, n + 2, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": n + 2})
            return bono["old_cpu_level"], bono["new_cpu_level"], ea[-1][1]["cpu_level"]

        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 2})
        self.assertEqual(ea[-1][1]["cpu_level"], "N1")
        self.assertEqual(await partido("WINNER_HOME", 10), ("N1", "N1", "N1"))
        self.assertEqual(await partido("WINNER_HOME", 20), ("N1", "N2", "N2"))  # 2 seguidas
        self.assertEqual(await partido("WINNER_AWAY", 30), ("N2", "N2", "N2"))
        self.assertEqual(await partido("WINNER_AWAY", 40), ("N2", "N1", "N1"))  # 2 derrotas
        self.srv.cfg["myclub"]["dificultad"]["activa"] = False
        await r(a, 50, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 50})
        self.assertEqual(ea[-1][1]["cpu_level"], "MATCH_LEVEL_REGULAR")  # apagada = fijo

    async def test_myclub_contrato_y_cansancio_por_partido(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(20)],
            "entrenadores": [{"coach_id": 8}], "club_de_prueba": {"team_name": "P", "gp": 0},
            "contrato_por_partido": 1,
            "cansancio": {"activo": True, "sube": 30, "baja": 10, "maximo": 50}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        club = self.srv.datos.club(a.steam_hex)
        club["jugadores"][19]["tiredness"] = 15
        self.srv.datos.guardar_club(a.steam_hex, club)
        for n in (2, 3):
            await r(a, n, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": n,
                           "winner_side": "WINNER_HOME", "gameend_reason": "NORMAL_END",
                           "past_game_played_pid": 7, "pes_srv_rtt_nums": 3})
        j = self.srv.datos.club(a.steam_hex)["jugadores"]
        self.assertEqual((j[0]["contract_term"], j[0]["tiredness"]), (720, 50))  # 10 partidos - 2; tope 50
        self.assertNotIn("contract_term", j[19])  # no inscrito: no gasta contrato
        self.assertEqual(j[19]["tiredness"], 0)  # descansa 10 por partido, sin bajar de 0

    def test_contrato_escala_y_migracion(self):
        # C (REA 05-nov noche): PES muestra contract_term // 90; club viejo guardado en partidos
        self.srv.cfg = {**self.srv.cfg, "myclub": {"renovacion_maximo": 50}}
        club = {"jugadores": [{"contract_term": 40}, {"contract_term": 100}, {}]}
        self.assertTrue(self.srv._migrar_contratos(club))
        self.assertEqual([j.get("contract_term") for j in club["jugadores"]], [3600, 4500, None])
        self.assertFalse(self.srv._migrar_contratos(club))  # solo una vez
        self.assertEqual(self.srv._contrato(club["jugadores"][2]), 900)  # 10 partidos por defecto

    def test_reglas_myclub_en_caliente(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {"venta_gp": 100, "renovacion_gp": 200}}
        cambiadas = self.srv.aplicar_reglas_myclub({"venta_gp": 150, "renovacion_gp": 200})
        self.assertEqual(cambiadas, ["venta_gp"])
        self.assertEqual(self.srv.cfg["myclub"]["venta_gp"], 150)
        self.assertEqual(self.srv.aplicar_reglas_myclub("roto"), [])

    def test_myclub_desenlace_vscom(self):
        d = self.srv._desenlace_vscom
        self.assertEqual(d({"winner": "WINNER_AWAY", "side": "HOME"}), "LOSE")
        self.assertEqual(d({"winner_side": "WINNER_NONE"}), "DRAW")
        self.assertEqual(d({"winner_side": "WINNER_AWAY"}), "LOSE")
        self.assertEqual(d({"home_score": 1, "away_score": 2, "side": "AWAY"}), "WIN")
        self.assertEqual(d({}), "")
        self.assertEqual(d({"winner_side": "WINNER_HOME", "gameend_reason": "GIVEUP"}), "LOSE")
        self.srv.cfg = {**self.srv.cfg, "myclub": {"abandono_es_derrota": False}}
        self.assertEqual(d({"winner_side": "WINNER_HOME", "gameend_reason": "GIVEUP"}), "WIN")

    async def test_myclub_tienda_y_mercado_dt(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {"mercado_dt": [24, {"coach_id": 37, "gp": 5000}]}}
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 1, {"msgid": "CMD_CHECK_MYCLUB_STORE", "rqid": 1})
        self.assertEqual(ea[-1][1]["is_enable_myclub_coin"], "NO")
        await self.srv.recibir(a, 2, {"msgid": "CMD_GET_MYCLUB_MARKET_COACHLIST", "rqid": 2})
        lista = ea[-1][1]["market_coach_list"]
        self.assertEqual(len(lista), 2)
        dt = msgpack_min.unpackb(msgpack_min.packb(lista[1].valor))
        self.assertEqual((dt["coach_id"], dt["gp"]), (37, 5000))

    async def test_myclub_fichar_dt_con_gp(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 6000},
            "mercado_dt": [{"coach_id": 24, "gp": 5000}, 37]}}
        a, ea = await self.entrar("A1")
        self.srv.datos.guardar_club(a.steam_hex, {"team_name": "P", "gp": 6000})  # club viejo
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_BUY_MYCLUB_MARKET_COACH", "rqid": 1, "coach_id": 24,
                       "use_pes_point": "NO", "is_coach_texture_use": "YES"})
        self.assertEqual(ea[-1][1]["gp"], 1000)
        dt = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["coach"].valor))
        self.assertEqual((dt["coach_id"], dt["coach_unique_id"]), (24, 2))
        await r(a, 2, {"msgid": "CMD_BUY_MYCLUB_MARKET_COACH", "rqid": 2, "coach_id": 24})
        self.assertEqual(ea[-1][1]["result"], "ERR_NOTFOUND")
        await r(a, 3, {"msgid": "CMD_GET_MYCLUB_COACHLIST", "rqid": 3})
        self.assertEqual(ea[-1][1]["coach_list_num"], 2)
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_MARKET_COACHLIST", "rqid": 4})
        self.assertEqual(ea[-1][1]["market_coach_list_num"], 1)
        await r(a, 5, {"msgid": "CMD_BUY_MYCLUB_MARKET_COACH", "rqid": 5, "coach_id": 37,
                       "use_pes_point": "YES"})
        self.assertEqual(ea[-1][1]["result"], "ERR_FORBIDDEN")

    async def test_myclub_gestion_entrenadores_y_plantillas(self):
        """B.15: entrenador por plantilla, despedir y renovar entrenador, copiar y borrar plantilla,
        dorsales, base y ciudad."""
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(20)],
            "entrenadores": [{"coach_id": 8}], "venta_dt_gp": 700, "renovacion_gp": 100,
            "club_de_prueba": {"team_name": "P", "gp": 6000},
            "mercado_dt": [{"coach_id": 24, "gp": 1000}]}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_BUY_MYCLUB_MARKET_COACH", "rqid": 1, "coach_id": 24,
                       "use_pes_point": "NO"})
        self.assertEqual(res()["gp"], 5000)
        # el titular (uid 1) no se puede despedir; el nuevo (uid 2) sí, hasta que dirija una plantilla
        await r(a, 2, {"msgid": "CMD_SELL_MYCLUB_COACH", "rqid": 2, "coach_unique_id": 1})
        self.assertEqual(res()["result"], "ERR_FORBIDDEN")
        await r(a, 3, {"msgid": "CMD_SET_MYCLUB_SQUAD_COACH", "rqid": 3, "squad_id": 1,
                       "coach_unique_id": 2, "tacticsPlanKind": 0, "gameplan_data_size": 3,
                       "gameplan_data": "QUJD"})
        self.assertEqual(res()["result"], "NOERR")
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 4})
        sq = msgpack_min.unpackb(msgpack_min.packb(res()["squad_list"][0].valor))
        self.assertEqual((sq["coach_id_list"], sq["coach_unique_id_list"], sq["gameplan_data"]),
                         ([24], [2], "QUJD"))
        await r(a, 5, {"msgid": "CMD_SELL_MYCLUB_COACH", "rqid": 5, "coach_unique_id": 2})
        self.assertEqual(res()["result"], "ERR_FORBIDDEN")  # dirige la plantilla 1
        await r(a, 6, {"msgid": "CMD_SET_MYCLUB_SQUAD_COACH", "rqid": 6, "squad_id": 1,
                       "coach_unique_id": 1})
        await r(a, 7, {"msgid": "CMD_SELL_MYCLUB_COACH", "rqid": 7, "coach_unique_id": 2})
        self.assertEqual((res()["result"], res()["gain_gp"]), ("NOERR", 700))
        await r(a, 8, {"msgid": "CMD_UPDATE_MYCLUB_COACH_CONTRACT", "rqid": 8, "coach_unique_id": 1})
        self.assertEqual(res()["result"], "NOERR")
        self.assertGreater(res()["contract_term"], 0)
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual(club["gp"], 5000 + 700 - 100)
        # copiar y borrar plantillas (la 1 no se borra)
        await r(a, 9, {"msgid": "CMD_COPY_MYCLUB_SQUAD", "rqid": 9, "copy_squad_id": 1})
        self.assertEqual(res()["squad_id"], 2)
        await r(a, 10, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 10})
        self.assertEqual(res()["squad_list_num"], 2)
        dos = msgpack_min.unpackb(msgpack_min.packb(res()["squad_list"][1].valor))
        self.assertEqual((dos["squad_id"], dos["name"]), (2, "Plantilla 1 (2)"))
        await r(a, 92, {"msgid": "CMD_SET_MYCLUB_MAIN_SQUAD", "rqid": 92, "squad_id": 2})
        await r(a, 96, {"msgid": "CMD_SET_MYCLUB_CLOUD_SQUAD", "rqid": 96, "cloud_squad_id": 2,
                        "is_use_cloud_match": "YES"})
        await r(a, 93, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 93})
        self.assertEqual((res()["main_squad_id"], res()["cloud_squad_id"]), (2, 2))
        await r(a, 97, {"msgid": "CMD_GET_MYCLUB_CLOUD_SQUADID", "rqid": 97})
        self.assertEqual(res()["squad_id"], 2)
        await r(a, 11, {"msgid": "CMD_DELETE_MYCLUB_SQUAD", "rqid": 11, "squad_id": 1})
        self.assertEqual(res()["result"], "ERR_FORBIDDEN")
        await r(a, 12, {"msgid": "CMD_DELETE_MYCLUB_SQUAD", "rqid": 12, "squad_id": 2})
        self.assertEqual(res()["result"], "NOERR")
        await r(a, 13, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 13})
        self.assertEqual((res()["squad_list_num"], res()["main_squad_id"]), (1, 1))
        await r(a, 94, {"msgid": "CMD_COPY_MYCLUB_SQUAD", "rqid": 94, "copy_squad_id": 1,
                        "name": "Plantilla2"})
        await r(a, 95, {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 95})
        dos = msgpack_min.unpackb(msgpack_min.packb(res()["squad_list"][1].valor))
        self.assertEqual(dos["name"], "Plantilla2")
        # dorsales, base del equipo y ciudad
        el = msgpack_min.packb({"player_id": 3, "number": 10})
        await r(a, 14, {"msgid": "CMD_SET_MYCLUB_PLAYER_NUMBER_FIXATION", "rqid": 14,
                        "player_number_list_num": 1, "player_number_list": [el]})
        await r(a, 15, {"msgid": "CMD_SET_MYCLUB_TEAM_BASE", "rqid": 15, "base_team_id": 101})
        await r(a, 16, {"msgid": "CMD_SET_MYCLUB_HOMETOWN", "rqid": 16,  # forma real (captura)
                        "home_town": msgpack_min.packb({"nation_id": 54, "label": "LIM", "name": "Lima"})})
        club = self.srv.datos.club(a.steam_hex)
        j3 = next(j for j in club["jugadores"] if j.get("gameplayer_unique_id") == 3)
        self.assertEqual(j3["uniform_number"], 10)
        self.assertEqual(club["base_team_id"], 101)
        self.assertEqual(club["home_town"], {"name": "Lima", "nation_id": 54, "label": "LIM"})

    async def test_myclub_logros_con_premio(self):
        """B.19: solo categorías del cliente; progreso real; cobro en GP una sola vez; historial."""
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_ACHIEVEMENTLIST", "rqid": 1})
        self.assertEqual(res()["achievement_list_num"], 0)  # apagados
        self.srv.cfg["myclub"]["logros"] = {"activo": True, "premios": {
            "PLAY_VSCOM_MATCH": [[50, 10000], [100, 20000]], "CREATE_SQUAD": [[1, 1000]],
            "MATCH50": [[1, 99]]}}  # MATCH50 no es del cliente: se ignora
        club = self.srv.datos.club(a.steam_hex)
        club["vscom"] = {"partidos": 60, "racha_max": 3}
        club["logros_cobrados"] = [{"id": 1, "gp": 300}]  # los de GP_ACHIEVEMENT (lista) no estorban
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_ACHIEVEMENTLIST", "rqid": 2})
        self.assertEqual(res()["achievement_list_num"], 2)
        l = {x["achievement_category"]: x for x in
             (msgpack_min.unpackb(msgpack_min.packb(e.valor)) for e in res()["achievement_list"])}
        self.assertEqual(set(l), {"PLAY_VSCOM_MATCH", "CREATE_SQUAD"})
        self.assertEqual((l["PLAY_VSCOM_MATCH"]["is_new"], l["CREATE_SQUAD"]["is_new"]), ("YES", "NO"))
        self.assertEqual((l["PLAY_VSCOM_MATCH"]["value"], l["PLAY_VSCOM_MATCH"]["received_milestone_level"]),
                         (60, 0))
        hito = msgpack_min.unpackb(l["PLAY_VSCOM_MATCH"]["milestone_list"][1])
        self.assertEqual((hito["target_value"], hito["value"]), (100, 60))
        await r(a, 31, {"msgid": "CMD_GET_MYCLUB_MAINMENU_INFO", "rqid": 31})
        info = msgpack_min.unpackb(msgpack_min.packb(res()["achievement_info"].valor))
        self.assertEqual((info["achievement_total_num"], info["achievement_num"],
                          info["achievement_not_recieved_num"]), (3, 1, 1))
        await r(a, 3, {"msgid": "CMD_RECV_MYCLUB_ACHIEVEMENT_BONUS", "rqid": 3, "is_all": "YES"})
        self.assertEqual(len(res()["recv_achievement_list"]), 1)
        rec = msgpack_min.unpackb(msgpack_min.packb(res()["recv_achievement_list"][0].valor))
        self.assertEqual((rec["achievement_id"], rec["received_milestone_level"]), (2, 1))
        self.assertEqual(self.srv.datos.club(a.steam_hex)["gp"], 10000)
        await r(a, 4, {"msgid": "CMD_RECV_MYCLUB_ACHIEVEMENT_BONUS", "rqid": 4, "is_all": "YES"})
        self.assertEqual(res()["recv_achievement_list"], [])  # no se cobra dos veces
        self.assertEqual(self.srv.datos.club(a.steam_hex)["gp"], 10000)
        # copiar una plantilla cumple CREATE_SQUAD
        await r(a, 6, {"msgid": "CMD_COPY_MYCLUB_SQUAD", "rqid": 6, "copy_squad_id": 1, "name": "B"})
        await r(a, 7, {"msgid": "CMD_RECV_MYCLUB_ACHIEVEMENT_BONUS", "rqid": 7,
                       "achievement_category": "CREATE_SQUAD", "is_all": "NO"})
        self.assertEqual(self.srv.datos.club(a.steam_hex)["gp"], 11000)
        # en monedas (con monedas gratis): 1000 GP de meta = 10 monedas
        self.srv.cfg["myclub"]["monedas_gratis"] = True
        self.srv.cfg["myclub"]["logros"]["moneda"] = "monedas"
        self.srv.cfg["myclub"]["logros"]["premios"]["SET_CLOUD_MATCH_SQUAD"] = [[1, 1000]]
        await r(a, 8, {"msgid": "CMD_SET_MYCLUB_CLOUD_SQUAD", "rqid": 8, "cloud_squad_id": 1})
        await r(a, 9, {"msgid": "CMD_RECV_MYCLUB_ACHIEVEMENT_BONUS", "rqid": 9, "is_all": "YES"})
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual((club["gp"], club.get("monedas")), (11000, 10))
        self.srv.cfg["myclub"]["monedas_gratis"] = False
        # el cobro quedó en el historial de GP (visible con myclub.historial)
        self.srv.cfg["myclub"]["historial"] = True
        await r(a, 5, {"msgid": "CMD_GET_MYCLUB_HISTORYLIST", "rqid": 5, "start_index": 0})
        self.assertEqual((res()["myclub_history_list_total"], res()["myclub_history_list_num"]), (2, 2))
        reg = msgpack_min.unpackb(msgpack_min.packb(res()["myclub_history_list"][1].valor))
        self.assertEqual(reg["history_data_kind"], "GP")
        self.assertEqual(msgpack_min.unpackb(reg["history_data_out"])["count"], 10000)

    async def test_eventos_lista_inscripcion_y_baja(self):
        """Parte C: sin eventos = lista vacía; con uno de config: lista, detalle, inscribirse y salir."""
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_GET_COMPEV_LIST", "rqid": 1, "compe_list_kind": "X", "compe_mode": "EVENT_COMPE"})
        self.assertEqual((res()["compe_num"], res()["compe_list"]), (0, []))
        self.srv.cfg["myclub"]["eventos"] = [{"compe_id": 7, "compe_name": "Copa Phoenix"}]
        await r(a, 2, {"msgid": "CMD_GET_COMPEV_LIST", "rqid": 2, "compe_list_kind": "X", "compe_mode": "EVENT_COMPE"})
        el = msgpack_min.unpackb(msgpack_min.packb(res()["compe_list"][0].valor))
        self.assertEqual((el["compe_id"], el["compe_name"], el["difficulty"]), (7, "Copa Phoenix", "COMPE_DIFFICULTY_3"))
        await r(a, 3, {"msgid": "CMD_GET_COMPEV_INFO", "rqid": 3, "compe_mode": "EVENT_COMPE", "compe_id": 9})
        self.assertEqual(res()["result"], "ERR_COMPE_NOTFOUND")
        await r(a, 4, {"msgid": "CMD_JOIN_COMPEV", "rqid": 4, "compe_mode": "EVENT_COMPE", "compe_id": 7,
                       "compe_cpu_level": "COMPE_DIFFICULTY_2"})
        self.assertEqual(res()["result"], "NOERR")
        await r(a, 5, {"msgid": "CMD_GET_COMPEV_MYENTRY_COMPEID", "rqid": 5, "compe_mode": "EVENT_COMPE"})
        self.assertEqual((res()["compe_id"], res()["compe_cpu_level"]), (7, "COMPE_DIFFICULTY_2"))
        await r(a, 6, {"msgid": "CMD_GET_COMPEV_MAINMENU_INFO", "rqid": 6, "compe_mode": "EVENT_COMPE",
                       "compe_id": 7, "target_pid": 1})
        self.assertEqual(res()["status"], "ENTRY")
        await r(a, 7, {"msgid": "CMD_LEAVE_COMPEV", "rqid": 7, "compe_mode": "EVENT_COMPE"})
        self.assertEqual(res()["result"], "NOERR")
        await r(a, 8, {"msgid": "CMD_LEAVE_COMPEV", "rqid": 8, "compe_mode": "EVENT_COMPE"})
        self.assertEqual(res()["result"], "ERR_COMPE_NOTENTRY")

    async def test_eventos_partido_suma_progreso_y_supera(self):
        """C.8: partido vs COM con evento inscrito: marca, dificultad, progreso, superado y premio."""
        jug = [{"gameplayer_id": 100 + i} for i in range(18)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": jug, "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0},
            "eventos": [{"compe_id": 7, "compe_name": "Copa", "partidos": 2,
                         "puntos_para_superar": 4, "premio_gp": 5000, "premio_monedas": 30}],
            "monedas_gratis": True}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_GET_EVENT_CATEGORY_LIST", "rqid": 1})
        cat = msgpack_min.unpackb(msgpack_min.packb(res()["event_category_list"][0].valor))
        self.assertEqual((cat["open_event_num"], cat["not_cleared_event_num"]), (1, 1))
        await r(a, 21, {"msgid": "CMD_GET_MYCLUB_SQUAD_BONUS_TARGET_PLAYERLIST", "rqid": 21, "compe_id": 7})
        self.assertEqual((res()["gameplayer_id_list"], res()["gameplayer_id_list_num"],
                          res()["need_player_num"]), ([], 0, 0))
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 2})
        self.assertEqual(res()["is_entry_compe"], "NO")
        await r(a, 3, {"msgid": "CMD_JOIN_COMPEV", "rqid": 3, "compe_mode": "EVENT_COMPE", "compe_id": 7,
                       "compe_cpu_level": "COMPE_DIFFICULTY_3"})
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 4})
        self.assertEqual((res()["is_entry_compe"], res()["cpu_level"]), ("YES", "COMPE_DIFFICULTY_3"))
        partido = lambda q, w: r(a, q, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": q,
                                        "winner_side": w, "gameend_reason": "NORMAL_END",
                                        "past_game_played_pid": 7, "pes_srv_rtt_nums": 3})
        await partido(5, "WINNER_HOME")
        await r(a, 6, {"msgid": "CMD_GET_COMPEV_MAINMENU_INFO", "rqid": 6, "compe_mode": "EVENT_COMPE"})
        self.assertEqual((res()["progress_game_count"], res()["win_point"], res()["compe_result"],
                          res()["clear_count"]), (1, 3, "NORESULT", 0))
        await partido(7, "WINNER_NONE")
        await r(a, 8, {"msgid": "CMD_GET_COMPEV_MAINMENU_INFO", "rqid": 8, "compe_mode": "EVENT_COMPE"})
        self.assertEqual((res()["progress_game_count"], res()["win_point"], res()["compe_result"],
                          res()["clear_count"]), (2, 4, "FINISHWIN", 1))
        await r(a, 9, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 9})
        self.assertEqual(res()["gp"], 5000)
        await r(a, 10, {"msgid": "CMD_GET_COMPEV_INFO", "rqid": 10, "compe_mode": "EVENT", "compe_id": 7})
        from phoenix_online.logica import _cifrado_comprimido
        self.assertLess(_cifrado_comprimido(res()), 984)
        # C.11: en la raíz solo compe_data; dentro compe_info (evento + requisitos) y el detalle
        self.assertEqual(set(res()) - {"msgid", "rqid", "result"}, {"compe_data"})
        datos = res()["compe_data"].valor
        info = datos["compe_info"].valor
        self.assertEqual((info["compe_id"], info["is_cleared"], info["cleared_count"], info["entry_player_num"]),
                         (7, "YES", 1, 0))
        self.assertEqual((datos["clear_match_num"], datos["clear_win_point"]), (2, 4))
        self.assertEqual(len(datos["event_compe_reward_list"]), 2)
        self.assertIn("compe_reward", datos["event_compe_reward_list"][0].valor)
        await r(a, 101, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 101})
        self.assertEqual(res()["pes_point"], 30)
        await partido(11, "WINNER_HOME")  # ya superado: no suma ni cobra otra vez
        await r(a, 12, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 12})
        self.assertEqual(res()["gp"], 5000)
        await r(a, 13, {"msgid": "CMD_GET_EVENT_CATEGORY_LIST", "rqid": 13})
        cat = msgpack_min.unpackb(msgpack_min.packb(res()["event_category_list"][0].valor))
        self.assertEqual(cat["not_cleared_event_num"], 0)
        msgpack_min.packb(res())

    async def test_myclub_intercambio_y_objetos_de_entrenamiento(self):
        """B.21: intercambio de cartas, objetos de habilidad/posición/DT/táctico y valoración."""
        jug = [{"gameplayer_id": 100 + i} for i in range(20)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": jug, "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0}, "habilidades_objeto": [41]}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        club = self.srv.datos.club(a.steam_hex)
        uids = [j["gameplayer_unique_id"] for j in club["jugadores"]]
        en_uso = self.srv._uids_en_uso(club)
        libres = [u for u in uids if u not in en_uso]
        await r(a, 2, {"msgid": "CMD_TRADE_MYCLUB_GAMEPLAYER", "rqid": 2,
                       "my_player_trade_list": libres[:1], "target_trade_player": 7511})
        self.assertEqual(res()["result"], "ERR_FORBIDDEN")
        await r(a, 3, {"msgid": "CMD_TRADE_MYCLUB_GAMEPLAYER", "rqid": 3,
                       "my_player_trade_list": libres[:2], "target_trade_player": 7511})
        f = msgpack_min.unpackb(msgpack_min.packb(res()["gameplayer"].valor))
        self.assertEqual(f["gameplayer_id"], 7511)
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual(len(club["jugadores"]), len(uids) - 1)
        uid = club["jugadores"][0]["gameplayer_unique_id"]
        await r(a, 4, {"msgid": "CMD_USE_MYCLUB_ADD_SKILL_ITEM", "rqid": 4, "gameplayer_unique_id": uid})
        self.assertEqual(res()["result"], "ERR_FORBIDDEN")  # sin objetos
        club["objetos_habilidad"], club["objetos_gestion"], club["objetos_tactico"] = 1, 2, 1
        club["objetos_posicion"], club["objetos_adaptacion"] = 1, 1
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 5, {"msgid": "CMD_USE_MYCLUB_ADD_SKILL_ITEM", "rqid": 5, "gameplayer_unique_id": uid})
        self.assertEqual(res()["add_skill_id"], 41)
        await r(a, 6, {"msgid": "CMD_USE_MYCLUB_ADD_POSITION_APTITUDE_ITEM", "rqid": 6,
                       "gameplayer_unique_id": uid, "use_count": 1})
        self.assertEqual(res()["result"], "NOERR")
        cu = self.srv.datos.club(a.steam_hex)["entrenadores"][0].get("coach_unique_id", 1)
        await r(a, 7, {"msgid": "CMD_USE_MYCLUB_MANAGEMENT_ITEM", "rqid": 7, "coach_unique_id": cu, "use_count": 2})
        self.assertEqual(res()["management"], 72)
        await r(a, 8, {"msgid": "CMD_USE_MYCLUB_TACTICAL_TRAINING", "rqid": 8, "squad_id": 1,
                       "coach_unique_id": cu, "is_offensive_formation": True, "use_count": 1})
        self.assertGreater(len(res()["gameplayer_list"]), 0)
        msgpack_min.packb(res())
        await r(a, 81, {"msgid": "CMD_GET_MYCLUB_ITEMLIST", "rqid": 81})
        variantes = [msgpack_min.unpackb(msgpack_min.packb(o.valor))["item_variation"] for o in res()["item_list"]]
        self.assertIn("IMPROVE_TEAM_ADAPTABILITY_RATE", variantes)  # sin usar: sale
        self.assertNotIn("ADD_SKILL", variantes)
        await r(a, 9, {"msgid": "CMD_SET_POSITION_RATING", "rqid": 9, "position": "CF", "rating": 80})
        await r(a, 10, {"msgid": "CMD_GET_POSITION_RATING", "rqid": 10, "target_pid": 1})
        self.assertEqual((res()["total_position_rating"], res()["total_position_match_num"]), (80, 1))

    async def test_myclub_regalos_bandeja_y_cobro(self):
        """B.4: bienvenida de config, regalo de admin, lista, cobro por HTTP y logro RECV_PRESENT."""
        jug = [{"gameplayer_id": 100 + i} for i in range(18)]
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": jug, "entrenadores": [{"coach_id": 8}], "monedas_gratis": True,
            "club_de_prueba": {"team_name": "P", "gp": 0},
            "regalo_bienvenida": [{"tipo": "gp", "cantidad": 5000}],
            "logros": {"activo": True, "premios": {"RECV_PRESENT": [[1, 1000]]}}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        club = self.srv.datos.club(a.steam_hex)
        self.srv.dar_regalo(club, "monedas", 20, "Copa")
        self.srv.dar_regalo(club, "objetos_recuperar", 2)
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_PRESENTLIST", "rqid": 2})
        self.assertEqual(res()["present_list_num"], 3)
        tipos = [msgpack_min.unpackb(msgpack_min.packb(x.valor))["type"] for x in res()["present_list"]]
        self.assertEqual(sorted(tipos), ["GP", "ITEM", "PES_POINT"])
        primero = msgpack_min.unpackb(msgpack_min.packb(res()["present_list"][0].valor))["present_id"]
        resp = self.srv.respuesta_http({"msgid": "CMD_RECV_MYCLUB_PRESENT", "rqid": "3",
                                        "pid": str(a.pid), "present_id": str(primero), "is_all": "NO"})
        self.assertEqual(resp["receive_result_list_num"], 1)
        msgpack_min.packb(resp)
        resp = self.srv.respuesta_http({"msgid": "CMD_RECV_MYCLUB_PRESENT", "rqid": "4",
                                        "pid": str(a.pid), "present_id": "0", "is_all": "YES"})
        self.assertEqual(resp["receive_result_list_num"], 2)
        await r(a, 5, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 5})
        self.assertEqual((res()["gp"], res()["pes_point"]), (5000, 20))
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual((club["objetos_recuperar"], club["regalos"]), (2, []))
        await r(a, 6, {"msgid": "CMD_GET_MYCLUB_PRESENTLIST", "rqid": 6})
        self.assertEqual(res()["present_list_num"], 0)  # la bienvenida no se repite
        await r(a, 7, {"msgid": "CMD_GET_MYCLUB_ACHIEVEMENTLIST", "rqid": 7})
        self.assertEqual(res()["achievement_list_num"], 1)

    async def test_clanes_crear_entrar_salir(self):
        """D.8: crear clan, verlo en la lista, otro entra, ajustes, expulsar y disolver."""
        a, ea = await self.entrar("C1")
        b, eb = await self.entrar("C2")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_CLAN_INFO", "rqid": 1, "owner_id": 0})
        self.assertEqual(ea[-1][1]["clan_info"].valor["clan_member_num"], 0)
        await r(a, 2, {"msgid": "CMD_CREATE_CLAN", "rqid": 2, "clan_name": "Fénix"})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 3, {"msgid": "CMD_GET_COOP_MAINMENU_INFO", "rqid": 3})
        self.assertEqual(ea[-1][1]["is_exist_owner_clan"], "YES")
        await r(b, 4, {"msgid": "CMD_GET_CLAN_SIMPLE_INFO_LIST", "rqid": 4, "start_index": 0,
                       "search_options": msgpack_min.packb({"clan_name": "fén"})})
        lista = eb[-1][1]
        self.assertEqual((lista["clan_list_num"], lista["all_list_num"]), (1, 1))
        info = lista["clan_list"][0].valor
        self.assertEqual(info["basic_settings"].valor["clan_name"], "Fénix")
        id_dueno = info["basic_settings"].valor["owner_info"].valor["user_id"].valor["pid"]
        await r(b, 5, {"msgid": "CMD_GET_CLAN_SIMPLE_INFO_LIST", "rqid": 5, "owner_pid_list": [id_dueno]})
        self.assertEqual(eb[-1][1]["clan_simple_info_list_num"], 1)
        await r(b, 6, {"msgid": "CMD_ENTRY_CLAN", "rqid": 6, "target_pid": id_dueno})
        await r(b, 7, {"msgid": "CMD_GET_CLAN_MEMBER_MODE", "rqid": 7})
        self.assertEqual(eb[-1][1]["clan_member_num"], 2)
        msgpack_min.packb(eb[-1][1])
        await r(a, 8, {"msgid": "CMD_SET_CLAN_SETTINGS", "rqid": 8, "clan_basic_settings": msgpack_min.packb(
            {"clan_name": "Fénix 2", "clan_comment": "hola", "enable_request": "NO"})})
        await r(b, 9, {"msgid": "CMD_GET_CLAN_SETTINGS", "rqid": 9})
        aj = eb[-1][1]["clan_basic_settings"].valor
        self.assertEqual((aj["clan_name"], aj["clan_comment"], aj["enable_request"]), ("Fénix 2", "hola", "NO"))
        await r(b, 10, {"msgid": "CMD_GET_CLAN_INFO", "rqid": 10, "owner_id": id_dueno})
        miembros = eb[-1][1]["clan_info"].valor["clan_members"]
        id_b = [x.valor["user_id"].valor["pid"] for x in miembros if x.valor["user_id"].valor["pid"] != id_dueno][0]
        await r(a, 11, {"msgid": "CMD_EXPEL_CLAN_MEMBER", "rqid": 11, "target_pid": id_b})
        await r(a, 12, {"msgid": "CMD_GET_CLAN_MEMBER_MODE", "rqid": 12})
        self.assertEqual(ea[-1][1]["clan_member_num"], 1)
        await r(a, 13, {"msgid": "CMD_LEAVE_CLAN", "rqid": 13, "owner_pid": id_dueno})
        await r(b, 14, {"msgid": "CMD_GET_CLAN_LIST", "rqid": 14, "start_index": 0})
        self.assertEqual(eb[-1][1]["clan_list_num"], 0)
        for c in ("CMD_GET_CLAN_REQUEST_LIST", "CMD_GET_CLAN_PENDING_LIST"):
            await r(a, 15, {"msgid": c, "rqid": 15})
            self.assertEqual(ea[-1][1]["result"], "NOERR")
        self.assertFalse({k for k in self.srv.sin_manejador if "CLAN" in k})

    async def test_seguir_y_rechazar(self):
        """D.8: seguir, bloquear seguimiento, dejar de seguir y lista de rechazados del Cooperativo."""
        a, ea = await self.entrar("F1")
        b, eb = await self.entrar("F2")
        r = self.srv.recibir
        id_b = self.srv.numero_de_clave(b.steam_hex)
        await r(a, 1, {"msgid": "CMD_FOLLOW_USER", "rqid": 1, "follow_user_id_list": [id_b], "unfollow_user_id_list": []})
        await r(a, 2, {"msgid": "CMD_LOCK_FOLLOW_USER", "rqid": 2, "target_pid": id_b, "is_lock": "YES"})
        await r(a, 3, {"msgid": "CMD_GET_FOLLOW_LIST", "rqid": 3})
        lista = ea[-1][1]
        self.assertEqual(lista["follow_list_num"], 1)
        self.assertEqual((lista["follow_list"][0].valor["user_id"].valor["pid"], lista["follow_list"][0].valor["is_locked"]),
                         (id_b & 0xFFFFFFFF, "YES"))
        msgpack_min.packb(lista)
        await r(a, 4, {"msgid": "CMD_FOLLOW_USER", "rqid": 4, "follow_user_id_list": [], "unfollow_user_id_list": [id_b]})
        await r(a, 5, {"msgid": "CMD_GET_FOLLOW_LIST", "rqid": 5})
        self.assertEqual(ea[-1][1]["follow_list_num"], 0)
        await r(a, 6, {"msgid": "CMD_SET_COOP_REJECT", "rqid": 6, "set_reject_pid_list": [id_b], "unset_reject_pid_list": []})
        await r(a, 7, {"msgid": "CMD_GET_COOP_REJECT_LIST", "rqid": 7})
        self.assertEqual(ea[-1][1]["reject_list_num"], 1)
        await r(a, 8, {"msgid": "CMD_SET_COOP_REJECT", "rqid": 8, "set_reject_pid_list": [], "unset_reject_pid_list": [id_b]})
        await r(a, 9, {"msgid": "CMD_GET_COOP_REJECT_LIST", "rqid": 9})
        self.assertEqual(ea[-1][1]["reject_list_num"], 0)
        self.assertFalse({k for k in self.srv.sin_manejador if "FOLLOW" in k or "REJECT" in k})

    async def test_ranking_pesleague_y_clanes_extra(self):
        """R (AzrakHQ MIT): ranking de PES League con los inscritos y los avisos de clan sin error."""
        self.srv.cfg = {**self.srv.cfg, "myclub": {"pesleague": [{"compe_id": 4}],
                                                    "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("R1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_PESLEAGUE_COMPE_RANKING", "rqid": 1, "compe_id": 4, "start_index": 0, "view_num": 20})
        self.assertEqual((ea[-1][1]["result"], ea[-1][1]["list_num"]), ("NOERR", 0))
        await r(a, 2, {"msgid": "CMD_JOIN_PESLEAGUE_COMPE", "rqid": 2, "compe_id": 4, "team_id": 174})
        await r(a, 3, {"msgid": "CMD_GET_PESLEAGUE_COMPE_RANKING", "rqid": 3, "compe_id": 4, "start_index": 0, "view_num": 20})
        res = ea[-1][1]
        self.assertEqual((res["list_num"], res["list_max"]), (1, 1))
        fila = res["rank_list"][0].valor
        self.assertEqual((fila["rank"], fila["team_id"], fila["is_mydata"]), (1, 174, "YES"))
        msgpack_min.packb(res)
        await r(a, 4, {"msgid": "CMD_GET_PESLEAGUE_COMPE_RANKING_BYID", "rqid": 4, "compe_id": 4, "view_num": 5})
        self.assertEqual(ea[-1][1]["list_num"], 1)
        for i, c in enumerate(["CMD_ACCEPT_CLAN_ENTRY", "CMD_CANCEL_CLAN_ENTRY", "CMD_CONFIRM_CLAN_REQUEST",
                               "CMD_REJECT_CLAN_REQUEST", "CMD_ACCEPT_CLAN_INVITE", "CMD_CANCEL_CLAN_INVITE",
                               "CMD_GET_CLAN_RANKING", "CMD_GET_PESLEAGUE_COMPE_REWARD_LIST",
                               "CMD_GET_PESLEAGUE_COMPE_REWARD_INFO", "CMD_GET_ONLINE_AD_FILE"], 10):
            await r(a, i, {"msgid": c, "rqid": i, "target_pid": 1})
            self.assertEqual(ea[-1][1]["result"], "NOERR", c)
            msgpack_min.packb(ea[-1][1])
        self.assertFalse({k for k in self.srv.sin_manejador if "CLAN" in k or "PESLEAGUE_COMPE_R" in k})

    async def test_reenvio_de_avisos_y_sycom_coop(self):
        """R (coop2.md AzrakHQ): SEND_NOTICE llega como WATCH_NOTICE al otro; SYNC_PROGRESS también vuelve al que lo manda."""
        a, ea = await self.entrar("N1")
        b, eb = await self.entrar("N2")
        r = self.srv.recibir
        pb = self.srv._pid_visible(b)
        await r(a, 1, {"msgid": "CMD_SEND_NOTICE", "rqid": 1, "pid_list": [pb], "data": "hola"})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        self.assertEqual((eb[-1][1]["msgid"], eb[-1][1]["data"]), ("CMD_WATCH_NOTICE", "hola"))
        n = len(ea)
        pa = self.srv._pid_visible(a)
        await r(a, 2, {"msgid": "CMD_SEND_NOTICE", "rqid": 2, "pid_list": [pa, pb],
                       "data": '{"notice_key":"SYNC_PROGRESS"}'})
        self.assertIn("CMD_WATCH_NOTICE", [m["msgid"] for _, m in ea[n:]])
        self.assertNotIn("game_sycom", self.srv.respuesta_gamerelay(a))

    async def test_busqueda_cooperativa(self):
        """R (pescoop.py/coop.md AzrakHQ): 2 en cola COOP -> WATCH_COOP_MATCHING FINISH con 2 grupos a ambos;
        CANCEL solo si estaba en cola; los datos de COOP van y vuelven; gamerelay lleva game_sycom P2P."""
        a, ea = await self.entrar("0110000100000091")
        b, eb = await self.entrar("0110000100000092")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_COOP_QUICKMATCHOPTION", "rqid": 1})
        self.assertEqual(ea[-1][1]["force_alone"], "YES")
        await r(a, 2, {"msgid": "CMD_START_COOP_MATCHING", "rqid": 2, "use_team_id": 262143})
        await r(a, 3, {"msgid": "CMD_CANCEL_COOP_MATCHING", "rqid": 3})
        self.assertEqual(ea[-1][1]["matching_state"], "CANCEL")
        n = len(ea)
        await r(a, 4, {"msgid": "CMD_CANCEL_COOP_MATCHING", "rqid": 4})
        self.assertEqual(len(ea), n + 1)           # sin cola: solo el NOERR, sin empuje
        await r(a, 5, {"msgid": "CMD_START_COOP_MATCHING", "rqid": 5, "use_team_id": 262143})
        await r(b, 6, {"msgid": "CMD_START_COOP_MATCHING", "rqid": 6, "use_team_id": 105})
        for e in (ea, eb):
            w = [m for _, m in e if m.get("msgid") == "CMD_WATCH_COOP_MATCHING"][-1]
            self.assertEqual((w["matching_state"], w["group_list_num"], w["rqid"]), ("FINISH", 2, 0))
            g0, g1 = (g.valor for g in w["group_list"])
            self.assertEqual((g0["use_team_id"], g1["use_team_id"]), (101, 105))
            m0 = g0["member_list"][0].valor["member"].valor
            m1 = g1["member_list"][0].valor["member"].valor
            self.assertEqual((m0["is_owner"], m0["side"], m1["is_owner"], m1["side"]), ("YES", "HOME", "NO", "AWAY"))
            msgpack_min.packb(w)
        self.assertEqual(self.srv.respuesta_gamerelay(a).get("game_sycom"), "P2P")
        await r(a, 7, {"msgid": "CMD_SET_COOP_MATCH_DATA", "rqid": 7, "match_data": "QUJD", "match_data_size": 3})
        await r(a, 8, {"msgid": "CMD_GET_COOP_MATCH_DATA", "rqid": 8})
        self.assertEqual((ea[-1][1]["match_data"], ea[-1][1]["match_data_size"]), ("QUJD", 3))
        await r(a, 9, {"msgid": "CMD_GET_COOP_LEVEL_RESULT", "rqid": 9})
        self.assertEqual(ea[-1][1]["coop_level_user_list"], [])
        # R (pescoop.award AzrakHQ): gol del local y pitazo final -> 30 y 10 puntos, pantalla de nivel
        await r(a, 10, {"msgid": "CMD_ADD_SCORE", "rqid": 10, "goal_side": "HOME", "gtimer": 100})
        for x, n in ((a, 11), (b, 12)):
            await r(x, n, {"msgid": "CMD_CHANGE_GAMEPHASE", "rqid": n, "phase": "END"})
        await r(a, 13, {"msgid": "CMD_GET_COOP_LEVEL_RESULT", "rqid": 13})
        filas = [f.valor for f in ea[-1][1]["coop_level_user_list"]]
        self.assertEqual(sorted(f["point"] for f in filas), [10, 30])
        self.assertTrue(all(f["before_level"] == 1 and f["status"] == "NONE" for f in filas))
        msgpack_min.packb(ea[-1][1])
        # seguir al compañero desde esa pantalla: llega user_id bin{pid, pname}
        uid = next(f["user_id"] for f in filas if f["point"] == 10)
        await r(a, 14, {"msgid": "CMD_FOLLOW_USER", "rqid": 14,
                        "follow_user_id_list": [msgpack_min.packb(uid.valor)]})
        await r(a, 15, {"msgid": "CMD_GET_COOP_PROFILE", "rqid": 15, "pid": uid.valor["pid"]})
        perfil = ea[-1][1]["coop_profile_data"].valor
        self.assertEqual((perfil["is_follow"], perfil["coop_level_point"], perfil["online_match"]), ("YES", 10, 1))
        await r(a, 16, {"msgid": "CMD_GET_COOP_MATCH_DATA", "rqid": 16})
        self.assertEqual(ea[-1][1]["match_data"], "QUJD")

    async def test_lote_azrak_salas_chat_bloqueo_matchday(self):
        """R (AzrakHQ, MIT): JOIN_ROOM_NTL con clave y room_owner; SET_ROOM_OPTION; chat; bloqueo de
        partidos; IPANDPORT; puntos y ranking de Matchday; aviso urgente una vez; tiempos del PES."""
        a, ea = await self.entrar("0110000100000081")
        b, eb = await self.entrar("0110000100000082")
        r = self.srv.recibir
        env = await self.srv.c_cmd_get_server_env(a, {})
        self.assertEqual((env["match_client_timeout_load_timeout"], env["matchrejectpid_max"],
                          env["is_revision_check"]), (180, 400, "NO"))
        await r(a, 1, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 1, "name": "X", "password": "123"})
        sala = [m for _, m in ea if m.get("msgid") == "CMD_CREATEJOIN_ROOM"][-1]["room_id"]
        await r(b, 2, {"msgid": "CMD_JOIN_ROOM_NTL", "rqid": 2, "room_id": sala, "password": "999"})
        self.assertEqual(eb[-1][1]["result"], "ERR_PASSWDINCORRECT")
        await r(a, 3, {"msgid": "CMD_SET_ROOM_OPTION", "rqid": 3, "name": "Nueva", "password": ""})
        self.assertEqual(self.srv.salas[sala].nombre, "Nueva")
        await r(b, 4, {"msgid": "CMD_JOIN_ROOM_NTL", "rqid": 4, "room_id": sala, "password": ""})
        resp = [m for _, m in eb if m.get("msgid") == "CMD_JOIN_ROOM_NTL"][-1]
        self.assertEqual((resp["result"], resp["room_stat"]), ("NOERR", "WAITING"))
        self.assertEqual(resp["room_owner"].valor["pid"], self.srv._pid_visible(a))
        msgpack_min.packb(resp)
        await r(a, 5, {"msgid": "CMD_SEND_TEXTCHAT", "rqid": 5, "channel": "ROOM", "statement": "hola"})
        chat = [m for _, m in eb if m.get("msgid") == "CMD_WATCH_TEXTCHAT"][-1]
        self.assertEqual((chat["statement"], chat["from_pid"]), ("hola", self.srv._pid_visible(a)))
        a.ntl["reflexive_addr"] = {"ip": "1.2.3.4", "port": 5}
        ip = await self.srv.c_cmd_get_ipandport_ntl(b, {"pid": self.srv._pid_visible(a)})
        self.assertEqual(ip["reflexive_addr"].valor["port"], 5)
        # bloqueo: a bloquea a b y no se emparejan
        await r(a, 6, {"msgid": "CMD_SET_MATCHREJECTPID", "rqid": 6, "reject_pid": self.srv._pid_visible(b),
                       "is_reject": "YES"})
        self.assertTrue(self.srv._no_emparejar(a, b))
        await r(a, 7, {"msgid": "CMD_SET_MATCHREJECTPID", "rqid": 7, "reject_pid": self.srv._pid_visible(b),
                       "is_reject": "NO"})
        self.assertFalse(self.srv._no_emparejar(a, b))
        # Matchday: inscritos en el mismo bando, gana a 2-0 y suma 3
        for x, eq in ((a, 101), (b, 108)):
            club = self.srv._club_propio(x) or {}
            club.setdefault("matchday_equipo", {})["3"] = {"grupo": "MATCHDAY_GROUP_A", "team_id": eq}
            club.setdefault("torneos_inscripcion", {})["matchday"] = [3]
            self.srv.datos.guardar_club(x.steam_hex, club)
        pareja = {"miembros": (a, b), "check": set(), "tipo": "MATCHDAY", "room_id": 990001}
        self.srv.parejas[a] = self.srv.parejas[b] = pareja
        self.srv._resultado_matchday(a, pareja, {"home_score": 2, "away_score": 0})
        self.srv._resultado_matchday(a, pareja, {"home_score": 2, "away_score": 0})  # una sola vez
        self.srv._resultado_matchday(b, pareja, {"home_score": 2, "away_score": 0})
        pts = await self.srv.c_cmd_get_matchday_achievement_reward(a, {})
        self.assertEqual((pts["add_matchday_point"], pts["gameplay_quality"]), (3, 30))
        self.assertEqual((await self.srv.c_cmd_get_matchday_achievement_reward(a, {}))["add_matchday_point"], 0)
        rk = await self.srv.c_cmd_get_matchday_ranking(b, {"compe_id": 3})
        self.assertEqual((rk["list_max"], rk["rank_list"][0].valor["matchday_point"],
                          rk["rank_list"][0].valor["team_id"]), (2, 3, 101))
        # aviso urgente: una vez por persona
        self.srv.cfg["aviso_urgente"] = {"id": 7, "titulo": "Hola", "texto": "Mantenimiento 22:00"}
        u = await self.srv.c_cmd_get_urgent_announcement(a, {})
        self.assertEqual((u["is_announcement"], u["announcement_data"].valor["body"]), ("YES", "Mantenimiento 22:00"))
        self.assertEqual(await self.srv.c_cmd_get_urgent_announcement(a, {}), {})
        del self.srv.cfg["aviso_urgente"]

    async def test_veredicto_por_req_gameaddpoint(self):
        """R (results.md AzrakHQ): goles contados una vez, END de los dos y REQ_GAMEADDPOINT -> empuje."""
        a, ea = await self.entrar("0110000100000071")
        b, eb = await self.entrar("0110000100000072")
        r = self.srv.recibir
        pareja = {"miembros": (a, b), "check": set(), "tipo": "OC_QUICK", "room_id": 990002}
        self.srv.parejas[a] = self.srv.parejas[b] = pareja
        await r(a, 1, {"msgid": "CMD_SET_GAMETEAM", "rqid": 1, "home_team_id": 101, "away_team_id": 108})
        for x in (a, b):  # los dos PES informan el mismo gol
            await r(x, 2, {"msgid": "CMD_ADD_SCORE", "rqid": 2, "goal_side": "AWAY", "gtimer": 1234})
        await r(b, 3, {"msgid": "CMD_REQ_GAMEADDPOINT", "rqid": 3})
        self.assertNotIn("CMD_WATCH_GAMEADDPOINT", [m["msgid"] for _, m in eb])  # sin END no hay veredicto
        for x in (a, b):
            await r(x, 4, {"msgid": "CMD_CHANGE_GAMEPHASE", "rqid": 4, "phase": "END"})
        await r(b, 5, {"msgid": "CMD_REQ_GAMEADDPOINT", "rqid": 5})
        v = [m for _, m in eb if m["msgid"] == "CMD_WATCH_GAMEADDPOINT"][-1]
        self.assertEqual((v["winner_side"], v["gameend_reason"], v["player_count"], v["rqid"]),
                         ("WINNER_AWAY", "NORMAL_END", 2, 0))
        self.assertEqual(v["player_list"][1].valor["team_id"], 108)
        msgpack_min.packb(v)

    async def test_torneo_en_linea_suma_puntos(self):
        """R (pestourney.record AzrakHQ): 3/1/0 por partido, una vez, y el menú 1vs1 muestra G/E/P."""
        self.srv.cfg.setdefault("myclub", {})["pesleague"] = [{"compe_id": 4}]
        a, ea = await self.entrar("0110000100000061")
        b, eb = await self.entrar("0110000100000062")
        for x in (a, b):
            club = self.srv._club_propio(x) or {}
            club.setdefault("torneos_inscripcion", {})["pesleague"] = [4]
            self.srv.datos.guardar_club(x.steam_hex, club)
        pareja = {"miembros": (a, b), "check": set(), "tipo": "PES_LEAGUE", "room_id": 990003}
        for x in (a, b):
            self.srv._resultado_pesleague(x, pareja, {"home_score": 1, "away_score": 1})
        self.srv._resultado_pesleague(a, pareja, {"home_score": 1, "away_score": 1})  # repetido
        menu = await self.srv.c_cmd_get_pesleague_1vs1_mainmenu_info(a, {"compe_id": 4})
        self.assertEqual((menu["point"], menu["draw_count"], menu["match_num"], menu["regulation_match_num"]),
                         (1, 1, 1, 1))
        rk = await self.srv.c_cmd_get_pesleague_compe_ranking(a, {"compe_id": 4})
        self.assertEqual(rk["rank_list"][0].valor["point"], 1)

    async def test_cortesia_y_derrota_por_abandono(self):
        """R (pesmanner.py AzrakHQ): abandono con el partido en juego = aviso de cortesía y, si fue uno
        solo, pierde 3-0 en Matchday. Partido completo = nada."""
        a, ea = await self.entrar("0110000100000051")
        b, eb = await self.entrar("0110000100000052")
        r = self.srv.recibir
        for x in (a, b):
            club = self.srv._club_propio(x) or {}
            club.setdefault("matchday_equipo", {})["3"] = {"grupo": "MATCHDAY_GROUP_A", "team_id": 101}
            club.setdefault("torneos_inscripcion", {})["matchday"] = [3]
            self.srv.datos.guardar_club(x.steam_hex, club)
        pareja = {"miembros": (a, b), "check": set(), "tipo": "MATCHDAY", "room_id": 990004}
        self.srv.parejas[a] = self.srv.parejas[b] = pareja
        await r(a, 1, {"msgid": "CMD_GET_MANNER_WARN", "rqid": 1})
        self.assertNotIn("warn_kind", ea[-1][1])
        for x in (a, b):
            await r(x, 2, {"msgid": "CMD_CHANGE_GAMEPHASE", "rqid": 2, "phase": "1ST"})
        await r(a, 3, {"msgid": "CMD_SEND_ABNORMALEND", "rqid": 3, "reason": "GIVEUP"})
        pareja["decidir_derrota"]()
        club_b = self.srv.datos.club(b.steam_hex)
        self.assertEqual(club_b["matchday_puntos"]["3"], 3)
        self.assertEqual(int((self.srv.datos.club(a.steam_hex).get("matchday_puntos") or {}).get("3") or 0), 0)
        await r(a, 4, {"msgid": "CMD_GET_MANNER_WARN", "rqid": 4})
        self.assertEqual(ea[-1][1]["warn_kind"], "DOWN_B")
        await r(a, 5, {"msgid": "CMD_GET_MANNER_WARN", "rqid": 5})
        self.assertNotIn("warn_kind", ea[-1][1])  # una sola vez
        self.assertEqual(self.srv.grado_modales(a.steam_hex.lower()), "A")  # 1 abandono sigue en A

    async def test_turn_propio_lista_y_direcciones(self):
        """R (turn.py AzrakHQ): lista TURN con port y tls_port no nulos + certificado; la dirección de
        relé va al rival (rqid 0) y el que manda recibe la que el rival mandó antes."""
        a, ea = await self.entrar("0110000100000041")
        b, eb = await self.entrar("0110000100000042")

        class TurnFalso:
            def activo(self):
                return True

            def lista(self, pid, direccion):
                return {"turn_server_list": [{"zone": 1, "address": direccion, "port": 3479, "tls_port": 5349}],
                        "id": "u", "pw": "p", "server_certificate": "QUJD", "server_certificate_data_size": 3}
        self.srv.turn_propio = TurnFalso()
        r = self.srv.respuesta_http({"msgid": "CMD_GET_TURN_SERVER_LIST", "rqid": "4", "pid": "9"},
                                    ip_local="100.64.0.9")
        self.assertEqual((r["result"], r["turn_server_list"][0]["address"], r["turn_server_list"][0]["tls_port"]),
                         ("NOERR", self.srv.ip_anuncio(ip_local="100.64.0.9"), 5349))
        pareja = {"miembros": (a, b), "check": set(), "tipo": "OC_QUICK", "room_id": 990005}
        self.srv.parejas[a] = self.srv.parejas[b] = pareja
        await self.srv.recibir(b, 1, {"msgid": "CMD_SEND_TURN_ADDRESS_DATA", "rqid": 1, "address": b"BB",
                                      "address_data_size": 2})
        w = [m for _, m in ea if m["msgid"] == "CMD_WATCH_TURN_ADDRESS_DATA"][-1]
        self.assertEqual((w["address"], w["rqid"]), (b"BB", 0))
        await self.srv.recibir(a, 2, {"msgid": "CMD_SEND_TURN_ADDRESS_DATA", "rqid": 2, "address": b"AA",
                                      "address_data_size": 2})
        self.assertEqual([m for _, m in ea if m["msgid"] == "CMD_WATCH_TURN_ADDRESS_DATA"][-1]["address"], b"BB")
        self.assertEqual([m for _, m in eb if m["msgid"] == "CMD_WATCH_TURN_ADDRESS_DATA"][-1]["address"], b"AA")
        self.srv.turn_propio = None

    async def test_recarga_config_en_caliente(self):
        """Cambiar myclub en config.json se aplica sin reiniciar (solo la clave myclub)."""
        import json as _j, tempfile, os as _o
        with tempfile.TemporaryDirectory() as d:
            ruta = _o.path.join(d, "config.json")
            with open(ruta, "w", encoding="utf-8") as f:
                _j.dump({"myclub": {"x": 1}, "turn": {"secreto": "no"}}, f)
            self.srv.ruta_config = ruta
            self.srv._mtime_config = None
            self.srv._recargar_config()          # fija la hora base
            turn_antes = self.srv.cfg.get("turn")
            with open(ruta, "w", encoding="utf-8") as f:
                _j.dump({"myclub": {"x": 2}, "turn": {"secreto": "otro"}}, f)
            _o.utime(ruta, (_o.path.getmtime(ruta) + 5, _o.path.getmtime(ruta) + 5))
            self.srv._recargar_config()
            self.assertEqual(self.srv.cfg["myclub"], {"x": 2})
            self.assertEqual(self.srv.cfg.get("turn"), turn_antes)
            with open(ruta, "w", encoding="utf-8") as f:
                f.write("{ roto")
            _o.utime(ruta, (_o.path.getmtime(ruta) + 10, _o.path.getmtime(ruta) + 10))
            self.srv._recargar_config()          # JSON roto: conserva la anterior
            self.assertEqual(self.srv.cfg["myclub"], {"x": 2})

    async def test_salas_coop_crear_entrar_salir(self):
        """D.8: crear sala cooperativa, verla en la lista, entrar, listo, opciones y salir."""
        a, ea = await self.entrar("S1")
        b, eb = await self.entrar("S2")
        r = self.srv.recibir
        basic = {"room_name": "Phoenix", "is_com_match": "YES", "team_level": 0, "password": ""}
        await r(a, 1, {"msgid": "CMD_CREATEJOIN_COOP_ROOM", "rqid": 1, "basic_settings": msgpack_min.packb(basic),
                       "match_settings": msgpack_min.packb({"user_team_id": 101, "enemy_team_id": 0, "cpu_level": 3})})
        sala = ea[-1][1]["room_id"]
        self.assertTrue(sala)
        await r(b, 2, {"msgid": "CMD_GET_COOP_ROOMLIST", "rqid": 2, "start_index": 0,
                       "search_options": msgpack_min.packb({"room_id": 0})})
        lista = eb[-1][1]
        self.assertEqual(lista["room_list_num"], 1)
        fila = lista["room_list"][0].valor
        self.assertEqual((fila["basic_settings"].valor["room_name"], fila["basic_settings"].valor["room_id"],
                          fila["match_settings"].valor["user_team_id"], fila["user_num"]), ("Phoenix", sala, 101, 1))
        msgpack_min.packb(lista)
        # nombre por defecto del PES («… de») se completa con el dueño
        sala_obj = self.srv.salas_coop[sala]
        sala_obj["basic"]["room_name"] = "Salón de juego de"
        completo = self.srv._ajustes_sala_coop(sala_obj).valor["room_name"]
        self.assertTrue(completo.startswith("Salón de juego de ") and len(completo) > len("Salón de juego de "), completo)
        sala_obj["basic"]["room_name"] = "Phoenix"
        await r(a, 30, {"msgid": "CMD_REQWATCHL_COOP_ROOM_MEMBER", "rqid": 50, "room_id": sala})
        self.assertEqual((ea[-1][1]["list_type"], ea[-1][1]["list_num"]), ("RELOAD", 1))
        await r(b, 3, {"msgid": "CMD_JOIN_COOP_ROOM", "rqid": 3, "room_id": sala, "password": "", "is_invite": "NO"})
        aviso = ea[-1][1]  # suscripción: al entrar B, A recibe la lista con SU rqid
        self.assertEqual((aviso["msgid"], aviso["rqid"], aviso["list_num"]), ("CMD_REQWATCHL_COOP_ROOM_MEMBER", 50, 2))
        self.assertIn("guest_info", aviso["list_data"][1].valor)
        await r(b, 4, {"msgid": "CMD_SET_COOP_MATCH_READY", "rqid": 4, "room_id": sala, "is_ready": "YES"})
        await r(b, 5, {"msgid": "CMD_GET_IS_JOIN_COOP_ROOM", "rqid": 5})
        self.assertEqual(eb[-1][1]["is_join_coop_room"], "YES")
        await r(a, 6, {"msgid": "CMD_GET_COOP_ROOM_MEMBER", "rqid": 6, "room_id": sala})
        miembros = [x.valor for x in ea[-1][1]["list_data"]]
        self.assertEqual([(x["is_owner"], x["is_ready"]) for x in miembros], [("YES", "NO"), ("NO", "YES")])
        await r(a, 7, {"msgid": "CMD_SET_COOP_ROOM_OPTION", "rqid": 7, "room_id": sala,
                       "match_settings": msgpack_min.packb({"user_team_id": 5, "enemy_team_id": 0, "cpu_level": 2})})
        await r(b, 8, {"msgid": "CMD_GET_COOP_ROOM_OPTION", "rqid": 8, "room_id": sala})
        self.assertEqual(eb[-1][1]["match_settings"].valor["user_team_id"], 5)
        await r(a, 9, {"msgid": "CMD_LEAVE_COOP_ROOM", "rqid": 9, "room_id": sala})
        await self.srv.cerrar(b)
        await r(a, 10, {"msgid": "CMD_GET_COOP_ROOMLIST", "rqid": 10, "start_index": 0})
        self.assertEqual(ea[-1][1]["room_list_num"], 0)
        self.assertFalse({k for k in self.srv.sin_manejador if "COOP" in k})

    async def test_coop_matchday_pesleague_pantallas_vacias(self):
        """Parte D: los 13 comandos responden NOERR; Matchday/PES League leen torneos de config."""
        self.srv.cfg = {**self.srv.cfg, "myclub": {"matchday": [{"compe_id": 3, "compe_name": "Jornada"}],
                                                    "pesleague": [{"compe_id": 4}],
                                                    "jugadores": [{"gameplayer_id": 100 + i} for i in range(18)],
                                                    "entrenadores": [{"coach_id": 8}],
                                                    "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        res = lambda: ea[-1][1]
        for i, c in enumerate(["CMD_GET_COOP_MAINMENU_INFO", "CMD_GET_IS_JOIN_COOP_ROOM", "CMD_GET_COOP_PROFILE",
                               "CMD_GET_COOP_REJECT_LIST", "CMD_GET_COOP_ROOMLIST", "CMD_GET_CLAN_LIST",
                               "CMD_JOIN_MATCHDAY_MODE", "CMD_GET_MATCHDAY_LIST", "CMD_LEAVE_MATCHDAY_MODE",
                               "CMD_JOIN_PESLEAGUE_COMPE_MODE", "CMD_GET_PESLEAGUE_MAINMENU_INFO",
                               "CMD_GET_PESLEAGUE_COMPE_LIST", "CMD_LEAVE_PESLEAGUE_COMPE_MODE"], 1):
            await r(a, i, {"msgid": c, "rqid": i, "start_index": 0})
            self.assertEqual(res()["result"], "NOERR", c)
            msgpack_min.packb(res())
            if c in ("CMD_GET_MATCHDAY_LIST", "CMD_GET_PESLEAGUE_COMPE_LIST"):
                self.assertEqual(res()["list_num"], 1)
        self.assertNotIn("CMD_GET_CLAN_LIST", getattr(self.srv, "sin_manejador", {}) or {})
        # inscripción propia: JOIN marca is_entry, LEAVE lo quita, torneo inexistente da error
        for i, (c, cid, lista, campo) in enumerate([
                ("CMD_JOIN_MATCHDAY", 3, "CMD_GET_MATCHDAY_LIST", "matchday"),
                ("CMD_JOIN_PESLEAGUE_COMPE", 4, "CMD_GET_PESLEAGUE_COMPE_LIST", "pesleague")], 20):
            await r(a, i, {"msgid": c, "rqid": i, "compe_id": cid})
            self.assertEqual(res()["result"], "NOERR", c)
            await r(a, i, {"msgid": lista, "rqid": i, "start_index": 0})
            self.assertEqual(res()["compe_list"][0].valor["is_entry"], "YES", campo)
            await r(a, i, {"msgid": c.replace("JOIN", "LEAVE"), "rqid": i, "compe_id": cid})
            await r(a, i, {"msgid": lista, "rqid": i, "start_index": 0})
            self.assertEqual(res()["compe_list"][0].valor["is_entry"], "NO", campo)
        for i, c in enumerate(["CMD_GET_MATCHDAY_INFO", "CMD_GET_MATCHDAY_MAINMENU_INFO"], 40):
            await r(a, i, {"msgid": c, "rqid": i, "compe_id": 3})
            self.assertEqual(res()["result"], "NOERR", c)
            msgpack_min.packb(res())
        self.assertEqual(res()["mainmenu_info"].valor["is_watch_match"], "NO")
        await r(a, 42, {"msgid": "CMD_GET_MATCHDAY_INFO", "rqid": 42, "compe_id": 3})
        sobre = res()["compe_info"].valor["info_overview"].valor  # Q: el evento va DENTRO (PESBUL)
        self.assertEqual(sobre["compe_id"], 3)
        self.assertGreater(sobre["end_group_match_date"], 0)
        self.assertEqual(res()["compe_info"].valor["is_check_eula"], "NO")
        self.assertEqual(res()["compe_info"].valor["win_reward_list"], [])  # premios = lista
        await r(a, 43, {"msgid": "CMD_JOIN_MATCHDAY", "rqid": 43, "compe_id": 0, "team_id": 174,
                        "entry_matchday_group": "MATCHDAY_GROUP_A"})
        self.assertIn("squad_rating_info", res())
        self.assertEqual(res()["compe_id"], 3)
        await r(a, 45, {"msgid": "CMD_GET_MATCHDAY_INFO", "rqid": 45, "compe_id": 0})  # el PES manda 0
        self.assertEqual(res()["compe_info"].valor["info_overview"].valor["compe_id"], 3)
        self.srv.cfg["myclub"]["matchday_info_extra"] = ["joined_matchday_group", "team_id"]
        await r(a, 44, {"msgid": "CMD_GET_MATCHDAY_INFO", "rqid": 44, "compe_id": 3})
        self.assertNotIn("all_reward_info", res()["compe_info"].valor)
        self.assertEqual((res()["compe_info"].valor["joined_matchday_group"], res()["compe_info"].valor["team_id"]),
                         ("MATCHDAY_GROUP_A", 174))
        for i, c in enumerate(["CMD_GET_PESLEAGUE_COMPE_INFO", "CMD_GET_PESLEAGUE_COMPE_PARTICIPATION_RIGHT"], 50):
            await r(a, i, {"msgid": c, "rqid": i, "compe_id": 4})
            self.assertEqual(res()["result"], "NOERR", c)
            msgpack_min.packb(res())
        await r(a, 30, {"msgid": "CMD_JOIN_MATCHDAY", "rqid": 30, "compe_id": 99})
        self.assertEqual(res()["result"], "ERR_COMPE_NOTFOUND")

    async def test_myclub_catalogo_club_y_rival_rotando(self):
        import os, struct, tempfile, zlib, json as _j, sys as _s
        _s.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))
        import catalogo_pesdb as cp
        filas = b"".join(struct.pack("<4I", i, 1000 + i, 108 if i < 20 else 109, (3 << 10) | (i % 30 + 1))
                         for i in range(40))
        crudo = b"\xff\x10\x81WESYS" + zlib.compress(filas)
        reg = struct.pack("<I", 1001) + bytes(239) + b"L. MESSI".ljust(61, b"\0") + bytes(8)
        nombres = cp.leer_nombres(bytes(8) + reg)
        self.assertEqual(nombres["1001"], "L. MESSI")
        bits = 1001 | (10 << 370) | (16 << 169)
        reg2 = bits.to_bytes(64, "little") + reg[64:]
        ficha = cp.leer_fichas(bytes(8) + reg2)["1001"]
        self.assertEqual((ficha["posicion_txt"], ficha["nacion"]), ("ED", 16))
        # habilidades: 6 bits = valor - 40 (control de balón en el bit 217)
        bits3 = 1001 | (12 << 370) | ((96 - 40) << 217) | ((95 - 40) << 332)
        ficha3 = cp.leer_fichas(bytes(8) + bits3.to_bytes(64, "little") + reg[64:])["1001"]
        self.assertEqual((ficha3["habilidades"]["control"], ficha3["habilidades"]["finalizacion"],
                          ficha3["habilidades"]["drible"]), (96, 95, 40))
        self.assertTrue(ficha3["rating_estimado"])
        cat = cp.armar(cp.leer_asignaciones(cp.descomprimir_wesys(crudo)), {"1001": {"rating": 90}}, nombres)
        self.assertEqual(cat["jugadores"]["1001"]["nombre"], "L. MESSI")
        self.assertEqual((len(cat["equipos"]["108"]), len(cat["equipos"]["109"])), (20, 20))
        ruta = os.path.join(tempfile.mkdtemp(), "catalogo.json")
        with open(ruta, "w") as f:
            _j.dump(cat, f)
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "catalogo": ruta, "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "base_team_id": 108},
            "rival": {"coach_id": 24, "equipos": [
                {"base_team_id": 109, "team_name": "B"}, {"base_team_id": 108, "team_name": "A"}]}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYERLIST", "rqid": 1})
        self.assertEqual(ea[-1][1]["gameplayer_list_num"], 20)
        f1 = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["gameplayer_list"][1].valor))
        self.assertEqual((f1["gameplayer_id"], f1["default_rarity"]), (1001, "PLAYER_RARITY_BLACK"))
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 2})
        self.assertEqual(ea[-1][1]["team_name"], "B")
        equipo = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["team_data"].valor))
        self.assertEqual(equipo["member_num"], 18)
        await r(a, 3, {"msgid": "CMD_SET_MYCLUB_VSCOM_GAMERESULT", "rqid": 3, "winner_side": "WINNER_NONE",
                       "past_game_played_pid": 7, "pes_srv_rtt_nums": 3})
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_VSCOM_OPPONENT", "rqid": 4})
        self.assertEqual(ea[-1][1]["team_name"], "A")

    async def test_myclub_fichar_agente_cazatalentos_y_renovar(self):
        import os, tempfile, json as _j
        cat = {"version": 1, "equipos": {"103": [{"gameplayer_id": 2000 + i} for i in range(5)]},
               "jugadores": {str(2000 + i): {"team_id": 103} for i in range(5)}}
        ruta = os.path.join(tempfile.mkdtemp(), "catalogo.json")
        with open(ruta, "w") as f:
            _j.dump(cat, f)
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "catalogo": ruta, "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 12000},
            "agentes": [{"agent_id": 1, "name": "Estrellas", "gp": 10000, "get_player_num": 1,
                         "equipos": [103]}],
            "cazatalentos": [{"agent_id": 50, "count": 2, "sell_gp": 300, "jugadores": [2001, 2002]}],
            "renovacion_gp": 100, "renovacion_partidos": 10}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_MARKET_AGENTLIST", "rqid": 1})
        self.assertEqual(ea[-1][1]["market_agent_list_num"], 1)
        el = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["market_agent_list"][0].valor))
        self.assertEqual(set(el), {"agent", "banner_file_info", "skill"})
        self.assertEqual(msgpack_min.unpackb(el["agent"])["agent_id"], 1)
        await r(a, 11, {"msgid": "CMD_GET_MYCLUB_AGENT_BOX_INFO", "rqid": 11, "agent_id": 1})
        caja = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["agent_info"].valor))
        self.assertEqual(caja["box_size"], 5)
        self.assertEqual(len(caja["main_player_rarity_info"]), 1)  # vacía = «caducó»
        self.assertIsInstance(caja["sub1_player_rarity_info"], bytes)  # un bin, no lista
        self.assertIn("agent", msgpack_min.unpackb(caja["agent_info"]))
        await r(a, 2, {"msgid": "CMD_USE_MYCLUB_AGENT", "rqid": 2, "agent_id": 1, "use_pes_point": "NO"})
        res = ea[-1][1]
        self.assertEqual((res["gp"], res["gameplayer_list_num"]), (2000, 1))
        nuevo = msgpack_min.unpackb(msgpack_min.packb(res["gameplayer_list"][0].valor))
        self.assertIn(nuevo["gameplayer_id"], range(2000, 2005))
        await r(a, 3, {"msgid": "CMD_USE_MYCLUB_AGENT", "rqid": 3, "agent_id": 1, "use_pes_point": "NO"})
        self.assertEqual(ea[-1][1]["result"], "ERR_INSUFFICIENT_GP")
        await r(a, 4, {"msgid": "CMD_GET_MYCLUB_SCOUTLIST", "rqid": 4})
        self.assertEqual(ea[-1][1]["scout_list_num"], 1)
        pedido = [msgpack_min.packb({"agent_id": 50})]
        await r(a, 5, {"msgid": "CMD_USE_MYCLUB_SCOUT", "rqid": 5, "scout_list": pedido})
        g = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["gameplayer"].valor))
        self.assertIn(g["gameplayer_id"], (2001, 2002))
        await r(a, 6, {"msgid": "CMD_SELL_MYCLUB_SCOUT", "rqid": 6, "scout_list": pedido})
        self.assertEqual((ea[-1][1]["gain_gp"], ea[-1][1]["gp"]), (300, 2300))
        await r(a, 7, {"msgid": "CMD_GET_MYCLUB_SCOUTLIST", "rqid": 7})
        self.assertEqual(ea[-1][1]["scout_list_num"], 0)
        await r(a, 8, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYERLIST", "rqid": 8})
        self.assertEqual(ea[-1][1]["gameplayer_list_num"], 3)
        ficha = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["gameplayer_list"][0].valor))
        self.assertEqual(ficha["contract_gp"], 100)  # precio de renovar sale de la ficha
        club = self.srv.datos.club(a.steam_hex)
        uid = club["jugadores"][0]["gameplayer_unique_id"]
        antes = self.srv._contrato(club["jugadores"][0])  # unidad del juego: partidos x 90
        await r(a, 9, {"msgid": "CMD_EXTEND_MYCLUB_GAMEPLAYER", "rqid": 9,
                       "gameplayer_unique_id_list": [str(uid)], "use_pes_point": "NO"})
        self.assertEqual(ea[-1][1]["gp"], 2200)
        fila = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["contract_term_list"][0].valor))
        self.assertEqual(fila["contract_term"], antes + 10 * 90)  # +10 partidos
        self.assertIsInstance(fila["gameplayer_unique_id"], int)  # texto = PES se congelaba

    async def test_myclub_logros_gp(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 1000}, "gp_logros": {"33": 1200}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_SET_GP_ACHIEVEMENT", "rqid": 1,
                       "new_achievement_bit_array": [0b101, 0b10]})
        self.assertEqual(ea[-1][1]["all_gp"], 1000 + 300 + 300 + 1200)
        await r(a, 2, {"msgid": "CMD_SET_GP_ACHIEVEMENT", "rqid": 2,
                       "new_achievement_bit_array": [0b101, 0b10]})
        self.assertEqual(ea[-1][1]["all_gp"], 2800)  # ya cobrados: no paga dos veces
        await r(a, 3, {"msgid": "CMD_GET_GP_ACHIEVEMENT", "rqid": 3})
        self.assertEqual(ea[-1][1]["all_achievement_bit_array"], [5, 2])

    async def test_myclub_bloquear_liberar_y_entrenador(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(20)],
            "entrenadores": [{"coach_id": 8}], "venta_gp": 100,
            "club_de_prueba": {"team_name": "P", "gp": 1000}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        await r(a, 2, {"msgid": "CMD_SET_MYCLUB_PROTECTED_ENABLE", "rqid": 2,
                       "gameplayer_unique_id_list": [19, 20]})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        await r(a, 3, {"msgid": "CMD_SELL_MYCLUB_GAMEPLAYER", "rqid": 3,
                       "gameplayer_unique_id_list": [19], "is_use_cloud_match": "NO"})
        self.assertEqual(ea[-1][1]["result"], "ERR_FORBIDDEN")  # bloqueado
        await r(a, 4, {"msgid": "CMD_SET_MYCLUB_PROTECTED_DISABLE", "rqid": 4,
                       "gameplayer_unique_id_list": [19]})
        await r(a, 5, {"msgid": "CMD_SELL_MYCLUB_GAMEPLAYER", "rqid": 5,
                       "gameplayer_unique_id_list": [19], "is_use_cloud_match": "NO"})
        self.assertEqual((ea[-1][1]["gain_gp"], ea[-1][1]["gp"]), (100, 1100))
        await r(a, 6, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYERLIST", "rqid": 6})
        self.assertEqual(ea[-1][1]["gameplayer_list_num"], 19)
        el = msgpack_min.packb({"gameplayer_unique_id": 18, "gameplayer_id": 117, "overall": 80})
        await r(a, 7, {"msgid": "CMD_CREATE_MYCLUB_STAFF", "rqid": 7, "player_info_list": [el]})
        await r(a, 8, {"msgid": "CMD_GET_MYCLUB_STAFF_LIST", "rqid": 8})
        self.assertEqual(ea[-1][1]["staff_list_num"], 1)
        await r(a, 9, {"msgid": "CMD_GET_MYCLUB_GAMEPLAYERLIST", "rqid": 9})
        self.assertEqual(ea[-1][1]["gameplayer_list_num"], 18)

    async def test_myclub_entrenador_titular_no_deja_hueco(self):
        # C (05-nov 09:20): convertir a un inscrito pone al primer libre en su mismo puesto
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(20)],
            "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 1000}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        miembros = [msgpack_min.packb({"gameplayer_unique_id": u}) for u in range(1, 19)]
        await r(a, 2, {"msgid": "CMD_SET_MYCLUB_SQUAD_GAMEPLAYER", "rqid": 2, "squad_id": 1,
                       "member_list": miembros})
        el = msgpack_min.packb({"gameplayer_unique_id": 5, "gameplayer_id": 104, "overall": 90})
        await r(a, 3, {"msgid": "CMD_CREATE_MYCLUB_STAFF", "rqid": 3, "player_info_list": [el]})
        club = self.srv.datos.club(a.steam_hex)
        orden = club["inscritos"]["1"]
        self.assertEqual(len(orden), 18)
        self.assertEqual(orden[4], 19)  # mismo puesto, primer jugador libre
        self.assertNotIn(5, orden)
        self.assertNotIn(5, [j.get("gameplayer_unique_id") for j in club["jugadores"]])

    async def test_myclub_usar_entrenador_de_experiencia(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 100 + i} for i in range(20)],
            "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 1000}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        el = msgpack_min.packb({"gameplayer_unique_id": 20, "gameplayer_id": 119, "overall": 150})
        await r(a, 2, {"msgid": "CMD_CREATE_MYCLUB_STAFF", "rqid": 2, "player_info_list": [el]})
        staff_id = self.srv.datos.club(a.steam_hex)["staff"][0]["id"]
        await r(a, 9, {"msgid": "CMD_GET_MYCLUB_MAINMENU_INFO", "rqid": 9})
        self.assertEqual(ea[-1][1]["staff_num"], 1)
        miembros = [msgpack_min.packb({"gameplayer_unique_id": u, "parameters": [95, 94, 95]})
                    for u in range(1, 19)]
        await r(a, 8, {"msgid": "CMD_SET_MYCLUB_SQUAD_GAMEPLAYER", "rqid": 8, "squad_id": 1,
                       "member_list": miembros})
        info = msgpack_min.packb({"gameplayer_unique_id": 3, "gameplayer_id": 102,
                                  "ability": [16, 36, 71, 90, 32, 14]})
        await r(a, 3, {"msgid": "CMD_USE_MYCLUB_STAFF", "rqid": 3, "player_info": info,
                       "staff_info_list": [msgpack_min.packb({"id": staff_id})],
                       "use_item_list": []})
        resp = ea[-1][1]
        self.assertEqual((resp["result"], resp["exp_added"]), ("NOERR", 150))
        post = resp["exp_info_post"].valor
        self.assertEqual((post["exp_total"], post["level"]), (150, 2))
        self.assertEqual(post["parameters"], [95, 94, 95])
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual(club["staff"], [])
        # el mismo entrenador no se puede usar dos veces
        await r(a, 4, {"msgid": "CMD_USE_MYCLUB_STAFF", "rqid": 4, "player_info": info,
                       "staff_info_list": [msgpack_min.packb({"id": staff_id})]})
        self.assertEqual(ea[-1][1]["result"], "ERR_NOTFOUND")

    async def test_myclub_boleto_de_contrato(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        await r(a, 2, {"msgid": "CMD_EXTEND_MYCLUB_ITEM_GAMEPLAYER", "rqid": 2,
                       "gameplayer_unique_id": 1})
        self.assertEqual(ea[-1][1]["result"], "ERR_FORBIDDEN")
        club = self.srv.datos.club(a.steam_hex)
        club["boletos_contrato"] = 1
        antes = self.srv._contrato(club["jugadores"][0])
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 3, {"msgid": "CMD_EXTEND_MYCLUB_ITEM_GAMEPLAYER", "rqid": 3,
                       "gameplayer_unique_id": 1})
        resp = ea[-1][1]
        self.assertEqual((resp["result"], resp["extend_ticket"]), ("NOERR", 0))
        self.assertEqual(self.srv.datos.club(a.steam_hex)["jugadores"][0]["contract_term"],
                         antes + 900)

    async def test_myclub_itemlist_cuenta_boletos_y_objetos(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        club = self.srv.datos.club(a.steam_hex)
        club.update({"boletos_contrato": 3, "objetos_recuperar": 2})
        for j in club["jugadores"]:
            j["tiredness"] = 4
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_ITEMLIST", "rqid": 2})
        items = {i.valor["item_type"]: i.valor["count"] for i in ea[-1][1]["item_list"]}
        self.assertEqual(items, {"CONTRACT": 3, "RECOVERY": 2})
        # los valores se cambian en caliente desde config
        self.srv.cfg["myclub"]["objetos"] = [{"clave": "boletos_contrato", "item_type": "X",
                                              "item_variation": "Y"}]
        await r(a, 3, {"msgid": "CMD_GET_MYCLUB_ITEMLIST", "rqid": 3})
        el = ea[-1][1]["item_list"][0].valor
        self.assertEqual((el["item_type"], el["item_variation"], el["count"]), ("X", "Y", 3))
        await r(a, 4, {"msgid": "CMD_USE_MYCLUB_ITEM_RECOVERY", "rqid": 4,
                       "gameplayer_unique_id_list": [1], "is_all": "NO", "use_pes_point": "NO"})
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual((club["objetos_recuperar"], club["jugadores"][0]["tiredness"]), (1, 0))

    async def test_myclub_recuperar_cansancio(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}, {"gameplayer_id": 2}],
            "entrenadores": [{"coach_id": 8}], "club_de_prueba": {"team_name": "P", "gp": 0}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 1})
        club = self.srv.datos.club(a.steam_hex)
        for j in club["jugadores"]:
            j["tiredness"] = 3
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 2, {"msgid": "CMD_USE_MYCLUB_ITEM_RECOVERY", "rqid": 2,
                       "gameplayer_unique_id_list": [1], "is_all": "NO", "use_pes_point": "NO"})
        self.assertEqual(ea[-1][1]["result"], "NOERR")
        self.assertEqual([j["tiredness"] for j in self.srv.datos.club(a.steam_hex)["jugadores"]],
                         [0, 3])
        await r(a, 3, {"msgid": "CMD_USE_MYCLUB_ITEM_RECOVERY", "rqid": 3,
                       "gameplayer_unique_id_list": [], "is_all": "YES"})
        self.assertEqual([j["tiredness"] for j in self.srv.datos.club(a.steam_hex)["jugadores"]],
                         [0, 0])

    async def test_myclub_subastas_de_la_casa(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 5000},
            "cazatalentos": [{"agent_id": 50, "grade": 3, "sell_gp": 500, "equipos": [103]}],
            "subastas": {"cantidad": 2, "duracion_min": 60, "incremento": 100}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_REFINE_SEARCH_AUCTION_LIST", "rqid": 1,
                       "start_index": 0})
        resp = ea[-1][1]
        self.assertEqual(resp["total_auction_info_list_num"], 2)
        info = resp["auction_info_list"][0].valor
        self.assertEqual((info["scout_id"], info["grade"], info["price_now"], info["auction_state"]),
                         (50, 3, 3000, "AUCTION_USER_STATE_NONE"))
        pid = info["product_id"]
        await r(a, 2, {"msgid": "CMD_TENDER_MYCLUB_AUCTION_SCOUT", "rqid": 2, "product_id": pid,
                       "tender_gp": 2000, "is_finalround": "NO"})
        self.assertEqual(ea[-1][1]["result"], "ERR_INSUFFICIENT_TENDER_GP")
        await r(a, 3, {"msgid": "CMD_TENDER_MYCLUB_AUCTION_SCOUT", "rqid": 3, "product_id": pid,
                       "tender_gp": 3000, "is_finalround": "NO"})
        resp = ea[-1][1]
        self.assertEqual((resp["result"], resp["gp"]), ("NOERR", 2000))
        det = resp["auction_scout_detail"].valor
        self.assertEqual((det["my_price"], det["auction_state"]), (3000, "AUCTION_USER_STATE_HIGHEST_BIDER"))
        await r(a, 4, {"msgid": "CMD_ADD_MYCLUB_AUCTION_WATCH_LIST", "rqid": 4, "product_id": pid})
        await r(a, 5, {"msgid": "CMD_GET_MYCLUB_AUCTION_WATCH_LIST", "rqid": 5})
        self.assertEqual(ea[-1][1]["total_auction_info_list_num"], 1)
        # se vence la subasta: el cazatalentos pasa al club y queda en el historial
        club = self.srv.datos.club(a.steam_hex)
        club["scouts"] = []
        for p in club["subastas"]["productos"]:
            p["fin"] = 0
        self.srv.datos.guardar_club(a.steam_hex, club)
        await r(a, 6, {"msgid": "CMD_GET_MYCLUB_AUCTION_REPORT_LIST", "rqid": 6})
        self.assertEqual(ea[-1][1]["total_auction_report_list_num"], 1)
        club = self.srv.datos.club(a.steam_hex)
        self.assertEqual([(c["agent_id"], c["grade"]) for c in club["scouts"]], [(50, 3)])
        self.assertEqual(club["gp"], 2000)
        self.assertEqual(len(club["subastas"]["productos"]), 2)  # la casa repone
        await r(a, 7, {"msgid": "CMD_GET_MYCLUB_AUCTION_PRICE_RANKING", "rqid": 7,
                       "ranking_kind": "X"})
        self.assertEqual(ea[-1][1]["auction_price_ranking"][0].valor["price"], 3000)
        await r(a, 8, {"msgid": "CMD_GET_MYCLUB_AUCTION_SUCCESSFUL_BID_INFO", "rqid": 8,
                       "product_id": pid})
        self.assertEqual(ea[-1][1]["best_bid_amount_price"], 3000)

    async def test_myclub_subasta_rival_de_la_casa_devuelve_gp(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 5000},
            "cazatalentos": [{"agent_id": 50, "grade": 1}],
            "subastas": {"cantidad": 1, "rival_casa": {"activo": True, "probabilidad": 1}}}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_GET_MYCLUB_REFINE_SEARCH_AUCTION_LIST", "rqid": 1})
        pid = ea[-1][1]["auction_info_list"][0].valor["product_id"]
        await r(a, 2, {"msgid": "CMD_TENDER_MYCLUB_AUCTION_SCOUT", "rqid": 2, "product_id": pid,
                       "tender_gp": 1000})
        resp = ea[-1][1]
        self.assertEqual(resp["gp"], 5000)  # la casa pujó más: el GP vuelve
        self.assertEqual(resp["auction_scout_detail"].valor["price_now"], 1100)
        await r(a, 3, {"msgid": "CMD_REQWATCH_MYCLUB_AUCTION_REFUNDED_GP", "rqid": 3})
        self.assertEqual(ea[-1][1]["total_gp"], 1000)

    async def test_myclub_monedas_gratis(self):
        self.srv.cfg = {**self.srv.cfg, "myclub": {
            "jugadores": [{"gameplayer_id": 1}], "entrenadores": [{"coach_id": 8}],
            "club_de_prueba": {"team_name": "P", "gp": 0, "monedas": 400},
            "mercado_dt": [{"coach_id": 24, "gp": 5000, "pes_point": 300}]}}
        a, ea = await self.entrar("A1")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_CHECK_MYCLUB_STORE", "rqid": 1})
        self.assertEqual(ea[-1][1]["is_enable_myclub_coin"], "NO")
        await r(a, 2, {"msgid": "CMD_GET_MYCLUB_USER_INFO", "rqid": 2})
        self.assertEqual(ea[-1][1]["pes_point"], 0)
        self.srv.cfg["myclub"]["monedas_gratis"] = True
        await r(a, 3, {"msgid": "CMD_CHECK_MYCLUB_STORE", "rqid": 3})
        self.assertEqual(ea[-1][1]["is_enable_myclub_coin"], "YES")
        await r(a, 4, {"msgid": "CMD_BUY_MYCLUB_MARKET_COACH", "rqid": 4, "coach_id": 24,
                       "use_pes_point": "YES"})
        self.assertEqual((ea[-1][1]["pes_point"], ea[-1][1]["gp"]), (100, 0))

    def test_avatar_http_vacio_y_guardado(self):
        r = self.srv.respuesta_http({"msgid": "CMD_GET_AVATAR_HTTP", "rqid": "3", "pid": "7"})
        self.assertEqual((r["result"], r["avatar_data"], r["avatar_data_size"]), ("NOERR", "", 0))
        import base64, hashlib
        img = bytes(49152)
        self.srv.avatares["7"] = base64.b64encode(img).decode()
        r = self.srv.respuesta_http({"msgid": "CMD_GET_AVATAR_HTTP", "rqid": "3", "pid": "7"})
        self.assertEqual((r["avatar_data_size"], r["avatar_md5"]), (49152, hashlib.md5(img).hexdigest()))

    async def test_direccion_del_anfitrion_para_rival_remoto(self):
        """R (pesmatch._for_viewer AzrakHQ): el PES de la PC del servidor declara 127.0.0.1; a un rival que
        llega por Tailscale/internet se le da host_addr con la IP pública y el mismo puerto; reflexive igual."""
        import base64, struct
        a, ea = await self.entrar("0110000100000061")
        b, eb = await self.entrar("0110000100000062")
        a.direccion, b.direccion = ("127.0.0.1", 5000), ("100.80.1.2", 5000)
        sa = struct.pack(">HH", 5739, 0) + bytes([127, 0, 0, 1]) + bytes(6)
        refl = msgpack_min.packb({"address_family": 1, "sa_data": base64.b64encode(sa).decode()})
        await self.srv.recibir(a, 1, {"msgid": "CMD_SEND_IPANDPORT_NTL", "rqid": 1, "ntl_nat_type": 1,
                                      "reflexive_addr": refl})
        r = await self.srv.c_cmd_get_ipandport_ntl(b, {"pid": self.srv._pid_visible(a)})
        self.assertEqual(r["reflexive_addr"], refl)
        h = msgpack_min.unpackb(r["host_addr"])
        crudo = base64.b64decode(h["sa_data"])
        self.assertEqual((struct.unpack(">H", crudo[:2])[0], ".".join(map(str, crudo[4:8]))), (5739, "1.2.3.4"))
        # el propio PES y un rival de la misma casa lo ven tal cual
        self.assertNotIn("host_addr", await self.srv.c_cmd_get_ipandport_ntl(a, {"pid": self.srv._pid_visible(a)}))
        b.direccion = ("192.168.1.5", 5000)
        self.assertNotIn("host_addr", await self.srv.c_cmd_get_ipandport_ntl(b, {"pid": self.srv._pid_visible(a)}))

    async def test_no_empareja_datos_distintos(self):
        """R (pesmatch._compatible AzrakHQ): sumas de CHECK_CHEAT distintas = no se emparejan (DATAPACK MXNV533)."""
        a, ea = await self.entrar("0110000100000051")
        b, eb = await self.entrar("0110000100000052")
        c, ec = await self.entrar("0110000100000053")
        r = self.srv.recibir
        await r(a, 1, {"msgid": "CMD_CHECK_CHEAT", "rqid": 1, "param_check_sum": 11, "entry_check_sum": 22})
        await r(b, 1, {"msgid": "CMD_CHECK_CHEAT", "rqid": 1, "param_check_sum": 99, "entry_check_sum": 22})
        await r(c, 1, {"msgid": "CMD_CHECK_CHEAT", "rqid": 1, "param_check_sum": 11, "entry_check_sum": 22})
        await r(a, 2, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_QUICK"})
        await r(b, 2, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_QUICK"})
        self.assertNotIn(a, self.srv.parejas)
        await r(c, 2, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_QUICK"})
        self.assertIs(self.srv.parejas[c], self.srv.parejas[a])

    async def test_ranking_por_orden_y_mi_posicion(self):
        """R (pesranking AzrakHQ): sort cambia el orden y point; Tasa de victorias pide 10+ partidos; _BYID sin
        buscado = ventana alrededor de quien pregunta, aunque esté fuera de la 1.ª página."""
        from phoenix_online import divisiones as dv
        claves = []
        for i in range(8):
            k = f"01101001000001{i:02d}"
            self.srv.datos.crear_jugador(k, f"J{i}")
            e = dv.estado_nuevo()
            e.update({"jugados": 10 + i, "win": i, "loss": 10, "rating": 1000 + 10 * (7 - i), "racha_max": i})
            self.srv.datos.guardar_division(k, e)
            claves.append(k)
        r = self.srv._ranking_division({"sort": "WIN_NUM", "start_index": 0, "view_num": 3})
        self.assertEqual([f["point"] for f in r["rank_list"]], [7, 6, 5])
        r = self.srv._ranking_division({"sort": "RATING", "start_index": 0, "view_num": 3})
        self.assertEqual(r["rank_list"][0]["user"]["pname"]["name"], "J0")
        self.assertIn(r["rank_list"][0]["user"]["manner"], ("A", "B", "C"))
        yo = self.srv.numero_de_clave(claves[0]) & 0xFFFFFFFF
        r = self.srv.respuesta_http({"msgid": "CMD_GET_RANKING_BYID", "rqid": "1", "pid": str(yo),
                                     "sort": "WIN_NUM", "start_index": "0", "view_num": "2"})
        self.assertIn("J0", [f["user"]["pname"]["name"] for f in r["rank_list"]])

    async def test_lista_de_bloqueo_y_palabras(self):
        """R (pesblock/peswords AzrakHQ): la lista de bloqueo de partidos se ve; palabras enteras."""
        a, ea = await self.entrar("0110000100000041")
        b, eb = await self.entrar("0110000100000042")
        await self.srv.recibir(a, 1, {"msgid": "CMD_SET_MATCHREJECTPID", "rqid": 1,
                                      "reject_pid": self.srv._pid_visible(b), "is_reject": "YES"})
        r = await self.srv.c_cmd_get_matchrejectlist(a, {})
        self.assertEqual(r["reject_list_num"], 1)
        msgpack_min.packb(r)
        self.assertEqual(await self.srv.c_cmd_check_string(a, {"str": "Computadora de la disputa"}), {})
        self.assertEqual((await self.srv.c_cmd_check_string(a, {"str": "p.u.t.a"}))["result"], "ERR_NGWORD")

    def test_actualizacion_en_vivo_propia(self):
        """R (liveupdate AzrakHQ): un .cpk con version_wu.bin en la carpeta se ofrece en GET_DLC_FILELIST con
        sha256 y dlc_version entera; sin carpeta, listas vacías como antes."""
        import hashlib, struct, tempfile
        from phoenix_online import actualizacion

        def utf(cols, filas):
            textos, datos_col = bytearray(b"<NULL>\0"), []
            def s(x):
                o = len(textos); textos.extend(x.encode() + b"\0"); return o
            fmt = {4: ">I", 6: ">Q", 0xA: ">I"}
            cab = b"".join(bytes([0x50 | ty]) + struct.pack(">I", s(n)) for n, ty in cols)
            fila_b = []
            for f in filas:
                fila_b.append(b"".join(struct.pack(fmt[ty], s(f[n]) if ty == 0xA else f[n]) for n, ty in cols))
            largo = len(fila_b[0])
            filas_off = 24 + len(cab)
            str_off = filas_off + largo * len(filas)
            data_off = str_off + len(textos)
            cuerpo = struct.pack(">IIIIHHI", filas_off, str_off, data_off, 0, len(cols), largo, len(filas)) + cab \
                + b"".join(fila_b) + bytes(textos)
            return b"@UTF" + struct.pack(">I", len(cuerpo)) + cuerpo

        version = b"1.02".ljust(10, b"\0") + b"1.07".ljust(10, b"\0") + b"Phoenix".ljust(10, b"\0")
        toc_utf = utf([("DirName", 0xA), ("FileName", 0xA), ("FileSize", 4), ("ExtractSize", 4), ("FileOffset", 6)],
                      [{"DirName": "common/etc/version", "FileName": "version_wu.bin", "FileSize": 30,
                        "ExtractSize": 30, "FileOffset": 0}])
        cab_len = 16 + len(utf([("TocOffset", 6), ("ContentOffset", 6)], [{"TocOffset": 0, "ContentOffset": 0}]))
        toc_off = cab_len
        contenido_off = toc_off + 16 + len(toc_utf)
        cab = utf([("TocOffset", 6), ("ContentOffset", 6)], [{"TocOffset": toc_off, "ContentOffset": contenido_off}])
        toc_utf = utf([("DirName", 0xA), ("FileName", 0xA), ("FileSize", 4), ("ExtractSize", 4), ("FileOffset", 6)],
                      [{"DirName": "common/etc/version", "FileName": "version_wu.bin", "FileSize": 30,
                        "ExtractSize": 30, "FileOffset": contenido_off - toc_off}])
        cpk = b"CPK " + bytes(4) + struct.pack("<Q", len(cab)) + cab + b"TOC " + bytes(4) \
            + struct.pack("<Q", len(toc_utf)) + toc_utf + version
        with tempfile.TemporaryDirectory() as d:
            self.srv.cfg["carpeta_actualizacion"] = d
            r = self.srv.respuesta_http({"msgid": "CMD_GET_DLC_FILELIST", "rqid": "1"}, ip_local="100.70.0.1")
            self.assertEqual(r["dlcfile_list_count"], 0)
            with open(os.path.join(d, "Phoenix_102.cpk"), "wb") as f:
                f.write(cpk)
            r = self.srv.respuesta_http({"msgid": "CMD_GET_DLC_FILELIST", "rqid": "1"}, ip_local="100.70.0.1")
            fila = r["dlcfile_list"][0]
            self.assertEqual((fila["dlc_version"], fila["size"], fila["hash"]),
                             ("102", len(cpk), hashlib.sha256(cpk).hexdigest()))
            self.assertTrue(fila["url"].endswith("/actualizacion/Phoenix_102.cpk"))
            self.assertIsNotNone(actualizacion.archivo_para(d, "phoenix_102.cpk"))
            self.assertIsNone(actualizacion.archivo_para(d, "../config.json"))
            msgpack_min.packb(r)
            # el escritor propio arma un CPK que el lector propio entiende (forma de los CPK del PES)
            tabla = os.path.join(d, "Team.bin")
            with open(tabla, "wb") as f:
                f.write(b"WESYS" + bytes(3000))
            ver = os.path.join(d, "v.bin")
            with open(ver, "wb") as f:
                f.write(actualizacion.archivo_version("1.03"))
            salida = os.path.join(d, "Phoenix_103.cpk")
            actualizacion.escribir_cpk(salida, [(actualizacion.ARCHIVO_VERSION, ver), ("common/etc/pesdb/Team.bin", tabla)])
            self.assertEqual(actualizacion.leer_de_cpk(salida, "common/etc/pesdb/Team.bin")[:5], b"WESYS")
            self.assertEqual(actualizacion.actual(d)[1]["dlc_version"], "103")
            with open(salida, "rb") as f:
                cab = f.read(0x800)
            self.assertEqual((cab[:4], cab[0x7FA:0x800]), (b"CPK ", b"(c)CRI"))

    async def test_playdata_y_tope_por_rival(self):
        """R (results.md AzrakHQ / OpenEleven): SET_PLAYDATA_MATCH se guarda; con max_partidos_rival_dia el
        mismo cruce deja de dar puntos ese día."""
        import json as _j
        a, ea = await self.entrar("0110000100000031")
        b, eb = await self.entrar("0110000100000032")
        pid = str(self.srv.numero_de_clave(a.steam_hex) & 0xFFFFFFFF)
        jd = {"scorer": [7511, 0], "team": [{"team_id": 101, "member": [{"player_id": 7511, "rating": 7.5,
                                                                        "play_time": 90}, None]}]}
        r = self.srv.respuesta_http({"msgid": "CMD_SET_PLAYDATA_MATCH", "rqid": "1", "pid": pid,
                                     "play_data": _j.dumps(jd)})
        self.assertEqual(r["result"], "NOERR")
        h = self.srv.datos.historial(a.steam_hex)["playdata"][-1]["resumen"]
        self.assertEqual((h["goleadores"], h["notas"]["7511"]["nota"]), ([7511], 7.5))
        self.srv.cfg["max_partidos_rival_dia"] = 2
        vistos = [self.srv._puntua({"miembros": (a, b)}) for _ in range(3)]
        self.assertEqual(vistos, [True, True, False])
        self.srv.cfg["max_partidos_rival_dia"] = 0
        self.assertTrue(self.srv._puntua({"miembros": (a, b)}))

    async def test_avatar_subido_queda_guardado(self):
        """R (pesavatar AzrakHQ): SET_AVATAR se guarda en la base y se lee con cualquier forma del pid."""
        import base64, hashlib
        a, ea = await self.entrar("0110000100000071")
        img = bytes(range(256)) * 192
        b64 = base64.b64encode(img).decode()
        await self.srv.recibir(a, 1, {"msgid": "CMD_SET_AVATAR", "rqid": 1, "avatar_data": b64,
                                      "avatar_data_size": len(img), "avatar_md5": hashlib.md5(img).hexdigest()})
        self.srv.avatares.clear()
        bajo = str(self.srv.numero_de_clave(a.steam_hex) & 0xFFFFFFFF)
        r = self.srv.respuesta_http({"msgid": "CMD_GET_AVATAR_HTTP", "rqid": "3", "pid": bajo})
        self.assertEqual((r["avatar_data"], r["avatar_data_size"]), (b64, 49152))
        await self.srv.recibir(a, 2, {"msgid": "CMD_SET_AVATAR", "rqid": 2, "avatar_data": "no*base64"})
        r = self.srv.respuesta_http({"msgid": "CMD_GET_AVATAR_HTTP", "rqid": "3", "pid": bajo})
        self.assertEqual(r["avatar_data_size"], 49152)   # lo inválido no borra el bueno

    async def test_tarjeta_con_nombre_en_conexion_menu(self):
        await self.entrar("A1")
        self.srv.datos.crear_jugador(Servidor._steam_de({"username": "A1"}) or "a1", "FRALEX")
        s, env = cliente(self.srv)
        await self.srv.recibir(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "A1"})
        await self.srv.recibir(s, 2, {"msgid": "CMD_START_MATCHING", "rqid": 2})
        tarjeta = msgpack_min.unpackb(msgpack_min.packb(env[-1][1]["member"][0].valor))
        nombre = msgpack_min.unpackb(tarjeta["pname"])["name"]
        self.assertEqual(nombre, "FRALEX")

    async def test_partida_rapida_cancelar(self):
        a, ea = await self.entrar("A1")
        await self.srv.recibir(a, 21, {"msgid": "CMD_START_MATCHING", "rqid": 2})
        await self.srv.recibir(a, 22, {"msgid": "CMD_CANCEL_MATCHING", "rqid": 3})
        self.assertEqual(ea[-1][1]["matching_state"], "CANCEL")
        self.assertEqual(self.srv.colas["OC_QUICK"], [])

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


class TestGamerelay(unittest.TestCase):
    """Cliente falso contra el relé (claves falsas: las reales viven solo en la PC)."""

    def setUp(self):
        from phoenix_online import gamerelay as g
        self.g = g
        self.k_ext, self.k_int = bytes(range(256)), bytes(range(16, 32))

        class SrvFalso:
            def __init__(s):
                s.autorizadas = {("1.2.3.4", 1)}

            def sonda_autorizada(s, ip, indice):
                return (ip, indice) in s.autorizadas

        self.rele = g.Rele(SrvFalso(), self.k_ext, self.k_int)
        self.salida = []

        class T:
            def sendto(s, d, a):
                self.salida.append((d, a))

        self.rele.connection_made(T())
        self.addr = ("1.2.3.4", 9000)

    def test_cifrado_ida_vuelta_y_checksum(self):
        g = self.g
        m = g.armar_marco(0x80, b"hola mundo")
        c = g.cifrar_exterior(self.k_ext, m)
        self.assertNotEqual(c, m)
        self.assertEqual(g.leer_marco(g.descifrar_exterior(self.k_ext, c)), (0x80, b"hola mundo"))
        self.assertIsNone(g.leer_marco(b"\x80\x00\x00x"))

    def test_marco_comprimido_del_cliente(self):
        import struct, zlib
        g = self.g
        cuerpo = g.armar_transporte(0, [(0, 0, [(0, g.carga_de(b"\x80"))]), (1, 0, [])])
        flags = 0x74
        cs = g.suma_control(bytes([flags & ~0x40]) + b"\x00\x00" + cuerpo)
        plano = bytes([flags]) + zlib.compress(struct.pack(">H", cs) + cuerpo)
        self.assertEqual(g.leer_marco(plano), (flags, cuerpo))
        self.assertIsNone(g.leer_marco(bytes([flags]) + zlib.compress(b"\x00\x00" + cuerpo)))

    def test_transporte_ejemplo_real(self):
        # C (ESPEC §2): 50 52 | ack 0 | 1 elemento | seq 0 | largo-1 = 2 | 02 1f c0
        cuerpo = bytes.fromhex("00000000" "00" "5052" "00000000" "00" "00000000" "0002" "021fc0")
        seq, secs = self.g.leer_transporte(cuerpo)
        self.assertEqual(secs, [(5, 0, [(0, b"\x02\x1f\xc0")], [])])
        # C (04-nov): sección del canal 6 con lista A y B (marca 0x63)
        real = bytes.fromhex("00000079" "00" "0000" "00000001" "1010" "00000002" "5050" "00000001"
                             "6063" "00000071" "00" "00000071" "0005" "050ca546fd38"
                             "8000" "0009" "00000070" "050c1caa7bbd")
        _, secs6 = self.g.leer_transporte(real)
        self.assertEqual(secs6[-1], (6, 0x71, [(0x71, bytes.fromhex("050ca546fd38"))],
                                    [(0x70, bytes.fromhex("050c1caa7bbd"))]))
        self.assertEqual(self.g.bloques(secs[0][2][0][1]), [b"\x1f\xc0"])
        self.assertEqual(self.g.armar_transporte(0, [(5, 0, [(0, b"\x02\x1f\xc0")])]), cuerpo)

    def _mandar(self, clave, secciones, seq=0):
        g = self.g
        self.rele.datagram_received(
            g.cifrar_exterior(clave, g.armar_marco(0x80, g.armar_transporte(seq, secciones))), self.addr)

    def _leer(self, clave):
        g = self.g
        d, _ = self.salida[-1]
        flags, cuerpo = g.leer_marco(g.descifrar_exterior(clave, d))
        return dict((c, (ack, a)) for c, ack, a, _b in g.leer_transporte(cuerpo)[1])

    def test_sonda_completa(self):
        g = self.g
        # KX
        nonce = 0x01020304
        self.rele.datagram_received(
            g.cifrar_exterior(self.k_ext, g.armar_marco(0x80, g.rc4(self.k_int, nonce.to_bytes(4, "big")))),
            self.addr)
        flags, cuerpo = g.leer_marco(g.descifrar_exterior(self.k_ext, self.salida[-1][0]))
        self.assertEqual((flags, len(cuerpo)), (0x80, 256))
        m = g.rc4(self.k_int, cuerpo)  # material que el cliente usa luego como clave (P)
        # canal 0: SYN -> SYN+ACK
        self._mandar(m, [(0, 0, [(0, g.carga_de(b"\x80"))]), (1, 0, [])])
        r = self._leer(m)
        self.assertEqual(r[0], (1, [(0, g.carga_de(b"\x90"))]))
        # ACK confirma el SYN+ACK -> establecido -> lista de participantes
        self._mandar(m, [(0, 1, [(1, g.carga_de(b"\x10"))]), (1, 0, [])], seq=1)
        r = self._leer(m)
        self.assertEqual(r[0][1], [])
        self.assertEqual(r[5][1], [(0, g.carga_de(b"\x1f\xc0"))])
        # canal 1: abrir 6 -> aceptado; canal 6: eco 2 veces
        self._mandar(m, [(0, 1, []), (1, 0, [(0, g.carga_de(b"\x80\x06"))]), (5, 1, []),
                         (6, 0, [(0, g.carga_de(b"\x0c\xaa\xbb\xcc\xdd"))])], seq=2)
        r = self._leer(m)
        self.assertEqual(r[1][1], [(0, g.carga_de(b"\x90\x06"))])
        self.assertNotIn(5, {c for c, (ack, a) in r.items() if a})
        self.assertEqual([g.bloques(c) for _, c in r[6][1]], [[b"\x0c\xaa\xbb\xcc\xdd"]] * 2)
        self.assertEqual([q for q, _ in r[6][1]], [0, 1])  # una copia por elemento

    def test_sin_autorizacion_no_hay_lista(self):
        g = self.g
        self.addr = ("9.9.9.9", 1)
        self._mandar(self.k_ext, [(0, 0, [(0, g.carga_de(b"\x80"))]), (1, 0, [])])
        self._mandar(self.k_ext, [(0, 1, [(1, g.carga_de(b"\x10"))]), (1, 0, [])], seq=1)
        self.assertNotIn(5, self._leer(self.k_ext))


class TestBlindaje(unittest.IsolatedAsyncioTestCase):
    """Protecciones del 10-nov (docs/AUDITORIA_ONLINE_10NOV.md)."""

    async def asyncSetUp(self):
        import copy
        self.cfg = copy.deepcopy(CFG)
        self.srv = Servidor(self.cfg, Datos())
        self.srv.dormir = _sin_pausa

    def _cliente(self, ip, ip_local="192.168.1.10"):
        enviados = []
        s = Sesion(lambda seq, m: enviados.append((seq, m)), (ip, 5555), "MENU")
        s.ip_local = ip_local
        self.srv.abrir(s)
        return s, enviados

    async def test_identidad_por_ip_con_steam_compartido(self):
        r = self.srv.recibir
        claves = {}
        for ip in ("192.168.1.10", "100.64.0.7", "100.64.0.8", "127.0.0.1"):
            s, env = self._cliente(ip)
            await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "410000004100000"})
            self.assertEqual(env[-1][1]["result"], "NOERR")
            claves[ip] = s.steam_hex
        # anfitrión (misma PC) y 127.0.0.1 conservan el SteamID: el club de Frank no cambia
        self.assertEqual(claves["192.168.1.10"], "0410000004100000")
        self.assertEqual(claves["127.0.0.1"], "0410000004100000")
        # cada amigo, su propia clave fija
        self.assertEqual(claves["100.64.0.7"], "7f644000070" + "00000")
        self.assertNotEqual(claves["100.64.0.7"], claves["100.64.0.8"])
        # un SteamID propio (no compartido) no se toca
        s, _ = self._cliente("100.64.0.9")
        await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "np_account_id": 0x0110000100000001})
        self.assertEqual(s.steam_hex, "0110000100000001")
        # por_ip manda y se puede apagar
        self.cfg["identidad"] = {"por_ip": {"100.64.0.7": "00000000000000aa"}}
        self.assertEqual(self.srv.clave_identidad("0410000004100000", "100.64.0.7"), "00000000000000aa")
        self.cfg["identidad"] = {"activo": False}
        self.assertEqual(self.srv.clave_identidad("0410000004100000", "100.64.0.7"), "0410000004100000")

    async def test_ip_automatica_en_svrlist_urls_y_rele(self):
        self.cfg["ip_publica"] = "auto"
        self.cfg["urls"] = {**self.cfg["urls"], "TOPINFO": "http://{ip}:5000/topinfo"}
        self.cfg["gamerelay"] = {"activo": True, "indice": 1, "puerto": 5100, "ip": "auto"}
        s, env = self._cliente("100.64.0.7", ip_local="100.64.0.1")
        await self.srv.recibir(s, 1, {"msgid": "CMD_GET_SVRLIST", "rqid": 1})
        lista = [msgpack_min.unpackb(x) if isinstance(x, bytes) else x.valor for x in env[-1][1]["svrlist"]]
        self.assertEqual({x["svraddr"] for x in lista}, {"100.64.0.1"})
        await self.srv.recibir(s, 2, {"msgid": "CMD_GET_URLLIST", "rqid": 2})
        urls = {e.valor["type"]: e.valor["url"] for e in env[-1][1]["ulist"]}
        self.assertEqual(urls["TOPINFO"], "http://100.64.0.1:5000/topinfo")
        self.assertTrue(urls["EULA_PES"].startswith("http://100.64.0.1:"))
        nodos = self.srv.lista_calidad(1, "100.64.0.7", "100.64.0.1")["quality_check_data_list"]
        self.assertEqual(nodos[0]["ip_addr"], "100.64.0.1")
        # una IP fija sigue mandando (VPS)
        self.cfg["ip_publica"] = "203.0.113.5"
        self.assertEqual(self.srv.ip_anuncio(s), "203.0.113.5")

    async def test_mantenimiento_y_version(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            marca = os.path.join(d, "MANT.txt")
            self.cfg["archivo_mantenimiento"] = marca
            s, env = self._cliente("100.64.0.7")
            open(marca, "w").close()
            await self.srv.recibir(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "np_account_id": 5})
            self.assertEqual(env[-1][1]["result"], "ERR_SERVERNOTREADY")
            self.assertTrue(self.srv.estado()["mantenimiento"])
            os.remove(marca)
            self.cfg["exigir_versiones"] = {"patch_version": "1.07.02"}
            await self.srv.recibir(s, 2, {"msgid": "CMD_LOGIN", "rqid": 2, "np_account_id": 5,
                                          "patch_version": "1.06.00"})
            self.assertEqual(env[-1][1]["result"], "ERR_DIFFERENT_PATCHVERSION")
            await self.srv.recibir(s, 3, {"msgid": "CMD_LOGIN", "rqid": 3, "np_account_id": 5,
                                          "patch_version": "1.07.02"})
            self.assertEqual(env[-1][1]["result"], "NOERR")

    async def test_estado_cuenta_comandos_sin_manejador(self):
        s, _ = self._cliente("100.64.0.7")
        await self.srv.recibir(s, 1, {"msgid": "CMD_ALGO_NUEVO", "rqid": 1})
        await self.srv.recibir(s, 2, {"msgid": "CMD_ALGO_NUEVO", "rqid": 2})
        e = self.srv.estado()
        self.assertEqual(e["comandos_sin_manejador"], {"CMD_ALGO_NUEVO": 2})
        self.assertEqual(e["conexiones"], 1)

    def test_respaldo_de_la_base(self):
        import sqlite3, tempfile
        with tempfile.TemporaryDirectory() as d:
            ruta = os.path.join(d, "p.db")
            datos = Datos(ruta)
            datos.crear_jugador("00000000000000aa", "FRALEX")
            copias = [datos.respaldar(os.path.join(d, "r"), guardar=2) for _ in range(3)]
            import glob, time as _t
            self.assertTrue(os.path.exists(copias[-1]))
            import threading
            hilo = []
            t = threading.Thread(target=lambda: hilo.append(datos.respaldar(os.path.join(d, "r"), 5)))
            t.start(); t.join()
            self.assertTrue(hilo and os.path.exists(hilo[0]))  # como en el servidor: otro hilo
            con = sqlite3.connect(copias[-1])
            fila = con.execute("select pname from jugadores").fetchone()
            con.close()
            self.assertEqual(fila[0], "FRALEX")
            self.assertLessEqual(len(glob.glob(os.path.join(d, "r", "*.db"))), 2)
            self.assertEqual(datos.db.execute("pragma journal_mode").fetchone()[0], "wal")
            datos.cerrar()

    def test_presupuesto_de_capturas(self):
        import tempfile
        from phoenix_online import servidor
        with tempfile.TemporaryDirectory() as d:
            p = servidor.Presupuesto(d, total_mb=1, archivo_mb=1)
            self.assertTrue(p.permitir(1000))
            self.assertFalse(p.permitir(2 * 1024 * 1024))
            self.assertFalse(servidor.Presupuesto(d, activo=False).permitir(1))

    def test_troceado_absurdo_se_rechaza(self):
        import hashlib, struct
        from phoenix_online import nclmio
        codec = Codec(CLAVE_FALSA, comprobar_clave=False)
        interno = struct.pack(">IIIB", 1 << 30, 8, 8, 0) + b"\0" * 8
        interno += b"\0" * (-len(interno) % 8)
        cif = codec._enc(interno)
        cab = struct.pack(">HHI", 0x0070, len(cif), 1)
        trama = nclmio.xor(cab + hashlib.md5(cab + cif).digest() + cif)
        with self.assertRaises(ValueError):
            LectorTramas(codec).alimentar(trama)


class TestPartidaDos(unittest.TestCase):
    """Fase 2-3 del relé (ESPEC_GAMERELAY_FASE23): dos PES falsos en la misma partida."""

    def setUp(self):
        from phoenix_online import gamerelay as g
        self.g = g
        self.k_ext, self.k_int = bytes(range(256)), bytes(range(16, 32))

        class SrvFalso:
            def sonda_autorizada(s, ip, indice):
                return False

        self.rele = g.Rele(SrvFalso(), self.k_ext, self.k_int)
        self.salida = []

        class T:
            def sendto(s, d, a):
                self.salida.append((d, a))

        self.rele.connection_made(T())
        self.A, self.B = ("100.64.0.1", 50001), ("100.64.0.2", 50002)
        self.rele.abrir_intento(700000001, [
            {"pid": "aa", "enter_no": 0, "ip_tcp": "100.64.0.1", "endpoints": [self.A]},
            {"pid": "bb", "enter_no": 1, "ip_tcp": "100.64.0.2", "endpoints": [("100.64.0.2", 50002)]}])
        self.claves, self.seq = {}, {}

    def _kx(self, addr):
        g = self.g
        self.rele.datagram_received(g.cifrar_exterior(self.k_ext, g.armar_marco(
            0x80, g.rc4(self.k_int, (7).to_bytes(4, "big")))), addr)
        d, _ = self.salida[-1]
        _, cuerpo = g.leer_marco(g.descifrar_exterior(self.k_ext, d))
        self.claves[addr] = g.rc4(self.k_int, cuerpo)
        self.seq[addr] = 0

    def _mandar(self, addr, secciones, flags=0):
        g = self.g
        cuerpo = bytearray(g.armar_transporte(self.seq[addr], secciones))
        cuerpo[4] = flags
        self.seq[addr] += 1
        self.rele.datagram_received(g.cifrar_exterior(self.claves[addr], g.armar_marco(0x01, bytes(cuerpo))), addr)

    def _ultimo(self, addr):
        g = self.g
        d = [d for d, a in self.salida if a == addr][-1]
        _, cuerpo = g.leer_marco(g.descifrar_exterior(self.claves[addr], d))
        seq, flags, eco, secs = g.leer_datagrama(cuerpo)
        return {"flags": flags, "eco": eco, "tam": len(cuerpo),
                "c": {c: (ack, a) for c, ack, a, _b in secs}}

    def _conectar(self, addr):
        g = self.g
        self._kx(addr)
        self._mandar(addr, [(0, 0, [(0, g.carga_de(b"\x80"))]), (1, 0, [])])
        self._mandar(addr, [(0, 1, [(1, g.carga_de(b"\x10"))]), (1, 0, [])])
        self._mandar(addr, [(0, 2, []), (1, 0, [(0, g.carga_de(b"\x80\x05"), ), ])])
        for k, ch in enumerate((9, 10, 12, 15)):
            self._mandar(addr, [(0, 2, []), (1, 1 + k, [(1 + k, g.carga_de(bytes([0x80, ch])))])])

    def test_lista_de_tres_y_reenvio(self):
        g = self.g
        self._conectar(self.A)
        self._conectar(self.B)
        lista_a = [g.bloques(c) for _, c in self._ultimo(self.A)["c"][5][1]]
        lista_b = [g.bloques(c) for _, c in self._ultimo(self.B)["c"][5][1]]
        self.assertEqual(lista_a, [[bytes([0x1F, 0xC0, 0x81])]])
        self.assertEqual(lista_b, [[bytes([0x1F, 0xC1, 0x80])]])
        # canal 9 de A: dos bloques en un elemento (consumen 2 claves); llega etiquetado a los dos
        self._mandar(self.A, [(0, 2, []), (1, 5, []), (9, 0, [(0, g.carga_de(b"\x11", b"\x22"))])], flags=0x80)
        ra, rb = self._ultimo(self.A), self._ultimo(self.B)
        self.assertEqual(ra["c"][9][0], 2)  # ack = siguiente clave esperada (2 bloques)
        self.assertEqual([g.bloques(c)[0] for _, c in rb["c"][9][1]], [b"\x00\x11", b"\x00\x22"])
        self.assertEqual([g.bloques(c)[0] for _, c in ra["c"][9][1]], [b"\x00\x11", b"\x00\x22"])
        self.assertEqual(ra["flags"], 0x40)  # eco RTT pedido con 0x80
        self.assertEqual(ra["eco"][0], self.seq[self.A] - 1)
        # repetición del mismo elemento: no se reenvía otra vez
        antes = len(self._ultimo(self.B)["c"][9][1])
        self._mandar(self.A, [(0, 2, []), (1, 5, []), (9, 0, [(0, g.carga_de(b"\x11", b"\x22"))])])
        self.assertEqual(len(self._ultimo(self.B)["c"][9][1]), antes)
        # canal 15 de B: a A con cabecera 0x4010; la copia TCP idéntica no se entrega dos veces
        self._mandar(self.B, [(0, 2, []), (1, 5, []), (15, 0, [(0, g.carga_de(b"LISTO"))])])
        quince = [g.bloques(c) for _, c in self._ultimo(self.A)["c"][15][1]]
        self.assertEqual(quince, [[b"\x40\x10LISTO"]])
        self.assertEqual(self.rele.reenviar_tcp("bb", 15, b"LISTO", 1), 0)
        self.assertEqual(self.rele.reenviar_tcp("bb", 15, b"OTRO", 1), 1)
        self.assertEqual(self.rele.reenviar_tcp("bb", 15, b"OTRO2", 0), 0)  # fromId equivocado
        # reordenación: la clave 1 llega antes que la 0 y se entrega cuando se cierra el hueco
        self._mandar(self.B, [(0, 2, []), (1, 5, []), (12, 1, [(1, g.carga_de(b"seg"))])])
        self.assertEqual(self._ultimo(self.A)["c"].get(12, (0, []))[1], [])
        self._mandar(self.B, [(0, 2, []), (1, 5, []), (12, 0, [(0, g.carga_de(b"pri"))])])
        doce = [g.bloques(c)[0] for _, c in self._ultimo(self.A)["c"][12][1]]
        self.assertEqual(doce, [b"\x40\x10pri", b"\x40\x10seg"])

    def test_mensaje_largo_y_tope_de_2048(self):
        g = self.g
        self._conectar(self.A)
        self._conectar(self.B)
        grande = bytes(range(256)) * 3  # 768 B -> 4 bloques (2 de cabecera + 768 = 770)
        for k in range(6):
            self._mandar(self.B, [(0, 2, []), (1, 5, []), (10, k * 4, [(k * 4, g.carga_de(*g.trozos(grande)))])])
        r = self._ultimo(self.A)
        self.assertLessEqual(r["tam"], 0x800)
        elems = r["c"][10][1]
        self.assertTrue(elems)
        # cada bloque del emisor es un mensaje propio (C 10-nov): 2 + 255 B -> 2 bloques = 2 claves
        self.assertEqual(b"".join(g.bloques(elems[0][1])), b"\x40\x10" + grande[:255])
        claves = [k for k, _ in elems]
        self.assertEqual(claves, sorted(claves))
        self.assertEqual(claves[1] - claves[0], 2)

    def test_ambiguo_no_liga_y_salida_cierra(self):
        g = self.g
        self.rele.abrir_intento(700000002, [
            {"pid": "cc", "enter_no": 0, "ip_tcp": "100.64.0.1", "endpoints": []},
            {"pid": "dd", "enter_no": 1, "ip_tcp": "9.9.9.9", "endpoints": []}])
        otro = ("100.64.0.1", 40000)  # misma IP que A y que cc, puerto no declarado: ambiguo
        self._conectar(otro)
        self.assertNotIn(5, {c for c, (ack, a) in self._ultimo(otro)["c"].items() if a})
        self.rele.miembro_salio(700000001, "aa")
        self.rele.miembro_salio(700000001, "bb")
        self.assertNotIn(700000001, self.rele.intentos)

    def test_endpoint_de_ntl(self):
        import base64, struct
        sa = struct.pack(">H", 50002) + b"\0\0" + bytes([100, 64, 0, 2]) + b"\0" * 6
        self.assertEqual(self.g.endpoint_de_ntl({"sa_data": base64.b64encode(sa).decode()}),
                         ("100.64.0.2", 50002))
        self.assertIsNone(self.g.endpoint_de_ntl({"sa_data": ""}))


class TestHuellaParche(unittest.IsolatedAsyncioTestCase):
    async def test_solo_empareja_con_los_mismos_archivos(self):
        srv = Servidor(dict(CFG, emparejar_misma_huella=True), Datos())
        srv.dormir = _sin_pausa
        r = srv.recibir
        ses = []
        for steam, archivos in (("a1", ["x", "y"]), ("b2", ["x", "z"]), ("c3", ["x", "y"])):
            s, e = cliente(srv)
            await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": steam})
            await r(s, 2, {"msgid": "CMD_SEND_IPANDPORT_NTL", "rqid": 2, "file_hash_list": archivos})
            ses.append((s, e))
        (a, ea), (b, eb), (c, ec) = ses
        await r(a, 3, {"msgid": "CMD_START_MATCHING", "rqid": 3, "matching_type": "OC_RANK"})
        await r(b, 3, {"msgid": "CMD_START_MATCHING", "rqid": 3, "matching_type": "OC_RANK"})
        self.assertEqual(eb[-1][1]["matching_state"], "SEARCHING")  # parche distinto: sigue buscando
        await r(c, 3, {"msgid": "CMD_START_MATCHING", "rqid": 3, "matching_type": "OC_RANK"})
        self.assertEqual(ec[-1][1]["matching_state"], "FINISH")
        self.assertEqual(ea[-1][1]["matching_state"], "FINISH")


class TestRelePrueba10Nov(unittest.IsolatedAsyncioTestCase):
    """Lo que faltó en la prueba real del 10-nov (rival de prueba + PES base)."""

    async def test_get_gamerelay_http_y_matchid(self):
        import copy
        cfg = copy.deepcopy(CFG)
        cfg["ip_publica"] = "auto"
        cfg["gamerelay"] = {"activo": True, "indice": 1, "puerto": 5100, "ip": "auto"}
        srv = Servidor(cfg, Datos())
        srv.dormir = _sin_pausa
        r = srv.respuesta_http({"msgid": "CMD_GET_GAMERELAY", "pid": "2", "rqid": "7",
                                "urltype": "GET_GAMERELAY"}, "192.168.100.236", "192.168.100.236")
        self.assertEqual((r["result"], r["rqid"], r["gamerelay_num"]), ("NOERR", 7, 1))
        self.assertEqual(r["gamerelay_list"][0]["ip_addr"], "192.168.100.236")
        self.assertIsInstance(r["gamerelay_list"][0], dict)  # mapa en línea, no bin
        # campos sueltos que el cliente copia a su resultado (PESBUL R7.3.494)
        self.assertEqual((r["gamerelay_index"], r["ip_addr"], r["port"]), (1, "192.168.100.236", 5100))
        a, ea = cliente(srv)
        b, eb = cliente(srv)
        for s, u in ((a, "a1"), (b, "b2")):
            await srv.recibir(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": u})
            await srv.recibir(s, 2, {"msgid": "CMD_START_MATCHING", "rqid": 2, "matching_type": "OC_RANK"})
        await srv.recibir(b, 3, {"msgid": "CMD_GET_MATCHID", "rqid": 3})
        self.assertTrue(eb[-1][1]["match_id"].startswith("PHOENIX-0700000"))
        self.assertEqual(eb[-1][1]["upload"], "YES")


class TestNube(unittest.IsolatedAsyncioTestCase):
    """Espejo en Supabase con un Supabase falso (sin red)."""

    async def test_sube_clubes_reglas_y_movimientos(self):
        import io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        srv.datos.guardar_club("aa", {"gp": 100, "team_name": "Phoenix FC", "vscom": {"partidos": 3}})
        pedidos = []
        tablas = {"myclub_reglas": [{"reglas": {"gp_por_partido": 999}, "actualizado": "t1"}],
                  "myclub_movimientos": [{"id": 5, "clave_servidor": "AA", "tipo": "gp", "cantidad": 50, "motivo": "premio"},
                                         {"id": 6, "clave_servidor": "zz", "tipo": "gp", "cantidad": 1}]}

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            tabla = req.full_url.split("/rest/v1/")[1].split("?")[0]
            pedidos.append((req.get_method(), tabla, req.full_url, _j.loads(req.data) if req.data else None))
            if req.get_method() == "GET":
                return Resp(_j.dumps(tablas.get(tabla, [])).encode())
            return Resp(b"")

        n = Nube(srv, "https://x.supabase.co", "clave-falsa", abrir=abrir)
        await n.vuelta()
        post = [p for p in pedidos if p[0] == "POST"][0]
        self.assertEqual(post[3][0]["clave_servidor"], "aa")
        self.assertEqual((post[3][0]["gp"], post[3][0]["nombre"], post[3][0]["partidos"]), (100, "Phoenix FC", 3))
        self.assertEqual(srv.cfg["myclub"]["gp_por_partido"], 999)
        self.assertEqual(srv.datos.club("aa")["gp"], 150)
        self.assertEqual([p[2].split("?")[1] for p in pedidos if p[0] == "PATCH"], ["id=eq.5&aplicado=is.null"])
        # el club cambió por el movimiento: se vuelve a subir; las reglas ya vistas no se reaplican
        pedidos.clear()
        tablas["myclub_movimientos"] = []
        srv.cfg["myclub"]["gp_por_partido"] = 1
        await n.vuelta()
        self.assertEqual([p[0] for p in pedidos if p[0] == "POST"], ["POST"])
        self.assertEqual(srv.cfg["myclub"]["gp_por_partido"], 1)

    async def test_si_falla_la_subida_se_reintenta(self):
        from phoenix_online.nube import Nube
        srv = Servidor(CFG, Datos())
        srv.datos.guardar_club("aa", {"gp": 1})

        def abrir(req, timeout=None):
            raise OSError("sin internet")

        n = Nube(srv, "https://x.supabase.co", "k", abrir=abrir)
        with self.assertRaises(OSError):
            await n.vuelta()
        self.assertEqual(srv.datos.cambiados, {"aa"})


class TestRivalUdp(unittest.IsolatedAsyncioTestCase):
    """10-nov: el rival de prueba entra al relé por UDP (KX, canal 0, claves 5/14/15) y queda ligado."""

    async def _par(self, rele, k_ext, k_int, espejo=False):
        from phoenix_online.gamerelay_cliente import ClienteRele
        loop = asyncio.get_running_loop()
        t, c = await loop.create_datagram_endpoint(lambda: ClienteRele(k_ext, k_int, espejo=espejo),
                                                   remote_addr=("127.0.0.1", self.puerto))
        self.addCleanup(t.close)
        return c, t.get_extra_info("sockname")[:2]

    async def test_dos_clientes_ligados_y_reenvio(self):
        from phoenix_online import gamerelay
        from phoenix_online.gamerelay_cliente import endpoint_ntl
        k_ext, k_int = bytes(range(256)), bytes(range(16))
        rele = gamerelay.Rele(None, k_ext, k_int)
        loop = asyncio.get_running_loop()
        t, _ = await loop.create_datagram_endpoint(lambda: rele, local_addr=("127.0.0.1", 0))
        self.addCleanup(t.close)
        self.puerto = t.get_extra_info("sockname")[1]
        pes, ep_pes = await self._par(rele, k_ext, k_int)
        rival, ep_rival = await self._par(rele, k_ext, k_int, espejo=True)
        # la declaración NTL se decodifica igual que la del PES
        self.assertEqual(gamerelay.endpoint_de_ntl(endpoint_ntl(*ep_rival)), tuple(ep_rival))
        rele.abrir_intento(700000001, [
            {"pid": "pes", "enter_no": 0, "ip_tcp": "127.0.0.1", "endpoints": [tuple(ep_pes)]},
            {"pid": "rival", "enter_no": 1, "ip_tcp": "127.0.0.1", "endpoints": [tuple(ep_rival)]}])
        # el relé solo manda secciones de las claves abiertas: el 9 se abre como hace el PES al jugar
        self.assertTrue(await pes.entrar(abrir=(5, 9, 14, 15)))
        self.assertTrue(await rival.entrar(abrir=(5, 9, 14, 15)))
        vida = [asyncio.ensure_future(pes.mantener(15, cada=0.02)),
                asyncio.ensure_future(rival.mantener(15, cada=0.02))]
        await asyncio.wait_for(asyncio.gather(pes.lista_llego.wait(), rival.lista_llego.wait()), 3)
        self.assertEqual(pes.lista, bytes([0x1F, 0xC0, 0x81]))
        self.assertEqual(rival.lista, bytes([0x1F, 0xC1, 0x80]))
        self.assertEqual(sorted(rele.intentos[700000001].pares), ["pes", "rival"])
        # el «PES» manda por el 15 un mensaje en fragmentos de 16 B (C 10-nov): cada bloque es un
        # mensaje propio; el rival lo recibe con la cabecera 0x4000 y, en modo espejo, lo devuelve
        frag = [b"\x40\xdd" + bytes(14), b"\x8f" + bytes(range(15)), b"\x8c" + bytes(12)]
        pes.poner(15, gamerelay.carga_de(*frag))
        for _ in range(100):
            if sum(c == 15 for c, _b in pes.recibidos) >= 3:
                break
            await asyncio.sleep(0.03)
        self.assertEqual([b for c, b in rival.recibidos if c == 15], [b"\x40\x00" + f for f in frag])
        self.assertEqual([b for c, b in pes.recibidos if c == 15], [b"\x40\x10" + f for f in frag])
        # canal 9: eco propio y copia para el otro con la etiqueta id<<3
        pes.poner(9, gamerelay.carga_de(b"\xfa"))
        for _ in range(100):
            if any(c == 9 and b[0] == 1 << 3 for c, b in pes.recibidos):
                break
            await asyncio.sleep(0.03)
        self.assertIn((9, b"\x00\xfa"), rival.recibidos)
        self.assertIn((9, b"\x00\xfa"), pes.recibidos)          # eco propio
        self.assertIn((9, b"\x08\xfa"), pes.recibidos)          # devuelto por el rival espejo
        for v in vida:
            v.cancel()


class TestPase(unittest.IsolatedAsyncioTestCase):
    """Pase de identidad de la web: liga la IP del amigo a su cuenta Phoenix (con Supabase falso)."""

    async def test_canjear_pase_liga_ip_y_sube_dueno(self):
        import hashlib as _h, io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        pases = {_h.sha256(b"ab" * 18).hexdigest(): "uuid-ana"}
        pedidos = []

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            tabla, _, q = req.full_url.split("/rest/v1/")[1].partition("?")
            pedidos.append((req.get_method(), tabla, q, _j.loads(req.data) if req.data else None))
            if req.get_method() == "PATCH" and tabla == "phoenix_pases":
                h = q.split("hash=eq.")[1].split("&")[0]
                u = pases.pop(h, None)  # un pase solo sirve una vez
                return Resp(_j.dumps([{"usuario": u}] if u else []).encode())
            return Resp(b"[]")

        # sin Supabase no hay pase
        self.assertEqual((await srv.canjear_pase("ab" * 18, "100.64.0.7"))[0], 503)
        srv.nube = Nube(srv, "https://x.supabase.co", "k", abrir=abrir)
        self.assertEqual((await srv.canjear_pase("no-hex!", "100.64.0.7"))[0], 400)
        self.assertEqual((await srv.canjear_pase("ab" * 18, "127.0.0.1"))[0], 400)  # PC del servidor
        codigo, r = await srv.canjear_pase("AB" * 18, "100.64.0.7")
        self.assertEqual((codigo, r["ok"]), (200, True))
        self.assertNotIn("ab" * 18, repr(pedidos))  # solo viaja el hash
        self.assertEqual((await srv.canjear_pase("ab" * 18, "100.64.0.7"))[0], 403)  # ya usado
        clave = Servidor.clave_de_usuario("uuid-ana")
        # el SteamID compartido desde esa IP ya no da la clave por IP sino la de la cuenta
        self.assertEqual(srv.clave_identidad("0410000004100000", "100.64.0.7"), clave)
        self.assertEqual(srv.clave_identidad("0410000004100000", "100.64.0.8"), "7f644000080000"[:2] + "64400008" + "000000")
        self.assertEqual(srv.clave_identidad("0410000004100000", "127.0.0.1"), "0410000004100000")
        # el club sube a la web con su dueño
        srv.datos.guardar_club(clave, {"gp": 5})
        await srv.nube.subir_clubes()
        post = [p for p in pedidos if p[0] == "POST"][0]
        self.assertEqual(post[3][0]["usuario"], "uuid-ana")

    async def test_tope_de_intentos(self):
        srv = Servidor({**CFG, "myclub": {}}, Datos())

        class NubeFalsa:
            async def canjear_pase(self, pase):
                return None
        srv.nube = NubeFalsa()
        codigos = [(await srv.canjear_pase("cd" * 18, "100.64.0.9"))[0] for _ in range(11)]
        self.assertEqual(codigos[:10], [403] * 10)
        self.assertEqual(codigos[10], 429)


class TestVincularWeb(unittest.IsolatedAsyncioTestCase):
    """«Vincular mi PES»: código en los Avisos del juego + nombre; el dueño llega de phoenix_duenos."""

    async def test_codigo_en_avisos_y_dueno_desde_la_web(self):
        import io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        duenos, pedidos = [], []

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            tabla, _, q = req.full_url.split("/rest/v1/")[1].partition("?")
            pedidos.append((req.get_method(), tabla, q, _j.loads(req.data) if req.data else None))
            if req.get_method() == "GET" and tabla == "phoenix_duenos":
                return Resp(_j.dumps(duenos).encode())
            return Resp(b"[]")

        class S:
            steam_hex = "7f644000070000"
        s = S()
        # sin espejo con la web no hay código; sin club tampoco
        self.assertFalse(any(a.get("announce_id") == 900 for a in srv._avisos_de(s)))
        srv.nube = Nube(srv, "https://x.supabase.co", "k", abrir=abrir)
        self.assertFalse(any(a.get("announce_id") == 900 for a in srv._avisos_de(s)))
        srv.datos.crear_jugador(s.steam_hex, "ana")
        srv.datos.guardar_club(s.steam_hex, {"gp": 10, "team_name": "Ana FC"})
        aviso = srv._avisos_de(s)[0]
        codigo = aviso["title"].rsplit(" ", 1)[1]
        self.assertTrue(codigo.isdigit() and len(codigo) == 6)
        self.assertEqual(srv._avisos_de(s)[0]["title"], aviso["title"])  # el mismo código siempre
        cuerpo = await srv.c_cmd_get_infobody(s, {"announce_id": 900})
        self.assertIn(codigo, cuerpo["announce_info"].valor["body"])
        await srv.nube.vuelta()
        sub = [p for p in pedidos if p[0] == "POST" and p[1] == "phoenix_vinculos"]
        self.assertEqual((sub[0][3][0]["codigo"], sub[0][3][0]["clave_servidor"]), (codigo, s.steam_hex))
        club = [p for p in pedidos if p[0] == "POST" and p[1] == "myclub_clubes"][0][3][0]
        self.assertEqual((club["nombre_jugador"], "usuario" in club), ("ana", False))
        # la persona escribe el código en la web -> phoenix_duenos -> el club se sube con su dueño
        pedidos.clear()
        duenos.append({"clave_servidor": s.steam_hex, "usuario": "uuid-ana"})
        await srv.nube.vuelta()
        self.assertFalse([p for p in pedidos if p[1] == "phoenix_vinculos" and p[0] == "POST"])  # ya subido
        club = [p for p in pedidos if p[0] == "POST" and p[1] == "myclub_clubes"][0][3][0]
        self.assertEqual(club["usuario"], "uuid-ana")
        self.assertFalse(any(a.get("announce_id") == 900 for a in srv._avisos_de(s)))  # ya no hace falta
        pedidos.clear()
        await srv.nube.vuelta()
        self.assertFalse([p for p in pedidos if p[1] == "phoenix_duenos"])  # nada pendiente


class TestLauncher(unittest.IsolatedAsyncioTestCase):
    """Phoenix Launcher: POST /entrar con la sesión de la web; exigir_launcher solo afecta al online."""

    async def test_entrar_y_exigir_launcher(self):
        import copy
        cfg = copy.deepcopy({**CFG, "myclub": {}})
        srv = Servidor(cfg, Datos())

        class NubeFalsa:
            async def usuario_de_token(self, token):
                return "uuid-ana" if token == "a.b.c" + "x" * 20 else None
        self.assertEqual((await srv.entrar_launcher("a.b.c" + "x" * 20, "100.64.0.7"))[0], 503)
        srv.nube = NubeFalsa()
        self.assertEqual((await srv.entrar_launcher("sin-puntos" * 3, "100.64.0.7"))[0], 400)
        self.assertEqual((await srv.entrar_launcher("x.y.z" + "x" * 20, "100.64.0.7"))[0], 403)
        codigo, r = await srv.entrar_launcher("a.b.c" + "x" * 20, "100.64.0.7")
        self.assertEqual(codigo, 200)
        self.assertEqual(srv.clave_identidad("0410000004100000", "100.64.0.7"), Servidor.clave_de_usuario("uuid-ana"))
        # anfitrión: presentado pero conserva su club
        codigo, r = await srv.entrar_launcher("a.b.c" + "x" * 20, "127.0.0.1")
        self.assertEqual((codigo, r["clave"]), (200, None))
        self.assertEqual(srv.clave_identidad("0410000004100000", "127.0.0.1"), "0410000004100000")

        class S:
            ip_local = "192.168.1.5"
            def __init__(self, ip): self.ip = ip
        cfg["identidad"] = {"exigir_launcher": True}
        self.assertFalse(srv._sin_launcher(S("100.64.0.7")))   # abrió el launcher
        self.assertTrue(srv._sin_launcher(S("100.64.0.8")))    # no lo abrió: sin online
        self.assertFalse(srv._sin_launcher(S("192.168.1.5")))  # la PC del servidor, exenta
        cfg["identidad"] = {}
        self.assertFalse(srv._sin_launcher(S("100.64.0.8")))   # apagado por defecto


class TestPidEnPartida(unittest.TestCase):
    """Prueba 6e: los 32 bits bajos del pid de FINISH son lo que el otro PES exige en REVISION_CHECK."""

    def _s(self, ip, clave, crudo):
        from phoenix_online.logica import Sesion
        s = Sesion(lambda *a: None, (ip, 1), "MENU")
        s.ip_local, s.steam_hex, s.steam_crudo, s.pid, s.pname = "192.168.1.5", clave, crudo, 7, "X"
        return s

    def test_amigos_con_steam_compartido_tienen_pid_distinto(self):
        import copy
        cfg = copy.deepcopy(CFG)
        srv = Servidor(cfg, Datos())
        compartido = "0410000004100000"
        # anfitrión: su SteamID tal cual (lo que ya pasó la prueba 6e)
        self.assertEqual(srv._pid_visible(self._s("127.0.0.1", compartido, compartido)), 0x0410000004100000)
        # dos amigos cuya IP termina igual: antes salían con los mismos 32 bits bajos
        a = srv.clave_identidad(compartido, "100.64.0.7")
        b = srv.clave_identidad(compartido, "100.64.1.7")
        pa = srv._pid_visible(self._s("100.64.0.7", a, compartido))
        pb = srv._pid_visible(self._s("100.64.1.7", b, compartido))
        bajos = {pa & 0xFFFFFFFF, pb & 0xFFFFFFFF, 0x04100000, 0x3A660}
        self.assertEqual(len(bajos), 4)
        self.assertEqual(pa >> 32, 0x01100001)
        # la clave guardada no cambia y el número es estable
        self.assertEqual(a, "7f644000070" + "00000")
        self.assertEqual(Servidor.numero_de_clave(a), pa)
        # un SteamID propio (no compartido) se queda igual
        self.assertEqual(Servidor.numero_de_clave("0110000100001234"), 0x0110000100001234)

    def test_modos_de_prueba(self):
        import copy
        cfg = copy.deepcopy(CFG)
        srv = Servidor(cfg, Datos())
        compartido = "0410000004100000"
        amigo = self._s("100.64.0.7", srv.clave_identidad(compartido, "100.64.0.7"), compartido)
        self.assertNotEqual(srv._pid_en_partida(amigo), 0x0410000004100000)
        cfg["identidad"] = {"pid_en_partida": "nativo"}
        self.assertEqual(srv._pid_en_partida(amigo), 0x0410000004100000)
        # pid_forzado manda sobre todo (solo para la prueba A/B en la PC del servidor)
        cfg["identidad"] = {"pid_forzado": {"127.0.0.1": "0110000100005678"}}
        anfitrion = self._s("127.0.0.1", compartido, compartido)
        self.assertEqual(srv._pid_en_partida(anfitrion), 0x0110000100005678)
        self.assertEqual(srv._pid_visible(anfitrion), 0x0110000100005678)


class TestMatchAnalyze(unittest.IsolatedAsyncioTestCase):
    async def test_respuesta_con_las_tres_claves(self):
        srv = Servidor(CFG, Datos())
        for cmd in ("c_cmd_get_match_analyze", "c_cmd_get_match_analyze_byid"):
            r = await getattr(srv, cmd)(None, {"analyze_data_type": 0, "target_pid": 4660})
            self.assertEqual(r, {"analyze_data_size": 0, "analyze_data_list": [], "game_res": "DRAW"})


class TestDivisiones(unittest.IsolatedAsyncioTestCase):
    """ESPEC_DIVISIONES parte A: SP, Elo, temporada y ascenso; resultado aplicado una sola vez."""

    def test_reglas_puras(self):
        from phoenix_online import divisiones as d
        r = d.Reglas()
        self.assertEqual(d.elo(1000, 1000, "WIN"), 1016)
        self.assertEqual(d.elo(1000, 1000, "LOSS"), 984)
        self.assertEqual(d.elo(1000, 1000, "DRAW"), 1000)
        e = d.estado_nuevo()
        res, sp, viejo = d.aplicar(e, r, 2, 1, 100, 200, 1000, "RIVAL", "AWAY", "p1")
        self.assertEqual((res, sp, viejo, e["rating"], e["sp"], e["jugados"]), ("WIN", 300, 1000, 1016, 300, 1))
        res, sp, _ = d.aplicar(e, r, 1, 1, 100, 200, 1000, "RIVAL", "HOME", "p2")
        self.assertEqual((res, sp), ("DRAW", 99))  # 300 x 0,33
        info = d.info_division(e, r)
        self.assertEqual((info["division"], info["promotion_point"], info["other_match_num"]), ("D12", 300, 3))
        self.assertEqual(info["match_history_list"][0]["home_score"], 2)
        self.assertEqual(info["match_history_list"][1]["enemy_side"], "HOME")
        self.assertEqual(info["match_history_list"][1]["away_score"], 1)
        # temporada completa: 399 SP >= 300 -> asciende a D11
        e["jugados"] = 10
        self.assertEqual(d.info_division(e, r)["other_match_num"], 0)
        self.assertEqual(d.cerrar_temporada(e, r), "UP")
        self.assertEqual((e["division"], e["sp"], e["jugados"], e["rating"]), ("D11", 0, 0, 1015))  # el empate contra 1000 restó 1
        filas = d.filas_carrera(e)
        self.assertEqual(len(filas), 12)
        self.assertEqual((filas[0]["division_type"], filas[0]["point"]), ("D12", 399))
        # D11 con pocos SP al final -> desciende
        e["jugados"], e["sp"] = 10, 100
        self.assertEqual(d.cerrar_temporada(e, r), "DOWN")
        self.assertEqual(e["division"], "D12")
        # D1 no asciende
        self.assertEqual(d.info_division({**d.estado_nuevo(), "division": "D1"}, r)["promotion_point"], 99999)

    async def test_partido_completo_por_el_servidor(self):
        from phoenix_online.logica import Sesion
        srv = Servidor(CFG, Datos())
        enviados = {}

        def ses(ip, clave, nombre):
            x = Sesion(lambda seq, msg, k=clave: enviados.setdefault(k, []).append(msg), (ip, 1), "MENU")
            x.steam_hex, x.pname, x.pid = clave, nombre, 1
            srv.datos.crear_jugador(clave, nombre)
            srv.sesiones.add(x)
            return x
        a = ses("100.64.0.1", "0110000100000001", "ANA")
        b = ses("100.64.0.2", "0110000100000002", "BETO")
        pareja = {"miembros": (a, b), "check": set(), "tipo": "OC_RANK", "room_id": 700000001,
                  "claves": [a.steam_hex, b.steam_hex]}
        srv.parejas[a] = srv.parejas[b] = pareja
        informe = {"msgid": "CMD_SEND_GAME_RESULT", "side": "HOME", "home_score": 3, "away_score": 1,
                   "home_team_id": 101, "away_team_id": 202, "abnormalend_reason": "NOERR"}
        await srv.c_cmd_send_game_result(a, informe)
        await srv.c_cmd_send_game_result(b, {**informe, "side": "AWAY"})  # el segundo no se aplica otra vez
        # R (pesdivisions/pesprofile AzrakHQ): equipos más usados, últimos equipos y perfil de alguien desconectado
        usados = await srv.c_cmd_get_often_used_team_list(b, {})
        self.assertEqual((usados["used_team_id_list"], usados["last_used_team_id"]), ([202], 202))
        srv.sesiones.discard(b)
        from phoenix_online import msgpack_min as mm
        uid = mm.packb({"pid": srv.numero_de_clave(b.steam_hex) & 0xFFFFFFFF, "pname": mm.Empaquetado({"name": "BETO"})})
        perfil = await srv.c_cmd_get_playerinfo(a, {"user_id": uid})
        self.assertEqual((perfil["pname"].valor["name"], perfil["rankmatch_teamLog"]), ("BETO", [202]))
        self.assertEqual(perfil["manner"].valor, {"manner": "A", "disconnect_rate": "LOW_RATE"})
        mm.packb(perfil)
        srv.sesiones.add(b)
        ea, eb = srv.datos.division(a.steam_hex), srv.datos.division(b.steam_hex)
        self.assertEqual((ea["win"], ea["sp"], ea["rating"], ea["jugados"]), (1, 300, 1016, 1))
        self.assertEqual((eb["loss"], eb["sp"], eb["rating"], eb["jugados"]), (0 + 1, 0, 984, 1))
        empujes = [m for m in enviados.get(a.steam_hex, []) if m["msgid"] == "CMD_WATCH_GAMEADDPOINT"]
        self.assertEqual(len(empujes), 1)
        self.assertEqual((empujes[0]["winner_side"], empujes[0]["player_count"]), ("WINNER_HOME", 2))
        info = await srv.c_cmd_get_division_info(a, {})
        self.assertIn("division_info", info)
        rk = await srv.c_cmd_get_ranking(a, {"start_index": 0, "view_num": 10})
        self.assertEqual(rk["rank_list"][0]["user"]["match_record"]["rating"], 1016)
        # fin anormal 0-0 en otra partida: no cuenta
        pareja2 = {**pareja, "room_id": 700000002}
        srv.parejas[a] = srv.parejas[b] = pareja2
        await srv.c_cmd_send_game_result(a, {**informe, "home_score": 0, "away_score": 0,
                                             "abnormalend_reason": "LOAD_TIMEOUT"})
        self.assertEqual(srv.datos.division(a.steam_hex)["jugados"], 1)
        # partida rápida: no toca Divisiones
        pareja3 = {**pareja, "room_id": 700000003, "tipo": "OC_QUICK"}
        srv.parejas[a] = srv.parejas[b] = pareja3
        await srv.c_cmd_send_game_result(a, informe)
        self.assertEqual(srv.datos.division(a.steam_hex)["jugados"], 1)
        # abandono con el partido en juego: el que sale pierde 3-0
        class ReleFalso:
            def __init__(s2): s2.jugando = set()
            def en_juego(s2, room): return room in s2.jugando
            def miembro_salio(s2, *a): pass
        srv.rele = ReleFalso()
        pareja4 = {**pareja, "room_id": 700000004}
        srv.parejas[a] = srv.parejas[b] = pareja4
        await srv.c_cmd_leave_game(b, {})  # aún no se jugaba: no cuenta
        self.assertEqual(srv.datos.division(b.steam_hex)["jugados"], 1)
        pareja5 = {**pareja, "room_id": 700000005}
        srv.parejas[a] = srv.parejas[b] = pareja5
        srv.rele.jugando.add(700000005)
        await srv.c_cmd_leave_game(b, {})  # BETO (visitante) se va en pleno partido
        eb2 = srv.datos.division(b.steam_hex)
        self.assertEqual((eb2["jugados"], eb2["loss"]), (2, 2))
        self.assertEqual(srv.datos.partido_division("div700000005")["home"]["score"], 3)
        srv.rele = None
        # historial de partidos: encendido, bien formado y siempre dentro de un marco
        r = await srv.c_cmd_get_gameresults(a, {})
        self.assertEqual(r["rlist_num"], 2)  # el partido normal y el abandono (el más nuevo primero)
        from phoenix_online import msgpack_min
        fila = r["rlist"][-1].valor
        self.assertEqual(fila["team"], [101, 202])  # enteros planos, no bin
        self.assertIsInstance(fila["score"][0], msgpack_min.Empaquetado)
        self.assertEqual(fila["player"][0].valor["cpu_level"], "0")
        for i in range(30):
            pz = {**pareja, "room_id": 700000100 + i}
            srv.parejas[a] = srv.parejas[b] = pz
            await srv.c_cmd_send_game_result(a, informe)
        r = await srv.c_cmd_get_gameresults(a, {"msgid": "CMD_GET_GAMERESULTS", "rqid": 9})
        self.assertGreater(r["rlist_num"], 0)
        from phoenix_online.logica import _cifrado_comprimido
        self.assertLessEqual(_cifrado_comprimido({"msgid": "CMD_GET_GAMERESULTS", "rqid": 9, "result": "NOERR", **r}), 0x3D8)
        self.assertGreaterEqual(r["rlist_num"], 5)  # comprimido caben varios
        srv.cfg = {**srv.cfg, "divisiones": {"historial_partidos": False}}
        self.assertEqual((await srv.c_cmd_get_gameresults(a, {}))["rlist_num"], 0)


class TestMyClubExtra(unittest.IsolatedAsyncioTestCase):
    """ESPEC parte B: nombre del club y de la plantilla, historial, regalos, logros (vacíos válidos)."""

    async def test_nombres_y_listas(self):
        from phoenix_online.logica import Sesion
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        x = Sesion(lambda *a: None, ("127.0.0.1", 1), "MENU")
        x.steam_hex = "aa"
        srv.datos.guardar_club("aa", {"team_name": "Viejo", "team_name_short": "VIE"})
        r = await srv.c_cmd_set_myclub_team_name(x, {"team_name": " Phoenix FC ", "team_name_short": "PHX",
                                                     "is_check_team_name": "YES", "team_name_commentary_id": 7})
        self.assertEqual(r, {})
        club = srv.datos.club("aa")
        self.assertEqual((club["team_name"], club["team_name_short"], club["team_name_commentary_id"]),
                         ("Phoenix FC", "PHX", 7))
        await srv.c_cmd_set_myclub_team_name(x, {"team_name": ""})  # vacío: no borra el nombre
        self.assertEqual(srv.datos.club("aa")["team_name"], "Phoenix FC")
        await srv.c_cmd_set_myclub_squad_name(x, {"squad_id": 1, "name": "Titulares"})
        self.assertEqual(srv.datos.club("aa")["nombres_plantilla"], {"1": "Titulares"})
        h = await srv.c_cmd_get_myclub_historylist(x, {"start_index": 5})
        self.assertEqual((h["index"], h["myclub_history_list_num"], h["myclub_history_list"]), (5, 0, []))
        self.assertEqual((await srv.c_cmd_get_myclub_presentlist(x, {}))["present_list"], [])
        self.assertEqual((await srv.c_cmd_get_myclub_achievementlist(x, {}))["achievement_list_num"], 0)
        srv.datos.guardar_club("aa", {**srv.datos.club("aa"), "vscom": {"partidos": 3, "ganados": 2, "perdidos": 1, "racha_max": 2}})
        st = (await srv.c_cmd_get_myclub_com_match_info(x, {}))["match_stats"].valor
        self.assertEqual((st["match_num"], st["win_num"], st["lose_num"], st["draw_num"], st["contWinMax_num"]), (3, 2, 1, 0, 2))
        self.assertEqual(await srv.c_cmd_get_myclub_cloud_squadid(x, {}), {"squad_id": 0})
        await srv.c_cmd_entry_myclub_weekly_ranking(x, {"is_entry": "YES"})
        self.assertTrue(srv.datos.club("aa")["semanal_inscrito"])


class TestRankingsHttp(unittest.IsolatedAsyncioTestCase):
    """B.0/B.12: rankings de myClub y Divisiones por HTTP (urltype con nombre) + URLs en la lista."""

    async def test_urls_y_respuestas(self):
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        tipos = {e.valor["type"] for e in (await srv.c_cmd_get_urllist(None, {}))["ulist"]}
        for t in ("MYCLUB_RANKING", "MYCLUB_RANKING_BYID", "MYCLUB_WEEKLY_RANKING",
                  "MYCLUB_WEEKLY_RANKING_BYID", "RECV_MYCLUB_PRESENT", "DIVISION_RANK", "DIVISION_RANK_BYID"):
            self.assertIn(t, tipos)
        for clave, nombre, g in (("0110000100000001", "ANA", 3), ("0110000100000002", "BETO", 5)):
            srv.datos.crear_jugador(clave, nombre)
            srv.datos.guardar_club(clave, {"team_name": f"{nombre} FC", "base_team_id": 9,
                                           "vscom": {"ganados": g, "partidos": g}})
        r = srv.respuesta_http({"msgid": "CMD_GET_MYCLUB_RANKING", "rqid": "4", "urltype": "MYCLUB_RANKING",
                                "pid": "1", "start_index": "0", "view_num": "50"})
        self.assertEqual((r["result"], r["rqid"], r["total_rank_num"]), ("NOERR", 4, 2))
        self.assertEqual([f["user"]["team_name"] for f in r["rank_list"]], ["BETO FC", "ANA FC"])
        self.assertEqual(r["rank_list"][0]["rank"], 1)
        self.assertNotIn("point", r["rank_list"][0])  # L: el ranking de club no lleva point
        r = srv.respuesta_http({"msgid": "CMD_GET_MYCLUB_RANKING_BYID", "rqid": "5", "pid": "1",
                                "xuid_one": str(0x0110000100000001)})
        self.assertEqual([f["user"]["team_name"] for f in r["rank_list"]], ["ANA FC"])
        self.assertEqual(r["rank_list"][0]["rank"], 2)
        r = srv.respuesta_http({"msgid": "CMD_GET_MYCLUB_WEEKLY_RANKING", "rqid": "6"})
        self.assertEqual((r["rank_list"], r["reward_list"]), ([], []))
        r = srv.respuesta_http({"msgid": "CMD_RECV_MYCLUB_PRESENT", "rqid": "7", "present_id": "1"})
        self.assertEqual(r["receive_result_list_num"], 0)
        r = srv.respuesta_http({"msgid": "CMD_GET_DIVISION_RANKING", "rqid": "8", "division": "D12"})
        self.assertEqual(r["total_rank_num"], 0)  # nadie jugó Divisiones todavía
        r = srv.respuesta_http({"msgid": "CMD_GET_DIVISION_RANKING", "rqid": "8", "division": "D12",
                                "pid": str(0x0110000100000001)})
        self.assertEqual(r["total_rank_num"], 1)  # quien pregunta siempre se ve
        srv.datos.guardar_division("0110000100000002", {"jugados": 3})
        srv.datos.guardar_division("0110000100000009", {"jugados": 1})  # cuenta sin jugador: no sale
        r = srv.respuesta_http({"msgid": "CMD_GET_DIVISION_RANKING", "rqid": "8", "division": "D12"})
        self.assertEqual([f["user"]["pname"]["name"] for f in r["rank_list"]], ["BETO"])
        srv.datos.guardar_club("0110000100000002", {"vscom": {}})
        r = srv.respuesta_http({"msgid": "CMD_GET_MYCLUB_RANKING", "rqid": "9"})
        self.assertIn("BETO FC", [f["user"]["team_name"] for f in r["rank_list"]])  # sin nombre: «<jugador> FC»


class TestNubeSinLlaveMaestra(unittest.IsolatedAsyncioTestCase):
    """Migración 164: el servidor solo llama a phoenix_srv_* con su secreto (sin service_role)."""

    async def test_vuelta_por_funciones(self):
        import io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        srv.datos.guardar_club("aa", {"gp": 100, "team_name": "Phoenix FC"})
        llamadas = []
        respuestas = {"phoenix_srv_reglas": [{"reglas": {"gp_por_partido": 7}, "actualizado": "t1"}],
                      "phoenix_srv_movimientos": [{"id": 5, "clave_servidor": "aa", "tipo": "gp", "cantidad": 50, "motivo": ""},
                                                  {"id": 6, "clave_servidor": "aa", "tipo": "gp", "cantidad": 99, "motivo": ""}],
                      "phoenix_srv_marcar_movimiento": None}

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            ruta = req.full_url.split("/rest/v1/")[1]
            self.assertTrue(ruta.startswith("rpc/"))  # nunca toca tablas directamente
            self.assertNotIn("Authorization", req.headers)
            cuerpo = _j.loads(req.data)
            self.assertEqual(cuerpo["p_secreto"], "s" * 40)
            f = ruta[4:]
            llamadas.append((f, cuerpo))
            if f == "phoenix_srv_marcar_movimiento":
                return Resp(_j.dumps(cuerpo["p_id"] == 5).encode())  # el 6 ya estaba aplicado
            return Resp(_j.dumps(respuestas.get(f, [])).encode())

        n = Nube(srv, "https://x.supabase.co", "sb_publishable_x", abrir=abrir, secreto="s" * 40)
        await n.vuelta()
        nombres = [f for f, _ in llamadas]
        self.assertIn("phoenix_srv_subir_clubes", nombres)
        self.assertEqual(srv.cfg["myclub"]["gp_por_partido"], 7)
        self.assertEqual(srv.datos.club("aa")["gp"], 150)  # solo el 5; el 6 no se da dos veces

    async def test_primera_vuelta_sube_todos_los_clubes(self):
        import io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        srv.datos.guardar_club("aa", {"gp": 1})
        srv.datos.guardar_club("bb", {"gp": 2})
        srv.datos.tomar_cambiados()  # como tras reiniciar: nada marcado como cambiado
        subidas = []

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            cuerpo = _j.loads(req.data)
            if req.full_url.endswith("phoenix_srv_subir_clubes"):
                subidas.append(sorted(f["clave_servidor"] for f in cuerpo["p_filas"]))
            return Resp(b"[]")

        n = Nube(srv, "https://x.supabase.co", "sb_publishable_x", abrir=abrir, secreto="s" * 40)
        await n.vuelta()
        await n.vuelta()
        self.assertEqual(subidas, [["aa", "bb"]])  # la primera sube todo; la segunda nada (sin cambios)

    async def test_divisiones_a_la_web(self):
        import io, json as _j
        from phoenix_online.nube import Nube
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        srv.datos.crear_jugador("aa", "ANA")
        srv.datos.guardar_division("aa", {"division": "D11", "rating": 1040, "win": 2,
                                          "archivadas": [{"win": 7, "draw": 1, "loss": 2, "jugados": 10}], "jugados": 2})
        srv.datos.div_cambiadas.clear()
        subidas = []

        class Resp(io.BytesIO):
            def __enter__(s): return s
            def __exit__(s, *a): return False

        def abrir(req, timeout=None):
            if req.full_url.endswith("phoenix_srv_subir_divisiones"):
                subidas.append(_j.loads(req.data)["p_filas"])
            return Resp(b"[]")

        n = Nube(srv, "https://x.supabase.co", "sb_publishable_x", abrir=abrir, secreto="s" * 40)
        await n.vuelta()
        await n.vuelta()
        self.assertEqual(len(subidas), 1)
        f = subidas[0][0]
        self.assertEqual((f["division"], f["rating"], f["victorias"], f["partidos_total"], f["nombre_jugador"]),
                         ("D11", 1040, 9, 12, "ANA"))

    def test_desde_config_prefiere_sin_llave_maestra(self):
        import os
        from phoenix_online.nube import desde_config
        srv = Servidor({**CFG, "myclub": {}}, Datos())
        cfg = {"supabase": {"activo": True, "url": "https://x.supabase.co", "clave_publica": "sb_publishable_x"}}
        viejo = {k: os.environ.pop(k, None) for k in ("PHOENIX_SERVIDOR_SECRETO", "PHOENIX_SUPABASE_KEY")}
        try:
            self.assertIsNone(desde_config(srv, cfg))
            os.environ["PHOENIX_SUPABASE_KEY"] = "maestra"
            os.environ["PHOENIX_SERVIDOR_SECRETO"] = "s" * 40
            n = desde_config(srv, cfg)
            self.assertEqual((n.clave, n.secreto), ("sb_publishable_x", "s" * 40))
        finally:
            for k, v in viejo.items():
                os.environ.pop(k, None)
                if v is not None:
                    os.environ[k] = v
