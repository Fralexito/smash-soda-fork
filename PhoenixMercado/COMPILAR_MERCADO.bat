@echo off
chcp 65001 >nul
cd /d "%~dp0.."
set "CM=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CM%" set "CM=cmake"
set "LOG=%~dp0..\build-mercado\compilar_mercado_log.txt"
if not exist "build-mercado" mkdir "build-mercado"

echo ============================================================
echo  Phoenix Mercado - compilacion (build-mercado\, no toca Phoenix Link)
echo ============================================================
echo INICIO %date% %time% > "%LOG%"

echo [1/3] Configurando...
"%CM%" -S PhoenixMercado -B build-mercado -A x64 >> "%LOG%" 2>&1
if errorlevel 1 (echo FALLO al configurar, mira build-mercado\compilar_mercado_log.txt & echo FIN CONFIGURAR_FALLO >> "%LOG%" & pause & exit /b 1)

echo [2/3] Compilando (Release)...
"%CM%" --build build-mercado --config Release >> "%LOG%" 2>&1
if errorlevel 1 (echo FALLO al compilar, mira build-mercado\compilar_mercado_log.txt & echo FIN COMPILAR_FALLO >> "%LOG%" & pause & exit /b 1)

echo [3/3] Pruebas...
echo ===== PRUEBAS >> "%LOG%"
"build-mercado\Release\PhoenixMercadoPruebas.exe" >> "%LOG%" 2>&1
if errorlevel 1 (echo ALGUNA PRUEBA FALLO, mira build-mercado\compilar_mercado_log.txt & echo FIN PRUEBAS_FALLO >> "%LOG%" & pause & exit /b 1)

echo FIN OK >> "%LOG%"
echo.
echo Todo bien. Ejecutables en build-mercado\Release\
pause
