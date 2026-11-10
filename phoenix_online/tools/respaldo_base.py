"""Respaldos de la base de datos del servidor (phoenix_online.db).

    python tools/respaldo_base.py ahora              -> hace una copia ya (sirve con el servidor abierto)
    python tools/respaldo_base.py lista              -> muestra las copias guardadas
    python tools/respaldo_base.py restaurar ARCHIVO  -> vuelve a una copia (SOLO con el servidor CERRADO;
                                                       antes guarda la base actual como *.antes_de_restaurar)

El servidor ya hace una copia al arrancar y cada 6 horas (config «respaldos»).
"""
import glob
import json
import os
import shutil
import sqlite3
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from phoenix_online.datos import Datos  # noqa: E402


def cfg():
    try:
        with open("config.json", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


if __name__ == "__main__":
    c = cfg()
    base = c.get("base_datos", "phoenix_online.db")
    carpeta = (c.get("respaldos") or {}).get("carpeta", "respaldos")
    accion = sys.argv[1] if len(sys.argv) > 1 else "lista"
    if accion == "ahora":
        d = Datos(base)
        print("Copia hecha:", d.respaldar(carpeta, 10_000))
        d.cerrar()
    elif accion == "restaurar" and len(sys.argv) > 2:
        origen = sys.argv[2]
        ok = sqlite3.connect(origen).execute("pragma integrity_check").fetchone()[0]
        if ok != "ok":
            sys.exit(f"La copia {origen} está dañada ({ok}); elige otra.")
        if os.path.exists(base):
            shutil.copy2(base, f"{base}.antes_de_restaurar_{time.strftime('%Y%m%d-%H%M%S')}")
        for extra in ("-wal", "-shm"):
            if os.path.exists(base + extra):
                os.rename(base + extra, f"{base}{extra}.viejo_{int(time.time())}")
        shutil.copy2(origen, base)
        print("Base restaurada desde", origen)
    else:
        for r in sorted(glob.glob(os.path.join(carpeta, "*.db"))):
            print(f"{r}  {os.path.getsize(r) // 1024} KB")
