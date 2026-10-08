"""Compara la partida limpia (r0) con una partida guardada por el juego tras vender/liberar a un jugador del usuario."""
import sys, json, struct
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mlparse import *
from mlsquad import *
import numpy as np
nm = {}

def cargar_catalogo(ruta):
    c = json.load(open(ruta))
    nm.update({p['pes_id']: (p['nombre'], p['posicion']) for p in c['jugadores']})
def N(pid): return nm.get(pid,('?',''))[0]

def blob(d):
    size=struct.unpack_from('<I',d,0x11403a8)[0]
    return 0x1140374, size
def leer_tabla(d, name, po, st, shift):
    """lista de (reg,pid) usados, con shift para tablas posteriores al blob"""
    out=[]
    for i in range(200):
        p=rec_start(po+shift,st,i)
        reg,pid=struct.unpack_from('<II',d,p+4)
        if pid==0 and reg in (0,0xffff):
            out.append(('E',reg,pid)) ; break
        out.append((i,reg,pid))
    return out

def main(base_fn, gt_fn):
    b=open(base_fn,'rb').read(); g=open(gt_fn,'rb').read()
    bs,bsz=blob(b); gs,gsz=blob(g)
    print('tamaños base/gt',len(b),len(g),'blob size',hex(bsz),hex(gsz),'delta',gsz-bsz,'archivo delta',len(g)-len(b))
    shift_gt = gsz-bsz
    # cabecera blob
    print('blob header words base:',[hex(x) for x in struct.unpack_from('<8I',b,0x1140374)])
    print('blob header words gt  :',[hex(x) for x in struct.unpack_from('<8I',g,0x1140374)])
    # rosters que cambian
    for k in range(NT):
        rb=roster(b,k); rg=roster(g,k)
        if rb!=rg or dorsals(b,k)[:len(rb)]!=dorsals(g,k)[:len(rg)] or count_byte(b,k)!=count_byte(g,k):
            print('EQUIPO',k,team_name(g,k),len(rb),'->',len(rg),'count',count_byte(b,k),'->',count_byte(g,k))
            print('   salen:',[N(p) for _,p in rb if (_,p) not in rg],' entran:',[N(p) for _,p in rg if (_,p) not in rb])
            if k==154:
                print('   orden nuevo:',[N(p) for _,p in rg])
                print('   dorsales   :',dorsals(g,k)[:len(rg)])
    # tablas
    for name,po,st,dr in TABLAS:
        sh = shift_gt if name in ('I','J') else 0
        lb=leer_tabla(b,name,po,st,0); lg=leer_tabla(g,name,po,st,sh)
        pb=[p for i,r,p in lb if i!='E']; pg=[p for i,r,p in lg if i!='E']
        same = pb==pg
        print('TABLA',name,'usados base/gt',len(pb),len(pg),'IGUAL' if same else 'CAMBIA')
        if not same:
            print('    base:',[N(p)[:10] for p in pb][:30])
            print('    gt  :',[N(p)[:10] for p in pg][:30])
    # K
    kb=K_lista(bytearray(b)); kg=K_lista(bytearray(g))
    print('K base:',[(hex(f),N(p)[:9]) for f,r,p in kb])
    print('K gt  :',[(hex(f),N(p)[:9]) for f,r,p in kg])
    print('K cola gt:',[hex(x) for x in K_get(g,len(kg))],[hex(x) for x in K_get(g,len(kg)+1)],[hex(x) for x in K_get(g,len(kg)+2)])
    # diff general agrupado (región alineada previa al blob)
    A=np.frombuffer(b,dtype=np.uint8); G=np.frombuffer(g,dtype=np.uint8)
    lo=0x11f510; hi=0x1140364
    d=np.where(A[lo:hi]!=G[lo:hi])[0]+lo
    grp=[]
    if len(d):
        s=d[0];p=d[0]
        for x in d[1:]:
            if x-p>40: grp.append((s,p)); s=x
            p=x
        grp.append((s,p))
    print('diferencias pre-blob fuera de equipos:',len(grp),'grupos')
    for s,p in grp[:150]:
        print('   %s-%s (%d) base=%s gt=%s'%(hex(s),hex(p),p-s+1,bytes(A[s:min(p+1,s+24)]).hex(),bytes(G[s:min(p+1,s+24)]).hex()))
if __name__=='__main__':
    if len(sys.argv) < 3:
        sys.exit('uso: diff_gt.py <base descifrado.bin> <guardado por el juego descifrado.bin> [catalogo.json]  (limitar la salida con | head)')
    if len(sys.argv) > 3: cargar_catalogo(sys.argv[3])
    main(sys.argv[1], sys.argv[2])
