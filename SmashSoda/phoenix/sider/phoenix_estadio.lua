-- =============================================================================
--  Phoenix Evolution · PHOENIX ESTADIO (módulo de Sider para PES 2021)
--  Une el partido con Phoenix Link (Parsec, PES corre solo en la PC del anfitrión):
--    · Juego → Link: content\phoenix\estado.json  (cada ~1 s y SOLO durante el partido)
--                    content\phoenix\resultado.json (una vez, al terminar un partido completo)
--      Link lo usa para: bloquear Start/Back/Guía a los invitados mientras se juega, arrancar y
--      cerrar el marcador solo, anunciar cada gol con su minuto en el chat y pausar si alguien se cae.
--    · Link → juego: content\phoenix\sala.txt (lo escribe Link; aquí SOLO se lee)
--    · Overlay (Espacio, y con 1 / º hasta «PHOENIX ESTADIO»): marcador con nombres, minuto,
--      goles con minuto, quién juega en cada lado con su ping, el árbitro y el chat de la sala.
--
--  Seguridad (ver PhoenixSync/sider/RIESGOS-SIDER.md):
--    · SOLO lectura del juego: match.stats() y ctx. No escribe memoria, no usa ffi, no tiene teclas,
--      no cambia nada del partido (no puede desincronizar).
--    · Fuera del partido solo llama a match.stats() 2 veces por segundo (coste ~0).
--    · El Lua de Sider 7.3.3 NO trae pcall ni os.rename: los errores se cuentan con una «bandera»
--      y tras 5 seguidos el módulo se apaga solo hasta reiniciar el juego (queda en sider.log).
--      Sin os.rename no hay escritura atómica: Link descarta una lectura a medias y se queda con la anterior.
--    · Necesita match-stats.enabled = 1 en sider.ini (ConmeGOL ya lo trae). Si no, no escribe nada.
--
--  Instalar: copiar a SiderAddons\modules\ y añadir al final de la lista de módulos de sider.ini:
--      lua.module = "phoenix_estadio.lua"
--  (en la carpeta del modo de ConmeGOL, que es la que copia el switcher). Desinstalar = quitar la línea.
-- =============================================================================

local m = { version = "1.0" }

local MAX_FALLOS   = 5
local CADA_SEG     = 0.5    -- cada cuánto se mira match.stats()
local ESCRIBIR_SEG = 1.0    -- estado.json como mucho 1 vez por segundo (y al momento en un gol o un cambio de fase)
local SALA_SEG     = 1.0    -- sala.txt se relee como mucho 1 vez por segundo (solo con el overlay abierto)
local MAX_SALA     = 4096

local carpeta = nil          -- <SiderAddons>\content\phoenix\
local mapaKits = nil         -- <SiderAddons>\content\kit-server\map.txt
local apagado, fallos = false, 0
local enCursoJuego, enCursoHud = false, false

local nombres = nil          -- id de equipo → nombre (de kit-server\map.txt), se carga una vez
local idLocal, idVisita = 0, 0
local torneo = 0

local p = nil                -- partido en curso (nil = no hay partido)
local ultimoMirar = -1000
local ultimoEscrito = -1000
local seq = 0
local ultimoResultado = ""   -- texto del último partido terminado (para el HUD)
local estadoArchivo = "sin partido"

local sala = nil             -- lo último leído de sala.txt
local salaLeida = -1000

-- ─── utilidades ──────────────────────────────────────────────────────────────

-- Corta una cadena UTF-8 a como mucho n bytes sin dejar una letra a medias.
local function cortarUtf8(s, n)
    if #s <= n then return s end
    local fin = n
    while fin > 0 do
        local b = s:byte(fin + 1)
        if not b or b < 0x80 or b >= 0xC0 then break end
        fin = fin - 1
    end
    return s:sub(1, fin)
end

-- ¿Es UTF-8 válido? (si sala.txt viniera roto, no se muestra)
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

-- Texto seguro para JSON: comillas, barras y caracteres de control escapados.
local function jsonTexto(s)
    s = tostring(s or "")
    s = s:gsub('[%c"\\]', function(c)
        if c == '"' then return '\\"' end
        if c == "\\" then return "\\\\" end
        return string.format("\\u%04x", c:byte())
    end)
    return '"' .. s .. '"'
end

local function escribir(nombre, texto)
    if not carpeta then return false end
    local f = io.open(carpeta .. nombre, "wb")
    if not f then return false end       -- la carpeta no existe o Link lo tiene abierto: se reintenta luego
    f:write(texto)
    f:close()
    return true
end

-- Nombres de equipos: el mismo mapa que usa kit-server (id, "País\Liga\Equipo").
local function cargarNombres()
    if nombres then return end
    nombres = {}
    if not mapaKits then return end
    local f = io.open(mapaKits, "rb")
    if not f then return end
    local texto = f:read(1048576) or ""
    f:close()
    texto = texto:gsub("^\239\187\191", "")
    for linea in texto:gmatch("[^\r\n]+") do
        linea = linea:gsub("#.*", "")
        local id, ruta = linea:match("^%s*(%d+)%s*,%s*\"?([^\"]*)")
        id = tonumber(id)
        if id and ruta then
            local nombre = ruta:match("([^\\]+)%s*$")
            if nombre and nombre ~= "" then nombres[id] = cortarUtf8(nombre, 32) end
        end
    end
end

local function nombreDe(id)
    cargarNombres()
    return (nombres and nombres[id]) or ""
end

-- Tope del minuto de cada periodo (un gol en el añadido de la 1.ª parte cuenta como 45').
local TOPE = { [1] = 45, [2] = 90, [3] = 105, [4] = 120 }

-- ─── juego → Link ────────────────────────────────────────────────────────────

local function jsonPartido(extra)
    local goles = {}
    for i, g in ipairs(p.goles) do
        goles[i] = string.format('{"m":%d,"l":"%s"}', g.m, g.l)
    end
    return string.format(
        '{"v":2,"fase":"%s","minuto":%d,"segundo":%d,"periodo":%d,"anadido":%d,"reloj_corre":%s,'
        .. '"local":%d,"visita":%d,"nombre_local":%s,"nombre_visita":%s,'
        .. '"goles_local":%d,"goles_visita":%d,"pk_local":%d,"pk_visita":%d,"goles":[%s],'
        .. '"completo":%s,"torneo":%d,"seq":%d%s}',
        p.fase, p.minuto, p.segundo, p.periodo, p.anadido, (p.quieto < 1.5) and "true" or "false",
        idLocal, idVisita, jsonTexto(nombreDe(idLocal)), jsonTexto(nombreDe(idVisita)),
        p.gl, p.gv, p.pkl, p.pkv, table.concat(goles, ","),
        p.completo and "true" or "false", torneo, seq, extra or "")
end

local function escribirEstado()
    seq = seq + 1
    if escribir("estado.json", jsonPartido()) then
        ultimoEscrito = os.clock()
        estadoArchivo = "estado.json al día (" .. os.date("%H:%M:%S") .. ")"
    else
        estadoArchivo = "no se pudo escribir estado.json (¿existe content\\phoenix?)"
    end
end

local function empezar()
    p = {
        goles = {}, gl = 0, gv = 0, pkl = 0, pkv = 0,
        periodo = 0, minuto = 0, segundo = 0, anadido = 0,
        reloj = -1, cambio = os.clock(), quieto = 0,
        maxPeriodo = 0, maxMinuto = 0,
        fase = "menu", completo = false,
    }
    log(string.format("[estadio] partido nuevo: %d (%s) vs %d (%s)", idLocal, nombreDe(idLocal), idVisita, nombreDe(idVisita)))
end

local function terminar()
    p.completo = p.maxPeriodo >= 3 or (p.maxPeriodo >= 2 and p.maxMinuto >= 89)
    p.fase = p.completo and "final" or "menu"
    p.quieto = 99
    escribirEstado()
    local marcador = string.format("%s %d-%d %s", nombreDe(idLocal), p.gl, p.gv, nombreDe(idVisita))
    if p.pkl > 0 or p.pkv > 0 then marcador = marcador .. string.format(" (penales %d-%d)", p.pkl, p.pkv) end
    if p.completo then
        escribir("resultado.json", jsonPartido(string.format(',"fin":%d', os.time())))
        ultimoResultado = "Último partido: " .. marcador
    else
        ultimoResultado = "Último partido (a medias): " .. marcador
    end
    log("[estadio] fin del partido: " .. marcador .. (p.completo and "" or " · abandonado"))
    p = nil
    estadoArchivo = "sin partido"
end

local function gol(lado)
    local tope = TOPE[p.periodo] or 999
    local min = p.minuto + 1
    if min > tope then min = tope end
    p.goles[#p.goles + 1] = { m = min, l = lado }
end

local function quitarGol(lado)
    for i = #p.goles, 1, -1 do
        if p.goles[i].l == lado then table.remove(p.goles, i) return end
    end
end

local function mirar()
    local st = match.stats()
    if not st then
        if p then terminar() end
        return
    end
    local t = os.clock()
    local per = tonumber(st.period) or 0
    local mi = tonumber(st.clock_minutes) or 0
    local se = tonumber(st.clock_seconds) or 0
    local reloj = mi * 60 + se
    -- ¿Empezó otro partido sin salir (revancha)? El reloj vuelve muy atrás tras haber jugado la 2.ª parte.
    if p and p.maxPeriodo >= 2 and per >= 1 and per <= 2 and reloj + 600 < p.reloj then terminar() end
    if not p then empezar() end

    if reloj ~= p.reloj or per ~= p.periodo then p.reloj = reloj; p.cambio = t end
    p.quieto = t - p.cambio
    p.periodo, p.minuto, p.segundo = per, mi, se
    p.anadido = tonumber(st.added_minutes) or 0
    p.pkl = tonumber(st.pk_home_score) or 0
    p.pkv = tonumber(st.pk_away_score) or 0
    if per > p.maxPeriodo then p.maxPeriodo = per end
    if per >= 1 and per <= 4 and mi > p.maxMinuto then p.maxMinuto = mi end

    -- Goles con su minuto (match.stats() no da goleadores: solo el marcador)
    local hl = tonumber(st.home_score) or 0
    local al = tonumber(st.away_score) or 0
    local hubo = false
    while hl > p.gl and #p.goles < 60 do p.gl = p.gl + 1; gol("l"); hubo = true end
    while al > p.gv and #p.goles < 60 do p.gv = p.gv + 1; gol("v"); hubo = true end
    while hl < p.gl do p.gl = p.gl - 1; quitarGol("l"); hubo = true end
    while al < p.gv do p.gv = p.gv - 1; quitarGol("v"); hubo = true end

    -- Fase. match.stats() no dice si el juego está en pausa: se deduce del reloj.
    --   reloj quieto en el minuto 45 / 105 → descanso; quieto ≥ 45 s → pausado (las repeticiones duran menos);
    --   periodo 0 = previa del partido → «menu» (Link no bloquea ni arranca nada).
    local fase = "en_juego"
    if per == 0 then fase = "menu"
    elseif per ~= 5 and p.quieto >= 3 and ((per == 1 and mi >= 45) or (per == 3 and mi >= 105)) then fase = "descanso"
    elseif p.quieto >= 45 then fase = "pausado" end
    local cambioFase = fase ~= p.fase
    p.fase = fase

    if hubo or cambioFase or t - ultimoEscrito >= ESCRIBIR_SEG then escribirEstado() end
end

function m.display_frame(ctx)
    if apagado then return end
    local t = os.clock()
    if t - ultimoMirar < CADA_SEG then return end
    ultimoMirar = t
    if enCursoJuego then
        fallos = fallos + 1
        log("[estadio] la vez anterior hubo un error (" .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[estadio] demasiados errores seguidos: el módulo se detiene hasta reiniciar el juego")
            return
        end
    end
    enCursoJuego = true
    torneo = tonumber(ctx.tournament_id) or torneo
    mirar()
    enCursoJuego = false
    fallos = 0
end

function m.set_teams(ctx, home, away)
    idLocal = tonumber(home) or tonumber(ctx.home_team) or 0
    idVisita = tonumber(away) or tonumber(ctx.away_team) or 0
end

-- ─── Link → juego (sala.txt) ─────────────────────────────────────────────────

local function leerSala()
    local t = os.clock()
    if t - salaLeida < SALA_SEG then return end
    salaLeida = t
    sala = nil
    if not carpeta then return end
    local f = io.open(carpeta .. "sala.txt", "rb")
    if not f then return end
    local s = f:read(MAX_SALA) or ""
    f:close()
    if s == "" or not esUtf8(s) then return end
    local d = { ja = {}, jb = {}, chat = {} }
    for linea in s:gmatch("[^\r\n]+") do
        local k, v = linea:match("^(%w+)=(.*)$")
        if k == "ja" or k == "jb" then
            local nombre, ping, mando = v:match("^(.-)|(%-?%d+)|(%d+)$")
            if nombre and #d[k] < 8 then d[k][#d[k] + 1] = { nombre = cortarUtf8(nombre, 20), ping = tonumber(ping) or -1, mando = tonumber(mando) or 0 } end
        elseif k == "chat" then
            if #d.chat < 4 then d.chat[#d.chat + 1] = cortarUtf8(v, 90) end
        elseif k then
            d[k] = cortarUtf8(v, 120)
        end
    end
    d.edad = os.time() - (tonumber(d.t) or 0)
    sala = d
end

local function textoPing(ms)
    if ms < 0 then return "  -- ms" end
    local nota = ms <= 60 and "bien" or (ms <= 100 and "justo" or "ALTO")
    return string.format("%4d ms %s", ms, nota)
end

-- Columna de ancho fijo (Consolas): se cuenta por letras, no por bytes, para que las tildes no la descuadren.
local function columna(s, ancho)
    local letras = 0
    for _ in s:gmatch("[^\128-\191]") do letras = letras + 1 end
    if letras >= ancho then return s end
    return s .. string.rep(" ", ancho - letras)
end

-- ─── overlay (HUD) ───────────────────────────────────────────────────────────

local NOMBRE_FASE = { menu = "previa", en_juego = "en juego", pausado = "en pausa", descanso = "descanso", final = "final" }
local NOMBRE_PERIODO = { [1] = "1.er tiempo", [2] = "2.º tiempo", [3] = "prórroga 1", [4] = "prórroga 2", [5] = "penales" }

local function textoHud()
    leerSala()
    local L = {}
    local vivo = sala and sala.edad <= 10
    local enlace = vivo and (sala.abierta == "1" and ("sala «" .. (sala.sala or "") .. "»") or "sala cerrada") or "Phoenix Link sin datos"
    L[#L + 1] = string.format("PHOENIX ESTADIO v%s  ·  %s", m.version, enlace)
    L[#L + 1] = ""

    local nl = (vivo and sala["local"]) or ""
    local nv = (vivo and sala.visita) or ""
    if p then
        local eqL = nombreDe(idLocal); if eqL == "" then eqL = "Local" end
        local eqV = nombreDe(idVisita); if eqV == "" then eqV = "Visita" end
        local linea = string.format("  %s  %d - %d  %s     %d'  %s  (%s)", eqL, p.gl, p.gv, eqV,
            p.minuto, NOMBRE_FASE[p.fase] or p.fase, NOMBRE_PERIODO[p.periodo] or "previa")
        if p.periodo == 5 then linea = linea .. string.format("  penales %d-%d", p.pkl, p.pkv) end
        L[#L + 1] = linea
        if nl ~= "" or nv ~= "" then L[#L + 1] = "  Phoenix: " .. nl .. " (local) vs " .. nv .. " (visita)" end
        if #p.goles > 0 then
            local g = {}
            for i, x in ipairs(p.goles) do
                if i > 10 then g[#g + 1] = "…" break end
                g[#g + 1] = x.m .. "' " .. (x.l == "l" and eqL or eqV)
            end
            L[#L + 1] = "  Goles: " .. table.concat(g, " · ")
        end
    else
        L[#L + 1] = "  Sin partido en curso. " .. (ultimoResultado ~= "" and ultimoResultado or "Al empezar uno, el marcador sale aquí.")
    end

    if vivo and sala.abierta == "1" then
        L[#L + 1] = ""
        L[#L + 1] = "  " .. columna("LOCAL" .. (nl ~= "" and (" · " .. nl) or ""), 36) .. "VISITA" .. (nv ~= "" and (" · " .. nv) or "")
        local n = math.max(#sala.ja, #sala.jb)
        if n == 0 then L[#L + 1] = "  (nadie con mando todavía)" end
        for i = 1, n do
            local a, b = sala.ja[i], sala.jb[i]
            local ca = a and string.format("%d. %s %s", a.mando, columna(a.nombre, 14), textoPing(a.ping)) or ""
            local cb = b and string.format("%d. %s %s", b.mando, columna(b.nombre, 14), textoPing(b.ping)) or ""
            L[#L + 1] = "  " .. columna(ca, 36) .. cb
        end
        if sala.arbitro and sala.arbitro ~= "" then L[#L + 1] = "  Árbitro: " .. sala.arbitro end
        if #sala.chat > 0 then
            L[#L + 1] = ""
            L[#L + 1] = "  Chat de la sala:"
            for _, c in ipairs(sala.chat) do L[#L + 1] = "   " .. c end
        end
    end
    L[#L + 1] = ""
    L[#L + 1] = "  [" .. estadoArchivo .. "]"
    return table.concat(L, "\n")
end

function m.overlay_on(ctx)
    if apagado then return "PHOENIX ESTADIO · módulo detenido por seguridad (ver sider.log)" end
    if enCursoHud then
        fallos = fallos + 1
        log("[estadio] el HUD falló la vez anterior (" .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then
            apagado = true
            log("[estadio] demasiados errores seguidos: el módulo se detiene hasta reiniciar el juego")
            return "PHOENIX ESTADIO · módulo detenido por seguridad (ver sider.log)"
        end
    end
    enCursoHud = true
    local texto = textoHud()
    enCursoHud = false
    return texto
end

function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    carpeta = base .. "content\\phoenix\\"
    mapaKits = base .. "content\\kit-server\\map.txt"
    ctx.register("set_teams", m.set_teams)
    ctx.register("display_frame", m.display_frame)
    ctx.register("overlay_on", m.overlay_on)
    log("[estadio] v" .. m.version .. " listo. Escribe estado.json/resultado.json y lee sala.txt en " .. carpeta)
end

return m
