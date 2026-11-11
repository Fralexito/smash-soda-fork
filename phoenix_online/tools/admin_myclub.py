"""Administración de myClub desde la consola (la web lo hará con botones más adelante).

Uso, en la carpeta del servidor:
    python -I tools/admin_myclub.py config.json lista
    python -I tools/admin_myclub.py config.json dar-gp FRALEX 5000     (suma; negativo = resta)
    python -I tools/admin_myclub.py config.json poner-gp FRALEX 20000  (deja el saldo exacto)
    python -I tools/admin_myclub.py config.json dar-monedas FRALEX 100  (monedas gratis; igual
    con poner-monedas). Las monedas solo se ven en PES con myclub.monedas_gratis = true.
    python -I tools/admin_myclub.py config.json poner-contrato FRALEX 30 1 2 3   (deja 30 partidos
    de contrato a los jugadores con esos gameplayer_unique_id; para probar la renovación)
    python -I tools/admin_myclub.py config.json dar-boletos FRALEX 3   (Boletos de contrato;
    igual con poner-boletos)
    python -I tools/admin_myclub.py config.json dar-recuperar FRALEX 3  (objetos «Recuperar
    resistencia»; igual con poner-recuperar)
    python -I tools/admin_myclub.py config.json regalo FRALEX gp 5000 "Premio del torneo"
    (pone un regalo en la «Bandeja entrada»; tipo gp, monedas o la clave de un objeto, p. ej.
    objetos_recuperar o boletos_contrato; el título es opcional; dura 30 días)

La persona se indica por su nombre en el juego (pname) o por su steam_hex.
Se puede usar con el servidor encendido: el cambio se ve la próxima vez que PES pida el club
(al entrar a myClub). Cada cambio queda anotado en club["admin"] con la hora.
"""
import json
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from phoenix_online.datos import Datos  # noqa: E402


def buscar(datos, quien):
    for j in datos.todos():
        if quien.lower() in ((j["pname"] or "").lower(), (j["steam_hex"] or "").lower()):
            return j
    return None


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if len(argv) < 2:
        print(__doc__)
        return 1
    with open(argv[0], encoding="utf-8") as f:
        cfg = json.load(f)
    datos = Datos(cfg.get("base_datos", "phoenix_online.db"))
    orden = argv[1]
    if orden == "lista":
        for j in datos.todos():
            club = datos.club(j["steam_hex"]) or {}
            com = club.get("vscom") or {}
            print(f'{j["pname"]:<16} {j["steam_hex"]:<18} club={club.get("team_name", "-"):<14} '
                  f'GP={club.get("gp", 0):>8} monedas={club.get("monedas", 0):>6} '
                  f'boletos={club.get("boletos_contrato", 0):>3} '
                  f'partidos={com.get("partidos", 0)}')
        return 0
    if orden == "poner-contrato" and len(argv) >= 5:
        j = buscar(datos, argv[2])
        club = datos.club(j["steam_hex"]) if j else None
        if club is None:
            print("No encontré a esa persona o todavía no tiene club.")
            return 1
        partidos, uids = int(argv[3]), {int(u) for u in argv[4:]}
        cambiados = []
        for jug in club.get("jugadores") or []:
            if jug.get("gameplayer_unique_id") in uids:
                jug["contract_term"] = partidos * 90  # C (05-nov): unidad del juego = partidos x 90
                cambiados.append(jug["gameplayer_unique_id"])
        club["contrato_x90"] = True
        club.setdefault("admin", []).append({"hora": int(time.time()), "orden": orden,
                                             "monto": partidos, "jugadores": cambiados})
        datos.guardar_club(j["steam_hex"], club)
        print(f'{j["pname"]}: contrato de {partidos} partidos a los unique_id {cambiados}')
        return 0
    if orden == "regalo" and len(argv) in (5, 6):
        from phoenix_online.logica import Servidor
        j = buscar(datos, argv[2])
        club = datos.club(j["steam_hex"]) if j else None
        if club is None:
            print("No encontré a esa persona o todavía no tiene club.")
            return 1
        tipo, cantidad = argv[3], int(argv[4])
        if cantidad <= 0:
            print("La cantidad debe ser mayor que 0.")
            return 1
        ide = Servidor.dar_regalo(club, tipo, cantidad, argv[5] if len(argv) == 6 else "")
        club.setdefault("admin", []).append({"hora": int(time.time()), "orden": orden,
                                             "tipo": tipo, "monto": cantidad, "regalo": ide})
        datos.guardar_club(j["steam_hex"], club)
        print(f'{j["pname"]}: regalo {ide} ({tipo} x{cantidad}) en su bandeja')
        return 0
    campos = {"dar-gp": "gp", "poner-gp": "gp", "dar-monedas": "monedas",
              "poner-monedas": "monedas", "dar-boletos": "boletos_contrato",
              "poner-boletos": "boletos_contrato", "dar-recuperar": "objetos_recuperar",
              "poner-recuperar": "objetos_recuperar"}
    if orden in campos and len(argv) == 4:
        campo = campos[orden]
        j = buscar(datos, argv[2])
        if j is None:
            print("No encontré a esa persona. Usa «lista» para ver los nombres.")
            return 1
        club = datos.club(j["steam_hex"])
        if club is None:
            print("Esa persona todavía no tiene club (debe entrar una vez a myClub).")
            return 1
        monto = int(argv[3])
        antes = int(club.get(campo) or 0)
        nuevo = antes + monto if orden.startswith("dar-") else monto
        if nuevo < 0:
            print(f"El saldo quedaría negativo ({nuevo}); no se cambió nada.")
            return 1
        club[campo] = nuevo
        club.setdefault("admin", []).append({"hora": int(time.time()), "orden": orden,
                                             "monto": monto, "antes": antes, "despues": nuevo})
        datos.guardar_club(j["steam_hex"], club)
        print(f'{j["pname"]}: {campo} {antes} -> {nuevo}')
        return 0
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main())
