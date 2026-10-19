# Simulación de la v0.18c: página propia en «Sider Mod Menu» (F1) y su tolerancia a versiones futuras del menú.
# Reutiliza el entorno de simular_v018b.py (misma carpeta). Uso: python3 simular_v018c.py phoenix-v0.18c-prueba.lua
import sys
src = open('simular_v018b.py', encoding='utf-8').read()
exec(src[:src.index("def s1():")])

MENU_LUA = r'''
function cargar_menu(src, variante)   -- variante: "v12" | "sin_original"
    local f = assert(loadstring(src, "phoenix.lua"))
    local m = f()
    local reg, paginas = {}, {}
    CLASICA = 0
    local ctx = { sider_dir = "X:\\nada\\" }
    ctx.register = function(ev, fn) reg[ev] = fn; if ev == "overlay_on" then CLASICA = CLASICA + 1 end end   -- el envoltorio del menú
    if variante ~= "sin_original" then ctx.modmenu_register = function(ev, fn) reg[ev] = fn end end         -- el registro original de Sider
    ctx.menu = { add = function(def) paginas[#paginas + 1] = def end }
    m.init(ctx)
    return m, reg, paginas
end
function nueva_ui(piezas)   -- piezas: lista de nombres de métodos que «tiene» esta versión del menú
    local ui = { e = {} }
    for _, n in ipairs(piezas) do
        ui[n] = function(self, a, b, c, d, e)
            if ROMPER and n == "section" then error("pieza rota a propósito") end
            self.e[#self.e + 1] = n .. ":" .. tostring(a) .. "|" .. tostring(b) .. (n == "section" and ("|" .. tostring(c)) or "")
            if n == "choice" then self.elegir = e end
            if n == "button" then self.pulsar = c end
        end
    end
    return ui
end
function texto_ui(ui) return table.concat(ui.e, "\n") end
'''
COMPLETA = ["section", "text", "kv", "message", "choice", "button", "row", "endrow", "separator"]

def montar_menu(variante="v12", archivos=None):
    L = nuevo(); L.execute(EXTRA_LUA); L.execute(MENU_LUA); g = L.globals()
    for k, v in (archivos or {}).items(): g.FILES[k] = v
    m, reg, paginas = g.cargar_menu(SRC, variante)
    todo(L)
    for rva, b in PAT.items(): g.setmem(BASE + rva, b + bytes(16))
    g.setmem(BANDERA, bytes([0]))
    return L, g, reg, paginas

def construir(L, g, pag, piezas):
    ui = g.nueva_ui(L.table(*piezas))
    pag[1].build(ui, None)
    return ui, str(g.texto_ui(ui))

res = []
def caso(nombre, f):
    ok, notas = f()
    print(('OK  ' if ok else 'MAL ') + nombre)
    for n_ in notas: print('      ' + str(n_)[:220].replace('\n', ' ¶ '))
    res.append(ok)

def c1():   # menú v1.2: página propia, sin página «clásica» repetida, sin leer memoria antes del arranque
    L, g, reg, pag = montar_menu()
    r0 = g.CNT.rpm
    ui, t = construir(L, g, pag, COMPLETA)
    ok = (len(pag) == 1 and pag[1].id == "phoenix-fichajes" and g.CLASICA == 0 and 'overlay_on' in reg
          and "section:Estado" in t and "Todavía no se sabe" in t and "choice:modo" in t and g.CNT.rpm == r0
          and str(pag[1].status(None)) == "Automático siempre")
    return ok, ['páginas: %d · páginas clásicas repetidas: %d · lecturas de memoria antes del arranque: %d' % (len(pag), g.CLASICA, g.CNT.rpm - r0), t]

def c2():   # tras el arranque: elegir «Activar» en el selector cambia el modo (solo esta sesión), como la tecla T
    L, g, reg, pag = montar_menu()
    boot(L, g, reg); tick(L, reg)
    ui, t = construir(L, g, pag, COMPLETA)
    ui.elegir(3)
    linea = linea_modo(reg)
    ui2, t2 = construir(L, g, pag, COMPLETA)
    chk = ["] ACTIVAR" in linea, str(pag[1].status(None)) == "Activar", "elegido en este menú" in t2, "Datos en vivo" in t2]
    ok = all(chk)
    ui2.elegir(1)
    ok = ok and str(pag[1].status(None)) == "Automático siempre"
    return ok, [linea, t2]

def c3():   # versión futura con SOLO «text»: la página sale como texto, sin errores
    L, g, reg, pag = montar_menu()
    ui, t = construir(L, g, pag, ["text"])
    ok = "Cómo entran los fichajes: Automático siempre" in t and "Estado" in t
    return ok, [t]

def c4():   # versión futura sin ninguna pieza conocida: no rompe
    L, g, reg, pag = montar_menu()
    ui, t = construir(L, g, pag, [])
    return t == "", ['(sin piezas: página vacía, sin error)']

def c5():   # menú sin el registro original guardado: se usa ctx.register (sale la página clásica repetida, nada se rompe)
    L, g, reg, pag = montar_menu("sin_original")
    ok = g.CLASICA == 1 and len(pag) == 1 and 'overlay_on' in reg
    return ok, ['páginas clásicas: %d' % g.CLASICA]

def c6():   # 3 errores seguidos en la página → pasa a «solo texto» con el overlay de siempre
    L, g, reg, pag = montar_menu()
    g.ROMPER = True
    errores = 0
    for _ in range(4):
        try: construir(L, g, pag, COMPLETA)
        except Exception: errores += 1
    g.ROMPER = False
    ui, t = construir(L, g, pag, COMPLETA)
    ok = errores == 3 and "La página dio errores" in t and "PHOENIX EVOLUTION" in t   # el 4.º intento ya va en «solo texto»
    return ok, ['errores provocados: %d' % errores, t[:200]]

def c7():   # sin menú: igual que la v0.18b (registro normal del overlay)
    L, g, reg = montar({'modo.txt': 'AUTO-SIEMPRE'})
    ok = 'overlay_on' in reg and 'AUTO-SIEMPRE' in linea_modo(reg)
    return ok, [linea_modo(reg)]

caso('C1 menú v1.2: página propia y sin página clásica repetida', c1)
caso('C2 selector de modo = tecla T (solo esta sesión)', c2)
caso('C3 versión futura con solo «text»', c3)
caso('C4 versión futura sin piezas conocidas', c4)
caso('C5 menú sin registro original', c5)
caso('C6 tres errores → solo texto', c6)
caso('C7 sin menú: como la v0.18b', c7)
print('TODO OK' if all(res) else 'HAY FALLOS')
