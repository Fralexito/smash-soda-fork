@echo off
rem Uso: ZZTEST.bat antes  |  ZZTEST.bat despues
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ZZTEST.ps1" %1
pause
