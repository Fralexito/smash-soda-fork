@echo off
chcp 65001 >nul
cd /d "%~dp0"
set "CM=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CM%" set "CM=cmake"
set "WT=%~dp0_phoenix-link"
set "LOG=%~dp0log_phoenix.txt"

echo ============================================================
echo  Phoenix Link - compilacion (copia aparte, no toca otras ramas)
echo ============================================================

taskkill /f /im PhoenixSoda.exe >nul 2>&1
taskkill /f /im PhoenixLink.exe >nul 2>&1

echo [1/4] Descargando la ultima version desde GitHub...
git fetch origin
if errorlevel 1 (echo FALLO al descargar con git fetch & pause & exit /b 1)

set "SHA="
for /f %%H in ('git for-each-ref --format^=%%^(objectname^) "refs/remotes/origin/redise*"') do set "SHA=%%H"
if "%SHA%"=="" (echo FALLO: no encuentro la rama del rediseno en GitHub & pause & exit /b 1)

findstr /c:"_phoenix-link" ".git\info\exclude" >nul 2>&1 || echo _phoenix-link/>>".git\info\exclude"

echo [2/4] Preparando la copia de trabajo de Phoenix Link...
if not exist "%WT%\.git" (
  git worktree add --detach "%WT%" %SHA%
) else (
  git -C "%WT%" checkout --detach --force %SHA%
)
if errorlevel 1 (echo FALLO al preparar la copia de trabajo & pause & exit /b 1)

echo [3/4] Configurando CMake (solo la primera vez tarda)...
if not exist "%WT%\build\CMakeCache.txt" (
  "%CM%" -S "%WT%" -B "%WT%\build" -G "Visual Studio 18 2026" > "%LOG%" 2>&1
  if errorlevel 1 (echo FALLO al configurar: revisa %LOG% & pause & exit /b 1)
)

echo [4/4] Compilando, espera unos minutos...
"%CM%" --build "%WT%\build" --config Release --parallel >> "%LOG%" 2>&1
set "RES=%errorlevel%"
findstr /i /c:" error " /c:"Error(s)" /c:"Elapsed" "%LOG%"
echo.
if "%RES%"=="0" (
  echo EXITO: compilacion correcta
  echo Programa: %WT%\x64\Release\PhoenixLink.exe
  start "" explorer "%WT%\x64\Release"
) else (
  echo FALLO: sube el archivo %LOG%
)
echo Terminado.
pause
