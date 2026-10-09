# Phoenix Evolution · escribe un aviso para el módulo phoenix.lua de Sider (prueba 1).
# Escritura "atómica": primero a avisos.tmp y luego se reemplaza avisos.txt de una vez,
# así el juego nunca lee un archivo a medio escribir.
param(
    [string]$Texto = "",
    [string]$Carpeta = ""
)
$ErrorActionPreference = 'Stop'
if ($Carpeta -eq "") { $Carpeta = Join-Path $PSScriptRoot '..\SiderAddons\content\phoenix' }
$Carpeta = [IO.Path]::GetFullPath($Carpeta)
if (-not (Test-Path (Join-Path $Carpeta '..\..\sider.ini'))) {
    Write-Host "No encuentro sider.ini dos carpetas arriba de: $Carpeta" -ForegroundColor Red
    Write-Host "Pasa la carpeta correcta con -Carpeta '...\SiderAddons\content\phoenix'"
    exit 1
}
New-Item -ItemType Directory -Force -Path $Carpeta | Out-Null
if ($Texto -eq "") { $Texto = Read-Host 'Escribe el aviso (usa | para pasar a otra línea)' }
$Texto = ($Texto -replace '\s*\|\s*', "`n").Trim()
if ($Texto.Length -gt 1500) { $Texto = $Texto.Substring(0, 1500) }
$tmp = Join-Path $Carpeta 'avisos.tmp'
$dst = Join-Path $Carpeta 'avisos.txt'
[IO.File]::WriteAllText($tmp, $Texto + "`n", (New-Object Text.UTF8Encoding $false))
$hecho = $false
for ($i = 0; $i -lt 20 -and -not $hecho; $i++) {
    try {
        if (Test-Path $dst) { [IO.File]::Replace($tmp, $dst, $null) } else { [IO.File]::Move($tmp, $dst) }
        $hecho = $true
    } catch { Start-Sleep -Milliseconds 100 }   # el juego lo estaba leyendo justo en ese instante: se reintenta
}
if (-not $hecho) { Write-Host "No se pudo reemplazar avisos.txt (¿bloqueado?)." -ForegroundColor Red; exit 1 }
Write-Host "Aviso enviado. En el juego: Espacio para abrir el overlay y 1 hasta ver PHOENIX EVOLUTION." -ForegroundColor Green
