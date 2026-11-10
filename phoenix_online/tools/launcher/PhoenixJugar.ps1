<#
Phoenix Jugar: botones «Jugar Online» y «Jugar Offline» del Phoenix Launcher (versión puente).
Plan: phoenix_online/docs/PLAN_LAUNCHER_PHOENIX.md

  powershell -NoProfile -ExecutionPolicy Bypass -File PhoenixJugar.ps1 -Modo Online
  powershell -NoProfile -ExecutionPolicy Bypass -File PhoenixJugar.ps1 -Modo Offline
  powershell -NoProfile -ExecutionPolicy Bypass -File PhoenixJugar.ps1 -Modo Salir    (cierra la sesión guardada)

Online: inicia sesión con la cuenta de la web (solo la primera vez; queda guardada cifrada para este usuario
de Windows), avisa al servidor Phoenix «este PES es de esta cuenta», redirige el online al servidor y abre el PES.
Offline: quita la redirección (solo las líneas #PHOENIX) y abre el PES. Nada del online.
Cambiar el archivo hosts pide permiso de administrador (Windows lo pregunta solo).
La configuración va en PhoenixJugar.json, junto a este archivo (sin contraseñas ni claves secretas: la clave
pública de la web es la misma que usa la página).
#>
param([ValidateSet("Online", "Offline", "Salir")][string]$Modo = "Online", [switch]$SoloHosts, [string]$IpHosts)

$ErrorActionPreference = "Stop"
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
$cfg = Get-Content (Join-Path $aqui "PhoenixJugar.json") -Raw -Encoding UTF8 | ConvertFrom-Json
$carpeta = Join-Path $env:APPDATA "PhoenixLauncher"
$sesion = Join-Path $carpeta "sesion.dat"
$hosts = Join-Path $env:SystemRoot "System32\drivers\etc\hosts"
$dominios = @("pes21-x64-gate.cs.konami.net", "pes21-x64-stun.cs.konami.net", "pesam.stun.service.konami.net",
              "info.service.konami.net", "cs.konami.net", "ntl.service.konami.net", "ntleu.service.konami.net",
              "ntljp.service.konami.net")

function Esta-Elevado {
    $p = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Poner-Hosts([string]$ip) {
    # $ip vacío = quitar. Solo toca las líneas con la marca #PHOENIX; copia de seguridad la primera vez.
    if (-not (Esta-Elevado)) {
        $argumentos = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Modo $Modo -SoloHosts -IpHosts `"$ip`""
        $p = Start-Process powershell -ArgumentList $argumentos -Verb RunAs -Wait -PassThru
        if ($p.ExitCode -ne 0) { throw "No se pudo cambiar el archivo hosts (¿se canceló el permiso?)." }
        return
    }
    $copia = "$hosts.antes_phoenix"
    if (-not (Test-Path $copia)) { Copy-Item $hosts $copia }
    $lineas = @(Get-Content $hosts -Encoding ASCII | Where-Object { $_ -notmatch "#PHOENIX" })
    if ($ip) { $lineas += $dominios | ForEach-Object { "$ip $_ #PHOENIX" } }
    Set-Content -Path $hosts -Value $lineas -Encoding ASCII
    ipconfig /flushdns | Out-Null
}

if ($SoloHosts) {   # segunda vuelta, ya como administrador
    try { Poner-Hosts $IpHosts; exit 0 } catch { Write-Host $_; exit 1 }
}

function Guardar-Sesion($refresh) {
    New-Item -ItemType Directory -Force $carpeta | Out-Null
    # DPAPI: solo este usuario de Windows en esta PC puede leerlo
    ConvertTo-SecureString $refresh -AsPlainText -Force | ConvertFrom-SecureString | Set-Content $sesion
}

function Leer-Sesion {
    if (-not (Test-Path $sesion)) { return $null }
    try {
        $s = Get-Content $sesion | ConvertTo-SecureString
        return [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($s))
    } catch { return $null }
}

function Pedir-Token {
    $cab = @{ apikey = $cfg.clave_publica_web }
    $refresh = Leer-Sesion
    if ($refresh) {
        try {
            $r = Invoke-RestMethod -Method Post -Uri "$($cfg.url_web_api)/auth/v1/token?grant_type=refresh_token" `
                -Headers $cab -ContentType "application/json" -Body (@{ refresh_token = $refresh } | ConvertTo-Json)
            Guardar-Sesion $r.refresh_token
            return $r.access_token
        } catch { Write-Host "Tu sesión guardada caducó; inicia sesión otra vez." }
    }
    Write-Host "Inicia sesión con tu cuenta de la web Phoenix (solo esta vez)."
    $correo = Read-Host "Correo"
    $clave = Read-Host "Contraseña" -AsSecureString
    $plano = [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($clave))
    try {
        $r = Invoke-RestMethod -Method Post -Uri "$($cfg.url_web_api)/auth/v1/token?grant_type=password" `
            -Headers $cab -ContentType "application/json" -Body (@{ email = $correo; password = $plano } | ConvertTo-Json)
    } catch { throw "Correo o contraseña incorrectos. (Si entras a la web con Discord/Google, pon una contraseña en tu perfil o espera a Phoenix Link.)" }
    finally { $plano = $null }
    Guardar-Sesion $r.refresh_token
    return $r.access_token
}

function Abrir-PES {
    $exe = $cfg.ruta_pes
    if (-not (Test-Path $exe)) { throw "No encuentro el PES en $exe (revisa ruta_pes en PhoenixJugar.json)." }
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe)
}

try {
    switch ($Modo) {
        "Salir" {
            Remove-Item $sesion -ErrorAction SilentlyContinue
            Write-Host "Sesión cerrada en esta PC."
        }
        "Offline" {
            Write-Host "Modo Offline: quito la conexión al servidor Phoenix y abro el PES."
            Poner-Hosts ""
            Abrir-PES
        }
        "Online" {
            $ip = $cfg.ip_servidor
            try { $estado = Invoke-RestMethod -Uri "http://${ip}:5000/estado" -TimeoutSec 5 }
            catch { throw "El servidor Phoenix ($ip) no responde. ¿Está encendido? ¿Tienes Tailscale abierto?" }
            if ($estado.mantenimiento) { throw "El servidor está en mantenimiento. Prueba en un rato (o juega Offline)." }
            $token = Pedir-Token
            try {
                $r = Invoke-RestMethod -Method Post -Uri "http://${ip}:5000/entrar" -ContentType "application/json" `
                    -Body (@{ token = $token } | ConvertTo-Json) -TimeoutSec 20
            } catch {
                $m = $_.ErrorDetails.Message
                if ($m) { throw (($m | ConvertFrom-Json).mensaje) } else { throw "No se pudo hablar con el servidor: $($_.Exception.Message)" }
            }
            Write-Host $r.mensaje
            Poner-Hosts $ip
            Write-Host "En línea: $($estado.jugadores_en_linea) jugador(es). Abriendo el PES..."
            Abrir-PES
        }
    }
} catch {
    Write-Host ""
    Write-Host "No se pudo: $_" -ForegroundColor Yellow
    Read-Host "Pulsa Enter para cerrar"
    exit 1
}
