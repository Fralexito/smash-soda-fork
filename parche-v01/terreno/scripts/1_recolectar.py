# -*- coding: utf-8 -*-
"""Paso 1 del terreno: lista (SOLO LECTURA) las rutas de caras, minicaras, kits,
estadios y balones de las tres fuentes: PES base, Conmegol y Sudamerican.

- .cpk: se lee solo el indice (TOC) con el lector de pes-db/panel-duelo/cpk_toc.py.
- carpetas livecpk: las de cada cpk.root del sider.ini (activas y comentadas, con su
  orden) + las subcarpetas de livecpk que no estan en el ini ("no_listado").
- content\\*-server de Sider (kit-server, stadium-server, ball-server, miniface...).

Salida: ../crudo_rutas.tsv  (fuente, tipo, contenedor, activo, prioridad, ruta_logica, bytes)
        ../crudo_contenedores.tsv (resumen por contenedor, incluidos los que fallan)
Uso:  python -I 1_recolectar.py
"""
import os, re, sys, struct, time

AQUI = os.path.dirname(os.path.abspath(__file__))
SALIDA = os.path.dirname(AQUI)
sys.path.insert(0, r"D:\Frank\IA\CLAUDE\pes-db\panel-duelo")
import cpk_toc  # utf() descifra y lee tablas @UTF

FUENTES = [
    ("base", r"C:\PES21\eFootball PES 2021", r"C:\PES21\eFootball PES 2021\Sider_7.4.1"),
    ("conmegol", r"D:\Frank\Games_\Conmegol Patch", r"D:\Frank\Games_\Conmegol Patch\SiderAddons"),
    ("sudamerican", r"D:\SP2026", r"D:\SP2026\Sudamerican Project Sider"),
    ("sud_wc26", None, r"D:\SP2026\Sudamerican Project Sider WC26"),
]
CLAVE = re.compile(r"face/|symbol/player|uniform|kit|stadium|/bg/|^bg/|ball|miniface", re.I)
RX_ROOT = re.compile(r'^\s*(;?)\s*cpk\.root\s*=\s*"([^"]*)"')


def toc(path):
    with open(path, "rb") as f:
        h = f.read(16)
        if h[:4] != b"CPK ":
            raise ValueError("sin cabecera CPK")
        hd = cpk_toc.utf(f.read(struct.unpack("<Q", h[8:16])[0]))
        if not hd:
            raise ValueError("cabecera ilegible")
        off = hd[0].get("TocOffset")
        if not off:
            raise ValueError("sin TocOffset")
        f.seek(off)
        t = f.read(16)
        if t[:4] != b"TOC ":
            raise ValueError("sin TOC")
        rows = cpk_toc.utf(f.read(struct.unpack("<Q", t[8:16])[0]))
        if rows is None:
            raise ValueError("TOC ilegible")
    for r in rows:
        d, n = r.get("DirName") or "", r.get("FileName") or ""
        yield (d + "/" + n if d else n), r.get("ExtractSize") or r.get("FileSize") or 0


def caminar(raiz):
    pila = [(raiz, "")]
    while pila:
        d, rel = pila.pop()
        try:
            it = list(os.scandir(d))
        except OSError:
            continue
        for e in it:
            r = rel + "/" + e.name if rel else e.name
            try:
                if e.is_dir(follow_symlinks=False):
                    pila.append((e.path, r))
                else:
                    yield r, e.stat().st_size
            except OSError:
                pass


def raices_ini(sider):
    out = []
    ini = os.path.join(sider, "sider.ini")
    if not os.path.isfile(ini):
        return out
    n = 0
    for l in open(ini, encoding="utf-8", errors="replace"):
        m = RX_ROOT.match(l)
        if not m:
            continue
        p = m.group(2).strip()
        abs_ = p if re.match(r"^[A-Za-z]:", p) else os.path.normpath(os.path.join(sider, p))
        out.append((abs_, m.group(1) == "", n))
        n += 1
    return out


def main():
    t0 = time.time()
    fo = open(os.path.join(SALIDA, "crudo_rutas.tsv"), "w", encoding="utf-8")
    fc = open(os.path.join(SALIDA, "crudo_contenedores.tsv"), "w", encoding="utf-8")
    fo.write("fuente\ttipo\tcontenedor\tactivo\tprioridad\truta_logica\tbytes\n")
    fc.write("fuente\ttipo\tcontenedor\testado\tarchivos_total\tarchivos_interes\tnota\n")
    base_cpk = {}

    def volcar(fuente, tipo, cont, activo, prio, gen):
        tot = interes = 0
        for ruta, b in gen:
            tot += 1
            ruta = ruta.replace("\\", "/").lstrip("./")
            if CLAVE.search(ruta):
                interes += 1
                fo.write("%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (fuente, tipo, cont, activo, prio, ruta, b))
        return tot, interes

    for fuente, juego, sider in FUENTES:
        # 1) cpk de Data y download
        if juego:
            for sub in ("Data", "download"):
                d = os.path.join(juego, sub)
                if not os.path.isdir(d):
                    continue
                for n in sorted(os.listdir(d)):
                    if not n.lower().endswith(".cpk"):
                        continue
                    p = os.path.join(d, n)
                    sz = os.path.getsize(p)
                    clave = (sub, n.lower(), sz)
                    if fuente == "base":
                        base_cpk[clave] = True
                    elif clave in base_cpk:
                        fc.write("%s\tcpk\t%s/%s\tigual_base\t\t\tmismo nombre y tamano que el base: no se repite\n" % (fuente, sub, n))
                        continue
                    try:
                        tot, it = volcar(fuente, "cpk", sub + "/" + n, "", "", toc(p))
                        fc.write("%s\tcpk\t%s/%s\tok\t%d\t%d\t\n" % (fuente, sub, n, tot, it))
                    except Exception as e:
                        fc.write("%s\tcpk\t%s/%s\terror\t\t\t%r\n" % (fuente, sub, n, e))
                    fo.flush()
        if not sider or not os.path.isdir(sider):
            fc.write("%s\tsider\t%s\tno_existe\t\t\t\n" % (fuente, sider))
            continue
        # 2) livecpk del sider.ini (orden = prioridad, 0 = gana)
        vistos = set()
        for abs_, activo, prio in raices_ini(sider):
            key = os.path.normcase(abs_)
            if key in vistos:
                continue
            vistos.add(key)
            cont = abs_.replace(sider, "<sider>") if abs_.startswith(sider) else abs_
            if not os.path.isdir(abs_):
                fc.write("%s\tlivecpk\t%s\tno_existe\t\t\tactivo=%s prio=%d\n" % (fuente, cont, activo, prio))
                continue
            tot, it = volcar(fuente, "livecpk", cont, "si" if activo else "no", prio, caminar(abs_))
            fc.write("%s\tlivecpk\t%s\tok\t%d\t%d\tactivo=%s prio=%d\n" % (fuente, cont, tot, it, activo, prio))
            fo.flush()
        # 3) subcarpetas de livecpk que no estan en el ini
        for lc in (os.path.join(sider, "livecpk"), os.path.join(sider, "content", "livecpk")):
            if not os.path.isdir(lc):
                continue
            for e in os.scandir(lc):
                if not e.is_dir():
                    continue
                key = os.path.normcase(e.path)
                if any(v.startswith(key) for v in vistos):
                    continue
                cont = e.path.replace(sider, "<sider>")
                tot, it = volcar(fuente, "livecpk", cont, "no_listado", "", caminar(e.path))
                fc.write("%s\tlivecpk\t%s\tno_listado\t%d\t%d\t\n" % (fuente, cont, tot, it))
        # 4) content\* de Sider (servidores de modulos)
        cd = os.path.join(sider, "content")
        if os.path.isdir(cd):
            for e in os.scandir(cd):
                if not e.is_dir() or e.name.lower() == "livecpk":
                    continue
                tot, it = volcar(fuente, "content", "<sider>/content/" + e.name, "", "",
                                 ((e.name + "/" + r, b) for r, b in caminar(e.path)))
                if it:
                    fc.write("%s\tcontent\t<sider>/content/%s\tok\t%d\t%d\t\n" % (fuente, e.name, tot, it))
        print(fuente, "listo", round(time.time() - t0), "s", flush=True)
    fo.close(); fc.close()


if __name__ == "__main__":
    main()
