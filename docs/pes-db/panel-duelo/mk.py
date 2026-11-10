import pickle,json,collections,sys
sys.path.insert(0,'tools')
from cmp import norm
S,pairs=pickle.load(open('panel/S.pkl','rb'))
NAT=json.load(open('panel/nat.json')); NAT={int(k):v for k,v in NAT.items()}
for k in (0,10,127,131,139): NAT.pop(k,None)
MC=json.load(open('panel/minifaces_con.json')); MS=json.load(open('panel/minifaces_sud.json'))
def rate(d):
    a=d['ab']
    return round(sum(a[20:25])/5) if d['pos']==0 else round(sum(sorted(a[:20],reverse=True)[:10])/10)
def vec(d): return [d['pos'],d['age'],d['height'],d['weight'],d['foot'],d['style'],d['wfu'],d['wfa'],d['form'],d['inj'],d['sk'],rate(d)]+d['ab']
def clubs(k,pid):
    P=S[k]; names=[]
    for t,_ in P['a'].get(pid,[]):
        n=P['t'].get(t,'')
        if n and n not in names and n not in NAT.values(): names.append(n)
    return ' / '.join(names[:2])
imgs={}
def img(m,pid):
    s=m.get(str(pid))
    if not s: return 0
    if s not in imgs: imgs[s]=len(imgs)
    return imgs[s]+1
nats=set(); rows=[]
for c,s,r in pairs:
    a=S['con']['p'].get(c); b=S['sud']['p'].get(s)
    if not a or not b: continue
    nats.add(a['nat'])
    rows.append([c,s,a['printed'],b['printed'] if b['printed']!=a['printed'] else '',a['nat'],clubs('con',c),clubs('sud',s),vec(a),vec(b),img(MC,c),img(MS,s)])
rows.sort(key=lambda x:-max(x[7][11],x[8][11]))
IM=[None]*len(imgs)
for k,v in imgs.items(): IM[v]=k
# clubes
cn={norm(n):t for t,n in S['con']['t'].items() if n}; sn={norm(n):t for t,n in S['sud']['t'].items() if n}
def squad(k,t):
    P=S[k]; out=[]
    for pid,l in P['a'].items():
        for tt,sh in l:
            if tt==t and pid in P['p']: out.append((pid,sh))
    return out
pmap_c={}; 
for c,s,r in pairs: pmap_c[c]=s
T=[]
for k in sorted(set(cn)&set(sn)):
    tc,ts=cn[k],sn[k]
    qc=squad('con',tc); qs=squad('sud',ts)
    if len(qc)<11 and len(qs)<11: continue
    cs={pid for pid,_ in qc}
    def lst(kk,q,other):
        P=S[kk]['p']; o=[]
        for pid,sh in q:
            d=P[pid]; o.append([pid,d['printed'],d['pos'],rate(d),sh,1 if (pid in other) else 0])
        return sorted(o,key=lambda x:-x[3])
    sc={pmap_c.get(p) for p in cs}
    T.append([S['con']['t'][tc],tc,ts,lst('con',qc,set(pmap_c)),lst('sud',qs,set(pmap_c.values()))])
print(len(rows),len(T),len(IM))
D=dict(R=rows,IM=IM,NAT={n:NAT.get(n,'País #%d'%n) for n in nats},T=T)
html=open('panel/tpl.html').read().replace('__DATA__',json.dumps(D,ensure_ascii=False,separators=(',',':')))
open('panel/index.html','w').write(html); print(len(html)/1e6,'MB')
