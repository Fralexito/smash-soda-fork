# paquete_diagnostico.ps1 - SOLO LEE Y COPIA. No cambia nada del servidor ni del PES.
# Reune en una carpeta con fecha lo necesario para diagnosticar la prueba, con los datos sensibles TACHADOS.
# NUNCA subas la carpeta resultante a GitHub (esta en .gitignore como diagnostico_*).
# Uso (en la carpeta del servidor):  powershell -ExecutionPolicy Bypass -File tools\paquete_diagnostico.ps1
#       opciones: -Horas 6 (cuanto log incluir, por defecto 6)  -Pes "C:\PES21\eFootball PES 2021"
param([int]$Horas = 6, [string]$Pes = "C:\PES21\eFootball PES 2021")
$ErrorActionPreference = "SilentlyContinue"
$raiz = Split-Path -Parent $PSScriptRoot
$sello = Get-Date -Format "yyyyMMdd-HHmmss"
$dest = Join-Path $raiz ("diagnostico_" + $sello)
New-Item -ItemType Directory -Path $dest | Out-Null

# Tacha: SteamID, ids hex de 15-16 digitos, valores de hash/token/secreto/clave y cadenas largas tipo base64
function Tachar([string]$t) {
    $t = [regex]::Replace($t, '(?i)("?(hash|token|secreto|secret|password|clave|pass|ticket|pase|username)"?\s*[:=]\s*"?)[^",\s}]+', '$1<TACHADO>')
    $t = [regex]::Replace($t, '\b7656119\d{10}\b', '<STEAMID>')
    $t = [regex]::Replace($t, '\b\d{16,20}\b', '<STEAMID>')
    $t = [regex]::Replace($t, '\b[0-9a-fA-F]{15,16}\b', '<ID>')
    $t = [regex]::Replace($t, '[A-Za-z0-9+/_-]{28,}={0,2}', '<TACHADO>')
    return $t
}
function Copiar-Tachado($origen, $nombre, $horas) {
    if (-not (Test-Path $origen)) { "no existe: $origen" | Add-Content (Join-Path $dest "LEEME.txt"); return }
    $corte = if ($horas) { (Get-Date).AddHours(-$horas) } else { $null }
    Get-Content $origen | Where-Object {
        if (-not $corte) { return $true }
        if ($_ -match '^(\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2})') { try { [datetime]$Matches[1] -ge $corte } catch { $true } } else { $true }
    } | ForEach-Object { Tachar $_ } | Set-Content -Encoding UTF8 (Join-Path $dest $nombre)
}

"Paquete de diagnostico creado el $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Set-Content (Join-Path $dest "LEEME.txt")
"Incluye (con SteamID, hashes y tokens TACHADOS): log del servidor, peticiones/respuestas de cada jugador (capturas de las ultimas horas, tope 60 MB), sider.log del PES, resumen del estado." | Add-Content (Join-Path $dest "LEEME.txt")
"NO incluye: config.json, claves, base de datos, capturas completas. NO LO SUBAS A GITHUB." | Add-Content (Join-Path $dest "LEEME.txt")

$err = Get-ChildItem (Join-Path $raiz "arranque_*.err") | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($err) { Copiar-Tachado $err.FullName "servidor_stderr.log" $Horas }
Copiar-Tachado (Join-Path $raiz "registro\servidor.log") "servidor.log" $Horas
Copiar-Tachado (Join-Path $raiz "registro\rival_udp.txt") "rival_udp.txt" $null
# Peticiones y respuestas de cada jugador (capturas/*.jsonl y http.jsonl) de las ultimas horas,
# TACHADAS. Tope 60 MB para no llenar el disco; lo mas reciente primero.
$capt = Join-Path $dest "capturas"
New-Item -ItemType Directory -Path $capt | Out-Null
$usado = 0
Get-ChildItem (Join-Path $raiz "capturas") -File -Filter *.jsonl | Where-Object { $_.LastWriteTime -ge (Get-Date).AddHours(-$Horas) } |
    Sort-Object LastWriteTime -Descending | ForEach-Object {
        if ($usado + $_.Length -le 60MB) {
            Copiar-Tachado $_.FullName ("capturas\" + $_.Name) $null
            $usado += $_.Length
        } else { "capturas: no cabe $($_.Name) ($([int]($_.Length/1KB)) KB)" | Add-Content (Join-Path $dest "LEEME.txt") }
    }
foreach ($n in @("Sider\sider.log", "Sider\sider-app.log")) {
    $p = Join-Path $Pes $n
    if (Test-Path $p) { Copiar-Tachado $p ($n -replace '\\', '_') $null }
}

$r = @()
$r += "Fecha: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$r += "Windows: $((Get-CimInstance Win32_OperatingSystem).Caption)"
$r += "Servidor (python): " + ((Get-CimInstance Win32_Process -Filter "Name='python.exe'" | Where-Object { $_.CommandLine -match 'phoenix_online' } | ForEach-Object { "PID $($_.ProcessId)" }) -join ", ")
$r += "PES2021.exe: " + ((Get-Process PES2021 | ForEach-Object { "PID $($_.Id) responde=$($_.Responding) memoria=$([int]($_.WorkingSet64/1MB))MB" }) -join ", ")
$exe = Join-Path $Pes "PES2021.exe"
if (Test-Path $exe) { $i = Get-Item $exe; $r += "PES2021.exe tamano=$($i.Length) fecha=$($i.LastWriteTime.ToString('yyyy-MM-dd HH:mm'))" }
$r += "Capturas en disco: $(@(Get-ChildItem (Join-Path $raiz 'capturas') -File).Count) archivos (copiadas tachadas las de las ultimas $Horas h)"
$r += "Cierres de aplicacion recientes de PES2021:"
Get-WinEvent -FilterHashtable @{ LogName = 'Application'; ProviderName = 'Application Error'; StartTime = (Get-Date).AddHours(-$Horas) } -MaxEvents 20 |
    Where-Object { $_.Message -match 'PES2021' } | ForEach-Object { $r += "  " + $_.TimeCreated.ToString('HH:mm:ss') + " " + (($_.Message -split "`n")[0..2] -join ' ') }
$r | Set-Content -Encoding UTF8 (Join-Path $dest "estado.txt")

Write-Host "Listo: $dest" -ForegroundColor Green
Write-Host "Revisa LEEME.txt. No lo subas a GitHub." -ForegroundColor Yellow
