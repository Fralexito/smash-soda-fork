import sys,csv,difflib,collections
sys.path.insert(0,'tools'); from cmp import load,norm
con={d['id']:d for d in load('db/con_Player.bin')}; sud={d['id']:d for d in load('db/sud_Player.bin')}
rows=list(csv.DictReader(open('db/maestra_jugadores.csv',encoding='utf-8-sig')))
nxt=max(int(r['id_phoenix'][4:]) for r in rows)
new=[]; log=[]; cnt=collections.Counter()
for r in rows:
    if r['tipo_coincidencia']!='mismo_id_nombre_distinto': continue
    c=con[int(r['id_conmegol'])]; s=sud[int(r['id_sudamerican'])]
    a,b=norm(c['printed']),norm(s['printed']); ta,tb=a.split(),b.split()
    ratio=difflib.SequenceMatcher(None,a,b).ratio()
    last=difflib.SequenceMatcher(None,ta[-1] if ta else '',tb[-1] if tb else '').ratio()
    same_nat=c['nat']==s['nat']; edad=abs(c['age']-s['age'])
    contenido=set(ta)<=set(tb) or set(tb)<=set(ta)
    same=same_nat and edad<=3 and (contenido or ratio>=0.7 or (norm(c['shirt'])==norm(s['shirt']) and c['shirt']) or (last>=0.75 and edad<=2))
    if same:
        r['tipo_coincidencia']='mismo_id_nombre_variante_ok'; r['nota']=''; v='mismo_jugador'
    else:
        # misma ID usada para personas distintas en cada base: separar en dos entradas Phoenix
        sid=r['id_sudamerican']; r['id_sudamerican']=''; r['estado']='solo_conmegol'; r['tipo_coincidencia']='exclusivo_conmegol_id_compartido_con_otra_persona'; r['nota']='mismo_id_en_sud_es_otra_persona_%s'%sid
        nxt+=1; ref=s
        new.append(['PXP-%06d'%nxt,s['printed'],s['nat'],s['age'],s['height'],s['weight'],s['regpos'],'',sid,'solo_sudamerican','exclusivo_sudamerican_id_compartido_con_otra_persona','mismo_id_en_conmegol_es_otra_persona_%s'%c['id']])
        v='personas_distintas_mismo_id'
    cnt[v]+=1; log.append([r['id_phoenix'],c['id'],c['printed'],s['id'],s['printed'],c['nat'],s['nat'],c['age'],s['age'],round(ratio,2),v])
rows2=[list(r.values()) for r in rows]+new
H=list(rows[0].keys())
csv.writer(open('db/maestra_jugadores_v1.csv','w',newline='',encoding='utf-8-sig')).writerows([H]+rows2)
csv.writer(open('db/revisar_resueltos.csv','w',newline='',encoding='utf-8-sig')).writerows([['id_phoenix','id_conmegol','nombre_conmegol','id_sudamerican','nombre_sudamerican','nac_con','nac_sud','edad_con','edad_sud','similitud','veredicto']]+log)
print(cnt,'jugadores Phoenix v1',len(rows2))
st=collections.Counter(r[9] for r in rows2); print(dict(st))
print('muestras separadas:',[(l[2],l[4]) for l in log if l[-1]!='mismo_jugador'][:6])
print('muestras iguales:',[(l[2],l[4]) for l in log if l[-1]=='mismo_jugador'][:6])
