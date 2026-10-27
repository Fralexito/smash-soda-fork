-- phoenix_marcador_free.lua · v0.1 · marcador y reloj para el Entrenamiento libre (modo «street»)
--
-- Qué hace: SOLO en la zona FREE_TRAINING (id 52) cuenta los goles (evento nativo «goal_scored» de Sider:
-- home_or_away 0 = local, 1 = visita) y lleva un cronómetro propio. Lo muestra en el overlay de Sider
-- (se ve cuando el overlay está abierto: Espacio; y con «1» / «º» se cambia de módulo) y además escribe
-- content\phoenix\marcador_free.json, pensado para un widget siempre visible de Phoenix Overlay.
-- No escribe memoria del juego, no toca archivos del juego. Apagado por defecto.
--
-- Cómo se enciende: en SiderAddons\phoenix_core\phoenix_core.json  →  "marcador_free": true
-- (si la clave falta, está apagado). Se relee cada 5 s: no hace falta reiniciar el juego para apagarlo.
--
-- Reloj: corre mientras pantalla.json diga «en_juego»; se pausa en menús/repetición y durante
-- PAUSA_GOL segundos tras un gol (la celebración). Es una aproximación: el reloj nativo del motor
-- del Entrenamiento libre aún no está localizado.
--
-- Deshacer: DESINSTALAR.ps1 quita la línea de sider.ini; o poner "marcador_free": false.

local m = { version = "0.4" }

local PAUSA_GOL = 12.0      -- segundos que se detiene el reloj tras un gol (celebración)
local MIRAR_SEG = 1.0       -- cada cuánto se relee zona/pantalla
local CONFIG_SEG = 5.0      -- cada cuánto se relee phoenix_core.json
local ESCRIBIR_SEG = 1.0    -- cada cuánto se reescribe marcador_free.json

local base, rutaCfg, rutaSonda, rutaPantalla, rutaSalida, rutaSonda2
local encendido = false
local enZona = false
local pantalla = ""
local golesL, golesV = 0, 0
local segundos = 0.0
local pausaHasta = 0.0
local tAnterior = nil
local tMirar, tConfig, tEscribir = -100, -100, -100

local function leer(ruta)
    local f = io.open(ruta, "rb")
    if not f then return nil end
    local t = f:read("*a")
    f:close()
    return t
end

local function mirarConfig()
    local t = leer(rutaCfg)
    encendido = (t ~= nil) and (t:find('"marcador_free"%s*:%s*true') ~= nil)
end

local canchaOn, rutaCancha, paredesOn = false, nil, false
local function mirarCancha()
    local t = leer(rutaCfg)
    canchaOn = (t ~= nil) and (t:find('"cancha_chica_free"%s*:%s*true') ~= nil)
    paredesOn = canchaOn and (t:find('"cancha_paredes"%s*:%s*true') ~= nil)
end

-- v0.2: en el Entrenamiento libre pantalla.json se queda en «cargando» (solo pasa a «en_juego» con match.stats
-- de un partido real). Señal propia de «el juego está jugando»: Core anota los cambios de estado de los actores
-- en phoenix_core\sonda_partido.txt; si el archivo ha crecido en los últimos 3 s hay actividad (en menú/pausa/carga
-- no crece).
local ACTIVIDAD_SEG = 3.0
local tamSonda = -1
local tCrecio = -100

local function mirarActividad(ahora)
    local f = io.open(rutaSonda2, "rb")
    if not f then return end
    local n = f:seek("end")
    f:close()
    if n and n > tamSonda and tamSonda >= 0 then tCrecio = ahora end
    if n then tamSonda = n end
end

local function mirarZonaYPantalla()
    local s = leer(rutaSonda)
    enZona = (s ~= nil) and (s:find('"nombre"%s*:%s*"FREE_TRAINING"') ~= nil)
    local p = leer(rutaPantalla)
    pantalla = p and (p:match('"pantalla"%s*:%s*"([%w_]+)"') or "") or ""
end

local function reiniciar(motivo)
    golesL, golesV, segundos, pausaHasta, tAnterior = 0, 0, 0.0, 0.0, nil
    log("[marcador_free] marcador y reloj a cero (" .. motivo .. ")")
end

local function texto()
    local mm = math.floor(segundos / 60)
    local ss = math.floor(segundos % 60)
    return string.format("STREET  LOCAL %d - %d VISITA   %02d:%02d", golesL, golesV, mm, ss)
end

local function escribir(ahora)
    if ahora - tEscribir < ESCRIBIR_SEG then return end
    tEscribir = ahora
    local f = io.open(rutaSalida, "wb")
    if not f then return end
    f:write(string.format('{"v":1,"activo":%s,"goles_local":%d,"goles_visita":%d,"segundos":%d}',
        (encendido and enZona) and "true" or "false", golesL, golesV, math.floor(segundos)))
    f:close()
end

function m.display_frame(ctx)
    local ahora = os.clock()
    if ahora - tConfig >= CONFIG_SEG then tConfig = ahora; mirarConfig(); mirarCancha() end
    if not encendido then tAnterior = nil; return end
    if ahora - tMirar >= MIRAR_SEG then tMirar = ahora; mirarZonaYPantalla(); mirarActividad(ahora) end
    if not enZona then tAnterior = nil; return end
    if tAnterior then
        local dt = ahora - tAnterior
        local jugando = (pantalla == "en_juego") or (ahora - tCrecio <= ACTIVIDAD_SEG)
        if dt > 0 and dt < 1.0 and jugando and ahora >= pausaHasta then
            segundos = segundos + dt
        end
    end
    tAnterior = ahora
    escribir(ahora)
end

function m.goal_scored(ctx, info)
    if not encendido or not enZona or not info then return end
    local visita = (tonumber(info.home_or_away) or 0) ~= 0
    if visita then golesV = golesV + 1 else golesL = golesL + 1 end
    pausaHasta = os.clock() + PAUSA_GOL
    log(string.format("[marcador_free] gol %s → %d-%d", visita and "visita" or "local", golesL, golesV))
end

-- v0.3: registro con hora de cada petición de archivo (livecpk_get_filepath) mientras la zona es FREE_TRAINING.
-- Solo escribe en phoenix_core\peticiones.log (tope ~2 MB); no devuelve ruta, así que no altera ninguna carga.
local rutaPeticiones, bytesPeticiones = nil, 0
-- v0.4: con "cancha_chica_free": true en phoenix_core.json sirve las 5 colisiones de portería (.geom) ya desplazadas hacia el centro
-- (content/phoenix/cancha/...), porque StadiumServer solo intercepta hit/stadium y no hit/goal ni FoxAnim.
local GEOM_CANCHA = {
    ["common/render/model/bg/hit/goal/goal_l.geom"] = true, ["common/render/model/bg/hit/goal/goal_r.geom"] = true,
    ["common/anime/foxanim/goal_hit.geom"] = true, ["common/anime/foxanim/goal_l.geom"] = true, ["common/anime/foxanim/goal_r.geom"] = true,
}
function m.get_filepath(ctx, filename, key)
    if not encendido or not enZona then return nil end
    if canchaOn and rutaCancha then
        local fl = string.gsub(string.lower(tostring(filename)), "\\", "/")
        if paredesOn and fl == "common/render/model/bg/hit/bill/st002/hit_billboard.geom" then
            local fp = base .. "content/phoenix/cancha_paredes/" .. fl
            local hp = io.open(fp, "rb")
            if hp then hp:close(); log("[cancha_paredes] sirvo " .. fl); return fp end
        end
        if GEOM_CANCHA[fl] then
            local f = rutaCancha .. fl
            local h = io.open(f, "rb")
            if h then h:close(); log("[cancha_chica_free] sirvo " .. fl); return f end
        end
    end
    if not rutaPeticiones or bytesPeticiones > 2000000 then return nil end
    local linea = string.format("%s %.3f %s" .. "\n", os.date("%H:%M:%S"), os.clock(), tostring(filename))
    local f = io.open(rutaPeticiones, "ab")
    if f then f:write(linea); f:close(); bytesPeticiones = bytesPeticiones + #linea end
    return nil
end

function m.set_teams(ctx)
    -- Entrenamiento libre nuevo: empieza de cero (si aún no sabemos la zona, se reinicia igual: no estorba).
    if encendido then reiniciar("set_teams") end
end

function m.context_reset(ctx)
    if encendido then reiniciar("context_reset") end
end

function m.overlay_on(ctx)
    if not encendido then return "MARCADOR STREET · apagado (phoenix_core.json → marcador_free)" end
    if not enZona then return "MARCADOR STREET · solo en Entrenamiento libre" end
    return texto()
end

function m.init(ctx)
    base = ctx.sider_dir or ".\\"
    rutaCfg = base .. "phoenix_core\\phoenix_core.json"
    rutaSonda = base .. "content\\phoenix\\sonda.json"
    rutaPantalla = base .. "content\\phoenix\\pantalla.json"
    rutaSalida = base .. "content\\phoenix\\marcador_free.json"
    rutaSonda2 = base .. "phoenix_core\\sonda_partido.txt"
    ctx.register("display_frame", m.display_frame)
    ctx.register("goal_scored", m.goal_scored)     -- Sider 7.3.4+ (en 7.3.3 solo deja un WARN)
    ctx.register("set_teams", m.set_teams)
    ctx.register("context_reset", m.context_reset)
    ctx.register("overlay_on", m.overlay_on)
    rutaPeticiones = base .. "phoenix_core\\peticiones.log"
    rutaCancha = base .. "content/phoenix/cancha/"
    ctx.register("livecpk_get_filepath", m.get_filepath)
    log("[marcador_free] v" .. m.version .. " listo (apagado salvo \"marcador_free\": true en phoenix_core.json)")
end

return m
