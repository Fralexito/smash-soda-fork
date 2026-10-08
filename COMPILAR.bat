@echo off
echo Compilando SmashSoda...
cd /d "C:\Users\WinterOS\smash-soda-fork"
cd build
cmake -G "Visual Studio 18 2026" -A x64 ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)
echo.
echo Compilando...
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)
echo.
echo EXITO: Compilacion completada
echo.
echo Ejecutando PhoenixLink.exe...
cd "C:\Users\WinterOS\smash-soda-fork\build\SmashSoda\Release"
if exist PhoenixLink.exe (
    echo Iniciando aplicacion...
    start PhoenixLink.exe
) else (
    echo ERROR: PhoenixLink.exe no encontrado
    pause
)
