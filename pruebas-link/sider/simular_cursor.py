"""Simulación de phoenix_cursor_prueba.lua fuera del juego (LuaJIT de lupa + memoria REAL de este proceso).
Imita lo que Sider 7.3.3 da (memory, match, fs, log, ctx, VirtualQuery ya declarado) y repasa todos los pasos:
búsqueda «menu» → se planta un «futbolista» con el ID → búsqueda «partido» → fotos A/N/B → gol → búsqueda «gol».
Después corre analizar_cursor.py sobre la carpeta generada: debe encontrar el byte +0x40 como candidato fuerte.
Uso:  py simular_cursor.py
"""
import os
import shutil
import sys
import time
import tempfile

from lupa.luajit21 import LuaRuntime

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import analizar_cursor  # noqa: E402

tmp = tempfile.mkdtemp(prefix="cursor_sim_")
sider_dir = tmp + "\\"
os.makedirs(os.path.join(tmp, "content", "phoenix"))

sys.stdout.reconfigure(encoding="utf-8")
lua = LuaRuntime(unpack_returned_tuples=True)
lua.globals().py_makedirs = lambda p: os.makedirs(p, exist_ok=True)
lua.execute(r"""
ffi = require("ffi"); bit = require("bit")
ffi.cdef[[
typedef struct { void* BaseAddress; void* AllocationBase; uint32_t AllocationProtect; uint16_t PartitionId;
                 size_t RegionSize; uint32_t State; uint32_t Protect; uint32_t Type; } MEMORY_BASIC_INFORMATION;
size_t VirtualQuery(const void* dir, MEMORY_BASIC_INFORMATION* mbi, size_t n);
]]
LOG = {}
function log(s) LOG[#LOG + 1] = s; print("LOG", s) end
memory = {}
function memory.get_system_info()
  return { lpMinimumApplicationAddress = ffi.cast("void*", 0x10000), lpMaximumApplicationAddress = ffi.cast("void*", 0x7FFFFFFEFFFF) }
end
function memory.search(pat, s, e)
  local p = ffi.cast("const uint8_t*", s); local n = e - s - #pat
  local b0, b1, b2, b3 = pat:byte(1, 4)
  for i = 0, n do
    if p[i] == b0 and p[i + 1] == b1 and p[i + 2] == b2 and p[i + 3] == b3 then return s + i end
  end
  return nil
end
SCORE = nil
match = { stats = function() return SCORE end }
fs = { make_dirs = function(p) py_makedirs(p) end }
HANDLERS = {}
CTX = { register = function(ev, f) HANDLERS[ev] = f end }
""")
lua.globals().CTX.sider_dir = sider_dir
src = open(os.path.join(AQUI, "phoenix_cursor_prueba.lua"), encoding="utf-8").read()
mod = lua.execute(src)
mod.init(lua.globals().CTX)
H = lua.globals().HANDLERS
ctx = lua.globals().CTX

def frames(max_n=100000):
    for _ in range(max_n):
        H.display_frame(ctx)
        if lua.eval("true") and not overlay_buscando():
            return
    raise RuntimeError("la búsqueda no terminó")

def overlay_buscando():
    return "buscando" in H.overlay_on(ctx)

def tecla(vk):
    H.key_down(ctx, vk)

J = 0x4A
tecla(J)                       # paso 1: búsqueda «menu»
frames()
# «Entra al partido»: aparece un futbolista con el ID en +0x1000 de un bloque nuevo
lua.execute("""
ENT = ffi.new("uint8_t[?]", 0x2000)
local id = ffi.cast("uint32_t*", ENT + 0x1000); id[0] = 162114
ENT[0x1040] = 0xFF   -- quién lo controla (0xFF = nadie)
SCORE = { home_score = 0, away_score = 0, clock_minutes = 10 }
""")
t_fin = time.time() + 1.0
while time.time() < t_fin:      # el módulo toma el 0-0 como punto de partida
    H.display_frame(ctx)
tecla(J)                       # paso 2: búsqueda «partido»
frames()
for nombre, valor in (("A1", 0), ("N1", 0xFF), ("B1", 1), ("A2", 0), ("B2", 1), ("N2", 0xFF)):
    lua.execute(f"ENT[0x1040] = {valor}; ENT[0x1100] = math.random(0,255)")  # +0x100 = ruido
    for _ in range(20):
        H.display_frame(ctx)
    tecla(J)
# gol del local; tras el gol aparece un registro nuevo con el ID
lua.execute("""
SCORE = { home_score = 1, away_score = 0, clock_minutes = 23 }
GOLREG = ffi.new("uint8_t[?]", 0x100); ffi.cast("uint32_t*", GOLREG + 0x20)[0] = 162114; GOLREG[0x24] = 23
""")
t_end = time.time() + 2.5
while time.time() < t_end:
    H.display_frame(ctx)
tecla(J)                       # paso 9: búsqueda «gol»
frames()
for _ in range(10):
    H.display_frame(ctx)
print(H.overlay_on(ctx))
carpeta = os.path.join(tmp, "content", "phoenix", "cursor")
print("\nArchivos:", sorted(os.listdir(carpeta)))
analizar_cursor.analizar(carpeta)
shutil.rmtree(tmp, ignore_errors=True)
