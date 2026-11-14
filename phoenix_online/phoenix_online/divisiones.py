"""Divisiones en línea 1 contra 1: temporada, puntos (SP), rating y ascensos (docs/ESPEC_DIVISIONES_Y_MYCLUB_EXTRA.md, parte A).

Lógica pura y sin red: el servidor (logica.py) guarda el estado de cada persona en la base y
llama aquí para aplicar un resultado, cerrar la temporada o armar las respuestas del menú.
Las reglas por defecto son las de PESBUL (no las reales de Konami, que no se conocen): todo se
puede cambiar en config.json → "divisiones" sin tocar código.
"""
import time

DIVISIONES = [f"D{n}" for n in range(12, 0, -1)]  # D12 (la más baja) .. D1 (la más alta)

# División: (se queda si SP >= , asciende si SP >= ). None = no desciende / no asciende.
UMBRALES = {
    "D12": (None, 300), "D11": (300, 800), "D10": (500, 1200), "D9": (800, 1650),
    "D8": (1000, 2000), "D7": (1500, 2500), "D6": (2000, 3000), "D5": (2500, 3500),
    "D4": (3000, 4000), "D3": (3500, 4500), "D2": (4000, 5000), "D1": (4500, None),
}
PARTIDOS_POR_TEMPORADA = 10
SP_EQUIPO_DEFECTO = 300
MULTIPLICADOR = {"WIN": 1.0, "DRAW": 0.33, "LOSS": 0.0}
RATING_INICIAL, ELO_K, ELO_ESCALA = 1000, 32, 400
SIN_ASCENSO = 99999  # el cliente lo entiende como «deshabilitado»
MAX_HISTORIAL = 20
MAX_RESULTADOS = 20

# L (A.1): estos motivos de fin cuentan como partido normal; cualquier otro = fin anormal
FINES_NORMALES = {"", "NotImplement", "NOERR", "NORMAL_END"}


def redondear(x):
    """Al entero más cercano, 0,5 sube (no el redondeo «del banquero» de Python)."""
    return int(x + 0.5) if x >= 0 else -int(-x + 0.5)


def estado_nuevo():
    return {"division": "D12", "rating": RATING_INICIAL, "temporada": 1, "sp": 0, "jugados": 0,
            "win": 0, "draw": 0, "loss": 0, "racha": 0, "racha_max": 0, "historial": [],
            "carrera": {}, "archivadas": [], "equipos": [], "resultados": [], "goles": 0, "goles_contra": 0}


def completar(estado):
    """Estado leído de la base, con las claves que falten (versiones viejas)."""
    base = estado_nuevo()
    base.update(estado or {})
    return base


class Reglas:
    def __init__(self, cfg=None):
        cfg = cfg or {}
        self.umbrales = {**UMBRALES, **{k: tuple(v) for k, v in (cfg.get("umbrales") or {}).items()}}
        self.partidos = int(cfg.get("partidos_por_temporada", PARTIDOS_POR_TEMPORADA))
        self.sp_equipo = int(cfg.get("sp_equipo_defecto", SP_EQUIPO_DEFECTO))
        self.sp_por_equipo = {int(k): int(v) for k, v in (cfg.get("sp_por_equipo") or {}).items()}
        self.multiplicador = {**MULTIPLICADOR, **(cfg.get("multiplicador") or {})}
        self.k = int(cfg.get("elo_k", ELO_K))
        self.tipos = set(cfg.get("tipos_que_cuentan", ["OC_RANK", "OC_RANKING"]))
        self.abandono = cfg.get("abandono", "no_count")
        # L (A.2): salir con el partido en juego = pierde 3-0 (política de PESBUL, se puede apagar)
        self.abandono_3_0 = bool(cfg.get("abandono_3_0", True))

    def sp_de(self, team_id):
        try:
            return self.sp_por_equipo.get(int(team_id), self.sp_equipo)
        except (TypeError, ValueError):
            return self.sp_equipo

    def descenso(self, division):
        return self.umbrales.get(division, (None, None))[0]

    def ascenso(self, division):
        return self.umbrales.get(division, (None, None))[1]


def resultado_de(propios, rivales):
    return "WIN" if propios > rivales else "LOSS" if propios < rivales else "DRAW"


def elo(rating, rating_rival, resultado, k=ELO_K):
    puntos = {"WIN": 1.0, "DRAW": 0.5, "LOSS": 0.0}[resultado]
    esperado = 1.0 / (1.0 + 10 ** ((rating_rival - rating) / ELO_ESCALA))
    return max(0, redondear(rating + k * (puntos - esperado)))


def temporada_terminada(estado, reglas):
    return estado["jugados"] >= reglas.partidos


def cerrar_temporada(estado, reglas):
    """Archiva la temporada y aplica ascenso/descenso. Devuelve 'UP', 'DOWN' o 'STAY'."""
    div = estado["division"]
    sp = estado["sp"]
    quedarse, subir = reglas.descenso(div), reglas.ascenso(div)
    i = DIVISIONES.index(div) if div in DIVISIONES else 0
    if subir is not None and sp >= subir and i + 1 < len(DIVISIONES):
        cambio, nueva = "UP", DIVISIONES[i + 1]
    elif quedarse is not None and sp < quedarse and i > 0:
        cambio, nueva = "DOWN", DIVISIONES[i - 1]
    else:
        cambio, nueva = "STAY", div
    _anotar_carrera(estado)
    estado["archivadas"] = (estado["archivadas"] + [{
        "temporada": estado["temporada"], "division": div, "sp": sp, "jugados": estado["jugados"],
        "win": estado["win"], "draw": estado["draw"], "loss": estado["loss"],
        "rating": estado["rating"], "cambio": cambio, "hora": int(time.time())}])[-200:]
    estado.update(division=nueva, temporada=estado["temporada"] + 1, sp=0, jugados=0,
                  win=0, draw=0, loss=0, historial=[])  # el rating NO se reinicia
    return cambio


def _anotar_carrera(estado):
    c = estado["carrera"].setdefault(estado["division"], {"point": 0, "rank": 0})
    c["point"] = max(c["point"], estado["sp"])


def aplicar(estado, reglas, propios, rivales, mi_equipo, su_equipo, rating_rival, nombre_rival,
            lado_rival, partido):
    """Un partido terminado para UNA persona. Devuelve (resultado, sp_ganados, rating_viejo)."""
    if temporada_terminada(estado, reglas):
        cerrar_temporada(estado, reglas)  # nunca quedarse atascado con la temporada llena
    resultado = resultado_de(propios, rivales)
    sp = redondear(reglas.sp_de(mi_equipo) * reglas.multiplicador[resultado])
    viejo = estado["rating"]
    estado["rating"] = elo(viejo, rating_rival, resultado, reglas.k)
    estado["sp"] += sp
    estado["jugados"] += 1
    estado[resultado.lower()] += 1
    estado["goles"] += propios
    estado["goles_contra"] += rivales
    estado["racha"] = estado["racha"] + 1 if resultado == "WIN" else 0
    estado["racha_max"] = max(estado["racha_max"], estado["racha"])
    casa = lado_rival == "AWAY"  # yo soy HOME si el rival es AWAY
    estado["historial"] = (estado["historial"] + [{
        "division_point": sp, "enemy_team_id": int(su_equipo or 0),
        "home_score": propios if casa else rivales, "away_score": rivales if casa else propios,
        "enemy_side": lado_rival, "enemy_name": str(nombre_rival or "")[:32],
        "is_finished_match": "YES", "win_lose": resultado, "enemy_cpu_level": ""}])[-MAX_HISTORIAL:]
    if mi_equipo:
        estado["equipos"] = ([int(mi_equipo)] + [e for e in estado["equipos"] if e != int(mi_equipo)])[:5]
    estado["resultados"] = ([partido] + estado["resultados"])[:MAX_RESULTADOS]
    _anotar_carrera(estado)
    return resultado, sp, viejo


# ---------- respuestas para el cliente (anidados como bin: los arma logica.py con Empaquetado) ----------
def info_division(estado, reglas):
    div = estado["division"]
    baja, sube = reglas.descenso(div), reglas.ascenso(div)
    return {
        "division": div,
        "demotion_point": baja or 0,
        "promotion_point": SIN_ASCENSO if sube is None else sube,
        "doublerank_promotion_point": SIN_ASCENSO,
        # 0 = temporada terminada (el cliente muestra «fin de temporada»)
        "other_match_num": 0 if temporada_terminada(estado, reglas) else estado["jugados"] + 1,
        "match_record": {"win_count": estado["win"], "draw_count": estado["draw"],
                         "lose_count": estado["loss"], "win_s_max": estado["racha_max"],
                         "rating": estado["rating"]},
        "match_history_list": list(estado["historial"]),
    }


def filas_carrera(estado):
    """Siempre 12 filas, D12 → D1 (la tabla del cliente es posicional)."""
    filas = []
    for d in DIVISIONES:
        c = estado["carrera"].get(d) or {}
        punto = c.get("point", 0)
        if d == estado["division"]:
            punto = max(punto, estado["sp"])
        filas.append({"division_type": d, "point": int(punto), "rank": int(c.get("rank", 0))})
    return filas


def partido_guardable(fecha, casa, fuera, motivo="NORMAL_END"):
    """Lo que se guarda de un partido para CMD_GET_GAMERESULTS. casa/fuera: {pid, name, team, score}."""
    return {"date": int(fecha), "reason": motivo if motivo not in FINES_NORMALES else "NORMAL_END",
            "home": casa, "away": fuera}
