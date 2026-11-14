"""Arma el catálogo de jugadores de myClub desde la base de datos del PROPIO juego.

Uso (en la PC, con los .bin ya sacados del cpk):
    python -I tools/catalogo_pesdb.py PlayerAssignment.bin [--player Player.bin] [--extra extra.json] -o catalogo.json

- PlayerAssignment.bin (dt10_x64.cpk → common/etc/pesdb/): cabecera con «WESYS» y luego zlib.
  Filas de 16 bytes = 4 × u32 LE: [fila][player_id][team_id][puesto<<10 | dorsal].
- --extra (opcional): JSON {"<player_id>": {"rating": 88, "nation_id": 7, ...}} con datos que
  aún no sabemos leer de Player.bin (media, nación). Lo que falte usa los valores por defecto
  del servidor (media 75).

El catálogo SALE de los datos del juego del usuario: queda solo en su PC (no se sube a GitHub,
igual que config.json). El servidor lo lee con config myclub.catalogo = "ruta/catalogo.json".
"""
import argparse
import json
import struct
import sys
import zlib


def descomprimir_wesys(crudo):
    """Busca el inicio del zlib detrás de la cabecera WESYS (C: «FF 10 81 WESYS» + zlib)."""
    inicio = crudo.find(b"WESYS")
    desde = inicio + 5 if inicio >= 0 else 0
    for i in range(desde, min(desde + 64, len(crudo) - 1)):
        if crudo[i] == 0x78:
            try:
                return zlib.decompress(crudo[i:])
            except zlib.error:
                continue
    raise ValueError("no encontré el bloque zlib (¿es PlayerAssignment.bin?)")


def leer_asignaciones(datos):
    """Devuelve {team_id: [{gameplayer_id, puesto, uniform_number}, ...]} en el orden del archivo
    (C, 05-nov: ese orden sirve como XI + banquillo, Messi = hueco 8 en el Barcelona)."""
    equipos = {}
    for off in range(0, len(datos) - 15, 16):
        _fila, jugador, equipo, mezcla = struct.unpack_from("<4I", datos, off)
        if not jugador or not equipo:
            continue
        equipos.setdefault(equipo, []).append(
            {"gameplayer_id": jugador, "puesto": mezcla >> 10, "uniform_number": mezcla & 0x3FF})
    return equipos


POSICIONES = ["PT", "DFC", "LI", "LD", "MCD", "MC", "MDI", "MDD", "MO", "EI", "ED", "SD", "DC"]

# C (RC 06-nov, RE_PLAYERBIN_HABILIDADES.md): cada habilidad = 6 bits = valor - 40, comprobado
# con 5 jugadores del PES base.
HABILIDADES = {
    "act_ofensiva": 306, "control": 217, "drible": 288, "posesion": 352, "pase_ras": 199,
    "pase_bombeado": 338, "finalizacion": 332, "cabeceo": 224, "balon_parado": 186,
    "efecto": 268, "act_defensiva": 211, "recuperacion": 248, "agresividad": 320,
    "act_portero": 262, "atajar": 300, "despejar": 205, "reflejos": 256, "cobertura": 236,
    "velocidad": 242, "aceleracion": 280, "potencia_tiro": 294, "salto": 230,
    "contacto": 326, "equilibrio": 312, "resistencia": 274,
}

# P (06-nov): la media real de PES es una suma ponderada por posición que no se guarda y aún
# no se leyó del exe. Mientras tanto: promedio de las habilidades clave de cada grupo de
# puestos + un ajuste por grupo, calibrado con Messi (94), Piqué (87) y Ter Stegen (89).
CLAVE_POR_GRUPO = {
    "PT": (["act_portero", "act_portero", "atajar", "despejar", "reflejos", "cobertura",
            "salto"], -5),
    "DEF": (["act_defensiva", "recuperacion", "agresividad", "cabeceo", "salto", "contacto",
             "velocidad", "pase_ras"], 2),
    "MED": (["pase_ras", "pase_bombeado", "control", "posesion", "act_ofensiva",
             "recuperacion", "resistencia", "drible"], 0),
    "ATA": (["act_ofensiva", "control", "drible", "posesion", "finalizacion", "pase_ras",
             "aceleracion", "equilibrio", "efecto"], 0),
}
GRUPO_DE_PUESTO = {0: "PT", 1: "DEF", 2: "DEF", 3: "DEF", 4: "MED", 5: "MED", 6: "MED",
                   7: "MED", 8: "MED", 9: "ATA", 10: "ATA", 11: "ATA", 12: "ATA"}



# C (14-nov, RE FUN_1414c6e90 + registros vivos del PES): el cliente usa 108 «parameters» por jugador
# (idx 0..107). Las 25 habilidades son los idx de PARAM108_HABILIDAD (valor 40..99, igual que
# HABILIDADES pero guardado directo). El resto sale de estos bits del registro de Player.bin
# (idx -> (bit_inicio, ancho)), hallados cruzando 26 jugadores reales (coincidencia exacta o con
# 1 solo jugador discrepante). Aptitudes de posición idx 1-13: 1 DC, 2 SD, 3 EI, 4 ED, 5 MO,
# 6 MCD, 7 MC, 8 MDI, 9 MDD, 10 DFC, 11 LI, 12 LD, 13 PT (valor 2 = posición registrada).
# idx 60-107 = marcas de 1 bit (habilidades especiales/estilos). Sin resolver (van a 0):
# 43, 45-48, 59, 68, 74, 83-87, 90, 95, 97, 102, 104, 106.
PARAM108_HABILIDAD = {
    14: "act_ofensiva", 15: "act_defensiva", 16: "act_portero", 17: "drible", 18: "control",
    19: "posesion", 20: "finalizacion", 21: "pase_ras", 22: "pase_bombeado", 23: "cabeceo",
    24: "recuperacion", 25: "agresividad", 26: "balon_parado", 27: "efecto", 28: "atajar",
    29: "despejar", 30: "reflejos", 31: "cobertura", 33: "velocidad", 34: "contacto",
    35: "equilibrio", 36: "potencia_tiro", 37: "aceleracion", 38: "salto", 39: "resistencia"}
PARAM108_BITS = {
    0: (370, 4), 1: (406, 2), 2: (414, 2), 3: (408, 2), 4: (412, 2), 5: (400, 2), 6: (350, 2),
    7: (392, 2), 8: (402, 2), 9: (396, 2), 10: (404, 2), 11: (254, 2), 12: (410, 2),
    13: (286, 2), 32: (398, 2), 40: (374, 3), 41: (394, 2), 42: (450, 1), 44: (390, 2),
    49: (318, 2), 50: (416, 2), 51: (377, 3), 52: (362, 4), 53: (384, 3), 54: (366, 4),
    55: (59, 5), 56: (380, 3), 57: (358, 4), 58: (178, 8), 60: (425, 1), 61: (467, 1),
    62: (462, 1), 63: (446, 1), 64: (465, 1), 65: (383, 1), 66: (456, 1), 67: (455, 1),
    69: (433, 1), 70: (418, 1), 71: (464, 1), 72: (461, 1), 73: (431, 1), 75: (449, 1),
    76: (460, 1), 77: (441, 1), 78: (447, 1), 79: (443, 1), 80: (420, 1), 81: (419, 1),
    82: (429, 1), 88: (427, 1), 89: (440, 1), 91: (466, 1), 92: (428, 1), 93: (452, 1),
    94: (422, 1), 96: (437, 1), 98: (442, 1), 99: (436, 1), 100: (430, 1), 101: (435, 1),
    103: (439, 1), 105: (223, 1), 107: (423, 1)}


def parametros_108(v, hab):
    """Los 108 «parameters» del cliente para un registro (v = entero de 512 bits) y sus 25
    habilidades (dict con valores 40..99)."""
    p = [0] * 108
    for i, (bit, ancho) in PARAM108_BITS.items():
        p[i] = (v >> bit) & ((1 << ancho) - 1)
    for i, k in PARAM108_HABILIDAD.items():
        p[i] = hab[k]
    return p


def estimar_media(hab, puesto):
    claves, ajuste = CLAVE_POR_GRUPO[GRUPO_DE_PUESTO.get(puesto, "MED")]
    valor = sum(hab[k] for k in claves) / len(claves) + ajuste
    return max(40, min(99, round(valor)))


def leer_fichas(datos):
    """C (05-nov): los primeros 64 B del registro como entero LE de 512 bits.
    Posición registrada = bits 370-373 (0 PT … 12 DC, ver POSICIONES; 6, 7 y 11 por orden
    estándar, sin ejemplo visto). Nación = bits 169-175 (índice de nación del juego, coincide
    con la selección nacional en el 89 %). Habilidades: HABILIDADES (6 bits, valor - 40).
    La media no está guardada: se estima con estimar_media (P) y queda rating_estimado = True.
    Un --extra con «rating» la reemplaza."""
    fichas = {}
    for off in range(8, len(datos) - 311, 312):
        v = int.from_bytes(datos[off:off + 64], "little")
        pid = v & 0xFFFFFFFF
        if pid:
            pos = (v >> 370) & 15
            hab = {k: ((v >> b) & 63) + 40 for k, b in HABILIDADES.items()}
            fichas[str(pid)] = {"posicion": pos, "posicion_txt": POSICIONES[pos] if pos < 13 else "",
                                "nacion": (v >> 169) & 127, "habilidades": hab,
                                "rating": estimar_media(hab, pos), "rating_estimado": True,
                                "parametros": parametros_108(v, hab)}
    return fichas


def leer_nombres(datos):
    """C (05-nov, Player.bin del PES base, ya descomprimido): registros de 312 B desde el
    offset 8; id = u32 en +0; nombre de camiseta ASCII en +243 (61 B, p. ej. «L. MESSI»).
    Los bytes +4..+59 son campos en bits sin documentar (la media no está guardada)."""
    nombres = {}
    for off in range(8, len(datos) - 311, 312):
        (pid,) = struct.unpack_from("<I", datos, off)
        crudo = datos[off + 243:off + 304].split(b"\0", 1)[0]
        if pid and crudo:
            nombres[str(pid)] = crudo.decode("latin-1")
    return nombres


def armar(asignaciones, extra=None, nombres=None, fichas=None):
    extra = extra or {}
    fichas = fichas or {}
    jugadores = {}
    for equipo, filas in asignaciones.items():
        for f in filas:
            j = jugadores.setdefault(str(f["gameplayer_id"]), {"team_id": equipo})
            if nombres and str(f["gameplayer_id"]) in nombres:
                j["nombre"] = nombres[str(f["gameplayer_id"])]
            j.update(fichas.get(str(f["gameplayer_id"]), {}))
            j.update(extra.get(str(f["gameplayer_id"]), {}))
    return {"version": 1,
            "equipos": {str(e): f for e, f in sorted(asignaciones.items())},
            "jugadores": jugadores}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("assignment")
    ap.add_argument("--player", help="Player.bin para añadir los nombres")
    ap.add_argument("--extra")
    ap.add_argument("-o", "--salida", default="catalogo.json")
    a = ap.parse_args(argv)
    with open(a.assignment, "rb") as f:
        asign = leer_asignaciones(descomprimir_wesys(f.read()))
    extra = {}
    if a.extra:
        with open(a.extra, encoding="utf-8") as f:
            extra = json.load(f)
    nombres = fichas = None
    if a.player:
        with open(a.player, "rb") as f:
            crudo = descomprimir_wesys(f.read())
        nombres, fichas = leer_nombres(crudo), leer_fichas(crudo)
    cat = armar(asign, extra, nombres, fichas)
    with open(a.salida, "w", encoding="utf-8") as f:
        json.dump(cat, f, ensure_ascii=False)
    print(f"{len(cat['equipos'])} equipos, {len(cat['jugadores'])} jugadores -> {a.salida}")


if __name__ == "__main__":
    sys.exit(main())
