# Lector de índice (TOC) de .cpk de PES 2021, solo lectura.
# Uso: python cpk_toc.py <carpeta_con_cpk> <salida.tsv>
# Salida: cpk \t FileSize \t ExtractSize \t ruta ; líneas #OK/#ERR por cpk.
# Las tablas @UTF pueden venir cifradas con el XOR estándar de CPK (m=0x655F, t=0x4115).
import struct, sys, os

def dec(b):
    m = 0x655F; o = bytearray()
    for x in b:
        o.append(x ^ (m & 0xFF)); m = (m * 0x4115) & 0xFFFF
    return bytes(o)

SZ = {0:1,1:1,2:2,3:2,4:4,5:4,6:8,7:8,8:4,0xA:4,0xB:8}
FM = {0:'>B',1:'>b',2:'>H',3:'>h',4:'>I',5:'>i',6:'>Q',7:'>q',8:'>f'}

def utf(buf):
    if buf[:4] != b'@UTF': buf = dec(buf)
    if buf[:4] != b'@UTF': return None
    _, rows_off, str_off, data_off, _, ncol, rowlen, nrows = struct.unpack('>IIIIIHHI', buf[4:32])
    base = 8; strs = buf[base+str_off:base+data_off]
    def S(o):
        e = strs.find(b'\0', o); return strs[o:e].decode('utf-8', 'replace')
    def rd(p, ty):
        if ty == 0xA: return S(struct.unpack('>I', buf[p:p+4])[0])
        if ty == 0xB:
            o, l = struct.unpack('>II', buf[p:p+8]); return buf[base+data_off+o:base+data_off+o+l]
        return struct.unpack(FM[ty], buf[p:p+SZ[ty]])[0]
    p = 32; cols = []
    for _ in range(ncol):
        fl = buf[p]; p += 1
        no = struct.unpack('>I', buf[p:p+4])[0]; p += 4
        st, ty = fl & 0xF0, fl & 0x0F; const = None
        if st == 0x30: const = rd(p, ty); p += SZ[ty]
        cols.append((S(no), st, ty, const))
    rows = []
    for r in range(nrows):
        q = base + rows_off + r*rowlen; row = {}
        for n, st, ty, const in cols:
            if st == 0x50: row[n] = rd(q, ty); q += SZ[ty]
            elif st == 0x30: row[n] = const
        rows.append(row)
    return rows

def one(path, out):
    name = os.path.basename(path)
    with open(path, 'rb') as f:
        h = f.read(16)
        if h[:4] != b'CPK ': out.write(f"#ERR\t{name}\tsin cabecera CPK\n"); return
        hd = utf(f.read(struct.unpack('<Q', h[8:16])[0]))
        if not hd: out.write(f"#ERR\t{name}\tcabecera ilegible\n"); return
        f.seek(hd[0]['TocOffset']); t = f.read(16)
        if t[:4] != b'TOC ': out.write(f"#ERR\t{name}\tsin TOC\n"); return
        rows = utf(f.read(struct.unpack('<Q', t[8:16])[0]))
        if rows is None: out.write(f"#ERR\t{name}\tTOC ilegible\n"); return
        for r in rows:
            out.write(f"{name}\t{r.get('FileSize')}\t{r.get('ExtractSize')}\t{r.get('DirName')}/{r.get('FileName')}\n")
        out.write(f"#OK\t{name}\t{len(rows)}\n")

if __name__ == '__main__':
    d = sys.argv[1]
    with open(sys.argv[2], 'w', encoding='utf-8') as o:
        for root, _, fs in os.walk(d):
            for n in sorted(fs):
                if n.lower().endswith('.cpk'):
                    try: one(os.path.join(root, n), o)
                    except Exception as e: o.write(f"#ERR\t{n}\t{e!r}\n")
                    o.flush()
