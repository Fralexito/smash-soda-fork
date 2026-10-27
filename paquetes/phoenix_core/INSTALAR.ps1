# Phoenix Core · fase 0 · instalador sin Phoenix Link.
# Uso (con PES CERRADO):  clic derecho › «Ejecutar con PowerShell», o:
#     powershell -ExecutionPolicy Bypass -File INSTALAR.ps1 -Sider "D:\Juegos\PES2021\SiderAddons"
# Copia el paquete (incluida phoenix_core.dll ya compilada) y añade el cargador a sider.ini
# ANTES de la línea de los mandos (esa tiene que seguir siendo la última). Guarda copia: sider.ini.antes-phoenix-core-AAAAMMDD-HHMMSS.
param([string]$Sider)
$ErrorActionPreference = 'Stop'
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Sider) { $Sider = Read-Host 'Carpeta de Sider (la que tiene sider.ini y la carpeta modules)' }
$ini = Join-Path $Sider 'sider.ini'
if (-not (Test-Path $ini)) { throw "No encuentro sider.ini en $Sider" }
if (Get-Process PES2021 -ErrorAction SilentlyContinue) { throw 'PES está abierto: ciérralo antes de instalar.' }
if (-not (Test-Path (Join-Path $aqui 'SiderAddons\phoenix_core\phoenix_core.dll'))) { throw 'Falta SiderAddons\phoenix_core\phoenix_core.dll: compílala primero (ver LEEME.md).' }

# 1) archivos (no pisa un phoenix_core.json que ya exista: es la configuración del jugador)
$cfg = Join-Path $Sider 'phoenix_core\phoenix_core.json'
$cfgGuardada = if (Test-Path $cfg) { [IO.File]::ReadAllBytes($cfg) } else { $null }
Copy-Item -Recurse -Force (Join-Path $aqui 'SiderAddons\*') $Sider
if ($cfgGuardada) { [IO.File]::WriteAllBytes($cfg, $cfgGuardada) }
Write-Host 'Archivos copiados.'

# 2) sider.ini (Latin-1 byte a byte, como el de los mandos)
$enc = [Text.Encoding]::GetEncoding(28591)
$copia = "$ini.antes-phoenix-core-" + (Get-Date -Format 'yyyyMMdd-HHmmss')
Copy-Item $ini $copia
$lineas = [Collections.Generic.List[string]]([IO.File]::ReadAllText($ini, $enc) -split "`r?`n")
$i = $lineas.FindIndex([Predicate[string]]{ param($l) $l -match '^\s*;?\s*luajit\.ext\.enabled\s*=' })
if ($i -ge 0) { $lineas[$i] = 'luajit.ext.enabled = 1' } else { $lineas.Add('luajit.ext.enabled = 1') }
[void]$lineas.RemoveAll([Predicate[string]]{ param($l) $l -match 'phoenix_core_loader\.lua|phoenix_marcador_free\.lua' })
$mods = @('lua.module = "phoenix_core_loader.lua"', 'lua.module = "phoenix_marcador_free.lua"')   # marcador del Entrenamiento libre (apagado salvo "marcador_free": true en phoenix_core.json)
$mandos = $lineas.FindIndex([Predicate[string]]{ param($l) $l -match '^\s*lua\.module\s*=.*phoenix_mandos' })
if ($mandos -ge 0) { $lineas.InsertRange($mandos, [string[]]$mods) }
else {
    $ultima = -1; for ($k = 0; $k -lt $lineas.Count; $k++) { if ($lineas[$k] -match '^\s*lua\.module\s*=') { $ultima = $k } }
    if ($ultima -ge 0) { $lineas.InsertRange($ultima + 1, [string[]]$mods) } else { $lineas.AddRange([string[]]$mods) }
}
[IO.File]::WriteAllText($ini, ($lineas -join "`r`n"), $enc)
Write-Host "sider.ini ajustado (copia en $copia)."
Write-Host 'Listo. Abre PES con Sider y mira SiderAddons\phoenix_core\phoenix_core.log y cargador.txt.'
