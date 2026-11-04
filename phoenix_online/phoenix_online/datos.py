"""Datos persistentes: jugadores, vínculos con la cuenta Phoenix y resultados (SQLite)."""
import json
import sqlite3
import time


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
        self.db = sqlite3.connect(ruta)
        self.db.executescript(ESQUEMA)
        # vinculos: {steam_hex: {"usuario_phoenix": ..., "pname": ...}} o None = abierto a todos
        self.vinculos = vinculos

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

    def guardar_resultado(self, room_id, partido, pid, datos):
        self.db.execute("insert into resultados(room_id, partido, pid, datos, hora) values (?,?,?,?,?)",
                        (room_id, partido, pid, json.dumps(datos, default=repr), time.time()))
        self.db.commit()

    def resultados(self, partido):
        filas = self.db.execute("select pid, datos from resultados where partido=? order by id",
                                (partido,)).fetchall()
        return {pid: json.loads(d) for pid, d in filas}
