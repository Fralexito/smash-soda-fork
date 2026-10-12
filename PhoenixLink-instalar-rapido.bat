@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Phoenix Link - Instalador rapido (beta 2)

:: Instala Phoenix Link ya compilado (unos 17 MB). No compila nada: son 2 o 3 minutos.
:: Si prefieres compilarlo en tu PC, usa PhoenixLink-instalar.bat (tarda mucho mas).
set "LOG=%TEMP%\PhoenixLink-instalador.log"
echo [%date% %time%] inicio rapido > "%LOG%"

:: --------- Permisos de administrador ----------
if not defined SKIP_ADMIN_CHECK set "SKIP_ADMIN_CHECK=0"
if not "%SKIP_ADMIN_CHECK%"=="1" (
  net session >nul 2>&1
  if %errorlevel% neq 0 (
    echo.
    echo Este instalador necesita permisos de administrador.
    echo Se va a reabrir pidiendo permiso: si sale una ventana de Windows, pulsa Si.
    echo.
    if not defined SS_ELEVATED (
      set "SS_ELEVATED=1"
      %SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Bypass ^
        -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
      timeout /t 5 >nul
      exit /b 1
    ) else (
      echo No se dio el permiso o no se pudo pedir.
      echo Haz clic derecho sobre el instalador y elige "Ejecutar como administrador".
      pause >nul
      exit /b 1
    )
  )
)

echo.
echo ==========================================================
echo  Phoenix Link - Instalador rapido (beta 2)
echo ==========================================================
echo Herramienta NO oficial para abrir salas en Parsec con mas
echo control de invitados y mandos. Baja Phoenix Link ya
echo compilado y los componentes que le falten a tu PC.
echo Necesita internet. Tarda unos minutos.
echo ==========================================================
echo.
echo Pulsa una tecla para continuar...
pause >nul

:: --------- Configuracion ----------
if not defined PHX_ZIP_URL set "PHX_ZIP_URL=https://github.com/Fralexito/smash-soda-fork/releases/download/beta-2/PhoenixLink-0.2.67-beta2.zip"
if not defined PHX_ZIP_SHA set "PHX_ZIP_SHA=151420705DFDFDC6BE2905188AF9E5251B618BD1707383CAF0B0631647AC7A57"
if not defined PHX_OVERLAY_URL set "PHX_OVERLAY_URL=https://github.com/Fralexito/phoenix-overlay/releases/download/v1.0.0-beta1/PhoenixGlass.exe"
if not defined PHX_OVERLAY_SHA set "PHX_OVERLAY_SHA=DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94"
set "VIGEMBUS_URL=https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe"
set "WEBVIEW2_URL=https://go.microsoft.com/fwlink/p/?LinkId=2124703"
set "VCREDIST_URL=https://aka.ms/vs/17/release/vc_redist.x64.exe"
if not defined STEP_CHECKS set "STEP_CHECKS=1"
if not defined STEP_INSTALL_VCREDIST set "STEP_INSTALL_VCREDIST=1"
if not defined STEP_INSTALL_VIGEMBUS set "STEP_INSTALL_VIGEMBUS=1"
if not defined STEP_INSTALL_WEBVIEW2 set "STEP_INSTALL_WEBVIEW2=1"
if not defined STEP_INSTALL_OVERLAY set "STEP_INSTALL_OVERLAY=1"
if not defined STEP_CREATE_SHORTCUTS set "STEP_CREATE_SHORTCUTS=1"
set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

:: --------- Comprobaciones previas ----------
if "%STEP_CHECKS%"=="0" goto :after_checks
echo.
echo [0/6] Comprobando tu PC...
if /i not "%PROCESSOR_ARCHITECTURE%"=="AMD64" if /i not "%PROCESSOR_ARCHITEW6432%"=="AMD64" (
  call :fallo "Phoenix Link necesita Windows de 64 bits."
  exit /b 1
)
tasklist /fi "imagename eq PhoenixLink.exe" 2>nul | find /i "PhoenixLink.exe" >nul
if not errorlevel 1 (
  call :fallo "Phoenix Link esta abierto. Cierralo y vuelve a ejecutar este instalador."
  exit /b 1
)
set "FREE_GB="
for /f "usebackq delims=" %%G in (`%PS% -NoProfile -Command "[math]::Floor((Get-PSDrive -Name $env:LOCALAPPDATA.Substring(0,1)).Free/1GB)" 2^>nul`) do set "FREE_GB=%%G"
if defined FREE_GB if !FREE_GB! LSS 1 (
  call :fallo "Hay menos de 1 GB libre en el disco C."
  exit /b 1
)
%PS% -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; try { Invoke-WebRequest -Uri 'https://github.com' -UseBasicParsing -Method Head -TimeoutSec 20 | Out-Null } catch { exit 1 }"
if errorlevel 1 (
  call :fallo "No hay conexion con GitHub. Revisa tu internet y vuelve a ejecutar."
  exit /b 1
)
echo Todo en orden.
:after_checks

:: --------- Carpeta de instalacion ----------
set "DEFAULT_INSTALL_PARENT=%ProgramFiles%"
if defined PHX_INSTALL_PARENT (
  set "INSTALL_PARENT=%PHX_INSTALL_PARENT%"
) else (
  echo.
  echo Elige donde instalar. Se crea una carpeta "Phoenix Link" dentro de la que elijas.
  echo Si cierras la ventana de carpetas, se usa %DEFAULT_INSTALL_PARENT%.
  for /f "usebackq delims=" %%i in (`%PS% -NoProfile -ExecutionPolicy Bypass -STA -Command ^
    "Add-Type -AssemblyName System.Windows.Forms; " ^
    "$dlg=New-Object System.Windows.Forms.FolderBrowserDialog; " ^
    "$dlg.Description='Elige la carpeta donde se creara Phoenix Link (por ejemplo C:\Program Files)'; " ^
    "$dlg.SelectedPath='%DEFAULT_INSTALL_PARENT%'; " ^
    "$dlg.ShowNewFolderButton=$true; " ^
    "if($dlg.ShowDialog() -eq 'OK'){ $dlg.SelectedPath }"`) do set "INSTALL_PARENT=%%i"
)
if "%INSTALL_PARENT%"=="" set "INSTALL_PARENT=%DEFAULT_INSTALL_PARENT%"
if "%INSTALL_PARENT:~-1%"=="\" (
  if not "%INSTALL_PARENT:~1,2%"==":\" set "INSTALL_PARENT=%INSTALL_PARENT:~0,-1%"
)
set "INSTALL_DIR=%INSTALL_PARENT%\Phoenix Link"
echo.
echo Se instalara en: "%INSTALL_DIR%"
echo [%time%] instalar en "%INSTALL_DIR%" >> "%LOG%"
if exist "%INSTALL_DIR%\PhoenixLink.exe" (
  echo Ya hay una instalacion de Phoenix Link ahi: se ACTUALIZA. Tus ajustes y datos no se tocan.
)

set "WORK=%LOCALAPPDATA%\PhoenixInstaller"
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" 2>nul
set "ZIP=%WORK%\PhoenixLink.zip"
set "STAGE=%WORK%\stage"
goto :main

:fallo
echo.
echo ==========================================================
echo  NO SE PUDO TERMINAR
echo ==========================================================
echo %~1
echo.
echo Manda a Fralex una foto de esta ventana, o este archivo:
echo   %LOG%
echo ==========================================================
echo [%time%] FALLO: %~1 >> "%LOG%"
pause
exit /b 1

:download
set "URL=%~1"
set "DEST=%~2"
set "LABEL=%~3"
echo Descargando %LABEL%...
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " ^
  "try { Invoke-WebRequest -Uri '%URL%' -OutFile '%DEST%' -UseBasicParsing } catch { " ^
  "  try { Start-BitsTransfer -Source '%URL%' -Destination '%DEST%' } catch { Write-Host $_.Exception.Message; exit 1 } }"
if errorlevel 1 exit /b 1
exit /b 0

:main

:: --------- VC++ runtime ----------
echo.
echo [1/6] Runtime de Visual C++...
if "%STEP_INSTALL_VCREDIST%"=="0" goto :after_vc
reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" /v Installed 2>nul | find "0x1" >nul
if not errorlevel 1 (
  echo Ya esta instalado.
  goto :after_vc
)
call :download "%VCREDIST_URL%" "%WORK%\vc_redist.x64.exe" "Runtime de Visual C++"
if errorlevel 1 (
  call :fallo "No se pudo bajar el runtime de Visual C++."
  exit /b 1
)
start "" /wait "%WORK%\vc_redist.x64.exe" /install /quiet /norestart
set "VC_RC=!errorlevel!"
if not "!VC_RC!"=="0" if not "!VC_RC!"=="3010" if not "!VC_RC!"=="1638" (
  call :fallo "El instalador del runtime de Visual C++ fallo, codigo !VC_RC!."
  exit /b 1
)
echo [%time%] vcredist ok >> "%LOG%"
:after_vc

:: --------- ViGEmBus ----------
echo.
echo [2/6] Controlador de mandos virtuales ViGEmBus...
if "%STEP_INSTALL_VIGEMBUS%"=="0" goto :after_vigem
reg query "HKLM\SYSTEM\CurrentControlSet\Services\ViGEmBus" >nul 2>&1
if not errorlevel 1 (
  echo Ya esta instalado.
  goto :after_vigem
)
call :download "%VIGEMBUS_URL%" "%WORK%\ViGEmBus.exe" "ViGEmBus"
if errorlevel 1 (
  call :fallo "No se pudo bajar ViGEmBus."
  exit /b 1
)
start "" /wait "%WORK%\ViGEmBus.exe" /qn
if errorlevel 1 (
  call :fallo "El instalador de ViGEmBus fallo."
  exit /b 1
)
echo [%time%] vigembus ok >> "%LOG%"
:after_vigem

:: --------- WebView2 ----------
echo.
echo [3/6] Motor de la interfaz WebView2...
if "%STEP_INSTALL_WEBVIEW2%"=="0" goto :after_wv2
set "WV2_OK="
for %%K in (
  "HKLM\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
  "HKLM\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
  "HKCU\Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
) do (
  reg query %%K /v pv >nul 2>&1
  if not errorlevel 1 set "WV2_OK=1"
)
if defined WV2_OK (
  echo Ya esta instalado.
  goto :after_wv2
)
call :download "%WEBVIEW2_URL%" "%WORK%\WebView2Setup.exe" "WebView2"
if errorlevel 1 (
  echo AVISO: no se pudo bajar WebView2. Phoenix Link usara su interfaz simple hasta que lo instales.
  goto :after_wv2
)
start "" /wait "%WORK%\WebView2Setup.exe" /silent /install
echo [%time%] webview2 ok >> "%LOG%"
:after_wv2

:: --------- Phoenix Link ----------
echo.
echo [4/6] Descargando Phoenix Link...
call :download "%PHX_ZIP_URL%" "%ZIP%" "Phoenix Link beta 2"
if errorlevel 1 (
  call :fallo "No se pudo bajar Phoenix Link. Si el enlace aun no esta publicado, pidele a Fralex el archivo."
  exit /b 1
)
%PS% -NoProfile -ExecutionPolicy Bypass -Command "$h=(Get-FileHash -Algorithm SHA256 -LiteralPath '%ZIP%').Hash; if ($h -ne $env:PHX_ZIP_SHA) { exit 2 }"
if errorlevel 1 (
  call :fallo "El archivo bajado no es el esperado (la suma de comprobacion no coincide). No se instalo nada."
  exit /b 1
)
%PS% -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -LiteralPath '%ZIP%' -DestinationPath '%STAGE%' -Force"
if errorlevel 1 (
  call :fallo "No se pudo descomprimir Phoenix Link."
  exit /b 1
)
echo.
echo [5/6] Instalando Phoenix Link...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
xcopy "%STAGE%\*" "%INSTALL_DIR%\" /E /I /Y >nul
if errorlevel 1 (
  call :fallo "No se pudieron copiar los archivos a %INSTALL_DIR%."
  exit /b 1
)
set "FALTA="
for %%F in (PhoenixLink.exe parsec.dll SDL2.dll WebView2Loader.dll ui\index.html sider\phoenix.lua sider\phoenix_referee.lua) do (
  if not exist "%INSTALL_DIR%\%%F" set "FALTA=!FALTA! %%F"
)
if defined FALTA (
  call :fallo "Faltan archivos en la instalacion:!FALTA!"
  exit /b 1
)
echo Phoenix Link instalado.
echo [%time%] link instalado >> "%LOG%"

:: --------- Overlay ----------
set "OVERLAY_DIR=%INSTALL_DIR%\overlay"
set "OVERLAY_ESTADO=no instalado"
echo.
echo [6/6] Overlay PhoenixGlass...
if "%STEP_INSTALL_OVERLAY%"=="0" goto :after_overlay
mkdir "%OVERLAY_DIR%" 2>nul
mkdir "%OVERLAY_DIR%\plugins" 2>nul
mkdir "%OVERLAY_DIR%\themes" 2>nul
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$u=$env:PHX_OVERLAY_URL; $s=$env:PHX_OVERLAY_SHA; $t='%WORK%\PhoenixGlass.exe'; $d='%OVERLAY_DIR%\PhoenixGlass.exe'; " ^
  "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " ^
  "try { Invoke-WebRequest -Uri $u -OutFile $t -UseBasicParsing } catch { Write-Host ('No se pudo bajar: ' + $_.Exception.Message); exit 1 }; " ^
  "$h=(Get-FileHash -Algorithm SHA256 -LiteralPath $t).Hash; " ^
  "if ($h -ne $s) { Remove-Item -LiteralPath $t -Force; Write-Host 'El archivo bajado no es el esperado.'; exit 2 }; " ^
  "Move-Item -LiteralPath $t -Destination $d -Force; Unblock-File -LiteralPath $d; exit 0"
set "OVERLAY_RC=!errorlevel!"
if "!OVERLAY_RC!"=="0" (
  set "OVERLAY_ESTADO=instalado"
  echo Overlay instalado.
) else (
  echo AVISO: el overlay no se instalo, codigo !OVERLAY_RC!. Es opcional: Phoenix Link funciona igual sin el.
)
:after_overlay

echo.
echo ============================================================
echo  INSTALACION TERMINADA
echo ============================================================
echo Phoenix Link instalado en: "%INSTALL_DIR%"
echo Overlay: %OVERLAY_ESTADO%
echo.
echo Siguiente paso: abre Phoenix Link, inicia sesion en Parsec,
echo ve a SYNC y luego a Modulos del juego, y pulsa INSTALAR
echo (con PES 2021 cerrado).
echo ============================================================
if not "%STEP_CREATE_SHORTCUTS%"=="0" call :shortcuts
rmdir /s /q "%WORK%" 2>nul
echo [%time%] fin correcto >> "%LOG%"
echo.
echo Pulsa una tecla para salir...
pause >nul
exit /b 0

:shortcuts
setlocal EnableExtensions DisableDelayedExpansion
set "APP_EXE=%INSTALL_DIR%\PhoenixLink.exe"
if not defined PHX_ACCESO_DIRECTO (
  choice /C SN /N /M "Crear un acceso directo en el escritorio? [S/N]: "
  if errorlevel 2 (
    echo Sin acceso directo.
    exit /b 0
  )
)
for /f "usebackq tokens=*" %%D in (`%PS% -NoProfile -Command "[Environment]::GetFolderPath('Desktop')"`) do set "DESKTOP_PATH=%%D"
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$w=New-Object -ComObject WScript.Shell;" ^
  "$s=$w.CreateShortcut('%DESKTOP_PATH%\Phoenix Link.lnk');" ^
  "$s.TargetPath='%APP_EXE%'; $s.WorkingDirectory='%INSTALL_DIR%'; $s.IconLocation='%APP_EXE%'; $s.Save();"
echo Acceso directo creado en el escritorio.
exit /b 0
