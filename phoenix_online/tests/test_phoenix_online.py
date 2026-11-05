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
        self.assertEqual((seq, m["rqid"], m["result"], m["url_num"]), (42, 9, "NOERR", 20))
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
        a, ea = await self.entrar("A1")
        b, eb = await self.entrar("B2")
        await self.srv.recibir(a, 5, {"msgid": "CMD_CREATEJOIN_ROOM", "rqid": 6, "name": "X"})
        # L (ESPEC_SALAS §2): primero la respuesta con room_id, luego el RELOAD de salas
        self.assertEqual([m["msgid"] for _, m in ea[-2:]], ["CMD_CREATEJOIN_ROOM", "CMD_REQWATCHL_ROOMLIST"])
        room = ea[-2][1]["room_id"]
        await self.srv.recibir(a, 6, {"msgid": "CMD_SET_GAMEENV", "rqid": 7, "gametime": 10})
        vistas = [m for _, m in eb if m.get("msgid") == "CMD_REQWATCHL_ROOMLIST" and m["list_num"]]
        self.assertTrue(vistas and vistas[-1]["list_type"] == "RELOAD")  # B vio la sala nueva
        fila = msgpack_min.unpackb(msgpack_min.packb(vistas[-1]["list_data"][0].valor))
        self.assertEqual((fila["vs_com"], fila["is_passwd"], fila["game_phase"], fila["is_room_owner"]),
                         ("PL_VS_PL", "NO", "DATASYNC", "NO"))
        self.assertIsInstance(fila["owner_pname"], bytes)
        self.assertIsInstance(fila["gameenv"], bytes)
        eb.clear()
        await self.srv.recibir(b, 9, {"msgid": "CMD_JOIN_ROOM", "rqid": 8, "room_id": room})
        listas = [m for _, m in eb if m["msgid"] == "CMD_REQWATCHL_ROOMPLAYERLIST_NTL"]
        self.assertEqual([m["list_type"] for m in listas], ["UPDATE", "ADD"])
        self.assertEqual(listas[-1]["list_num"], 2)  # sin esto: UQOW_0006
        self.assertTrue(all(seq == 9 for seq, _ in eb))  # secuencia actual de la sesión
        self.assertEqual(eb[-1][1]["msgid"], "CMD_WATCH_ENTRY_GAME")
        await self.srv.recibir(a, 7, {"msgid": "CMD_ENTRY_GAME", "rqid": 10, "entry": "TRUE"})
        self.assertEqual(ea[-1][1]["entryflag"], "TRUE")
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
        self.assertEqual((fila["is_owner"], fila["room_entry_no"], fila["side"]), ("YES", 1, ""))
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
        self.assertEqual(msgpack_min.unpackb(msgpack_min.packb(m["manner"].valor))["manner"], "AA")

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
        self.assertEqual((j[0]["contract_term"], j[0]["tiredness"]), (98, 50))  # tope 50
        self.assertNotIn("contract_term", j[19])  # no inscrito: no gasta contrato
        self.assertEqual(j[19]["tiredness"], 0)  # descansa 10 por partido, sin bajar de 0

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
        antes = int(club["jugadores"][0].get("contract_term") or 0)
        await r(a, 9, {"msgid": "CMD_EXTEND_MYCLUB_GAMEPLAYER", "rqid": 9,
                       "gameplayer_unique_id_list": [str(uid)], "use_pes_point": "NO"})
        self.assertEqual(ea[-1][1]["gp"], 2200)
        fila = msgpack_min.unpackb(msgpack_min.packb(ea[-1][1]["contract_term_list"][0].valor))
        self.assertEqual(fila["contract_term"], antes + 10)
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
