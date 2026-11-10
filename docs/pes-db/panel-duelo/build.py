import sys,csv,struct,collections,json
sys.path.insert(0,'tools'); sys.path.insert(0,'/home/user/matozanato/footballlife-new-leagues/tools')
from cmp import norm
BODY=[("pos",434,4,0),("height",216,7,100),("weight",256,7,30),("age",408,6,15),("nat",233,9,0),("foot",514,1,0),("wfu",454,2,0),("wfa",462,2,0),("form",438,3,0),("inj",458,2,0),("style",155,5,0)]
AB=[("OffAw",370),("BallCtl",281),("Dribbl",352),("TightPos",416),("LowPass",263),("LoftPass",402),("Finish",396),("Header",288),("PlaceK",250),("Curl",332),("Speed",306),("Accel",344),("KickPw",358),("Jump",294),("PhysCt",390),("Balance",376),("Stamina",338),("DefAw",275),("BallWin",312),("Aggress",384),("GKAw",326),("GKCatch",364),("GKClear",269),("GKReflex",320),("GKReach",300)]
SK=[519,523,485,497,482,287,528,525,500,495,527,506,518,513,494,499,524,505,511,507,487,484,483,493,515,512,488,490,496,521,522,501,502,491,504,517,503,530,492,516,486,489,531,526,510,529,447,520]
def load(p):
    b=open(p,'rb').read(); out={}
    for i in range(len(b)//312):
        r=b[i*312:(i+1)*312]; v=int.from_bytes(r,'little'); pid=struct.unpack_from('<I',r,8)[0]
        d={k:((v>>bit)&((1<<w)-1))+a for k,bit,w,a in BODY}
        d['ab']=[((v>>bit)&63)+40 for _,bit in AB]
        d['sk']=sum(((v>>bit)&1)<<j for j,bit in enumerate(SK))
        nm=[r[0x44+j*0x3d:0x44+(j+1)*0x3d].split(b'\0')[0].decode('utf-8','replace') for j in range(4)]
        d['name'],d['shirt'],d['screen'],d['printed']=nm; d['id']=pid; out[pid]=d
    return out
def teams(p):
    b=open(p,'rb').read(); o={}
    for i in range(len(b)//1532):
        r=b[i*1532:(i+1)*1532]; o[struct.unpack_from('<I',r,8)[0]]=r[0x170:0x170+0x46].split(b'\0')[0].decode('utf-8','replace')
    return o
def asg(p):
    b=open(p,'rb').read(); m=collections.defaultdict(list)
    for i in range(len(b)//16):
        _,pid,tid,x=struct.unpack_from('<IIII',b,i*16); m[pid].append((tid,(x&0x3ff)+1))
    return m
S={}
for k in('con','sud'):
    S[k]=dict(p=load(f'db/{k}_Player.bin'),t=teams(f'db/{k}_Team.bin'),a=asg(f'db/{k}_PlayerAssignment.bin'))
M=list(csv.DictReader(open('/mnt/project-files/pes-db/tabla-maestra/maestra_jugadores_v1.csv',encoding='utf-8-sig')))
pairs=[(int(r['id_conmegol']),int(r['id_sudamerican']),r) for r in M if r['id_conmegol'] and r['id_sudamerican']]
print('pares',len(pairs))
def diff(c,s): return sum(abs(x-y) for x,y in zip(c['ab'],s['ab']))
dd=[diff(S['con']['p'][a],S['sud']['p'][b]) for a,b,_ in pairs if a in S['con']['p'] and b in S['sud']['p']]
print('iguales',sum(1 for x in dd if x==0),'distintos',sum(1 for x in dd if x>0),'dif>30',sum(1 for x in dd if x>30))
import pickle; pickle.dump((S,pairs),open('panel/S.pkl','wb'))
