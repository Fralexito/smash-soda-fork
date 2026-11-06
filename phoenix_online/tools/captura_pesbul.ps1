# Graba con pktmon (viene con Windows) TODO el tráfico hacia/desde UNA IP (la del servidor a estudiar). Ejecutar como administrador.
#   iniciar:  powershell -ExecutionPolicy Bypass -File captura_pesbul.ps1 -Accion iniciar -Ip <IP> [-Carpeta <dir>]
#   parar:    powershell -ExecutionPolicy Bypass -File captura_pesbul.ps1 -Accion parar   [-Carpeta <dir>]
# La IP NO se guarda en este archivo. Las capturas van a -Carpeta (por defecto capturas_pesbul junto a phoenix_online) y NUNCA al repo.
param(
  [Parameter(Mandatory=$true)][ValidateSet('iniciar','parar','estado')][string]$Accion,
  [string]$Ip,
  [string]$Carpeta = 'D:\Frank\IA\CLAUDE\pes-online\capturas_pesbul'
)
New-Item -ItemType Directory -Force -Path $Carpeta | Out-Null
$etl = Join-Path $Carpeta 'pesbul.etl'
switch ($Accion) {
  'iniciar' {
    if (-not $Ip) { throw 'Falta -Ip' }
    pktmon stop 2>$null | Out-Null
    pktmon filter remove | Out-Null
    pktmon filter add 'pesbul' -i $Ip | Out-Null          # TCP y UDP de/hacia esa IP, cualquier puerto
    pktmon start --capture --comp nics --pkt-size 0 -f $etl | Out-Null
    Write-Host "Capturando hacia/desde $Ip -> $etl"
  }
  'parar' {
    pktmon stop | Out-Null
    $stamp = Get-Date -Format yyyyMMdd_HHmmss
    $pcap = Join-Path $Carpeta "pesbul_$stamp.pcapng"
    pktmon etl2pcap $etl -o $pcap | Out-Null
    Write-Host "Captura detenida. pcapng: $pcap"
  }
  'estado' { pktmon status }
}
