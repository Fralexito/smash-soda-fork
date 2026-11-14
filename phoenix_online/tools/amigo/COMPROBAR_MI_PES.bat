@echo off
rem Phoenix Online - COMPROBAR_MI_PES: SOLO LEE (no cambia nada). Muestra tamanos y huellas del PES.
rem Doble clic. No necesita administrador.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0COMPROBAR_MI_PES.ps1" %*
echo.
pause
