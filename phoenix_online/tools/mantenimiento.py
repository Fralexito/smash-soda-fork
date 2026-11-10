"""Modo mantenimiento: nadie NUEVO puede entrar (los que ya juegan siguen).

    python tools/mantenimiento.py on    -> crea MANTENIMIENTO.txt
    python tools/mantenimiento.py off   -> lo quita
    python tools/mantenimiento.py       -> dice cómo está

Úsalo antes de actualizar o reiniciar el servidor: cuando GET /estado diga 0 jugadores,
ya se puede cerrar sin cortar el partido de nadie.
"""
import os
import sys

ARCHIVO = "MANTENIMIENTO.txt"

if __name__ == "__main__":
    accion = (sys.argv[1] if len(sys.argv) > 1 else "").lower()
    if accion == "on":
        with open(ARCHIVO, "w", encoding="utf-8") as f:
            f.write("Servidor en mantenimiento. Borra este archivo (o usa «off») para abrirlo.\n")
        print("Mantenimiento ACTIVADO: nadie nuevo puede entrar.")
    elif accion == "off":
        if os.path.exists(ARCHIVO):
            os.remove(ARCHIVO)
        print("Mantenimiento DESACTIVADO: el servidor acepta jugadores.")
    else:
        print("Mantenimiento:", "ACTIVADO" if os.path.exists(ARCHIVO) else "desactivado")
