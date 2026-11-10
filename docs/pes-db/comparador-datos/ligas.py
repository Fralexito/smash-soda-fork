import sys,struct,collections,csv
sys.path.insert(0,'tools'); from cmp import norm
from teams import teams
def rd(k):
    comp=open(f'db/{k}_Competition.bin','rb').read(); reg=open(f'db/{k}_CompetitionRegulation.bin','rb').read(); ent=open(f'db/{k}_CompetitionEntry.bin','rb').read()
    C={}
    for i in range(len(comp)//36):
        r=comp[i*36:(i+1)*36]; cid=r[5]; code=r[8:36].split(b'\0')[0].decode('latin1')
        C[cid]=dict(code=code,region=r[3])
    for i in range(len(reg)//2352):
        r=reg[i*2352:(i+1)*2352]; cid=r[8]; typ=r[9]; n=r[11]
        nm=r[0x14:0x14+0x73].split(b'\0')[0].decode('utf-8','replace')
        if cid in C: C[cid].update(type=typ,teams_decl=n,name=nm)
    E=collections.defaultdict(list)
    for i in range(len(ent)//12):
        t,eid,cid,pos=struct.unpack_from('<IIBB',ent,i*12); E[cid].append(t)
    return C,E
T={k:teams(f'db/{k}_Team.bin') for k in('con','sud')}
D={k:rd(k) for k in('con','sud')}
for k in D: print(k,'competiciones',len(D[k][0]),'con equipos',sum(1 for c in D[k][1] if D[k][1][c]),'filas',sum(len(v) for v in D[k][1].values()))
def key(code): return code
rows=[]
allc=set(c['code'] for c in D['con'][0].values())|set(c['code'] for c in D['sud'][0].values())
byc={k:{v['code']:(cid,v) for cid,v in D[k][0].items()} for k in D}
for code in sorted(allc):
    r=[code]
    info={}
    for k in ('con','sud'):
        if code in byc[k]:
            cid,v=byc[k][code]; ts=D[k][1].get(cid,[]); names=sorted(norm(T[k].get(t,('?',''))[0]) for t in ts)
            info[k]=(cid,v,ts,names)
    c=info.get('con'); s=info.get('sud')
    common=len(set(c[3])&set(s[3])) if c and s else ''
    rows.append([code,(c or s)[1].get('name',''),c[0] if c else '',len(c[2]) if c else '',s[0] if s else '',len(s[2]) if s else '',common,'ambas' if c and s else ('solo conmegol' if c else 'solo sudamerican')])
csv.writer(open('db/ligas_comparacion.csv','w',newline='',encoding='utf-8-sig')).writerows([['codigo','nombre','id_comp_conmegol','equipos_conmegol','id_comp_sud','equipos_sud','equipos_en_comun_por_nombre','estado']]+rows)
cnt=collections.Counter(r[-1] for r in rows); print(cnt)
for r in rows:
    if r[-1]=='ambas' and (r[3] or r[5]): print(r[:7])
print('--- solo conmegol con equipos'); print([ (r[0],r[3]) for r in rows if r[-1]=='solo conmegol' and r[3]][:40])
print('--- solo sud con equipos'); print([ (r[0],r[5]) for r in rows if r[-1]=='solo sudamerican' and r[5]][:40])
