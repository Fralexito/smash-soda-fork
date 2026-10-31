# mover_porteria_fmdl.py — desplaza los modelos de portería (.fmdl de PES 2021, Fox v2.03) hacia el centro del campo.
# Uso:  python mover_porteria_fmdl.py <entrada.fmdl> <salida.fmdl> [desplazamiento_m=37.5]
# Los vértices (float32 xyz, paso de 12 B) y los volúmenes (bbox/centros) del modelo guardan x ≈ ±52.5 como float32.
# Se mueve TODO float32 alineado a 4 B con 44 < |v| < 60 una distancia dx hacia el centro (−52.5 → −15.0, +52.5 → +15.0).
# Las coordenadas y/z son < 10, no se ven afectadas. El tamaño del archivo no cambia (reempaquetar con GzsTool igual que el original).
# Probado el 27-oct-2026 con goal.fmdl (410 floats), goal5L/5R_net.fmdl y goal3L/3R_net.fmdl del Camp Nou: arcos visibles a ≈15 m.
import struct, sys
ent, sal = sys.argv[1], sys.argv[2]
dx = float(sys.argv[3]) if len(sys.argv) > 3 else 37.5
d = bytearray(open(ent, 'rb').read())
n = 0
for i in range(0, len(d) - 3, 4):
    v = struct.unpack_from('<f', d, i)[0]
    if 44 < abs(v) < 60:
        struct.pack_into('<f', d, i, v + dx if v < 0 else v - dx)
        n += 1
open(sal, 'wb').write(d)
print(n, 'floats movidos')
