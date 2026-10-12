@echo off
setlocal
title Phoenix - modulos Lua para PES 2021
rem ============================================================================
rem  Instala (o quita) los modulos Lua de Phoenix en PES 2021 + ConmeGOL, igual que
rem  el boton "Modulos del juego" de Phoenix Link, sin necesitar Link:
rem    phoenix.lua          v0.18b  Phoenix Sync (avisos de la web, recarga de datos)
rem    phoenix_referee.lua  v1.3    Phoenix Referee (marcador, goles, HUD del partido)
rem   - en <juego>\SiderAddons y en cada modo del parche (...\<modo>\SiderAddons)
rem   - copia de sider.ini (sider.ini.phoenix-<fecha>) antes de tocarlo, y vuelta atras si algo falla
rem   - lua.module = "<modulo>" debajo del ultimo modulo activo
rem   - cpk.root = ".\livecpk\Phoenix-DB" SOLO si el sider.ini tiene una base de parche reconocida
rem     (una raiz con common\etc\pesdb\Player.bin o olmosjr23\Database); si no, no se pone
rem     (regla "sin base reconocida no se pone": con el parche 27 taparia los jugadores nuevos)
rem   - guarda <modulo>.antes-v<version> si habia otra version
rem  Usa el modulo de esta misma carpeta o lo baja de GitHub (version fija); en ambos casos
rem  comprueba su sha256. Si algo no cuadra, no toca nada.
rem  Doble clic = instalar. Para quitar Phoenix del juego: PhoenixJuego-instalar.bat desinstalar
rem  (quita las lineas de sider.ini y aparta los .lua con otro nombre; no borra nada).
rem ============================================================================
set "PHX_BAT=%~f0"
set "PHX_DIR=%~dp0"
set "PHX_MODO=%~1"
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
    $desinstalar = ($env:PHX_MODO -ieq 'desinstalar')
    $COMMIT = 'd0ae1b61a51ab82bd5dfe818daad7ab10fe7c676'   # version fija de Phoenix Link de donde salen los modulos
    $BASE = "https://raw.githubusercontent.com/Fralexito/smash-soda-fork/$COMMIT/SmashSoda/phoenix/sider/"
    $MODULOS = @(
        @{ N = 'phoenix.lua';         V = '0.18b'; S = '9ED74F457B4C2F407D4851B09B701A5FFB63E3A1F5707B322C13AAD6F95526AB'; Db = $true;  Viejo = '' },
        @{ N = 'phoenix_referee.lua'; V = '1.3';   S = '3A702AD7A3D050B9D150C60A6F22092C3F0A3CDF39F1AD8387826C0B6CC2A7E3'; Db = $false; Viejo = 'phoenix_estadio.lua' }
    )
    $L1 = [Text.Encoding]::GetEncoding(28591)   # lee y escribe los bytes tal cual
    $sello = Get-Date -Format 'yyyyMMdd-HHmmss'

    Write-Host ''
    Write-Host $(if ($desinstalar) { '  PHOENIX - quitar los modulos Lua de PES 2021' } else { '  PHOENIX - modulos Lua para PES 2021 (beta 2)' }) -ForegroundColor Cyan
    Write-Host '  ------------------------------------------' -ForegroundColor Cyan
    Write-Host ''

    # 1) PES cerrado: Sider lee los modulos al arrancar
    $abierto = Get-Process -Name 'PES2021', 'sider' -ErrorAction SilentlyContinue
    if ($abierto) {
        Write-Host '  [X] PES 2021 (o Sider) esta abierto. Cierralo y vuelve a ejecutar este archivo.' -ForegroundColor Red
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
        Write-Host "  [X] En '$juego' no esta SiderAddons\sider.ini. Elige la carpeta principal del juego (la de PES2021.exe)." -ForegroundColor Red
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
    foreach ($d in $destinos) {
        if (-not (Test-Path -LiteralPath (Join-Path $d 'modules') -PathType Container)) {
            Write-Host "  [X] Falta la carpeta modules en $d. No se toca nada." -ForegroundColor Red
            return
        }
    }

    # 3b) Un sider.ini de solo lectura es una decision tuya: no se pisa. Se avisa antes de tocar nada.
    foreach ($d in $destinos) {
        if ((Get-Item -LiteralPath (Join-Path $d 'sider.ini')).IsReadOnly) {
            Write-Host "  [X] El sider.ini de $d esta marcado como SOLO LECTURA. Quitale esa marca (clic derecho > Propiedades) y repite. No se toca nada." -ForegroundColor Red
            return
        }
    }

    # 4) Los .lua: de esta carpeta o de GitHub, siempre con sha256 (solo al instalar)
    $fuente = @{}
    if (-not $desinstalar) {
        foreach ($m in $MODULOS) {
            $local = Join-Path $env:PHX_DIR $m.N
            if ((Test-Path -LiteralPath $local) -and ((Hash $local) -eq $m.S)) {
                $fuente[$m.N] = $local
                Write-Host "  $($m.N) v$($m.V): el de esta carpeta."
                continue
            }
            Write-Host "  Bajando $($m.N) v$($m.V) de GitHub..."
            $tmp = Join-Path $env:TEMP "phoenix-$sello-$($m.N)"
            try {
                [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
                Invoke-WebRequest -Uri ($BASE + $m.N) -OutFile $tmp -UseBasicParsing
            } catch {
                Write-Host "  [X] No se pudo bajar $($m.N). Revisa tu internet, o pon $($m.N) junto a este .bat y repite. No se toco nada." -ForegroundColor Red
                return
            }
            if ((Hash $tmp) -ne $m.S) {
                Remove-Item -LiteralPath $tmp -Force
                Write-Host "  [X] El $($m.N) bajado no es la v$($m.V) esperada. No se toca nada; avisa a Fralex." -ForegroundColor Red
                return
            }
            $fuente[$m.N] = $tmp
        }
    }

    # 5) Comprobar TODOS los sider.ini antes de tocar ninguno
    $rxRaiz = '^\s*((?:[;#]\s*)*)cpk\.root\s*=\s*"([^"]*)"'
    $rxMod = '^\s*((?:[;#]\s*)*)lua\.module\s*=\s*"([^"]*)"'
    $esPhx = { param($r) $r.TrimEnd('\', '/') -match '(?i)phoenix-db$' }
    $planes = @()
    $sinBase = @()
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

        foreach ($m in $MODULOS) {
            if ($desinstalar) {
                # quita todas las lineas del modulo (activas o comentadas); con phoenix.lua, tambien la raiz Phoenix-DB
                for ($i = $v.Count - 1; $i -ge 0; $i--) {
                    $mm = [regex]::Match($v[$i], $rxMod)
                    if ($mm.Success -and $mm.Groups[2].Value -ieq $m.N) { $v.RemoveAt($i); $cambio = $true }
                }
                if ($m.Db) {
                    for ($i = $v.Count - 1; $i -ge 0; $i--) {
                        $mm = [regex]::Match($v[$i], $rxRaiz)
                        if ($mm.Success -and (& $esPhx $mm.Groups[2].Value)) { $v.RemoveAt($i); $cambio = $true }
                    }
                }
                continue
            }

            # 5a) raiz Phoenix-DB (solo phoenix.lua) antes de la primera raiz con la base reconocida del parche
            if ($m.Db) {
                $activa = $false; $comentadaPhx = -1; $primeraBase = -1
                for ($i = 0; $i -lt $v.Count; $i++) {
                    $mm = [regex]::Match($v[$i], $rxRaiz)
                    if (-not $mm.Success) { continue }
                    $com = $mm.Groups[1].Value.Length -gt 0
                    $r = $mm.Groups[2].Value
                    if (& $esPhx $r) {
                        if (-not $com) { $activa = $true } elseif ($comentadaPhx -lt 0) { $comentadaPhx = $i }
                        continue
                    }
                    if ($com) { continue }
                    $rn = ($r.ToLower()).Replace('/', '\')
                    $esOlmos = $rn.EndsWith('olmosjr23\database')
                    if ($primeraBase -lt 0 -and ($esOlmos -or (Test-Path -LiteralPath (Join-Path (Join-Path $d $r) 'common\etc\pesdb\Player.bin')))) { $primeraBase = $i }
                }
                if (-not $activa) {
                    if ($primeraBase -ge 0) {
                        $donde = $primeraBase
                        if ($comentadaPhx -ge 0) { $v.RemoveAt($comentadaPhx); if ($comentadaPhx -lt $donde) { $donde-- } }
                        $v.Insert($donde, 'cpk.root = ".\livecpk\Phoenix-DB"')
                        $cambio = $true
                    } else {
                        $sinBase += $d   # sin base reconocida: Phoenix-DB NO se pone
                    }
                }
            }

            # 5b) lua.module = "<modulo>" (descomenta la suya o va debajo del ultimo modulo activo)
            $ya = $false; $comentadaMod = -1; $ultimaActiva = -1; $ultima = -1
            for ($i = 0; $i -lt $v.Count; $i++) {
                $mm = [regex]::Match($v[$i], $rxMod)
                if (-not $mm.Success) { continue }
                $com = $mm.Groups[1].Value.Length -gt 0
                if ($mm.Groups[2].Value -ieq $m.N) {
                    if (-not $com) { $ya = $true } elseif ($comentadaMod -lt 0) { $comentadaMod = $i }
                }
                $ultima = $i
                if (-not $com) { $ultimaActiva = $i }
            }
            if (-not $ya) {
                if ($comentadaMod -ge 0) { $v[$comentadaMod] = "lua.module = `"$($m.N)`"" }
                else {
                    $tras = if ($ultimaActiva -ge 0) { $ultimaActiva } else { $ultima }
                    if ($tras -lt 0) { Write-Host "  [X] $ruta no tiene lista de modulos (lua.module). No se toca nada." -ForegroundColor Red; return }
                    $v.Insert($tras + 1, "lua.module = `"$($m.N)`"")
                }
                $cambio = $true
            }
            # 5c) el modulo viejo con otro nombre se apaga (si quedaran los dos, escribirian el mismo estado.json)
            if ($m.Viejo) {
                for ($i = $v.Count - 1; $i -ge 0; $i--) {
                    $mm = [regex]::Match($v[$i], $rxMod)
                    if ($mm.Success -and $mm.Groups[2].Value -ieq $m.Viejo) { $v.RemoveAt($i); $cambio = $true }
                }
            }
        }
        $nuevo = [string]::Join($nl, $v) + $(if ($finalNl) { $nl } else { '' })
        $planes += [pscustomobject]@{ Dir = $d; Ini = $ruta; Nuevo = $nuevo; Cambio = $cambio }
    }

    # 6) Aplicar. Si algo falla a medias, los sider.ini ya escritos vuelven a su copia.
    $hechos = 0
    $escritos = @()   # @{ Ini; Copia }
    try {
        foreach ($p in $planes) {
            Write-Host "  > $($p.Dir)"
            # 6a) los .lua (sin linea en sider.ini todavia no hacen nada)
            if (-not $desinstalar) {
                foreach ($c in @('content\phoenix', 'modules')) { New-Item -ItemType Directory -Force -Path (Join-Path $p.Dir $c) | Out-Null }
                foreach ($m in $MODULOS) {
                    $dest = Join-Path $p.Dir "modules\$($m.N)"
                    if (Test-Path -LiteralPath $dest) {
                        if ((Hash $dest) -eq $m.S) { Write-Host "      $($m.N) v$($m.V) ya estaba"; continue }
                        $vieja = [regex]::Match([IO.File]::ReadAllText($dest, $L1), 'version\s*=\s*"([^"]+)"').Groups[1].Value
                        $stem = [IO.Path]::GetFileNameWithoutExtension($m.N)
                        $copia = if ($vieja) { "$stem.lua.antes-v$vieja" } else { "$stem.lua.antes-$sello" }
                        if (Test-Path -LiteralPath (Join-Path $p.Dir "modules\$copia")) { $copia = "$copia-$sello" }
                        Copy-Item -LiteralPath $dest -Destination (Join-Path $p.Dir "modules\$copia") -Force
                        Write-Host "      version anterior de $($m.N) guardada como modules\$copia"
                    }
                    Copy-Item -LiteralPath $fuente[$m.N] -Destination $dest -Force
                    if ((Hash $dest) -ne $m.S) { throw "La copia de $($m.N) no quedo identica en $($p.Dir)." }
                    Write-Host "      $($m.N) v$($m.V) copiado"
                }
                # carpetas vacias de Phoenix-DB: solo donde se puso su raiz
                if ($sinBase -notcontains $p.Dir) {
                    New-Item -ItemType Directory -Force -Path (Join-Path $p.Dir 'livecpk\Phoenix-DB\common\etc\pesdb') | Out-Null
                }
            }
            # 6b) sider.ini: copia con sello, escritura en un archivo temporal y reemplazo
            if ($p.Cambio) {
                $copiaIni = "$($p.Ini).phoenix-$sello"
                Copy-Item -LiteralPath $p.Ini -Destination $copiaIni -Force
                $tmpIni = "$($p.Ini).tmp-$sello"
                [IO.File]::WriteAllText($tmpIni, $p.Nuevo, $L1)
                Move-Item -LiteralPath $tmpIni -Destination $p.Ini -Force
                $escritos += @{ Ini = $p.Ini; Copia = $copiaIni }
                Write-Host "      sider.ini actualizado (copia: sider.ini.phoenix-$sello)"
            } else {
                Write-Host '      sider.ini no necesitaba cambios'
            }
            if ($sinBase -contains $p.Dir) { Write-Host '      Phoenix-DB NO se puso: este sider.ini no tiene una base de parche reconocida (tus datos del parche no se tapan).' -ForegroundColor Yellow }
            # 6c) desinstalar: los .lua se apartan con otro nombre (no se borra nada)
            if ($desinstalar) {
                foreach ($m in $MODULOS) {
                    $f = Join-Path $p.Dir "modules\$($m.N)"
                    if (Test-Path -LiteralPath $f) {
                        Move-Item -LiteralPath $f -Destination "$f.desinstalado-$sello" -Force
                        Write-Host "      $($m.N) apartado como $($m.N).desinstalado-$sello"
                    }
                }
            }
            $hechos++
        }
    } catch {
        Write-Host ''
        Write-Host "  [X] Algo fallo: $($_.Exception.Message)" -ForegroundColor Red
        foreach ($e in $escritos) {
            try { Copy-Item -LiteralPath $e.Copia -Destination $e.Ini -Force; Write-Host "      sider.ini restaurado: $($e.Ini)" } catch { Write-Host "      NO se pudo restaurar $($e.Ini); la copia esta en $($e.Copia)" -ForegroundColor Red }
        }
        Write-Host '  Se dejo todo como estaba. Avisa a Fralex con una foto de esta ventana.' -ForegroundColor Yellow
        return
    }

    Write-Host ''
    if ($desinstalar) {
        Write-Host "  LISTO: Phoenix quitado del juego en $hechos carpeta(s). Para volver a ponerlo, ejecuta este .bat sin 'desinstalar'." -ForegroundColor Green
    } else {
        Write-Host "  LISTO: modulos Phoenix en $hechos carpeta(s)." -ForegroundColor Green
        Write-Host '  Abre PES 2021. En SiderAddons\sider.log debe salir [phoenix.lua] sin "is NOT activated".'
        if ($sinBase.Count -lt $planes.Count) { Write-Host '  Phoenix-DB queda vacia: los datos llegan desde Phoenix Sync / Phoenix Link.' }
    }
}
try { Main } catch {
    Write-Host ""
    Write-Host ("  [X] No se pudo completar: " + $_.Exception.Message) -ForegroundColor Red
    Write-Host "  Si un archivo esta en uso, cierra PES, Sider y los programas que lo tengan abierto (antivirus, editor) y repite. Si lo hizo a medias, los sider.ini ya cambiados se restauraron." -ForegroundColor Yellow
}
