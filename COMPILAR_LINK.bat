@echo off
chcp 65001 >nul
setlocal
REM =============================================================================
REM  Phoenix Link · compilar SIN BORRAR NADA (doble clic)
REM  - No hace git checkout ni git pull: compila lo que hay en esta carpeta tal cual.
REM    (COMPILAR_PHOENIX.bat hace checkout --force y BORRA lo que no esta commiteado: no usarlo.)
REM  - Si existe C:\dev\phoenix-overlay\bin\PhoenixGlass.exe, lo copia a la carpeta overlay del programa.
REM  - Resultado: x64\Release\PhoenixLink.exe  (con ui\, sider\ y overlay\ al lado)
REM =============================================================================
cd /d "%~dp0"
echo.
echo  === Phoenix Link: compilar (sin tocar tus cambios) ===
echo.

where cmake >nul 2>nul
if errorlevel 1 (
  echo  Falta CMake. Instalalo con:  winget install -e --id Kitware.CMake
  echo  y vuelve a abrir este archivo.
  pause
  exit /b 1
)

if not exist build\CMakeCache.txt (
  echo  Primera vez: preparando la carpeta build...
  cmake -S . -B build -A x64
  if errorlevel 1 goto error
)

echo  Compilando (tarda de 1 a 10 minutos)...
cmake --build build --config Release
if errorlevel 1 goto error

if exist "C:\dev\phoenix-overlay\bin\PhoenixGlass.exe" (
  if not exist "x64\Release\overlay" mkdir "x64\Release\overlay"
  copy /y "C:\dev\phoenix-overlay\bin\PhoenixGlass.exe" "x64\Release\overlay\PhoenixGlass.exe" >nul
  echo  Overlay PhoenixGlass copiado a x64\Release\overlay\
)

echo.
echo  LISTO: %~dp0x64\Release\PhoenixLink.exe
echo  Cierra Phoenix Link si estaba abierto y abre ese archivo.
echo.
pause
exit /b 0

:error
echo.
echo  ERROR al compilar. No se borro nada. Copia lo que dice arriba y mandaselo a Claude.
echo.
pause
exit /b 1
