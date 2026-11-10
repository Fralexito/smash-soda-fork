import sys,struct,zlib
sys.path.insert(0,'/tmp/claude-0/-home-user/51590cf9-7efa-5921-bdfe-6d8e02aa66d1/scratchpad/tools')
import cpkx
def get(cpk,name):
    f,rows,hd=cpkx.toc(cpk)
    for r in rows:
        if r['FileName']==name and 'pesdb' in r['DirName']:
            f.seek(r['FileOffset']+hd['ContentOffset']); return f.read(r['FileSize'])
def unwesys(b):
    # header: 3 bytes + b'WESYS' + comp(le32) + uncomp(le32); if comp==uncomp data is stored raw
    if b[3:8]!=b'WESYS': return b
    comp,unc=struct.unpack('<II',b[8:16])
    body=b[16:16+comp]
    return body if comp==unc else zlib.decompress(body)
if __name__=='__main__':
    cpk,name,out=sys.argv[1:4]
    b=get(cpk,name); d=unwesys(b); print(name,len(b),'->',len(d)); open(out,'wb').write(d)
