# Phoenix Core · deshacer la función «coop_online» (arreglo del Cooperativo 1v1).
# Uso (con PES CERRADO):  powershell -ExecutionPolicy Bypass -File DESHACER_COOP.ps1 -Sider "D:\Juegos\PES2021\SiderAddons"
# Qué hace: guarda copia de phoenix_core.json y deja "coop_online": false. NO borra la dll ni toca el exe: los ganchos son
# solo de memoria y desaparecen al cerrar PES. Para volver a la dll anterior a esta función, con PES cerrado copia
# respaldo_coop\phoenix_core.antes_coop.dll sobre SiderAddons\phoenix_core\phoenix_core.dll (-RestaurarDll).
param([Parameter(Mandatory)][string]$Sider, [switch]$RestaurarDll)
$ErrorActionPreference = 'Stop'
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
if (Get-Process PES2021 -ErrorAction SilentlyContinue) { throw 'PES está abierto: ciérralo antes de deshacer.' }
$cfg = Join-Path $Sider 'phoenix_core\phoenix_core.json'
if (Test-Path $cfg) {
    $copia = "$cfg.antes-deshacer-coop-" + (Get-Date -Format 'yyyyMMdd-HHmmss')
    Copy-Item -Force $cfg $copia
    $txt = [IO.File]::ReadAllText($cfg)
    $nuevo = [regex]::Replace($txt, '("coop_online"\s*:\s*)true', '${1}false')
    if ($nuevo -ne $txt) { [IO.File]::WriteAllText($cfg, $nuevo); Write-Host "coop_online apagada. Copia: $copia" }
    else { Write-Host 'coop_online ya estaba apagada (o no figuraba en el json): no cambio nada.' }
} else { Write-Host 'No hay phoenix_core.json: la función ya está apagada por defecto.' }
if ($RestaurarDll) {
    $vieja = Join-Path $aqui 'respaldo_coop\phoenix_core.antes_coop.dll'
    $dest = Join-Path $Sider 'phoenix_core\phoenix_core.dll'
    if (-not (Test-Path $vieja)) { throw 'No encuentro respaldo_coop\phoenix_core.antes_coop.dll' }
    if (Test-Path $dest) { Copy-Item -Force $dest "$dest.antes-deshacer-coop-" + (Get-Date -Format 'yyyyMMdd-HHmmss') }
    Copy-Item -Force $vieja $dest
    Write-Host 'dll anterior restaurada.'
}
