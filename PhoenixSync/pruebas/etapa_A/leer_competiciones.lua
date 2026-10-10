-- leer_competiciones.lua  (Etapa A · SOLO LECTURA)
-- Proyecto Phoenix · FRALEX · 10 oct 2026
-- Qué hace: escribe en sider.log (no cambia NADA del juego):
--   1) comprueba que los bytes del .exe en memoria coinciden con lo visto en el archivo
--      (la función que fija el número de equipos de los torneos de selecciones);
--   2) lee la tabla de competiciones (registros de 264 bytes, la misma que usa Competition-Server)
--      y anota cuántos registros hay y qué tipo tiene cada uno;
--   3) anota el tournament_id de cada partido que ve (una vez por valor).
-- Tecla 4 (con este módulo elegido en el overlay): repetir la lectura (útil dentro del menú de Liga Máster).
-- NO usa memory.write. Todo va dentro de pcall: si algo falla, lo anota y sigue.

local m = { version = "A1.0 (solo lectura)" }

local TAG = "[leer_competiciones] "
local vistos = {}
local nvistos = 0
local estado = "sin leer"
local lecturas = 0

local function L(s) pcall(log, TAG .. s) end

local function hexs(s)
  local t = {}
  for i = 1, #s do t[#t + 1] = string.format("%02X", s:byte(i)) end
  return table.concat(t, " ")
end

local function rd(addr, n)
  local ok, v = pcall(memory.read, addr, n)
  if ok and v and #v == n then return v end
  return nil
end

local function u32(s, off)
  return s:byte(off + 1) + s:byte(off + 2) * 256 + s:byte(off + 3) * 65536 + s:byte(off + 4) * 16777216
end

-- 1) Función del .exe: cadena de comparaciones que escribe el número de equipos
local function leer_funcion()
  -- direcciones y bytes esperados (visto en PES2021.exe, 458.806.784 B)
  local casos = {
    { 0x1412F0635, "C6 83 B0 7A 78 01 20", "ID 37 FAKE_KONAMI -> 32 equipos" },
    { 0x1412F0643, "C6 83 B0 7A 78 01 18", "ID 33 EURO -> 24 equipos" },
    { 0x1412F0651, "C6 83 B0 7A 78 01 20", "ID 27 MUNDIAL -> 32 equipos" },
    { 0x1412F065F, "C6 83 B0 7A 78 01 10", "ID 35 COPA ASIA -> 16 equipos" },
    { 0x1412F066D, "C6 83 B0 7A 78 01 0C", "ID 34 COPA AMERICA -> 12 equipos" },
    { 0x1412F067B, "C6 83 B0 7A 78 01 0E", "ID 36 COPA AFRICA -> 14 equipos" },
    { 0x1412F0689, "C6 83 B0 7A 78 01 26", "ID 5 -> 38 equipos" },
  }
  local bien, mal = 0, 0
  for _, c in ipairs(casos) do
    local esperado = c[2]:gsub("%s+", "")
    local real = rd(c[1], 7)
    if real then
      local h = hexs(real):gsub("%s+", "")
      if h == esperado then
        bien = bien + 1
        L(string.format("OK  0x%X  %s", c[1], c[3]))
      else
        mal = mal + 1
        L(string.format("NO COINCIDE 0x%X esperado=%s real=%s  (%s)", c[1], esperado, h, c[3]))
      end
    else
      mal = mal + 1
      L(string.format("NO SE PUDO LEER 0x%X (%s)", c[1], c[3]))
    end
  end
  L(string.format("Funcion de equipos: %d coinciden, %d no", bien, mal))
  return bien, mal
end

-- 2) Tabla de registros de 264 bytes (firma que usa Competition-Server)
local FIRMA = "\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x4F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

local function leer_tabla()
  local ok, a = pcall(memory.search_process, FIRMA)
  if not ok or not a then
    L("No se encontro la firma de la tabla de competiciones (" .. tostring(a) .. ")")
    return 0
  end
  local base = a + 8
  L(string.format("Tabla de competiciones: firma en 0x%X, registro 0 en 0x%X", a, base))
  local total, ultimo = 0, 0
  local vacios = 0
  for k = 0, 249 do
    local r = rd(base + k * 264, 264)
    if not r then
      L(string.format("registro %d: no se pudo leer; paro", k))
      break
    end
    local id, orden = u32(r, 0), u32(r, 4)
    if id == 0 and orden == 0 then
      vacios = vacios + 1
      if vacios >= 3 then break end
    else
      vacios = 0
      total = total + 1
      ultimo = k
      L(string.format("reg %3d | offset %5d | ID=%-6d orden=%-4d | tipo1=%02X tipo2=%02X tipo3=%02X | suben=%d bajan=%d",
        k, k * 264, id, orden, r:byte(25), r:byte(26), r:byte(33), u32(r, 48), u32(r, 52)))
    end
  end
  L(string.format("RESUMEN tabla: %d registros con datos, el ultimo es el numero %d (offset %d)", total, ultimo, ultimo * 264))
  return total
end

local function leer_todo(ctx)
  lecturas = lecturas + 1
  L(string.format("=== lectura %d ===", lecturas))
  local ok1, e1 = pcall(leer_funcion)
  if not ok1 then L("error en funcion: " .. tostring(e1)) end
  local ok2, e2 = pcall(leer_tabla)
  if not ok2 then L("error en tabla: " .. tostring(e2)) end
  estado = string.format("lectura %d hecha (mira sider.log)", lecturas)
end

local function ver_partido(ctx)
  local ok, t = pcall(function() return ctx.tournament_id end)
  if ok and t and not vistos[t] and nvistos < 300 then
    vistos[t] = true
    nvistos = nvistos + 1
    L(string.format("tournament_id visto: %s  (grupo=%d etapa=%d)", tostring(t), math.floor(t / 1024), t % 1024))
  end
end

function m.set_teams(ctx, home, away)
  ver_partido(ctx)
end

function m.key_down(ctx, vkey)
  if vkey == 0x34 then -- tecla 4
    leer_todo(ctx)
  end
end

function m.overlay_on(ctx)
  return string.format("leer_competiciones %s\nEstado: %s\nTecla 4 = leer otra vez (escribe en sider.log)\nIDs de torneo vistos: %d",
    m.version, estado, nvistos)
end

function m.init(ctx)
  L("Modulo cargado " .. m.version .. " - SOLO LECTURA")
  ctx.register("set_teams", m.set_teams)
  ctx.register("key_down", m.key_down)
  ctx.register("overlay_on", m.overlay_on)
  leer_todo(ctx)
end

return m
