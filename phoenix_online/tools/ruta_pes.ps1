# Rutas de teclas ya probadas para el PES BASE (C:\PES21), de un solo golpe.
# Uso:  powershell -ExecutionPolicy Bypass -File ruta_pes.ps1 <ruta>
#   rutas: arranque | matchday | torneo | coop | bandeja
# Cada ruta parte de donde dice su cabecera. Usa el ayudante del MCP de PES (SendInput en la ventana de PES).
param([Parameter(Mandatory=$true)][string]$Ruta)
$ay = 'D:\Frank\IA\CLAUDE\pes-mcp\ayudante.ps1'
$fr = 'C:\Temp\claude\online\frente2.ps1'
function T($k,$n=1,$e=1){ & powershell -NoProfile -ExecutionPolicy Bypass -File $fr | Out-Null; & powershell -NoProfile -ExecutionPolicy Bypass -File $ay -Accion tecla -Teclas $k -Veces $n | Out-Null; Start-Sleep -Seconds $e }
switch ($Ruta) {
  # PES recién abierto (pantalla «THE PITCH IS OURS») -> menú principal
  'arranque' { T Enter 1 24; T Enter 2 3 }
  # menú principal -> pantalla principal de Matchday (ya inscrito)
  'matchday' { T Derecha 1 1; T Enter 1 2; T Abajo 3 1; T Enter 1 13; T Enter 1 11; T Enter 1 9; T Enter 1 14 }
  # menú principal -> Torneo en línea hasta la tarjeta (luego: EULA/equipo a mano)
  'torneo'   { T Derecha 1 1; T Enter 1 2; T Abajo 4 1; T Enter 1 13; T Enter 1 11; T Enter 1 10 }
  # menú principal -> Cooperativo -> Partido de clan -> crear salón
  'coop'     { T Derecha 1 1; T Enter 1 2; T Abajo 2 1; T Enter 1 13; T Enter 1 9; T Derecha 1 1; T Enter 1 10; T Enter 1 9; T Enter 1 9; T Enter 1 12 }
  # menú principal -> myClub -> Empezar -> Bandeja entrada
  'bandeja'  { T Abajo 1 1; T Izquierda 1 1; T Enter 1 16; T Enter 1 21; T Enter 1 14; T Enter 1 12; T Abajo 1 1; T Derecha 1 1; T Enter 1 9 }
  default { Write-Error "ruta desconocida: $Ruta" }
}
Write-Output "ruta $Ruta hecha"
