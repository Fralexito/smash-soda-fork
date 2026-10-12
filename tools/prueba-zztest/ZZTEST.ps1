# Prueba ZZTEST (SOLO LECTURA de PES): localiza en que archivo guarda PES los "Datos personales" (Usuario N).
# ANTES.bat  -> respaldo con sello + listado.   DESPUES.bat -> compara y escribe INFORME.txt
param([ValidateSet('antes','despues')][string]$Fase)
$ErrorActionPreference = 'Continue'
$out = Join-Path $env:USERPROFILE 'Desktop\ZZTEST'
New-Item -ItemType Directory -Force $out | Out-Null

# Rutas donde PES / el emulador CPY / Steam pueden guardar. Solo se usan las que existen.
$od = $env:OneDrive
$roots = @(
  "$env:USERPROFILE\Documents\KONAMI",
  "$env:USERPROFILE\OneDrive\Documents\KONAMI",
  $(if($od){ "$od\Documents\KONAMI" }),
  "$env:APPDATA\CPY_SAVES",
  "$env:ProgramData\KONAMI",
  "$env:PUBLIC\Documents\KONAMI",
  "${env:ProgramFiles(x86)}\Steam\userdata",
  "$env:ProgramFiles\Steam\userdata"
)
# Todas las cuentas de Windows de esta PC (PES puede guardar en otra cuenta, p. ej. la del OneDrive)
foreach($u in Get-ChildItem (Split-Path $env:USERPROFILE) -Directory -ErrorAction SilentlyContinue){
  $roots += "$($u.FullName)\Documents\KONAMI","$($u.FullName)\OneDrive\Documents\KONAMI","$($u.FullName)\AppData\Roaming\CPY_SAVES"
}
$roots = $roots | ?{ $_ } | select -Unique | ?{ Test-Path $_ }
# Carpeta del juego (se pide una vez y se recuerda): solo se mira, p. ej. settings.dat o save\
$cfg = "$out\ruta-juego.txt"
if(-not (Test-Path $cfg)){
  $g = Read-Host 'Pega la carpeta de tu PES (donde esta PES2021.exe) o pulsa Enter para omitir'
  Set-Content $cfg $g.Trim('" ')
}
$juego = (Get-Content $cfg -ErrorAction SilentlyContinue | select -First 1)
if($juego -and (Test-Path $juego)){ foreach($sub in 'save','settings.dat'){ $p=Join-Path $juego $sub; if(Test-Path $p){ $roots += $p } } }

function Foto {
  foreach($r in $roots){
    $items = if((Get-Item $r).PSIsContainer){ Get-ChildItem $r -Recurse -File -Force -ErrorAction SilentlyContinue } else { Get-Item $r }
    $items | ?{ $_.FullName -notmatch '[\\/](Download|Crashpad)[\\/]' -and $_.Extension -notin '.pak','.ucas','.utoc','.cpk' } | %{
      [pscustomobject]@{ Path=$_.FullName; Size=$_.Length; Time=$_.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'); Sha=(Get-FileHash $_.FullName -Algorithm SHA1 -ErrorAction SilentlyContinue).Hash }
    }
  }
}

if(Get-Process PES2021,Settings -ErrorAction SilentlyContinue){ Write-Host 'AVISO: PES o Settings estan abiertos. Cierralos y vuelve a ejecutar.' -ForegroundColor Yellow; exit 1 }

$snap = Foto
if($Fase -eq 'antes'){
  $sello = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'
  $dest = "$out\respaldo-$sello"
  foreach($f in $snap){ $rel = $f.Path -replace '^[A-Za-z]:','' -replace '^[\\/]',''; $d = Join-Path $dest $rel; New-Item -ItemType Directory -Force (Split-Path $d) | Out-Null; Copy-Item $f.Path $d -Force -ErrorAction SilentlyContinue }
  $snap | Export-Csv "$out\antes.csv" -NoTypeInformation -Encoding UTF8
  $snap | select Time,Size,Path | Sort-Object Path | Format-Table -AutoSize | Out-String -Width 300 | Set-Content "$out\listado-antes.txt"
  "Carpetas revisadas:`n$($roots -join "`n")" | Set-Content "$out\carpetas.txt"
  Write-Host "LISTO. $($snap.Count) archivos anotados y copiados a $dest" -ForegroundColor Green
  Write-Host 'Ahora: abre PES > Datos personales > edita un Usuario libre (nombre ZZTEST + cambia un boton) > SAL de PES por completo > ejecuta DESPUES.bat'
} else {
  if(-not (Test-Path "$out\antes.csv")){ Write-Host 'Primero ejecuta ANTES.bat' -ForegroundColor Yellow; exit 1 }
  $a = Import-Csv "$out\antes.csv"; $ha=@{}; $a | %{ $ha[$_.Path]=$_ }
  $hn=@{}; $snap | %{ $hn[$_.Path]=$_ }
  $nuevos = $snap | ?{ -not $ha.ContainsKey($_.Path) }
  $cambiados = $snap | ?{ $ha.ContainsKey($_.Path) -and $ha[$_.Path].Sha -ne $_.Sha }
  $borrados = $a | ?{ -not $hn.ContainsKey($_.Path) }
  $txt = @("INFORME ZZTEST  $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')", "PC: $env:COMPUTERNAME  Usuario: $env:USERNAME", '', "Carpetas revisadas:", ($roots -join "`n"), '', '== CAMBIADOS ==')
  $txt += ($cambiados | %{ "$($_.Time)  $($_.Size) bytes (antes $($ha[$_.Path].Size))  $($_.Path)" })
  $txt += '', '== NUEVOS =='; $txt += ($nuevos | %{ "$($_.Time)  $($_.Size) bytes  $($_.Path)" })
  $txt += '', '== BORRADOS =='; $txt += ($borrados | %{ $_.Path })
  $txt | Set-Content "$out\INFORME.txt" -Encoding UTF8
  $txt | Write-Host
  Write-Host "`nListo. Envia el archivo: $out\INFORME.txt" -ForegroundColor Green
}
