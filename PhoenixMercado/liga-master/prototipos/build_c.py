"""PROTOTIPO (fase experimental): mover un jugador del equipo DEL USUARIO a un equipo de la IA en ML00000000 descifrado.
Las direcciones de las tablas (mlsquad.TABLAS, L, K) son las del guardado de referencia (City, tras fichar a Neymar) y
solo valen para ese guardado: el módulo C++ definitivo debe localizarlas por anclas. Ver ../ESTRUCTURA-ML.md."""
import sys, json, struct
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mlparse import *
from mlsquad import *
nm = {}   # pes_id -> (nombre, posicion); lo llena cargar_catalogo()

def cargar_catalogo(ruta):
    c = json.load(open(ruta))
    nm.clear()
    nm.update({p['pes_id']: (p['nombre'], p['posicion']) for p in c['jugadores']})

def roster_set(d, k, entries, dors):
    s=team_base(k)+0x14c
    assert len(entries)<=40
    for i in range(40):
        reg,pid=entries[i] if i<len(entries) else (65535,0)
        struct.pack_into('<II',d,s+8*i,reg,pid)
        struct.pack_into('<H',d,s+4+0x14a+2*i, dors[i] if i<len(dors) else 0)
    d[s+4+0x2d6]=len(entries)

def mover_usuario_a_ia(d, k_user, k_dest, pid, dorsal_dest):
    ro=roster(d,k_user); do=dorsals(d,k_user)[:len(ro)]
    idx=[i for i,(reg,p) in enumerate(ro) if p==pid][0]
    reg=ro[idx][0]
    assert idx==HAALAND_IDX or True
    # 1) arrays por jugador (orden de plantilla)
    for name,po,st,dr in TABLAS:
        # Las offsets están calculados con Haaland en idx 9; para otro idx habría que recalcular (aquí idx==9)
        quitar_registro(d,po,st,idx)
    # 2) tabla L (inversa): posicion p = 25 - idx
    L_quitar(d, 25-idx)
    # 3) lista de alineación K
    kl=K_lista(d)
    ki=[i for i,(f,r,p) in enumerate(kl) if p==pid][0]
    # sustituto: primer jugador de K (a partir de la reserva, idx>=18) con posición ofensiva
    ataque={'CF','SS','AMF','LWF','RWF'}
    cand=[i for i,(f,r,p) in enumerate(kl) if i>=18 and nm.get(p,('?','?'))[1] in ataque]
    sust=cand[0]
    f_s,r_s,p_s=kl[sust]
    n=len(kl)
    # coloca al sustituto en el hueco del que se va, quita al sustituto de su sitio original (compactando)
    nuevo=list(kl)
    nuevo[ki]=(kl[ki][0],r_s,p_s)
    del nuevo[sust]
    # reescribe K: n-1 usados, siguiente libre = c0, resto c7 (patrón de 'antes' con 25 jugadores)
    for i,(f,r,p) in enumerate(nuevo): K_set(d,i,f,r,p)
    K_set(d,len(nuevo),0xc0,0xffff,0)
    K_set(d,len(nuevo)+1,0xc7,0xffff,0)
    # 4) roster y dorsales del usuario
    ro2=[e for i,e in enumerate(ro) if i!=idx]; do2=[x for i,x in enumerate(do) if i!=idx]
    roster_set(d,k_user,ro2,do2)
    # 5) destino IA
    rd=roster(d,k_dest); dd=dorsals(d,k_dest)[:len(rd)]
    roster_set(d,k_dest,rd+[(reg,pid)],dd+[dorsal_dest])
    return idx,ki,sust

if __name__=='__main__':
    if len(sys.argv) < 4:
        sys.exit('uso: build_c.py <ML descifrado.bin> <salida.bin> <catalogo.json> [equipo_usuario equipo_destino pes_id dorsal]  (por defecto 154 66 133543 19)')
    ent, sal, cat = sys.argv[1:4]
    k_user, k_dest, pid, dorsal = [int(x) for x in sys.argv[4:8]] if len(sys.argv) >= 8 else (154, 66, 133543, 19)
    cargar_catalogo(cat)
    d = bytearray(open(ent, 'rb').read())
    res = mover_usuario_a_ia(d, k_user, k_dest, pid, dorsal)
    print('idx plantilla', res[0], 'K idx', res[1], 'sustituto K', res[2])
    open(sal, 'wb').write(d)
    print('escrito', sal, len(d))
