"""Compara las fotos de phoenix_cursor_prueba.lua y propone dónde guarda PES 2021 el mando que controla
a cada futbolista. Uso:  py analizar_cursor.py <carpeta content\\phoenix\\cursor copiada de la PC de Fralex>

Fotos esperadas (nombre de la marca → quién controla al futbolista objetivo):
  A1, A2 = teclado · B1, B2 = mando físico · N1, N2 = nadie.
Un byte es «candidato fuerte» si A1 == A2, B1 == B2, N1 == N2 y los tres valores son distintos.
«Controlado por alguien» (sí/no) si A == B != N.
"""
import os
import re
import sys

VENT, MEDIO = 0x1000, 0x800


def leer_marcas(carpeta):
    fotos = {}
    for nombre in sorted(os.listdir(carpeta)):
        m = re.match(r"marca_(\d+)_(.+)\.bin$", nombre)
        if not m:
            continue
        datos = open(os.path.join(carpeta, nombre), "rb").read()
        paso = VENT + 8
        ventanas = []
        for i in range(len(datos) // paso):
            dir_ = int.from_bytes(datos[i * paso:i * paso + 8], "little")
            ventanas.append((dir_, datos[i * paso + 8:(i + 1) * paso]))
        fotos[m.group(2)] = ventanas
    return fotos


def analizar(carpeta):
    fotos = leer_marcas(carpeta)
    print("Fotos:", ", ".join(fotos) or "ninguna")
    faltan = [k for k in ("A1", "A2", "B1", "B2", "N1", "N2") if k not in fotos]
    if faltan:
        print("Faltan las fotos", faltan, "→ solo se muestra lo que se pueda.")
    n = min((len(v) for v in fotos.values()), default=0)
    fuertes, alguien = [], []
    for c in range(n):
        def val(k, o):
            d, v = fotos[k][c]
            return v[o] if d else None
        for o in range(VENT):
            if faltan:
                break
            a1, a2, b1, b2, n1, n2 = (val(k, o) for k in ("A1", "A2", "B1", "B2", "N1", "N2"))
            if None in (a1, b1, n1):
                continue
            if a1 == a2 and b1 == b2 and n1 == n2:
                if len({a1, b1, n1}) == 3:
                    fuertes.append((c + 1, o - MEDIO, a1, b1, n1))
                elif a1 == b1 != n1:
                    alguien.append((c + 1, o - MEDIO, a1, n1))
    print(f"\nCandidatos FUERTES (teclado / mando físico / nadie distintos): {len(fuertes)}")
    for c, o, a, b, nn in fuertes[:200]:
        print(f"  c{c} {o:+#06x}: teclado={a:#04x} mando={b:#04x} nadie={nn:#04x}")
    print(f"\n«Controlado por alguien» (teclado = mando ≠ nadie): {len(alguien)}")
    for c, o, a, nn in alguien[:200]:
        print(f"  c{c} {o:+#06x}: con cursor={a:#04x} sin={nn:#04x}")
    goles = [k for k in fotos if k.startswith("gol")]
    if goles and fuertes:
        print("\nEsos bytes en las fotos de gol:")
        for c, o, *_ in fuertes[:40]:
            vals = []
            for k in goles:
                d, v = fotos[k][c - 1]
                vals.append(f"{k}={v[o + MEDIO]:#04x}" if d else f"{k}=?")
            print(f"  c{c} {o:+#06x}: " + " ".join(vals))
    ruta = os.path.join(carpeta, "tras_gol.txt")
    if os.path.exists(ruta):
        todo = b"".join(v for f in fotos.values() for _, v in f)
        lineas = [l.split() for l in open(ruta, encoding="utf-8") if l.strip() and not l.startswith("#")]
        print(f"\nTras el gol aparecieron {len(lineas)} direcciones nuevas con el ID:")
        for partes in lineas[:60]:
            if len(partes) < 2 or partes[1] == "ilegible":
                print("  ", partes[0], "ilegible")
                continue
            ctx = bytes.fromhex(partes[1])
            copia = ctx[32:96] in todo   # ¿es una copia de nuestras propias fotos?
            print(f"   {partes[0]}{'  (posible copia propia de la prueba)' if copia else ''}")
            print("     ", partes[1][96:256 + 96])


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    analizar(sys.argv[1])
