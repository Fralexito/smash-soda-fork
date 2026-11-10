@echo off
rem Phoenix Online - liga ESTA PC a tu cuenta de la web (pase de 10 minutos).
rem 1) En la web Phoenix, con tu sesion iniciada, pide un pase para el online y copialo.
rem 2) Abre este archivo (doble clic). No hace falta ser administrador.
rem Desde ese momento, al entrar al online tendras siempre TU club, aunque cambies de PC.
rem El pase solo sirve una vez y caduca a los 10 minutos.
setlocal
set "IP=%~1"
if "%IP%"=="" set /p "IP=IP del servidor Phoenix (te la da Frank): "
set /p "PASE=Pega tu pase y pulsa Enter: "
if "%IP%"=="" goto falta
if "%PASE%"=="" goto falta
powershell -NoProfile -Command "try { $r = Invoke-RestMethod -Method Post -Uri ('http://' + $env:IP + ':5000/pase') -ContentType 'application/json' -Body (@{pase=$env:PASE} | ConvertTo-Json); Write-Host $r.mensaje } catch { $m = $_.ErrorDetails.Message; if ($m) { Write-Host (($m | ConvertFrom-Json).mensaje) } else { Write-Host ('No se pudo hablar con el servidor: ' + $_.Exception.Message) } }"
pause
exit /b 0
:falta
echo Falta la IP o el pase. No se cambio nada.
pause
exit /b 1
