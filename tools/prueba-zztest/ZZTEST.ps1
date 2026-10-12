# Prueba ZZTEST (SOLO LECTURA): foto de las carpetas de guardado de PES antes y despues de editar un Usuario.
# Uso: ZZTEST.ps1 antes   (con PES cerrado)  ->  editar Usuario 32 en PES (nombre ZZTEST + 1 boton) y SALIR de PES  ->  ZZTEST.ps1 despues
param([ValidateSet('antes','despues')][string]$Fase)
$out = Join-Path $env:USERPROFILE 'Desktop\ZZTEST'
New-Item -ItemType Directory -Force $out | Out-Null
$roots = @("$env:USERPROFILE\Documents\KONAMI","$env:USERPROFILE\OneDrive\Documents\KONAMI","$env:APPDATA\CPY_SAVES","$env:ProgramData\KONAMI","${env:ProgramFiles(x86)}\Steam\userdata")
$game = Get-ItemPropertyValue 'HKCU:\Software\Valve\Steam' SteamPath -ErrorAction SilentlyContinue
$files = foreach($r in $roots){ if(Test-Path $r){ Get-ChildItem $r -Recurse -File -Force -ErrorAction SilentlyContinue | ?{ $_.FullName -notmatch '[\\/]Download[\\/]' } } }
$snap = $files | %{ [pscustomobject]@{ Path=$_.FullName; Size=$_.Length; Time=$_.LastWriteTimeUtc.ToString('o'); Sha=(Get-FileHash $_.FullName -Algorithm SHA1).Hash } }
$snap | Export-Csv "$out\$Fase.csv" -NoTypeInformation
if($Fase -eq 'antes'){
  foreach($f in $files | ?{ $_.FullName -match 'KONAMI|CPY_SAVES' }){ $dst = Join-Path "$out\respaldo" ($f.FullName -replace '[:\]','_'); New-Item -ItemType Directory -Force "$out\respaldo" | Out-Null; Copy-Item $f.FullName $dst -Force }
  "Listo: $($snap.Count) archivos anotados y copiados a $out. Ahora edita el Usuario 32 en PES (nombre ZZTEST + cambia un boton) y SAL de PES."
} else {
  $a = Import-Csv "$out\antes.csv"
  $ha = @{}; $a | %{ $ha[$_.Path] = $_.Sha }
  $cambios = $snap | ?{ -not $ha.ContainsKey($_.Path) -or $ha[$_.Path] -ne $_.Sha }
  $cambios | ft Path,Size,Time -auto | Out-String -Width 250 | Tee-Object "$out\cambios.txt"
  "Resultado en $out\cambios.txt (pasamelo)."
}
