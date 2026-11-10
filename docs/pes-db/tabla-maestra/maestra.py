import sys,collections,csv
sys.path.insert(0,'tools')
from cmp import load,norm
import teams as TM   # reutiliza lectura de equipos
import ligas as LG
con=load('db/con_Player.bin'); sud=load('db/sud_Player.bin')
for db in (con,sud):
    for d in db: d['pn']=norm(d['printed'])
ci={d['id']:d for d in con}; si={d['id']:d for d in sud}
# union-find sobre nodos ('c',id) y ('s',id)
par={}
def f(x):
    par.setdefault(x,x)
    while par[x]!=x: par[x]=par[par[x]]; x=par[x]
    return x
def u(a,b): par[f(a)]=f(b)
for d in con: f(('c',d['id']))
for d in sud: f(('s',d['id']))
why={}
for i in set(ci)&set(si):
    u(('c',i),('s',i)); why[i]='mismo_id_mismo_nombre' if ci[i]['pn']==si[i]['pn'] else 'mismo_id_nombre_distinto'
ck=collections.defaultdict(list); sk=collections.defaultdict(list)
for d in con: ck[(d['pn'],d['nat'])].append(d)
for d in sud: sk[(d['pn'],d['nat'])].append(d)
amb=0; byname=0
for k in set(ck)&set(sk):
    if len(ck[k])==1 and len(sk[k])==1:
        a,b=ck[k][0],sk[k][0]
        if f(('c',a['id']))!=f(('s',b['id'])) and a['id']!=b['id']:
            # solo une si ninguno ya está ligado por ID a otro lado
            if a['id'] not in si and b['id'] not in ci:
                u(('c',a['id']),('s',b['id'])); why[('x',a['id'],b['id'])]='mismo_nombre_id_distinto'; byname+=1
    else: amb+=1
# parecidos (revisar a mano)
par_rows=list(csv.DictReader(open('db/parecidos.csv',encoding='utf-8-sig')))
sim={}
for r in par_rows: sim[int(r['id_conmegol'])]=(int(r['id_sudamerican']),r['similitud'])
groups=collections.defaultdict(list)
for n in list(par): groups[f(n)].append(n)
rows=[]; n=0
def sortkey(g): 
    ids=[x[1] for x in g]; return min(ids)
for root,g in sorted(groups.items(),key=lambda kv:sortkey(kv[1])):
    cs=[x[1] for x in g if x[0]=='c']; ss=[x[1] for x in g if x[0]=='s']
    n+=1; pid='PXP-%06d'%n
    c=ci[cs[0]] if cs else None; s=si[ss[0]] if ss else None
    ref=c or s
    if c and s:
        tipo=why.get(cs[0]) or 'mismo_nombre_id_distinto'
        if tipo=='mismo_id_nombre_distinto' or tipo=='mismo_nombre_id_distinto': nota='revisar' if tipo=='mismo_id_nombre_distinto' else ''
        else: nota=''
        estado='en_ambas'
    elif c: tipo='exclusivo_conmegol'; nota=''; estado='solo_conmegol'
    else: tipo='exclusivo_sudamerican'; nota=''; estado='solo_sudamerican'
    if c and not s and cs[0] in sim: nota='parecido_con_sud_%d_sim_%s_revisar'%sim[cs[0]]
    if len(cs)>1 or len(ss)>1: nota=(nota+' multiple_ids').strip()
    rows.append([pid,ref['printed'],ref['nat'],ref['age'],ref['height'],ref['weight'],ref['regpos'],';'.join(map(str,cs)),';'.join(map(str,ss)),estado,tipo,nota])
H=['id_phoenix','nombre','nac_cod','edad','altura','peso','pos_registrada','id_conmegol','id_sudamerican','estado','tipo_coincidencia','nota']
csv.writer(open('db/maestra_jugadores.csv','w',newline='',encoding='utf-8-sig')).writerows([H]+rows)
st=collections.Counter(r[9] for r in rows); tp=collections.Counter(r[10] for r in rows)
print('jugadores Phoenix',len(rows),dict(st)); print(dict(tp)); print('nombre+nac ambiguos (duplicados) sin unir',amb,'unidos por nombre',byname)
print('con nota revisar',sum(1 for r in rows if r[11]))
# equipos
tc=TM.D['con']['teams']; ts=TM.D['sud']['teams']
cn=collections.defaultdict(list); sn=collections.defaultdict(list)
for t,(nm,ab) in tc.items(): cn[norm(nm)].append(t)
for t,(nm,ab) in ts.items(): sn[norm(nm)].append(t)
used_s=set(); trows=[]; k=0
for nm in sorted(set(cn)|set(sn)):
    if not nm: continue
    c=cn.get(nm,[]); s=sn.get(nm,[])
    k+=1; ref=(tc[c[0]] if c else ts[s[0]])
    est='en_ambas' if c and s else ('solo_conmegol' if c else 'solo_sudamerican')
    trows.append(['PXT-%05d'%k,ref[0],ref[1],';'.join(map(str,c)),';'.join(map(str,s)),est,'mismo_id' if c and s and c[0]==s[0] else ('mismo_nombre_id_distinto' if c and s else 'exclusivo')])
csv.writer(open('db/maestra_equipos.csv','w',newline='',encoding='utf-8-sig')).writerows([['id_phoenix','nombre','abrev','id_conmegol','id_sudamerican','estado','tipo_coincidencia']]+trows)
print('equipos Phoenix',len(trows),dict(collections.Counter(r[5] for r in trows)))
# ligas por codigo
lrows=[]; 
for i,rw in enumerate(csv.DictReader(open('db/ligas_comparacion.csv',encoding='utf-8-sig')),1):
    lrows.append(['PXL-%03d'%i,rw['codigo'],rw['nombre'],rw['id_comp_conmegol'],rw['id_comp_sud'],rw['equipos_conmegol'],rw['equipos_sud'],rw['estado']])
csv.writer(open('db/maestra_ligas.csv','w',newline='',encoding='utf-8-sig')).writerows([['id_phoenix','codigo','nombre','id_comp_conmegol','id_comp_sudamerican','equipos_conmegol','equipos_sudamerican','estado']]+lrows)
print('ligas',len(lrows))
