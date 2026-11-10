# -*- coding: utf-8 -*-
"""Paso 2 del terreno: clasifica crudo_rutas.tsv en caras, minicaras, kits, estadios y
balones; agrupa por id; marca choques; escribe CSV + resumen.json (para la vista HTML).
Uso: python -I 2_clasificar.py   (despues de 1_recolectar.py)
"""
import csv, os, re, json, collections, struct

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.dirname(AQUI)
MAESTRA = r"D:\Frank\IA\CLAUDE\pes-db\tabla-maestra"
SIDERS = {
    "base": r"C:\PES21\eFootball PES 2021\Sider_7.4.1",
    "conmegol": r"D:\Frank\Games_\Conmegol Patch\SiderAddons",
    "sudamerican": r"D:\SP2026\Sudamerican Project Sider",
    "sud_wc26": r"D:\SP2026\Sudamerican Project Sider WC26",
}
# fuente "logica" para comparar: la WC26 es una variante del Sudamerican
FAMILIA = {"base": "base", "conmegol": "conmegol", "sudamerican": "sudamerican", "sud_wc26": "sudamerican"}

R = [
    ("cara", re.compile(r"(?:^|/)face/real/(\d+)/")),
    ("minicara", re.compile(r"(?:^|/)render/symbol/player/(\d+)\.dds$")),
    ("kit", re.compile(r"(?:^|/)uniform/texture/(?:[^/]*/)?u(\d{4})([pg]\d)")),
    ("estadio", re.compile(r"(?:^|/)bg/(st\d{3})(?:/|_)")),
    ("estadio", re.compile(r"(?:^|/)stadium/(st\d{3})")),
    ("balon", re.compile(r"(?:^|/)model/ball/(ball\d{3})")),
]
SERVERS = {"kit-server": "kit", "stadium-server": "estadio", "ball-server": "balon"}


def clasificar(ruta):
    p = ruta.lower()
    if p.startswith("miniface_server/"):
        m = R[1][1].search(p)
        return ("minicara", m.group(1), "miniface_server") if m else None
    top = p.split("/", 1)[0]
    if top in SERVERS:
        partes = p.split("/")
        corte = [i for i, x in enumerate(partes) if x in ("asset", "common", "addon", "config.txt", "p1", "p2", "p3",
                                                          "g1", "g2", "g3", "order.ini", "gk_order.ini", "#windx11")]
        fin = corte[0] if corte else len(partes) - 1
        if fin <= 1:
            return None
        return (SERVERS[top], "/".join(partes[1:fin]), top)
    for cat, rx in R:
        m = rx.search(p)
        if m:
            return (cat, (m.group(1).lstrip("0") or "0") if cat == "kit" else m.group(1), "juego")
    return None


def leer_mapa_kits():
    """kit-server/map.txt de cada fuente: carpeta -> ids de equipo."""
    res = {}
    for f, s in SIDERS.items():
        p = os.path.join(s, "content", "kit-server", "map.txt")
        if not os.path.isfile(p):
            continue
        d = collections.defaultdict(list)
        for l in open(p, encoding="utf-8-sig", errors="replace"):
            m = re.match(r'^\s*(\d+)\s*,\s*"([^"]+)"', l)
            if m:
                d[m.group(2).replace("\\", "/").lower()].append(m.group(1))
        res[f] = d
    return res


def maestra():
    jug_con, jug_sud, nom = collections.defaultdict(set), collections.defaultdict(set), {}
    for r in csv.DictReader(open(os.path.join(MAESTRA, "maestra_jugadores.csv"), encoding="utf-8-sig")):
        nom[r["id_phoenix"]] = r["nombre"]
        for x in filter(None, r["id_conmegol"].split(";")):
            jug_con[x.strip()].add(r["id_phoenix"])
        for x in filter(None, r["id_sudamerican"].split(";")):
            jug_sud[x.strip()].add(r["id_phoenix"])
    eq_con, eq_sud = collections.defaultdict(set), collections.defaultdict(set)
    for r in csv.DictReader(open(os.path.join(MAESTRA, "maestra_equipos.csv"), encoding="utf-8-sig")):
        nom[r["id_phoenix"]] = r["nombre"]
        for x in filter(None, r["id_conmegol"].split(";")):
            eq_con[x.strip()].add(r["id_phoenix"])
        for x in filter(None, r["id_sudamerican"].split(";")):
            eq_sud[x.strip()].add(r["id_phoenix"])
    return jug_con, jug_sud, eq_con, eq_sud, nom


def dds_dim(path):
    try:
        with open(path, "rb") as f:
            h = f.read(20)
        if h[:4] == b"DDS ":
            hgt, wid = struct.unpack("<II", h[12:20])
            return "%dx%d" % (wid, hgt)
    except OSError:
        pass
    return ""


def ruta_real(fuente, cont, ruta):
    if not cont.startswith("<sider>") and not re.match(r"^[A-Za-z]:", cont):
        return None
    base = cont.replace("<sider>", SIDERS[fuente])
    if "/content/" in cont.replace("\\", "/"):
        return os.path.join(os.path.dirname(base), ruta)
    return os.path.join(base, ruta)


def main():
    mapa_kits = leer_mapa_kits()
    jug_con, jug_sud, eq_con, eq_sud, nom = maestra()

    grupos = {}  # (cat, id, fuente, cont) -> dict
    por_ruta = collections.defaultdict(list)  # (familia, ruta_logica) -> [(fuente,tipo,cont,activo,prio,bytes)]
    for r in csv.DictReader(open(os.path.join(OUT, "crudo_rutas.tsv"), encoding="utf-8"), delimiter="\t"):
        c = clasificar(r["ruta_logica"])
        if not c:
            continue
        cat, id_, via = c
        k = (cat, id_, r["fuente"], r["contenedor"])
        g = grupos.get(k)
        if not g:
            g = grupos[k] = {"categoria": cat, "id": id_, "fuente": r["fuente"], "via": via, "tipo": r["tipo"],
                             "contenedor": r["contenedor"], "activo": r["activo"], "prioridad": r["prioridad"],
                             "n_archivos": 0, "bytes": 0, "ejemplo": r["ruta_logica"]}
        g["n_archivos"] += 1
        g["bytes"] += int(r["bytes"] or 0)
        if via in ("juego", "miniface_server"):
            rl = r["ruta_logica"].lower()
            if via == "miniface_server":
                rl = rl[rl.find("common/"):]
            por_ruta[rl].append((r["fuente"], r["tipo"], r["contenedor"], r["activo"], r["prioridad"], int(r["bytes"] or 0)))

    # nombres / equipos
    for g in grupos.values():
        g["nombre"] = ""
        if g["categoria"] in ("cara", "minicara"):
            fam = FAMILIA[g["fuente"]]
            src = jug_con if fam == "conmegol" else jug_sud if fam == "sudamerican" else None
            if src is not None:
                g["nombre"] = " / ".join(sorted(nom[x] for x in src.get(g["id"], ())))
        elif g["categoria"] == "kit":
            fam = FAMILIA[g["fuente"]]
            if g["via"] == "kit-server":
                ids = mapa_kits.get(g["fuente"], {}).get(g["id"], [])
                g["nombre"] = "ids " + ";".join(ids) if ids else ""
            else:
                src = eq_con if fam == "conmegol" else eq_sud if fam == "sudamerican" else None
                if src is not None:
                    g["nombre"] = " / ".join(sorted(nom[x] for x in src.get(g["id"], ())))

    campos = ["categoria", "id", "nombre", "fuente", "via", "tipo", "contenedor", "activo", "prioridad",
              "n_archivos", "bytes", "ejemplo"]
    resumen = {"por_categoria": {}, "choques": {}}
    for cat in ("cara", "minicara", "kit", "estadio", "balon"):
        filas = sorted((g for g in grupos.values() if g["categoria"] == cat),
                       key=lambda g: (g["id"].zfill(8), g["fuente"], g["contenedor"]))
        with open(os.path.join(OUT, "inventario_%ss.csv" % cat if not cat.endswith("n") else "inventario_%ses.csv" % cat),
                  "w", newline="", encoding="utf-8-sig") as f:
            w = csv.DictWriter(f, campos, extrasaction="ignore")
            w.writeheader()
            w.writerows(filas)
        cnt = collections.defaultdict(lambda: {"ids": set(), "archivos": 0, "bytes": 0})
        for g in filas:
            c = cnt[FAMILIA[g["fuente"]]]
            c["ids"].add(g["id"]); c["archivos"] += g["n_archivos"]; c["bytes"] += g["bytes"]
        resumen["por_categoria"][cat] = {k: {"ids": len(v["ids"]), "archivos": v["archivos"], "mb": round(v["bytes"] / 1e6)}
                                         for k, v in cnt.items()}

    # ---------- choque 1: el mismo id (misma carpeta logica) en mas de una fuente
    idx = collections.defaultdict(lambda: collections.defaultdict(lambda: [0, 0]))
    for g in grupos.values():
        if g["via"] not in ("juego", "miniface_server"):
            continue
        d = idx[(g["categoria"], g["id"])][FAMILIA[g["fuente"]]]
        d[0] += g["n_archivos"]; d[1] += g["bytes"]
    ch1 = []
    for (cat, id_), fams in idx.items():
        if len(fams) < 2:
            continue
        mayor = max(fams.items(), key=lambda kv: kv[1][1])[0]
        ch1.append({"categoria": cat, "id": id_, "fuentes": "+".join(sorted(fams)),
                    "base": "%d arch / %d KB" % (fams["base"][0], fams["base"][1] // 1024) if "base" in fams else "",
                    "conmegol": "%d arch / %d KB" % (fams["conmegol"][0], fams["conmegol"][1] // 1024) if "conmegol" in fams else "",
                    "sudamerican": "%d arch / %d KB" % (fams["sudamerican"][0], fams["sudamerican"][1] // 1024) if "sudamerican" in fams else "",
                    "mas_pesada": mayor})
    # dimension de minicaras sueltas en choque (cabecera DDS, barato)
    dims_hechas = 0
    for c in ch1:
        if c["categoria"] != "minicara" or dims_hechas > 20000:
            continue
        rl = "common/render/symbol/player/%s.dds" % c["id"]
        for fuente, tipo, cont, *_ in por_ruta.get(rl, []):
            if tipo == "cpk":
                continue
            ruta = rl if not cont.endswith("miniface_server") else None
            p = ruta_real(fuente, cont, rl)
            if "miniface_server" in cont:
                continue
            if p and os.path.isfile(p):
                c["dim_" + FAMILIA[fuente]] = dds_dim(p); dims_hechas += 1
    with open(os.path.join(OUT, "choques_misma_ruta.csv"), "w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, ["categoria", "id", "fuentes", "base", "conmegol", "sudamerican", "mas_pesada",
                               "dim_base", "dim_conmegol", "dim_sudamerican"])
        w.writeheader(); w.writerows(sorted(ch1, key=lambda c: (c["categoria"], c["id"].zfill(8))))
    resumen["choques"]["misma_ruta"] = dict(collections.Counter((c["categoria"], c["fuentes"]) for c in ch1).most_common())
    resumen["choques"]["misma_ruta"] = {"%s | %s" % k: v for k, v in resumen["choques"]["misma_ruta"].items()}

    # ---------- choque 2: mismo id, persona/equipo distinto (tabla maestra)
    ch2 = []
    for (cat, id_), fams in idx.items():
        if "conmegol" not in fams or "sudamerican" not in fams:
            continue
        if cat in ("cara", "minicara"):
            a, b = jug_con.get(id_, set()), jug_sud.get(id_, set())
        elif cat == "kit":
            a, b = eq_con.get(id_, set()), eq_sud.get(id_, set())
        else:
            continue
        if a and b and not (a & b):
            ch2.append({"categoria": cat, "id": id_,
                        "conmegol_es": " / ".join(sorted(nom[x] for x in a)),
                        "sudamerican_es": " / ".join(sorted(nom[x] for x in b)),
                        "id_phoenix_conmegol": ";".join(sorted(a)), "id_phoenix_sudamerican": ";".join(sorted(b))})
    # ids que chocan aunque solo uno traiga cara (para la DB global igual importa)
    ids_ident_jug = sorted((x for x in set(jug_con) & set(jug_sud) if not (jug_con[x] & jug_sud[x])), key=lambda s: s.zfill(8))
    ids_ident_eq = sorted((x for x in set(eq_con) & set(eq_sud) if not (eq_con[x] & eq_sud[x])), key=lambda s: s.zfill(8))
    with open(os.path.join(OUT, "choques_identidad.csv"), "w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, ["categoria", "id", "conmegol_es", "sudamerican_es", "id_phoenix_conmegol", "id_phoenix_sudamerican"])
        w.writeheader(); w.writerows(sorted(ch2, key=lambda c: (c["categoria"], c["id"].zfill(8))))
    resumen["choques"]["identidad"] = dict(collections.Counter(c["categoria"] for c in ch2))
    resumen["choques"]["identidad_ids_maestra"] = {"jugadores": len(ids_ident_jug), "equipos": len(ids_ident_eq)}

    # ---------- choque 3: dentro de una misma fuente, la misma ruta en varios contenedores
    ch3 = collections.Counter(); ej3 = {}
    for rl, lst in por_ruta.items():
        porf = collections.defaultdict(list)
        for x in lst:
            porf[FAMILIA[x[0]]].append(x)
        for fam, xs in porf.items():
            conts = {(x[1], x[2]) for x in xs}
            if len(conts) < 2:
                continue
            act = sorted((int(x[4]) if x[4] != "" else 999, x[2]) for x in xs if x[3] == "si")
            gana = act[0][1] if act else sorted(conts)[-1][1]
            par = (fam, " > ".join(sorted({x[2] for x in xs})[:4]))
            ch3[par] += 1
            ej3.setdefault(par, (rl, gana))
    with open(os.path.join(OUT, "choques_internos.csv"), "w", newline="", encoding="utf-8-sig") as f:
        w = csv.writer(f)
        w.writerow(["fuente", "contenedores_que_traen_la_misma_ruta", "n_rutas", "ejemplo_ruta", "gana_segun_orden"])
        for (fam, conts), n in ch3.most_common():
            w.writerow([fam, conts, n, ej3[(fam, conts)][0], ej3[(fam, conts)][1]])
    resumen["choques"]["internos_top"] = [{"fuente": k[0], "contenedores": k[1], "n": n} for k, n in ch3.most_common(25)]

    # ---------- contenedores (para la propuesta de paquetes)
    porcont = collections.defaultdict(lambda: collections.Counter())
    for g in grupos.values():
        porcont[(g["fuente"], g["tipo"], g["contenedor"], g["activo"], g["prioridad"])][g["categoria"]] += 1
    resumen["contenedores"] = [{"fuente": k[0], "tipo": k[1], "contenedor": k[2], "activo": k[3], "prioridad": k[4],
                                **dict(v)} for k, v in sorted(porcont.items(), key=lambda kv: -sum(kv[1].values()))]
    resumen["errores_lectura"] = [l.rstrip("\n").split("\t") for l in open(os.path.join(OUT, "crudo_contenedores.tsv"), encoding="utf-8")
                                  if "\terror\t" in l or "\tno_existe\t" in l]
    resumen["ejemplos_identidad"] = ch2[:60]
    json.dump(resumen, open(os.path.join(OUT, "resumen.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print(json.dumps({k: resumen[k] for k in ("por_categoria",)}, ensure_ascii=False, indent=1))
    print(json.dumps(resumen["choques"], ensure_ascii=False, indent=1)[:4000])


if __name__ == "__main__":
    main()
