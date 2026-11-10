# Extrae las minicaras (common/render/symbol/player/<id>.dds) de los cpk de cada parche
# y las guarda como webp pequeno en minifaces_<parche>.json (id -> base64). Solo lectura de los cpk.
import sys,os,json,struct,io,base64,glob
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import cpk_toc as C
from PIL import Image
H=os.path.dirname(os.path.abspath(__file__))
feat=json.load(open(os.path.join(H,'feat.json')))
def toc(path):
    f=open(path,'rb'); h=f.read(16)
    hd=C.utf(f.read(struct.unpack('<Q',h[8:16])[0]))
    f.seek(hd[0]['TocOffset']); t=f.read(16)
    return f,C.utf(f.read(struct.unpack('<Q',t[8:16])[0])),hd[0]
def run(name,cpks,loose,ids):
    want={str(i) for i in ids}; found={}
    for cp in cpks:
        if not os.path.exists(cp): print('falta',cp); continue
        f,rows,hd=toc(cp); off=hd.get('ContentOffset',0); n=0
        for r in rows:
            if r.get('DirName')=='common/render/symbol/player' and r['FileName'].endswith('.dds'):
                pid=r['FileName'][:-4]
                if pid in want and r['FileSize']==r['ExtractSize']:
                    f.seek(r['FileOffset']+off); found[pid]=f.read(r['FileSize']); n+=1
        print(name,os.path.basename(cp),n,flush=True)
    for d in loose:
        for pid in want:
            p=os.path.join(d,pid+'.dds')
            if os.path.exists(p): found[pid]=open(p,'rb').read()
    out={}
    for pid,b in found.items():
        try:
            im=Image.open(io.BytesIO(b)).convert('RGBA'); im.thumbnail((64,64))
            bio=io.BytesIO(); im.save(bio,'WEBP',quality=70); out[pid]=base64.b64encode(bio.getvalue()).decode()
        except Exception as e: print('err',pid,e)
    json.dump(out,open(os.path.join(H,f'minifaces_{name}.json'),'w'))
    print(name,'con imagen',len(out),'de',len(want),flush=True)
M=os.path.join(os.path.expanduser('~'),'mnt'); CP=os.path.join(M,'Conmegol Patch'); SP=os.path.join(M,'SP2026')
if sys.argv[1]=='con':
    run('con',[os.path.join(CP,'Data','dt14_all.cpk')]+[os.path.join(CP,'download',f'dt80_{k}00E_x64.cpk') for k in range(1,8)],
        [os.path.join(CP,'SiderAddons','livecpk','Logos','common','render','symbol','player'),os.path.join(CP,'SiderAddons','livecpk','Leyendas','common','render','symbol','player')],feat['con'])
else:
    run('sud',[os.path.join(SP,'download',f'dt80_{k}00E_x64.cpk') for k in range(1,8)],[],feat['sud'])
