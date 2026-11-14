# REVISAR_ANTES_DE_LA_PRUEBA.ps1 - SOLO LEE. No cambia el firewall, ni la red, ni nada.
# Lo ejecuta Frank en SU PC antes de la prueba con el amigo. Cada punto termina en OK o FALTA.
# Si algo FALTA en el firewall, SOLO avisa: la decision y el cambio los hace Frank.
$ErrorActionPreference = "SilentlyContinue"
$res = @()
function Punto($nombre, $ok, $detalle) {
    $estado = if ($ok) { "OK   " } else { "FALTA" }
    $color = if ($ok) { "Green" } else { "Yellow" }
    Write-Host ("[{0}] {1}" -f $estado, $nombre) -ForegroundColor $color
    if ($detalle) { Write-Host ("        {0}" -f $detalle) }
    $script:res += [pscustomobject]@{ Punto = $nombre; Ok = $ok }
}

Write-Host ""
Write-Host "=== REVISAR ANTES DE LA PRUEBA (solo lectura) ===" -ForegroundColor Cyan

# (a) IP de Tailscale
$ts = Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like "100.*" -and [int]($_.IPAddress.Split('.')[1]) -ge 64 -and [int]($_.IPAddress.Split('.')[1]) -le 127 }
if ($ts) {
    Punto "Tailscale con sesion iniciada" $true ("Tu IP de Tailscale: " + (($ts | Select-Object -First 1).IPAddress) + "  (esta es la que le das al amigo)")
} else {
    Punto "Tailscale con sesion iniciada" $false "No hay IP 100.x. Abre Tailscale e inicia sesion; luego ejecuta esto otra vez."
}

# (b) Perfil de red de la interfaz Tailscale
$ifTs = Get-NetAdapter | Where-Object { $_.Name -like "*Tailscale*" -or $_.InterfaceDescription -like "*Tailscale*" }
if ($ifTs) {
    $perfil = Get-NetConnectionProfile -InterfaceIndex $ifTs[0].ifIndex
    if ($perfil) {
        $cat = [string]$perfil.NetworkCategory
        Punto "Perfil de red de Tailscale" $true ("Es: " + $cat + ". (Si es Publico, el firewall suele bloquear mas; no lo cambio yo.)")
    } else {
        Punto "Perfil de red de Tailscale" $false "La interfaz existe pero no tiene perfil de red (probablemente sin sesion)."
    }
} else {
    Punto "Perfil de red de Tailscale" $false "No hay interfaz Tailscale en esta PC."
}

# (c) Reglas de entrada del firewall para python.exe y PES2021.exe (solo lectura)
foreach ($prog in @("python.exe", "PES2021.exe")) {
    $reglas = @()
    $filtros = Get-NetFirewallApplicationFilter | Where-Object { $_.Program -and ($_.Program -like "*\$prog") }
    foreach ($f in $filtros) {
        $r = $f | Get-NetFirewallRule
        if ($r -and $r.Direction -eq "Inbound" -and $r.Enabled -eq "True") { $reglas += $r }
    }
    if ($reglas.Count -eq 0) {
        Punto ("Firewall: regla de entrada para " + $prog) $false "No hay ninguna regla de entrada activa. Windows puede preguntar al primer uso; si no, hay que permitirlo (lo decide Frank)."
    } else {
        $perm = $reglas | Where-Object { $_.Action -eq "Allow" }
        $bloq = $reglas | Where-Object { $_.Action -eq "Block" }
        $det = ($reglas | ForEach-Object { "{0} -> {1}, perfiles: {2}" -f $_.DisplayName, $_.Action, $_.Profile }) -join " | "
        if ($bloq) { Punto ("Firewall: " + $prog) $false ("HAY una regla que BLOQUEA: " + $det) }
        elseif ($perm) { Punto ("Firewall: " + $prog) $true $det }
    }
}

# (d) Puertos que escucha el servidor y si es en 0.0.0.0
$tcp = @(10000, 10001, 5000, 80, 5349)
$udp = @(3478, 5730, 5100, 5350)
$pidsPython = (Get-Process python -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id)
foreach ($p in $tcp) {
    $c = Get-NetTCPConnection -State Listen -LocalPort $p | Where-Object { $pidsPython -contains $_.OwningProcess }
    if ($c) {
        $todas = @($c | Where-Object { $_.LocalAddress -in @("0.0.0.0", "::") }).Count -gt 0
        Punto ("Servidor escucha TCP " + $p) $todas ("Direccion: " + (($c | Select-Object -ExpandProperty LocalAddress -Unique) -join ", ") + $(if (-not $todas) { "  (solo local: el amigo no podria entrar)" }))
    } else { Punto ("Servidor escucha TCP " + $p) $false "No escucha (servidor apagado o puerto distinto)." }
}
foreach ($p in $udp) {
    $c = Get-NetUDPEndpoint -LocalPort $p | Where-Object { $pidsPython -contains $_.OwningProcess }
    if ($c) {
        $todas = @($c | Where-Object { $_.LocalAddress -in @("0.0.0.0", "::") }).Count -gt 0
        Punto ("Servidor escucha UDP " + $p) $todas ("Direccion: " + (($c | Select-Object -ExpandProperty LocalAddress -Unique) -join ", "))
    } else { Punto ("Servidor escucha UDP " + $p) $false "No escucha." }
}

# (e) Lineas #PHOENIX del hosts
$hosts = Join-Path $env:SystemRoot "System32\drivers\etc\hosts"
$ph = Get-Content $hosts | Where-Object { $_ -match "#PHOENIX" }
if ($ph) {
    Punto "Archivo hosts con lineas #PHOENIX" $true ("" + $ph.Count + " linea(s). Ejemplo: " + ($ph | Select-Object -First 1))
} else {
    Punto "Archivo hosts con lineas #PHOENIX" $false "No hay. En la PC de Frank puede ser normal (usa el servidor en la misma PC); el AMIGO si las necesita (CONECTAR_PHOENIX.bat)."
}

Write-Host ""
$faltan = ($res | Where-Object { -not $_.Ok }).Count
Write-Host ("RESUMEN: {0} OK, {1} FALTA" -f (($res | Where-Object { $_.Ok }).Count), $faltan) -ForegroundColor Cyan
Write-Host "Este programa NO cambio nada. Si algo FALTA en el firewall, la decision de permitirlo es de Frank." -ForegroundColor Green
