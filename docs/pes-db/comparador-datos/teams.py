import sys,struct,collections,csv
sys.path.insert(0,'tools'); from cmp import load,norm
REC=1532
def teams(p):
    b=open(p,'rb').read(); out={}
    for i in range(len(b)//REC):
        r=b[i*REC:(i+1)*REC]
        tid=struct.unpack_from('<I',r,8)[0]
        nm=r[0x170:0x170+0x46].split(b'\0')[0].decode('utf-8','replace')
        ab=r[0x372:0x375].decode('latin1','replace').strip('\0')
        out[tid]=(nm,ab)
    return out
def assign(p):
    b=open(p,'rb').read(); rows=[]
    for i in range(len(b)//16):
        rid,pid,tid,x=struct.unpack_from('<IIII',b,i*16); rows.append((pid,tid,x&0x3ff))
    return rows
D={}
for k in ('con','sud'):
    D[k]=dict(players={d['id']:d for d in load(f'db/{k}_Player.bin')},teams=teams(f'db/{k}_Team.bin'),asg=assign(f'db/{k}_PlayerAssignment.bin'))
    print(k,'equipos',len(D[k]['teams']),'asignaciones',len(D[k]['asg']))
    sq=collections.Counter(t for p,t,_ in D[k]['asg']); D[k]['sq']=sq
    print('  ejemplos',[(t,D[k]['teams'].get(t)) for t in list(D[k]['teams'])[:4]])
# team comparison by normalized name
cn={norm(n):(t,n,ab) for t,(n,ab) in D['con']['teams'].items() if n}
sn={norm(n):(t,n,ab) for t,(n,ab) in D['sud']['teams'].items() if n}
both=set(cn)&set(sn)
print('equipos por nombre: con',len(cn),'sud',len(sn),'en ambos',len(both),'solo con',len(set(cn)-set(sn)),'solo sud',len(set(sn)-set(cn)))
print('mismo id y nombre',sum(1 for k in both if cn[k][0]==sn[k][0]))
def squad(db,tid): return [db['players'][p]['printed'] for p,t,_ in db['asg'] if t==tid and p in db['players']]
w=csv.writer(open('db/equipos_comparacion.csv','w',newline='',encoding='utf-8-sig'))
w.writerow(['nombre','abrev','id_conmegol','id_sudamerican','jugadores_conmegol','jugadores_sudamerican','estado'])
for k in sorted(set(cn)|set(sn)):
    c=cn.get(k); s=sn.get(k)
    w.writerow([(c or s)[1],(c or s)[2],c[0] if c else '',s[0] if s else '',D['con']['sq'].get(c[0],0) if c else '',D['sud']['sq'].get(s[0],0) if s else '','ambos' if c and s else ('solo conmegol' if c else 'solo sudamerican')])
# exclusivos por equipo
def ex(me,other):
    oi=D[other]['players']; oi_names={(norm(d['printed']),d['nat']) for d in oi.values()}
    res=[]
    byp=collections.defaultdict(list)
    for p,t,_ in D[me]['asg']: byp[p].append(t)
    for pid,d in D[me]['players'].items():
        if pid in oi or (norm(d['printed']),d['nat']) in oi_names: continue
        ts=byp.get(pid,[])
        tn=', '.join(D[me]['teams'].get(t,('?',''))[0] for t in ts[:2]) or '(sin equipo)'
        res.append([pid,d['printed'],d['nat'],d['age'],tn])
    return res
for me,other,f in (('con','sud','exclusivos_conmegol_con_equipo.csv'),('sud','con','exclusivos_sudamerican_con_equipo.csv')):
    r=ex(me,other); csv.writer(open('db/'+f,'w',newline='',encoding='utf-8-sig')).writerows([['id','nombre','nac','edad','equipo(s)']]+r)
    cnt=collections.Counter(x[4] for x in r)
    print(me,'exclusivos',len(r),'sin equipo',cnt['(sin equipo)'],'top equipos:',cnt.most_common(12))
