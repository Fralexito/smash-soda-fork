-- phoenix_arcos.lua  v1.0 (7-nov-2026)  -- Diseño de arco de futsal ALEATORIO por partido, solo para Calle 30x20.
-- Cómo funciona: copia una de las texturas de content\phoenix\arcos\<1..5>\futsal_post_bsm.ftex sobre la textura que usa
-- SOLO el arco de la cancha Calle (stadium-server\Phoenix\Calle 30x20\...\goal\...\futsal_post_bsm.ftex) al elegir equipos
-- (antes de que cargue el estadio). Los estadios normales no usan esa textura: no les afecta.
-- Config: modules\phoenix_arcos.ini  ->  modo=0 (aleatorio) | 1..5 (fijo).   Teclas (overlay de Sider abierto): F6 = siguiente diseño.
-- Sin pcall (no existe en el entorno de módulos de Sider).
local m = {}
local cfg = { modo = 0 }
local NOMBRES = { "rojo/blanco", "negro/blanco", "blanco liso", "azul/blanco", "amarillo/negro" }
local VK_F6 = 0x75
local ultimo = 0

local function ruta_destino(ctx)
    return ctx.sider_dir .. "/content/stadium-server/Phoenix/Calle 30x20/Asset/model/bg/common/goal/sourceimages/#windx11/futsal_post_bsm.ftex"
end

local function leer_ini(ctx)
    local f = io.open(ctx.sider_dir .. "/modules/phoenix_arcos.ini", "r")
    if not f then return end
    for linea in f:lines() do
        local k, v = string.match(linea, "^%s*(%w+)%s*=%s*(%d+)")
        if k == "modo" then cfg.modo = tonumber(v) or 0 end
        if k == "ultimo" then ultimo = tonumber(v) or 0 end
    end
    f:close()
end

local function escribir_ini(ctx)
    local f = io.open(ctx.sider_dir .. "/modules/phoenix_arcos.ini", "w")
    if not f then return end
    f:write("modo=" .. tostring(cfg.modo) .. "\n")
    f:write("ultimo=" .. tostring(ultimo) .. "\n")
    f:close()
end

local function aplicar(ctx, n)
    local src = io.open(ctx.sider_dir .. "/content/phoenix/arcos/" .. n .. "/futsal_post_bsm.ftex", "rb")
    if not src then log("[phoenix_arcos] no encuentro la variante " .. tostring(n)); return false end
    local datos = src:read("*a"); src:close()
    if not datos or #datos < 100 or string.sub(datos, 1, 4) ~= "FTEX" then log("[phoenix_arcos] variante " .. tostring(n) .. " no valida"); return false end
    local dst = io.open(ruta_destino(ctx), "wb")
    if not dst then log("[phoenix_arcos] no puedo escribir la textura de Calle (no existe la carpeta?)"); return false end
    dst:write(datos); dst:close()
    ultimo = n
    escribir_ini(ctx)                          -- recuerda el ultimo entre arranques (para no repetirlo)
    log(string.format("[phoenix_arcos] arco %d (%s) aplicado, modo=%d", n, NOMBRES[n] or "?", cfg.modo))
    return true
end

local function elegir()
    if cfg.modo >= 1 and cfg.modo <= 5 then return cfg.modo end
    local n = math.random(1, 5)
    if n == ultimo then n = (n % 5) + 1 end    -- nunca el mismo dos partidos seguidos
    return n
end

function m.set_teams(ctx)
    leer_ini(ctx)
    aplicar(ctx, elegir())
end

function m.key_down(ctx, vkey)
    if vkey == VK_F6 then
        cfg.modo = (cfg.modo + 1) % 6          -- 0=aleatorio,1..5 fijo
        escribir_ini(ctx)
        log("[phoenix_arcos] F6: modo=" .. cfg.modo .. (cfg.modo == 0 and " (aleatorio)" or (" (" .. NOMBRES[cfg.modo] .. ")")))
    end
end

function m.init(ctx)
    math.randomseed(os.time() * 7919 + math.floor(os.clock() * 100003))
    math.random(); math.random()
    leer_ini(ctx)
    ctx.register("set_teams", m.set_teams)
    ctx.register("key_down", m.key_down)
    log("[phoenix_arcos] v1.1 listo: modo=" .. cfg.modo .. " (0=aleatorio, 1..5 fijo; F6 cambia)")
end

return m
