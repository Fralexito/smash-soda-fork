# Phoenix Core · desinstalador. Quita la línea del cargador de sider.ini (con copia) y borra el módulo.
# Deja la carpeta SiderAddons\phoenix_core (registro y configuración) por si hace falta revisarla; bórrala a mano si quieres.
param([string]$Sider)
$ErrorActionPreference = 'Stop'
if (-not $Sider) { $Sider = Read-Host 'Carpeta de Sider (la que tiene sider.ini)' }
$ini = Join-Path $Sider 'sider.ini'
if (-not (Test-Path $ini)) { throw "No encuentro sider.ini en $Sider" }
if (Get-Process PES2021 -ErrorAction SilentlyContinue) { throw 'PES está abierto: ciérralo antes de desinstalar.' }
$enc = [Text.Encoding]::GetEncoding(28591)
$copia = "$ini.antes-quitar-phoenix-core-" + (Get-Date -Format 'yyyyMMdd-HHmmss')
Copy-Item $ini $copia
$lineas = [Collections.Generic.List[string]]([IO.File]::ReadAllText($ini, $enc) -split "`r?`n")
$n = $lineas.RemoveAll([Predicate[string]]{ param($l) $l -match 'phoenix_core_loader\.lua|phoenix_marcador_free\.lua' })
[IO.File]::WriteAllText($ini, ($lineas -join "`r`n"), $enc)
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $Sider 'modules\phoenix_core_loader.lua')
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $Sider 'modules\phoenix_marcador_free.lua')
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $Sider 'content\phoenix\marcador_free.json')
Write-Host "Quitadas $n línea(s) de sider.ini (copia en $copia). Phoenix Core desinstalado."
