import asyncio, json, sys, re
sys.path.insert(0, '/home/user/smash-soda-fork/phoenix_online')
sys.path.insert(0, '/home/user/smash-soda-fork/phoenix_online/tests')
import logging; logging.disable(logging.CRITICAL)
from phoenix_online.datos import Datos
from phoenix_online.logica import Servidor, CONFIRMACIONES, VIGILANCIAS
from phoenix_online.msgpack_min import Empaquetado
import test_phoenix_online as T

CMDS = json.load(open('/mnt/project-files/pes-online/referencia_azrak_mit/notes/re/functions/data/commands.json'))

def claves(v, out):
    if isinstance(v, Empaquetado): v = v.valor
    if isinstance(v, dict):
        for k, x in v.items():
            out.add(k); claves(x, out)
    elif isinstance(v, list):
        for x in v: claves(x, out)

async def main():
    cfg = dict(T.CFG)
    cfg["myclub"] = {"matchday": [{"compe_id": 3}], "pesleague": [{"compe_id": 4}],
                     "jugadores": [{"gameplayer_id": 100 + i} for i in range(18)],
                     "entrenadores": [{"coach_id": 8}], "club_de_prueba": {"team_name": "P", "gp": 0}}
    srv = Servidor(cfg, Datos()); srv.dormir = T._sin_pausa
    s, env = T.cliente(srv); r = srv.recibir
    await r(s, 1, {"msgid": "CMD_LOGIN", "rqid": 1, "username": "CMP"})
    await r(s, 2, {"msgid": "CMD_GET_PLAYER_DATA", "rqid": 2})
    await r(s, 3, {"msgid": "CMD_CREATE_PLAYER", "rqid": 3})
    informe = []
    for c in CMDS:
        n = c["cmd"]; esperadas = set(c["reply"])
        if not esperadas or n in VIGILANCIAS: continue
        if not hasattr(srv, "c_" + n.lower()): continue
        antes = len(env)
        try:
            await r(s, 9, {"msgid": n, "rqid": 9, "compe_id": 3, "start_index": 0, "view_num": 10})
        except Exception as e:
            informe.append((n, c["area"], "EXCEPCION " + repr(e)[:80], [])); continue
        resp = [m for _, m in env[antes:] if m.get("msgid") == n]
        if not resp: continue
        mias = set(); claves(resp[-1], mias)
        if resp[-1].get("result") not in (None, "NOERR"):
            informe.append((n, c["area"], "ERROR " + str(resp[-1].get("result")), [])); continue
        faltan = sorted(esperadas - mias - {"result"})
        if faltan:
            informe.append((n, c["area"], f"faltan {len(faltan)} de {len(esperadas)}", faltan))
    informe.sort(key=lambda x: (x[1], x[0]))
    for n, a, est, f in informe:
        print(f"{a} | {n} | {est} | {', '.join(f)}")
    print("TOTAL con huecos:", len(informe))
asyncio.run(main())
