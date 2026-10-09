-- =============================================================================
--  Phoenix Evolution · Puente en vivo para Sider (PES 2021)
--  Prueba 1 — lee un archivo de texto y lo muestra en el overlay de Sider.
--  Prueba 3 (v0.6, «escribir un número»): tecla V = Velocidad de Lamine a 99 SOLO en la
--  memoria (los archivos no se tocan; al cerrar el juego vuelve a 90). Tecla N = devolverla
--  a su valor. Escribe con WriteProcessMemory (si la zona ya no existe, falla sin cerrar el
--  juego) y solo tras volver a leer y comprobar ID + nombre + cualidades un instante antes.
--  Prueba 2 (v0.4, «solo mirar», lectura segura con copia) — con la tecla B, y SOLO si el overlay muestra
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

local m = { version = "0.14-prueba" }

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
--  v0.3 (prueba en el juego, 02:41): con 58 bytes exactos NO apareció → el juego cambia
--  algunos bits al cargar (forma física, lesión…). v0.4 busca el ID solo (4 bytes) y
--  VERIFICA cada candidato leyendo sus cualidades: si al menos 4 de 5 coinciden, es él.
--  Además busca el nombre en mayúsculas («LAMINE YAMAL»), como lo guarda Player.bin.
local VK_B        = 0x42
local VK_V        = 0x56   -- escribir Velocidad 99
local VK_N        = 0x4E   -- devolver Velocidad original
local VK_K        = 0x4B   -- v0.12: pedir al juego que relea la BASE (pesdb), como hace Editar → Cargar
local VK_P        = 0x50   -- v0.14: recarga COMPLETA (EDIT + base) al entrar a un modo, como Editar → Cargar
local VK_L        = 0x4C   -- v0.11: pedir al juego que recargue EDIT + base al volver al menú principal
local NUEVA_VEL   = 99
local TROZO       = 24 * 1024 * 1024   -- bytes revisados por cada cuadro con el overlay abierto
local MAX_HITS    = 12
local MAX_CAND    = 20000              -- candidatos de ID que se llegan a verificar como máximo
local PID         = 162114
local PAT_A = "\66\121\2\0"            -- 162114 en u32 little-endian
local PAT_B = "LAMINE YAMAL"
local REC_ARCHIVO = "\0\0\0\0\0\0\0\0\66\121\2\0\38\78\53\49\0\0\0\144\128\132\30\28\0\0\0\83\0\216\185\132\40\21\120\98\214\7\200\15\0\208\170\53\181\9\172\175\27\153\178\3\116\145\40\137\169\164\72\34\128\42\22\66\132\200\9\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\76\65\77\73\78\69\32\89\65\77\65\76\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\76\65\77\73\78\69\32\89\65\77\65\76\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\76\97\109\105\110\101\32\89\97\109\97\108\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"   -- la ficha completa tal como está en Player.bin (para comparar)
local OFS_NOMBRE = 129             -- «LAMINE YAMAL» empieza en el byte 129 de la ficha
local CAMPOS = { {"Velocidad",306,90}, {"Aceleracion",344,93}, {"Regate",352,93}, {"Finalizacion",396,81}, {"Pase raso",263,82} }

-- v0.8: la pantalla de habilidades siguió en 90 aunque TODAS las copias empaquetadas ya tenían 99
-- (el log lo prueba: incluso copias nuevas nacían con 99). Entonces la pantalla lee otra forma de la
-- ficha: «desempaquetada» (cada cualidad en su propio byte o número). Se buscan varias formas posibles
-- con las cualidades de Lamine en el orden de la pantalla y en el orden del archivo (sin la velocidad).
local EXTRA = {
    { "pantalla_bytes",  "\83\89\93\92\82\84\81\62\73\85" },
    { "pantalla_menos40", "\43\49\53\52\42\44\41\22\33\45" },
    { "bits_bytes",      "\85\82\93\93\78\83\87\67\76\81\84\92" },
    { "bits_menos40",    "\45\42\53\53\38\43\47\27\36\41\44\52" },
    { "pantalla_u16",    "\83\0\89\0\93\0\92\0\82\0\84\0\81\0\62\0" },
    { "pantalla_u32",    "\83\0\0\0\89\0\0\0\93\0\0\0\92\0\0\0\82\0\0\0\84\0\0\0" },
}

local sonda = nil      -- estado de la búsqueda
local MBI = nil
local BUF_TAM = 1024 * 1024 + 4096   -- copia de trabajo: 1 MB + solape
local buf, bufIni, bufFin = nil, 0, 0
local RPM, WPM, PROC = nil, nil, nil  -- Read/WriteProcessMemory sobre el propio proceso
local escritos = {}                   -- { dir, antes, despues } de cada byte cambiado (para devolverlo con N)
local estadoEscritura = nil

-- CRASH 02:43 (v0.3): leer DIRECTO una zona que otro hilo (ReShade, al recargar efectos)
-- acababa de liberar → 0xC0000005 dentro de sider.dll. Desde v0.4 NUNCA se lee la memoria
-- del juego directamente: se COPIA con ReadProcessMemory, que si la zona ya no existe
-- devuelve «falló» en vez de cerrar el juego. Las búsquedas se hacen sobre esa copia.
local function prepararLector()
    if RPM then return true end
    -- nombres propios (phx_*) apuntando a las funciones de Windows: así no chocan con
    -- declaraciones de otros módulos (ffi.cdef no deja redeclarar).
    -- v0.6: nombres con versión (phx06_*): tras Shift+R el módulo nuevo vive en el MISMO Lua que
    -- el viejo, y ffi.cdef no deja declarar dos veces el mismo nombre.
    ffi.cdef[[
        int   phx06_RPM(void* proceso, const void* desde, void* hacia, size_t n, size_t* leidos) __asm__("ReadProcessMemory");
        int   phx06_WPM(void* proceso, void* hacia, const void* desde, size_t n, size_t* escritos) __asm__("WriteProcessMemory");
        void* phx06_GCP(void) __asm__("GetCurrentProcess");
    ]]
    RPM, WPM, PROC = ffi.C.phx06_RPM, ffi.C.phx06_WPM, ffi.C.phx06_GCP()
    buf = ffi.new("uint8_t[?]", BUF_TAM)
    bufIni = tonumber(ffi.cast("uint64_t", buf)); bufFin = bufIni + BUF_TAM
    return true
end

local leidos1 = nil
-- Copia n bytes desde la dirección a hacia buf (desde el byte 0). Devuelve cuántos copió (0 = falló).
local function copiar(a, n)
    leidos1 = leidos1 or ffi.new("size_t[1]")
    leidos1[0] = 0
    local ok = RPM(PROC, ffi.cast("const void*", a), buf, n, leidos1)
    if ok == 0 then return 0 end
    return tonumber(leidos1[0])
end

-- Lee len bytes como texto Lua, de forma segura (nil si la zona ya no existe).
local function leerSeguro(a, len)
    if copiar(a, len) ~= len then return nil end
    return ffi.string(buf, len)
end

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

local function iniciarSonda(accion)
    prepararLector()
    local si = memory.get_system_info()
    MBI = MBI or ffi.new("MEMORY_BASIC_INFORMATION[1]")
    sonda = {
        dir = dirNum(si.lpMinimumApplicationAddress), tope = dirNum(si.lpMaximumApplicationAddress),
        regIni = 0, regFin = 0, pos = 0,
        hitsA = {}, hitsB = {}, recA = {}, ctxB = {}, extra = {}, mb = 0, regiones = 0, candidatos = 0, fallosLectura = 0, vistos = {},
        propiaA = dirNum(ffi.cast("const char*", PAT_A)), propiaB = dirNum(ffi.cast("const char*", PAT_B)),
        t0 = os.clock(), hecho = false, accion = accion or "mirar",
    }
    for _, e in ipairs(EXTRA) do
        sonda.extra[e[1]] = { hits = {}, ctx = {}, propia = dirNum(ffi.cast("const char*", e[2])) }
    end
    log("[phoenix] sonda v0.8: inicio de la búsqueda de " .. PID .. " (solo lectura, copia segura)")
end

-- Siguiente región privada (heap del juego), COMMIT y legible. La memoria «mapeada» (gráficos,
-- archivos) y la del propio .exe se saltan: ahí no viven las fichas y es la que más cambia.
local function siguienteRegion()
    local tam = ffi.sizeof("MEMORY_BASIC_INFORMATION")
    while sonda.dir < sonda.tope do
        local r = ffi.C.VirtualQuery(ffi.cast("void*", sonda.dir), MBI, tam)
        if r == 0 then return false end
        local base = dirNum(MBI[0].BaseAddress)
        local size = tonumber(MBI[0].RegionSize)
        local estadoMem, prot, tipo = tonumber(MBI[0].State), tonumber(MBI[0].Protect), tonumber(MBI[0].Type)
        sonda.dir = base + size
        local legible = estadoMem == 0x1000 and tipo == 0x20000 and bit.band(prot, 0x100) == 0
                        and bit.band(prot, 0x01) == 0 and bit.band(prot, 0xEE) ~= 0
        local esBuf = base < bufFin and base + size > bufIni
        if legible and not esBuf then
            sonda.regIni, sonda.regFin, sonda.pos = base, base + size, base
            sonda.regiones = sonda.regiones + 1
            return true
        end
    end
    return false
end

-- Busca pat dentro de la copia (buf[0..n)), que corresponde a la dirección real «origen».
local function buscarEnCopia(pat, n, origen, lista, propia, verificar, ctxTabla)
    local d = 0
    while #lista < MAX_HITS and d < n do
        local h = memory.search(pat, bufIni + d, bufIni + n)
        if not h then return end
        local off = dirNum(h) - bufIni
        local a = origen + off
        if a ~= propia and not sonda.vistos[a] then
            sonda.vistos[a] = true
            if verificar then
                sonda.candidatos = sonda.candidatos + 1
                if sonda.candidatos > MAX_CAND then return end
                local guardado = ffi.string(buf, n)          -- la verificación reutiliza buf
                local rec = leerSeguro(a - 8, 312)
                ffi.copy(buf, guardado, n)
                if rec then
                    local k = 0
                    for _, c in ipairs(CAMPOS) do if leerBits(rec, c[2], 6) + 40 == c[3] then k = k + 1 end end
                    if k >= 4 then lista[#lista + 1] = a; sonda.recA[#lista] = rec end
                end
            else
                lista[#lista + 1] = a
                local i0 = math.max(0, off - 64)
                local ct = ctxTabla or sonda.ctxB
                ct[#lista] = ffi.string(buf + i0, math.min(224, n - i0))
            end
        end
        d = off + 1
    end
end

local function hex(s) return (s:gsub(".", function(ch) return string.format("%02x", ch:byte()) end)) end

local function cerrarSonda()
    sonda.hecho = true
    sonda.seg = os.clock() - sonda.t0
    log(string.format("[phoenix] sonda: fin. %d regiones, %d MB, %.1f s. Candidatos ID: %d, fichas verificadas: %d, nombre: %d, lecturas fallidas: %d",
        sonda.regiones, sonda.mb, sonda.seg, sonda.candidatos, #sonda.hitsA, #sonda.hitsB, sonda.fallosLectura))
    for i, a in ipairs(sonda.hitsA) do
        local rec = sonda.recA[i]
        local partes = {}
        for _, c in ipairs(CAMPOS) do partes[#partes + 1] = c[1] .. " " .. (leerBits(rec, c[2], 6) + 40) end
        log(string.format("[phoenix] sonda A%d @ %s  %s  hex=%s", i, memory.hex(a), table.concat(partes, ", "), hex(rec:sub(1, 96))))
    end
    for i, a in ipairs(sonda.hitsB) do
        log(string.format("[phoenix] sonda B%d @ %s  %s", i, memory.hex(a), hex(sonda.ctxB[i] or "")))
    end
    for _, e in ipairs(EXTRA) do
        local x = sonda.extra[e[1]]
        log(string.format("[phoenix] sonda EXTRA %s: %d coincidencia(s)", e[1], #x.hits))
        for i, a in ipairs(x.hits) do
            log(string.format("[phoenix] sonda EXTRA %s #%d @ %s  %s", e[1], i, memory.hex(a), hex(x.ctx[i] or "")))
        end
    end
    -- v0.5: cada nombre que esté en el byte 129 de una ficha → leer la ficha entera y compararla con el archivo
    sonda.copias = {}
    for _, a in ipairs(sonda.hitsB) do
        local ini = a - OFS_NOMBRE
        local rec = leerSeguro(ini, 312)
        if rec and rec:sub(9, 12) == PAT_A and rec:sub(OFS_NOMBRE + 1, OFS_NOMBRE + 12) == PAT_B then
            local dif, lista = 0, {}
            for k = 1, 312 do
                if rec:byte(k) ~= REC_ARCHIVO:byte(k) then
                    dif = dif + 1
                    if #lista < 24 then lista[#lista + 1] = string.format("%d:%02x>%02x", k - 1, REC_ARCHIVO:byte(k), rec:byte(k)) end
                end
            end
            local partes = {}
            for _, c in ipairs(CAMPOS) do partes[#partes + 1] = c[1] .. " " .. (leerBits(rec, c[2], 6) + 40) end
            sonda.copias[#sonda.copias + 1] = { dir = ini, dif = dif, partes = table.concat(partes, " · ") }
            log(string.format("[phoenix] sonda COPIA @ %s  bytes distintos al archivo: %d  [%s]  %s  hex=%s",
                memory.hex(ini), dif, table.concat(lista, " "), table.concat(partes, ", "), hex(rec)))
        end
    end
end

-- ─── Fase 2: escribir UN byte con comprobación previa ───────────────────────────────────
-- Velocidad = bits 306..311 → todos dentro del byte 38 de la ficha (bits 2..7 de ese byte).
local BYTE_VEL, DESPL_VEL = 38, 2

local unByte = nil
local function escribirByte(a, valor)
    unByte = unByte or ffi.new("uint8_t[1]")
    unByte[0] = valor
    local n = ffi.new("size_t[1]")
    local ok = WPM(PROC, ffi.cast("void*", a), unByte, 1, n)
    return ok ~= 0 and tonumber(n[0]) == 1
end

local function escribirVelocidad()
    local hechos, saltados = 0, 0
    for _, a in ipairs(sonda.hitsA) do
        local ini = a - 8
        local rec = leerSeguro(ini, 312)              -- se vuelve a leer JUSTO antes de escribir
        -- v0.7: las copias que usa el juego NO llevan el nombre en el byte 129 (v0.6 las saltó y la
        -- pantalla siguió en 90). Ahora se exige algo igual de seguro y que sí cumplen: ID en +8 y
        -- los bytes 12..65 (todas las cualidades) IDÉNTICOS a Player.bin, salvo el byte de Velocidad.
        local valido = rec and rec:sub(9, 12) == PAT_A
        if valido then
            for k = 13, 66 do
                -- se ignoran el byte de Velocidad y los bytes 54-57 (forma, lesión, pie malo: el juego los varía)
                local ignorar = k == BYTE_VEL + 1 or (k >= 55 and k <= 58)
                if not ignorar and rec:byte(k) ~= REC_ARCHIVO:byte(k) then valido = false break end
            end
        end
        if valido then
            local dir = ini + BYTE_VEL
            local antes = rec:byte(BYTE_VEL + 1)
            local despues = bit.bor(bit.band(antes, 0x03), bit.lshift(NUEVA_VEL - 40, DESPL_VEL))
            if escribirByte(dir, despues) and leerSeguro(dir, 1) == string.char(despues) then
                escritos[#escritos + 1] = { dir = dir, antes = antes, despues = despues }
                hechos = hechos + 1
                log(string.format("[phoenix] fase2: Velocidad %d -> %d en %s (byte %02x -> %02x)",
                    leerBits(rec, 306, 6) + 40, NUEVA_VEL, memory.hex(ini), antes, despues))
            else
                saltados = saltados + 1
                log("[phoenix] fase2: no se pudo escribir en " .. memory.hex(ini) .. " (zona cambiada): no se tocó nada")
            end
        else
            saltados = saltados + 1
            log("[phoenix] fase2: la ficha en " .. memory.hex(ini) .. " ya no coincide: no se tocó")
        end
    end
    estadoEscritura = string.format("Velocidad %d escrita en %d ficha(s)%s. Sal y vuelve a entrar a la pantalla de habilidades de Lamine. (N = devolver a 90)",
        NUEVA_VEL, hechos, saltados > 0 and (" · " .. saltados .. " saltada(s) por seguridad") or "")
end

local function devolverVelocidad()
    prepararLector()
    local ok, no = 0, 0
    for _, e in ipairs(escritos) do
        if leerSeguro(e.dir, 1) == string.char(e.despues) and escribirByte(e.dir, e.antes) then ok = ok + 1 else no = no + 1 end
    end
    log(string.format("[phoenix] fase2: devueltos %d byte(s), %d ya no estaban (el juego los movió o recargó)", ok, no))
    escritos = {}
    estadoEscritura = string.format("Velocidad devuelta a su valor en %d ficha(s)%s.", ok, no > 0 and (" · " .. no .. " ya no estaban") or "")
end

local function pasoSonda()
    if not sonda or sonda.hecho then return end
    local presupuesto = TROZO
    while presupuesto > 0 do
        if sonda.pos >= sonda.regFin then
            if not siguienteRegion() then
                cerrarSonda()
                if sonda.accion == "velocidad" then escribirVelocidad() end
                return
            end
        end
        local fin = math.min(sonda.regFin, sonda.pos + 1024 * 1024)
        local n = math.min(sonda.regFin, fin + 64) - sonda.pos       -- 64 bytes de solape
        local copiado = copiar(sonda.pos, n)
        if copiado > 0 then
            buscarEnCopia(PAT_A, copiado, sonda.pos, sonda.hitsA, sonda.propiaA, true)
            buscarEnCopia(PAT_B, copiado, sonda.pos, sonda.hitsB, sonda.propiaB, false)
            for _, e in ipairs(EXTRA) do
                local x = sonda.extra[e[1]]
                buscarEnCopia(e[2], copiado, sonda.pos, x.hits, x.propia, false, x.ctx)
            end
        else
            sonda.fallosLectura = sonda.fallosLectura + 1   -- la zona cambió: se salta, sin leerla
            if copiar(sonda.pos, 1) == 0 then sonda.pos = sonda.regFin; fin = sonda.regFin end
        end
        presupuesto = presupuesto - (fin - sonda.pos)
        sonda.mb = sonda.mb + (fin - sonda.pos) / 1048576
        sonda.pos = fin
    end
end

local function textoSonda()
    if not sonda then
        return "\n\n[PRUEBAS] B = buscar a Lamine (solo mirar). Solo en el menú, nunca en un partido."
    end
    if not sonda.hecho then
        return string.format("\n\n[PRUEBA 2] Buscando... %d MB revisados en %d zonas. IDs revisados: %d · fichas: %d · nombre: %d",
            sonda.mb, sonda.regiones, sonda.candidatos, #sonda.hitsA, #sonda.hitsB)
    end
    local t = { string.format("\n\n[PRUEBA 2] Terminado: %d MB en %.1f s · IDs revisados %d · fichas de Lamine %d · nombre %d",
        sonda.mb, sonda.seg or 0, sonda.candidatos, #sonda.hitsA, #sonda.hitsB) }
    for i, a in ipairs(sonda.hitsA) do
        if i > 3 then break end
        local rec = sonda.recA[i]
        local partes = {}
        for _, c in ipairs(CAMPOS) do partes[#partes + 1] = c[1] .. " " .. (leerBits(rec, c[2], 6) + 40) end
        t[#t + 1] = string.format("  #%d %s → %s", i, memory.hex(a), table.concat(partes, " · "))
    end
    if #sonda.hitsA == 0 then t[#t + 1] = "  No apareció la ficha verificada (no es un fallo: se analiza el diario)." end
    if estadoEscritura then t[#t + 1] = "  [FASE 2] " .. estadoEscritura end
    local resumen = {}
    for _, e in ipairs(EXTRA) do resumen[#resumen + 1] = e[1] .. " " .. #sonda.extra[e[1]].hits end
    t[#t + 1] = "  [DESEMPAQUETADA] " .. table.concat(resumen, " · ")
    for i, c in ipairs(sonda.copias or {}) do
        t[#t + 1] = string.format("  Copia %d %s → %s · %s", i, memory.hex(c.dir),
            c.dif == 0 and "IGUAL al archivo" or (c.dif .. " bytes distintos"), c.partes)
    end
    return table.concat(t, "\n")
end


-- ─── v0.11 · INTERRUPTOR NATIVO «recargar al volver al menú principal» ──────────────────
-- Hallado el 2026-10-09 leyendo PES2021.exe (sección .trace, sin cifrar):
--   menu::ModeFlowCmnInitFunctor (se ejecuta al volver al menú principal desde un modo) hace
--     cmp byte [exe+0x37F5C39], 0  →  si NO es 0: crea la tarea «editLoadDataInTopMenu»
--     (la misma fábrica 0x1EFB1F0 que usa Editar→Cargar) y pone el byte otra vez a 0.
--   Konami lo usa tras guardar en «Ser una Leyenda» (EditSaveForBLPlayerSave → pone 1).
-- Por seguridad, ANTES de escribir se comprueba que el código del exe es EXACTAMENTE el esperado
-- (si el exe fuera otra versión, no se toca nada). Se escribe 1 byte, en una zona de datos (.bss).
local RVA_BANDERA = 0x37F5C39
local CHEQUEOS = {
    { 0xAEF770,  "\128\61\194\100\208\2\0" },   -- cmp byte ptr [rip+0x2d064c2], 0
    { 0x1EFB440, "\136\13\243\167\143\1\195" }, -- mov [rip+0x18fa7f3], cl ; ret
}
local exeBase = nil
local recarga = nil      -- texto de estado para el overlay
local banderaVista = nil -- último valor leído
local function baseExe()
    if exeBase then return exeBase end
    ffi.cdef[[ void* phx11_GMH(const char* nombre) __asm__("GetModuleHandleA"); ]]
    exeBase = tonumber(ffi.cast("uint64_t", ffi.C.phx11_GMH(nil)))
    return exeBase
end
local function leerBandera()
    local b = leerSeguro(baseExe() + RVA_BANDERA, 1)
    return b and b:byte(1) or nil
end
local function pedirRecarga()
    prepararLector()
    local base = baseExe()
    for _, c in ipairs(CHEQUEOS) do
        local real = leerSeguro(base + c[1], #c[2])
        if real ~= c[2] then
            recarga = string.format("NO se tocó nada: el código en exe+%X no es el esperado (%s)", c[1], real and hex(real) or "ilegible")
            log("[phoenix] " .. recarga)
            return
        end
    end
    local antes = leerBandera()
    if antes == nil or antes > 1 then
        recarga = "NO se tocó nada: el interruptor tiene un valor raro (" .. tostring(antes) .. ")"
        log("[phoenix] " .. recarga); return
    end
    local ok = escribirByte(base + RVA_BANDERA, 1)
    local despues = leerBandera()
    recarga = string.format("[%s] interruptor %d → %s %s · ahora vuelve al MENÚ PRINCIPAL",
        os.date("%H:%M:%S"), antes, tostring(despues), (ok and despues == 1) and "✓" or "✗ (no se pudo escribir)")
    log(string.format("[phoenix] pedir recarga: base exe %s, bandera %d -> %s (ok=%s)", memory.hex(base), antes, tostring(despues), tostring(ok)))
end
local function textoRecarga()
    if not RPM then return "\n  [RECARGA] tecla L = pedir recarga del EDIT y la base al volver al menú principal" end
    local b = leerBandera()
    if banderaVista == 1 and b == 0 then
        log("[phoenix] el juego consumió el interruptor (1 -> 0) a las " .. os.date("%H:%M:%S") .. ": recarga hecha")
        recarga = (recarga or "") .. "  ·  ✓ EL JUEGO RECARGÓ (" .. os.date("%H:%M:%S") .. ")"
    end
    banderaVista = b
    return "\n  [RECARGA] interruptor = " .. tostring(b) .. (recarga and ("  ·  " .. recarga) or "  ·  tecla L = pedir recarga")
end

-- ─── v0.12 · PRUEBA B: releer la BASE (pesdb) sin entrar a Editar ───────────────────────
-- Editar → Cargar (proceso 0x13E4580) hace: gestor = [exe+0x37F5C28] ; 0x1EF2FA0(gestor, 0).
-- 0x1EF2FA0 = «iniciar relectura de la base»: si [gestor+0x88] ≠ 0 (ya hay una) devuelve 0 sin hacer
-- nada; si no, crea un objeto de 0x48 B que el gestor avanza cuadro a cuadro (lee pesdb\*.bin) y
-- luego libera ([gestor+0x88] vuelve a 0). Aquí se LLAMA a esa función del juego desde Sider.
-- Riesgo aceptado por FRALEX (05:17): si el juego no tolera la llamada desde este hilo, puede
-- cerrarse (no se guarda nada, no hay daño en datos). Antes se comprueban los bytes exactos.
local RVA_GESTOR = 0x37F5C28
local RVA_RELEER = 0x1EF2FA0
local RVA_CREAR_GESTOR = 0x1EEBBA0   -- v0.13: crea el gestor si no existe (Editar→Cargar lo llama con 1)
local CHEQUEOS_BASE = {
    { 0x1EEBBA0, "\64\83\72\131\236\48\72\199\68\36\32\254\255\255\255\15\182\217\72\131\61\110\160\144\1\0\117\98\199\68\36\72\35\0" },  -- crear gestor: …cmp [exe+0x37F5C28],0 (34 B)
    { 0x1EF2FA0, "\64\87\72\131\236\48\72\199\68\36\32\254\255\255\255\72\137\92\36\72\139\250\72\139\217" },  -- inicio de la función (25 B)
    { 0x1EF2250, "\72\139\5\209\57\144\1\195" },  -- mov rax, [exe+0x37F5C28] ; ret  (lector del gestor)
    { 0x13E4580, "\64\83\72\131\236\32\72\139\217\177\1\232\16\118\176\0\232\187\220\176\0\51\210\72\139\200\232\1\234\176\0\199" },  -- Editar→Cargar: …xor edx,edx; call 0x1EF2FA0 (32 B)
}
local estadoBase = nil
local releerVisto = nil
local function leerU64(a)
    local s = leerSeguro(a, 8)
    if not s then return nil end
    local v = ffi.new("uint64_t[1]"); ffi.copy(v, s, 8)
    return v[0]
end
local function pedirBase()
    prepararLector()
    local base = baseExe()
    for _, c in ipairs(CHEQUEOS_BASE) do
        local real = leerSeguro(base + c[1], #c[2])
        if real ~= c[2] then
            estadoBase = string.format("NO se llamó: el código en exe+%X no es el esperado", c[1])
            log("[phoenix] " .. estadoBase); return
        end
    end
    local gestor = leerU64(base + RVA_GESTOR)
    if gestor == nil then
        estadoBase = "NO se llamó: no se pudo leer el gestor"
        log("[phoenix] " .. estadoBase); return
    end
    -- v0.13 (05:21): en el menú principal el gestor NO existe (el juego lo crea al entrar a Editar o al
    -- recargar, y lo destruye al volver al menú). Editar→Cargar hace primero 0x1EEBBA0(1) = crearlo.
    if gestor == 0 then
        log(string.format("[phoenix] prueba B: el gestor no existe; llamando a exe+%X(1) para crearlo...", RVA_CREAR_GESTOR))
        ffi.cast("void (*)(uint8_t)", base + RVA_CREAR_GESTOR)(1)
        gestor = leerU64(base + RVA_GESTOR)
        log("[phoenix] prueba B: gestor ahora " .. (gestor and memory.hex(tonumber(gestor)) or "ilegible"))
        if not gestor or gestor == 0 then
            estadoBase = "NO se llamó: no se pudo crear el gestor"
            log("[phoenix] " .. estadoBase); return
        end
    end
    local enCursoB = leerU64(gestor + 0x88)
    if enCursoB == nil or enCursoB ~= 0 then
        estadoBase = "NO se llamó: ya hay una relectura en curso (o no se pudo leer el gestor)"
        log("[phoenix] " .. estadoBase); return
    end
    log(string.format("[phoenix] prueba B: llamando a exe+%X(gestor %s, 0)...", RVA_RELEER, memory.hex(tonumber(gestor))))
    local f = ffi.cast("uint8_t (*)(void*, int)", base + RVA_RELEER)
    local r = f(ffi.cast("void*", gestor), 0)
    estadoBase = string.format("[%s] relectura de la base pedida → %s", os.date("%H:%M:%S"), r ~= 0 and "aceptada ✓" or "rechazada ✗")
    log("[phoenix] prueba B: respuesta " .. tostring(r))
end
local function textoBase()
    if not exeBase then return "\n  [BASE] tecla K = releer la base (prueba B)" end
    local gestor = leerU64(exeBase + RVA_GESTOR)
    local obj = (gestor and gestor ~= 0) and leerU64(gestor + 0x88) or nil
    local ahora = obj and (obj ~= 0 and "EN CURSO" or "libre") or "?"
    if releerVisto == "EN CURSO" and ahora == "libre" then
        log("[phoenix] prueba B: relectura terminada a las " .. os.date("%H:%M:%S"))
        estadoBase = (estadoBase or "") .. "  ·  ✓ BASE RELEÍDA (" .. os.date("%H:%M:%S") .. ")"
    end
    releerVisto = ahora
    return "\n  [BASE] relectura: " .. ahora .. (estadoBase and ("  ·  " .. estadoBase) or "  ·  tecla K = releer la base")
end

-- ─── v0.14 · RECARGA COMPLETA: el menú principal carga «como Cargar» (con la base) ──────────
-- Hallado 05:30: Editar → Cargar es el proceso «ProcessEditDataLoad::CreateReloadPesdb» (tabla en
-- .data 0x34DA048 → fábrica 0x130F220), que construye el proceso con la bandera [+0x8C] = 1 y la pasa
-- como 1.er byte de parámetros a la tarea de carga (0x130F280: «editLoadData», parámetros 1,1,0,0,0).
-- El menú principal (0xAEF78A) usa «mov dword [rsp+0x20], 0x100» → parámetros 0,1,0,0,0: SIN base.
-- La tecla K (v0.13) leyó la base pero NO la aplicó (prueba 05:27: Lamine siguió en 99).
-- PARCHE (solo en memoria, se borra al cerrar el juego; el exe NO se toca): byte exe+0xAEF78E 00 → 01.
-- Así la recarga del menú principal pasa 0x101 = igual que Cargar. Antes se comprueban los 8 bytes.
local RVA_PARAM = 0xAEF78A
local PARAM_ORIGINAL = "\199\68\36\32\0\1\0\0"   -- c7 44 24 20 00 01 00 00
local PARAM_PARCHE   = "\199\68\36\32\1\1\0\0"   -- c7 44 24 20 01 01 00 00
local estadoParche = nil
local VP = nil
local function parchear()
    prepararLector()
    local base = baseExe()
    local real = leerSeguro(base + RVA_PARAM, 8)
    if real == PARAM_PARCHE then estadoParche = "recarga completa YA activa"; return true end
    if real ~= PARAM_ORIGINAL then
        estadoParche = "NO se tocó: el código en exe+AEF78A no es el esperado (" .. (real and hex(real) or "ilegible") .. ")"
        log("[phoenix] " .. estadoParche); return false
    end
    if not VP then
        ffi.cdef[[ int phx14_VP(void* dir, size_t n, uint32_t nueva, uint32_t* vieja) __asm__("VirtualProtect"); ]]
        VP = ffi.C.phx14_VP
    end
    local dir = base + RVA_PARAM + 4
    local vieja = ffi.new("uint32_t[1]")
    if VP(ffi.cast("void*", dir), 1, 0x40, vieja) == 0 then
        estadoParche = "NO se pudo cambiar la protección de la página"; log("[phoenix] " .. estadoParche); return false
    end
    local ok = escribirByte(dir, 1)
    local v2 = ffi.new("uint32_t[1]")
    VP(ffi.cast("void*", dir), 1, vieja[0], v2)
    local despues = leerSeguro(base + RVA_PARAM, 8)
    if ok and despues == PARAM_PARCHE then
        estadoParche = "[" .. os.date("%H:%M:%S") .. "] recarga completa ACTIVADA ✓ (hasta cerrar el juego)"
        log("[phoenix] parche exe+AEF78E 00 -> 01 aplicado (protección original " .. tonumber(vieja[0]) .. ")")
        return true
    end
    estadoParche = "✗ el parche no quedó (" .. (despues and hex(despues) or "ilegible") .. ")"
    log("[phoenix] " .. estadoParche); return false
end
local function recargaCompleta()
    if parchear() then pedirRecarga() end
end
local function textoParche()
    return "\n  [COMPLETA] " .. (estadoParche or "tecla P = recarga completa (EDIT + base) al entrar a un modo")
end

function m.key_down(ctx, vkey)
    if apagado then return end
    local libre = not sonda or sonda.hecho
    if vkey == VK_B and libre then iniciarSonda("mirar") end
    if vkey == VK_L then pedirRecarga() end
    if vkey == VK_K then pedirBase() end
    if vkey == VK_P then recargaCompleta() end
    -- v0.10: V y N DESACTIVADAS. Escribir en memoria ensucia la prueba de la base servida por Sider
    -- (03:44 se pulsó V y volvió a poner 99 en una copia con 90). El camino elegido es el archivo.
end

-- ─── v0.9 · ESPÍA DE LECTURAS (solo anota; no lee ni cambia datos) ─────────────────────
-- Hipótesis: el juego vuelve a leer common\etc\pesdb\Player.bin cada vez que lo necesita
-- (por eso aparecen copias nuevas con Velocidad 90). Si es así, basta con servirle NUESTRO
-- Player.bin (livecpk_get_filepath) y cada pantalla nueva vería los cambios, sin tocar memoria.
local lecturas = {}            -- nombre corto → veces que se empezó a leer
local lineasLog = 0
local function cortoPesdb(nombre)
    local n = string.lower(nombre or "")
    local corto = n:match("pesdb\\([%w_]+%.bin)$")
    return corto
end
-- v0.10: huella de lo que REALMENTE lee el juego (para saber si fue nuestro v95 o el v99).
-- Se lee el trozo que Sider entrega (como hace CommonLib) y se calcula una suma simple cada 64 bytes.
local huella = {}
local function sumaTrozo(s)
    local h = 0
    for i = 1, #s, 64 do h = (h * 31 + s:byte(i)) % 4294967296 end
    return h
end
function m.livecpk_read(ctx, filename, addr, len, total_size, offset)
    local corto = cortoPesdb(filename)
    if not corto then return end
    if corto == "player.bin" and addr and len and len > 0 then
        local h = sumaTrozo(memory.read(addr, len))
        if lineasLog < 300 then
            lineasLog = lineasLog + 1
            log(string.format("[phoenix] huella player.bin: offset %d len %d suma %u", offset or -1, len, h))
        end
    end
    if offset == 0 then
        lecturas[corto] = (lecturas[corto] or 0) + 1
        if lineasLog < 300 then
            lineasLog = lineasLog + 1
            log(string.format("[phoenix] espía: lectura #%d de %s (total %d bytes) a las %s",
                lecturas[corto], corto, total_size or -1, os.date("%H:%M:%S")))
        end
    end
end
local function textoEspia()
    local t = {}
    for k, v in pairs(lecturas) do t[#t + 1] = k .. " ×" .. v end
    table.sort(t)
    return "\n  [ESPÍA] lecturas de la base: " .. (#t > 0 and table.concat(t, " · ") or "ninguna todavía")
end

local function textoOverlay()
    refrescar()
    pasoSonda()
    local cab = string.format("PHOENIX EVOLUTION  ·  puente en vivo v%s  ·  %s", m.version, estado)
    if actualizado ~= "" then cab = cab .. "  ·  último aviso " .. actualizado end
    return cab .. "\n\n" .. contenido .. textoSonda() .. textoEspia() .. textoRecarga() .. textoBase() .. textoParche()
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
    ctx.register("livecpk_read", m.livecpk_read)
    log("[phoenix] v" .. m.version .. " listo (tecla B = buscar en memoria; tecla L = pedir recarga nativa; tecla K = releer la base). Archivo: " .. ruta)
end

return m
