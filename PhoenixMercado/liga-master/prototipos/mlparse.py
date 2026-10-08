import struct, json, re
BASE0=0x50; TSTRIDE=1680; NT=700
def load(fn): return open(fn,'rb').read()
def team_base(k): return BASE0+TSTRIDE*k
def team_name(d,k):
    b=team_base(k)+4
    raw=d[b:b+64]
    raw=raw.split(b'\0')[0]
    try: return raw.decode('utf-8','replace')
    except: return str(raw)
def roster(d,k,maxn=40):
    s=team_base(k)+0x14c
    out=[]
    for i in range(maxn):
        reg,pid=struct.unpack_from('<II',d,s+8*i)
        if reg==65535 and pid==0: break
        out.append((reg,pid))
    return out
def all_rosters(d):
    return {k:roster(d,k) for k in range(NT)}
def dorsals(d,k):
    s=team_base(k)+0x14c
    return [struct.unpack_from('<H',d,s+4+0x14a+2*i)[0] for i in range(40)]
def count_byte(d,k):
    s=team_base(k)+0x14c
    return d[s+4+0x2d6]
