# COMPROBAR_MI_PES.ps1 - SOLO LEE. No modifica, no instala, no descarga nada.
# Muestra el tamano y el SHA-256 de PES2021.exe y el tamano/fecha de la carpeta de datos del PES,
# para compararlos con los de Frank (VALORES_DE_FRANK.txt).
param([string]$Carpeta = "")

function Buscar-Pes {
    $candidatas = @(
        "C:\PES21\eFootball PES 2021",
        "C:\Program Files (x86)\Steam\steamapps\common\eFootball PES 2021 SEASON UPDATE",
        "C:\Program Files\Steam\steamapps\common\eFootball PES 2021 SEASON UPDATE",
        "D:\SteamLibrary\steamapps\common\eFootball PES 2021 SEASON UPDATE",
        "D:\Games\eFootball PES 2021"
    )
    foreach ($c in $candidatas) { if (Test-Path (Join-Path $c "PES2021.exe")) { return $c } }
    return $null
}

if (-not $Carpeta) { $Carpeta = Buscar-Pes }
if (-not $Carpeta) {
    $Carpeta = Read-Host "No encontre PES solo. Escribe la carpeta donde esta PES2021.exe"
}
$Carpeta = $Carpeta.Trim('"').Trim()
$exe = Join-Path $Carpeta "PES2021.exe"
if (-not (Test-Path $exe)) { Write-Host "No encuentro PES2021.exe en: $Carpeta" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "=== COMPROBAR MI PES (solo lectura) ===" -ForegroundColor Cyan
Write-Host "Carpeta: $Carpeta"
$i = Get-Item $exe
Write-Host ""
Write-Host "PES2021.exe"
Write-Host ("  Tamano : {0} bytes" -f $i.Length)
Write-Host ("  Fecha  : {0:yyyy-MM-dd HH:mm}" -f $i.LastWriteTime)
Write-Host "  SHA-256: calculando (puede tardar ~1 minuto)..."
$h = (Get-FileHash -Algorithm SHA256 -Path $exe).Hash
Write-Host "  SHA-256: $h"

foreach ($sub in @("Data", "download")) {
    $d = Join-Path $Carpeta $sub
    Write-Host ""
    if (-not (Test-Path $d)) { Write-Host "Carpeta ${sub}: no existe"; continue }
    $f = Get-ChildItem -Path $d -Recurse -File -ErrorAction SilentlyContinue
    $tot = ($f | Measure-Object Length -Sum).Sum
    $ult = ($f | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
    Write-Host ("Carpeta {0}: {1} archivos, {2} bytes, ultimo cambio {3:yyyy-MM-dd HH:mm}" -f $sub, $f.Count, $tot, $ult)
}

$dp = Join-Path $Carpeta "download\DpFileList.bin"
if (Test-Path $dp) {
    Write-Host ""
    Write-Host "download\DpFileList.bin"
    Write-Host ("  Tamano : {0} bytes" -f (Get-Item $dp).Length)
    Write-Host ("  SHA-256: {0}" -f (Get-FileHash -Algorithm SHA256 -Path $dp).Hash)
}

Write-Host ""
Write-Host "Archivos .cpk de download (nombre y tamano):"
Get-ChildItem -Path (Join-Path $Carpeta "download") -Filter *.cpk -File -ErrorAction SilentlyContinue |
    Sort-Object Name | ForEach-Object { Write-Host ("  {0}  {1}" -f $_.Name, $_.Length) }

Write-Host ""
Write-Host "Listo. No se cambio nada. Manda estos numeros a Frank." -ForegroundColor Green
