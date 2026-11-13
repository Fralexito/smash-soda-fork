"""Lógica del servidor: sesiones, login, lobby, salas del amistoso 1v1 y TURN.

Todo sale de cap_72 §11 (especificación v1, sala limpia). Lo que la
especificación no fija con certeza está marcado «P» y concentrado en
constantes para corregirlo con REA sin tocar el resto.
"""
import asyncio
import base64
import hashlib
import hmac
import ipaddress
import itertools
import json
import logging
import os
import random
import re
import time
from types import SimpleNamespace

from . import divisiones as div
from . import msgpack_min
from .nclmio import MAX_CIFRADO_CLIENTE
from .msgpack_min import Empaquetado

log = logging.getLogger("phoenix_online")

NOERR = "NOERR"

# Interruptores de emergencia (config «interruptores.apagados», se recargan en caliente).
# Cada módulo se corta en su ENTRADA: el PES recibe un error y vuelve al menú; el resto sigue.
MODULOS_INTERRUPTOR = {
    "divisiones": {"CMD_START_MATCHING": "OC_RANK"},
    "partida_rapida": {"CMD_START_MATCHING": "OC_QUICK"},
    "salas": {"CMD_CREATEJOIN_ROOM": None, "CMD_JOIN_ROOM": None},
    "cooperativo": {"CMD_START_COOP_MATCHING": None, "CMD_CREATEJOIN_COOP_ROOM": None,
                    "CMD_JOIN_COOP_ROOM": None},
    "matchday": {"CMD_JOIN_MATCHDAY": None},
    "pesleague": {"CMD_JOIN_PESLEAGUE_COMPE": None},
    "eventos": {"CMD_JOIN_COMPEV": None, "CMD_REENTRY_COMPEV": None},
    "chat": {"CMD_SEND_TEXTCHAT": None},
    "clanes": {"CMD_CREATE_CLAN": None, "CMD_ENTRY_CLAN": None, "CMD_INVITE_CLAN": None},
    "subastas": {"CMD_TENDER_MYCLUB_AUCTION_SCOUT": None},
    "intercambio": {"CMD_TRADE_MYCLUB_GAMEPLAYER": None},
    "turn": {},  # se mira en respuesta_http: sin lista TURN el PES intenta solo conexión directa
}
# Claves de primer nivel de config.json que se pueden cambiar en caliente sin riesgo (además de myclub)
CLAVES_EN_CALIENTE = ("interruptores", "max_partidos_rival_dia", "sonda_vida", "revision_check",
                      "veredicto_al_final", "emparejar_mismos_datos", "exigir_versiones",
                      "direccion_para_remoto", "palabras_bloqueadas")

# Comandos que solo esperan msgid/rqid/result (cap_72 §11.4)
CONFIRMACIONES = {
    "CMD_SEND_PSID", "CMD_SET_CURRENT_PLAYER", "CMD_LOGIN_MENU", "CMD_SET_LANGUAGE",
    "CMD_JOIN_OFFLINE_MODE",
    "CMD_LEAVE_OFFLINE_MODE", "CMD_SEND_HEARTBEAT",
    # L (PESBUL): estos también se contestan solo con NOERR
    "CMD_CHECK_CHEAT", "CMD_GET_DMM_NOTIFICATION",
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
        self.pid_visible = 0  # pid que el PES conoce como suyo (se fija al crear o unirse a una sala)
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
        self.max_players = max(2, min(self.max_players, 2))  # R: 2 fijo en el vestíbulo 1 vs 1
        self.partido = None
        # R (rooms.md/livetests.md AzrakHQ, probado con 2 PES): número de entrada por jugador tras
        # «Empezar partido», quiénes ya mandaron ENTRY_GAME (solo a ellos se les actualiza la lista de
        # jugadores), quiénes salieron del partido y el número de partido de la sala
        self.entradas = {}
        self.escuchan = set()
        self.salieron = set()
        self.game_no = 0

    @staticmethod
    def pid_de(s):
        """pid que el PES conoce como SUYO (el de GET_PLAYER_DATA = _pid_visible). Desde ae851ea
        (10-nov) ya no es el número interno s.pid; con s.pid el dueño no se encontraba en la lista de
        jugadores de su sala y salía VZLN_0002 al crearla (prueba 11-nov 19:50)."""
        return getattr(s, "pid_visible", 0) or s.pid or 0

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
        if env.get("limitTime") in (None, "", "NOSET"):
            env["limitTime"] = "MIDDLE"  # R (roomlist.md): con NOSET la sala no sale en ninguna lista
        return {
            "slot": slot, "room_id": self.room_id,
            "name": self.nombre, "room_name": self.nombre, "roomName": self.nombre,
            "status": self.estado if isinstance(self.estado, str) else "WAITING",
            "game_phase": "DATASYNC", "gameenv": Empaquetado(env),
            "gamer_num": len(self.miembros),
            "gamer": [Empaquetado({"pid": self.pid_de(x), "is_room_owner": _texto_si(x is self.dueno),
                                   "side": "HOME" if x is self.dueno else "AWAY", "enter_no": i + 1,
                                   "game_no": self.game_no}) for i, x in enumerate(self.miembros)],
            "is_room_owner": _texto_si(para is self.dueno), "enter_no": entra, "game_no": self.game_no,
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
        # R (roomcrash.md/rooms.md AzrakHQ, probado con 2 PES): side HOME/AWAY, is_quality_bad y
        # has_guest «NO», game_entry_no = nº de entrada tras «Empezar partido» (0 antes) y SIN guest_info
        # cuando has_guest es NO
        fila = {
            "pid": self.pid_de(s), "room_player_id": numero + 1,
            "room_entry_no": numero + 1, "game_entry_no": int(self.entradas.get(s) or 0),
            "pname": self.pname(s),
            "lang": s.lang if isinstance(s.lang, str) and s.lang else "en",
            "is_owner": _texto_si(s is self.dueno), "side": "HOME" if s is self.dueno else "AWAY",
            "is_quality_bad": "NO", "has_guest": "NO",
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
        self.ruta_config = None
        self._mtime_config = None
        self.sesiones = set()
        self.por_pid = {}
        self.salas = {}
        self.suscritos_salas = set()  # quienes miran la lista de salas (REQWATCHL_ROOMLIST)
        self.turn_propio = None  # relé TURN propio (turn_propio.py), si config turn_propio.activo
        self.turn_pendiente = {}  # pid destino -> (address, size)
        self.dormir = asyncio.sleep
        self.avatares = {}    # pid (texto) -> base64 del avatar
        self.sondas = {}      # (ip, índice de nodo) -> (pid, hasta): sondas UDP autorizadas
        self.calidades = {}   # pid -> {índice: medida} de SET_GAMERELAY_QUALITY
        self.colas = {}       # matching_type -> [Sesion] buscando rival
        self.parejas = {}     # Sesion -> Pareja de partida rápida
        self.inicio = time.time()
        self.sin_manejador = {}  # msgid -> veces (para priorizar qué implementar)
        self.salas_coop = {}      # room_id -> sala cooperativa (D.8, en memoria)
        self._ids_coop = itertools.count(1)
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
        self.suscritos_salas.discard(sesion)
        self._salir_de_cola(sesion)
        self._salir_de_sala_coop(sesion)
        pareja = self._pareja_por_clave(sesion)
        # R (evoweb, hilo PESBUL p. 9): por la sesión EXACTA. El PES abre más de una conexión; si otra
        # del mismo jugador sigue viva, cerrar esta no es un abandono
        sigue = bool(sesion.steam_hex) and any(x.steam_hex == sesion.steam_hex for x in self.sesiones)
        if pareja is not None and pareja.get("en_juego") and not pareja.get("terminado") \
                and not pareja.get("abandonaron") and not sigue:
            # R (pesmanner.dropped): se cerró su PES con el partido en marcha
            pareja.setdefault("abandonaron", set()).add(sesion)
            self._anotar_modales((sesion.steam_hex or "").lower(), True, "DISCONNECTED")
            self._programar_derrota(pareja)
        self._dejar_pareja(sesion, "se cortó su conexión")
        # C: la sonda autorizada se revoca al desconectar el TCP de ese pid
        pid = str(self._pid_visible(sesion))
        if pid not in {str(self._pid_visible(x)) for x in self.sesiones}:
            for k in [k for k, v in self.sondas.items() if v[0] == pid]:
                del self.sondas[k]

    # ---------- despacho ----------
    def _recargar_config(self):
        """Recarga en caliente cfg['myclub'] y las CLAVES_EN_CALIENTE (interruptores…; fechas de torneos, tokens de prueba…) cuando cambia
        config.json, para probar valores sin reiniciar servidor ni PES. Nunca toca claves, turn ni red."""
        ruta = self.ruta_config
        if not ruta:
            return
        try:
            m = os.path.getmtime(ruta)
            if self._mtime_config is None:
                self._mtime_config = m
                return
            if m == self._mtime_config:
                return
            self._mtime_config = m
            with open(ruta, encoding="utf-8") as f:
                nuevo = json.load(f)
            if isinstance(nuevo.get("myclub"), dict):
                self.cfg["myclub"] = nuevo["myclub"]
                log.info("config.json: myclub recargado en caliente")
            cambiadas = [k for k in CLAVES_EN_CALIENTE if nuevo.get(k) != self.cfg.get(k)]
            for k in cambiadas:
                if k in nuevo:
                    self.cfg[k] = nuevo[k]
                else:
                    self.cfg.pop(k, None)
            if cambiadas:
                log.warning("config.json: recargado en caliente %s; apagados=%s", cambiadas,
                            self._modulos_apagados() or "ninguno")
        except (OSError, ValueError) as e:
            log.warning("recarga de config.json fallida (se mantiene la anterior): %s", e)

    async def recibir(self, sesion, secuencia, msg):
        self._recargar_config()
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
        apagado = self._interruptor(msgid, msg)
        if apagado:
            log.warning("interruptor: módulo «%s» apagado, rechazo %s de %s", apagado, msgid,
                        sesion.direccion)
            sesion.enviar({"msgid": msgid, "rqid": rqid, "result": self._error_interruptor()})
            return
        manejador = getattr(self, "c_" + msgid.lower(), None)
        if manejador is not None:
            gp_antes = self._gp_actual(sesion, msgid)
            try:
                extra = await manejador(sesion, msg)
            except Exception:
                # un fallo nuestro no debe cerrar la conexión: el cliente lo vive como desconexión
                log.exception("fallo en %s", msgid)
                extra = {"result": "ERR_SERVERNOTREADY"}
            if gp_antes is not None:
                self._anotar_movimiento_gp(sesion, msgid, gp_antes)
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

    # ---------- Interruptores de emergencia ----------
    def _modulos_apagados(self):
        return [str(x).lower() for x in ((self.cfg.get("interruptores") or {}).get("apagados") or [])]

    def _error_interruptor(self):
        return str((self.cfg.get("interruptores") or {}).get("error") or "ERR_SERVERNOTREADY")

    def _interruptor(self, msgid, msg):
        """Nombre del módulo apagado que corta este comando, o None."""
        apagados = self._modulos_apagados()
        if not apagados:
            return None
        for modulo in apagados:
            regla = MODULOS_INTERRUPTOR.get(modulo) or {}
            if msgid in regla:
                tipo = regla[msgid]
                if tipo is None or str(msg.get("matching_type") or "OC_QUICK") == tipo:
                    return modulo
        return None

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
                 ("RECV_MYCLUB_PRESENT", "/recv_myclub_present"),
                 # Ghidra 12-nov (FUN_141de2b50): ranking de clanes de Cooperativo
                 ("CLAN_RANKING", "/clan_ranking"), ("CLAN_RANKING_BYID", "/clan_ranking_byid"))
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

    def _puntua(self, pareja):
        """R (ServerOptions/LiveSettingCatalog de OpenEleven): contra el farmeo, como máximo N partidos que
        dan puntos contra el MISMO rival por día (config max_partidos_rival_dia; 0 o sin poner = sin
        límite). Se decide una vez por partido y vale para Divisiones, Matchday y Torneo en línea."""
        if "puntua" in pareja:
            return pareja["puntua"]
        tope = int(self.cfg.get("max_partidos_rival_dia") or 0)
        claves = frozenset((x.steam_hex or "").lower() for x in pareja.get("miembros") or ())
        if len(claves) == 1 and "" not in claves and len(pareja.get("miembros") or ()) == 2:
            # misma identidad en los dos lados (dos PES con el mismo SteamID y la misma IP): no hay rival
            # real, así que no cuenta para ranking, puntos ni rachas (anti-autofarmeo)
            log.warning("partido %s sin puntos: los dos jugadores son la misma identidad (%s); "
                        "pon identidad.por_ip para separarlos", pareja.get("room_id"), next(iter(claves)))
            pareja["puntua"] = False
            return False
        if tope <= 0 or len(claves) != 2 or "" in claves:
            pareja["puntua"] = True
            return True
        dia = time.strftime("%Y-%m-%d")
        cruces = self.__dict__.setdefault("_cruces_dia", {})
        for k in [k for k in cruces if k[0] != dia]:
            del cruces[k]
        n = cruces.get((dia, claves), 0)
        pareja["puntua"] = n < tope
        if pareja["puntua"]:
            cruces[(dia, claves)] = n + 1
        else:
            log.info("partido %s no da puntos: ya van %d hoy contra el mismo rival", pareja.get("room_id"), n)
        return pareja["puntua"]

    def _resultado_division(self, s, pareja, m):
        """CMD_SEND_GAME_RESULT de un partido de Divisiones (A.1/A.2). El PRIMER informe completo
        se aplica a los dos jugadores; el segundo solo se compara."""
        reglas = self._reglas_division()
        if pareja.get("tipo") not in reglas.tipos:
            log.info("resultado de %s: no es de Divisiones, no cambia rating", pareja.get("tipo"))
            return
        if not self._puntua(pareja):
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
        # R (results.md AzrakHQ): se guarda para contestar CMD_REQ_GAMEADDPOINT al instante
        pareja["veredicto"] = {"player_count": len(lista), "player_list": lista, "winner_side": ganador,
                               "gameend_reason": "NORMAL_END", "is_pk": "NO"}

    def _reenviar_aviso(self, s, m, vigilancia):
        """R (coop2.md + pesmatch._relay_notice de AzrakHQ, probado en vivo): CMD_SEND_*NOTICE
        {pid_list, data} se empuja como CMD_WATCH_*NOTICE {data} (rqid 0) a cada pid, una vez, sin
        volver al que lo manda; EXCEPTO SYNC_PROGRESS, que el Cooperativo espera también de sí mismo
        (TaskSyncProgress 0x141147fd0: grupo + yo, 30 s)."""
        pids = m.get("pid_list") or []
        if isinstance(pids, int):
            pids = [pids]
        datos = m.get("data", "")
        eco = vigilancia == "CMD_WATCH_NOTICE" and "SYNC_PROGRESS" in str(datos)
        mio = self._pid_visible(s)
        objetivos = []
        for p in pids:
            try:
                p = int(p)
            except (TypeError, ValueError):
                continue
            if p not in objetivos and (p not in (mio, mio & 0xFFFFFFFF) or eco):
                objetivos.append(p)
        for x in list(self.sesiones):
            pv = self._pid_visible(x)
            if pv in objetivos or (pv & 0xFFFFFFFF) in objetivos:
                x.enviar({"msgid": vigilancia, "rqid": 0, "result": NOERR, "data": datos})
        return {}

    async def c_cmd_send_notice(self, s, m):
        return self._reenviar_aviso(s, m, "CMD_WATCH_NOTICE")

    async def c_cmd_send_sync_notice(self, s, m):
        return self._reenviar_aviso(s, m, "CMD_WATCH_SYNC_NOTICE")

    async def c_cmd_send_system_notice(self, s, m):
        return self._reenviar_aviso(s, m, "CMD_WATCH_SYSTEM_NOTICE")

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
        # R (match.md/datacheck.md/menus.md AzrakHQ): los tiempos del partido son los valores por defecto
        # del propio PES (ctor 0x141e45ee0): carga 180 s, fuera de juego largo 120, inicio 240, falta de
        # comandos 180 (con 60 una carga lenta del 2.º PC daba LOAD_TIMEOUT). matchrejectpid_max 0 hacía
        # que «Añadir a la lista de bloqueo» dijera siempre «lista llena»; el PES usa 400.
        # is_revision_check: solo "YES" exacto compara sumas de datos entre los 2 PES (GNCX_0006 si
        # difieren); "NO" deja jugar con parches distintos. Se cambia en config (revision_check).
        return {"proto_opt": proto, "max_division": "D12",
                "teamlobby_game_rating_border_normal": 0, "teamlobby_game_rating_border_hard": 0,
                "is_revision_check": "YES" if self.cfg.get("revision_check") else "NO",
                "quality_check_wait_sec": 0, "trigger_keep_alive": 30,
                "matchrejectpid_max": 400, "match_client_timeout_load_timeout": 180,
                "match_client_timeout_long_outofplay": 120, "match_client_timeout_init_not_end": 240,
                "match_client_timeout_cmd_lack": 180, "is_check_myclub_level": 0,
                "log_update_level": 0, "lobby_ping_good": 50, "lobby_ping_normal": 150,
                "quick_ping_good": 50, "quick_ping_normal": 150, "konamiid_api_retry_sec": 5}

    # C (PESBUL R7.3.169): el menú online pide este lote al entrar; con NOERR vacío falla
    async def c_cmd_get_match_analyze_env(self, s, m):
        return {"posession": 0, "passtype": 0, "deffence": 0, "match_up": 0,
                "favorite_play": 0, "favorite_play_rate": 0}

    async def c_cmd_get_match_analyze(self, s, m):
        """L (ESPEC_PARTIDO_REAL_PESBUL §2) + R (pesresults.py AzrakHQ): el PES pide el análisis de juego
        (target_pid) y él mismo promedia los últimos 10. Lista de BIN (un mapa en línea cerró el PES al
        abrir «Cobertura de juego», en vivo 07-nov). Sin datos: lista vacía."""
        tipo = str(m.get("analyze_data_type") or "ONLINE_DIVISIONS")
        clave = (self._clave_de_id(m.get("target_pid")) if m.get("target_pid") else None) or (getattr(s, "steam_hex", None) or "")
        lista = (self.datos.historial(clave)["analisis"].get(tipo) or []) if clave else []
        return {"analyze_data_size": len(lista), "analyze_data_list": [Empaquetado(x) for x in lista],
                "game_res": "DRAW"}

    async def c_cmd_send_match_analyze(self, s, m):
        """Tras Divisiones (y myClub) el PES sube 46 números de su juego; se guardan los últimos 10."""
        datos = m.get("analyze_data")
        if isinstance(datos, (bytes, bytearray)):
            try:
                datos = msgpack_min.unpackb(bytes(datos))
            except Exception:
                datos = None
        if isinstance(datos, dict) and s.steam_hex:
            limpio = {str(k): v for k, v in datos.items() if isinstance(v, (int, str)) and not isinstance(v, bool)
                      or (isinstance(v, list) and all(isinstance(x, int) for x in v))}
            tipo = str(m.get("analyze_data_type") or "ONLINE_DIVISIONS")
            h = self.datos.historial(s.steam_hex)
            h.setdefault("analisis", {})[tipo] = (list(h["analisis"].get(tipo) or []) + [limpio])[-10:]
            self.datos.guardar_historial(s.steam_hex, h)
        return {}

    def _guardar_historial(self, pareja, m):
        """El primer resultado de una pareja se guarda en el historial de los dos (últimos 30)."""
        if pareja.get("historial_guardado"):
            return
        try:
            goles = (int(m.get("home_score") or 0), int(m.get("away_score") or 0))
        except (TypeError, ValueError):
            return
        pareja["historial_guardado"] = True
        tipo = str(pareja.get("tipo") or "").upper()
        categoria = ("RANK" if tipo in self._reglas_division().tipos else "MATCHDAY" if "MATCHDAY" in tipo
                     else "PESLEAGUE" if "PES_LEAGUE" in tipo else "COOP" if tipo == "COOP" else "LOBBY")
        casa, fuera = pareja["miembros"]
        equipos = [self._id_entero(m.get("home_team_id")) or 0, self._id_entero(m.get("away_team_id")) or 0]
        fichas = []
        for x, eq, g in zip((casa, fuera), equipos, goles):
            j = self.datos.jugador(x.steam_hex or "") or {}
            fichas.append({"pid": self._pid_en_partida(x), "name": j.get("pname") or x.pname or "",
                           "team": eq, "score": g, "clave": (x.steam_hex or "").lower()})
        p = {"date": int(time.time()), "home": fichas[0], "away": fichas[1], "categoria": categoria,
             "reason": "NORMAL_END", "partido": pareja.get("room_id")}
        if categoria == "PESLEAGUE":
            p["compe"] = self._cid_torneo(casa, {}, "pesleague") or 0
        for x in (casa, fuera):
            if x.steam_hex:
                h = self.datos.historial(x.steam_hex)
                h["partidos"] = (list(h.get("partidos") or []) + [p])[-30:]
                self.datos.guardar_historial(x.steam_hex, h)

    c_cmd_get_match_analyze_byid = c_cmd_get_match_analyze

    def _guardar_playdata(self, formulario):
        """R (results.md AzrakHQ, en vivo): ~2 s antes del resultado cada PES manda por HTTP el resumen del
        partido en JSON (goleadores, nota y minutos por jugador, estadio...). La mejor fuente de notas y
        goleadores: se guarda tal cual (los últimos 10) con un resumen leído."""
        clave = self._clave_de_id(formulario.get("pid"))
        crudo = formulario.get("play_data")
        if not clave or not isinstance(crudo, str) or len(crudo) > 200_000:
            return
        try:
            j = json.loads(crudo)
        except ValueError:
            log.warning("play_data ilegible de %s", clave[-6:])
            return
        if not isinstance(j, dict):
            return
        notas = {}
        for eq in j.get("team") or []:
            for mi in (eq or {}).get("member") or [] if isinstance(eq, dict) else []:
                if isinstance(mi, dict) and mi.get("player_id"):
                    notas[str(mi["player_id"])] = {"nota": mi.get("rating"), "minutos": mi.get("play_time")}
        resumen = {"fecha": int(time.time()), "goleadores": [g for g in (j.get("scorer") or []) if g],
                   "equipos": [(e or {}).get("team_id") for e in (j.get("team") or []) if isinstance(e, dict)],
                   "estadio": j.get("stadium_id"), "notas": notas}
        h = self.datos.historial(clave)
        h["playdata"] = (list(h.get("playdata") or []) + [{"resumen": resumen, "crudo": j}])[-10:]
        self.datos.guardar_historial(clave, h)
        log.info("resumen del partido de %s: goleadores %s", clave[-6:], resumen["goleadores"])

    # ---------- Cortesía (R: pesmanner.py de AzrakHQ, avisos vistos en vivo el 07-nov) ----------
    RECIENTES_MODALES = 10
    MOTIVOS_PROPIOS = {"GIVEUP", "EXIT", "NO_OPERATION"}

    @staticmethod
    def _grado_modales(abandonos):
        return "A" if abandonos <= 1 else "B" if abandonos <= 3 else "C"

    def _anotar_modales(self, clave, abandono, motivo=""):
        if not clave:
            return
        d = self.datos.modales(clave)
        d["hist"] = (list(d.get("hist") or []) + [1 if abandono else 0])[-self.RECIENTES_MODALES:]
        self.datos.guardar_modales(clave, d)
        if abandono:
            log.info("cortesía: %s dejó un partido antes de tiempo (%s): %s", clave[-6:], motivo,
                     self._grado_modales(sum(d["hist"])))

    def grado_modales(self, clave):
        return self._grado_modales(sum((self.datos.modales(clave) if clave else {}).get("hist") or []))

    async def c_cmd_get_manner_warn(self, s, m):
        """Un aviso (una vez) tras el abandono que lo causó: sigue A -> DOWN_B, baja a B -> A_TO_B,
        en B -> DOWN_C, baja a C u otro abandono en C -> STILL_C. Sin aviso: sin warn_kind."""
        clave = (s.steam_hex or "").lower()
        if not clave:
            return {}
        d = self.datos.modales(clave)
        q = sum(d.get("hist") or [])
        g, antes, avisados = self._grado_modales(q), d.get("grado") or "A", int(d.get("avisados") or 0)
        tipo = None
        if "ABC".index(g) > "ABC".index(antes):
            tipo = "A_TO_B" if g == "B" else "STILL_C"
        elif g == antes and q > avisados:
            tipo = {"A": "DOWN_B", "B": "DOWN_C", "C": "STILL_C"}[g]
        d["grado"], d["avisados"] = g, q
        self.datos.guardar_modales(clave, d)
        return {"warn_kind": tipo} if tipo else {}

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

    def _avisar_identidad_repetida(self, s):
        """Dos PC distintas (otra IP) con la MISMA clave de jugador se verían como la misma persona
        (mismo club, mismo pid). Un PES abre varias conexiones desde su IP: eso es normal y no avisa.
        Solo avisa; la separación real se configura con identidad.por_ip (ver clave_identidad)."""
        ip = (getattr(s, "ip", None) or "").split("%")[0].removeprefix("::ffff:")
        clave = (s.steam_hex or "").lower()
        if not clave or not ip:
            return False
        for x in self.sesiones:
            if x is s or (x.steam_hex or "").lower() != clave:
                continue
            otra = (getattr(x, "ip", None) or "").split("%")[0].removeprefix("::ffff:")
            if otra and otra != ip:
                log.warning("identidad repetida: %s ya está en el servidor desde %s y ahora entra desde %s; "
                            "se verán como la misma persona. Pon identidad.por_ip {IP: clave} para separarlos",
                            clave, otra, ip)
                return True
        return False

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
        self._avisar_identidad_repetida(s)
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
        # R (pescmds.py/lobby2.md AzrakHQ, probado en vivo por ellos): mismos campos SIN key/key_size
        # (nuestra clave de sesión no cambia). chat_probi "NO" = chat permitido (0x1410f9d49).
        return {"nation_code_server": str(self.cfg.get("pais_servidor") or "US"), "chat_probi": "NO",
                "is_select_code_changed": "NO", "subscription_active": "NO", "subscription_expire_date": 0}

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
        # R (announce.py AzrakHQ): solo cuando hay reinicio anunciado van maintenance_warn YES y la hora
        if self.en_mantenimiento():
            return {"maintenance_warn": "YES", "maintenance_time": int(time.time()) + 300}
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

    # ---------- Direcciones de un PES tal como debe verlas su rival (R: pesmatch._for_viewer AzrakHQ) ----------
    @staticmethod
    def _es_remota(ip):
        """¿Llega desde fuera de la casa del servidor (internet o Tailscale 100.64/10)?"""
        try:
            d = ipaddress.ip_address(str(ip or "").split("%")[0])
        except ValueError:
            return False
        d = getattr(d, "ipv4_mapped", None) or d
        return d.is_global or d in ipaddress.ip_network("100.64.0.0/10")

    def _direcciones_para(self, fila, x, viewer):
        """El PES que juega en la PC del servidor (o en su red) saca su dirección «pública» de nuestro
        STUN por la red local: declara 127.0.0.1 o 192.168.x. Un amigo de otra casa no llega ahí. Para ese
        amigo se pone en host_addr la IP por la que él llega al servidor (misma salida a internet),
        con el mismo puerto. reflexive_addr NO se toca: el PES la usa como etiqueta para decidir quién
        abre el enlace y, cambiada, los dos se quedan esperando (MXNV_0013). Si ni así conecta, va el TURN.
        config.direccion_para_remoto = false lo apaga."""
        if viewer is None or viewer is x or not self.cfg.get("direccion_para_remoto", True):
            return fila
        if not self._es_remota(getattr(viewer, "ip", None)):
            return fila
        v = fila.get("reflexive_addr")
        try:
            obj = v
            if isinstance(obj, (bytes, bytearray)):
                obj = msgpack_min.unpackb(bytes(obj))
            if isinstance(obj, Empaquetado):
                obj = obj.valor
            if not isinstance(obj, dict) or obj.get("address_family") not in (1, None):
                return fila
            sa = obj.get("sa_data")
            como_texto = isinstance(sa, str)
            crudo = base64.b64decode(sa + "=" * (-len(sa) % 4)) if como_texto else bytes(sa or b"")
            if len(crudo) < 8 or ipaddress.IPv4Address(crudo[4:8]).is_global:
                return fila
            nueva = ipaddress.IPv4Address(self.ip_anuncio(viewer))
        except (ValueError, TypeError, KeyError):
            return fila
        if not (nueva.is_global or nueva in ipaddress.ip_network("100.64.0.0/10")):
            return fila
        crudo = crudo[:4] + nueva.packed + crudo[8:]
        nuevo = {**obj, "sa_data": base64.b64encode(crudo).decode() if como_texto else crudo}
        fila = dict(fila)
        if isinstance(v, (bytes, bytearray)):
            fila["host_addr"] = msgpack_min.packb(nuevo)
        elif isinstance(v, Empaquetado):
            fila["host_addr"] = Empaquetado(nuevo)
        else:
            fila["host_addr"] = nuevo
        log.info("dirección de %s para un rival remoto: %s (host_addr)", getattr(x, "pid", "?"), nueva)
        return fila

    def _miembro(self, s, orden, dueno, lado=None, para=None):
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
        return Empaquetado(self._direcciones_para(f, s, para))

    @staticmethod
    def _vigilar_emparejamiento(s, estado, miembros):
        # C (r7.3.641/644): el estado llega por un EMPUJE con rqid 0 y result NOERR obligatorio;
        # START_MATCHING solo lleva msgid/rqid/result
        # R (pesmatch.py AzrakHQ, probado en vivo): ping y los 3 indicadores de partida especial
        s.enviar({"msgid": "CMD_WATCH_MATCHING", "rqid": 0, "result": NOERR,
                  "matching_state": estado, "player_num": len(miembros), "ping": 30,
                  "is_special_match": "NO", "is_first_rookie_match": "NO", "is_same_group_match": "NO",
                  "member": miembros})

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

    def _datos_juego(self, x):
        """R (pesmatch._game_data AzrakHQ, probado en vivo): lo que debe ser igual para jugar: las 2 sumas de
        CHECK_CHEAT (los PES comparan param_check_sum y luego entry_check_sum antes del saque; distinto =
        «error DATAPACK» MXNV533) y las versiones. None si el PES no las mandó (se empareja con cualquiera)."""
        sumas = getattr(x, "sumas", None) or self.__dict__.get("_sumas_por_clave", {}).get(
            (getattr(x, "steam_hex", None) or "").lower())
        if not sumas or not sumas.get("param"):
            return None
        v = getattr(x, "versiones", None) or {}
        return sumas["param"], sumas.get("entry", ""), v.get("patch_version", ""), v.get("dlc_version", "")

    def _misma_huella(self, a, b):
        if self.cfg.get("emparejar_mismos_datos", True):
            da, db = self._datos_juego(a), self._datos_juego(b)
            if da is not None and db is not None and da != db:
                log.info("no se empareja %s con %s: datos del juego distintos (sumas o versión)", a.pid, b.pid)
                return False
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
        if "PES_LEAGUE" in tipo.upper():
            # R (pesmatch._queue_key AzrakHQ): en Torneo en línea solo se empareja dentro del MISMO torneo
            tipo = f"{tipo}:{self._cid_torneo(s, m, 'pesleague') or 0}"
        cola = self.colas.setdefault(tipo, [])
        # Parche: solo se empareja a quien tiene LOS MISMOS archivos (file_hash_list de NTL);
        # con parches distintos el partido se desincroniza o el juego se cierra
        rival = next((x for x in cola if x is not s and x in self.sesiones
                      and self._misma_huella(s, x) and not self._no_emparejar(s, x)), None)
        if rival is None:
            cola.append(s)
            await self.dormir(PAUSA_EMPUJE)
            if s in cola:  # tarjeta propia en el panel LOCAL mientras busca (r7.3.647)
                self._vigilar_emparejamiento(s, "SEARCHING", [self._miembro(s, 0, False)])
            return None
        cola.remove(rival)
        # el que esperaba es anfitrión/LOCAL; el que completa la pareja, VISITANTE (r7.3.644/650)
        pareja = {"miembros": (rival, s), "check": set(), "tipo": tipo}
        self.parejas[rival] = self.parejas[s] = pareja
        self._abrir_partida(pareja)
        await self.dormir(PAUSA_EMPUJE)
        for x in (rival, s):
            miembros = [self._miembro(rival, 0, True, "HOME", para=x), self._miembro(s, 1, False, "AWAY", para=x)]
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

    # ---------- Cooperativo: búsqueda rápida 1 humano por bando + CPU (R: pescoop.py / coop.md AzrakHQ) ----------
    SIN_EQUIPO_COOP = 262143  # use_team_id cuando el jugador no trae equipo propio

    async def c_cmd_get_coop_quickmatchoption(self, s, m):
        # force_alone: sin búsqueda en grupo (2 humanos en un bando necesitan 2 pares P2P)
        return {"is_vscom": "NO", "force_alone": "YES", "cpu_level_min": "NORMAL", "cpu_level_max": "NORMAL"}

    def _grupo_coop(self, x, orden, dueno, gid, equipo, lado, para=None):
        """Un grupo de un humano. Anidado como lo lee 0x141dc9a70: group_list[] -> bin{member_list[] ->
        bin{member: ficha 1v1, ...}}; un solo is_owner YES, en el dueño LOCAL."""
        fila = Empaquetado({"member": self._miembro(x, orden, dueno, lado, para=para), "guest_kind": "GUEST_NONE",
                            "is_follow": "NO", "coop_level": self.nivel_coop((x.steam_hex or "").lower())[0],
                            "coop_manner": self.grado_modales((x.steam_hex or "").lower())})
        return Empaquetado({"member_list": [fila], "member_list_num": 1, "use_team_id": equipo,
                            "owner_pid": self._pid_en_partida(x), "group_id": gid})

    async def c_cmd_start_coop_matching(self, s, m):
        """R: misma cola y pareja que Partida rápida (tipo COOP). Sin equipo propio en ambos bandos el
        PES carga un equipo nulo y se cierra: se ponen dos equipos reales distintos."""
        self._salir_de_cola(s)
        self._dejar_pareja(s, "busca Cooperativo")
        s.equipo_coop = self._id_entero(m.get("use_team_id")) or self.SIN_EQUIPO_COOP
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        cola = self.colas.setdefault("COOP", [])
        rival = next((x for x in cola if x is not s and x in self.sesiones and self._misma_huella(s, x)
                      and not self._no_emparejar(s, x) and not self._no_emparejar(s, x, "rechazados")), None)
        if rival is None:
            cola.append(s)
            return None
        cola.remove(rival)
        pareja = {"miembros": (rival, s), "check": set(), "tipo": "COOP"}
        self.parejas[rival] = self.parejas[s] = pareja
        self._abrir_partida(pareja)
        base = [int(x) for x in ((self.cfg.get("myclub") or {}).get("coop_equipos") or [101, 108])][:2]
        ta = rival.equipo_coop if rival.equipo_coop != self.SIN_EQUIPO_COOP else base[0]
        tb = s.equipo_coop if s.equipo_coop != self.SIN_EQUIPO_COOP else base[-1]
        await self.dormir(PAUSA_EMPUJE)
        for x in (rival, s):
            grupos = [self._grupo_coop(rival, 0, True, 1, ta, "HOME", para=x),
                      self._grupo_coop(s, 1, False, 2, tb, "AWAY", para=x)]
            x.enviar({"msgid": "CMD_WATCH_COOP_MATCHING", "rqid": 0, "result": NOERR,
                      "matching_state": "FINISH", "group_list_num": 2, "group_list": grupos})
        log.info("pareja de COOP: %s (equipo %s) vs %s (equipo %s)", rival.pid, ta, s.pid, tb)
        return None

    async def c_cmd_cancel_coop_matching(self, s, m):
        # R: como Partida rápida, la búsqueda solo se cierra con el empuje CANCEL (coop.md: estado 4)
        estaba = s in self.colas.get("COOP", [])
        self._salir_de_cola(s)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        if estaba:
            s.enviar({"msgid": "CMD_WATCH_COOP_MATCHING", "rqid": 0, "result": NOERR,
                      "matching_state": "CANCEL", "group_list_num": 0, "group_list": []})
        return None

    async def c_cmd_set_coop_match_data(self, s, m):
        """R: tras cada partido Cooperativo el PES sube sus tablas de puntuación (texto base64, ~480 B) y
        las pide antes del siguiente (GET_COOP_MATCH_DATA) y en la ficha de Cooperativo."""
        datos, tam = m.get("match_data"), m.get("match_data_size")
        if s.steam_hex and isinstance(datos, str) and isinstance(tam, int) and 0 < tam <= 4096 and len(datos) <= 8192:
            d = self.datos.coop_datos(s.steam_hex)
            d.update({"size": tam, "data": datos})
            self.datos.guardar_coop_datos(s.steam_hex, d)
        return {}

    async def c_cmd_get_coop_match_data(self, s, m):
        d = self.datos.coop_datos(s.steam_hex) if s.steam_hex else {}
        return {"match_data_size": int(d.get("size") or 0), "match_data": str(d.get("data") or "")}

    # R (pescoop.py AzrakHQ): puntos por partido Cooperativo terminado, un nivel cada 100
    PUNTOS_NIVEL_COOP = 100
    NIVEL_MAX_COOP = 99
    PREMIO_COOP = {"WIN": 30, "DRAW": 20, "LOSE": 10}

    def nivel_coop(self, clave):
        d = self.datos.coop_datos(clave) if clave else {}
        pts = int(d.get("puntos") or 0)
        return min(self.NIVEL_MAX_COOP, 1 + pts // self.PUNTOS_NIVEL_COOP), pts, int(d.get("partidos") or 0)

    def _premiar_coop(self, pareja):
        """Al pitazo final (los 2 en END) de un Cooperativo: puntos a los 2, una vez por pareja. El antes
        y después se guardan para GET_COOP_LEVEL_RESULT, que llega cuando la pareja ya no existe."""
        if pareja.get("tipo") != "COOP" or pareja.get("coop_premiado") or len(pareja["miembros"]) != 2:
            return
        pareja["coop_premiado"] = True
        g = pareja.get("marcador") or {"HOME": 0, "AWAY": 0}
        filas = {}
        for x, lado in zip(pareja["miembros"], ("HOME", "AWAY")):
            clave = (x.steam_hex or "").lower()
            if not clave:
                continue
            mio, suyo = (g["HOME"], g["AWAY"]) if lado == "HOME" else (g["AWAY"], g["HOME"])
            res = "WIN" if mio > suyo else "LOSE" if mio < suyo else "DRAW"
            d = self.datos.coop_datos(clave)
            antes = int(d.get("puntos") or 0)
            d["puntos"] = antes + self.PREMIO_COOP[res]
            d["partidos"] = int(d.get("partidos") or 0) + 1
            d["victorias"] = int(d.get("victorias") or 0) + (res == "WIN")
            self.datos.guardar_coop_datos(clave, d)
            nivel = lambda p: min(self.NIVEL_MAX_COOP, 1 + p // self.PUNTOS_NIVEL_COOP)
            filas[clave] = {"punto": self.PREMIO_COOP[res], "antes": antes, "despues": d["puntos"],
                            "nivel_antes": nivel(antes), "nivel_despues": nivel(d["puntos"])}
        ultimo = self.__dict__.setdefault("_coop_ultimo", {})
        for clave in filas:
            ultimo[clave] = {"miembros": list(filas), "filas": filas}
        log.info("nivel COOP: %s", {k[-6:]: f"{v['antes']}->{v['despues']}" for k, v in filas.items()})

    async def c_cmd_get_coop_level_result(self, s, m):
        """R (pescoop.py AzrakHQ, parser 0x141ba8ec0): los 2 jugadores con puntos y nivel antes/después.
        Vacía = sin pantalla de nivel."""
        mia = (getattr(s, "steam_hex", None) or "").lower()
        ultimo = self.__dict__.get("_coop_ultimo", {}).get(mia)
        sigo = self.datos.relaciones(mia)["sigo"] if mia else []
        lista = []
        for clave in (ultimo or {}).get("miembros", []):
            d = ultimo["filas"][clave]
            lista.append(Empaquetado({
                "status": "LEVEL_UP" if d["nivel_despues"] > d["nivel_antes"] else "NONE",
                "user_id": self._usuario_plano(clave), "is_clan_owner": "NO",
                "is_follow": "YES" if clave != mia and clave in sigo else "NO",
                "evalution": d["punto"], "reference": self.PUNTOS_NIVEL_COOP - d["despues"] % self.PUNTOS_NIVEL_COOP,
                "point": d["punto"], "before_point": d["antes"], "after_point": d["despues"],
                "before_level": d["nivel_antes"], "after_level": d["nivel_despues"]}))
        return {"coop_level_user_list": lista}

    # ---------- Red, chat y bloqueo (R: pesmatch/pesrooms/pesblock/pescmds de AzrakHQ) ----------
    async def c_cmd_get_ipandport_ntl(self, s, m):
        """Direcciones NTL que declaró otro jugador (SEND_IPANDPORT_NTL), por su pid."""
        x = self._sesion_de_pid(m.get("pid")) or s
        decl = dict(self.ntl_por_clave.get(x.steam_hex) or {}) if x.steam_hex else {}
        decl.update(x.ntl or {})
        out = {"pid": self._pid_visible(x), "ntl_nat_type": int(decl.get("ntl_nat_type") or 0)}
        for k in ("host_addr", "reflexive_addr", "relayed_addr"):
            v = decl.get(k)
            if v:
                out[k] = Empaquetado(v) if isinstance(v, dict) else v
        return self._direcciones_para(out, x, s)

    CHAT_MAX = 120

    async def c_cmd_send_textchat(self, s, m):
        """{channel, statement, list_flg, list_pid} -> CMD_WATCH_TEXTCHAT {channel, from_pid, name,
        statement} a los de la sala o del partido (y al que escribe). Con list_flg CHAT_SEND solo a
        list_pid."""
        texto = str(m.get("statement") or "")[:self.CHAT_MAX]
        if not texto.strip():
            return {}
        canal = str(m.get("channel") or "ROOM")
        sala = self.salas.get(s.room_id)
        pareja = self._pareja_por_clave(s)
        if canal == "ROOM" and sala:
            para = list(sala.miembros)
        elif pareja:
            para = list(pareja["miembros"])
        elif sala:
            para = list(sala.miembros)
        else:
            para = [s]
        lista = m.get("list_pid")
        if str(m.get("list_flg") or "") == "CHAT_SEND" and isinstance(lista, list) and lista:
            elegidos = {str(p) for p in lista}
            para = [x for x in para if x is s or str(self._pid_visible(x)) in elegidos]
        if s not in para:
            para.append(s)
        cuerpo = {"channel": canal, "from_pid": self._pid_visible(s), "name": s.pname or "",
                  "statement": texto}
        for x in para:
            if not x.completar("CMD_WATCH_TEXTCHAT", **cuerpo):
                x.enviar({"msgid": "CMD_WATCH_TEXTCHAT", "rqid": 0, "result": NOERR, **cuerpo})
        log.info("chat %s de %s a %d: %r", canal, s.pid, len(para), texto)
        return {}

    def _no_emparejar(self, a, b, lista="no_emparejar"):
        """Lista de bloqueo de partidos (SET_MATCHREJECTPID) o, en Cooperativo, la de SET_COOP_REJECT
        (R: pescoop.blocked AzrakHQ): si uno bloqueó al otro no se emparejan."""
        ka, kb = (a.steam_hex or "").lower(), (b.steam_hex or "").lower()
        if not ka or not kb:
            return False
        return kb in self.datos.relaciones(ka)[lista] or ka in self.datos.relaciones(kb)[lista]

    async def c_cmd_set_matchrejectpid(self, s, m):
        poner = self._texto(m.get("is_reject", "YES")).upper() != "NO"
        ident = m.get("reject_pid")
        if poner:
            self._cambiar_relacion(s, "no_emparejar", [ident], [])
        else:
            self._cambiar_relacion(s, "no_emparejar", [], [ident])
        return {}

    async def c_cmd_send_tamper_data(self, s, m):
        # El propio PES avisa que los datos de jugadores cambiaron en memoria tras entrar (un trainer)
        log.warning("AVISO de datos alterados de %s: %s", s.pid,
                    {k: v for k, v in m.items() if k not in ("msgid", "rqid")})
        return {}

    async def c_cmd_reconnect(self, s, m):
        """R (match.md: OnlineSystemClientReconnect): tras caerse la conexión el PES abre otra y manda
        {pid, platform, patch_version, dlc_version, online_mode} SIN volver a entrar. La sesión nueva
        toma el puesto del pid. Solo si viene de la misma IP que la sesión anterior (nadie se hace pasar
        por otro con un pid ajeno)."""
        clave = self._clave_de_id(m.get("pid"))
        viejo = self._sesion_de_pid(m.get("pid"))
        ip_antes = (viejo.ip if viejo else None) or (self.ntl_por_clave.get(clave) or {}).get("ip_tcp")
        if not clave or not ip_antes or ip_antes != s.ip:
            log.warning("RECONNECT rechazado: pid %r desde %s", m.get("pid"), s.ip)
            return {"result": "ERR_LOGIN_FAILED"}
        j = self.datos.jugador(clave)
        if not j:
            return {"result": "ERR_LOGIN_FAILED"}
        s.steam_hex = clave
        if viejo is not None:
            s.ntl = dict(viejo.ntl or {})
            s.huella = getattr(viejo, "huella", None)
            s.versiones = getattr(viejo, "versiones", {})
        # vuelve a su partido aunque la sesión vieja ya se haya cerrado (se busca por la clave, y la
        # pareja sigue viva en la entrada del rival); si se cayó con el partido en juego y volvió antes
        # de los 15 s, el corte no cuenta como abandono
        pareja = self.parejas.pop(viejo, None) if viejo is not None else None
        pareja = pareja or next((p for p in self.parejas.values()
                                 if any((x.steam_hex or "").lower() == clave for x in p["miembros"])), None)
        if pareja is not None:
            pareja["miembros"] = tuple(s if (x.steam_hex or "").lower() == clave else x for x in pareja["miembros"])
            self.parejas[s] = pareja
            aband = pareja.get("abandonaron") or set()
            volvio = {x for x in aband if (x.steam_hex or "").lower() == clave}
            if volvio:
                aband -= volvio
                pareja["abandonaron"] = aband
                d = self.datos.modales(clave)
                if (d.get("hist") or [])[-1:] == [1]:
                    d["hist"][-1] = 0
                    self.datos.guardar_modales(clave, d)
                log.info("RECONNECT de %s a tiempo: el corte no cuenta como abandono", clave[-6:])
        self._registrar(s, j)
        log.info("RECONNECT de %s (pid %s)", clave[-6:], s.pid)
        return {}

    # ---------- Matchday tras el partido (R: pesmatchday.py de AzrakHQ) ----------
    PUNTOS_MATCHDAY = {"WIN": 3, "DRAW": 1, "LOSE": 0}

    def _resultado_matchday(self, s, pareja, m):
        """Cada PES informa su propio resultado: se suman SUS puntos (victoria 3, empate 1) una sola
        vez por partido. Quedan pendientes para la pantalla de puntos (ACHIEVEMENT_REWARD)."""
        if "MATCHDAY" not in str(pareja.get("tipo") or "").upper() or not s.steam_hex or not self._puntua(pareja):
            return
        try:
            goles = (int(m.get("home_score") or 0), int(m.get("away_score") or 0))
        except (TypeError, ValueError):
            return
        casa = pareja["miembros"][0] is s or (pareja["miembros"][0].steam_hex == s.steam_hex)
        mios, suyos = goles if casa else goles[::-1]
        res = "WIN" if mios > suyos else "LOSE" if mios < suyos else "DRAW"
        club = self._club_propio(s)
        if club is None:
            return
        partido = f"md{pareja.get('room_id')}"
        vistos = club.setdefault("matchday_partidos_vistos", [])
        if partido in vistos:
            return
        vistos.append(partido)
        del vistos[:-50]
        cid = str(self._cid_torneo(s, {}, "matchday") or 0)
        pts = self.PUNTOS_MATCHDAY[res]
        puntos, jugados = club.setdefault("matchday_puntos", {}), club.setdefault("matchday_jugados", {})
        puntos[cid] = int(puntos.get(cid) or 0) + pts
        jugados[cid] = int(jugados.get(cid) or 0) + 1
        club["matchday_pendiente"] = int(club.get("matchday_pendiente") or 0) + pts
        e = next((x for x in self._torneos("matchday") if str(x["compe_id"]) == cid), None)
        gp = self._premios_matchday(e or {}).get("WIN" if res == "WIN" else "LOSE", 0)
        antes = puntos[cid] - pts
        if gp:
            club["gp"] = int(club.get("gp") or 0) + gp
        # Metas de puntos: el PES las pinta SIEMPRE como «Monedas de myClub» (11-nov, igual que los
        # logros), así que se pagan con la misma regla que los logros (_premio_logro: GP o monedas)
        for meta, premio in self._hitos_matchday(e or {}):
            if antes < meta <= puntos[cid]:
                campo, cantidad = self._premio_logro(premio)
                club[campo] = int(club.get(campo) or 0) + cantidad
                if campo == "gp":
                    gp += cantidad
        self.datos.guardar_club(s.steam_hex, club)
        log.info("Matchday %s: %s +%d puntos +%d GP (torneo %s)", s.pid, res, pts, gp, cid)

    def _resultado_pesleague(self, s, pareja, m):
        """R (pestourney.record AzrakHQ): Torneo en línea, 3 por victoria y 1 por empate, con G/E/P y goles.
        Cada PES suma lo suyo, una vez por partido, solo dentro de las fechas del torneo."""
        if "PES_LEAGUE" not in str(pareja.get("tipo") or "").upper() or not s.steam_hex or not self._puntua(pareja):
            return
        try:
            goles = (int(m.get("home_score") or 0), int(m.get("away_score") or 0))
        except (TypeError, ValueError):
            return
        cid = self._cid_torneo(s, {}, "pesleague") or 0
        e = next((x for x in self._torneos("pesleague") if x["compe_id"] == cid), None)
        if e is None or not e["inicio"] <= time.time() < e["fin"]:
            return
        casa = pareja["miembros"][0] is s or (pareja["miembros"][0].steam_hex == s.steam_hex)
        mios, suyos = goles if casa else goles[::-1]
        res = "WIN" if mios > suyos else "LOSE" if mios < suyos else "DRAW"
        club = self._club_propio(s)
        if club is None:
            return
        partido = f"pl{pareja.get('room_id')}"
        vistos = club.setdefault("pesleague_partidos_vistos", [])
        if partido in vistos:
            return
        vistos.append(partido)
        del vistos[:-50]
        k = str(cid)
        st = club.setdefault("pesleague_stats", {}).setdefault(k, {"WIN": 0, "DRAW": 0, "LOSE": 0, "gf": 0, "gc": 0})
        st[res] += 1
        st["gf"] += mios
        st["gc"] += suyos
        puntos = club.setdefault("pesleague_puntos", {})
        puntos[k] = int(puntos.get(k) or 0) + self.PUNTOS_MATCHDAY[res]
        self.datos.guardar_club(s.steam_hex, club)
        log.info("Torneo %s: %s %s %d-%d (+%d)", cid, s.pid, res, mios, suyos, self.PUNTOS_MATCHDAY[res])

    def _stats_pesleague(self, club, cid):
        st = ((club or {}).get("pesleague_stats") or {}).get(str(cid)) or {}
        g, e, p = int(st.get("WIN") or 0), int(st.get("DRAW") or 0), int(st.get("LOSE") or 0)
        return {"win_count": g, "draw_count": e, "lose_count": p, "match_num": g + e + p,
                "get_score": int(st.get("gf") or 0), "lose_score": int(st.get("gc") or 0),
                "diff_score": int(st.get("gf") or 0) - int(st.get("gc") or 0),
                "point": int(((club or {}).get("pesleague_puntos") or {}).get(str(cid)) or 0)}

    async def c_cmd_get_matchday_achievement_reward(self, s, m):
        """Pantalla de puntos tras el partido: «Revisión del juego» = puntos/10 en décimas, factor 1.0."""
        club = self._club_propio(s) if s.steam_hex else None
        pts = int((club or {}).get("matchday_pendiente") or 0)
        if club is not None and pts:
            club["matchday_pendiente"] = 0
            self.datos.guardar_club(s.steam_hex, club)
        return {"add_matchday_point": pts, "gameplay_quality": pts * 10, "squad_rating": 10,
                "achievement_reward_list": []}

    async def c_cmd_get_matchday_watch_match_reward(self, s, m):
        return {"is_exist_watch_match_reward": "NO", "watch_match_reward_list": []}

    def _filas_matchday(self, cid, grupo=None):
        """Inscritos de un torneo Matchday (de un bando, o de todos con grupo None), ordenados:
        [(puntos, jugados, clave, team_id, grupo)]."""
        filas = []
        for clave in self.datos.claves_de_clubes():
            club = self.datos.club(clave) or {}
            suyo = (club.get("matchday_equipo") or {}).get(str(cid))
            if not suyo or (grupo is not None and (suyo.get("grupo") or "") != grupo):
                continue
            filas.append((int((club.get("matchday_puntos") or {}).get(str(cid)) or 0),
                          int((club.get("matchday_jugados") or {}).get(str(cid)) or 0), clave,
                          int(suyo.get("team_id") or 0), str(suyo.get("grupo") or "")))
        filas.sort(key=lambda f: (-f[0], f[1], f[2]))
        return filas

    def _premios_matchday(self, e):
        """GP por partido de Matchday: torneo "premios" = {"WIN": gp, "LOSE": gp} (el empate cobra
        LOSE). "premios": "NO" lo apaga. Se muestran en INFO (all_reward_info) y se pagan al final."""
        p = e.get("premios")
        if p == "NO":
            return {}
        p = p if isinstance(p, dict) else {}
        return {k: int(p.get(k, d)) for k, d in (("WIN", 1000), ("LOSE", 300)) if isinstance(p.get(k, d), int)}

    def _hitos_matchday(self, e):
        """Premios por puntos de evento («Recomp. pt. de evento»): torneo "premios_puntos" =
        [[puntos, gp], ...]; "NO" los quita. Se pagan una vez al cruzar cada meta."""
        p = e.get("premios_puntos", [[3, 500], [9, 1000], [15, 2000]])
        if p == "NO" or not isinstance(p, list):
            return []
        return [(int(a), int(b)) for a, b in (x for x in p if isinstance(x, (list, tuple)) and len(x) == 2)
                if isinstance(a, int) and isinstance(b, int) and a > 0 and b > 0]

    def _info_premios_matchday(self, e):
        """R (matchday2.md AzrakHQ Q3): all_reward_info es un ARRAY con un bin {achievement_reward_list,
        required_point_final_reward, win/lose/watching/pickup_*_reward_list [regalos], pickup_reward_type}.
        Nunca un bin suelto (cerraba el PES). myclub.matchday_premios_info = "NO" vuelve a []."""
        premios = self._premios_matchday(e)
        hitos = self._hitos_matchday(e)
        if (not premios and not hitos) or \
                str((self.cfg.get("myclub") or {}).get("matchday_premios_info", "SI")) == "NO":
            return []
        lista = lambda k, n: [self._premio_gp(n, premios[k], cantidad=premios[k])] if premios.get(k) else []
        # Tabla de «Info. de las recompensas» (prueba 11-nov: con [] salía solo el texto, sin tabla)
        logros = [Empaquetado({"is_repeat": "NO", "require_point": meta,
                               "reward_list": [self._premio_gp(10 + i, gp, tipo=self._tipo_premio_logro())]})
                  for i, (meta, gp) in enumerate(hitos)]
        return [Empaquetado({"achievement_reward_list": logros, "required_point_final_reward": 0,
                             "win_reward_list": lista("WIN", 1), "lose_reward_list": lista("LOSE", 2),
                             "watching_reward_list": [], "pickup_win_reward_list": [],
                             "pickup_lose_reward_list": [], "pickup_reward_type": "MATCHDAY_PICKUP_REWARD_NONE"})]

    async def c_cmd_get_matchday_ranking(self, s, m):
        """Ranking de MI bando: entradas bin {pname: bin{pid, pname: bin{...}}, entry_name, team_id, rank,
        matchday_point, recode_date} (forma comprobada por AzrakHQ en el parser del PES)."""
        cid = str(self._cid_torneo(s, m, "matchday") or 0)
        mio = ((self.datos.club(s.steam_hex) or {}).get("matchday_equipo") or {}).get(cid) if s.steam_hex else None
        grupo = (mio or {}).get("grupo") or ""
        filas = self._filas_matchday(cid, grupo)
        try:
            inicio = max(0, int(m.get("start_index") or 0))
            cuantos = max(1, min(100, int(m.get("view_num") or 100)))
        except (TypeError, ValueError):
            inicio, cuantos = 0, 100
        ahora = int(time.time())
        lista = []
        for rank, (pts, _n, clave, equipo, _g) in enumerate(filas, 1):
            if rank <= inicio or len(lista) >= cuantos:
                continue
            lista.append(Empaquetado({"pname": self._usuario_plano(clave), "entry_name": "", "team_id": equipo,
                                      "rank": rank, "matchday_point": pts, "recode_date": ahora}))
        return {"rank_list": lista, "list_num": len(lista), "list_max": len(filas), "update_date": ahora}

    async def c_cmd_get_pesleague_compe_overview_info(self, s, m):
        """Texto propio del torneo (reglas, premios): myclub.pesleague[].resumen en config."""
        cid = self._id_entero(m.get("compe_id"))
        e = next((x for x in self._torneos("pesleague") if x["compe_id"] == cid), None)
        return {"overview_info": str((e or {}).get("resumen") or "")}

    # ---------- Ver partidos (R: pesreplay.py). Sin grabaciones propias aún: lista vacía válida ----------
    async def c_cmd_get_watching_match_list(self, s, m):
        return {"match_list": [], "match_list_num": 0, "total_match_num": 0}

    async def c_cmd_send_watching_result(self, s, m):
        return {}

    # ---------- Aviso urgente al entrar (R: announce.py): config aviso_urgente {id, titulo, texto} ----------
    async def c_cmd_get_urgent_announcement(self, s, m):
        aviso = self.cfg.get("aviso_urgente")
        if not isinstance(aviso, dict) or not aviso.get("texto"):
            return {}
        ident = int(aviso.get("id") or 1)
        vistos = self.__dict__.setdefault("_avisos_vistos", set())
        quien = (s.steam_hex or str(s.pid), ident)
        if quien in vistos:
            return {}
        vistos.add(quien)
        datos = {"announcement_id": ident, "online_mode": m.get("online_mode", "QUICK"),
                 "announcement_type": m.get("announcement_type", "PRE_MAIN"), "read_check": "NO",
                 "title": str(aviso.get("titulo") or "Phoenix Online"), "body": str(aviso["texto"])}
        return {"is_announcement": "YES", "announcement_data": Empaquetado(datos)}

    # ---------- Durante el partido (R: pesmatch.py/events.py + results.md de AzrakHQ) ----------
    async def c_cmd_check_cheat(self, s, m):
        # Solo se lee result; cualquier error bloquea la entrada online (CCWR450/455). Se guardan las sumas
        s.sumas = {"param": str(m.get("param_check_sum", "")), "entry": str(m.get("entry_check_sum", ""))}
        if s.steam_hex:  # puede llegar por otra conexión que la de la búsqueda
            self.__dict__.setdefault("_sumas_por_clave", {})[s.steam_hex.lower()] = s.sumas
        return {}

    async def c_cmd_set_gameteam(self, s, m):
        pareja = self._pareja_por_clave(s)
        if pareja is not None:
            pareja["equipos"] = (self._id_entero(m.get("home_team_id")) or 0,
                                 self._id_entero(m.get("away_team_id")) or 0)
        return {}

    async def c_cmd_change_gamephase(self, s, m):
        """R (pesmatch.py/lobby2.md AzrakHQ): cuando los 2 PES dicen END se empuja el veredicto a los
        dos. Sin él, al salir el primero de la pantalla de resultado el otro ve «se perdió la conexión»,
        y los contadores «N partidos / Victorias» del vestíbulo no suben. En Divisiones el veredicto
        con rating sale con SEND_GAME_RESULT (llega después), así que aquí no se manda."""
        pareja = self._pareja_por_clave(s)
        fase = str(m.get("phase") or m.get("game_phase") or "")
        if pareja is None or not fase:
            return {}
        fases = pareja.setdefault("fases", {})
        fases[self._pid_visible(s)] = fase
        if fase.startswith("1ST"):
            pareja["en_juego"] = True
        if len(fases) >= 2 and all(f == "END" for f in fases.values()) and not pareja.get("modales_hecho"):
            # partido completo: cuenta como «terminado bien» para quien no abandonó
            pareja["modales_hecho"] = True
            pareja["terminado"] = True
            for x in pareja["miembros"]:
                if x not in pareja.get("abandonaron", set()):
                    self._anotar_modales((x.steam_hex or "").lower(), False)
            self._premiar_coop(pareja)
        if pareja.get("tipo") in self._reglas_division().tipos or pareja.get("veredicto_al_final"):
            return {}
        if len(fases) >= 2 and all(f == "END" for f in fases.values()) \
                and self.cfg.get("veredicto_al_final", True):
            v = self._veredicto_sin_rating(pareja)
            if v is not None:
                pareja["veredicto"] = pareja["veredicto_al_final"] = v
                for x in pareja["miembros"]:
                    if x in self.sesiones:
                        x.enviar({"msgid": "CMD_WATCH_GAMEADDPOINT", "rqid": 0, "result": NOERR, **v})
        return {}

    async def c_cmd_add_score(self, s, m):
        """Los dos PES mandan cada gol {goal_side, gtimer}; se cuenta una vez por (gtimer, lado)."""
        pareja = self._pareja_por_clave(s)
        lado = str(m.get("goal_side") or "").upper()
        if pareja is not None and lado in ("HOME", "AWAY"):
            vistos = pareja.setdefault("goles_vistos", set())
            clave = (str(m.get("gtimer")), lado)
            if clave not in vistos:
                vistos.add(clave)
                goles = pareja.setdefault("marcador", {"HOME": 0, "AWAY": 0})
                goles[lado] += 1
        return {}

    def _veredicto_sin_rating(self, pareja):
        """Partido que no es de Divisiones: pantalla de resultado con el rating sin cambios."""
        fases = (pareja.get("fases") or {}).values()
        if "END" not in fases:
            return None
        g = pareja.get("marcador") or {"HOME": 0, "AWAY": 0}
        ganador = ("WINNER_HOME" if g["HOME"] > g["AWAY"] else "WINNER_AWAY" if g["AWAY"] > g["HOME"]
                   else "WINNER_NONE")
        equipos = pareja.get("equipos") or (0, 0)
        lista = []
        for x, lado, eq in zip(pareja["miembros"], ("HOME", "AWAY"), equipos):
            nombre = x.pname or ""
            lista.append(Empaquetado({
                "pid": self._pid_en_partida(x),
                "pes_pname": Empaquetado({"name": nombre, "xuid_one": "", "is_deleted": "NO",
                                          "psn_user_id": Empaquetado({"account_id": 0, "online_id": "",
                                                                      "is_valid_cache": "NO"})}),
                "rating_type": "RANKMATCH", "rating": RATING_INICIAL, "oldrating": RATING_INICIAL,
                "rating_boost_type": "NONE", "rating_boost_rate": 0, "rating_compe": 0, "team_id": int(eq or 0),
                "side": lado, "contWin_num": 0, "skill_rank": "SKILL_RANK_NONE",
                "is_rankup_skill_rank": "NO", "is_first_skillrank": "NO"}))
        return {"player_count": len(lista), "player_list": lista, "winner_side": ganador,
                "gameend_reason": "NORMAL_END", "is_pk": "NO"}

    async def c_cmd_req_gameaddpoint(self, s, m):
        """R (results.md): el PES que perdió la conexión espera el veredicto; a los 10 s lo pide aquí y a
        los 20 s se rinde (sin pantalla de rating). Se contesta NOERR y se empuja ya WATCH_GAMEADDPOINT."""
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        pareja = self._pareja_por_clave(s)
        if pareja is None:
            return None
        v = pareja.get("veredicto") or self._veredicto_sin_rating(pareja)
        if v is not None:
            pareja["veredicto"] = v
            s.enviar({"msgid": "CMD_WATCH_GAMEADDPOINT", "rqid": 0, "result": NOERR, **v})
        else:
            log.info("REQ_GAMEADDPOINT de %s: el partido no llegó a END, sin veredicto", s.pid)
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
        lobbies = [l for l in self.cfg.get("lobbies") or [] if l.get("lobby_type") == tipo]
        # «Vestíbulo de juego en equipo» (lobby_type MULTI, lobby2.md AzrakHQ): sin vestíbulo propio
        # el PES decía «Datos no encontrados». Si la config no trae uno MULTI se ofrece uno por
        # defecto (config "lobby_equipo": "NO" lo quita). Ojo: el partido de este modo va por el
        # relé SC de Konami y aún no conecta (necesita un arreglo en el Core).
        if not lobbies and tipo == "MULTI" and str(self.cfg.get("lobby_equipo", "SI")) != "NO":
            lobbies = [{"lobby_id": 2, "name": "Phoenix Juego en equipo", "player_num": 0,
                        "player_num_max": 200, "gamerelay_index": 0, "lobby_type": "MULTI"}]
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
        mala = self.palabra_bloqueada(texto)
        if mala:
            log.info("texto rechazado de %s (palabra %r)", s.pid, mala)
            return {"result": "ERR_NGWORD"}
        return {}

    @staticmethod
    def _normal(texto):
        import unicodedata
        t = unicodedata.normalize("NFKD", str(texto or "")).casefold()
        t = "".join(c for c in t if not unicodedata.combining(c))
        return re.sub(r"\s+", " ", t)

    def palabra_bloqueada(self, texto):
        """R (peswords AzrakHQ): palabras ENTERAS (antes «COMPUTADORA» o «disputa» se rechazaban por contener
        una grosería dentro); una frase con espacios vale en cualquier parte; letras sueltas «p.u.t.a» se
        juntan. Lista base + config palabras_bloqueadas (sin tildes ni mayúsculas que importen)."""
        lista = [self._normal(p) for p in (*PALABRAS_PROHIBIDAS, *(self.cfg.get("palabras_bloqueadas") or []))]
        t = self._normal(texto)
        juntas = re.sub(r"(?<![^\W_])(?:[^\W_][\W_]+){2,}[^\W_](?![^\W_])",
                        lambda x: re.sub(r"[\W_]+", "", x.group()), t)
        for cand in (t, juntas):
            for w in lista:
                if not w:
                    continue
                if " " in w:
                    if w in cand:
                        return w
                elif re.search(rf"(?<![^\W_]){re.escape(w)}(?![^\W_])", cand):
                    return w
        return None

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
        # R (pesprofile AzrakHQ, en vivo 07-nov): el PES pide el perfil con user_id bin{pid, pname};
        # el jugador puede estar desconectado (p. ej. desde la lista de bloqueo)
        destino = self._sesion_de_pid(m.get("pid")) or self._sesion_de_pid(self._pid_de_blob(m.get("user_id")))
        if destino is None:
            otro = self._clave_de_id(m.get("user_id")) or (self._clave_de_id(m.get("pid")) if m.get("pid") else None)
            if otro and otro != (getattr(s, "steam_hex", None) or "").lower():
                destino = SimpleNamespace(steam_hex=otro, pname="", pid=None, ip=None, steam_crudo=None)
        destino = destino or s
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
                "manner": self._ficha_modales(destino.steam_hex),
                "rankmatch_teamLog": self._equipos_usados(destino.steam_hex)[:5]}

    def _ficha_modales(self, clave):
        """R (pesprofile AzrakHQ, en vivo): bin{manner, disconnect_rate}; LOW_RATE / SEMI_HIGH_RATE /
        HIGH_RATE = Bajo / Moderadamente alto / Alto (tabla 0x1437f33f0), con los mismos abandonos que
        el grado de cortesía. Un texto suelto aquí cierra el PES."""
        clave = (clave or "").lower()
        q = sum((self.datos.modales(clave) if clave else {}).get("hist") or [])
        tasa = "LOW_RATE" if q <= 1 else "SEMI_HIGH_RATE" if q <= 3 else "HIGH_RATE"
        return Empaquetado({"manner": self._grado_modales(q), "disconnect_rate": tasa})

    @staticmethod
    def _pid_de_blob(v):
        if isinstance(v, (bytes, bytearray)):
            try:
                v = msgpack_min.unpackb(bytes(v))
            except Exception:  # noqa: BLE001
                return None
        if isinstance(v, Empaquetado):
            v = v.valor
        return v.get("pid") if isinstance(v, dict) else None

    def _equipos_usados(self, clave, solo_divisiones=True):
        """Equipos propios de los últimos partidos (el más reciente primero), del historial guardado."""
        clave = (clave or "").lower()
        if not clave:
            return []
        out = []
        for p in reversed(self.datos.historial(clave).get("partidos") or []):
            if solo_divisiones and p.get("categoria") != "RANK":
                continue
            for lado in ("home", "away"):
                f = p.get(lado) or {}
                if f.get("clave") == clave and int(f.get("team") or 0) > 0:
                    out.append(int(f["team"]))
        return out

    async def c_cmd_get_often_used_team_list(self, s, m):
        """R (pesdivisions AzrakHQ, parsebench 07-nov): «Equipos más elegidos» en Divisiones, hasta 6 por
        uso y luego el último; lista de enteros (L PESBUL: id simple, el cliente hace id << 14). Sin
        partidos, sin la lista."""
        usados = self._equipos_usados(getattr(s, "steam_hex", None))
        if not usados:
            return {"last_used_team_id": 0, "used_team_id_list_num": 0}
        cuenta = {}
        for i, eq in enumerate(usados):
            n, primero = cuenta.get(eq, (0, i))
            cuenta[eq] = (n + 1, primero)
        top = sorted(cuenta, key=lambda e: (-cuenta[e][0], cuenta[e][1]))[:6]
        return {"used_team_id_list": top, "used_team_id_list_num": len(top), "last_used_team_id": usados[0]}

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
        tipo = str(m.get("game_res_type") or "RANK").upper()
        if tipo in ("ALL", "PESLEAGUE") and s.steam_hex:
            # R (pesresults.py AzrakHQ): «ALL» = todos los partidos (Información online -> Registro),
            # «PESLEAGUE» = los del Torneo en línea {pesleague_compe_id}
            cid = self._id_entero(m.get("pesleague_compe_id"))
            for p in reversed(self.datos.historial(s.steam_hex)["partidos"]):
                if tipo == "PESLEAGUE" and (p.get("categoria") != "PESLEAGUE" or (cid and p.get("compe") != cid)):
                    continue
                filas.append(self._fila_resultado(p))
        elif tipo == "MYCLUB" and s.steam_hex:
            # myClub «Últimos 20 partidos» (Estadíst.): los partidos vs COM del club (club["vscom"]["historial"]);
            # el PES no manda el marcador en SET_MYCLUB_VSCOM_GAMERESULT, así que se muestra 1-0 / 0-0 / 0-1
            club = self.datos.club(s.steam_hex) or {}
            for h in list(reversed((club.get("vscom") or {}).get("historial") or []))[:20]:
                filas.append(self._fila_resultado(self._partido_vscom_fila(s, h)))
        else:
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

    def _partido_vscom_fila(self, s, h):
        """Fila de resultado para un partido vs COM de myClub (marcador aproximado por el desenlace)."""
        g = {"WIN": (1, 0), "DRAW": (0, 0), "LOSE": (0, 1)}.get(h.get("desenlace"), (0, 0))
        nombre = (self.datos.jugador(s.steam_hex or "") or {}).get("pname") or s.pname or ""
        return {"date": int(h.get("hora") or 0), "categoria": "VSCOM",
                "reason": str((h.get("datos") or {}).get("gameend_reason") or "NORMAL_END"),
                "home": {"pid": self._pid_visible(s), "name": nombre, "team": 0, "score": g[0]},
                "away": {"pid": 0, "name": "COM", "team": 0, "score": g[1], "com": True}}

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
                                "pid": f["pid"], "side": lado, "is_com": "YES" if f.get("com") else "NO", "cpu_level": "0",
                                "is_follow": "NO", "is_guest": "NO"})
        casa, fuera = p["home"], p["away"]
        # R (pesresults.CATEGORY AzrakHQ, en vivo 07-nov): nombre exacto del enum del PES; Partida
        # rápida, sala y desconocidos van como PESLEAGUE_LOBBY (sin etiqueta), Cooperativo ONLINE_COOP
        categoria = {"RANK": "RANK_MATCH", "MATCHDAY": "MATCHDAY", "COOP": "ONLINE_COOP",
                     "PESLEAGUE": "PESLEAGUE_LOBBY", "VSCOM": "VSCOM_MATCH"}.get(p.get("categoria") or "", "PESLEAGUE_LOBBY") \
            if p.get("categoria") else "ONLINE_DIVISIONS"
        return Empaquetado({"date": int(p["date"]), "match_category": categoria, "ex_type": "NONE",
                            "gameend_reason": p.get("reason") or "NORMAL_END", "pk_flag": "NO",
                            "score": [marcador(casa["score"]), marcador(fuera["score"])],
                            "player": [jugador(casa, "HOME"), jugador(fuera, "AWAY")],
                            "team": [int(casa["team"]), int(fuera["team"])]})

    async def c_cmd_get_ranking(self, s, m):
        return self._ranking_division(m)

    # R (pesranking AzrakHQ, en vivo 07-nov): sort RATING|WIN_NUM|MATCH_NUM|WIN_STRAIGHT|WIN_RATE; point = el
    # valor del orden; Tasa de victorias solo con 10+ partidos (nota del propio PES). _BYID: ventana alrededor del pid
    ORDEN_RANKING = {
        "RATING": lambda c: c["rating"], "WIN_NUM": lambda c: c["w"], "MATCH_NUM": lambda c: c["w"] + c["d"] + c["l"],
        "WIN_STRAIGHT": lambda c: c["racha_max"],
        "WIN_RATE": lambda c: round(100 * c["w"] / max(1, c["w"] + c["d"] + c["l"])),
    }

    def _ranking_division(self, m, por_id=False, todo=False):
        # L (PESBUL r728): elementos como MAPAS (no bin)
        division = m.get("division_type") or m.get("division")
        estados = self.datos.divisiones()
        pide = str(m.get("pid") or "")
        orden = str(m.get("sort") or "").upper()
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
                arch = e.get("archivadas") or []
                carrera = {"rating": e["rating"], "racha_max": e["racha_max"],
                           "w": e["win"] + sum(a.get("win", 0) for a in arch),
                           "d": e["draw"] + sum(a.get("draw", 0) for a in arch),
                           "l": e["loss"] + sum(a.get("loss", 0) for a in arch)}
                filas.append((f, e, carrera, es_quien_pide))
        if division not in (None, "") and orden in ("", "POINT"):
            # R: ranking de una división = puntos de esta temporada, luego victorias
            filas.sort(key=lambda x: (-x[1]["sp"], -x[1]["win"], int(x[0]["pid"] or 0)))
            valor = lambda x: x[1]["sp"]
        else:
            clave_orden = self.ORDEN_RANKING.get(orden, self.ORDEN_RANKING["RATING"])
            if orden == "WIN_RATE":
                filas = [x for x in filas if x[2]["w"] + x[2]["d"] + x[2]["l"] >= 10]
            # L (A.7): a igual valor, SP y luego pid
            filas.sort(key=lambda x: (-clave_orden(x[2]), -x[1]["sp"], int(x[0]["pid"] or 0)))
            valor = (lambda x: x[1]["sp"]) if orden == "" else (lambda x: clave_orden(x[2]))
        try:
            inicio = max(0, int(m.get("start_index") or 0))
            cuantos = min(100, max(0, int(m.get("view_num") or 50)))
        except (TypeError, ValueError):
            inicio, cuantos = 0, 50
        if por_id:
            en = next((i for i, x in enumerate(filas) if x[3]), 0)
            inicio = max(0, en - cuantos // 2)
        if todo:
            inicio, cuantos = 0, len(filas)
        lista = []
        for i, (f, e, c, _) in enumerate(filas[inicio:inicio + cuantos]):
            perfil = self.datos.perfil(f["steam_hex"])
            cuenta_pid = self.numero_de_clave(f["steam_hex"]) or f["pid"]
            lista.append({"rank": inicio + i + 1, "point": int(valor((f, e, c, None))), "user": {
                "pid": cuenta_pid,
                "pname": {"name": f["pname"], "xuid_one": "NotImplement",
                          "psn_user_id": {"account_id": 0, "online_id": f["pname"],
                                          "is_valid_cache": "NO"},
                          "is_deleted": "NO"},
                # R: aquí manner es TEXTO suelto (en GET_PLAYERINFO es bin)
                "division": e["division"], "manner": self.grado_modales((f["steam_hex"] or "").lower()),
                "favoriteTeam": perfil["favorite_team"],
                "favoritePlayer": perfil["favorite_player"], "intro": perfil["intro"],
                "match_record": {"win_count": c["w"], "draw_count": c["d"], "lose_count": c["l"],
                                 "win_s_max": e["racha_max"], "rating": e["rating"]}}})
        return {"date": int(time.time()), "total_rank_num": len(filas), "rank_list": lista}

    async def c_cmd_get_ranking_byid(self, s, m):
        return self._ranking_division(m, por_id=True)

    c_cmd_get_division_ranking = c_cmd_get_ranking
    c_cmd_get_division_ranking_byid = c_cmd_get_ranking_byid

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
                "present_num": len(self._regalos(club)) if club else 0,
                "scount_num": sum(int(c.get("count") or 1) for c in club.get("scouts") or []),
                "staff_num": len(club.get("staff") or []),
                "is_free_enable": "YES", "is_coop_enable": "NO", "is_auction_state_changed": "NO",
                # C (10-nov, PES): «Registros de myClub» muestra «Progreso num/total» de aquí
                "achievement_info": Empaquetado(self._resumen_logros(club, s)),
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
        # B.21b (P): no hay item_type propio para estos; item_variation = texto de «uso de
        # objeto» del cliente. Solo salen si el club tiene alguno (no tocan la lista de hoy).
        {"clave": "objetos_habilidad", "item_type": "STORE", "item_variation": "ADD_SKILL",
         "mostrar_vacio": False},
        {"clave": "objetos_posicion", "item_type": "STORE", "item_variation": "ADD_POSITION_APTITUDE",
         "mostrar_vacio": False},
        {"clave": "objetos_gestion", "item_type": "STORE", "item_variation": "IMPROVE_MANAGEMENT",
         "mostrar_vacio": False},
        {"clave": "objetos_adaptacion", "item_type": "STORE",
         "item_variation": "IMPROVE_TEAM_ADAPTABILITY_RATE", "mostrar_vacio": False},
        {"clave": "objetos_tactico", "item_type": "STORE", "item_variation": "TACTICS_TRAINING",
         "mostrar_vacio": False},
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
        """R (pesblock AzrakHQ): reject_list [bin{pid, pname}] + num, como la lista de bloqueo de Cooperativo.
        Antes iba siempre vacía aunque SET_MATCHREJECTPID sí guardaba."""
        clave = (getattr(s, "steam_hex", None) or "").lower()
        filas = [self._usuario_plano(x) for x in self.datos.relaciones(clave)["no_emparejar"]] if clave else []
        return self._lista_que_cabe("CMD_GET_MATCHREJECTLIST", filas, lambda f: {
            "reject_list": f, "reject_list_num": len(f)})

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
        # C.8: si hay evento inscrito, el partido cuenta para él (is_entry_compe) y la COM juega
        # con la dificultad elegida al inscribirse
        _, ins = self._inscripcion_activa(s)
        nivel = self._nivel_cpu(self._club_inicial(s)) if not r.get("cpu_level_fijo") \
            else r.get("cpu_level", "MATCH_LEVEL_REGULAR")
        if ins and str(ins.get("cpu_level") or "") not in ("", "COMPE_DIFFICULTY_NONE"):
            nivel = str(ins["cpu_level"])
        return {"side": r.get("side", "AWAY"), "cpu_level": nivel,
                "is_entry_compe": self._marca_evento() if ins else "NO",
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

    # ---------- B.21: intercambio de cartas y objetos de entrenamiento ----------
    # Los objetos se guardan como contadores en el club (se dan con admin o como premio).
    # Su item_variation en ITEMLIST aún no se conoce (N), por eso no salen en OBJETOS_DEFECTO.
    OBJETO_POR_USO = {"habilidad": "objetos_habilidad", "posicion": "objetos_posicion",
                      "gestion": "objetos_gestion", "adaptacion": "objetos_adaptacion",
                      "tactico": "objetos_tactico"}

    def _gastar_objeto(self, club, uso, cuantos=1):
        """Resta `cuantos` objetos del contador del club; None si alcanzan, o el error."""
        clave = self.OBJETO_POR_USO[uso]
        cuantos = max(1, int(cuantos or 1))
        if int(club.get(clave) or 0) < cuantos:
            return "ERR_FORBIDDEN"
        club[clave] = int(club.get(clave) or 0) - cuantos
        return None

    def _jugador_uid(self, club, uid):
        return next((x for x in club.get("jugadores") or []
                     if x.get("gameplayer_unique_id") == uid), None)

    async def c_cmd_trade_myclub_gameplayer(self, s, m):
        """B.21: das tus cartas repetidas (my_player_trade_list) y recibes target_trade_player.
        Config myclub.intercambio = {activo (def. sí), cartas (def. 2)}. No se dan cartas
        inscritas en una plantilla ni protegidas."""
        club = self._club_inicial(s)
        cfg = (self.cfg.get("myclub") or {}).get("intercambio") or {}
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        if cfg.get("activo") is False:
            return {"result": "ERR_FORBIDDEN"}
        objetivo = self._id_entero(m.get("target_trade_player"))
        cartas = list(dict.fromkeys(self._lista_uids(m, "my_player_trade_list")))
        en_uso = self._uids_en_uso(club)
        dadas = [self._jugador_uid(club, u) for u in cartas]
        if not objetivo or len(cartas) < int(cfg.get("cartas") or 2) or None in dadas \
                or any(u in en_uso for u in cartas) or any(j.get("is_protected") == "YES" for j in dadas):
            return {"result": "ERR_FORBIDDEN"}
        club["jugadores"] = [j for j in club["jugadores"] if j.get("gameplayer_unique_id") not in cartas]
        nuevo = (self._equipo_catalogo_jugador(objetivo) or {"gameplayer_id": objetivo})
        ficha = self._fichar(club, [nuevo])[0]
        self.datos.guardar_club(s.steam_hex or "", club)
        log.info("myClub %s: intercambia %d cartas por el jugador %d", s.steam_hex, len(cartas), objetivo)
        return {"gameplayer": ficha}

    def _equipo_catalogo_jugador(self, gameplayer_id):
        """Datos del catálogo (rating, puesto…) para un jugador, si hay catálogo."""
        try:
            cat = self._catalogo() or {}
        except Exception:
            return None
        datos = (cat.get("jugadores") or {}).get(str(gameplayer_id))
        if not isinstance(datos, dict):
            return None
        return {**datos, "gameplayer_id": gameplayer_id}

    async def c_cmd_use_myclub_add_skill_item(self, s, m):
        """B.21: objeto de habilidad. La habilidad sale de config myclub.habilidades_objeto
        (lista de ids; P) por turno; responde add_skill_id."""
        club = self._club_inicial(s)
        j = self._jugador_uid(club or {}, m.get("gameplayer_unique_id"))
        if club is None or j is None:
            return {"result": "ERR_NOTFOUND"}
        if j.get("add_skill_id"):
            return {"result": "ERR_FORBIDDEN"}
        error = self._gastar_objeto(club, "habilidad")
        if error:
            return {"result": error}
        ids = [x for x in (self.cfg.get("myclub") or {}).get("habilidades_objeto") or [1] if isinstance(x, int)] or [1]
        j["add_skill_id"] = ids[int(club.get("habilidades_dadas") or 0) % len(ids)]
        club["habilidades_dadas"] = int(club.get("habilidades_dadas") or 0) + 1
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"add_skill_id": j["add_skill_id"]}

    async def c_cmd_use_myclub_add_position_aptitude_item(self, s, m):
        """B.21: aptitud de posición. Guarda cuántos objetos recibió el jugador (P: el efecto
        lo aplica el cliente según su ficha)."""
        club = self._club_inicial(s)
        j = self._jugador_uid(club or {}, m.get("gameplayer_unique_id"))
        if club is None or j is None:
            return {"result": "ERR_NOTFOUND"}
        error = self._gastar_objeto(club, "posicion", m.get("use_count"))
        if error:
            return {"result": error}
        j["aptitud_extra"] = int(j.get("aptitud_extra") or 0) + max(1, int(m.get("use_count") or 1))
        self.datos.guardar_club(s.steam_hex or "", club)
        return {}

    def _usar_objeto_dt(self, s, m, uso, clave, paso, tope):
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        dts = club.get("entrenadores") or []
        i, d = next(((i, d) for i, d in enumerate(dts)
                     if d.get("coach_unique_id", i + 1) == m.get("coach_unique_id")), (None, None))
        if d is None:
            return {"result": "ERR_NOTFOUND"}
        veces = max(1, int(m.get("use_count") or 1))
        error = self._gastar_objeto(club, uso, veces)
        if error:
            return {"result": error}
        d[clave] = min(tope, int(d.get(clave) or (100 if clave == "adaptability_rate" else 70)) + paso * veces)
        self.datos.guardar_club(s.steam_hex or "", club)
        ficha = self._ficha_dt_myclub(i, d)
        return {"coach": ficha, **ficha.valor}

    async def c_cmd_use_myclub_management_item(self, s, m):
        """B.21: objeto de gestión: +1 de gestión del DT por objeto (P), tope 99."""
        return self._usar_objeto_dt(s, m, "gestion", "management", 1, 99)

    async def c_cmd_use_myclub_adaptability_rate_item(self, s, m):
        """B.21: objeto de adaptación: +5 % de adaptación del DT por objeto (P), tope 100."""
        return self._usar_objeto_dt(s, m, "adaptacion", "adaptability_rate", 5, 100)

    async def c_cmd_use_myclub_tactical_training(self, s, m):
        """B.21: entrenamiento táctico de una plantilla. P: cada objeto da
        myclub.exp_tactico (def. 100) de experiencia a los inscritos de squad_id."""
        club = self._club_inicial(s)
        if club is None:
            return {"result": "ERR_NOTFOUND"}
        veces = max(1, int(m.get("use_count") or 1))
        error = self._gastar_objeto(club, "tactico", veces)
        if error:
            return {"result": error}
        if not club.get("jugadores"):
            club["jugadores"] = self._plantel_inicial()[0]
        sq = self._id_entero(m.get("squad_id")) or self._activa(club)
        ganada = int((self.cfg.get("myclub") or {}).get("exp_tactico") or 100) * veces
        fichas = []
        for i, j in self._elegidos(club, club["jugadores"], sq):
            j["exp_total"] = int(j.get("exp_total") or 0) + ganada
            fichas.append(self._ficha_jugador_myclub(i, j))
        self.datos.guardar_club(s.steam_hex or "", club)
        return {"gameplayer_list": fichas}

    async def c_cmd_get_position_rating(self, s, m):
        """B.21 (P): valoración por posición del jugador online; hoy solo la propia.
        pos_rating_list = bins {position, rating, match_num} (forma P)."""
        club = self._club_propio(s) or {}
        datos = club.get("valoracion_posiciones") or {}
        lista = [Empaquetado({"position": pos, "rating": int(v[0] / max(1, v[1])), "match_num": int(v[1])})
                 for pos, v in datos.items()]
        total = sum(v[1] for v in datos.values())
        return {"pos_rating_list": lista,
                "total_position_rating": int(sum(v[0] for v in datos.values()) / total) if total else 0,
                "total_position_match_num": total}

    async def c_cmd_set_position_rating(self, s, m):
        club = self._club_propio(s)
        pos, nota = m.get("position"), m.get("rating")
        if club is not None and isinstance(pos, str) and pos and isinstance(nota, int):
            v = club.setdefault("valoracion_posiciones", {}).setdefault(pos, [0, 0])
            v[0] += nota
            v[1] += 1
            self.datos.guardar_club(s.steam_hex, club)
        return {}

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
        """B.3/B.17: historial de GP del club (lo más nuevo primero). Vacío salvo config
        myclub.historial = true, hasta ver en el PES cómo muestra los registros (tokens P)."""
        try:
            inicio = max(0, int(m.get("start_index") or 0))
        except (TypeError, ValueError):
            inicio = 0
        club = self._club_propio(s) if (self.cfg.get("myclub") or {}).get("historial") else None
        movs = list(reversed((club or {}).get("historial_gp") or []))
        pagina = [self._registro_historial(x) for x in movs[inicio:inicio + 100]]
        cuerpo = lambda: {"msgid": "CMD_GET_MYCLUB_HISTORYLIST", "rqid": 0, "result": "NOERR",
                          "myclub_history_list_total": len(movs), "index": inicio,
                          "myclub_history_list_num": len(pagina), "myclub_history_list": pagina}
        while pagina and _cifrado_comprimido(cuerpo()) > MAX_CIFRADO_CLIENTE:
            pagina = pagina[:len(pagina) * 3 // 4]  # que quepa en una trama (como GAMERESULTS)
        return {"myclub_history_list_total": len(movs), "index": inicio,
                "myclub_history_list_num": len(pagina), "myclub_history_list": pagina}

    MAX_HISTORIAL_GP = 100

    def _gp_actual(self, s, msgid):
        """GP del club antes de un comando que puede moverlos (solo comandos de myClub)."""
        if "MYCLUB" not in msgid and "GP_ACHIEVEMENT" not in msgid or not s.steam_hex:
            return None
        club = self.datos.club(s.steam_hex)
        return int(club.get("gp") or 0) if club else None

    def _anotar_movimiento_gp(self, s, msgid, antes):
        """Si el comando cambió los GP, queda un registro para el historial (B.3)."""
        club = self.datos.club(s.steam_hex)
        if not club:
            return
        delta = int(club.get("gp") or 0) - antes
        if not delta:
            return
        club["historial_gp"] = ((club.get("historial_gp") or []) + [
            {"hora": int(time.time()), "cmd": msgid, "gp": delta}])[-self.MAX_HISTORIAL_GP:]
        self.datos.guardar_club(s.steam_hex, club)

    @staticmethod
    def _registro_historial(mov):
        """P (B.17): in = lo que el club dio, out = lo que recibió; tipo y data_type «GP»."""
        objeto = lambda: Empaquetado({"item_type": "", "item_variation": "", "item_effect_arg": 0,
                                      "count": 0, "pes_point_price": 0})
        delta = int(mov.get("gp") or 0)
        return Empaquetado({
            "history_data_kind": "GP",
            "history_data_in": Empaquetado({"data_type": "GP", "entity_id": 0,
                                            "count": -delta if delta < 0 else 0, "item_data": objeto()}),
            "history_data_out": Empaquetado({"count": delta if delta > 0 else 0, "item_data": objeto()})})

    # ---------- Regalos (B.4): «Bandeja entrada» de myClub ----------
    # club["regalos"] = [{id, tipo: gp|monedas|<clave de objeto>, cantidad, titulo, hora, hasta}].
    # Se dan con tools/admin_myclub.py (regalo …) o el de bienvenida de config
    # (myclub.regalo_bienvenida = [{tipo, cantidad, titulo}], una sola vez por club).
    MAX_REGALOS = 30

    @staticmethod
    def dar_regalo(club, tipo, cantidad, titulo="", dias=30):
        """Pone un regalo en la bandeja del club (lo usan admin y el servidor)."""
        regalos = club.setdefault("regalos", [])
        nuevo = max([int(r.get("id") or 0) for r in regalos] + [int(club.get("regalo_ultimo_id") or 0)]) + 1
        club["regalo_ultimo_id"] = nuevo
        ahora = int(time.time())
        regalos.append({"id": nuevo, "tipo": str(tipo), "cantidad": int(cantidad),
                        "titulo": str(titulo or ""), "hora": ahora, "hasta": ahora + int(dias) * 86400})
        return nuevo

    def _regalos(self, club):
        """Regalos vigentes; da una vez el de bienvenida de config."""
        if club is None:
            return []
        bienvenida = (self.cfg.get("myclub") or {}).get("regalo_bienvenida") or []
        if bienvenida and not club.get("bienvenida_dada"):
            for r in bienvenida:
                if isinstance(r, dict) and r.get("tipo") and int(r.get("cantidad") or 0) > 0:
                    self.dar_regalo(club, r["tipo"], r["cantidad"], r.get("titulo") or "Bienvenida")
            club["bienvenida_dada"] = True
        ahora = int(time.time())
        club["regalos"] = [r for r in club.get("regalos") or [] if int(r.get("hasta") or 0) > ahora]
        return club["regalos"]

    def _datos_regalo(self, r):
        """Campos del regalo para el cliente: (type, nombre, item{…})."""
        tipo, n = r.get("tipo"), int(r.get("cantidad") or 0)
        # B.22 (prueba): item_type "" no es ninguno de los 8 tokens del cliente; config
        # myclub.regalo_item_type_moneda lo cambia (p. ej. "NotImplement" o "STORE") para GP/monedas.
        tok = str((self.cfg.get("myclub") or {}).get("regalo_item_type_moneda") or "")
        objeto = {"item_type": tok if tipo in ("gp", "monedas") else "", "item_variation": "",
                  "item_effect_arg": 0, "count": 0, "pes_point_price": 0}
        # Prueba 12-nov: el aviso falso de «máximo» sale de la LISTA; el objeto va vacío (count 0).
        # myclub.regalo_lista_modo: "count" = item.count=n; "tipo_item" = type "ITEM" con count=n.
        modo = str((self.cfg.get("myclub") or {}).get("regalo_lista_modo") or "")
        if modo in ("count", "tipo_item") and tipo in ("gp", "monedas"):
            objeto["count"] = n
        if tipo == "gp":
            return ("ITEM" if modo == "tipo_item" else "GP"), r.get("titulo") or f"{n} GP", objeto
        if tipo == "monedas":
            return "PES_POINT", r.get("titulo") or f"{n} monedas", objeto
        o = next((x for x in self._objetos() if x.get("clave") == tipo), {})
        objeto.update({"item_type": str(o.get("item_type") or ""),
                       "item_variation": str(o.get("item_variation") or ""), "count": n})
        return "ITEM", r.get("titulo") or "Objeto", objeto

    async def c_cmd_get_myclub_presentlist(self, s, m):
        """B.4: los regalos pendientes del club (lista vacía válida si no hay)."""
        club = self._club_propio(s)
        regalos = self._regalos(club)
        if club is not None:
            self.datos.guardar_club(s.steam_hex, club)
        lista = []
        for r in regalos[:self.MAX_REGALOS]:
            tipo, nombre, objeto = self._datos_regalo(r)
            mc = self.cfg.get("myclub") or {}
            if tipo == "GP" and mc.get("regalo_type_gp"):
                tipo = str(mc["regalo_type_gp"])
            ficha = {
                "present_id": int(r["id"]), "name": nombre, "name_jp": nombre, "title": nombre,
                "body": "", "entity_id": 0, "grade": 0, "type": tipo, "player_level": 0,
                "contract_term": 0, "team_id": 0, "present_type": "NORMAL",
                "item": Empaquetado(objeto), "receipt_deadline": int(r["hasta"]),
                "count": int(r.get("cantidad") or 0)}
            # L (Ghidra 12-nov, FUN_141c473e0): en la ficha «skill» e «item» son LISTAS de mapas directos.
            # Los `type` que conoce el cliente: E_FOOTBALL_POINT, PLAYER, UNIFORM, ITEM, RENTAL_PLAYER,
            # SCOUT, AGENT, COACH, PES_POINT («GP» no existe ahí). Modos en caliente de regalo_lista_modo:
            # "listas" (elemento bin, item/skill listas) y "listas_directo" (elemento mapa directo).
            # Pruebas en caliente sin tocar código: myclub.regalo_ficha_extra = {clave: valor} se mezcla
            # en la ficha de la lista (p. ej. {"present_type": "GM"}); valor null quita la clave
            for k, v in (mc.get("regalo_ficha_extra") or {}).items():
                if v is None:
                    ficha.pop(k, None)
                else:
                    ficha[k] = v
            modo_l = str(mc.get("regalo_lista_modo") or "")
            if modo_l in ("listas", "listas_directo"):
                ficha["item"] = [dict(objeto, count=int(r.get("cantidad") or 0))]
                ficha["skill"] = []
            elif modo_l in ("mapas", "mapas_directo"):
                # igual que en RECV: item y skill como MAPAS directos (paso 0x30 en el lector)
                ficha["item"] = dict(objeto, count=int(r.get("cantidad") or 0))
                ficha["skill"] = {"skill_category": "", "skill_id": 0}
            lista.append(ficha if modo_l.endswith("_directo") else Empaquetado(ficha))
            prueba = {"msgid": "CMD_GET_MYCLUB_PRESENTLIST", "rqid": 0, "result": "NOERR",
                      "present_list_num": len(lista), "present_list": lista}
            if _cifrado_comprimido(prueba) > MAX_CIFRADO_CLIENTE:
                lista.pop()
                break
        return {"present_list_num": len(lista), "present_list": lista}

    def _sesion_http(self, formulario, ip_cliente):
        """Sesión TCP de quien llama por HTTP: por su pid y, si no, por su IP."""
        try:
            pid = int(str(formulario.get("pid") or "0"))
        except ValueError:
            pid = 0
        con_club = [x for x in self.sesiones if x.steam_hex]
        por_pid = [x for x in con_club if pid and pid in (x.pid, self._pid_visible(x))]
        if por_pid:
            return por_pid[0]
        ip = (ip_cliente or "").split("%")[0].removeprefix("::ffff:")
        por_ip = [x for x in con_club if (x.ip or "").split("%")[0].removeprefix("::ffff:") == ip]
        return por_ip[0] if len(por_ip) == 1 else None

    def recibir_regalos(self, formulario, ip_cliente=None):
        """B.4 (HTTP): cobra un regalo (present_id) o todos (is_all YES)."""
        self._recargar_config()
        s = self._sesion_http(formulario, ip_cliente)
        club = self._club_propio(s) if s is not None else None
        if club is None:
            return {"result": "ERR_NOTFOUND", "receive_result_list_num": 0, "receive_result_list": []}
        todos = str(formulario.get("is_all") or "").upper() in ("YES", "1", "TRUE")
        try:
            elegido = int(str(formulario.get("present_id") or "0"))
        except ValueError:
            elegido = 0
        cobrar = [r for r in self._regalos(club) if todos or int(r["id"]) == elegido]
        salida = []
        fichas = []
        gp_antes = int(club.get("gp") or 0)
        for r in cobrar:
            tipo, n = r.get("tipo"), int(r.get("cantidad") or 0)
            if tipo == "gp":
                club["gp"] = int(club.get("gp") or 0) + n
            elif tipo == "monedas":
                club["monedas"] = int(club.get("monedas") or 0) + n
            else:
                club[tipo] = int(club.get(tipo) or 0) + n
            club["regalos"].remove(r)
            club["regalos_recibidos"] = int(club.get("regalos_recibidos") or 0) + 1
            data_type, nombre, objeto = self._datos_regalo(r)
            # L (Ghidra 11-nov): el lector da a cada elemento la ficha completa del regalo
            # (present_id, name, type, item{…}); modo de prueba «ficha» en regalos_respuesta
            fichas.append(Empaquetado({
                "present_id": int(r["id"]), "name": nombre, "name_jp": nombre, "title": nombre,
                "body": "", "entity_id": 0, "grade": 0, "type": data_type, "player_level": 0,
                "contract_term": 0, "team_id": 0, "present_type": "NORMAL",
                "item": Empaquetado(dict(objeto, count=n)), "receipt_deadline": int(r["hasta"]),
                "count": n}))
            salida.append({"name_jp": nombre, "title": nombre, "entity_id": 0, "grade": 0,
                           "skill": 0, "skill_category": "", "skill_id": 0, "player_level": 0,
                           "contract_term": 0, "team_id": 0, "present_type": "NORMAL",
                           **objeto, "count": n, "receipt_deadline": int(r["hasta"]),
                           "result": "NOERR"})
        delta = int(club.get("gp") or 0) - gp_antes
        if delta:
            club["historial_gp"] = ((club.get("historial_gp") or []) + [
                {"hora": int(time.time()), "cmd": "CMD_RECV_MYCLUB_PRESENT", "gp": delta}])[-self.MAX_HISTORIAL_GP:]
        self.datos.guardar_club(s.steam_hex, club)
        log.info("Regalos: %s cobra %d regalo(s)", s.steam_hex, len(salida))
        # Prueba en el PES (11-nov): cobra bien pero el juego avisa «ya tienes el máximo»
        # (ERR_RECV_PRESENT_FULL). Config myclub.regalos_respuesta para probar formas:
        # "normal" (result NOERR por elemento), "sin_result", "vacio" (""), "cero" (0),
        # "solo_top" (sin lista).
        modo = str((self.cfg.get("myclub") or {}).get("regalos_respuesta") or "elemento_item")
        # L (Ghidra 12-nov + reward_list semanal B.7): «skill» es un MAPA {skill_category, skill_id},
        # no un entero; mandar 0 puede romper el elemento. myclub.regalos_skill: "plano" (como antes),
        # "mapa" (mapa directo) o "bin" (mapa empaquetado)
        forma = str((self.cfg.get("myclub") or {}).get("regalos_skill") or "plano")
        if forma in ("mapa", "bin"):
            for e in salida:
                sk = {"skill_category": e.pop("skill_category", ""), "skill_id": e.pop("skill_id", 0)}
                e["skill"] = Empaquetado(sk) if forma == "bin" else sk
        for e in salida:
            if modo == "sin_result":
                e.pop("result", None)
            elif modo == "vacio":
                e["result"] = ""
            elif modo == "cero":
                e["result"] = 0
        if modo in ("plano_top", "plano_top_ficha") and salida:
            # Prueba 12-nov: el aviso «máximo» sale aun con solo_top, o sea no viene de la lista. El
            # commands.json de AzrakHQ da a esta respuesta campos PLANOS arriba (present_id, name, item,
            # count, item_type…) además de la lista: se manda el primer regalo también plano
            primero = dict(fichas[0].valor) if (modo == "plano_top_ficha" and fichas) else dict(salida[0])
            primero.pop("result", None)
            primero.setdefault("present_id", int(cobrar[0]["id"]))
            primero.setdefault("body", "")
            primero.setdefault("type", self._datos_regalo(cobrar[0])[0])
            primero.setdefault("item_effect_arg", 0)
            primero.setdefault("pes_point_price", 0)
            return {"result": "NOERR", **primero, "receive_result_list_num": len(salida),
                    "receive_result_list": salida}
        if modo == "elemento_item":
            # L (Ghidra 12-nov, FUN_141cd7f00 descompilado entero): cada elemento de receive_result_list
            # es un MAPA DIRECTO {"item": <ficha del regalo>, "result": <texto>}. Dentro de la ficha,
            # «skill» e «item» son LISTAS de mapas directos (sin bin). El result por elemento vale
            # «NotImplement» si falta. Textos de prueba en caliente: myclub.regalos_result_elemento y
            # myclub.regalo_type_gp (el type «GP» se pinta como «Monedas de myClub»).
            mc = self.cfg.get("myclub") or {}
            # CAUSA DEL AVISO «máximo» (Ghidra+breakpoint 13-nov, FUN_1410d68d0): el cliente compara el
            # result de cada elemento con SUCCESS_ADDED / SUCCESS_EXTEND / FAIL_DUPLICATE / FAIL_FULL
            # (FUN_1402218c0); con NOERR no coincide ninguna, cuenta 0 y muestra el texto de «lleno»
            res_elem = str(mc.get("regalos_result_elemento") or "SUCCESS_ADDED")
            elems = []
            for f in fichas:
                e = dict(f.valor)
                if e.get("type") == "GP" and mc.get("regalo_type_gp"):
                    e["type"] = str(mc["regalo_type_gp"])
                # Corrección (misma lectura): item y skill se recorren con paso 0x30 = MAPAS directos,
                # no listas (paso 0x18). myclub.regalos_anidado="listas" deja la forma anterior de prueba.
                if str(mc.get("regalos_anidado") or "mapas") == "listas":
                    e["skill"] = []
                    e["item"] = [dict(e["item"].valor)]
                else:
                    e["skill"] = {"skill_category": "", "skill_id": 0}
                    e["item"] = dict(e["item"].valor)
                # igual en RECV: myclub.regalo_recv_extra se mezcla en la ficha del elemento
                for k, v in (mc.get("regalo_recv_extra") or {}).items():
                    if v is None:
                        e.pop(k, None)
                    else:
                        e[k] = v
                elems.append({"item": e, "result": res_elem})
            return {"result": "NOERR" if elems else "ERR_NOTFOUND",
                    "receive_result_list_num": len(elems), "receive_result_list": elems}
        if modo in ("ficha_result", "ficha_result_mapa"):
            # Prueba 12-nov: combinación nunca probada. El lector (FUN_141cd7f00) lee por elemento la
            # ficha ANIDADA (item{…}, skill{…}) Y una clave result; «normal» manda result pero plano,
            # y «ficha» manda anidado pero sin result. Aquí van las dos cosas juntas.
            completas = []
            for f in fichas:
                e = dict(f.valor)
                e["skill"] = Empaquetado({"skill_category": "", "skill_id": 0})
                e["result"] = "NOERR"
                completas.append(Empaquetado(e) if modo == "ficha_result" else e)
            return {"result": "NOERR" if completas else "ERR_NOTFOUND",
                    "receive_result_list_num": len(completas), "receive_result_list": completas}
        if modo == "ficha":
            return {"result": "NOERR" if fichas else "ERR_NOTFOUND",
                    "receive_result_list_num": len(fichas), "receive_result_list": fichas}
        if modo == "solo_top":
            return {"result": "NOERR" if salida else "ERR_NOTFOUND"}
        return {"result": "NOERR" if salida else "ERR_NOTFOUND",
                "receive_result_list_num": len(salida), "receive_result_list": salida}

    async def c_cmd_get_myclub_achievementlist(self, s, m):
        """B.5/B.17: logros del club. Apagados (lista vacía) salvo config myclub.logros.activo."""
        club = self._club_propio(s)
        lista = [self._logro_bin(club, l) for l in self._logros(club, s)] if club is not None else []
        return {"achievement_list_num": len(lista), "achievement_list": lista}

    async def c_cmd_send_myclub_read_achievementlist(self, s, m):
        return {}

    # ---------- logros (B.14 y B.17; los textos y el sentido de value/target son P) ----------
    # categoría: [(meta, GP del premio), ...]; el progreso sale del récord contra la COM
    # B.19 (Ghidra): el cliente SOLO acepta estas 24 categorías (otra = el logro no se pinta).
    CATEGORIAS_LOGRO = (
        "RECV_PRESENT", "PLAY_VSCOM_MATCH", "JOIN_VSCOM_COMPE", "PLAY_CASUAL_MATCH", "PLAY_CLAN_MATCH",
        "PLAY_RANK_MATCH_COACH", "PLAY_RANK_MATCH", "JOIN_EVENT_COMPE", "GET_PLAYER_BY_AGENT",
        "GET_PLAYER_BY_SCOUT", "GET_PLAYER_BY_RENTAL", "GET_COACH", "EXTEND_PLAYER_CONTRACT",
        "RECOVERY_ITEM_USE", "TRAINING_PLAYER", "CREATE_SQUAD", "SET_CLOUD_MATCH_SQUAD", "SET_AVATAR",
        "CHECK_TACTICS_COMPARISON_MENU", "PLAY_SPECIAL_MATCH", "GET_SKILL_RANK", "SUCCESSFUL_BID_SCOUT",
        "GET_JUST_ONE_PLAYER_BY_SCOUT", "PERFECT_TEAM_SPIRIT")
    # categoría: [[meta, GP del premio], ...] (hitos en orden); solo las que el servidor puede medir
    LOGROS_DEFECTO = {
        "PLAY_VSCOM_MATCH": [[1, 1000], [10, 3000], [50, 10000], [100, 20000]],
        "PLAY_RANK_MATCH": [[1, 1000], [10, 3000], [50, 10000]],
        "GET_COACH": [[1, 2000]],
        "CREATE_SQUAD": [[1, 1000]],
        "SET_CLOUD_MATCH_SQUAD": [[1, 1000]],
    }

    def _progreso_logros(self, club, s=None):
        com = club.get("vscom") or {}
        prog = {"PLAY_VSCOM_MATCH": int(com.get("partidos") or 0),
                "GET_COACH": len(club.get("dt_comprados") or []),
                "CREATE_SQUAD": int(club.get("plantillas_creadas") or 0),
                "SET_CLOUD_MATCH_SQUAD": 1 if club.get("plantilla_nube") else 0,
                "JOIN_EVENT_COMPE": int(club.get("eventos_inscritos") or 0),
                "RECV_PRESENT": int(club.get("regalos_recibidos") or 0)}
        rank = 0
        if s is not None and s.steam_hex:
            try:
                e = div.completar(self.datos.division(s.steam_hex))
                rank = e["jugados"] + sum(int(a.get("jugados") or 0) for a in e["archivadas"])
            except Exception:
                rank = 0
        prog["PLAY_RANK_MATCH"] = rank
        return prog

    def _logros(self, club, s=None):
        """[(categoría, progreso, [(meta, gp)], hitos cobrados)] o [] si están apagados."""
        cfg = (self.cfg.get("myclub") or {}).get("logros") or {}
        if not cfg.get("activo") or club is None:
            return []
        progreso = self._progreso_logros(club, s)
        # ojo: «logros_cobrados» (lista) ya lo usa GP_ACHIEVEMENT; los de myClub van aparte
        cobrados = club.get("logros_myclub") if isinstance(club.get("logros_myclub"), dict) else {}
        salida = []
        tabla = dict(cfg.get("premios") or self.LOGROS_DEFECTO)
        if not cfg.get("premios") and self._eventos():
            tabla.setdefault("JOIN_EVENT_COMPE", [[1, 1000], [5, 3000]])
        for cat, hitos in tabla.items():
            if cat not in self.CATEGORIAS_LOGRO or cat not in progreso:
                continue  # el cliente no la pinta, o el servidor aún no sabe medirla
            hitos = sorted((int(h[0]), int(h[1])) for h in hitos
                           if isinstance(h, (list, tuple)) and len(h) == 2 and int(h[0]) > 0)
            if hitos:
                salida.append((cat, progreso[cat], hitos, min(int(cobrados.get(cat) or 0), len(hitos))))
        return salida

    def _premio_logro(self, gp):
        """(campo del club, cantidad). C (11-nov, PES): el cliente SIEMPRE muestra el premio de logro
        como «Monedas de myClub». Con myclub.logros.moneda = "monedas" (y monedas_gratis encendido)
        se acreditan monedas: gp / logros.gp_por_moneda (100 por defecto), mínimo 1. Si no, GP."""
        cfg = self.cfg.get("myclub") or {}
        if str((cfg.get("logros") or {}).get("moneda") or "gp") == "monedas" and self._monedas_activas():
            por = max(1, int((cfg.get("logros") or {}).get("gp_por_moneda") or 100))
            return "monedas", max(1, gp // por)
        return "gp", gp

    def _tipo_premio_logro(self):
        """«type» del regalo según lo que de verdad se paga (monedas = PES_POINT, si no GP)."""
        return "PES_POINT" if self._premio_logro(1)[0] == "monedas" else "GP"

    def _resumen_logros(self, club, s=None):
        """Resumen de MAINMENU_INFO: hitos en total, alcanzados y alcanzados sin cobrar (P: el
        sentido de cada contador sale de sus nombres)."""
        total = alcanzados = sin_cobrar = 0
        for _, valor, hitos, cobrados in self._logros(club, s):
            total += len(hitos)
            hechos = sum(1 for meta, _ in hitos if valor >= meta)
            alcanzados += hechos
            sin_cobrar += max(0, hechos - cobrados)
        return {"achievement_not_recieved_num": sin_cobrar, "achievement_num": alcanzados,
                "achievement_total_num": total}

    def _premio_monedas(self, n, monedas):
        """Regalo de monedas gratis (premio de evento): type PES_POINT, count = monedas."""
        return self._premio_gp(n, monedas, cantidad=monedas, tipo="PES_POINT", nombre=f"{monedas} monedas")

    def _premio_gp(self, n, gp, cantidad=None, tipo="GP", nombre=None):
        """Regalo de B.14 con todos los campos; el premio real (GP) lo suma el servidor al cobrar.
        Prueba (10-nov: el PES descartó los logros con estos valores): config myclub.logros.regalo
        = {clave: valor} sobrescribe campos del regalo; logros.regalo_quitar = [claves] los quita."""
        cfg = (self.cfg.get("myclub") or {}).get("logros") or {}
        # L (Ghidra 11-nov): el icono/texto del premio sale de «type» (tabla data_type: GP, PES_POINT…;
        # sin «type» = PES_POINT = la estrella); present_type es NORMAL/GM/EVENTER/KONAMISTAFF;
        # el objeto va anidado en «item» (bin)
        regalo = {
            "present_id": n, "name": nombre or f"{gp} GP", "name_jp": nombre or f"{gp} GP",
            "title": nombre or f"{gp} GP", "body": "", "entity_id": 0, "grade": 0, "type": tipo, "player_level": 0, "contract_term": 0,
            "team_id": 0, "present_type": "NORMAL",
            "item": Empaquetado({"item_type": "", "item_variation": "", "item_effect_arg": 0,
                                 "count": 0, "pes_point_price": 0}),
            "receipt_deadline": 0, "count": self._premio_logro(gp)[1] if cantidad is None else cantidad}
        regalo.update({k: v for k, v in (cfg.get("regalo") or {}).items() if isinstance(v, (int, str))})
        for k in cfg.get("regalo_quitar") or []:
            regalo.pop(k, None)
        return Empaquetado(regalo)

    def _logro_bin(self, club, logro):
        cat, valor, hitos, cobrados = logro
        vistos = club.get("logros_vistos") or []
        cfg = (self.cfg.get("myclub") or {}).get("logros") or {}
        hitos_bin = []
        for i, (meta, gp) in enumerate(hitos):
            hito = {"target_value": meta, "value": min(valor, meta), "received_milestone_level": cobrados}
            if not cfg.get("sin_regalo"):
                hito["present"] = self._premio_gp(i + 1, gp)
            hitos_bin.append(Empaquetado(hito))
        listo = cobrados < len(hitos) and valor >= hitos[cobrados][0]
        # C (10-nov, PES): value y received_milestone_level los lee el cliente AL NIVEL DEL LOGRO
        # (dentro del hito no se usan: la barra salía 0/1); se dejan también en el hito por si acaso
        logro = {"achievement_category": cat, "value": valor, "received_milestone_level": cobrados,
                 "received_all": "YES" if cobrados >= len(hitos) else "NO",
                 "is_new": "YES" if listo and cat not in vistos else "NO",
                 "milestone_list_num": len(hitos_bin), "milestone_list": hitos_bin}
        # prueba: campos extra en cada logro (p. ej. {"achievement_id": 0})
        logro.update({k: v for k, v in (cfg.get("logro_extra") or {}).items() if isinstance(v, (int, str))})
        return Empaquetado(logro)

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
        for n, (cat, valor, hitos, cobrados) in enumerate(self._logros(club, s)):
            if not todos and cat != pedido:
                continue
            ganado = 0
            while cobrados < len(hitos) and valor >= hitos[cobrados][0]:
                ganado += hitos[cobrados][1]
                cobrados += 1
            if not ganado:
                continue
            club.setdefault("logros_myclub", {})[cat] = cobrados
            moneda, cantidad = self._premio_logro(ganado)
            club[moneda] = int(club.get(moneda) or 0) + cantidad
            recibidos.append(Empaquetado({
                "achievement_category": cat, "achievement_id": self.CATEGORIAS_LOGRO.index(cat) + 1,
                "achievement_bonus": Empaquetado({"present": self._premio_gp(n + 1, ganado)}),
                "received_all": "YES" if cobrados >= len(hitos) else "NO",
                "received_milestone_level": cobrados}))
            log.info("myClub %s cobra el logro %s: +%d %s", s.steam_hex, cat, cantidad, moneda)
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
        club["plantillas_creadas"] = int(club.get("plantillas_creadas") or 0) + 1
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


    # ---------- Eventos de competición online (docs/ESPEC_EVENTOS_ONLINE.md, parte C) ----------
    # Los eventos salen de config myclub.eventos = [{compe_id, compe_name, compe_kind, inicio, fin,
    # dificultad, ...}] (más adelante, de la web). Sin eventos = lista vacía (pantalla vacía válida).
    MAX_EVENTOS = 20

    def _eventos(self):
        ahora = int(time.time())
        salida = []
        for e in (self.cfg.get("myclub") or {}).get("eventos") or []:
            if isinstance(e, dict) and isinstance(e.get("compe_id"), int):
                salida.append({**e, "inicio": int(e.get("inicio") or ahora - 86400),
                               "fin": int(e.get("fin") or ahora + 30 * 86400)})
        return salida[:self.MAX_EVENTOS]

    def _evento(self, compe_id):
        return next((e for e in self._eventos() if e["compe_id"] == compe_id), None)

    @staticmethod
    def _evento_bin(e, extra=None):
        t = lambda k, d="": str(e.get(k)) if isinstance(e.get(k), (str, int)) and e.get(k) != "" else d
        i = lambda k: int(e.get(k) or 0) if isinstance(e.get(k), (int, float)) else 0
        sin_archivo = Empaquetado({"filename": "", "hash_str": ""})
        dif = t("dificultad", "COMPE_DIFFICULTY_3")  # con NONE el PES hacía matchmaking humano (11-nov)
        datos = {
            "compe_id": e["compe_id"], "compe_name": t("compe_name", f"Evento {e['compe_id']}"),
            "compe_kind": t("compe_kind", "COMEVENT"),
            "compe_start_time": e["inicio"], "compe_end_time": e["fin"],
            "compe_cpu_level": dif, "difficulty": dif,
            "is_cleared": "NO", "is_locked": "NO", "is_coop_match": "NO", "is_squad_bonus": "NO",
            "add_limit_vscom_win_rate": i("add_limit_vscom_win_rate"),
            "add_limit_rating_min": i("add_limit_rating_min"), "add_limit_rating_max": i("add_limit_rating_max"),
            "add_limit_match_num_min": i("add_limit_match_num_min"),
            "add_limit_match_num_max": i("add_limit_match_num_max"),
            "filename": "", "hash_str": "", "banner_file_info": sin_archivo,
            "list_banner_file_info": Empaquetado({"filename": "", "hash_str": ""})}
        datos.update(extra or {})
        # Para pruebas en el PES: el evento de config puede pisar cualquier campo con
        # «campos»: {clave: valor} (p. ej. is_squad_bonus "false" en vez de "NO")
        datos.update({k: v for k, v in (e.get("campos") or {}).items() if isinstance(v, (int, str))})
        return Empaquetado(datos)

    def _inscripcion(self, s, modo=None):
        club = self._club_propio(s)
        ins = (club or {}).get("eventos_inscripcion") or {}
        return club, (ins.get(str(modo)) if modo is not None else ins)

    def _inscripcion_activa(self, s):
        """(modo, inscripción) del evento en curso, o (None, None). El partido vs COM no trae
        compe_mode (C.8): vale la primera inscripción guardada."""
        club = self._club_propio(s)
        for modo, ins in ((club or {}).get("eventos_inscripcion") or {}).items():
            if isinstance(ins, dict) and self._evento(ins.get("compe_id")) is not None:
                return modo, ins
        return None, None

    def _marca_evento(self):
        """P (C.8): texto de is_entry_compe con evento inscrito. Config myclub.marca_evento."""
        return str((self.cfg.get("myclub") or {}).get("marca_evento") or "YES")

    @staticmethod
    def _umbral_evento(e):
        """(partidos para superar, puntos para superar) del evento; 1 partido y 0 puntos si no
        se dice nada en config."""
        return max(1, int(e.get("partidos") or 1)), max(0, int(e.get("puntos_para_superar") or 0))

    def _superados(self, club):
        return (club or {}).get("eventos_superados") or {}

    def _evento_extra(self, club, e):
        veces = int(self._superados(club).get(str(e["compe_id"])) or 0)
        return {"is_cleared": "YES" if veces else "NO", "cleared_count": veces}

    async def c_cmd_get_event_category_list(self, s, m):
        """C.8 extra: categorías de la pantalla Eventos. Una sola categoría con todos los eventos
        de config (texto en myclub.evento_categoria; P: «COMEVENT»). Sin eventos = lista vacía."""
        # C (§16.9, probado): sin eventos = lista vacía y el emblema de jornada vacío
        emblema = Empaquetado({
            "is_use_emblem": "NO", "is_auto_make_compe_name": "NO", "matchday_group_type": "",
            "team_list_group_a": [], "team_list_group_b": [], "compe_db_id": 0})
        eventos = self._eventos()
        if not eventos:
            return {"event_category_list": [], "matchday_emblem_info": emblema}
        club = self._club_propio(s)
        sin_superar = sum(1 for e in eventos if not self._superados(club).get(str(e["compe_id"])))
        # P: el texto de categoría no se conoce aún. myclub.evento_categoria puede ser una lista:
        # una categoría por texto (para probar varios a la vez en el PES; sobra lo que no reconoce)
        textos = (self.cfg.get("myclub") or {}).get("evento_categoria") or "COMEVENT"
        textos = [str(x) for x in (textos if isinstance(textos, list) else [textos]) if x][:12]
        lista = [Empaquetado({
            "event_category": texto, "open_event_num": len(eventos), "not_cleared_event_num": sin_superar,
            "recent_event_start_time": max(e["inicio"] for e in eventos),
            "banner_file_info": Empaquetado({"filename": "", "hash_str": ""})}) for texto in textos]
        return {"event_category_list": lista, "matchday_emblem_info": emblema}

    def _vscom_equipo_evento(self, ins):
        e = self._evento(ins.get("compe_id")) if ins else None
        v = (e or {}).get("vscom_team_id")
        if isinstance(v, int) and v > 0:
            return v
        riv = (self.cfg.get("myclub") or {}).get("rival") or {}
        v = riv.get("base_team_id")
        return v if isinstance(v, int) and v > 0 else 109

    @staticmethod
    def _nivel_cpu_evento(ins):
        n = str((ins or {}).get("cpu_level") or "")
        if n.startswith("COMPE_DIFFICULTY_"):
            return n
        return {"EASY": "COMPE_DIFFICULTY_2", "NORMAL": "COMPE_DIFFICULTY_3",
                "HARD": "COMPE_DIFFICULTY_4"}.get(n, "COMPE_DIFFICULTY_3")

    async def c_cmd_get_compev_mainmenu_info(self, s, m):
        club, ins = self._inscripcion(s, m.get("compe_mode"))
        ins = ins or {}
        return {"status": "ENTRY" if ins else "", "compe_result": str(ins.get("resultado") or "NORESULT"),
                "win_point": int(ins.get("puntos") or 0),
                "progress_game_count": int(ins.get("progreso") or 0),
                # Prueba 11-nov: con vscom_team_id 0 («Próximo rival: -») el PES buscó rival humano
                # (Matchmaking) en vez de jugar contra la COM. cpu_level ha de ser un token del
                # cliente (COMPE_DIFFICULTY_1..5/NONE), no «NORMAL».
                "vscom_team_id": self._vscom_equipo_evento(ins),
                "cpu_level": self._nivel_cpu_evento(ins),
                "clear_count": int(self._superados(club).get(str(ins.get("compe_id"))) or 0) if ins else 0}

    async def c_cmd_get_compev_list(self, s, m):
        club = self._club_propio(s)
        lista = [self._evento_bin(e, self._evento_extra(club, e)) for e in self._eventos()]
        return {"compe_list_kind": str(m.get("compe_list_kind") or ""), "compe_num": len(lista),
                "compe_list": lista, "banner_file_info": Empaquetado({"filename": "", "hash_str": ""})}

    async def c_cmd_get_compev_info(self, s, m):
        e = self._evento(self._id_entero(m.get("compe_id")))
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        partidos, puntos = self._umbral_evento(e)
        premio = int(e.get("premio_gp") or 0)
        monedas = int(e.get("premio_monedas") or 0) if self._monedas_activas() else 0
        # Prueba en el PES (11-nov): sin estos campos el cliente decide solo «No cumples los
        # requisitos» y no manda JOIN_COMPEV. Por defecto, requisitos abiertos; cada evento de
        # config puede fijar los suyos con las mismas claves.
        abiertos = {"entry_national_id": 0, "entry_team_id": 0, "entry_player_num": 0,
                    "entry_teamspirit_min": 0, "entry_teamspirit_max": 999,
                    "entry_teamstrength_min": 0, "entry_teamstrength_max": 99999,
                    "entry_coach_id": 0, "player_num": 11}
        requisitos = {k: int(e[k]) if isinstance(e.get(k), int) else v for k, v in abiertos.items()}
        hay_premio = "YES" if premio > 0 or monedas > 0 else "NO"
        # C.11 (Ghidra, lector FUN_141c849d0): en la raíz solo se lee compe_data; dentro va
        # compe_info (el evento con requisitos y compe_id) y el resto del detalle. Mandado plano,
        # el cliente dejaba compe_id y entry_* en 0 → «No cumples los requisitos».
        compe_info = self._evento_bin(e, {**requisitos, "entry_type": str(e.get("entry_type") or ""),
                                          **self._evento_extra(self._club_propio(s), e)})
        premios = []
        if premio > 0:
            premios.append(self._premio_gp(1, premio, cantidad=premio))
        if monedas > 0:
            premios.append(self._premio_monedas(2, monedas))
        lista_premios = [Empaquetado({"cleared_count": 0, "compe_reward": p, "is_compe_reward_exist": "YES"})
                         for p in premios]
        detalle = {"compe_info": compe_info, "clear_match_num": partidos, "clear_win_point": puntos,
                   "ad_file_name": "", "logo_file_name": "", "event_compe_reward_list": lista_premios,
                   "is_compe_reward_exist": hay_premio, "is_same_team_power_level": "NO",
                   "is_limited_matching_target": "NO", "precondition_compe_id": 0,
                   "precondition_compe_name": "", "is_exist_unlocked_compe_by_cleared": "NO",
                   "squad_bonus": Empaquetado({"reward": premios[0] if premios else Empaquetado({})}),
                   "is_pesleague_stadium": "NO", "is_efootball_pro_stadium": "NO"}
        if (self.cfg.get("myclub") or {}).get("eventos_info_plano") == "YES":
            # respaldo: la forma vieja (plana) que se usó hasta el 11-nov
            extra = {**requisitos, **{k: v for k, v in detalle.items() if k not in (
                "compe_info", "event_compe_reward_list", "squad_bonus")}}
            return {"compe_data": self._evento_bin(e, extra), "compe_info": self._evento_bin(e, extra),
                    "event_compe_reward_list": premios}
        return {"compe_data": Empaquetado(detalle)}

    async def c_cmd_get_compev_myentry_compeid(self, s, m):
        modo = m.get("compe_mode")
        _, ins = self._inscripcion(s, modo)
        ins = ins or {}
        return {"compe_mode": str(modo or ""), "compe_id": int(ins.get("compe_id") or 0),
                "compe_cpu_level": str(ins.get("cpu_level") or "COMPE_DIFFICULTY_NONE"),
                "progress_game_count": int(ins.get("progreso") or 0)}

    async def c_cmd_get_myclub_squad_bonus_target_playerlist(self, s, m):
        """L (Ghidra 11-nov): respuesta {gameplayer_id_list, gameplayer_id_list_num,
        need_player_num}. El cliente comprueba con esto el «bonus de plantilla» antes de
        inscribir; sin estas claves dice «No cumples los requisitos». Por evento en config:
        bonus_jugadores (ids de jugador) y bonus_necesarios (def. 0 = no exige nada)."""
        e = self._evento(self._id_entero(m.get("compe_id"))) or {}
        ids = [int(x) for x in e.get("bonus_jugadores") or [] if isinstance(x, int)]
        # L (Ghidra 11-nov, tras un cierre con enteros sueltos): cada elemento es un bin
        # {gameplayer_id}
        return {"gameplayer_id_list": [Empaquetado({"gameplayer_id": x}) for x in ids],
                "gameplayer_id_list_num": len(ids),
                "need_player_num": int(e.get("bonus_necesarios") or 0)}

    async def c_cmd_join_compev(self, s, m):
        e = self._evento(self._id_entero(m.get("compe_id")))
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        ahora = int(time.time())
        if ahora < e["inicio"]:
            return {"result": "ERR_COMPE_BEFORE_ENTRYTIME"}
        if ahora > e["fin"]:
            return {"result": "ERR_COMPE_AFTER_ENTRYTIME"}
        club = self._club_propio(s)
        if club is None:
            return {"result": "ERR_NOTREGISTERED"}
        club.setdefault("eventos_inscripcion", {})[str(m.get("compe_mode") or "")] = {
            "compe_id": e["compe_id"], "cpu_level": str(m.get("compe_cpu_level") or "COMPE_DIFFICULTY_NONE"),
            "progreso": 0, "puntos": 0, "resultado": "NORESULT", "hora": ahora}
        club["eventos_inscritos"] = int(club.get("eventos_inscritos") or 0) + 1
        self.datos.guardar_club(s.steam_hex, club)
        log.info("Eventos: %s se inscribe en el evento %d", s.steam_hex, e["compe_id"])
        return {}

    async def c_cmd_leave_compev(self, s, m):
        club, ins = self._inscripcion(s, m.get("compe_mode"))
        if not ins:
            return {"result": "ERR_COMPE_NOTENTRY"}
        club["eventos_inscripcion"].pop(str(m.get("compe_mode") or ""), None)
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_reentry_compev(self, s, m):
        e = self._evento(self._id_entero(m.get("compe_id")))
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        if int(time.time()) > e["fin"]:
            return {"result": "ERR_COMPE_AFTER_ENTRYTIME"}
        return {}

    async def c_cmd_join_compev_mode(self, s, m):
        return {}

    async def c_cmd_leave_compev_mode(self, s, m):
        return {}

    async def c_cmd_sync_compev_progress(self, s, m):
        return {}

    async def c_cmd_get_complg_myentry_compeid(self, s, m):
        return {"compe_id": 0}

    # ---------- Cooperativo, Clanes, Matchday y PES League (docs/ESPEC_COOP_MATCHDAY_PESLEAGUE.md) ----------
    # Por ahora pantallas vacías válidas (sin salas, clanes ni torneos). Matchday y PES League
    # leen torneos de config (myclub.matchday / pesleague = [{compe_id, compe_name, inicio, fin}]).
    async def c_cmd_get_coop_mainmenu_info(self, s, m):
        # L (D.8): is_exist_owner_clan = el jugador tiene clan (dueño o miembro); division = texto
        dueno, _ = self._clan_de(s.steam_hex)
        division = ""
        if (self.cfg.get("myclub") or {}).get("coop_division", "YES") != "NO" and s.steam_hex:
            division = str(self._estado_division(s.steam_hex).get("division") or "")
        return {"is_coop_banned": "NO", "is_season_end": "NO", "division": division, "pre_season_rank": 0,
                "pre_season_rank_all_clan_num": 0, "division_change_type": "",
                "is_exist_owner_clan": "YES" if dueno else "NO", "is_exist_new_request": "NO",
                "is_exist_new_pending": "NO"}

    async def c_cmd_get_is_join_coop_room(self, s, m):
        return {"is_join_coop_room": "YES" if self._sala_coop_de(s) else "NO"}

    async def c_cmd_get_coop_profile(self, s, m):
        """R (pescoop.py AzrakHQ, verificado con parsebench sobre el parser del PES): dos bin de primer
        nivel lado a lado, user_info{pid, pname} y coop_profile_data{...}. Antes respondíamos vacío y
        la ficha salía con valores por defecto."""
        mia = (s.steam_hex or "").lower()
        clave = (self._clave_de_id(m.get("pid")) if m.get("pid") else None) or mia
        d = self.datos.coop_datos(clave) if clave else {}
        sigo = self.datos.relaciones(mia)["sigo"] if mia else []
        nivel, puntos, jugados = self.nivel_coop(clave)
        datos = {"is_follow": "YES" if clave and clave != mia and clave in sigo else "NO",
                 "coop_level": nivel, "coop_level_point": puntos,
                 "follower": self.datos.seguidores(clave) if clave else 0,
                 "coop_manner": self.grado_modales(clave), "online_match": jugados, "vscom_match": 0,
                 "match_data_size": int(d.get("size") or 0), "match_data": str(d.get("data") or "")}
        return {"user_info": self._usuario_plano(clave) if clave else Empaquetado({}),
                "coop_profile_data": Empaquetado(datos)}

    # ---------- Seguir y rechazar jugadores (D.8, L): tabla relaciones ----------
    def _clave_de_id(self, ident):
        # R (pescoop._user_pids AzrakHQ): tras un Cooperativo el PES manda user_id bin{pid, pname{...}}
        if isinstance(ident, (bytes, bytearray)):
            try:
                ident = msgpack_min.unpackb(bytes(ident))
            except Exception:  # noqa: BLE001
                return None
        if isinstance(ident, Empaquetado):
            ident = ident.valor
        if isinstance(ident, dict):
            ident = ident.get("pid")
        n = self._id_entero(ident)
        if not n:
            return None
        for j in self.datos.todos():
            if self._es_id_de(n, j["steam_hex"]):
                return j["steam_hex"].lower()
        return None

    def _cambiar_relacion(self, s, campo, poner, quitar):
        clave = (s.steam_hex or "").lower()
        if not clave:
            return
        d = self.datos.relaciones(clave)
        for ident in poner or []:
            otro = self._clave_de_id(ident)
            if otro and otro != clave and otro not in d[campo]:
                d[campo].append(otro)
        for ident in quitar or []:
            otro = self._clave_de_id(ident)
            if otro in d[campo]:
                d[campo].remove(otro)
                if campo == "sigo" and otro in d["bloqueados"]:
                    d["bloqueados"].remove(otro)
        d[campo] = d[campo][-(400 if campo == "no_emparejar" else 100):]  # 400 = matchrejectpid_max
        self.datos.guardar_relaciones(clave, d)

    @staticmethod
    def _lista_ids(v):
        return v if isinstance(v, list) else ([] if v is None else [v])

    async def c_cmd_follow_user(self, s, m):
        self._cambiar_relacion(s, "sigo", self._lista_ids(m.get("follow_user_id_list")),
                               self._lista_ids(m.get("unfollow_user_id_list")))
        return {}

    async def c_cmd_lock_follow_user(self, s, m):
        clave = (s.steam_hex or "").lower()
        otro = self._clave_de_id(m.get("target_pid"))
        if clave and otro:
            d = self.datos.relaciones(clave)
            if self._texto(m.get("is_lock")) in ("YES", "1", "True"):
                if otro not in d["bloqueados"]:
                    d["bloqueados"].append(otro)
            elif otro in d["bloqueados"]:
                d["bloqueados"].remove(otro)
            self.datos.guardar_relaciones(clave, d)
        return {}

    async def c_cmd_get_follow_list(self, s, m):
        d = self.datos.relaciones(s.steam_hex)
        mi_clan = self._clan_de(s.steam_hex)[0]
        filas = []
        for otro in d["sigo"]:
            u = self._usuario_clave(otro).valor
            filas.append(Empaquetado({**u, "is_locked": "YES" if otro in d["bloqueados"] else "NO",
                                      "is_clan_member": "YES" if mi_clan and self._clan_de(otro)[0] == mi_clan
                                      else "NO"}))
        return self._lista_que_cabe("CMD_GET_FOLLOW_LIST", filas, lambda f: {
            "follow_list": f, "follow_list_num": len(f)})

    async def c_cmd_set_coop_reject(self, s, m):
        self._cambiar_relacion(s, "rechazados", self._lista_ids(m.get("set_reject_pid_list")),
                               self._lista_ids(m.get("unset_reject_pid_list")))
        return {}

    # ---------- Salas cooperativas (D.8, L): en memoria, se borran al quedar vacías ----------
    # Sala = {"room_id", "basic" (dict), "match" (dict), "miembros": [Sesion], "listos": set,
    # "compe_id"}. Los ajustes se devuelven tal como los mandó el PES (así los tipos son los suyos).
    # El partido en sí (relé entre PCs) aún no: la sala sirve para crearla, verla y entrar.
    # myclub.coop_salas = "NO" vuelve a la lista vacía de antes.
    MAX_MIEMBROS_COOP = 6

    def _salas_coop_activas(self):
        return (self.cfg.get("myclub") or {}).get("coop_salas", "YES") != "NO"

    def _sala_coop_de(self, s):
        for sala in self.salas_coop.values():
            if s in sala["miembros"]:
                return sala
        return None

    def _salir_de_sala_coop(self, s):
        sala = self._sala_coop_de(s)
        if not sala:
            return
        sala["miembros"].remove(s)
        sala["listos"].discard(s)
        sala.get("vigilantes", {}).pop(s, None)
        if not sala["miembros"]:
            self.salas_coop.pop(sala["room_id"], None)
        else:
            self._avisar_sala_coop(sala)

    # L (11-nov, comparando con la forma que usa PESBUL; código propio): basic_settings se ARMA en el
    # servidor con todas sus claves (devolver lo que mandó el PES pinta el dueño «NotImplement»);
    # en room_list match_settings es LISTA si no hay ajustes y bin {user_team_id, enemy_team_id,
    # cpu_level(texto)} si los hay; la lista vigilada usa list_type RELOAD.
    def _ajustes_sala_coop(self, sala):
        dueno = sala["miembros"][0] if sala["miembros"] else None
        clave_d = (dueno.steam_hex or "").lower() if dueno else ""
        _, c = self._clan_de(clave_d)
        b = sala["basic"]
        nombre_sala = self._texto(b.get("room_name")) or "Sala Phoenix"
        # Prueba 11-nov: el PES manda el nombre por defecto «Salón de juego de» SIN el dueño (lo espera
        # completado por el servidor); si termina en « de» se le añade el nombre del dueño.
        nombre_dueno = str((self.datos.jugador(clave_d) or {}).get("pname") or "") if clave_d else ""
        if nombre_dueno and nombre_sala.rstrip().lower().endswith(" de"):
            nombre_sala = nombre_sala.rstrip() + " " + nombre_dueno
        return Empaquetado({
            "room_id": sala["room_id"],
            "room_name": nombre_sala,
            "clan_name": self._texto(b.get("clan_name")) or (c or {}).get("nombre") or "",
            "clan_owner_info": self._usuario_plano(clave_d),
            "is_follow": "NO",
            "is_com_match": self._texto(b.get("is_com_match")) or "NO",
            "is_use_chat": "YES", "is_same_level": "NO", "team_level": 0, "coop_level": 0,
            "is_same_manner": "NO", "coop_manner": 0, "room_entry_restriction": "NONE",
            "password": "", "lang": ""})

    def _ajustes_partido_coop(self, sala, vacio):
        ms = sala.get("match") or {}
        if not ms:
            return vacio
        return Empaquetado({"user_team_id": self._id_entero(ms.get("user_team_id")) or 0,
                            "enemy_team_id": self._id_entero(ms.get("enemy_team_id")) or 0,
                            "cpu_level": self._texto(ms.get("cpu_level"))})

    def _fila_sala_coop(self, sala):
        return Empaquetado({"basic_settings": self._ajustes_sala_coop(sala),
                            "match_settings": self._ajustes_partido_coop(sala, []),
                            "user_num": len(sala["miembros"]), "compe_id": sala.get("compe_id") or 0,
                            "is_game": "NO"})

    async def c_cmd_get_coop_roomlist(self, s, m):
        if not self._salas_coop_activas():
            return {"room_list": [], "room_list_num": 0, "all_room_num": 0}
        b = self._mapa(m.get("search_options"))
        salas = [x for x in self.salas_coop.values() if x["miembros"]]
        if self._id_entero(b.get("room_id")):
            salas = [x for x in salas if x["room_id"] == self._id_entero(b.get("room_id"))]
        inicio = m.get("start_index") if isinstance(m.get("start_index"), int) else 0
        filas = [self._fila_sala_coop(x) for x in salas[max(0, inicio):max(0, inicio) + 20]]
        return self._lista_que_cabe("CMD_GET_COOP_ROOMLIST", filas, lambda f: {
            "room_list": f, "room_list_num": len(f), "all_room_num": len(salas)})

    async def c_cmd_createjoin_coop_room(self, s, m):
        if not self._salas_coop_activas():
            return {}
        self._salir_de_sala_coop(s)
        basic = {k: v for k, v in self._mapa(m.get("basic_settings")).items() if k != "room_id"}
        sala = {"room_id": next(self._ids_coop), "basic": basic,
                "match": self._mapa(m.get("match_settings")), "miembros": [s], "listos": set(),
                "compe_id": self._id_entero(m.get("compe_id")) or 0}
        self.salas_coop[sala["room_id"]] = sala
        log.info("sala cooperativa %s creada por %s", sala["room_id"], s.pname)
        return {"room_id": sala["room_id"]}

    async def c_cmd_join_coop_room(self, s, m):
        sala = self.salas_coop.get(self._id_entero(m.get("room_id")))
        if not sala or len(sala["miembros"]) >= self.MAX_MIEMBROS_COOP:
            return {}
        clave = self._texto(sala["basic"].get("password"))
        if clave and clave != self._texto(m.get("password")):
            return {}
        if s not in sala["miembros"]:
            self._salir_de_sala_coop(s)
            sala["miembros"].append(s)
            self._avisar_sala_coop(sala, menos=s)
        return {}

    async def c_cmd_leave_coop_room(self, s, m):
        self._salir_de_sala_coop(s)
        return {}

    async def c_cmd_set_coop_match_ready(self, s, m):
        sala = self._sala_coop_de(s)
        if sala:
            (sala["listos"].add if self._texto(m.get("is_ready")) in ("YES", "1", "True") else
             sala["listos"].discard)(s)
            self._avisar_sala_coop(sala, menos=s)
        return {}

    async def c_cmd_set_coop_room_option(self, s, m):
        sala = self._sala_coop_de(s)
        if sala and sala["miembros"][0] is s:
            if m.get("basic_settings") is not None:
                sala["basic"] = {k: v for k, v in self._mapa(m["basic_settings"]).items() if k != "room_id"}
            if m.get("match_settings") is not None:
                sala["match"] = self._mapa(m["match_settings"])
        return {}

    async def c_cmd_get_coop_room_option(self, s, m):
        sala = self.salas_coop.get(self._id_entero(m.get("room_id"))) or self._sala_coop_de(s)
        if not sala:
            return {"result": "ERR_ROOMNOTFOUND"}
        return {"room_id": sala["room_id"], "basic_settings": self._ajustes_sala_coop(sala),
                "match_settings": self._ajustes_partido_coop(sala, Empaquetado({})),
                "is_game": "NO", "room_phase": "NotImplement", "is_start_count_down": "NO",
                "countdown_end_time": 0, "is_stop_countdown": "NO", "is_start_matching": "NO"}

    def _miembros_sala_coop(self, sala, para):
        filas = []
        sigo = self.datos.relaciones(para.steam_hex)["sigo"] if para else []
        for i, x in enumerate((sala or {}).get("miembros") or []):
            clave = (x.steam_hex or "").lower()
            filas.append(Empaquetado({
                "player_index": i, "user_info": self._usuario_plano(clave),
                "is_owner": "YES" if i == 0 else "NO", "coop_level": 1,
                "is_clan_member": "YES" if self._clan_de(clave)[0] else "NO",
                "is_follow": "YES" if clave in sigo else "NO",
                "is_guest": "NO", "guest_info": Empaquetado({}), "is_platform_guest": "NO",
                "is_ready": "YES" if x in sala["listos"] else "NO"}))
        return filas

    async def c_cmd_get_coop_room_member(self, s, m):
        sala = self.salas_coop.get(self._id_entero(m.get("room_id"))) or self._sala_coop_de(s)
        filas = self._miembros_sala_coop(sala, s)
        return {"list_data": filas, "list_num": len(filas)}

    def _lista_vigilada_coop(self, sala, para):
        filas = self._miembros_sala_coop(sala, para)
        return {"room_id": sala["room_id"] if sala else 0,
                "list_type": str((self.cfg.get("myclub") or {}).get("coop_lista_tipo") or "RELOAD"),
                "list_num": len(filas), "list_key_name": "player_index", "list_data": filas}

    async def c_cmd_reqwatchl_coop_room_member(self, s, m):
        """L (Ghidra FUN_141e29680 + prueba 11-nov): tras crear la sala el PES manda
        REQWATCHL_COOP_ROOM_MEMBER{room_id} y espera la lista vigilada (list_type RELOAD, list_num,
        list_key_name, list_data). Es una SUSCRIPCIÓN: el rqid se guarda y cada cambio de la sala
        (entra, sale, listo) se vuelve a mandar con ese mismo rqid."""
        sala = self.salas_coop.get(self._id_entero(m.get("room_id"))) or self._sala_coop_de(s)
        if not sala:
            return {"result": "ERR_ROOMNOTFOUND"}
        sala.setdefault("vigilantes", {})[s] = m.get("rqid", 0)
        return self._lista_vigilada_coop(sala, s)

    def _avisar_sala_coop(self, sala, menos=None):
        """Reenvía la lista de miembros a cada PES suscrito (con SU rqid de la suscripción)."""
        for x, rqid in list((sala or {}).get("vigilantes", {}).items()):
            if x is menos or x not in sala["miembros"]:
                continue
            try:
                x.enviar({"msgid": "CMD_REQWATCHL_COOP_ROOM_MEMBER", "rqid": rqid, "result": "NOERR",
                          **self._lista_vigilada_coop(sala, x)})
            except Exception:
                log.exception("no pude avisar a la sala coop %s", sala.get("room_id"))

    async def c_cmd_get_coop_reject_list(self, s, m):
        filas = [self._usuario_plano(x) for x in self.datos.relaciones(s.steam_hex)["rechazados"]]
        return self._lista_que_cabe("CMD_GET_COOP_REJECT_LIST", filas, lambda f: {
            "reject_list": f, "reject_list_num": len(f)})

    # ---------- Clanes (D.8, L): guardados en la tabla clanes (uno por dueño) ----------
    # Un clan = {"nombre", "comentario", "abierto" (YES/NO), "miembros": [claves], "creado"}.
    # Entrar es directo (sin solicitudes): las listas de solicitudes van vacías hasta ver en el PES
    # cómo pinta «status» (token P). myclub.clanes = "NO" vuelve a las pantallas vacías de antes.
    @staticmethod
    def _texto(v):
        if isinstance(v, (bytes, bytearray)):
            return bytes(v).decode("utf-8", "replace")
        return "" if v is None else str(v)

    @staticmethod
    def _mapa(v):
        if isinstance(v, (bytes, bytearray)) and v:
            try:
                v = msgpack_min.unpackb(bytes(v))
            except Exception:
                return {}
        if isinstance(v, Empaquetado):
            v = v.valor
        return v if isinstance(v, dict) else {}

    def _clanes_activos(self):
        return (self.cfg.get("myclub") or {}).get("clanes", "YES") != "NO"

    def _clan_de(self, clave):
        """(dueño, clan) del clan en que está la clave (como dueño o miembro), o (None, None)."""
        clave = (clave or "").lower()
        if not clave or not self._clanes_activos():
            return None, None
        propio = self.datos.clan(clave)
        if propio:
            return clave, propio
        for dueno, c in self.datos.clanes():
            if clave in (c.get("miembros") or []):
                return dueno, c
        return None, None

    def _es_id_de(self, n, clave):
        """¿El número n es esa clave? (cuenta completa, sus 32 bits bajos o el pid del servidor)."""
        cuenta = self.numero_de_clave(clave)
        return bool(n) and n in (cuenta, cuenta & 0xFFFFFFFF, (self.datos.jugador(clave) or {}).get("pid"))

    def _clan_por_id(self, ident):
        """Clan cuyo dueño tiene ese número de cuenta (el que ven los PES) o ese pid del servidor."""
        n = self._id_entero(ident)
        if not n:
            return None, None
        for dueno, c in self.datos.clanes():
            if self._es_id_de(n, dueno):
                return dueno, c
        return None, None

    def _usuario_clave(self, clave, ampliado=False):
        """L (D.8): bin de usuario (user_id + pname) de cualquier jugador guardado, conectado o no."""
        j = self.datos.jugador(clave) or {}
        nombre = j.get("pname") or ""
        cuenta = self.numero_de_clave(clave)
        # L (Ghidra + volcado 11-nov): user_id es un BIN {pid: int32}, no un entero; con el entero el
        # lector de GET_CLAN_SETTINGS (FUN_141deb550) copiaba con un puntero -1 y el PES se cerraba
        # (msvcrt+0x7b212, pila 0x141debda8). pid = los 32 bits bajos de la cuenta.
        # L (pedido GET_CLAN_INFO.owner_id del propio PES): user_id = bin{pid, pname{...}}; el nombre
        # va DENTRO de user_id (si va al lado, «Dueño» sale «NotImplement»)
        u = {"user_id": Empaquetado({
            "pid": cuenta & 0xFFFFFFFF,
            "pname": Empaquetado({"name": nombre, "xuid_one": str(cuenta),
                                  "psn_user_id": Empaquetado({"account_id": cuenta, "online_id": nombre,
                                                              "is_valid_cache": "YES"}),
                                  "is_deleted": "NO"})})}
        if ampliado:
            # P: en listas de miembros el PES lee además coop_level, is_follow, last_login,
            # current_online_mode, is_join_online_mode; tipos aún sin ver: van solo los seguros
            # L (Ghidra 12-nov, FUN_141de2b50): last_login = ENTERO (epoch; sin él «Último acceso» sale 31/12/1969),
            # is_follow / is_join_online_mode = texto. Probado en el PES con clan_miembro_extra.
            # «Miembros en línea» (probado en el PES 12-nov): con is_join_online_mode "YES" muestra el texto
            # del enum current_online_mode («MYCLUB» -> «myClub»); sin él «No String». Conectado = tiene sesión.
            en_linea = any((x.steam_hex or "").lower() == (clave or "").lower() for x in self.sesiones)
            u.update({"coop_level": 1, "is_follow": "NO", "last_login": int(time.time()),
                      "is_join_online_mode": "YES" if en_linea else "NO"})
            if en_linea:
                u["current_online_mode"] = "MYCLUB"
            u.update((self.cfg.get("myclub") or {}).get("clan_miembro_extra") or {})
        return Empaquetado(u)

    def _usuario_plano(self, clave):
        """L (Ghidra + prueba 11-nov): en user_info (miembros de sala), reject_list y clan_owner_info el
        bloque de usuario es bin{pid, pname{...}} DIRECTO, sin la clave user_id (que solo existe en
        owner_info/clan_members/follow_list). Con la forma equivocada la sala se quedaba en blanco."""
        return self._usuario_clave(clave).valor["user_id"]

    def _ajustes_clan(self, dueno, c):
        return Empaquetado({"clan_name": c.get("nombre") or "", "owner_info": self._usuario_clave(dueno),
                            "enable_request": c.get("abierto") or "YES",
                            "clan_comment": c.get("comentario") or "", "is_banned": "NO"})

    @staticmethod
    def _estadisticas_clan():
        cero = {"match_count": 0, "win_count": 0, "draw_count": 0, "lose_count": 0,
                "win_s": 0, "win_s_max": 0, "lost_goal": 0}
        return Empaquetado({"current_division": "", "current_rating": 0,
                            "season_match_stats": Empaquetado(dict(cero)),
                            "total_match_stats": Empaquetado(dict(cero)),
                            "season_win_count": 0, "residual_div_first_count": 0})

    def _info_clan(self, dueno, c):
        miembros = [self._usuario_clave(x, ampliado=True) for x in (c.get("miembros") or [dueno])]
        return Empaquetado({"basic_settings": self._ajustes_clan(dueno, c), "clan_members": miembros,
                            "clan_member_num": len(miembros), "clan_stats_online": self._estadisticas_clan(),
                            "clan_stats_myclub": self._estadisticas_clan()})

    MAX_MIEMBROS_CLAN = 30

    async def c_cmd_create_clan(self, s, m):
        if not self._clanes_activos() or not s.steam_hex:
            return {}
        nombre = self._texto(m.get("clan_name")).strip()[:32] or (s.pname or "Clan Phoenix")
        clave = s.steam_hex.lower()
        dueno, c = self._clan_de(clave)
        if dueno == clave:  # ya es dueño: se renombra
            self.datos.guardar_clan(clave, {**c, "nombre": nombre})
            return {}
        if dueno:  # estaba en otro clan: lo deja para fundar el suyo
            self._sacar_de_clan(dueno, c, clave)
        self.datos.guardar_clan(clave, {"nombre": nombre, "comentario": "", "abierto": "YES",
                                        "miembros": [clave], "creado": int(time.time())})
        log.info("clan creado: %s (dueño %s)", nombre, clave)
        return {}

    def _sacar_de_clan(self, dueno, c, clave):
        if clave == dueno:
            self.datos.disolver_clan(dueno)
            log.info("clan disuelto: %s", c.get("nombre"))
            return
        c["miembros"] = [x for x in (c.get("miembros") or []) if x != clave]
        self.datos.guardar_clan(dueno, c)

    async def c_cmd_get_clan_settings(self, s, m):
        dueno, c = self._clan_de(s.steam_hex)
        if not dueno:
            return {"clan_basic_settings": Empaquetado({"clan_name": "", "owner_info": self._usuario_clave(s.steam_hex or ""),
                                                        "enable_request": "YES", "clan_comment": "",
                                                        "is_banned": "NO"})}
        return {"clan_basic_settings": self._ajustes_clan(dueno, c)}

    async def c_cmd_set_clan_settings(self, s, m):
        clave = (s.steam_hex or "").lower()
        c = self.datos.clan(clave) if self._clanes_activos() else None
        if not c:
            return {}  # solo el dueño cambia los ajustes
        a = self._mapa(m.get("clan_basic_settings"))
        if a.get("clan_name"):
            c["nombre"] = self._texto(a["clan_name"]).strip()[:32] or c["nombre"]
        if "clan_comment" in a:
            c["comentario"] = self._texto(a["clan_comment"])[:200]
        if a.get("enable_request") in ("YES", "NO", b"YES", b"NO"):
            c["abierto"] = self._texto(a["enable_request"])
        self.datos.guardar_clan(clave, c)
        return {}

    async def c_cmd_get_clan_info(self, s, m):
        dueno, c = self._clan_por_id(m.get("owner_id"))
        if not dueno:
            dueno, c = self._clan_de(s.steam_hex)
        if not dueno:
            # P (D.8): sin clan, clan_info vacío en vez de un ERR que no conocemos
            return {"clan_info": Empaquetado({"basic_settings": Empaquetado({
                "clan_name": "", "owner_info": self._usuario_clave(s.steam_hex or ""), "enable_request": "YES",
                "clan_comment": "", "is_banned": "NO"}), "clan_members": [], "clan_member_num": 0,
                "clan_stats_online": self._estadisticas_clan(), "clan_stats_myclub": self._estadisticas_clan()})}
        return {"clan_info": self._info_clan(dueno, c)}

    def _lista_que_cabe(self, msgid, filas, armar):
        while filas and _cifrado_comprimido({"msgid": msgid, "rqid": 0, "result": "NOERR",
                                             **armar(filas)}) > MAX_CIFRADO_CLIENTE:
            filas = filas[:len(filas) * 3 // 4]
        return armar(filas)

    async def c_cmd_get_clan_simple_info_list(self, s, m):
        if "owner_pid_list" in m:
            filas = []
            for ident in m.get("owner_pid_list") or []:
                dueno, c = self._clan_por_id(ident)
                if dueno:
                    filas.append(Empaquetado({"clan_name": c.get("nombre") or "",
                                              "owner_info": self._usuario_clave(dueno)}))
            return self._lista_que_cabe("CMD_GET_CLAN_SIMPLE_INFO_LIST", filas, lambda f: {
                "clan_simple_info_list": f, "clan_simple_info_list_num": len(f)})
        return await self.c_cmd_get_clan_list(s, m)

    async def c_cmd_get_clan_list(self, s, m):
        inicio = m.get("start_index") if isinstance(m.get("start_index"), int) else 0
        inicio = max(0, inicio)
        b = self._mapa(m.get("search_options"))
        buscar = self._texto(b.get("clan_name")).strip().lower()
        todos = [(d, c) for d, c in (self.datos.clanes() if self._clanes_activos() else [])
                 if not buscar or buscar in (c.get("nombre") or "").lower()]
        if self._texto(b.get("is_joined_clan")) == "YES":
            # «Tu lista de clanes» (11-nov, prueba en el PES): solo los clanes donde estás, abiertos o no
            clave = (s.steam_hex or "").lower()
            todos = [(d, c) for d, c in todos if d == clave or clave in (c.get("miembros") or [])]
        elif self._texto(b.get("enable_request")) == "YES":
            todos = [(d, c) for d, c in todos if (c.get("abierto") or "YES") == "YES"]
        filas = [self._info_clan(d, c) for d, c in todos[inicio:inicio + 20]]
        return self._lista_que_cabe("CMD_GET_CLAN_LIST", filas, lambda f: {
            "clan_list": f, "clan_list_num": len(f), "all_list_num": len(todos), "start_index": inicio})

    @classmethod
    def _plano(cls, v):
        """Quita los Empaquetado de forma recursiva: el lector de rankings quiere mapas directos."""
        if isinstance(v, Empaquetado):
            v = v.valor
        if isinstance(v, dict):
            return {k: cls._plano(x) for k, x in v.items()}
        if isinstance(v, list):
            return [cls._plano(x) for x in v]
        return v

    def _ranking_clanes(self, m):
        """Lector FUN_141de2b50: clan_ranking = lista de MAPAS directos {rank, rating,
        division_change_type, basic_settings{..., owner_info{user_id{...}, ...}}, match_stats{7 enteros}}."""
        try:
            inicio = max(0, int(m.get("start_index") or 0))
            cuantos = min(100, max(1, int(m.get("view_num") or 50)))
        except (TypeError, ValueError):
            inicio, cuantos = 0, 50
        todos = list(self.datos.clanes()) if self._clanes_activos() else []
        todos.sort(key=lambda dc: (-len(dc[1].get("miembros") or []), (dc[1].get("nombre") or "").lower()))
        lista = []
        for i, (d, c) in enumerate(todos[inicio:inicio + cuantos]):
            lista.append({"rank": inicio + i + 1, "rating": 0, "division_change_type": "NotImplement",
                          "basic_settings": {"clan_name": c.get("nombre") or "",
                                             "owner_info": self._plano(self._usuario_clave(d, ampliado=True)),
                                             "enable_request": c.get("abierto") or "YES",
                                             "clan_comment": c.get("comentario") or "", "is_banned": "NO"},
                          "match_stats": self._plano(self._estadisticas_clan())["total_match_stats"]})
        # «Última actualiz.» salía 31/12 (fecha 0): el lector no tiene campo propio; candidatos como
        # en los otros rankings (sin probar en el PES)
        ahora = int(time.time())
        return {"date": ahora, "update_date": ahora, "clan_ranking": lista, "clan_ranking_num": len(lista),
                "clan_ranking_all_list_num": len(todos)}

    async def c_cmd_get_clan_ranking(self, s, m):
        return self._ranking_clanes(m)

    async def c_cmd_get_clan_ranking_byid(self, s, m):
        r = self._ranking_clanes({"start_index": 0, "view_num": 100})
        dueno, _c = self._clan_de((s.steam_hex or ""))
        nombre = self._plano(self._usuario_clave(dueno))["user_id"]["pid"] if dueno else None
        if nombre is not None:
            r["clan_ranking"] = [x for x in r["clan_ranking"]
                                 if x["basic_settings"]["owner_info"]["user_id"]["pid"] == nombre]
            r["clan_ranking_num"] = len(r["clan_ranking"])
        return r

    async def c_cmd_get_clan_request_list(self, s, m):
        return {"request_info_list": [], "request_info_list_num": 0}

    async def c_cmd_get_clan_pending_list(self, s, m):
        return {"pending_info_list": [], "pending_info_list_num": 0}

    async def c_cmd_entry_clan(self, s, m):
        """Pedir entrar = entrar directo si el clan admite solicitudes y tiene sitio."""
        dueno, c = self._clan_por_id(m.get("target_pid"))
        clave = (s.steam_hex or "").lower()
        if not dueno or not clave:
            return {}
        if (c.get("abierto") or "YES") != "YES" or len(c.get("miembros") or []) >= self.MAX_MIEMBROS_CLAN:
            return {}
        antes, ca = self._clan_de(clave)
        if antes == dueno:
            return {}
        if antes:
            self._sacar_de_clan(antes, ca, clave)
            c = self.datos.clan(dueno) or c
        c["miembros"] = list(c.get("miembros") or [dueno]) + [clave]
        self.datos.guardar_clan(dueno, c)
        return {}

    async def c_cmd_invite_clan(self, s, m):
        # P: la invitación se acepta sola si el invitado no tiene clan (sin bandeja de avisos aún)
        clave = (s.steam_hex or "").lower()
        c = self.datos.clan(clave) if self._clanes_activos() else None
        n = self._id_entero(m.get("target_id"))
        if not c or not n:
            return {}
        for j in self.datos.todos():
            if self._es_id_de(n, j["steam_hex"]):
                destino = j["steam_hex"].lower()
                if not self._clan_de(destino)[0] and len(c.get("miembros") or []) < self.MAX_MIEMBROS_CLAN:
                    c["miembros"] = list(c.get("miembros") or [clave]) + [destino]
                    self.datos.guardar_clan(clave, c)
                break
        return {}

    async def c_cmd_leave_clan(self, s, m):
        clave = (s.steam_hex or "").lower()
        dueno, c = self._clan_de(clave)
        if dueno:
            self._sacar_de_clan(dueno, c, clave)
        return {}

    async def c_cmd_expel_clan_member(self, s, m):
        clave = (s.steam_hex or "").lower()
        c = self.datos.clan(clave) if self._clanes_activos() else None
        n = self._id_entero(m.get("target_pid"))
        if c and n:
            fuera = [x for x in c.get("miembros") or [] if x != clave and self._es_id_de(n, x)]
            if fuera:
                c["miembros"] = [x for x in c["miembros"] if x not in fuera]
                self.datos.guardar_clan(clave, c)
        return {}

    async def c_cmd_get_clan_member_mode(self, s, m):
        dueno, c = self._clan_de(s.steam_hex)
        miembros = [self._usuario_clave(x, ampliado=True) for x in ((c or {}).get("miembros") or [])]
        return self._lista_que_cabe("CMD_GET_CLAN_MEMBER_MODE", miembros, lambda f: {
            "clan_members": f, "clan_member_num": len(f)})

    async def c_cmd_join_matchday_mode(self, s, m):
        return {}

    async def c_cmd_leave_matchday_mode(self, s, m):
        return {}

    def _torneos(self, clave):
        self._recargar_config()
        ahora = int(time.time())
        salida = []
        for e in (self.cfg.get("myclub") or {}).get(clave) or []:
            if isinstance(e, dict) and isinstance(e.get("compe_id"), int):
                salida.append({**e, "inicio": int(e.get("inicio") or ahora - 86400),
                               "fin": int(e.get("fin") or ahora + 30 * 86400)})
        return salida[:self.MAX_EVENTOS]

    @staticmethod
    def _banner():
        return Empaquetado({"filename": "", "hash_str": ""})

    def _equipos_bando(self, e, lado):
        """Ids de equipo de los clubes inscritos en ese bando (entry_matchday_group del JOIN)."""
        ids = []
        for clave in self.datos.claves_de_clubes():
            club = self.datos.club(clave) or {}
            mio = (club.get("matchday_equipo") or {}).get(str(e["compe_id"])) or {}
            if str(mio.get("grupo") or "").upper().endswith("_" + lado.upper()) and mio.get("team_id"):
                ids.append(int(mio["team_id"]))
        return ids

    def _bando_matchday(self, e, lado):
        """D.7: group_info_a/b = {team_list, team_list_num, group_name, group_desc, ...}.
        Con "bandos": "NO" en el torneo vuelve al bin vacío que ya se probó en la lista.
        team_list = equipos inscritos en el bando (prueba 11-nov: con [] el PES daba la fase de
        grupos por terminada y «Empezar partido» salía en gris)."""
        if str(e.get("bandos") or "") == "NO":
            return Empaquetado({})
        nombre = str(e.get(f"bando_{lado}") or ("Bando A" if lado == "a" else "Bando B"))
        # R (pesmatchday AzrakHQ, partido completo en vivo): team_list = equipos PERMITIDOS del bando
        # (con group_type TEAM el buscador de equipos se limita a ellos; nunca vacío). Config por
        # torneo: "equipos_a"/"equipos_b"; por defecto 4 equipos reales del juego base por bando.
        base = {"a": [101, 108, 114, 127], "b": [100, 116, 120, 131]}[lado]
        equipos = [int(x) for x in (e.get(f"equipos_{lado}") or base) if isinstance(x, int)]
        equipos += [x for x in self._equipos_bando(e, lado) if x not in equipos]
        # Texto de la tarjeta del bando (con group_desc "" la tarjeta salía sin texto, matchday2.md)
        desc = str(e.get(f"bando_{lado}_texto") or "Elige un equipo de este bando y suma puntos para tu lado.")
        return Empaquetado({"team_list": equipos, "team_list_num": len(equipos), "group_name": nombre, "group_desc": desc,
                            "is_entry_restricted": "NO", "pre_pickup_users_pid_list": []})

    @staticmethod
    def _fechas_matchday(e):
        """Fases escalonadas (antes todas = fin y el PES pintaba «fase de grupos terminada»):
        grupos hasta fin-2d, elección de jugadores fin-36h, gran final desde fin-1d. Cada fecha
        se puede fijar en el torneo de config con la misma clave."""
        fin, dia = e["fin"], 86400
        base = {"end_group_match_date": fin - 2 * dia, "pickup_member_date": fin - dia - dia // 2,
                "entry_pickup_match_start_date": fin - dia, "watch_match_start_date": fin - dia,
                "start_match_limit_date": fin - 2 * dia,
                # R (matchday2.md AzrakHQ, receta que llegó a partido): estos 3 son duración en
                # minutos, no fechas; 2 días en «minutos» no tenía sentido
                "group_match_time": 10, "pickup_match_time": 10, "watch_wait_time": 0}
        return {k: int(e[k]) if isinstance(e.get(k), int) else v for k, v in base.items()}

    def _campos_matchday(self, s, e):
        return {
            "compe_id": e["compe_id"], "base_id": int(e.get("base_id") or e["compe_id"]),
            "compe_name": str(e.get("compe_name") or f"Matchday {e['compe_id']}"),
            "is_disp_compe_name": "YES",
            "group_info_a": self._bando_matchday(e, "a"), "group_info_b": self._bando_matchday(e, "b"),
            # R (matchday2.md / pesmatchday AzrakHQ, partido completo en vivo 7-nov): plantilla
            # especial (el equipo elegido en el JOIN), grupos por equipo, partido único. Con
            # SQUAD_MYCLUB el PES salía del menú con LEAVE_MATCHDAY (prueba 11-nov 14:52).
            "is_entry_restricted": "NO", "match_type": str(e.get("match_type") or "SINGLE_MATCH"),
            "squad_type": str(e.get("squad_type") or "MATCHDAY_SQUAD_SPECIAL"),
            "group_type": str(e.get("group_type") or "MATCHDAY_GROUP_TYPE_TEAM"),
            "rating_rank": "MATCHDAY_RATING_RANK_NONE",
            "start_date": e["inicio"], "entry_limit_date": self._fechas_matchday(e)["end_group_match_date"],
            "end_group_match_date": self._fechas_matchday(e)["end_group_match_date"],
            "pickup_member_date": self._fechas_matchday(e)["pickup_member_date"], "end_date": e["fin"], "entry_user_num": 0,
            "entry_user_max": int(e.get("cupo") or 1000), "is_end": "NO",
            "banner_file_info": self._banner(), "list_banner_file_info": self._banner(),
            "is_use_emblem": "NO", "is_use_emblem_list": "NO",
            "is_entry": self._inscrito(s, "matchday", e["compe_id"]),
            # R: valores de la entrada que llegó a partido (AzrakHQ): enum real, no ""
            "watch_match_reward_state": "NOT_FINISH_PICKUP_MATCH", "compe_db_id": 0,
            "is_possible_watch": "NO"}

    async def c_cmd_get_matchday_list(self, s, m):
        lista = [Empaquetado(self._campos_matchday(s, e)) for e in self._torneos("matchday")]
        return {"compe_list": lista, "list_num": len(lista), "list_max": len(lista), "is_no_check": "NO",
                "add_limit_rating": 0}

    @staticmethod
    def _valoracion_plantilla():
        return Empaquetado({"sp_squad_rating": 0, "rating_kind_player_bonus_value_list": [],
                            "evaluation_a": 0, "evaluation_b": 0, "superiority_border_ratio": 0})

    async def c_cmd_get_matchday_info(self, s, m):
        """D.5: tras inscribirse el PES pide INFO con compe_id 0 (ver _cid_torneo)."""
        cid = self._cid_torneo(s, m, "matchday")
        e = next((x for x in self._torneos("matchday") if x["compe_id"] == cid), None)
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        mio = ((club or {}).get("matchday_equipo") or {}).get(str(cid)) or {}
        info = {**self._campos_matchday(s, e),
                **{k: v for k, v in self._fechas_matchday(e).items()
                   if k not in ("end_group_match_date", "pickup_member_date")},
                "is_check_eula": "NO", "eula_url": "",
                "ball_id": int(e.get("ball_id") or 0), "use_stadium_id": int(e.get("estadio") or 0),
                "fixed_timezone": "TIME_ZONE_TYPE_NONE", "fixed_weather": "WEATHER_TYPE_NONE",
                "fixed_season": "SEASON_TYPE_NONE", "is_fix_condition": "NO",
                "is_equalization": "NO", "is_pesleague_stadium": "NO", "is_efootball_pro_stadium": "NO",
                "joined_matchday_group": str(mio.get("grupo") or ""), "pickup_select": "NO",
                "team_id": int(mio.get("team_id") or 0), "pickup_cut_off_rank": 0, "team_list_num": 0,
                "all_reward_info": self._info_premios_matchday(e), "achievement_reward_list": [],
                "win_reward_list": [], "lose_reward_list": [],
                "watching_reward_list": [], "pickup_win_reward_list": [],
                "pickup_lose_reward_list": [], "squad_rating_info": self._valoracion_plantilla()}
        # L (Ghidra): los 7 bloques de premios son ARRAYS (lista de bin regalo); como bin suelto el
        # PES se cerraba (0xc0000005). Respaldo: myclub.matchday_info_extra = "minimo" o [claves]
        # manda solo lo que la LISTA ya acepta + is_check_eula (+ esas claves).
        cfgm = self.cfg.get("myclub") or {}
        extra = cfgm.get("matchday_info_extra", "todo")
        if extra != "todo":
            extra = [] if extra == "minimo" else list(extra or [])
            base = set(self._campos_matchday(s, e)) | {"is_check_eula"} | set(extra)
            info = {k: v for k, v in info.items() if k in base}
        # Q (Ghidra de PESBUL público, 11-nov, notes/re matchday2: 0x14105d370 + 0x14105cead): el
        # cliente saca compe_id y end_group_match_date de compe_info.info_overview (mismo mapa que una
        # entrada de la LISTA). Si no están ahí valen 0 y con progress GROUP_MATCH y ahora >= fecha
        # baja a «fase de grupos terminada» (botón gris). Va DENTRO de compe_info, no como clave
        # hermana. myclub.matchday_overview_dentro = "NO" vuelve a la forma anterior.
        campos = self._campos_matchday(s, e)
        if cfgm.get("matchday_overview_dentro") != "NO":
            solo_info = {k: v for k, v in info.items() if k not in campos or k == "compe_db_id"}
            solo_info["info_overview"] = Empaquetado(dict(campos))
            return {"compe_info": Empaquetado(solo_info)}
        salida = {"compe_info": Empaquetado(info)}
        # P (Ghidra): forma anterior, mapa como clave hermana (myclub.matchday_info_overview "NO" lo quita).
        if cfgm.get("matchday_info_overview") != "NO":
            salida["info_overview"] = Empaquetado(dict(info))
        return salida

    async def c_cmd_get_matchday_mainmenu_info(self, s, m):
        """D.7: con matchday_progress "" (no es token válido) la pantalla salía con textos vacíos.
        Fase por config myclub.matchday_progreso (defecto MATCHDAY_PROGRESS_GROUP_MATCH)."""
        cid = self._cid_torneo(s, m, "matchday")
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        mio = ((club or {}).get("matchday_equipo") or {}).get(str(cid)) or {}
        puntos = (club or {}).get("matchday_puntos") or {}
        grupo = str(mio.get("grupo") or "MATCHDAY_GROUP_NONE")
        cfgm = self.cfg.get("myclub") or {}
        # L (Ghidra 11-nov, FUN_140db6d60 + FUN_14105b690): con el estado GROUP_MATCH el PES solo da
        # la fase por jugable si el pid del usuario está en pickup_users_group_a/b de su bando (o un
        # indicador de sesión); con las listas vacías salía «Finalizó la fase de partidos de grupo».
        # myclub.matchday_pickup_yo = "NO" vuelve a las listas vacías.
        lista_a, lista_b = [], []
        if s.steam_hex and str(cfgm.get("matchday_pickup_yo", "NO")) != "NO":
            yo = self._usuario_plano((s.steam_hex or "").lower())
            (lista_b if grupo.upper().endswith("_B") else lista_a).append(yo)
        # Clasificación real (antes 0 de 0): mi puesto en mi bando y la suma de puntos de cada bando
        todas = self._filas_matchday(cid) if cid else []
        mi_bando = [f for f in todas if f[4] == str(mio.get("grupo") or "")] if mio else []
        mi_puesto = next((i for i, f in enumerate(mi_bando, 1) if f[2] == (s.steam_hex or "").lower()
                          or f[2] == s.steam_hex), 0)
        total_a = sum(f[0] for f in todas if f[4].upper().endswith("_A"))
        total_b = sum(f[0] for f in todas if f[4].upper().endswith("_B"))
        return {"mainmenu_info": Empaquetado({
            "matchday_progress": str(cfgm.get("matchday_progreso") or "MATCHDAY_PROGRESS_GROUP_MATCH"),
            "group_match_info": Empaquetado({
                "matchday_point": int(puntos.get(str(cid)) or 0), "matchday_rank": mi_puesto,
                "matchday_ranking_user_num": len(mi_bando), "matchday_ratio": 0,
                "total_matchday_point_group_a": total_a, "total_matchday_point_group_b": total_b,
                "pickup_match_score_group_a": 0, "pickup_match_score_group_b": 0,
                "is_hide_graph": "NO"}),  # R: va DENTRO de group_match_info (matchday2.md)
            "pickup_user_info": Empaquetado({"pickup_users_group_a": lista_a, "pickup_users_group_b": lista_b,
                                             "pickup_user_group_a_num": len(lista_a),
                                             "pickup_user_group_b_num": len(lista_b),
                                             "is_failed_pickup_users": "NO"}),
            "is_final_reward": "NO", "is_get_final_reward": "NO", "is_watch_match": "NO",
            "final_reward_list": [], "pickup_reward_list": [],
            "group_result": grupo, "pickup_reward_type": "MATCHDAY_PICKUP_REWARD_NONE"})}

    # Inscripción propia en torneos de Matchday / PES League (respuesta vacía = NOERR; las claves
    # de respuesta reales se completan cuando se lean en Ghidra).
    def _cid_torneo(self, s, m, clave):
        """compe_id del pedido. El PES de Matchday lo pierde y manda 0 tras inscribirse (probado
        11-nov): entonces vale el último torneo de esta sesión, el inscrito o el único que haya."""
        cid = self._id_entero(m.get("compe_id"))
        if cid:
            self._recordar_torneo(s, clave, cid)
            return cid
        torneos = self._torneos(clave)
        cid = (getattr(s, "ultimo_torneo", None) or {}).get(clave)
        if not cid and s.steam_hex:
            club = self.datos.club(s.steam_hex) or {}
            inscritos = (club.get("torneos_inscripcion") or {}).get(clave) or []
            cid = inscritos[-1] if inscritos else 0
        if not cid and len(torneos) == 1:
            cid = torneos[0]["compe_id"]
        return cid or 0

    @staticmethod
    def _recordar_torneo(s, clave, cid):
        try:
            s.ultimo_torneo = {**(getattr(s, "ultimo_torneo", None) or {}), clave: cid}
        except AttributeError:
            pass

    def _inscribir_torneo(self, s, m, clave):
        cid = self._cid_torneo(s, m, clave)
        e = next((x for x in self._torneos(clave) if x["compe_id"] == cid), None)
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        ahora = int(time.time())
        if ahora > e["fin"]:
            return {"result": "ERR_COMPE_AFTER_ENTRYTIME"}
        club = self._club_propio(s)
        if club is None:
            return {"result": "ERR_NOTREGISTERED"}
        inscritos = club.setdefault("torneos_inscripcion", {}).setdefault(clave, [])
        if cid not in inscritos:
            inscritos.append(cid)
            self.datos.guardar_club(s.steam_hex, club)
            log.info("%s: %s se inscribe en el torneo %d", clave, s.steam_hex, cid)
        return {}

    def _salir_torneo(self, s, m, clave):
        cid = self._id_entero(m.get("compe_id"))
        club = self._club_propio(s)
        if club is None:
            return {}
        inscritos = (club.get("torneos_inscripcion") or {}).get(clave) or []
        if cid in inscritos:
            inscritos.remove(cid)
            self.datos.guardar_club(s.steam_hex, club)
        return {}

    def _inscrito(self, s, clave, cid):
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        return "YES" if club and cid in ((club.get("torneos_inscripcion") or {}).get(clave) or []) else "NO"

    async def c_cmd_join_matchday(self, s, m):
        r = self._inscribir_torneo(s, m, "matchday")
        if r:
            return r
        club = self._club_propio(s)
        cid = self._cid_torneo(s, m, "matchday")
        mio = {"grupo": str(m.get("entry_matchday_group") or ""),
               "team_id": self._id_entero(m.get("team_id")) or 0}
        # L (Ghidra FUN_141bc4bc0): GET_MATCHDAY_SP_GAMEPLAN devuelve gameplan_data(+_size) y
        # sp_member_list, que el PES manda aquí al inscribirse (sp_gameplan_data / sp_member_list)
        if isinstance(m.get("sp_gameplan_data"), str) and m["sp_gameplan_data"]:
            mio["sp_gameplan_data"] = m["sp_gameplan_data"]
        if isinstance(m.get("sp_member_list"), list):
            mio["sp_member_list"] = [int(x) for x in m["sp_member_list"] if isinstance(x, int)]
        club.setdefault("matchday_equipo", {})[str(cid)] = mio
        self.datos.guardar_club(s.steam_hex, club)
        return {"squad_rating_info": self._valoracion_plantilla(), "compe_id": cid}

    async def c_cmd_get_matchday_sp_gameplan(self, s, m):
        """Estrategia especial de Matchday: sin manejador la pantalla principal dejaba «Empezar
        partido» en gris. Devuelve lo guardado al inscribirse (o el plan por defecto y lista vacía)."""
        cid = self._cid_torneo(s, m, "matchday")
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        mio = ((club or {}).get("matchday_equipo") or {}).get(str(cid)) or {}
        tam, texto = self._plan_texto(mio.get("sp_gameplan_data"))
        return {"gameplan_data_size": tam, "gameplan_data": texto,
                "sp_member_list": list(mio.get("sp_member_list") or [])}

    async def c_cmd_set_matchday_sp_gameplan(self, s, m):
        cid = self._cid_torneo(s, m, "matchday")
        club = self._club_propio(s)
        if club is None:
            return {}
        mio = club.setdefault("matchday_equipo", {}).setdefault(str(cid), {})
        for k_in, k_out in (("gameplan_data", "sp_gameplan_data"), ("sp_gameplan_data", "sp_gameplan_data")):
            if isinstance(m.get(k_in), str) and m[k_in]:
                mio[k_out] = m[k_in]
        if isinstance(m.get("sp_member_list"), list):
            mio["sp_member_list"] = [int(x) for x in m["sp_member_list"] if isinstance(x, int)]
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_leave_matchday(self, s, m):
        return self._salir_torneo(s, m, "matchday")

    async def c_cmd_join_pesleague_compe(self, s, m):
        r = self._inscribir_torneo(s, m, "pesleague")
        if r:
            return r
        club = self._club_propio(s)
        club.setdefault("pesleague_equipo", {})[str(self._cid_torneo(s, m, "pesleague"))] = \
            self._id_entero(m.get("team_id")) or 0
        self.datos.guardar_club(s.steam_hex, club)
        return {}

    async def c_cmd_leave_pesleague_compe(self, s, m):
        return self._salir_torneo(s, m, "pesleague")

    def _filas_ranking_pesleague(self, cid, yo):
        """R (notes/re tourney.md de AzrakHQ, MIT; forma de cada fila = parser 0x141c1b2e0): los inscritos
        del torneo, por puntos (los guarda club['pesleague_puntos'][cid]; sin partidos jugados = 0)."""
        filas = []
        for j in self.datos.todos():
            club = self.datos.club(j["steam_hex"]) or {}
            if cid not in ((club.get("torneos_inscripcion") or {}).get("pesleague") or []):
                continue
            pts = int(((club.get("pesleague_puntos") or {}).get(str(cid))) or 0)
            filas.append((pts, j))
        filas.sort(key=lambda x: (-x[0], x[1]["pid"]))
        salida = []
        for n, (pts, j) in enumerate(filas, 1):
            u = self._usuario_plano(j["steam_hex"].lower())
            club = self.datos.club(j["steam_hex"]) or {}
            salida.append((n, Empaquetado({
                "pid": u.valor["pid"], "entry_name": j["pname"] or "",
                "team_id": int((club.get("pesleague_equipo") or {}).get(str(cid)) or 0),
                "clan_owner_name": u.valor["pname"], "rank": n,
                **self._stats_pesleague(club, cid), "recode_date": int(time.time()),
                "continuous_win": 0, "is_get_participation_right": "NO",
                "is_mydata": "YES" if j["steam_hex"].lower() == (yo or "").lower() else "NO",
                "rank_type": "NONE"}), j["steam_hex"].lower()))
        return salida

    async def c_cmd_get_pesleague_compe_ranking(self, s, m):
        cid = self._cid_torneo(s, m, "pesleague")
        todas = self._filas_ranking_pesleague(cid, s.steam_hex)
        ini = max(0, int(self._id_entero(m.get("start_index")) or 0))
        n = max(1, min(100, int(self._id_entero(m.get("view_num")) or 100)))
        pagina = [f for _, f, _ in todas[ini:ini + n]]
        return {"rank_list": pagina, "list_num": len(pagina), "list_max": len(todas),
                "update_date": int(time.time())}

    async def c_cmd_get_pesleague_compe_ranking_byid(self, s, m):
        cid = self._cid_torneo(s, m, "pesleague")
        todas = self._filas_ranking_pesleague(cid, s.steam_hex)
        mias = [f for _, f, k in todas if k == (s.steam_hex or "").lower()]
        return {"rank_list": mias, "list_num": len(mias), "list_max": len(todas),
                "update_date": int(time.time())}

    async def c_cmd_get_online_ad_file(self, s, m):
        """R (menus.md AzrakHQ, parser 0x141dde9c0): carteles del estadio para Matchday, PES League,
        eventos y partido. Nombres vacíos = no descarga nada. Sin respuesta el botón «Participa»
        de Matchday se quedaba en la misma pantalla (prueba 11-nov 14:45)."""
        return {"ad_banner_file_name": "", "ad_signage_file_name": ""}

    async def c_cmd_get_pesleague_compe_reward_list(self, s, m):
        return {"reward_list": [], "reward_list_num": 0}

    async def c_cmd_get_pesleague_compe_reward_info(self, s, m):
        return {"reward_info": Empaquetado({"reward_list": [], "reward_list_num": 0})}

    # Clanes: aquí «pedir entrar» ya entra directo, así que no hay solicitudes pendientes que
    # aceptar, rechazar o cancelar (los pedidos del juego se contestan sin error).
    async def c_cmd_accept_clan_entry(self, s, m):
        return {}

    async def c_cmd_cancel_clan_entry(self, s, m):
        return {}

    async def c_cmd_confirm_clan_request(self, s, m):
        return {}

    async def c_cmd_reject_clan_request(self, s, m):
        return {}

    async def c_cmd_accept_clan_invite(self, s, m):
        return {}

    async def c_cmd_cancel_clan_invite(self, s, m):
        return {}

    async def c_cmd_join_pesleague_compe_mode(self, s, m):
        return {}

    async def c_cmd_leave_pesleague_compe_mode(self, s, m):
        return {}

    async def c_cmd_get_pesleague_mainmenu_info(self, s, m):
        # Con division "" el PES dice «inscripción cerrada»: se manda la división real (D12..D1)
        division = ""
        if s.steam_hex and (self.cfg.get("myclub") or {}).get("pesleague_division", "YES") != "NO":
            division = str(self._estado_division(s.steam_hex).get("division") or DIVISION_INICIAL)
        return {"division": division, "new_result": "NO"}

    async def c_cmd_get_pesleague_1vs1_mainmenu_info(self, s, m):
        # D.6: sin fechas el cliente da el torneo por terminado («Se calcularon los resultados finales»)
        e = self._torneo_pesleague(m) or next(iter(self._torneos("pesleague")), None)
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        # Matchmaking siempre abierto (ventana = intervalo) salvo que el torneo diga otra cosa;
        # equipo y nombre del club inscrito (con "" salía «Nom. del equipo» vacío)
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        equipo = int(((club or {}).get("pesleague_equipo") or {}).get(str(e["compe_id"])) or 0)
        # Prueba 11-nov (PES base): con intervalo = ventana = 86400 salía «el período abre en 59:42» y
        # sin botón; con 3600/3600 sale «Selecciona Empezar partido» y «el período cierra en 0:59:41».
        intervalo = int(e.get("matching_interval") or 3600)
        return {"matching_first_date": int(e["inicio"]), "matching_time": int(e.get("matching_time") or intervalo),
                "matching_interval": intervalo, "compe_end_time": int(e["fin"]), "team_id": equipo,
                "team_name": str((club or {}).get("team_name") or ""),
                **{k: v for k, v in self._stats_pesleague(club, e["compe_id"]).items()
                   if k in ("point", "win_count", "lose_count", "draw_count", "match_num")},
                # R (pestourney.py, en vivo 08-nov): «Mejor resultado de N partidos seguidos» = jugados (mín. 1)
                "regulation_match_num": max(1, self._stats_pesleague(club, e["compe_id"])["match_num"]),
                "rank_type": "NONE"}

    async def c_cmd_get_pesleague_compe_team_list(self, s, m):
        return {"team_list": [], "team_list_num": 0}

    async def c_cmd_get_pesleague_compe_list(self, s, m):
        lista = []
        for e in self._torneos("pesleague"):
            lista.append(Empaquetado({
                "compe_id": e["compe_id"], "compe_name": str(e.get("compe_name") or f"Torneo {e['compe_id']}"),
                "start_date": e["inicio"], "end_date": e["fin"], "entry_limit_date": e["fin"],
                "match_end_date": e["fin"], "entry_user_limit": "NO", "entry_user_num": 0,
                "entry_user_max": int(e.get("cupo") or 1000), "entry_limit_division": "", "entry_limit_division_min": str(e.get("division_min") or "D12"),
                "entry_limit_division_max": str(e.get("division_max") or "D1"), "is_end": "NO",
                "is_entry": self._inscrito(s, "pesleague", e["compe_id"]), "is_gm_account": "NO",
                "compe_db_id": e["compe_id"],
                "banner_file_info": self._banner(), "list_banner_file_info": self._banner()}))
        return {"compe_list": lista, "list_num": len(lista), "list_max": len(lista), "is_no_check": "NO",
                "add_limit_rating": 0, "is_end": "YES"}  # R (pesreplay.py AzrakHQ): fin de la paginación

    def _torneo_pesleague(self, m):
        cid = self._id_entero(m.get("compe_id"))
        return next((x for x in self._torneos("pesleague") if x["compe_id"] == cid), None)

    async def c_cmd_get_pesleague_compe_info(self, s, m):
        e = self._torneo_pesleague(m)
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        datos = {
            "compe_id": e["compe_id"], "compe_name": str(e.get("compe_name") or f"Torneo {e['compe_id']}"),
            "is_myclub": "YES", "match_type": str(e.get("match_type") or ""), "start_date": e["inicio"],
            "end_date": e["fin"], "entry_limit_date": e["fin"], "match_end_date": e["fin"],
            "entry_user_limit": "NO", "entry_user_num": 0, "entry_user_max": int(e.get("cupo") or 1000),
            "entry_limit_division": "", "entry_limit_division_min": str(e.get("division_min") or "D12"),
            "entry_limit_division_max": str(e.get("division_max") or "D1"), "banner_file_info": self._banner(),
            "is_exist_overview_info": "NO", "is_exist_participation_right": "NO", "is_exist_rewardInfo": "NO",
            "compe_db_id": e["compe_id"], "use_ball_id": int(e.get("ball_id") or 0), "is_e_sports": "NO",
            "is_pesleague_stadium": "NO", "is_efootball_pro_stadium": "NO",
            "entry_compe": self._inscrito(s, "pesleague", e["compe_id"]),
            # Prueba 11-nov: con team_id 0 y entry_compe YES el PES volvía a pedir los Términos de Uso
            # (aceptación local, sin comando) y el equipo en cada «Reanudar torneo»: va el equipo inscrito
            "team_id": self._equipo_pesleague(s, e["compe_id"]),
            "clan_info": Empaquetado({}), "compe_regulation": Empaquetado({}), "eula_url": ""}
        # R (pestourney.compe_info AzrakHQ, probado en vivo): los campos van ARRIBA y también dentro de
        # compe_data. Prueba 12-nov: solo dentro, el PES seguía pidiendo los Términos en cada «Reanudar
        # torneo» aunque entry_compe era YES y eula_url "" (regla eula_agree de peslg_path.md)
        return {**datos, "compe_data": Empaquetado(dict(datos))}

    def _equipo_pesleague(self, s, cid):
        club = self.datos.club(s.steam_hex or "") if s.steam_hex else None
        return int(((club or {}).get("pesleague_equipo") or {}).get(str(cid)) or 0)

    async def c_cmd_get_pesleague_compe_participation_right(self, s, m):
        e = self._torneo_pesleague(m)
        if e is None:
            return {"result": "ERR_COMPE_NOTFOUND"}
        return {"participation_right": Empaquetado({
            "compe_id": e["compe_id"], "compe_name": str(e.get("compe_name") or ""),
            "banner_file_info": self._banner(), "start_date": e["inicio"], "end_date": e["fin"],
            "need_match_num": 0, "need_point": 0, "player_point": 0, "clan_name": "", "clan_owner_pid": 0,
            "clan_owner_name": Empaquetado({"pname": ""}), "is_get": "YES"})}

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
        self._sumar_partido_evento(club, desenlace)
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

    def _sumar_partido_evento(self, club, desenlace):
        """C.8: con evento inscrito, el partido vs COM suma al progreso. Puntos por resultado en
        config myclub.puntos_evento (por defecto 3/1/0). Al llegar a los partidos y puntos del
        evento queda «superado» (FINISHWIN) y, la primera vez, se cobran sus premio_gp."""
        for modo, ins in (club.get("eventos_inscripcion") or {}).items():
            e = self._evento(ins.get("compe_id")) if isinstance(ins, dict) else None
            if e is None or ins.get("resultado") == "FINISHWIN":
                continue
            tabla = (self.cfg.get("myclub") or {}).get("puntos_evento") or {"WIN": 3, "DRAW": 1, "LOSE": 0}
            ins["progreso"] = int(ins.get("progreso") or 0) + 1
            ins["puntos"] = int(ins.get("puntos") or 0) + int(tabla.get(desenlace, 0) or 0)
            partidos, puntos = self._umbral_evento(e)
            if ins["progreso"] >= partidos and ins["puntos"] >= puntos:
                ins["resultado"] = "FINISHWIN"
                sup = club.setdefault("eventos_superados", {})
                clave = str(e["compe_id"])
                if not sup.get(clave):
                    club["gp"] = int(club.get("gp") or 0) + int(e.get("premio_gp") or 0)
                    if self._monedas_activas():
                        club["monedas"] = int(club.get("monedas") or 0) + int(e.get("premio_monedas") or 0)
                sup[clave] = int(sup.get(clave) or 0) + 1
                log.info("Eventos: evento %s superado (%d partidos, %d puntos)", clave,
                         ins["progreso"], ins["puntos"])
            return

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
        # R (rooms.md AzrakHQ, probado en vivo 06-nov): RELOAD SOLO como respuesta a un pedido nuevo de
        # vigilancia (su caché está vacía). Un RELOAD empujado sobre una caché con datos hace que el
        # mezclador 0x1411c1340 lance std::out_of_range y el PES se cierra. Lo demás va como ADD/UPDATE/DEL.
        if msgid == "CMD_REQWATCHL_ROOMLIST":
            self.suscritos_salas.add(s)
            salas = list(self.salas.values())
            s.completar(msgid, **self._lista_vigilada("room_id", "RELOAD",
                                                      [Empaquetado(x.ficha(s, i)) for i, x in enumerate(salas)]))
        elif msgid == "CMD_REQWATCHL_ROOMPLAYERLIST_NTL":
            sala = self.salas.get(s.room_id)
            datos = [Empaquetado(self._direcciones_para(sala.ficha_jugador(x, i), x, s))
                     for i, x in enumerate(sala.miembros)] if sala else []
            s.completar(msgid, **self._lista_vigilada("pid", "RELOAD", datos))
        elif msgid == "CMD_WATCH_TURN_ADDRESS_DATA" and s.pid in self.turn_pendiente:
            addr, size = self.turn_pendiente.pop(s.pid)
            s.completar(msgid, address=addr, address_data_size=size)

    @staticmethod
    def _lista_vigilada(clave, tipo, datos):
        return {"list_type": tipo, "list_key_name": clave, "list_num": len(datos), "list_data": datos}

    @staticmethod
    def _empujar(x, msgid, **campos):
        """Empuje con rqid 0 y NOERR (forma de AzrakHQ y de nuestra búsqueda rápida, probada en vivo)."""
        x.enviar({"msgid": msgid, "rqid": 0, "result": NOERR, **campos})

    def _empujar_salas_a_todos(self, accion="UPDATE", salas=None):
        """ADD / UPDATE / DEL de salas a quienes miran la lista (nunca RELOAD empujado)."""
        for sala in salas or []:
            for x in list(self.suscritos_salas):
                if x not in self.sesiones:
                    self.suscritos_salas.discard(x)
                    continue
                if accion == "DEL":
                    datos = [Empaquetado({"room_id": sala.room_id})]
                else:
                    datos = [Empaquetado(sala.ficha(x, 0))]
                self._empujar(x, "CMD_REQWATCHL_ROOMLIST", **self._lista_vigilada("room_id", accion, datos))

    def _empujar_jugadores(self, sala, accion="UPDATE", solo=None, se_fue=None):
        """UPDATE (o DEL de quien se fue) de la lista de jugadores, SOLO a los miembros que ya mandaron
        ENTRY_GAME desde que entraron (empujar antes cerraba el PES del dueño)."""
        for x in list(sala.miembros):
            if x not in sala.escuchan or (solo is not None and x not in solo) or x not in self.sesiones:
                continue
            if accion == "DEL":
                datos = [Empaquetado({"pid": se_fue})]
            else:
                datos = [Empaquetado(self._direcciones_para(sala.ficha_jugador(y, i), y, x))
                         for i, y in enumerate(sala.miembros)]
            self._empujar(x, "CMD_REQWATCHL_ROOMPLAYERLIST_NTL", **self._lista_vigilada("pid", accion, datos))

    # ---------- Salas del vestíbulo amistoso 1 vs 1 (R: pesrooms.py + rooms.md de AzrakHQ) ----------
    async def c_cmd_createjoin_room(self, s, m):
        await self._salir_de_sala(s)
        s.pid_visible = self._pid_visible(s)
        nombre = m.get("name") if isinstance(m.get("name"), str) and m.get("name") else f"Sala de {s.pname}"
        sala = Sala(s, nombre[:64], {k: v for k, v in m.items() if k not in ("msgid", "rqid")})
        self.salas[sala.room_id] = sala
        s.room_id, s.entry = sala.room_id, False
        self.suscritos_salas.add(s)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR, "room_id": sala.room_id})
        # R (roomcrash.md): el dueño busca SU sala en la lista justo después: ADD de inmediato
        await self.dormir(PAUSA_EMPUJE)
        self._empujar_salas_a_todos("ADD", [sala])
        return None

    async def c_cmd_set_gameenv(self, s, m):
        """R: los 9 ajustes llegan empaquetados en game_env (bin). Se guardan y se repiten en la fila
        de la sala; sin ellos limitTime quedaba en NOSET y la sala no salía para el 2.º jugador."""
        sala = self.salas.get(s.room_id)
        if sala and s is sala.dueno:
            env = m.get("game_env")
            if isinstance(env, (bytes, bytearray)):
                try:
                    env = msgpack_min.unpackb(bytes(env))
                except Exception:
                    env = None
            if not isinstance(env, dict):
                env = {k: v for k, v in m.items() if k in GAMEENV_SALA}
            sala.gameenv.update({k: v for k, v in env.items() if k in GAMEENV_SALA})
            self._empujar_salas_a_todos("UPDATE", [sala])
        return {}

    async def c_cmd_join_room(self, s, m):
        """R (commands.json + server_errors.tsv de AzrakHQ): el PES manda CMD_JOIN_ROOM_NTL {room_id,
        is_invited, password} y espera room_owner (bin {pid, pname, direcciones}) y room_stat. Los
        errores válidos son los de su tabla (ERR_PASSWDINCORRECT, ERR_ROOMSTATNOTMATCH, ...)."""
        sala = self.salas.get(self._id_entero(m.get("room_id")))
        if sala is None:
            return {"result": "ERR_ROOMNOTFOUND"}
        s.pid_visible = self._pid_visible(s)
        if s not in sala.miembros:
            if len(sala.miembros) >= sala.max_players:
                return {"result": "ERR_ROOMISFULL"}
            if sala.estado != "WAITING":
                return {"result": "ERR_ROOMSTATNOTMATCH"}
            clave = m.get("password", m.get("passwd"))
            if sala.passwd and str(clave or "") != str(sala.passwd):
                return {"result": "ERR_PASSWDINCORRECT"}
            if self.cfg.get("exigir_misma_huella") and s.huella != sala.dueno.huella:
                log.info("huella distinta: %s vs %s", s.huella, sala.dueno.huella)
                return {"result": "ERR_DIFFDCVERSION"}  # R: «Paquete de datos incompatible»
            await self._salir_de_sala(s)
            sala.miembros.append(s)
            log.info("sala %s: entra %s", sala.room_id, s.pid)
        s.room_id, s.entry = sala.room_id, False
        sala.escuchan.discard(s)
        otros = {x for x in sala.miembros if x is not s}
        dueno = sala.dueno
        ficha = {"pid": self._pid_visible(dueno), "pname": Sala.pname(dueno)}
        for k in ("host_addr", "reflexive_addr", "relayed_addr"):
            v = dueno.ntl.get(k)
            if v:
                ficha[k] = Empaquetado(v) if isinstance(v, dict) else v
        ficha = self._direcciones_para(ficha, dueno, s)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR,
                  "room_id": sala.room_id, "room_owner": Empaquetado(ficha), "room_stat": sala.estado})
        await self.dormir(PAUSA_EMPUJE)
        self._empujar_salas_a_todos("UPDATE", [sala])
        self._empujar_jugadores(sala, "UPDATE", solo=otros)
        return None

    c_cmd_join_room_ntl = c_cmd_join_room

    async def c_cmd_set_room_option(self, s, m):
        """R (pesrooms.py): {pid, name, password, is_anyplayers, auto_start_num, game_rating_limit};
        solo el dueño cambia su sala y todos ven la lista nueva."""
        sala = self.salas.get(s.room_id)
        if sala is None:
            return {"result": "ERR_NOTINROOM"}
        if s is not sala.dueno:
            return {"result": "ERR_NOTROOMOWNER"}
        if isinstance(m.get("name"), str) and m["name"].strip():
            sala.nombre = m["name"][:64]
        if "password" in m:
            sala.passwd = str(m.get("password") or "") or None
            sala.opciones["password"] = sala.passwd or ""
        for k in ("is_anyplayers", "auto_start_num", "game_rating_limit"):
            if k in m and m[k] not in (None, ""):
                sala.opciones[k] = m[k]
        self._empujar_salas_a_todos("UPDATE", [sala])
        return {}

    async def c_cmd_entry_game(self, s, m):
        """«Empezar partido» (entry TRUE) o cancelarlo. Desde aquí su lista de jugadores puede
        actualizarse. A todos: WATCH_ENTRY_GAME {entry_count, entrygame:[bin{pid, entryNum, gameNum}]}
        y un UPDATE de jugadores (la comprobación de inicio del dueño corre tras ese UPDATE)."""
        sala = self.salas.get(s.room_id)
        if sala is None:
            return {"result": "ERR_NOTINROOM"}
        sala.escuchan.add(s)
        entrada = m.get("entry", "FALSE")
        s.entry = (entrada.upper() in ("TRUE", "YES", "1")) if isinstance(entrada, str) else bool(entrada)
        if s.entry:
            sala.entradas.setdefault(s, len(sala.entradas) + 1)
        else:
            sala.entradas.pop(s, None)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR,
                  "entryflag": "TRUE" if s.entry else "FALSE", "gameNum": sala.game_no})
        await self.dormir(PAUSA_EMPUJE)
        lista = [Empaquetado({"pid": Sala.pid_de(x), "entryNum": n, "gameNum": sala.game_no})
                 for x, n in sala.entradas.items()]
        for x in sala.miembros:
            self._empujar(x, "CMD_WATCH_ENTRY_GAME", entry_count=len(lista), entrygame=lista)
        self._empujar_jugadores(sala, "UPDATE")
        return None

    async def c_cmd_set_gamemember(self, s, m):
        """El dueño pide empezar (lo manda en cada cambio). Solo con los 2 dentro y los 2 con «Empezar
        partido»: WATCH_DECIDE_GAMEENV {game_env} + WATCH_ROOMSTATE SETENV y se abre la partida (relé,
        id de partido, resultados) igual que en la búsqueda rápida. Fuera de una sala: solo NOERR."""
        sala = self.salas.get(s.room_id)
        if sala is None or s is not sala.dueno or sala.estado != "WAITING":
            return {}
        if len(sala.miembros) < 2 or set(sala.entradas) != set(sala.miembros):
            return {}
        sala.estado = "SETENV"
        sala.salieron = set()
        sala.partido = f"{sala.room_id}-{int(time.time())}"
        invitado = next(x for x in sala.miembros if x is not sala.dueno)
        for x in sala.miembros:
            self._salir_de_cola(x)
            self._dejar_pareja(x, "empieza partido en sala")
        pareja = {"miembros": (sala.dueno, invitado), "check": set(), "tipo": "FREE", "sala": sala.room_id}
        self.parejas[sala.dueno] = self.parejas[invitado] = pareja
        self._abrir_partida(pareja)
        env = {**GAMEENV_SALA, **sala.gameenv}
        log.info("sala %s: empieza el partido %s (%s vs %s)", sala.room_id, pareja["room_id"],
                 sala.dueno.pid, invitado.pid)
        s.enviar({"msgid": m["msgid"], "rqid": m.get("rqid", 0), "result": NOERR})
        for x in sala.miembros:
            self._empujar(x, "CMD_WATCH_DECIDE_GAMEENV", game_env=Empaquetado(env))
            self._empujar(x, "CMD_WATCH_ROOMSTATE", event="StateChanged", state="SETENV",
                          owner_pid=self._pid_visible(sala.dueno), password="")
        await self.dormir(PAUSA_EMPUJE)
        self._empujar_salas_a_todos("UPDATE", [sala])
        return None

    async def c_cmd_send_game_result(self, s, m):
        pareja = self._pareja_por_clave(s)
        if pareja and pareja.get("room_id"):
            try:
                self._resultado_division(s, pareja, m)
            except Exception:
                log.exception("no se pudo aplicar el resultado de Divisiones")
            try:
                self._resultado_matchday(s, pareja, m)
            except Exception:
                log.exception("no se pudo aplicar el resultado de Matchday")
            try:
                self._resultado_pesleague(s, pareja, m)
            except Exception:
                log.exception("no se pudo aplicar el resultado del Torneo en línea")
            try:
                self._guardar_historial(pareja, m)
            except Exception:
                log.exception("no se pudo guardar el historial del partido")
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
        """R (pesmanner.py): el propio PES avisa GIVEUP / EXIT (salir desde la pausa) o NO_OPERATION
        (un minuto sin tocar nada). Con el partido ya empezado y sin terminar, cuenta como abandono; a
        los 15 s, si abandonó UNO solo, pierde 3-0 (o por lo que iba perdiendo) en Matchday, Torneo y
        Divisiones. Un corte de red entre los dos no cuenta contra nadie."""
        motivo = str(m.get("reason") or "").upper()
        log.warning("fin anormal pid=%s motivo=%s", s.pid, motivo or m)
        pareja = self._pareja_por_clave(s)
        if motivo in self.MOTIVOS_ENLACE and self._anular_por_enlace(pareja, s, motivo):
            self._dejar_pareja(s, "enlace P2P cortado")
            return {}
        if pareja is not None and pareja.get("en_juego") and not pareja.get("terminado") \
                and motivo in self.MOTIVOS_PROPIOS:
            marcados = pareja.setdefault("abandonaron", set())
            if s not in marcados:
                marcados.add(s)
                self._anotar_modales((s.steam_hex or "").lower(), True, motivo)
                self._programar_derrota(pareja)
        self._dejar_pareja(s, "fin anormal")
        return {}

    # R (match.md AzrakHQ, 0x1410f6220): el propio PES informa del estado del ENLACE P2P: DISCONN_SC = enlace
    # caido, DEGRADE_SC = enlace degradado. Si pasa con los DOS aun conectados al servidor no es culpa de
    # nadie: el partido se anula (sin derrota, sin puntos, sin racha).
    MOTIVOS_ENLACE = {"DISCONN_SC", "DEGRADE_SC"}

    def _anular_por_enlace(self, pareja, quien, motivo):
        """P2P cortado entre los 2 PES con los 2 todavia en el servidor: anula la partida sin derrota."""
        if pareja is None or pareja.get("terminado") or pareja.get("anulada") or not pareja.get("en_juego"):
            return False
        if not all(x in self.sesiones for x in pareja["miembros"]):
            return False  # alguien se fue del servidor: eso lo decide el abandono normal
        pareja["anulada"] = True
        pareja["puntua"] = False  # ni Divisiones, ni Matchday, ni Torneo en linea la cuentan
        pareja.pop("abandonaron", None)
        pareja.pop("derrota_pendiente", None)
        pareja["terminado"] = True
        log.warning("partido %s anulado sin derrota: enlace P2P cortado (%s, pid=%s) con los dos en el servidor",
                    pareja.get("room_id"), motivo, getattr(quien, "pid", None))
        return True

    def _sondear_vida(self, pareja):
        """R (match.md): CMD_WATCH_CHECK_ALIVE_PLAYER {result NOERR} es un empujon del servidor; el PES
        contesta con SEND_ALIVE y SEND_ALIVEPLAYER. Solo con config sonda_vida=true (por defecto apagado:
        en vivo nunca se ha visto)."""
        if not self.cfg.get("sonda_vida") or pareja is None:
            return 0
        n = 0
        for x in pareja["miembros"]:
            if x in self.sesiones:
                self._empujar(x, "CMD_WATCH_CHECK_ALIVE_PLAYER", result=NOERR)
                n += 1
        return n

    async def c_cmd_send_alive(self, s, m):
        """{pid, reason}: reason = estado del partido (NOERR normal). DISCONN_SC / DEGRADE_SC = enlace P2P."""
        motivo = str(m.get("reason") or "").upper()
        if motivo in self.MOTIVOS_ENLACE:
            self._anular_por_enlace(self._pareja_por_clave(s), s, motivo)
        return {}

    async def c_cmd_send_aliveplayer(self, s, m):
        """{pid, pid_list}: a quien oye por P2P. Solo se acepta; la anulacion llega por reason de SEND_ALIVE."""
        return {}

    def _programar_derrota(self, pareja):
        if pareja.get("derrota_programada") or pareja.get("anulada"):
            return
        pareja["derrota_programada"] = True
        pareja["derrota_pendiente"] = True  # sin veredicto normal mientras se decide

        def decidir():
            pareja.pop("derrota_pendiente", None)
            if pareja.get("anulada"):
                return
            marcados = pareja.get("abandonaron") or set()
            if len(marcados) == 1:
                try:
                    self._derrota_por_abandono(pareja, next(iter(marcados)))
                except Exception:
                    log.exception("derrota por abandono")
        pareja["decidir_derrota"] = decidir
        try:
            asyncio.get_running_loop().call_later(15, decidir)
        except RuntimeError:
            pass

    def _derrota_por_abandono(self, pareja, quien):
        if pareja.get("derrota") is not None:
            return
        casa, fuera = pareja["miembros"]
        g = pareja.get("marcador") or {"HOME": 0, "AWAY": 0}
        # por clave: tras un RECONNECT la sesión de la pareja ya no es el mismo objeto
        es_casa = quien is casa or (bool(quien.steam_hex) and quien.steam_hex == casa.steam_hex)
        if not es_casa and quien is not fuera and quien.steam_hex != fuera.steam_hex:
            return
        quien = casa if es_casa else fuera
        gana = "AWAY" if es_casa else "HOME"
        pierde = "HOME" if gana == "AWAY" else "AWAY"
        marcador = dict(g) if g[gana] > g[pierde] else {gana: 3, pierde: 0}
        pareja["derrota"] = quien
        m = {"home_score": marcador["HOME"], "away_score": marcador["AWAY"], "abnormalend_reason": "NORMAL_END"}
        for x in (casa, fuera):
            self._resultado_matchday(x, pareja, m)
            self._resultado_pesleague(x, pareja, m)
        self._resultado_division(quien, pareja, m)
        log.info("partido %s: %s abandonó; gana el otro %s-%s", pareja.get("room_id"), quien.pid,
                 marcador["HOME"], marcador["AWAY"])

    async def c_cmd_leave_game(self, s, m):
        self._abandono_division(s)
        self._dejar_pareja(s, "salió del partido")
        # R (pesrooms.py, probado en vivo): cuando los 2 salieron del partido, StateChanged WAITING los
        # devuelve a la sala para la revancha
        sala = self.salas.get(s.room_id)
        if sala is not None:
            sala.salieron.add(s)
            self._volver_a_sala(sala)
        return {}

    def _volver_a_sala(self, sala):
        """Cuando todos los que siguen en la sala ya salieron del partido, la sala vuelve a WAITING para
        la revancha (entradas a cero, partido nuevo). También se llama si el OTRO se va de la sala sin
        mandar LEAVE_GAME (PESBUL: la sala quedaba inservible tras el partido). True si volvió."""
        if not sala.miembros or sala.estado == "WAITING" or not (sala.salieron >= set(sala.miembros)):
            return False
        sala.estado, sala.entradas, sala.salieron = "WAITING", {}, set()
        sala.game_no += 1
        for x in sala.miembros:
            x.entry = False
            self._empujar(x, "CMD_WATCH_ROOMSTATE", event="StateChanged", state="WAITING",
                          owner_pid=self._pid_visible(sala.dueno), password="")
        self._empujar_salas_a_todos("UPDATE", [sala])
        log.info("sala %s: vuelven a la sala tras el partido", sala.room_id)
        return True

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
        if not sala or s not in sala.miembros:
            return
        sala.miembros.remove(s)
        sala.entradas.pop(s, None)
        sala.escuchan.discard(s)
        sala.salieron.discard(s)
        if not sala.miembros:
            del self.salas[sala.room_id]
            self._empujar_salas_a_todos("DEL", [sala])
            log.info("sala %s cerrada", sala.room_id)
            return
        self._empujar_jugadores(sala, "DEL", se_fue=Sala.pid_de(s))
        if sala.dueno is s:
            # R (pesrooms.py): el nuevo dueño recibe AppointOwner (con la clave); los demás OwnerChanged
            nuevo = sala.dueno = sala.miembros[0]
            self._empujar(nuevo, "CMD_WATCH_ROOMSTATE", event="AppointOwner", state=sala.estado,
                          owner_pid=self._pid_visible(nuevo), password=sala.passwd or "")
            for x in sala.miembros[1:]:
                self._empujar(x, "CMD_WATCH_ROOMSTATE", event="OwnerChanged", state=sala.estado,
                              owner_pid=self._pid_visible(nuevo), password="")
            self._empujar_jugadores(sala, "UPDATE")
        self._empujar_salas_a_todos("UPDATE", [sala])
        # el que se fue de la sala sin LEAVE_GAME no debe dejar la sala atascada en SETENV
        self._volver_a_sala(sala)

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
        # R (coop2.md AzrakHQ, 0x1410c2b40): el Cooperativo toma el modo de red de game_sycom; con ""
        # no hay sesión multijugador (MXNV_0010). Partida rápida y salas lo fijan solas. Solo a quien
        # está en una sala de Cooperativo. myclub.coop_game_sycom = "NO" lo quita.
        clave = (getattr(s, "steam_hex", "") or "").lower()
        pareja = self._pareja_por_clave(s) if s is not None else None
        if (pareja or {}).get("tipo") == "COOP" and (self.cfg.get("myclub") or {}).get("coop_game_sycom") != "NO":
            r["game_sycom"] = "P2P"
        elif clave and (self.cfg.get("myclub") or {}).get("coop_game_sycom") != "NO" and any(
                (getattr(x, "steam_hex", "") or "").lower() == clave
                for sala in getattr(self, "salas_coop", {}).values() for x in sala["miembros"]):
            r["game_sycom"] = "P2P"
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

    def respuesta_livedata(self, flujo, formulario, ip_cliente=None, ip_local=None):
        """Datos en vivo por HTTP: «no hay actualización» con las versiones del juego (P, PESBUL)."""
        log.info("HTTP livedata %s campos=%s", flujo, sorted(formulario))
        # Prueba 12-nov: el PES pide CMD_GET_DLC_FILELIST por «POST //livedata/LiveDataSetFlow»; antes
        # todo lo de livedata/ recibía «no_update» y la actualización propia nunca se ofrecía
        if formulario.get("msgid") == "CMD_GET_DLC_FILELIST":
            return self.respuesta_http(formulario, ip_cliente, ip_local)
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
        self._recargar_config()
        if msgid == "CMD_GET_TURN_SERVER_LIST" and "turn" in self._modulos_apagados():
            log.warning("interruptor: módulo «turn» apagado, lista TURN vacía")
            return {"msgid": msgid, "rqid": int(formulario.get("rqid", 0) or 0), "result": NOERR}
        if msgid == "CMD_GET_TURN_SERVER_LIST" and self.turn_propio is not None and self.turn_propio.activo():
            # R (turn.py AzrakHQ): una entrada con port y tls_port distintos de 0 (si no, el PES la descarta),
            # dirección = la IP por la que este PES llega al servidor, y el certificado en DER
            pid = formulario.get("pid", "0")
            return {"msgid": msgid, "rqid": int(formulario.get("rqid", 0) or 0), "result": NOERR,
                    **self.turn_propio.lista(pid, self.ip_anuncio(ip_local=ip_local))}
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
            if msgid.endswith("_BYID"):
                # el buscado se busca en TODA la tabla (antes solo en la 1.ª página); si no se pidió a
                # nadie concreto, ventana alrededor de quien pregunta (R: pesranking AzrakHQ)
                todos = self._ranking_division(formulario, todo=True)
                r = self._solo_buscado(todos, formulario)
                if not r["rank_list"] and not self._hay_buscado(formulario):
                    r = self._ranking_division(formulario, por_id=True)
            else:
                r = self._ranking_division(formulario)
            return {**base, **r}
        if msgid in ("CMD_GET_MYCLUB_RANKING", "CMD_GET_MYCLUB_RANKING_BYID"):
            r = self._ranking_myclub(formulario)
            if msgid.endswith("_BYID"):
                r = self._solo_buscado(r, formulario)
            return {**base, **r}
        if msgid in ("CMD_GET_CLAN_RANKING", "CMD_GET_CLAN_RANKING_BYID"):
            # Probado en el PES 12-nov: el ranking de clanes llega por HTTP (POST //clan_ranking con
            # division, is_old_season, is_myclub, start_index, view_num), no por el socket del menú
            return {**base, **self._ranking_clanes(formulario)}
        if msgid in ("CMD_GET_MYCLUB_WEEKLY_RANKING", "CMD_GET_MYCLUB_WEEKLY_RANKING_BYID"):
            # B.7: sin torneo semanal propio todavía: respuesta vacía válida
            return {**base, "date": int(time.time()), "total_rank_num": 0, "rank_list": [],
                    "max_weekly_point": 0, "group_number": 0, "division": "", "reward_list": []}
        if msgid == "CMD_GET_DLC_FILELIST":
            # R (pescmds.py AzrakHQ): listas vacías = nada que bajar; dlc_version igual a la del PES.
            # Con un .cpk en la carpeta «actualizacion» se ofrece SIEMPRE (actualizacion.py); la URL usa
            # la IP por la que llegó este pedido (un PES de la casa no llega a la IP pública: SGJC_0007)
            ver = str(self.cfg.get("dlc_version") or "7.00")
            from . import actualizacion
            url_base = f"http://{self.ip_anuncio(ip_local=ip_local)}:{self.cfg.get('puerto_http', 5000)}"
            f = actualizacion.fila(self.cfg.get("carpeta_actualizacion", "actualizacion"), url_base)
            if f is not None:
                return {**base, "dlc_version": ver, "dlcfile_list_count": 1, "dlcfile_list": [f],
                        "variation_file_list_count": 0, "variation_file_list": []}
            return {**base, "dlc_version": ver,
                    "dlcfile_list_count": 0, "dlcfile_list": "",
                    "variation_file_list_count": 0, "variation_file_list": ""}
        if msgid == "CMD_SET_PLAYDATA_MATCH":
            self._guardar_playdata(formulario)
            return base
        if msgid == "CMD_RECV_MYCLUB_PRESENT":
            return {**base, **self.recibir_regalos(formulario, ip_cliente)}
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
                "division": self._estado_division(clave)["division"], "manner": self.grado_modales(clave),
                "favoriteTeam": perfil["favorite_team"], "favoritePlayer": perfil["favorite_player"],
                "intro": perfil["intro"],
                "match_record": {"win_count": g, "draw_count": e, "lose_count": p,
                                 "win_s_max": int(com.get("racha_max") or 0), "rating": RATING_INICIAL},
                # C (prueba 10-nov): sin nombre el PES pinta «NotImplement»: nombre del jugador
                "team_id": int(club.get("base_team_id") or 0),
                "team_name": nombre_club}})
        return {"date": int(time.time()), "total_rank_num": len(filas), "rank_list": lista}

    @staticmethod
    def _hay_buscado(m):
        return any(m.get(k) not in (None, "", "0", 0, "NotImplement") for k in ("target_pid", "xuid_one", "np_account_id"))

    @staticmethod
    def _solo_buscado(r, m):
        """_BYID: la fila de la persona buscada (por pid, xuid_one o np_account_id)."""
        # «pid» es quien pregunta; el buscado viene en xuid_one / np_account_id (o target_pid)
        buscados = {str(m.get(k)) for k in ("target_pid", "xuid_one", "np_account_id")
                    if m.get(k) not in (None, "", "0", 0, "NotImplement")} or {str(m.get("pid"))}
        filas = [f for f in r["rank_list"] if str(f["user"]["pid"]) in buscados
                 or str(f["user"]["pid"] & 0xFFFFFFFF) in buscados]
        return {**r, "rank_list": filas[:1]}

    AVATAR_MAX = 0xC000  # 128x128x3

    def _avatar(self, pid):
        """R (pesavatar AzrakHQ) + C (PESBUL avatar_service): avatar_data = base64 de una imagen 128x128x3,
        avatar_data_size = bytes y avatar_md5 = md5 de esos bytes; el PES lo acepta solo si el tamaño es
        1..0xC000 y el md5 cuadra. Se devuelve tal cual lo subió. Sin avatar: datos vacíos (silueta)."""
        clave = self._clave_de_id(pid)
        a = self.datos.avatar(clave) if clave else None
        if a is None:
            texto = self.avatares.get(str(pid or ""), "")
            if not texto:
                return {"avatar_data": "", "avatar_data_size": 0, "avatar_md5": ""}  # silueta (así ya se probó)
            crudo = base64.b64decode(texto)
            a = {"data": texto, "size": len(crudo), "md5": hashlib.md5(crudo).hexdigest()}
        return {"avatar_data": a["data"], "avatar_data_size": a["size"], "avatar_md5": a["md5"]}

    async def c_cmd_get_avatar_http(self, s, m):
        return self._avatar(m.get("pid"))

    c_cmd_get_avatar = c_cmd_get_avatar_http

    async def c_cmd_set_avatar(self, s, m):
        """R (pesavatar AzrakHQ): al salir del editor (Ajustes en línea > Perfil de usuario) el PES sube
        {avatar_data_size, avatar_data, avatar_md5}. Solo el avatar propio; se guarda en la base para que
        sobreviva a un reinicio y lo vean los demás aunque estés desconectado."""
        datos = m.get("avatar_data") or ""
        if isinstance(datos, (bytes, bytearray)):
            datos = bytes(datos).decode("ascii", "replace")
        try:
            crudo = base64.b64decode(datos, validate=True) if datos else b""
        except (ValueError, TypeError):
            log.warning("avatar de %s no es base64; ignorado", s.pid)
            return {}
        if len(crudo) > self.AVATAR_MAX:
            return {}
        md5 = str(m.get("avatar_md5") or hashlib.md5(crudo).hexdigest()).lower()
        tam = self._id_entero(m.get("avatar_data_size")) or len(crudo)
        if getattr(s, "steam_hex", None):
            self.datos.guardar_avatar(s.steam_hex, datos, tam, md5)
        else:
            self.avatares[str(self._pid_visible(s))] = datos
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
        """R (turn.py AzrakHQ, probado en vivo): la dirección de relé de un PES va byte a byte a su
        rival como WATCH_TURN_ADDRESS_DATA (rqid 0). Quien la manda recibe también la que el rival mandó
        antes (pudo llegar antes de que el otro vigilara). Sin pareja: el destino es m['pid'] (antiguo)."""
        addr, size = m.get("address", b""), int(m.get("address_data_size", 0) or 0)
        pareja = self._pareja_por_clave(s)
        if pareja is None:
            destino = m.get("pid")
            otro = self.por_pid.get(destino)
            if not (otro and otro.completar("CMD_WATCH_TURN_ADDRESS_DATA", address=addr, address_data_size=size)):
                self.turn_pendiente[destino] = (addr, size)
            return {}
        guardadas = pareja.setdefault("turn_addr", {})
        guardadas[s] = {"address": addr, "address_data_size": size}
        for x in pareja["miembros"]:
            if x is s:
                continue
            if x in self.sesiones:
                self._empujar(x, "CMD_WATCH_TURN_ADDRESS_DATA", **guardadas[s])
            if x in guardadas:
                self._empujar(s, "CMD_WATCH_TURN_ADDRESS_DATA", **guardadas[x])
        log.info("TURN: dirección de %s (%d bytes) enviada al rival", s.pid, size)
        return {}
