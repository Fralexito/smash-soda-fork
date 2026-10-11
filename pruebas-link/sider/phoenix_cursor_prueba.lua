-- =============================================================================
--  Phoenix Evolution · PRUEBA «CURSOR POR MANDO» (módulo de Sider para PES 2021)
--  SOLO PARA PROBAR EN LA PC DE FRALEX, EN UN AMISTOSO OFFLINE CONTRA LA CPU.
--  NO es un módulo de Phoenix Link (por eso vive en pruebas-link\sider\ y no en SmashSoda\phoenix\sider\).
--
--  Qué busca: dónde guarda el juego qué mando (jugador 1-8 de PES) controla a qué futbolista,
--  y dónde apunta quién marcó cada gol. Método en docs/INVESTIGACION-CURSOR-MANDO.md.
--
--  Cómo: se toma un futbolista conocido (por defecto Lamine Yamal, ID 162114, del Barça) y se busca
--  su ID en la memoria (1) en el menú y (2) ya en el partido. Lo que solo aparece en el partido es el
--  futbolista «en el campo». Alrededor de esas direcciones se guardan fotos (4 KB) en momentos
--  marcados (Lamine con el teclado, sin nadie, con el mando físico…) y se anotan los bytes que
--  cambian poco. Después, en el PC de desarrollo, un script compara las fotos (analizar_cursor.py).
--
--  Seguridad (reglas de PhoenixSync/sider/RIESGOS-SIDER.md y 05-PUERTA-EN-VIVO.md):
--    · SOLO LECTURA. No escribe memoria, no usa VirtualProtect, no llama funciones del juego.
--    · NUNCA lee la memoria del juego directamente: COPIA con ReadProcessMemory, que si la zona
--      ya no existe devuelve «falló» en vez de cerrar el juego (crash 0xC0000005 del 9 oct).
--    · Los errores se cuentan con una «bandera» (el Lua de Sider 7.3.3 no trae pcall);
--      5 seguidos → el módulo se apaga solo hasta reiniciar el juego.
--    · Teclas propias: J (hacer el paso) e I (volver un paso), solo con el overlay en esta prueba.
--      Ningún módulo de ConmeGOL usa J ni I (revisado el 11 oct).
--    · Escribe solo en SiderAddons\content\phoenix\cursor\ (archivos de texto y fotos .bin).
--
--  Instalar (con PES CERRADO): copiar a SiderAddons\modules\ y añadir al final de la lista de
--  módulos de sider.ini:   lua.module = "phoenix_cursor_prueba.lua"
--  Desinstalar = quitar esa línea (o restaurar sider.ini.antes-cursor) y borrar el archivo.
-- =============================================================================

local m = { version = "0.1" }

local ID_DEFECTO   = 162114           -- Lamine Yamal (el mismo que usó SYNC en sus pruebas)
local VK_J, VK_I   = 0x4A, 0x49
local TROZO        = 4 * 1024 * 1024  -- bytes copiados por cuadro durante una búsqueda
local MAX_HITS     = 20000
local MAX_CAND     = 96               -- direcciones vigiladas como mucho
local VENT, MEDIO  = 0x1000, 0x800    -- foto de 4 KB alrededor de cada dirección (−2 KB … +2 KB)
local VIGILAR_SEG  = 0.25
local MIRAR_SEG    = 0.5
local MAX_CAMBIOS_BYTE = 12           -- un byte que cambia más veces deja de anotarse (posiciones, relojes)
local MAX_LINEAS   = 40000
local MAX_FALLOS   = 5

local PASOS = {
    { t = "escaneo", n = "menu",    txt = "En el MENÚ PRINCIPAL (antes de elegir equipos)." },
    { t = "escaneo", n = "partido", txt = "Ya en el PARTIDO (teclado y mando físico en el Barça), en PAUSA." },
    { t = "marca",   n = "A1", txt = "Lamine con el balón, controlado por el TECLADO. Pausa." },
    { t = "marca",   n = "N1", txt = "Lamine SIN cursor (los dos mandos en otros jugadores, lejos de él). Pausa." },
    { t = "marca",   n = "B1", txt = "Lamine con el balón, controlado por el MANDO FÍSICO. Pausa." },
    { t = "marca",   n = "A2", txt = "Otra vez: Lamine con el balón y el TECLADO. Pausa." },
    { t = "marca",   n = "B2", txt = "Otra vez: Lamine con el balón y el MANDO FÍSICO. Pausa." },
    { t = "marca",   n = "N2", txt = "Otra vez: Lamine SIN cursor. Pausa." },
    { t = "escaneo", n = "gol",     txt = "Mete un gol CON LAMINE (cualquier mando). Cuando vuelva el saque, pausa." },
    { t = "fin",     n = "fin",     txt = "Listo. Sal del partido y cierra PES. Pásale a Claude la carpeta content\\phoenix\\cursor." },
}

local ffi = ffi or require("ffi")
local bit = bit or require("bit")

local carpeta, objetivo, pat = nil, ID_DEFECTO, nil
local apagado, fallos, enCurso = false, 0, false
local paso = 1
local aviso = ""

local RPM, PROC, MBI = nil, nil, nil
local buf, bufIni, bufFin = nil, 0, 0
local cur, prev, cnt = nil, nil, nil      -- fotos vigiladas (MAX_CAND × VENT) y cuenta de cambios por byte
local leidos1 = nil

local escaneos = {}                       -- nombre → { lista = {...}, set = {...} }
local esc = nil                           -- búsqueda en curso
local cand = {}                           -- direcciones vigiladas
local validas = {}                        -- foto de cada candidata leída bien la última vez
local vigilando, ultimoVigilar, t0 = false, -1000, os.clock()
local lineas, nLineas, ultimoVolcado = {}, 0, -1000
local ultimoMirar, golL, golV, golesVistos, marcaPendiente = -1000, -1, -1, 0, nil
local nMarcas = 0

-- ─── utilidades ──────────────────────────────────────────────────────────────

local function dirNum(p) return tonumber(ffi.cast("uint64_t", p)) end
local function hex(s) return (s:gsub(".", function(ch) return string.format("%02x", ch:byte()) end)) end
local function u32(v) return string.char(v % 256, math.floor(v / 256) % 256, math.floor(v / 65536) % 256, math.floor(v / 16777216) % 256) end

local function escribir(nombre, texto, modo)
    if not carpeta then return false end
    local f = io.open(carpeta .. nombre, modo or "wb")
    if not f then return false end
    f:write(texto)
    f:close()
    return true
end

local function anotar(linea)
    if nLineas >= MAX_LINEAS then return end
    nLineas = nLineas + 1
    lineas[#lineas + 1] = string.format("%.2f %s", os.clock() - t0, linea)
end

local function volcarLineas()
    if #lineas == 0 then return end
    if escribir("cambios.txt", table.concat(lineas, "\n") .. "\n", "ab") then lineas = {} end
    ultimoVolcado = os.clock()
end

local function prepararLector()
    if RPM then return end
    -- nombres con versión: ffi.cdef no deja redeclarar (Shift+R) ni chocar con phoenix.lua (phx06_*)
    ffi.cdef[[
        int   phxc01_RPM(void* proceso, const void* desde, void* hacia, size_t n, size_t* leidos) __asm__("ReadProcessMemory");
        void* phxc01_GCP(void) __asm__("GetCurrentProcess");
    ]]
    RPM, PROC = ffi.C.phxc01_RPM, ffi.C.phxc01_GCP()
    buf = ffi.new("uint8_t[?]", TROZO)
    bufIni = dirNum(buf); bufFin = bufIni + TROZO
    leidos1 = ffi.new("size_t[1]")
end

-- Copia n bytes de la dirección a hacia destino. Devuelve cuántos copió (0 = falló, sin cerrar el juego).
local function copiar(a, n, destino)
    leidos1[0] = 0
    if RPM(PROC, ffi.cast("const void*", a), destino or buf, n, leidos1) == 0 then return 0 end
    return tonumber(leidos1[0])
end

local function leerSeguro(a, n)
    if copiar(a, n) ~= n then return nil end
    return ffi.string(buf, n)
end

-- ¿La zona [base, base+tam) se cruza con alguna memoria NUESTRA (copias y fotos)?
local function esPropia(base, tam)
    local zonas = { { bufIni, bufFin } }
    if cur then
        local t = MAX_CAND * VENT
        zonas[#zonas + 1] = { dirNum(cur), dirNum(cur) + t }
        zonas[#zonas + 1] = { dirNum(prev), dirNum(prev) + t }
    end
    for _, z in ipairs(zonas) do
        if base < z[2] and base + tam > z[1] then return true end
    end
    return false
end

-- ─── búsqueda del ID en la memoria (por trozos, unos MB por cuadro) ──────────

local function empezarEscaneo(nombre)
    prepararLector()
    local si = memory.get_system_info()
    MBI = MBI or ffi.new("MEMORY_BASIC_INFORMATION[1]")
    esc = {
        nombre = nombre, dir = dirNum(si.lpMinimumApplicationAddress), tope = dirNum(si.lpMaximumApplicationAddress),
        regIni = 0, regFin = 0, pos = 0, lista = {}, set = {}, mb = 0, regiones = 0, fallidas = 0,
        propia = dirNum(ffi.cast("const char*", pat)), t0 = os.clock(),
    }
    aviso = "buscando el ID " .. objetivo .. " («" .. nombre .. "»)…"
    log("[cursor] búsqueda «" .. nombre .. "» del ID " .. objetivo .. " (solo lectura, copia segura)")
end

-- Siguiente región privada, COMMIT y legible (como la sonda de SYNC): el heap del juego.
local function siguienteRegion()
    local tam = ffi.sizeof("MEMORY_BASIC_INFORMATION")
    while esc.dir < esc.tope do
        if ffi.C.VirtualQuery(ffi.cast("void*", esc.dir), MBI, tam) == 0 then return false end
        local base = dirNum(MBI[0].BaseAddress)
        local size = tonumber(MBI[0].RegionSize)
        local estado, prot, tipo = tonumber(MBI[0].State), tonumber(MBI[0].Protect), tonumber(MBI[0].Type)
        esc.dir = base + size
        local legible = estado == 0x1000 and tipo == 0x20000 and bit.band(prot, 0x100) == 0
                        and bit.band(prot, 0x01) == 0 and bit.band(prot, 0xEE) ~= 0
        if legible and not esPropia(base, size) then
            esc.regIni, esc.regFin, esc.pos = base, base + size, base
            esc.regiones = esc.regiones + 1
            return true
        end
    end
    return false
end

local function terminarEscaneo()
    local e = esc
    esc = nil
    e.seg = os.clock() - e.t0
    escaneos[e.nombre] = e
    -- Archivo con todas las direcciones y 96 bytes alrededor (−32 … +64)
    local L = { string.format("# búsqueda «%s» del ID %d · %d regiones · %d MB · %.1f s · %d direcciones · %d lecturas fallidas",
        e.nombre, objetivo, e.regiones, e.mb, e.seg, #e.lista, e.fallidas) }
    for i, a in ipairs(e.lista) do
        if i > 4000 then L[#L + 1] = "# …cortado en 4000" break end
        local ctx = leerSeguro(a - 32, 96)
        L[#L + 1] = string.format("%X %s", a, ctx and hex(ctx) or "ilegible")
    end
    escribir("busqueda_" .. e.nombre .. ".txt", table.concat(L, "\n") .. "\n")
    local msg = string.format("búsqueda «%s» lista: %d direcciones (%d MB en %.0f s, %d lecturas fallidas)", e.nombre, #e.lista, e.mb, e.seg, e.fallidas)

    if e.nombre == "partido" then
        local menu = escaneos.menu
        cand = {}
        local nuevas = 0
        for _, a in ipairs(e.lista) do
            if not (menu and menu.set[a]) then
                nuevas = nuevas + 1
                if #cand < MAX_CAND then cand[#cand + 1] = a end
            end
        end
        if #cand > 0 then
            local t = MAX_CAND * VENT
            cur = cur or ffi.new("uint8_t[?]", t)
            prev = prev or ffi.new("uint8_t[?]", t)
            cnt = cnt or ffi.new("uint8_t[?]", t)
            ffi.fill(cnt, t, 0)
            for i, a in ipairs(cand) do validas[i] = copiar(a - MEDIO, VENT, prev + (i - 1) * VENT) == VENT end
            local C = { "# candidatas (solo en el partido, no en el menú): índice dirección" }
            for i, a in ipairs(cand) do C[#C + 1] = string.format("%d %X", i, a) end
            escribir("candidatas.txt", table.concat(C, "\n") .. "\n")
            vigilando = true
            anotar(string.format("# VIGILANDO %d candidatas (de %d nuevas)", #cand, nuevas))
        end
        msg = msg .. string.format(" · %d nuevas respecto al menú · vigilando %d", nuevas, #cand)
    elseif e.nombre == "gol" then
        local antes = escaneos.partido
        local N = { "# direcciones que aparecen tras el gol (no estaban en «partido»): dirección y 256 bytes (−64 … +192)" }
        local n = 0
        for _, a in ipairs(e.lista) do
            if not (antes and antes.set[a]) then
                n = n + 1
                local ctx = leerSeguro(a - 64, 256)
                N[#N + 1] = string.format("%X %s", a, ctx and hex(ctx) or "ilegible")
            end
        end
        escribir("tras_gol.txt", table.concat(N, "\n") .. "\n")
        msg = msg .. string.format(" · %d nuevas tras el gol", n)
    end
    aviso = msg
    log("[cursor] " .. msg)
end

local function pasoEscaneo()
    local presupuesto = TROZO
    while esc and presupuesto > 0 do
        if esc.pos >= esc.regFin then
            if not siguienteRegion() then terminarEscaneo() return end
        end
        local n = math.min(presupuesto, esc.regFin - esc.pos)
        local leidos = copiar(esc.pos, n)
        if leidos == 0 then
            esc.fallidas = esc.fallidas + 1
            esc.pos = esc.regFin                        -- la zona desapareció: se salta, sin riesgo
        else
            local d = 0
            while d < leidos and #esc.lista < MAX_HITS do
                local h = memory.search(pat, bufIni + d, bufIni + leidos)
                if not h then break end
                local off = dirNum(h) - bufIni
                local a = esc.pos + off
                if a % 4 == 0 and a ~= esc.propia and not esc.set[a] then
                    esc.set[a] = true
                    esc.lista[#esc.lista + 1] = a
                end
                d = off + 1
            end
            esc.pos = esc.pos + n
            esc.mb = esc.mb + n / 1048576
        end
        presupuesto = presupuesto - n
    end
end

-- ─── fotos y vigilancia ──────────────────────────────────────────────────────

local function foto(nombre)
    if #cand == 0 then aviso = "todavía no hay candidatas (falta la búsqueda «partido»)" return end
    nMarcas = nMarcas + 1
    local partes = {}
    local a64 = ffi.new("uint64_t[1]")
    local okN = 0
    for i, a in ipairs(cand) do
        local destino = cur + (i - 1) * VENT
        local ok = copiar(a - MEDIO, VENT, destino) == VENT
        if not ok then ffi.fill(destino, VENT, 0) else okN = okN + 1 end
        a64[0] = ok and a or 0
        partes[#partes + 1] = ffi.string(a64, 8)
        partes[#partes + 1] = ffi.string(destino, VENT)
    end
    local archivo = string.format("marca_%02d_%s.bin", nMarcas, nombre)
    local st = match.stats()
    local marcador = st and string.format("%s-%s min %s", tostring(st.home_score), tostring(st.away_score), tostring(st.clock_minutes)) or "sin partido"
    escribir(archivo, table.concat(partes))
    escribir("marcas.txt", string.format("%s %s %s %s · %d/%d fotos bien\n", archivo, nombre, os.date("%H:%M:%S"), marcador, okN, #cand), "ab")
    anotar("# MARCA " .. nombre .. " → " .. archivo)
    aviso = "foto «" .. nombre .. "» guardada (" .. okN .. "/" .. #cand .. ")"
    log("[cursor] " .. aviso .. " · " .. marcador)
end

-- Compara las fotos de ahora con las anteriores (de 8 en 8 bytes) y anota los bytes que cambian poco.
local function vigilar()
    for i, a in ipairs(cand) do
        local o = (i - 1) * VENT
        local ok = copiar(a - MEDIO, VENT, cur + o) == VENT
        if ok and validas[i] then
            local c8 = ffi.cast("uint64_t*", cur + o)
            local p8 = ffi.cast("uint64_t*", prev + o)
            for w = 0, VENT / 8 - 1 do
                if c8[w] ~= p8[w] then
                    for b = w * 8, w * 8 + 7 do
                        local k = o + b
                        if cur[k] ~= prev[k] then
                            if cnt[k] < 255 then cnt[k] = cnt[k] + 1 end
                            if cnt[k] <= MAX_CAMBIOS_BYTE then
                                anotar(string.format("c%d %+d %02x>%02x", i, b - MEDIO, prev[k], cur[k]))
                            end
                        end
                    end
                end
            end
        end
        if ok then ffi.copy(prev + o, cur + o, VENT) end
        validas[i] = ok
    end
end

local function mirarGoles()
    local st = match.stats()
    if not st then golL, golV = -1, -1 return end
    local hl, al = tonumber(st.home_score) or 0, tonumber(st.away_score) or 0
    if golL >= 0 and (hl ~= golL or al ~= golV) then
        golesVistos = golesVistos + 1
        local lado = (hl > golL) and "l" or ((al > golV) and "v" or "x")
        anotar(string.format("# GOL %s marcador %d-%d min %s", lado, hl, al, tostring(st.clock_minutes)))
        log(string.format("[cursor] gol (%s) %d-%d", lado, hl, al))
        if #cand > 0 then
            foto("gol" .. golesVistos .. lado)
            marcaPendiente = { t = os.clock() + 2.0, n = "gol" .. golesVistos .. lado .. "_2s" }
        end
    end
    golL, golV = hl, al
end

-- ─── eventos ─────────────────────────────────────────────────────────────────

local function bandera(donde)
    if apagado then return false end
    if enCurso then
        fallos = fallos + 1
        log("[cursor] error en la vez anterior (" .. donde .. ", " .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[cursor] demasiados errores seguidos: la prueba se detiene hasta reiniciar el juego")
            return false
        end
    end
    enCurso = true
    return true
end
local function bajar() enCurso = false; fallos = 0 end

function m.display_frame(ctx)
    if not bandera("cuadro") then return end
    if esc then pasoEscaneo() end
    local t = os.clock()
    if t - ultimoMirar >= MIRAR_SEG then ultimoMirar = t; mirarGoles() end
    if marcaPendiente and t >= marcaPendiente.t then local n = marcaPendiente.n; marcaPendiente = nil; foto(n) end
    if vigilando and not esc and t - ultimoVigilar >= VIGILAR_SEG then ultimoVigilar = t; vigilar() end
    if t - ultimoVolcado >= 2 then volcarLineas() end
    bajar()
end

function m.key_down(ctx, vkey)
    if not bandera("tecla") then return end
    if vkey == VK_I and not esc then
        if paso > 1 then paso = paso - 1 end
        aviso = "vuelto al paso " .. paso
    elseif vkey == VK_J and not esc then
        local p = PASOS[paso]
        if p.t == "escaneo" then
            empezarEscaneo(p.n); paso = paso + 1
        elseif p.t == "marca" then
            foto(p.n); if #cand > 0 then paso = paso + 1 end
        else
            volcarLineas(); aviso = "terminado: ya puedes cerrar PES"
        end
    end
    bajar()
end

function m.overlay_on(ctx)
    if apagado then return "PRUEBA CURSOR · detenida por seguridad (ver sider.log)" end
    if not bandera("overlay") then return "PRUEBA CURSOR · detenida por seguridad (ver sider.log)" end
    local p = PASOS[paso]
    local L = {}
    L[#L + 1] = string.format("PRUEBA CURSOR POR MANDO v%s  ·  futbolista objetivo: ID %d  ·  solo lectura", m.version, objetivo)
    L[#L + 1] = ""
    L[#L + 1] = string.format("  Paso %d de %d:  %s", paso, #PASOS, p.txt)
    if esc then
        L[#L + 1] = string.format("  …buscando «%s»: %d MB, %d encontradas. Espera (no cierres el juego).", esc.nombre, esc.mb, #esc.lista)
    elseif p.t ~= "fin" then
        L[#L + 1] = "  Cuando esté así, pulsa J.   (I = volver un paso)"
    end
    L[#L + 1] = ""
    L[#L + 1] = "  " .. (aviso ~= "" and aviso or "—")
    L[#L + 1] = string.format("  Vigilando %d direcciones · fotos %d · goles vistos %d · líneas %d", #cand, nMarcas, golesVistos, nLineas)
    L[#L + 1] = "  Carpeta: " .. tostring(carpeta)
    bajar()
    return table.concat(L, "\n")
end

function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    carpeta = base .. "content\\phoenix\\cursor\\"
    if fs and fs.make_dirs then fs.make_dirs(carpeta) end
    if not escribir("leeme.txt", "Archivos de la prueba «cursor por mando» (phoenix_cursor_prueba.lua). Se pueden borrar.\r\n") then
        carpeta = base .. "content\\phoenix\\cursor_"      -- sin subcarpeta: mismos archivos con prefijo
        escribir("leeme.txt", "Archivos de la prueba «cursor por mando». Se pueden borrar.\r\n")
    end
    local f = io.open((carpeta or "") .. "objetivo.txt", "rb")
    if f then
        local v = tonumber((f:read(32) or ""):match("%d+"))
        f:close()
        if v and v > 0 and v < 4294967296 then objetivo = v end
    end
    pat = u32(objetivo)
    escribir("cambios.txt", string.format("# prueba cursor v%s · objetivo %d · %s\n", m.version, objetivo, os.date("%Y-%m-%d %H:%M:%S")))
    ctx.register("display_frame", m.display_frame)
    ctx.register("key_down", m.key_down)
    ctx.register("overlay_on", m.overlay_on)
    log("[cursor] v" .. m.version .. " lista (solo lectura). Objetivo ID " .. objetivo .. ". Archivos en " .. carpeta)
end

return m
