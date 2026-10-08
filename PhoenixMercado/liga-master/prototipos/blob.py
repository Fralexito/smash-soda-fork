"""Descomprime el «blob» de un ML descifrado (ESTRUCTURA-ML §14): 18 tramos zlib de 256 KB con cabecera de 12 B entre tramos.
uso: blob.py <ML descifrado.bin> …  → escribe <nombre>_blob.bin (4.678.743 B en este guardado). No se versionan los .bin."""
import struct,zlib,sys
def extraer(ml):
    H=0x1140364; SZ=0x11403a8
    end=SZ+4+struct.unpack_from('<I',ml,SZ)[0]
    pos=0x11403d4; chunks=[]
    while pos<end-2:
        if not (ml[pos]==0x78 and ml[pos+1] in (0x01,0x5e,0x9c,0xda)):
            nxt=next((p for p in range(pos,min(pos+64,end)) if ml[p]==0x78 and ml[p+1] in (0x01,0x5e,0x9c,0xda)),None)
            if nxt is None: break
            pos=nxt
        z=zlib.decompressobj(); out=z.decompress(ml[pos:end]); used=len(ml[pos:end])-len(z.unused_data)
        chunks.append(out); pos+=used
    return b''.join(chunks)
if __name__=='__main__':
    for fn in sys.argv[1:]:
        b=extraer(open(fn,'rb').read()); open(fn.replace('.bin','')+'_blob.bin','wb').write(b); print(fn,len(b))
