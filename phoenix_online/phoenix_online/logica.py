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
import json
import logging
import time

from . import msgpack_min
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

# P (PESBUL matchmaking_policy): opciones guardadas de «Opciones de Matchmaking»
OPCIONES_RAPIDA = {"is_pass_level": "NO", "pass_level": 1, "rating_level": "RATING_LEVEL_1",
                   "ping_level": "PING_LEVEL_3", "team_class": "TEAM_NONE", "is_league": "NO",
                   "is_team_power_lv": "NO", "is_good_reputation": "YES"}

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
        self.opciones_rapida = dict(OPCIONES_RAPIDA)

    def enviar(self, mensaje):
        self._enviar_trama(self.secuencia, mensaje)

    def completar(self, msgid, **campos):
        """Empuja un mensaje usando el rqid de una vigilancia abierta. False si no hay."""
        rqid = self.vigilancias.get(msgid)
        if rqid is None:
            return False
        self.enviar({"msgid": msgid, "rqid": rqid, "result": NOERR, **campos})
        return True


def _texto_si(v, defecto="NO"):
    """Indicadores al cable como TEXTO «YES»/«NO» (ESPEC_SALAS §0: bool o int = PES se cierra)."""
    if isinstance(v, str) and v:
        return v
    if v is None:
        return defecto
    return "YES" if v else "NO"


def _binario(o):
    """bin con msgpack dentro (también para listas: guest_info = bin([]))."""
    return msgpack_min.packb(o)


GAMEENV_SALA = {"cpuLevel": "HARDEST", "gametime": 10, "injury": "YES", "ball_type": 22,
                "exGame": "NO", "pkOnOff": "NO", "substitution": 3, "exSubstitution": "YES",
                "limitTime": "MIDDLE"}  # L (ESPEC_SALAS §1b): valores de una sala nueva


class Sala:
    ids = itertools.count(1)

    def __init__(self, dueno, nombre, opciones=None):
        self.room_id = next(Sala.ids)
        self.nombre = nombre
        self.dueno = dueno
        self.miembros = [dueno]
        self.estado = "WAITING"  # WAITING -> SETENV -> GAME
        self.gameenv = {}
        self.opciones = dict(opciones or {})
        self.passwd = self.opciones.get("password") or self.opciones.get("passwd") or None
        try:
            self.max_players = int(self.opciones.get("max_players") or 2)
        except (TypeError, ValueError):
            self.max_players = 2
        self.partido = None

    @staticmethod
    def pname(s):
        """L §1d: bin(mapa) con name, xuid_one, psn_user_id (bin) e is_deleted, todo en texto."""
        nombre = s.pname or ""
        cuenta = Servidor._numero_cuenta(s) or (s.pid or 0)
        return Empaquetado({"name": nombre, "xuid_one": str(cuenta),
                            "psn_user_id": Empaquetado({"account_id": int(cuenta), "online_id": nombre,
                                                        "is_valid_cache": "YES"}),
                            "is_deleted": "NO"})

    def ficha(self, para=None, slot=0):
        """L ESPEC_SALAS §1: fila de sala; anidados como bin, enumerados como texto."""
        o = self.opciones
        entra = self.miembros.index(para) + 1 if para in self.miembros else 0
        env = {**GAMEENV_SALA, **{k: v for k, v in self.gameenv.items() if k in GAMEENV_SALA}}
        return {
            "slot": slot, "room_id": self.room_id,
            "name": self.nombre, "room_name": self.nombre, "roomName": self.nombre,
            "status": self.estado if isinstance(self.estado, str) else "WAITING",
            "game_phase": "DATASYNC", "gameenv": Empaquetado(env),
            "gamer_num": len(self.miembros),
            "gamer": [Empaquetado({"pid": x.pid or 0, "is_room_owner": _texto_si(x is self.dueno),
                                   "side": "HOME" if i == 0 else "AWAY", "enter_no": i + 1,
                                   "game_no": 1}) for i, x in enumerate(self.miembros)],
            "is_room_owner": _texto_si(para is self.dueno), "enter_no": entra, "game_no": 1,
            "is_passwd": _texto_si(bool(self.passwd)), "max_players": self.max_players,
            "is_invite_limit": _texto_si(o.get("is_invite_limit")), "dlcontent": "7.00",
            "is_original_player": _texto_si(o.get("use_original_player"), "YES"),
            "vs_com": o.get("vs_com") if isinstance(o.get("vs_com"), str) and o.get("vs_com") else "PL_VS_PL",
            "owner_pname": self.pname(self.dueno), "owner_lang": "EN",
            "room_user_count": len(self.miembros),
            "is_anyplayers": _texto_si(o.get("is_anyplayers")),
            "is_select_gk": _texto_si(o.get("is_select_gk")),
            "auto_start_num": int(o.get("auto_start_num") or 0), "room_number": self.room_id,
            "game_rating_limit": o.get("game_rating_limit") if isinstance(o.get("game_rating_limit"), str)
            and o.get("game_rating_limit") else "NONE",
        }

    def ficha_jugador(self, s, numero):
        """L ESPEC_SALAS §3: enteros obligatorios pid/room_player_id/room_entry_no/game_entry_no;
        textos obligatorios lang/is_owner/side/is_quality_bad/has_guest; pname y guest_info bin."""
        fila = {
            "pid": s.pid or 0, "room_player_id": s.pid or 0,
            "room_entry_no": numero + 1, "game_entry_no": numero + 1,
            "pname": self.pname(s),
            "lang": s.lang if isinstance(s.lang, str) and s.lang else "US",
            "is_owner": _texto_si(s is self.dueno), "side": "",
            "is_quality_bad": "", "has_guest": "", "guest_info": _binario([]),
            "ntl_nat_type": int(s.ntl.get("ntl_nat_type", 0) or 0),
        }
        for k in ("host_addr", "reflexive_addr", "relayed_addr"):
            v = s.ntl.get(k)
            if isinstance(v, (bytes, bytearray)) and v:
                fila[k] = bytes(v)  # ya viene como bin(mapa{address_family, sa_data}) del cliente
        return fila


class Servidor:
    def __init__(self, config, datos):
        self.cfg = config
        self.datos = datos
        self.sesiones = set()
        self.por_pid = {}
        self.salas = {}
        self.turn_pendiente = {}  # pid destino -> (address, size)
        self.dormir = asyncio.sleep
        self.avatares = {}    # pid (texto) -> base64 del avatar
        self.sondas = {}      # (ip, índice de nodo) -> (pid, hasta): sondas UDP autorizadas
        self.calidades = {}   # pid -> {índice: medida} de SET_GAMERELAY_QUALITY
        self.colas = {}       # matching_type -> [Sesion] buscando rival
        self.parejas = {}     # Sesion -> Pareja de partida rápida

    # ---------- ciclo de vida ----------
    def abrir(self, sesion):
        self.sesiones.add(sesion)

    async def cerrar(self, sesion):
        self.sesiones.discard(sesion)
        if sesion.pid is not None and self.por_pid.get(sesion.pid) is sesion:
            del self.por_pid[sesion.pid]
        await self._salir_de_sala(sesion)
        self._salir_de_cola(sesion)
        self.parejas.pop(sesion, None)
        # C: la sonda autorizada se revoca al desconectar el TCP de ese pid
        pid = str(self._pid_visible(sesion))
        if pid not in {str(self._pid_visible(x)) for x in self.sesiones}:
            for k in [k for k, v in self.sondas.items() if v[0] == pid]:
                del self.sondas[k]

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
            try:
                extra = await manejador(sesion, msg)
            except Exception:
                # un fallo nuestro no debe cerrar la conexión: el cliente lo vive como desconexión
                log.exception("fallo en %s", msgid)
                extra = {"result": "ERR_SERVERNOTREADY"}
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
        urls = dict(self.cfg["urls"])  # {type: url}; debe incluir TOPINFO, BANNER, GET_TURN_SERVER_LIST...
        # C (PESBUL, 02-nov): justo tras confirmar el país el cliente busca EULA_PES, EULA_MYCLUB y
        # EULA_PRIVACY_POLICY en esta lista; si falta una da «Se ha producido un error» sin red.
        base = self.cfg.get("http_base") or next(
            (u.rsplit("/", 1)[0] for u in urls.values() if u.startswith("http")), "http://127.0.0.1:5000")
        for t, ruta in self.URLS_EULA:
            urls.setdefault(t, base + ruta)
        return {"url_num": len(urls),
                "ulist": [Empaquetado({"type": t, "url": u}) for t, u in urls.items()]}

    URLS_EULA = (("EULA_PES", "/eula/pes"), ("EULA_MYCLUB", "/eula/myclub"),
                 ("EULA_PRIVACY_POLICY", "/eula/privacy-policy"),
                 # C (PESBUL R7.3.172): tras crear el usuario el cliente busca la clave exacta
                 # «FLOW» (y las LiveDataSet*); si falta: «Error al descargar los nuevos archivos»
                 ("FLOW", "/livedata/LiveDataSetFlow"),
                 ("LiveDataSetFlow", "/livedata/LiveDataSetFlow"),
                 ("LiveDataSetFlowOnMode", "/livedata/LiveDataSetFlowOnMode"),
                 ("LiveDataSetFlowOffMode", "/livedata/LiveDataSetFlowOffMode"),
                 ("LiveDataSetFlowExhibition", "/livedata/LiveDataSetFlowExhibition"),
                 ("LiveDataSetEditData", "/livedata/LiveDataSetEditData"),
                 # C (PESBUL R7.3.634 + crash 02-nov en 140c5ec4b): imágenes del menú online; si
                 # falta la clave la URL es NULL y TopMode revienta (c0000005, FUN_140c5f7b0)
                 ("DMM_TOP", "/asset/dmm-top"), ("WEEKLY_INFO_BANNER", "/asset/weekly-info-banner"),
                 ("BANNER", "/asset/banner"), ("RANK", "/rank"), ("DIVISION_RANK", "/division_rank"),
                 ("AVATAR", "/avatar/AVATAR"),
                 # C (PESBUL local 04-nov): clave exacta del chequeo de red; si falta, el
                 # cliente falla antes de abrir el socket UDP del relé
                 ("GET_GAMERELAY_QUALITYCHECKLIST", "/gamerelay/qualitychecklist"),
                 ("GAMERELAY", "/gamerelay"))
    VERSIONES_EULA = ("eula_version", "eula_myclub_version", "eula_privacy_policy_version")
    VERSIONES_EULA_CERO = ("eula_privacy_notice_version", "eula_privacy_policy_california_version",
                           "eula_privacy_policy_japan_version")

    async def c_cmd_get_svrversion(self, s, m):
        # P (02-nov): versión del juego 1.07.02; patch/dlc/livedata se devuelven tal como los
        # manda el cliente; siempre los 10 campos, también el de Japón. EULA "0" por defecto:
        # con "1" (como PESBUL) el cliente ya no mostró país ni aviso (prueba 02-nov 01:01).
        v = {k: self._texto(x) for k, x in self.cfg.get("versiones", {}).items()}
        r = {"version": v.get("version") if v.get("version") not in (None, "", "0") else "1.07.02"}
        for k, defecto in (("patch_version", "1.07.02"), ("dlc_version", "7.00"),
                           ("livedata_version", "0")):
            r[k] = self._texto(m.get(k)) or (v.get(k) if v.get(k) not in (None, "", "0") else defecto)
        for k in self.VERSIONES_EULA:
            r[k] = v.get(k) or "0"
        for k in self.VERSIONES_EULA_CERO:
            r[k] = v.get(k) or "0"
        return r

    async def c_cmd_get_svrtime(self, s, m):
        return {"date": int(time.time())}  # C: entero; P: segundos Unix (el exe suma 9 h JST)

    @staticmethod
    def _texto(v):
        # C (REA 29-oct, fns 141bd88e0/141bcdc60): el cliente lee estos campos como CADENA;
        # un entero o bool rompe la elección de país (error 4 tras confirmar)
        if isinstance(v, bool):
            return "1" if v else "0"
        return "" if v is None else str(v)

    PRIVACIDADES = ("PRIVACY_POLICY", "PRIVACY_NOTICE", "PRIVACY_POLICY_CALIFORNIA",
                    "PRIVACY_POLICY_JAPAN")

    @classmethod
    def _nacion(cls, n):
        # C (30-oct): has_area es el literal "YES"/"NO" y privacy_kind uno de los enums de
        # arriba. Con "1"/"0" el cliente toma la ruta de zonas, la halla vacía y da error 4.
        n = {k: cls._texto(v) for k, v in n.items()}
        n["has_area"] = "YES" if n.get("has_area", "NO").upper() in ("YES", "1", "TRUE") else "NO"
        if n.get("privacy_kind") not in cls.PRIVACIDADES:
            n["privacy_kind"] = "PRIVACY_POLICY"
        return n

    async def c_cmd_get_eula_nation_list(self, s, m):
        # Nunca vacía: el cliente aborta con una lista de 0 elementos
        naciones = [self._nacion(n) for n in self.cfg["naciones"]]
        return {"nation_list": [Empaquetado(n) for n in naciones],
                "nation_list_num": len(naciones)}

    async def c_cmd_get_eula_area_list(self, s, m):
        # P (REA 29-oct §16): al confirmar el país el cliente indexa el vector de ZONAS; vacío ->
        # error 4. Claves del lector aún sin leer: se usan las de la lista de naciones (cadenas).
        nacion = self._texto(m.get("nation_code", self.cfg.get("nacion", "")))
        zonas = self.cfg.get("areas", {}).get(nacion) or self.cfg.get("areas_por_defecto", [])
        zonas = [{k: self._texto(v) for k, v in z.items()} for z in zonas]
        return {"area_list": [Empaquetado(z) for z in zonas], "area_list_num": len(zonas)}

    async def c_cmd_get_eula_nation_code(self, s, m):
        # nation_code = país YA guardado: si coincide con la lista, el cliente salta la pantalla;
        # "" obliga a elegir
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

    AVISO_POR_DEFECTO = {"title": "Phoenix Online", "body": "Bienvenido al servidor Phoenix."}

    def _avisos(self):
        # Sin avisos el cliente muestra «Lista de imágenes: no hay datos»: siempre al menos uno
        return self.cfg.get("anuncios") or [self.AVISO_POR_DEFECTO]

    @staticmethod
    def _aviso_basico(a, n):
        # C (30-oct): announce_type es un enum de texto; sin imagen = NO_BANNER_INFO.
        # date entero de 32 bits; evoked_text vacío.
        return {"announce_type": "NO_BANNER_INFO", "title": str(a.get("title", "")),
                "announce_id": int(a.get("announce_id", n)), "priority": int(a.get("priority", 0)),
                "popup_type": "ONCE", "weekly_info_type": "MYCLUB_AGENT", "event_id": 0,
                "date": int(a.get("date", 0)), "evoked_text": ""}

    async def c_cmd_get_globalinfotitlelist(self, s, m):
        avisos = [Empaquetado(self._aviso_basico(a, i)) for i, a in enumerate(self._avisos(), 1)]
        return {"announce_info_list": avisos,
                "annnounce_info_list_num": len(avisos)}  # tres «n», así lo lee el cliente

    async def c_cmd_get_privateinfotitlelist(self, s, m):
        return {"announce_info_list": [], "annnounce_info_list_num": 0}

    async def c_cmd_get_infobody(self, s, m):
        avisos = self._avisos()
        n = int(m.get("announce_id", 1) or 1)
        a = avisos[n - 1] if 0 < n <= len(avisos) else avisos[0]
        return {"announce_info": Empaquetado({
            "announce_basic_info": Empaquetado(self._aviso_basico(a, n)),
            "body": str(a.get("body", "")), "link_url": "", "direct_move": "",
            "use_template": "", "template_id": 0})}

    async def c_cmd_get_server_env(self, s, m):
        # C (PESBUL R7.3.169/631): nombres probados en Ghidra; valores de cuenta nueva.
        # max_division es el TOKEN "D12" (el entero o "10" da VNBP_0002); is_revision_check 0.
        # C (PESBUL 04-nov): proto_opt es una CADENA JSON compacta con el perfil de red
        # (por defecto «MatchCommandDropMode 1»); antes mandábamos el entero 0
        proto = self.cfg.get("proto_opt",
                             {"CS": {"MatchCommandBufferingControl": {"MatchCommandDropMode": 1}}})
        if not isinstance(proto, (str, int)):
            proto = json.dumps(proto, sort_keys=True, separators=(",", ":"))
        return {"proto_opt": proto, "max_division": "D12",
                "teamlobby_game_rating_border_normal": 0, "teamlobby_game_rating_border_hard": 0,
                "is_revision_check": 0, "quality_check_wait_sec": 0, "trigger_keep_alive": 30,
                "matchrejectpid_max": 0, "match_client_timeout_load_timeout": 60,
                "match_client_timeout_long_outofplay": 60, "match_client_timeout_init_not_end": 60,
                "match_client_timeout_cmd_lack": 60, "is_check_myclub_level": 0,
                "log_update_level": 0, "lobby_ping_good": 50, "lobby_ping_normal": 150,
                "quick_ping_good": 50, "quick_ping_normal": 150, "konamiid_api_retry_sec": 5}

    # C (PESBUL R7.3.169): el menú online pide este lote al entrar; con NOERR vacío falla
    async def c_cmd_get_match_analyze_env(self, s, m):
        return {"posession": 0, "passtype": 0, "deffence": 0, "match_up": 0,
                "favorite_play": 0, "favorite_play_rate": 0}

    async def c_cmd_get_manner_warn(self, s, m):
        return {"warn_kind": 0}

    async def c_cmd_get_game_team_list(self, s, m):
        return {"team_list": [], "team_list_num": 0}

    async def c_cmd_get_dmm_playerinfo(self, s, m):
        return {"is_present_inbox": 0}

    async def c_cmd_send_log(self, s, m):
        log.info("SEND_LOG del cliente: %s", str(m.get("title", ""))[:80])
        return {}

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
            if k == "username" and isinstance(v, str) and v.strip("0 "):
                # C (02-nov / 04-nov): username es SIEMPRE hex sin ceros ('3a660'). Con el PES base
                # el SteamID 0x0410000004100000 llega como '410000004100000', solo dígitos: no es decimal
                v = v.strip().lower()
                if all(c in "0123456789abcdef" for c in v) and len(v) <= 16:
                    return v.zfill(16)
            if isinstance(v, str) and v.isdigit() and int(v) > 0:
                return format(int(v), "016x")  # P: np_account_id en decimal como texto
            if isinstance(v, str) and v.strip("0 "):
                v = v.strip().lower()
                # C (02-nov): CMD_LOGIN trae username = hex SIN ceros a la izquierda ('3a660')
                if all(c in "0123456789abcdef" for c in v) and len(v) <= 16:
                    return v.zfill(16)
                return v
        return None

    def _entrar(self, s, m):
        steam_hex = self._steam_de(m) or s.steam_hex or ""
        if not self.datos.autorizado(steam_hex):
            log.info("login rechazado (sin cuenta Phoenix vinculada): %s", steam_hex)
            return {"result": "ERR_LOGIN_FAILED"}  # C: texto de error del exe
        s.steam_hex = steam_hex
        s.versiones = {k: m.get(k) for k in ("patch_version", "dlc_version",
                                              "livedata_version", "extradata_version")}
        # La conexión MENU (matchmaking) no pide GET_PLAYER_DATA: sin esto la tarjeta salía
        # «Phoenix» en vez del nombre del jugador (captura 02-nov)
        j = self.datos.jugador(steam_hex)
        if j is not None:
            self._registrar(s, j)
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
        # C (PESBUL R7.3.121, Frida): el cliente instala el perfil solo si has_data == "YES"
        # (FUN_141211da0); luego lee pid y pname (bin con name). Un sobre «rico» con listas y
        # alias hace que se desconecte; cualquier otro has_data = crear perfil (bucle).
        j = self.datos.jugador(s.steam_hex or "")
        if not j:
            return {"has_data": "NO"}
        self._registrar(s, j)
        return {"has_data": "YES", "pid": self._pid_visible(s),
                "pname": Empaquetado(self._identidad(s, j["pname"]))}

    def _pid_visible(self, s):
        return self._numero_cuenta(s) or s.pid

    @staticmethod
    def _numero_cuenta(s):
        try:
            return int(s.steam_hex or "0", 16)
        except ValueError:
            return 0

    @classmethod
    def _identidad(cls, s, nombre):
        # C (crash 02-nov en 141d3fe49 + PESBUL): psn_user_id NO es texto sino otro MessagePack
        # empaquetado (bin) con account_id/online_id/is_valid_cache; con "" el lector revienta.
        cuenta = cls._numero_cuenta(s)
        texto = str(cuenta)
        # P (PESBUL 02-nov): xuid_one y steam_id llevan el número de plataforma en texto
        return {"name": nombre, "xuid_one": texto or "0",
                "psn_user_id": Empaquetado({"account_id": cuenta, "online_id": nombre,
                                            "is_valid_cache": "1", "steam_id": texto}),
                "is_deleted": "0", "steam_id": texto}

    async def c_cmd_create_player(self, s, m):
        nombre = m.get("pname") or m.get("name") or f"Phoenix_{(s.steam_hex or '')[-4:]}"
        if isinstance(nombre, (bytes, dict)):
            nombre = f"Phoenix_{(s.steam_hex or '')[-4:]}"
        j = self.datos.crear_jugador(s.steam_hex or "", nombre)
        self._registrar(s, j)
        return {"pid": self._pid_visible(s), "name": j["pname"]}

    async def c_cmd_set_currentplayer(self, s, m):
        # C (PESBUL R7.3.122): el msgid real va SIN guion entre CURRENT y PLAYER; sin respuesta
        # el cliente espera ~53 s y se desconecta
        return {}

    c_cmd_set_current_player = c_cmd_set_currentplayer

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

    # ---------- Partida rápida: matchmaking (PESBUL r7.3.641-649, solo hechos de protocolo) ----------
    async def c_cmd_get_quickmatchoption(self, s, m):
        return dict(s.opciones_rapida)

    async def c_cmd_set_quickmatchoption(self, s, m):
        for k in OPCIONES_RAPIDA:
            if k in m:
                s.opciones_rapida[k] = m[k]
        return {}

    async def c_cmd_get_maintenance_info(self, s, m):
        return {}

    def _salir_de_cola(self, s):
        for cola in self.colas.values():
            if s in cola:
                cola.remove(s)

    def _miembro(self, s, orden, dueno, lado=None):
        """Una ficha del roster de CMD_WATCH_MATCHING (bin con MessagePack dentro)."""
        ident = self._identidad(s, s.pname or "Phoenix")
        f = {"matching_order": orden, "is_owner": "YES" if dueno else "NO",
             "pid": self._pid_visible(s) or 0, "pname": Empaquetado(ident),
             "ntl_nat_type": int(s.ntl.get("ntl_nat_type") or 0),
             "has_guestplayer": "NO", "guest_info": Empaquetado([]),
             "rating": 0, "manner": "", "team_power_lv": "", "division": "",
             "retry_num": 0, "base_team_id": 0, "team_name": "",
             "hometown_info": Empaquetado([]), "game_result_wdl": [],
             "is_pes_league_mode": "NO", "skill_rank": "", "is_entry_weekly_myclub": "NO",
             "weekly_point": 0, "join_compe_rating": 0}
        if lado is not None:
            f["side"] = lado  # C (r7.3.650): sin side, el 2.º jugador no ve su tarjeta
        for k in ("host_addr", "reflexive_addr", "relayed_addr"):
            v = s.ntl.get(k)
            if isinstance(v, dict):
                v = Empaquetado(v)
            if v:
                f[k] = v
        return Empaquetado(f)

    @staticmethod
    def _vigilar_emparejamiento(s, estado, miembros):
        # C (r7.3.641/644): el estado llega por un EMPUJE con rqid 0 y result NOERR obligatorio;
        # START_MATCHING solo lleva msgid/rqid/result
        s.enviar({"msgid": "CMD_WATCH_MATCHING", "rqid": 0, "result": NOERR,
                  "matching_state": estado, "player_num": len(miembros), "member": miembros})

    async def c_cmd_start_matching(self, s, m):
        tipo = str(m.get("matching_type") or "OC_QUICK")
        self._salir_de_cola(s)
        self.parejas.pop(s, None)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        cola = self.colas.setdefault(tipo, [])
        rival = next((x for x in cola if x is not s and x in self.sesiones), None)
        if rival is None:
            cola.append(s)
            await self.dormir(PAUSA_EMPUJE)
            if s in cola:  # tarjeta propia en el panel LOCAL mientras busca (r7.3.647)
                self._vigilar_emparejamiento(s, "SEARCHING", [self._miembro(s, 0, False)])
            return None
        cola.remove(rival)
        # el que esperaba es anfitrión/LOCAL; el que completa la pareja, VISITANTE (r7.3.644/650)
        miembros = [self._miembro(rival, 0, True, "HOME"), self._miembro(s, 1, False, "AWAY")]
        pareja = {"miembros": (rival, s), "check": set()}
        self.parejas[rival] = self.parejas[s] = pareja
        await self.dormir(PAUSA_EMPUJE)
        for x in (rival, s):
            self._vigilar_emparejamiento(x, "FINISH", miembros)
        log.info("pareja de %s: %s vs %s", tipo, rival.pid, s.pid)
        return None

    async def c_cmd_cancel_matching(self, s, m):
        self._salir_de_cola(s)
        self.parejas.pop(s, None)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        # C (r7.3.648): la pantalla de búsqueda solo se cierra con el empuje CANCEL
        self._vigilar_emparejamiento(s, "CANCEL", [])
        return None

    async def c_cmd_set_gamerelay_check_res(self, s, m):
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        pareja = self.parejas.get(s)
        if pareja is None:
            return None
        pareja["check"].add(s)
        # P (política PESBUL): se libera cuando los dos informaron; el cliente espera 30 s máx.
        if len(pareja["check"]) == 2:
            for x in pareja["miembros"]:
                x.enviar({"msgid": "CMD_WATCH_GAMERELAY_CHECK_RES", "rqid": 0,
                          "result": NOERR, "error_code": NOERR})
        return None

    async def c_cmd_send_matching_result(self, s, m):
        if m.get("error_code") not in (None, NOERR):
            log.warning("matching_result con error %s pid=%s", m.get("error_code"), s.pid)
        return {}

    async def c_cmd_set_gamemember(self, s, m):
        return {}

    # ---------- Lobby (§11.5 pasos 1-2) ----------
    async def c_cmd_get_lobbylist(self, s, m):
        tipo = m.get("lobby_type", "FREE")
        lobbies = [l for l in self.cfg["lobbies"] if l["lobby_type"] == tipo]
        # C (PESBUL 04-nov): el cliente liga la calidad del lobby a gamerelay_index == índice del
        # nodo del checklist; con 0 no hay nodo. Si la config pone 0, se usa el del relé.
        nodos = self._nodos_relay()
        indice = nodos[0]["gamerelay_index"] if nodos else 0
        return {"lobby_list_num": len(lobbies),
                "lobby_list": [Empaquetado({**l, "player_num": len(self.por_pid),
                                            "gamerelay_index": l.get("gamerelay_index") or indice})
                               for l in lobbies]}

    async def c_cmd_get_lobbyinfo(self, s, m):
        return {"has_info": True, "title": "Phoenix Online",
                "sentence_info": self.cfg.get("lobby_info", ""), "sentence_intro": ""}

    def _url_gamerelay_web(self):
        # C (PESBUL 04-nov): JOIN_LOBBY lleva http://<IP>:5000/gamerelay; vacío deja al cliente
        # en XXJA_0005 / «no se reciben datos» en el vestíbulo
        u = self.cfg["urls"].get("GAMERELAY_WEB")
        if u:
            return u
        base = self.cfg.get("http_base") or f"http://{self.cfg.get('ip_publica', '127.0.0.1')}:" \
                                            f"{self.cfg.get('puerto_http', 5000)}"
        return base.rstrip("/") + "/gamerelay"

    async def c_cmd_get_blocklist(self, s, m):
        # C (PESBUL R7.3.183): respuesta pelada = XXJA_0005 y el cliente sale del vestíbulo;
        # lista vacía tampoco sirve. Un bloque «Main» como bin.
        bloque = int(m.get("lobby_id") or getattr(s, "lobby_id", None) or 1)
        return {"count": 1, "bklist": [Empaquetado({"name": "Main", "player_num": len(self.por_pid),
                                                     "max_player_num": 100, "block_id": max(1, bloque)})]}

    async def c_cmd_join_lobby(self, s, m):
        s.lobby_id = m.get("lobby_id") or 1
        return {"gamerelay_web_url": self._url_gamerelay_web(), "bklist": []}

    # ---------- Vigilancias ----------
    async def _al_vigilar(self, s, msgid):
        if msgid == "CMD_REQWATCHL_ROOMLIST":
            self._empujar_salas(s, "RELOAD", list(self.salas.values()))
        elif msgid == "CMD_REQWATCHL_ROOMPLAYERLIST_NTL" and s.room_id in self.salas:
            # L (ESPEC_SALAS §2.5): al abrir el watch dentro de la sala, RELOAD con los jugadores
            self._empujar_jugadores(s, self.salas[s.room_id], "RELOAD")
        elif msgid == "CMD_WATCH_TURN_ADDRESS_DATA" and s.pid in self.turn_pendiente:
            addr, size = self.turn_pendiente.pop(s.pid)
            s.completar(msgid, address=addr, address_data_size=size)

    def _empujar_salas(self, s, accion=None, salas=None):
        # L (ESPEC_SALAS §1): siempre RELOAD con TODAS las salas, list_key_name «slot» y la fila
        # vista por quien recibe (con ADD el cliente queda vacío -> VZLN_0002)
        salas = list(self.salas.values())
        s.completar("CMD_REQWATCHL_ROOMLIST", list_type="RELOAD", list_num=len(salas),
                    list_key_name="slot",
                    list_data=[Empaquetado(x.ficha(s, i)) for i, x in enumerate(salas)])

    def _empujar_salas_a_todos(self, accion=None, salas=None):
        for otro in list(self.sesiones):
            self._empujar_salas(otro)

    def _empujar_jugadores(self, s, sala, accion="RELOAD", miembros=None):
        # L (ESPEC_SALAS §3): PESBUL manda RELOAD; al entrar un segundo jugador se mantiene el
        # orden UPDATE -> ADD que ya se había comprobado (§11.5.6)
        miembros = sala.miembros if miembros is None else miembros
        datos = [Empaquetado(sala.ficha_jugador(x, sala.miembros.index(x))) for x in miembros]
        s.completar("CMD_REQWATCHL_ROOMPLAYERLIST_NTL", list_type=accion, list_num=len(datos),
                    list_key_name="pid", list_data=datos)

    # ---------- Salas (§11.5 pasos 4-8) ----------
    async def c_cmd_createjoin_room(self, s, m):
        await self._salir_de_sala(s)
        nombre = m.get("name") if isinstance(m.get("name"), str) and m.get("name") else f"Sala de {s.pname}"
        sala = Sala(s, nombre, {k: v for k, v in m.items() if k not in ("msgid", "rqid")})
        self.salas[sala.room_id] = sala
        s.room_id, s.entry = sala.room_id, False
        # L (ESPEC_SALAS §2): PRIMERO la respuesta, DESPUÉS el RELOAD de salas
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR,
                  "room_id": sala.room_id})
        self._empujar_salas_a_todos()
        return None

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
        entrada = m.get("entry", "TRUE")
        s.entry = (entrada.upper() in ("TRUE", "YES", "1")) if isinstance(entrada, str) else bool(entrada)
        # C/L (ESPEC_SALAS §4): entryflag = eco en texto y gameNum
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR,
                  "entryflag": "TRUE" if s.entry else "FALSE",
                  "gameNum": 1 if sala and sala.partido else 0})
        if not sala:
            return None
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

    # ---------- Gamerelay: comprobación de red (ESPEC_GAMERELAY.md §1 y §5) ----------
    def _nodos_relay(self):
        """Nodos UDP del relé que se anuncian al cliente (C: índice 1, IP del servidor, 5100, grid 0)."""
        g = self.cfg.get("gamerelay") or {}
        if not g.get("activo", True):
            return []
        return [{"gamerelay_index": int(g.get("indice", 1)),
                 "ip_addr": str(g.get("ip") or self.cfg.get("ip_publica", "127.0.0.1")),
                 "port": int(g.get("puerto", 5100)), "grid": int(g.get("grid", 0))}]

    def lista_calidad(self, pid, ip_cliente=None):
        """Respuesta de GET_GAMERELAY_QUALITYCHECKLIST (HTTP o TCP).

        Efecto lateral (C): autoriza una sonda UDP desde esa IP contra cada nodo hasta que
        el pid se desconecte (tope 86 400 s)."""
        nodos = self._nodos_relay()
        if ip_cliente:
            hasta = time.time() + 86400
            for n in nodos:
                self.sondas[(ip_cliente, n["gamerelay_index"])] = (str(pid or ""), hasta)
        medidas = self.calidades.get(str(pid or ""), {})
        g = self.cfg.get("gamerelay") or {}
        return {"quality_check_data_list": [Empaquetado(n) for n in nodos],  # P: bin con mapa
                "quality_check_data_num": len(nodos),
                "quality_threshold_1": int(g.get("umbral_1", 80)),   # P: política propia (ms)
                "quality_threshold_2": int(g.get("umbral_2", 150)),
                # lista vacía = «mídelo tú» (L): así PES vuelve a medir cada vez
                "quality_list": [Empaquetado(v) for v in medidas.values()] if g.get("recordar") else [],
                "quality_list_num": len(medidas) if g.get("recordar") else 0}

    def sonda_autorizada(self, ip_cliente, indice):
        x = self.sondas.get((ip_cliente, indice))
        return bool(x and x[1] > time.time())

    async def c_cmd_get_gamerelay_qualitychecklist(self, s, m):
        ip = s.direccion[0] if isinstance(s.direccion, tuple) else None
        return self.lista_calidad(self._pid_visible(s), ip)

    async def c_cmd_get_gamerelay(self, s, m):
        nodos = self._nodos_relay()
        return {"gamerelay_web_url": self._url_gamerelay_web(), "gamerelay_num": len(nodos),
                "gamerelay_list": [Empaquetado(n) for n in nodos]}

    async def c_cmd_set_gamerelay_quality(self, s, m):
        # C: cada elemento llega como bin con un mapa {gamerelay_index, quality(ms), last_update};
        # 0xFFFFFFFF = no se pudo medir. Se guarda por pid e índice; respuesta solo NOERR.
        pid = str(self._pid_visible(s))
        for x in m.get("quality_list") or []:
            if isinstance(x, (bytes, bytearray)):
                try:
                    x = msgpack_min.unpackb(bytes(x))
                except Exception:
                    continue
            if isinstance(x, dict) and "gamerelay_index" in x:
                self.calidades.setdefault(pid, {})[int(x["gamerelay_index"])] = {
                    "gamerelay_index": int(x["gamerelay_index"]),
                    "quality": int(x.get("quality", 0xFFFFFFFF)),
                    "last_update": int(x.get("last_update", 0))}
                log.info("calidad gamerelay pid=%s nodo=%s %s ms", pid, x["gamerelay_index"],
                         x.get("quality"))
        return {}

    async def c_cmd_check_local_gamerelay_player(self, s, m):
        # C (REA 29-oct, fn 141b9cdc0): claves leídas por el cliente. Sin relé propio por ahora:
        # 0 relés por modo -> el partido va P2P (directo o por TURN).
        return {"is_local_gamerelay_player": False, "gamerelay_register_url_data": "[]",
                "build_gamerelay_num_for_1vs1": 0, "build_gamerelay_num_for_3vs3": 0,
                "build_gamerelay_num_for_11vs11": 0}

    def respuesta_livedata(self, flujo, formulario):
        """Datos en vivo por HTTP: «no hay actualización» con las versiones del juego (P, PESBUL)."""
        log.info("HTTP livedata %s campos=%s", flujo, sorted(formulario))
        return {"msgid": formulario.get("msgid") or flujo, "rqid": int(formulario.get("rqid", 0) or 0),
                "result": NOERR, "flow": flujo, "urltype": flujo, "status": "online",
                "no_update": True, "update_required": False, "maintenance": False,
                "live_update_status": "ok", "game_version": "1.07.02", "exe_version": "1.07.02",
                "data_pack_version": "DP07", "data_pack_number": "7.00"}

    # ---------- tienda y logros (cuenta nueva: todo vacío) ----------
    # C (PESBUL R7.3.124): las *_list son ARRAYS msgpack nativos; como bin el cliente lee un
    # número de elementos basura y revienta
    async def c_cmd_get_store_product_list(self, s, m):
        return {"product_list": [], "product_list_num": 0}

    async def c_cmd_get_gp_achievement(self, s, m):
        return {"all_gp": 0, "all_achievement_bit_array": [], "pes_point": 0}

    async def c_cmd_get_store_transaction_state(self, s, m):
        return {"state": 0, "product_id": "", "use_num": 0, "possession_num": 0,
                "live_transaction_id": "", "transaction_id": "",
                "used_present_product_id_list": [], "used_present_product_id_list_num": 0,
                "steam_order_id": ""}

    def respuesta_http(self, formulario, ip_cliente=None):
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
        if msgid in ("CMD_GET_GAMERELAY_QUALITYCHECKLIST", "GET_GAMERELAY_QUALITYCHECKLIST"):
            return {**base, **self.lista_calidad(formulario.get("pid"), ip_cliente)}
        if msgid in ("CMD_GET_AVATAR_HTTP", "CMD_GET_AVATAR"):
            return {**base, **self._avatar(formulario.get("pid"))}
        if msgid == "CMD_AUTH_STEAM":
            # Ticket = GetAuthSessionTicket en hex + SteamID64 en claro. Verificar la firma
            # del ticket queda pendiente; hoy la identidad la garantiza el vínculo de Link.
            resumen = {k: (f"<{len(v)} car.>" if len(str(v)) > 40 else v)
                       for k, v in formulario.items()}
            log.info("HTTP CMD_AUTH_STEAM %s", resumen)
            return base
        log.warning("comando HTTP sin manejador: %s campos=%s", msgid, sorted(formulario))
        return base

    def _avatar(self, pid):
        """C (PESBUL avatar_service): avatar_data = base64 de una imagen 128x128x3 (49152 B),
        avatar_data_size = bytes decodificados y avatar_md5 = md5 de esos bytes. Sin avatar:
        NOERR con datos vacíos (no se inventa imagen)."""
        texto = self.avatares.get(str(pid or ""), "")
        crudo = base64.b64decode(texto) if texto else b""
        return {"avatar_data": texto, "avatar_data_size": len(crudo),
                "avatar_md5": hashlib.md5(crudo).hexdigest() if crudo else ""}

    async def c_cmd_get_avatar_http(self, s, m):
        return self._avatar(m.get("pid"))

    c_cmd_get_avatar = c_cmd_get_avatar_http

    async def c_cmd_set_avatar(self, s, m):
        # el cliente sube el base64 tal cual; se guarda en memoria (P: persistir en la web)
        datos = m.get("avatar_data")
        if isinstance(datos, (bytes, str)) and datos:
            self.avatares[str(self._pid_visible(s))] = datos.decode() if isinstance(datos, bytes) else datos
        return {}

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
