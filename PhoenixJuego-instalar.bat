@echo off
setlocal
title Phoenix - instalar phoenix.lua en PES 2021
rem ============================================================================
rem  Instala phoenix.lua v0.18 (Phoenix Sync) en PES 2021 + ConmeGOL Patch 26,
rem  igual que el boton "Modulos del juego" de Phoenix Link, sin necesitar Link:
rem   - en <juego>\SiderAddons y en cada modo del parche (...\<modo>\SiderAddons)
rem   - copia de sider.ini (sider.ini.phoenix-<fecha>) antes de tocarlo
rem   - cpk.root = ".\livecpk\Phoenix-DB" justo antes de la base del parche
rem   - lua.module = "phoenix.lua" debajo del ultimo modulo activo
rem   - carpetas content\phoenix y livecpk\Phoenix-DB\common\etc\pesdb (vacias)
rem   - guarda phoenix.lua.antes-v<version> si habia otra version
rem  Usa phoenix.lua de esta misma carpeta o lo baja de GitHub; en ambos casos
rem  comprueba su sha256. Si algo no cuadra, no toca nada.
rem ============================================================================
set "PHX_BAT=%~f0"
set "PHX_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$t=[IO.File]::ReadAllText($env:PHX_BAT); iex $t.Substring($t.IndexOf('#__PS'+'__'))"
echo.
pause
exit /b

#__PS__
function Hash($p) {   # sha256 sin Get-FileHash (no siempre esta)
    $h = [Security.Cryptography.SHA256]::Create()
    try { ([BitConverter]::ToString($h.ComputeHash([IO.File]::ReadAllBytes($p)))).Replace('-', '') } finally { $h.Dispose() }
}
function Main {
    $ErrorActionPreference = 'Stop'
    $VERSION = '0.18'
    $SHA = 'A39B54DE9F6A579D539ECDC432E14BBC2B32B982C14F82D16E1F304651E4F361'
    $URL = 'https://raw.githubusercontent.com/Fralexito/smash-soda-fork/redise%C3%B1o-phoenix-portal/SmashSoda/phoenix/sider/phoenix.lua'
    $L1 = [Text.Encoding]::GetEncoding(28591)   # lee y escribe los bytes tal cual
    $sello = Get-Date -Format 'yyyyMMdd-HHmmss'

    Write-Host ''
    Write-Host '  PHOENIX - phoenix.lua v0.18 para PES 2021' -ForegroundColor Cyan
    Write-Host '  ------------------------------------------' -ForegroundColor Cyan
    Write-Host ''

    # 1) PES cerrado: Sider lee los modulos al arrancar
    if (Get-Process -Name 'PES2021' -ErrorAction SilentlyContinue) {
        Write-Host '  [X] PES 2021 esta abierto. Cierralo y vuelve a ejecutar este archivo.' -ForegroundColor Red
        return
    }

    # 2) Carpeta del juego
    $juego = $env:PHX_JUEGO   # opcional: carpeta ya elegida (sin dialogo)
    if (-not $juego) { try {
        Add-Type -AssemblyName System.Windows.Forms
        $dlg = New-Object System.Windows.Forms.FolderBrowserDialog
        $dlg.Description = 'Elige la carpeta de PES 2021 (la que tiene PES2021.exe y SiderAddons)'
        $dlg.ShowNewFolderButton = $false
        if ($dlg.ShowDialog() -eq 'OK') { $juego = $dlg.SelectedPath }
    } catch { } }
    if (-not $juego) { $juego = (Read-Host '  Pega la carpeta de PES 2021').Trim('" ') }
    if (-not $juego) { Write-Host '  [X] Sin carpeta: no se toca nada.' -ForegroundColor Red; return }
    if ((Split-Path $juego -Leaf) -ieq 'SiderAddons') { $juego = Split-Path $juego -Parent }
    if (-not (Test-Path -LiteralPath (Join-Path $juego 'SiderAddons\sider.ini'))) {
        Write-Host "  [X] En '$juego' no esta SiderAddons\sider.ini. Elige la carpeta principal del juego." -ForegroundColor Red
        return
    }

    # 3) Destinos: raiz + cada modo del parche (<juego>\<x>\<modo>\SiderAddons\sider.ini)
    $destinos = @(Join-Path $juego 'SiderAddons')
    Get-ChildItem -LiteralPath $juego -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne 'SiderAddons' } | ForEach-Object {
        Get-ChildItem -LiteralPath $_.FullName -Directory -ErrorAction SilentlyContinue | Where-Object {
            Test-Path -LiteralPath (Join-Path $_.FullName 'SiderAddons\sider.ini')
        } | Sort-Object FullName | ForEach-Object { $destinos += (Join-Path $_.FullName 'SiderAddons') }
    }
    Write-Host '  Carpetas de Sider encontradas:'
    $destinos | ForEach-Object { Write-Host "    - $_" }
    Write-Host ''

    # 4) phoenix.lua v0.18: de esta carpeta o de GitHub, siempre con sha256
    $lua = $null
    $local = Join-Path $env:PHX_DIR 'phoenix.lua'
    if ((Test-Path -LiteralPath $local) -and ((Hash $local) -eq $SHA)) {
        $lua = $local
        Write-Host '  phoenix.lua v0.18: el de esta carpeta.'
    } else {
        Write-Host '  Bajando phoenix.lua v0.18 de GitHub...'
        $tmp = Join-Path $env:TEMP "phoenix-$sello.lua"
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $URL -OutFile $tmp -UseBasicParsing
        } catch {
            Write-Host '  [X] No se pudo bajar. Pon phoenix.lua (v0.18) junto a este .bat y repite.' -ForegroundColor Red
            return
        }
        if ((Hash $tmp) -ne $SHA) {
            Remove-Item -LiteralPath $tmp -Force
            Write-Host '  [X] El phoenix.lua bajado no es la v0.18 esperada. No se toca nada; avisa a Fralex.' -ForegroundColor Red
            return
        }
        $lua = $tmp
    }

    # 5) Comprobar TODOS los sider.ini antes de tocar ninguno
    $rxRaiz = '^\s*((?:[;#]\s*)*)cpk\.root\s*=\s*"([^"]*)"'
    $rxMod = '^\s*((?:[;#]\s*)*)lua\.module\s*=\s*"([^"]*)"'
    $planes = @()
    foreach ($d in $destinos) {
        $ruta = Join-Path $d 'sider.ini'
        $texto = [IO.File]::ReadAllText($ruta, $L1)
        $nl = if ($texto.Contains("`r`n")) { "`r`n" } else { "`n" }
        $finalNl = $texto.EndsWith("`n")
        $v = New-Object System.Collections.Generic.List[string]
        $plano = $texto -replace "`r`n", "`n"
        if ($finalNl) { $plano = $plano.Substring(0, $plano.Length - 1) }   # solo el ultimo salto: las lineas en blanco se quedan
        $plano.Split("`n") | ForEach-Object { $v.Add($_) }
        $cambio = $false

        # 5a) cpk.root = ".\livecpk\Phoenix-DB" antes de la primera raiz con la base del parche
        $activa = $false; $comentadaPhx = -1; $primera = -1; $primeraBase = -1
        for ($i = 0; $i -lt $v.Count; $i++) {
            $m = [regex]::Match($v[$i], $rxRaiz)
            if (-not $m.Success) { continue }
            $com = $m.Groups[1].Value.Length -gt 0
            $r = $m.Groups[2].Value
            if ($r.TrimEnd('\', '/') -match '(?i)phoenix-db$') {
                if (-not $com) { $activa = $true } elseif ($comentadaPhx -lt 0) { $comentadaPhx = $i }
                continue
            }
            if ($com) { continue }
            if ($primera -lt 0) { $primera = $i }
            if ($primeraBase -lt 0 -and (Test-Path -LiteralPath (Join-Path (Join-Path $d $r) 'common\etc\pesdb\Player.bin'))) { $primeraBase = $i }
        }
        if (-not $activa) {
            $donde = if ($primeraBase -ge 0) { $primeraBase } else { $primera }
            if ($donde -lt 0) { Write-Host "  [X] $ruta no tiene lineas cpk.root. No se toca nada." -ForegroundColor Red; return }
            if ($comentadaPhx -ge 0) { $v.RemoveAt($comentadaPhx); if ($comentadaPhx -lt $donde) { $donde-- } }
            $v.Insert($donde, 'cpk.root = ".\livecpk\Phoenix-DB"')
            $cambio = $true
        }

        # 5b) lua.module = "phoenix.lua" (descomenta la suya o va debajo del ultimo modulo activo)
        $ya = $false; $comentadaMod = -1; $ultimaActiva = -1; $ultima = -1
        for ($i = 0; $i -lt $v.Count; $i++) {
            $m = [regex]::Match($v[$i], $rxMod)
            if (-not $m.Success) { continue }
            $com = $m.Groups[1].Value.Length -gt 0
            if ($m.Groups[2].Value -ieq 'phoenix.lua') {
                if (-not $com) { $ya = $true } elseif ($comentadaMod -lt 0) { $comentadaMod = $i }
            }
            $ultima = $i
            if (-not $com) { $ultimaActiva = $i }
        }
        if (-not $ya) {
            if ($comentadaMod -ge 0) { $v[$comentadaMod] = 'lua.module = "phoenix.lua"' }
            else {
                $tras = if ($ultimaActiva -ge 0) { $ultimaActiva } else { $ultima }
                if ($tras -lt 0) { Write-Host "  [X] $ruta no tiene lista de modulos (lua.module). No se toca nada." -ForegroundColor Red; return }
                $v.Insert($tras + 1, 'lua.module = "phoenix.lua"')
            }
            $cambio = $true
        }
        $nuevo = [string]::Join($nl, $v) + $(if ($finalNl) { $nl } else { '' })
        $planes += [pscustomobject]@{ Dir = $d; Ini = $ruta; Nuevo = $nuevo; Cambio = $cambio }
    }

    # 6) Aplicar
    $hechos = 0
    foreach ($p in $planes) {
        Write-Host "  > $($p.Dir)"
        if ($p.Cambio) {
            Copy-Item -LiteralPath $p.Ini -Destination "$($p.Ini).phoenix-$sello" -Force
            $tmpIni = "$($p.Ini).tmp-$sello"
            [IO.File]::WriteAllText($tmpIni, $p.Nuevo, $L1)
            Move-Item -LiteralPath $tmpIni -Destination $p.Ini -Force
            Write-Host "      sider.ini actualizado (copia: sider.ini.phoenix-$sello)"
        } else {
            Write-Host '      sider.ini ya tenia las lineas de Phoenix'
        }
        foreach ($c in @('content\phoenix', 'livecpk\Phoenix-DB\common\etc\pesdb', 'modules')) {
            New-Item -ItemType Directory -Force -Path (Join-Path $p.Dir $c) | Out-Null
        }
        $dest = Join-Path $p.Dir 'modules\phoenix.lua'
        if (Test-Path -LiteralPath $dest) {
            if ((Hash $dest) -eq $SHA) { Write-Host '      phoenix.lua v0.18 ya estaba'; $hechos++; continue }
            $vieja = [regex]::Match([IO.File]::ReadAllText($dest, $L1), 'version\s*=\s*"([^"]+)"').Groups[1].Value
            $copia = if ($vieja) { "phoenix.lua.antes-v$vieja" } else { "phoenix.lua.antes-$sello" }
            if (Test-Path -LiteralPath (Join-Path $p.Dir "modules\$copia")) { $copia = "$copia-$sello" }
            Copy-Item -LiteralPath $dest -Destination (Join-Path $p.Dir "modules\$copia") -Force
            Write-Host "      version anterior guardada como modules\$copia"
        }
        Copy-Item -LiteralPath $lua -Destination $dest -Force
        Write-Host '      phoenix.lua v0.18 copiado'
        $hechos++
    }

    Write-Host ''
    Write-Host "  LISTO: phoenix.lua v$VERSION en $hechos carpeta(s)." -ForegroundColor Green
    Write-Host '  Abre PES 2021. En SiderAddons\sider.log debe salir [phoenix.lua] sin "is NOT activated".'
    Write-Host '  Phoenix-DB queda vacia: los datos llegan desde Phoenix Sync / Phoenix Link.'
}
Main
