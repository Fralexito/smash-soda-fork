"""Prototipo: edición de plantilla del equipo del usuario en ML de PES 2021 (FASE EXPERIMENTAL).
Todas las tablas por jugador del equipo del usuario están alineadas con el orden de la lista de plantilla
(roster del bloque del equipo). Cada registro empieza 8 bytes antes del pid: [x][reg][pid]...
"""
import struct
from mlparse import *

# (nombre, offset del pid de 'Haaland' (idx roster 9) en el guardado DESPUES, stride, dirección)
# dirección: +1 = orden de plantilla, -1 = orden inverso (tabla L)
TABLAS = [
    ('A', 0xbe60a4, 24, +1), ('B', 0xc063f4, 44, +1), ('C', 0xc51510, 368, +1), ('D', 0xc7a618, 192, +1),
    ('E', 0xc7fb38, 52, +1), ('F', 0xcab55c, 108, +1), ('G', 0xcacb38, 108, +1), ('H', 0xcae114, 108, +1),
    ('M', 0x105611c, 5628, +1), ('I', 0x1263d4f, 48, +1), ('J', 0x12d46ab, 16, +1),
]
HAALAND_IDX = 9

def rec_start(pidoff, stride, idx, ref=HAALAND_IDX):
    return pidoff - 8 + (idx - ref) * stride

def es_vacio(d, p):
    reg, pid = struct.unpack_from('<II', d, p + 4)
    return pid == 0 and reg in (0, 0xffff)

def usados(d, pidoff, stride):
    n = 0
    while True:
        p = rec_start(pidoff, stride, n)
        if es_vacio(d, p): return n
        n += 1
        if n > 200: raise RuntimeError('tabla sin fin')

def quitar_registro(d, pidoff, stride, idx):
    """Compacta: elimina el registro idx y sube los siguientes; el último usado queda con la plantilla de vacío."""
    n = usados(d, pidoff, stride)
    if not (0 <= idx < n): raise ValueError('idx fuera de rango %d/%d' % (idx, n))
    base = rec_start(pidoff, stride, 0)
    vacio = bytes(d[base + n * stride: base + (n + 1) * stride])   # primer registro vacío = plantilla
    for i in range(idx, n - 1):
        d[base + i * stride: base + (i + 1) * stride] = d[base + (i + 1) * stride: base + (i + 2) * stride]
    d[base + (n - 1) * stride: base + n * stride] = vacio
    return n

# ---------- tabla L (orden inverso, crece hacia direcciones bajas) ----------
L_PIDOFF = 0xc06178           # pid de Haaland (idx plantilla 9) -> posicion p=16 en el guardado DESPUES
L_STRIDE = 24
L_P_HAALAND = 16
def L_rec(p): return L_PIDOFF - 8 + (p - L_P_HAALAND) * L_STRIDE

def L_quitar(d, p_quitar):
    """Quita el registro en posición p (p=0 es el más bajo en memoria = último fichado). Los de p<p_quitar bajan 1 posición hacia arriba."""
    # usados: desde p_min (primer registro no vacío) hasta p=25 (el último, Donnarumma). Contamos hacia abajo.
    pmin = L_P_HAALAND
    while not es_vacio(d, L_rec(pmin - 1)):
        pmin -= 1
    vacio = bytes(d[L_rec(pmin - 1): L_rec(pmin - 1) + L_STRIDE])     # plantilla de vacío (registro justo antes del primero usado)
    for p in range(p_quitar, pmin, -1):
        d[L_rec(p): L_rec(p) + L_STRIDE] = d[L_rec(p - 1): L_rec(p - 1) + L_STRIDE]
    d[L_rec(pmin): L_rec(pmin) + L_STRIDE] = vacio
    return pmin

# ---------- lista de alineación K ----------
K_PIDOFF = 0xb7f960           # pid de Haaland -> K10
K_HAALAND = 10
K_STRIDE = 16
def K_rec(i): return K_PIDOFF - 8 + (i - K_HAALAND) * K_STRIDE
def K_get(d, i): return struct.unpack_from('<4I', d, K_rec(i))
def K_set(d, i, flag, reg, pid): struct.pack_into('<4I', d, K_rec(i), flag, reg, pid, 0)
def K_lista(d):
    out = []; i = 0
    while True:
        f, reg, pid, _ = K_get(d, i)
        if pid == 0 and reg == 0xffff: break
        out.append((f, reg, pid)); i += 1
    return out
