@echo off
rem Phoenix Evolution · enviar un aviso al juego (modulo phoenix.lua de Sider). Se puede usar con el juego abierto.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PhoenixAviso.ps1" %*
pause
