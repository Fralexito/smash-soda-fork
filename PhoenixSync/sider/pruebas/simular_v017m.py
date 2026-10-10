# Simulación de phoenix_v017m.lua en LuaJIT con un entorno falso de Sider (memoria falsa).
import sys
from lupa import luajit21 as lj
SRC = open(sys.argv[1], encoding='utf-8').read()
HARNESS = r'''
local realffi = require("ffi")
local BASE = 0x140000000
MEM = {}            -- { ini, bytes(string) }
LOGS = {}
CNT = { rpm = 0, wpm = 0, vp = 0, rpm_fuera = 0 }
function setmem(a, s) MEM[#MEM + 1] = { a, s } end
function clearmem() MEM = {} end
local function buscar(a, n)
    for i = #MEM, 1, -1 do local r = MEM[i]
        if a >= r[1] and a + n <= r[1] + #r[2] then return r[2]:sub(a - r[1] + 1, a - r[1] + n) end
    end
end
local C = {}
local function num(p) return tonumber(realffi.cast("uint64_t", realffi.cast("const void*", p))) end
C.phx06_RPM = function(proc, desde, hacia, n, leidos)
    CNT.rpm = CNT.rpm + 1
    local s = buscar(num(desde), tonumber(n))
    if not s then CNT.rpm_fuera = CNT.rpm_fuera + 1; return 0 end
    realffi.copy(hacia, s, #s); if leidos ~= nil then leidos[0] = #s end
    return 1
end
C.phx06_WPM = function(...) CNT.wpm = CNT.wpm + 1; return 0 end
C.phx06_GCP = function() return realffi.cast("void*", 1) end
C.phx11_GMH = function() return realffi.cast("void*", BASE) end
C.phx15b_VP = function(...) CNT.vp = CNT.vp + 1; return 0 end
ffi = setmetatable({ cdef = function() end, C = setmetatable(C, { __index = function(t, k) error("C." .. k .. " no simulado") end }) }, { __index = realffi })
bit = require("bit")
log = function(s) LOGS[#LOGS + 1] = s end
memory = { hex = function(a) return string.format("%x", a) end, read = function(a, n) return string.rep("\0", n) end }
function cargar(src)
    local f = assert(loadstring(src, "phoenix.lua"))
    local m = f()
    local reg = {}
    m.init({ sider_dir = "X:\\nada\\", register = function(ev, fn) reg[ev] = fn end })
    return m, reg
end
function u32(v) return string.char(v % 256, math.floor(v / 256) % 256, math.floor(v / 65536) % 256, math.floor(v / 16777216) % 256) end
function gestor(est, modo, b90)
    local t = {}
    for i = 1, 0x100 do t[i] = "\0" end
    local s = table.concat(t)
    local function put(off, x) s = s:sub(1, off) .. x .. s:sub(off + #x + 1) end
    put(0x30, u32(est)); put(0x38, u32(modo)); put(0x3c, u32(7)); put(0x44, u32(0)); put(0x90, string.char(b90, 0, 0, 0))
    return s
end
BASE_ = BASE
'''
def nuevo():
    L = lj.LuaRuntime(unpack_returned_tuples=True)
    L.execute(HARNESS)
    return L
CHK = [(0x14B6A60, bytes([72,139,5,169,243,36,2,195])), (0x13040C9, bytes([232,146,41,27,0,131,120,56,1,116,52])), (0x14B7587, bytes([68,137,65,56,69,133,192,117,7]))]
def codigo(L, ok=True):
    for rva, b in CHK:
        bb = b if ok else bytes([0x90])*len(b)
        L.globals().setmem(0x140000000 + rva, bb)
def ptr(L, a): L.globals().setmem(0x140000000 + 0x3705E10, a.to_bytes(8, 'little'))
def logs(L): return [str(x) for x in L.globals().LOGS.values()]
def run(nombre, prep):
    L = nuevo(); g = L.globals()
    m, reg = g.cargar(SRC)
    res = prep(L, g, m, reg)
    c = g.CNT
    print('== %s' % nombre)
    for l in logs(L):
        if 'modo' in l: print('   LOG', l[:200])
    print('   overlay [MODO]:', res)
    print('   contadores: RPM=%d  WPM(escrituras)=%d  VirtualProtect=%d' % (c.rpm, c.wpm, c.vp))
    return L, c
def modo_line(txt):
    for l in str(txt).split('\n'):
        if '[MODO]' in l: return l.strip()[:190]
G = 0x000001F2A3B40000
def caso1(L, g, m, reg):
    codigo(L); ptr(L, G); g.setmem(G, g.gestor(4, 0, 0))
    w0, v0 = g.CNT.wpm, g.CNT.vp
    reg['key_down'](None, 0x4D)                      # tecla M con modo 0
    assert g.CNT.wpm == w0 and g.CNT.vp == v0, 'la tecla M escribió'
    g.setmem(G, g.gestor(1, 1, 0))
    reg['key_down'](None, 0x4D)                      # modo 1
    g.setmem(G, g.gestor(4, 2, 1))
    t = reg['overlay_on'](None)
    assert g.CNT.wpm == w0 and g.CNT.vp == v0, 'el overlay escribió'
    return modo_line(t)
def caso2(L, g, m, reg):                              # exe distinto
    codigo(L, ok=False); ptr(L, G); g.setmem(G, g.gestor(4, 1, 0))
    reg['key_down'](None, 0x4D); r0 = g.CNT.rpm
    reg['key_down'](None, 0x4D)
    assert g.CNT.rpm == r0, 'siguió leyendo con exe distinto'
    return modo_line(reg['overlay_on'](None))
def caso3(L, g, m, reg):                              # gestor aún no existe (puntero 0)
    codigo(L); ptr(L, 0)
    reg['key_down'](None, 0x4D)
    return modo_line(reg['overlay_on'](None))
def caso4(L, g, m, reg):                              # gestor liberado (zona ilegible) y puntero raro
    codigo(L); ptr(L, G)
    reg['key_down'](None, 0x4D)
    a = modo_line(reg['overlay_on'](None))
    ptr(L, 0xFFFF800012345678)
    reg['key_down'](None, 0x4D)
    return a + '  ||  ' + modo_line(reg['overlay_on'](None))
def caso5(L, g, m, reg):                              # durante «Activar»: lecturas de player.bin
    codigo(L); ptr(L, G); g.setmem(G, g.gestor(4, 0, 0))
    f = reg['livecpk_read']
    f(None, 'common\\etc\\pesdb\\Player.bin', 0, 16, 1769252, 0)      # arranque
    g.setmem(G, g.gestor(1, 1, 0)); f(None, 'common\\etc\\pesdb\\Player.bin', 0, 16, 1769252, 0)   # estado 22: modo 1
    g.setmem(G, g.gestor(1, 0, 0)); f(None, 'common\\etc\\pesdb\\Player.bin', 0, 16, 1769252, 0)   # carga EDIT: modo 0
    f(None, 'common\\etc\\pesdb\\Team.bin', 0, 16, 42618, 0)
    f(None, 'otra\\cosa.bin', 0, 16, 10, 0)
    return modo_line(reg['overlay_on'](None))
def caso6(L, g, m, reg):                              # Shift+R: el módulo se carga dos veces en el mismo Lua
    codigo(L); ptr(L, G); g.setmem(G, g.gestor(4, 1, 0))
    m2, reg2 = g.cargar(SRC)
    reg2['key_down'](None, 0x4D)
    return modo_line(reg2['overlay_on'](None))
for n, f in (('1 normal: 0 → 1 → 2', caso1), ('2 exe distinto', caso2), ('3 gestor no existe', caso3), ('4 gestor ilegible / puntero raro', caso4), ('5 lecturas de player.bin durante Activar', caso5), ('6 módulo recargado (Shift+R)', caso6)):
    run(n, f)
print('TODO OK')
