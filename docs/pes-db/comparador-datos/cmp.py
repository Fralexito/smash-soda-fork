import struct, unicodedata, re, collections, csv, sys
F={'nat':(233,9,0),'height':(216,7,100),'weight':(256,7,30),'age':(408,6,15),'regpos':(434,4,0),'speed':(306,6,40),'finishing':(396,6,40),'ball_control':(281,6,40),'stamina':(338,6,40),'style':(155,5,0),'foot':(514,1,0),'form':(438,3,0)}
def load(p):
    b=open(p,'rb').read(); n=len(b)//312; out=[]
    for i in range(n):
        r=b[i*312:(i+1)*312]; v=int.from_bytes(r,'little')
        d={'id':struct.unpack_from('<I',r,8)[0]}
        for k,(bit,w,add) in F.items(): d[k]=((v>>bit)&((1<<w)-1))+add
        nm=[r[0x44+j*0x3d:0x44+(j+1)*0x3d].split(b'\0')[0].decode('utf-8','replace') for j in range(4)]
        d['name'],d['shirt'],d['screen'],d['printed']=nm
        out.append(d)
    return out
def norm(s):
    s=unicodedata.normalize('NFKD',s); s=''.join(c for c in s if not unicodedata.combining(c))
    return re.sub(r'[^a-z0-9 ]','',s.lower()).strip()
if __name__=='__main__':
    con=load('db/con_Player.bin'); sud=load('db/sud_Player.bin')
    print('registros',len(con),len(sud))
    for nm,db in (('con',con),('sud',sud)):
        print(nm,'ids únicos',len({d['id'] for d in db}),'sin nombre',sum(1 for d in db if not d['name']))
        for d in db[:3]: print(' ',d)
    ci={d['id']:d for d in con}; si={d['id']:d for d in sud}
    same=set(ci)&set(si)
    print('IDs en ambas',len(same))
    sn=sum(1 for i in same if norm(ci[i]['name'])==norm(si[i]['name']) )
    print('mismo ID y mismo nombre',sn)
    # by name+nat
    def key(d): return (norm(d['name']),d['nat'])
    ck=collections.defaultdict(list); sk=collections.defaultdict(list)
    for d in con: 
        if d['name']: ck[key(d)].append(d)
    for d in sud:
        if d['name']: sk[key(d)].append(d)
    both=set(ck)&set(sk)
    print('claves nombre+nacionalidad: con',len(ck),'sud',len(sk),'en ambas',len(both))
    idsame=sum(1 for k in both if ck[k][0]['id']==sk[k][0]['id'])
    print('  de ellas con el MISMO ID',idsame,'con ID DISTINTO',len(both)-idsame)
    # exclusivos
    ex_c=[ck[k][0] for k in ck if k not in sk]; ex_s=[sk[k][0] for k in sk if k not in ck]
    print('exclusivos conmegol',len(ex_c),'exclusivos sudamerican',len(ex_s))
    # parecidos: mismo apellido-ish (nombre normalizado distinto pero misma id) 
    diff=[(ci[i],si[i]) for i in same if norm(ci[i]['name'])!=norm(si[i]['name'])]
    print('mismo ID pero nombre distinto',len(diff))
    for a,b in diff[:8]: print('   ',a['id'],a['name'],'|',b['name'])
    # similar: mismo nombre pero distinta nacionalidad o IDs distintos
    csv.writer(open('db/exclusivos_conmegol.csv','w',newline='',encoding='utf-8')).writerows([['id','name','shirt','nat','age','height']]+[[d['id'],d['name'],d['shirt'],d['nat'],d['age'],d['height']] for d in ex_c])
    csv.writer(open('db/exclusivos_sudamerican.csv','w',newline='',encoding='utf-8')).writerows([['id','name','shirt','nat','age','height']]+[[d['id'],d['name'],d['shirt'],d['nat'],d['age'],d['height']] for d in ex_s])
    w=csv.writer(open('db/mismos_distinto_id.csv','w',newline='',encoding='utf-8')); w.writerow(['name','nat','id_conmegol','id_sud'])
    for k in both:
        if ck[k][0]['id']!=sk[k][0]['id']: w.writerow([ck[k][0]['name'],k[1],ck[k][0]['id'],sk[k][0]['id']])
