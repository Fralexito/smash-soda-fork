# Simulación de la v0.17e: las teclas de prueba (B, K, L, P, U) no hacen nada; M sigue leyendo.
# Compara con una versión anterior (segundo argumento) para demostrar la diferencia.
# Uso: python3 simular_v017e.py phoenix-v0.17e-prueba.lua phoenix-v0.17d-prueba.lua   (junto a simular_v017m.py)
import sys
src_sim = open('simular_v017m.py', encoding='utf-8').read()
cab = src_sim[:src_sim.index("for n, f in (('1 normal")]
exec(cab)
NUEVA = SRC
VIEJA = open(sys.argv[2], encoding='utf-8').read()
BASE = 0x140000000
G_ED = 0x000001F2C0000000
def preparar(L):
    """Memoria falsa en la que las teclas antiguas SÍ actuarían: código esperado, interruptor en 0, gestor de edición vivo y libre."""
    g = L.globals(); codigo(L); ptr(L, G); g.setmem(G, g.gestor(4, 0, 0))
    g.setmem(BASE + 0xAEF770, bytes([128,61,194,100,208,2,0])); g.setmem(BASE + 0x1EFB440, bytes([136,13,243,167,143,1,195]))
    g.setmem(BASE + 0x37F5C39, bytes([0]))
    # contador de llamadas a funciones del juego: se intercepta ffi.cast a punteros de función
    L.execute('''LLAMADAS = 0
        local rf = require("ffi"); local viejo = ffi.cast
        ffi = setmetatable({ cdef = ffi.cdef, C = ffi.C, cast = function(t, v)
            if type(t) == "string" and t:find("%(%*%)") then return function(...) LLAMADAS = LLAMADAS + 1; return 0 end end
            return viejo(t, v) end }, { __index = rf })''')
TECLAS = [('B', 0x42), ('K', 0x4B), ('L', 0x4C), ('P', 0x50), ('U', 0x55)]
def probar(src, nombre):
    out = {}
    for letra, vk in TECLAS:
        L = nuevo(); g = L.globals(); m, reg = g.cargar(src); preparar(L)
        actuo = False
        try:
            reg['key_down'](None, vk)
        except Exception:
            actuo = True          # la tecla entró en su acción (la simulación no reproduce esa acción entera)
        ign = any('tecla %s ignorada' % letra in l for l in logs(L))
        out[letra] = (g.CNT.wpm, g.CNT.vp, g.LLAMADAS, ign, actuo)
    print('== ' + nombre)
    for letra, (wpm, vp, ll, ign, actuo) in out.items():
        print('   tecla %s → escrituras %d · VirtualProtect %d · llamadas a funciones del juego %d · anotada como ignorada: %s%s' % (letra, wpm, vp, ll, 'sí' if ign else 'no', ' · entró en su acción' if actuo else ''))
    return out
a = probar(NUEVA, 'v0.17e (teclas desactivadas)')
b = probar(VIEJA, 'versión anterior (teclas activas)')
ok = all(w == 0 and v == 0 and ll == 0 and ign and not act for (w, v, ll, ign, act) in a.values())
dif = sum(1 for k in a if (b[k][0] + b[k][1] + b[k][2]) > 0 or b[k][4])
# M sigue leyendo y el overlay avisa
L = nuevo(); g = L.globals(); m, reg = g.cargar(NUEVA); preparar(L)
reg['key_down'](None, 0x4D); reg['key_down'](None, 0x4B)
ov = str(reg['overlay_on'](None))
lm = any('modo (tecla M)' in l for l in logs(L))
lt = [l.strip() for l in ov.split('\n') if '[TECLAS]' in l]
pistas = [l.strip()[:70] for l in ov.split('\n') if 'desactivada' in l and '[TECLAS]' not in l]
print('== tecla M en la v0.17e: muestra anotada = %s' % ('sí' if lm else 'no'))
print('   ' + (lt[0] if lt else 'SIN LÍNEA [TECLAS]'))
for p in pistas: print('   ' + p)
print('teclas que en la versión anterior actuaban (escribían, llamaban o buscaban): %d de 5' % dif)
print('TODO OK' if ok and lm and lt and 'última ignorada: K' in lt[0] else 'HAY FALLOS')
