# Simulación de la v0.18: TRES MODOS DE RECARGA (ACTIVAR por defecto · AUTO-FICHAJES · AUTO-SIEMPRE)
# con memoria, archivos y reloj falsos. Reutiliza el entorno de simular_v017m.py / simular_v017d.py.
# Uso: python3 simular_v018.py phoenix-v0.18-prueba.lua   (en la misma carpeta que los otros simuladores)
import sys
src_sim = open('simular_v017d.py', encoding='utf-8').read()
exec(src_sim[:src_sim.index("caso('D1")])
BASE = 0x140000000
EXTRA_LUA = r'''
local rf = require("ffi")
FILES = {}
io.open = function(p, mo)
    local n = p:match("([^\\]+)$"); local c = FILES[n]
    if c == nil then return nil end
    return { read = function(self, k) return c:sub(1, tonumber(k) or #c) end, close = function() end }
end
ESCRITO = {}
function patch(a, s)       -- escribe dentro de la región más nueva que contiene [a, a+#s)
    for i = #MEM, 1, -1 do local r = MEM[i]
        if a >= r[1] and a + #s <= r[1] + #r[2] then local o = a - r[1]; r[2] = r[2]:sub(1, o) .. s .. r[2]:sub(o + #s + 1); return end
    end
    setmem(a, s)
end
ffi.C.phx15b_VP = function(d, n, nueva, vieja) CNT.vp = CNT.vp + 1; if vieja ~= nil then vieja[0] = 0x20 end; return 1 end
ffi.C.phx06_WPM = function(p, hacia, desde, n, w)
    CNT.wpm = CNT.wpm + 1
    local a = tonumber(rf.cast("uint64_t", rf.cast("const void*", hacia))); n = tonumber(n)
    patch(a, rf.string(desde, n)); ESCRITO[#ESCRITO + 1] = string.format("%X", a - 0x140000000)
    if w ~= nil then w[0] = n end
    return 1
end
function leer1(a) for i = #MEM, 1, -1 do local r = MEM[i]; if a >= r[1] and a < r[1] + #r[2] then return r[2]:byte(a - r[1] + 1) end end end
'''
PAT = {0x20AF73B: bytes([72,131,191,152,0,0,0,0,117,107,51,219,137,92,36,40,199,68,36]),
       0x20AC6B9: bytes([232,82,226,9,255,72,133,192,116,16,72,141,21,22,83]),
       0x20AE664: bytes([102,199,68,36,48,0,1]),
       0xAEF770: bytes([128,61,194,100,208,2,0]), 0x1EFB440: bytes([136,13,243,167,143,1,195]),
       0xAEF78A: bytes([199,68,36,32,0,1,0,0])}
BANDERA = BASE + 0x37F5C39
def montar(archivos=None, exe_ok=True):
    L = nuevo(); L.execute(EXTRA_LUA); g = L.globals()
    for k, v in (archivos or {}).items(): g.FILES[k] = v
    m, reg = g.cargar(SRC)
    todo(L)
    for rva, b in PAT.items():
        if rva == 0xAEF770 and not exe_ok: b = bytes([0x90]) * len(b)
        g.setmem(BASE + rva, b + bytes(16))
    g.setmem(BANDERA, bytes([0]))
    return L, g, reg
def tick(L, reg, n=1):
    for _ in range(n): L.execute('RELOJC = RELOJC + 3'); reg['display_frame'](None)
def bandera(g): return g.leer1(BANDERA)
def param(g): return g.leer1(BASE + 0xAEF78E)
def esc(g): return [str(x) for x in g.ESCRITO.values()]
def linea_modo(reg):
    t = str(reg['overlay_on'](None)).split('\n')
    i = [k for k, l in enumerate(t) if '[MODO PHOENIX]' in l][0]
    out = [t[i].strip()]
    for l in t[i + 1:i + 3]:
        if '[AUTO]' in l: out.append(l.strip())
    return ' | '.join(out)
res = []
def caso(nombre, f):
    L, g, reg, ok, notas = f()
    print(('OK  ' if ok else 'MAL ') + nombre)
    for n_ in notas: print('      ' + str(n_)[:200])
    for l in logs(L):
        if 'v0.18' in l: print('      LOG ' + l[10:190])
    res.append(ok)
def boot(L, g, reg):
    arrancar(L, g, reg); return len(esc(g))      # el arranque escribe A, B y C (botón nativo, como la v0.17e)

def s1():   # sin modo.txt → ACTIVAR: tras el arranque no se lee la memoria ni se escribe nada, pase lo que pase
    L, g, reg = montar({'recargar.txt': 'v1'})
    tick(L, reg); n0 = boot(L, g, reg); r0 = g.CNT.rpm
    for zona, cosa in ((7, None), (8, 'v2'), (7, None), (13, 'v3'), (19, None), (7, None)):
        modos(L, zona)
        if cosa: g.FILES['recargar.txt'] = cosa
        tick(L, reg, 3)
    ok = n0 == 3 and len(esc(g)) == 3 and g.CNT.rpm == r0 and bandera(g) == 0
    return L, g, reg, ok, ['escrituras del arranque: %d (A, B, C) · después: %d · lecturas de memoria en 18 pasos: %d' % (n0, len(esc(g)) - n0, g.CNT.rpm - r0), linea_modo(reg)]

def s2():   # AUTO-SIEMPRE: no recarga en la primera entrada; sí al volver al menú desde un modo
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE\r\n'})
    n0 = boot(L, g, reg); tick(L, reg, 3)
    a = len(esc(g)) - n0                                   # menú tras arrancar: nada
    modos(L, 8); tick(L, reg, 3); b = len(esc(g)) - n0     # en Partido: nada
    modos(L, 7); tick(L, reg); c = esc(g)[n0:]             # vuelve al menú, momento seguro → 1 byte de código + interruptor
    tick(L, reg, 3); d = len(esc(g)) - n0                  # no repite
    linea1 = linea_modo(reg)
    g.patch(BANDERA, bytes([0])); modos(L, 8); tick(L, reg)   # el juego lo consume al entrar a Partido
    linea2 = linea_modo(reg)
    modos(L, 7); tick(L, reg); e = esc(g)[n0:]             # otra vuelta al menú → solo el interruptor (el byte ya está)
    ok = a == 0 and b == 0 and c == ['AEF78E', '37F5C39'] and d == 2 and e == ['AEF78E', '37F5C39', '37F5C39'] and bandera(g) == 1 and param(g) == 1
    return L, g, reg, ok, ['menú tras arrancar: %d escrituras · en Partido: %d · al volver: %s · repetido: %d · segunda vuelta: %s' % (a, b, c, d - 2, e[2:]), linea1, linea2]

def s3():   # AUTO-FICHAJES: solo con aviso nuevo, y solo en el momento seguro
    L, g, reg = montar({'modo.txt': 'auto-fichajes', 'recargar.txt': '2026-10-10 09:00'})
    n0 = boot(L, g, reg); tick(L, reg, 3)
    a = len(esc(g)) - n0                                   # el aviso que ya había al abrir no cuenta
    modos(L, 8); g.FILES['recargar.txt'] = '2026-10-10 10:30'; tick(L, reg, 3)
    b = len(esc(g)) - n0                                   # aviso nuevo en Partido: espera
    pend = linea_modo(reg)
    modos(L, 7); base_bd(L, 1); tick(L, reg, 2)
    c = len(esc(g)) - n0                                   # menú pero la base cargando: espera
    base_bd(L, 4); tick(L, reg); d = esc(g)[n0:]           # momento seguro → enciende
    g.patch(BANDERA, bytes([0]))
    for z in (8, 7, 13, 7, 8, 7): modos(L, z); tick(L, reg, 2)
    e = len(esc(g)) - n0                                   # sin aviso nuevo: nada más
    ok = a == 0 and b == 0 and c == 0 and d == ['AEF78E', '37F5C39'] and e == 2
    return L, g, reg, ok, ['al abrir: %d · aviso en Partido: %d · base cargando: %d · momento seguro: %s · después sin aviso: %d más' % (a, b, c, d, e - 2), pend]

def s4():   # modo.txt raro → ACTIVAR; tecla T cambia solo esta sesión; si Link cambia modo.txt, manda modo.txt
    L, g, reg = montar({'modo.txt': 'TURBO'})
    n0 = boot(L, g, reg); tick(L, reg)
    o = [linea_modo(reg)]
    reg['key_down'](None, 0x54); o.append(linea_modo(reg))
    reg['key_down'](None, 0x54); o.append(linea_modo(reg))
    reg['key_down'](None, 0x54); o.append(linea_modo(reg))
    reg['key_down'](None, 0x54); o.append(linea_modo(reg))     # otra vez AUTO-FICHAJES por tecla
    g.FILES['modo.txt'] = '﻿ACTIVAR'; tick(L, reg); o.append(linea_modo(reg))
    ok = ('ACTIVAR (el de por defecto)' in o[0] and 'algo raro' in o[0] and 'AUTO-FICHAJES' in o[1] and 'tecla T' in o[1]
          and 'AUTO-SIEMPRE' in o[2] and 'ACTIVAR' in o[3] and 'AUTO-FICHAJES' in o[4] and 'ACTIVAR' in o[5] and 'Phoenix Link' in o[5]
          and len(esc(g)) == n0)
    return L, g, reg, ok, o

def s5():   # el exe no es el esperado en el interruptor → no escribe; a los 3 fallos se apaga lo automático
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE'}, exe_ok=False)
    n0 = boot(L, g, reg); tick(L, reg)
    modos(L, 8); tick(L, reg); modos(L, 7); tick(L, reg, 6)
    o = linea_modo(reg)
    ok = len(esc(g)) - n0 == 0 and bandera(g) == 0 and param(g) == 0 and 'ACTIVAR' in o and 'apagó' in o
    return L, g, reg, ok, ['escrituras después del arranque: %s · interruptor: %d' % (esc(g)[n0:], bandera(g)), o]

def s6():   # nunca escribe fuera del momento seguro: Partido, Editar, Liga Máster, cambio de modo
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE', 'recargar.txt': 'a'})
    n0 = boot(L, g, reg); tick(L, reg)
    modos(L, 8); g.FILES['recargar.txt'] = 'b'; tick(L, reg, 5)
    modos(L, 13); edicion(L, 0x1F2C0000000); tick(L, reg, 5)
    modos(L, 7); tick(L, reg, 3)                                  # menú con Editar aún vivo
    edicion(L, 0); modos(L, 19); tick(L, reg, 5)
    modos(L, 7, 1, 19); tick(L, reg, 3)                           # menú cambiando de modo
    a = len(esc(g)) - n0
    modos(L, 7); tick(L, reg)
    ok = a == 0 and esc(g)[n0:] == ['AEF78E', '37F5C39']
    return L, g, reg, ok, ['escrituras fuera del momento seguro: %d · en el momento seguro: %s' % (a, esc(g)[n0:])]

def s7():   # el juego ya tenía el interruptor en 1 (p. ej. Ser una Leyenda): no se escribe nada
    L, g, reg = montar({'modo.txt': 'AUTO-FICHAJES', 'recargar.txt': 'a'})
    n0 = boot(L, g, reg); tick(L, reg)
    g.patch(BANDERA, bytes([1])); g.FILES['recargar.txt'] = 'b'; tick(L, reg, 3)
    o = linea_modo(reg)
    ok = len(esc(g)) == n0 and 'ya encendido' in o and 'pendientes' not in o
    return L, g, reg, ok, [o]

def s8():   # antes del arranque (sin lectura de la base) lo automático no toca la memoria
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE', 'recargar.txt': 'a'})
    r0 = g.CNT.rpm
    g.FILES['recargar.txt'] = 'b'; modos(L, 8); tick(L, reg, 3); modos(L, 7); tick(L, reg, 3)
    ok = g.CNT.rpm == r0 and g.CNT.wpm == 0
    return L, g, reg, ok, ['lecturas de memoria: %d · escrituras: %d' % (g.CNT.rpm - r0, g.CNT.wpm)]

def s9():   # ritmo: 300 cuadros en el mismo segundo = un solo paso; Shift+R (módulo cargado 2 veces) no rompe
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE'})
    boot(L, g, reg); L.execute('RELOJC = RELOJC + 3')
    r0 = g.CNT.rpm
    for _ in range(300): reg['display_frame'](None)
    una = g.CNT.rpm - r0
    m2, reg2 = g.cargar(SRC); reg2['livecpk_read'](None, P, 0, 16, 1769252, 0); L.execute('RELOJC = RELOJC + 3'); reg2['display_frame'](None)
    o = linea_modo(reg2)
    ok = 0 < una < 40 and 'AUTO-SIEMPRE' in o
    return L, g, reg, ok, ['lecturas de memoria en 300 cuadros del mismo segundo: %d (un paso)' % una, o]

caso('S1 sin modo.txt → ACTIVAR: no lee ni escribe nada nuevo', s1)
caso('S2 AUTO-SIEMPRE: recarga al volver al menú desde un modo', s2)
caso('S3 AUTO-FICHAJES: solo con aviso nuevo y en el momento seguro', s3)
caso('S4 modo.txt raro, tecla T y Link manda', s4)
caso('S5 exe distinto en el interruptor → 3 fallos → apagado', s5)
caso('S6 nunca escribe en Partido, Editar, Liga Máster ni cambiando de modo', s6)
caso('S7 interruptor ya encendido por el juego', s7)
caso('S8 antes del arranque no toca la memoria', s8)
caso('S9 ritmo (1 paso cada 2 s) y Shift+R', s9)
print('TODO OK' if all(res) else 'HAY FALLOS')
