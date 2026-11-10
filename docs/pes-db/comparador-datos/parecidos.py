import sys,collections,csv,difflib
sys.path.insert(0,'tools'); from cmp import load,norm
con=load('db/con_Player.bin'); sud=load('db/sud_Player.bin')
for db in (con,sud):
    for d in db: d['pn']=norm(d['printed']); d['sn']=norm(d['shirt'])
ci={d['id'] for d in con}; si={d['id'] for d in sud}
ck={(d['pn'],d['nat']) for d in con}; sk={(d['pn'],d['nat']) for d in sud}
exc=[d for d in con if d['id'] not in si and (d['pn'],d['nat']) not in sk]
exs=[d for d in sud if d['id'] not in ci and (d['pn'],d['nat']) not in ck]
bn=collections.defaultdict(list)
for d in exs: bn[d['nat']].append(d)
def toks(s): return s.split()
def score(a,b):
    ta,tb=toks(a['pn']),toks(b['pn'])
    if not ta or not tb: return 0
    r=difflib.SequenceMatcher(None,a['pn'],b['pn']).ratio()
    # mismo apellido (último token) y misma inicial / uno contenido en el otro
    cont=(set(ta)<=set(tb) or set(tb)<=set(ta)) and min(len(ta),len(tb))>=1 and abs(a['age']-b['age'])<=1 and abs(a['height']-b['height'])<=2
    return max(r,0.9) if cont else r
res=[]
for a in exc:
    best=None
    for b in bn.get(a['nat'],()):
        if abs(a['age']-b['age'])>2 or abs(a['height']-b['height'])>4: continue
        s=score(a,b)
        if s>=0.88 and (best is None or s>best[0]): best=(s,b)
    if best: res.append((a,best[1],best[0]))
print('exclusivos con',len(exc),'sud',len(exs),'parecidos (misma nac., edad ±2, altura ±4, nombre similar):',len(res))
alta=[r for r in res if r[2]>=0.95]; print('  con similitud >=0.95:',len(alta))
for a,b,s in res[:12]: print('  ',round(s,2),a['id'],a['printed'],'|',b['id'],b['printed'],a['nat'],a['age'],b['age'])
w=csv.writer(open('db/parecidos.csv','w',newline='',encoding='utf-8-sig')); w.writerow(['similitud','id_conmegol','nombre_conmegol','id_sudamerican','nombre_sudamerican','nac','edad_con','edad_sud','altura_con','altura_sud'])
for a,b,s in sorted(res,key=lambda r:-r[2]): w.writerow([round(s,2),a['id'],a['printed'],b['id'],b['printed'],a['nat'],a['age'],b['age'],a['height'],b['height']])
mc={a['id'] for a,b,s in res}; ms={b['id'] for a,b,s in res}
rest_c=[d for d in exc if d['id'] not in mc]; rest_s=[d for d in exs if d['id'] not in ms]
print('exclusivos reales tras quitar parecidos: conmegol',len(rest_c),'sudamerican',len(rest_s))
H=['id','nombre','camiseta','nac','edad','altura','peso','pos']
r=lambda db:[[d['id'],d['printed'],d['shirt'],d['nat'],d['age'],d['height'],d['weight'],d['regpos']] for d in db]
csv.writer(open('db/exclusivos_conmegol_v2.csv','w',newline='',encoding='utf-8-sig')).writerows([H]+r(rest_c))
csv.writer(open('db/exclusivos_sudamerican_v2.csv','w',newline='',encoding='utf-8-sig')).writerows([H]+r(rest_s))
