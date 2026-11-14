@echo off
rem Phoenix Online - conecta este PES 2021 al servidor de Frank (solo para pruebas).
rem Uso: clic derecho > Ejecutar como administrador. Pide la IP del servidor.
rem Deshacer: DESCONECTAR_PHOENIX.bat
setlocal EnableDelayedExpansion
net session >nul 2>&1
if errorlevel 1 (
  echo Hay que abrirlo con clic derecho ^> "Ejecutar como administrador".
  pause
  exit /b 1
)
set "IP=%~1"
if "%IP%"=="" set /p "IP=IP del servidor Phoenix (te la da Frank): "
if "%IP%"=="" (
  echo No escribiste ninguna IP. No se cambio nada.
  pause
  exit /b 1
)
set "HOSTS=%SystemRoot%\System32\drivers\etc\hosts"
set "COPIA=%SystemRoot%\System32\drivers\etc\hosts.antes_phoenix"
if not exist "%COPIA%" copy /y "%HOSTS%" "%COPIA%" >nul
rem Quita solo las lineas que puso Phoenix antes (marca #PHOENIX)
findstr /v /c:"#PHOENIX" "%HOSTS%" > "%TEMP%\hosts_phoenix.tmp"
for %%D in (pes21-x64-gate.cs.konami.net pes21-x64-stun.cs.konami.net pesam.stun.service.konami.net info.service.konami.net cs.konami.net ntl.service.konami.net ntleu.service.konami.net ntljp.service.konami.net) do (
  echo %IP% %%D #PHOENIX>> "%TEMP%\hosts_phoenix.tmp"
)
copy /y "%TEMP%\hosts_phoenix.tmp" "%HOSTS%" >nul
del "%TEMP%\hosts_phoenix.tmp"
ipconfig /flushdns >nul
echo.
echo Listo: PES 2021 se conectara al servidor Phoenix en %IP%.
echo Copia del hosts original: %COPIA%
echo Para volver a la normalidad: DESCONECTAR_PHOENIX.bat (como administrador).
pause
