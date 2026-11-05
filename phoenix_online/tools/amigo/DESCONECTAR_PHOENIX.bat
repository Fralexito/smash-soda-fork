@echo off
rem Phoenix Online - quita la conexion al servidor de Frank (deja el hosts como estaba).
net session >nul 2>&1
if errorlevel 1 (
  echo Hay que abrirlo con clic derecho ^> "Ejecutar como administrador".
  pause
  exit /b 1
)
set "HOSTS=%SystemRoot%\System32\drivers\etc\hosts"
findstr /v /c:"#PHOENIX" "%HOSTS%" > "%TEMP%\hosts_phoenix.tmp"
copy /y "%TEMP%\hosts_phoenix.tmp" "%HOSTS%" >nul
del "%TEMP%\hosts_phoenix.tmp"
ipconfig /flushdns >nul
echo Listo: se quitaron las lineas de Phoenix. PES vuelve a lo normal.
pause
