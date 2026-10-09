"""Referencia con direcciones FIJAS del guardado ML00000006 (8 oct 2026: City 25 jugadores, tras el partido del slot 6)
y la misma lógica del prototipo v6, generalizada a cualquier jugador/sustituto. Sirvió para comprobar que el módulo C++
(core/LigaMaster.cpp, moverUsuarioAIA, que localiza todo por anclas) produce el MISMO archivo byte a byte.
uso: ref_v7_juego.py <ML descifrado.bin> <salida.bin> k_usuario k_destino pid pid_sustituto dorsal
Las direcciones solo valen para ese guardado (el bloque grande cambia de tamaño y desplaza las tablas I y J)."""
import sys, struct
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mlsquad
from mlparse import *
from build_c import roster_set
ent, sal, k_user, k_dest, pid, pid_sust, dorsal = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6]), int(sys.argv[7])
d = bytearray(open(ent,'rb').read())
# pid0 de idx 0 → ajustar a la convención de mlsquad (pidoff = pid de idx 9)
T = [('A',0xbe5fcc,24),('A2',0xbe639c,24),('B',0xc06268,44),('C',0xc50820,368),('D',0xc79f58,192),('E',0xc7f964,52),
     ('F',0xcab190,108),('G',0xcac76c,108),('H',0xcadd48,108),('M',0x1049b40,5628),('I',0x1267d37,48),('J',0x12d87b3,16)]
ro = roster(d,k_user); do = dorsals(d,k_user)[:len(ro)]; n=len(ro)
idx = [i for i,(r,p) in enumerate(ro) if p==pid][0]
idxS = [i for i,(r,p) in enumerate(ro) if p==pid_sust][0]
for name,pid0,st in T:
    mlsquad.quitar_registro(d, pid0 + 9*st, st, idx)
# L: pid de idx0 en 0xc06250; en mlsquad p = 25 - idx con L_PIDOFF = pid de idx 9 (p=16)
mlsquad.L_PIDOFF = 0xc06250 - 9*24; mlsquad.L_P_HAALAND = 16
mlsquad.L_quitar(d, 25-idx)
# K
mlsquad.K_PIDOFF = 0xb7f8b8 + 8 + 10*16; mlsquad.K_HAALAND = 10
LU, RO = 0x18f9d8, 0x18fa00
arr = list(d[LU:LU+n]); roles = list(d[RO:RO+6])
assert d[LU+n]==0xff
kl = mlsquad.K_lista(d); assert len(kl)==n
dec = lambda v: v-1 if v>idx else v
pos_h = arr.index(idx); pos_s = arr.index(idxS)
arr[pos_h] = idxS; del arr[pos_s]
flags = [f for i,(f,r,p) in enumerate(kl) if i!=pos_s]
arr = [dec(v) for v in arr]
roles = [idxS if v==idx else v for v in roles]; roles=[dec(v) for v in roles]
# roster usuario y destino
reg = ro[idx][0]
ro2=[e for i,e in enumerate(ro) if i!=idx]; do2=[x for i,x in enumerate(do) if i!=idx]
roster_set(d,k_user,ro2,do2)
rd=roster(d,k_dest); dd=dorsals(d,k_dest)[:len(rd)]
roster_set(d,k_dest,rd+[(reg,pid)],dd+[dorsal])
d[LU:LU+len(arr)] = bytes(arr); d[LU+len(arr)] = 0xff
d[RO:RO+6] = bytes(roles)
libre_flag = kl and struct.unpack_from('<I',d,mlsquad.K_rec(n))[0]
for i,v in enumerate(arr): mlsquad.K_set(d,i,flags[i],ro2[v][0],ro2[v][1])
mlsquad.K_set(d,len(arr), max(0xc0, libre_flag-1),0xffff,0)
mlsquad.K_set(d,len(arr)+1,0xc7,0xffff,0)
open(sal,'wb').write(d)
print('ref escrito', sal, 'idx',idx,'idxS',idxS,'arr',arr,'roles',roles,'libre_flag',hex(libre_flag))
