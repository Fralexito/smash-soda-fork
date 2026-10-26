-- =============================================================================
--  Phoenix Evolution · PRUEBA «MÁS DE 8 MANDOS» — FASE 1: UN 9.º MANDO (módulo de Sider para PES 2021)
--  v0.9 · 20-oct-2026 la Phoenix es un ESPEJO de la pantalla de PES, que sigue debajo y recibe los mandos 1–8 (ya no
--        se bloquea la entrada): lados y nombres («Datos personales») de 1–8 leídos de la pantalla de PES en vivo; los
--        mandos 9–16 se mueven con SU palanca/cruceta (estado DirectInput leído de HidMan); «Confirmar» de PES cierra
--        las dos. Detalles en cap_60_k (20-oct ~16:00).
--  v0.6 · 20-oct-2026 la pantalla pasa a ser una RÉPLICA EXACTA de la de PES (píxeles reales recortados de una
--        captura, a pantalla completa 1920x1080; filas solo para mandos conectados; más de 8 = la tabla se desplaza).
--  v0.5 · 20-oct-2026 PANTALLA PHOENIX «ELEGIR LADO» dentro de la ventana de Sider (imagen dibujada por el
--        módulo: 3 zonas, una fila por mando con su color; ↑↓ elegir, ←→ lado, ENTER/A confirmar); TODOS los
--        mandos extra (no solo uno) → mandos lógicos 8–15; parches de código aceptan índices < 16.
--  v0.4 · 20-oct-2026 (flechas de colores nuevos para los mandos 9+; ajustes solo durante el partido y restaurados
--        al salir o al deshacer, para evitar el bucle de la pantalla de lados; K = escribir ajustes ya; contador
--        de cuántas veces el juego pide el color de la flecha 9+)
--  v0.3 · 20-oct-2026 (lado en ajustes: 0 nadie, 1 Local, 2 Visitante; tolera código ya parchado) · antes v0.2 (escribe la configuración del partido en vez de la ranura; recorte 7→23) · SOLO PARA PROBAR EN LA PC DE FRALEX, EN UN AMISTOSO OFFLINE.
--  Base: libro personal cap_60_j / cap_60_k / cap_62 (exe ConmeGOL / PATCH10100).
--
--  Qué hace (SOLO cuando pulsas I en el MENÚ; nada antes):
--   1. Busca entre los dispositivos DirectInput del juego (árbol de 0x1437F0AF8+0x90) el mando que
--      NO está en las 8 casillas de settings.dat (tu mando físico) y lo da de alta como mando lógico 8
--      llamando a los mismos métodos de HidMan que usa el juego para los 8 primeros:
--      +0xE8 (botones, copiados del mando lógico 1), +0x120 (activar), +0x118 y +0x110 (su GUID).
--   2. Cambia dos sitios del código para que el partido lea la entrada nº 9:
--      · FUN_140A64810 (64 bytes, en su sitio): acepta índices < 9; para el 8 usa botones por defecto.
--      · 0x140A648AF: «cmp edx, 8» → «cmp edx, 9» dentro de FUN_140A64850.
--      Antes de escribir COMPRUEBA los bytes originales; si no coinciden, no toca nada.
--   3. Ya en el partido, si la ranura 8 está vacía, la llena copiando una ranura que juegue en LOCAL
--      (lado 0) con: nº 8, mando 8, otro jugador del mismo equipo.
--   4. (v0.4) Colores de flecha: el juego pide el color de cada flecha a FUN_1420BD9C0(salida, nº mando, variante),
--      que solo tiene 6 colores (mandos 0–5) y da gris al resto. Se desvía su entrada a un código propio que,
--      para los mandos 8–23, devuelve un color de COLORES_NUEVOS; para 0–7 sigue el código original intacto.
--  Pulsar I otra vez = deshacer los cambios de código (restaura los bytes originales y los ajustes).
--  J = marca en el registro. K = escribir ya los ajustes del 9.º mando (si el automático no lo hizo). Todo vuelve a la normalidad al cerrar PES (no toca archivos del juego).
--
--  Instalar (con PES CERRADO): copiar a SiderAddons\modules\ y en sider.ini, en lugar de la sonda:
--      lua.module = "phoenix_mandos_fase1.lua"
--  Requiere luajit.ext.enabled = 1. Escribe su registro en content\phoenix\mandos\fase1.txt.
-- =============================================================================

--  v0.9.36 · 23-oct-2026 PASO 2 de cap_60_n: GESTOR DE FOCO de los menús (WindowManager, [0x1436F9C10]) a 16 mandos
--        (44 parches de diseno_24 «## WM» + FUN_140E718A0 reescrita en página propia). Si en m.init el gestor aún no
--        existe se parchea directamente; si ya existe, se MIGRA a un objeto nuevo de 0x288 en el primer cuadro en menú.
--        Además, los 6 bucles «primer mando activo» (§2.5) suben de 8 a 16 (tabla PARCHES).
--  v0.9.38 · 23-oct-2026 ○ de los mandos 9–16 en la pantalla de lados de PES (pausa y menú): PES solo oye el ○ en sus 8
--        FILAS (SideSelectEach, una por mando 0–7). Al ver ○ de un mando 9+ se llama al método «atrás» de una fila
--        (FUN_140C68350, el mismo que llama PES) sobre el objeto SideSelect guardado. Sin teclado. Ver V9.atrasExtras.
local m = { version = "0.9.60" }

local G_HIDMAN, G_GESTOR, G_DI8 = 0x1437F0A58, 0x1436F3DC8, 0x1437F0AF8
local CAPS = 0x14351A840
local N_RANURAS, TAM_RANURA = 24, 0x74
local NUEVO = 8                                   -- índice del primer mando nuevo (el 9.º)
local MAX_MANDOS = 16                             -- tope: ViGEm admite 16 mandos a la vez
local VK_I, VK_J, VK_K = 0x49, 0x4A, 0x4B
local MAX_FALLOS = 5

-- Parches de código: dirección, bytes originales, bytes nuevos (hex)
local PARCHES = {
    { dir = 0x140A64810, nombre = "FUN_140A64810 (llenar entradas del partido)",
      orig  = "4d8bd84c8bd183fa08732f448bca4180bc09e0bb00000074214a8d04cdf45d00004903c14c8d04414969c9d40700004d8bcb4903cae906000000c3cccccccccc",
      -- v0.8.9: del 8 en adelante usa la CONFIGURACIÓN DEL MANDO 1 (+0xBBE8) en vez de un puntero NULO: con los extra
      -- ya enlazados (v0.8.8), FUN_140A64850 leía esa configuración nula → cierre de PES (c0000005 en FUN_141F9C3B0,
      -- 20-oct 14:49). 0–7 idéntico al original. Probado ejecutándolo: herramientas\probar_relleno_entradas.ps1.
      nuevo = "4d8bd84c8bd1448bca83fa08731d4180bc09e0bb00000074114a8d04cdf45d00004903c14c8d0441eb08c34c8d81e8bb00004969c9d40700004d8bcb4903ca90" },
    { dir = 0x140A648AF, nombre = "FUN_140A64850 cmp edx,8",     -- el índice solo se usa para pedir el mando lógico a HidMan (32)
      orig  = "83fa08", nuevo = "83fa10" },
    { dir = 0x140A571A3, nombre = "FUN_140A571A0 recorte mando > 7",
      orig  = "83fa08", nuevo = "83fa18" },
    -- v0.9.16 (23-oct, hallado con REA/Frida + Ghidra): match::Command (FUN_1408A9180) recorre las 24 ranuras y SOLO
    -- manda la entrada al jugador si el mando de la ranura (+0x10) es < 8; si no, la ranura pasa a control de la CPU.
    -- Por eso el jugador de un mando 9+ tenía ranura, jugador y entrada pero no se movía. Se sube el tope a 16.
    { dir = 0x1408A94AC, nombre = "FUN_1408A9180 cmp dl,8 (entrada humana solo < 8)",
      orig  = "80fa08", nuevo = "80fa10" },
    { dir = 0x1408A99D0, nombre = "FUN_1408A9180 cmp dil,8 (mando > 7 = CPU)",
      orig  = "4080ff08", nuevo = "4080ff10" },
    -- FUN_140A647C0 (configuración del bloque N, sin tope: con N ≥ 8 leía fuera de las 8 configuraciones): ahora N ≥ 8
    -- usa la del mando 1, igual que el parche de FUN_140A64810. Cabe en los 32 bytes (18 + relleno 0xCC hasta 0x140A647E0).
    { dir = 0x140A647C0, nombre = "FUN_140A647C0 configuración N ≥ 8 = la del mando 1",
      orig  = "8bc2488d04c0488d80f45d0000488d0441c3cccccccccccccccccccccccccccc",
      nuevo = "8bc283f808720231c0488d04c0488d80f45d0000488d0441c3cccccccccccccc" },
    -- v0.9.20 (23-oct, Fralex: «no me deja escoger equipo al mando de visitante, solo el mando 1»): los menús de antes
    -- del partido buscan «el mando de cada lado» recorriendo solo los mandos 0–7 de la lista de lados (S+0x2D); si no
    -- hay ninguno, usan el primer mando conectado (el anfitrión). Se sube el recorrido a 16 (la lista tiene 24).
    { dir = 0x140C9EFE7, nombre = "menu::TeamSelectMatchBase: quién elige el equipo de cada lado (< 8 → < 16)", orig = "83f808", nuevo = "83f810" },
    { dir = 0x140ACB4A6, nombre = "menu::ExhibitionTeamDataLoad: mando de cada lado (< 8 → < 16)", orig = "83f808", nuevo = "83f810" },
    { dir = 0x140A9E4F2, nombre = "menu::ExhibitionUserDataDoalog (Visitante) (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140A9E622, nombre = "menu::ExhibitionUserDataDoalog (Local) (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140AAC202, nombre = "menu::ExhibitionUserDataErrorDialog 1 (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140AAC302, nombre = "menu::ExhibitionUserDataErrorDialog 2 (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140AAC402, nombre = "menu::ExhibitionUserDataErrorDialog 3 (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140AAC502, nombre = "menu::ExhibitionUserDataErrorDialog 4 (< 8 → < 16)", orig = "83fb08", nuevo = "83fb10" },
    -- v0.9.36 (cap_60_n §2.5 / cap_60_m §1.C): «primer mando activo» = bit más bajo de la máscara de HidMan, buscado solo
    -- en 0–7 («ninguno» = 8). Se busca en 0–15 («ninguno» = 16). Sin tabla detrás: solo se compara con el dueño +0x114.
    { dir = 0x140E3BACE, nombre = "primer mando activo FUN_140E3BA30: cmp ecx,8 → 16", orig = "83f908", nuevo = "83f910" },
    { dir = 0x140E3BAD3, nombre = "primer mando activo FUN_140E3BA30: ninguno = 8 → 16", orig = "b908000000", nuevo = "b910000000" },
    { dir = 0x140E3BD59, nombre = "primer mando activo FUN_140E3BD30: cmp r9d,8 → 16", orig = "4183f908", nuevo = "4183f910" },
    { dir = 0x140E3BD5F, nombre = "primer mando activo FUN_140E3BD30: ninguno = 8 → 16", orig = "41b908000000", nuevo = "41b910000000" },
    { dir = 0x140E430A7, nombre = "primer mando activo FUN_140E42F80: cmp ebx,8 → 16", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140E430AC, nombre = "primer mando activo FUN_140E42F80: ninguno = 8 → 16", orig = "bb08000000", nuevo = "bb10000000" },
    { dir = 0x140E43237, nombre = "primer mando activo FUN_140E431F0: cmp ecx,8 → 16", orig = "83f908", nuevo = "83f910" },
    { dir = 0x140E4323C, nombre = "primer mando activo FUN_140E431F0: ninguno = 8 → 16", orig = "b908000000", nuevo = "b910000000" },
    { dir = 0x140E43699, nombre = "primer mando activo FUN_140E43620: cmp ecx,8 → 16", orig = "83f908", nuevo = "83f910" },
    { dir = 0x140E4369E, nombre = "primer mando activo FUN_140E43620: ninguno = 8 → 16", orig = "b908000000", nuevo = "b910000000" },
    { dir = 0x140E43C68, nombre = "primer mando activo FUN_140E43C30: cmp ecx,8 → 16", orig = "83f908", nuevo = "83f910" },
    { dir = 0x140E43C6D, nombre = "primer mando activo FUN_140E43C30: ninguno = 8 → 16", orig = "b908000000", nuevo = "b910000000" },
}
-- v0.9.22 (23-oct, hallado con REA/Frida + Ghidra): el GESTOR DE CURSORES DE MENÚ (singleton [0x1436F9BC8], creado
-- la primera vez que se usa un menú en FUN_140E679E0, ctor FUN_140E67580) guarda el cursor de cada mando en una tabla
-- de 8 entradas de 0x58 bytes (+0x80). Con el mando 16 en la lista del Visitante, PES leía y ESCRIBÍA la entrada 15,
-- fuera del objeto (memoria de otro): la lista no se movía y podía corromper memoria. Se agranda a 16 entradas.
-- Hay que ponerlo ANTES de que exista el gestor (al cargar el módulo); si ya existe, no se toca.
local G_CURSORES = 0x1436F9BC8
local PARCHES_CURSORES = {   -- v0.9.35: lista completa del diseño (cap_60_n, tabla CUR), con el byte que faltaba en la v0.9.22
    { dir = 0x140E67A24, nombre = "cursores: tamaño 0x340 → 0x640", orig = "ba40030000", nuevo = "ba40060000" },
    { dir = 0x140E675D7, nombre = "cursores: construir 8 → 16 entradas", orig = "be08000000", nuevo = "be10000000" },
    { dir = 0x140E675DF, nombre = "cursores: tamaño de entrada sigue en 0x58 (fallo de la v0.9.22)", orig = "8d5650", nuevo = "8d5648" },
    { dir = 0x140E678DB, nombre = "cursores: recorrido de cada cuadro < 16", orig = "83ff08", nuevo = "83ff10" },
    { dir = 0x140E67906, nombre = "cursores: terminar 16", orig = "bf08000000", nuevo = "bf10000000" },
    { dir = 0x140E67CC6, nombre = "cursores: crear 16 punteros", orig = "be08000000", nuevo = "be10000000" },
    { dir = 0x140E67EC3, nombre = "cursores: FUN_140E67EC0 < 16", orig = "83fa08", nuevo = "83fa10" },
    { dir = 0x140E682C1, nombre = "cursores: FUN_140E67EE0 < 16", orig = "4183fe08", nuevo = "4183fe10" },
    { dir = 0x140E6782E, nombre = "cursores: destructor 16 entradas", orig = "448d42b0", nuevo = "448d42b8" },
    { dir = 0x140E67846, nombre = "cursores: destructor tamaño 0x640", orig = "ba40030000", nuevo = "ba40060000" },
    { dir = 0x140E39DE4, nombre = "ventanas: dueño > 7 → > 15 (FUN_140E39DA0)", orig = "83b81401000007", nuevo = "83b8140100000f" },
    { dir = 0x140E3DCBE, nombre = "ventanas: dueño > 7 → > 15 (FUN_140E3DCA0)", orig = "83b81401000007", nuevo = "83b8140100000f" },
}
-- v0.9.24 (23-oct): en vez de agrandar el gestor de cursores (v0.9.22 impedía arrancar PES), cada función que entra
-- en su tabla de 8 cursores con «nº de mando × 0x58» usa ahora «(nº de mando AND 7) × 0x58»: el mando 9+ usa el cursor
-- de puntero del mando (n − 8) en vez de leer/escribir fuera de la tabla. Mismo tamaño de instrucciones (mov eax,edx +
-- imul r64,rax → and edx,7 + imul r32,edx). FUN_140E67EE0/68370 no se tocan (solo se llaman con 0–7).
local PARCHES_CURSOR_AND7 = {
    { dir = 0x140E6797F, nombre = "cursor de menú FUN_140E67970: mando AND 7", orig = "8bc24c8d442420486bf858", nuevo = "83e2076bfa584c8d442420" },
    { dir = 0x140E67A77, nombre = "cursor de menú FUN_140E67A70 (posición del cursor): mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67AA0, nombre = "cursor de menú FUN_140E67AA0: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67B30, nombre = "cursor de menú FUN_140E67B30: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67B40, nombre = "cursor de menú FUN_140E67B40: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67B50, nombre = "cursor de menú FUN_140E67B50: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67B70, nombre = "cursor de menú FUN_140E67B70: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67D50, nombre = "cursor de menú FUN_140E67D50: mando AND 7", orig = "8bc2486bd058", nuevo = "83e2076bd258" },
    { dir = 0x140E67B9F, nombre = "cursor de menú FUN_140E67B90: mando AND 7", orig = "8bc2498bf0486bf858", nuevo = "83e207498bf06bfa58" },
    { dir = 0x140E67BEA, nombre = "cursor de menú FUN_140E67BE0: mando AND 7", orig = "8bc2498bf8486bd858", nuevo = "83e207498bf86bda58" },
    { dir = 0x140E67C46, nombre = "cursor de menú FUN_140E67C40: mando AND 7", orig = "8bc2488d542430486bd858", nuevo = "83e2076bda58488d542430" },
    { dir = 0x140E67C70, nombre = "cursor de menú FUN_140E67C70: mando AND 7", orig = "8bc2450fb6c8486bd058", nuevo = "83e207450fb6c86bd258" },
    { dir = 0x140E67D76, nombre = "cursor de menú FUN_140E67D70: mando AND 7", orig = "8bc2488d9980000000486bd058", nuevo = "83e207488d99800000006bd258" },
}
for _, p in ipairs(PARCHES_CURSOR_AND7) do PARCHES[#PARCHES + 1] = p end
-- Configuración del partido: [[0x143705E10]+0x50]+0x3F308 (listas de 24 por ranura)
local G_AJUSTES, OFF_AJ = 0x143705E10, 0x3F308
local AJ_LADO, AJ_MANDO, AJ_LOCAL = 0x2D, 0x45, 0x180
-- Color de flecha (v0.4). Formato del juego: R | G<<8 | B<<16 | A<<24 (FUN_140490590).
-- Originales (mandos 0–5): cian, rosa, amarillo, verde, naranja, violeta; 6–7 y resto: gris.
-- primeros 16 bytes: mov [rsp+10],rbx · push rdi · sub rsp,40 · movzx edi,dl (13 = los que se mueven) · mov rbx,rcx
local F_COLOR, COLOR_ORIG = 0x1420BD9C0, "48895c2410574883ec400fb6fa488bd9"
local COLORES_NUEVOS = {            -- mandos 8, 9, 10… (se repiten si hay más de 16)
    { 255,  40,  40 },  -- 8  rojo
    {  40,  90, 255 },  -- 9  azul
    { 255, 255, 255 },  -- 10 blanco
    { 100,  50,  10 },  -- 11 chocolate   (26-oct, Fralex: colores lo más distintos posible; se mantienen
    { 100,   0, 170 },  -- 12 morado oscuro  rojo, azul, blanco y negro. Antes: marrón (≈ naranja), verde
    {   0, 128, 128 },  -- 13 turquesa oscuro  oscuro (≈ verde), granate (≈ rojo) y azul marino (≈ azul).
    { 128, 128,   0 },  -- 14 oliva           Mismos colores en piezas.raw: herramientas\generar_replica_lados.ps1)
    {  32,  32,  32 },  -- 15 negro
}
-- Métodos esperados en la vftable de HidMan (comprobación de seguridad)
local METODOS = { [0xE8] = 0x141B52C80, [0x110] = 0x141B52FC0, [0x118] = 0x141B521A0, [0x120] = 0x141B52170 }

-- en Sider ffi ya existe (luajit.ext.enabled = 1); fuera del juego (vista previa) no hay ffi
local ffi = ffi or (pcall and (function() local ok, f = pcall(require, "ffi"); return ok and f or nil end)()) or nil
local carpeta, apagado, enCurso, fallos = nil, false, false, 0
local RPM, VQ, PROC, buf, leidos1, mbi
local lineas, t0, ultimoMirar, ultimoVolcado = {}, os.clock(), -1000, -1000
local paginas = {}
local estado = "En espera. En el MENÚ pulsa I para activar el 9.º mando."
local activo, parchado, guidNuevo, rellenos = false, false, nil, 0

-- ─── utilidades ──────────────────────────────────────────────────────────────
local function anotar(l) lineas[#lineas + 1] = string.format("%.2f %s", os.clock() - t0, l) end
local function volcar()
    if #lineas == 0 or not carpeta then return end
    local f = io.open(carpeta .. "fase1.txt", "ab")
    if f then f:write(table.concat(lineas, "\n") .. "\n"); f:close(); lineas = {} end
    ultimoVolcado = os.clock()
end
local function hex2bin(h) return (h:gsub("..", function(c) return string.char(tonumber(c, 16)) end)) end
local function bin2hex(s) return (s:gsub(".", function(c) return string.format("%02x", c:byte()) end)) end

local function preparar()
    if RPM then return end
    ffi.cdef[[
        typedef struct { uint64_t BaseAddress; uint64_t AllocationBase; uint32_t AllocationProtect;
                         uint16_t PartitionId; uint16_t r0; uint64_t RegionSize;
                         uint32_t State; uint32_t Protect; uint32_t Type; uint32_t r1; } phxf1_MBI;
        int    phxf1_RPM(void* p, const void* desde, void* hacia, size_t n, size_t* leidos) __asm__("ReadProcessMemory");
        void*  phxf1_GCP(void) __asm__("GetCurrentProcess");
        size_t phxf1_VQ(const void* dir, phxf1_MBI* info, size_t tam) __asm__("VirtualQuery");
        void*  phxf1_VA(void* dir, size_t tam, uint32_t tipo, uint32_t prot) __asm__("VirtualAlloc");
        int32_t phxf1_carpeta(void* h, int32_t csidl, void* t, uint32_t f, char* ruta) __asm__("SHGetFolderPathA");
        void     phxf1_tecla(uint8_t vk, uint8_t scan, uint32_t flags, uintptr_t extra) __asm__("keybd_event");
        void*    phxf1_ventana(void) __asm__("GetForegroundWindow");
        uint32_t phxf1_pidventana(void* w, uint32_t* pid) __asm__("GetWindowThreadProcessId");
        uint32_t phxf1_pid(void) __asm__("GetCurrentProcessId");
        typedef struct { const char* p; uint64_t n; } phxf1_txt;
        typedef void (*phxf1_m_idx_ptr)(void* self, uint32_t* idx, void* p);
        typedef void (*phxf1_m_idx)(void* self, uint32_t* idx);
        typedef void (*phxf1_m_v)(void* self);
    ]]
    RPM, VQ, PROC = ffi.C.phxf1_RPM, ffi.C.phxf1_VQ, ffi.C.phxf1_GCP()
    buf = ffi.new("uint8_t[?]", 0x1000)
    leidos1 = ffi.new("size_t[1]")
    mbi = ffi.new("phxf1_MBI")
end
local LEGIBLE = { [0x02] = true, [0x04] = true, [0x08] = true, [0x20] = true, [0x40] = true, [0x80] = true }
local function paginaSegura(a)
    local p = math.floor(a / 4096)
    local v = paginas[p]
    if v ~= nil then return v end
    v = false
    if VQ(ffi.cast("const void*", p * 4096), mbi, ffi.sizeof(mbi)) ~= 0 then
        local prot = tonumber(mbi.Protect)
        v = tonumber(mbi.State) == 0x1000 and prot % 0x200 < 0x100 and LEGIBLE[prot % 0x100] == true
    end
    -- v0.9 BLINDAJE: solo se recuerda «legible». Una página no legible se vuelve a mirar la próxima vez: la pantalla de
    -- lados de PES se crea en memoria nueva cada vez y una página vista vacía antes quedaba «prohibida» para siempre.
    -- (Si una página recordada se libera luego, ReadProcessMemory falla sin romper nada.)
    if v then paginas[p] = v end
    return v
end
local function copiar(a, n)
    if not a or a < 0x10000 or n < 1 or n > 0x1000 then return nil end
    if not paginaSegura(a) or not paginaSegura(a + n - 1) then return nil end
    leidos1[0] = 0
    if RPM(PROC, ffi.cast("const void*", a), buf, n, leidos1) == 0 or tonumber(leidos1[0]) ~= n then return nil end
    return buf
end
local function leer(a, n) local p = copiar(a, n); return p and ffi.string(p, n) or nil end
local function u8(a) local p = copiar(a, 1); return p and p[0] or nil end
local function u32(a) local p = copiar(a, 4); return p and tonumber(ffi.cast("uint32_t*", p)[0]) or nil end
local function ptr(a) local p = copiar(a, 8); return p and tonumber(ffi.cast("uint64_t*", p)[0]) or nil end

-- ─── paso 1: encontrar el mando físico (no está en settings.dat) ──────────────
local function guidsDeSettings(hid)
    local s = {}
    local gi = ptr(hid + 0xD0)
    if not gi then return s end
    for i = 0, 7 do
        local g = leer(gi + i * 0x20 + 0x10, 16)
        if g then s[g] = true end
    end
    return s
end
local function mandosDirectInput()
    local di = ptr(G_DI8); if not di or di == 0 then return {} end
    local cab = ptr(di + 0x90); if not cab or cab == 0 then return {} end
    local raiz = ptr(cab + 8)
    local res, pila, vistos = {}, { raiz }, 0
    while #pila > 0 and vistos < 200 do
        local n = table.remove(pila)
        if n and n ~= 0 and n ~= cab and u8(n + 0x19) == 0 then
            vistos = vistos + 1
            local g, disp = leer(n + 0x20, 16), ptr(n + 0x30)
            local tipo = disp and disp ~= 0 and u8(disp + 0x29) or nil
            -- v0.8.7 BLINDAJE: «vivo» = el objeto del dispositivo tiene su interfaz DirectInput (+0x20 ≠ 0). Las entradas
            -- viejas de mandos que ya no existen la tienen a 0: PES lee basura de ellas (en vivo, 20-oct: el 90760ff5
            -- dado de alta como mando 12 «pulsaba» START sin parar → pausa / pantalla de lados / trabado)
            -- v0.9.59 (26-oct, Fralex: «solo hay 15 mandos»): la interfaz DirectInput de la envoltura está en +0x10; +0x20 es
            -- el enlace «siguiente» de la lista (rea/mandos22_informe.md), así que el ÚLTIMO dispositivo salía «sin mando
            -- real» aunque existiera (hoy el puesto 16 de Link). PES quita del mapa los mandos desenchufados.
            local vivo = (disp and disp ~= 0 and (ptr(disp + 0x10) or 0) ~= 0) or false
            if g then res[#res + 1] = { guid = g, tipo = tipo, vivo = vivo } end
            pila[#pila + 1] = ptr(n); pila[#pila + 1] = ptr(n + 0x10)
        end
    end
    return res
end

-- ─── paso 1b: dar de alta el mando lógico 8 en HidMan ─────────────────────────
local function textoBotonesDe(hid, i)
    local ini = ptr(hid + 0x90); if not ini then return nil end
    local s = ini + i * 0x68 + 0x28
    local n, cap = ptr(s + 0x10), ptr(s + 0x18)
    if not n or n < 1 or n > 2000 then return nil end
    local donde = (cap and cap > 15) and ptr(s) or s
    return leer(donde, n)
end
-- v0.8.5 BLINDAJE: antes se copiaba SIEMPRE la configuración de botones del mando lógico 1 a los extra (mandos
-- virtuales de Xbox de Link/ViGEm). Si ese mando es de otro tipo (p. ej. el DualSense físico), algún eje o gatillo en
-- reposo podía leerse como START → PES pausaba y abría su pantalla de lados («unirse al partido» de
-- Match/MatchPrePause → 1_side_setting). Ahora se copia la configuración MÁS REPETIDA entre los mandos 1–8 (los de
-- Link son iguales entre sí); si hay empate, la del mando con número más alto.
local function textoBotonesDelMando1(hid)
    local cuenta, mejor, mejorN = {}, nil, 0
    for i = 7, 0, -1 do
        local t = textoBotonesDe(hid, i)
        if t then
            cuenta[t] = (cuenta[t] or 0) + 1
            if cuenta[t] > mejorN then mejor, mejorN = t, cuenta[t] end
        end
    end
    if mejor then anotar(string.format("botones para los extra: la configuración que se repite %d de 8 veces", mejorN)) end
    return mejor
end
local function darDeAlta(hid, guid, n)
    local vt = ptr(hid); if not vt then return false, "sin vftable" end
    for off, esperado in pairs(METODOS) do
        if ptr(vt + off) ~= esperado then return false, string.format("vftable+0x%X no coincide", off) end
    end
    -- BLINDAJE: no volver a dar de alta un mando lógico que ya tiene ese mismo GUID (pasaba al pulsar I otra vez o tras
    -- recargar el módulo; cada alta repetida reemplaza datos internos de HidMan sin necesidad)
    local gi = ptr(hid + 0xD0)
    if gi and leer(gi + (n or NUEVO) * 0x20 + 0x10, 16) == guid then return true, "ya estaba dado de alta (no se repite)" end
    local txt = textoBotonesDelMando1(hid)
    if not txt then return false, "no pude leer los botones del mando 1" end
    local self = ffi.cast("void*", hid)
    local idx = ffi.new("uint32_t[1]", n or NUEVO)
    local cbuf = ffi.new("char[?]", #txt + 1, txt)
    local sv = ffi.new("phxf1_txt[1]")
    sv[0].p = cbuf; sv[0].n = #txt
    local g = ffi.new("uint8_t[16]"); ffi.copy(g, guid, 16)
    ffi.cast("phxf1_m_idx_ptr", METODOS[0xE8])(self, idx, sv)
    ffi.cast("phxf1_m_idx", METODOS[0x120])(self, idx)
    ffi.cast("phxf1_m_v", METODOS[0x118])(self)
    ffi.cast("phxf1_m_idx_ptr", METODOS[0x110])(self, idx, g)
    return true, "botones (" .. #txt .. " car.) + activar + GUID"
end

-- ─── paso 2: parches de código ────────────────────────────────────────────────
local function aplicarParches(deshacer)
    for _, p in ipairs(PARCHES) do
        local n = #p.orig / 2
        local actual = leer(p.dir, n)
        local esperado = deshacer and p.nuevo or p.orig
        if not actual then return false, "no se pudo leer " .. p.nombre end
        local cmp = bin2hex(actual):sub(1, #esperado)
        if not deshacer and bin2hex(actual):sub(1, #p.nuevo) == p.nuevo then p.ya = true
        elseif cmp ~= esperado then return false, "bytes distintos en " .. p.nombre .. ": " .. bin2hex(actual) end
    end
    for _, p in ipairs(PARCHES) do
        if p.ya and not deshacer then anotar("ya estaba parchado: " .. p.nombre) else
        memory.write(p.dir, hex2bin(deshacer and p.orig or p.nuevo))
        anotar((deshacer and "restaurado " or "parchado ") .. p.nombre) end
        p.ya = nil
    end
    return true
end

-- ─── paso 4 (v0.4): colores de flecha para los mandos 8–23 ───────────────────
-- Página propia (ejecutable): +0x000 código, +0x100 tabla de 24 colores, +0x200 contador de peticiones por mando.
-- Código (rcx = salida, dl = nº mando, r8b = variante):
--   cmp dl,8 / jb ORIG / cmp dl,24 / jae ORIG / movzx eax,dl / mov r9,tabla / inc dword [r9+rax*4+0x100]
--   mov eax,[r9+rax*4] / mov [rcx],eax / mov rax,rcx / ret
--   ORIG: las 4 instrucciones originales (13 bytes, sin direcciones relativas) / jmp 0x1420BD9CD
-- Entrada de FUN_1420BD9C0: mov rax,codigo / jmp rax / nop (13 bytes).
local pagColor, colorDesviado = nil, false
local function u64le(v)
    local s = {}
    for i = 1, 8 do s[i] = string.char(v % 256); v = math.floor(v / 256) end
    return table.concat(s)
end
local function rgba(c) return c[1] + c[2] * 0x100 + c[3] * 0x10000 + 0xFF000000 end
-- Página propia (4 KB, ejecutable) compartida: +0x000 colores · +0x300/+0x400/+0x480 ganchos · +0x500 datos
local function pagina()
    if pagColor then return pagColor end
    local p = ffi.C.phxf1_VA(nil, 0x1000, 0x3000, 0x40)
    if p == nil then return nil end
    pagColor = tonumber(ffi.cast("uint64_t", p))
    return pagColor
end
-- Código de la página (probado fuera del juego ejecutándolo de verdad: herramientas\probar_codigo_colores.ps1).
local function codigoColor(tabla)
    return hex2bin("80fa08" .. "7224" .. "80fa18" .. "731f" .. "0fb6c2") .. hex2bin("49b9") .. u64le(tabla)
        -- primero el contador y DESPUÉS el color: «mov eax,[…]» pisa rax (índice). v0.6.4 lo hacía al revés → crash
        -- al empezar el partido (inc en dirección rax=color; PES c0000005 en página+0x1B, 20-oct 13:21) y, antes,
        -- escrituras sueltas en memoria del juego (probable causa del cierre en free() de las 13:06)
        .. hex2bin("41ff848100010000" .. "418b0481" .. "8901" .. "4889c8" .. "c3")
        .. hex2bin("48895c2410574883ec400fb6fa" .. "ff2500000000") .. u64le(F_COLOR + 13)
end
local function llenarPagina()
    local tabla = pagColor + 0x100
    local t = ffi.cast("uint32_t*", tabla)
    for i = 0, 23 do
        t[i] = i < 8 and 0xFFD3D3D3 or rgba(COLORES_NUEVOS[(i - 8) % #COLORES_NUEVOS + 1])
        t[i + 0x40] = 0                                             -- contadores (+0x200)
    end
    local codigo = codigoColor(tabla)
    ffi.copy(ffi.cast("void*", pagColor), codigo, #codigo)
    if ffi.string(ffi.cast("const char*", pagColor), #codigo) ~= codigo then return false, "la página no quedó bien escrita" end
    return true
end
local function desviarColor()
    if colorDesviado then return true end
    local actual = leer(F_COLOR, #COLOR_ORIG / 2)
    if not actual then return false, "no se pudo leer FUN_1420BD9C0" end
    local hx = bin2hex(actual)
    -- BLINDAJE: si el desvío ya está puesto (módulo recargado con Shift+R o versión anterior), se ADOPTA su página y
    -- se reescribe su código con el correcto (arregla en caliente una página de una versión con fallo).
    if hx:sub(1, 4) == "48b8" and hx:sub(21, 26) == "ffe090" then
        local d = 0
        for i = 8, 1, -1 do d = d * 256 + actual:byte(2 + i) end
        local firma = leer(d, 5)
        if firma and bin2hex(firma) == "80fa087224" then
            pagColor = d
            local ok, msg = llenarPagina(); if not ok then return false, msg end
            colorDesviado = true
            anotar(string.format("colores de flecha: desvío ya puesto, página 0x%X adoptada y código renovado", d))
            return true
        end
        return false, "FUN_1420BD9C0 está desviada por algo que no es este módulo: no la toco"
    end
    if hx ~= COLOR_ORIG then return false, "bytes distintos en FUN_1420BD9C0: " .. hx end
    if not pagina() then return false, "VirtualAlloc falló" end
    local ok, msg = llenarPagina(); if not ok then return false, msg end
    -- la página ya está lista y comprobada ANTES de desviar la entrada (nunca se salta a código a medias)
    memory.write(F_COLOR, hex2bin("48b8") .. u64le(pagColor) .. hex2bin("ffe090"))
    colorDesviado = true
    anotar(string.format("colores de flecha: FUN_1420BD9C0 desviada a 0x%X (%d colores nuevos)", pagColor, #COLORES_NUEVOS))
    return true
end
local function restaurarColor()
    if not colorDesviado then return end
    memory.write(F_COLOR, hex2bin(COLOR_ORIG:sub(1, 26)))
    colorDesviado = false
    anotar("colores de flecha: código original restaurado")
end
local function peticionesColor()
    if not pagColor then return "-" end
    local t, s = ffi.cast("uint32_t*", pagColor + 0x200), {}
    for i = 8, 15 do s[#s + 1] = tostring(t[i]) end
    return table.concat(s, "/")
end

-- ─── ganchos (v0.8) ───────────────────────────────────────────────────────────
-- 1) PRODUCTOR FUN_141FB9040 (reparte mandos→jugadores al preparar el partido): justo al entrar, si el módulo está
--    activo, escribe en la lista de ajustes S los lados (y mandos) de los mandos extra — y los de 1–8 si se confirmaron
--    en la pantalla Phoenix —. Así llegan EXACTAMENTE a tiempo (en v0.6/0.7 se escribían con el partido ya empezado:
--    tarde → los extra no entraban; en v0.3, escribiendo sin parar, sí entró el 9.º).
-- 2) PANTALLA DE LADOS DE PES: menu::SideSelect se crea en FUN_140ABEAD0 y se destruye en FUN_140ABF000 (que lee S):
--    se cuentan aperturas/cierres para abrir y cerrar SOLA la pantalla Phoenix encima de la de PES.
-- Cada gancho: entrada → «mov rax,código; jmp rax» (rax no es argumento en x64) → código previo + las instrucciones
-- originales movidas (sin direcciones relativas) → «jmp» de vuelta. Solo usa rax, r10, r11 (volátiles, no argumentos).
-- Datos en página+0x500: +0 activo · +1 nativos confirmados · +8 lados 0–7 · +0x10 lados 8–15 · +0x18 mandos 8–15 ·
--   +0x20 veces productor · +0x24 aperturas pantalla lados · +0x28 cierres · +0x30 último objeto SideSelect
local OFF_DATOS = 0x500
local GANCHOS = {
    prod   = { dir = 0x141FB9040, off = 0x300, orig = "40555356574154415541564157488d6c2488", nombre = "productor FUN_141FB9040" },
    -- v0.8.4: el de «crear» va en +0x800 (hasta 0x100 bytes): además copia la pila (diagnóstico) a datos+0x40..+0x240
    abre   = { dir = 0x140ABEAD0, off = 0x800, max = 0x100, orig = "4488442418488954241048894c240853555657", nombre = "pantalla de lados: crear" },
    cierra = { dir = 0x140ABF000, off = 0x480, orig = "40574883ec3048c7442420feffffff", nombre = "pantalla de lados: destruir" },
}
local function hx8(v) return string.format("%02x", v % 256) end
local function hx32(v) return hx8(v) .. hx8(math.floor(v / 0x100)) .. hx8(math.floor(v / 0x10000)) .. hx8(math.floor(v / 0x1000000)) end
local function hx64(v) return bin2hex(u64le(v)) end
-- Código previo del productor. «global» = 0x143705E10 (se puede cambiar para probarlo fuera del juego).
local function preProductor(datos, global)
    local fin = { "41ff4220",                                                     -- inc [datos+0x20] (contador)
        "4d8b5a10", "4c8998" .. hx32(OFF_AJ + AJ_LADO + 8),                        -- lados 8–15
        "4d8b5a18", "4c8998" .. hx32(OFF_AJ + AJ_MANDO + 8),                       -- mandos 8–15
        "4d8b5a38", "4c8998" .. hx32(OFF_AJ + AJ_LOCAL + 8) }                      -- v0.9.29: perfil (usuario local) 8–15
    local nat = "4d8b5a08" .. "4c8998" .. hx32(OFF_AJ + AJ_LADO)                    -- lados 0–7 (si nativos)
    local B = table.concat(fin)
    local C = "41807a0100" .. "74" .. hx8(#nat / 2) .. nat                           -- cmp byte [r10+1],0 / jz fin
    local tras3 = B .. C
    local p3 = "49ba" .. hx64(datos) .. "41803a00" .. "74" .. hx8(#tras3 / 2)       -- r10 = datos; activo? si no, fin
    local tras2 = p3 .. tras3
    local p2 = "488b4050" .. "4885c0" .. "74" .. hx8(#tras2 / 2)                    -- rax = [rax+0x50]; nulo → fin
    local tras1 = p2 .. tras2
    local p1 = "48a1" .. hx64(global) .. "4885c0" .. "74" .. hx8(#tras1 / 2)       -- rax = [global]; nulo → fin
    return p1 .. tras1
end
local function preContador(datos, off, guardarRcx)
    -- guardarRcx: true = el objeto en datos+0x30 (crear); un número = en ese desplazamiento (v0.9.55: destruir → 0x248; libre tras la copia de pila +0x40…+0x23F)
    local donde = (guardarRcx == true) and 0x30 or guardarRcx
    return "49ba" .. hx64(datos) .. "41ff42" .. hx8(off) .. (donde and ("49894a" .. hx8(donde)) or "")
end
-- v0.8.1: al CREARSE la pantalla de lados de PES se limpian en S los mandos 9–16 (lado 0, mando 0x20) ANTES de que
-- ella los lea: los menús nunca ven mandos extra y durante el partido ya no hace falta tocar S desde Lua (en v0.8
-- quitarlos a mitad de partido hizo que PES abriera su pantalla de lados en pleno partido).
local function preAbre(datos, global)
    local limpiar = "4d31db" .. "4c8998" .. hx32(OFF_AJ + AJ_LADO + 8)               -- xor r11,r11 · lados 8–15 = 0
        .. "49bb" .. string.rep("20", 8) .. "4c8998" .. hx32(OFF_AJ + AJ_MANDO + 8)   -- mandos 8–15 = 0x20
        .. "49bb" .. string.rep("ff", 8) .. "4c8998" .. hx32(OFF_AJ + AJ_LOCAL + 8)   -- v0.9.29: perfil 8–15 = FF
    local p2 = "488b4050" .. "4885c0" .. "74" .. hx8(#limpiar / 2)
    local tras1 = p2 .. limpiar
    local p1 = "48a1" .. hx64(global) .. "4885c0" .. "74" .. hx8(#tras1 / 2)
    -- diagnóstico v0.8.4: copia 64 valores de la pila (direcciones de retorno = quién abrió la pantalla) a datos+0x40:
    -- xor eax,eax · L: mov r11,[rsp+rax*8] · mov [r10+rax*8+0x40],r11 · inc eax · cmp eax,64 · jb L
    local pila = "31c0" .. "4c8b1cc4" .. "4d895cc240" .. "ffc0" .. "83f840" .. "72f0"
    return preContador(datos, 0x24, true) .. pila .. p1 .. tras1
end
local function codigoGancho(g, pre)
    return hex2bin(pre .. g.orig .. "ff2500000000") .. u64le(g.dir + #g.orig / 2)
end
-- Pone un gancho comprobando todo; si ya estaba (recarga del módulo), se adopta y se reescribe su código.
local function ponerGancho(g, pre)
    local base = pagina(); if not base then return false, "sin página" end
    local n = #g.orig / 2
    local actual = leer(g.dir, n); if not actual then return false, "no se pudo leer " .. g.nombre end
    local hx = bin2hex(actual)
    local destino = base + g.off
    local codigo = codigoGancho(g, pre)
    if #codigo > (g.max or 0x80) then return false, "código demasiado largo para " .. g.nombre end
    if hx:sub(1, 4) == "48b8" and hx:sub(21, 24) == "ffe0" then
        local d = 0
        for i = 8, 1, -1 do d = d * 256 + actual:byte(2 + i) end
        local firma = leer(d, 2)
        if not firma or (bin2hex(firma) ~= "48a1" and bin2hex(firma) ~= "49ba") then
            return false, g.nombre .. " está desviada por algo que no es este módulo: no la toco"
        end
        -- BLINDAJE: el código nuevo podría no caber en el hueco de una versión anterior → no se arriesga
        if #codigo > 0x80 and d ~= base + g.off then return false, g.nombre .. ": hay un gancho de otra versión; reinicia PES" end
        destino = d                                                             -- página de una carga anterior
    elseif hx ~= g.orig then
        return false, "bytes distintos en " .. g.nombre .. ": " .. hx
    end
    ffi.copy(ffi.cast("void*", destino), codigo, #codigo)
    if ffi.string(ffi.cast("const char*", destino), #codigo) ~= codigo then return false, "código mal escrito en " .. g.nombre end
    if hx == g.orig then   -- la entrada se cambia solo cuando el código de destino ya está listo y comprobado
        memory.write(g.dir, hex2bin("48b8") .. u64le(destino) .. hex2bin("ffe0" .. string.rep("90", n - 12)))
    end
    g.puesto, g.destino = true, destino
    anotar(string.format("gancho puesto: %s → 0x%X", g.nombre, destino))
    return true
end
local function quitarGancho(g)
    if not g.puesto then return end
    local actual = leer(g.dir, 2)
    if actual and bin2hex(actual) == "48b8" then memory.write(g.dir, hex2bin(g.orig)) end
    g.puesto = false
    anotar("gancho quitado: " .. g.nombre)
end
local function datos() return pagColor and ffi.cast("uint8_t*", pagColor + OFF_DATOS) or nil end
local function contadorDatos(off)
    local d = datos(); if not d then return 0 end
    return tonumber(ffi.cast("uint32_t*", d + off)[0])
end
local function ponerGanchosPantalla()
    if not ffi or not memory then return end
    local base = pagina(); if not base then return end
    local d = base + OFF_DATOS
    local ok1, m1 = ponerGancho(GANCHOS.abre, preAbre(d, G_AJUSTES))
    local ok2, m2 = ponerGancho(GANCHOS.cierra, preContador(d, 0x28, 0x248))
    if not ok1 then anotar("gancho NO puesto: " .. tostring(m1)) end
    if not ok2 then anotar("gancho NO puesto: " .. tostring(m2)) end
    if ok1 and not ok2 then quitarGancho(GANCHOS.abre) end              -- o los dos o ninguno
end

-- ─── paso 3 (v0.2): escribir el 9.º mando en la CONFIGURACIÓN del partido ─────
-- El productor oficial (FUN_141FB9040) crea cada ranura con lado = ajustes+0x2D+i y mando lógico =
-- ajustes+0x45+i; así el propio juego crea y mantiene la ranura 8.
local previoAj = {}
local function lista(base, off)
    local s = leer(base + off, 24); if not s then return "?" end
    local t = {}
    for i = 1, 24 do t[i] = string.format("%02x", s:byte(i)) end
    return table.concat(t, " ")
end
-- v0.4: los ajustes se escriben UNA vez por partido (cuando hay cursores activos) y se devuelven a su valor
-- original al salir del partido o al deshacer: en v0.3, reescribirlos cada 0,5 s dejaba la pantalla de lados
-- en bucle (ella también lee estas listas y no tiene fila para el mando 8).
-- v0.5: varios mandos extra y un PLAN de lados que llena la pantalla Phoenix (plan[i] = 0 Local, 1 sin lado,
-- 2 Visitante). Los mandos 0–7 confirmados en la pantalla también se escriben (al confirmar y al empezar el partido).
local ajOrig, ajEscrito, sinPartidoDesde, sinCursorDesde = {}, false, nil, nil
local plan, nativosConfirmados = {}, false
local pedirCierre = false                             -- v0.8: la pantalla pide cerrarse sola tras ENTER
local frenado = false                                 -- v0.8.8: freno de emergencia (bucle de pausas): no meter extras
local extras = {}                                     -- mandos extra dados de alta: { idx = 8.., guid = … }
-- v0.9.25: NÚMERO PRESTADO en los menús. Los menús de PES (foco, cursores, listas, Datos personales) solo existen para
-- los mandos 0–7. Entre el cierre de la pantalla de lados y el saque, un lado que solo tiene mandos 9+ «toma prestado»
-- un número libre 0–7: su mando físico se enlaza en HidMan a esa plaza y S pone ese número en su lado. En el saque se
-- devuelve (el gancho del productor ya escribe los lados 0–7 desde el plan y los 8–15 desde los datos).
local devolverPrestados                               -- (definida más abajo)
local PHX = { OCULTOS = {}, CAPITAN = {}, PERFIL_DE = {}, PRESTADOS = {} }   -- v0.9.30: en una tabla (Lua: máx. 200 locales)
-- v0.9.53 (24-oct, al volver de «Editar Datos personales» la Phoenix dejaba de leer la pantalla de PES): PES crea la
-- pantalla NUEVA antes de destruir la VIEJA, así que aperturas y cierres quedan iguales aunque esté abierta. El gancho de
-- destruir guarda ahora en datos+0x248 el objeto destruido: está abierta si el último creado (+0x30) no es ese.
function PHX.pantallaPESAbierta()   -- en PHX: el cuerpo del módulo ya tiene 200 variables locales
    local d = datos(); if not d then return false end
    if contadorDatos(0x24) ~= contadorDatos(0x28) then return true end
    -- v0.9.55: el destruido va en datos+0x248 (en la v0.9.53/54 iba en +0x38, que son los perfiles 8–15: los pisaba)
    local creado, destruido = tonumber(ffi.cast("uint64_t*", d + 0x30)[0]), tonumber(ffi.cast("uint64_t*", d + 0x248)[0])
    return creado >= 0x10000 and destruido ~= 0 and creado ~= destruido
end
-- OCULTOS                                    -- v0.9.30: [mando 0–7] = lado S que tenía (oculto en los menús por el capitán)
-- CAPITAN                                    -- v0.9.30: [1 Local | 2 Visitante] = nº de mando capitán (0–15)
-- PERFIL_DE                                  -- v0.9.29: [mando extra] = nº de «Datos personales» elegido (0–31)
local function hayPrestados(t) for _ in pairs(t) do return true end return false end   -- Sider no tiene next()
-- PRESTADOS                                  -- [k] = { idx = mando extra, r = fila de dispositivo, codigo = lado S }
local CODIGO_LADO = { [0] = 1, [1] = 0, [2] = 2 }     -- pantalla → lista de ajustes (0 nadie, 1 Local, 2 Visitante)
local LADO_DE_CODIGO = { [0] = 1, [1] = 0, [2] = 2 }  -- y al revés
-- v0.8: pasa el plan confirmado a los datos que lee el gancho del productor
local function actualizarDatos()
    local d = datos(); if not d then return end
    local registrado = {}
    for _, e in ipairs(extras) do registrado[e.idx] = true end
    for i = 0, 7 do d[8 + i] = CODIGO_LADO[plan[i] or 1] end                     -- 1–8 (solo se usan si nativos)
    for k = 0, 7 do
        local i = NUEVO + k
        -- v0.9.13: sin elección = CENTRO (antes «or 0» = Local: los extra entraban al partido en Local sin elegir)
        d[0x10 + k] = registrado[i] and CODIGO_LADO[plan[i] or 1] or 0
        -- v0.9.57 (24-oct, Fralex: en el partido el mando 9 sale con otro nombre): S+0x45 NO es solo «mando», es el
        -- USUARIO de la ranura (medido: al cambiar el usuario del teclado, S+0x45[0] pasó a 0x08/0x0D = su perfil). Con «i»
        -- el extra salía con el usuario de su número (MORGADO…). Ahora va el elegido con L2/R2.
        d[0x18 + k] = registrado[i] and (PHX.PERFIL_DE[i] or i) or 0x20
        -- v0.9.29: «Datos personales» (usuario local) de cada extra: el elegido con L2/R2 en la pantalla de lados (o el de
        -- su número). Lo copia el gancho del productor a S+0x180 al saque.
        d[0x38 + k] = (registrado[i] and CODIGO_LADO[plan[i] or 1] ~= 0) and (PHX.PERFIL_DE[i] or i) or 0xFF
    end
    d[1] = nativosConfirmados and 1 or 0
    -- v0.9.25: con números prestados, el gancho del productor (en el saque) escribe los lados 0–7 = los de S ahora,
    -- pero con el número prestado en el CENTRO: así el saque lo crea con su número 9+ y no con el prestado
    if hayPrestados(PHX.PRESTADOS) or hayPrestados(PHX.OCULTOS) then
        local a0 = ptr(G_AJUSTES); local b0 = (a0 and a0 ~= 0) and ptr(a0 + 0x50) or nil
        local s = (b0 and b0 ~= 0) and (b0 + OFF_AJ) or nil      -- (direccionAjustes está definida más abajo)
        if s then
            for i = 0, 7 do d[8 + i] = PHX.OCULTOS[i] or (PHX.PRESTADOS[i] and 0) or (u8(s + AJ_LADO + i) or 0) end
            d[1] = 1
        end
    end
    d[0] = (activo and #extras > 0 and not frenado) and 1 or 0                 -- el último: «activo» al final
end
local function direccionAjustes()
    local a = ptr(G_AJUSTES); if not a or a == 0 then return nil end
    local b = ptr(a + 0x50); if not b or b == 0 then return nil end
    return b + OFF_AJ
end
local function enPartido(tabla)
    if not tabla or tabla == 0 then return false end
    for i = 0, 7 do
        local r = tabla + i * TAM_RANURA
        local mnd, jug = u32(r + 0x10), u32(r + 0x20)
        if mnd and mnd < 24 and jug and jug < 22 and u8(r + 0x68) == 1 then return true end
    end
    return false
end
-- Escribe en la lista de lados (y, para los extra, en la de mandos) lo que diga el plan.
local function escribirUno(s, i, lado, motivo)
    local l0, m0 = u8(s + AJ_LADO + i), u8(s + AJ_MANDO + i)
    if l0 == nil or m0 == nil then return end
    if not ajOrig[i] then ajOrig[i] = { lado = l0, mando = m0 } end
    local codigo = CODIGO_LADO[lado]
    if l0 ~= codigo then memory.write(s + AJ_LADO + i, string.char(codigo)) end
    local u = PHX.PERFIL_DE[i] or i                                                    -- v0.9.57: usuario elegido
    if i >= NUEVO and m0 ~= u then memory.write(s + AJ_MANDO + i, string.char(u)) end
    anotar(string.format("ajustes ranura %d (%s): lado %d→%d, mando/usuario %d→%d", i, motivo, l0, codigo, m0, i >= NUEVO and u or m0))
end
local function ahora() return (os.time and os.time()) or os.clock() end   -- segundos de reloj real
local function hayRanurasJugando(tabla)
    if not tabla or tabla == 0 then return false end
    for i = 0, N_RANURAS - 1 do
        local r = tabla + i * TAM_RANURA
        local mnd, jug = u32(r + 0x10), u32(r + 0x20)
        if mnd and mnd < 24 and jug and jug < 22 then return true end
    end
    return false
end
local function ponerAjustes(s, motivo)
    -- BLINDAJE: los mandos 1–8 NO se tocan al empezar el partido (se escriben solo al pulsar ENTER en la pantalla);
    -- así manda la última elección, también si luego se cambió en la pantalla de lados de PES.
    for _, e in ipairs(extras) do escribirUno(s, e.idx, plan[e.idx] or 0, motivo) end
    ajEscrito = true
    rellenos = rellenos + 1
end
local function quitarAjustes(motivo)
    if not ajEscrito then return end
    local s = direccionAjustes()
    if s then
        for i, o in pairs(ajOrig) do
            if i >= NUEVO then   -- los 0–7 los deja como los eligió la pantalla: también valen para el menú
                memory.write(s + AJ_LADO + i, string.char(o.lado))
                memory.write(s + AJ_MANDO + i, string.char(o.mando))
                anotar(string.format("ajustes ranura %d devueltos (%s): lado %d, mando %d", i, motivo, o.lado, o.mando))
                ajOrig[i] = nil
            end
        end
    end
    ajEscrito = false
end
local function escribirAjustes(forzar)
    local s = direccionAjustes(); if not s then return end
    for nombre, off in pairs({ lado = AJ_LADO, mando = AJ_MANDO, ["local"] = AJ_LOCAL }) do
        local v = lista(s, off)
        if previoAj[nombre] ~= v then anotar("ajustes." .. nombre .. " = " .. v); previoAj[nombre] = v end
    end
    local gest = ptr(G_GESTOR)
    local tabla = (gest and gest ~= 0) and ptr(gest + 0x180) or nil
    local jugando = enPartido(tabla)
    if forzar then
        ponerAjustes(s, "tecla K")
    elseif jugando then
        sinPartidoDesde, sinCursorDesde = nil, nil
        -- v0.9.41: préstamo de la pausa: si se reanuda sin que el productor vuelva a pasar, se devuelve igual al jugar
        if hayPrestados(PHX.PRESTADOS) and hayRanurasJugando(tabla) then devolverPrestados("reanudar") end
        if not ajEscrito then
            if GANCHOS.prod.puesto then
                -- v0.8.1: lo escribe el gancho del productor a tiempo y lo limpia el gancho de la pantalla de lados:
                -- desde Lua NO se toca S durante el partido (ni para devolverlo)
                if contadorDatos(0x20) ~= (previoAj.prodVisto or -1) then
                    previoAj.prodVisto = contadorDatos(0x20)
                    anotar(string.format("partido en curso: el productor pasó por el gancho %d vez/veces", previoAj.prodVisto))
                    if hayPrestados(PHX.PRESTADOS) or hayPrestados(PHX.OCULTOS) then devolverPrestados("saque") end      -- v0.9.25: en el partido juega con su 9+
                end
            else
                ponerAjustes(s, "partido en curso (método viejo: sin gancho)")
            end
        end
    elseif ajEscrito then
        -- BLINDAJE: solo se devuelven cuando de verdad se salió del partido (ninguna ranura con mando y jugador) durante
        -- 15 s de reloj real; antes bastaban 3 s sin cursor activo, y en repeticiones o el descanso podía quitarlos a mitad
        -- de partido. os.clock() es tiempo de CPU (corre más rápido con varios hilos): para esperas se usa os.time().
        -- y por si la tabla guarda datos viejos al salir: también se devuelven tras 90 s seguidos sin ningún cursor activo
        sinCursorDesde = sinCursorDesde or ahora()
        if hayRanurasJugando(tabla) then
            sinPartidoDesde = nil
        else
            sinPartidoDesde = sinPartidoDesde or ahora()
        end
        if (sinPartidoDesde and ahora() - sinPartidoDesde >= 15) or ahora() - sinCursorDesde >= 90 then
            quitarAjustes("fuera del partido"); sinPartidoDesde = nil; sinCursorDesde = nil
        end
    end
    if tabla and tabla ~= 0 then
        local r8 = tabla + NUEVO * TAM_RANURA
        local m8, j8 = u32(r8 + 0x10), u32(r8 + 0x20)
        local v = tostring(m8) .. "/" .. tostring(j8) .. "/" .. tostring(u8(r8))
        if previoAj.r8 ~= v then
            anotar("ranura 8 (mando/jugador/lado) = " .. v); previoAj.r8 = v
            if m8 == NUEVO then estado = string.format("¡El juego creó la ranura 8! Jugador %s. Mueve tu mando físico.", tostring(j8)) end
        end
        local f8 = lista(r8, 0x68):sub(1, 32)
        if previoAj.f8 ~= f8 then anotar("ranura 8 +0x68.. = " .. f8); previoAj.f8 = f8 end
    end
    -- se anota la primera vez que el juego pide un color 8+ y luego cada 10 s como mucho
    local pc = peticionesColor()
    if previoAj.pc ~= pc and (not previoAj.pcT or os.clock() - previoAj.pcT > 10 or previoAj.pc == "0/0/0/0/0/0/0/0") then
        anotar("peticiones de color de flecha (mandos 8–15) = " .. pc); previoAj.pc = pc; previoAj.pcT = os.clock()
    end
end

-- (v0.1, ya no se usa: el juego borraba la ranura escrita desde fuera)
local function rellenarRanura()
    local gest = ptr(G_GESTOR)
    local tabla = (gest and gest ~= 0) and ptr(gest + 0x180) or nil
    if not tabla or tabla == 0 then return end
    local r8 = tabla + NUEVO * TAM_RANURA
    if u32(r8 + 0x10) ~= 0xFFFFFFFF then return end
    local ref
    for i = 0, 7 do
        local r = tabla + i * TAM_RANURA
        local jug = u32(r + 0x20)
        if u8(r) == 0 and jug and jug < 22 then ref = r; break end
    end
    if not ref then return end
    local datos = leer(ref, TAM_RANURA); if not datos then return end
    local jug = u32(ref + 0x20)
    -- Local = jugadores 0–10 (el 0 suele ser el portero): otro jugador de campo del mismo equipo
    local nuevoJug = (jug > 1) and (jug - 1) or (jug + 1)
    memory.write(r8, datos)
    memory.write(r8 + 0x01, string.char(NUEVO))
    memory.write(r8 + 0x10, memory.pack("u32", NUEVO))
    memory.write(r8 + 0x18, memory.pack("u32", NUEVO))
    memory.write(r8 + 0x20, memory.pack("u32", nuevoJug))
    memory.write(r8 + 0x2C, memory.pack("u32", 255))
    rellenos = rellenos + 1
    anotar(string.format("ranura 8 rellenada (vez %d): Local, jugador %d (copiada de jugador %d)", rellenos, nuevoJug, jug))
    estado = string.format("9.º mando en el partido: LOCAL, jugador %d. ¡Muévelo con tu mando físico!", nuevoJug)
end

-- ─── v0.8.8: ENLACE de los mandos extra con su mando físico ──────────────────
-- Medido en vivo (20-oct): la tabla de HidMan (+0xD0, 32 × 0x20) hace de dos cosas a la vez: FILA DE DISPOSITIVO
-- (+0x00 objeto del mando físico, +0x08 plaza lógica a la que está enlazado) y PLAZA LÓGICA (+0x0C fila de dispositivo
-- enlazada, +0x10 GUID). FUN_141B54F60 solo enlaza un mando cuando APARECE: como los GUID de las plazas 8–15 se
-- escriben con los mandos ya conectados, nunca se enlazaban (+0x0C = −1) → los extra no tenían mando de verdad y PES
-- veía «usuarios con el mando desconectado» → pausa / pantalla de lados / bucle. Aquí se enlazan igual que lo hace PES.
-- El GUID de cada mando físico está en [objeto+0x20] (medido en vivo en las 16 filas).
local function i32(a) local p = copiar(a, 4); return p and tonumber(ffi.cast("int32_t*", p)[0]) or nil end
local function enlazarExtras(motivo)
    local hid = ptr(G_HIDMAN); if not hid or hid == 0 then return 0 end
    local tab, fin = ptr(hid + 0xD0), ptr(hid + 0xD8)
    if not tab or not fin or tab == 0 then return 0 end
    local n = math.floor((fin - tab) / 0x20)
    if n < 16 or n > 64 then return 0 end                     -- BLINDAJE: tamaño de tabla inesperado → no se toca
    local hechos = 0
    for _, e in ipairs(extras) do
        local L = e.idx
        if L < n and i32(tab + L * 0x20 + 0xC) == -1 then
            for r = 0, n - 1 do
                local fila = tab + r * 0x20
                local dev = ptr(fila)
                if dev and dev ~= 0 and i32(fila + 8) == -1 then
                    local gp = ptr(dev + 0x20)
                    if gp and gp ~= 0 and leer(gp, 16) == e.guid then
                        ffi.cast("int32_t*", fila + 8)[0] = L                -- fila de dispositivo → plaza lógica
                        ffi.cast("int32_t*", tab + L * 0x20 + 0xC)[0] = r    -- plaza lógica → fila de dispositivo
                        hechos = hechos + 1
                        anotar(string.format("enlace (%s): plaza lógica %d ↔ fila de dispositivo %d", motivo, L, r))
                        break
                    end
                end
            end
        end
    end
    return hechos
end
-- v0.9.25: préstamo de número 0–7 en los menús (ver PHX.PRESTADOS). Todo con comprobaciones: si algo no cuadra, no se toca.
devolverPrestados = function(motivo)
    local hid = ptr(G_HIDMAN); local v = (hid and hid ~= 0) and ptr(hid + 0x48) or nil
    local s = direccionAjustes()
    for k, p in pairs(PHX.PRESTADOS) do
        if v and v ~= 0 and ptr(v + k * 8) == p.objE and ptr(v + p.idx * 8) == p.objK then
            ffi.cast("uint64_t*", v + k * 8)[0] = p.objK
            ffi.cast("uint64_t*", v + p.idx * 8)[0] = p.objE
        end
        if s and u8(s + AJ_LADO + k) == p.codigo then memory.write(s + AJ_LADO + k, string.char(0)) end
        anotar(string.format("préstamo devuelto (%s): mando %d deja el número %d", motivo, p.idx + 1, k + 1))
    end
    PHX.PRESTADOS = {}
    -- v0.9.30: los mandos 1–8 ocultos por el capitán. En el saque ya los repuso el gancho del productor (d[8+i]); en una
    -- pantalla de lados nueva se vuelven a poner en S para que no se pierda su lado
    if motivo ~= "saque" and s then
        for n, codigo in pairs(PHX.OCULTOS) do
            if u8(s + AJ_LADO + n) == 0 then memory.write(s + AJ_LADO + n, string.char(codigo)) end
        end
    end
    for n in pairs(PHX.OCULTOS) do anotar(string.format("mando %d vuelve a su lado (%s)", n + 1, motivo)) end
    PHX.OCULTOS = {}
    actualizarDatos()
end
-- v0.9.28: el préstamo INTERCAMBIA los objetos de mando lógico en la lista de HidMan (+0x48, un puntero por mando; es
-- lo que devuelve FUN_141B52460 «dame el mando N»). Así el número k devuelve el mando físico del extra y viceversa.
-- (v0.9.27 cambiaba los enlaces fila↔plaza, pero el objeto del mando k seguía leyendo su mando de antes.)
local function prestarNumeros()
    local hid = ptr(G_HIDMAN); if not hid or hid == 0 then return end
    local v, vfin = ptr(hid + 0x48), ptr(hid + 0x50)
    if not v or not vfin or v == 0 or math.floor((vfin - v) / 8) < 16 then return end
    local s = direccionAjustes(); if not s then return end
    for _, codigo in ipairs({ 1, 2 }) do                         -- 1 Local · 2 Visitante (código de S)
        -- v0.9.30 CAPITÁN: si el lado tiene capitán, solo él maneja los menús de ese equipo (equipo, estrategia). Los
        -- demás mandos 1–8 de ese lado se ocultan en S hasta el saque (el gancho del productor los repone).
        local cap = PHX.CAPITAN[codigo]
        if cap and cap < NUEVO and u8(s + AJ_LADO + cap) ~= codigo then cap = nil end
        if cap and cap >= NUEVO and not (plan[cap] and CODIGO_LADO[plan[cap]] == codigo) then cap = nil end
        if cap then
            for i = 0, NUEVO - 1 do
                if i ~= cap and u8(s + AJ_LADO + i) == codigo then
                    PHX.OCULTOS[i] = codigo; memory.write(s + AJ_LADO + i, string.char(0))
                    anotar(string.format("capitán %d (%s): el mando %d no maneja los menús", cap + 1, codigo == 1 and "Local" or "Visitante", i + 1))
                end
            end
        end
        local hayNativo = false
        for i = 0, NUEVO - 1 do if u8(s + AJ_LADO + i) == codigo then hayNativo = true end end
        if not hayNativo then
            local e
            for _, x in ipairs(extras) do
                if not e and x.idx == cap then e = x end
            end
            for _, x in ipairs(extras) do
                if not e and plan[x.idx] and CODIGO_LADO[plan[x.idx]] == codigo then e = x end
            end
            local k
            for c = NUEVO - 1, 0, -1 do
                if not k and not PHX.PRESTADOS[c] and not PHX.OCULTOS[c] and u8(s + AJ_LADO + c) == 0 then k = c end
            end
            local objK, objE = k and ptr(v + k * 8), e and ptr(v + e.idx * 8)
            if e and k and objK and objE and objK ~= 0 and objE ~= 0 then
                ffi.cast("uint64_t*", v + k * 8)[0] = objE
                ffi.cast("uint64_t*", v + e.idx * 8)[0] = objK
                memory.write(s + AJ_LADO + k, string.char(codigo))
                memory.write(s + AJ_LADO + e.idx, string.char(0))
                PHX.PRESTADOS[k] = { idx = e.idx, codigo = codigo, objK = objK, objE = objE }
                anotar(string.format("préstamo: el mando %d usa el número %d en los menús (%s)", e.idx + 1, k + 1, codigo == 1 and "Local" or "Visitante"))
            elseif e then
                anotar(string.format("préstamo NO hecho para el mando %d (número libre %s, objetos %s/%s)", e.idx + 1, tostring(k), tostring(objK), tostring(objE)))
            end
        end
    end
    actualizarDatos()
end
-- Lee el archivo de Phoenix Link (formato en Link: phoenix/core/MandosExtra.h): «PHXM», versión 1, N, N × 0x40
-- (GUID 16 + mapa 48). Devuelve { [i] = guid } (i = 0 → puesto 9) o nil. BLINDAJE: firma/versión/tamaño exactos.
local shell32
local function carpetaDocumentos()
    shell32 = shell32 or ffi.load("shell32")                               -- siempre existe en Windows
    local buf = ffi.new("char[520]")
    if shell32.phxf1_carpeta(nil, 5, nil, 0, buf) ~= 0 then return nil end -- CSIDL_PERSONAL = Documentos (aunque se haya movido a D:)
    return ffi.string(buf)
end
local function puestosDeLink()
    local doc = carpetaDocumentos(); if not doc then return nil end
    local f = io.open(doc .. "\\KONAMI\\eFootball PES 2021 SEASON UPDATE\\phoenix_mandos_extra.dat", "rb")
    if not f then return nil end
    local a = f:read("*a"); f:close()
    if not a or #a < 12 or a:sub(1, 4) ~= "PHXM" then return nil end
    local function u32s(p) local b1, b2, b3, b4 = a:byte(p, p + 3); return b1 + b2 * 256 + b3 * 65536 + b4 * 16777216 end
    local ver, n = u32s(5), u32s(9)
    if ver ~= 1 or n > 24 or #a ~= 12 + n * 0x40 then anotar("archivo de Link con formato inesperado: no se usa"); return nil end
    local t = {}
    for i = 0, n - 1 do
        local g = a:sub(13 + i * 0x40, 12 + i * 0x40 + 16)
        if g ~= string.rep("\0", 16) then t[i] = g end
    end
    return t
end
-- ─── activar / deshacer ───────────────────────────────────────────────────────
local function activar()
    preparar()
    local c = copiar(CAPS, 8)
    if not (c and c[0] == 1 and c[2] == 0x20) then estado = "Exe distinto: no hago nada."; anotar(estado); return end
    local hid = ptr(G_HIDMAN)
    if not hid or hid == 0 then estado = "HidMan no listo: espera al menú y vuelve a pulsar I."; return end
    local enSettings = guidsDeSettings(hid)
    local candidatos, vivos = {}, {}
    for _, d in ipairs(mandosDirectInput()) do
        anotar("dispositivo DI: tipo " .. tostring(d.tipo) .. " guid " .. bin2hex(d.guid) .. (enSettings[d.guid] and " (en settings.dat)" or "") .. (d.vivo and "" or " (SIN mando real: se ignora)"))
        if d.tipo == 1 and d.vivo and not enSettings[d.guid] then vivos[d.guid] = d end
    end
    -- v0.8.10: MISMO NÚMERO QUE EN PHOENIX LINK. Link escribe en Documentos\KONAMI\…\phoenix_mandos_extra.dat el GUID
    -- de cada puesto 9+ (casilla i = puesto 9+i): ese mando va a la plaza lógica 8+i (= «mando 9+i» en PES). Un puesto
    -- sin mando real conectado se salta (no se rellena con otro). Sin archivo de Link: orden de Windows (como antes).
    local link = puestosDeLink()
    if link then
        for i = 0, MAX_MANDOS - NUEVO - 1 do
            local g = link[i]
            -- v0.9.6 BLINDAJE: un mando que ya es uno de los 8 de PES nunca se da de alta otra vez como extra (en vivo,
            -- 20-oct: al mover el mando 1 se movía también la última fila extra)
            if g and enSettings[g] then
                anotar(string.format("puesto %d de Link (%s): ya es uno de los mandos 1–8 de PES → se salta", NUEVO + i + 1, bin2hex(g)))
            elseif g and vivos[g] and vivos[g] ~= "usado" then
                candidatos[#candidatos + 1] = { guid = g, idx = NUEVO + i }
                vivos[g] = "usado"                               -- v0.9.7: el mismo mando nunca en dos puestos
            elseif g then
                anotar(string.format("puesto %d de Link (%s): sin mando real conectado → se salta", NUEVO + i + 1, bin2hex(g)))
            end
        end
        anotar(string.format("numeración de Phoenix Link: %d puesto(s) extra con mando", #candidatos))
    else
        for _, d in ipairs(mandosDirectInput()) do
            if vivos[d.guid] and vivos[d.guid] ~= "usado" and #candidatos < MAX_MANDOS - NUEVO then
                candidatos[#candidatos + 1] = { guid = d.guid, idx = NUEVO + #candidatos }
                vivos[d.guid] = "usado"
            end
        end
        anotar("sin archivo de Link (phoenix_mandos_extra.dat): orden de Windows")
    end
    if #candidatos == 0 then estado = "No encontré mandos extra (¿están conectados?)."; anotar(estado); return end
    local ok, msg = aplicarParches(false)
    if not ok then estado = "No parché: " .. msg; anotar(estado); return end
    parchado = true
    -- v0.5: TODOS los mandos que no están en settings.dat → mandos lógicos 8, 9, 10… (hasta 15)
    extras = {}
    for k, d in ipairs(candidatos) do
        local idx = d.idx
        local ok2, msg2 = darDeAlta(hid, d.guid, idx)
        anotar(string.format("alta en HidMan del mando %d (%s): %s · %s", idx, bin2hex(d.guid), tostring(ok2), msg2))
        if ok2 then extras[#extras + 1] = { idx = idx, guid = d.guid } end
        if not ok2 and k == 1 then
            aplicarParches(true); parchado = false
            estado = "Alta fallida (" .. msg2 .. "); código restaurado."; return
        end
    end
    guidNuevo = extras[1].guid
    local nEnl = enlazarExtras("al activar")
    anotar(string.format("enlazados al activar: %d de %d mandos extra", nEnl, #extras))
    local ok3, msg3 = desviarColor()
    if not ok3 then anotar("colores de flecha NO activados: " .. tostring(msg3)) end
    activo = true
    actualizarDatos()                                    -- primero los datos, luego el gancho que los usa
    local ok4, msg4 = ponerGancho(GANCHOS.prod, preProductor(pagColor + OFF_DATOS, G_AJUSTES))
    if not ok4 then anotar("gancho del productor NO puesto (se usa el método viejo): " .. tostring(msg4)) end
    estado = string.format("Activado: %d mando(s) extra. Elige su lado en esta pantalla y empieza un amistoso. (I = deshacer)", #extras)
end
local function deshacer()
    activo = false
    actualizarDatos()                                    -- el gancho deja de escribir antes de quitarlo
    quitarGancho(GANCHOS.prod)
    quitarAjustes("deshacer")
    restaurarColor()
    if parchado then
        local ok, msg = aplicarParches(true)
        parchado = not ok
        anotar("deshacer: " .. tostring(ok) .. (msg and (" · " .. msg) or ""))
    end
    activo = false
    estado = "Desactivado: código y ajustes originales restaurados (el mando 9 queda dado de alta hasta cerrar PES)."
end

-- ─── pantalla «Local / Visitante» — RÉPLICA EXACTA de la de PES (v0.6) ────────
-- La ventana de Sider muestra a pantalla completa una imagen (BMP 1920x1080) que este módulo arma con los
-- PÍXELES REALES de la pantalla de PES (piezas recortadas de una captura por herramientas\generar_replica_lados.ps1,
-- en content\phoenix\lados_replica\): fondo, cabecera Local/Visitante, filas de 90 px, mandos DualSense de color,
-- mando gris, flechas y botones de abajo. Con más de 8 mandos la tabla se desplaza (8 filas a la vista).
-- Se vuelve a dibujar solo cuando algo cambia (archivo nuevo cada vez, para que Sider recargue la textura).
-- Nombres opcionales: content\phoenix\mandos\nombres.txt con líneas «9=PEPE» (si no, «Usuario N» como PES).
local P = { W = 1920, H = 1080, VISIBLES = 8 }
local recursos, fb, fbN = nil, nil, 0
local sel, filas, nImagen, rutaImagen, rutaVieja, aviso, pendiente = 1, {}, 0, nil, nil, "", true
local desplaz = 0                                   -- primera fila visible (para más de 8 mandos)
local carpetaLados

local function leerBin(ruta)
    local f = io.open(ruta, "rb"); if not f then return nil end
    local s = f:read("*a"); f:close(); return s
end
local function aTabla0(s)                 -- sin ffi (vista previa fuera del juego): tabla con índice 0
    local t = {}
    for i = 1, #s do t[i - 1] = s:byte(i) end
    return t
end
local function cargarRecursos()
    if recursos then return recursos end
    -- medidas en texto simple (Sider no deja usar loadstring/pcall en los módulos)
    local txt = leerBin(carpetaLados .. "replica.txt"); if not txt then return nil end
    local med = { piezas = {}, letras = {}, flechas = { izq = {}, der = {} } }
    for linea in txt:gmatch("[^\r\n]+") do
        local t = {}
        for w in linea:gmatch("%S+") do t[#t + 1] = w end
        local k = t[1]
        local function n(i) return tonumber(t[i]) end
        if k == "pieza" then med.piezas[t[2]] = { n(3), n(4), n(5) }
        elseif k == "letra" then med.letras[n(2)] = { n(3), n(4), n(5) }
        elseif k == "cx" then med.cx = { n(2), n(3), n(4) }
        elseif k == "color_nombre" then med.color_nombre = { n(2), n(3), n(4) }
        elseif k == "izq" or k == "der" then
            for c = 1, 3 do local v = n(c + 1); if v and v > 0 then med.flechas[k][c] = v end end
        elseif k and n(2) then med[k] = n(2) end
    end
    local r = { med = med, fondo = leerBin(carpetaLados .. "fondo.raw"), fuente = leerBin(carpetaLados .. "fuente.raw"),
                piezas = leerBin(carpetaLados .. "piezas.raw"), pfondo = leerBin(carpetaLados .. "piezas_fondo.raw") }
    if not (r.fondo and r.fuente and r.piezas and r.pfondo) or #r.fondo ~= P.W * P.H * 4 then return nil end
    fbN = P.W * P.H * 4
    if ffi then
        r.pf = ffi.cast("const uint8_t*", r.fuente); r.pz = ffi.cast("const uint8_t*", r.piezas)
        r.pzf = ffi.cast("const uint8_t*", r.pfondo)
        fb = ffi.new("uint8_t[?]", fbN)
    else
        r.pf = aTabla0(r.fuente); r.pz = aTabla0(r.piezas); r.pzf = aTabla0(r.pfondo); r.tfondo = aTabla0(r.fondo)
        fb = {}
    end
    recursos = r
    return r
end

local floor = math.floor
local function pintar(i, c, a)            -- mezcla un color (r,g,b) con opacidad a (0–255) en el píxel i (BGRA)
    if a <= 0 then return end
    if a >= 255 then fb[i] = c[3]; fb[i + 1] = c[2]; fb[i + 2] = c[1]; return end
    local k = a / 255
    fb[i] = floor(fb[i] + (c[3] - fb[i]) * k + 0.5)
    fb[i + 1] = floor(fb[i + 1] + (c[2] - fb[i + 1]) * k + 0.5)
    fb[i + 2] = floor(fb[i + 2] + (c[1] - fb[i + 2]) * k + 0.5)
end
local function letras(s)                  -- UTF-8 → lista de códigos
    local t, i = {}, 1
    while i <= #s do
        local b = s:byte(i)
        if b < 0x80 then t[#t + 1] = b; i = i + 1
        elseif b < 0xE0 then t[#t + 1] = (b - 0xC0) * 64 + (s:byte(i + 1) - 0x80); i = i + 2
        else t[#t + 1] = ((b - 0xE0) * 64 + (s:byte(i + 1) - 0x80)) * 64 + (s:byte(i + 2) - 0x80); i = i + 3 end
    end
    return t
end
local function medir(s)
    local w, L = 0, recursos.med.letras
    for _, cp in ipairs(letras(s)) do local g = L[cp] or L[63]; w = w + (g[3] or (g[2] - 1)) end  -- [3] = avance con decimales
    return floor(w + 0.5)
end
local function texto(s, x, y, c, a)
    local med, pf = recursos.med, recursos.pf
    local AW, AH, L = med.atlas_ancho, med.atlas_alto, med.letras
    local xf = x
    for _, cp in ipairs(letras(s)) do
        local g = L[cp] or L[63]
        x = floor(xf + 0.5)
        for yy = 0, AH - 1 do
            local py = y + yy
            if py >= 0 and py < P.H then
                for xx = 0, g[2] - 1 do
                    local px = x + xx
                    local al = pf[yy * AW + g[1] + xx]
                    if al > 0 and px >= 0 and px < P.W then pintar((py * P.W + px) * 4, c, floor(al * a / 255)) end
                end
            end
        end
        xf = xf + (g[3] or (g[2] - 1))
    end
    return floor(xf + 0.5)
end
-- Pega una pieza real de PES en (x, y) trasladando su diferencia con el fondo limpio que tenía debajo
-- (fondo destino + (pieza − fondo origen)): queda idéntica aunque la textura de la tabla cambie de un sitio a otro.
local function pieza(nombre, x, y)
    local p = recursos.med.piezas[nombre]; if not p then return end
    local pz, pzf = recursos.pz, recursos.pzf
    local ini, w, h = p[1], p[2], p[3]
    for yy = 0, h - 1 do
        local o = ((y + yy) * P.W + x) * 4
        local s = (ini + yy * w) * 4
        for xx = 0, w - 1 do
            local i, j = o + xx * 4, s + xx * 4
            local b = fb[i] + pz[j] - pzf[j]
            local g = fb[i + 1] + pz[j + 1] - pzf[j + 1]
            local r = fb[i + 2] + pz[j + 2] - pzf[j + 2]
            fb[i] = b < 0 and 0 or (b > 255 and 255 or b)
            fb[i + 1] = g < 0 and 0 or (g > 255 and 255 or g)
            fb[i + 2] = r < 0 and 0 or (r > 255 and 255 or r)
        end
    end
end

-- Una fila como en PES: nombre + mando de color en su columna, mando gris en las otras dos, flechas ‹ › hacia
-- donde se puede mover (encendidas en la fila elegida, apagadas en las demás).
local function dibujarFila(v, f, elegida)
    local med = recursos.med
    local y = med.y0 + v * med.fila
    local col = f.lado                                         -- 0 Local · 1 centro · 2 Visitante
    for c = 0, 2 do
        -- como en PES: el mando gris del centro no sale en la fila del primer mando
        if c ~= col and not (c == 1 and f.idx == 0) then pieza("gris", med.cx[c + 1] - 47, y + med.mando_y) end
    end
    local cx = med.cx[col + 1]
    pieza("m" .. (f.idx % 16), cx - 47, y + med.mando_y)
    local nombre = f.nombre
    if f.lado ~= 1 and PHX.CAPITAN[CODIGO_LADO[f.lado]] == f.idx then nombre = nombre .. " CAP" end   -- v0.9.30
    while #nombre > 1 and medir(nombre) > 280 do nombre = nombre:sub(1, -2) end
    texto(nombre, cx - floor(medir(nombre) / 2), y + 8 - med.tope_mayuscula, med.color_nombre, 255)
    -- como en PES: flechas encendidas si el mando está en un lado, apagadas en el centro (v0.9: ya no hay «fila
    -- elegida»: cada mando mueve la suya, como en PES)
    local enc = (col ~= 1) and "_on" or "_off"
    if col > 0 then pieza("izq" .. enc, med.flechas.izq[col + 1] - 3, y + med.flecha_y) end
    if col < 2 then pieza("der" .. enc, med.flechas.der[col + 1] - 3, y + med.flecha_y) end
end

local function u32le(v) return string.char(v % 256, floor(v / 256) % 256, floor(v / 65536) % 256, floor(v / 16777216) % 256) end
-- v0.9: la imagen solo cubre la parte de arriba (la tabla termina en y = 854): la barra de botones de abajo es la DE
-- PES, que cambia según el estado («Editar Datos personales» desaparece con el mando 1 en el centro, etc.)
P.H_IMG = 870
P.TABLA = { x = 249, y = 47, w = 1422, h = 807 }      -- rectángulo de la tabla en 1920×1080 (medido en la captura 1.png)
P.ALFA_PARTIDO = 0.88                                 -- «un poco» translúcida en pleno partido
local function cabeceraBMP()
    local n = P.W * P.H_IMG * 4
    return "BM" .. u32le(54 + n) .. u32le(0) .. u32le(54) .. u32le(40) .. u32le(P.W) .. u32le(4294967296 - P.H_IMG)
        .. string.char(1, 0, 32, 0) .. u32le(0) .. u32le(n) .. u32le(2835) .. u32le(2835) .. u32le(0) .. u32le(0)
end
local function dibujarPantalla(ruta)
    local r = cargarRecursos(); if not r then return false end
    if ffi then ffi.copy(fb, r.fondo, fbN) else for i = 0, fbN - 1 do fb[i] = r.tfondo[i] end end
    -- con más de 8 mandos, la tabla se desplaza para que la fila elegida esté siempre a la vista
    if sel - 1 < desplaz then desplaz = sel - 1 end
    if sel > desplaz + P.VISIBLES then desplaz = sel - P.VISIBLES end
    for v = 0, P.VISIBLES - 1 do
        local f = filas[desplaz + v + 1]
        if f then dibujarFila(v, f, desplaz + v + 1 == sel) end
    end
    if #filas > P.VISIBLES then                               -- barra de desplazamiento fina, gris como la tabla
        local med = recursos.med
        local alto = P.VISIBLES * med.fila
        local barra = floor(alto * P.VISIBLES / #filas)
        local y1 = med.y0 + floor((alto - barra) * desplaz / (#filas - P.VISIBLES))
        for y = med.y0, med.y0 + alto - 1 do
            local dentro = y >= y1 and y < y1 + barra
            for x = 1658, 1663 do pintar((y * P.W + x) * 4, dentro and { 120, 122, 128 } or { 190, 191, 196 }, 255) end
        end
    end
    local f = io.open(ruta, "wb"); if not f then return false end
    -- v0.9.5: EN PARTIDO solo la tabla (sin bordes): alrededor se ve el partido y la tabla va un poco translúcida
    -- (image_alpha en overlay_on), como la pantalla de lados de PES en pleno partido (indicación de Fralex)
    if ffi and P.modoPartido then
        local R = P.TABLA
        local n = R.w * R.h * 4
        f:write("BM" .. u32le(54 + n) .. u32le(0) .. u32le(54) .. u32le(40) .. u32le(R.w) .. u32le(4294967296 - R.h)
            .. string.char(1, 0, 32, 0) .. u32le(0) .. u32le(n) .. u32le(2835) .. u32le(2835) .. u32le(0) .. u32le(0))
        local filasTxt = {}
        for y = 0, R.h - 1 do filasTxt[#filasTxt + 1] = ffi.string(fb + ((R.y + y) * P.W + R.x) * 4, R.w * 4) end
        f:write(table.concat(filasTxt)); f:close()
        return true
    end
    f:write(cabeceraBMP())
    local nImg = P.W * P.H_IMG * 4
    if ffi then f:write(ffi.string(fb, nImg))
    else
        local trozo = {}
        for i = 0, nImg - 1, 4096 do
            local n = math.min(4096, nImg - i)
            for j = 1, n do trozo[j] = fb[i + j - 1] end
            for j = n + 1, #trozo do trozo[j] = nil end
            f:write(string.char((table.unpack or unpack)(trozo)))
        end
    end
    f:close()
    return true
end

-- ─── v0.9: ESPEJO de la pantalla de lados de PES ──────────────────────────────
-- Medido en vivo (20-oct ~15:45): el objeto de la pantalla (lo guarda el gancho de su constructor en datos+0x30) tiene
-- en +0x90 … +0xC8 las 8 FILAS (una por mando, vftable 0x142715608): [fila+0x88]+0x114 = nº de mando (dueño), fila+0x98 = nº de «Datos
-- personales» (perfil), fila+0xA4 = lado (0 Local · 1 centro · 2 Visitante; mismo código que esta pantalla).
-- (todo lo de v0.9 va en la tabla V9: Lua admite como mucho 200 variables locales en el cuerpo del módulo)
local V9 = { SS_FILAS = 0x90, SS_VT_FILA = 0x142715608, FILA_MANDO = 0x9C, FILA_PERFIL = 0x98, FILA_LADO = 0xA4,
             PERFILES_PTR = { 0x143700A80, 0x1437F1078, 0x14351AAA0 }, PERFILES_OFF = 0x96FFF0, PERFIL_TAM = 0x980,
             PERFIL_N = 32, basePerfiles = nil, perfilesIntento = -100, DI_ESTADO = 0x238, dirPrevia = {}, vistoCentro = {} }
local function ladosNativos()
    local d = datos(); if not d or not PHX.pantallaPESAbierta() then return nil end   -- PES no la tiene abierta
    local obj = tonumber(ffi.cast("uint64_t*", d + 0x30)[0])
    if obj < 0x10000 then return nil end
    local t, n = {}, 0
    for k = 0, 7 do
        local fila = ptr(obj + V9.SS_FILAS + 8 * k)
        -- BLINDAJE: solo filas de verdad (vftable exacta) y valores en rango
        if fila and fila >= 0x10000 and ptr(fila) == V9.SS_VT_FILA then
            local pad, perfil, lado = u32(fila + V9.FILA_MANDO), u32(fila + V9.FILA_PERFIL), u8(fila + V9.FILA_LADO)
            -- v0.9.54 (medido en vivo 24-oct): el mando de la fila es su DUEÑO ([fila+0x88]+0x114, lo que usa PES en
            -- FUN_140E3D4D0). +0x9C no lo es: al volver de «Editar Datos personales» la fila del mando 1 tenía +0x9C = 10
            -- y la Phoenix la descartaba (por eso «se quedaba pegado» el mando 1)
            local w = ptr(fila + 0x88)
            if w and w >= 0x10000 then pad = u32(w + 0x114) or pad end
            if pad and pad < NUEVO and lado and lado <= 2 then t[pad] = { lado = lado, perfil = perfil, fila = fila }; n = n + 1 end
        end
    end
    if n == 0 then   -- v0.9.50 DIAGNÓSTICO: por qué no se lee la pantalla de PES (al volver de «Editar Datos personales»)
        local f0 = ptr(obj + V9.SS_FILAS)
        local txt = string.format("objeto %X fila0 %s vtabla %s mando %s lado %s", obj, f0 and string.format("%X", f0) or "nil",
            f0 and f0 >= 0x10000 and string.format("%X", ptr(f0) or 0) or "-", f0 and f0 >= 0x10000 and tostring(u32(f0 + V9.FILA_MANDO)) or "-",
            f0 and f0 >= 0x10000 and tostring(u8(f0 + V9.FILA_LADO)) or "-")
        if txt ~= V9.diagFilas then V9.diagFilas = txt; anotar("DIAG pantalla de PES sin filas legibles: " .. txt) end
    end
    return n > 0 and t or nil
end
-- «Datos personales» (los nombres FRALEX, ARENZ…): lista de 0x980 bytes por perfil, nombre en +0x14 (0x2C bytes,
-- UTF-8), +0x10 = 0/1. Medido en vivo: perfil i ↔ mando i; nombre vacío → PES pone «Usuario N». La lista ocupa SU
-- PROPIO bloque de memoria (cabecera: +8 = 0x4FFE0, el tamaño; los perfiles empiezan en +0x10); la dirección cambia en
-- cada arranque y no hay un puntero fijo corto (medido 20-oct ~16:20), así que se BUSCA una vez recorriendo las
-- regiones de memoria y se valida la lista entera (32 perfiles con forma correcta y al menos uno con nombre).
function V9.perfilesValidos(base)
    local llenos = 0
    for k = 0, V9.PERFIL_N - 1 do
        local r = leer(base + k * V9.PERFIL_TAM + 0x10, 0x30)
        if not r or r:byte(1) > 1 or r:byte(2) ~= 0 or r:byte(3) ~= 0 or r:byte(4) ~= 0 then return false end
        local fin = r:find("\0", 5, true)
        if not fin or r:sub(5, fin - 1):find("%c") then return false end
        if fin > 5 then llenos = llenos + 1 end
    end
    return llenos > 0
end
function V9.buscarPerfiles()
    local a, vistas, t0b = 0x10000, 0, os.clock()
    while a < 0x7FFFFFFF0000 and vistas < 200000 do
        if VQ(ffi.cast("const void*", a), mbi, ffi.sizeof(mbi)) == 0 then break end
        vistas = vistas + 1
        local base, tam = tonumber(mbi.BaseAddress), tonumber(mbi.RegionSize)
        if tam <= 0 then break end
        -- región comprometida, lectura/escritura (0x04), comienzo de su bloque y del tamaño justo
        -- v0.9.43: medido en vivo 24-oct, el bloque de la lista se parte en regiones de 0x4000 (la primera 0x04, otras
        -- 0x40): ya no se exige que la primera mida 0x14000 (la lista se valida entera igual). Por eso desde el 20-oct
        -- salía «NO encontrados» y todos se llamaban «Usuario N»
        local pr = tonumber(mbi.Protect)
        if tonumber(mbi.State) == 0x1000 and (pr == 0x04 or pr == 0x40) and base == tonumber(mbi.AllocationBase)
           and tam >= 0x1000 and u32(base + 8) == 0x4FFE0 and V9.perfilesValidos(base) then
            anotar(string.format("Datos personales encontrados en 0x%X (%d regiones miradas, %.0f ms)", base, vistas, (os.clock() - t0b) * 1000))
            return base
        end
        a = base + tam
    end
    anotar(string.format("Datos personales NO encontrados (%d regiones, %.0f ms): se usa «Usuario N»", vistas, (os.clock() - t0b) * 1000))
    return nil
end
local function nombrePerfil(perfil)
    -- BLINDAJE: la lista guardada se vuelve a validar (barato) antes de usarla; si dejó de valer, se busca otra vez
    -- (como mucho cada 30 s, para no recorrer la memoria en cada cuadro)
    if V9.basePerfiles and os.clock() - (V9.validadoT or -100) >= 2 then
        V9.validadoT = os.clock()
        if not V9.perfilesValidos(V9.basePerfiles) then V9.basePerfiles = nil end
    end
    -- v0.9.7: como mucho 3 búsquedas por sesión (en otro parche puede no existir la lista: no trabar cada 30 s)
    if not V9.basePerfiles and ahora() - V9.perfilesIntento >= 30 and (V9.busquedas or 0) < 3 then
        V9.busquedas = (V9.busquedas or 0) + 1
        V9.perfilesIntento = ahora(); V9.basePerfiles = V9.buscarPerfiles()
    end
    if not V9.basePerfiles or not perfil or perfil >= V9.PERFIL_N then return nil end
    local r = leer(V9.basePerfiles + perfil * V9.PERFIL_TAM + 0x14, 0x2C); if not r then return nil end
    local fin = r:find("\0", 1, true)
    local nombre = r:sub(1, (fin or 45) - 1)
    if nombre == "" or nombre:find("%c") then return nil end
    return nombre
end
-- Estado DirectInput de un mando extra (medido en vivo con un mando virtual de prueba, 20-oct ~15:55): plaza lógica
-- (HidMan +0xD0, idx×0x20) +0xC = fila de dispositivo → +0 objeto → [objeto+0x20] (empieza por el GUID) +0x238 =
-- DIJOYSTATE: X (0 izq · 0x7FFF centro · 0xFFFF der), Y, …, cruceta (+0x20, centésimas de grado; 0xFFFFFFFF suelta),
-- botones (+0x30, 0x80 = pulsado). Devuelve dx, dy en −1/0/1.
function V9.direccionExtra(e)
    local hid = ptr(G_HIDMAN); if not hid or hid == 0 then return 0, 0 end
    local tab = ptr(hid + 0xD0); if not tab or tab == 0 then return 0, 0 end
    local r = i32(tab + e.idx * 0x20 + 0xC); if not r or r < 0 or r >= 64 then return 0, 0 end
    -- v0.9.6 BLINDAJE: la fila de dispositivo tiene que estar enlazada a ESTA plaza (si no, es el mando de otro) y su
    -- GUID no puede ser el de ninguno de los mandos 1–8
    if i32(tab + r * 0x20 + 8) ~= e.idx then return 0, 0 end
    for k = 0, NUEVO - 1 do if leer(tab + k * 0x20 + 0x10, 16) == e.guid then return 0, 0 end end
    local dev = ptr(tab + r * 0x20); if not dev or dev < 0x10000 then return 0, 0 end
    local w = ptr(dev + 0x20); if not w or w < 0x10000 then return 0, 0 end
    if leer(w, 16) ~= e.guid then return 0, 0 end                -- BLINDAJE: es SU mando (mismo GUID)
    local s = copiar(w + V9.DI_ESTADO, 0x50); if not s then return 0, 0 end   -- v0.9.43: 0x50 (los botones están en +0x30; con 0x24 se leía fuera)
    local p = ffi.cast("uint32_t*", s)
    local x, y, pov = tonumber(p[0]), tonumber(p[1]), tonumber(p[8])
    V9.crudo = V9.crudo or {}
    V9.crudo[e.idx] = string.format("X %04X Y %04X cruceta %d", x, y, pov)    -- v0.9.17: para el registro
    -- v0.9.29: L2/R2 para elegir «Datos personales». Mando virtual tipo Xbox por DirectInput: LB = botón 4, RB = botón 5,
    -- gatillos = eje Z (+0x08: < 0x3000 uno, > 0xD000 el otro). −1 = anterior, +1 = siguiente, 0 = nada.
    local z, b = tonumber(p[2]), ffi.cast("uint8_t*", s)
    -- v0.9.32 (medido en vivo 23-oct en el mando 16 de Link: □0 ✕1 ○2 △3 L1 4 R1 5 L2 6 R2 7 L3 8): L2/R2 = botones 6/7,
    -- como en PES («L2/R2 Elige Datos personales»). En la v0.9.29 eran 4/5, que en estos mandos son L1/R1.
    local bl, br = b[0x30 + 6] >= 0x80, b[0x30 + 7] >= 0x80
    -- v0.9.43 (24-oct: el mismo mando 16 ahora da ○ = botón 1, distribución tipo Xbox): ahí L2/R2 son el eje Z, así que
    -- también valen los gatillos por Z (como decía la v0.9.29)
    if not bl and not br and z then bl, br = z > 0xD000, z < 0x3000 end
    V9.perfilDir = V9.perfilDir or {}
    V9.perfilDir[e.idx] = bl and -1 or (br and 1 or 0)
    -- v0.9.8 BLINDAJE: si una persona ocupa dos puestos (p. ej. mando 1 y puesto 15 de Link, visto en vivo 20-oct),
    -- el extra se mueve IGUAL que un mando 1–8: entonces se ignora (lo mueve PES con el otro mando)
    -- v0.9.10: la comparación con los mandos 1–8 se DESACTIVA: medido en vivo, el puesto 15 de Link tenía la palanca
    -- TRABADA a la derecha; al mover el mando 1 a la derecha «coincidía», se ignoraba, y al soltar contaba como un
    -- movimiento nuevo → la fila 15 se movía con el mando 1. Ahora manda la regla de «pasar por el centro» (moverExtras).
    if false then
        local firma = x .. "/" .. y .. "/" .. pov
        for _, fn in ipairs(V9.firmasNativas or {}) do
            if fn == firma then
                if not V9.avisoEspejo then V9.avisoEspejo = {} end
                if not V9.avisoEspejo[e.idx] then
                    V9.avisoEspejo[e.idx] = true
                    anotar(string.format("mando %d se mueve igual que uno de los mandos 1–8 (misma persona en dos puestos de Link): se ignora", e.idx + 1))
                end
                return 0, 0
            end
        end
    end
    -- v0.9.18 BLINDAJE (registro del 23-oct): el mando 9 se fue solo a Local con «X 0000 Y 0000 cruceta 0» = estado
    -- todo a cero (mando virtual sin datos todavía): no es una lectura real. Y el 16 se fue a Visitante con X DEDE (palanca
    -- algo torcida y suelta): el umbral pasa de 0x3000/0xD000 a 0x1000/0xF000 (un empujón de verdad llega a 0/FFFF).
    -- v0.9.21: también «X 0000 Y 0000» con la cruceta suelta (mando 12, 23-oct 06:50): X e Y a cero a la vez es
    -- un mando virtual sin datos, nunca una palanca real (una palanca real en la esquina no da 0 exacto en los dos ejes)
    if x == 0 and y == 0 then return 0, 0, false end
    local dx = (x < 0x1000) and -1 or ((x > 0xF000) and 1 or 0)
    local dy = (y < 0x1000) and -1 or ((y > 0xF000) and 1 or 0)
    if pov <= 36000 then
        if pov >= 4500 and pov <= 13500 then dx = 1 elseif pov >= 22500 and pov <= 31500 then dx = -1 end
        if pov <= 4500 or pov >= 31500 then dy = -1 elseif pov >= 13500 and pov <= 22500 then dy = 1 end
    end
    return dx, dy, true                                    -- v0.9.11: true = lectura real
end

-- Filas: los 8 mandos de settings.dat (conectado = su GUID está entre los dispositivos del juego) y los extra.
local function nombresGuardados()
    local t, s = {}, leerBin(carpeta .. "nombres.txt")
    if s then for n, nombre in s:gmatch("(%d+)%s*=%s*([^\r\n]+)") do t[tonumber(n)] = nombre end end
    return t
end
local function construirFilas()
    local nombres = nombresGuardados()
    local conect, nExtra, codigos = {}, #extras, nil
    preparar()
    local hid = ptr(G_HIDMAN)
    if hid and hid ~= 0 then
        local idxDe, gi = {}, ptr(hid + 0xD0)
        if gi then for i = 0, 7 do local g = leer(gi + i * 0x20 + 0x10, 16); if g then idxDe[g] = i end end end
        local libres = 0
        for _, d in ipairs(mandosDirectInput()) do
            if d.tipo == 1 and d.vivo then
                if idxDe[d.guid] then conect[idxDe[d.guid]] = true else libres = libres + 1 end
            end
        end
        -- v0.9.4: sin activar NO se muestran filas 9+ (antes salían filas que no se podían mover)
        if not activo then nExtra = 0 end
    end
    local s = direccionAjustes()
    if s then codigos = leer(s + AJ_LADO, NUEVO) end
    local nuevas = {}
    -- v0.8.10: con el módulo activo, los extra son EXACTAMENTE las plazas activadas (numeración de Link: puede haber
    -- huecos, p. ej. 9, 10, 12…); sin activar, se muestran como antes (contiguas)
    local esExtra = {}
    if activo then for _, e in ipairs(extras) do esExtra[e.idx] = true end
    else for i = NUEVO, NUEVO + nExtra - 1 do esExtra[i] = true end end
    local nat = ladosNativos()                           -- v0.9: lo que muestra AHORA la pantalla de PES
    for i = 0, MAX_MANDOS - 1 do
      if i < NUEVO or esExtra[i] then
        local lado = plan[i]
        if i < NUEVO and nat and nat[i] then lado = nat[i].lado end
        if lado == nil and i < NUEVO and codigos then lado = LADO_DE_CODIGO[codigos:byte(i + 1)] end
        -- v0.9: como en PES, un mando empieza en el CENTRO (sin jugar) la primera vez y luego donde se dejó
        if lado == nil then lado = 1 end
        -- v0.9.21 (23-oct, «esos mandos se pusieron ahí automáticamente»): fuera del partido, cada pantalla de lados
        -- NUEVA empieza con los extra en el CENTRO; recordar el lado arrastraba elecciones viejas o lecturas falsas
        -- (v0.9.21 los ponía al centro en cada pantalla nueva; v0.9.27 lo quita: Fralex quiere que conserven su lado,
        -- como los mandos 1–8; las lecturas vacías que los movían solos ya se ignoran desde la v0.9.21)
        if i >= NUEVO then
            -- v0.9.17 DIAGNÓSTICO: un extra que aparece ya en un lado (recordado de la vez anterior)
            if lado ~= 1 and V9.ladoAnotado ~= nil and V9.ladoAnotado[i] ~= lado then
                anotar(string.format("fila del mando %d empieza en el lado %d (recordado de la vez anterior)", i + 1, lado))
            end
            V9.ladoAnotado = V9.ladoAnotado or {}; V9.ladoAnotado[i] = lado
            plan[i] = lado
        end
        -- como en PES: solo hay fila para los mandos conectados (los extra solo existen si se detectaron)
        if i >= NUEVO or conect[i] then
            local nombre
            -- v0.9.2: los extra también llevan el «Datos personales» de su número, como el propio PES en el partido
            -- (medido 20-oct ~16:40: el mando 9 juega como MORGADO = perfil 8, el 12 como BETO HORSEMAN = perfil 11);
            -- nombres.txt sigue mandando si alguien pone un nombre a mano
            if i < NUEVO then nombre = nombrePerfil(nat and nat[i] and nat[i].perfil or i)
            else nombre = nombres[i + 1] or nombrePerfil(PHX.PERFIL_DE[i] or i) end
            nuevas[#nuevas + 1] = { idx = i, lado = lado, nombre = nombre or ("Usuario " .. (i + 1)), conectado = true }
        end
      end
    end
    filas = nuevas
    actualizarDatos()                                     -- v0.9.13: lo que se ve = lo que irá al partido
    desplaz = math.max(0, math.min(desplaz, #filas - P.VISIBLES))   -- v0.9.7: sin huecos si hay menos filas
    if sel > #filas then sel = #filas end
    if sel < 1 then sel = 1 end
end

local function refrescar(msg)
    if msg then aviso = msg end
    pendiente = true
end
local function moverSel(d) if #filas > 0 then sel = (sel - 1 + d) % #filas + 1; refrescar() end end
-- v0.9: con más de 8 filas, ↑/↓ (teclado o cruceta de un mando extra) desplazan la tabla una fila; al moverse un
-- mando, su fila se pone a la vista (sel solo sirve ya para eso: no se dibuja ninguna «fila elegida»)
function V9.desplazar(d)
    if #filas <= P.VISIBLES then return end
    local nuevo = math.max(0, math.min(#filas - P.VISIBLES, desplaz + d))
    if nuevo ~= desplaz then
        desplaz = nuevo
        sel = (d > 0) and (desplaz + P.VISIBLES) or (desplaz + 1)
        refrescar()
    end
end
function V9.aLaVista(f) for k, g in ipairs(filas) do if g == f then sel = k end end end
-- Cada cuadro: copia de la pantalla de PES el lado y el nombre de los mandos 1–8
function V9.reflejarNativos()
    local nat = ladosNativos()
    -- v0.9.49 DIAGNÓSTICO (24-oct, Fralex: al volver de «Editar Datos personales» la fila del mando 1 se queda pegada):
    -- se anota si deja de leerse la pantalla de PES y cada cambio de usuario del mando 1
    local diag = nat and nat[0] and string.format("fila %X perfil %s", nat[0].fila or 0, tostring(nat[0].perfil)) or "sin pantalla de PES legible"
    if diag ~= V9.diagNat then V9.diagNat = diag; anotar("DIAG mando 1 en la pantalla de PES: " .. diag) end
    if not nat then return end
    for _, f in ipairs(filas) do
        local n = (f.idx < NUEVO) and nat[f.idx] or nil
        if n then
            -- v0.9.6: sin saltar la tabla hacia esa fila (en vivo hacía volver la vista arriba al mover el mando 1)
            if n.lado ~= f.lado then
                V9.ultimoNativo = { t = os.clock(), d = (n.lado > f.lado) and 1 or -1 }   -- v0.9.10: para coincidencias
                f.lado = n.lado; plan[f.idx] = n.lado; refrescar()
                V9.revisarCoincidencia()
            end
            local nombre = nombrePerfil(n.perfil) or ("Usuario " .. (f.idx + 1))
            if nombre ~= f.nombre then f.nombre = nombre; refrescar() end
        end
    end
end
-- Cada cuadro: los mandos 9–16 mueven SU fila con su palanca o cruceta (un paso por pulsación, como en PES)
-- v0.9.10: MISMA PERSONA EN DOS SITIOS (medido en vivo: el mando físico del anfitrión mueve el mando 1 de PES y a la vez
-- llega por Link al puesto 15). Si una fila extra se mueve en el mismo momento (±0,35 s) y hacia el mismo lado que una
-- fila 1–8, dos veces, ese extra queda «gemelo»: se ignora y se devuelve al centro. Se anota en el registro.
V9.gemelo, V9.coinciden = {}, {}
function V9.revisarCoincidencia()
    local n, x = V9.ultimoNativo, V9.ultimoExtra
    if not n or not x or n.usado or x.usado then return end
    if math.abs(n.t - x.t) <= 0.35 and n.d == x.d then
        n.usado, x.usado = true, true
        V9.coinciden[x.idx] = (V9.coinciden[x.idx] or 0) + 1
        if V9.coinciden[x.idx] >= 3 and not V9.gemelo[x.idx] then
            V9.gemelo[x.idx] = true
            anotar(string.format("mando %d se mueve a la vez que uno de los mandos 1–8 (misma persona en dos puestos): se ignora", x.idx + 1))
            for _, f in ipairs(filas) do
                if f.idx == x.idx then f.lado = 1; plan[f.idx] = 1; actualizarDatos(); refrescar() end
            end
        end
    end
end
-- v0.9.30: botón n (0–31) del mando de la plaza i (cualquier mando por DirectInput: los virtuales de Link, 1–16).
-- nil si no se puede leer (p. ej. el mando físico del anfitrión, que no va por DirectInput).
function V9.boton(i, n)
    local hid = ptr(G_HIDMAN); if not hid or hid == 0 then return nil end
    local tab = ptr(hid + 0xD0); if not tab or tab == 0 then return nil end
    local r = i32(tab + i * 0x20 + 0xC); if not r or r < 0 or r >= 64 then return nil end
    if i32(tab + r * 0x20 + 8) ~= i then return nil end
    local dev = ptr(tab + r * 0x20); if not dev or dev < 0x10000 then return nil end
    local w = ptr(dev + 0x20); if not w or w < 0x10000 then return nil end
    local b = u8(w + V9.DI_ESTADO + 0x30 + n); if not b then return nil end
    return b >= 0x80
end
-- v0.9.30 CAPITÁN por botón (v0.9.31: L3 = botón 8 en un mando tipo Xbox por DirectInput): el primero que lo pulsa en su lado es
-- capitán; si el capitán lo vuelve a pulsar, lo deja. Otro no se lo quita (el anfitrión sí, con F5/F6).
V9.r3Previo = {}
function V9.capitanPorBoton()
    for _, f in ipairs(filas) do
        local p = V9.boton(f.idx, 8)                    -- v0.9.31: L3 (botón 8), pedido de Fralex; R3 se usa en PES
        if p and not V9.r3Previo[f.idx] and f.lado ~= 1 then
            local codigo = CODIGO_LADO[f.lado]
            if PHX.CAPITAN[codigo] == f.idx then PHX.CAPITAN[codigo] = nil; anotar(string.format("mando %d deja de ser capitán", f.idx + 1))
            elseif PHX.CAPITAN[codigo] == nil then PHX.CAPITAN[codigo] = f.idx; anotar(string.format("mando %d es capitán de %s", f.idx + 1, codigo == 1 and "Local" or "Visitante")) end
            refrescar()
        end
        V9.r3Previo[f.idx] = p
    end
    -- si el capitán se cambió de lado, deja de serlo
    for codigo, idx in pairs(PHX.CAPITAN) do
        local sigue = false
        for _, f in ipairs(filas) do if f.idx == idx and f.lado ~= 1 and CODIGO_LADO[f.lado] == codigo then sigue = true end end
        if not sigue then PHX.CAPITAN[codigo] = nil end
    end
end
-- v0.9.33 (23-oct, Fralex: «en pausa, en Local/Visitante, con un mando extra no me deja volver atrás con círculo»): la
-- pantalla de lados de PES solo escucha a los mandos 1–8. Cuando un mando 9+ pulsa ✕ u ○, se pulsa la tecla del teclado
-- que PES tiene para esa acción (settings.dat, casilla del teclado: ✕ = Z, ○ = C; mismo orden que en los mandos).
-- Se baja la tecla al pulsar y se suelta al soltar (PES lee el teclado por cuadros: un toque instantáneo se perdería).
V9.botonPrevio = {}
-- v0.9.34: medido por Fralex en su PES: Z = atrás (○), C = Editar Datos personales (□); ✕ = X (confirmar)
local TECLAS_DE_BOTON = { { b = 1, vk = 0x58, scan = 0x2D }, { b = 2, vk = 0x5A, scan = 0x2C } }   -- ✕ → X · ○ → Z
function V9.botonesAPes()
    do return end   -- v0.9.37: DESACTIVADO. Con el foco a 16 (v0.9.36) los menús ya oyen a los mandos 9+; la tecla
    -- (que PES toma como el mando 1) hacía que ○ del mando 9 actuara sobre el equipo del mando 1 en la estrategia.
    for _, e in ipairs(extras) do                                          -- v0.9.34: todos los extra (también en la pausa)
        local f = e
        do
            for _, t in ipairs(TECLAS_DE_BOTON) do
                local k = f.idx * 32 + t.b
                local p = V9.boton(f.idx, t.b)
                if p and not V9.botonPrevio[k] then ffi.C.phxf1_tecla(t.vk, t.scan, 0, 0); anotar(string.format("mando %d pulsa %s → tecla para PES", f.idx + 1, t.b == 1 and "✕" or "○"))
                elseif not p and V9.botonPrevio[k] then ffi.C.phxf1_tecla(t.vk, t.scan, 2, 0) end
                V9.botonPrevio[k] = p and true or nil
            end
        end
    end
end
-- v0.9.38 (23-oct, Fralex: en pausa › Local/Visitante el ○ de un mando 9+ no vuelve atrás; los 1–8 sí).
-- CAUSA (Ghidra, informes/datos/rea/atras_01…11): cada ventana de menú lee SU mando dueño (+0x88→+0x114) en
-- FUN_140E3C810; con el código 1 (○) pone el sonido 2 y llama a vtable+0x190. En menu::SideSelect (vtable 0x142673558)
-- solo las 8 FILAS SideSelectEach (+0x90…+0xC8, vtable 0x142715608, dueño = mando 0–7) tienen ese método:
-- FUN_140C68350 → si FUN_140ABFA40(padre) lo permite, manda al padre el mensaje 0x1081002 (FUN_1414A0610) y el padre
-- (FUN_140AC0700) hace «return». El objeto es de 0x118 con 8 filas fijas: un mando 9+ no tiene fila → nadie oye su ○.
-- ARREGLO: al ver ○ (botón 2, flanco) de un mando 9+ con la pantalla de PES abierta, se pone el mismo sonido y se llama a
-- FUN_140C68350 sobre una fila válida del objeto guardado (datos+0x30). Comprobaciones: pantalla abierta (contadores),
-- vtables del objeto (+0 y +0x70), fila con vtable de fila, padre (+0x38) = objeto, vtable+0x190 = FUN_140C68350 y
-- bytes de FUN_140C68350 iguales a los del exe. Si algo no cuadra, no se llama a nada (solo se anota).
V9.SS_VT, V9.SS_VT2, V9.ATRAS_FILA, V9.SONIDO = 0x142673558, 0x1426737B0, 0x140C68350, 0x1436F9A88
V9.ATRAS_ORIG = "40534883ec4048c7442420feffffff488bd9488b4938e8d576e5ff84c0750c33d2488bcbe807951d00eb2c488d051ef39401"   -- 93 bytes
             .. "4889442428c744243002100801c64424340048895c2438488d542428488bcbe86a828300904883c4405bc3"            -- (PES2021.exe)
V9.circuloPrevio, V9.cuadroAtras, V9.ultimoAtras = {}, 0, -100
-- v0.9.52 □ DE UN MANDO 9+ = «Editar Datos personales» de SU perfil (24-oct, Fralex: con el mando 16 no pasaba nada).
-- Ghidra (informes/datos/rea/cuadrado_01…03): en la fila SideSelectEach, □ (código 2 → vtable+0x198) es FUN_140C68470:
--   si [fila+0x88]+0xA0 == fila+0x94 → DAT_1434AC580 = perfil a editar; FUN_1414BF2D0(FUN_1414BEA50(), mando dueño)
--   (apila el cursor del editor para ese mando); manda al padre el TaskMessage 0x1081003 (FUN_1414A0610) y el sonido
--   FUN_140E41880(fila, 0x3D). Un mando 9+ no tiene fila: se hace lo mismo con una fila válida pero con SU perfil y SU
--   número de mando (el foco ya admite 16). Antes se comprueba que los bytes de FUN_140C68470 son los del exe.
V9.CUAD_FILA, V9.CUAD_PERFIL, V9.CUAD_MSG_VT = 0x140C68470, 0x1434AC580, 0x1425B76A0
V9.CUAD_ORIG = "40574883ec4048c7442420feffffff48895c2450488bf9e8a44c1d003b8794000000755e"   -- primeros 36 bytes (PES2021.exe)
V9.cuadradoPrevio = {}
function V9.cuadradoExtras()
    local abierta = PHX.pantallaPESAbierta()   -- BLINDAJE: solo con la pantalla de lados de PES abierta
    for _, e in ipairs(extras) do
        local p = V9.boton(e.idx, 2)                       -- □ en la distribución tipo Xbox (X = botón 2), como el ○ = 1
        if p and abierta and V9.cuadradoPrevio[e.idx] == false then V9.editarPerfilDe(e.idx) end
        if p ~= nil then V9.cuadradoPrevio[e.idx] = p end
    end
end
function V9.editarPerfilDe(idx)
    local fila, porque = V9.filaParaAtras()
    if not fila then anotar("□ del mando " .. (idx + 1) .. ": " .. tostring(porque)); return end
    if ptr(V9.SS_VT_FILA + 0x198) ~= V9.CUAD_FILA then anotar("□: vtable de fila distinta → no se hace nada"); return end
    if V9.cuadCodigoOk == nil then
        local b = leer(V9.CUAD_FILA, #V9.CUAD_ORIG / 2)
        V9.cuadCodigoOk = (b ~= nil and bin2hex(b) == V9.CUAD_ORIG)
        if not V9.cuadCodigoOk then anotar("□ de mandos 9+: FUN_140C68470 no coincide con el exe → desactivado (" .. (b and bin2hex(b) or "nil") .. ")") end
    end
    if not V9.cuadCodigoOk then return end
    local w = ptr(fila + 0x88)
    if not w or w < 0x10000 or u32(w + 0xA0) ~= u32(fila + 0x94) then anotar("□ del mando " .. (idx + 1) .. ": la pantalla no está en la página de filas"); return end
    local perfil = PHX.PERFIL_DE[idx] or idx
    memory.write(V9.CUAD_PERFIL, u32le(perfil))
    local gestor = ffi.cast("uint64_t(*)(void)", 0x1414BEA50)()
    ffi.cast("void(*)(uint64_t, uint32_t)", 0x1414BF2D0)(gestor, idx)
    V9.msgCuad = V9.msgCuad or ffi.new("uint64_t[3]")
    V9.msgCuad[0] = V9.CUAD_MSG_VT
    V9.msgCuad[1] = 0x1081003                              -- código (+8) y el byte +0xC = 0
    V9.msgCuad[2] = fila                                   -- remitente
    ffi.cast("void(*)(uint64_t, void*)", 0x1414A0610)(fila, V9.msgCuad)
    ffi.cast("void(*)(uint64_t, uint32_t)", 0x140E41880)(fila, 0x3D)
    V9.editandoPerfil = ahora()                               -- v0.9.55: el cierre que viene NO es confirmar
    anotar(string.format("□ del mando %d → «Editar Datos personales» de su perfil %d (%s)", idx + 1, perfil + 1, nombrePerfil(perfil) or "sin nombre"))
end
function V9.dispositivo(i)                                  -- objeto DirectInput del mando de la plaza i (o nil)
    local hid = ptr(G_HIDMAN); if not hid or hid == 0 then return nil end
    local tab = ptr(hid + 0xD0); if not tab or tab == 0 then return nil end
    local r = i32(tab + i * 0x20 + 0xC); if not r or r < 0 or r >= 64 then return nil end
    if i32(tab + r * 0x20 + 8) ~= i then return nil end
    local dev = ptr(tab + r * 0x20); if not dev or dev < 0x10000 then return nil end
    return dev
end
function V9.filaParaAtras()
    local d = datos(); if not d then return nil, "sin datos" end
    local obj = tonumber(ffi.cast("uint64_t*", d + 0x30)[0])
    if obj < 0x10000 or ptr(obj) ~= V9.SS_VT or ptr(obj + 0x70) ~= V9.SS_VT2 then return nil, "el objeto guardado no es un menu::SideSelect vivo" end
    if ptr(V9.SS_VT_FILA + 0x190) ~= V9.ATRAS_FILA then return nil, "vtable de fila distinta" end
    if V9.atrasCodigoOk == nil then
        local b = leer(V9.ATRAS_FILA, #V9.ATRAS_ORIG / 2)
        V9.atrasCodigoOk = (b ~= nil and bin2hex(b) == V9.ATRAS_ORIG)
        if not V9.atrasCodigoOk then anotar("○ de mandos 9+: FUN_140C68350 no coincide con el exe → desactivado") end
    end
    if not V9.atrasCodigoOk then return nil, "código distinto al del exe" end
    local elegida
    for k = 0, 7 do
        local fila = ptr(obj + V9.SS_FILAS + 8 * k)
        if fila and fila >= 0x10000 and ptr(fila) == V9.SS_VT_FILA and ptr(fila + 0x38) == obj then
            local w = ptr(fila + 0x88)
            if w and w >= 0x10000 and (not elegida or (u8(fila + 0x90) == 1 and u8(elegida + 0x90) ~= 1)) then elegida = fila end
        end
    end
    if not elegida then return nil, "ninguna fila válida" end
    return elegida, obj
end
function V9.atrasExtras()
    V9.cuadroAtras = V9.cuadroAtras + 1
    local abierta = PHX.pantallaPESAbierta()
    local quien
    for _, e in ipairs(extras) do
        local p = V9.boton(e.idx, 1)   -- v0.9.40: medido en vivo, el ○ del mando 16 llega como botón 1 de DirectInput (tipo Xbox: B), no 2
        -- flanco: solo si antes se le vio SUELTO (false); nil = no legible / recién abierta → no cuenta
        if p and V9.circuloPrevio[e.idx] == false and not quien then quien = e.idx end
        if p and V9.circuloPrevio[e.idx] ~= false and V9.circuloPrevio[e.idx] ~= true then anotar(string.format("○ del mando %d visto sin estado previo", e.idx + 1)) end
        if p == nil then V9.circuloPrevio[e.idx] = nil else V9.circuloPrevio[e.idx] = p end
    end
    if not quien then return end
    -- v0.9.39 DIAGNÓSTICO: en vivo (v0.9.38) el ○ no hacía nada y no se anotaba nada: se anota cada salida
    if not abierta then anotar(string.format("○ del mando %d: la pantalla de lados de PES no consta abierta (contadores %d/%d)", quien + 1, contadorDatos(0x24), contadorDatos(0x28))); return end
    if V9.cuadroAtras - V9.ultimoAtras < 15 then anotar("○ ignorado: demasiado seguido"); return end            -- como mucho un «atrás» cada ~¼ s
    -- si ese mando físico es también uno de los 1–8 (mismo dispositivo), PES ya oye su ○: no se duplica
    local dev = V9.dispositivo(quien)
    for k = 0, NUEVO - 1 do if dev and V9.dispositivo(k) == dev then anotar(string.format("○ del mando %d: es el mismo dispositivo que el mando %d → lo oye PES", quien + 1, k + 1)); return end end
    local fila, obj = V9.filaParaAtras()
    if not fila then anotar(string.format("○ del mando %d: no se vuelve atrás (%s)", quien + 1, tostring(obj))); return end
    V9.ultimoAtras = V9.cuadroAtras
    local snd = ptr(V9.SONIDO)                                         -- SoundManager: sonido 2 (= el de ○ de PES)
    if snd and snd >= 0x10000 then memory.write(snd + 0x9E + 2, "\1") end
    ffi.cast("phxf1_m_v", V9.ATRAS_FILA)(ffi.cast("void*", fila))
    anotar(string.format("mando %d pulsa ○ → «atrás» en la pantalla de lados de PES (fila %X del objeto %X)", quien + 1, fila, obj))
end
-- v0.9.60 (26-oct, Fralex: los mandos 9+ solo tenían ○ en la pantalla de lados: ○ con cambios los DESHACE, así que el
-- mando 1 volvía al centro). ✕ de un mando 9+ = ✕ de la fila: FUN_140C683B0 (vtable de fila +0x188) manda al padre el
-- mensaje 0x1081001 (confirmar), como el ✕ de los mandos 1–8. Mismas comprobaciones que el ○.
V9.equisPrevio, V9.ultimoEquis = {}, -100
function V9.confirmarExtras()
    local abierta = PHX.pantallaPESAbierta()
    local quien
    for _, e in ipairs(extras) do
        local p = V9.boton(e.idx, 0)                     -- ✕ = botón 0 (distribución tipo Xbox: A), como ○ = 1
        if p and V9.equisPrevio[e.idx] == false and not quien then quien = e.idx end
        if p == nil then V9.equisPrevio[e.idx] = nil else V9.equisPrevio[e.idx] = p end
    end
    if not quien or not abierta then return end
    if V9.cuadroAtras - V9.ultimoEquis < 15 then return end
    local dev = V9.dispositivo(quien)
    for k = 0, NUEVO - 1 do if dev and V9.dispositivo(k) == dev then return end end   -- mismo mando físico que un 1–8: ya lo oye PES
    if ptr(V9.SS_VT_FILA + 0x188) ~= 0x140C683B0 or bin2hex(leer(0x140C683B0, 18) or "") ~= "40534883ec4048c7442420feffffff488bd9" then
        anotar("✕ de mandos 9+: FUN_140C683B0 distinta al exe → desactivado"); return end
    local fila, obj = V9.filaParaAtras()
    if not fila then anotar(string.format("✕ del mando %d: no se confirma (%s)", quien + 1, tostring(obj))); return end
    V9.ultimoEquis = V9.cuadroAtras
    ffi.cast("phxf1_m_v", 0x140C683B0)(ffi.cast("void*", fila))
    anotar(string.format("mando %d pulsa ✕ → «confirmar» en la pantalla de lados de PES (fila %X)", quien + 1, fila))
end
function V9.moverExtras()
    V9.capitanPorBoton()                                   -- v0.9.30
    V9.botonesAPes()                                       -- v0.9.33
    local extraDe, yaDesplazo = {}, false
    -- estado actual (X/Y/cruceta) de los mandos 1–8, para reconocer un extra que es la misma persona
    V9.firmasNativas = {}
    local hid = ptr(G_HIDMAN)
    local tab = (hid and hid ~= 0) and ptr(hid + 0xD0) or nil
    if false and tab and tab ~= 0 then                    -- v0.9.11: sin uso (comparación desactivada en 0.9.10)
        for k = 0, NUEVO - 1 do
            local r = i32(tab + k * 0x20 + 0xC)
            local dev = (r and r >= 0 and r < 64) and ptr(tab + r * 0x20) or nil
            local w = (dev and dev >= 0x10000) and ptr(dev + 0x20) or nil
            local s = (w and w >= 0x10000) and copiar(w + V9.DI_ESTADO, 0x24) or nil
            if s then
                local p = ffi.cast("uint32_t*", s)
                V9.firmasNativas[#V9.firmasNativas + 1] = tonumber(p[0]) .. "/" .. tonumber(p[1]) .. "/" .. tonumber(p[8])
            end
        end
    end
    for _, e in ipairs(extras) do extraDe[e.idx] = e end
    for _, f in ipairs(filas) do
        local e = extraDe[f.idx]
        if e then
            local dx, dy, real = V9.direccionExtra(e)
            local prev = V9.dirPrevia[f.idx] or { 0, 0 }
            -- v0.9.10 BLINDAJE: un mando solo cuenta después de haberlo visto EN EL CENTRO desde que se abrió la pantalla
            -- (un puesto de Link con la palanca trabada o una lectura rara nunca mueve su fila solo)
            if real and dx == 0 and dy == 0 then V9.vistoCentro[f.idx] = true end
            if not V9.vistoCentro[f.idx] or V9.gemelo[f.idx] then dx, dy = 0, 0 end
            if dx ~= 0 and prev[1] == 0 then
                local l = math.max(0, math.min(2, f.lado + dx))
                -- v0.9.9: tampoco salta la vista (en vivo «me jala hasta la última fila»)
                if l ~= f.lado then
                    -- v0.9.17 DIAGNÓSTICO (23-oct, «algunos mandos se ponen a un lado solos»): quién movió la fila y con qué lectura
                    anotar(string.format("fila del mando %d → lado %d por su palanca (%s)", f.idx + 1, l, (V9.crudo or {})[f.idx] or "?"))
                    f.lado = l; plan[f.idx] = l; actualizarDatos(); refrescar()
                    V9.ultimoExtra = { t = os.clock(), d = dx, idx = f.idx }
                    V9.revisarCoincidencia()
                end
            end
            -- v0.9.29: L2/R2 → «Datos personales» anterior/siguiente (32 perfiles, como en PES); solo al pulsar (flanco)
            local pd = (V9.perfilDir or {})[f.idx] or 0
            if pd ~= 0 and (V9.perfilPrev or {})[f.idx] ~= pd and V9.vistoCentro[f.idx] then
                local actual = PHX.PERFIL_DE[f.idx] or f.idx
                -- v0.9.48 (24-oct, Fralex: «hay mandos repetidos»): como en PES, se saltan los perfiles que ya usa otro
                -- mando (los 1–8 según su fila de PES; los 9+ según lo elegido aquí)
                local usado, nat = {}, ladosNativos()
                for _, g in ipairs(filas) do
                    if g.idx ~= f.idx then
                        local pg = (g.idx < NUEVO) and (nat and nat[g.idx] and nat[g.idx].perfil) or (PHX.PERFIL_DE[g.idx] or g.idx)
                        if pg then usado[pg] = true end
                    end
                end
                local nuevo = actual
                for _ = 1, 32 do nuevo = (nuevo + pd) % 32; if not usado[nuevo] then break end end
                PHX.PERFIL_DE[f.idx] = nuevo
                f.nombre = nombrePerfil(PHX.PERFIL_DE[f.idx]) or ("Usuario " .. (PHX.PERFIL_DE[f.idx] + 1))
                anotar(string.format("mando %d elige «Datos personales» %d (%s)", f.idx + 1, PHX.PERFIL_DE[f.idx] + 1, f.nombre))
                actualizarDatos(); refrescar()
            end
            V9.perfilPrev = V9.perfilPrev or {}; V9.perfilPrev[f.idx] = pd
            -- v0.9.7: un solo desplazamiento por cuadro y solo con arriba/abajo «puro» (en diagonal se cambia de lado)
            if dy ~= 0 and dx == 0 and prev[2] == 0 and not yaDesplazo then V9.desplazar(dy); yaDesplazo = true end
            V9.dirPrevia[f.idx] = { dx, dy }
        end
    end
end
local function cambiarLado(d)
    local f = filas[sel]; if not f then return end
    local l = math.max(0, math.min(2, f.lado + d))
    if l ~= f.lado then f.lado = l; plan[f.idx] = l; refrescar("") end
end
local function confirmar()
    nativosConfirmados = true
    for _, f in ipairs(filas) do plan[f.idx] = f.lado end
    local s = direccionAjustes()
    if s then for i = 0, NUEVO - 1 do if plan[i] then escribirUno(s, i, plan[i], "pantalla Phoenix") end end end
    -- BLINDAJE: «hay extra» = alguna fila con mando 9+ (antes se contaban filas: con menos de 8 mandos de PES
    -- conectados, los extra no se activaban)
    local hayExtra = false
    for _, f in ipairs(filas) do if f.idx >= NUEVO then hayExtra = true end end
    local gest = ptr(G_GESTOR)
    local jugando = hayRanurasJugando((gest and gest ~= 0) and ptr(gest + 0x180) or nil)
    if hayExtra and not activo and not jugando then activar() end
    actualizarDatos()                                    -- lo confirmado es lo que el gancho escribirá al empezar
    construirFilas()
    refrescar(hayExtra and (activo and "Listo · mandos extra activos · empieza el partido" or estado) or "Listo · lados guardados")
    pedirCierre = true                                   -- v0.8: se cierra sola y queda la de PES para «Confirmar»
    volcar()
end

-- ─── seguridad y eventos ──────────────────────────────────────────────────────
local function bandera(d)
    if apagado then return false end
    if enCurso then
        fallos = fallos + 1
        log("[fase1] error en la vez anterior (" .. d .. ", " .. fallos .. "/" .. MAX_FALLOS .. ")")
        if fallos >= MAX_FALLOS then apagado = true; log("[fase1] demasiados errores: me detengo"); return false end
    end
    enCurso = true; return true
end
local function bajar() enCurso = false; fallos = 0 end

-- ─── v0.8: la pantalla Phoenix se abre y se cierra SOLA con la de PES ────────
-- Sider no tiene función para abrir su ventana desde un módulo: se pulsan sus teclas (las de sider.ini:
-- overlay.vkey.toggle / overlay.vkey.next-module) SOLO si la ventana en primer plano es la de PES.
local AUTO_PANTALLA = true
local VK_VENTANA, VK_SIGUIENTE, VK_ANTERIOR = 0x20, 0x31, 0xC0
local mostrado, auto, prevAbre, prevCierra = false, nil, nil, nil
local enPartidoAhora = false                          -- v0.9: declarado aquí (lo usa también display_frame)
local ultimoDibujo = -1                               -- v0.9.7: aquí (lo usan m.show e imagenActual)
local function pesDelante()
    local w = ffi.C.phxf1_ventana(); if w == nil then return false end
    local pid = ffi.new("uint32_t[1]")
    ffi.C.phxf1_pidventana(w, pid)
    return pid[0] == ffi.C.phxf1_pid()
end
local function pulsar(vk) ffi.C.phxf1_tecla(vk, 0, 0, 0); ffi.C.phxf1_tecla(vk, 0, 2, 0) end   -- 2 = soltar
local bloqueoAuto = false
-- v0.8.11: Sider SOLO deja bloquear la entrada al módulo que controla su ventana («only module that currently controls
-- overlay can block input»: en v0.8.2–0.8.10 esto daba error en cada cuadro y CORTABA la apertura automática → había
-- que pulsar Espacio). Ahora no se intenta bloquear antes de que la Phoenix esté en pantalla (m.show bloquea).
local function bloquearJuego() end
local function soltarJuego() bloqueoAuto = false end
-- v0.8.3: «partido en curso» PEGADO: se enciende al ver ranuras jugando y solo se apaga tras 30 s seguidos sin ninguna
-- (en la pausa de PES las ranuras se vacían un momento: en v0.8.1 eso dejaba abrir la Phoenix en pleno partido)
local partidoVisto, sinRanurasDesde = false, nil
local function enPartidoPegado()
    local gest = ptr(G_GESTOR)
    if hayRanurasJugando((gest and gest ~= 0) and ptr(gest + 0x180) or nil) then
        partidoVisto, sinRanurasDesde = true, nil
    elseif partidoVisto then
        sinRanurasDesde = sinRanurasDesde or ahora()
        if ahora() - sinRanurasDesde >= 30 then partidoVisto, sinRanurasDesde = false, nil end
    end
    return partidoVisto
end
-- v0.8.3: diagnóstico — durante el partido, cada segundo se anota si cambia la lista de mandos de Windows
-- (DirectInput) o la tabla de GUID de HidMan (16 primeros mandos lógicos): ¿se desconecta algo antes de que PES
-- abra su pantalla de lados?
local huellaDI, huellaHid, ultimaHuella = nil, nil, -100
local function vigilarMandos()
    if ahora() - ultimaHuella < 1 or not enPartidoPegado() then return end
    ultimaHuella = ahora()
    local t = {}
    for _, d in ipairs(mandosDirectInput()) do if d.tipo == 1 then t[#t + 1] = bin2hex(d.guid):sub(1, 8) .. (d.vivo and "" or "(muerto)") end end
    table.sort(t)
    local di = #t .. ":" .. table.concat(t, ",")
    if huellaDI and di ~= huellaDI then anotar("DIAG mandos de Windows cambiaron: " .. huellaDI .. " → " .. di) end
    huellaDI = di
    local hid = ptr(G_HIDMAN)
    local gi = (hid and hid ~= 0) and ptr(hid + 0xD0) or nil
    if gi then
        local h = {}
        for i = 0, 15 do local g = leer(gi + i * 0x20 + 0x10, 4); h[#h + 1] = g and bin2hex(g) or "?" end
        local hh = table.concat(h, ",")
        if huellaHid and hh ~= huellaHid then anotar("DIAG GUID de HidMan cambiaron: " .. huellaHid .. " → " .. hh) end
        huellaHid = hh
    end
end
local reintentoGancho, intentosGancho = -100, 0
local function vigilarPantallaPES()
    -- BLINDAJE: si al cargar el módulo el código del juego aún no estaba listo (bytes distintos), se reintenta
    -- cada 3 s, como mucho 20 veces
    if AUTO_PANTALLA and not GANCHOS.abre.puesto and intentosGancho < 20 and ahora() - reintentoGancho >= 3 then
        reintentoGancho, intentosGancho = ahora(), intentosGancho + 1
        ponerGanchosPantalla()
    end
    if not AUTO_PANTALLA or not GANCHOS.abre.puesto or not GANCHOS.cierra.puesto then return end
    local a, c = contadorDatos(0x24), contadorDatos(0x28)
    if prevAbre == nil then prevAbre, prevCierra = a, c; return end
    if a ~= prevAbre then
        prevAbre = a
        -- v0.8.10: quién la abrió (dirección de retorno guardada por el gancho). En pleno partido PES la abre desde la
        -- fábrica FUN_140ABF5E0 (retorno 0x140ABF63A, medido en vivo). «Partido» = ranuras jugando AHORA, o partido visto
        -- hace poco Y abierta por esa fábrica; la marca «pegada» sola tapaba la pantalla de lados del principio.
        local d0, ret = datos(), 0
        if d0 then ret = tonumber(ffi.cast("uint64_t*", d0 + 0x40)[0]) end
        local gest0 = ptr(G_GESTOR)
        local ahoraJugando = hayRanurasJugando((gest0 and gest0 ~= 0) and ptr(gest0 + 0x180) or nil)
        -- v0.9.7: cualquier apertura con el partido visto hace menos de 30 s sin cursores también cuenta como partido
        local esPartido = ahoraJugando or (enPartidoPegado() and ret == 0x140ABF63A)
            or (partidoVisto and sinRanurasDesde ~= nil and ahora() - sinRanurasDesde < 30)
        anotar(string.format("PES abrió su pantalla de lados (retorno %X, ranuras jugando %s, partido visto %s)", ret, tostring(ahoraJugando), tostring(partidoVisto)))
        if esPartido then
            -- BLINDAJE: en pleno partido (también en pausa, cuando los cursores se vacían) NO se abre sola
            anotar("PES abrió su pantalla de lados con el partido en curso → la Phoenix NO se abre")
            -- diagnóstico v0.8.4: direcciones de código del juego que había en la pila (quién la abrió)
            local d = datos()
            if d then
                local q, t = ffi.cast("uint64_t*", d + 0x40), {}
                for k = 0, 63 do
                    local v = tonumber(q[k])
                    if v >= 0x140001000 and v < 0x146000000 then t[#t + 1] = string.format("+%X:%X", k * 8, v) end
                end
                anotar("DIAG pila al abrirse en partido: " .. table.concat(t, " "))
            end
            -- y cómo estaban las ranuras de cursor (lado/mando/jugador) de los 16 primeros
            local gest = ptr(G_GESTOR)
            local tabla = (gest and gest ~= 0) and ptr(gest + 0x180) or nil
            if tabla and tabla ~= 0 then
                local t = {}
                for i = 0, 15 do
                    local r = tabla + i * TAM_RANURA
                    t[#t + 1] = string.format("%d:%s/%s/%s", i, tostring(u8(r)), tostring(u32(r + 0x10)), tostring(u32(r + 0x20)))
                end
                anotar("DIAG ranuras (lado/mando/jugador): " .. table.concat(t, " "))
            end
            -- v0.9.3: Fralex quiere cambiar de lado también en pleno partido (pausa → «Elegir lado»). Desde la v0.9 la
            -- Phoenix no bloquea ni escribe S para 1–8 (solo copia la de PES), y los extra van por el gancho del
            -- productor: se abre igual que en el menú
            -- v0.9.4: solo si de verdad hay mandos 9–16 activos; si no, queda la de PES sola (es la original)
            V9.enPartido = true                              -- v0.9.21: en la pausa se conserva el lado de cada uno
            if activo and #extras > 0 then
                actualizarDatos()
                auto = { fase = "abrir", cuadros = 0, intentos = 0, enPartido = true }
                anotar("→ abro la Phoenix encima (en partido)")
            else
                anotar("en partido sin mandos extra activos → se queda la pantalla de PES")
            end
        else
            V9.enPartido = false                             -- v0.9.21: pantalla de lados del MENÚ (los extra empiezan al centro)
            if hayPrestados(PHX.PRESTADOS) or hayPrestados(PHX.OCULTOS) then devolverPrestados("nueva pantalla de lados") end   -- v0.9.25
            nativosConfirmados = false; actualizarDatos()    -- nueva elección: lo de la vez anterior no se arrastra
            auto = { fase = "abrir", cuadros = 0, intentos = 0 }
            partidoVisto, sinRanurasDesde = false, nil       -- es la elección de un partido nuevo
            -- v0.8.10: el juego se bloquea YA, para que la pantalla de PES no se pueda confirmar antes de que salga la
            -- Phoenix (en vivo se confirmó en 1,8 s y la Phoenix llegó tarde)
            bloquearJuego()
            anotar("→ abro la Phoenix encima")
        end
    end
    if c ~= prevCierra then
        prevCierra = c
        auto = mostrado and { fase = "cerrar", cuadros = 0 } or nil
        anotar("PES cerró su pantalla de lados" .. (mostrado and " → cierro la Phoenix" or ""))
        -- v0.9.19 (23-oct, Fralex: «no le deja hacer su estrategia… lo detecta como la máquina hasta llegar al partido»):
        -- los menús de antes del saque (estrategia) miran la lista de lados de S, y hasta ahora los extra solo se
        -- escribían en el gancho del productor (al saque). Se escriben ya al cerrar la pantalla de lados (fuera del
        -- partido); el gancho de «crear pantalla de lados» los vuelve a limpiar si se reabre, como antes.
        -- v0.9.55 (24-oct, Fralex: con el mando 16 en Local, tras □ ya no volvía con ○): si se cerró para abrir «Editar Datos
        -- personales» (□ de un mando 9+), NO es una confirmación: no se escriben lados ni se presta número (el préstamo
        -- cambiaba el mando del editor y el ○ del 16 ya no llegaba). Vuelve a abrirse la pantalla de lados después.
        if V9.editandoPerfil and ahora() - V9.editandoPerfil <= 5 then
            V9.editandoPerfil = nil
            anotar("cierre por «Editar Datos personales»: no se confirma nada")
        elseif activo and #extras > 0 and not frenado and not enPartidoPegado() then
            local s = direccionAjustes()
            if s then
                for _, e in ipairs(extras) do
                    if plan[e.idx] and plan[e.idx] ~= 1 then escribirUno(s, e.idx, plan[e.idx], "cierre de la pantalla de lados") end
                end
            end
            prestarNumeros()                                  -- v0.9.25
        elseif activo and #extras > 0 and not frenado and enPartidoPegado() then
            -- v0.9.41 (24-oct, Fralex: en la PAUSA, Estrategia no deja modificar al Visitante si solo tiene mandos 9+):
            -- en vivo, en la SEGUNDA pausa sí dejaba: el productor ya había puesto al mando 16 en S al reanudar, y con el
            -- foco a 16 PES lo acepta en Estrategia. Así que en la pausa se escriben los 9+ en S ya, sin préstamo.
            local s = direccionAjustes()
            if s then
                for _, e in ipairs(extras) do
                    if plan[e.idx] and plan[e.idx] ~= 1 then escribirUno(s, e.idx, plan[e.idx], "cierre de la pantalla de lados en pausa") end
                end
            end
        end
    end
    if not auto then soltarJuego(); return end
    auto.cuadros = auto.cuadros + 1
    if auto.fase == "abrir" then
        if mostrado then auto = nil; soltarJuego(); return end
        -- BLINDAJE: si mientras tanto empezó el partido, se deja de intentar
        if not auto.enPartido and enPartidoPegado() then
            auto = nil; soltarJuego(); anotar("empezó el partido: dejo de abrir la Phoenix"); return
        end
        if auto.cuadros < 5 or not pesDelante() then return end      -- v0.8.13: una tecla cada 5 cuadros (con 2 se pasaba de largo: Sider avisa «show» un cuadro después)
        auto.cuadros = 0
        auto.intentos = auto.intentos + 1
        -- BLINDAJE: mientras se pulsan las teclas de Sider, el juego NO las recibe (Espacio/«1» podían avanzar o cerrar
        -- menús de PES); las teclas de la ventana de Sider siguen funcionando con este bloqueo (no es el «duro»)
        bloquearJuego()
        -- 1.º: abrir la ventana de Sider; luego «siguiente módulo» hasta llegar a este; si la ventana ya estaba
        -- abierta (el 1.º la cerró), a mitad se vuelve a abrir y se sigue
        -- v0.9: «módulo ANTERIOR» en vez de «siguiente»: este módulo es el ÚLTIMO de sider.ini, así que desde el primero
        -- basta UNA pulsación (antes se recorrían todos los módulos y a veces se pasaba de largo). Si Sider recordaba
        -- otro módulo, se sigue retrocediendo hasta llegar.
        if auto.intentos == 1 or auto.intentos == 40 then pulsar(VK_VENTANA)
        elseif auto.intentos < 80 then pulsar(VK_ANTERIOR)
        else auto = nil; soltarJuego(); anotar("no pude abrir la pantalla Phoenix sola (ábrela con Espacio y 1)") end
    else
        if not mostrado then auto = nil; soltarJuego(); return end
        if auto.cuadros < 3 or not pesDelante() then return end
        pulsar(VK_VENTANA); auto = nil
    end
end
-- v0.8.8: cada segundo, con el módulo activo: re-enlazar los extra si PES soltó alguno (mando reconectado), anotar si
-- tienen objeto de mando lógico, y FRENO DE EMERGENCIA: si el productor se ejecuta más de 15 veces en 10 s (bucle de
-- pausa/reanudar), se dejan de meter los extra en el partido (datos+0 = 0) para que el juego no se trabe.
local ultimoCuidado, ventanaDesde, ventanaCuenta, estadoObj = -100, nil, nil, nil
-- v0.9.14: AJUSTES DE CURSOR de cada mando. Ghidra (20-oct ~21:30): el productor de ranuras FUN_141FB9040 lee, por
-- mando lógico i, el registro S_base+0x48+i*0x980 (FUN_1414C2250: byte 0 o 1 según el modo; byte 2 en el modo 0x10).
-- Si vale 3, la ranura queda en estado 3 y PES NO le da jugador (en vivo: mando 13 «VICTOR» con 03 → sin jugador;
-- mando 11 con 01 → jugador). Los mandos 1–8 lo cambian en los menús de PES; los 9+ no tienen menú → se pone 1.
local function cursorDeExtras()
    local a = ptr(G_AJUSTES); if not a or a == 0 then return end
    local b = ptr(a + 0x50); if not b or b == 0 then return end
    for _, e in ipairs(extras) do
        local r = b + 0x48 + e.idx * 0x980
        local v = leer(r, 3)
        if v then
            for k = 1, 3 do
                if v:byte(k) == 3 then
                    memory.write(r + k - 1, string.char(1))
                    anotar(string.format("mando %d: ajuste de cursor %d era 3 (sin jugador automático) → 1", e.idx + 1, k - 1))
                end
            end
        end
    end
end
local function cuidarExtras()
    if ahora() - ultimoCuidado < 1 then return end
    ultimoCuidado = ahora()
    enlazarExtras("revisión")
    cursorDeExtras()
    local hid = ptr(G_HIDMAN)
    local ini = (hid and hid ~= 0) and ptr(hid + 0x90) or nil
    if ini then
        local t = {}
        for _, e in ipairs(extras) do t[#t + 1] = string.format("%d:%s", e.idx, (ptr(ini + e.idx * 0x68) or 0) ~= 0 and "sí" or "no") end
        local s = table.concat(t, " ")
        if s ~= estadoObj then estadoObj = s; anotar("objeto de mando lógico de los extra: " .. s) end
    end
    local c = contadorDatos(0x20)
    if not ventanaDesde or ahora() - ventanaDesde >= 10 then ventanaDesde, ventanaCuenta = ahora(), c end
    if not frenado and c - ventanaCuenta > 15 then
        frenado = true
        actualizarDatos()
        anotar(string.format("FRENO DE EMERGENCIA: el productor se ejecutó %d veces en menos de 10 s (bucle de pausas); los mandos extra dejan de meterse en el partido", c - ventanaCuenta))
        estado = "Freno de emergencia: PES entraba en bucle de pausas; los mandos extra se han sacado del partido."
    end
end
-- ─── v0.9.36: GESTOR DE FOCO de los menús (menusystem::WindowManager, [0x1436F9C10]) a 16 mandos ─────────────
-- Diseño: cap_60_n §2. Objeto de 16 = 0x288: +0x08 16 pilas × 0x18 (las 8 primeras donde siempre) · +0x188 16 focos
-- × 0x10 (ventana +0, nivel +8). 44 parches (diseno_24 «## WM»: usos + crear/destruir 0x148 → 0x288) y FUN_140E718A0
-- (desenrollada a 8) reescrita en página propia (diseno_25 «f718a0», 134 bytes, call absoluto a 0x141B52380).
-- Página propia (0x2000, RWX): +0x000 objeto NUEVO (0x288, ceros) · +0x288…+0x3F7 canario 0xCD · +0x3F8 copia de
-- [0x1427C1560] (RTTI, vtable[-1]) · +0x400 vtable propia (8 entradas → +0x500) · +0x500 wm_dtor (mov rax,rcx; ret:
-- no libera, el bloque no es del montón del juego) · +0x600 FUN_140E718A0 nueva.
-- Sin next/pcall/loadstring (Sider). Todo en la tabla PHX (límite de 200 locales del cuerpo principal).
PHX.G_WM, PHX.VT_WM, PHX.F718A0 = 0x1436F9C10, 0x1427C1568, 0x140E718A0
PHX.F718A0_ORIG = "40534883ec20488bd9e8d20a"          -- push rbx · sub rsp,20 · mov rbx,rcx · call 141B52380 + 3 primeros bytes del call
PHX.F718A0_COD = "534883ec204889cb48b88023b54101000000ffd031d20fbcd0750231d283fa10735c448b83900100004531c989d0c1e004"
              .. "8b8c03900100004439c17c064189c84189d141ba010000004489d0c1e0048b8c03900100004439c17e064189c84589d1"
              .. "41ffc24183fa1072df4489c8c1e004488b8403880100004883c4205bc331c04883c4205bc3"
PHX.WM_DTOR = "4889c8c3"
PHX.PARCHES_WM = {
    { dir = 0x140E712A6, nombre = "foco 71260: lea r14,[rcx+0xC8] (focos)", orig = "4c8db1c8000000", nuevo = "4c8db188010000" },
    { dir = 0x140E712D0, nombre = "foco 71260: cmp ebp,8", orig = "83fd08", nuevo = "83fd10" },
    { dir = 0x140E71338, nombre = "foco 71260: cmp ebp,8 (bucle pilas)", orig = "83fd08", nuevo = "83fd10" },
    { dir = 0x140E71346, nombre = "foco 71260: cmp r12d,7 (mando N>7 sale)", orig = "4183fc07", nuevo = "4183fc0f" },
    { dir = 0x140E713F8, nombre = "foco 71260: lea rbx,[r13+0xD0] (niveles)", orig = "498d9dd0000000", nuevo = "498d9d90010000" },
    { dir = 0x140E71457, nombre = "foco 71260: cmp esi,8 (bucle focos)", orig = "83fe08", nuevo = "83fe10" },
    { dir = 0x140E714D5, nombre = "foco 71490: cmp r10d,8", orig = "4183fa08", nuevo = "4183fa10" },
    { dir = 0x140E71526, nombre = "foco 71520: cmp edx,8", orig = "83fa08", nuevo = "83fa10" },
    { dir = 0x140E7162F, nombre = "foco 71620: mov [rcx+rax*8+0xC8],rdx", orig = "488994c1c8000000", nuevo = "488994c188010000" },
    { dir = 0x140E71637, nombre = "foco 71620: lea rax,[rbx+0xD] (nivel = (N+0xD)*16)", orig = "488d430d", nuevo = "488d4319" },
    { dir = 0x140E71827, nombre = "foco 71820: cmp edx,8", orig = "83fa08", nuevo = "83fa10" },
    { dir = 0x140E719B3, nombre = "foco 719B0: lea rax,[rcx+0xC8]", orig = "488d81c8000000", nuevo = "488d8188010000" },
    { dir = 0x140E719CC, nombre = "foco 719B0: cmp r8d,8", orig = "4183f808", nuevo = "4183f810" },
    { dir = 0x140E71A2F, nombre = "foco 719E0: mov rdx,[rdi+rcx*8+0xC8]", orig = "488b94cfc8000000", nuevo = "488b94cf88010000" },
    { dir = 0x140E71A46, nombre = "foco 719E0: add rax,0xD", orig = "4883c00d", nuevo = "4883c019" },
    { dir = 0x140E71B09, nombre = "foco 71AE0: cmp ebx,8", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140E71B90, nombre = "foco 71AE0: cmp [rax+r11+0xC8],rdx", orig = "4a399418c8000000", nuevo = "4a39941888010000" },
    { dir = 0x140E71B9C, nombre = "foco 71AE0: mov [rax+r11+0xC8],rcx", orig = "4a898c18c8000000", nuevo = "4a898c1888010000" },
    { dir = 0x140E71BA4, nombre = "foco 71AE0: lea rax,[rbx+0xD]", orig = "488d430d", nuevo = "488d4319" },
    { dir = 0x140E71C22, nombre = "foco 71BE0: lea rcx,[rbp+0xC8]", orig = "488d8dc8000000", nuevo = "488d8d88010000" },
    { dir = 0x140E71C42, nombre = "foco 71BE0: cmp edx,8", orig = "83fa08", nuevo = "83fa10" },
    { dir = 0x140E71C90, nombre = "foco 71BE0: cmp r10,8 (comprobación de rango)", orig = "4983fa08", nuevo = "4983fa10" },
    { dir = 0x140E71C9A, nombre = "foco 71BE0: mov [rsp+r10+0x20],r12b → +0x60", orig = "4688641420", nuevo = "4688641460" },
    { dir = 0x140E71C9F, nombre = "foco 71BE0: cmp r11d,8", orig = "4183fb08", nuevo = "4183fb10" },
    { dir = 0x140E71CDF, nombre = "foco 71BE0: mov byte [rsp+r10+0x20],1 → +0x60", orig = "42c644142001", nuevo = "42c644146001" },
    { dir = 0x140E71CF2, nombre = "foco 71BE0: cmp r11d,8", orig = "4183fb08", nuevo = "4183fb10" },
    { dir = 0x140E71D1E, nombre = "foco 71BE0: mov [rbp+rax*8+0xC8],r12", orig = "4c89a4c5c8000000", nuevo = "4c89a4c588010000" },
    { dir = 0x140E71D26, nombre = "foco 71BE0: lea rax,[r15+0xD]", orig = "498d470d", nuevo = "498d4719" },
    { dir = 0x140E71DB2, nombre = "foco 71BE0: lea rsi,[rsp+0x20] → +0x60", orig = "488d742420", nuevo = "488d742460" },
    { dir = 0x140E71DB7, nombre = "foco 71BE0: lea rdi,[rbp+0xD0]", orig = "488dbdd0000000", nuevo = "488dbd90010000" },
    { dir = 0x140E71DF5, nombre = "foco 71BE0: cmp ebx,8", orig = "83fb08", nuevo = "83fb10" },
    { dir = 0x140E71E58, nombre = "foco 71E10: movups xmm0,[r15+0xC8]", orig = "410f1087c8000000", nuevo = "410f108788010000" },
    { dir = 0x140E71EAB, nombre = "foco 71E10: mov rcx,[r15+0xC8]", orig = "498b8fc8000000", nuevo = "498b8f88010000" },
    { dir = 0x140E71ED2, nombre = "foco 71E10: movups [r15+0xC8],xmm0", orig = "410f1187c8000000", nuevo = "410f118788010000" },
    { dir = 0x140E71EEE, nombre = "foco 71E10: lea rax,[r13+0xD]", orig = "498d450d", nuevo = "498d4519" },
    { dir = 0x140E71EF5, nombre = "foco 71E10: mov [r15+0xC8],rcx", orig = "49898fc8000000", nuevo = "49898f88010000" },
    { dir = 0x140E7159A, nombre = "foco 71570: mov edx,0x148 → 0x288 (tamaño al crear)", orig = "ba48010000", nuevo = "ba88020000" },
    { dir = 0x140E710F1, nombre = "foco 710B0: lea r8d,[rdx-0x10] → [rdx-8] (16 pilas)", orig = "448d42f0", nuevo = "448d42f8" },
    { dir = 0x140E71113, nombre = "foco 710B0: lea r8d,[rdx-8] → [rdx+0] (16 focos)", orig = "448d42f8", nuevo = "448d4200" },
    { dir = 0x140E71117, nombre = "foco 710B0: lea rcx,[rdi+0xC8] (base focos)", orig = "488d8fc8000000", nuevo = "488d8f88010000" },
    { dir = 0x140E71203, nombre = "foco 711E0: add rcx,0xC8", orig = "4881c1c8000000", nuevo = "4881c188010000" },
    { dir = 0x140E71216, nombre = "foco 711E0: lea r8d,[rdx-8] → [rdx+0]", orig = "448d42f8", nuevo = "448d4200" },
    { dir = 0x140E71230, nombre = "foco 711E0: lea r8d,[rdx-0x10] → [rdx-8]", orig = "448d42f0", nuevo = "448d42f8" },
    { dir = 0x140E7123F, nombre = "foco 711E0: mov edx,0x148 → 0x288", orig = "ba48010000", nuevo = "ba88020000" },
}
-- texto «ventana/nivel» del foco i (16 bytes desde i*16 en la cadena s), sin pasar por números de 64 bits
function PHX.wmFoco(s, i)
    local w = bin2hex(s:sub(i * 16 + 1, i * 16 + 8):reverse()):gsub("^0+", "")
    local nv = 0
    for k = 12, 9, -1 do nv = nv * 256 + s:byte(i * 16 + k) end
    if nv >= 0x80000000 then nv = nv - 0x100000000 end
    return (w == "" and "0" or w) .. "/" .. string.format("%d", nv)
end
function PHX.hx(n) return (n and n >= 0 and n < 2 ^ 53) and string.format("%X", n) or tostring(n) end
-- ¿están TODOS los bytes originales (44 sitios + los 12 de entrada de FUN_140E718A0)?
function PHX.wmBytesOriginales()
    for _, p in ipairs(PHX.PARCHES_WM) do
        local a = leer(p.dir, #p.orig / 2)
        if not a or bin2hex(a) ~= p.orig then return false, "bytes distintos en " .. p.nombre .. ": " .. (a and bin2hex(a) or "?") end
    end
    local e = leer(PHX.F718A0, 12)
    if not e or bin2hex(e) ~= PHX.F718A0_ORIG then return false, "bytes distintos en la entrada de FUN_140E718A0: " .. (e and bin2hex(e) or "?") end
    return true
end
function PHX.wmPagina()
    if PHX.pagWM then return PHX.pagWM end
    local p = ffi.C.phxf1_VA(nil, 0x2000, 0x3000, 0x40)
    if p == nil then return nil end
    local b = tonumber(ffi.cast("uint64_t", p))
    local q = ffi.cast("uint8_t*", b)
    ffi.fill(q, 0x2000, 0)
    ffi.fill(q + 0x288, 0x3F8 - 0x288, 0xCD)                              -- canario
    local v = ffi.cast("uint64_t*", b + 0x3F8)
    v[0] = ptr(PHX.VT_WM - 8) or 0                                        -- RTTI de la vtable original
    for i = 1, 8 do v[i] = b + 0x500 end                                  -- vtable propia (+0x400): todo → wm_dtor
    local dt, cod = hex2bin(PHX.WM_DTOR), hex2bin(PHX.F718A0_COD)
    ffi.copy(q + 0x500, dt, #dt)
    ffi.copy(q + 0x600, cod, #cod)
    if #cod ~= 134 or ffi.string(q + 0x500, #dt) ~= dt or ffi.string(q + 0x600, #cod) ~= cod then return nil end
    PHX.pagWM = b
    anotar(string.format("foco a 16: página propia en %X (objeto +0, vtable +0x400, wm_dtor +0x500, FUN_140E718A0 nueva +0x600)", b))
    return b
end
-- Deshace (en orden inverso) lo que haya escrito wmEscribirParches.
function PHX.wmDeshacer()
    local h = PHX.wmEscritos or {}
    for k = #h, 1, -1 do memory.write(h[k].dir, hex2bin(h[k].orig)); anotar("foco a 16: deshecho " .. h[k].nombre) end
    PHX.wmEscritos = {}
end
-- Escribe los 44 parches y el salto de FUN_140E718A0 a pag+0x600, leyendo cada uno de vuelta. Si alguno no queda,
-- deshace todo lo escrito y devuelve false.
function PHX.wmEscribirParches(pag)
    PHX.wmEscritos = {}
    for _, p in ipairs(PHX.PARCHES_WM) do
        memory.write(p.dir, hex2bin(p.nuevo))
        PHX.wmEscritos[#PHX.wmEscritos + 1] = p
        local a = leer(p.dir, #p.nuevo / 2)
        if not a or bin2hex(a) ~= p.nuevo then
            anotar("foco a 16: no quedó escrito " .. p.nombre .. ": " .. (a and bin2hex(a) or "?"))
            PHX.wmDeshacer(); return false
        end
    end
    anotar(string.format("foco a 16: %d parches escritos (usos, tamaño 0x288, ctor y dtor de 16)", #PHX.PARCHES_WM))
    local salto = hex2bin("48b8") .. u64le(pag + 0x600) .. hex2bin("ffe0")      -- mov rax,pag+0x600 · jmp rax (12 bytes)
    local e = { dir = PHX.F718A0, nombre = "entrada de FUN_140E718A0", orig = PHX.F718A0_ORIG }
    memory.write(PHX.F718A0, salto)
    PHX.wmEscritos[#PHX.wmEscritos + 1] = e
    if leer(PHX.F718A0, 12) ~= salto then
        anotar("foco a 16: no quedó escrito el salto de FUN_140E718A0"); PHX.wmDeshacer(); return false
    end
    anotar(string.format("foco a 16: FUN_140E718A0 → %X (versión de 16)", pag + 0x600))
    return true
end
-- m.init: anotar los dos singletons; si el gestor de foco aún no existe, parchear ya; si existe, migrar más tarde.
-- v0.9.58 HUD DE 16 (25-oct, Fralex: los mandos 13–16 solo tienen la flechita; sin nombre/número ni barra de potencia).
-- Ghidra (informes/datos/rea/hud16_informe.md): ModelCursorName (nombre/número/avatar según «Tipo de cursor») y ModelGauge
-- (barra) tienen tablas de 12 modelos (FUN_1420D5BF0: ranura ≥ 12 → −1). Se agrandan los objetos, se añade una tabla de
-- 16 (CN +0x118; MG T1 crece a 16 y T2 pasa a +0x168), se crean 4+4+4 modelos con ids nuevos 0xD0–0xDB (FUN_141F9E610
-- aprende su tipo) y se suben los topes. Solo al cargar el módulo (en m.init no existe ningún partido). Se comprueban
-- TODOS los bytes originales antes de escribir nada; si uno no coincide, no se toca nada.
-- Además cueva F: un usuario SIN nombre recibe la textura «Usuario N» (antes no se creaba y salía vacío en la cancha).
PHX.HUD16_CUEVAS = {
    { k = "E", off = 0x000, cod = "81fad00000007c2281fadc0000007d1ab80100000081fad40000007c0cb00481fad80000007c02b005c383fa197d0333c0c383fa297c0c48b81de6f94101000000ffe0b801000000c3" },
    { k = "A", off = 0x080, cod = "488dbb1801000031c0b910000000f348ab488bc3488b5c24484883c4305fc3" },
    { k = "B", off = 0x0C0, cod = "31db488b84dfa0000000488984df18010000ffc383fb0c72e9bbd000000089da4889f948b880600d4201000000ffd0488984dff8faffffffc381fbd400000072dd4c8d4f0c498bce4c8d4708ba1900000048b8b7cf0d4201000000ffe0" },
    { k = "C", off = 0x140, cod = "bbd000000089da4889e948b8e0e3f94101000000ffd0ffc381fbd400000072e54883c70831c0b910000000f348ab488b5c2430b001488b6c243849bb15c20d420100000041ffe3" },
    { k = "G", off = 0x1A0, cod = "488dbb6801000031c0b910000000f348ab488bc3488b5c24304883c4205fc3" },
    { k = "D", off = 0x1E0, cod = "31db488b84dda0000000488984dd68010000ffc383fb0c72e931db8d93d40000004889e948b880600d4201000000ffd0488984dda00000008d93d80000004889e948b880600d4201000000ffd0488984ddc8010000ffc383fb0472bf4c8d4d0c488bce4c8d4508ba4700000048b82ff30d4201000000ffe0" },
    { k = "H", off = 0x280, cod = "488dbb00ffffffbed400000048b8e0e6f94101000000ffd04889c189f248b8e0e3f94101000000ffd0ffc681fedc00000072d94881c76801000031c0b910000000f348ab488b5c2430b001488b6c243849bbceeb0d420100000041ffe3" },
    { k = "F", off = 0x300, cod = "4883bd200100000075234489f1c1e904488d951001000048b830cc0c4201000000ffd04883bd2001000000740c48b888f70c4201000000ffe048b849f80c4201000000ffe0" },
}
PHX.HUD16_BYTES = {   -- dirección, original, nuevo
    { dir = 0x1420DC155, orig = "ba18010000", nuevo = "ba98010000", nombre = "CN reserva 0x118 → 0x198" },
    { dir = 0x1420DEA55, orig = "ba68010000", nuevo = "bae8010000", nombre = "MG reserva 0x168 → 0x1E8" },
    { dir = 0x1420DC5BF, orig = "83f80b", nuevo = "83f80f", nombre = "CN obtener modelo ≤ 15" },
    { dir = 0x1420DC5C6, orig = "488b84c7a0000000", nuevo = "488b84c718010000", nombre = "CN obtener modelo de la tabla +0x118" },
    { dir = 0x1420DF0E7, orig = "4183f90c", nuevo = "4183f910", nombre = "MG obtener modelo T2 < 16" },
    { dir = 0x1420DF106, orig = "4183f90c", nuevo = "4183f910", nombre = "MG obtener modelo T1 < 16" },
    { dir = 0x1420DF0ED, orig = "4a8b8ccfa0000000", nuevo = "4a8b8ccf68010000", nombre = "MG T2 en +0x168" },
    { dir = 0x1420DF11B, orig = "488b84dfa0000000", nuevo = "488b84df68010000", nombre = "MG T2 en +0x168 (2)" },
    { dir = 0x1420DF429, orig = "498d9ea0000000", nuevo = "498d9e68010000", nombre = "MG T2 en +0x168 (modo por lados)" },
    { dir = 0x1420D5CAF, orig = "83fb0c", nuevo = "83fb10", nombre = "índice de modelo: ranura < 16" },
}
PHX.HUD16_GANCHOS = {  -- en orden: tipo de id, constructores, init y release; F aparte
    { dir = 0x141F9E610, orig = "83fa197d0333c0c383fa297c6a", k = "E", nombre = "tipo de los ids 0xD0–0xDB" },
    { dir = 0x1420DC0C1, orig = "488bc3488b5c24484883c430", k = "A", nombre = "CN constructor: tabla nueva a cero" },
    { dir = 0x1420DE9E9, orig = "488bc3488b5c24304883c420", k = "G", nombre = "MG constructor: T2 nueva a cero" },
    { dir = 0x1420DCFA9, orig = "4c8d4f0c498bce4c8d47088d5519", k = "B", nombre = "CN init: 16 modelos" },
    { dir = 0x1420DF320, orig = "4c8d4d0c488bce4c8d4508418d5747", k = "D", nombre = "MG init: 16+16 modelos" },
    { dir = 0x1420DC209, orig = "488b5c2430b001488b6c2438", k = "C", nombre = "CN release" },
    { dir = 0x1420DEBC2, orig = "488b5c2430b001488b6c2438", k = "H", nombre = "MG release" },
    { dir = 0x1420CF77A, orig = "4883bd20010000000f86c1000000", k = "F", nombre = "«Usuario N» para usuarios sin nombre" },
}
function PHX.hud16Inicio()
    for _, b in ipairs(PHX.HUD16_BYTES) do
        local a = leer(b.dir, #b.orig / 2)
        if not a or bin2hex(a) ~= b.orig then anotar("HUD de 16: bytes distintos en " .. b.nombre .. " (" .. (a and bin2hex(a) or "?") .. ") → no se toca nada"); return end
    end
    for _, g in ipairs(PHX.HUD16_GANCHOS) do
        local a = leer(g.dir, #g.orig / 2)
        if not a or bin2hex(a) ~= g.orig then anotar("HUD de 16: bytes distintos en " .. g.nombre .. " (" .. (a and bin2hex(a) or "?") .. ") → no se toca nada"); return end
    end
    local p = ffi.C.phxf1_VA(nil, 0x1000, 0x3000, 0x40)
    if p == nil then anotar("HUD de 16: VirtualAlloc falló"); return end
    local base = tonumber(ffi.cast("uint64_t", p))
    local q, dirDe = ffi.cast("uint8_t*", base), {}
    ffi.fill(q, 0x1000, 0xCC)
    for _, c in ipairs(PHX.HUD16_CUEVAS) do
        local cod = hex2bin(c.cod)
        ffi.copy(q + c.off, cod, #cod)
        if ffi.string(q + c.off, #cod) ~= cod then anotar("HUD de 16: no se pudo copiar la cueva " .. c.k); return end
        dirDe[c.k] = base + c.off
    end
    for _, g in ipairs(PHX.HUD16_GANCHOS) do
        local n = #g.orig / 2
        local salto = hex2bin("48b8") .. u64le(dirDe[g.k]) .. hex2bin("ffe0") .. string.rep(hex2bin("90"), n - 12)
        memory.write(g.dir, salto)
        anotar(string.format("HUD de 16: gancho %s → cueva %s en %X (%s)", string.format("%X", g.dir), g.k, dirDe[g.k], g.nombre))
    end
    for _, b in ipairs(PHX.HUD16_BYTES) do memory.write(b.dir, hex2bin(b.nuevo)); anotar("HUD de 16: parchado " .. b.nombre) end
    PHX.hud16Pagina = base
    anotar("HUD de 16: nombre/número y barra de potencia para las ranuras 12–15 (mandos 13–16)")
end

function PHX.wmInicio()
    local wm, cur = ptr(PHX.G_WM), ptr(G_CURSORES)
    anotar(string.format("al cargar: [0x1436F9C10] (gestor de foco) = %s · [0x1436F9BC8] (gestor de cursores) = %s",
        PHX.hx(wm), PHX.hx(cur)))
    if wm == nil then anotar("foco a 16: [0x1436F9C10] ilegible → no se toca nada"); return end
    if wm ~= 0 then
        PHX.wmPendiente = true
        anotar("foco a 16: el gestor YA existe → MIGRACIÓN pendiente para el primer cuadro en un menú")
        return
    end
    local ok, msg = PHX.wmBytesOriginales()
    if not ok then anotar("foco a 16: " .. msg .. " → no se toca nada"); return end
    local pag = PHX.wmPagina()
    if not pag then anotar("foco a 16: no pude reservar/llenar la página propia → no se toca nada"); return end
    if PHX.wmEscribirParches(pag) then
        PHX.wmHecho = "parcheado al cargar"
        anotar("foco a 16: aplicado ANTES de que exista el gestor (PES lo creará de 0x288 con 16 pilas y 16 focos)")
    end
end
-- MIGRACIÓN (una sola vez, todo en el mismo display_frame): gestor viejo de 0x148 → NUEVO de 0x288 en la página propia.
function PHX.wmMigrar()
    anotar("foco a 16: MIGRACIÓN (primer cuadro en menú)")
    local viejo = ptr(PHX.G_WM)
    if not viejo or viejo == 0 then anotar("foco a 16: [0x1436F9C10] = " .. tostring(viejo) .. " → no se migra"); return end
    local vt = ptr(viejo)
    if vt ~= PHX.VT_WM then
        anotar(string.format("foco a 16: [VIEJO=%X] = %s (se esperaba 1427C1568) → no se migra", viejo, PHX.hx(vt)))
        return
    end
    local ok, msg = PHX.wmBytesOriginales()
    if not ok then anotar("foco a 16: " .. msg .. " → no se migra"); return end
    local pilas, focos = leer(viejo + 0x08, 0xC0), leer(viejo + 0xC8, 0x80)
    if not pilas or not focos then anotar("foco a 16: no pude leer pilas/focos del viejo → no se migra"); return end
    local nuevo = PHX.wmPagina()
    if not nuevo then anotar("foco a 16: no pude reservar/llenar la página propia → no se migra"); return end
    ffi.cast("uint64_t*", nuevo)[0] = nuevo + 0x400                     -- [NUEVO] = vtable propia
    ffi.copy(ffi.cast("uint8_t*", nuevo + 0x08), pilas, 0xC0)           -- 8 pilas (los vectores se trasladan)
    ffi.copy(ffi.cast("uint8_t*", nuevo + 0x188), focos, 0x80)          -- 8 focos
    if ffi.string(ffi.cast("const char*", nuevo + 0x08), 0xC0) ~= pilas or ffi.string(ffi.cast("const char*", nuevo + 0x188), 0x80) ~= focos then
        anotar("foco a 16: la copia al nuevo no quedó bien → no se migra"); return
    end
    local t = {}
    for i = 0, 7 do
        t[#t + 1] = string.format("%d:%s", i, PHX.wmFoco(focos, i))
    end
    anotar(string.format("foco a 16: VIEJO=%X → NUEVO=%X; copiadas 8 pilas (0xC0) y 8 focos (0x80): %s", viejo, nuevo, table.concat(t, " ")))
    if not PHX.wmEscribirParches(nuevo) then anotar("foco a 16: parches deshechos → no se migra (el viejo sigue en uso)"); return end
    memory.write(PHX.G_WM, u64le(nuevo))
    if ptr(PHX.G_WM) ~= nuevo then
        memory.write(PHX.G_WM, u64le(viejo)); PHX.wmDeshacer()
        anotar("foco a 16: el puntero [0x1436F9C10] no quedó → restaurado el viejo y deshechos los parches"); return
    end
    anotar(string.format("foco a 16: [0x1436F9C10] = %X (NUEVO)", nuevo))
    memory.write(viejo + 0x08, string.rep("\0", 0xC0))                  -- el viejo queda con pilas vacías (sin doble liberación)
    local z = leer(viejo + 0x08, 0xC0)
    anotar("foco a 16: pilas del viejo puestas a cero: " .. ((z == string.rep("\0", 0xC0)) and "sí" or "NO (¡ojo!)"))
    PHX.wmHecho, PHX.wmViejo = "migrado", viejo
    anotar("foco a 16: MIGRACIÓN HECHA")
end
-- display_frame: la migración espera a un cuadro con PES en un menú (no en partido ni con ranuras jugando).
function PHX.wmIntentar()
    if enPartidoPegado() then return end
    local g = ptr(G_GESTOR)
    if hayRanurasJugando((g and g ~= 0) and ptr(g + 0x180) or nil) then return end
    PHX.wmPendiente = false                                             -- una sola vez, salga bien o mal
    PHX.wmMigrar()
end
-- Cada 2 s, ya hecho: canario intacto (si se migró) y cambios de los 16 focos de [0x1436F9C10] (para el registro).
function PHX.wmVigilar()
    if not PHX.wmHecho or ahora() - (PHX.wmUltimaVista or -100) < 2 then return end
    PHX.wmUltimaVista = ahora()
    if PHX.wmHecho == "migrado" and not PHX.wmCanarioRoto then
        local c = ffi.string(ffi.cast("const char*", PHX.pagWM + 0x288), 0x3F8 - 0x288)
        if c ~= string.rep("\205", 0x3F8 - 0x288) then PHX.wmCanarioRoto = true; anotar("foco a 16: ¡CANARIO PISADO! (algo escribió más allá de +0x288 del nuevo)") end
    end
    local wm = ptr(PHX.G_WM)
    local f = (wm and wm ~= 0) and leer(wm + 0x188, 0x100) or nil
    if not f then return end
    local t = {}
    for i = 0, 15 do
        if f:sub(i * 16 + 1, i * 16 + 12) ~= string.rep("\0", 12) then t[#t + 1] = string.format("%d:%s", i, PHX.wmFoco(f, i)) end
    end
    local s = PHX.hx(wm) .. " " .. table.concat(t, " ")
    if s ~= PHX.wmHuella then PHX.wmHuella = s; anotar("foco a 16: focos " .. s) end
end
function m.display_frame(ctx)
    if not bandera("cuadro") then return end
    if PHX.wmPendiente then PHX.wmIntentar() end          -- v0.9.36: migración del gestor de foco a 16 (una vez, en menú)
    PHX.wmVigilar()                                       -- v0.9.36: canario y focos en el registro (cada 2 s)
    if pedirCierre then pedirCierre = false; if mostrado then auto = { fase = "cerrar", cuadros = 0 } end end
    vigilarPantallaPES()
    -- v0.9: mientras se ve la Phoenix (en el menú), espejo de la de PES + mandos extra
    if mostrado and not enPartidoAhora and #filas > 0 then V9.reflejarNativos(); V9.moverExtras() end
    if activo and #extras > 0 then V9.confirmarExtras() end   -- v0.9.60: ✕ de los mandos 9+ confirma
    if activo and #extras > 0 then V9.atrasExtras() end   -- v0.9.38: ○ de los mandos 9+ en la pantalla de lados de PES
    if activo and #extras > 0 then V9.cuadradoExtras() end   -- v0.9.51: □ de los mandos 9+ edita SU perfil (v0.9.56: también en la pausa)
    -- v0.9.34: en la PAUSA del partido (sin la pantalla Phoenix) los menús de PES solo escuchan a los mandos 1–8: ✕/○ de
    -- un mando 9+ se pasan como teclas. Solo con el partido visto y SIN ranuras jugando (pausa), nunca jugando.
    if activo and #extras > 0 and not mostrado and enPartidoPegado() then
        local g = ptr(G_GESTOR)
        if not hayRanurasJugando((g and g ~= 0) and ptr(g + 0x180) or nil) then V9.botonesAPes() end
    end
    vigilarMandos()
    if activo then cuidarExtras() end
    -- v0.9.4: si al abrir PES los puestos 9+ de Link aún no tenían mando (en vivo: «0 puestos extra con mando» y nunca
    -- se reintentaba → los extra no se movían), se reintenta cada 5 s en el menú, solo cuando ya hay alguno vivo
    -- v0.9.7 BLINDAJE: no reactiva si se apagó con I; no cuenta los mandos 1–8; solo reintenta si cambió el número de
    -- puestos vivos, y como mucho 5 veces por sesión (antes podía parchar/desparchar cada 5 s y llenar el registro)
    if not activo and not parchado and not apagado and not V9.apagadoManual and (V9.intentosAct or 0) < 5
       and ahora() - (V9.reintentoAct or -100) >= 5 and not enPartidoPegado() then
        V9.reintentoAct = ahora()
        local link, vivos = puestosDeLink(), 0
        if link then
            local vivo, enS = {}, guidsDeSettings(ptr(G_HIDMAN) or 0)
            for _, d in ipairs(mandosDirectInput()) do if d.tipo == 1 and d.vivo then vivo[d.guid] = true end end
            for _, g in pairs(link) do if vivo[g] and not enS[g] then vivos = vivos + 1 end end
        end
        if vivos > 0 and vivos ~= V9.vivosPrevios then
            V9.vivosPrevios = vivos; V9.intentosAct = (V9.intentosAct or 0) + 1
            anotar(string.format("reintento: %d puesto(s) 9+ de Link ya tienen mando → activo", vivos))
            activar()
            if mostrado then construirFilas(); refrescar() end
        end
    end
    local t = os.clock()
    if activo and t - ultimoMirar >= 0.5 then ultimoMirar = t; escribirAjustes() end
    if t - ultimoVolcado >= 2 then volcar() end
    bajar()
end
local VK_ARRIBA, VK_ABAJO, VK_IZQ, VK_DER, VK_ENTER = 0x26, 0x28, 0x25, 0x27, 0x0D
function m.key_down(ctx, vkey)
    if not bandera("tecla") then return end
    -- v0.9.12: atajos I/K/J DESACTIVADOS: desde la v0.9 el teclado también llega a PES, y una «I» (medido en vivo
    -- 20-oct 19:25) apagaba los mandos extra para toda la sesión. La activación es automática.
    if false and vkey == VK_I then
        -- BLINDAJE: con el partido en marcha no se activa ni se desactiva (cambiar el código del juego a mitad de
        -- partido, con ranuras usando los mandos 9+, es justo lo que puede romperlo). Se hace en el menú.
        local gest = ptr(G_GESTOR)
        -- v0.9.7: también en la pausa del partido (ahí las ranuras se vacían un momento)
        if enPartidoPegado() or hayRanurasJugando((gest and gest ~= 0) and ptr(gest + 0x180) or nil) then
            estado = "Con el partido en marcha no se activa ni desactiva: hazlo en el menú."
            anotar("tecla I ignorada: partido en curso")
        elseif activo or parchado then deshacer(); V9.apagadoManual = true
        else V9.apagadoManual = false; activar() end
        construirFilas(); refrescar(estado)
    elseif false and vkey == VK_K then
        if activo then escribirAjustes(true) end
    elseif false and vkey == VK_J then
        anotar("#### MARCA ####")
    -- v0.9: ←/→/ENTER ya no los usa la Phoenix: llegan a la pantalla de PES (debajo), que mueve la fila del mando 1 y
    -- confirma; la Phoenix lo copia. ↑/↓ solo desplazan la tabla cuando hay más de 8 mandos.
    elseif vkey == 0x74 or vkey == 0x75 then           -- v0.9.30: F5 / F6 = capitán de Local / Visitante (anfitrión)
        local codigo = (vkey == 0x74) and 1 or 2
        local lista = {}
        for _, f in ipairs(filas) do if f.lado ~= 1 and CODIGO_LADO[f.lado] == codigo then lista[#lista + 1] = f.idx end end
        local actual, sig = PHX.CAPITAN[codigo], nil
        for n, idx in ipairs(lista) do if idx == actual then sig = lista[n + 1] end end
        if actual == nil then sig = lista[1] end
        PHX.CAPITAN[codigo] = sig
        anotar(string.format("anfitrión: capitán de %s = %s", codigo == 1 and "Local" or "Visitante", sig and ("mando " .. (sig + 1)) or "ninguno"))
        refrescar()
    elseif vkey == VK_ARRIBA then V9.desplazar(-1)
    elseif vkey == VK_ABAJO then V9.desplazar(1)
    end
    volcar()
    bajar()
end
-- El mando del anfitrión también maneja la pantalla (cruceta / palanca izquierda / A)
local DPAD = { [1] = { 0, -1 }, [2] = { 0, 1 }, [4] = { -1, 0 }, [8] = { 1, 0 } }
function m.gamepad_input(ctx, inputs)
    if not bandera("mando") then return end
    for nombre, v in pairs(inputs) do
        if nombre == "DPAD" and DPAD[v] then
            local d = DPAD[v]
            if d[2] ~= 0 then moverSel(d[2]) else cambiarLado(d[1]) end
        elseif nombre == "LSy" and v ~= 0 then moverSel(-v)
        elseif nombre == "LSx" and v ~= 0 then cambiarLado(v)
        elseif nombre == "A" and v == 1 then confirmar()
        end
    end
    bajar()
end
function m.show(ctx)
    -- BLINDAJE: solo se bloquea el juego si la pantalla se puede dibujar; si faltan piezas, se ve el texto y el juego
    -- sigue respondiendo (antes una pantalla invisible + bloqueo parecía un cuelgue)
    local ok = cargarRecursos() ~= nil
    -- v0.8.12 BLINDAJE: en pleno partido NUNCA se bloquea el juego (si la ventana de Sider quedaba abierta en este
    -- módulo, nada se movía: ni mandos ni teclado); allí solo se muestra un texto pequeño
    -- v0.9.3: con la pantalla de lados de PES abierta (también en la pausa del partido) se muestra la Phoenix completa
    enPartidoAhora = enPartidoPegado() and contadorDatos(0x24) == contadorDatos(0x28)
    P.modoPartido = enPartidoPegado() and not enPartidoAhora     -- pantalla de lados abierta en pleno partido
    -- v0.9: el juego ya NO se bloquea: la pantalla de PES sigue debajo recibiendo los mandos 1–8 (cada uno mueve su
    -- fila y «Confirmar» funciona como siempre); la Phoenix solo la copia y añade los mandos 9–16
    mostrado = true
    -- v0.8.9: los mandos extra se activan SOLOS al abrirse la pantalla (en el menú, nunca en pleno partido): así la
    -- tabla sale completa desde el principio, sin pulsar I
    if not activo and not parchado and not enPartidoPegado() and not V9.apagadoManual then activar() end
    ultimoDibujo = -1                                     -- v0.9.7: dibujo nuevo al abrir (modo menú/partido correcto)
    construirFilas()
    -- un mando extra que ya tenía la palanca movida al abrirse no salta de lado: hay que soltar y volver a mover
    V9.dirPrevia, V9.vistoCentro = {}, {}
    V9.gemelo, V9.coinciden = {}, {}                      -- v0.9.11: cada apertura empieza limpia
    for _, e in ipairs(extras) do local dx, dy = V9.direccionExtra(e); V9.dirPrevia[e.idx] = { dx, dy } end
    desplaz, sel = 0, 1
    refrescar()
end
function m.hide(ctx)
    mostrado = false
    P.modoPartido = false
    if input and input.set_blocked then input.set_blocked(false) end
end
-- Sider no deja borrar archivos (os.remove no existe en los módulos): las imágenes viejas se VACÍAN (0 bytes).
local function vaciar(ruta) local f = io.open(ruta, "wb"); if f then f:close() end end
local function imagenActual()
    -- como mucho 5 dibujos por segundo (al mantener una flecha, el teclado repite muy rápido; cada dibujo son 8 MB)
    if not pendiente or os.clock() - ultimoDibujo < 0.2 then return rutaImagen end
    pendiente = false
    ultimoDibujo = os.clock()
    nImagen = nImagen + 1
    local ruta = string.format("%spantalla_%d.bmp", carpeta, nImagen)
    if not dibujarPantalla(ruta) then anotar("pantalla: no se pudo dibujar (¿faltan piezas en " .. tostring(carpetaLados) .. "?)"); return rutaImagen end
    if rutaVieja then vaciar(rutaVieja) end       -- se vacía la de hace dos dibujos (la anterior puede seguir en uso)
    rutaVieja, rutaImagen = rutaImagen, ruta
    anotar(string.format("pantalla: imagen %d dibujada (fila elegida %d de %d) en %.0f ms", nImagen, sel, #filas, (os.clock() - ultimoDibujo) * 1000))
    log("[fase1] imagen " .. nImagen)            -- en sider.log, junto a «Loaded 2D texture» de Sider
    return rutaImagen
end
function m.overlay_on(ctx)
    if apagado then return "FASE 1 · detenida por seguridad (ver sider.log)" end
    if enPartidoAhora then
        return string.format("PHOENIX mandos v%s · partido en curso: la pantalla de lados no se muestra aquí (Espacio = cerrar)", m.version)
    end
    local img = imagenActual()
    if not img then
        return string.format("PHOENIX · Local / Visitante · v%s\n%s\n(faltan las piezas en content\\phoenix\\lados_replica\\ — ver generar_replica_lados.ps1)", m.version, estado)
    end
    -- a pantalla completa y sin texto ni fondo de Sider: solo la réplica de la pantalla de PES
    -- OJO: Sider toma como fracción de la pantalla solo los valores < 1 (1.0 = ¡1 píxel!, medido en sider.log)
    if P.modoPartido then
        local R = P.TABLA
        return "", img, { image_width = R.w / P.W, image_height = R.h / P.H, image_alpha = P.ALFA_PARTIDO,
                          image_hmargin = R.x, image_vmargin = R.y, background_color = "00000000" }
    end
    return "", img, { image_width = 0.99999, image_height = P.H_IMG / P.H, image_alpha = 1.0, image_hmargin = 0, image_vmargin = 0,
                      background_color = "00000000" }
end
-- Para probar los ganchos fuera del juego (herramientas\probar_ganchos.js): devuelve el código en hex con
-- direcciones de prueba; el arnés lo EJECUTA con memoria falsa en lugar de la del juego.
function m._codigos(datos, global)
    return preProductor(datos, global), preAbre(datos, global), preContador(datos, 0x28, 0x248)
end
-- Para probar el dibujo fuera del juego (herramientas\vista_previa_lados.js)
function m._vista_previa(base, filasPrueba, seleccion, msg)
    carpeta, carpetaLados = base, base
    filas, sel, aviso = filasPrueba, seleccion or 1, msg or ""
    return dibujarPantalla(base .. "vista_previa.bmp")
end
function m.init(ctx)
    local base = ctx.sider_dir or ".\\"
    carpeta = base .. "content\\phoenix\\mandos\\"
    carpetaLados = base .. "content\\phoenix\\lados_replica\\"
    if fs and fs.make_dirs then fs.make_dirs(carpeta) end
    local f = io.open(carpeta .. "fase1.txt", "ab")
    if f then f:write(string.format("# fase1 v%s · %s\n", m.version, os.date("%Y-%m-%d %H:%M:%S"))); f:close() end
    for nombre, ftipo in (fs and fs.find_files and fs.find_files(carpeta .. "pantalla_*.bmp") or function() end) do
        vaciar(carpeta .. nombre)               -- imágenes viejas de la vez anterior (se quedan en 0 bytes)
    end
    -- teclas reales de la ventana de Sider (por si alguien las cambió en sider.ini)
    local ini = leerBin(base .. "sider.ini")
    if ini then
        local t = ini:match("\noverlay%.vkey%.toggle%s*=%s*(0x%x+)"); if t then VK_VENTANA = tonumber(t) end
        local s = ini:match("\noverlay%.vkey%.next%-module%s*=%s*(0x%x+)"); if s then VK_SIGUIENTE = tonumber(s) end
        local a = ini:match("\noverlay%.vkey%.prev%-module%s*=%s*(0x%x+)"); if a then VK_ANTERIOR = tonumber(a) end
        -- v0.9: si este módulo NO es el último con ventana en sider.ini, retroceder no es lo más corto: se avanza
        local ultimo
        for nombre in ini:gmatch("\n%s*lua%.module%s*=%s*\"([^\"]+)\"") do ultimo = nombre end
        if ultimo and not ultimo:find("phoenix_mandos_fase1", 1, true) then VK_ANTERIOR = VK_SIGUIENTE end
    end
    -- v0.8: ganchos de la pantalla de lados de PES (solo cuentan aperturas/cierres; no cambian nada del juego)
    if ffi and memory then preparar(); ponerGanchosPantalla() end
    -- v0.9.22: gestor de cursores de menú a 16 mandos, solo si todavía no existe (si existe, su tabla es de 8)
    -- v0.9.23: DESACTIVADO — con estos parches PES no llegó a abrir (23-oct 02:06, dos arranques). Falta saber qué otra
    -- parte del gestor depende de 8 (ver cap_60_k). Se deja el código para retomarlo.
    -- v0.9.35: REACTIVADO con el byte que faltaba (0x140E675DF). En m.init el gestor aún no existe (visto en fase1.txt).
    if ffi and memory then
        local g = ptr(G_CURSORES)
        if g == nil or g ~= 0 then
            anotar("gestor de cursores YA creado (o ilegible): no se agranda; los mandos 9+ no moverán listas de menú hasta reiniciar PES")
        else
            local todo = true
            for _, p in ipairs(PARCHES_CURSORES) do
                local a = leer(p.dir, #p.orig / 2)
                if not a or (bin2hex(a) ~= p.orig and bin2hex(a) ~= p.nuevo) then todo = false; anotar("gestor de cursores: bytes distintos en " .. p.nombre .. ": " .. (a and bin2hex(a) or "?")) end
            end
            if todo then
                for _, p in ipairs(PARCHES_CURSORES) do memory.write(p.dir, hex2bin(p.nuevo)); anotar("parchado " .. p.nombre) end
                -- con 16 cursores de verdad ya no hace falta «mando AND 7» (v0.9.24): se quitan de la lista de parches
                for i = #PARCHES, 1, -1 do if PARCHES[i].nombre:find("AND 7", 1, true) then table.remove(PARCHES, i) end end
                anotar("cursores de menú a 16: los parches «AND 7» de la v0.9.24 ya no se aplican")
            end
        end
        -- v0.9.36: gestor de FOCO de menús a 16 (después del de cursores: con foco a 16, FUN_140E71620/71BE0/71E10
        -- llaman a FUN_140E67AA0(cursor, N ≥ 8), que necesita cursores de 16 o el «AND 7» de la v0.9.24)
        PHX.wmInicio()
        PHX.hud16Inicio()                                 -- v0.9.58: cursor/nombre y barra de los mandos 13–16
    end
    -- v0.8.6: el flujo de pausa propio YA NO se sirve desde el módulo (en v0.8.5 los eventos livecpk_make_key /
    -- livecpk_get_filepath coincidieron con un cierre de PES dentro de sider.dll +0x1B64 al arrancar). Ahora es un
    -- archivo de datos en livecpk\Menu\common\script\flow\Match\MatchPrePause.json (copia del original de ConmeGOL al
    -- lado, «.conmegol-original-20261020»).
    ctx.register("display_frame", m.display_frame)
    ctx.register("key_down", m.key_down)
    -- v0.9: gamepad_input ya no se registra: el mando del anfitrión lo recibe la pantalla de PES
    ctx.register("show", m.show)
    ctx.register("hide", m.hide)
    ctx.register("overlay_on", m.overlay_on)
    log("[fase1] v" .. m.version .. " cargada: pantalla «Elegir lado» en la ventana de Sider (I = activar mandos extra)")
end

return m
