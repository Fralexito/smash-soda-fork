# Crea el instalador de Phoenix Link en UN SOLO .exe (PhoenixLink-Setup-<version>.exe) desde la carpeta ya COMPILADA.
# Uso:  powershell -ExecutionPolicy Bypass -File tools\CREAR-INSTALADOR-EXE.ps1 [-Release <x64\Release>] [-Salida <carpeta>]
# Qué mete dentro: Phoenix Link (sin .pdb ni imgui.ini; overlay aparte), el overlay (PhoenixGlass.exe) y el instalador .bat
# de siempre. El .exe pide administrador, lo vuelca a una carpeta temporal y lo ejecuta apuntando a esos zips locales.
# No publica nada: solo deja el .exe (y su SHA256) en la carpeta de salida.
# Requiere que Phoenix Link y el overlay estén compilados y que Link esté CERRADO si hay que recompilar.
param(
    [string]$Release = '',
    [string]$Salida = 'C:\dev\phoenix-beta2-dist'
)
$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $PSScriptRoot
if (-not $Release) { $Release = Join-Path $raiz 'x64\Release' }
foreach ($f in 'PhoenixLink.exe', 'parsec.dll', 'SDL2.dll', 'WebView2Loader.dll', 'ui\index.html', 'sider\phoenix.lua', 'sider\phoenix_referee.lua', 'fichajes', 'overlay\PhoenixGlass.exe') {
    if (-not (Test-Path -LiteralPath (Join-Path $Release $f))) { throw "Falta ${f} en ${Release} (¿compilación incompleta?)" }
}
$h = Join-Path $raiz 'SmashSoda\phoenix\PhoenixBuild.h'
$ver = (Select-String -Path $h -Pattern 'kVersion = "([^"]+)"').Matches[0].Groups[1].Value
$csc = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path $csc)) { throw 'No se encontró csc.exe de .NET Framework.' }

$trabajo = Join-Path $env:TEMP ('phx-setup-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $trabajo | Out-Null
try {
    # 1) Phoenix Link (sin overlay, sin .pdb, sin imgui.ini)
    $stage = Join-Path $trabajo 'link'
    robocopy $Release $stage /E /XF *.pdb imgui.ini NotoSansSC*.ttf /XD overlay /NFL /NDL /NJH /NJS | Out-Null
    $zipLink = Join-Path $trabajo 'PhoenixLink.zip'
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zipLink -CompressionLevel Optimal

    # 2) Overlay: el zip lleva PhoenixGlass.exe en la raíz
    $zipOv = Join-Path $trabajo 'PhoenixGlass.zip'
    $exeOv = Join-Path $Release 'overlay\PhoenixGlass.exe'
    Compress-Archive -LiteralPath $exeOv -DestinationPath $zipOv -CompressionLevel Optimal

    # 3) Datos de verificación que lee el lanzador
    $datos = Join-Path $trabajo 'datos.txt'
    @(
        ('PHX_VERSION=' + $ver),
        ('PHX_ZIP_SHA=' + (Get-FileHash -LiteralPath $zipLink -Algorithm SHA256).Hash),
        ('PHX_OVERLAY_SHA=' + (Get-FileHash -LiteralPath $zipOv -Algorithm SHA256).Hash),
        ('PHX_OVERLAY_EXE_SHA=' + (Get-FileHash -LiteralPath $exeOv -Algorithm SHA256).Hash)
    ) | Set-Content -LiteralPath $datos -Encoding ascii

    # 4) Manifiesto: pedir administrador al abrir
    $manifiesto = Join-Path $trabajo 'app.manifest'
    @'
<?xml version="1.0" encoding="utf-8"?>
<assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1">
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security><requestedPrivileges><requestedExecutionLevel level="requireAdministrator" uiAccess="false"/></requestedPrivileges></security>
  </trustInfo>
  <application xmlns="urn:schemas-microsoft-com:asm.v3"><windowsSettings><dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true</dpiAware></windowsSettings></application>
</assembly>
'@ | Set-Content -LiteralPath $manifiesto -Encoding utf8

    # 5) Compilar el lanzador con todo incrustado
    New-Item -ItemType Directory -Force -Path $Salida | Out-Null
    $exe = Join-Path $Salida "PhoenixLink-Setup-$ver.exe"
    if (Test-Path -LiteralPath $exe) { Remove-Item -LiteralPath $exe -Force }
    $icono = Join-Path $raiz 'SmashSoda\icon.ico'
    & $csc /nologo /target:winexe /platform:x64 "/out:$exe" "/win32icon:$icono" "/win32manifest:$manifiesto" `
        /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll `
        "/resource:$zipLink,PhoenixLink.zip" "/resource:$zipOv,PhoenixGlass.zip" "/resource:$datos,datos.txt" "/resource:$icono,icono.ico" `
        (Join-Path $PSScriptRoot 'instalador\Stub.cs')
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe)) { throw 'No se pudo compilar el instalador.' }
    $sha = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    Set-Content -LiteralPath "$exe.sha256.txt" -Value "$sha  $(Split-Path -Leaf $exe)" -Encoding ascii
    Write-Host ("Instalador: " + $exe + "  (" + [math]::Round((Get-Item $exe).Length / 1MB, 1) + " MB)")
    Write-Host "SHA256: $sha"
}
finally {
    Remove-Item -LiteralPath $trabajo -Recurse -Force -ErrorAction SilentlyContinue
}
