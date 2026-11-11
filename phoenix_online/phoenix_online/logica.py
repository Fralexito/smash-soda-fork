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
import os
import random
import time

from . import divisiones as div
from . import msgpack_min
from .nclmio import MAX_CIFRADO_CLIENTE
from .msgpack_min import Empaquetado

log = logging.getLogger("phoenix_online")

NOERR = "NOERR"

# Comandos que solo esperan msgid/rqid/result (cap_72 §11.4)
CONFIRMACIONES = {
    "CMD_SEND_PSID", "CMD_SET_CURRENT_PLAYER", "CMD_LOGIN_MENU", "CMD_SET_LANGUAGE",
    "CMD_JOIN_OFFLINE_MODE",
    "CMD_LEAVE_OFFLINE_MODE", "CMD_SEND_HEARTBEAT", "CMD_SET_ROOM_OPTION",
    # L (PESBUL): estos también se contestan solo con NOERR
    "CMD_CHECK_CHEAT", "CMD_GET_DMM_NOTIFICATION", "CMD_GET_URGENT_ANNOUNCEMENT",
    # L (ESPEC_GAMERELAY_FASE23 §5.1): durante una partida de 2 solo esperan NOERR, y A TIEMPO
    # (SET_GAMEPLAYER_PLAYTIME sin respuesta = ABNORMALEND a los ~55 s para los dos)
    "CMD_SET_GAMEPLAYER_PLAYTIME", "CMD_SET_GAMETEAM", "CMD_CHANGE_GAMEPHASE",
    "CMD_SEND_MATCH_ANALYZE", "CMD_ADD_SCORE", "CMD_ADD_FOUL", "CMD_SEND_MATCH_TEAM_INFO",
    "CMD_SEND_STATS_DATA", "CMD_REQ_GAMEADDPOINT", "CMD_START_SEND_GAME_SEQUENCE_DATA",
    "CMD_SET_GAMEMEMBERENV", "CMD_SET_GAMERELAY_MATCH_QUALITY", "CMD_SET_LOCAL_GAMERELAY_LIST",
}

# C (§17, capturado 05-nov): gameplan que el propio cliente generó para un 4-2-3-1 con 18
# inscritos (3 bloques de táctica de 0xA0 + tabla de huecos 0..17). Se usa como plan inicial
# para que un club nuevo tenga XI + banquillo inscritos.
GAMEPLAN_DEFECTO = base64.b64decode(
    "AAEBAwIEBAcGDAgDNAo/CikMWQwPEkASKB5WHhIrNCA0AAEBAwIEBAcGDAgDNAo/CikMWQwPEkASKB5WHhIrNCA0AAEBAwIEBAcGDAgDNAo/CikMWQwPEkASKB5WHhIrNCA0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAAAv//////////////AAAAAAABAQMCBAQHBggMAzQKPwopDFkMDxJAEigeVh4SIDQrNAABAQMCBAQHBggMAzQKPwopDFkMDxJAEigeVh4SIDQrNAABAQMCBAQHBggMAzQKPwopDFkMDxJAEigeVh4SIDQrNAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAL//////////////wAAAAAAAQEDAgQEBwYIDAM0Cj8KKQxZDA8SQBIoHlYeEiA0KzQAAQEDAgQEBwYIDAM0Cj8KKQxZDA8SQBIoHlYeEiA0KzQAAQEDAgQEBwYIDAM0Cj8KKQxZDA8SQBIoHlYeEiA0KzQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAC//////////////8AAAAAAAECAwQFBgcICQoLDA0ODxAR/////////////////////////////wkJCQkJCf///wAAAAABAAA=")

# Vigilancias que el cliente deja abiertas y el servidor completa más tarde
VIGILANCIAS = {
    "CMD_REQWATCHL_ROOMLIST", "CMD_REQWATCHL_ROOMPLAYERLIST_NTL", "CMD_WATCH_ROOMSTATE",
    "CMD_WATCH_ENTRY_GAME", "CMD_WATCH_DECIDE_GAMEENV", "CMD_WATCH_DECIDE_GAMEENV_FREE",
    "CMD_WATCH_NOTICE", "CMD_WATCH_TURN_ADDRESS_DATA",
    # L (PESBUL): avisos y chat quedan abiertos; se completan solo cuando hay algo que decir
    "CMD_WATCH_SYNC_NOTICE", "CMD_WATCH_SYSTEM_NOTICE", "CMD_WATCH_TEXTCHAT",
}

# L: palabras que CHECK_STRING rechaza con ERR_NGWORD (único error que el cliente acepta ahí).
# Lista corta a propósito: un filtro grande rechaza nombres inocentes.
PALABRAS_PROHIBIDAS = ("FUCK", "SHIT", "NIGGER", "CUNT", "HITLER", "NAZI",
                       "MIERDA", "PUTA", "MARICON", "CONCHETUMARE")

DIVISION_INICIAL = "D12"
RATING_INICIAL = 1000
MODALES_DEFECTO = "AA"  # L: fichas de modales AA/A/B/C/D; vacío se pinta «-»

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
        self.steam_crudo = None  # SteamID tal como lo mandó el PES (antes de la clave por IP/cuenta)
        self.pid = None
        self.pname = None
        self.vigilancias = {}  # msgid -> rqid
        self.room_id = None
        self.ntl = {}
        self.huella = None
        self.gameseq_vistos = 0  # registros GameSeq TCP anotados (diagnóstico)
        self.ultima_alineacion = None
        self.versiones = {}
        self.entry = False
        self.lang = None
        self.cerrar_tras_responder = False
        self.opciones_rapida = dict(OPCIONES_RAPIDA)
        self.modo_online = None
        self.lobby_id = None
        self.block_id = None
        self.ip_local = None  # IP del servidor por la que entró este cliente (anuncio automático)
        self.desde = time.time()

    @property
    def ip(self):
        return self.direccion[0] if isinstance(self.direccion, tuple) else None

    def enviar(self, mensaje):
        self._enviar_trama(self.secuencia, mensaje)

    def completar(self, msgid, **campos):
        """Empuja un mensaje usando el rqid de una vigilancia abierta. False si no hay."""
        rqid = self.vigilancias.get(msgid)
        if rqid is None:
            return False
        self.enviar({"msgid": msgid, "rqid": rqid, "result": NOERR, **campos})
        return True


def _cifrado_comprimido(mensaje):
    """Bytes cifrados que ocuparía el mensaje en una trama comprimida (cabecera interna 9 B + zlib, a 8)."""
    import zlib
    n = 9 + len(zlib.compress(msgpack_min.packb(mensaje)))
    return n + (-n % 8)


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
        self.inicio = time.time()
        self.sin_manejador = {}  # msgid -> veces (para priorizar qué implementar)
        self.rele = None         # gamerelay.Rele (lo pone servidor.principal)
        self.ntl_por_clave = {}  # clave de persona -> última declaración NTL (+ ip TCP)
        self.huella_por_clave = {}  # clave de persona -> (n.º de archivos, sha256) de su file_hash_list
        self._partidas = itertools.count(1)  # salas sintéticas de las parejas: 700 000 000 + n
        self.nube = None          # espejo en Supabase (nube.py), si está activo
        self.intentos_pase = {}   # ip -> [instantes] de POST /pase (tope contra abusos)

    def aplicar_reglas_myclub(self, reglas):
        """Reglas de myClub editables en caliente (archivo reglas_myclub o, más adelante, la
        tabla myclub_reglas de Supabase). Se mezclan sobre cfg["myclub"]: lo nuevo manda.
        Devuelve las claves cambiadas."""
        if not isinstance(reglas, dict):
            return []
        actual = self.cfg.setdefault("myclub", {})
        cambiadas = [k for k, v in reglas.items() if actual.get(k) != v]
        actual.update(reglas)
        if cambiadas:
            log.info("reglas myClub actualizadas en caliente: %s", ", ".join(sorted(cambiadas)))
        return cambiadas

    def estado(self):
        """Resumen para GET /estado (Link, web o un vistazo desde el celular)."""
        en_linea = {x.steam_hex for x in self.sesiones if x.steam_hex}
        return {"servidor": "Phoenix Online", "activo_desde": int(self.inicio),
                "minutos_activo": int((time.time() - self.inicio) // 60),
                "conexiones": len(self.sesiones), "jugadores_en_linea": len(en_linea),
                "salas": len(self.salas), "buscando_rival": sum(len(c) for c in self.colas.values()),
                "parejas": len({id(p) for p in self.parejas.values()}),
                "mantenimiento": self.en_mantenimiento(),
                "rele": self.rele.estado() if self.rele is not None and hasattr(self.rele, "estado") else None,
                "comandos_sin_manejador": dict(sorted(self.sin_manejador.items(),
                                                      key=lambda kv: -kv[1])[:30])}

    # ---------- ciclo de vida ----------
    def abrir(self, sesion):
        self.sesiones.add(sesion)

    async def cerrar(self, sesion):
        self.sesiones.discard(sesion)
        if sesion.pid is not None and self.por_pid.get(sesion.pid) is sesion:
            del self.por_pid[sesion.pid]
        await self._salir_de_sala(sesion)
        self._salir_de_cola(sesion)
        self._dejar_pareja(sesion, "se cortó su conexión")
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
            self.sin_manejador[msgid] = self.sin_manejador.get(msgid, 0) + 1
            log.warning("comando sin manejador: %s campos=%s", msgid, sorted(msg))
            if "MYCLUB" in msgid:
                # myClub: la petición completa sirve para deducir formatos (gameplan, plantilla…)
                log.warning("peticion completa %s: %r", msgid, msg)
            extra = {}
        result = extra.pop("result", NOERR)
        sesion.enviar({"msgid": msgid, "rqid": rqid, "result": result, **extra})

    # ---------- ConnectGate (§11.3) ----------
    # ---------- IP que se anuncia al cliente ----------
    def ip_anuncio(self, s=None, ip_local=None):
        """IP del servidor que se le da al cliente (lista de servidores, URL, relé).

        `ip_publica` fija (VPS con IP propia, o una IP concreta) manda. Con "auto" o vacía se
        usa la IP por la que ESE cliente llegó al servidor: sirve igual en la misma PC, en la
        red de casa, por Tailscale/ZeroTier o en un VPS sin NAT, sin tocar la configuración."""
        fija = str(self.cfg.get("ip_publica") or "auto").strip()
        if fija.lower() != "auto":
            return fija
        ip = ip_local or (getattr(s, "ip_local", None) if s is not None else None)
        if ip and ip not in ("0.0.0.0", "::"):
            return ip.split("%")[0].removeprefix("::ffff:")
        return "127.0.0.1"

    def _con_ip(self, texto, s=None, ip_local=None):
        """Sustituye «{ip}» en una URL de la configuración por la IP anunciada."""
        if isinstance(texto, str) and "{ip}" in texto:
            return texto.replace("{ip}", self.ip_anuncio(s, ip_local))
        return texto

    def _urls(self, s=None):
        return {t: self._con_ip(u, s) for t, u in self.cfg["urls"].items()}

    async def c_cmd_get_urllist(self, s, m):
        urls = self._urls(s)  # {type: url}; debe incluir TOPINFO, BANNER, GET_TURN_SERVER_LIST...
        # C (PESBUL, 02-nov): justo tras confirmar el país el cliente busca EULA_PES, EULA_MYCLUB y
        # EULA_PRIVACY_POLICY en esta lista; si falta una da «Se ha producido un error» sin red.
        base = self._con_ip(self.cfg.get("http_base"), s) or next(
            (u.rsplit("/", 1)[0] for u in urls.values() if u.startswith("http")),
            f"http://{self.ip_anuncio(s)}:{self.cfg.get('puerto_http', 5000)}")
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
                 # C (05-nov, FUN_140d491c0): la caja del agente descarga su banner desde la
                 # URL tipo 17; sin la clave la URL es NULL y PES se cierra al abrir la caja
                 ("MYCLUB_AGENT_BANNER", "/asset/myclub-agent-banner"),
                 # C (PESBUL local 04-nov): clave exacta del chequeo de red; si falta, el
                 # cliente falla antes de abrir el socket UDP del relé
                 ("GET_GAMERELAY_QUALITYCHECKLIST", "/gamerelay/qualitychecklist"),
                 ("GAMERELAY", "/gamerelay"),
                 # L (ESPEC parte B.0/B.12, Ghidra 10-nov): estos comandos van por HTTP con
                 # urltype=<este nombre>; sin la URL el cliente nunca los pide
                 ("DIVISION_RANK_BYID", "/division_rank_byid"),
                 ("MYCLUB_RANKING", "/myclub_ranking"), ("MYCLUB_RANKING_BYID", "/myclub_ranking_byid"),
                 ("MYCLUB_WEEKLY_RANKING", "/myclub_weekly_ranking"),
                 ("MYCLUB_WEEKLY_RANKING_BYID", "/myclub_weekly_ranking_byid"),
                 ("RECV_MYCLUB_PRESENT", "/recv_myclub_present"))
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
        ip = self.ip_anuncio(s)
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

    ID_AVISO_CODIGO = 900

    def _avisos_de(self, s):
        """Avisos generales y, si el espejo con la web está activo y su club aún no tiene cuenta,
        primero uno con su código de «Vincular mi PES» (lo escribe en la web y queda unido)."""
        avisos = list(self._avisos())
        clave = getattr(s, "steam_hex", None)
        if self.nube is not None and clave and self.datos.club(clave) is not None:
            codigo = self.datos.codigo_web(clave)
            if codigo:
                web = (self.cfg.get("supabase") or {}).get("web", "la web Phoenix")
                avisos.insert(0, {"announce_id": self.ID_AVISO_CODIGO, "priority": 100,
                                  "title": f"Tu código Phoenix: {codigo}",
                                  "body": f"Para unir este club a tu cuenta: entra a {web} con tu sesión, "
                                          f"toca «Vincular mi PES» y escribe {codigo}. Se hace una sola vez. "
                                          f"Si usas el mismo nombre que en la web, allí ya verás tu club."})
        return avisos

    async def c_cmd_get_globalinfotitlelist(self, s, m):
        avisos = [Empaquetado(self._aviso_basico(a, i)) for i, a in enumerate(self._avisos_de(s), 1)]
        return {"announce_info_list": avisos,
                "annnounce_info_list_num": len(avisos)}  # tres «n», así lo lee el cliente

    async def c_cmd_get_privateinfotitlelist(self, s, m):
        return {"announce_info_list": [], "annnounce_info_list_num": 0}

    async def c_cmd_get_infobody(self, s, m):
        avisos = self._avisos_de(s)
        n = int(m.get("announce_id", 1) or 1)
        a = next((x for x in avisos if int(x.get("announce_id", 0) or 0) == n), None) or \
            (avisos[n - 1] if 0 < n <= len(avisos) else avisos[0])
        return {"announce_info": Empaquetado({
            "announce_basic_info": Empaquetado(self._aviso_basico(a, n)),
            "body": str(a.get("body", "")), "link_url": "", "direct_move": "",
            "use_template": "", "template_id": 0})}

    async def c_cmd_get_division_info(self, s, m):
        # L (PESBUL online_division): con NOERR pelado el cliente cae en «fin de temporada» y
        # repite la sonda; cuenta nueva = D12, partido 1 de la temporada, sin historial
        # L (ESPEC_DIVISIONES A.4): anidados como bin; historial = partidos de la temporada
        j = self.datos.jugador(s.steam_hex or "") or {}
        info = div.info_division(self._estado_division(s.steam_hex), self._reglas_division())
        info["match_record"] = Empaquetado(info["match_record"])
        info["match_history_list"] = [Empaquetado(h) for h in info["match_history_list"]]
        return {"pname": Sala.pname(s) if s.pname else Empaquetado(self._identidad(s, j.get("pname", ""))),
                "division_info": Empaquetado(info)}

    # ---------- Divisiones en línea (div = phoenix_online/divisiones.py) ----------
    def _reglas_division(self):
        return div.Reglas(self.cfg.get("divisiones"))

    def _estado_division(self, clave):
        return div.completar(self.datos.division(clave or "") if clave else None)

    async def c_cmd_finish_season(self, s, m):
        """L (A.0): el cliente cierra la temporada; solo se archiva si se jugó entera o ya ganó el
        ascenso (nunca una temporada vacía ni una a medias sin ascenso)."""
        if not s.steam_hex:
            return {}
        reglas = self._reglas_division()
        e = self._estado_division(s.steam_hex)
        sube = reglas.ascenso(e["division"])
        if e["jugados"] and (div.temporada_terminada(e, reglas) or (sube is not None and e["sp"] >= sube)):
            cambio = div.cerrar_temporada(e, reglas)
            self.datos.guardar_division(s.steam_hex, e)
            log.info("temporada cerrada para %s: %s -> %s", s.steam_hex, cambio, e["division"])
        return {}

    def _resultado_division(self, s, pareja, m):
        """CMD_SEND_GAME_RESULT de un partido de Divisiones (A.1/A.2). El PRIMER informe completo
        se aplica a los dos jugadores; el segundo solo se compara."""
        reglas = self._reglas_division()
        if pareja.get("tipo") not in reglas.tipos:
            log.info("resultado de %s: no es de Divisiones, no cambia rating", pareja.get("tipo"))
            return
        partido = f"div{pareja['room_id']}"
        try:
            goles = (max(0, int(m.get("home_score") or 0)), max(0, int(m.get("away_score") or 0)))
        except (TypeError, ValueError):
            log.warning("partido %s: marcador ilegible %r", partido, m)
            return
        motivo = m.get("abnormalend_reason")
        motivo = motivo if isinstance(motivo, str) else ""
        previo = self.datos.partido_division(partido)
        if previo is not None:
            if (previo.get("home", {}).get("score"), previo.get("away", {}).get("score")) != goles \
                    and not previo.get("no_cuenta"):
                log.warning("partido %s: los dos informes NO coinciden (%s vs %s); manda el primero",
                            partido, (previo["home"]["score"], previo["away"]["score"]), goles)
            return
        casa, fuera = pareja["miembros"]
        claves = pareja.get("claves") or [self._clave_partida(casa), self._clave_partida(fuera)]
        equipos = [m.get("home_team_id") or 0, m.get("away_team_id") or 0]
        fichas = []
        for x, clave, eq, g in zip((casa, fuera), claves, equipos, goles):
            j = self.datos.jugador(clave) or {}
            fichas.append({"pid": self._pid_en_partida(x), "name": j.get("pname") or x.pname or "",
                           "team": int(eq) if isinstance(eq, int) else 0, "score": g, "clave": clave})
        guardado = div.partido_guardable(time.time(), fichas[0], fichas[1], motivo)
        if motivo not in div.FINES_NORMALES and reglas.abandono == "no_count":
            # L (A.2, política no_count): un fin anormal no se adivina, no cuenta para nadie
            guardado["no_cuenta"] = True
            self.datos.guardar_partido_division(partido, guardado)
            log.info("partido %s: fin anormal (%s) %s-%s: no cuenta", partido, motivo, *goles)
            return
        if not self.datos.guardar_partido_division(partido, guardado):
            return
        estados = [self._estado_division(c) for c in claves]
        viejos = [e["rating"] for e in estados]
        cambios = []
        for i, (e, clave) in enumerate(zip(estados, claves)):
            o = 1 - i
            res, sp, viejo = div.aplicar(e, reglas, goles[i], goles[o], fichas[i]["team"], fichas[o]["team"],
                                         viejos[o], fichas[o]["name"], ("AWAY", "HOME")[i], partido)
            self.datos.guardar_division(clave, e)
            cambios.append((res, sp, viejo, e))
        log.info("partido %s aplicado: %s %d-%d %s (rating %d->%d / %d->%d)", partido, fichas[0]["name"],
                 goles[0], goles[1], fichas[1]["name"], viejos[0], estados[0]["rating"], viejos[1],
                 estados[1]["rating"])
        self._empujar_cambio_rating(pareja, fichas, cambios, goles)

    def _empujar_cambio_rating(self, pareja, fichas, cambios, goles):
        """L (A.3): CMD_WATCH_GAMEADDPOINT NO lo pide el cliente: se EMPUJA (rqid 0) una vez a cada
        participante que siga conectado; es lo que arma la pantalla «Cambios en el rating»."""
        ganador = "WINNER_HOME" if goles[0] > goles[1] else "WINNER_AWAY" if goles[1] > goles[0] else "WINNER_NONE"
        lista = []
        for f, (res, sp, viejo, e), lado in zip(fichas, cambios, ("HOME", "AWAY")):
            lista.append(Empaquetado({
                "pid": f["pid"],
                "pes_pname": Empaquetado({"name": f["name"], "xuid_one": "NotImplement",
                                          "psn_user_id": Empaquetado({"account_id": 0, "online_id": f["name"],
                                                                      "is_valid_cache": "NO"}),
                                          "is_deleted": "NO"}),
                "rating_type": "RANKMATCH", "rating": e["rating"], "oldrating": viejo,
                "rating_boost_type": "", "rating_boost_rate": 0, "rating_compe": 0,
                "team_id": f["team"], "side": lado, "contWin_num": e["racha"],
                "skill_rank": "SKILL_RANK_NONE", "is_rankup_skill_rank": "NO", "is_first_skillrank": "NO"}))
        for x in pareja["miembros"]:
            if x in self.sesiones:
                x.enviar({"msgid": "CMD_WATCH_GAMEADDPOINT", "rqid": 0, "result": NOERR,
                          "player_count": len(lista), "player_list": lista, "winner_side": ganador,
                          "gameend_reason": "NORMAL_END", "is_pk": "NO"})

    async def c_cmd_join_online_mode(self, s, m):
        s.modo_online = m.get("online_mode") if isinstance(m.get("online_mode"), str) else None
        return {}

    async def c_cmd_leave_online_mode(self, s, m):
        s.modo_online = None
        return {}

    async def c_cmd_get_server_env(self, s, m):
        # C (PESBUL R7.3.169/631): nombres probados en Ghidra; valores de cuenta nueva.
        # max_division es el TOKEN "D12" (el entero o "10" da VNBP_0002); is_revision_check 0.
        # C (PESBUL 04-nov): proto_opt es una CADENA JSON compacta con el perfil de red
        # (por defecto «MatchCommandDropMode 1»); antes mandábamos el entero 0
        proto = self.cfg.get("proto_opt",
                             {"CS": {"MatchCommandBufferingControl": {"MatchCommandDropMode": 1}}})
        if isinstance(proto, dict) and s.modo_online == "RANKMATCH":
            # L (PESBUL): solo en Divisiones, margen de 60 s antes de dar el partido por perdido
            margen = {"Match": {"ActualMatchHzFatalConditionDeterminationTimeUs": 60000000}}
            proto = {**proto, **{k: margen for k in ("P2P", "P2P_HIGH_LEVEL", "P2P_LOW_LEVEL",
                                                       "P2P_GIVE_UP_QUICKLY_LEVEL")}}
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

    async def c_cmd_get_match_analyze(self, s, m):
        """L (ESPEC_PARTIDO_REAL_PESBUL §2): antes del partido el PES pide el análisis del rival
        (target_pid). El cliente lee analyze_data_list y analyze_data_size; con el sobre vacío el
        menú rebota. Sin historial guardado: lista vacía y DRAW (lo mismo que un jugador nuevo)."""
        log.info("MATCH_ANALYZE pedido: tipo=%r objetivo=%r", m.get("analyze_data_type"), m.get("target_pid"))
        return {"analyze_data_size": 0, "analyze_data_list": [], "game_res": "DRAW"}

    c_cmd_get_match_analyze_byid = c_cmd_get_match_analyze

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

    # ---------- Identidad provisional por persona (docs/PLAN_MULTIUSUARIO.md §1) ----------
    # C (02/04-nov): todas las copias del mismo PES mandan el MISMO SteamID (CPY 0x3a660 = 239200;
    # el PES base, 0x0410000004100000). Sin esto, todos los amigos caerían en el mismo club.
    STEAM_COMPARTIDOS = ("000000000003a660", "0410000004100000")

    def clave_identidad(self, steam_hex, ip_cliente=None, ip_local=None):
        """Clave con la que se guarda a la persona (jugador, perfil y club).

        - SteamID propio (no compartido): ese mismo.
        - SteamID compartido: si la IP está en identidad.por_ip, la clave que diga ahí; si el
          cliente está en la MISMA PC que el servidor (anfitrión), el SteamID tal cual (así el
          club de Frank no cambia); si no, una clave fija derivada de su IP («7f» + IP en hex).
          Con Tailscale/ZeroTier cada amigo tiene IP fija, así que conserva su club.
        - Pase de la web (POST /pase): la IP queda ligada a la cuenta Phoenix y la clave sale de
          la cuenta («7d» + sha1 del usuario), así la persona conserva su club aunque cambie de PC.
          No se aplica a la PC del servidor (el club de Frank no cambia)."""
        steam_hex = (steam_hex or "").lower()
        ide = self.cfg.get("identidad") or {}
        if not ide.get("activo", True) or not steam_hex:
            return steam_hex
        compartidos = {str(x).lower().zfill(16) for x in
                       ide.get("steam_compartidos", self.STEAM_COMPARTIDOS)}
        if steam_hex not in compartidos or not ip_cliente:
            return steam_hex
        ip = ip_cliente.split("%")[0].removeprefix("::ffff:")
        fija = (ide.get("por_ip") or {}).get(ip)
        if fija:
            return str(fija).lower()
        if ip.startswith("127.") or ip == "::1" or (ip_local and ip == ip_local.removeprefix("::ffff:")) \
                or ip in (ide.get("ips_anfitrion") or []):
            return steam_hex
        # pase de la web: esa IP quedó ligada a una cuenta Phoenix (manda sobre la clave por IP)
        por_pase = self.datos.clave_por_ip(ip) if hasattr(self.datos, "clave_por_ip") else None
        if por_pase:
            return por_pase
        partes = ip.split(".")
        if len(partes) == 4 and all(p.isdigit() for p in partes):
            return "7f" + "".join(f"{int(p):02x}" for p in partes) + "000000"
        return "7e" + hashlib.sha1(ip.encode()).hexdigest()[:14]

    @staticmethod
    def clave_de_usuario(usuario):
        return "7d" + hashlib.sha1(str(usuario).encode()).hexdigest()[:14]

    async def canjear_pase(self, pase, ip_cliente, ip_local=None):
        """POST /pase {"pase": "..."} desde Link (o a mano): liga esa IP a la cuenta del pase.
        Devuelve (código HTTP, {ok, mensaje}). Nunca guarda ni registra el pase en claro."""
        ip = (ip_cliente or "").split("%")[0].removeprefix("::ffff:")
        pase = str(pase or "").strip().lower()
        if self.nube is None:
            return 503, {"ok": False, "mensaje": "el servidor no está conectado a la web (Supabase)"}
        if not (16 <= len(pase) <= 128) or any(c not in "0123456789abcdef" for c in pase):
            return 400, {"ok": False, "mensaje": "pase con formato raro"}
        if ip.startswith("127.") or ip == "::1" or (ip_local and ip == ip_local.removeprefix("::ffff:")):
            return 400, {"ok": False, "mensaje": "la PC del servidor no usa pase (conserva su club)"}
        ahora = time.time()
        intentos = [t for t in self.intentos_pase.get(ip, []) if ahora - t < 600]
        if len(intentos) >= 10:
            return 429, {"ok": False, "mensaje": "demasiados intentos; espera 10 minutos"}
        self.intentos_pase[ip] = intentos + [ahora]
        try:
            usuario = await self.nube.canjear_pase(pase)
        except Exception as e:
            log.warning("pase: Supabase no respondió (%s)", e)
            return 502, {"ok": False, "mensaje": "la web no respondió; prueba otra vez"}
        if not usuario:
            log.info("pase rechazado desde %s (no existe, usado o caducado)", ip)
            return 403, {"ok": False, "mensaje": "pase no válido, ya usado o caducado (dura 10 min)"}
        clave = self.clave_de_usuario(usuario)
        self.datos.guardar_pase(ip, usuario, clave)
        log.info("pase aceptado: %s queda ligada a la cuenta %s (clave %s)", ip, usuario, clave)
        return 200, {"ok": True, "mensaje": "listo: entra al online y tendrás tu propio club", "clave": clave}

    async def entrar_launcher(self, token, ip_cliente, ip_local=None):
        """POST /entrar {"token": sesión de la web} desde Phoenix Launcher (botón «Jugar Online»).
        Supabase confirma la sesión; esa PC queda ligada a la cuenta (su club va con su cuenta) y
        presentada (para exigir_launcher). El token no se guarda ni se registra."""
        ip = (ip_cliente or "").split("%")[0].removeprefix("::ffff:")
        token = str(token or "").strip()
        if self.nube is None:
            return 503, {"ok": False, "mensaje": "el servidor no está conectado a la web (Supabase)"}
        if not (20 <= len(token) <= 4096) or token.count(".") != 2:
            return 400, {"ok": False, "mensaje": "sesión con formato raro"}
        ahora = time.time()
        intentos = [t for t in self.intentos_pase.get(ip, []) if ahora - t < 600]
        if len(intentos) >= 30:
            return 429, {"ok": False, "mensaje": "demasiados intentos; espera 10 minutos"}
        self.intentos_pase[ip] = intentos + [ahora]
        try:
            usuario = await self.nube.usuario_de_token(token)
        except Exception as e:
            log.warning("launcher: Supabase no respondió (%s)", e)
            return 502, {"ok": False, "mensaje": "la web no respondió; prueba otra vez"}
        if not usuario:
            return 403, {"ok": False, "mensaje": "tu sesión de la web caducó: vuelve a iniciar sesión"}
        anfitrion = ip.startswith("127.") or ip == "::1" or (ip_local and ip == ip_local.removeprefix("::ffff:"))
        self.datos.presentar(ip, usuario)
        clave = None
        if not anfitrion:  # la PC del servidor conserva su club
            clave = self.clave_de_usuario(usuario)
            self.datos.guardar_pase(ip, usuario, clave)
        log.info("launcher: %s presentado con la cuenta %s%s", ip, usuario, "" if clave else " (anfitrión)")
        return 200, {"ok": True, "mensaje": "listo: abre el PES y entra al online", "clave": clave}

    def _sin_launcher(self, s):
        """identidad.exigir_launcher: solo entra al online quien abrió el PES con el launcher
        (en las últimas N horas). La PC del servidor queda exenta. El offline nunca se ve afectado."""
        ide = self.cfg.get("identidad") or {}
        if not ide.get("exigir_launcher"):
            return False
        ip = (s.ip or "").split("%")[0].removeprefix("::ffff:")
        if ip.startswith("127.") or ip == "::1" or (s.ip_local and ip == s.ip_local.removeprefix("::ffff:")):
            return False
        return not self.datos.presentado(ip, float(ide.get("launcher_horas", 12)))

    def _clave_de(self, s, m):
        crudo = self._steam_de(m)
        if crudo is None:
            return s.steam_hex
        s.steam_crudo = crudo.lower()
        return self.clave_identidad(crudo, s.ip, s.ip_local)

    def en_mantenimiento(self):
        """Archivo MANTENIMIENTO.txt (o el de la config) presente = nadie nuevo entra; los que
        ya están siguen. Sirve para actualizar el servidor sin cortar un partido."""
        ruta = self.cfg.get("archivo_mantenimiento", "MANTENIMIENTO.txt")
        return bool(ruta) and os.path.exists(ruta)

    def _version_distinta(self, m):
        """exigir_versiones: {"patch_version": "1.07.02", ...}. Una copia con otro parche/BD
        daría plantillas o partidos desincronizados: mejor un aviso claro al entrar."""
        for k, quiero in (self.cfg.get("exigir_versiones") or {}).items():
            v = m.get(k)
            if v is not None and str(v) != str(quiero):
                return k, v, quiero
        return None

    def _entrar(self, s, m):
        steam_hex = self._clave_de(s, m) or s.steam_hex or ""
        if self.en_mantenimiento():
            log.info("login rechazado: servidor en mantenimiento (%s)", steam_hex)
            return {"result": "ERR_SERVERNOTREADY"}
        if self._sin_launcher(s):
            log.info("login rechazado: %s no abrió el PES con Phoenix Launcher", s.ip)
            return {"result": "ERR_SERVERNOTREADY"}
        distinta = self._version_distinta(m)
        if distinta:
            log.warning("login rechazado: %s=%r y el servidor exige %r (%s)", *distinta, steam_hex)
            return {"result": "ERR_DIFFERENT_PATCHVERSION"}
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
        s.steam_hex = self._clave_de(s, m)
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
        forzado = self._pid_forzado(s)
        return forzado or self._numero_cuenta(s) or s.pid

    def _pid_forzado(self, s):
        """identidad.pid_forzado {"IP": "hex de 16"}: SOLO para la prueba que decide de dónde saca el
        PES el pid que manda en CMD_REVISION_CHECK (del SteamID propio o del pid que le da el servidor)."""
        tabla = (self.cfg.get("identidad") or {}).get("pid_forzado") or {}
        ip = (getattr(s, "ip", None) or "").split("%")[0].removeprefix("::ffff:")
        v = tabla.get(ip)
        try:
            return int(str(v), 16) if v else 0
        except ValueError:
            return 0

    # Claves que inventa el servidor (no son SteamID): «7f»+IP, «7e»+sha1(IP), «7d»+sha1(cuenta).
    PREFIJOS_DERIVADOS = ("7d", "7e", "7f")
    STEAM_BASE = 0x0110000100000000  # prefijo de una cuenta Steam individual (32 bits altos 0x01100001)
    PIDS_NATIVOS_COMPARTIDOS = (0x04100000, 0x0003A660)

    @classmethod
    def numero_de_clave(cls, clave):
        """Número de cuenta (pid de 64 bits) que ven los PES para una clave del servidor.

        C (prueba 6e, 10-nov): en CMD_REVISION_CHECK cada PES manda como pid los 32 bits BAJOS de su
        número, y el otro PES lo compara con los 32 bits bajos del pid que le dimos en la lista de
        FINISH. Las claves derivadas «7f»+IP+«000000» tenían los 32 bits bajos casi iguales para
        todos (IP terminada en el mismo número = mismo pid): ahora salen de un resumen de la clave,
        distintos entre amigos y distintos de los SteamID compartidos. La clave guardada no cambia."""
        clave = (clave or "").lower()
        try:
            n = int(clave or "0", 16)
        except ValueError:
            return 0
        if len(clave) == 16 and clave[:2] in cls.PREFIJOS_DERIVADOS:
            bajo = int.from_bytes(hashlib.sha1(clave.encode()).digest()[:4], "big") & 0x7FFFFFFF
            if bajo == 0 or bajo in cls.PIDS_NATIVOS_COMPARTIDOS:
                bajo = (bajo ^ 0x5A5A5A5A) & 0x7FFFFFFF or 1
            return cls.STEAM_BASE | bajo
        return n

    @classmethod
    def _numero_cuenta(cls, s):
        return cls.numero_de_clave(s.steam_hex)

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
        # L (FASE23 §1.1): el UDP del relé no trae pid; se reconoce por estas direcciones
        if s.steam_hex:
            self.ntl_por_clave[s.steam_hex] = {**s.ntl, "ip_tcp": s.ip}
        hashes = m.get("file_hash_list")
        if hashes is not None:
            crudo = repr(hashes).encode()
            s.huella = (len(hashes) if hasattr(hashes, "__len__") else 0,
                        hashlib.sha256(crudo).hexdigest())
            if s.steam_hex:
                self.huella_por_clave[s.steam_hex] = s.huella
            log.info("huella de archivos de %s: %d archivos, %s", s.pid, s.huella[0], s.huella[1][:12])
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

    def _pid_en_partida(self, s):
        """pid de la ficha de FINISH: los 32 bits bajos son lo que el OTRO PES exigirá en el
        CMD_REVISION_CHECK. identidad.pid_en_partida = "clave" (por defecto: el número que el
        servidor da a esa persona) o "nativo" (el SteamID tal cual lo mandó su PES)."""
        modo = (self.cfg.get("identidad") or {}).get("pid_en_partida", "clave")
        if modo == "nativo" and s.steam_crudo and not self._pid_forzado(s):
            try:
                return int(s.steam_crudo, 16)
            except ValueError:
                pass
        return self._pid_visible(s) or 0

    def _miembro(self, s, orden, dueno, lado=None):
        """Una ficha del roster de CMD_WATCH_MATCHING (bin con MessagePack dentro)."""
        ident = self._identidad(s, s.pname or "Phoenix")
        f = {"matching_order": orden, "is_owner": "YES" if dueno else "NO",
             "pid": self._pid_en_partida(s), "pname": Empaquetado(ident),
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

    # ---------- Partida de 2 por el relé (ESPEC_GAMERELAY_FASE23 §1.5) ----------
    def _endpoints(self, clave, sesiones=()):
        """Direcciones UDP declaradas por una persona: reflexiva y host (la TURN no cuenta)."""
        from .gamerelay import endpoint_de_ntl
        decl = dict(self.ntl_por_clave.get(clave) or {})
        for x in sesiones:
            if x.ntl:
                decl.update(x.ntl)
        return [e for e in (endpoint_de_ntl(decl.get("reflexive_addr")),
                            endpoint_de_ntl(decl.get("host_addr"))) if e]

    def _abrir_partida(self, pareja):
        """Sala sintética de la pareja (anfitrión enter_no 0, invitado 1) y aviso al relé."""
        anfitrion, invitado = pareja["miembros"]
        room_id = 700_000_000 + next(self._partidas)
        pareja["room_id"] = room_id
        pareja["claves"] = [self._clave_partida(anfitrion), self._clave_partida(invitado)]
        if self.rele is None or not hasattr(self.rele, "abrir_intento"):
            return room_id
        miembros = []
        for enter_no, x in enumerate((anfitrion, invitado)):
            clave = self._clave_partida(x)
            miembros.append({"pid": clave, "enter_no": enter_no, "ip_tcp": x.ip,
                             "endpoints": (lambda c=clave, ses=x: self._endpoints(c, (ses,)))})
        self.rele.abrir_intento(room_id, miembros)
        return room_id

    def _huella(self, x):
        return x.huella or self.huella_por_clave.get(x.steam_hex or "")

    def _misma_huella(self, a, b):
        # apagado hasta comprobar con 2 PES iguales que su huella coincide (si no, nunca se emparejarían)
        if not self.cfg.get("emparejar_misma_huella", False):
            return True
        ha, hb = self._huella(a), self._huella(b)
        if ha is None or hb is None:
            return True  # sin dato no se bloquea (se registra para aprender)
        if ha != hb:
            log.info("no se empareja %s con %s: archivos del juego distintos", a.pid, b.pid)
        return ha == hb

    def _clave_partida(self, x):
        return x.steam_hex or f"sesion{id(x)}"

    def _dejar_pareja(self, s, motivo=""):
        pareja = self.parejas.pop(s, None)
        if not pareja:
            return
        room_id = pareja.get("room_id")
        if room_id and self.rele is not None and hasattr(self.rele, "miembro_salio"):
            self.rele.miembro_salio(room_id, self._clave_partida(s))
        if motivo:
            log.info("pareja %s: %s se va (%s)", room_id, s.pid, motivo)

    def _pareja_por_clave(self, s):
        p = self.parejas.get(s)
        if p:
            return p
        clave = s.steam_hex
        for x, p in self.parejas.items():
            if clave and x.steam_hex == clave:
                return p
        return None

    async def c_cmd_send_game_sequence_data(self, s, m):
        """L (FASE23 §3.4): copia TCP de los mensajes de 10/12/15. Respuesta NOERR siempre; el
        reenvío al otro jugador es efecto lateral (deduplicado con la copia UDP)."""
        if self.rele is None or not hasattr(self.rele, "reenviar_tcp"):
            return {}
        for r in m.get("game_seq_data") or []:
            try:
                if isinstance(r, (bytes, bytearray)):
                    r = msgpack_min.unpackb(bytes(r))
                if not isinstance(r, dict) or int(r.get("type", 0)) != 2:
                    continue
                canal = int(r.get("channelId", -1))
                if canal not in (10, 12, 15):
                    continue
                d = r.get("data")
                crudo = base64.b64decode(d + "=" * (-len(d) % 4)) if isinstance(d, str) else bytes(d or b"")
                if len(crudo) < 9:
                    continue
                largo = int.from_bytes(crudo[7:9], "big")
                mensaje = crudo[9:9 + largo]
                # diagnóstico (10-nov): los primeros registros de cada persona, completos
                s.gameseq_vistos += 1
                if s.gameseq_vistos <= 80:
                    log.info("GameSeq TCP de %s: canal %s fromId %s seq %s cab %s msg %s", s.pid, canal,
                             r.get("fromId"), r.get("sequenceNumber"), crudo[:9].hex(), mensaje.hex())
                if mensaje:
                    self.rele.reenviar_tcp(self._clave_partida(s), canal, mensaje, r.get("fromId"))
            except Exception:
                log.exception("GameSeq TCP ilegible de %s", s.pid)
        return {}

    async def c_cmd_start_matching(self, s, m):
        tipo = str(m.get("matching_type") or "OC_QUICK")
        self._salir_de_cola(s)
        self._dejar_pareja(s, "busca otra partida")
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        cola = self.colas.setdefault(tipo, [])
        # Parche: solo se empareja a quien tiene LOS MISMOS archivos (file_hash_list de NTL);
        # con parches distintos el partido se desincroniza o el juego se cierra
        rival = next((x for x in cola if x is not s and x in self.sesiones
                      and self._misma_huella(s, x)), None)
        if rival is None:
            cola.append(s)
            await self.dormir(PAUSA_EMPUJE)
            if s in cola:  # tarjeta propia en el panel LOCAL mientras busca (r7.3.647)
                self._vigilar_emparejamiento(s, "SEARCHING", [self._miembro(s, 0, False)])
            return None
        cola.remove(rival)
        # el que esperaba es anfitrión/LOCAL; el que completa la pareja, VISITANTE (r7.3.644/650)
        miembros = [self._miembro(rival, 0, True, "HOME"), self._miembro(s, 1, False, "AWAY")]
        pareja = {"miembros": (rival, s), "check": set(), "tipo": tipo}
        self.parejas[rival] = self.parejas[s] = pareja
        self._abrir_partida(pareja)
        await self.dormir(PAUSA_EMPUJE)
        for x in (rival, s):
            self._vigilar_emparejamiento(x, "FINISH", miembros)
        log.info("pareja de %s: %s vs %s", tipo, rival.pid, s.pid)
        return None

    async def c_cmd_cancel_matching(self, s, m):
        self._salir_de_cola(s)
        self._dejar_pareja(s, "canceló")
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        # C (r7.3.648): la pantalla de búsqueda solo se cierra con el empuje CANCEL
        self._vigilar_emparejamiento(s, "CANCEL", [])
        return None

    async def c_cmd_set_gamerelay_check_res(self, s, m):
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        pareja = self._pareja_por_clave(s)
        if pareja is None:
            return None
        if m.get("error_code") not in (None, NOERR, "") or int(m.get("failed_reason") or 0):
            log.warning("chequeo de conexión con error %s (motivo %s) de %s", m.get("error_code"),
                        m.get("failed_reason"), s.pid)
            return None
        pareja["check"].add(self._clave_partida(s))
        # P (política PESBUL): se libera cuando los dos informaron; el cliente espera 30 s máx.
        if len(pareja["check"]) == 2 and not pareja.get("check_enviado"):
            pareja["check_enviado"] = True  # una sola vez por partida
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
        nodos = self._nodos_relay(s)
        indice = nodos[0]["gamerelay_index"] if nodos else 0
        return {"lobby_list_num": len(lobbies),
                "lobby_list": [Empaquetado({**l, "player_num": len(self.por_pid),
                                            "gamerelay_index": l.get("gamerelay_index") or indice})
                               for l in lobbies]}

    async def c_cmd_get_lobbyinfo(self, s, m):
        return {"has_info": True, "title": "Phoenix Online",
                "sentence_info": self.cfg.get("lobby_info", ""), "sentence_intro": ""}

    def _url_gamerelay_web(self, s=None):
        # C (PESBUL 04-nov): JOIN_LOBBY lleva http://<IP>:5000/gamerelay; vacío deja al cliente
        # en XXJA_0005 / «no se reciben datos» en el vestíbulo
        u = self._con_ip(self.cfg["urls"].get("GAMERELAY_WEB"), s)
        if u:
            return u
        base = self._con_ip(self.cfg.get("http_base"), s) or \
            f"http://{self.ip_anuncio(s)}:{self.cfg.get('puerto_http', 5000)}"
        return base.rstrip("/") + "/gamerelay"

    async def c_cmd_get_blocklist(self, s, m):
        # C (PESBUL R7.3.183): respuesta pelada = XXJA_0005 y el cliente sale del vestíbulo;
        # lista vacía tampoco sirve. Un bloque «Main» como bin.
        bloque = int(m.get("lobby_id") or getattr(s, "lobby_id", None) or 1)
        return {"count": 1, "bklist": [Empaquetado({"name": "Main", "player_num": len(self.por_pid),
                                                     "max_player_num": 100, "block_id": max(1, bloque)})]}

    async def c_cmd_join_lobby(self, s, m):
        s.lobby_id = m.get("lobby_id") or 1
        return {"gamerelay_web_url": self._url_gamerelay_web(s), "bklist": []}

    async def c_cmd_join_block(self, s, m):
        # L (PESBUL): tras GET_BLOCKLIST el cliente entra al bloque; sin respuesta = C_RL_JS_0002
        try:
            s.block_id = int(m.get("block_id") or 0)
        except (TypeError, ValueError):
            s.block_id = 0
        return {}

    async def c_cmd_leave_block(self, s, m):
        # L: salir del bloque también saca de la sala
        await self._salir_de_sala(s)
        s.block_id = None
        return {}

    async def c_cmd_leave_lobby(self, s, m):
        await self._salir_de_sala(s)
        s.block_id = s.lobby_id = None
        return {}

    async def c_cmd_check_string(self, s, m):
        # L (PESBUL R7.3.341): campo «str»; sin respuesta el cliente espera para siempre
        texto = m.get("str") if isinstance(m.get("str"), str) else ""
        mal = any(p in texto.upper() for p in PALABRAS_PROHIBIDAS)
        return {"result": "ERR_NGWORD"} if mal else {}

    # ---------- Perfil y registros ----------
    async def c_cmd_set_playerprofile(self, s, m):
        # L (PESBUL R7.3.130.3): «profile» = bin(msgpack) con favorite_team/favorite_player/intro
        perfil = m.get("profile")
        if isinstance(perfil, (bytes, bytearray)):
            try:
                perfil = msgpack_min.unpackb(perfil)
            except Exception:
                perfil = {}
        perfil = perfil if isinstance(perfil, dict) else {}
        def campo(k, n):
            v = perfil.get(k)
            return v[:n] if isinstance(v, str) else ""
        self.datos.guardar_perfil(s.steam_hex or "", campo("favorite_team", 100),
                                  campo("favorite_player", 100), campo("intro", 512))
        return {"error_favorite_team": "", "error_favorite_player": "", "error_intro": ""}

    def _sesion_de_pid(self, pid):
        for x in self.sesiones:
            if pid and str(self._pid_visible(x)) == str(pid):
                return x
        return None

    async def c_cmd_get_playerinfo(self, s, m):
        # L (PESBUL R7.3.141/636): pid + pname(bin) + favoriteTeam/favoritePlayer/intro ARRIBA
        # (si faltan, «Editar perfil» muestra NotImplement); rankmatch_stats y manner van como
        # bin; division es TEXTO, el resto números
        destino = self._sesion_de_pid(m.get("pid")) or s
        j = self.datos.jugador(destino.steam_hex or "") or {}
        nombre = j.get("pname") or destino.pname or ""
        perfil = self.datos.perfil(destino.steam_hex or "")
        cero = {"match_num": 0, "win_num": 0, "lose_num": 0, "draw_num": 0, "goal": 0,
                "lost_goal": 0, "rating": RATING_INICIAL, "division": 0, "acclaim_level": 1}
        pname = {**self._identidad(destino, nombre), "account_id": self._numero_cuenta(destino),
                 "online_id": nombre, "is_valid_cache": "1", "disconnect_rate": 0,
                 "favoriteTeam": perfil["favorite_team"],
                 "favoritePlayer": perfil["favorite_player"], "intro": perfil["intro"],
                 "regist_team_info": {"team_id": 0, "team_name": "", "team_type": 0},
                 "myclub_stats": {**cero, "gp": 0, "coins": 0},
                 "matchday_stats_single": dict(cero), "matchday_stats_coop": dict(cero),
                 "coop_profile_data": {"level": 1, "exp": 0, "rating": RATING_INICIAL,
                                       "match_num": 0, "win_num": 0, "lose_num": 0,
                                       "draw_num": 0},
                 "match_data_size": 0, "match_data": b""}
        e = self._estado_division(destino.steam_hex)
        partidos = sum(a["jugados"] for a in e["archivadas"]) + e["jugados"]
        estadisticas = {"division": e["division"], "rating": e["rating"],
                        "max_weekly_point": 0, "current_weekly_point": 0, "match_num": partidos,
                        "win_num": e["win"] + sum(a["win"] for a in e["archivadas"]),
                        "lose_num": e["loss"] + sum(a["loss"] for a in e["archivadas"]),
                        "draw_num": e["draw"] + sum(a["draw"] for a in e["archivadas"]),
                        "contWin_num": e["racha"], "contWinMax_num": e["racha_max"],
                        "goal": e["goles"], "lostGoal": e["goles_contra"]}
        return {"pid": self._pid_visible(destino), "pname": Empaquetado(pname),
                "favoriteTeam": perfil["favorite_team"],
                "favoritePlayer": perfil["favorite_player"], "intro": perfil["intro"],
                "rankmatch_stats": Empaquetado(estadisticas),
                "manner": Empaquetado({"manner": MODALES_DEFECTO}),
                "rankmatch_teamLog": []}

    async def c_cmd_get_often_used_team_list(self, s, m):
        # L (PESBUL, parser 0x1D46940): ids de equipo SIMPLES (el cliente hace id << 14)
        return {"last_used_team_id": 0, "used_team_id_list": [], "used_team_id_list_num": 0}

    async def c_cmd_get_division_point_info(self, s, m):
        # L (PESBUL r7.3.630): la tabla es POSICIONAL: siempre 12 filas, D12 -> D1, como bin
        e = self._estado_division(s.steam_hex)
        return {"server_num": 1, "division_list": [Empaquetado(f) for f in div.filas_carrera(e)]}

    async def c_cmd_get_gameresults(self, s, m):
        # L (PESBUL r7.3.629): sin historial = no_match «YES»; un elemento mal formado cierra PES
        # L (A.6, confirmado en PESBUL 10-nov): todo bin salvo «team» (enteros). Una clave en el nivel
        # equivocado cerró el PES en PESBUL: divisiones.historial_partidos = false lo apaga.
        if not (self.cfg.get("divisiones") or {}).get("historial_partidos", True):
            return {"rlist_num": 0, "rlist": [], "no_match": "YES"}
        filas = []
        for partido in self._estado_division(s.steam_hex)["resultados"][:div.MAX_RESULTADOS]:
            p = self.datos.partido_division(partido)
            if p and not p.get("no_cuenta"):
                filas.append(self._fila_resultado(p))
        # L/C: la respuesta entera en UNA trama (medida ya comprimida, como la manda nclmio): el envío
        # troceado hacia el cliente aún no está probado en el PES. Se quitan los más viejos.
        while filas and _cifrado_comprimido({"msgid": m.get("msgid", "CMD_GET_GAMERESULTS"),
                                             "rqid": m.get("rqid", 0), "result": NOERR,
                                             "rlist_num": len(filas), "rlist": filas}) > MAX_CIFRADO_CLIENTE:
            filas.pop()
        if not filas:
            return {"rlist_num": 0, "rlist": [], "no_match": "YES"}
        return {"rlist_num": len(filas), "rlist": filas}

    @staticmethod
    def _fila_resultado(p):
        def marcador(g):
            return Empaquetado({"score_1st": int(g), "score_2nd": 0, "score_ex1st": 0, "score_ex2nd": 0,
                                "score_pk": 0})

        def jugador(f, lado):
            return Empaquetado({"pname": Empaquetado({"name": f["name"], "xuid_one": "NotImplement",
                                                      "psn_user_id": Empaquetado({"account_id": 0,
                                                                                  "online_id": f["name"],
                                                                                  "is_valid_cache": "NO"}),
                                                      "is_deleted": "NO"}),
                                "pid": f["pid"], "side": lado, "is_com": "NO", "cpu_level": "0",
                                "is_follow": "NO", "is_guest": "NO"})
        casa, fuera = p["home"], p["away"]
        return Empaquetado({"date": int(p["date"]), "match_category": "ONLINE_DIVISIONS", "ex_type": "NONE",
                            "gameend_reason": p.get("reason") or "NORMAL_END", "pk_flag": "NO",
                            "score": [marcador(casa["score"]), marcador(fuera["score"])],
                            "player": [jugador(casa, "HOME"), jugador(fuera, "AWAY")],
                            "team": [int(casa["team"]), int(fuera["team"])]})

    async def c_cmd_get_ranking(self, s, m):
        return self._ranking_division(m)

    def _ranking_division(self, m):
        # L (PESBUL r728): elementos como MAPAS (no bin); ordenados por rating y luego puntos
        division = m.get("division_type") or m.get("division")
        estados = self.datos.divisiones()
        pide = str(m.get("pid") or "")
        filas = []
        for f in self.datos.todos():
            clave = (f["steam_hex"] or "").lower()
            e = div.completar(estados.get(clave))
            n = self.numero_de_clave(clave)
            # C (prueba 10-nov): con todos los registrados salían filas repetidas (cuentas viejas de
            # pruebas con el mismo nombre): solo quien jugó Divisiones, y siempre quien pregunta
            jugo = clave in estados and (e["jugados"] or e["archivadas"])
            es_quien_pide = pide and pide in (str(n), str(n & 0xFFFFFFFF), str(f["pid"]))
            if (jugo or es_quien_pide) and (division in (None, "") or e["division"] == division):
                filas.append((f, e))
        # L (A.7): rating de mayor a menor, luego SP, luego pid
        filas.sort(key=lambda fe: (-fe[1]["rating"], -fe[1]["sp"], int(fe[0]["pid"] or 0)))
        try:
            inicio = max(0, int(m.get("start_index") or 0))
            cuantos = min(100, max(0, int(m.get("view_num") or 50)))
        except (TypeError, ValueError):
            inicio, cuantos = 0, 50
        lista = []
        for i, (f, e) in enumerate(filas[inicio:inicio + cuantos]):
            perfil = self.datos.perfil(f["steam_hex"])
            cuenta_pid = self.numero_de_clave(f["steam_hex"]) or f["pid"]
            lista.append({"rank": inicio + i + 1, "point": e["sp"], "user": {
                "pid": cuenta_pid,
                "pname": {"name": f["pname"], "xuid_one": "NotImplement",
                          "psn_user_id": {"account_id": 0, "online_id": f["pname"],
                                          "is_valid_cache": "NO"},
                          "is_deleted": "NO"},
                "division": e["division"], "manner": MODALES_DEFECTO,
                "favoriteTeam": perfil["favorite_team"],
                "favoritePlayer": perfil["favorite_player"], "intro": perfil["intro"],
                "match_record": {"win_count": e["win"], "draw_count": e["draw"], "lose_count": e["loss"],
                                 "win_s_max": e["racha_max"], "rating": e["rating"]}}})
        return {"date": int(time.time()), "total_rank_num": len(filas), "rank_list": lista}

    c_cmd_get_ranking_byid = c_cmd_get_ranking
    c_cmd_get_division_ranking = c_cmd_get_ranking
    c_cmd_get_division_ranking_byid = c_cmd_get_ranking

    # ---------- myClub (docs/ESPEC_MYCLUB.md, leído del exe) ----------
    # C (§12): todo objeto anidado y todo elemento de lista de objetos = bin(msgpack); las listas
    # de enteros van en línea. Un mapa en línea donde el lector espera bin = fallo de memoria.
    # P: progress_status es un enum aún sin recuperar; se prueba desde config.json
    # («myclub»: {"progreso_sin_club": "...", "progreso_con_club": "..."}) sin tocar código.
    MYCLUB_GP_INICIAL = 0

    def _cfg_myclub(self):
        c = self.cfg.get("myclub") or {}
        return {"sin": c.get("progreso_sin_club", ""), "con": c.get("progreso_con_club", ""),
                "gp": int(c.get("gp_inicial", self.MYCLUB_GP_INICIAL))}

    CONCEPTO_JUGADOR = ("transition_counter", "transition_possession", "build_up_long",
                        "build_up_short", "attacking_area_side", "attacking_area_center",
                        "mobility_fix", "mobility_float", "defence_type_forecheck",
                        "defence_type_retreat", "press_rule_net_defence", "press_rule_seal_off",
                        "first_df_aggressive", "first_df_safety")
    CONCEPTO_DT = ("transition", "build_up", "attacking_area", "mobility", "defence_type",
                   "press_rule", "first_df", "passsupport_dist", "offence_num", "defence_line",
                   "defence_compact", "defence_num")

    @staticmethod
    def _rareza(j):
        """C (§18): token CON prefijo; «WHITE» suelto = NotImplement = jugador inelegible («!»).
        P: cortes de valoración de myClub (negro ≥85, oro 80, plata 75, bronce 70)."""
        r = j.get("default_rarity")
        if isinstance(r, str) and r.startswith("PLAYER_RARITY_"):
            return r
        v = j.get("rating") if isinstance(j.get("rating"), int) else 75
        for corte, nombre in ((85, "BLACK"), (80, "GOLD"), (75, "SILVER"), (70, "BRONZE")):
            if v >= corte:
                return "PLAYER_RARITY_" + nombre
        return "PLAYER_RARITY_WHITE"

    def _plantel_myclub(self, s):
        """Jugadores, entrenadores y plantillas del club: lo guardado o, si no hay, el de config
        (myclub.jugadores / myclub.entrenadores). C (§14): con 0 jugadores el cliente sale."""
        cfg = self.cfg.get("myclub") or {}
        club = self._club_inicial(s) or {}
        jugadores = club.get("jugadores") or cfg.get("jugadores") or []
        dts = club.get("entrenadores") or cfg.get("entrenadores") or []
        return [j for j in jugadores if isinstance(j, dict)], [d for d in dts if isinstance(d, dict)]

    EXP_POR_NIVEL = 100  # P: curva simple hasta conocer la real
    # C (REA 05-nov noche, 0x140d36fe0): PES muestra contract_term / 90 (entero) como «partidos»
    # y exige >= 90 para jugar. P: la unidad son minutos (un partido = 90). En la base se guarda
    # ya en esa unidad; la config habla siempre en PARTIDOS.
    MIN_POR_PARTIDO = 90

    def _partidos_a_unidad(self, partidos):
        return int(partidos) * self.MIN_POR_PARTIDO

    def _contrato_inicial(self):
        return self._partidos_a_unidad((self.cfg.get("myclub") or {}).get("contrato_inicial", 10))

    def _contrato(self, j):
        v = j.get("contract_term")
        return v if isinstance(v, int) else self._contrato_inicial()

    def _migrar_contratos(self, club):
        """Clubes guardados antes del 05-nov guardaban contract_term en «partidos» (100 = tope).
        Se pasan una vez a la unidad del juego (x90), con el tope de renovación."""
        if club.get("contrato_x90"):
            return False
        tope = int((self.cfg.get("myclub") or {}).get("renovacion_maximo") or 50)
        for j in club.get("jugadores") or []:
            if isinstance(j.get("contract_term"), int):
                j["contract_term"] = self._partidos_a_unidad(min(j["contract_term"], tope))
        club["contrato_x90"] = True
        return True

    def _ficha_jugador_myclub(self, i, j):
        e = lambda k, d=0: j.get(k) if isinstance(j.get(k), int) else d
        t = lambda k, d="NO": j.get(k) if isinstance(j.get(k), str) else d
        cfgm = self.cfg.get("myclub") or {}
        limite = e("level_limit", 30)
        exp = e("exp_total")
        nivel = max(e("level", 1), min(limite, 1 + exp // self.EXP_POR_NIVEL))
        return Empaquetado({
            "gameplayer_unique_id": e("gameplayer_unique_id", i + 1),
            "gameplayer_id": e("gameplayer_id"), "games": e("games"), "goal": e("goal"),
            "assist": e("assist"), "tiredness": e("tiredness"),
            "contract_term": self._contrato(j), "gameplayer_cost": e("gameplayer_cost", 3),
            "rating": e("rating", 75),
            "concept": Empaquetado({k: e(k) for k in self.CONCEPTO_JUGADOR}),
            "is_rental": t("is_rental"), "is_subscription": t("is_subscription"),
            # C (07:25, lector 141c66d60): el precio de renovar sale de la ficha, no de un comando
            "sell_gp": e("sell_gp", int(cfgm.get("venta_gp") or 0)),
            "contract_gp": e("contract_gp", int(cfgm.get("renovacion_gp") or 0)),
            "contract_pes_point": e("contract_pes_point", int(cfgm.get("renovacion_monedas") or 0)),
            "uniform_number": e("uniform_number", i + 1),
            "player_level_need_exp": [], "level_info": Empaquetado({
                "level": nivel, "level_limit": limite, "exp_total": exp,
                "exp_current_level": exp % self.EXP_POR_NIVEL,
                "exp_to_next_level": 0 if nivel >= limite else
                self.EXP_POR_NIVEL - exp % self.EXP_POR_NIVEL}),
            "parameters": list(j.get("parameters") or []),
            "enhanced_parameters": list(j.get("enhanced_parameters") or []),
            "add_skill_id": e("add_skill_id"), "is_protected": t("is_protected"),
            "redcard_info_list": [], "default_overall": e("default_overall", e("rating", 75)),
            "default_rarity": self._rareza(j), "team_id": e("team_id"),
            "nation_id": e("nation_id")})

    def _ficha_dt_myclub(self, i, d):
        e = lambda k, x=0: d.get(k) if isinstance(d.get(k), int) else x
        t = lambda k, x="NO": d.get(k) if isinstance(d.get(k), str) else x
        return Empaquetado({
            "coach_unique_id": e("coach_unique_id", i + 1), "coach_id": e("coach_id"),
            "contract_term": e("contract_term", 10),
            "offensive_main_concept": Empaquetado({k: e(k) for k in self.CONCEPTO_DT}),
            "defensive_main_concept": Empaquetado({k: e(k) for k in self.CONCEPTO_DT}),
            "formation_data": t("formation_data", ""), "overall": e("overall", 70),
            "management": e("management", 70), "adaptability_rate": e("adaptability_rate", 100),
            "sell_gp": self._venta_dt_gp(d), "contract_gp": 0, "contract_pes_point": 0,
            "current_point_division": 0, "current_point_sim": 0, "current_point_vscom": 0,
            "is_use_variable_formation": t("is_use_variable_formation"),
            "is_coach_texture_use": t("is_coach_texture_use"), "team_id": e("team_id"),
            "nation_id": e("nation_id")})

    async def c_cmd_get_myclub_server_env(self, s, m):
        return {"license_out_player_list": [], "license_out_coach_list": [],
                "market_scout_list": [], "sell_gp": [], "is_all_club_edition_set": "NO"}

    async def c_cmd_get_myclub_user_info(self, s, m):
        cfg = self._cfg_myclub()
        # P: «club_de_prueba» en config = fingir un club ya creado para ver si el cliente pasa
        # al menú principal de myClub (y pide IS_FIRST_* / SEND_MYCLUB_PROGRESS_STATUS)
        club = self._club_inicial(s)
        # P: prueba «sin club = error»: config myclub.resultado_sin_club (p. ej. ERR_NOTREGISTERED)
        error = (self.cfg.get("myclub") or {}).get("resultado_sin_club")
        if not club and error:
            return {"result": error}
        c = club or {}
        texto = lambda k: c.get(k) if isinstance(c.get(k), str) else ""
        entero = lambda k: c.get(k) if isinstance(c.get(k), int) else 0
        casa = c.get("home_town") if isinstance(c.get("home_town"), dict) else {}
        return {"gp": c.get("gp", cfg["gp"]), "pes_point": self._monedas(c),
                "progress_status": cfg["con"] if club else cfg["sin"],
                "team_name": texto("team_name"), "team_name_short": texto("team_name_short"),
                "base_team_id": entero("base_team_id"),
                "team_name_commentary_id": entero("team_name_commentary_id"),
                "user_team_name": texto("team_name"),
                "user_team_name_short": texto("team_name_short"),
                "user_base_team_id": entero("base_team_id"),
                "user_team_name_commentary_id": entero("team_name_commentary_id"),
                "norma_point_division": 0, "norma_point_sim": 0, "norma_point_vscom": 0,
                "home_town": Empaquetado({"nation_id": int(casa.get("nation_id") or 0),
                                          "label": str(casa.get("label") or ""),
                                          "name": str(casa.get("name") or "")}),
                "nation_id": entero("nation_id"), "label": texto("label"), "name": texto("name"),
                "add_uniform_list": [], "select_add_uniform_id": 0,
                "is_iconic_moment_view": "NO"}

    async def c_cmd_send_myclub_progress_status(self, s, m):
        # C (§13): máquina de estados guiada por el SERVIDOR: el cliente manda «status» (texto)
        # y lee «next_status» (texto). P: sin tabla conocida, se guarda y se devuelve el mismo.
        estado = m.get("status") if isinstance(m.get("status"), str) else ""
        club = self.datos.club(s.steam_hex or "")
        if club is not None:
            club["progress_status"] = estado
            self.datos.guardar_club(s.steam_hex or "", club)
        siguiente = ((self.cfg.get("myclub") or {}).get("siguiente_estado") or {}).get(estado, estado)
        return {"next_status": siguiente}

    async def c_cmd_get_myclub_is_first_memberlist(self, s, m):
        return {"is_first": "YES"}  # C (§13): texto YES/NO

    async def c_cmd_get_myclub_is_first_agent_multiple(self, s, m):
        return {"is_first": "YES"}

    async def c_cmd_get_myclub_mainmenu_info(self, s, m):
        # C (§15.1). P: game_result_wdl / game_result / cpu_level se omiten hasta conocer su forma
        # (una clave ausente deja el valor por defecto del cliente).
        _, dts = self._plantel_myclub(s)
        jugadores, _ = self._plantel_myclub(s)
        sin_archivo = lambda: Empaquetado({"filename": "", "hash_str": ""})
        club = self._club_inicial(s) or {}
        com = club.get("vscom") or {}
        return {"com_countWin_num": int(com.get("racha") or 0),
                "com_match_num": int(com.get("partidos") or 0), "oppo_level": 0,
                "weekly_group_rank": 0, "weekly_group_rank_total_num": 0, "rank": 0,
                "rank_total_num": 0, "rank_coach": 0, "rank_coach_total_num": 0,
                # P (vivo 06-nov): la tarjeta Entrenamiento decía 0 con 1 entrenador; antes iba fijo en 0.
                # P: scount_num [sic] = cazatalentos del club.
                "present_num": 0,
                "scount_num": sum(int(c.get("count") or 1) for c in club.get("scouts") or []),
                "staff_num": len(club.get("staff") or []),
                "is_free_enable": "YES", "is_coop_enable": "NO", "is_auction_state_changed": "NO",
                "achievement_info": Empaquetado({"achievement_not_recieved_num": 0,
                                                 "achievement_num": 0, "achievement_total_num": 0}),
                "agent_panel_file_info": sin_archivo(), "liveevent_panel_file_info": sin_archivo(),
                "matchday_emblem_info": Empaquetado({
                    "is_use_emblem": "NO", "is_auto_make_compe_name": "NO",
                    "matchday_group_type": "", "team_list_group_a": [], "team_list_group_b": [],
                    "compe_db_id": 0})}

    async def c_cmd_get_myclub_login_bonus(self, s, m):
        return {"login_bonus_list": [], "login_bonus_list_num": 0, "today_bonus_number": 0,
                "is_bonus": "NO"}  # C (§15.2): «sin bonus»

    # P (05-nov noche): item_type/item_variation son texto libre (no hay enum en el exe, RE
    # noche §ITEMLIST). Candidatos de la tabla de categorías 0x142ad6780: CONTRACT y RECOVERY.
    # Se cambian en caliente con myclub.objetos para probar otros valores sin reiniciar.
    OBJETOS_DEFECTO = [
        {"clave": "boletos_contrato", "item_type": "CONTRACT", "item_variation": "CONTRACT"},
        {"clave": "objetos_recuperar", "item_type": "RECOVERY", "item_variation": "RECOVERY"},
    ]

    def _objetos(self):
        lista = (self.cfg.get("myclub") or {}).get("objetos")
        return [o for o in (lista if isinstance(lista, list) else self.OBJETOS_DEFECTO)
                if isinstance(o, dict) and o.get("clave")]

    async def c_cmd_get_myclub_itemlist(self, s, m):
        # C (§15.3, lector 0x141bed520): item_list = lista de bin({item_type, item_variation,
        # item_effect_arg, count, pes_point_price}). Los conteos salen del club.
        club = self._club_inicial(s) or {}
        lista = [Empaquetado({"item_type": str(o.get("item_type") or "NotImplement"),
                              "item_variation": str(o.get("item_variation") or "NotImplement"),
                              "item_effect_arg": int(o.get("item_effect_arg") or 0),
                              "count": int(club.get(o["clave"]) or 0),
                              "pes_point_price": int(o.get("pes_point_price") or 0)})
                 for o in self._objetos()
                 if o.get("mostrar_vacio", True) or int(club.get(o["clave"]) or 0) > 0]
        return {"item_list": lista, "item_list_num": len(lista)}

    @staticmethod
    def _a_json(v):
        """bytes -> {"b64": ...} para guardarlo en el JSON del club tal cual llegó."""
        if isinstance(v, (bytes, bytearray)):
            return {"b64": base64.b64encode(bytes(v)).decode()}
        return v

    @staticmethod
    def _de_json(v):
        if isinstance(v, dict) and set(v) == {"b64"}:
            return base64.b64decode(v["b64"])
        return v

    def _catalogo(self):
        """Catálogo sacado de la base del juego (tools/catalogo_pesdb.py), ruta en config
        myclub.catalogo. Se lee una vez. Sin catálogo = {}."""
        ruta = (self.cfg.get("myclub") or {}).get("catalogo")
        if not ruta:
            return {}
        if getattr(self, "_cat_ruta", None) != ruta:
            try:
                with open(ruta, encoding="utf-8") as f:
                    self._cat = json.load(f)
            except (OSError, ValueError) as e:
                log.warning("myClub: no pude leer el catálogo %s: %s", ruta, e)
                self._cat = {}
            self._cat_ruta = ruta
        return self._cat

    def _equipo_catalogo(self, team_id, cuantos=None):
        """Jugadores de un equipo del catálogo, en el orden del juego (XI + banquillo)."""
        cat = self._catalogo()
        filas = (cat.get("equipos") or {}).get(str(team_id)) or []
        datos = cat.get("jugadores") or {}
        salida = []
        for f in filas[:cuantos] if cuantos else filas:
            extra = datos.get(str(f.get("gameplayer_id"))) or {}
            salida.append({**extra, "gameplayer_id": f.get("gameplayer_id"),
                           "uniform_number": f.get("uniform_number") or 0,
                           "team_id": int(team_id)})
        return salida

    def _plantel_inicial(self):
        """Copia propia de los jugadores y DT de config (myclub.jugadores / entrenadores), con
        gameplayer_unique_id / coach_unique_id fijos para que cada club los guarde a su nombre.
        Sin myclub.jugadores, se toma del catálogo el equipo club_de_prueba.base_team_id."""
        cfg = self.cfg.get("myclub") or {}
        base = cfg.get("jugadores") or []
        if not base:
            equipo = (cfg.get("club_de_prueba") or {}).get("base_team_id")
            base = self._equipo_catalogo(equipo) if equipo else []
        jug = [dict(j, gameplayer_unique_id=j.get("gameplayer_unique_id") or i + 1)
               for i, j in enumerate(x for x in base if isinstance(x, dict))]
        dts = [dict(d, coach_unique_id=d.get("coach_unique_id") or i + 1)
               for i, d in enumerate(x for x in cfg.get("entrenadores") or [] if isinstance(x, dict))]
        return jug, dts

    def _club_inicial(self, s):
        """Club guardado de ESTA persona. Si no tiene y config trae myclub.club_de_prueba, se le
        crea uno propio (copia de la plantilla) y se guarda: así cada cuenta tiene sus GP,
        su orden de plantilla y sus partidos. Sin plantilla en config = None."""
        club = self.datos.club(s.steam_hex or "")
        if club is not None:
            if self._migrar_contratos(club):
                self.datos.guardar_club(s.steam_hex or "", club)
            return club
        base = (self.cfg.get("myclub") or {}).get("club_de_prueba")
        if not isinstance(base, dict) or not s.steam_hex:
            return None
        club = dict(base)
        club.setdefault("gp", self._cfg_myclub()["gp"])
        club["jugadores"], club["entrenadores"] = self._plantel_inicial()
        club["contrato_x90"] = True
        self.datos.guardar_club(s.steam_hex, club)
        log.info("myClub: club propio creado para %s (%s)", s.steam_hex, club.get("team_name"))
        return club

    def _club_o_prueba(self, s):
        return self._club_inicial(s) or {}

    def _sumar_gp(self, s, cantidad):
        """Suma (o resta) GP al club guardado. Devuelve el saldo nuevo, o None si no alcanza."""
        club = self._club_inicial(s)
        if club is None:
            return None
        saldo = int(club.get("gp") or 0) + int(cantidad)
        if saldo < 0:
            return None
        club["gp"] = saldo
        self.datos.guardar_club(s.steam_hex or "", club)
        return saldo

    def _plan_texto(self, guardado=None):
        """C (05-nov, depurador): gameplan_data es TEXTO base64 (el lector lo copia como cadena
        terminada en NUL); los bytes crudos empiezan por 0x00 y quedaban vacíos → sin tabla de
        huecos → los 11 huecos al miembro 0 (11× Courtois, todos «(PT)»). gameplan_data_size =
        bytes ANTES de base64 (536). Devuelve (size, texto)."""
        v = self._de_json(guardado) if guardado else b""
        if isinstance(v, str) and v:
            try:
                crudo = base64.b64decode(v, validate=True)
            except Exception:
                crudo = v.encode("latin-1", "replace")
                v = base64.b64encode(crudo).decode()
            return len(crudo), v
        crudo = bytes(v) if isinstance(v, (bytes, bytearray)) and v else GAMEPLAN_DEFECTO
        return len(crudo), base64.b64encode(crudo).decode()

    async def c_cmd_get_myclub_squad_gameplan(self, s, m):
        # C (§15.4/§17): el plan (XI inscrito + banquillo) lo guarda el servidor tal cual lo mandó
        # el cliente con SET_MYCLUB_SQUAD_GAMEPLAN; vacío = «menos de 11 inscritos»
        club = self.datos.club(s.steam_hex or "") or {}
        planes = club.get("gameplans") or {}
        p = planes.get(str(m.get("squad_id") or 1)) or {}
        tam, texto = self._plan_texto(p.get("gameplan_data"))
        return {"gameplan_data_size": tam, "gameplan_data": texto}

    async def c_cmd_set_myclub_squad_gameplayer(self, s, m):
        # C (§17): el cliente manda los 18 inscritos (XI + 7) en orden; se guarda ese orden
        club = self._club_o_prueba(s)
        orden = []
        for el in m.get("member_list") or []:
            if isinstance(el, (bytes, bytearray)):
                try:
                    el = msgpack_min.unpackb(el)
                except Exception:
                    continue
            if isinstance(el, dict) and isinstance(el.get("gameplayer_unique_id"), int):
                orden.append(el["gameplayer_unique_id"])
                if isinstance(el.get("uniform_number"), int) and 1 <= el["uniform_number"] <= 99:
                    club.setdefault("dorsales_plantilla", {}).setdefault(
                        str(m.get("squad_id") or 1), {})[str(el["gameplayer_unique_id"])] = el["uniform_number"]
                # P (06-nov): «parameters» del miembro = el formato que el cliente espera en
                # exp_info_pre/post (con lista vacía la carta del entrenamiento sale PT 40 / 0)
                par = el.get("parameters")
                if isinstance(par, list) and par:
                    club.setdefault("parametros_cliente", {})[str(el["gameplayer_unique_id"])] = \
                        [int(x) for x in par if isinstance(x, int)]
        if orden and not club.get("_parametros_vistos"):
            ej = (club.get("parametros_cliente") or {}).get(str(orden[0]))
            log.warning("myClub: parameters de ejemplo (uid %s): %r", orden[0], ej)
            club["_parametros_vistos"] = True
        club.setdefault("inscritos", {})[str(m.get("squad_id") or 1)] = orden
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_set_myclub_squad_gameplan(self, s, m):
        log.warning("peticion completa CMD_SET_MYCLUB_SQUAD_GAMEPLAN: %r", m)
        club = self._club_o_prueba(s)
        planes = club.setdefault("gameplans", {})
        planes[str(m.get("squad_id") or 1)] = {
            k: self._a_json(v) for k, v in m.items() if k not in ("msgid", "rqid", "client", "svrtype")}
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_set_myclub_main_squad(self, s, m):
        # C (§15.5): solo msgid/rqid/result. C (10-nov, PES): tras copiar, el PES activa la copia
        # con este comando; se guarda cuál es la plantilla activa (la que juega).
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        sq = self._id_entero(m.get("squad_id"))
        if club is not None and sq in self._ids_plantillas(club):
            club["plantilla_activa"] = sq
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    def _activa(self, club):
        sq = self._id_entero((club or {}).get("plantilla_activa"))
        return sq if sq in self._ids_plantillas(club or {}) else 1

    async def c_cmd_set_myclub_cloud_squad(self, s, m):
        # C (§15.6): solo result. C (10-nov, PES): {cloud_squad_id, is_use_cloud_match}; se guarda
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        sq = self._id_entero(m.get("cloud_squad_id"))
        if club is not None and sq in self._ids_plantillas(club):
            club["plantilla_nube"] = sq
            if isinstance(m.get("is_use_cloud_match"), str):
                club["usa_nube"] = m["is_use_cloud_match"]
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    def _nube(self, club):
        sq = self._id_entero((club or {}).get("plantilla_nube"))
        return sq if sq in self._ids_plantillas(club or {}) else 0

    async def c_cmd_get_myclub_player_number_fixation(self, s, m):
        return {"player_number_list": [], "player_number_list_num": 0}  # C (§15.7)

    async def c_cmd_get_myclub_scoutlist(self, s, m):
        # C (§15.8 + §33): cazatalentos del club
        club = self._club_inicial(s) or {}
        lista = [self._ficha_scout(c) for c in self._scouts(club)]
        return {"scout_list": lista, "scout_list_num": len(lista)}

    async def c_cmd_get_myclub_weekly_ranking_basic_info(self, s, m):
        # C (§15.9). P: «reward» (regalo) se omite hasta tener uno real
        return {"basic_info": Empaquetado({
            "eura_url": "", "is_beginner": "YES", "is_join": "NO", "is_rejected": "NO",
            "is_season_end": "NO", "division": "", "division_change_type": "",
            "is_no_play_season": "NO", "is_group_rank_1st": "NO"})}

    async def c_cmd_get_myclub_cloudmatch_bonus(self, s, m):
        # C (§15.10). P: «cloud_bonus» (regalo) se omite
        return {"match_num": 0, "win_num": 0, "lose_num": 0, "draw_num": 0,
                "match_list": [], "match_list_num": 0}

    # ---------- Fichar jugadores: representantes (agentes) y cazatalentos (C, 05-nov §33) ----------
    def _agentes(self):
        """Config myclub.agentes = [{agent_id, name, gp, pes_point, get_player_num,
        equipos: [team_id...] o jugadores: [gameplayer_id...], media_min, posiciones}]."""
        return [a for a in (self.cfg.get("myclub") or {}).get("agentes") or []
                if isinstance(a, dict) and isinstance(a.get("agent_id"), int)]

    def _bolsa(self, origen, club):
        """Candidatos (fichas de catálogo) de un agente/cazatalentos, sin los que el club ya tiene."""
        tiene = {j.get("gameplayer_id") for j in (club or {}).get("jugadores") or []}
        if origen.get("jugadores"):
            bolsa = [{"gameplayer_id": int(g)} for g in origen["jugadores"]]
            datos = self._catalogo().get("jugadores") or {}
            bolsa = [{**(datos.get(str(b["gameplayer_id"])) or {}), **b} for b in bolsa]
        else:
            bolsa = [j for t in origen.get("equipos") or [] for j in self._equipo_catalogo(t)]
        minimo = int(origen.get("media_min") or 0)
        puestos = set(origen.get("posiciones") or [])  # p. ej. ["DC", "EI", "ED"] (catálogo)
        return [b for b in bolsa if b.get("gameplayer_id") not in tiene
                and int(b.get("rating") or 75) >= minimo
                and (not puestos or b.get("posicion_txt") in puestos)]

    def _ficha_agente(self, a):
        """C (05-nov, lector 141c20550): cada elemento es bin({agent: bin({...}),
        banner_file_info: bin, skill: bin}); banner y skill son HERMANOS de «agent». Antes iban
        sueltos y el cliente marcaba la respuesta como error y volvía al menú."""
        agente = Empaquetado({
            "agent_id": a["agent_id"], "name": a.get("name") or "", "name_jp": a.get("name_jp") or "",
            "type": a.get("type") or "", "target_level": int(a.get("target_level") or 0),
            # C (06:30, FUN_140d46f90): con token_list vacía y un byte del agente en 0 la caja
            # dice «caducó». P: un token {level, count} y is_draw_checked YES (config agentes)
            "token_list": [Empaquetado({"level": int(a.get("token_level") or 1),
                                        "count": int(a.get("token_count") or 1)})],
            "token_list_num": 1,
            "bgm_id": int(a.get("bgm_id") or 0), "uniform": int(a.get("uniform") or 0),
            "get_player_num": int(a.get("get_player_num") or 1),
            "is_draw_checked": a.get("is_draw_checked") or "YES",
            "is_buy_gp_pes_point": "YES", "pes_point": int(a.get("pes_point") or 0),
            "gp": int(a.get("gp") or 0), "agent_end_time": int(time.time()) + 7 * 86400,
            "agent_token_end_time": int(time.time()) + 7 * 86400})
        return Empaquetado({
            "agent": agente,
            "banner_file_info": Empaquetado({"filename": "", "hash_str": ""}),
            "skill": Empaquetado({"skill_category": a.get("skill_category") or "",
                                  "skill_id": int(a.get("skill_id") or 0),
                                  "limit_count": 0, "use_count": 0})})

    def _fichar(self, club, nuevos):
        """Añade jugadores al club con unique_id nuevo y contrato completo; devuelve las fichas."""
        if not club.get("jugadores"):
            club["jugadores"] = self._plantel_inicial()[0]
        jug = club["jugadores"]
        siguiente = max([int(j.get("gameplayer_unique_id") or 0) for j in jug] + [0]) + 1
        fichas = []
        for n in nuevos:
            j = {**n, "gameplayer_unique_id": siguiente, "contract_term": self._contrato_inicial()}
            j.pop("puesto", None)
            siguiente += 1
            jug.append(j)
            fichas.append(self._ficha_jugador_myclub(len(jug) - 1, j))
        return fichas

    def _cobrar(self, club, gp, monedas, con_monedas):
        """Cobra en GP o en monedas gratis. Devuelve el código de error o None."""
        if con_monedas:
            if not self._monedas_activas():
                return "ERR_FORBIDDEN"
            if int(club.get("monedas") or 0) < monedas:
                return "ERR_INSUFFICIENT_PES_POINT"
            club["monedas"] = int(club.get("monedas") or 0) - monedas
        else:
            if int(club.get("gp") or 0) < gp:
                return "ERR_INSUFFICIENT_GP"
            club["gp"] = int(club.get("gp") or 0) - gp
        return None

    async def c_cmd_get_myclub_market_agentlist(self, s, m):
        # C (§16.1 + §33, lector 141c20550). Agentes de config myclub.agentes
        lista = [self._ficha_agente(a) for a in self._agentes()]
        return {"market_agent_list": lista, "market_agent_list_num": len(lista),
                "market_subscription": Empaquetado({"target_level": 0, "bgm_id": 0, "uniform": 0,
                                                    "end_time": int(time.time()) + 7 * 86400,
                                                    "limit_count": 0, "use_count": 0})}

    async def c_cmd_get_myclub_agent_box_info(self, s, m):
        a = next((x for x in self._agentes() if x["agent_id"] == m.get("agent_id")), None)
        if a is None:
            return {"result": "ERR_NOTFOUND"}
        bolsa = self._bolsa(a, self._club_inicial(s))
        # C (05-nov, lector 141c2e880): *_player_rarity_info son LISTAS de bin({rarity, count});
        # un bin suelto se leía como lista → basura → PES se cerraba
        # C (05-nov, constructor de la caja FUN_140d46f90): si la lista de rareza está VACÍA el
        # cliente muestra «No puedes usar este Representante porque ya caducó» (msg 0x22602be).
        # P: un elemento {rarity, count} con los candidatos que quedan
        rareza = int(a.get("rareza_caja") or 1)
        par = lambda n=len(bolsa): Empaquetado({"rarity": rareza, "count": n})
        # C (05-nov, lector 141c2e880): DOS niveles agent_info; el de fuera lleva la caja y el
        # de dentro agent + banner_file_info + skill (con un solo nivel PES se cerraba)
        return {"agent_info": Empaquetado({
            "agent_info": self._ficha_agente(a), "box_size": len(bolsa),
            "box_pack_num": len(bolsa), "reset_remain_count": 0,
            # C (x64dbg 06:25, 141c3055f): main = LISTA de bin; sub1 y sub2 = UN bin cada una
            "main_player_rarity_info": [par()], "sub1_player_rarity_info": par(0),
            "sub2_player_rarity_info": par(0), "is_use_reset": "NO"})}

    async def c_cmd_get_myclub_agent_draw_rate(self, s, m):
        # C (lector 141ca9080): main/sub1/sub2 = bin({rate_table: [int], rate_player_num: [int]});
        # listas paralelas, una fila por rareza. P: una sola fila con todos los candidatos
        a = next((x for x in self._agentes() if x["agent_id"] == m.get("agent_id")), None)
        n = len(self._bolsa(a, self._club_inicial(s))) if a else 0
        tasa = lambda: Empaquetado({"rate_table": [10000], "rate_player_num": [n]})
        return {"main": tasa(), "sub1": tasa(), "sub2": tasa()}

    async def c_cmd_reset_myclub_agent_box(self, s, m):
        return {}

    async def c_cmd_use_myclub_agent(self, s, m):
        # C (§33, lector de USE_MYCLUB_AGENT): petición {agent_id, use_pes_point, use_token,
        # use_token_level}; respuesta gameplayer_list (fichas nuevas), agent_bonus_item_list,
        # pes_point y gp = saldos nuevos
        a = next((x for x in self._agentes() if x["agent_id"] == m.get("agent_id")), None)
        club = self._club_inicial(s)
        if a is None or club is None:
            return {"result": "ERR_NOTFOUND"}
        bolsa = self._bolsa(a, club)
        n = max(1, int(a.get("get_player_num") or 1))
        if len(bolsa) < n:
            return {"result": "ERR_NOTFOUND"}
        error = self._cobrar(club, int(a.get("gp") or 0), int(a.get("pes_point") or 0),
                             m.get("use_pes_point") == "YES")
        if error:
            return {"result": error}
        fichas = self._fichar(club, random.sample(bolsa, n))
        self.datos.guardar_club(s.steam_hex or "", club)
        log.info("myClub %s usó agente %s: %d jugador(es)", s.steam_hex, a["agent_id"], n)
        return {"gameplayer_list": fichas, "gameplayer_list_num": len(fichas),
                "agent_bonus_item_list": [], "pes_point": self._monedas(club),
                "gp": int(club.get("gp") or 0)}

    def _ficha_scout(self, c):
        return Empaquetado({
            "agent_id": int(c.get("agent_id") or 0),
            "skill": Empaquetado({"skill_category": c.get("skill_category") or "",
                                  "skill_id": int(c.get("skill_id") or 0)}),
            "grade": int(c.get("grade") or 1), "count": int(c.get("count") or 1),
            "rate": Empaquetado({"rate_table": 0, "rate_player_num": 0}),
            "target_level": int(c.get("target_level") or 0), "sell_gp": int(c.get("sell_gp") or 0)})

    def _scouts(self, club):
        """Cazatalentos del club (club.scouts); si no tiene, los de config myclub.cazatalentos."""
        if "scouts" not in club:
            club["scouts"] = [dict(c) for c in (self.cfg.get("myclub") or {}).get("cazatalentos") or []
                              if isinstance(c, dict)]
        return club["scouts"]

    @staticmethod
    def _scout_pedido(m):
        el = m.get("scout_list")
        if isinstance(el, list):
            el = el[0] if el else None
        if isinstance(el, (bytes, bytearray)):
            try:
                el = msgpack_min.unpackb(el)
            except Exception:
                el = None
        return el if isinstance(el, dict) else {}

    def _buscar_scout(self, club, m):
        pedido = self._scout_pedido(m)
        for i, c in enumerate(self._scouts(club)):
            if int(c.get("agent_id") or 0) == pedido.get("agent_id"):
                return i, c
        return None, None

    async def c_cmd_get_myclub_procurable_scoutlist_gameplayer(self, s, m):
        return {"scout_list": [], "scout_list_num": 0}

    async def c_cmd_get_myclub_procurable_gameplayerlist_scout(self, s, m):
        club = self._club_inicial(s) or {}
        _, c = self._buscar_scout(club, m)
        bolsa = self._bolsa(c, club) if c else []
        lista = [Empaquetado({"gameplayer_id": b["gameplayer_id"], "level_limit": 30,
                              "team_id": int(b.get("team_id") or 0), "is_draw": "YES"})
                 for b in bolsa[:50]]
        return {"gameplayer_list": lista, "gameplayer_list_num": len(lista),
                "rate": Empaquetado({"rate_table": 0, "rate_player_num": len(lista)})}

    async def c_cmd_use_myclub_scout(self, s, m):
        # C (§33): usar un cazatalentos = UNA ficha nueva en «gameplayer» (bin)
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        i, c = self._buscar_scout(club, m)
        bolsa = self._bolsa(c, club) if c else []
        if not bolsa:
            return {"result": "ERR_NOTFOUND"}
        ficha = self._fichar(club, [random.choice(bolsa)])[0]
        if int(c.get("count") or 1) > 1:
            c["count"] = int(c["count"]) - 1
        else:
            club["scouts"].pop(i)
        self.datos.guardar_club(s.steam_hex or "", club)
        # P (vivo 05:10): sin gp la barra superior mostraba «G 0»; se manda el saldo
        return {"gameplayer": ficha, "gp": int(club.get("gp") or 0), "pes_point": self._monedas(club)}

    async def c_cmd_sell_myclub_scout(self, s, m):
        club = self._club_inicial(s)
        i, c = self._buscar_scout(club, m) if club is not None else (None, None)
        if c is None:
            return {"result": "ERR_NOTFOUND"}
        ganancia = int(c.get("sell_gp") or 0)
        club["scouts"].pop(i)
        club["gp"] = int(club.get("gp") or 0) + ganancia
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"gain_gp": ganancia, "gp": club["gp"]}

    async def c_cmd_extend_myclub_gameplayer(self, s, m):
        # C (§33): renovar contratos. P: +N partidos (myclub.renovacion_partidos, 10) por
        # myclub.renovacion_gp GP cada jugador (o renovacion_monedas)
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        cfg = self.cfg.get("myclub") or {}
        uids = [int(u) for u in m.get("gameplayer_unique_id_list") or [] if str(u).isdigit()]
        elegidos = [j for j in club.get("jugadores") or [] if j.get("gameplayer_unique_id") in uids]
        extra = self._partidos_a_unidad(cfg.get("renovacion_partidos") or 10)
        # P: el juego avisa de «máxima duración»; config en partidos (def. 50)
        tope = self._partidos_a_unidad(cfg.get("renovacion_maximo") or 50)
        # P: a quien ya está en el tope no se le cobra (vivo 07:30: Messi con 100 pagó 200 GP)
        a_cobrar = [j for j in elegidos if self._contrato(j) < tope]
        error = self._cobrar(club, int(cfg.get("renovacion_gp") or 0) * len(a_cobrar),
                             int(cfg.get("renovacion_monedas") or 0) * len(a_cobrar),
                             m.get("use_pes_point") == "YES")
        if error:
            return {"result": error}
        for j in elegidos:
            j["contract_term"] = min(self._contrato(j) + extra, tope)
        self.datos.guardar_club(s.steam_hex or "", club)
        lista = [Empaquetado({"gameplayer_unique_id": int(j["gameplayer_unique_id"]),
                              "gameplayer_id": int(j.get("gameplayer_id") or 0),
                              "contract_term": j["contract_term"]}) for j in elegidos]
        return {"contract_term_list": lista, "gp": int(club.get("gp") or 0),
                "pes_point": self._monedas(club)}

    async def c_cmd_extend_myclub_coach(self, s, m):
        club = self._club_inicial(s)
        dt = next((d for d in (club or {}).get("entrenadores") or []
                   if d.get("coach_unique_id") == m.get("coach_unique_id")), None)
        if dt is None:
            return {"result": "ERR_NOTFOUND"}
        cfg = self.cfg.get("myclub") or {}
        error = self._cobrar(club, int(cfg.get("renovacion_gp") or 0),
                             int(cfg.get("renovacion_monedas") or 0), m.get("use_pes_point") == "YES")
        if error:
            return {"result": error}
        dt["contract_term"] = int(dt.get("contract_term") or 0) + int(cfg.get("renovacion_partidos") or 10)
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"contract_term": dt["contract_term"], "gp": int(club.get("gp") or 0),
                "pes_point": self._monedas(club)}

    async def c_cmd_get_myclub_market_coachlist(self, s, m):
        # C (§16.2). DT a la venta: config myclub.mercado_dt = [coach_id...] (de MyclubCoach.bin)
        # P: cada entrada puede ser un id o {coach_id, gp, pes_point}; el campo del precio en
        # GP aún no está confirmado («gp» de prueba).
        comprados = set((self._club_inicial(s) or {}).get("dt_comprados") or [])
        ids = [c for c in self._mercado_dt() if c.get("coach_id") not in comprados]
        ahora = int(time.time())
        lista = [Empaquetado({
            "coach_id": int(c.get("coach_id") or 0), "pes_point": int(c.get("pes_point") or 0),
            "gp": int(c.get("gp") or 0), "overall": 70,
            "offensive_main_concept": Empaquetado({k: 0 for k in self.CONCEPTO_DT}),
            "defensive_main_concept": Empaquetado({k: 0 for k in self.CONCEPTO_DT}),
            "formation_data": "", "is_locked": "NO", "management": 70, "adaptability_rate": 100,
            "is_use_variable_formation": "NO"}) for c in ids if isinstance(c.get("coach_id"), int)]
        return {"market_coach_list": lista, "market_coach_list_num": len(lista),
                "now_time": ahora, "end_time": ahora + 7 * 86400}

    def _mercado_dt(self):
        return [c if isinstance(c, dict) else {"coach_id": c}
                for c in (self.cfg.get("myclub") or {}).get("mercado_dt") or []]

    async def c_cmd_buy_myclub_market_coach(self, s, m):
        # C (05-nov, lector 141d8ba10). Petición {coach_id, use_pes_point, is_coach_texture_use}.
        # Respuesta: gp y pes_point = saldos NUEVOS, coach = ficha del DT nuevo (bin), is_first.
        con_monedas = m.get("use_pes_point") == "YES"
        if con_monedas and not self._monedas_activas():
            return {"result": "ERR_FORBIDDEN"}
        coach_id = m.get("coach_id") if isinstance(m.get("coach_id"), int) else 0
        oferta = next((c for c in self._mercado_dt() if c.get("coach_id") == coach_id), None)
        club = self._club_inicial(s)
        if oferta is None or club is None or coach_id in (club.get("dt_comprados") or []):
            return {"result": "ERR_NOTFOUND"}
        moneda = "monedas" if con_monedas else "gp"
        precio = int(oferta.get("pes_point" if con_monedas else "gp") or 0)
        if int(club.get(moneda) or 0) < precio:
            return {"result": "ERR_INSUFFICIENT_PES_POINT" if con_monedas else "ERR_INSUFFICIENT_GP"}
        # C (05-nov): clubes guardados antes de a61d371 no tenían «entrenadores» (salían de
        # config); sin esto el DT inicial se perdía al fichar el primero
        if not club.get("entrenadores"):
            club["entrenadores"] = self._plantel_inicial()[1]
        if not club.get("jugadores"):
            club["jugadores"] = self._plantel_inicial()[0]
        dts = club["entrenadores"]
        uid = max([int(d.get("coach_unique_id") or 0) for d in dts] + [0]) + 1
        nuevo = {**{k: v for k, v in oferta.items() if k not in ("gp", "pes_point")},
                 "coach_unique_id": uid, "contract_term": 10,
                 "is_coach_texture_use": m.get("is_coach_texture_use") or "NO"}
        primero = not dts
        dts.append(nuevo)
        club[moneda] = int(club.get(moneda) or 0) - precio
        club.setdefault("dt_comprados", []).append(coach_id)
        self.datos.guardar_club(s.steam_hex or "", club)
        log.info("myClub %s fichó DT %d por %d %s", s.steam_hex, coach_id, precio, moneda)
        return {"gp": int(club.get("gp") or 0), "pes_point": self._monedas(club), "coach": self._ficha_dt_myclub(len(dts) - 1, nuevo),
                "is_first": "YES" if primero else "NO"}

    async def c_cmd_get_myclub_coach_contract_norma(self, s, m):
        # C (05-nov, lector 141c5b360): puntos del contrato del DT. P: sin normas = todo 0.
        return {"current_point_division": 0, "current_point_sim": 0, "current_point_vscom": 0,
                "norma_point_division": 0, "norma_point_sim": 0, "norma_point_vscom": 0,
                "is_contract_end": "NO"}

    def _monedas_activas(self):
        """Decisión de Frank (05-nov): Monedas de myClub SOLO como premio gratis (config
        myclub.monedas_gratis = true). Nunca se venden: la tienda de compra sigue sin manejador."""
        return bool((self.cfg.get("myclub") or {}).get("monedas_gratis"))

    def _monedas(self, club):
        return int((club or {}).get("monedas") or 0) if self._monedas_activas() else 0

    async def c_cmd_check_myclub_store(self, s, m):
        # C (05-nov, lector 141c46300): sin respuesta propia, «Pagar con GP» decía «función no
        # disponible». is_enable_myclub_coin = YES solo con monedas gratis activadas.
        return {"is_enable_myclub_coin": "YES" if self._monedas_activas() else "NO",
                "is_enable_subscription": "NO"}

    # ---------- Subastas de la casa (ESPEC §41 y §49) ----------
    # C (REA): un producto es un cazatalentos (scout_id = agent_id, grade = estrellas). Con una
    # sola persona no hay vendedores: la «casa» pone a subasta cazatalentos de config.
    # P: time_limit, auction_end_time y finalround_end_time = segundos Unix absolutos.
    ESTADO_NADA = "AUCTION_USER_STATE_NONE"
    ESTADO_PUJA = "AUCTION_USER_STATE_BID_PARTICIPATION"
    ESTADO_MEJOR = "AUCTION_USER_STATE_HIGHEST_BIDER"

    def _cfg_subastas(self):
        c = dict((self.cfg.get("myclub") or {}).get("subastas") or {})
        c.setdefault("activa", True)
        c.setdefault("cantidad", 6)
        c.setdefault("duracion_min", 60)
        c.setdefault("incremento", 100)
        c.setdefault("precio_por_estrella", 1000)
        c.setdefault("seguimiento_max", 20)
        c.setdefault("deseos_max", 10)
        c.setdefault("rival_casa", {})
        return c

    def _catalogo_subastas(self):
        cfgm = self.cfg.get("myclub") or {}
        lista = self._cfg_subastas().get("productos") or cfgm.get("cazatalentos") or []
        return [p for p in lista if isinstance(p, dict) and p.get("agent_id")]

    def _subastas(self, club):
        """Estado de las subastas del club; cierra las vencidas y repone hasta «cantidad»."""
        st = club.setdefault("subastas", {})
        for k in ("productos", "seguimiento", "deseos", "reembolsos", "historial"):
            st.setdefault(k, [])
        st.setdefault("siguiente_id", 1)
        cfg = self._cfg_subastas()
        ahora = int(time.time())
        abiertas = []
        for p in st["productos"]:
            if p["fin"] > ahora:
                abiertas.append(p)
                continue
            if p.get("mejor") == "yo":
                # P: gana la subasta = el cazatalentos pasa a «Cazatalentos» del club
                plantilla = next((dict(c) for c in self._catalogo_subastas()
                                  if int(c["agent_id"]) == p["scout_id"]), {"agent_id": p["scout_id"]})
                plantilla.pop("precio_inicial", None)
                plantilla.update({"grade": p["grade"], "count": 1})
                self._scouts(club).append(plantilla)
                p["ganada"] = True
                log.info("myClub subasta %s ganada por %d GP", p["product_id"], p["precio"])
            st["historial"].append({k: p.get(k) for k in (
                "product_id", "scout_id", "grade", "skill_category", "skill_id", "precio", "fin",
                "mi_puja", "ganada")})
        st["productos"] = abiertas
        st["historial"] = st["historial"][-100:]
        catalogo = self._catalogo_subastas()
        while cfg["activa"] and catalogo and len(st["productos"]) < int(cfg["cantidad"]):
            c = random.choice(catalogo)
            grade = int(c.get("grade") or 1)
            inicial = int(c.get("precio_inicial") or grade * int(cfg["precio_por_estrella"]))
            st["productos"].append({
                "product_id": st["siguiente_id"], "scout_id": int(c["agent_id"]), "grade": grade,
                "skill_category": c.get("skill_category") or "", "skill_id": int(c.get("skill_id") or 0),
                "precio_inicial": inicial, "precio": inicial, "pujas": 0, "mejor": None,
                "mi_puja": 0, "inicio": ahora, "fin": ahora + int(cfg["duracion_min"]) * 60})
            st["siguiente_id"] += 1
        return st

    def _producto(self, st, pid, con_historial=False):
        for p in st["productos"] + (st["historial"] if con_historial else []):
            if p.get("product_id") == pid:
                return p
        return None

    def _estado_usuario(self, p):
        if p.get("mejor") == "yo":
            return self.ESTADO_MEJOR
        return self.ESTADO_PUJA if p.get("mi_puja") else self.ESTADO_NADA

    def _info_subasta(self, st, p):
        yn = lambda v: "YES" if v else "NO"
        return {"product_id": p["product_id"], "scout_id": p["scout_id"], "grade": p["grade"],
                "skill": Empaquetado({"skill_category": p.get("skill_category") or "",
                                      "skill_id": int(p.get("skill_id") or 0)}),
                "price_now": p["precio"], "bid_num": int(p.get("pujas") or 0),
                "time_limit": p["fin"], "finalround_end_time": p["fin"],
                "auction_state": self._estado_usuario(p), "is_bid_user_max": "NO",
                "update_time": int(time.time()), "is_highest_bider": yn(p.get("mejor") == "yo"),
                "is_register_watchlist": yn(p["product_id"] in st["seguimiento"]),
                "is_bid": yn(p.get("mi_puja"))}

    def _detalle_subasta(self, st, p):
        d = self._info_subasta(st, p)
        d.update({"price_start": p.get("precio_inicial", p["precio"]),
                  "auction_start_time": p.get("inicio", 0), "auction_end_time": p["fin"],
                  "is_highest_bidder": d["is_highest_bider"], "is_reservation": "NO",
                  "second_price": 0, "third_price": 0, "my_price": int(p.get("mi_puja") or 0),
                  "reservation_price": 0})
        for k in ("time_limit", "update_time", "is_register_watchlist"):
            d.pop(k, None)  # C (RE noche): el detalle no lleva time_limit ni update_time
        return Empaquetado(d)

    def _lista_subastas(self, st, productos, m):
        inicio = m.get("start_index") if isinstance(m.get("start_index"), int) else 0
        lista = [Empaquetado(self._info_subasta(st, p)) for p in productos[inicio:inicio + 50]]
        return {"auction_info_list": lista, "total_auction_info_list_num": len(productos)}

    def _club_subastas(self, s):
        club = self._club_inicial(s)
        if club is None:
            return None, None
        return club, self._subastas(club)

    async def c_cmd_get_myclub_refine_search_auction_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None or not self._cfg_subastas()["activa"]:
            return {"auction_info_list": [], "total_auction_info_list_num": 0}  # C (§16.3)
        self.datos.guardar_club(s.steam_hex or "", club)
        return self._lista_subastas(st, st["productos"], m)

    async def c_cmd_get_myclub_auction_watch_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None:
            return {"auction_info_list": [], "total_auction_info_list_num": 0}  # C (§16.4)
        st["seguimiento"] = [i for i in st["seguimiento"] if self._producto(st, i)]
        self.datos.guardar_club(s.steam_hex or "", club)
        return self._lista_subastas(st, [self._producto(st, i) for i in st["seguimiento"]], m)

    async def c_cmd_add_myclub_auction_watch_list(self, s, m):
        club, st = self._club_subastas(s)
        pid = m.get("product_id")
        if club is None or self._producto(st, pid) is None:
            return {"result": "ERR_NOTFOUND"}
        if pid not in st["seguimiento"]:
            if len(st["seguimiento"]) >= int(self._cfg_subastas()["seguimiento_max"]):
                return {"result": "ERR_WATCH_LIST_MAX"}
            st["seguimiento"].append(pid)
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_delete_myclub_auction_watch_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        if m.get("is_all") == "YES":
            st["seguimiento"] = []
        else:
            st["seguimiento"] = [i for i in st["seguimiento"] if i != m.get("product_id")]
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_get_myclub_auction_wish_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None:
            return {"wish_info_list": [], "total_wish_info_list_num": 0}  # C (§16.5)
        lista = []
        for w in st["deseos"]:
            hay = any(p["grade"] == w["grade"] and p.get("skill_id") == w["skill_id"]
                      for p in st["productos"])
            lista.append(Empaquetado({
                "wishlist_id": w["wishlist_id"], "grade": w["grade"],
                "skill": Empaquetado({"skill_category": w["skill_category"], "skill_id": w["skill_id"]}),
                "is_exhibited": "YES" if hay else "NO"}))
        return {"wish_info_list": lista, "total_wish_info_list_num": len(lista)}

    async def c_cmd_add_myclub_auction_wish_list(self, s, m):
        # P: la petición no se leyó con REA; se aceptan skill (bin) y grade
        club, st = self._club_subastas(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        sk = m.get("skill")
        if isinstance(sk, (bytes, bytearray)):
            sk = msgpack_min.unpackb(sk)
        sk = sk if isinstance(sk, dict) else {}
        w = {"grade": int(m.get("grade") or 0), "skill_category": sk.get("skill_category") or "",
             "skill_id": int(sk.get("skill_id") or 0)}
        if any((x["grade"], x["skill_id"]) == (w["grade"], w["skill_id"]) for x in st["deseos"]):
            return {"result": "ERR_WISH_LIST_ALREADY_ADD"}
        if len(st["deseos"]) >= int(self._cfg_subastas()["deseos_max"]):
            return {"result": "ERR_WISH_LIST_MAX"}
        w["wishlist_id"] = max([x["wishlist_id"] for x in st["deseos"]] + [0]) + 1
        st["deseos"].append(w)
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_delete_myclub_auction_wish_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        quitar = set(self._lista_uids(m, "wishlist_id_list"))
        if isinstance(m.get("wishlist_id"), int):
            quitar.add(m["wishlist_id"])
        st["deseos"] = [] if m.get("is_all") == "YES" else [
            w for w in st["deseos"] if w["wishlist_id"] not in quitar]
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_get_myclub_auction_scout_detail(self, s, m):
        club, st = self._club_subastas(s)
        p = self._producto(st, m.get("product_id"), True) if club is not None else None
        if p is None:
            return {"result": "ERR_NOTFOUND"}
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"auction_scout_detail": self._detalle_subasta(st, p)}

    async def c_cmd_watch_myclub_auction_scout_detail(self, s, m):
        r = await self.c_cmd_get_myclub_auction_scout_detail(s, m)
        if "auction_scout_detail" in r:
            return {"update_auction_scout_detail": r["auction_scout_detail"]}
        return r

    async def c_cmd_leave_myclub_auction_scout_detail(self, s, m):
        return {}

    async def c_cmd_tender_myclub_auction_scout(self, s, m):
        """Pujar {product_id, tender_gp, is_finalround}. P: tender_gp es la puja total. El GP
        queda apartado; si la casa puja más (rival_casa), se devuelve al momento."""
        club, st = self._club_subastas(s)
        p = self._producto(st, m.get("product_id")) if club is not None else None
        if p is None:
            return {"result": "ERR_TENDER_TIMEOUT" if club is not None else "ERR_NOTFOUND"}
        cfg = self._cfg_subastas()
        puja = int(m.get("tender_gp") or 0)
        minimo = p["precio"] + (int(cfg["incremento"]) if p.get("pujas") else 0)
        if p.get("mejor") == "yo" or puja < minimo:
            return {"result": "ERR_INSUFFICIENT_TENDER_GP"}
        falta = puja - int(p.get("mi_puja") or 0)
        if int(club.get("gp") or 0) < falta:
            return {"result": "ERR_INSUFFICIENT_GP"}
        club["gp"] = int(club.get("gp") or 0) - falta
        p.update({"precio": puja, "mi_puja": puja, "mejor": "yo", "pujas": int(p.get("pujas") or 0) + 1})
        rival = cfg["rival_casa"] if isinstance(cfg["rival_casa"], dict) else {}
        tope = int(p["precio_inicial"] * float(rival.get("tope_factor") or 2))
        if (rival.get("activo") and random.random() < float(rival.get("probabilidad") or 0.3)
                and puja + int(cfg["incremento"]) <= tope):
            # la casa sube la puja: tu GP vuelve y puedes volver a pujar
            p.update({"precio": puja + int(cfg["incremento"]), "mejor": "casa",
                      "pujas": p["pujas"] + 1, "mi_puja": 0})
            club["gp"] += puja
            st["reembolsos"].append({"product_id": p["product_id"], "gp": puja})
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"auction_scout_detail": self._detalle_subasta(st, p), "gp": int(club["gp"])}

    async def c_cmd_tender_myclub_auction_scout_finalround_reservation(self, s, m):
        # P: sin ronda final en las subastas de la casa: se trata como una puja normal
        return await self.c_cmd_tender_myclub_auction_scout(s, m)

    async def c_cmd_tender_myclub_auction_scout_finalround_reservation_cancel(self, s, m):
        club, st = self._club_subastas(s)
        p = self._producto(st, m.get("product_id")) if club is not None else None
        if p is None:
            return {"result": "ERR_NOTFOUND"}
        return {"auction_scout_detail": self._detalle_subasta(st, p), "gain_gp": 0}

    async def c_cmd_get_myclub_auction_report_list(self, s, m):
        club, st = self._club_subastas(s)
        if club is None:
            return {"auction_report_list": [], "total_auction_report_list_num": 0}
        mias = [p for p in st["productos"] if p.get("mi_puja") or p.get("mejor") == "yo"]
        mias += [p for p in reversed(st["historial"]) if p.get("mi_puja")][:20]
        lista = [Empaquetado(self._info_subasta(st, {**p, "pujas": p.get("pujas", 1),
                                                    "mejor": "yo" if p.get("ganada") else p.get("mejor")}))
                 for p in mias]
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"auction_report_list": lista, "total_auction_report_list_num": len(lista)}

    async def c_cmd_get_myclub_auction_successful_bid_info(self, s, m):
        # C (RE noche): auction_successful_bid_list = lista de ENTEROS. P: precios finales
        # recientes de cazatalentos con el mismo scout_id/grade del producto pedido
        club, st = self._club_subastas(s)
        if club is None:
            return {"auction_successful_bid_list": [], "auction_successful_bid_list_num": 0,
                    "best_bid_amount_price": 0, "best_bid_amount_day": 0, "average_bid_price": 0}
        p = self._producto(st, m.get("product_id"), True) or {}
        precios = [int(h["precio"]) for h in st["historial"]
                   if not p or (h.get("scout_id"), h.get("grade")) == (p.get("scout_id"), p.get("grade"))][-10:]
        return {"auction_successful_bid_list": precios, "auction_successful_bid_list_num": len(precios),
                "best_bid_amount_price": max(precios, default=0), "best_bid_amount_day": 0,
                "average_bid_price": sum(precios) // len(precios) if precios else 0}

    async def c_cmd_get_myclub_auction_price_ranking(self, s, m):
        # C (RE noche): elemento sin «rank»; el orden de la lista es el ranking. ranking_kind
        # se ignora (no tiene enum propio).
        club, st = self._club_subastas(s)
        if club is None:
            return {"auction_price_ranking": [], "auction_price_ranking_num": 0}  # C (§16.6)
        orden = sorted(st["historial"], key=lambda h: -int(h.get("precio") or 0))[:20]
        lista = [Empaquetado({"product_id": h["product_id"], "scout_id": h["scout_id"],
                              "grade": h["grade"],
                              "skill": Empaquetado({"skill_category": h.get("skill_category") or "",
                                                    "skill_id": int(h.get("skill_id") or 0)}),
                              "price": int(h["precio"]), "end_time": int(h["fin"])}) for h in orden]
        return {"auction_price_ranking": lista, "auction_price_ranking_num": len(lista)}

    async def c_cmd_reqwatch_myclub_auction_refunded_gp(self, s, m):
        club, st = self._club_subastas(s)
        if club is None or not st["reembolsos"]:
            return {"refunded_gp_list": [], "total_gp": 0}
        # P: elemento de refunded_gp_list sin leer con REA; se manda la lista de montos
        montos = [int(r["gp"]) for r in st["reembolsos"]]
        st["reembolsos"] = []
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"refunded_gp_list": montos, "total_gp": sum(montos)}

    async def c_cmd_get_myclub_rental_gameplayerlist(self, s, m):
        # C (§16.7): sin respuesta = C_NDZS_0112 en «Acuerdo de préstamo»
        return {"rental_gameplayer_list": [], "rental_gameplayer_list_num": 0}

    async def c_cmd_get_matchrejectlist(self, s, m):
        return {"reject_list": [], "reject_list_num": 0}  # C (§16.8)

    async def c_cmd_get_event_category_list(self, s, m):
        # C (§16.9): sin eventos; el emblema de jornada vacío como en MAINMENU_INFO
        return {"event_category_list": [], "matchday_emblem_info": Empaquetado({
            "is_use_emblem": "NO", "is_auto_make_compe_name": "NO", "matchday_group_type": "",
            "team_list_group_a": [], "team_list_group_b": [], "compe_db_id": 0})}

    async def c_cmd_get_pesleague_compe_result_list(self, s, m):
        return {"result_list": [], "result_list_num": 0}  # C (§16.10)

    async def c_cmd_get_player_parameterlist(self, s, m):
        # C (§19): un elemento por jugador pedido; sin respuesta PES se cierra (c0000409)
        salida = []
        for el in m.get("player_list") or []:
            if isinstance(el, (bytes, bytearray)):
                try:
                    el = msgpack_min.unpackb(el)
                except Exception:
                    continue
            if not isinstance(el, dict):
                continue
            salida.append(Empaquetado({
                "gameplayer_id_info": Empaquetado({
                    "gameplayer_unique_id": el.get("gameplayer_unique_id") or 0,
                    "gameplayer_id": el.get("gameplayer_id") or 0}),
                "parameters": [], "is_cost_over": "NO"}))
        return {"param_playerlist": salida}

    RIVAL_UID_BASE = 900001  # C (05-nov): uids 1..18 chocaban con los del club propio

    def _plantilla_bin(self, jugadores, coach_id, nombre, plan=None):
        tam, texto = self._plan_texto(plan)
        # C (05-nov, partido jugado): con uids 1..18 el rival salía 11 veces «T. Courtois»;
        # cada jugador del rival necesita un unique_id propio, distinto de los del club
        miembros = [Empaquetado({
            "gameplayer_unique_id": j.get("gameplayer_unique_id") or self.RIVAL_UID_BASE + i,
            "gameplayer_id": j.get("gameplayer_id") or 0,
            "uniform_number": j.get("uniform_number") or i + 1,
            "player_level": j.get("level") or 1, "parameters": [],
            "default_rarity": self._rareza(j)}) for i, j in enumerate(jugadores[:18])]
        return Empaquetado({
            "squad_id": 1, "name": nombre, "member_list": miembros, "member_num": len(miembros),
            "is_entry_compe": "NO", "coach_id_list": [coach_id], "coach_unique_id_list": [1],
            "gameplan_data_size": tam, "gameplan_data": texto, "is_club_edition": "NO",
            "club_edition_type": ""})

    def _nivel_cpu(self, club):
        """Nivel de la COM para este club. Config myclub.dificultad = {activa, niveles [orden de
        fácil a difícil], inicial, victorias_para_subir, derrotas_para_bajar}. Apagada o sin
        niveles = el cpu_level fijo de myclub.rival (como antes)."""
        cfg = self.cfg.get("myclub") or {}
        fijo = (cfg.get("rival") or {}).get("cpu_level", "MATCH_LEVEL_REGULAR")
        d = cfg.get("dificultad") or {}
        niveles = [n for n in d.get("niveles") or [] if isinstance(n, str)]
        if not d.get("activa") or not niveles:
            return fijo
        com = (club or {}).get("vscom") or {}
        idx = com.get("nivel_idx")
        if not isinstance(idx, int):
            idx = niveles.index(d["inicial"]) if d.get("inicial") in niveles else 0
        return niveles[max(0, min(idx, len(niveles) - 1))]

    def _ajustar_dificultad(self, com, desenlace):
        """Tras un partido: sube un nivel con N victorias seguidas, baja con M derrotas seguidas.
        Guarda nivel_idx, nivel_antes y las rachas en club["vscom"]."""
        d = (self.cfg.get("myclub") or {}).get("dificultad") or {}
        niveles = [n for n in d.get("niveles") or [] if isinstance(n, str)]
        if not d.get("activa") or not niveles:
            return
        idx = com.get("nivel_idx")
        if not isinstance(idx, int):
            idx = niveles.index(d["inicial"]) if d.get("inicial") in niveles else 0
        com["nivel_antes"] = niveles[max(0, min(idx, len(niveles) - 1))]
        com["derrotas_seguidas"] = int(com.get("derrotas_seguidas") or 0) + 1 \
            if desenlace == "LOSE" else 0
        subir = int(d.get("victorias_para_subir") or 3)
        bajar = int(d.get("derrotas_para_bajar") or 0)
        if desenlace == "WIN" and com.get("racha", 0) and com["racha"] % subir == 0:
            idx = min(idx + 1, len(niveles) - 1)
        elif bajar and com["derrotas_seguidas"] >= bajar:
            idx = max(idx - 1, 0)
            com["derrotas_seguidas"] = 0
        com["nivel_idx"] = idx

    def _rival_de_turno(self, s):
        """Config myclub.rival; si trae «equipos» [{base_team_id, team_name, team_name_short,
        coach_id, cpu_level}...], el rival rota según los partidos vs COM ya jugados del club."""
        r = dict((self.cfg.get("myclub") or {}).get("rival") or {})
        equipos = [e for e in r.pop("equipos", None) or [] if isinstance(e, dict)]
        if equipos:
            jugados = int(((self._club_inicial(s) or {}).get("vscom") or {}).get("partidos") or 0)
            elegido = equipos[jugados % len(equipos)]
            r = {**r, **elegido}
            if "jugadores" not in elegido:
                r.pop("jugadores", None)
        return r

    async def c_cmd_get_myclub_vscom_opponent(self, s, m):
        # C (§19): rival COM. Config myclub.rival = {jugadores [...18 por puesto], coach_id,
        # team_name, team_name_short, base_team_id, cpu_level, side}. P: anidados = bin.
        r = self._rival_de_turno(s)
        jugadores = [j for j in r.get("jugadores") or [] if isinstance(j, dict)]
        if not jugadores and r.get("base_team_id"):
            jugadores = self._equipo_catalogo(r["base_team_id"], 18)
        nombre = r.get("team_name") or "Rival"
        return {"side": r.get("side", "AWAY"),
                "cpu_level": self._nivel_cpu(self._club_inicial(s)) if not r.get("cpu_level_fijo")
                else r.get("cpu_level", "MATCH_LEVEL_REGULAR"),
                "team_name": nombre, "team_name_short": r.get("team_name_short", "RIV"),
                "base_team_id": int(r.get("base_team_id") or 0),
                "team_data": self._plantilla_bin(jugadores, int(r.get("coach_id") or 0), nombre,
                                                 None),
                "pes_pname": Empaquetado({"name": nombre, "xuid_one": "0",
                                          "psn_user_id": Empaquetado({"account_id": 0,
                                                                      "online_id": nombre,
                                                                      "is_valid_cache": "1"}),
                                          "is_deleted": "0"}),
                # Lector 141da7b80: adaptabilidad = lista de int; concept_data = lista con un
                # bin por jugador {gameplayer_id, concept bin}; gameplan también arriba.
                "pid": 0, "gameplan_data_size": self._plan_texto()[0],
                "gameplan_data": self._plan_texto()[1], "coach_adaptability_rate": [100],
                # C (05-nov 02:03): con un concepto por jugador la respuesta medía 1080 B cifrados
                # y no cabía en una trama (0x410). El lector hace push por elemento: [] = vector
                # vacío. Config myclub.rival.con_conceptos = true para volver a mandarlos.
                "concept_data": [Empaquetado({
                    "gameplayer_id": j.get("gameplayer_id") or 0,
                    "concept": Empaquetado({k: 0 for k in self.CONCEPTO_JUGADOR})})
                    for j in jugadores[:18]] if r.get("con_conceptos") else [],
                "team_name_commentary_id": 0}

    async def c_cmd_create_myclub_team(self, s, m):
        # C (§12): la respuesta solo lleva msgid/rqid/result; ERR_ALREADYEXISTS si ya hay club
        if self.datos.club(s.steam_hex or ""):
            return {"result": "ERR_ALREADYEXISTS"}
        casa = m.get("home_town")
        if isinstance(casa, (bytes, bytearray)):
            try:
                casa = msgpack_min.unpackb(casa)
            except Exception:
                casa = {}
        club = {k: m.get(k) for k in ("team_name", "team_name_short", "base_team_id", "coach_id",
                                      "rental_gameplayer_id", "team_name_commentary_id",
                                      "nation_id", "label", "name")
                if isinstance(m.get(k), (str, int))}
        club["home_town"] = casa if isinstance(casa, dict) else {}
        club["gp"] = self._cfg_myclub()["gp"]
        # C (§14): con 0 jugadores el cliente sale de myClub; mientras no haya agentes/fichajes,
        # el club nuevo arranca con la plantilla de config
        club["jugadores"], club["entrenadores"] = self._plantel_inicial()
        club["squads"] = []
        club["staff"] = []
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_create_myclub_squad(self, s, m):
        club = self.datos.club(s.steam_hex or "")
        if club is None:
            return {"result": "ERR_DATABASE"}
        squad_id = len(club.get("squads", [])) + 1
        club.setdefault("squads", []).append({"squad_id": squad_id,
                                              "name": m.get("name") if isinstance(m.get("name"), str) else "",
                                              "coach_unique_id": m.get("coach_unique_id") or 0})
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"squad_id": squad_id}

    async def c_cmd_create_myclub_staff(self, s, m):
        club = self.datos.club(s.steam_hex or "")
        if club is None:
            return {"result": "ERR_DATABASE"}
        claves = ("gameplayer_unique_id", "gameplayer_base_id", "gameplayer_id",
                  "gameplayer_level", "registared_position", "position", "stronger_foot",
                  "overall", "ability", "play_style", "nation_id", "club_team_id")
        salida = []
        for el in m.get("player_info_list") or []:
            if isinstance(el, (bytes, bytearray)):
                try:
                    el = msgpack_min.unpackb(el)
                except Exception:
                    continue
            if not isinstance(el, dict):
                continue
            base = {k: el.get(k) if isinstance(el.get(k), (int, str)) else 0 for k in claves}
            nuevo = {"id": max([e.get("id", 0) for e in club.get("staff", [])] + [0]) + 1,
                     "base_player_info": base, "base_exp": int(base.get("overall") or 0)}
            club.setdefault("staff", []).append(nuevo)
            # C (07:50): el jugador convertido desaparece de los miembros.
            # C (05-nov 09:20): si estaba inscrito (titular o banquillo), su puesto lo ocupa
            # el primer jugador libre; antes quedaba un hueco y PES no dejaba jugar (§42).
            uid = base.get("gameplayer_unique_id")
            if uid:
                self._sacar_de_plantilla(club, uid)
                club["jugadores"] = [j for j in club.get("jugadores") or []
                                     if j.get("gameplayer_unique_id") != uid]
            salida.append(Empaquetado({**nuevo, "base_player_info": Empaquetado(base)}))
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"staff_info_list": salida}

    async def c_cmd_get_myclub_staff_list(self, s, m):
        # C (07:50): staff_list = lista de bin({id, base_player_info, base_exp})
        club = self._club_inicial(s) or {}
        lista = [Empaquetado({**e, "base_player_info": Empaquetado(e.get("base_player_info") or {})})
                 for e in club.get("staff") or []]
        return {"staff_list_num": len(lista), "staff_list": lista}

    # P (06-nov): el cliente no manda «parameters» en SET_MYCLUB_SQUAD_GAMEPLAYER (llega None).
    # Se arman con las habilidades del catálogo (Player.bin) en el orden de la ficha de
    # detalle del juego. Por confirmar en vivo que este es el orden y la escala (valor real).
    ORDEN_HABILIDADES = [
        "act_ofensiva", "control", "drible", "posesion", "pase_ras", "pase_bombeado",
        "finalizacion", "cabeceo", "balon_parado", "efecto", "act_defensiva", "recuperacion",
        "agresividad", "act_portero", "atajar", "despejar", "reflejos", "cobertura",
        "velocidad", "aceleracion", "potencia_tiro", "salto", "contacto", "equilibrio",
        "resistencia"]

    def _habilidades_catalogo(self, j):
        datos = (self._catalogo().get("jugadores") or {}).get(str(j.get("gameplayer_id"))) or {}
        hab = j.get("habilidades") or datos.get("habilidades") or {}
        if not isinstance(hab, dict) or not hab:
            return []
        return [int(hab.get(k) or 40) for k in self.ORDEN_HABILIDADES]

    def _info_exp(self, j, habilidades=None):
        """C (REA 05-nov noche, 0x141ca40d0): exp_info_pre/post = UN bin con level, level_limit,
        exp_total, exp_current_level, exp_to_next_level, parameters, enhanced_parameters,
        add_skill_id."""
        e = lambda k, d=0: j.get(k) if isinstance(j.get(k), int) else d
        limite = e("level_limit", 30)
        exp = e("exp_total")
        nivel = max(e("level", 1), min(limite, 1 + exp // self.EXP_POR_NIVEL))
        return Empaquetado({
            "level": nivel, "level_limit": limite, "exp_total": exp,
            "exp_current_level": exp % self.EXP_POR_NIVEL,
            "exp_to_next_level": 0 if nivel >= limite else self.EXP_POR_NIVEL - exp % self.EXP_POR_NIVEL,
            # C (vivo 06-nov): con parameters vacío, «Resultados del entrenamiento» pintaba la
            # carta con media 40 y todos los atributos 0.
            "parameters": list(j.get("parameters") or habilidades or []),
            "enhanced_parameters": list(j.get("enhanced_parameters") or []),
            "add_skill_id": e("add_skill_id")})

    async def c_cmd_use_myclub_staff(self, s, m):
        """C (REA 05-nov noche, petición 0x141ca2480): player_info (bin, 12 claves),
        staff_info_list [bin {id, base_player_info, base_exp}], use_item_list [bin
        {item_variation, count}]. Respuesta: gameplayer_unique_id, exp_added, exp_info_pre,
        exp_info_post, sell_gp, contract_gp, contract_pes_point, traning_result [], _num.
        P: cada entrenador de experiencia da su base_exp x myclub.staff_exp_factor (def. 1)."""
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        des = lambda v: msgpack_min.unpackb(v) if isinstance(v, (bytes, bytearray)) else v
        info = des(m.get("player_info")) or {}
        log.warning("peticion completa CMD_USE_MYCLUB_STAFF: player_info=%r use_item_list=%r",
                    info, m.get("use_item_list"))  # para confirmar ability y la curva de exp
        uid = info.get("gameplayer_unique_id") if isinstance(info, dict) else None
        j = next((x for x in club.get("jugadores") or [] if x.get("gameplayer_unique_id") == uid), None)
        ids = set()
        for el in m.get("staff_info_list") or []:
            el = des(el)
            if isinstance(el, dict) and isinstance(el.get("id"), int):
                ids.add(el["id"])
        usados = [e for e in club.get("staff") or [] if e.get("id") in ids]
        if j is None or not usados:
            return {"result": "ERR_NOTFOUND"}
        factor = float((self.cfg.get("myclub") or {}).get("staff_exp_factor") or 1)
        ganada = int(sum(int(e.get("base_exp") or 0) for e in usados) * factor)
        # C (vivo 06-nov): player_info.ability son solo 6 enteros (no las habilidades) y con
        # ellos la carta seguía en PT 40. Se usan los parameters que el cliente mandó al
        # guardar la plantilla (SET_MYCLUB_SQUAD_GAMEPLAYER).
        habilidades = ((club.get("parametros_cliente") or {}).get(str(uid))
                       or self._habilidades_catalogo(j))
        antes = self._info_exp(j, habilidades)
        j["exp_total"] = int(j.get("exp_total") or 0) + ganada
        club["staff"] = [e for e in club.get("staff") or [] if e.get("id") not in ids]
        self.datos.guardar_club(s.steam_hex or "", club)
        cfgm = self.cfg.get("myclub") or {}
        log.info("myClub %s: %d entrenador(es) dan +%d exp a %s", s.steam_hex, len(usados), ganada, uid)
        return {"gameplayer_unique_id": int(uid), "exp_added": ganada,
                "exp_info_pre": antes, "exp_info_post": self._info_exp(j, habilidades),
                "sell_gp": int(j.get("sell_gp", cfgm.get("venta_gp") or 0)),
                "contract_gp": int(j.get("contract_gp", cfgm.get("renovacion_gp") or 0)),
                "contract_pes_point": int(j.get("contract_pes_point", cfgm.get("renovacion_monedas") or 0)),
                "traning_result": [], "traning_result_num": 0}

    async def c_cmd_extend_myclub_item_gameplayer(self, s, m):
        """Boleto de contrato. C (REA 05-nov noche, 0x141c00870 / 0x141c00ed0): petición
        {gameplayer_unique_id}; respuesta {contract_term: UN bin {gameplayer_unique_id,
        gameplayer_id, contract_term}, extend_ticket: boletos que quedan}. Boletos en
        club["boletos_contrato"] (se dan con admin o como premio)."""
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        uid = m.get("gameplayer_unique_id")
        j = next((x for x in club.get("jugadores") or [] if x.get("gameplayer_unique_id") == uid), None)
        boletos = int(club.get("boletos_contrato") or 0)
        if j is None:
            return {"result": "ERR_NOTFOUND"}
        if boletos <= 0:
            return {"result": "ERR_FORBIDDEN"}
        cfg = self.cfg.get("myclub") or {}
        tope = self._partidos_a_unidad(cfg.get("renovacion_maximo") or 50)
        j["contract_term"] = min(self._contrato(j) + self._partidos_a_unidad(
            cfg.get("renovacion_partidos") or 10), tope)
        club["boletos_contrato"] = boletos - 1
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"contract_term": Empaquetado({"gameplayer_unique_id": int(uid),
                                              "gameplayer_id": int(j.get("gameplayer_id") or 0),
                                              "contract_term": j["contract_term"]}),
                "extend_ticket": club["boletos_contrato"]}

    async def c_cmd_use_myclub_item_recovery(self, s, m):
        """Recuperar cansancio. C (REA 05-nov noche, 0x141c81660 / 0x141c81e80): petición
        {gameplayer_unique_id_list, is_all, use_pes_point}; respuesta {pes_point}.
        P: cuesta myclub.recuperar_monedas monedas gratis por jugador (def. 0 = gratis)."""
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        uids = set(self._lista_uids(m))
        todos = m.get("is_all") == "YES"
        elegidos = [j for j in club.get("jugadores") or []
                    if todos or j.get("gameplayer_unique_id") in uids]
        objetos = int(club.get("objetos_recuperar") or 0)
        coste = 0
        if m.get("use_pes_point") != "YES" and objetos >= len(elegidos) > 0:
            # P: «Recuperar resistencia (uno)» = un objeto por jugador
            club["objetos_recuperar"] = objetos - len(elegidos)
        else:
            coste = int((self.cfg.get("myclub") or {}).get("recuperar_monedas") or 0) * len(elegidos)
        if coste:
            error = self._cobrar(club, 0, coste, True)
            if error:
                return {"result": error}
        for j in elegidos:
            j["tiredness"] = 0
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"pes_point": self._monedas(club)}

    @staticmethod
    def _sacar_de_plantilla(club, uid):
        """Quita uid de cada plantilla inscrita y pone en su mismo puesto al primer jugador
        libre (no inscrito). Mismo puesto = la tabla de huecos del gameplan sigue valiendo."""
        for clave, orden in (club.get("inscritos") or {}).items():
            if uid not in orden:
                continue
            libres = [j.get("gameplayer_unique_id") for j in club.get("jugadores") or []
                      if j.get("gameplayer_unique_id") not in orden
                      and j.get("gameplayer_unique_id") != uid]
            if libres:
                orden[orden.index(uid)] = libres[0]
            else:
                orden.remove(uid)

    # ---------- menú del jugador: bloquear y liberar (C, 07:50) ----------
    @staticmethod
    def _uids_en_uso(club):
        """Jugadores inscritos en alguna plantilla (no se pueden liberar ni convertir)."""
        return {u for lista in (club.get("inscritos") or {}).values() for u in lista or []}

    @staticmethod
    def _lista_uids(m, clave="gameplayer_unique_id_list"):
        return [int(u) for u in m.get(clave) or [] if isinstance(u, int) or str(u).isdigit()]

    def _proteger(self, s, m, valor):
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        uids = set(self._lista_uids(m))
        for j in club.get("jugadores") or []:
            if j.get("gameplayer_unique_id") in uids:
                j["is_protected"] = valor
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    async def c_cmd_set_myclub_protected_enable(self, s, m):
        return self._proteger(s, m, "YES")

    async def c_cmd_set_myclub_protected_disable(self, s, m):
        return self._proteger(s, m, "NO")

    async def c_cmd_sell_myclub_gameplayer(self, s, m):
        # C (07:50): liberar. REQ gameplayer_unique_id_list + is_use_cloud_match;
        # RES gain_gp y gp. Precio = sell_gp de la ficha (config myclub.venta_gp).
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        uids = set(self._lista_uids(m))
        elegidos = [j for j in club.get("jugadores") or [] if j.get("gameplayer_unique_id") in uids]
        if not elegidos or any(j.get("is_protected") == "YES" for j in elegidos) \
                or uids & self._uids_en_uso(club):
            return {"result": "ERR_FORBIDDEN"}  # bloqueados o inscritos en la plantilla: no
        base = int((self.cfg.get("myclub") or {}).get("venta_gp") or 0)
        ganancia = sum(int(j.get("sell_gp", base)) for j in elegidos)
        club["jugadores"] = [j for j in club["jugadores"] if j not in elegidos]
        club["gp"] = int(club.get("gp") or 0) + ganancia
        self.datos.guardar_club(s.steam_hex or "", club)
        log.info("myClub %s liberó %d jugador(es): +%d GP", s.steam_hex, len(elegidos), ganancia)
        return {"gain_gp": ganancia, "gp": club["gp"]}

    async def c_cmd_get_myclub_feature_player_list(self, s, m):
        return {"gameplayer_list": [], "gameplayer_list_num": 0,
                "hold_gameplayer_list": [], "hold_gameplayer_list_num": 0}

    async def c_cmd_get_myclub_gameplayerlist(self, s, m):
        jugadores, _ = self._plantel_myclub(s)
        fichas = [self._ficha_jugador_myclub(i, j) for i, j in enumerate(jugadores)]
        return {"gameplayer_list": fichas, "gameplayer_list_num": len(fichas),
                "gameplayer_list_max": 250,
                "main_edition_gameplayer_list": [], "main_edition_gameplayer_list_num": 0}

    async def c_cmd_get_myclub_coachlist(self, s, m):
        _, dts = self._plantel_myclub(s)
        fichas = [self._ficha_dt_myclub(i, d) for i, d in enumerate(dts)]
        return {"coach_list": fichas, "coach_list_num": len(fichas)}

    @staticmethod
    def _elegidos(club, jugadores, squad_id=1):
        """C (§17): los 18 inscritos (XI + 7) en el orden guardado; si no hay, los 18 primeros.
        Devuelve [(índice, jugador)]."""
        uid = lambda i, j: j.get("gameplayer_unique_id") or i + 1
        por_uid = {uid(i, j): (i, j) for i, j in enumerate(jugadores)}
        orden = [u for u in (club.get("inscritos") or {}).get(str(squad_id)) or [] if u in por_uid]
        if not orden:
            orden = [uid(i, j) for i, j in enumerate(jugadores)]
        return [por_uid[u] for u in orden[:18]]

    @staticmethod
    def _ids_plantillas(club):
        """B.15: plantillas del club (la 1 siempre existe; COPY/DELETE añaden o quitan otras)."""
        ids = [int(x) for x in club.get("plantillas_ids") or [] if isinstance(x, int)]
        return [1] + sorted(x for x in set(ids) if x != 1)

    @staticmethod
    def _dt_de_plantilla(club, dts, squad_id=1):
        """B.15: el entrenador asignado con SET_MYCLUB_SQUAD_COACH; si no hay, el primero."""
        uid = (club.get("dt_plantilla") or {}).get(str(squad_id))
        elegido = [d for d in dts if d.get("coach_unique_id") == uid]
        return elegido[:1] or dts[:1]

    def _plantilla_principal(self, s, squad_id=1):
        jugadores, dts = self._plantel_myclub(s)
        if not jugadores:
            return None
        club = self.datos.club(s.steam_hex or "") or {}
        sq = str(squad_id)
        plan = (club.get("gameplans") or {}).get(sq) or (club.get("gameplans") or {}).get("1") or {}
        elegidos = self._elegidos(club, jugadores, squad_id)
        dts = self._dt_de_plantilla(club, dts, squad_id)
        dorsales = (club.get("dorsales_plantilla") or {}).get(sq) or {}
        miembros = [Empaquetado({
            "gameplayer_unique_id": j.get("gameplayer_unique_id") or i + 1,
            "gameplayer_id": j.get("gameplayer_id") or 0,
            "uniform_number": dorsales.get(str(j.get("gameplayer_unique_id") or i + 1))
            or j.get("uniform_number") or i + 1,
            "player_level": j.get("level") or 1, "parameters": list(j.get("parameters") or []),
            "default_rarity": self._rareza(j)})
            for i, j in elegidos]
        plantilla = Empaquetado({
            "squad_id": int(squad_id),
            "name": (club.get("nombres_plantilla") or {}).get(sq) or f"Plantilla {squad_id}",
            "member_list": miembros,
            "member_num": len(miembros), "is_entry_compe": "NO",
            "coach_id_list": [d.get("coach_id") or 0 for d in dts[:1]],
            "coach_unique_id_list": [d.get("coach_unique_id") or 1 for d in dts[:1]],
            "gameplan_data_size": self._plan_texto(plan.get("gameplan_data"))[0],
            "gameplan_data": self._plan_texto(plan.get("gameplan_data"))[1],
            "is_club_edition": "NO",
            "club_edition_type": ""})
        return plantilla

    # ---------- myClub extra (docs/ESPEC_DIVISIONES_Y_MYCLUB_EXTRA.md, parte B) ----------
    NOMBRE_MAX = 32

    async def c_cmd_set_myclub_team_name(self, s, m):
        """B.1: guardar el nombre del club (y el corto y el id de locución); respuesta solo NOERR."""
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        if club is None:
            return {}
        nombre = m.get("team_name")
        corto = m.get("team_name_short")
        if isinstance(nombre, str) and nombre.strip():
            club["team_name"] = nombre.strip()[:self.NOMBRE_MAX]
        if isinstance(corto, str) and corto.strip():
            club["team_name_short"] = corto.strip()[:8]
        if isinstance(m.get("team_name_commentary_id"), int):
            club["team_name_commentary_id"] = m["team_name_commentary_id"]
        self.datos.guardar_club(s.steam_hex, club)
        log.info("myClub: %s cambia el nombre del club a %r (%r)", s.steam_hex, club.get("team_name"),
                 club.get("team_name_short"))
        return {}

    async def c_cmd_set_myclub_squad_name(self, s, m):
        """B.2: nombre de una plantilla (squad_id); respuesta solo NOERR."""
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        nombre = m.get("name")
        if club is None or not isinstance(nombre, str) or not nombre.strip():
            return {}
        club.setdefault("nombres_plantilla", {})[str(m.get("squad_id") or 1)] = nombre.strip()[:self.NOMBRE_MAX]
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_get_myclub_historylist(self, s, m):
        """B.3: historial vacío válido hasta conocer los tokens de history_data_kind/data_type."""
        try:
            inicio = max(0, int(m.get("start_index") or 0))
        except (TypeError, ValueError):
            inicio = 0
        return {"myclub_history_list_total": 0, "index": inicio, "myclub_history_list_num": 0,
                "myclub_history_list": []}

    async def c_cmd_get_myclub_presentlist(self, s, m):
        """B.4: sin regalos pendientes (lista vacía válida)."""
        return {"present_list_num": 0, "present_list": []}

    async def c_cmd_get_myclub_achievementlist(self, s, m):
        """B.5/B.17: logros del club. Apagados (lista vacía) salvo config myclub.logros.activo."""
        club = self._club_propio(s)
        lista = [self._logro_bin(club, l) for l in self._logros(club)] if club is not None else []
        return {"achievement_list_num": len(lista), "achievement_list": lista}

    async def c_cmd_send_myclub_read_achievementlist(self, s, m):
        return {}

    # ---------- logros (B.14 y B.17; los textos y el sentido de value/target son P) ----------
    # categoría: [(meta, GP del premio), ...]; el progreso sale del récord contra la COM
    LOGROS_DEFECTO = {"MATCH50": [[50, 10000]], "MATCH100": [[100, 20000]], "SUCCESSIVE5": [[5, 5000]]}

    def _logros(self, club):
        """[(categoría, progreso, [(meta, gp)], hitos cobrados)] o [] si están apagados."""
        cfg = (self.cfg.get("myclub") or {}).get("logros") or {}
        if not cfg.get("activo") or club is None:
            return []
        com = club.get("vscom") or {}
        progreso = {"MATCH50": int(com.get("partidos") or 0), "MATCH100": int(com.get("partidos") or 0),
                    "SUCCESSIVE5": int(com.get("racha_max") or 0)}
        # ojo: «logros_cobrados» (lista) ya lo usa GP_ACHIEVEMENT; los de myClub van aparte
        cobrados = club.get("logros_myclub") if isinstance(club.get("logros_myclub"), dict) else {}
        salida = []
        for cat, hitos in (cfg.get("premios") or self.LOGROS_DEFECTO).items():
            if cat not in progreso:
                continue  # sin forma de medirlo todavía (DIFFERENT20, POWERFUL_ENEMY, HIGH_SUCCESS)
            hitos = [(int(h[0]), int(h[1])) for h in hitos if isinstance(h, (list, tuple)) and len(h) == 2]
            if hitos:
                salida.append((cat, progreso[cat], hitos, min(int(cobrados.get(cat) or 0), len(hitos))))
        return salida

    @staticmethod
    def _premio_gp(n, gp):
        """Regalo de B.14 con todos los campos; el premio real (GP) lo suma el servidor al cobrar."""
        return Empaquetado({
            "present_id": n, "name_jp": f"{gp} GP", "title": f"{gp} GP", "entity_id": 0, "grade": 0,
            "player_level": 0, "contract_term": 0, "team_id": 0, "present_type": "NORMAL",
            "item_type": "", "item_variation": "", "item_effect_arg": 0, "count": gp,
            "pes_point_price": 0, "receipt_deadline": 0})

    def _logro_bin(self, club, logro):
        cat, valor, hitos, cobrados = logro
        vistos = club.get("logros_vistos") or []
        hitos_bin = [Empaquetado({"target_value": meta, "value": min(valor, meta),
                                  "received_milestone_level": cobrados,
                                  "present": self._premio_gp(i + 1, gp)})
                     for i, (meta, gp) in enumerate(hitos)]
        listo = cobrados < len(hitos) and valor >= hitos[cobrados][0]
        return Empaquetado({"achievement_category": cat,
                            "received_all": "YES" if cobrados >= len(hitos) else "NO",
                            "is_new": "YES" if listo and cat not in vistos else "NO",
                            "milestone_list_num": len(hitos_bin), "milestone_list": hitos_bin})

    async def c_cmd_set_myclub_achievement(self, s, m):
        """B.17: marcar un logro como visto; respuesta solo NOERR."""
        club = self._club_propio(s)
        cat = m.get("achievement_category")
        if club is not None and isinstance(cat, str) and cat:
            vistos = club.setdefault("logros_vistos", [])
            if cat not in vistos:
                vistos.append(cat)
                self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_recv_myclub_achievement_bonus(self, s, m):
        """B.17: cobrar los premios de logro alcanzados (uno o todos con is_all); suma los GP."""
        club = self._club_propio(s)
        todos = str(m.get("is_all") or "").upper() == "YES"
        pedido = m.get("achievement_category")
        recibidos = []
        for n, (cat, valor, hitos, cobrados) in enumerate(self._logros(club)):
            if not todos and cat != pedido:
                continue
            ganado = 0
            while cobrados < len(hitos) and valor >= hitos[cobrados][0]:
                ganado += hitos[cobrados][1]
                cobrados += 1
            if not ganado:
                continue
            club.setdefault("logros_myclub", {})[cat] = cobrados
            club["gp"] = int(club.get("gp") or 0) + ganado
            recibidos.append(Empaquetado({
                "achievement_category": cat, "achievement_id": n,
                "achievement_bonus": Empaquetado({"present": self._premio_gp(n + 1, ganado)}),
                "received_all": "YES" if cobrados >= len(hitos) else "NO",
                "received_milestone_level": cobrados}))
            log.info("myClub %s cobra el logro %s: +%d GP", s.steam_hex, cat, ganado)
        if recibidos:
            self.datos.guardar_club(s.steam_hex, club)
        return {"recv_achievement_list": recibidos}

    async def c_cmd_entry_myclub_weekly_ranking(self, s, m):
        """B.7: inscribirse (o no) al ranking semanal; se guarda en el club, respuesta solo NOERR."""
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        if club is not None:
            club["semanal_inscrito"] = str(m.get("is_entry") or "").upper() in ("YES", "1", "TRUE")
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_get_myclub_com_match_info(self, s, m):
        """B.13 (Ghidra): match_stats = bin con 8 enteros; récord del club contra la COM."""
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        com = (club or {}).get("vscom") or {}
        e = lambda k: int(com.get(k) or 0)
        return {"match_stats": Empaquetado({
            "match_num": e("partidos"), "win_num": e("ganados"), "lose_num": e("perdidos"),
            "draw_num": e("empates"), "goal": e("goles"), "lostGoal": e("goles_contra"),
            "contWin_num": e("racha"), "contWinMax_num": e("racha_max")})}

    async def c_cmd_get_myclub_cloud_squadid(self, s, m):
        """B.13: sin plantilla en la nube (P: 0 es lo seguro, sin captura)."""
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        return {"squad_id": self._nube(club)}

    # ---------- gestión del club (ESPEC_DIVISIONES_Y_MYCLUB_EXTRA.md, parte B.15) ----------
    def _club_propio(self, s):
        return self._club_inicial(s) if s.steam_hex else None

    @staticmethod
    def _id_entero(v):
        try:
            return int(v)
        except (TypeError, ValueError):
            return None

    async def c_cmd_set_myclub_squad_coach(self, s, m):
        """B.15.1: entrenador (y su plan de juego) de una plantilla; respuesta solo NOERR."""
        club = self._club_propio(s)
        sq = self._id_entero(m.get("squad_id")) or 1
        uid = self._id_entero(m.get("coach_unique_id"))
        if club is None:
            return {}
        if uid is None or not any(d.get("coach_unique_id") == uid for d in club.get("entrenadores") or []):
            return {"result": "ERR_NOTFOUND"}
        club.setdefault("dt_plantilla", {})[str(sq)] = uid
        if isinstance(m.get("gameplan_data"), str) and m["gameplan_data"]:
            plan = club.setdefault("gameplans", {}).setdefault(str(sq), {})
            plan["gameplan_data"] = m["gameplan_data"]
            if isinstance(m.get("tacticsPlanKind"), int):
                plan["tacticsPlanKind"] = m["tacticsPlanKind"]
        self.datos.guardar_club(s.steam_hex, club)
        log.info("myClub %s: plantilla %d con el entrenador %d", s.steam_hex, sq, uid)
        return {}

    async def c_cmd_sell_myclub_coach(self, s, m):
        """B.15.2: despedir (vender) un entrenador; respuesta gain_gp. El último entrenador o uno
        que dirige una plantilla no se puede vender."""
        club = self._club_propio(s)
        uid = self._id_entero(m.get("coach_unique_id"))
        dts = (club or {}).get("entrenadores") or []
        dt = next((d for d in dts if d.get("coach_unique_id") == uid), None)
        if dt is None:
            return {"result": "ERR_NOTFOUND"}
        en_uso = {(self._dt_de_plantilla(club, dts, sq) or [{}])[0].get("coach_unique_id")
                  for sq in self._ids_plantillas(club)}
        if len(dts) <= 1 or uid in en_uso:
            return {"result": "ERR_FORBIDDEN"}
        ganancia = self._venta_dt_gp(dt)
        club["entrenadores"] = [d for d in dts if d is not dt]
        club["gp"] = int(club.get("gp") or 0) + ganancia
        self.datos.guardar_club(s.steam_hex, club)
        log.info("myClub %s despidió al entrenador %s: +%d GP", s.steam_hex, dt.get("coach_id"), ganancia)
        return {"gain_gp": ganancia}

    def _venta_dt_gp(self, dt):
        if isinstance(dt.get("sell_gp"), int):
            return max(0, dt["sell_gp"])
        return max(0, int((self.cfg.get("myclub") or {}).get("venta_dt_gp") or 0))

    async def c_cmd_update_myclub_coach_contract(self, s, m):
        """B.15.3: renovar el contrato del entrenador (mismo precio y plazo que EXTEND_MYCLUB_COACH,
        siempre en GP); respuesta contract_term."""
        club = self._club_propio(s)
        uid = self._id_entero(m.get("coach_unique_id"))
        dt = next((d for d in (club or {}).get("entrenadores") or [] if d.get("coach_unique_id") == uid), None)
        if dt is None:
            return {"result": "ERR_NOTFOUND"}
        cfg = self.cfg.get("myclub") or {}
        error = self._cobrar(club, int(cfg.get("renovacion_gp") or 0), 0, False)
        if error:
            return {"result": error}
        dt["contract_term"] = int(dt.get("contract_term") or 0) + int(cfg.get("renovacion_partidos") or 10)
        self.datos.guardar_club(s.steam_hex, club)
        return {"contract_term": dt["contract_term"]}

    async def c_cmd_copy_myclub_squad(self, s, m):
        """B.15.4: copiar una plantilla (inscritos, plan, entrenador y nombre); respuesta squad_id."""
        club = self._club_propio(s)
        origen = self._id_entero(m.get("copy_squad_id")) or 1
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        ids = self._ids_plantillas(club)
        if origen not in ids:
            return {"result": "ERR_NOTFOUND"}
        if len(ids) >= int((self.cfg.get("myclub") or {}).get("max_plantillas") or 3):
            return {"result": "ERR_FORBIDDEN"}
        nuevo = max(ids) + 1
        o, n = str(origen), str(nuevo)
        for clave in ("inscritos", "gameplans", "dt_plantilla", "dorsales_plantilla"):
            if o in (club.get(clave) or {}):
                valor = club[clave][o]
                club[clave][n] = list(valor) if isinstance(valor, list) else \
                    dict(valor) if isinstance(valor, dict) else valor
        if isinstance(m.get("name"), str) and m["name"].strip():  # C (10-nov): el PES manda el nombre
            nombre_nuevo = m["name"].strip()[:self.NOMBRE_MAX]
        else:
            nombre = (club.get("nombres_plantilla") or {}).get(o) or f"Plantilla {origen}"
            nombre_nuevo = f"{nombre[:self.NOMBRE_MAX - 4]} ({nuevo})"
        club.setdefault("nombres_plantilla", {})[n] = nombre_nuevo
        club["plantillas_ids"] = ids + [nuevo]
        self.datos.guardar_club(s.steam_hex, club)
        log.info("myClub %s copió la plantilla %d en la %d", s.steam_hex, origen, nuevo)
        return {"squad_id": nuevo}

    async def c_cmd_delete_myclub_squad(self, s, m):
        """B.15.5: borrar una plantilla (la 1, la principal, no se borra); respuesta solo NOERR."""
        club = self._club_propio(s)
        sq = self._id_entero(m.get("squad_id"))
        if club is None or sq not in self._ids_plantillas(club):
            return {"result": "ERR_NOTFOUND"}
        if sq == 1:
            return {"result": "ERR_FORBIDDEN"}
        club["plantillas_ids"] = [x for x in self._ids_plantillas(club) if x != sq]
        for clave in ("inscritos", "gameplans", "dt_plantilla", "nombres_plantilla", "dorsales_plantilla"):
            (club.get(clave) or {}).pop(str(sq), None)
        if club.get("plantilla_activa") == sq:
            club["plantilla_activa"] = 1  # se borró la activa: vuelve a jugar la 1
        if club.get("plantilla_nube") == sq:
            club["plantilla_nube"] = 1  # (el PES ya no deja borrarla; por si acaso)
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_set_myclub_player_number_fixation(self, s, m):
        """B.15: fijar dorsales. Se guarda la lista y se aplica el número a los jugadores del club
        (player_id = id único o id del jugador); respuesta solo NOERR."""
        club = self._club_propio(s)
        if club is None:
            return {}
        fijados = {}
        for el in m.get("player_number_list") or []:
            if isinstance(el, (bytes, bytearray)):
                try:
                    el = msgpack_min.unpackb(el)
                except Exception:
                    continue
            if isinstance(el, dict) and isinstance(el.get("player_id"), int) \
                    and isinstance(el.get("number"), int) and 1 <= el["number"] <= 99:
                fijados[el["player_id"]] = el["number"]
        for j in club.get("jugadores") or []:
            num = fijados.get(j.get("gameplayer_unique_id"), fijados.get(j.get("gameplayer_id")))
            if num:
                j["uniform_number"] = num
        club["dorsales_fijos"] = [[k, v] for k, v in fijados.items()]
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_set_myclub_team_base(self, s, m):
        """B.15: «Copiar base del equipo» = base_team_id; respuesta solo NOERR."""
        club = self._club_propio(s)
        base = self._id_entero(m.get("base_team_id"))
        if club is not None and base is not None and base >= 0:
            club["base_team_id"] = base
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_set_myclub_hometown(self, s, m):
        """B.15 (C, captura del 10-nov): home_town llega como bin(packb({nation_id, label, name}));
        se acepta también texto suelto con nation_id/label arriba. Respuesta solo NOERR."""
        club = self._club_propio(s)
        if club is None:
            return {}
        casa = dict(club.get("home_town") or {}) if isinstance(club.get("home_town"), dict) else {}
        pedido = m.get("home_town")
        if isinstance(pedido, (bytes, bytearray)):
            try:
                pedido = msgpack_min.unpackb(pedido)
            except Exception:
                pedido = None
        if isinstance(pedido, dict):
            fuente = pedido
        else:
            fuente = {"name": pedido, "nation_id": m.get("nation_id"), "label": m.get("label")}
        if isinstance(fuente.get("name"), str):
            casa["name"] = fuente["name"][:64]
        if isinstance(fuente.get("nation_id"), int):
            casa["nation_id"] = fuente["nation_id"]
        if isinstance(fuente.get("label"), str):
            casa["label"] = fuente["label"][:64]
        club["home_town"] = casa
        log.info("myClub %s: ciudad del club %r (país %s)", s.steam_hex, casa.get("name"), casa.get("nation_id"))
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_set_myclub_add_uniform_id(self, s, m):
        """B.15: añadir camiseta. Se guarda; USER_INFO sigue mandando add_uniform_list vacía hasta
        conocer el formato de sus elementos. Respuesta solo NOERR."""
        club = self._club_propio(s)
        uni = self._id_entero(m.get("add_uniform_id"))
        if club is not None and uni is not None:
            lista = [u for u in club.get("uniformes_extra") or [] if u != uni]
            club["uniformes_extra"] = (lista + [uni])[-20:]
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_get_myclub_gameplayer_need_experiencelist(self, s, m):
        return {}  # B.15: la respuesta no tiene claves conocidas; solo NOERR


    async def c_cmd_get_myclub_squadlist(self, s, m):
        plantilla = self._plantilla_principal(s)
        if plantilla is None:
            return {"squad_list": [], "squad_list_num": 0, "edition_squad_list": [],
                    "edition_squad_list_num": 0, "main_squad_id": 0, "cloud_squad_id": 0}
        club = self.datos.club(s.steam_hex or "") or {}
        lista = [plantilla]
        for sq in self._ids_plantillas(club)[1:]:
            otra = self._plantilla_principal(s, sq)
            prueba = {"msgid": "CMD_GET_MYCLUB_SQUADLIST", "rqid": 0, "result": "NOERR",
                      "squad_list": lista + [otra], "squad_list_num": len(lista) + 1,
                      "edition_squad_list": [], "edition_squad_list_num": 0, "main_squad_id": 1,
                      "cloud_squad_id": 0}
            if otra is None or _cifrado_comprimido(prueba) > MAX_CIFRADO_CLIENTE:
                log.warning("myClub: SQUADLIST recortada a %d plantilla(s) (límite de trama)", len(lista))
                break
            lista.append(otra)
        return {"squad_list": lista, "squad_list_num": len(lista), "edition_squad_list": [],
                "edition_squad_list_num": 0, "main_squad_id": self._activa(club),
                "cloud_squad_id": self._nube(club)}

    async def c_cmd_get_myclub_main_squad(self, s, m):
        # C (05-nov, lector 141c06500): plantilla principal de target_pid. P: hoy solo la propia
        # (ver la de otro jugador necesita la identidad propia y buscar su club por pid).
        jugadores, dts = self._plantel_myclub(s)
        club = self.datos.club(s.steam_hex or "") or {}
        activa = self._activa(club)
        plantilla = self._plantilla_principal(s, activa)
        # C (05-nov 04:00): con las 18 fichas + DT medía ~1600 B cifrados; el envío troceado
        # hacia el cliente aún no lo acepta PES (pantalla en blanco → C_WNJJ_0002). Hasta
        # resolverlo, solo la plantilla (los miembros ya llevan id, dorsal, nivel y rareza);
        # config myclub.main_squad_completo = true para volver a mandar las fichas.
        completo = (self.cfg.get("myclub") or {}).get("main_squad_completo")
        fichas = [self._ficha_jugador_myclub(i, j) for i, j in self._elegidos(club, jugadores, activa)] \
            if completo else []
        dt = [self._ficha_dt_myclub(i, d) for i, d in enumerate(self._dt_de_plantilla(club, dts, activa))] \
            if completo else []
        return {"squad": plantilla or Empaquetado({}), "gameplayer_list": fichas,
                "gameplayer_list_num": len(fichas), "coach_list": dt, "coach_list_num": len(dt),
                "team_name": club.get("team_name") or "",
                "base_team_id": int(club.get("base_team_id") or 0)}

    async def c_cmd_get_myclub_gameplayer_experience(self, s, m):
        # C/P (05-nov, lector candidato 141bf0cf0): tras el partido, los jugadores con su
        # experiencia y nivel ya actualizados (misma ficha que GAMEPLAYERLIST)
        jugadores, _ = self._plantel_myclub(s)
        club = self.datos.club(s.steam_hex or "") or {}
        fichas = [self._ficha_jugador_myclub(i, j)
                  for i, j in self._elegidos(club, jugadores, self._activa(club))]
        return {"gameplayer_list": fichas, "gameplayer_list_num": len(fichas)}

    async def c_cmd_get_myclub_game_bonus(self, s, m):
        # C (05-nov, lector 141c7a820): premio del último partido. Listas vacías = sin extras.
        club = self._club_inicial(s) or {}
        u = (club.get("vscom") or {}).get("ultimo") or {}
        nivel = self._nivel_cpu(club)
        antes = (club.get("vscom") or {}).get("nivel_antes") or nivel
        return {"gain_gp": int(u.get("gp") or 0), "gp": int(club.get("gp") or 0),
                "base_gp": int(u.get("gp") or 0), "game_res": u.get("desenlace") or "",
                "match_num": int((club.get("vscom") or {}).get("partidos") or 0),
                "new_cpu_level": nivel, "old_cpu_level": antes,
                "is_first_win": "YES" if u.get("primera_victoria") else "NO",
                "gp_bonus_list": [], "gp_bonus_list_num": 0,
                "gp_bonus_elected_myplayer_num": 0, "special_match_bonus_list": [],
                "squad_bonus_item_list": []}

    async def c_cmd_get_myclub_procurable_gameplayerlist(self, s, m):
        return {"gameplayer_list": [], "gameplayer_list_num": 0}

    async def c_cmd_set_myclub_club_edition_info(self, s, m):
        return {}  # C: el lector solo mira msgid/rqid/result

    def _lado_propio(self):
        """El jugador ocupa el lado contrario al del rival COM (config myclub.rival.side)."""
        r = ((self.cfg.get("myclub") or {}).get("rival") or {}).get("side", "AWAY")
        return "AWAY" if r == "HOME" else "HOME"

    def _desenlace_vscom(self, m):
        """C (05-nov, 2 partidos reales): el ganador llega en winner_side ∈ WINNER_HOME/AWAY/NONE
        (NONE = empate); NO llegan goles. Otros campos: gameend_reason (NORMAL_END, INJURY…),
        cpu_level «NORMAL», ex_flag/pk_flag, rtt. P: los pares de goles quedan por si acaso.
        Devuelve WIN/DRAW/LOSE/"".
        Decisión de Frank (05-nov): abandonar cuenta como derrota. Se apaga con
        myclub.abandono_es_derrota = false; los motivos se cambian con myclub.motivos_abandono."""
        cfg = self.cfg.get("myclub") or {}
        if cfg.get("abandono_es_derrota", True) and \
                m.get("gameend_reason") in (cfg.get("motivos_abandono") or ["GIVEUP"]):
            return "LOSE"
        orden = [m.get("winner_side")] + list(m.values())
        for v in orden:
            if isinstance(v, str) and v.startswith("WINNER_"):
                lado = v[len("WINNER_"):]
                if lado in ("DRAW", "NONE"):
                    return "DRAW"
                propio = m.get("side") if isinstance(m.get("side"), str) else self._lado_propio()
                return "WIN" if lado == propio else "LOSE"
        pares = (("score", "opponent_score"), ("home_score", "away_score"))
        for a, b in pares:
            if isinstance(m.get(a), int) and isinstance(m.get(b), int):
                x, y = m[a], m[b]
                if a == "home_score" and m.get("side") == "AWAY":
                    x, y = y, x
                return "WIN" if x > y else "DRAW" if x == y else "LOSE"
        return ""

    def _guardar_partido_vscom(self, s, m):
        """Guarda el partido vs COM en el club de la persona y le da los GP de config
        (myclub.gp_por_partido = {"WIN": n, "DRAW": n, "LOSE": n, "": n})."""
        # C (05-nov): al ENTRAR a myClub el cliente manda un GAMERESULT sin partido
        # (past_game_played_pid 0, pes_srv_rtt_nums 0, WINNER_NONE): no se guarda ni da GP
        if not m.get("past_game_played_pid") and not m.get("pes_srv_rtt_nums"):
            log.info("myClub: GAMERESULT sin partido (al entrar), ignorado")
            return
        club = self._club_inicial(s)
        if club is None:
            return
        desenlace = self._desenlace_vscom(m)
        tabla = (self.cfg.get("myclub") or {}).get("gp_por_partido") or {}
        premio = int(tabla.get(desenlace, 0) or 0)
        com = club.setdefault("vscom", {"partidos": 0, "racha": 0, "historial": []})
        com["partidos"] = int(com.get("partidos") or 0) + 1
        com["racha"] = int(com.get("racha") or 0) + 1 if desenlace == "WIN" else 0
        contador = {"WIN": "ganados", "DRAW": "empates", "LOSE": "perdidos"}.get(desenlace)
        if contador:
            com[contador] = int(com.get(contador) or 0) + 1
        com["racha_max"] = max(int(com.get("racha_max") or 0), com["racha"])
        self._ajustar_dificultad(com, desenlace)
        simple = {k: v for k, v in m.items() if isinstance(v, (int, str)) and k not in ("msgid", "rqid")}
        com.setdefault("historial", []).append({"hora": int(time.time()), "desenlace": desenlace,
                                                "gp": premio, "datos": simple})
        com["historial"] = com["historial"][-50:]
        club["gp"] = int(club.get("gp") or 0) + premio
        if self._monedas_activas():
            tabla_m = (self.cfg.get("myclub") or {}).get("monedas_por_partido") or {}
            club["monedas"] = int(club.get("monedas") or 0) + int(tabla_m.get(desenlace, 0) or 0)
        victorias_antes = sum(1 for h in com["historial"][:-1] if h.get("desenlace") == "WIN")
        com["ultimo"] = {"desenlace": desenlace, "gp": premio,
                         "primera_victoria": desenlace == "WIN" and victorias_antes == 0}
        # Experiencia para los 18 inscritos (P: valores de config myclub.exp_por_partido)
        if not club.get("jugadores"):
            club["jugadores"] = self._plantel_inicial()[0]
        tabla_exp = (self.cfg.get("myclub") or {}).get("exp_por_partido") or {
            "WIN": 60, "DRAW": 40, "LOSE": 30, "": 30}
        ganada = int(tabla_exp.get(desenlace, 0) or 0)
        cfgm = self.cfg.get("myclub") or {}
        # C (05-nov noche): cada partido gasta contrato (myclub.contrato_por_partido, 0 = apagado;
        # ojo: PES muestra 100 como «1», ver §39.x) y cansa a los inscritos (myclub.cansancio).
        gasto = self._partidos_a_unidad(cfgm.get("contrato_por_partido") or 0)
        cans = cfgm.get("cansancio") or {}
        inscritos = set()
        for _, j in self._elegidos(club, club["jugadores"], self._activa(club)):
            inscritos.add(id(j))
            j["exp_total"] = int(j.get("exp_total") or 0) + ganada
            j["games"] = int(j.get("games") or 0) + 1
            if gasto:
                j["contract_term"] = max(0, self._contrato(j) - gasto)
            if cans.get("activo"):
                j["tiredness"] = min(int(cans.get("maximo", 100)),
                                     int(j.get("tiredness") or 0) + int(cans.get("sube", 10)))
        if cans.get("activo"):
            for j in club["jugadores"]:
                if id(j) not in inscritos:
                    j["tiredness"] = max(0, int(j.get("tiredness") or 0) - int(cans.get("baja", 10)))
        self.datos.guardar_club(s.steam_hex or "", club)
        log.info("myClub vs COM %s: %s, +%d GP (saldo %d)", s.steam_hex, desenlace or "?",
                 premio, club["gp"])

    async def c_cmd_set_myclub_vscom_gameresult(self, s, m):
        self._guardar_partido_vscom(s, m)
        j = self.datos.jugador(s.steam_hex or "") or {}
        nombre = j.get("pname") or s.pname or ""
        cuenta = self._numero_cuenta(s)
        return {"gameend_reason": m.get("gameend_reason") if isinstance(m.get("gameend_reason"), str) else "",
                "player_data": Empaquetado({
                    "pid": self._pid_visible(s),
                    "pes_pname": Empaquetado({"name": nombre, "xuid_one": str(cuenta),
                                              "psn_user_id": Empaquetado({
                                                  "account_id": cuenta, "online_id": nombre,
                                                  "is_valid_cache": "1"}),
                                              "is_deleted": "0"}),
                    "rating_type": "", "rating": RATING_INICIAL, "oldrating": RATING_INICIAL,
                    "rating_boost_type": "", "rating_boost_rate": 0, "rating_compe": 0,
                    "team_id": m.get("team") if isinstance(m.get("team"), int) else 0,
                    "side": "", "contWin_num": 0, "skill_rank": "",
                    "is_rankup_skill_rank": "NO", "is_first_skillrank": "NO", "is_first": "NO"})}

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
        pareja = self._pareja_por_clave(s)
        if pareja and pareja.get("room_id"):
            try:
                self._resultado_division(s, pareja, m)
            except Exception:
                log.exception("no se pudo aplicar el resultado de Divisiones")
            datos = {k: v for k, v in m.items() if k not in ("msgid", "rqid")}
            partido = f"pareja{pareja['room_id']}"
            self.datos.guardar_resultado(pareja["room_id"], partido, s.pid or 0, datos)
            res = self.datos.resultados(partido)
            if len(res) >= 2:
                iguales = len({repr(v.get("result", v)) for v in res.values()}) == 1
                log.info("partido %s cerrado; los dos lados %s", partido,
                         "coinciden" if iguales else "NO coinciden (revisar)")
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
        self._dejar_pareja(s, "fin anormal")
        return {}

    async def c_cmd_leave_game(self, s, m):
        self._abandono_division(s)
        self._dejar_pareja(s, "salió del partido")
        return {}

    def _abandono_division(self, s):
        """L (A.2, CLEAN_LEAVE): quien SALE con el partido de Divisiones en juego (ya hubo canal 9)
        y sin resultado aún pierde 3-0. Un corte de conexión no entra aquí (es ambiguo: no cuenta)."""
        pareja = self._pareja_por_clave(s)
        if not pareja or not pareja.get("room_id") or s not in pareja["miembros"]:
            return
        reglas = self._reglas_division()
        if not reglas.abandono_3_0 or pareja.get("tipo") not in reglas.tipos:
            return
        if self.rele is None or not hasattr(self.rele, "en_juego") or not self.rele.en_juego(pareja["room_id"]):
            return
        if self.datos.partido_division(f"div{pareja['room_id']}") is not None:
            return  # ya hay resultado: salir después del final no es abandono
        sale_casa = s is pareja["miembros"][0]
        log.info("partido div%s: %s abandona con el partido en juego: pierde 3-0", pareja["room_id"], s.pid)
        self._resultado_division(s, pareja, {"home_score": 0 if sale_casa else 3,
                                             "away_score": 3 if sale_casa else 0,
                                             "abnormalend_reason": "NORMAL_END"})

    async def c_cmd_leave_room(self, s, m):
        if s in self.parejas:
            self._abandono_division(s)
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
    def _nodos_relay(self, s=None, ip_local=None):
        """Nodos UDP del relé que se anuncian al cliente (C: índice 1, IP del servidor, 5100, grid 0)."""
        g = self.cfg.get("gamerelay") or {}
        if not g.get("activo", True):
            return []
        ip = str(g.get("ip") or "auto")
        return [{"gamerelay_index": int(g.get("indice", 1)),
                 "ip_addr": ip if ip.lower() != "auto" else self.ip_anuncio(s, ip_local),
                 "port": int(g.get("puerto", 5100)), "grid": int(g.get("grid", 0))}]

    def lista_calidad(self, pid, ip_cliente=None, ip_local=None, s=None):
        """Respuesta de GET_GAMERELAY_QUALITYCHECKLIST (HTTP o TCP).

        Efecto lateral (C): autoriza una sonda UDP desde esa IP contra cada nodo hasta que
        el pid se desconecte (tope 86 400 s)."""
        nodos = self._nodos_relay(s, ip_local)
        if ip_cliente:
            hasta = time.time() + 86400
            for n in nodos:
                self.sondas[(ip_cliente, n["gamerelay_index"])] = (str(pid or ""), hasta)
        medidas = self.calidades.get(str(pid or ""), {})
        g = self.cfg.get("gamerelay") or {}
        # C (04-nov, Divisiones): con los nodos como bin el cliente no los lee (reporta índice 0
        # y 0xFFFFFFFF sin mandar UDP). L (PESBUL): aquí van como MAPAS EN LÍNEA, umbrales 150/200.
        return {"quality_check_data_list": [dict(n) for n in nodos],
                "quality_check_data_num": len(nodos),
                "quality_threshold_1": int(g.get("umbral_1", 150)),
                "quality_threshold_2": int(g.get("umbral_2", 200)),
                # lista vacía = «mídelo tú» (L): así PES vuelve a medir cada vez
                "quality_list": [dict(v) for v in medidas.values()] if g.get("recordar") else [],
                "quality_list_num": len(medidas) if g.get("recordar") else 0}

    def sonda_autorizada(self, ip_cliente, indice):
        x = self.sondas.get((ip_cliente, indice))
        return bool(x and x[1] > time.time())

    async def c_cmd_get_gamerelay_qualitychecklist(self, s, m):
        return self.lista_calidad(self._pid_visible(s), s.ip, s=s)

    def respuesta_gamerelay(self, s=None, ip_local=None):
        """Nodo del relé para la partida. P (10-nov): mismos campos que el checklist, como mapas
        en línea (con bin el cliente no leyó los nodos del checklist)."""
        nodos = self._nodos_relay(s, ip_local)
        web = self._url_gamerelay_web(s) if s is not None else (
            self._con_ip(self.cfg.get("http_base"), ip_local=ip_local) or
            f"http://{self.ip_anuncio(ip_local=ip_local)}:{self.cfg.get('puerto_http', 5000)}") + "/gamerelay"
        r = {"gamerelay_web_url": web, "gamerelay_num": len(nodos),
             "gamerelay_list": [dict(n) for n in nodos]}
        if nodos:
            # C (PESBUL R7.3.494/602, comparado el 10-nov): el cliente copia a su objeto de
            # resultado SOLO los campos sueltos gamerelay_index/ip_addr/port/grid (si no llegan
            # queda «NotImplement»/0 y el SEND_MATCHING_RESULT sale con MXNV_0010)
            r.update({"gamerelay_index": nodos[0]["gamerelay_index"], "ip_addr": nodos[0]["ip_addr"],
                      "port": nodos[0]["port"], "grid": nodos[0]["grid"]})
        return r

    async def c_cmd_get_gamerelay(self, s, m):
        return self.respuesta_gamerelay(s)

    async def c_cmd_get_matchid(self, s, m):
        # C (prueba 10-nov): el cliente lo pide justo tras el FINISH (campos client/svrtype).
        # P: el id de la partida = la sala sintética de la pareja (700 000 000 + n)
        pareja = self._pareja_por_clave(s)
        mid = int(pareja.get("room_id") or 0) if pareja else 0
        # C (PESBUL main.py 21668): match_id es TEXTO «PESBUL-<10 dígitos>» + upload «YES»
        return {"match_id": "PHOENIX-%010d" % mid, "upload": "YES"}

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
        # C (05-nov): el cliente la pide antes del partido y pone all_gp en la barra superior;
        # con 0 fijo la barra quedaba en «G 0» tras jugar. Se devuelve el saldo del club.
        club = self._club_inicial(s) or {}
        return {"all_gp": int(club.get("gp") or 0),
                "all_achievement_bit_array": list(club.get("logros_bits") or []),
                "pes_point": self._monedas(club)}

    async def c_cmd_set_gp_achievement(self, s, m):
        # C (05-nov, lectores 141bb27a0/141bb2e40): el cliente manda new_achievement_bit_array
        # (palabras de bits de los logros de la sesión, SIN importes) y espera all_gp y
        # all_achievement_bit_array (todos los ya cobrados). P: cada bit NUEVO paga
        # myclub.gp_logros["<bit>"] o myclub.gp_logro_defecto (300). Bit = palabra*32 + bit.
        club = self._club_inicial(s)
        if club is None:
            return {"all_gp": 0, "all_achievement_bit_array": []}
        nuevos = [int(x) & 0xFFFFFFFF for x in m.get("new_achievement_bit_array") or []
                  if isinstance(x, int)]
        viejos = list(club.get("logros_bits") or [])
        viejos += [0] * (len(nuevos) - len(viejos))
        cfg = self.cfg.get("myclub") or {}
        tabla = cfg.get("gp_logros") or {}
        por_defecto = int(cfg.get("gp_logro_defecto", 300))
        ganado, bits = 0, []
        for i, palabra in enumerate(nuevos):
            recien = palabra & ~viejos[i]
            for b in range(32):
                if recien >> b & 1:
                    bits.append(i * 32 + b)
                    ganado += int(tabla.get(str(i * 32 + b), por_defecto))
            viejos[i] |= palabra
        log.info("myClub %s logros nuevos %s (+%d GP), crudo %s", s.steam_hex, bits, ganado, nuevos)
        club["logros_bits"] = viejos
        if ganado:
            club["gp"] = int(club.get("gp") or 0) + ganado
            club.setdefault("logros_cobrados", []).append(
                {"hora": int(time.time()), "bits": bits, "gp": ganado})
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"all_gp": int(club.get("gp") or 0), "all_achievement_bit_array": viejos}

    async def c_cmd_get_myclub_commentary_info(self, s, m):
        # C (05-nov, lector 141c4ced0): datos para el comentarista. P: listas vacías válidas.
        com = (self._club_inicial(s) or {}).get("vscom") or {}
        e = lambda k: m.get(k) if isinstance(m.get(k), int) else 0
        return {"team_spirit_prematch": e("team_spirit"),
                "team_power_level_prematch": e("team_power_level"),
                "total_match_num": int(com.get("partidos") or 0),
                "cont_win_num": int(com.get("racha") or 0),
                "cont_win_max": int(com.get("racha_max") or com.get("racha") or 0),
                "coach_match_num": int(com.get("partidos") or 0),
                "is_coach_norma_clear": "NO", "coach_extended": "NO",
                "gameplayer_extended_list": [], "division_stats": [],
                # C (05-nov, crash 21:44 en 141c4e427): get_player_result es LISTA de bin
                # {gameplayer_id, num}; un mapa suelto hacía caer a PES en la carga
                "get_player_result": []}

    async def c_cmd_send_match_team_info(self, s, m):
        # C (05-nov, lector 141d63cc0): solo msgid/rqid/result. Se guarda la alineación jugada
        # para unirla al próximo resultado vs COM.
        s.ultima_alineacion = m.get("match_team_info")
        return {}

    async def c_cmd_get_store_transaction_state(self, s, m):
        return {"state": 0, "product_id": "", "use_num": 0, "possession_num": 0,
                "live_transaction_id": "", "transaction_id": "",
                "used_present_product_id_list": [], "used_present_product_id_list_num": 0,
                "steam_order_id": ""}

    def respuesta_http(self, formulario, ip_cliente=None, ip_local=None):
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
            return {**base, **self.lista_calidad(formulario.get("pid"), ip_cliente, ip_local)}
        if msgid in ("CMD_GET_GAMERELAY", "GET_GAMERELAY"):
            # C (prueba 10-nov): tras el FINISH el cliente lo pide por HTTP (POST /get_gamerelay,
            # campos msgid/pid/rqid/urltype); sin respuesta manda CHECK_RES ER_MATCHING_RETRY y
            # SEND_MATCHING_RESULT con gamerelay_ip_addr «NotImplement» (MXNV_0010)
            return {**base, **self.respuesta_gamerelay(ip_local=ip_local)}
        if msgid in ("CMD_GET_AVATAR_HTTP", "CMD_GET_AVATAR"):
            return {**base, **self._avatar(formulario.get("pid"))}
        if msgid in ("CMD_GET_RANKING", "CMD_GET_RANKING_BYID", "CMD_GET_DIVISION_RANKING",
                     "CMD_GET_DIVISION_RANKING_BYID"):
            r = self._ranking_division(formulario)
            if msgid.endswith("_BYID"):
                r = self._solo_buscado(r, formulario)
            return {**base, **r}
        if msgid in ("CMD_GET_MYCLUB_RANKING", "CMD_GET_MYCLUB_RANKING_BYID"):
            r = self._ranking_myclub(formulario)
            if msgid.endswith("_BYID"):
                r = self._solo_buscado(r, formulario)
            return {**base, **r}
        if msgid in ("CMD_GET_MYCLUB_WEEKLY_RANKING", "CMD_GET_MYCLUB_WEEKLY_RANKING_BYID"):
            # B.7: sin torneo semanal propio todavía: respuesta vacía válida
            return {**base, "date": int(time.time()), "total_rank_num": 0, "rank_list": [],
                    "max_weekly_point": 0, "group_number": 0, "division": "", "reward_list": []}
        if msgid == "CMD_RECV_MYCLUB_PRESENT":
            # B.4: no hay regalos pendientes que recibir
            return {**base, "receive_result_list_num": 0, "receive_result_list": []}
        if msgid == "CMD_AUTH_STEAM":
            # Ticket = GetAuthSessionTicket en hex + SteamID64 en claro. Verificar la firma
            # del ticket queda pendiente; hoy la identidad la garantiza el vínculo de Link.
            resumen = {k: (f"<{len(v)} car.>" if len(str(v)) > 40 else v)
                       for k, v in formulario.items()}
            log.info("HTTP CMD_AUTH_STEAM %s", resumen)
            return base
        log.warning("comando HTTP sin manejador: %s campos=%s", msgid, sorted(formulario))
        return base

    def _ranking_myclub(self, m):
        """L (B.6/B.12): rank_list de MAPAS directos {rank, user{pid, pname, division, ..., team_id,
        team_name}}. Orden propio de Phoenix: puntos vs COM (3 por victoria, 1 por empate) y luego
        menos partidos."""
        try:
            inicio = max(0, int(m.get("start_index") or 0))
            cuantos = min(100, max(1, int(m.get("view_num") or 50)))
        except (TypeError, ValueError):
            inicio, cuantos = 0, 50
        filas = []
        for clave in self.datos.claves_de_clubes():
            club = self.datos.club(clave) or {}
            com = club.get("vscom") or {}
            g, e, p = int(com.get("ganados") or 0), int(com.get("empates") or 0), int(com.get("perdidos") or 0)
            filas.append((3 * g + e, -int(com.get("partidos") or 0), clave, club, (g, e, p, com)))
        filas.sort(key=lambda f: (-f[0], -f[1], f[2]))
        lista = []
        for i, (_pts, _np, clave, club, (g, e, p, com)) in enumerate(filas[inicio:inicio + cuantos]):
            j = self.datos.jugador(clave) or {}
            perfil = self.datos.perfil(clave)
            nombre = j.get("pname") or ""
            nombre_club = club.get("team_name") or (f"{nombre} FC" if nombre else "Phoenix FC")
            # C (prueba 10-nov): con team_name solo dentro de «user» el PES pintaba «NotImplement»
            # (su valor por defecto = clave no leída): va también al nivel del elemento (P)
            lista.append({"rank": inicio + i + 1, "team_id": int(club.get("base_team_id") or 0),
                          "team_name": nombre_club, "user": {
                "pid": self.numero_de_clave(clave) or j.get("pid") or 0,
                "pname": {"name": nombre, "xuid_one": str(self.numero_de_clave(clave) or 0),
                          "psn_user_id": {"account_id": 0, "online_id": nombre, "is_valid_cache": "NO"},
                          "is_deleted": "NO"},
                "division": self._estado_division(clave)["division"], "manner": MODALES_DEFECTO,
                "favoriteTeam": perfil["favorite_team"], "favoritePlayer": perfil["favorite_player"],
                "intro": perfil["intro"],
                "match_record": {"win_count": g, "draw_count": e, "lose_count": p,
                                 "win_s_max": int(com.get("racha_max") or 0), "rating": RATING_INICIAL},
                # C (prueba 10-nov): sin nombre el PES pinta «NotImplement»: nombre del jugador
                "team_id": int(club.get("base_team_id") or 0),
                "team_name": nombre_club}})
        return {"date": int(time.time()), "total_rank_num": len(filas), "rank_list": lista}

    @staticmethod
    def _solo_buscado(r, m):
        """_BYID: la fila de la persona buscada (por pid, xuid_one o np_account_id)."""
        # «pid» es quien pregunta; el buscado viene en xuid_one / np_account_id (o target_pid)
        buscados = {str(m.get(k)) for k in ("target_pid", "xuid_one", "np_account_id")
                    if m.get(k) not in (None, "", "0", 0, "NotImplement")} or {str(m.get("pid"))}
        filas = [f for f in r["rank_list"] if str(f["user"]["pid"]) in buscados
                 or str(f["user"]["pid"] & 0xFFFFFFFF) in buscados]
        return {**r, "rank_list": filas[:1]}

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
