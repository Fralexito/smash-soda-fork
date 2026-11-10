import pickle,json
S,pairs=pickle.load(open('panel/S.pkl','rb'))
def rate(d):
    a=d['ab']
    if d['pos']==0: return sum(a[20:25])/5
    return sum(sorted(a[:20],reverse=True)[:10])/10
sc=[]
for c,s,r in pairs:
    if c in S['con']['p'] and s in S['sud']['p']:
        sc.append((max(rate(S['con']['p'][c]),rate(S['sud']['p'][s])),c,s))
sc.sort(reverse=True)
N=1500
F=sc[:N]
json.dump(dict(con=[c for _,c,_ in F],sud=[s for _,_,s in F]),open('panel/feat.json','w'))
print(len(F),F[0],F[-1])
