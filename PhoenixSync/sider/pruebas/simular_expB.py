# Simulación del EXPERIMENTO B: con los bytes originales en memoria falsa, el arranque debe escribir
# SOLO los parches A y B, y dejar C tal cual. Compara con la v0.17e (que escribe A, B y C).
# Uso: python3 simular_expB.py phoenix-v0.17e-B-prueba.lua phoenix-v0.17e-prueba.lua  (junto a simular_v017m.py)
import sys
src_sim = open('simular_v017m.py', encoding='utf-8').read()
exec(src_sim[:src_sim.index("for n, f in (('1 normal")])
BASE = 0x140000000
ORIG = {'A': (0x20AF73B, bytes([72,131,191,152,0,0,0,0,117,107,51,219,137,92,36,40,199,68,36])),
        'B': (0x20AC6B9, bytes([232,82,226,9,255,72,133,192,116,16,72,141,21,22,83])),
        'C': (0x20AE664, bytes([102,199,68,36,48,0,1]))}
NUEVO = {'A': bytes([199,135,148,0,0,0,3,0,0,0,235,115]), 'B': bytes([199,135,148,0,0,0,22,0,0,0,233,186,37,0,0]), 'C': bytes([102,199,68,36,48,1,1])}
def correr(src, nombre, c_ya_puesto=False):
    L = nuevo(); g = L.globals()
    # memoria escribible: WPM escribe de verdad en la memoria falsa; VirtualProtect responde OK
    L.execute('''
        local rf = require("ffi")
        ffi.C.phx15b_VP = function(...) CNT.vp = CNT.vp + 1; return 1 end
        ESCRITO = {}
        ffi.C.phx06_WPM = function(p, hacia, desde, n, w)
            CNT.wpm = CNT.wpm + 1
            local a = tonumber(rf.cast("uint64_t", rf.cast("const void*", hacia))); n = tonumber(n)
            local s = rf.string(desde, n); setmem(a, s); ESCRITO[#ESCRITO + 1] = string.format("%X", a - 0x140000000)
            if w ~= nil then w[0] = n end
            return 1
        end''')
    m, reg = g.cargar(open(src, encoding='utf-8').read())
    codigo(L); ptr(L, G); g.setmem(G, g.gestor(4, 0, 0))
    for k, (rva, b) in ORIG.items():
        g.setmem(BASE + rva, (NUEVO['C'] if (k == 'C' and c_ya_puesto) else b) + bytes(32))
    reg['livecpk_read'](None, 'common\\etc\\pesdb\\Player.bin', 0, 16, 1769252, 0)   # arranque: aplica el botón nativo
    escritos = sorted(set(str(x) for x in g.ESCRITO.values()))
    estado = {}
    L.execute('function leer(a, n) for i = #MEM, 1, -1 do local r = MEM[i]; if a >= r[1] and a + n <= r[1] + #r[2] then local t = r[2]:sub(a - r[1] + 1, a - r[1] + n); local h = {}; for j = 1, #t do h[j] = string.format("%02x", t:byte(j)) end; return table.concat(h) end end end')
    for k, (rva, b) in ORIG.items():
        cur = bytes.fromhex(g.leer(BASE + rva, len(NUEVO[k])))
        estado[k] = 'parcheado' if cur == NUEVO[k] else ('original' if cur == b[:len(NUEVO[k])] else 'otro')
    bot = [l for l in logs(L) if 'experimento B' in l or 'parche' in l or 'auto' in l]
    print('== %s%s' % (nombre, ' (C ya puesto antes)' if c_ya_puesto else ''))
    print('   escrituras en: %s' % ', '.join(escritos))
    print('   estado final: A=%s · B=%s · C=%s' % (estado['A'], estado['B'], estado['C']))
    for l in bot: print('   LOG ' + l[10:150])
    return estado
e1 = correr(sys.argv[1], 'experimento B')
e2 = correr(sys.argv[2], 'v0.17e (referencia)')
e3 = correr(sys.argv[1], 'experimento B', c_ya_puesto=True)
ok = e1 == {'A': 'parcheado', 'B': 'parcheado', 'C': 'original'} and e2 == {'A': 'parcheado', 'B': 'parcheado', 'C': 'parcheado'} and e3['C'] == 'parcheado'
print('TODO OK' if ok else 'HAY FALLOS')
