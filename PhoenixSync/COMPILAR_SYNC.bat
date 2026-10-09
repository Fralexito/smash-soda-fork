@echo off
chcp 65001 >nul
cd /d "%~dp0.."
set "CM=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CM%" set "CM=cmake"
set "LOG=%~dp0..\build-sync\compilar_sync_log.txt"
if not exist "build-sync" mkdir "build-sync"

echo ============================================================
echo  Phoenix Sync - compilacion (build-sync\, no toca Phoenix Link)
echo ============================================================
echo INICIO %date% %time% > "%LOG%"

echo [1/3] Configurando...
"%CM%" -S PhoenixSync -B build-sync -A x64 >> "%LOG%" 2>&1
if errorlevel 1 (echo FALLO al configurar, mira build-sync\compilar_sync_log.txt & echo FIN CONFIGURAR_FALLO >> "%LOG%" & pause & exit /b 1)

echo [2/3] Compilando (Release)...
"%CM%" --build build-sync --config Release >> "%LOG%" 2>&1
if errorlevel 1 (echo FALLO al compilar, mira build-sync\compilar_sync_log.txt & echo FIN COMPILAR_FALLO >> "%LOG%" & pause & exit /b 1)

echo [3/3] Pruebas...
echo ===== PRUEBAS >> "%LOG%"
"build-sync\Release\PhoenixSyncPruebas.exe" >> "%LOG%" 2>&1
if errorlevel 1 (echo ALGUNA PRUEBA FALLO, mira build-sync\compilar_sync_log.txt & echo FIN PRUEBAS_FALLO >> "%LOG%" & pause & exit /b 1)

echo FIN OK >> "%LOG%"
echo.
echo Todo bien. Ejecutables en build-sync\Release\
pause
