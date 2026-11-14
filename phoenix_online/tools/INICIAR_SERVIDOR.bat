@echo off
rem Abre Phoenix Online y lo reabre solo si se cae. Ctrl+C para cerrarlo.
rem Copia este archivo a la carpeta del servidor (donde esta config.json).
cd /d "%~dp0"
title Phoenix Online (servidor)
python tools\arrancar_servidor.py config.json
pause
