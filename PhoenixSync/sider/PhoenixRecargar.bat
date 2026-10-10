@echo off
setlocal
rem Phoenix Evolution - aviso de "hay fichajes nuevos" para phoenix.lua (v0.18).
rem Este archivo va en <juego>\SiderAddons\content\phoenix\ y escribe ahi recargar.txt.
rem Cuando Phoenix Sync meta fichajes, hara lo mismo que este archivo (solo para probar).
set "DIR=%~dp0"
<nul set /p "=%date% %time% %random%" > "%DIR%recargar.tmp"
move /y "%DIR%recargar.tmp" "%DIR%recargar.txt" >nul
echo.
echo   Aviso de fichajes nuevos escrito en
echo   %DIR%recargar.txt
echo.
echo   En modo AUTO-FICHAJES o AUTO-SIEMPRE, el juego recargara solo
echo   la proxima vez que estes en el menu principal y entres a un modo.
echo   En modo ACTIVAR este aviso no hace nada.
echo.
pause
