@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Phoenix Link - Instalador (beta 1)

:: Registro de lo que hace este instalador (para depurar sin acceso a tu PC).
set "LOG=%TEMP%\PhoenixLink-instalador.log"
echo [%date% %time%] inicio > "%LOG%"

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
      if "%~1"=="" (
        %SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Bypass ^
          -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
      ) else (
        %SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Bypass ^
          -Command "Start-Process -FilePath '%~f0' -Verb RunAs -ArgumentList '%*'"
      )
      timeout /t 5 >nul
      exit /b 1
    ) else (
      echo No se dio el permiso o no se pudo pedir.
      echo Haz clic derecho sobre el instalador y elige "Ejecutar como administrador".
      echo.
      pause >nul
      exit /b 1
    )
  )
)

echo.
echo ==========================================================
echo  Phoenix Link - Instalador (beta 1)
echo ==========================================================
echo Phoenix Link es una herramienta NO oficial para abrir salas
echo en Parsec con mas control de invitados y mandos. No tiene
echo relacion con Parsec.
echo.
echo Este instalador baja las herramientas necesarias y COMPILA
echo Phoenix Link en tu PC. Se compila aqui porque la licencia del
echo SDK de Parsec no permite repartir el programa ya compilado.
echo.
echo - Necesita internet y unos 12 GB libres en el disco C.
echo - Tarda entre 30 y 90 minutos la primera vez. No cierres la
echo   ventana aunque parezca quieta; pulsa una tecla si no avanza.
echo - Si falla, no se toca nada de tu juego. Manda una foto de
echo   la ventana, o el archivo %LOG%
echo ==========================================================
echo.
echo Pulsa una tecla para continuar...
pause >nul

:: --------- Configuracion ----------
:: Version exacta de Phoenix Link (commit). La rama tiene una enie en el nombre y en .bat falla: se usa el SHA.
if not defined PHX_COMMIT set "PHX_COMMIT=a59c4b115461b52f27ad85762ae2040cae27282f"
set "COMMIT=%PHX_COMMIT%"
set "REPO_URL=https://github.com/Fralexito/smash-soda-fork.git"
:: Overlay PhoenixGlass (repo Fralexito/phoenix-overlay). Con sha256: si no coincide, no se instala.
if not defined PHX_OVERLAY_URL set "PHX_OVERLAY_URL=https://github.com/Fralexito/phoenix-overlay/releases/download/v1.0.0-beta1/PhoenixGlass.exe"
if not defined PHX_OVERLAY_SHA set "PHX_OVERLAY_SHA=DFD0BB6AC8513C8189A10E39DA4A85E4A12D1A70873340F84C271CF299BE2E94"
set "VIGEMBUS_URL=https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe"
:: WebView2 (motor de la interfaz): instalador oficial de Microsoft
set "WEBVIEW2_URL=https://go.microsoft.com/fwlink/p/?LinkId=2124703"

:: --------- Pasos (1=hacer, 0=saltar). Se pueden fijar desde fuera para probar. ----------
if not defined STEP_CHECKS set "STEP_CHECKS=1"
if not defined STEP_INSTALL_GIT set "STEP_INSTALL_GIT=1"
if not defined STEP_INSTALL_BUILD_TOOLS set "STEP_INSTALL_BUILD_TOOLS=1"
if not defined STEP_INSTALL_CMAKE set "STEP_INSTALL_CMAKE=1"
if not defined STEP_INSTALL_VIGEMBUS set "STEP_INSTALL_VIGEMBUS=1"
if not defined STEP_INSTALL_WEBVIEW2 set "STEP_INSTALL_WEBVIEW2=1"
if not defined STEP_CLONE_SOURCE set "STEP_CLONE_SOURCE=1"
if not defined STEP_BUILD set "STEP_BUILD=1"
if not defined STEP_INSTALL_APP set "STEP_INSTALL_APP=1"
if not defined STEP_MIGRATE_DATA set "STEP_MIGRATE_DATA=1"
if not defined STEP_INSTALL_OVERLAY set "STEP_INSTALL_OVERLAY=1"
if not defined STEP_CREATE_SHORTCUTS set "STEP_CREATE_SHORTCUTS=1"
if not defined STEP_CLEANUP set "STEP_CLEANUP=1"

:: VS Build Tools 2026 (use aka.ms link which always points to latest stable)
set "VS_BUILD_TOOLS_URL=https://aka.ms/vs/18/stable/vs_buildtools.exe"

set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

:: --------- Comprobaciones previas (no cambian nada) ----------
if "%STEP_CHECKS%"=="0" goto :after_checks
echo.
echo [0/8] Comprobando tu PC...
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
if defined FREE_GB (
  echo Espacio libre en el disco de trabajo: !FREE_GB! GB
  echo [%time%] espacio libre !FREE_GB! GB >> "%LOG%"
  if !FREE_GB! LSS 12 (
    call :fallo "Hay menos de 12 GB libres en el disco C. Libera espacio y vuelve a ejecutar."
    exit /b 1
  )
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
  echo.
  for /f "usebackq delims=" %%i in (`%PS% -NoProfile -ExecutionPolicy Bypass -STA -Command ^
    "Add-Type -AssemblyName System.Windows.Forms; " ^
    "$dlg=New-Object System.Windows.Forms.FolderBrowserDialog; " ^
    "$dlg.Description='Elige la carpeta donde se creara Phoenix Link (por ejemplo C:\Program Files)'; " ^
    "$dlg.SelectedPath='%DEFAULT_INSTALL_PARENT%'; " ^
    "$dlg.ShowNewFolderButton=$true; " ^
    "if($dlg.ShowDialog() -eq 'OK'){ $dlg.SelectedPath }"`) do set "INSTALL_PARENT=%%i"
)
if "%INSTALL_PARENT%"=="" set "INSTALL_PARENT=%DEFAULT_INSTALL_PARENT%"

:: Remove trailing slash for non-root paths so name checks are reliable.
if "%INSTALL_PARENT:~-1%"=="\" (
  if not "%INSTALL_PARENT:~1,2%"==":\" set "INSTALL_PARENT=%INSTALL_PARENT:~0,-1%"
)

:: Always install into a "Phoenix Link" subfolder inside the selected path.
set "INSTALL_DIR=%INSTALL_PARENT%\Phoenix Link"
echo.
echo Carpeta elegida: "%INSTALL_PARENT%"
echo Se instalara en: "%INSTALL_DIR%"
echo [%time%] instalar en "%INSTALL_DIR%" commit %COMMIT% >> "%LOG%"
if exist "%INSTALL_DIR%\PhoenixLink.exe" (
  echo Ya hay una instalacion de Phoenix Link ahi: se ACTUALIZA. Tus ajustes y datos no se tocan.
)

:: --------- Work dirs ----------
set "WORK=%LOCALAPPDATA%\SmashSodaInstaller"
set "SRC=%WORK%\src"
set "BUILD=%WORK%\build"
set "SRC_DIR=%SRC%\smash-soda"
set "DEPS_DIR=%SRC_DIR%\dependencies"
set "OVERLAY_TMP=%WORK%\PhoenixGlass.exe.descarga"
set "VIGEMBUS_SETUP=%WORK%\ViGEmBus_1.22.0_x64_x86_arm64.exe"
set "WEBVIEW2_SETUP=%WORK%\MicrosoftEdgeWebview2Setup.exe"
set "VS_BOOTSTRAP=%WORK%\vs_BuildTools.exe"
set "GIT_SETUP=%WORK%\Git-64-bit.exe"
set "CMAKE_SETUP=%WORK%\cmake-windows-x86_64.msi"

if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" "%SRC%" "%BUILD%" 2>nul

goto :main

:: --------- Mensaje de error comun ----------
:fallo
echo.
echo ==========================================================
echo  NO SE PUDO TERMINAR
echo ==========================================================
echo %~1
echo.
echo Tu juego y tus archivos no se tocaron.
echo Manda a Fralex una foto de esta ventana, o este archivo:
echo   %LOG%
echo ==========================================================
echo [%time%] FALLO: %~1 >> "%LOG%"
pause
exit /b 1

:: --------- Helper: download with progress ----------
:download
set "URL=%~1"
set "DEST=%~2"
set "LABEL=%~3"
if "%URL%"=="" (
  echo ERROR: falta la direccion de descarga de "%LABEL%".
  exit /b 1
)
echo Descargando %LABEL%...
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$u='%URL%'; $d='%DEST%'; $l='%LABEL%'; " ^
  "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " ^
  "try { " ^
  "  Write-Progress -Activity $l -Status 'Descargando...' -PercentComplete 0; " ^
  "  Invoke-WebRequest -Uri $u -OutFile $d -UseBasicParsing; " ^
  "  Write-Progress -Activity $l -Completed; " ^
  "  Write-Host 'Descargado: ' $l; " ^
  "} catch { " ^
  "  try { " ^
  "    Write-Host 'Fallo la descarga normal. Probando con BITS...'; " ^
  "    Start-BitsTransfer -Source $u -Destination $d -DisplayName $l; " ^
  "  } catch { " ^
  "    Write-Host 'Fallo la descarga:'; " ^
  "    Write-Host $_.Exception.Message; " ^
  "    exit 1; " ^
  "  } " ^
  "}"
if errorlevel 1 exit /b 1
exit /b 0

:: --------- Helper: download latest GitHub release asset by regex ----------
:download_latest_github_asset
set "GH_REPO=%~1"
set "GH_REGEX=%~2"
set "GH_DEST=%~3"
set "GH_LABEL=%~4"
if "%GH_REPO%"=="" (
  echo ERROR: falta el repositorio de GitHub para "%GH_LABEL%".
  exit /b 1
)
if "%GH_REGEX%"=="" (
  echo ERROR: falta el patron del archivo para "%GH_LABEL%".
  exit /b 1
)
if "%GH_DEST%"=="" (
  echo ERROR: falta el destino para "%GH_LABEL%".
  exit /b 1
)
echo Descargando %GH_LABEL%...
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$repo='%GH_REPO%'; $regex='%GH_REGEX%'; $dest='%GH_DEST%'; " ^
  "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " ^
  "try { " ^
  "  $api = 'https://api.github.com/repos/' + $repo + '/releases/latest'; " ^
  "  $rel = Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent'='SmashSodaInstaller' }; " ^
  "  $asset = $rel.assets | Where-Object { $_.name -match $regex } | Select-Object -First 1; " ^
  "  if (-not $asset) { throw 'No hay un archivo que coincida en la ultima version.' } " ^
  "  Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $dest -UseBasicParsing; " ^
  "} catch { " ^
  "  Write-Host 'Fallo la descarga:'; " ^
  "  Write-Host $_.Exception.Message; " ^
  "  exit 1; " ^
  "}"
if errorlevel 1 exit /b 1
exit /b 0

:main

:: --------- Install Git ----------
echo.
echo [1/8] Instalando Git...
if "%STEP_INSTALL_GIT%"=="0" (
  echo Saltado [1/8] Git.
  goto :after_install_git
)
where git >nul 2>&1
if %errorlevel% neq 0 (
  where winget >nul 2>&1
  if %errorlevel% equ 0 (
    echo Usando winget para instalar Git...
    winget install -e --id Git.Git --silent --accept-package-agreements --accept-source-agreements
    set "WINGET_GIT_RC=!errorlevel!"
    if not "!WINGET_GIT_RC!"=="0" if not "!WINGET_GIT_RC!"=="3010" (
      echo AVISO: winget no pudo instalar Git, codigo !WINGET_GIT_RC!.
      echo Probando con la descarga directa de Git...
      call :download_latest_github_asset "git-for-windows/git" "^Git-.*-64-bit\.exe$" "%GIT_SETUP%" "Git for Windows"
      if errorlevel 1 (
        call :fallo "No se pudo bajar el instalador de Git."
        exit /b 1
      )
      start "" /wait "%GIT_SETUP%" /VERYSILENT /NORESTART /NOCANCEL /SP-
      if errorlevel 1 (
        call :fallo "El instalador de Git fallo."
        exit /b 1
      )
    )
  ) else (
    echo No hay winget. Probando con la descarga directa de Git...
    call :download_latest_github_asset "git-for-windows/git" "^Git-.*-64-bit\.exe$" "%GIT_SETUP%" "Git for Windows"
    if errorlevel 1 (
      call :fallo "No se pudo bajar el instalador de Git."
      exit /b 1
    )
    start "" /wait "%GIT_SETUP%" /VERYSILENT /NORESTART /NOCANCEL /SP-
    if errorlevel 1 (
      call :fallo "El instalador de Git fallo."
      exit /b 1
    )
  )
)

:: Re-check git availability, try common install path if PATH is stale
where git >nul 2>&1
if %errorlevel% neq 0 (
  if exist "%ProgramFiles%\Git\bin\git.exe" (
    set "PATH=%ProgramFiles%\Git\bin;%PATH%"
  ) else if exist "%ProgramFiles(x86)%\Git\bin\git.exe" (
    set "PATH=%ProgramFiles(x86)%\Git\bin;%PATH%"
  )
)
where git >nul 2>&1
if %errorlevel% neq 0 (
  call :fallo "Git no se instalo o no se encuentra. Instalalo a mano desde https://git-scm.com/download/win y repite."
  exit /b 1
)
echo Git listo.
echo [%time%] git ok >> "%LOG%"
:after_install_git

:: --------- Install Build Tools ----------
echo.
echo [2/8] Instalando Visual Studio Build Tools (compilador de C++)...
if "%STEP_INSTALL_BUILD_TOOLS%"=="0" (
  echo Saltado [2/8] Build Tools.
  goto :after_install_build_tools
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_INSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\setup.exe"
set "VSINSTALL="
set "VSINSTALL_COMPLETE="

set "VS_ADDS=--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.VC.ATL --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended"

if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`
    "%VSWHERE%" -latest -products * ^
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.ATL ^
    -property installationPath
  `) do set "VSINSTALL_COMPLETE=%%i"
  for /f "usebackq tokens=*" %%i in (`
    "%VSWHERE%" -latest -products * -property installationPath
  `) do set "VSINSTALL=%%i"
)

if defined VSINSTALL_COMPLETE (
  set "VSINSTALL=!VSINSTALL_COMPLETE!"
  echo Ya hay un compilador de C++ con ATL. No se instala otro.
  goto :vs_ready
)
if defined VSINSTALL goto :vs_modify
goto :vs_fresh_install

:vs_modify
echo.
echo Hay un Visual Studio instalado, pero le faltan componentes de C++ necesarios
echo para compilar Phoenix Link, por ejemplo ATL. Se los agrega ahora. Puede
echo tardar varios minutos. Espera, por favor...
echo "%VSINSTALL%"
if not exist "%VS_INSTALLER%" (
  echo AVISO: no encuentro el instalador de Visual Studio en:
  echo "%VS_INSTALLER%"
  echo No se pueden agregar los componentes solos. Si la compilacion falla,
  echo abre el instalador de Visual Studio, elige Modificar y agrega
  echo "C++ ATL for latest build tools".
  goto :vs_ready
)
"%VS_INSTALLER%" modify --installPath "%VSINSTALL%" --quiet --norestart %VS_ADDS%
set "VS_MODIFY_RC=!errorlevel!"
if not "!VS_MODIFY_RC!"=="0" if not "!VS_MODIFY_RC!"=="3010" (
  echo AVISO: Visual Studio no pudo agregar los componentes, codigo !VS_MODIFY_RC!.
  echo Si la compilacion falla por falta de ATL, abre el instalador de Visual
  echo Studio, elige Modificar y agrega "C++ ATL for latest build tools".
)
goto :vs_ready

:vs_fresh_install
  echo No hay Visual Studio Build Tools. Instalando...
  echo Esto tarda varios minutos. Espera, por favor...
  echo.

  where winget >nul 2>&1
  if %errorlevel% equ 0 (
    echo Usando winget para instalar Visual Studio Build Tools 2026 con ATL...
    winget install -e --id Microsoft.VisualStudio.BuildTools ^
      --override "--wait --quiet --norestart %VS_ADDS%"
    set "WINGET_RC=!errorlevel!"
    if not "!WINGET_RC!"=="0" if not "!WINGET_RC!"=="3010" (
      echo AVISO: winget no pudo instalarlo, codigo !WINGET_RC!.
      echo Probando con el instalador directo de Visual Studio Build Tools...
      call :download "%VS_BUILD_TOOLS_URL%" "%VS_BOOTSTRAP%" "Visual Studio Build Tools"
      if errorlevel 1 (
        call :fallo "No se pudo bajar el instalador de Visual Studio Build Tools."
        exit /b 1
      )

      echo Ejecutando el instalador de Visual Studio Build Tools con ATL...
      "%VS_BOOTSTRAP%" --wait --quiet --norestart %VS_ADDS%
      set "VSBT_RC=!errorlevel!"
      if not "!VSBT_RC!"=="0" if not "!VSBT_RC!"=="3010" (
        call :fallo "El instalador de Visual Studio Build Tools fallo, codigo !VSBT_RC!."
        exit /b 1
      )
    )
  ) else (
    echo Bajando el instalador de Visual Studio Build Tools...
    call :download "%VS_BUILD_TOOLS_URL%" "%VS_BOOTSTRAP%" "Visual Studio Build Tools"
    if errorlevel 1 (
      call :fallo "No se pudo bajar el instalador de Visual Studio Build Tools."
      exit /b 1
    )

    echo Ejecutando el instalador de Visual Studio Build Tools con ATL...
    "%VS_BOOTSTRAP%" --wait --quiet --norestart %VS_ADDS%
    set "VSBT_RC=!errorlevel!"
    if not "!VSBT_RC!"=="0" if not "!VSBT_RC!"=="3010" (
      call :fallo "El instalador de Visual Studio Build Tools fallo, codigo !VSBT_RC!."
      exit /b 1
    )
  )

  echo Esperando a que termine la instalacion...
  timeout /t 10 >nul

  :: Wait for vswhere to appear
  set WAIT_COUNT=0
  :wait_vs
  if !WAIT_COUNT! GEQ 60 (
    echo.
    echo AVISO: no se pudo confirmar la instalacion de Build Tools a tiempo.
    echo Se sigue igual. Si la compilacion falla, reinicia el PC y repite.
    goto :after_install_build_tools
  )

  timeout /t 5 >nul
  set /a WAIT_COUNT+=1
  echo Comprobando la instalacion... (!WAIT_COUNT!/60)

  if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`
      "%VSWHERE%" -latest -products * ^
      -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.ATL ^
      -property installationPath
    `) do set "VSINSTALL=%%i"
  )

  if not defined VSINSTALL goto wait_vs
goto :vs_ready

:vs_ready

echo.
echo Visual Studio Build Tools en:
echo "%VSINSTALL%"

:: Initialize the VS environment
echo.
echo Preparando el entorno de compilacion...
if exist "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" (
  call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
  if !errorlevel! neq 0 (
    echo AVISO: no se pudo preparar el entorno de Visual Studio.
    echo Se sigue igual. Si la compilacion falla, reinicia el PC y repite.
    goto :after_install_build_tools
  )
) else (
  echo AVISO: no encuentro VsDevCmd.bat en:
  echo "%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
  echo Se sigue igual. Si la compilacion falla, reinicia el PC y repite.
  goto :after_install_build_tools
)

echo Entorno de C++ listo.
echo [%time%] visual studio ok >> "%LOG%"
:after_install_build_tools

:: --------- Install CMake ----------
echo.
echo [3/8] Instalando CMake...
:: Use the native Windows (Kitware) CMake via its full path. never a PATH "cmake",
:: which may be an MSYS2/MinGW build (Unix Makefiles, no VS generator => build fails).
set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if "%STEP_INSTALL_CMAKE%"=="0" (
  echo Saltado [3/8] CMake.
  goto :after_install_cmake
)
:: 1) Try winget first if it is available (fast).
if not exist "%CMAKE_EXE%" (
  where winget >nul 2>&1
  if !errorlevel! equ 0 (
    echo Usando winget para instalar CMake...
    winget install -e --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
    timeout /t 5 >nul
  )
)

:: 2) If the native CMake still is not at the expected path (no winget, winget failed,
::    or it was installed elsewhere), download Kitware's MSI and install it there.
if not exist "%CMAKE_EXE%" (
  echo Bajando CMake directo de Kitware...
  call :download_latest_github_asset "Kitware/CMake" "^cmake-.*-windows-x86_64\.msi$" "%CMAKE_SETUP%" "CMake"
  if errorlevel 1 (
    call :fallo "No se pudo bajar el instalador de CMake."
    exit /b 1
  )
  msiexec /i "%CMAKE_SETUP%" /qn /norestart ADD_CMAKE_TO_PATH=System
  if errorlevel 1 (
    call :fallo "El instalador de CMake fallo."
    exit /b 1
  )
  timeout /t 5 >nul
)

if not exist "%CMAKE_EXE%" (
  call :fallo "No encuentro CMake en %CMAKE_EXE% despues de instalarlo. Instalalo desde https://cmake.org/download/ y repite."
  exit /b 1
)
echo CMake listo: "%CMAKE_EXE%"
echo [%time%] cmake ok >> "%LOG%"
:after_install_cmake

:: --------- ViGEmBus driver ----------
echo.
echo [4/8] Controlador de mandos virtuales ViGEmBus...
if "%STEP_INSTALL_VIGEMBUS%"=="0" (
  echo Saltado [4/8] ViGEmBus.
  goto :after_install_vigembus
)
reg query "HKLM\SYSTEM\CurrentControlSet\Services\ViGEmBus" >nul 2>&1
if not errorlevel 1 (
  echo ViGEmBus ya esta instalado. No se vuelve a instalar.
  goto :after_install_vigembus
)
call :download "%VIGEMBUS_URL%" "%VIGEMBUS_SETUP%" "ViGEmBus Driver"
if errorlevel 1 (
  call :fallo "No se pudo bajar ViGEmBus."
  exit /b 1
)
if not exist "%VIGEMBUS_SETUP%" (
  call :fallo "No se encuentra el instalador de ViGEmBus descargado."
  exit /b 1
)
start "" /wait "%VIGEMBUS_SETUP%" /qn
if errorlevel 1 (
  call :fallo "El instalador de ViGEmBus fallo."
  exit /b 1
)
echo [%time%] vigembus ok >> "%LOG%"
:after_install_vigembus

:: --------- WebView2 (motor de la interfaz) ----------
echo.
echo [5/8] Motor de la interfaz WebView2...
if "%STEP_INSTALL_WEBVIEW2%"=="0" (
  echo Saltado [5/8] WebView2.
  goto :after_install_webview2
)
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
  echo WebView2 ya esta instalado.
  goto :after_install_webview2
)
call :download "%WEBVIEW2_URL%" "%WEBVIEW2_SETUP%" "WebView2 Runtime"
if errorlevel 1 (
  echo AVISO: no se pudo bajar WebView2. Phoenix Link usara su interfaz simple hasta que lo instales desde https://developer.microsoft.com/microsoft-edge/webview2/
  goto :after_install_webview2
)
start "" /wait "%WEBVIEW2_SETUP%" /silent /install
echo [%time%] webview2 instalado >> "%LOG%"
:after_install_webview2

:: --------- Clone source ----------
echo.
echo [6/8] Descargando Phoenix Link (version %COMMIT:~0,7%)...
if "%STEP_CLONE_SOURCE%"=="0" (
  echo Saltado [6/8] Descarga del codigo.
  goto :after_clone_source
)
if exist "%SRC_DIR%" rmdir /s /q "%SRC_DIR%"
mkdir "%SRC_DIR%" 2>nul
git -C "%SRC_DIR%" init -q
git -C "%SRC_DIR%" remote add origin "%REPO_URL%"
git -C "%SRC_DIR%" fetch --depth 1 origin %COMMIT%
if %errorlevel% neq 0 goto :clone_fallo
git -C "%SRC_DIR%" checkout -q --detach FETCH_HEAD
:clone_fallo
if %errorlevel% neq 0 (
  call :fallo "No se pudo descargar el codigo de Phoenix Link desde GitHub."
  exit /b 1
)
set "DEPS_DIR=%SRC_DIR%\dependencies"
if not exist "%DEPS_DIR%" set "DEPS_DIR=%SRC_DIR%\Dependencies"
echo Codigo descargado.
echo [%time%] codigo descargado >> "%LOG%"
:after_clone_source

:: --------- Build ----------
echo.
echo [7/8] Compilando Phoenix Link (la parte lenta)...
if "%STEP_BUILD%"=="0" (
  echo Saltado [7/8] Compilacion.
  goto :after_build
)
"%CMAKE_EXE%" -S "%SRC_DIR%" -B "%BUILD%" -A x64
if %errorlevel% neq 0 (
  call :fallo "CMake no pudo preparar la compilacion."
  exit /b 1
)

echo.
echo Generador de CMake:
"%CMAKE_EXE%" -LA -N "%BUILD%" | findstr /C:"CMAKE_GENERATOR"

echo.
mkdir "%SRC_DIR%\x64\release" 2>nul
"%CMAKE_EXE%" --build "%BUILD%" --config Release
if %errorlevel% neq 0 (
  call :fallo "La compilacion fallo. Si dice LNK1104, Phoenix Link esta abierto: cierralo y repite."
  exit /b 1
)
echo [%time%] compilacion ok >> "%LOG%"
:after_build

:: --------- Install ----------
if "%STEP_INSTALL_APP%"=="0" (
  echo.
  echo Saltado [8/8] Copia de archivos.
  goto :after_install_app
)
set "RELEASE_DIR="
if exist "%SRC_DIR%\x64\release\PhoenixLink.exe" set "RELEASE_DIR=%SRC_DIR%\x64\release"
if not defined RELEASE_DIR if exist "%SRC_DIR%\x64\Release\PhoenixLink.exe" set "RELEASE_DIR=%SRC_DIR%\x64\Release"
if not defined RELEASE_DIR if exist "%BUILD%\Release\PhoenixLink.exe" set "RELEASE_DIR=%BUILD%\Release"
if not defined RELEASE_DIR if exist "%BUILD%\SmashSoda\Release\PhoenixLink.exe" set "RELEASE_DIR=%BUILD%\SmashSoda\Release"
if not defined RELEASE_DIR (
  for /f "delims=" %%F in ('dir /b /s "%BUILD%\PhoenixLink.exe" 2^>nul') do (
    if not defined RELEASE_DIR set "RELEASE_DIR=%%~dpF"
  )
)
if not defined RELEASE_DIR (
  call :fallo "No encuentro PhoenixLink.exe despues de compilar."
  exit /b 1
)

echo.
echo [8/8] Instalando Phoenix Link...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
xcopy "%RELEASE_DIR%\*" "%INSTALL_DIR%\" /E /I /Y
if %errorlevel% neq 0 (
  call :fallo "No se pudieron copiar los archivos a %INSTALL_DIR%."
  exit /b 1
)
:: Comprobacion final: lo imprescindible tiene que estar
set "FALTA="
for %%F in (PhoenixLink.exe parsec.dll SDL2.dll WebView2Loader.dll ui\index.html sider\phoenix.lua sider\phoenix_referee.lua) do (
  if not exist "%INSTALL_DIR%\%%F" set "FALTA=!FALTA! %%F"
)
if defined FALTA (
  call :fallo "Faltan archivos en la instalacion:!FALTA!"
  exit /b 1
)
echo [%time%] instalado en "%INSTALL_DIR%" >> "%LOG%"
:after_install_app

:: --------- Migrate user data ----------
:: Solo copia lo que NO exista ya en la carpeta de datos de Phoenix Link (nunca pisa nada).
if "%STEP_MIGRATE_DATA%"=="0" (
  echo.
  echo Saltado: traer datos de una version anterior.
  goto :after_migrate_user_data
)
echo.
echo Buscando datos de Smash Soda para traerlos...
set "OLD_APPDATA_DIR=%APPDATA%\SmashSodaTwo"
set "NEW_APPDATA_DIR=%APPDATA%\Trybuchet\Smash Soda"
if exist "%OLD_APPDATA_DIR%" (
  if not exist "%NEW_APPDATA_DIR%" mkdir "%NEW_APPDATA_DIR%"
  for %%F in (
    banned.json
    banned_ip_addresses.json
    games.json
    preferences.json
    mods.json
    profiles.json
    session.json
    tiers.json
    vip.json
  ) do (
    if exist "%OLD_APPDATA_DIR%\%%F" (
      if not exist "%NEW_APPDATA_DIR%\%%F" (
        copy "%OLD_APPDATA_DIR%\%%F" "%NEW_APPDATA_DIR%\%%F" >nul
        if !errorlevel! equ 0 (
          echo Traido %%F
        ) else (
          echo AVISO: no se pudo traer %%F
        )
      )
    )
  )
) else (
  echo No hay datos anteriores en "%OLD_APPDATA_DIR%". Nada que traer.
)
:after_migrate_user_data

:: --------- Overlay PhoenixGlass ----------
set "OVERLAY_DIR=%INSTALL_DIR%\overlay"
set "OVERLAY_ESTADO=no instalado"
if "%STEP_INSTALL_OVERLAY%"=="0" (
  echo.
  echo Saltado: overlay.
  goto :after_install_overlay
)
echo.
echo Instalando el overlay PhoenixGlass...
mkdir "%OVERLAY_DIR%" 2>nul
mkdir "%OVERLAY_DIR%\plugins" 2>nul
mkdir "%OVERLAY_DIR%\themes" 2>nul
:: Descarga a un archivo temporal, comprueba el sha256 y solo entonces lo pone en su sitio.
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$u=$env:PHX_OVERLAY_URL; $s=$env:PHX_OVERLAY_SHA; $t='%OVERLAY_TMP%'; $d='%OVERLAY_DIR%\PhoenixGlass.exe'; " ^
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
  echo        Si ya tenias un overlay en esa carpeta, se deja como estaba.
)
echo [%time%] overlay !OVERLAY_ESTADO! >> "%LOG%"
:after_install_overlay

echo.
echo ============================================================
echo  INSTALACION TERMINADA
echo ============================================================
echo Phoenix Link instalado en: "%INSTALL_DIR%"
echo Overlay: %OVERLAY_ESTADO%
echo.
echo Siguiente paso: abre Phoenix Link, ve a SYNC y luego a
echo Modulos del juego, y pulsa INSTALAR (con PES 2021 cerrado).
echo ============================================================
echo.

if "%STEP_CREATE_SHORTCUTS%"=="0" (
  echo Saltado: acceso directo.
) else (
  call :shortcuts
)

echo.
echo Limpiando archivos temporales...
if "%STEP_CLEANUP%"=="0" (
  echo Saltado: limpieza.
) else (
  rmdir /s /q "%WORK%"
)
echo [%time%] fin correcto >> "%LOG%"

echo.
echo Pulsa una tecla para salir...
pause >nul
exit /b 0

:: --------- Optional shortcut ----------
:shortcuts
setlocal EnableExtensions DisableDelayedExpansion
set "APP_NAME=Phoenix Link"
set "APP_EXE=%INSTALL_DIR%\PhoenixLink.exe"
if not exist "%APP_EXE%" set "APP_EXE=%RELEASE_DIR%\PhoenixLink.exe"

if not exist "%APP_EXE%" (
  echo AVISO: no encuentro PhoenixLink.exe para crear el acceso directo.
  exit /b 0
)

if not defined PHX_ACCESO_DIRECTO (
  choice /C SN /N /M "Crear un acceso directo en el escritorio? [S/N]: "
  if errorlevel 2 (
    echo Sin acceso directo.
    exit /b 0
  )
)

:: Resolve the current user's Desktop folder properly
for /f "usebackq tokens=*" %%D in (`%PS% -NoProfile -Command "[Environment]::GetFolderPath('Desktop')"`) do set "DESKTOP_PATH=%%D"

if not exist "%DESKTOP_PATH%" mkdir "%DESKTOP_PATH%"

:: Create shortcut
%PS% -NoProfile -ExecutionPolicy Bypass -Command ^
  "$w=New-Object -ComObject WScript.Shell;" ^
  "$d='%DESKTOP_PATH%\%APP_NAME%.lnk';" ^
  "$s=$w.CreateShortcut($d);" ^
  "$s.TargetPath='%APP_EXE%';" ^
  "$s.WorkingDirectory='%INSTALL_DIR%';" ^
  "$s.IconLocation='%APP_EXE%';" ^
  "$s.Save();"

echo Acceso directo creado en el escritorio.
exit /b 0
