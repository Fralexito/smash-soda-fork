# Simulación de la opción A (v0.17a): «última recarga» con reloj falso. Reutiliza el entorno de simular_v017m.py
import sys, re
src_sim = open('simular_v017m.py', encoding='utf-8').read()
cab = src_sim[:src_sim.index("for n, f in (('1 normal")]            # todo menos la ejecución de los 6 casos
cab = cab.replace("SRC = open(sys.argv[1], encoding='utf-8').read()", "SRC = open(sys.argv[1], encoding='utf-8').read()\nRELOJ_LUA = 'RELOJ = 1000; os.time = function() return RELOJ end'")
cab = cab.replace("    L.execute(HARNESS)\n", "    L.execute(HARNESS)\n    L.execute(RELOJ_LUA)\n")
exec(cab)
P = 'common\\etc\\pesdb\\Player.bin'
def linea(txt, clave='[ÚLTIMA RECARGA]'):
    for l in str(txt).split('\n'):
        if clave in l: return l.strip()
def leer(L, g, reg, modo, t, est=1):
    L.execute('RELOJ = %d' % t); g.setmem(G, g.gestor(est, modo, 0)); reg['livecpk_read'](None, P, 0, 16, 1769252, 0)
def ult(reg): return linea(reg['overlay_on'](None))
res = []
def caso(nombre, f, espera):
    L = nuevo(); g = L.globals(); m, reg = g.cargar(SRC)
    w0, v0 = g.CNT.wpm, g.CNT.vp
    out = f(L, g, m, reg)
    ok = all(e in o for e, o in zip(espera, out)) and len(espera) == len(out)
    # las escrituras solo pueden venir del botón nativo de la v0.17 (aquí falla la lectura falsa y no escribe)
    print(('OK  ' if ok else 'MAL ') + nombre)
    for o in out: print('      ' + o[:150])
    print('      escrituras del módulo: WPM=%d VP=%d' % (g.CNT.wpm - w0, g.CNT.vp - v0))
    for l in logs(L):
        if 'última recarga' in l: print('      LOG ' + l[10:150])
    res.append(ok)
def a1(L, g, m, reg):
    codigo(L); ptr(L, G); o = [ult(reg)]
    leer(L, g, reg, 0, 1000); leer(L, g, reg, 0, 1105); o.append(ult(reg))          # arranque (2 pasadas)
    leer(L, g, reg, 1, 1117); leer(L, g, reg, 0, 1120); o.append(ult(reg))          # Activar (1 y 3 s después 0)
    leer(L, g, reg, 1, 1237); leer(L, g, reg, 0, 1240); o.append(ult(reg))          # otro Activar
    return o
def a2(L, g, m, reg):
    codigo(L); ptr(L, G)
    leer(L, g, reg, 1, 2000); leer(L, g, reg, 0, 2003); o = [ult(reg)]
    leer(L, g, reg, 0, 2060); o.append(ult(reg))                                      # Editar → Cargar un minuto después
    leer(L, g, reg, 1, 2100); leer(L, g, reg, 0, 2116); o.append(ult(reg))          # 2.ª recarga lenta (16 s): cuenta como normal
    return o
def a3(L, g, m, reg):
    codigo(L, ok=False); ptr(L, G); leer(L, g, reg, 1, 3000); r0 = g.CNT.rpm
    leer(L, g, reg, 1, 3010); assert g.CNT.rpm == r0
    return [ult(reg)]
def a4(L, g, m, reg):
    codigo(L); ptr(L, G); leer(L, g, reg, 1, 4000); leer(L, g, reg, 0, 4003); o = [ult(reg)]
    m2, reg2 = g.cargar(SRC); o.append(ult(reg2))                                     # Shift+R: sin memoria
    leer(L, g, reg2, 0, 4100); o.append(ult(reg2))
    return o
def a5(L, g, m, reg):
    codigo(L); ptr(L, G); leer(L, g, reg, 2, 5000); o = [ult(reg)]
    leer(L, g, reg, 1, 5100); leer(L, g, reg, 0, 5050); o.append(ult(reg))          # reloj hacia atrás: no se toma como 2.ª recarga
    ptr(L, 0); L.execute('RELOJ = 5300'); reg['livecpk_read'](None, P, 0, 16, 1769252, 0); o.append(ult(reg))   # gestor inexistente: no cambia
    return o
caso('A1 arranque → Activar → Activar', a1, ['todavía no vista', 'NORMAL', 'ACTIVAR', 'ACTIVAR'])
caso('A2 Activar → Editar/Cargar → 2.ª recarga lenta', a2, ['ACTIVAR', 'NORMAL', 'NORMAL'])
caso('A3 exe distinto: no lee y no apunta', a3, ['todavía no vista'])
caso('A4 Shift+R: empieza sin memoria', a4, ['ACTIVAR', 'todavía no vista', 'NORMAL'])
caso('A5 modo raro, reloj atrás, gestor inexistente', a5, ['DESCONOCIDA', 'NORMAL', 'NORMAL'])
print('TODO OK' if all(res) else 'HAY FALLOS')
