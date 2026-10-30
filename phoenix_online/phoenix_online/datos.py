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
        v = (self.vinculos or {}).get(steam_hex.lower())
        if v and v.get("pname"):
            pname = v["pname"]  # el nombre manda la web Phoenix
        cur = self.db.execute("insert into jugadores(steam_hex, pname, creado) values (?,?,?)",
                              (steam_hex.lower(), pname, time.time()))
        self.db.commit()
        return {"pid": cur.lastrowid, "pname": pname}

    def guardar_resultado(self, room_id, partido, pid, datos):
        self.db.execute("insert into resultados(room_id, partido, pid, datos, hora) values (?,?,?,?,?)",
                        (room_id, partido, pid, json.dumps(datos, default=repr), time.time()))
        self.db.commit()

    def resultados(self, partido):
        filas = self.db.execute("select pid, datos from resultados where partido=? order by id",
                                (partido,)).fetchall()
        return {pid: json.loads(d) for pid, d in filas}
