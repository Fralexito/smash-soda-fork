-- =============================================================================
--  Phoenix Evolution · PHOENIX SONDA (módulo de Sider para PES 2021) · pieza del MCP de PES
--  Escribe content\phoenix\sonda.json con TODO lo que Sider sabe del juego, para que el
--  servidor MCP (pes-mcp\servidor.mjs) se lo dé a Claude en vivo:
--    · ctx completo (equipos, torneo, estadio, clima, hora, dificultad, duración, cambios…)
--    · match.stats() completo (marcador, periodo, reloj, añadido, penales y el puntero «ptr»)
--    · zona del juego (menú principal, Partido, Liga Máster, Editar…) leída del gestor de modos
--    · los últimos 40 eventos de Sider con sus datos (set_teams, set_match_settings, goal_scored…)
--
--  Seguridad:
--    · SOLO LECTURA. Ningún evento devuelve nada (no cambia equipos, estadio, ajustes ni archivos).
--    · No escribe memoria. La zona se lee con ReadProcessMemory (copia segura: si la zona ya no
--      existe, falla sin cerrar el juego) y solo si el código del exe es el estudiado (si no, se omite).
--    · Sin ffi (luajit.ext.enabled = 0) funciona igual, solo que sin «zona».
--    · Sin pcall en Sider 7.3.3: «bandera» de errores; tras 5 seguidos el módulo se apaga solo.
--    · Escribe como mucho 1 vez por segundo (y al momento cuando llega un evento).
--
--  Opcional y suelto: funciona en un PES 2021 limpio con Sider 7.3.x (ConmeGOL es solo banco de pruebas).
--  Instalar: copiar a SiderAddons\modules\ y añadir al final de la lista de módulos de sider.ini:
--      lua.module = "phoenix_sonda.lua"
--  Desinstalar = quitar la línea (o poner ; delante). En sider.log debe salir «[sonda] v1.0 listo».
-- =============================================================================

local m = { version = "1.0.1" }

local CADA_SEG   = 1.0
local MAX_EVENTOS = 40
local MAX_FALLOS = 5

local ruta = nil
local apagado, fallos, enCurso = false, 0, false
local ultimo = -1000
local pendiente = false
local seq = 0
local eventos = {}
local ctxVisto = nil

-- ─── JSON mínimo (sin librerías) ─────────────────────────────────────────────
local function jsonTexto(s)
    s = tostring(s or "")
    s = s:gsub('[%c"\\]', function(c)
        if c == '"' then return '\\"' end
        if c == "\\" then return "\\\\" end
        return string.format("\\u%04x", c:byte())
    end)
    return '"' .. s .. '"'
end

local function json(v, prof)
    prof = prof or 0
    local t = type(v)
    if t == "nil" then return "null" end
    if t == "boolean" then return v and "true" or "false" end
    if t == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return "null" end
        if v == math.floor(v) and math.abs(v) < 1e15 then return string.format("%d", v) end
        return string.format("%.6g", v)
    end
    if t == "string" then return jsonTexto(v) end
    if t ~= "table" or prof > 3 then return jsonTexto(tostring(v)) end
    local n = #v
    if n > 0 then
        local partes = {}
        for i = 1, math.min(n, 64) do partes[i] = json(v[i], prof + 1) end
        return "[" .. table.concat(partes, ",") .. "]"
    end
    local claves = {}
    for k, x in pairs(v) do
        local tx = type(x)
        if tx ~= "function" then claves[#claves + 1] = tostring(k) end
        if #claves >= 80 then break end
    end
    local partes = {}
    for _, k in ipairs(claves) do
        local x = v[k]
        if x == nil then x = v[tonumber(k)] end
        partes[#partes + 1] = jsonTexto(k) .. ":" .. json(x, prof + 1)
    end
    return "{" .. table.concat(partes, ",") .. "}"
end

-- Copia plana de una tabla: números, textos y booleanos tal cual; punteros (userdata/cdata) como texto.
local function plano(tb)
    local r = {}
    if type(tb) ~= "table" then return r end
    for k, v in pairs(tb) do
        local tv = type(v)
        if tv == "number" or tv == "string" or tv == "boolean" then r[k] = v
        elseif tv == "userdata" or tv == "cdata" then r[k] = tostring(v)
        elseif tv == "table" and k ~= "kits" then r[k] = plano(v) end
    end
    return r
end

-- ─── ZONA del juego (solo lectura, copia segura) ─────────────────────────────
-- Misma lectura que phoenix.lua v0.17d (verificada el 9-oct): gestor de modos = [exe+0x3704E38],
-- modo actual = u32 [gestor+0xF0], cambio en curso = byte [gestor+0x12C], siguiente = u32 [gestor+0x130].
local RVA_PTR_MODOS = 0x3704E38
local CHEQUEOS = {
    { 0x149A470, "\72\139\5\193\169\38\2\195" },
    { 0x149F4F0, "\139\129\240\0\0\0\51\201\131\248\61\15\67\193" },
    { 0x149F510, "\128\185\44\1\0\0\0\116\8\139\129\48\1\0\0\235\6" },
}
local NOMBRES_ZONA = { [0] = "UNKNOWN", "TEST_MENU", "TEST_ONLINE", "TEST_1ON1", "BOOT_STEP", "TITLE_DEMO_LOOP", "FIRST_SETTINGS", "TOP_MENU", "EXHIBITION", "COMPETITION", "PD_VERSUS", "RS_MATCH", "COOP_MATCH", "EDIT", "EDIT_LIVEUPDATE", "UEFA_CL", "UEFA_EL", "UEFA_EURO", "UEFA_LEAGUE", "UEFA_ML", "UEFA_BL", "CONMEBOL_LB", "CONMEBOL_SD", "AFC_ACL", "ONLINE_RANK", "ONLINE_QUICK", "ONLINE_LOBBY", "ONLINE_MOM_LOBBY", "ONLINE_COOP_MATCH", "ONLINE_WATCHMATCH", "MYCLUB", "MYCLUB_RANK", "MYCLUB_COM_MATCH", "MYCLUB_COM_COACH", "MYCLUB_COM_COMPE", "MYCLUB_LOBBY", "MYCLUB_EVENT_COMPE", "MYCLUB_COOP_MATCH", "MYCLUB_EVENT_COMPE_COOP", "MYCLUB_COM_COMPE_COOP", "MYCLUB_PES_LEAGUE_MATCHING", "MYCLUB_PES_LEAGUE_CLAN_COOP_MATCHING", "MYCLUB_PES_LEAGUE_LOBBY", "MYCLUB_MATCHDAY", "MYCLUB_MATCHDAY_COOP", "STATS_AND_INFO", "STATS_AND_INFO_INFO", "STATS_AND_INFO_DP", "STATS_AND_INFO_LIVE", "ONLINE_SETTINGS", "ONLINE_EVENT", "STEPUP_TUTORIAL", "FREE_TRAINING", "SYSTEM_SETTINGS", "PES_LEAGUE", "PES_LEAGUE_MATCHING", "PES_LEAGUE_CLAN_COOP_MATCHING", "PES_LEAGUE_RANDOM_MATCHING", "PES_LEAGUE_LOBBY", "MATCHDAY", "MATCHDAY_COOP" }

local RPM, PROC, BASE, buf, leidos = nil, nil, nil, nil, nil
local zonaOK = nil   -- nil = sin comprobar · true = exe estudiado · false = no se lee

local function prepararLector()
    if RPM then return true end
    if not ffi then return false end
    -- nombres propios (phxs1_*) para no chocar con otros módulos (ffi.cdef no deja redeclarar).
    -- Tras Shift+R el módulo nuevo vive en el MISMO Lua: la marca en la tabla ffi (compartida) evita
    -- declararlos dos veces. (La v1.0 usaba rawget, que el Lua de Sider NO trae: probado el 25-oct.)
    if not ffi.__phxs1_cdef then
        ffi.cdef[[
            int   phxs1_RPM(void* p, const void* desde, void* hacia, size_t n, size_t* leidos) __asm__("ReadProcessMemory");
            void* phxs1_GCP(void) __asm__("GetCurrentProcess");
            void* phxs1_GMH(const char* nombre) __asm__("GetModuleHandleA");
        ]]
        ffi.__phxs1_cdef = true
    end
    RPM, PROC = ffi.C.phxs1_RPM, ffi.C.phxs1_GCP()
    BASE = tonumber(ffi.cast("uint64_t", ffi.C.phxs1_GMH(nil)))
    buf = ffi.new("uint8_t[?]", 256); leidos = ffi.new("size_t[1]")
    return true
end

local function leer(a, n)
    leidos[0] = 0
    if RPM(PROC, ffi.cast("const void*", a), buf, n, leidos) == 0 or tonumber(leidos[0]) ~= n then return nil end
    return ffi.string(buf, n)
end

local function u32(s, i)
    local a, b, c, d = s:byte(i, i + 3)
    return a + b * 256 + c * 65536 + d * 16777216
end

local function mirarZona()
    if zonaOK == false or not prepararLector() then return nil end
    if zonaOK == nil then
        zonaOK = true
        for _, c in ipairs(CHEQUEOS) do
            if leer(BASE + c[1], #c[2]) ~= c[2] then
                zonaOK = false
                log(string.format("[sonda] zona: el código en exe+%X no es el estudiado: zona apagada", c[1]))
                return nil
            end
        end
    end
    local p = leer(BASE + RVA_PTR_MODOS, 8)
    if not p then return nil end
    local lo, hi = u32(p, 1), u32(p, 5)
    if hi >= 0x8000 or (hi == 0 and lo < 0x10000) then return { id = -1, nombre = "sin gestor" } end
    local s = leer(hi * 4294967296 + lo + 0xF0, 0x44)
    if not s then return nil end
    local id, cambiando, sig = u32(s, 1), s:byte(0x3D), u32(s, 0x41)
    local z = { id = id, nombre = NOMBRES_ZONA[id] or "?" }
    if cambiando ~= 0 then z.siguiente = sig; z.siguiente_nombre = NOMBRES_ZONA[sig] or "?" end
    return z
end

-- ─── escritura ───────────────────────────────────────────────────────────────
local function escribir(ctx)
    seq = seq + 1
    local st = match and match.stats and match.stats() or nil
    local datos = {
        v = 1, sonda = m.version, t = os.time(), reloj = os.clock(), seq = seq,
        ctx = plano(ctx or ctxVisto),
        stats = st and plano(st) or nil,
        en_partido = st ~= nil,
        zona = mirarZona(),
        eventos = eventos,
    }
    local f = io.open(ruta, "wb")
    if not f then return end      -- el lector lo tiene abierto: se reintenta al siguiente segundo
    f:write(json(datos))
    f:close()
end

local function anotar(nombre, info)
    if apagado then return end
    local e = { t = os.time(), reloj = os.clock(), evento = nombre }
    if info ~= nil then e.datos = (type(info) == "table") and plano(info) or info end
    eventos[#eventos + 1] = e
    while #eventos > MAX_EVENTOS do table.remove(eventos, 1) end
    pendiente = true
end

-- ─── eventos de Sider (todos devuelven nil: no cambian nada) ─────────────────
function m.display_frame(ctx)
    if apagado or not ruta then return end
    if enCurso then
        fallos = fallos + 1
        if fallos >= MAX_FALLOS then apagado = true; log("[sonda] 5 errores seguidos: módulo apagado hasta reiniciar el juego") return end
    end
    local t = os.clock()
    if not pendiente and t - ultimo < CADA_SEG then return end
    enCurso = true
    ctxVisto = ctx
    ultimo, pendiente = t, false
    escribir(ctx)
    enCurso = false
    fallos = 0
end

function m.set_teams(ctx, home, away) anotar("set_teams", { local_id = home, visita_id = away }) end
function m.set_match_settings(ctx, opciones) anotar("set_match_settings", opciones) end
function m.set_conditions(ctx, opciones) anotar("set_conditions", opciones) end
function m.after_set_conditions(ctx) anotar("after_set_conditions") end
function m.set_stadium(ctx, opciones) anotar("set_stadium", opciones) end
function m.set_stadium_choice(ctx, estadio) anotar("set_stadium_choice", { estadio = estadio }) end
function m.context_reset(ctx) anotar("context_reset") end
function m.goal_scored(ctx, info) anotar("goal_scored", info) end
function m.show(ctx) anotar("show") end
function m.hide(ctx) anotar("hide") end

function m.init(ctx)
    local carpeta = (ctx.sider_dir or ".\\") .. "content\\phoenix\\"
    if fs and fs.make_dirs then fs.make_dirs(carpeta) end
    ruta = carpeta .. "sonda.json"
    ctxVisto = ctx
    ctx.register("display_frame", m.display_frame)
    ctx.register("set_teams", m.set_teams)
    ctx.register("set_match_settings", m.set_match_settings)
    ctx.register("set_conditions", m.set_conditions)
    ctx.register("after_set_conditions", m.after_set_conditions)
    ctx.register("set_stadium", m.set_stadium)
    ctx.register("set_stadium_choice", m.set_stadium_choice)
    ctx.register("context_reset", m.context_reset)
    ctx.register("show", m.show)
    ctx.register("hide", m.hide)
    ctx.register("goal_scored", m.goal_scored)   -- Sider 7.3.4+ (en 7.3.3 solo deja un WARN)
    log("[sonda] v" .. m.version .. " listo. Escribe " .. ruta .. (ffi and " (con zona)" or " (sin ffi: sin zona)"))
end

return m
