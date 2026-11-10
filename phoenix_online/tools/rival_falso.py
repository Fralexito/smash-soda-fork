"""Rival de prueba: un «segundo jugador» falso para probar el emparejamiento con UN solo PES.

Entra al servidor como otro jugador (SteamID inventado), busca partida del mismo tipo que tú y,
cuando el servidor os empareja, informa «chequeo de conexión correcto». Así se puede ver en el
PES real todo el camino hasta la carga del partido (fase 2 del relé) sin tener a un amigo.
NO juega: el partido no puede empezar de verdad (no manda mandos), así que el PES acabará
mostrando un error de conexión al cargar. Es solo para diagnosticar.

Uso (en la carpeta del servidor, con el servidor abierto):
    python tools/rival_falso.py                 -> Divisiones en línea (OC_RANK)
    python tools/rival_falso.py OC_QUICK        -> Partida rápida
Primero arranca este rival y DESPUÉS busca partida en el PES (o al revés: da igual).
"""
import asyncio
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from phoenix_online.nclmio import Codec, LectorTramas  # noqa: E402

STEAM_FALSO = "0110000100001234"


async def main(tipo):
    with open("config.json", encoding="utf-8") as f:
        cfg = json.load(f)
    with open(cfg["clave_nclmio"], "rb") as f:
        codec = Codec(f.read(), blowfish_le=cfg.get("blowfish_le", False))
    host = "127.0.0.1"
    lector, (r, w) = LectorTramas(codec), await asyncio.open_connection(host, cfg["puerto_menu"])
    seq = 0

    async def mandar(msg):
        nonlocal seq
        seq += 1
        w.write(codec.codificar(seq, {"rqid": seq, **msg}))
        await w.drain()

    async def esperar(msgid, segundos=600):
        fin = asyncio.get_running_loop().time() + segundos
        while asyncio.get_running_loop().time() < fin:
            datos = await asyncio.wait_for(r.read(65536), fin - asyncio.get_running_loop().time())
            if not datos:
                raise ConnectionError("el servidor cerró la conexión")
            for _t, _s, m in lector.alimentar(datos):
                print("  <-", m.get("msgid"), m.get("result"), m.get("matching_state", ""))
                if m.get("msgid") == msgid:
                    return m
        raise TimeoutError(msgid)

    await mandar({"msgid": "CMD_LOGIN", "username": STEAM_FALSO.lstrip("0")})
    await esperar("CMD_LOGIN")
    await mandar({"msgid": "CMD_GET_PLAYER_DATA"})
    if (await esperar("CMD_GET_PLAYER_DATA")).get("has_data") != "YES":
        await mandar({"msgid": "CMD_CREATE_PLAYER", "pname": "RIVAL_PRUEBA"})
        await esperar("CMD_CREATE_PLAYER")
    print(f"Rival de prueba dentro. Buscando {tipo}… (ahora busca partida en tu PES)")
    await mandar({"msgid": "CMD_START_MATCHING", "matching_type": tipo})
    while True:
        m = await esperar("CMD_WATCH_MATCHING")
        if m.get("matching_state") == "FINISH":
            break
    print("¡Emparejados! Informo chequeo de conexión correcto.")
    await asyncio.sleep(3)
    await mandar({"msgid": "CMD_SET_GAMERELAY_CHECK_RES", "error_code": "NOERR", "failed_reason": 0})
    try:
        await esperar("CMD_WATCH_GAMERELAY_CHECK_RES", 120)
        print("El servidor liberó la barrera del chequeo (fase 2 correcta hasta aquí).")
        await mandar({"msgid": "CMD_SEND_MATCHING_RESULT", "error_code": "NOERR",
                      "quality_select_kind": "SELECTED"})
        await asyncio.sleep(90)  # mantiene la conexión mientras el PES intenta cargar
    finally:
        await mandar({"msgid": "CMD_LEAVE_GAME"})
        w.close()
        print("Rival de prueba fuera. Mira registro/servidor.log para el detalle.")


if __name__ == "__main__":
    asyncio.run(main(sys.argv[1] if len(sys.argv) > 1 else "OC_RANK"))
