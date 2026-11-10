import struct, sys, os
sys.path.insert(0,'/mnt/project-files/pes-conmegol/herramientas')
import cpk_toc as C
def toc(path):
    f=open(path,'rb'); h=f.read(16)
    hd=C.utf(f.read(struct.unpack('<Q',h[8:16])[0]))
    f.seek(hd[0]['TocOffset']); t=f.read(16)
    rows=C.utf(f.read(struct.unpack('<Q',t[8:16])[0]))
    return f,rows,hd[0]
if __name__=='__main__':
    p=sys.argv[1]; f,rows,hd=toc(p)
    off=hd.get('ContentOffset',0)
    for r in rows:
        print(r.get('DirName'),r.get('FileName'),r.get('FileSize'),r.get('ExtractSize'),r.get('FileOffset'))
