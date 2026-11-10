import pickle,collections,sys
sys.path.insert(0,'tools')
S,pairs=pickle.load(open('panel/S.pkl','rb'))
from cmp import norm
votes=collections.defaultdict(collections.Counter)
for k in('con','sud'):
    P,T,A=S[k]['p'],S[k]['t'],S[k]['a']
    members=collections.defaultdict(list)
    for pid,l in A.items():
        for t,_ in l: members[t].append(pid)
    for t,ms in members.items():
        if 18<=len(ms)<=60:
            c=collections.Counter(P[m]['nat'] for m in ms if m in P)
            if not c: continue
            n,cnt=c.most_common(1)[0]
            if cnt/len(ms)>=0.8: votes[n][T[t]]+=1
nm={n:c.most_common(1)[0][0] for n,c in votes.items()}
print(len(nm)); print({n:nm[n] for n in sorted(nm)[:60]})
import json; json.dump(nm,open('panel/nat.json','w'))
