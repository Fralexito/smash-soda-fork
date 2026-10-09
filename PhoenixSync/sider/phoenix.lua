-- =============================================================================
--  Phoenix Evolution · Puente en vivo para Sider (PES 2021)
--  Prueba 1 — lee un archivo de texto y lo muestra en el overlay de Sider.
--  Prueba 2 (v0.3, «solo mirar») — con la tecla B, y SOLO si el overlay muestra
--  este módulo, busca la ficha de Lamine Yamal (ID 162114) en la memoria del
--  juego y la LEE. NUNCA escribe en la memoria, NO registra hooks del partido,
--  NO cambia archivos del juego. Si algo falla, se apaga solo (ver sider.log).
--
--  Archivo que lee:  <SiderAddons>\content\phoenix\avisos.txt  (UTF-8, ≤ 4 KB)
--  Quién lo escribe: Phoenix Link / Phoenix Sync (o PhoenixAviso.bat)
--  Cómo se ve:       Espacio abre el overlay de Sider; con 1 / º se pasa de
--                    módulo hasta llegar a «Phoenix Evolution».
--  Desinstalar:      quitar (o poner ; delante de) la línea
--                    lua.module = "phoenix.lua"  en sider.ini
-- =============================================================================

local m = { version = "0.3-prueba" }

local CADA_SEG    = 2      -- cada cuántos segundos se vuelve a mirar el archivo (solo con el overlay abierto)
local MAX_BYTES   = 4096   -- nunca se lee más que esto
local MAX_LINEAS  = 14
local MAX_ANCHO   = 110    -- bytes por línea (se corta sin partir letras con tilde)
local MAX_FALLOS  = 5      -- tras 5 errores internos seguidos, el módulo se apaga

local ruta = nil
local ultimoChequeo = -1000
local contenido = "Esperando el primer aviso..."
local estado = "sin datos"
local actualizado = ""
local firmaAnterior = nil
local fallos = 0
local apagado = false

-- Corta una cadena UTF-8 a como mucho n bytes sin dejar una letra a medias.
local function cortarUtf8(s, n)
    if #s <= n then return s end
    local fin = n
    -- retroceder mientras el byte siguiente sea de continuación (10xxxxxx)
    while fin > 0 do
        local b = s:byte(fin + 1)
        if not b or b < 0x80 or b >= 0xC0 then break end
        fin = fin - 1
    end
    return s:sub(1, fin) .. "..."
end

-- ¿Es UTF-8 válido? Si no, no se muestra (evita basura en pantalla si el archivo se escribió mal).
local function esUtf8(s)
    local i, n = 1, #s
    while i <= n do
        local b = s:byte(i)
        local extra
        if b < 0x80 then extra = 0
        elseif b >= 0xC2 and b <= 0xDF then extra = 1
        elseif b >= 0xE0 and b <= 0xEF then extra = 2
        elseif b >= 0xF0 and b <= 0xF4 then extra = 3
        else return false end
        for k = 1, extra do
            local c = s:byte(i + k)
            if not c or c < 0x80 or c > 0xBF then return false end
        end
        i = i + extra + 1
    end
    return true
end

-- Deja el texto seguro para mostrar: sin BOM, sin \r, sin caracteres de control, líneas y ancho limitados.
local function limpiar(s)
    if not esUtf8(s) then return "(el aviso no está en UTF-8: no se muestra)" end
    s = s:gsub("^\239\187\191", "")
    s = s:gsub("\r", "")
    s = s:gsub("[%z\1-\8\11-\31\127]", "")
    local salida, n = {}, 0
    for linea in (s .. "\n"):gmatch("(.-)\n") do
        n = n + 1
        if n > MAX_LINEAS then salida[#salida + 1] = "(...)" break end
        salida[#salida + 1] = cortarUtf8(linea, MAX_ANCHO)
    end
    while #salida > 0 and salida[#salida] == "" do salida[#salida] = nil end
    if #salida == 0 then return "(aviso vacío)" end
    return table.concat(salida, "\n")
end

local function leerArchivo()
    local f = io.open(ruta, "rb")
    if not f then return nil end
    local s = f:read(MAX_BYTES + 1) or ""
    f:close()
    if #s > MAX_BYTES then
        -- archivo demasiado grande: se recorta sin partir una letra
        local fin = MAX_BYTES
        while fin > 0 do
            local b = s:byte(fin + 1)
            if not b or b < 0x80 or b >= 0xC0 then break end
            fin = fin - 1
        end
        s = s:sub(1, fin)
    end
    return s
end

local function refrescar()
    local ahora = os.time()
    if ahora - ultimoChequeo < CADA_SEG then return end
    ultimoChequeo = ahora
    local s = leerArchivo()
    if s == nil then
        estado = "sin archivo de avisos"
        return
    end
    estado = "conectado"
    if s ~= firmaAnterior then
        firmaAnterior = s
        contenido = limpiar(s)
        actualizado = os.date("%H:%M:%S")
    end
end

-- ─────────────────────────────────────────────────────────────────────────────
--  PRUEBA 2 · «SOLO MIRAR»: buscar a un jugador en la memoria (solo lectura)
-- ─────────────────────────────────────────────────────────────────────────────
--  Patrón A: los 58 bytes de Player.bin desde el ID (+8) con sus cualidades
--  (base viva olmosjr23, registro 16179). Patrón B: el nombre «Lamine Yamal».
--  Se recorre la memoria por regiones (VirtualQuery, ya declarado por la librería
--  memory de Sider), solo las COMMIT legibles, en trozos pequeños por cuadro para
--  no congelar el menú.
local VK_B        = 0x42
local TROZO       = 24 * 1024 * 1024   -- bytes revisados por cada cuadro con el overlay abierto
local MAX_HITS    = 12
local PID         = 162114
local PAT_A = "\66\121\2\0\38\78\53\49\0\0\0\144\128\132\30\28\0\0\0\83\0\216\185\132\40\21\120\98\214\7\200\15\0\208\170\53\181\9\172\175\27\153\178\3\116\145\40\137\169\164\72\34\128\42\22\66\132\200\9"
local PAT_B = "Lamine Yamal"
local CAMPOS = { {"Velocidad",306}, {"Aceleracion",344}, {"Regate",352}, {"Finalizacion",396}, {"Pase raso",263} }

local sonda = nil      -- estado de la búsqueda
local MBI = nil

local function leerBits(s, pos, n)
    local v = 0
    for i = 0, n - 1 do
        local p = pos + i
        local b = s:byte(math.floor(p / 8) + 1) or 0
        if bit.band(b, bit.lshift(1, p % 8)) ~= 0 then v = v + 2 ^ i end
    end
    return v
end

local function dirNum(p) return tonumber(ffi.cast("uint64_t", p)) end

local function iniciarSonda()
    local si = memory.get_system_info()
    MBI = MBI or ffi.new("MEMORY_BASIC_INFORMATION[1]")
    sonda = {
        dir = dirNum(si.lpMinimumApplicationAddress), tope = dirNum(si.lpMaximumApplicationAddress),
        regIni = 0, regFin = 0, pos = 0,
        hitsA = {}, hitsB = {}, mb = 0, regiones = 0,
        propiaA = dirNum(ffi.cast("const char*", PAT_A)), propiaB = dirNum(ffi.cast("const char*", PAT_B)),
        t0 = os.clock(), hecho = false,
    }
    log("[phoenix] sonda: inicio de la búsqueda de " .. PID .. " (solo lectura)")
end

-- Pasa a la siguiente región legible. Devuelve false cuando ya no quedan.
local function siguienteRegion()
    local tam = ffi.sizeof("MEMORY_BASIC_INFORMATION")
    while sonda.dir < sonda.tope do
        local r = ffi.C.VirtualQuery(ffi.cast("void*", sonda.dir), MBI, tam)
        if r == 0 then return false end
        local base = dirNum(MBI[0].BaseAddress)
        local size = tonumber(MBI[0].RegionSize)
        local estadoMem, prot = tonumber(MBI[0].State), tonumber(MBI[0].Protect)
        sonda.dir = base + size
        local legible = estadoMem == 0x1000 and bit.band(prot, 0x100) == 0 and bit.band(prot, 0x01) == 0
                        and bit.band(prot, 0xEE) ~= 0
        if legible then
            sonda.regIni, sonda.regFin, sonda.pos = base, base + size, base
            sonda.regiones = sonda.regiones + 1
            return true
        end
    end
    return false
end

local function buscarEn(pat, desde, hasta, lista, propia)
    local d = desde
    while #lista < MAX_HITS and d < hasta do
        local h = memory.search(pat, d, hasta)
        if not h then return end
        local a = dirNum(h)
        if a ~= propia then lista[#lista + 1] = a end
        d = a + 1
    end
end

local function pasoSonda()
    if not sonda or sonda.hecho then return end
    local presupuesto = TROZO
    while presupuesto > 0 do
        if sonda.pos >= sonda.regFin then
            if not siguienteRegion() then
                sonda.hecho = true
                sonda.seg = os.clock() - sonda.t0
                log(string.format("[phoenix] sonda: fin. %d regiones, %d MB, %.1f s. Patrón A: %d, nombre: %d",
                    sonda.regiones, sonda.mb, sonda.seg, #sonda.hitsA, #sonda.hitsB))
                for i, a in ipairs(sonda.hitsA) do
                    local rec = memory.read(a - 8, 312)
                    local partes = {}
                    for _, c in ipairs(CAMPOS) do partes[#partes + 1] = c[1] .. " " .. (leerBits(rec, c[2], 6) + 40) end
                    log(string.format("[phoenix] sonda A%d @ %s  ceros_antes=%s  %s", i, memory.hex(a),
                        tostring(rec:sub(1, 8) == string.rep("\0", 8)), table.concat(partes, ", ")))
                end
                for i, a in ipairs(sonda.hitsB) do
                    local ctxb = memory.read(a - 64, 160)
                    log(string.format("[phoenix] sonda B%d @ %s  %s", i, memory.hex(a),
                        (ctxb:gsub(".", function(ch) return string.format("%02x", ch:byte()) end))))
                end
                return
            end
        end
        local fin = math.min(sonda.regFin, sonda.pos + presupuesto)
        local finSolape = math.min(sonda.regFin, fin + #PAT_A)
        buscarEn(PAT_A, sonda.pos, finSolape, sonda.hitsA, sonda.propiaA)
        buscarEn(PAT_B, sonda.pos, finSolape, sonda.hitsB, sonda.propiaB)
        presupuesto = presupuesto - (fin - sonda.pos)
        sonda.mb = sonda.mb + (fin - sonda.pos) / 1048576
        sonda.pos = fin
    end
end

local function textoSonda()
    if not sonda then
        return "\n\n[PRUEBA 2 · SOLO MIRAR] Pulsa B (en el menú, nunca en un partido) para buscar a Lamine Yamal en la memoria."
    end
    if not sonda.hecho then
        return string.format("\n\n[PRUEBA 2] Buscando... %d MB revisados en %d zonas. Patrón: %d · nombre: %d",
            sonda.mb, sonda.regiones, #sonda.hitsA, #sonda.hitsB)
    end
    local t = { string.format("\n\n[PRUEBA 2] Terminado: %d MB en %.1f s · ficha encontrada %d vez/veces · nombre %d vez/veces",
        sonda.mb, sonda.seg or 0, #sonda.hitsA, #sonda.hitsB) }
    for i, a in ipairs(sonda.hitsA) do
        if i > 3 then break end
        local rec = memory.read(a - 8, 312)
        local partes = {}
        for _, c in ipairs(CAMPOS) do partes[#partes + 1] = c[1] .. " " .. (leerBits(rec, c[2], 6) + 40) end
        t[#t + 1] = string.format("  #%d %s → %s", i, memory.hex(a), table.concat(partes, " · "))
    end
    if #sonda.hitsA == 0 then t[#t + 1] = "  No apareció la ficha con ese formato (no es un fallo: se analiza el diario)." end
    return table.concat(t, "\n")
end

function m.key_down(ctx, vkey)
    if apagado then return end
    if vkey == VK_B and (not sonda or sonda.hecho) then iniciarSonda() end
end

local function textoOverlay()
    refrescar()
    pasoSonda()
    local cab = string.format("PHOENIX EVOLUTION  ·  puente en vivo v%s  ·  %s", m.version, estado)
    if actualizado ~= "" then cab = cab .. "  ·  último aviso " .. actualizado end
    return cab .. "\n\n" .. contenido .. textoSonda()
end

-- Ojo: el Lua de Sider NO trae pcall (lo confirma el volcado de env.lua en sider.log).
-- Sider ya atrapa los errores de cada evento y sigue funcionando; para contar fallos usamos una
-- «bandera»: se levanta al entrar y se baja al salir bien. Si al entrar la encontramos levantada,
-- es que la vez anterior hubo un error (Sider la cortó a la mitad).
local enCurso = false

function m.overlay_on(ctx)
    if apagado then return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)" end
    if enCurso then
        fallos = fallos + 1
        log("[phoenix] la vez anterior hubo un error (" .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[phoenix] demasiados errores seguidos: el módulo se detiene hasta reiniciar el juego")
            return "PHOENIX EVOLUTION · módulo detenido por seguridad (ver sider.log)"
        end
    end
    enCurso = true
    local res = textoOverlay()
    enCurso = false
    fallos = 0
    return res
end

function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    ruta = base .. "content\\phoenix\\avisos.txt"
    ctx.register("overlay_on", m.overlay_on)
    ctx.register("key_down", m.key_down)
    log("[phoenix] v" .. m.version .. " listo (solo lectura; tecla B = buscar en memoria). Archivo: " .. ruta)
end

return m
