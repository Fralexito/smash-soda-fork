# Phoenix · Mandos 9–16 para PES 2021 (módulo de Sider) — instalador sin Phoenix Link.
# Uso (con PES CERRADO):  clic derecho › «Ejecutar con PowerShell», o:
#     powershell -ExecutionPolicy Bypass -File INSTALAR.ps1 -Sider "D:\Juegos\PES2021\SiderAddons"
# Copia los archivos del paquete y ajusta sider.ini (guarda antes una copia: sider.ini.antes-phoenix-mandos-AAAAMMDD-HHMMSS).
param([string]$Sider)
$ErrorActionPreference = 'Stop'
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Sider) { $Sider = Read-Host 'Carpeta de Sider (la que tiene sider.ini y la carpeta modules)' }
$ini = Join-Path $Sider 'sider.ini'
if (-not (Test-Path $ini)) { throw "No encuentro sider.ini en $Sider" }
if (Get-Process PES2021 -ErrorAction SilentlyContinue) { throw 'PES está abierto: ciérralo antes de instalar.' }

# 1) archivos
Copy-Item -Recurse -Force (Join-Path $aqui 'SiderAddons\*') $Sider
Write-Host 'Archivos copiados.'

# 2) sider.ini (lectura/escritura byte a byte con Latin-1 para no estropear acentos ni la codificación original)
$enc = [Text.Encoding]::GetEncoding(28591)
$copia = "$ini.antes-phoenix-mandos-" + (Get-Date -Format 'yyyyMMdd-HHmmss')
Copy-Item $ini $copia
$lineas = [Collections.Generic.List[string]]([IO.File]::ReadAllText($ini, $enc) -split "`r?`n")
function Poner($clave, $valor) {
    $i = $lineas.FindIndex([Predicate[string]]{ param($l) $l -match "^\s*;?\s*$([regex]::Escape($clave))\s*=" })
    if ($i -ge 0) { $lineas[$i] = "$clave = $valor" } else { $lineas.Add("$clave = $valor") }
}
Poner 'luajit.ext.enabled' '1'
Poner 'overlay.enabled' '1'
Poner 'overlay.image-alpha-max' '1.0'
# carpeta livecpk propia, la PRIMERA de la lista (así su MatchPrePause.json gana a los de otros parches)
[void]$lineas.RemoveAll([Predicate[string]]{ param($l) $l -match 'livecpk\\PhoenixMandos' })
$primera = $lineas.FindIndex([Predicate[string]]{ param($l) $l -match '^\s*cpk\.root\s*=' })
$cpk = 'cpk.root = ".\livecpk\PhoenixMandos"'
if ($primera -ge 0) { $lineas.Insert($primera, $cpk) } else { $lineas.Add($cpk) }
# el módulo, el ÚLTIMO de la lista de módulos (la pantalla se abre sola retrocediendo un módulo en la ventana de Sider)
[void]$lineas.RemoveAll([Predicate[string]]{ param($l) $l -match 'phoenix_mandos_fase1\.lua' })
$ultima = -1; for ($i = 0; $i -lt $lineas.Count; $i++) { if ($lineas[$i] -match '^\s*lua\.module\s*=') { $ultima = $i } }
$mod = 'lua.module = "phoenix_mandos_fase1.lua"'
if ($ultima -ge 0) { $lineas.Insert($ultima + 1, $mod) } else { $lineas.Add($mod) }
[IO.File]::WriteAllText($ini, ($lineas -join "`r`n"), $enc)
Write-Host "sider.ini ajustado (copia en $copia)."
Write-Host 'Listo. Abre PES con Sider y mira SiderAddons\content\phoenix\mandos\fase1.txt para comprobar que cargó.'
