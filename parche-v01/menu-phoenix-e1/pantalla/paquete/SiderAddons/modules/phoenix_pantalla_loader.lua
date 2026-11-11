-- =============================================================================
--  Phoenix Pantalla · cargador (módulo de Sider para PES 2021) · v0.1.1 · 10-nov-2026
--  Carga SiderAddons\phoenix_pantalla\phoenix_pantalla.dll con LoadLibraryA y llama a phx_pantalla_iniciar(carpeta) UNA vez.
--  La DLL engancha Present por su cuenta; no hace falta tick por fotograma. No depende de Phoenix Core.
--  Apagar: quitar (o comentar con ;) la línea lua.module de este módulo en sider.ini, o crear el archivo vacío
--  SiderAddons\phoenix_pantalla\PHOENIX_PANTALLA_OFF. Lo que pasa se anota en phoenix_pantalla\cargador.txt y phoenix_pantalla.log.
-- =============================================================================
local m = { version = "0.1.1" }
local carpeta

local function anotar(txt)
    local f = carpeta and io.open(carpeta .. "cargador.txt", "ab")
    if f then f:write(os.date("%Y-%m-%d %H:%M:%S") .. "  " .. txt .. "\r\n"); f:close() end
end

local function cargar()
    ffi.cdef[[
        void* phxpant_LoadLibraryA(const char* nombre) __asm__("LoadLibraryA");
        void* phxpant_GetProcAddress(void* modulo, const char* nombre) __asm__("GetProcAddress");
    ]]
    local dll = ffi.C.phxpant_LoadLibraryA(carpeta .. "phoenix_pantalla.dll")
    if dll == nil then anotar("sin phoenix_pantalla.dll (o no carga): pantalla apagada"); return end
    local pi = ffi.C.phxpant_GetProcAddress(dll, "phx_pantalla_iniciar")
    local pv = ffi.C.phxpant_GetProcAddress(dll, "phx_pantalla_version")
    if pi == nil or pv == nil then anotar("la DLL no tiene la API esperada: no se usa"); return end
    local iniciar = ffi.cast("int (*)(const char*)", pi)
    local version = ffi.cast("const char* (*)(void)", pv)
    local r = iniciar(carpeta)
    local modos = { [1] = "activa", [0] = "apagada (PHOENIX_PANTALLA_OFF)", [-1] = "error" }
    anotar(string.format("Pantalla %s iniciada: %s (cargador %s)", ffi.string(version()), modos[r] or tostring(r), m.version))
end

function m.init(ctx)
    carpeta = (ctx.sider_dir or ".\\") .. "phoenix_pantalla\\"
    -- Sider no ofrece pcall en este entorno: si algo falla, Sider desactiva solo este modulo (y lo anota en sider.log).
    cargar()
end

return m
