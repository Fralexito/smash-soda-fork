# Herramientas de SOLO LECTURA sobre una copia de PES2021.exe
import numpy as np, struct, bisect, re, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
F = np.memmap('PES2021.exe', dtype=np.uint8, mode='r')
SECS = {  # nombre: (rva, vsize, rawptr, rawsize)
 '.trace':(0x1000,0x252d800,0x600,0x252d800), '.rdata':(0x252f000,0xe6c000,0x252de00,0xe6bc00),
 '.data':(0x339b000,0x72a000,0x3399a00,0x334800), '.pdata':(0x3ac5000,0x1d1000,0x36ce200,0x1d0e00),
 '.bss':(0x3c98000,0x9000,0x389f400,0x8e00), '.data1':(0x3ca1000,0x73000,0x38a8200,0x72a00)}
def off(rva):
    for n,(r,vs,rp,rs) in SECS.items():
        if r <= rva < r+rs: return rp + (rva-r)
    return None
def sec(rva):
    for n,(r,vs,rp,rs) in SECS.items():
        if r <= rva < r+vs: return n
    return '?'
def rd(rva, n):
    o = off(rva)
    return bytes(F[o:o+n]) if o is not None else None
CODE = bytes(F[0x600:0x600+0x252d800]); CODE_RVA = 0x1000
RDATA = bytes(F[0x252de00:0x252de00+0xe6bc00]); RDATA_RVA = 0x252f000
_raw = bytes(F[0x36ce200:0x36ce200+0x1d0e00]); _raw = _raw[:len(_raw)//12*12]
_pd = np.frombuffer(_raw, dtype=np.uint32).reshape(-1,3)
_pd = _pd[_pd[:,0] != 0]
FB = _pd[:,0].tolist(); FE = _pd[:,1].tolist()
def func(rva):
    i = bisect.bisect_right(FB, rva)-1
    if i >= 0 and FB[i] <= rva < FE[i]: return FB[i], FE[i]
    return None
md = Cs(CS_ARCH_X86, CS_MODE_64); md.detail = False
def dis(rva, n=0x60, stop=None):
    b = CODE[rva-CODE_RVA: rva-CODE_RVA+n]; out=[]
    for i in md.disasm(b, 0x140000000+rva):
        out.append(i)
    return out
def show(rva, n=0x60, mark=None):
    for i in dis(rva, n):
        r = i.address-0x140000000
        s = '%8X  %-7s %s' % (r, i.mnemonic, i.op_str)
        m = re.search(r'rip ([+-]) (0x[0-9a-f]+)', i.op_str)
        if m:
            t = r + i.size + (int(m.group(2),16) if m.group(1)=='+' else -int(m.group(2),16))
            s += '    ; -> %X [%s]' % (t, sec(t))
            st = cstr(t)
            if st: s += ' "%s"' % st
        m2 = re.match(r'^0x([0-9a-f]+)$', i.op_str)
        if m2 and i.mnemonic in ('call','jmp'): pass
        print(s)
def cstr(rva, mx=90):
    if not (RDATA_RVA <= rva < RDATA_RVA+len(RDATA)): return None
    o = rva-RDATA_RVA; e = RDATA.find(b'\0', o, o+mx)
    if e <= o+3: return None
    s = RDATA[o:e]
    if all(32 <= c < 127 for c in s): return s.decode()
    return None
_A = None
def _arr():
    global _A
    if _A is None:
        c = np.frombuffer(CODE, dtype=np.uint8)
        n = len(c)-8
        d = (c[0:n].astype(np.uint32) | (c[1:n+1].astype(np.uint32)<<8) | (c[2:n+2].astype(np.uint32)<<16) | (c[3:n+3].astype(np.uint32)<<24))
        _A = d + np.arange(n, dtype=np.uint32) + np.uint32(CODE_RVA+4)   # destino si la instrucción acaba tras el disp32
    return _A
def xrefs(target, extras=(0,1,2,4)):
    """Posiciones (rva del disp32) cuyo destino RIP-relativo es target. extras = bytes inmediatos tras el disp."""
    a = _arr(); res=[]
    for ex in extras:
        idx = np.nonzero(a == np.uint32(target-ex))[0]
        for i in idx.tolist(): res.append((i+CODE_RVA, ex))
    return sorted(res)
def insn_at_disp(dr, back=12):
    """Devuelve la instrucción que contiene el disp32 en rva dr (probando inicios hacia atrás dentro de la función)."""
    f = func(dr)
    if not f: return None
    # desensamblar linealmente desde el inicio de la función
    b = CODE[f[0]-CODE_RVA: f[1]-CODE_RVA]
    for i in md.disasm(b, 0x140000000+f[0]):
        r = i.address-0x140000000
        if r <= dr < r+i.size: return i
        if r > dr: break
    return None

def funcs_in(a,b):
    i = bisect.bisect_left(FB,a); out=[]
    while i < len(FB) and FB[i] < b: out.append((FB[i],FE[i])); i+=1
    return out
def brief(rva, n):
    """Solo llamadas, referencias RIP y escrituras/comparaciones con inmediato."""
    for i in dis(rva, n):
        r = i.address-0x140000000; o = i.op_str
        keep = i.mnemonic in ('call',) or 'rip' in o or (i.mnemonic in ('cmp','mov','test','sub','add') and re.search(r'\[r\w+ \+ 0x[0-9a-f]+\], (0x)?[0-9a-f]+$', o)) or (i.mnemonic=='cmp' and re.search(r'\[r\w+ \+ 0x', o)) or (i.mnemonic=='jmp' and o.startswith('0x'))
        if not keep: continue
        s = '%8X  %-6s %s' % (r, i.mnemonic, o)
        m = re.search(r'rip ([+-]) (0x[0-9a-f]+)', o)
        if m:
            t = r + i.size + (int(m.group(2),16) if m.group(1)=='+' else -int(m.group(2),16))
            s += '    ; -> %X [%s]' % (t, sec(t)); st = cstr(t)
            if st: s += ' "%s"' % st
        print(s)

def callers(target):
    """Sitios con call/jmp rel32 (E8/E9) hacia target."""
    c = np.frombuffer(CODE, dtype=np.uint8); a = _arr()
    idx = np.nonzero(a == np.uint32(target))[0]; out=[]
    for i in idx.tolist():
        if i>0 and CODE[i-1] in (0xE8,0xE9): out.append(i-1+CODE_RVA)
    return out
