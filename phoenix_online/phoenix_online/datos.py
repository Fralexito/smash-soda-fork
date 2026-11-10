"""Datos persistentes: jugadores, vínculos con la cuenta Phoenix y resultados (SQLite)."""
import glob
import json
import logging
import os
import sqlite3
import time

log = logging.getLogger("phoenix_online.datos")


ESQUEMA = """
create table if not exists jugadores (
  pid integer primary key autoincrement,
  steam_hex text unique not null,
  pname text not null,
  creado real not null
);
create table if not exists perfiles (
  steam_hex text primary key,
  favorite_team text not null default '',
  favorite_player text not null default '',
  intro text not null default ''
);
create table if not exists myclub (
  steam_hex text primary key,
  datos text not null
);
create table if not exists pases (
  ip text primary key,
  usuario text not null,
  clave text not null,
  hora real not null
);
create table if not exists resultados (
  id integer primary key autoincrement,
  room_id integer not null,
  partido text not null,
  pid integer not null,
  datos text not null,
  hora real not null
);
"""


class Datos:
    def __init__(self, ruta=":memory:", vinculos=None):
        self.ruta = ruta
        # timeout: si otro programa (admin_myclub, respaldo) tiene la base un instante, se espera
        # en vez de fallar con «database is locked»
        self.db = sqlite3.connect(ruta, timeout=10)
        if ruta != ":memory:":
            # WAL: un corte de luz o un cierre a la fuerza no deja la base a medio escribir, y
            # las herramientas pueden leer mientras el servidor escribe
            self.db.execute("pragma journal_mode=wal")
            self.db.execute("pragma synchronous=normal")
            fila = self.db.execute("pragma quick_check").fetchone()
            if not fila or fila[0] != "ok":
                log.error("BASE DE DATOS DAÑADA (%s): restaurar el último respaldo de "
                          "respaldos/ con tools/respaldo_base.py", fila)
        self.db.executescript(ESQUEMA)
        # vinculos: {steam_hex: {"usuario_phoenix": ..., "pname": ...}} o None = abierto a todos
        self.vinculos = vinculos
        self.cambiados = set()  # clubes que cambiaron desde la última subida a la nube

    def autorizado(self, steam_hex):
        return self.vinculos is None or steam_hex.lower() in self.vinculos

    def jugador(self, steam_hex):
        f = self.db.execute("select pid, pname from jugadores where steam_hex=?",
                            (steam_hex.lower(),)).fetchone()
        return {"pid": f[0], "pname": f[1]} if f else None

    def crear_jugador(self, steam_hex, pname):
        # C (02-nov): el nombre que devuelve GET_PLAYER_DATA debe ser el MISMO que el cliente
        # mandó en CREATE_PLAYER (con el de la web el cliente repite CREATE_PLAYER en bucle).
        # El nombre de la web Phoenix queda en vinculos.json para la web, no para el juego.
        ya = self.jugador(steam_hex)
        if ya:
            # el cliente puede volver a pedir crear (p. ej. tras un cierre antes de guardar):
            # se reutiliza el perfil y se toma el nombre que acaba de escribir
            if pname and pname != ya["pname"]:
                self.db.execute("update jugadores set pname=? where steam_hex=?",
                                (pname, steam_hex.lower()))
                self.db.commit()
                ya["pname"] = pname
            return ya
        cur = self.db.execute("insert into jugadores(steam_hex, pname, creado) values (?,?,?)",
                              (steam_hex.lower(), pname, time.time()))
        self.db.commit()
        return {"pid": cur.lastrowid, "pname": pname}

    def todos(self):
        """Todos los jugadores registrados, del más antiguo al más nuevo."""
        return [{"pid": f[0], "pname": f[1], "steam_hex": f[2]} for f in
                self.db.execute("select pid, pname, steam_hex from jugadores order by pid")]

    def perfil(self, steam_hex):
        """Equipo favorito, jugador favorito y presentación («Editar perfil» del juego)."""
        f = self.db.execute("select favorite_team, favorite_player, intro from perfiles "
                            "where steam_hex=?", ((steam_hex or "").lower(),)).fetchone()
        return dict(zip(("favorite_team", "favorite_player", "intro"), f or ("", "", "")))

    def guardar_perfil(self, steam_hex, favorite_team, favorite_player, intro):
        self.db.execute("insert into perfiles(steam_hex, favorite_team, favorite_player, intro) "
                        "values (?,?,?,?) on conflict(steam_hex) do update set "
                        "favorite_team=excluded.favorite_team, "
                        "favorite_player=excluded.favorite_player, intro=excluded.intro",
                        ((steam_hex or "").lower(), favorite_team, favorite_player, intro))
        self.db.commit()

    def club(self, steam_hex):
        """Club de myClub guardado (dict) o None si aún no se creó."""
        f = self.db.execute("select datos from myclub where steam_hex=?",
                            ((steam_hex or "").lower(),)).fetchone()
        return json.loads(f[0]) if f else None

    def guardar_club(self, steam_hex, club):
        self.db.execute("insert into myclub(steam_hex, datos) values (?,?) on conflict(steam_hex) "
                        "do update set datos=excluded.datos",
                        ((steam_hex or "").lower(), json.dumps(club)))
        self.db.commit()
        self.cambiados.add((steam_hex or "").lower())

    def tomar_cambiados(self):
        c, self.cambiados = self.cambiados, set()
        return sorted(c)

    def marcar_cambiados(self, claves):
        self.cambiados.update(claves)

    # ---- pase de identidad (web): IP de la persona -> su cuenta Phoenix ----
    def guardar_pase(self, ip, usuario, clave):
        self.db.execute("insert into pases(ip, usuario, clave, hora) values (?,?,?,?) on conflict(ip) "
                        "do update set usuario=excluded.usuario, clave=excluded.clave, hora=excluded.hora",
                        (ip, str(usuario), clave.lower(), time.time()))
        self.db.commit()

    def clave_por_ip(self, ip):
        f = self.db.execute("select clave from pases where ip=?", (ip,)).fetchone()
        return f[0] if f else None

    def usuario_de_clave(self, clave):
        f = self.db.execute("select usuario from pases where clave=? order by hora desc limit 1",
                            ((clave or "").lower(),)).fetchone()
        return f[0] if f else None

    def guardar_resultado(self, room_id, partido, pid, datos):
        self.db.execute("insert into resultados(room_id, partido, pid, datos, hora) values (?,?,?,?,?)",
                        (room_id, partido, pid, json.dumps(datos, default=repr), time.time()))
        self.db.commit()

    def resultados(self, partido):
        filas = self.db.execute("select pid, datos from resultados where partido=? order by id",
                                (partido,)).fetchall()
        return {pid: json.loads(d) for pid, d in filas}

    # ---------- respaldos ----------
    def respaldar(self, carpeta="respaldos", guardar=28):
        """Copia completa y coherente de la base (API de respaldo de SQLite, sirve con el
        servidor en marcha). Guarda las `guardar` copias más nuevas; devuelve la ruta."""
        if self.ruta == ":memory:":
            return None
        os.makedirs(carpeta, exist_ok=True)
        base = os.path.splitext(os.path.basename(self.ruta))[0]
        destino = os.path.join(carpeta, f"{base}_{time.strftime('%Y%m%d-%H%M%S')}.db")
        # C (PC de Frank, 10-nov): se llama desde otro hilo (asyncio.to_thread) y SQLite no deja
        # usar self.db fuera de su hilo: conexión propia solo para copiar
        origen = sqlite3.connect(self.ruta, timeout=30)
        copia = sqlite3.connect(destino)
        try:
            origen.backup(copia)
        finally:
            copia.close()
            origen.close()
        viejas = sorted(glob.glob(os.path.join(carpeta, f"{base}_*.db")))[:-max(1, int(guardar))]
        for v in viejas:  # solo nuestras propias copias automáticas, nunca la base viva
            try:
                os.remove(v)
            except OSError:
                pass
        return destino

    def cerrar(self):
        try:
            self.db.commit()
            self.db.close()
        except sqlite3.Error:
            pass
