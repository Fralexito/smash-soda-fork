# Empaqueta la release de Phoenix Link y (con -Subir) la sube a la web como versión NO publicada.
#
# UN SOLO COMANDO (lo normal):
#   powershell -ExecutionPolicy Bypass -File tools\EMPAQUETAR-RELEASE.ps1 -Subir -Notas "Qué trae esta versión, en español simple"
#   (+ -Obligatoria si nadie debe quedarse con la anterior)
#   Hace: compila en una copia LIMPIA del commit actual (git worktree, sin PhoenixBetaSello.h), comprueba que el exe no lleva
#   el sello del dueño, empaqueta sin .pdb, imgui.ini, overlay\ ni datos personales, calcula el sha256 y lo sube a la web
#   con el token de ESTA PC (el de Phoenix Link, cifrado con DPAPI; la cuenta tiene que ser admin). La web comprueba el
#   sha256 y la deja SIN publicar: Fralex la publica en https://fralexito.github.io/link-versiones/ y cada Link la ve en < 1 h.
#
# Solo el zip (como antes, para PhoenixLink-instalar-rapido.bat):
#   powershell -ExecutionPolicy Bypass -File tools\EMPAQUETAR-RELEASE.ps1 -Release <carpeta x64\Release> -Salida <carpeta> [-Nombre PhoenixLink-0.2.67-beta2.zip]
#   Después hay que poner ese SHA256 y la URL del release en PhoenixLink-instalar-rapido.bat (PHX_ZIP_SHA / PHX_ZIP_URL).
param(
    [string]$Release = '',
    [string]$Salida = '',
    [string]$Nombre = '',
    [string]$Raiz = '',
    [switch]$Subir,
    [string]$Notas = '',
    [switch]$Obligatoria,
    [string]$Base = 'https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/mercado/v1',
    [string]$Token = ''   # solo pruebas; si no, se lee el de Phoenix Link de esta PC
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $Salida) { $Salida = Join-Path $env:TEMP 'phoenix-release' }

# --- 1) Compilar en limpio si no se dio -Release -------------------------------------------------
$copiaLimpia = ''
if (-not $Release) {
    if (-not $Subir) { throw 'Falta -Release <carpeta x64\Release> (o usa -Subir para compilar y subir de una vez).' }
    $copiaLimpia = Join-Path $env:TEMP ('phx-release-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
    Write-Host "Compilando en una copia limpia del commit actual: $copiaLimpia"
    git -C $repo worktree add --detach $copiaLimpia HEAD | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear la copia limpia (git worktree).' }
    $gen = 'Visual Studio 18 2026'
    $cache = Join-Path $repo 'build\CMakeCache.txt'
    if (Test-Path $cache) { $m = Select-String -Path $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=(.+)$'; if ($m) { $gen = $m.Matches[0].Groups[1].Value } }
    cmake -S $copiaLimpia -B (Join-Path $copiaLimpia 'build') -G $gen | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'cmake (configurar) falló.' }
    cmake --build (Join-Path $copiaLimpia 'build') --config Release | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'La compilación falló: no se sube nada.' }
    $Release = Join-Path $copiaLimpia 'x64\Release'
    $Raiz = $copiaLimpia
}

if (-not (Test-Path -LiteralPath (Join-Path $Release 'PhoenixLink.exe'))) { throw "No hay PhoenixLink.exe en ${Release}" }
$necesarios = 'parsec.dll', 'SDL2.dll', 'WebView2Loader.dll', 'ui\index.html', 'sider\phoenix.lua', 'sider\phoenix_referee.lua', 'fichajes'
if ($Subir) { $necesarios += 'PhoenixActualizador.exe' }   # sin él, la PC no podría instalar la siguiente
foreach ($f in $necesarios) {
    if (-not (Test-Path -LiteralPath (Join-Path $Release $f))) { throw "Falta ${f} en ${Release} (¿compilación incompleta?)" }
}
if (-not $Raiz) { $Raiz = Join-Path $PSScriptRoot '..' }
$h = Join-Path $Raiz 'SmashSoda\phoenix\PhoenixBuild.h'
$version = (Select-String -Path $h -Pattern 'kVersion = "([^"]+)"').Matches[0].Groups[1].Value
$etapa = (Select-String -Path $h -Pattern 'kEtapa = "([^"]+)"').Matches[0].Groups[1].Value
if (-not $Nombre) { $Nombre = "PhoenixLink-$version-$($etapa -replace '\s', '').zip" }

# --- 2) Nunca repartir el sello del dueño (exime de la caducidad de la beta) -----------------------
if (Test-Path -LiteralPath (Join-Path $Raiz 'SmashSoda\phoenix\PhoenixBetaSello.h')) {
    if ($Subir) { throw "Esta carpeta tiene PhoenixBetaSello.h: su exe lleva el sello del dueño. Usa -Subir sin -Release (compila en limpio)." }
    Write-Warning 'Esta carpeta tiene PhoenixBetaSello.h: el exe lleva el sello del dueño. No repartas este zip.'
}
$sellos = @()
foreach ($c in @((Join-Path $repo 'SmashSoda\phoenix\PhoenixBetaSello.h'), (Join-Path $repo '_phoenix-link\SmashSoda\phoenix\PhoenixBetaSello.h'), 'C:\dev\smash-soda-fork\_phoenix-link\SmashSoda\phoenix\PhoenixBetaSello.h')) {
    if (Test-Path -LiteralPath $c) { $m = Select-String -Path $c -Pattern 'PHOENIX_SELLO_DUENO\s+"([^"]+)"'; if ($m) { $sellos += $m.Matches[0].Groups[1].Value } }
}
if ($sellos.Count) {
    $bytes = [System.IO.File]::ReadAllBytes((Join-Path $Release 'PhoenixLink.exe'))
    $texto = [System.Text.Encoding]::ASCII.GetString($bytes)
    foreach ($s in $sellos) { if ($texto.Contains($s)) { throw 'PhoenixLink.exe lleva el sello secreto del dueño: NO se empaqueta. Compila en limpio (-Subir sin -Release).' } }
    Write-Host 'Sello del dueño: no está en el exe (bien).'
}

# --- 3) Empaquetar (sin .pdb, imgui.ini, overlay\ ni datos personales) ----------------------------
New-Item -ItemType Directory -Force -Path $Salida | Out-Null
$stage = Join-Path $Salida ('stage-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $stage | Out-Null
robocopy $Release $stage /E /XF *.pdb *.ilk *.exp *.lib *.log *.dmp imgui.ini portable.txt phoenix-token.dat NotoSansSC*.ttf /XD overlay Trybuchet _actualizacion /NFL /NDL /NJH /NJS | Out-Null
$zip = Join-Path $Salida $Nombre
if (Test-Path -LiteralPath $zip) {
    if ($Subir) { Remove-Item -LiteralPath $zip } else { throw "Ya existe ${zip}: elige otro nombre o borralo a mano." }
}
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$sha = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLower()
$tam = (Get-Item -LiteralPath $zip).Length
Set-Content -LiteralPath "$zip.sha256.txt" -Value "$($sha.ToUpper())  $Nombre" -Encoding ascii
Remove-Item -Recurse -Force -LiteralPath $stage
Write-Host ("Zip:     " + $zip + "  (" + [math]::Round($tam / 1MB, 1) + " MB)")
Write-Host "Versión: $version ($etapa)"
Write-Host "SHA256:  $sha"

# --- 4) Subir a la web como versión NO publicada ---------------------------------------------------
if ($Subir) {
    if ($tam -gt 52428800) { throw 'El zip pasa de 50 MB (límite de Supabase Storage).' }
    if (-not $Token) {
        $dat = Join-Path $env:APPDATA 'Trybuchet\Smash Soda\phoenix-token.dat'
        if (-not (Test-Path -LiteralPath $dat)) { throw "Esta PC no tiene Phoenix Link vinculado ($dat). Vincúlala con tu cuenta admin y vuelve a probar." }
        Add-Type -AssemblyName System.Security
        $plano = [System.Security.Cryptography.ProtectedData]::Unprotect([System.IO.File]::ReadAllBytes($dat), $null, 'CurrentUser')
        $Token = [System.Text.Encoding]::UTF8.GetString($plano)
    }
    $cab = @{ Authorization = "Bearer $Token" }
    $cuerpo = @{ version = $version; etapa = $etapa; sha256 = $sha; tamano = $tam; notas = $Notas; obligatoria = [bool]$Obligatoria } | ConvertTo-Json -Compress
    try {
        $r = Invoke-RestMethod -Method Post -Uri "$Base/link/versiones" -Headers $cab -ContentType 'application/json; charset=utf-8' -Body ([System.Text.Encoding]::UTF8.GetBytes($cuerpo))
    } catch {
        $msg = $_.ErrorDetails.Message
        if ($msg -match 'NO_AUTORIZADO') { throw 'La cuenta vinculada en esta PC no es admin: no puede subir versiones.' }
        if ($msg -match 'VERSION_YA_PUBLICADA') { throw "La versión $version ya está publicada. Sube kVersion en PhoenixBuild.h y vuelve a compilar." }
        throw "La web no aceptó la versión: $msg"
    }
    if (-not $r.ok) { throw "La web respondió un error: $($r | ConvertTo-Json -Compress)" }
    Write-Host "Registrada la $version (id $($r.datos.id)). Subiendo el zip..."
    Invoke-WebRequest -Method Put -Uri $r.datos.subir_url -InFile $zip -ContentType 'application/zip' -UseBasicParsing | Out-Null
    $c = Invoke-RestMethod -Method Post -Uri "$Base/link/versiones/subida" -Headers $cab -ContentType 'application/json' -Body (@{ id = $r.datos.id } | ConvertTo-Json -Compress)
    if (-not $c.ok) { throw "La web no confirmó el zip: $($c | ConvertTo-Json -Compress)" }
    Write-Host ''
    Write-Host "LISTO: la $version está subida y comprobada, SIN publicar." -ForegroundColor Green
    Write-Host 'Publícala en https://fralexito.github.io/link-versiones/ (botón Publicar). Cada Link la verá en menos de 1 hora.'
}
if ($copiaLimpia) {
    git -C $repo worktree remove --force $copiaLimpia | Out-Null
}
