# =============================================================================
#  Phoenix Evolution · ayudante de Windows del MCP de PES (lo llama servidor.mjs)
#  Acciones:
#    proceso                         -> ¿PES2021 y sider.exe abiertos? base del exe, ventana (JSON)
#    salud                           -> lo anterior + ¿responde?, ventana colgada, CPU en 1 s, hilos
#    fallos  [-Dias 2]               -> cierres y cuelgues de PES/Sider/Core en el Visor de eventos de Windows
#    leer   -Expr "<expr>" -N <n>    -> lee n bytes (SOLO LECTURA) en la dirección de <expr> (JSON, hex)
#             <expr>: exe | modulo.dll | 0x1234 | 1234 (hex) | [<expr>] (puntero de 8 bytes) | sumas y restas
#             ej.: "[exe+0x3704E38]+0xF0", "phoenix_core.dll+0x1A20"
#    firma  -Patron "48 8B ?? .." [-N 20] [-Todo] [-Modulo x.dll] [-Desp d -Fin f]
#                                    -> busca bytes con comodines en el código (o en todo con -Todo)
#    modulos [-Todo]                 -> DLL cargadas en PES (base y tamaño)
#    escribir -Expr "<expr>" -Hex "90 90" -Esperado "74 05"
#                                    -> escribe SOLO si lo que hay ahora es -Esperado (lo decide servidor.mjs)
#    captura -Salida <png> [-Ancho 1280] [-Veces 1] [-Intervalo 500]  -> foto(s) SOLO de la ventana de PES
#             al frente: copia de la pantalla · al fondo: Windows Graphics Capture (captura_wgc.cs) · si no, PrintWindow
#    tecla  -Teclas "Enter" | "Shift+R" [-Veces N]  -> pulsa teclas en la ventana de PES (SendInput)
#  Todas las lecturas abren el proceso solo con permiso de LEER (PROCESS_VM_READ | QUERY_INFORMATION).
#  Solo «escribir» lo abre con permiso de escribir, y servidor.mjs solo la llama si config.json lo permite.
# =============================================================================
param(
    [Parameter(Mandatory = $true)][ValidateSet('proceso', 'salud', 'fallos', 'leer', 'firma', 'modulos', 'escribir', 'captura', 'tecla')][string]$Accion,
    [string]$Expr = '', [int]$N = 64,
    [string]$Salida = '', [int]$Ancho = 1280, [int]$Intervalo = 500,
    [string]$Teclas = '', [int]$Veces = 1,
    [string]$Patron = '', [switch]$Todo, [string]$Modulo = '', [int]$Desp = -1, [int]$Fin = -1,
    [string]$Hex = '', [string]$Esperado = '',
    [int]$Dias = 2
)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$codigo = @"
using System; using System.Diagnostics; using System.Runtime.InteropServices; using System.Text; using System.Collections.Generic;
public static class PhxPes {
  [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenProcess(uint acceso, bool heredar, int pid);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool ReadProcessMemory(IntPtr h, IntPtr dir, byte[] buf, IntPtr n, out IntPtr leidos);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool WriteProcessMemory(IntPtr h, IntPtr dir, byte[] buf, IntPtr n, out IntPtr escritos);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool VirtualProtectEx(IntPtr h, IntPtr dir, UIntPtr n, uint nueva, out uint vieja);
  [DllImport("kernel32.dll")] static extern bool FlushInstructionCache(IntPtr h, IntPtr dir, UIntPtr n);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct R { public int L, T, Rr, B; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsHungAppWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] static extern void keybd_event(byte vk, byte scan, uint flags, IntPtr extra);
  // Windows no deja que un proceso de fondo se ponga al frente; un Alt «suelto» levanta ese bloqueo.
  public static bool Traer(IntPtr h) {
    if (IsIconic(h)) ShowWindow(h, 9);
    for (int k = 0; k < 3; k++) {
      if (GetForegroundWindow() == h) return true;
      keybd_event(0x12, 0x38, 0, IntPtr.Zero); keybd_event(0x12, 0x38, 2, IntPtr.Zero);
      SetForegroundWindow(h); System.Threading.Thread.Sleep(500);
    }
    return GetForegroundWindow() == h;
  }
  [StructLayout(LayoutKind.Sequential)] public struct KI { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr extra; }
  [StructLayout(LayoutKind.Sequential)] public struct INP { public uint type; public KI ki; public uint pad1, pad2; }
  [DllImport("user32.dll")] static extern uint SendInput(uint n, INP[] i, int size);

  static IntPtr h; static long baseExe; static int pid;

  // escribir = false: QUERY_INFORMATION | VM_READ (Windows no deja escribir con este permiso).
  // escribir = true: además VM_OPERATION | VM_WRITE (solo para la acción «escribir»).
  public static bool Abrir(bool escribir) {
    Process[] ps = Process.GetProcessesByName("PES2021");
    if (ps.Length == 0) return false;
    pid = ps[0].Id;
    baseExe = (long)ps[0].MainModule.BaseAddress;
    h = OpenProcess(0x0400 | 0x0010 | (escribir ? 0x0008u | 0x0020u : 0u), false, pid);
    return h != IntPtr.Zero;
  }
  public static long Base() { return baseExe; }
  public static int Pid() { return pid; }
  public static byte[] Leer(long dir, int n) {
    byte[] b = new byte[n]; IntPtr r;
    if (!ReadProcessMemory(h, (IntPtr)dir, b, (IntPtr)n, out r) || (long)r < n) return null;
    return b;
  }
  static bool Iguales(byte[] a, byte[] b) { if (a.Length != b.Length) return false; for (int k = 0; k < a.Length; k++) if (a[k] != b[k]) return false; return true; }
  // Escribe solo si lo que hay ahora es «esp». Devuelve null si fue bien, o el motivo.
  public static string Escribir(long dir, byte[] d, byte[] esp, out byte[] antes) {
    antes = Leer(dir, d.Length);
    if (antes == null) return "zona ilegible (no existe o está protegida)";
    if (esp == null || !Iguales(antes, esp)) return "los bytes de ahora no son los esperados: no se escribió nada";
    uint vieja; bool cambio = VirtualProtectEx(h, (IntPtr)dir, (UIntPtr)d.Length, 0x40, out vieja);
    IntPtr esc; bool ok = WriteProcessMemory(h, (IntPtr)dir, d, (IntPtr)d.Length, out esc);
    int error = Marshal.GetLastWin32Error();
    if (cambio) { uint x; VirtualProtectEx(h, (IntPtr)dir, (UIntPtr)d.Length, vieja, out x); }
    FlushInstructionCache(h, (IntPtr)dir, (UIntPtr)d.Length);
    if (!ok) return "Windows no dejó escribir (error " + error + ")";
    byte[] ahora = Leer(dir, d.Length);
    if (ahora == null || !Iguales(ahora, d)) return "se escribió, pero al releer no coincide (el juego lo cambió al momento)";
    return null;
  }

  // Módulos cargados (nombre -> base, tamaño).
  public static long ModBase(string nombre, out long tam) {
    foreach (ProcessModule m in Process.GetProcessById(pid).Modules)
      if (string.Equals(m.ModuleName, nombre, StringComparison.OrdinalIgnoreCase)) { tam = m.ModuleMemorySize; return (long)m.BaseAddress; }
    throw new Exception("el módulo " + nombre + " no está cargado en PES");
  }
  // «exe+0x…», «sider.dll+0x…» o «fuera de los módulos».
  public static string Donde(long dir) {
    foreach (ProcessModule m in Process.GetProcessById(pid).Modules) {
      long b = (long)m.BaseAddress;
      if (dir >= b && dir < b + m.ModuleMemorySize) return (b == baseExe ? "exe" : m.ModuleName) + "+0x" + (dir - b).ToString("X");
    }
    return "fuera de los módulos (memoria del juego)";
  }

  // Expresión de dirección: término (+|- término)*; término = exe | nombre.dll | número hex | [expresión]
  static string s; static int i;
  public static long Resolver(string expr) { s = expr.Replace(" ", ""); i = 0; long v = Suma(); if (i != s.Length) throw new Exception("expresión rara cerca de '" + s.Substring(i) + "'"); return v; }
  static long Suma() {
    long v = Termino();
    while (i < s.Length && (s[i] == '+' || s[i] == '-')) { char op = s[i++]; long t = Termino(); v = op == '+' ? v + t : v - t; }
    return v;
  }
  static long Termino() {
    if (i < s.Length && s[i] == '[') {
      i++; long d = Suma();
      if (i >= s.Length || s[i] != ']') throw new Exception("falta ']'");
      i++;
      byte[] b = Leer(d, 8);
      if (b == null) throw new Exception("no se pudo leer el puntero en 0x" + d.ToString("X"));
      return BitConverter.ToInt64(b, 0);
    }
    int j = i;
    while (j < s.Length && (char.IsLetterOrDigit(s[j]) || s[j] == '_' || s[j] == '.')) j++;
    string tok = s.Substring(i, j - i).ToLower();
    if (tok.EndsWith(".dll") || tok.EndsWith(".exe")) { i = j; long t; return ModBase(tok, out t); }
    if (tok == "exe") { i = j; return baseExe; }
    int ini = i;
    if (s.Length - i >= 2 && s.Substring(i, 2).ToLower() == "0x") { i += 2; ini = i; }
    while (i < s.Length && Uri.IsHexDigit(s[i])) i++;
    if (i == ini) throw new Exception("se esperaba un número hex en la posición " + i);
    return Convert.ToInt64(s.Substring(ini, i - ini), 16);
  }

  // Firma (AOB): «48 8B 05 ?? ?? ?? ?? C3». Recorre las secciones de código del módulo (o todas con «todo»).
  public static List<long> Firma(string patron, int max, bool todo, long modBase) {
    string[] partes = patron.Trim().Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
    byte[] p = new byte[partes.Length]; bool[] fijo = new bool[partes.Length];
    for (int k = 0; k < partes.Length; k++) { fijo[k] = !partes[k].StartsWith("?"); if (fijo[k]) p[k] = Convert.ToByte(partes[k], 16); }
    int primero = Array.IndexOf(fijo, true);
    if (primero < 0) throw new Exception("la firma necesita al menos un byte fijo");
    List<long> res = new List<long>();
    byte[] cab = Leer(modBase, 0x1000);
    if (cab == null) throw new Exception("no se pudo leer la cabecera del módulo");
    int e = BitConverter.ToInt32(cab, 0x3C);
    int nsec = BitConverter.ToUInt16(cab, e + 6), opt = BitConverter.ToUInt16(cab, e + 20);
    for (int k = 0; k < nsec && res.Count < max; k++) {
      int sec = e + 24 + opt + k * 40;
      uint vtam = BitConverter.ToUInt32(cab, sec + 8), va = BitConverter.ToUInt32(cab, sec + 12), car = BitConverter.ToUInt32(cab, sec + 36);
      if (!todo && (car & 0x20000000) == 0) continue;
      Buscar(modBase + va, vtam, p, fijo, primero, max, res);
    }
    return res;
  }
  static void Buscar(long ini, long tam, byte[] p, bool[] fijo, int primero, int max, List<long> res) {
    const int TROZO = 4 << 20;
    for (long off = 0; off < tam && res.Count < max; off += TROZO) {
      int n = (int)Math.Min(TROZO + p.Length - 1, tam - off);
      byte[] b = Leer(ini + off, n);
      if (b == null) { // alguna página no se deja leer: se recorre en bloques de 64 KB y se saltan los ilegibles
        for (int sub = 0; sub < n && res.Count < max; sub += 65536) {
          int m = Math.Min(65536 + p.Length - 1, n - sub);
          byte[] c = Leer(ini + off + sub, m);
          if (c != null) Coincidir(c, ini + off + sub, p, fijo, primero, max, res);
        }
        continue;
      }
      Coincidir(b, ini + off, p, fijo, primero, max, res);
    }
  }
  static void Coincidir(byte[] b, long dir0, byte[] p, bool[] fijo, int primero, int max, List<long> res) {
    for (int k = 0; k + p.Length <= b.Length && res.Count < max; k++) {
      if (b[k + primero] != p[primero]) continue;
      bool ok = true;
      for (int q = 0; q < p.Length; q++) if (fijo[q] && b[k + q] != p[q]) { ok = false; break; }
      if (ok && !res.Contains(dir0 + k)) res.Add(dir0 + k);
    }
  }

  public static void Tecla(ushort scan, bool ext, bool soltar) {
    INP[] a = new INP[1];
    a[0].type = 1; a[0].ki.wScan = scan; a[0].ki.dwFlags = 0x8 | (soltar ? 0x2u : 0u) | (ext ? 1u : 0u);
    SendInput(1, a, Marshal.SizeOf(typeof(INP)));
  }
}
"@

# El C# se compila una vez y se guarda junto al script (así cada llamada tarda menos).
$dll = Join-Path $PSScriptRoot 'ayudante.dll'
$firma = Join-Path $PSScriptRoot 'ayudante.firma'
$hash = [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($codigo))).Replace('-', '')
if (-not (Test-Path $dll) -or -not (Test-Path $firma) -or (Get-Content $firma -Raw).Trim() -ne $hash) {
    # Si la versión vieja de la DLL está cargada por otra llamada, se compila con otro nombre.
    try { Add-Type -TypeDefinition $codigo -OutputAssembly $dll -OutputType Library }
    catch { $dll = Join-Path $PSScriptRoot ('ayudante-' + $hash.Substring(0, 8) + '.dll'); if (-not (Test-Path $dll)) { Add-Type -TypeDefinition $codigo -OutputAssembly $dll -OutputType Library } }
    if ($dll -like '*ayudante.dll') { Set-Content -Path $firma -Value $hash -NoNewline }
}
Add-Type -Path $dll
Add-Type -AssemblyName System.Drawing

function Json($o) { $o | ConvertTo-Json -Depth 6 -Compress }
# Windows Graphics Capture (captura_wgc.cs): se compila la primera vez que hace falta, aparte del ayudante,
# para que si este Windows no la soporta el resto siga funcionando.
function Cargar-Wgc {
    if ('PhxWgc' -as [type]) { return }
    $cs = Join-Path $PSScriptRoot 'captura_wgc.cs'
    if (-not (Test-Path $cs)) { throw 'falta captura_wgc.cs' }
    $src = Get-Content $cs -Raw
    $hw = [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($src))).Replace('-', '').Substring(0, 12)
    $dllW = Join-Path $PSScriptRoot "captura_wgc-$hw.dll"
    if (-not (Test-Path $dllW)) {
        # Add-Type no acepta los .winmd como referencia (intenta cargarlos y falla: probado el 29-oct en Windows 11).
        # El compilador csc.exe de .NET Framework sí, así que se compila con él y solo con los .winmd necesarios.
        $fw = [Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory()
        $wm = "$env:SystemRoot\System32\WinMetadata"
        $refs = @(@('Windows.Foundation.winmd', 'Windows.Graphics.winmd', 'Windows.Storage.winmd') | ForEach-Object { Join-Path $wm $_ } | Where-Object { Test-Path $_ })
        if ($refs.Count -lt 3) { $refs = @(Get-ChildItem "$wm\*.winmd" | ForEach-Object FullName) }  # Windows antiguo: un solo Windows.winmd
        $refs += @('System.Runtime.WindowsRuntime.dll', 'System.Runtime.dll', 'System.Runtime.InteropServices.WindowsRuntime.dll', 'System.Threading.Tasks.dll', 'System.Drawing.dll') | ForEach-Object { Join-Path $fw $_ } | Where-Object { Test-Path $_ }
        $salidaCsc = & (Join-Path $fw 'csc.exe') /nologo /target:library /optimize "/out:$dllW" ($refs | ForEach-Object { "/r:$_" }) $cs 2>&1
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path $dllW)) {
            $t = (($salidaCsc | Out-String) -replace '\s+', ' ').Trim(); if ($t.Length -gt 600) { $t = $t.Substring(0, 600) }
            throw "no compiló la captura de Windows: $t"
        }
    }
    Add-Type -Path $dllW
}
function Hex2Bytes([string]$h) {
    $h = $h -replace '\s', ''
    if ($h.Length % 2 -ne 0 -or $h -notmatch '^[0-9a-fA-F]*$') { throw "hex no válido: $h" }
    $b = New-Object byte[] ($h.Length / 2)
    for ($k = 0; $k -lt $b.Length; $k++) { $b[$k] = [Convert]::ToByte($h.Substring($k * 2, 2), 16) }
    , $b
}
function Bytes2Hex($b) { [BitConverter]::ToString($b).Replace('-', '') }

$pes = Get-Process PES2021 -ErrorAction SilentlyContinue | Select-Object -First 1

switch ($Accion) {
    'proceso' {
        $sider = Get-Process sider -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $pes) { Json @{ pes = $false; sider = [bool]$sider }; break }
        $base = '0x{0:X}' -f [int64]$pes.MainModule.BaseAddress
        Json @{
            pes = $true; sider = [bool]$sider; pid = $pes.Id; base_exe = $base
            ventana = ($pes.MainWindowHandle -ne [IntPtr]::Zero); titulo = $pes.MainWindowTitle
            memoria_mb = [math]::Round($pes.WorkingSet64 / 1MB); abierto_desde = $pes.StartTime.ToString('s')
            responde = $pes.Responding
        }
    }
    'salud' {
        $sider = Get-Process sider -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $pes) { Json @{ pes = $false; sider = [bool]$sider }; break }
        $cpu1 = $pes.TotalProcessorTime.TotalSeconds; Start-Sleep -Milliseconds 1000; $pes.Refresh()
        $h = $pes.MainWindowHandle
        Json @{
            pes = $true; sider = [bool]$sider; pid = $pes.Id; responde = $pes.Responding
            colgada = $(if ($h -ne [IntPtr]::Zero) { [PhxPes]::IsHungAppWindow($h) } else { $null })
            minimizada = $(if ($h -ne [IntPtr]::Zero) { [PhxPes]::IsIconic($h) } else { $null })
            al_frente = ($h -ne [IntPtr]::Zero -and [PhxPes]::GetForegroundWindow() -eq $h)
            cpu_seg_por_seg = [math]::Round($pes.TotalProcessorTime.TotalSeconds - $cpu1, 2)
            hilos = $pes.Threads.Count; memoria_mb = [math]::Round($pes.WorkingSet64 / 1MB)
            abierto_hace_min = [math]::Round(((Get-Date) - $pes.StartTime).TotalMinutes, 1)
        }
    }
    'fallos' {
        $ev = Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000, 1001, 1002; StartTime = (Get-Date).AddDays(-$Dias) } -ErrorAction SilentlyContinue |
            Where-Object { $_.Message -match 'PES2021|sider|phoenix' } | Select-Object -First 25
        $lista = @(foreach ($e in $ev) {
            $p = $e.Properties | ForEach-Object { "$($_.Value)" }
            $x = [ordered]@{ hora = $e.TimeCreated.ToString('s'); id = $e.Id }
            switch ($e.Id) {
                1000 { $x.tipo = 'cierre'; $x.programa = $p[0]; $x.modulo = $p[3]; $x.version_modulo = $p[4]; $x.excepcion = $p[6]; $x.desplazamiento = $p[7] }
                1002 { $x.tipo = 'cuelgue (Windows lo dio por colgado)'; $x.programa = $p[0] }
                1001 { $x.tipo = 'informe de Windows (WER)'; $x.evento = $p[2]; $x.datos = ($p[5..9] -join ' | ') }
            }
            $x
        })
        Json @{ dias = $Dias; fallos = $lista }
    }
    'leer' {
        if (-not $pes) { throw 'PES2021 no está abierto.' }
        if ($N -lt 1 -or $N -gt 4096) { throw 'N debe estar entre 1 y 4096.' }
        if (-not [PhxPes]::Abrir($false)) { throw 'No se pudo abrir PES2021 para leer.' }
        $dir = [PhxPes]::Resolver($Expr)
        $b = [PhxPes]::Leer($dir, $N)
        if ($null -eq $b) { Json @{ ok = $false; dir = ('0x{0:X}' -f $dir); motivo = 'zona ilegible (no existe o está protegida)' }; break }
        Json @{ ok = $true; dir = ('0x{0:X}' -f $dir); donde = [PhxPes]::Donde($dir); rva = $(if ($dir -ge [PhxPes]::Base() -and $dir -lt [PhxPes]::Base() + 0x10000000) { '0x{0:X}' -f ($dir - [PhxPes]::Base()) } else { 'fuera del exe' }); n = $N; hex = (Bytes2Hex $b) }
    }
    'firma' {
        if (-not $pes) { throw 'PES2021 no está abierto.' }
        if (-not [PhxPes]::Abrir($false)) { throw 'No se pudo abrir PES2021 para leer.' }
        $tam = [long]0
        $mb = $(if ($Modulo) { [PhxPes]::ModBase($Modulo, [ref]$tam) } else { [PhxPes]::Base() })
        $nombre = $(if ($Modulo) { $Modulo } else { 'exe' })
        $reloj = [Diagnostics.Stopwatch]::StartNew()
        $hallados = [PhxPes]::Firma($Patron, [math]::Max(1, $N), [bool]$Todo, $mb)
        $lista = @(foreach ($d in $hallados) {
            $x = [ordered]@{ donde = "$nombre+0x{0:X}" -f ($d - $mb); dir = '0x{0:X}' -f $d }
            if ($Desp -ge 0 -and $Fin -gt 0) {
                $r = [PhxPes]::Leer($d + $Desp, 4)
                if ($r) { $dest = $d + $Fin + [BitConverter]::ToInt32($r, 0); $x.destino = "$nombre+0x{0:X}" -f ($dest - $mb); $x.destino_dir = '0x{0:X}' -f $dest }
            }
            $x
        })
        Json @{ patron = $Patron; modulo = $nombre; secciones = $(if ($Todo) { 'todas' } else { 'código' }); coincidencias = $lista.Count; ms = $reloj.ElapsedMilliseconds; lista = $lista }
    }
    'modulos' {
        if (-not $pes) { throw 'PES2021 no está abierto.' }
        $win = $env:SystemRoot.ToLower()
        $lista = @(foreach ($m in $pes.Modules) {
            if (-not $Todo -and $m.FileName.ToLower().StartsWith($win)) { continue }
            [ordered]@{ nombre = $m.ModuleName; base = '0x{0:X}' -f [int64]$m.BaseAddress; kb = [math]::Round($m.ModuleMemorySize / 1KB); ruta = $m.FileName }
        })
        Json @{ total = $pes.Modules.Count; mostrados = $lista.Count; modulos = $lista }
    }
    'escribir' {
        if (-not $pes) { throw 'PES2021 no está abierto.' }
        $datos = Hex2Bytes $Hex; $esp = Hex2Bytes $Esperado
        if ($datos.Length -lt 1 -or $datos.Length -gt 256) { throw 'Entre 1 y 256 bytes.' }
        if (-not [PhxPes]::Abrir($true)) { throw 'Windows no dejó abrir PES2021 con permiso de escribir.' }
        $dir = [PhxPes]::Resolver($Expr)
        $antes = $null
        $motivo = [PhxPes]::Escribir($dir, $datos, $esp, [ref]$antes)
        Json @{ ok = ($null -eq $motivo); motivo = $motivo; pid = [PhxPes]::Pid(); dir = ('0x{0:X}' -f $dir); donde = [PhxPes]::Donde($dir); rva = $(if ($dir -ge [PhxPes]::Base() -and $dir -lt [PhxPes]::Base() + 0x10000000) { '0x{0:X}' -f ($dir - [PhxPes]::Base()) } else { 'fuera del exe' }); antes = $(if ($antes) { Bytes2Hex $antes } else { $null }); ahora = $(if ($null -eq $motivo) { $Hex.Replace(' ', '').ToUpper() } else { $null }) }
    }
    'captura' {
        if (-not $pes -or $pes.MainWindowHandle -eq [IntPtr]::Zero) { throw 'PES2021 no tiene ventana.' }
        $h = $pes.MainWindowHandle
        if ([PhxPes]::IsIconic($h)) { throw 'La ventana de PES está minimizada: no se puede fotografiar sin restaurarla (eso lo decide la persona).' }
        $archivos = @()
        $total = [math]::Min([math]::Max($Veces, 1), 6)
        for ($k = 1; $k -le $total; $k++) {
            $r = New-Object PhxPes+R; [void][PhxPes]::GetWindowRect($h, [ref]$r)
            $w = $r.Rr - $r.L; $al = $r.B - $r.T
            if ($w -lt 320 -or $al -lt 200) { throw "La ventana de PES mide ${w}x${al}: está minimizada o escondida." }
            $bmp = New-Object System.Drawing.Bitmap $w, $al
            $g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
            # Con DirectX, PrintWindow devuelve una imagen VIEJA o negra (probado el 25-oct: la selección del menú se movía
            # y la foto no). Si PES está al frente se copia ese rectángulo de la pantalla, que sí es lo que se ve ahora.
            $aviso = $null
            if ([PhxPes]::GetForegroundWindow() -eq $h) {
                $g.ReleaseHdc($dc); $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size); $metodo = 'pantalla'
            } else {
                # Al fondo: primero Windows Graphics Capture (lo último que el juego dibujó); si falla, PrintWindow.
                $g.ReleaseHdc($dc); $wgc = $null; $errWgc = $null
                try { Cargar-Wgc; $wgc = [PhxWgc]::Capturar($h, 2500) } catch { $errWgc = $_.Exception.InnerException; if (-not $errWgc) { $errWgc = $_.Exception }; $errWgc = $errWgc.Message }
                if ($wgc) { $viejo = $bmp; $bmp = $wgc; $w = $bmp.Width; $al = $bmp.Height; $metodo = 'wgc' }
                else {
                    $dc = $g.GetHdc(); [void][PhxPes]::PrintWindow($h, $dc, 2); $g.ReleaseHdc($dc); $metodo = 'PrintWindow'
                    $aviso = "PES no está al frente y la captura de Windows falló ($errWgc): la imagen puede ser vieja."
                }
            }
            $g.Dispose(); if ($viejo) { $viejo.Dispose(); $viejo = $null }
            if ($Ancho -gt 0 -and $w -gt $Ancho) {
                $alto2 = [int]($al * $Ancho / $w)
                $chica = New-Object System.Drawing.Bitmap $Ancho, $alto2
                $g2 = [System.Drawing.Graphics]::FromImage($chica)
                $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $g2.DrawImage($bmp, 0, 0, $Ancho, $alto2); $g2.Dispose(); $bmp.Dispose(); $bmp = $chica
            }
            $f = $(if ($total -eq 1) { $Salida } else { $Salida -replace '\.png$', "_$k.png" })
            $bmp.Save($f, [System.Drawing.Imaging.ImageFormat]::Png)
            $archivos += $f; $ancho2 = $bmp.Width; $alto3 = $bmp.Height
            $bmp.Dispose()
            if ($k -lt $total) { Start-Sleep -Milliseconds $Intervalo }
        }
        Json @{ ok = $true; ancho = $ancho2; alto = $alto3; archivo = $archivos[0]; archivos = $archivos; metodo = $metodo; aviso = $aviso }
    }
    'tecla' {
        if (-not $pes -or $pes.MainWindowHandle -eq [IntPtr]::Zero) { throw 'PES2021 no tiene ventana.' }
        # Teclas permitidas (código de escaneo, ¿extendida?). La X NO está: en repeticiones abre «Guardar repetición».
        $mapa = @{
            Enter = @(0x1C, $false); Esc = @(0x01, $false); Espacio = @(0x39, $false); Tab = @(0x0F, $false); Retroceso = @(0x0E, $false)
            Arriba = @(0x48, $true); Abajo = @(0x50, $true); Izquierda = @(0x4B, $true); Derecha = @(0x4D, $true)
            Shift = @(0x2A, $false); Ctrl = @(0x1D, $false)
            Q = @(0x10, $false); W = @(0x11, $false); E = @(0x12, $false); R = @(0x13, $false); A = @(0x1E, $false); S = @(0x1F, $false); D = @(0x20, $false)
            Z = @(0x2C, $false); C = @(0x2E, $false); V = @(0x2F, $false); F = @(0x21, $false)
            '1' = @(0x02, $false); '2' = @(0x03, $false)
        }
        $partes = $Teclas.Split('+') | ForEach-Object { $_.Trim() }
        foreach ($p in $partes) { if (-not $mapa.ContainsKey($p)) { throw "Tecla no permitida: $p" } }
        # Las teclas solo se mandan si PES está AL FRENTE: si no, irían a otra ventana.
        $estabaMin = [PhxPes]::IsIconic($pes.MainWindowHandle)
        if (-not [PhxPes]::Traer($pes.MainWindowHandle)) { throw 'PES no se pudo poner al frente: no se pulsó nada (para no escribir en otra ventana).' }
        Start-Sleep -Milliseconds $(if ($estabaMin) { 2500 } else { 600 })
        for ($k = 0; $k -lt [math]::Min([math]::Max($Veces, 1), 20); $k++) {
            if ([PhxPes]::GetForegroundWindow() -ne $pes.MainWindowHandle) { throw "PES dejó de estar al frente tras $k pulsación(es): se paró." }
            foreach ($p in $partes) { [PhxPes]::Tecla([uint16]$mapa[$p][0], [bool]$mapa[$p][1], $false); Start-Sleep -Milliseconds 40 }
            Start-Sleep -Milliseconds 60
            [array]::Reverse($partes)
            foreach ($p in $partes) { [PhxPes]::Tecla([uint16]$mapa[$p][0], [bool]$mapa[$p][1], $true); Start-Sleep -Milliseconds 20 }
            [array]::Reverse($partes)
            Start-Sleep -Milliseconds 250
        }
        Json @{ ok = $true; teclas = $Teclas; veces = $Veces }
    }
}
