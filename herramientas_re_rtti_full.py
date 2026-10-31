# Extensión del mapa Fox (Parte 1): RTTI MSVC COMPLETO de PES2021.exe (todas las clases con vftable), jerarquía y clasificación por tema.
# Reusa pe.py (PE/BASE). Salida: clases_pes.csv, clases_pes.json
import re, struct, json, csv, sys
sys.path.insert(0,'.')
from pe import PE, BASE
p = PE()
secs = {s[0]: s for s in p.secs}
rd, dt = secs['.rdata'], secs['.data']
def sb(s): return bytes(p.m[s[3]:s[3]+s[4]])
RD, DT = sb(rd), sb(dt)
CODE_SECS = [s for s in p.secs if s[0] in ('.trace','.text')]
def is_code(q):
    r = q - BASE
    return any(va <= r < va + vs for n, va, vs, ro, rs in CODE_SECS)
# 1) TypeDescriptors: '.?AV<n1>@<n2>@...@@\0' ; el TD empieza 16 B antes (vtable ptr + spare)
pat = re.compile(rb'\.\?A[VU]([A-Za-z0-9_$?<>,\-]+(?:@[A-Za-z0-9_$?<>,\-]+)*)@@\x00')
tds = {}
for S, sd in ((RD, rd), (DT, dt)):
    for m in pat.finditer(S):
        parts = m.group(1).decode('latin1').split('@')
        tds[sd[1] + m.start() - 16] = '::'.join(reversed(parts))
print('TypeDescriptors', len(tds))
# 2) COLs (rdata): sig=1, pTypeDescriptor rva en +12, self en +20
tdset = set(tds)
cols = {}
for i in range(0, len(RD) - 24, 4):
    if struct.unpack_from('<I', RD, i)[0] != 1: continue
    td = struct.unpack_from('<I', RD, i + 12)[0]
    if td in tdset and struct.unpack_from('<I', RD, i + 20)[0] == rd[1] + i:
        cols[rd[1] + i] = (tds[td], td, struct.unpack_from('<I', RD, i + 16)[0], struct.unpack_from('<I', RD, i + 4)[0])
print('COLs', len(cols))
# 3) vftables: qword == BASE+col_rva (en rdata); la vftable empieza 8 B después
colmap = {BASE + c: c for c in cols}
vft = {}
for i in range(0, len(RD) - 8, 8):
    q = struct.unpack_from('<Q', RD, i)[0]
    c = colmap.get(q)
    if c: vft.setdefault(c, []).append(BASE + rd[1] + i + 8)
def slots_of(v):
    o = p.off(v - BASE); out = []
    for k in range(0, 400):
        q = struct.unpack_from('<Q', p.m, o + 8 * k)[0]
        if not is_code(q): break
        out.append(q)
    return out
def bases_of(chd_rva):
    o = p.off(chd_rva)
    if o is None: return []
    n = struct.unpack_from('<I', p.m, o + 8)[0]; arr = struct.unpack_from('<I', p.m, o + 12)[0]
    ao = p.off(arr); res = []
    for k in range(min(n, 40)):
        bcd = struct.unpack_from('<I', p.m, ao + 4 * k)[0]; bo = p.off(bcd)
        td = struct.unpack_from('<I', p.m, bo)[0]; res.append(tds.get(td, '?'))
    return res
TEMAS = [
 ('balon_fisica', r'\bAndPh|Ball|Physics|Rigid|Collision|\bPh[A-Z]|Hit|Goalpost|GoalNet'),
 ('input_comandos', r'CommandPlayer|ThinkUnit|\bPad|Command|Input|Controller|Cursor'),
 ('jugadores_anim', r'AnimePlayer|anime::|Player|Motion|Action|Keeper|Stamina|Formation|Tactics'),
 ('partido_reglas', r'Referee|Rule|Offside|Foul|Card|Penalty|Corner|ThrowIn|GoalKick|Kickoff|Match|Score|Timer|Clock|Training|Restart'),
 ('camara', r'Camera|Cam[A-Z]|Replay|Director|Cut'),
 ('estadio_render', r'Stadium|Pitch|Grass|Render|Draw|Shader|Model|Material|Light|Effect|Particle|Texture|Fmdl|Geom|Screen2d|Gauge|Hud|Ui\b|Scoreboard|Banner|Audience|Crowd'),
 ('sonido_video', r'Sound|Audio|Voice|Movie|Video|Sound'),
 ('red_online', r'Network|Online|Session|Socket|Lobby|Matching|Peer|Packet|Server'),
 ('fox_motor', r'^fox::|Fox|Gr::|gr::|Tpp|Fx|Scene|Entity|Resource|Archive|Package|Job|Task|Thread|Allocator|Memory|File'),
 ('juego_menus', r'Menu|Edit|Option|Setting|League|Cup|Career|Team|Roster|Season|Transfer|Save|Profile|Master'),
]
def tema(n):
    for t, rx in TEMAS:
        if re.search(rx, n): return t
    return 'otros'
rows = []
for c, (name, td, chd, off) in sorted(cols.items(), key=lambda kv: kv[1][0]):
    for v in vft.get(c, []):
        sl = slots_of(v)
        sig = ''
        for q in sl:
            o = p.off(q - BASE)
            if o is None: continue
            b = bytes(p.m[o:o + 16])
            if len(b) == 16 and b[0] != 0xC3 and not (b[0] == 0x33 and b[1] == 0xC0):   # saltar ranuras triviales (ret / xor eax,eax)
                sig = b.hex(' ').upper(); break
        bs = bases_of(chd)
        rows.append(dict(clase=name, vtable_rva=hex(v - BASE), vtable=hex(v), ranuras=len(sl), offset_vptr=off, bases=';'.join(bs[1:]), tema=tema(name), firma_slot=sig))
print('vftables', len(rows))
json.dump(rows, open('clases_pes.json', 'w'), indent=1, ensure_ascii=False)
with open('clases_pes.csv', 'w', newline='', encoding='utf-8') as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
from collections import Counter
print(Counter(r['tema'] for r in rows))
