"""v6: estrategia de v5 (tablas/L/roster/K) + alineación por índices (0x18f9d8) y roles (0x18fa00) corregidos."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_c
from build_c import *
ent, sal, cat = sys.argv[1:4]
cargar_catalogo(cat)
d = bytearray(open(ent,'rb').read())
LU, RO = 0x18f9d8, 0x18fa00
ro = roster(d,154)
HI = [i for i,(r,p) in enumerate(ro) if p==133543][0]
NEY = [i for i,(r,p) in enumerate(ro) if p==40352][0]
arr = list(d[LU:LU+26]); roles = list(d[RO:RO+6])
assert HI==9 and arr.count(255)==0
print('antes', arr, roles, 'Neymar idx', NEY)
dec = lambda v: v-1 if v>HI else v
# Neymar sustituye a Haaland en el XI; se retira de su sitio original
pos_h = arr.index(HI); pos_n = arr.index(NEY)
arr[pos_h] = NEY; del arr[pos_n]
arr = [dec(v) for v in arr]
roles = [NEY if v==HI else v for v in roles]; roles = [dec(v) for v in roles]
res = mover_usuario_a_ia(d,154,66,133543,19)
d[LU:LU+len(arr)] = bytes(arr); d[LU+len(arr)] = 0xff
d[RO:RO+6] = bytes(roles)
# K = espejo por pid de la alineación (flags como estaban)
ro2 = roster(d,154); kl = K_lista(d)
for i,v in enumerate(arr):
    f = kl[i][0]; K_set(d,i,f,ro2[v][0],ro2[v][1])
open(sal,'wb').write(d)
print('despues', arr, roles)
print('XI:', [nm.get(ro2[v][1],('?','?')) for v in arr[:11]])
print('banca:', [nm.get(ro2[v][1],('?','?'))[0] for v in arr[11:18]])
print('resto:', [nm.get(ro2[v][1],('?','?'))[0] for v in arr[18:]])
print('K ok:', all(K_lista(d)[i][2]==ro2[v][1] for i,v in enumerate(arr)), 'K len', len(K_lista(d)), 'roster', len(ro2))
