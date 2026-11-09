-- =============================================================================
--  Phoenix Core · cargador (módulo de Sider para PES 2021) · v0.1.0 · 26-oct-2026
--  Carga SiderAddons\phoenix_core\phoenix_core.dll con LoadLibraryA, llama a phx_core_iniciar(carpeta) una vez y a
--  phx_core_tick() en cada fotograma. Todo lo demás (funciones on/off, modo seguro, registro) vive en la DLL.
--  Si falta la DLL o no tiene la API esperada, no hace nada y lo apunta en phoenix_core\cargador.txt.
--  Apagar sin desinstalar: crear el archivo SiderAddons\phoenix_core\PHOENIX_CORE_OFF (vacío).
-- =============================================================================
local m = { version = "0.1.0" }
local API_ESPERADA = 1
local carpeta, tick

local function anotar(txt)
    local f = carpeta and io.open(carpeta .. "cargador.txt", "ab")
    if f then f:write(os.date("%Y-%m-%d %H:%M:%S") .. "  " .. txt .. "\r\n"); f:close() end
end

local function cargar()
    ffi.cdef[[
        void* phxcore_LoadLibraryA(const char* nombre) __asm__("LoadLibraryA");
        void* phxcore_GetProcAddress(void* modulo, const char* nombre) __asm__("GetProcAddress");
    ]]
    local dll = ffi.C.phxcore_LoadLibraryA(carpeta .. "phoenix_core.dll")
    if dll == nil then anotar("sin phoenix_core.dll (o no carga): Core apagado"); return end
    local function f(nombre, firma)
        local p = ffi.C.phxcore_GetProcAddress(dll, nombre)
        if p == nil then return nil end
        return ffi.cast(firma, p)
    end
    local version = f("phx_core_version", "const char* (*)(void)")
    local api     = f("phx_core_api_version", "int (*)(void)")
    local iniciar = f("phx_core_iniciar", "int (*)(const char*)")
    tick          = f("phx_core_tick", "void (*)(void)")
    if not (version and api and iniciar and tick) then anotar("la DLL no tiene la API de Phoenix Core: no se usa"); tick = nil; return end
    if api() ~= API_ESPERADA then anotar(string.format("API %d distinta de la esperada %d: no se usa", api(), API_ESPERADA)); tick = nil; return end
    local r = iniciar(carpeta)
    local modos = { [1] = "normal", [2] = "MODO SEGURO", [0] = "apagado (PHOENIX_CORE_OFF)", [-1] = "error" }
    anotar(string.format("Core %s iniciado: %s (cargador %s)", ffi.string(version()), modos[r] or tostring(r), m.version))
    if r <= 0 then tick = nil end
end

function m.display_frame(ctx)
    if tick then tick() end
end

function m.init(ctx)
    carpeta = (ctx.sider_dir or ".\\") .. "phoenix_core\\"
    cargar()
    ctx.register("display_frame", m.display_frame)
end

return m
