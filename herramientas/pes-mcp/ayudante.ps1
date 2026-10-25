# =============================================================================
#  Phoenix Evolution · ayudante de Windows del MCP de PES (lo llama servidor.mjs)
#  Acciones:
#    proceso                         -> ¿PES2021 y sider.exe abiertos? base del exe, ventana (JSON)
#    leer   -Expr "<expr>" -N <n>    -> lee n bytes (SOLO LECTURA) en la dirección de <expr> (JSON, hex)
#             <expr>: exe | 0x1234 | 1234 (hex) | [<expr>] (puntero de 8 bytes) | sumas y restas
#             ej.: "[exe+0x3704E38]+0xF0"
#    captura -Salida <png> [-Ancho 1280]  -> foto SOLO de la ventana de PES (PrintWindow)
#    tecla  -Teclas "Enter" | "Shift+R" [-Veces N]  -> pulsa teclas en la ventana de PES (SendInput)
#  La lectura abre el proceso solo con permiso de LEER (PROCESS_VM_READ | QUERY_INFORMATION):
#  Windows no deja escribir con ese permiso aunque hubiera un error en este código.
# =============================================================================
param(
    [Parameter(Mandatory = $true)][ValidateSet('proceso', 'leer', 'captura', 'tecla')][string]$Accion,
    [string]$Expr = '', [int]$N = 64,
    [string]$Salida = '', [int]$Ancho = 1280,
    [string]$Teclas = '', [int]$Veces = 1
)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$codigo = @"
using System; using System.Diagnostics; using System.Runtime.InteropServices; using System.Text; using System.Collections.Generic;
public static class PhxPes {
  [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenProcess(uint acceso, bool heredar, int pid);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool ReadProcessMemory(IntPtr h, IntPtr dir, byte[] buf, IntPtr n, out IntPtr leidos);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct R { public int L, T, Rr, B; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
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

  static IntPtr h; static long baseExe;

  public static bool Abrir() {
    Process[] ps = Process.GetProcessesByName("PES2021");
    if (ps.Length == 0) return false;
    baseExe = (long)ps[0].MainModule.BaseAddress;
    h = OpenProcess(0x0400 | 0x0010, false, ps[0].Id);   // QUERY_INFORMATION | VM_READ: solo leer
    return h != IntPtr.Zero;
  }
  public static long Base() { return baseExe; }
  public static byte[] Leer(long dir, int n) {
    byte[] b = new byte[n]; IntPtr r;
    if (!ReadProcessMemory(h, (IntPtr)dir, b, (IntPtr)n, out r) || (long)r < n) return null;
    return b;
  }

  // Expresión de dirección: término (+|- término)*; término = exe | número hex | [expresión]
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
    if (s.Length - i >= 3 && s.Substring(i, 3).ToLower() == "exe") { i += 3; return baseExe; }
    int ini = i;
    if (s.Length - i >= 2 && s.Substring(i, 2).ToLower() == "0x") { i += 2; ini = i; }
    while (i < s.Length && Uri.IsHexDigit(s[i])) i++;
    if (i == ini) throw new Exception("se esperaba un número hex en la posición " + i);
    return Convert.ToInt64(s.Substring(ini, i - ini), 16);
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
    Add-Type -TypeDefinition $codigo -OutputAssembly $dll -OutputType Library
    Set-Content -Path $firma -Value $hash -NoNewline
}
Add-Type -Path $dll
Add-Type -AssemblyName System.Drawing

function Json($o) { $o | ConvertTo-Json -Depth 5 -Compress }

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
    'leer' {
        if (-not $pes) { throw 'PES2021 no está abierto.' }
        if ($N -lt 1 -or $N -gt 4096) { throw 'N debe estar entre 1 y 4096.' }
        if (-not [PhxPes]::Abrir()) { throw 'No se pudo abrir PES2021 para leer.' }
        $dir = [PhxPes]::Resolver($Expr)
        $b = [PhxPes]::Leer($dir, $N)
        if ($null -eq $b) { Json @{ ok = $false; dir = ('0x{0:X}' -f $dir); motivo = 'zona ilegible (no existe o está protegida)' }; break }
        Json @{ ok = $true; dir = ('0x{0:X}' -f $dir); rva = $(if ($dir -ge [PhxPes]::Base() -and $dir -lt [PhxPes]::Base() + 0x10000000) { '0x{0:X}' -f ($dir - [PhxPes]::Base()) } else { 'fuera del exe' }); n = $N; hex = ([BitConverter]::ToString($b).Replace('-', '')) }
    }
    'captura' {
        if (-not $pes -or $pes.MainWindowHandle -eq [IntPtr]::Zero) { throw 'PES2021 no tiene ventana.' }
        $h = $pes.MainWindowHandle
        if ([PhxPes]::IsIconic($h)) { throw 'La ventana de PES está minimizada: no se puede fotografiar sin restaurarla (eso lo decide la persona).' }
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
            [void][PhxPes]::PrintWindow($h, $dc, 2); $g.ReleaseHdc($dc); $metodo = 'PrintWindow'
            $aviso = 'PES no está al frente: la imagen puede ser vieja. Con las acciones encendidas, una tecla inofensiva lo trae al frente.'
        }
        $g.Dispose()
        if ($Ancho -gt 0 -and $w -gt $Ancho) {
            $alto2 = [int]($al * $Ancho / $w)
            $chica = New-Object System.Drawing.Bitmap $Ancho, $alto2
            $g2 = [System.Drawing.Graphics]::FromImage($chica)
            $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $g2.DrawImage($bmp, 0, 0, $Ancho, $alto2); $g2.Dispose(); $bmp.Dispose(); $bmp = $chica
        }
        $bmp.Save($Salida, [System.Drawing.Imaging.ImageFormat]::Png)
        Json @{ ok = $true; ancho = $bmp.Width; alto = $bmp.Height; archivo = $Salida; metodo = $metodo; aviso = $aviso }
        $bmp.Dispose()
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
