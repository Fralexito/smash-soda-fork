# Reescribe las 5 formas convexas de hit_billboard.geom como 4 muros altos (sin huecos).
# Uso: python hacer_vallas.py entrada.geom salida.geom
import struct,sys
src,dst=sys.argv[1],sys.argv[2]
d=bytearray(open(src,'rb').read())
V0=0x5e0
IN,OUT=19.5,20.0     # cara interior / exterior en x
ZIN,ZOUT=11.5,12.0   # cara interior / exterior en z
H=3.5
def setv(i,x,y,z): struct.pack_into('<4f',d,V0+16*i,x,y,z,0.0)
def corners(longax,xr,zr):
    # (thick lo/hi, long lo/hi, y 0/H) -> 8 vértices con el patrón de S1: A=extremo bajo, B=alto
    if longax=='z': tl,th,ll,lh=xr[0],xr[1],zr[0],zr[1]
    else: tl,th,ll,lh=zr[0],zr[1],xr[0],xr[1]
    def p(t,y,l): return (t,y,l) if longax=='z' else (l,y,t)
    c=[p(tl,H,ll),p(th,H,ll),p(th,0,ll),p(tl,0,ll),p(tl,H,lh),p(tl,0,lh),p(th,0,lh),p(th,H,lh)]
    out=[]
    for (a,b,cc) in c:
        out.append((a,b,cc) if longax=='z' else (a,b,cc))
    return out
def box(longax,xr,zr):
    if longax=='z':
        tl,th=xr; ll,lh=zr
        return [(tl,H,ll),(th,H,ll),(th,0,ll),(tl,0,ll),(tl,H,lh),(tl,0,lh),(th,0,lh),(th,H,lh)]
    tl,th=zr; ll,lh=xr
    return [(ll,H,tl),(ll,H,th),(ll,0,th),(ll,0,tl),(lh,H,tl),(lh,0,tl),(lh,0,th),(lh,H,th)]
PAT=[(0,1,2,3),(4,5,6,7),(4,7,1,0),(7,6,2,1),(5,4,0,3)]
shapes=[ # (offset cabecera, ids de vértice, eje largo, rango x, rango z, extras)
 (0x140,[0,1,2,3,4,5,6,7],'z',(-OUT,-IN),(-ZOUT,ZOUT),[]),
 (0x200,[10,11,12,13,14,15,16,17],'x',(-OUT,OUT),(ZIN,ZOUT),[18,19,20,21]),
 (0x310,[22,23,24,25,26,27,8,9],'x',(-OUT,OUT),(-ZOUT,-ZIN),[]),
 (0x3d0,[28,29,30,31,32,33,34,35],'z',(IN,OUT),(-ZOUT,ZOUT),[]),
 (0x490,[36,37,38,39,40,41,42,43],'x',(-OUT,OUT),(ZIN,ZOUT),[44,45,46,47,48,49]),
]
for off,ids,ax,xr,zr,extra in shapes:
    b=box(ax,xr,zr)
    for k,i in enumerate(ids): setv(i,*b[k])
    for j,i in enumerate(extra): setv(i,*b[j%8])
    nf=5 if off in(0x140,0x310,0x3d0) else 11
    faces=[tuple(ids[x] for x in f) for f in PAT]
    while len(faces)<nf: faces.append(faces[len(faces)%5])
    q=off+0x80
    for f in faces:
        struct.pack_into('<4H',d,q,*f); q+=12
    hx=((xr[1]-xr[0])/2,H/2,(zr[1]-zr[0])/2); cx=((xr[0]+xr[1])/2,H/2,(zr[0]+zr[1])/2)
    struct.pack_into('<4f',d,off+0x30,*hx,0.0); struct.pack_into('<4f',d,off+0x40,*cx,0.0)
struct.pack_into('<4f',d,0xf0+0x30,OUT,H/2,ZOUT,0.0)
struct.pack_into('<4f',d,0xf0+0x40,0.0,H/2,0.0,0.0)
open(dst,'wb').write(d)
print('escrito',dst,len(d))
