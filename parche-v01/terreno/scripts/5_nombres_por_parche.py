# -*- coding: utf-8 -*-
"""Paso 5: nombre y equipo de cada id de jugador SEGUN CADA PARCHE (para «Elegir cara»).
Solo lectura. Lee Player.bin (312 B/registro: id u32 en +8, nombre impreso en +251),
PlayerAssignment.bin (16 B: fila, id jugador, id equipo, ...) y Team.bin (1.532 B: id en +8,
nombre en +0x170) del .cpk de base de datos de cada fuente.
Salida: ../nombres_por_parche.csv  (fuente,id,nombre,equipos)
"""
import os, sys, struct, zlib, csv, glob

sys.path.insert(0, r"D:\Frank\IA\CLAUDE\pes-db\panel-duelo")
import cpk_toc as C

OUT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE_CPKS = sorted(glob.glob(r"C:\PES21\eFootball PES 2021\Data\dt*.cpk")) + \
            sorted(glob.glob(r"C:\PES21\eFootball PES 2021\download\dt80_*.cpk"))
FUENTES = {
    "base": BASE_CPKS,  # gana el ultimo que lo traiga (download pisa a Data)
    "conmegol": [r"D:\Frank\Games_\Conmegol Patch\download\CGP_database.cpk"],
    "sudamerican": [r"D:\SP2026\download\SP_Subs.cpk"],
}


def indice(p):
    f = open(p, "rb")
    h = f.read(16)
    hd = C.utf(f.read(struct.unpack("<Q", h[8:16])[0]))[0]
    f.seek(hd["TocOffset"])
    t = f.read(16)
    if t[:4] != b"TOC ":
        f.close()
        return None, {}, 0
    r = C.utf(f.read(struct.unpack("<Q", t[8:16])[0]))
    add = min(hd["TocOffset"], hd.get("ContentOffset") or hd["TocOffset"])
    return f, {(x["DirName"] + "/" + x["FileName"]).lower(): x for x in r}, add


def leer(cpks, nombre):
    res = None
    for p in cpks:
        try:
            f, idx, add = indice(p)
        except Exception:
            continue
        x = idx.get(nombre)
        if x:
            f.seek(x["FileOffset"] + add)
            b = f.read(x["FileSize"])
            if b[3:8] == b"WESYS":
                b = zlib.decompress(b[16:])
            res = b
        if f:
            f.close()
    return res


def cadena(b):
    return b.split(b"\0", 1)[0].decode("utf-8", "replace").strip()


def main():
    w = csv.writer(open(os.path.join(OUT, "nombres_por_parche.csv"), "w", newline="", encoding="utf-8-sig"))
    w.writerow(["fuente", "id", "nombre", "equipos"])
    for fuente, cpks in FUENTES.items():
        pl = leer(cpks, "common/etc/pesdb/player.bin")
        pa = leer(cpks, "common/etc/pesdb/playerassignment.bin") or b""
        te = leer(cpks, "common/etc/pesdb/team.bin") or b""
        if not pl:
            print(fuente, "sin Player.bin")
            continue
        equipos = {}
        for k in range(0, len(te) - 1531, 1532):
            equipos[struct.unpack_from("<I", te, k + 8)[0]] = cadena(te[k + 0x170:k + 0x170 + 70])
        por_jug = {}
        for k in range(0, len(pa) - 15, 16):
            _, jid, tid = struct.unpack_from("<III", pa, k)
            if tid in equipos:
                por_jug.setdefault(jid, []).append(equipos[tid])
        n = 0
        for k in range(0, len(pl) - 311, 312):
            jid = struct.unpack_from("<I", pl, k + 8)[0]  # +0 es otro dato; el id va en +8
            w.writerow([fuente, jid, cadena(pl[k + 251:k + 312]), " / ".join(por_jug.get(jid, []))])
            n += 1
        print(fuente, n, "jugadores,", len(equipos), "equipos")


if __name__ == "__main__":
    main()
