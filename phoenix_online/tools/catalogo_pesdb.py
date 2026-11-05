"""Arma el catálogo de jugadores de myClub desde la base de datos del PROPIO juego.

Uso (en la PC, con los .bin ya sacados del cpk):
    python -I tools/catalogo_pesdb.py PlayerAssignment.bin [--extra extra.json] -o catalogo.json

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


def armar(asignaciones, extra=None):
    extra = extra or {}
    jugadores = {}
    for equipo, filas in asignaciones.items():
        for f in filas:
            j = jugadores.setdefault(str(f["gameplayer_id"]), {"team_id": equipo})
            j.update(extra.get(str(f["gameplayer_id"]), {}))
    return {"version": 1,
            "equipos": {str(e): f for e, f in sorted(asignaciones.items())},
            "jugadores": jugadores}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("assignment")
    ap.add_argument("--extra")
    ap.add_argument("-o", "--salida", default="catalogo.json")
    a = ap.parse_args(argv)
    with open(a.assignment, "rb") as f:
        asign = leer_asignaciones(descomprimir_wesys(f.read()))
    extra = {}
    if a.extra:
        with open(a.extra, encoding="utf-8") as f:
            extra = json.load(f)
    cat = armar(asign, extra)
    with open(a.salida, "w", encoding="utf-8") as f:
        json.dump(cat, f, ensure_ascii=False)
    print(f"{len(cat['equipos'])} equipos, {len(cat['jugadores'])} jugadores → {a.salida}")


if __name__ == "__main__":
    sys.exit(main())
