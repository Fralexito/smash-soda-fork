# Empaqueta la release de Phoenix Link (el zip que baja PhoenixLink-instalar-rapido.bat) desde una carpeta ya COMPILADA.
# Uso:  powershell -ExecutionPolicy Bypass -File tools\EMPAQUETAR-RELEASE.ps1 -Release <carpeta x64\Release> -Salida <carpeta> [-Nombre PhoenixLink-0.2.67-beta2.zip]
# Hace: copia la carpeta sin .pdb, imgui.ini ni overlay\ (PhoenixGlass va aparte), crea el zip y escribe su SHA256.
# Despues hay que poner ese SHA256 y la URL del release en PhoenixLink-instalar-rapido.bat (PHX_ZIP_SHA / PHX_ZIP_URL).
# Compila SIEMPRE desde una copia limpia del commit (git worktree) y no en el arbol compartido.
param(
    [Parameter(Mandatory = $true)][string]$Release,
    [Parameter(Mandatory = $true)][string]$Salida,
    [string]$Nombre = '',
    [string]$Raiz = ''
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath (Join-Path $Release 'PhoenixLink.exe'))) { throw "No hay PhoenixLink.exe en ${Release}" }
foreach ($f in 'parsec.dll', 'SDL2.dll', 'WebView2Loader.dll', 'ui\index.html', 'sider\phoenix.lua', 'sider\phoenix_referee.lua', 'fichajes') {
    if (-not (Test-Path -LiteralPath (Join-Path $Release $f))) { throw "Falta ${f} en ${Release} (¿compilación incompleta?)" }
}
if (-not $Nombre) {
    # -Raiz = la carpeta (worktree) de donde se compilo; si no se da, la de este script. Asi el nombre sale de la version COMPILADA.
    if (-not $Raiz) { $Raiz = Join-Path $PSScriptRoot '..' }
    $h = Join-Path $Raiz 'SmashSoda\phoenix\PhoenixBuild.h'
    $v = (Select-String -Path $h -Pattern 'kVersion = "([^"]+)"').Matches[0].Groups[1].Value
    $e = (Select-String -Path $h -Pattern 'kEtapa = "([^"]+)"').Matches[0].Groups[1].Value -replace '\s', ''
    $Nombre = "PhoenixLink-$v-$e.zip"
}
New-Item -ItemType Directory -Force -Path $Salida | Out-Null
$stage = Join-Path $Salida ('stage-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $stage | Out-Null
robocopy $Release $stage /E /XF *.pdb imgui.ini NotoSansSC*.ttf /XD overlay /NFL /NDL /NJH /NJS | Out-Null
$zip = Join-Path $Salida $Nombre
if (Test-Path -LiteralPath $zip) { throw "Ya existe ${zip}: elige otro nombre o borralo a mano." }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$sha = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
Set-Content -LiteralPath "$zip.sha256.txt" -Value "$sha  $Nombre" -Encoding ascii
Write-Host ("Zip:    " + $zip + "  (" + [math]::Round((Get-Item $zip).Length / 1MB, 1) + " MB)")
Write-Host "SHA256: $sha"
Write-Host "Carpeta temporal (puedes borrarla): $stage"
