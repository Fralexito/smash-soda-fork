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
create table if not exists presentaciones (
  ip text primary key,
  usuario text not null,
  hora real not null
);
create table if not exists vinculos_web (
  clave text primary key,
  codigo text unique not null,
  usuario text,
  subido integer not null default 0,
  hora real not null
);
create table if not exists divisiones (
  steam_hex text primary key,
  datos text not null
);
create table if not exists partidos_division (
  partido text primary key,
  datos text not null,
  hora real not null
);
create table if not exists coop_datos (
    steam_hex text primary key,
    datos text not null
);
create table if not exists relaciones (
  steam_hex text primary key,
  datos text not null
);
create table if not exists clanes (
  dueno text primary key,
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
        self.div_cambiadas = set()  # estados de Divisiones que cambiaron (para la web)

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

    def claves_de_clubes(self):
        """Todas las claves con club guardado (para la primera subida completa a la web)."""
        return [f[0] for f in self.db.execute("select steam_hex from myclub")]

    # ---- seguidos y rechazados (Cooperativo): {"sigo": [], "bloqueados": [], "rechazados": []} ----
    def relaciones(self, steam_hex):
        f = self.db.execute("select datos from relaciones where steam_hex=?",
                            ((steam_hex or "").lower(),)).fetchone()
        d = json.loads(f[0]) if f else {}
        return {k: list(d.get(k) or []) for k in ("sigo", "bloqueados", "rechazados", "no_emparejar")}

    def seguidores(self, steam_hex):
        """Cuántos jugadores siguen a esta clave (ficha de Cooperativo: follower)."""
        clave = (steam_hex or "").lower()
        return sum(1 for (datos,) in self.db.execute("select datos from relaciones")
                   if clave in (json.loads(datos).get("sigo") or []))

    def guardar_relaciones(self, steam_hex, d):
        self.db.execute("insert into relaciones(steam_hex, datos) values (?,?) on conflict(steam_hex) "
                        "do update set datos=excluded.datos", ((steam_hex or "").lower(), json.dumps(d)))
        self.db.commit()

    # ---- tablas de Cooperativo que sube el PES (SET_COOP_MATCH_DATA): {"size": n, "data": "base64"} ----
    def coop_datos(self, steam_hex):
        f = self.db.execute("select datos from coop_datos where steam_hex=?",
                            ((steam_hex or "").lower(),)).fetchone()
        return json.loads(f[0]) if f else {}

    def guardar_coop_datos(self, steam_hex, d):
        self.db.execute("insert into coop_datos(steam_hex, datos) values (?,?) on conflict(steam_hex) "
                        "do update set datos=excluded.datos", ((steam_hex or "").lower(), json.dumps(d)))
        self.db.commit()

    # ---- clanes (Cooperativo): uno por dueño; miembros = claves de jugador ----
    def clan(self, dueno):
        f = self.db.execute("select datos from clanes where dueno=?", ((dueno or "").lower(),)).fetchone()
        return json.loads(f[0]) if f else None

    def guardar_clan(self, dueno, clan):
        self.db.execute("insert into clanes(dueno, datos) values (?,?) on conflict(dueno) "
                        "do update set datos=excluded.datos", ((dueno or "").lower(), json.dumps(clan)))
        self.db.commit()

    def disolver_clan(self, dueno):
        """El dueño deja su clan: se guarda aparte como disuelto (no se pierde el registro)."""
        c = self.clan(dueno)
        if c is not None:
            self.db.execute("delete from clanes where dueno=?", ((dueno or "").lower(),))
            self.db.execute("insert or replace into clanes(dueno, datos) values (?,?)",
                            (f"disuelto:{(dueno or '').lower()}:{int(time.time())}",
                             json.dumps({**c, "disuelto": True})))
            self.db.commit()

    def clanes(self):
        """[(dueño, clan)] de los clanes vivos, del más antiguo al más nuevo."""
        return [(f[0], json.loads(f[1])) for f in
                self.db.execute("select dueno, datos from clanes where dueno not like 'disuelto:%' "
                                "order by rowid")]

    # ---- pase de identidad (web): IP de la persona -> su cuenta Phoenix ----
    def guardar_pase(self, ip, usuario, clave):
        self.db.execute("insert into pases(ip, usuario, clave, hora) values (?,?,?,?) on conflict(ip) "
                        "do update set usuario=excluded.usuario, clave=excluded.clave, hora=excluded.hora",
                        (ip, str(usuario), clave.lower(), time.time()))
        self.db.commit()

    def presentar(self, ip, usuario):
        """El launcher abrió el PES de esta IP con esta cuenta (para exigir_launcher)."""
        self.db.execute("insert into presentaciones(ip, usuario, hora) values (?,?,?) on conflict(ip) "
                        "do update set usuario=excluded.usuario, hora=excluded.hora", (ip, str(usuario), time.time()))
        self.db.commit()

    def presentado(self, ip, horas):
        f = self.db.execute("select hora from presentaciones where ip=?", (ip,)).fetchone()
        return bool(f and time.time() - f[0] < horas * 3600)

    def clave_por_ip(self, ip):
        f = self.db.execute("select clave from pases where ip=?", (ip,)).fetchone()
        return f[0] if f else None

    def usuario_de_clave(self, clave):
        clave = (clave or "").lower()
        f = self.db.execute("select usuario from vinculos_web where clave=? and usuario is not null",
                            (clave,)).fetchone()
        if f:
            return f[0]
        f = self.db.execute("select usuario from pases where clave=? order by hora desc limit 1",
                            (clave,)).fetchone()
        return f[0] if f else None

    # ---- vincular desde la web: código de 6 cifras que la persona ve en los Avisos del juego ----
    VIDA_CODIGO = 30 * 86400

    def codigo_web(self, clave):
        """Código para «Vincular mi PES» en la web; None si esa clave ya está vinculada."""
        import secrets
        clave = (clave or "").lower()
        f = self.db.execute("select codigo, usuario, hora from vinculos_web where clave=?", (clave,)).fetchone()
        if f and f[1]:
            return None
        if f and time.time() - f[2] < self.VIDA_CODIGO - 86400:
            return f[0]
        for _ in range(50):
            codigo = f"{secrets.randbelow(900000) + 100000}"
            if not self.db.execute("select 1 from vinculos_web where codigo=?", (codigo,)).fetchone():
                break
        self.db.execute("insert into vinculos_web(clave, codigo, usuario, subido, hora) values (?,?,null,0,?) "
                        "on conflict(clave) do update set codigo=excluded.codigo, subido=0, hora=excluded.hora",
                        (clave, codigo, time.time()))
        self.db.commit()
        return codigo

    def codigos_pendientes(self):
        """[(clave, codigo, subido, hora)] de los que aún no tienen cuenta."""
        return self.db.execute("select clave, codigo, subido, hora from vinculos_web where usuario is null").fetchall()

    def marcar_codigos_subidos(self, claves):
        self.db.executemany("update vinculos_web set subido=1 where clave=?", [(c,) for c in claves])
        self.db.commit()

    def vincular(self, clave, usuario):
        self.db.execute("update vinculos_web set usuario=? where clave=?", (str(usuario), (clave or "").lower()))
        self.db.commit()
        self.cambiados.add((clave or "").lower())  # el club se vuelve a subir, ya con su dueño

    def guardar_resultado(self, room_id, partido, pid, datos):
        self.db.execute("insert into resultados(room_id, partido, pid, datos, hora) values (?,?,?,?,?)",
                        (room_id, partido, pid, json.dumps(datos, default=repr), time.time()))
        self.db.commit()

    # ---- Divisiones en línea (docs/ESPEC_DIVISIONES_Y_MYCLUB_EXTRA.md, parte A) ----
    def division(self, steam_hex):
        f = self.db.execute("select datos from divisiones where steam_hex=?",
                            ((steam_hex or "").lower(),)).fetchone()
        return json.loads(f[0]) if f else None

    def guardar_division(self, steam_hex, estado):
        self.db.execute("insert into divisiones(steam_hex, datos) values (?,?) on conflict(steam_hex) "
                        "do update set datos=excluded.datos", ((steam_hex or "").lower(), json.dumps(estado)))
        self.db.commit()
        self.div_cambiadas.add((steam_hex or "").lower())

    def divisiones(self):
        return {k: json.loads(d) for k, d in self.db.execute("select steam_hex, datos from divisiones")}

    def guardar_partido_division(self, partido, datos):
        """Un partido de Divisiones ya aplicado. False si ya estaba (nunca se aplica dos veces)."""
        try:
            self.db.execute("insert into partidos_division(partido, datos, hora) values (?,?,?)",
                            (partido, json.dumps(datos), time.time()))
        except sqlite3.IntegrityError:
            return False
        self.db.commit()
        return True

    def partido_division(self, partido):
        f = self.db.execute("select datos from partidos_division where partido=?", (partido,)).fetchone()
        return json.loads(f[0]) if f else None

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
