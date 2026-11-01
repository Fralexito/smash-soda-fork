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

local m = { version = "0.8" }

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
    encendido = (t ~= nil) and (t:find('"marcador_free"%s*:%s*true') ~= nil) and not t:find('"street_activo"%s*:%s*false')   -- 0.4.102: con el Street apagado no hay marcador
end

local canchaOn, rutaCancha, paredesOn, seasonForzada, suplentesCero = false, nil, false, nil, false
local arcoMedida = 0
local CARPETA_ARCO = { [1] = "arco_4x2/", [2] = "arco_3x2/", [3] = "arco_5x22/" }
local climaForzado = nil   -- v0.8: json "clima_forzado": 0 = sin lluvia (la lluvia pone el cielo/fondo blanco en la cancha chica)
local function mirarCancha()
    local t = leer(rutaCfg)
    local sf = t and t:match('"season_forzada"%s*:%s*(%d)')
    seasonForzada = sf and tonumber(sf) or nil
    local cf = t and t:match('"clima_forzado"%s*:%s*(%d)')
    climaForzado = cf and tonumber(cf) or nil
    suplentesCero = (t ~= nil) and (t:find('"suplentes_cero"%s*:%s*1') ~= nil)
    canchaOn = (t ~= nil) and (t:find('"cancha_chica_free"%s*:%s*true') ~= nil)
    local am = t and t:match('"arco_medida"%s*:%s*([%d%.]+)')   -- 0.4.105: 0 original, 1 = 4x2, 2 = 3x2, 3 = 5x2,2
    arcoMedida = am and math.floor(tonumber(am) + 0.5) or 0
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

-- v0.9 (0.4.99): el marcador y el reloj del módulo se sincronizan con match.stats() del juego una vez por segundo.
-- Antes solo contaban goal_scored y se reiniciaban en set_teams/context_reset: la revancha propia (sin set_teams) los dejaba con los goles y los segundos del partido anterior.
local function sincronizarConJuego()
    -- 0.4.104: el Lua de Sider no trae pcall (daba «attempt to call global pcall» en cada fotograma y dejaba muerta la sincronización)
    local st = match and match.stats and match.stats()
    if not st then return end
    local hl, al = tonumber(st.home_score), tonumber(st.away_score)
    if hl and al and (hl ~= golesL or al ~= golesV) then
        log(string.format("[marcador_free] marcador sincronizado con el juego %d-%d -> %d-%d", golesL, golesV, hl, al))
        golesL, golesV = hl, al
    end
    local mi, se = tonumber(st.clock_minutes), tonumber(st.clock_seconds)
    if mi and se then
        local t = mi * 60 + se
        if t + 30 < segundos then log(string.format("[marcador_free] reloj sincronizado con el juego %d s -> %d s", math.floor(segundos), t)); segundos = t end
    end
end

-- 0.4.104: césped de la minicancha. StadiumServer.lua (cargado antes) contesta él mismo la petición de pitch_st002.fpk con su copia original, así que el servicio
-- por livecpk de este módulo nunca llegaba a tocarlo (en el log solo salían los arcos). Solución: con Street + cancha chica en FREE_TRAINING se copia el césped chico sobre
-- el archivo que sirve StadiumServer y al salir de la zona / apagar el Street / arrancar PES se restaura el original (respaldo-20261026 = original del juego).
local rutaPitchSS, rutaPitchOrig, rutaPitchChica
local cespedChica = false
local function copiarArchivo(origen, destino)
    local t = leer(origen)
    if not t or #t < 1000 then return false end
    local f = io.open(destino, "wb")
    if not f then return false end
    local ok = f:write(t)
    f:close()
    return ok ~= nil
end
local function ajustarCesped()
    if not rutaPitchSS then return end
    local quiere = encendido and enZona and canchaOn
    if quiere == cespedChica then return end
    local ok = copiarArchivo(quiere and rutaPitchChica or rutaPitchOrig, rutaPitchSS)
    if ok then
        cespedChica = quiere
        log("[cancha_visual] césped " .. (quiere and "CHICO puesto" or "ORIGINAL restaurado") .. " en StadiumServer (pitch_st002.fpk)")
    else
        log("[cancha_visual] no pude cambiar el césped (" .. (quiere and "chico" or "original") .. "); se reintenta")
    end
end

local function mirarZonaYPantalla()
    local s = leer(rutaSonda)
    enZona = (s ~= nil) and (s:find('"nombre"%s*:%s*"FREE_TRAINING"') ~= nil)
    ajustarCesped()
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
    if ahora - tMirar >= MIRAR_SEG then tMirar = ahora; mirarZonaYPantalla(); if enZona then sincronizarConJuego() end; mirarActividad(ahora) end
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
-- 1-nov (Core 0.4.102): la cancha DIBUJADA del street (césped 30x20 y arcos a ±15) estaba en el AddOn «Goal Post Colour\Global»
-- del Camp Nou de StadiumServer, así que salía siempre, con el Street apagado y en cualquier partido en el Camp Nou. Ahora vive
-- en content\phoenix\cancha_visual\ y la sirve este módulo SOLO en el Entrenamiento con el Street encendido (street_activo).
local CANCHA_VISUAL = {}
for _, r in ipairs({ "goal_post.fpk", "goal_post.fpkd", "goalnet_l.fpk", "goalnet_l.fpkd", "goalnet_r.fpk", "goalnet_r.fpkd",
                     "goalnet_hangpole_l.fpk", "goalnet_hangpole_l.fpkd", "goalnet_hangpole_r.fpk", "goalnet_hangpole_r.fpkd" }) do
    CANCHA_VISUAL["asset/model/bg/common/goal/#win/" .. r] = true
end
CANCHA_VISUAL["asset/model/bg/st002/pitch/#win/pitch_st002.fpk"] = true
function m.get_filepath(ctx, filename, key)
    if not encendido or not enZona then
        local fz = string.gsub(string.gsub(string.lower(tostring(filename)), "\\", "/"), "//+", "/")
        if encendido and CANCHA_VISUAL[(string.gsub(fz, "^/+", ""))] then log("[cancha_visual] pedido " .. fz .. " pero NO se sirve: zona no es FREE_TRAINING todavía") end
        return nil
    end
    if canchaOn and rutaCancha then
        local fl = string.gsub(string.lower(tostring(filename)), "\\", "/")
        local fl2 = string.gsub(fl, "//+", "/")
        if paredesOn and (fl2 == "common/render/model/bg/hit/bill/hit_billboard.geom" or fl2 == "common/render/model/bg/hit/bill/st002/hit_billboard.geom") then
            local fp = base .. "content/phoenix/cancha_paredes/common/render/model/bg/hit/bill/st002/hit_billboard.geom"
            local hp = io.open(fp, "rb")
            if hp then hp:close(); log("[cancha_paredes] sirvo " .. fl); return fp end
        end
        local flv = string.gsub(fl2, "^/+", "")
        local carp = CARPETA_ARCO[arcoMedida]
        if carp and (CANCHA_VISUAL[flv] or GEOM_CANCHA[fl]) and flv ~= "asset/model/bg/st002/pitch/#win/pitch_st002.fpk" then
            local fa = base .. "content/phoenix/cancha_visual/" .. carp .. flv
            local ha = io.open(fa, "rb")
            if ha then ha:close(); log("[cancha_visual] sirvo (" .. carp .. ") " .. flv); return fa end
        end
        if CANCHA_VISUAL[flv] then
            local f = base .. "content/phoenix/cancha_visual/" .. flv
            local h = io.open(f, "rb")
            if h then h:close(); log("[cancha_visual] sirvo " .. flv); return f end
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

-- v0.5: con "season_forzada": 0|1 en phoenix_core.json, el Entrenamiento libre usa siempre esa temporada (0 verano, 1 invierno).
-- El vacío blanco y el césped oscuro de la cancha chica salían con season 0 (el gestor de clima la elige según el mes real).
function m.set_conditions(ctx, options)
    mirarConfig(); mirarCancha()
    if (seasonForzada == nil and climaForzado == nil) or not encendido or not options then return nil end
    local s = leer(rutaSonda)
    if not (s and s:find('"nombre"%s*:%s*"FREE_TRAINING"')) then return nil end
    local cambio = false
    if seasonForzada ~= nil and tonumber(options.season) ~= seasonForzada then
        log(string.format("[marcador_free] season %s -> %d (entrenamiento)", tostring(options.season), seasonForzada))
        options.season = seasonForzada; cambio = true
    end
    if climaForzado ~= nil and (tonumber(options.weather) ~= climaForzado or (options.weather_effects ~= nil and tonumber(options.weather_effects) ~= 0)) then
        log(string.format("[marcador_free] clima %s (efectos %s) -> %d sin efectos (entrenamiento)", tostring(options.weather), tostring(options.weather_effects), climaForzado))
        options.weather = climaForzado; if options.weather_effects ~= nil then options.weather_effects = 0 end; cambio = true
    end
    if not cambio then return nil end
    return options
end

-- v0.6: con "suplentes_cero": 1 el Entrenamiento libre se juega sin banquillo (substitutions = 0): prueba para que no salgan los suplentes en fila en la banda.
function m.set_match_settings(ctx, options)
    mirarConfig(); mirarCancha()
    if not suplentesCero or not encendido or not options then return nil end
    local s = leer(rutaSonda)
    if not (s and s:find('"nombre"%s*:%s*"FREE_TRAINING"')) then return nil end
    log(string.format("[marcador_free] suplentes %s -> 0 (entrenamiento)", tostring(options.substitutions)))
    options.substitutions = 0
    return options
end

function m.set_teams(ctx)
    -- Entrenamiento libre nuevo: empieza de cero (si aún no sabemos la zona, se reinicia igual: no estorba).
    if encendido then reiniciar("set_teams") end
end

function m.context_reset(ctx)
    if encendido then reiniciar("context_reset") end
end

-- v0.8 «panel Phoenix Street»: F9 abre/cierra · F10 siguiente · F11 ON/OFF · ← → ajusta el valor. Escribe órdenes en phoenix_core\ordenes.txt (el Core las lee cada segundo y guarda el json).
-- Las claves congeladas del portero NO están en la lista. «vivo» = se aplica al instante; «sig.partida» = se guarda y se usa en la próxima partida/arranque.
local PANEL = {
    { id = "street_activo", txt = "MODO STREET (todo el Street en el Entrenamiento)", tipo = "bool", def = true, modo = "fuera" },   -- 0.4.102
    { id = "street_1v1_saque", txt = "1v1 saque: 0 normal · 1 portero centro · 2 portero su área · 3 libre", tipo = "num", paso = 1, lo = 0, hi = 3, def = 0, modo = "vivo" },
    { id = "arco_medida", txt = "Medida del arco: 0 original · 1 = 4x2 · 2 = 3x2 · 3 = 5x2,2 (al reentrar al entrenamiento)", tipo = "num", paso = 1, lo = 0, hi = 3, def = 0, modo = "sig.partida" },
    { id = "street_l1_sin_portero", txt = "L1 no elige al arquero salvo balón cerca o hacia él (1 sí · 0 no; L1+R3 lo fuerza)", tipo = "num", paso = 1, lo = 0, hi = 1, def = 1, modo = "vivo" },
    { id = "arquero_dist_seleccion", txt = "  Distancia para poder elegir al arquero (m)", tipo = "num", paso = 1, lo = 2, hi = 25, def = 7, modo = "vivo" },
    { id = "red_tope_rebote", txt = "Tope de rebote en la red (no sale más rápido de lo que entró): 1 sí · 0 no", tipo = "num", paso = 1, lo = 0, hi = 1, def = 1, modo = "vivo" },
    { id = "red_anclas", txt = "Anclas de la tela a la medida del arco: 1 sí · 0 no", tipo = "num", paso = 1, lo = 0, hi = 1, def = 1, modo = "vivo" },
    { id = "street_jugadores_local", txt = "Jugadores mi equipo (0 = no tocar; EXPERIMENTAL)", tipo = "num", paso = 1, lo = 0, hi = 11, def = 0, modo = "vivo" },
    { id = "street_jugadores_rival", txt = "Jugadores rival (0 = no tocar; EXPERIMENTAL)", tipo = "num", paso = 1, lo = 0, hi = 11, def = 0, modo = "vivo" },
    { id = "street_jugadores_lado", txt = "Mi equipo es la instancia 0 o 1 (si baja el rival y no el mío, cambia a 1)", tipo = "num", paso = 1, lo = 0, hi = 1, def = 0, modo = "vivo" },
    { id = "tiro_fuerte", txt = "Tiro fuerte", tipo = "bool", modo = "vivo" },
    { id = "tiro_potencia_k", txt = "  Tiro: potencia", tipo = "num", paso = 0.05, lo = 0.8, hi = 1.8, def = 1.25, modo = "vivo" },
    { id = "tiro_altura_k", txt = "  Tiro: altura", tipo = "num", paso = 0.05, lo = 0.3, hi = 1.2, def = 0.7, modo = "vivo" },
    { id = "pase_rapido_preciso", txt = "Pase rápido y preciso", tipo = "bool", modo = "vivo" },
    { id = "pase_vel_k", txt = "  Pase: velocidad", tipo = "num", paso = 0.05, lo = 0.9, hi = 1.6, def = 1.2, modo = "vivo" },
    { id = "pase_correccion", txt = "  Pase: corrección", tipo = "num", paso = 0.1, lo = 0.0, hi = 1.0, def = 0.6, modo = "vivo" },
    { id = "muros_core", txt = "Paredes (rebote)", tipo = "bool", modo = "vivo" },
    { id = "muro_rebote", txt = "  Rebote", tipo = "num", paso = 0.05, lo = 0.4, hi = 0.9, def = 0.75, modo = "vivo" },
    { id = "muro_friccion", txt = "  Fricción", tipo = "num", paso = 0.01, lo = 0.9, hi = 1.0, def = 0.97, modo = "vivo" },
    { id = "red_frena_free", txt = "Freno de red del Core (apagado = red nativa; sin muro de fondo en el arco)", tipo = "bool", modo = "vivo" },
    { id = "red_resorte", txt = "Red del arco", tipo = "bool", modo = "vivo" },
    { id = "ia_cancha_chica", txt = "IA de cancha chica", tipo = "bool", modo = "vivo" },
    { id = "jugadores_formacion", txt = "Formación de saque", tipo = "bool", modo = "vivo" },
    { id = "arquero_pase_corto", txt = "Portero: pase corto", tipo = "bool", modo = "vivo" },
    { id = "arquero_controla", txt = "Arquero controla (no despeja)", tipo = "bool", modo = "vivo" },
    { id = "clima_forzado", txt = "Clima despejado", tipo = "clima", modo = "sig.partida" },
    { id = "street_dos_tiempos", txt = "Dos tiempos", tipo = "bool", modo = "sig.partida" },
    { id = "street_pantalla_descanso", txt = "Pantalla de descanso", tipo = "bool", modo = "sig.partida" },
    { id = "reloj_visual_2t", txt = "Reloj 2.º tiempo 45→90 (visual)", tipo = "bool", modo = "vivo" },
    { id = "final_nativo", txt = "Final nativo (experimental)", tipo = "bool", modo = "vivo" },
    { id = "descanso_fase3", txt = "Reloj 2.º tiempo en fase 3", tipo = "bool", modo = "sig.partida" },
    { id = "descanso_base45", txt = "Reloj 2.º tiempo base 45", tipo = "bool", modo = "sig.partida" },
}
local panelAbierto, panelSel, panelCache, panelCambio, tPanelLeer = true, 1, {}, {}, -100   -- v0.8: el panel es lo primero que se ve en esta página del overlay de Sider (F9 alterna con el marcador)
local function panelValor(it)
    local t = leer(rutaCfg) or ""
    if it.tipo == "bool" then
        if it.def ~= nil and not t:find('"' .. it.id .. '"') then return it.def end   -- 0.4.102: clave ausente = valor por defecto
        return t:find('"' .. it.id .. '"%s*:%s*true') ~= nil
    end
    if it.tipo == "clima" then return t:match('"clima_forzado"%s*:%s*(%d)') ~= nil end
    local v = t:match('"' .. it.id .. '"%s*:%s*([%d%.]+)')
    return tonumber(v) or it.def
end
local function panelEnviar(linea)
    local f = io.open(base .. "phoenix_core\\ordenes.txt", "ab")
    if f then f:write(linea .. "\r\n"); f:close() end
end
local function panelLeer()
    local ahora = os.clock()
    if ahora - tPanelLeer < 1.0 then return end
    tPanelLeer = ahora
    for _, it in ipairs(PANEL) do
        if not panelCambio[it.id] or ahora - panelCambio[it.id] > 4.0 then panelCache[it.id] = panelValor(it) end
    end
end
local function panelTexto()
    panelLeer()
    local s = { "PHOENIX STREET   F9 marcador/panel · 8 ↑ · 9 ↓ · 0 cambiar · RePag baja / AvPag sube (o Izq/Der) · F10/F11 también valen" }
    for i, it in ipairs(PANEL) do
        local v = panelCache[it.id]; local est
        if it.tipo == "num" then est = string.format("%.2f", tonumber(v) or it.def) else est = v and "ON" or "OFF" end
        s[#s + 1] = string.format("%s %s: %s%s", i == panelSel and ">" or " ", it.txt, est, it.modo == "sig.partida" and "  (sig. partida)" or it.modo == "fuera" and "  (se aplica al salir del Entrenamiento)" or "")
    end
    return table.concat(s, "\n")
end
-- 1-nov (0.4.105): teclas numéricas. 8 = opción anterior · 9 = siguiente · 0 = cambiar valor (bool: enciende/apaga; num: sube un paso y vuelve al mínimo al pasar el máximo).
-- Valen la fila superior (0x38/0x39/0x30) y el teclado numérico (0x68/0x69/0x60), y SOLO si el overlay de Sider está mostrando esta página (overlay_on se llamó hace <0,6 s).
-- Choques revisados: attack_mentality.lua usa 8/9 pero NO está en sider.ini; trophy.lua usa el 0 de la fila superior para recargar su map.txt (inofensivo; el teclado numérico no choca).
local tOverlayVisto = -100
local function panelCambiarValor(it)
    if not it then return end
    panelLeer()
    if it.tipo == "num" then
        local v = (tonumber(panelCache[it.id]) or it.def) + it.paso
        if v > it.hi then v = it.lo end
        v = math.floor(v * 100 + 0.5) / 100
        panelCache[it.id] = v; panelCambio[it.id] = os.clock(); panelEnviar(string.format("%s=%.2f", it.id, v))
    else
        local nuevo = not panelCache[it.id]
        panelCache[it.id] = nuevo; panelCambio[it.id] = os.clock()
        if it.tipo == "clima" then panelEnviar("clima_forzado=" .. (nuevo and "0" or "-1")) else panelEnviar(it.id .. "=" .. (nuevo and "1" or "0")) end
    end
end
function m.key_down(ctx, vkey)
    if vkey == 0x78 then panelAbierto = not panelAbierto; return end              -- F9
    if not panelAbierto then return end
    local it = PANEL[panelSel]
    if (vkey == 0x38 or vkey == 0x68 or vkey == 0x39 or vkey == 0x69 or vkey == 0x30 or vkey == 0x60) then
        if os.clock() - tOverlayVisto > 0.6 then return end                         -- solo con el overlay abierto en esta página
        if vkey == 0x38 or vkey == 0x68 then panelSel = (panelSel - 2) % #PANEL + 1       -- 8: anterior
        elseif vkey == 0x39 or vkey == 0x69 then panelSel = panelSel % #PANEL + 1         -- 9: siguiente
        else panelCambiarValor(it) end                                                    -- 0: cambiar
        return
    end
    if vkey == 0x79 then panelSel = panelSel % #PANEL + 1                          -- F10
    elseif vkey == 0x7A and it and it.tipo ~= "num" then                            -- F11
        panelLeer(); local nuevo = not panelCache[it.id]
        panelCache[it.id] = nuevo; panelCambio[it.id] = os.clock()
        if it.tipo == "clima" then panelEnviar("clima_forzado=" .. (nuevo and "0" or "-1")) else panelEnviar(it.id .. "=" .. (nuevo and "1" or "0")) end
    elseif (vkey == 0x21 or vkey == 0x22) and it and it.tipo ~= "num" then         -- 1-nov: RePag = OFF · AvPag = ON
        local nuevo = (vkey == 0x22)
        panelCache[it.id] = nuevo; panelCambio[it.id] = os.clock()
        if it.tipo == "clima" then panelEnviar("clima_forzado=" .. (nuevo and "0" or "-1")) else panelEnviar(it.id .. "=" .. (nuevo and "1" or "0")) end
    elseif (vkey == 0x25 or vkey == 0x27 or vkey == 0x21 or vkey == 0x22) and it and it.tipo == "num" then   -- ← → · 1-nov: RePag resta · AvPag suma
        local sube = (vkey == 0x27 or vkey == 0x22)
        panelLeer(); local v = (tonumber(panelCache[it.id]) or it.def) + (sube and it.paso or -it.paso)
        if v < it.lo then v = it.lo elseif v > it.hi then v = it.hi end
        v = math.floor(v * 100 + 0.5) / 100
        panelCache[it.id] = v; panelCambio[it.id] = os.clock(); panelEnviar(string.format("%s=%.2f", it.id, v))
    end
end

function m.overlay_on(ctx)
    tOverlayVisto = os.clock()
    if panelAbierto then return panelTexto() end
    if not encendido then return "MARCADOR STREET · apagado (phoenix_core.json → marcador_free)" end
    if not enZona then return "MARCADOR STREET · solo en Entrenamiento libre" end
    -- v0.7: aviso de medio tiempo / final que escribe el Core (street_dos_tiempos) en phoenix_core\street_estado.json
    local aviso = ""
    local e = leer(base .. "phoenix_core\\street_estado.json")
    if e then
        if e:find('"final"') then aviso = "*** FINAL *** " elseif e:find('"medio_tiempo"') then aviso = "** MEDIO TIEMPO ** " end
    end
    return aviso .. texto()
end

function m.init(ctx)
    base = ctx.sider_dir or ".\\"
    rutaCfg = base .. "phoenix_core\\phoenix_core.json"
    rutaSonda = base .. "content\\phoenix\\sonda.json"
    rutaPantalla = base .. "content\\phoenix\\pantalla.json"
    rutaSalida = base .. "content\\phoenix\\marcador_free.json"
    rutaSonda2 = base .. "phoenix_core\\sonda_partido.txt"
    local ss = base .. "content/stadium-server/España/Spotify Camp Nou/Asset/model/bg/st002/pitch/#Win/pitch_st002.fpk"
    if leer(ss .. ".respaldo-20261026-013452") then
        rutaPitchSS = ss
        rutaPitchOrig = ss .. ".respaldo-20261026-013452"
        rutaPitchChica = base .. "content/phoenix/cancha_visual/Asset/model/bg/st002/pitch/#Win/pitch_st002.fpk"
        if copiarArchivo(rutaPitchOrig, rutaPitchSS) then log("[cancha_visual] césped original asegurado al arrancar") end
    end
    ctx.register("display_frame", m.display_frame)
    ctx.register("goal_scored", m.goal_scored)     -- Sider 7.3.4+ (en 7.3.3 solo deja un WARN)
    ctx.register("set_teams", m.set_teams)
    ctx.register("set_conditions", m.set_conditions)
    ctx.register("set_match_settings", m.set_match_settings)
    ctx.register("context_reset", m.context_reset)
    ctx.register("overlay_on", m.overlay_on)
    ctx.register("key_down", m.key_down)
    rutaPeticiones = base .. "phoenix_core\\peticiones.log"
    rutaCancha = base .. "content/phoenix/cancha/"
    ctx.register("livecpk_get_filepath", m.get_filepath)
    log("[marcador_free] v" .. m.version .. " listo (apagado salvo \"marcador_free\": true en phoenix_core.json)")
end

return m
