"""Lógica del servidor: sesiones, login, lobby, salas del amistoso 1v1 y TURN.

Todo sale de cap_72 §11 (especificación v1, sala limpia). Lo que la
especificación no fija con certeza está marcado «P» y concentrado en
constantes para corregirlo con REA sin tocar el resto.
"""
import asyncio
import base64
import hashlib
import hmac
import itertools
import logging
import time

from .msgpack_min import Empaquetado

log = logging.getLogger("phoenix_online")

NOERR = "NOERR"

# Comandos que solo esperan msgid/rqid/result (cap_72 §11.4)
CONFIRMACIONES = {
    "CMD_SEND_PSID", "CMD_SET_CURRENT_PLAYER", "CMD_LOGIN_MENU", "CMD_SET_LANGUAGE",
    "CMD_JOIN_ONLINE_MODE", "CMD_LEAVE_ONLINE_MODE", "CMD_JOIN_OFFLINE_MODE",
    "CMD_LEAVE_OFFLINE_MODE", "CMD_SEND_HEARTBEAT", "CMD_SET_ROOM_OPTION",
}

# Vigilancias que el cliente deja abiertas y el servidor completa más tarde
VIGILANCIAS = {
    "CMD_REQWATCHL_ROOMLIST", "CMD_REQWATCHL_ROOMPLAYERLIST_NTL", "CMD_WATCH_ROOMSTATE",
    "CMD_WATCH_ENTRY_GAME", "CMD_WATCH_DECIDE_GAMEENV", "CMD_WATCH_DECIDE_GAMEENV_FREE",
    "CMD_WATCH_NOTICE", "CMD_WATCH_TURN_ADDRESS_DATA",
}

PAUSA_EMPUJE = 0.25  # cientos de ms entre empujes encadenados (§11.5.6)


class Sesion:
    """Una conexión TCP de un cliente PES."""

    def __init__(self, enviar_trama, direccion, svrtype):
        self._enviar_trama = enviar_trama  # callable(secuencia, mensaje)
        self.direccion = direccion
        self.svrtype = svrtype
        self.secuencia = 0  # última recibida: se usa al responder Y al empujar (§11.2.7)
        self.steam_hex = None
        self.pid = None
        self.pname = None
        self.vigilancias = {}  # msgid -> rqid
        self.room_id = None
        self.ntl = {}
        self.huella = None
        self.versiones = {}
        self.entry = False
        self.lang = None
        self.cerrar_tras_responder = False

    def enviar(self, mensaje):
        self._enviar_trama(self.secuencia, mensaje)

    def completar(self, msgid, **campos):
        """Empuja un mensaje usando el rqid de una vigilancia abierta. False si no hay."""
        rqid = self.vigilancias.get(msgid)
        if rqid is None:
            return False
        self.enviar({"msgid": msgid, "rqid": rqid, "result": NOERR, **campos})
        return True


class Sala:
    ids = itertools.count(1)

    def __init__(self, dueno, nombre):
        self.room_id = next(Sala.ids)
        self.nombre = nombre
        self.dueno = dueno
        self.miembros = [dueno]
        self.estado = "WAITING"  # WAITING -> SETENV -> GAME
        self.gameenv = {}
        self.opciones = {}
        self.passwd = None
        self.max_players = 2
        self.partido = None

    def ficha(self):
        g = self.gameenv
        return {
            "room_id": self.room_id, "name": self.nombre, "status": self.estado,
            "game_phase": 0, "gameenv": g.get("gameenv", 0), "is_passwd": bool(self.passwd),
            "max_players": self.max_players, "owner_pname": self.dueno.pname or "",
            "room_user_count": len(self.miembros), "vs_com": g.get("vs_com", False),
            **{k: g[k] for k in ("gametime", "injury", "ball_type", "exGame", "pkOnOff",
                                  "substitution", "exSubstitution", "limitTime", "cpuLevel")
               if k in g},
        }

    def ficha_jugador(self, s, numero):
        return {
            "pid": s.pid, "pname": s.pname or "", "room_player_id": s.pid,
            "room_entry_no": numero, "game_entry_no": numero,
            "is_owner": s is self.dueno, "side": numero, "lang": s.lang or 0,
            "is_quality_bad": False, "has_guest": False, "guest_info": b"",
            "account_id": s.steam_hex or "", "online_id": s.pname or "", "is_valid_cache": True,
            "xuid_one": "", "psn_user_id": "", "is_deleted": False,  # nunca SteamID en campos de Xbox/PSN
            "ntl_nat_type": s.ntl.get("ntl_nat_type", 0),
            "host_addr": s.ntl.get("host_addr", b""),
            "reflexive_addr": s.ntl.get("reflexive_addr", b""),
            "relayed_addr": s.ntl.get("relayed_addr", b""),
        }


class Servidor:
    def __init__(self, config, datos):
        self.cfg = config
        self.datos = datos
        self.sesiones = set()
        self.por_pid = {}
        self.salas = {}
        self.turn_pendiente = {}  # pid destino -> (address, size)
        self.dormir = asyncio.sleep

    # ---------- ciclo de vida ----------
    def abrir(self, sesion):
        self.sesiones.add(sesion)

    async def cerrar(self, sesion):
        self.sesiones.discard(sesion)
        if sesion.pid is not None and self.por_pid.get(sesion.pid) is sesion:
            del self.por_pid[sesion.pid]
        await self._salir_de_sala(sesion)

    # ---------- despacho ----------
    async def recibir(self, sesion, secuencia, msg):
        sesion.secuencia = secuencia
        if not isinstance(msg, dict) or "msgid" not in msg:
            log.warning("mensaje sin msgid de %s: %r", sesion.direccion, msg)
            return
        msgid = msg["msgid"]
        rqid = msg.get("rqid", 0)
        if msgid in VIGILANCIAS:
            sesion.vigilancias[msgid] = rqid
            await self._al_vigilar(sesion, msgid)
            return
        manejador = getattr(self, "c_" + msgid.lower(), None)
        if manejador is not None:
            extra = await manejador(sesion, msg)
            if extra is None:
                return  # el manejador ya respondió o no responde
        elif msgid in CONFIRMACIONES:
            extra = {}
        else:
            # Desconocido: NOERR mínimo destraba los GET_*; se registra para investigarlo
            log.warning("comando sin manejador: %s campos=%s", msgid, sorted(msg))
            extra = {}
        result = extra.pop("result", NOERR)
        sesion.enviar({"msgid": msgid, "rqid": rqid, "result": result, **extra})

    # ---------- ConnectGate (§11.3) ----------
    async def c_cmd_get_urllist(self, s, m):
        urls = self.cfg["urls"]  # {type: url}; debe incluir TOPINFO, BANNER, GET_TURN_SERVER_LIST...
        return {"url_num": len(urls),
                "ulist": [Empaquetado({"type": t, "url": u}) for t, u in urls.items()]}

    async def c_cmd_get_svrversion(self, s, m):
        return dict(self.cfg["versiones"])

    async def c_cmd_get_svrtime(self, s, m):
        return {"date": int(time.time())}  # C: entero; P: segundos Unix (el exe suma 9 h JST)

    @staticmethod
    def _texto(v):
        # C (REA 29-oct, fns 141bd88e0/141bcdc60): el cliente lee estos campos como CADENA;
        # un entero o bool rompe la elección de país (error 4 tras confirmar)
        if isinstance(v, bool):
            return "1" if v else "0"
        return "" if v is None else str(v)

    async def c_cmd_get_eula_nation_list(self, s, m):
        naciones = [{k: self._texto(v) for k, v in n.items()} for n in self.cfg["naciones"]]
        return {"nation_list": [Empaquetado(n) for n in naciones],
                "nation_list_num": len(naciones)}

    async def c_cmd_get_eula_nation_code(self, s, m):
        return {"nation_code": self._texto(self.cfg["nacion"]),
                "area_code": self._texto(self.cfg.get("area", ""))}

    async def c_cmd_get_svrlist(self, s, m):
        ip = self.cfg["ip_publica"]
        # C (REA 29-oct): ConnectAccount cuenta los servidores «ACCOUNT» de esta lista y, si no
        # hay ninguno, da error fatal sin abrir red. ACCOUNT y MENU pueden ser el mismo puerto.
        menu = self.cfg["puerto_menu"]
        lista = [
            {"svrtype": t, "svraddr": ip, "svrport": self.cfg.get(f"puerto_{t.lower()}", menu),
             "svrgid": 1, "player_num": len(self.por_pid), "max_player_num": 1000}
            for t in ("ACCOUNT", "MENU")
        ]
        return {"server_num": len(lista), "svrlist": [Empaquetado(x) for x in lista]}  # P: bin

    async def c_cmd_get_globalinfotitlelist(self, s, m):
        avisos = self.cfg.get("anuncios", [])
        return {"announce_info_list": [Empaquetado(a) for a in avisos],
                "annnounce_info_list_num": len(avisos)}  # tres «n», así lo lee el cliente

    async def c_cmd_get_privateinfotitlelist(self, s, m):
        return {"announce_info_list": [], "annnounce_info_list_num": 0}

    async def c_cmd_get_infobody(self, s, m):
        return {}  # P: campos del cuerpo de anuncio

    async def c_cmd_get_server_env(self, s, m):
        return {"proto_opt": 1 if self.cfg.get("turn") else 0}

    async def c_cmd_disconnect(self, s, m):
        # Tras CMD_GET_SVRLIST el cliente manda CMD_DISCONNECT {logoutstat: "MOV"} y salta al
        # servidor MENU de la lista (P: informe público de PESBUL; confirmar en la captura)
        log.info("CMD_DISCONNECT logoutstat=%s (%s)", m.get("logoutstat"), s.svrtype)
        s.cerrar_tras_responder = True
        return {}

    # ---------- Inicio de sesión (§11.4 + REA 29-oct, guía §14) ----------
    # Orden real del cliente (OnlineLoginTaskConnectAccount): CMD_AUTH_STEAM ->
    # CMD_GET_CHALLENGECODE -> CMD_LOGIN_ACCOUNT -> CMD_SEND_PSID -> CMD_GET_PLAYER_DATA.
    # Los campos de la petición de AUTH_STEAM aún no se conocen (P): se registran todos.
    CAMPOS_STEAM = ("np_account_id", "username", "steam_id", "steamid", "account_id")

    @classmethod
    def _steam_de(cls, m):
        """SteamID del mensaje en hex de 16 cifras, o None si no viene."""
        for k in cls.CAMPOS_STEAM:
            v = m.get(k)
            if isinstance(v, int) and v > 0:
                return format(v, "016x")
            if isinstance(v, bytes) and len(v) == 8:
                return v.hex()
            if isinstance(v, str) and v.isdigit() and int(v) > 0:
                return format(int(v), "016x")  # P: np_account_id en decimal como texto
            if isinstance(v, str) and v:
                return v.lower()
        return None

    def _entrar(self, s, m):
        steam_hex = self._steam_de(m) or s.steam_hex or ""
        if not self.datos.autorizado(steam_hex):
            log.info("login rechazado (sin cuenta Phoenix vinculada): %s", steam_hex)
            return {"result": "ERR_LOGIN_FAILED"}  # C: texto de error del exe
        s.steam_hex = steam_hex
        s.versiones = {k: m.get(k) for k in ("patch_version", "dlc_version",
                                              "livedata_version", "extradata_version")}
        return {}

    async def c_cmd_auth_steam(self, s, m):
        # El ticket es un EncryptedAppTicket (P): solo lo abre la clave de Konami, así que no
        # sirve para verificar. La identidad real la garantiza el vínculo de Phoenix Link.
        log.info("CMD_AUTH_STEAM campos=%s", sorted(m))
        s.steam_hex = self._steam_de(m) or s.steam_hex
        return {}

    async def c_cmd_get_challengecode(self, s, m):
        return {"challenge_code": base64.b16encode(hashlib.sha256(
            f"{id(s)}{time.time()}".encode()).digest()[:8]).decode()}

    async def c_cmd_login_account(self, s, m):
        log.info("CMD_LOGIN_ACCOUNT campos=%s", sorted(m))
        return self._entrar(s, m)

    async def c_cmd_login(self, s, m):
        return self._entrar(s, m)

    async def c_cmd_get_player_data(self, s, m):
        # C (REA 29-oct, lector 141d3f450): has_data, is_valid_cache e is_deleted son CADENA;
        # account_id es NÚMERO; pid entero. P: el texto exacto de has_data ("1"/"0").
        j = self.datos.jugador(s.steam_hex or "")
        if not j:
            return {"has_data": "0"}
        self._registrar(s, j)
        return {"has_data": "1", "pid": j["pid"],
                "pname": Empaquetado(self._identidad(s, j["pname"])),
                "account_id": self._numero_cuenta(s)}

    @staticmethod
    def _numero_cuenta(s):
        try:
            return int(s.steam_hex or "0", 16)
        except ValueError:
            return 0

    @classmethod
    def _identidad(cls, s, nombre):
        # nunca SteamID en campos de Xbox/PSN: xuid_one y psn_user_id vacíos
        return {"name": nombre, "xuid_one": "", "psn_user_id": "", "account_id": cls._numero_cuenta(s),
                "online_id": nombre, "is_valid_cache": "1", "is_deleted": "0"}

    async def c_cmd_create_player(self, s, m):
        nombre = m.get("pname") or m.get("name") or f"Phoenix_{(s.steam_hex or '')[-4:]}"
        if isinstance(nombre, (bytes, dict)):
            nombre = f"Phoenix_{(s.steam_hex or '')[-4:]}"
        j = self.datos.crear_jugador(s.steam_hex or "", nombre)
        self._registrar(s, j)
        return {"pid": j["pid"]}

    def _registrar(self, s, j):
        s.pid, s.pname = j["pid"], j["pname"]
        viejo = self.por_pid.get(s.pid)
        if viejo is not None and viejo is not s and viejo.room_id in self.salas:
            # Reconexión: la sesión nueva hereda el puesto en la sala; la vieja ya no
            # puede sacarla al cerrarse tarde (fallo «session cleanup» que sufrió PESBUL)
            sala = self.salas[viejo.room_id]
            sala.miembros = [s if x is viejo else x for x in sala.miembros]
            if sala.dueno is viejo:
                sala.dueno = s
            s.room_id, viejo.room_id = viejo.room_id, None
        self.por_pid[s.pid] = s

    async def c_cmd_set_language(self, s, m):
        s.lang = m.get("lang", s.lang)
        return {}

    async def c_cmd_send_ipandport_ntl(self, s, m):
        for k in ("ntl_nat_type", "host_addr", "reflexive_addr", "relayed_addr"):
            if k in m:
                s.ntl[k] = m[k]
        hashes = m.get("file_hash_list")
        if hashes is not None:
            crudo = repr(hashes).encode()
            s.huella = (len(hashes) if hasattr(hashes, "__len__") else 0,
                        hashlib.sha256(crudo).hexdigest())
        return {}

    # ---------- Lobby (§11.5 pasos 1-2) ----------
    async def c_cmd_get_lobbylist(self, s, m):
        tipo = m.get("lobby_type", "FREE")
        lobbies = [l for l in self.cfg["lobbies"] if l["lobby_type"] == tipo]
        return {"lobby_list_num": len(lobbies),
                "lobby_list": [Empaquetado({**l, "player_num": len(self.por_pid)}) for l in lobbies]}

    async def c_cmd_get_lobbyinfo(self, s, m):
        return {"has_info": True, "title": "Phoenix Online",
                "sentence_info": self.cfg.get("lobby_info", ""), "sentence_intro": ""}

    async def c_cmd_join_lobby(self, s, m):
        return {"gamerelay_web_url": self.cfg["urls"].get("GAMERELAY_WEB", ""), "bklist": []}

    # ---------- Vigilancias ----------
    async def _al_vigilar(self, s, msgid):
        if msgid == "CMD_REQWATCHL_ROOMLIST":
            self._empujar_salas(s, "RELOAD", list(self.salas.values()))
        elif msgid == "CMD_WATCH_TURN_ADDRESS_DATA" and s.pid in self.turn_pendiente:
            addr, size = self.turn_pendiente.pop(s.pid)
            s.completar(msgid, address=addr, address_data_size=size)

    def _empujar_salas(self, s, accion, salas):
        s.completar("CMD_REQWATCHL_ROOMLIST", list_type=accion, list_num=len(salas),
                    list_key_name="room_id", list_data=[Empaquetado(x.ficha()) for x in salas])

    def _empujar_salas_a_todos(self, accion, salas):
        for otro in list(self.sesiones):
            self._empujar_salas(otro, accion, salas)

    def _empujar_jugadores(self, s, sala, accion, miembros):
        datos = [Empaquetado(sala.ficha_jugador(x, i)) for i, x in enumerate(miembros)]
        s.completar("CMD_REQWATCHL_ROOMPLAYERLIST_NTL", list_type=accion, list_num=len(datos),
                    list_key_name="pid", list_data=datos)

    # ---------- Salas (§11.5 pasos 4-8) ----------
    async def c_cmd_createjoin_room(self, s, m):
        await self._salir_de_sala(s)
        sala = Sala(s, m.get("name") or f"Sala de {s.pname}")
        sala.passwd = m.get("passwd") or None
        self.salas[sala.room_id] = sala
        s.room_id, s.entry = sala.room_id, False
        self._empujar_salas_a_todos("ADD", [sala])
        return {"room_id": sala.room_id}

    async def c_cmd_set_gameenv(self, s, m):
        sala = self.salas.get(s.room_id)
        if sala and s is sala.dueno:
            sala.gameenv = {k: v for k, v in m.items() if k not in ("msgid", "rqid")}
            self._empujar_salas_a_todos("UPDATE", [sala])
        return {}

    async def c_cmd_join_room(self, s, m):
        sala = self.salas.get(m.get("room_id"))
        if sala is None:
            return {"result": "ERR_ROOMNOTFOUND"}  # C
        if len(sala.miembros) >= sala.max_players or sala.estado != "WAITING":
            return {"result": "ERR_ROOMISFULL"}  # C
        if sala.passwd and m.get("passwd") != sala.passwd:
            return {"result": "ERR_PASSWD"}  # P
        if self.cfg.get("exigir_misma_huella") and s.huella != sala.dueno.huella:
            log.info("huella distinta: %s vs %s", s.huella, sala.dueno.huella)
            return {"result": "ERR_DIFFERENT_PATCHVERSION"}  # C: evita SYNC_FAILED a mitad del partido
        await self._salir_de_sala(s)
        previos = list(sala.miembros)
        s.room_id, s.entry = sala.room_id, False
        sala.miembros.append(s)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR,
                  "room_id": sala.room_id})
        # Orden que funcionó (§11.5.6): UPDATE sin el que entra → RELOAD salas → ADD con él → ENTRY_GAME
        for x in sala.miembros:
            self._empujar_jugadores(x, sala, "UPDATE", previos)
        await self.dormir(PAUSA_EMPUJE)
        self._empujar_salas_a_todos("RELOAD", list(self.salas.values()))
        await self.dormir(PAUSA_EMPUJE)
        for x in sala.miembros:
            self._empujar_jugadores(x, sala, "ADD", sala.miembros)
        await self.dormir(PAUSA_EMPUJE)
        for x in sala.miembros:
            # C: el cliente lee entry_count y entrygame [{entryNum, gameNum}]
            x.completar("CMD_WATCH_ENTRY_GAME", entry_count=len(sala.miembros),
                        entrygame=[{"entryNum": i, "gameNum": i} for i in range(len(sala.miembros))])
        return None

    async def c_cmd_entry_game(self, s, m):
        sala = self.salas.get(s.room_id)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        if not sala:
            return None
        s.entry = bool(m.get("entry", True))
        if sala.estado == "WAITING" and len(sala.miembros) == sala.max_players \
                and all(x.entry for x in sala.miembros):
            sala.estado = "SETENV"
            for x in sala.miembros:
                x.completar("CMD_WATCH_ROOMSTATE", event="StateChanged", state="SETENV")
            await self.dormir(PAUSA_EMPUJE)
            for x in sala.miembros:
                if not x.completar("CMD_WATCH_DECIDE_GAMEENV_FREE", **sala.gameenv):
                    x.completar("CMD_WATCH_DECIDE_GAMEENV", **sala.gameenv)
            sala.estado = "GAME"
            sala.partido = f"{sala.room_id}-{int(time.time())}"
            self._empujar_salas_a_todos("UPDATE", [sala])
        return None

    async def c_cmd_send_game_result(self, s, m):
        sala = self.salas.get(s.room_id)
        if sala and sala.partido:
            datos = {k: v for k, v in m.items() if k not in ("msgid", "rqid")}
            self.datos.guardar_resultado(sala.room_id, sala.partido, s.pid, datos)
            res = self.datos.resultados(sala.partido)
            if len(res) == len(sala.miembros):
                iguales = len({repr(v.get("result", v)) for v in res.values()}) == 1
                log.info("partido %s cerrado; los dos lados %s", sala.partido,
                         "coinciden" if iguales else "NO coinciden (revisar)")
        return {}

    async def c_cmd_send_abnormalend(self, s, m):
        log.warning("fin anormal pid=%s motivo=%s", s.pid, m.get("reason", m))
        return {}

    async def c_cmd_leave_game(self, s, m):
        return {}

    async def c_cmd_leave_room(self, s, m):
        if m.get("error_code"):
            log.warning("salida con error %s pid=%s", m["error_code"], s.pid)  # MXNV_0013 = P2P falló
        await self._salir_de_sala(s)
        return {}

    async def _salir_de_sala(self, s):
        sala = self.salas.get(s.room_id) if s.room_id else None
        s.room_id, s.entry = None, False
        if not sala:
            return
        if s in sala.miembros:
            sala.miembros.remove(s)
        if not sala.miembros:
            del self.salas[sala.room_id]
            self._empujar_salas_a_todos("DEL", [sala])
            return
        if sala.dueno is s:
            sala.dueno = sala.miembros[0]
        sala.estado = "WAITING"
        for x in sala.miembros:
            x.entry = False
            self._empujar_jugadores(x, sala, "RELOAD", sala.miembros)
        self._empujar_salas_a_todos("UPDATE", [sala])

    # ---------- TURN (§11.6) ----------
    def credenciales_turn(self, pid):
        t = self.cfg["turn"]
        usuario = f"{int(time.time()) + t.get('vida_segundos', 3600)}:{pid}"
        clave = base64.b64encode(hmac.new(t["secreto"].encode(), usuario.encode(),
                                          hashlib.sha1).digest()).decode()
        return usuario, clave

    async def c_cmd_check_local_gamerelay_player(self, s, m):
        # C (REA 29-oct, fn 141b9cdc0): claves leídas por el cliente. Sin relé propio por ahora:
        # 0 relés por modo -> el partido va P2P (directo o por TURN).
        return {"is_local_gamerelay_player": False, "gamerelay_register_url_data": "[]",
                "build_gamerelay_num_for_1vs1": 0, "build_gamerelay_num_for_3vs3": 0,
                "build_gamerelay_num_for_11vs11": 0}

    def respuesta_http(self, formulario):
        """Comandos que el cliente manda por WinHTTP (base CommandObjectHttp, REA 29-oct).

        Los nombres exactos de los campos aún no se conocen (P): se registran y se
        buscan por varios nombres. Devuelve None si no es un comando.
        """
        msgid = formulario.get("msgid")
        if not msgid:
            return None
        if msgid == "CMD_GET_TURN_SERVER_LIST" and self.cfg.get("turn"):
            return self.respuesta_turn(formulario)
        base = {"msgid": msgid, "rqid": int(formulario.get("rqid", 0) or 0), "result": NOERR}
        if msgid == "CMD_AUTH_STEAM":
            # Ticket = GetAuthSessionTicket en hex + SteamID64 en claro. Verificar la firma
            # del ticket queda pendiente; hoy la identidad la garantiza el vínculo de Link.
            resumen = {k: (f"<{len(v)} car.>" if len(str(v)) > 40 else v)
                       for k, v in formulario.items()}
            log.info("HTTP CMD_AUTH_STEAM %s", resumen)
            return base
        log.warning("comando HTTP sin manejador: %s campos=%s", msgid, sorted(formulario))
        return base

    def respuesta_turn(self, formulario):
        t = self.cfg["turn"]
        pid = formulario.get("pid", "0")
        usuario, clave = self.credenciales_turn(pid)
        return {"msgid": "CMD_GET_TURN_SERVER_LIST", "rqid": int(formulario.get("rqid", 0)),
                "result": NOERR,
                "turn_server_list": [{"zone": z.get("zone", "default"), "address": z["address"],
                                      "port": z.get("port", 3479), "tls_port": z.get("tls_port", 0)}
                                     for z in t["servidores"]],
                "id": usuario, "pw": clave, "origin": t.get("origin", ""),
                "server_certificate": b"", "server_certificate_data_size": 0}

    async def c_cmd_send_turn_address_data(self, s, m):
        destino = m.get("pid")
        addr, size = m.get("address", b""), m.get("address_data_size", 0)
        otro = self.por_pid.get(destino)
        # El bloque viaja intacto; si el otro aún no vigila, se guarda
        if not (otro and otro.completar("CMD_WATCH_TURN_ADDRESS_DATA",
                                        address=addr, address_data_size=size)):
            self.turn_pendiente[destino] = (addr, size)
        return {}
