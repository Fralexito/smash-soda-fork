# -*- coding: utf-8 -*-
"""Paso 3: choques de identidad de JUGADORES con cara/minicara.
La tabla maestra une por regla «mismo id = mismo jugador», asi que su cruce nunca da
choque. Los sospechosos son las filas tipo_coincidencia = mismo_id_nombre_distinto (711).
Aqui se cruzan con las caras/minicaras que existen en Conmegol y Sudamerican.
Salida: ../choques_identidad_jugadores.csv y se agrega a ../resumen.json
"""
import csv, os, json, collections

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.dirname(AQUI)
MAESTRA = r"D:\Frank\IA\CLAUDE\pes-db\tabla-maestra\maestra_jugadores.csv"
COMP = r"D:\Frank\IA\CLAUDE\pes-db\comparador"
FAM = {"base": "base", "conmegol": "conmegol", "sudamerican": "sudamerican", "sud_wc26": "sudamerican"}

tiene = collections.defaultdict(set)  # (cat, id) -> familias
for cat, f in (("cara", "inventario_caras.csv"), ("minicara", "inventario_minicaras.csv")):
    for r in csv.DictReader(open(os.path.join(OUT, f), encoding="utf-8-sig")):
        tiene[(cat, r["id"])].add(FAM[r["fuente"]])

# nombres en cada base para el mismo id (comparador del hilo de BD, si trae ambos nombres)
filas = []
for r in csv.DictReader(open(MAESTRA, encoding="utf-8-sig")):
    if r["tipo_coincidencia"] != "mismo_id_nombre_distinto":
        continue
    for idc in filter(None, r["id_conmegol"].split(";")):
        if idc not in r["id_sudamerican"].split(";"):
            continue
        c = tiene.get(("cara", idc), set()); m = tiene.get(("minicara", idc), set())
        filas.append({"id": idc, "id_phoenix": r["id_phoenix"], "nombre_maestra": r["nombre"], "nota": r["nota"],
                      "cara_en": "+".join(sorted(c)), "minicara_en": "+".join(sorted(m)),
                      "riesgo": "ALTO: cara en las dos" if {"conmegol", "sudamerican"} <= c else
                                ("medio: minicara en las dos" if {"conmegol", "sudamerican"} <= m else "bajo")})
with open(os.path.join(OUT, "choques_identidad_jugadores.csv"), "w", newline="", encoding="utf-8-sig") as f:
    w = csv.DictWriter(f, ["id", "id_phoenix", "nombre_maestra", "nota", "cara_en", "minicara_en", "riesgo"])
    w.writeheader(); w.writerows(sorted(filas, key=lambda x: (x["riesgo"], x["id"].zfill(8))))
res = json.load(open(os.path.join(OUT, "resumen.json"), encoding="utf-8"))
res["choques"]["identidad_jugadores_revisar"] = dict(collections.Counter(x["riesgo"] for x in filas))
json.dump(res, open(os.path.join(OUT, "resumen.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print(len(filas), res["choques"]["identidad_jugadores_revisar"])
for x in filas[:8]:
    print(x)
