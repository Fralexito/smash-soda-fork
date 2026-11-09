-- phoenix_street_reglas.lua · v0.1 · reglas de calle para el Street (APAGADO por defecto)
-- Reglas: (1) "primero a N goles" gana y termina; (2) "gol de oro": si al acabar el tiempo hay empate, el siguiente gol gana.
-- Claves en SiderAddons\phoenix_core/phoenix_core.json:  "street_regla_meta": 0 (0 = apagado, 3..15 = goles para ganar) ; "street_gol_oro": false
-- Solo actua en FREE_TRAINING con street_activo. No escribe memoria. Para terminar el partido anade la linea "street_fin_pedido=1" (una sola vez)
-- a phoenix_core/ordenes.txt; el Core 0.5.13+ la atiende (final nativo con revancha) y borra el archivo.
local m = { version = "0.1" }
local base, rutaCfg, rutaSonda, rutaIni
local meta, oro = 0, false
local golesL, golesV, finPedido, avisoHasta, aviso = 0, 0, false, 0, ""
local enZona, tCfg, tZona = false, -100, -100
local function leer(r) local f = io.open(r, "rb"); if not f then return nil end; local t = f:read("*a"); f:close(); return t end
local tPedido = 0
local function pedirFin()   -- igual que el panel: se anade una linea a ordenes.txt (el Core la lee cada ~1 s y borra el archivo)
    local f = io.open(base .. "phoenix_core/ordenes.txt", "ab")
    if not f then return false end
    f:write("street_fin_pedido=1" .. string.char(13, 10)); f:close()
    return true
end
local function releer()
    local t = leer(rutaCfg) or ""
    local c = leer(rutaIni) or ""
    meta = tonumber(c:match("street_regla_meta%s*=%s*([%d%.]+)")) or 0
    meta = math.floor(meta + 0.5)
    oro = (c:match("street_gol_oro%s*=%s*(%d)") or "0") ~= "0"
    if t:find('"street_activo"%s*:%s*false') then meta = 0; oro = false end
end
local function terminar(motivo, ahora)
    if finPedido then return end
    finPedido = true
    aviso = motivo; avisoHasta = ahora + 15
    pedirFin(); tPedido = ahora
    log("[street_reglas] " .. motivo)
end
function m.goal_scored(ctx, info)
    if not enZona or not info then return end
    if (tonumber(info.home_or_away) or 0) ~= 0 then golesV = golesV + 1 else golesL = golesL + 1 end
    local ahora = os.clock()
    if meta > 0 and (golesL >= meta or golesV >= meta) then
        terminar(string.format("GANA %s: %d-%d (a %d goles)", golesL >= meta and "LOCAL" or "VISITA", golesL, golesV, meta), ahora)
    elseif oro then
        local st = match and match.stats and match.stats()
        local mi = st and tonumber(st.clock_minutes) or 0
        if mi >= 90 and golesL ~= golesV then terminar(string.format("GOL DE ORO: %d-%d", golesL, golesV), ahora) end
    end
end
function m.display_frame(ctx)
    local ahora = os.clock()
    if ahora - tCfg >= 5 then tCfg = ahora; releer() end
    if finPedido and ahora - tPedido > 3 then   -- el Core borra ordenes.txt al atender: entonces se puede volver a pedir (revancha: marcador a 0)
        local o = leer(base .. "phoenix_core/ordenes.txt") or ""
        if not o:find("street_fin_pedido") or ahora - tPedido > 20 then finPedido = false; golesL, golesV = 0, 0 end
    end
    if ahora - tZona >= 1 then tZona = ahora; local s = leer(rutaSonda) or ""; enZona = s:find('"nombre"%s*:%s*"FREE_TRAINING"') ~= nil end
end
function m.reiniciar() golesL, golesV, finPedido, aviso = 0, 0, false, "" end
function m.overlay_on(ctx)
    if meta == 0 and not oro then return "REGLAS CALLE: apagadas" end
    if os.clock() < avisoHasta then return "*** " .. aviso .. " ***" end
    return string.format("REGLAS CALLE: a %s goles · gol de oro %s · %d-%d", meta > 0 and tostring(meta) or "-", oro and "SI" or "no", golesL, golesV)
end
function m.init(ctx)
    base = ctx.sider_dir or ".\\"
    rutaCfg = base .. "phoenix_core/phoenix_core.json"
    rutaSonda = base .. "content/phoenix/sonda.json"
    rutaIni = base .. "content/phoenix/reglas_street.ini"
    ctx.register("display_frame", m.display_frame)
    ctx.register("goal_scored", m.goal_scored)
    ctx.register("set_teams", m.reiniciar)
    ctx.register("context_reset", m.reiniciar)
    ctx.register("overlay_on", m.overlay_on)
    log("[street_reglas] v" .. m.version .. " listo (apagado salvo street_regla_meta>0 o street_gol_oro)")
end
return m
