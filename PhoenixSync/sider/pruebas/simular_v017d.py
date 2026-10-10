# Simulación de la Fase D (v0.17d): «zona» y «momento seguro», con memoria y relojes falsos.
# Reutiliza el entorno de simular_v017m.py (debe estar en la misma carpeta). Uso: python3 simular_v017d.py phoenix-v0.17d-prueba.lua
import sys
src_sim = open('simular_v017m.py', encoding='utf-8').read()
cab = src_sim[:src_sim.index("for n, f in (('1 normal")]
cab = cab.replace("SRC = open(sys.argv[1], encoding='utf-8').read()", "SRC = open(sys.argv[1], encoding='utf-8').read()\nRELOJ_LUA = 'RELOJ = 1000; os.time = function() return RELOJ end; RELOJC = 100; os.clock = function() return RELOJC end'")
cab = cab.replace("    L.execute(HARNESS)\n", "    L.execute(HARNESS)\n    L.execute(RELOJ_LUA)\n")
exec(cab)
BASE = 0x140000000
P = 'common\\etc\\pesdb\\Player.bin'
GM = 0x000001F2B0000000          # gestor de modos falso
CHKZ = [(0x149A470, bytes([72,139,5,193,169,38,2,195])), (0x149F4F0, bytes([139,129,240,0,0,0,51,201,131,248,61,15,67,193])),
        (0x149F510, bytes([128,185,44,1,0,0,0,116,8,139,129,48,1,0,0,235,6])), (0x1EF2250, bytes([72,139,5,209,57,144,1,195]))]
def codigo_zona(L, ok=True):
    for rva, b in CHKZ: L.globals().setmem(BASE + rva, b if ok else bytes([0x90]) * len(b))
def modos(L, modo, cambiando=0, sig=0, ptr=GM):
    g = L.globals(); g.setmem(BASE + 0x3704E38, ptr.to_bytes(8, 'little'))
    blk = bytearray(0x44); blk[0:4] = modo.to_bytes(4, 'little'); blk[0x3C] = cambiando; blk[0x40:0x44] = sig.to_bytes(4, 'little')
    if ptr: g.setmem(ptr + 0xF0, bytes(blk))
def edicion(L, ptr): L.globals().setmem(BASE + 0x37F5C28, ptr.to_bytes(8, 'little'))
def base_bd(L, est, modo=0): L.globals().setmem(G, L.globals().gestor(est, modo, 0))
def linea(txt, clave='[ZONA]'):
    for l in str(txt).split('\n'):
        if clave in l: return l.strip()
def zona(reg): return linea(reg['overlay_on'](None))
def arrancar(L, g, reg, t=100):
    """Primera lectura de la base: a partir de aquí el módulo empieza a muestrear la zona."""
    L.execute('RELOJC = %d' % t); reg['livecpk_read'](None, P, 0, 16, 1769252, 0)
res = []
def caso(nombre, f, espera):
    L = nuevo(); g = L.globals(); m, reg = g.cargar(SRC)
    out = f(L, g, m, reg)
    ok = len(espera) == len(out) and all(all(x in o for x in (e if isinstance(e, tuple) else (e,))) for e, o in zip(espera, out))
    print(('OK  ' if ok else 'MAL ') + nombre)
    for o in out: print('      ' + str(o)[:170])
    print('      escrituras: WPM=%d (las únicas posibles son las del botón nativo de la v0.17; aquí su lectura falsa falla y no escribe)' % g.CNT.wpm)
    for l in logs(L):
        if 'zona' in l or 'evento' in l: print('      LOG ' + l[10:175])
    res.append(ok)
def todo(L, modo=7, est=4, ed=0, cambiando=0, sig=0):
    codigo(L); ptr(L, G); codigo_zona(L); modos(L, modo, cambiando, sig); edicion(L, ed); base_bd(L, est)
def d1(L, g, m, reg):       # menú principal quieto → seguro; luego Partido, Editar, Liga Máster
    todo(L); arrancar(L, g, reg); o = [zona(reg)]
    modos(L, 8); o.append(zona(reg))
    modos(L, 13); edicion(L, 0x1F2C0000000); o.append(zona(reg))
    modos(L, 19); edicion(L, 0); o.append(zona(reg))
    modos(L, 7); o.append(zona(reg))
    return o
def d2(L, g, m, reg):       # menú principal pero NO seguro: base cargando / edición activa / cambiando de modo
    todo(L, est=1); arrancar(L, g, reg); o = [zona(reg)]
    base_bd(L, 4); edicion(L, 0x1F2C0000000); o.append(zona(reg))
    edicion(L, 0); modos(L, 7, 1, 8); o.append(zona(reg))
    modos(L, 7); o.append(zona(reg))
    return o
def d3(L, g, m, reg):       # exe distinto: no lee la zona
    codigo(L); ptr(L, G); base_bd(L, 4); codigo_zona(L, ok=False); modos(L, 7); edicion(L, 0); arrancar(L, g, reg)
    a = zona(reg); r0 = g.CNT.rpm; zona(reg); una = g.CNT.rpm - r0; r1 = g.CNT.rpm; zona(reg); otra = g.CNT.rpm - r1
    # tras el primer rechazo, mirar el overlay no vuelve a comprobar el código de la zona: el número de lecturas
    # por mirada es fijo (las que ya hacía el overlay antes) y ninguna va a las direcciones de la zona
    L.execute('ZON = 0; local viejo = ffi.C.phx06_RPM; ffi.C.phx06_RPM = function(p, d, h, n, l) local a = tonumber(require("ffi").cast("uint64_t", require("ffi").cast("const void*", d))); if a == 0x140000000 + 0x3704E38 or a == 0x140000000 + 0x149A470 then ZON = ZON + 1 end; return viejo(p, d, h, n, l) end')
    zona(reg)
    return [a, 'lecturas por mirada: %d y %d · lecturas de la zona tras el rechazo: %d' % (una, otra, g.ZON)]
def d4(L, g, m, reg):       # gestor de modos inexistente / puntero raro / zona ilegible
    todo(L); modos(L, 7, ptr=0); arrancar(L, g, reg); o = [zona(reg)]
    g.setmem(BASE + 0x3704E38, (0xFFFF800000001000).to_bytes(8, 'little')); o.append(zona(reg))
    g.setmem(BASE + 0x3704E38, (0x1F2D0000000).to_bytes(8, 'little')); o.append(zona(reg))
    return o
def d5(L, g, m, reg):       # eventos de Sider: no devuelven nada
    todo(L, modo=8); arrancar(L, g, reg)
    r1 = reg['set_teams'](None, 108, 172); r2 = reg['context_reset'](None)
    return ['set_teams devolvió %s · context_reset devolvió %s' % (r1, r2), zona(reg)]
def d6(L, g, m, reg):       # como mucho una muestra por segundo al leer archivos; nada antes del arranque
    todo(L); f = reg['livecpk_read']
    r0 = g.CNT.rpm
    for i in range(50): f(None, 'common\\otra\\cosa%d.bin' % i, 0, 16, 10, 0)      # antes de la primera lectura de la base
    antes = g.CNT.rpm - r0
    arrancar(L, g, reg, 100); r1 = g.CNT.rpm
    for i in range(200): f(None, 'common\\otra\\cosa%d.bin' % i, 0, 16, 10, 0)     # mismo segundo
    mismo = g.CNT.rpm - r1
    L.execute('RELOJC = 102'); f(None, 'common\\otra\\cosa.bin', 0, 16, 10, 0); desp = g.CNT.rpm - r1 - mismo
    return ['lecturas antes del arranque: %d' % antes, 'lecturas en 200 archivos del mismo segundo: %d (una sola muestra: 4 comprobaciones + 5)' % mismo, 'lecturas 2 s después (1 muestra): %d' % desp]
def d7(L, g, m, reg):       # el gestor de la base no se puede leer → no es seguro
    codigo(L); codigo_zona(L); modos(L, 7); edicion(L, 0); ptr(L, 0); arrancar(L, g, reg)
    return [zona(reg)]
caso('D1 menú → Partido → Editar → Liga Máster → menú', d1, [('TOP_MENU (7)', 'seguro: SÍ'), ('EXHIBITION (8)', 'NO (no es el menú'), ('EDIT (13)', 'edición ACTIVA', 'NO'), ('UEFA_ML (19)', 'NO'), ('TOP_MENU (7)', 'SÍ')])
caso('D2 menú principal pero no seguro', d2, [('TOP_MENU', 'NO (la base no está quieta'), ('TOP_MENU', 'NO (el gestor de edición'), ('→ EXHIBITION (8)', 'NO (cambiando'), ('TOP_MENU', 'SÍ')])
caso('D3 exe distinto', d3, ['exe distinto', 'lecturas de la zona tras el rechazo: 0'])
caso('D4 gestor de modos inexistente / raro / ilegible', d4, ['todavía no existe', 'ilegible o raro', 'gestor de modos ilegible'])
caso('D5 eventos de Sider', d5, ['set_teams devolvió None · context_reset devolvió None', 'EXHIBITION (8)'])
caso('D6 ritmo de muestreo', d6, ['arranque: 0', 'mismo segundo: 9 ', '(1 muestra): 5'])
caso('D7 gestor de la base ilegible', d7, [('TOP_MENU', 'NO (la base no está quieta (estado nil)')])
print('TODO OK' if all(res) else 'HAY FALLOS')
