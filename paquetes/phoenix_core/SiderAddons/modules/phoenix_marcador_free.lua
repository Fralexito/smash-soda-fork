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

local m = { version = "0.1" }

local PAUSA_GOL = 12.0      -- segundos que se detiene el reloj tras un gol (celebración)
local MIRAR_SEG = 1.0       -- cada cuánto se relee zona/pantalla
local CONFIG_SEG = 5.0      -- cada cuánto se relee phoenix_core.json
local ESCRIBIR_SEG = 1.0    -- cada cuánto se reescribe marcador_free.json

local base, rutaCfg, rutaSonda, rutaPantalla, rutaSalida
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
    if ahora - tConfig >= CONFIG_SEG then tConfig = ahora; mirarConfig() end
    if not encendido then tAnterior = nil; return end
    if ahora - tMirar >= MIRAR_SEG then tMirar = ahora; mirarZonaYPantalla() end
    if not enZona then tAnterior = nil; return end
    if tAnterior then
        local dt = ahora - tAnterior
        if dt > 0 and dt < 1.0 and pantalla == "en_juego" and ahora >= pausaHasta then
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
    ctx.register("display_frame", m.display_frame)
    ctx.register("goal_scored", m.goal_scored)     -- Sider 7.3.4+ (en 7.3.3 solo deja un WARN)
    ctx.register("set_teams", m.set_teams)
    ctx.register("context_reset", m.context_reset)
    ctx.register("overlay_on", m.overlay_on)
    log("[marcador_free] v" .. m.version .. " listo (apagado salvo \"marcador_free\": true en phoenix_core.json)")
end

return m
