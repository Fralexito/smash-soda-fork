"""Lee (sin ejecutar) el código fuente del servidor de AzrakHQ (MIT) con ast y saca, por comando, las
claves que devuelve en el primer nivel y su tipo (bin / lista / texto / num). Luego corre NUESTRO
servidor con un pedido mínimo y marca claves que faltan o tienen otro tipo.

Uso: python tools/formas_azrak_estatico.py <carpeta pesserver> > docs/COMPARACION_TIPOS_AZRAK.md
"""
import ast, asyncio, glob, logging, os, sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(AQUI))
sys.path.insert(0, os.path.join(os.path.dirname(AQUI), "tests"))
logging.disable(logging.CRITICAL)
from phoenix_online.datos import Datos  # noqa: E402
from phoenix_online.logica import Servidor, VIGILANCIAS  # noqa: E402
from phoenix_online.msgpack_min import Empaquetado  # noqa: E402
import test_phoenix_online as T  # noqa: E402

BIN = {"packb", "_blob", "blob", "_pname", "_member", "_group", "pname_blob", "_pack"}


def tipo_nodo(n):
    if isinstance(n, ast.Constant):
        return "texto" if isinstance(n.value, str) else "num" if isinstance(n.value, (int, float)) else "?"
    if isinstance(n, (ast.List, ast.ListComp)):
        return "lista"
    if isinstance(n, ast.Call):
        f = n.func.attr if isinstance(n.func, ast.Attribute) else getattr(n.func, "id", "")
        if f in BIN or f.endswith("_blob") or f.startswith("_pname"):
            return "bin"
        if f in ("blobs", "list", "sorted"):
            return "lista"
        if f in ("int", "len"):
            return "num"
        if f in ("str", "_yes"):
            return "texto"
    if isinstance(n, ast.IfExp):
        a, b = tipo_nodo(n.body), tipo_nodo(n.orelse)
        return a if a == b else "?"
    return "?"


def formas_suyas(raiz):
    out = {}
    for ruta in glob.glob(os.path.join(raiz, "*.py")):
        arbol = ast.parse(open(ruta, encoding="utf-8").read())
        for f in ast.walk(arbol):
            if not isinstance(f, ast.FunctionDef):
                continue
            nombres = [d.args[0].value for d in f.decorator_list
                       if isinstance(d, ast.Call) and d.args and isinstance(d.args[0], ast.Constant)
                       and str(d.args[0].value).startswith("CMD_")]
            if not nombres:
                continue
            claves = {}
            for r in ast.walk(f):
                if isinstance(r, ast.Return) and isinstance(r.value, ast.Dict):
                    for k, v in zip(r.value.keys, r.value.values):
                        if isinstance(k, ast.Constant) and k.value != "result":
                            t = tipo_nodo(v)
                            if claves.get(k.value, "?") == "?":
                                claves[k.value] = t
            for n in nombres:
                out[n] = (claves, os.path.basename(ruta))
    return out


def tipo_mio(v):
    return ("bin" if isinstance(v, Empaquetado) else "lista" if isinstance(v, list) else
            "num" if isinstance(v, int) and not isinstance(v, bool) else "texto" if isinstance(v, str) else "?")


async def main():
    suyas = formas_suyas(sys.argv[1])
    cfg = dict(T.CFG)
    cfg["myclub"] = {"matchday": [{"compe_id": 3}], "pesleague": [{"compe_id": 4}],
                     "jugadores": [{"gameplayer_id": 100 + i} for i in range(18)],
                     "entrenadores": [{"coach_id": 8}], "club_de_prueba": {"team_name": "P", "gp": 0}}
    srv = Servidor(cfg, Datos())
    srv.dormir = T._sin_pausa
    s, env = T.cliente(srv)
    r = srv.recibir
    await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "0110000100000099"})
    await r(s, 2, {"msgid": "CMD_GET_PLAYER_DATA", "rqid": 2})
    await r(s, 3, {"msgid": "CMD_CREATE_PLAYER", "rqid": 3, "name": "Phoenix"})
    filas, sin_handler, iguales = [], [], 0
    for n, (claves, archivo) in sorted(suyas.items()):
        if not claves or n in VIGILANCIAS or n in ("CMD_LOGIN", "CMD_CREATE_PLAYER", "CMD_LOGOUT"):
            continue
        if not hasattr(srv, "c_" + n.lower()):
            sin_handler.append(n)
            continue
        antes = len(env)
        try:
            await r(s, 9, {"msgid": n, "rqid": 9, "compe_id": 3, "start_index": 0, "view_num": 10})
        except Exception as e:  # noqa: BLE001
            filas.append((n, archivo, ["nuestro lanza " + type(e).__name__], []))
            continue
        resp = [m for _, m in env[antes:] if m.get("msgid") == n]
        mio = resp[-1] if resp else {}
        if mio.get("result") not in (None, "NOERR"):
            filas.append((n, archivo, [f"nuestro responde {mio.get('result')}"], []))
            continue
        faltan = sorted(k for k in claves if k not in mio)
        distinto = sorted(f"{k}: suyo {t} / nuestro {tipo_mio(mio[k])}" for k, t in claves.items()
                          if k in mio and t != "?" and tipo_mio(mio[k]) not in ("?", t))
        if faltan or distinto:
            filas.append((n, archivo, distinto, faltan))
        else:
            iguales += 1
    print("# Tipos de respuesta: AzrakHQ (lectura estática de su código) contra Phoenix\n")
    print("Su código se LEE con ast (no se ejecuta). Solo claves de primer nivel de sus `return {...}`.\n")
    print(f"- Igual forma: {iguales}\n- Con diferencias: {len(filas)}\n- Sin handler nuestro: {len(sin_handler)}\n")
    print("| Comando | Su archivo | Tipo distinto | Faltan en el nuestro |\n|---|---|---|---|")
    for n, a, d, f in filas:
        print(f"| {n} | {a} | {'; '.join(d)} | {', '.join(f)} |")
    if sin_handler:
        print("\n## Sin handler nuestro\n\n" + ", ".join(sin_handler))


asyncio.run(main())
