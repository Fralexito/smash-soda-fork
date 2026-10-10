@echo off
rem Phoenix Sync automatico: doble clic UNA vez.
rem  - Copia PhoenixSync.exe a %APPDATA%\Phoenix Mercado\bin
rem  - Deja "Phoenix Sync.bat" en la carpeta Inicio de Windows: Sync arranca solo
rem    (ventana minimizada) cada vez que enciendes la PC y vigila los fichajes.
rem  - Lo arranca ya, si no estaba funcionando.
rem Para quitarlo: Win+R -> shell:startup -> borra "Phoenix Sync.bat".
setlocal
set "ORIGEN=%~dp0..\build-sync\Release\PhoenixSync.exe"
set "BIN=%APPDATA%\Phoenix Mercado\bin"
set "INICIO=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"

if not exist "%ORIGEN%" (echo No encuentro "%ORIGEN%". Compila PhoenixSync primero. & pause & exit /b 1)
tasklist /fi "imagename eq PhoenixSync.exe" | find /i "PhoenixSync.exe" >nul && (
  echo Phoenix Sync esta abierto. Cierra su ventana ^(o pulsa Ctrl+C en ella^) y vuelve a abrir este archivo.
  pause & exit /b 1
)
if not exist "%BIN%" mkdir "%BIN%"
copy /y "%ORIGEN%" "%BIN%\PhoenixSync.exe" >nul || (echo No se pudo copiar PhoenixSync.exe & pause & exit /b 1)

> "%BIN%\PhoenixSyncAuto.bat" (
  echo @echo off
  echo title Phoenix Sync ^(automatico^) - no cierres esta ventana
  echo cd /d "%%~dp0"
  echo tasklist /fi "imagename eq PhoenixSync.exe" ^| find /i "PhoenixSync.exe" ^>nul ^&^& exit /b
  echo :vuelta
  echo PhoenixSync.exe sync-vigilar 1440
  echo timeout /t 10 /nobreak ^>nul
  echo goto vuelta
)
> "%INICIO%\Phoenix Sync.bat" (
  echo @echo off
  echo start "Phoenix Sync" /min "%BIN%\PhoenixSyncAuto.bat"
)
start "Phoenix Sync" /min "%BIN%\PhoenixSyncAuto.bat"
echo.
echo   Listo. Phoenix Sync ya esta vigilando ^(ventana minimizada^)
echo   y arrancara solo cada vez que enciendas la PC.
echo.
pause
