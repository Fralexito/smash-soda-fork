@echo off
setlocal
rem Phoenix Evolution - elegir el modo de recarga de phoenix.lua (v0.18).
rem Este archivo va en <juego>\SiderAddons\content\phoenix\ y escribe ahi modo.txt.
rem Cuando exista el boton en Phoenix Link, Link hara lo mismo que este archivo.
set "DIR=%~dp0"
echo.
echo   PHOENIX - modo de recarga
echo.
echo   1 = ACTIVAR (el de por defecto)
echo       Recarga SOLO cuando pulsas Partido - Datos Actual. en vivo - Activar.
echo.
echo   2 = AUTO-FICHAJES
echo       Activar sigue funcionando y ADEMAS recarga sola cuando hay fichajes nuevos
echo       (al volver al menu principal y entrar a un modo).
echo.
echo   3 = AUTO-SIEMPRE
echo       Activar sigue funcionando y ADEMAS recarga sola CADA VEZ que vuelves
echo       al menu principal y entras a un modo.
echo.
choice /c 123 /n /m "  Elige 1, 2 o 3: "
if errorlevel 3 (set "MODO=AUTO-SIEMPRE") else if errorlevel 2 (set "MODO=AUTO-FICHAJES") else (set "MODO=ACTIVAR")
<nul set /p "=%MODO%" > "%DIR%modo.tmp"
move /y "%DIR%modo.tmp" "%DIR%modo.txt" >nul
echo.
echo   Listo: modo %MODO% guardado en
echo   %DIR%modo.txt
echo   Si el juego esta abierto, se aplica en unos 2 segundos.
echo.
pause
